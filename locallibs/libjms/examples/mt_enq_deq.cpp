#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>

#include <jms.hpp>
#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

namespace
{
volatile sig_atomic_t s_terminate = 0;
sigset_t sig_set;
void sig_handler(int sig)
{
   s_terminate = 1;
}

void signal_handler_thread()
{
    std::cerr << "starting sig thread" << std::endl;
    while (!s_terminate)
    {
        int sig = 0;
        sigwait(&sig_set, &sig);
        if (sig == SIGUSR2)
        {
            continue;
        }
        sig_handler(sig);
    }
    std::cerr << "Terminated sig thread" << std::endl;
}

}

struct mt_enq_deq_config
{
    mt_enq_deq_config():timeout(0), dequeue_threads(1), enqueue_threads(1), messages_count(100),commit_count(1), parallel_work(true)
    {}
    std::string connect_to;
    std::string connect_from;
    std::string queue_name_to;
    std::string queue_name_from;
    unsigned timeout;
    unsigned dequeue_threads;
    unsigned enqueue_threads;
    unsigned messages_count;
    unsigned commit_count;
    bool parallel_work;
};

namespace
{

namespace po = boost::program_options;
po::variables_map vm;




bool init(int argc, char *argv[], mt_enq_deq_config& conf)
{
    try
    {
        po::options_description config("File configuration");
        config.add_options()
            ("connect_to", po::value(&conf.connect_to)->multitoken(), "connect to broker for enqueue")
            ("connect_from", po::value(&conf.connect_from), "connect to broker for dequeue")
            ("queue_name_to", po::value(&conf.queue_name_to), "queue name to enqueue")
            ("queue_name_from", po::value(&conf.queue_name_from), "queue name to dequeue")
            ("dequeue_threads", po::value(&conf.dequeue_threads)->default_value(1), "dequeue threads count")
            ("enqueue_threads", po::value(&conf.enqueue_threads)->default_value(1), "enqueue threads count")
            ("timeout", po::value(&conf.timeout)->default_value(5), "timeout for one dequeue try")
            ("messages_count", po::value(&conf.messages_count)->default_value(100), "benchmark messages count")
            ("commit_count", po::value(&conf.commit_count), "messages count received before commit")
            ("parallel_work", po::value(&conf.parallel_work)->default_value(true), "if receive an transmit should be simultaneous")
            ;

        po::options_description promt_arg("Usage");
        promt_arg.add_options()
            ("help", "show this message")
            ("config,c", po::value<std::string>()->default_value("mtenqdeq.conf"), "path to configuration file")
            ;


        po::store(po::parse_command_line(argc, argv, promt_arg), vm);

        if (vm.count("help"))
        {
            std::cout << promt_arg << std::endl << config << std::endl;
            return false;
        }
        std::ifstream ifs(vm["config"].as<std::string>().c_str());
        if(!ifs)
        {
            throw std::logic_error(std::string("Config file : `") + vm["config"].as<std::string>() + "\' not found " );

        }
/*
        if (!vm.count("commit_count") || conf.commit_count <= 0 )
        {
            conf.commit_count = conf.messages_count;
        }
*/
        po::store(po::parse_config_file(ifs, config, true ), vm);
        po::notify(vm);
    }
    catch (const std::exception& e)
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return false;
    }
    return true;
}

}


void enqueuer(mt_enq_deq_config conf, int id , size_t& i)
{
    try
    {
        jms::connection test_connect(conf.connect_to, false);
        jms::text_message mess_enq;
        std::string txt = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<test><jms_perf_chk_message text = \"This is a jms perfomance test message\" /></test>";
        jms::text_queue tq_enq = test_connect.create_text_queue(conf.queue_name_to);
        size_t uncommited = 0;
        for (i = 0; i < conf.messages_count && !s_terminate ; ++i)
        {
            mess_enq.text = txt
                + "<ID>" + std::to_string(i) + "</ID><SENDER>"
                + std::to_string(id) + "</SENDER>";
            tq_enq.enqueue(mess_enq);
            if (++uncommited >= conf.commit_count)
            {
                test_connect.commit();
                uncommited = 0;
            }
        }
        test_connect.commit();
    }
    catch (const jms::mq_error& e)
    {
        std::cerr << "Enqueue thread #" << id << " failed with exception:" << e.what() <<  "(" << e.get_errcode() << ")\n";
    }

}

void dequeuer(mt_enq_deq_config conf, int id , size_t& i)
{
    try
    {
        jms::connection test_connect(conf.connect_from, false);
        jms::text_message mess_deq;
        jms::text_queue tq_deq = test_connect.create_text_queue(conf.queue_name_from);
        std::chrono::steady_clock::time_point tm = std::chrono::steady_clock::now();
        size_t uncommited = 0;
        for (i = 0; i < conf.messages_count && !s_terminate ; )
        {
            if (tq_deq.dequeue(mess_deq, conf.timeout))
            {
                if (++uncommited >= conf.commit_count)
                {
                    test_connect.commit();
                    uncommited = 0;
                }
                ++i;
            }
            else if (!conf.timeout)
            {
                if (std::chrono::duration_cast<std::chrono::seconds>( std::chrono::steady_clock::now() - tm).count() > conf.messages_count / 100  )
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        test_connect.commit();
    }
    catch (const jms::mq_error& e)
    {
        std::cerr << "Dequeue thread #" << id << " failed with exception:" << e.what() <<  "(" << e.get_errcode() << ")\n";
    }
}



int main(int argc, char* argv[])
{
   mt_enq_deq_config conf;
   if (!init(argc, argv, conf))
   {
      return 1;
   }

   sigemptyset(&sig_set);
   sigaddset(&sig_set, SIGHUP);
   sigaddset(&sig_set, SIGINT);
   sigaddset(&sig_set, SIGUSR1);
   sigaddset(&sig_set, SIGUSR2);

   if (pthread_sigmask(SIG_BLOCK, &sig_set, NULL))
   {
      std::cerr << "Can not setup signal handlers " << std::endl;
      return 1;
   }
   std::thread sig_thread(&signal_handler_thread);

   std::vector<std::thread> senders(conf.enqueue_threads);
   std::vector<std::thread> receivers(conf.dequeue_threads);
   std::vector<size_t> send_counters(senders.size());
   std::vector<size_t> recv_counters(receivers.size());


   int ret = 0;
   try
   {
       for (size_t i = 0; i < senders.size(); ++i)
       {
           senders[i] = std::thread(enqueuer, conf, i, std::ref(send_counters[i]));
       }

       auto wait_for_send = [&]() { for(auto & t : senders) { t.join(); } };

       if (!conf.parallel_work) { wait_for_send(); }


       for (size_t i = 0; i < receivers.size(); ++i)
       {
           receivers[i] = std::thread(dequeuer, conf, i, std::ref(recv_counters[i]));
       }

       if (conf.parallel_work) { wait_for_send(); }
       for(auto & t : receivers)
       {
           t.join();
       }


       size_t i = 0;
       for (auto c : send_counters)
       {
           std::cout << "Sender " << std::setw(6) << (++i) << " " << std::setw(6) << c << " messages sent \n"  ;
       }
       std::cout << std::endl;
       i = 0;
       for (auto c : recv_counters)
       {
           std::cout << "Receiver " << std::setw(6) << (++i) << " " << std::setw(6) << c << " messages received \n"  ;
       }
       std::cout << std::endl;

       std::cout << "Exiting" << std::endl;
   }
   catch (const jms::mq_error& e)
   {
       std::cerr << e.what() << " - " << e.get_errcode() << std::endl;
       ret = 1;
   }
   s_terminate = 1;
   kill(getpid(), SIGUSR2);
   sig_thread.join();
   return ret;
}

