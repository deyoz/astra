#include "payment.h"
#include "xml_unit.h"
#include "basic.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "checkin.h"
#include "print.h"
#include "tripinfo.h"
#include "oralib.h"

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
        "       class,pr_refuse,pax_grp.tid, ";

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
      "     RTRIM(pax.surname||' '||pax.name) AS pax "
      "FROM pax_grp,pax,airps "
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
    Qry.SQLText="SELECT receipt_id,grp_id,pax,annul_date FROM bag_receipts WHERE no=:no";
    Qry.CreateVariable("no",otFloat,NodeAsFloat("receipt_no",reqNode));
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("Квитанция не найдена");
    receipt_id=Qry.FieldAsInteger("receipt_id");
    string pax=Qry.FieldAsString("pax");

    if (!Qry.FieldIsNULL("grp_id") && Qry.FieldIsNULL("annul_date"))
    {
      int grp_id=Qry.FieldAsInteger("grp_id");
      condition+=
        "     :pax AS pax"
        "FROM pax_grp,airps"
        "WHERE pax_grp.airp_arv=airps.code AND"
        "      pax_grp.grp_id=:grp_id AND rownum=1";
      Qry.Clear();
      Qry.CreateVariable("pax",otInteger,pax);
      Qry.CreateVariable("grp_id",otInteger,grp_id);

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
    NewTextChild(resNode,"pax",Qry.FieldAsString("pax"));
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
    xmlNodePtr receiptNode=NewTextChild(node,"receipt");
    NewTextChild(receiptNode,"id",Qry.FieldAsInteger("receipt_id"));
    NewTextChild(receiptNode,"no",Qry.FieldAsFloat("no"));
    NewTextChild(receiptNode,"form_type",Qry.FieldAsString("form_type"));
    if (!Qry.FieldIsNULL("grp_id"))
      NewTextChild(receiptNode,"grp_id",Qry.FieldAsInteger("grp_id"));
    NewTextChild(receiptNode,"pax",Qry.FieldAsString("pax"));
    NewTextChild(receiptNode,"status",Qry.FieldAsString("status"));
    NewTextChild(receiptNode,"pr_lat",(int)(Qry.FieldAsInteger("pr_lat")!=0));
    NewTextChild(receiptNode,"tickets",Qry.FieldAsString("tickets"));
    NewTextChild(receiptNode,"prev_no",Qry.FieldAsString("prev_no"),"");
    NewTextChild(receiptNode,"airline",Qry.FieldAsString("airline"));
    NewTextChild(receiptNode,"aircode",Qry.FieldAsString("aircode"));
    if (!Qry.FieldIsNULL("flt_no"))
    {
      NewTextChild(receiptNode,"flt_no",Qry.FieldAsInteger("flt_no"));
      NewTextChild(receiptNode,"suffix",Qry.FieldAsString("suffix"));
    };
    NewTextChild(receiptNode,"airp_dep",Qry.FieldAsString("airp_dep"));
    NewTextChild(receiptNode,"airp_arv",Qry.FieldAsString("airp_arv"));
    NewTextChild(receiptNode,"ex_amount",Qry.FieldAsInteger("ex_amount"));
    NewTextChild(receiptNode,"ex_weight",Qry.FieldAsInteger("ex_weight"));
    if (!Qry.FieldIsNULL("bag_type"))
      NewTextChild(receiptNode,"bag_type",Qry.FieldAsInteger("bag_type"));
    else
      NewTextChild(receiptNode,"bag_type");
    NewTextChild(receiptNode,"bag_name",Qry.FieldAsString("bag_name"),"");
    if (!Qry.FieldIsNULL("value_tax"))
      NewTextChild(receiptNode,"value_tax",Qry.FieldAsFloat("value_tax"));
    NewTextChild(receiptNode,"rate",Qry.FieldAsFloat("rate"));
    NewTextChild(receiptNode,"rate_cur",Qry.FieldAsString("rate_cur"));
    NewTextChild(receiptNode,"pay_rate",Qry.FieldAsFloat("pay_rate"));
    NewTextChild(receiptNode,"pay_rate_cur",Qry.FieldAsString("pay_rate_cur"));
    NewTextChild(receiptNode,"pay_type",Qry.FieldAsString("pay_type"));
    NewTextChild(receiptNode,"pay_doc",Qry.FieldAsString("pay_doc"),"");
    NewTextChild(receiptNode,"remarks",Qry.FieldAsString("remarks"),"");
    NewTextChild(receiptNode,"issue_date",DateTimeToStr(Qry.FieldAsDateTime("issue_date")));
    NewTextChild(receiptNode,"issue_place",Qry.FieldAsString("issue_place"),"");
    if (!Qry.FieldIsNULL("annul_date"))
      NewTextChild(receiptNode,"annul_date",DateTimeToStr(Qry.FieldAsDateTime("annul_date")));
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

void PaymentInterface::UpdReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_dep=NodeAsInteger("point_dep",reqNode);
  int grp_id=NodeAsInteger("grp_id",reqNode);
  int tid=LockAndUpdTid(point_dep,grp_id,NodeAsInteger("tid",reqNode));
  NewTextChild(resNode,"tid",tid);

  xmlNodePtr node=NodeAsNode("receipt",reqNode)->children;
  bool pr_del=NodeAsIntegerFast("pr_del",node);
  xmlNodePtr idNode=GetNodeFast("id",node);

  TQuery Qry(&OraSession);

  if (!pr_del)
  {
    if (idNode!=NULL)
    {
      Qry.Clear();
      Qry.SQLText=
        "UPDATE bag_receipts "
        "SET status='З',annul_date=SYSTEM.UTCSYSDATE,annul_user_id=:user_id "
        "WHERE receipt_id=:receipt_id";
      Qry.CreateVariable("receipt_id",otInteger,NodeAsInteger(idNode));
      Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
      Qry.Execute();
      if (Qry.RowsProcessed()<=0)
        throw UserException("Квитанция не найдена. Обновите данные");
    };
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  SELECT id__seq.nextval,SYSTEM.UTCSYSDATE INTO :receipt_id,:issue_date FROM dual; "
      "  INSERT INTO bag_receipts "
      "        (receipt_id,no,form_type,grp_id,pax,status,pr_lat,tickets,prev_no,airline,aircode,flt_no,suffix, "
      "         airp_dep,airp_arv,ex_amount,ex_weight,bag_type,bag_name,value_tax,rate,rate_cur, "
      "         pay_rate,pay_rate_cur,pay_type,pay_doc,remarks,issue_date,issue_place,issue_user_id) "
      "  VALUES(:receipt_id,:no,:form_type,:grp_id,:pax,:status,:pr_lat,:tickets,:prev_no,:airline,:aircode,:flt_no,:suffix, "
      "         :airp_dep,:airp_arv,:ex_amount,:ex_weight,:bag_type,:bag_name,:value_tax,:rate,:rate_cur, "
      "         :pay_rate,:pay_rate_cur,:pay_type,:pay_doc,:remarks,:issue_date,:issue_place,:issue_user_id); "
      "END;";
    Qry.CreateVariable("receipt_id",otInteger,FNull);
    double new_no=NodeAsFloatFast("no",node);
    string new_form_type=NodeAsStringFast("form_type",node);
    Qry.CreateVariable("no",otFloat,new_no);
    Qry.CreateVariable("form_type",otString,new_form_type);
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("pax",otString,NodeAsStringFast("pax",node));
    Qry.CreateVariable("status",otString,"П");
    Qry.CreateVariable("pr_lat",otInteger,(int)(NodeAsIntegerFast("pr_lat",node)!=0));
    Qry.CreateVariable("tickets",otString,NodeAsStringFast("tickets",node));
    Qry.CreateVariable("prev_no",otString,NodeAsStringFast("prev_no",node));
    Qry.CreateVariable("airline",otString,NodeAsStringFast("airline",node));
    Qry.CreateVariable("aircode",otString,NodeAsStringFast("aircode",node));
    if (!NodeIsNULLFast("flt_no",node,true))
      Qry.CreateVariable("flt_no",otInteger,NodeAsIntegerFast("flt_no",node));
    else
      Qry.CreateVariable("flt_no",otInteger,FNull);
    Qry.CreateVariable("suffix",otString,NodeAsStringFast("suffix",node,""));
    Qry.CreateVariable("airp_dep",otString,NodeAsStringFast("airp_dep",node));
    Qry.CreateVariable("airp_arv",otString,NodeAsStringFast("airp_arv",node));
    if (!NodeIsNULLFast("ex_amount",node,true))
      Qry.CreateVariable("ex_amount",otInteger,NodeAsIntegerFast("ex_amount",node));
    else
      Qry.CreateVariable("ex_amount",otInteger,FNull);
    if (!NodeIsNULLFast("ex_weight",node,true))
      Qry.CreateVariable("ex_weight",otInteger,NodeAsIntegerFast("ex_weight",node));
    else
      Qry.CreateVariable("ex_weight",otInteger,FNull);
    if (!NodeIsNULLFast("bag_type",node))
      Qry.CreateVariable("bag_type",otInteger,NodeAsIntegerFast("bag_type",node));
    else
      Qry.CreateVariable("bag_type",otInteger,FNull);
    Qry.CreateVariable("bag_name",otString,NodeAsStringFast("bag_name",node));
    if (!NodeIsNULLFast("value_tax",node,true))
      Qry.CreateVariable("value_tax",otFloat,NodeAsFloatFast("value_tax",node));
    else
      Qry.CreateVariable("value_tax",otFloat,FNull);
    Qry.CreateVariable("rate",otFloat,NodeAsFloatFast("rate",node));
    Qry.CreateVariable("rate_cur",otString,NodeAsStringFast("rate_cur",node));
    Qry.CreateVariable("pay_rate",otFloat,NodeAsFloatFast("pay_rate",node));
    Qry.CreateVariable("pay_rate_cur",otString,NodeAsStringFast("pay_rate_cur",node));
    Qry.CreateVariable("pay_type",otString,NodeAsStringFast("pay_type",node));
    Qry.CreateVariable("pay_doc",otString,NodeAsStringFast("pay_doc",node));
    Qry.CreateVariable("remarks",otString,NodeAsStringFast("remarks",node));
    Qry.CreateVariable("issue_date",otDate,FNull);
    Qry.CreateVariable("issue_place",otString,NodeAsStringFast("issue_place",node));
    Qry.CreateVariable("issue_user_id",otInteger,reqInfo->user.user_id);
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
            << setw(10) << setfill('0') << setprecision(0) << NodeAsFloatFast("no",node)
            << " уже использован";
        throw UserException(msg.str());
      }
      else
        throw;
    };

    NewTextChild(resNode,"receipt_id",Qry.GetVariableAsInteger("receipt_id"));
    NewTextChild(resNode,"issue_date",DateTimeToStr(Qry.GetVariableAsDateTime("issue_date")));

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
    Qry.CreateVariable("curr_no",otFloat,new_no);
    Qry.CreateVariable("type",otString,new_form_type);
    Qry.Execute();
  }
  else
  {
    Qry.Clear();
    Qry.SQLText=
      "UPDATE bag_receipts "
      "SET status='А',annul_date=SYSTEM.UTCSYSDATE,annul_user_id=:user_id "
      "WHERE receipt_id=:receipt_id";
    Qry.CreateVariable("receipt_id",otInteger,NodeAsInteger(idNode));
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
      throw UserException("Квитанция не найдена. Обновите данные");
  };

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

void PaymentInterface::ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool pr_lat=NodeAsInteger("pr_lat",reqNode)!=0;
  string str;

  TBaseTableRow &airline = base_tables.get("airlines").get_row("code", NodeAsString("airline",reqNode));
  str=airline.AsString("code",pr_lat);
  if (str.empty()) str=NodeAsString("airline",reqNode);
  NewTextChild(resNode,"carrier",str);
  str=airline.AsString("short_name",pr_lat);
  if (str.empty()) str=airline.AsString("name",pr_lat);
  if (str.empty()) str=airline.AsString("code",pr_lat);
  if (str.empty()) str=NodeAsString("airline",reqNode);
  NewTextChild(resNode,"issued_by",str);

  TBaseTableRow &airp_dep = base_tables.get("airps").get_row("code", NodeAsString("airp_dep",reqNode));
  TBaseTableRow &city_dep = base_tables.get("cities").get_row("code", airp_dep.AsString("city"));
  str=city_dep.AsString("name",pr_lat);
  if (str.empty()) airp_dep.AsString("name",pr_lat);
  if (str.empty()) city_dep.AsString("code",pr_lat);
  if (str.empty()) airp_dep.AsString("code",pr_lat);
  if (str.empty()) str=NodeAsString("airp_dep",reqNode);
  NewTextChild(resNode,"from",str);

  TBaseTableRow &airp_arv = base_tables.get("airps").get_row("code", NodeAsString("airp_arv",reqNode));
  TBaseTableRow &city_arv = base_tables.get("cities").get_row("code", airp_arv.AsString("city"));
  str=city_arv.AsString("name",pr_lat);
  if (str.empty()) airp_arv.AsString("name",pr_lat);
  if (str.empty()) city_arv.AsString("code",pr_lat);
  if (str.empty()) airp_arv.AsString("code",pr_lat);
  if (str.empty()) str=NodeAsString("airp_arv",reqNode);
  NewTextChild(resNode,"to",str);

  TReqInfo *reqInfo = TReqInfo::Instance();

  TDateTime d;
  if (GetNode("issue_date",reqNode)!=NULL)
    d=NodeAsDateTime("issue_date",reqNode);
  else
    d=NowUTC();
  NewTextChild(resNode,"issue_date",DateTimeToStr(d));
  d=UTCToLocal(d,reqInfo->desk.tz_region);
  NewTextChild(resNode,"issue_date_local",DateTimeToStr(d,"ddmmmyy",(int)pr_lat));

  if (GetNode("annul_date",reqNode)!=NULL)
  {
    d=NodeAsDateTime("annul_date",reqNode);
    d=UTCToLocal(d,reqInfo->desk.tz_region);
    NewTextChild(resNode,"annul_date_local",DateTimeToStr(d,"ddmmmyy",(int)pr_lat));
  };

  ostringstream issue_place;
  vector<string> validator;

  get_validator(validator);

  for(vector<string>::iterator i=validator.begin();i!=validator.end();i++)
  {
    issue_place << *i << endl;
  };
  NewTextChild(resNode,"issue_place",issue_place.str());
};

