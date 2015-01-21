#ifndef _CHECKINBASETYPES_H_
#define _CHECKINBASETYPES_H_

#include <serverlib/int_parameters.h>
#include "exceptions.h"

namespace Ticketing
{
    MakeIntParamType(FlightNum_t, unsigned);
    MakeIntParamType(SystemAddrs_t, int);
    MakeIntParamType(EdifactProfile_t, int);
    MakeIntParamType(Point_t, int);

    FlightNum_t getFlightNum(const std::string &s);

} //namespace Ticketing

#endif /*_CHECKINBASETYPES_H_*/
