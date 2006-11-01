#include <stdlib.h>
#include "tripinfo.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "basic.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "prepreg.h"
#include "brd.h"

using namespace std;
using namespace BASIC;

struct TTrferItem {
  std::string last_trfer;
  int hall_id;
  std::string hall_name;
  int seats,adult,child,baby,foreigner;
  int umnr,vip,rkWeight,bagAmount,bagWeight,excess;
};


struct TCounterItem {
  std::string cl;
  std::string target;
  std::vector<TTrferItem> trfer;
  int cfg,resa,tranzit;
};


void TSQLParams::addVariable( TVar &var )
{
  vars.push_back( var );
}

void TSQLParams::addVariable( string aname, otFieldType atype, string avalue )
{
  TVar var( aname, atype, avalue );
  vars.push_back( var );
}

void TSQLParams::clearVariables( )
{
  vars.clear();
}

void TSQLParams::setVariables( TQuery &Qry ) {
  Qry.ClearVariables();
  for ( std::vector<TVar>::iterator ip=vars.begin(); ip!=vars.end(); ip++ ) {
    Qry.CreateVariable( ip->name, ip->type, ip->value );
    ProgTrace( TRACE5, "Qry.CreateVariable name=%s, value=%s", ip->name.c_str(), ip->value.c_str() );
  }
}

TSQL::TSQL() {
 /* в этом конструкторе задаются окончания запроса по рейсам и переменные участв. в запросе */
 createSQLTrips();
}

TSQL *TSQL::Instance() {
  static TSQL *instance_ = 0;
  if ( !instance_ )
    instance_ = new TSQL();
  return instance_;
}

void TSQL::createSQLTrips( ) {
  TSQLParams p;

  /* ------------ПОСАДКА-------------- */
  /* задаем текст */
  p.sqlfrom =
    "points,trip_stations ";
  p.sqlwhere =
    "points.point_id=trip_stations.point_id AND "
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "trip_stations.desk= :desk AND trip_stations.work_mode='П' AND "
    "gtimer.is_final_stage(  points.point_id, :brd_stage_type, :brd_open_stage_id) <> 0 ";
  /* задаем переменные */
  p.addVariable( "brd_stage_type", otInteger, IntToString( stBoarding ) );
  p.addVariable( "brd_open_stage_id", otInteger, IntToString( sOpenBoarding ) );
  /* запоминаем */
  sqltrips[ "BRDBUS.EXE" ] = p;
  /* не забываем очищать за собой переменные */
  p.clearVariables();

  /* ------------ЦЕНТРОВКА------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "NVL(points.est_out,points.scd_out) BETWEEN system.UTCSYSDATE-1 AND system.UTCSYSDATE+1 ";
  sqltrips[ "CENT.EXE" ] = p;
  p.clearVariables();


  /* ------------РЕГИСТРАЦИЯ------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.pr_del=0 AND "
    "(points.act_out IS NULL OR :act_ignore<>0) AND "
    "(gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_open_stage_id)<>0 OR "
    " gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_close_stage_id)<>0 OR "
    " gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_doc_stage_id)<>0) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sRegDoc ) );
  sqltrips[ "AIR.EXE" ] = p;
  p.clearVariables();

  /* ------------ДОКУМЕНТАЦИЯ------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.pr_del=0 AND "
    "(points.act_out IS NOT NULL OR "
    " points.act_out IS NULL AND "
    " gtimer.is_final_stage( points.point_id, :ckin_stage_type, :no_active_stage_id)=0 AND "
    " gtimer.is_final_stage( points.point_id, :ckin_stage_type, :ckin_prep_stage_id)=0) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "no_active_stage_id", otInteger, IntToString( sNoActive ) );
  p.addVariable( "ckin_prep_stage_id", otInteger, IntToString( sPrepCheckIn ) );
  sqltrips[ "DOCS.EXE" ] = p;
  p.clearVariables();

  /* ------------КАССА------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "(gtimer.is_final_stage( points.point_id, :ckin_stage_type, :ckin_open_stage_id)<>0 OR "
    " gtimer.is_final_stage( points.point_id, :ckin_stage_type, :ckin_close_stage_id)<>0) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  sqltrips[ "KASSA.EXE" ] = p;
  p.clearVariables();

  /* ------------ПОДГОТОВКА------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.pr_del=0 AND "
    "(points.act_out IS NOT NULL OR "
    " points.act_out IS NULL AND "
    " gtimer.is_final_stage(points.point_id, :ckin_stage_type, :no_active_stage_id)=0) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "no_active_stage_id", otInteger, IntToString( sNoActive ) );
  sqltrips[ "PREPREG.EXE" ] = p;
  p.clearVariables();

  /* ------------ТЕЛЕГРАММЫ------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "TRUNC(NVL(points.act_out,NVL(points.est_out,points.scd_out)))-TRUNC(system.UTCSYSDATE) "
    "BETWEEN -15 AND 1 AND points.pr_del>=0 ";
  sqltrips[ "TLG.EXE" ] = p;
  p.clearVariables();
}

/*

посадка:

SELECT trips.trip_id,
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str
FROM trips,trip_stations
WHERE trips.trip_id=trip_stations.trip_id AND
      trips.act IS NULL AND trips.status=0 AND
      trip_stations.name= :station AND trip_stations.work_mode='╧' AND
      gtimer.is_final_stage(  trips.trip_id, :brd_stage_type, :brd_open_stage_id) <> 0
ORDER BY NVL(act,NVL(est,scd))

регистрация:

SELECT trips.trip_id,
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str
FROM trips
WHERE trips.status=0 AND
      (act IS NULL OR :act_ignore<>0) AND
      (gtimer.is_final_stage(trips.trip_id, :ckin_stage_type, :ckin_open_stage_id)<>0 OR
       gtimer.is_final_stage(trips.trip_id, :ckin_stage_type, :ckin_close_stage_id)<>0 OR
       gtimer.is_final_stage(trips.trip_id, :ckin_stage_type, :ckin_doc_stage_id)<>0)
ORDER BY NVL(act,NVL(est,scd))

документация:

SELECT trips.trip_id,
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str
FROM trips
WHERE trips.status=0 AND
      (act IS NOT NULL OR
       act IS NULL AND
       gtimer.is_final_stage( trips.trip_id, :ckin_stage_type, :no_active_stage_id)=0 AND
       gtimer.is_final_stage( trips.trip_id, :ckin_stage_type, :ckin_prep_stage_id)=0)
ORDER BY NVL(act,NVL(est,scd))

касса:

SELECT trips.trip_id,
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str
FROM trips
WHERE trips.act IS NULL AND trips.status=0 AND
      (gtimer.is_final_stage( trips.trip_id, :ckin_stage_type, :ckin_open_stage_id)<>0 OR
       gtimer.is_final_stage( trips.trip_id, :ckin_stage_type, :ckin_close_stage_id)<>0)
ORDER BY NVL(act,NVL(est,scd))

телеграммы:

SELECT trip_id,1 AS pr_dep,
       '|-> '||
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str,
       NVL(act,NVL(est,scd)) AS act
FROM trips
WHERE TRUNC(NVL(act,NVL(est,scd)))-TRUNC(SYSDATE) BETWEEN -15 AND 1 AND
      status<>-1
UNION
SELECT trip_id,0 AS pr_dep,
       '->| '||
       trip||
       DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'',
              TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))||
       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'',
              TO_CHAR(scd,'(DD)')) AS str,
       NVL(act,NVL(est,scd)) AS act
FROM trips_in
WHERE TRUNC(NVL(act,NVL(est,scd)))-TRUNC(SYSDATE) BETWEEN -15 AND 1 AND
      status<>-1
UNION
SELECT -1,1,'═х юяЁхф.',TO_DATE(NULL) FROM dual
ORDER BY act


*/

void TSQL::setSQLTripList( TQuery &Qry, TReqInfo &info ) {
  Qry.Clear();
  TSQLParams p = Instance()->sqltrips[ info.screen.name ];
  string sql =
    "SELECT points.point_id, "
    "       points.airp, "
    "       system.AirpTZRegion(points.airp) AS tz_region, "
    "       points.airline, "
    "       points.flt_no, "
    "       points.suffix, "
    "       points.scd_out, "
    "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out ";
  sql+=
    "FROM " + p.sqlfrom;
  if (!info.user.access.airlines.empty())
    sql+=",aro_airlines";
  if (!info.user.access.airps.empty())
    sql+=",aro_airps";
  sql+="WHERE " + p.sqlwhere + " AND points.scd_out IS NOT NULL ";
  if (!info.user.access.airlines.empty())
    sql+="AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
      sql+="AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    else
      sql+="AND aro_airps.airp IN (points.airp,ckin.get_airp_arv(points.move_id,points.point_num)) AND "
           "aro_airps.aro_id=:user_id ";
  };
  sql+="ORDER BY real_out";
  Qry.SQLText = sql;
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  p.setVariables( Qry );
  if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
    Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
  if ( info.screen.name == "BRDBUS.EXE" )
    Qry.CreateVariable( "desk", otString, info.desk.code );
};

void TSQL::setSQLTripInfo( TQuery &Qry, TReqInfo &info ) {
  Qry.Clear();
  TSQLParams p = Instance()->sqltrips[ info.screen.name ];
  string sql=
    "SELECT points.point_id, "
    "       points.airline, "
    "       points.flt_no, "
    "       points.suffix, "
    "       points.craft, "
    "       points.airp, "
    "       system.AirpTZRegion(points.airp) AS tz_region, "
    "       points.scd_out, "
    "       points.act_out, "
    "       SUBSTR(ckin.get_classes(points.point_id),1,50) AS classes, "
    "       SUBSTR(ckin.get_airps(points.point_id),1,50) AS places, "
    "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out "
    "       points.trip_type, "
    "       points.litera, "
    "       points.remark, "
    "       points.pr_tranzit, ";
/*
  if ( info.screen.name == "BRDBUS.EXE" )
    sql+="gtimer.get_stage_time(points.point_id,:brd_close_stage_id) AS brd_to, "
         "gtimer.get_stage( points.point_id, :brd_stage_type ) as brd_stage ";

  if ( info.screen.name == "CENT.EXE" )
    sql+="gtimer.get_stage_time(points.point_id,:brd_close_stage_id) AS brd_to, "
         "gtimer.get_stage( points.point_id, :ckin_stage_type ) AS ckin_stage, "
         "gtimer.get_stage( points.point_id, :craft_stage_type ) AS craft_stage "
//       comp.pr_saloninit
  if ( info.screen.name == "AIR.EXE" )
    sql+="points.act_out, "
         "gtimer.get_stage_time(points.point_id,:ckin_close_stage_id) AS ckin_to, "
         "gtimer.get_stage_time(points.point_id,:brd_open_stage_id) AS brd_from, "
         "gtimer.get_stage_time(points.point_id,:brd_close_stage_id) AS brd_to, "
         "gtimer.get_stage( points.point_id, :ckin_stage_type ) as ckin_stage "
      //pr_tranzit_reg

  if ( info.screen.name == "DOCS.EXE" )
    sql+="gtimer.get_stage_time(points.point_id,:gangway_stage_id) AS ladder_to, "
         "DECODE(points.act_out,NULL,gtimer.get_stage( points.point_id, :ckin_stage_type ),
                                     :takeoff_stage_id) AS ckin_stage "
      //pr_tranzit_reg

  if ( info.screen.name == "KASSA.EXE" )
    sql+="gtimer.get_stage_time(points.point_id,:ckin_close_stage_id) AS ckin_to, "
         "gtimer.get_stage( points.point_id, :ckin_stage_type ) as ckin_stage "
  if ( info.screen.name == "PREPREG.EXE" );
    sql+="gtimer.get_stage_time(points.point_id,:ckin_close_stage_id) AS ckin_to, "
         "gtimer.get_stage( points.point_id, :ckin_stage_type ) as ckin_stage "
       //tranzitable
       //pr_tranzit
       //pr_saloninit
  if ( info.screen.name == "TLG.EXE" )
    sql+="DECODE(points.act_out,NULL,gtimer.get_stage( points.point_id, :craft_stage_type ),
                                     :takeoff_stage_id) AS craft_stage "*/
  sql+=
    "FROM " + p.sqlfrom;
  if (!info.user.access.airlines.empty())
    sql+=",aro_airlines";
  if (!info.user.access.airps.empty())
    sql+=",aro_airps";
  sql+="WHERE " + p.sqlwhere + " AND points.scd_out IS NOT NULL AND points.point_id=:point_id ";
  if (!info.user.access.airlines.empty())
    sql+="AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
      sql+="AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    else
      sql+="AND aro_airps.airp IN (points.airp,ckin.get_airp_arv(points.move_id,points.point_num)) AND "
           "aro_airps.aro_id=:user_id ";
  };
  Qry.SQLText = sql;
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  p.setVariables( Qry );
  if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
    Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
  if ( info.screen.name == "BRDBUS.EXE" )
    Qry.CreateVariable( "desk", otString, info.desk.code );
};

/*******************************************************************************/
void TripsInterface::GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "TripsInterface::ReadTrips" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr tripsNode = NewTextChild( dataNode, "trips" );
  //если по компаниям и портам полномочий нет - пустой список рейсов
  if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
      reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return;

  TQuery Qry( &OraSession );
  TSQL::setSQLTripList( Qry, *reqInfo );
  Qry.Execute();
  string scd_out,real_out;
  string desk_time=DateTimeToStr(reqInfo->desk.time,"dd");
  for(;!Qry.Eof;Qry.Next())
  {
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    scd_out= DateTimeToStr(UTCToClient(Qry.FieldAsDateTime("scd_out"),Qry.FieldAsString("tz_region")),"dd");
    real_out=DateTimeToStr(UTCToClient(Qry.FieldAsDateTime("real_out"),Qry.FieldAsString("tz_region")),"dd");
    ostringstream trip;
    trip << Qry.FieldAsString("airline")
         << Qry.FieldAsInteger("flt_no")
         << Qry.FieldAsString("suffix");
    if (desk_time!=real_out)
      trip << "/" << real_out;
    if (scd_out!=real_out)
      trip << "(" << scd_out << ")";
    if (!(reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.size()==1)||
        reqInfo->screen.name=="TLG.EXE")
      trip << " " << Qry.FieldAsString("airp");

    NewTextChild( tripNode, "trip_id", Qry.FieldAsInteger( "point_id" ) );
    NewTextChild( tripNode, "str", trip.str() );
  };
};

void TripsInterface::GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  int point_id = NodeAsInteger( "point_id", reqNode );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "point_id", point_id );
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
//    if ( GetNode( "tripheader", reqNode ) ) /* Считать заголовок */
//      readTripHeader( point_id, dataNode );
    if ( GetNode( "counters", reqNode ) ) /* Считать заголовок */
      BrdInterface::readTripCounters( point_id, dataNode );
  };
//    BrdInterface::Trip(ctxt,reqNode,resNode);
  if (reqInfo->screen.name == "CENT.EXE")
  {
//    if ( GetNode( "tripheader", reqNode ) ) /* Считать заголовок */
//      readTripHeader( point_id, dataNode );
    if ( GetNode( "counters", reqNode ) ) /* Считать заголовок */
      readTripCounters( point_id, dataNode );
  };
//    CentInterface::ReadTripInfo(ctxt,reqNode,resNode);
  if (reqInfo->screen.name == "PREPREG.EXE")
  {
//    if ( GetNode( "tripheader", reqNode ) ) /* Считать заголовок */
//      readTripHeader( point_id, dataNode );
    if ( GetNode( "tripcounters", reqNode ) )
      PrepRegInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "crsdata", reqNode ) )
      PrepRegInterface::readTripData( point_id, dataNode );
  };
//    PrepRegInterface::ReadTripInfo(ctxt,reqNode,resNode);

};

void TripsInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripHeader" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );
  //если по компаниям и портам полномочий нет - пустой список рейсов
  if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
      reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return;

  TQuery Qry( &OraSession );
  TSQL::setSQLTripInfo( Qry, *reqInfo );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof)
  {
    showErrorMessage( "Информация о рейсе недоступна" );
    return;
  };
  xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
  NewTextChild( node, "point_id", Qry.FieldAsInteger( "point_id" ) );
  NewTextChild( node, "airline", Qry.FieldAsString( "airline" ) );
  NewTextChild( node, "flt_no", Qry.FieldAsInteger( "flt_no" ) );
  NewTextChild( node, "suffix", Qry.FieldAsString( "suffix" ) );
  NewTextChild( node, "craft", Qry.FieldAsString( "craft" ) );
  NewTextChild( node, "airp", Qry.FieldAsString( "airp" ) );
  TDateTime scd_out,act_out,real_out;
  char *tz_region=Qry.FieldAsString("tz_region");
  scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
  real_out=UTCToClient(Qry.FieldAsDateTime("real_out"),tz_region);
  NewTextChild( node, "scd_out", DateTimeToStr(scd_out) );
  NewTextChild( node, "real_out", DateTimeToStr(real_out) );
  if (!Qry.FieldIsNULL("act_out"))
  {
    act_out= UTCToClient(Qry.FieldAsDateTime("act_out"),tz_region);
    NewTextChild( node, "act_out", DateTimeToStr(scd_out) );
  }
  else
  {
    act_out= 0;
    NewTextChild( node, "act_out" );
  };
  NewTextChild( node, "classes", Qry.FieldAsString( "classes" ) );
  NewTextChild( node, "places", Qry.FieldAsString( "places" ) );
  NewTextChild( node, "trip_type", Qry.FieldAsString( "trip_type" ) );
  NewTextChild( node, "litera", Qry.FieldAsString( "litera" ) );
  NewTextChild( node, "pr_tranzit", (int)Qry.FieldAsInteger( "pr_tranzit" )!=0 );
/*  NewTextChild( node, "", Qry.FieldAs( "" ) );
  NewTextChild( node, "", Qry.FieldAs( "" ) );
  NewTextChild( node, "", Qry.FieldAs( "" ) );
  NewTextChild( node, "", Qry.FieldAs( "" ) );
  NewTextChild( node, "", Qry.FieldAs( "" ) );*/

  TTripStages tripStages( point_id );
  TStagesRules *stagesRules = TStagesRules::Instance();

  //статусы рейсов
  string status;
  if ( reqInfo->screen.name == "BRDBUS.EXE" )
  {
    status = stagesRules->status( stBoarding, tripStages.getStage( stBoarding ) );
  };
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    status = stagesRules->status( stCheckIn, tripStages.getStage( stCheckIn ) );
  };
  if ( reqInfo->screen.name == "CENT.EXE" )
  {
    TStage ckin_stage =  tripStages.getStage( stCheckIn );
    TStage craft_stage = tripStages.getStage( stCraft );
    if ( craft_stage == sRemovalGangWay || craft_stage == sTakeoff )
      status = stagesRules->status( stCraft, craft_stage );
    else
      status = stagesRules->status( stCheckIn, ckin_stage );
  };
  if ( reqInfo->screen.name == "DOCS.EXE" )
  {
    if (act_out==0)
      status = stagesRules->status( stCheckIn, tripStages.getStage( stCheckIn ) );
    else
      status = stagesRules->status( stCheckIn, sTakeoff );
  };
  if ( reqInfo->screen.name == "TLG.EXE" )
  {
    if (act_out==0)
      status = stagesRules->status( stCraft, tripStages.getStage( stCraft ) );
    else
      status = stagesRules->status( stCraft, sTakeoff );
  };
  NewTextChild( node, "status", status );

  //всякие дополнительные времена
  TDateTime stage_time=0;
  if ( reqInfo->screen.name == "BRDBUS.EXE" ||
       reqInfo->screen.name == "CENT.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseBoarding ), tz_region );
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseCheckIn ), tz_region );
  if ( reqInfo->screen.name == "DOCS.EXE" )
    stage_time = UTCToClient( tripStages.time( sRemovalGangWay ), tz_region );
  if (stage_time!=0)
    NewTextChild( node, "stage_time", DateTimeToStr(stage_time) );

  //признак назначенного салона
  if ( reqInfo->screen.name == "CENT.EXE" ||
       reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    Qry.Clear();
    Qry.SQLText="SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    NewTextChild( node, "pr_saloninit", (int)(!Qry.Eof) );
  };


}


/*
----------------посадка----------------:
SELECT  trips.trip_id,
         trips.bc,
         SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
         SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
         TO_CHAR(gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id),'HH24:MI') AS brd_to,
         TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
         trips.triptype,
         trips.litera,
         stages.brd_stage,
         trips.remark
  FROM  trips,trip_stations,
   ( SELECT gtimer.get_stage( :trip_id, :brd_stage_type ) as brd_stage FROM dual ) stages
  WHERE trip_stations.trip_id= :trip_id AND trips.act IS NULL AND
        trip_stations.name= :station AND trip_stations.work_mode='╧' AND
        trips.trip_id= :trip_id AND
        trips.status=0 AND
        stages.brd_stage = :brd_open_stage_id

SELECT COUNT(*) AS reg,
       NVL(SUM(DECODE(pr_brd,0,0,1)),0) AS brd
FROM pax_grp,pax
WHERE pax_grp.grp_id=pax.grp_id AND point_id=:trip_id AND pr_brd IS NOT NULL

----------------центровка----------------:

SELECT  trips.trip_id,
         trips.bc,
         SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
         SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
         TO_CHAR(gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id),'HH24:MI') AS brd_to,
         TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
         trips.triptype,
         trips.litera,
         gtimer.get_stage( trips.trip_id, :ckin_stage_type ) AS ckin_stage,
         gtimer.get_stage( trips.trip_id, :craft_stage_type ) AS craft_stage,
         trips.remark,
         comp.pr_saloninit
  FROM  trips,
        (SELECT COUNT(*) AS pr_saloninit FROM trip_comp_elems
         WHERE trip_id=:trip_id AND rownum<2) comp
  WHERE trips.trip_id= :trip_id AND
        NVL(est,scd) BETWEEN SYSDATE-1 AND SYSDATE+1 AND act IS NULL AND
        trips.status=0


----------------регистрация----------------:
 SELECT  trips.trip_id,
         trips.trip,trips.flt_no,trips.scd,trips.act,trips.company,trips.bc,
         SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
         SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
         TO_CHAR(gtimer.get_stage_time(trips.trip_id,:ckin_close_stage_id),'HH24:MI') AS ckin_to,
         TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
         gtimer.get_stage_time(trips.trip_id,:brd_open_stage_id) AS brd_from,
         gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id) AS brd_to,
         trips.triptype,
         trips.litera,
         stages.ckin_stage,
         trips.remark,
         ckin.get_tranzit_reg(trips.trip_id) AS pr_tranzit_reg
  FROM  trips,
        ( SELECT gtimer.get_stage( :trip_id, :ckin_stage_type ) as ckin_stage FROM dual ) stages
  WHERE trips.trip_id= :trip_id AND
        trips.status=0 AND (act IS NULL OR :act_ignore<>0) AND
        stages.ckin_stage IN (:ckin_open_stage_id,:ckin_close_stage_id,:ckin_doc_stage_id)

SELECT  place.num,
        place.cod AS airp_cod,
        place.city AS city_cod,
        airps.name AS airp_name,
        cities.name AS city_name,
        remark
FROM place,airps,cities
WHERE place.cod=airps.cod AND place.city=cities.cod AND
      trip_id=:trip_id AND num>0
ORDER BY num

SELECT class AS cl_cod,name AS cl_name,cfg
FROM trip_classes,classes
WHERE classes.id=trip_classes.class AND trip_id= :trip_id
ORDER BY lvl

SELECT name AS brd_name FROM trip_stations
WHERE trip_id=:trip_id AND work_mode='╧'

SELECT point_num AS num, airps.city AS city_cod, class AS cl_cod,
       crs_ok-ok AS noshow,
       crs_tranzit-tranzit AS trnoshow,
       tranzit+ok+goshow AS show,
       free_ok,
       free_goshow,
       nooccupy
FROM counters2,airps
WHERE counters2.airp=airps.cod AND point_id=:trip_id



----------------документация----------------:
SELECT trips.trip_id,
       trips.trip,trips.scd,trips.bc,
       SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
       SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
       TO_CHAR(gtimer.get_stage_time(trips.trip_id,:gangway_stage_id),'HH24:MI') AS ladder_to,
       TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
       trips.triptype,
       trips.litera,
       DECODE(act,NULL,stages.ckin_stage,:takeoff_stage_id) AS ckin_stage,
       trips.remark,
       ckin.get_tranzit_reg(trips.trip_id) AS pr_tranzit_reg
FROM  trips,
      ( SELECT gtimer.get_stage( :trip_id, :ckin_stage_type ) as ckin_stage FROM dual ) stages
WHERE trips.trip_id= :trip_id AND
      trips.status=0 AND
      (act IS NOT NULL OR
       act IS NULL AND
       stages.ckin_stage NOT IN (:no_active_stage_id,:ckin_prep_stage_id))

----------------касса----------------:
SELECT trips.trip_id,trips.company,trips.flt_no,trips.suffix,trips.triptype,
       trips.bc,
       SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
       SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
       TO_CHAR(gtimer.get_stage_time(trips.trip_id,:ckin_close_stage_id),'HH24:MI') AS ckin_to,
       TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
       trips.triptype,
       trips.litera,
       stages.ckin_stage,
       trips.remark
FROM  trips,
      ( SELECT gtimer.get_stage( :trip_id, :ckin_stage_type ) as ckin_stage FROM dual ) stages
WHERE trips.trip_id= :trip_id AND trips.act IS NULL AND
      trips.status=0 AND
      stages.ckin_stage IN (:ckin_open_stage_id,:ckin_close_stage_id)


----------------подготовка----------------:
SELECT  trips.trip_id,
         trips.trip,trips.company,trips.flt_no,trips.bc,
         SUBSTR( ckin.get_classes( trips.trip_id ), 1, 255 ) AS classes,
         SUBSTR( ckin.get_places( trips.trip_id ), 1, 255 ) AS places,
         TO_CHAR(gtimer.get_stage_time(trips.trip_id,:ckin_close_stage_id),'HH24:MI') AS ckin_to,
         TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
         trips.triptype,
         trips.litera,
         stages.ckin_stage,
         trips.remark,
         DECODE(trips_in.trip_id,NULL,0,1) AS tranzitable,
         DECODE(trips.trip,trips_in.trip,1,0) AS pr_tranzit,
         comp.pr_saloninit
  FROM  trips,trips_in,
        ( SELECT gtimer.get_stage( :trip_id, :ckin_stage_type ) as ckin_stage FROM dual ) stages,
        (SELECT COUNT(*) AS pr_saloninit FROM trip_comp_elems
         WHERE trip_id=:trip_id AND rownum<2) comp
  WHERE trips.trip_id= :trip_id AND trips.act IS NULL AND
        trips.trip_id= trips_in.trip_id(+) AND
        trips_in.status(+)=0  AND
        trips.status=0 AND
        stages.ckin_stage NOT IN (:no_active_stage_id)


----------------телеграммы----------------:

SELECT trips.trip_id,
       trips.company,trips.flt_no,trips.suffix,trips.scd,trips.bc,
       SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes,
       SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places,
       TO_CHAR(NVL(NVL(trips.act,trips.est),trips.scd),'HH24:MI') AS takeoff,
       trips.triptype,
       trips.litera,
       DECODE(act,NULL,gtimer.get_stage( trips.trip_id, :craft_stage_type ),:takeoff_stage_id) AS craft_stage,
       trips.remark,
       1 AS pr_dep
FROM  trips
WHERE :pr_dep<>0 AND trips.trip_id= :trip_id AND
      TRUNC(NVL(trips.act,NVL(trips.est,trips.scd)))-TRUNC(SYSDATE) BETWEEN -15 AND 1 AND
      trips.status<>-1
UNION
SELECT trips_in.trip_id,
       trips_in.company,trips_in.flt_no,trips_in.suffix,trips_in.scd,trips_in.bc,
       TO_CHAR(NULL) AS classes,
       SUBSTR(ckin.get_places(trips_in.trip_id,0),1,255) AS places,
       TO_CHAR(NVL(NVL(trips_in.act,trips_in.est),trips_in.scd),'HH24:MI') AS takeoff,
       trips_in.triptype,
       trips_in.litera,
       TO_NUMBER(NULL),
       trips_in.remark,
       0 AS pr_dep
FROM  trips_in
WHERE :pr_dep=0 AND trips_in.trip_id= :trip_id AND
      TRUNC(NVL(trips_in.act,NVL(trips_in.est,trips_in.scd)))-TRUNC(SYSDATE) BETWEEN -15 AND 1 AND
      trips_in.status<>-1

SELECT num,cod FROM place
WHERE :pr_dep<>0 AND trip_id=:trip_id AND num>0
UNION
SELECT num,cod FROM place_in
WHERE :pr_dep=0 AND trip_id=:trip_id AND num<0
ORDER BY num

*/

void readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripsInterface::readTripCounters" );
  vector<TCounterItem> counters;
  /*считаем информацию по классам и п/н из Counters2 */
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT counters2.class, "\
                "       points.airp, "\
                "       trip_classes.cfg, "\
                "       counters2.crs_ok, "\
                "       counters2.crs_tranzit "\
                " FROM counters2,classes,trip_classes,points "\
                " WHERE counters2.class=classes.code AND "\
                "       counters2.point_dep=trip_classes.point_id AND "\
                "       counters2.class=trip_classes.class AND "\
                "       counters2.point_arv=points.point_id AND "\
                "       counters2.point_dep=:point_id "\
                " ORDER BY classes.priority,points.point_num ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  TCounterItem counterItem;
  while ( !Qry.Eof ) {
    counterItem.cl = Qry.FieldAsString( "class" );
    counterItem.target = Qry.FieldAsString( "airp" );
    counterItem.cfg = Qry.FieldAsInteger( "cfg" );
    counterItem.resa = Qry.FieldAsInteger( "crs_ok" );
    counterItem.tranzit = Qry.FieldAsInteger( "crs_tranzit" );
    counters.push_back( counterItem );
    Qry.Next();
  }

  /*считаем цифровую информацию о пассажирах и багаже по классам, п/н рейса, п/н трансфера и залам */
  Qry.Clear();
  Qry.SQLText = "SELECT a.class,a.airp_arv AS target,DECODE(a.last_trfer,' ',NULL,a.last_trfer) AS last_trfer, "\
                "       halls2.id AS hall_id,halls2.name AS hall_name, "\
                "       a.seats,a.adult,a.child,a.baby,b.foreigner,d.excess, "\
                "       b.umnr,b.vip,c.bagAmount,c.bagWeight,c.rkWeight "\
                " FROM halls2, "\
                "( SELECT class,airp_arv,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(seats) AS seats, "\
                "         SUM(DECODE(pers_type,'ВЗ',1,0)) AS adult, "\
                "         SUM(DECODE(pers_type,'РБ',1,0)) AS child, "\
                "         SUM(DECODE(pers_type,'РМ',1,0)) AS baby "\
                "   FROM pax_grp,pax,v_last_trfer "\
                "  WHERE pax_grp.grp_id=pax.grp_id AND "\
                "        pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        point_dep=:point_id AND pr_brd IS NOT NULL "\
                "  GROUP BY class,airp_arv,last_trfer,hall ) a, "\
                "( SELECT class,airp_arv,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(DECODE(rem_code,'UMNR',1,0)) AS umnr, "\
                "         SUM(DECODE(rem_code,'VIP',1,0)) AS vip, "\
                "         SUM(DECODE(rem_code,'FRGN',1,0)) AS foreigner "\
                "   FROM pax_grp,pax,v_last_trfer, "\
                "  (SELECT DISTINCT pax_id,rem_code FROM pax_rem "\
                "    WHERE rem_code IN ('UMNR','VIP','FRGN')) pax_rem "\
                "  WHERE pax_grp.grp_id=pax.grp_id AND "\
                "        pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        pax.pax_id=pax_rem.pax_id AND "\
                "        point_dep=:point_id AND pr_brd IS NOT NULL "\
                "  GROUP BY class,airp_arv,last_trfer,hall) b, "\
                "( SELECT class,airp_arv,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(DECODE(pr_cabin,0,amount,0)) AS bagAmount, "\
                "         SUM(DECODE(pr_cabin,0,weight,0)) AS bagWeight, "\
                "         SUM(DECODE(pr_cabin,0,0,weight)) AS rkWeight "\
                "   FROM pax_grp,v_last_trfer,bag2 "\
                "  WHERE pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        pax_grp.grp_id=bag2.grp_id AND "\
                "        point_dep=:point_id AND pr_refuse=0 "\
                "  GROUP BY class,airp_arv,last_trfer,hall) c, "\
                "(SELECT class,airp_arv,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "        SUM(excess) AS excess "\
                "  FROM pax_grp,v_last_trfer "\
                " WHERE pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "       point_dep=:point_id AND pr_refuse=0 "\
                " GROUP BY class,airp_arv,last_trfer,hall) d "\
                "WHERE a.hall=halls2.id AND "\
                "      a.class=b.class(+) AND "\
                "      a.airp_arv=b.airp_arv(+) AND "\
                "      a.last_trfer=b.last_trfer(+) AND "\
                "      a.hall=b.hall(+) AND "\
                "      a.class=c.class(+) AND "\
                "      a.airp_arv=c.airp_arv(+) AND "\
                "      a.last_trfer=c.last_trfer(+) AND "\
                "      a.hall=c.hall(+) AND "\
                "      a.class=d.class(+) AND "\
                "      a.airp_arv=d.airp_arv(+) AND "\
                "      a.last_trfer=d.last_trfer(+) AND "\
                "      a.hall=d.hall(+) ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  string cl,target;
  vector<TCounterItem>::iterator c;
  while ( !Qry.Eof ) {
    cl = Qry.FieldAsString( "class" );
    target = Qry.FieldAsString( "target" );
    for ( c=counters.begin(); c!=counters.end(); c++ ) {
      if ( c->cl == cl && c->target == target )
        break;
    }
    if ( c == counters.end() ) {
      TCounterItem counterItem;
      counterItem.cl = Qry.FieldAsString( "class" );
      counterItem.target = Qry.FieldAsString( "target" );
      counterItem.cfg = 0;
      counterItem.resa = 0;
      counterItem.tranzit = 0;
      c = counters.insert( counters.end(), counterItem );
    }
    TTrferItem TrferItem;
    TrferItem.last_trfer = Qry.FieldAsString( "last_trfer" );
    TrferItem.hall_id = Qry.FieldAsInteger( "hall_id" );
    TrferItem.hall_name = Qry.FieldAsString( "hall_name" );
    TrferItem.seats = Qry.FieldAsInteger( "seats" );
    TrferItem.adult = Qry.FieldAsInteger( "adult");
    TrferItem.child = Qry.FieldAsInteger( "child" );
    TrferItem.baby = Qry.FieldAsInteger( "baby" );
    TrferItem.foreigner = Qry.FieldAsInteger( "foreigner" );
    TrferItem.umnr = Qry.FieldAsInteger( "umnr" );
    TrferItem.vip = Qry.FieldAsInteger( "vip" );
    TrferItem.rkWeight = Qry.FieldAsInteger( "rkWeight" );
    TrferItem.bagAmount = Qry.FieldAsInteger( "bagAmount" );
    TrferItem.bagWeight = Qry.FieldAsInteger( "bagWeight" );
    TrferItem.excess = Qry.FieldAsInteger( "excess" );
    c->trfer.push_back( TrferItem );
    Qry.Next();
  }
  /* считаем информацию о досылаемом багаже по п/н рейса */
  Qry.Clear();
  Qry.SQLText = "SELECT airp_arv AS target,SUM(amount) AS amount,SUM(weight) AS weight "\
                " FROM unaccomp_bag WHERE point_dep=:point_id "\
                " GROUP BY airp_arv ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    target = Qry.FieldAsString( "target" );
    for ( c=counters.begin(); c!=counters.end(); c++ ) {
      if ( c->cl.empty() && c->target == target )
        break;
    }
    if ( c == counters.end() ) {
      TCounterItem counterItem;
      counterItem.target = Qry.FieldAsString( "target" );
      counterItem.cfg = 0;
      counterItem.resa = 0;
      counterItem.tranzit = 0;
      c = counters.insert( counters.end(), counterItem );
    }
    TTrferItem TrferItem;
    TrferItem.seats = 0;
    TrferItem.adult = 0;
    TrferItem.child = 0;
    TrferItem.baby = 0;
    TrferItem.foreigner = 0;
    TrferItem.umnr = 0;
    TrferItem.vip = 0;
    TrferItem.rkWeight = 0;
    TrferItem.bagAmount = Qry.FieldAsInteger( "bagAmount" );
    TrferItem.bagWeight = Qry.FieldAsInteger( "bagWeight" );
    TrferItem.excess = 0;
    c->trfer.push_back( TrferItem );
    Qry.Next();
  }
  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  xmlNodePtr counterItemNode, TrferNode, TrferItemNode;
  for ( vector<TCounterItem>::iterator c=counters.begin(); c!=counters.end(); c++ ) {
    counterItemNode = NewTextChild( node, "counteritem" );
    NewTextChild( counterItemNode, "cl",  c->cl );
    NewTextChild( counterItemNode, "target", c->target );
    NewTextChild( counterItemNode, "cfg", c->cfg );
    NewTextChild( counterItemNode, "resa", c->resa );
    NewTextChild( counterItemNode, "tranzit", c->tranzit );
    TrferNode = NewTextChild( counterItemNode, "trfer" );
    for ( vector<TTrferItem>::iterator trfer=c->trfer.begin(); trfer!=c->trfer.end(); trfer++ ) {
      TrferItemNode = NewTextChild( TrferNode, "trferitemnode" );
      NewTextChild( TrferItemNode, "last_trfer", trfer->last_trfer );
      NewTextChild( TrferItemNode, "hall_id", trfer->hall_id );
      NewTextChild( TrferItemNode, "hall_name", trfer->hall_name );
      NewTextChild( TrferItemNode, "seats", trfer->seats );
      NewTextChild( TrferItemNode, "adult", trfer->adult );
      NewTextChild( TrferItemNode, "child", trfer->child );
      NewTextChild( TrferItemNode, "baby", trfer->baby );
      NewTextChild( TrferItemNode, "foreigner", trfer->foreigner );
      NewTextChild( TrferItemNode, "umnr", trfer->umnr );
      NewTextChild( TrferItemNode, "vip", trfer->vip );
      NewTextChild( TrferItemNode, "rkweight", trfer->rkWeight );
      NewTextChild( TrferItemNode, "bagamount", trfer->bagAmount );
      NewTextChild( TrferItemNode, "bagweight", trfer->bagWeight );
      NewTextChild( TrferItemNode, "excess", trfer->excess );
    }
  }
}

void viewPNL( int point_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  Qry.SQLText =
    "SELECT pnr_ref, "\
    "       RTRIM(surname||' '||name) full_name, "\
    "       pers_type, "\
    "       class, "\
    "       seat_no, "\
    "       target, "\
    "       report.get_trfer_airp(airp_arv) AS last_target, "\
    "       report.get_PSPT(pax_id) AS document, "\
    "       pax_id, "\
    "       crs_pnr.pnr_id "\
    " FROM crs_pnr,tlg_binding,crs_pax,v_last_crs_trfer "\
    "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND "\
    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "\
    "      crs_pnr.pnr_id=v_last_crs_trfer.pnr_id(+) AND "\
    "      crs_pax.pr_del=0 "\
    "ORDER BY DECODE(pnr_ref,NULL,0,1),pnr_ref,pnr_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  RQry.SQLText =
    "SELECT crs_pax_rem.rem, crs_pax_rem.rem_code, NVL(rem_types.priority,-1) AS priority "\
    " FROM crs_pax_rem,rem_types "\
    "WHERE crs_pax_rem.rem_code=rem_types.code(+) AND crs_pax_rem.pax_id=:pax_id "\
    "ORDER BY priority DESC,rem_code,rem ";
  RQry.DeclareVariable( "pax_id", otInteger );

  dataNode = NewTextChild( dataNode, "trippnl" );
  while ( !Qry.Eof ) {
    xmlNodePtr itemNode = NewTextChild( dataNode, "item" );
    NewTextChild( itemNode, "pnr_ref", Qry.FieldAsString( "pnr_ref" ) );
    NewTextChild( itemNode, "full_name", Qry.FieldAsString( "full_name" ) );
    NewTextChild( itemNode, "pers_type", Qry.FieldAsString( "pers_type" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "seat_no", Qry.FieldAsString( "seat_no" ) );
    NewTextChild( itemNode, "target", Qry.FieldAsString( "target" ) );
    NewTextChild( itemNode, "last_target", Qry.FieldAsString( "last_target" ) );
    RQry.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    RQry.Execute();
    string rem, rem_code, rcode, ticket;
    while ( !RQry.Eof ) {
      rem += string( ".R/" ) + RQry.FieldAsString( "rem" ) + "   ";
      rem_code = RQry.FieldAsString( "rem_code" );
      rcode = rem_code;
      rcode = upperc( rcode );
      if ( rcode.find( "TKNO" ) != string::npos  && ticket.empty() ) {
      	ticket = RQry.FieldAsString( "rem" );
      }
      RQry.Next();
    }
    NewTextChild( itemNode, "ticket", ticket );
    NewTextChild( itemNode, "document", Qry.FieldAsString( "document" ) );
    NewTextChild( itemNode, "rem", rem );
    NewTextChild( itemNode, "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    NewTextChild( itemNode, "pnr_id", Qry.FieldAsInteger( "pnr_id" ) );
    Qry.Next();
  }
}



