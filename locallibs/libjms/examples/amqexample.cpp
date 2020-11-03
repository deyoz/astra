#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include "jms.hpp"
#include <unistd.h>

int main(int argc, char* argv[])
{
    std::string brokerURI =
        "failover://(tcp://127.0.0.1:61616"
         "?connection.Username=admin"
         "&connection.Password=admin"
///       "?wireFormat=openwire"
//        "&connection.useAsyncSend=true"
//        "&transport.commandTracingEnabled=true"
//        "&transport.tcpTracingEnabled=true"
//        "&wireFormat.tightEncodingEnabled=true"
        ")";


    jms::connection cl(brokerURI);
    //jms::amq_queue q = cl.create_text_queue("TEST");
    //jms::text_message msg = q.dequeue();
    //std::cout << msg.text << std::endl; 
    jms::text_queue qping = cl.create_text_queue("PING");
    jms::text_queue qpong = cl.create_text_queue("PONG");
    jms::text_message msg, ping, pong;
    ping.text = "PING";
    pong.text = "PONG";
    jms::recepients agents;
    agents.push_back("PING");
    agents.push_back("PONG");
    std::cout << "Start game:" << std::endl;
    bool first = true;
    int count = 0;
    while (count < 5)
    {
       if (first)
       {
          first = false;
          qping.enqueue(ping, agents);
       }
       if (qpong.dequeue(msg, 5))
       {
          if (msg.text == "PING")
          {
             std::cout << msg.text << std::endl;
             qping.enqueue(pong, agents);
          }
       }
       if (qping.dequeue(msg, 5))
       {
          if (msg.text == "PONG")
          {
             std::cout << msg.text << std::endl;
             qping.enqueue(ping, agents);
          }
       }
       sleep(1);
       ++count;
    }
    return 0;
}
