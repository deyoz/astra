#ifdef HAVE_AMQP_CPP
#ifndef ASIO_HANDLER_HPP
#define ASIO_HANDLER_HPP

#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "asio_ssl.hpp"
#include <amqpcpp.h>
#include <boost/asio.hpp>
#include "stream_holder.hpp"

namespace amqp
{
class stream_buffer;

constexpr uint16_t default_heartbeat = std::numeric_limits<uint16_t>::max();

class asio_handler: public AMQP::ConnectionHandler
{
public:
    static constexpr size_t ASIO_INPUT_BUFFER_SIZE = 1024 << 3 ; //8KiB
    asio_handler(boost::asio::io_service& ioService);
    void connect(const std::string& host, uint16_t port, bool tls, uint16_t heartbeat);
    void close();
    virtual ~asio_handler() noexcept;
    std::string get_error() const
    {
        return err_desc;
    }

    asio_handler(const asio_handler&) = delete;
    asio_handler& operator=(const asio_handler&) = delete;

    bool wait_for_amqp_readiness();
    bool wait_for_disconnect();
    void break_wait();
    void check_error();
    void set_on_disconnect(const std::function<void (bool)>& cb);

private:

    enum connection_state_t {Disconnected, Handshake, Connected, Communicating, ClosingComm, ShuttingDown, Closed, ShuttingDownByErr, ClosedByErr, Error } ;
    enum ext_connection_state_t {AmqpDisconnected, AmqpNotReady, AmqpReady, AmqpError } ;

    typedef std::deque<std::vector<char>> OutputBuffers;

    virtual void onData(AMQP::Connection *connection, const char *data, size_t size) override;
    virtual void onReady(AMQP::Connection *connection) override;
    virtual void onError(AMQP::Connection *connection, const char *message) override;
    virtual void onClosed(AMQP::Connection *connection) override;
    // receives heartbeat interval from server, returns our heartbeat
    virtual uint16_t onNegotiate(AMQP::Connection *connection, uint16_t interval) override;
    virtual void onHeartbeat(AMQP::Connection *connection) override;

    void do_connect(const std::string& host, uint16_t port, bool tls);
    void do_handshake();
    void do_read();
    void do_write();
    void do_disconnect(bool is_graceful = false);
    void do_reset();
    void do_timer(size_t seconds);
    void do_disconnect_notify();
    void parse_data();
    void set_error(const std::string& desc, const boost::system::error_code& ec = boost::system::error_code());
    void handle_timer(const boost::system::error_code& ec);
    bool handle_error(const boost::system::error_code& e);
    size_t get_comm_timeout() const
    {
        return static_cast<size_t>(heartbeat_) * 2;
    }
private:

    std::atomic_flag no_errors;

    std::mutex mtx;
    std::condition_variable cv;
    ext_connection_state_t ext_state_;

    boost::asio::io_service& io_service_;
    boost::asio::deadline_timer timer_;
    sirena_net::stream_holder stream_;
    sirena_net::context_ptr ctx_;
    std::shared_ptr<stream_buffer> stream_buffer_;
    AMQP::Connection* connection_;
    OutputBuffers output_buffer_;
    bool writing_;
    bool reading_;
    uint16_t heartbeat_;
    connection_state_t connection_state_;
    std::string err_desc;
    std::chrono::high_resolution_clock::time_point last_data_tm;
    std::function<void (bool)> on_disconnect;
};
} // namespace amqp
#endif// ASIO_HANDLER_HPP

#endif // HAVE_AMQP_CPP
