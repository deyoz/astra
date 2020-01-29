#include "commit_rollback.h"

#include "cursctl.h"
#include "dbcpp_session.h"

#define NICKNAME "ASM"
#include "slogger.h"

void commit()
{
    OciCpp::mainSession().commit();
#ifdef ENABLE_PG
    if (auto sess = DbCpp::mainPgSessionPtr(STDLOG, false))
    {
        sess->commit();
    }
#endif // ENABLE_PG
}

void commitInTestMode()
{
    OciCpp::mainSession().commitInTestMode();
#ifdef ENABLE_PG
    if (auto sess = DbCpp::mainPgSessionPtr(STDLOG, false))
    {
        sess->commit();
    }
#endif // ENABLE_PG
}

void rollback()
{
    OciCpp::mainSession().rollback();
#ifdef ENABLE_PG
    if (auto sess = DbCpp::mainPgSessionPtr(STDLOG, false))
    {
        sess->rollback();
    }
#endif // ENABLE_PG
}

void rollbackInTestMode()
{
    OciCpp::mainSession().rollbackInTestMode();
#ifdef ENABLE_PG
    if (auto sess = DbCpp::mainPgSessionPtr(STDLOG, false))
    {
        sess->rollback();
    }
#endif // ENABLE_PG
}
