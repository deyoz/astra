#include <set>

#include "str_utils.h"
#include "bgnd.h"
#include "bgnd_task.h"
#include "oci_seq.h"
#include "cursctl.h"
#include "rip_oci.h"
#include "dates_oci.h"
#include "posthooks.h"
#include "daemon_kicker.h"
#include "new_daemon.h"
#include "helpcpp.h"
#include "message.h"
#include "checkunit.h"
#include "lwriter.h"
#include "pg_cursctl.h"
#include "pg_seq.h"
#include "pg_rip.h"
#include "tcl_utils.h"
#include "attempts.h"
#include "lngv_user.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace
{

static std::unique_ptr<bgnd::Storage> storage;

static const size_t MaxPartSize = 4000;
class OracleStorage : public bgnd::Storage
{
public:
    void commit() override {
        ::commit();
    }

    std::set<bgnd::BgndReqId> readAllReqs(const std::string& tag) override {
        std::set<bgnd::BgndReqId> tmpReqs;;
        std::string id;
        OciCpp::CursCtl cr(make_curs("SELECT ID FROM BGND_REQS WHERE TAG = :tag"));
        cr.bind( ":tag", tag ).def(id).exec();
        while (!cr.fen()) {
            tmpReqs.insert(bgnd::BgndReqId(id));
        }
        return tmpReqs;
    }

    std::set<bgnd::BgndReqId> readAllReqsUnordered(const std::string& tag) override {
        LogError(STDLOG) << "readAllReqsUnordered oracle N/A";
        return {};
    }

    bgnd::BgndReqId nextBgndId() override {
        static const std::string prefix("bgnd");
        return bgnd::BgndReqId(HelpCpp::convertToId(
                    HelpCpp::objId("BGND_SEQ"), OBJ_ID_LEN - prefix.size(), prefix));
    }

    boost::optional<bgnd::Request> readReq(const bgnd::BgndReqId& id) override {
        boost::optional<bgnd::Request> req;
        unsigned attempt = 0;
        size_t parts;
        std::string uid, cmd, reqText;
        make_curs("SELECT ATTEMPT, PARENT_ID, REQ_TEXT, USER_ID, PARTS FROM BGND_REQS WHERE ID = :id_")
            .bind(":id_", id)
            .def(attempt).def(cmd).def(reqText).def(uid).def(parts)
            .EXfet();
        if (parts > 1) {
            std::string reqPart;
            size_t oldPartNum = 0, partNum = 0;
            OciCpp::CursCtl curs(make_curs("SELECT REQ_TEXT, PART FROM BGND_REQS_PARTS WHERE ID = :id_ ORDER BY PART"));
            curs.bind(":id_", id)
                .def(reqPart)
                .def(partNum);
            curs.exec();
            while (!curs.fen()) {
                if (partNum != ++oldPartNum) {
                    LogError(STDLOG) << "wrong parts sequence for id: " << id;
                    return boost::optional<bgnd::Request>();
                }
                reqText += reqPart;
            }
            if (partNum + 1 != parts) {
                LogError(STDLOG) << "not all parts has been read for id: " << id;
                return boost::optional<bgnd::Request>();
            }
        }
        if (!uid.empty()) {
            req = bgnd::Request(id, ct::UserId(uid), ct::CommandId(cmd), reqText, attempt);
        }
        return req;
    }

    boost::optional<bgnd::BgndReqId> addReq(const std::string& tag, const ct::UserId& uid,
            const ct::CommandId& parCmdId, const std::string& req) override {
        try {
            const bgnd::BgndReqId id(nextBgndId());
            const std::vector<std::string> reqStr(StrUtils::str_parts_utf8(req, MaxPartSize));
            make_curs("INSERT INTO BGND_REQS (ID, ATTEMPT, PARENT_ID, RDATE, REQ_TEXT, USER_ID, TAG, PARTS)"
                    "VALUES (:id_, 0, :par, :rd, :req, :uid_, :tag, :parts)")
                .bind(":id_", id).bind(":par", parCmdId)
                .bind(":rd", Dates::currentDateTime())
                .bind(":req", reqStr.at(0)).bind(":uid_", uid).bind( ":tag", tag )
                .bind(":parts", reqStr.size())
                .exec();
            if (reqStr.size() > 1) {
                for (size_t i = 1, i_end = reqStr.size(); i != i_end; ++i) {
                    make_curs("INSERT INTO BGND_REQS_PARTS(ID, REQ_TEXT, PART) "
                            "VALUES(:id_, :req, :part)")
                        .bind(":id_", id)
                        .bind(":req", reqStr.at(i))
                        .bind(":part", i)
                        .exec();
                }
            }
            return id;
        } catch (const OciCpp::ociexception& e) {
            LogError(STDLOG) << e.what();
            return boost::none;
        }
    }

    void delReq(const bgnd::BgndReqId& id) override {
        make_curs("DELETE FROM BGND_REQS WHERE ID = :id_")
            .bind(":id_", id)
            .exec();
        make_curs("DELETE FROM BGND_REQS_PARTS WHERE ID = :id_")
            .bind(":id_", id)
            .exec();
    }

    void delPrevOrEqualReqs(const bgnd::BgndReqId& id) override {
        LogError(STDLOG) << "delPrevOrEqualReqs oracle N/A";
    }

    void delAllReqs() const override {
        make_curs("DELETE FROM BGND_REQS").exec();
        make_curs("DELETE FROM BGND_REQS_PARTS").exec();
    }

    void addError(const bgnd::BgndReqId& id, const ct::CommandId& parCmdId, const std::string& errText) override {
        try {
            make_curs("INSERT INTO BGND_ERRS(ID, PARENT_ID, ERR_TEXT)"
                "VALUES (:id_, :cmdId, :err)")
              .bind(":id_", id).bind(":cmdId", parCmdId).bind(":err", errText)
              .exec();
        } catch (const OciCpp::ociexception& e) {
            LogTrace(TRACE0) << "reqId=" << id << " parCmdId=" << parCmdId << " errText=[" << errText << "]";
            LogError(STDLOG) << "addBgndError failed: " << e.what();
        }
    }

    bgnd::Errors readErrors(const ct::CommandId& parCmdId) override {
        std::string id, err;
        bgnd::Errors errors;
        OciCpp::CursCtl cr(make_curs("SELECT ID, ERR_TEXT FROM BGND_ERRS WHERE PARENT_ID = :id_"));
        cr.bind(":id_", parCmdId)
          .def(id).def(err)
          .exec();
        while (!cr.fen()) {
            errors.push_back(bgnd::Error(bgnd::BgndReqId(id), parCmdId, err));
        }
        return errors;
    }

    void delErrors(const ct::CommandId& parCmdId) override {
        make_curs("DELETE FROM BGND_ERRS WHERE PARENT_ID = :id_")
          .bind(":id_", parCmdId)
          .exec();
    }
};

class PgStorage : public bgnd::Storage
{
public:
    PgStorage(PgCpp::SessionDescriptor sd)
        : sd_(sd)
    {}

    void commit() override {
        PgCpp::commit();
    }

    std::set<bgnd::BgndReqId> readAllReqs(const std::string& tag) override {
        std::set<bgnd::BgndReqId> tmpReqs;;
        std::string id;
        PgCpp::CursCtl cr(make_pg_curs(sd_, "SELECT ID FROM BGND_REQS WHERE TAG = :tag"));
        cr.bind( ":tag", tag ).def(id).exec();
        while (!cr.fen()) {
            tmpReqs.insert(bgnd::BgndReqId(id));
        }
        return tmpReqs;
    }

    std::set<bgnd::BgndReqId> readAllReqsUnordered(const std::string& tag) override {
        std::set<bgnd::BgndReqId> tmpReqs;;
        std::string id;
        PgCpp::CursCtl cr(make_pg_curs(sd_, "SELECT MAX(ID) FROM BGND_REQS WHERE TAG = :tag GROUP BY REQ_TEXT"));
        cr.bind( ":tag", tag ).def(id).exec();
        while (!cr.fen()) {
            tmpReqs.insert(bgnd::BgndReqId(id));
        }
        return tmpReqs;
    }

    bgnd::BgndReqId nextBgndId() override {
        static const std::string prefix("bgnd");
        return bgnd::BgndReqId(HelpCpp::convertToId(
                    PgCpp::Sequence(sd_, "SEQ_BGND").nextval<std::string>(STDLOG),
                    OBJ_ID_LEN - prefix.size(), prefix));
    }

    boost::optional<bgnd::Request> readReq(const bgnd::BgndReqId& id) override {
        boost::optional<bgnd::Request> req;
        int attempt = 0;
        std::string uid, cmd, reqText;
        make_pg_curs(sd_, "SELECT ATTEMPT, PARENT_ID, REQ_TEXT, USER_ID FROM BGND_REQS WHERE ID = :id_")
            .bind(":id_", id)
            .def(attempt).def(cmd).def(reqText).def(uid)
            .EXfet();
        if (!uid.empty()) {
            req = bgnd::Request(id, ct::UserId(uid), ct::CommandId(cmd), reqText, attempt);
        }
        return req;
    }

    boost::optional<bgnd::BgndReqId> addReq(const std::string& tag, const ct::UserId& uid,
            const ct::CommandId& parCmdId, const std::string& req) override {
        const bgnd::BgndReqId id(nextBgndId());
        make_pg_curs(sd_, "INSERT INTO BGND_REQS (ID, ATTEMPT, PARENT_ID, RDATE, REQ_TEXT, USER_ID, TAG)"
                "VALUES (:id_, 0, :par, :rd, :req, :uid_, :tag)")
            .bind(":id_", id).bind(":par", parCmdId)
            .bind(":rd", Dates::currentDateTime())
            .bind(":req", req).bind(":uid_", uid).bind( ":tag", tag )
            .exec();
        return id;
    }

    void delReq(const bgnd::BgndReqId& id) override {
        make_pg_curs(sd_, "DELETE FROM BGND_REQS WHERE ID = :id_")
            .bind(":id_", id)
            .exec();
    }

    void delPrevOrEqualReqs(const bgnd::BgndReqId& id) override {
        make_pg_curs(sd_, "DELETE FROM BGND_REQS WHERE ID IN"
                " (SELECT B2.ID FROM BGND_REQS B1, BGND_REQS B2 WHERE B1.ID = :id_"
                "   AND B2.ID <= B1.ID AND B2.REQ_TEXT = B1.REQ_TEXT)")
            .bind(":id_", id)
            .exec();
    }

    void delAllReqs() const override {
        make_pg_curs(sd_, "DELETE FROM BGND_REQS").exec();
    }

    void addError(const bgnd::BgndReqId& id, const ct::CommandId& parCmdId, const std::string& errText) override {
        make_pg_curs(sd_, "INSERT INTO BGND_ERRS(ID, PARENT_ID, ERR_TEXT)"
                "VALUES (:id_, :cmdId, :err)")
            .bind(":id_", id).bind(":cmdId", parCmdId).bind(":err", errText)
            .exec();
    }

    bgnd::Errors readErrors(const ct::CommandId& parCmdId) override {
        std::string id, err;
        bgnd::Errors errors;
        PgCpp::CursCtl cr(make_pg_curs(sd_, "SELECT ID, ERR_TEXT FROM BGND_ERRS WHERE PARENT_ID = :id_"));
        cr.bind(":id_", parCmdId)
          .def(id).def(err)
          .exec();
        while (!cr.fen()) {
            errors.push_back(bgnd::Error(bgnd::BgndReqId(id), parCmdId, err));
        }
        return errors;
    }

    void delErrors(const ct::CommandId& parCmdId) override {
        make_pg_curs(sd_, "DELETE FROM BGND_ERRS WHERE PARENT_ID = :id_")
          .bind(":id_", parCmdId)
          .exec();
    }

private:
    PgCpp::SessionDescriptor sd_;
};

static void fillQueue(const std::string& tag, const boost::posix_time::ptime&, std::queue<bgnd::BgndReqId>& reqs)
{
    static const std::queue<bgnd::BgndReqId>::size_type MAX_QUEUE_SIZE = 128;
    bgnd::Storage& s = bgnd::Storage::instance();
    for (const bgnd::BgndReqId& reqId : s.readAllReqs(tag)) {
        reqs.push(reqId);
        if (reqs.size() == MAX_QUEUE_SIZE) {
            break;
        }
    }
}

static void fillQueueUnordered(const std::string& tag, const boost::posix_time::ptime&, std::queue<bgnd::BgndReqId>& reqs)
{
    static const std::queue<bgnd::BgndReqId>::size_type MAX_QUEUE_SIZE = 128;
    bgnd::Storage& s = bgnd::Storage::instance();
    for (const bgnd::BgndReqId& reqId : s.readAllReqsUnordered(tag)) {
        reqs.push(reqId);
        if (reqs.size() == MAX_QUEUE_SIZE) {
            break;
        }
    }
}

static void delReqs__(bgnd::Storage& s, const bgnd::BgndReqId& id, bool unordered)
{
    if (unordered) {
        s.delPrevOrEqualReqs(id);
    } else {
        s.delReq(id);
    }
}

class BgndReqFailed
    : public Posthooks::BaseHook
{
public:
    BgndReqFailed(bgnd::BgndRequestTask& task, const boost::posix_time::ptime& tm, const bgnd::Request& req, bool unordered)
        : task_(task), tm_(tm), req_(req), unordered_(unordered)
    {}
    virtual BgndReqFailed* clone() const {
        return new BgndReqFailed(*this);
    }
    virtual void run() {
        bgnd::Storage& s= bgnd::Storage::instance();
        if (const Message& msg = task_.handleError(tm_, req_)) {
            LogTrace(TRACE0) << "Request [" << req_.req << "]";
            LogError(STDLOG) << "Handle bgnd request " << req_.id << "[" << req_.attempt << "] failed: " << msg;
            s.addError(req_.id, req_.parCmdId, msg.toString(UserLanguage::en_US()));
        }
        delReqs__(s, req_.id, unordered_);
        ServerFramework::AttemptsCounter::getCounter("BGND").dropAttempt(req_.id.get());
        s.commit();
    }
private:
    virtual bool less2(const Posthooks::BaseHook* ptr) const noexcept {
        const BgndReqFailed* p = dynamic_cast<const BgndReqFailed*>(ptr);
        return req_.id < p->req_.id;
    }

private:
    bgnd::BgndRequestTask& task_;
    const boost::posix_time::ptime tm_;
    bgnd::Request req_;
    bool unordered_;
};

} // unnamed

namespace bgnd
{

Storage::~Storage()
{}

Storage& Storage::instance()
{
    ASSERT(storage != nullptr && "BgndStorage is not set");
    return *storage;
}

void Storage::setupStorage(Storage* s)
{
    storage.reset(s);
}

Request::Request(const BgndReqId& id_, const ct::UserId& uid_, const ct::CommandId& cmd_, const std::string& req_, unsigned a_)
    : id(id_), uid(uid_), parCmdId(cmd_), req(req_), attempt(a_)
{
}

Error::Error(const BgndReqId& id_, const ct::CommandId& cmd_, const std::string& s_)
    : id(id_), parCmdId(cmd_), errText(s_)
{
}

std::ostream& operator<<(std::ostream& os, const Request& r)
{
    return os << r.id << " " << r.uid << " " << r.parCmdId << " "
        << r.attempt << " [" << r.req << "]";
}

boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId& uid, const ct::CommandId& parCmdId,
                                  const std::string& req, const std::string& socketName)
{
    if (auto id = Storage::instance().addReq(tag, uid, parCmdId, req)) {
        if (tag != "QUEUES") {
            Posthooks::sethAfter(ServerFramework::DaemonKicker(socketName));
        }
        return id;
    }
    return boost::none;
}

boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId& uid, const ct::CommandId& cmdId, const std::string& req)
{
    static const std::string socketName("CMD_BGND");
    return addReq(tag, uid, cmdId, req, socketName);
}

void delErrors(const ct::CommandId& parCmdId)
{
    Storage::instance().delErrors(parCmdId);
}

BgndRequestTask::BgndRequestTask(const std::string& tag, const ServerFramework::DaemonTaskTraits& traits)
    : ServerFramework::CyclicDaemonTask<BgndReqId>(traits), unordered_(false)
{
    fillQueueFunc_ = std::bind(fillQueue, tag, std::placeholders::_1, std::placeholders::_2);
}

BgndRequestTask::BgndRequestTask(const std::string& tag, bool unordered, const ServerFramework::DaemonTaskTraits& traits)
    : ServerFramework::CyclicDaemonTask<BgndReqId>(traits), unordered_(unordered)
{
    fillQueueFunc_ = unordered_
        ? std::bind(fillQueueUnordered, tag, std::placeholders::_1, std::placeholders::_2)
        : std::bind(fillQueue, tag, std::placeholders::_1, std::placeholders::_2);
}

Message BgndRequestTask::handleError(const boost::posix_time::ptime& tm, const bgnd::Request& req) const
{
    return Message(STDLOG, _("Bgnd request %1% failed.")).bind(req.id.get());
}

int BgndRequestTask::run(const boost::posix_time::ptime& tm, const BgndReqId& id, bool)
{
    monitor_set_request((name() + ": " + id.get()).c_str());

    LogTrace(TRACE5) << "BgndRequestTask::run " << id;
    Storage& s = Storage::instance();
    boost::optional<bgnd::Request> req(s.readReq(id));
    if (!req) {
        LogError(STDLOG) << "BgndRequest::read failed: " << id;
        delReqs__(s, id, unordered_);
        return 0;
    }
    Posthooks::sethRollb(BgndReqFailed(*this, tm, *req, unordered_));
    CutLogLevel llHolder;
    auto& counter = ServerFramework::AttemptsCounter::getCounter("BGND");
    const auto attempt = counter.makeAttempt(id.get());
    counter.dropAttempt(id.get());
    if (attempt == ServerFramework::AttemptState::LastAttempt) {
        static const int LOG_LEVEL = 19;
        llHolder.reset(LOG_LEVEL);
    } else if (attempt == ServerFramework::AttemptState::AllAttemptsExhausted) {
        LogTrace(TRACE1) << *req;
        std::string error("ALL ATTEMPTS TO PROCESS FAILED FOR BGND REQUEST: ");
        error += id.get();
        LogError(STDLOG) << error;
        s.addError(id, req->parCmdId, error);
        delReqs__(s, id, unordered_);
        return 0;
    } else if (attempt == ServerFramework::AttemptState::TotallyBroken) {
        LogTrace(TRACE1) << *req;
        LogError(STDLOG) << "addError FAILED FOR BGND REQUEST " << id << " - SKIPPING IT";
        delReqs__(s, id, unordered_);
        return 0;
    }

    bgndErr_ = run(tm, *req);
    if (bgndErr_) {
        LogTrace(TRACE0) << "BgndError : " << id << ", " << req->parCmdId << ", err: " << bgndErr_;
        return 1;
    }
    delReqs__(s, id, unordered_);
    return 0;
}

void setupOracleStorage()
{
    ServerFramework::setupAttemptCounter("BGND",
            [](){ make_curs("DELETE FROM PROC_ATTEMPTS PA"
                " WHERE PROC_TYPE = 'BGND'"
                " AND NOT EXISTS(SELECT 1 FROM BGND_REQS BR"
                "   WHERE PA.ID = BR.ID)").exec(); });
    Storage::setupStorage(new OracleStorage);
}

void setupPgStorage()
{
    static PgCpp::SessionDescriptor sd = PgCpp::getManagedSession(
            readStringFromTcl("MAIN(PG_CONNECT_STRING)"));
    ServerFramework::setupAttemptCounter("BGND",
            [](){ make_pg_curs(sd, "DELETE FROM PROC_ATTEMPTS PA"
                " WHERE PROC_TYPE = 'BGND'"
                " AND NOT EXISTS(SELECT 1 FROM BGND_REQS BR"
                "   WHERE PA.ID = BR.ID)").exec(); }, sd);
    Storage::setupStorage(new PgStorage(sd));
}

void cleanBgndReqs() {
    Storage::instance().delAllReqs();
}

} // bgnd

#ifdef XP_TESTING
#include <serverlib/xp_test_utils.h>
#include <serverlib/query_runner.h>
#include <serverlib/checkunit.h>

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

void init_bgnd_tests()
{}

namespace
{

using namespace bgnd;

const std::string TEST_TAG = "BGND";
const ct::UserId tstUserId("auth00000000001");
const ct::CommandId tstCmdId("cmd000000000001");

void setupOra()
{
    testInitDB();
    bgnd::setupOracleStorage();
}

void setupPg()
{
    testInitDB();
    bgnd::setupPgStorage();
}

START_TEST(check_bgnd_reqs)
{
    bgnd::Storage& s = Storage::instance();
    const boost::posix_time::ptime t = Dates::currentDateTime();
    const std::string req("Request Text");
    const ct::UserId uid = tstUserId;
    const ct::CommandId cmdId = tstCmdId;
    std::queue<BgndReqId> ids;

    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.empty());

    boost::optional<bgnd::BgndReqId> reqId = addReq(TEST_TAG, uid, cmdId, req);
    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.size() == 1);

    boost::optional<Request> tmpReq(s.readReq(*reqId));
    fail_unless((bool)tmpReq == true);

    ids = std::queue<BgndReqId>();
    s.delReq(*reqId);
    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.size() == 0);

    tmpReq = s.readReq(*reqId);
    fail_unless((bool)tmpReq == false);
} END_TEST

START_TEST(check_bgnd_req_parts)
{
    bgnd::Storage& s = Storage::instance();
    std::string req("Request Text");
    while (req.size() < 4000) {
        req += req;
    }
    const ct::UserId uid = tstUserId;
    const ct::CommandId cmdId = tstCmdId;
    boost::optional<bgnd::BgndReqId> reqId = s.addReq(TEST_TAG, uid, cmdId, req);
    fail_unless(static_cast<bool>(reqId) == true, "addReq failed");

    boost::optional<bgnd::Request> outReq = s.readReq(*reqId);
    fail_unless(static_cast<bool>(outReq) == true, "readReq failed");
    fail_unless(outReq->req == req, "invalid req text after read");

    s.delReq(*reqId);
    outReq = s.readReq(*reqId);
    fail_unless(static_cast<bool>(outReq) == false, "delReq failed");
} END_TEST

START_TEST(check_bgnd_errors)
{
    bgnd::Storage& s = bgnd::Storage::instance();
    const ct::CommandId cmdId1("cmd000000000001");
    const ct::CommandId cmdId2("cmd000000000002");
    s.addError(s.nextBgndId(), cmdId1, "cmd 1 error");
    s.addError(s.nextBgndId(), cmdId2, "cmd 2 error");
    s.addError(s.nextBgndId(), cmdId1, "cmd 1 error");
    s.addError(s.nextBgndId(), cmdId2, "cmd 2 error");
    s.addError(s.nextBgndId(), cmdId1, "cmd 1 error");

    const std::vector<Error> errs1(s.readErrors(cmdId1));
    const std::vector<Error> errs2(s.readErrors(cmdId2));

    fail_unless(errs1.size() == 3, "invalid cmd1 errors: %zd", errs1.size());
    fail_unless(errs2.size() == 2, "invalid cmd2 errors: %zd", errs2.size());
} END_TEST

enum class BgndCheckTaskAction { Ok, Fail };
static BgndCheckTaskAction runAction = BgndCheckTaskAction::Ok;
static BgndCheckTaskAction errAction = BgndCheckTaskAction::Ok;
static std::string lastError;

class BgndCheckTask : public bgnd::BgndRequestTask
{
public:
    BgndCheckTask()
        : bgnd::BgndRequestTask(TEST_TAG, ServerFramework::DaemonTaskTraits::OracleAndHooks())
    {}

    Message run(const boost::posix_time::ptime&, const bgnd::Request& r) override {
        LogError(STDLOG) << r << " action=" << static_cast<int>(runAction);
        switch (runAction) {
        case BgndCheckTaskAction::Ok: return Message();
        case BgndCheckTaskAction::Fail: return Message(STDLOG, _("::run fails"));
        }
        return Message(STDLOG, _("unknown action"));
    }

    Message handleError(const boost::posix_time::ptime& tm, const bgnd::Request& r) const override {
        LogError(STDLOG) << r << " action=" << static_cast<int>(errAction);
        switch (errAction) {
        case BgndCheckTaskAction::Ok: {
            lastError = "";
            return Message();
        }
        case BgndCheckTaskAction::Fail: {
            lastError = "::handleError fails";
            return Message(STDLOG, _("::handleError fails"));
        }
        }
        return Message(STDLOG, _("unknown action"));
    }
};

START_TEST(check_bgnd_attempts)
{
    ServerFramework::AttemptsCounter::useDb(true);
    const ct::UserId uid = tstUserId;
    const ct::CommandId cmdId = tstCmdId;
    boost::optional<bgnd::BgndReqId> reqId = addReq(TEST_TAG, uid, cmdId, "Good request");
    auto& counter = ServerFramework::AttemptsCounter::getCounter("BGND");
    ServerFramework::AttemptState attempt = ServerFramework::AttemptState::TotallyBroken;
    for (int i = 0; i < 2; ++i) {
        LogTrace(TRACE5) << "makeAttempt " << (i + 1);
        attempt = counter.makeAttempt(reqId->get());
        fail_unless(attempt == ServerFramework::AttemptState::Fine,
                "attempt state is %d instead of Fine(%d)",
                attempt, ServerFramework::AttemptState::Fine);
    }
    attempt = counter.makeAttempt(reqId->get());
    fail_unless(attempt == ServerFramework::AttemptState::LastAttempt,
            "attempt state is %d instead of LastAttempt(%d)",
            attempt, ServerFramework::AttemptState::LastAttempt);
    attempt = counter.makeAttempt(reqId->get());
    fail_unless(attempt == ServerFramework::AttemptState::AllAttemptsExhausted,
            "attempt state is %d instead of AllAttemptsExhausted(%d)",
            attempt, ServerFramework::AttemptState::AllAttemptsExhausted);
    for (int i = 0; i < 10; ++i) {
        LogTrace(TRACE5) << "makeAttempt " << (i + 1);
        attempt = counter.makeAttempt(reqId->get());
        fail_unless(attempt == ServerFramework::AttemptState::TotallyBroken,
                "attempt state is %d instead of TotallyBroken(%d)",
                attempt, ServerFramework::AttemptState::TotallyBroken);
    }
} END_TEST

START_TEST(check_bgnd)
{
    const boost::posix_time::ptime t = Dates::currentDateTime();
    const ct::UserId uid = tstUserId;
    const ct::CommandId cmdId = tstCmdId;
    std::queue<BgndReqId> ids;
    ServerFramework::DaemonTaskPtr p(new BgndCheckTask);

    boost::optional<bgnd::BgndReqId> reqId = addReq(TEST_TAG, uid, cmdId, "Good request");
    runAction = BgndCheckTaskAction::Ok;
    ServerFramework::runTask(p, Dates::currentDateTime());
    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.empty());

    reqId = addReq(TEST_TAG, uid, cmdId, "Request with error");
    runAction = BgndCheckTaskAction::Fail;
    errAction = BgndCheckTaskAction::Ok;
    ServerFramework::runTask(p, Dates::currentDateTime());
    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.empty());
    fail_unless(lastError == "");

    reqId = addReq(TEST_TAG, uid, cmdId, "Bad request and bad error handle");
    runAction = BgndCheckTaskAction::Fail;
    errAction = BgndCheckTaskAction::Fail;
    ServerFramework::runTask(p, Dates::currentDateTime());
    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.empty());
    LogTrace(TRACE5) << lastError;
    fail_unless(lastError == "::handleError fails");

} END_TEST

#define SUITENAME "bgndOra"
TCASEREGISTER(setupOra, 0)
{
    // Закомментрировано в Астре
//    ADD_TEST(check_bgnd_reqs);
//    ADD_TEST(check_bgnd_req_parts);
//    ADD_TEST(check_bgnd_errors);
//    ADD_TEST(check_bgnd_attempts);
//    ADD_TEST(check_bgnd);
}
TCASEFINISH
#undef SUITENAME
#define SUITENAME "bgndPg"
TCASEREGISTER(setupPg, 0)
{
    // Закомментрировано в Астре
//    ADD_TEST(check_bgnd_reqs);
//    ADD_TEST(check_bgnd_req_parts);
//    ADD_TEST(check_bgnd_errors);
//    ADD_TEST(check_bgnd_attempts);
//    ADD_TEST(check_bgnd);
}
TCASEFINISH

} // namespace
#endif // XP_TESTING
