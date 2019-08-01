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
#include "postpone_trip_task.h"
#include "trip_tasks.h"

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

static void ConfigAgentToWaitJxt(const std::string& pult,
                                 const Ticketing::SystemAddrs_t &rida,
                                 const edilib::EdiSessionId_t &sida,
                                 const edifact::KickInfo &kickInfo)
{
  if (!kickInfo.jxt) return;

  if (kickInfo.background_mode()) return;

  LogTrace(TRACE3) << "ConfigAgentToWait for pult " << pult;

  if (kickInfo.parentSessId!=ASTRA::NoExists) {
    copy_notify_levb(kickInfo.parentSessId, sida.get(), true);
  } else {
    ServerFramework::getQueryRunner().
        getEdiHelpManager().
        configForPerespros(STDLOG, make_xml_kick(kickInfo).c_str(), sida.get(), 15 /*seconds to wait*/);
  }
}

static void ConfigAgentToWaitTask(const std::string& pult,
                                  const Ticketing::SystemAddrs_t &rida,
                                  const edilib::EdiSessionId_t &sida,
                                  const edifact::KickInfo &kickInfo)
{
  if (!kickInfo.task) return;

  LogTrace(TRACE3) << "ConfigAgentToWait for task " << kickInfo.task.get().name;

  if (kickInfo.parentSessId!=ASTRA::NoExists)
  {
    if (!PostponeTripTaskHandling::copyWaiting(edilib::EdiSessionId_t(kickInfo.parentSessId), sida))
      throw EXCEPTIONS::Exception("%s: nothing for session_id=%d", __FUNCTION__, kickInfo.parentSessId);
  }
  else
  {
    PostponeTripTaskHandling::postpone(TTripTaskKey(kickInfo.task.get().point_id,
                                                    kickInfo.task.get().name,
                                                    IntToString(kickInfo.reqCtxtId)),
                                       sida);
  }
}

void ConfigAgentToWait(const Ticketing::SystemAddrs_t& rida,
                       const std::string& pult,
                       const edilib::EdiSessionId_t& sida,
                       const edifact::KickInfo &kickInfo)
{
  ConfigAgentToWaitJxt(pult.c_str(), rida, sida, kickInfo);
  ConfigAgentToWaitTask(pult.c_str(), rida, sida, kickInfo);
}

void MeetAgentExpectations(const edifact::RemoteResults & res)
{
    res.updateDb();
    LogTrace(TRACE1) << __FUNCTION__ << " " << res;
    if (!res.isSystemPult())  //!!!vlad msgId?
    {
        LogTrace(TRACE3) << "confirm_notify_levb for edisession: " << res.ediSession();
        confirm_notify_levb(res.ediSession().get(), true);
    }

    boost::optional<TTripTaskKey> task=PostponeTripTaskHandling::deleteWaiting(res.ediSession());
    if (task)
    {
      //последняя телеграмма, которую ожидали
      //выполняем задачу
      add_trip_task(task.get());
    }
}

static bool isDoomedToWait()
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
