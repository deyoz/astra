#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

#include "basic.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "cache.h"
#include "passenger.h"
#include "events.h"
#include "points.h"
#include "stl_utils.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;



using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
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


void get_basel_aero_flight_stat( BASIC::TDateTime part_key, int point_id, std::vector<TBaselStat> &stats );
void write_basel_aero_stat( BASIC::TDateTime time_create, const std::vector<TBaselStat> &stats );
void read_basel_aero_stat( const string &airp, ofstream &f );

void sych_basel_aero_stat( BASIC::TDateTime utcdate )
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
		  flights.Lock(); //лочим весь транзитный рейс
		  
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

void write_basel_aero_stat( BASIC::TDateTime time_create, const std::vector<TBaselStat> &stats )
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
    Qry.SetVariable( "viewdate", i->viewDate == NoExists?FNull:i->viewDate );
    Qry.SetVariable( "viewflight", i->viewFlight );
    Qry.SetVariable( "viewname", i->viewName );
    Qry.SetVariable( "viewgroup", i->viewGroup == NoExists?FNull:i->viewGroup );
    Qry.SetVariable( "viewpct", i->viewPCT == NoExists?FNull:i->viewPCT );
    Qry.SetVariable( "viewweight", i->viewWeight == NoExists?FNull:i->viewWeight );
    Qry.SetVariable( "viewcarryon", i->viewCarryon == NoExists?FNull:i->viewCarryon );
    Qry.SetVariable( "viewpayweight", i->viewPayWeight == NoExists?FNull:i->viewPayWeight );
    Qry.SetVariable( "viewtag", i->viewTag );
    Qry.SetVariable( "viewuncheckin", i->viewUncheckin );
    Qry.SetVariable( "viewstatus", i->viewStatus );
    Qry.SetVariable( "viewcheckinno", i->viewCheckinNo == NoExists?FNull:i->viewCheckinNo );
    Qry.SetVariable( "viewstation", i->viewStation );
    Qry.SetVariable( "viewclienttype", i->viewClientType );
    Qry.SetVariable( "viewcheckintime", i->viewCheckinTime == NoExists?FNull:i->viewCheckinTime );
    Qry.SetVariable( "viewboardingtime", i->viewBoardingTime == NoExists?FNull:i->viewBoardingTime );
    Qry.SetVariable( "viewdepartureplantime", i->viewDeparturePlanTime == NoExists?FNull:i->viewDeparturePlanTime );
    Qry.SetVariable( "viewdeparturerealtime", i->viewDepartureRealTime == NoExists?FNull:i->viewDepartureRealTime );
    Qry.SetVariable( "viewbagnorms", i->viewBagNorms );
    Qry.SetVariable( "viewpctweightpaidbytype", i->viewPCTWeightPaidByType );
    Qry.SetVariable( "viewclass", i->viewClass );
    Qry.SetVariable( "point_id", i->point_id );
    Qry.SetVariable( "airp", i->airp );
    Qry.SetVariable( "pax_id", i->pax_id == NoExists?FNull:i->pax_id );
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
    strline << (Qry.FieldAsDateTime( viewdate_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdate_idx ),region), "dd.mm.yyyy")) << ";";
    strline << Qry.FieldAsString( viewflight_idx ) << ";";
    strline << Qry.FieldAsString( viewname_idx ) << ";";
    strline << Qry.FieldAsInteger( viewgroup_idx ) << ";";
    strline << (Qry.FieldAsInteger( viewpct_idx ) == 0?"":Qry.FieldAsString( viewpct_idx )) << ";";
    strline << (Qry.FieldAsInteger( viewweight_idx ) == 0?"":Qry.FieldAsString( viewweight_idx )) << ";";
    strline << (Qry.FieldAsInteger( viewcarryon_idx ) == 0?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    strline << (Qry.FieldAsInteger( viewpayweight_idx ) == 0?"":Qry.FieldAsString( viewpayweight_idx )) << ";";
    strline << Qry.FieldAsString( viewtag_idx ) << ";";
    strline << Qry.FieldAsString( viewuncheckin_idx ) << ";";
    strline << Qry.FieldAsString( viewstatus_idx ) << ";";
    strline << (Qry.FieldAsInteger( viewcheckinno_idx ) == NoExists?"":Qry.FieldAsString( viewcarryon_idx )) << ";";
    strline << Qry.FieldAsString( viewstation_idx ) << ";";
    strline << Qry.FieldAsString( viewclienttype_idx ) << ";";
    strline << (Qry.FieldAsDateTime( viewcheckintime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckintime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << (Qry.FieldAsDateTime( viewcheckinduration_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewcheckinduration_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << (Qry.FieldAsDateTime( viewboardingtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewboardingtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << (Qry.FieldAsDateTime( viewdepartureplantime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdepartureplantime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << (Qry.FieldAsDateTime( viewdeparturerealtime_idx ) == NoExists?"":DateTimeToStr(UTCToLocal(Qry.FieldAsDateTime( viewdeparturerealtime_idx ),region), "dd.mm.yyyy hh:nn")) << ";";
    strline << Qry.FieldAsString( viewbagnorms_idx ) << ";";
    strline << Qry.FieldAsString( viewpctweightpaidbytype_idx ) << ";";
    strline << Qry.FieldAsString( viewclass_idx );
    f << ConvertCodepage( strline.str(), "CP866", "WINDOWS-1251" ) << endl;
    strline.str(string());
    strline.str();
  }
}

void get_basel_aero_flight_stat(BASIC::TDateTime part_key, int point_id, std::vector<TBaselStat> &stats )
{
  stats.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT airline, flt_no, suffix, airp, scd_out, act_out AS real_out ";
  if (part_key!=NoExists)
  {
    sql << "FROM arx_points "
           "WHERE part_key=:part_key AND point_id=:point_id AND "
           "      pr_del=0 AND pr_reg<>0 ";
    Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
    sql << "FROM points "
           "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 ";
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  TTripInfo operFlt;
  operFlt.Init(Qry);

  map< pair<int, int>, pair<TDateTime, TDateTime> > events;
  Qry.Clear();
  sql.str("");
  sql <<
    "SELECT id3 AS grp_id, id2 AS reg_no, "
    "       MIN(DECODE(INSTR(msg,'зарегистрирован'),0,TO_DATE(NULL),time)) AS ckin_time, "
    "       MAX(DECODE(INSTR(msg,'прошел посадку'),0,TO_DATE(NULL),time)) AS brd_time ";
  if (part_key!=NoExists)
  {
    sql <<
      "FROM arx_events "
      "WHERE type=:evtPax AND part_key=:part_key AND id1=:point_id AND ";
    Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
    sql <<
      "FROM events "
      "WHERE type=:evtPax AND id1=:point_id AND ";
  sql <<
    "      (msg like '%зарегистрирован%' OR msg like '%прошел посадку%') "
    "GROUP BY id3, id2";
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("evtPax", otString, EncodeEventType(ASTRA::evtPax));
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int grp_id=Qry.FieldIsNULL("grp_id")?NoExists:Qry.FieldAsInteger("grp_id");
    int reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
    TDateTime ckin_time=Qry.FieldIsNULL("ckin_time")?NoExists:Qry.FieldAsDateTime("ckin_time");
    TDateTime brd_time=Qry.FieldIsNULL("brd_time")?NoExists:Qry.FieldAsDateTime("brd_time");
    events[ make_pair(grp_id, reg_no) ] = make_pair(ckin_time, brd_time);
  };

  TQuery PaxNormQry(&OraSession);
  TQuery GrpNormQry(&OraSession);
  TQuery BagQry(&OraSession);
  TQuery PaidQry(&OraSession);
  TQuery TimeQry(&OraSession);
  if (part_key!=NoExists)
  {
    PaxNormQry.SQLText=
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_pax_norms,bag_norms "
      "WHERE arx_pax_norms.norm_id=bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id "
      "UNION "
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_pax_norms,arx_bag_norms "
      "WHERE arx_pax_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id "
      "ORDER BY bag_type, norm_type NULLS LAST";
    PaxNormQry.CreateVariable("part_key", otDate, part_key);

    GrpNormQry.SQLText=
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_grp_norms,bag_norms "
      "WHERE arx_grp_norms.norm_id=bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id "
      "UNION "
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_grp_norms,arx_bag_norms "
      "WHERE arx_grp_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id "
      "ORDER BY bag_type, norm_type NULLS LAST";
    GrpNormQry.CreateVariable("part_key", otDate, part_key);

    BagQry.SQLText=
      "SELECT bag_type, SUM(amount) AS amount, SUM(weight) AS weight "
      "FROM arx_pax_grp,arx_bag2 "
      "WHERE arx_pax_grp.part_key=arx_bag2.part_key AND "
      "      arx_pax_grp.grp_id=arx_bag2.grp_id AND "
      "      arx_pax_grp.part_key=:part_key AND "
      "      arx_pax_grp.point_dep=:point_id AND "
      "      arx_pax_grp.grp_id=:grp_id AND "
      "      arch.bag_pool_refused(arx_bag2.part_key,arx_bag2.grp_id,arx_bag2.bag_pool_num,arx_pax_grp.class,arx_pax_grp.bag_refuse)=0 "
      "GROUP BY bag_type";
    BagQry.CreateVariable("part_key", otDate, part_key);
    BagQry.CreateVariable("point_id", otInteger, point_id);

    PaidQry.SQLText=
      "SELECT bag_type, weight FROM arx_paid_bag WHERE part_key=:part_key AND grp_id=:grp_id";
    PaidQry.CreateVariable("part_key", otDate, part_key);
  }
  else
  {
    PaxNormQry.SQLText=
      "SELECT pax_norms.bag_type, pax_norms.norm_id, pax_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM pax_norms,bag_norms "
      "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";

    GrpNormQry.SQLText=
      "SELECT grp_norms.bag_type, grp_norms.norm_id, grp_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM grp_norms,bag_norms "
      "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";

    BagQry.SQLText=
      "SELECT bag_type, SUM(amount) AS amount, SUM(weight) AS weight "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
      "GROUP BY bag_type";

    PaidQry.SQLText=
      "SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id";

    TimeQry.SQLText =
      "SELECT time,NVL(stations.name,aodb_pax_change.desk) station, client_type, airp "
      " FROM aodb_pax_change,stations "
      " WHERE pax_id=:pax_id AND aodb_pax_change.reg_no=:reg_no AND "
      "       aodb_pax_change.work_mode=:work_mode AND "
      "       aodb_pax_change.desk=stations.desk(+) AND aodb_pax_change.work_mode=stations.work_mode(+)";
    TimeQry.DeclareVariable( "pax_id", otInteger );
    TimeQry.DeclareVariable( "reg_no", otInteger );
    TimeQry.CreateVariable( "work_mode", otString, "Р" );
  };
  PaxNormQry.DeclareVariable("pax_id", otInteger);
  GrpNormQry.DeclareVariable("grp_id", otInteger);
  BagQry.DeclareVariable("grp_id", otInteger);
  PaidQry.DeclareVariable("grp_id", otInteger);


  Qry.Clear();
  sql.str("");
  sql <<
    "SELECT pax_grp.grp_id, pax_grp.class, pax.pax_id, pax.surname, pax.name, "
    "       pax.refuse, pax.pr_brd, pax.reg_no, "
    "         ";
  if (part_key!=NoExists)
  {
    sql <<
      "arch.get_bagAmount2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
      "arch.get_bagWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
      "arch.get_rkWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
      "arch.get_excess(pax_grp.part_key,pax_grp.grp_id,pax.pax_id) AS excess, "
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
      "ckin.get_excess(pax_grp.grp_id,pax.pax_id) AS excess, "
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
    TBaselStat stat;
    stat.point_id = point_id;
    stat.airp = operFlt.airp;
    stat.viewGroup = Qry.FieldAsInteger("grp_id");
    stat.pax_id = Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
    int main_pax_id=Qry.FieldIsNULL("main_pax_id")?NoExists:Qry.FieldAsInteger("main_pax_id");
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
    stat.viewPayWeight = Qry.FieldAsInteger("excess");
    stat.viewTag = string(Qry.FieldAsString("tags")).substr(0,100);
    pair<TDateTime, TDateTime> times(NoExists, NoExists);
    TQuery &NormQry=stat.pax_id==NoExists?GrpNormQry:PaxNormQry;
    if (stat.pax_id!=NoExists)
    {
      stat.viewUncheckin = ElemIdToNameLong(etRefusalType, Qry.FieldAsString("refuse")).substr(0,50);
      stat.viewStatus = string(Qry.FieldIsNULL("refuse")?(Qry.FieldAsInteger("pr_brd")==0?"зарегистрирован":"прошел посадку"):"разрегистрирован").substr(0,30);
      stat.viewCheckinNo = Qry.FieldAsInteger("reg_no");
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,stat.viewCheckinNo));
      if (i!=events.end()) times=i->second;
      if (!Qry.FieldIsNULL("refuse") || Qry.FieldAsInteger("pr_brd")==0) times.second=NoExists;

      NormQry.SetVariable("pax_id", stat.pax_id);
    }
    else
    {
      stat.viewCheckinNo = NoExists;
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(stat.viewGroup,NoExists));
      if (i!=events.end()) times=i->second;

      NormQry.SetVariable("grp_id", stat.viewGroup);
    };
    stat.viewCheckinTime = times.first;
    stat.viewChekinDuration = NoExists;
    stat.viewBoardingTime = times.second;
    stat.viewDeparturePlanTime = operFlt.scd_out;
    stat.viewDepartureRealTime = operFlt.real_out;

    std::map< int/*bag_type*/, CheckIn::TNormItem> norms;
    NormQry.Execute();
    int prior_bag_type=NoExists;
    for(;!NormQry.Eof;NormQry.Next())
    {
      CheckIn::TPaxNormItem paxNormItem;
      CheckIn::TNormItem normItem;
      paxNormItem.fromDB(NormQry);
      normItem.fromDB(NormQry);

      int bag_type=paxNormItem.bag_type==NoExists?-1:paxNormItem.bag_type;

      if (prior_bag_type==bag_type) continue;
      prior_bag_type=bag_type;
      if (normItem.empty())
      {
        if (paxNormItem.empty()) continue;
/*        if (pax_id!=NoExists)
          printf("norm not found norm_id=%s part_key=%s pax_id=%d\n",
                 paxNormItem.norm_id==NoExists?"NoExists":IntToString(paxNormItem.norm_id).c_str(),
                 part_key==NoExists?"NoExists":DateTimeToStr(part_key,"dd.mm.yy hh:nn:ss").c_str(),
                 pax_id);
        else
          printf("norm not found norm_id=%s part_key=%s grp_id=%d\n",
                 paxNormItem.norm_id==NoExists?"NoExists":IntToString(paxNormItem.norm_id).c_str(),
                 part_key==NoExists?"NoExists":DateTimeToStr(part_key,"dd.mm.yy hh:nn:ss").c_str(),
                 grp_id);*/
        continue;
      };

      norms[bag_type]=normItem;
    };

    std::map< int/*bag_type*/, CheckIn::TNormItem>::const_iterator n=norms.begin();
    for(;n!=norms.end();++n)
    {
      if (n!=norms.begin()) stat.viewBagNorms += ", ";
      if (n->first!=-1) {
        string tmp;
        tmp = IntToString( n->first );
        while ( tmp.size() < 2 ) tmp = "0" + tmp;
        stat.viewBagNorms += tmp + ": ";
      }
      stat.viewBagNorms += n->second.str();
    };

    if (stat.pax_id==NoExists || stat.pax_id==main_pax_id)
    {
      map< int/*bag_type*/, TPaidToLogInfo> paid;

      BagQry.SetVariable("grp_id",stat.viewGroup);
      BagQry.Execute();
      if (!BagQry.Eof)
      {
        //багаж есть
        for(;!BagQry.Eof;BagQry.Next())
        {
          int bag_type=BagQry.FieldIsNULL("bag_type")?-1:BagQry.FieldAsInteger("bag_type");

          std::map< int/*bag_type*/, TPaidToLogInfo>::iterator i=paid.find(bag_type);
          if (i!=paid.end())
          {
            i->second.bag_amount+=BagQry.FieldAsInteger("amount");
            i->second.bag_weight+=BagQry.FieldAsInteger("weight");
          }
          else
          {
            TPaidToLogInfo &paidInfo=paid[bag_type];
            paidInfo.bag_type=bag_type;
            paidInfo.bag_amount=BagQry.FieldAsInteger("amount");
            paidInfo.bag_weight=BagQry.FieldAsInteger("weight");
          };
        };

        PaidQry.SetVariable("grp_id",stat.viewGroup);
        PaidQry.Execute();
        for(;!PaidQry.Eof;PaidQry.Next())
        {
          int bag_type=PaidQry.FieldIsNULL("bag_type")?-1:PaidQry.FieldAsInteger("bag_type");
          TPaidToLogInfo &paidInfo=paid[bag_type];
          paidInfo.bag_type=bag_type;
          paidInfo.paid_weight=PaidQry.FieldAsInteger("weight");
        };

        map< int, TPaidToLogInfo>::const_iterator p=paid.begin();
        for(;p!=paid.end();++p)
        {
          if (p!=paid.begin()) stat.viewPCTWeightPaidByType += ", ";
          if (p->second.bag_type!=-1) {
            string tmp = IntToString( p->second.bag_type );
            while ( tmp.size() < 2 ) tmp = "0" + tmp;
            stat.viewPCTWeightPaidByType += tmp + ":";
          }
          stat.viewPCTWeightPaidByType += IntToString( p->second.bag_amount ) + "/";
          stat.viewPCTWeightPaidByType += IntToString( p->second.bag_weight ) + "/";
          stat.viewPCTWeightPaidByType += IntToString( p->second.paid_weight );
        };
      };
    };
    stat.viewClass = ElemIdToNameLong(etClass, Qry.FieldAsString("class"));
    if ( stat.pax_id!=NoExists ) {
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
