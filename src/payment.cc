#include "payment.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "checkin.h"
#include "print.h"
#include "tripinfo.h"
#include "oralib.h"
#include "astra_service.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

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

  xmlNodePtr dataNode=GetNode("data",resNode);
  if (dataNode==NULL)
    dataNode = NewTextChild(resNode, "data");

  TQuery Qry(&OraSession);

  int pax_id=-1;
  int grp_id=-1;
  bool pr_unaccomp=false;
  bool pr_annul_rcpt=false;
  //сначала попробуем получить pax_id
  if (search_type==searchByReceiptNo)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT receipt_id,annul_date,point_id,grp_id,ckin.get_main_pax_id(grp_id) AS pax_id "
      "FROM bag_receipts WHERE no=:no";
    Qry.CreateVariable("no",otFloat,NodeAsFloat("receipt_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND");
    pr_annul_rcpt=!Qry.FieldIsNULL("annul_date");
    if (Qry.FieldIsNULL("grp_id"))
    {
      //NULL если группа разрегистрирована по ошибке агента
      //тогда выводим только квитанцию
      LoadReceipts(Qry.FieldAsInteger("receipt_id"),false,dataNode);
    }
    else
    {
      grp_id=Qry.FieldAsInteger("grp_id");
      if (!Qry.FieldIsNULL("pax_id"))
        pax_id=Qry.FieldAsInteger("pax_id");
      else
        //NULL если несопровождаемый багаж
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
      "SELECT grp_id,ckin.get_main_pax_id(grp_id) AS pax_id "
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
      //NULL если несопровождаемый багаж
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
        "SELECT pax_grp.grp_id,point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       report.get_last_trfer_airp(pax_grp.grp_id) AS last_trfer_airp, "
        "       class,bag_refuse,pax_grp.tid, "
        "       pax.reg_no, "
        "       RTRIM(pax.surname||' '||pax.name) AS pax_name, "
        "       document AS pax_doc "
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
        "SELECT pax_grp.grp_id,point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       report.get_last_trfer_airp(pax_grp.grp_id) AS last_trfer_airp, "
        "       class,bag_refuse,pax_grp.tid, "
        "       NULL AS reg_no, "
        "       NULL AS pax_name, "
        "       NULL AS pax_doc "
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
        AstraLocale::showErrorMessage(msg); //в любом случае показываем аннулированную квитанцию
    };
  };

  if (search_type==searchByReceiptNo && grp_id==-1) return;

  grp_id=Qry.FieldAsInteger("grp_id");
  NewTextChild(dataNode,"grp_id",grp_id);
  NewTextChild(dataNode,"point_dep",Qry.FieldAsInteger("point_dep"));
  NewTextChild(dataNode,"airp_dep",Qry.FieldAsString("airp_dep"));
  NewTextChild(dataNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(dataNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(dataNode,"last_trfer_airp",Qry.FieldAsString("last_trfer_airp"));
  try
  {
    NewTextChild(dataNode,"last_trfer_city",
                 base_tables.get("airps").get_row("code",Qry.FieldAsString("last_trfer_airp")).AsString("city"));
  }
  catch(EBaseTableError)
  {
    NewTextChild(dataNode,"last_trfer_city");
  };
  NewTextChild(dataNode,"class",Qry.FieldAsString("class"));
  NewTextChild(dataNode,"pr_refuse",(int)(Qry.FieldAsInteger("bag_refuse")!=0));
  NewTextChild(dataNode,"reg_no",Qry.FieldAsInteger("reg_no"));
  NewTextChild(dataNode,"pax_name",Qry.FieldAsString("pax_name"));
  NewTextChild(dataNode,"pax_doc",Qry.FieldAsString("pax_doc"));
  NewTextChild(dataNode,"tid",Qry.FieldAsInteger("tid"));

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
    "SELECT airline,flt_no,suffix,scd,airp_dep,pr_mark_norms "
    "FROM market_flt WHERE grp_id=:grp_id ";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    xmlNodePtr markFltNode=NewTextChild(dataNode,"mark_flight");
    NewTextChild(markFltNode,"airline",Qry.FieldAsString("airline"));
    NewTextChild(markFltNode,"flt_no",Qry.FieldAsInteger("flt_no"));
    NewTextChild(markFltNode,"suffix",Qry.FieldAsString("suffix"));
    NewTextChild(markFltNode,"scd",DateTimeToStr(Qry.FieldAsDateTime("scd")));
    NewTextChild(markFltNode,"airp_dep",Qry.FieldAsString("airp_dep"));
    NewTextChild(markFltNode,"pr_mark_norms",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
    NewTextChild(markFltNode,"pr_mark_rates",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
  };

  CheckInInterface::LoadBag(grp_id,dataNode);
  CheckInInterface::LoadPaidBag(grp_id,dataNode);
  LoadReceipts(grp_id,true,dataNode);

  //ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
};

void PaymentInterface::LoadReceipts(int id, bool pr_grp, xmlNodePtr dataNode)
{
  if (dataNode==NULL) return;

  TQuery Qry(&OraSession);
  if (pr_grp)
  {
    //квитанции предоплаты
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
    GetReceipt(Qry,rcpt);
    PutReceipt(rcpt,rcpt_id,NewTextChild(node,"receipt"));
  };
};

int PaymentInterface::LockAndUpdTid(int point_dep, int grp_id, int tid)
{
  TQuery Qry(&OraSession);
  //лочим рейс
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id,tid__seq.nextval AS tid "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  int new_tid=Qry.FieldAsInteger("tid");

  Qry.Clear();
  Qry.SQLText=
    "UPDATE pax_grp "
    "SET tid=tid__seq.currval "
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

    CheckInInterface::SaveBag(point_dep,grp_id,ASTRA::NoExists,reqNode);
    CheckInInterface::SavePaidBag(grp_id,reqNode);

    TReqInfo::Instance()->MsgToLog(
            "Данные по багажным тарифам и ценному багажу сохранены.",
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
                "  SELECT id__seq.nextval INTO :receipt_id FROM dual; "
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
          logmsg << "Квитанция предоплаты " << aircode << " " << no << " введена. ";
        }
        else
        {
          if (aircode==old_aircode && no==old_no)
          {
            logmsg << "Квитанция предоплаты " << aircode << " " << no << " изменена. ";
          }
          else
          {
            logmsg << "Квитанция предоплаты " << old_aircode << " " << old_no << " удалена";
            reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
            logmsg.str("");
            logmsg << "Квитанция предоплаты " << aircode << " " << no << " введена. ";
          };
        };
        if (!NodeIsNULLFast("ex_weight",node,true))
        {
          if (!NodeIsNULLFast("bag_type",node))
            logmsg << "Тип багажа: " << NodeAsIntegerFast("bag_type",node)
                   << ", вес багажа: " << NodeAsIntegerFast("ex_weight",node) << " кг.";
          else
            logmsg << "Вес багажа: " << NodeAsIntegerFast("ex_weight",node) << " кг.";
        };
        if (!NodeIsNULLFast("value",node,true))
        {
          logmsg << "Ценность багажа: "
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
        logmsg << "Квитанция предоплаты " << old_aircode << " " << old_no << " удалена";
        reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
    };
}

//из базы в структуру
bool PaymentInterface::GetReceipt(int id, TBagReceipt &rcpt)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT * FROM bag_receipts WHERE receipt_id=:id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();

  return GetReceipt(Qry,rcpt);
};

//из базы в структуру
bool PaymentInterface::GetReceipt(TQuery &Qry, TBagReceipt &rcpt)
{
  if (Qry.Eof) return false;
  if(rcpt.prnParams.pr_lat == NoExists) ProgError(STDLOG, "Udefined rcpt.prnParams.pr_lat");
  rcpt.tag_lang.Init(
          rcpt.prnParams.pr_lat, 
          Qry.FieldAsInteger("is_inter")!=0,
          Qry.FieldAsString("desk_lang")
          );
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

  //формы оплаты
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
  return true;
};

//из структуры в базу
int PaymentInterface::PutReceipt(TBagReceipt &rcpt, int point_id, int grp_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT id__seq.nextval,SYSTEM.UTCSYSDATE INTO :receipt_id,:issue_date FROM dual; "
    "  INSERT INTO bag_receipts "
    "        (receipt_id,point_id,grp_id,status,is_inter,desk_lang,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name, "
    "         tickets,prev_no,airline,aircode,flt_no,suffix,airp_dep,airp_arv,ex_amount,ex_weight,value_tax, "
    "         rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,remarks, "
    "         issue_date,issue_place,issue_user_id,issue_desk) "
    "  VALUES(:receipt_id,:point_id,:grp_id,:status,:is_inter,:desk_lang,:form_type,:no,:pax_name,:pax_doc,:service_type,:bag_type,:bag_name, "
    "         :tickets,:prev_no,:airline,:aircode,:flt_no,:suffix,:airp_dep,:airp_arv,:ex_amount,:ex_weight,:value_tax, "
    "         :rate,:rate_cur,:exch_rate,:exch_pay_rate,:pay_rate_cur,:remarks, "
    "         :issue_date,:issue_place,:issue_user_id,:issue_desk); "
    "END;";
  Qry.CreateVariable("receipt_id",otInteger,FNull);
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("status",otString,"П");
  Qry.CreateVariable("is_inter",otInteger,(int)rcpt.tag_lang.IsInter());
  Qry.CreateVariable("desk_lang",otString,TReqInfo::Instance()->desk.lang);
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
  for(vector<TBagPayType>::iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();i++,num++)
  {
    Qry.SetVariable("num",num);
    Qry.SetVariable("pay_type",i->pay_type);
    Qry.SetVariable("pay_rate_sum",i->pay_rate_sum);
    Qry.SetVariable("extra",i->extra);
    Qry.Execute();
  };
  return receipt_id;
};


//из структуры в XML
void PaymentInterface::PutReceipt(TBagReceipt &rcpt, int rcpt_id, xmlNodePtr rcptNode)
{
  if (rcptNode==NULL) return;

  if (rcpt_id!=-1)
    NewTextChild(rcptNode,"id",rcpt_id);
  NewTextChild(rcptNode,"pr_lat",(int)rcpt.tag_lang.IsInter());
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
  for(vector<TBagPayType>::iterator i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();i++)
  {
    xmlNodePtr node=NewTextChild(payTypesNode,"pay_type");
    NewTextChild(node,"pay_type",i->pay_type);
    NewTextChild(node,"pay_rate_sum",i->pay_rate_sum);
    NewTextChild(node,"extra",i->extra,"");
  };
};

double PaymentInterface::GetCurrNo(int user_id, string form_type)
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

//из XML в структуру
void PaymentInterface::GetReceipt(xmlNodePtr reqNode, TBagReceipt &rcpt)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr rcptNode=NodeAsNode("receipt",reqNode);

  int grp_id=NodeAsInteger("grp_id",reqNode);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.airline,points.flt_no,points.suffix, "
    "       pax_grp.airp_dep,pax_grp.airp_arv, "
    "       ckin.get_main_pax_id(pax_grp.grp_id) AS pax_id, "
    "       point_dep, "
    "       point_arv "
    "FROM points,pax_grp "
    "WHERE points.point_id=pax_grp.point_dep AND points.pr_del>=0 AND "
    "      pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof)
    throw AstraLocale::UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");

  rcpt.airline=Qry.FieldAsString("airline");
  rcpt.flt_no=Qry.FieldAsInteger("flt_no");
  rcpt.suffix=Qry.FieldAsString("suffix");
  rcpt.airp_dep=Qry.FieldAsString("airp_dep");
  if (GetNode("airp_arv",rcptNode)!=NULL)
    rcpt.airp_arv=NodeAsString("airp_arv",rcptNode);
  else
    rcpt.airp_arv=Qry.FieldAsString("airp_arv");
  rcpt.tag_lang.Init(
          Qry.FieldAsInteger("point_dep"),
          Qry.FieldAsInteger("point_arv"),
          NULL,
          rcpt.prnParams.pr_lat
          );

  //проверим, надо ли выводить в квитанцию коммерческий рейс
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,pr_mark_norms FROM market_flt WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (!Qry.Eof && Qry.FieldAsInteger("pr_mark_norms")!=0)
  {
    rcpt.airline=Qry.FieldAsString("airline");
    rcpt.flt_no=Qry.FieldAsInteger("flt_no");
    rcpt.suffix=Qry.FieldAsString("suffix");
  };

  rcpt.aircode=base_tables.get("airlines").get_row("code", rcpt.airline).AsString("aircode");
  rcpt.form_type=NodeAsString("form_type",rcptNode);
  if (rcpt.form_type.empty())
    throw AstraLocale::UserException("MSG.RECEIPT_BLANK_TYPE_NOT_SET");

  if (NodeIsNULL("no",rcptNode) )
  {
    //превью с незаданным номером квитанции (в т.ч. первое превью)
    rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
    rcpt.pax_name=transliter(NodeAsString("pax_name",rcptNode),1,rcpt.tag_lang.GetLang() != AstraLocale::LANG_RU);
  }
  else
  {
    rcpt.no=NodeAsFloat("no",rcptNode);
    rcpt.pax_name=NodeAsString("pax_name",rcptNode);
  };

  rcpt.pax_doc=NodeAsString("pax_doc",rcptNode);
  rcpt.bag_name=NodeAsString("bag_name",rcptNode);
  rcpt.tickets=NodeAsString("tickets",rcptNode);
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
  rcpt.remarks=""; //формируется налету

  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt);

  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";

  //формы оплаты
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
    //не заданы формы оплаты - скорее всего первое превью
    TBagPayType payType;
    payType.pay_type=CASH_PAY_TYPE_ID;
    payType.pay_rate_sum=CalcPayRateSum(rcpt);
    payType.extra="";
    rcpt.pay_types.push_back(payType);
  };

};

void PaymentInterface::PutReceiptFields(TBagReceipt &rcpt, xmlNodePtr node)
{
  if (node==NULL) return;

  PrintDataParser parser(rcpt);

  //формируем в ответ образ телеграммы
  xmlNodePtr fieldsNode=NewTextChild(node,"fields");
  NewTextChild(fieldsNode,"pax_doc",         parser.pts.get_tag(TAG::PAX_DOC));
  NewTextChild(fieldsNode,"pax_name",        parser.pts.get_tag(TAG::PAX_NAME));
  NewTextChild(fieldsNode,"tickets",         parser.pts.get_tag(TAG::TICKETS));
  NewTextChild(fieldsNode,"prev_no",         parser.pts.get_tag(TAG::PREV_NO));
  NewTextChild(fieldsNode,"aircode",         parser.pts.get_tag(TAG::AIRCODE));
  NewTextChild(fieldsNode,"issue_place1",    parser.pts.get_tag(TAG::ISSUE_PLACE1));
  NewTextChild(fieldsNode,"issue_place2",    parser.pts.get_tag(TAG::ISSUE_PLACE2));
  NewTextChild(fieldsNode,"issue_place3",    parser.pts.get_tag(TAG::ISSUE_PLACE3));
  NewTextChild(fieldsNode,"issue_place4",    parser.pts.get_tag(TAG::ISSUE_PLACE4));
  NewTextChild(fieldsNode,"issue_place5",    parser.pts.get_tag(TAG::ISSUE_PLACE5));
  NewTextChild(fieldsNode,"SkiBT",           parser.pts.get_tag(TAG::SKI_BT));
  NewTextChild(fieldsNode,"GolfBT",          parser.pts.get_tag(TAG::GOLF_BT));
  NewTextChild(fieldsNode,"PetBT",           parser.pts.get_tag(TAG::PET_BT));
  NewTextChild(fieldsNode,"BulkyBT",         parser.pts.get_tag(TAG::BULKY_BT));
  NewTextChild(fieldsNode,"BulkyBTLetter",   parser.pts.get_tag(TAG::BULKY_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBT",         parser.pts.get_tag(TAG::OTHER_BT));
  NewTextChild(fieldsNode,"ValueBT",         parser.pts.get_tag(TAG::VALUE_BT));
  NewTextChild(fieldsNode,"ValueBTLetter",   parser.pts.get_tag(TAG::VALUE_BT_LETTER));
  NewTextChild(fieldsNode,"OtherBTLetter",   parser.pts.get_tag(TAG::OTHER_BT_LETTER));
  NewTextChild(fieldsNode,"service_type",    parser.pts.get_tag(TAG::SERVICE_TYPE));
  NewTextChild(fieldsNode,"bag_name",        parser.pts.get_tag(TAG::BAG_NAME));
  NewTextChild(fieldsNode,"amount_letters",  parser.pts.get_tag(TAG::AMOUNT_LETTERS));
  NewTextChild(fieldsNode,"amount_figures",  parser.pts.get_tag(TAG::AMOUNT_FIGURES));
  NewTextChild(fieldsNode,"currency",        parser.pts.get_tag(TAG::CURRENCY));
  NewTextChild(fieldsNode,"issue_date",      parser.pts.get_tag(TAG::ISSUE_DATE, "ddmmmyy"));
  NewTextChild(fieldsNode,"to",              parser.pts.get_tag(TAG::TO));
  NewTextChild(fieldsNode,"remarks1",        parser.pts.get_tag(TAG::REMARKS1));
  NewTextChild(fieldsNode,"remarks2",        parser.pts.get_tag(TAG::REMARKS2));
  NewTextChild(fieldsNode,"exchange_rate",   parser.pts.get_tag(TAG::EXCHANGE_RATE));
  NewTextChild(fieldsNode,"total",           parser.pts.get_tag(TAG::TOTAL));
  NewTextChild(fieldsNode,"airline",         parser.pts.get_tag(TAG::AIRLINE));
  NewTextChild(fieldsNode,"airline_code",    parser.pts.get_tag(TAG::AIRLINE_CODE));
  NewTextChild(fieldsNode,"point_dep",       parser.pts.get_tag(TAG::POINT_DEP));
  NewTextChild(fieldsNode,"point_arv",       parser.pts.get_tag(TAG::POINT_ARV));
  NewTextChild(fieldsNode,"rate",            parser.pts.get_tag(TAG::RATE));
  NewTextChild(fieldsNode,"charge",          parser.pts.get_tag(TAG::CHARGE));
  NewTextChild(fieldsNode,"ex_weight",       parser.pts.get_tag(TAG::EX_WEIGHT));
  NewTextChild(fieldsNode,"pay_form",        parser.pts.get_tag(TAG::PAY_FORM));

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

void PaymentInterface::PutReceiptFields(int id, xmlNodePtr node)
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

  GetReceipt(Qry,rcpt);
  PutReceipt(rcpt,id,node);
  PutReceiptFields(rcpt,node);

  string status=Qry.FieldAsString("status");

  if (rcpt.annul_date!=NoExists && !rcpt.annul_desk.empty())
  {
    TDateTime annul_date_local = UTCToLocal(rcpt.annul_date, CityTZRegion(DeskCity(rcpt.annul_desk)));
    ostringstream annul_str;
    
    if (status=="З")
      annul_str << getLocaleText("MSG.RECEIPT.REPLACEMENT") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    if (status=="А")
      annul_str << getLocaleText("MSG.RECEIPT.REFUND") << " "
                << DateTimeToStr(annul_date_local, (string)"ddmmmyy", TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
    ReplaceTextChild(NodeAsNode("fields",node),"annul_str",annul_str.str());
  };
};

void PaymentInterface::ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr rcptNode=NewTextChild(resNode,"receipt");

  if (GetNode("receipt/id",reqNode)==NULL)
  {
    //этот код выполняется при выводе еще не напечатанной квитанции
    TBagReceipt rcpt;
    rcpt.prnParams.get_prn_params(reqNode);
    GetReceipt(reqNode,rcpt);
    PutReceipt(rcpt,-1,rcptNode);
    PutReceiptFields(rcpt,rcptNode);
  }
  else
  {
    //этот код выполняется при выводе напечатанной квитанции
    PutReceiptFields(NodeAsInteger("receipt/id",reqNode),rcptNode);
  };
};

void PaymentInterface::ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TBagReceipt rcpt;
  rcpt.prnParams.get_prn_params(reqNode);
  int rcpt_id=NodeAsInteger("receipt/id",reqNode);

  if (!GetReceipt(rcpt_id,rcpt))
    throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");

  PutReceipt(rcpt,rcpt_id,NewTextChild(resNode,"receipt"));

  rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator(rcpt);
  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";

  xmlNodePtr rcptNode=NewTextChild(resNode,"new_receipt");
  PutReceipt(rcpt,-1,rcptNode);
  PutReceiptFields(rcpt,rcptNode); //образ квитанции
};

void PaymentInterface::AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TBagReceipt rcpt;
    rcpt.prnParams.get_prn_params(reqNode);
    TQuery Qry(&OraSession);

    int rcpt_id=NodeAsInteger("receipt/id",reqNode);
    int point_dep=0;
    int grp_id=0;
    if (GetNode("point_dep",reqNode)!=NULL)
    {
      //квитанция привязана к группе
      point_dep=NodeAsInteger("point_dep",reqNode);
      grp_id=NodeAsInteger("grp_id",reqNode);
      int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
      NewTextChild(resNode,"tid",tid);
    }
    else
    {
      //квитанция не привязана к группе
      Qry.Clear();
      Qry.SQLText="SELECT point_id FROM bag_receipts WHERE receipt_id=:receipt_id";
      Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
      Qry.Execute();
      if (Qry.Eof)
          throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");
      point_dep=Qry.FieldAsInteger("point_id");
    };

    Qry.Clear();
    Qry.SQLText=
        "UPDATE bag_receipts "
        "SET status=:new_status,annul_date=:annul_date,annul_user_id=:user_id,annul_desk=:desk "
        "WHERE receipt_id=:receipt_id AND status=:old_status";
    Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
    Qry.CreateVariable("old_status",otString,"П");
    Qry.CreateVariable("new_status",otString,"А");
    Qry.CreateVariable("annul_date",otDate,NowUTC());
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
        throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_CHANGED.REFRESH_DATA");

    if (!GetReceipt(rcpt_id,rcpt))
        throw AstraLocale::UserException("MSG.RECEIPT_TO_ANNUL_NOT_FOUND.REFRESH_DATA");

    PutReceipt(rcpt,rcpt_id,NewTextChild(resNode,"receipt"));

    ostringstream logmsg;
    logmsg << "Квитанция " << rcpt.form_type << " "
           << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no
           << " аннулирована";
    reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
};

void PaymentInterface::PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();

    int point_dep=NodeAsInteger("point_dep",reqNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
    NewTextChild(resNode,"tid",tid);

    TQuery Qry(&OraSession);

    TBagReceipt rcpt;
    rcpt.prnParams.get_prn_params(reqNode);
    int rcpt_id;
    ostringstream logmsg;
    xmlNodePtr rcptNode;
    if (GetNode("receipt/no",reqNode)!=NULL)
    {
        if (GetNode("receipt/id",reqNode)==NULL)
        {
            //печать новой квитанции
            GetReceipt(reqNode,rcpt);
            rcptNode=NewTextChild(resNode,"receipt");
            logmsg << "Квитанция " << rcpt.form_type << " "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no
                   << " напечатана";
        }
        else
        {
            //замена бланка
            rcpt_id=NodeAsInteger("receipt/id",reqNode);

            Qry.Clear();
            Qry.SQLText=
                "UPDATE bag_receipts "
                "SET status=:new_status,annul_date=:annul_date,annul_user_id=:user_id,annul_desk=:desk "
                "WHERE receipt_id=:receipt_id AND status=:old_status";
            Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
            Qry.CreateVariable("old_status",otString,"П");
            Qry.CreateVariable("new_status",otString,"З");
            Qry.CreateVariable("annul_date",otDate,NowUTC());
            Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
            Qry.CreateVariable("desk",otString,reqInfo->desk.code);
            Qry.Execute();
            if (Qry.RowsProcessed()<=0)
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_CHANGED.REFRESH_DATA");

            if (!GetReceipt(rcpt_id,rcpt))
                throw AstraLocale::UserException("MSG.RECEIPT_TO_REPLACE_NOT_FOUND.REFRESH_DATA");

            PutReceipt(rcpt,rcpt_id,NewTextChild(resNode,"receipt"));

            double old_no=rcpt.no;
            rcpt.no=NodeAsFloat("receipt/no",reqNode);
            rcpt.issue_date=rcpt.annul_date;
            rcpt.issue_desk=reqInfo->desk.code;
            rcpt.issue_place=get_validator(rcpt);
            rcpt.annul_date=NoExists;
            rcpt.annul_desk="";

            rcptNode=NewTextChild(resNode,"new_receipt");
            logmsg << "Замена бланка " << rcpt.form_type << ": "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << old_no << " на "
                   << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
        };

        rcpt_id=PutReceipt(rcpt,point_dep,grp_id);  //запись квитанции в базу
        createSofiFileDATA( rcpt_id );
        //вывод
        PutReceipt(rcpt,rcpt_id,rcptNode); //квитанция
        PutReceiptFields(rcpt,rcptNode); //образ квитанции

        //изменяем номер бланка в пачке
        Qry.Clear();
        Qry.SQLText=
            "BEGIN "
            "  UPDATE form_packs SET curr_no=:curr_no+1 "
            "  WHERE user_id=:user_id AND type=:type; "
            "  IF SQL%NOTFOUND THEN "
            "    INSERT INTO form_packs(user_id,curr_no,type) "
            "    VALUES(:user_id,:curr_no+1,:type); "
            "  END IF; "
            "END;";
        Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
        Qry.CreateVariable("curr_no",otFloat,rcpt.no);
        Qry.CreateVariable("type",otString,rcpt.form_type);
        Qry.Execute();
    }
    else
    {
        //повтор печати
        rcpt_id=NodeAsInteger("receipt/id",reqNode);
        if (!GetReceipt(rcpt_id,rcpt))
            throw AstraLocale::UserException("MSG.RECEIPT_NOT_FOUND.REFRESH_DATA");
        rcptNode=NewTextChild(resNode,"receipt");
        logmsg << "Повтор печати квитанции " << rcpt.form_type << " "
               << fixed << setw(10) << setfill('0') << setprecision(0) << rcpt.no;
    };

    //последовательность для принтера
    PrintDataParser parser(rcpt);
    string data;
    bool hex;
    PrintInterface::GetPrintDataBR(
            rcpt.form_type,
            parser,
            data,
            hex,
            reqNode
            ); //последовательность для принтера
    SetProp( NewTextChild(rcptNode, "form", data), "hex", (int)hex);
    reqInfo->MsgToLog(logmsg.str(),ASTRA::evtPay,point_dep,0,grp_id);
};
