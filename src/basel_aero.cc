#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"
#include "misc.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "cache.h"
#include "passenger.h"
#include "baggage.h"
#include "events.h"
#include "points.h"
#include "stl_utils.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;

const std::string BASEL_AERO = "BASEL_AERO";

TBaselAeroAirps *TBaselAeroAirps::Instance()
{
  static TBaselAeroAirps *instance_ = 0;
  if ( !instance_ )
    instance_ = new TBaselAeroAirps();
  return instance_;
}

TBaselAeroAirps::TBaselAeroAirps()
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airp, dir FROM file_sets WHERE code=:code AND pr_denial=0";
  Qry.CreateVariable( "code", otString, BASEL_AERO );
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    insert( make_pair( Qry.FieldAsString( "airp" ), Qry.FieldAsString( "dir" ) ) );
  }
}


bool is_sync_basel_pax( const TTripInfo &tripInfo )
{
  TBaselAeroAirps *airps = TBaselAeroAirps::Instance();
  for ( map<string,string>::iterator i=airps->begin(); i!=airps->end(); i++ ) {
    if ( i->first == tripInfo.airp )
      return true;
  };
  return false;
}

struct TBaselStat {
    TDateTime viewDate;
    string viewFlight;
    string viewName;
    int viewGroup;
    int viewPCT;
    int viewWeight;
    int viewCarryon;
    int viewPayWeight;
    string viewTag;
    string viewUncheckin;
    string viewStatus;
    int viewCheckinNo;
    string viewStation;
    string viewClientType;
    TDateTime viewCheckinTime;
    TDateTime viewChekinDuration;
  TDateTime viewBoardingTime;
    TDateTime viewDeparturePlanTime;
    TDateTime viewDepartureRealTime;
    string viewBagNorms;
    string viewPCTWeightPaidByType;
    string viewClass;
    int point_id;
    string airp;
    int pax_id;
};


void get_basel_aero_flight_stat( TDateTime part_key, int point_id, std::vector<TBaselStat> &stats );
void write_basel_aero_stat( TDateTime time_create, const std::vector<TBaselStat> &stats );
void read_basel_aero_stat( const string &airp, ofstream &f );

void sych_basel_aero_stat( TDateTime utcdate )
{
  TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT points.point_id, NVL(act_out,NVL(est_out,scd_out)) real_time FROM points, trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND points.time_out<=:vdate AND "
    "      pr_del=0 AND airp=:airp AND trip_sets.pr_basel_stat=0 AND pr_reg<>0";
  TDateTime trunc_date;
  modf( utcdate, &trunc_date );
  ProgTrace( TRACE5,"sych_basel_aero_stat: utcdate=%s, trunc_date=%s",
             DateTimeToStr( utcdate, "dd.mm.yyyy hh:nn" ).c_str(),
             DateTimeToStr(  trunc_date - 2, "dd.mm.yyyy hh:nn" ).c_str() );
  Qry.CreateVariable( "vdate", otDate, trunc_date - 2 );
  Qry.DeclareVariable( "airp", otString );
  TQuery TripSetsQry(&OraSession);
  TripSetsQry.SQLText =
    "UPDATE trip_sets SET pr_basel_stat=1 WHERE point_id=:point_id";
  TripSetsQry.DeclareVariable( "point_id", otInteger );
  TQuery ReWriteQry(&OraSession);
  ReWriteQry.SQLText =
    "SELECT last_create FROM file_sets WHERE airp=:airp AND code=:code";
  ReWriteQry.DeclareVariable( "airp", otString );
  ReWriteQry.CreateVariable( "code", otString, BASEL_AERO );
  TQuery DeleteQry(&OraSession);
  DeleteQry.SQLText =
    "DELETE basel_stat WHERE airp=:airp AND rownum <= 10000";
  DeleteQry.DeclareVariable( "airp", otString );
  TQuery FileSetsQry(&OraSession);
  FileSetsQry.SQLText =
    "UPDATE file_sets SET last_create=:vdate WHERE code=:code AND airp=:airp";
  FileSetsQry.CreateVariable( "vdate", otDate, utcdate );
  FileSetsQry.CreateVariable( "code", otString, BASEL_AERO );
  FileSetsQry.DeclareVariable( "airp", otString );

  std::vector<TBaselStat> stats;
  TBaselAeroAirps *airps = TBaselAeroAirps::Instance();
  for ( map<string,string>::iterator iairp=airps->begin(); iairp!=airps->end(); iairp++ ) {
    ReWriteQry.SetVariable( "airp", iairp->first );
    ReWriteQry.Execute();
    bool clear_before_write = true;
    int last_year, last_month, last_day;
    int curr_year, curr_month, curr_day;
    if ( !ReWriteQry.FieldIsNULL( "last_create" ) ) { // проверка на то, что последние данные были собраны в пред. месяцах
      DecodeDate( ReWriteQry.FieldAsDateTime( "last_create" ), last_year, last_month, last_day );
      DecodeDate( utcdate, curr_year, curr_month, curr_day );
      clear_before_write = ( curr_month != last_month || curr_year != last_year );
    }
    if ( clear_before_write ) { // надо очистить таблицу по аэропорту
       ProgTrace( TRACE5, "airp=%s, clear_before_write=%d, curr_month=%d, last_month=%d, curr_year=%d, last_year=%d",
                          iairp->first.c_str(), clear_before_write, curr_month, last_month, curr_year, last_year );
       DeleteQry.SetVariable( "airp", iairp->first );
       DeleteQry.Execute();
       while ( DeleteQry.RowsProcessed() ) {
         DeleteQry.Execute();
         OraSession.Commit();
       }
    }
    Qry.SetVariable( "airp", iairp->first );
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      if ( Qry.FieldAsDateTime( "real_time" ) > utcdate - 2 ) {
        continue;
      }
      stats.clear();

          TFlights  flights;
          flights.Get( Qry.FieldAsInteger( "point_id" ), ftTranzit );
          flights.Lock(__FUNCTION__); //лочим весь транзитный рейс

      TripSetsQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
      TripSetsQry.Execute(); //лочим рейс
      get_basel_aero_flight_stat( ASTRA::NoExists, Qry.FieldAsInteger( "point_id" ), stats );
      if ( !stats.empty() ) {
        ProgTrace( TRACE5, "point_id=%d, stats.size()=%zu", Qry.FieldAsInteger( "point_id" ), stats.size() );
        write_basel_aero_stat( utcdate, stats );
      }
      OraSession.Commit();
    }
    ostringstream filename;
    filename <<iairp->second<<"ASTRA-" << ElemIdToElem(etAirp, iairp->first, efmtCodeNative, AstraLocale::LANG_EN)
               << "-" << DateTimeToStr(utcdate, "yyyymm") << ".csv";
    ofstream f;
    f.open( filename.str().c_str() );
    if (!f.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",filename.str().c_str());
    try {
      read_basel_aero_stat( iairp->first, f );
      f.close();
      FileSetsQry.SetVariable( "airp", iairp->first );
      FileSetsQry.Execute();
      OraSession.Commit();
    }
    catch(...) {
      try { f.close(); } catch( ... ) { };
      try {
        //в случае ошибки запишем пустой файл
        f.open(filename.str().c_str());
        if (f.is_open()) f.close();
      }
      catch( ... ) { };
      throw;
    };
  }
}

void write_basel_aero_stat( TDateTime time_create, const std::vector<TBaselStat> &stats )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO basel_stat(viewdate,viewflight,viewname,viewgroup,viewpct,viewweight,"
    "                       viewcarryon,viewpayweight,viewtag,viewuncheckin,viewstatus,"
    "                       viewcheckinno,viewstation,viewclienttype,viewcheckintime,"
    "                       viewcheckinduration,viewboardingtime,viewdepartureplantime,"
    "                       viewdeparturerealtime,viewbagnorms,viewpctweightpaidbytype,"
    "                       viewclass,point_id,airp,pax_id,time_create) "
    "VALUES(:viewdate,:viewflight,:viewname,:viewgroup,:viewpct,:viewweight,"
    "       :viewcarryon,:viewpayweight,:viewtag,:viewuncheckin,:viewstatus,"
    "       :viewcheckinno,:viewstation,:viewclienttype,:viewcheckintime,"
    "       :viewcheckinduration,:viewboardingtime,:viewdepartureplantime,"
    "       :viewdeparturerealtime,:viewbagnorms,:viewpctweightpaidbytype,"
    "       :viewclass,:point_id,:airp,:pax_id,:time_create)";
  Qry.DeclareVariable( "viewdate", otDate );
  Qry.DeclareVariable( "viewflight", otString );
  Qry.DeclareVariable( "viewname", otString );
  Qry.DeclareVariable( "viewgroup", otInteger );
  Qry.DeclareVariable( "viewpct", otInteger );
  Qry.DeclareVariable( "viewweight", otInteger );
  Qry.DeclareVariable( "viewcarryon", otInteger );
  Qry.DeclareVariable( "viewpayweight", otInteger );
  Qry.DeclareVariable( "viewtag", otString );
  Qry.DeclareVariable( "viewuncheckin", otString );
  Qry.DeclareVariable( "viewstatus", otString );
  Qry.DeclareVariable( "viewcheckinno", otInteger );
  Qry.DeclareVariable( "viewstation", otString );
  Qry.DeclareVariable( "viewclienttype", otString );
  Qry.DeclareVariable( "viewcheckintime", otDate );
  Qry.CreateVariable( "viewcheckinduration", otDate, FNull );
  Qry.DeclareVariable( "viewboardingtime", otDate );
  Qry.DeclareVariable( "viewdepartureplantime", otDate );
  Qry.DeclareVariable( "viewdeparturerealtime", otDate );
  Qry.DeclareVariable( "viewbagnorms", otString );
  Qry.DeclareVariable( "viewpctweightpaidbytype", otString );
  Qry.DeclareVariable( "viewclass", otString );
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.DeclareVariable( "airp", otString );
  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.CreateVariable( "time_create", otDate, time_create );


  for ( std::vector<TBaselStat>::const_iterator i=stats.begin(); i!=stats.end(); i++ ) {
    i->viewDate == NoExists?
      Qry.SetVariable( "viewdate", FNull ):
      Qry.SetVariable( "viewdate", i->viewDate );
    Qry.SetVariable( "viewflight", i->viewFlight );
    Qry.SetVariable( "viewname", i->viewName );
    i->viewGroup == NoExists?
      Qry.SetVariable( "viewgroup", FNull ):
      Qry.SetVariable( "viewgroup", i->viewGroup );
    i->viewPCT == NoExists?
      Qry.SetVariable( "viewpct", FNull ):
      Qry.SetVariable( "viewpct", i->viewPCT );
    i->viewWeight == NoExists?
      Qry.SetVariable( "viewweight", FNull ):
      Qry.SetVariable( "viewweight", i->viewWeight );
    i->viewCarryon == NoExists?
      Qry.SetVariable( "viewcarryon", FNull ):
      Qry.SetVariable( "viewcarryon", i->viewCarryon );
    i->viewPayWeight == NoExists?
      Qry.SetVariable( "viewpayweight", FNull ):
      Qry.SetVariable( "viewpayweight", i->viewPayWeight );
    Qry.SetVariable( "viewtag", i->viewTag );
    Qry.SetVariable( "viewuncheckin", i->viewUncheckin );
    Qry.SetVariable( "viewstatus", i->viewStatus );
    i->viewCheckinNo == NoExists?
      Qry.SetVariable( "viewcheckinno", FNull ):
      Qry.SetVariable( "viewcheckinno", i->viewCheckinNo );
    Qry.SetVariable( "viewstation", i->viewStation );
    Qry.SetVariable( "viewclienttype", i->viewClientType );
    i->viewCheckinTime == NoExists?
      Qry.SetVariable( "viewcheckintime", FNull ):
      Qry.SetVariable( "viewcheckintime", i->viewCheckinTime );
    i->viewBoardingTime == NoExists?
      Qry.SetVariable( "viewboardingtime", FNull ):
      Qry.SetVariable( "viewboardingtime", i->viewBoardingTime );
    i->viewDeparturePlanTime == NoExists?
      Qry.SetVariable( "viewdepartureplantime", FNull ):
      Qry.SetVariable( "viewdepartureplantime", i->viewDeparturePlanTime );
    i->viewDepartureRealTime == NoExists?
      Qry.SetVariable( "viewdeparturerealtime", FNull ):
      Qry.SetVariable( "viewdeparturerealtime", i->viewDepartureRealTime );
    Qry.SetVariable( "viewbagnorms", i->viewBagNorms );
    Qry.SetVariable( "viewpctweightpaidbytype", i->viewPCTWeightPaidByType );
    Qry.SetVariable( "viewclass", i->viewClass );
    Qry.SetVariable( "point_id", i->point_id );
    Qry.SetVariable( "airp", i->airp );
    i->pax_id == NoExists?
      Qry.SetVariable( "pax_id", FNull ):
      Qry.SetVariable( "pax_id", i->pax_id );
    Qry.Execute();
  }
}

void read_basel_aero_stat( const string &airp, ofstream &f )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT viewdate,viewflight,viewname,viewgroup,viewpct,viewweight,"
    "       viewcarryon,viewpayweight,viewtag,viewuncheckin,viewstatus,"
    "       viewcheckinno,viewstation,viewclienttype,viewcheckintime,"
    "       viewcheckinduration,viewboardingtime,viewdepartureplantime,"
    "       viewdeparturerealtime,viewbagnorms,viewpctweightpaidbytype,"
    "       viewclass FROM basel_stat "
    " WHERE airp=:airp ";
  Qry.CreateVariable( "airp", otString, airp );
  Qry.Execute();
  int viewdate_idx = Qry.FieldIndex( "viewdate" );
  int viewflight_idx = Qry.FieldIndex( "viewflight" );
  int viewname_idx = Qry.FieldIndex( "viewname" );
  int viewgroup_idx = Qry.FieldIndex( "viewgroup" );
  int viewpct_idx = Qry.FieldIndex( "viewpct" );
  int viewweight_idx = Qry.FieldIndex( "viewweight" );
  int viewcarryon_idx = Qry.FieldIndex( "viewcarryon" );
  int viewpayweight_idx = Qry.FieldIndex( "viewpayweight" );
  int viewtag_idx = Qry.FieldIndex( "viewtag" );
  int viewuncheckin_idx = Qry.FieldIndex( "viewuncheckin" );
  int viewstatus_idx = Qry.FieldIndex( "viewstatus" );
  int viewcheckinno_idx = Qry.FieldIndex( "viewcheckinno" );
  int viewstation_idx = Qry.FieldIndex( "viewstation" );
  int viewclienttype_idx = Qry.FieldIndex( "viewclienttype" );
  int viewcheckintime_idx = Qry.FieldIndex( "viewcheckintime" );
  int viewcheckinduration_idx = Qry.FieldIndex( "viewcheckinduration" );
  int viewboardingtime_idx = Qry.FieldIndex( "viewboardingtime" );
  int viewdepartureplantime_idx = Qry.FieldIndex( "viewdepartureplantime" );
  int viewdeparturerealtime_idx = Qry.FieldIndex( "viewdeparturerealtime" );
  int viewbagnorms_idx = Qry.FieldIndex( "viewbagnorms" );
  int viewpctweightpaidbytype_idx = Qry.FieldIndex( "viewpctweightpaidbytype" );
  int viewclass_idx = Qry.FieldIndex( "viewclass" );
  int fields_count = Qry.FieldsCount();
  for ( int idx=0; idx<fields_count; idx++ ) {
    f << string(Qry.FieldName( idx ));
    if ( idx < fields_count - 1 )
      f << ";";
  }
  f << endl;
  string region = AirpTZRegion( airp );
  ostringstream strline;
  for ( ; !Qry.Eof; Qry.Next() ) {
    if (Qry.FieldIsNULL( viewdate_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdate_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdate_idx ),region), "dd.mm.yyyy")) << ";";
    strline << Qry.FieldAsString( viewflight_idx ) << ";";
    strline << Qry.FieldAsString( viewname_idx ) << ";";
    strline << Qry.FieldAsInteger( viewgroup_idx ) << ";";
    if (Qry.FieldIsNULL( viewpct_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewpct_idx ) == 0?"":Qry.FieldAsString( viewpct_idx )) << ";";
    if (Qry.FieldIsNULL( viewweight_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewweight_idx ) == 0?"":Qry.FieldAsString( viewweight_idx )) << ";";
    if (Qry.FieldIsNULL( viewcarryon_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewcarryon_idx ) == 0?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    if (Qry.FieldIsNULL( viewpayweight_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewpayweight_idx ) == 0?"":Qry.FieldAsString( viewpayweight_idx )) << ";";
    strline << Qry.FieldAsString( viewtag_idx ) << ";";
    strline << Qry.FieldAsString( viewuncheckin_idx ) << ";";
    strline << Qry.FieldAsString( viewstatus_idx ) << ";";
    if (Qry.FieldIsNULL( viewcheckinno_idx )) strline << ";";
    else
      strline << (Qry.FieldAsInteger( viewcheckinno_idx ) == NoExists?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    strline << Qry.FieldAsString( viewstation_idx ) << ";";
    strline << Qry.FieldAsString( viewclienttype_idx ) << ";";
    if (Qry.FieldIsNULL( viewcheckintime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewcheckintime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckintime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewcheckinduration_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewcheckinduration_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckinduration_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewboardingtime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewboardingtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewboardingtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewdepartureplantime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdepartureplantime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdepartureplantime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    if (Qry.FieldIsNULL( viewdeparturerealtime_idx )) strline << ";";
    else
      strline << (Qry.FieldAsDateTime( viewdeparturerealtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdeparturerealtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << Qry.FieldAsString( viewbagnorms_idx ) << ";";
    strline << Qry.FieldAsString( viewpctweightpaidbytype_idx ) << ";";
    strline << Qry.FieldAsString( viewclass_idx );
    f << ConvertCodepage( strline.str(), "CP866", "WINDOWS-1251" ) << endl;
    strline.str(string());
    strline.str();
  }
}

void get_basel_aero_flight_stat(TDateTime part_key, int point_id, std::vector<TBaselStat> &stats )
{
  stats.clear();

  TTripInfo operFlt;
  if (!operFlt.getByPointId(part_key, point_id, FlightProps(FlightProps::NotCancelled,
                                                            FlightProps::WithCheckIn))) return;

  TRegEvents events;
  events.fromDB(part_key, point_id);

  string bag_sql;
  string bag_pc_sql;
  TQuery BagQry(&OraSession);
  TQuery TimeQry(&OraSession);
  if (part_key!=NoExists)
  {
    bag_sql=
      "SELECT arx_bag2.* "
      "FROM arx_pax_grp,arx_bag2 "
      "WHERE arx_pax_grp.part_key=arx_bag2.part_key AND "
      "      arx_pax_grp.grp_id=arx_bag2.grp_id AND "
      "      arx_pax_grp.part_key=:part_key AND "
      "      arx_pax_grp.point_dep=:point_id AND "
      "      arx_pax_grp.grp_id=:grp_id AND "
      "      arch.bag_pool_refused(arx_bag2.part_key,arx_bag2.grp_id,arx_bag2.bag_pool_num,arx_pax_grp.class,arx_pax_grp.bag_refuse)=0";
  }
  else
  {
    bag_sql=
      "SELECT bag2.* "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0";
    bag_pc_sql=
      "SELECT bag2.* "
      "FROM bag2 "
      "WHERE bag2.grp_id=:grp_id AND "
      "      ckin.get_bag_pool_pax_id(bag2.grp_id,bag2.bag_pool_num,0)=:pax_id";

    TimeQry.SQLText =
      "SELECT time,NVL(stations.name,aodb_pax_change.desk) station, client_type, stations.airp "
      " FROM aodb_pax_change,stations "
      " WHERE pax_id=:pax_id AND aodb_pax_change.reg_no=:reg_no AND "
      "       aodb_pax_change.work_mode=:work_mode AND "
      "       aodb_pax_change.desk=stations.desk(+) AND aodb_pax_change.work_mode=stations.work_mode(+)";
    TimeQry.DeclareVariable( "pax_id", otInteger );
    TimeQry.DeclareVariable( "reg_no", otInteger );
    TimeQry.CreateVariable( "work_mode", otString, "Р" );
  };

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT pax_grp.grp_id, pax_grp.class, NVL(pax_grp.piece_concept, 0) AS piece_concept, "
    "       pax.pax_id, pax.surname, pax.name, "
    "       pax.refuse, pax.pr_brd, pax.reg_no, "
    "         ";
  if (part_key!=NoExists)
  {
    sql <<
      "arch.get_bagAmount2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
      "arch.get_bagWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
      "arch.get_rkWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
      "arch.get_excess_wt(pax.part_key, pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.excess, pax_grp.bag_refuse) AS excess_wt, "
      "pax.excess_pc, "
      "arch.get_birks2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,'RU') AS tags, "
      "arch.get_main_pax_id2(pax_grp.part_key,pax_grp.grp_id) AS main_pax_id "
      "FROM arx_pax_grp pax_grp, arx_pax pax "
      "WHERE pax_grp.part_key=pax.part_key(+) AND "
      "      pax_grp.grp_id=pax.grp_id(+) AND "
      "      pax_grp.part_key=:part_key AND "
      "      pax_grp.point_dep=:point_id AND "
      "      pax_grp.status NOT IN ('E') "
      "ORDER BY pax.reg_no NULLS LAST, pax.seats DESC NULLS LAST";
      Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
  {
    sql <<
      "ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
      "ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
      "ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
      "ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
      "ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
      "ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,'RU') AS tags, "
      "ckin.get_main_pax_id2(pax_grp.grp_id) AS main_pax_id "
      "FROM pax_grp, pax "
      "WHERE pax_grp.grp_id=pax.grp_id(+) AND "
      "      pax_grp.point_dep=:point_id AND "
      "      pax_grp.status NOT IN ('E') "
      "ORDER BY pax.reg_no NULLS LAST, pax.seats DESC NULLS LAST";
  };
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    bool piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
    int main_pax_id=Qry.FieldIsNULL("main_pax_id")?NoExists:Qry.FieldAsInteger("main_pax_id");

    TBaselStat stat;
    stat.point_id = point_id;
    stat.airp = operFlt.airp;
    stat.viewGroup = Qry.FieldAsInteger("grp_id");
    stat.pax_id = Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
    stat.viewDate = operFlt.scd_out;
    stat.viewFlight = operFlt.airline;
    string tmp = IntToString( operFlt.flt_no );
    while ( tmp.size() < 3 ) tmp = "0" + tmp;
    stat.viewFlight += tmp + operFlt.suffix;
    if (stat.pax_id!=NoExists)
      stat.viewName = string(Qry.FieldAsString("surname")) + "/" + Qry.FieldAsString("name");
    else
      stat.viewName = "БАГАЖ БЕЗ СОПРОВОЖДЕНИЯ";
    stat.viewName = stat.viewName.substr(0,130);
    stat.viewPCT = Qry.FieldAsInteger("bag_amount");
    stat.viewWeight = Qry.FieldAsInteger("bag_weight");
    stat.viewCarryon = Qry.FieldAsInteger("rk_weight");
    stat.viewPayWeight = TComplexBagExcess(TBagPieces(Qry.FieldAsInteger("excess_pc")),
                                           TBagKilos(Qry.FieldAsInteger("excess_wt"))).getDeprecatedInt();
    stat.viewTag = string(Qry.FieldAsString("tags")).substr(0,100);
    pair<TDateTime, TDateTime> times(NoExists, NoExists);
    std::list< std::pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
    if (stat.pax_id!=NoExists)
    {
      stat.viewUncheckin = ElemIdToNameLong(etRefusalType, Qry.FieldAsString("refuse")).substr(0,50);
      stat.viewStatus = string(Qry.FieldIsNULL("refuse")?(Qry.FieldAsInteger("pr_brd")==0?"зарегистрирован":"прошел посадку"):"разрегистрирован").substr(0,30);
      stat.viewCheckinNo = Qry.FieldAsInteger("reg_no");
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,stat.viewCheckinNo));
      if (i!=events.end()) times=i->second;
      if (!Qry.FieldIsNULL("refuse") || Qry.FieldAsInteger("pr_brd")==0) times.second=NoExists;

      if (!piece_concept)
        WeightConcept::PaxNormsFromDB(part_key, stat.pax_id, norms);
    }
    else
    {
      stat.viewCheckinNo = NoExists;
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,NoExists));
      if (i!=events.end()) times=i->second;

      if (!piece_concept)
        WeightConcept::GrpNormsFromDB(part_key, stat.viewGroup, norms);
    };
    stat.viewCheckinTime = times.first;
    stat.viewChekinDuration = NoExists;
    stat.viewBoardingTime = times.second;
    stat.viewDeparturePlanTime = operFlt.scd_out;
    stat.viewDepartureRealTime = operFlt.act_out?operFlt.act_out.get():NoExists;

    if (!piece_concept)
    {
      std::map< string/*bag_type_view*/, WeightConcept::TNormItem> norms_normal;
      WeightConcept::ConvertNormsList(norms, norms_normal);
      std::map< string/*bag_type_view*/, WeightConcept::TNormItem>::const_iterator n=norms_normal.begin();
      for(;n!=norms_normal.end();++n)
      {
        if (n!=norms_normal.begin()) stat.viewBagNorms += ", ";
        if (!n->first.empty())
          stat.viewBagNorms += n->first + ": ";
        stat.viewBagNorms += n->second.str(AstraLocale::LANG_RU);
      };
    };

    TPaidToLogInfo paidInfo;
    BagQry.Clear();
    BagQry.CreateVariable("grp_id", otInteger, stat.viewGroup);
    if (part_key!=NoExists)
    {
      BagQry.CreateVariable("part_key", otDate, part_key);
      BagQry.CreateVariable("point_id", otInteger, point_id);
    };
    if (piece_concept && stat.pax_id!=NoExists)
    {
      BagQry.SQLText=bag_pc_sql;
      BagQry.CreateVariable("pax_id", otInteger, stat.pax_id);
    }
    else if (stat.pax_id==NoExists || stat.pax_id==main_pax_id)
    {
      BagQry.SQLText=bag_sql;
    };
    if (!BagQry.SQLText.IsEmpty())
    {
      BagQry.Execute();
      if (!BagQry.Eof)
      {
        for(;!BagQry.Eof;BagQry.Next())
        {
          CheckIn::TBagItem bagItem;
          bagItem.fromDB(BagQry);
          paidInfo.add(bagItem);
        };
        if (!piece_concept)
        {
          WeightConcept::TPaidBagList paid;
          WeightConcept::PaidBagFromDB(part_key, stat.viewGroup, paid);
          for(WeightConcept::TPaidBagList::const_iterator p=paid.begin(); p!=paid.end(); ++p)
            paidInfo.add(*p);
        }
        else
        {
          if (part_key==NoExists)
          {
            TPaidRFISCListWithAuto paid;
            paid.fromDB(stat.pax_id==NoExists?stat.viewGroup:stat.pax_id, stat.pax_id==NoExists);
            for(TPaidRFISCListWithAuto::const_iterator p=paid.begin(); p!=paid.end(); ++p)
            {
              if (p->second.trfer_num!=0) continue;
              paidInfo.add(p->second);
            };
          };
        };
        ostringstream str;
        for(map<TEventsSumBagKey, TEventsSumBagItem>::const_iterator b=paidInfo.bag.begin(); b!=paidInfo.bag.end(); ++b)
        {
          if (b->second.empty()) continue;

          if (!str.str().empty()) str << ", ";
          if (!b->first.bag_type_view.empty())
            str << b->first.bag_type_view << ":";
          if (b->first.is_trfer)
            str << "T:";
          str << b->second.amount << "/" << b->second.weight << "/" << b->second.paid;
        };
        stat.viewPCTWeightPaidByType=str.str();
      };
    };

    stat.viewClass = ElemIdToNameLong(etClass, Qry.FieldAsString("class"));
    if ( part_key==NoExists && stat.pax_id!=NoExists ) {
      TimeQry.SetVariable( "pax_id", stat.pax_id );
      TimeQry.SetVariable( "reg_no", stat.viewCheckinNo );
      TimeQry.Execute();
      if ( !TimeQry.Eof ) {
        if ( string(TimeQry.FieldAsString( "airp" )) == stat.airp )
          stat.viewStation = TimeQry.FieldAsString( "station" );
        stat.viewClientType = TimeQry.FieldAsString( "client_type" );
      }
    }
    stats.push_back( stat );
  };
};

int basel_stat(int argc,char **argv)
{
  TDateTime part_key=ASTRA::NoExists;
  int point_id=ASTRA::NoExists;

  char buf[200];
  int res;
  char c;

  for(int pass=0; pass<3; pass++)
  {
    printf("input part_key(dd.mm.yy hh:nn:ss): ");
    *buf=0;
    c=0;
    res=scanf("%[^\n]", buf);
    scanf("%c", &c);
    if (res==1 && *buf!=0)
    {
      if (StrToDateTime(buf, "dd.mm.yy hh:nn:ss", part_key)==EOF)
      {
        printf("wrong part_key!\n");
        if (pass<2) continue; else return 0;
      }
    };
    break;
  };

  for(int pass=0; pass<3; pass++)
  {
    printf("input point_id: ");
    *buf=0;
    c=0;
    res=scanf("%[^\n]", buf);
    scanf("%c", &c);
    if (res==1 && *buf!=0)
    {
      if (StrToInt(buf, point_id)==EOF)
      {
        printf("wrong point_id!\n");
        if (pass<2) continue; else return 0;
      }
    };
    break;
  };

  std::vector<TBaselStat> stats;
  get_basel_aero_flight_stat(part_key, point_id, stats);
  write_basel_aero_stat( NowUTC(), stats);

  return 0;
};

