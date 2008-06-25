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
#include "docs.h"
#include "stat.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;


void PrepRegInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripCounters" );
  TQuery Qry( &OraSession );

  Qry.SQLText =
     "SELECT -100 AS num, "
     "        '�ᥣ�' AS firstcol, "
     "        SUM(cfg) AS cfg, "
     "        SUM(resa) AS resa, "
     "        SUM(tranzit) AS tranzit, "
     "        SUM(block) AS block, "
     "        SUM(avail) AS avail, "
     "        SUM(prot) AS prot "
     "FROM trip_classes, "
     "     (SELECT point_dep AS point_id,class, "
     "             SUM(crs_ok) AS resa, "
     "             SUM(crs_tranzit) AS tranzit, "
     "             MAX(avail) AS avail "
     "      FROM counters2 "
     "      WHERE counters2.point_dep=:point_id "
     "      GROUP BY point_dep,class) c "
     "WHERE c.point_id=trip_classes.point_id AND "
     "      c.class=trip_classes.class "
     "UNION "
     "SELECT classes.priority-10, "
     "       c.class, "
     "       MAX(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       MAX(block), "
     "       MAX(avail), "
     "       MAX(prot) "
     "FROM counters2 c,trip_classes,classes "
     "WHERE c.point_dep=trip_classes.point_id AND "
     "      c.class=trip_classes.class AND "
     "      c.class=classes.code AND "
     "      c.point_dep=:point_id  "
     "GROUP BY classes.priority,c.class "
     "UNION "
     "SELECT points.point_num, "
     "       points.airp, "
     "       SUM(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       SUM(block), "
     "       SUM(avail), "
     "       SUM(prot)  "
     "FROM counters2 c,trip_classes,points "
     "WHERE c.point_dep=trip_classes.point_id AND "
     "      c.class=trip_classes.class AND "
     "      c.point_arv=points.point_id AND "
     "      c.point_dep=:point_id AND points.pr_del>=0 "
     "GROUP BY points.point_num,points.airp "
     "ORDER BY 1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  while ( !Qry.Eof ) {
    xmlNodePtr itemNode = NewTextChild( node, "item" );
    NewTextChild( itemNode, "firstcol", Qry.FieldAsString( "firstcol" ) );
    NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
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

  Qry.Clear();
  Qry.SQLText =
    "SELECT airline, flt_no, airp, "
    "       point_num,DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string airline = Qry.FieldAsString( "airline" );
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  string airp = Qry.FieldAsString( "airp" );
  int point_num = Qry.FieldAsInteger( "point_num" );
  int first_point = Qry.FieldAsInteger( "first_point" );

  Qry.Clear();
  Qry.SQLText =
    "SELECT airp FROM points "
    "WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "
    "ORDER BY point_num ";
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.Execute();
  xmlNodePtr node;
  node = NewTextChild( tripdataNode, "airps" );
  while ( !Qry.Eof ) {
    NewTextChild( node, "airp", Qry.FieldAsString( "airp" ) );
    Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT class FROM trip_classes,classes "\
    " WHERE trip_classes.class=classes.code AND point_id=:point_id "\
    "ORDER BY classes.priority ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  node = NewTextChild( tripdataNode, "classes" );
  while ( !Qry.Eof ) {
    NewTextChild( node, "class", Qry.FieldAsString( "class" ) );
    Qry.Next();
  }

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "\
    " SELECT MAX(ckin.get_crs_priority(code,:airline,:flt_no,:airp)) INTO :priority FROM crs2; "\
    "END;";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "airp", otString, airp );
  Qry.DeclareVariable( "priority", otInteger );
  Qry.Execute();
  bool empty_priority = Qry.VariableIsNULL( "priority" );
  int priority;
  if ( !empty_priority )
    priority = Qry.GetVariableAsInteger( "priority" );
  ProgTrace( TRACE5, "airline=%s, flt_no=%d, airp=%s, empty_priority=%d, priority=%d",
             airline.c_str(), flt_no, airp.c_str(), empty_priority, priority );
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
    "SELECT NULL AS code,'��騥 �����' AS name,0 AS sort, "\
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
  Qry.CreateVariable( "airp", otString, airp );
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
    "SELECT crs, "
    "       target, "
    "       class, "
    "       SUM(resa) AS resa, "
    "       SUM(tranzit) AS tranzit "
    " FROM crs_data,tlg_binding, "
    "      (SELECT MIN(point_num) AS point_num,airp FROM points "
    "       WHERE first_point=:first_point AND point_num>:point_num AND pr_del=0 "
    "       GROUP BY airp) p "
    "WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND "
    "      crs_data.target=p.airp(+) AND "
    "      crs_data.target<>:airp_dep AND "
    "      (resa IS NOT NULL OR tranzit IS NOT NULL) "
    "GROUP BY crs,p.point_num,target,class "
    "ORDER BY crs,p.point_num,target ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.CreateVariable( "airp_dep", otString, airp );
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
    Qry.CreateVariable( "airp", otString, airp );
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
  TQuery Qry( &OraSession );
  xmlNodePtr node = GetNode( "crsdata", reqNode );
  if ( node != NULL )
  {
    Qry.Clear();
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
    node = node->children;
    while ( node !=NULL ) {
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
      TReqInfo::Instance()->MsgToLog( string( "�������� ����� �� �த���." ) +
                                      " �����: , �/�: " + target +
                                      ", �����: " + cl + ", �த: " +
                                      IntToString( resa ) + ", ���: " + IntToString(tranzit),
                                      evtFlt, point_id );
      node = node->next;
    };
  };

  node = GetNode( "trip_sets", reqNode );
  if ( node != NULL )
  {
    //��稬 ३�
    Qry.Clear();
    Qry.SQLText =
      "SELECT point_num,pr_tranzit,first_point, "
      "       ckin.tranzitable(point_id) AS tranzitable "
      "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 FOR UPDATE ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("���� �� ������. ������� �����");

    TQuery SetsQry( &OraSession );
    SetsQry.Clear();
    SetsQry.SQLText =
      "SELECT pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam, "
      "       pr_check_pay,pr_exam_check_pay,pr_trfer_reg, "
      "       pr_reg_with_tkn,pr_reg_with_doc "
      "FROM trip_sets WHERE point_id=:point_id";
    SetsQry.CreateVariable("point_id",otInteger,point_id);
    SetsQry.Execute();
    if (SetsQry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    bool new_pr_tranzit,old_pr_tranzit,
         new_pr_tranz_reg,      old_pr_tranz_reg,
         new_pr_check_load,     old_pr_check_load,
         new_pr_overload_reg,   old_pr_overload_reg,
         new_pr_exam,           old_pr_exam,
         new_pr_check_pay,      old_pr_check_pay,
         new_pr_exam_check_pay, old_pr_exam_check_pay,
         new_pr_trfer_reg,      old_pr_trfer_reg,
         new_pr_reg_with_tkn,   old_pr_reg_with_tkn,
         new_pr_reg_with_doc,   old_pr_reg_with_doc;
    old_pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
    old_pr_tranz_reg=SetsQry.FieldAsInteger("pr_tranz_reg")!=0;
    old_pr_check_load=SetsQry.FieldAsInteger("pr_check_load")!=0;
    old_pr_overload_reg=SetsQry.FieldAsInteger("pr_overload_reg")!=0;
    old_pr_exam=SetsQry.FieldAsInteger("pr_exam")!=0;
    old_pr_check_pay=SetsQry.FieldAsInteger("pr_check_pay")!=0;
    old_pr_exam_check_pay=SetsQry.FieldAsInteger("pr_exam_check_pay")!=0;
    old_pr_trfer_reg=SetsQry.FieldAsInteger("pr_trfer_reg")!=0;
    old_pr_reg_with_tkn=SetsQry.FieldAsInteger("pr_reg_with_tkn")!=0;
    old_pr_reg_with_doc=SetsQry.FieldAsInteger("pr_reg_with_doc")!=0;

    new_pr_tranzit=NodeAsInteger("pr_tranzit",node)!=0;
    new_pr_tranz_reg=NodeAsInteger("pr_tranz_reg",node)!=0;
    new_pr_check_load=NodeAsInteger("pr_check_load",node)!=0;
    new_pr_overload_reg=NodeAsInteger("pr_overload_reg",node)!=0;
    new_pr_exam=NodeAsInteger("pr_exam",node)!=0;
    new_pr_check_pay=NodeAsInteger("pr_check_pay",node)!=0;
    //!!!��⮬ ���� GetNode 01.04.08
    if (GetNode("pr_exam_check_pay",node)!=NULL)
      new_pr_exam_check_pay=NodeAsInteger("pr_exam_check_pay",node)!=0;
    else
      new_pr_exam_check_pay=old_pr_exam_check_pay;
    if (GetNode("pr_reg_with_tkn",node)!=NULL)
      new_pr_reg_with_tkn=NodeAsInteger("pr_reg_with_tkn",node)!=0;
    else
      new_pr_reg_with_tkn=old_pr_reg_with_tkn;
    if (GetNode("pr_reg_with_doc",node)!=NULL)
      new_pr_reg_with_doc=NodeAsInteger("pr_reg_with_doc",node)!=0;
    else
      new_pr_reg_with_doc=old_pr_reg_with_doc;
    //!!!��⮬ ���� GetNode 01.04.08
    new_pr_trfer_reg=NodeAsInteger("pr_trfer_reg",node)!=0;

    if (old_pr_tranzit!=new_pr_tranzit ||
        old_pr_tranz_reg!=new_pr_tranz_reg)
    {
      if (Qry.FieldAsInteger("tranzitable")!=0)
      {
        //३� tranzitable
        bool pr_tranz_reg;
        if (new_pr_tranzit)
          pr_tranz_reg=new_pr_tranz_reg;
        else
          pr_tranz_reg=false;
        int first_point=Qry.FieldAsInteger("first_point");
        int point_num=Qry.FieldAsInteger("point_num");
        if (old_pr_tranzit != new_pr_tranzit)
        {
          Qry.Clear();
          if (new_pr_tranzit)
            Qry.SQLText =
              "BEGIN "
              "  UPDATE points SET pr_tranzit=:pr_tranzit WHERE point_id=:point_id AND pr_del>=0; "
              "  UPDATE points SET first_point=:first_point "
              "  WHERE first_point=:point_id AND point_num>:point_num AND pr_del>=0; "
              "END; ";
          else
            Qry.SQLText =
              "BEGIN "
              "  UPDATE points SET pr_tranzit=:pr_tranzit WHERE point_id=:point_id AND pr_del>=0; "
              "  UPDATE points SET first_point=:point_id "
              "  WHERE first_point=:first_point AND point_num>:point_num AND pr_del>=0; "
              "END; ";
          Qry.CreateVariable("pr_tranzit",otInteger,(int)new_pr_tranzit);
          Qry.CreateVariable("point_id",otInteger,point_id);
          Qry.CreateVariable("first_point",otInteger,first_point);
          Qry.CreateVariable("point_num",otInteger,point_num);
          Qry.Execute();
        };
        Qry.Clear();
        Qry.SQLText="UPDATE trip_sets SET pr_tranz_reg=:pr_tranz_reg WHERE point_id=:point_id";
        Qry.CreateVariable("pr_tranz_reg",otInteger,(int)pr_tranz_reg);
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();

        TLogMsg msg;
        msg.msg = "��⠭����� ०��";
        if ( !pr_tranz_reg ) msg.msg += " ���";
        msg.msg += " ���ॣ����樨 �࠭��� ���";
        if ( !new_pr_tranzit )
          msg.msg += " ���࠭��⭮�� ३�";
        else
          msg.msg += " �࠭��⭮�� ३�";
        msg.ev_type=evtFlt;
        msg.id1=point_id;
        TReqInfo::Instance()->MsgToLog(msg);
      };
    };
    if (old_pr_check_load!=new_pr_check_load ||
        old_pr_overload_reg!=new_pr_overload_reg ||
        old_pr_exam!=new_pr_exam ||
        old_pr_check_pay!=new_pr_check_pay ||
        old_pr_exam_check_pay!=new_pr_exam_check_pay ||
        old_pr_trfer_reg!=new_pr_trfer_reg ||
        old_pr_reg_with_tkn!=new_pr_reg_with_tkn ||
        old_pr_reg_with_doc!=new_pr_reg_with_doc)
    {
      Qry.Clear();
      Qry.SQLText=
        "UPDATE trip_sets "
        "SET pr_check_load=:pr_check_load, "
        "    pr_overload_reg=:pr_overload_reg, "
        "    pr_exam=:pr_exam, "
        "    pr_check_pay=:pr_check_pay, "
        "    pr_exam_check_pay=:pr_exam_check_pay, "
        "    pr_trfer_reg=:pr_trfer_reg, "
        "    pr_reg_with_tkn=:pr_reg_with_tkn, "
        "    pr_reg_with_doc=:pr_reg_with_doc "
        "WHERE point_id=:point_id";
      Qry.CreateVariable("pr_check_load",otInteger,(int)new_pr_check_load);
      Qry.CreateVariable("pr_overload_reg",otInteger,(int)new_pr_overload_reg);
      Qry.CreateVariable("pr_exam",otInteger,(int)new_pr_exam);
      Qry.CreateVariable("pr_check_pay",otInteger,(int)new_pr_check_pay);
      Qry.CreateVariable("pr_exam_check_pay",otInteger,(int)new_pr_exam_check_pay);
      Qry.CreateVariable("pr_trfer_reg",otInteger,(int)new_pr_trfer_reg);
      Qry.CreateVariable("pr_reg_with_tkn",otInteger,(int)new_pr_reg_with_tkn);
      Qry.CreateVariable("pr_reg_with_doc",otInteger,(int)new_pr_reg_with_doc);
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.Execute();

      TLogMsg msg;
      msg.ev_type=evtFlt;
      msg.id1=point_id;
      if (old_pr_check_load!=new_pr_check_load)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_check_load ) msg.msg += " ���";
        msg.msg += " ����஫� ����㧪� �� ॣ����樨";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_overload_reg!=new_pr_overload_reg)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_overload_reg ) msg.msg += " �����"; else msg.msg += " ࠧ�襭��";
        msg.msg += " ॣ����樨 �� �ॢ�襭�� ����㧪�";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_exam!=new_pr_exam)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_exam ) msg.msg += " ���";
        msg.msg += " ��ᬮ�஢��� ����஫� ��। ��ᠤ���";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_check_pay!=new_pr_check_pay)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_check_pay ) msg.msg += " ���";
        msg.msg += " ����஫� ������ ������ �� ��ᠤ��";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_exam_check_pay!=new_pr_exam_check_pay)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_exam_check_pay ) msg.msg += " ���";
        msg.msg += " ����஫� ������ ������ �� ��ᬮ��";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_trfer_reg!=new_pr_trfer_reg)
      {
        msg.msg = "��⠭����� ०��";
        if ( !new_pr_trfer_reg ) msg.msg += " �����"; else msg.msg += " ࠧ�襭��";
        msg.msg += " ��ࠡ�⪨ �࠭��� �� ॣ����樨";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_reg_with_tkn!=new_pr_reg_with_tkn)
      {
        msg.msg = "��⠭����� ०��";
        if ( new_pr_reg_with_tkn ) msg.msg += " �����"; else msg.msg += " ࠧ�襭��";
        msg.msg += " ॣ����樨 ��� ����஢ ����⮢";
        TReqInfo::Instance()->MsgToLog(msg);
      };
      if (old_pr_reg_with_doc!=new_pr_reg_with_doc)
      {
        msg.msg = "��⠭����� ०��";
        if ( new_pr_reg_with_doc ) msg.msg += " �����"; else msg.msg += " ࠧ�襭��";
        msg.msg += " ॣ����樨 ��� ����஢ ���㬥�⮢";
        TReqInfo::Instance()->MsgToLog(msg);
      };
    };
  };

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
  int pr_lat = 0;
  ProgTrace(TRACE5, "PrepRegInterface::ViewPNL, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  viewPNL( point_id, dataNode );
  get_report_form("PNLPaxList", resNode);
  STAT::set_variables(resNode);
  xmlNodePtr formDataNode = GetNode("form_data/variables", resNode);
  PaxListVars(point_id, pr_lat, formDataNode);
}

void PrepRegInterface::ViewCRSList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  int pr_lat = 0;
  ProgTrace(TRACE5, "PrepRegInterface::ViewPNL, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  viewCRSList( point_id, dataNode );
  get_report_form("PNLPaxList", resNode);
  STAT::set_variables(resNode);
  xmlNodePtr formDataNode = GetNode("form_data/variables", resNode);
  PaxListVars(point_id, pr_lat, formDataNode);
}


void PrepRegInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
