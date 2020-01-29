#pragma once

#include "db_row.h"
#include "pg_cursctl.h"

namespace PgCpp {

using BaseRow = dbcpp::BaseRow<PgCpp::CursCtl>;

template<typename... Args>
using Row = dbcpp::Row<PgCpp::CursCtl, Args...>;

} // PgCpp
