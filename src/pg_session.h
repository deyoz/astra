#pragma once

#include <serverlib/pg_session.h>

namespace PgCpp {

PgCpp::SessionDescriptor& getPgManaged();
PgCpp::SessionDescriptor& getPgReadOnly();
PgCpp::SessionDescriptor& getPgAutoCommit();
void throw_no_data_found(PgCpp::CursCtl &cur);

} // namespace PgCpp

#define get_pg_curs(x) make_pg_curs(PgCpp::getPgManaged(), (x))
#define get_pg_curs_autocommit(x) make_pg_curs_autonomous(PgCpp::getPgAutoCommit(), (x))
#define get_pg_curs_nocache(x) make_pg_curs_nocache(PgCpp::getPgManaged(), (x))
