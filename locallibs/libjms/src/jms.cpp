#if HAVE_CONFIG_H
#endif /* HAVE_CONFIG_H */

#include "jms.hpp"

#ifdef HAVE_ORACLE_AQ
#include "aq.hpp"
#endif // HAVE_ORACLE_AQ

#ifdef HAVE_ACTIVEMQ
#include "amq.hpp"
#endif // HAVE_ACTIVEMQ

#ifdef HAVE_AMQP_CPP
#include "amqp.hpp"
#endif //HAVE_AMQP_CPP

#include <boost/algorithm/string/trim.hpp>
#include "jms_log.hpp"

namespace jms_null_logger
{
null_stream lout;
}


namespace jms
{

connection::connection(const std::string& connect_str, bool trace)
{
   std::string connect_string = boost::trim_copy(connect_str);
   if (connect_string.substr(0,7) == "amqp://" || connect_string.substr(0,8) == "amqps://" )
   {
#ifdef HAVE_AMQP_CPP
      impl_ = std::shared_ptr<detail::connection>(new amqp::connection(connect_string));
#else // HAVE_AMQP_CPP
      throw jms::mq_error(-1, "Built without AMQP support");
#endif // HAVE_AMQP_CPP
   }
   else if (connect_string.find("oracle://") == 0 ||  connect_string.find("://") == std::string::npos)
   {
#ifdef HAVE_ORACLE_AQ
      impl_ = std::shared_ptr<detail::connection>(new AQ::connection(connect_string, trace));
#else // HAVE_ORACLE_AQ
      throw jms::mq_error(-1, "Built without Oracle AQ support");
#endif // HAVE_ORACLE_AQ
   }
   else
   {
#ifdef HAVE_ACTIVEMQ
       impl_ = std::shared_ptr<detail::connection>(new AMQ::amqconnection(connect_string));
#else // HAVE_ACTIVEMQ
       throw jms::mq_error(-1, "Built without ActiveMQ support");
#endif // HAVE_ACTIVEMQ
   }
}

bool connection::has_internal_recode() const
{
    return impl_->has_internal_recode();
}


connection::~connection()
{
}

text_queue connection::create_text_queue(const std::string& name, bool xact)
{
    return text_queue(name, impl_->create_text_queue(name, xact));
}

std::string connection::listen(const std::vector<std::string>& names)
{
   return impl_->listen(names);
}

void connection::check()
{
   impl_->check();
}

void connection::commit()
{
    impl_->commit();
}

void connection::rollback()
{
    impl_->rollback();
}

void connection::break_wait()
{
   impl_->break_wait();
}

text_queue::text_queue(const std::string& name, const std::shared_ptr<detail::text_queue>& impl)
    : impl_(impl), m_name(name)
{
}

text_queue::~text_queue()
{
}

text_message text_queue::dequeue(const recepient& agent, const std::string& corr_id_mask)
{
   return impl_->dequeue(agent, corr_id_mask);
}

bool text_queue::dequeue(text_message& msg, unsigned wait_delay, const recepient& agent, const std::string& corr_id_mask)
{
   return impl_->dequeue(msg, wait_delay, agent, corr_id_mask);
}

void text_queue::enqueue(const text_message& msg, const recepients& agents)
{
   impl_->enqueue(msg, agents);
}

} // namespace jms

