#include "trip_tasks.h"
#include "apis.h"
#include "astra_consts.h"
#include "alarms.h"
#include "web_main.h"
#include "qrys.h"
#include "timer.h"
#include "telegram.h"
#include "etick.h"
#include "tlg/remote_system_context.h"
#include "apps_interaction.h"
#include "counters.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;

const std::string LCI = "LCI";
const std::string COM = "COM";
const std::string SOM = "SOM";
const std::string FWD_POSTFIX = "->>";
const std::string UCM_FWD = "UCM" + FWD_POSTFIX;
const std::string LDM_FWD = "LDM" + FWD_POSTFIX;
const std::string CPM_FWD = "CPM" + FWD_POSTFIX;
const std::string SLS_FWD = "SLS" + FWD_POSTFIX;

namespace TypeB {
    void check_tlg_out(const TTripTaskKey &task);
}

void emd_sys_update(const TTripTaskKey &task);

const TTripTaskKey& TTripTaskKey::toDB(TQuery &Qry) const
{
  if (Qry.GetVariableIndex("point_id")<0)
    Qry.DeclareVariable("point_id", otInteger);
  if (Qry.GetVariableIndex("name")<0)
    Qry.DeclareVariable("name", otString);
  if (Qry.GetVariableIndex("params")<0)
    Qry.DeclareVariable("params", otString);
  point_id!=ASTRA::NoExists?
    Qry.SetVariable("point_id", point_id):
    Qry.SetVariable("point_id", FNull);
  Qry.SetVariable("name", name);
  Qry.SetVariable("params", params);
  return *this;
}

TTripTaskKey& TTripTaskKey::fromDB(TQuery &Qry)
{
  point_id=Qry.FieldIsNULL("point_id")?
             ASTRA::NoExists:
             Qry.FieldAsInteger("point_id");
  name=Qry.FieldAsString("name");
  params=Qry.FieldAsString("params");
  return *this;
}

std::string TTripTaskKey::traceStr() const
{
  std::ostringstream s;
  s << *this;
  return s.str();
}

std::ostream& operator<<(std::ostream& os, const TTripTaskKey& task)
{
  os << "point_id: " << task.point_id << ", "
        "name: " << task.name << ", "
        "params: " << task.params;
  return os;
}

struct TTripTasks {
    map<string, void (*)(const TTripTaskKey&)> items;
    TTripTasks();
    static TTripTasks *Instance();
};

TTripTasks::TTripTasks()
{
    items.insert(make_pair(SYNC_NEW_CHKD, TypeB::SyncNewCHKD ));
    items.insert(make_pair(SYNC_ALL_CHKD, TypeB::SyncAllCHKD ));
    items.insert(make_pair(EMD_REFRESH, emd_refresh_task ));
    items.insert(make_pair(EMD_TRY_BIND, emd_try_bind_task ));
    items.insert(make_pair(EMD_SYS_UPDATE, emd_sys_update ));
    items.insert(make_pair(SEND_NEW_APPS_INFO, sendNewAPPSInfo));
    items.insert(make_pair(SEND_ALL_APPS_INFO, sendAllAPPSInfo));
    items.insert(make_pair(CREATE_APIS, create_apis_task));
    items.insert(make_pair(CHECK_CREW_ALARMS, check_crew_alarms_task));
    TSyncTlgOutMng::Instance()->add_tasks(items);
}

TTripTasks *TTripTasks::Instance()
{
    static TTripTasks *instance_ = 0;
    if ( !instance_ )
        instance_ = new TTripTasks();
    return instance_;
}

void sync_trip_task(const TTripTaskKey& task, TDateTime next_exec)
{
  if (next_exec==ASTRA::NoExists)
    remove_trip_task(task);
  else
    add_trip_task(task, next_exec);
}

void TlgOutParamsToLog(LEvntPrms& prms, const string &params)
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
    if(TSyncTlgOutMng::Instance()->IsTlgToSync(task_name))
        TlgOutParamsToLog(prms, params);
    else
        prms << PrmSmpl<std::string>("params", params);
}

enum TTaskState {tsAdd, tsUpdate, tsDelete, tsDone};

void taskToLog(
        const TTripTaskKey& task,
        TTaskState ts,
        TDateTime next_exec,
        TDateTime new_next_exec = ASTRA::NoExists)
{
    TLogLocale tlocale;

    tlocale.ev_type=ASTRA::evtFltTask;
    tlocale.id1=task.point_id;

    tlocale.prms << PrmSmpl<std::string>("task_name", task.name);

    paramsToLog(tlocale.prms, task.name, task.params);

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

    TReqInfo::Instance()->LocaleToLog(tlocale);
}

void add_trip_task(int point_id, const string& task_name, const string &params,
                   TDateTime new_next_exec)
{
  add_trip_task(TTripTaskKey(point_id, task_name, params), new_next_exec);
}

void add_trip_task(const TTripTaskKey& task, TDateTime new_next_exec)
{
  if (new_next_exec==ASTRA::NoExists) new_next_exec=NowUTC();

  TCachedQuery CQry(
    "SELECT id, last_exec, next_exec FROM trip_tasks "
    "WHERE point_id=:point_id AND name=:name AND "
    "      (params = :params OR params IS NULL AND :params IS NULL) "
    "FOR UPDATE");
  task.toDB(CQry.get());
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
      Qry.CreateVariable("next_exec", otDate, new_next_exec);
      task.toDB(Qry);
      try
      {
        Qry.Execute();
        if (Qry.RowsProcessed()>0)
        ProgTrace(TRACE5, "trip_tasks: task added (id=%d point_id=%d task_name=%s next_exec=%s)",
                          Qry.GetVariableAsInteger("id"),
                          task.point_id,
                          task.name.c_str(),
                          DateTimeToStr(new_next_exec, "dd.mm.yy hh:nn:ss").c_str());

        taskToLog(task, tsAdd, new_next_exec);
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

                taskToLog(task, tsUpdate, next_exec, new_next_exec);
            } else {
                ProgTrace(TRACE5, "trip_tasks: task added (id=%d next_exec=%s)",
                                  task_id,
                                  DateTimeToStr(new_next_exec, "dd.mm.yy hh:nn:ss").c_str());

                taskToLog(task, tsAdd, new_next_exec);
            }
        };
      };
    };
    break;
  };
};

void remove_trip_task(const TTripTaskKey& task)
{
  TCachedQuery CQry(
    "SELECT id, last_exec, next_exec FROM trip_tasks "
    "WHERE point_id=:point_id AND name=:name AND "
    "      (params = :params OR params IS NULL AND :params IS NULL) "
    "FOR UPDATE");
  task.toDB(CQry.get());
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

          taskToLog(task, tsDelete, next_exec);
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

        taskToLog(task, tsDelete, next_exec);
    }
  };
};

void check_trip_tasks()
{
    LogTrace(TRACE5) << "check_trip_tasks started";
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
        TReqInfo::Instance()->clear();

        int task_id=Qry.FieldAsInteger("id");
        TTripTaskKey task(Qry);
        try
        {
            SelQry.SetVariable("id", task_id);
            SelQry.Execute();
            if (SelQry.Eof) continue;
            TDateTime next_exec=SelQry.FieldAsDateTime("next_exec");
            TDateTime last_exec=SelQry.FieldIsNULL("last_exec")?ASTRA::NoExists:SelQry.FieldAsDateTime("last_exec");

            map<string, void (*)(const TTripTaskKey&)>::const_iterator iTask = TTripTasks::Instance()->items.find(task.name);
            if(iTask == TTripTasks::Instance()->items.end())
              throw EXCEPTIONS::Exception("task %s not found", task.name.c_str());

            if (last_exec==ASTRA::NoExists || last_exec<next_exec)
            {
              ProgTrace(TRACE5, "%s: task started (%s)",
                        __FUNCTION__, task.traceStr().c_str());

              iTask->second(task);

              taskToLog(task, tsDone, next_exec);
              ProgTrace(TRACE5, "%s: task finished (%s)",
                        __FUNCTION__, task.traceStr().c_str());
            };
            UpdQry.SetVariable("id", task_id);
            UpdQry.SetVariable("next_exec", next_exec);
            UpdQry.SetVariable("last_exec", nowUTC);
            UpdQry.Execute();

            OraSession.Commit();
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
    };
    LogTrace(TRACE5) << "check_trip_tasks ended";
}

void get_flt_period(pair<TDateTime, TDateTime> &val)
{
    TDateTime now = NowUTC();
    modf(now, &now);
    val.first = now - 5;
    val.second = now + CREATE_SPP_DAYS() + 1;
}

void calc_tlg_out_point_ids(const TSimpleFltInfo &flt, set<int> &tlg_out_point_ids)
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
        tlg_out_point_ids.insert(Qry.get().FieldAsInteger("point_id"));
    }
}

void calc_tlg_out_point_ids(int tlg_out_typeb_addrs_id, set<int> &tlg_out_point_ids, string &tlg_type)
{
    QParams QryParams;
    QryParams << QParam("id", otInteger, tlg_out_typeb_addrs_id);
    TCachedQuery Qry("select tlg_type, airline, flt_no, airp_dep, airp_arv from typeb_addrs where id = :id", QryParams);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        TSimpleFltInfo flt;
        tlg_type = Qry.get().FieldAsString("tlg_type");
        flt.airline = Qry.get().FieldAsString("airline");
        flt.airp_dep = Qry.get().FieldAsString("airp_dep");
        flt.airp_arv = Qry.get().FieldAsString("airp_arv");
        if(Qry.get().FieldIsNULL("flt_no"))
            flt.flt_no = ASTRA::NoExists;
        else
            flt.flt_no = Qry.get().FieldAsInteger("flt_no");
        calc_tlg_out_point_ids(flt, tlg_out_point_ids);
    }
}

struct TTripTask {
    public:
        int point_id;
        string name; // название задачи trip_tasks.name
        virtual string paramsToString()const =0;
        virtual void paramsFromString(const string &params)=0;
        virtual TDateTime actual_next_exec(TDateTime curr_next_exec) const = 0;
        TTripTask(): point_id(ASTRA::NoExists) {}
        TTripTask(int vpoint_id, const string &vname): point_id(vpoint_id), name(vname) {}
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
        virtual ~TTripTask() {};
};

struct TCreatePointTripTask:public TTripTask {
    public:
        TypeB::TCreatePoint create_point;
        string paramsToString() const;
        void paramsFromString(const string &params);
//        TDateTime actual_next_exec(TDateTime curr_next_exec) const;
        TCreatePointTripTask(int vpoint_id, const string &vname):TTripTask(vpoint_id, vname){}
        bool operator < (const TCreatePointTripTask &task) const
        {
          if (!(TTripTask::operator ==(task)))
            return TTripTask::operator <(task);
          return create_point < task.create_point;
        }
        bool operator == (const TCreatePointTripTask &task) const
        {
          return TTripTask::operator ==(task) &&
              create_point == task.create_point;
        }
};

class TEmdRefreshTripTask: public TCreatePointTripTask
{
  public:
    TEmdRefreshTripTask(int vpoint_id): TCreatePointTripTask(vpoint_id, EMD_REFRESH) {}
    TDateTime actual_next_exec(TDateTime curr_next_exec) const;
    void get_actual_create_points(set<TypeB::TCreatePoint> &cps) const
    {
      cps.clear();
      cps.insert(TypeB::TCreatePoint(sCloseCheckIn, 0));
    }
};

struct TTlgOutTripTask:public TCreatePointTripTask {
    public:
        TTlgOutTripTask(int vpoint_id, const string &vtype): TCreatePointTripTask(vpoint_id, vtype) {}
        TDateTime actual_next_exec(TDateTime curr_next_exec) const;
        void get_actual_create_points(set<TypeB::TCreatePoint> &cps) const
        {
            cps.clear();
            QParams QryParams;
            QryParams.clear();
            QryParams << QParam("point_id", otInteger, point_id);
            TCachedQuery fltQry(
                    "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
                    "       point_id,point_num,first_point,pr_tranzit "
                    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0",
                    QryParams);
            fltQry.get().Execute();
            TAdvTripInfo flt;
            if(fltQry.get().Eof) return;
            flt.Init(fltQry.get());
            TypeB::TSendInfo(name, flt, TypeB::TCreatePoint()).
                getCreatePoints(vector<TSimpleMktFlight>(), cps); //!!!name & пустой mktFlights vlad это очень бэд и надо что-то с этим делать.
        }
};

TDateTime TTlgOutTripTask::actual_next_exec(TDateTime curr_next_exec) const
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

struct TSOMTripTask:public TTlgOutTripTask {
    TSOMTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, SOM) {}
};

struct TCOMTripTask:public TTlgOutTripTask {
    TCOMTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, COM) {}
};

struct TLCITripTask:public TTlgOutTripTask {
    TLCITripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, LCI) {}
};

struct TUCMFwdTripTask:public TTlgOutTripTask {
    TUCMFwdTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, UCM_FWD) {}
};

struct TLDMFwdTripTask:public TTlgOutTripTask {
    TLDMFwdTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, LDM_FWD) {}
};

struct TCPMFwdTripTask:public TTlgOutTripTask {
    TCPMFwdTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, CPM_FWD) {}
};

struct TSLSFwdTripTask:public TTlgOutTripTask {
    TSLSFwdTripTask(int vpoint_id): TTlgOutTripTask(vpoint_id, SLS_FWD) {}
};

struct TAPISTripTask:public TCreatePointTripTask {
    TAPISTripTask(int vpoint_id): TCreatePointTripTask(vpoint_id, CREATE_APIS) {}
};

struct TCrewAlarmsTripTask:public TCreatePointTripTask {
    TCrewAlarmsTripTask(int vpoint_id): TCreatePointTripTask(vpoint_id, CHECK_CREW_ALARMS) {}
};

TDateTime TEmdRefreshTripTask::actual_next_exec(TDateTime curr_next_exec) const
{
    TTripStage ts;
    TTripStages::LoadStage(point_id, (TStage)create_point.stage_id, ts);
    TDateTime result=ASTRA::NoExists;
    if (ts.act == ASTRA::NoExists)
      result = ts.est != ASTRA::NoExists ? ts.est : ts.scd;
    else
      result = ts.act;

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

bool is_fwd_tlg(const string &tlg_type)
{
    return tlg_type.substr(3, FWD_POSTFIX.size()) == FWD_POSTFIX;
}

namespace TypeB {

// task_name обязан быть типом телеграммы
void check_tlg_out(const TTripTaskKey& task)
{
    if(is_fwd_tlg(task.name)) {
        string tlg_type = task.name.substr(0, 3);
        TQuery Qry(&OraSession);
        Qry.SQLText=
            "SELECT "
            " tlgs_in.id, "
            " tlgs_in.num "
            "FROM tlgs_in, "
            "     (SELECT DISTINCT tlg_source.tlg_id AS id "
            "      FROM tlg_source,tlg_binding "
            "      WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
            "            tlg_binding.point_id_spp=:point_id and "
            "            tlg_source.has_errors = 0 and "
            "            tlg_source.has_alarm_errors = 0 "
            "     ) ids "
            "WHERE tlgs_in.id=ids.id and tlgs_in.type = :tlg_type "
            "ORDER BY id desc, num ";
        Qry.CreateVariable("point_id",otInteger,task.point_id);
        Qry.CreateVariable("tlg_type",otString,tlg_type);
        Qry.Execute();
        int aid = ASTRA::NoExists;
        for(;!Qry.Eof;Qry.Next()) {
            int id = Qry.FieldAsInteger("id");
            if(aid == ASTRA::NoExists)
                aid = id;
            else if(aid != id)
                break;
            TForwarder forwarder(
                    task.point_id,
                    Qry.FieldAsInteger("id"),
                    Qry.FieldAsInteger("num")
                    );
            forwarder << task.name;
            vector<TypeB::TCreateInfo> createInfo;
            forwarder.getInfo(createInfo);
            TelegramInterface::SendTlg(createInfo, ASTRA::NoExists, true);
        }
    } else {
        vector<TCreateInfo> createInfo;
        TCreator creator(task.point_id, TCreatePoint(task.params));
        creator << task.name;
        creator.getInfo(createInfo);
        TelegramInterface::SendTlg(createInfo);
    }
}

}

template <typename T>
void get_actual_trip_tasks(const T &pattern, set<T> &tasks)
{
    tasks.clear();

    set<TypeB::TCreatePoint> cps;
    pattern.get_actual_create_points(cps);
    for(set<TypeB::TCreatePoint>::const_iterator i = cps.begin(); i != cps.end(); ++i) {
        T task(pattern.point_id);
        task.create_point=*i;
        tasks.insert(task);
    }
}

template <typename T>
void sync_trip_tasks(int point_id)
{
    map<T, TTripTaskTimes> curr_tasks;
    set<T> actual_tasks;
    T pattern(point_id);
    get_curr_trip_tasks<T>(pattern, curr_tasks);
    get_actual_trip_tasks<T>(pattern, actual_tasks);
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
          sync_trip_task(TTripTaskKey(actual_task->point_id,
                                      actual_task->name,
                                      actual_task->paramsToString()),
                         actual_task->actual_next_exec(curr_task->second.next_exec));
        }
        else
        {
            if (proc_curr) {
                //надо удалить задачу из trip_tasks
              sync_trip_task(TTripTaskKey(curr_task->first.point_id,
                                          curr_task->first.name,
                                          curr_task->first.paramsToString()),
                             ASTRA::NoExists);
            }
            if(proc_actual) {
                //надо добавить задачу в trip_tasks
              sync_trip_task(TTripTaskKey(actual_task->point_id,
                                          actual_task->name,
                                          actual_task->paramsToString()),
                             actual_task->actual_next_exec(ASTRA::NoExists));
            }
        };


        if (proc_curr_and_actual || proc_curr) ++curr_task;
        if (proc_curr_and_actual || proc_actual) ++actual_task;
    }
}

void on_change_trip(const string &descr, int point_id, ChangeTrip::Whence whence)
{
    ProgTrace(TRACE5, "%s: %s; point_id: %d", __FUNCTION__, descr.c_str(), point_id);
    try {
        if (whence==ChangeTrip::SoppWriteDests ||
            whence==ChangeTrip::DeleteISGTrips ||
            whence==ChangeTrip::CrsDataApplyUpdates ||
            whence==ChangeTrip::SeasonCreateSPP ||
            whence==ChangeTrip::PointsDestDoEvents ||
            whence==ChangeTrip::AODBParseFlight)
          CheckIn::TCounters().recount(point_id, CheckIn::TCounters::Total, descr.c_str());

        TSyncTlgOutMng::Instance()->sync_all(point_id);
        sync_trip_tasks<TEmdRefreshTripTask>(point_id);
//        sync_trip_tasks<TAPISTripTask>(point_id);
//        sync_trip_tasks<TCrewAlarmsTripTask>(point_id);
    } catch(std::exception &E) {
        ProgError(STDLOG,"%s: %s (point_id=%d): %s", __FUNCTION__, descr.c_str(), point_id,E.what());
    };
}

void emd_sys_update(const TTripTaskKey &task)
{
  TReqInfo *reqInfo=TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeUTC;

  try
  {
    TEMDSystemUpdateList emdList;
    EMDSystemUpdateInterface::EMDCheckDisassociation(task.point_id, emdList);
    EMDSystemUpdateInterface::EMDChangeDisassociation(edifact::KickInfo(), emdList);
  }
  catch(AstraLocale::UserException &e)
  {
    string err_id;
    LEvntPrms err_prms;
    e.getAdvParams(err_id, err_prms);

    reqInfo->LocaleToLog("EVT.EMD_SYS_UPDATE",
                         LEvntPrms() << PrmLexema("text", err_id, err_prms),
                         ASTRA::evtPay,
                         task.point_id);
  };
}

const string TSyncTlgOutMng::cache_prefix = "TYPEB_ADDRS_";
const string TSyncTlgOutMng::cache_fwd = "FORWARDING";

TSyncTlgOutMng *TSyncTlgOutMng::Instance()
{
  static TSyncTlgOutMng *instance_ = 0;
  if ( !instance_ )
    instance_ = new TSyncTlgOutMng();
  return instance_;
};

void TSyncTlgOutMng::sync_all(int point_id)
{
    for(std::map<std::string, void (*)(int)>::iterator i = items.begin(); i != items.end(); i++) i->second(point_id);
}

void TSyncTlgOutMng::sync_by_type(const string &type, int point_id)
{
    std::map<std::string, void (*)(int)>::iterator i = items.find(type);
    if(i == items.end())
        throw EXCEPTIONS::Exception("sync_by_type: wrong type %s", type.c_str());
    i->second(point_id);
}

void TSyncTlgOutMng::sync_by_cache(const string &cache_name, int point_id)
{
    if(cache_name.substr(0, cache_prefix.size()) != cache_prefix)
        throw EXCEPTIONS::Exception("sync_by_cache: wrong cache %s", cache_name.c_str());
    try {
        if(cache_name.substr(cache_prefix.size()) == cache_fwd) {
            for(std::map<std::string, void (*)(int)>::iterator i = items.begin(); i != items.end(); i++) {
                if(is_fwd_tlg(i->first)) sync_by_type(i->first, point_id);
            }
        } else {
            sync_by_type(cache_name.substr(cache_prefix.size(), cache_prefix.size() + 3), point_id);
        }
    } catch(EXCEPTIONS::Exception &E) {
        throw EXCEPTIONS::Exception("sync_by_cache: %s, msg: %s", cache_name.c_str(), E.what());
    }
}

bool TSyncTlgOutMng::IsTlgToSync(const string &tlg_type)
{
    bool result = false;
    for(std::map<std::string, void (*)(int)>::iterator i = items.begin(); i != items.end(); i++) {
        if(i->first == tlg_type) {
            result = true;
            break;
        }
    }
    return result;
}

void TSyncTlgOutMng::add_tasks(map<string, void (*)(const TTripTaskKey&)> &tasks)
{
    for(std::map<std::string, void (*)(int)>::iterator i = items.begin(); i != items.end(); i++)
        tasks.insert(make_pair(i->first, TypeB::check_tlg_out));
}

bool TSyncTlgOutMng::IsCacheToSync(const string &cache_name)
{
    bool result = false;
    if(cache_name == cache_prefix + cache_fwd)
        result = true;
    else
        for(std::map<std::string, void (*)(int)>::iterator i = items.begin(); i != items.end(); i++) {
            if(cache_prefix + i->first == cache_name) {
                result = true;
                break;
            }
        }
    return result;
}

TSyncTlgOutMng::TSyncTlgOutMng()
{
    items.insert(make_pair(LCI, sync_trip_tasks<TLCITripTask>));
    items.insert(make_pair(COM, sync_trip_tasks<TCOMTripTask>));
    items.insert(make_pair(SOM, sync_trip_tasks<TSOMTripTask>));
    items.insert(make_pair(UCM_FWD, sync_trip_tasks<TUCMFwdTripTask>));
    items.insert(make_pair(LDM_FWD, sync_trip_tasks<TLDMFwdTripTask>));
    items.insert(make_pair(CPM_FWD, sync_trip_tasks<TCPMFwdTripTask>));
    items.insert(make_pair(SLS_FWD, sync_trip_tasks<TSLSFwdTripTask>));
}
