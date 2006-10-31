#include <stdlib.h>
#include "prepreg.h"
#include "basic.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include <map>
#include "stages.h"
#include "oralib.h"
#include "stl_utils.h"
#include "tripinfo.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;


void PrepRegInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripCounters" );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT -100 as num,'Всего' as firstcol, "\
    "       SUM(DECODE(point_num,1,cfg,0)) as cfg, "\
    "       SUM(crs_ok) as resa, "\
    "       SUM(crs_tranzit) as tranzit, "\
    "       SUM(DECODE(point_num,1,block,0)) as block, "\
    "       SUM(DECODE(point_num,1,avail,0)) as avail, "\
    "       SUM(DECODE(point_num,1,prot,0)) as prot "\
    " FROM counters2,trip_classes "\
    "WHERE counters2.point_dep=trip_classes.point_id AND "\
    "      counters2.class=trip_classes.class AND "
    "      counters2.point_dep=:point_id "
    "UNION "\
    "SELECT classes.lvl-10 as num,counters2.class as firstcol, "\
    "       SUM(DECODE(point_num,1,cfg,0)) as cfg, "\
    "       SUM(crs_ok) as resa, "\
    "       SUM(crs_tranzit) as tranzit, "\
    "       SUM(DECODE(point_num,1,block,0)) as block, "\
    "       SUM(DECODE(point_num,1,avail,0)) as avail, "\
    "       SUM(DECODE(point_num,1,prot,0)) as prot "\
    " FROM counters2,trip_classes,classes "\
    "WHERE counters2.point_dep=trip_classes.point_id AND "\
    "      counters2.class=trip_classes.class AND "\
    "      counters2.class=classes.id AND "\
    "      counters2.point_dep=:point_id "\
    " GROUP BY classes.lvl,counters2.class "\
    "UNION "\
    "SELECT point_num as num,airp as firstcol, "\
    "       SUM(cfg) as cfg, "\
    "       SUM(crs_ok) as resa, "\
    "       SUM(crs_tranzit) as tranzit, "\
    "       SUM(block) as block, "\
    "       SUM(avail) as avail, "\
    "       SUM(prot) as prot "\
    " FROM counters2,trip_classes "\
    "WHERE counters2.point_dep=trip_classes.point_id AND "\
    "      counters2.class=trip_classes.class AND "\
    "      counters2.point_dep=:point_id "\
    "GROUP BY point_num,airp "\
    "ORDER BY num "; /*!!!*/
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  while ( !Qry.Eof ) {
    xmlNodePtr itemNode = NewTextChild( node, "item" );
    NewTextChild( itemNode, "firstcol", Qry.FieldAsString( "firstcol" ) );
    NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
    NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
    NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
    NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
    NewTextChild( itemNode, "block", Qry.FieldAsInteger( "block" ) );
    NewTextChild( itemNode, "avail", Qry.FieldAsInteger( "avail" ) );
    NewTextChild( itemNode, "prot", Qry.FieldAsInteger( "prot" ) );
    Qry.Next();
  }
}

void PrepRegInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripData" );
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT cod,city FROM place "\
    " WHERE trip_id=:point_id AND num>0 "\
    " ORDER BY num ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  xmlNodePtr placesNode = NewTextChild( tripdataNode, "places" );
  while ( !Qry.Eof ) {
    itemNode = NewTextChild( placesNode, "place" );
    NewTextChild( itemNode, "cod", Qry.FieldAsString( "cod" ) );
    NewTextChild( itemNode, "city", Qry.FieldAsString( "city" ) );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT class FROM trip_classes,classes "\
    " WHERE trip_classes.class=classes.id AND point_id=:point_id "\
    "ORDER BY classes.lvl ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  xmlNodePtr node = NewTextChild( tripdataNode, "classes" );
  while ( !Qry.Eof ) {
    NewTextChild( node, "class", Qry.FieldAsString( "class" ) );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText = "SELECT airline, flt_no FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string airline = Qry.FieldAsString( "airline" );
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "\
    " SELECT MAX(ckin.get_crs_priority(code,:airline,:flt_no,:airp)) INTO :priority FROM crs2; "\
    "END;";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "airp", otString, TReqInfo::Instance()->opt.airport ); //??? пульт в аэропорту
  Qry.DeclareVariable( "priority", otInteger );
  Qry.Execute();
  bool empty_priority = Qry.VariableIsNULL( "priority" );
  int priority;
  if ( !empty_priority )
    priority = Qry.GetVariableAsInteger( "priority" );
  ProgTrace( TRACE5, "airline=%s, flt_no=%d, airp=%s, empty_priority=%d, priority=%d",
             airline.c_str(), flt_no, TReqInfo::Instance()->opt.airport.c_str(), empty_priority, priority );
  Qry.Clear();
  Qry.SQLText =
    "SELECT crs2.code,crs2.name,1 AS sort, "\
    "       DECODE(crs_data.crs,NULL,0,1) AS pr_charge, "\
    "       DECODE(crs_pnr.crs,NULL,0,1) AS pr_list, "\
    "       DECODE(NVL(:priority,0),0,0, "\
    "       DECODE(ckin.get_crs_priority(crs2.code,:airline,:flt_no,:airp),:priority,1,0)) AS pr_crs_main "\
    " FROM crs2, "\
    " (SELECT DISTINCT crs FROM crs_set "\
    "   WHERE airline=:airline AND "\
    "         (flt_no=:flt_no OR flt_no IS NULL) AND "\
    "         (airp_dep=:airp OR airp_dep IS NULL)) crs_set, "\
    " (SELECT DISTINCT crs FROM crs_data,tlg_binding "\
    "  WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id) crs_data, "\
    " (SELECT DISTINCT crs FROM crs_pnr,tlg_binding "\
    "  WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id) crs_pnr "\
    "WHERE crs2.code=crs_set.crs(+) AND "\
    "      crs2.code=crs_data.crs(+) AND "\
    "      crs2.code=crs_pnr.crs(+) AND "\
    "      (crs_set.crs IS NOT NULL OR crs_data.crs IS NOT NULL OR crs_pnr.crs IS NOT NULL) "\
    "UNION "\
    "SELECT NULL AS code,'Общие данные' AS name,0 AS sort, "\
    "       DECODE(trip_data.crs,0,0,1) AS pr_charge,0,0 "\
    "FROM dual, "\
    " (SELECT COUNT(*) AS crs FROM trip_data WHERE point_id=:point_id) trip_data "\
    "ORDER BY sort,name ";
  if ( empty_priority )
    Qry.CreateVariable( "priority", otInteger, FNull );
  else
    Qry.CreateVariable( "priority", otInteger, priority );
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "airp", otString, TReqInfo::Instance()->opt.airport ); //???
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  node = NewTextChild( tripdataNode, "crs" );
  while ( !Qry.Eof ) {
    itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "code", Qry.FieldAsString( "code" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "name" ) );
    NewTextChild( itemNode, "pr_charge", Qry.FieldAsInteger( "pr_charge" ) );
    NewTextChild( itemNode, "pr_list", Qry.FieldAsInteger( "pr_list" ) );
    NewTextChild( itemNode, "pr_crs_main", Qry.FieldAsInteger( "pr_crs_main" ) );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT crs, "\
    "       target, "\
    "       class, "\
    "       SUM(resa) AS resa, "\
    "       SUM(tranzit) AS tranzit "\
    " FROM crs_data,tlg_binding,place "\
    "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND "\
    "      crs_data.point_id=place.trip_id(+) AND crs_data.target=place.cod(+) AND "\
    "      (resa IS NOT NULL OR tranzit IS NOT NULL) "\
    "GROUP BY crs,place.num,target,class "\
    "ORDER BY crs,place.num,target ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  node = NewTextChild( tripdataNode, "crsdata" );
  while ( !Qry.Eof ) {
    itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "crs", Qry.FieldAsString( "crs" ) );
    NewTextChild( itemNode, "target", Qry.FieldAsString( "target" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    if ( Qry.FieldIsNULL( "resa" ) )
      NewTextChild( itemNode, "resa", -1 );
    else
      NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
    if ( Qry.FieldIsNULL( "tranzit" ) )
      NewTextChild( itemNode, "tranzit", -1 );
    else
      NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
    Qry.Next();
  }
  Qry.Clear();
  if ( empty_priority || !priority ) {
    tst();
    Qry.SQLText =
      "SELECT target,class, "\
      "       0 AS priority, "\
      "       NVL(SUM(resa),0) AS resa, "\
      "       NVL(SUM(tranzit),0) AS tranzit "\
      "FROM crs_data,tlg_binding "\
      "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id "\
      "GROUP BY target,class "\
      "UNION "\
      "SELECT target,class,1,resa,tranzit "\
      "FROM trip_data WHERE point_id=:point_id "\
      "ORDER BY target,class,priority DESC ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", point_id );
  }
  else {
    tst();
    Qry.SQLText =
      "SELECT target,class, "\
      "       0 AS priority, "\
      "       NVL(SUM(resa),0) AS resa, "\
      "       NVL(SUM(tranzit),0) AS tranzit "\
      "FROM crs_data,tlg_binding "\
      "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND "\
      "      crs IN (SELECT code FROM crs2 "\
      "              WHERE ckin.get_crs_priority(code,:airline,:flt_no,:airp)=:priority) "\
      "GROUP BY target,class "\
      "UNION "\
      "SELECT target,class,1,resa,tranzit "\
      "FROM trip_data WHERE point_id=:point_id "\
      "ORDER BY target,class,priority DESC ";
    Qry.CreateVariable( "priority", otInteger, priority );
    Qry.CreateVariable( "airline", otString, airline );
    Qry.CreateVariable( "flt_no", otInteger, flt_no );
    Qry.CreateVariable( "airp", otString, TReqInfo::Instance()->opt.airport ); //???
    Qry.CreateVariable( "point_id", otInteger, point_id );
  }
  Qry.Execute();
  string old_target;
  string old_class;

  while ( !Qry.Eof ) {
    if ( Qry.FieldAsString( "target" ) != old_target ||
         Qry.FieldAsString( "class" ) != old_class ) {
      itemNode = NewTextChild( node, "itemcrs" );
      NewTextChild( itemNode, "crs" );
      NewTextChild( itemNode, "target", Qry.FieldAsString( "target" ) );
      NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
      NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
      NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
      old_target = Qry.FieldAsString( "target" );
      old_class = Qry.FieldAsString( "class" );
    }
    Qry.Next();
  }
}

void PrepRegInterface::CrsDataApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace(TRACE5, "TripInfoInterface::CrsDataApplyUpdates, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amWrite );
  xmlNodePtr node = GetNode( "crsdata", reqNode );
  if ( !node || !node->children )
    return;
  node = node->children;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "BEGIN "\
    " UPDATE trip_data SET resa= :resa, tranzit= :tranzit "\
    "  WHERE point_id=:point_id AND target=:target AND class=:class; "\
    " IF SQL%NOTFOUND THEN "\
    "  INSERT INTO trip_data(point_id,target,class,resa,tranzit,avail) "\
    "   VALUES(:point_id,:target,:class,:resa,:tranzit,NULL); "\
    " END IF; "\
    "END; ";
  Qry.DeclareVariable( "resa", otInteger );
  Qry.DeclareVariable( "tranzit", otInteger );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "target", otString );
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  string target, cl;
  int resa, tranzit;
  while ( node ) {
    xmlNodePtr snode = node->children;
    target = NodeAsStringFast( "target", snode );
    cl = NodeAsStringFast( "class", snode );
    resa = NodeAsIntegerFast( "resa", snode );
    tranzit = NodeAsIntegerFast( "tranzit", snode );
    Qry.SetVariable( "target", target );
    Qry.SetVariable( "class", cl );
    Qry.SetVariable( "resa", resa );
    Qry.SetVariable( "tranzit", tranzit );
    Qry.Execute();
    TReqInfo::Instance()->MsgToLog( string( "Изменены данные по продаже." ) +
                                    " Центр: , п/н: " + target +
                                    ", класс: " + cl + ", прод: " +
                                    IntToString( resa ) + ", трзт: " + IntToString(tranzit),
                                    evtFlt, point_id );
    node = node->next;
  }
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "\
    " ckin.recount(:point_id); "\
    "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( GetNode( "tripcounters", reqNode ) ) {
    readTripCounters( point_id, dataNode );
  }
}

void PrepRegInterface::ViewPNL(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace(TRACE5, "PrepRegInterface::ViewPNL, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  viewPNL( point_id, dataNode );
}


void PrepRegInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
