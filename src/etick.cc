#include "etick.h"
#include <string>
#include <daemon.h>
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
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

using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::CouponStatus;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace JxtContext;
using namespace BASIC;
using namespace EXCEPTIONS;

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);

  OrigOfRequest org(*TReqInfo::Instance());
  TickDispByNum tickDisp(org, tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
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
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("pax_id",reqNode),csaPax,point_id))
    showProgError("Нет связи с сервером эл. билетов");
};

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("grp_id",reqNode),csaGrp,point_id))
    showProgError("Нет связи с сервером эл. билетов");
};

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node=GetNode("check_point_id",reqNode);
  int point_id=-1;
  if (node!=NULL) point_id=NodeAsInteger(node);
  if (ETCheckStatus(OrigOfRequest(*TReqInfo::Instance()),NodeAsInteger("point_id",reqNode),csaFlt,point_id))
    showProgError("Нет связи с сервером эл. билетов");
};

void ETStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ServerFramework::getQueryRunner().getEdiHelpManager().Answer();
    ProgTrace(TRACE3,"Kick on change of status request");
    JxtCont *sysCont = getJxtContHandler()->sysContext();
    string glob_err = sysCont->read("ChangeOfStatusError");
    sysCont->remove("ChangeOfStatusError");
    if (!glob_err.empty())
      showErrorMessage(string("Ошибка сервера эл. билетов: ")+glob_err);

};

bool ETCheckStatus(const OrigOfRequest &org, int id, TETCheckStatusArea area, int point_id)
{
  TQuery Qry(&OraSession);
  string sql=
    "SELECT points.point_id, points.airline AS oper_carrier, points.flt_no, "
    "       points.scd_out AS scd, points.act_out AS act, system.AirpTZRegion( points.airp ) AS region,"
    "       pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, "
    "       pax.ticket_no, pax.coupon_no, "
    "       pax.refuse, pax.pr_brd, "
    "       etickets.point_id AS tick_point_id, "
    "       etickets.airp_dep AS tick_airp_dep, "
    "       etickets.airp_arv AS tick_airp_arv, "
    "       etickets.coupon_status AS coupon_status "
    "FROM points,pax_grp,pax,etickets "
    "WHERE points.point_id=pax_grp.point_dep AND pax_grp.grp_id=pax.grp_id AND "
    "      pax.ticket_no IS NOT NULL AND pax.coupon_no IS NOT NULL AND "
    "      pax.ticket_no=etickets.ticket_no(+) AND "
    "      pax.coupon_no=etickets.coupon_no(+) AND ";
  switch (area)
  {
    case csaFlt:
      ProgTrace(TRACE5,"point_id=%d",id);
      sql=sql+"pax_grp.point_dep=:point_id ";
      Qry.CreateVariable("point_id",otInteger,id);
      break;
    case csaGrp:
      sql=sql+"pax.grp_id=:grp_id ";
      Qry.CreateVariable("grp_id",otInteger,id);
      break;
    case csaPax:
      sql=sql+"pax.pax_id=:pax_id ";
      Qry.CreateVariable("pax_id",otInteger,id);
      break;
    default: ;
  }

  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  UPDATE etickets "
    "  SET point_id=:point_id, airp_dep=:airp_dep, airp_arv=:airp_arv "
    "  WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO etickets(ticket_no,coupon_no,point_id,airp_dep,airp_arv,coupon_status) "
    "    VALUES(:ticket_no,:coupon_no,:point_id,:airp_dep,:airp_arv,NULL); "
    "  END IF; "
    "END;";
  UpdQry.DeclareVariable("ticket_no",otString);
  UpdQry.DeclareVariable("coupon_no",otInteger);
  UpdQry.DeclareVariable("point_id",otInteger);
  UpdQry.DeclareVariable("airp_dep",otString);
  UpdQry.DeclareVariable("airp_arv",otString);


  Qry.SQLText=sql;
  Qry.Execute();
  list<Ticket> ltick;

  if (!Qry.Eof)
  {
    for(;!Qry.Eof;Qry.Next())
    {
      int point_id=Qry.FieldAsInteger("point_id");
      string airp_dep=Qry.FieldAsString("airp_dep");
      string airp_arv=Qry.FieldAsString("airp_arv");

      coupon_status status;
      if (Qry.FieldIsNULL("coupon_status"))
        status=coupon_status(OriginalIssue);
      else
        status=coupon_status(Qry.FieldAsString("coupon_status"),true);

      coupon_status real_status;
      if (!Qry.FieldIsNULL("refuse"))
        //разрегистрирован
        real_status=coupon_status(OriginalIssue);
      else
      {
        if (Qry.FieldAsInteger("pr_brd")==0)
        	//не посажен
          real_status=coupon_status(Checked);
        else
        {
          if (Qry.FieldIsNULL("act"))
             //самолет не улетел
            real_status=coupon_status(Boarded);
          else
            real_status=coupon_status(Flown);
        };
      };
      if (status!=real_status)
      {
        ProgTrace(TRACE5,"status=%s real_status=%s",status.dispCode(),real_status.dispCode());
        Coupon_info ci (Qry.FieldAsInteger("coupon_no"),real_status);
        //if (area==csaFlt)
        //{
          TDateTime scd_local=UTCToLocal(Qry.FieldAsDateTime("scd"),Qry.FieldAsString("region"));
          ptime scd(DateTimeToBoost(scd_local));
          Itin itin(Qry.FieldAsString("oper_carrier"), //marketing carrier
                  "",                                  //operating carrier
                  Qry.FieldAsInteger("flt_no"),
                  -1,
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
        ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                         Qry.FieldAsString("ticket_no"),
                         Qry.FieldAsInteger("coupon_no"),
                         real_status.dispCode());

      };
      if (Qry.FieldIsNULL("tick_point_id") ||
          point_id!=Qry.FieldAsInteger("tick_point_id") ||
          airp_dep!=Qry.FieldAsString("tick_airp_dep") ||
          airp_arv!=Qry.FieldAsString("tick_airp_arv"))
      {
        tst();
        UpdQry.SetVariable("ticket_no",Qry.FieldAsString("ticket_no"));
        UpdQry.SetVariable("coupon_no",Qry.FieldAsInteger("coupon_no"));
        UpdQry.SetVariable("point_id",point_id);
        UpdQry.SetVariable("airp_dep",airp_dep.c_str());
        UpdQry.SetVariable("airp_arv",airp_arv.c_str());
        UpdQry.Execute();
      };
    };
  };


  if (point_id>=0)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT etickets.ticket_no,etickets.coupon_no, "
      "       points.airline AS oper_carrier, points.flt_no, points.scd_out AS scd, "
      "       system.AirpTZRegion( points.airp ) AS region, "
      "       etickets.airp_dep, etickets.airp_arv "
      "FROM etickets,points,pax "
      "WHERE etickets.point_id=points.point_id AND "
      "      etickets.ticket_no=pax.ticket_no(+) AND "
      "      etickets.coupon_no=pax.coupon_no(+) AND "
      "      etickets.point_id=:point_id AND "
      "      pax.ticket_no IS NULL AND "
      "      etickets.coupon_status IS NOT NULL";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      tst();
      Coupon_info ci (Qry.FieldAsInteger("coupon_no"),OriginalIssue);
      TDateTime scd_local=UTCToLocal(Qry.FieldAsDateTime("scd"),Qry.FieldAsString("region"));
      ptime scd(DateTimeToBoost(scd_local));
      Itin itin(Qry.FieldAsString("oper_carrier"), //marketing carrier
                  "",                                  //operating carrier
                  Qry.FieldAsInteger("flt_no"),
                  -1,
                  scd.date(),
                  time_duration(not_a_date_time), // not a date time
                  Qry.FieldAsString("airp_dep"),
                  Qry.FieldAsString("airp_arv"));
      Coupon cpn(ci,itin);
      list<Coupon> lcpn;
      lcpn.push_back(cpn);
      Ticket tick(Qry.FieldAsString("ticket_no"), lcpn);
      ltick.push_back(tick);
      ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                       Qry.FieldAsString("ticket_no"),
                       Qry.FieldAsInteger("coupon_no"),
                       "O");
    };
  };


  if (!ltick.empty())
    ChangeStatus::ETChangeStatus(org, ltick);



  return (!ltick.empty());
}





