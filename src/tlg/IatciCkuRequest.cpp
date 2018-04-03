#include "IatciCkuRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;


CkuRequest::CkuRequest(const iatci::CkuParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQCKU,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.outboundFlight().airline(),
                                                                            params.outboundFlight().flightNum())),
      m_params(params)
{}

std::string CkuRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string CkuRequest::funcCode() const
{
    return "U";
}

void CkuRequest::collectMessage()
{
    viewLorElement(pMes(), m_params.org());
    if(m_params.cascade())
        viewChdElement(pMes(), *m_params.cascade());

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    viewFdqElement(pMes(), m_params.outboundFlight(), m_params.inboundFlight());

    int currPxg = 0;
    for(const auto& pxg: m_params.fltGroup().paxGroups()) {
        PushEdiPointW(pMes());

        SetEdiSegGr(pMes(), SegGrElement(2, currPxg));
        SetEdiPointToSegGrW(pMes(), SegGrElement(2, currPxg));

        if(pxg.infant()) {
            viewPpdElement(pMes(), pxg.pax(), *pxg.infant());
        } else {
            viewPpdElement(pMes(), pxg.pax());
        }

        if(pxg.reserv()) {
            viewPrdElement(pMes(), *pxg.reserv());
        }
        if(pxg.baggage()) {
            viewPbdElement(pMes(), *pxg.baggage());
        }
        if(pxg.service()) {
            viewPsiElement(pMes(), *pxg.service());
        }

        if(pxg.updPax()) {
            viewUpdElement(pMes(), *pxg.updPax());
        }
        if(pxg.updSeat()) {
            viewUsdElement(pMes(), *pxg.updSeat());
        }
        if(pxg.updBaggage()) {
            viewUbdElement(pMes(), *pxg.updBaggage());
        }
        if(pxg.updService()) {
            viewUsiElement(pMes(), *pxg.updService());
        }

        int curApg = 0;
        if(pxg.updDoc() || pxg.updVisa() || pxg.updAddress()) {
            PushEdiPointW(pMes());
            edilib::SetEdiSegGr(pMes(), SegGrElement(3, curApg));
            SetEdiPointToSegGrW(pMes(), SegGrElement(3, curApg), "SegGr3(apg) not found");

            if(pxg.updDoc() || pxg.updVisa()) {
                viewUapElement(pMes(), pxg.pax(), pxg.updDoc(), pxg.updVisa());
            } else {
                viewUapElement(pMes(), false/*not-infant*/);
            }

            if(pxg.updAddress()) {
                viewAddElement(pMes(), *pxg.updAddress(), *pxg.updAddress());
            }

            PopEdiPointW(pMes());
            curApg++;
        }

        if(pxg.updInfantDoc() || pxg.updInfantVisa() || pxg.updInfantAddress()) {
            ASSERT(pxg.infant());
            PushEdiPointW(pMes());
            SetEdiSegGr(pMes(), SegGrElement(3, curApg));
            SetEdiPointToSegGrW(pMes(), SegGrElement(3, curApg), "SegGr3(apg) not found");

            if(pxg.updInfantDoc() || pxg.updInfantVisa()) {
                viewUapElement(pMes(), *pxg.infant(), pxg.updInfantDoc(), pxg.updInfantVisa());
            } else {
                viewUapElement(pMes(), true/*infant*/);
            }

            if(pxg.updInfantAddress()) {
                viewAddElement(pMes(), *pxg.updInfantAddress(), *pxg.updInfantAddress());
            }

            PopEdiPointW(pMes());
            curApg++;
        }

        PopEdiPointW(pMes());
        currPxg++;
    }
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkuRequest(const iatci::CkuParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendCkuRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    CkuRequest ckuReq(params, pult, ctxt, kick);
    ckuReq.sendTlg();
    return ckuReq.ediSessId();
}

}//namespace edifact
