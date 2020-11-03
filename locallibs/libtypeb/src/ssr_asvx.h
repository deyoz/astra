#ifndef _SSR_ASVX_H_
#define _SSR_ASVX_H_

#include "ssr_parser.h"
#include <etick/tick_data.h>

namespace typeb_parser
{
class SsrAsvx : public Ssr
{
    std::string TickNum;
    Ticketing::TickStatAction::TickStatAction_t TickAct;

    SsrAsvx(const std::string &text)
        :Ssr(SsrTypes::Asvx, text)
    {}

public:
    const std::string &tickNum() const { return TickNum; }
    Ticketing::TickStatAction::TickStatAction_t
            tickAct() const { return TickAct; }

    void setTickNum(const std::string &tnum)
    {
        TickNum = tnum;
    }
    void setTickAct(Ticketing::TickStatAction::TickStatAction_t act)
    {
        TickAct = act;
    }
    static Ssr * parse (const std::string &code,
                        const std::string &txt);
};

} // namespace typeb_parser

#endif /*_SSR_ASVX_H_*/
