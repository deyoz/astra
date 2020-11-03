/*
*  C++ Implementation: AutomPNRAddr
*
* Description: PNR element
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
*
*/
#include <boost/regex.hpp>

#include "AutomPNRAddr.h"
#include "typeb_parse_exceptions.h"
#include "typeb_msg.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace boost;

// .L/06Çîêç/èé
AutomPNRAddr * AutomPNRAddr::parse(const std::string & pnrstr)
{
    regex expression("^([0-9A-ZÄ-ü]{2,12})/?([0-9A-ZÄ-ü]{2,2}[A-ZÄ-ü]?)?$");
    smatch what;
    if (!regex_match(pnrstr, what, expression) || what.size() < 2)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << elemName();
    }

    AutomPNRAddr elem;
    elem.Recloc  = what[1];
    if(what.size() > 2)
        elem.Airline = what[2];

    return new AutomPNRAddr(elem);
}

std::ostream & operator << (std::ostream& os, const AutomPNRAddr &pnr)
{
    os << pnr.recloc() << "/" << pnr.airline();
    return os;
}

}
