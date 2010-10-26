/*
*  C++ Implementation: AgentWaitsForRemote
*
* Description:
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#include "AgentWaitsForRemote.h"
#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include "boost/format.hpp"
#include "jxtlib/jxt_cont.h"
#include "environ.h"
#include "jxt/EtickXmlRequestCtxt.h"
//
#include "remote_system_context.h"
#include "local.h"
#include "etick_msg.h"
#include "basetables.h"
#include "jxtlib/xml_msg.h"
#include "RemoteResults.h"

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include "serverlib/slogger.h"

namespace Ticketing
{

namespace
{
    std::string make_xml_kick()
    {
        const char * text =
                "<?xml version=\"1.0\" encoding=\"CP866\"?>"
                "<term>"
                "<query handle=\"%1%\" id=\"%2%\" ver=\"0\" opr=\"%3%\">"
                "<kick>%4%</kick>"
                "</query>"
                "</term>";

        using namespace JxtContext;
        std::string iface = getSysContext()->read("CUR_IFACE", "");
        std::string handle= getSysContext()->read("HANDLE","");
        std::string oper  = getSysContext()->read("OPR",   "");

        boost::format str(text);
        str % handle % iface % oper % "";

        return str.str();
    }
}

static void ConfigAgentToWait(const std::string& pult,
                              const Ticketing::SystemAddrs_t &rida,
                              const edilib::EdiSessionId_t &sida)
{
    ServerFramework::getQueryRunner().
            getEdiHelpManager().
            configForPerespros(STDLOG, make_xml_kick().c_str(), 56 /*seconds to wait*/);

    edifact::RemoteResults::add(pult, sida, rida);
}

void ConfigAgentToWait(const Ticketing::SystemAddrs_t &rida,
                       const edilib::EdiSessionId_t &sida)
{
    if(Environment::Environ::Instance().handlerType() == Environment::HumanHandler)
    {
        ConfigAgentToWait(getEtickXmlCtxt()->pult().pultCode().c_str(), rida, sida);
    }
}

// void MeetAgentExpectations(const edilib::EdiSessionId_t &sida,
//                            const edifact::RemoteStatus &s)
// {
//     edifact::pRemoteResults rr = edifact::RemoteResults::readDb(sida);
//     if(rr)
//     {
//         rr->setStatus(s);
//         MeetAgentExpectations(*rr);
//     }
// }

void MeetAgentExpectations(const edifact::RemoteResults & res)
{
    res.updateDb();
    LogTrace(TRACE1) << res;
    // cleanOldRecords can be called in some daemon
    ServerFramework::EdiHelpManager::cleanOldRecords();
    ServerFramework::EdiHelpManager::confirm_notify(res.pult().c_str());
}

bool isDoomedToWait()
{
    return ServerFramework::getQueryRunner().getEdiHelpManager().mustWait();
}

inline void addTimeoutMessage(const char *systype, const char *airline)
{
    if(Environment::Environ::Instance().handlerType() == Environment::HumanHandler)
    {
        if(systype && airline)
            addXmlMessageBox(getResNode(), Tr(EtsErr::TIMEOUT_SYST) << systype << airline);
        else
            addXmlMessageBox(getResNode(), Tr(EtsErr::TIMEOUT));
    }
}

void addTimeoutMessage(const RemoteSystemContext::SystemContext &cont)
{
    addTimeoutMessage(cont.systemType()->code(), BaseTables::Company(cont.remoteAirline())->code().c_str());
}

void addTimeoutMessage()
{
    addTimeoutMessage(0,0);
}

}
