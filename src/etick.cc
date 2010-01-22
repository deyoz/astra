#include "etick.h"
#include <string>
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#include "astra_tick_view_xml.h"
#include "astra_tick_read_edi.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "checkin.h"
#include "term_version.h"
#include "jxtlib/jxt_cont.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/query_runner.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace Ticketing;
using namespace edilib;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickView;
using namespace Ticketing::TickMng;
using namespace Ticketing::ChangeStatus;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace ASTRA;
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

  int req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc));

  XMLDoc xmlCtxt("UTF-8","context");
  if (xmlCtxt.docPtr()==NULL)
    throw EXCEPTIONS::Exception("SearchETByTickNo: CreateXMLDoc failed");
  xmlNodePtr rootNode=NodeAsNode("/context",xmlCtxt.docPtr());
  NewTextChild(rootNode,"point_id",point_id);
  SetProp(rootNode,"req_ctxt_id",req_ctxt);

  TickDispByNum tickDisp(org, XMLTreeToText(xmlCtxt.docPtr()), req_ctxt, tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
  //в лог отсутствие связи
  if (TReqInfo::Instance()->desk.compatible(DEFER_ETSTATUS_VERSION))
  {
    xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
    SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
    NewTextChild(errNode,"message","Нет связи с сервером эл. билетов");
  }
  else
  {
    if ( strcmp((char *)reqNode->name, "SearchETByTickNo") == 0 )
      NewTextChild(resNode,"connect_error");
    else
      showProgError("Нет связи с сервером эл. билетов");
  };
};

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ServerFramework::getQueryRunner().getEdiHelpManager().Answer();

    string context;
    int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);

    AstraContext::ClearContext("TERM_REQUEST", req_ctxt_id);

    AstraContext::GetContext("EDI_RESPONSE",
                             req_ctxt_id,
                             context);
    AstraContext::ClearContext("EDI_RESPONSE", req_ctxt_id);

    XMLDoc ediResCtxt;
    if (context.empty())
      throw EXCEPTIONS::Exception("ETSearchInterface::KickHandler: context EDI_RESPONSE empty");

    int ret = ReadEdiMessage(context.c_str());
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
            ProgTrace(TRACE1, "ETS: ERROR %s", errc);
            const char * err_msg = GetDBFName(pMes,
                                              DataElement(4440),
                                              SegmElement("IFT"));
            if (*err_msg==0)
            {
              throw EXCEPTIONS::UserException("СЭБ: ОШИБКА %s", errc);
            }
            else
            {
              ProgTrace(TRACE1, "ETS: %s", err_msg);
              throw EXCEPTIONS::UserException("СЭБ: %s", err_msg);
            }
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

void ETStatusInterface::SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  int new_pr_etstatus=sign(NodeAsInteger("pr_etstatus",reqNode));
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");

  int old_pr_etstatus=sign(Qry.FieldAsInteger("pr_etstatus"));
  if (old_pr_etstatus==0 && new_pr_etstatus<0)
  {
    Qry.SQLText="UPDATE trip_sets SET pr_etstatus=-1 WHERE point_id=:point_id";
    Qry.Execute();
    TReqInfo::Instance()->MsgToLog( "Установлен режим временной отмены интерактива с СЭБ", evtFlt, point_id );
  }
  else
    if (old_pr_etstatus==new_pr_etstatus)
      throw UserException("Рейс уже переведен в данный режим");
    else
      throw UserException("Рейс не может быть переведен в данный режим");
};

void ChangeAreaStatus(TETCheckStatusArea area, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool tckin_version=GetNode("segments",reqNode)!=NULL;

  bool only_one;
  xmlNodePtr segNode;
  if (tckin_version)
  {
    segNode=NodeAsNode("segments/segment",reqNode);
    only_one=segNode->next==NULL;
  }
  else
  {
    segNode=reqNode;
    only_one=true;
  };
  bool processed=false;
  int req_ctxt=ASTRA::NoExists;
  for(;segNode!=NULL;segNode=segNode->next)
  {
    int id;
    switch (area)
    {
      case csaFlt:
        id=NodeAsInteger("point_id",segNode);
        break;
      case csaGrp:
        id=NodeAsInteger("grp_id",segNode);
        break;
      case csaPax:
        id=NodeAsInteger("pax_id",segNode);
        break;
      default: return;
    }

    try
    {
      xmlNodePtr node=GetNode("check_point_id",segNode);
      int check_point_id=-1;
      if (node!=NULL) check_point_id=NodeAsInteger(node);
      TTripInfo fltInfo;
      map<int,TTicketListCtxt> mtick;
      if (ETStatusInterface::ETCheckStatus(id,area,check_point_id,false,fltInfo,mtick))
      {
        if (req_ctxt==ASTRA::NoExists)
          req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc));
        if (ETStatusInterface::ETChangeStatus(req_ctxt,fltInfo,mtick))
        {
          //если сюда попали, то записать ошибку связи в лог
          processed=true; //послана хотя бы одна телеграмма
        };
      };
    }
    catch(UserException &e)
    {
      if (!only_one)
      {
        TQuery Qry(&OraSession);
        Qry.Clear();
        switch (area)
        {
          case csaFlt:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points "
              "WHERE point_id=:id";
            break;
          case csaGrp:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points,pax_grp "
              "WHERE points.point_id=pax_grp.point_dep AND "
              "      grp_id=:id";
            break;
          case csaPax:
            Qry.SQLText=
              "SELECT airline,flt_no,suffix,airp,scd_out "
              "FROM points,pax_grp,pax "
              "WHERE points.point_id=pax_grp.point_dep AND "
              "      pax_grp.grp_id=pax.grp_id AND "
              "      pax_id=:id";
            break;
          default: throw;
        };
        Qry.CreateVariable("id",otInteger,id);
        Qry.Execute();
        if (!Qry.Eof)
        {
          TTripInfo fltInfo(Qry);
          throw UserException("Рейс %s: %s",
                              GetTripName(fltInfo,true,false).c_str(),
                              e.what());
        }
        else
          throw;
      }
      else
        throw;
    };
    if (!tckin_version) break; //старый терминал
  };

  if (processed)
  {
    if (TReqInfo::Instance()->desk.compatible(DEFER_ETSTATUS_VERSION))
    {
      xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
      SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
      NewTextChild(errNode,"message","Нет связи с сервером эл. билетов");
    }
    else
      showProgError("Нет связи с сервером эл. билетов");
  };
};

void ETStatusInterface::ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaPax,ctxt,reqNode,resNode);
};

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaGrp,ctxt,reqNode,resNode);
};

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaFlt,ctxt,reqNode,resNode);
};

struct TETErrorFlight
{
  vector<string> global_errors;
  vector<string> errors;
};

void ETStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ServerFramework::getQueryRunner().getEdiHelpManager().Answer();

    string context;
    if (GetNode("@req_ctxt_id",reqNode)!=NULL)  //req_ctxt_id отсутствует, если телеграмма сформирована не от пульта
    {
      int req_ctxt_id=NodeAsInteger("@req_ctxt_id",reqNode);
      AstraContext::GetContext("TERM_REQUEST",
                               req_ctxt_id,
                               context);
      AstraContext::ClearContext("TERM_REQUEST", req_ctxt_id);
      XMLDoc termReqCtxt;
      if (context.empty())
        throw EXCEPTIONS::Exception("ETStatusInterface::KickHandler: context TERM_REQUEST empty");

      context=ConvertCodepage(context,"CP866","UTF-8");
      termReqCtxt.set(context);
      if (termReqCtxt.docPtr()==NULL)
        throw EXCEPTIONS::Exception("ETStatusInterface::KickHandler: context TERM_REQUEST wrong XML format");;

      xml_decode_nodelist(termReqCtxt.docPtr()->children);

      AstraContext::GetContext("EDI_RESPONSE",
                               req_ctxt_id,
                               context);
      AstraContext::ClearContext("EDI_RESPONSE", req_ctxt_id);
      XMLDoc ediResCtxt;
      if (context.empty())
        throw EXCEPTIONS::Exception("ETStatusInterface::KickHandler: context EDI_RESPONSE empty");

      context=ConvertCodepage(context,"CP866","UTF-8");
      ediResCtxt.set(context);
      if (ediResCtxt.docPtr()==NULL)
        throw EXCEPTIONS::Exception("ETStatusInterface::KickHandler: context EDI_RESPONSE wrong XML format");;

      xml_decode_nodelist(ediResCtxt.docPtr()->children);

      xmlNodePtr termReqNode=NodeAsNode("/term/query",termReqCtxt.docPtr())->children;
      if (termReqNode==NULL)
        throw EXCEPTIONS::Exception("ETStatusInterface::KickHandler: context TERM_REQUEST termReqNode=NULL");;
      string termReqName=(char*)(termReqNode->name);
      bool defer_etstatus=(termReqName=="ChangePaxStatus" ||
                           termReqName=="ChangeGrpStatus" ||
                           termReqName=="ChangeFltStatus");

      xmlNodePtr ediResNode=NodeAsNode("/context",ediResCtxt.docPtr());

      //flight,вектор global_error, вектор пар pax+ticket/coupon_error
      map<string, pair< vector<string>, vector< pair<string,string> > > > errors;
      xmlNodePtr ticketNode=NodeAsNode("tickets",ediResNode)->children;
      for(;ticketNode!=NULL;ticketNode=ticketNode->next)
      {
        string flight=NodeAsString("flight",ticketNode);
        string pax;
        if (GetNode("pax",ticketNode)!=NULL) pax=NodeAsString("pax",ticketNode);
        bool tick_event=false;
        for(xmlNodePtr node=ticketNode->children;node!=NULL;node=node->next)
        {
          if (strcmp((const char*)node->name,"coupon_status")==0) tick_event=true;

          if (!(strcmp((const char*)node->name,"global_error")==0 ||
                strcmp((const char*)node->name,"ticket_error")==0 ||
                strcmp((const char*)node->name,"coupon_error")==0)) continue;

          tick_event=true;

          pair< vector<string>, vector< pair<string,string> > > &err=errors[flight];

          if (strcmp((const char*)node->name,"global_error")==0)
          {
            if (find(err.first.begin(),err.first.end(),NodeAsString(node))==err.first.end())
              err.first.push_back(NodeAsString(node));
          }
          else
          {
            err.second.push_back(make_pair(pax,NodeAsString(node)));
          };
        };
        if (!tick_event)
        {
          ostringstream msg;
          msg << "Результат обращения к СЭБ для эл. билета "
              << NodeAsString("ticket_no",ticketNode) << "/"
              << NodeAsInteger("coupon_no",ticketNode)
              << " не определен. ";
          pair< vector<string>, vector< pair<string,string> > > &err=errors[flight];
          err.second.push_back(make_pair(pax,msg.str()));
        };
      };

      if (!errors.empty())
      {
        TReqInfo *reqInfo = TReqInfo::Instance();

        bool use_flight=(GetNode("segments",termReqNode)!=NULL &&
                         NodeAsNode("segments/segment",termReqNode)->next!=NULL);  //определим по запросу TERM_REQUEST;
        map<string, pair< vector<string>, vector< pair<string,string> > > >::iterator i;
        if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION) && !defer_etstatus)
        {
          ostringstream msg;
          for(i=errors.begin();i!=errors.end();i++)
          {
            if (use_flight)
              msg << "Рейс " << i->first << ":" << std::endl;
            for(vector<string>::iterator j=i->second.first.begin(); j!=i->second.first.end(); j++)
            {
              if (use_flight) msg << "     ";
              msg << *j << std::endl;
            };
            for(vector< pair<string,string> >::iterator j=i->second.second.begin(); j!=i->second.second.end(); j++)
            {
              if (use_flight) msg << "     ";
              if (!(j->first.empty()))
              {
                msg << j->first << ":" << std::endl
                    << "     ";
                if (use_flight) msg << "     ";
              };
              msg << j->second << std::endl;
            };
          };
          NewTextChild(resNode,"ets_error",msg.str());
          return;
        }
        else
        {
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.first.empty())
              throw EXCEPTIONS::UserException("%s",i->second.first.begin()->c_str());
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.second.empty())
              throw EXCEPTIONS::UserException("%s",(i->second.second.begin())->second.c_str());
        };
      };

      if (termReqName=="SavePax" ||
          termReqName=="SaveUnaccompBag" ||
          termReqName=="TCkinSavePax" ||
          termReqName=="TCkinSaveUnaccompBag")
      {
        CheckInInterface::SavePax(termReqNode, ediResNode, resNode);
      };
    };
};

bool ETStatusInterface::ETCheckStatus(int id,
                                      TETCheckStatusArea area,
                                      int check_point_id,
                                      bool check_connect,
                                      TTripInfo &fltInfo,
                                      map<int,TTicketListCtxt> &mtick,
                                      bool before_checkin)
{
  bool result=false;

  fltInfo.Clear();
  mtick.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<  "SELECT points.airline,points.flt_no,points.suffix,points.airp,points.scd_out, "
          "       points.act_out AS real_out,points.point_id, trip_sets.pr_etstatus ";
  switch (area)
  {
    case csaFlt:
      sql << "FROM points,trip_sets "
             "WHERE trip_sets.point_id=points.point_id AND "
             "      points.point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,id);
      break;
    case csaGrp:
      sql << "FROM points,trip_sets,pax_grp "
             "WHERE trip_sets.point_id=points.point_id AND "
             "      points.point_id=pax_grp.point_dep AND "
             "      pax_grp.grp_id=:grp_id ";
      Qry.CreateVariable("grp_id",otInteger,id);
      break;
    case csaPax:
      sql << "FROM points,trip_sets,pax_grp,pax "
             "WHERE trip_sets.point_id=points.point_id AND "
             "      points.point_id=pax_grp.point_dep AND "
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

    fltInfo.Init(Qry);
    if ((Qry.FieldAsInteger("pr_etstatus")>=0 || check_connect) &&
        !GetTripSets(tsETLOnly,fltInfo))
    {
      TDateTime act_out=ASTRA::NoExists;
      if (!Qry.FieldIsNULL("real_out")) act_out=Qry.FieldAsDateTime("real_out");

      Qry.Clear();
      sql.str("");
      sql <<
        "SELECT pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, "
        "       pax.ticket_no, pax.coupon_no, "
        "       pax.refuse, pax.pr_brd, "
        "       etickets.point_id AS tick_point_id, "
        "       etickets.airp_dep AS tick_airp_dep, "
        "       etickets.airp_arv AS tick_airp_arv, "
        "       etickets.coupon_status AS coupon_status, "
        "       pax.grp_id, pax.pax_id, pax.reg_no, "
        "       pax.surname, pax.name, pax.pers_type "
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

      Qry.SQLText=sql.str().c_str();
      Qry.Execute();
      if (!Qry.Eof)
      {
        for(;!Qry.Eof;Qry.Next())
        {
          string ticket_no=Qry.FieldAsString("ticket_no");
          int coupon_no=Qry.FieldAsInteger("coupon_no");

          string airp_dep=Qry.FieldAsString("airp_dep");
          string airp_arv=Qry.FieldAsString("airp_arv");

          CouponStatus status;
          if (Qry.FieldIsNULL("coupon_status"))
            status=CouponStatus(CouponStatus::OriginalIssue);//CouponStatus::Notification ???
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
            TTicketListCtxt &ltick=mtick[real_status->codeInt()];
            if (ltick.second.docPtr()==NULL)
            {
              ltick.second.set("UTF-8","context");
              if (ltick.second.docPtr()==NULL)
                throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
              NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
            };

            ProgTrace(TRACE5,"status=%s real_status=%s",status->dispCode(),real_status->dispCode());
            Coupon_info ci (coupon_no,real_status);

            TDateTime scd_local=UTCToLocal(fltInfo.scd_out,
                                           AirpTZRegion(fltInfo.airp));
            ptime scd(DateTimeToBoost(scd_local));
            Itin itin(fltInfo.airline,                      //marketing carrier
                    "",                                  //operating carrier
                    fltInfo.flt_no,0,
                    SubClass(),
                    scd.date(),
                    time_duration(not_a_date_time), // not a date time
                    airp_dep,
                    airp_arv);
            Coupon cpn(ci,itin);

            list<Coupon> lcpn;
            lcpn.push_back(cpn);

            Ticket tick(ticket_no, lcpn);
            ltick.first.push_back(tick);
            result=true;

            xmlNodePtr node=NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
            NewTextChild(node,"ticket_no",ticket_no);
            NewTextChild(node,"coupon_no",coupon_no);
            NewTextChild(node,"point_id",point_id);
            NewTextChild(node,"airp_dep",airp_dep);
            NewTextChild(node,"airp_arv",airp_arv);
            NewTextChild(node,"flight",GetTripName(fltInfo,true,false));
            NewTextChild(node,"grp_id",Qry.FieldAsInteger("grp_id"));
            NewTextChild(node,"pax_id",Qry.FieldAsInteger("pax_id"));
            if (!before_checkin)
              NewTextChild(node,"reg_no",Qry.FieldAsInteger("reg_no"));
            ostringstream pax;
            pax << "Пассажир " << Qry.FieldAsString("surname")
                << (Qry.FieldIsNULL("name")?"":" ") << Qry.FieldAsString("name")
                << " (" << Qry.FieldAsString("pers_type") << ")";
            NewTextChild(node,"pax",pax.str());

            ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                             ticket_no.c_str(),
                             coupon_no,
                             real_status->dispCode());

          };
        };
      };
    };
  };

  if (check_point_id>=0)
  {
    //проверка билетов, пассажиры которых разрегистрированы (по всему рейсу)
    Qry.Clear();
    Qry.SQLText=
      "SELECT points.airline,points.flt_no,points.suffix,points.airp,points.scd_out, "
      "       points.act_out AS real_out,points.point_id,trip_sets.pr_etstatus "
      "FROM points,trip_sets "
      "WHERE trip_sets.point_id=points.point_id AND "
      "      points.point_id=:point_id AND pr_del>=0 ";
    Qry.CreateVariable("point_id",otInteger,check_point_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      CouponStatus real_status=CouponStatus(CouponStatus::OriginalIssue);
      fltInfo.Init(Qry);
      if ((Qry.FieldAsInteger("pr_etstatus")>=0 || check_connect) &&
          !GetTripSets(tsETLOnly,fltInfo))
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
          string ticket_no=Qry.FieldAsString("ticket_no");
          int coupon_no=Qry.FieldAsInteger("coupon_no");

          TTicketListCtxt &ltick=mtick[real_status->codeInt()];
          if (ltick.second.docPtr()==NULL)
          {
            ltick.second.set("UTF-8","context");
            if (ltick.second.docPtr()==NULL)
              throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
            NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
          };

          Coupon_info ci (coupon_no,real_status);
          TDateTime scd_local=UTCToLocal(fltInfo.scd_out,
                                         AirpTZRegion(fltInfo.airp));
          ptime scd(DateTimeToBoost(scd_local));
          Itin itin(fltInfo.airline,                          //marketing carrier
                      "",                                  //operating carrier
                      fltInfo.flt_no,0,
                      SubClass(),
                      scd.date(),
                      time_duration(not_a_date_time), // not a date time
                      Qry.FieldAsString("airp_dep"),
                      Qry.FieldAsString("airp_arv"));
          Coupon cpn(ci,itin);
          list<Coupon> lcpn;
          lcpn.push_back(cpn);
          Ticket tick(ticket_no, lcpn);
          ltick.first.push_back(tick);
          result=true;

          xmlNodePtr node=NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
          NewTextChild(node,"ticket_no",ticket_no);
          NewTextChild(node,"coupon_no",coupon_no);
          NewTextChild(node,"point_id",check_point_id);
          NewTextChild(node,"airp_dep",Qry.FieldAsString("airp_dep"));
          NewTextChild(node,"airp_arv",Qry.FieldAsString("airp_arv"));
          NewTextChild(node,"flight",GetTripName(fltInfo,true,false));

          ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                           ticket_no.c_str(),
                           coupon_no,
                           real_status->dispCode());
        };
      };
    };
  };
  return result;
}

bool ETStatusInterface::ETChangeStatus(const int reqCtxtId,
                                       const TTripInfo &fltInfo,
                                       const map<int,TTicketListCtxt> &mtick)
{
  bool result=false;

  string oper_carrier;
  bool init_edi_addrs=false;

  for(map<int,TTicketListCtxt>::const_iterator i=mtick.begin();i!=mtick.end();i++)
  {
    const TTicketList &ltick=i->second.first;
    if (ltick.empty()) continue;

    if (oper_carrier.empty())
      oper_carrier=fltInfo.airline;
    if (!init_edi_addrs)
      init_edi_addrs=set_edi_addrs(fltInfo.airline,fltInfo.flt_no);

    if (oper_carrier.empty())
      throw EXCEPTIONS::Exception("ETCheckStatus: unkown operation carrier");
    if (!init_edi_addrs)
    {
      ProgError(STDLOG,
                "ETCheckStatus: edifact UNB-adresses not defined (airline=%s, flt_no=%d)",
                fltInfo.airline.c_str(),fltInfo.flt_no);
      throw UserException("Для рейса не задан адрес СЭБ");
    };

    try
    {
      TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
      if (!row.code_lat.empty()) oper_carrier=row.code_lat;
    }
    catch(EBaseTableError) {};

    ProgTrace(TRACE5,"ETCheckStatus: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                     oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

    TReqInfo& reqInfo = *(TReqInfo::Instance());
    xmlNodePtr rootNode=NodeAsNode("/context",i->second.second.docPtr());

    if (reqCtxtId!=ASTRA::NoExists)
      SetProp(rootNode,"req_ctxt_id",reqCtxtId);
    SetProp(rootNode,"desk",reqInfo.desk.code);
    SetProp(rootNode,"user",reqInfo.user.descr);
    SetProp(rootNode,"screen",reqInfo.screen.name);

    string ediCtxt=XMLTreeToText(i->second.second.docPtr());

    if (reqInfo.desk.code.empty())
    {
      //не запрос
      OrigOfRequest org(oper_carrier);
      ChangeStatus::ETChangeStatus(org, ltick, ediCtxt, reqCtxtId);
    }
    else
    {
      OrigOfRequest org(oper_carrier,reqInfo);
      ChangeStatus::ETChangeStatus(org, ltick, ediCtxt, reqCtxtId);
    };

    result=true;
  };
  return result;
};





