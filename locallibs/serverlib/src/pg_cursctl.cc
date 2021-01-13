#ifdef ENABLE_PG
#include "pg_cursctl.h"
#include "pg_session.h"

#include <libpq-fe.h>
#include <arpa/inet.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "exception.h"
#include "helpcpp.h"
#include "dates.h"
#include "pg_row.h"
#include "tcl_utils.h"
#include "testmode.h"
#include "logopt.h"
#include "pg_locks.h"
#include "tclmon.h"
#include "str_utils.h"
#include "algo.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

#define PG_RESULT(x) static_cast<PGresult*>(x)
#define PG_CONN(x) static_cast<PGconn*>(x)

namespace PgCpp
{

const short* nullInd(bool isNull) noexcept
{
    static const short null = -1;
    static const short notNull = 0;
    return &(isNull ? null : notNull);
}

LockTimeout::LockTimeout(SessionDescriptor psd, size_t timeout_ms)
    : psd_(psd)
{
    make_pg_curs_nocache(psd_, "SET lock_timeout = " + std::to_string(timeout_ms)).exec();
}

LockTimeout::~LockTimeout()
{
    make_pg_curs_nocache(psd_, "SET lock_timeout = 0").exec();
}

namespace details
{

using AutoPgResult = std::unique_ptr<PGresult, decltype(&PQclear)>;
static AutoPgResult makeAutoResult(PGresult* res)
{
    return AutoPgResult(res, &PQclear);
}

class PgExecutor
{
public:
    std::string sql;
    std::vector<std::string> params;

    virtual void exec(const ServerFramework::NickFileLine&,
            PGconn*, SessionDescriptor,
            int nParams, const char* const * values, int* lengths,
            int* formats, Oid* types, int resultFormat);
    virtual ~PgExecutor() = default;
};

class PgCacheExecutor: public PgExecutor
{
public:
    bool isPrepared;
    std::string prepStmtId;
    std::vector<Oid> prepTypes;

    virtual void exec(const ServerFramework::NickFileLine&,
            PGconn*, SessionDescriptor,
            int nParams, const char* const * values, int* lengths,
            int* formats, Oid* types, int resultFormat) override;
};

class PgFetcher
{
public:
    PgFetcher(CursCtl& c)
        : cur_(c)
    {}
    virtual ~PgFetcher() {}

    virtual PGresult* getResult() = 0;
    virtual void fetchNext() = 0;
    virtual int rowcount() = 0;
    virtual int currentRow() = 0;
    virtual void clear() = 0;
    virtual void dump(std::ostream&) const = 0;
protected:
    PGconn* conn() {
        return PG_CONN(cur_.sess_->conn());
    }
    CursCtl& cur_;
};

std::ostream& operator<<(std::ostream& os, const PgFetcher& f)
{
    f.dump(os);
    return os;
}

static int affectedRows(PGresult* res)
{
    const char* rows = PQcmdTuples(res);
    if (rows && *rows) {
        char* endptr = nullptr;
        int rowcount = strtol(rows, &endptr, 10);
        if (endptr && *endptr != 0) {
            return 0;
        }
        return rowcount;
    }
    return 0;
}

class PgAllRowsFetcher : public PgFetcher
{
public:
    PgAllRowsFetcher(CursCtl& c)
        : PgFetcher(c), res_(nullptr), totalRows_(0), currRow_(-1), endOfData_(false)
    {}
    PGresult* getResult() override {
        if (endOfData_) {
            return nullptr;
        }
        if (res_ == nullptr) {
            res_ = PQgetResult(conn());
            if (!res_) {
                LogError(STDLOG) << "PGresult is null - do we are out-of-memory on pg-server?";
                endOfData_ = true;
                return nullptr;
            }
            totalRows_ = affectedRows(res_);
            currRow_ = 0;
            endOfData_ = (totalRows_ == 0);
            while (PGresult* res = PQgetResult(conn())) {
                PQclear(res);
            }
        }
        return res_;
    }
    void fetchNext() override {
        if (endOfData_) {
            return;
        }
        ++currRow_;
        endOfData_ = (currRow_ >= totalRows_);
    }
    int rowcount() override {
        return totalRows_;
    }
    int currentRow() override {
        return currRow_;
    }
    void clear() override {
        while (res_) {
            PQclear(res_);
            res_ = PQgetResult(conn());
        }
    }
    void dump(std::ostream& os) const override {
        os << " row " << currRow_ << " of " << totalRows_
            << LogOpt(" end-of-data", endOfData_);
        if (!res_) {
            return;
        }
        auto st = PQresultStatus(res_);
        os << " status is " << st << " [" << PQresStatus(st) << ']';
    }
private:
    PGresult* res_; // may be use here makeAutoResult
    int totalRows_;
    int currRow_;
    bool endOfData_;
};

class PgSingleRowFetcher : public PgFetcher
{
public:
    PgSingleRowFetcher(CursCtl& c)
        : PgFetcher(c), res_(nullptr, &PQclear), endOfData_(false)
    {}
    PGresult* getResult() override {
        if (endOfData_) {
            return nullptr;
        }
        if (!res_) {
            fetchResult();
        }
        return res_.get();
    }
    void fetchNext() override {
        if (endOfData_) {
            return;
        }
        fetchResult();
    }
    int rowcount() override {
        return 0;
    }
    int currentRow() override {
        return 0;
    }
    void clear() override {
        if (!endOfData_) { // try to drop remaining rows of answer
            PQrequestCancel(conn());
        }
        while (res_) {
            res_.reset(PQgetResult(conn()));
        }
    }
    void dump(std::ostream& os) const override {
        os << " single-row-mode" << LogOpt(" end-of-data", endOfData_);
        if (!res_) {
            return;
        }
        auto st = PQresultStatus(res_.get());
        os << " status is " << st << " [" << PQresStatus(st) << ']';
    }
private:
    void fetchResult() {
        PGresult* res = PQgetResult(conn());
        res_.reset(res);
        if (!res) {
            LogError(STDLOG) << "PGresult is null - are we out-of-memory on pg-server?";
            // note: no return
            // PQresultStatus will return PGRES_FATAL_ERROR,
            // which will LogError once more
            // and then stop with endOfData == true.
        }
        switch (auto st = PQresultStatus(res)) {
        case PGRES_TUPLES_OK: // no more rows
            clear(); // because otherwise until the cursor itself is deleted,
                     // libpq will continue to think that this query is _not_ finished
            break;
        case PGRES_SINGLE_TUPLE: // another row
            // ok, do nothing else
            break;
        default: {
            LogError(STDLOG) << "error after PQgetResult: status=" << st
                << " " << PQresultErrorMessage(res);
            res_.reset();
        }
        }
        endOfData_ = !static_cast<bool>(res_);
    }
    AutoPgResult res_;
    bool endOfData_;
};

static void traceNotices(void *arg, const char *message)
{
    if (message) {
        ProgError(STDLOG, "d=%p notice=[%s]", arg, message);
    }
}

std::ostream& operator<<(std::ostream& os, const Argument& a)
{
    return os << "t: " << a.type << " fmt: " << a.format << " len: " << a.length;
}

enum SessionType { Managed = 1, ReadOnly = 2, AutoCommit = 3 };

struct SessionDescription
{
    SessionPtr sess;
    std::string connectStr;
    SessionType type;
    boost::posix_time::ptime lastUse;
    bool active;
};

class SessionBehaviour
{
public:
    virtual ~SessionBehaviour() {}
    virtual void onInitTransaction(SessionDescriptor, PGconn*) = 0;
    virtual void onCommit(SessionDescriptor, Session&) = 0;
    virtual void onRollback(SessionDescriptor, Session&) = 0;
    virtual void setInitialSavepoints(SessionDescriptor, const std::list<std::string>& savepoints) = 0;
#ifdef XP_TESTING
    virtual void onCommitInTestMode(SessionDescriptor, Session&) = 0;
    virtual void onRollbackInTestMode(SessionDescriptor, Session&) = 0;
#endif // XP_TESTING
    virtual void onBeforeBadQuery(SessionDescriptor, PGconn*) = 0;
    virtual void onAfterBadQuery(SessionDescriptor, PGconn*) = 0;
};

static const char* savepointQuery = "SAVEPOINT SP_EXEC_SQL";
static const char* rollbackQuery = "ROLLBACK TO SAVEPOINT SP_EXEC_SQL";

ResultCode sqlStateToResultCode(const char* sqlState)
{
    if (!sqlState) {
        // isn't an error, ok
        return ResultCode::Ok;
    }
    // Error codes:
    // PostgreSQL documentation, Appendix A
    static struct {
        const char* sqlState;
        int len;
        ResultCode res;
    } errCodes[] = {
        {"08XXX", 2, ResultCode::BadConnection},    // Class 08: Connection Exception
        {"23XXX", 2, ResultCode::ConstraintFail},   // Class 23 - Integrity Constraint Violation
        {"42XXX", 2, ResultCode::BadSqlText},       // Class 42 - Syntax Error or Access Rule Violation
        {"57XXX", 2, ResultCode::BadConnection},    // Class 57: Operator Intervention
        {"25006", 5, ResultCode::ReadOnly},         // 25006: cannot execute in a read-only transaction
        {"40001", 5, ResultCode::BadConnection},    // 40001: Transaction Rollback: serialization_failure
        {"40P01", 5, ResultCode::Deadlock},         // 40P01: deadlock_detected
        {"55P03", 5, ResultCode::Busy}              // 55P03: lock_not_available
    };
    for (const auto& ec : errCodes) {
        if (std::strncmp(sqlState, ec.sqlState, ec.len) == 0) {
            return ec.res;
        }
    }
    return ResultCode::Fatal;
}

static void checkPgErr(const char* nick, const char* file, int line, SessionDescriptor sd, PGresult* res)
{
    ASSERT(res != nullptr && "PGresult is null - do we are out-of-memory on pg-server?");
    const auto tmpRes = makeAutoResult(res);
    const auto sqlState = PQresultErrorField(res, PG_DIAG_SQLSTATE);
    const auto resultCode = sqlStateToResultCode(sqlState);

    if (resultCode != ResultCode::Ok) {
        const auto err = PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY);
        LogError(nick, file, line) << "expected successful query " << err
            << '\n' << PQresultErrorMessage(res);
        throw PgException(nick, file, line, sd, resultCode, err);
    }
}

#ifdef XP_TESTING
static std::string currentSavepoint(int spCnt)
{
    return "SP_XP_TESTING_" + std::to_string(spCnt);
}
#endif // XP_TESTING

static PGresult* beginManagedTransaction(PGconn* conn)
{
    return PQexec(conn, "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ WRITE");
}

static PGresult* beginReadOnlyTransaction(PGconn* conn)
{
    return PQexec(conn, "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ ONLY");
}

#ifdef XP_TESTING
static std::set<SessionDescriptor> transactions;
#endif // XP_TESTING

static void beginManagedTransaction(PGconn* conn, SessionDescriptor sd)
{
#ifdef XP_TESTING
    if (inTestMode()) {
        if (transactions.count(sd) == 0) {
            checkPgErr(STDLOG, sd, beginManagedTransaction(conn));
            transactions.insert(sd);
        }
        checkPgErr(STDLOG, sd, PQexec(PG_CONN(conn), ("SAVEPOINT " + currentSavepoint(++sd->sess->spCounter_)).c_str()));
    } else {
        checkPgErr(STDLOG, sd, beginManagedTransaction(conn));
    }
#else
    checkPgErr(STDLOG, sd, beginManagedTransaction(conn));
#endif
}

static void beginReadOnlyTransaction(PGconn* conn, SessionDescriptor sd)
{
#ifdef XP_TESTING
    if (inTestMode()) {
        if (!transactions.count(sd)) {
            checkPgErr(STDLOG, sd, beginReadOnlyTransaction(conn));
            transactions.insert(sd);
        }
        checkPgErr(STDLOG, sd, PQexec(PG_CONN(conn), ("SAVEPOINT " + currentSavepoint(++sd->sess->spCounter_)).c_str()));
    } else {
        checkPgErr(STDLOG, sd, beginReadOnlyTransaction(conn));
    }
#else
    checkPgErr(STDLOG, sd, beginReadOnlyTransaction(conn));
#endif
}

static bool isQueryInProgress(PGconn* conn)
{
    return PQtransactionStatus(conn) == PQTRANS_ACTIVE;
}

static bool isQueryInProgress(const Session& sess)
{
    return isQueryInProgress(PG_CONN(sess.conn()));
}

class SessionManager
{
public:
    static SessionManager& instance();

    Session& getSession(const std::string& connStr, SessionType);

    Session& getSession(SessionDescriptor);

    Session& getAutoCommitSession(SessionDescriptor);

    SessionBehaviour& getSessionBehaviour(SessionDescriptor);

    void handleError(SessionDescriptor, ResultCode erc);

    void commit() {
        finishTransaction([](SessionBehaviour& b, SessionDescriptor sd, Session& sess)
                { b.onCommit(sd, sess); });
    }

    void rollback() {
        finishTransaction([](SessionBehaviour& b, SessionDescriptor sd, Session& sess)
                { b.onRollback(sd, sess); });
    }

    void makeSavepoint(const std::string& name);
    void rollbackSavepoint(const std::string& name);
    void resetSavepoint(const std::string& name);

#ifdef XP_TESTING
    void finishTransactionInTestMode(const std::function<void (SessionBehaviour&, SessionDescriptor, Session&)>& f) {
        auto now = Dates::currentDateTime();
        for (auto& sd : sessions_) {
            if (!sd.active && transactions.count(sd.sess->sd_) == 0) {
                disconnectUnused(sd, now);
                continue;
            }
            if (sd.sess) {
                if (!isQueryInProgress(*sd.sess)) {
                    sd.active = false;
                    f(getSessionBehaviour(&sd), sd.sess->sd_, *sd.sess);
                    transactions.erase(sd.sess->sd_);
                } else if (sd.type == SessionType::Managed) {
                    LogError(STDLOG) << "skipping commit/rollback for managed session " << &sd;
                }
            }
        }
        globalSavepoints_.clear();
    }

    void commitInTestMode() {
        finishTransactionInTestMode([](SessionBehaviour& b, SessionDescriptor sd, Session& sess) {
            b.onCommitInTestMode(sd, sess);
        });
    }

    void rollbackInTestMode() {
        finishTransactionInTestMode([](SessionBehaviour& b, SessionDescriptor sd, Session& sess) {
            b.onRollbackInTestMode(sd, sess);
        });
    }
#endif // XP_TESTING

    void activateSession(SessionDescriptor sd);

#ifndef XP_TESTING
private:
#endif // !XP_TESTING
    auto findSession(const std::string& connStr, SessionType t) {
        return std::find_if(sessions_.begin(), sessions_.end(),
                [&connStr, t](const auto& p)
                { return p.connectStr == connStr && p.type == t; });
    }

    auto findSession(SessionDescriptor sd) {
        return std::find_if(sessions_.begin(), sessions_.end(),
                [sd](const auto& p)
                { return &p == sd; });
    }

    void finishTransaction(const std::function<void (SessionBehaviour&, SessionDescriptor, Session&)>& f) {
        auto now = Dates::currentDateTime();
        for (auto& sd : sessions_) {
            if (!sd.active) {
                disconnectUnused(sd, now);
                continue;
            }
            if (sd.sess) {
                if (!isQueryInProgress(*sd.sess)) {
                    sd.active = false;
                    f(getSessionBehaviour(&sd), sd.sess->sd_, *sd.sess);
                } else if (sd.type == SessionType::Managed) {
                    LogError(STDLOG) << "skipping commit/rollback for managed session " << &sd;
                }
            }
        }
        globalSavepoints_.clear();
    }

    void disconnectUnused(SessionDescription& sd, const boost::posix_time::ptime& now) {
        ASSERT(sd.lastUse.is_not_a_date_time() == false && "expecting sd.lastUse to be initialized always");
        if (!sd.active
                && sd.sess
                && (now - sd.lastUse) > maxIdleTime_) {
            LogTrace(TRACE1) << "disconnect session d=" << &sd;
            sd.sess = SessionPtr();
#ifdef XP_TESTING
            transactions.erase(&sd);
#endif // XP_TESTING
        }
    }

    void reconnectSession(SessionDescription&, const SessionDescriptor);

    SessionManager();

    // no need to use b-tree search because of small size
    std::list<SessionDescription> sessions_;
    boost::posix_time::time_duration maxIdleTime_;
    std::list<std::string> globalSavepoints_;
};

template<typename T>
T reverseValue(const char* data)
{
    T result;
    char *dest = reinterpret_cast<char*>(&result);
    for(size_t i=0; i<sizeof(T); i++)
    {
        dest[i] = data[sizeof(T)-i-1];
    }
    return result;
}

template<typename T>
T htont(T src)
{
#   if __BYTE_ORDER__  == __ORDER_LITTLE_ENDIAN__
        return reverseValue<T>(reinterpret_cast<const char*>(&src));
#   else
        return src;
#   endif
}

bool PgTraits<float>::setNull(char* value)
{
    float* v = reinterpret_cast<float*>(value);
    *v = 0;
    return true;
}

void PgTraits<float>::fillBindData(std::vector<char>& dst, float v)
{
    dst.resize(sizeof(v));
    v = htont(v);
    memcpy(dst.data(), &v, sizeof(v));
}

bool PgTraits<float>::setValue(char* value, const char* data, int)
{
    float* v = reinterpret_cast<float*>(value);
    char* endptr = nullptr;
    *v = strtof(data, &endptr);
    if (endptr && *endptr != 0) {
        return false;
    }
    return true;
}

bool PgTraits<double>::setNull(char* value)
{
    double* v = reinterpret_cast<double*>(value);
    *v = 0;
    return true;
}

void PgTraits<double>::fillBindData(std::vector<char>& dst, double v)
{
    dst.resize(sizeof(v));
    v = htont(v);
    memcpy(dst.data(), &v, sizeof(v));
}

bool PgTraits<double>::setValue(char* value, const char* data, int)
{
    double* v = reinterpret_cast<double*>(value);
    char* endptr = nullptr;
    *v = strtod(data, &endptr);
    if (endptr && *endptr != 0) {
        return false;
    }
    return true;
}


bool PgTraits<int>::setNull(char* value)
{
    int* v = reinterpret_cast<int*>(value);
    *v = 0;
    return true;
}

void PgTraits<int>::fillBindData(std::vector<char>& dst, int v)
{
    dst.resize(sizeof(v));
    v = htonl(v);
    memcpy(dst.data(), &v, sizeof(v));
}

bool PgTraits<int>::setValue(char* value, const char* data, int)
{
    int* v = reinterpret_cast<int*>(value);
    char* endptr = nullptr;
    *v = strtol(data, &endptr, 10);
    if (endptr && *endptr != 0) {
        return false;
    }
    return true;
}

bool PgTraits<bool>::setNull(char* value)
{
    bool* v = reinterpret_cast<bool*>(value);
    *v = false;
    return true;
}

void PgTraits<bool>::fillBindData(std::vector<char>& dst, bool v)
{
    dst.resize(sizeof(v));
    v = htonl(v);
    memcpy(dst.data(), &v, sizeof(v));
}

bool PgTraits<bool>::setValue(char* value, const char* data, int)
{
    bool* s = reinterpret_cast<bool*>(value);
    if (data[1] == 0) {
        if (data[0] == 't') {
            *s = true;
            return true;
        }
        if (data[0] == 'f') {
            *s = false;
            return true;
        }
    }
    char* endptr = nullptr;
    int v = strtol(data, &endptr, 10);
    if (endptr && *endptr != 0) {
        return false;
    }
    *s = v == 1;
    return true;
}

bool PgTraits<short>::setNull(char* value)
{
    short* v = reinterpret_cast<short*>(value);
    *v = 0;
    return true;
}

void PgTraits<short>::fillBindData(std::vector<char>& dst, short v)
{
    PgTraits<int>::fillBindData(dst, v);
}

bool PgTraits<short>::setValue(char* value, const char* data, int)
{
    short* v = reinterpret_cast<short*>(value);
    char* endptr = nullptr;
    *v = strtol(data, &endptr, 10);
    if (endptr && *endptr != 0) {
        return false;
    }
    return true;
}

bool PgTraits<long long>::setNull(char* value)
{
    long long* v = reinterpret_cast<long long*>(value);
    *v = 0;
    return true;
}

void PgTraits<long long>::fillBindData(std::vector<char>& dst, long long v)
{
    dst.resize(sizeof(v));
    v = htobe64(v);
    memcpy(dst.data(), &v, sizeof(v));
}

bool PgTraits<long long>::setValue(char* value, const char* data, int)
{
    long long* v = reinterpret_cast<long long*>(value);
    char* endptr = nullptr;
    *v = strtoll(data, &endptr, 10);
    if (endptr && *endptr != 0) {
        return false;
    }
    return true;
}

static boost::date_time::special_values specialDtFromPgString(const char* value)
{
    // boost's parser wants "+infinity" as a pos_inf string
    // pg returns "infinity"
    static const auto pos_inf = "infinity";
    static const auto neg_inf = "-infinity";
    if (std::strcmp(value, pos_inf) == 0) {
        return boost::date_time::pos_infin;
    } else if (std::strcmp(value, neg_inf) == 0) {
        return boost::date_time::neg_infin;
    } else {
        return boost::date_time::not_special;
    }
}

bool PgTraits<boost::gregorian::date>::setNull(char* value)
{
    boost::gregorian::date* v = reinterpret_cast<boost::gregorian::date*>(value);
    *v = boost::gregorian::date(boost::gregorian::not_a_date_time);
    return true;
}

void PgTraits<boost::gregorian::date>::fillBindData(std::vector<char>& dst, const boost::gregorian::date& d)
{
    if (d.is_not_a_date()) {
        dst.clear();
        return;
    }

    if (!d.is_special()) {
        dst.resize(11, 0); // 4+1+2+1+2+zero byte
        sprintf(dst.data(), "%04d-%02d-%02d", (short)d.year(), (short)d.month(), (short)d.day());
    } else if (d.is_pos_infinity()) {
        static const auto pos_inf_str = "infinity";
        static const auto pos_inf_str_size = std::strlen(pos_inf_str) + 1;
        dst.assign(pos_inf_str, pos_inf_str + pos_inf_str_size);
    } else {
        static const auto neg_inf_str = "-infinity";
        static const auto neg_inf_str_size = std::strlen(neg_inf_str) + 1;
        dst.assign(neg_inf_str, neg_inf_str + neg_inf_str_size);
    }
}

bool PgTraits<boost::gregorian::date>::setValue(char* value, const char* data, int)
{
    boost::gregorian::date* v = reinterpret_cast<boost::gregorian::date*>(value);

    const auto special = specialDtFromPgString(data);
    if (special == boost::date_time::not_special) {
        *v = boost::gregorian::from_simple_string(data);
        if (v->is_not_a_date()) {
                return false;
        }
    } else {
        *v = boost::gregorian::date(special);
    }

    return true;
}

bool PgTraits<boost::posix_time::ptime>::setNull(char* value)
{
    boost::posix_time::ptime* v = reinterpret_cast<boost::posix_time::ptime*>(value);
    *v = boost::posix_time::ptime(boost::posix_time::not_a_date_time);
    return true;
}

void PgTraits<boost::posix_time::ptime>::fillBindData(std::vector<char>& dst, const boost::posix_time::ptime& t)
{
    if (t.is_not_a_date_time()) {
        dst.clear();
        return;
    }

    if (!t.is_special()) {
        dst.resize(24, 0); // 4+1+2+1+2+1+2+1+2+1+2+1+3+zero byte
        const boost::gregorian::date& d(t.date());
        const boost::posix_time::time_duration td(t.time_of_day());
        sprintf(dst.data(), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                (short)d.year(), (short)d.month(), (short)d.day(),
                (short)td.hours(), (short)td.minutes(), (short)td.seconds(),
                (short)(td.total_milliseconds() % 1000));
    } else if (t.is_pos_infinity()) {
        static const auto pos_inf_str = "infinity";
        static const auto pos_inf_str_size = std::strlen(pos_inf_str) + 1;
        dst.assign(pos_inf_str, pos_inf_str + pos_inf_str_size);
    } else {
        static const auto neg_inf_str = "-infinity";
        static const auto neg_inf_str_size = std::strlen(neg_inf_str) + 1;
        dst.assign(neg_inf_str, neg_inf_str + neg_inf_str_size);
    }
}

bool PgTraits<boost::posix_time::ptime>::setValue(char* value, const char* data, int)
{
    using namespace boost::posix_time;
    ptime* v = reinterpret_cast<ptime*>(value);

    const auto special = specialDtFromPgString(data);
    if (special == boost::date_time::not_special) {
        int yy, mm, dd, hh, mi, ss, fsec;
        const auto n = sscanf(data, "%d-%d-%d %d:%d:%d.%d", &yy, &mm, &dd, &hh, &mi, &ss, &fsec);

        if (n == 7) {
            const char* dotPos = strchr(data, '.');
            auto fsecLen = strcspn(dotPos, "+-") - 1;
            fsec *= pow(10, static_cast<int>(3 - fsecLen));
        } else if (n == 6) {
            fsec = 0;
        } else {
            return false;
        }

        *v = ptime(
                boost::gregorian::date(yy, mm, dd),
                time_duration(hours(hh) + minutes(mi) + seconds(ss) + millisec(fsec)));

        if (v->is_not_a_date_time()) {
            return false;
        }
    } else {
        *v = ptime(special);
    }

    return true;
}

bool PgTraits<std::string>::setNull(char* value)
{
    std::string* v = reinterpret_cast<std::string*>(value);
    v->clear();
    return true;
}

void PgTraits<std::string>::fillBindData(std::vector<char>& dst, const std::string& s)
{
    if (s.empty()) {
        dst.clear();
        return;
    }
    const int sz = s.length();
    dst.resize(sz + 1);
    memcpy(dst.data(), s.c_str(), sz);
    dst[sz] = 0;
}

bool PgTraits<std::string>::setValue(char* value, const char* data, int len)
{
    std::string* s = reinterpret_cast<std::string*>(value);
    s->assign(data, len);
    return true;
}

void PgTraits<const char*>::fillBindData(std::vector<char>& dst, const char* s)
{
    if (!s) {
        dst.clear();
        return;
    }
    const int sz = strlen(s);
    dst.resize(sz + 1);
    memcpy(dst.data(), s, sz);
    dst[sz] = 0;
}

bool PgTraits<char>::setNull(char* value)
{
    *value = 0;
    return true;
}

void PgTraits<char>::fillBindData(std::vector<char>& dst, char c)
{
    dst = {c, '\0'};
}

bool PgTraits<char>::setValue(char* value, const char* data, int len)
{
    if (len > 1) {
        return false;
    }
    *value = *data;
    return true;
}

bool fillCharBuffFromHex(char* src, int srcSz, const char* data)
{
    if (!data || data[0] != '\\' || data[1] != 'x') {
        ProgTrace(TRACE1, "invalid data format: [%2.2s]", data);
        return false;
    }
    data += 2;
    const int sz = strlen(data);
    if (sz % 2 != 0 || sz > 2 * srcSz) {
        ProgTrace(TRACE1, "invalid data size=%d (buffer sz=%d)", sz, srcSz);
        return false;
    }
    char* endptr = nullptr;
    char byteBuff[3] = {};
    for (int i = 0; i < sz; i += 2) {
        byteBuff[0] = data[i];
        byteBuff[1] = data[i + 1];
        src[i / 2] = strtol(byteBuff, &endptr, 16);
        if (endptr && *endptr != 0) {
            LogTrace(TRACE1) << "invalid hex value at " << (i + 2) << '[' << byteBuff << ']';
            return false;
        }
    }
    return true;
}

void dumpCursor(const CursCtl& cr)
{
    LogTrace(TRACE1) << "*** dump cursor from "
        << cr.pos_.nick << ':' << cr.pos_.file << ':' << cr.pos_.line
        << "erc: " << cr.erc_
        << LogOpt(" unstb", !cr.stable_) << LogOpt(" autonull", cr.autoNull_);
    LogTrace(TRACE1) << " sql: [" << cr.exectr_->sql << ']';
    if (cr.fetcher_) {
        LogTrace(TRACE1) << *cr.fetcher_;
    }

    auto cache = dynamic_cast<PgCacheExecutor*>(cr.exectr_.get());
    if (cache) {
        LogTrace(TRACE1) << " cache:" << LogOpt(" prepared", cache->isPrepared)
                         << " stmtId=" << cache->prepStmtId
                         << " ptypes: " << LogCont(" ", cache->prepTypes);
    }
    if (!cr.exectr_->params.empty()) {
        LogTrace(TRACE1) << " *** BIND INFORMATION ***";
    }
    for (std::size_t i = 0; i != cr.exectr_->params.size(); ++i) {
        auto b = cr.binds_.find(cr.exectr_->params[i]);
        if (b == cr.binds_.end()) {
            LogTrace(TRACE1) << " $" << std::to_string(i + 1) << ": " << cr.exectr_->params[i] << " nothing bound";
        } else {
            LogTrace(TRACE1) << " $" << std::to_string(i + 1) << ": " << cr.exectr_->params[i] << ' ' << b->second.arg;
            if (b->second.value.size()) {
                if (b->second.arg.format) {
                    LogTrace(TRACE1) << " value: " << HelpCpp::memdump(b->second.value.data(), b->second.value.size());
                } else {
                    LogTrace(TRACE1) << " value: " << LogCont("", b->second.value);
                }
            } else {
                LogTrace(TRACE1) << " value: null";
            }

            if (!cr.stable_) {
                if (b->second.ind) {
                    LogTrace(TRACE1) << " ind: " << *b->second.ind;
                } else {
                    LogTrace(TRACE1) << " ind is null";
                }
                LogTrace(TRACE1) << " data points to: " << b->second.data;
            }
        }
    }

    if (!cr.defs_.empty()) {
        LogTrace(TRACE1) << " *** DEFS INFORMATION *** ";
    }
    for (const auto& d : cr.defs_) {
        LogTrace(TRACE1) << ' ' << d.arg << " ptr: " << static_cast<void*>(d.value)
            << " ind: " << d.ind << " defIdx: " << d.defaultValue;
    }
    if (!cr.noThrowErrors_.empty()) {
        LogTrace(TRACE1) << " nothrow: " << LogCont(" ", cr.noThrowErrors_);
    }
    LogTrace(TRACE1) << "*** cursor dump end ***";
}

void handleError(const CursCtl& cr, SessionDescriptor sd, ResultCode erc, const char* errText, const char* errCode)
{
    if (erc == ResultCode::Ok) {
        return;
    }
    LogTrace(TRACE5) << "handleError: " << erc
        << " [" << (errText ? errText : "-") << "]"
        << " [" << (errCode ? errCode : "-") << "]";

    switch (erc) { // critical error
    case ResultCode::Fatal:
    case ResultCode::BadSqlText:
    case ResultCode::BadBind: {
        dumpCursor(cr);
        throw PgException(STDLOG, sd, erc, errText);
        break;
    }
    default:
        break;
    }

    if (!cr.noThrowErrors_.count(erc)) {
        dumpCursor(cr);
        throw PgException(STDLOG, sd, erc, errText);
    }
}

bool checkResult(CursCtl& cur, SessionDescriptor sd, PgResult r)
{
    // if we are in SRM, and the query returned 0 rows,
    // r can be nullptr. That is fine.
    const auto sqlState = PQresultErrorField(PG_RESULT(r), PG_DIAG_SQLSTATE);
    const auto resultCode = sqlStateToResultCode(sqlState);

    if (resultCode == ResultCode::Ok) {
        return true;
    }

    if (const auto ctxt = PQresultErrorField(PG_RESULT(r), PG_DIAG_CONTEXT)) {
        LogTrace(TRACE1) << "context: " << ctxt;
    }

    cur.erc_ = resultCode;
    handleError(cur, sd, resultCode, PQresultErrorField(PG_RESULT(r), PG_DIAG_MESSAGE_PRIMARY), sqlState);
    return false;
}

using Positions = std::vector<std::string>;

static PGresult* prepareData(PGconn* conn, PgCacheExecutor* data, Oid* types)
{
    return PQprepare(conn,
        data->prepStmtId.c_str(),
        data->sql.c_str(),
        data->params.size(),
        types
    );
}

SessionManager::SessionManager()
{
#ifdef XP_TESTING
    maxIdleTime_ = boost::posix_time::pos_infin;
#else
    maxIdleTime_ = boost::posix_time::seconds(readIntFromTcl("PGCPP(SESSION_IDLE_SECONDS)", 60));
#endif // XP_TESTING
}

SessionManager& SessionManager::instance()
{
    static SessionManager pool;
    return pool;
}

static SessionPtr setupSession(const std::string& connStr, SessionType t)
{
    SessionPtr s = Session::create(connStr.c_str());
    return s;
}

void SessionManager::activateSession(SessionDescriptor sd)
{
    if (!sd) return;
    auto it = findSession(sd);
    ASSERT(it != sessions_.end() && "activateSession with invalid descriptor");

    if (!it->active) {
        getSessionBehaviour(sd).onInitTransaction(sd, PG_CONN(it->sess->conn_));
        it->active = true;
        getSessionBehaviour(sd).setInitialSavepoints(sd, globalSavepoints_);
    }
}

void SessionManager::reconnectSession(SessionDescription& sess, const SessionDescriptor sd)
{
    sess.sess = setupSession(sess.connectStr, sess.type);
    sess.sess->sd_ = sd;
    sess.active = false;
    sess.lastUse = Dates::currentDateTime();
    LogTrace(TRACE1) << "reconnectSession d=" << sd;
    PQsetNoticeProcessor(PG_CONN(sess.sess->conn_), traceNotices, sd);
}

static std::string hidePasswordInConnStr(const std::string& connStr)
{
    size_t endPasswordPos = connStr.find('@');
    if (endPasswordPos == std::string::npos) {
        return connStr;
    }

    size_t beginPasswordPos = connStr.find_last_of(':', endPasswordPos);
    if (beginPasswordPos == std::string::npos) {
        return connStr;
    }

    if (beginPasswordPos >= endPasswordPos) {
        return connStr;
    }
    beginPasswordPos += 1;

    size_t length = endPasswordPos - beginPasswordPos;
    std::string str = connStr;
    return str.replace(beginPasswordPos, length, length, '*');
}

Session& SessionManager::getSession(const std::string& connStr, SessionType t)
{
    auto it = findSession(connStr, t);
    if (it != sessions_.end()) { // session was created
        if (!it->sess) { // session was disconnected
            reconnectSession(*it, static_cast<SessionDescriptor>(&*it));
        }
        return *it->sess;
    }
    SessionPtr s = setupSession(connStr, t);
    sessions_.push_back(SessionDescription{s, connStr, t, Dates::currentDateTime(), false});
    s->sd_ = static_cast<SessionDescriptor>(&sessions_.back());
    PQsetNoticeProcessor(PG_CONN(s->conn_), traceNotices, s->sd_);
    LogTrace(TRACE1) << "createSession " << hidePasswordInConnStr(connStr) << ' ' << t << " d=" << s->sd_;
    return *sessions_.back().sess;
}

Session& SessionManager::getSession(SessionDescriptor sd)
{
    auto it = findSession(sd);
    if (it == sessions_.end()) {
        LogTrace(TRACE0) << "getSession with bad descriptor: " << sd;
        throw PgException(STDLOG, 0, ResultCode::Fatal, "invalid session descriptor");
    }
    if (!it->sess) { // session was disconnected
        reconnectSession(*it, sd);
    }
    // getSession by descriptor is called only from createCursor
    // any createCursor should activate session
    activateSession(sd);
    it->lastUse = Dates::currentDateTime();
    return *it->sess;
}

Session& SessionManager::getAutoCommitSession(SessionDescriptor sd)
{
    auto it = findSession(sd);
    if (it == sessions_.end()) {
        LogTrace(TRACE0) << "getSession with bad descriptor: " << sd;
        throw PgException(STDLOG, 0, ResultCode::Fatal, "invalid session descriptor");
    }
    if (it->type == AutoCommit) {
        return getSession(static_cast<SessionDescriptor>(&*it));
    }
    const std::string connStr = it->connectStr;
    it = findSession(connStr, AutoCommit);
    if (it != sessions_.end()) {
        return getSession(static_cast<SessionDescriptor>(&*it));
    }
    return getSession(connStr, AutoCommit);
}

void SessionManager::makeSavepoint(const std::string& name)
{
    for (SessionDescription& sd : sessions_) {
        if (sd.sess && sd.type == SessionType::Managed) {
            LogTrace(TRACE1) << "savepoint " << name << " d=" << sd.sess->sd_;
            make_pg_curs(sd.sess->sd_, "SAVEPOINT " + name).exec();
        }
    }
    globalSavepoints_.push_back(name);
}

void SessionManager::rollbackSavepoint(const std::string& name)
{
    for (SessionDescription& sd : sessions_) {
        if (sd.sess && sd.type == SessionType::Managed) {
            LogTrace(TRACE1) << "rollback to savepoint " << name << " d=" << sd.sess->sd_;
            checkPgErr(STDLOG, sd.sess->sd_, PQexec(PG_CONN(sd.sess->conn_), ("ROLLBACK TO SAVEPOINT " + name).c_str()));
        }
    }
    const auto rit = std::find(globalSavepoints_.crbegin(), globalSavepoints_.crend(), name);
    if (rit != globalSavepoints_.crend()) {
        globalSavepoints_.erase(--rit.base(), globalSavepoints_.cend());
    }
}

void SessionManager::resetSavepoint(const std::string& name)
{
    for (SessionDescription& sd : sessions_) {
        if (sd.sess && sd.type == SessionType::Managed) {
            LogTrace(TRACE1) << "reset savepoint " << name << " d=" << sd.sess->sd_;
            make_pg_curs(sd.sess->sd_, "SAVEPOINT " + name).exec();
        }
    }
    globalSavepoints_.push_back(name);
}

class ManagedSessionBehaviour : public SessionBehaviour
{
public:
    void onInitTransaction(SessionDescriptor sd, PGconn* conn) override {
        LogTrace(TRACE1) << "beginManagedTransaction " << sd;
        beginManagedTransaction(conn, sd);
    }
    void onCommit(SessionDescriptor sd, Session& sess) override {
        sess.commit();
    }
    void onRollback(SessionDescriptor sd, Session& sess) override {
        sess.rollback();
    }
    void setInitialSavepoints(SessionDescriptor sd, const std::list<std::string>& savepoints) override {
        for (const auto& sp: savepoints) {
            LogTrace(TRACE1) << "savepoint " << sp << " for new transaction d=" << sd;
            make_pg_curs(sd, "SAVEPOINT " + sp).exec();
        }
    }
#ifdef XP_TESTING
    void onCommitInTestMode(SessionDescriptor sd, Session& sess) override {
        sess.commitInTestMode();
    }
    void onRollbackInTestMode(SessionDescriptor sd, Session& sess) override {
        sess.rollbackInTestMode();
    }
#endif // XP_TESTING
    void onBeforeBadQuery(SessionDescriptor sd, PGconn* conn) override {
        checkPgErr(STDLOG, sd, PQexec(conn, savepointQuery));
    }
    void onAfterBadQuery(SessionDescriptor sd, PGconn* conn) override {
        checkPgErr(STDLOG, sd, PQexec(conn, rollbackQuery));
    }
};

class ReadOnlySessionBehaviour : public SessionBehaviour
{
public:
    void onInitTransaction(SessionDescriptor sd, PGconn* conn) override {
        LogTrace(TRACE1) << "beginReadOnlyTransaction " << sd;
        beginReadOnlyTransaction(conn, sd);
    }
    void onCommit(SessionDescriptor sd, Session& sess) override {
        sess.commit();
    }
    void onRollback(SessionDescriptor sd, Session& sess) override {
        sess.rollback();
    }
    void setInitialSavepoints(SessionDescriptor, const std::list<std::string>&) override {}
#ifdef XP_TESTING
    void onCommitInTestMode(SessionDescriptor sd, Session& sess) override {
        sess.commitInTestMode();
    }
    void onRollbackInTestMode(SessionDescriptor sd, Session& sess) override {
        sess.rollbackInTestMode();
    }
#endif // XP_TESTING
    void onBeforeBadQuery(SessionDescriptor sd, PGconn* conn) override {
        checkPgErr(STDLOG, sd, PQexec(conn, savepointQuery));
    }
    void onAfterBadQuery(SessionDescriptor sd, PGconn* conn) override {
        checkPgErr(STDLOG, sd, PQexec(conn, rollbackQuery));
    }
};

class AutoCommitSessionBehaviour : public SessionBehaviour
{
public:
    void onInitTransaction(SessionDescriptor, PGconn*) override {}
    void onCommit(SessionDescriptor, Session&) override {}
    void onRollback(SessionDescriptor, Session&) override {}
    void setInitialSavepoints(SessionDescriptor, const std::list<std::string>&) override {}
#ifdef XP_TESTING
    void onCommitInTestMode(SessionDescriptor, Session&) override {}
    void onRollbackInTestMode(SessionDescriptor, Session&) override {}
#endif // XP_TESTING
    void onBeforeBadQuery(SessionDescriptor, PGconn*) override {}
    void onAfterBadQuery(SessionDescriptor, PGconn*) override {}
};

SessionBehaviour& SessionManager::getSessionBehaviour(SessionDescriptor sd)
{
    static ManagedSessionBehaviour managed;
    static ReadOnlySessionBehaviour readonly;
    static AutoCommitSessionBehaviour autocommit;

    auto it = findSession(sd);
    if (it == sessions_.end()) {
        // since AutoCommitSessionBehaviour does nothing it perfectly fits for NOP
        return autocommit;
    }
    switch (it->type) {
    case SessionType::Managed: return managed;
    case SessionType::ReadOnly: return readonly;
    case SessionType::AutoCommit: return autocommit;
    }
    throw PgException(STDLOG, nullptr, ResultCode::Fatal,
            "unknown behaviour: " + std::to_string(static_cast<int>(it->type)));
}

void SessionManager::handleError(SessionDescriptor sd, ResultCode erc)
{
    LogTrace(TRACE1) << "session " << sd << " handleError " << erc;
    if (sd == 0 || erc == ResultCode::Ok) {
        return;
    }
    auto it = findSession(sd);
    if (it == sessions_.end()) {
        LogError(STDLOG) << "invalid session descriptor in SessionManager::handleError " << sd;
        return;
    }
    // try to reconnect for any session
    if (erc == ResultCode::BadConnection) {
        LogTrace(TRACE1) << "disconnect session " << sd << " due to BadConnection";
        it->sess = SessionPtr{};
#ifdef XP_TESTING
        transactions.erase(sd);
#endif // XP_TESTING
        return;
    }
    // for readonly - reconnect on any error
    if (it->type == SessionType::ReadOnly) {
        LogTrace(TRACE1) << "disconnect readonly session " << sd << " due to error " << erc;
        it->sess = SessionPtr{};
#ifdef XP_TESTING
        transactions.erase(sd);
#endif // XP_TESTING
        reconnectSession(*it, sd);
        return;
    }
    // for autocommit do nothing
}

} // details

using namespace PgCpp::details;

// ======     Session     ===== //

SessionDescriptor getManagedSession(const std::string& connStr)
{
    return SessionManager::instance().getSession(connStr, SessionType::Managed).sd_;
}

SessionDescriptor getReadOnlySession(const std::string& connStr)
{
    return SessionManager::instance().getSession(connStr, SessionType::ReadOnly).sd_;
}

SessionDescriptor getAutoCommitSession(const std::string& connStr)
{
    return SessionManager::instance().getSession(connStr, SessionType::AutoCommit).sd_;
}

void commit()
{
    SessionManager::instance().commit();
}

void rollback()
{
    SessionManager::instance().rollback();
}

void makeSavepoint(const std::string& name)
{
    SessionManager::instance().makeSavepoint(name);
}

void rollbackSavepoint(const std::string& name)
{
    SessionManager::instance().rollbackSavepoint(name);
}

void resetSavepoint(const std::string& name)
{
    SessionManager::instance().resetSavepoint(name);
}

#ifdef XP_TESTING
void commitInTestMode()
{
    SessionManager::instance().commitInTestMode();
}

void rollbackInTestMode()
{
    SessionManager::instance().rollbackInTestMode();
}
#endif // XP_TESTING

bool copyDataFrom(Session& sess, const std::string& sql, const char* data, size_t size)
{
    PGconn* conn     = PG_CONN(sess.conn());
    AutoPgResult res = makeAutoResult(PQexec(conn, sql.c_str()));
    if (PQresultStatus(&*res) != PGRES_COPY_IN) {
        LogTrace(TRACE0) << "COPY_IN prepare failed: " << PQerrorMessage(conn);
        return false;
    }

    int ret = PQputCopyData(conn, data, size);
    if (ret != 1) {
        LogTrace(TRACE0) << "PQputCopyData failed (ret=" << ret << "): " << PQerrorMessage(conn);
        return false;
    }

    ret = PQputCopyEnd(conn, NULL);
    if (ret != 1) {
        LogTrace(TRACE0) << "PQputCopyEnd() failed (ret=" << ret << "): " << PQerrorMessage(conn);
        return false;
    }

    bool result = true;
    while (res) {
        res.reset(PQgetResult(conn));
        if (res) {
            auto st = PQresultStatus(&*res);
            if (st != PGRES_COMMAND_OK) {
                LogTrace(TRACE0) << "PQgetResult after PQputCopyEnd() failed (status=" << st
                                 << "): " << PQresStatus(st);
                if(st == PGRES_FATAL_ERROR)
                    LogError(STDLOG) << PQerrorMessage(conn);
                else
                    LogTrace(TRACE0) << PQerrorMessage(conn);
                result = false;
            }
        }
    }
    return result;
}

std::string connStringWithAppName(const std::string& connStr, const std::string& app_suffix)
{
  return connStr
    + (connStr.find('?')==std::string::npos ? "?":"&")
    +"application_name="+tclmonCurrentProcessName()
    +"_"+std::to_string(getpid())
    + (app_suffix.empty() ? app_suffix : ("_"+app_suffix));
}

static PGconn* connect(const char* a_connStr, bool exitOnFail)
{
    const std::string connStr=connStringWithAppName(a_connStr);
    PGconn* c = PQconnectdb(connStr.c_str());
    const ConnStatusType st = PQstatus(c);
    if (st != CONNECTION_OK) {
        LogTrace(TRACE0) << PQerrorMessage(c);
        PQfinish(c);

        if (exitOnFail) {
            LogError(STDLOG) << "failed to connect to " << connStr;
            sleep(1);
            exit(1);
        } else {
            LogTrace(TRACE0) << "failed to connect to " << connStr;
            throw PgException(STDLOG, 0, ResultCode::Fatal, "failed to connect");
        }
    }
    PQsetNoticeProcessor(c, traceNotices, nullptr);
    return c;
}

static void initSession(Session& sess)
{
    PGconn* conn = PG_CONN(sess.conn());

    static const std::string timeZoneQuery = "SET TIME ZONE '"
                                           + readStringFromTcl("PGCPP(TIMEZONE)", "UTC")
                                           + "'";
    checkPgErr(STDLOG, nullptr, PQexec(conn, timeZoneQuery.c_str()));

    static const std::string clientEncodingQuery = "SET CLIENT_ENCODING TO '"
                                                 + readStringFromTcl("PGCPP(CLIENT_ENCODING)", "WIN866")
                                                 + "'";
    checkPgErr(STDLOG, nullptr, PQexec(conn, clientEncodingQuery.c_str()));
}

SessionPtr Session::create(const char *connStr)
{
    SessionPtr sess(new Session(connStr));
    initSession(*sess);
    return sess;
}

Session::Session(const char* connStr)
    : sd_(0)
{
#ifdef XP_TESTING
    spCounter_ = 0;
#endif // XP_TESTING
    conn_ = connect(connStr, false);
}

Session::~Session()
{
    if (conn_) {
        PQfinish(PG_CONN(conn_));
    }
}

CursCtl Session::createCursor(const char* n, const char* f, int l, const char* sql)
{
    return CursCtl(shared_from_this(), n, f, l, cursData(sql, true));
}

CursCtl Session::createCursor(const char* n, const char* f, int l, const std::string& sql)
{
    return CursCtl(shared_from_this(), n, f, l, cursData(sql, true));
}

CursCtl Session::createCursorNoCache(const char* n, const char* f, int l, const char* sql)
{
    return CursCtl(shared_from_this(), n, f, l, cursData(sql, false));
}

CursCtl Session::createCursorNoCache(const char* n, const char* f, int l, const std::string& sql)
{
    return CursCtl(shared_from_this(), n, f, l, cursData(sql, false));
}

PgConn Session::conn() const
{
    return conn_;
}

static std::string makePStmtId(const std::string& sql)
{
    return "p" + std::to_string(std::hash<std::string>()(sql));
}

static std::string convertSql(Positions& ps, const std::string& inStr);

std::shared_ptr<PgExecutor> Session::cursData(const std::string& sql, bool cache)
{
    std::shared_ptr<PgExecutor> res;

    if (cache) {
        const auto stmtId = makePStmtId(sql);

        const auto it = cursCache_.find(stmtId);
        if (it != cursCache_.end()) {
            return it->second;
        }

        const auto data = new PgCacheExecutor();
        data->isPrepared = false;
        data->prepStmtId = stmtId;
        res.reset(data);

        cursCache_.insert({stmtId, res});
    } else {
        res.reset(new PgExecutor());
    }

    Positions p;
    const auto newSql = convertSql(p, sql);

    res->sql = newSql;
    res->params = p;

    return res;
}


void Session::commit()
{
#ifdef XP_TESTING
    if (inTestMode()) {
        ++spCounter_;
        LogTrace(TRACE5) << "commit session " << sd_ << " SAVEPOINT " << currentSavepoint(spCounter_);
        checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), ("SAVEPOINT " + currentSavepoint(spCounter_)).c_str()));
        return;
    }
#endif //XP_TESTING
    LogTrace(TRACE1) << "commit session " << sd_;
    checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), "COMMIT"));
}

void Session::rollback()
{
#ifdef XP_TESTING
    if (inTestMode()) {
        auto const&& sp_name = currentSavepoint(spCounter_);
        ProgTrace(TRACE5, "rollback session %p TO SAVEPOINT %s", sd_, sp_name.c_str());
        ASSERT(0 != spCounter_ && "rollback to savepoint without savepoint")
        checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), ("ROLLBACK TO SAVEPOINT " + sp_name).c_str()));
        return;
    }
#endif //XP_TESTING
    LogTrace(TRACE1) << "rollback session " << sd_;
    checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), "ROLLBACK"));
}

#ifdef XP_TESTING
void Session::commitInTestMode()
{
    checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), "COMMIT"));
}

void Session::rollbackInTestMode()
{
    checkPgErr(STDLOG, sd_, PQexec(PG_CONN(conn_), "ROLLBACK"));
}

CursorCache& Session::getCursorCache()
{
    return cursCache_;
}
#endif // XP_TESTING

// ======   PgException   ===== //

PgException::PgException(SessionDescriptor sd, ResultCode erc, const std::string& err)
    : comtech::Exception(err), erc_(erc)
{
    SessionManager::instance().handleError(sd, erc);
}

PgException::PgException(const char* nick, const char* file, int line,
        SessionDescriptor sd, ResultCode erc, const std::string& err)
    : comtech::Exception(nick, file, line, "", err), erc_(erc)
{
    SessionManager::instance().handleError(sd, erc);
}

ResultCode PgException::errCode() const
{
    return erc_;
}

// ======     CursCtl     ===== //

CursCtl::CursCtl(std::shared_ptr<Session> sess, const char* n, const char* f, int l, std::shared_ptr<PgExecutor> exectr)
    : sess_(std::move(sess)), exectr_(std::move(exectr)),pos_{n, f, l}, erc_(ResultCode::Ok),
      autoNull_(false), stable_(true), singleRowMode_(false)
{
    if (isQueryInProgress(PG_CONN(sess_->conn()))) {
        throw PgException(n, f, l, sess_->sd_, ResultCode::Fatal,
                          "cursor creation disabled while SRM cursor is active");
    }
}

CursCtl::~CursCtl()
{
    for (details::IDefaultValueHolder* p : defaults_) {
        delete p;
    }
    if (fetcher_) {
        fetcher_->clear();
    }
}

CursCtl& CursCtl::defRow(BaseRow& r)
{
    r.def(*this);
    return *this;
}

short CursCtl::defIndPlaceholder_ = 0;

static bool validPlaceholderChar(char c)
{
    static const char breakers[] = " ,.();:{}" // misc
                                   "+-/*"    // arith
                                   "<>=!"     // compare
                                   "|";      // concat
    return strchr(breakers, c) == NULL;
}

static int appendPos(Positions& ps, const std::string& pl)
{
    int i = 0;
    for (const auto& p : ps) {
        ++i;
        if (p == pl) {
            return i;
        }
    }
    ps.push_back(pl);
    return ++i;
}

bool isDml(const std::string& s)
{
    auto pos_beg = s.find_first_not_of(' ');
    if (pos_beg == std::string::npos) {
        return false;
    }
    auto pos = s.find_first_of(" (+-'\n",pos_beg);
    if (pos == std::string::npos) {
        return false;
    }
    std::string cmd = s.substr(pos_beg, pos);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    return cmd == "SELECT"
        || cmd == "INSERT"
        || cmd == "UPDATE"
        || cmd == "DELETE"
        || cmd == "WITH";
}

static std::string convertSql(Positions& ps, const std::string& inStr)
{
    enum State { BeforeColon, AfterColon, InsidePlaceholder };

    if (inStr.empty() || !isalpha(inStr.front())) {
        throw PgException(STDLOG, nullptr, ResultCode::BadSqlText, "invalid sql");
    }
    if (!isDml(inStr)) {
        return inStr;
    }
    std::string s, pl;
    s.reserve(inStr.length() + 10);
    int sz = inStr.size();
    State st = BeforeColon;
    bool prevPlaceholderChar = false;
    for (int i = 0; i < sz; ++i) {
        const char c = inStr[i];
        switch (st) {
        case BeforeColon: {
            if (c == ':' && !prevPlaceholderChar) {
                st = AfterColon;
            } else {
                s += c;
            }
            break;
        }
        case AfterColon: {
            if (validPlaceholderChar(c)) {
                pl += c;
                st = InsidePlaceholder;
            } else {
                s += ':';
                s += c;
                st = BeforeColon;
            }
            break;
        }
        case InsidePlaceholder: {
            if (validPlaceholderChar(c)) {
                pl += c;
            } else {
                const int pos = appendPos(ps, pl);
                s += '$';
                s += std::to_string(pos);
                if (c == ':') { // cast directly after a placeholder " :plh::INTEGER "
                    st = AfterColon;
                } else {
                    s += c;
                    st = BeforeColon;
                }
                pl.clear();
            }
            break;
        }
        default:
            LogError(STDLOG) << '[' << inStr << ']';
            throw PgException(STDLOG, nullptr, ResultCode::Fatal, "convertSql failed");
        }
        prevPlaceholderChar = validPlaceholderChar(c);
    }
    if (!pl.empty()) {
        const int pos = appendPos(ps, pl);
        s += '$';
        s += std::to_string(pos);
    }
    return s;
}

void PgExecutor::exec(const ServerFramework::NickFileLine& pos,
        PGconn* conn, SessionDescriptor sd,
        int nParams, const char* const * values, int* lengths,
        int* formats, Oid* types, int resultFormat)
{
    if (isQueryInProgress(conn)) {
        throw PgException(pos.nick, pos.file, pos.line, sd, ResultCode::Fatal,
                "do not execute query while previous is not fully processed");
    }
    const int res = PQsendQueryParams(conn, sql.c_str(), nParams,
            types, values, lengths, formats, resultFormat);
    if (res != 1) {
        throw PgException(STDLOG, sd, ResultCode::Fatal, "send query failed");
    }
}

void PgCacheExecutor::exec(const ServerFramework::NickFileLine& pos,
        PGconn* conn, SessionDescriptor sd,
        int nParams, const char* const * values, int* lengths,
        int* formats, Oid* types, int resultFormat)
{
    if (isQueryInProgress(conn)) {
        throw PgException(pos.nick, pos.file, pos.line, sd, ResultCode::Fatal,
                "do not execute query while previous is not fully processed");
    }
    if (isPrepared) {
        if (!std::equal(prepTypes.cbegin(), prepTypes.cend(), types, types + nParams)) {
            LogTrace(TRACE5) << "old ptypes: " << LogCont(" ", prepTypes);
            LogTrace(TRACE5) << "new ptypes: " << LogCont(" ", std::vector<Oid>(types, types + nParams));
            LogError(pos.nick, pos.file, pos.line)
                << "execution plan was prepared for different parameter types;"
                << " dropping cache";
            // PostgreSQL 11 documentation, 34.3.1
            // Also, although there is no libpq function for deleting a prepared statement,
            // the SQL DEALLOCATE statement can be used for that purpose.
            checkPgErr(STDLOG, sd, PQexec(PG_CONN(conn), ("DEALLOCATE " + prepStmtId).c_str()));
            isPrepared = false;
        }
    }
    if (!isPrepared) {
        checkPgErr(pos.nick, pos.file, pos.line, sd, prepareData(conn, this, types));
        isPrepared = true;
        prepTypes.assign(types, types + nParams);
    }
    const int res = PQsendQueryPrepared(conn, prepStmtId.c_str(),
            nParams, values, lengths, formats, resultFormat);
    if (res != 1) {
        throw PgException(STDLOG, sd, ResultCode::Fatal, "send query failed");
    }
}

PgResult CursCtl::execSql()
{
    erc_ = ResultCode::Ok;
    const auto nParams = exectr_->params.size();

    std::vector<Oid> types; types.reserve(nParams);
    std::vector<const char*> values; values.reserve(nParams);
    std::vector<int> lengths; lengths.reserve(nParams);
    std::vector<int> formats; formats.reserve(nParams);
    const auto resultFormat = 0;

    for (const std::string& pn : exectr_->params) {
        auto ai = binds_.find(pn);
        if (ai == binds_.end()) {
            throw PgException(STDLOG, sess_->sd_, ResultCode::BadBind, "ERROR: no bindings for :" + pn);
        }

        details::BindArg& a = ai->second;

        if (!stable_) {
            if (!a.ind || *a.ind != -1) {
                a.setValue(a.value, a.data);
            } else {
                a.value.clear();
            }
        }
        types.push_back(a.arg.type);
        formats.push_back(a.arg.format);
        values.push_back(a.value.size()? a.value.data() : nullptr);
        lengths.push_back(a.value.size());
    }

    if (!noThrowErrors_.empty()) {
#ifdef XP_TESTING
        LogTrace(TRACE7) << sess_->sd_ << " savepoint before bad query"
            " [" << exectr_->sql << "]"
            ": expected errors " << LogCont(" ", noThrowErrors_);
#else
        LogTrace(TRACE7) << sess_->sd_ << " savepoint before bad query: expected errors " << LogCont(" ", noThrowErrors_);
#endif
        SessionManager::instance().getSessionBehaviour(sess_->sd_)
            .onBeforeBadQuery(sess_->sd_, PG_CONN(sess_->conn()));
    }

    exectr_->exec(pos_,
            PG_CONN(sess_->conn()),
            sess_->sd_,
            nParams,
            values.data(),
            lengths.data(),
            formats.data(),
            types.data(),
            resultFormat);
    if (singleRowMode_) {
        int res = PQsetSingleRowMode(PG_CONN(sess_->conn()));
        if (res != 1) {
            throw PgException(STDLOG, sess_->sd_, ResultCode::BadBind, "ERROR: failed to setup single-row-mode");
        }
    }
    PgResult res = fetcher_->getResult();
    if (!checkResult(*this, sess_->sd_, res)) {
        LogTrace(TRACE5) << sess_->sd_ << " rollback due to expected error";
        SessionManager::instance().getSessionBehaviour(sess_->sd_)
            .onAfterBadQuery(sess_->sd_, PG_CONN(sess_->conn()));
    }
    return res;
}

int CursCtl::fieldsCount() const
{
    return fields_.size();
}

bool CursCtl::fieldIsNull(const std::string& fname) const
{
    auto field = algo::find_opt<boost::optional>(fields_, StrUtils::ToUpper(fname));
    if(field) {
        return field->IsNull;
    }

    throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal,
                      "Field with name '" + fname + "' does not fetched!");
}

std::string CursCtl::fieldValue(const std::string& fname) const
{
    auto field = algo::find_opt<boost::optional>(fields_, StrUtils::ToUpper(fname));
    if(field) {
        return field->Value;
    }

    throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal,
                      "Field with name '" + fname + "' does not fetched!");
}

int CursCtl::fieldIndex(const std::string& fname) const
{
    auto field = algo::find_opt<boost::optional>(fields_, StrUtils::ToUpper(fname));
    if(field) {
        return field->Index;
    }

    return -1;
}

bool CursCtl::fieldIsNull(int fieldIndex) const
{
    auto opt = algo::find_opt_if<boost::optional>(fields_,
                  [fieldIndex](const auto& kv) { return kv.second.Index == fieldIndex; });
    if(opt) {
        return opt->second.IsNull;
    }

    throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal,
                      "Field with index '" + std::to_string(fieldIndex) + "' does not fetched!");
}

std::string CursCtl::fieldValue(int fieldIndex) const
{
    auto opt = algo::find_opt_if<boost::optional>(fields_,
                  [fieldIndex](const auto& kv) { return kv.second.Index == fieldIndex; });
    if(opt) {
        return opt->second.Value;
    }

    throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal,
                      "Field with index '" + std::to_string(fieldIndex) + "' does not fetched!");
}

std::string CursCtl::fieldName(int fieldIndex) const
{
    auto opt = algo::find_opt_if<boost::optional>(fields_,
                  [fieldIndex](const auto& kv) { return kv.second.Index == fieldIndex; });
    if(opt) {
        return opt->second.Name;
    }

    throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal,
                      "Field with index '" + std::to_string(fieldIndex) + "' does not fetched!");
}

static void setupFetcher(std::unique_ptr<PgFetcher>& f, CursCtl& c, bool singleRowMode)
{
    if (f) {
        f->clear();
    }
    if (singleRowMode) {
        f.reset(new PgSingleRowFetcher(c));
    } else {
        f.reset(new PgAllRowsFetcher(c));
    }
}

int CursCtl::exfet()
{
    if (singleRowMode_) {
        erc_ = ResultCode::Fatal;
        throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal, "do not use exfet in single-row-mode");
    }
    SessionManager::instance().activateSession(sess_->sd_);
    setupFetcher(fetcher_, *this, singleRowMode_);
    execSql();
    fen();
    return erc_;
}

int CursCtl::EXfet()
{
    if (singleRowMode_) {
        erc_ = ResultCode::Fatal;
        throw PgException(STDLOG, sess_->sd_, ResultCode::Fatal, "do not use EXfet in single-row-mode");
    }
    SessionManager::instance().activateSession(sess_->sd_);
    setupFetcher(fetcher_, *this, singleRowMode_);
    execSql();
    fetcher_->getResult();
    if (fetcher_->rowcount() > 1) {
        erc_ = ResultCode::TooManyRows;
        char errText[50] = {};
        sprintf(errText, "fetched %d rows, expected 1", fetcher_->rowcount());
        handleError(*this, sess_->sd_, erc_, errText);
        return erc_;
    }
    fen();
    return erc_;
}

int CursCtl::exec()
{
    SessionManager::instance().activateSession(sess_->sd_);
    setupFetcher(fetcher_, *this, singleRowMode_);
    execSql();
    return erc_;
}

int CursCtl::fen()
{
    if (erc_) {
        return erc_;
    }
    if (!fetcher_) {
        erc_ = ResultCode::Fatal;
        handleError(*this, sess_->sd_, erc_, "fetch is not available (forgot to call exec?)");
        return erc_;
    }
    PGresult* res = fetcher_->getResult();
    if (res == nullptr) {
        erc_ = NoDataFound;
        return erc_;
    }
    if (singleRowMode_) {
        if (!checkResult(*this, sess_->sd_, res)) {
            LogTrace(TRACE5) << sess_->sd_ << " rollback due to expected error";
            SessionManager::instance().getSessionBehaviour(sess_->sd_)
                .onAfterBadQuery(sess_->sd_, PG_CONN(sess_->conn()));
            return erc_;
        }
    }
    const int currRow = fetcher_->currentRow();
    const int sz = defs_.size();
    for (int i = 0; i < sz; ++i) {
        details::DefArg& d = defs_[i];
        if (PQgetisnull(PG_RESULT(res), currRow, i)) {
            if (d.ind) {
                *d.ind = -1;
                if (d.defaultValue >= 0) {
                    details::IDefaultValueHolder* p = defaults_[d.defaultValue]; // FIXME check for proper ind
                    p->setValue(d.value);
                } else if (!d.setNull(d.value)) {
                    erc_ = ResultCode::FetchNull;
                    char errText[50] = {};
                    sprintf(errText, "setNull failed at [%d,%d] is NULL", currRow, i);
                    handleError(*this, sess_->sd_, erc_, errText);
                    return erc_;
                }
                continue;
            } else {
                erc_ = ResultCode::FetchNull;
                char errText[50] = {};
                sprintf(errText, "value at [%d,%d] is NULL", currRow, i);
                handleError(*this, sess_->sd_, erc_, errText);
                return erc_;
            }
        }
        if (d.ind) {
            *d.ind = 0;
        }
        char* value = PQgetvalue(PG_RESULT(res), currRow, i);
        const int len = PQgetlength(PG_RESULT(res), currRow, i);
        ASSERT(PQfformat(PG_RESULT(res), i) == 0 && "only text format is supported");
        if (!d.setValue(d.value, value, len)) {
            erc_ = ResultCode::FetchFail;
            char errText[50] = {};
            sprintf(errText, "setValue failed at [%d,%d]", currRow, i);
            handleError(*this, sess_->sd_, erc_, errText);
            return erc_;
        }
    }
    fetcher_->fetchNext();
    return erc_;
}

int CursCtl::nefen()
{
    if (erc_) {
        return erc_;
    }
    if (!fetcher_) {
        erc_ = ResultCode::Fatal;
        handleError(*this, sess_->sd_, erc_, "fetch is not available (forgot to call exec?)");
        return erc_;
    }
    PGresult* res = fetcher_->getResult();
    if (res == nullptr) {
        erc_ = NoDataFound;
        return erc_;
    }
    if (singleRowMode_) {
        if (!checkResult(*this, sess_->sd_, res)) {
            LogTrace(TRACE5) << sess_->sd_ << " rollback due to expected error";
            SessionManager::instance().getSessionBehaviour(sess_->sd_)
                .onAfterBadQuery(sess_->sd_, PG_CONN(sess_->conn()));
            return erc_;
        }
    }
    const int currRow = fetcher_->currentRow();
    const int sz = PQnfields(PG_RESULT(res));
    LogTrace(TRACE5) << "sz = " << sz;
    fields_.clear();
    for(int i = 0; i < sz; ++i) {
        std::string fname = PQfname(PG_RESULT(res), i);

        LogTrace(TRACE5) << "field[" << i << "] "
                         << "name=" << fname << " "
                         << "format=" << PQfformat(PG_RESULT(res), i);

        ASSERT(PQfformat(PG_RESULT(res), i) == 0 && "only text format is supported");

        LogTrace(TRACE5) << "field name=" << fname;
        fname = StrUtils::ToUpper(fname);

        char* value = PQgetvalue(PG_RESULT(res), currRow, i);
        LogTrace(TRACE5) << "value=" << value;

        const int len = PQgetlength(PG_RESULT(res), currRow, i);
        LogTrace(TRACE5) << "len=" << len;

        bool isNull = PQgetisnull(PG_RESULT(res), currRow, i);
        LogTrace(TRACE5) << "isNull=" << isNull;

        fields_.emplace(fname, Field{fname, isNull, std::string(value, len), i });
    }

    fetcher_->fetchNext();
    return erc_;
}

int CursCtl::rowcount()
{
    return fetcher_->rowcount();
}

void CursCtl::appendBinding(const std::string& pl, const details::BindArg& ba)
{
    auto i = binds_.find(pl);
    if (i != binds_.end()) {
        LogError(pos_.nick, pos_.file, pos_.line) << "bind duplication of '" << pl << "'";
        //use last binded value on duplication
        i->second = ba;
    } else {
        binds_.emplace(pl, ba);
    }
}

bool CursCtl::hasBinding(const std::string& pl)
{
    if (exectr_->params.empty()) {
        LogTrace(TRACE1) << exectr_->sql;
        throw PgException(NICKNAME, pos_.file, pos_.line, sess_->sd_, ResultCode::BadSqlText,
                          "no placeholders in sql string");
    }
    return std::find(exectr_->params.cbegin(), exectr_->params.cend(), pl)
            != exectr_->params.cend();
}

void CursCtl::throwBadBindException(const char* f, int l, const std::string& err)
{
    LogError(STDLOG) << "error in sql from " << pos_.file << ":" << pos_.line;
    throw PgException(NICKNAME, f, l, sess_->sd_, ResultCode::BadBind, err);
}

CursCtl make_curs_(const char* n, const char* f, int l, SessionDescriptor sd, const char* sql)
{
    return SessionManager::instance().getSession(sd).createCursor(n, f, l, sql);
}

CursCtl make_curs_(const char* n, const char* f, int l, SessionDescriptor sd, const std::string& sql)
{
    return SessionManager::instance().getSession(sd).createCursor(n, f, l, sql);
}

CursCtl make_curs_autonomous_(const char* n, const char* f, int l, SessionDescriptor sd, const char* sql)
{
    return SessionManager::instance().getAutoCommitSession(sd).createCursor(n, f, l, sql);
}

CursCtl make_curs_autonomous_(const char* n, const char* f, int l, SessionDescriptor sd, const std::string& sql)
{
    return SessionManager::instance().getAutoCommitSession(sd).createCursor(n, f, l, sql);
}

CursCtl make_curs_nocache_(const char* n, const char* f, int l, SessionDescriptor sd, const char* sql)
{
    return SessionManager::instance().getSession(sd).createCursorNoCache(n, f, l, sql);
}

CursCtl make_curs_nocache_(const char* n, const char* f, int l, SessionDescriptor sd, const std::string& sql)
{
    return SessionManager::instance().getSession(sd).createCursorNoCache(n, f, l, sql);
}

} // PgCpp

#ifdef XP_TESTING
#include "test.h"
#include "xp_test_utils.h"
#include "checkunit.h"
#include "pg_rip.h"
#include "rip_validators.h"
#include "pg_row.h"
#include "savepoint.h"
#include "pg_int_parameters.h"

#ifdef make_curs
#undef make_curs
#endif // make_curs
#define make_curs(x) make_pg_curs(sd, x)
#define make_curs_nocache(x) make_pg_curs_nocache(sd, x)

namespace
{

static const char* getTestConnectString()
{
    static std::string connStr = readStringFromTcl("PG_CONNECT_STRING");
    return connStr.c_str();
}

static PgCpp::SessionDescriptor sd = 0;

void setupManaged()
{
    sd = PgCpp::getManagedSession(getTestConnectString());
}

void setupReadOnly()
{
    sd = PgCpp::getReadOnlySession(getTestConnectString());
}


START_TEST(test_readonly)
{
    int i = 0;
    make_pg_curs(sd, "SELECT 1").def(i).EXfet();
    fail_unless(i == 1, "expected 1 got %d", i);

    struct {
        const char* sql;
    } checks[] = {
        {"INSERT INTO TEST_FOR_UPDATE (FLD1) VALUES (3)"},
        {"UPDATE TEST_FOR_UPDATE SET FLD1 = 3"},
        {"DELETE FROM TEST_FOR_UPDATE"},
        {"ALTER TABLE TEST_FOR_UPDATE ADD FLD2 VARCHAR(1)"},
    };
    for (const auto& c : checks) {
        try {
            make_pg_curs(sd, c.sql)
                .exec();
            fail_if(true, "expecting exception in readonly session for [%s]", c.sql);
        } catch (const PgCpp::PgException& e) {
            LogTrace(TRACE5) << e.what();
        }
    }
} END_TEST

START_TEST(test_char_buff)
{
    char s[10] = {};
    char b1[] = "abc";
    make_curs("select 'aaaa' where :b = 'abc'")
        .def(s).bind("b", b1).EXfet();
    CHECK_EQUAL_STRINGS(s, "aaaa");

    char b2[] = "cba";
    make_curs("select 'bbbb' where :b != 'abc'")
        .def(s).bind("b", b2).EXfet();
    CHECK_EQUAL_STRINGS(s, "bbbb");

    const short bindInd = -1;
    make_curs("select 'cccc' where :b = 'cba'")
        .def(s).bind("b", b2, &bindInd).EXfet();
    CHECK_EQUAL_STRINGS(s, "bbbb");

    make_curs("select 'cccc' where :b is null")
        .def(s).bind("b", b2, &bindInd).EXfet();
    CHECK_EQUAL_STRINGS(s, "cccc");

    short defInd = 0;
    make_curs("select null")
        .def(s, &defInd).EXfet();
    CHECK_EQUAL_STRINGS(s, "");

    make_curs("select null")
        .defNull(s, "dddd").EXfet();
    CHECK_EQUAL_STRINGS(s, "dddd");

    // this should not compile: s is 10 bytes while 01234567890 is 12 bytes
    // make_curs("").defNull(s, "01234567890");
} END_TEST

START_TEST(test_cstring)
{
    char s[10] = {};
    const char* b = "abc";
    make_curs("select 'aaaa' where :b = 'abc'")
        .def(s).bind("b", b).EXfet();
    CHECK_EQUAL_STRINGS(s, "aaaa");

    b = "cba";
    make_curs("select 'bbbb' where :b != 'abc'")
        .def(s).bind("b", b).EXfet();
    CHECK_EQUAL_STRINGS(s, "bbbb");

    const short ind = -1;
    make_curs("select 'cccc' where :b = 'cba'")
        .def(s).bind("b", b, &ind).EXfet();
    CHECK_EQUAL_STRINGS(s, "bbbb");

    make_curs("select 'cccc' where :b is null")
        .def(s).bind("b", b, &ind).EXfet();
    CHECK_EQUAL_STRINGS(s, "cccc");

    short defInd = 0;
    make_curs("select null")
        .def(s, &defInd).EXfet();
    CHECK_EQUAL_STRINGS(s, "");
} END_TEST

START_TEST(test_char_buff_overflow)
{
    char s[3] = {}; // 'zzz' requires 4 bytes, we have only 3 - ignore last zero
    make_curs("select 'zzz'")
        .def(s).EXfet();
    ProgTrace(TRACE5, "[%s]", s);
    fail_unless(strncmp(s, "zzz", 3) == 0, "fetching exact size failed");
    try {
        char s[2]; // 'zzz' requires 4 bytes, we have only 2 - error
        make_curs("select 'zzz'")
            .def(s).EXfet();
        fail_if(1, "fetching into smaller buffer should fail");
    } catch (const PgCpp::PgException& e) {
        // pass
    }
} END_TEST

START_TEST(test_string)
{
    std::string s1, s2("abc");
    make_curs("select 'aaaa' where :s2='abc'")
        .def(s1).bind(":s2", s2).EXfet();
    CHECK_EQUAL_STRINGS(s1, "aaaa");

    const short ind = -1;
    make_curs("select 'bbbb' where :s2='abc'")
        .def(s1).bind(":s2", s2, &ind).EXfet();
    CHECK_EQUAL_STRINGS(s1, "aaaa");

    make_curs("select 'bbbb' where :s2 is null")
        .def(s1).bind(":s2", s2, &ind).EXfet();
    CHECK_EQUAL_STRINGS(s1, "bbbb");

    short defInd = 0;
    make_curs("select null")
        .def(s1, &defInd).EXfet();
    CHECK_EQUAL_STRINGS(s1, "");
    fail_unless(defInd = -1, "fetch NULL should set indicator to 1");

    make_curs("select null")
        .defNull(s1, "cccc").EXfet();
    CHECK_EQUAL_STRINGS(s1, "cccc");
} END_TEST

START_TEST(test_string_select_int)
{
    std::string s;
    make_curs("select :s")
        .bind(":s", 12345)
        .def(s).EXfet();
    CHECK_EQUAL_STRINGS(s, "12345");
} END_TEST

START_TEST(test_bool)
{
    bool b1 = true, b2 = false;
    make_curs("select 1 where :b1 = True")
        .bind(":b1", b1)
        .def(b2).EXfet();
    fail_unless(b2 == true, "failed: b2=%d expected true", b2);

    b1 = false;
    make_curs("select False where :b1 != True")
        .bind(":b1", b1)
        .def(b2).EXfet();
    fail_unless(b2 == false, "failed: b2=%d expected false", b2);

    const short ind = -1;
    make_curs("select True where :b1 = False")
        .bind(":b1", b1, &ind)
        .def(b2).EXfet();
    fail_unless(b2 == false, "failed: b2=%d expected false", b2);

    make_curs("select True where :b1 is null")
        .bind(":b1", b1, &ind)
        .def(b2).EXfet();
    fail_unless(b2 == true, "failed: b2=%d expected true", b2);

    short defInd = 0;
    make_curs("select null")
        .def(b2, &defInd).EXfet();
    fail_unless(b2 == false, "failed: b2=%d expected false for NULL", b2);

    make_curs("select null")
        .defNull(b2, true).EXfet();
    fail_unless(b2 == true, "failed: b2=%d expected true from default for NULL", b2);
} END_TEST

START_TEST(test_char)
{
    char a = 'a', c = 0;
    make_curs("select 'c' where :a = 'a'")
        .bind(":a", a)
        .def(c).EXfet();
    fail_unless(c == 'c', "failed: c=%c expected 'c'", c);

    a = 'b';
    make_curs("select 'd' where :a != 'a'")
        .bind(":a", a)
        .def(c).EXfet();
    fail_unless(c == 'd', "failed: c=%c expected 'd'", c);

    const short ind = -1;
    make_curs("select 'e' where :a is null")
        .bind(":a", a, &ind)
        .def(c).EXfet();
    fail_unless(c == 'e', "failed: c=%c expected 'e'", c);

    short defInd = 0;
    make_curs("select null")
        .def(c, &defInd).EXfet();
    fail_unless(c == 0, "failed: c=%c expected 0 for NULL", c);

    make_curs("select null")
        .defNull(c, 'f').EXfet();
    fail_unless(c == 'f', "failed: c=%c expected 'f' from default for NULL", c);
} END_TEST

START_TEST(test_short)
{
    short i1 = 12345, i2 = -1;
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2, "failed: i2=%d expected 2", i2);
    fail_unless(12345 == i1);

    i1 = -12345, i2 = 0;
    make_curs("select 3 where :i < -5")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(3 == i2, "failed: i2=%d expected 3", i2);

    const short ind = -1;
    make_curs("select 4 where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless(4 == i2, "failed: i2=%d expected 4", i2);

    short defInd = 0;
    make_curs("select null")
        .def(i2, &defInd).EXfet();
    fail_unless(i2 == 0, "failed: i2=%d expected 0 for NULL", i2);

    make_curs("select null")
        .defNull(i2, 5).EXfet();
    fail_unless(i2 == 5, "failed: i2=%d expected 5 from default for NULL", i2);
} END_TEST

START_TEST(test_int)
{
    int i1 = 12345, i2 = 0;
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2, "failed: i2=%d expected 2", i2);

    i1 = -12345, i2 = 0;
    make_curs("select 3 where :i < -5")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(3 == i2, "failed: i2=%d expected 3", i2);

    const short ind = -1;
    make_curs("select 4 where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless(4 == i2, "failed: i2=%d expected 4", i2);

    short defInd = 0;
    make_curs("select null")
        .def(i2, &defInd).EXfet();
    fail_unless(i2 == 0, "failed: i2=%d expected 0 for NULL", i2);

    make_curs("select null")
        .defNull(i2, 5).EXfet();
    fail_unless(i2 == 5, "failed: i2=%d expected 5 from default for NULL", i2);
} END_TEST

START_TEST(test_unsigned)
{
    unsigned i1 = 12345, i2 = 0;
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2, "failed: i2=%u expected 2", i2);
} END_TEST

START_TEST(test_long_long)
{
    long long i1 = 12345, i2 = 0;
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2, "failed: i2=%d expected 2", i2);
    fail_unless(12345 == i1);

    i1 = -12345, i2 = 0;
    make_curs("select 3 where :i < -5")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(3 == i2, "failed: i2=%d expected 3", i2);

    const short ind = -1;
    make_curs("select 21474836470 where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless(21474836470l == i2, "failed: i2=%d expected 21474836470", i2);

    short defInd = 0;
    make_curs("select null")
        .def(i2, &defInd).EXfet();
    fail_unless(i2 == 0, "failed: i2=%d expected 0 for NULL", i2);

    make_curs("select null")
        .defNull(i2, 5).EXfet();
    fail_unless(i2 == 5, "failed: i2=%d expected 5 from default for NULL", i2);
} END_TEST

START_TEST(pg_test_float)
{
    float e = 0.001f;
    float f1 = 12345.67f, f2 = 0.00f;
    make_curs("select 2.34 where ABS(12345.67 - :i) <= 0.001").bind(":i", f1).def(f2).EXfet();
    fail_unless(fabs(2.34f - f2) <= e, "failed: f2=%f expected 2.34", f2);
    fail_unless(12345.67f == f1, "failed: f1=%f expected 12345.67", f1);

    f1 = -12345.67f, f2 = 0.00f;
    make_curs("select 3.0 where :i < -5").bind(":i", f1).def(f2).EXfet();
    fail_unless(fabs(3.0f - f2) <= e, "failed: f2=%f expected 3.0", f2);

    short defInd = 0;
    make_curs("select null").def(f2, &defInd).EXfet();
    fail_unless(f2 == 0.00, "failed: f2=%f expected 0.00 for NULL", f2);

    make_curs("select null").defNull(f2, 5.1f).EXfet();
    fail_unless(f2 == 5.1f, "failed: f2=%f expected 5.1 from default for NULL", f2);
}
END_TEST

START_TEST(pg_test_double)
{
    double f1 = 12345.6789, f2 = 0.00;
    make_curs("select 2.34 where :i = float '12345.6789'").bind(":i", f1).def(f2).EXfet();
    fail_unless(2.34 == f2, "failed: f2=%lf expected 2.34", f2);
    fail_unless(12345.6789 == f1);

    f1 = -12345.67, f2 = 0.00;
    make_curs("select 3.0 where :i < -5").bind(":i", f1).def(f2).EXfet();
    fail_unless(3.0 == f2, "failed: f2=%lf expected 3.0", f2);

    short defInd = 0;
    make_curs("select null").def(f2, &defInd).EXfet();
    fail_unless(f2 == 0.00, "failed: f2=%lf expected 0.00 for NULL", f2);

    make_curs("select null").defNull(f2, 5.1).EXfet();
    fail_unless(f2 == 5.1, "failed: f2=%lf expected 5.1 from default for NULL", f2);
}
END_TEST

START_TEST(test_date)
{
    boost::gregorian::date d1(boost::gregorian::from_simple_string("1980-07-09")), d2;
    make_curs("select to_date('26/02/1984', 'DD/MM/YYYY')"
            " where :d1 = to_date('1980.07.09', 'YYYY.MM.DD')"
            " and :d1 < to_date('1980-12-31', 'YYYY-MM-DD')"
            " and :d1 > to_date('1980.01.01', 'YYYY.MM.DD')")
        .bind(":d1", d1)
        .def(d2).EXfet();
    CHECK_EQUAL_STRINGS(Dates::yyyymmdd(d2), "19840226");

    const short ind = -1;
    make_curs("select to_date('08.11.2012', 'DD.MM.YYYY') where :d1 is null")
        .bind(":d1", d1, &ind)
        .def(d2).EXfet();
    CHECK_EQUAL_STRINGS(Dates::yyyymmdd(d2), "20121108");

    short defInd = 0;
    make_curs("select null")
        .def(d2, &defInd).EXfet();
    fail_unless(defInd == -1, "failed: defInd=%d expected -1 for NULL", defInd);
    fail_unless(d2.is_not_a_date() == true, "failed: not-a-date expected for NULL");

    make_curs("select null")
        .defNull(d2, boost::gregorian::date(2012, 11, 8)).EXfet();
    CHECK_EQUAL_STRINGS(Dates::yyyymmdd(d2), "20121108");
} END_TEST

START_TEST(test_datetime)
{
    const boost::posix_time::ptime t1(boost::gregorian::date(1980, 7, 9),
            boost::posix_time::time_duration(4, 30, 11) + boost::posix_time::millisec(3));
    boost::posix_time::ptime t2;

    make_curs("select to_timestamp('26/02/1984 06:20:58.43', 'DD/MM/YYYY HH24:MI:SS.MS')"
            " where :t1 = to_timestamp('1980.07.09 04:30:11.003', 'YYYY.MM.DD HH24:MI:SS.MS')"
            " and :t1 > to_timestamp('19800709 043011.002', 'YYYYMMDD HH24MISS.MS')"
            " and :t1 < to_timestamp('19800709 043011.004', 'YYYYMMDD HH24MISS.MS')")
        .bind(":t1", t1)
        .def(t2).EXfet();
    CHECK_EQUAL_STRINGS(Dates::ptime_to_undelimited_string(t2), "19840226062058");
    int fsec = t2.time_of_day().total_milliseconds() % 1000;
    fail_unless(fsec == 430, "expected 430ms got %d", fsec);

    const short ind = -1;
    make_curs("select to_timestamp('20150722 093251.082', 'YYYYMMDD HH24MISS.MS') where :t1 is null")
        .bind(":t1", t1, &ind)
        .def(t2).EXfet();
    CHECK_EQUAL_STRINGS(Dates::ptime_to_undelimited_string(t2), "20150722093251");
    fsec = t2.time_of_day().total_milliseconds() % 1000;
    fail_unless(fsec == 82, "expected 82ms got %d", fsec);

    // no timezone in the server response
    make_curs("select timestamp '1984-02-26 06:20:58.43'").def(t2).EXfet();
    CHECK_EQUAL_STRINGS(Dates::ptime_to_undelimited_string(t2), "19840226062058");

    // microseconds in timestamp rounded to milliseconds
    make_curs("select timestamp '2020-04-15 15:51:28.049890+0'").def(t2).EXfet();
    CHECK_EQUAL_STRINGS(to_iso_string(t2), "20200415T155128.049000");
    make_curs("select timestamp '2020-04-15 15:51:28.004985'").def(t2).EXfet();
    CHECK_EQUAL_STRINGS(to_iso_string(t2), "20200415T155128.004000");
    make_curs("select timestamp '2020-04-15 15:51:28.00498'").def(t2).EXfet();
    CHECK_EQUAL_STRINGS(to_iso_string(t2), "20200415T155128.004000");
    make_curs("select timestamp '2020-04-15 15:51:28.1049'").def(t2).EXfet();
    CHECK_EQUAL_STRINGS(to_iso_string(t2), "20200415T155128.104000");

    make_curs("set time zone -7").exec();
    make_curs("select to_timestamp('26/02/1984 06:20:58.43', 'DD/MM/YYYY HH24:MI:SS.MS')")
        .def(t2).EXfet();
    CHECK_EQUAL_STRINGS(boost::posix_time::to_iso_extended_string(t2), "1984-02-26T06:20:58.430000");

    short defInd = 0;
    make_curs("select null")
        .def(t2, &defInd).EXfet();
    fail_unless(defInd == -1, "failed: defInd=%d expected -1 for NULL", defInd);
    fail_unless(t2.is_not_a_date_time() == true, "failed: not-a-date expected for NULL");

    make_curs("select null")
        .defNull(t2, t1).EXfet();
    CHECK_EQUAL_STRINGS(Dates::ptime_to_undelimited_string(t2), "19800709043011");
} END_TEST

START_TEST(test_datetime_with_tz)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_TIME_WITH_TZ").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_TIME_WITH_TZ ( FLD1 TIME WITH TIME ZONE );").exec();
    make_pg_curs_autonomous(sd, "INSERT INTO TEST_TIME_WITH_TZ VALUES ('2003-04-12 04:05:06 America/New_York')").exec();
    PgCpp::commit();
    boost::posix_time::ptime t1;
    try {
        PgCpp::CursCtl cr(make_curs("INSERT INTO TEST_TIME_WITH_TZ (FLD1) VALUES (:t1_)"));
        cr.bind(":t1_", Dates::currentDateTime())
            .exec();
        fail_if(true, "bind ptime to 'time with time zone' is invalid and should throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::BadSqlText, "ptime is not compatible with 'time with time zone' type");
    }
    PgCpp::rollback();
    try {
        make_curs("SELECT FLD1 FROM TEST_TIME_WITH_TZ")
            .def(t1)
            .noThrowError(PgCpp::Busy)
            .exfet();
        fail_if(true, "def ptime to 'time with time zone' is invalid and should throw");
    } catch (const PgCpp::PgException& e) {
        LogTrace(TRACE5) << e.errCode();
        fail_unless(e.errCode() == PgCpp::FetchFail, "ptime is not compatible with 'time with time zone' type");
    }

} END_TEST

DECL_RIP(RipInt, int);

START_TEST(test_rip_int)
{
    RipInt i1(12345), i2(0);
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2.get(), "failed: i2=%d expected 2", i2.get());

    i1 = RipInt(-12345), i2 = RipInt(0);
    make_curs("select 3 where :i < -5")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(3 == i2.get(), "failed: i2=%d expected 3", i2.get());

    const short ind = -1;
    make_curs("select 2147483643 where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless(2147483643 == i2.get(), "failed: i2=%d expected 2147483643", i2.get());


    try {
        short defInd = 0;
        make_curs("select null")
            .def(i2, &defInd).EXfet();
        fail_unless(true, "expected exception for NULL even with indicator");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::FetchNull, "%d", e.errCode());
    }

    make_curs("select null")
        .defNull(i2, 5).EXfet();
    fail_unless(i2.get() == 5, "failed: i2=%d expected 5 from default for NULL", i2.get());
} END_TEST

DECL_RIP_LENGTH(RipStr, std::string, 1, 4);

START_TEST(test_rip_string)
{
    RipStr i1("val"), i2("-");
    make_curs("select 2 where :i = 'val'")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless("2" == i2.get(), "failed: i2=%s expected '2'", i2.get().c_str());

    const short ind = -1;
    make_curs("select 'VAL' where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless("VAL" == i2.get(), "failed: i2=%s expected 'VAL'", i2.get().c_str());

    try {
        short defInd = 0;
        make_curs("select null")
            .def(i2, &defInd).EXfet();
        fail_unless(true, "expected exception for NULL even with indicator");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::FetchNull, "%d", e.errCode());
    }

    try {
        make_curs("select 'VALUE'")
            .def(i2).EXfet();
        fail_unless(true, "expected exception for bad value");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::FetchFail, "%d", e.errCode());
    }

    make_curs("select null")
        .defNull(i2, "wow").EXfet();
    fail_unless(i2.get() == "wow", "failed: i2=%s expected 'wow' from default for NULL", i2.get().c_str());
} END_TEST

MakeIntParamType(ParIntInt, int);

START_TEST(test_parint_int)
{
    ParIntInt i1(12345), i2(0);
    make_curs("select 2 where :i = 12345")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(2 == i2.get(), "failed: i2=%d expected 2", i2.get());

    i1 = ParIntInt(-12345), i2 = ParIntInt(0);
    make_curs("select 3 where :i < -5")
        .bind(":i", i1)
        .def(i2).EXfet();
    fail_unless(3 == i2.get(), "failed: i2=%d expected 3", i2.get());

    const short ind = -1;
    make_curs("select 2147483643 where :i is null")
        .bind(":i", i1, &ind)
        .def(i2).EXfet();
    fail_unless(2147483643 == i2.get(), "failed: i2=%d expected 2147483643", i2.get());

    short defInd = 0;
    make_curs("select null")
        .def(i2, &defInd).EXfet();
    fail_unless(i2.valid() == false, "expected empty parint for null");
    fail_unless(defInd == -1, "failed: defInd=%d expected -1 for NULL", defInd);

    make_curs("select null")
        .defNull(i2, 5).EXfet();
    fail_unless(i2.get() == 5, "failed: i2=%d expected 5 from default for NULL", i2.get());
} END_TEST

START_TEST(test_byte_array)
{
    using byte_array = std::array<char, 4>;
    byte_array s1{{'a', '\0', 3, 'z'}};
    byte_array s2{};
    const byte_array s3{{'\0', 'b', 'c', '\2'}};
    const byte_array s4{{'\1', 'a', 'b', '\3'}};
    make_curs("select E'\\\\x00626302' where :s1 = E'\\\\x6100037a'")
        .def(s2).bind("s1", s1).EXfet();
    fail_unless(s2 == s3, "def failed");

    s1 = {{'a', 'b', 'c', 'd'}};
    make_curs("select E'\\\\x01616203' where :s1 != E'\\\\x6100037a'")
        .def(s2).bind(":s1", s1).EXfet();
    ProgTrace(TRACE5, "%s", HelpCpp::memdump(s2.data(), s2.size()).c_str());
    fail_unless(s2 == s4, "def failed");

    const short bindInd = -1;
    make_curs("select E'\\\\x00626302' where :s1 = E'\\\\x6100037a'")
        .def(s2).bind("s1", s1, &bindInd).EXfet();
    fail_unless(s2 == s4, "expected no changes where nothing found");

    make_curs("select E'\\\\x00626302' where :s1 is null")
        .def(s2).bind("s1", s1, &bindInd).EXfet();
    fail_unless(s2 == s3, "expected changes when is NULL");

    short defInd = 0;
    make_curs("select null")
        .def(s2, &defInd).EXfet();
    fail_unless(defInd == -1, "failed: defInd=%d expected -1 for NULL", defInd);
    fail_unless(s2 == byte_array(), "expected zero-filled when NULL");

    make_curs("select null")
        .defNull(s2, s4).EXfet();
    fail_unless(s2 == s4, "defNull failed");

    // this should not compile: s2 is 4 bytes while s22 is 12 bytes
    //std::array<char, 12> s22;
    //make_curs("select null").defNull(s2, s22);
} END_TEST

START_TEST(test_null)
{
    std::string s("321");
    try {
        make_curs("select null").def(s).EXfet();
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::FetchNull, "fetch NULL without an indicator should fail");
    }
    CHECK_EQUAL_STRINGS(s, "321");
    short defInd = 0;
    make_curs("select null").def(s, &defInd).EXfet();
    fail_unless(defInd == -1, "fetch NULL should set indicator to 1");
    fail_unless(s.empty(), "fetch NULL should set NULL value for type");

    make_curs("select 'aaaa' where :s is null")
        .def(s).bind("s", std::string()).EXfet();
    fail_unless(s == "aaaa", "empty std::string should be treate as NULL");

    char emptyBuff[3] = {};
    make_curs("select 'bbbb' where :s is null")
        .def(s).bind("s", emptyBuff).EXfet();
    fail_unless(s == "bbbb", "empty char-buffer should be treate as NULL");

    const char* emptyCstr = nullptr;
    make_curs("select 'cccc' where :s is null")
        .def(s).bind("s", emptyCstr).EXfet();
    fail_unless(s == "cccc", "empty C-string should be treate as NULL");
} END_TEST

START_TEST(test_autonull)
{
    std::string s1("321"), s2;
    int i1 = 123, i2 = 0;
    long long l1 = 21474836470, l2 = 0;
    make_curs("select null, 'ss', null, 22, null, 21474836475")
        .autoNull()
        .def(s1).def(s2).def(i1).def(i2).def(l1).def(l2)
        .EXfet();
    fail_unless(s1.empty(), "expected empty string for NULL");
    fail_unless(s2 == "ss", "expected 'ss'");
    fail_unless(i1 == 0, "expected 0 for NULL");
    fail_unless(i2 == 22, "expected 22");
    fail_unless(l1 == 0, "expected 0 for NULL");
    fail_unless(l2 = 21474836475, "expected 21474836475");
} END_TEST

START_TEST(test_bind)
{
    const boost::gregorian::date dt(2017, 6, 29);

    boost::gregorian::date d_dt;
    int d_id = 0, d_awk = 0, d_pc = 0, d_id_ = 0;
    std::string d_cmd;

    //arbitrary bind order
    make_curs(
        "select x.id, x.dt, x.pc, x.awk, :id_, x.cmd from"
        " (select :id id, :pc pc, :awk awk, :fln fln, :dt_ dt, :cmd cmd) x"
        " where x.id != :id_"
    )
    .def(d_id).def(d_dt).def(d_pc).def(d_awk).def(d_id_).def(d_cmd)
    .bind(":cmd", RipStr("1234")).bind(":id", 10).bind(":id_", 20)
    .bind(":awk", 199).bind(":fln", 111).bind(":dt_", dt).bind(":pc", 1)
    .EXfet();

    fail_unless(d_id == 10, "expected 10");
    fail_unless(d_dt == dt, "invalid date");
    fail_unless(d_pc == 1, "expected 1");
    fail_unless(d_id_ == 20, "expected 20");
    fail_unless(d_awk == 199, "expected 199");
    fail_unless(d_cmd == "1234", "invalid rip");

    //missing bind
    try {
        make_curs(
            "select x.id, x.dt, x.pc, x.awk, :id_, x.cmd from"
            " (select :id id, :pc pc, :awk awk, :fln fln, :dt_ dt, :cmd cmd) x"
            " where x.id != :id_"
        )
        .def(d_id).def(d_dt).def(d_pc).def(d_awk).def(d_id_).def(d_cmd)
        .bind(":cmd", RipStr("1234")).bind(":id", 10).bind(":id_", 20)
        .bind(":awk", 199).bind(":fln", 111).bind(":dt", dt).bind(":pc", 1)
        .EXfet();

        fail_unless(false, "expected exception for missing bind");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::BadBind, "%d", e.errCode());
    }

    //duplicated bind - will use last binded value
    PgCpp::CursCtl cc = make_curs(
        "select x.id, x.dt, x.pc, x.awk, :id_, x.cmd from"
        " (select :id id, :pc pc, :awk awk, :fln fln, :dt_ dt, :cmd cmd) x"
        " where x.id = :id_"
    );
    cc.def(d_id).def(d_dt).def(d_pc).def(d_awk).def(d_id_).def(d_cmd)
      .bind(":cmd", RipStr("1234")).bind(":id", 10).bind(":id_", 20)
      .bind(":awk", 199).bind(":fln", 111).bind(":dt_", dt).bind(":pc", 1)
      .bind(":id", 20);

    fail_unless(cc.EXfet() == PgCpp::Ok, "record should be found");
    fail_unless(d_id == 20, "expected 20");
    fail_unless(d_id_ == 20, "expected 20");
}
END_TEST

START_TEST(test_bind_ParNotEq1)
{
    std::string _inp="1";
    std::string _res;

    make_curs("select :inp where :inp!=' '" ).
    def(_res).
    bind(":inp", _inp).
    EXfet();
    LogTrace(TRACE5)<<"_inp='"<<_inp<<"' _res='"<<_res<<"'";
    fail_unless(_res ==_inp, "expected 1");
}
END_TEST

START_TEST(test_bind_ParNotEq2)
{
    std::string _inp="1";
    int _res;

    make_curs("select 1 where :inp!=' '" ).
    def(_res).
    bind(":inp", _inp).
    EXfet();
    LogTrace(TRACE5)<<"_inp='"<<_inp<<"' _res="<<_res;
    fail_unless(_res == 1, "expected 1");
}
END_TEST

int get_an_int()
{
    return 5;
}

START_TEST(test_rvalue_bind)
{
    auto cur1 = make_curs("SELECT :v");
    int ret;

    cur1.stb().bind(":v", get_an_int()).def(ret).EXfet();
    fail_unless(ret == 5, "stable rvalue bind failed: ret = %d", ret);

    auto cur2 = make_curs("SELECT :v");
    try {
        cur2.unstb().bind(":v", get_an_int());
        fail_unless(false, "expected throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::BadBind,
                    "expected BadBind, got %d", e.errCode());
    }
} END_TEST

START_TEST(test_stb_bind)
{
    auto cur = make_curs("SELECT :v");
    cur.stb();

    int i = 10, j;
    cur.bind(":v", i).def(j);

    i = 15;
    cur.EXfet();

    fail_unless(j == 10, "stable bind failed, j = %d", j);
} END_TEST

START_TEST(test_unstb_bind)
{
    auto cur = make_curs("SELECT :v");
    cur.unstb();

    int i = 10, j;
    cur.bind(":v", i).def(j);

    i = 15;
    cur.EXfet();

    fail_unless(j == 15, "unstable bind failed, j = %d", j);
} END_TEST

START_TEST(test_second_exec_after_error)
{
    int v = 0;
    auto cur = make_curs("SELECT 1");
    cur.def(v);
    cur.exec();
    cur.fen();
    fail_unless(v == 1, "initial select failed");
    const auto res = cur.fen();
    fail_unless(res == PgCpp::NoDataFound, "expected NoDataFound after fetching single row");
    v = 0;
    cur.exec();
    cur.fen();
    fail_unless(v == 1, "repeated select failed");
} END_TEST

START_TEST(test_bind_mode_change)
{
    auto cur = make_curs("SELECT :v");
    cur.stb().bind(":v", 5);

    try {
        cur.unstb();
        fail_unless(false, "expected throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::BadBind,
            "expected BadBind, got %d", e.errCode());
    }
} END_TEST

static void test_nothrow_fetch_null__(PgCpp::SessionDescriptor sd)
{
    using namespace PgCpp;
    std::string s("321");
    PgCpp::CursCtl cr = make_curs("select null");
    cr.noThrowError(ResultCode::FetchNull)
        .noThrowError(ResultCode::TooManyRows)
        .noThrowError(ResultCode::Busy)
        .def(s).EXfet();
    fail_unless(s == "321", "no changes are expected");
    fail_unless(cr.err() == ResultCode::FetchNull, "expected CERR_NULL got %d", cr.err());
}

static void test_modes(void(*f)(PgCpp::SessionDescriptor), std::vector<PgCpp::SessionType> modes)
{
    for (auto m : modes) {
        LogTrace(TRACE5) << "start testing mode " << m;
        switch (m) {
        case PgCpp::SessionType::Managed: {
            sd = PgCpp::getManagedSession(getTestConnectString());
            break;
        }
        case PgCpp::SessionType::ReadOnly: {
            sd = PgCpp::getReadOnlySession(getTestConnectString());
            break;
        }
        case PgCpp::SessionType::AutoCommit: {
            sd = PgCpp::getAutoCommitSession(getTestConnectString());
            break;
        }
        }
        f(sd);
    }
}

START_TEST(test_nothrow_fetch_null)
{
    test_modes(test_nothrow_fetch_null__,
            {PgCpp::SessionType::Managed, PgCpp::SessionType::ReadOnly, PgCpp::SessionType::AutoCommit});
} END_TEST

static void test_nothrow_too_many_rows__(PgCpp::SessionDescriptor sd)
{
    using namespace PgCpp;
    std::string s("321");
    PgCpp::CursCtl cr = make_curs("select 1 union select 2");
    cr.noThrowError(ResultCode::TooManyRows)
        .def(s).EXfet();
    fail_unless(s == "321", "no changes are expected");
    fail_unless(cr.err() == ResultCode::TooManyRows, "expected CERR_EXACT got %d", cr.err());

    PgCpp::CursCtl cr2 = make_curs("select 1 union select 2");
    try {
        cr2.noThrowError(ResultCode::FetchNull).noThrowError(ResultCode::Busy) // ignore only NULL and BUSY
            .def(s).EXfet();
        fail_unless(s == "321", "no changes are expected");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == ResultCode::TooManyRows, "expected CERR_EXACT");
    }
    fail_unless(cr2.err() == ResultCode::TooManyRows, "expected CERR_EXACT got %d", cr2.err());
}

START_TEST(test_nothrow_too_many_rows)
{
    test_modes(test_nothrow_too_many_rows__,
            {PgCpp::SessionType::Managed, PgCpp::SessionType::ReadOnly, PgCpp::SessionType::AutoCommit});
} END_TEST

static void test_nothrow_uniq_constr__(PgCpp::SessionDescriptor sd)
{
    using PgCpp::ResultCode;
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_NOTHROW").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_NOTHROW ( FLD1 VARCHAR(1) NOT NULL );").exec();
    make_pg_curs_autonomous(sd, "CREATE UNIQUE INDEX TEST_NOTHROW_FLD1 ON TEST_NOTHROW (FLD1);").exec();

    make_curs("INSERT INTO TEST_NOTHROW(FLD1) VALUES ('1')").exec();
    PgCpp::CursCtl cr = make_curs("INSERT INTO TEST_NOTHROW(FLD1) VALUES ('1')");
    cr.noThrowError(ResultCode::ConstraintFail)
        .exec();
    fail_unless(cr.err() == ResultCode::ConstraintFail, "expected ConstraintFail got %d", cr.err());

    make_curs("INSERT INTO TEST_NOTHROW(FLD1) VALUES ('2')").exec();
    int cnt = 0;
    make_curs("SELECT COUNT(*) FROM TEST_NOTHROW").def(cnt).EXfet();
    fail_unless(cnt == 2, "number of rows failed: expected %d got %d", 2, cnt);

    try {
        make_curs("INSERT INTO TEST_NOTHROW(FLD1) VALUES ('1')").exec();
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == ResultCode::ConstraintFail, "expected ConstraintFail got %d", e.errCode());
        PgCpp::rollbackInTestMode();
    }
}

START_TEST(test_nothrow_uniq_constr)
{
    test_modes(test_nothrow_uniq_constr__,
            {PgCpp::SessionType::Managed, PgCpp::SessionType::AutoCommit});
} END_TEST

START_TEST(test_rowcount)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_ROWCOUNT").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_ROWCOUNT ( FLD1 VARCHAR(1) NOT NULL );").exec();
    {
        int v = 0, i = 0;
        PgCpp::CursCtl cr = make_curs("SELECT 1 UNION SELECT 2 UNION SELECT 3 ORDER BY 1");
        cr.def(v).exec();
        while (!cr.fen()) {
            ++i;
            fail_unless(v == i, "v=%d expected %d", v, i);
        }
        fail_unless(cr.rowcount() == 3, "rowcount=%d expected 3", cr.rowcount());
    }

    for (int i = 0; i < 2; ++i) {
        PgCpp::CursCtl cr = make_curs("INSERT INTO TEST_ROWCOUNT (FLD1) VALUES ('1')");
        cr.exec();
        fail_unless(cr.rowcount() == 1, "expected 1 after inserting 1 row but get %d", cr.rowcount());
    }
    {
        PgCpp::CursCtl cr = make_curs("UPDATE TEST_ROWCOUNT SET FLD1 = '3'");
        cr.exec();
        fail_unless(cr.rowcount() == 2, "expected 2 after updating 2 rows but got %d", cr.rowcount());
    }

    {
        PgCpp::CursCtl cr = make_curs("delete from TEST_ROWCOUNT where FLD1 = '3'");
        cr.exec();
        LogTrace(TRACE3) << "rowcount = " << cr.rowcount();
        fail_unless(cr.rowcount() == 2, "expected 2 rows deleted, got %d", cr.rowcount());
    }
} END_TEST

START_TEST(test_bind_int_after_long_int)
{
  make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_BIND_INT").exec();
  make_pg_curs_autonomous(sd, "CREATE TABLE TEST_BIND_INT ( FLD1 INTEGER );").exec();
  make_pg_curs_autonomous(sd, "INSERT INTO TEST_BIND_INT (FLD1) VALUES(999);").exec();
  std::string query="SELECT COUNT(*) FROM TEST_BIND_INT WHERE FLD1=:FLD";

  {// make cached cursor and bind long long int
    long long int fld=999;

    int tmp=0;
    PgCpp::CursCtl cr = make_curs(query);
    cr.bind(":FLD",fld);
    cr.def(tmp);
    cr.exec();
    cr.fen();
    fail_unless(tmp == 1, "tmp=%d expected 1", tmp);
  }

  { //reuse cached cursor and bind int
    int fld=999;

    int tmp=0;
    PgCpp::CursCtl cr = make_curs(query);
    cr.bind(":FLD",fld);
    cr.def(tmp);
    cr.exec();
    cr.fen();
    fail_unless(tmp == 1, "tmp=%d expected 1", tmp);
  }

}
END_TEST

enum TestEnum { te_1 = 1, te_2 = 2 };

START_TEST(test_row)
{
    // check def for several rows
    PgCpp::Row<int, std::string, TestEnum> row;
    PgCpp::CursCtl cr(make_curs("select s.* from ("
                " select 1, 'lol', 2"
                " union"
                " select 2, 'ka', 4"
                ") s order by 1"));
    cr.defRow(row);
    cr.exec();
    int i = 0;
    while (!cr.fen()) {
        if (i == 0) {
            fail_unless(row.get<0>() == 1);
            fail_unless(row.get<1>() == "lol");
            fail_unless(row.get<2>() == TestEnum(2));
        } else if (i == 1) {
            fail_unless(row.get<0>() == 2);
            fail_unless(row.get<1>() == "ka");
            // конструктор enum не проверяет значение
            // поэтому имеем такой эффект
            fail_unless(row.get<2>() == TestEnum(4));
        }
        ++i;
    }
} END_TEST

START_TEST(test_row_null)
{
    PgCpp::Row<boost::optional<int>> row;
    PgCpp::CursCtl cr(make_curs("select s.v from ("
                " select 1 as n, NULL as v"
                " union"
                " select 2 as n, 3 as v"
                " union"
                " select 3 as n, NULL as v) s order by n"));
    cr.defRow(row);
    cr.exec();
    int i = 0;
    while (!cr.fen()) {
        if (i == 0 || i == 2) {
            fail_unless(static_cast<bool>(row.get<0>()) == false, "row %d expecting empty optional", i);
        } else if (i == 1) {
            fail_unless(static_cast<bool>(row.get<0>()) == true, "row %d expecting value in optional", i);
            fail_unless(row.get<0>().get() == 3);
        }
        ++i;
    }
} END_TEST

START_TEST(test_func)
{
    int cnt = 0;
    make_curs("CREATE OR REPLACE FUNCTION TEST_FUNC() RETURNS INT AS $$DECLARE\n"
            "  cnt integer;\n"
            "BEGIN\n"
            "  cnt := 5;\n"
            "  cnt := cnt + 4;\n"
            "  RETURN cnt;\n"
            "END; $$ LANGUAGE plpgsql;")
        .exec();
    make_curs("SELECT TEST_FUNC()")
        .def(cnt)
        .EXfet();
    fail_unless(cnt == 9, "cnt=%d expected 9", cnt);
} END_TEST

static int countLines(PgCpp::SessionDescriptor sd, const std::string& table)
{
    int cnt = 0;
    const std::string sql = "SELECT COUNT(*) FROM " + table;
    PgCpp::CursCtl cr = make_pg_curs(sd, sql);
    cr.def(cnt);
    cr.EXfet();
    return cnt;
}

START_TEST(test_savepoint)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_SAVEPOINTS").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_SAVEPOINTS ( FLD1 VARCHAR(1))").exec();

    int cnt = countLines(sd, "TEST_SAVEPOINTS");
    fail_unless(cnt == 0, "expected cnt=0 got %d", cnt);
    PgCpp::Savepoint sp1(sd, "SP1");
    PgCpp::Savepoint sp2(sd, "SP2");
    make_curs("INSERT INTO TEST_SAVEPOINTS (FLD1) VALUES ('1')").exec();
    make_curs("INSERT INTO TEST_SAVEPOINTS (FLD1) VALUES ('2')").exec();
    sp1.reset();
    cnt = countLines(sd, "TEST_SAVEPOINTS");
    fail_unless(cnt == 2, "expected cnt=2 got %d", cnt);
    make_curs("INSERT INTO TEST_SAVEPOINTS (FLD1) VALUES ('3')").exec();
    cnt = countLines(sd, "TEST_SAVEPOINTS");
    fail_unless(cnt == 3, "expected cnt=3 got %d", cnt);
    sp1.rollback();
    cnt = countLines(sd, "TEST_SAVEPOINTS");
    fail_unless(cnt == 2, "expected cnt=2 got %d", cnt);
    sp2.rollback();
    cnt = countLines(sd, "TEST_SAVEPOINTS");
    fail_unless(cnt == 0, "expected cnt=0 got %d", cnt);
} END_TEST

START_TEST(test_for_update)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_FOR_UPDATE").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_FOR_UPDATE ( FLD1 VARCHAR(1) );").exec();
    make_pg_curs_autonomous(sd, "INSERT INTO TEST_FOR_UPDATE VALUES ('1')").exec();
    make_pg_curs_autonomous(sd, "INSERT INTO TEST_FOR_UPDATE VALUES ('2')").exec();
    sd = nullptr; // do not use sd, use separate sessions

    auto s1 = PgCpp::Session::create(getTestConnectString());
    s1->createCursor(STDLOG, "BEGIN").exec();
    auto s2 = PgCpp::Session::create(getTestConnectString());
    s2->createCursor(STDLOG, "BEGIN").exec();
    std::string f1;
    s1->createCursor(STDLOG, "SELECT FLD1 FROM TEST_FOR_UPDATE WHERE FLD1 = '1' FOR UPDATE")
        .def(f1)
        .EXfet();
    fail_unless(f1 == "1", "expected successfull fetch");

    std::string f2;
    s2->createCursor(STDLOG, "SELECT FLD1 FROM TEST_FOR_UPDATE FOR UPDATE SKIP LOCKED")
        .def(f2)
        .EXfet();
    fail_unless(f2 == "2", "expected no fetch");

    f2.clear();
    PgCpp::CursCtl cr = s2->createCursor(STDLOG,
            "SELECT FLD1 FROM TEST_FOR_UPDATE WHERE FLD1 = '1' FOR UPDATE NOWAIT");
    cr.noThrowError(PgCpp::Busy)
        .def(f2)
        .EXfet();
    fail_unless(cr.err() == PgCpp::Busy, "expected busy error got %d", cr.err());
    fail_unless(f2.empty(), "expected no fetch");
} END_TEST

START_TEST(test_syntax_error)
{
    auto cur = make_curs("SELEC 1");
    int tmp = -1;
    cur.def(tmp);

    try {
        cur.EXfet();
        fail_unless(false, "expected throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::BadSqlText,
            "expected BadSqlText, got %d", e.errCode());
    }
    fail_unless(tmp == -1, "expected no change");
} END_TEST

START_TEST(test_bind_typo)
{
    auto cur = make_curs("SELECT :bindme");
    try {
        cur.bind(":bindmE", 1);
        fail_unless(false, "expected throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::BadBind,
            "expected BadBind, got %d", e.errCode());
    }
} END_TEST

START_TEST(test_date_special)
{
    namespace dt = boost::gregorian;

    dt::date date1(dt::pos_infin);
    dt::date ndate1;
    make_curs("SELECT :s").bind(":s", date1).def(ndate1).EXfet();
    fail_unless(ndate1.is_pos_infinity(),
        "expected posinf, got %s", dt::to_simple_string(ndate1).c_str());

    dt::date date2(dt::neg_infin);
    dt::date ndate2;
    make_curs("SELECT :s").bind(":s", date2).def(ndate2).EXfet();
    fail_unless(ndate2.is_neg_infinity(),
        "expected neginf, got %s", dt::to_simple_string(ndate2).c_str());

    dt::date date3(dt::not_a_date_time);
    dt::date ndate3;
    short nullInd = 0;
    make_curs("SELECT :s").bind(":s", date3).def(ndate3, &nullInd).EXfet();
    fail_unless(ndate3.is_not_a_date(),
        "expected not-a-date-time, got %s", dt::to_simple_string(ndate3).c_str());

    int r = 0;
    make_curs("SELECT 1 WHERE :v is null").bind(":v", date3).def(r).EXfet();
    fail_unless(r == 1, "expected not-a-date-time to be null");
} END_TEST

START_TEST(test_datetime_special)
{
    namespace dt = boost::posix_time;

    dt::ptime time1(dt::pos_infin);
    dt::ptime ntime1;
    make_curs("SELECT :s").bind(":s", time1).def(ntime1).EXfet();
    fail_unless(ntime1.is_pos_infinity(),
        "expected posinf, got %s", dt::to_simple_string(ntime1).c_str());

    dt::ptime time2(dt::neg_infin);
    dt::ptime ntime2;
    make_curs("SELECT :s").bind(":s", time2).def(ntime2).EXfet();
    fail_unless(ntime2.is_neg_infinity(),
        "expected neginf, got %s", dt::to_simple_string(ntime2).c_str());

    dt::ptime time3(dt::not_a_date_time);
    dt::ptime ntime3;
    short nullInd = 0;
    make_curs("SELECT :s").bind(":s", time3).def(ntime3, &nullInd).EXfet();
    fail_unless(ntime3.is_not_a_date_time(),
        "expected not-a-date-time, got %s", dt::to_simple_string(ntime3).c_str());

    int r = 0;
    make_curs("SELECT 1 WHERE :v is null").bind(":v", time3).def(r).EXfet();
    fail_unless(r == 1, "expected not-a-date-time to be null");
} END_TEST

START_TEST(test_loop_bind)
{
    auto cur = make_curs("SELECT :v");

    std::vector<int> data(100);
    for (auto i = 0; i != 100; ++i) {
        data[i] = i - 50;
    }

    int i = 0, r;

    cur.unstb().bind(":v", i).def(r);

    for (auto l: data) {
        i = l;
        cur.EXfet();
        fail_unless(r == l, "loop fetch failed");
    }
} END_TEST

START_TEST(test_stb_ind_change)
{
    std::string a("Hello"), b;
    short ind = 0;

    auto cur = make_curs("select :v");
    cur.stb().bind(":v", a, &ind).def(b).EXfet();
    CHECK_EQUAL_STRINGS(a, b);

    ind = -1;
    cur.EXfet();
    CHECK_EQUAL_STRINGS(a, b);
} END_TEST

START_TEST(test_unstb_ind_change)
{
    std::string a("Hello"), b;
    short ind = 0;

    auto cur = make_curs("select :v");
    cur.unstb().bind(":v", a, &ind).def(b).EXfet();
    CHECK_EQUAL_STRINGS(a, b);

    try {
        ind = -1;
        cur.EXfet();
        fail_unless(false, "expected throw");
    } catch (const PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::FetchNull,
            "expected FetchNull, got %d", e.errCode());
    }
    ind = 0;
    cur.EXfet();
    CHECK_EQUAL_STRINGS(a, b);
} END_TEST

START_TEST(test_autonomous)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_AUTONOMOUS").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_AUTONOMOUS ( FLD1 VARCHAR(1) NOT NULL );").exec();
    int count = 0;
    make_pg_curs(sd, "SELECT COUNT(*) FROM TEST_AUTONOMOUS")
        .def(count)
        .EXfet();
    make_pg_curs(sd, "INSERT INTO TEST_AUTONOMOUS (FLD1) VALUES (1)")
        .exec();
    ++count;
    int newCount = 0;
    make_pg_curs(sd, "SELECT COUNT(*) FROM TEST_AUTONOMOUS")
        .def(newCount)
        .EXfet();
    fail_unless(newCount == count, "expected %d rows got %d", count, newCount);
    make_pg_curs_autonomous(sd, "SELECT COUNT(*) FROM TEST_AUTONOMOUS")
        .def(newCount)
        .EXfet();
    fail_unless(newCount < count, "expected rows from managed (%d) > rows from autocommit (%d)", count, newCount);
} END_TEST

START_TEST(test_cursor_cache)
{
    auto sess = PgCpp::Session::create(getTestConnectString());
    fail_unless(sess->getCursorCache().size() == 0, "initial cursor cache not empty");

    auto cur1 = sess->createCursor(STDLOG, "select :v1 where :v2 = :v3");
    fail_unless(sess->getCursorCache().size() == 1, "cursor not cached");

    auto cur2 = sess->createCursor(STDLOG, "select :v1 where :v2 = :v3");
    fail_unless(sess->getCursorCache().size() == 1, "cursor not found in cache");

    auto cur3 = sess->createCursorNoCache(STDLOG, "select :v1 where :v2 != :v3");
    fail_unless(sess->getCursorCache().size() == 1, "cursor wasn't supposed to be cached");
} END_TEST

static void test_disconnect_inactive__(PgCpp::SessionDescriptor sd)
{
    auto now = Dates::currentDateTime();
    xp_testing::fixDateTime(now);

    auto& sm = PgCpp::details::SessionManager::instance();
    sm.maxIdleTime_ = boost::posix_time::seconds(1);
    auto d = sm.findSession(sd);
    fail_unless(d->active == false, "d=%p is expected to be inactive after create", sd);

    PgCpp::commit();
    fail_unless(d->active == false, "d=%p is expected to be inactive after commit", sd);

    make_pg_curs(sd, "SELECT 1").EXfet();
    fail_unless(d->active, "d=%p is expected to be active after createCursor");

    PgCpp::rollback();
    fail_unless(d->active == false, "d=%p is expected to be inactive after rollback", sd);

    // shift time to ensure: now - lastUse > maxIdleTime_
    now += sm.maxIdleTime_ + boost::posix_time::minutes(5);
    xp_testing::fixDateTime(now);
    PgCpp::commit();
    fail_unless(d->active == false);
    fail_unless(static_cast<bool>(d->sess) == false,
            "d=%p inactive session should be disconnected after commit", sd);

    int v = 0;
    make_pg_curs(sd, "SELECT 1").def(v).EXfet();
    fail_unless(d->active, "d=%p is expected to be active after createCursor");
    fail_unless(static_cast<bool>(d->sess) == true,
            "d=%p inactive session should be reconnected again on demand", sd);

    PgCpp::commit();
    fail_unless(static_cast<bool>(d->sess) == true,
            "d=%p active session should not be disconnected", sd);
}

START_TEST(test_disconnect_inactive)
{
    test_modes(test_disconnect_inactive__,
            {PgCpp::SessionType::Managed, PgCpp::SessionType::ReadOnly, PgCpp::SessionType::AutoCommit});
} END_TEST

START_TEST(test_binary_wrapper)
{
    const std::string notUtf8 = "Hello, " "\xAC" "\xA8" "\xE0";

    make_pg_curs(sd, "CREATE TEMPORARY TABLE TEST_BINARY ( FLD1 BYTEA )").exec();

    PgCpp::commit();
    // can't insert the easy way
    try {
        make_pg_curs(sd, "INSERT INTO TEST_BINARY (FLD1) VALUES(:val)")
            .bind(":val", notUtf8)
            .exec();
        fail_unless(false, "expected failed insert");
    } catch (const PgCpp::PgException& e) {
        // 42804 datatype_mismatch
        fail_unless(e.errCode() == PgCpp::ResultCode::BadSqlText,
                    "expected BadSqlText, got %d", e.errCode());
    }
    PgCpp::rollback();

    // but can insert the long way
    PgCpp::BinaryBindHelper bh{notUtf8.data(), notUtf8.size()};
    make_pg_curs(sd, "INSERT INTO TEST_BINARY (FLD1) VALUES(:val)")
        .bind(":val", bh)
        .exec();
    fail_unless(countLines(sd, "TEST_BINARY") == 1, "insert silently failed");

    std::string tmpStr;
    // def works, but the result is in Pg's hex escape format
    make_pg_curs(sd, "SELECT FLD1 FROM TEST_BINARY").def(tmpStr).EXfet();
    fail_unless(tmpStr == "\\x48656c6c6f2c20aca8e0", "def failed: %s", tmpStr.c_str());

    // which DefHelper will decode
    PgCpp::BinaryDefHelper<std::string> defStr{tmpStr};
    make_pg_curs(sd, "SELECT FLD1 FROM TEST_BINARY").def(defStr).EXfet();
    fail_unless(tmpStr == notUtf8, "def failed: %s", tmpStr.c_str());
} END_TEST

START_TEST(test_disconnect_inactive_autonomous)
{
    make_pg_curs_autonomous(sd, "DROP TABLE IF EXISTS TEST_AUTONOMOUS").exec();
    make_pg_curs_autonomous(sd, "CREATE TABLE TEST_AUTONOMOUS ( FLD1 VARCHAR(1) NOT NULL );").exec();
    auto& sm = PgCpp::details::SessionManager::instance();
    sm.maxIdleTime_ = boost::posix_time::seconds(1);
    auto d = sm.findSession(getTestConnectString(), PgCpp::SessionType::AutoCommit);
    make_pg_curs_autonomous(sd, "INSERT INTO TEST_AUTONOMOUS (FLD1) VALUES ('4')").exec();
    PgCpp::commit();
    fail_unless(d->active == false && static_cast<bool>(d->sess) == true,
            "session should be marked as inactive but still connected");
    xp_testing::fixDateTime(Dates::currentDateTime() + boost::posix_time::hours(1));
    PgCpp::commit();
    fail_unless(d->active == false && static_cast<bool>(d->sess) == false,
            "expecting session to be disconnected");
    PgCpp::CursCtl cr = make_pg_curs_autonomous(sd, "DELETE FROM TEST_AUTONOMOUS WHERE FLD1 = '4'");
    cr.exec();
    fail_unless(cr.rowcount() == 1, "expecting 1 row to be deleted but got %d", cr.rowcount());
    fail_unless(d->active == true, "expecting session to be reconnected");
} END_TEST

START_TEST(test_disconnect_inactive_get_session)
{
    auto now = Dates::currentDateTime();
    xp_testing::fixDateTime(now);

    auto& sm = PgCpp::details::SessionManager::instance();
    sm.maxIdleTime_ = boost::posix_time::seconds(1);
    auto d = sm.findSession(sd);

    // shift time to ensure: now - lastUse > maxIdleTime_
    now += sm.maxIdleTime_ + boost::posix_time::minutes(5);
    xp_testing::fixDateTime(now);
    PgCpp::commit(); // sets inactive
    fail_unless(d->active == false);
    PgCpp::commit(); // disconnects
    fail_unless(static_cast<bool>(d->sess) == false,
            "d=%p inactive session should be disconnected after commit", sd);

    auto tmpSd = PgCpp::getReadOnlySession(getTestConnectString());
    fail_unless(d->active == false, "reconnect should not begin transaction");
    make_pg_curs(tmpSd, "SELECT 1").EXfet();
    fail_unless(d->active == true, "make_curs should begin transaction");
    fail_unless(tmpSd == sd, "invalid session descriptor: expected %p got %p", sd, tmpSd);

    int v = 0;
    make_pg_curs(tmpSd, "SELECT 1").def(v).EXfet();
    fail_unless(v == 1, "does not work as expected");
    fail_unless(d->active == true);
} END_TEST

START_TEST(test_single_row_mode)
{
    int v = 0;
    PgCpp::CursCtl cr(make_curs("SELECT 1 UNION SELECT 2 UNION SELECT 3 ORDER BY 1"));
    cr.def(v)
        .singleRowMode()
        .exec();
    for (int i = 0; i < 3; ++i) {
        const int fenRes = cr.fen();
        fail_unless(fenRes == 0, "fen result failed: expected 0 got %d", fenRes);
        fail_unless(v == (i + 1), "fetch value failed: expected %d got %d", (i + 1), v);
    }
} END_TEST

START_TEST(test_single_row_mode_nested)
{
    auto cr1 = make_curs("SELECT 1");
    auto cr2 = make_curs("SELECT 2");

    cr2.singleRowMode().exec();
    try {
        // can't create anymore
        make_curs("SELECT 3");
        fail_unless(false, "expected exception when trying to create the third cursor");
    } catch (PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::Fatal, "expected Fatal, got %d", e.errCode());
    }

    try {
        // and can't exec what was already created
        cr1.exec();
        fail_unless(false, "expected exception when trying to exec the first cursor");
    } catch (PgCpp::PgException& e) {
        fail_unless(e.errCode() == PgCpp::ResultCode::Fatal, "expected Fatal, got %d", e.errCode());
    }

    auto sess = PgCpp::Session::create(getTestConnectString());
    // of course, a different session will work just fine
    auto cur = sess->createCursor(STDLOG, "SELECT 2");
} END_TEST

START_TEST(test_single_row_mode_bad_row)
{
    int v = 0;
    PgCpp::CursCtl cr(make_curs("SELECT S.FLD FROM"
                " (SELECT 1 AS FLD, 1 AS NUM"
                " UNION SELECT 2 AS FLD, 2 AS NUM"
                " UNION SELECT NULL AS FLD, 3 AS NUM"
                " UNION SELECT 4 AS FLD, 4 AS NUM) S"
                " ORDER BY S.NUM"));
    cr.def(v)
        .singleRowMode()
        .noThrowError(PgCpp::ResultCode::FetchNull)
        .exec();
    fail_unless(cr.fen() == 0, "fetch first row failed, expected 0 got %d", cr.err());
    fail_unless(v == 1, "bad value in first row, expected 1 got %d", v);
    fail_unless(cr.fen() == 0, "fetch second row failed, expected 0 got %d", cr.err());
    fail_unless(v == 2, "bad value in second row, expected 2 got %d", v);
    fail_unless(cr.fen() == PgCpp::ResultCode::FetchNull, "fetch third row failed, expected FetchNull got %d", cr.err());
    fail_unless(cr.fen() == PgCpp::ResultCode::FetchNull, "fetch forth row failed, expected FetchNull got %d", cr.err());
} END_TEST

START_TEST(test_single_row_mode_empty_select)
{
    int i = -1;
    auto cr = make_curs("SELECT 1 WHERE 1 = 2");
    cr.singleRowMode().def(i).exec();
    while (!cr.fen()) {
        fail_if(true, "didn't expect any data");
    }
    fail_unless(cr.err() == PgCpp::ResultCode::NoDataFound, "expected NoDataFound, got %d", cr.err());
    fail_unless(i == -1, "expected i = -1, got %d", i);
} END_TEST

START_TEST(test_session_destruction)
{
    std::weak_ptr<PgCpp::Session> wp;
    {
        auto cr = make_curs("SELECT 1");
        cr.exec();
        wp = cr.sess_;

        // expecting 2 shared_ptrs: one in SessionManager, the other in CursCtl
        fail_unless(cr.sess_.use_count() == 2, "initial use_count: expected 2, got %d", cr.sess_.use_count());
        try {
            // nonexistent bind
            cr.bind(":lol", 1);
            fail_unless(false, "expected exception for nonexistent bind");
        } catch (PgCpp::PgException& e) {
            fail_unless(e.errCode() == PgCpp::BadSqlText, "expected BadSqlText, got %d", e.errCode());
        }
        // the exception caused a reconnect
        // now the original session is gone from SessionManager
        fail_unless(cr.sess_.use_count() == 1, "after reconnect: expected 1, got %d", cr.sess_.use_count());
    }
    // and now the session is completely gone
    fail_unless(wp.expired(), "the session is not deleted yet");
} END_TEST

void test_skip_commit__(PgCpp::SessionDescriptor sd)
{
    auto d = PgCpp::SessionManager::instance().findSession(sd);

    fail_unless(d->active == false, "expect session to be inactive initially");
    auto cr = make_curs("SELECT 1");
    cr.singleRowMode().exec();

    fail_unless(d->active, "expect active after exec");
    PgCpp::commit(); // this will LogError in Managed mode
    fail_unless(d->active, "expect ignored commit");

    while (!cr.fen()) {}
    PgCpp::commit();
    fail_unless(!d->active, "expect not ignored commit");
}

START_TEST(test_skip_commit)
{
    test_modes(test_skip_commit__,
        {PgCpp::SessionType::Managed, PgCpp::SessionType::ReadOnly, PgCpp::SessionType::AutoCommit});
} END_TEST

START_TEST(test_convertSql)
{
    static const struct {
        std::string inSql;
        std::string exSql;
    } tcOk[] = {
        {"COMMIT", "COMMIT"}, // nothing to convert
        {"SELECT :i", "SELECT $1"}, // simple
        {"SELECT :345", "SELECT $1"}, // digits
        {"INSERT INTO A VALUES(:v1,:oth, :that,:s2)", "INSERT INTO A VALUES($1,$2, $3,$4)"},
        {"INSERT INTO A VALUES(:v3, :oth,:v3)", "INSERT INTO A VALUES($1, $2,$1)"}, // multiple
        {"SELECT :i ::INTEGER", "SELECT $1 ::INTEGER"}, // casts
        {"SELECT :i::INTEGER", "SELECT $1::INTEGER"}, // without whitespace
        {"SELECT :i||:j", "SELECT $1||$2"}, // concat
        {"UPDATE A SET B=:i>:j WHERE :i!=:j AND 2*:j<:i",
         "UPDATE A SET B=$1>$2 WHERE $1!=$2 AND 2*$2<$1"}, // operators
        {"UPDATE A SET B=((:d1+(-:d2)/:ABS+ABS(5*:dyet-3)))",
         "UPDATE A SET B=(($1+(-$2)/$3+ABS(5*$4-3)))"}, // operators
        {"CREATE TABLE A (:d)", "CREATE TABLE A (:d)"}, // DDL - no conversion
        {"WITH A AS (SELECT :a a) SELECT a FROM A WHERE a = :b",
         "WITH A AS (SELECT $1 a) SELECT a FROM A WHERE a = $2"}, // WITH
        //{"   SELECT :i", "   SELECT $1"}, // spaces at begin
        {"SELECT(1),:i", "SELECT(1),$1"}, // no space, but (
        {"SELECT\n1,:i", "SELECT\n1,$1"}, // no space, but \n
        {"SELECT+1,:i", "SELECT+1,$1"}, // no space, but +
        {"SELECT-1,:i", "SELECT-1,$1"}, // no space, but -
        {"SELECT'1',:i", "SELECT'1',$1"}, // no space, but '
    };
    const auto nOk = sizeof(tcOk) / sizeof(tcOk[0]);

    for (auto i = 0; i != nOk; ++i) {
        PgCpp::Positions pos;
        const std::string got = PgCpp::convertSql(pos, tcOk[i].inSql);
        CHECK_EQUAL_STRINGS(tcOk[i].exSql, got);
    }
} END_TEST

START_TEST(test_convertSql_bad)
{
    static const struct {
        std::string inSql;
    } tcNok[] = {
        {""}, // empty sql
        {" SELECT :i"}, // starts with a space
        {"345"} // garbage in
    };
    const auto nNok = sizeof(tcNok) / sizeof(tcNok[0]);

    for (auto i = 0; i != nNok; ++i) {
        PgCpp::Positions pos;
        try {
            PgCpp::convertSql(pos, tcNok[i].inSql);
            fail_unless(false, "exception expected");
        } catch (const PgCpp::PgException& e) {
            fail_unless(e.errCode() == PgCpp::ResultCode::BadSqlText, "expected BadSqlText, got %d", e.errCode());
        }
    }
} END_TEST

START_TEST(bind_json_dict)
{
    auto c = make_curs("select '{\"1\": :v1}'");
    PgCpp::dumpCursor(c);
    c.bind(":v1", 1);
}
END_TEST

START_TEST(test_diff_bind_types)
{
    static const std::string sql = "SELECT :v1, :v2, :v3";

    // intended to bind std::size_t everywhere, but typo'd in one of the params
    auto cr1 = make_curs(sql);
    cr1.bind(":v1", 1ull).bind(":v2", 2).bind(":v3", 3ull).exec();

    fail_unless(
        (static_cast<PgCpp::PgCacheExecutor*>(cr1.exectr_.get())->prepTypes ==
        std::vector<Oid>{static_cast<Oid>(PgCpp::PgOid::Int8),
                         static_cast<Oid>(PgCpp::PgOid::Int4),
                         static_cast<Oid>(PgCpp::PgOid::Int8)}),
        "expected 8,4,8"
    );

    // now we have a different cursor, without the typo
    auto cr2 = make_curs(sql);
    cr2.bind(":v1", 1ull).bind(":v2", 2ull).bind(":v3", 3ull);

    // loaded executor from cache; reused the same plan
    fail_unless(
        (static_cast<PgCpp::PgCacheExecutor*>(cr2.exectr_.get())->prepTypes ==
        std::vector<Oid>{static_cast<Oid>(PgCpp::PgOid::Int8),
                         static_cast<Oid>(PgCpp::PgOid::Int4),
                         static_cast<Oid>(PgCpp::PgOid::Int8)}),
        "expected 8,4,8"
    );

    cr2.exec(); // this will LogError

    // except that the types don't match: DEALLOCATE, prepare again
    fail_unless(
        (static_cast<PgCpp::PgCacheExecutor*>(cr2.exectr_.get())->prepTypes ==
        std::vector<Oid>{static_cast<Oid>(PgCpp::PgOid::Int8),
                         static_cast<Oid>(PgCpp::PgOid::Int8),
                         static_cast<Oid>(PgCpp::PgOid::Int8)}),
        "expected 8,8,8"
    );
} END_TEST

START_TEST(test_select_wo_space)
{
    {
        int b=0;
        make_curs("SELECT(1)").def(b).EXfet();
        fail_unless(b==1,"expected 1");
    }
    {
        int b=0;
        make_curs("SELECT\n1").def(b).EXfet();
        fail_unless(b==1,"expected 1");
    }
    {
        int b=0;
        make_curs("SELECT+1").def(b).EXfet();
        fail_unless(b==1,"expected 1");
    }
    {
        int b=0;
        make_curs("SELECT-1").def(b).EXfet();
        fail_unless(b==-1,"expected -1");
    }
    {
        std::string b;
        make_curs("SELECT'1'").def(b).EXfet();
        fail_unless(b=="1","expected \"1\"");
    }
} END_TEST

#define SUITENAME "PgSqlUtil"
TCASEREGISTER(setupReadOnly, nullptr)
{
    ADD_TEST(test_readonly);
    ADD_TEST(test_char_buff);
    ADD_TEST(test_cstring);
    ADD_TEST(test_char_buff_overflow);
    ADD_TEST(test_string);
    ADD_TEST(test_string_select_int);
    ADD_TEST(test_bool);
    ADD_TEST(test_char);
    ADD_TEST(test_short);
    ADD_TEST(test_int);
    ADD_TEST(test_unsigned);
    ADD_TEST(pg_test_double);
    ADD_TEST(pg_test_float);
    ADD_TEST(test_long_long);
    ADD_TEST(test_date);
    ADD_TEST(test_date_special);
    ADD_TEST(test_datetime);
    ADD_TEST(test_datetime_special);
    ADD_TEST(test_rip_int);
    ADD_TEST(test_rip_string);
    ADD_TEST(test_parint_int);
    ADD_TEST(test_byte_array);
    ADD_TEST(test_null);
    ADD_TEST(test_autonull);
    ADD_TEST(test_row);
    ADD_TEST(test_row_null);
    ADD_TEST(test_bind);
    ADD_TEST(test_bind_ParNotEq1)
    ADD_TEST(test_bind_ParNotEq2)
    ADD_TEST(test_rvalue_bind);
    ADD_TEST(test_stb_bind);
    ADD_TEST(test_unstb_bind);
    ADD_TEST(test_second_exec_after_error);
    ADD_TEST(test_bind_mode_change);
    ADD_TEST(test_stb_ind_change);
    ADD_TEST(test_bind_typo);
    ADD_TEST(test_syntax_error);
    ADD_TEST(test_loop_bind);
    ADD_TEST(test_cursor_cache);
    ADD_TEST(test_cursor_cache);
    ADD_TEST(test_disconnect_inactive_get_session);
    ADD_TEST(test_single_row_mode);
    ADD_TEST(test_single_row_mode_nested);
    ADD_TEST(test_single_row_mode_bad_row);
    ADD_TEST(test_single_row_mode_empty_select);
    ADD_TEST(test_session_destruction);
    ADD_TEST(test_diff_bind_types);
    ADD_TEST(bind_json_dict);
    ADD_TEST(test_select_wo_space);
}TCASEFINISH

TCASEREGISTER(setupManaged, nullptr)
{
    ADD_TEST(test_func);
    ADD_TEST(test_unstb_ind_change);
    ADD_TEST(test_autonomous);
    ADD_TEST(test_binary_wrapper);
    ADD_TEST(test_disconnect_inactive_autonomous);
    ADD_TEST(test_savepoint);
    ADD_TEST(test_for_update);
    ADD_TEST(test_rowcount);
    ADD_TEST(test_datetime_with_tz);
    ADD_TEST(test_bind_int_after_long_int);
}TCASEFINISH

TCASEREGISTER(nullptr, nullptr)
{
    ADD_TEST(test_nothrow_fetch_null);
    ADD_TEST(test_nothrow_too_many_rows);
    ADD_TEST(test_nothrow_uniq_constr);
    ADD_TEST(test_disconnect_inactive);
    ADD_TEST(test_skip_commit);
    ADD_TEST(test_convertSql);
    ADD_TEST(test_convertSql_bad);
}TCASEFINISH

} // namespace

#endif // XP_TESTING
#endif // ENABLE_PG

void init_postgres_tests()
{}
