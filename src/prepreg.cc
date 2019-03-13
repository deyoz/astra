#include <stdlib.h>
#include <map>
#include "prepreg.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stages.h"
#include "oralib.h"
#include "stl_utils.h"
#include "tripinfo.h"
#include "docs/docs_common.h"
#include "stat/stat_utils.h"
#include "salons.h"
#include "sopp.h"
#include "points.h"
#include "term_version.h"
#include "trip_tasks.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void PrepRegInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripCounters" );
  TQuery Qry( &OraSession );

  bool cfg_exists=!TCFG(point_id).empty();
  bool free_seating=false;
  if (!cfg_exists)
    free_seating=SALONS2::isFreeSeating(point_id);

  Qry.SQLText =
     "SELECT -100 AS num, "
     "        'Всего' AS firstcol, "
     "        SUM(cfg) AS cfg, "
     "        SUM(crs_ok) AS resa, "
     "        SUM(crs_tranzit) AS tranzit, "
     "        SUM(block) AS block, "
     "        SUM(avail) AS avail, "
     "        SUM(prot) AS prot "
     "FROM "
     " (SELECT MAX(cfg) AS cfg, "
     "         SUM(crs_ok) AS crs_ok, "
     "         SUM(crs_tranzit) AS crs_tranzit, "
     "         MAX(block) AS block, "
     "         DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(MAX(avail),0)) AS avail, "
     "         MAX(prot) AS prot "
     "  FROM counters2 c,trip_classes,classes "
     "  WHERE c.point_dep=trip_classes.point_id(+) AND "
     "        c.class=trip_classes.class(+) AND "
     "        (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "        c.class=classes.code AND "
     "        c.point_dep=:point_id  "
     "  GROUP BY classes.priority,c.class) "
     "UNION "
     "SELECT classes.priority-10, "
     "       c.class, "
     "       MAX(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       MAX(block), "
     "       DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(MAX(avail),0)), "
     "       MAX(prot) "
     "FROM counters2 c,trip_classes,classes "
     "WHERE c.point_dep=trip_classes.point_id(+) AND "
     "      c.class=trip_classes.class(+) AND "
     "      (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "      c.class=classes.code AND "
     "      c.point_dep=:point_id  "
     "GROUP BY classes.priority,c.class "
     "UNION "
     "SELECT points.point_num+100, "
     "       points.airp, "
     "       SUM(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       SUM(block), "
     "       DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(SUM(avail),0)), "
     "       SUM(prot)  "
     "FROM counters2 c,trip_classes,points "
     "WHERE c.point_dep=trip_classes.point_id(+) AND "
     "      c.class=trip_classes.class(+) AND "
     "      (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "      c.point_arv=points.point_id AND "
     "      c.point_dep=:point_id AND points.pr_del>=0 "
     "GROUP BY points.point_num,points.airp "
     "UNION "
     "SELECT 1, "
     "       'JMP', "
     "       jmp_cfg, "
     "       TO_NUMBER(NULL), "
     "       TO_NUMBER(NULL), "
     "       TO_NUMBER(NULL), "
     "       jmp_nooccupy, "
     "       TO_NUMBER(NULL) "
     "FROM counters2 c, trip_sets "
     "WHERE c.point_dep=trip_sets.point_id AND "
     "      c.point_dep=:point_id AND trip_sets.use_jmp<>0 AND "
     "      rownum<2 "
     "ORDER BY 1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "cfg_exists", otInteger, (int)cfg_exists );
  Qry.CreateVariable( "free_seating", otInteger, (int)free_seating );
  Qry.Execute();

  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  while ( !Qry.Eof )
  {
    int num=Qry.FieldAsInteger( "num" );
    string firstcol=Qry.FieldAsString( "firstcol" );
    xmlNodePtr itemNode = NewTextChild( node, "item" );
    if ( num>=-10 && num < 0 ) // классы
      NewTextChild( itemNode, "firstcol", ElemIdToCodeNative(etClass, firstcol) );
    else // аэропорты
      if (num >= 100)
        NewTextChild( itemNode, "firstcol", ElemIdToCodeNative(etAirp, firstcol) );
      else
        NewTextChild( itemNode, "firstcol", getLocaleText(firstcol) );

    if (!Qry.FieldIsNULL( "cfg" ))
      NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
    else
      NewTextChild( itemNode, "cfg" );
    if (!Qry.FieldIsNULL( "resa" ))
      NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
    else
      NewTextChild( itemNode, "resa" );
    if (!Qry.FieldIsNULL( "tranzit" ))
      NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
    else
      NewTextChild( itemNode, "tranzit" );
    if (!Qry.FieldIsNULL( "block" ))
      NewTextChild( itemNode, "block", Qry.FieldAsInteger( "block" ) );
    else
      NewTextChild( itemNode, "block" );
    if (!Qry.FieldIsNULL( "avail" ))
      NewTextChild( itemNode, "avail", Qry.FieldAsInteger( "avail" ) );
    else
      NewTextChild( itemNode, "avail" );
    if (!Qry.FieldIsNULL( "prot" ))
      NewTextChild( itemNode, "prot", Qry.FieldAsInteger( "prot" ) );
    else
      NewTextChild( itemNode, "prot" );
    Qry.Next();
  }
}

void PrepRegInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripData" );
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode;
  TQuery Qry( &OraSession );

  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, airp, scd_out, "
    "       point_id, point_num, first_point, pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TAdvTripInfo fltInfo(Qry);

  xmlNodePtr node;
  node = NewTextChild( tripdataNode, "airps" );
  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       fltInfo.point_id,
                       fltInfo.point_num,
                       fltInfo.first_point,
                       fltInfo.pr_tranzit,
                       trtNotCurrent,
                       trtNotCancelled );
  for(TTripRoute::const_iterator r=route.begin(); r!=route.end(); ++r)
    NewTextChild( node, "airp", r->airp );

  node = NewTextChild( tripdataNode, "classes" );
  TCFG cfg(point_id);
  for(TCFG::const_iterator c=cfg.begin(); c!=cfg.end(); ++c)
    NewTextChild( node, "class", c->cls );

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    " SELECT MAX(ckin.get_crs_priority(code,:airline,:flt_no,:airp)) INTO :priority FROM typeb_senders; "
    "END;";
  Qry.CreateVariable( "airline", otString, fltInfo.airline );
  Qry.CreateVariable( "flt_no", otInteger, fltInfo.flt_no );
  Qry.CreateVariable( "airp", otString, fltInfo.airp );
  Qry.DeclareVariable( "priority", otInteger );
  Qry.Execute();
  bool empty_priority = Qry.VariableIsNULL( "priority" );
  int priority = 0;
  if ( !empty_priority )
    priority = Qry.GetVariableAsInteger( "priority" );
  ProgTrace( TRACE5, "airline=%s, flt_no=%d, airp=%s, empty_priority=%d, priority=%d",
             fltInfo.airline.c_str(), fltInfo.flt_no, fltInfo.airp.c_str(), empty_priority, priority );
  Qry.Clear();
  Qry.SQLText =
    "SELECT typeb_senders.code, "
    "       name,1 AS sort, "
    "       DECODE(crs_data.crs,NULL,0,1) AS pr_charge, "
    "       DECODE(crs_pnr.crs,NULL,0,1) AS pr_list, "
    "       DECODE(NVL(:priority,0),0,0, "
    "       DECODE(ckin.get_crs_priority(typeb_senders.code,:airline,:flt_no,:airp),:priority,1,0)) AS pr_crs_main "
    " FROM typeb_senders, "
    " (SELECT DISTINCT crs FROM crs_set "
    "   WHERE airline=:airline AND "
    "         (flt_no=:flt_no OR flt_no IS NULL) AND "
    "         (airp_dep=:airp OR airp_dep IS NULL)) crs_set, "
    " (SELECT DISTINCT sender AS crs FROM crs_data,tlg_binding "
    "  WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
    "        point_id_spp=:point_id AND crs_data.system='CRS') crs_data, "
    " (SELECT DISTINCT sender AS crs FROM crs_pnr,tlg_binding "
    "  WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
    "        point_id_spp=:point_id AND crs_pnr.system='CRS') crs_pnr "
    "WHERE typeb_senders.code=crs_set.crs(+) AND "
    "      typeb_senders.code=crs_data.crs(+) AND "
    "      typeb_senders.code=crs_pnr.crs(+) AND "
    "      (crs_set.crs IS NOT NULL OR crs_data.crs IS NOT NULL OR crs_pnr.crs IS NOT NULL) "
    "UNION "
    "SELECT NULL AS code,'Общие данные' AS name,0 AS sort, "
    "       DECODE(trip_data.crs,0,0,1) AS pr_charge,0,0 "
    "FROM dual, "
    " (SELECT COUNT(*) AS crs FROM trip_data WHERE point_id=:point_id) trip_data "
    "ORDER BY sort,name ";
  if ( empty_priority )
    Qry.CreateVariable( "priority", otInteger, FNull );
  else
    Qry.CreateVariable( "priority", otInteger, priority );
  Qry.CreateVariable( "airline", otString, fltInfo.airline );
  Qry.CreateVariable( "flt_no", otInteger, fltInfo.flt_no );
  Qry.CreateVariable( "airp", otString, fltInfo.airp );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  node = NewTextChild( tripdataNode, "crs" );
  while ( !Qry.Eof ) {
    itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "code", Qry.FieldAsString( "code" ) );
    if ( Qry.FieldAsInteger( "sort" ) == 0 )
        NewTextChild( itemNode, "name", getLocaleText( Qry.FieldAsString( "name" ) ) );
    else
      NewTextChild( itemNode, "name", ElemIdToNameLong(etTypeBSender,Qry.FieldAsString("code")) );
    NewTextChild( itemNode, "pr_charge", Qry.FieldAsInteger( "pr_charge" ) );
    NewTextChild( itemNode, "pr_list", Qry.FieldAsInteger( "pr_list" ) );
    NewTextChild( itemNode, "pr_crs_main", Qry.FieldAsInteger( "pr_crs_main" ) );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT sender, "
    "       airp_arv, "
    "       class, "
    "       SUM(resa) AS resa, "
    "       SUM(tranzit) AS tranzit "
    " FROM crs_data,tlg_binding, "
    "      (SELECT MIN(point_num) AS point_num,airp FROM points "
    "       WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "
    "       GROUP BY airp) p "
    "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
    "      point_id_spp=:point_id AND crs_data.system='CRS' AND "
    "      crs_data.airp_arv=p.airp(+) AND "
    "      crs_data.airp_arv<>:airp_dep AND "
    "      (resa IS NOT NULL OR tranzit IS NOT NULL) "
    "GROUP BY sender,p.point_num,airp_arv,class "
    "ORDER BY sender,p.point_num,airp_arv ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "first_point", otInteger, fltInfo.pr_tranzit?fltInfo.first_point:fltInfo.point_id );
  Qry.CreateVariable( "point_num", otInteger, fltInfo.point_num );
  Qry.CreateVariable( "airp_dep", otString, fltInfo.airp );

  Qry.Execute();
  node = NewTextChild( tripdataNode, "crsdata" );
  while ( !Qry.Eof ) {
    itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "crs", Qry.FieldAsString( "sender" ) );
    NewTextChild( itemNode, "target", Qry.FieldAsString( "airp_arv" ) );
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
    Qry.SQLText =
      "SELECT airp_arv,class, "
      "       0 AS priority, "
      "       NVL(SUM(resa),0) AS resa, "
      "       NVL(SUM(tranzit),0) AS tranzit "
      "FROM crs_data,tlg_binding "
      "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
      "      point_id_spp=:point_id AND crs_data.system='CRS' "
      "GROUP BY airp_arv,class "
      "UNION "
      "SELECT airp_arv,class,1,resa,tranzit "
      "FROM trip_data WHERE point_id=:point_id "
      "ORDER BY airp_arv,class,priority DESC ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", point_id );
  }
  else {
    Qry.SQLText =
      "SELECT airp_arv,class, "
      "       0 AS priority, "
      "       NVL(SUM(resa),0) AS resa, "
      "       NVL(SUM(tranzit),0) AS tranzit "
      "FROM crs_data,tlg_binding "
      "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
      "      point_id_spp=:point_id AND crs_data.system='CRS' AND "
      "      sender IN (SELECT code FROM typeb_senders "
      "                 WHERE ckin.get_crs_priority(code,:airline,:flt_no,:airp)=:priority) "
      "GROUP BY airp_arv,class "
      "UNION "
      "SELECT airp_arv,class,1,resa,tranzit "
      "FROM trip_data WHERE point_id=:point_id "
      "ORDER BY airp_arv,class,priority DESC ";
    Qry.CreateVariable( "priority", otInteger, priority );
    Qry.CreateVariable( "airline", otString, fltInfo.airline );
    Qry.CreateVariable( "flt_no", otInteger, fltInfo.flt_no );
    Qry.CreateVariable( "airp", otString, fltInfo.airp );
    Qry.CreateVariable( "point_id", otInteger, point_id );
  }
  Qry.Execute();
  string old_airp_arv;
  string old_class;

  while ( !Qry.Eof ) {
    if ( Qry.FieldAsString( "airp_arv" ) != old_airp_arv ||
         Qry.FieldAsString( "class" ) != old_class ) {
      itemNode = NewTextChild( node, "itemcrs" );
      NewTextChild( itemNode, "crs" );
      NewTextChild( itemNode, "target", Qry.FieldAsString( "airp_arv" ) );
      NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
      NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
      NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
      old_airp_arv = Qry.FieldAsString( "airp_arv" );
      old_class = Qry.FieldAsString( "class" );
    }
    Qry.Next();
  }
}

void CheckJMPCount(int point_id, int jmp_cfg)
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText="SELECT SUM(jmp_tranzit)+SUM(jmp_ok)+SUM(jmp_goshow) AS jmp_show "
              "FROM counters2 "
              "WHERE point_dep=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  int jmp_show=Qry.FieldAsInteger("jmp_show");
  if (jmp_show>jmp_cfg)
  {
    if (jmp_cfg>0)
      throw UserException("MSG.NEED_TO_CANCEL_CKIN_FOR_PAX_ON_JUMP_SEATS");
    else
      throw UserException("MSG.NEED_TO_CANCEL_CKIN_FOR_ALL_PAX_ON_JUMP_SEATS");
  };
}

void PrepRegInterface::CrsDataApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  bool question = NodeAsInteger( "question", reqNode, 0 );
  ProgTrace(TRACE5, "TripInfoInterface::CrsDataApplyUpdates, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amWrite );

  //лочим рейс - весь маршрут, т.к. pr_tranzit может поменяться
  TFlights flights;
  flights.Get( point_id, ftAll );
  flights.Lock(__FUNCTION__);

  TQuery Qry( &OraSession );
  xmlNodePtr node = GetNode( "crsdata", reqNode );
  if ( node != NULL )
  {
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      " UPDATE trip_data SET resa= :resa, tranzit= :tranzit "
      "  WHERE point_id=:point_id AND airp_arv=:airp_arv AND class=:class; "
      " IF SQL%NOTFOUND THEN "
      "  INSERT INTO trip_data(point_id,airp_arv,class,resa,tranzit,avail) "
      "   VALUES(:point_id,:airp_arv,:class,:resa,:tranzit,NULL); "
      " END IF; "
      "END; ";
    Qry.DeclareVariable( "resa", otInteger );
    Qry.DeclareVariable( "tranzit", otInteger );
    Qry.DeclareVariable( "class", otString );
    Qry.DeclareVariable( "airp_arv", otString );
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", point_id );
    string airp_arv, cl;
    int resa, tranzit;
    node = node->children;
    while ( node !=NULL ) {
      xmlNodePtr snode = node->children;
      airp_arv = NodeAsStringFast( "target", snode );
      cl = NodeAsStringFast( "class", snode );
      resa = NodeAsIntegerFast( "resa", snode );
      if (resa<0) resa=0;
      tranzit = NodeAsIntegerFast( "tranzit", snode );
      if (tranzit<0) tranzit=0;
      Qry.SetVariable( "airp_arv", airp_arv );
      Qry.SetVariable( "class", cl );
      Qry.SetVariable( "resa", resa );
      Qry.SetVariable( "tranzit", tranzit );
      Qry.Execute();
      TReqInfo::Instance()->LocaleToLog("EVT.SALE_CHANGED", LEvntPrms() << PrmElem<std::string>("airp", etAirp, airp_arv)
                                        << PrmElem<std::string>("cl", etClass, cl) << PrmSmpl<int>("resa", resa)
                                        << PrmSmpl<int>("tranzit", tranzit), evtFlt, point_id );
      node = node->next;
    };
    SALONS2::AutoSetCraft( point_id );
  };

  node = GetNode( "trip_sets", reqNode );
  if ( node != NULL )
  {
    Qry.Clear();
    Qry.SQLText =
      "SELECT point_num,pr_tranzit,first_point, "
      "       ckin.tranzitable(point_id) AS tranzitable "
      "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";// FOR UPDATE ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

    TQuery SetsQry( &OraSession );
    SetsQry.Clear();
    SetsQry.SQLText =
      "SELECT pr_tranz_reg,pr_block_trzt "
      "FROM trip_sets WHERE point_id=:point_id";
    SetsQry.CreateVariable("point_id",otInteger,point_id);
    SetsQry.Execute();
    if (SetsQry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    bool new_pr_tranzit,          old_pr_tranzit,
         new_pr_tranz_reg,        old_pr_tranz_reg,
         new_pr_block_trzt,		  old_pr_block_trzt;

    old_pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
    old_pr_tranz_reg=SetsQry.FieldAsInteger("pr_tranz_reg")!=0;
    old_pr_block_trzt=SetsQry.FieldAsInteger("pr_block_trzt")!=0;
    new_pr_tranzit=NodeAsInteger("pr_tranzit",node)!=0;
    new_pr_tranz_reg=NodeAsInteger("pr_tranz_reg",node)!=0;
    new_pr_block_trzt=NodeAsInteger("pr_block_trzt",node,1)!=0;

    TTripSetList oldSetList, newSetList;
    oldSetList.fromDB(point_id);
    if (oldSetList.empty()) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    newSetList.fromXML(node);
    newSetList.append(oldSetList);

    vector<int> check_waitlist_alarms, check_diffcomp_alarms;
    bool pr_isTranzitSalons = false;
    if (old_pr_tranzit!=new_pr_tranzit ||
        old_pr_tranz_reg!=new_pr_tranz_reg ||
        old_pr_block_trzt!=new_pr_block_trzt ||
        oldSetList.value<bool>(tsFreeSeating)!=newSetList.value<bool>(tsFreeSeating)) {
      pr_isTranzitSalons = true;
    }

    if (old_pr_tranzit!=new_pr_tranzit ||
        old_pr_tranz_reg!=new_pr_tranz_reg ||
        old_pr_block_trzt!=new_pr_block_trzt)
    {
      if (Qry.FieldAsInteger("tranzitable")!=0) //является ли пункт промежуточным в маршруте
      {
        //рейс tranzitable
        bool pr_tranz_reg,pr_block_trzt;
        if (new_pr_tranzit)
          pr_tranz_reg=new_pr_tranz_reg;
        else
          pr_tranz_reg=false;
        if (new_pr_tranzit)
          pr_block_trzt=new_pr_block_trzt && !pr_tranz_reg;
        else
          pr_block_trzt=false;
        if ( pr_tranz_reg!=old_pr_tranz_reg && !pr_tranz_reg ) { // отмена перерегистрации транзита
            TQuery DelQry( &OraSession );
            DelQry.SQLText =
              "SELECT grp_id  FROM pax_grp,points "
              " WHERE points.point_id=:point_id AND "
              "       point_dep=:point_id AND pax_grp.status NOT IN ('E') AND "
            "       bag_refuse=0 AND status=:status AND rownum<2 ";
            DelQry.CreateVariable( "point_id", otInteger, point_id );
            DelQry.CreateVariable( "status", otString, EncodePaxStatus( psTransit ) );
            DelQry.Execute();
            if ( !DelQry.Eof ) {
                ProgTrace( TRACE5, "question=%d", question );
                if ( question ) {
                    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
                    NewTextChild( dataNode, "question", getLocaleText("QST.TRANZIT_RECHECKIN_CAUTION.CANCEL") );
                    return;
              }
              map<int,TAdvTripInfo> segs; // набор рейсов
            TDeletePaxFilter filter;
            filter.status=EncodePaxStatus( psTransit );
              DeletePassengers( point_id, filter, segs );
              DeletePassengersAnswer( segs, resNode );
            }
        }
        //есть ли транзитные пассажиры pax_grp.status='T'

        int first_point=Qry.FieldAsInteger("first_point");
        int point_num=Qry.FieldAsInteger("point_num");
        if (old_pr_tranzit != new_pr_tranzit)
        {
          set_pr_tranzit(point_id, point_num, first_point, new_pr_tranzit);
        };
        Qry.Clear();
        Qry.SQLText=
          "UPDATE trip_sets SET pr_tranz_reg=:pr_tranz_reg,pr_block_trzt=:pr_block_trzt"
          " WHERE point_id=:point_id";
        Qry.CreateVariable("pr_tranz_reg",otInteger,(int)pr_tranz_reg);
        Qry.CreateVariable("pr_block_trzt",otInteger,(int)pr_block_trzt);
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();

        if (old_pr_tranzit!=new_pr_tranzit ||
            old_pr_tranz_reg!=new_pr_tranz_reg ||
            old_pr_block_trzt!=new_pr_block_trzt) {
          TLogLocale tlocale;
          tlocale.lexema_id = "EVT.SET_MODE";
          if ( !pr_tranz_reg ) tlocale.prms << PrmLexema("trans_reg", "EVT.WITHOUT_TRANS_REG");
          tlocale.prms << PrmLexema("trans_reg", "EVT.TRANS_REG");
          if ( !pr_block_trzt ) tlocale.prms << PrmLexema("block_trans", "EVT.WITHOUT_BLOCK_TRANS");
          tlocale.prms << PrmLexema("block_trans", "EVT.WITHOUT_BLOCK_TRANS");
          if ( !new_pr_tranzit )
            tlocale.prms << PrmLexema("trans", "EVT.NON_TRANS_FLIGHT");
          else
            tlocale.prms << PrmLexema("trans", "EVT.TRANS_FLIGHT");
          tlocale.ev_type=evtFlt;
          tlocale.id1=point_id;
          TReqInfo::Instance()->LocaleToLog(tlocale);
        }

        check_diffcomp_alarms.push_back( point_id );
        if ( !pr_isTranzitSalons ) {
          check_waitlist_alarms.push_back( point_id );
        }
      };
      if ( pr_isTranzitSalons ) {
        check_waitlist_alarms.push_back( point_id );
      }
    };

    if (oldSetList.value<bool>(tsUseJmp)!=newSetList.value<bool>(tsUseJmp) ||
        oldSetList.value<int>(tsJmpCfg)!=newSetList.value<int>(tsJmpCfg))
    {
      //проверим что кол-во зарегистрированных JMP меньше или равно кол-ву мест JMP
      CheckJMPCount(point_id, newSetList.value<bool>(tsUseJmp)?newSetList.value<int>(tsJmpCfg):0);
    }

    TTripSetList differSetList;
    set_difference(newSetList.begin(), newSetList.end(),
                   oldSetList.begin(), oldSetList.end(),
                   inserter(differSetList, differSetList.end()), TTripSetListItemLess);
    differSetList.toDB(point_id);

    if (oldSetList.value<bool>(tsFreeSeating)!=newSetList.value<bool>(tsFreeSeating))
    {
      if ( newSetList.value<bool>(tsFreeSeating) ) {
        SALONS2::DeleteSalons( point_id );
      }
      check_diffcomp_alarms.push_back( point_id );
      check_waitlist_alarms.push_back( point_id );
    }

    for ( vector<int>::iterator ipoint_id=check_diffcomp_alarms.begin();
          ipoint_id!=check_diffcomp_alarms.end(); ipoint_id++ ) {
      SALONS2::check_diffcomp_alarm( *ipoint_id );
    }
    if ( !check_waitlist_alarms.empty() ) {
      if ( pr_isTranzitSalons ) {
        SALONS2::check_waitlist_alarm_on_tranzit_routes( check_waitlist_alarms, __FUNCTION__ );
      }
      else {
        for ( vector<int>::iterator ipoint_id=check_waitlist_alarms.begin();
              ipoint_id!=check_waitlist_alarms.end(); ipoint_id++ ) {
          check_waitlist_alarm( *ipoint_id );
        }
      }
    }
    if (oldSetList.value<bool>(tsAPISControl)!=newSetList.value<bool>(tsAPISControl))
    {
      check_apis_alarms(point_id);
    }
    if (oldSetList.value<bool>(tsAPISManualInput)!=newSetList.value<bool>(tsAPISManualInput))
    {
      set<Alarm::Enum> checked_alarms;
      checked_alarms.insert(Alarm::APISManualInput);
      check_apis_alarms(point_id, checked_alarms);
    }
  };

  on_change_trip( CALL_POINT, point_id, ChangeTrip::CrsDataApplyUpdates );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( GetNode( "tripcounters", reqNode ) ) {
    readTripCounters( point_id, dataNode );
  }
}

void PrepRegInterface::ViewCRSList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace(TRACE5, "PrepRegInterface::ViewPNL, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  viewCRSList( point_id, dataNode );
  get_compatible_report_form("PNLPaxList", reqNode, resNode);
  xmlNodePtr formDataNode = STAT::set_variables(resNode);
  TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
  PaxListVars(point_id, rpt_params, formDataNode);
  string real_out = NodeAsString("real_out", formDataNode);
  string scd_out = NodeAsString("scd_out", formDataNode);
  string date = real_out + (real_out == scd_out ? "" : "(" + scd_out + ")");
  NewTextChild(formDataNode, "caption", getLocaleText("CAP.DOC.PNL_PAX_LIST",
              LParams() << LParam("trip", NodeAsString("trip", formDataNode))
                  << LParam("date", date)
              ));
}


void PrepRegInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
