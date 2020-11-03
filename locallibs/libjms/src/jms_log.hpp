#ifndef JMS_LOG_H
#define JMS_LOG_H

#include <ostream>
#include <iomanip>

namespace jms_null_logger
{
class null_stream {};
template <typename T>
inline null_stream & operator<<(null_stream & s, T const &) {return s;}
//manip
inline null_stream & operator<<(null_stream & s, std::ostream &(std::ostream&)) {return s;}
extern null_stream lout;
}

#if defined JMS_STDERR_LOGGING

#include <iostream>
#define LERR std::cerr << "\n"
#define LDEBUG std::cerr << "\n"
#define LDEBUG2 std::cerr << "\n"
#define LINFO std::cerr << "\n"
#define LWARN std::cerr << "\n"
#elif defined HAVE_LIBLOG

#include <log/log.hpp>

#define LINFO LINFO_EX("libjms")
#define LDEBUG LDEBUG_EX("libjms")
#define LDEBUG2 LDEBUG_EX("libjms_net")
#define LERR LERR_EX("libjms")
#define LWARN LWARN_EX("libjms")

#elif defined JMS_SERVERLIB_LOGGING

#define _NO_TEST_H_ 1
#include <serverlib/slogger.h>

#define LERR LogError("LIBJMS",__FILE__,__LINE__)
#define LDEBUG LogTrace(12,"LIBJMS",__FILE__,__LINE__)
#define LDEBUG2 LogTrace(12,"LIBJMS_NET",__FILE__,__LINE__)
#define LINFO LogTrace(12,"LIBJMS",__FILE__,__LINE__)
#define LWARN LogTrace(1,"LIBJMS",__FILE__,__LINE__) 

#else
#define LERR jms_null_logger::lout
#define LDEBUG jms_null_logger::lout
#define LDEBUG2 jms_null_logger::lout
#define LINFO jms_null_logger::lout
#define LWARN jms_null_logger::lout


#endif 


#endif // JMS_LOG_H
