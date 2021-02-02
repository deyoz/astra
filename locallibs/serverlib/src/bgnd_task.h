#pragma once

#include "bgnd.h"
#include "daemon_event.h"
#include "daemon_task.h"

namespace bgnd
{

// если запрос не выполнен, то можно его выбросить, а можно попробовать переобработать позже
// для второго варианта достаточно сохранить этот же запрос с новым id

class BgndRequestTask
    : public ServerFramework::CyclicDaemonTask<BgndReqId>
{
public:
    BgndRequestTask(const std::string& tag, const ServerFramework::DaemonTaskTraits& traits);
    BgndRequestTask(const std::string& tag, bool unordered, const ServerFramework::DaemonTaskTraits& traits);
    virtual Message run(const boost::posix_time::ptime&, const bgnd::Request&) = 0;
    virtual Message handleError(const boost::posix_time::ptime& tm, const bgnd::Request& req) const;

    int run(const boost::posix_time::ptime&, const BgndReqId&, bool) override;

protected:
    using ServerFramework::CyclicDaemonTask<BgndReqId>::run;
    Message bgndErr_;
    bool unordered_;
};

} // bgnd
