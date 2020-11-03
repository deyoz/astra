/*
*  C++ Implementation: RemFqtxElem
*
* Description: Ремарка FQTV/R/U в TypeB телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
*
*/

#include <serverlib/str_utils.h>
#include <serverlib/isdigit.h>

#include "RemFqtxElem.h"
#include "typeb_msg.h"
// #include ""

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

Remark * RemFqtxElem::parse(const std::string & code, const std::string & rem)
{
    std::string awk;

    LogTrace(TRACE3) << "RemFqtxElem::parse('" << code << "', '" << rem << "')";

    if(rem.length() < 5) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::INV_FQTx_FORMAT)  << code << " element too short";
    }

    std::size_t pos = rem.find(' ');
    if(pos != std::string::npos && pos < 4 && pos > 1)
    {
        awk = rem.substr(0, pos);
    }
    else
    {
        if((ISDIGIT(rem[2]) || ISSPACE(rem[2])))
            pos = 2;
        else
            pos = 3;
        awk = rem.substr(0,pos);
    }
    pos = StrUtils::lNonSpacePos(rem, pos);
    if(pos == rem.size()) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::INV_FQTx_FORMAT)  << code << " element too short";
    }

    std::size_t pos_end = rem.find('-', pos);
    LogTrace(TRACE3) << "pos=" << pos << "pos_end=" << pos_end;
    std::string fqtnum = rem.substr(pos, pos_end-pos);
    std::string free_text;

    if(pos_end != std::string::npos)
        free_text = rem.substr(pos_end+1);

    tst();
    return new RemFqtxElem(code, rem, awk, fqtnum, free_text);
}

} // namespace typeb_parser

