#ifdef HAVE_ACTIVEMQ
#ifndef __AMQ_H__
#define __AMQ_H__

#include "detail.hpp"
#include "text_message.hpp"

#include <memory>

#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/MessageConsumer.h>
#include <cms/MessageProducer.h>

#include <activemq/library/ActiveMQCPP.h>

namespace AMQ
{

class AMQLibrary
{
public:
   AMQLibrary() 
   {
      if (!conns)
      {  
         activemq::library::ActiveMQCPP::initializeLibrary();
      }
      conns++; 
   }
   ~AMQLibrary() 
   {
      conns--; 
      if (!conns)
      {
         activemq::library::ActiveMQCPP::shutdownLibrary(); 
      }
   }
private:
   static int conns;  
};

class amqconnection : public detail::connection, private AMQLibrary
{
public:
    amqconnection(const std::string&);
    virtual void check() override;
    virtual void commit() override;
    virtual void rollback() override;
    virtual void break_wait() override;
    virtual std::string listen(const std::vector<std::string>&) override;
    virtual std::shared_ptr<detail::text_queue> create_text_queue(const std::string& , bool xact=true) override;
    virtual ~amqconnection() {}
private:
    amqconnection(const amqconnection&);
    amqconnection& operator=(const amqconnection&);
private:
    std::shared_ptr<cms::Connection> connection_;
    std::shared_ptr<cms::Session> session_;
};

class amq_queue : public detail::text_queue
{
friend class amqconnection;
public:
    jms::text_message dequeue(const jms::recepient&, const std::string& = "") override;
    bool dequeue(jms::text_message& msg, unsigned wait_delay = 0,
                 const jms::recepient& = jms::recepient(), const std::string& = "") override;
    void enqueue(const jms::text_message&, const jms::recepients&) override;
private:
   amq_queue(cms::Session&, const std::string&);
private:
   std::string name_;
   cms::Session& session_;
   std::shared_ptr<cms::MessageConsumer> consumer_;
   std::shared_ptr<cms::MessageProducer> producer_;
};

}// namespace AMQ

#endif // __AMQ_H__
#endif // HAVE_ACTIVEMQ
