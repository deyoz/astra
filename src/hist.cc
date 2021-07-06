#include "hist.h"
#include "PgOraConfig.h"
#include "exceptions.h"
#include "astra_utils.h"
#include <serverlib/str_utils.h>
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace std;
using namespace EXCEPTIONS;

HistoryEventId::HistoryEventId() :
  order_(PgOra::getSeqNextVal_int("events__seq")),
  time_(Dates::second_clock::universal_time()) {}

HistoryTable::HistoryTable(const std::string& tableName)
{
  auto cur = make_db_curs("SELECT id, code, ident_field, history_fields "
                          "FROM history_tables "
                          "WHERE code=:code",
                          PgOra::getROSession("HISTORY_TABLES"));
  cur.stb()
     .def(tableId_)
     .def(tableName_)
     .def(identField_)
     .def(historyFields_)
     .bind(":code", StrUtils::ToLower(tableName))
     .EXfet();

  if(cur.err() == DbCpp::ResultCode::NoDataFound)
    throw Exception("%s: Incorrect table name '%s'", __func__, tableName.c_str());
}

bool HistoryTable::lockRow(const RowId_t& rowId)
{
  std::ostringstream sql;
  sql << "UPDATE " << tableName_ << " "
      << "SET " << identField_ << "=" << identField_  << " "
      << "WHERE " << identField_ << "=:row_ident";

  LogTrace(TRACE6) << __func__ << ": " << sql.str();

  auto cur = make_db_curs(sql.str(), PgOra::getRWSession(tableName_));

  cur.stb()
     .bind(":row_ident", rowId.get())
     .exec();

  return cur.rowcount() > 0;
}

std::optional<HistoryEventId> HistoryTable::addRow(const RowId_t& rowId)
{
  std::optional<HistoryEventId> lastEventId=getLastEventId(rowId);
  HistoryEventId nextEventId;

  std::ostringstream sql;
  sql << "INSERT INTO hist_" << tableName_ << "("
      << identField_ << ", " << historyFields_ << ", hist_time, hist_order) " << std::endl
      << "(SELECT :row_ident, " << historyFields_ << ", :next_hist_time, :next_hist_order " << std::endl
      << " FROM " << tableName_ << std::endl
      << " WHERE " << identField_ << "=:row_ident";

  if (lastEventId)
  {
    sql << std::endl
        << (PgOra::supportsPg(tableName_)?"EXCEPT":"MINUS") << std::endl
        << " SELECT :row_ident, " << historyFields_ << ", :next_hist_time, :next_hist_order " << std::endl
        << " FROM hist_" << tableName_ << std::endl
        << " WHERE " << identField_ << "=:row_ident AND hist_order=:last_hist_order AND hist_time=:last_hist_time";
  }

  sql << ")";

  LogTrace(TRACE6) << __func__ << ": " << sql.str();

  auto cur = make_db_curs(sql.str(), PgOra::getRWSession(tableName_));

  cur.stb()
     .bind(":row_ident", rowId.get())
     .bind(":next_hist_order", nextEventId.order())
     .bind(":next_hist_time",  nextEventId.time());

  if (lastEventId)
  {
    cur.bind(":last_hist_order", lastEventId.value().order())
       .bind(":last_hist_time",  lastEventId.value().time());
  }

  cur.exec();

  if (cur.rowcount() > 0) return nextEventId;

  return {};
}

std::optional<HistoryEventId> HistoryTable::getLastEventId(const RowId_t& rowId)
{
  auto cur = make_db_curs("SELECT hist_order, open_time "
                          "FROM history_events "
                          "WHERE row_ident=:row_ident AND table_id=:table_id AND close_time=:upper_bound_time",
                          PgOra::getROSession("HISTORY_EVENTS"));

  int histOrder;
  Dates::DateTime_t openTime;

  cur.stb()
     .def(histOrder)
     .def(openTime)
     .bind(":row_ident", rowId.get())
     .bind(":table_id", tableId_)
     .bind(":upper_bound_time", upperBoundTime())
     .exec();

  std::optional<HistoryEventId> result;

  while (!cur.fen())
  {
    if (result)
      throw Exception("%s: history_events is corrupted! (table_id=%d, row_ident=%d)",
                      __func__, tableId_, rowId.get());
    result.emplace(histOrder, openTime);
  }

  return result;
}

void HistoryTable::closeLastEvent(const RowId_t& rowId,
                                  const std::optional<HistoryEventId>& nextEventId)
{
  auto cur = make_db_curs("UPDATE history_events "
                          "SET close_time=:close_time, close_user=:close_user, close_desk=:close_desk "
                          "WHERE row_ident=:row_ident AND table_id=:table_id AND close_time=:upper_bound_time",
                          PgOra::getRWSession("HISTORY_EVENTS"));

  cur.stb()
     .bind(":row_ident", rowId.get())
     .bind(":table_id", tableId_)
     .bind(":upper_bound_time", upperBoundTime())
     .bind(":close_time", nextEventId?nextEventId.value().time():
                                      Dates::second_clock::universal_time())
     .bind(":close_user", TReqInfo::Instance()->user.descr)
     .bind(":close_desk", TReqInfo::Instance()->desk.code)
     .exec();
}

void HistoryTable::openNextEvent(const RowId_t& rowId,
                                 const HistoryEventId& nextEventId)
{
  auto cur = make_db_curs(
    "INSERT INTO history_events(table_id, row_ident, open_time, open_user, open_desk, hist_order, close_time) "
    "VALUES(:table_id, :row_ident, :open_time, :open_user, :open_desk, :hist_order, :upper_bound_time)",
    PgOra::getRWSession("HISTORY_EVENTS"));

  cur.stb()
     .bind(":row_ident", rowId.get())
     .bind(":table_id", tableId_)
     .bind(":upper_bound_time", upperBoundTime())
     .bind(":open_time", nextEventId.time())
     .bind(":open_user", TReqInfo::Instance()->user.descr)
     .bind(":open_desk", TReqInfo::Instance()->desk.code)
     .bind(":hist_order", nextEventId.order())
     .exec();
}

void HistoryTable::synchronize(const RowId_t& rowId)
{
  if (!lockRow(rowId))
  {
    closeLastEvent(rowId, std::nullopt);
    return;
  }

  std::optional<HistoryEventId> nextEventId=addRow(rowId);

  if (!nextEventId) return;

  closeLastEvent(rowId, nextEventId);
  openNextEvent(rowId, nextEventId.value());
}

#ifdef XP_TESTING

std::vector<RowId_t> HistoryTable::getLastEventRowIds() const
{
  auto cur = make_db_curs("SELECT row_ident FROM history_events "
                          "WHERE table_id=:table_id "
                          "ORDER BY open_time DESC, hist_order DESC",
                          PgOra::getROSession("HISTORY_EVENTS"));

  int rowIdent;

  cur.stb()
     .def(rowIdent)
     .bind(":table_id", tableId_)
     .exec();

  std::vector<RowId_t> result;

  while (!cur.fen())
  {
    RowId_t rowId(rowIdent);
    if (algo::contains(result, rowId)) continue;
    result.push_back(rowId);
  }

  return result;
}

#endif/*XP_TESTING*/