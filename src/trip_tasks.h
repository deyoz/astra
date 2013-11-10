#ifndef _TRIP_TASKS_H_
#define _TRIP_TASKS_H_

#include <string>
#include "basic.h"
#include "astra_consts.h"

const std::string US_CUSTOMS_CODE="ž‘";
const std::string BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL";
const std::string BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL";
const std::string BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL="BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL";
const std::string ON_TAKEOFF="ON_TAKEOFF";

const std::string SYNC_NEW_CHKD="SYNC_NEW_CHKD";
const std::string SYNC_ALL_CHKD="SYNC_ALL_CHKD";

void add_trip_task(int point_id, const std::string& task_name, BASIC::TDateTime next_exec=ASTRA::NoExists);
void remove_trip_task(int point_id, const std::string& task_name);

void check_trip_tasks();


#endif

