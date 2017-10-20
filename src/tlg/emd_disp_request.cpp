#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "astra_context.h"
#include "astra_ticket.h"
#include "remote_results.h"
#include "AgentWaitsForRemote.h"
#include "view_edi_elements.h"
#include "emd_disp_request.h"
#include "remote_system_context.h"

#include <edilib/edi_func_cpp.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace edifact
{
using namespace edilib;
using namespace Ticketing;


EmdDispRequestByNum::EmdDispRequestByNum(const EmdDispByNum& dispParams)
    : EmdRequest(dispParams), m_dispParams(dispParams)
{
}

void EmdDispRequestByNum::collectMessage()
{
    viewOrgElement(pMes(), m_dispParams.org());
    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    TktElem tkt;
    tkt.m_ticketNum = m_dispParams.tickNum();
    viewTktElement(pMes(), tkt);
}

std::string EmdDispRequestByNum::mesFuncCode() const
{
    return "791";
}

}//namespace edifact
