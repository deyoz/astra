#include "attempts.h"

#include <memory>
#include "new_daemon.h"
#include "daemon_event.h"
#include "daemon_task.h"
#include "cursctl.h"
#include "pg_cursctl.h"
#include "helpcpp.h"
#include "testmode.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace
{
using ServerFramework::AttemptsCounter;
using ServerFramework::AttemptState;
using AttemptsCounterPtr = std::unique_ptr<AttemptsCounter>;

// temporary solution to be able to move subsystems to pg independently
// replace this with single AttemptsCounter when migration is done
std::map<std::string, AttemptsCounterPtr> counters;
#ifdef XP_TESTING
static bool useDbForAttempts__ = false;
std::map<std::string, AttemptsCounterPtr> countersForTests;
#endif // XP_TESTING

void check(const std::string& type, std::function<void ()> f)
{
    auto it = counters.find(type);
    if (it != counters.end()) {
        LogError(STDLOG) << "AttemptsCounter is already set for type [" << type << ']';
        throw ServerFramework::Exception(STDLOG, __FUNCTION__,
                "AttemptsCounter is already set for type [" + type + "]");
    }
    if (!f) {
        LogError(STDLOG) << "AttemptsCounter clear function is empty for type [" << type << ']';
        throw ServerFramework::Exception(STDLOG, __FUNCTION__,
                "AttemptsCounter clear function is empty for type [" + type + "]");
    }
}

class OraAttemptsCounter : public AttemptsCounter
{
public:
    OraAttemptsCounter(const std::string& type, std::function<void ()> fClear)
        : AttemptsCounter(type, fClear)
    {}

    AttemptState makeAttempt(const std::string& id) override {
        int attempts = 0;
        OciCpp::CursCtl cr = make_curs("BEGIN CHECK_ATTEMPTS(:id_, :t_, :attempts_); END;");
        cr.bind(":id_", id).bind(":t_", type()).bindOut(":attempts_", attempts).exec();
        return ServerFramework::makeAttemptState(attempts);
    }

    void undoAttempt(const std::string& id) override {
        const static std::string query(
                "UPDATE PROC_ATTEMPTS SET ATTEMPTS = ATTEMPTS - 1"
                " WHERE ID = :id_ AND PROC_TYPE = :t_ AND ATTEMPTS > 0");
#ifdef XP_TESTING
        if (inTestMode()) {
            make_curs("DECLARE PRAGMA AUTONOMOUS_TRANSACTION; BEGIN "
                    + query
                    + "; COMMIT; END;")
                .bind(":id_", id).bind(":t_", type())
                .exec();
            return;
        }
#endif // XP_TESTING
        make_curs(query)
            .bind(":id_", id).bind(":t_", type())
            .exec();
    }

    void dropAttempt(const std::string& id) override {
        const static std::string query(
                "DELETE FROM PROC_ATTEMPTS"
                " WHERE ID = :id_ AND PROC_TYPE = :t_");
#ifdef XP_TESTING
        if (inTestMode()) {
            make_curs("DECLARE PRAGMA AUTONOMOUS_TRANSACTION; BEGIN "
                    + query
                    + "; COMMIT; END;")
                .bind(":id_", id).bind(":t_", type())
                .exec();
            return;
        }
#endif // XP_TESTING
        make_curs(query)
            .bind(":id_", id).bind(":t_", type())
            .exec();
    }
};

class PgAttemptsCounter : public AttemptsCounter
{
public:
    PgAttemptsCounter(const std::string& type, std::function<void ()> fClear, PgCpp::SessionDescriptor sd)
        : AttemptsCounter(type, fClear), sd_(sd)
    {}

    AttemptState makeAttempt(const std::string& id) override {
        int attempts = 0;
        make_pg_curs_autonomous(sd_, "SELECT CHECK_ATTEMPTS(:id_, :t_)")
            .bind(":id_", id).bind(":t_", type())
            .def(attempts)
            .EXfet();
        return ServerFramework::makeAttemptState(attempts);
    }

    void undoAttempt(const std::string& id) override {
        const static std::string query(
                "UPDATE PROC_ATTEMPTS SET ATTEMPTS = ATTEMPTS - 1"
                " WHERE ID = :id_ AND PROC_TYPE = :t_ AND ATTEMPTS > 0");
#ifdef XP_TESTING
        if (inTestMode()) {
            make_pg_curs_autonomous(sd_, query)
                .bind(":id_", id).bind(":t_", type())
                .exec();
            return;
        }
#endif // XP_TESTING
        make_pg_curs(sd_, query)
            .bind(":id_", id).bind(":t_", type())
            .exec();
    }

    void dropAttempt(const std::string& id) override {
        const static std::string query(
                "DELETE FROM PROC_ATTEMPTS"
                " WHERE ID = :id_ AND PROC_TYPE = :t_");
#ifdef XP_TESTING
        if (inTestMode()) {
            make_pg_curs_autonomous(sd_, query)
                .bind(":id_", id).bind(":t_", type())
                .exec();
            return;
        }
#endif // XP_TESTING
        make_pg_curs(sd_, query)
            .bind(":id_", id).bind(":t_", type())
            .exec();
    }

private:
    PgCpp::SessionDescriptor sd_;
};

#ifdef XP_TESTING
class NopAttemptsCounter : public AttemptsCounter
{
public:
    NopAttemptsCounter(const std::string& type)
        : AttemptsCounter(type, [](){})
    {}
    AttemptState makeAttempt(const std::string& id) override {
        return ServerFramework::makeAttemptState(1);
    }
    void undoAttempt(const std::string& id) override {}
    void dropAttempt(const std::string& id) override {}
};
#endif // XP_TESTING

} // namespace

namespace ServerFramework
{

AttemptState makeAttemptState(unsigned attempts)
{
    if (attempts < 3) {
        return AttemptState::Fine;
    }
    if (attempts == 3) {
        return AttemptState::LastAttempt;
    }
    if (attempts == 4) {
        return AttemptState::AllAttemptsExhausted;
    }
    return AttemptState::TotallyBroken;
}

AttemptsCounter& AttemptsCounter::getCounter(const std::string& type)
{
#ifdef XP_TESTING
    if (inTestMode() && useDbForAttempts__ == false) {
        return *countersForTests.find(type)->second;
    }
#endif // XP_TESTING
    auto it = counters.find(type);
    if (it == counters.end()) {
        LogError(STDLOG) << "AttemptsCounter is missing for type [" << type << ']';
        throw Exception(STDLOG, __FUNCTION__, "AttemptsCounter is missing for type [" + type + "]");
    }
    return *it->second;
}

AttemptsCounter::AttemptsCounter(const std::string& type, std::function<void ()> fClear)
    : type_(type), fClear_(fClear)
{}

const std::string& AttemptsCounter::type() const
{
    return type_;
}

void AttemptsCounter::clear()
{
    fClear_();
}

#ifdef XP_TESTING
void AttemptsCounter::useDb(bool f)
{
    useDbForAttempts__ = f;
}
#endif // XP_TESTING

void setupAttemptCounter(const std::string& type, std::function<void ()> funcClear)
{
    check(type, funcClear);
#ifdef XP_TESTING
    if (inTestMode()) {
        countersForTests[type] = AttemptsCounterPtr(new NopAttemptsCounter(type));
    }
#endif // XP_TESTING
    counters.emplace(type, AttemptsCounterPtr(new OraAttemptsCounter(type, funcClear)));
}

void setupAttemptCounter(const std::string& type, std::function<void ()> funcClear, PgCpp::SessionDescriptor sd)
{
    check(type, funcClear);
#ifdef XP_TESTING
    if (inTestMode()) {
        countersForTests[type] = AttemptsCounterPtr(new NopAttemptsCounter(type));
    }
#endif // XP_TESTING
    counters.emplace(type, AttemptsCounterPtr(new PgAttemptsCounter(type, funcClear, sd)));
}

class ClearProcAttemptsTask : public DaemonTask
{
public:
    ClearProcAttemptsTask()
        : DaemonTask(DaemonTaskTraits::OracleOnly())
    {}

private:
    int run(const boost::posix_time::ptime& time) override {
        for (auto& c : counters) {
            c.second->clear();
        }
        return 0;
    }
};

void setupAttemptsCleanerTask(NewDaemon& d, unsigned freqInSeconds)
{
    DaemonEventPtr t1(new TimerDaemonEvent(freqInSeconds));
    DaemonTaskPtr proc(new ClearProcAttemptsTask());
    t1->addTask(proc);
    d.addEvent(t1);
}

} // ServerFramework
