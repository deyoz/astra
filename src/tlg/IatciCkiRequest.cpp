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
        SetEdiPointToSegGrW(pMes(), SegGrElement(2, currPxg++));

        viewPpdElement(pMes(), pxg.pax());
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

        if(pxg.doc()) {
            PushEdiPointW(pMes());
            edilib::SetEdiSegGr(pMes(), 3);
            edilib::SetEdiPointToSegGrW(pMes(), 3);

            viewPapElement(pMes(), *pxg.doc());

            PopEdiPointW(pMes());
        }

        PopEdiPointW(pMes());
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
