#include "CheckinBaseTypes.h"

#include <etick/exceptions.h>
#include <etick/tick_data.h>

#include <boost/lexical_cast.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing
{

FlightNum_t getFlightNum(const std::string &s)
{
    try {
        if(s != ItinStatus::Open)
            return FlightNum_t(boost::lexical_cast<int>(s));
        else
            return FlightNum_t();
    }
    catch(boost::bad_lexical_cast &e) {
        throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_FL_NUM");
    }
}

}//namespace Ticketing
