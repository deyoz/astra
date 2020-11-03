#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <cassert>
#include <iostream>
#include "jms.hpp"
#include "text_message.hpp"
#include "aq.hpp"

namespace AQ
{
oracle_param split_connect_string(const std::string& connect_string);
}


int main(int argc, char* argv[])
{

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost.net:666");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost.net:666");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost.net");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost.net");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass?payload=SYS.BINARYAQPL");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "");
       assert(p.payload_ns     == "SYS");
       assert(p.payload_type_name     == "BINARYAQPL");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=SYS.BINARYAQPL");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "SYS");
       assert(p.payload_type_name     == "BINARYAQPL");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=BINARYAQPL");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "USER");
       assert(p.payload_type_name     == "BINARYAQPL");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=Jms");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   try
   {
       AQ::oracle_param p = AQ::split_connect_string("userpass@myhost");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("Invalid oracle connect string: userpass@myhost") );
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=Jms&dummy=dummy");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }

   {
       AQ::oracle_param p = AQ::split_connect_string("oracle://user/pass@myhost/XE?payload=Jms&dummy=dummy");
       assert(p.login        == "user" );
       assert(p.password     == "pass" );
       assert(p.server     == "myhost/XE");
       assert(p.payload_ns     == "");
       assert(p.payload_type_name     == "");
   }



   try
   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload");

   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AQ params: inconsistent pair-value token" ));
   }

   try
   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload&dummy=11");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AQ params: inconsistent pair-value token" ));
   }

   try
   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=Jms&dummy");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AQ params: inconsistent pair-value token" ));
   }

   try
   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?payload=Jms&PAYLOAD=BIN1");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AQ params: duplicated parameter: PAYLOAD" ));
   }


   try
   {
       AQ::oracle_param p = AQ::split_connect_string("user/pass@myhost/XE?peyload=Jms");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AQ params: unknown parameter: peyload" ) );
   }



   if (argc > 1) try
   {
       AQ::oracle_param p = AQ::split_connect_string(argv[1]);
       std::cout
           << "\nlogin             : " << p.login
           << "\npassword          : " << p.password
           << "\nserver            : " << p.server
           << "\npayload_ns        : " << p.payload_ns
           << "\npayload_type_name : " << p.payload_type_name
           << std::endl;
   }
   catch(const std::exception & e)
   {
       std::cerr << "\nException in test from command line :\n " << e.what() << std::endl;
       return 1;
   }

   std::cout << "\n\nTest Passed\n" << std::endl;
   return 0;
}

