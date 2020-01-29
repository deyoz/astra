#include "savepoint.h"
#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "cursctl.h"

namespace OciCpp 
{
Savepoint::Savepoint(std::string const &p):point(p)
{
    LogTrace(TRACE3) << "savepoint " << point;
    make_curs("savepoint " + point).exec();
}

void Savepoint::rollback()
{
    LogTrace(TRACE3) << "rollback to savepoint " << point;
    make_curs("rollback to savepoint " + point).exec();
}

void Savepoint::reset()
{
    LogTrace(TRACE3) << "reset savepoint " << point;
    make_curs("savepoint " + point).exec();
}
} // OciCpp

#ifdef ENABLE_PG
#include "pg_cursctl.h"

namespace PgCpp
{

Savepoint::Savepoint(SessionDescriptor sd, const std::string& p)
    : sd_(sd), name_(p)
{
    LogTrace(TRACE1) << "savepoint " << name_ << " d=" << sd_;
    make_pg_curs(sd_, "SAVEPOINT " + name_)
        .exec();
}

void Savepoint::rollback()
{
    LogTrace(TRACE1) << "rollback to savepoint " << name_ << " d=" << sd_;
    make_pg_curs(sd_, "ROLLBACK TO SAVEPOINT " + name_)
        .exec();
}

void Savepoint::reset()
{
    LogTrace(TRACE1) << "reset savepoint " << name_ << " d=" << sd_;
    make_pg_curs(sd_, "SAVEPOINT " + name_)
        .exec();
}

} // PgCpp
#endif // ENABLE_PG
