#pragma once

#include <libnsi/nsi.h>

namespace boost { namespace posix_time { class ptime; } }

namespace nsi {

std::string centerTimeZone();

boost::posix_time::ptime timeGmtToCity(const boost::posix_time::ptime&, nsi::CityId);
boost::posix_time::ptime timeGmtToCity(const boost::posix_time::ptime&, nsi::PointId);

boost::posix_time::ptime timeCityToGmt(const boost::posix_time::ptime&, nsi::CityId);
boost::posix_time::ptime timeCityToGmt(const boost::posix_time::ptime&, nsi::PointId);

boost::posix_time::ptime timeGmtToCenter(const boost::posix_time::ptime&);
boost::posix_time::ptime timeCenterToGmt(const boost::posix_time::ptime&);

boost::posix_time::ptime timeCityToCenter(const boost::posix_time::ptime&, nsi::CityId);
boost::posix_time::ptime timeCityToCenter(const boost::posix_time::ptime&, nsi::PointId);

boost::posix_time::ptime timeCenterToCity(const boost::posix_time::ptime&, nsi::CityId);
boost::posix_time::ptime timeCenterToCity(const boost::posix_time::ptime&, nsi::PointId);

} //nsi
