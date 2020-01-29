#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <cassert>
#include <iostream>
#include "jms.hpp"
#include "text_message.hpp"
#include "amqp.hpp"


int main(int argc, char* argv[])
{

   {
       amqp::Address a("amqps://user:pass@myhost.net:666/");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 666 );
   }

   {
       amqp::Address a("amqps://user:pass@myhost.net/");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
   }

   {
       amqp::Address a("amqp://user:pass@myhost.net/");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == false  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5672 );
   }
   {
       amqp::Address a("amqps://user:pass@myhost.net:666/hostname");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "hostname"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 666 );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666hostname");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number") );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666h/ostname");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number") );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.ne:t666/hostname");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number") );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:66666");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number" ) );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:/aa");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number" ) );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong port number" ) );
   }



   {
       amqp::Address a("amqps://user:pass@myhost.net:666/hostname?QoS=100&dummy=dummy");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "hostname"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 666 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "dummy" );
   }

   {
       amqp::Address a("amqps://user:pass@myhost.net?QoS=100");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://user:@myhost.net?QoS=100");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://:@myhost.net?QoS=100");
       assert(a.login().user()     == "" );
       assert(a.login().password() == "" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://:pass@myhost.net?QoS=100");
       assert(a.login().user()     == "" );
       assert(a.login().password() == "pass" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }


   {
       amqp::Address a("amqps://@myhost.net?QoS=100");
       assert(a.login().user()     == "" );
       assert(a.login().password() == "" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://user@myhost.net?QoS=100");
       assert(a.login().user()     == "user" );
       assert(a.login().password() == "" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "myhost.net" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://?QoS=100");
       assert(a.login().user()     == "guest" );
       assert(a.login().password() == "guest" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "localhost" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos")  == 100 );
       assert(a.get_str_param("dummy")  == "" );
   }

   {
       amqp::Address a("amqps://");
       assert(a.login().user()     == "guest" );
       assert(a.login().password() == "guest" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "localhost" );
       assert(a.port()             == 5671 );
       assert(a.get_int_param("qos",1)  == 1 );
       assert(a.get_str_param("dummy")  == "" );
  }
  {
       amqp::Address a("amqps://:1000");
       assert(a.login().user()     == "guest" );
       assert(a.login().password() == "guest" );
       assert(a.is_tls()           == true  );
       assert(a.vhost()            == "/"  );
       assert(a.hostname()         == "localhost" );
       assert(a.port()             == 1000 );
       assert(a.get_int_param("qos",1)  == 1 );
       assert(a.get_str_param("dummy")  == "" );
   }




   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?/hostname");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong options separator (?) position") );
   }


   try
   {
       amqp::Address a("amqps://user:pass@myhost.net?:666");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP address: wrong options separator (?) position") );
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?Qos");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP params: inconsistent pair-value token" ));
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?Qos&dummy=11");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP params: inconsistent pair-value token" ));
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?Qos=1&dummy");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP params: inconsistent pair-value token" ));
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?Qos=1&qoS=10");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP params: duplicated parameter: qoS" ));
   }

   try
   {
       amqp::Address a("amqps://user:pass@myhost.net:666?Qos=1&QQS=10");
   }
   catch(const jms::mq_error& e)
   {
       assert(e.what()             == std::string("invalid AMQP params: unknown parameter: QQS" ) );
   }



   if (argc > 1) try
   {
       amqp::Address a(argv[1]);
       std::cout
           << "\nUser : " << a.login().user()
           << "\nPass : " << a.login().password()
           << "\nTLS  : " << a.is_tls()
           << "\nVHost: " << a.vhost()
           << "\nHost : " << a.hostname()
           << "\nPort : " << a.port()
           << "\nQoS  : " << a.get_int_param("qos",1)
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

