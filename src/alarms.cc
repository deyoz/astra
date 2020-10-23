#include "alarms.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"
#include "pers_weights.h"
#include "astra_elems.h"
#include "remarks.h"
#include "transfer.h"
#include "typeb_utils.h"
#include "trip_tasks.h"
#include "salons.h"
#include "qrys.h"
#include "pax_calc_data.h"
#include "emdoc.h"
#include "sopp.h"
#include "date_time.h"
#include "custom_alarms.h"
#include "apis_creator.h"
#include "flt_settings.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

const AlarmTypesList &AlarmTypes()
{
  static AlarmTypesList alarmTypes;
  return alarmTypes;
}

void checkAlarm(const TTripTaskKey &task)
{
  Alarm::Enum alarm=AlarmTypes().decode(task.params);
  switch(alarm)
  {
    case Alarm::APISControl:
        check_apis_alarms(task.point_id);
        break;
    case Alarm::APPSProblem:
      check_apps_alarm(task.point_id);
      break;
    case Alarm::IAPIProblem:
      check_iapi_alarm(task.point_id);
      break;
    default:
      throw Exception("Unsupported");
      break;
  }
}

void addTaskForCheckingAlarm(int point_id, Alarm::Enum alarm)
{
  add_trip_task(TTripTaskKey(point_id, CHECK_ALARM, AlarmTypes().encode(alarm)));
}

void TripAlarms( int point_id, BitSet<Alarm::Enum> &Alarms )
{
    Alarms.clearFlags();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT pr_etstatus,pr_salon,act "
        " FROM trip_sets, trip_stages, "
        " ( SELECT COUNT(*) pr_salon FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2 ) a "
        " WHERE trip_sets.point_id=:point_id AND "
        "       trip_stages.point_id(+)=trip_sets.point_id AND "
        "       trip_stages.stage_id(+)=:OpenCheckIn ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "OpenCheckIn", otInteger, sOpenCheckIn );
    Qry.Execute();
    if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    TTripSetList setList;
    setList.fromDB(point_id);
    if (setList.empty()) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    if ( !Qry.FieldAsInteger( "pr_salon" ) &&
         !Qry.FieldIsNULL( "act" ) &&
         !setList.value<bool>(tsFreeSeating) ) {
        Alarms.setFlag( Alarm::Salon );
    }
    if ( Qry.FieldAsInteger( "pr_etstatus" ) < 0 ) {
        Alarms.setFlag( Alarm::ETStatus );
    }

    Qry.Clear();
    Qry.SQLText = "select alarm_type from trip_alarms where point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) Alarms.setFlag(AlarmTypes().decode(Qry.FieldAsString("alarm_type")));
}

string TripAlarmName( Alarm::Enum alarm )
{
    return ElemIdToElem(etAlarmType, AlarmTypes().encode(alarm), efmtNameLong, AstraLocale::LANG_RU);
}

string TripAlarmString( Alarm::Enum alarm )
{
    return ElemIdToNameLong(etAlarmType, AlarmTypes().encode(alarm));
}

bool get_alarm( int point_id, Alarm::Enum alarm_type )
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select * from trip_alarms where point_id = :point_id and alarm_type = :alarm_type";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("alarm_type", otString, AlarmTypes().encode(alarm_type));
    Qry.Execute();
    return !Qry.Eof;
}

void set_alarm( int point_id, Alarm::Enum alarm_type, bool alarm_value )
{
    TQuery Qry(&OraSession);
    if(alarm_value)
        Qry.SQLText = "insert into trip_alarms(point_id, alarm_type) values(:point_id, :alarm_type)";
    else
        Qry.SQLText = "delete from trip_alarms where point_id = :point_id and alarm_type = :alarm_type";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("alarm_type", otString, AlarmTypes().encode(alarm_type));
    try {
        Qry.Execute();
        if(Qry.RowsProcessed()) {
            TReqInfo::Instance()->LocaleToLog(alarm_value?"EVT.ALARM_SET":"EVT.ALARM_DELETED", LEvntPrms()
                                              << PrmElem<string>("alarm_type", etAlarmType, AlarmTypes().encode(alarm_type), efmtNameLong),
                                              evtFlt, point_id );
        }
    } catch (EOracleError &E) {
        if(E.Code != 1) throw;
    }
}

bool calc_overload_alarm( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT max_commerce FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  bool overload_alarm=false;
  if (!Qry.FieldIsNULL("max_commerce"))
  {
    int load = getCommerceWeight( point_id, onlyCheckin, CWTotal );
    ProgTrace(TRACE5,"check_overload_alarm: max_commerce=%d load=%d",Qry.FieldAsInteger("max_commerce"),load);
    overload_alarm=(load>Qry.FieldAsInteger("max_commerce"));
  };
  return overload_alarm;
};

bool check_overload_alarm( int point_id )
{
  bool overload_alarm=calc_overload_alarm( point_id );
  set_alarm( point_id, Alarm::Overload, overload_alarm );
    return overload_alarm;
};

bool calc_waitlist_alarm( int point_id )
{
  if ( SALONS2::isFreeSeating( point_id ) ) {
    return false;
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pax.pax_id "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.status NOT IN ('E') AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      salons.is_waitlist(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,rownum)<>0 AND "
    "      rownum<2";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return !Qry.Eof;
};

/* есть пассажиры, которые на листе ожидания */
bool check_waitlist_alarm( int point_id )
{
    bool waitlist_alarm = calc_waitlist_alarm( point_id );

  set_alarm( point_id, Alarm::Waitlist, waitlist_alarm );
    return waitlist_alarm;
};

/* есть пассажиры, которые зарегистрированы, но не посажены */
bool check_brd_alarm( int point_id )
{
    bool brd_alarm = false;
    if ( CheckStageACT(point_id, sCloseBoarding) ) {
    TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText =
        "SELECT pax_id FROM pax, pax_grp "
        " WHERE pax_grp.point_dep=:point_id AND "
        "       pax_grp.grp_id=pax.grp_id AND "
      "       pax_grp.status NOT IN ('E') AND "
        "       pax.wl_type IS NULL AND "
        "       pax.pr_brd = 0 AND "
        "       rownum < 2 ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
      brd_alarm = !Qry.Eof;
    }
    set_alarm( point_id, Alarm::Brd, brd_alarm );
    return brd_alarm;
};

/* есть ошибочные телеграммы, не скорректированные впоследствии */
bool check_tlg_in_alarm(int point_id_tlg, int point_id_spp) // point_id_spp м.б. NoExists
{
  bool result = false;
  if (point_id_spp!=NoExists)
  {
    const char* sql=
      "SELECT tlg_binding.point_id_tlg "
      "FROM tlg_binding, tlg_source, typeb_in_history "
      "WHERE tlg_binding.point_id_tlg=tlg_source.point_id_tlg AND "
      "      tlg_source.tlg_id=typeb_in_history.prev_tlg_id(+) AND "
      "      typeb_in_history.prev_tlg_id IS NULL AND "
      "      tlg_binding.point_id_spp=:point_id_spp AND "
      "      NVL(tlg_source.has_alarm_errors,0)<>0 AND rownum<2 ";
    QParams QryParams;
    QryParams << QParam("point_id_spp", otInteger, point_id_spp);
    TCachedQuery CachedQry(sql, QryParams);
    TQuery &Qry=CachedQry.get();
    Qry.Execute();
    result = !Qry.Eof;
    set_alarm(point_id_spp, Alarm::TlgIn, result);
  }
  else
  {
    const char* sql=
      "SELECT tlg_binding.point_id_spp, tlg_source.has_alarm_errors "
      "FROM tlg_binding, "
      "     (SELECT MAX(DECODE(NVL(tlg_source.has_alarm_errors,0), 0, 0, 1)) AS has_alarm_errors "
      "      FROM tlg_source, typeb_in_history "
      "      WHERE tlg_source.tlg_id=typeb_in_history.prev_tlg_id(+) AND "
      "            typeb_in_history.prev_tlg_id IS NULL AND "
      "            tlg_source.point_id_tlg=:point_id_tlg) tlg_source "
      "WHERE tlg_binding.point_id_tlg=:point_id_tlg";
    QParams QryParams;
    QryParams << QParam("point_id_tlg", otInteger, point_id_tlg);
    TCachedQuery CachedQry(sql, QryParams);
    TQuery &Qry=CachedQry.get();
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      result=Qry.FieldAsInteger("has_alarm_errors")!=0;
      set_alarm(Qry.FieldAsInteger("point_id_spp"), Alarm::TlgIn, result);
    };
  };
  return result;
};

/* есть ошибочные или требующие коррекции телеграммы */
bool check_tlg_out_alarm(int point_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select * from tlg_out where "
        "   point_id = :point_id and "
        "   (has_errors <> 0 or completed = 0) and "
        "   rownum < 2";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    bool result = not Qry.Eof;
    set_alarm(point_id, Alarm::TlgOut, result);
    return result;
}

/* есть пассажиры спецобслуживания */
bool check_spec_service_alarm(int point_id)
{
    TRemGrp alarm_rems;
    alarm_rems.Load(retALARM_SS, point_id);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select pax_rem.rem_code from "
        "  pax_grp, "
        "  pax,  "
        "  pax_rem "
        "where "
        "  pax_grp.point_dep = :point_id and "
        "  pax_grp.status NOT IN ('E') and "
        "  pax_grp.grp_id = pax.grp_id and "
        "  pax.refuse is null and "
        "  pax.pax_id = pax_rem.pax_id ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next())
        if(alarm_rems.exists(Qry.FieldAsString("rem_code"))) break;
    bool result = not Qry.Eof;
    set_alarm(point_id, Alarm::SpecService, result);
    return result;
}

void check_unattached_trfer_alarm( const set<int> &point_ids )
{
  for(set<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    int point_id=*i;
    bool result=false;
    if ( CheckStageACT(point_id, sCloseCheckIn) )
    {
      InboundTrfer::TUnattachedTagMap unattached_grps;
      InboundTrfer::GetUnattachedTags(point_id, unattached_grps);
      result = !unattached_grps.empty();
    };
    set_alarm( point_id, Alarm::UnattachedTrfer, result );
  };
};

void check_unattached_trfer_alarm( int point_id )
{
  set<int> point_ids;
  point_ids.insert(point_id);
  check_unattached_trfer_alarm( point_ids );
};

bool need_check_u_trfer_alarm_for_grp( int point_id )
{
  return CheckStageACT(point_id, sCloseCheckIn);
};

void check_u_trfer_alarm_for_grp( int point_id,
                                  int grp_id,
                                  const map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> &tags_before )

{
  bool alarm=get_alarm(point_id, Alarm::UnattachedTrfer);
  if (tags_before.empty() && !alarm) return;
  map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> tags_after;
  InboundTrfer::GetCheckedTags(grp_id, idGrp, tags_after);
  if (tags_after.empty() && alarm) return;
  if (tags_before==tags_after) return;
  check_unattached_trfer_alarm(point_id);
};

void check_u_trfer_alarm_for_next_trfer( int id,  //м.б. point_id или grp_id
                                         TIdType id_type )
{
  set<int> next_trfer_point_ids;
  InboundTrfer::GetNextTrferCheckedFlts(id, id_type, next_trfer_point_ids);
  check_unattached_trfer_alarm(next_trfer_point_ids);
};

bool check_conflict_trfer_alarm(int point_id)
{
  bool conflict_trfer_alarm = false;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT grp_id "
    "FROM pax_grp "
    "WHERE point_dep = :point_id AND "
    "      status NOT IN ('E') AND "
    "      trfer_conflict<>0 AND rownum<2 ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  conflict_trfer_alarm = !Qry.Eof;
  set_alarm( point_id, Alarm::ConflictTrfer, conflict_trfer_alarm );
    return conflict_trfer_alarm;
}

bool need_crew_checkin(const TAdvTripInfo &fltInfo)
{
  TTripRoute route;
  route.GetRouteAfter(fltInfo, trtNotCurrent, trtNotCancelled);
  for(const TTripRouteItem& r : route)
    if (!APIS::SettingsList()
          .getByAirps(fltInfo.airline, fltInfo.airp, r.airp)
          .filterFormatsFromList(getCrewFormats())
          .empty()) return true;

  return false;
}

void check_crew_alarms_task(const TTripTaskKey &task)
{
  check_crew_alarms(task.point_id);
};

void check_crew_alarms(int point_id)
{
  bool crew_checkin = false;
  bool crew_number = false;
  bool crew_diff = false;

  TQuery Qry(&OraSession);
  bool do_check=CheckStageACT(point_id, sOpenCheckIn);
  if (!do_check)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT point_id FROM trip_tasks "
      "WHERE point_id=:point_id AND name=:name AND "
      "      (next_exec IS NOT NULL AND next_exec<=:now_utc OR "
      "       next_exec IS NULL AND last_exec IS NOT NULL) ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("name", otString, BEFORE_TAKEOFF_70_US_CUSTOMS_ARRIVAL);
    Qry.CreateVariable("now_utc", otDate, NowUTC());
    Qry.Execute();
    if (!Qry.Eof) do_check=true;
  };

  if ( do_check )
  {
    int sopp_num=NoExists;
    int checkin_num=NoExists;
    TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText =
      "SELECT NVL(cockpit,0)+NVL(cabin,0) AS num "
      "FROM trip_crew "
      "WHERE point_id=:point_id AND (cockpit IS NOT NULL OR cabin IS NOT NULL)";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if (!Qry.Eof) sopp_num=Qry.FieldAsInteger("num");

    Qry.SQLText=
      "SELECT COUNT(*) AS num "
      "FROM pax_grp, pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.point_dep=:point_id AND "
      "      pax_grp.status IN ('E') AND "
      "      pax.refuse IS NULL";
    Qry.Execute();
    if (!Qry.Eof && Qry.FieldAsInteger("num")!=0) checkin_num=Qry.FieldAsInteger("num");

    crew_diff=sopp_num!=NoExists && checkin_num!=NoExists && sopp_num!=checkin_num;

    if (checkin_num==NoExists || checkin_num<=0 ||
        sopp_num==NoExists || sopp_num<=0)
    {
      TAdvTripInfo fltInfo;
      if (fltInfo.getByPointId(point_id))
      {
        if (checkin_num==NoExists || checkin_num<=0)
        {
          do
          {
            //экипаж не зарегистрирован
            //надо проверить mintrans, форматы APIS
            if (!GetTripSets(tsNoCrewCkinAlarm, fltInfo) &&
                GetTripSets(tsMintransFile, fltInfo))
            {
              crew_checkin=true;
              break;
            };

            if (need_crew_checkin(fltInfo))
            {
              crew_checkin=true;
              break;
            };
          }
          while(false);
        };

        if (sopp_num==NoExists || sopp_num<=0)
        {
          //кол-во экипажа неизвестно
          //надо проверить формирование LDM
          TypeB::TCreator creator(fltInfo);
          creator << "LDM";
          vector<TypeB::TCreateInfo> createInfo;
          creator.getInfo(createInfo);
          if (!createInfo.empty()) crew_number=true;
        };
      };
    };
  };
  set_alarm( point_id, Alarm::CrewCheckin, crew_checkin );
  set_alarm( point_id, Alarm::CrewNumber, crew_number );
  set_alarm( point_id, Alarm::CrewDiff, crew_diff );
}

void check_apis_alarms(int point_id)
{
  static set<Alarm::Enum> checked_alarms={Alarm::APISDiffersFromBooking,
                                          Alarm::APISIncomplete,
                                          Alarm::APISManualInput};
  check_apis_alarms(point_id, checked_alarms);
};

void check_apis_alarms(int point_id, const set<Alarm::Enum> &checked_alarms)
{
  set<APIS::TAlarmType> off_alarms;

  if (checked_alarms.find(Alarm::APISDiffersFromBooking)!=checked_alarms.end())
    off_alarms.insert(APIS::atDiffersFromBooking);
  if (checked_alarms.find(Alarm::APISIncomplete)!=checked_alarms.end())
    off_alarms.insert(APIS::atIncomplete);
  if (checked_alarms.find(Alarm::APISManualInput)!=checked_alarms.end())
    off_alarms.insert(APIS::atManualInput);

  if (off_alarms.empty()) return;

  bool apis_control=false;
  bool apis_manual_input=false;

  TRouteAPICheckInfo route_check_info(point_id);
  if (route_check_info.apis_generation())
  {
    //хотя бы по одному из направлений настроен APIS
    TTripSetList setList;
    setList.fromDB(point_id);
    apis_control=setList.value(tsAPISControl, false);
    apis_manual_input=setList.value(tsAPISManualInput, false);

    if (!apis_control) off_alarms.clear();
    if (apis_manual_input) off_alarms.erase(APIS::atManualInput);

    if (!off_alarms.empty())
    {
      bool need_crs_pax=off_alarms.find(APIS::atDiffersFromBooking)!=off_alarms.end();

      ostringstream sql;
      sql << "SELECT pax_grp.airp_arv, pax_grp.status, "
             "       pax.name, pax.crew_type, pax.doco_confirm, pax_calc_data.*, ";
      if (need_crs_pax)
        sql << "       crs_pax.pax_id AS crs_pax_id ";
      else
        sql << "       NULL AS crs_pax_id ";
      sql << "FROM pax_grp, pax, pax_calc_data ";
      if (need_crs_pax)
        sql << ", crs_pax ";
      sql << "WHERE pax_grp.grp_id = pax.grp_id AND "
             "      pax.pax_id = pax_calc_data.pax_calc_data_id(+) AND "
             "      pax_grp.point_dep = :point_id AND "
             "      pax.refuse IS NULL ";
      if (need_crs_pax)
        sql << "      AND pax.pax_id=crs_pax.pax_id(+) AND crs_pax.pr_del(+)=0 ";
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=sql.str().c_str();
      Qry.CreateVariable("point_id", otInteger, point_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        int col_name=Qry.FieldIndex("name");
        int col_airp_arv=Qry.FieldIndex("airp_arv");
        int col_status=Qry.FieldIndex("status");
        int col_crew_type=Qry.FieldIndex("crew_type");
        int col_doco_confirm=Qry.FieldIndex("doco_confirm");
        int col_crs_pax_id=Qry.FieldIndex("crs_pax_id");
        AllAPIAttrs allAPIAttrs(ASTRA::NoExists);
        for(;!Qry.Eof;Qry.Next())
        {
          if (off_alarms.empty()) break;

          string name=Qry.FieldAsString(col_name);
          string airp_arv=Qry.FieldAsString(col_airp_arv);
          TPaxStatus status=DecodePaxStatus(Qry.FieldAsString(col_status));
          ASTRA::TCrewType::Enum crew_type = CrewTypes().decode(Qry.FieldAsString(col_crew_type));
          ASTRA::TPaxTypeExt pax_ext(status, crew_type);
          bool docoConfirmed=Qry.FieldAsInteger(col_doco_confirm)!=0;
          bool crsExists=!Qry.FieldIsNULL(col_crs_pax_id);

          boost::optional<const TCompleteAPICheckInfo&> check_info=route_check_info.get(airp_arv);
          if (!check_info || check_info.get().apis_formats().empty()) continue;

          set<APIS::TAlarmType> alarms=allAPIAttrs.getAlarms(Qry, name!="CBBG", pax_ext, docoConfirmed, crsExists, check_info.get(), off_alarms);
          for(set<APIS::TAlarmType>::const_iterator a=alarms.begin(); a!=alarms.end(); ++a) off_alarms.erase(*a);
        }
      }
    }
  }

  if (checked_alarms.find(Alarm::APISDiffersFromBooking)!=checked_alarms.end())
    set_alarm( point_id,
               Alarm::APISDiffersFromBooking,
               apis_control && off_alarms.find(APIS::atDiffersFromBooking)==off_alarms.end() );
  if (checked_alarms.find(Alarm::APISIncomplete)!=checked_alarms.end())
    set_alarm( point_id,
               Alarm::APISIncomplete,
               apis_control && off_alarms.find(APIS::atIncomplete)==off_alarms.end() );
  if (checked_alarms.find(Alarm::APISManualInput)!=checked_alarms.end())
    set_alarm( point_id,
               Alarm::APISManualInput,
               apis_control && !apis_manual_input && off_alarms.find(APIS::atManualInput)==off_alarms.end() );

};

void check_unbound_emd_alarm( int point_id )
{
  bool emd_alarm = false;
  if ( CheckStageACT(point_id, sCloseCheckIn) )
    emd_alarm=PaxASVCList::ExistsUnboundBagEMD(point_id);
  set_alarm( point_id, Alarm::UnboundEMD, emd_alarm );
};

bool check_apps_alarm( int point_id )
{
  bool apps_alarm=existsAlarmByPointId(point_id,
                                       {Alarm::APPSNegativeDirective,
                                        Alarm::APPSError,
                                        Alarm::APPSConflict},
                                       {paxCheckIn, paxPnl});
  set_alarm( point_id, Alarm::APPSProblem, apps_alarm );
  return apps_alarm;
}

bool check_iapi_alarm( int point_id )
{
  bool iapi_alarm=existsAlarmByPointId(point_id,
                                       {Alarm::IAPINegativeDirective},
                                       {paxCheckIn});
  set_alarm( point_id, Alarm::IAPIProblem, iapi_alarm );
  return iapi_alarm;
}

void TTripAlarm::check()
{
  ProgTrace(TRACE5, "TTripAlarm::check: %s", traceStr().c_str());
  switch(type)
  {
    case Alarm::Brd:
      check_brd_alarm(id);
      break;
    case Alarm::UnboundEMD:
      check_unbound_emd_alarm(id);
      break;
    case Alarm::SyncCabinClass:
      add_trip_task(id, AlarmTypes().encode(type), "");
      break;
    case Alarm::APISControl:
    case Alarm::APPSProblem:
    case Alarm::IAPIProblem:
      addTaskForCheckingAlarm(id, type);
      break;
    default:
      ProgError(STDLOG, "TTripAlarm::check: alarm not supported (%s)", traceStr().c_str());
  }
}

void TGrpAlarm::check()
{
    switch(type)
    {
        case Alarm::SyncCustomAlarms:
            TCustomAlarms().getByGrpId(id).toDB();
            break;
        default:
            ProgError(STDLOG, "TGrpAlarm::check: alarm not supported (%s)", traceStr().c_str());
    }
}

void TPaxAlarm::check()
{
    switch(type)
    {
        case Alarm::SyncCustomAlarms:
            TCustomAlarms().getByPaxId(id).toDB();
            break;
        default:
            ProgError(STDLOG, "TPaxAlarm::check: alarm not supported (%s)", traceStr().c_str());
    }
}

namespace Posthooks
{
template <> void Simple<TTripAlarm>::run()
{
  p.check();
}

template <> void Simple<TGrpAlarm>::run()
{
  p.check();
}

template <> void Simple<TPaxAlarm>::run()
{
  p.check();
}
}

void TTripAlarmHook::set(Alarm::Enum _type, const int& _id)
{
  sethBefore(TSomeonesAlarmHook<TTripAlarm>(TTripAlarm(_type, _id)));
}

void TGrpAlarmHook::set(Alarm::Enum _type, const int& _id)
{
    if(_type == Alarm::SyncCustomAlarms)
        sethBefore(TSomeonesAlarmHook<TGrpAlarm>(TGrpAlarm(_type, _id)));
    else {
        TCachedQuery Qry("SELECT point_dep FROM pax_grp WHERE grp_id=:grp_id",
                QParams() << QParam("grp_id", otInteger, _id));
        Qry.get().Execute();
        if (Qry.get().Eof) return;

        sethBefore(TSomeonesAlarmHook<TTripAlarm>(TTripAlarm(_type, Qry.get().FieldAsInteger("point_dep"))));
    }
}

void TPaxAlarmHook::set(Alarm::Enum _type, const int& _id)
{
    if(_type == Alarm::SyncCustomAlarms)
        sethBefore(TSomeonesAlarmHook<TPaxAlarm>(TPaxAlarm(_type, _id)));
    else {
        TCachedQuery Qry("SELECT pax_grp.point_dep FROM pax_grp, pax WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id",
                QParams() << QParam("pax_id", otInteger, _id));
        Qry.get().Execute();
        if (Qry.get().Eof) return;

        sethBefore(TSomeonesAlarmHook<TTripAlarm>(TTripAlarm(_type, Qry.get().FieldAsInteger("point_dep"))));
    }
}

void TCrsPaxAlarmHook::set(Alarm::Enum _type, const int& _id)
{
  TCachedQuery Qry("SELECT tlg_binding.point_id_spp "
                   "FROM tlg_binding, crs_pnr, crs_pax "
                   "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
                   "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
                   "      crs_pax.pax_id=:pax_id AND "
                   "      crs_pnr.system='CRS' AND crs_pax.pr_del=0 ",
                   QParams() << QParam("pax_id", otInteger, _id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    sethBefore(TSomeonesAlarmHook<TTripAlarm>(TTripAlarm(_type, Qry.get().FieldAsInteger("point_id_spp"))));
}

static std::string getPaxAlarmTable(const PaxOrigin paxOrigin)
{
  switch (paxOrigin)
  {
    case paxCheckIn:
      return "pax_alarms";
      break;
    case paxPnl:
      return "crs_pax_alarms";
      break;
    default:
      break;
  }

  return "";
}

static bool setAlarmByPaxId(const int paxId, const Alarm::Enum alarmType, const bool alarmValue, const PaxOrigin paxOrigin)
{
  string table_name=getPaxAlarmTable(paxOrigin);
  if (table_name.empty()) return false;

  TQuery Qry(&OraSession);
  if(alarmValue)
      Qry.SQLText = "insert into " + table_name + "(pax_id, alarm_type) values(:pax_id, :alarm_type)";
  else
      Qry.SQLText = "delete from " + table_name + " where pax_id = :pax_id and alarm_type = :alarm_type";
  Qry.CreateVariable("pax_id", otInteger, paxId);
  Qry.CreateVariable("alarm_type", otString, AlarmTypes().encode(alarmType));
  try {
      Qry.Execute();
      if(Qry.RowsProcessed()>0) return true;

  } catch (EOracleError &E) {
      if(E.Code != 1 && E.Code != 2291) throw; // parent key not found, unique constraint violated
  }

  return false;
}

bool addAlarmByPaxId(const int paxId,
                     const std::initializer_list<Alarm::Enum>& alarms,
                     const std::initializer_list<PaxOrigin>& origins)
{
  bool result=false;
  for(const auto& alarmType : alarms)
    for(const auto& paxOrigin : origins)
      if (setAlarmByPaxId(paxId, alarmType, true, paxOrigin)) result=true;

  return result;
}

bool deleteAlarmByPaxId(const int paxId,
                        const std::initializer_list<Alarm::Enum>& alarms,
                        const std::initializer_list<PaxOrigin>& origins)
{
  bool result=false;
  for(const auto& alarmType : alarms)
    for(const auto& paxOrigin : origins)
      if (setAlarmByPaxId(paxId, alarmType, false, paxOrigin)) result=true;

  return result;
}

bool deleteAlarmByGrpId(const int grpId, const Alarm::Enum alarmType)
{
  TCachedQuery Qry("DELETE FROM pax_alarms WHERE alarm_type=:alarm_type AND pax_id IN (SELECT pax_id FROM pax WHERE grp_id=:grp_id)",
                   QParams() << QParam("grp_id", otInteger, grpId)
                             << QParam("alarm_type", otString, AlarmTypes().encode(alarmType)));
  Qry.get().Execute();
  return (Qry.get().RowsProcessed()>0);
}

bool existsAlarmByPaxId(const int paxId, const Alarm::Enum alarmType, const PaxOrigin paxOrigin)
{
  string table_name=getPaxAlarmTable(paxOrigin);
  if (table_name.empty()) return false;

  TCachedQuery Qry("SELECT 1 FROM " + table_name + " WHERE alarm_type=:alarm_type AND pax_id=:pax_id",
                   QParams() << QParam("pax_id", otInteger, paxId)
                             << QParam("alarm_type", otString, AlarmTypes().encode(alarmType)));
  Qry.get().Execute();
  return (!Qry.get().Eof);
}

bool existsAlarmByGrpId(const int grpId, const Alarm::Enum alarmType)
{
  TCachedQuery Qry("SELECT 1 FROM pax_alarms, pax "
                   "WHERE pax_alarms.pax_id=pax.pax_id AND pax_alarms.alarm_type=:alarm_type AND pax.grp_id=:grp_id AND rownum<2",
                   QParams() << QParam("grp_id", otInteger, grpId)
                             << QParam("alarm_type", otString, AlarmTypes().encode(alarmType)));
  Qry.get().Execute();
  return (!Qry.get().Eof);
}

static std::string getAlarmSQLEnum(const std::initializer_list<Alarm::Enum>& alarms)
{
  set<string> values;
  for(const auto& a : alarms)
    values.insert(AlarmTypes().encode(a));
  return GetSQLEnum(values);
}

bool existsAlarmByPointId(const int pointId,
                          const std::initializer_list<Alarm::Enum>& alarms,
                          const std::initializer_list<PaxOrigin>& origins)
{
  for(const auto& paxOrigin : origins)
  {
    string sql;
    switch(paxOrigin)
    {
      case paxCheckIn:
        sql = "SELECT 1 FROM pax_alarms, pax, pax_grp "
              "WHERE pax_alarms.pax_id=pax.pax_id AND "
              "      pax.grp_id=pax_grp.grp_id AND "
              "      pax_grp.point_dep=:point_id AND "
              "      rownum<2 AND "
              "      pax_alarms.alarm_type IN "+ getAlarmSQLEnum(alarms);
        break;
      case paxPnl:
        sql = "SELECT 1 FROM crs_pax_alarms, crs_pax, crs_pnr, tlg_binding "
              "WHERE crs_pax_alarms.pax_id=crs_pax.pax_id AND "
              "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
              "      crs_pnr.point_id=tlg_binding.point_id_tlg AND "
              "      tlg_binding.point_id_spp=:point_id AND "
              "      rownum<2 AND "
              "      crs_pax_alarms.alarm_type IN "+ getAlarmSQLEnum(alarms);
        break;
      default:
        continue;
    }

    TCachedQuery Qry(sql, QParams() << QParam("point_id", otInteger, pointId));

    Qry.get().Execute();
    if (!Qry.get().Eof) return true;
  }
  return false;
}

void getAlarmByPointId(const int pointId, const Alarm::Enum alarmType, std::set<int>& paxIds)
{
  paxIds.clear();

  TCachedQuery Qry("SELECT pax_alarms.pax_id "
                   "FROM pax_alarms, pax, pax_grp "
                   "WHERE pax_alarms.pax_id=pax.pax_id AND "
                   "      pax.grp_id=pax_grp.grp_id AND "
                   "      pax_grp.point_dep=:point_id AND "
                   "      pax_alarms.alarm_type=:alarm_type",
                   QParams() << QParam("point_id", otInteger, pointId)
                             << QParam("alarm_type", otString, AlarmTypes().encode(alarmType)));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    paxIds.insert(Qry.get().FieldAsInteger("pax_id"));
}
