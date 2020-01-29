#ifdef HAVE_AMQP_CPP

#include <serverlib/exception.h>
#include <serverlib/daemon_impl.h>

#include "amqp_channel.h"

#define NICKNAME "MIXA"
#include <serverlib/slogger.h>

namespace amqp {

class StreamBuffer
{
public:
    StreamBuffer(size_t size) :
        data_(size, 0), use_(0)
    { }

    void readComplete(const size_t size) {
        ASSERT(!(data_.size() < (use_ + size)));
        use_ += size;

    }

    std::pair<char*, size_t> getWriteBuffer() {
        if (data_.size() == use_) {
            data_.resize(2 * data_.size());
            LogTrace(TRACE1) << "Resize stream buffer up to " << data_.size() << " bytes";
        }

        return std::pair<char*, size_t>(data_.data() + use_, data_.size() - use_);
    }

    std::pair<const char*, size_t> parseData() const {
        return std::pair<const char*, size_t>(data_.data(), use_);
    }

    void shl(const size_t count) {
        ASSERT(!(use_ < count));

        const size_t diff = use_ - count;
        if (0 == diff) {
            use_ = 0;
            return;
        }

        std::memmove(data_.data(), data_.data() + count, diff);
        use_ = use_ - count;
    }

private:
    std::vector<char> data_;
    size_t use_;
};

ConnectionHandler::ConnectionHandler(Channel& holder, boost::asio::io_service& ios,
                                     const std::string& h, const uint16_t p)
    : ioService_(ios), context_(boost::asio::ssl::context::tlsv11_client), socket_(), heartbeat_(0), timer_(ios),
      lastRead_(), holder_(holder), host_(h), port_(p), streamBuffer_(new StreamBuffer(ASIO_INPUT_BUFFER_SIZE)),
      connection_(nullptr), outputBuffer_()
{
    context_.set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 );

    socket_.reset(new SocketType(ioService_, context_));
    connect();
}

ConnectionHandler::~ConnectionHandler()
{ }

const std::string& ConnectionHandler::host() const
{
    return host_;
}

uint16_t ConnectionHandler::port() const
{
    return port_;
}

boost::asio::io_service& ConnectionHandler::ioService()
{
    return ioService_;
}

void ConnectionHandler::connect()
{
    LogTrace(TRACE5) << __FUNCTION__;

    outputBuffer_.emplace_back();

    boost::system::error_code re; // resolve error
    const boost::posix_time::seconds timeout(15);
    boost::asio::ip::tcp::resolver::query query(host_, std::to_string(port_));
    boost::asio::ip::tcp::resolver::iterator iter = boost::asio::ip::tcp::resolver(ioService_).resolve(query, re);
    if (re) {
        LogError(STDLOG) << "Resolve error " << re << ": " << re.message() << " (" << host_ << ')';

        std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(ioService_));
        timer->expires_from_now(timeout);
        timer->async_wait([this, timer](const boost::system::error_code& ec){ holder_.reset(); });

        return;
    }

    std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(ioService_));
    timer->expires_from_now(timeout);
    timer->async_wait([this, timer](const boost::system::error_code& ec) {
        if (!ec) {
            boost::system::error_code e;

            LogError(STDLOG) << "Connection timed out: " << host_ << ':' << port_;

            socket_->lowest_layer().cancel(e);
        }
    });

    boost::asio::async_connect(socket_->lowest_layer(), iter,
            [this, timer, timeout](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) {
                if (!ec) {
                    socket_->async_handshake(boost::asio::ssl::stream_base::client,
                            [this, timer, timeout](boost::system::error_code ec) {
                                if (!ec) {
                                    read();
                                    outputBuffer_.pop_front();
                                    if(!outputBuffer_.empty()) {
                                        write();
                                    }

                                    timer->cancel(ec);
                                } else {
                                    LogError(STDLOG) << "SSL handshake error:" << ec << ": " << ec.message();

                                    timer->expires_from_now(timeout);
                                    timer->async_wait([this, timer](const boost::system::error_code& ){ holder_.reset(); });
                                }
                            });
                } else {
                    LogError(STDLOG) << "Connection error:" << ec << ": " << ec.message();

                    timer->expires_from_now(timeout);
                    timer->async_wait([this, timer](const boost::system::error_code& ){ holder_.reset(); });
                }
            });
}

uint16_t ConnectionHandler::onNegotiate(AMQP::Connection* const connection, uint16_t interval)
{
    LogTrace(TRACE1) << "Desire heartbeat: " << interval;

    heartbeat_ = interval;
    if (heartbeat_) {
        resetHeartbeat();
    }

    return heartbeat_;
}

void ConnectionHandler::onData(AMQP::Connection* const connection, const char* data, const size_t size)
{
    connection_ = connection;
    outputBuffer_.emplace_back(data, data + size);
    if (1 == outputBuffer_.size()) {
        write();
    }
}

void ConnectionHandler::onHeartbeat(AMQP::Connection* connection)
{
    lastRead_ = std::chrono::steady_clock::now();
    connection->heartbeat();
}

void ConnectionHandler::read()
{
    const auto wd = streamBuffer_->getWriteBuffer();
    socket_->async_read_some(boost::asio::buffer(wd.first, wd.second),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    lastRead_ = std::chrono::steady_clock::now();
                    streamBuffer_->readComplete(length);
                    parse();
                    read();
                } else {
                    LogError(STDLOG) << "Error reading: " << ec << ": " << ec.message();

                    timer_.cancel(ec);
                    socket_->lowest_layer().cancel(ec);
                    socket_->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    socket_->lowest_layer().close(ec);
                    streamBuffer_->shl(streamBuffer_->parseData().second);
                    outputBuffer_.clear();
                    outputBuffer_.emplace_back();
                    ioService_.post([this](){ holder_.reset(); });
                }
            });
}

void ConnectionHandler::write()
{
    boost::asio::async_write(*socket_, boost::asio::buffer(outputBuffer_.front()),
            [this](boost::system::error_code ec, std::size_t length ) {
                if(!ec) {
                    outputBuffer_.pop_front();
                    if(!outputBuffer_.empty()) {
                        write();
                    }
                } else {
                    LogError(STDLOG) << "Error writing: " << ec << ": " << ec.message();

                    socket_->lowest_layer().cancel(ec);
                }
            });
}

void ConnectionHandler::parse()
{
    ASSERT(nullptr != connection_);

    const auto pd = streamBuffer_->parseData();
    const size_t count = connection_->parse(pd.first, pd.second);
    streamBuffer_->shl(count);
}

void ConnectionHandler::resetHeartbeat()
{
    timer_.expires_from_now(boost::posix_time::seconds(heartbeat_));
    timer_.async_wait([this](const boost::system::error_code& e) {
                          if (boost::asio::error::operation_aborted == e) {
                              LogTrace(TRACE1) << "Heartbeat time canceled.";
                              return;
                          }

                          boost::system::error_code ec;
                          const auto diff = std::chrono::steady_clock::now() - lastRead_;
                          if (std::chrono::seconds(2 * heartbeat_) < diff) {
                              LogError(STDLOG) << "Heartbeat: " << heartbeat_ << " expired: "
                                               << std::chrono::duration_cast<std::chrono::seconds>(diff).count();
                              socket_->lowest_layer().cancel(ec);
                              return;
                          }

                          resetHeartbeat();
                      });
}

void ConnectionHandler::onReady(AMQP::Connection* const connection) { }

void ConnectionHandler::onError(AMQP::Connection* const connection, const char* const message)
{
    boost::system::error_code ec;

    LogError(STDLOG) << "AMQP error: " << message << ' ' << this;

    socket_->lowest_layer().cancel(ec);
}

void ConnectionHandler::onClosed(AMQP::Connection* const connection)
{
    boost::system::error_code ec;

    LogTrace(TRACE1) << "AMQP closed connection";

    socket_->lowest_layer().cancel(ec);
}

Channel::Channel(boost::asio::io_service& io_s, RstCallback& rst, const AMQP::Address& addr)
    : ch_(std::make_unique< ConnectionHandler >(*this, io_s, addr.hostname(), addr.port())),
      conn_(std::make_unique< AMQP::Connection >(ch_.get(), addr.login(), addr.vhost())),
      channel_(std::make_unique< AMQP::Channel >(conn_.get())), rst_(rst)
{
    rst_(*channel_);
}


Channel::Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host, const uint16_t port)
    : ch_(new ConnectionHandler(*this, io_s, host, port)), conn_(new AMQP::Connection(ch_.get())),
      channel_(new AMQP::Channel(conn_.get())), rst_(rst)
{
    rst_(*channel_);
}

Channel::Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host,
                 const uint16_t port, const AMQP::Login& l)
    : ch_(new ConnectionHandler(*this, io_s, host, port)), conn_(new AMQP::Connection(ch_.get(), l)),
      channel_(new AMQP::Channel(conn_.get())), rst_(rst)
{
    rst_(*channel_);
}

Channel::Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host,
                 const uint16_t port, const std::string& vhost)
    : ch_(new ConnectionHandler(*this, io_s, host, port)), conn_(new AMQP::Connection(ch_.get(), vhost)),
      channel_(new AMQP::Channel(conn_.get())), rst_(rst)
{
    rst_(*channel_);
}

Channel::Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host, const uint16_t port,
                 const AMQP::Login& l, const std::string& vhost)
    : ch_(new ConnectionHandler(*this, io_s, host, port)), conn_(new AMQP::Connection(ch_.get(), l, vhost)),
      channel_(new AMQP::Channel(conn_.get())), rst_(rst)
{
    rst_(*channel_);
}

void Channel::startTransaction()
{
    channel_->startTransaction();
}

AMQP::Deferred& Channel::commitTransaction()
{
    return channel_->commitTransaction();
}

AMQP::Deferred& Channel::rollbackTransaction()
{
    return channel_->rollbackTransaction();
}

bool Channel::publish(const std::string& exchange, const std::string& routingKey, const AMQP::Envelope& env)
{
    return channel_->publish(exchange, routingKey, env);
}

AMQP::Deferred& Channel::publishTransaction(const std::string& exchange,
                                            const std::string& routingKey,
                                            const AMQP::Envelope& env)
{
    channel_->startTransaction();
    this->publish(exchange, routingKey, env);
    return channel_->commitTransaction();
}

AMQP::Deferred& Channel::publishTransaction(const std::string& exchange,
                                            const std::string& routingKey,
                                            const std::vector<AMQP::Envelope>& envelops)
{
    channel_->startTransaction();

    for (const auto& e : envelops) {
      this->publish(exchange, routingKey, e);
    }

    return channel_->commitTransaction();
}

void Channel::reset()
{
    LogError(STDLOG) << "Reset amqp channel";

    std::unique_ptr<ConnectionHandler> connHandler(new ConnectionHandler(*this, ch_->ioService(), ch_->host(), ch_->port()));
    std::unique_ptr<AMQP::Connection> connection(new AMQP::Connection(connHandler.get(), conn_->login(), conn_->vhost()));

    channel_.reset(new AMQP::Channel(connection.get()));
    conn_ = std::move(connection);
    ch_ = std::move(connHandler);

    rst_(*channel_);
}

} // namespace amqp

#endif // HAVE_AMQP_CPP
