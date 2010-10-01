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

void BrdInterface::readTripCounters( int point_id, xmlNodePtr dataNode, bool used_for_web_rpt, string client_type )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery ClassesQry(&OraSession);
  ClassesQry.Clear();
  ClassesQry.SQLText=
    "SELECT trip_classes.class "
    "FROM trip_classes,classes "
    "WHERE trip_classes.class=classes.code AND "
    "      point_id=:point_id "
    "ORDER BY classes.priority";
  ClassesQry.CreateVariable( "point_id", otInteger, point_id );
  ClassesQry.Execute();

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
            "    pax "
            "WHERE "
            "    pax_grp.grp_id=pax.grp_id AND "
            "    point_dep=:point_id AND class=:class AND "
            "    pr_brd IS NOT NULL ";
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
  for(;!ClassesQry.Eof;ClassesQry.Next())
  {
    char* cl=ClassesQry.FieldAsString("class");
    Qry.SetVariable("class",cl);
    Qry.Execute();
    if(Qry.Eof) continue;
    int vreg = Qry.FieldAsInteger("reg");
    int vbrd = Qry.FieldAsInteger("brd");
    reg_str << cl << vreg << " ";
    brd_str << cl << vbrd << " ";
    reg+=vreg;
    brd+=vbrd;
    if(fr_reg_str.str().size())
        fr_reg_str << "/";
    fr_reg_str << cl << vreg;
    if(fr_brd_str.str().size())
        fr_brd_str << "/";
    fr_brd_str << cl << vbrd;
    if(fr_not_brd_str.str().size())
        fr_not_brd_str << "/";
    fr_not_brd_str << cl << vreg - vbrd;
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
          " nvl(sum(decode(pers_type, 'ВЗ', 1, 0)), 0) adl, "
          " nvl(sum(decode(pers_type, 'РБ', 1, 0)), 0) chd, "
          " nvl(sum(decode(pers_type, 'РМ', 1, 0)), 0) inf, ";
      if (reqInfo->screen.name == "BRDBUS.EXE")
          SQLText +=
              " NVL(SUM(DECODE(pr_brd,0,0,decode(pers_type, 'ВЗ', 1, 0))),0) AS brd_adl, "
              " NVL(SUM(DECODE(pr_brd,0,0,decode(pers_type, 'РБ', 1, 0))),0) AS brd_chd, "
              " NVL(SUM(DECODE(pr_brd,0,0,decode(pers_type, 'РМ', 1, 0))),0) AS brd_inf ";
      else
          SQLText +=
              " NVL(SUM(DECODE(pr_exam,0,0,decode(pers_type, 'ВЗ', 1, 0))),0) AS brd_adl, "
              " NVL(SUM(DECODE(pr_exam,0,0,decode(pers_type, 'РБ', 1, 0))),0) AS brd_chd, "
              " NVL(SUM(DECODE(pr_exam,0,0,decode(pers_type, 'РМ', 1, 0))),0) AS brd_inf ";
      SQLText +=
          "from "
          " pax_grp, "
          " pax "
          "where "
          " pax_grp.grp_id=pax.grp_id AND "
          " point_dep = :point_id and "
          " pr_brd is not null ";
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

          ReplaceTextChild(variablesNode, "total", pax_str.str() + "(" + fr_reg_str.str() + ")");
          ReplaceTextChild(variablesNode, "total_brd", brd_pax_str.str() + "(" + fr_brd_str.str() + ")");
          ReplaceTextChild(variablesNode, "total_not_brd", not_brd_pax_str.str() + "(" + fr_not_brd_str.str() + ")");
      }
  }
};

int get_new_tid(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT tid__seq.nextval AS tid FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  return Qry.FieldAsInteger("tid");
}

void BrdInterface::DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id=NodeAsInteger("point_id",reqNode);
  bool boarding = NodeAsInteger("boarding", reqNode)!=0;

  get_new_tid(point_id);

  TQuery Qry(&OraSession);
  string sql=
            "DECLARE "
            "  CURSOR cur IS "
            "    SELECT pax_id FROM pax_grp,pax "
            "    WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND ";

  if (reqInfo->screen.name == "BRDBUS.EXE")
    sql+=   "          pr_brd=:mark; ";
  else
    sql+=   "          pr_exam=:mark; ";

  sql+=     "curRow       cur%ROWTYPE; "
            "BEGIN "
            "  FOR curRow IN cur LOOP ";

  if (reqInfo->screen.name == "BRDBUS.EXE")
    sql+=   "    UPDATE pax SET pr_brd=DECODE(:mark,0,1,0),tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_brd=:mark; ";
  else
    sql+=   "    UPDATE pax SET pr_exam=DECODE(:mark,0,1,0),tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_exam=:mark; ";

  sql+=     "    IF SQL%FOUND THEN "
            "      mvd.sync_pax(curRow.pax_id,:term); "
            "    END IF; "
            "  END LOOP; "
            "END; ";
  Qry.SQLText=sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "mark", otInteger, (int)!boarding );
  Qry.CreateVariable( "term", otString, reqInfo->desk.code );
  Qry.Execute();
  string msg;
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    if (boarding)
      msg=getLocaleText("MSG.PASSENGER.ALL_BOARDED");
    else
      msg=getLocaleText("MSG.PASSENGER.ALL_NOT_BOARDED");
  }
  else
  {
    if (boarding)
      msg=getLocaleText("MSG.PASSENGER.ALL_EXAMED");
    else
      msg=getLocaleText("MSG.PASSENGER.ALL_NOT_EXAMED");
  };
  reqInfo->MsgToLog(msg, evtPax, point_id);

  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    check_brd_alarm( point_id );
  };

  GetPax(reqNode,resNode,false);

  ASTRA::showMessage(msg);
};

bool BrdInterface::PaxUpdate(int point_id, int pax_id, int &tid, bool mark, bool pr_exam_with_brd)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  //лочим рейс
  int new_tid = get_new_tid(point_id);

  TQuery Qry(&OraSession);
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    Qry.SQLText=
      "UPDATE pax "
      "SET pr_brd=:mark, "
      "    pr_exam=DECODE(:pr_exam_with_brd,0,pr_exam,:mark), "
      "    tid=tid__seq.currval "
      "WHERE pax_id=:pax_id AND tid=:tid";
    Qry.CreateVariable("pr_exam_with_brd",otInteger,(int)pr_exam_with_brd);
  }
  else
    Qry.SQLText=
      "UPDATE pax SET pr_exam=:mark, tid=tid__seq.currval "
      "WHERE pax_id=:pax_id AND tid=:tid";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.CreateVariable("mark", otInteger, (int)mark);
  Qry.Execute();
  if (Qry.RowsProcessed()>0)
  {
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  mvd.sync_pax(:pax_id, :term); "
      "END;";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("term", otString, reqInfo->desk.code);
    Qry.Execute();

    Qry.Clear();
    Qry.SQLText=
      "SELECT surname,name,reg_no,grp_id FROM pax WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      int grp_id=Qry.FieldAsInteger("grp_id");

      string msg = (string)
                  "Пассажир " + Qry.FieldAsString("surname") + " " +
                  Qry.FieldAsString("name");
      if (reqInfo->screen.name == "BRDBUS.EXE")
      {
        if (pr_exam_with_brd)
          msg+=     (mark ? " прошел досмотр," : " возвращен на досмотр,");
        msg+=     (mark ? " прошел посадку" : " высажен");
      }
      else
        msg+=     (mark ? " прошел досмотр" : " возвращен на досмотр");

      TReqInfo::Instance()->MsgToLog(msg, evtPax,
                                     point_id,
                                     Qry.FieldAsInteger("reg_no"),
                                     grp_id);

      //отвяжем сквозняков от предыдущих сегментов
      int tckin_id;
      int tckin_seg_no;
      SeparateTCkin(grp_id,cssAllPrevCurr,cssCurr,new_tid,tckin_id,tckin_seg_no);

      if (reqInfo->screen.name == "BRDBUS.EXE")
      {
        check_brd_alarm( point_id );
      };
    };
    tid=new_tid;
    return true;
  }
  else return false;
}

bool ChckSt(int pax_id, string& curr_seat_no)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT curr_seat_no,bp_seat_no, "
      "       TRANSLATE(UPPER(curr_seat_no),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS curr_seat_no2, "
      "       TRANSLATE(UPPER(bp_seat_no),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS bp_seat_no2 "
      "FROM "
      " (SELECT salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'list') AS curr_seat_no, "
      "         bp_print.seat_no AS bp_seat_no "
      "  FROM bp_print,pax, "
      "       (SELECT MAX(time_print) AS time_print FROM bp_print WHERE pax_id=:pax_id) a "
      "  WHERE bp_print.time_print=a.time_print AND bp_print.pax_id=:pax_id AND "
      "        pax.pax_id=:pax_id)";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      curr_seat_no=Qry.FieldAsString("curr_seat_no");
      if (!Qry.FieldIsNULL("bp_seat_no") &&
          strcmp(Qry.FieldAsString("curr_seat_no2"), Qry.FieldAsString("bp_seat_no2"))!=0)
        return false;
    };
    return true;
}

void BrdInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetPax(reqNode,resNode,false);
};

void BrdInterface::GetPax(xmlNodePtr reqNode, xmlNodePtr resNode, bool used_for_web_rpt)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    if(strcmp((char *)reqNode->name, "PaxByScanData") == 0)
      throw AstraLocale::UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");


    int point_id=NodeAsInteger("point_id",reqNode);
    int reg_no=-1;
    int hall=-1;

    if ( GetNode( "LoadForm", reqNode ) )
        get_report_form("ExamBrdbus", resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    if ( GetNode( "LoadVars", reqNode ) ) {
        PaxListVars(point_id, 0, variablesNode);
        STAT::set_variables(resNode);
        NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT"));
        NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
    }
    string client_type;

    if (used_for_web_rpt)
      client_type = NodeAsString( "client_type", reqNode );

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

    TQuery Qry(&OraSession);
    Qry.Clear();
    string condition;
    if(strcmp((char *)reqNode->name, "PaxByPaxId") == 0)
    {
      if (!NodeIsNULL("hall",reqNode))
        hall=NodeAsInteger("hall",reqNode);
      //получим point_id и reg_no
      Qry.Clear();
      Qry.SQLText=
        "SELECT point_dep,reg_no "
        "FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id AND pr_brd IS NOT NULL";
      Qry.CreateVariable("pax_id",otInteger,NodeAsInteger("pax_id",reqNode));
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
      reg_no=Qry.FieldAsInteger("reg_no");
      if (point_id==Qry.FieldAsInteger("point_dep"))
      {
        condition+=" AND reg_no=:reg_no ";
        Qry.Clear();
        Qry.CreateVariable("reg_no",otInteger,reg_no);
      }
      else
      {
        point_id=Qry.FieldAsInteger("point_dep");
        Qry.Clear();
        if (reqInfo->screen.name != "BRDBUS.EXE" &&
            TripsInterface::readTripHeader( point_id, dataNode ))
        {
          //проверим hall
          if (hall!=-1)
          {
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
                       HallQry.FieldAsString("flt_airp"))!=0) hall=-1;
          };

          readTripCounters( point_id, dataNode, false, "" );
          readTripData( point_id, dataNode );
        }
        else
        {
          FltQry.SetVariable("point_id",point_id);
          FltQry.Execute();
          if (!FltQry.Eof)
          {
            TTripInfo info(FltQry);
            throw AstraLocale::UserException(100, "MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info)));
          }
          else
            throw AstraLocale::UserException(100, "MSG.PASSENGER.OTHER_FLIGHT");
        };
      };
    }
    else if(strcmp((char *)reqNode->name, "PaxByRegNo") == 0)
    {
      if (!NodeIsNULL("hall",reqNode))
        hall=NodeAsInteger("hall",reqNode);
      reg_no=NodeAsInteger("reg_no",reqNode);
      condition+=" AND reg_no=:reg_no ";
      Qry.Clear();
      Qry.CreateVariable("reg_no",otInteger,reg_no);
    };

    ostringstream sql;
    sql << "SELECT "
           "    pax_id, "
           "    pax_grp.grp_id, "
           "    pr_brd, "
           "    pr_exam, "
           "    reg_no, "
           "    surname, "
           "    pax.name, "
           "    pers_type, "
           "    class, "
           "    pax_grp.status, "
           "    NVL(report.get_last_trfer_airp(pax_grp.grp_id),pax_grp.airp_arv) AS airp_arv, "
           "    salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no, "
           "    seats, "
           "    wl_type, "
           "    ticket_no, "
           "    coupon_no, "
           "    document, "
           "    pax.tid, "
           "    ckin.get_remarks(pax_id,', ',0) AS remarks, "
           "    NVL(ckin.get_bagAmount(pax_grp.grp_id,NULL,rownum),0) AS bag_amount, "
           "    NVL(ckin.get_bagWeight(pax_grp.grp_id,NULL,rownum),0) AS bag_weight, "
           "    NVL(ckin.get_rkAmount(pax_grp.grp_id,NULL,rownum),0) AS rk_amount, "
           "    NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
           "    NVL(pax_grp.excess,0) AS excess, "
           "    kassa.get_value_bag_count(pax_grp.grp_id) AS value_bag_count, "
           "    ckin.get_birks(pax_grp.grp_id,NULL,:lang) AS tags, "
           "    kassa.pr_payment(pax_grp.grp_id) AS pr_payment, "
           "    client_type, "
           "    tckin_id, seg_no ";

    if (used_for_web_rpt)
      sql << ", users2.descr AS user_descr ";

    sql << "FROM "
           "    pax_grp, "
           "    pax, "
           "    tckin_pax_grp ";

    if (used_for_web_rpt)
      sql << ", users2 ";

    sql << "WHERE "
           "    pax_grp.grp_id=pax.grp_id AND "
           "    pax_grp.grp_id=tckin_pax_grp.grp_id(+) AND ";

    if (used_for_web_rpt)
      sql << "  pax_grp.user_id=users2.user_id AND ";

    sql << "    point_dep= :point_id AND pr_brd IS NOT NULL ";

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
    sql << condition;
    sql << " ORDER BY reg_no ";

    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("lang",otString,reqInfo->desk.lang);
    Qry.SQLText = sql.str().c_str();
    Qry.Execute();
    if (reg_no!=-1 && Qry.Eof)
      throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");

    TQuery TCkinQry(&OraSession);

    xmlNodePtr listNode = NewTextChild(dataNode, "passengers");
    if (!Qry.Eof)
    {
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

      int col_pax_id=Qry.FieldIndex("pax_id");
      int col_grp_id=Qry.FieldIndex("grp_id");
      int col_pr_brd=Qry.FieldIndex("pr_brd");
      int col_pr_exam=Qry.FieldIndex("pr_exam");
      int col_reg_no=Qry.FieldIndex("reg_no");
      int col_surname=Qry.FieldIndex("surname");
      int col_name=Qry.FieldIndex("name");
      int col_pers_type=Qry.FieldIndex("pers_type");
      int col_class=Qry.FieldIndex("class");
      int col_status=Qry.FieldIndex("status");
      int col_airp_arv=Qry.FieldIndex("airp_arv");
      int col_seat_no=Qry.FieldIndex("seat_no");
      int col_seats=Qry.FieldIndex("seats");
      int col_wl_type=Qry.FieldIndex("wl_type");
      int col_ticket_no=Qry.FieldIndex("ticket_no");
      int col_coupon_no=Qry.FieldIndex("coupon_no");
      int col_document=Qry.FieldIndex("document");
      int col_tid=Qry.FieldIndex("tid");
      int col_remarks=Qry.FieldIndex("remarks");
      int col_bag_amount=Qry.FieldIndex("bag_amount");
      int col_bag_weight=Qry.FieldIndex("bag_weight");
      int col_rk_amount=Qry.FieldIndex("rk_amount");
      int col_rk_weight=Qry.FieldIndex("rk_weight");
      int col_excess=Qry.FieldIndex("excess");
      int col_value_bag_count=Qry.FieldIndex("value_bag_count");
      int col_tags=Qry.FieldIndex("tags");
      int col_pr_payment=Qry.FieldIndex("pr_payment");
      int col_client_type=Qry.FieldIndex("client_type");
      int col_tckin_id=Qry.FieldIndex("tckin_id");
      int col_seg_no=Qry.FieldIndex("seg_no");
      int col_user_descr=NoExists;
      if (used_for_web_rpt) col_user_descr=Qry.FieldIndex("user_descr");


      TPaxSeats priorSeats(point_id);
      for(;!Qry.Eof;Qry.Next())
      {
          int pax_id=Qry.FieldAsInteger(col_pax_id);

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
          NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
          NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
          NewTextChild(paxNode, "seats", Qry.FieldAsInteger(col_seats), 1);
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
            NewTextChild(paxNode,"seat_no_str","ЛО");
            NewTextChild(paxNode,"seat_no_alarm",(int)true);
          };
          NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no), "");
          NewTextChild(paxNode, "coupon_no", Qry.FieldAsInteger(col_coupon_no), 0);
          NewTextChild(paxNode, "document", Qry.FieldAsString(col_document), "");
          NewTextChild(paxNode, "tid", Qry.FieldAsInteger(col_tid));
          NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess), 0);
          NewTextChild(paxNode, "value_bag_count", Qry.FieldAsInteger(col_value_bag_count), 0);
          NewTextChild(paxNode, "pr_payment", Qry.FieldAsInteger(col_pr_payment)!=0, false);
          NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount), 0);
          NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight), 0);
          NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger(col_rk_amount), 0);
          NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight), 0);
          NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags), "");
          NewTextChild(paxNode, "remarks", Qry.FieldAsString(col_remarks), "");
          if (DecodeClientType(Qry.FieldAsString(col_client_type))!=ctTerm)
            NewTextChild(paxNode, "client_name", ElemIdToNameShort(etClientType, Qry.FieldAsString(col_client_type)));

          if (used_for_web_rpt)
            NewTextChild(paxNode, "user_descr", Qry.FieldAsString(col_user_descr)); //login не надо использовать, так как он м.б. пустым


          if (!Qry.FieldIsNULL(col_tckin_id))
          {
            TCkinQry.Clear();
            TCkinQry.SQLText=
              "SELECT pax_grp.point_arv, "
              "       points.airline, "
              "       points.flt_no, "
              "       points.suffix, "
              "       points.airp, "
              "       points.scd_out "
              "FROM points,pax_grp,tckin_pax_grp, "
              "     (SELECT MAX(seg_no) AS seg_no FROM tckin_pax_grp "
              "      WHERE tckin_id=:tckin_id AND seg_no<:seg_no) a "
              "WHERE points.point_id=pax_grp.point_dep AND "
              "      pax_grp.grp_id=tckin_pax_grp.grp_id AND "
              "      tckin_pax_grp.tckin_id=:tckin_id AND "
              "      tckin_pax_grp.seg_no=a.seg_no";
            TCkinQry.CreateVariable("tckin_id",otInteger,Qry.FieldAsInteger(col_tckin_id));
            TCkinQry.CreateVariable("seg_no",otInteger,Qry.FieldAsInteger(col_seg_no));
            TCkinQry.Execute();
            if (!TCkinQry.Eof)
            {
              TTripInfo info(TCkinQry);

              TDateTime scd_out_local = UTCToLocal(info.scd_out,AirpTZRegion(info.airp));

              ostringstream trip;
              trip << ElemIdToCodeNative(etAirline, info.airline)
                   << setw(3) << setfill('0') << info.flt_no
                   << ElemIdToCodeNative(etSuffix, info.suffix)
                   << '/' << DateTimeToStr(scd_out_local,"dd");

              NewTextChild(paxNode, "inbound_flt", trip.str());

              int point_arv=TCkinQry.FieldAsInteger("point_arv");
              TCkinQry.Clear();
              TCkinQry.SQLText=
                "SELECT scd_in, NVL(act_in,NVL(est_in,scd_in)) AS real_in "
                "FROM points WHERE point_id=:point_id";
              TCkinQry.CreateVariable("point_id",otInteger,point_arv);
              TCkinQry.Execute();
              if (!TCkinQry.Eof && !TCkinQry.FieldIsNULL("scd_in") && !TCkinQry.FieldIsNULL("real_in"))
              {
                if (TCkinQry.FieldAsDateTime("real_in")-TCkinQry.FieldAsDateTime("scd_in") >= 20.0/1440)  //задержка более 20 мин
                  NewTextChild(paxNode, "inbound_delay_alarm", (int)true);
              };
            };
          };

          if (reg_no==Qry.FieldAsInteger(col_reg_no))
          {
              int mark;
              if (reqInfo->screen.name == "BRDBUS.EXE")
                  mark=Qry.FieldAsInteger(col_pr_brd);
              else
                  mark=Qry.FieldAsInteger(col_pr_exam);
              bool boarding = NodeAsInteger("boarding", reqNode)!=0;
              int tid;
              if (GetNode("tid",reqNode)!=NULL)
              {
                  tid=NodeAsInteger("tid",reqNode);
                  if (tid!=Qry.FieldAsInteger(col_tid))
                      throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA", AstraLocale::LParams() << AstraLocale::LParam("surname", Qry.FieldAsString(col_surname)));
              }
              else
                  tid=Qry.FieldAsInteger(col_tid);

              if(!boarding && !mark || boarding && mark)
              {
                  if (reqInfo->screen.name == "BRDBUS.EXE")
                  {
                      if (mark)
                          AstraLocale::showErrorMessage("MSG.PASSENGER.BOARDED_ALREADY",120);
                      else
                          AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BOARDING",120);
                  }
                  else
                  {
                      if (mark)
                          AstraLocale::showErrorMessage("MSG.PASSENGER.EXAMED_ALREADY",120);
                      else
                          AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM",120);
                  };
              }
              else
              {
                  if (hall==-1)
                  {
                      if(reqInfo->screen.name == "BRDBUS.EXE")
                          AstraLocale::showErrorMessage("MSG.NOT_SET_BOARDING_HALL");
                      else
                          AstraLocale::showErrorMessage("MSG.NOT_SET_EXAM_HALL");
                      continue;
                  };

                  bool pr_exam_with_brd=false;
                  bool pr_exam=false;
                  bool pr_check_pay=false;
                  int pr_etstatus=0;
                  TQuery SetsQry(&OraSession);
                  if (reqInfo->screen.name == "BRDBUS.EXE")
                  {
                      SetsQry.Clear();
                      SetsQry.SQLText=
                          "SELECT pr_misc FROM trip_hall "
                          "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
                          "ORDER BY DECODE(hall,NULL,1,0)";
                      SetsQry.CreateVariable("point_id",otInteger,point_id);
                      SetsQry.CreateVariable("hall",otInteger,hall);
                      SetsQry.CreateVariable("type",otInteger,2);
                      SetsQry.Execute();
                      if (!SetsQry.Eof) pr_exam_with_brd=SetsQry.FieldAsInteger("pr_misc")!=0;
                  };

                  SetsQry.Clear();
                  SetsQry.SQLText=
                      "SELECT pr_exam,pr_check_pay,pr_exam_check_pay,pr_etstatus "
                      "FROM trip_sets WHERE point_id=:point_id";
                  SetsQry.CreateVariable("point_id",otInteger,point_id);
                  SetsQry.Execute();
                  if (SetsQry.Eof)
                    throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

                  pr_exam=SetsQry.FieldAsInteger("pr_exam")!=0;
                  if (reqInfo->screen.name == "BRDBUS.EXE")
                    pr_check_pay=SetsQry.FieldAsInteger("pr_check_pay")!=0;
                  else
                    pr_check_pay=SetsQry.FieldAsInteger("pr_exam_check_pay")!=0;
                  pr_etstatus=SetsQry.FieldAsInteger("pr_etstatus");

                  if (boarding && !Qry.FieldIsNULL(col_wl_type))
                  {
                      AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_CONFIRM_FOROM_WAIT_LIST");
                  }
                  else if (reqInfo->screen.name == "BRDBUS.EXE" &&
                           boarding && Qry.FieldAsInteger(col_pr_exam)==0 &&
                           !pr_exam_with_brd && pr_exam)
                  {
                      AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_EXAM");
                  }
                  else if
                      (boarding && Qry.FieldAsInteger(col_pr_payment)==0 &&
                       pr_check_pay)
                      {
                          AstraLocale::showErrorMessage("MSG.PASSENGER.NOT_BAG_PAID");
                      }
                  else
                  {
                      string curr_seat_no;
                      if(reqInfo->screen.name == "BRDBUS.EXE" &&
                              boarding &&
                              GetNode("confirmations/seat_no",reqNode)==NULL &&
                              !ChckSt(pax_id, curr_seat_no))
                      {
                          xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                          NewTextChild(confirmNode,"reset",true);
                          NewTextChild(confirmNode,"type","seat_no");
                          ostringstream msg;
                          if (curr_seat_no.empty())
                              msg << AstraLocale::getLocaleText("MSG.PASSENGER.SALON_SEAT_NO_NOT_DEFINED") << endl;
                          else
                              msg << AstraLocale::getLocaleText("MSG.PASSENGER.SALON_SEAT_NO_CHANGED_TO", AstraLocale::LParams() << LParam("seat_no", curr_seat_no)) << endl;

                          msg << getLocaleText("MSG.PASSENGER.INVALID_BP_SEAT_NO") << endl
                              << getLocaleText("QST.CONTINUE_BRD");
                          NewTextChild(confirmNode,"message",msg.str());
                      }
                      else
                      {
                          if(reqInfo->screen.name != "BRDBUS.EXE" &&
                                  !boarding &&
                                  GetNode("confirmations/pr_brd",reqNode)==NULL &&
                                  Qry.FieldAsInteger(col_pr_brd)!=0)
                          {
                              xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                              NewTextChild(confirmNode,"reset",true);
                              NewTextChild(confirmNode,"type","pr_brd");
                              ostringstream msg;
                              msg << getLocaleText("MSG.PASSENGER.BOARDED_ALREADY") << endl
                                  << getLocaleText("QST.PASSENGER.RETURN_FOR_EXAM");
                              NewTextChild(confirmNode,"message",msg.str());
                          }
                          else
                              // update
                              if (PaxUpdate(point_id,pax_id,tid,!mark,pr_exam_with_brd))
                              {
                                  mark = !mark;
                                  if (reqInfo->screen.name == "BRDBUS.EXE")
                                  {
                                      ReplaceTextChild(paxNode, "pr_brd", mark);
                                      if (pr_exam_with_brd)
                                          ReplaceTextChild(paxNode, "pr_exam", mark);
                                  }
                                  else
                                      ReplaceTextChild(paxNode, "pr_exam", mark);
                                  ReplaceTextChild(paxNode, "tid", tid);
                                  ReplaceTextChild(dataNode,"updated",pax_id);
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

                                  if (reqInfo->screen.name == "BRDBUS.EXE")
                                  {
                                      if (mark)
                                      {
                                        if (DecodePaxStatus(Qry.FieldAsString(col_status))==psTCheckin)
                                          AstraLocale::showErrorMessage("MSG.ATTENTION_TCKIN_GET_COUPON");
                                        else
                                          AstraLocale::showMessage("MSG.PASSENGER.BOARDING");
                                      }
                                      else
                                          AstraLocale::showMessage("MSG.PASSENGER.DEBARKED");
                                  }
                                  else
                                  {
                                      if (mark)
                                          AstraLocale::showMessage("MSG.PASSENGER.EXAM");
                                      else
                                          AstraLocale::showMessage("MSG.PASSENGER.RETURNED_EXAM");
                                  };

                              }
                              else
                                  throw AstraLocale::UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA", LParams() << LParam("surname", Qry.FieldAsString(col_surname)));
                      };
                  };
              };
          };
      };//for(;!Qry.Eof;Qry.Next())
    };
    readTripCounters(point_id, dataNode, used_for_web_rpt, client_type);
};





