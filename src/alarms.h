#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "transfer.h"

class Alarm
{
  public:
    enum Enum
    {
      Salon,
      Waitlist,
      Brd,
      Overload,
      ETStatus,
      DiffComps,
      SpecService,
      TlgIn,
      TlgOut,
      UnattachedTrfer,
      ConflictTrfer,
      CrewCheckin,
      CrewNumber,
      CrewDiff,
      APISDiffersFromBooking,
      APISIncomplete,
      APISManualInput,
      UnboundEMD,
      APPSProblem,
      APPSOutage,
      APPSConflict,
      APPSNegativeDirective,
      APPSError,
      WBDifferLayout,
      WBDifferSeats
    };

    typedef std::list< std::pair<Enum, std::string> > TPairs;

    static const TPairs& pairs()
    {
      static TPairs l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Salon,                  "SALON"                    ));
        l.push_back(std::make_pair(Waitlist,               "WAITLIST"                 ));
        l.push_back(std::make_pair(Brd,                    "BRD"                      ));
        l.push_back(std::make_pair(Overload,               "OVERLOAD"                 ));
        l.push_back(std::make_pair(ETStatus,               "ET_STATUS"                ));
        l.push_back(std::make_pair(DiffComps,              "DIFF_COMPS"               ));
        l.push_back(std::make_pair(SpecService,            "SPEC_SERVICE"             ));
        l.push_back(std::make_pair(TlgIn,                  "TLG_IN"                   ));
        l.push_back(std::make_pair(TlgOut,                 "TLG_OUT"                  ));
        l.push_back(std::make_pair(UnattachedTrfer,        "UNATTACHED_TRFER"         ));
        l.push_back(std::make_pair(ConflictTrfer,          "CONFLICT_TRFER"           ));
        l.push_back(std::make_pair(CrewCheckin,            "CREW_CHECKIN"             ));
        l.push_back(std::make_pair(CrewNumber,             "CREW_NUMBER"              ));
        l.push_back(std::make_pair(CrewDiff,               "CREW_DIFF"                ));
        l.push_back(std::make_pair(APISDiffersFromBooking, "APIS_DIFFERS_FROM_BOOKING"));
        l.push_back(std::make_pair(APISIncomplete,         "APIS_INCOMPLETE"          ));
        l.push_back(std::make_pair(APISManualInput,        "APIS_MANUAL_INPUT"        ));
        l.push_back(std::make_pair(UnboundEMD,             "UNBOUND_EMD"              ));
        l.push_back(std::make_pair(APPSProblem,            "APPS_PROBLEM"             ));
        l.push_back(std::make_pair(APPSOutage,             "APPS_OUTAGE"              ));
        l.push_back(std::make_pair(APPSConflict,           "APPS_CONFLICT"            ));
        l.push_back(std::make_pair(APPSNegativeDirective,  "APPS_NEGATIVE_DIRECTIVE"  ));
        l.push_back(std::make_pair(APPSError,              "APPS_ERROR"               ));
        l.push_back(std::make_pair(WBDifferLayout,         "WB_DIFF_LAYOUT"           ));
        l.push_back(std::make_pair(WBDifferSeats,          "WB_DIFF_SEATS"            ));
      }
      return l;
    }
};

class AlarmTypesList : public ASTRA::PairList<Alarm::Enum, std::string>
{
  private:
    virtual std::string className() const { return "AlarmTypesList"; }
  public:
    AlarmTypesList() : ASTRA::PairList<Alarm::Enum, std::string>(Alarm::pairs(),
                                                                 boost::none,
                                                                 boost::none) {}
};

const AlarmTypesList& AlarmTypes();

void TripAlarms( int point_id, BitSet<Alarm::Enum> &Alarms );
void PaxAlarms( int pax_id, BitSet<Alarm::Enum> &Alarms );
void CrsPaxAlarms( int pax_id, BitSet<Alarm::Enum> &Alarms );
std::string TripAlarmString( Alarm::Enum alarm );
bool get_alarm( int point_id, Alarm::Enum alarm_type );
bool get_pax_alarm( int pax_id, Alarm::Enum alarm_type );
bool get_crs_pax_alarm( int pax_id, Alarm::Enum alarm_type );
void set_alarm( int point_id, Alarm::Enum alarm_type, bool alarm_value );
void set_pax_alarm( int pax_id, Alarm::Enum alarm_type, bool alarm_value );
void set_crs_pax_alarm( int pax_id, Alarm::Enum alarm_type, bool alarm_value );
void synch_trip_alarm(int point_id, Alarm::Enum alarm_type);

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
void check_u_trfer_alarm_for_next_trfer( int id,  //м.б. point_id или grp_id
                                         ASTRA::TIdType id_type );
bool check_conflict_trfer_alarm(int point_id);
void check_crew_alarms(int point_id);
void check_crew_alarms_task(int point_id, const std::string& task_name, const std::string &params);
void check_apis_alarms(int point_id);
void check_apis_alarms(int point_id, const std::set<Alarm::Enum> &checked_alarms);
void check_unbound_emd_alarm( int point_id );
void check_unbound_emd_alarm( std::set<int> &pax_ids );
bool check_apps_alarm( int point_id );
bool calc_apps_alarm( int point_id );

#endif

