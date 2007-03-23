#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "astra_utils.h"
#include "stages.h"
#include "tripinfo.h"
#include "etick.h"
#include "astra_ticket.h"

using namespace EXCEPTIONS;
using namespace std;

void BrdInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->screen.name == "BRDBUS.EXE")
    JxtContext::getJxtContHandler()->currContext()->write("TRIP_ID", point_id);
  else
    JxtContext::getJxtContHandler()->currContext()->write("EXAM_TRIP_ID", point_id);
  SetCounters(point_id,dataNode);
};

void BrdInterface::SetCounters( int point_id, xmlNodePtr dataNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

/*  int point_id;
  if (reqInfo->screen.name == "BRDBUS.EXE")
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
  else
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("EXAM_TRIP_ID");*/

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
            "    point_dep=:point_id AND "
            "    pr_brd IS NOT NULL ";
  Qry.SQLText = sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if(!Qry.Eof)
  {
      xmlNodePtr countersNode = GetNode("counters", dataNode);
      if (countersNode==NULL)
        countersNode=NewTextChild(dataNode, "counters");
      ReplaceTextChild(countersNode, "reg", Qry.FieldAsInteger("reg"));
      ReplaceTextChild(countersNode, "brd", Qry.FieldAsInteger("brd"));
  }
}


int get_new_tid(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if(Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
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
  {
    sql+=   "    UPDATE pax SET pr_brd=DECODE(:mark,0,1,0),tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_brd=:mark; "
            "    IF SQL%FOUND THEN "
            "      mvd.sync_pax(curRow.pax_id,:term); "
            "    END IF; ";
    Qry.CreateVariable( "term", otString, reqInfo->desk.code );
  }
  else
    sql+=   "    UPDATE pax SET pr_exam=DECODE(:mark,0,1,0),tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_exam=:mark; ";

  sql+=     "  END LOOP; "
            "END; ";
  Qry.SQLText=sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "mark", otInteger, (int)!boarding );
  Qry.Execute();
  char *msg;
  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    if (boarding)
      msg="Все пассажиры прошли посадку";
    else
      msg="Все пассажиры высажены";
  }
  else
  {
    if (boarding)
      msg="Все пассажиры прошли досмотр";
    else
      msg="Все пассажиры возвращены на досмотр";
  };
  reqInfo->MsgToLog(msg, evtPax, point_id);

  GetPax(reqNode,resNode);

 /* if (reqInfo->screen.name == "BRDBUS.EXE" &&
      ETCheckStatus(Ticketing::OrigOfRequest(*reqInfo),point_id,csaFlt))
    showProgError("Нет связи с сервером эл. билетов");
  else*/
    showMessage(msg);
};

void BrdInterface::Deplane(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id;
  if (reqInfo->screen.name == "BRDBUS.EXE")
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
  else
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("EXAM_TRIP_ID");

  get_new_tid(point_id);

  TQuery Qry(&OraSession);
  string sql=
            "DECLARE "
            "  CURSOR cur IS "
            "    SELECT pax_id FROM pax_grp,pax "
            "    WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_id AND ";

  if (reqInfo->screen.name == "BRDBUS.EXE")
    sql+=   "          pr_brd=1; ";
  else
    sql+=   "          pr_exam=1; ";

  sql+=     "curRow       cur%ROWTYPE; "
            "BEGIN "
            "  FOR curRow IN cur LOOP ";

  if (reqInfo->screen.name == "BRDBUS.EXE")
  {
    sql+=   "    UPDATE pax SET pr_brd=0,tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_brd=1; "
            "    IF SQL%FOUND THEN "
            "      mvd.sync_pax(curRow.pax_id,:term); "
            "    END IF; ";
    Qry.CreateVariable( "term", otString, reqInfo->desk.code );
  }
  else
    sql+=   "    UPDATE pax SET pr_exam=0,tid=tid__seq.currval "
            "    WHERE pax_id=curRow.pax_id AND pr_exam=1; ";

  sql+=     "  END LOOP; "
            "END; ";
  Qry.SQLText=sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if (reqInfo->screen.name == "BRDBUS.EXE")
    TReqInfo::Instance()->MsgToLog("Все пассажиры высажены", evtPax, point_id);
  else
    TReqInfo::Instance()->MsgToLog("Все пассажиры возвращены на досмотр", evtPax, point_id);
}

bool BrdInterface::PaxUpdate(int pax_id, int &tid, bool mark)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id;
  if (reqInfo->screen.name == "BRDBUS.EXE")
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
  else
    point_id = JxtContext::getJxtContHandler()->currContext()->readInt("EXAM_TRIP_ID");

  int new_tid = get_new_tid(point_id);

  TQuery Qry(&OraSession);
  if (reqInfo->screen.name == "BRDBUS.EXE")
    Qry.SQLText=
      "UPDATE pax SET pr_brd=:mark, tid=tid__seq.currval "
      "WHERE pax_id=:pax_id AND tid=:tid";
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
    if (reqInfo->screen.name == "BRDBUS.EXE")
    {
      Qry.Clear();
      Qry.SQLText=
        "BEGIN "
        "  mvd.sync_pax(:pax_id, :term); "
        "END;";
      Qry.CreateVariable("pax_id", otInteger, pax_id);
      Qry.CreateVariable("term", otString, reqInfo->desk.code);
      Qry.Execute();
    };
    Qry.Clear();
    Qry.SQLText=
      "SELECT surname,name,reg_no,grp_id FROM pax WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      string msg = (string)
                  "Пассажир " + Qry.FieldAsString("surname") + " " +
                  Qry.FieldAsString("name");
      if (reqInfo->screen.name == "BRDBUS.EXE")
        msg+=     (mark ? " прошел посадку" : " высажен");
      else
        msg+=     (mark ? " прошел досмотр" : " возвращен на досмотр");

      TReqInfo::Instance()->MsgToLog(msg, evtPax,
                                     point_id,
                                     Qry.FieldAsInteger("reg_no"),
                                     Qry.FieldAsInteger("grp_id"));
    };
    tid=new_tid;
    return true;
  }
  else return false;
}

void BrdInterface::PaxUpd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int pax_id = NodeAsInteger("pax_id", reqNode);
    int tid = NodeAsInteger("tid", reqNode);
    bool mark = NodeAsInteger("pr_brd", reqNode)!=0;
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    if (PaxUpdate(pax_id, tid, mark))
      NewTextChild(dataNode, "pr_brd", (int)mark);
    else
    {
      NewTextChild(dataNode, "pr_brd", -1);
      showErrorMessage("Изменения по пассажиру производились с другой стойки. Обновите данные");
    };
    NewTextChild(dataNode, "new_tid", tid);

    TReqInfo *reqInfo = TReqInfo::Instance();
    int point_id;
    if (reqInfo->screen.name == "BRDBUS.EXE")
      point_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
    else
      point_id = JxtContext::getJxtContHandler()->currContext()->readInt("EXAM_TRIP_ID");
    SetCounters(point_id,dataNode);
}

bool ChckSt(int pax_id, string seat_no)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT LPAD(seat_no,3,'0') AS seat_no1, LPAD(:seat_no,3,'0') AS seat_no2 FROM bp_print, "
        "  (SELECT MAX(time_print) AS time_print FROM bp_print WHERE pax_id=:pax_id) a "
        "WHERE pax_id=:pax_id AND bp_print.time_print=a.time_print ";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("seat_no", otString, seat_no);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      if (!Qry.FieldIsNULL("seat_no1") &&
          strcmp(Qry.FieldAsString("seat_no1"), Qry.FieldAsString("seat_no2"))!=0)
        return false;
    };
    return true;
}

void BrdInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetPax(reqNode,resNode);
};

void BrdInterface::GetPax(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    int point_id=NodeAsInteger("point_id",reqNode);
    int reg_no=-1;

    xmlNodePtr dataNode=GetNode("data",resNode);
    if (dataNode==NULL)
      dataNode = NewTextChild(resNode, "data");

    TQuery Qry(&OraSession);
    Qry.Clear();
    string condition;
    if(strcmp((char *)reqNode->name, "PaxByPaxId") == 0)
    {
      //получим point_id и reg_no
      Qry.Clear();
      Qry.SQLText=
        "SELECT point_dep,reg_no "
        "FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id AND pr_brd IS NOT NULL";
      Qry.CreateVariable("pax_id",otInteger,NodeAsInteger("pax_id",reqNode));
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("Пассажир не зарегистрирован");
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
        if (TripsInterface::readTripHeader( point_id, dataNode ))
        {
          readTripCounters( point_id, dataNode );
        }
        else
        {
          TQuery FltQry(&OraSession);
          FltQry.Clear();
          FltQry.SQLText=
            "SELECT airline,flt_no,suffix,airp,scd_out, "
            "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
            "FROM points WHERE point_id=:point_id";
          FltQry.CreateVariable("point_id",otInteger,point_id);
          FltQry.Execute();
          if (!FltQry.Eof)
          {
            TTripInfo info(FltQry);
            throw UserException("Пассажир с рейса " + GetTripName(info));
          }
          else
            throw UserException("Пассажир с другого рейса");
        };
      };
    }
    else if(strcmp((char *)reqNode->name, "PaxByRegNo") == 0)
    {
      reg_no=NodeAsInteger("reg_no",reqNode);
      condition+=" AND reg_no=:reg_no ";
      Qry.Clear();
      Qry.CreateVariable("reg_no",otInteger,reg_no);
    };

    condition+=" AND point_dep= :point_id AND pr_brd IS NOT NULL ";
    Qry.CreateVariable("point_id",otInteger,point_id);

    string sqlText = (string)
        "SELECT "
        "    pax_id, "
        "    point_dep AS point_id, "
        "    airp_dep, "
        "    pr_brd, "
        "    pr_exam, "
        "    doc_check, "
        "    reg_no, "
        "    surname, "
        "    name, "
        "    pers_type, "
        "    seat_no, "
        "    seats, "
        "    ticket_no, "
        "    coupon_no, "
        "    document, "
        "    pax.tid, "
        "    LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no_str, "
        "    ckin.get_remarks(pax_id,', ',0) AS remarks, "
        "    NVL(ckin.get_bagAmount(pax_grp.grp_id,NULL,rownum),0) AS bag_amount, "
        "    NVL(ckin.get_bagWeight(pax_grp.grp_id,NULL,rownum),0) AS bag_weight, "
        "    NVL(ckin.get_rkAmount(pax_grp.grp_id,NULL,rownum),0) AS rk_amount, "
        "    NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
        "    NVL(pax_grp.excess,0) AS excess, "
        "    NVL(value_bag.count,0) AS value_bag_count, "
        "    ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
        "    kassa.pr_payment(pax_grp.grp_id) AS pr_payment "
        "FROM "
        "    pax_grp, "
        "    pax, "
        "    ( "
        "     SELECT "
        "        grp_id, "
        "        COUNT(*) AS count "
        "     FROM "
        "        value_bag "
        "     GROUP BY "
        "        grp_id "
        "    ) value_bag "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax_grp.grp_id=value_bag.grp_id(+)  " +
        condition +
        " ORDER BY reg_no ";

    Qry.SQLText = sqlText;
    Qry.Execute();
    if (reg_no!=-1 && Qry.Eof)
      throw UserException("Пассажир не зарегистрирован");

    xmlNodePtr listNode = NewTextChild(dataNode, "passengers");
    for(;!Qry.Eof;Qry.Next())
    {
      int pax_id=Qry.FieldAsInteger("pax_id");

      xmlNodePtr paxNode = NewTextChild(listNode, "pax");
      NewTextChild(paxNode, "pax_id", pax_id);
      NewTextChild(paxNode, "pr_brd",  Qry.FieldAsInteger("pr_brd")!=0, false);
      NewTextChild(paxNode, "pr_exam", Qry.FieldAsInteger("pr_exam")!=0, false);
      NewTextChild(paxNode, "doc_check", Qry.FieldAsInteger("doc_check")!=0, false);
      NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger("reg_no"));
      NewTextChild(paxNode, "surname", Qry.FieldAsString("surname"));
      NewTextChild(paxNode, "name", Qry.FieldAsString("name"), "");
      NewTextChild(paxNode, "pers_type", Qry.FieldAsString("pers_type"), EncodePerson(adult));
      NewTextChild(paxNode, "seat_no", Qry.FieldAsString("seat_no_str"), "");
      NewTextChild(paxNode, "seats", Qry.FieldAsInteger("seats"), 1);
      NewTextChild(paxNode, "ticket_no", Qry.FieldAsString("ticket_no"), "");
      NewTextChild(paxNode, "coupon_no", Qry.FieldAsInteger("coupon_no"), 0);
      NewTextChild(paxNode, "document", Qry.FieldAsString("document"), "");
      NewTextChild(paxNode, "tid", Qry.FieldAsInteger("tid"));
      NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"), 0);
      NewTextChild(paxNode, "value_bag_count", Qry.FieldAsInteger("value_bag_count"), 0);
      NewTextChild(paxNode, "pr_payment", Qry.FieldAsInteger("pr_payment")!=0, false);
      NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"), 0);
      NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"), 0);
      NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger("rk_amount"), 0);
      NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"), 0);
      NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"), "");
      NewTextChild(paxNode, "remarks", Qry.FieldAsString("remarks"), "");

      if (reg_no==Qry.FieldAsInteger("reg_no"))
      {
        int mark;
        if (reqInfo->screen.name == "BRDBUS.EXE")
          mark=Qry.FieldAsInteger("pr_brd");
        else
          mark=Qry.FieldAsInteger("pr_exam");
        bool boarding = NodeAsInteger("boarding", reqNode)!=0;
        int tid;
        if (GetNode("tid",reqNode)!=NULL)
        {
          tid=NodeAsInteger("tid",reqNode);
          if (tid!=Qry.FieldAsInteger("tid"))
            throw UserException("Изменения по пассажиру производились с другой стойки. Обновите данные");
        }
        else
          tid=Qry.FieldAsInteger("tid");

        if(!boarding && !mark || boarding && mark)
        {
          if (reqInfo->screen.name == "BRDBUS.EXE")
          {
            if (mark)
              showErrorMessage("Пассажир уже прошел посадку");
            else
              showErrorMessage("Пассажир не прошел посадку");
          }
          else
          {
            if (mark)
              showErrorMessage("Пассажир уже прошел досмотр");
            else
              showErrorMessage("Пассажир не прошел досмотр");
          };
        }
        else
        {
          if (reqInfo->screen.name == "BRDBUS.EXE" &&
              boarding && Qry.FieldAsInteger("pr_exam")==0 &&
              strcmp(Qry.FieldAsString("airp_dep"),"СУР")==0)
          {
            showErrorMessage("Пассажир не прошел досмотр");
          }
          else
          {

            if(reqInfo->screen.name == "BRDBUS.EXE" &&
               boarding &&
               GetNode("confirmations/seat_no",reqNode)==NULL &&
               !ChckSt(pax_id, Qry.FieldAsString("seat_no")))
            {
              xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
              NewTextChild(confirmNode,"reset",true);
              NewTextChild(confirmNode,"type","seat_no");
              ostringstream msg;
              if (Qry.FieldIsNULL("seat_no"))
                msg << "Номер места пассажира в салоне не определен." << endl;
              else
                msg << "Номер места пассажира в салоне был изменен на "
                    << Qry.FieldAsString("seat_no") << "." << endl;

              msg << "Номер места, указанный в посадочном талоне, недействителен!" << endl
                  << "Продолжить операцию посадки?";
              NewTextChild(confirmNode,"message",msg.str());
            }
            else
            {
              if(reqInfo->screen.name != "BRDBUS.EXE" &&
                 !boarding &&
                 GetNode("confirmations/pr_brd",reqNode)==NULL &&
                 Qry.FieldAsInteger("pr_brd")!=0)
              {
                xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
                NewTextChild(confirmNode,"reset",true);
                NewTextChild(confirmNode,"type","pr_brd");
                ostringstream msg;
                msg << "Пассажир уже прошел посадку." << endl
                    << "Возвратить на досмотр?";
                NewTextChild(confirmNode,"message",msg.str());
              }
              else
                // update
                if (PaxUpdate(pax_id,tid,!mark))
                {
                  mark = !mark;
                  if (reqInfo->screen.name == "BRDBUS.EXE")
                    ReplaceTextChild(paxNode, "pr_brd", mark);
                  else
                    ReplaceTextChild(paxNode, "pr_exam", mark);
                  ReplaceTextChild(paxNode, "tid", tid);
                  ReplaceTextChild(dataNode,"updated");
                /*  if (reqInfo->screen.name == "BRDBUS.EXE" &&
                      ETCheckStatus(Ticketing::OrigOfRequest(*reqInfo),pax_id,csaPax))
                    showProgError("Нет связи с сервером эл. билетов");
                  else*/
                    if (reqInfo->screen.name == "BRDBUS.EXE")
                    {
                      if (mark)
                        showMessage("Пассажир прошел посадку");
                      else
                        showMessage("Пассажир высажен");
                    }
                    else
                    {
                      if (mark)
                        showMessage("Пассажир прошел досмотр");
                      else
                        showMessage("Пассажир возвращен на досмотр");
                    };

                }
                else
                  throw UserException("Изменения по пассажиру производились с другой стойки. Обновите данные");
            };
          };
        };
      };
    };

    SetCounters(point_id,dataNode);
};

void BrdInterface::BrdList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    int point_id;
    if (reqInfo->screen.name == "BRDBUS.EXE")
      point_id = JxtContext::getJxtContHandler()->currContext()->readInt("TRIP_ID");
    else
      point_id = JxtContext::getJxtContHandler()->currContext()->readInt("EXAM_TRIP_ID");

    bool pr_list;
    string condition;
    if(strcmp((char *)reqNode->name, "brd_list") == 0) {
        pr_list=true;
        if (reqInfo->screen.name == "BRDBUS.EXE")
          condition = " AND point_dep= :point_id AND pr_brd= :pr_brd ";
        else
          condition = " AND point_dep= :point_id AND pr_exam= :pr_brd ";
    } else if(strcmp((char *)reqNode->name, "search_reg") == 0) {
        pr_list=false;
        condition = " AND point_dep= :point_id AND reg_no= :reg_no AND pr_brd IS NOT NULL ";
    } else if(strcmp((char *)reqNode->name, "search_bar") == 0) {
        pr_list=false;
        condition = " AND pax_id= :pax_id AND pr_brd IS NOT NULL ";
    } else throw Exception("BrdInterface::BrdList: Unknown command tag %s", reqNode->name);

    string sqlText = (string)
        "SELECT "
        "    pax_id, "
        "    pax.grp_id, "
        "    point_dep AS point_id, "
        "    airp_dep, "
        "    pr_brd, "
        "    pr_exam, "
        "    doc_check, "
        "    reg_no, "
        "    surname, "
        "    name, "
        "    pers_type, "
        "    seat_no, "
        "    seats, "
        "    pax.tid, "
        "    LPAD(seat_no,3,'0')||DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no_str, "
        "    ckin.get_remarks(pax_id,', ',0) AS remarks, "
        "    NVL(ckin.get_rkAmount(pax_grp.grp_id,NULL,rownum),0) AS rk_amount, "
        "    NVL(ckin.get_rkWeight(pax_grp.grp_id,NULL,rownum),0) AS rk_weight, "
        "    NVL(pax_grp.excess,0) AS excess, "
        "    NVL(value_bag.count,0) AS value_bag_count, "
        "    ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
        "    kassa.pr_payment(pax_grp.grp_id) AS pr_payment  "
        "FROM "
        "    pax_grp, "
        "    pax, "
        "    ( "
        "     SELECT "
        "        grp_id, "
        "        COUNT(*) AS count "
        "     FROM "
        "        value_bag "
        "     GROUP BY "
        "        grp_id "
        "    ) value_bag "
        "WHERE "
        "    pax_grp.grp_id=pax.grp_id AND "
        "    pax_grp.grp_id=value_bag.grp_id(+)  " +
        condition +
        "ORDER BY reg_no ";
    TQuery Qry(&OraSession);
    Qry.SQLText = sqlText;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode)); //???
    SQLParams.setSQL(&Qry);
    Qry.Execute();
    if (!pr_list && Qry.Eof)
      throw UserException(1, "Пассажир не зарегистрирован");
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr listNode = NewTextChild(dataNode, "brd_list");
    for(;!Qry.Eof;Qry.Next())
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      int mark;
      if (reqInfo->screen.name == "BRDBUS.EXE")
        mark=Qry.FieldAsInteger("pr_brd");
      else
        mark=Qry.FieldAsInteger("pr_exam");
      int tid=Qry.FieldAsInteger("tid");
      if (!pr_list)
      {
        if (point_id!=Qry.FieldAsInteger("point_id"))
        {
          point_id=Qry.FieldAsInteger("point_id");
          //сначала попробуем
          if (strcmp(Qry.FieldAsString("airp_dep"),"СУР")==0 &&
              TripsInterface::readTripHeader( point_id, dataNode ))
          {
            readTripCounters( point_id, dataNode );
          }
          else
          {
            TQuery FltQry(&OraSession);
            FltQry.Clear();
            FltQry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out, "
              "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
              "FROM points WHERE point_id=:point_id";
            FltQry.CreateVariable("point_id",otInteger,point_id);
            FltQry.Execute();
            if (!FltQry.Eof)
            {
              TTripInfo info(FltQry);
              throw UserException(1, "Пассажир с рейса " + GetTripName(info));
            }
            else
              throw UserException(1, "Пассажир с другого рейса");
          };
        };
        bool boarding = NodeAsInteger("boarding", reqNode)!=0;
        if(!boarding && !mark || boarding && mark)
        {
          if (reqInfo->screen.name == "BRDBUS.EXE")
          {
            if (mark)
              throw UserException(1, "Пассажир уже прошел посадку");
            else
              throw UserException(1, "Пассажир не прошел посадку");
          }
          else
          {
            if (mark)
              throw UserException(1, "Пассажир уже прошел досмотр");
            else
              throw UserException(1, "Пассажир не прошел досмотр");
          };
        };

        if (reqInfo->screen.name == "BRDBUS.EXE" &&
            boarding && Qry.FieldAsInteger("pr_exam")==0 &&
            strcmp(Qry.FieldAsString("airp_dep"),"СУР")==0)
        {
          showErrorMessage("Пассажир не прошел досмотр");
        }
        else
        {
          if(reqInfo->screen.name == "BRDBUS.EXE" &&
             !mark &&
             !ChckSt(pax_id, Qry.FieldAsString("seat_no")))
          {
              NewTextChild(dataNode, "failed");
          }
          else
          {
              // update
              if (PaxUpdate(pax_id,tid,!mark))
              {
                mark = !mark;
                NewTextChild(dataNode, "pr_brd", (int)mark);
              }
              else
              {
                NewTextChild(dataNode, "pr_brd", -1); //???
                showErrorMessage("Изменения по пассажиру производились с другой стойки. Обновите данные");
              };
              NewTextChild(dataNode, "new_tid", tid);
          };
        };
      };

      xmlNodePtr paxNode = NewTextChild(listNode, "pax");
      NewTextChild(paxNode, "pax_id", pax_id);
      NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger("grp_id"));
      NewTextChild(paxNode, "point_id", Qry.FieldAsInteger("point_id"));
      if (reqInfo->screen.name == "BRDBUS.EXE")
      {
        NewTextChild(paxNode, "pr_brd",  mark);
        //удалить потом нафиг:
        NewTextChild(paxNode, "old_pr_brd", mark); //???
      }
      else
        NewTextChild(paxNode, "pr_brd",  Qry.FieldAsInteger("pr_brd"));
      NewTextChild(paxNode, "pr_exam", Qry.FieldAsInteger("pr_exam"));
      NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger("reg_no"));
      NewTextChild(paxNode, "surname", Qry.FieldAsString("surname"));
      NewTextChild(paxNode, "name", Qry.FieldAsString("name"));
      NewTextChild(paxNode, "pers_type", Qry.FieldAsString("pers_type"));
      NewTextChild(paxNode, "seats", Qry.FieldAsInteger("seats"));
      NewTextChild(paxNode, "doc_check", Qry.FieldAsInteger("doc_check"));
      NewTextChild(paxNode, "tid", tid);
      NewTextChild(paxNode, "seat_no", Qry.FieldAsString("seat_no"));
      NewTextChild(paxNode, "seat_no_str", Qry.FieldAsString("seat_no_str"));
      NewTextChild(paxNode, "remarks", Qry.FieldAsString("remarks"));
      NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger("rk_amount"));
      NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
      NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"));
      NewTextChild(paxNode, "value_bag_count", Qry.FieldAsInteger("value_bag_count"));
      NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
      NewTextChild(paxNode, "pr_payment", Qry.FieldAsInteger("pr_payment"));
      if (!pr_list) break;
    };

    SetCounters(point_id,dataNode);
};



