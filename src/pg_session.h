#pragma once

#include <serverlib/pg_session.h>

namespace PgCpp {

PgCpp::SessionDescriptor& getPgManaged();
PgCpp::SessionDescriptor& getPgReadOnly();
PgCpp::SessionDescriptor& getPgAutoCommit();
void throw_no_data_found(PgCpp::CursCtl &cur);

} // namespace PgCpp

#define get_pg_curs(x) PgCpp::make_curs_(STDLOG, PgCpp::getPgManaged(), (x))
#define get_pg_curs_nocache(x) PgCpp::make_curs_nocache_(STDLOG, PgCpp::getPgManaged(), (x))
#define get_pg_curs_autocommit(x) PgCpp::make_curs_autonomous_(STDLOG, PgCpp::getPgAutoCommit(), (x))
