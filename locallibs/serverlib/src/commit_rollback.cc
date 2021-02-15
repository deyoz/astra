#include "commit_rollback.h"

#include "cursctl.h"
#include "dbcpp_session.h"
#include "pg_cursctl.h"

#define NICKNAME "ASM"
#include "slogger.h"

void commit()
{
    OciCpp::mainSession().commit();
#ifdef ENABLE_PG
    PgCpp::commit();
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->commit();
    }
#endif // ENABLE_PG
}

void commitInTestMode()
{
    OciCpp::mainSession().commitInTestMode();
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
    OciCpp::mainSession().rollback();
#ifdef ENABLE_PG
    PgCpp::rollback();
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->rollback();
    }
#endif // ENABLE_PG
}

void rollbackInTestMode()
{
    OciCpp::mainSession().rollbackInTestMode();
#ifdef ENABLE_PG
#ifdef XP_TESTING
    PgCpp::rollbackInTestMode();
#endif //XP_TESTING
    if (auto sess = DbCpp::mainPgManagedSessionPtr(STDLOG, false)) {
        sess->rollback();
    }
#endif // ENABLE_PG
}
