
#if HAVE_CONFIG_H
#endif

#include <typeinfo>
#include <vector>
#include <functional>
#include <sys/types.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "daemon_task.h"
#include "tclmon.h"
#include "tcl_utils.h"
#include "testmode.h"
#include "ocilocal.h"
#include "posthooks.h"
#include "monitor_ctl.h"
#include "query_runner.h"
#include "ourtime.h"
#include "sirenaproc.h"
#include "cursctl.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

static void commitWithHooks()
{
    ProgTrace( TRACE1 , "Commit" );
    callPostHooksBefore();
    ServerFramework::applicationCallbacks()->commit_db();
    callPostHooksCommit();
    callPostHooksAfter();
}

static void commit1()
{
    ServerFramework::applicationCallbacks()->commit_db();
}

static void rollback1()
{
    ServerFramework::applicationCallbacks()->rollback_db();
}

namespace ServerFramework
{

DaemonTaskTraits DaemonTaskTraits::Nothing()
{
    return DaemonTaskTraits(0, 0, 0, 0);
}
DaemonTaskTraits DaemonTaskTraits::OracleAndHooks()
{
    return DaemonTaskTraits(emptyHookTables, commitWithHooks, rollback1, callPostHooksAlways);
}
DaemonTaskTraits DaemonTaskTraits::OracleOnly()
{
    return DaemonTaskTraits(0, commit1, rollback1, 0);
}

static void callCommitPostHooks()
{
    callPostHooksBefore();
    callPostHooksCommit();
    callPostHooksAfter();
}

DaemonTaskTraits DaemonTaskTraits::HooksOnly()
{
    return DaemonTaskTraits(emptyHookTables, callCommitPostHooks, callRollbackPostHooks, callPostHooksAlways);
}

size_t BaseDaemonTask::timeout() const
{
    return 600; // default 180 seconds for every task
}

void BaseDaemonTask::monitorRequest()
{
    monitor_idle_zapr(1);
}

std::string BaseDaemonTask::name()
{
     return typeid(*this).name() ;
}

} // namespace ServerFramework

#ifdef XP_TESTING
#include <queue>
#include "xp_test_utils.h"
#include "new_daemon.h"
#include "daemon_event.h"
#include "http_server.h"
#include "deadlock_exception.h"
#include "pg_cursctl.h"

#include "checkunit.h"
#include "log_manager.h"

namespace {

using namespace ServerFramework;

#ifdef ENABLE_PG
PgCpp::SessionDescriptor pgReadOnly = nullptr;
PgCpp::SessionDescriptor pgManaged = nullptr;
#endif//ENABLE_PG

class TestDaemon
     : public NewDaemon
{
public:
    TestDaemon()
        : NewDaemon("DAEMON")
    {}
    virtual ~TestDaemon() {}
    virtual void init() {
        ProgError(STDLOG, "TestDaemon::init");
    }
};

static size_t tasks = 0;
struct Foo
{
    int fld;
};

class TestCyclicTask
    : public CyclicDaemonTask<Foo>
{
public:
    TestCyclicTask()
        : CyclicDaemonTask<Foo>(ServerFramework::DaemonTaskTraits::Nothing())
    {
        fillQueueFunc_ = &fillQueue;
    }
private:
    virtual int run(boost::posix_time::ptime const&, const Foo& foo, bool endOfData) {
        std::ostringstream oss;
        oss << foo.fld;
        LogLevelHolder llHolder("foo", oss.str());

        LogTrace(TRACE1) << "processing Foo: " << foo.fld << " endOfData=" << endOfData;
        LogTrace(TRACE5) << "LogTrace(TRACE5)";
        LogTrace(TRACE0) << "LogTrace(TRACE0)";
        LogError(STDLOG) << "LogError(STDLOG)";
        LogWarning(STDLOG) << "LogWarnig(STDLOG)";
        LogTrace(TRACE5) << "LogTrace(TRACE5)";
        ++tasks;
        return 0;
    }
    static void fillQueue(boost::posix_time::ptime const&, std::queue<Foo>& queue) {
        static size_t cnt = 3;
        Foo f = {2};
        if (cnt) {
            for (size_t i = 0; i < cnt; ++i) {
                f.fld = 2 * i;
                queue.push(f);
            }
            --cnt;
        }
        LogTrace(TRACE5) << "fillQueue: " << queue.size();
    }
};

START_TEST(cycleTask)
{
    tasks = 0;
    DaemonEventPtr ev(new TimerDaemonEvent(1));
    ev->addTask(DaemonTaskPtr(new TestCyclicTask));
    LogManager::Instance().load("foo", "2");
    for (size_t i = 0; i < 10; ++i) {
        ev->runTasks();
    }
    fail_unless(tasks == 6, "invalid tasks: %zd", tasks);
}END_TEST

class SetTestResult: public Posthooks::BaseHook
{
public:
    SetTestResult(const std::string& v) : v_(v) {}
    virtual ~SetTestResult() {}
    virtual void run();
    virtual SetTestResult* clone() const {
        return new SetTestResult(*this);
    }
    virtual bool less2(const BaseHook*h) const noexcept {
        return v_ < dynamic_cast<const SetTestResult&> (*h).v_;
    }
private:
    const std::string v_;
};

class TestSetHooks : public DaemonTask
{
public:
    static int failFlag; // 0 - ok, 1 - error, 2 - throw

    static std::string hooksResult;

    TestSetHooks() : DaemonTask(ServerFramework::DaemonTaskTraits::HooksOnly())
    {}
private:
    virtual int run(boost::posix_time::ptime const&) {
        LogTrace(TRACE5) << "run";
        sethBefore(SetTestResult("BEFORE"));
        sethAfter(SetTestResult(" AFTER"));
        sethAlways(SetTestResult(" ALWAYS"));
        sethRollb(SetTestResult(" ROLLBACK"));
        sethCommit(SetTestResult(" COMMIT"));
        if (failFlag == 2) {
            throw comtech::Exception("Lol");
        } else if (failFlag == 1) {
            return 1;
        }
        return 0;
    }
};
int TestSetHooks::failFlag = 0;
std::string TestSetHooks::hooksResult;

void SetTestResult::run()
{
    TestSetHooks::hooksResult += v_;
}

START_TEST(daemon_sethooks)
{
    DaemonEventPtr ev(new TimerDaemonEvent(1));
    TestSetHooks::failFlag = 0; // work without errors
    ev->addTask(DaemonTaskPtr(new TestSetHooks));
    ev->runTasks();
    fail_unless(TestSetHooks::hooksResult == "BEFORE COMMIT AFTER ALWAYS", "invalid result: [%s]", TestSetHooks::hooksResult.c_str());
    TestSetHooks::failFlag = 1; // return error
    TestSetHooks::hooksResult.clear();
    ev->runTasks();
    fail_unless(TestSetHooks::hooksResult == " ROLLBACK ALWAYS", "invalid result: [%s]", TestSetHooks::hooksResult.c_str());
    TestSetHooks::failFlag = 2; // throw
    TestSetHooks::hooksResult.clear();
    ev->runTasks();
    fail_unless(TestSetHooks::hooksResult == " ROLLBACK ALWAYS", "invalid result: [%s]", TestSetHooks::hooksResult.c_str());
} END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(cycleTask);
    ADD_TEST(daemon_sethooks);
}TCASEFINISH

static std::string pgConnStr()
{
    return readStringFromTcl("PG_CONNECT_STRING");
}

#ifdef ENABLE_PG
class TestPgReconnectTask
    : public ServerFramework::DaemonTask
{
public:
    TestPgReconnectTask()
        : ServerFramework::DaemonTask(ServerFramework::DaemonTaskTraits::OracleAndHooks())
    {}
private:
    virtual int run(boost::posix_time::ptime const&) {
        int v = 0;
        make_pg_curs(pgReadOnly, "SELECT FLD1 FROM TEST_FOR_UPDATE")
            .def(v)
            .exfet();
        LogError(STDLOG) << "selected " << v;
        // start task then drop PG connection
        // select * from pg_stat_activity where client_addr='127.0.0.1';
        // select pg_terminate_backend(pid)  from pg_stat_activity where pid=27345;
        // session should throw once and then reconnect again
        return 0;
    }
};

class TestPgAutoDisconnectTask
    : public ServerFramework::DaemonTask
{
public:
    TestPgAutoDisconnectTask()
        : ServerFramework::DaemonTask(ServerFramework::DaemonTaskTraits::OracleAndHooks())
    {}
private:
    virtual int run(boost::posix_time::ptime const& t) {
        if (t.time_of_day().minutes() % 2) {
            LogTrace(TRACE5) << "skip task every odd minute: " << t.time_of_day().minutes();
            return 0;
        }
        int v = 0;
        make_pg_curs(pgManaged, "SELECT 2")
            .def(v)
            .EXfet();
        ASSERT(v == 2);
        LogError(STDLOG) << "selected " << v;
        return 0;
    }
};

class TestPgThrowRollback
    : public ServerFramework::DaemonTask
{
public:
    TestPgThrowRollback()
        : ServerFramework::DaemonTask(ServerFramework::DaemonTaskTraits::OracleAndHooks())
    {}
private:
    virtual int run(boost::posix_time::ptime const&) {
        int v = 1;
        make_pg_curs(pgManaged, "SELECT COUNT(*) FROM TEST_NOTHROW")
            .def(v)
            .exfet();
        ASSERT(v == 0, "expecting throw due to constraint violation and then rollback")
        LogError(STDLOG) << "good selected " << v << " rows";
        make_pg_curs(pgManaged, "INSERT INTO TEST_NOTHROW (FLD1) VALUES (:v)")
            .bind(":v", v)
            .exec();
        make_pg_curs(pgManaged, "INSERT INTO TEST_NOTHROW (FLD1) VALUES (:v)")
            .bind(":v", v)
            .exec();
        return 0;
    }
};

class TestPgBadSqlTask
    : public ServerFramework::DaemonTask
{
public:
    TestPgBadSqlTask(int t)
        : ServerFramework::DaemonTask(ServerFramework::DaemonTaskTraits::OracleAndHooks()),
        type_(t), sd_(nullptr)
    {}
private:
    virtual int run(boost::posix_time::ptime const&) {
        int v = 1;
        if (sd_ == nullptr) {
            switch (type_) {
            case 1: {
                sd_ = PgCpp::getManagedSession(pgConnStr());
                break;
            }
            case 2: {
                sd_ = PgCpp::getReadOnlySession(pgConnStr());
                break;
            }
            case 3: {
                sd_ = PgCpp::getAutoCommitSession(pgConnStr());
                break;
            }
            }
        }
        make_pg_curs(sd_, "SELECT COUNT(*) FROM TEST_NOTHROW")
            .def(v)
            .exfet();
        ASSERT(v == 0, "expecting throw due to bad SQL then rollback and on the next run pg-session should be fine again");
        LogError(STDLOG) << "good selected " << v << " rows";
        make_pg_curs(sd_, "INSERT INTO TEST_UNKNOWN_TABLE (FLD1) VALUES (:v)")
            .bind(":v", v)
            .exec();
        return 0;
    }
    int type_;
    PgCpp::SessionDescriptor sd_;
};
#endif//ENABLE_PG

class TimerDaemonTask
    : public ServerFramework::DaemonTask
{
public:
    TimerDaemonTask()
        : ServerFramework::DaemonTask(ServerFramework::DaemonTaskTraits::Nothing())
    {}
private:
    virtual int run(boost::posix_time::ptime const&) {
        LogTrace(TRACE1) << "var: [" << readStringFromTcl("LOLKA", "") << "]";
        std::string astr(1000000, 'a');
        LogTrace(TRACE5) << astr;
        return 0;
    }
    virtual bool doNeedRepeat() {
        return true;
    }
};
class TestHttpServer
    : public HttpServer
{
public:
    TestHttpServer()
        : HttpServer("0.0.0.0", "8888")
    {}
    virtual ~TestHttpServer()
    {}
    virtual void handleRequest(const std::string& requestPath, const http::request& req, http::reply& rep) {
        ProgError(STDLOG, "handleRequest: %s", requestPath.c_str());
        for (size_t i = 0; i < req.headers.size(); ++i) {
            ProgError(STDLOG, "%s : %s", req.headers[i].name.c_str(), req.headers[i].value.c_str());
        }
        HttpServer::returnStatics("./" + requestPath, req, rep);
    }
};

class EdiCheckTask :
    public ServerFramework::BaseDaemonTask
{
public:
    explicit EdiCheckTask(const ServerFramework::DaemonTaskTraits& tr) :
        BaseDaemonTask(tr)
    {
        connect2DB();
    }

    int run(const char* buff, size_t buffSz, const boost::posix_time::ptime&) {
        //EdiHelpManager::confirm_notify("МОВКАМ", 1);
        uint8_t buf[1024] = {};
        unsigned short binlen;
        std::string address;
        std::string text;
        std::string sessionId;
        OciCpp::CursCtl cur(make_curs("SELECT "
                                            "INTMSGID, ADDRESS, TEXT, SESSION_ID FROM EDI_HELP "
                                      "")
        );

        cur.autoNull()
            .defFull(buf + sizeof(uint32_t), 3 * sizeof(uint32_t), 0, &binlen, SQLT_BIN)
            .def(address)
            .def(text)
            .def(sessionId)
            .exfet();

        LogTrace(TRACE5) << "Rowcount: " << cur.rowcount();
        if (cur.rowcount()) {
            uint32_t tmp = htonl(1);
            memcpy(buf, &tmp, sizeof(uint32_t));
            memcpy(buf + sizeof(uint32_t) + binlen, text.c_str(), text.size());
            send_signal(address.c_str(), buf, sizeof(uint32_t) + binlen + text.size());
            LogTrace(TRACE5) << "Signal sent";

            make_curs("DECLARE\nPRAGMA AUTONOMOUS_TRANSACTION;\n"
                                   "BEGIN\n"
                                       "DELETE FROM EDI_HELP;\n"
                                       "COMMIT;\n"
                                   "END;")
                .exec();
        }
        return 0;
    }
};

} // namespace

int main_test_daemon(int supervisorSocket, int argc, char *argv[])
{
    NewDaemon d("DAEMON");
    DaemonEventPtr timerEv(new TimerDaemonEvent(5));
    timerEv->addTask(DaemonTaskPtr(new TestCyclicTask));
    //timerEv->addTask(DaemonTaskPtr(new TimerDaemonTask));
#ifdef ENABLE_PG
    timerEv->addTask(DaemonTaskPtr(new TestPgReconnectTask));
    timerEv->addTask(DaemonTaskPtr(new TestPgAutoDisconnectTask));
#endif//ENABLE_PG
    //timerEv->addTask(DaemonTaskPtr(new TestPgThrowRollback));
    //timerEv->addTask(DaemonTaskPtr(new TestPgBadSqlTask(1)));
    //timerEv->addTask(DaemonTaskPtr(new TestPgBadSqlTask(2)));
    //timerEv->addTask(DaemonTaskPtr(new TestPgBadSqlTask(3)));
    d.addEvent(timerEv);
    d.addEvent(DaemonEventPtr(new TestHttpServer));
#ifdef ENABLE_PG
    pgReadOnly = PgCpp::getReadOnlySession(pgConnStr());
    pgManaged = PgCpp::getManagedSession(pgConnStr());
    make_pg_curs(pgManaged, "DELETE FROM TEST_NOTHROW").exec();
    PgCpp::commit();
#endif//ENABLE_PG
    d.init();
    d.run();
    return 0;
}

int main_edi_help_checker_daemon(int supervisorSocket, int argc, char *argv[])
{
    NewDaemon d("EDI_CHECKER");

    DaemonEventPtr timer(new TimerDaemonEvent(1));
    timer->addTask(DaemonTaskPtr(new EdiCheckTask(DaemonTaskTraits::Nothing())));
    d.addEvent(timer);
    d.init();
    d.run();

    return 0;
}

#endif // XP_TESTING
