#include "ssr_asvx.h"
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

#include <serverlib/str_utils.h>


namespace typeb_parser
{
using namespace Ticketing::TickStatAction;

Ssr* SsrAsvx::parse(const std::string &code,
                    const std::string &txt)
{
    ProgTrace(TRACE3, "Parse SSR ASVX");
    SsrAsvx ssr(txt);

    std::string textNoSpace = StrUtils::delSpaces(txt);
    if(textNoSpace.size() == 13)
    {
        textNoSpace += 'N';
    }
    if(textNoSpace.size() != 14)
    {
        throw typeb_parse_except(STDLOG,TBMsg::INV_TICKNUM,
                                "Invalid emd num length");
    }
    ssr.setTickAct(GetTickActionAirimp(textNoSpace.substr(13).c_str()));
    ssr.setTickNum(textNoSpace.substr(0,13));

    return new SsrAsvx(ssr);
}

}//namespace typeb_parser
