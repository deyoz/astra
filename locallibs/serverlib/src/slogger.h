//
// C++ Interface: slogger
//
// Description: Потоковый логгировщег
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _NO_TEST_H_
#define PROHIBIT_UNDEFINED_NICKNAME
#ifndef NICKNAME
#ifdef PROHIBIT_UNDEFINED_NICKNAME
#error "NICKNAME undefined. DO NOT WRITE CODE IN HEADERS!"
#endif
#define NICKNAME "DO_NOT_WRITE_CODE_IN_HEADERS"
#endif

#include "test.h"
#endif //_NO_TEST_H_

#ifndef _SLOGGER_H_
#define _SLOGGER_H_
#ifdef __cplusplus
#include <sstream>

namespace boost {  namespace posix_time {  class ptime;  class time_duration;  }  }
namespace boost {  namespace gregorian {  class date;  }  }

namespace stream_logger
{

class LogStream
{
public:
    LogStream(std::ostringstream& s) : stream(s) {}
    std::ostringstream& guts() & { return stream; }

    template<typename T, typename = decltype(std::declval<std::ostream&>() << std::declval<const T&>())>
    LogStream& operator<<(const T& v) {
        stream << v;
        return *this;
    }
private:
    std::ostringstream& stream;
};


namespace details
{

template<typename T, typename = decltype(std::declval<LogStream&>() << std::declval<const T&>())>
void chooseLogStream(std::ostringstream& s, const T& v) {
    LogStream l(s);
    l << v;
}

template<typename T, typename... Ignored>
void chooseLogStream(std::ostringstream& s, const T& v, Ignored const&...) {
    s << v;
}
} // details

class StreamLogger {
    int Level;
    const char *Nick;
    const char *File;
    unsigned Line;
    bool CutLog;

    std::ostringstream* Stream;
    void flush();
protected:
    StreamLogger(const StreamLogger&);
    friend StreamLogger LogTrace(int level, const char *nick, const char *file, unsigned line);
    friend StreamLogger LogWarning(const char *nick, const char *file, unsigned line);
    friend StreamLogger LogError(const char *nick, const char *file, unsigned line);

    StreamLogger(int level, const char *nick, const char *file, unsigned line);
public:

    StreamLogger & operator<< (char* v) { return this->operator<< (const_cast<char const*>(v)); }
    StreamLogger & operator<< (char const* v);
    StreamLogger & operator<< (const boost::gregorian::date& );
    StreamLogger & operator<< (const boost::posix_time::ptime& );
    StreamLogger & operator<< (const boost::posix_time::time_duration& );
    StreamLogger & operator<< (std::basic_ostream<char>& (*func)(std::basic_ostream<char>&));
    StreamLogger & operator<< (std::ios_base& (*func)(std::ios_base&));
    StreamLogger & operator<< (std::basic_ios<char>& (*func)(std::basic_ios<char>&));

    std::ostringstream &stream();

    static bool writeCallStack(int = -2);
    template < class T > StreamLogger & operator << (const T &t)
    {
        if (!CutLog || writeCallStack()) {
            details::chooseLogStream(stream(), t);
        }

        return *this;
    }

    bool cutLog() {  return !CutLog; }

    StreamLogger & setf(std::ios::fmtflags fmtfl);
    StreamLogger & setf(std::ios::fmtflags fmtfl, std::ios::fmtflags mask);

    ~StreamLogger();
};

inline StreamLogger LogTrace(int level, const char *nick, const char *file, unsigned line)
{
    return StreamLogger(level, nick, file, line);
}
inline StreamLogger LogWarning(const char *nick, const char *file, unsigned line)
{
    return StreamLogger(0, nick, file, line);
}
inline StreamLogger LogError(const char *nick, const char *file, unsigned line)
{
    return StreamLogger(-1, nick, file, line);
}

namespace details
{
struct ContainerElemToOstream
{
    template<typename Stream, typename T>
    void operator()(Stream& os, const T& t) const {
        os << t;
    }
};
} // details

template<typename Cont, typename ToStreamer>
struct LogCont__
{
    LogCont__(const char* d, const Cont& c, ToStreamer s)
        : delim(d), ref(c), toStreamer(s)
    {}

    template<typename Stream, typename T>
    void toStream(Stream& s, const T& v) const {
        toStreamer(s, v);
    }

    const char* delim;
    const Cont& ref;
    ToStreamer toStreamer;
};

namespace details
{
template<typename Cont, typename ToStreamer>
auto createLogCont(const char* d, const Cont& c, ToStreamer s) {
    return LogCont__<Cont, ToStreamer>(d, c, s);
}

template<typename Stream, typename LogCont>
void toStream(Stream& s, const LogCont& logCont)
{
    bool needDelim = false;
    for (const auto& v : logCont.ref) {
        if (needDelim) {
            s << logCont.delim;
        } else {
            needDelim = true;
        }
        logCont.toStream(s, v);
    }
}

} // details

template<typename Cont, typename ToStreamer>
inline std::ostream& operator<<(std::ostream& s, const LogCont__<Cont, ToStreamer>& vs)
{
    details::toStream(s, vs);
    return s;
}

template<typename Cont, typename ToStreamer>
inline LogStream& operator<<(LogStream& s, const LogCont__<Cont, ToStreamer>& vs)
{
    details::toStream(s, vs);
    return s;
}

template<typename Cont>
inline auto LogCont(const char* delim, const Cont& c)
{
    return details::createLogCont(delim, c, details::ContainerElemToOstream());
}

template<typename Cont, typename ToStreamer>
inline auto LogCont(const char* delim, const Cont& c, ToStreamer f)
{
    return details::createLogCont(delim, c, f);
}

} // stream_logger

using stream_logger::LogStream;
using stream_logger::StreamLogger;
using stream_logger::LogTrace;
using stream_logger::LogWarning;
using stream_logger::LogError;
using stream_logger::LogCont;

#endif /* __cplusplus */
#endif /*_SLOGGER_H_*/
