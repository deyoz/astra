#ifndef _TRIP_TASKS_H_
#define _TRIP_TASKS_H_

#include <string>
#include "basic.h"
#include "astra_consts.h"
#include <set>

const std::string US_CUSTOMS_CODE="ûë";
const std::string TR_CUSTOMS_CODE="íê";
const std::string BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL";
const std::string BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL";
const std::string BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL";
const std::string ON_CLOSE_CHECKIN="ON_CLOSE_CHECKIN";
const std::string ON_TAKEOFF="ON_TAKEOFF";
const std::string LCI="LCI";

const std::string SYNC_NEW_CHKD="SYNC_NEW_CHKD";
const std::string SYNC_ALL_CHKD="SYNC_ALL_CHKD";
const std::string EMD_SYS_UPDATE="EMD_SYS_UPDATE";

void add_trip_task(int point_id,
                   const std::string& task_name,
                   const std::string &params,
                   BASIC::TDateTime new_next_exec=ASTRA::NoExists);
void remove_trip_task(int point_id,
                      const std::string& task_name,
                      const std::string &params);

void check_trip_tasks();
#define CALL_POINT (string)__FILE__ + ":" +  IntToString(__LINE__)
void on_change_trip(const std::string &descr, int point_id);

void sync_lci_trip_tasks(int point_id);

struct TSimpleFltInfo {
    std::string airline;
    std::string airp_dep;
    std::string airp_arv;
    int flt_no;
    TSimpleFltInfo(): flt_no(ASTRA::NoExists) {};
};

void calc_lci_point_ids(const TSimpleFltInfo &flt, std::set<int> &point_ids);
void calc_lci_point_ids(int lci_typeb_addrs_id, std::set<int> &point_ids);

#endif

