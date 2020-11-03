/*
*  C++ Implementation: SeatConfigElem
*
* Description: SeatConfig в ETL телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "SeatConfigElem.h"
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"
#include <serverlib/isdigit.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;
using namespace boost;

// CFG/008C060Y/TU3
SeatConfigElem * SeatConfigElem::parse (const std::string &txt)
{
    std::string FlType;
    std::list<Total2Class> Totals;

    LogTrace(TRACE3) << __FUNCTION__ ;

    if(!regex_match(txt,
        regex("^([0-9]{3}[A-ZА-ЯЁ]{1}){1,}(/[A-ZА-ЯЁ0-9]{3}){0,1}$"), match_any))
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
                "Seat configuration element";
    }

    string::size_type pos = txt.find('/');
    if(pos!=string::npos)
    {
        FlType = txt.substr(pos+1);
    }

    string totals_str = txt.substr(0,pos);
    // Надеемся что регексом все проверено
    pos=0;
    while(pos < totals_str.size())
    {
        string::size_type pos2=pos;
        while(pos2<totals_str.size() && ISDIGIT(totals_str[pos2])) pos2++;
        unsigned nseats = lexical_cast<unsigned>(totals_str.substr(pos, pos2-pos));
        unsigned char rbd = totals_str[pos2];
        LogTrace(TRACE3) << "nseats:" << nseats << " rbd:" << rbd;
        Totals.push_back(Total2Class(nseats,rbd));
        pos=pos2+1;
    }
    return new SeatConfigElem(Totals, FlType);
}

} //namespace typeb_parser
