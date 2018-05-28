#include "IatciCkxRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "iatci_help.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;


CkxRequest::CkxRequest(const iatci::CkxParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQCKX,
                     iatci::readDcs(params.outboundFlight(),
                                    params.inboundFlight())),
      m_params(params)
{}

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
    viewLorElement(pMes(), m_params.org());
    if(m_params.cascade())
        viewChdElement(pMes(), *m_params.cascade());

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    viewFdqElement(pMes(), m_params.outboundFlight());

    int currPxg = 0;
    for(const auto& pxg: m_params.fltGroup().paxGroups()) {
        PushEdiPointW(pMes());

        SetEdiSegGr(pMes(), SegGrElement(2, currPxg));
        SetEdiPointToSegGrW(pMes(), SegGrElement(2, currPxg++));

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

        PopEdiPointW(pMes());
    }
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkxRequest(const iatci::CkxParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendCkxRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    CkxRequest ckxReq(params, pult, ctxt, kick);
    ckxReq.sendTlg();
    return ckxReq.ediSessId();
}

}//namespace edifact
