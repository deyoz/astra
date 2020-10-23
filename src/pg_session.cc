#include "pg_session.h"
#include "exceptions.h"

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace PgCpp {

static std::string getPgConnectString()
{
    static std::string cs = readStringFromTcl("PG_CONNECT_STRING", "");
    return cs;
}

PgCpp::SessionDescriptor& getPgManaged()
{
    static PgCpp::SessionDescriptor sd = PgCpp::getManagedSession(getPgConnectString());
    return sd;
}

PgCpp::SessionDescriptor& getPgReadOnly()
{
    static PgCpp::SessionDescriptor sd = PgCpp::getReadOnlySession(getPgConnectString());
    return sd;
}

PgCpp::SessionDescriptor& getPgAutoCommit()
{
    static PgCpp::SessionDescriptor sd = PgCpp::getAutoCommitSession(getPgConnectString());
    return sd;
}

void throw_no_data_found(PgCpp::CursCtl &cur)
{
    if(cur.err() == NoDataFound) {
        details::dumpCursor(cur);
        throw EXCEPTIONS::Exception("No data found");
    }
}

} // namespace PgCpp
