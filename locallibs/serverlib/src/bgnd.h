#pragma once

#include <vector>
#include <set>
#include <boost/optional.hpp>

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

class Storage
{
public:
    virtual ~Storage();

    static Storage& instance();
    static void setupStorage(Storage*);

    virtual void commit() = 0;
    virtual std::set<BgndReqId> readAllReqs(const std::string& tag) = 0;
    virtual std::set<BgndReqId> readAllReqsUnordered(const std::string& tag) = 0;
    virtual BgndReqId nextBgndId() = 0;
    virtual boost::optional<Request> readReq(const BgndReqId& id) = 0;
    virtual boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId&,
            const ct::CommandId&, const std::string& req) = 0;
    virtual void delReq(const BgndReqId&) = 0;
    virtual void delPrevOrEqualReqs(const BgndReqId&) = 0;
    virtual void delAllReqs() const = 0;

    virtual void addError(const BgndReqId&, const ct::CommandId&, const std::string& errText) = 0;
    virtual Errors readErrors(const ct::CommandId&) = 0;
    virtual void delErrors(const ct::CommandId&) = 0;
};

void setupOracleStorage();
void setupPgStorage();
void cleanBgndReqs();

} // bgnd
