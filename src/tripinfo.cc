#include <stdlib.h>
#include <string>
#include "tripinfo.h"
#include "stages.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "astra_misc.h"
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
#include "convert.h"
#include "astra_misc.h"
#include "term_version.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

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
  p.clearVariables();

  /* ------------ДОКУМЕНТАЦИЯ------------ */
  p.sqlfrom =
    "points,trip_final_stages";
  p.sqlwhere =
    "points.point_id= trip_final_stages.point_id AND "
    "points.pr_del=0 AND "
    "points.time_out>=system.UTCSYSDATE-30 AND "
    "trip_final_stages.stage_type=:ckin_stage_type AND "
    "trip_final_stages.stage_id IN (:ckin_open_stage_id,:ckin_close_stage_id,:ckin_doc_stage_id) ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "ckin_open_stage_id", otInteger, IntToString( sOpenCheckIn ) );
  p.addVariable( "ckin_close_stage_id", otInteger, IntToString( sCloseCheckIn ) );
  p.addVariable( "ckin_doc_stage_id", otInteger, IntToString( sCloseBoarding ) );
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
    "time_out BETWEEN TRUNC(system.UTCSYSDATE)-15 AND TRUNC(system.UTCSYSDATE)+5 ";
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
    "       points.airline_fmt, "
    "       points.airp_fmt, "
    "       points.suffix_fmt, "
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
  sql+="ORDER BY flt_no,airline, "
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
    "       points.craft_fmt, "
    "       points.airp, "
    "       points.scd_out, "
    "       points.act_out, "
    "       points.bort, "
    "       points.park_out, "
    "       SUBSTR(ckin.get_classes(points.point_id,:vlang),1,50) AS classes, "
    "       SUBSTR(ckin.get_airps(points.point_id,:vlang),1,50) AS route, "
    "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
    "       points.trip_type, "
    "       points.litera, "
    "       points.remark, "
    "       ckin.tranzitable(points.point_id) AS tranzitable, "
    "       ckin.get_pr_tranzit(points.point_id) AS pr_tranzit, "
    "       points.first_point ";

  vector<int> &rights=info.user.access.rights;

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
  Qry.CreateVariable( "vlang", otString, info.desk.lang );

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
    TDateTime real_out_client;
};

bool lessTripListItem(const TTripListItem& item1,const TTripListItem& item2)
{
  return item1.real_out_client>item2.real_out_client;
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
    NewTextChild( tripNode, "str", AstraLocale::getLocaleText("Непривязанные") );
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
    TDateTime scd_out_client;

    TTripInfo info(Qry);

    try
    {
      listItem.point_id=Qry.FieldAsInteger("point_id");
      listItem.trip_name=GetTripName(info,ecCkin,reqInfo->screen.name=="TLG.EXE",true); //ecCkin? !!!vlad
      info.get_client_dates(scd_out_client,listItem.real_out_client);
      list.push_back(listItem);
    }
    catch(AstraLocale::UserException &E)
    {
      AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
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
  xmlNodePtr dataNode=NewTextChild( resNode, "data" );
  GetSegInfo(reqNode, resNode, dataNode);
  //обработка многосегментного запроса
  xmlNodePtr node=GetNode("segments",reqNode);
  if (node!=NULL)
  {
    xmlNodePtr segsNode=NewTextChild(dataNode,"segments");
    for(node=node->children;node!=NULL;node=node->next)
      GetSegInfo(node, NULL, NewTextChild(segsNode,"segment"));
  };
  //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};

void TripsInterface::GetSegInfo(xmlNodePtr reqNode, xmlNodePtr resNode, xmlNodePtr dataNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  int point_id = NodeAsInteger( "point_id", reqNode );
  NewTextChild( dataNode, "point_id", point_id );

  if ( GetNode( "tripheader", reqNode ) )
    if ( !readTripHeader( point_id, dataNode ) )
      AstraLocale::showErrorMessage( "MSG.FLT.NOT_AVAILABLE" );


  if (reqInfo->screen.name == "BRDBUS.EXE" ||
      reqInfo->screen.name == "EXAM.EXE" )
  {
      if ( GetNode( "counters", reqNode ) )
      {
          TRptParams rpt_params(reqInfo->desk.lang);
          BrdInterface::readTripCounters( point_id, rpt_params, dataNode, rtUnknown, "" );
      };
      if ( GetNode( "tripdata", reqNode ) )
          BrdInterface::readTripData( point_id, dataNode );
      if ( GetNode( "paxdata", reqNode ) && resNode!=NULL ) {
          BrdInterface::GetPax(reqNode,resNode);
      }
  };
  if (reqInfo->screen.name == "AIR.EXE")
  {
    if ( GetNode( "tripcounters", reqNode ) )
      CheckInInterface::readTripCounters( point_id, dataNode );
    if ( GetNode( "tripdata", reqNode ) )
      CheckInInterface::readTripData( point_id, dataNode );
    if ( GetNode( "tripsets", reqNode ) )
      CheckInInterface::readTripSets( point_id, dataNode );

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
  if (reqInfo->screen.name == "TLG.EXE" ||
      reqInfo->screen.name == "DOCS.EXE")
  {
    if ( GetNode( "tripdata", reqNode ) && point_id != -1 )
      TelegramInterface::readTripData( point_id, dataNode );
    if ( GetNode( "ckin_zones", reqNode ) )
        DocsInterface::GetZoneList(point_id, dataNode);
  };
};

void TripsInterface::readOperFltHeader( const TTripInfo &info, xmlNodePtr node )
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  if ( reqInfo->screen.name == "AIR.EXE" )
    NewTextChild( node, "flight", GetTripName(info,ecCkin,true,false) );

  NewTextChild( node, "airline", info.airline );
  try
  {
    TAirlinesRow &row = (TAirlinesRow&)base_tables.get("airlines").get_row("code",info.airline);
    if (!reqInfo->desk.compatible(LATIN_VERSION))
      NewTextChild( node, "airline_lat", row.code_lat );
    NewTextChild( node, "aircode", row.aircode );
  }
  catch(EBaseTableError) {};

  NewTextChild( node, "flt_no", info.flt_no );
  NewTextChild( node, "suffix", info.suffix );
  NewTextChild( node, "airp", info.airp );
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "KASSA.EXE" )
  {
    //внимание! локальная дата порта
    NewTextChild( node, "scd_out_local", DateTimeToStr(UTCToLocal(info.scd_out,AirpTZRegion(info.airp))) );
  };
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
  TTripInfo info(Qry);
  xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
  NewTextChild( node, "point_id", Qry.FieldAsInteger( "point_id" ) );

  readOperFltHeader(info,node);

  string &tz_region=AirpTZRegion(info.airp);
  TDateTime scd_out_client,
            act_out_client,
            real_out_client;

  scd_out_client= UTCToClient(info.scd_out,tz_region);
  real_out_client=UTCToClient(info.real_out,tz_region);

  if ( reqInfo->screen.name == "TLG.EXE" )
  {
    ostringstream trip;
    trip << ElemIdToCodeNative(etAirline,info.airline)
         << setw(3) << setfill('0') << info.flt_no
         << ElemIdToCodeNative(etSuffix,info.suffix);
    NewTextChild( node, "flight", trip.str() );
  };

  NewTextChild( node, "scd_out", DateTimeToStr(scd_out_client) );
  NewTextChild( node, "real_out", DateTimeToStr(real_out_client,"hh:nn") );
  if (!Qry.FieldIsNULL("act_out"))
  {
    act_out_client= UTCToClient(Qry.FieldAsDateTime("act_out"),tz_region);
    NewTextChild( node, "act_out", DateTimeToStr(act_out_client) );
  }
  else
  {
    act_out_client= ASTRA::NoExists;
    NewTextChild( node, "act_out" );
  };

  NewTextChild( node, "craft", ElemIdToElemCtxt(ecCkin,etCraft, Qry.FieldAsString( "craft" ), (TElemFmt)Qry.FieldAsInteger( "craft_fmt" )) );
  NewTextChild( node, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( node, "park", Qry.FieldAsString( "park_out" ) );
  NewTextChild( node, "classes", Qry.FieldAsString( "classes" ) );
  NewTextChild( node, "route", Qry.FieldAsString( "route" ) );
  NewTextChild( node, "places", Qry.FieldAsString( "route" ) );
  NewTextChild( node, "trip_type", ElemIdToCodeNative(etTripType,Qry.FieldAsString( "trip_type" )) );
  NewTextChild( node, "litera", Qry.FieldAsString( "litera" ) );
  NewTextChild( node, "remark", Qry.FieldAsString( "remark" ) );
  NewTextChild( node, "pr_tranzit", (int)Qry.FieldAsInteger( "pr_tranzit" )!=0 );

  //trip нужен для ChangeTrip клиента:
  NewTextChild( node, "trip", GetTripName(info,ecCkin,reqInfo->screen.name=="TLG.EXE",true)); //ecCkin? !!!vlad

  TTripStages tripStages( point_id );
  TStagesRules *stagesRules = TStagesRules::Instance();

  //статусы рейсов
  string status;
  if ( reqInfo->screen.name == "BRDBUS.EXE" ||
       reqInfo->screen.name == "EXAM.EXE")
  {
    status = stagesRules->status( stBoarding, tripStages.getStage( stBoarding ), true );
  };
  if ( reqInfo->screen.name == "AIR.EXE" ||
       reqInfo->screen.name == "PREPREG.EXE" )
  {
    status = stagesRules->status( stCheckIn, tripStages.getStage( stCheckIn ), true );
  };
  if ( reqInfo->screen.name == "KASSA.EXE" ||
       reqInfo->screen.name == "CENT.EXE" )
  {
    TStage ckin_stage =  tripStages.getStage( stCheckIn );
    TStage craft_stage = tripStages.getStage( stCraft );
    if ( craft_stage == sRemovalGangWay || craft_stage == sTakeoff )
      status = stagesRules->status( stCraft, craft_stage, true );
    else
      status = stagesRules->status( stCheckIn, ckin_stage, true );
  };
  if ( reqInfo->screen.name == "DOCS.EXE" )
  {
    if (act_out_client==ASTRA::NoExists)
      status = stagesRules->status( stCheckIn, tripStages.getStage( stCheckIn ), true );
    else
      status = stagesRules->status( stCheckIn, sTakeoff, true );
  };
  if ( reqInfo->screen.name == "TLG.EXE" )
  {
    if (act_out_client==ASTRA::NoExists)
      status = stagesRules->status( stCraft, tripStages.getStage( stCraft ), true );
    else
      status = stagesRules->status( stCraft, sTakeoff, true );
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

  if (!reqInfo->desk.compatible(NEW_TERM_VERSION))
  {
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
  };

  if (reqInfo->screen.name == "CENT.EXE" ||
      reqInfo->screen.name == "KASSA.EXE" )
  {
    NewTextChild( node, "craft_stage", tripStages.getStage( stCraft ) );
  };

  if (reqInfo->screen.name == "AIR.EXE" && reqInfo->desk.airp == "ВНК")
  {
    TQuery Qryh( &OraSession );
    Qryh.Clear();
    Qryh.SQLText=
      "SELECT start_time FROM trip_stations "
      "WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.CreateVariable( "desk", otString, reqInfo->desk.code );
    Qryh.CreateVariable( "work_mode", otString, "Р" );
    Qryh.Execute();
    if (!Qryh.Eof)
      NewTextChild( node, "start_check_info", (int)!Qryh.FieldIsNULL( "start_time" ) );
  };

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
      "       pr_reg_with_tkn,pr_reg_with_doc,pr_etstatus,pr_airp_seance "
      "FROM trip_sets WHERE point_id=:point_id ";
    Qryh.CreateVariable( "point_id", otInteger, point_id );
    Qryh.Execute();
    if (Qryh.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
/*    if (Qryh.FieldAsInteger("pr_etstatus")<0)
    {
      //вывод "Нет связи с СЭБ" в информации по рейсу
      string remark=Qry.FieldAsString( "remark" );
      if (!remark.empty()) remark.append(" ");
      remark.append("Нет связи с СЭБ.");
      ReplaceTextChild(node, "remark",  remark);
    };*/

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
      NewTextChild( node, "pr_reg_with_tkn", (int)(Qryh.FieldAsInteger("pr_reg_with_tkn")!=0) );
      NewTextChild( node, "pr_reg_with_doc", (int)(Qryh.FieldAsInteger("pr_reg_with_doc")!=0) );
      if (!Qryh.FieldIsNULL("pr_airp_seance"))
        NewTextChild( node, "pr_airp_seance", (int)(Qryh.FieldAsInteger("pr_airp_seance")!=0) );
      else
        NewTextChild( node, "pr_airp_seance" );
      NewTextChild( node, "pr_trfer_reg", (int)false );  //!!!потом убрать GetNode 01.12.08
      NewTextChild( node, "pr_bp_market_flt", (int)false );  //!!!потом убрать 14.04.09
    };
    if (reqInfo->screen.name == "AIR.EXE" ||
        reqInfo->screen.name == "BRDBUS.EXE" ||
        reqInfo->screen.name == "EXAM.EXE")
    {
      NewTextChild( node, "pr_etstatus", Qryh.FieldAsInteger("pr_etstatus") );
      NewTextChild( node, "pr_etl_only", (int)GetTripSets(tsETLOnly,info) );
    };
    if (reqInfo->screen.name == "AIR.EXE")
    {
      NewTextChild( node, "pr_no_ticket_check", (int)GetTripSets(tsNoTicketCheck,info) );
    };
  };

  {
  	string stralarms;
    BitSet<TTripAlarmsType> Alarms;
    TripAlarms( point_id, Alarms );
    for ( int ialarm=0; ialarm<atLength; ialarm++ ) {
      string rem;
      TTripAlarmsType alarm = (TTripAlarmsType)ialarm;
      if ( !Alarms.isFlag( alarm ) )
      	continue;
      switch( alarm ) {
      	case atWaitlist:
      		tst();
          if (reqInfo->screen.name == "CENT.EXE" ||
  	          reqInfo->screen.name == "PREPREG.EXE" ||
  	          reqInfo->screen.name == "AIR.EXE" ||
              reqInfo->screen.name == "BRDBUS.EXE" ||
              reqInfo->screen.name == "EXAM.EXE" ||
              reqInfo->screen.name == "DOCS.EXE") {
            tst();
            rem = TripAlarmString( alarm );
          }
          break;
        case atOverload:
          if (reqInfo->screen.name == "CENT.EXE" ||
          	  reqInfo->screen.name == "PREPREG.EXE")
          	rem = TripAlarmString( alarm );
          break;
        case atBrd:
          if (reqInfo->screen.name == "BRDBUS.EXE" ||
          	  reqInfo->screen.name == "DOCS.EXE")
          	rem = TripAlarmString( alarm );
          break;
        case atSalon:
          if (reqInfo->screen.name == "CENT.EXE" ||
  	          reqInfo->screen.name == "PREPREG.EXE" ||
  	          reqInfo->screen.name == "AIR.EXE")
          	rem = TripAlarmString( alarm );
          break;
        case atETStatus:
        	if (reqInfo->screen.name == "AIR.EXE" ||
        		  reqInfo->screen.name == "BRDBUS.EXE")
          	rem = TripAlarmString( alarm );
          break;
        case atSeance:
        	if (reqInfo->screen.name == "AIR.EXE")
          	rem = TripAlarmString( alarm );
          break;
      	default:
          break;
      }
      if ( !rem.empty() ) {
        if ( !stralarms.empty() )
        	stralarms += " ";
        stralarms += "!" + rem;
      }
    }
    if ( !stralarms.empty() ) {
      NewTextChild( node, "alarms", stralarms );
    }
  }

  if (reqInfo->screen.name == "AIR.EXE")
    NewTextChild( node, "pr_mixed_norms", (int)GetTripSets(tsMixedNorms,info) );

  if (reqInfo->screen.name == "KASSA.EXE")
  {
    NewTextChild( node, "pr_mixed_rates", (int)GetTripSets(tsMixedNorms,info) );
    NewTextChild( node, "pr_mixed_taxes", (int)GetTripSets(tsMixedNorms,info) );
  };

  return true;
}

void TripsInterface::readHalls( std::string airp_dep, std::string work_mode, xmlNodePtr dataNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT halls2.id "
    "FROM station_halls,halls2,stations "
    "WHERE station_halls.hall=halls2.id AND halls2.airp=:airp_dep AND "
    "     station_halls.airp=stations.airp AND "
    "     station_halls.station=stations.name AND "
    "     stations.desk=:desk AND stations.work_mode=:work_mode";
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.CreateVariable("desk",otString, TReqInfo::Instance()->desk.code);
  Qry.CreateVariable("work_mode",otString,work_mode);

  Qry.Execute();
  if (Qry.Eof)
  {
    Qry.Clear();
    Qry.SQLText =
      "SELECT id FROM halls2 WHERE airp=:airp_dep";
    Qry.CreateVariable("airp_dep",otString,airp_dep);
    Qry.Execute();
  };
  xmlNodePtr node = NewTextChild( dataNode, "halls" );
  for(;!Qry.Eof;Qry.Next())
  {
    xmlNodePtr itemNode = NewTextChild( node, "hall" );
    NewTextChild( itemNode, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( itemNode, "name", ElemIdToNameLong( etHall, Qry.FieldAsInteger( "id" ) ) );
  };
};

int GetFltLoad( int point_id, const TTripInfo &fltInfo)
{
  TQuery Qry(&OraSession);
  Qry.CreateVariable("point_id", otInteger, point_id);

  bool prSummer=is_dst(fltInfo.scd_out,
                       AirpTZRegion(fltInfo.airp));

  int load=0;
  //пассажиры
  Qry.SQLText=
    "SELECT NVL(SUM(weight_win),0) AS weight_win, "
    "       NVL(SUM(weight_sum),0) AS weight_sum "
    "FROM pax_grp,pax,pers_types "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax.pers_type=pers_types.code AND "
    "      pax_grp.point_dep=:point_id AND pax.refuse IS NULL";
  Qry.Execute();
  if (!Qry.Eof)
  {
    if (prSummer)
      load+=Qry.FieldAsInteger("weight_sum");
    else
      load+=Qry.FieldAsInteger("weight_win");
  };

  //багаж
  Qry.SQLText=
    "SELECT NVL(SUM(weight),0) AS weight "
    "FROM pax_grp,bag2 "
    "WHERE pax_grp.grp_id=bag2.grp_id AND "
    "      pax_grp.point_dep=:point_id AND pax_grp.bag_refuse=0";
  Qry.Execute();
  if (!Qry.Eof)
    load+=Qry.FieldAsInteger("weight");

  //груз, почта
  Qry.SQLText=
    "SELECT NVL(SUM(cargo),0) AS cargo, "
    "       NVL(SUM(mail),0) AS mail "
    "FROM trip_load, points "
    "WHERE trip_load.point_dep=:point_id AND "
    "      points.point_id=trip_load.point_arv AND "
    "      points.pr_del=0";
  Qry.Execute();
  if (!Qry.Eof)
    load+=Qry.FieldAsInteger("cargo")+Qry.FieldAsInteger("mail");
  return load;
};

class TPaxLoadItem
{
  public:
    //критерии группировки
    string class_id;
    string class_view;
    int class_priority;

    int cls_grp_id;
    string cls_grp_view;
    int cls_grp_priority;

    int hall_id;
    string hall_airp_view;
    string hall_name_view;

    int point_arv;
    string airp_arv_view;
    int airp_arv_priority;

    string trfer_airline_id;
    int trfer_flt_no;
    string trfer_suffix_id;
    string trfer_airp_arv_id;
    string trfer_view;

    int user_id;
    string user_view;

    string client_type_id;
    string client_type_view;
    int client_type_priority;

    string grp_status_id;
    string grp_status_view;
    int grp_status_priority;

    string ticket_rem;

    string rem_code;

    //данные
    int cfg; //компоновка
    int crs_ok,crs_tranzit; //данные бронирования
    int seats,adult,child,baby; //пассажиры
    int rk_weight,bag_amount,bag_weight; //багаж
    int excess; //платный вес

    TPaxLoadItem():
      class_priority(NoExists),
      cls_grp_id(NoExists),
      cls_grp_priority(NoExists),
      hall_id(NoExists),
      point_arv(NoExists),
      airp_arv_priority(NoExists),
      trfer_flt_no(NoExists),
      user_id(NoExists),
      client_type_priority(NoExists),
      grp_status_priority(NoExists),
      cfg(0),
      crs_ok(0), crs_tranzit(0),
      seats(0), adult(0), child(0), baby(0),
      rk_weight(0), bag_amount(0), bag_weight(0),
      excess(0) {};

    bool operator == (const TPaxLoadItem &item) const
    {
      return class_id == item.class_id &&
             cls_grp_id == item.cls_grp_id &&
             hall_id == item.hall_id &&
             point_arv == item.point_arv &&
             trfer_airline_id == item.trfer_airline_id &&
             trfer_flt_no == item.trfer_flt_no &&
             trfer_suffix_id == item.trfer_suffix_id &&
             trfer_airp_arv_id == item.trfer_airp_arv_id &&
             user_id == item.user_id &&
             client_type_id == item.client_type_id &&
             grp_status_id == item.grp_status_id &&
             ticket_rem == item.ticket_rem &&
             rem_code == item.rem_code;
    };
    TPaxLoadItem& operator += (const TPaxLoadItem &item)
    {
      cfg+=item.cfg;
      crs_ok+=item.crs_ok;
      crs_tranzit+=item.crs_tranzit;
      seats+=item.seats;
      adult+=item.adult;
      child+=item.child;
      baby+=item.baby;
      rk_weight+=item.rk_weight;
      bag_amount+=item.bag_amount;
      bag_weight+=item.bag_weight;
      excess+=item.excess;
      return *this;
    };

};

class TPaxLoadOrder
{
  public:
    vector<string> fields;
    bool operator () (const TPaxLoadItem &item1, const TPaxLoadItem &item2) const
    {
      for(vector<string>::const_iterator f=fields.begin();f!=fields.end();f++)
      {
        if (*f=="class")
        {
          if (item1.class_priority!=NoExists &&
              item2.class_priority!=NoExists)
          {
            if (item1.class_priority!=item2.class_priority)
              return item1.class_priority<item2.class_priority;
          }
          else
          {
            if (item1.class_priority!=item2.class_priority)
              return item1.class_priority!=NoExists;
          };
          continue;
        };
        if (*f=="cl_grp")
        {
          if (item1.cls_grp_priority!=NoExists &&
              item2.cls_grp_priority!=NoExists)
          {
            if (item1.cls_grp_priority!=item2.cls_grp_priority)
              return item1.cls_grp_priority<item2.cls_grp_priority;
          }
          else
          {
            if (item1.cls_grp_priority!=item2.cls_grp_priority)
              return item1.cls_grp_priority!=NoExists;
          };
          continue;
        };
        if (*f=="hall")
        {
          if (!item1.hall_name_view.empty() &&
              !item2.hall_name_view.empty())
          {
            if (item1.hall_airp_view!=item2.hall_airp_view)
              return item1.hall_airp_view<item2.hall_airp_view;
            if (item1.hall_name_view!=item2.hall_name_view)
              return item1.hall_name_view<item2.hall_name_view;
          }
          else
          {
            if (item1.hall_name_view!=item2.hall_name_view)
              return !item1.hall_name_view.empty();
          };
          continue;
        };
        if (*f=="airp_arv")
        {
          if (item1.airp_arv_priority!=item2.airp_arv_priority)
            return item1.airp_arv_priority<item2.airp_arv_priority;
          continue;
        };
        if (*f=="trfer")
        {
          if (item1.trfer_view!=item2.trfer_view)
            return item1.trfer_view<item2.trfer_view;
          continue;
        };
        if (*f=="user")
        {
          if (item1.user_view!=item2.user_view)
            return item1.user_view<item2.user_view;
          continue;
        };
        if (*f=="client_type")
        {
          if (item1.client_type_priority!=item2.client_type_priority)
            return item1.client_type_priority<item2.client_type_priority;
          continue;
        };
        if (*f=="status")
        {
          if (item1.grp_status_priority!=item2.grp_status_priority)
            return item1.grp_status_priority<item2.grp_status_priority;
          continue;
        };
        if (*f=="ticket_rem")
        {
          if (item1.ticket_rem!=item2.ticket_rem)
            return item1.ticket_rem<item2.ticket_rem;
          continue;
        };
        if (*f=="rems")
        {
          if (item1.rem_code!=item2.rem_code)
            return item1.rem_code<item2.rem_code;
          continue;
        };
      };
      return false;
    };
};

bool comparePaxLoadItem(const TPaxLoadItem &item1/*, const TPaxLoadItem &item2, const vector<string> &order*/)
{
  return false;
};

void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  reqNode=GetNode("tripcounters",reqNode);
  if (reqNode==NULL) return;

  resNode=NewTextChild(resNode,"tripcounters");

  //секция столбцов
  xmlNodePtr fieldsNode=NewTextChild(resNode,"fields");

  NewTextChild(fieldsNode,"field","title");
  xmlNodePtr reqFieldsNode=NodeAsNode("fields",reqNode);
  TPaxLoadOrder paxLoadOrder;
  for(xmlNodePtr node=reqFieldsNode->children;node!=NULL;node=node->next)
  {
    paxLoadOrder.fields.push_back((char*)node->name);
    NewTextChild(fieldsNode,"field",(char*)node->name);
  };
  NewTextChild(fieldsNode,"field","cfg");
  NewTextChild(fieldsNode,"field","crs_ok");
  NewTextChild(fieldsNode,"field","crs_tranzit");
  NewTextChild(fieldsNode,"field","seats");
  NewTextChild(fieldsNode,"field","adult");
  NewTextChild(fieldsNode,"field","child");
  NewTextChild(fieldsNode,"field","baby");
  NewTextChild(fieldsNode,"field","rk_weight");
  NewTextChild(fieldsNode,"field","bag_amount");
  NewTextChild(fieldsNode,"field","bag_weight");
  NewTextChild(fieldsNode,"field","excess");
  NewTextChild(fieldsNode,"field","load");

  //строка 'итого'
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points "
    "WHERE point_id=:point_id AND pr_del>=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TTripInfo fltInfo(Qry);

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
    "         NVL(SUM(DECODE(pers_type,:adult,1,0)),0) AS adult, "
    "         NVL(SUM(DECODE(pers_type,:child,1,0)),0) AS child, "
    "         NVL(SUM(DECODE(pers_type,:baby,1,0)),0) AS baby "
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
  Qry.CreateVariable("adult",otString,EncodePerson(ASTRA::adult));
  Qry.CreateVariable("child",otString,EncodePerson(ASTRA::child));
  Qry.CreateVariable("baby",otString,EncodePerson(ASTRA::baby));
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

  //секция строк
  xmlNodePtr rowsNode=NewTextChild(resNode,"rows");

  xmlNodePtr rowNode=NewTextChild(rowsNode,"row");
  NewTextChild(rowNode,"title",AstraLocale::getLocaleText("Всего"));
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
  NewTextChild(rowNode,"load",GetFltLoad(point_id,fltInfo),0);

  if (paxLoadOrder.fields.empty()) return;

  xmlNodePtr node2=reqFieldsNode->children;
  bool pr_class=      GetNodeFast("class",node2)!=NULL;
  bool pr_cl_grp=     GetNodeFast("cl_grp",node2)!=NULL;
  bool pr_hall=       GetNodeFast("hall",node2)!=NULL;
  bool pr_airp_arv=   GetNodeFast("airp_arv",node2)!=NULL;
  bool pr_trfer=      GetNodeFast("trfer",node2)!=NULL;
  bool pr_user=       GetNodeFast("user",node2)!=NULL;
  bool pr_client_type=GetNodeFast("client_type",node2)!=NULL;
  bool pr_status=     GetNodeFast("status",node2)!=NULL;
  bool pr_ticket_rem= GetNodeFast("ticket_rem",node2)!=NULL;
  bool pr_rems=       GetNodeFast("rems",node2)!=NULL;

  vector<string> rems;
  if (pr_rems)
  {
    xmlNodePtr node=NodeAsNodeFast("rems",node2)->children;
    for(;node!=NULL;node=node->next)
      rems.push_back(NodeAsString(node));
    if (rems.empty()) pr_rems=false;
  };

  list<TPaxLoadItem> paxLoad;

  for(int pass=1;pass<=5;pass++)
  {
    //1. Вычисление cfg
    //2. Вычисление crs_ok, crs_tranzit
    //3. Вычисление seats, adult, child, baby
    //4. Вычисление rk_weight, bag_amount, bag_weight
    //5. Вычисление excess
    if (pass==1 && (pr_cl_grp || pr_hall || pr_airp_arv || pr_trfer || pr_user || pr_client_type || pr_status || pr_ticket_rem || pr_rems) ||
        pass==2 && (pr_cl_grp || pr_hall || pr_trfer || pr_user || pr_client_type || pr_status || pr_ticket_rem || pr_rems) ||
        pass==4 && (pr_ticket_rem || pr_rems) ||
        pass==5 && (pr_ticket_rem || pr_rems)) continue;

    ostringstream sql,group_by;

    if (pass==1)
    {
      //запрос по компоновке
      if (pr_class)       group_by << ", trip_classes.class";

      sql << "SELECT SUM(trip_classes.cfg) AS cfg, " << endl
          << "       " << group_by.str().erase(0,1) << endl
          << "FROM trip_classes " << endl
          << "WHERE point_id=:point_id " << endl;
    };
    if (pass==2)
    {
      //запрос по брони
      if (pr_class)       group_by << ", counters2.class";
      if (pr_airp_arv)    group_by << ", counters2.point_arv";

      sql << "SELECT SUM(counters2.crs_ok) AS crs_ok, " << endl
          << "       SUM(counters2.crs_tranzit) AS crs_tranzit, " << endl
          << "       " << group_by.str().erase(0,1) << endl
          << "FROM counters2 " << endl
          << "WHERE point_dep=:point_id " << endl;
    };
    if (pass==3)
    {
      //запрос по пассажирам
      if (pr_class)       group_by << ", pax_grp.class";
      if (pr_cl_grp)      group_by << ", pax_grp.class_grp";
      if (pr_hall)        group_by << ", pax_grp.hall";
      if (pr_airp_arv)    group_by << ", pax_grp.point_arv";
      if (pr_trfer)       group_by << ", v_last_trfer.airline"
                                   << ", v_last_trfer.flt_no"
                                   << ", v_last_trfer.suffix"
                                   << ", v_last_trfer.airp_arv";
      if (pr_user)        group_by << ", pax_grp.user_id";
      if (pr_client_type) group_by << ", pax_grp.client_type";
      if (pr_status)      group_by << ", pax_grp.status";
      if (pr_ticket_rem)  group_by << ", pax.ticket_rem";
      if (pr_rems)        group_by << ", pax_rem.rem_code";

      sql << "SELECT SUM(pax.seats) AS seats, " << endl
          << "       SUM(DECODE(pax.pers_type,:adult,1,0)) AS adult, " << endl
          << "       SUM(DECODE(pax.pers_type,:child,1,0)) AS child, " << endl
          << "       SUM(DECODE(pax.pers_type,:baby,1,0)) AS baby, " << endl
          << "       " << group_by.str().erase(0,1) << endl
          << "FROM pax_grp,pax " << endl;

      if (pr_trfer) sql << "    ,v_last_trfer" << endl;
      if (pr_rems)
      {
        sql << "    ,(SELECT DISTINCT pax_id,rem_code FROM pax_rem" << endl
            << "      WHERE rem_code IN " << GetSQLEnum(rems) << endl
            << "     ) pax_rem " << endl;
      };

      sql << "WHERE pax_grp.grp_id=pax.grp_id AND " << endl
          << "      pax_grp.point_dep=:point_id AND pax.pr_brd IS NOT NULL " << endl;
      if (pr_trfer) sql << "      AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;
      if (pr_rems)  sql << "      AND pax.pax_id=pax_rem.pax_id " << endl;
    };
    if (pass==4)
    {
      //запрос по багажу
      if (pr_class)       group_by << ", pax_grp.class";
      if (pr_cl_grp)      group_by << ", pax_grp.class_grp";
      if (pr_hall)        group_by << ", bag2.hall";
      if (pr_airp_arv)    group_by << ", pax_grp.point_arv";
      if (pr_trfer)       group_by << ", v_last_trfer.airline"
                                   << ", v_last_trfer.flt_no"
                                   << ", v_last_trfer.suffix"
                                   << ", v_last_trfer.airp_arv";
      if (pr_user)        group_by << ", bag2.user_id";
      if (pr_client_type) group_by << ", pax_grp.client_type";
      if (pr_status)      group_by << ", pax_grp.status";

      sql << "SELECT SUM(DECODE(bag2.pr_cabin,0,bag2.amount,0)) AS bag_amount, " << endl
          << "       SUM(DECODE(bag2.pr_cabin,0,bag2.weight,0)) AS bag_weight, " << endl
          << "       SUM(DECODE(bag2.pr_cabin,0,0,bag2.weight)) AS rk_weight, " << endl
          << "       " << group_by.str().erase(0,1) << endl
          << "FROM pax_grp,bag2 " << endl;

      if (pr_trfer) sql << "    ,v_last_trfer" << endl;

      sql << "WHERE pax_grp.grp_id=bag2.grp_id AND " << endl
          << "      pax_grp.point_dep=:point_id AND pax_grp.bag_refuse=0 " << endl;
      if (pr_trfer) sql << "      AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;
    };
    if (pass==5)
    {
      //запрос по платному багажу
      if (pr_class)       group_by << ", pax_grp.class";
      if (pr_cl_grp)      group_by << ", pax_grp.class_grp";
      if (pr_airp_arv)    group_by << ", pax_grp.point_arv";
      if (pr_trfer)       group_by << ", v_last_trfer.airline"
                                   << ", v_last_trfer.flt_no"
                                   << ", v_last_trfer.suffix"
                                   << ", v_last_trfer.airp_arv";
      if (pr_client_type) group_by << ", pax_grp.client_type";
      if (pr_status)      group_by << ", pax_grp.status";
      
      ostringstream select;
      select << group_by.str();
      if (pr_hall)      { select   << ", NVL(bag2.hall,pax_grp.hall) AS hall";
                          group_by << ", NVL(bag2.hall,pax_grp.hall)"; };
      if (pr_user)      { select   << ", NVL(bag2.user_id,pax_grp.user_id) AS user_id";
                          group_by << ", NVL(bag2.user_id,pax_grp.user_id)"; };

      sql << "SELECT SUM(excess) AS excess, " << endl
          << "       " << select.str().erase(0,1) << endl
          << "FROM pax_grp " << endl;
          
      if (pr_trfer)
        sql << "    ,v_last_trfer" << endl;
      
      if (pr_hall || pr_user)
        sql << "    ,(SELECT bag2.grp_id,bag2.hall,bag2.user_id " << endl
            << "     FROM bag2, " << endl
            << "          (SELECT bag2.grp_id,MAX(bag2.num) AS num " << endl
            << "           FROM pax_grp,bag2 " << endl
            << "           WHERE pax_grp.grp_id=bag2.grp_id AND pax_grp.point_dep=:point_id " << endl
            << "           GROUP BY bag2.grp_id) last_bag " << endl
            << "     WHERE bag2.grp_id=last_bag.grp_id AND bag2.num=last_bag.num) bag2 " << endl;

      sql << "WHERE pax_grp.point_dep=:point_id AND pax_grp.bag_refuse=0 " << endl;
      if (pr_trfer)
        sql << "      AND pax_grp.grp_id=v_last_trfer.grp_id(+) " << endl;
      if (pr_hall || pr_user)
        sql << "      AND pax_grp.grp_id=bag2.grp_id(+) " << endl;
    };

    sql << "GROUP BY " << group_by.str().erase(0,1) << endl;

    Qry.Clear();
    Qry.SQLText = sql.str().c_str();
    Qry.CreateVariable("point_id",otInteger,point_id);
    if (pass==3)
    {
      Qry.CreateVariable("adult",otString,EncodePerson(ASTRA::adult));
      Qry.CreateVariable("child",otString,EncodePerson(ASTRA::child));
      Qry.CreateVariable("baby",otString,EncodePerson(ASTRA::baby));
    };
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TPaxLoadItem item;
      if (pr_class) item.class_id=Qry.FieldAsString("class");
      if (pr_cl_grp && !Qry.FieldIsNULL("class_grp")) item.cls_grp_id=Qry.FieldAsInteger("class_grp");
      if (pr_hall && !Qry.FieldIsNULL("hall")) item.hall_id=Qry.FieldAsInteger("hall");
      if (pr_airp_arv) item.point_arv=Qry.FieldAsInteger("point_arv");
      if (pr_trfer)
      {
        item.trfer_airline_id=Qry.FieldAsString("airline");
        if (!Qry.FieldIsNULL("flt_no"))
          item.trfer_flt_no=Qry.FieldAsInteger("flt_no");
        item.trfer_suffix_id=Qry.FieldAsString("suffix");
        item.trfer_airp_arv_id=Qry.FieldAsString("airp_arv");
      };
      if (pr_user) item.user_id=Qry.FieldAsInteger("user_id");
      if (pr_client_type) item.client_type_id=Qry.FieldAsString("client_type");
      if (pr_status) item.grp_status_id=Qry.FieldAsString("status");
      if (pr_ticket_rem) item.ticket_rem=Qry.FieldAsString("ticket_rem");
      if (pr_rems) item.rem_code=Qry.FieldAsString("rem_code");

      if (pass==1)
      {
        item.cfg=Qry.FieldAsInteger("cfg");
      };
      if (pass==2)
      {
        item.crs_ok=Qry.FieldAsInteger("crs_ok");
        item.crs_tranzit=Qry.FieldAsInteger("crs_tranzit");
      };
      if (pass==3)
      {
        item.seats=Qry.FieldAsInteger("seats");
        item.adult=Qry.FieldAsInteger("adult");
        item.child=Qry.FieldAsInteger("child");
        item.baby=Qry.FieldAsInteger("baby");
      };
      if (pass==4)
      {
        item.rk_weight=Qry.FieldAsInteger("rk_weight");
        item.bag_amount=Qry.FieldAsInteger("bag_amount");
        item.bag_weight=Qry.FieldAsInteger("bag_weight");
      };
      if (pass==5)
      {
        item.excess=Qry.FieldAsInteger("excess");
      };

      list<TPaxLoadItem>::iterator i=find(paxLoad.begin(), paxLoad.end(), item);
      if (i!=paxLoad.end())
        *i+=item;
      else
        paxLoad.push_back(item);
    };
  };

  //заполняем все недостающие поля TPaxLoadItem

  TQuery HallsQry(&OraSession);
  HallsQry.Clear();
  HallsQry.SQLText="SELECT airp FROM halls2 WHERE id=:id";
  HallsQry.DeclareVariable("id",otInteger);
  map<int, pair<string,string> > halls; //кэшируем информацию по залам

  TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText="SELECT airp,point_num FROM points WHERE point_id=:point_id AND pr_del>=0";
  PointsQry.DeclareVariable("point_id",otInteger);
  map<int, pair<string,int> > points; //кэшируем информацию по пунктам

  TQuery UsersQry(&OraSession);
  UsersQry.Clear();
  UsersQry.SQLText="SELECT descr FROM users2 WHERE user_id=:user_id AND pr_denial>=0";
  UsersQry.DeclareVariable("user_id",otInteger);
  map<int, string> users; //кэшируем информацию по агентам

  for(list<TPaxLoadItem>::iterator i=paxLoad.begin();i!=paxLoad.end();i++)
  {
    if (!i->class_id.empty())
    {
      i->class_view=ElemIdToCodeNative(etClass,i->class_id);
      try
      {
        i->class_priority=getBaseTable(etClass).get_row("code",i->class_id,true).AsInteger("priority");
      }
      catch(EBaseTableError) { throw; };
    };
    if (i->cls_grp_id!=NoExists)
    {
      i->cls_grp_view=ElemIdToCodeNative(etClsGrp,i->cls_grp_id);
      try
      {
        i->cls_grp_priority=getBaseTable(etClsGrp).get_row("id",i->cls_grp_id,true).AsInteger("priority");
      }
      catch(EBaseTableError) { throw; };
    };
    if (i->hall_id!=NoExists)
    {
      map<int, pair<string,string> >::const_iterator h=halls.find(i->hall_id);
      if (h!=halls.end())
      {
        i->hall_airp_view=h->second.first;
        i->hall_name_view=h->second.second;
      }
      else
      {
        HallsQry.SetVariable("id",i->hall_id);
        HallsQry.Execute();
        if (!HallsQry.Eof)
        {
          if (HallsQry.FieldAsString("airp")!=fltInfo.airp)
            i->hall_airp_view=ElemIdToCodeNative(etAirp, HallsQry.FieldAsString("airp"));
          else
            i->hall_airp_view="";

          i->hall_name_view=ElemIdToNameLong(etHall, i->hall_id);
        };
        halls[i->hall_id]=make_pair(i->hall_airp_view,i->hall_name_view);
      };
    };
    if (i->point_arv!=NoExists)
    {
      map<int, pair<string,int> >::const_iterator p=points.find(i->point_arv);
      if (p!=points.end())
      {
        i->airp_arv_view=p->second.first;
        i->airp_arv_priority=p->second.second;
      }
      else
      {
        PointsQry.SetVariable("point_id",i->point_arv);
        PointsQry.Execute();
        if (!PointsQry.Eof)
        {
          i->airp_arv_view=ElemIdToCodeNative(etAirp, PointsQry.FieldAsString("airp"));
          i->airp_arv_priority=PointsQry.FieldAsInteger("point_num");
        };
        points[i->point_arv]=make_pair(i->airp_arv_view,i->airp_arv_priority);
      };
    };
    TLastTrferInfo trferInfo;
    trferInfo.airline=i->trfer_airline_id;
    trferInfo.flt_no=i->trfer_flt_no;
    trferInfo.suffix=i->trfer_suffix_id;
    trferInfo.airp_arv=i->trfer_airp_arv_id;
    i->trfer_view=trferInfo.str();
    if (i->user_id!=NoExists)
    {
      map<int, string>::const_iterator u=users.find(i->user_id);
      if (u!=users.end())
      {
        i->user_view=u->second;
      }
      else
      {
        UsersQry.SetVariable("user_id",i->user_id);
        UsersQry.Execute();
        if (!UsersQry.Eof)
        {
          i->user_view=UsersQry.FieldAsString("descr");
        };
        users[i->user_id]=i->user_view;
      };
    };
    if (!i->client_type_id.empty())
    {
      i->client_type_view=ElemIdToNameShort(etClientType,i->client_type_id);
      try
      {
        i->client_type_priority=getBaseTable(etClientType).get_row("code",i->client_type_id,true).AsInteger("priority");
      }
      catch(EBaseTableError) { throw; };
    };
    if (!i->grp_status_id.empty())
    {
      i->grp_status_view=ElemIdToNameLong(etGrpStatusType,i->grp_status_id);
      try
      {
        i->grp_status_priority=getBaseTable(etGrpStatusType).get_row("code",i->grp_status_id,true).AsInteger("priority");
      }
      catch(EBaseTableError) { throw; };
    };
  };

  //сортируем массив
  paxLoad.sort(paxLoadOrder);

  xmlNodePtr node;
  for(list<TPaxLoadItem>::iterator i=paxLoad.begin();i!=paxLoad.end();i++)
  {
    rowNode=NewTextChild(rowsNode,"row");
    NewTextChild(rowNode,"seats",i->seats,0);
    NewTextChild(rowNode,"adult",i->adult,0);
    NewTextChild(rowNode,"child",i->child,0);
    NewTextChild(rowNode,"baby",i->baby,0);

    if (!pr_ticket_rem && !pr_rems)
    {
      NewTextChild(rowNode,"bag_amount",i->bag_amount,0);
      NewTextChild(rowNode,"bag_weight",i->bag_weight,0);
      NewTextChild(rowNode,"rk_weight",i->rk_weight,0);
      NewTextChild(rowNode,"excess",i->excess,0);

      if (!pr_cl_grp && !pr_trfer && !pr_client_type && !pr_status && !pr_hall && !pr_user)
      {
        NewTextChild(rowNode,"crs_ok",i->crs_ok,0);
        NewTextChild(rowNode,"crs_tranzit",i->crs_tranzit,0);
        if (!pr_airp_arv)
          NewTextChild(rowNode,"cfg",i->cfg,0);
      };
    };

    if (pr_class)
    {
      NewTextChild(rowNode,"class",i->class_view);
      if (i->class_id.empty())
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("Несопр"));
    };
    if (pr_cl_grp)
    {
      if (i->cls_grp_id!=NoExists)
      {
        node=NewTextChild(rowNode,"cl_grp",i->cls_grp_view);
        SetProp(node,"id",i->cls_grp_id);
      }
      else
      {
        node=NewTextChild(rowNode,"cl_grp");
        SetProp(node,"id",0);
        ReplaceTextChild(rowNode,"title",AstraLocale::getLocaleText("Несопр"));
      };
    };
    if (pr_hall)
    {
      if (i->hall_id!=NoExists)
      {
        ostringstream hall_name;
        hall_name << i->hall_name_view;
        if (!i->hall_airp_view.empty())
          hall_name << "(" << i->hall_airp_view << ")";
        node=NewTextChild(rowNode,"hall",hall_name.str());
        SetProp(node,"id",i->hall_id);
      }
      else
      {
        node=NewTextChild(rowNode,"hall");
        SetProp(node,"id",-1);
      };
    };
    if (pr_airp_arv)
    {
      node=NewTextChild(rowNode,"airp_arv",i->airp_arv_view);
      SetProp(node,"id",i->point_arv);
    };
    if (pr_trfer)
    {
      NewTextChild(rowNode,"trfer",i->trfer_view);
    };
    if (pr_user)
    {
      node=NewTextChild(rowNode,"user",i->user_view);
      SetProp(node,"id",i->user_id);
    };
    if (pr_client_type)
    {
      node=NewTextChild(rowNode,"client_type",i->client_type_view);
      SetProp(node,"id",(int)DecodeClientType(i->client_type_id.c_str()));
    };
    if (pr_status)
    {
      node=NewTextChild(rowNode,"status",i->grp_status_view);
      SetProp(node,"id",(int)DecodePaxStatus(i->grp_status_id.c_str()));
    };
    if (pr_ticket_rem)
    {
      NewTextChild(rowNode,"ticket_rem",i->ticket_rem);
    };
    if (pr_rems)
    {
      NewTextChild(rowNode,"rems",i->rem_code);
    };
  };
};

void viewCRSList( int point_id, xmlNodePtr dataNode )
{
	TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  TQuery Qry( &OraSession );
  TPaxSeats priorSeats( point_id );
  Qry.Clear();
  ostringstream sql;

  sql <<
     "SELECT "
     "      ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr_ref, "
     "      crs_pnr.status AS pnr_status, "
     "      crs_pnr.priority AS pnr_priority, "
     "      RTRIM(crs_pax.surname||' '||crs_pax.name) full_name, "
     "      crs_pax.pers_type, "
     "      crs_pnr.class,crs_pnr.subclass, "
     "      crs_pax.seat_xname, "
     "      crs_pax.seat_yname, "
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
     "      pax.seats pax_seats, "
     "      pax_grp.status grp_status, "
     "      pax.refuse, "
     "      pax.grp_id, "
     "      pax.wl_type "
     "FROM crs_pnr,crs_pax,pax,pax_grp,"
     "       ( ";


  sql << CheckInInterface::GetSearchPaxSubquery(psCheckin, true, false, false, false, "")
      << "UNION \n"
      << CheckInInterface::GetSearchPaxSubquery(psGoshow,  true, false, false, false, "")
      << "UNION \n"
      << CheckInInterface::GetSearchPaxSubquery(psTransit, true, false, false, false, "");

  sql <<
     "       ) ids "
     "WHERE crs_pnr.pnr_id=ids.pnr_id AND "
     "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
     "      crs_pax.pax_id=pax.pax_id(+) AND "
     "      pax.grp_id=pax_grp.grp_id(+) AND "
     "      crs_pax.pr_del=0 "
     "ORDER BY crs_pnr.point_id";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
  Qry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
  Qry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
  Qry.Execute();
  // места пассажира
  TQuery SQry( &OraSession );
  SQry.SQLText =
    "BEGIN "
    " IF :mode=0 THEN "
    "  :seat_no:=salons.get_seat_no(:pax_id,:seats,:layer_type,:point_id,'_seats',:pax_row); "
    " ELSE "
    "  :seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'_seats',:crs_row); "
    " END IF; "
    "END;";
  SQry.DeclareVariable( "mode", otInteger );
  SQry.DeclareVariable( "pax_id", otInteger );
  SQry.DeclareVariable( "xname", otString );
  SQry.DeclareVariable( "yname", otString );
  SQry.DeclareVariable( "layer_type", otString );
  SQry.DeclareVariable( "seats", otInteger );
  SQry.DeclareVariable( "point_id", otInteger );
  SQry.DeclareVariable( "pax_row", otInteger );
  SQry.DeclareVariable( "crs_row", otInteger );
  SQry.DeclareVariable( "seat_no", otString );
  // старые места пассажира
  TQuery QrySeat( &OraSession );
  QrySeat.SQLText =
    "SELECT first_xname, first_yname, layer_type FROM trip_comp_layers, comp_layer_types "
    " WHERE point_id=:point_id AND "
    "       pax_id=:pax_id AND "
    "       trip_comp_layers.layer_type=comp_layer_types.code AND "
    "       comp_layer_types.pr_occupy=1 "
    "ORDER BY priority ASC, time_create DESC";
  QrySeat.CreateVariable( "point_id", otInteger, point_id );
  QrySeat.DeclareVariable( "pax_id", otInteger );

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

  xmlNodePtr tripsNode = NewTextChild( dataNode, "tlg_trips" );
  Qry.Execute();
  if (Qry.Eof) return;

  string def_pers_type=EncodePerson(ASTRA::adult); //специально не перекодируем, так как идет подсчет по типам
  string def_class=ElemIdToCodeNative(etClass, EncodeClass(ASTRA::Y));
  string def_status=EncodePaxStatus(ASTRA::psCheckin);

  xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
  NewTextChild(defNode, "pnr_ref", "");
  NewTextChild(defNode, "pnr_status", "");
  NewTextChild(defNode, "pnr_priority", "");
  NewTextChild(defNode, "pers_type", def_pers_type);
  NewTextChild(defNode, "class", def_class);
  NewTextChild(defNode, "seats", 1);
  NewTextChild(defNode, "last_target", "");
  NewTextChild(defNode, "ticket", "");
  NewTextChild(defNode, "document", "");
  NewTextChild(defNode, "status", def_status);
  NewTextChild(defNode, "rem", "");
  NewTextChild(defNode, "nseat_no", "");
  NewTextChild(defNode, "wl_type", "");
  NewTextChild(defNode, "layer_type", "");
  NewTextChild(defNode, "isseat", 1);
  NewTextChild(defNode, "reg_no", "");
  NewTextChild(defNode, "refuse", "");

  int point_id_tlg=-1;
  xmlNodePtr tripNode,paxNode,node;
  int col_pnr_ref=Qry.FieldIndex("pnr_ref");
  int col_pnr_status=Qry.FieldIndex("pnr_status");
  int col_pnr_priority=Qry.FieldIndex("pnr_priority");
  int col_full_name=Qry.FieldIndex("full_name");
  int col_pers_type=Qry.FieldIndex("pers_type");
  int col_class=Qry.FieldIndex("class");
  int col_subclass=Qry.FieldIndex("subclass");
  int col_seat_xname=Qry.FieldIndex("seat_xname");
  int col_seat_yname=Qry.FieldIndex("seat_yname");
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
  int col_refuse=Qry.FieldIndex("refuse");
  int col_grp_id=Qry.FieldIndex("grp_id");
  int col_grp_status=Qry.FieldIndex("grp_status");
  int col_pax_seats=Qry.FieldIndex("pax_seats");
  int col_wl_type=Qry.FieldIndex("wl_type");
  int mode; // режим для поиска мест 0 - регистрация иначе список pnl
  int crs_row=1, pax_row=1;
  for(;!Qry.Eof;Qry.Next())
  {
  	mode = -1; // не надо искать место
  	string seat_no;
    if (!Qry.FieldIsNULL(col_grp_id))
    {
      PointsQry.SetVariable("grp_id",Qry.FieldAsInteger(col_grp_id));
      PointsQry.Execute();
      if (!PointsQry.Eof&&point_id!=PointsQry.FieldAsInteger("point_id")) continue;
    };

    if (point_id_tlg!=Qry.FieldAsInteger(col_point_id_tlg))
    {
      point_id_tlg=Qry.FieldAsInteger(col_point_id_tlg);
      tripNode = NewTextChild( tripsNode, "tlg_trip" );
      TlgTripsQry.SetVariable("point_id",point_id_tlg);
      TlgTripsQry.Execute();
      if (TlgTripsQry.Eof) throw AstraLocale::UserException("MSG.FLT.NOT_FOUND.REPEAT_QRY");
      ostringstream trip;
      trip << ElemIdToCodeNative(etAirline,TlgTripsQry.FieldAsString("airline") )
           << setw(3) << setfill('0') << TlgTripsQry.FieldAsInteger("flt_no")
           << ElemIdToCodeNative(etSuffix,TlgTripsQry.FieldAsString("suffix"))
           << "/" << DateTimeToStr(TlgTripsQry.FieldAsDateTime("scd"),"ddmmm",TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU)
           << " " << ElemIdToCodeNative(etAirp,TlgTripsQry.FieldAsString("airp_dep"));
      NewTextChild(tripNode,"name",trip.str());
      paxNode = NewTextChild(tripNode,"passengers");
    };
    node = NewTextChild(paxNode,"pax");

    NewTextChild( node, "pnr_ref", Qry.FieldAsString( col_pnr_ref ), "" );
    NewTextChild( node, "pnr_status", Qry.FieldAsString( col_pnr_status ), "" );
    NewTextChild( node, "pnr_priority", Qry.FieldAsString( col_pnr_priority ), "" );
    NewTextChild( node, "full_name", Qry.FieldAsString( col_full_name ) );
    NewTextChild( node, "pers_type", Qry.FieldAsString( col_pers_type ), def_pers_type ); //специально не перекодируем, так как идет подсчет по типам
    NewTextChild( node, "class", ElemIdToCodeNative(etClass,Qry.FieldAsString( col_class )), def_class );
    NewTextChild( node, "subclass", ElemIdToCodeNative(etSubcls,Qry.FieldAsString( col_subclass ) ));
    NewTextChild( node, "seats", Qry.FieldAsInteger( col_seats ), 1 );
    NewTextChild( node, "target", ElemIdToCodeNative(etAirp,Qry.FieldAsString( col_target ) ));
    if (!Qry.FieldIsNULL(col_last_target))
    {
      try
      {
        TAirpsRow &row=(TAirpsRow&)(base_tables.get("airps").get_row("code/code_lat",Qry.FieldAsString( col_last_target )));
        NewTextChild( node, "last_target", ElemIdToCodeNative(etAirp,row.code));
      }
      catch(EBaseTableError)
      {
        NewTextChild( node, "last_target", ElemIdToCodeNative(etAirp,Qry.FieldAsString( col_last_target ) ));
      };
    };

    NewTextChild( node, "ticket", Qry.FieldAsString( col_ticket ), "" );

    if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
      NewTextChild( node, "document", Qry.FieldAsString( col_document ), "" );
    else
      NewTextChild( node, "document", Qry.FieldAsString( col_document ) );

    NewTextChild( node, "status", Qry.FieldAsString( col_status ), def_status );

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
   	if ( !Qry.FieldIsNULL( col_wl_type ) )
   		NewTextChild( node, "wl_type", Qry.FieldAsString( col_wl_type ) );

    if (!Qry.FieldIsNULL(col_grp_id))
    {
      NewTextChild( node, "reg_no", Qry.FieldAsInteger( col_reg_no ) );
      NewTextChild( node, "refuse", Qry.FieldAsString( col_refuse ), "" );
      if ( !Qry.FieldIsNULL( col_refuse ) )
      	continue; // не надо искать место
      mode = 0;
      SQry.SetVariable( "layer_type", Qry.FieldAsString( col_grp_status ) );
      SQry.SetVariable( "seats", Qry.FieldAsInteger(col_pax_seats)  );
      SQry.SetVariable( "point_id", point_id );
      SQry.SetVariable( "pax_row", pax_row );
      ProgTrace( TRACE5, "mode=%d, pax_id=%d, seats=%d, point_id=%d, pax_row=%d, layer_type=%s",
                         mode, Qry.FieldAsInteger( col_pax_id ), Qry.FieldAsInteger(col_pax_seats), point_id,
                         pax_row, Qry.FieldAsString( col_grp_status ) );
    }
    else {
    	mode = 1;
    	SQry.SetVariable( "xname", Qry.FieldAsString( col_seat_xname ) );
    	SQry.SetVariable( "yname", Qry.FieldAsString( col_seat_yname ) );
    	SQry.SetVariable( "layer_type", FNull );
    	SQry.SetVariable( "seats", Qry.FieldAsInteger(col_seats)  );
    	SQry.SetVariable( "point_id", Qry.FieldAsInteger(col_point_id_tlg) );
    	SQry.SetVariable( "crs_row", crs_row );
      ProgTrace( TRACE5, "mode=%d, pax_id=%d, seats=%d, point_id=%d, crs_row=%d, layer_type=%s",
                         mode, Qry.FieldAsInteger( col_pax_id ), Qry.FieldAsInteger(col_seats), point_id,
                         crs_row, "" );
    }
    SQry.SetVariable( "mode", mode );
    SQry.SetVariable( "pax_id", Qry.FieldAsInteger( col_pax_id ) );
    SQry.SetVariable( "seat_no", FNull );
    SQry.Execute();
    if ( mode == 0 )
    	pax_row++;
    else
    	crs_row++;
    NewTextChild( node, "isseat", (!SQry.VariableIsNULL( "seat_no" ) || !Qry.FieldAsInteger( col_seats ) ) );
    if ( !SQry.VariableIsNULL( "seat_no" ) ) {
    	string seat_no = SQry.GetVariableAsString( "seat_no" );
    	string layer_type;
    	if ( mode == 0 ) {
    		layer_type = ((TGrpStatusTypesRow&)grp_status_types.get_row("code",Qry.FieldAsString( col_grp_status ))).layer_type;
    	}
    	else {
    		layer_type = SQry.GetVariableAsString( "layer_type" );
    	}
    	switch ( DecodeCompLayerType( (char*)layer_type.c_str() ) ) { // 12.12.08 для совместимости со старой версией
    		case cltProtCkin:
    			NewTextChild( node, "preseat_no", seat_no );
    			NewTextChild( node, "crs_seat_no", string(Qry.FieldAsString( col_seat_xname ))+Qry.FieldAsString( col_seat_yname ) );
    			break;
    		case cltPNLCkin:
    			NewTextChild( node, "crs_seat_no", seat_no );
    			break;
    		default:
    			NewTextChild( node, "seat_no", seat_no );
    			break;
    	}
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
      	seat_no = LTrimString( seat_no );
    	NewTextChild( node, "nseat_no", seat_no );
   		NewTextChild( node, "layer_type", layer_type );
    } // не задано место
    else
    	if ( mode == 0 && Qry.FieldAsInteger( col_seats ) ) {
    		string old_seat_no;
    		if ( Qry.FieldIsNULL( col_wl_type ) ) {
    		  old_seat_no = priorSeats.getSeats( Qry.FieldAsInteger( col_pax_id ), "seats" );
    		  if ( !old_seat_no.empty() )
    		  	old_seat_no = "(" + old_seat_no + ")";
    		}
    		else
    			old_seat_no = AstraLocale::getLocaleText("ЛО");
    		if ( !old_seat_no.empty() )
    		  NewTextChild( node, "nseat_no", old_seat_no );
   		}
  };

};

bool Calc_overload_alarm( int point_id, const TTripInfo &fltInfo )
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT max_commerce FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  if (Qry.FieldIsNULL("max_commerce")) return false;
	int load=GetFltLoad(point_id,fltInfo);
  ProgTrace(TRACE5,"max_commerce=%d load=%d",Qry.FieldAsInteger("max_commerce"),load);
	return (load>Qry.FieldAsInteger("max_commerce"));
}

void Set_overload_alarm( int point_id, bool overload_alarm )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT overload_alarm FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if ( !Qry.Eof && (int)overload_alarm != Qry.FieldAsInteger( "overload_alarm" ) ) {
    Qry.Clear();
    Qry.SQLText=
      "UPDATE trip_sets SET overload_alarm=:overload_alarm WHERE point_id=:point_id";
    Qry.CreateVariable("overload_alarm", otInteger, overload_alarm);
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
  	string msg = "Тревога 'Перегрузка'";
  	if ( overload_alarm )
  		msg += " установлена";
  	else
  		msg += " отменена";
  	TReqInfo::Instance()->MsgToLog( msg, evtFlt, point_id );
  }
}


/* есть пассажиры, которые на листе ожидания */
bool check_waitlist_alarm( int point_id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT waitlist_alarm FROM trip_sets WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	SEATS2::TPassengers p;
	bool waitlist_alarm = SEATS2::GetPassengersForWaitList( point_id, p, true );
	if ( !Qry.Eof && (int)waitlist_alarm != Qry.FieldAsInteger( "waitlist_alarm" ) ) {
		Qry.Clear();
		Qry.SQLText =
		  "UPDATE trip_sets SET waitlist_alarm=:waitlist_alarm WHERE point_id=:point_id";
	  Qry.CreateVariable( "point_id", otInteger, point_id );
	  Qry.CreateVariable( "waitlist_alarm", otInteger, waitlist_alarm );
  	Qry.Execute();
  	string msg = "Тревога 'Лист ожидания'";
  	if ( waitlist_alarm )
  		msg += " установлена";
  	else
  		msg += " отменена";
  	TReqInfo::Instance()->MsgToLog( msg, evtFlt, point_id );
	}
	return waitlist_alarm;
}

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
	Qry.Clear();
	Qry.SQLText =
	  "SELECT brd_alarm FROM trip_sets WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( !Qry.Eof && (int)brd_alarm != Qry.FieldAsInteger( "brd_alarm" ) ) {
		Qry.Clear();
		Qry.SQLText =
		  "UPDATE trip_sets SET brd_alarm=:brd_alarm WHERE point_id=:point_id";
	  Qry.CreateVariable( "point_id", otInteger, point_id );
	  Qry.CreateVariable( "brd_alarm", otInteger, brd_alarm );
  	Qry.Execute();
  	string msg = "Тревога 'Посадка'";
  	if ( brd_alarm )
  		msg += " установлена";
  	else
  		msg += " отменена";
  	TReqInfo::Instance()->MsgToLog( msg, evtFlt, point_id );
  }
	return brd_alarm;
}

