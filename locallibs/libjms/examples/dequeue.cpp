#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <iostream>
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
jms::connection * conn_ptr = nullptr ;
void sig_handler(int sig)
{
   s_terminate = 1;
   if (conn_ptr) conn_ptr->break_wait();
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

namespace
{

namespace po = boost::program_options;
po::variables_map vm;

bool init(int argc, char *argv[])
{
    try
    {
        po::options_description config("Usage");
        config.add_options()
          ("help", "show this message")
          ("queue,q", po::value<std::string>(), "queue name")
          ("connect,c", po::value<std::string>(), "connect string to broker")
          ("recepient,r", po::value<std::string>(), "recepient")
          ("file,f", po::value<std::string>(), "input filename")
          ("timeout,t", po::value<unsigned>(), "input filename")

          ;

        po::store(po::parse_command_line(argc, argv, config), vm);

        if (vm.count("help"))
        {
            std::cout << config << std::endl;
            return false;
        }
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

int main(int argc, char* argv[])
{
   if (!init(argc, argv))
   {
      return 1;
   }

   std::string connect_string;
   std::string queue_name;
   std::string file_name;
   std::string recepient;
   unsigned timeout = 0;

   if (!vm.count("connect"))
   {
      std::cout << "No connect string specified" << std::endl;
      return 1;
   }
   else
   {
      connect_string = vm["connect"].as<std::string>();
   }

   if (!vm.count("queue"))
   {
      std::cout << "No queue name specified" << std::endl;
      return 1;
   }
   else
   {
      queue_name = vm["queue"].as<std::string>();
   }

   if (vm.count("recepient"))
   {
      recepient = vm["recepient"].as<std::string>();
   }

   if (vm.count("timeout"))
   {
      timeout = vm["timeout"].as<unsigned>();
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

   int ret = 0;
   try
   {
       jms::connection que_connect(connect_string);
       conn_ptr = &que_connect;
       jms::text_queue output_queue(que_connect.create_text_queue(queue_name));
       jms::text_message msg;
       while (!s_terminate)
       {
           std::cout << "Dequeueing" << std::endl;
           if ( output_queue.dequeue(msg, timeout, recepient))
           {
               std::cout << "Dequeued: \n" << msg.text << std::endl;
               que_connect.commit();
           }
        }
        conn_ptr = nullptr;
        std::cout << "Exiting" << std::endl;
   }
   catch (const jms::mq_error& e)
   {
       conn_ptr = nullptr;
       std::cerr << e.what() << " - " << e.get_errcode() << std::endl;
       ret = 1;
   }
   s_terminate = 1;
   kill(getpid(), SIGUSR2);
   sig_thread.join();
   return ret;
}

