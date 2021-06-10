#include "commit_rollback.h"

#ifdef ENABLE_ORACLE
#include "cursctl.h"
#endif // ENABLE_ORACLE
#include "dbcpp_session.h"
#include "pg_cursctl.h"

#define NICKNAME "ASM"
#include "slogger.h"

void commit()
{
#ifdef ENABLE_ORACLE
    OciCpp::mainSession().commit();
#endif // ENABLE_ORACLE
#ifdef ENABLE_PG
    PgCpp::commit();
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->commit();
    }
#endif // ENABLE_PG
}

void commitInTestMode()
{
#ifdef ENABLE_ORACLE
    OciCpp::mainSession().commitInTestMode();
#endif // ENABLE_ORACLE
#ifdef ENABLE_PG
#ifdef XP_TESTING
    PgCpp::commitInTestMode();
#endif //XP_TESTING
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->commit();
    }
#endif // ENABLE_PG
}

void rollback()
{
#ifdef ENABLE_ORACLE
    OciCpp::mainSession().rollback();
#endif // ENABLE_ORACLE
#ifdef ENABLE_PG
    PgCpp::rollback();
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->rollback();
    }
#endif // ENABLE_PG
}

void rollbackInTestMode()
{
#ifdef ENABLE_ORACLE
    OciCpp::mainSession().rollbackInTestMode();
#endif // ENABLE_ORACLE
#ifdef ENABLE_PG
#ifdef XP_TESTING
    PgCpp::rollbackInTestMode();
#endif //XP_TESTING
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->rollback();
    }
#endif // ENABLE_PG
}
