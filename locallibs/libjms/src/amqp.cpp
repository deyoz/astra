#ifdef HAVE_AMQP_CPP
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <amqpcpp.h>
#include "jms_log.hpp"
#include "asiohandler.hpp"
#include "amqp.hpp"
#include "errors.hpp"
#include <boost/algorithm/string.hpp>

namespace amqp
{
namespace
{
const std::chrono::seconds init_channel_timeout(20);
//const std::chrono::seconds deferred_wait_timeout(20);
const std::chrono::seconds enqueue_task_timeout(20);
const std::chrono::seconds commit_timeout(20);
const std::chrono::seconds rollback_timeout(20);
const std::chrono::seconds start_consume_timeout(20);
const std::chrono::seconds close_timeout(20);
}


// sets some variable to true during destruction,
// see io_service_thread_runner::get_ioservice()
class destroy_flag
{
    bool& flag;
    public:
        destroy_flag(bool& fl):flag(fl)  {}
        ~destroy_flag() {  flag = true; }
};

// it will be a sigletone while at least one connection object exists.
// Every connection stores shared_ptr to this singletone.
// The first one creates singletone, which starts the thread.
// After destruction of all connection objects, all shared_ptr will be destructed also,
// and singletone will be deleted, its destructor will stop work and joins the thread
// in case of concurrent deleting / creation singleton will be recreated

class io_service_thread_runner
{
public:
    static std::shared_ptr<io_service_thread_runner> get_runner()
    {
        // maybe redundant check but who knows?
        if (is_termination)
        {
            throw jms::mq_error(-1, "Connection create during program termination detected");
        }
        std::lock_guard<std::mutex> lck(create_mtx);
        std::shared_ptr<io_service_thread_runner> ret = current_runner.lock();
        if (!ret)
        {
            ret.reset(new io_service_thread_runner);
            current_runner = ret;
        }
        return ret;
    }
    net::io_service& get_ioservice()
    {
        // this stuff will be destructed during cleanup
        // before static members of this class
        // and should set `is_termination' flag
        static destroy_flag termination_hook(is_termination);
        return io_serv;
    }

    ~io_service_thread_runner()
    {
        work.reset();
        ioserv_thread.join();
        LDEBUG << "ioserv_thread completed";
        get_ioservice().reset();
    }
    void thread_main()
    {
        for(;;)
        {
            LDEBUG2 << "Running thread";
            try
            {
                get_ioservice().run();
                break;
            }
            catch(const std::exception& e)
            {
                LDEBUG2 << "ioserv_thread catched exception : " << e.what();
            }
            catch(...)
            {
                LDEBUG2 << "ioserv_thread catched unknown exception";
            }
        }
        LDEBUG2 << "ioserv_thread ending";
    }
private:
    io_service_thread_runner()
        :work(new net::io_service::work(get_ioservice()))
    {
        LDEBUG << "starting ioserv_thread";
        ioserv_thread = std::thread(std::bind(&io_service_thread_runner::thread_main, this));
    }
    net::io_service io_serv;
    std::thread ioserv_thread;
    std::unique_ptr<net::io_service::work> work;
    static std::weak_ptr<io_service_thread_runner> current_runner;
    static bool is_termination;
    static std::mutex create_mtx;
};

std::weak_ptr<io_service_thread_runner> io_service_thread_runner::current_runner;
std::mutex io_service_thread_runner::create_mtx;
bool io_service_thread_runner::is_termination = false;


Address::Address(const std::string& address) : _vhost("/")
{
    size_t pos = 0;
    if (!address.compare(0, (pos = 7), "amqp://"))
    {
        _port  = amqp_port;
        _is_tls = false;

    }
    else if (!address.compare(0, (pos = 8), "amqps://"))
    {
        _port  = amqps_port;
        _is_tls = true;
    }
    else
        throw jms::mq_error(-1, "invalid AMQP address");

    // do we have a '@' to split user-data and hostname?
    size_t at = address.find_first_of('@', pos);

    size_t colon = address.find_first_of(':', pos);


    if (at != std::string::npos)
    {
        // assign the login
        _login = AMQP::Login(
                address.substr(pos, ((colon < at ) ? colon : at) - pos ),
                (colon < at ) ? address.substr(colon + 1, at - colon - 1) : ""
                );
        // set pos to the start of the hostname
        pos = at + 1;
        if (colon < at )
            colon = address.find_first_of(':', pos);
    }

    size_t qmark = address.find_first_of('?', pos);
    // find out where the vhost is set (starts with a slash)
    size_t slash = address.find_first_of('/', pos);

    std::string options;

    if (qmark != std::string::npos )
    {
        if ( (colon != std::string::npos && colon > qmark ) 
          || (slash != std::string::npos && slash > qmark ) ) 
        {
            throw jms::mq_error(-1, "invalid AMQP address: wrong options separator (?) position");
        }
        options.assign(address, qmark + 1, std::string::npos);
    }
    else
    {
        qmark = address.size();
    }

    // was a vhost set?
    if (slash != std::string::npos && slash + 1 < qmark)
        _vhost.assign(address, slash + 1, qmark - (slash + 1));

    if (slash == std::string::npos) slash = qmark;

    // was a portnumber specified (colon must appear before the slash of the vhost)
    if (colon != std::string::npos)
    {
        // port is between colon and slash (or eol when slash == npos)
        char* endptr;
        _port = strtoul(address.c_str() + colon + 1, &endptr, 10);
        if(not endptr or *endptr != '/' or _port > 0xffffU)
            throw jms::mq_error(-1, "invalid AMQP address: wrong port number");
    }
    else
    {
        colon = slash;
    }
    _hostname.assign(address, pos, colon - pos);
    if (_hostname.empty())
    {
        _hostname = "localhost";
    }

    if (!options.empty())
    {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, options, boost::algorithm::is_any_of("&"));
        static const char * valid_params[] = { "qos", "dummy", "heartbeat", };

        for (const auto& t : tokens)
        {
            std::vector<std::string> pv_pair;
            boost::algorithm::split(pv_pair, t, boost::algorithm::is_any_of("="));
            if (pv_pair.size() != 2)
            {
                throw jms::mq_error(-1, "invalid AMQP params: inconsistent pair-value token" );
            }
            auto ins = _parameters.emplace(boost::to_lower_copy(pv_pair[0]), std::move(pv_pair[1]));
            if (! ins.second )
            {
                throw jms::mq_error(-1, "invalid AMQP params: duplicated parameter: " + pv_pair[0] );
            }
            auto found = std::find(std::begin(valid_params), std::end(valid_params), ins.first->first);
            if (found == std::end(valid_params))
            {
                throw jms::mq_error(-1, "invalid AMQP params: unknown parameter: " + pv_pair[0] );
            }
        }
    }
}

const std::string & Address::get_str_param(const std::string& name) const
{
    static const std::string empty;
    auto found = _parameters.find(name);
    if (found != _parameters.end())
    {
        return found->second;
    }
    return empty;
}

int Address::get_int_param(const std::string& name, int nvl) const
{
    try
    {
        const auto & val = get_str_param(name);
        if (val.empty()) return nvl;
        int ret = std::stoi(val);
        return ret;
    }
    catch(...)
    {
        throw jms::mq_error(-1, "invalid AMQP params: invalid integer parameter: " + name);
    }
}

Address::operator std::string () const
{
    // result object
    std::string str(_is_tls ? "amqps://" : "amqp://");
    // append login
    str.append(_login.user()).append(":").append(_login.password()).append("@").append(_hostname);

    // do we need a special portnumber?
    if (  (_is_tls && _port != amqps_port)
       || (!_is_tls && _port != amqp_port)
       )
    {
        str.append(":").append(std::to_string(_port));
    }
    // append default vhost
    str.append("/");
    // do we have a special vhost?
    if (_vhost != "/") str.append(_vhost);
    return str;
}

connection::connection(const std::string& brokerURI)
    : runner_(io_service_thread_runner::get_runner())
    , addr(brokerURI)
    , qos(1)
    , handler(std::make_shared<asio_handler>(get_ioservice()))
{
    int q = addr.get_int_param("qos", 1);
    if (q < 0)
    {
        throw jms::mq_error(-1, "Negative QoS value not allowed");
    }
    else
    {
        qos = static_cast<size_t>(q);
    }

    int hb = addr.get_int_param("heartbeat", default_heartbeat);
    uint16_t heartbeat = default_heartbeat;
    if (hb < 0)
    {
        throw jms::mq_error(-1, "Negative Heartbeat value not allowed");
    }
    else
    {
        heartbeat = static_cast<uint16_t>(hb);
    }


    handler->set_on_disconnect(std::bind(&connection::on_handler_disconnect__, this, std::placeholders::_1));
    LDEBUG << "connect";
    handler->connect(addr.hostname(), addr.port(), addr.is_tls(), heartbeat);
    LDEBUG << "connection create " << this;
    connection::get_ioservice().post(
            [this]()
            {
                conn = std::make_shared<AMQP::Connection>(handler.get(), addr.login(), addr.vhost());
            }
        );
    handler->wait_for_amqp_readiness();
    handler->check_error();
}


connection::~connection()
{
   LDEBUG << "~connection()";
   try {
       {
           std::lock_guard<std::mutex> lck(channels_mtx);
           channels_.clear();
       }
       handler->close();
       LDEBUG << "~connection() disconnected";
       conn.reset();
       LDEBUG << "~connection() resetting handler";
       handler.reset();
   } catch (...) {
       LERR << "exception catched in ~connection() ";
   }
   LDEBUG << "~connection() end " << this;
}
net::io_service& connection::get_ioservice()
{
    return runner_->get_ioservice();
}

void connection::check()
{
    handler->check_error();
}

void connection::invoke_method(void (amqp_queue::* method)())
{
    std::lock_guard<std::mutex> lck(channels_mtx);
    for (auto& i : channels_)
    {
        if (i) (i.get()->*method)();
    }
    LDEBUG << "removing unused channels";
    channels_.remove_if([](const std::shared_ptr<amqp_queue>& p){ return p.use_count() < 2; });
    LDEBUG << "channels size:" << channels_.size();
}

void connection::commit()
{
    invoke_method(&amqp_queue::commit);
}

void connection::rollback()
{
    invoke_method(&amqp_queue::rollback);
}

void connection::break_wait()
{
    std::unique_lock<std::mutex> lck(channels_mtx, std::try_to_lock);
    // if mutex locked by another thread we don't need to break waiting in dequeue
    //
    if (lck.owns_lock())
    {
        for (auto& i : channels_)
        {
            if (i) i->break_wait();
        }
    }
}

void connection::on_handler_disconnect__(bool is_graceful)
{
    LDEBUG2 << "Handler Disconnected";
    break_wait();
}


std::string connection::listen(const std::vector<std::string>& queue_names)
{
   // do smth   
   return std::string();
}

std::shared_ptr<detail::text_queue> connection::create_text_queue(const std::string& name , bool xact)
{
   std::lock_guard<std::mutex> lck(channels_mtx);
   auto i = std::find_if(channels_.begin(), channels_.end(), [&name](auto&p){ return p->fits_name(name); });
   if(i != channels_.end())
       return *i;
   // auto p = std::make_shared<amqp_queue>(*this, name, qos);
   std::shared_ptr<amqp_queue> p(new amqp_queue(*this, name, qos));
   p->init_channel(xact);
   channels_.push_back(p);
   return p;
}

static auto get_channel_pegs(const std::string& name)
{
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, name, boost::algorithm::is_any_of("/"));
    if (tokens.size() == 1)
    {
        return std::make_tuple(tokens[0], std::string(), std::string());
    }
    else if (tokens.size() == 2)
    {
        return std::make_tuple(std::string(), tokens[0], tokens[1]);
    }
    else if (tokens.size() == 3)
    {
        return std::make_tuple(tokens[2], tokens[0], tokens[1]);
    }
    else
    {
        LERR << "invalid AMQP queue params";
        throw jms::mq_error(-1, "invalid AMQP queue params");
    }
}

bool amqp_queue::fits_name(const std::string& name) const
{
    return std::tie(name__, exchange__, routing_key__) == get_channel_pegs(name);
}

amqp_queue::amqp_queue(connection& c,  const std::string& name, size_t qos)
    : dequeue_task_working(false), conn(c), has_published(false), has_unacked(false), need_recover(false)
    , unacked_message_tag(0), qos__(qos), is_consuming_(false), need_dequeue_notify__(false)
{
    no_errors.clear();
    no_errors.test_and_set();

    std::tie(name__, exchange__, routing_key__) = get_channel_pegs(name);
    LDEBUG << "amqp_queue created " << this << '/' << &conn;
}

void amqp_queue::init_channel(bool xact)
{
    post_and_wait_task_with_deferred_result("Init_Channel", init_channel_timeout, &amqp_queue::init_channel__, xact);
    LDEBUG << "Init channel complete";
}

void amqp_queue::init_channel__(std::shared_ptr<std::promise<bool>>& promise, bool xact)
{
    LDEBUG2 << "init_channel";
    //channel__.reset(new AMQP::Channel(conn.get_connection_impl()));
    channel__ = std::make_unique<AMQP::Channel>(conn.get_connection_impl());
    channel__->onError(
            [this](const char* msg)
            {
                LERR << "Channel error:" << msg;
                errors_strings.emplace_back(msg);
                no_errors.clear();
                break_wait();
            });
    if(qos__)
    {
        LDEBUG2 << "setQos";
        channel__->setQos(qos__);
    }
    if(xact)
    {
        LDEBUG2 << "start transaction";
        wait_for_deferred_result__(promise, channel__->startTransaction());
    }
    else
        promise->set_value(true);
}


amqp_queue::~amqp_queue()
{
    LDEBUG << "~amqp_queue()" << this;
    try {
        post_and_wait_task("Channel close", close_timeout, &amqp_queue::close__);
    } catch (const jms::mq_error& e) {
        LDEBUG << "catched exception '" << e.what() << "' in 'channel close' task";
    } catch (...) {
        LDEBUG << "catched exception in 'channel close' task";
    }
    LDEBUG << "~amqp_queue() end " << this;
}

bool amqp_queue::close__()
{
    channel__.reset();
    return true;
}

void amqp_queue::check_error()
{
    if (!no_errors.test_and_set())
    {
        throw jms::mq_error(-1, errors_strings.front());
    }
    conn.check();
}

void amqp_queue::commit()
{
    LDEBUG << "Commiting";
    check_error();
    if (has_unacked || has_published) {
        post_and_wait_task_with_deferred_result("Commit", commit_timeout,
                &amqp_queue::commit__, has_unacked, unacked_message_tag);
        has_unacked = false;
        has_published = false;
    }
    LDEBUG << "Commit complete";
}

void amqp_queue::commit__(deferred_result_holder_t& promise, bool need_ack, uint64_t tag)
{
    if(need_ack)
    {
        channel__->ack(tag, AMQP::multiple);
    }
    wait_for_deferred_result__(promise, channel__->commitTransaction());
}


void amqp_queue::rollback()
{
    LDEBUG << "Rollbacking";
    check_error();
    if (has_unacked || has_published) {
        post_and_wait_task_with_deferred_result("Rollback", rollback_timeout, &amqp_queue::rollback__);
        need_recover = true;
        has_unacked = false;
        has_published = false;
    }
    LDEBUG << "Rollback complete";
}

void amqp_queue::rollback__(deferred_result_holder_t& promise)
{
    wait_for_deferred_result__(promise, channel__->rollbackTransaction());
}


void amqp_queue::recover__(deferred_result_holder_t& promise)
{
    wait_for_deferred_result__(promise, channel__->recover(AMQP::requeue));
}

void amqp_queue::break_wait()
{
    std::lock_guard<std::mutex> lck(queue_mtx);
    queue_cv.notify_all();
}

namespace
{
constexpr char rabbitmq_delay_property[] = {"x-delay"};
}

void amqp_queue::enqueue(const jms::text_message& msg, const jms::recepients& agents)
{
   check_error();
   LDEBUG << "enqueue";
   try
   {
      LDEBUG << "Publishing";
      post_and_wait_task("Enqueue", enqueue_task_timeout, &amqp_queue::enqueue__, agents, msg);
      has_published = true;
   }
   catch (const std::exception& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.what());
   }
}

template <typename R, class ...fArgs, class ...Args>
std::future<R> amqp_queue::post_task(R (amqp_queue::*f)(fArgs...), Args&&... args)
{
    std::shared_ptr<std::packaged_task<R()>> task_ptr( new
            std::packaged_task<R()>(
                std::bind(f, this, std::forward<Args>(args)...)
            ));
    conn.get_ioservice().post(//[task_ptr, this]() {(*task_ptr)(); } );
            std::bind(&std::packaged_task<R()>::operator(), task_ptr));

    return std::move(task_ptr->get_future());
}


template <typename R, class ...fArgs, class ...Args>
void amqp_queue::post_and_wait_task(const std::string& task_name, const std::chrono::seconds& timeout,
        R (amqp_queue::*f)(fArgs...), Args&&... args)
{
    auto result = post_task(f, std::forward<Args>(args)...);
    if (result.wait_for(timeout) != std::future_status::ready)
    {
        LERR << (task_name + " task timed out");
        throw jms::mq_error(-1, task_name + " task timed out");
    }
    if (!result.get())
    {
        LERR << (task_name + " task thas errors");
        throw jms::mq_error(-1, task_name + " task has errors");
    }
}

template <typename R, class ...fArgs, class ...Args>
void amqp_queue::post_and_wait_task_with_deferred_result(const std::string& task_name, const std::chrono::seconds& timeout,
        R (amqp_queue::*f)(fArgs...), Args&&... args)
{
    std::shared_ptr<std::promise<bool>> promise = std::make_shared<std::promise<bool>>();
    auto result = promise->get_future();
    conn.get_ioservice().post(
            std::bind(f, this, promise, std::forward<Args>(args)...));

    if (result.wait_for(timeout) != std::future_status::ready)
    {
        LERR << "Task " << task_name << " is timed out";
        throw jms::mq_error(-1,task_name + " task timed out");
    }
    try
    {
        if (!result.get())
        {
            LERR << "Task " << task_name << " has errors";
            throw jms::mq_error(-1, task_name + " task has errors");
        }
    }
    catch(const std::exception& e)
    {
        LERR << "Exception in task " << task_name << " " << e.what();
        throw jms::mq_error(-1, task_name + " task has errors");
            
    }
}

void amqp_queue::wait_for_deferred_result__(deferred_result_holder_t promise, AMQP::Deferred& operation)
{
    operation
        .onSuccess([promise]()
            {
                try
                {
                LDEBUG2 << "Setting value ok ";
                    promise->set_value(true);
                }
                catch(...){}
            })
        .onError([promise](const char* msg)
            {
                try
                {
                LDEBUG2 << "Setting exception " << msg;

                    promise->set_exception(std::make_exception_ptr(jms::mq_error(-1, msg)));
                }
                catch(...){}
            });
}



bool amqp_queue::enqueue__(jms::recepients agents, const jms::text_message& msg)
{
    AMQP::Envelope env(msg.text.data(), msg.text.size());
    env.setPersistent();
    if (msg.properties.delay)
    {
        AMQP::Table headers;
        headers[rabbitmq_delay_property] = msg.properties.delay;
        env.setHeaders(std::move(headers));
    }
    if (msg.properties.expiration)
    {
        env.setExpiration(std::to_string(msg.properties.expiration));
    }
    if (!msg.properties.correlation_id.empty())
    {
        env.setCorrelationID(msg.properties.correlation_id);
    }
    if (!msg.properties.reply_to.empty())
    {
        env.setReplyTo(msg.properties.reply_to);
    }

    if (agents.empty())
    {
        agents.push_back(routing_key__);
    }
    bool no_errors = true;
    for (const auto & i : agents)
    {
        LDEBUG2 << "publishing for" << i;
        if (!channel__->publish(exchange__, i, env))
        {
            LERR << "Error: publishing for" << i;
            no_errors = false;
        }
    }
    return no_errors;
}

namespace {
template <typename R, typename Arg0, typename ... Args>
Arg0 get_first_arg(std::function<R(Arg0, Args...)>);
using MessageCallbackFirstArgType = decltype(get_first_arg(std::declval<AMQP::MessageCallback>())) ;
}//anonymous ns

bool amqp_queue::start_consume__()
{
    std::weak_ptr<detail::text_queue> self(shared_from_this());
    channel__->consume(name__).onReceived(
        [this, self](MessageCallbackFirstArgType message, uint64_t deliveryTag, bool redelivered)
            {
                if (!self.expired())
                {
                    LDEBUG2  << "Got message " << deliveryTag;

                    consumed_messages_internal__.emplace(
                         jms::text_message{ { message.body(), static_cast<size_t>(message.bodySize()) },
                          {
                            message.headers().get(rabbitmq_delay_property),
                            static_cast<int>(std::strtol(message.expiration().c_str(), nullptr, 10)),
                            message.correlationID(),
                            message.replyTo()
                          } }, deliveryTag);
                    if(need_dequeue_notify__)
                    {
                        need_dequeue_notify__ = false;
                        dequeue__();
                    }
                }
            }
        );
    return true;
}


void amqp_queue::dequeue__()
{
    LDEBUG2  << "Need dequeue notify " << need_dequeue_notify__;

    if (need_dequeue_notify__) return;

    if (!consumed_messages_internal__.empty())
    {
        std::lock_guard<std::mutex> lck(queue_mtx);
        if (consumed_messages.empty())
        {
            std::swap(consumed_messages_internal__, consumed_messages);
            queue_cv.notify_all();
        }
        // it clears only here because if need_dequeue_notify__ setted
        // dequeue__ will be called another one time from consume callback 
        // regardless of amqp_queue::dequeue method call
        dequeue_task_working = false;
    }
    else
    {
        need_dequeue_notify__ = true;
    }
}


bool amqp_queue::dequeue(jms::text_message& msg, unsigned wait_delay, bool use_wait_delay, const jms::recepient& agent, const std::string& corr_id_mask)
{
   bool result = false;
   check_error();
   try
   {
       auto get_msg = [&]() -> bool
       {
           if (!consumed_messages.empty())
           {
               msg = std::move(std::get<0>(consumed_messages.front()));
               unacked_message_tag = std::get<1>(consumed_messages.front());
               consumed_messages.pop();
               has_unacked = true;
               return true;
          }
          return false;
       };

       if (!is_consuming_)
       {
           post_and_wait_task("Dequeue start",
                   wait_delay ? std::min(std::chrono::seconds(wait_delay), start_consume_timeout)
                   :start_consume_timeout , &amqp_queue::start_consume__);
           is_consuming_ = true;
           LDEBUG  << "Consuming started";
           check_error();
       }

       if (need_recover)
       {
           post_and_wait_task_with_deferred_result("Recover",
                   wait_delay ? std::min(std::chrono::seconds(wait_delay), start_consume_timeout)
                   :start_consume_timeout , &amqp_queue::recover__);
           check_error();
           LDEBUG  << "Recover requested";
           need_recover = false;
       }


       std::unique_lock<std::mutex> lck(queue_mtx); // LOCK
       check_error();

       if (get_msg())
       {
           return true;
       }

       if (!dequeue_task_working)
       {
           dequeue_task_working = true;
           post_task(&amqp_queue::dequeue__);
       }

       if (use_wait_delay)
       {
           if (wait_delay)
           {
               if (queue_cv.wait_for(lck, std::chrono::seconds(wait_delay)) == std::cv_status::timeout)
               {
                   return false;
               }
           }
           else
           {
               return false;
           }
       }
       else if (!use_wait_delay)
       {
           queue_cv.wait(lck);
       }
       if (get_msg() )
       {
           return true;
       }
       check_error();
       return false;
   }
   catch (const jms::mq_error& e)
   {
       LERR << e.what();
       throw;
   }
   catch (const std::exception& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.what());
   }
   return result;
}

bool amqp_queue::dequeue(jms::text_message& msg, unsigned wait_delay, const jms::recepient& agent, const std::string& corr_id_mask)
{
    return dequeue(msg, wait_delay, true , agent, corr_id_mask);
}

jms::text_message amqp_queue::dequeue(const jms::recepient& agent, const std::string& corr_id_mask)
{
   jms::text_message msg;
   if (!dequeue(msg, 0, false, agent, corr_id_mask))
   {
      jms::mq_error e(-1, "dequeue_failed");
      LERR << e.what();
      throw e;
   }
   return msg;
}

} //namespace AMQ
#endif // HAVE_AMQP_CPP
