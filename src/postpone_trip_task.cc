#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "postpone_trip_task.h"
#include "trip_tasks.h"
#include "qrys.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/pg_cursctl.h>
#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

void PostponeTripTaskHandling::insertDb(const TTripTaskKey& task, edilib::EdiSessionId_t sessId)
{
  LogTrace(TRACE3) << "add session " << sessId << " for postpone task " << task;

  DB::TCachedQuery Qry(PgOra::getRWSession("POSTPONED_TRIP_TASK"),
    "INSERT INTO postponed_trip_task(session_id, point_id, name, params) "
    "VALUES(:session_id, :point_id, :name, :params) ", STDLOG);
  Qry.get().CreateVariable("session_id", otInteger, sessId.get());
  Qry.get().CreateVariable("point_id", otInteger, task.point_id != ASTRA::NoExists ? task.point_id : FNull);
  Qry.get().CreateVariable("name", otString, task.name);
  Qry.get().CreateVariable("params", otString, task.params);
  Qry.get().Execute();

  LogTrace(TRACE1) << "insert into POSTPONED_TRIP_TASK for edisess=" << sessId << "; " << task;
}

boost::optional<TTripTaskKey> deleteDbPg(edilib::EdiSessionId_t sessId)
{
  auto cur = DbCpp::mainPgManagedSession(STDLOG).createPgCursor(
      STDLOG, "DELETE FROM postponed_trip_task WHERE session_id=:session_id "
              "RETURNING point_id, name, params", true);

  std::string name, params;
  int point_id;
  cur.bind(":session_id", sessId.get()).
      def(point_id).
      def(name).
      def(params).
      EXfet();

  if(cur.err() != PgCpp::ResultCode::NoDataFound)
  {
      auto cur = DbCpp::mainPgManagedSession(STDLOG).createPgCursor(
          STDLOG, "SELECT COUNT(*) FROM postponed_trip_task "
                  "WHERE point_id=:point_id AND name=:name AND params=:params", true);

      int remained = 0;
      cur.bind(":point_id", point_id).
          bind(":name", name).
          bind(":params", params).
          def(remained).
          EXfet();

      if (remained == 0)
      {
        TTripTaskKey task(point_id, name, params);

        LogTrace(TRACE1) << "delete from POSTPONED_TRIP_TASK for edisess=" << sessId
                         << "; got " << task;

        return task;
      }
  }
  return boost::none;
}

boost::optional<TTripTaskKey> deleteDbOra(edilib::EdiSessionId_t sessId)
{
  TCachedQuery Qry(
    "BEGIN "
    "  DELETE FROM postponed_trip_task WHERE session_id=:session_id "
    "    RETURNING point_id, name, params INTO :point_id, :name, :params; "
    "  IF SQL%FOUND THEN "
    "    SELECT COUNT(*) INTO :remained FROM postponed_trip_task "
    "    WHERE point_id=:point_id AND name=:name AND params=:params; "
    "  END IF; "
    "END;", QParams() << QParam("session_id", otInteger, sessId.get())
                      << QParam("point_id", otInteger, FNull)
                      << QParam("name", otString, FNull)
                      << QParam("params", otString, FNull)
                      << QParam("remained", otInteger, FNull));

  Qry.get().Execute();

  if (Qry.get().VariableIsNULL("remained")) return boost::none;

  int remained = Qry.get().GetVariableAsInteger("remained");

  if(remained == 0) {
    TTripTaskKey task(Qry.get().GetVariableAsInteger("point_id"),
                      Qry.get().GetVariableAsString("name"),
                      Qry.get().GetVariableAsString("params"));

    LogTrace(TRACE1) << "delete from POSTPONED_TRIP_TASK for edisess=" << sessId
                     << "; got " << task;

    return task;
  }

  return boost::none;
}

boost::optional<TTripTaskKey> PostponeTripTaskHandling::deleteDb(edilib::EdiSessionId_t sessId)
{
  if(PgOra::supportsPg("POSTPONED_TRIP_TASK"))
    return deleteDbPg(sessId);
  else
    return deleteDbOra(sessId);
}

void PostponeTripTaskHandling::deleteWaiting(const TTripTaskKey &task)
{
  LogTrace(TRACE3) << "delete postponed records for task: " << task;

  DB::TCachedQuery Qry(PgOra::getRWSession("POSTPONED_TRIP_TASK"),
    "DELETE FROM postponed_trip_task "
    "WHERE point_id=:point_id AND name=:name AND params=:params", STDLOG);

  Qry.get().CreateVariable("point_id", otInteger, task.point_id != ASTRA::NoExists ? task.point_id : FNull);
  Qry.get().CreateVariable("name", otString, task.name);
  Qry.get().CreateVariable("params", otString, task.params);

  Qry.get().Execute();
}

void PostponeTripTaskHandling::postpone(const TTripTaskKey &task, edilib::EdiSessionId_t sessId)
{
  insertDb(task, sessId);
}

boost::optional<TTripTaskKey> PostponeTripTaskHandling::deleteWaiting(edilib::EdiSessionId_t sessId)
{
  LogTrace(TRACE3) << "try to find postponed task for session: " << sessId;
  return deleteDb(sessId);
}

boost::optional<TTripTaskKey> PostponeTripTaskHandling::findWaiting(edilib::EdiSessionId_t sessId)
{
  DB::TCachedQuery Qry(PgOra::getROSession("POSTPONED_TRIP_TASK"),
    "SELECT * FROM postponed_trip_task WHERE session_id=:session_id", STDLOG);
  Qry.get().CreateVariable("session_id", otInteger, sessId.get());
  Qry.get().Execute();

  if (!Qry.get().Eof) {
    const int point_id = Qry.get().FieldIsNULL("point_id") ? ASTRA::NoExists : Qry.get().FieldAsInteger("point_id");
    const std::string name = Qry.get().FieldAsString("name");
    const std::string params = Qry.get().FieldAsString("params");

    return TTripTaskKey(point_id, name, params);
  }

  return boost::none;
}

bool PostponeTripTaskHandling::copyWaiting(edilib::EdiSessionId_t srcSessId, edilib::EdiSessionId_t destSessId)
{
  boost::optional<TTripTaskKey> task = findWaiting(srcSessId);
  if (!task) return false;

  postpone(task.get(), destSessId);

  return true;
}

