//
// C++ Interface: ssr_tknx
//
// Description: SSR TKNX parser
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _SSR_TKNX_H_
#define _SSR_TKNX_H_
#include <string>
#include "ssr_parser.h"
#include <etick/tick_data.h>

namespace typeb_parser
{
class SsrTknx : public Ssr
{
    std::string TickNum;
    Ticketing::TickStatAction::TickStatAction_t TickAct;

    SsrTknx(const std::string &text)
        :Ssr(SsrTypes::Tknx, text)
    {
    }
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

#endif /*_SSR_TKNX_H_*/
