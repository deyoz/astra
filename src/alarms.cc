#include "alarms.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms )
{
	Alarms.clearFlags();
	TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT overload_alarm,brd_alarm,waitlist_alarm,pr_etstatus,pr_salon,act,pr_airp_seance,diffcomp_alarm "
    " FROM trip_sets, trip_stages, "
    " ( SELECT COUNT(*) pr_salon FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2 ) a "
    " WHERE trip_sets.point_id=:point_id AND "
    "       trip_stages.point_id(+)=trip_sets.point_id AND "
    "       trip_stages.stage_id(+)=:OpenCheckIn ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "OpenCheckIn", otInteger, sOpenCheckIn );
  Qry.Execute();
	if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  if ( Qry.FieldAsInteger( "overload_alarm" ) ) {
   	Alarms.setFlag( atOverload );
  }
  if ( Qry.FieldAsInteger( "waitlist_alarm" ) ) {
   	Alarms.setFlag( atWaitlist );
  }
  if ( Qry.FieldAsInteger( "brd_alarm" ) ) {
   	Alarms.setFlag( atBrd );
  }
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
  if ( Qry.FieldAsInteger( "diffcomp_alarm" ) ) {
   	Alarms.setFlag( atDiffComps );
  }
}

string TripAlarmName( TTripAlarmsType alarm )
{
  switch( alarm )
  {
    case atOverload:  return "Перегрузка";
		case atWaitlist:  return "Лист ожидания";
		case atBrd:       return "Посадка";
		case atSalon:     return "Не назначен салон";
		case atETStatus:  return "Нет связи с СЭБ";
		case atSeance:    return "Не определен сеанс";
    case atDiffComps: return "Различие компоновок";
		default:          return "";
  };
}

string TripAlarmString( TTripAlarmsType alarm )
{
  return AstraLocale::getLocaleText( TripAlarmName( alarm ) );
}

bool get_alarm( int point_id, TTripAlarmsType alarm_type )
{
  string alarm_column;
  switch(alarm_type)
  {
    case atWaitlist:
      alarm_column="waitlist_alarm";
      break;
    case atBrd:
      alarm_column="brd_alarm";
      break;
    case atOverload:
      alarm_column="overload_alarm";
      break;
    case atDiffComps:
      alarm_column="diffcomp_alarm";
      break;
    default: throw Exception("get_alarm: alarm_type=%d not processed", (int)alarm_type);
  };
  TQuery Qry(&OraSession);
  ostringstream msg;
  msg << "SELECT " << alarm_column << " FROM trip_sets WHERE point_id=:point_id";
  Qry.SQLText = msg.str().c_str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	return (!Qry.Eof && Qry.FieldAsInteger(alarm_column)!=0);
}

void set_alarm( int point_id, TTripAlarmsType alarm_type, bool alarm_value )
{
  string alarm_column;
  switch(alarm_type)
  {
    case atWaitlist:
      alarm_column="waitlist_alarm";
      break;
    case atBrd:
      alarm_column="brd_alarm";
      break;
    case atOverload:
      alarm_column="overload_alarm";
      break;
    case atDiffComps:
      alarm_column="diffcomp_alarm";
      break;
    default: throw Exception("set_alarm: alarm_type=%d not processed", (int)alarm_type);
  };

  TQuery Qry(&OraSession);
  ostringstream msg;
  msg << "UPDATE trip_sets SET " << alarm_column << "=:alarm "
      << "WHERE point_id=:point_id AND " << alarm_column << "<>:alarm ";
  Qry.SQLText = msg.str().c_str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "alarm", otInteger, (int)alarm_value );
	Qry.Execute();
	if (Qry.RowsProcessed()>0)
	{
	  msg.str("");
	  msg << "Тревога '" << TripAlarmName(alarm_type) << "' "
	      << (alarm_value?"установлена":"отменена");
	  TReqInfo::Instance()->MsgToLog( msg.str(), evtFlt, point_id );
  }
}

bool calc_overload_alarm( int point_id, const TTripInfo &fltInfo )
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
  	int load=GetFltLoad(point_id,fltInfo);
    ProgTrace(TRACE5,"check_overload_alarm: max_commerce=%d load=%d",Qry.FieldAsInteger("max_commerce"),load);
    overload_alarm=(load>Qry.FieldAsInteger("max_commerce"));
  };
  return overload_alarm;
};

bool check_overload_alarm( int point_id, const TTripInfo &fltInfo )
{
  bool overload_alarm=calc_overload_alarm( point_id, fltInfo );
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
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT act FROM trip_stages WHERE point_id=:point_id AND stage_id=:CloseBoarding AND act IS NOT NULL";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "CloseBoarding", otInteger, sCloseBoarding );
	Qry.Execute();
	bool brd_alarm = false;
	if ( !Qry.Eof ) {
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


