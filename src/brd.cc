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

void BrdInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode,node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airp FROM points WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");
  string airp_dep=Qry.FieldAsString("airp");

  Qry.Clear();
  Qry.SQLText =
    "SELECT id,name FROM halls2 WHERE airp=:airp_dep";
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "halls" );
  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "hall" );
    NewTextChild( itemNode, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "name" ) );
  };
}

void BrdInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
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
  };
};

int get_new_tid(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if(Qry.Eof) throw UserException("���� �� ������. ������� �����");
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
      msg="�� ���ᠦ��� ��諨 ��ᠤ��";
    else
      msg="�� ���ᠦ��� ��ᠦ���";
  }
  else
  {
    if (boarding)
      msg="�� ���ᠦ��� ��諨 ��ᬮ��";
    else
      msg="�� ���ᠦ��� �����饭� �� ��ᬮ��";
  };
  reqInfo->MsgToLog(msg, evtPax, point_id);

  GetPax(reqNode,resNode);

 /* if (reqInfo->screen.name == "BRDBUS.EXE" &&
      ETCheckStatus(Ticketing::OrigOfRequest(*reqInfo),point_id,csaFlt))
    showProgError("��� �裡 � �ࢥ஬ �. ����⮢");
  else*/
    showMessage(msg);
};

bool BrdInterface::PaxUpdate(int point_id, int pax_id, int &tid, bool mark, bool pr_exam_with_brd)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  //��稬 ३�
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
                  "���ᠦ�� " + Qry.FieldAsString("surname") + " " +
                  Qry.FieldAsString("name");
      if (reqInfo->screen.name == "BRDBUS.EXE")
      {
        if (pr_exam_with_brd)
          msg+=     (mark ? " ��襫 ��ᬮ��," : " �����饭 �� ��ᬮ��,");
        msg+=     (mark ? " ��襫 ��ᠤ��" : " ��ᠦ��");
      }
      else
        msg+=     (mark ? " ��襫 ��ᬮ��" : " �����饭 �� ��ᬮ��");

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
    int hall=-1;

    xmlNodePtr dataNode=GetNode("data",resNode);
    if (dataNode==NULL)
      dataNode = NewTextChild(resNode, "data");

    TQuery Qry(&OraSession);
    Qry.Clear();
    string condition;
    if(strcmp((char *)reqNode->name, "PaxByPaxId") == 0)
    {
      hall=NodeAsInteger("hall",reqNode);
      //����稬 point_id � reg_no
      Qry.Clear();
      Qry.SQLText=
        "SELECT point_dep,reg_no "
        "FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id AND pr_brd IS NOT NULL";
      Qry.CreateVariable("pax_id",otInteger,NodeAsInteger("pax_id",reqNode));
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("���ᠦ�� �� ��ॣ����஢��");
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
            throw UserException("���ᠦ�� � ३� " + GetTripName(info));
          }
          else
            throw UserException("���ᠦ�� � ��㣮�� ३�");
        };
      };
    }
    else if(strcmp((char *)reqNode->name, "PaxByRegNo") == 0)
    {
      hall=NodeAsInteger("hall",reqNode);
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
      throw UserException("���ᠦ�� �� ��ॣ����஢��");

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
            throw UserException("��������� �� ���ᠦ��� �ந��������� � ��㣮� �⮩��. ������� �����");
        }
        else
          tid=Qry.FieldAsInteger("tid");

        if(!boarding && !mark || boarding && mark)
        {
          if (reqInfo->screen.name == "BRDBUS.EXE")
          {
            if (mark)
              showErrorMessage("���ᠦ�� 㦥 ��襫 ��ᠤ��");
            else
              showErrorMessage("���ᠦ�� �� ��襫 ��ᠤ��");
          }
          else
          {
            if (mark)
              showErrorMessage("���ᠦ�� 㦥 ��襫 ��ᬮ��");
            else
              showErrorMessage("���ᠦ�� �� ��襫 ��ᬮ��");
          };
        }
        else
        {
          bool pr_exam_with_brd=false;
          bool pr_exam=false;
          bool pr_check_pay=false;
          if (reqInfo->screen.name == "BRDBUS.EXE")
          {
            TQuery SetsQry(&OraSession);
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

            SetsQry.Clear();
            SetsQry.SQLText=
              "SELECT pr_exam,pr_check_pay FROM trip_sets WHERE point_id=:point_id";
            SetsQry.CreateVariable("point_id",otInteger,point_id);
            SetsQry.Execute();
            if (!SetsQry.Eof)
            {
              pr_exam=SetsQry.FieldAsInteger("pr_exam")!=0;
              pr_check_pay=SetsQry.FieldAsInteger("pr_check_pay")!=0;
            };
          };


          if (reqInfo->screen.name == "BRDBUS.EXE" &&
              boarding && Qry.FieldAsInteger("pr_exam")==0 &&
              !pr_exam_with_brd && pr_exam)
          {
            showErrorMessage("���ᠦ�� �� ��襫 ��ᬮ��");
          }
          else if
             (reqInfo->screen.name == "BRDBUS.EXE" &&
              boarding && Qry.FieldAsInteger("pr_payment")==0 &&
              pr_check_pay)
          {
            showErrorMessage("���ᠦ�� �� ����⨫ �����");
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
                msg << "����� ���� ���ᠦ�� � ᠫ��� �� ��।����." << endl;
              else
                msg << "����� ���� ���ᠦ�� � ᠫ��� �� ������� �� "
                    << Qry.FieldAsString("seat_no") << "." << endl;

              msg << "����� ����, 㪠����� � ��ᠤ�筮� ⠫���, ������⢨⥫��!" << endl
                  << "�த������ ������ ��ᠤ��?";
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
                msg << "���ᠦ�� 㦥 ��襫 ��ᠤ��." << endl
                    << "�������� �� ��ᬮ��?";
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
                  ReplaceTextChild(dataNode,"updated");
                /*  if (reqInfo->screen.name == "BRDBUS.EXE" &&
                      ETCheckStatus(Ticketing::OrigOfRequest(*reqInfo),pax_id,csaPax))
                    showProgError("��� �裡 � �ࢥ஬ �. ����⮢");
                  else*/
                    if (reqInfo->screen.name == "BRDBUS.EXE")
                    {
                      if (mark)
                        showMessage("���ᠦ�� ��襫 ��ᠤ��");
                      else
                        showMessage("���ᠦ�� ��ᠦ��");
                    }
                    else
                    {
                      if (mark)
                        showMessage("���ᠦ�� ��襫 ��ᬮ��");
                      else
                        showMessage("���ᠦ�� �����饭 �� ��ᬮ��");
                    };

                }
                else
                  throw UserException("��������� �� ���ᠦ��� �ந��������� � ��㣮� �⮩��. ������� �����");
            };
          };
        };
      };
    };

    readTripCounters(point_id,dataNode);
};





