#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */


#define BOOST_UTF8_BEGIN_NAMESPACE namespace test {
#define BOOST_UTF8_END_NAMESPACE }
#define BOOST_UTF8_DECL

// boost 1.63 fix
#ifdef VERSION
  #undef VERSION
#endif   
// end boost 1.63 fix

//#define BOOST_TEST_MODULE MQ_TEST
#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include "jms.hpp"
#include "text_message.hpp"
//#include "tests_main.hpp"
//#include "prog_args.hpp"
//#include "log/log.hpp"

namespace po = boost::program_options;

namespace
{

std::string connect_string;
std::string queue_name;

void init_tests(int argc, char* argv[])
{
    po::options_description promt_arg("Usage");
    promt_arg.add_options()
        ("help", "show this message")
        ("aq_connect,c", po::value<std::string>(&connect_string)->multitoken(), "connect to database")
        ("queue_name,q", po::value<std::string>(&queue_name), "queue name")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, promt_arg), vm);
    try
    {

        if (vm.count("help"))
        {
            std::cout << promt_arg << std::endl;
            return;
        }
        po::notify(vm);

    }
    catch (const std::exception& e)
    {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return;
    }

}


}

using boost::unit_test::test_suite;



BOOST_AUTO_TEST_CASE(russian_message_test)
{
   jms::connection test_connect(connect_string);
   test_connect.check();
   /*
   jms::text_queue queue = test_connect.create_text_queue(queue_name);
   for (size_t i = 4000; i < 5000; ++i)
   {
      jms::text_message message1, message2;
      jms::recepients agents;
      message1.text = std::string(i, 'A');
      queue.enqueue(message1, agents);
      //std::cerr << "enqueue: " << i - 4000 << " message size: " << message1.text.size() << std::endl;
      message2 = queue.dequeue();
      //std::cerr << "dequeue: " << i - 4000 << " message size: " << message2.text.size() << std::endl;
      BOOST_CHECK(message1.text == message2.text);
   }
   test_connect.commit();
   */
}

BOOST_AUTO_TEST_CASE(huge_message_test)
{
   //BOOST_FAIL("skip blob");
   /*
   jms::connection test_connect("test/test@oracle/main");
   jms::text_queue queue = test_connect.create_text_queue("PAYRESULT");
   jms::text_message message1;
   
   
   message1.text = std::string(4000, 'G');
   for (int i = 0; i < 10; ++i)
   {
      message1.text += "MEDVED";
   }
   std::cerr << message1.text << std::endl;
   //LINFO << "Enqueue size: " << message1.text.size();
   queue.enqueue(message1);
   //LINFO << "enqueue";   
   jms::text_message message2 = queue.dequeue();
   std::cerr << message2.text << std::endl;
   //LINFO << "dequeue";
   //LINFO << "Dequeue size: " << message2.data.size();
   BOOST_CHECK(message1.text == message2.text);
   */
}

/*
BOOST_AUTO_TEST_CASE(text_message)
{
   //jms::connection test_connect("test/test@oracle/main");
   jms::connection test_connect("queadm/queadm@oracle/main");
   jms::text_queue tq = test_connect.create_text_queue("PAYRESULT");
   for (int i = 0; i < 3; ++i)
   {
      jms::text_message tmp;
      tmp.text = std::string(3000, 'B'); 
      tq.enqueue(tmp);
      //tmp = tq.dequeue();
   }
   //test_connect.rollback();
   for(int i = 0; i < 3; ++i)
   {
      std::cout << "dequeing message..." << std::endl;
      jms::text_message mess = tq.dequeue();
      std::cout << "message is dequeued" << std::endl;
      std::cout << mess.text << std::endl;
      std::cout << mess.text.size() << std::endl;
      test_connect.commit();
   }
   //test_connect.commit();

}*/


test_suite* init_unit_test_suite(int argc, char* argv[])
{
   boost::unit_test::framework::master_test_suite().p_name.value = "MAIN";
   init_tests(argc, argv);
   return 0;
}

