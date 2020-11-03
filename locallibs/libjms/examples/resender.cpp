#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "jms.hpp"
#include "text_message.hpp"
#include <signal.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

// This is GCC stdlibc++ bug 54562 workaround
// check  https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54562
// for issue description

#if !defined _LIBCPP_VERSION && defined __GLIBCXX__  && defined _GLIBCXX_USE_CLOCK_MONOTONIC \
    && !defined NO_STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND && !defined STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
#if defined __GNUC__  && (__GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 9))) && !defined __clang__
#define STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
#elif !defined STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND && !defined NO_STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
#warning You Are using libstdc++ from gcc. Please check that library corresponds to gcc version 4.9 or above, or you need to define STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
//#define STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
#endif // __GNUC__ version


#endif //_LIBCPP_VERSION etc

namespace
{
class cerrlogger
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock;
public:
    cerrlogger(const char* file, int line) : lock(mtx)
    {
        std::cerr << file << ':' << line << '\t';
    }
    virtual ~cerrlogger() { std::cerr << std::endl; }
    template <typename T>
    const cerrlogger& operator << (const T& t) const
    {
        std::cerr << t;
        return *this;
    }
};
std::mutex cerrlogger::mtx;
}
#define LINFO  cerrlogger(__FILE__, __LINE__)






namespace
{
volatile sig_atomic_t working = 1;
std::atomic_flag watchdog = ATOMIC_FLAG_INIT;
std::atomic_flag started = ATOMIC_FLAG_INIT;

std::chrono::steady_clock::time_point last_send_time = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point last_recv_time = std::chrono::steady_clock::now();
std::timed_mutex lock_mtx;

std::shared_ptr<jms::connection> send_conn_ptr;
std::shared_ptr<jms::connection> recv_conn_ptr;

}

namespace po = boost::program_options;
struct resend_config
{
    std::string aq_connect_to;
    std::string aq_connect_from;
    std::string queue_name_to;
    std::string queue_name_from;
    std::chrono::seconds watchdog_period;
};


void sig_handler(int sig)
{
    working = 0;
}




void resender_main(const resend_config& config)
{
    std::lock_guard<std::timed_mutex> lock(lock_mtx);
    started.clear();
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);

    send_conn_ptr.reset(new jms::connection(config.aq_connect_to));
    jms::connection& send_connect(*send_conn_ptr);
    jms::text_queue send_queue = send_connect.create_text_queue(config.queue_name_to);
    recv_conn_ptr.reset(new jms::connection(config.aq_connect_from));
    jms::connection& receive_connect(*recv_conn_ptr);
    jms::text_queue receive_queue = receive_connect.create_text_queue(config.queue_name_from);
    jms::text_message msg;
    std::chrono::seconds retry_timeout =  config.watchdog_period / 4;
    std::chrono::seconds send_timeout = config.watchdog_period / 2;
    int iretry_timeout = retry_timeout.count();

    while (working)
    {
        watchdog.test_and_set();
        try
        {
            if (!receive_queue.dequeue(msg, iretry_timeout))
            {
                auto last_time = std::chrono::steady_clock::now();
                bool need_send_check = false;
                bool db_broken = false;
                {
                    if ( std::chrono::duration_cast<std::chrono::seconds>
                                (last_time - last_send_time)  > send_timeout)
                    {
                        last_send_time = last_time;
                        need_send_check = true;
                    }

                    if( std::chrono::duration_cast<std::chrono::seconds>
                            (last_time - last_recv_time)  <   retry_timeout / 2)
                    {
                        db_broken = true;
                    }
                    last_recv_time = last_time;
                }

                if (db_broken)
                {
                    LINFO << "db broken";
                    return;
                }

                if (need_send_check)
                {
                    LINFO << "checking send connect";
                    send_connect.check();
                }
                LINFO << "rolling";
                continue;
            }

            LINFO << "resend";
            LINFO << msg.text;

            send_queue.enqueue(msg);
            send_connect.commit();
            {
                last_send_time = std::chrono::steady_clock::now();
            }
            receive_connect.commit();

        }
        catch (const std::exception& e)
        {
            LINFO << e.what();
            break;
        }

    }
}

int main(int argc, char* argv[])
{

    resend_config conf;
    po::options_description config("File configuration");
    int watchdog_period = 100;
    config.add_options()
        ("aq_connect_to", po::value(&conf.aq_connect_to)->multitoken(), "connect to database to enque")
        ("aq_connect_from", po::value(&conf.aq_connect_from), "connect to database to deque")
        ("queue_name_to", po::value(&conf.queue_name_to), "queue name to enque")
        ("queue_name_from", po::value(&conf.queue_name_from), "queue name to deque")
        ("watchdog_period", po::value(&watchdog_period), "benchmark messages count")
        ;
    po::options_description promt_arg("Usage");
    promt_arg.add_options()
        ("help", "show this message")
        ("config,c", po::value<std::string>()->default_value("resend.conf"), "path to configuration file")
        ;

    po::variables_map vm;
    watchdog_period = std::max(2, watchdog_period);

#ifdef STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
    const auto steady_clock_epoch_diff = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
        - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch());
#endif //STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND


    try
    {
        po::store(po::parse_command_line(argc, argv, promt_arg), vm);

        if(vm.count("help"))
        {
            std::cout << promt_arg << std::endl << config << std::endl;

            return 0;
        }
        po::store(po::parse_environment(promt_arg,"JMS_RESENDER_"), vm);

        std::ifstream ifs(vm["config"].as<std::string>().c_str());
        if(!ifs)
        {
            throw std::logic_error(std::string("Config file : `") + vm["config"].as<std::string>() + "\' not found " );

        }

        po::store(po::parse_config_file(ifs, config, true ), vm);
        po::notify(vm);
        conf.watchdog_period = std::chrono::seconds(watchdog_period);

    }
    catch(const std::exception& e)
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }


    while (working)
    {
        started.test_and_set();
        std::thread resend_task(std::bind(&resender_main, conf));
        int lock_try = 0;
        const std::chrono::seconds lock_try_period(1);
        const int max_lock_try = conf.watchdog_period / lock_try_period;
        while(lock_try++ < max_lock_try && started.test_and_set())
        {
            std::this_thread::sleep_for(lock_try_period);
        }
        if (lock_try >= max_lock_try)
        {
            LINFO << "Seems working thread deadlocked";
            pthread_cancel(resend_task.native_handle());
            resend_task.detach();
            continue;
        }

        std::unique_lock<std::timed_mutex> join_lock(lock_mtx, std::defer_lock);

        while (resend_task.joinable())
        {
            watchdog.clear();
            LINFO << "joinable";
#ifdef STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
            if (join_lock.try_lock_until(std::chrono::steady_clock::now() + steady_clock_epoch_diff + conf.watchdog_period ))
#else
            if (join_lock.try_lock_for(conf.watchdog_period ))
#endif //STDLIBCXX_MUTEX_TRY_LOCK_FOR_WORKAROUND
            {
                join_lock.unlock();

                if (resend_task.joinable())
                {
                    LINFO << "join";
                    resend_task.join();
                }
            }
            else
            {
                LINFO << "watchdog";
                if (!watchdog.test_and_set())
                {
                    LINFO << "Resend Thread " << resend_task.get_id() << " frozen ";
                    send_conn_ptr->break_wait();
                    recv_conn_ptr->break_wait();
                    pthread_cancel(resend_task.native_handle());
                    resend_task.detach();
                }
            }

        }
    }

    return 0;
}
