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
#include "astra_context.h"
#include "edi_utils.h"

#include <boost/format.hpp>

#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include <jxtlib/jxt_cont.h>
#include <jxtlib/jxt_xml_cont.h>
#include <jxtlib/xml_msg.h>

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace AstraEdifact;

namespace Ticketing
{

static void ConfigAgentToWait_(const std::string& pult,
                               const Ticketing::SystemAddrs_t &rida,
                               const edilib::EdiSessionId_t &sida,
                               const edifact::KickInfo &kickInfo)
{
  //std::string iface = JxtContext::getSysContext()->read("CUR_IFACE", ""); Не использовать эту конструкцию, так как от имени одного пульта м.б. множество разных параллельных запросов

  ServerFramework::getQueryRunner().
      getEdiHelpManager().
      configForPerespros(STDLOG, make_xml_kick(kickInfo).c_str(), sida.get(), 15 /*seconds to wait*/);

  //edifact::RemoteResults::add(pult, sida, rida); здесь не надо, потому что это логически не связано с AgentToWait

}

void ConfigAgentToWait(const Ticketing::SystemAddrs_t& rida,
                       const std::string& pult,
                       const edilib::EdiSessionId_t& sida,
                       const edifact::KickInfo &kickInfo)
{
    //if(Environment::Environ::Instance().handlerType() == Environment::HumanHandler)    
    if (!kickInfo.empty())
    {
        LogTrace(TRACE3) << "ConfigAgentToWait for pult " << pult;
        ConfigAgentToWait_(pult.c_str(), rida, sida, kickInfo);
    }
}

void MeetAgentExpectations(const edifact::RemoteResults & res)
{
    res.updateDb();
    LogTrace(TRACE1) << res;

    if (!res.pult().empty())
    {
      LogTrace(TRACE3) << "confirm_notify_levb for edisession: " << res.ediSession();
      confirm_notify_levb(res.ediSession().get());
    };
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
