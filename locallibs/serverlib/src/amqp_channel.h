#ifdef HAVE_AMQP_CPP

#pragma once

#include "amqp_connection_handler.h"

namespace AMQP { class Connection; }
namespace AMQP { class Channel; }
namespace boost { namespace asio { class io_context; } }

namespace amqp {

class Channel
    : public std::enable_shared_from_this<Channel>
{
public:
    typedef std::function<void (AMQP::Channel& ch)> RstCallback;

public:
    template<typename... Ts>
    static std::shared_ptr<Channel> make_shared(Ts&&... args) {
        return std::make_shared<Channel>(std::forward<Ts>(args)...);
    }

    Channel(boost::asio::io_service& io_s, RstCallback& rst, const AMQP::Address& addr);
    Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host, const uint16_t port);
    Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host, const uint16_t port, const AMQP::Login& login);
    Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host, const uint16_t port, const std::string& vhost);
    Channel(boost::asio::io_service& io_s, RstCallback& rst, const std::string& host,
            const uint16_t port, const AMQP::Login& login, const std::string& vhost);

    Channel(const Channel& other) = delete;
    Channel& operator=(const Channel& other) = delete;

    Channel(Channel&& other) = delete;
    Channel& operator=(Channel&& other) = delete;

    void startTransaction();
    AMQP::Deferred& commitTransaction();
    AMQP::Deferred& rollbackTransaction();

    bool publish(const std::string& exchange, const std::string& routingKey, const AMQP::Envelope& env);

    AMQP::Deferred& publishTransaction(const std::string& exchange,
                                       const std::string& routingKey,
                                       const AMQP::Envelope& env);
    AMQP::Deferred& publishTransaction(const std::string& exchange,
                                       const std::string& routingKey,
                                       const std::vector<AMQP::Envelope>& envelops);
private:
    friend class ConnectionHandler; // call reset()
    void reset();

private:
    std::unique_ptr<ConnectionHandler> ch_;
    std::unique_ptr<AMQP::Connection> conn_;
    std::unique_ptr<AMQP::Channel> channel_;
    RstCallback rst_;
};

} // namespace amqp

#endif // HAVE_AMQP_CPP
