#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "astra_utils.h"
#include "astra_misc.h"

enum TTripAlarmsType { atSalon, atWaitlist, atBrd, atOverload, atETStatus, atSeance, atDiffComps, atSpecService, atTlgOut, atLength };
extern const char *TripAlarmsTypeS[];

std::string EncodeAlarmType(TTripAlarmsType alarm);
TTripAlarmsType DecodeAlarmType(const std::string &alarm);

void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms );
std::string TripAlarmString( TTripAlarmsType alarm );
bool get_alarm( int point_id, TTripAlarmsType alarm_type );
void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value );

bool calc_overload_alarm( int point_id );
bool check_overload_alarm( int point_id );
bool check_waitlist_alarm( int point_id );
bool check_brd_alarm( int point_id );
bool check_tlg_out_alarm(int point_id);


#endif

