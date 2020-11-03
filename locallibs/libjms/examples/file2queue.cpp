#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include <fstream>
#include <sstream>
#include <jms.hpp>
#include <boost/program_options.hpp>


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

   if (!vm.count("file"))
   {
      std::cout << "No input filename specified" << std::endl;
      return 1;
   }
   else
   {
      file_name = vm["file"].as<std::string>();
   }

   if (vm.count("recepient"))
   {
      recepient = vm["recepient"].as<std::string>();
   }
   try
   {
       jms::connection que_connect(connect_string);
       jms::text_queue input_queue(que_connect.create_text_queue(queue_name));

       std::ifstream input_file(file_name, std::ios_base::in | std::ios_base::binary);
       if (!input_file.good())
       {
          std::cout << "Cannot open file" << std::endl;
          return 1;
       }
       jms::text_message test_event;
       jms::recepients agents;
       if (!recepient.empty())
       {
          agents.push_back(recepient);
          test_event.properties.reply_to = recepient;
       }
       std::cerr << "Try to enqueue" << std::endl;

       test_event.text.assign(std::istreambuf_iterator<decltype(test_event.text)::value_type>(input_file), std::istreambuf_iterator<decltype(test_event.text)::value_type>());
       input_queue.enqueue(test_event, agents);
       std::cerr << "Try to commit" << std::endl;
       que_connect.commit();
   }
   catch (const jms::mq_error& e)
   {
       std::cerr << e.what() << " - " << e.get_errcode() << std::endl;
       return 1;
   }
   catch (...)
   {
       std::cerr << "Unhandled exception " << std::endl;
       return 1;
   }
   return 0;
}

