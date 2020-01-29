/*
*  C++ Implementation: ItinElem
*
* Description: Œ àèàãâ ¢ ETH á®®¡é¥­¨¨
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2007
*
*/
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "ItinElem.h"
#include "typeb_parse_exceptions.h"
#include "typeb_msg.h"
#include <serverlib/dates_io.h>
#include "etick/lang.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser {
using boost::bad_lexical_cast;
using namespace boost::gregorian;
using namespace boost;
using namespace boost::posix_time;

// -ITIN/U6123YDMESVX14MAR07ÿOK/0700
ItinElem * ItinElem::parse(const std::string & itin)
{
    regex expression("^([0-9A-Z€-Ÿð]{2,2}[A-Z€-Ÿð]?)(\\d{1,4})"//U6 123
                     "([A-Z]{1})([A-Z€-Ÿð]{3})([A-Z€-Ÿð]{3})" //Y DME SVX
                     "(\\d{2}[A-Z]{3}\\d{2})\\s*([A-Z]{2})/"//14MAR07
                     "(\\d{4})$");
    smatch what;
    if (!regex_match(itin, what, expression) || what.size() != 9)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << elemName();
    }

    ItinElem elem;
    elem.Airline = what[1];
    elem.FlightNum = lexical_cast<unsigned>(what[2]);
    elem.ClsLeter  = std::string(what[3])[0];
    elem.DepPoint  = what[4];
    elem.ArrPoint  = what[5];

    std::string date_time = what[6] + "/" + what[8];
    try {
        elem.DepDateTime = HelpCpp::time_cast(date_time.c_str(),"%d%b%y/%H%M", ENGLISH);
    }
    catch (std::ios_base::failure &e)
    {
        LogWarning(STDLOG) << e.what();
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_DATETIME_FORMAT_z1z) <<
                "DDMONYY/HHMM";
    }
    catch (std::out_of_range &e)
    {
        LogWarning(STDLOG) << e.what();
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_DATETIME_FORMAT_z1z) <<
                "DDMONYY/HHMM";
    }
    return new ItinElem(elem);
}

OperCarrierElem * OperCarrierElem::parse(const std::string & carr)
{
    regex expression("^([0-9A-Z€-Ÿð]{2,2}[A-Z€-Ÿð]?)(\\d{0,4})$");//U6 123
    smatch what;
    if (!regex_match(carr, what, expression) || what.size() != 3)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << elemName();
    }

    OperCarrierElem elem;
    elem.Airline = what[1];
    if(!what.str(2).empty())
    {
        elem.FlightNum = lexical_cast<unsigned>(what[2]);
    }
    else
    {
        elem.FlightNum = 0;
    }

    return new OperCarrierElem(elem);
}

std::ostream & operator <<(std::ostream & os, const ItinElem &itin)
{
    os << itin.name() << ": ";
    os << itin.airline() << "-" << itin.flightNum() << " ";
    if(itin.clsLeter())
    {
        os << itin.clsLeter() << " ";
    }
    os << itin.depPoint() << " " << itin.arrPoint() << " "
            << itin.depDateTime();

    return os;
}

std::ostream & operator <<(std::ostream & os, const OperCarrierElem & oper)
{
    os << oper.name() << ": " << oper.airline();
    if(oper.flightNum())
    {
        os << "-" << oper.flightNum();
    }

    return os;
}

}


