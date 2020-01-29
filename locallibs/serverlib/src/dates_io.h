#ifndef _SERVERLIB_DATES_IO_H_
#define _SERVERLIB_DATES_IO_H_
#include <string>
#include "dates.h"

namespace HelpCpp {

std::string string_cast(const boost::gregorian::date&, char const *format, int l = -1);
std::string string_cast(const boost::posix_time::ptime&, char const *format, int l = -1);
std::string string_cast(const boost::posix_time::time_duration&, char const *format, int l = -1);

// functions that throws
boost::gregorian::date date_cast(const char *data, const char *format="", int l=-1, bool strict=true);
boost::posix_time::ptime time_cast(const char *data, const char *format="", int l=-1, bool strict=true);
boost::posix_time::time_duration timed_cast(const char *data, const char *format="", int l=-1, bool strict=true);

//functions that not throws
boost::gregorian::date simple_date_cast(const char *data, const char *format="", int l=-1);
boost::posix_time::ptime simple_time_cast(const char *data, const char *format="", int l=-1);
boost::posix_time::time_duration simple_timed_cast(const char *data, const char *format="", int l=-1);

std::string date6to8(std::string const &six);

} // namespace HelpCpp




#endif //_SERVERLIB_DATES_IO_H_
