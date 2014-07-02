#include "basic.h"
#include "stat.h"
#include "docs.h"
#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "oralib.h"
#include "cache.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "stages.h"
#include "tripinfo.h"
#include "etick.h"
#include "astra_ticket.h"
#include "aodb.h"
#include "alarms.h"
#include "passenger.h"
#include "rozysk.h"
#include "transfer.h"
#include "points.h"
#include "salons.h"
#include "term_version.h"
#include "events.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

void BrdInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airp FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TripsInterface::readHalls(Qry.FieldAsString("airp"), "П", tripdataNode);
}

void BrdInterface::readTripCounters( const int point_id,
                                     const TRptParams &rpt_params,
                                     xmlNodePtr dataNode,
                                     const TRptType rpt_type,
                                     const string &client_type )
{
    if( not (
            rpt_type == rtEXAM or rpt_type == rtEXAMTXT or
            rpt_type == rtWEB or rpt_type == rtWEBTXT or
            rpt_type == rtNOREC or rpt_type == rtNORECTXT or
            rpt_type == rtGOSHO or rpt_type == rtGOSHOTXT or
            rpt_type == rtUnknown
            )
      )
        throw Exception("BrdInterface::readTripCounters: unexpected rpt_type %d", rpt_type);
    bool used_for_web_rpt = (rpt_type == rtWEB or rpt_type == rtWEBTXT);
    bool used_for_norec_rpt = (rpt_type == rtNOREC or rpt_type == rtNORECTXT);
    bool used_for_gosho_rpt = (rpt_type == rtGOSHO or rpt_type == rtGOSHOTXT);
    TReqInfo *reqInfo = TReqInfo::Instance();

    TQuery Qry(&OraSession);
    string sql=
        "SELECT "
        "    COUNT(*) AS reg, ";
    if (reqInfo->screen.name == "BRDBUS.EXE")
        sql+=   "    NVL(SUM(DECODE(pr_brd,0,0,1)),0) AS brd ";
    else
        sql+=   "    NVL(SUM(DECODE(pr_exam,0,0,1)),0) AS brd ";
    sql+=     "FROM "
        "    pax_grp, "
        "    pax ";
    if(used_for_norec_rpt or used_for_gosho_rpt)
        sql += ", crs_pax ";
    sql+=
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    point_dep=:point_id AND class=:class AND "
        "    pax_grp.status NOT IN ('E') AND "
        "    pr_brd IS NOT NULL ";
    if(used_for_norec_rpt or used_for_gosho_rpt) {
        sql +=
            " and pax.pax_id = crs_pax.pax_id(+) and "
            " (crs_pax.pax_id is null or crs_pax.pr_del <> 0) ";
        if(used_for_gosho_rpt) {
            sql += " and pax_grp.status not in(:psCheckin, :psTCheckin) ";
            Qry.CreateVariable("psCheckin", otString, EncodePaxStatus(psCheckin));
            Qry.CreateVariable("psTCheckin", otString, EncodePaxStatus(psTCheckin));
        }
    }
    if(used_for_web_rpt) {
        if(!client_type.empty()) {
            sql += " AND pax_grp.client_type = :client_type ";
            Qry.CreateVariable("client_type",otString,client_type);
        } else {
            sql += " AND pax_grp.client_type IN (:ctWeb, :ctKiosk) ";
            Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
            Qry.CreateVariable("ctKiosk", otString, EncodeClientType(ctKiosk));
        }
    }
    Qry.SQLText = sql;
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.DeclareVariable( "class", otString );

    ostringstream reg_str,brd_str, fr_reg_str, fr_brd_str, fr_not_brd_str;
    int reg=0,brd=0;
    TCFG cfg(point_id);
    bool cfg_exists=!cfg.empty();
    bool free_seating=false;
    if (!cfg_exists)
      free_seating=SALONS2::isFreeSeating(point_id);
    if (!cfg_exists && free_seating) cfg.get(NoExists);
    for(TCFG::const_iterator c=cfg.begin(); c!=cfg.end(); ++c)
    {
        Qry.SetVariable("class",c->cls);
        Qry.Execute();
        if(Qry.Eof) continue;

        int vreg = Qry.FieldAsInteger("reg");
        int vbrd = Qry.FieldAsInteger("brd");
        if (!cfg_exists && free_seating && vreg==0 && vbrd==0) continue;

        string class_client_view=ElemIdToCodeNative(etClass,c->cls);
        string class_report_view=rpt_params.ElemIdToReportElem(etClass,c->cls,efmtCodeNative);

        reg_str << class_client_view << vreg << " ";
        brd_str << class_client_view << vbrd << " ";
        reg+=vreg;
        brd+=vbrd;
        if(!fr_reg_str.str().empty()) fr_reg_str << "/";
        fr_reg_str << class_report_view << vreg;
        if(!fr_brd_str.str().empty()) fr_brd_str << "/";
        fr_brd_str << class_report_view << vbrd;
        if(!fr_not_brd_str.str().empty()) fr_not_brd_str << "/";
        fr_not_brd_str << class_report_view << vreg - vbrd;
    };

    xmlNodePtr countersNode = GetNode("counters", dataNode);
    if (countersNode==NULL)
        countersNode=NewTextChild(dataNode, "counters");
    ReplaceTextChild(countersNode, "reg", reg);
    ReplaceTextChild(countersNode, "brd", brd);
    ReplaceTextChild(countersNode, "reg_str", reg_str.str());
    ReplaceTextChild(countersNode, "brd_str", brd_str.str());

    xmlNodePtr variablesNode = GetNode("/term/answer/form_data/variables", dataNode->doc);
    if(variablesNode) {
        Qry.Clear();
        string SQLText =
            "select "
            " nvl(sum(decode(pax.pers_type, 'ВЗ', 1, 0)), 0) adl, "
            " nvl(sum(decode(pax.pers_type, 'РБ', 1, 0)), 0) chd, "
            " nvl(sum(decode(pax.pers_type, 'РМ', 1, 0)), 0) inf, ";
        if (reqInfo->screen.name == "BRDBUS.EXE")
            SQLText +=
                " NVL(SUM(DECODE(pr_brd,0,0,decode(pax.pers_type, 'ВЗ', 1, 0))),0) AS brd_adl, "
                " NVL(SUM(DECODE(pr_brd,0,0,decode(pax.pers_type, 'РБ', 1, 0))),0) AS brd_chd, "
                " NVL(SUM(DECODE(pr_brd,0,0,decode(pax.pers_type, 'РМ', 1, 0))),0) AS brd_inf ";
        else
            SQLText +=
                " NVL(SUM(DECODE(pr_exam,0,0,decode(pax.pers_type, 'ВЗ', 1, 0))),0) AS brd_adl, "
                " NVL(SUM(DECODE(pr_exam,0,0,decode(pax.pers_type, 'РБ', 1, 0))),0) AS brd_chd, "
                " NVL(SUM(DECODE(pr_exam,0,0,decode(pax.pers_type, 'РМ', 1, 0))),0) AS brd_inf ";
        SQLText +=
            "from "
            " pax_grp, "
            " pax ";
        if(used_for_norec_rpt or used_for_gosho_rpt)
            SQLText += ", crs_pax ";
        SQLText +=
            "where "
            " pax_grp.grp_id=pax.grp_id AND "
            " point_dep = :point_id and "
            " pax_grp.status NOT IN ('E') AND "
            " pr_brd is not null ";
        if(used_for_norec_rpt or used_for_gosho_rpt) {
            SQLText +=
                " and pax.pax_id = crs_pax.pax_id(+) and "
                " (crs_pax.pax_id is null or crs_pax.pr_del <> 0) ";
            if(used_for_gosho_rpt) {
                SQLText += " and pax_grp.status not in(:psCheckin, :psTCheckin) ";
                Qry.CreateVariable("psCheckin", otString, EncodePaxStatus(psCheckin));
                Qry.CreateVariable("psTCheckin", otString, EncodePaxStatus(psTCheckin));
            }
        }

        if(used_for_web_rpt) {
            if(!client_type.empty()) {
                SQLText += " AND pax_grp.client_type = :client_type ";
                Qry.CreateVariable("client_type",otString,client_type);
            } else {
                SQLText += " AND pax_grp.client_type IN (:ctWeb, :ctKiosk) ";
                Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
                Qry.CreateVariable("ctKiosk", otString, EncodeClientType(ctKiosk));
            }
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(!Qry.Eof) {
            ostringstream pax_str, brd_pax_str, not_brd_pax_str;
            int adl = Qry.FieldAsInteger("adl");
            int chd = Qry.FieldAsInteger("chd");
            int inf = Qry.FieldAsInteger("inf");
            int brd_adl = Qry.FieldAsInteger("brd_adl");
            int brd_chd = Qry.FieldAsInteger("brd_chd");
            int brd_inf = Qry.FieldAsInteger("brd_inf");
            pax_str
                << adl << "/"
                << chd << "/"
                << inf;
            brd_pax_str
                << brd_adl << "/"
                << brd_chd << "/"
                << brd_inf;
            not_brd_pax_str
                << adl - brd_adl << "/"
                << chd - brd_chd << "/"
                << inf - brd_inf;

            string total = pax_str.str() + "(" + fr_reg_str.str() + ")";
            string total_brd = brd_pax_str.str() + "(" + fr_brd_str.str() + ")";
            string total_not_brd = not_brd_pax_str.str() + "(" + fr_not_brd_str.str() + ")";
            ReplaceTextChild(variablesNode, "total", total);
            ReplaceTextChild(variablesNode, "total_brd", total_brd);
            ReplaceTextChild(variablesNode, "total_not_brd", total_not_brd);
            NewTextChild(variablesNode, "exam_totals", getLocaleText("CAP.DOC.EXAMBRD.EXAM_TOTALS",
                        LParams()
                        << LParam("total", total)
                        << LParam("total_brd", total_brd)
                        << LParam("total_not_brd", total_not_brd)
                        ));
            NewTextChild(variablesNode, "brd_totals", getLocaleText("CAP.DOC.EXAMBRD.BRD_TOTALS",
                        LParams()
                        << LParam("total", total)
                        << LParam("total_brd", total_brd)
                        << LParam("total_not_brd", total_not_brd)
                        ));
        }
    }
};

void get_new_tid(int locked_point_id, TQuery &Qry)
{
  if (locked_point_id!=NoExists)
  {
    TFlights flights;
    flights.Get( locked_point_id, ftTranzit );
    flights.Lock(); //лочим весь транзитный рейс
  };
  Qry.Clear();
  Qry.SQLText=
    "SELECT cycle_tid__seq.nextval FROM dual";
  Qry.Execute();
}

void BrdInterface::DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id=NodeAsInteger("point_id",reqNode);
  bool boarding = NodeAsInteger("boarding", reqNode)!=0;

  TQuery PaxQry(&OraSession);

  get_new_tid(point_id, PaxQry);
  
  ostringstream sql;
  PaxQry.Clear();
  sql.str("");
  sql << "SELECT pax_id,reg_no,pr_brd FROM pax_grp,pax "
         "WHERE pax_grp.grp_id=pax.grp_id AND "
         "      point_dep=:point_id AND "
         "      pax_grp.status NOT IN ('E') AND ";
  if (reqInfo->screen.name == "BRDBUS.EXE")
    sql << "pr_brd=:mark";
  else
    sql << "pr_exam=:mark";
  PaxQry.SQLText=sql.str().c_str();
  PaxQry.CreateVariable( "point_id", otInteger, point_id );
  PaxQry.CreateVariable( "mark", otInteger, (int)!boarding );
  PaxQry.Execute();
  if (!PaxQry.Eof)
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    sql.str("");
    if (reqInfo->screen.name == "BRDBUS.EXE")
      sql << "UPDATE pax SET pr_brd=DECODE(:mark,0,1,0),tid=cycle_tid__seq.currval "
             "WHERE pax_id=:pax_id AND pr_brd=:mark ";
    else
      sql << "UPDATE pax SET pr_exam=DECODE(:mark,0,1,0),tid=cycle_tid__seq.currval "
             "WHERE pax_id=:pax_id AND pr_exam=:mark ";
    Qry.SQLText=sql.str().c_str();
    Qry.DeclareVariable( "pax_id", otInteger );
    Qry.CreateVariable( "mark", otInteger, (int)!boarding );
    for(;!PaxQry.Eof;PaxQry.Next())
    {
      int pax_id=PaxQry.FieldAsInteger("pax_id");
      Qry.SetVariable("pax_id", pax_id);
      Qry.Execute();
      if (Qry.RowsProcessed()>0)
        rozysk::sync_pax(pax_id, reqInfo->desk.code, reqInfo->user.descr);
      if (reqInfo->screen.name == "BRDBUS.EXE")
      {
        bool boarded=!PaxQry.FieldIsNULL("pr_brd") && PaxQry.FieldAsInteger("pr_brd")!=0;
        if (boarded)
          update_pax_change( point_id, pax_id, PaxQry.FieldAsInteger("reg_no"), "П" );
      };
    };
  };

  string lexeme_id;
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    if (boarding)
      lexeme_id="EVT.PASSENGER.ALL_BOARDED";
    else
      lexeme_id="EVT.PASSENGER.ALL_NOT_BOARDED";
  }
  else
  {
    if (boarding)
      lexeme_id="EVT.PASSENGER.ALL_EXAMED";
    else
      lexeme_id="EVT.PASSENGER.ALL_NOT_EXAMED";
  };
  reqInfo->LocaleToLog(lexeme_id, evtPax, point_id);

  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    check_brd_alarm( point_id );
    check_u_trfer_alarm_for_next_trfer(point_id, idFlt);
  };

  GetPax(reqNode,resNode);
  xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
  if(variablesNode) SetProp(variablesNode, "update");
  NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
  if(bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");

  showMessage(lexeme_id);
};

bool PaxUpdate(int point_id, int pax_id, int tid, bool mark, bool pr_exam_with_brd)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    Qry.SQLText=
      "UPDATE pax "
      "SET pr_brd=:mark, "
      "    pr_exam=DECODE(:pr_exam_with_brd,0,pr_exam,:mark), "
      "    tid=cycle_tid__seq.currval "
      "WHERE pax_id=:pax_id AND tid=:tid";
    Qry.CreateVariable("pr_exam_with_brd",otInteger,(int)pr_exam_with_brd);
  }
  else
    Qry.SQLText=
      "UPDATE pax SET pr_exam=:mark, tid=cycle_tid__seq.currval "
      "WHERE pax_id=:pax_id AND tid=:tid";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.CreateVariable("mark", otInteger, (int)mark);
  Qry.Execute();
  if (Qry.RowsProcessed()<=0) return false;

  rozysk::sync_pax(pax_id, reqInfo->desk.code, reqInfo->user.descr);

  Qry.Clear();
  Qry.SQLText=
    "SELECT surname, name, reg_no, grp_id, cycle_tid__seq.currval AS tid "
    "FROM pax "
    "WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    int grp_id=Qry.FieldAsInteger("grp_id");

    string lexema_id;
    if (reqInfo->screen.name == "BRDBUS.EXE")
    {
      if (pr_exam_with_brd)
        lexema_id = (mark ? "EVT.PASSENGER.EXAMED_WITH_BRD" : "EVT.PASSENGER.NOT_EXAMED_WITH_BRD");
      else
        lexema_id = (mark ? "EVT.PASSENGER.BOARDED" : "EVT.PASSENGER.NOT_BOARDED");
      if (is_sync_paxs(point_id))
        update_pax_change( point_id, pax_id, Qry.FieldAsInteger("reg_no"), "П");
    }
    else
      lexema_id = (mark ? "EVT.PASSENGER.EXAMED" : "EVT.PASSENGER.NOT_EXAMED");

    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << PrmSmpl<string>("surname", Qry.FieldAsString("surname"))
                                      << PrmSmpl<string>("name", Qry.FieldAsString("name")),
                                      evtPax, point_id, Qry.FieldAsInteger("reg_no"), grp_id);

    //отвяжем сквозняков от предыдущих сегментов
    SeparateTCkin(grp_id,cssAllPrevCurr,cssCurr,Qry.FieldAsInteger("tid"));

    if (reqInfo->screen.name == "BRDBUS.EXE")
    {
      check_brd_alarm( point_id );
      check_u_trfer_alarm_for_next_trfer(grp_id, idGrp);
    };
  };
  return true;
}

bool CheckSeat(int pax_id, string& curr_seat_no)
{
    curr_seat_no.clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'list',1,0) AS curr_seat_no, "
      "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'list',1,1) AS curr_seat_no_lat, "
      "       bp_print.seat_no_lat AS bp_seat_no_lat "
      "FROM bp_print,pax, "
      "     (SELECT MAX(time_print) AS time_print FROM bp_print WHERE pax_id=:pax_id AND pr_print<>0) a "
      "WHERE bp_print.time_print=a.time_print AND bp_print.pax_id=:pax_id AND "
      "      pax.pax_id=:pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      curr_seat_no=Qry.FieldAsString("curr_seat_no");
      
      if (!Qry.FieldIsNULL("bp_seat_no_lat") &&
          strcmp(Qry.FieldAsString("curr_seat_no_lat"), Qry.FieldAsString("bp_seat_no_lat"))!=0)
        return false;
    };
    return true;
}

void BrdInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetPax(reqNode,resNode);
  xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
  if(variablesNode) SetProp(variablesNode, "update");
  NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
  if(bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");
};

void BrdInterface::GetPaxQuery(TQuery &Qry, const int point_id,
                                            const int reg_no,
                                            const int pax_id,
                                            const string &lang,
                                            const TRptType rpt_type,
                                            const string &client_type,
                                            const TSortType sort )
{
    if( not (
            rpt_type == rtEXAM or rpt_type == rtEXAMTXT or
            rpt_type == rtWEB or rpt_type == rtWEBTXT or
            rpt_type == rtNOREC or rpt_type == rtNORECTXT or
            rpt_type == rtGOSHO or rpt_type == rtGOSHOTXT or
            rpt_type == rtUnknown
            )
      )
        throw Exception("BrdInterface::GetPaxQuery: unexpected rpt_type %d", rpt_type);
    bool used_for_web_rpt = (rpt_type == rtWEB or rpt_type == rtWEBTXT);
    bool used_for_norec_rpt = (rpt_type == rtNOREC or rpt_type == rtNORECTXT);
    bool used_for_gosho_rpt = (rpt_type == rtGOSHO or rpt_type == rtGOSHOTXT);
    Qry.Clear();
    ostringstream sql;
    sql << "SELECT "
        "    pax.pax_id, "
        "    pax_grp.grp_id, "
        "    pr_brd, "
        "    pr_exam, "
        "    reg_no, "
        "    pax.surname, "
        "    pax.name, "
        "    pax.pers_type, "
        "    class, "
        "    pax_grp.status, "
        "    pax_grp.airp_arv, "
        "    NVL(report.get_last_trfer_airp(pax_grp.grp_id),pax_grp.airp_arv) AS last_airp_arv, "
        "    salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'_seats',rownum) AS seat_no, "
        "    pax.seats, "
        "    wl_type, "
        "    ticket_no, "
        "    coupon_no, "
        "    pax.tid, "
        "    NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
        "    NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
        "    NVL(ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_amount, "
        "    NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight, "
        "    DECODE(pax_grp.bag_refuse,0,pax_grp.excess,0) AS excess, "
        "    ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
        "    client_type ";
    if(rpt_type == rtUnknown)
        sql << ", tckin_id, seg_no ";

    if (used_for_web_rpt)
        sql << ", users2.descr AS user_descr ";

    sql << "FROM "
        "    pax_grp, "
        "    pax ";
    if(used_for_norec_rpt or used_for_gosho_rpt)
        sql << ", crs_pax ";
    if(rpt_type == rtUnknown)
        sql << ", tckin_pax_grp ";

    if (used_for_web_rpt)
        sql << ", users2 ";

    sql << "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND ";
    if(used_for_norec_rpt or used_for_gosho_rpt) {
        sql
            << " pax.pax_id = crs_pax.pax_id(+) and "
            << " (crs_pax.pax_id is null or "
            << " crs_pax.pr_del <> 0) and ";
        if(used_for_gosho_rpt) {
            sql << " pax_grp.status not in(:psCheckin, :psTCheckin) and ";
            Qry.CreateVariable("psCheckin", otString, EncodePaxStatus(psCheckin));
            Qry.CreateVariable("psTCheckin", otString, EncodePaxStatus(psTCheckin));
        }
    }
    if(rpt_type == rtUnknown)
        sql << "    pax_grp.grp_id=tckin_pax_grp.grp_id(+) AND ";

    if (used_for_web_rpt)
        sql << "  pax_grp.user_id=users2.user_id(+) AND "; //pax_grp.user_id=NULL для client_type=ctPNL

    sql << "    point_dep= :point_id AND pax_grp.status NOT IN ('E') AND pr_brd IS NOT NULL ";

    if (used_for_web_rpt) {
        if(!client_type.empty()) {
            sql << " AND pax_grp.client_type = :client_type ";
            Qry.CreateVariable("client_type",otString,client_type);
        } else {
            sql << " AND pax_grp.client_type IN (:ctWeb, :ctKiosk) ";
            Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
            Qry.CreateVariable("ctKiosk", otString, EncodeClientType(ctKiosk));
        }
    };
    if (reg_no!=NoExists)
    {
        sql << " AND reg_no=:reg_no ";
        Qry.CreateVariable("reg_no",otInteger,reg_no);
    };
    if (pax_id!=NoExists)
    {
        sql << " AND pax.pax_id=:pax_id ";
        Qry.CreateVariable("pax_id",otInteger,pax_id);
    };
    switch(sort) {
        case stRegNo:
            sql << " ORDER BY pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            sql << " ORDER BY pax.surname, pax.name, pax.reg_no, pax.seats DESC ";
            break;
        case stSeatNo:
            sql << " ORDER BY seat_no, pax.reg_no, pax.seats DESC ";
            break;
    }

    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("lang",otString,lang);
    Qry.SQLText = sql.str().c_str();
};

void put_exambrd_vars(xmlNodePtr variablesNode)
{
    NewTextChild(variablesNode, "cap_checked", getLocaleText("CAP.DOC.EXAMBRD.CHECKED.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_exam", getLocaleText("CAP.DOC.EXAMBRD.EXAM.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_brd", getLocaleText("CAP.DOC.EXAMBRD.BRD.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_no_exam", getLocaleText("CAP.DOC.EXAMBRD.NO_EXAM.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_no_brd", getLocaleText("CAP.DOC.EXAMBRD.NO_BRD.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
}

void LoadAPIS(const TCheckDocInfo &check_info, const CheckIn::TAPISItem &apis, xmlNodePtr resNode)
{
  if (resNode==NULL) return;

  xmlNodePtr infoNode=NewTextChild(resNode, "check_info");
  check_info.toXML(infoNode);
  apis.toXML(resNode);
};

void LoadAPIS(int point_id, const string &airp_arv, int pax_id, xmlNodePtr resNode)
{
  if (resNode==NULL) return;

  TCompleteCheckDocInfo checkDocInfo=GetCheckDocInfo(point_id, airp_arv);
  LoadAPIS(checkDocInfo.pass, CheckIn::TAPISItem().fromDB(pax_id), resNode);
};

void GetAPISAlarms(int pax_id, bool isCBBG, const TCheckDocInfo &check_info, const CheckIn::TAPISItem &apis, set<APIS::TAlarmType> &alarms)
{
  alarms.clear();
  if (isCBBG) return;

  CheckIn::TPaxDocaItem docaB, docaR, docaD;
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=apis.doca.begin(); d!=apis.doca.end(); ++d)
  {
    if (d->type=="B") docaB=*d;
    if (d->type=="R") docaR=*d;
    if (d->type=="D") docaD=*d;
  };
  //грузим данные из бронирования
  CheckIn::TAPISItem crs_apis;
  LoadCrsPaxDoc(pax_id, crs_apis.doc);
  if ((crs_apis.doc.getEqualAttrsFieldsMask(apis.doc) & crs_apis.doc.getNotEmptyFieldsMask()) != crs_apis.doc.getNotEmptyFieldsMask())
    alarms.insert(APIS::atDiffersFromBooking);
  else
  {
    LoadCrsPaxVisa(pax_id, crs_apis.doco);
    if ((crs_apis.doco.getEqualAttrsFieldsMask(apis.doco) & crs_apis.doco.getNotEmptyFieldsMask()) != crs_apis.doco.getNotEmptyFieldsMask())
      alarms.insert(APIS::atDiffersFromBooking);
    else
    {
      LoadCrsPaxDoca(pax_id, crs_apis.doca);
      CheckIn::TPaxDocaItem crs_docaB, crs_docaR, crs_docaD;
      for(list<CheckIn::TPaxDocaItem>::const_iterator d=crs_apis.doca.begin(); d!=crs_apis.doca.end(); ++d)
      {
        if (d->type=="B") crs_docaB=*d;
        if (d->type=="R") crs_docaR=*d;
        if (d->type=="D") crs_docaD=*d;
      };
      if ((crs_docaB.getEqualAttrsFieldsMask(docaB) & crs_docaB.getNotEmptyFieldsMask()) != crs_docaB.getNotEmptyFieldsMask() ||
          (crs_docaR.getEqualAttrsFieldsMask(docaR) & crs_docaR.getNotEmptyFieldsMask()) != crs_docaR.getNotEmptyFieldsMask() ||
          (crs_docaD.getEqualAttrsFieldsMask(docaD) & crs_docaD.getNotEmptyFieldsMask()) != crs_docaD.getNotEmptyFieldsMask())
        alarms.insert(APIS::atDiffersFromBooking);
    };
  };

  if ((apis.doc.getNotEmptyFieldsMask() & check_info.doc.required_fields)!=check_info.doc.required_fields)
    alarms.insert(APIS::atIncomplete);
  else
  {
    if (apis.doco.getNotEmptyFieldsMask()!=NO_FIELDS && //пришла непустая информация о визе
        (apis.doco.getNotEmptyFieldsMask() & check_info.doco.required_fields)!=check_info.doco.required_fields)
      alarms.insert(APIS::atIncomplete);
    else
    {
      if ((docaB.getNotEmptyFieldsMask() & check_info.docaB.required_fields)!=check_info.docaB.required_fields ||
          (docaR.getNotEmptyFieldsMask() & check_info.docaR.required_fields)!=check_info.docaR.required_fields ||
          (docaD.getNotEmptyFieldsMask() & check_info.docaD.required_fields)!=check_info.docaD.required_fields)
        alarms.insert(APIS::atIncomplete);
    };
  };

  long int required_not_empty_fields=check_info.doc.required_fields & apis.doc.getNotEmptyFieldsMask();
  if ((apis.doc.scanned_attrs & required_not_empty_fields) != required_not_empty_fields)
    alarms.insert(APIS::atManualInput);

};

typedef map<string/*airp_arv*/, pair<TCompleteCheckDocInfo, set<string> > > TAPISMap;

bool GetAPISAlarms(const int pax_id,
                   const bool isCBBG,
                   const string &airp_arv,
                   const bool apis_control,
                   const bool apis_manual_input,
                   const TAPISMap &apis_map,
                   TCheckDocInfo &check_info,
                   CheckIn::TAPISItem &apis,
                   set<APIS::TAlarmType> &alarms,
                   bool &document_alarm)
{
  check_info.clear();
  apis.clear();
  alarms.clear();
  document_alarm=false;

  TAPISMap::const_iterator iAPISMap=apis_map.find(airp_arv);
  if (apis_control &&
      iAPISMap!=apis_map.end() &&
      !iAPISMap->second.second.empty())
  {
    //по данному направлению формируется APIS и разрешен вывод тревог
    check_info=iAPISMap->second.first.pass;
    apis.fromDB(pax_id);
    GetAPISAlarms(pax_id, isCBBG, check_info, apis, alarms);
    if (alarms.find(APIS::atIncomplete)!=alarms.end() ||
        (!apis_manual_input && alarms.find(APIS::atManualInput)!=alarms.end()))
      document_alarm=true;
    return true;
  };
  return false;
};


void SaveAPIS(int point_id, int pax_id, int tid, xmlNodePtr reqNode)
{
  if (reqNode==NULL) return;
  //берем новый tid
  TQuery Qry(&OraSession);
  get_new_tid(NoExists, Qry);
  bool apis_control=GetAPISControl(point_id);

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.grp_id, pax_grp.status, "
    "       pax.reg_no, pax.surname, pax.name, pax.pers_type "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax.pax_id=:pax_id AND pax.tid=:tid FOR UPDATE";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  int grp_id=Qry.FieldAsInteger("grp_id");
  string status=Qry.FieldAsString("status");
  int reg_no=Qry.FieldAsInteger("reg_no");
  string surname=Qry.FieldAsString("surname");
  string name=Qry.FieldAsString("name");
  string pers_type=Qry.FieldAsString("pers_type");

  CheckIn::TAPISItem prior_apis;
  prior_apis.fromDB(pax_id);

  Qry.Clear();
  Qry.SQLText=
    "UPDATE pax "
    "SET tid=cycle_tid__seq.currval "
    "WHERE pax_id=:pax_id AND tid=:tid";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.Execute();
  if (Qry.RowsProcessed()<=0)
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  CheckIn::TAPISItem apis=prior_apis;
  xmlNodePtr node2=reqNode->children;
  xmlNodePtr docNode;
  docNode=GetNodeFast("document",node2);
  if (docNode!=NULL)
    SavePaxDoc(pax_id, apis.doc.fromXML(docNode), Qry);
  docNode=GetNodeFast("doco",node2);
  if (docNode!=NULL)
    SavePaxDoco(pax_id, apis.doco.fromXML(docNode), Qry);
  xmlNodePtr docaNode=GetNodeFast("addresses",node2);
  if (docaNode!=NULL)
  {
    apis.doca.clear();
    for(docaNode=docaNode->children; docaNode!=NULL; docaNode=docaNode->next)
    {
      CheckIn::TPaxDocaItem docaItem;
      docaItem.fromXML(docaNode);
      if (docaItem.empty()) continue;
      apis.doca.push_back(docaItem);
    };
    SavePaxDoca(pax_id, apis.doca, Qry);
  };

  if (apis_control)
  {
    list<pair<string, string> > msgs;
    GetAPISLogMsgs(prior_apis, apis, msgs);
    for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
    {
      LEvntPrms params;
      logPaxName(status, surname, name, pers_type, params);
      TReqInfo::Instance()->LocaleToLog((*m).first, params << PrmSmpl<string>("params", (*m).second),
                                     ASTRA::evtPax, point_id, reg_no, grp_id);
    };
  }
  else
  {
    if (!(apis.doc.equalAttrs(prior_apis.doc) &&
          apis.doco.equalAttrs(prior_apis.doco) &&
          apis.doca==prior_apis.doca))
    {
      string lexema_id;
      if (status!=EncodePaxStatus(psCrew)) lexema_id = "EVT.CHANGED_PASSENGER_DATA";
      else lexema_id = "EVT.CHANGED_CREW_MEMBER_DATA";
      LEvntPrms params;
      logPaxName(status, surname, name, pers_type, params);
      TReqInfo::Instance()->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, reg_no, grp_id);
    };
  };
};

const int DOCUMENT_ALARM_ERRCODE=128;

void BrdInterface::GetPax(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    enum TSearchType {updateByPaxId,
                      updateByRegNo,
                      updateByScanData,
                      refreshByPaxId,
                      refreshWholeFlight};

    TSearchType search_type=refreshWholeFlight;
    if( strcmp((char *)reqNode->name, "PaxByPaxId") == 0) search_type=updateByPaxId;
    else
      if( strcmp((char *)reqNode->name, "PaxByRegNo") == 0) search_type=updateByRegNo;
      else
        if( strcmp((char *)reqNode->name, "PaxByScanData") == 0) search_type=updateByScanData;
        else
          if( strcmp((char *)reqNode->name, "SavePaxAPIS") == 0) search_type=refreshByPaxId;

    TReqInfo *reqInfo = TReqInfo::Instance();

    enum TScreenType {sSecurity, sBoarding};
    TScreenType screen=(reqInfo->screen.name == "BRDBUS.EXE"?sBoarding:sSecurity);

    int point_id=NodeAsInteger("point_id",reqNode);
    int found_point_id=NoExists;
    int reg_no=NoExists;
    int found_pax_id=NoExists;

    get_new_report_form("ExamBrdbus", reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    if ( GetNode( "LoadVars", reqNode ) ) {
        TRptParams rpt_params(reqInfo->desk.lang);
        PaxListVars(point_id, rpt_params, variablesNode);
        put_exambrd_vars(variablesNode);
        STAT::set_variables(resNode);
        NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT"));
        NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
    }

    xmlNodePtr dataNode=GetNode("data",resNode);
    if (dataNode==NULL)
      dataNode = NewTextChild(resNode, "data");

    TQuery FltQry(&OraSession);
    FltQry.Clear();
    FltQry.SQLText=
      "SELECT airline,flt_no,suffix,airp,scd_out, "
      "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
      "FROM points WHERE point_id=:point_id AND pr_del>=0";
    FltQry.DeclareVariable("point_id",otInteger);

    bool showWholeFlight=false;

    TQuery Qry(&OraSession);
    if(search_type==updateByScanData ||
       search_type==updateByPaxId ||
       search_type==refreshByPaxId)
    {
      if(search_type==updateByPaxId ||
         search_type==refreshByPaxId)
      {
        found_pax_id=NodeAsInteger("pax_id",reqNode);
        //получим point_id и reg_no
        Qry.Clear();
        Qry.SQLText=
          "SELECT point_dep,reg_no "
          "FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id AND pr_brd IS NOT NULL";
        Qry.CreateVariable("pax_id",otInteger,found_pax_id);
        Qry.Execute();
        if (Qry.Eof)
        {
          if (search_type==updateByPaxId)
            AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
          if (search_type==refreshByPaxId)
            AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        };
        found_point_id=Qry.FieldAsInteger("point_dep");
        reg_no=Qry.FieldAsInteger("reg_no");
      }
      else
      {
        SearchPaxByScanData(reqNode, found_point_id, reg_no, found_pax_id);
        if (found_point_id==NoExists || reg_no==NoExists || found_pax_id==NoExists)
          throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
      };

      if (point_id!=found_point_id)
      {
        if (search_type==refreshByPaxId)
          AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

        point_id=found_point_id;

        if (screen==sSecurity &&
            TripsInterface::readTripHeader( point_id, dataNode ))
        {
          TRptParams rpt_params(reqInfo->desk.lang);
          PaxListVars(point_id, rpt_params, variablesNode);
          put_exambrd_vars(variablesNode);
          STAT::set_variables(resNode);
          NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT"));
          NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
          readTripCounters(point_id, rpt_params, dataNode, rtUnknown, "");
          readTripData( point_id, dataNode );
        }
        else
        {
          FltQry.SetVariable("point_id",point_id);
          FltQry.Execute();
          if (!FltQry.Eof)
          {
            TTripInfo info(FltQry);
            throw AstraLocale::UserException(100, "MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
          }
          else
            throw AstraLocale::UserException(100, "MSG.PASSENGER.OTHER_FLIGHT");
        };

        showWholeFlight=true;
      };
    };

    if(search_type==updateByRegNo)
    {
      reg_no=NodeAsInteger("reg_no",reqNode);
    };

    if(search_type==refreshWholeFlight)
    {
      //общий список
      showWholeFlight=true;
    };

    //проверяем настройки APIS по каждому направлению
    TAPISMap apis_map;

    TTripRoute route;
    route.GetRouteAfter(NoExists,point_id,trtNotCurrent,trtNotCancelled);
    for(TTripRoute::iterator r = route.begin(); r != route.end(); ++r)
    {
      set<string> apis_formats;
      TCompleteCheckDocInfo check_info=GetCheckDocInfo(point_id, r->airp, apis_formats);
      apis_map.insert( make_pair(r->airp, make_pair(check_info, apis_formats)));
    };

    //признак контроля данных APIS
    Qry.Clear();
    Qry.SQLText=
        "SELECT apis_control, apis_manual_input "
        "FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    bool apis_control=Qry.FieldAsInteger("apis_control")!=0;
    bool apis_manual_input=Qry.FieldAsInteger("apis_manual_input")!=0;

    if(search_type==updateByPaxId ||
       search_type==updateByRegNo ||
       search_type==updateByScanData)
    {
      bool set_mark = NodeAsInteger("boarding", reqNode)!=0;
      int tid=(GetNode("tid",reqNode)==NULL?NoExists:NodeAsInteger("tid",reqNode));

      TFlights flights;
      flights.Get( point_id, ftTranzit );
      flights.Lock(); //лочим весь транзитный рейс

      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id, pax_grp.airp_arv, pax_grp.status, "
        "       pax.pax_id, pax.surname, pax.name, pax.seats, "
        "       pax.pr_brd, pax.pr_exam, pax.wl_type, pax.ticket_rem, pax.tid "
        "FROM pax_grp, pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      point_dep= :point_id AND pax_grp.status NOT IN ('E') AND pr_brd IS NOT NULL AND "
        "      reg_no=:reg_no";
      Qry.CreateVariable("point_id", otInteger, point_id);
      Qry.CreateVariable("reg_no", otInteger, reg_no);
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
      int grp_id=Qry.FieldAsInteger("grp_id");
      string airp_arv=Qry.FieldAsString("airp_arv");
      TPaxStatus grp_status=DecodePaxStatus(Qry.FieldAsString("status"));

      class TPaxItem
      {
        public:
          int pax_id;
          string surname, name;
          string wl_type;
          string ticket_rem;
          bool pr_brd, pr_exam, already_marked;
          int tid;
          bool updated;
          TPaxItem() {clear();};
          void clear()
          {
            pax_id=NoExists;
            surname.clear();
            name.clear();
            wl_type.clear();
            ticket_rem.clear();
            pr_brd=false;
            pr_exam=false;
            already_marked=false;
            tid=NoExists;
            updated=false;
          };
          bool exists() const
          {
            return pax_id!=NoExists;
          };
      };

      TPaxItem paxWithSeat, paxWithoutSeat;
      string surname;
      string name;
      for(;!Qry.Eof;Qry.Next())
      {
        if (grp_id!=Qry.FieldAsInteger("grp_id"))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        int seats=Qry.FieldAsInteger("seats");
        TPaxItem &pax=seats>0?paxWithSeat:paxWithoutSeat;
        if (pax.exists())
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        pax.pax_id=Qry.FieldAsInteger("pax_id");
        pax.surname=Qry.FieldAsString("surname");
        pax.name=Qry.FieldAsString("name");
        pax.wl_type=Qry.FieldAsString("wl_type");
        pax.ticket_rem=Qry.FieldAsString("ticket_rem");
        pax.pr_brd=Qry.FieldAsInteger("pr_brd")!=0;
        pax.pr_exam=Qry.FieldAsInteger("pr_exam")!=0;
        pax.already_marked=(screen==sBoarding?pax.pr_brd:pax.pr_exam);
        pax.tid=Qry.FieldAsInteger("tid");
      };

      if (!paxWithSeat.exists() && !paxWithoutSeat.exists())
        throw EXCEPTIONS::Exception("!paxWithSeat.exists() && !paxWithoutSeat.exists()"); //не обязательно, на всякий случай

      try  //catch(int)
      {
        //============================ проверка совпадения tid ============================
        if (tid!=NoExists)
        {
          //проверим чтобы хотя бы один из двух tid совпадал
          if ((!paxWithSeat.exists() || tid!=paxWithSeat.tid) &&
              (!paxWithoutSeat.exists() || tid!=paxWithoutSeat.tid))
          {
            TPaxItem &pax=paxWithSeat.exists()?paxWithSeat:paxWithoutSeat;
            throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                             AstraLocale::LParams() << AstraLocale::LParam("surname", pax.surname+(pax.name.empty()?"":" ")+pax.name));
          };
        };

        //============================ проверка признака досмотра/посадки ============================
        if ((!paxWithSeat.exists() || set_mark==paxWithSeat.already_marked) &&
            (!paxWithoutSeat.exists() || set_mark==paxWithoutSeat.already_marked))
        {
          if (screen==sBoarding)
          {
              if (set_mark)
                  AstraLocale::showErrorMessage("MSG.PASSENGER.BOARDED_ALREADY",120);
              else
                  AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BOARDING",120);
          }
          else
          {
              if (set_mark)
                  AstraLocale::showErrorMessage("MSG.PASSENGER.EXAMED_ALREADY",120);
              else
                  AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM",120);
          };
          throw 1;
        };

        //============================ зал посадки ============================
        int hall=NoExists;
        if (GetNode("hall",reqNode)!=NULL && !NodeIsNULL("hall",reqNode))
        {
          //проверка принадлежности зала а/п вылета рейса
          hall=NodeAsInteger("hall",reqNode);
          TQuery HallQry(&OraSession);
          HallQry.Clear();
          HallQry.SQLText=
            "SELECT halls2.airp AS hall_airp, points.airp AS flt_airp "
            "FROM halls2,points "
            "WHERE points.point_id=:point_id AND pr_del=0 AND pr_reg<>0 AND "
            "      halls2.id(+)=:hall AND "
            "      points.airp=halls2.airp(+)";
          HallQry.CreateVariable("point_id",otInteger,point_id);
          HallQry.CreateVariable("hall",otInteger,hall);
          HallQry.Execute();
          if (HallQry.Eof ||
              strcmp(HallQry.FieldAsString("hall_airp"),
                     HallQry.FieldAsString("flt_airp"))!=0) hall=NoExists;
        };

        if (hall==NoExists)
        {
            if(screen==sBoarding)
                AstraLocale::showErrorMessage("MSG.NOT_SET_BOARDING_HALL");
            else
                AstraLocale::showErrorMessage("MSG.NOT_SET_EXAM_HALL");
            throw 1;
        };

        //============================ настройки рейса ============================
        bool pr_exam_with_brd=false;
        bool pr_exam=false;
        bool pr_check_pay=false;
        int pr_etstatus=0;
        bool free_seating=false;
        if (screen==sBoarding)
        {
            Qry.Clear();
            Qry.SQLText=
                "SELECT pr_misc FROM trip_hall "
                "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
                "ORDER BY DECODE(hall,NULL,1,0)";
            Qry.CreateVariable("point_id",otInteger,point_id);
            Qry.CreateVariable("hall",otInteger,hall);
            Qry.CreateVariable("type",otInteger,2);
            Qry.Execute();
            if (!Qry.Eof) pr_exam_with_brd=Qry.FieldAsInteger("pr_misc")!=0;
        };

        Qry.Clear();
        Qry.SQLText=
            "SELECT pr_exam,pr_check_pay,pr_exam_check_pay,pr_etstatus,pr_free_seating "
            "FROM trip_sets WHERE point_id=:point_id";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();
        if (Qry.Eof)
          throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

        pr_exam=Qry.FieldAsInteger("pr_exam")!=0;
        if (screen==sBoarding)
          pr_check_pay=Qry.FieldAsInteger("pr_check_pay")!=0;
        else
          pr_check_pay=Qry.FieldAsInteger("pr_exam_check_pay")!=0;
        pr_etstatus=Qry.FieldAsInteger("pr_etstatus");
        free_seating=Qry.FieldAsInteger("pr_free_seating")!=0;

        //============================ проверка листа ожидания ============================
        if (set_mark && !free_seating &&
            ((paxWithSeat.exists() && !paxWithSeat.wl_type.empty()) ||
             (paxWithoutSeat.exists() && !paxWithoutSeat.wl_type.empty())))   //!!!vlad младенцы без мест и ЛО?
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_CONFIRM_FROM_WAIT_LIST");
            throw 1;
        };

        //============================ проверка прохождения досмотра ============================
        if (screen==sBoarding && set_mark && !pr_exam_with_brd && pr_exam &&
            ((paxWithSeat.exists() && !paxWithSeat.pr_exam) ||
             (paxWithoutSeat.exists() && !paxWithoutSeat.pr_exam)))
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM");
            throw 1;
        };

        //============================ проверка оплаты багажа ============================
        if (set_mark && pr_check_pay && !BagPaymentCompleted(grp_id))
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BAG_PAID");
            throw 1;
        };

        bool without_monitor=false;
        Qry.Clear();
        Qry.SQLText=
            "SELECT without_monitor FROM stations "
            "WHERE desk=:desk AND work_mode=:work_mode";
        Qry.CreateVariable("desk", otString, reqInfo->desk.code);
        Qry.CreateVariable("work_mode", otString, "П");
        Qry.Execute();
        if (!Qry.Eof) without_monitor=Qry.FieldAsInteger("without_monitor")!=0;

        //============================ проверим APIS alarms ============================
        if (screen==sBoarding && set_mark)
        {
          for(int pass=0;pass<2;pass++)
          {
            TPaxItem &pax=(pass==0?paxWithSeat:paxWithoutSeat);
            if (!pax.exists()) continue;

            TCheckDocInfo check_info;
            CheckIn::TAPISItem apis;
            set<APIS::TAlarmType> alarms;
            bool document_alarm=false;
            GetAPISAlarms(pax.pax_id, pax.name=="CBBG", airp_arv, apis_control, apis_manual_input, apis_map,
                          check_info, apis, alarms, document_alarm);
            if (document_alarm &&
                reqInfo->desk.compatible(APIS_CONTROL_VERSION))
            {
              if (alarms.find(APIS::atIncomplete)!=alarms.end())
              {
                AstraLocale::showErrorMessage("MSG.PASSENGER.APIS_INCOMPLETE", DOCUMENT_ALARM_ERRCODE);
                throw 1;
              };
              if (alarms.find(APIS::atManualInput)!=alarms.end())
              {
                AstraLocale::showErrorMessage("MSG.PASSENGER.APIS_MANUAL_INPUT", DOCUMENT_ALARM_ERRCODE);
                throw 1;
              };
            };
          };
        };

        if (!without_monitor)
        {
          //============================ вопрос в терминал ============================
          bool reset=false;
          for(int pass=1;pass<=3;pass++)
          {
            switch(pass)
            {
              case 1: if (screen==sBoarding && set_mark &&
                          GetNode("confirmations/ssr",reqNode)==NULL)
                      {
                        TRemGrp rem_grp;
                        rem_grp.Load(retBRD_WARN, point_id);
                        if ((!paxWithSeat.exists() || GetRemarkStr(rem_grp, paxWithSeat.pax_id).empty()) &&
                            (!paxWithoutSeat.exists() || GetRemarkStr(rem_grp, paxWithoutSeat.pax_id).empty())) break;

                        xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                        NewTextChild(confirmNode,"reset",(int)reset);
                        NewTextChild(confirmNode,"type","ssr");
                        ostringstream msg;
                        msg << getLocaleText("MSG.PASSENGER.NEEDS_SPECIAL_SERVICE") << endl
                            << getLocaleText("QST.CONTINUE_BRD");
                        NewTextChild(confirmNode,"message",msg.str());
                        throw 1;
                      };
                      break;
              case 2: if (screen==sBoarding && set_mark &&
                          GetNode("confirmations/seat_no",reqNode)==NULL)
                      {
                        if (free_seating) break;
                        string curr_seat_no;
                        if (paxWithSeat.exists() && CheckSeat(paxWithSeat.pax_id, curr_seat_no)) break;
                        if (paxWithoutSeat.exists() && CheckSeat(paxWithoutSeat.pax_id, curr_seat_no)) break;

                        xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                        NewTextChild(confirmNode,"reset",(int)reset);
                        NewTextChild(confirmNode,"type","seat_no");
                        ostringstream msg;
                        if (curr_seat_no.empty())
                            msg << AstraLocale::getLocaleText("MSG.PASSENGER.SALON_SEAT_NO_NOT_DEFINED") << endl;
                        else
                            msg << AstraLocale::getLocaleText("MSG.PASSENGER.SALON_SEAT_NO_CHANGED_TO", AstraLocale::LParams() << LParam("seat_no", curr_seat_no)) << endl;

                        msg << getLocaleText("MSG.PASSENGER.INVALID_BP_SEAT_NO") << endl
                            << getLocaleText("QST.CONTINUE_BRD");
                        NewTextChild(confirmNode,"message",msg.str());
                        throw 1;
                      };
                      break;
              case 3: if (screen==sSecurity && !set_mark &&
                          GetNode("confirmations/pr_brd",reqNode)==NULL &&
                          ((paxWithSeat.exists() && paxWithSeat.pr_brd) ||
                           (paxWithoutSeat.exists() && paxWithoutSeat.pr_brd)))
                      {
                        xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                        NewTextChild(confirmNode,"reset",(int)reset);
                        NewTextChild(confirmNode,"type","pr_brd");
                        ostringstream msg;
                        msg << getLocaleText("MSG.PASSENGER.BOARDED_ALREADY") << endl
                            << getLocaleText("QST.PASSENGER.RETURN_FOR_EXAM");
                        NewTextChild(confirmNode,"message",msg.str());
                        throw 1;
                      };
                      break;
            };
          }; //for(int pass=1;pass<=3;pass++)
        };

        //============================ собственно посадка пассажира(ов) ============================
        get_new_tid(NoExists, Qry);  //не лочим рейс, так как лочим ранее
        for(int pass=0;pass<2;pass++)
        {
          TPaxItem &pax=(pass==0?paxWithSeat:paxWithoutSeat);
          if (!pax.exists()) continue;
          if (set_mark==pax.already_marked) continue;

          if (!PaxUpdate(point_id,pax.pax_id,pax.tid,set_mark,pr_exam_with_brd))
            throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                             LParams() << LParam("surname", pax.surname+(pax.name.empty()?"":" ")+pax.name));
          pax.updated=true;
        };

        if (reqInfo->desk.compatible(MULTI_BOARDING_VERSION))
        {
          xmlNodePtr node=NewTextChild(dataNode,"updated");
          if (paxWithSeat.exists() && paxWithSeat.updated)
            NewTextChild(node, "pax_id", paxWithSeat.pax_id);
          if (paxWithoutSeat.exists() && paxWithoutSeat.updated)
            NewTextChild(node, "pax_id", paxWithoutSeat.pax_id);
        }
        else
        {
          if (paxWithSeat.exists() && paxWithSeat.updated)
            NewTextChild(dataNode, "updated", paxWithSeat.pax_id);
          else
            if (paxWithoutSeat.exists() && paxWithoutSeat.updated)
              NewTextChild(dataNode, "updated", paxWithoutSeat.pax_id);
        };

        //pr_etl_only
        FltQry.SetVariable("point_id",point_id);
        FltQry.Execute();
        if (!FltQry.Eof)
        {
          TTripInfo info(FltQry);
          xmlNodePtr node=NewTextChild(dataNode,"trip_sets");
          NewTextChild( node, "pr_etl_only", (int)GetTripSets(tsETLOnly,info) );
          NewTextChild( node, "pr_etstatus", pr_etstatus );
        }
        else
          throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

        if (screen==sBoarding)
        {
            if (set_mark)
            {
              if (grp_status==psTCheckin &&
                  ((paxWithSeat.exists() && paxWithSeat.ticket_rem!="TKNE") ||
                   (paxWithoutSeat.exists() && paxWithoutSeat.ticket_rem!="TKNE")))
              {
                if (!without_monitor)
                  AstraLocale::showErrorMessage("MSG.ATTENTION_TCKIN_GET_COUPON");
                else
                  AstraLocale::showMessage("MSG.ATTENTION_TCKIN_GET_COUPON");
              }
              else
                AstraLocale::showMessage("MSG.PASSENGER.BOARDING");
            }
            else
                AstraLocale::showMessage("MSG.PASSENGER.DEBARKED");
        }
        else
        {
            if (set_mark)
                AstraLocale::showMessage("MSG.PASSENGER.EXAM");
            else
                AstraLocale::showMessage("MSG.PASSENGER.RETURNED_EXAM");
        };
      }
      catch(int) {};
    }
    else
    {

    };

    //выводим список или измененных пассажиров в терминал
    xmlNodePtr listNode = NewTextChild(dataNode, "passengers");

    Qry.Clear();
    GetPaxQuery(Qry, point_id, showWholeFlight?NoExists:reg_no, NoExists, reqInfo->desk.lang, rtUnknown, "");
    Qry.Execute();
    if (!Qry.Eof)
    {
      TQuery TCkinQry(&OraSession);

      bool free_seating=SALONS2::isFreeSeating(point_id);

      string def_pers_type=ElemIdToCodeNative(etPersType, EncodePerson(ASTRA::adult));
      string def_class=ElemIdToCodeNative(etClass, EncodeClass(ASTRA::Y));

      xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
      NewTextChild(defNode, "pr_brd", (int)false);
      NewTextChild(defNode, "pr_exam", (int)false);
      NewTextChild(defNode, "name", "");
      NewTextChild(defNode, "pers_type", def_pers_type);
      NewTextChild(defNode, "class", def_class);
      NewTextChild(defNode, "seats", 1);
      NewTextChild(defNode, "ticket_no", "");
      NewTextChild(defNode, "coupon_no", 0);
      NewTextChild(defNode, "document", "");
      NewTextChild(defNode, "excess", 0);
      NewTextChild(defNode, "value_bag_count", 0);
      NewTextChild(defNode, "pr_payment", (int)false);
      NewTextChild(defNode, "bag_amount", 0);
      NewTextChild(defNode, "bag_weight", 0);
      NewTextChild(defNode, "rk_amount", 0);
      NewTextChild(defNode, "rk_weight", 0);
      NewTextChild(defNode, "tags", "");
      NewTextChild(defNode, "remarks", "");
      NewTextChild(defNode, "client_name", "");
      NewTextChild(defNode, "inbound_flt", "");
      NewTextChild(defNode, "inbound_delay_alarm", (int)false);
      NewTextChild(defNode, "document_alarm", (int)false);

      int col_pax_id=Qry.FieldIndex("pax_id");
      int col_grp_id=Qry.FieldIndex("grp_id");
      int col_pr_brd=Qry.FieldIndex("pr_brd");
      int col_pr_exam=Qry.FieldIndex("pr_exam");
      int col_reg_no=Qry.FieldIndex("reg_no");
      int col_surname=Qry.FieldIndex("surname");
      int col_name=Qry.FieldIndex("name");
      int col_pers_type=Qry.FieldIndex("pers_type");
      int col_class=Qry.FieldIndex("class");
      int col_airp_arv=Qry.FieldIndex("airp_arv");
      int col_last_airp_arv=Qry.FieldIndex("last_airp_arv");
      int col_seat_no=Qry.FieldIndex("seat_no");
      int col_seats=Qry.FieldIndex("seats");
      int col_wl_type=Qry.FieldIndex("wl_type");
      int col_ticket_no=Qry.FieldIndex("ticket_no");
      int col_coupon_no=Qry.FieldIndex("coupon_no");
      int col_tid=Qry.FieldIndex("tid");
      int col_bag_amount=Qry.FieldIndex("bag_amount");
      int col_bag_weight=Qry.FieldIndex("bag_weight");
      int col_rk_amount=Qry.FieldIndex("rk_amount");
      int col_rk_weight=Qry.FieldIndex("rk_weight");
      int col_excess=Qry.FieldIndex("excess");
      int col_tags=Qry.FieldIndex("tags");
      int col_client_type=Qry.FieldIndex("client_type");
      int col_tckin_id=Qry.FieldIndex("tckin_id");
      int col_seg_no=Qry.FieldIndex("seg_no");

      TCkinRoute tckin_route;
      TPaxSeats priorSeats(point_id);
      TRemGrp rem_grp;
      if(not Qry.Eof) rem_grp.Load(retBRD_VIEW, point_id);
      for(;!Qry.Eof;Qry.Next())
      {
          int pax_id=Qry.FieldAsInteger(col_pax_id);
          string surname=Qry.FieldAsString(col_surname);
          string name=Qry.FieldAsString(col_name);
          string airp_arv=Qry.FieldAsString(col_airp_arv);

          xmlNodePtr paxNode = NewTextChild(listNode, "pax");
          NewTextChild(paxNode, "pax_id", pax_id);
          NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
          NewTextChild(paxNode, "pr_brd",  Qry.FieldAsInteger(col_pr_brd)!=0, false);
          NewTextChild(paxNode, "pr_exam", Qry.FieldAsInteger(col_pr_exam)!=0, false);
          NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
          NewTextChild(paxNode, "surname", Qry.FieldAsString(col_surname));
          NewTextChild(paxNode, "name", Qry.FieldAsString(col_name), "");
          NewTextChild(paxNode, "pers_type", ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)), def_pers_type);
          NewTextChild(paxNode, "class", ElemIdToCodeNative(etClass, Qry.FieldAsString(col_class)), def_class);
          NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_last_airp_arv)));
          NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
          NewTextChild(paxNode, "seats", Qry.FieldAsInteger(col_seats), 1);
          if (!free_seating)
          {
            if (Qry.FieldIsNULL(col_wl_type))
            {
              //не на листе ожидания, но возможно потерял место при смене компоновки
              if (Qry.FieldIsNULL(col_seat_no) && Qry.FieldAsInteger(col_seats)>0)
              {
                ostringstream seat_no_str;
                seat_no_str << "("
                            << priorSeats.getSeats(pax_id,"seats")
                            << ")";
                NewTextChild(paxNode,"seat_no_str",seat_no_str.str());
                NewTextChild(paxNode,"seat_no_alarm",(int)true);
              };
            }
            else
            {
              NewTextChild(paxNode,"seat_no_str",AstraLocale::getLocaleText("ЛО"));
              NewTextChild(paxNode,"seat_no_alarm",(int)true);
            };
          };
          NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no), "");
          NewTextChild(paxNode, "coupon_no", Qry.FieldAsInteger(col_coupon_no), 0);
          NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(NoExists, pax_id, false), "");
          NewTextChild(paxNode, "tid", Qry.FieldAsInteger(col_tid));
          NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess), 0);
          int value_bag_count;
          bool pr_payment=BagPaymentCompleted(Qry.FieldAsInteger(col_grp_id), &value_bag_count);
          NewTextChild(paxNode, "value_bag_count", value_bag_count, 0);
          NewTextChild(paxNode, "pr_payment", (int)pr_payment, (int)false);
          NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount), 0);
          NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight), 0);
          NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger(col_rk_amount), 0);
          NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight), 0);
          NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags), "");
          NewTextChild(paxNode, "remarks", GetRemarkStr(rem_grp, pax_id), "");
          if (DecodeClientType(Qry.FieldAsString(col_client_type))!=ctTerm)
            NewTextChild(paxNode, "client_name", ElemIdToNameShort(etClientType, Qry.FieldAsString(col_client_type)));

          TCheckDocInfo check_info;
          CheckIn::TAPISItem apis;
          set<APIS::TAlarmType> alarms;
          bool document_alarm=false;

        /*  ProgTrace(TRACE5, "pax_id=%d name=%s airp_arv=%s apis_control=%d apis_manual_input=%d apis_map.size()=%zu",
                    pax_id, name.c_str(), airp_arv.c_str(), (int)apis_control, (int)apis_manual_input, apis_map.size());
        */
          if (GetAPISAlarms(pax_id, name=="CBBG", airp_arv, apis_control, apis_manual_input, apis_map,
                            check_info, apis, alarms, document_alarm))
          {
            if (document_alarm)
              NewTextChild(paxNode, "document_alarm", (int)true);
            if(search_type==updateByPaxId ||
               search_type==updateByRegNo ||
               search_type==updateByScanData ||
               search_type==refreshByPaxId)
              //для определенных search_type грузим в терминал APIS
              LoadAPIS(check_info, apis, paxNode);
          }
          else
          {
            //тревоги не выводим, но для определенных search_type грузим в терминал APIS
            if(search_type==updateByPaxId ||
               search_type==updateByRegNo ||
               search_type==updateByScanData ||
               search_type==refreshByPaxId)
            {
              TCheckDocInfo check_info;
              TAPISMap::const_iterator iAPISMap=apis_map.find(airp_arv);
              if (iAPISMap!=apis_map.end()) check_info=iAPISMap->second.first.pass;
              LoadAPIS(check_info, CheckIn::TAPISItem().fromDB(pax_id), paxNode);
            };
          };

          if (!alarms.empty())
          {
            if (alarms.size()==1)
              NewTextChild(paxNode, "alarm", APIS::EncodeAlarmType(*(alarms.begin())));
            else
            {
              xmlNodePtr alarmsNode=NewTextChild(paxNode, "alarms");
              for(set<APIS::TAlarmType>::const_iterator a=alarms.begin(); a!=alarms.end(); ++a)
                NewTextChild(alarmsNode, "alarm", APIS::EncodeAlarmType(*a));
            };
          };


          if (!Qry.FieldIsNULL(col_tckin_id))
          {
            TCkinRouteItem inboundSeg;
            tckin_route.GetPriorSeg(Qry.FieldAsInteger(col_tckin_id),
                                    Qry.FieldAsInteger(col_seg_no),
                                    crtIgnoreDependent,
                                    inboundSeg);
            if (inboundSeg.grp_id!=NoExists)
            {
              TTripRouteItem priorAirp;
              TTripRoute().GetPriorAirp(NoExists,inboundSeg.point_arv,trtNotCancelled,priorAirp);
              if (priorAirp.point_id!=NoExists)
              {
                FltQry.SetVariable( "point_id", priorAirp.point_id );
                FltQry.Execute();
                if (!FltQry.Eof)
                {
                  TTripInfo inFlt(FltQry);
                  TDateTime scd_out_local = inFlt.scd_out==NoExists?
                                              NoExists:
                                              UTCToLocal(inFlt.scd_out,AirpTZRegion(inFlt.airp));

                  ostringstream trip;
                  trip << ElemIdToCodeNative(etAirline, inFlt.airline)
                       << setw(3) << setfill('0') << inFlt.flt_no
                       << ElemIdToCodeNative(etSuffix, inFlt.suffix)
                       << '/'
                       << (scd_out_local==NoExists?"??":DateTimeToStr(scd_out_local,"dd"));

                  NewTextChild(paxNode, "inbound_flt", trip.str());

                  TCkinQry.Clear();
                  TCkinQry.SQLText=
                    "SELECT scd_in, NVL(act_in,NVL(est_in,scd_in)) AS real_in "
                    "FROM points WHERE point_id=:point_id";
                  TCkinQry.CreateVariable("point_id",otInteger,inboundSeg.point_arv);
                  TCkinQry.Execute();
                  if (!TCkinQry.Eof && !TCkinQry.FieldIsNULL("scd_in") && !TCkinQry.FieldIsNULL("real_in"))
                  {
                    if (TCkinQry.FieldAsDateTime("real_in")-TCkinQry.FieldAsDateTime("scd_in") >= 20.0/1440)  //задержка более 20 мин
                      NewTextChild(paxNode, "inbound_delay_alarm", (int)true);
                  };
                };
              };
            };
          };
      };//for(;!Qry.Eof;Qry.Next())
    };
    TRptParams rpt_params(reqInfo->desk.lang);
    readTripCounters(point_id, rpt_params, dataNode, rtUnknown, "");
};

void BrdInterface::LoadPaxAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int pax_id=NodeAsInteger("pax_id",reqNode);
  int tid=NodeAsInteger("tid",reqNode);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.airp_arv "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id AND pax_grp.point_dep=:point_id AND pax.tid=:tid";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
  string airp_arv=Qry.FieldAsString("airp_arv");

  LoadAPIS(point_id, airp_arv, pax_id, resNode);
};

void BrdInterface::SavePaxAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int pax_id=NodeAsInteger("pax_id",reqNode);
  int tid=NodeAsInteger("tid",reqNode);
  xmlNodePtr apisNode=NodeAsNode("apis", reqNode);

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock();

  SaveAPIS(point_id, pax_id, tid, apisNode);

  GetPax(reqNode, resNode);
};





