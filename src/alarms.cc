#include "alarms.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"
#include "pers_weights.h"
#include "astra_elems.h"
#include "remarks.h"
#include "transfer.h"

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
    "SEANCE",
    "DIFF_COMPS",
    "SPEC_SERVICE",
    "TLG_OUT",
    "UNATTACHED_TRFER",
    "CONFLICT_TRFER"
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
        "SELECT pr_etstatus,pr_salon,act,pr_airp_seance "
        " FROM trip_sets, trip_stages, "
        " ( SELECT COUNT(*) pr_salon FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2 ) a "
        " WHERE trip_sets.point_id=:point_id AND "
        "       trip_stages.point_id(+)=trip_sets.point_id AND "
        "       trip_stages.stage_id(+)=:OpenCheckIn ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "OpenCheckIn", otInteger, sOpenCheckIn );
    Qry.Execute();
    if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
    if ( !Qry.FieldAsInteger( "pr_salon" ) && !Qry.FieldIsNULL( "act" ) ) {
        Alarms.setFlag( atSalon );
    }
    if ( Qry.FieldAsInteger( "pr_etstatus" ) < 0 ) {
        Alarms.setFlag( atETStatus );
    }
    if (USE_SEANCES())
    {
        if ( Qry.FieldIsNULL( "pr_airp_seance" ) ) {
            Alarms.setFlag( atSeance );
        }
    };
    Qry.Clear();
    Qry.SQLText = "select alarm_type from trip_alarms where point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) Alarms.setFlag(DecodeAlarmType(Qry.FieldAsString("alarm_type")));
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
        case atTlgOut:
        case atUnattachedTrfer:
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

void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value )
{
    switch(alarm_type)
    {
        case atWaitlist:
        case atBrd:
        case atOverload:
        case atDiffComps:
        case atSpecService:
        case atTlgOut:
        case atUnattachedTrfer:
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
            ostringstream msg;
            msg << "Тревога '" << TripAlarmName(alarm_type) << "' "
                << (alarm_value?"установлена":"отменена");
            TReqInfo::Instance()->MsgToLog( msg.str(), evtFlt, point_id );
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
  set_alarm( point_id, atOverload, overload_alarm );
	return overload_alarm;
};

bool calc_waitlist_alarm( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pax.pax_id "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax.pr_brd IS NOT NULL AND pax.seats > 0 AND "
    "      salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'list',rownum) IS NULL AND "
    "      rownum<2";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return !Qry.Eof;
};

/* есть пассажиры, которые на листе ожидания */
bool check_waitlist_alarm( int point_id )
{
	bool waitlist_alarm = calc_waitlist_alarm( point_id );

  set_alarm( point_id, atWaitlist, waitlist_alarm );
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
    set_alarm(point_id, atTlgOut, result);
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

bool check_unattached_trfer_alarm( int point_id )
{
  bool result = false;
	if ( CheckStageACT(point_id, sCloseBoarding) )
  {
    InboundTrfer::TUnattachedTagMap unattached_grps;
    InboundTrfer::GetUnattachedTags(point_id, unattached_grps);
    result = !unattached_grps.empty();
	};
	set_alarm( point_id, atUnattachedTrfer, result );
	return result;
};

bool need_check_u_trfer_alarm_for_grp( int point_id )
{
  return CheckStageACT(point_id, sCloseCheckIn);
};

bool check_u_trfer_alarm_for_grp( int point_id,
                                  int grp_id,
                                  const map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> &tags_before )

{
  bool alarm=get_alarm(point_id, atUnattachedTrfer);
  if (tags_before.empty() && !alarm) return alarm;
  map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> tags_after;
  InboundTrfer::GetCheckedTags(grp_id, false, tags_after);
  if (tags_after.empty() && alarm) return alarm;
  if (tags_before==tags_after) return alarm;
  return check_unattached_trfer_alarm(point_id);
};





