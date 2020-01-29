#ifndef SERVERLIB_BGND_DAEMON_TASK_H
#define SERVERLIB_BGND_DAEMON_TASK_H

#include <vector>
#include <boost/optional.hpp>

#include "daemon_event.h"
#include "daemon_task.h"
#include "objid.h"
#include "rip.h"
#include "rip_validators.h"
#include "message.h"

namespace OciCpp { class OciSession; }

namespace bgnd
{
DECL_RIP_LENGTH(BgndReqId, std::string, OBJ_ID_LEN, OBJ_ID_LEN);

struct Request
{
    Request(const BgndReqId&, const ct::UserId&, const ct::CommandId&, const std::string&, unsigned attempt = 0);

    BgndReqId id;
    ct::UserId uid;
    ct::CommandId parCmdId;
    std::string req;
    unsigned attempt;
};
std::ostream& operator<<(std::ostream&, const bgnd::Request&);

struct Error
{
    Error(const BgndReqId&, const ct::CommandId&, const std::string&);

    BgndReqId id;
    ct::CommandId parCmdId;
    std::string errText;
};
typedef std::vector<Error> Errors;

boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId&, const ct::CommandId&, const std::string&);
boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId& uid, const ct::CommandId& cmdId,
                                  const std::string& req, const std::string& socketName);
Errors readErrors(const ct::CommandId&);
void delErrors(const ct::CommandId&);

void setSession(OciCpp::OciSession*);

// если запрос не выполнен, то можно его выбросить, а можно попробовать переобработать позже
// для второго варианта достаточно сохранить этот же запрос с новым id

class BgndRequestTask
    : public ServerFramework::CyclicDaemonTask<BgndReqId>
{
public:
    virtual ~BgndRequestTask() {}
    BgndRequestTask(const std::string& tag, const ServerFramework::DaemonTaskTraits& traits);
    virtual Message run(const boost::posix_time::ptime&, const bgnd::Request&) = 0;
    virtual Message handleError(const boost::posix_time::ptime& tm, const bgnd::Request& req) const;

private:
    int run(const boost::posix_time::ptime&, const BgndReqId&, bool) override final;

protected:
    Message bgndErr_;
};

#ifdef XP_TESTING
namespace external_tests {

void check_bgnd_reqs();
void check_daemon_task();
void check_bgnd_errors();

}
#endif

} // bgnd

#endif /* SERVERLIB_BGND_DAEMON_TASK_H */

