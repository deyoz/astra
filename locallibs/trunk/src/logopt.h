#ifndef SERVERLIB_LOGOPT_H
#define SERVERLIB_LOGOPT_H

#include <iosfwd>
#include <boost/optional.hpp>

namespace stream_logger
{

class LogStream;

template<typename T>
struct LogOpt__
{
    LogOpt__(const T* op_, const char* p_, const char* n_)
        : ref(op_), prefix(p_), none(n_)
    {}

    const T* ref;
    const char* prefix;
    const char* none;
};

template<typename T>
LogOpt__<T> LogOpt(const char* pref, const boost::optional<T>& opt, const char* none)
{
    return LogOpt__<T>(opt.get_ptr(), pref, none);
}

template<typename T>
LogOpt__<T> LogOpt(const char* pref, T* opt, const char* none)
{
    return LogOpt__<T>(opt, pref, none);
}

template<typename T>
LogOpt__<T> LogOpt(const char* pref, const boost::optional<T>& opt)
{
    return LogOpt__<T>(opt.get_ptr(), pref, "");
}

template<typename T>
LogOpt__<T> LogOpt(const char* pref, T* opt)
{
    return LogOpt__<T>(opt, pref, "");
}

template<typename T>
LogOpt__<T> LogOpt(const boost::optional<T>& opt, const char* none)
{
    return LogOpt__<T>(opt.get_ptr(), "", none);
}

template<typename T>
LogOpt__<T> LogOpt(T* opt, const char* none)
{
    return LogOpt__<T>(opt, "", none);
}

template<typename T>
LogOpt__<T> LogOpt(const boost::optional<T>& opt)
{
    return LogOpt__<T>(opt.get_ptr(), "", "");
}

template<typename T>
LogOpt__<T> LogOpt(const T* opt)
{
    return LogOpt__<T>(opt, "", "");
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const LogOpt__<T>& lo)
{
    return static_cast<bool>(lo.ref)
        ? (os << lo.prefix << *lo.ref)
        : (os << lo.none);
}

template<typename T>
LogStream& operator<<(LogStream& os, const LogOpt__<T>& lo)
{
    return static_cast<bool>(lo.ref)
        ? (os << lo.prefix << *lo.ref)
        : (os << lo.none);
}

inline LogOpt__<bool> LogOpt(const char* name, bool v)
{
    return LogOpt__<bool>(nullptr, "", (v ? name : ""));
}
} // stream_logger

using stream_logger::LogOpt;
#endif /* SERVERLIB_LOGOPT_H */
