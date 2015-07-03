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
                       atTlgIn,
                       atTlgOut,
                       atUnattachedTrfer,
                       atConflictTrfer,
                       atCrewCheckin,
                       atCrewNumber,
                       atCrewDiff,
                       atAPISDiffersFromBooking,
                       atAPISIncomplete,
                       atAPISManualInput,
                       atUnboundEMD,
                       atAPPSProblem,
                       atAPPSOutage,
                       atAPPSConflict,
                       atAPPSNegativeDirective,
                       atAPPSError,
                       atLength };
extern const char *TripAlarmsTypeS[];

std::string EncodeAlarmType(TTripAlarmsType alarm);
TTripAlarmsType DecodeAlarmType(const std::string &alarm);

void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms );
void PaxAlarms( int pax_id, BitSet<TTripAlarmsType> &Alarms );
std::string TripAlarmString( TTripAlarmsType alarm );
bool get_alarm( int point_id, TTripAlarmsType alarm_type );
bool get_pax_alarm( int pax_id, TTripAlarmsType alarm_type );
void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value );
void set_pax_alarm( int pax_id, TTripAlarmsType alarm_type, bool alarm_value );
void process_pax_alarm( const int pax_id, const TTripAlarmsType alarm_type, const bool alarm_value );
void synch_trip_alarm(int point_id, TTripAlarmsType alarm_type);

bool calc_overload_alarm( int point_id );
bool check_overload_alarm( int point_id );
bool check_waitlist_alarm( int point_id );
bool check_brd_alarm( int point_id );
bool check_tlg_in_alarm(int point_id_tlg, int point_id_spp);
bool check_tlg_out_alarm(int point_id);
bool check_spec_service_alarm(int point_id);
void check_unattached_trfer_alarm( const std::set<int> &point_ids );
void check_unattached_trfer_alarm( int point_id );
bool need_check_u_trfer_alarm_for_grp( int point_id );
void check_u_trfer_alarm_for_grp( int point_id,
                                  int grp_id,
                                  const std::map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> &tags_before );
void check_u_trfer_alarm_for_next_trfer( int id,  //�.�. point_id ��� grp_id
                                         ASTRA::TIdType id_type );
bool check_conflict_trfer_alarm(int point_id);
void check_crew_alarms(int point_id);
void check_crew_alarms_task(int point_id, const std::string& task_name, const std::string &params);
void check_apis_alarms(int point_id);
void check_apis_alarms(int point_id, const std::set<TTripAlarmsType> &checked_alarms);
void check_unbound_emd_alarm( int point_id );
void check_unbound_emd_alarm( std::set<int> &pax_ids );
bool check_app_alarm( int point_id );
bool calc_app_alarm( int point_id );

#endif

