#include "IatciBprRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

BprRequest::BprRequest(const iatci::BprParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQBPR,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.flight().airline())),
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
    viewLorElement(pMes(), m_params.origin());
    if(m_params.cascadeDetails())
        viewChdElement(pMes(), *m_params.cascadeDetails());

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    viewFdqElement(pMes(), m_params.flight(), m_params.flightFromPrevHost());

    edilib::SetEdiSegGr(pMes(), 2);
    edilib::SetEdiPointToSegGrW(pMes(), 2);
    viewPpdElement(pMes(), m_params.pax());
    if(m_params.reserv())
        viewPrdElement(pMes(), *m_params.reserv());
    if(m_params.seat())
        viewPsdElement(pMes(), *m_params.seat());
    if(m_params.baggage())
        viewPbdElement(pMes(), *m_params.baggage());
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
    LogTrace(TRACE3) << "Created edisession with id " << bprReq.ediSessId();
    return bprReq.ediSessId();
}

}//namespace edifact
