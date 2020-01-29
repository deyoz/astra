#include "dbcpp_session.h"
#ifdef ENABLE_PG

#include <libpq-fe.h>

#include "cursctl.h"
#include "dbcpp_cursctl.h"
#include "oci8cursor.h"
#include "pg_cursctl.h"
#include "pg_session.h"
#include "testmode.h"

#define NICKNAME "ASM"
#include "slogger.h"

namespace PgCpp
{
    namespace details
    {
        PgCpp::ResultCode sqlStateToResultCode(const char* sqlState);
    }
}

namespace DbCpp
{
    OraSession::OraSession(const char* n, const char* f, int l, const std::string& connStr)
        : mSession(new OciCpp::OciSession(n, f, l, connStr))
        , mReleaseOnDestruction(false)
    {
    }

    OraSession::OraSession(const char* n, const char* f, int l, OciCpp::OciSession* session,
                           bool releaseOnDestruction)
        : mSession(session)
        , mReleaseOnDestruction(releaseOnDestruction)
    {
    }

    OraSession::~OraSession()
    {
        if (mReleaseOnDestruction)
        {
            mSession.release();
        }
    }

    CursCtl OraSession::createCursor(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, true);
    }

    CursCtl OraSession::createCursor(const char* n, const char* f, int l, const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), true);
    }

    CursCtl OraSession::createCursorNoCache(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, false);
    }

    CursCtl OraSession::createCursorNoCache(const char* n, const char* f, int l,
                                            const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), false);
    }

    OciCpp::CursCtl OraSession::createOraCursor(const char* n, const char* f, int l,
                                                const char* sql, bool cacheit)
    {
        if (!is_alive())
        {
            reconnect();
        }
        if (cacheit)
        {
            return mSession->createCursor(n, f, l, sql);
        } else
        {
            return make_curs_no_cache_(sql, mSession.get(), n, f, l);
        }
    }

    OciCpp::OciSession& OraSession::get_sess4oci8() const { return *mSession; }
    cda_text* OraSession::mk_cda__(const char* nick, const char* file, int line,
                                   const std::string& query) const
    {
        cda_text* CU = newCursor(*mSession, query);
        if (CU == NULL)
        {
            LogTrace(TRACE1) << "query='" << query << "'";
            throw comtech::Exception(nick, file, line, __func__, "Sql error");
        }
        return CU;
    }
    bool OraSession::is_alive() const { return mSession->is_alive(); }
    void OraSession::reconnect() { mSession->reconnect(); }

    void OraSession::commit() { mSession->commit(); }
    void OraSession::rollback() { mSession->rollback(); }
    std::string OraSession::getConnectString() const { return mSession->getConnectString(); }
    CopyResult OraSession::copyDataFrom(const std::string& sql, const char* data, size_t size)
    {
        throw comtech::Exception(STDLOG, __func__, "Call of unsupported OraSession method");
    }

    static void beginManagedTransaction(const char* nick, const char* file, int line,
                                        PgCpp::Session& session)
    {
        PGresult* res = PQexec(static_cast<PGconn*>(session.conn()),
                               "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ WRITE");
        ASSERT(res != nullptr && "PGresult is null - do we are out-of-memory on pg-server?");
        std::unique_ptr<PGresult, decltype(&PQclear)> tmpRes(res, &PQclear);
        const char* sqlState = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        if (sqlState)
        {
            const auto err = PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY);
            LogError(nick, file, line) << "expected successful query " << err << '\n';

            PgCpp::ResultCode pg_resultCode = PgCpp::details::sqlStateToResultCode(sqlState);
            if (pg_resultCode == PgCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on PQexec(). exit";
                exit(1);
            }
            throw comtech::Exception(nick, file, line, __func__, PQresultErrorMessage(res));
        }
        if (inTestMode())
        {
            session.commit();
        }
    }

    PgSession::PgSession(const char* n, const char* f, int l, const std::string& connStr,
                         DbSessionType type)
        : mType(type)
        , mIsActive(false)
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr.c_str()))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
    }

    PgSession::PgSession(const std::string& connStr, DbSessionType type)
        : mType(type)
        , mIsActive(false)
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr.c_str()))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
    }

    PgSession::PgSession(const char* connStr, DbSessionType type)
        : mType(type)
        , mIsActive(false)
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
    }

    PgSession::~PgSession() {}

    void PgSession::setClientInfo(const std::string& clientInfo)
    {
        LogTrace(TRACE1) << "PgSession::setClientInfo(" << clientInfo << ")";
    }

    CursCtl PgSession::createCursor(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, true);
    }

    CursCtl PgSession::createCursor(const char* n, const char* f, int l, const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), true);
    }

    CursCtl PgSession::createCursorNoCache(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, false);
    }

    CursCtl PgSession::createCursorNoCache(const char* n, const char* f, int l,
                                           const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), false);
    }

    PgCpp::CursCtl PgSession::createPgCursor(const char* n, const char* f, int l, const char* sql,
                                             bool cacheit)
    {
        if (cacheit)
        {
            return mSession->createCursor(n, f, l, sql);
        } else
        {
            return mSession->createCursorNoCache(n, f, l, sql);
        }
    }

    void PgSession::commit()
    {
        if (mIsActive)
        {
            mSession->commit();
        }
        mIsActive = false;
    }

    void PgSession::rollback()
    {
        if (mIsActive)
        {
            mSession->rollback();
        }
        mIsActive = false;
    }

#ifdef XP_TESTING
    void PgSession::rollbackInTestMode()
    {
        if (inTestMode())
        {
            mSession->rollbackInTestMode();
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
    }
#endif // XP_TESTING

    CopyResult PgSession::copyDataFrom(const std::string& sql, const char* data, size_t size)
    {
        activateSession();
        PGconn* conn = static_cast<PGconn*>(mSession->conn());
        std::unique_ptr<PGresult, decltype(&PQclear)> res(PQexec(conn, sql.c_str()), &PQclear);
        if (PQresultStatus(&*res) != PGRES_COPY_IN)
        {
            LogTrace(TRACE0) << "COPY_IN prepare failed: " << PQerrorMessage(conn);
            return CopyResult::PrepareError;
        }

        int ret = PQputCopyData(conn, data, size);
        if (ret != 1)
        {
            LogTrace(TRACE0) << "PQputCopyData failed (ret=" << ret
                             << "): " << PQerrorMessage(conn);
            return CopyResult::CopyDataError;
        }

        ret = PQputCopyEnd(conn, NULL);
        if (ret != 1)
        {
            LogTrace(TRACE0) << "PQputCopyEnd() failed (ret=" << ret
                             << "): " << PQerrorMessage(conn);
            return CopyResult::CopyDataError;
        }

        CopyResult result = CopyResult::Ok;
        while (res)
        {
            res.reset(PQgetResult(conn));
            if (res)
            {
                auto st = PQresultStatus(&*res);
                if (st != PGRES_COMMAND_OK)
                {
                    LogTrace(TRACE0) << "PQgetResult after PQputCopyEnd() failed (status=" << st
                                     << "): " << PQresStatus(st);
                    if (st == PGRES_FATAL_ERROR)
                    {
                        LogError(STDLOG) << PQerrorMessage(conn);
                    } else
                    {
                        LogTrace(TRACE0) << PQerrorMessage(conn);
                    }
                    result = CopyResult::CopyDataError;
                }
            }
        }
        return result;
    }

    void PgSession::activateSession()
    {
        if ((mType == DbSessionType::Managed || inTestMode()) && !mIsActive)
        {
            if (!inTestMode())
            {
                beginManagedTransaction(STDLOG, *mSession);
            }
            mIsActive = true;
        }
    }

    static std::unique_ptr<OraSession> mainSessPtr;
    OraSession& mainOraSession(STDLOG_SIGNATURE)
    {
        if (!mainSessPtr || &mainSessPtr->get_sess4oci8() != OciCpp::pmainSession())
        {
            mainSessPtr.reset(new OraSession(STDLOG_VARIABLE, OciCpp::pmainSession(), true));
        }
        return *mainSessPtr;
    }
    OraSession* mainOraSessionPtr(STDLOG_SIGNATURE) { return &mainOraSession(STDLOG_VARIABLE); }

    struct RollbackOnDestruction
    {
        void operator()(PgSession* session) const
        {
#ifdef XP_TESTING
            if (inTestMode())
            {
                session->rollbackInTestMode();
            } else
#endif // XP_TESTING
            {
                session->rollback();
            }
            delete session;
        }
    };
    static std::unique_ptr<PgSession, RollbackOnDestruction> mainPgSessPtr;
    static bool mainPgSessionReaded = false;
    PgSession& mainPgSession(STDLOG_SIGNATURE)
    {
        PgSession* ptr = mainPgSessionPtr(STDLOG_VARIABLE);
        if (!ptr)
        {
            throw comtech::Exception(STDLOG_VARIABLE, __func__,
                                     "Failed to create main PostgreSQL session");
        }
        return *ptr;
    }
    PgSession* mainPgSessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPgSessionReaded && createIfNotExist)
        {
            mainPgSessionReaded = true;
            LogTrace(TRACE1) << "mainPgSessionPtr() called from " << nick << ":" << file << ":"
                             << line;
            std::string connectString = readStringFromTcl("PG_CONNECT_STRING", "");
            if (!connectString.empty())
            {
                mainPgSessPtr.reset(new PgSession(connectString, DbSessionType::Managed));
            } else
            {
                LogTrace(TRACE0) << "tcl parameter 'PG_CONNECT_STRING' is not set";
            }
        }
        return mainPgSessPtr.get();
    }
}
#endif // ENABLE_PG
