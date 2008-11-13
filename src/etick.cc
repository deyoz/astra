#include "etick.h"
#include <string>
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include "query_runner.h"
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#include "astra_tick_view_xml.h"
#include "astra_tick_read_edi.h"
#include "jxt_cont.h"
#include "basic.h"
#include "exceptions.h"
#include "base_tables.h"
#include "tripinfo.h"

using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;
using namespace Ticketing::ChangeStatus;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace JxtContext;
using namespace BASIC;
using namespace EXCEPTIONS;

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);
  int point_id=NodeAsInteger("point_id",reqNode);
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT airline,flt_no,airp FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
  TTripInfo info;
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.airp=Qry.FieldAsString("airp");
  if (GetTripSets(tsETLOnly,info))
    throw UserException("Работа с сервером эл. билетов в интерактивном режиме запрещена");
  if (!set_edi_addrs(info.airline,info.flt_no))
    throw UserException("Для рейса %s%d не определен адрес сервера эл. билетов",info.airline.c_str(),info.flt_no);

  string oper_carrier=info.airline;

  try
  {
    TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
    if (!row.code_lat.empty()) oper_carrier=row.code_lat;
  }
  catch(EBaseTableError) {};

  ProgTrace(TRACE5,"ETSearch: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                   oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

  OrigOfRequest org(oper_carrier,*TReqInfo::Instance());
  TickDispByNum tickDisp(org, tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
  //в лог отсутствие связи
/*  TLogMsg msg;
  msg.ev_type=ASTRA::evtFlt;
  msg.id1=point_id;
  msg.msg="Ошибка связи с СЭБ при попытке получить образ эл. билета "+tick_no;
  TReqInfo::Instance()->MsgToLog(msg);*/
  if( strcmp((char *)reqNode->name, "SearchETByTickNo") == 0)
    NewTextChild(resNode,"connect_error");
  else
    showProgError("Нет связи с сервером эл. билетов");
};

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE3,"KickHandler ...");
    ServerFramework::getQueryRunner().getEdiHelpManager().Answer();
    JxtCont *sysCont = getJxtContHandler()->sysContext();
    const char *edi_tlg = sysCont->readC("ETDisplayTlg");
    if(!edi_tlg)
      throw EXCEPTIONS::Exception("Can't read context 'ETDisplayTlg'");
    int ret = ReadEdiMessage(edi_tlg);
    sysCont->remove("ETDisplayTlg");
    if(ret == EDI_MES_STRUCT_ERR){
      throw EXCEPTIONS::Exception("Error in message structure: %s",EdiErrGetString());
    } else if( ret == EDI_MES_NOT_FND){
        throw EXCEPTIONS::Exception("No message found in template: %s",EdiErrGetString());
    } else if( ret == EDI_MES_ERR) {
        throw EXCEPTIONS::Exception("Edifact error ");
    }

    EDI_REAL_MES_STRUCT *pMes= GetEdiMesStruct();
    int num = GetNumSegGr(pMes, 3);
    if(!num){
        if(GetNumSegment(pMes, "ERC")){
            const char *errc = GetDBFName(pMes, DataElement(9321),
                                          "ET_NEG",
                                          CompElement("C901"),
                                          SegmElement("ERC"));
            ProgTrace(TRACE3,"err code=%s", errc);
            const char * err_msg = GetDBFName(pMes,
                                              DataElement(4440),
                                              SegmElement("IFT"), "ET_NEG");
            ProgTrace(TRACE1, "СЭБ: %s", err_msg);
            throw EXCEPTIONS::UserException(err_msg);
        }
        throw EXCEPTIONS::Exception("ETS error");
    } else if(num==1){
        try{
            xmlNodePtr dataNode=getNode(astra_iface(resNode, "ETViewForm"),"data");
            Pnr pnr = PnrRdr::doRead<Pnr>
                    (PnrEdiRead(GetEdiMesStruct()));
            Pnr::Trace(TRACE2, pnr);

            PnrDisp::doDisplay
                    (PnrXmlView(dataNode), pnr);
        }
        catch(edilib::EdiExcept &e)
        {
            throw EXCEPTIONS::Exception("edilib: %s", e.what());
        }
    } else {
        throw EXCEPTIONS::UserException("Просмотр списка эл. билетов не поддерживается"); //пока не поддерживается
    }
}

void ETStatusInterface::ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  NewTextChild(resNode,"ok");
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int check_point_id=-1;
  if (node!=NULL) check_point_id=NodeAsInteger(node);
  TLogMsg msg;
  if (ETCheckStatus(NodeAsInteger("pax_id",reqNode),csaPax,check_point_id,msg))
  {
    //если сюда попали, то записать ошибку связи в лог
    //TReqInfo::Instance()->MsgToLog(msg);
    showProgError("Нет связи с сервером эл. билетов");
  };
};

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  NewTextChild(resNode,"ok");
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int check_point_id=-1;
  if (node!=NULL) check_point_id=NodeAsInteger(node);
  TLogMsg msg;
  if (ETCheckStatus(NodeAsInteger("grp_id",reqNode),csaGrp,check_point_id,msg))
  {
    //если сюда попали, то записать ошибку связи в лог
    //TReqInfo::Instance()->MsgToLog(msg);
    showProgError("Нет связи с сервером эл. билетов");
  };
};

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  NewTextChild(resNode,"ok");
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int check_point_id=-1;
  if (node!=NULL) check_point_id=NodeAsInteger(node);
  TLogMsg msg;
  if (ETCheckStatus(NodeAsInteger("point_id",reqNode),csaFlt,check_point_id,msg))
  {
    //если сюда попали, то записать ошибку связи в лог
    //TReqInfo::Instance()->MsgToLog(msg);
    showProgError("Нет связи с сервером эл. билетов");
  };
};

void ETStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ServerFramework::getQueryRunner().getEdiHelpManager().Answer();
    ProgTrace(TRACE3,"Kick on change of status request");
    JxtCont *sysCont = getJxtContHandler()->sysContext();
    string glob_err = sysCont->read("ChangeOfStatusError");
    sysCont->remove("ChangeOfStatusError");
    NewTextChild(resNode,"ok");
    if (!glob_err.empty())
      showErrorMessage(string("Ошибка сервера эл. билетов: ")+glob_err);

};

bool ETCheckStatus(int id, TETCheckStatusArea area, int check_point_id, TLogMsg &errMsg)
{
  errMsg.Clear();
  errMsg.ev_type=ASTRA::evtPax;

  list<Ticket> ltick;
  string oper_carrier;
  bool init_edi_addrs=false;

  TQuery UpdQry(&OraSession);
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<  "SELECT points.airline,points.flt_no,points.suffix,points.airp,points.scd_out, "
          "       points.act_out AS real_out,points.point_id ";
  switch (area)
  {
    case csaFlt:
      sql << "FROM points "
             "WHERE points.point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,id);
      break;
    case csaGrp:
      sql << ",pax_grp.grp_id "
             "FROM points,pax_grp "
             "WHERE points.point_id=pax_grp.point_dep AND "
             "      pax_grp.grp_id=:grp_id ";
      Qry.CreateVariable("grp_id",otInteger,id);
      break;
    case csaPax:
      sql << ",pax_grp.grp_id,pax.reg_no "
             "FROM points,pax_grp,pax "
             "WHERE points.point_id=pax_grp.point_dep AND "
             "      pax_grp.grp_id=pax.grp_id AND "
             "      pax.pax_id=:pax_id ";
      Qry.CreateVariable("pax_id",otInteger,id);
    break;
    default: ;
  };
  sql << " AND points.pr_del>=0 ";
  Qry.SQLText=sql.str().c_str();
  Qry.Execute();
  if (!Qry.Eof)
  {
    int point_id=Qry.FieldAsInteger("point_id");
    errMsg.id1=point_id;
    if (area==csaPax)
      errMsg.id2=Qry.FieldAsInteger("reg_no");
    if (area==csaGrp || area==csaPax)
      errMsg.id3=Qry.FieldAsInteger("grp_id");

    TTripInfo info(Qry);
    if (!GetTripSets(tsETLOnly,info))
    {
      TDateTime act_out=ASTRA::NoExists;
      if (!Qry.FieldIsNULL("real_out")) act_out=Qry.FieldAsDateTime("real_out");

      Qry.Clear();
      sql.str("");
      sql <<
        "SELECT pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, "
        "       pax.ticket_no, pax.coupon_no, pax.ticket_confirm, "
        "       pax.refuse, pax.pr_brd, "
        "       etickets.point_id AS tick_point_id, "
        "       etickets.airp_dep AS tick_airp_dep, "
        "       etickets.airp_arv AS tick_airp_arv, "
        "       etickets.coupon_status AS coupon_status "
        "FROM pax_grp,pax,etickets "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax.ticket_rem='TKNE' AND "
        "      pax.ticket_no IS NOT NULL AND pax.coupon_no IS NOT NULL AND "
        "      pax.ticket_no=etickets.ticket_no(+) AND "
        "      pax.coupon_no=etickets.coupon_no(+) AND ";
      switch (area)
      {
        case csaFlt:
          sql << " pax_grp.point_dep=:point_id ";
          Qry.CreateVariable("point_id",otInteger,id);
          break;
        case csaGrp:
          sql << " pax.grp_id=:grp_id ";
          Qry.CreateVariable("grp_id",otInteger,id);
          break;
        case csaPax:
          sql << " pax.pax_id=:pax_id ";
          Qry.CreateVariable("pax_id",otInteger,id);
          break;
        default: ;
      }

      UpdQry.Clear();
      UpdQry.SQLText=
        "BEGIN "
        "  IF :ticket_confirm<>0 THEN "
        "    UPDATE etickets "
        "    SET point_id=:point_id, airp_dep=:airp_dep, airp_arv=:airp_arv "
        "    WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
        "    IF SQL%NOTFOUND THEN "
        "      INSERT INTO etickets(ticket_no,coupon_no,point_id,airp_dep,airp_arv,coupon_status) "
        "      VALUES(:ticket_no,:coupon_no,:point_id,:airp_dep,:airp_arv,NULL); "
        "    END IF; "
        "  ELSE "
        "    DELETE FROM etickets WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
        "  END IF; "
        "END;";
      UpdQry.DeclareVariable("ticket_no",otString);
      UpdQry.DeclareVariable("coupon_no",otInteger);
      UpdQry.DeclareVariable("point_id",otInteger);
      UpdQry.DeclareVariable("airp_dep",otString);
      UpdQry.DeclareVariable("airp_arv",otString);
      UpdQry.DeclareVariable("ticket_confirm",otInteger);


      Qry.SQLText=sql.str().c_str();
      Qry.Execute();
      if (!Qry.Eof)
      {
        for(;!Qry.Eof;Qry.Next())
        {
          string airp_dep=Qry.FieldAsString("airp_dep");
          string airp_arv=Qry.FieldAsString("airp_arv");
          bool pr_confirm=Qry.FieldAsInteger("ticket_confirm")!=0;

          if (pr_confirm)
          {
            CouponStatus status;
            if (Qry.FieldIsNULL("coupon_status"))
              status=CouponStatus(CouponStatus::OriginalIssue);
            else
              status=CouponStatus::fromDispCode(Qry.FieldAsString("coupon_status"));

            CouponStatus real_status;
            if (!Qry.FieldIsNULL("refuse"))
              //разрегистрирован
              real_status=CouponStatus(CouponStatus::OriginalIssue);
            else
            {
              if (Qry.FieldAsInteger("pr_brd")==0)
              	//не посажен
                real_status=CouponStatus(CouponStatus::Checked);
              else
              {
                if (act_out==ASTRA::NoExists)
                   //самолет не улетел
                  real_status=CouponStatus(CouponStatus::Boarded);
                else
                  real_status=CouponStatus(CouponStatus::Flown);
              };
            };
            if (status!=real_status)
            {
              ProgTrace(TRACE5,"status=%s real_status=%s",status->dispCode(),real_status->dispCode());
              Coupon_info ci (Qry.FieldAsInteger("coupon_no"),real_status);
              //if (area==csaFlt)
              //{
                TDateTime scd_local=UTCToLocal(info.scd_out,
                                               AirpTZRegion(info.airp));
                ptime scd(DateTimeToBoost(scd_local));
                Itin itin(info.airline,                      //marketing carrier
                        "",                                  //operating carrier
                        info.flt_no,0,
                        SubClass(),
                        scd.date(),
                        time_duration(not_a_date_time), // not a date time
                        airp_dep,
                        airp_arv);
                Coupon cpn(ci,itin);
              //}
              //else
              //  Coupon cpn(ci);

              list<Coupon> lcpn;
              lcpn.push_back(cpn);

              Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
              ltick.push_back(tick);
              if (oper_carrier.empty())
                oper_carrier=info.airline;
              if (!init_edi_addrs)
                init_edi_addrs=set_edi_addrs(info.airline,info.flt_no);

              ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                               Qry.FieldAsString("ticket_no"),
                               Qry.FieldAsInteger("coupon_no"),
                               real_status->dispCode());

            };
          };
          if (Qry.FieldIsNULL("tick_point_id") && pr_confirm ||
              !Qry.FieldIsNULL("tick_point_id") &&
              (point_id!=Qry.FieldAsInteger("tick_point_id") ||
               airp_dep!=Qry.FieldAsString("tick_airp_dep") ||
               airp_arv!=Qry.FieldAsString("tick_airp_arv") ||
               !pr_confirm))
          {
            UpdQry.SetVariable("ticket_no",Qry.FieldAsString("ticket_no"));
            UpdQry.SetVariable("coupon_no",Qry.FieldAsInteger("coupon_no"));
            UpdQry.SetVariable("point_id",point_id);
            UpdQry.SetVariable("airp_dep",airp_dep.c_str());
            UpdQry.SetVariable("airp_arv",airp_arv.c_str());
            UpdQry.SetVariable("ticket_confirm",(int)pr_confirm);
            UpdQry.Execute();
          };
        };
      };
    };
  };

  if (check_point_id>=0)
  {
    if (errMsg.id1==0)
    {
      errMsg.id1=check_point_id;
      if (area==csaGrp)
        errMsg.id3=id;
    };
    //проверка билетов, пассажиры которых разрегистрированы (по всему рейсу)
    Qry.Clear();
    Qry.SQLText=
      "SELECT points.airline,points.flt_no,points.suffix,points.airp,points.scd_out, "
      "       points.act_out AS real_out,points.point_id "
      "FROM points "
      "WHERE point_id=:point_id AND pr_del>=0 ";
    Qry.CreateVariable("point_id",otInteger,check_point_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      TTripInfo info(Qry);
      if (!GetTripSets(tsETLOnly,info))
      {
        Qry.Clear();
        Qry.SQLText=
          "SELECT etickets.ticket_no, etickets.coupon_no, "
          "       etickets.airp_dep, etickets.airp_arv "
          "FROM etickets,pax "
          "WHERE etickets.ticket_no=pax.ticket_no(+) AND "
          "      etickets.coupon_no=pax.coupon_no(+) AND "
          "      etickets.point_id=:point_id AND "
          "      pax.pax_id IS NULL AND "
          "      etickets.coupon_status IS NOT NULL";
        Qry.CreateVariable("point_id",otInteger,check_point_id);
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
          Coupon_info ci (Qry.FieldAsInteger("coupon_no"),CouponStatus(CouponStatus::OriginalIssue));
          TDateTime scd_local=UTCToLocal(info.scd_out,
                                         AirpTZRegion(info.airp));
          ptime scd(DateTimeToBoost(scd_local));
          Itin itin(info.airline,                          //marketing carrier
                      "",                                  //operating carrier
                      info.flt_no,0,
                      SubClass(),
                      scd.date(),
                      time_duration(not_a_date_time), // not a date time
                      Qry.FieldAsString("airp_dep"),
                      Qry.FieldAsString("airp_arv"));
          Coupon cpn(ci,itin);
          list<Coupon> lcpn;
          lcpn.push_back(cpn);
          Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
          ltick.push_back(tick);
          if (oper_carrier.empty())
            oper_carrier=info.airline;
          if (!init_edi_addrs)
            init_edi_addrs=set_edi_addrs(info.airline,info.flt_no);

          ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                           Qry.FieldAsString("ticket_no"),
                           Qry.FieldAsInteger("coupon_no"),
                           "O");
        };
      };
    };
  };

  if (!ltick.empty())
  {
    if (ltick.size()>1)
      errMsg.msg=="Ошибка связи с СЭБ при попытке изменения статуса эл. билетов";
    else
      errMsg.msg=="Ошибка связи с СЭБ при попытке изменения статуса эл. билетa";

    if (oper_carrier.empty())
      throw EXCEPTIONS::Exception("ETCheckStatus: unkown operation carrier");
    if (!init_edi_addrs)
      throw EXCEPTIONS::Exception("ETCheckStatus: edifact UNB-adresses not defined ");

    try
    {
      TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
      if (!row.code_lat.empty()) oper_carrier=row.code_lat;
    }
    catch(EBaseTableError) {};

    ProgTrace(TRACE5,"ETCheckStatus: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                     oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

    TReqInfo& reqInfo = *(TReqInfo::Instance());
    if (reqInfo.desk.code.empty())
    {
      //не запрос
      OrigOfRequest org(oper_carrier);
      ChangeStatus::ETChangeStatus(org, ltick);
    }
    else
    {
      OrigOfRequest org(oper_carrier,reqInfo);
      ChangeStatus::ETChangeStatus(org, ltick);
    };
  };
  return (!ltick.empty());
}





