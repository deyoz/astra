#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include "jms.hpp"
#include "text_message.hpp"


void correlation_id_change()
{
   jms::connection test_connect("ego/ego@aq_main");
   jms::text_queue tq = test_connect.create_text_queue("AQADM.TEST1_QUEUE");
   jms::text_message message;
   std::cout << "dequeing message..." << std::endl;
   bool result = tq.dequeue(message, 4, std::string(), "4020");
   test_connect.rollback();
   result = tq.dequeue(message, 4, std::string(), "4021");
   std::cout << "message is dequeued: " << result << std::endl;
   std::cout << message.text << std::endl;
   std::cout << message.text.size() << std::endl;
   test_connect.rollback();
}

void wait_connected_message()
{
   jms::connection test_connect("ego/ego@aq_main");
   jms::text_queue tq1 = test_connect.create_text_queue("AQADM.TEST1_QUEUE");
   jms::text_queue tq2 = test_connect.create_text_queue("AQADM.TEST2_QUEUE");
   jms::text_message first = tq1.dequeue();
   std::cout << "first message readed: " << first.text << std::endl;
   jms::text_message second;
   bool result = tq2.dequeue(second, 4, std::string(), first.text);
   std::cout << "second message reading result: " << result << " " << second.text << std::endl; 
   test_connect.commit();
   std::cout << "try to read next message" << std::endl;
   jms::text_message third = tq1.dequeue();
   std::cout << "third message readed: " << third.text << std::endl;
   test_connect.commit();
}

void read_from_multi()
{
   //jms::connection test_connect("test/test@aq_main");
   //jms::text_queue tq1 = test_connect.create_text_queue("AQADM.MULTITEST1");
   jms::connection test_connect("ego/ego@aq_main");
   jms::text_queue tq1 = test_connect.create_text_queue("AQADM.SHOP");
   jms::text_message first;
   if (tq1.dequeue(first, 5, "medved"))
   {
      std::cout << "first message readed from: " << first.properties.reply_to << " text: " << first.text << std::endl;
   }
   std::cout << "finished" << std::endl;
   test_connect.commit();
}

void amqp_dequeue_and_rollback()
{
   jms::connection test_connect("amqp://sms.tst.serv:VgGu8coyB@vv-bunny-02.sirena-travel.ru?QoS=10");
   jms::text_queue tq1 = test_connect.create_text_queue("sms.tst.tstqueue");
   jms::text_message msg;
   for (int i = 1; i <= 10 ; ++i) {
       std::cout << "Trying to dequeue " << i;
       if (tq1.dequeue(msg, 5)) {
           std::cout << "message text: " << msg.text << std::endl;
       }
       std::cout << "rollback start" << std::endl;
       test_connect.rollback();
       std::cout << "rollback finished" << std::endl;
   }
   std::cout << "finished" << std::endl;
}

int main(int agrc, char *argv[])
{
   //wait_connected_message();
   //read_from_multi();
   amqp_dequeue_and_rollback();
   return 0;
}
