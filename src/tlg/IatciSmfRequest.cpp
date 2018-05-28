#include "IatciSmfRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "iatci_help.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

SmfRequest::SmfRequest(const iatci::SmfParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQSMF,
                     iatci::readDcs(params.outboundFlight(),
                                    params.inboundFlight())),
      m_params(params)
{
}

std::string SmfRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string SmfRequest::funcCode() const
{
    return "S";
}

void SmfRequest::collectMessage()
{
    viewLorElement(pMes(), m_params.org());
    viewFdqElement(pMes(), m_params.outboundFlight());
    if(m_params.seatRequest())
        viewSrpElement(pMes(), *m_params.seatRequest());
    if(m_params.cascade())
        viewChdElement(pMes(), *m_params.cascade());
}

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendSmfRequest(const iatci::SmfParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendSmfRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    SmfRequest smfReq(params, pult, ctxt, kick);
    smfReq.sendTlg();
    LogTrace(TRACE3) << "Created edisession with id " << smfReq.ediSessId();
    return smfReq.ediSessId();
}

}//namespace edifact
