#include "IatciCkuRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

CkuRequest::CkuRequest(const iatci::CkuParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQCKU,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.flight().airline())),
      m_params(params)
{
}

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
    viewLorElement(pMes(), m_params.origin());
    if(m_params.cascadeDetails())
        viewChdElement(pMes(), *m_params.cascadeDetails());

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    viewFdqElement(pMes(), m_params.flight(), m_params.flightFromPrevHost());

    edilib::SetEdiSegGr(pMes(), 2);
    edilib::SetEdiPointToSegGrW(pMes(), 2);
    viewPpdElement(pMes(), m_params.pax());
    if(m_params.updPax())
        viewUpdElement(pMes(), *m_params.updPax());
    if(m_params.updSeat())
        viewUsdElement(pMes(), *m_params.updSeat());
    if(m_params.updBaggage())
        viewUbdElement(pMes(), *m_params.updBaggage());
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
