#include "IatciBprRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;

BprRequest::BprRequest(const iatci::BprParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQBPR,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.outboundFlight().airline(),
                                                                            params.outboundFlight().flightNum())),
      m_params(params)
{
}

std::string BprRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string BprRequest::funcCode() const
{
    return "B";
}

void BprRequest::collectMessage()
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
        SetEdiPointToSegGrW(pMes(), SegGrElement(2, currPxg++));

        viewPpdElement(pMes(), pxg.pax());
        if(pxg.reserv()) {
            viewPrdElement(pMes(), *pxg.reserv());
        }
        if(pxg.baggage()) {
            viewPbdElement(pMes(), *pxg.baggage());
        }
        if(pxg.service()) {
            viewPsiElement(pMes(), *pxg.service());
        }

        PopEdiPointW(pMes());
    }
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendBprRequest(const iatci::BprParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendBprRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    BprRequest bprReq(params, pult, ctxt, kick);
    bprReq.sendTlg();
    return bprReq.ediSessId();
}

}//namespace edifact
