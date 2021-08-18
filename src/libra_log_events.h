#pragma once

#include "astra_dates.h"

#include <string>
#include <list>


namespace LIBRA
{

struct LogEvent
{
    std::string       Airline;
    std::string       BortNum;
    std::string       Category;
    int               PointId;
    std::string       RusMsg;
    std::string       LatMsg;
    std::string       LogType;
    Dates::DateTime_t LogTime;
    std::string       EvUser;
    std::string       EvStation;

    bool isAhm() const { return LogType == "AHM"; }
};

std::ostream& operator<<(std::ostream& os, const LogEvent& e);

}//namespace LIBRA
