#pragma once

#include "tlg/CheckinBaseTypes.h"

#include <string>


namespace Environment {

class Environ
{
public:
    static Ticketing::Airline_t airline();

    static Ticketing::Airline_t systemAirline();
    static std::string systemAirlineCode();
};

}//namespace Environment
