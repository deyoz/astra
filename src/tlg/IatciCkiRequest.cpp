#include "IatciCkiRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

CkiRequest::CkiRequest(const iatci::CkiParams& params, const KickInfo& kick)
    : EdifactRequest("pult", "ctxt", kick, DCQCKI,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(params.flight().airline())),
      m_params(params)
{
}

std::string CkiRequest::mesFuncCode() const
{
    return ""; // no MSG
}

void CkiRequest::collectMessage()
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

edilib::EdiSessionId_t SendCkiRequest(const iatci::CkiParams& params,
                                      const KickInfo& kick)
{
    CkiRequest ckiReq(params, kick);
    ckiReq.sendTlg();
    return ckiReq.ediSessId();
}

}//namespace edifact
