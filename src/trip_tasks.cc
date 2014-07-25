#include "trip_tasks.h"
#include "apis.h"
#include "astra_consts.h"
#include "alarms.h"
#include "web_main.h"
#include "qrys.h"
#include "timer.h"
#include "telegram.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;

namespace TypeB {
    void check_lci(int point_id, const std::string &task_name, const std::string &params);
}

const
  struct {
    string task_name;
    void (*p)(int point_id, const string& task_name, const string &params);
  } trip_tasks []={
    {BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL, create_apis_task},
    {BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL, create_apis_task},
    {BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL, check_crew_alarms},
    {LCI, TypeB::check_lci},
    {SYNC_NEW_CHKD, TypeB::SyncNewCHKD },
    {SYNC_ALL_CHKD, TypeB::SyncAllCHKD }
  };

void sync_trip_task(int point_id, const string& task_name, const string &params, TDateTime next_exec)
{
  if (next_exec==ASTRA::NoExists)
    remove_trip_task(point_id, task_name, params);
  else      
    add_trip_task(point_id, task_name, params, next_exec);  
}

void LCIparamsToLog(LEvntPrms& prms, const string &params)
{
    TypeB::TCreatePoint cp(params);
    ostringstream result;
    result << setw(2) << setfill('0') << -cp.time_offset / 60
        << "-"
        << setw(2) << setfill('0') << -cp.time_offset % 60;
    PrmLexema lexema("params", "EVT.STAGE.TIME_BEFORE");
    lexema.prms << PrmElem<int>("stage", etGraphStage, cp.stage_id, efmtNameLong)
                << PrmSmpl<std::string>("time", result.str());
    prms << lexema;
}

void paramsToLog(LEvntPrms& prms, const string &task_name, const string &params)
{
    if(task_name == LCI)
        LCIparamsToLog(prms, params);
    else
        prms << PrmSmpl<std::string>("params", params);
}

enum TTaskState {tsAdd, tsUpdate, tsDelete, tsDone};

void taskToLog(TLogLocale& tlocale,
        const string &task_name,
        const string &params,
        TTaskState ts,
        TDateTime next_exec,
        TDateTime new_next_exec = ASTRA::NoExists)
{
    tlocale.prms << PrmSmpl<std::string>("task_name", task_name);

    paramsToLog(tlocale.prms, task_name, params);

    switch(ts) {
        case tsAdd:
            tlocale.lexema_id = "EVT.TASK_CREATED";
            break;
        case tsUpdate:
            tlocale.lexema_id = "EVT.TASK_MODIFIED";
            break;
        case tsDelete:
            tlocale.lexema_id = "EVT.TASK_DELETED";
            break;
        case tsDone:
            tlocale.lexema_id = "EVT.TASK_COMPLETED";
            break;
    }
    if(ts == tsUpdate) {
        PrmLexema lexema("time", "EVT.OLD_NEW_PLAN_TIME");
        lexema.prms << PrmDate("old_time", next_exec, "dd.mm.yy hh:nn")
                    << PrmDate("new_time", new_next_exec, "dd.mm.yy hh:nn");
        tlocale.prms << lexema;
    } else {
        PrmLexema lexema("time", "EVT.PLAN_TIME");
        lexema.prms << PrmDate("time", next_exec, "dd.mm.yy hh:nn");
        tlocale.prms << lexema;
    }
}

void add_trip_task(int point_id, const string& task_name, const string &params, TDateTime new_next_exec)
{
  if (new_next_exec==ASTRA::NoExists) new_next_exec=NowUTC();

  QParams QryParams;
  QryParams << QParam("point_id", otInteger, point_id);
  QryParams << QParam("name", otString, task_name);
  QryParams << QParam("params", otString, params);
  TCachedQuery CQry(
    "SELECT id, last_exec, next_exec FROM trip_tasks "
    "WHERE point_id=:point_id AND name=:name AND "
    "      (params = :params OR params IS NULL AND :params IS NULL) "
    "FOR UPDATE", QryParams);
  for(int pass=0; pass<2; pass++)
  {
    CQry.get().Execute();
    if (CQry.get().Eof)
    {
      TQuery Qry(&OraSession);
	     Qry.SQLText =
        "BEGIN "
        "  SELECT cycle_id__seq.nextval INTO :id FROM dual; "
        "  INSERT INTO trip_tasks(id, point_id, name, params, last_exec, next_exec) "
        "  VALUES(:id, :point_id, :name, :params, NULL, :next_exec); "
        "END;";
      Qry.DeclareVariable("id", otInteger);
      Qry.CreateVariable("point_id", otInteger, point_id);
      Qry.CreateVariable("name", otString, task_name);
      Qry.CreateVariable("params", otString, params);
      Qry.CreateVariable("next_exec", otDate, new_next_exec);
      try
      {
        Qry.Execute();
        if (Qry.RowsProcessed()>0)
        ProgTrace(TRACE5, "trip_tasks: task added (id=%d point_id=%d task_name=%s next_exec=%s)",
                          Qry.GetVariableAsInteger("id"),
                          point_id,
                          task_name.c_str(),
                          DateTimeToStr(new_next_exec, "dd.mm.yy hh:nn:ss").c_str());
        TLogLocale tlocale;
        tlocale.ev_type=ASTRA::evtFltTask;
        tlocale.id1=point_id;
        taskToLog(tlocale, task_name, params, tsAdd, new_next_exec);
        TReqInfo::Instance()->LocaleToLog(tlocale);
      }
      catch(EOracleError E)
      {
        if (E.Code==1 && pass==0) continue;
        throw;
      };
    }
    else
    {
      int task_id=CQry.get().FieldAsInteger("id");
      TDateTime last_exec=CQry.get().FieldIsNULL("last_exec")?
                            ASTRA::NoExists:
                            CQry.get().FieldAsDateTime("last_exec");
      TDateTime next_exec=CQry.get().FieldIsNULL("next_exec")?
                          ASTRA::NoExists:
                          CQry.get().FieldAsDateTime("next_exec");
      if ((last_exec==ASTRA::NoExists || new_next_exec>last_exec) &&
          next_exec!=new_next_exec)
      {
        TQuery Qry(&OraSession);
	       Qry.SQLText =
          "UPDATE trip_tasks SET next_exec=:next_exec WHERE id=:id";
        Qry.CreateVariable("id", otInteger, task_id);
        Qry.CreateVariable("next_exec", otDate, new_next_exec);
        Qry.Execute();
        if (Qry.RowsProcessed()>0)
        {
            if (next_exec!=ASTRA::NoExists) {
                ProgTrace(TRACE5, "trip_tasks: task changed (id=%d next_exec=%s new_next_exec=%s)",
                                   task_id,
                                   DateTimeToStr(next_exec, "dd.mm.yy hh:nn:ss").c_str(),
                                   DateTimeToStr(new_next_exec, "dd.mm.yy hh:nn:ss").c_str());
                TLogLocale tlocale;
                tlocale.ev_type=ASTRA::evtFltTask;
                tlocale.id1=point_id;
                taskToLog(tlocale, task_name, params, tsUpdate, next_exec, new_next_exec);
                TReqInfo::Instance()->LocaleToLog(tlocale);
            } else {
                ProgTrace(TRACE5, "trip_tasks: task added (id=%d next_exec=%s)",
                                  task_id,
                                  DateTimeToStr(new_next_exec, "dd.mm.yy hh:nn:ss").c_str());
                TLogLocale tlocale;
                tlocale.ev_type=ASTRA::evtFltTask;
                tlocale.id1=point_id;
                taskToLog(tlocale, task_name, params, tsAdd, new_next_exec);
                TReqInfo::Instance()->LocaleToLog(tlocale);
            }
        };
      };
    };
    break;
  };
};

void remove_trip_task(int point_id, const string& task_name, const string &params)
{
  QParams QryParams;
  QryParams << QParam("point_id", otInteger, point_id);
  QryParams << QParam("name", otString, task_name);
  QryParams << QParam("params", otString, params);
  TCachedQuery CQry(
    "SELECT id, last_exec, next_exec FROM trip_tasks "
    "WHERE point_id=:point_id AND name=:name AND "
    "      (params = :params OR params IS NULL AND :params IS NULL) "
    "FOR UPDATE", QryParams);
  CQry.get().Execute();
  if (CQry.get().Eof) return;
  int task_id=CQry.get().FieldAsInteger("id");
  TDateTime last_exec=CQry.get().FieldIsNULL("last_exec")?
                        ASTRA::NoExists:
                        CQry.get().FieldAsDateTime("last_exec");
  TDateTime next_exec=CQry.get().FieldIsNULL("next_exec")?
                        ASTRA::NoExists:
                        CQry.get().FieldAsDateTime("next_exec");

  if (last_exec!=ASTRA::NoExists)
  {
    //оставляем строку
    if (next_exec!=ASTRA::NoExists)
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText =
        "UPDATE trip_tasks SET next_exec=NULL WHERE id=:id";
      Qry.CreateVariable("id", otInteger, task_id);
      Qry.Execute();
      if (Qry.RowsProcessed()>0) {
          ProgTrace(TRACE5, "trip_tasks: task deleted (id=%d next_exec=%s)",
                            task_id,
                            DateTimeToStr(next_exec, "dd.mm.yy hh:nn:ss").c_str());
          TLogLocale tlocale;
          tlocale.ev_type=ASTRA::evtFltTask;
          tlocale.id1=point_id;
          taskToLog(tlocale, task_name, params, tsDelete, next_exec);
          TReqInfo::Instance()->LocaleToLog(tlocale);
      }
    };
  }
  else
  {
    //удаляем строку
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "DELETE FROM trip_tasks WHERE id=:id";
    Qry.CreateVariable("id", otInteger, task_id);
    Qry.Execute();
    if (Qry.RowsProcessed()>0 && next_exec!=ASTRA::NoExists) {
        ProgTrace(TRACE5, "trip_tasks: task deleted (id=%d next_exec=%s)",
                          task_id,
                          DateTimeToStr(next_exec, "dd.mm.yy hh:nn:ss").c_str());
        TLogLocale tlocale;
        tlocale.ev_type=ASTRA::evtFltTask;
        tlocale.id1=point_id;
        taskToLog(tlocale, task_name, params, tsDelete, next_exec);
        TReqInfo::Instance()->LocaleToLog(tlocale);
    }
  };
};

void check_trip_tasks()
{
    TDateTime nowUTC=NowUTC();

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "SELECT id, point_id, name, params FROM trip_tasks WHERE next_exec<=:now_utc";
    Qry.CreateVariable("now_utc", otDate, nowUTC);
    Qry.Execute();

    TQuery SelQry(&OraSession);
    SelQry.Clear();
    SelQry.SQLText =
        "SELECT last_exec, next_exec FROM trip_tasks "
        "WHERE id=:id AND next_exec<=:now_utc FOR UPDATE";
    SelQry.DeclareVariable("id", otInteger);
    SelQry.CreateVariable("now_utc", otDate, nowUTC);

    TQuery UpdQry(&OraSession);
    UpdQry.Clear();
    UpdQry.SQLText =
        "UPDATE trip_tasks "
        "SET next_exec=DECODE(next_exec, :next_exec, NULL, next_exec), last_exec=:last_exec "
        "WHERE id=:id";
    UpdQry.DeclareVariable("id", otInteger);
    UpdQry.DeclareVariable("next_exec", otDate);
    UpdQry.DeclareVariable("last_exec", otDate);

    for(;!Qry.Eof;Qry.Next())
    {
        int task_id=Qry.FieldAsInteger("id");
        int point_id=Qry.FieldAsInteger("point_id");
        string task_name=Qry.FieldAsString("name");
        string params = Qry.FieldAsString("params");

        try
        {
            SelQry.SetVariable("id", task_id);
            SelQry.Execute();
            if (SelQry.Eof) continue;
            TDateTime next_exec=SelQry.FieldAsDateTime("next_exec");
            TDateTime last_exec=SelQry.FieldIsNULL("last_exec")?ASTRA::NoExists:SelQry.FieldAsDateTime("last_exec");
            bool task_processed=false;
            for(int i=sizeof(trip_tasks)/sizeof(trip_tasks[0])-1;i>=0;i--)
            {
                if (trip_tasks[i].task_name!=task_name) continue;
                task_processed=true;
                if (last_exec==ASTRA::NoExists ||
                        last_exec<next_exec)
                {
                    trip_tasks[i].p(point_id, task_name, params);
                    TLogLocale tlocale;
                    tlocale.ev_type=ASTRA::evtFltTask;
                    tlocale.id1=point_id;
                    taskToLog(tlocale, task_name, params, tsDone, next_exec);
                    TReqInfo::Instance()->LocaleToLog(tlocale);
                };
            };
            if (task_processed)
            {
                UpdQry.SetVariable("id", task_id);
                UpdQry.SetVariable("next_exec", next_exec);
                UpdQry.SetVariable("last_exec", nowUTC);
                UpdQry.Execute();
            };
            OraSession.Commit();
        }
        catch( EOracleError &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s params=%s",
                    point_id, task_name.c_str(), params.c_str() );
            ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
            ProgError( STDLOG, "SQL: %s", E.SQLText());
        }
        catch( EXCEPTIONS::Exception &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s params=%s",
                    point_id, task_name.c_str(), params.c_str() );
            ProgError( STDLOG, "Exception: %s", E.what());
        }
        catch( std::exception &E )
        {
            try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s params=%s",
                    point_id, task_name.c_str(), params.c_str() );
            ProgError( STDLOG, "std::exception: %s", E.what());
        }
        catch( ... )
        {
            try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "check_trip_tasks: point_id=%d task_name=%s params=%s",
                    point_id, task_name.c_str(), params.c_str() );
            ProgError( STDLOG, "Unknown error");
        };
    };
}

void get_flt_period(pair<TDateTime, TDateTime> &val)
{
    TDateTime now = NowUTC();
    modf(now, &now);
    val.first = now - 5;
    val.second = now + CREATE_SPP_DAYS() + 1;
}

void calc_lci_point_ids(const TSimpleFltInfo &flt, set<int> &lci_point_ids)
{
    QParams QryParams;
    ostringstream sql;
    sql << "select point_id, point_num, first_point, pr_tranzit from points where ";
    if(not flt.airline.empty()) {
        sql << "airline = :airline and ";
        QryParams << QParam("airline", otString, flt.airline);
    }
    if(not flt.airp_dep.empty()) {
        sql << "airp = :airp_dep and ";
        QryParams << QParam("airp_dep", otString, flt.airp_dep);
    }
    if(flt.flt_no != ASTRA::NoExists) {
        sql << "flt_no = :flt_no and ";
        QryParams << QParam("flt_no", otInteger, flt.flt_no);
    }
    sql << "(time_out>=:first_date AND time_out<=:last_date) AND act_out IS NULL ";
    pair<TDateTime, TDateTime> period;
    get_flt_period(period);
    QryParams << QParam("first_date", otDate, period.first);
    QryParams << QParam("last_date", otDate, period.second);
    TCachedQuery Qry(sql.str(), QryParams);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        if(not flt.airp_arv.empty()) {
            TTripRoute route;
            route.GetRouteAfter(ASTRA::NoExists,
                    Qry.get().FieldAsInteger("point_id"),
                    Qry.get().FieldAsInteger("point_num"),
                    Qry.get().FieldAsInteger("first_point"),
                    Qry.get().FieldAsInteger("pr_tranzit"),
                    trtNotCurrent,trtNotCancelled);
            if (route.empty() or route.begin()->airp != flt.airp_arv)
                continue;
        }
        lci_point_ids.insert(Qry.get().FieldAsInteger("point_id"));
    }
}

void calc_lci_point_ids(int lci_typeb_addrs_id, set<int> &lci_point_ids)
{
    QParams QryParams;
    QryParams << QParam("id", otInteger, lci_typeb_addrs_id);
    TCachedQuery Qry("select airline, flt_no, airp_dep, airp_arv from typeb_addrs where id = :id", QryParams);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        TSimpleFltInfo flt;
        flt.airline = Qry.get().FieldAsString("airline");
        flt.airp_dep = Qry.get().FieldAsString("airp_dep");
        flt.airp_arv = Qry.get().FieldAsString("airp_arv");
        flt.flt_no = Qry.get().FieldAsInteger("flt_no");
        calc_lci_point_ids(flt, lci_point_ids);
    }
}

struct TTripTask {
    public:
        int point_id;
        string name; // название задачи trip_tasks.name
        virtual string paramsToString()const =0;
        virtual void paramsFromString(const string &params)=0;
        virtual TDateTime actual_next_exec(TDateTime curr_next_exec) const = 0;
        TTripTask(): point_id(ASTRA::NoExists) {};
        TTripTask(int vpoint_id, const string &vname): point_id(vpoint_id), name(vname) {};
        bool operator < (const TTripTask &task) const
        {
            if(point_id != task.point_id) 
                return point_id < task.point_id;
            return name < task.name;
        }
        bool operator == (const TTripTask &task) const
        {
            return
                point_id == task.point_id and
                name == task.name;

        }
};

struct TCreatePointTripTask:public TTripTask {
    public:
        TypeB::TCreatePoint create_point;
        string paramsToString() const;
        void paramsFromString(const string &params);
        TDateTime actual_next_exec(TDateTime curr_next_exec) const;
        TCreatePointTripTask(int vpoint_id, const string &vname):TTripTask(vpoint_id, vname){}
        bool operator < (const TCreatePointTripTask &task) const
        {
            if ( not(dynamic_cast<const TTripTask&>(*this) == dynamic_cast<const TTripTask&>(task)) )
                return dynamic_cast<const TTripTask&>(*this) < dynamic_cast<const TTripTask&>(task);
            return create_point < task.create_point;
        }
        bool operator == (const TCreatePointTripTask &task) const
        {
            return
                dynamic_cast<const TTripTask&>(*this) == dynamic_cast<const TTripTask&>(task) and
                create_point == task.create_point;
        }
};

struct TLCITripTask:public TCreatePointTripTask {
    public:
        TLCITripTask(int vpoint_id): TCreatePointTripTask(vpoint_id, LCI) {}
        TDateTime actual_next_exec(TDateTime curr_next_exec) const;
        bool operator < (const TLCITripTask &task) const
        {
            return dynamic_cast<const TCreatePointTripTask&>(*this) < dynamic_cast<const TCreatePointTripTask&>(task);
        }
        bool operator == (const TLCITripTask &task) const
        {
            return
                dynamic_cast<const TCreatePointTripTask&>(*this) == dynamic_cast<const TCreatePointTripTask&>(task);
        }
};

TDateTime TLCITripTask::actual_next_exec(TDateTime curr_next_exec) const
{
    TTripStage ts;
    TTripStages::LoadStage(point_id, (TStage)create_point.stage_id, ts);
    TDateTime result=ASTRA::NoExists;
    if(create_point.time_offset < 0) {
        if (ts.act == ASTRA::NoExists)            
          result = ts.est != ASTRA::NoExists ? ts.est : ts.scd;
        else           
          //Удаляем из trip_tasks запланированную, но еще не выполненную задачу
          return ASTRA::NoExists;
    };
    if(create_point.time_offset == 0) {
        if (ts.act == ASTRA::NoExists)
          //ничего не меняем в trip_tasks
          return curr_next_exec;
        else
          result = ts.act;
    };    
    if(create_point.time_offset > 0) {
        return ASTRA::NoExists; 
    };    
    
    if (result != ASTRA::NoExists)
        result = result + create_point.time_offset / 1440.;
    return result;
}

TDateTime TCreatePointTripTask::actual_next_exec(TDateTime curr_next_exec) const
{
    TTripStage ts;
    TTripStages::LoadStage(point_id, (TStage)create_point.stage_id, ts);
    TDateTime result=ASTRA::NoExists;
    if(create_point.time_offset < 0) {
        if (ts.act == ASTRA::NoExists)            
          result = ts.est != ASTRA::NoExists ? ts.est : ts.scd;
        else           
          //ничего не меняем в trip_tasks  
          return curr_next_exec;
    };
    if(create_point.time_offset == 0) {
        if (ts.act == ASTRA::NoExists)
          //ничего не меняем в trip_tasks
          return curr_next_exec;
        else
          result = ts.act;            
    };    
    if(create_point.time_offset > 0) {
        if (ts.act == ASTRA::NoExists)
          //удаление из trip_tasks
          return ASTRA::NoExists;
        else
          result = ts.act;  
    };    
    
    if (result != ASTRA::NoExists)
        result = result + create_point.time_offset / 1440.;
    return result;
}

void TCreatePointTripTask::paramsFromString(const string &params)
{
    create_point.paramsFromString(params);
}

string TCreatePointTripTask::paramsToString() const
{
    return create_point.paramsToString();
}

struct TTripTaskTimes
{
    TDateTime last_exec, next_exec;
    TTripTaskTimes() : last_exec(ASTRA::NoExists),
                       next_exec(ASTRA::NoExists)
    {}
    TTripTaskTimes(TQuery &Qry)
    {
        last_exec = Qry.FieldIsNULL("last_exec") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_exec");
        next_exec = Qry.FieldIsNULL("next_exec") ? ASTRA::NoExists : Qry.FieldAsDateTime("next_exec");
    }
};

template <typename T>
void get_curr_trip_tasks(const T &pattern, map<T, TTripTaskTimes> &tasks)
{
    tasks.clear();
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, pattern.point_id);
    QryParams << QParam("name", otString, pattern.name);
    TCachedQuery Qry("select next_exec, last_exec, params from trip_tasks where point_id = :point_id and name = :name for update", QryParams);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        T task(pattern.point_id);
        task.paramsFromString(Qry.get().FieldAsString("params"));
        if(not tasks.insert(make_pair(task, TTripTaskTimes(Qry.get()))).second)
            throw EXCEPTIONS::Exception("get_curr_trip_tasks: insert failed");
    }
}

namespace TypeB {

void check_lci(int point_id, const string &task_name, const string &params)
{
    vector<TCreateInfo> createInfo;
    TCreator creator(point_id, TCreatePoint(params));
    creator << "LCI";
    creator.getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo);
}

template <typename T>
void get_actual_trip_tasks(const T &pattern, set<T> &tasks)
{
    tasks.clear();
    QParams QryParams;
    QryParams.clear();
    QryParams << QParam("point_id", otInteger, pattern.point_id);
    TCachedQuery fltQry(
            "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
            "       point_id,point_num,first_point,pr_tranzit "
            "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0",
            QryParams);
    fltQry.get().Execute();
    TAdvTripInfo flt;
    if(fltQry.get().Eof) return;
    flt.Init(fltQry.get());
    set<TCreatePoint> cps;
    TSendInfo(pattern.name, flt, TCreatePoint()).
        getCreatePoints(vector<TSimpleMktFlight>(), cps); //!!!pattern.name & пустой mktFlights vlad это очень бэд и надо что-то с этим делать.
    for(set<TCreatePoint>::const_iterator i = cps.begin(); i != cps.end(); ++i) {
        T task(pattern.point_id);
        task.create_point=*i;
        tasks.insert(task);
    }
}

}

template <typename T>
void sync_trip_tasks(int point_id)
{
    map<T, TTripTaskTimes> curr_tasks;
    set<T> actual_tasks;
    T pattern(point_id);
    get_curr_trip_tasks<T>(pattern, curr_tasks);
    TypeB::get_actual_trip_tasks<T>(pattern, actual_tasks);
    typename map<T, TTripTaskTimes>::const_iterator curr_task=curr_tasks.begin();
    typename set<T>::const_iterator actual_task = actual_tasks.begin();
    for(;curr_task!=curr_tasks.end() || actual_task!=actual_tasks.end();)
    {
        bool proc_curr=   actual_task==actual_tasks.end() || 
                          (curr_task!=curr_tasks.end() &&  curr_task->first < *actual_task);

        bool proc_actual= curr_task==curr_tasks.end() ||
                          (actual_task!=actual_tasks.end() &&  *actual_task < curr_task->first);
        bool proc_curr_and_actual = 
                          curr_task!=curr_tasks.end() && 
                          actual_task!=actual_tasks.end() &&
                          curr_task->first == *actual_task;


        if (proc_curr_and_actual) {
            //синхронизация задач в trip_tasks
            sync_trip_task(actual_task->point_id, 
                           actual_task->name, 
                           actual_task->paramsToString(),
                           actual_task->actual_next_exec(curr_task->second.next_exec));
        }
        else   
        {
            if (proc_curr) {
                //надо удалить задачу из trip_tasks 
                sync_trip_task(curr_task->first.point_id, 
                               curr_task->first.name, 
                               curr_task->first.paramsToString(),
                               ASTRA::NoExists);
            }
            if(proc_actual) {
                //надо добавить задачу в trip_tasks
                sync_trip_task(actual_task->point_id, 
                               actual_task->name, 
                               actual_task->paramsToString(),
                               actual_task->actual_next_exec(ASTRA::NoExists));
            }
        };    


        if (proc_curr_and_actual || proc_curr) ++curr_task;
        if (proc_curr_and_actual || proc_actual) ++actual_task;
    }
}

void sync_lci_trip_tasks(int point_id)
{
    sync_trip_tasks<TLCITripTask>(point_id);
}

void on_change_trip(const string &descr, int point_id)
{
    ProgTrace(TRACE5, "%s: %s; point_id: %d", __FUNCTION__, descr.c_str(), point_id);
    try {
        sync_lci_trip_tasks(point_id);
    } catch(std::exception &E) {
        ProgError(STDLOG,"%s: %s (point_id=%d): %s", __FUNCTION__, descr.c_str(), point_id,E.what());
    };
}
