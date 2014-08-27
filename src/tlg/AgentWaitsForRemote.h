#pragma once

#include "CheckinBaseTypes.h"
#include <edilib/EdiSessionId_t.h>

namespace edifact
{
    class RemoteStatus;
    class RemoteResults;
}//namespace edifact

namespace Ticketing
{
    namespace RemoteSystemContext
    {
        class SystemContext;
    }//namespace RemoteSystemContext

    void ConfigAgentToWait(const Ticketing::SystemAddrs_t &rida,
                           const std::string& pult,
                           const edilib::EdiSessionId_t& sida,
                           int reqCtxtId);
    void MeetAgentExpectations(const edifact::RemoteResults &res);
    bool isDoomedToWait();

    void addTimeoutMessage();
    void addTimeoutMessage(const RemoteSystemContext::SystemContext &cont);

}//namespace Ticketing
