#include "dbcpp_session.h"
#ifdef ENABLE_PG

#include <libpq-fe.h>

#include "cursctl.h"
#include "dbcpp_cursctl.h"
#include "oci8cursor.h"
#include "pg_cursctl.h"
#include "pg_session.h"
#include "testmode.h"
#include "str_utils.h"
#include "isdigit.h"

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
    std::ostream & operator << (std::ostream& os, const DbSessionForType x)
    {
      os << 
      (
      (x)==DbSessionForType::Reading            ?"DbSessionForType::Reading":
      (x)==DbSessionForType::ManagedReadWrite   ?"DbSessionForType::ManagedReadWrite":
      (x)==DbSessionForType::AutonomousReadWrite?"DbSessionForType::AutonomousReadWrite":
                                                 "DbSessionForType::???"
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

    PgSession_wo_CheckSQL::PgSession_wo_CheckSQL(const char* n, const char* f, int l, const std::string& connStr,
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

    PgSession_wo_CheckSQL::PgSession_wo_CheckSQL(const std::string& connStr, DbSessionType type)
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

    PgSession_wo_CheckSQL::PgSession_wo_CheckSQL(const char* connStr, DbSessionType type)
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

    PgSession_wo_CheckSQL::~PgSession_wo_CheckSQL() {}

    
    CursCtl PgSession_wo_CheckSQL::createCursor(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, true);
    }

    CursCtl PgSession_wo_CheckSQL::createCursor(const char* n, const char* f, int l, const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), true);
    }

    CursCtl PgSession_wo_CheckSQL::createCursorNoCache(const char* n, const char* f, int l, const char* sql)
    {
        return CursCtl(*this, n, f, l, sql, false);
    }

    CursCtl PgSession_wo_CheckSQL::createCursorNoCache(const char* n, const char* f, int l,
                                           const std::string& sql)
    {
        return CursCtl(*this, n, f, l, sql.c_str(), false);
    }
    

    PgCpp::CursCtl PgSession_wo_CheckSQL::createPgCursor(const char* n, const char* f, int l, const std::string& sql,
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
    
    void PgSession_wo_CheckSQL::commit()
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

    void PgSession_wo_CheckSQL::rollback()
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

    bool PgSession_wo_CheckSQL::setSessionType(DbSessionType type, bool no_throw)
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
              if(no_throw) 
              {
                return false;
              }
              else
              {
                LogTrace(TRACE0)<<__func__
                  << ": Cannot change session type to 'Autonomous' during active "
                  << "'Managed' transaction";
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

    void PgSession_wo_CheckSQL::setForReading()
    {
      // если сессия неактивна, сделаем её автономной.
      // если активна - она останется managed
      setSessionType(DbSessionType::Autonomous, true/*no_throw*/);
    }
    
    void PgSession_wo_CheckSQL::setForManagedReadWrite()
    {
      // переведём сессию в режим managed на всякий случай
      setSessionType(DbSessionType::Managed, false/*no_throw*/);
      activateSession();
    }
    
    void PgSession_wo_CheckSQL::setForAutonomousReadWrite()
    {
      if (inTestMode())
      {
        setSessionType(DbCpp::DbSessionType::Managed,false/*no_throw*/);
        activateSession();
      }
      else
      {
        setSessionType(DbCpp::DbSessionType::Autonomous,false/*no_throw*/);
      }
    }
    
#ifdef XP_TESTING
    void PgSession_wo_CheckSQL::rollbackInTestMode()
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

    CopyResult PgSession_wo_CheckSQL::copyDataFrom(const std::string& sql, const char* data, size_t size)
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

    void PgSession_wo_CheckSQL::activateSession()
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

    
    
    void PgSession::prepareSession(const char* nick, const char* file, int line)
    {
      if (mForType==DbSessionForType::Reading) {
        mPgSession->setForReading();
      } else if (mForType==DbSessionForType::ManagedReadWrite) {
        mPgSession->setForManagedReadWrite();
      } else if (mForType==DbSessionForType::AutonomousReadWrite) {
        mPgSession->setForAutonomousReadWrite();
      } else {
          LogError(STDLOG_VARIABLE)<<mForType;
          throw comtech::Exception(STDLOG_VARIABLE, __func__,
                                   "Invalid param");
      }
    }
    
        
    PgSession::PgSession(PgSession_wo_CheckSQL* s,
                  DbSessionForType sess_for_type)
        : mForType(sess_for_type)
        , mPgSession(s)
        , mDelete(false)
    {
        prepareSession(STDLOG);
    }

    PgSession::PgSession(const std::string& connStr, DbSessionType type)
        : mForType(type==DbSessionType::Managed ? DbSessionForType::ManagedReadWrite : DbSessionForType::AutonomousReadWrite)
        , mPgSession(new PgSession_wo_CheckSQL(connStr,type))
        , mDelete(true)
    {
        prepareSession(STDLOG);
    }
    
    PgSession::~PgSession() {
      if (mDelete) {
        delete mPgSession;
      }
    }

    static bool word_exists(const std::string& sql, const std::string& word)
    {
        std::string s = StrUtils::ToUpper(sql);
        std::string w = StrUtils::ToUpper(word);

        for (auto pos = s.find(w); pos != std::string::npos; pos = s.find(w, pos + 1))
        {
            if ((pos == 0 || (!ISALPHA(s[pos - 1]) && !ISDIGIT(s[pos - 1]) && s[pos - 1] != '_'))
                && (pos + w.length() >= s.length()
                    || (!ISALPHA(s[pos + w.length()]) && !ISDIGIT(s[pos + w.length()])
                        && s[pos + w.length()] != '_')))
            {
                return true;
            }
        }
        return false;
    }

    static bool simple_is_reading_only(const std::string& sql)
    {
        // Very simple check for readonly sql request
        auto pos_beg = sql.find_first_not_of(' ');
        if (pos_beg == std::string::npos)
        {
            return false;
        }
        auto pos = sql.find_first_of(" (+-'\n", pos_beg);
        if (pos == std::string::npos)
        {
            return false;
        }
        std::string cmd = sql.substr(pos_beg, pos);
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "INSERT" || cmd == "UPDATE" || cmd == "DELETE")
        {
            return false;
        }

        if (cmd == "SELECT")
        {
            std::string s2 = sql.substr(pos);
            if (word_exists(s2, "NEXTVAL") || (word_exists(s2, "FOR") && word_exists(s2, "UPDATE")))
            {
                return false;
            }
            return true;
        }

        if (cmd == "WITH")
        {
            std::string s2 = sql.substr(pos);
            if (word_exists(s2, "INSERT") || word_exists(s2, "UPDATE") || word_exists(s2, "DELETE")
                || word_exists(s2, "NEXTVAL"))
            {
                return false;
            }
            return true;
        }

        return false;
    }

    static bool is_reading_only(const std::string& connectString, const std::string& sql)
    {
#ifdef XP_TESTING
        static std::map<size_t, bool> results;

        size_t hash = std::hash<std::string>{}(sql);
        if (results.find(hash) == results.end())
        {
            PgCpp::ResultCode result = PgCpp::ResultCode::Ok;
            DbCpp::PgSession sess(connectString, DbCpp::DbSessionType::Autonomous);
            if (inTestMode())
            {
                make_db_curs("ROLLBACK", sess).exec();
            }
            make_db_curs("BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ ONLY", sess).exec();
            try
            {
                make_db_curs(sql, sess).exec();
                make_db_curs("ROLLBACK", sess).exec();
            } catch (const PgCpp::PgException& e)
            {
                result = e.errCode();
            }
            results[hash] = !(result == PgCpp::ResultCode::ReadOnly);
        }

        return results[hash];
#else
        return true;
#endif // XP_TESTING
    }

    void PgSession::prepareCursor(const char* nick, const char* file, int line,
        const std::string& sql)
    {
        if (DbSessionForType::Reading == mForType
            && (!simple_is_reading_only(sql) || !is_reading_only(getConnectString(), sql)))
        {
            LogError(STDLOG_VARIABLE) << "Invalid SQL for readind: " << sql;
            throw comtech::Exception(STDLOG_VARIABLE, __func__, "Invalid SQL for readind");
        }
        prepareSession(STDLOG);
    }

    CursCtl PgSession::createCursor(const char* n, const char* f, int l, const char* sql)
    {
        prepareCursor(STDLOG,sql);
        return CursCtl(*this, n, f, l, sql, true);
    }

    CursCtl PgSession::createCursor(const char* n, const char* f, int l, const std::string& sql)
    {
        prepareCursor(STDLOG,sql);
        return CursCtl(*this, n, f, l, sql.c_str(), true);
    }

    CursCtl PgSession::createCursorNoCache(const char* n, const char* f, int l, const char* sql)
    {
        prepareCursor(STDLOG,sql);
        return CursCtl(*this, n, f, l, sql, false);
    }

    CursCtl PgSession::createCursorNoCache(const char* n, const char* f, int l,
                                           const std::string& sql)
    {
        prepareCursor(STDLOG,sql);
        return CursCtl(*this, n, f, l, sql.c_str(), false);
    }
    
    PgCpp::CursCtl PgSession::createPgCursor(const char* n, const char* f, int l, const std::string& sql,
                                             bool cacheit)
    {
        prepareCursor(STDLOG,sql);
        return mPgSession->createPgCursor(n, f, l, sql,cacheit);
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
        void operator()(PgSession_wo_CheckSQL* session) const
        {
            session->rollback();
            delete session;
        }
    };
    
    static std::unique_ptr<PgSession_wo_CheckSQL, RollbackOnDestruction> mainPg2SessPtr;
    static bool mainPg2SessionReaded = false;
    static std::unique_ptr<PgSession> mainPgSessPtr;
    static std::unique_ptr<PgSession> mainPgReadOnlySessPtr;

    static PgSession_wo_CheckSQL* mainPgSessionPtrCommon(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPg2SessionReaded && createIfNotExist)
        {
            mainPg2SessionReaded = true;
            LogTrace(TRACE1) << "mainPgManagedSessionPtr() called from " << nick << ":" << file << ":" << line;
            std::string connectString = readStringFromTcl("PG_CONNECT_STRING", "");
            if (!connectString.empty())
            {
                mainPg2SessPtr.reset(new PgSession_wo_CheckSQL(connectString, DbSessionType::Managed));
            } else
            {
                LogTrace(TRACE0) << "tcl parameter 'PG_CONNECT_STRING' is not set";
            }
        }
        return mainPg2SessPtr.get();
    }


    
    PgSession* mainPgManagedSessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPgSessPtr)
        {
          auto ptr = mainPgSessionPtrCommon(STDLOG_VARIABLE,createIfNotExist);
          if (ptr)
            mainPgSessPtr.reset(new PgSession(ptr, DbSessionForType::ManagedReadWrite));
        }
        return mainPgSessPtr.get();
    }

    PgSession& mainPgManagedSession(STDLOG_SIGNATURE)
    {
        PgSession* ptr = mainPgManagedSessionPtr(STDLOG_VARIABLE);
        if (!ptr)
        {
            throw comtech::Exception(STDLOG_VARIABLE, __func__,
                                     "Failed to create main PostgreSQL session");
        }
        return *ptr;
    }

    PgSession* mainPgReadOnlySessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPgReadOnlySessPtr)
        {
          auto ptr = mainPgSessionPtrCommon(STDLOG_VARIABLE,createIfNotExist);
          if (ptr)
            mainPgReadOnlySessPtr.reset(new PgSession(ptr, DbSessionForType::Reading));
        }
        return mainPgReadOnlySessPtr.get();
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



    static std::unique_ptr<PgSession_wo_CheckSQL, RollbackOnDestruction> mainPg2AutonomousSessPtr;
    static bool mainPg2AutonomousSessionReaded = false;
    static std::unique_ptr<PgSession> mainPgAutonomousSessPtr;

    static PgSession_wo_CheckSQL* mainPgSessionPtrAutonomousCommon(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if (!mainPg2AutonomousSessionReaded && createIfNotExist)
        {
            mainPg2AutonomousSessionReaded = true;
            LogTrace(TRACE1) << "mainPgSessionPtrAutonomousCommon() called from " << nick << ":" << file << ":" << line;
            std::string connectString = readStringFromTcl("PG_CONNECT_STRING", "");
            if (!connectString.empty())
            {
                mainPg2AutonomousSessPtr.reset(new PgSession_wo_CheckSQL(connectString, DbSessionType::Autonomous));
            } else
            {
                LogTrace(TRACE0) << "tcl parameter 'PG_CONNECT_STRING' is not set";
            }
        }
        return mainPg2AutonomousSessPtr.get();
    }


    
    PgSession* mainPgAutonomousSessionPtr(STDLOG_SIGNATURE, bool createIfNotExist)
    {
        if(inTestMode())
        {
          return mainPgManagedSessionPtr(STDLOG_VARIABLE,createIfNotExist);
        }
        if (!mainPgAutonomousSessPtr)
        {
          auto ptr = mainPgSessionPtrAutonomousCommon(STDLOG_VARIABLE,createIfNotExist);
          if (ptr)
            mainPgAutonomousSessPtr.reset(new PgSession(ptr, DbSessionForType::AutonomousReadWrite));
        }
        return mainPgAutonomousSessPtr.get();
    }

    PgSession& mainPgAutonomousSession(STDLOG_SIGNATURE)
    {
        PgSession* ptr = mainPgAutonomousSessionPtr(STDLOG_VARIABLE);
        if (!ptr)
        {
            throw comtech::Exception(STDLOG_VARIABLE, __func__,
                                     "Failed to create main PostgreSQL session");
        }
        return *ptr;
    }


}
#endif // ENABLE_PG
