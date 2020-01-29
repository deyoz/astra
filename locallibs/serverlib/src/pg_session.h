#pragma once

#include <memory>

#include "pg_cursctl.h"

namespace PgCpp
{

using PgConn = void*;
using CursorCache = std::map<std::string, std::shared_ptr<details::PgExecutor>>;
using SessionPtr = std::shared_ptr<Session>;

class Session
    : public std::enable_shared_from_this<Session>
{
public:
    static SessionPtr create(const char* connStr);
    ~Session();
    Session(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) = delete;

    CursCtl createCursor(const char* n, const char* f, int l, const char* sql);
    CursCtl createCursor(const char* n, const char* f, int l, const std::string& sql);
    CursCtl createCursorNoCache(const char* n, const char* f, int l, const char* sql);
    CursCtl createCursorNoCache(const char* n, const char* f, int l, const std::string& sql);

    PgConn conn() const;

    void commit();
    void rollback();
#ifdef XP_TESTING
    void commitInTestMode();
    void rollbackInTestMode();
    CursorCache& getCursorCache();
    int getSavepointCounter() const;
    void increaseSavepointCounter();
#endif // XP_TESTING
private:
    Session(const char*);
    std::shared_ptr<details::PgExecutor> cursData(const std::string& sql, bool cache = true);

    PgConn conn_;
    SessionDescriptor sd_;
    CursorCache cursCache_;
#ifdef XP_TESTING
    int spCnt_;
#endif // XP_TESTING

    friend class details::SessionManager;
    friend class CursCtl;
    friend SessionDescriptor getManagedSession(const std::string&);
    friend SessionDescriptor getReadOnlySession(const std::string&);
    friend SessionDescriptor getAutoCommitSession(const std::string&);
};

} // PgCpp
