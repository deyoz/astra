#ifdef BUILD_AGAINST_PYTHON

#include <string>
#include <boost/python.hpp>
#include <boost/python/list.hpp>

#include <jms.hpp>

#ifdef HAVE_LIBLOG 

#include <log/log.hpp>
#define LINFO LINFO_EX("io_layer")
#define LERR LERR_EX("io_layer")
#define LDEBUG LDEBUG_EX("io_layer")

#elif defined JMS_SERVERLIB_LOGGING

#define _NO_TEST_H_ 1
#include <serverlib/slogger.h>
#define LINFO LogTrace(12,"IO_LAYER",__FILE__,__LINE__)
#define LDEBUG LogTrace(1,"IO_LAYER",__FILE__,__LINE__)
#define LERR LogTrace(1,"IO_LAYER",__FILE__,__LINE__)

#else

#define LINFO std::stringstream()
#define LERR std::stringstream()
#define LDEBUG std::stringstream()

#endif


using namespace boost::python;
//void bind_datetime();

namespace 
{

std::string connect_string;

void init_module(const std::string& connect_init, const std::string& log_init)
{
#ifdef HAVE_LIBLOG 
   init_logs("jmspy", log_init);
#endif
   connect_string = connect_init;   
   //bind_datetime();
}

void enqueue(const std::string& queue_name, const std::string& text)
{
   jms::connection mq_session(connect_string);
   jms::text_queue queue = mq_session.create_text_queue(queue_name);
   jms::text_message message;
   jms::recepients agents;
   message.text = text;
   LINFO << "*******  Put to " << queue_name << " queue ********";
   LINFO << "text: " << message.text;
   LINFO << "text_size: " << message.text.size();
   queue.enqueue(message, agents);
   mq_session.commit();
}

std::string dequeue(const std::string& queue_name)
{
   jms::connection mq_session(connect_string);
   jms::text_queue queue = mq_session.create_text_queue(queue_name);
   jms::text_message message = queue.dequeue();
   LINFO << "*******  Receive from " << queue_name << " queue ********";
   LINFO << "text: " << message.text;
   LINFO << "text_size: " << message.text.size();
   mq_session.commit();
   return message.text;
}

}

BOOST_PYTHON_MODULE(jmspy)
{
   def("init_module", init_module);
   def("dequeue", dequeue);
   def("enqueue", enqueue);
}
#endif // BUILD_AGAINST_PYTHON
