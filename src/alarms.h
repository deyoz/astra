#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "transfer.h"

enum TTripAlarmsType { atSalon,
                       atWaitlist,
                       atBrd,
                       atOverload,
                       atETStatus,
                       atSeance,
                       atDiffComps,
                       atSpecService,
                       atTlgOut,
                       atUnattachedTrfer,
                       atConflictTrfer,
                       atLength };
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
bool check_spec_service_alarm(int point_id);
void check_unattached_trfer_alarm( const std::set<int> &point_ids );
void check_unattached_trfer_alarm( int point_id );
bool need_check_u_trfer_alarm_for_grp( int point_id );
void check_u_trfer_alarm_for_grp( int point_id,
                                  int grp_id,
                                  const std::map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> &tags_before );
void check_u_trfer_alarm_for_next_trfer( int id,  //м.б. point_id или grp_id
                                         ASTRA::TIdType id_type );
bool check_conflict_trfer_alarm(int point_id);
void check_crew_alarms(int point_id, const std::string& task_name);


#endif

