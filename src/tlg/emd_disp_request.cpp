#include "config.h"
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


#ifdef XP_TESTING
#include <serverlib/func_placeholders.h>

void runEdiTimer_4testsOnly();

static std::string FP_init_eds(const std::vector<std::string> &p)
{
    using namespace Ticketing::RemoteSystemContext;

    ASSERT(p.size() == 3);
    std::string airline = p.at(0);
    std::string ediAddrTo = p.at(1);
    std::string ediAddrFrom = p.at(2);

    EdsSystemContext::create4TestsOnly(airline, ediAddrTo, ediAddrFrom);

    // for compatibility
    set_edi_addrs(std::make_pair(ediAddrFrom, ediAddrTo));
    return "";
}

std::string FP_run_daemon(const std::vector<std::string> &params) {
    assert(params.size() > 0);
    if(params.at(0) == "edi_timeout") {
        runEdiTimer_4testsOnly();
    }
    return "";
}

FP_REGISTER("init_eds",   FP_init_eds);
FP_REGISTER("run_daemon", FP_run_daemon);

#endif /* XP_TESTING */
