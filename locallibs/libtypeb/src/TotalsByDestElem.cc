//
// C++ Interface: TotalsByDestElem
//
// Description: Описание TotalsByDestElem ETL сообщения
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "TotalsByDestElem.h"
#include "typeb_parse_exceptions.h"
#include "typeb_msg.h"
#include <serverlib/str_utils.h>
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser{
using namespace boost;
using namespace std;

//DME008Y
TotalsByDestElem * TotalsByDestElem::parse(const std::string & txt_)
{
    LogTrace(TRACE3) << typeid(TotalsByDestElem).name() ;

    std::string txt = StrUtils::delSpaces(txt_);

    if(!regex_match(txt,
        regex("^[A-ZА-Я]{3,3}[0-9]{1,3}[A-ZА-Я]{1,1}$"), match_any))
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
                "Totals by destination element";
    }

    string dest = txt.substr(0,3);
    string npass_str = txt.substr(3, txt.length()-1-3);
    std::string Class = txt.substr(txt.length()-1,1);
    unsigned npass;
    try{
//         LogTrace(TRACE3) << "npass: " << npass_str;
        npass = lexical_cast<unsigned>(npass_str);
//         LogTrace(TRACE3) << "npass: " << npass;
    }
    catch(bad_lexical_cast &e)
    {
        throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z_FIELD_z2z, e.what()) <<
                "Totals by destination element" << "Total number of passengers";
    }

    return new TotalsByDestElem(dest, npass, Class);
}

} // namespace typeb_parser
