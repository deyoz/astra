/*
*  C++ Implementation: FlightElem
*
* Description: FlightElem ¢ ETL â¥«¥£à ¬¬¥
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <vector>

#include "FreqTrElem.h"
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"
#include <serverlib/isdigit.h>
#include <serverlib/dates.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using std::string;
using boost::bad_lexical_cast;
using namespace boost;
// .F/KL 123456789
FreqTrElem * FreqTrElem::parse (const std::string &txt)
{
    regex expression("^([0-9A-Z€-Ÿð]{2,2}[A-Z€-Ÿð]?)[[:blank:]]+([0-9A-Z€-Ÿð]{1,25})"/*U6 1232323232323*/);
    smatch what;
    if (!regex_match(txt, what, expression) || what.size() != 3)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) << elemName();
    }

    return new FreqTrElem(what[1], what[2]);
}

} // typeb_parser
