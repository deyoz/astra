#include <stdlib.h>
#include <string>
#include "tripinfo.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "brd.h"
#include "checkin.h"
#include "prepreg.h"
#include "telegram.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

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
  /* ------------ДОСМОТР-------------- */
  /* задаем текст */
  p.sqlfrom =
    "points ";
  p.sqlwhere =
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "gtimer.is_final_stage(  points.point_id, :brd_stage_type, :brd_open_stage_id) <> 0 ";
  /* задаем переменные */
  p.addVariable( "brd_stage_type", otInteger, IntToString( stBoarding ) );
  p.addVariable( "brd_open_stage_id", otInteger, IntToString( sOpenBoarding ) );
  /* запоминаем */
  sqltrips[ "BRDBUS.EXE" ] = p;
  sqltrips[ "EXAM.EXE" ] = p;
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
    "(gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_open_stage_id)<>0 OR "
    " gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_close_stage_id)<>0 OR "
    " gtimer.is_final_stage(points.point_id, :ckin_stage_type, :ckin_doc_stage_id)<>0) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sCloseBoarding ) );
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
    "       points.airline, "
    "       points.flt_no, "
    "       points.suffix, "
    "       points.scd_out, "
    "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out ";
  sql+=
    "FROM " + p.sqlfrom;

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
    sql+=",trip_stations";
  if (!info.user.access.airlines.empty())
    sql+=",aro_airlines";
  if (!info.user.access.airps.empty())
    sql+=",aro_airps";
  sql+=" WHERE " + p.sqlwhere + " AND pr_reg<>0 ";

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
    sql+="AND points.point_id=trip_stations.point_id "
         "AND trip_stations.desk= :desk AND trip_stations.work_mode=:work_mode ";

  if ( info.screen.name == "AIR.EXE" )
  {
    vector<int>::iterator i;
    for(i=info.user.access.rights.begin();i!=info.user.access.rights.end();i++)
      if (*i==320||*i==330) break;
    if (i==info.user.access.rights.end())
      sql+="AND points.act_out IS NULL ";
  };

  if (!info.user.access.airlines.empty())
    sql+="AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
      sql+="AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    else
      sql+="AND aro_airps.airp IN "
           "     (points.airp,ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point), "
           "                                 points.point_num)) AND "
           "aro_airps.aro_id=:user_id ";
  };
  sql+="ORDER BY TRUNC(real_out) DESC,flt_no,airline, "
       "         NVL(suffix,' '),move_id,point_num";
  Qry.SQLText = sql;
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  p.setVariables( Qry );
  if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
    Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
  {
    Qry.CreateVariable( "desk", otString, info.desk.code );
    if (info.screen.name == "BRDBUS.EXE")
      Qry.CreateVariable( "work_mode", otString, "П");
    else
      Qry.CreateVariable( "work_mode", otString, "Р");
  };

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
    "       points.scd_out, "
    "       points.act_out, "
    "       points.bort, "
    "       points.park_out, "
    "       SUBSTR(ckin.get_classes(points.point_id),1,50) AS classes, "
    "       SUBSTR(ckin.get_airps(points.point_id),1,50) AS route, "
    "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
    "       points.trip_type, "
    "       points.litera, "
    "       points.remark, "
    "       points.pr_tranzit, "
    "       points.first_point ";

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
  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
    sql+=",trip_stations";
  if (!info.user.access.airlines.empty())
    sql+=",aro_airlines";
  if (!info.user.access.airps.empty())
    sql+=",aro_airps";
  sql+=" WHERE " + p.sqlwhere + " AND pr_reg<>0 AND points.point_id=:point_id ";

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
    sql+="AND points.point_id=trip_stations.point_id "
         "AND trip_stations.desk= :desk AND trip_stations.work_mode=:work_mode ";

  if ( info.screen.name == "AIR.EXE" )
  {
    vector<int>::iterator i;
    for(i=info.user.access.rights.begin();i!=info.user.access.rights.end();i++)
      if (*i==320||*i==330) break;
    if (i==info.user.access.rights.end())
      sql+="AND points.act_out IS NULL ";
  };

  if (!info.user.access.airlines.empty())
    sql+="AND aro_airlines.airline=points.airline AND aro_airlines.aro_id=:user_id ";
  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
      sql+="AND aro_airps.airp=points.airp AND aro_airps.aro_id=:user_id ";
    else
      sql+="AND aro_airps.airp IN "
           "     (points.airp,ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point), "
           "                                 points.point_num)) AND "
           "aro_airps.aro_id=:user_id ";
  };
  Qry.SQLText = sql;
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  p.setVariables( Qry );
  if (!info.user.access.airlines.empty() || !info.user.access.airps.empty())
    Qry.CreateVariable( "user_id", otInteger, info.user.user_id );
  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport)
  {
    Qry.CreateVariable( "desk", otString, info.desk.code );
    if (info.screen.name == "BRDBUS.EXE")
      Qry.CreateVariable( "work_mode", otString, "П");
    else
      Qry.CreateVariable( "work_mode", otString, "Р");
  };
};

/*******************************************************************************/
void TripsInterface::GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "TripsInterface::GetTripList" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr tripsNode = NewTextChild( dataNode, "trips" );
  xmlNodePtr tripNode;
  //если по компаниям и портам полномочий нет - пустой список рейсов
  if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
      reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return;

  if (reqInfo->screen.name=="TLG.EXE")
  {
    tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "trip_id", -1 );
    NewTextChild( tripNode, "str", "Непривязанные" );
  };

  TQuery Qry( &OraSession );
  TSQL::setSQLTripList( Qry, *reqInfo );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    tripNode = NewTextChild( tripsNode, "trip" );
    TTripInfo info(Qry);

    NewTextChild( tripNode, "trip_id", Qry.FieldAsInteger( "point_id" ) );
    NewTextChild( tripNode, "str", GetTripName(info,reqInfo->screen.name=="TLG.EXE") );
  };
};

void TripsInterface::GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  int point_id = NodeAsInteger( "point_id", reqNode );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "point_id", point_id );

  if ( GetNode( "tripheader", reqNode ) )
    if ( !readTripHeader( point_id, dataNode ) )
      showErrorMessage( "Информация о рейсе недоступна" );

  if (reqInfo->screen.name == "BRDBUS.EXE" ||
      reqInfo->screen.name == "EXAM.EXE" )
  {
    if ( GetNode( "counters", reqNode ) )
      BrdInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "tripdata", reqNode ) )
      BrdInterface::readTripData( point_id, dataNode );
    if ( GetNode( "paxdata", reqNode ) )
      BrdInterface::GetPax(reqNode,resNode);
  };
  if (reqInfo->screen.name == "AIR.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      CheckInInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "tripdata", reqNode ) )
      CheckInInterface::readTripData( point_id, dataNode );
  };
  if (reqInfo->screen.name == "CENT.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) ) {
      readPaxLoad( point_id, reqNode, dataNode ); //djek
    }
  };
  if (reqInfo->screen.name == "PREPREG.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      PrepRegInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "crsdata", reqNode ) )
      PrepRegInterface::readTripData( point_id, dataNode );
  };
  if (reqInfo->screen.name == "TLG.EXE")
  {
    if ( GetNode( "tripdata", reqNode ) && point_id != -1 )
      TelegramInterface::readTripData( point_id, dataNode );
  };

};
bool TripsInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripsInterface::readTripHeader" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );
  //если по компаниям и портам полномочий нет - пустой список рейсов
  if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
      reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return false;

  if (reqInfo->screen.name=="TLG.EXE" && point_id==-1)
  {
    xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
    NewTextChild( node, "point_id", -1 );
    return true;
  };
  TQuery Qry( &OraSession );
  TSQL::setSQLTripInfo( Qry, *reqInfo );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof) return false;
  xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
  NewTextChild( node, "point_id", Qry.FieldAsInteger( "point_id" ) );
  NewTextChild( node, "airline", Qry.FieldAsString( "airline" ) );
  NewTextChild( node, "flt_no", Qry.FieldAsInteger( "flt_no" ) );
  NewTextChild( node, "suffix", Qry.FieldAsString( "suffix" ) );
  NewTextChild( node, "craft", Qry.FieldAsString( "craft" ) );
  NewTextChild( node, "airp", Qry.FieldAsString( "airp" ) );
  TDateTime scd_out,act_out,real_out;
  string &tz_region=AirpTZRegion(Qry.FieldAsString( "airp" ));
  scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
  real_out=UTCToClient(Qry.FieldAsDateTime("real_out"),tz_region);
  NewTextChild( node, "scd_out", DateTimeToStr(scd_out) );
  NewTextChild( node, "real_out", DateTimeToStr(real_out,"hh:nn") );
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
  NewTextChild( node, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( node, "park", Qry.FieldAsString( "park_out" ) );
  NewTextChild( node, "classes", Qry.FieldAsString( "classes" ) );
  NewTextChild( node, "route", Qry.FieldAsString( "route" ) );
  NewTextChild( node, "places", Qry.FieldAsString( "route" ) );
  NewTextChild( node, "trip_type", Qry.FieldAsString( "trip_type" ) );
  NewTextChild( node, "litera", Qry.FieldAsString( "litera" ) );
  NewTextChild( node, "remark", Qry.FieldAsString( "remark" ) );
  NewTextChild( node, "pr_tranzit", (int)Qry.FieldAsInteger( "pr_tranzit" )!=0 );

  TTripInfo info(Qry);
  NewTextChild( node, "trip", GetTripName(info,reqInfo->screen.name=="TLG.EXE") );

  TTripStages tripStages( point_id );
  TStagesRules *stagesRules = TStagesRules::Instance();

  //статусы рейсов
  string status;
  if ( reqInfo->screen.name == "BRDBUS.EXE" ||
       reqInfo->screen.name == "EXAM.EXE")
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
       reqInfo->screen.name == "EXAM.EXE" ||
       reqInfo->screen.name == "CENT.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseBoarding ), tz_region );
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseCheckIn ), tz_region );
  if ( reqInfo->screen.name == "DOCS.EXE" )
    stage_time = UTCToClient( tripStages.time( sRemovalGangWay ), tz_region );
  if (stage_time!=0)
    NewTextChild( node, "stage_time", DateTimeToStr(stage_time,"hh:nn") );

  //признак назначенного салона
  if ( reqInfo->screen.name == "CENT.EXE" ||
       reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText="SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.Execute();
    NewTextChild( node, "pr_saloninit", (int)(!Qryh.Eof) );
  };

  if (reqInfo->screen.name == "CENT.EXE" )
  {
    NewTextChild( node, "craft_stage", tripStages.getStage( stCraft ) );
  };

  if (reqInfo->screen.name == "AIR.EXE" ||
      reqInfo->screen.name == "DOCS.EXE" ||
      reqInfo->screen.name == "PREPREG.EXE")
  {
    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "ckin_stage", tripStages.getStage( stCheckIn ) );
      NewTextChild( node, "tranzitable", (int)(!Qry.FieldIsNULL("first_point")) );
    };

    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText=
      "SELECT NVL(pr_tranz_reg,0) AS pr_tranz_reg "
      "FROM trip_sets WHERE point_id=:point_id ";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.Execute();
    if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
    NewTextChild( node, "pr_tranz_reg", (int)(Qryh.FieldAsInteger("pr_tranz_reg")!=0) );
  };
  return true;
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

string convertLastTrfer(string s)
{
  string res;
  string::size_type i;
  if ((i=s.find('/'))!=string::npos)
    res=s.substr(i+1)+'('+s.substr(0,i)+')';
  else
    res=s;
  return res;
};

void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  reqNode=GetNode("tripcounters",reqNode);
  if (reqNode==NULL) return;
  xmlNodePtr node=NodeAsNode("fields",reqNode);
  resNode=NewTextChild(resNode,"tripcounters");
  xmlNodePtr node2=NewTextChild(resNode,"fields");
  xmlNodePtr remNode=NULL;
  bool pr_class=false,
       pr_cl_grp=false,
       pr_hall=false,
       pr_airp_arv=false,
       pr_trfer=false,
       pr_user=false,
       pr_rems=false;
  ostringstream order_by;
  NewTextChild(node2,"field","title");
  for(node=node->children;node!=NULL;node=node->next)
  {

    if (strcmp((char*)node->name,"class")==0)
    {
      pr_class=true;
      order_by << ", classes.priority";
    };
    if (strcmp((char*)node->name,"cl_grp")==0)
    {
      pr_cl_grp=true;
      order_by << ", cls_grp.priority";
    };
    if (strcmp((char*)node->name,"hall")==0)
    {
      pr_hall=true;
      order_by << ", halls2.name";
    };
    if (strcmp((char*)node->name,"airp_arv")==0)
    {
      pr_airp_arv=true;
      order_by << ", points.point_num";
    };
    if (strcmp((char*)node->name,"trfer")==0)
    {
      pr_trfer=true;
      order_by << ", a.last_trfer";
    };
    if (strcmp((char*)node->name,"user")==0)
    {
      pr_user=true;
      order_by << ", users2.descr,a.user_id";
    };
    if (strcmp((char*)node->name,"rems")==0)
    {
      if (node->children!=NULL)
      {
        remNode=node;
        pr_rems=true;
        order_by << ", a.rem_code";
      }
      else
        continue;
    };
    NewTextChild(node2,"field",(char*)node->name);
  };
  NewTextChild(node2,"field","cfg");
  NewTextChild(node2,"field","crs_ok");
  NewTextChild(node2,"field","crs_tranzit");
  NewTextChild(node2,"field","seats");
  NewTextChild(node2,"field","adult");
  NewTextChild(node2,"field","child");
  NewTextChild(node2,"field","baby");
  NewTextChild(node2,"field","rk_weight");
  NewTextChild(node2,"field","bag_amount");
  NewTextChild(node2,"field","bag_weight");
  NewTextChild(node2,"field","excess");

  node2=NewTextChild(resNode,"rows");

  //строка 'итого'
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT a.seats,a.adult,a.child,a.baby, "
    "       b.bag_amount+d.bag_amount AS bag_amount, "
    "       b.bag_weight+d.bag_weight AS bag_weight, "
    "       b.rk_weight, "
    "       c.crs_ok,c.crs_tranzit, "
    "       e.excess,f.cfg "
    "FROM "
    " (SELECT NVL(SUM(seats),0) AS seats, "
    "         NVL(SUM(DECODE(pers_type,'ВЗ',1,0)),0) AS adult, "
    "         NVL(SUM(DECODE(pers_type,'РБ',1,0)),0) AS child, "
    "         NVL(SUM(DECODE(pers_type,'РМ',1,0)),0) AS baby "
    "  FROM pax_grp,pax "
    "  WHERE pax_grp.grp_id=pax.grp_id AND "
    "        point_dep=:point_id AND pr_brd IS NOT NULL) a, "
    " (SELECT NVL(SUM(DECODE(pr_cabin,0,amount,0)),0) AS bag_amount, "
    "         NVL(SUM(DECODE(pr_cabin,0,weight,0)),0) AS bag_weight, "
    "         NVL(SUM(DECODE(pr_cabin,0,0,weight)),0) AS rk_weight "
    "  FROM pax_grp,bag2 "
    "  WHERE pax_grp.grp_id=bag2.grp_id AND "
    "        point_dep=:point_id AND pr_refuse=0) b, "
    " (SELECT NVL(SUM(crs_ok),0) AS crs_ok, "
    "         NVL(SUM(crs_tranzit),0) AS crs_tranzit "
    "  FROM counters2 "
    "  WHERE point_dep=:point_id) c, "
    " (SELECT NVL(SUM(amount),0) AS bag_amount, "
    "         NVL(SUM(weight),0) AS bag_weight "
    "  FROM unaccomp_bag "
    "  WHERE point_dep=:point_id) d, "
    " (SELECT NVL(SUM(excess),0) AS excess "
    "  FROM pax_grp "
    "  WHERE point_dep=:point_id AND pr_refuse=0) e, "
    " (SELECT NVL(SUM(cfg),0) AS cfg "
    "  FROM trip_classes "
    "  WHERE point_id=:point_id) f";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");

  xmlNodePtr rowNode=NewTextChild(node2,"row");
  NewTextChild(rowNode,"title","Всего");
  NewTextChild(rowNode,"seats",Qry.FieldAsInteger("seats"),0);
  NewTextChild(rowNode,"adult",Qry.FieldAsInteger("adult"),0);
  NewTextChild(rowNode,"child",Qry.FieldAsInteger("child"),0);
  NewTextChild(rowNode,"baby",Qry.FieldAsInteger("baby"),0);
  NewTextChild(rowNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
  NewTextChild(rowNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
  NewTextChild(rowNode,"rk_weight",Qry.FieldAsInteger("rk_weight"),0);
  NewTextChild(rowNode,"crs_ok",Qry.FieldAsInteger("crs_ok"),0);
  NewTextChild(rowNode,"crs_tranzit",Qry.FieldAsInteger("crs_tranzit"),0);
  NewTextChild(rowNode,"excess",Qry.FieldAsInteger("excess"),0);
  NewTextChild(rowNode,"cfg",Qry.FieldAsInteger("cfg"),0);

  //строка select для подзапросов
  ostringstream select;
  if (pr_class) select << ", class";
  if (pr_cl_grp) select << ", class_grp";
  if (pr_hall) select << ", hall";
  if (pr_airp_arv) select << ", point_arv";
  if (pr_trfer) select << ", NVL(last_trfer,' ') AS last_trfer";
  if (pr_user) select << ", user_id";
  if (pr_rems) select << ", rem_code";

  //строка group by для подзапросов
  ostringstream group_by;
  if (pr_class) group_by << ", class";
  if (pr_cl_grp) group_by << ", class_grp";
  if (pr_hall) group_by << ", hall";
  if (pr_airp_arv) group_by << ", point_arv";
  if (pr_trfer) group_by << ", NVL(last_trfer,' ')";
  if (pr_user) group_by << ", user_id";
  if (pr_rems) group_by << ", rem_code";

  if (group_by.str().empty()) return;

  ostringstream sql;
  sql << "SELECT a.seats,a.adult,a.child,a.baby" << endl;

  if (!pr_rems)
  {
   /* if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user && !pr_class)
      sql << ",NVL(b.bag_amount,0)+NVL(d.bag_amount,0) AS bag_amount" << endl
          << ",NVL(b.bag_weight,0)+NVL(d.bag_weight,0) AS bag_weight" << endl
          << ",b.rk_weight,e.excess" << endl;
    else*/
      sql << ",b.bag_amount,b.bag_weight,b.rk_weight,e.excess" << endl;

    if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user)
    {
      sql << ",c.crs_ok,c.crs_tranzit" << endl;
      if (!pr_airp_arv) sql << ",f.cfg" << endl;
    };
  };

  if (pr_class)    sql << ",a.class" << endl;
  if (pr_cl_grp)   sql << ",a.class_grp AS cl_grp_id,cls_grp.code AS cl_grp_code" << endl;
  if (pr_hall)     sql << ",a.hall AS hall_id,halls2.name AS hall_name" << endl;
  if (pr_airp_arv) sql << ",a.point_arv,points.airp AS airp_arv" << endl;
  if (pr_trfer)    sql << ",DECODE(a.last_trfer,' ',NULL,a.last_trfer) AS last_trfer" << endl;
  if (pr_user)     sql << ",a.user_id,users2.descr AS user_descr" << endl;
  if (pr_rems)     sql << ",a.rem_code" << endl;

  sql << "FROM" << endl
      << "(SELECT SUM(seats) AS seats, " << endl
      << "        SUM(DECODE(pers_type,'ВЗ',1,0)) AS adult, " << endl
      << "        SUM(DECODE(pers_type,'РБ',1,0)) AS child, " << endl
      << "        SUM(DECODE(pers_type,'РМ',1,0)) AS baby, " << endl
      << "        " << select.str().erase(0,1) << endl;

  sql << "FROM pax_grp,pax ";
  if (pr_trfer) sql << ",v_last_trfer";
  if (pr_rems)
  {
    sql << endl
        << ",(SELECT DISTINCT pax_id,rem_code FROM pax_rem" << endl
        << "  WHERE rem_code IN ( ";
    for(node=remNode->children;node!=NULL;node=node->next)
    {
      sql << "'" << NodeAsString(node) << "'";
      if (node->next!=NULL) sql << ", ";
    };
    sql << ")) rems" << endl;
  }
  else sql << endl;

  sql << "WHERE pax_grp.grp_id=pax.grp_id AND " << endl
      << "      point_dep=:point_id AND pr_brd IS NOT NULL " << endl;
  if (pr_trfer) sql << "AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;
  if (pr_rems) sql << "AND pax.pax_id=rems.pax_id " << endl;

  sql << "GROUP BY " << group_by.str().erase(0,1) << ") a" << endl;

  if (!pr_rems)
  {
    //запрос по багажу
    sql << ",(SELECT SUM(DECODE(pr_cabin,0,amount,0)) AS bag_amount, " << endl
        << "         SUM(DECODE(pr_cabin,0,weight,0)) AS bag_weight, " << endl
        << "         SUM(DECODE(pr_cabin,0,0,weight)) AS rk_weight, " << endl
        << "         " << select.str().erase(0,1) << endl;

    sql << "FROM pax_grp,bag2 ";
    if (pr_trfer) sql << ",v_last_trfer";

    sql << endl
        << "WHERE pax_grp.grp_id=bag2.grp_id AND " << endl
        << "      point_dep=:point_id AND pr_refuse=0 " << endl;
    if (pr_trfer) sql << "AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;

    sql << "GROUP BY " << group_by.str().erase(0,1) << ") b" << endl;

    //запрос по платному багажу
    sql << ",(SELECT SUM(excess) AS excess, " << endl
        << "         " << select.str().erase(0,1) << endl;

    sql << "FROM pax_grp ";
    if (pr_trfer) sql << ",v_last_trfer";

    sql << endl
        << "WHERE point_dep=:point_id AND pr_refuse=0 " << endl;
    if (pr_trfer) sql << "AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;

    sql << "GROUP BY " << group_by.str().erase(0,1) << ") e" << endl;


    if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user)
    {
      //запрос по брони
      sql << ",(SELECT SUM(crs_ok) AS crs_ok, " << endl
          << "         SUM(crs_tranzit) AS crs_tranzit, " << endl
          << "         " << select.str().erase(0,1) << endl
          << "  FROM counters2 " << endl
          << "  WHERE point_dep=:point_id " << endl
          << "  GROUP BY " << group_by.str().erase(0,1) << ") c" << endl;

    /*  if (!pr_class)
      {
        //запрос по досылаемому багажу
        sql << ",(SELECT SUM(amount) AS bag_amount, " << endl
            << "         SUM(weight) AS bag_weight, " << endl
            << "         " << select.str().erase(0,1) << endl
            << "  FROM unaccomp_bag " << endl
            << "  WHERE point_dep=:point_id " << endl
            << "  GROUP BY " << group_by.str().erase(0,1) << ") d" << endl;
      };*/

      if (!pr_airp_arv)
      {
        //запрос по компоновке
        sql << ",(SELECT SUM(cfg) AS cfg," << select.str().erase(0,1) << endl
            << "  FROM trip_classes " << endl
            << "  WHERE point_id=:point_id " << endl
            << "  GROUP BY " << group_by.str().erase(0,1) << ") f" << endl;
      };
    };
  };

  if (pr_class) sql << ",classes";
  if (pr_cl_grp) sql << ",cls_grp";
  if (pr_hall) sql << ",halls2";
  if (pr_airp_arv) sql << ",points";
  if (pr_user) sql << ",users2";

  sql << endl;

  ostringstream where;
  if (pr_class)    where << " AND a.class=classes.code" << endl;
  if (pr_cl_grp)   where << " AND a.class_grp=cls_grp.id" << endl;
  if (pr_hall)     where << " AND a.hall=halls2.id" << endl;
  if (pr_airp_arv) where << " AND a.point_arv=points.point_id" << endl;
  if (pr_user)     where << " AND a.user_id=users2.user_id" << endl;

  if (!pr_rems)
  {
    if (pr_class)    where << " AND a.class=b.class(+) AND a.class=e.class(+)" << endl;
    if (pr_cl_grp)   where << " AND a.class_grp=b.class_grp(+) AND a.class_grp=e.class_grp(+)" << endl;
    if (pr_hall)     where << " AND a.hall=b.hall(+) AND a.hall=e.hall(+)" << endl;
    if (pr_airp_arv) where << " AND a.point_arv=b.point_arv(+) AND a.point_arv=e.point_arv(+)" << endl;
    if (pr_trfer)    where << " AND a.last_trfer=b.last_trfer(+) AND a.last_trfer=e.last_trfer(+)" << endl;
    if (pr_user)     where << " AND a.user_id=b.user_id(+) AND a.user_id=e.user_id(+)" << endl;

    if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user)
    {
      if (pr_class)    where << " AND a.class=c.class(+)" << endl;
      if (pr_airp_arv) where << " AND a.point_arv=c.point_arv(+)" << endl;
      /*if (!pr_class)    where << " AND a.point_arv=d.point_arv(+)" << endl;*/
      if (!pr_airp_arv) where << " AND a.class=f.class(+)" << endl;
    };
  };

  if (!where.str().empty())
  {
    //удалим первый AND
    sql << "WHERE " << where.str().erase(0,4);
  };

  if (!order_by.str().empty())
  {
    sql << "ORDER BY " << order_by.str().erase(0,1);
  };

  ProgTrace(TRACE5,"SQL=%s",sql.str().c_str());

  Qry.Clear();
  Qry.SQLText = sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();

  for(;!Qry.Eof;Qry.Next())
  {
    rowNode=NewTextChild(node2,"row");
    NewTextChild(rowNode,"seats",Qry.FieldAsInteger("seats"),0);
    NewTextChild(rowNode,"adult",Qry.FieldAsInteger("adult"),0);
    NewTextChild(rowNode,"child",Qry.FieldAsInteger("child"),0);
    NewTextChild(rowNode,"baby",Qry.FieldAsInteger("baby"),0);

    if (!pr_rems)
    {
      NewTextChild(rowNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
      NewTextChild(rowNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
      NewTextChild(rowNode,"rk_weight",Qry.FieldAsInteger("rk_weight"),0);
      NewTextChild(rowNode,"excess",Qry.FieldAsInteger("excess"),0);

      if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user)
      {
        NewTextChild(rowNode,"crs_ok",Qry.FieldAsInteger("crs_ok"),0);
        NewTextChild(rowNode,"crs_tranzit",Qry.FieldAsInteger("crs_tranzit"),0);
        if (!pr_airp_arv)
          NewTextChild(rowNode,"cfg",Qry.FieldAsInteger("cfg"),0);
      };
    };

    if (pr_class)
    {
      NewTextChild(rowNode,"class",Qry.FieldAsString("class"));
    };
    if (pr_cl_grp)
    {
      node=NewTextChild(rowNode,"cl_grp",Qry.FieldAsString("cl_grp_code"));
      SetProp(node,"id",Qry.FieldAsInteger("cl_grp_id"));
    };
    if (pr_hall)
    {
      node=NewTextChild(rowNode,"hall",Qry.FieldAsString("hall_name"));
      SetProp(node,"id",Qry.FieldAsInteger("hall_id"));
    };
    if (pr_airp_arv)
    {
      node=NewTextChild(rowNode,"airp_arv",Qry.FieldAsString("airp_arv"));
      SetProp(node,"id",Qry.FieldAsInteger("point_arv"));
    };
    if (pr_trfer)
    {
      NewTextChild(rowNode,"trfer",
                   convertLastTrfer(Qry.FieldAsString("last_trfer")));
    };

    if (pr_user)
    {
      node=NewTextChild(rowNode,"user",Qry.FieldAsString("user_descr"));
      SetProp(node,"id",Qry.FieldAsInteger("user_id"));
    };
    if (pr_rems)
    {
      NewTextChild(rowNode,"rems",Qry.FieldAsString("rem_code"));
    };
  };

  if (!pr_rems)
  {
    //отдельная строка по досылаемому багажу
    Qry.Clear();
    Qry.SQLText =
      "SELECT NVL(SUM(amount),0) AS bag_amount, "
      "       NVL(SUM(weight),0) AS bag_weight "
      "FROM unaccomp_bag "
      "WHERE point_dep=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof ||
        Qry.FieldAsInteger("bag_amount")==0 &&
        Qry.FieldAsInteger("bag_weight")==0) return;

    rowNode=NewTextChild(node2,"row");
    NewTextChild(rowNode,"title","Досыл");
    NewTextChild(rowNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
    NewTextChild(rowNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
  };
}

void viewPNL( int point_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  Qry.SQLText =
    "SELECT pax.reg_no, "
    "       ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr_ref, "
    "       RTRIM(crs_pax.surname||' '||crs_pax.name) full_name, "
    "       crs_pax.pers_type, "
    "       crs_pnr.class,crs_pnr.subclass, "
    "       crs_pax.seat_no,crs_pax.preseat_no, "
    "       crs_pax.seats seats, "
    "       crs_pnr.target, "
    "       report.get_trfer_airp(airp_arv) AS last_target, "
    "       report.get_PSPT(crs_pax.pax_id) AS document, "
    "       report.get_TKNO(crs_pax.pax_id) AS ticket, "
    "       crs_pax.pax_id, "
    "       crs_pax.tid tid, "
    "       crs_pnr.pnr_id "
    " FROM crs_pnr,tlg_binding,crs_pax,v_last_crs_trfer,pax "
    "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND "
    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pnr.pnr_id=v_last_crs_trfer.pnr_id(+) AND "
    "      crs_pax.pax_id=pax.pax_id(+) AND "
    "      crs_pax.pr_del=0 "
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
    if (!Qry.FieldIsNULL("reg_no"))
      NewTextChild( itemNode, "reg_no", Qry.FieldAsInteger( "reg_no" ) );
    NewTextChild( itemNode, "pnr_ref", Qry.FieldAsString( "pnr_ref" ), "" );
    NewTextChild( itemNode, "full_name", Qry.FieldAsString( "full_name" ) );
    NewTextChild( itemNode, "pers_type", Qry.FieldAsString( "pers_type" ), EncodePerson(ASTRA::adult) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "subclass", Qry.FieldAsString( "subclass" ) );
    NewTextChild( itemNode, "seat_no", Qry.FieldAsString( "seat_no" ), "" );
    NewTextChild( itemNode, "preseat_no", Qry.FieldAsString( "preseat_no" ), "" );
    NewTextChild( itemNode, "seats", Qry.FieldAsInteger( "seats" ), 1 );
    NewTextChild( itemNode, "target", Qry.FieldAsString( "target" ) );
    NewTextChild( itemNode, "last_target", Qry.FieldAsString( "last_target" ), "" );
    RQry.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    RQry.Execute();
    string rem, rem_code;
    xmlNodePtr stcrNode = NULL;
    for(;!RQry.Eof;RQry.Next())
    {
      rem += string( ".R/" ) + RQry.FieldAsString( "rem" ) + "   ";
      rem_code = RQry.FieldAsString( "rem_code" );
      if ( rem_code == "STCR" && !stcrNode )
      {
      	stcrNode = NewTextChild( itemNode, "step", "down" );
      };
    };
    NewTextChild( itemNode, "ticket", Qry.FieldAsString( "ticket" ), "" );
    NewTextChild( itemNode, "document", Qry.FieldAsString( "document" ), "" );
    NewTextChild( itemNode, "rem", rem, "" );
    NewTextChild( itemNode, "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    NewTextChild( itemNode, "pnr_id", Qry.FieldAsInteger( "pnr_id" ) );
    NewTextChild( itemNode, "tid", Qry.FieldAsInteger( "tid" ) );
    Qry.Next();
  }
}

string GetTripName( TTripInfo &info, bool showAirp )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TDateTime scd_out,real_out,desk_time;
  string &tz_region=AirpTZRegion(info.airp);
  modf(reqInfo->desk.time,&desk_time);
  modf(UTCToClient(info.scd_out,tz_region),&scd_out);
  modf(UTCToClient(info.real_out,tz_region),&real_out);
  ostringstream trip;
  trip << info.airline
       << info.flt_no
       << info.suffix;

  if (desk_time!=real_out)
  {
    if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(real_out,"mm"))
      trip << "/" << DateTimeToStr(real_out,"dd");
    else
      trip << "/" << DateTimeToStr(real_out,"dd.mm");
  };
  if (scd_out!=real_out)
    trip << "(" << DateTimeToStr(scd_out,"dd") << ")";
  if (!(reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.size()==1)||showAirp)
    trip << " " << info.airp;
  return trip.str();
};

