#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "transfer.h"
#include "trip_tasks.h"
#include "passenger.h"
#include "serverlib/posthooks.h"

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
      APISControl,
      APISDiffersFromBooking,
      APISIncomplete,
      APISManualInput,
      UnboundEMD,
      APPSProblem,
      APPSOutage,
      APPSConflict,
      APPSNegativeDirective,
      APPSError,
      APPSNotScdInTime,
      IAPIProblem,
      IAPINegativeDirective,
      WBDifferLayout,
      WBDifferSeats,
//ниже перечислены не настоящие тревоги, а флаги о необходимости что-то сделать с пассажиром
//относятся только к тревогам пассажиров, а не рейсов
      SyncEmds,
      SyncCabinClass,
      SyncCustomAlarms,
      SyncIAPI,
      SyncAPPS
    };

    typedef std::list< std::pair<Enum, std::string> > TPairs;

    static const TPairs& pairs()
    {
      static TPairs l=
       {
        {Salon,                  "SALON"                    },
        {Waitlist,               "WAITLIST"                 },
        {Brd,                    "BRD"                      },
        {Overload,               "OVERLOAD"                 },
        {ETStatus,               "ET_STATUS"                },
        {DiffComps,              "DIFF_COMPS"               },
        {SpecService,            "SPEC_SERVICE"             },
        {TlgIn,                  "TLG_IN"                   },
        {TlgOut,                 "TLG_OUT"                  },
        {UnattachedTrfer,        "UNATTACHED_TRFER"         },
        {ConflictTrfer,          "CONFLICT_TRFER"           },
        {CrewCheckin,            "CREW_CHECKIN"             },
        {CrewNumber,             "CREW_NUMBER"              },
        {CrewDiff,               "CREW_DIFF"                },
        {APISControl,            "APIS_CONTROL"             },  //только для вызова хука и вычисления сразу трех тревог APIS
        {APISDiffersFromBooking, "APIS_DIFFERS_FROM_BOOKING"},
        {APISIncomplete,         "APIS_INCOMPLETE"          },
        {APISManualInput,        "APIS_MANUAL_INPUT"        },
        {UnboundEMD,             "UNBOUND_EMD"              },
        {APPSProblem,            "APPS_PROBLEM"             },
        {APPSOutage,             "APPS_OUTAGE"              },
        {APPSConflict,           "APPS_CONFLICT"            },
        {APPSNegativeDirective,  "APPS_NEGATIVE_DIRECTIVE"  },
        {APPSError,              "APPS_ERROR"               },
        {APPSNotScdInTime,       "APPS_NOT_SCD_IN_TIME"     },
        {IAPIProblem,            "IAPI_PROBLEM"             },
        {IAPINegativeDirective,  "IAPI_NEGATIVE_DIRECTIVE"  },
        {WBDifferLayout,         "WB_DIFF_LAYOUT"           },
        {WBDifferSeats,          "WB_DIFF_SEATS"            },
        {SyncEmds,               "SYNC_EMDS"                },
        {SyncCabinClass,         "SYNC_CABIN_CLASS"         },
        {SyncCustomAlarms,       "SYNC_CUSTOM_ALARMS"       },
        {SyncIAPI,               "SYNC_IAPI"                },
        {SyncAPPS,               "SYNC_APPS"                },
       };
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

void checkAlarm(const TTripTaskKey &task);
void addTaskForCheckingAlarm(int point_id, Alarm::Enum alarm);

void TripAlarms( int point_id, BitSet<Alarm::Enum> &Alarms );
std::string TripAlarmString( Alarm::Enum alarm );
bool get_alarm( int point_id, Alarm::Enum alarm_type );
void set_alarm( int point_id, Alarm::Enum alarm_type, bool alarm_value );

bool calc_overload_alarm( int point_id );
bool check_overload_alarm( int point_id );
bool check_waitlist_alarm( int point_id );
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
void check_crew_alarms_task(const TTripTaskKey &task);
void check_apis_alarms(int point_id);
void check_apis_alarms(int point_id, const std::set<Alarm::Enum> &checked_alarms);
bool check_apps_alarm(int point_id);
bool check_iapi_alarm(int point_id);
bool check_apps_scd_alarm(const PointId_t& point_id);

template<typename T>
class TSomeonesAlarm
{
  public:
    Alarm::Enum type;
    T id;
    TSomeonesAlarm(Alarm::Enum _type, const T& _id) : type(_type), id(_id) {}
    bool operator < (const TSomeonesAlarm &alarm) const
    {
      if (!(id==alarm.id))
        return id < alarm.id;
      return type < alarm.type;
    }
    std::string traceStr()
    {
      std::ostringstream s;
      s << "type=" << AlarmTypes().encode(type) << ", id=" << id;
      return s.str();
    }
};

class TTripAlarm : public TSomeonesAlarm<int>
{
  public:
    TTripAlarm(Alarm::Enum _type, const int& _id) : TSomeonesAlarm<int>(_type, _id) {}
    void check();
};

class TGrpAlarm : public TSomeonesAlarm<int>
{
  public:
    TGrpAlarm(Alarm::Enum _type, const int& _id) : TSomeonesAlarm<int>(_type, _id) {}
    void check();
};

class TPaxAlarm : public TSomeonesAlarm<int>
{
  public:
    TPaxAlarm(Alarm::Enum _type, const int& _id) : TSomeonesAlarm<int>(_type, _id) {}
    void check();
};

template<typename T>
class TSomeonesAlarmHook : public Posthooks::Simple<T>
{
  public:
    TSomeonesAlarmHook(const T& _alarm) : Posthooks::Simple<T>(_alarm) {}
};

class TTripAlarmHook : public TSomeonesAlarmHook<TTripAlarm>
{
  public:
    static void set(Alarm::Enum _type, const int& _id);
    static void setAlways(Alarm::Enum _type, const int& _id);
};

class TGrpAlarmHook : public TSomeonesAlarmHook<TGrpAlarm>
{
  public:
    static void set(Alarm::Enum _type, const int& _id);
};

class TPaxAlarmHook : public TSomeonesAlarmHook<TPaxAlarm>
{
  public:
    static void set(Alarm::Enum _type, const int& _id);
};

class TCrsPaxAlarmHook : public TSomeonesAlarmHook<TPaxAlarm>
{
  public:
    static void set(Alarm::Enum _type, const int& _id);
};

bool addAlarmByPaxId(const int paxId,
                     const std::initializer_list<Alarm::Enum>& alarms,
                     const std::initializer_list<PaxOrigin>& origins);
bool deleteAlarmByPaxId(const int paxId,
                        const std::initializer_list<Alarm::Enum>& alarms,
                        const std::initializer_list<PaxOrigin>& origins);
bool deleteAlarmByGrpId(const int grpId, const Alarm::Enum alarmType);
bool existsAlarmByPaxId(const int paxId, const Alarm::Enum alarmType, const PaxOrigin paxOrigin);
bool existsAlarmByGrpId(const int grpId, const Alarm::Enum alarmType);
bool existsAlarmByPointId(const int pointId,
                          const std::initializer_list<Alarm::Enum>& alarms,
                          const std::initializer_list<PaxOrigin>& origins);
void getAlarmByPointId(const int pointId, const Alarm::Enum alarmType, std::set<int>& paxIds);
std::set<PaxId_t> getAlarmByPointId(const PointId_t& pointId, const Alarm::Enum alarmType,
                                    const PaxOrigin origin);

#endif

