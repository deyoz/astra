#ifndef _CHECKINBASETYPES_H_
#define _CHECKINBASETYPES_H_

#include "serverlib/int_parameters.h"

namespace ASTRA
{
    MakeIntParamType(Airline_t, std::string);
    MakeIntParamType(FlightNum_t, unsigned);
    MakeIntParamType(RouterId_t, std::string);
    MakeIntParamType(SystemAddrs_t, int);
    MakeIntParamType(EdifactProfile_t, int);
} //namespace ASTRA

#endif /*_CHECKINBASETYPES_H_*/
