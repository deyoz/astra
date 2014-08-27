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
#include "remote_system_context.h"
#include "remote_results.h"
#include "astra_utils.h"

#include <boost/format.hpp>

#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include <jxtlib/jxt_cont.h>
#include <jxtlib/jxt_xml_cont.h>
#include <jxtlib/xml_msg.h>

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing
{

namespace
{
    std::string make_xml_kick(int reqCtxtId = 0)
    {
        std::string text =
        "<?xml version=\"1.0\" encoding=\"CP866\"?>"
        "<term>"
        "<query handle=\"%1%\" id=\"%2%\" ver=\"0\" opr=\"%3%\" screen=\"%4%\" lang=\"%5%\" term_id=\"%6%\">";
        if(reqCtxtId)
            text += "<kick req_ctxt_id=\"%7%\"></kick>";
        else
            text += "<kick/>";
        text +=
        "</query>"
        "</term>";

        using namespace JxtContext;
        TReqInfo *reqInfo = TReqInfo::Instance();
        std::string iface = getSysContext()->read("CUR_IFACE", "");
        std::string handle= getSysContext()->read("HANDLE","");

        std::string oper  = reqInfo->user.login;
        std::string screen= reqInfo->screen.name;
        std::string lang  = reqInfo->desk.lang;
        double   term_id  = reqInfo->desk.term_id != ASTRA::NoExists ? reqInfo->desk.term_id : 0.;

        boost::format str(text);
        str % handle % iface % oper % screen % lang % FloatToString(term_id, 0);
        if(reqCtxtId)
            str % reqCtxtId;

        return str.str();
    }
}

static void ConfigAgentToWait_(const std::string& pult,
                               const Ticketing::SystemAddrs_t &rida,
                               const edilib::EdiSessionId_t &sida,
                               int reqCtxtId)
{
    ServerFramework::getQueryRunner().
            getEdiHelpManager().
            configForPerespros(STDLOG, make_xml_kick(reqCtxtId).c_str(), 0, 56 /*seconds to wait*/);

    edifact::RemoteResults::add(pult, sida, rida);
}

void ConfigAgentToWait(const Ticketing::SystemAddrs_t& rida,
                       const std::string& pult,
                       const edilib::EdiSessionId_t& sida,
                       int reqCtxtId)
{
    //if(Environment::Environ::Instance().handlerType() == Environment::HumanHandler)
    {
        LogTrace(TRACE3) << "ConfigAgentToWait for pult " << pult;
        ConfigAgentToWait_(pult.c_str(), rida, sida, reqCtxtId);
    }
}

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
    //if(Environment::Environ::Instance().handlerType() == Environment::HumanHandler)
//    {
//        if(systype && airline)
//            addXmlMessageBox(getResNode(), Tr(EtsErr::TIMEOUT_SYST) << systype << airline);
//        else
//            addXmlMessageBox(getResNode(), Tr(EtsErr::TIMEOUT));
//    }
}

void addTimeoutMessage(const RemoteSystemContext::SystemContext &cont)
{
    //addTimeoutMessage(cont.systemType()->code(), BaseTables::Company(cont.remoteAirline())->code().c_str());
}

void addTimeoutMessage()
{
    //addTimeoutMessage(0,0);
}

}//namespace Ticketing
