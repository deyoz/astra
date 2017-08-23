#include "et_cos_request.h"
#include "astra_ticket.h"

#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;

EtCosRequest::EtCosRequest(const EtCosParams& chngStatData)
    : EtRequest(chngStatData), m_chngStatData(chngStatData)
{
}

std::string EtCosRequest::mesFuncCode() const
{
    return "142";
}

void EtCosRequest::collectMessage()
{
    BaseTables::Router rot(sysCont()->routerCanonName());
    viewOrgElement2(pMes(), m_chngStatData.org(), rot->translit());
    ProgTrace(TRACE2,"Tick.size()=%zu", m_chngStatData.ltick().size());
    SetEdiFullSegment(pMes(), SegmElement("EQN"),
                      HelpCpp::string_cast(m_chngStatData.ltick().size()) + ":TD");
    int sg1 = 0;
    Ticketing::Ticket::Trace(TRACE4, m_chngStatData.ltick());
    if(m_chngStatData.isGlobItin()) {
        viewItin2(pMes(), m_chngStatData.itin(), rot->translit());
    }

    for(std::list<Ticketing::Ticket>::const_iterator i = m_chngStatData.ltick().begin();
        i != m_chngStatData.ltick().end(); i++, sg1++)
    {
        ProgTrace(TRACE2, "sg1=%d", sg1);
        const Ticketing::Ticket& tick = (*i);
        SetEdiSegGr(pMes(), 1, sg1);
        SetEdiPointToSegGrW(pMes(), 1, sg1);
        SetEdiFullSegment(pMes(), SegmElement("TKT"), tick.ticknum() + ":T");

        PushEdiPointW(pMes());
        int sg2 = 0;
        for(std::list<Ticketing::Coupon>::const_iterator j = tick.getCoupon().begin();
            j != tick.getCoupon().end(); j++, sg2++)
        {
            const Ticketing::Coupon& cpn = (*j);
            ProgTrace(TRACE2, "sg2=%d", sg2);
            SetEdiSegGr(pMes(), 2, sg2);
            SetEdiPointToSegGrW(pMes(), 2, sg2);

            SetEdiFullSegment(pMes(), SegmElement("CPN"),
                              HelpCpp::string_cast(cpn.couponInfo().num()) + ":" +
                              cpn.couponInfo().status()->code());

            if(cpn.haveItin()) {
                viewItin(pMes(), cpn.itin(), cpn.couponInfo().num());
            }
        }

        PopEdiPointW(pMes());
        ResetEdiPointW(pMes());
    }
}

edilib::EdiSessionId_t SendEtCosRequest(const EtCosParams& cosParams)
{
    edifact::EtCosRequest cosReq(cosParams);
    cosReq.sendTlg();
    return cosReq.ediSessId();
}

}//namespace edifact
