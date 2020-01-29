/*
*  C++ Implementation: ssr_tknx
*
* Description: SSR TKNX parser
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <boost/shared_ptr.hpp>
#include "ssr_tknx.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>
#include <serverlib/str_utils.h>
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"

namespace typeb_parser
{
using namespace boost;
using namespace std;
using namespace Ticketing;
using namespace Ticketing::TickStatAction;

Ssr * SsrTknx::parse (const std::string &code,
                      const std::string &txt)
{
    ProgTrace(TRACE3, "Parse SSR TKNX");
    SsrTknx ssr(txt);

    string textNoSpace = StrUtils::delSpaces(txt);
    if(textNoSpace.size() == 13)
    {
        textNoSpace += 'N';
    }
    if(textNoSpace.size() != 14)
    {
        throw typeb_parse_except(STDLOG,TBMsg::INV_TICKNUM,
                               "Invalid ticket num length");
    }
    ssr.setTickAct(GetTickActionAirimp(textNoSpace.substr(13).c_str()));
    ssr.setTickNum(textNoSpace.substr(0,13));

    return new SsrTknx(ssr);
}

} // namespace typeb_parser

