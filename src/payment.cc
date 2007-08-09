#include "payment.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "checkin.h"
#include "print.h"
#include "tripinfo.h"
#include "oralib.h"
#include "astra_service.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

void PaymentInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int receipt_id=-1;

  TQuery Qry(&OraSession);

  string sqlText = (string)
        "SELECT pax_grp.grp_id,point_dep,airp_dep,airp_arv,airps.city AS city_arv, "
        "       class,pr_refuse,pax_grp.tid, "
        "       RTRIM(pax.surname||' '||pax.name) AS pax_name, "
        "       document AS pax_doc "
        "FROM pax_grp,pax,airps ";

  string condition;
  if( strcmp((char *)reqNode->name, "PaxByPaxId") == 0 ||
      strcmp((char *)reqNode->name, "PaxByRegNo") == 0 )
  {
    int reg_no;
    if( strcmp((char *)reqNode->name, "PaxByPaxId") == 0 )
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT point_dep,reg_no "
        "FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,NodeAsInteger("pax_id",reqNode));
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("Пассажир не зарегистрирован");
      reg_no=Qry.FieldAsInteger("reg_no");

      if (point_id!=Qry.FieldAsInteger("point_dep"))
      {
        point_id=Qry.FieldAsInteger("point_dep");
        if (!TripsInterface::readTripHeader( point_id, resNode ))
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
    else
    {
      reg_no=NodeAsInteger("reg_no",reqNode);
    };

    condition+=
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.airp_arv=airps.code AND "
      "      point_dep=:point_id AND reg_no=:reg_no ";
    Qry.Clear();
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("reg_no",otInteger,reg_no);
    sqlText+=condition;
    Qry.SQLText = sqlText;
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("Пассажир не зарегистрирован");
  }
  else
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT receipt_id,annul_date,ckin.get_main_pax_id(grp_id) AS pax_id "
      "FROM bag_receipts WHERE no=:no";
    Qry.CreateVariable("no",otFloat,NodeAsFloat("receipt_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("Квитанция не найдена");
    receipt_id=Qry.FieldAsInteger("receipt_id");

    if (!Qry.FieldIsNULL("pax_id") && Qry.FieldIsNULL("annul_date"))
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      condition+=
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax_grp.airp_arv=airps.code AND "
        "      pax.pax_id=:pax_id";
      Qry.Clear();
      Qry.CreateVariable("pax_id",otInteger,pax_id);

      sqlText+=condition;
      Qry.SQLText = sqlText;
      Qry.Execute();
      if (!Qry.Eof) receipt_id=-1;
    };
  };

  if (receipt_id!=-1)
  {
    LoadReceipts(receipt_id,false,resNode);
  }
  else
  {
    int grp_id=Qry.FieldAsInteger("grp_id");
    NewTextChild(resNode,"grp_id",grp_id);
    NewTextChild(resNode,"point_dep",Qry.FieldAsInteger("point_dep"));
    NewTextChild(resNode,"airp_dep",Qry.FieldAsString("airp_dep"));
    NewTextChild(resNode,"airp_arv",Qry.FieldAsString("airp_arv"));
    NewTextChild(resNode,"city_arv",Qry.FieldAsString("city_arv"));
    NewTextChild(resNode,"class",Qry.FieldAsString("class"));
    NewTextChild(resNode,"pr_refuse",(int)(Qry.FieldAsInteger("pr_refuse")!=0));
    NewTextChild(resNode,"pax_name",Qry.FieldAsString("pax_name"));
    NewTextChild(resNode,"pax_doc",Qry.FieldAsString("pax_doc"));
    NewTextChild(resNode,"tid",Qry.FieldAsInteger("tid"));

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
        if (first_part==ticket.substr(1,ticket.size()-2))
          tickets+=ticket.substr(ticket.size()-2);
        else
        {
          tickets+=ticket;
          first_part=ticket.substr(1,ticket.size()-2);
        };
      };
    };

    NewTextChild(resNode,"subclass",subcl);
    NewTextChild(resNode,"tickets",tickets);

    CheckInInterface::LoadBag(resNode);
    CheckInInterface::LoadPaidBag(resNode);
    LoadReceipts(grp_id,true,resNode);
  };
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
      "WHERE grp_id=:grp_id AND annul_date IS NULL ORDER BY issue_date";
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
    PutReceipt(rcpt,rcpt_id,node);
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
    "WHERE point_id=:point_id AND pr_reg<>0 AND pr_del=0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
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
    throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
  return new_tid;
};

void PaymentInterface::SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep=NodeAsInteger("point_dep",reqNode);
  int grp_id=NodeAsInteger("grp_id",reqNode);
  int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
  NewTextChild(resNode,"tid",tid);

  CheckInInterface::SaveBag(reqNode);
  CheckInInterface::SavePaidBag(reqNode);
};

void PaymentInterface::UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep=NodeAsInteger("point_dep",reqNode);
  int grp_id=NodeAsInteger("grp_id",reqNode);
  int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
  NewTextChild(resNode,"tid",tid);

  xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
  bool pr_del=NodeAsIntegerFast("pr_del",node);
  xmlNodePtr idNode=GetNodeFast("id",node);

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (idNode!=NULL)
    Qry.CreateVariable("receipt_id",otInteger,NodeAsInteger(idNode));
  else
    Qry.CreateVariable("receipt_id",otInteger,FNull);

  if (!pr_del)
  {
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
    Qry.CreateVariable("no",otString,NodeAsStringFast("no",node));
    Qry.CreateVariable("aircode",otString,NodeAsStringFast("aircode",node));
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
      throw UserException("Квитанция не найдена. Обновите данные");

    NewTextChild(resNode,"receipt_id",Qry.GetVariableAsInteger("receipt_id"));
  }
  else
  {
    Qry.SQLText="DELETE FROM bag_prepay WHERE receipt_id=:receipt_id";
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
      throw UserException("Квитанция не найдена. Обновите данные");
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
  rcpt.pr_lat=Qry.FieldAsInteger("pr_lat")!=0;
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
  rcpt.pay_form=Qry.FieldAsString("pay_form");
  rcpt.remarks=Qry.FieldAsString("remarks");
  rcpt.issue_date=Qry.FieldAsDateTime("issue_date");
  rcpt.issue_desk=Qry.FieldAsString("issue_desk");
  rcpt.issue_place=Qry.FieldAsString("issue_place");
  if (!Qry.FieldIsNULL("annul_date"))
    rcpt.annul_date=Qry.FieldAsDateTime("annul_date");
  else
    rcpt.annul_date=NoExists;
  rcpt.annul_desk=Qry.FieldAsString("annul_desk");
  return true;
};

//из структуры в базу
int PaymentInterface::PutReceipt(TBagReceipt &rcpt, int grp_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT id__seq.nextval,SYSTEM.UTCSYSDATE INTO :receipt_id,:issue_date FROM dual; "
    "  INSERT INTO bag_receipts "
    "        (receipt_id,grp_id,status,pr_lat,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name, "
    "         tickets,prev_no,airline,aircode,flt_no,suffix,airp_dep,airp_arv,ex_amount,ex_weight,value_tax, "
    "         rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,pay_form,remarks, "
    "         issue_date,issue_place,issue_user_id,issue_desk) "
    "  VALUES(:receipt_id,:grp_id,:status,:pr_lat,:form_type,:no,:pax_name,:pax_doc,:service_type,:bag_type,:bag_name, "
    "         :tickets,:prev_no,:airline,:aircode,:flt_no,:suffix,:airp_dep,:airp_arv,:ex_amount,:ex_weight,:value_tax, "
    "         :rate,:rate_cur,:exch_rate,:exch_pay_rate,:pay_rate_cur,:pay_form,:remarks, "
    "         :issue_date,:issue_place,:issue_user_id,:issue_desk); "
    "END;";
  Qry.CreateVariable("receipt_id",otInteger,FNull);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("status",otString,"П");
  Qry.CreateVariable("pr_lat",otInteger,(int)rcpt.pr_lat);
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
  Qry.CreateVariable("pay_form",otString,rcpt.pay_form);
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
    {
      ostringstream msg;
      msg.setf(ios::fixed);
      msg << "Бланк квитанции с номером "
          << setw(10) << setfill('0') << setprecision(0) << rcpt.no
          << " уже использован";
      throw UserException(msg.str());
    }
    else
      throw;
  };
  return Qry.GetVariableAsInteger("receipt_id");
};


//из структуры в XML
void PaymentInterface::PutReceipt(TBagReceipt &rcpt, int rcpt_id, xmlNodePtr resNode)
{
  if (resNode==NULL) return;

  xmlNodePtr rcptNode=NewTextChild(resNode,"receipt");

  if (rcpt_id!=-1)
    NewTextChild(rcptNode,"id",rcpt_id);
  NewTextChild(rcptNode,"pr_lat",(int)rcpt.pr_lat);
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
  NewTextChild(rcptNode,"pay_form",rcpt.pay_form);
  NewTextChild(rcptNode,"remarks",rcpt.remarks,"");
  NewTextChild(rcptNode,"issue_date",DateTimeToStr(rcpt.issue_date));
  NewTextChild(rcptNode,"issue_place",rcpt.issue_place);
  if (rcpt.annul_date!=NoExists)
    NewTextChild(rcptNode,"annul_date",DateTimeToStr(rcpt.annul_date));
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
    throw UserException("Печать установленного бланка не производится");
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
    "       ckin.get_main_pax_id(pax_grp.grp_id) AS pax_id "
    "FROM points,pax_grp "
    "WHERE points.point_id=pax_grp.point_dep AND "
    "      pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof)
    throw UserException("Информация по рейсу или пассажиру изменена. Обновите данные");
  //int pax_id=Qry.FieldAsInteger("pax_id");

  rcpt.airline=Qry.FieldAsString("airline");
  rcpt.aircode=base_tables.get("airlines").get_row("code", rcpt.airline).AsString("aircode");
  rcpt.flt_no=Qry.FieldAsInteger("flt_no");
  rcpt.suffix=Qry.FieldAsString("suffix");
  rcpt.airp_dep=Qry.FieldAsString("airp_dep");
  rcpt.airp_arv=Qry.FieldAsString("airp_arv");
  string city_dep=base_tables.get("airps").get_row("code", rcpt.airp_dep).AsString("city");
  string city_arv=base_tables.get("airps").get_row("code", rcpt.airp_arv).AsString("city");
  string country_dep=base_tables.get("cities").get_row("code", city_dep).AsString("country");
  string country_arv=base_tables.get("cities").get_row("code", city_arv).AsString("country");
  rcpt.pr_lat=country_dep!="РФ" || country_arv!="РФ";
  rcpt.form_type=NodeAsString("form_type",rcptNode);
  if (rcpt.form_type.empty())
    throw UserException("Не установлен тип бланка");

  if (GetNode("no",rcptNode)==NULL)
  {
     //Превью квитанции
    rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
    rcpt.pay_form="НАЛ";
  }
  else
  {
     //Запись квитанции
    rcpt.no=NodeAsFloat("no",rcptNode);
    rcpt.bag_name=NodeAsString("bag_name",rcptNode);
    rcpt.pay_form=NodeAsString("pay_form",rcptNode);
  };
  rcpt.pax_name=NodeAsString("pax_name",rcptNode);
  rcpt.pax_doc=NodeAsString("pax_doc",rcptNode);
  rcpt.service_type=NodeAsInteger("service_type",rcptNode);
  if (!NodeIsNULL("bag_type",rcptNode))
    rcpt.bag_type=NodeAsInteger("bag_type",rcptNode);
  else
    rcpt.bag_type=-1;

  rcpt.tickets=NodeAsString("tickets",rcptNode);
  rcpt.prev_no=NodeAsString("prev_no",rcptNode);
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
  rcpt.issue_place=get_validator();

  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";
};

void PaymentInterface::PutReceiptFields(TBagReceipt &rcpt, PrintDataParser &parser, xmlNodePtr node)
{
  if (node==NULL) return;

  //формируем в ответ образ телеграммы
  xmlNodePtr fieldsNode=NewTextChild(node,"fields");
  NewTextChild(fieldsNode,"pax_doc",parser.GetTagAsString("pax_doc"));
  NewTextChild(fieldsNode,"pax_name",parser.GetTagAsString("pax_name"));
  NewTextChild(fieldsNode,"bag_name",parser.GetTagAsString("bag_name"));
  NewTextChild(fieldsNode,"amount_figures",parser.GetTagAsString("amount_figures"));
  NewTextChild(fieldsNode,"tickets",parser.GetTagAsString("tickets"));
  NewTextChild(fieldsNode,"prev_no",parser.GetTagAsString("prev_no"));
  NewTextChild(fieldsNode,"pay_form",parser.GetTagAsString("pay_form"));
  NewTextChild(fieldsNode,"aircode",parser.GetTagAsString("aircode"));
  NewTextChild(fieldsNode,"issue_place1",parser.GetTagAsString("issue_place1"));
  NewTextChild(fieldsNode,"issue_place2",parser.GetTagAsString("issue_place2"));
  NewTextChild(fieldsNode,"issue_place3",parser.GetTagAsString("issue_place3"));
  NewTextChild(fieldsNode,"issue_place4",parser.GetTagAsString("issue_place4"));
  if (!rcpt.pr_lat)
  {
    NewTextChild(fieldsNode,"service_type",parser.GetTagAsString("service_type"));
    NewTextChild(fieldsNode,"amount_letters",parser.GetTagAsString("amount_letters"));
    NewTextChild(fieldsNode,"currency",parser.GetTagAsString("currency"));
    NewTextChild(fieldsNode,"issue_date",parser.GetTagAsString("issue_date_str"));
    NewTextChild(fieldsNode,"to",parser.GetTagAsString("to"));
    NewTextChild(fieldsNode,"remarks1",parser.GetTagAsString("remarks1"));
    NewTextChild(fieldsNode,"remarks2",parser.GetTagAsString("remarks2"));
    NewTextChild(fieldsNode,"exchange_rate",parser.GetTagAsString("exchange_rate"));
    NewTextChild(fieldsNode,"total",parser.GetTagAsString("total"));
    NewTextChild(fieldsNode,"airline",parser.GetTagAsString("airline"));
  }
  else
  {
    NewTextChild(fieldsNode,"service_type",parser.GetTagAsString("service_type_lat"));
    NewTextChild(fieldsNode,"amount_letters",parser.GetTagAsString("amount_letters_lat"));
    NewTextChild(fieldsNode,"currency",parser.GetTagAsString("currency_lat"));
    NewTextChild(fieldsNode,"issue_date",parser.GetTagAsString("issue_date_str_lat"));
    NewTextChild(fieldsNode,"to",parser.GetTagAsString("to_lat"));
    NewTextChild(fieldsNode,"remarks1",parser.GetTagAsString("remarks1_lat"));
    NewTextChild(fieldsNode,"remarks2",parser.GetTagAsString("remarks2_lat"));
    NewTextChild(fieldsNode,"exchange_rate",parser.GetTagAsString("exchange_rate_lat"));
    NewTextChild(fieldsNode,"total",parser.GetTagAsString("total_lat"));
    NewTextChild(fieldsNode,"airline",parser.GetTagAsString("airline_lat"));
  };

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
    throw UserException("Квитанция не найдена. Обновите данные");

  GetReceipt(Qry,rcpt);
  PrintDataParser parser(rcpt);
  PutReceiptFields(rcpt,parser,node);

  string status=Qry.FieldAsString("status");

  if (rcpt.annul_date!=NoExists && !rcpt.annul_desk.empty())
  {
    TDateTime annul_date_local = UTCToLocal(rcpt.annul_date, CityTZRegion(DeskCity(rcpt.annul_desk)));
    ostringstream annul_str;
    if (status=="З")
      annul_str << "ЗАМЕНА " << DateTimeToStr(annul_date_local, (string)"ddmmmyy", false);
    if (status=="А")
      annul_str << "ВОЗВРАТ " << DateTimeToStr(annul_date_local, (string)"ddmmmyy", false);
    ReplaceTextChild(NodeAsNode("fields",node),"annul_str",annul_str.str()); //на русском языке
  };
};

void PaymentInterface::ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (GetNode("receipt/id",reqNode)==NULL)
  {
    TBagReceipt rcpt;
    GetReceipt(reqNode,rcpt);
    PrintDataParser parser(rcpt);
    PutReceiptFields(rcpt,parser,resNode);
  }
  else
  {
    PutReceiptFields(NodeAsInteger("receipt/id",reqNode),resNode);
  };
};

void PaymentInterface::ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TBagReceipt rcpt;
  int rcpt_id=NodeAsInteger("receipt/id",reqNode);

  if (!GetReceipt(rcpt_id,rcpt))
    throw UserException("Заменяемая квитанция не найдена. Обновите данные");
  rcpt.no=GetCurrNo(reqInfo->user.user_id,rcpt.form_type);
  rcpt.issue_date=NowUTC();
  rcpt.issue_desk=reqInfo->desk.code;
  rcpt.issue_place=get_validator();
  rcpt.annul_date=NoExists;
  rcpt.annul_desk="";

  PrintDataParser parser(rcpt);
  PutReceiptFields(rcpt,parser,resNode); //образ квитанции
};

void PaymentInterface::AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_dep=NodeAsInteger("point_dep",reqNode);
  int grp_id=NodeAsInteger("grp_id",reqNode);
  int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
  NewTextChild(resNode,"tid",tid);

  int rcpt_id=NodeAsInteger("receipt/id",reqNode);

  TQuery Qry(&OraSession);
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
    throw UserException("Аннулируемая квитанция была изменена. Обновите данные");
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
  int rcpt_id;
  if (GetNode("receipt/no",reqNode)!=NULL)
  {
    if (GetNode("receipt/id",reqNode)==NULL)
    {
      //печать новой квитанции
      GetReceipt(reqNode,rcpt);
    }
    else
    {
      //замена бланка
      rcpt_id=NodeAsInteger("receipt/id",reqNode);

      if (!GetReceipt(rcpt_id,rcpt))
        throw UserException("Заменяемая квитанция не найдена. Обновите данные");
      rcpt.no=NodeAsFloat("receipt/no",reqNode);
      rcpt.issue_date=NowUTC();
      rcpt.issue_desk=reqInfo->desk.code;
      rcpt.issue_place=get_validator();
      rcpt.annul_date=NoExists;
      rcpt.annul_desk="";

      Qry.Clear();
      Qry.SQLText=
        "UPDATE bag_receipts "
        "SET status=:new_status,annul_date=:annul_date,annul_user_id=:user_id,annul_desk=:desk "
        "WHERE receipt_id=:receipt_id AND status=:old_status";
      Qry.CreateVariable("receipt_id",otInteger,rcpt_id);
      Qry.CreateVariable("old_status",otString,"П");
      Qry.CreateVariable("new_status",otString,"З");
      Qry.CreateVariable("annul_date",otDate,rcpt.issue_date);
      Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
      Qry.CreateVariable("desk",otString,rcpt.issue_desk);
      Qry.Execute();
      if (Qry.RowsProcessed()<=0)
        throw UserException("Заменяемая квитанция была изменена. Обновите данные");
    };

    rcpt_id=PutReceipt(rcpt,grp_id);
    createSofiFileDATA( rcpt_id );
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
      throw UserException("Квитанция не найдена. Обновите данные");
  };

  //вывод
  PutReceipt(rcpt,rcpt_id,resNode); //квитанция
  PrintDataParser parser(rcpt);
  PutReceiptFields(rcpt,parser,resNode); //образ квитанции
  int prn_type=NodeAsInteger("prn_type", reqNode);
  string data;
  PrintInterface::GetPrintDataBR(rcpt.form_type,prn_type,parser,data); //последовательность для принтера
  NewTextChild(resNode, "form", data);
};
