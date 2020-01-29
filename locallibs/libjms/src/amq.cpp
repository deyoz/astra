#ifdef HAVE_ACTIVEMQ
#include "amq.hpp"
#include "errors.hpp"

#include <cms/CMSException.h>
#include <cms/Message.h>
#include <cms/TextMessage.h>
#include <cms/Topic.h>

#include <activemq/core/ActiveMQConnectionFactory.h>
#include "jms_log.hpp"

namespace AMQ
{

int AMQLibrary::conns = 0;

amqconnection::amqconnection(const std::string& brokerURI)
{
   try
   {
      activemq::core::ActiveMQConnectionFactory connectionFactory(brokerURI);
      connection_.reset(connectionFactory.createConnection());
      session_.reset(connection_->createSession(cms::Session::AUTO_ACKNOWLEDGE));
      connection_->start();
   }
   catch (const cms::CMSException& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.getMessage());
   }
}

void amqconnection::check()
{
   // do smth
}

void amqconnection::commit()
{
   session_->commit();
}

void amqconnection::rollback()
{
   session_->rollback();
}

void amqconnection::break_wait()
{
   session_->recover();
}

std::string amqconnection::listen(const std::vector<std::string>& queue_names)
{
   // do smth   
   return std::string();
}

std::shared_ptr<detail::text_queue> amqconnection::create_text_queue(const std::string& name, bool xact)
{   
   return std::shared_ptr<detail::text_queue>(new amq_queue(*session_, name));
}

amq_queue::amq_queue(cms::Session& session, const std::string& name)
: session_(session), name_(name)
{
   try
   {
      std::shared_ptr<cms::Topic> topic(session_.createTopic(name_)); // use topic for broadcast
      consumer_.reset(session_.createConsumer(topic.get()));
      producer_.reset(session_.createProducer(topic.get()));
      producer_->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
   }
   catch (const cms::CMSException& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.getMessage());
   }
}

void amq_queue::enqueue(const jms::text_message& msg, const jms::recepients& agents)
{
   try
   {
      std::shared_ptr<cms::TextMessage> message(session_.createTextMessage(msg.text));
      message->setCMSCorrelationID(msg.properties.correlation_id);
      message->setCMSExpiration(msg.properties.expiration);
      if (!msg.properties.reply_to.empty())
      {
         std::shared_ptr<cms::Topic> qreply_to(session_.createTopic(msg.properties.reply_to));
         message->setCMSReplyTo(qreply_to.get());
      }

      if (!agents.empty())
      {
         for (jms::recepients::const_iterator pos = agents.begin(); pos != agents.end(); ++pos)
         {
            std::shared_ptr<cms::MessageProducer> producer(session_.createProducer(session_.createTopic(*pos)));
            producer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
            producer->send(message.get());
         }
      }
      else
      {
         producer_->send(message.get());
      }
   }
   catch (const cms::CMSException& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.getMessage());
   }
}

bool amq_queue::dequeue(jms::text_message& msg, unsigned wait_delay, const jms::recepient& agent, const std::string& corr_id_mask)
{
   bool result = false;
   try
   {
      std::shared_ptr<cms::MessageConsumer> consumer;
      if (!agent.empty())
      {
         consumer.reset(session_.createConsumer(session_.createTopic(agent)));
      }
      else
      {
         consumer = consumer_;
      }

      std::shared_ptr<cms::Message> message;
      if (wait_delay)
      {
         message.reset(consumer->receive(wait_delay));
      }
      else
      {
         message.reset(consumer->receive());
      }
      if (message)
      {
         const cms::TextMessage* txtMsg = dynamic_cast<const cms::TextMessage*>(message.get());
         if (txtMsg != 0)
         {
            msg.text = txtMsg->getText();
            msg.properties.correlation_id = txtMsg->getCMSCorrelationID();
            msg.properties.expiration = txtMsg->getCMSExpiration();
            if (txtMsg->getCMSReplyTo())
            {
               const cms::Topic* topic = dynamic_cast<const cms::Topic*>(txtMsg->getCMSReplyTo());
               if (topic)
               {
                  msg.properties.reply_to = topic->getTopicName();
               }
            }
         }
         else
         {
            LWARN << "Unexpected message type.";
         }
         result = true;
      }
      else
      {
         result = false;
      }
   }
   catch (const cms::CMSException& e)
   {
      LERR << e.what();
      throw jms::mq_error(-1, e.getMessage());
   }
   return result;
}

jms::text_message amq_queue::dequeue(const jms::recepient& agent, const std::string& corr_id_mask)
{
   jms::text_message msg;
   if (!dequeue(msg, 0, agent))
   {
      throw jms::mq_error(-1, "dequeue_failed");
   }
   return msg;
}

} //namespace AMQ

#endif // HAVE_ACTIVEMQ
