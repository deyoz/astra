#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "astra_utils.h"
#include "astra_misc.h"
#include "comp_layers.h"

enum TTripAlarmsType { atSalon, atWaitlist, atBrd, atOverload, atETStatus, atSeance, atDiffComps, atLength };
void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms );
std::string TripAlarmString( TTripAlarmsType alarm );
bool get_alarm( int point_id, TTripAlarmsType alarm_type );
void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value );

bool calc_overload_alarm( int point_id, const TTripInfo &fltInfo );
bool check_overload_alarm( int point_id, const TTripInfo &fltInfo );
bool check_waitlist_alarm( int point_id );
bool check_brd_alarm( int point_id );
void check_alarms(const TPointIdsForCheck &point_ids_spp);


#endif

