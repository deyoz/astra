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



namespace po = boost::program_options;

namespace
{
/*

const std::string NO_CONFIG_FILE = "config file not found";

std::string get_filename(const po::variables_map& varmap)
{
   if(varmap.count("config-file"))
   {
      return varmap["config-file"].as<std::string>();
   }

   const char* file_name = getenv("GET_MSGS_CONFIG");
   if(!file_name)
   {
       file_name = "get_msgs.conf";      
   }
   
   return std::string(file_name);
}
*/

}

int main(int argc, char* argv[])
{
   
    //set_debug_log("stplib internal functionality", false);
    /*
       int main()
       {
       jms::connection test_connect("queadm/queadm");
       jms::text_queue tq = test_connect.create_text_queue("TEST_QUEUE");

       jms::text_message mess;
       while (!std::cin.eof()) {
       char c = 0;
       std::cin.get(c);
       mess.text += c;
       }
       tq.enqueue(mess);
       test_connect.commit();
       std::cout << "enqueued bytes: " << mess.text.size() << std::endl;

       return 0;
       }
       */
    po::options_description config("File configuration");
    config.add_options()
        ("aq_connect", po::value<std::string>()->multitoken(), "connect to database")
        ("aq_connect_deq", po::value<std::string>()->multitoken()->default_value(""), "connect to database to dequeue (if needed)")
        ("queue_name", po::value<std::string>(), "queue name")
        ("queue_name_deq", po::value<std::string>()->default_value(""), "queue name to deque (if needed)")
        ("delay", po::value<int>()->default_value(0), "message delay time")
        ("expiration", po::value<int>()->default_value(0), "message expiration time")
        ("corr_id", po::value<std::string>()->multitoken()->default_value(""), "correlation id for enqueue")
        ("corr_id_deq", po::value<std::string>()->multitoken()->default_value(""), "correlation id mask for dequeue")
        ("timeout", po::value<int>()->default_value(0), "dequeue timeout")
        ;


    po::options_description promt_arg("Usage");   
    promt_arg.add_options()
        ("help", "show this message")
        ("config,c", po::value<std::string>()->default_value("dc_chk.conf"), "path to configuration file")
        ("noenqueue,e", "no enqueue message")
        ("nodequeue,d", "no dequeue message")
        ("timeout,t", po::value<int>()->default_value(0), "dequeue timeout")
        ("message,m", "get custom message from stdin")
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
        po::store(po::parse_environment(promt_arg, "JMS_DC_CHK_"), vm);

        std::ifstream ifs(vm["config"].as<std::string>().c_str());
        if(!ifs)
        {
            throw std::logic_error(std::string("Config file: `") + vm["config"].as<std::string>() + "\' not found ");

        }

        po::store(po::parse_config_file(ifs, config, true), vm);          

        po::notify(vm);

        if(vm["aq_connect_deq"].defaulted())
        {
            vm.erase("aq_connect_deq");
            vm.insert(std::make_pair("aq_connect_deq", vm["aq_connect"]));
        }
        if(vm["queue_name_deq"].defaulted())
        {
            vm.erase("queue_name_deq");
            vm.insert(std::make_pair("queue_name_deq", vm["queue_name"]));
        }

    }
    catch(const std::exception& e) 
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }

    jms::connection test_connect(vm["aq_connect"].as<std::string>());
    jms::text_queue tq = test_connect.create_text_queue(vm["queue_name"].as<std::string>());
    jms::text_message mess;

    if(vm.count("message"))
    {
        while (std::cin.good())
        {
            char c = 0;
            std::cin.get(c);
            if(std::cin.good())
            {
               mess.text+=c;
            }
        }
    }
    else
    {
        mess.text = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<benchmark><jms_perf_chk_message text = \"This is a jms delay and correlation id test message\" /></benchmark>";
    }

    mess.properties.delay = vm["delay"].as<int>();
    mess.properties.expiration = vm["expiration"].as<int>();
    mess.properties.correlation_id = vm["corr_id"].as<std::string>();
 
    boost::posix_time::ptime start, finish;
    if(!vm.count("noenqueue"))
    {

        start = boost::posix_time::microsec_clock::local_time();

        tq.enqueue(mess);
        test_connect.commit();    

        finish = boost::posix_time::microsec_clock::local_time();

        std::cerr << "Enqueue message in " << std::setw(12) << (finish - start).total_microseconds() << " microseconds" << std::endl;
    }

    std::cout.flush();

    
    jms::connection test_connect_deq(vm["aq_connect_deq"].as<std::string>());
    jms::text_queue tq_deq = test_connect_deq.create_text_queue(vm["queue_name_deq"].as<std::string>());
    jms::text_message mess_deq;
    int dequeue_timeout = vm["timeout"].as<int>();

    if(!vm.count("nodequeue"))
    {

        start = boost::posix_time::microsec_clock::local_time();    
        if (dequeue_timeout)
        {
            tq_deq.dequeue(mess_deq, dequeue_timeout, "", vm["corr_id_deq"].as<std::string>());
        }
        else
        {
            mess_deq = tq_deq.dequeue("", vm["corr_id_deq"].as<std::string>());
        }

        test_connect_deq.commit();    

        std::cerr << mess_deq.properties.delay << "\t"<< mess_deq.properties.expiration << "\t"
                  << mess_deq.properties.correlation_id << "\n Message:" << std::endl;
        std::cout << mess_deq.text << std::endl;

        finish = boost::posix_time::microsec_clock::local_time();

        std::cerr << "Received message in " << std::setw(12) 
                  << (finish - start).total_microseconds() 
                  << " microseconds" << std::endl;
    }

   return 0;
}
