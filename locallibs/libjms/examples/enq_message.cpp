#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "jms.hpp"
#include "text_message.hpp"


void message_from_cin()
{
    jms::connection test_connect("ego/ego@aq_main");
    jms::text_queue tq = test_connect.create_text_queue("AQADM.TEST1_QUEUE");

    jms::text_message mess;
    jms::recepients agents;
    while (!std::cin.eof())
    {
        char c = 0;
        std::cin.get(c);
        mess.text += c;
    }
    tq.enqueue(mess, agents);
    test_connect.commit();
    std::cout << "enqueued bytes: " << mess.text.size() << std::endl;
}

void send_connected_messages()
{
    //jms::connection test_connect("test/test@aq_main");
    //jms::text_queue tq1 = test_connect.create_text_queue("AQADM.MULTITEST1");
    //jms::text_queue tq2 = test_connect.create_text_queue("AQADM.MULTITEST1");
    jms::connection test_connect("ego/ego@aq_main");
    jms::text_queue tq1 = test_connect.create_text_queue("AQADM.SHOP");
    jms::text_queue tq2 = test_connect.create_text_queue("AQADM.SHOP");
    srand((unsigned)time(0));
    int value = random();
    std::stringstream s;
    s << value;
    jms::text_message first;
    jms::recepients agents;
    agents.push_back("medved");
    first.text = "hello world";
    first.properties.correlation_id = s.str();
    first.properties.reply_to = "preved";
    std::cout << "send message with correlation_id: " << first.properties.correlation_id << std::endl;
    tq2.enqueue(first, agents);
    //jms::text_message second;
    //second.text = s.str();
    //tq1.enqueue(second);
    std::cout << "second message has been sent" << std::endl;
    test_connect.commit(); 
}

int main(int argc, char *argv[])
{
    send_connected_messages();
    return 0;
}
