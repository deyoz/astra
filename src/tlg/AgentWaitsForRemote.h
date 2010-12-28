//
// C++ Interface: AgentWaitsForRemote
//
// Description: Agent Waits For Remote System
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _AGENTWAITSFORREMOTE_H_
#define _AGENTWAITSFORREMOTE_H_
#include "TicketBaseTypesFwd.h"
#include "edilib/EdiSessionId_t.h"

namespace edifact
{
    class RemoteStatus;
    class RemoteResults;
}
namespace Ticketing
{
    namespace RemoteSystemContext
    {
        class SystemContext;
    }
    void ConfigAgentToWait(const Ticketing::SystemAddrs_t &rida,
                           const edilib::EdiSessionId_t &sida);
    void MeetAgentExpectations(const edifact::RemoteResults &res);
    bool isDoomedToWait();

    void addTimeoutMessage();
    void addTimeoutMessage(const RemoteSystemContext::SystemContext &cont);
}

#endif /*_AGENTWAITSFORREMOTE_H_*/
