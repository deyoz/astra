#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "postpone_trip_task.h"
#include "trip_tasks.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

void PostponeTripTaskHandling::insertDb(const TTripTaskKey& task, edilib::EdiSessionId_t sessId)
{
  LogTrace(TRACE3) << "add session " << sessId << " for postpone task " << task;

  TCachedQuery Qry(
    "INSERT INTO postponed_trip_task(session_id, point_id, name, params) "
    "VALUES(:session_id, :point_id, :name, :params) ");
  Qry.get().CreateVariable("session_id", otInteger, sessId.get());
  task.toDB(Qry.get());
  Qry.get().Execute();

  LogTrace(TRACE1) << "insert into POSTPONED_TRIP_TASK for edisess=" << sessId << "; " << task;
}

boost::optional<TTripTaskKey> PostponeTripTaskHandling::deleteDb(edilib::EdiSessionId_t sessId)
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

void PostponeTripTaskHandling::deleteWaiting(const TTripTaskKey &task)
{
  LogTrace(TRACE3) << "delete postponed records for task: " << task;

  TCachedQuery Qry(
    "DELETE FROM postponed_trip_task "
    "WHERE point_id=:point_id AND name=:name AND params=:params");
  task.toDB(Qry.get());
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
  TCachedQuery Qry(
    "SELECT * FROM postponed_trip_task WHERE session_id=:session_id");
  Qry.get().CreateVariable("session_id", otInteger, sessId.get());
  Qry.get().Execute();
  if (!Qry.get().Eof)
    return TTripTaskKey(Qry.get());

  return boost::none;
}

bool PostponeTripTaskHandling::copyWaiting(edilib::EdiSessionId_t srcSessId, edilib::EdiSessionId_t destSessId)
{
  boost::optional<TTripTaskKey> task = findWaiting(srcSessId);
  if (!task) return false;

  postpone(task.get(), destSessId);

  return true;
}

