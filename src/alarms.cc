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
#include "brd.h"
#include "emdoc.h"
#include "sopp.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

const char *TripAlarmsTypeS[] = {
    "SALON",
    "WAITLIST",
    "BRD",
    "OVERLOAD",
    "ET_STATUS",
    "DIFF_COMPS",
    "SPEC_SERVICE",
    "TLG_IN",
    "TLG_OUT",
    "UNATTACHED_TRFER",
    "CONFLICT_TRFER",
    "CREW_CHECKIN",
    "CREW_NUMBER",
    "CREW_DIFF",
    "APIS_DIFFERS_FROM_BOOKING",
    "APIS_INCOMPLETE",
    "APIS_MANUAL_INPUT",
    "UNBOUND_EMD",
    "APPS_PROBLEM",
    "APPS_OUTAGE",
    "APPS_CONFLICT",
    "APPS_NEGATIVE_DIRECTIVE",
    "APPS_ERROR"
};

TTripAlarmsType DecodeAlarmType( const string &alarm )
{
  int i;
  for( i=0; i<(int)atLength; i++ )
    if ( alarm == TripAlarmsTypeS[ i ] )
      break;
  if ( i == atLength )
      throw Exception("DecodeAlarmType: unknown alarm type %s", alarm.c_str());
  else
    return (TTripAlarmsType)i;
}

string EncodeAlarmType(const TTripAlarmsType alarm )
{
    if(alarm < 0 or alarm >= atLength)
        throw Exception("EncodeAlarmType: wrong alarm type %d", alarm);
    return TripAlarmsTypeS[ alarm ];
}

void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms )
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
         !setList.value(tsFreeSeating) ) {
        Alarms.setFlag( atSalon );
    }
    if ( Qry.FieldAsInteger( "pr_etstatus" ) < 0 ) {
        Alarms.setFlag( atETStatus );
    }

    Qry.Clear();
    Qry.SQLText = "select alarm_type from trip_alarms where point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) Alarms.setFlag(DecodeAlarmType(Qry.FieldAsString("alarm_type")));
}

static void PassengerAlarms( const int pax_id, BitSet<TTripAlarmsType> &Alarms, const bool is_crs )
{
    string table_name = is_crs?"crs_pax_alarms":"pax_alarms";
    Alarms.clearFlags();
    TQuery Qry(&OraSession);
    Qry.SQLText = "select alarm_type from " + table_name + " where pax_id = :pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) Alarms.setFlag(DecodeAlarmType(Qry.FieldAsString("alarm_type")));
}

void PaxAlarms( int pax_id, BitSet<TTripAlarmsType> &Alarms )
{
    PassengerAlarms( pax_id, Alarms, false );
}

void CrsPaxAlarms( int pax_id, BitSet<TTripAlarmsType> &Alarms )
{
    PassengerAlarms( pax_id, Alarms, true );
}

string TripAlarmName( TTripAlarmsType alarm )
{
    return ElemIdToElem(etAlarmType, EncodeAlarmType(alarm), efmtNameLong, AstraLocale::LANG_RU);
}

string TripAlarmString( TTripAlarmsType alarm )
{
    return ElemIdToNameLong(etAlarmType, EncodeAlarmType(alarm));
}

bool get_alarm( int point_id, TTripAlarmsType alarm_type )
{
    switch(alarm_type)
    {
        case atWaitlist:
        case atBrd:
        case atOverload:
        case atDiffComps:
        case atSpecService:
        case atTlgIn:
        case atTlgOut:
        case atUnattachedTrfer:
        case atConflictTrfer:
        case atCrewCheckin:
        case atCrewNumber:
        case atCrewDiff:
        case atAPISDiffersFromBooking:
        case atAPISIncomplete:
        case atAPISManualInput:
        case atUnboundEMD:
        case atAPPSProblem:
        case atAPPSOutage:
            break;
        default: throw Exception("get_alarm: alarm_type=%s not processed", EncodeAlarmType(alarm_type).c_str());
    };
    TQuery Qry(&OraSession);
    Qry.SQLText = "select * from trip_alarms where point_id = :point_id and alarm_type = :alarm_type";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("alarm_type", otString, EncodeAlarmType(alarm_type));
    Qry.Execute();
    return !Qry.Eof;
}

static bool get_passenger_alarm( const int pax_id, const TTripAlarmsType alarm_type, const bool is_crs )
{
    switch(alarm_type)
    {
        case atAPPSConflict:
        case atAPPSNegativeDirective:
        case atAPPSError:
        break;
        default: throw Exception("get_passenger_alarm: alarm_type=%s not processed", EncodeAlarmType(alarm_type).c_str());
    };

    string table_name = is_crs?"crs_pax_alarms":"pax_alarms";
    TQuery Qry(&OraSession);
    Qry.SQLText = "select alarm_type from " + table_name + " where pax_id = :pax_id and alarm_type = :alarm_type";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("alarm_type", otString, EncodeAlarmType(alarm_type));
    Qry.Execute();
    return !Qry.Eof;
}

bool get_pax_alarm( int pax_id, TTripAlarmsType alarm_type )
{
    return get_passenger_alarm( pax_id, alarm_type, false );
}

bool get_crs_pax_alarm( int pax_id, TTripAlarmsType alarm_type )
{
    return get_passenger_alarm( pax_id, alarm_type, true );
}

void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value )
{
    switch(alarm_type)
    {
        case atWaitlist:
        case atBrd:
        case atOverload:
        case atDiffComps:
        case atSpecService:
        case atTlgIn:
        case atTlgOut:
        case atUnattachedTrfer:
        case atConflictTrfer:
        case atCrewCheckin:
        case atCrewNumber:
        case atCrewDiff:
        case atAPISDiffersFromBooking:
        case atAPISIncomplete:
        case atAPISManualInput:
        case atUnboundEMD:
        case atAPPSProblem:
        case atAPPSOutage:
            break;
        default: throw Exception("set_alarm: alarm_type=%s not processed", EncodeAlarmType(alarm_type).c_str());
    };

    TQuery Qry(&OraSession);
    if(alarm_value)
        Qry.SQLText = "insert into trip_alarms(point_id, alarm_type) values(:point_id, :alarm_type)";
    else
        Qry.SQLText = "delete from trip_alarms where point_id = :point_id and alarm_type = :alarm_type";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("alarm_type", otString, EncodeAlarmType(alarm_type));
    try {
        Qry.Execute();
        if(Qry.RowsProcessed()) {
            TReqInfo::Instance()->LocaleToLog(alarm_value?"EVT.ALARM_SET":"EVT.ALARM_DELETED", LEvntPrms()
                                              << PrmElem<string>("alarm_type", etAlarmType, EncodeAlarmType(alarm_type), efmtNameLong),
                                              evtFlt, point_id );
        }
    } catch (EOracleError &E) {
        if(E.Code != 1) throw;
    }
}

static void process_pax_alarm( const int pax_id, const TTripAlarmsType alarm_type, const bool alarm_value, const bool crs_pax )
{
  TQuery Qry(&OraSession);
  if (crs_pax)
    Qry.SQLText = "SELECT point_id_spp AS point_dep "
                  "FROM crs_pax, crs_pnr, tlg_binding "
                  "WHERE crs_pax.pnr_id = crs_pnr.pnr_id AND "
                  "      crs_pnr.point_id = tlg_binding.point_id_tlg AND "
                  "      pax_id = :pax_id";
  else
    Qry.SQLText = "SELECT point_dep FROM pax, pax_grp "
                  "WHERE pax.grp_id = pax_grp.grp_id AND pax_id = :pax_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();

  if(Qry.Eof)
      throw Exception("process_pax_alarm: passenger info not found");

  synch_trip_alarm(Qry.FieldAsInteger("point_dep"), alarm_type);
}

static void set_passenger_alarm( const int pax_id, const TTripAlarmsType alarm_type, const bool alarm_value, const bool is_crs )
{
    switch(alarm_type)
    {
        case atAPPSConflict:
        case atAPPSNegativeDirective:
        case atAPPSError:
            break;
        default: throw Exception("set_passenger_alarm: alarm_type=%s not processed", EncodeAlarmType(alarm_type).c_str());
    };

    string table_name = is_crs?"crs_pax_alarms":"pax_alarms";
    TQuery Qry(&OraSession);
    if(alarm_value)
        Qry.SQLText = "insert into " + table_name + "(pax_id, alarm_type) values(:pax_id, :alarm_type)";
    else
        Qry.SQLText = "delete from " + table_name + " where pax_id = :pax_id and alarm_type = :alarm_type";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("alarm_type", otString, EncodeAlarmType(alarm_type));
    try {
        Qry.Execute();

        if(Qry.RowsProcessed()) {
            process_pax_alarm(pax_id, alarm_type, alarm_value, is_crs);
        }
    } catch (EOracleError &E) {
        if(E.Code != 1 && E.Code != 2291) throw; // parent key not found, unique constraint violated
    }
}

void set_pax_alarm( int pax_id, TTripAlarmsType alarm_type, bool alarm_value )
{
    set_passenger_alarm( pax_id, alarm_type, alarm_value, false );
}

void set_crs_pax_alarm( int pax_id, TTripAlarmsType alarm_type, bool alarm_value )
{
    set_passenger_alarm( pax_id, alarm_type, alarm_value, true );
}

void synch_trip_alarm(int point_id, TTripAlarmsType alarm_type) {
  switch(alarm_type)
  {
      case atAPPSConflict:
      case atAPPSNegativeDirective:
      case atAPPSError:
          check_apps_alarm(point_id);
          break;
      default: throw Exception("synch_trip_alarm: alarm_type=%s not processed", EncodeAlarmType(alarm_type).c_str());
  };
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
  set_alarm( point_id, atOverload, overload_alarm );
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
    "      salons.is_waitlist(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,rownum)<>0 AND "
    "      rownum<2";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return !Qry.Eof;
};

/* ���� ���ᠦ���, ����� �� ���� �������� */
bool check_waitlist_alarm( int point_id )
{
    bool waitlist_alarm = calc_waitlist_alarm( point_id );

  set_alarm( point_id, atWaitlist, waitlist_alarm );
    return waitlist_alarm;
};

/* ���� ���ᠦ���, ����� ��ॣ����஢���, �� �� ��ᠦ��� */
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
    set_alarm( point_id, atBrd, brd_alarm );
    return brd_alarm;
};

/* ���� �訡��� ⥫��ࠬ��, �� ᪮�४�஢���� ���᫥��⢨� */
bool check_tlg_in_alarm(int point_id_tlg, int point_id_spp) // point_id_spp �.�. NoExists
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
    set_alarm(point_id_spp, atTlgIn, result);
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
      set_alarm(Qry.FieldAsInteger("point_id_spp"), atTlgIn, result);
    };
  };
  return result;
};

/* ���� �訡��� ��� �ॡ��騥 ���४樨 ⥫��ࠬ�� */
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
    set_alarm(point_id, atTlgOut, result);
    return result;
}

/* ���� ���ᠦ��� ᯥ殡�㦨����� */
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
    set_alarm(point_id, atSpecService, result);
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
    set_alarm( point_id, atUnattachedTrfer, result );
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
  bool alarm=get_alarm(point_id, atUnattachedTrfer);
  if (tags_before.empty() && !alarm) return;
  map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> tags_after;
  InboundTrfer::GetCheckedTags(grp_id, idGrp, tags_after);
  if (tags_after.empty() && alarm) return;
  if (tags_before==tags_after) return;
  check_unattached_trfer_alarm(point_id);
};

void check_u_trfer_alarm_for_next_trfer( int id,  //�.�. point_id ��� grp_id
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
  set_alarm( point_id, atConflictTrfer, conflict_trfer_alarm );
    return conflict_trfer_alarm;
}

bool need_crew_checkin(const TAdvTripInfo &fltInfo)
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  TBaseTable &basecities = base_tables.get( "cities" );

  string country_dep = ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", fltInfo.airp )).city)).country;

  TTripRoute route;
  route.GetRouteAfter(NoExists,
                      fltInfo.point_id,
                      fltInfo.point_num,
                      fltInfo.first_point,
                      fltInfo.pr_tranzit,
                      trtNotCurrent, trtNotCancelled);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT format FROM apis_sets "
    "WHERE airline=:airline AND "
    "      country_dep=:country_dep AND country_arv=:country_arv AND "
    "      format IN ('EDI_CN', 'EDI_IN', 'EDI_US', 'EDI_USBACK', 'EDI_UK', 'EDI_ES', 'EDI_KR', 'EDI_AZ') AND pr_denial=0 AND rownum<2";
  Qry.CreateVariable("airline", otString, fltInfo.airline);
  Qry.CreateVariable("country_dep", otString, country_dep);
  Qry.DeclareVariable("country_arv", otString);

  TTripRoute::const_iterator r=route.begin();
  for(; r!=route.end(); ++r)
  {
    string country_arv = ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", r->airp )).city)).country;
    Qry.SetVariable("country_arv", country_arv);
    Qry.Execute();
    if (!Qry.Eof) break;
  };
  return (r!=route.end());
};

void check_crew_alarms_task(int point_id, const string& task_name, const string &params)
{
  check_crew_alarms(point_id);
};

void check_crew_alarms(int point_id)
{
  bool crew_checkin = false;
  bool crew_number = false;
  bool crew_diff = false;

  TQuery Qry(&OraSession);
  bool do_check=CheckStageACT(point_id, sCloseCheckIn);
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
    Qry.CreateVariable("now_utc", otDate, BASIC::NowUTC());
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
      Qry.SQLText=
        "SELECT airline, flt_no, suffix, airp, scd_out, "
        "       point_id, point_num, first_point, pr_tranzit "
        "FROM points "
        "WHERE point_id=:point_id ";
      Qry.Execute();
      if (!Qry.Eof)
      {
        TAdvTripInfo fltInfo(Qry);

        if (checkin_num==NoExists || checkin_num<=0)
        {
          do
          {
            //���� �� ��ॣ����஢��
            //���� �஢���� mintrans, �ଠ�� APIS
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
          //���-�� ����� �������⭮
          //���� �஢���� �ନ஢���� LDM
          TypeB::TCreator creator(fltInfo);
          creator << "LDM";
          vector<TypeB::TCreateInfo> createInfo;
          creator.getInfo(createInfo);
          if (!createInfo.empty()) crew_number=true;
        };
      };
    };
    };
  set_alarm( point_id, atCrewCheckin, crew_checkin );
  set_alarm( point_id, atCrewNumber, crew_number );
    set_alarm( point_id, atCrewDiff, crew_diff );
}

void check_apis_alarms(int point_id)
{
  set<TTripAlarmsType> checked_alarms;
  checked_alarms.insert(atAPISDiffersFromBooking);
  checked_alarms.insert(atAPISIncomplete);
  checked_alarms.insert(atAPISManualInput);
  check_apis_alarms(point_id, checked_alarms);
};

void check_apis_alarms(int point_id, const set<TTripAlarmsType> &checked_alarms)
{
  set<APIS::TAlarmType> off_alarms;

  if (checked_alarms.find(atAPISDiffersFromBooking)!=checked_alarms.end())
    off_alarms.insert(APIS::atDiffersFromBooking);
  if (checked_alarms.find(atAPISIncomplete)!=checked_alarms.end())
    off_alarms.insert(APIS::atIncomplete);
  if (checked_alarms.find(atAPISManualInput)!=checked_alarms.end())
    off_alarms.insert(APIS::atManualInput);

  if (off_alarms.empty()) return;

  bool apis_control=false;
  bool apis_manual_input=false;

  TRouteAPICheckInfo route_check_info(point_id);
  if (route_check_info.apis_generation())
  {
    //��� �� �� ������ �� ���ࠢ����� ����஥� APIS
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
      sql << "SELECT pax.pax_id, pax.name, pax_grp.airp_arv, pax_grp.status, ";
      if (need_crs_pax)
        sql << "       crs_pax.pax_id AS crs_pax_id ";
      else
        sql << "       NULL AS crs_pax_id ";
      sql << "FROM pax_grp, pax ";
      if (need_crs_pax)
        sql << ", crs_pax ";
      sql << "WHERE pax_grp.grp_id = pax.grp_id AND "
             "      pax_grp.point_dep = :point_id AND "
             "      pax.refuse IS NULL ";
      if (need_crs_pax)
        sql << "      AND pax.pax_id=crs_pax.pax_id(+) AND crs_pax.pr_del(+)=0 ";
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=sql.str().c_str();
      Qry.CreateVariable("point_id", otInteger, point_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (off_alarms.empty()) break;

        int pax_id=Qry.FieldAsInteger("pax_id");
        int crs_pax_id=Qry.FieldIsNULL("crs_pax_id")?NoExists:Qry.FieldAsInteger("crs_pax_id");
        string name=Qry.FieldAsString("name");
        string airp_arv=Qry.FieldAsString("airp_arv");
        TPaxStatus status=DecodePaxStatus(Qry.FieldAsString("status"));

        boost::optional<const TCompleteAPICheckInfo&> check_info=route_check_info.get(airp_arv);
        if (!check_info || check_info.get().apis_formats().empty()) continue;

        CheckIn::TAPISItem apis;
        if (off_alarms.find(APIS::atDiffersFromBooking)!=off_alarms.end() ||
            off_alarms.find(APIS::atIncomplete)!=off_alarms.end())
          //��㧨� �� �����
          apis.fromDB(pax_id);
        else
          //��㧨� ⮫쪮 ���㬥��
          CheckIn::LoadPaxDoc(pax_id, apis.doc);

        set<APIS::TAlarmType> alarms;
        GetAPISAlarms(status, name!="CBBG", crs_pax_id, check_info.get(), apis, off_alarms, alarms);
        for(set<APIS::TAlarmType>::const_iterator a=alarms.begin(); a!=alarms.end(); ++a) off_alarms.erase(*a);
      };
    };
  };

  if (checked_alarms.find(atAPISDiffersFromBooking)!=checked_alarms.end())
    set_alarm( point_id,
               atAPISDiffersFromBooking,
               apis_control && off_alarms.find(APIS::atDiffersFromBooking)==off_alarms.end() );
  if (checked_alarms.find(atAPISIncomplete)!=checked_alarms.end())
    set_alarm( point_id,
               atAPISIncomplete,
               apis_control && off_alarms.find(APIS::atIncomplete)==off_alarms.end() );
  if (checked_alarms.find(atAPISManualInput)!=checked_alarms.end())
    set_alarm( point_id,
               atAPISManualInput,
               apis_control && !apis_manual_input && off_alarms.find(APIS::atManualInput)==off_alarms.end() );

};

void check_unbound_emd_alarm( int point_id )
{
  bool emd_alarm = false;
    if ( CheckStageACT(point_id, sCloseCheckIn) )
    emd_alarm=PaxASVCList::ExistsUnboundEMD(point_id);
    set_alarm( point_id, atUnboundEMD, emd_alarm );
};

void check_unbound_emd_alarm( set<int> &pax_ids )
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.point_dep FROM pax_grp, pax WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";
  Qry.DeclareVariable("pax_id", otInteger);
  set<int> point_ids;
  for(set<int>::const_iterator i=pax_ids.begin(); i!=pax_ids.end(); ++i)
  {
    Qry.SetVariable("pax_id", *i);
    Qry.Execute();
    if (!Qry.Eof) point_ids.insert(Qry.FieldAsInteger("point_dep"));
  };

  for(set<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
    check_unbound_emd_alarm(*i);
};

bool check_apps_alarm( int point_id )
{
  bool apps_alarm=calc_apps_alarm( point_id );
  set_alarm( point_id, atAPPSProblem, apps_alarm );
  return apps_alarm;
}

bool calc_apps_alarm( int point_id )
{
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT alarm_type FROM pax_alarms, pax, pax_grp "
    "WHERE pax_grp.point_dep = :point_id AND pax_alarms.pax_id = pax.pax_id "
    "       AND pax.grp_id = pax_grp.grp_id AND ( alarm_type = :apps_error OR "
    "       alarm_type = :apps_conflict OR alarm_type = :apps_negative )";
  PaxQry.CreateVariable("point_id", otInteger, point_id);
  PaxQry.CreateVariable("apps_error", otString, EncodeAlarmType(atAPPSError));
  PaxQry.CreateVariable("apps_conflict", otString, EncodeAlarmType(atAPPSConflict));
  PaxQry.CreateVariable("apps_negative", otString, EncodeAlarmType(atAPPSNegativeDirective));

  PaxQry.Execute();

  TQuery CrsPaxQry(&OraSession);
  CrsPaxQry.Clear();
  CrsPaxQry.SQLText=
    "SELECT alarm_type FROM crs_pax_alarms, crs_pax, crs_pnr, tlg_binding "
    "WHERE tlg_binding.point_id_spp = :point_id AND "
    "      tlg_binding.point_id_tlg = crs_pnr.point_id AND "
    "      crs_pnr.pnr_id = crs_pax.pnr_id AND crs_pax_alarms.pax_id = crs_pax.pax_id AND "
    "      ( alarm_type = :apps_error OR alarm_type = :apps_conflict OR "
    "      alarm_type = :apps_negative )";
  CrsPaxQry.CreateVariable("point_id", otInteger, point_id);
  CrsPaxQry.CreateVariable("apps_error", otString, EncodeAlarmType(atAPPSError));
  CrsPaxQry.CreateVariable("apps_conflict", otString, EncodeAlarmType(atAPPSConflict));
  CrsPaxQry.CreateVariable("apps_negative", otString, EncodeAlarmType(atAPPSNegativeDirective));

  CrsPaxQry.Execute();

  return (!PaxQry.Eof || !CrsPaxQry.Eof);
}
