#ifndef _TRIP_TASKS_H_
#define _TRIP_TASKS_H_

#include <string>
#include "date_time.h"
#include "astra_consts.h"
#include "oralib.h"
#include <set>

using BASIC::date_time::TDateTime;

const std::string all_other_handler_id="all_other";

const std::string US_CUSTOMS_CODE="ЮС";
const std::string TR_CUSTOMS_CODE="ТР";
const std::string BEFORE_TAKEOFF_30="Takeoff -30";
const std::string BEFORE_TAKEOFF_60="Takeoff -60";
const std::string ON_CLOSE_CHECKIN="ON_CLOSE_CHECKIN";
const std::string ON_CLOSE_BOARDING="ON_CLOSE_BOARDING";
const std::string ON_TAKEOFF="ON_TAKEOFF";
const std::string CREATE_APIS="APIS";
const std::string CHECK_CREW_ALARMS="CREW_ALARMS";

const std::string SYNC_NEW_CHKD="SYNC_NEW_CHKD";
const std::string SYNC_ALL_CHKD="SYNC_ALL_CHKD";
const std::string STAT_FV="STAT_FV";
const std::string EMD_REFRESH="EMD_REFRESH";
const std::string EMD_TRY_BIND="EMD_TRY_BIND";
const std::string EMD_SYS_UPDATE="EMD_SYS_UPDATE";
const std::string SEND_NEW_APPS_INFO="SEND_NEW_APPS_INFO";
const std::string SEND_ALL_APPS_INFO="SEND_ALL_APPS_INFO";
const std::string COLLECT_STAT="COLLECT_STAT";
const std::string SEND_TYPEB_ON_TAKEOFF="SEND_TYPEB_ON_TAKEOFF";

class TTripTaskKey
{
  public:
    int point_id;
    std::string name;
    std::string params;
    TTripTaskKey(int _point_id, const std::string& _name, const std::string& _params) :
      point_id(_point_id), name(_name), params(_params) {}
    TTripTaskKey(TQuery &Qry) { fromDB(Qry); }
    const TTripTaskKey& toDB(TQuery &Qry) const;
    TTripTaskKey& fromDB(TQuery &Qry);
    std::string traceStr() const;
};

struct TTripTasks {
    std::map<std::string, void (*)(const TTripTaskKey&)> items;
    TTripTasks();
    static TTripTasks *Instance();
};

class TaskState
{
  public:
    enum Enum {Add, Update, Delete, Done};
};

void taskToLog(
        const TTripTaskKey& task,
        TaskState::Enum ts,
        TDateTime next_exec,
        TDateTime new_next_exec = ASTRA::NoExists);

std::ostream& operator<<(std::ostream& os, const TTripTaskKey& task);

void add_trip_task(int point_id,
                   const std::string& task_name,
                   const std::string &params,
                   TDateTime new_next_exec=ASTRA::NoExists);
void add_trip_task(const TTripTaskKey &task,
                   TDateTime new_next_exec=ASTRA::NoExists);
void remove_trip_task(const TTripTaskKey &task);

#define CALL_POINT (string)__FILE__ + ":" +  IntToString(__LINE__)

class ChangeTrip
{
  public:
    enum Whence
    {
      ExecStages,
      SoppWriteTrips,
      SoppWriteDests,
      DropFlightFact,
      DeleteISGTrips,
      CrsDataApplyUpdates,
      SeasonCreateSPP,
      PointsDestDoEvents,
      FlightStagesSave,
      AODBParseFlight
    };
};

void on_change_trip(const std::string &descr, int point_id, ChangeTrip::Whence whence);

struct TSimpleFltInfo {
    std::string airline;
    std::string airp_dep;
    std::string airp_arv;
    int flt_no;
    TSimpleFltInfo(): flt_no(ASTRA::NoExists) {};
};

void calc_tlg_out_point_ids(const TSimpleFltInfo &flt, std::set<int> &point_ids);
void calc_tlg_out_point_ids(int typeb_addrs_id, std::set<int> &point_ids, std::string &tlg_type);

class TSyncTlgOutMng {
    private:
        static const std::string cache_prefix;
        static const std::string cache_fwd;
        static const std::string fwd_postfix;
        std::map<std::string, void (*)(int)> items;
    public:
        static TSyncTlgOutMng *Instance();
        TSyncTlgOutMng();

        void sync_all(int point_id);

        bool IsTlgToSync(const std::string &tlg_type);
        bool IsCacheToSync(const std::string &cache_name);

        // Из названия кэша достается тип телеграммы, напр. TYPEB_ADDRS_LCI -> LCI
        // и синхронизируется только этот тип.
        void sync_by_cache(const std::string &cache_name, int point_id);

        void sync_by_type(const std::string &type, int point_id);
        void add_tasks(std::map<std::string, void (*)(const TTripTaskKey&)> &items);
};

void deferOrExecuteFlightTask(const TTripTaskKey& task);

#endif

