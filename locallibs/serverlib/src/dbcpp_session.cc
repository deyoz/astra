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
    std::ostream & operator << (std::ostream& os, const DbSessionType x)
    {
      os << 
      (
      (x)==DbSessionType::Managed   ?"DbSessionType::Managed":
      (x)==DbSessionType::Autonomous?"DbSessionType::Autonomous":
                                     "DbSessionType::???"
      );
      
      return os;
    }
    std::ostream & operator << (std::ostream& os, const DbType x)
    {
      os << 
      (
      (x)==DbType::Oracle  ?"DbType::Oracle":
      (x)==DbType::Postgres?"DbType::Postgres":
                            "DbType::???"
      );
      
      return os;
    }

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
#ifdef XP_TESTING
        , mType_test(mType)
        , mIsActive_test(mIsActive)
#endif // XP_TESTING
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr.c_str()))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
#ifdef XP_TESTING
        //LogTrace(TRACE0) << "Create (tests) of type=" << mType_test << " mIsActive_test=" << mIsActive_test <<" mSession="<<mSession<< " mConnectString="<<mConnectString<<" this="<<this;
#endif // XP_TESTING
    }

    PgSession::PgSession(const std::string& connStr, DbSessionType type)
        : mType(type)
        , mIsActive(false)
#ifdef XP_TESTING
        , mType_test(mType)
        , mIsActive_test(mIsActive)
#endif // XP_TESTING
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr.c_str()))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
#ifdef XP_TESTING
        //LogTrace(TRACE0) << "Create (tests) of type=" << mType_test << " mIsActive_test=" << mIsActive_test <<" mSession="<<mSession<< " mConnectString="<<mConnectString<<" this="<<this;
#endif // XP_TESTING
    }

    PgSession::PgSession(const char* connStr, DbSessionType type)
        : mType(type)
        , mIsActive(false)
#ifdef XP_TESTING
        , mType_test(mType)
        , mIsActive_test(mIsActive)
#endif // XP_TESTING
        , mConnectString(connStr)
        , mSession(PgCpp::Session::create(connStr))
    {
        if (inTestMode())
        {
            beginManagedTransaction(STDLOG, *mSession);
            mIsActive = true;
        }
#ifdef XP_TESTING
        //LogTrace(TRACE0) << "Create (tests) of type=" << mType_test << " mIsActive_test=" << mIsActive_test <<" mSession="<<mSession<< " mConnectString="<<mConnectString<<" this="<<this;
#endif // XP_TESTING
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
    PgCpp::CursCtl PgSession::createPgCursor(const char* n, const char* f, int l, const std::string& sql,
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
        //LogTrace(TRACE5) << "commit mIsActive=" << mIsActive << " mSession="<<mSession<<" mConnectString="<<mConnectString<<" this="<<this;
        if (mIsActive)
        {
            mSession->commit();
        }
        mIsActive = false;
#ifdef XP_TESTING
        mIsActive_test=mIsActive;
#endif // XP_TESTING
    }

    void PgSession::rollback()
    {
        //LogTrace(TRACE5) << "rollback mIsActive=" << mIsActive << " mSession="<<mSession<<" mConnectString="<<mConnectString<<" this="<<this;
        if (mIsActive)
        {
            mSession->rollback();
        }
        mIsActive = false;
#ifdef XP_TESTING
        mIsActive_test=mIsActive;
#endif // XP_TESTING
    }

    bool PgSession::setSessionType(DbSessionType type, bool no_throw)
    {
//        if (mType != type) 
//          LogTrace(TRACE5) << "Changing session of type '" << mType << "' to type '" << type << "' mConnectString="<<mConnectString<<" this="<<this;
#ifdef XP_TESTING
        if (inTestMode())
        {
            if (mType_test != type) 
              LogTrace(TRACE5) << "Changing session (tests) of type '" << mType_test << "' to type '" << type << "'";
            if (mType_test != type && mIsActive_test)
            {
              LogTrace(TRACE0)<<__func__
                << "Cannot change session type to 'Autonomous' during active "
                << "'Managed' transaction";
              if(no_throw) 
              {
                return false;
              }
              else
              {
                throw comtech::Exception(STDLOG, __func__,
                                         "Cannot change session type to 'Autonomous' during active "
                                         "'Managed' transaction");
              }        
            }
        }
        mType_test=type;
#endif // XP_TESTING

        if (mType == type)
        {
            return true;
        }
        if (!inTestMode())
        {
            if (mIsActive)
            {
              if(no_throw) 
              {
//                LogTrace(TRACE0)<<__func__
//                  << "Cannot change session type to 'Autonomous' during active "
//                  << "'Managed' transaction";
                return false;
              }
              else
              {
                throw comtech::Exception(STDLOG, __func__,
                                         "Cannot change session type to 'Autonomous' during active "
                                         "'Managed' transaction");
              }        
            }
            mType = type;
        }
        return true;
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
        mIsActive_test=false;
        //LogTrace(TRACE5) << "mIsActive_test=" << mIsActive_test<< " mConnectString="<<mConnectString<<" this="<<this;
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
#ifdef XP_TESTING
        if (mType_test == DbSessionType::Managed)
        {
            //LogTrace(TRACE5) << "mIsActive_test=" << mIsActive_test<< " mConnectString="<<mConnectString<<" this="<<this;
            mIsActive_test=true;
        }
#endif // XP_TESTING
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
            session->rollback();
            delete session;
        }
    };
    
    static std::unique_ptr<PgSession, RollbackOnDestruction> mainPgSessPtr;
    static bool mainPgSessionReaded = false;

    static PgSession* mainPgSessionPtrCommon(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPgSessionReaded && createIfNotExist)
        {
            mainPgSessionReaded = true;
            LogTrace(TRACE1) << "mainPgSessionPtr() called from " << nick << ":" << file << ":" << line;
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


    
    PgSession* mainPgSessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        PgSession* ptr = mainPgSessionPtrCommon(STDLOG_VARIABLE,createIfNotExist);
        // переведём сессию в режим managed на всякий случай
        if (ptr)
        {
#if 1
          mainPgSessPtr->setSessionType(DbSessionType::Managed, true/*no_throw*/);
          mainPgSessPtr->activateSession();
#endif /* 0 */            
        }
        return ptr;
    }

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

#if 1    
    PgSession* mainPgReadOnlySessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        PgSession* ptr = mainPgSessionPtrCommon(STDLOG_VARIABLE,createIfNotExist);
        if (ptr)
        {
          // если сессия неактивна, сделаем её автономной.
          // если активна - она останется managed
          ptr->setSessionType(DbSessionType::Autonomous, true/*no_throw*/);
        }
        return ptr;
    }
    
    PgSession& mainPgReadOnlySession(STDLOG_SIGNATURE)
    {
        PgSession* ptr = mainPgReadOnlySessionPtr(STDLOG_VARIABLE);
        if (!ptr)
        {
            throw comtech::Exception(STDLOG_VARIABLE, __func__,
                                     "Failed to create main PostgreSQL session");
        }
        return *ptr;
    }
#endif /* 0 */
    
}
#endif // ENABLE_PG
