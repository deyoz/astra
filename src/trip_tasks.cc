#include "trip_tasks.h"
#include "apis.h"
#include "astra_consts.h"
#include "alarms.h"
#include "web_main.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;

const
  struct {
    string task_name;
    void (*p)(int point_id, const string& task_name);
  } trip_tasks []={
    {BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL, create_apis_task},
    {BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL, create_apis_task},
    {BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL, check_crew_alarms},
    {SYNC_NEW_CHKD, TypeB::SyncNewCHKD },
    {SYNC_ALL_CHKD, TypeB::SyncAllCHKD }
  };

void add_trip_task(int point_id, const string& task_name, TDateTime next_exec)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText =
    "BEGIN "
    "  UPDATE trip_tasks SET next_exec=:next_exec WHERE point_id=:point_id AND name=:name; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO trip_tasks(point_id,name,last_exec,next_exec) "
    "    VALUES(:point_id,:name,NULL,:next_exec); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("name", otString, task_name);
  if (next_exec!=ASTRA::NoExists)
    Qry.CreateVariable("next_exec", otDate, next_exec);
  else
    Qry.CreateVariable("next_exec", otDate, NowUTC());
  Qry.Execute();
};

void remove_trip_task(int point_id, const string& task_name)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText = "DELETE trip_tasks WHERE point_id=:point_id AND name=:name";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("name", otString, task_name);
  Qry.Execute();
};

void check_trip_tasks()
{
  TDateTime nowUTC=NowUTC();

  TQuery Qry(&OraSession);
  Qry.Clear();
	Qry.SQLText =
    "SELECT point_id, name, last_exec, next_exec FROM trip_tasks WHERE next_exec<=:now_utc";
  Qry.CreateVariable("now_utc", otDate, nowUTC);
  Qry.Execute();

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText =
    "UPDATE trip_tasks "
    "SET next_exec=DECODE(next_exec, :next_exec, NULL, next_exec), last_exec=:last_exec "
    "WHERE point_id=:point_id AND name=:name";
  UpdQry.DeclareVariable("point_id", otInteger);
  UpdQry.DeclareVariable("name", otString);
  UpdQry.DeclareVariable("next_exec", otDate);
  UpdQry.DeclareVariable("last_exec", otDate);

  for(;!Qry.Eof;Qry.Next())
  {
    int point_id=Qry.FieldAsInteger("point_id");
    string task_name=Qry.FieldAsString("name");
    TDateTime next_exec=Qry.FieldAsDateTime("next_exec");
    TDateTime last_exec=Qry.FieldIsNULL("last_exec")?ASTRA::NoExists:Qry.FieldAsDateTime("last_exec");
    bool task_processed=false;
    try
    {
      for(int i=sizeof(trip_tasks)/sizeof(trip_tasks[0])-1;i>=0;i--)
      {
        if (trip_tasks[i].task_name!=task_name) continue;
        task_processed=true;
        if (last_exec==ASTRA::NoExists ||
            last_exec<next_exec)
        {
          ProgTrace(TRACE5, "check_trip_tasks: point_id=%d task_name=%s started", point_id, task_name.c_str());
          trip_tasks[i].p(point_id, task_name);

          ProgTrace(TRACE5, "check_trip_tasks: point_id=%d task_name=%s finished", point_id, task_name.c_str());
        };
      };
      if (task_processed)
      {
        UpdQry.SetVariable("point_id", point_id);
        UpdQry.SetVariable("name", task_name);
        UpdQry.SetVariable("next_exec", next_exec);
        UpdQry.SetVariable("last_exec", nowUTC);
        UpdQry.Execute();
      };
      OraSession.Commit();
    }
	  catch( EOracleError &E )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s", point_id, task_name.c_str() );
      ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
      ProgError( STDLOG, "SQL: %s", E.SQLText());
    }
    catch( EXCEPTIONS::Exception &E )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s", point_id, task_name.c_str() );
      ProgError( STDLOG, "Exception: %s", E.what());
    }
    catch( std::exception &E )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s", point_id, task_name.c_str() );
      ProgError( STDLOG, "std::exception: %s", E.what());
    }
    catch( ... )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s", point_id, task_name.c_str() );
      ProgError( STDLOG, "Unknown error");
    };

  };
}

