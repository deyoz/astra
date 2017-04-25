#include <string>

#include "oralib.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_context.h"
#include "convert.h"
#include "astra_date_time.h"
#include "misc.h"
#include "astra_misc.h"
#include "points.h"
#include "stages.h"
#include "astra_service.h"
#include "meridian.h"
#include "astra_callbacks.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace BASIC::date_time;
using namespace AstraLocale;

namespace MERIDIAN {

bool is_sync_meridian( const TTripInfo &tripInfo )
{
  return GetTripSets( tsSyncMeridian, tripInfo );
}

////////////////////////////////////MERIDIAN SYSTEM/////////////////////////////
void GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string airline;
  int flt_no;
  string str_flt_no;
  string suffix;
  string str_scd_out;
  TDateTime scd_out;
  string airp_dep;
  string region;
  TElemFmt fmt;

  xmlNodePtr node = GetNode( "airline", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airline' not found" );
  airline = NodeAsString( node );
  airline = ElemToElemId( etAirline, airline, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRLINE.INVALID",
                           LParams()<<LParam("airline",NodeAsString(node)) );
  node = GetNode( "flt_no", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'flt_no' not found" );
  str_flt_no =  NodeAsString( node );
    if ( StrToInt( str_flt_no.c_str(), flt_no ) == EOF ||
           flt_no > 99999 || flt_no <= 0 )
        throw UserException( "MSG.FLT_NO.INVALID",
                               LParams()<<LParam("flt_no", str_flt_no) );
  node = GetNode( "suffix", reqNode );
  if ( node != NULL ) {
    suffix =  NodeAsString( node );
    if ( !suffix.empty() ) {
      suffix = ElemToElemId( etSuffix, suffix, fmt );
      if ( fmt == efmtUnknown )
        throw UserException( "MSG.SUFFIX.INVALID",
                               LParams()<<LParam("suffix",NodeAsString(node)) );
    }
  }
  node = GetNode( "scd_out", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'scd_out' not found" );
  str_scd_out = NodeAsString( node );
  ProgTrace( TRACE5, "str_scd_out=|%s|", str_scd_out.c_str() );
  if ( str_scd_out.empty() )
        throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
    else
        if ( StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn", scd_out ) == EOF )
            throw UserException( "MSG.FLIGHT_DATE.INVALID",
                                   LParams()<<LParam("scd_out", str_scd_out) );
    node = GetNode( "airp_dep", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airp_dep' not found" );
  airp_dep = NodeAsString( node );
  airp_dep = ElemToElemId( etAirp, airp_dep, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRP.INVALID_INPUT_VALUE",
                           LParams()<<LParam("airp",NodeAsString(node)) );
  TReqInfo *reqInfo = TReqInfo::Instance();
  region = AirpTZRegion( airp_dep );
  scd_out = LocalToUTC( scd_out, region );
  ProgTrace( TRACE5, "scd_out=%f", scd_out );
  if ( !reqInfo->user.access.airlines().permitted( airline ) ||
       !reqInfo->user.access.airps().permitted( airp_dep ) )
    throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );

  int findMove_id, point_id;
  if ( !TPoints::isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp_dep, ASTRA::NoExists, scd_out,
        findMove_id, point_id  ) )
     throw UserException( "MSG.FLIGHT.NOT_FOUND" );

    TFlightStations stations;
    stations.Load( point_id );
    TFlightStages stages;
    stages.Load( point_id );
    TCkinClients CkinClients;
 TTripStages::ReadCkinClients( point_id, CkinClients );
    xmlNodePtr flightNode = NewTextChild( resNode, "trip" );
    airline += str_flt_no + suffix;
    SetProp( flightNode, "flightNumber", airline );
  SetProp( flightNode, "date", DateTimeToStr( UTCToClient( scd_out, region ), "dd.mm.yyyy hh:nn" ) );
  SetProp( flightNode, "departureAirport", airp_dep );
  node = NewTextChild( flightNode, "stages" );
  CreateXMLStage( CkinClients, sPrepCheckIn, stages.GetStage( sPrepCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenCheckIn, stages.GetStage( sOpenCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseCheckIn, stages.GetStage( sCloseCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenBoarding, stages.GetStage( sOpenBoarding ), node, region );
  CreateXMLStage( CkinClients, sCloseBoarding, stages.GetStage( sCloseBoarding ), node, region );
  CreateXMLStage( CkinClients, sOpenWEBCheckIn, stages.GetStage( sOpenWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseWEBCheckIn, stages.GetStage( sCloseWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenKIOSKCheckIn, stages.GetStage( sOpenKIOSKCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseKIOSKCheckIn, stages.GetStage( sCloseKIOSKCheckIn ), node, region );
  tstations sts;
  stations.Get( sts );
  xmlNodePtr node1 = NULL;
  for ( tstations::iterator i=sts.begin(); i!=sts.end(); i++ ) {
    if ( node1 == NULL )
      node1 = NewTextChild( flightNode, "stations" );
    SetProp( NewTextChild( node1, "station", i->name ), "work_mode", i->work_mode );
  }
}

struct Tids {
  int pax_tid;
  int grp_tid;
  Tids( ) {
    pax_tid = -1;
    grp_tid = -1;
  }
  Tids( int vpax_tid, int vgrp_tid ) {
    pax_tid = vpax_tid;
    grp_tid = vgrp_tid;
  }
};
/*
bool checkAccess( const std::string &airline, const TTripInfo &tripInfo, const std::set<std::string> &airps ) {
  return ( airline == tripInfo.airline || airps.find( tripInfo.airp ) != airps.end() );
}
*/
string getMeridianContextName( const string airline ) {
  string ret = airline + "_meridian_sync";
  return ret;
}

void GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "@time", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag '@time' not found" );
  string str_date = NodeAsString( node );
  node = GetNode( "@airline", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag '@airline' not found" );
  string airline = NodeAsString( node );
  TElemFmt fmt;
  airline = ElemToElemId( etAirline, airline, fmt );
  if ( fmt == efmtUnknown ) {
    throw AstraLocale::UserException( "Tag '@airline' unknown airline" );
  }
  if ( !TReqInfo::Instance()->user.access.airlines().permitted( airline ) ) {
    throw AstraLocale::UserException( "Airline is not permit for user" );
  }
  if (  TReqInfo::Instance()->client_type != ctWeb ) {
      throw AstraLocale::UserException( "Invalid client type" );
  }
  TDateTime vdate, vpriordate;
  if ( StrToDateTime( str_date.c_str(), "dd.mm.yyyy hh:nn:ss", vdate ) == EOF )
        throw UserException( "Invalid tag value '@time'" );
  bool pr_reset = ( GetNode( "@reset", reqNode ) != NULL );
  string prior_paxs;
  map<int,Tids> Paxs; // pax_id, <pax_tid, grp_tid> список пассажиров переданных ранее
  xmlDocPtr paxsDoc;
  if ( !pr_reset && AstraContext::GetContext( getMeridianContextName( airline ), 0, prior_paxs ) != NoExists ) {
    paxsDoc = TextToXMLTree( prior_paxs );
    try {
      xmlNodePtr nodePax = paxsDoc->children;
      node = GetNode( "@time", nodePax );
      if ( node == NULL )
        throw AstraLocale::UserException( "Tag '@time' not found in context" );
      if ( StrToDateTime( NodeAsString( node ), "dd.mm.yyyy hh:nn:ss", vpriordate ) == EOF )
            throw UserException( "Invalid tag value '@time' in context" );
      if ( vpriordate == vdate ) { // разбор дерева при условии, что предыдущий запрос не передал всех пассажиров за заданный момент времени
        nodePax = nodePax->children;
        for ( ; nodePax!=NULL && string((char*)nodePax->name) == "pax"; nodePax=nodePax->next ) {
          Paxs[ NodeAsInteger( "@pax_id", nodePax ) ] = Tids( NodeAsInteger( "@pax_tid", nodePax ), NodeAsInteger( "@grp_tid", nodePax ) );
          ProgTrace( TRACE5, "pax_id=%d, pax_tid=%d, grp_tid=%d",
                     NodeAsInteger( "@pax_id", nodePax ),
                     NodeAsInteger( "@pax_tid", nodePax ),
                     NodeAsInteger( "@grp_tid", nodePax ) );
        }
      }
    }
    catch( ... ) {
      xmlFreeDoc( paxsDoc );
      throw;
    }
    xmlFreeDoc( paxsDoc );
  }
  TQuery Qry(&OraSession);
  std::set<std::string> accessAirps;
  Qry.SQLText =
    "SELECT airp FROM meridian_airps_owner WHERE airline=:airline";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.Execute();
  bool pr_airps = !Qry.Eof;
  for ( ;!Qry.Eof; Qry.Next() ) {
    accessAirps.insert( Qry.FieldAsString( "airp" ) );
  }
  Qry.Clear();
  string sql_text =
    "SELECT pax_id,reg_no,work_mode,point_id,desk,client_type,time "
    " FROM aodb_pax_change "
    "WHERE time >= :time AND time <= :uptime ";
  if ( pr_airps ) {
      sql_text += " AND ( airline=:airline OR airp IN ";
      sql_text += GetSQLEnum( accessAirps ) + " ) ";
  }
  else {
    sql_text += " AND airline=:airline ";
  }
  sql_text += "ORDER BY time, pax_id, work_mode ";
  Qry.SQLText = sql_text;
  ProgTrace( TRACE5, "sql_text=%s", sql_text.c_str() );
  TDateTime nowUTC = NowUTC();
  if ( nowUTC - 1 > vdate )
    vdate = nowUTC - 1;
  Qry.CreateVariable( "time", otDate, vdate );
  Qry.CreateVariable( "uptime", otDate, nowUTC - 1.0/1440.0 );
  Qry.CreateVariable( "airline", otString, airline );
  Qry.Execute();
  TQuery PaxQry(&OraSession);
  PaxQry.SQLText =
       "SELECT pax.pax_id,pax.reg_no,pax.surname||RTRIM(' '||pax.name) name,"
       "       pax_grp.grp_id,"
       "       pax_grp.airp_arv,pax_grp.airp_dep,"
       "       pax_grp.class,pax.refuse,"
       "       pax.pers_type, "
       "       NVL(pax.is_female,1) as is_female, "
       "       pax.subclass, "
       "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'tlg',rownum) AS seat_no, "
       "       pax.seats seats, "
       "       ckin.get_excess(pax_grp.grp_id,pax.pax_id) excess,"
       "       ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkamount,"
       "       ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkweight,"
       "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagamount,"
       "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagweight,"
       "       ckin.get_bag_pool_pax_id(pax.grp_id,pax.bag_pool_num) AS bag_pool_pax_id, "
       "       pax.bag_pool_num, "
       "       pax.pr_brd, "
       "       pax_grp.status, "
       "       pax_grp.client_type, "
       "       pax_doc.no document, "
       "       pax.ticket_no, pax.tid pax_tid, pax_grp.tid grp_tid "
       " FROM pax_grp, pax, pax_doc "
       " WHERE pax_grp.grp_id=pax.grp_id AND "
       "       pax.pax_id=:pax_id AND "
       "       pax.wl_type IS NULL AND "
       "       pax.pax_id=pax_doc.pax_id(+) ";
  PaxQry.DeclareVariable( "pax_id", otInteger );
  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable( "pax_id", otInteger );
  TQuery FltQry(&OraSession);
  FltQry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  FltQry.DeclareVariable( "point_id", otInteger );
  TDateTime max_time = NoExists;
  node = NULL;
  string res;
  int pax_count = 0;
  int prior_pax_id = -1;
  Tids tids;
  map<int,TTripInfo> trips;
  map<int,bool> sync_meridian;
  int col_pax_tid = -1;
  int col_grp_tid = -1;
  int col_point_id = -1;
  int col_grp_id = -1;
  int col_name = -1;
  int col_class = -1;
  int col_subclass = -1;
  int col_pers_type = -1;
  int col_is_female =-1;
  int col_airp_dep = -1;
  int col_airp_arv = -1;
  int col_seat_no = -1;
  int col_seats = -1;
  int col_excess = -1;
  int col_rkamount = -1;
  int col_rkweight = -1;
  int col_bagamount = -1;
  int col_bagweight = -1;
  int col_pr_brd = -1;
  int col_client_type = -1;
  // пробег по всем пассажирам у которых время больше или равно текущему
  int count_row = 0;
  for ( ;!Qry.Eof && pax_count<=500; Qry.Next() ) { //по-хорошему меридиан никакого отношения к веб-регистрации не имеет
    count_row++;
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    if ( pax_id == prior_pax_id ) // удаляем дублирование строки с одним и тем же pax_id для регистрации и посадки
      continue; // предыдущий пассажир он же и текущий
    prior_pax_id = pax_id;
    int p_id = Qry.FieldAsInteger( "point_id" );
    if ( trips.find( Qry.FieldAsInteger( "point_id" ) ) == trips.end() ) {
      FltQry.SetVariable( "point_id", p_id );
      FltQry.Execute();
      if ( FltQry.Eof )
        throw EXCEPTIONS::Exception("WebRequestsIface::GetPaxsInfo: flight not found, (point_id=%d)", Qry.FieldAsInteger( "point_id" ) );
      TTripInfo tripInfo( FltQry );
      trips.insert( make_pair( p_id, tripInfo ) );
    }
    if ( sync_meridian.find( p_id ) == sync_meridian.end() ) {
      sync_meridian.insert( make_pair( p_id, is_sync_meridian( trips[ p_id ] ) ) );
    }
    if ( !sync_meridian[ p_id ] /*|| //но описывается в таблице web_clients как веб-регистрация!
         !checkAccess( airline, trips[ p_id ], accessAirps )*/ ) { // в запросе появилась авиакомпания и аэропорты принадлежащие авиакомпаниям
      continue;
    }

    if ( max_time != NoExists && max_time != Qry.FieldAsDateTime( "time" ) ) { // сравнение времени с пред. значением, если изменилось, то
      ProgTrace( TRACE5, "Paxs.clear(), vdate=%s, max_time=%s",
                 DateTimeToStr( vdate, ServerFormatDateTimeAsString ).c_str(),
                 DateTimeToStr( max_time, ServerFormatDateTimeAsString ).c_str() );
      Paxs.clear(); // изменилось время - удаляем всех предыдущих пассажиров с пред. временем
    }
    max_time = Qry.FieldAsDateTime( "time" );
    PaxQry.SetVariable( "pax_id", pax_id );
    PaxQry.Execute();
    if ( col_point_id < 0 ) {
      col_pax_tid = PaxQry.GetFieldIndex( "pax_tid" );
      col_grp_tid = PaxQry.GetFieldIndex( "grp_tid" );
      col_point_id = PaxQry.GetFieldIndex( "point_id" );
      col_grp_id = PaxQry.GetFieldIndex( "grp_id" );
      col_name = PaxQry.GetFieldIndex( "name" );
      col_class = PaxQry.GetFieldIndex( "class" );
      col_subclass = PaxQry.GetFieldIndex( "subclass" );
      col_pers_type = PaxQry.GetFieldIndex( "pers_type" );
      col_is_female = PaxQry.GetFieldIndex( "is_female" );
      col_airp_dep = PaxQry.GetFieldIndex( "airp_dep" );
      col_airp_arv = PaxQry.GetFieldIndex( "airp_arv" );
      col_seat_no = PaxQry.GetFieldIndex( "seat_no" );
      col_seats = PaxQry.GetFieldIndex( "seats" );
      col_excess = PaxQry.GetFieldIndex( "excess" );
      col_rkamount = PaxQry.GetFieldIndex( "rkamount" );
      col_rkweight = PaxQry.GetFieldIndex( "rkweight" );
      col_bagamount = PaxQry.GetFieldIndex( "bagamount" );
      col_bagweight = PaxQry.GetFieldIndex( "bagweight" );
      col_pr_brd = PaxQry.GetFieldIndex( "pr_brd" );
      col_client_type = PaxQry.GetFieldIndex( "client_type" );
    }
    if ( !PaxQry.Eof ) {
      tids.pax_tid = PaxQry.FieldAsInteger( col_pax_tid );
      tids.grp_tid = PaxQry.FieldAsInteger( col_grp_tid );
    }
    else {
      tids.pax_tid = -1; // пассажир был удален в пред. раз
      tids.grp_tid = -1;
    }
    // пассажир передавался и не изменился
    if ( Paxs.find( pax_id ) != Paxs.end() &&
         Paxs[ pax_id ].pax_tid == tids.pax_tid &&
         Paxs[ pax_id ].grp_tid == tids.grp_tid )
      continue; // уже передавали пассажира
    // пассажира не передавали или он изменился
    if ( node == NULL )
      node = NewTextChild( resNode, "passengers" );
    Paxs[ pax_id ] = tids; // изменения
    pax_count++;
    xmlNodePtr paxNode = NewTextChild( node, "pax" );
    NewTextChild( paxNode, "pax_id", pax_id );
    NewTextChild( paxNode, "point_id", p_id );
    if ( PaxQry.Eof ) {
      NewTextChild( paxNode, "status", "delete" );
      continue;
    }
    NewTextChild( paxNode, "flight", trips[ p_id ].airline + IntToString( trips[ p_id ].flt_no ) + trips[ p_id ].suffix );
    NewTextChild( paxNode, "scd_out", DateTimeToStr(trips[ p_id ].scd_out, ServerFormatDateTimeAsString ) );
    NewTextChild( paxNode, "grp_id", PaxQry.FieldAsString( col_grp_id ) );
    NewTextChild( paxNode, "name", PaxQry.FieldAsString( col_name ) );
    NewTextChild( paxNode, "class", PaxQry.FieldAsString( col_class ) );
    NewTextChild( paxNode, "subclass", PaxQry.FieldAsString( col_subclass ) );
    NewTextChild( paxNode, "pers_type", PaxQry.FieldAsString( col_pers_type ) );
    if ( DecodePerson( PaxQry.FieldAsString( col_pers_type ) ) == ASTRA::adult ) {
      NewTextChild( paxNode, "gender", (PaxQry.FieldAsInteger(col_is_female)==0?"M":"F") );
    }
    NewTextChild( paxNode, "airp_dep", PaxQry.FieldAsString( col_airp_dep ) );
    NewTextChild( paxNode, "airp_arv", PaxQry.FieldAsString( col_airp_arv ) );
    NewTextChild( paxNode, "seat_no", PaxQry.FieldAsString( col_seat_no ) );
    NewTextChild( paxNode, "seats", PaxQry.FieldAsInteger( col_seats ) );
    NewTextChild( paxNode, "excess", PaxQry.FieldAsInteger( col_excess ) );
    NewTextChild( paxNode, "rkamount", PaxQry.FieldAsInteger( col_rkamount ) );
    NewTextChild( paxNode, "rkweight", PaxQry.FieldAsInteger( col_rkweight ) );
    NewTextChild( paxNode, "bagamount", PaxQry.FieldAsInteger( col_bagamount ) );
    NewTextChild( paxNode, "bagweight", PaxQry.FieldAsInteger( col_bagweight ) );
    if ( PaxQry.FieldIsNULL( col_pr_brd ) )
      NewTextChild( paxNode, "status", "uncheckin" );
    else
      if ( PaxQry.FieldAsInteger( col_pr_brd ) == 0 )
        NewTextChild( paxNode, "status", "checkin" );
      else
        NewTextChild( paxNode, "status", "boarded" );
    NewTextChild( paxNode, "client_type", PaxQry.FieldAsString( col_client_type ) );
    res.clear();
    TCkinRoute ckinRoute;
    if ( ckinRoute.GetRouteAfter( PaxQry.FieldAsInteger( col_grp_id ), crtNotCurrent, crtIgnoreDependent ) ) { // есть сквозная регистрация
      xmlNodePtr rnode = NewTextChild( paxNode, "tckin_route" );
      int seg_no=1;
      for ( vector<TCkinRouteItem>::iterator i=ckinRoute.begin(); i!=ckinRoute.end(); i++ ) {
         xmlNodePtr inode = NewTextChild( rnode, "seg" );
         SetProp( inode, "num", seg_no );
         NewTextChild( inode, "flight", i->operFlt.airline + IntToString(i->operFlt.flt_no) + i->operFlt.suffix );
         NewTextChild( inode, "airp_dep", i->airp_dep );
         NewTextChild( inode, "airp_arv", i->airp_arv );
         NewTextChild( inode, "scd_out", DateTimeToStr( i->operFlt.scd_out, ServerFormatDateTimeAsString ) );
         seg_no++;
      }
    }
    RemQry.SetVariable( "pax_id", pax_id );
    RemQry.Execute();
    res.clear();
    for ( ;!RemQry.Eof; RemQry.Next() ) {
      if ( !res.empty() )
        res += "\n";
      res += RemQry.FieldAsString( "rem" );
    }
    if ( !res.empty() )
      NewTextChild( paxNode, "rems", res );
    NewTextChild( paxNode, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
  } //end for
  if ( max_time == NoExists )
    max_time = vdate; // не выбрано ни одного пассажира - передаем время, которые пришло с терминала
  SetProp( resNode, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
  AstraContext::ClearContext( getMeridianContextName( airline ), 0 );
  if ( !Paxs.empty() ) { // есть пассажиры - сохраняем всех переданных
    paxsDoc = CreateXMLDoc( "paxs" );
    try {
      node = paxsDoc->children;
      SetProp( node, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
      for ( map<int,Tids>::iterator p=Paxs.begin(); p!=Paxs.end(); p++ ) {
        xmlNodePtr n = NewTextChild( node, "pax" );
        SetProp( n, "pax_id", p->first );
        SetProp( n, "pax_tid", p->second.pax_tid );
        SetProp( n, "grp_tid", p->second.grp_tid );
      }
      AstraContext::SetContext( getMeridianContextName( airline ), 0, XMLTreeToText( paxsDoc ) );
      ProgTrace( TRACE5, "xmltreetotext=%s", XMLTreeToText( paxsDoc ).c_str() );
    }
    catch( ... ) {
      xmlFreeDoc( paxsDoc );
      throw;
    }
    xmlFreeDoc( paxsDoc );
  }
  ProgTrace( TRACE5, "count_row=%d", count_row);
}
////////////////////////////////////END MERIDIAN SYSTEM/////////////////////////////
} //namespace MERIDIAN
