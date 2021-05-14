#include "db_savepoint.h"
#include "hooked_session.h"
#include "pg_session.h"

#include <serverlib/cursctl.h>
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace DB {

Savepoint::Savepoint(const std::string& name)
    : m_name(name)
{
    execSpCmd("savepoint " + m_name);
}

void Savepoint::rollback()
{
    execSpCmd("rollback to savepoint " + m_name);
}

//---------------------------------------------------------------------------------------

void execSpCmd(const std::string& cmd)
{
    LogTrace(TRACE3) << "DB " << cmd;
    // ocicpp
    make_curs_no_cache(cmd).exec();
    // pgcpp
    make_pg_curs_nocache(PgCpp::getPgManaged(), cmd).exec();
    // dbcpp
    make_db_curs_no_cache(cmd, *get_main_pg_rw_sess(STDLOG)).exec();
}

}//namespace DB
