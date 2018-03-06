#include "IatciCkiRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;


CkiRequest::CkiRequest(const iatci::CkiParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQCKI,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.outboundFlight().airline(),
                                                                            params.outboundFlight().flightNum())),
      m_params(params)
{}

std::string CkiRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string CkiRequest::funcCode() const
{
    return "I";
}

void CkiRequest::collectMessage()
{
    viewLorElement(pMes(), m_params.org());
    if(m_params.cascade()) {
        viewChdElement(pMes(), *m_params.cascade());
    }

    SetEdiSegGr(pMes(), 1);
    SetEdiPointToSegGrW(pMes(), 1);
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
        if(pxg.seat()) {
            viewPsdElement(pMes(), *pxg.seat());
        }
        if(pxg.baggage()) {
            viewPbdElement(pMes(), *pxg.baggage());
        }
        if(pxg.service()) {
            viewPsiElement(pMes(), *pxg.service());
        }

        int curApg = 0;
        if(pxg.doc() || pxg.address()) {
            PushEdiPointW(pMes());
            edilib::SetEdiSegGr(pMes(), SegGrElement(3, curApg));
            edilib::SetEdiPointToSegGrW(pMes(), SegGrElement(3, curApg), "SegGr3(apg) not found");

            if(pxg.doc()) {
                viewPapElement(pMes(), *pxg.doc(), pxg.pax());
            } else {
                viewPapElement(pMes(), false/*not-infant*/);
            }

            if(pxg.address()) {
                viewAddElement(pMes(), *pxg.address());
            }

            PopEdiPointW(pMes());
            curApg++;
        }

        if(pxg.infantDoc() || pxg.infantAddress()) {
            ASSERT(pxg.infant());

            PushEdiPointW(pMes());
            edilib::SetEdiSegGr(pMes(), SegGrElement(3, curApg));
            edilib::SetEdiPointToSegGrW(pMes(), SegGrElement(3, curApg), "SegGr3(apg) not found");

            if(pxg.infantAddress()) {
                viewPapElement(pMes(), *pxg.infantDoc(), *pxg.infant());
            } else {
                viewPapElement(pMes(), true/*infant*/);
            }

            if(pxg.infantAddress()) {
                viewAddElement(pMes(), *pxg.infantAddress());
            }

            PopEdiPointW(pMes());
            curApg++;
        }

        PopEdiPointW(pMes());
        currPxg++;
    }
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkiRequest(const iatci::CkiParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendCkiRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    CkiRequest ckiReq(params, pult, ctxt, kick);
    ckiReq.sendTlg();
    return ckiReq.ediSessId();
}

}//namespace edifact
