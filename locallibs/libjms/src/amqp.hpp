#ifdef HAVE_AMQP_CPP
#ifndef __AMQP_H__
#define __AMQP_H__

#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include "detail.hpp"
#include "text_message.hpp"
#include "asiohandler.hpp"
#include <list>
#include <queue>
#include <amqpcpp.h>
#include <signal.h>
#include <future>


namespace amqp
{
class amqp_queue;
class asio_handler;

class Address
{
private:
    AMQP::Login _login;
    std::string _hostname;
    uint16_t _port;
    std::string _vhost;
    bool _is_tls;
    std::map<std::string, std::string> _parameters;
public:
    static const uint16_t amqp_port = 5672;
    static const uint16_t amqps_port = 5671;

    Address(const std::string& address);

    Address(std::string host, uint16_t port, bool tls, AMQP::Login login, std::string vhost) : _login(std::move(login)),
                                                                               _hostname(std::move(host)),
                                                                               _port(port),
                                                                               _vhost(std::move(vhost)),
                                                                               _is_tls(tls)
    {}

    virtual ~Address() {}

    const AMQP::Login &login() const
    {
        return _login;
    }

    const std::string &hostname() const
    {
        return _hostname;
    }

    uint16_t port() const
    {
        return _port;
    }

    bool is_tls() const
    {
        return _is_tls;
    }

    const std::string &vhost() const
    {
        return _vhost;
    }

    const std::string & get_str_param(const std::string& name) const;
    int get_int_param(const std::string& name, int nvl = 0) const;

    operator std::string () const;
};



class io_service_thread_runner;


class connection : public detail::connection
{
public:
    connection(const std::string&);
    virtual void check() override;
    virtual void commit() override;
    virtual void rollback() override;
    virtual void break_wait() override;
    virtual std::string listen(const std::vector<std::string>&) override;
    virtual std::shared_ptr<detail::text_queue> create_text_queue(const std::string& , bool xact=true) override;
    virtual ~connection();
    net::io_service& get_ioservice();
    AMQP::Connection* get_connection_impl() const
    {
        return conn.get();
    }
    void on_handler_disconnect__(bool is_graceful);
private:

    const std::shared_ptr<io_service_thread_runner> runner_;
    Address addr;
    size_t qos;
    std::mutex channels_mtx;
    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;
    void invoke_method(void (amqp_queue::*)());
    std::shared_ptr<asio_handler> handler;
    std::shared_ptr<AMQP::Connection> conn;
    std::list<std::shared_ptr<amqp_queue>> channels_;
};

class amqp_queue : public detail::text_queue
{
friend class connection;
public:
    jms::text_message dequeue(const jms::recepient&, const std::string& = "") override;
    bool dequeue(jms::text_message& msg, unsigned wait_delay = 0,
                 const jms::recepient& = jms::recepient(), const std::string& = "") override;
    void enqueue(const jms::text_message&, const jms::recepients&) override;
    virtual ~amqp_queue();
private:
    amqp_queue(connection&, const std::string&, size_t );
    bool fits_name(const std::string& name) const;

    bool dequeue(jms::text_message& msg, unsigned wait_delay, bool,
                 const jms::recepient&, const std::string&);
    typedef std::shared_ptr<std::promise<bool>> deferred_result_holder_t;
    typedef std::pair<jms::text_message, uint64_t> msg_and_tag_t;
    typedef std::queue<msg_and_tag_t, std::list<msg_and_tag_t>> messages_queue_t;

    template <typename R, class ...fArgs, class ...Args>
    std::future<R> post_task(R (amqp_queue::*f)(fArgs...), Args&&...);

    template <typename R, class ...fArgs, class ...Args>
    void post_and_wait_task(
            const std::string& task_name,
            const std::chrono::seconds& timeout,
            R (amqp_queue::*f)(fArgs...), Args&&... args);

    template <typename R, class ...fArgs, class ...Args>
    void post_and_wait_task_with_deferred_result(
            const std::string& task_name,
            const std::chrono::seconds& timeout,
            R (amqp_queue::*f)(fArgs...), Args&&... args);


    void wait_for_deferred_result__(deferred_result_holder_t promise, AMQP::Deferred& operation);
    bool enqueue__(jms::recepients agents, const jms::text_message& msg);
    void dequeue__();
    bool start_consume__();
    void init_channel__(deferred_result_holder_t& promise, bool xact);
    void commit__(deferred_result_holder_t& promise, bool need_ack, uint64_t tag);
    void rollback__(deferred_result_holder_t& promise);
    void recover__(deferred_result_holder_t& promise);
    bool close__();




    void check_error();
    void commit();
    void rollback();
    void break_wait();
    void init_channel(bool xact);

    std::atomic_flag no_errors;
    std::list<std::string> errors_strings;
    bool dequeue_task_working;


    std::mutex queue_mtx;
    std::condition_variable queue_cv;
    connection& conn;
    bool has_published;
    bool has_unacked;
    bool need_recover;
    uint64_t unacked_message_tag;

    std::string name__;
    std::string routing_key__;
    std::string exchange__;
    size_t qos__;
    std::unique_ptr<AMQP::Channel> channel__;

    bool is_consuming_;
    messages_queue_t consumed_messages;

    bool need_dequeue_notify__;
    messages_queue_t consumed_messages_internal__;

};

}// namespace AMQ

#endif // __AMQP_H__
#endif // HAVE_AMQP_CPP
