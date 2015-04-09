#include "IatciCkxRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

CkxRequest::CkxRequest(const iatci::CkxParams& params, const KickInfo& kick)
    : EdifactRequest("pult", "ctxt", kick, DCQCKX,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.flight().airline())),
      m_params(params)
{
}

std::string CkxRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string CkxRequest::funcCode() const
{
    return "X";
}

void CkxRequest::collectMessage()
{
    viewLorElement(pMes(), m_params.origin());
    if(m_params.cascadeDetails())
        viewChdElement(pMes(), *m_params.cascadeDetails());

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    viewFdqElement(pMes(), m_params.flight());

    edilib::SetEdiSegGr(pMes(), 2);
    edilib::SetEdiPointToSegGrW(pMes(), 2);
    viewPpdElement(pMes(), m_params.pax());
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkxRequest(const iatci::CkxParams& params,
                                      const KickInfo& kick)
{
    CkxRequest ckxReq(params, kick);
    ckxReq.sendTlg();
    return ckxReq.ediSessId();
}

}//namespace edifact
