#pragma once

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace Dates {
    using namespace boost::gregorian;
    using namespace boost::posix_time;
    typedef boost::gregorian::date Date_t;
    typedef boost::posix_time::ptime DateTime_t;
}//namespace Dates
