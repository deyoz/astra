#include <set>

#include "bgnd.h"
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
#include "lngv_user.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

static OciCpp::OciSession* sess = NULL;
static const size_t MaxPartSize = 4000;

namespace
{

static void fillQueue(const std::string& tag, const boost::posix_time::ptime&, std::queue<bgnd::BgndReqId>& reqs)
{
    static const std::queue<bgnd::BgndReqId>::size_type MAX_QUEUE_SIZE = 128;

    LogTrace( TRACE5 ) << __FUNCTION__ << ". tag: " << tag;
    std::string id;
    std::set<bgnd::BgndReqId> tmpReqs;
    OciCpp::CursCtl cr(sess->createCursor(STDLOG, "SELECT ID FROM BGND_REQS WHERE TAG = :tag"));
    cr.bind( ":tag", tag ).def(id).exec();
    while (!cr.fen()) {
        tmpReqs.insert(bgnd::BgndReqId(id));
    }

    std::set<bgnd::BgndReqId>::const_iterator it(tmpReqs.begin());
    for ( ;(tmpReqs.end() != it) && (reqs.size() < MAX_QUEUE_SIZE); ++it) {
        reqs.push(*it);
    }
}

bgnd::BgndReqId nextBgndId()
{
    static const std::string prefix("bgnd");
    return bgnd::BgndReqId(HelpCpp::convertToId( HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
}

void addError(const bgnd::BgndReqId& id, const ct::CommandId& parCmdId, const std::string& errText)
{
    try {
        sess->createCursor(STDLOG, "INSERT INTO BGND_ERRS(ID, PARENT_ID, ERR_TEXT)"
            "VALUES (:id_, :cmdId, :err)")
          .bind(":id_", id).bind(":cmdId", parCmdId).bind(":err", errText)
          .exec();
    } catch (const OciCpp::ociexception& e) {
        LogTrace(TRACE0) << "reqId=" << id << " parCmdId=" << parCmdId << " errText=[" << errText << "]";
        LogError(STDLOG) << "addBgndError failed: " << e.what();
    }
}

boost::optional<bgnd::Request> readReq(const bgnd::BgndReqId& id)
{
    boost::optional<bgnd::Request> req;
    unsigned attempt = 0;
    size_t parts;
    std::string uid, cmd, reqText;
    //TODO increase attempt
    sess->createCursor(STDLOG, "SELECT ATTEMPT, PARENT_ID, REQ_TEXT, USER_ID, PARTS FROM BGND_REQS WHERE ID = :id_")
        .bind(":id_", id)
        .def(attempt).def(cmd).def(reqText).def(uid).def(parts)
        .EXfet();
    if (parts > 1) {
        std::string reqPart;
        size_t oldPartNum = 0, partNum = 0;
        auto curs = sess->createCursor(STDLOG, "SELECT REQ_TEXT, PART FROM BGND_REQS_PARTS WHERE ID = :id_");
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

void delReq(const bgnd::BgndReqId& id)
{
    LogTrace(TRACE5) << "delReq " << id;
    sess->createCursor(STDLOG, "DELETE FROM BGND_REQS WHERE ID = :id_")
        .bind(":id_", id)
        .exec();
    sess->createCursor(STDLOG, "DELETE FROM BGND_REQS_PARTS WHERE ID = :id_")
        .bind(":id_", id)
        .exec();
}

class BgndReqFailed
    : public Posthooks::BaseHook
{
public:
    BgndReqFailed(bgnd::BgndRequestTask& task, const boost::posix_time::ptime& tm, const bgnd::Request& req)
        : task_(task), tm_(tm), req_(req)
    { }
    virtual BgndReqFailed* clone() const {
        return new BgndReqFailed(*this);
    }
    virtual void run() {
        if (const Message& msg = task_.handleError(tm_, req_)) {
            LogError(STDLOG) << "Handle bgnd request " << req_.req << " failed: " << msg; 
            addError(req_.id, req_.parCmdId, msg.toString(UserLanguage::en_US()));
        }
        delReq(req_.id);
        commit();
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
};

} // unnamed

namespace bgnd
{

Request::Request(const BgndReqId& id_, const ct::UserId& uid_, const ct::CommandId& cmd_, const std::string& req_, unsigned a_)
    : id(id_), uid(uid_), parCmdId(cmd_), req(req_), attempt(a_)
{
}

Error::Error(const BgndReqId& id_, const ct::CommandId& cmd_, const std::string& s_)
    : id(id_), parCmdId(cmd_), errText(s_)
{
}

void setSession(OciCpp::OciSession* s)
{
    sess = s;
}

std::ostream& operator<<(std::ostream& os, const Request& r)
{
    return os << r.id << " " << r.uid << " " << r.parCmdId << " "
        << r.attempt << " [" << r.req << "]";
}

static std::vector<std::string> str_parts(std::string const &str, size_t psz)
{
    std::vector<std::string> ret;
    size_t const sz = str.size();
    for (size_t pos = 0; pos < sz; pos += psz) {
        ret.push_back(str.substr(pos, psz));
    }
    return ret;
}

boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId& uid, const ct::CommandId& parCmdId,
                                  const std::string& req, const std::string& socketName)
{
    const BgndReqId id(nextBgndId());
    const std::vector<std::string> reqStr(str_parts(req, MaxPartSize));
    try {
        sess->createCursor(STDLOG, "INSERT INTO BGND_REQS (ID, ATTEMPT, PARENT_ID, RDATE, REQ_TEXT, USER_ID, TAG, PARTS)"
                "VALUES (:id_, 0, :par, :rd, :req, :uid_, :tag, :parts)")
            .bind(":id_", id).bind(":par", parCmdId)
            .bind(":rd", boost::posix_time::microsec_clock::local_time())
            .bind(":req", reqStr.at(0)).bind(":uid_", uid).bind( ":tag", tag )
            .bind(":parts", reqStr.size())
            .exec();
        if (reqStr.size() > 1) {
            for (size_t i = 1, i_end = reqStr.size(); i != i_end; ++i) {
                sess->createCursor(STDLOG, "INSERT INTO BGND_REQS_PARTS(ID, REQ_TEXT, PART) "
                        "VALUES(:id_, :req, :part)")
                    .bind(":id_", id)
                    .bind(":req", reqStr.at(i))
                    .bind(":part", i)
                    .exec();
            }
        }
        Posthooks::sethAfter(ServerFramework::DaemonKicker(socketName));
    } catch (const OciCpp::ociexception& e) {
        LogError(STDLOG) << e.what();
        return boost::optional<BgndReqId>();
    }
    return id;
}

boost::optional<BgndReqId> addReq(const std::string& tag, const ct::UserId& uid, const ct::CommandId& cmdId, const std::string& req)
{
    static const std::string socketName("CMD_BGND");
    return addReq(tag, uid, cmdId, req, socketName);
}

Errors readErrors(const ct::CommandId& parCmdId)
{
    std::string id, err;
    Errors errors;
    OciCpp::CursCtl cr(sess->createCursor(STDLOG, "SELECT ID, ERR_TEXT FROM BGND_ERRS WHERE PARENT_ID = :id_"));
    cr.bind(":id_", parCmdId)
      .def(id).def(err)
      .exec();
    while (!cr.fen()) {
        errors.push_back(Error(BgndReqId(id), parCmdId, err));
    }
    return errors;
}

void delErrors(const ct::CommandId& parCmdId)
{
    sess->createCursor(STDLOG, "DELETE FROM BGND_ERRS WHERE PARENT_ID = :id_")
      .bind(":id_", parCmdId)
      .exec();
}

BgndRequestTask::BgndRequestTask(const std::string& tag, const ServerFramework::DaemonTaskTraits& traits)
    : ServerFramework::CyclicDaemonTask<BgndReqId>(traits)
{
    fillQueueFunc_ = std::bind(fillQueue, tag, std::placeholders::_1, std::placeholders::_2);
}

Message BgndRequestTask::handleError(const boost::posix_time::ptime& tm, const bgnd::Request& req) const
{
    return Message(STDLOG, _("Bgnd request %1% failed.")).bind(req.id.get());
}

int BgndRequestTask::run(const boost::posix_time::ptime& tm, const BgndReqId& id, bool)
{
    static const int BGND_ALREADY_PROCESSED = 20044;
    static const int BGND_LAST_ATTEMPT_TO_PROCESS = 20045;

    LogTrace(TRACE5) << "BgndRequestTask::run " << id;

    boost::optional<bgnd::Request> req(readReq(id));
    if (!req) {
        LogError(STDLOG) << "BgndRequest::read failed: " << id;
        delReq(id);
        return 0;
    }

    OciCpp::CursCtl cr = make_curs("BEGIN CHECK_BGND(:id_); END;");
    cr  .bind(":id_", id)
        .noThrowError(BGND_LAST_ATTEMPT_TO_PROCESS)
        .noThrowError(BGND_ALREADY_PROCESSED);

    cr.exec();

    CutLogLevel llHolder;
    if (BGND_LAST_ATTEMPT_TO_PROCESS == cr.err()) {
        static const int LOG_LEVEL = 19;
        llHolder.reset(LOG_LEVEL);
    } else if (BGND_ALREADY_PROCESSED == cr.err()) {
        std::string error("ALL ATTEMPTS TO PROCESS FAILED FOR BGND REQUEST: ");
        error += id.get();
        addError(id, req->parCmdId, error);
        delReq(id);

        LogError(STDLOG) << error;
        return 0;
    }


    Posthooks::sethRollb(BgndReqFailed(*this, tm, *req));
    bgndErr_ = run(tm, *req);
    if (bgndErr_) {
        LogTrace(TRACE0) << "BgndError : " << id << ", " << req->parCmdId << ", err: " << bgndErr_;
        return 1;
    }

    delReq(id);
    return 0;
}

#ifdef XP_TESTING
namespace external_tests {
const std::string TEST_TAG = "BGND";
const std::string prefix = "tst";

class TestRequestTask : public BgndRequestTask
{
public:

    TestRequestTask() : BgndRequestTask(TEST_TAG, ServerFramework::DaemonTaskTraits::OracleAndHooks())
    { }

    virtual Message run(const boost::posix_time::ptime&, const bgnd::Request&)
    {
        return Message();
    }
};

void check_bgnd_reqs()
{
    setSession(&OciCpp::mainSession());
    const boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());
    const std::string req("Request Text");
    const ct::UserId uid(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
    const ct::CommandId cmdId(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
    std::queue<BgndReqId> ids;

    fillQueue(TEST_TAG, t, ids);
    fail_unless(ids.empty());

    boost::optional<bgnd::BgndReqId> reqId = addReq(TEST_TAG, uid, cmdId, req);
    fillQueue(TEST_TAG, boost::posix_time::microsec_clock::local_time(), ids);
    fail_unless(ids.size() == 1);

    boost::optional<Request> tmpReq(readReq(*reqId));
    fail_unless((bool)tmpReq == true);

    ids = std::queue<BgndReqId>();
    delReq(*reqId);
    fillQueue(TEST_TAG, boost::posix_time::microsec_clock::local_time(), ids);
    fail_unless(ids.size() == 0);

    tmpReq = readReq(*reqId);
    fail_unless((bool)tmpReq == false);
}

void check_daemon_task()
{
    setSession(&OciCpp::mainSession());
    const ct::UserId uid(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
    const ct::CommandId cmdId(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));

    boost::optional<bgnd::BgndReqId> reqId1 = addReq(TEST_TAG, uid, cmdId, "Request 1");
    boost::optional<bgnd::BgndReqId> reqId2 = addReq(TEST_TAG, uid, cmdId, "Request 2");
    OciCpp::mainSession().commit();

    std::queue<BgndReqId> ids;
    fillQueue(TEST_TAG, boost::posix_time::microsec_clock::local_time(), ids);
    fail_unless(ids.size() == 2);

    ServerFramework::runTask(std::make_shared<TestRequestTask>()
                             , boost::posix_time::second_clock::local_time());

    ids = std::queue<bgnd::BgndReqId>();
    fillQueue(TEST_TAG, boost::posix_time::microsec_clock::local_time(), ids);
    fail_unless(ids.size() == 0);
}

void check_bgnd_errors()
{
    setSession(&OciCpp::mainSession());
    const ct::CommandId cmdId1(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
    const ct::CommandId cmdId2(HelpCpp::convertToId(HelpCpp::objId(), OBJ_ID_LEN - prefix.size(), prefix));
    addError(nextBgndId(), cmdId1, "cmd 1 error");
    addError(nextBgndId(), cmdId2, "cmd 2 error");
    addError(nextBgndId(), cmdId1, "cmd 1 error");
    addError(nextBgndId(), cmdId2, "cmd 2 error");
    addError(nextBgndId(), cmdId1, "cmd 1 error");

    const std::vector<Error> errs1(readErrors(cmdId1));
    const std::vector<Error> errs2(readErrors(cmdId2));

    fail_unless(errs1.size() == 3, "invalid cmd1 errors: %zd", errs1.size());
    fail_unless(errs2.size() == 2, "invalid cmd2 errors: %zd", errs2.size());
}
}
#endif

} // bgnd

