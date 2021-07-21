#include "date_time.h"
#include "stat/stat_utils.h"
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
#include "apps_interaction.h"
#include "iapi_interaction.h"
#include "baggage_calc.h"
#include "sopp.h"
#include "rfisc.h"
#include "dev_utils.h"
#include "pax_events.h"
#include "custom_alarms.h"
#include "telegram.h"
#include "pax_calc_data.h"
#include "pax_confirmations.h"
#include "checkin.h"
#include "baggage_ckin.h"

#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

namespace {

std::string get_last_trfer_airp(const GrpId_t& grp_id)
{
  std::string result;
  DB::TQuery Qry(PgOra::getROSession("TRANSFER"), STDLOG);
  Qry.SQLText =
      "SELECT airp_arv "
      "FROM transfer "
      "WHERE grp_id = :grp_id "
      "AND pr_final <> 0 ";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.Execute();
  if (!Qry.Eof) {
    result = Qry.FieldAsString("airp_arv");
  }
  return result;
}

} // namespace

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
        "    pax_grp.point_dep=:point_id AND NVL(pax.cabin_class, pax_grp.class)=:class AND "
        "    pax_grp.status NOT IN ('E') AND "
        "    pax.pr_brd IS NOT NULL ";
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
            sql += " AND pax_grp.client_type IN (:ctWeb, :ctMobile, :ctKiosk) ";
            Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
            Qry.CreateVariable("ctMobile", otString, EncodeClientType(ctMobile));
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
                SQLText += " AND pax_grp.client_type IN (:ctWeb, :ctMobile, :ctKiosk) ";
                Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
                Qry.CreateVariable("ctMobile", otString, EncodeClientType(ctMobile));
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

void lock_and_get_new_tid(int locked_point_id)
{
  if (locked_point_id!=NoExists)
  {
    TFlights flights;
    flights.Get( locked_point_id, ftTranzit );
    flights.Lock(__FUNCTION__); //лочим весь транзитный рейс
  };
  TCachedQuery Qry("SELECT cycle_tid__seq.nextval FROM dual");
  Qry.get().Execute();
}

void BrdInterface::DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id=NodeAsInteger("point_id",reqNode);
  bool boarding = NodeAsInteger("boarding", reqNode)!=0;

  lock_and_get_new_tid(point_id);

  ostringstream sql;
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  sql.str("");
  sql << "SELECT pax.grp_id, pax_id,reg_no,pr_brd FROM pax_grp,pax "
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

    TAdvTripInfo fltInfo;
    fltInfo.getByPointId(point_id);

    set<int> bsm_grp;

    for(;!PaxQry.Eof;PaxQry.Next())
    {
      int pax_id=PaxQry.FieldAsInteger("pax_id");
      RegNo_t regNo(PaxQry.FieldAsInteger("reg_no"));
      Qry.SetVariable("pax_id", pax_id);

      bool boarded=!PaxQry.FieldIsNULL("pr_brd") && PaxQry.FieldAsInteger("pr_brd")!=0;

      if (reqInfo->screen.name == "BRDBUS.EXE" and boarded)
          bsm_grp.insert(PaxQry.FieldAsInteger("grp_id"));

      Qry.Execute();

      TPaxEvent().toDB(pax_id, (boarding ? TPaxEventTypes::BRD : TPaxEventTypes::UNBRD));

      if (Qry.RowsProcessed()>0)
        rozysk::sync_pax(pax_id, reqInfo->desk.code, reqInfo->user.descr);

      if (reqInfo->screen.name == "BRDBUS.EXE" and boarded)
          updatePaxChange( fltInfo, PaxId_t(pax_id), regNo, TermWorkingMode::Boarding );
    };
    if(not bsm_grp.empty()) {
        BSM::TBSMAddrs BSMaddrs;
        BSM::IsSend(fltInfo, BSMaddrs, true);
        if(not BSMaddrs.empty())
            for(const auto &i: bsm_grp)
                BSM::Send(point_id,i,true,BSM::TTlgContentCHG(),BSMaddrs);
    }
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
    TTripAlarmHook::set(Alarm::Brd, point_id);
    check_u_trfer_alarm_for_next_trfer(point_id, idFlt);
  };

  GetPax(reqNode,resNode);

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
    TPaxEvent().toDB(pax_id, (mark ? TPaxEventTypes::BRD : TPaxEventTypes::UNBRD));
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
    RegNo_t regNo(Qry.FieldAsInteger("reg_no"));

    string lexema_id;
    if (reqInfo->screen.name == "BRDBUS.EXE")
    {
      if (pr_exam_with_brd)
        lexema_id = (mark ? "EVT.PASSENGER.EXAMED_WITH_BRD" : "EVT.PASSENGER.NOT_EXAMED_WITH_BRD");
      else
        lexema_id = (mark ? "EVT.PASSENGER.BOARDED" : "EVT.PASSENGER.NOT_BOARDED");
      if (is_sync_paxs(point_id))
        updatePaxChange( PointId_t(point_id), PaxId_t(pax_id), regNo, TermWorkingMode::Boarding);
    }
    else
      lexema_id = (mark ? "EVT.PASSENGER.EXAMED" : "EVT.PASSENGER.NOT_EXAMED");

    TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << PrmSmpl<string>("surname", Qry.FieldAsString("surname"))
                                      << PrmSmpl<string>("name", Qry.FieldAsString("name")),
                                      evtPax, point_id, regNo.get(), grp_id);

    //отвяжем сквозняков от предыдущих сегментов
    SeparateTCkin(grp_id,cssAllPrevCurr,cssCurr,Qry.FieldAsInteger("tid"));

    if (reqInfo->screen.name == "BRDBUS.EXE")
    {
      TTripAlarmHook::set(Alarm::Brd, point_id);
      check_u_trfer_alarm_for_next_trfer(grp_id, idGrp);
    };
  };
  return true;
}

std::string get_bp_seat_no_lat(TDevOper::Enum op_type, int pax_id)
{
    DB::TCachedQuery Qry(
            PgOra::getROSession("CONFIRM_PRINT"),
            "SELECT confirm_print.seat_no_lat "
            "FROM confirm_print, "
            "     (SELECT MAX(time_print) AS time_print FROM confirm_print "
            "      WHERE pax_id=:pax_id "
            "      AND voucher IS NULL "
            "      AND pr_print<>0 "
            "      AND op_type = :op_type) a "
            "WHERE confirm_print.time_print=a.time_print "
            "AND confirm_print.pax_id=:pax_id "
            "AND voucher IS NULL "
            "AND confirm_print.op_type = :op_type ",
            QParams() << QParam("pax_id", otInteger, pax_id) << QParam("op_type", otString, DevOperTypes().encode(op_type)),
            STDLOG);
    Qry.get().Execute();
    std::string result;
    if(not Qry.get().Eof)
        result = Qry.get().FieldAsString("seat_no_lat");
    return result;
}

bool CheckSeat(int pax_id, const string& scan_data)
{
    string scan_seat_no;
    if (!scan_data.empty())
    {
      BCBPSections sections;
      try
      {
        BCBPSections::get(scan_data, 0, scan_data.size(), sections, true);
        scan_seat_no=sections.seat_number(0);
      }
      catch(EXCEPTIONS::Exception &e)
      {
        LogTrace(TRACE5) << '\n' << sections;
        ProgTrace(TRACE5, "%s: %s", __FUNCTION__, e.what());
      }
    }

    if (scan_seat_no.empty())
    {
      //пытаемся сравнить с последним номером места, который отдали на печать
      DB::TQuery Qry(PgOra::getROSession("PAX-salons.get_seat_no"), STDLOG);
      Qry.SQLText =
        "SELECT salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,NULL,NULL,'list',1,1) AS curr_seat_no_list_lat "
        "FROM pax "
        "WHERE pax.pax_id=:pax_id";
      Qry.CreateVariable("pax_id", otInteger, pax_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        const std::string bp_seat_no_lat = get_bp_seat_no_lat(TDevOper::PrnBP, pax_id);
        if (!bp_seat_no_lat.empty() &&
            bp_seat_no_lat != Qry.FieldAsString("curr_seat_no_list_lat"))
        {
          return false;
        }
      }
    }
    else
    {
      //сравниваем с номером места, который пришел в 2D
      TQuery Qry(&OraSession);
      Qry.SQLText =
        "SELECT LPAD(salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,NULL,NULL,'one',1,0),4,'0') AS curr_seat_no_one, "
        "       LPAD(salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,NULL,NULL,'one',1,1),4,'0') AS curr_seat_no_one_lat "
        "FROM pax "
        "WHERE pax_id=:pax_id";
      Qry.CreateVariable("pax_id", otInteger, pax_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        if (scan_seat_no!=Qry.FieldAsString("curr_seat_no_one") &&
            scan_seat_no!=Qry.FieldAsString("curr_seat_no_one_lat"))
          return false;
      }
    }

    return true;
}

void BrdInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetPax(reqNode,resNode);
};

std::string BrdInterface::get_seat_no(const PaxId_t& pax_id, int seats, int is_jmp, const std::string& status,
                                      const PointId_t& point_dep, int rownum)
{
  TQuery Qry(&OraSession, STDLOG);
  Qry.SQLText = "SELECT salons.get_seat_no(:pax_id,:seats,:is_jmp,:status,:point_dep,'_seats',:num) AS seat_no FROM DUAL ";
  Qry.CreateVariable("pax_id",otInteger,pax_id.get());
  Qry.CreateVariable("seats",otInteger,seats);
  Qry.CreateVariable("is_jmp",otInteger,is_jmp);
  Qry.CreateVariable("status",otString,status);
  Qry.CreateVariable("point_dep",otInteger,point_dep.get());
  Qry.CreateVariable("num",otInteger,rownum);
  Qry.Execute();
  return Qry.FieldAsString("seat_no");
}

void BrdInterface::GetPaxQuery(DB::TQuery &Qry,
                               const int point_id,
                               const int reg_no,
                               const int pax_id,
                               const ASTRA::TRptType rpt_type,
                               const std::string &client_type,
                               const TSortType sort,
                               const bool usePaxCalcData)
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
    const bool used_for_web_rpt = (rpt_type == rtWEB or rpt_type == rtWEBTXT);
    const bool used_for_norec_rpt = (rpt_type == rtNOREC or rpt_type == rtNORECTXT);
    const bool used_for_gosho_rpt = (rpt_type == rtGOSHO or rpt_type == rtGOSHOTXT);
    const bool used_for_brd_and_exam = (rpt_type == rtUnknown);
    ostringstream sql;
    sql <<
        "SELECT "
        "    pax_grp.grp_id, "
        "    pax_grp.airp_arv, "
        "    pax_grp.class, "
        "    COALESCE(pax.cabin_class, pax_grp.class) AS cabin_class, "
        "    pax_grp.status, "
        "    pax_grp.client_type, "
        "    pax_grp.excess_wt, pax_grp.bag_refuse, "
        "    pax.*, "
        "    pax_grp.point_dep, "
        "    NULL AS excess_pc ";
    if (used_for_brd_and_exam)
        sql << ", tckin_pax_grp.tckin_id "
               ", tckin_pax_grp.grp_num "
               ", crs_pax.pax_id AS crs_pax_id "
               ", crs_pax.bag_norm AS crs_bag_norm "
               ", crs_pax.bag_norm_unit AS crs_bag_norm_unit ";

    if (used_for_web_rpt)
        sql << ", users2.descr AS user_descr ";

    if (usePaxCalcData)
        sql << ", pax_calc_data.* ";

    sql << "FROM pax_grp ";
    if (used_for_brd_and_exam) {
      sql << "LEFT OUTER JOIN tckin_pax_grp ON pax_grp.grp_id = tckin_pax_grp.grp_id ";
    }
    if (used_for_web_rpt) {
      sql << "LEFT OUTER JOIN users2 ON pax_grp.user_id = users2.user_id ";
    }

    sql << "JOIN ";

    if (used_for_norec_rpt
        or used_for_gosho_rpt
        or used_for_brd_and_exam
        or usePaxCalcData)
    {
      sql << "(";
    }
    sql << "pax ";

    if (used_for_norec_rpt or used_for_gosho_rpt) {
      sql << "LEFT OUTER JOIN crs_pax ON pax.pax_id = crs_pax.pax_id "
          << "                        AND (crs_pax.pax_id IS NULL OR crs_pax.pr_del <> 0) ";
    }
    if (used_for_brd_and_exam) {
      sql << "LEFT OUTER JOIN crs_pax ON pax.pax_id = crs_pax.pax_id AND crs_pax.pr_del = 0 ";
    }
    if (usePaxCalcData) {
      sql << "LEFT OUTER JOIN pax_calc_data ON pax.pax_id = pax_calc_data.pax_calc_data_id ";
    }

    if (used_for_norec_rpt
        or used_for_gosho_rpt
        or used_for_brd_and_exam
        or usePaxCalcData)
    {
      sql << ") ";
    }

    sql << "ON pax_grp.grp_id = pax.grp_id "
        << "WHERE ";

    if(used_for_gosho_rpt) {
      sql << "pax_grp.status NOT IN(:psCheckin, :psTCheckin) AND ";
      Qry.CreateVariable("psCheckin", otString, EncodePaxStatus(psCheckin));
      Qry.CreateVariable("psTCheckin", otString, EncodePaxStatus(psTCheckin));
    }

    sql << "point_dep = :point_id AND pax_grp.status NOT IN ('E') AND pr_brd IS NOT NULL ";

    if (used_for_web_rpt) {
        if(!client_type.empty()) {
            sql << " AND pax_grp.client_type = :client_type ";
            Qry.CreateVariable("client_type",otString,client_type);
        } else {
            sql << " AND pax_grp.client_type IN (:ctWeb, :ctMobile, :ctKiosk) ";
            Qry.CreateVariable("ctWeb", otString, EncodeClientType(ctWeb));
            Qry.CreateVariable("ctMobile", otString, EncodeClientType(ctMobile));
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
        case stServiceCode:
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
    Qry.SQLText = sql.str().c_str();
}

void put_exambrd_vars(xmlNodePtr variablesNode)
{
    NewTextChild(variablesNode, "cap_checked", getLocaleText("CAP.DOC.EXAMBRD.CHECKED.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_exam", getLocaleText("CAP.DOC.EXAMBRD.EXAM.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_brd", getLocaleText("CAP.DOC.EXAMBRD.BRD.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_no_exam", getLocaleText("CAP.DOC.EXAMBRD.NO_EXAM.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
    NewTextChild(variablesNode, "cap_no_brd", getLocaleText("CAP.DOC.EXAMBRD.NO_BRD.FLIGHT", LParams() << LParam("flight", get_flight(variablesNode))));
}

static void LoadAPIS(TPaxTypeExt pax_ext, const TCompleteAPICheckInfo &check_info, const CheckIn::TAPISItem &apis, xmlNodePtr resNode)
{
  if (resNode==NULL) return;

  xmlNodePtr infoNode=NewTextChild(resNode, "check_info");
  check_info.get(pax_ext).toXML(infoNode);
  apis.toXML(resNode);
}

static void LoadAPIS(const TPaxSegmentPair& paxSegment, TPaxTypeExt pax_ext, int pax_id, xmlNodePtr resNode)
{
  if (resNode==NULL) return;

  LoadAPIS(pax_ext, TCompleteAPICheckInfo(paxSegment), CheckIn::TAPISItem().fromDB(pax_id), resNode);
}

void SaveAPIS(int point_id, int pax_id, int tid, xmlNodePtr reqNode)
{
  if (reqNode==NULL) return;
  //берем новый tid
  lock_and_get_new_tid(point_id); //здесь лочим

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.* FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND pax.pax_id=:pax_id AND pax.tid=:tid";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  CheckIn::TSimplePaxGrpItem grp;
  grp.fromDB(Qry);

  CheckIn::TSimplePaxItem pax;
  if (!pax.getByPaxId(pax_id))
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

  if (!(pax.refuse.empty() && pax.api_doc_applied())) return;

  bool apis_control=GetAPISControl(point_id);
  TCompleteAPICheckInfo checkInfo(grp.getSegmentPair());
  TDateTime checkDate=UTCToLocal(NowUTC(), AirpTZRegion(grp.airp_dep));

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

  PaxIdWithSegmentPair paxId(PaxId_t(pax_id), grp.getSegmentPair());
  ModifiedPaxRem modifiedPaxRem(paxCheckIn);

  CheckIn::TAPISItem apis=prior_apis;
  xmlNodePtr node2=reqNode->children;
  xmlNodePtr docNode;
  docNode=GetNodeFast("document",node2);
  if (docNode!=nullptr)
  {
    apis.doc.fromXML(docNode);
    HandleDoc(grp, pax, checkInfo, checkDate, apis.doc);
    CheckIn::SavePaxDoc(paxId, apis.doc, prior_apis.doc, modifiedPaxRem);
  }
  docNode=GetNodeFast("doco",node2);
  if (docNode!=nullptr)
  {
    apis.doco.fromXML(docNode);
    HandleDoco(grp, pax, checkInfo, checkDate, apis.doco);
    CheckIn::SavePaxDoco(paxId, apis.doco, prior_apis.doco, modifiedPaxRem);
  }
  xmlNodePtr docaNode=GetNodeFast("addresses",node2);
  if (docaNode!=nullptr)
  {
    apis.doca_map.fromXML(docaNode);
    HandleDoca(grp, pax, checkInfo, apis.doca_map);
    CheckIn::SavePaxDoca(paxId, apis.doca_map, prior_apis.doca_map, modifiedPaxRem);
  };

  modifiedPaxRem.executeCallbacksByPaxId(TRACE5);
  modifiedPaxRem.executeCallbacksByCategory(TRACE5);

  PaxCalcData::ChangesHolder changesHolder(paxCheckIn);
  PaxCalcData::addChanges(modifiedPaxRem, changesHolder);
  changesHolder.executeCallbacksByPaxId(TRACE5);

  if (modifiedPaxRem.exists({remDOC, remDOCO, remDOCA}))
  {
    APPS::AppsCollector appsCollector;
    appsCollector.addPaxItem(PaxId_t(pax_id));
    appsCollector.send();
  }

  if (apis_control)
  {
    list<pair<string, string> > msgs;
    GetAPISLogMsgs(prior_apis, apis, msgs);
    for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
    {
      LEvntPrms params;
      logPaxName(EncodePaxStatus(grp.status), pax.surname, pax.name, EncodePerson(pax.pers_type), params);
      TReqInfo::Instance()->LocaleToLog((*m).first, params << PrmSmpl<string>("params", (*m).second),
                                     ASTRA::evtPax, point_id, pax.reg_no, grp.id);
    };
  }
  else
  {
    if (!(apis.doc.equalAttrs(prior_apis.doc) &&
          apis.doco.equalAttrs(prior_apis.doco) &&
          apis.doca_map==prior_apis.doca_map))
    {
      string lexema_id=grp.status!=psCrew?"EVT.CHANGED_PASSENGER_DATA":
                                          "EVT.CHANGED_CREW_MEMBER_DATA";
      LEvntPrms params;
      logPaxName(EncodePaxStatus(grp.status), pax.surname, pax.name, EncodePerson(pax.pers_type), params);
      TReqInfo::Instance()->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, pax.reg_no, grp.id);
    };
  };
};

static set<APIS::TAlarmType> getAPISAlarms(AllAPIAttrs& allAPIAttrs,
                                           DB::TQuery& Qry,
                                           const bool apiDocApplied,
                                           const ASTRA::TPaxTypeExt paxTypeExt,
                                           const bool docoConfirmed,
                                           const bool crsExists,
                                           const TRouteAPICheckInfo& routeCheckInfo,
                                           const AirportCode_t& airpArv,
                                           const TTripSetList& setList,
                                           bool &documentAlarm)
{
  static set<APIS::TAlarmType> requiredAlarms={APIS::atDiffersFromBooking,
                                               APIS::atIncomplete,
                                               APIS::atManualInput};

  documentAlarm=false;
  set<APIS::TAlarmType> result;

  if (!setList.value<bool>(tsAPISControl)) return result;


  boost::optional<const TCompleteAPICheckInfo&> checkInfo=routeCheckInfo.get(airpArv.get());
  if (checkInfo && !checkInfo.get().apis_formats().empty())
  {
    result=allAPIAttrs.getAlarms(Qry, apiDocApplied, paxTypeExt, docoConfirmed, crsExists, checkInfo.get(), requiredAlarms);
    documentAlarm=result.count(APIS::atIncomplete)>0 ||
                  (!setList.value<bool>(tsAPISManualInput) && result.count(APIS::atManualInput)>0);
  }

  return result;
}

const int DOCUMENT_ALARM_ERRCODE=128;

static void setDeboardingMessage(const string& sbgError,
                                 const bool sbgTimeout,
                                 const bool withoutMonitor,
                                 const AstraLocale::LParams& fullnameParam,
                                 boost::optional<LexemaData>& error,
                                 boost::optional<LexemaData>& warning,
                                 boost::optional<LexemaData>& message)
{
  error=boost::none;
  warning=boost::none;
  message=boost::none;

  if (withoutMonitor || (sbgError.empty() && !sbgTimeout))
  {
    message=boost::in_place("MSG.PASSENGER.DEBARKED2", fullnameParam);
    return;
  };

  static const vector<string> brdConfirmErrorCodes= {"FR1R", "FR2P", "CNXX", "FR1X", "FR2X"};
  static const vector<string> brdCancelErrorCodes= {"TODT", "TOND", "FR3X", "CNXI", "CNXB", "TEHF", "TEHS"};

  for(const string& errorCode : brdConfirmErrorCodes)
    if (sbgError.find("#"+errorCode)!=string::npos)
    {
      AstraLocale::LParams lParams = fullnameParam;
      error=boost::in_place("WRAP.PASSENGER.BOARDING2", lParams << AstraLocale::LParam("text", LexemaData("MSG.SBG_ERROR."+errorCode)));
      return;
    }

  for(const string& errorCode : brdCancelErrorCodes)
    if (sbgError.find("#"+errorCode)!=string::npos)
    {
      AstraLocale::LParams lParams = fullnameParam;
      warning=boost::in_place("WRAP.PASSENGER.NOT_BOARDING2", lParams << AstraLocale::LParam("text", LexemaData("MSG.SBG_ERROR."+errorCode)));
      return;
    }

  warning=boost::in_place("MSG.PASSENGER.NOT_BOARDING2", fullnameParam);
}

static PaxConfirmations::Segments getForPaxConfirmations(const TTripInfo& flt,
                                                         const GrpId_t& grpId,
                                                         const std::list<PaxId_t>& paxIds)
{
  PaxConfirmations::Segments result;

  result.emplace_back();
  PaxConfirmations::Segment& segment=result.back();
  segment.flt=flt;
  if (!segment.grp.getByGrpId(grpId.get()))
  {
    result.clear();
    return result;
  }
  for(const PaxId_t& paxId : paxIds)
  {
    CheckIn::TSimplePaxItem pax;
    if (pax.getByPaxId(paxId.get()))
      segment.paxs.emplace(paxId, pax);
  }

  return result;
}

void BrdInterface::GetPax(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    enum TSearchType {updateByPaxId,
                      updateByRegNo,
                      updateByScanData,
                      refreshByPaxId,
                      refreshWholeFlight};

    TSearchType search_type=refreshWholeFlight;
    if( strcmp((const char*)reqNode->name, "PaxByPaxId") == 0) search_type=updateByPaxId;
    else
      if( strcmp((const char*)reqNode->name, "PaxByRegNo") == 0) search_type=updateByRegNo;
      else
        if( strcmp((const char*)reqNode->name, "PaxByScanData") == 0) search_type=updateByScanData;
        else
          if( strcmp((const char*)reqNode->name, "SavePaxAPIS") == 0) search_type=refreshByPaxId;

    TReqInfo *reqInfo = TReqInfo::Instance();

    enum TScreenType {sSecurity, sBoarding};
    TScreenType screen=(reqInfo->screen.name == "BRDBUS.EXE"?sBoarding:sSecurity);

    ScanDeviceInfo scanDevice(GetNode("scan_device", reqNode));

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

    bool showWholeFlight=false;

    DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","PAX"}), STDLOG);
    if(search_type==updateByScanData ||
       search_type==updateByPaxId ||
       search_type==refreshByPaxId)
    {
      if(search_type==updateByPaxId ||
         search_type==refreshByPaxId)
      {
        found_pax_id=NodeAsInteger("pax_id",reqNode);
        //получим point_id и reg_no
        Qry.SQLText=
          "SELECT point_dep,reg_no "
          "FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id AND pr_brd IS NOT NULL";
        Qry.CreateVariable("pax_id",otInteger,found_pax_id);
        Qry.Execute();
        if (Qry.Eof)
        {
          if (search_type==updateByPaxId)
            throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
          if (search_type==refreshByPaxId)
            throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        };
        found_point_id=Qry.FieldAsInteger("point_dep");
        reg_no=Qry.FieldAsInteger("reg_no");
      }
      else
      {
          bool isBoardingPass;
        SearchPaxByScanData(reqNode, found_point_id, reg_no, found_pax_id, isBoardingPass);
        if (found_point_id==NoExists || reg_no==NoExists || found_pax_id==NoExists || !isBoardingPass)
        {
          LogTrace(TRACE5) << __FUNCTION__
                           << ": found_point_id=" << found_point_id
                           << ", reg_no=" << reg_no
                           << ", found_pax_id=" << found_pax_id
                           << boolalpha << ", isBoardingPass=" << isBoardingPass;
          if (!isBoardingPass)
            throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
          if (found_point_id==NoExists)
            throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
          if (found_pax_id==NoExists)
            throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
          throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN_WITH_REG_NO");
        }
      };

      if (point_id!=found_point_id)
      {
        if (search_type==refreshByPaxId)
          throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

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
          TTripInfo info;
          if (info.getByPointId(point_id))
            throw AstraLocale::UserException(100, "MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
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

    TAdvTripInfo fltInfo;
    if (!fltInfo.getByPointId(point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    bool pr_etl_only=GetTripSets(tsETSNoExchange, fltInfo) ||
                     !GetTripSets(tsChangeETStatusWhileBoarding, fltInfo);
    bool check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, fltInfo);

    DB::TQuery etStatusQry(PgOra::getROSession("TRIP_SETS"), STDLOG);
    etStatusQry.SQLText=
        "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id";
    etStatusQry.CreateVariable("point_id",otInteger,point_id);
    etStatusQry.Execute();
    if (etStatusQry.Eof)
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    int pr_etstatus=etStatusQry.FieldAsInteger("pr_etstatus");

    //признак контроля данных APIS
    TTripSetList setList;
    setList.fromDB(point_id);
    if (setList.empty())
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

    //проверяем настройки APIS по каждому направлению
    TRouteAPICheckInfo route_check_info(point_id);
    bool apis_generation = route_check_info.apis_generation();

    if(search_type==updateByPaxId ||
       search_type==updateByRegNo ||
       search_type==updateByScanData)
    {
      bool set_mark = NodeAsInteger("boarding", reqNode)!=0;
      int tid=(GetNode("tid",reqNode)==NULL?NoExists:NodeAsInteger("tid",reqNode));
      string sbg_error = NodeAsString("sbg_error", reqNode, "");
      bool sbg_timeout = NodeAsInteger("sbg_timeout", reqNode, (int)false)!=0;

      TFlights flights;
      flights.Get( point_id, ftTranzit );
      flights.Lock(__FUNCTION__); //лочим весь транзитный рейс

      if (set_mark && !EMDAutoBoundRegNo::exists(reqNode))
      {
        EMDAutoBoundInterface::refreshEmd(EMDAutoBoundRegNo(PointId_t(point_id), RegNo_t(reg_no)), reqNode);
        if (isDoomedToWait()) return;
      };

      DB::TQuery Qry3(PgOra::getROSession({"PAX_GRP", "PAX", "PAX_CALC_DATA"}), STDLOG);
      Qry3.SQLText=
        "SELECT pax_grp.grp_id, pax_grp.point_arv, pax_grp.airp_arv, pax_grp.status, "
        "       pax.pax_id, pax.surname, pax.name, pax.seats, pax.crew_type, "
        "       pax.pr_brd, pax.pr_exam, pax.wl_type, pax.is_jmp, pax.ticket_rem, pax.tid, pax.doco_confirm, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'list',rownum,0) AS seat_no, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'seats',rownum,1) AS seat_no_for_bgr, "
        "       pax_calc_data.* "
        "FROM pax_grp, pax, pax_calc_data "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax.pax_id=pax_calc_data.pax_calc_data_id(+) AND "
        "      point_dep= :point_id AND pax_grp.status NOT IN ('E') AND pr_brd IS NOT NULL AND "
        "      reg_no=:reg_no";
      Qry3.CreateVariable("point_id", otInteger, point_id);
      Qry3.CreateVariable("reg_no", otInteger, reg_no);
      Qry3.Execute();
      if (Qry3.Eof)
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
      int grp_id=Qry3.FieldAsInteger("grp_id");
      int point_arv=Qry3.FieldAsInteger("point_arv");
      string airp_arv=Qry3.FieldAsString("airp_arv");
      TPaxStatus grp_status=DecodePaxStatus(Qry3.FieldAsString("status").c_str());
      ASTRA::TCrewType::Enum crew_type = CrewTypes().decode(Qry3.FieldAsString("crew_type"));
      ASTRA::TPaxTypeExt pax_ext(grp_status, crew_type);

      class TPaxItem
      {
        public:
          int pax_id;
          string surname, name;
          string wl_type;
          string ticket_rem;
          bool pr_brd, pr_exam, already_marked, is_jmp;
          int tid;
          string seat_no;
          string seat_no_for_bgr;
          bool updated;
          bool docoConfirmed;
          set<APIS::TAlarmType> alarms;
          bool documentAlarm;
          TPaxItem() {clear();}
          void clear()
          {
            pax_id=NoExists;
            surname.clear();
            name.clear();
            wl_type.clear();
            ticket_rem.clear();
            pr_brd=false;
            pr_exam=false;
            is_jmp = false;
            already_marked=false;
            tid=NoExists;
            seat_no.clear();
            seat_no_for_bgr.clear();
            updated=false;
            docoConfirmed=false;
            alarms.clear();
            documentAlarm=false;
          }
          bool exists() const
          {
            return pax_id!=NoExists;
          }
          string full_name() const
          {
            ostringstream s;
            s << surname;
            if (!name.empty())
              s << " " << name;
            return s.str();
          }
          string bgr_message(const ScanDeviceInfo &dev) const
          {
            list<string> msg;
            //1-я строка
            msg.push_back(transliter(full_name(), 1, true));

            string seat_str="SEAT: "+seat_no_for_bgr;
            if (seat_str.size()>dev.display_width())
              seat_str=seat_no_for_bgr;
            if (seat_str.size()>dev.display_width())
              seat_str.clear();
            //2-я строка
            msg.push_back(seat_str);

            return dev.bgr_message(msg);
          }

      };

      TPaxItem paxWithSeat, paxWithoutSeat;
      string surname;
      string name;
      AllAPIAttrs allAPIAttrs(fltInfo.scd_out);
      for(;!Qry3.Eof;Qry3.Next())
      {
        if (grp_id!=Qry3.FieldAsInteger("grp_id"))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        int seats=Qry3.FieldAsInteger("seats");
        TPaxItem &pax=seats>0?paxWithSeat:paxWithoutSeat;
        if (pax.exists())
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        pax.pax_id=Qry3.FieldAsInteger("pax_id");
        pax.surname=Qry3.FieldAsString("surname");
        pax.name=Qry3.FieldAsString("name");
        pax.wl_type=Qry3.FieldAsString("wl_type");
        pax.ticket_rem=Qry3.FieldAsString("ticket_rem");
        pax.pr_brd=Qry3.FieldAsInteger("pr_brd")!=0;
        pax.pr_exam=Qry3.FieldAsInteger("pr_exam")!=0;
        pax.already_marked=(screen==sBoarding?pax.pr_brd:pax.pr_exam);
        pax.tid=Qry3.FieldAsInteger("tid");
        pax.seat_no=Qry3.FieldAsString("seat_no");
        pax.seat_no_for_bgr=Qry3.FieldAsString("seat_no_for_bgr");
        pax.is_jmp = Qry3.FieldAsInteger("is_jmp") != 0;
        pax.docoConfirmed = Qry3.FieldAsInteger("doco_confirm") != 0;
        pax.alarms=getAPISAlarms(allAPIAttrs, Qry3, pax.name!="CBBG", pax_ext, pax.docoConfirmed, false,
                                 route_check_info, AirportCode_t(airp_arv), setList, pax.documentAlarm);
      };

      if (!paxWithSeat.exists() && !paxWithoutSeat.exists())
        throw EXCEPTIONS::Exception("!paxWithSeat.exists() && !paxWithoutSeat.exists()"); //не обязательно, на всякий случай

      AstraLocale::LParams fullnameParam;
      fullnameParam << AstraLocale::LParam("fullname", paxWithSeat.exists()?paxWithSeat.full_name():paxWithoutSeat.full_name());

      bool without_monitor=false;
      DB::TQuery Qry4(PgOra::getROSession("STATIONS"), STDLOG);
      Qry4.SQLText=
          "SELECT without_monitor FROM stations "
          "WHERE desk=:desk AND work_mode=:work_mode";
      Qry4.CreateVariable("desk", otString, reqInfo->desk.code);
      Qry4.CreateVariable("work_mode", otString, "П");
      Qry4.Execute();
      if (!Qry4.Eof) without_monitor=Qry4.FieldAsInteger("without_monitor")!=0;

      boost::optional<LexemaData> error;
      boost::optional<LexemaData> warning;
      boost::optional<LexemaData> message;
      if (screen==sBoarding && !set_mark)
        setDeboardingMessage(sbg_error, sbg_timeout, without_monitor, fullnameParam, error, warning, message);

      class CompleteWithError {};
      class CompleteWithSuccess {};

      try  //catch(CompleteWithError / CompleteWithSuccess)
      {
        if (error)
        {
          AstraLocale::showErrorMessage(error.get());
          throw CompleteWithError();
        }

        //============================ проверка совпадения tid ============================
        if (tid!=NoExists)
        {
          //проверим чтобы хотя бы один из двух tid совпадал
          if ((!paxWithSeat.exists() || tid!=paxWithSeat.tid) &&
              (!paxWithoutSeat.exists() || tid!=paxWithoutSeat.tid))
          {
            TPaxItem &pax=paxWithSeat.exists()?paxWithSeat:paxWithoutSeat;
            throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                             AstraLocale::LParams() << AstraLocale::LParam("surname", pax.full_name()));
          };
        };

        //============================ проверка признака досмотра/посадки ============================
        if ((!paxWithSeat.exists() || set_mark==paxWithSeat.already_marked) &&
            (!paxWithoutSeat.exists() || set_mark==paxWithoutSeat.already_marked))
        {
          if (screen==sBoarding)
          {
            ModelParams modelParams(scanDevice.dev_model());
            int dup_scan_timeout=modelParams.getAsInteger("dup_scan_timeout", 0);
            LogTrace(TRACE5) << "dev_model=" << scanDevice.dev_model() << ", dup_scan_timeout=" << dup_scan_timeout;
            for(int pass=0;pass<2;pass++)
            {
              TPaxItem &pax=(pass==0?paxWithSeat:paxWithoutSeat);
              if (!pax.exists()) continue;
              if (dup_scan_timeout<=0) continue;

              TPaxEvent paxEvent;
              paxEvent.fromDB(pax.pax_id, set_mark?TPaxEventTypes::BRD:
                                                   TPaxEventTypes::UNBRD);

              double interval=(NowUTC()-paxEvent.time)*MSecsPerDay;
              LogTrace(TRACE5) << "pax_id=" << pax.pax_id
                               << ", paxEvent.desk=" << paxEvent.desk
                               << ", interval=" << interval
                               << ", dup_scan_timeout=" << dup_scan_timeout;
              if (paxEvent.desk==TReqInfo::Instance()->desk.code &&
                  interval<=dup_scan_timeout)
              {
                pax.updated=true;
                throw CompleteWithSuccess();
              }
            }

            LEvntPrms prms;
            prms << PrmSmpl<string>("fullname", paxWithSeat.exists()?paxWithSeat.full_name():paxWithoutSeat.full_name());

            if (set_mark)
            {
              TReqInfo::Instance()->LocaleToLog("EVT.PASSENGER.BOARDED_ALREADY", prms, evtPax, point_id, reg_no, grp_id);
              AstraLocale::showErrorMessage("MSG.PASSENGER.BOARDED_ALREADY2", fullnameParam, 120);
            }
            else
              AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BOARDING2", fullnameParam, 120);
          }
          else
          {
            if (set_mark)
              AstraLocale::showErrorMessage("MSG.PASSENGER.EXAMED_ALREADY2", fullnameParam, 120);
            else
              AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM2", fullnameParam, 120);
          };

          throw CompleteWithError();
        };

        //========================= проверка запрета посадки пассажира JMP ==================================
        if (screen==sBoarding) {
          TPaxItem &pax=paxWithoutSeat.exists()?paxWithoutSeat:paxWithSeat;
          if ( pax.exists() &&
               pax.is_jmp &&
               !pax.already_marked &&
                GetTripSets( tsDeniedBoardingJMP, fltInfo ) ) {
              AstraLocale::showErrorMessage("MSG.PASSENGER.JMP_BOARDED_DENIAL");
              throw CompleteWithError();
          }
        }


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
            throw CompleteWithError();
        };

        //============================ настройки рейса ============================
        bool pr_exam_with_brd=false;
        if (screen==sBoarding)
        {
            DB::TQuery THallQry(PgOra::getROSession("TRIP_HALL"), STDLOG);
            THallQry.SQLText=
                "SELECT pr_misc FROM trip_hall "
                "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
                "ORDER BY "
                "CASE WHEN hall is NULL THEN 1 ELSE 0 END";
            THallQry.CreateVariable("point_id",otInteger,point_id);
            THallQry.CreateVariable("hall",otInteger,hall);
            THallQry.CreateVariable("type",otInteger,(int)tsExamWithBrd);
            THallQry.Execute();
            if (!THallQry.Eof) pr_exam_with_brd=THallQry.FieldAsInteger("pr_misc")!=0;
        };

        //============================ проверка листа ожидания ============================
        if (set_mark && !setList.value<bool>(tsFreeSeating) &&
            ((paxWithSeat.exists() && !paxWithSeat.wl_type.empty()) ||
             (paxWithoutSeat.exists() && !paxWithoutSeat.wl_type.empty())))   //!!vlad младенцы без мест и ЛО?
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_CONFIRM_FROM_WAIT_LIST");
            throw CompleteWithError();
        };

        //============================ проверка прохождения досмотра ============================
        if (screen==sBoarding && set_mark && !pr_exam_with_brd && setList.value<bool>(tsExam) &&
            ((paxWithSeat.exists() && !paxWithSeat.pr_exam) ||
             (paxWithoutSeat.exists() && !paxWithoutSeat.pr_exam)))
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM2", fullnameParam);
            throw CompleteWithError();
        };

        //============================ проверка оплаты багажа ============================
        if (set_mark &&
            (screen==sBoarding?setList.value<bool>(tsCheckPay):setList.value<bool>(tsExamCheckPay)) &&
            (!WeightConcept::BagPaymentCompleted(grp_id) ||
             !RFISCPaymentCompleted(grp_id, paxWithSeat.pax_id, check_pay_on_tckin_segs)||
             !RFISCPaymentCompleted(grp_id, paxWithoutSeat.pax_id, check_pay_on_tckin_segs)))
        {
            AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BAG_PAID");
            throw CompleteWithError();
        };

        //============================ проверим APIS alarms ============================
        if (screen==sBoarding && set_mark)
        {
          for(int pass=0;pass<2;pass++)
          {
            TPaxItem &pax=(pass==0?paxWithSeat:paxWithoutSeat);
            if (!pax.exists()) continue;

            if (pax.documentAlarm)
            {
              if (pax.alarms.count(APIS::atIncomplete)>0)
              {
                AstraLocale::showErrorMessage("MSG.PASSENGER.APIS_INCOMPLETE", without_monitor?0:DOCUMENT_ALARM_ERRCODE);
                throw CompleteWithError();
              };
              if (pax.alarms.count(APIS::atManualInput)>0)
              {
                AstraLocale::showErrorMessage("MSG.PASSENGER.APIS_MANUAL_INPUT", without_monitor?0:DOCUMENT_ALARM_ERRCODE);
                throw CompleteWithError();
              };
            };
          };
        };

        //============================ проверка APPS и IAPI статуса пассажира ============================
        if (set_mark)
        {
          if (APPS::checkAPPSSets(PointId_t(point_id), PointId_t(point_arv))) {
              if((paxWithSeat.exists()    && !APPS::allowedToBoarding(PaxId_t(paxWithSeat.pax_id))) ||
                 (paxWithoutSeat.exists() && !APPS::allowedToBoarding(PaxId_t(paxWithoutSeat.pax_id)))) {
                  throw AstraLocale::UserException("MSG.PASSENGER.APPS_PROBLEM");
              }
          }

          boost::optional<const TCompleteAPICheckInfo &> check_info=route_check_info.get(airp_arv);
          if (check_info)
          {
            if ((paxWithSeat.exists()    && !IAPI::PassengerStatus::allowedToBoarding(PaxId_t(paxWithSeat.pax_id), check_info.get())) ||
                (paxWithoutSeat.exists() && !IAPI::PassengerStatus::allowedToBoarding(PaxId_t(paxWithoutSeat.pax_id), check_info.get())))
              throw AstraLocale::UserException("MSG.PASSENGER.APPS_PROBLEM");
          }
        }

        if (!without_monitor)
        {
          //============================ вопрос в терминал ============================
          bool reset=false;
          for(int pass=1;pass<=5;pass++)
          {
            switch(pass)
            {
              case 1: if (screen==sBoarding && set_mark &&
                          GetNode("confirmations/ssr",reqNode)==NULL)
                      {
                        TRemGrp rem_grp;
                        rem_grp.Load(retBRD_WARN, point_id);
                        rem_grp.erase("MSG");
                        if ((!paxWithSeat.exists() || GetRemarkStr(rem_grp, paxWithSeat.pax_id, reqInfo->desk.lang).empty()) &&
                            (!paxWithoutSeat.exists() || GetRemarkStr(rem_grp, paxWithoutSeat.pax_id, reqInfo->desk.lang).empty())) break;

                        xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                        NewTextChild(confirmNode,"reset",(int)reset);
                        NewTextChild(confirmNode,"type","ssr");
                        ostringstream msg;
                        msg << getLocaleText("MSG.PASSENGER.NEEDS_SPECIAL_SERVICE") << endl
                            << getLocaleText("QST.CONTINUE_BRD");
                        NewTextChild(confirmNode,"message",msg.str());
                        throw CompleteWithError();
                      };
                      break;
              case 2: if (screen==sBoarding && set_mark &&
                          GetNode("confirmations/seat_no",reqNode)==NULL)
                      {
                        if (setList.value<bool>(tsFreeSeating)) break;
                        string curr_seat_no, scan_data;
                        if (GetNode("scan_data",reqNode)!=NULL)
                          scan_data=NodeAsString("scan_data",reqNode);
                        if (paxWithSeat.exists())
                        {
                          if (CheckSeat(paxWithSeat.pax_id, scan_data) && !paxWithSeat.seat_no.empty()) break;
                          curr_seat_no=paxWithSeat.seat_no;
                        }
                        else if (paxWithoutSeat.exists())
                        {
                          if (CheckSeat(paxWithoutSeat.pax_id, "")) break;
                          curr_seat_no=paxWithoutSeat.seat_no;
                        }

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
                        throw CompleteWithError();
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
                        msg << getLocaleText("MSG.PASSENGER.BOARDED_ALREADY2", fullnameParam) << endl
                            << getLocaleText("QST.PASSENGER.RETURN_FOR_EXAM");
                        NewTextChild(confirmNode,"message",msg.str());
                        throw CompleteWithError();
                      };
                      break;
              case 4: if (screen==sBoarding && set_mark &&
                          GetNode("confirmations/pr_msg",reqNode)==NULL)
                      {
                        string txt = GetRemarkMSGText( paxWithSeat.pax_id, "MSG" );
                        if ( !txt.empty() ) {
                          std::vector<std::string> strs;
                          SeparateString( txt.c_str(), 30, strs );
                          ostringstream msg;
                          xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                          NewTextChild(confirmNode,"reset",(int)reset);
                          NewTextChild(confirmNode,"type","pr_msg");
                          for ( const auto& s : strs ) {
                            msg << s << endl;
                          }
                          msg  << getLocaleText("QST.CONTINUE_BRD");
                          NewTextChild(confirmNode,"message",msg.str());
                          throw CompleteWithError();
                        }
                      };
                      break;
              case 5: if (screen==sBoarding && set_mark &&
                          reqInfo->desk.compatible(PAX_CONFIRMATIONS_VERSION) &&
                          !PaxConfirmations::AppliedMessages::exists(reqNode))
                      {
                        std::list<PaxId_t> paxIds;
                        if (paxWithSeat.exists()) paxIds.emplace_back(paxWithSeat.pax_id);
                        if (paxWithoutSeat.exists()) paxIds.emplace_back(paxWithoutSeat.pax_id);

                        PaxConfirmations::Messages messages(DCSAction::Boarding,
                                                            getForPaxConfirmations(fltInfo, GrpId_t(grp_id), paxIds),
                                                            false);
                        if (messages.toXML(dataNode, AstraLocale::OutputLang())) throw CompleteWithError();
                      }
                      break;

            };
          }; //for(int pass=1;pass<=5;pass++)
        };

        //============================ собственно посадка пассажира(ов) ============================
        lock_and_get_new_tid(NoExists);  //не лочим рейс, так как лочим ранее

        if (PaxConfirmations::AppliedMessages::exists(reqNode))
        {
          std::list<PaxId_t> paxIds;
          if (paxWithSeat.exists()) paxIds.emplace_back(paxWithSeat.pax_id);
          if (paxWithoutSeat.exists()) paxIds.emplace_back(paxWithoutSeat.pax_id);
          PaxConfirmations::AppliedMessages(reqNode).toLog(getForPaxConfirmations(fltInfo, GrpId_t(grp_id), paxIds));
        }

        boost::optional<BSM::TBSMAddrs> BSMaddrs;

        for(int pass=0;pass<2;pass++)
        {
            TPaxItem &pax=(pass==0?paxWithSeat:paxWithoutSeat);
            if (!pax.exists()) continue;
            if (set_mark==pax.already_marked) continue;

            if (!PaxUpdate(point_id,pax.pax_id,pax.tid,set_mark,pr_exam_with_brd))
                throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                        LParams() << LParam("surname", pax.full_name()));
            //BSM
            if(screen == sBoarding) {
                if(not BSMaddrs) {
                    BSMaddrs = boost::in_place();
                    BSM::IsSend(fltInfo, BSMaddrs.get(), true);
                }
                if(not BSMaddrs->empty())
                    BSM::Send(point_id,pax.pax_id,false,BSM::TTlgContentCHG(),BSMaddrs.get());
            }

            pax.updated=true;
        };

        throw CompleteWithSuccess();
      }
      catch(CompleteWithSuccess)
      {
        xmlNodePtr node=NewTextChild(dataNode,"updated");
        if (paxWithSeat.exists() && paxWithSeat.updated)
          NewTextChild(node, "pax_id", paxWithSeat.pax_id);
        if (paxWithoutSeat.exists() && paxWithoutSeat.updated)
          NewTextChild(node, "pax_id", paxWithoutSeat.pax_id);

        if (paxWithSeat.exists() && paxWithSeat.updated)
          NewTextChild(dataNode, "bgr_message", paxWithSeat.bgr_message(scanDevice), "");
        else if (paxWithoutSeat.exists() && paxWithoutSeat.updated)
          NewTextChild(dataNode, "bgr_message", paxWithoutSeat.bgr_message(scanDevice), "");

        node=NewTextChild(dataNode,"trip_sets");
        NewTextChild( node, "pr_etl_only", (int)pr_etl_only );
        NewTextChild( node, "pr_etstatus", pr_etstatus );

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
                AstraLocale::showMessage("MSG.PASSENGER.BOARDING2", fullnameParam);
            }
            else
            {
              if (warning)
                AstraLocale::showErrorMessage(warning.get());
              else if (message)
                AstraLocale::showMessage(message.get());
              else
                AstraLocale::showMessage("MSG.PASSENGER.DEBARKED2", fullnameParam);
            }
        }
        else
        {
            if (set_mark)
                AstraLocale::showMessage("MSG.PASSENGER.EXAM2", fullnameParam);
            else
            {
              if ((!sbg_error.empty() || sbg_timeout) && !without_monitor)
                AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM2", fullnameParam);
              else
                AstraLocale::showMessage("MSG.PASSENGER.RETURNED_EXAM2", fullnameParam);
            }
        };
      }
      catch(CompleteWithError) {}
    }
    else
    {

    };

    //выводим список или измененных пассажиров в терминал
    xmlNodePtr listNode = NewTextChild(dataNode, "passengers");

    DB::TQuery QryPaxes(PgOra::getROSession({"PAX","PAX_GRP","CRS_PAX","TCKIN_PAX_GRP","PAX_CALC_DATA"}), STDLOG);
    GetPaxQuery(QryPaxes, point_id, showWholeFlight ? NoExists : reg_no, NoExists,
                rtUnknown, "", stRegNo, apis_generation);
    LogTrace(TRACE3) << __func__ << ": SQL=" << QryPaxes.SQLText;
    QryPaxes.Execute();
    LogTrace(TRACE3) << __func__ << ": Eof=" << QryPaxes.Eof;
    if (!QryPaxes.Eof)
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
      NewTextChild(defNode, "bag_norm", "");
      if ( apis_generation ) {
        NewTextChild(defNode, "apisFlags", "");
      }
      NewTextChild(defNode, "document", "");
      NewTextChild(defNode, "excess", "0");
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

      int col_pax_id=QryPaxes.FieldIndex("pax_id");
      int col_grp_id=QryPaxes.FieldIndex("grp_id");
      int col_pr_brd=QryPaxes.FieldIndex("pr_brd");
      int col_pr_exam=QryPaxes.FieldIndex("pr_exam");
      int col_reg_no=QryPaxes.FieldIndex("reg_no");
      int col_surname=QryPaxes.FieldIndex("surname");
      int col_name=QryPaxes.FieldIndex("name");
      int col_pers_type=QryPaxes.FieldIndex("pers_type");
      int col_class=QryPaxes.FieldIndex("class");
      int col_cabin_class=QryPaxes.FieldIndex("cabin_class");
      int col_airp_arv=QryPaxes.FieldIndex("airp_arv");
      int col_status=QryPaxes.FieldIndex("status");
      int col_point_dep=QryPaxes.FieldIndex("point_dep");
      int col_seats=QryPaxes.FieldIndex("seats");
      int col_is_jmp=QryPaxes.FieldIndex("is_jmp");
      int col_wl_type=QryPaxes.FieldIndex("wl_type");
      int col_ticket_no=QryPaxes.FieldIndex("ticket_no");
      int col_coupon_no=QryPaxes.FieldIndex("coupon_no");
      int col_tid=QryPaxes.FieldIndex("tid");
      int col_client_type=QryPaxes.FieldIndex("client_type");
      int col_tckin_id=QryPaxes.FieldIndex("tckin_id");
      int col_grp_num=QryPaxes.FieldIndex("grp_num");
      int col_crs_pax_id=QryPaxes.FieldIndex("crs_pax_id");
      int col_crs_bag_norm=QryPaxes.FieldIndex("crs_bag_norm");
      int col_crs_bag_norm_unit=QryPaxes.FieldIndex("crs_bag_norm_unit");
      int col_refuse=QryPaxes.FieldIndex("refuse");
      int col_doco_confirm=QryPaxes.FieldIndex("doco_confirm");
      int col_bag_refuse=QryPaxes.FieldIndex("bag_refuse");
      int col_excess_wt_raw=QryPaxes.FieldIndex("excess_wt");

      TCkinRoute tckin_route;
      TPaxSeats priorSeats(point_id);
      TRemGrp rem_grp;
      if(not QryPaxes.Eof) rem_grp.Load(retBRD_VIEW, point_id);

      set<TComplexBagExcessNodeList::Props> props={TComplexBagExcessNodeList::ContainsOnlyNonZeroExcess};
      if (NodeAsInteger("col_excess_type", reqNode, (int)false)!=0)
        props.insert(TComplexBagExcessNodeList::UnitRequiredAnyway);

      TComplexBagExcessNodeList excessNodeList(OutputLang(), props, "+");

      boost::optional<TCustomAlarms> custom_alarms;
      AllAPIAttrs allAPIAttrs(fltInfo.scd_out);

      using namespace CKIN;
      BagReader bag_reader(PointId_t(point_id), std::nullopt, READ::BAGS_AND_TAGS);
      MainPax viewPax;
      int rownum = 0;
      for(;!QryPaxes.Eof;QryPaxes.Next())
      {
          rownum++;
          const int grp_id=QryPaxes.FieldAsInteger(col_grp_id);
          const int pax_id=QryPaxes.FieldAsInteger(col_pax_id);
          const int crs_pax_id=QryPaxes.FieldIsNULL(col_crs_pax_id)?NoExists:QryPaxes.FieldAsInteger(col_crs_pax_id);
          const std::string surname=QryPaxes.FieldAsString(col_surname);
          const std::string name=QryPaxes.FieldAsString(col_name);
          const std::string airp_arv=QryPaxes.FieldAsString(col_airp_arv);
          const TPaxStatus grp_status=DecodePaxStatus(QryPaxes.FieldAsString(col_status).c_str());
          const TCrewType::Enum crew_type = CrewTypes().decode(QryPaxes.FieldAsString("crew_type").c_str());
          const ASTRA::TPaxTypeExt pax_ext(grp_status, crew_type);
          const bool docoConfirmed=QryPaxes.FieldAsInteger(col_doco_confirm)!=0;

          const int bag_refuse = QryPaxes.FieldAsInteger(col_bag_refuse);
          std::optional<int> opt_bag_pool_num;
          if(!QryPaxes.FieldIsNULL("bag_pool_num")) {
              opt_bag_pool_num = QryPaxes.FieldAsInteger("bag_pool_num");
              viewPax.saveMainPax(GrpId_t(grp_id), PaxId_t(pax_id), bag_refuse!=0);
          }

          const int excess_wt_raw = QryPaxes.FieldAsInteger(col_excess_wt_raw);
          std::string last_airp_arv = get_last_trfer_airp(GrpId_t(grp_id));
          if (last_airp_arv.empty()) {
              last_airp_arv = airp_arv;
          }

          if(not custom_alarms or not showWholeFlight) {
              custom_alarms = boost::in_place();
              custom_alarms->fromDB(showWholeFlight, showWholeFlight ? point_id : pax_id);
          }

          const std::string seat_no = BrdInterface::get_seat_no(
                PaxId_t(QryPaxes.FieldAsInteger(col_pax_id)),
                QryPaxes.FieldAsInteger(col_seats),
                QryPaxes.FieldAsInteger(col_is_jmp),
                QryPaxes.FieldAsString(col_status),
                PointId_t(QryPaxes.FieldAsInteger(col_point_dep)),
                rownum);
          LogTrace(TRACE3) << __func__ << ": seat_no=" << seat_no;
          xmlNodePtr paxNode = NewTextChild(listNode, "pax");
          NewTextChild(paxNode, "pax_id", pax_id);
          NewTextChild(paxNode, "grp_id", grp_id);
          NewTextChild(paxNode, "pr_brd",  QryPaxes.FieldAsInteger(col_pr_brd)!=0, false);
          NewTextChild(paxNode, "pr_exam", QryPaxes.FieldAsInteger(col_pr_exam)!=0, false);
          NewTextChild(paxNode, "reg_no", QryPaxes.FieldAsInteger(col_reg_no));
          NewTextChild(paxNode, "surname", surname);
          NewTextChild(paxNode, "name", name, "");
          NewTextChild(paxNode, "pers_type", ElemIdToCodeNative(etPersType, QryPaxes.FieldAsString(col_pers_type)), def_pers_type);
          NewTextChild(paxNode, "class", classIdsToCodeNative(QryPaxes.FieldAsString(col_class),
                                                              QryPaxes.FieldAsString(col_cabin_class)), def_class);
          NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, last_airp_arv));
          NewTextChild(paxNode, "seat_no", seat_no);
          NewTextChild(paxNode, "seats", QryPaxes.FieldAsInteger(col_seats), 1);
          if (!free_seating)
          {
            if (QryPaxes.FieldIsNULL(col_wl_type))
            {
              //не на листе ожидания, но возможно потерял место при смене компоновки
              if (seat_no.empty() && QryPaxes.FieldAsInteger(col_seats)>0)
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
          NewTextChild(paxNode, "ticket_no", QryPaxes.FieldAsString(col_ticket_no), "");
          NewTextChild(paxNode, "coupon_no", QryPaxes.FieldAsInteger(col_coupon_no), 0);
          if ( apis_generation ) {
            bool paxNotRefused=!Qry.FieldIsNULL(col_grp_id) && QryPaxes.FieldIsNULL(col_refuse);
            NewTextChild( paxNode, "apisFlags", allAPIAttrs.view(QryPaxes, paxNotRefused), "" );
          }

          NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(std::nullopt, pax_id, false), "");
          NewTextChild(paxNode, "tid", QryPaxes.FieldAsInteger(col_tid));

          excessNodeList.add(paxNode, "excess", TBagPieces(countPaidExcessPC(PaxId_t(pax_id), true /*include_all_svc*/)),
          TBagKilos(viewPax.excessWt(GrpId_t(grp_id), PaxId_t(pax_id), excess_wt_raw)));

          int value_bag_count=0;
          bool pr_payment=RFISCPaymentCompleted(grp_id, pax_id, check_pay_on_tckin_segs) &&
                          WeightConcept::BagPaymentCompleted(grp_id, &value_bag_count);

          if (value_bag_count!=0)
          {
            ostringstream s;
            s << value_bag_count << getLocaleText("ЦБ");
            NewTextChild(paxNode, "value_bag_count", s.str());
          };
          //ticket_bag_norm
          CheckIn::TPaxTknItem tkn;
          tkn.fromDB( QryPaxes );

          boost::optional<TBagQuantity> crsBagNorm=CheckIn::TSimplePaxItem::getCrsBagNorm(QryPaxes, col_crs_bag_norm, col_crs_bag_norm_unit);

          TETickItem etick;
          if (tkn.validET())
            etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false);

          NewTextChild(paxNode, "bag_norm", trueBagNormView(crsBagNorm, etick, OutputLang()), "");

          NewTextChild(paxNode, "pr_payment", (int)pr_payment, (int)false);
          if(opt_bag_pool_num) {
              NewTextChild(paxNode, "bag_amount", bag_reader.bagAmount(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "bag_weight", bag_reader.bagWeight(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "rk_amount", bag_reader.rkAmount(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "rk_weight", bag_reader.rkWeight(GrpId_t(grp_id), opt_bag_pool_num));
              NewTextChild(paxNode, "tags", bag_reader.tags(GrpId_t(grp_id), opt_bag_pool_num, reqInfo->desk.lang));
          }
          NewTextChild(paxNode, "remarks", GetRemarkStr(rem_grp, pax_id, reqInfo->desk.lang), "");

          if (DecodeClientType(QryPaxes.FieldAsString(col_client_type).c_str())!=ctTerm)
            NewTextChild(paxNode, "client_name", ElemIdToNameShort(etClientType, QryPaxes.FieldAsString(col_client_type)));

        /*  ProgTrace(TRACE5, "pax_id=%d name=%s airp_arv=%s apis_control=%d apis_manual_input=%d apis_map.size()=%zu",
                    pax_id, name.c_str(), airp_arv.c_str(), (int)apis_control, (int)apis_manual_input, apis_map.size());
        */

          bool documentAlarm=false;
          set<APIS::TAlarmType> alarms=getAPISAlarms(allAPIAttrs, QryPaxes, name!="CBBG", pax_ext, docoConfirmed, crs_pax_id!=NoExists,
                                                     route_check_info, AirportCode_t(airp_arv), setList, documentAlarm);
          if (documentAlarm)
            NewTextChild(paxNode, "document_alarm", (int)true);

          if (!alarms.empty())
          {
              xmlNodePtr alarmsNode=NewTextChild(paxNode, "alarms");
              for(set<APIS::TAlarmType>::const_iterator a=alarms.begin(); a!=alarms.end(); ++a)
                  NewTextChild(alarmsNode, "alarm", APIS::EncodeAlarmType(*a));
          };

          custom_alarms->toXML(paxNode, pax_id);

          if(search_type==updateByPaxId ||
             search_type==updateByRegNo ||
             search_type==updateByScanData ||
             search_type==refreshByPaxId)
          {
            //для определенных search_type грузим в терминал APIS
            boost::optional<const TCompleteAPICheckInfo &> check_info=route_check_info.get(airp_arv);
            LoadAPIS(pax_ext,
                     check_info?check_info.get():TCompleteAPICheckInfo(),
                     CheckIn::TAPISItem().fromDB(pax_id),
                     paxNode);
          }

          if (!QryPaxes.FieldIsNULL(col_tckin_id))
          {
            auto inboundGrp=tckin_route.getPriorGrp(QryPaxes.FieldAsInteger(col_tckin_id),
                                                    QryPaxes.FieldAsInteger(col_grp_num),
                                                    TCkinRoute::IgnoreDependence,
                                                    TCkinRoute::WithTransit);

            if (inboundGrp)
            {
              TTripRouteItem priorAirp;
              TTripRoute().GetPriorAirp(NoExists,inboundGrp.get().point_arv,trtNotCancelled,priorAirp);
              if (priorAirp.point_id!=NoExists)
              {
                TTripInfo inFlt;
                if (inFlt.getByPointId(priorAirp.point_id))
                {
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
                  TCkinQry.CreateVariable("point_id",otInteger,inboundGrp.get().point_arv);
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
      NewTextChild(dataNode, "col_excess_type", (int)excessNodeList.unitRequired());

    };
    TRptParams rpt_params(reqInfo->desk.lang);
    readTripCounters(point_id, rpt_params, dataNode, rtUnknown, "");

    variablesNode = GetNode("form_data/variables", resNode);
    if(variablesNode) SetProp(variablesNode, "update");
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
};

void BrdInterface::LoadPaxAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int pax_id=NodeAsInteger("pax_id",reqNode);
  int tid=NodeAsInteger("tid",reqNode);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.airp_arv, pax_grp.status, pax.crew_type "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id AND pax_grp.point_dep=:point_id AND pax.tid=:tid";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
  TPaxSegmentPair paxSegment(point_id, Qry.FieldAsString("airp_arv"));
  TPaxStatus grp_status=DecodePaxStatus(Qry.FieldAsString("status"));
  TCrewType::Enum crew_type = CrewTypes().decode(Qry.FieldAsString("crew_type"));
  ASTRA::TPaxTypeExt pax_ext(grp_status, crew_type);

  LoadAPIS(paxSegment, pax_ext, pax_id, resNode);
};

void BrdInterface::SavePaxAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int pax_id=NodeAsInteger("pax_id",reqNode);
  int tid=NodeAsInteger("tid",reqNode);
  xmlNodePtr apisNode=NodeAsNode("apis", reqNode);

  TFlights flightsForLock;
  flightsForLock.Get( point_id, ftTranzit );
  flightsForLock.Lock(__FUNCTION__);

  SaveAPIS(point_id, pax_id, tid, apisNode);

  GetPax(reqNode, resNode);
};

//////////// для системы информирования

void BrdInterface::OpenBrdInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  CheckInInterface::OpenDESKInfo( reqNode, resNode, "П" );
}





