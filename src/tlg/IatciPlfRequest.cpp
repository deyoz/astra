#include "IatciPlfRequest.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "iatci_help.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {


PlfRequest::PlfRequest(const iatci::PlfParams& params,
                       const std::string& pult,
                       const std::string& ctxt,
                       const KickInfo& kick)
    : EdifactRequest(pult, ctxt, kick, DCQPLF,
                     iatci::readDcs(params.outboundFlight(),
                                    params.inboundFlight())),
      m_params(params)
{}

std::string PlfRequest::mesFuncCode() const
{
    return ""; // no MSG
}

std::string PlfRequest::funcCode() const
{
    return "P";
}


void PlfRequest::collectMessage()
{
    viewLorElement(pMes(), m_params.org());
    viewFdqElement(pMes(), m_params.outboundFlight());
    viewSpdElement(pMes(), m_params.personal());
    if(m_params.cascade())
        viewChdElement(pMes(), *m_params.cascade());
}


//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendPlfRequest(const iatci::PlfParams& params,
                                      const std::string& pult,
                                      const std::string& ctxt,
                                      const KickInfo& kick)
{
    LogTrace(TRACE3) << "SendPlfRequest from pult[" << pult << "] "
                     << "with context length[" << ctxt.length() << "]";
    PlfRequest plfReq(params, pult, ctxt, kick);
    plfReq.sendTlg();
    LogTrace(TRACE3) << "Created edisession with id " << plfReq.ediSessId();
    return plfReq.ediSessId();
}

}//namespace edifact
