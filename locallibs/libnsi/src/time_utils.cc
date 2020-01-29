#include <boost/date_time/posix_time/ptime.hpp>
#include <serverlib/localtime.h>

#include "nsi.h"
#include "callbacks.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace nsi {

std::string centerTimeZone()
{
    return nsi::City(nsi::callbacks().centerCity()).timezone();
}

boost::posix_time::ptime timeGmtToCity(const boost::posix_time::ptime& t, nsi::CityId id)
{
    return LocalTime().setGMTime(t).setZone(nsi::City(id).timezone()).getBoostLocalTime()
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeGmtToCity(const boost::posix_time::ptime& t, nsi::PointId id)
{
    return timeGmtToCity(t, nsi::Point(id).cityId());
}

boost::posix_time::ptime timeCityToGmt(const boost::posix_time::ptime& t, nsi::CityId id)
{
    return LocalTime(t, nsi::City(id).timezone()).getBoostGMTime()
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeCityToGmt(const boost::posix_time::ptime& t, nsi::PointId id)
{
    return timeCityToGmt(t, nsi::Point(id).cityId());
}

boost::posix_time::ptime timeGmtToCenter(const boost::posix_time::ptime& t)
{
    return LocalTime().setGMTime(t).setZone(centerTimeZone()).getBoostLocalTime()
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeCenterToGmt(const boost::posix_time::ptime& t)
{
    return LocalTime(t, centerTimeZone()).getBoostGMTime()
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeCityToCenter(const boost::posix_time::ptime& t, nsi::CityId id)
{
    return City2City(t, nsi::City(id).timezone(), centerTimeZone())
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeCityToCenter(const boost::posix_time::ptime& t, nsi::PointId id)
{
    return timeCityToCenter(t, nsi::Point(id).cityId());
}

boost::posix_time::ptime timeCenterToCity(const boost::posix_time::ptime& t, nsi::CityId id)
{
    return City2City(t, centerTimeZone(), nsi::City(id).timezone())
        + boost::posix_time::time_duration(0, 0, 0, t.time_of_day().fractional_seconds());
}

boost::posix_time::ptime timeCenterToCity(const boost::posix_time::ptime& t, nsi::PointId id)
{
    return timeCenterToCity(t, nsi::Point(id).cityId());
}

} //nsi
