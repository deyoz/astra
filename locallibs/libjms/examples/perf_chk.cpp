#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <string>
#include <iostream>
#include <iomanip>
#include <sys/resource.h>

#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "jms.hpp"
#include "text_message.hpp"



namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    std::string connect_string;
    std::string queue_name;
    size_t messages_count = 0;
    size_t commit_count = 0;
    po::options_description promt_arg("Usage");   
    promt_arg.add_options()
        ("help", "show this message")
        ("aq_connect,c", po::value<std::string>(&connect_string)->multitoken(), "connect to database")
        ("queue_name,q", po::value<std::string>(&queue_name), "queue name")
        ("msgs_count,n", po::value(&messages_count)->default_value(100), "benchmark messages count")
        ("commit_count,m", po::value(&commit_count), "count of received before commit")
        ;

       ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, promt_arg), vm);
    try
    {
        if (vm.count("help"))
        {
            std::cout << promt_arg << std::endl;
            return 0;
        }
 
        if (!vm.count("commit_count") || commit_count <= 0 )
        {
            commit_count = messages_count;
        }
        po::notify(vm);

    }
    catch (const std::exception& e) 
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }
    try
    {
        std::cout << "Try to connect" << std::endl;
        jms::connection test_connect(connect_string, false);
        std::cout << "Connected" << std::endl;
        //jms::text_queue tq_enq = test_connect.create_text_queue(queue_name);
        std::cout << "Queue created" << std::endl;
        jms::text_message mess_enq;
        std::string txt = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<benchmark><jms_perf_chk_message text = \"This is a jms perfomance test message\" /></benchmark>";

        std::cout << "Sending " << messages_count << " messages" << std::endl;
        boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
        for (size_t i = 0; i < messages_count; ++i)
        {
            jms::text_queue tq_enq = test_connect.create_text_queue(queue_name);
            mess_enq.text = txt + "<ID>" + std::to_string(i) + "</ID>";

            /*
               std::string stat;
               {
               std::ifstream procst("/proc/self/statm");
               std::getline(procst, stat);
               }

               debug << i << " " << stat << std::endl;
               */
            /*        
                      mess.text = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?><benchmark><jms_perf_chk_message num = \"")+
                      std::string(boost::lexical_cast<std::string>(i)) + "This is a jms perfomance test message\" /></benchmark>";
                      */
            tq_enq.enqueue(mess_enq); 
            //std::cout << "equeue: " << i << " " << mess_enq.text << std::endl;

        }

        boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();

        std::cout << "Sent " << messages_count << " messages in " 
            << std::setw(12) << (finish - start).total_microseconds() 
            << " microseconds" << std::endl;

        start = boost::posix_time::microsec_clock::local_time();
        test_connect.commit();
        finish = boost::posix_time::microsec_clock::local_time();

        std::cout << "Commited " << messages_count << " queues in " 
            << std::setw(12) << (finish - start).total_microseconds() 
            << " microseconds" << std::endl;
 
        jms::text_queue tq_deq = test_connect.create_text_queue(queue_name);
        jms::text_message mess_deq;

        std::cout << "Receiving " << messages_count << " messages" << std::endl;
        start = boost::posix_time::microsec_clock::local_time();

        size_t uncommited = 0;
        for (size_t i = 0; i < messages_count; ++i )
        {
            mess_deq = tq_deq.dequeue();
            if (++uncommited >= commit_count)
            {
                test_connect.commit();
                uncommited = 0;
            }
        }
        finish = boost::posix_time::microsec_clock::local_time();

        std::cout << "Received " << messages_count 
            << " messages in " << std::setw(12) << (finish - start).total_microseconds() << " microseconds" << std::endl;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }


   return 0;
}
