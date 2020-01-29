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
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
 

namespace
{
class cerrlogger
{
public:
    cerrlogger(const char* file, int line)
    {
    }
    virtual ~cerrlogger() { std::cerr << std::endl; }
    template <typename T>
    const cerrlogger& operator << (const T& t) const
    {
        std::cerr << t;
        return *this;
    }
};

}
#define LINFO  cerrlogger(__FILE__, __LINE__)






namespace
{
volatile sig_atomic_t working = 1;
volatile sig_atomic_t watchdog = 1;
volatile sig_atomic_t chld_working = 0;
std::chrono::steady_clock::time_point last_send_time = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point last_recv_time = std::chrono::steady_clock::now();
pid_t pid = 0;
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

   if (sig == SIGUSR1)
   {
       LINFO <<  getpid() << " Maintance stop";
       working = 0;
   }
   else if (sig == SIGUSR2)
   {
       LINFO <<  getpid() <<" SIGUSR2 Received";
       watchdog = 1;
   }
   else if (sig == SIGINT)
   {
       LINFO << getpid() << " SIGINT Received";
       working = 0;
   }
   else if (sig == SIGCHLD)
   {
       LINFO <<  getpid() << " SIGCHLD Received";
       chld_working = 0;
   }
}




void resender_main(const resend_config& config)
{
    jms::connection send_connect(config.aq_connect_to);
    jms::text_queue send_queue = send_connect.create_text_queue(config.queue_name_to);
    jms::connection receive_connect(config.aq_connect_from);
    jms::text_queue receive_queue = receive_connect.create_text_queue(config.queue_name_from);
    jms::text_message msg;
    std::chrono::seconds retry_timeout =  config.watchdog_period / 4;
    std::chrono::seconds send_timeout = config.watchdog_period / 2;
    int iretry_timeout = retry_timeout.count();
    while (working)
    {
        kill(pid, SIGUSR2);
        try
        {
            if (!receive_queue.dequeue(msg, iretry_timeout))
            {
                auto last_time = std::chrono::steady_clock::now();
                bool need_send_check = false;
                bool db_broken = false;

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

            send_queue.enqueue(msg);
            send_connect.commit();
            last_send_time = std::chrono::steady_clock::now();
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


    signal(SIGHUP, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGCHLD, sig_handler);


    while (working)
    {
        chld_working = 1;
        watchdog = 1;
        pid = fork();
        switch (pid)
        {
        case -1:
            throw std::runtime_error("can't fork");
        case 0:
            pid = getppid();
            resender_main(conf);
            return 0;
        default:
            break;
        }

        while (watchdog && chld_working)
        {
            watchdog = 0;
            unsigned int time_to_wait = watchdog_period;

            while ((time_to_wait = sleep(time_to_wait)))
            {
                if(!working)
                {
                    kill(pid, SIGINT);
                    time_to_wait = 5;
                    while ((time_to_wait = sleep(time_to_wait)))
                    {
                        if (!chld_working)
                        {
                            break;
                        }
                    }
                    if (chld_working)
                    {
                        kill(pid, SIGKILL);
                    }
                    break;
                }
                if (!chld_working)
                {
                    break;
                }
            }
        }

        if (chld_working)
        {
            kill(pid, SIGKILL);
        }
        waitpid(pid, NULL, 0);
    }

    return 0;
}
