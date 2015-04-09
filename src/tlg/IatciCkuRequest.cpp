#include "IatciCkuRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

CkuRequest::CkuRequest(const iatci::CkuParams& params, const KickInfo& kick)
    : EdifactRequest("pult", "ctxt", kick, DCQCKI,
                     Ticketing::RemoteSystemContext::DcsSystemContext::read(/*params.m_nextFlight.m_airline*/"")),
      m_params(params)
{
}

std::string CkuRequest::mesFuncCode() const
{
    return ""; // no MSG
}

void CkuRequest::collectMessage()
{
//    viewLorElement(pMes(), m_params.m_origin);

//    edilib::SetEdiSegGr(pMes(), 1);
//    edilib::SetEdiPointToSegGrW(pMes(), 1);
//    viewFdqElement(pMes(), m_params.m_nextFlight, m_params.m_currFlight);

//    edilib::SetEdiSegGr(pMes(), 2);
//    edilib::SetEdiPointToSegGrW(pMes(), 2);
//    viewPpdElement(pMes(), m_params.m_pax);
//    if(m_params.m_reserv)
//        viewPrdElement(pMes(), m_params.m_reserv.get());
//    if(m_params.m_seat)
//        viewPsdElement(pMes(), m_params.m_seat.get());
//    if(m_params.m_baggage)
//        viewPbdElement(pMes(), m_params.m_baggage.get());
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkuRequest(const iatci::CkuParams& params,
                                      const KickInfo& kick)
{
    CkuRequest ckuReq(params, kick);
    ckuReq.sendTlg();
    return ckuReq.ediSessId();
}

}//namespace edifact
