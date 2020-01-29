#ifdef HAVE_AMQP_CPP

#pragma once

#include <deque>
#include <vector>
#include <memory>
#include <chrono>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>

#include <amqpcpp.h>

namespace amqp {

class Channel;
class StreamBuffer;

class ConnectionHandler
    : public AMQP::ConnectionHandler
{
public:
    static constexpr size_t ASIO_INPUT_BUFFER_SIZE = 4 * 1024; //4kb

public:
    ConnectionHandler(Channel& holder, boost::asio::io_service& ios, const std::string& h, const uint16_t p);

    virtual ~ConnectionHandler();

    ConnectionHandler(const ConnectionHandler&) = delete;
    ConnectionHandler& operator=(const ConnectionHandler&) = delete;

    const std::string& host() const;

    uint16_t port() const;

    boost::asio::io_service& ioService();

private:
    typedef std::deque<std::vector<char> > OutputBuffers;

private:
    virtual uint16_t onNegotiate(AMQP::Connection* const connection, uint16_t interval) override;

    virtual void onData(AMQP::Connection* const connection, const char* const data, const size_t size) override;

    virtual void onHeartbeat(AMQP::Connection* connection) override;

    virtual void onReady(AMQP::Connection* const connection) override;

    virtual void onError(AMQP::Connection* const connection, const char* const message) override;

    virtual void onClosed(AMQP::Connection* const connection) override;

    void connect();

    void read();

    void write();

    void parse();

    void resetHeartbeat();

private:
    using SocketType = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    boost::asio::io_service& ioService_;
    boost::asio::ssl::context context_;
    std::shared_ptr<SocketType> socket_;

    uint16_t heartbeat_;
    boost::asio::deadline_timer timer_;
    std::chrono::steady_clock::time_point lastRead_;

    Channel& holder_;
    const std::string host_;
    const uint16_t port_;
    std::unique_ptr<StreamBuffer> streamBuffer_;
    AMQP::Connection* connection_;
    OutputBuffers outputBuffer_;
};

} // namespace amqp

#endif // HAVE_AMQP_CPP
