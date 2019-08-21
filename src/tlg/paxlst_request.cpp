#include "paxlst_request.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

using namespace edilib;


PaxlstReqParams::PaxlstReqParams(const std::string& airline,
                                 const Paxlst::PaxlstInfo& paxlstInfo)
    : RequestParams(Ticketing::OrigOfRequest(airline),
                    "", // ctxt
                    edifact::KickInfo(),
                    airline,
                    Ticketing::FlightNum_t()),
      m_paxlstInfo(paxlstInfo)
{}

const Paxlst::PaxlstInfo& PaxlstReqParams::paxlst() const
{
    return m_paxlstInfo;
}

const Ticketing::RemoteSystemContext::SystemContext* PaxlstReqParams::readSysCont() const
{
    return Ticketing::RemoteSystemContext::IapiSystemContext::read();
}

//---------------------------------------------------------------------------------------

PaxlstRequest::PaxlstRequest(const PaxlstReqParams& params)
    : EdifactRequest(params.org().pult(),
                     params.context(),
                     params.kickInfo(),
                     PAXLST,
                     params.readSysCont())
{
    m_paxlstInfo.reset(new Paxlst::PaxlstInfo(params.paxlst()));
}

std::string PaxlstRequest::mesFuncCode() const
{
    return "";
}

std::string PaxlstRequest::funcCode() const
{
    return "745";
}

void PaxlstRequest::updateMesHead()
{
    ASSERT(msgHead());
    ASSERT(m_paxlstInfo);

    strcpy(msgHead()->FseId, m_paxlstInfo->settings().appRef().c_str());
    if(!m_paxlstInfo->settings().unh_number().empty()) {
        strcpy(msgHead()->unh_number, m_paxlstInfo->settings().unh_number().c_str());
    }
}

void PaxlstRequest::collectMessage()
{
    ASSERT(m_paxlstInfo);
    collectPAXLST(pMes(), *m_paxlstInfo);
}

bool PaxlstRequest::needRemoteResults() const
{
    return false;
}

bool PaxlstRequest::needSaveEdiSessionContext() const
{
    return false;
}

bool PaxlstRequest::needConfigAgentToWait() const
{
    return false;
}

}//namespace edifact
