#include <serverlib/new_daemon.h>
#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/perfom.h>
#include "trip_tasks.h"
#include "astra_utils.h"
#include "astra_main.h"


#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

class FlightTasksDaemon : public ServerFramework::NewDaemon
{
  public:
    FlightTasksDaemon() : ServerFramework::NewDaemon("flight_tasks") {}
};

static int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("FLT_TASKS_WAIT_INTERVAL",1000,60000,10000);
  return VAR;
}

static void check_trip_tasks(const std::string& handler_id);

int main_flight_tasks_tcl( int supervisorSocket, int argc, char *argv[] )
{
  try
  {
    sleep(10);
    InitLogTime(argc>0?argv[0]:NULL);

    std::string handler_id;
    if(argc!=2) {
      LogError(STDLOG) << __FUNCTION__ << ": wrong number of parameters: " << argc;
    } else {
        handler_id = argv[1];
    }

    FlightTasksDaemon daemon;
    init_locale();

    for( ;; )
    {
      InitLogTime(argc>0?argv[0]:NULL);
      PerfomInit();
      base_tables.Invalidate();

      check_trip_tasks(handler_id);
      sleep(WAIT_INTERVAL()/1000); //в секундах
    };
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

static void check_trip_tasks(const std::string& handler_id)
{
    LogTrace(TRACE5) << "check_trip_tasks started, handler_id=" << handler_id;
    TDateTime nowUTC=NowUTC();

    list< pair<int, TTripTaskKey> > tasks;

    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      if (handler_id==all_other_handler_id)
        Qry.SQLText =
            "SELECT id, point_id, name, params "
            "FROM trip_tasks "
            "WHERE next_exec<=:now_utc AND (proc_name=:proc_name OR proc_name IS NULL)";
      else
        Qry.SQLText =
            "SELECT id, point_id, name, params "
            "FROM trip_tasks "
            "WHERE next_exec<=:now_utc AND proc_name=:proc_name";
      Qry.CreateVariable("now_utc", otDate, nowUTC);
      Qry.CreateVariable("proc_name", otString, handler_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
        tasks.emplace_back(Qry.FieldAsInteger("id"), TTripTaskKey(Qry));
    }

    if (tasks.empty())
    {
      LogTrace(TRACE5) << "check_trip_tasks ended (" << tasks.size() << " tasks), handler_id=" << handler_id;
      return;
    }

    TQuery SelQry(&OraSession);
    SelQry.Clear();
    SelQry.SQLText =
        "SELECT last_exec, next_exec, tid FROM trip_tasks "
        "WHERE id=:id AND next_exec<=:now_utc";
    SelQry.DeclareVariable("id", otInteger);
    SelQry.CreateVariable("now_utc", otDate, nowUTC);

    TQuery UpdQry(&OraSession);
    UpdQry.Clear();
    UpdQry.SQLText =
        "UPDATE trip_tasks "
        "SET next_exec=DECODE(tid, :tid, DECODE(next_exec, :next_exec, TO_DATE(NULL), next_exec), next_exec), last_exec=:last_exec "
        "WHERE id=:id";
    UpdQry.DeclareVariable("id", otInteger);
    UpdQry.DeclareVariable("tid", otInteger);
    UpdQry.DeclareVariable("next_exec", otDate);
    UpdQry.DeclareVariable("last_exec", otDate);

    for(const auto& i : tasks)
    {
        TReqInfo::Instance()->clear();
        emptyHookTables();

        int task_id=i.first;
        const TTripTaskKey& task=i.second;
        try
        {
            SelQry.SetVariable("id", task_id);
            SelQry.Execute();
            if (SelQry.Eof) continue;
            TDateTime next_exec=SelQry.FieldAsDateTime("next_exec");
            TDateTime last_exec=SelQry.FieldIsNULL("last_exec")?ASTRA::NoExists:SelQry.FieldAsDateTime("last_exec");
            int tid=SelQry.FieldIsNULL("tid")?ASTRA::NoExists:SelQry.FieldAsInteger("tid");

            map<string, void (*)(const TTripTaskKey&)>::const_iterator iTask = TTripTasks::Instance()->items.find(task.name);
            if(iTask == TTripTasks::Instance()->items.end())
              throw EXCEPTIONS::Exception("task %s not found", task.name.c_str());

            if (last_exec==ASTRA::NoExists || last_exec<next_exec)
            {
              ProgTrace(TRACE5, "%s: task started (%s)",
                        __FUNCTION__, task.traceStr().c_str());

              iTask->second(task);

              taskToLog(task, TaskState::Done, next_exec);
              ProgTrace(TRACE5, "%s: task finished (%s)",
                        __FUNCTION__, task.traceStr().c_str());
            };
            UpdQry.SetVariable("id", task_id);
            if (tid!=ASTRA::NoExists)
              UpdQry.SetVariable("tid", tid);
            else
              UpdQry.SetVariable("tid", FNull);

            UpdQry.SetVariable("next_exec", next_exec);
            UpdQry.SetVariable("last_exec", nowUTC);
            UpdQry.Execute();

            callPostHooksBefore();
            OraSession.Commit();
            callPostHooksAfter();
        }
        catch( EOracleError &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            LogError(STDLOG) << __FUNCTION__ << ": " << task;
            ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
            ProgError( STDLOG, "SQL: %s", E.SQLText());
        }
        catch( EXCEPTIONS::Exception &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            LogError(STDLOG) << __FUNCTION__ << ": " << task;
            ProgError( STDLOG, "Exception: %s", E.what());
        }
        catch( std::exception &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            LogError(STDLOG) << __FUNCTION__ << ": " << task;
            ProgError( STDLOG, "std::exception: %s", E.what());
        }
        catch( ... )
        {
            try { OraSession.Rollback(); } catch(...) {};
            LogError(STDLOG) << __FUNCTION__ << ": " << task;
            ProgError( STDLOG, "Unknown error");
        };
        callPostHooksAlways();
    };
    LogTrace(TRACE5) << "check_trip_tasks ended (" << tasks.size() << " tasks), handler_id=" << handler_id;
}
