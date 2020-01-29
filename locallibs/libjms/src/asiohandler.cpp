#ifdef HAVE_AMQP_CPP
#include <iostream>

#include "asiohandler.hpp"
#include <amqpcpp/bytebuffer.h>
#include <boost/asio/buffer.hpp>
#include "jms_log.hpp"
#include "errors.hpp"

#ifdef HAVE_SSL
#ifdef BOOST_ASIO_NO_OPENSSL_STATIC_INIT

namespace boost {
namespace asio {
namespace ssl {
namespace detail {
boost::asio::detail::shared_ptr<openssl_init<true>::do_init> openssl_init<true>::singleton_;
openssl_init<true>::mutex openssl_init<true>::mtx_;
} // namespace detail
} // namespace ssl
} // namespace asio
} // namespace boost
#endif //BOOST_ASIO_NO_OPENSSL_AUTO_INIT
#endif // HAVE_SSL


#undef LDEBUG2
#define LDEBUG2 jms_null_logger::lout
//#define LDEBUG2 std::cerr << "\n" << this << std::dec << ">>"


/*
              State diagram of asio_handler
=====================================================================

  +--->  Disconnected  -------------------------+
  |           |                                 |
  |           V                                 |
  |       Handshake ----------------------------+
  |           |                                 |
  |           V                                 |
  |       Connected ----------------------------+
  |           |                                 |
  |           V                                 V
  |     Communicating -----------+----> ShuttingDownByErr
  |           |                  |              |
  |           V                  |              V
  |      ClosingComm ------------+         ClosedByErr
  |           |                                 |
  |           V                                 V
  |      ShuttingDown                         Error
  |           |
  |           V
  +--------Closed

=====================================================================
*/


namespace amqp
{

namespace
{
const size_t connect_timeout_sec = 15;
const size_t amqp_connect_timeout_sec = 15;
const size_t amqp_disconnect_timeout_sec = 15;
const size_t stream_buffer_min = 8;
}
using boost::asio::ip::tcp;

class stream_buffer
{
public:
    stream_buffer(const stream_buffer&) = delete;
    stream_buffer& operator=(const stream_buffer&) = delete;
    stream_buffer(size_t size) :
        data_(std::max(size, stream_buffer_min )), used_(0)
    { }

    void on_read_to_buffer_complete(const size_t size)
    {
        used_ += size;
    }

    boost::asio::mutable_buffers_1 get_buffer_for_read_to()
    {
        if (data_.size() == used_)
        {
            data_.resize(data_.size() << 1 );
            LDEBUG2<< "Resizing stream_buffer space to " << data_.size();
        }
        return boost::asio::buffer(data_.data() + used_, data_.size() - used_);
    }

    void clear()
    {
        used_ = 0;
    }

    AMQP::ByteBuffer get_buffer() const
    {
        return AMQP::ByteBuffer(data_.data(), used_);
    }

    void erase_head(const size_t count)
    {
        if (!count) return;
        if ( used_ < count)
            throw std::range_error("Wrong Buffer Erase requested");
        used_ -= count;
        if (used_)
        {
            std::memmove(data_.data(), data_.data() + count, used_);
        }
    }

private:
    std::vector<char> data_;
    size_t used_;
};



asio_handler::asio_handler(boost::asio::io_service& io_service) :
        ext_state_(AmqpDisconnected),
        io_service_(io_service),
        timer_(io_service),
        stream_(),
        stream_buffer_(new stream_buffer(ASIO_INPUT_BUFFER_SIZE)),
        connection_(nullptr),
        writing_(false),
        reading_(false),
        heartbeat_(default_heartbeat),
        connection_state_(Disconnected)
{
    no_errors.clear();
    no_errors.test_and_set();
}

asio_handler::~asio_handler() noexcept
{
    LDEBUG2 << "~asio_handler ";
}

void asio_handler::check_error()
{
    if (!no_errors.test_and_set())
    {
        throw jms::mq_error(-1, err_desc);
    }
}

void asio_handler::connect(const std::string& host, uint16_t port, bool tls, uint16_t heartbeat)
{
    heartbeat_ = heartbeat;
    do_connect(host, port, tls);
}

void asio_handler::set_error(const std::string& desc, const boost::system::error_code& ec)
{

    err_desc = desc;
}

bool asio_handler::handle_error(const boost::system::error_code& e)
{

    if (connection_state_ == Closed || connection_state_ == ClosedByErr)
    {
        do_disconnect_notify();
        return false;
    }

    if(!e)
    {
        return true;
    }
    std::stringstream str;

    LDEBUG2 << " handle error with " << e.message() << "(code = " << e.value() << ") " << connection_state_ ;
    str << e.message() << "(" << e.value() << ")";
    switch(connection_state_)
    {
        case Disconnected:
                set_error("Connect error:" + str.str());
                break;
        case Handshake:
                set_error("Handshake error:" + str.str());
                break;
        case Connected:
        case Communicating:
                set_error("Network error:" + str.str());
                break;
        case ClosingComm:
                set_error("Connection closing error:" + str.str());
                break;
        // for next states disconnect in progress already
        case ShuttingDown:
        case Closed:
        case ShuttingDownByErr:
        case ClosedByErr:
        case Error:
                return false;
        default:
                throw jms::mq_error(-1, "ErrHandler:Invalid state " + std::to_string(connection_state_) );
    }

    connection_state_ = ShuttingDownByErr;
    do_disconnect();
    return false;
}

void asio_handler::do_handshake()
{
    LDEBUG2 << "do_handshake";
    stream_.async_handshake(sirena_net::stream_holder::client,
                            [this](boost::system::error_code ec) {
                                LDEBUG2 << "handshake completed ";
                                if(handle_error(ec))
                                {
                                    connection_state_ = Connected;
                                    do_read();
                                    do_write();
                                    do_timer(amqp_connect_timeout_sec);
                                }
                             });
}


void asio_handler::do_timer(size_t seconds)
{
    LDEBUG2 << "do_timer " << seconds;
    boost::posix_time::time_duration time = boost::posix_time::seconds(seconds);
    timer_.expires_from_now(time);
    timer_.async_wait([this](const boost::system::error_code& ec){ handle_timer(ec); });
}

void asio_handler::handle_timer(const boost::system::error_code& ec)
{
    LDEBUG2 << "handle_timer " << ec ;
    if(!ec)
    {
        switch(connection_state_)
        {
        case Communicating:
            if (heartbeat_)
            {
                size_t diff = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - last_data_tm).count();
                if (diff + 1 >= get_comm_timeout())
                {
                    set_error("connection failed by heartbeat timeout");
                    break;
                }
                do_timer( get_comm_timeout() - diff + 3);
            }
            return;
        case Disconnected:
            set_error("connection failed by timeout");
            break;
        case Handshake:
            set_error("TLS handshake failed by timeout");
            break;
        case Connected:
            set_error("AMQP connection failed by timeout");
            break;
        case ClosingComm:
            set_error("Closing AMQP connection failed by timeout");
            break;
        case ShuttingDown:
        case Closed:
        case ShuttingDownByErr:
        case ClosedByErr:
        case Error:
            break;
        default:
            throw jms::mq_error(-1, "TimerHandler:Invalid state " + std::to_string(connection_state_) );

        }

        connection_state_ = ShuttingDownByErr;
        do_disconnect();

    }
}


void asio_handler::do_connect(const std::string& host, uint16_t port, bool tls)
{
    LDEBUG2<< "do_connect";

    {//locked block
        std::lock_guard<std::mutex> lck(mtx);
        ext_state_ = AmqpNotReady;
    }//locked block

    if (tls)
    {
          LDEBUG2<< "tls requested";
#ifndef HAVE_SSL
        throw std::runtime_error("Built without tls support");
#else
        ctx_.reset(new sirena_net::context_type(net::ssl::context::tlsv12_client));
        ctx_->set_verify_mode(net::ssl::context::verify_none);
#endif
    }
    stream_ = sirena_net::stream_holder::create_stream(io_service_, ctx_);

    std::string s_port = std::to_string(port);

    net::ip::tcp::resolver::query query(host, s_port);
    LDEBUG2<< "resolving";
    net::ip::tcp::resolver::iterator iter = net::ip::tcp::resolver(io_service_).resolve(query);

    do_timer(connect_timeout_sec);
    LDEBUG2<< "connecting";

    net::async_connect(stream_.lowest_layer(), iter,
            [this](boost::system::error_code ec, net::ip::tcp::resolver::iterator)
            {
                LDEBUG2<< "handling connect";
                if (handle_error(ec))
                {
                    connection_state_ = Handshake;
                    do_handshake();
                }
            }

    );

}

void asio_handler::onData(
        AMQP::Connection *connection, const char *data, size_t size)
{
    LDEBUG2<< "onData" ;
    if (
            connection_state_ == ShuttingDownByErr 
        ||  connection_state_ == ClosedByErr
        ||  connection_state_ == Error )
    {
        return;
    }
    connection_ = connection;
    output_buffer_.push_back(std::vector<char>(data, data + size));
    do_write();
}

void asio_handler::do_disconnect(bool is_graceful)
{
    LDEBUG2<< "do_disconnect "  << is_graceful;
    if (is_graceful)
    {
        stream_.shutdown();
    }
    timer_.cancel();
    stream_.close();
    switch (connection_state_)
    {
    case ShuttingDown:      connection_state_ = Closed;      break;
    case ShuttingDownByErr: connection_state_ = ClosedByErr; break;
    default:
        throw jms::mq_error(-1, "Disconnect:Invalid state " + std::to_string(connection_state_) );
    }

    do_disconnect_notify();
}

void asio_handler::do_disconnect_notify()
{
    if ((writing_ || reading_)) return;

    switch (connection_state_)
    {
    case      Closed: connection_state_ = Disconnected; break;
    case ClosedByErr: connection_state_ = Error;        break;
    default:
        throw jms::mq_error(-1, "DiscNotify:Invalid state " + std::to_string(connection_state_) );
    }

    std::lock_guard<std::mutex> lck(mtx);
    ext_state_ = AmqpError;
    if (Disconnected == connection_state_)
    {
        ext_state_ = AmqpDisconnected;
    }
    else
    {
        no_errors.clear();
    }
    LDEBUG2 << "Notifying all about disconnect";
    cv.notify_all();
    //callback should be call inside lock, or its object could be destroyed already
    if (on_disconnect) on_disconnect(connection_state_ != Error);
}

void asio_handler::set_on_disconnect(const std::function<void (bool)>& cb)
{
    LDEBUG2 << "set_on_disconnect";
    on_disconnect = cb;
}

void asio_handler::do_read()
{
    LDEBUG2<< "do_read " << connection_state_;
    if (connection_state_ != Connected && connection_state_ != Communicating && connection_state_ != ClosingComm)
    {
        return;
    }
    LDEBUG2<< "planning read";
    reading_ = true;
    net::async_read(stream_, stream_buffer_->get_buffer_for_read_to(), net::transfer_at_least(1),
            [this](boost::system::error_code ec, std::size_t length)
            {
                LDEBUG2<< "read complete";
                reading_ = false;
                if (handle_error(ec))
                {
                    stream_buffer_->on_read_to_buffer_complete(length);
                    parse_data();
                    do_read();
                }
           });
}

void asio_handler::do_write()
{
    LDEBUG2<< "do_write (" << writing_<< "/" << connection_state_ << "/" << output_buffer_.empty() << ")" ;
    if (
        !writing_
        && (   connection_state_ == Connected
            || connection_state_ == Communicating
            || connection_state_ == ClosingComm
            )
        && (!output_buffer_.empty())
       )
    {
        writing_ = true;
        LDEBUG2<< "planning write" ;
        boost::asio::async_write(stream_,
                boost::asio::buffer(output_buffer_.front()),
                [this](boost::system::error_code ec, std::size_t length )
                {
                    LDEBUG2<< "write complete" ;
                    writing_ = false;
                    if (handle_error(ec))
                    {
                        LDEBUG2<< "Left=" << output_buffer_.size();
                        output_buffer_.pop_front();
                        do_write();
                    }
               });
    }
}

void asio_handler::close()
{
    {//locked block
        std::lock_guard<std::mutex> lck(mtx);
        if (ext_state_ == AmqpError || ext_state_ == AmqpDisconnected)
        {
            return;
        }
    }//locked block
    //this method called from another thread than ioservice.run()
    io_service_.post(
                [this]() {
                    LDEBUG2 << "amqp connection closing";
                    if (!connection_)
                    {
                        connection_state_ = ShuttingDown;
                        do_disconnect(true);
                    }
                    else
                    {
                        connection_state_ = ClosingComm;
                        do_timer(amqp_disconnect_timeout_sec);
                        connection_->close();
                    }
                } );
    wait_for_disconnect();
}

void asio_handler::parse_data()
{
    LDEBUG2<< "parse_data";
    if (!connection_)
    {
        return;
    }
    const size_t count = connection_->parse(stream_buffer_->get_buffer());
    stream_buffer_->erase_head(count);
    last_data_tm = std::chrono::high_resolution_clock::now();
}


bool asio_handler::wait_for_amqp_readiness()
{
    std::unique_lock<std::mutex> lck(mtx);
    if (ext_state_ != AmqpError && ext_state_ != AmqpReady)
    {
        cv.wait(lck);
    }
    return (ext_state_ == AmqpReady);
}

bool asio_handler::wait_for_disconnect()
{
    std::unique_lock<std::mutex> lck(mtx);
    if (ext_state_ != AmqpError && ext_state_ != AmqpDisconnected)
    {
        cv.wait(lck);
    }
    return (ext_state_ == AmqpDisconnected);
}


void asio_handler::break_wait()
{
    cv.notify_all();
}


void asio_handler::onReady(AMQP::Connection *connection)
{
    LDEBUG2 << "onReady";
    connection_state_ = Communicating;
    if (heartbeat_)
    {
        do_timer(get_comm_timeout());
    }
    std::lock_guard<std::mutex> lck(mtx);
    ext_state_ = AmqpReady;
    cv.notify_all();
}

void asio_handler::onError(AMQP::Connection *connection, const char *message)
{
    if (
            connection_state_ == ShuttingDownByErr
        ||  connection_state_ == ClosedByErr
        ||  connection_state_ == Error )
    {
        return;
    }
    LDEBUG2 << "onError " << message;
    set_error(std::string("AMQP error:" ) + message );
    LDEBUG2<< "AMQP error " << message;
    connection_state_ = ShuttingDownByErr;
    //we shouldn't call do_disconnect directly from callback, because amqpcpp library isn't thread safe
    io_service_.post([this](){ do_disconnect(true); } ); //graceful disconnect because high-level error
}

void asio_handler::onClosed(AMQP::Connection *connection)
{
    LDEBUG2 << "AMQP closed connection";
    connection_ = nullptr;
    connection_state_ = ShuttingDown;
    //we shouldn't call do_disconnect directly from callback, because amqpcpp library isn't thread safe
    io_service_.post([this](){ do_disconnect(true); } );
}

uint16_t asio_handler::onNegotiate(AMQP::Connection *connection, uint16_t interval)
{
    LDEBUG2 << "onNegotiate interval: " << interval;
    if (heartbeat_ == default_heartbeat) heartbeat_ = interval;
    return heartbeat_ ;
}

void asio_handler::onHeartbeat(AMQP::Connection *connection)
{
    LDEBUG2 << "onHeartbeat ";
    connection->heartbeat();
    if (connection_state_ == Communicating && heartbeat_)
    {
        do_timer(get_comm_timeout());
    }
}




} //namespace amqp
#endif // HAVE_AMQP_CPP
