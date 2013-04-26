#include "payment.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "print.h"
#include "tripinfo.h"
#include "oralib.h"
#include "astra_service.h"
#include "term_version.h"
#include "astra_misc.h"
#include "checkin.h"
#include "baggage.h"
#include "passenger.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

void PaymentInterface::BuildTransfer(const TTrferRoute &trfer, xmlNodePtr transferNode)
{
  //��饭��� ��楤�� �� �᭮�� CheckInInterface::BuildTransfer
  if (transferNode==NULL) return;

  xmlNodePtr node=NewTextChild(transferNode,"transfer");

  int iDay,iMonth,iYear;
  for(TTrferRoute::const_iterator t=trfer.begin();t!=trfer.end();t++)
  {
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"airline",t->operFlt.airline);
    NewTextChild(trferNode,"aircode",
                 base_tables.get("airlines").get_row("code",t->operFlt.airline).AsString("aircode"));
    
    NewTextChild(trferNode,"flt_no",t->operFlt.flt_no);
    NewTextChild(trferNode,"suffix",t->operFlt.suffix);
    
    //���
    DecodeDate(t->operFlt.scd_out,iYear,iMonth,iDay);
    NewTextChild(trferNode,"local_date",iDay);
    
    NewTextChild(trferNode,"airp_dep",t->operFlt.airp);
    NewTextChild(trferNode,"airp_arv",t->airp_arv);

    NewTextChild(trferNode,"city_dep",
                 base_tables.get("airps").get_row("code",t->operFlt.airp).AsString("city"));
    NewTextChild(trferNode,"city_arv",
                 base_tables.get("airps").get_row("code",t->airp_arv).AsString("city"));
  };
};

void PaymentInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  enum TSearchType {searchByPaxId,searchByGrpId,searchByRegNo,searchByReceiptNo};

  TSearchType search_type;
  if( strcmp((char *)reqNode->name, "PaxByPaxId") == 0) search_type=searchByPaxId;
  else
    if( strcmp((char *)reqNode->name, "PaxByGrpId") == 0) search_type=searchByGrpId;
    else
      if( strcmp((char *)reqNode->name, "PaxByRegNo") == 0) search_type=searchByRegNo;
      else
        if( strcmp((char *)reqNode->name, "PaxByReceiptNo") == 0) search_type=searchByReceiptNo;
        else
          if( strcmp((char *)reqNode->name, "PaxByScanData") == 0)
            throw AstraLocale::UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");
          else return;

  int point_id=NodeAsInteger("point_id",reqNode);
  
  TPrnParams prnParams(reqNode);

  xmlNodePtr dataNode=GetNode("data",resNode);
  if (dataNode==NULL)
    dataNode = NewTextChild(resNode, "data");

  TQuery Qry(&OraSession);

  int pax_id=-1;
  int grp_id=-1;
  bool pr_unaccomp=false;
  bool pr_annul_rcpt=false;
  //᭠砫� ���஡㥬 ������� pax_id
  if (search_type==searchByReceiptNo)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT receipt_id,annul_date,point_id,grp_id,ckin.get_main_pax_id2(grp_id) AS pax_id "
      "FROM bag_receipts WHERE no=:no";
    Qry.CreateVariable("no",otFloat,NodeAsFloat("receipt_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND");
    pr_annul_rcpt=!Qry.FieldIsNULL("annul_date");
    if (Qry.FieldIsNULL("grp_id"))
    {
      //NULL �᫨ ��㯯� ࠧॣ����஢��� �� �訡�� �����
      //⮣�� �뢮��� ⮫쪮 ���⠭��
      LoadReceipts(Qry.FieldAsInteger("receipt_id"),false,prnParams.pr_lat,dataNode);
    }
    else
    {
      grp_id=Qry.FieldAsInteger("grp_id");
      if (!Qry.FieldIsNULL("pax_id"))
        pax_id=Qry.FieldAsInteger("pax_id");
      else
        //NULL �᫨ ��ᮯ஢������� �����
        pr_unaccomp=true;
    };
  };
  if (search_type==searchByRegNo)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT pax.grp_id,pax.pax_id "
      "FROM pax_grp,pax "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.point_dep=:point_id AND reg_no=:reg_no ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("reg_no",otInteger,NodeAsInteger("reg_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
    grp_id=Qry.FieldAsInteger("grp_id");
    pax_id=Qry.FieldAsInteger("pax_id");
  };
  if (search_type==searchByGrpId)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT grp_id,ckin.get_main_pax_id2(grp_id) AS pax_id "
      "FROM pax_grp "
      "WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,NodeAsInteger("grp_id",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.PAX_GRP_OR_LUGGAGE_NOT_CHECKED_IN");
    grp_id=Qry.FieldAsInteger("grp_id");
    if (!Qry.FieldIsNULL("pax_id"))
      pax_id=Qry.FieldAsInteger("pax_id");
    else
      //NULL �᫨ ��ᮯ஢������� �����
      pr_unaccomp=true;
  };
  if (search_type==searchByPaxId)
  {
    pax_id=NodeAsInteger("pax_id",reqNode);
  };

  int point_dep;

  if (!(search_type==searchByReceiptNo && grp_id==-1))
  {
    Qry.Clear();
    if (!pr_unaccomp)
    {
      Qry.SQLText=
        "SELECT pax_grp.grp_id, pax.pax_id, "
        "       point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       class,bag_refuse,pax_grp.tid, "
        "       pax.reg_no, "
        "       RTRIM(pax.surname||' '||pax.name) AS pax_name "
        "FROM pax_grp,pax,airps "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax_grp.airp_arv=airps.code AND "
        "      pax.pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,pax_id);
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN");
    }
    else
    {
      Qry.SQLText=
        "SELECT pax_grp.grp_id, NULL AS pax_id, "
        "       point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       class,bag_refuse,pax_grp.tid, "
        "       NULL AS reg_no, "
        "       NULL AS pax_name "
        "FROM pax_grp,airps "
        "WHERE pax_grp.airp_arv=airps.code AND grp_id=:grp_id ";
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.Execute();
      if (Qry.Eof)
        throw AstraLocale::UserException("MSG.LUGGAGE_NOT_CHECKED_IN");
    };
    point_dep=Qry.FieldAsInteger("point_dep");
  }
  else
  {
    point_dep=Qry.FieldAsInteger("point_id");
  };

  if (point_id!=point_dep)
  {
    point_id=point_dep;
    if (!TripsInterface::readTripHeader( point_id, dataNode) && !pr_annul_rcpt)
    {
      TQuery FltQry(&OraSession);
      FltQry.Clear();
      FltQry.SQLText=
        "SELECT airline,flt_no,suffix,airp,scd_out, "
        "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
        "FROM points WHERE point_id=:point_id AND pr_del>=0";
      FltQry.CreateVariable("point_id",otInteger,point_id);
      FltQry.Execute();
      string msg;
      if (!FltQry.Eof)
      {
        TTripInfo info(FltQry);
        if (!pr_unaccomp)
          msg=getLocaleText("MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
      }
      else
      {
        if (!pr_unaccomp)
          msg=getLocaleText("MSG.PASSENGER.FROM_OTHER_FLIGHT");
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_OTHER_FLIGHT");
      };
      if (!pr_annul_rcpt)
        throw AstraLocale::UserException(msg);
      else
        AstraLocale::showErrorMessage(msg); //� �� ��砥 �����뢠�� ���㫨஢����� ���⠭��
    };
  };

  if (search_type==searchByReceiptNo && grp_id==-1) return;

  grp_id=Qry.FieldAsInteger("grp_id");
  NewTextChild(dataNode,"grp_id",grp_id);
  NewTextChild(dataNode,"point_dep",Qry.FieldAsInteger("point_dep"));
  NewTextChild(dataNode,"airp_dep",Qry.FieldAsString("airp_dep"));
  NewTextChild(dataNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(dataNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(dataNode,"class",Qry.FieldAsString("class"));
  NewTextChild(dataNode,"pr_refuse",(int)(Qry.FieldAsInteger("bag_refuse")!=0));
  NewTextChild(dataNode,"reg_no",Qry.FieldAsInteger("reg_no"));
  NewTextChild(dataNode,"pax_name",Qry.FieldAsString("pax_name"));
  TQuery PaxDocQry(&OraSession);
  if (!Qry.FieldIsNULL("pax_id"))
    NewTextChild(dataNode,"pax_doc",CheckIn::GetPaxDocStr(NoExists, Qry.FieldAsInteger("pax_id"), PaxDocQry));
  else
    NewTextChild(dataNode,"pax_doc");
  NewTextChild(dataNode,"tid",Qry.FieldAsInteger("tid"));
  //���ଠ�� �� �࠭���� ������
  TTrferRoute trfer;
  trfer.GetRoute(grp_id, trtNotFirstSeg);
  if (TReqInfo::Instance()->desk.compatible(BAG_RCPT_KITS_VERSION))
  {
    if (!trfer.empty())
      BuildTransfer(trfer, dataNode);
  }
  else
  {
    string last_trfer_airp, last_trfer_city;
    if (!trfer.empty())
    {
      last_trfer_airp=trfer.back().airp_arv;
      try
      {
        last_trfer_city=base_tables.get("airps").get_row("code",last_trfer_airp).AsString("city");
      }
      catch(EBaseTableError) {};
    };
    NewTextChild(dataNode, "last_trfer_airp", last_trfer_airp);
    NewTextChild(dataNode, "last_trfer_city", last_trfer_city);
  };

  if (!pr_unaccomp)
  {
    string subcl;
    Qry.Clear();
    Qry.SQLText=
      "SELECT DISTINCT subclass FROM pax "
      "WHERE grp_id=:grp_id AND subclass IS NOT NULL";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      subcl=Qry.FieldAsString("subclass");
      Qry.Next();
      if (!Qry.Eof) subcl="";
    };

    Qry.Clear();
    Qry.SQLText=
      "SELECT ticket_no FROM pax "
      "WHERE grp_id=:grp_id AND refuse IS NULL AND ticket_no IS NOT NULL "
      "ORDER BY ticket_no";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();


    string tickets,ticket,first_part;
    for(;!Qry.Eof;Qry.Next())
    {
      if (!tickets.empty()) tickets+='/';
      ticket=Qry.FieldAsString("ticket_no");
      if (ticket.size()<=2)
      {
        tickets+=ticket;
        first_part="";
      }
      else
      {
        if (first_part==ticket.substr(0,ticket.size()-2))
          tickets+=ticket.substr(ticket.size()-2);
        else
        {
          tickets+=ticket;
          first_part=ticket.substr(0,ticket.size()-2);
        };
      };
    };

    NewTextChild(dataNode,"subclass",subcl);
    NewTextChild(dataNode,"tickets",tickets);
  }
  else
  {
    NewTextChild(dataNode,"subclass");
    NewTextChild(dataNode,"tickets");
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT mark_trips.airline,mark_trips.flt_no,mark_trips.suffix, "
    "       mark_trips.scd,mark_trips.airp_dep,pr_mark_norms "
    "FROM pax_grp,mark_trips "
    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    xmlNodePtr markFltNode=NewTextChild(dataNode,"mark_flight");
    NewTextChild(markFltNode,"airline",Qry.FieldAsString("airline"));
    if (TReqInfo::Instance()->desk.compatible(BAG_RCPT_KITS_VERSION) &&
        !TReqInfo::Instance()->desk.compatible(AIRCODE_BUGFIX_VERSION))
      NewTextChild(markFltNode,"aircode",Qry.FieldAsString("airline"));
    else
      NewTextChild(markFltNode,"aircode",
                   base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline")).AsString("aircode"));
    NewTextChild(markFltNode,"flt_no",Qry.FieldAsInteger("flt_no"));
    NewTextChild(markFltNode,"suffix",Qry.FieldAsString("suffix"));
    NewTextChild(markFltNode,"scd",DateTimeToStr(Qry.FieldAsDateTime("scd")));
    NewTextChild(markFltNode,"airp_dep",Qry.FieldAsString("airp_dep"));
    NewTextChild(markFltNode,"pr_mark_norms",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
    NewTextChild(markFltNode,"pr_mark_rates",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
  };
  
  if (!pr_unaccomp)
  {
    //����㧪� ������� ���ᠦ�஢ �������� �㫮�
    Qry.Clear();
    Qry.SQLText=
      "SELECT pax_id, bag_pool_num, surname, name, pers_type, seats, refuse "
      "FROM pax "
      "WHERE grp_id=:grp_id AND bag_pool_num IS NOT NULL AND "
      "      pax_id=ckin.get_bag_pool_pax_id(grp_id,bag_pool_num)";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    xmlNodePtr paxsNode=NewTextChild(dataNode,"passengers");
    for(;!Qry.Eof;Qry.Next())
    {
      xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
      NewTextChild(paxNode,"pax_id",Qry.FieldAsInteger("pax_id"));
      NewTextChild(paxNode,"surname",Qry.FieldAsString("surname"));
      NewTextChild(paxNode,"name",Qry.FieldAsString("name"),"");
      NewTextChild(paxNode,"pers_type",Qry.FieldAsString("pers_type"),EncodePerson(ASTRA::adult));
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger("seats"),1);
      NewTextChild(paxNode,"refuse",Qry.FieldAsString("refuse"),"");
      NewTextChild(paxNode,"bag_pool_num",Qry.FieldAsInteger("bag_pool_num"));
    };
  };

  CheckIn::LoadBag(grp_id,dataNode);
  CheckInInterface::LoadPaidBag(grp_id,dataNode);
  LoadReceipts(grp_id,true,prnParams.pr_lat,dataNode);
};

void PaymentInterface::LoadReceipts(int id, bool pr_grp, bool pr_lat, xmlNodePtr dataNode)
{
  if (dataNode==NULL) return;

  TQuery Qry(&OraSession);
  if (pr_grp)
  {
    //���⠭樨 �।������
    Qry.Clear();
    Qry.SQLText="SELECT * FROM bag_prepay WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id", otInteger, id);
    Qry.Execute();
    xmlNodePtr node=NewTextChild(dataNode,"prepayment");
    for(;!Qry.Eof;Qry.Next())
    {
      xmlNodePtr receiptNode=NewTextChild(node,"receipt");
      NewTextChild(receiptNode,"id",Qry.FieldAsInteger("receipt_id"));
      NewTextChild(receiptNode,"no",Qry.FieldAsString("no"));
      NewTextChild(receiptNode,"aircode",Qry.FieldAsString("aircode"));
      if (!Qry.FieldIsNULL("ex_weight"))
        NewTextChild(receiptNode,"ex_weight",Qry.FieldAsInteger("ex_weight"));
      if (!Qry.FieldIsNULL("bag_type"))
        NewTextChild(receiptNode,"bag_type",Qry.FieldAsInteger("bag_type"));
      else
        NewTextChild(receiptNode,"bag_type");
      if (!Qry.FieldIsNULL("value"))
      {
        NewTextChild(receiptNode,"value",Qry.FieldAsFloat("value"));
        NewTextChild(receiptNode,"value_cur",Qry.FieldAsString("value_cur"));
      };
    };
  };

  Qry.Clear();
  if (pr_grp)
  {
    Qry.SQLText=
      "SELECT * FROM bag_receipts "
      "WHERE grp_id=:grp_id ORDER BY issue_date";
    Qry.CreateVariable("grp_id", otInteger, id);
  }
  else
  {
    Qry.SQLText=
      "SELECT * FROM bag_receipts "
      "WHERE receipt_id=:receipt_id";
    Qry.CreateVariable("receipt_id", otInteger, id);
  };
  Qry.Execute();
  xmlNodePtr node=NewTextChild(dataNode,"payment");
  for(;!Qry.Eof;Qry.Next())
  {
    int rcpt_id=Qry.FieldAsInteger("receipt_id");
    TBagReceipt rcpt;
    GetReceiptFromDB(Qry,rcpt);
    xmlNodePtr rcptNode=NewTextChild(node,"receipt");
    PutReceiptToXML(rcpt,rcpt_id,pr_lat,rcptNode);
  };
};

int PaymentInterface::LockAndUpdTid(int point_dep, int grp_id, int tid)
{
  TQuery Qry(&OraSession);
  //��稬 ३�
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id,cycle_tid__seq.nextval AS tid "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  int new_tid=Qry.FieldAsInteger("tid");

  Qry.Clear();
  Qry.SQLText=
    "UPDATE pax_grp "
    "SET tid=cycle_tid__seq.currval "
    "WHERE grp_id=:grp_id AND tid=:tid";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("tid",otInteger,tid);
  Qry.Execute();
  if (Qry.RowsProcessed()<=0)
    throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
  return new_tid;
};

void PaymentInterface::SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    CheckIn::SaveBag(point_dep,grp_id,ASTRA::NoExists,reqNode);
    CheckInInterface::SavePaidBag(grp_id,reqNode);

    TReqInfo::Instance()->MsgToLog(
            "����� �� ������� ��䠬 � 業���� ������ ��࠭���.",
            ASTRA::evtPay,point_dep,0,grp_id);
};

void PaymentInterface::UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
    bool pr_del=NodeAsIntegerFast("pr_del",node);
    xmlNodePtr idNode=GetNodeFast("id",node);

    string old_aircode, old_no;
    TQuery Qry(&OraSession);
    Qry.Clear();
    if (idNode!=NULL)
    {
        Qry.CreateVariable("receipt_id",otInteger,NodeAsInteger(idNode));
        Qry.SQLText="SELECT * FROM bag_prepay WHERE receipt_id=:receipt_id";
        Qry.Execute();
        if (Qry.Eof) throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        old_aircode=Qry.FieldAsString("aircode");
        old_no=Qry.FieldAsString("no");
    }
    else
        Qry.CreateVariable("receipt_id",otInteger,FNull);


    if (!pr_del)
    {
        string aircode = NodeAsStringFast("aircode",node);
        string no = NodeAsStringFast("no",node);

        if (idNode==NULL)
            Qry.SQLText=
                "BEGIN "
                "  SELECT cycle_id__seq.nextval INTO :receipt_id FROM dual; "
                "  INSERT INTO bag_prepay "
                "        (receipt_id,grp_id,no,aircode,ex_weight,bag_type,value,value_cur) "
                "  VALUES(:receipt_id,:grp_id,:no,:aircode,:ex_weight,:bag_type,:value,:value_cur); "
                "END; ";
        else
            Qry.SQLText=
                "UPDATE bag_prepay "
                "SET no=:no,aircode=:aircode,ex_weight=:ex_weight, "
                "    bag_type=:bag_type,value=:value,value_cur=:value_cur "
                "WHERE receipt_id=:receipt_id AND grp_id=:grp_id";

        Qry.CreateVariable("grp_id",otInteger,grp_id);
        Qry.CreateVariable("no",otString,no);
        Qry.CreateVariable("aircode",otString,aircode);
        if (!NodeIsNULLFast("ex_weight",node,true))
            Qry.CreateVariable("ex_weight",otInteger,NodeAsIntegerFast("ex_weight",node));
        else
            Qry.CreateVariable("ex_weight",otInteger,FNull);
        if (!NodeIsNULLFast("bag_type",node))
            Qry.CreateVariable("bag_type",otInteger,NodeAsIntegerFast("bag_type",node));
        else
            Qry.CreateVariable("bag_type",otInteger,FNull);
        if (!NodeIsNULLFast("value",node,true))
            Qry.CreateVariable("value",otFloat,NodeAsFloatFast("value",node));
        else
            Qry.CreateVariable("value",otFloat,FNull);
        Qry.CreateVariable("value_cur",otString,NodeAsStringFast("value_cur",node,""));
        Qry.Execute();
        if (idNode!=NULL && Qry.RowsProcessed()<=0)
            throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");

        NewTextChild(resNode,"receipt_id",Qry.GetVariableAsInteger("receipt_id"));

        ostringstream logmsg;
        if (idNode==NULL)
        {
          logmsg << "���⠭�� �।������ " << aircode << " " << no << " �������. ";
        }
        else
        {
          if (aircode==old_aircode && no==old_no)
          {
            logmsg << "���⠭�� �।������ " << aircode << " " << no << " ��������. ";
          }
          else
          {
            logmsg << "���⠭�� �।������ " << old_aircode << " " << old_no << " 㤠����";
            reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
            logmsg.str("");
            logmsg << "���⠭�� �।������ " << aircode << " " << no << " �������. ";
          };
        };
        if (!NodeIsNULLFast("ex_weight",node,true))
        {
          if (!NodeIsNULLFast("bag_type",node))
            logmsg << "��� ������: " << NodeAsIntegerFast("bag_type",node)
                   << ", ��� ������: " << NodeAsIntegerFast("ex_weight",node) << " ��.";
          else
            logmsg << "��� ������: " << NodeAsIntegerFast("ex_weight",node) << " ��.";
        };
        if (!NodeIsNULLFast("value",node,true))
        {
          logmsg << "�������� ������: "
                 << fixed << setprecision(2) << NodeAsFloatFast("value",node)
                 << NodeAsStringFast("value_cur",node,"");
        };
        reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
    }
    else
    {
        Qry.SQLText=
            "DELETE FROM bag_prepay WHERE receipt_id=:receipt_id";
        Qry.Execute();
        if (Qry.RowsProcessed()<=0)
            throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        ostringstream logmsg;
        logmsg << "���⠭�� �।������ " << old_aircode << " " << old_no << " 㤠����";
        reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
    };
}

//�� ���� � ��������
bool PaymentInterface::GetReceiptFromDB(int id, TBagReceipt &rcpt)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT * FROM bag_receipts WHERE receipt_id=:id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();

  return GetReceiptFromDB(Qry,rcpt);
};

//�� ���� � ��������
bool PaymentInterface::GetReceiptFromDB(TQuery &Qry, TBagReceipt &rcpt)
{
  if (Qry.Eof) return false;
  rcpt.form_type=Qry.FieldAsString("form_type");
  rcpt.no=Qry.FieldAsFloat("no");
  rcpt.pax_name=Qry.FieldAsString("pax_name");
  rcpt.pax_doc=Qry.FieldAsString("pax_doc");
  rcpt.service_type=Qry.FieldAsInteger("service_type");
  if (!Qry.FieldIsNULL("bag_type"))
    rcpt.bag_type=Qry.FieldAsInteger("bag_type");
  else
    rcpt.bag_type=-1;
  rcpt.bag_name=Qry.FieldAsString("bag_name");
  rcpt.tickets=Qry.FieldAsString("tickets");
  rcpt.prev_no=Qry.FieldAsString("prev_no");
  rcpt.airline=Qry.FieldAsString("airline");
  rcpt.aircode=Qry.FieldAsString("aircode");
  if (!Qry.FieldIsNULL("flt_no"))
    rcpt.flt_no=Qry.FieldAsInteger("flt_no");
  else
    rcpt.flt_no=-1;
  rcpt.suffix=Qry.FieldAsString("suffix");
  rcpt.scd_local_date=Qry.FieldIsNULL("scd_local_date")?NoExists:Qry.FieldAsDateTime("scd_local_date");
  rcpt.airp_dep=Qry.FieldAsString("airp_dep");
  rcpt.airp_arv=Qry.FieldAsString("airp_arv");
  if (!Qry.FieldIsNULL("ex_amount"))
    rcpt.ex_amount=Qry.FieldAsInteger("ex_amount");
  else
    rcpt.ex_amount=-1;
  if (!Qry.FieldIsNULL("ex_weight"))
    rcpt.ex_weight=Qry.FieldAsInteger("ex_weight");
  else
    rcpt.ex_weight=-1;
  if (!Qry.FieldIsNULL("value_tax"))
    rcpt.value_tax=Qry.FieldAsFloat("value_tax");
  else
    rcpt.value_tax=-1.0;
  rcpt.rate=Qry.FieldAsFloat("rate");
  rcpt.rate_cur=Qry.FieldAsString("rate_cur");
  rcpt.exch_rate=Qry.FieldAsInteger("exch_rate");
  rcpt.exch_pay_rate=Qry.FieldAsFloat("exch_pay_rate");
  rcpt.pay_rate_cur=Qry.FieldAsString("pay_rate_cur");
  rcpt.remarks=Qry.FieldAsString("remarks");
  rcpt.issue_date=Qry.FieldAsDateTime("issue_date");
  rcpt.issue_desk=Qry.FieldAsString("issue_desk");
  rcpt.issue_place=Qry.FieldAsString("issue_place");
  if (!Qry.FieldIsNULL("annul_date"))
    rcpt.annul_date=Qry.FieldAsDateTime("annul_date");
  else
    rcpt.annul_date=NoExists;
  rcpt.annul_desk=Qry.FieldAsString("annul_desk");
  
  rcpt.is_inter=Qry.FieldAsInteger("is_inter")!=0;
  rcpt.desk_lang=Qry.FieldAsString("desk_lang");

  //��� ������
  rcpt.pay_types.clear();
  TQuery PayTypesQry(&OraSession);
  PayTypesQry.SQLText=
    "SELECT * FROM bag_pay_types WHERE receipt_id=:receipt_id ORDER BY num";
  PayTypesQry.CreateVariable("receipt_id",otInteger,Qry.FieldAsInteger("receipt_id"));
  PayTypesQry.Execute();
  TBagPayType payType;
  for(;!PayTypesQry.Eof;PayTypesQry.Next())
  {
    payType.pay_type=PayTypesQry.FieldAsString("pay_type");
    payType.pay_rate_sum=PayTypesQry.FieldAsFloat("pay_rate_sum");
    payType.extra=PayTypesQry.FieldAsString("extra");
    rcpt.pay_types.push_back(payType);
  };

  rcpt.kit_id=Qry.FieldIsNULL("kit_id")?NoExists:Qry.FieldAsInteger("kit_id");
  rcpt.kit_num=Qry.FieldIsNULL("kit_num")?NoExists:Qry.FieldAsInteger("kit_num");
  rcpt.kit_items.clear();
  if (rcpt.kit_id!=NoExists)
  {
    TQuery KitQry(&OraSession);
    KitQry.SQLText="SELECT * FROM bag_rcpt_kits WHERE kit_id=:kit_id ORDER BY kit_num";
    KitQry.CreateVariable("kit_id", otInteger, rcpt.kit_id);
    KitQry.Execute();
    for(int k=0;!KitQry.Eof;KitQry.Next(),k++)
    {
      if (k!=KitQry.FieldAsInteger("kit_num"))
        throw Exception("PaymentInterface::GetReceiptFromDB: Data integrity is broken (kit_id=%d)",rcpt.kit_id);
      TBagReceiptKitItem item;
      item.form_type=KitQry.FieldAsString("form_type");
      item.no=KitQry.FieldAsFloat("no");
      item.aircode=KitQry.FieldAsString("aircode");
      rcpt.kit_items.push_back(item);
    };
  };

  return true;
};

//�� �������� � ����
int PaymentInterface::PutReceiptToDB(const TBagReceipt &rcpt, int point_id, int grp_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT cycle_id__seq.nextval,SYSTEM.UTCSYSDATE INTO :receipt_id,:issue_date FROM dual; "
    "  INSERT INTO bag_receipts "
    "        (receipt_id,point_id,grp_id,status,is_inter,desk_lang,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name, "
    "         tickets,prev_no,airline,aircode,flt_no,suffix,scd_local_date,airp_dep,airp_arv,ex_amount,ex_weight,value_tax, "
    "         rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,remarks, "
    "         issue_date,issue_place,issue_user_id,issue_desk,kit_id,kit_num) "
    "  VALUES(:receipt_id,:point_id,:grp_id,:status,:is_inter,:desk_lang,:form_type,:no,:pax_name,:pax_doc,:service_type,:bag_type,:bag_name, "
    "         :tickets,:prev_no,:airline,:aircode,:flt_no,:suffix,:scd_local_date,:airp_dep,:airp_arv,:ex_amount,:ex_weight,:value_tax, "
    "         :rate,:rate_cur,:exch_rate,:exch_pay_rate,:pay_rate_cur,:remarks, "
    "         :issue_date,:issue_place,:issue_user_id,:issue_desk,:kit_id,:kit_num); "
    "END;";
  Qry.CreateVariable("receipt_id",otInteger,FNull);
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("status",otString,"�");
  Qry.CreateVariable("is_inter",otInteger,(int)rcpt.is_inter);
  Qry.CreateVariable("desk_lang",otString,rcpt.desk_lang);
  Qry.CreateVariable("form_type",otString,rcpt.form_type);
  Qry.CreateVariable("no",otFloat,rcpt.no);
  Qry.CreateVariable("pax_name",otString,rcpt.pax_name);
  Qry.CreateVariable("pax_doc",otString,rcpt.pax_doc);
  Qry.CreateVariable("service_type",otInteger,rcpt.service_type);
  if (rcpt.bag_type!=-1)
    Qry.CreateVariable("bag_type",otInteger,rcpt.bag_type);
  else
    Qry.CreateVariable("bag_type",otInteger,FNull);
  Qry.CreateVariable("bag_name",otString,rcpt.bag_name);
  Qry.CreateVariable("tickets",otString,rcpt.tickets);
  Qry.CreateVariable("prev_no",otString,rcpt.prev_no);

  Qry.CreateVariable("airline",otString,rcpt.airline);
  Qry.CreateVariable("aircode",otString,rcpt.aircode);
  if (rcpt.flt_no!=-1)
    Qry.CreateVariable("flt_no",otInteger,rcpt.flt_no);
  else
    Qry.CreateVariable("flt_no",otInteger,FNull);
  Qry.CreateVariable("suffix",otString,rcpt.suffix);
  rcpt.scd_local_date==NoExists?Qry.CreateVariable("scd_local_date",otDate,FNull):
                                Qry.CreateVariable("scd_local_date",otDate,rcpt.scd_local_date);
  Qry.CreateVariable("airp_dep",otString,rcpt.airp_dep);
  Qry.CreateVariable("airp_arv",otString,rcpt.airp_arv);
  if (rcpt.ex_amount!=-1)
    Qry.CreateVariable("ex_amount",otInteger,rcpt.ex_amount);
  else
    Qry.CreateVariable("ex_amount",otInteger,FNull);
  if (rcpt.ex_weight!=-1)
    Qry.CreateVariable("ex_weight",otInteger,rcpt.ex_weight);
  else
    Qry.CreateVariable("ex_weight",otInteger,FNull);
  if (rcpt.value_tax!=-1.0)
    Qry.CreateVariable("value_tax",otFloat,rcpt.value_tax);
  else
    Qry.CreateVariable("value_tax",otFloat,FNull);
  Qry.CreateVariable("rate",otFloat,rcpt.rate);
  Qry.CreateVariable("rate_cur",otString,rcpt.rate_cur);
  if (rcpt.exch_rate!=-1)
    Qry.CreateVariable("exch_rate",otInteger,rcpt.exch_rate);
  else
    Qry.CreateVariable("exch_rate",otInteger,FNull);
  if (rcpt.exch_pay_rate!=-1.0)
    Qry.CreateVariable("exch_pay_rate",otFloat,rcpt.exch_pay_rate);
  else
    Qry.CreateVariable("exch_pay_rate",otFloat,FNull);
  Qry.CreateVariable("pay_rate_cur",otString,rcpt.pay_rate_cur);
  Qry.CreateVariable("remarks",otString,rcpt.remarks);
  Qry.CreateVariable("issue_date",otDate,rcpt.issue_date);
  Qry.CreateVariable("issue_place",otString,rcpt.issue_place);
  Qry.CreateVariable("issue_user_id",otInteger,reqInfo->user.user_id);
  Qry.CreateVariable("issue_desk",otString,rcpt.issue_desk);
  if (rcpt.kit_id!=NoExists && rcpt.kit_num!=NoExists)
  {
    Qry.CreateVariable("kit_id",otInteger,rcpt.kit_id);
    Qry.CreateVariable("kit_num",otInteger,rcpt.kit_num);
  }
  else
  {
    Qry.CreateVariable("kit_id",otInteger,FNull);
    Qry.CreateVariable("kit_num",otInteger,FNull);
  };
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code==1)
      throw AstraLocale::UserException("MSG.RECEIPT_BLANK_NO_ALREADY_USED", LParams() << LParam("no", rcpt.no));
    else
      throw;
  };
  int receipt_id=Qry.GetVariableAsInteger("receipt_id");

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO bag_pay_types(receipt_id,num,pay_type,pay_rate_sum,extra) "
    "VALUES (:receipt_id,:num,:pay_type,:pay_rate_sum,:extra)";
  Qry.CreateVariable("receipt_id",otInteger,receipt_id);
  Qry.DeclareVariable("num",otInteger);
  Qry.DeclareVariable("pay_type",otString);
  Qry.DeclareVariable("pay_rate_sum",otFloat);
  Qry.DeclareVariable("extra",otString);
  int num=1;
  for(vector<TBagPayType>::const_iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();++i,num++)
  {
    Qry.SetVariable("num",num);
    Qry.SetVariable("pay_type",i->pay_type);
    Qry.SetVariable("pay_rate_sum",i->pay_rate_sum);
    Qry.SetVariable("extra",i->extra);
    Qry.Execute();
  };
  
  if (rcpt.kit_id==NoExists && rcpt.kit_num!=NoExists)
  {
    //�஢�ઠ �㡫�஢���� ����஢ ���⠭権
    Qry.Clear();
    Qry.SQLText=
      "SELECT receipt_id FROM bag_receipts WHERE form_type=:form_type AND no=:no";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("no", otFloat);
    for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i)
    {
      if (i->form_type==rcpt.form_type && i->no==rcpt.no) continue;
      Qry.SetVariable("form_type", i->form_type);
      Qry.SetVariable("no", i->no);
      Qry.Execute();
      if (!Qry.Eof)
        throw AstraLocale::UserException("MSG.RECEIPT_BLANK_NO_ALREADY_USED", LParams() << LParam("no", i->no));
    };
  
    //���� �������� �������� � bag_rcpt_kits
    Qry.Clear();
    Qry.SQLText=
      "INSERT INTO bag_rcpt_kits(kit_id, kit_num, form_type, no, aircode) "
      "VALUES(:receipt_id, :kit_num, :form_type, :no, :aircode)";
    Qry.CreateVariable("receipt_id", otInteger, receipt_id);
    Qry.DeclareVariable("kit_num", otInteger);
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("no", otFloat);
    Qry.DeclareVariable("aircode", otString);
    int num=0;
    for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i,num++)
    {
      Qry.SetVariable("kit_num", num);
      Qry.SetVariable("form_type", i->form_type);
      Qry.SetVariable("no", i->no);
      Qry.SetVariable("aircode", i->aircode);
      Qry.Execute();
    };
    Qry.Clear();
    Qry.SQLText=
      "UPDATE bag_receipts SET kit_id=:receipt_id, kit_num=:kit_num WHERE receipt_id=:receipt_id";
    Qry.CreateVariable("receipt_id", otInteger, receipt_id);
    Qry.CreateVariable("kit_num", otInteger, rcpt.kit_num);
    Qry.Execute();
  };
  
  return receipt_id;
};


//�� �������� � XML
void PaymentInterface::PutReceiptToXML(const TBagReceipt &rcpt, int rcpt_id, bool pr_lat, xmlNodePtr rcptNode)
{
  if (rcptNode==NULL) return;

  if (rcpt_id!=-1)
    NewTextChild(rcptNode,"id",rcpt_id);
  TTagLang tag_lang;
  tag_lang.Init(rcpt, pr_lat);
  NewTextChild(rcptNode,"pr_lat",(int)(tag_lang.IsInter()));
  NewTextChild(rcptNode,"form_type",rcpt.form_type);
  NewTextChild(rcptNode,"no",rcpt.no);
  NewTextChild(rcptNode,"pax_name",rcpt.pax_name);
  NewTextChild(rcptNode,"pax_doc",rcpt.pax_doc);
  NewTextChild(rcptNode,"service_type",rcpt.service_type);
  if (rcpt.bag_type!=-1)
    NewTextChild(rcptNode,"bag_type",rcpt.bag_type);
  else
    NewTextChild(rcptNode,"bag_type");
  NewTextChild(rcptNode,"bag_name",rcpt.bag_name,"");
  NewTextChild(rcptNode,"tickets",rcpt.tickets);
  NewTextChild(rcptNode,"prev_no",rcpt.prev_no,"");
  NewTextChild(rcptNode,"airline",rcpt.airline);
  NewTextChild(rcptNode,"aircode",rcpt.aircode);
  if (rcpt.flt_no!=-1)
  {
    NewTextChild(rcptNode,"flt_no",rcpt.flt_no);
    NewTextChild(rcptNode,"suffix",rcpt.suffix);
  };
  NewTextChild(rcptNode,"airp_dep",rcpt.airp_dep);
  NewTextChild(rcptNode,"airp_arv",rcpt.airp_arv);
  NewTextChild(rcptNode,"ex_amount",rcpt.ex_amount,-1);
  NewTextChild(rcptNode,"ex_weight",rcpt.ex_weight,-1);
  NewTextChild(rcptNode,"value_tax",rcpt.value_tax,-1.0);
  NewTextChild(rcptNode,"rate",rcpt.rate);
  NewTextChild(rcptNode,"rate_cur",rcpt.rate_cur);
  NewTextChild(rcptNode,"exch_rate",rcpt.exch_rate,-1);
  NewTextChild(rcptNode,"exch_pay_rate",rcpt.exch_pay_rate,-1.0);
  NewTextChild(rcptNode,"pay_rate_cur",rcpt.pay_rate_cur);
  NewTextChild(rcptNode,"remarks",rcpt.remarks,"");
  NewTextChild(rcptNode,"issue_date",DateTimeToStr(rcpt.issue_date));
  NewTextChild(rcptNode,"issue_place",rcpt.issue_place);
  if (rcpt.annul_date!=NoExists)
    NewTextChild(rcptNode,"annul_date",DateTimeToStr(rcpt.annul_date));

  xmlNodePtr payTypesNode=NewTextChild(rcptNode,"pay_types");
  for(vector<TBagPayType>::const_iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();++i)
  {
    xmlNodePtr node=NewTextChild(payTypesNode,"pay_type");
    NewTextChild(node,"pay_type",i->pay_type);
    NewTextChild(node,"pay_rate_sum",i->pay_rate_sum);
    NewTextChild(node,"extra",i->extra,"");
  };
  
  NewTextChild(rcptNode,"kit_id",rcpt.kit_id,NoExists);
  NewTextChild(rcptNode,"kit_num",rcpt.kit_num,NoExists);
  if (rcpt.kit_num!=NoExists)
  {
    xmlNodePtr kitItemsNode=NewTextChild(rcptNode,"kit_items");
    for(vector<TBagReceiptKitItem>::const_iterator i=rcpt.kit_items.begin();i!=rcpt.kit_items.end();++i)
    {
      xmlNodePtr node=NewTextChild(kitItemsNode,"item");
      NewTextChild(node,"form_type",i->form_type);
      NewTextChild(node,"no",i->no);
      NewTextChild(node,"aircode",i->aircode);
    };
  };
};

double PaymentInterface::GetCurrNo(int user_id, const string &form_type)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT curr_no "
    "FROM form_types,form_packs "
    "WHERE form_types.code=form_packs.type(+) AND "
    "      form_types.code=:form_type AND "
    "      form_packs.user_id(+)=:user_id";
  Qry.CreateVariable("form_type",otString,form_type);
  Qry.CreateVariable("user_id",otInteger,user_id);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.BLANK_SET_PRINTING_UNAVAILABLE");
  if (!Qry.FieldIsNULL("curr_no"))
    return Qry.FieldAsFloat("curr_no");
  else
    return -1.0;
};

double PaymentInterface::GetNextNo(const string &form_type, double no)
{
  double result=-1.0;
  try
  {
    const TFormTypesRow &form=(const TFormTypesRow&)(base_tables.get("form_types").get_row("code",form_type));
    no+=1;
    ostringstream no_str;
    no_str << fixed << setw(form.no_len) << setfill('0') << setprecision(0) << no;
    if ((int)no_str.str().size()==form.no_len)
    {
      if ((int)form.code.size()>form.series_len)
      {
        if (no_str.str().find(form.code.substr(form.series_len))==0) result=no;
      }
      else result=no;
    };
  }
  catch(EBaseTableError) {};
  return result;
};

string GetKitPrevNoStr(const vector<TBagReceiptKitItem> &items)
{
  ostringstream result;
  vector<string> rcpts;
  for(vector<TBagReceiptKitItem>::const_iterator i=items.begin();i!=items.end();++i)
  {
    int no_len=10;
    try
    {
      no_len=base_tables.get("form_types").get_row("code",i->form_type).AsInteger("no_len");
    }
    catch(EBaseTableError) {};
    ostringstream no_str;
    no_str << fixed << setw(no_len) << setfill('0') << setprecision(0) << i->no;
    rcpts.push_back(no_str.str());
    if (i==items.begin()) result << i->aircode;
  };
  result << GetBagRcptStr(rcpts);
  return result.str();
};

//�� XML � ��������
void PaymentInterface::GetReceiptFromXML(xmlNodePtr reqNode, TBagReceipt &rcpt)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  TPrnParams prnParams(reqNode);

  xmlNodePtr rcptNode=NodeAsNode("receipt",reqNode);

  int grp_id=NoExists;
  TQuery Qry(&OraSession);
  if (GetNode("kit_id",rcptNode)!=NULL)
  {
    //⥫��ࠬ�� �� ��������
    int kit_id=NodeAsInteger("kit_id",rcptNode);
    Qry.Clear();
    Qry.SQLText="SELECT * FROM bag_receipts WHERE kit_id=:kit_id AND rownum<2";
    Qry.CreateVariable("kit_id",otInteger,kit_id);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    GetReceiptFromDB(Qry,rcpt);
    if (Qry.FieldIsNULL("grp_id"))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    grp_id=Qry.FieldAsInteger("grp_id");
    rcpt.kit_num=NodeAsInteger("kit_num",rcptNode); //��易⥫쭮 ���������
  }
  else
  {
    rcpt.kit_id=NoExists;
    grp_id=NodeAsInteger("grp_id",reqNode);
    if (GetNode("kit_num",rcptNode)!=NULL)
      rcpt.kit_num=NodeAsInteger("kit_num",rcptNode);
    else
      rcpt.kit_num=NoExists;
    rcpt.kit_items.clear();
    if (rcpt.kit_num!=NoExists)
    {
      //⥫��ࠬ�� �� ��������, �� ��. �������� �� �� ��।����
      //� �⮬ ��砥 ������ ������� ᯨ᮪ ����஢, �������� � ��������
      xmlNodePtr node=NodeAsNode("kit_items",rcptNode)->children;
      for(;node!=NULL;node=node->next)
      {
        TBagReceiptKitItem item;
        item.form_type=NodeAsString("form_type",node);
        item.no=NodeAsFloat("no",node);
        item.aircode=NodeAsString("aircode",node);
        rcpt.kit_items.push_back(item);
      };
    };
  };
  
  TTrferRoute route;
  if (!route.GetRoute(grp_id, trtWithFirstSeg) || route.empty())
    throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_dep, point_arv, pr_mark_norms, "
    "       mark_trips.airline AS airline_mark, "
    "       mark_trips.flt_no AS flt_no_mark, "
    "       mark_trips.suffix AS suffix_mark "
    "FROM pax_grp,mark_trips "
    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND "
    "      pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");

  if (rcpt.kit_num==NoExists || rcpt.kit_num==0)
  {
    if (Qry.FieldAsInteger("pr_mark_norms")!=0)
    {
      rcpt.airline=Qry.FieldAsString("airline_mark");
      rcpt.flt_no=Qry.FieldAsInteger("flt_no_mark");
      rcpt.suffix=Qry.FieldAsString("suffix_mark");
    }
    else
    {
      rcpt.airline=route.begin()->operFlt.airline;
      rcpt.flt_no=route.begin()->operFlt.flt_no;
      rcpt.suffix=route.begin()->operFlt.suffix;

    };
    rcpt.scd_local_date=route.begin()->operFlt.scd_out; //TTrferRoute ᮤ�ন� ������� ����
    rcpt.airp_dep=route.begin()->operFlt.airp;
    if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
      rcpt.airp_arv=route.begin()->airp_arv;
    else
      rcpt.airp_arv=NodeAsString("airp_arv",rcptNode);
  }
  else
  {
    if (!(rcpt.kit_num>=0 && rcpt.kit_num<(int)route.size()))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    const TTrferRouteItem &item=route.at(rcpt.kit_num);
    rcpt.airline=item.operFlt.airline;
    rcpt.flt_no=item.operFlt.flt_no;
    rcpt.suffix=item.operFlt.suffix;
    rcpt.scd_local_date=item.operFlt.scd_out;
    rcpt.airp_dep=item.operFlt.airp;
    rcpt.airp_arv=item.airp_arv;
  };
  
  if (rcpt.kit_num==NoExists)
  {
    rcpt.aircode=base_tables.get("airlines").get_row("code", rcpt.airline).AsString("aircode");
    rcpt.form_type=NodeAsString("form_type",rcptNode);
    if (rcpt.form_type.empty())
      throw AstraLocale::UserException("MSG.RECEIPT_BLANK_TYPE_NOT_SET");
    if (NodeIsNULL("no",rcptNode) )
      //�ॢ�� � ��������� ����஬ ���⠭樨 (� �.�. ��ࢮ� �ॢ��)
      rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
    else
      rcpt.no=NodeAsFloat("no",rcptNode);
  }
  else
  {
    if (!(rcpt.kit_num>=0 && rcpt.kit_num<(int)rcpt.kit_items.size()))
      throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
    const TBagReceiptKitItem &item=rcpt.kit_items.at(rcpt.kit_num);
    rcpt.aircode=item.aircode;
    rcpt.form_type=item.form_type;
    rcpt.no=item.no;
  };
  
  if (rcpt.kit_id==NoExists)
  {
    TTagLang tag_lang;
    //�� �� ��⠭�����
    tag_lang.Init(Qry.FieldAsInteger("point_dep"),
                  Qry.FieldAsInteger("point_arv"),
                  route,
                  prnParams.pr_lat);
    rcpt.is_inter=tag_lang.IsInter();  //�����, �� ���樠�����㥬 ���砫�
    rcpt.desk_lang=tag_lang.GetLang();
    
    if (NodeIsNULL("no",rcptNode) )
      //�ॢ�� � ��������� ����஬ ���⠭樨 (� �.�. ��ࢮ� �ॢ��)
      rcpt.pax_name=transliter(NodeAsString("pax_name",rcptNode),1,tag_lang.GetLang() != AstraLocale::LANG_RU);
    else
      rcpt.pax_name=NodeAsString("pax_name",rcptNode);

    rcpt.pax_doc=NodeAsString("pax_doc",rcptNode);
    rcpt.bag_name=NodeAsString("bag_name",rcptNode);
    rcpt.tickets=NodeAsString("tickets",rcptNode);
    if (rcpt.kit_num!=NoExists)
      rcpt.prev_no=GetKitPrevNoStr(rcpt.kit_items).substr(0,50);
    else
      rcpt.prev_no=NodeAsString("prev_no",rcptNode);
    
    rcpt.service_type=NodeAsInteger("service_type",rcptNode);
    if (!NodeIsNULL("bag_type",rcptNode))
      rcpt.bag_type=NodeAsInteger("bag_type",rcptNode);
    else
      rcpt.bag_type=-1;

    if (rcpt.service_type!=3)
    {
      rcpt.ex_amount=NodeAsInteger("ex_amount",rcptNode);
      rcpt.ex_weight=NodeAsInteger("ex_weight",rcptNode);
      rcpt.value_tax=-1.0;
    }
    else
    {
      rcpt.ex_amount=-1;
      rcpt.ex_weight=-1;
      rcpt.value_tax=NodeAsFloat("value_tax",rcptNode);
    };
    rcpt.rate=NodeAsFloat("rate",rcptNode);
    rcpt.rate_cur=NodeAsString("rate_cur",rcptNode);
    rcpt.pay_rate_cur=NodeAsString("pay_rate_cur",rcptNode);
    if (rcpt.rate_cur!=rcpt.pay_rate_cur)
    {
      rcpt.exch_rate=NodeAsInteger("exch_rate",rcptNode);
      rcpt.exch_pay_rate=NodeAsFloat("exch_pay_rate",rcptNode);
    }
    else
    {
      rcpt.exch_rate=-1;
      rcpt.exch_pay_rate=-1.0;
    };
    //��� ������
    rcpt.pay_types.clear();
    unsigned int none_count=0;
    unsigned int cash_count=0;

    xmlNodePtr node=NodeAsNode("pay_types",rcptNode)->children;
    for(;node!=NULL;node=node->next)
    {
      TBagPayType payType;
      payType.pay_type=NodeAsString("pay_type",node);
      payType.pay_rate_sum=NodeAsFloat("pay_rate_sum",node);
      payType.extra=NodeAsString("extra",node);
      rcpt.pay_types.push_back(payType);

      if (payType.pay_type==NONE_PAY_TYPE_ID) none_count++;
      if (payType.pay_type==CASH_PAY_TYPE_ID) cash_count++;
    };
    if (rcpt.pay_types.size() > 2)
      throw AstraLocale::UserException("MSG.PAY_TYPE.NO_MORE_2");
    if (none_count > 1)
      throw AstraLocale::UserException("MSG.PAY_TYPE.ONLY_ONCE", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,NONE_PAY_TYPE_ID)));
    if (none_count > 0 && rcpt.pay_types.size() > none_count)
      throw AstraLocale::UserException("MSG.PAY_TYPE.NO_MIX", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,NONE_PAY_TYPE_ID)));
    if (cash_count > 1)
      throw AstraLocale::UserException("MSG.PAY_TYPE.ONLY_ONCE", LParams() << LParam("pay_type", ElemIdToCodeNative(etPayType,CASH_PAY_TYPE_ID)));


    if (rcpt.pay_types.empty())
    {
      //�� ������ ��� ������ - ᪮॥ �ᥣ� ��ࢮ� �ॢ��
      TBagPayType payType;
      payType.pay_type=CASH_PAY_TYPE_ID;
      payType.pay_rate_sum=CalcPayRateSum(rcpt);
      payType.extra="";
      rcpt.pay_types.push_back(payType);
    };
  };

  rcpt.remarks=""; //�ନ����� ������

  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);

  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";
};

void PaymentInterface::PutReceiptFields(const TBagReceipt &rcpt, bool pr_lat, xmlNodePtr node)
{
  if (node==NULL) return;

  PrintDataParser parser(rcpt, pr_lat);

  //�ନ�㥬 � �⢥� ��ࠧ ⥫��ࠬ��
  xmlNodePtr fieldsNode=NewTextChild(node,"fields");
  NewTextChild(fieldsNode,"pax_doc",         parser.pts.get_tag_no_err(TAG::PAX_DOC));
  NewTextChild(fieldsNode,"pax_name",        parser.pts.get_tag_no_err(TAG::PAX_NAME));
  NewTextChild(fieldsNode,"tickets",         parser.pts.get_tag_no_err(TAG::TICKETS));
  NewTextChild(fieldsNode,"prev_no",         parser.pts.get_tag_no_err(TAG::PREV_NO));
  NewTextChild(fieldsNode,"aircode",         parser.pts.get_tag_no_err(TAG::AIRCODE));
  NewTextChild(fieldsNode,"issue_place1",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE1));
  NewTextChild(fieldsNode,"issue_place2",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE2));
  NewTextChild(fieldsNode,"issue_place3",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE3));
  NewTextChild(fieldsNode,"issue_place4",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE4));
  NewTextChild(fieldsNode,"issue_place5",    parser.pts.get_tag_no_err(TAG::ISSUE_PLACE5));
  NewTextChild(fieldsNode,"SkiBT",           parser.pts.get_tag_no_err(TAG::SKI_BT));
  NewTextChild(fieldsNode,"GolfBT",          parser.pts.get_tag_no_err(TAG::GOLF_BT));
  NewTextChild(fieldsNode,"PetBT",           parser.pts.get_tag_no_err(TAG::PET_BT));
  NewTextChild(fieldsNode,"BulkyBT",         parser.pts.get_tag_no_err(TAG::BULKY_BT));
  NewTextChild(fieldsNode,"BulkyBTLetter",   parser.pts.get_tag_no_err(TAG::BULKY_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBT",         parser.pts.get_tag_no_err(TAG::OTHER_BT));
  NewTextChild(fieldsNode,"ValueBT",         parser.pts.get_tag_no_err(TAG::VALUE_BT));
  NewTextChild(fieldsNode,"ValueBTLetter",   parser.pts.get_tag_no_err(TAG::VALUE_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBTLetter",   parser.pts.get_tag_no_err(TAG::OTHER_BT_LETTER));
  NewTextChild(fieldsNode,"service_type",    parser.pts.get_tag_no_err(TAG::SERVICE_TYPE));
  NewTextChild(fieldsNode,"bag_name",        parser.pts.get_tag_no_err(TAG::BAG_NAME));
  NewTextChild(fieldsNode,"amount_letters",  parser.pts.get_tag_no_err(TAG::AMOUNT_LETTERS));
  NewTextChild(fieldsNode,"amount_figures",  parser.pts.get_tag_no_err(TAG::AMOUNT_FIGURES));
  NewTextChild(fieldsNode,"currency",        parser.pts.get_tag_no_err(TAG::CURRENCY));
  NewTextChild(fieldsNode,"issue_date",      parser.pts.get_tag_no_err(TAG::ISSUE_DATE, "ddmmmyy"));
  NewTextChild(fieldsNode,"to",              parser.pts.get_tag_no_err(TAG::TO));
  NewTextChild(fieldsNode,"remarks1",        parser.pts.get_tag_no_err(TAG::REMARKS1));
  NewTextChild(fieldsNode,"remarks2",        parser.pts.get_tag_no_err(TAG::REMARKS2));
  NewTextChild(fieldsNode,"exchange_rate",   parser.pts.get_tag_no_err(TAG::EXCHANGE_RATE));
  NewTextChild(fieldsNode,"total",           parser.pts.get_tag_no_err(TAG::TOTAL));
  NewTextChild(fieldsNode,"airline",         parser.pts.get_tag_no_err(TAG::AIRLINE));
  NewTextChild(fieldsNode,"airline_code",    parser.pts.get_tag_no_err(TAG::AIRLINE_CODE));
  NewTextChild(fieldsNode,"point_dep",       parser.pts.get_tag_no_err(TAG::POINT_DEP));
  NewTextChild(fieldsNode,"point_arv",       parser.pts.get_tag_no_err(TAG::POINT_ARV));
  NewTextChild(fieldsNode,"rate",            parser.pts.get_tag_no_err(TAG::RATE));
  NewTextChild(fieldsNode,"charge",          parser.pts.get_tag_no_err(TAG::CHARGE));
  NewTextChild(fieldsNode,"ex_weight",       parser.pts.get_tag_no_err(TAG::EX_WEIGHT));
  NewTextChild(fieldsNode,"pay_form",        parser.pts.get_tag_no_err(TAG::PAY_FORM));

  if (rcpt.no!=-1.0)
  {
    ostringstream no;
    no << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
    NewTextChild(fieldsNode,"no",no.str());
  }
  else
  {
    NewTextChild(fieldsNode,"no");
  };
  NewTextChild(fieldsNode,"annul_str");
};

void PaymentInterface::PutReceiptFields(int id, bool pr_lat, xmlNodePtr node)
{
  if (node==NULL) return;

  TBagReceipt rcpt;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT * FROM bag_receipts WHERE receipt_id=:id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  if (Qry.Eof)
      throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");

  GetReceiptFromDB(Qry,rcpt);
  PutReceiptToXML(rcpt,id,pr_lat,node);
  PutReceiptFields(rcpt,pr_lat,node);

  string status=Qry.FieldAsString("status");

  if (rcpt.annul_date!=NoExists && !rcpt.annul_desk.empty())
  {
    TDateTime annul_date_local = UTCToLocal(rcpt.annul_date, CityTZRegion(DeskCity(rcpt.annul_desk)));
    ostringstream annul_str;
    
    if (status=="�")
      annul_str << getLocaleText("MSG.RECEIPT.REPLACEMENT") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    if (status=="�")
      annul_str << getLocaleText("MSG.RECEIPT.REFUND") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    ReplaceTextChild(NodeAsNode("fields",node),"annul_str",annul_str.str());
  };
};

void PaymentInterface::ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr rcptNode=NewTextChild(resNode,"receipt");
  
  TPrnParams prnParams(reqNode);

  if (GetNode("receipt/id",reqNode)==NULL)
  {
  
    if (!TReqInfo::Instance()->desk.compatible(BAG_RCPT_KITS_VERSION) &&
        NodeAsNode("receipt",reqNode)->children==NULL)
      throw UserException("MSG.BEFORE_RECEIPT_PRINT_CLOSE_PAYMENT_FORMS");
    //��� ��� �믮������ �� �뢮�� �� �� �����⠭��� ���⠭樨
    TBagReceipt rcpt;
    GetReceiptFromXML(reqNode,rcpt);
    PutReceiptToXML(rcpt,-1,prnParams.pr_lat,rcptNode);
    PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode);
  }
  else
  {
    //��� ��� �믮������ �� �뢮�� �����⠭��� ���⠭樨
    PutReceiptFields(NodeAsInteger("receipt/id",reqNode),prnParams.pr_lat,rcptNode);
  };
};

void PaymentInterface::ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TBagReceipt rcpt;
  int rcpt_id=NodeAsInteger("receipt/id",reqNode);

  if (!GetReceiptFromDB(rcpt_id,rcpt))
    throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");

  if (rcpt.kit_id!=NoExists)
    //�� �।�ᬮ�७� ��楤�� ������ ������ ��� ���⠭権 �� ��������
    throw AstraLocale::UserException("MSG.RECEIPT_REPLACEMENT_DENIAL");

  TPrnParams prnParams(reqNode);

  PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,NewTextChild(resNode,"receipt"));

  rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);
  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";

  xmlNodePtr rcptNode=NewTextChild(resNode,"new_receipt");
  PutReceiptToXML(rcpt,-1,prnParams.pr_lat,rcptNode);
  PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode); //��ࠧ ���⠭樨
};

void PaymentInterface::AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TBagReceipt rcpt;
  TQuery Qry(&OraSession);

  int point_dep=NoExists;
  int grp_id=NoExists;
  if (GetNode("point_dep",reqNode)!=NULL)
  {
    //���⠭�� �ਢ易�� � ��㯯�
    point_dep=NodeAsInteger("point_dep",reqNode);
    grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);
  };
  
  TPrnParams prnParams(reqNode);
  
  vector<int> rcpt_ids;
  xmlNodePtr rcptsNode;
  string new_status;
  if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
  {
    new_status=NodeAsInteger("replacement",reqNode)!=0?"�":"�";
    xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
    for(;node!=NULL;node=node->next)
      rcpt_ids.push_back(NodeAsInteger(node));
    rcptsNode=NewTextChild(resNode,"receipts");
  }
  else
  {
    new_status="�";
    rcpt_ids.push_back(NodeAsInteger("receipt/id",reqNode));
    rcptsNode=resNode;
  };
  
  for(vector<int>::const_iterator rcpt_id=rcpt_ids.begin();rcpt_id!=rcpt_ids.end();++rcpt_id)
  {
    if (GetNode("point_dep",reqNode)==NULL)
    {
      //���⠭�� �� �ਢ易�� � ��㯯�
      Qry.Clear();
      Qry.SQLText="SELECT point_id FROM bag_receipts WHERE receipt_id=:receipt_id";
      Qry.CreateVariable("receipt_id",otInteger,*rcpt_id);
      Qry.Execute();
      if (Qry.Eof)
      {
        if (rcpt_ids.size()>1)
          throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
        else
          throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
      };
      point_dep=Qry.FieldAsInteger("point_id");
    };

    Qry.Clear();
    Qry.SQLText=
        "UPDATE bag_receipts "
        "SET status=:new_status,annul_date=:annul_date,annul_user_id=:user_id,annul_desk=:desk "
        "WHERE receipt_id=:receipt_id AND status=:old_status";
    Qry.CreateVariable("receipt_id",otInteger,*rcpt_id);
    Qry.CreateVariable("old_status",otString,"�");
    Qry.CreateVariable("new_status",otString,new_status);
    Qry.CreateVariable("annul_date",otDate,NowUTC());
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
    {
      if (rcpt_ids.size()>1)
        throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_CHANGED.REFRESH_DATA");
      else
        throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_CHANGED.REFRESH_DATA");
    };

    if (!GetReceiptFromDB(*rcpt_id,rcpt))
    {
      if (rcpt_ids.size()>1)
          throw AstraLocale::UserException("MSG.RECEIPTS_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
        else
          throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
    };

    PutReceiptToXML(rcpt,*rcpt_id,prnParams.pr_lat,NewTextChild(rcptsNode,"receipt"));

    ostringstream logmsg;
    logmsg << "���⠭�� " << rcpt.form_type << " "
           << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no
           << " ���㫨஢���";
    reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,
                                   point_dep!=NoExists?point_dep:0,
                                   0,
                                   grp_id!=NoExists?grp_id:0);
  };
};

void PaymentInterface::PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    TQuery Qry(&OraSession);
    
    TPrnParams prnParams(reqNode);

    TBagReceipt rcpt;
    int rcpt_id;
    ostringstream logmsg;
    xmlNodePtr rcptNode;
    if (GetNode("receipt/no",reqNode)!=NULL)
    {
        if (GetNode("receipt/id",reqNode)==NULL)
        {
            //����� ����� ���⠭樨
            GetReceiptFromXML(reqNode,rcpt);
            rcptNode=NewTextChild(resNode,"receipt");
            logmsg << "���⠭�� " << rcpt.form_type << " "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no
                   << " �����⠭�";
        }
        else
        {
            //������ ������
            rcpt_id=NodeAsInteger("receipt/id",reqNode);

            Qry.Clear();
            Qry.SQLText=
                "UPDATE bag_receipts "
                "SET status=:new_status,annul_date=:annul_date,annul_user_id=:user_id,annul_desk=:desk "
                "WHERE receipt_id=:receipt_id AND status=:old_status";
            Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
            Qry.CreateVariable("old_status",otString,"�");
            Qry.CreateVariable("new_status",otString,"�");
            Qry.CreateVariable("annul_date",otDate,NowUTC());
            Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
            Qry.CreateVariable("desk",otString,reqInfo->desk.code);
            Qry.Execute();
            if (Qry.RowsProcessed()<=0)
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_CHANGED.REFRESH_DATA");

            if (!GetReceiptFromDB(rcpt_id,rcpt))
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");
                
            if (rcpt.kit_id!=NoExists)
              //�� �।�ᬮ�७� ��楤�� ������ ������ ��� ���⠭権 �� ��������
              throw AstraLocale::UserException("MSG.RECEIPT_REPLACEMENT_DENIAL");

            PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,NewTextChild(resNode,"receipt"));

            double old_no=rcpt.no;
            rcpt.no=NodeAsFloat("receipt/no",reqNode);
            rcpt.issue_date=rcpt.annul_date;
            rcpt.issue_desk=reqInfo->desk.code;
            rcpt.issue_place=get_validator(rcpt, prnParams.pr_lat);
            rcpt.annul_date=NoExists;
            rcpt.annul_desk="";

            rcptNode=NewTextChild(resNode,"new_receipt");
            logmsg << "������ ������ " << rcpt.form_type << ": "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << old_no << " �� "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
        };

        rcpt_id=PutReceiptToDB(rcpt,point_dep,grp_id);  //������ ���⠭樨 � ����
        createSofiFileDATA( rcpt_id );
        //�뢮�
        if (!GetReceiptFromDB(rcpt_id,rcpt)) //�����⠥� �� ���� ��-�� kit_id
          throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        PutReceiptToXML(rcpt,rcpt_id,prnParams.pr_lat,rcptNode); //���⠭��
        PutReceiptFields(rcpt,prnParams.pr_lat,rcptNode); //��ࠧ ���⠭樨

        double next_no=GetNextNo(rcpt.form_type, rcpt.no);
        //�����塞 ����� ������ � ��窥
        Qry.Clear();
        Qry.SQLText=
            "BEGIN "
            "  IF :next_no IS NOT NULL THEN "
            "    UPDATE form_packs SET curr_no=:next_no "
            "    WHERE user_id=:user_id AND type=:type; "
            "    IF SQL%NOTFOUND THEN "
            "      INSERT INTO form_packs(user_id,curr_no,type) "
            "      VALUES(:user_id,:next_no,:type); "
            "    END IF; "
            "  ELSE "
            "    DELETE FROM form_packs WHERE user_id=:user_id AND type=:type; "
            "  END IF; "
            "END;";
        Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
        Qry.CreateVariable("type",otString,rcpt.form_type);
        if (next_no!=-1.0)
          Qry.CreateVariable("next_no",otFloat,next_no);
        else
          Qry.CreateVariable("next_no",otFloat,FNull);
        Qry.Execute();
    }
    else
    {
        //����� ����
        rcpt_id=NodeAsInteger("receipt/id",reqNode);
        if (!GetReceiptFromDB(rcpt_id,rcpt))
          throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        rcptNode=NewTextChild(resNode,"receipt");
        logmsg << "����� ���� ���⠭樨 " << rcpt.form_type << " "
               << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
    };

    //��᫥����⥫쭮��� ��� �ਭ��
    PrintDataParser parser(rcpt, prnParams.pr_lat);
    string data;
    bool hex;
    PrintInterface::GetPrintDataBR(
            rcpt.form_type,
            parser,
            data,
            hex,
            reqNode
            ); //��᫥����⥫쭮��� ��� �ਭ��
    SetProp( NewTextChild(rcptNode, "form", data), "hex", (int)hex);
    reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
};
