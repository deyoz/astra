#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "hooked_session.h"

#include <serverlib/posthooks.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/testmode.h>
#include <serverlib/algo.h>
#include <tclmon/tcl_utils.h>

#include <tuple>
#include <typeinfo>
#include <typeindex>
#include <map>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace {

template <class Session>
class SessionCommit: public Posthooks::BaseHook
{
    typedef SessionCommit<Session> ThisType;
    virtual bool less2(const Posthooks::BaseHook* ptr) const noexcept {
        return this->sess < dynamic_cast<const ThisType*>(ptr)->sess;
    }

    Session* sess;
public:
    SessionCommit<Session>(Session* s)
        : sess(s)
    {}

    ThisType* clone() const { return new ThisType(*this); }

    void run() {
        if(sess) {
            LogTrace(TRACE1) << "hooked commit";
            sess->commit();
        }
    }
};

//---------------------------------------------------------------------------------------

template <class Session>
class SessionRollback: public Posthooks::BaseHook
{
    typedef SessionRollback<Session> ThisType;
    virtual bool less2(const Posthooks::BaseHook* ptr) const noexcept {
        return this->sess < dynamic_cast<const ThisType*>(ptr)->sess;
    }

    Session* sess;
public:
    SessionRollback<Session>(Session* s)
        : sess(s)
    {}

    ThisType* clone() const { return new ThisType(*this); }

    virtual void run() {
        if(sess) {
            LogTrace(TRACE1) << "hooked rollback";
            sess->rollback();
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////

struct AlwaysCommitHooks
{
    template <class Session>
    static void Register(Session* s)
    {
        Posthooks::sethCommit(SessionCommit<Session>(s));
        Posthooks::sethRollb(SessionCommit<Session>(s));
    }

    template <class Session>
    static DbCpp::DbSessionType getSessionType();
};

//

template <>
DbCpp::DbSessionType AlwaysCommitHooks::getSessionType<DbCpp::PgSession>()
{
  return DbCpp::DbSessionType::Managed;
}

//---------------------------------------------------------------------------------------

struct AlwaysRollbackHooks
{
    template <class Session>
    static void Register(Session* s)
    {
        Posthooks::sethCommit(SessionRollback<Session>(s));
        Posthooks::sethRollb(SessionRollback<Session>(s));
    }

    template <class Session>
    static DbCpp::DbSessionType getSessionType();
};

//

template <>
DbCpp::DbSessionType AlwaysRollbackHooks::getSessionType<DbCpp::PgSession>()
{
    return DbCpp::DbSessionType::Managed;
}

//---------------------------------------------------------------------------------------

struct CommitRollbackHooks
{
    template <class Session>
    static void Register(Session* s)
    {
        Posthooks::sethCommit(SessionCommit<Session>(s));
        Posthooks::sethRollb(SessionRollback<Session>(s));
    }

    template <class Session>
    static DbCpp::DbSessionType getSessionType();
};

//

template <>
DbCpp::DbSessionType CommitRollbackHooks::getSessionType<DbCpp::PgSession>()
{
    return DbCpp::DbSessionType::Managed;
}

//---------------------------------------------------------------------------------------

struct DoNothingHooks
{
    template <class Session>
    static void Register(Session* s)
    {
    }

    template <class Session>
    static DbCpp::DbSessionType getSessionType();
};

//

template <>
DbCpp::DbSessionType DoNothingHooks::getSessionType<DbCpp::PgSession>()
{
    return DbCpp::DbSessionType::Autonomous;
}

//---------------------------------------------------------------------------------------

struct AutonomousHooks
{
    template <class Session>
    static void Register(Session* s)
    {
        if(inTestMode())
        {
            Posthooks::sethCommit(SessionCommit<Session>(s));
            Posthooks::sethRollb(SessionRollback<Session>(s));
        }
    }

    template <class Session>
    static DbCpp::DbSessionType getSessionType();
};

//

template <>
DbCpp::DbSessionType AutonomousHooks::getSessionType<DbCpp::PgSession>()
{
    return DbCpp::DbSessionType::Autonomous;
}

//---------------------------------------------------------------------------------------

struct ArxOraConnection
{
    static const char* connect_string_tcl()
    {
        return "CONNECT_STRING";
    }
};

struct ArxPgConnection
{
    static const char* connect_string_tcl()
    {
#ifdef XP_TESTING
        if(inTestMode()) {
            return "PG_CONNECT_STRING";
        } else {
            return "ARX_PG_CONNECT_STRING";
        }
#endif//XP_TESTING

        return "ARX_PG_CONNECT_STRING";
    }
};

//---------------------------------------------------------------------------------------

class SessionInstanceKey
{
private:
    std::type_index m_hooksIndex;
    std::type_index m_connectionIndex;
    std::type_index m_sessionIndex;

public:
    SessionInstanceKey(std::type_index hooksIndex,
                       std::type_index connectionIndex,
                       std::type_index sessionIndex)
        : m_hooksIndex(hooksIndex),
          m_connectionIndex(connectionIndex),
          m_sessionIndex(sessionIndex)
    {}

    bool operator<(const SessionInstanceKey& other) const
    {
        return std::make_tuple(m_hooksIndex, m_connectionIndex, m_sessionIndex) <
               std::make_tuple(other.m_hooksIndex, other.m_connectionIndex, other.m_sessionIndex);
    }
};

//---------------------------------------------------------------------------------------

template <class Session>
class SessionInstanceData
{
private:
    std::unique_ptr<Session> m_sessionPtr;

public:
    SessionInstanceData(Session* sess)
        : m_sessionPtr(sess)
    {}

    Session* get() const {
        return m_sessionPtr.get();
    }
};

//---------------------------------------------------------------------------------------

template <class Hooks,
          class Connection,
          class Session>
Session* createSession()
{
    std::string connStr = readStringFromTcl(Connection::connect_string_tcl(), "");
    if(connStr.empty())
    {
        LogTrace(TRACE5) << Connection::connect_string_tcl() << " not found or empty!";
        return nullptr;
    }

    return new Session(connStr, Hooks::template getSessionType<Session>());
}

//---------------------------------------------------------------------------------------

template <>
DbCpp::OraSession* createSession<CommitRollbackHooks, ArxOraConnection, DbCpp::OraSession>()
{
    std::string connStr = readStringFromTcl(ArxOraConnection::connect_string_tcl(), "");
    if(connStr.empty())
    {
        LogTrace(TRACE5) << ArxOraConnection::connect_string_tcl() << " not found or empty!";
        return nullptr;
    }

    return new DbCpp::OraSession(STDLOG, connStr);
}

//---------------------------------------------------------------------------------------

template <class Hooks,
          class Connection,
          class Session>
Session* getSessionInstance()
{
    SessionInstanceKey key(std::type_index(typeid(Hooks)),
                           std::type_index(typeid(Connection)),
                           std::type_index(typeid(Session)));

    static std::map< SessionInstanceKey, SessionInstanceData<Session> > sessMap;
    if(!algo::contains(sessMap, key)) {
        sessMap.insert(std::make_pair(key,
                                      SessionInstanceData<Session>(createSession<Hooks, Connection, Session>())));
    }

    return sessMap.at(key).get();
}

//---------------------------------------------------------------------------------------

template <class Hooks,
          class Connection,
          class Session>
struct SessionTraits
{
    static Session* getSession()
    {
        Session* sess = ::getSessionInstance<Hooks, Connection, Session>();
        if(sess) {
            Hooks::Register(sess);
        }
        return sess;
    }
};

//---------------------------------------------------------------------------------------

using ArxOraSession_ReadWrite = SessionTraits<CommitRollbackHooks, ArxOraConnection, DbCpp::OraSession>;

using ArxPgSession_ReadWrite = SessionTraits<CommitRollbackHooks, ArxPgConnection, DbCpp::PgSession>;
using ArxPgSession_ReadOnly  = SessionTraits<DoNothingHooks,      ArxPgConnection, DbCpp::PgSession>;

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

DbCpp::Session* get_main_ora_sess(STDLOG_SIGNATURE)
{
    return DbCpp::mainOraSessionPtr(STDLOG_VARIABLE);
}

//

DbCpp::Session* get_main_pg_ro_sess(STDLOG_SIGNATURE)
{
    return DbCpp::mainPgReadOnlySessionPtr(STDLOG_VARIABLE);
}

DbCpp::Session* get_main_pg_rw_sess(STDLOG_SIGNATURE)
{
    return DbCpp::mainPgManagedSessionPtr(STDLOG_VARIABLE);
}

DbCpp::Session* get_main_pg_au_sess(STDLOG_SIGNATURE)
{
    return DbCpp::mainPgAutonomousSessionPtr(STDLOG_VARIABLE);
}

//---------------------------------------------------------------------------------------

DbCpp::Session* get_arx_ora_rw_sess(STDLOG_SIGNATURE)
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return get_main_ora_sess(STDLOG_VARIABLE);
    }
#endif//XP_TESTING

    DbCpp::Session* sess = ArxOraSession_ReadWrite::getSession();
    if(sess != nullptr) {
        return sess;
    }
    return get_main_ora_sess(STDLOG_VARIABLE);
}

//---------------------------------------------------------------------------------------

DbCpp::Session* get_arx_pg_ro_sess(STDLOG_SIGNATURE)
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return get_main_pg_rw_sess(STDLOG_VARIABLE);
    }
#endif//XP_TESTING

    DbCpp::Session* sess = ArxPgSession_ReadOnly::getSession();
    if(sess != nullptr) {
        return sess;
    }
    return get_main_pg_ro_sess(STDLOG_VARIABLE);
}

DbCpp::Session* get_arx_pg_rw_sess(STDLOG_SIGNATURE)
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return get_main_pg_rw_sess(STDLOG_VARIABLE);
    }
#endif//XP_TESTING

    DbCpp::Session* sess = ArxPgSession_ReadWrite::getSession();
    if(sess != nullptr) {
        return sess;
    }
    return get_main_pg_rw_sess(STDLOG_VARIABLE);
}
