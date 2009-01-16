#include <stdlib.h>
#include <string>
#include "tripinfo.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "basic.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "brd.h"
#include "checkin.h"
#include "prepreg.h"
#include "telegram.h"
#include "docs.h"
#include "stat.h"
#include "print.h"
#include "astra_consts.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

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

  /* ------все рейсы без учета статусов------ */
  p.sqlfrom = "points";
  p.sqlwhere = "points.pr_del>=0 ";
  sqltrips[ "ALL POINTS" ] = p;
  p.clearVariables();

  /* ------------ПОСАДКА-------------- */
  /* ------------ДОСМОТР-------------- */
  /* задаем текст */
  p.sqlfrom =
    "points,trip_final_stages";
  p.sqlwhere =
    "points.point_id= trip_final_stages.point_id AND "
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "trip_final_stages.stage_type=:brd_stage_type AND "
    "trip_final_stages.stage_id=:brd_open_stage_id ";
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
    "time_out BETWEEN system.UTCSYSDATE-1 AND system.UTCSYSDATE+1 ";
  sqltrips[ "CENT.EXE" ] = p;
  p.clearVariables();


  /* ------------РЕГИСТРАЦИЯ------------ */
  /* ------------ДОКУМЕНТАЦИЯ------------ */
  p.sqlfrom =
    "points,trip_final_stages";
  p.sqlwhere =
    "points.point_id= trip_final_stages.point_id AND "
    "points.pr_del=0 AND "
    "trip_final_stages.stage_type=:ckin_stage_type AND "
    "trip_final_stages.stage_id IN (:ckin_open_stage_id,:ckin_close_stage_id,:ckin_doc_stage_id) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sCloseBoarding ) );
  sqltrips[ "AIR.EXE" ] = p;
  sqltrips[ "DOCS.EXE" ] = p;
  p.clearVariables();

  /* ------------КАССА------------ */
  p.sqlfrom =
    "points,trip_final_stages";
  p.sqlwhere =
    "points.point_id= trip_final_stages.point_id AND "
    "points.act_out IS NULL AND points.pr_del=0 AND "
    "trip_final_stages.stage_type=:ckin_stage_type AND "
    "trip_final_stages.stage_id IN (:ckin_open_stage_id,:ckin_close_stage_id,:ckin_doc_stage_id) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sCloseBoarding ) );
  sqltrips[ "KASSA.EXE" ] = p;
  p.clearVariables();

  /* ------------ПОДГОТОВКА------------ */
  p.sqlfrom =
    "points,trip_final_stages";
  p.sqlwhere =
    "points.point_id= trip_final_stages.point_id AND "
    "points.pr_del=0 AND "
    "trip_final_stages.stage_type=:ckin_stage_type AND "
    "trip_final_stages.stage_id IN (:ckin_prep_stage_id,:ckin_open_stage_id,:ckin_close_stage_id,:ckin_doc_stage_id) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_prep_stage_id", otInteger, IntToString( sPrepCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sCloseBoarding ) );
  sqltrips[ "PREPREG.EXE" ] = p;
  p.clearVariables();

  /* ------------ТЕЛЕГРАММЫ------------ */
  p.sqlfrom =
    "points";
  p.sqlwhere =
    "points.pr_del>=0 AND "
    "time_out BETWEEN TRUNC(system.UTCSYSDATE)-60 AND TRUNC(system.UTCSYSDATE)+2 ";
  sqltrips[ "TLG.EXE" ] = p;
  p.clearVariables();
}

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

  vector<int> &rights=info.user.access.rights;

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
    sql+=",trip_stations";

  sql+=" WHERE " + p.sqlwhere + " AND pr_reg<>0 ";

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
    sql+="AND points.point_id=trip_stations.point_id "
         "AND trip_stations.desk= :desk AND trip_stations.work_mode=:work_mode ";

  if ( info.screen.name == "AIR.EXE" )
  {
    vector<int>::iterator i;
    for(i=rights.begin();i!=rights.end();i++)
      if (*i==320||*i==330||*i==335) break;
    if (i==rights.end())
      sql+="AND points.act_out IS NULL ";
  };

  if (!info.user.access.airlines.empty())
  {
    if (info.user.access.airlines_permit)
      sql+="AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
    else
      sql+="AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
  };

  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
    {
      if (info.user.access.airps_permit)
        sql+="AND points.airp IN "+GetSQLEnum(info.user.access.airps);
      else
        sql+="AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    else
    {
      if (info.user.access.airps_permit)
        sql+="AND (points.airp IN "+GetSQLEnum(info.user.access.airps)+" OR "+
             "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) IN "+
                   GetSQLEnum(info.user.access.airps)+")";
      else
        sql+="AND (points.airp NOT IN "+GetSQLEnum(info.user.access.airps)+" OR "+
             "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) NOT IN "+
                   GetSQLEnum(info.user.access.airps)+")";
    };
  };
  sql+="ORDER BY TRUNC(real_out) DESC,flt_no,airline, "
       "         NVL(suffix,' '),move_id,point_num";
  Qry.SQLText = sql;
  p.setVariables( Qry );

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
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
  TSQLParams p;
  if (info.screen.name == "KASSA.EXE")
    p = Instance()->sqltrips[ "ALL POINTS" ];
  else
    p = Instance()->sqltrips[ info.screen.name ];

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
    "       ckin.tranzitable(points.point_id) AS tranzitable, "
    "       ckin.get_pr_tranzit(points.point_id) AS pr_tranzit, "
    "       points.first_point ";

  vector<int> &rights=info.user.access.rights;
  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end()) // система информирования
    sql+=",start_time ";

  sql+=
    "FROM " + p.sqlfrom;

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
    sql+=",trip_stations";

  sql+=" WHERE " + p.sqlwhere + " AND pr_reg<>0 AND points.point_id=:point_id ";

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
    sql+="AND points.point_id=trip_stations.point_id "
         "AND trip_stations.desk= :desk AND trip_stations.work_mode=:work_mode ";

  if ( info.screen.name == "AIR.EXE" )
  {
    vector<int>::iterator i;
    for(i=rights.begin();i!=rights.end();i++)
      if (*i==320||*i==330||*i==335) break;
    if (i==rights.end())
      sql+="AND points.act_out IS NULL ";
  };

  if (!info.user.access.airlines.empty())
  {
    if (info.user.access.airlines_permit)
      sql+="AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
    else
      sql+="AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
  };

  if (!info.user.access.airps.empty())
  {
    if ( info.screen.name != "TLG.EXE" )
    {
      if (info.user.access.airps_permit)
        sql+="AND points.airp IN "+GetSQLEnum(info.user.access.airps);
      else
        sql+="AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    else
    {
      if (info.user.access.airps_permit)
        sql+="AND (points.airp IN "+GetSQLEnum(info.user.access.airps)+" OR "+
             "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) IN "+
                   GetSQLEnum(info.user.access.airps)+")";
      else
        sql+="AND (points.airp NOT IN "+GetSQLEnum(info.user.access.airps)+" OR "+
             "     ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) NOT IN "+
                   GetSQLEnum(info.user.access.airps)+")";
    };
  };

  Qry.SQLText = sql;
  p.setVariables( Qry );

  if ((info.screen.name == "BRDBUS.EXE" || info.screen.name == "AIR.EXE") &&
       info.user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end())
  {
    Qry.CreateVariable( "desk", otString, info.desk.code );
    if (info.screen.name == "BRDBUS.EXE")
      Qry.CreateVariable( "work_mode", otString, "П");
    else
      Qry.CreateVariable( "work_mode", otString, "Р");
  };
};

class TTripListItem
{
  public:
    int point_id;
    string trip_name;
    TDateTime real_out_local_date;
};

bool lessTripListItem(const TTripListItem& item1,const TTripListItem& item2)
{
  return item1.real_out_local_date>item2.real_out_local_date;
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

  if (reqInfo->screen.name=="TLG.EXE")
  {
    tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "trip_id", -1 );
    NewTextChild( tripNode, "str", "Непривязанные" );
  };

  if (reqInfo->user.access.airlines.empty() && reqInfo->user.access.airlines_permit ||
      reqInfo->user.access.airps.empty() && reqInfo->user.access.airps_permit) return;

  vector<TTripListItem> list;
  TQuery Qry( &OraSession );
  TSQL::setSQLTripList( Qry, *reqInfo );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTripListItem listItem;

    TTripInfo info(Qry);

    try
    {
      listItem.point_id=Qry.FieldAsInteger("point_id");
      listItem.trip_name=GetTripName(info,reqInfo->screen.name=="TLG.EXE",true);
      listItem.real_out_local_date=info.real_out_local_date;
      list.push_back(listItem);
    }
    catch(UserException &E)
    {
      showErrorMessage((string)E.what()+". Некоторые рейсы не отображаются");
    };
  };

  stable_sort(list.begin(),list.end(),lessTripListItem);

  for(vector<TTripListItem>::iterator i=list.begin();i!=list.end();i++)
  {
    tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "trip_id", i->point_id );
    NewTextChild( tripNode, "str", i->trip_name );
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
      if ( GetNode( "paxdata", reqNode ) ) {
          BrdInterface::GetPax(reqNode,resNode);
      }
  };
  if (reqInfo->screen.name == "AIR.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      CheckInInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "tripdata", reqNode ) )
      CheckInInterface::readTripData( point_id, dataNode );
    xmlNodePtr node;
    node=GetNode( "tripBPpectabs", reqNode );
    if (node!=NULL)
    {
        int prn_type = NodeAsInteger("prn_type", node, NoExists);
        string dev_model = NodeAsString("dev_model", node, "");
        string fmt_type = NodeAsString("fmt_type", node, "");
        check_CUTE_certified(prn_type, dev_model, fmt_type);
        if(dev_model.empty())
            GetTripBPPectabs( point_id, prn_type, dataNode );
        else
            GetTripBPPectabs( point_id, dev_model, fmt_type, dataNode );
    };
    node=GetNode( "tripBTpectabs", reqNode );
    if (node!=NULL)
    {
        int prn_type = NodeAsInteger("prn_type", node, NoExists);
        string dev_model = NodeAsString("dev_model", node, "");
        string fmt_type = NodeAsString("fmt_type", node, "");
        check_CUTE_certified(prn_type, dev_model, fmt_type);
        if(dev_model.empty())
            GetTripBTPectabs( point_id, prn_type, dataNode );
        else
            GetTripBTPectabs( point_id, dev_model, fmt_type, dataNode );
    };
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
  ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
};

bool TripsInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripsInterface::readTripHeader" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access( amRead );

  if (reqInfo->screen.name=="TLG.EXE" && point_id==-1)
  {
    xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
    NewTextChild( node, "point_id", -1 );
    return true;
  };

  if (reqInfo->user.access.airlines.empty() && reqInfo->user.access.airlines_permit ||
      reqInfo->user.access.airps.empty() && reqInfo->user.access.airps_permit)
    return false;

  TQuery Qry( &OraSession );
  TSQL::setSQLTripInfo( Qry, *reqInfo );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof) return false;
  xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
  NewTextChild( node, "point_id", Qry.FieldAsInteger( "point_id" ) );
  string airline=Qry.FieldAsString( "airline" );
  NewTextChild( node, "airline", airline );
  try
  {
    TAirlinesRow &row = (TAirlinesRow&)base_tables.get("airlines").get_row("code",airline);
    NewTextChild( node, "airline_lat", row.code_lat );
    NewTextChild( node, "aircode", row.aircode );
  }
  catch(EBaseTableError) {};

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
  NewTextChild( node, "trip", GetTripName(info,reqInfo->screen.name=="TLG.EXE",true) );

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
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    status = stagesRules->status( stCheckIn, tripStages.getStage( stCheckIn ) );
  };
  if ( reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "CENT.EXE" )
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
       reqInfo->screen.name == "CENT.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" )
    stage_time = UTCToClient( tripStages.time( sCloseBoarding ), tz_region );
  if ( reqInfo->screen.name == "AIR.EXE" ||
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

  if (reqInfo->screen.name == "CENT.EXE" ||
      reqInfo->screen.name == "KASSA.EXE" )
  {
    NewTextChild( node, "craft_stage", tripStages.getStage( stCraft ) );
  };
  vector<int> &rights=reqInfo->user.access.rights;
  if ((reqInfo->screen.name == "BRDBUS.EXE" || reqInfo->screen.name == "AIR.EXE") &&
       reqInfo->user.user_type==utAirport &&
       find(rights.begin(),rights.end(),335)==rights.end()) { // система информирования
    NewTextChild( node, "start_check_info", (int)!Qry.FieldIsNULL( "start_time" ) );
  }


  if (reqInfo->screen.name == "AIR.EXE" ||
      reqInfo->screen.name == "BRDBUS.EXE" ||
      reqInfo->screen.name == "EXAM.EXE" ||
      reqInfo->screen.name == "DOCS.EXE" ||
      reqInfo->screen.name == "PREPREG.EXE")
  {
    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "ckin_stage", tripStages.getStage( stCheckIn ) );
      NewTextChild( node, "tranzitable", (int)(Qry.FieldAsInteger("tranzitable")!=0) );
    };

    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText=
      "SELECT NVL(pr_tranz_reg,0) AS pr_tranz_reg, "
      "       NVL(pr_block_trzt,0) AS pr_block_trzt, "
      "       pr_check_load,pr_overload_reg,pr_exam,pr_check_pay,pr_exam_check_pay, "
      "       pr_reg_with_tkn,pr_reg_with_doc,pr_bp_market_flt,pr_etstatus "
      "FROM trip_sets WHERE point_id=:point_id ";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.Execute();
    if (Qryh.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
    if (Qryh.FieldAsInteger("pr_etstatus")<0)
    {
      //вывод "Нет связи с СЭБ" в информации по рейсу
      string remark=Qry.FieldAsString( "remark" );
      if (!remark.empty()) remark.append(" ");
      remark.append("Нет связи с СЭБ.");
      ReplaceTextChild(node, "remark",  remark);
    };

    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "DOCS.EXE" ||
        reqInfo->screen.name == "PREPREG.EXE")
    {
      NewTextChild( node, "pr_tranz_reg", (int)(Qryh.FieldAsInteger("pr_tranz_reg")!=0) );
      NewTextChild( node, "pr_block_trzt", (int)(Qryh.FieldAsInteger("pr_block_trzt")!=0) );
      NewTextChild( node, "pr_check_load", (int)(Qryh.FieldAsInteger("pr_check_load")!=0) );
      NewTextChild( node, "pr_overload_reg", (int)(Qryh.FieldAsInteger("pr_overload_reg")!=0) );
      NewTextChild( node, "pr_exam", (int)(Qryh.FieldAsInteger("pr_exam")!=0) );
      NewTextChild( node, "pr_check_pay", (int)(Qryh.FieldAsInteger("pr_check_pay")!=0) );
      NewTextChild( node, "pr_exam_check_pay", (int)(Qryh.FieldAsInteger("pr_exam_check_pay")!=0) );
      NewTextChild( node, "pr_trfer_reg", (int)false );  //!!!потом убрать GetNode 01.12.08
      NewTextChild( node, "pr_reg_with_tkn", (int)(Qryh.FieldAsInteger("pr_reg_with_tkn")!=0) );
      NewTextChild( node, "pr_reg_with_doc", (int)(Qryh.FieldAsInteger("pr_reg_with_doc")!=0) );
      NewTextChild( node, "pr_bp_market_flt", (int)(Qryh.FieldAsInteger("pr_bp_market_flt")!=0) );
    };
    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "BRDBUS.EXE" ||
        reqInfo->screen.name == "EXAM.EXE")
    {
      NewTextChild( node, "pr_etstatus", Qryh.FieldAsInteger("pr_etstatus") );
      NewTextChild( node, "pr_etl_only", (int)GetTripSets(tsETLOnly,info) );
    };
  };
  return true;
}

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
      order_by << ", NVL(classes.priority,100)";
    };
    if (strcmp((char*)node->name,"cl_grp")==0)
    {
      pr_cl_grp=true;
      order_by << ", NVL(cls_grp.priority,100)";
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
    "       b.bag_amount, "
    "       b.bag_weight, "
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
    "        point_dep=:point_id AND bag_refuse=0) b, "
    " (SELECT NVL(SUM(crs_ok),0) AS crs_ok, "
    "         NVL(SUM(crs_tranzit),0) AS crs_tranzit "
    "  FROM counters2 "
    "  WHERE point_dep=:point_id) c, "
    " (SELECT NVL(SUM(excess),0) AS excess "
    "  FROM pax_grp "
    "  WHERE point_dep=:point_id AND bag_refuse=0) e, "
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
  if (pr_class) select << ", NVL(class,' ') AS class";
  if (pr_cl_grp) select << ", NVL(class_grp,-1) AS class_grp";
  if (pr_hall) select << ", hall";
  if (pr_airp_arv) select << ", point_arv";
  if (pr_trfer) select << ", NVL(last_trfer,' ') AS last_trfer";
  if (pr_user) select << ", user_id";
  if (pr_rems) select << ", rem_code";

  //строка group by для подзапросов
  ostringstream group_by;
  if (pr_class) group_by << ", NVL(class,' ')";
  if (pr_cl_grp) group_by << ", NVL(class_grp,-1)";
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
      sql << ",b.bag_amount,b.bag_weight,b.rk_weight,e.excess" << endl;

    if (!pr_cl_grp && !pr_hall && !pr_trfer && !pr_user)
    {
      sql << ",c.crs_ok,c.crs_tranzit" << endl;
      if (!pr_airp_arv) sql << ",f.cfg" << endl;
    };
  };

  if (pr_class)    sql << ",DECODE(a.class,' ',NULL,a.class) AS class" << endl;
  if (pr_cl_grp)   sql << ",DECODE(a.class_grp,-1,NULL,a.class_grp) AS cl_grp_id"
                          ",cls_grp.code AS cl_grp_code" << endl;
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

  sql << "WHERE pax_grp.grp_id=pax.grp_id(+) AND " << endl
      << "      (pax.grp_id IS NULL AND pax_grp.class IS NULL OR " << endl
      << "       pax.grp_id IS NOT NULL) AND " << endl
      << "      point_dep=:point_id AND pr_brd(+) IS NOT NULL " << endl;
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
        << "      point_dep=:point_id AND bag_refuse=0 " << endl;
    if (pr_trfer) sql << "AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;

    sql << "GROUP BY " << group_by.str().erase(0,1) << ") b" << endl;

    //запрос по платному багажу
    sql << ",(SELECT SUM(excess) AS excess, " << endl
        << "         " << select.str().erase(0,1) << endl;

    sql << "FROM pax_grp ";
    if (pr_trfer) sql << ",v_last_trfer";

    sql << endl
        << "WHERE point_dep=:point_id AND bag_refuse=0 " << endl;
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
  if (pr_class)    where << " AND a.class=classes.code(+)" << endl;
  if (pr_cl_grp)   where << " AND a.class_grp=cls_grp.id(+)" << endl;
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
      if (Qry.FieldIsNULL("class"))
        ReplaceTextChild(rowNode,"title","Несопр");
    };
    if (pr_cl_grp)
    {
      node=NewTextChild(rowNode,"cl_grp",Qry.FieldAsString("cl_grp_code"));
      SetProp(node,"id",Qry.FieldAsInteger("cl_grp_id"));
      if (Qry.FieldIsNULL("cl_grp_id"))
        ReplaceTextChild(rowNode,"title","Несопр");
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
}

void viewCRSList( int point_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText=
     "SELECT "
     "      ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr_ref, "
     "      RTRIM(crs_pax.surname||' '||crs_pax.name) full_name, "
     "      crs_pax.pers_type, "
     "      crs_pnr.class,crs_pnr.subclass, "
     "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "
     "      salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS preseat_no, "
     "      crs_pax.seats seats, "
     "      crs_pnr.target, "
     "      crs_pnr.last_target, "
     "      report.get_PSPT(crs_pax.pax_id) AS document, "
     "      report.get_TKNO(crs_pax.pax_id) AS ticket, "
     "      crs_pax.pax_id, "
     "      crs_pax.tid tid, "
     "      crs_pnr.pnr_id, "
     "      crs_pnr.point_id AS point_id_tlg, "
     "      ids.status, "
     "      pax.reg_no, "
     "      salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no, "
     "      pax.refuse, "
     "      pax.grp_id "
     "FROM crs_pnr,crs_pax,pax,pax_grp, "
     "       ( "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_ok AS status "
     "        FROM crs_pnr, "
     "         (SELECT b2.point_id_tlg, "
     "                 airp_arv_tlg,class_tlg,status "
     "          FROM crs_displace2,tlg_binding b1,tlg_binding b2 "
     "          WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND "
     "                b1.point_id_spp=b2.point_id_spp AND "
     "                crs_displace2.point_id_spp=:point_id AND "
     "                b1.point_id_spp<>:point_id) crs_displace "
     "        WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND "
     "              crs_pnr.target=crs_displace.airp_arv_tlg AND "
     "              crs_pnr.class=crs_displace.class_tlg AND "
     "              crs_displace.status=:ps_ok AND "
     "              crs_pnr.wl_priority IS NULL "
     "        UNION "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_ok "
     "        FROM crs_pnr,tlg_binding "
     "        WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
     "              tlg_binding.point_id_spp= :point_id AND "
     "              crs_pnr.wl_priority IS NULL "
     "        UNION "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_goshow "
     "        FROM crs_pnr, "
     "         (SELECT b2.point_id_tlg, "
     "                 airp_arv_tlg,class_tlg,status "
     "          FROM crs_displace2,tlg_binding b1,tlg_binding b2 "
     "          WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND "
     "                b1.point_id_spp=b2.point_id_spp AND "
     "                crs_displace2.point_id_spp=:point_id AND "
     "                b1.point_id_spp<>:point_id) crs_displace "
     "        WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND "
     "              crs_pnr.target=crs_displace.airp_arv_tlg AND "
     "              crs_pnr.class=crs_displace.class_tlg AND "
     "              crs_displace.status=:ps_goshow AND "
     "              crs_pnr.wl_priority IS NULL "
     "        MINUS "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_goshow "
     "        FROM crs_pnr,tlg_binding "
     "        WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
     "              tlg_binding.point_id_spp= :point_id AND "
     "              crs_pnr.wl_priority IS NULL "
     "        UNION "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_transit "
     "        FROM crs_pnr, "
     "         (SELECT b2.point_id_tlg, "
     "                 airp_arv_tlg,class_tlg,status "
     "          FROM crs_displace2,tlg_binding b1,tlg_binding b2 "
     "          WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND "
     "                b1.point_id_spp=b2.point_id_spp AND "
     "                crs_displace2.point_id_spp=:point_id AND "
     "                b1.point_id_spp<>:point_id) crs_displace "
     "        WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND "
     "              crs_pnr.target=crs_displace.airp_arv_tlg AND "
     "              crs_pnr.class=crs_displace.class_tlg AND "
     "              crs_displace.status=:ps_transit AND "
     "              crs_pnr.wl_priority IS NULL "
     "        MINUS "
     "        SELECT DISTINCT crs_pnr.pnr_id,:ps_transit "
     "        FROM crs_pnr,tlg_binding "
     "        WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
     "              tlg_binding.point_id_spp= :point_id AND "
     "              crs_pnr.wl_priority IS NULL "
     "       ) ids "
     "WHERE crs_pnr.pnr_id=ids.pnr_id AND "
     "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
     "      crs_pax.pax_id=pax.pax_id(+) AND "
     "      pax.grp_id=pax_grp.grp_id(+) AND "
     "      crs_pax.pr_del=0 "
     "ORDER BY crs_pnr.point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psOk) );
  Qry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
  Qry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
  Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType(ASTRA::cltProtCkin) );
  Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );
  Qry.Execute();

  //ремарки пассажиров
  TQuery RQry( &OraSession );
  RQry.SQLText =
    "SELECT crs_pax_rem.rem, crs_pax_rem.rem_code, NVL(rem_types.priority,-1) AS priority "
    "FROM crs_pax_rem,rem_types "
    "WHERE crs_pax_rem.rem_code=rem_types.code(+) AND crs_pax_rem.pax_id=:pax_id "
    "ORDER BY priority DESC,rem_code,rem ";
  RQry.DeclareVariable( "pax_id", otInteger );

  //рейс пассажиров
  TQuery TlgTripsQry( &OraSession );
  TlgTripsQry.SQLText=
    "SELECT airline,flt_no,suffix,scd,airp_dep "
    "FROM tlg_trips WHERE point_id=:point_id ";
  TlgTripsQry.DeclareVariable("point_id",otInteger);

  TQuery PointsQry( &OraSession );
  PointsQry.SQLText=
    "SELECT point_dep AS point_id FROM pax_grp WHERE grp_id=:grp_id";
  PointsQry.DeclareVariable("grp_id",otInteger);

  Qry.Execute();
  dataNode = NewTextChild( dataNode, "tlg_trips" );
  int point_id_tlg=-1;
  xmlNodePtr tripNode,paxNode,node;
  int col_pnr_ref=Qry.FieldIndex("pnr_ref");
  int col_full_name=Qry.FieldIndex("full_name");
  int col_pers_type=Qry.FieldIndex("pers_type");
  int col_class=Qry.FieldIndex("class");
  int col_subclass=Qry.FieldIndex("subclass");
  int col_crs_seat_no=Qry.FieldIndex("crs_seat_no");
  int col_preseat_no=Qry.FieldIndex("preseat_no");
  int col_seats=Qry.FieldIndex("seats");
  int col_target=Qry.FieldIndex("target");
  int col_last_target=Qry.FieldIndex("last_target");
  int col_document=Qry.FieldIndex("document");
  int col_ticket=Qry.FieldIndex("ticket");
  int col_pax_id=Qry.FieldIndex("pax_id");
  int col_tid=Qry.FieldIndex("tid");
  int col_pnr_id=Qry.FieldIndex("pnr_id");
  int col_point_id_tlg=Qry.FieldIndex("point_id_tlg");
  int col_status=Qry.FieldIndex("status");
  int col_reg_no=Qry.FieldIndex("reg_no");
  int col_seat_no=Qry.FieldIndex("seat_no");
  int col_refuse=Qry.FieldIndex("refuse");
  int col_grp_id=Qry.FieldIndex("grp_id");

  for(;!Qry.Eof;Qry.Next())
  {
    if (!Qry.FieldIsNULL(col_grp_id))
    {
      PointsQry.SetVariable("grp_id",Qry.FieldAsInteger(col_grp_id));
      PointsQry.Execute();
      if (!PointsQry.Eof&&point_id!=PointsQry.FieldAsInteger("point_id")) continue;
    };

    if (point_id_tlg!=Qry.FieldAsInteger(col_point_id_tlg))
    {
      point_id_tlg=Qry.FieldAsInteger(col_point_id_tlg);
      tripNode = NewTextChild( dataNode, "tlg_trip" );
      TlgTripsQry.SetVariable("point_id",point_id_tlg);
      TlgTripsQry.Execute();
      if (TlgTripsQry.Eof) throw UserException("Рейс не найден. Повторите запрос");
      ostringstream trip;
      trip << TlgTripsQry.FieldAsString("airline")
           << setw(3) << setfill('0') << TlgTripsQry.FieldAsInteger("flt_no")
           << TlgTripsQry.FieldAsString("suffix")
           << "/" << DateTimeToStr(TlgTripsQry.FieldAsDateTime("scd"),"ddmmm")
           << " " << TlgTripsQry.FieldAsString("airp_dep");
      NewTextChild(tripNode,"name",trip.str());
      paxNode = NewTextChild(tripNode,"passengers");
    };
    node = NewTextChild(paxNode,"pax");

    NewTextChild( node, "pnr_ref", Qry.FieldAsString( col_pnr_ref ), "" );
    NewTextChild( node, "full_name", Qry.FieldAsString( col_full_name ) );
    NewTextChild( node, "pers_type", Qry.FieldAsString( col_pers_type ), EncodePerson(ASTRA::adult) );
    NewTextChild( node, "class", Qry.FieldAsString( col_class ), EncodeClass(ASTRA::Y) );
    NewTextChild( node, "subclass", Qry.FieldAsString( col_subclass ) );
    NewTextChild( node, "crs_seat_no", Qry.FieldAsString( col_crs_seat_no ), "" );
    NewTextChild( node, "preseat_no", Qry.FieldAsString( col_preseat_no ), "" );
    NewTextChild( node, "seats", Qry.FieldAsInteger( col_seats ), 1 );
    NewTextChild( node, "target", Qry.FieldAsString( col_target ) );
    if (!Qry.FieldIsNULL(col_last_target))
    {
      try
      {
        TAirpsRow &row=(TAirpsRow&)(base_tables.get("airps").get_row("code/code_lat",Qry.FieldAsString( col_last_target )));
        NewTextChild( node, "last_target", row.code);
      }
      catch(EBaseTableError)
      {
        NewTextChild( node, "last_target", Qry.FieldAsString( col_last_target ) );
      };
    };

    NewTextChild( node, "ticket", Qry.FieldAsString( col_ticket ), "" );
    NewTextChild( node, "document", Qry.FieldAsString( col_document ), "" );
    NewTextChild( node, "status", Qry.FieldAsString( col_status ), EncodePaxStatus(ASTRA::psOk) );

    RQry.SetVariable( "pax_id", Qry.FieldAsInteger( col_pax_id ) );
    RQry.Execute();
    string rem, rem_code;
    xmlNodePtr stcrNode = NULL;
    for(;!RQry.Eof;RQry.Next())
    {
      rem += string( ".R/" ) + RQry.FieldAsString( "rem" ) + "   ";
      rem_code = RQry.FieldAsString( "rem_code" );
      if ( rem_code == "STCR" && !stcrNode )
      {
      	stcrNode = NewTextChild( node, "step", "down" );
      };
    };
    NewTextChild( node, "rem", rem, "" );
    NewTextChild( node, "pax_id", Qry.FieldAsInteger( col_pax_id ) );
    NewTextChild( node, "pnr_id", Qry.FieldAsInteger( col_pnr_id ) );
    NewTextChild( node, "tid", Qry.FieldAsInteger( col_tid ) );

    if (!Qry.FieldIsNULL(col_grp_id))
    {
      NewTextChild( node, "reg_no", Qry.FieldAsInteger( col_reg_no ) );
      NewTextChild( node, "seat_no", Qry.FieldAsString( col_seat_no ), "" );
      NewTextChild( node, "refuse", Qry.FieldAsString( col_refuse ), "" );
    };
  };

};

string GetTripName( TTripInfo &info, bool showAirp, bool prList )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TDateTime scd_out_local_date,desk_time;
  string &tz_region=AirpTZRegion(info.airp);
  modf(reqInfo->desk.time,&desk_time);
  modf(UTCToClient(info.scd_out,tz_region),&scd_out_local_date);
  modf(UTCToClient(info.real_out,tz_region),&info.real_out_local_date);
  ostringstream trip;
  trip << info.airline
       << setw(3) << setfill('0') << info.flt_no
       << info.suffix;

  if (prList)
  {
    if (info.flt_no<10000) trip << " ";
    if (info.flt_no<1000)  trip << " ";
  };

  if (desk_time!=info.real_out_local_date)
  {
    if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(info.real_out_local_date,"mm"))
      trip << "/" << DateTimeToStr(info.real_out_local_date,"dd");
    else
      trip << "/" << DateTimeToStr(info.real_out_local_date,"dd.mm");
  };
  if (scd_out_local_date!=info.real_out_local_date)
    trip << "(" << DateTimeToStr(scd_out_local_date,"dd") << ")";
  if (!(reqInfo->user.user_type==utAirport &&
        reqInfo->user.access.airps_permit &&
        reqInfo->user.access.airps.size()==1)||showAirp)
    trip << " " << info.airp;
  if(info.pr_del != ASTRA::NoExists and info.pr_del != 0)
      trip << " " << (info.pr_del < 0 ? "(удл.)" : "(отм.)");

  return trip.str();
};

bool GetTripSets( TTripSetType setType, TTripInfo &info )
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_misc, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM misc_set "
    "WHERE type=:type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("type",otInteger,(int)setType);
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  if (Qry.Eof)
  {
    switch(setType)
    {
      //запрет интерактива с СЭБом
      case tsETLOnly: return false;
              default: return false;
    };
  };
  return Qry.FieldAsInteger("pr_misc")!=0;
};

