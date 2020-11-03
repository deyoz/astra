/*
*  C++ Implementation: remark_element
*
* Description: Ремарка TKNE в TypeB телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
/// @todo There is may be -1SURNAME/NAME at end of TKNE remark

#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "RemTkneElem.h"
#include "typeb_parse_exceptions.h"
#include "typeb_msg.h"
#include "typeb_template.h"
#include <serverlib/str_utils.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
using namespace std;

Remark * RemTkneElem::parse(const std::string &code, const std::string & rem_)
{
    size_t curr=0;

    std::string rem = StrUtils::delSpaces(rem_);

    if(rem.length() < 10)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_TICKET_NUM, "Invalid remark length");
    }

    if(rem.substr(0, 3) == "HK1")
        curr = 3;

    /*INF123456789012/1*/
    bool inf = false;
    if(rem.substr(curr, 3) == "INF")
    {
        inf = true;
        LogTrace(TRACE3) << "Infant";
        curr += 3;
    }

    string::size_type couppos = rem.find('/', curr);
    if(couppos == string::npos)
    {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_COUPON_NUM, "No coupon");
    }

    string tnum = rem.substr(curr, couppos - curr);

    if (!boost::regex_match(tnum,
         boost::regex("^[0-9A-ZА-ЯЁ]{3}[0-9]{10}$"), boost::match_any))
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_TICKET_NUM,
                                 "Bad length or has alpha: " + tnum);
    }

    unsigned cnum;
    try{
        cnum = boost::lexical_cast<unsigned> (rem.substr(couppos+1));
    }
    catch(boost::bad_lexical_cast &e)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_COUPON_NUM,
                                 "Non numeric");
    }

    if(cnum == 0 || cnum > 4)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_COUPON_NUM,
                                 string("Bad num for coupon: ")+
                                         rem.substr(couppos+1));
    }

    return new RemTkneElem(rem, tnum, cnum, inf);
}

std::ostream & operator <<(std::ostream & os, const RemTkneElem & tkne)
{
    os << tkne.templ()->accessName() << ": ";
    os << tkne.code() << " " << tkne.ticknum() << "/" << tkne.coupon() << " ";
    os << "INF:" << tkne.infant();

    return os;
}

} // namespace typeb_parser

