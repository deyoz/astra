#include "etick.h"
#include <string>
#include "xml_unit.h"
#include "tlg/edi_tlg.h"
#include "astra_ticket.h"
#include "etick_change_status.h"
#include "astra_tick_view_xml.h"
#include "astra_tick_read_edi.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "checkin.h"
#include "web_main.h"
#include "term_version.h"
#include "misc.h"
#include "qrys.h"
#include <jxtlib/jxtlib.h>
#include <jxtlib/xml_stuff.h>
#include <serverlib/query_runner.h>
#include <edilib/edi_func_cpp.h>
#include <serverlib/testmode.h>

// TODO emd
#include "emdoc.h"
#include "astra_emd.h"
#include "astra_emd_view_xml.h"
#include "edi_utils.h"
#include "tlg/emd_disp_request.h"
#include "tlg/emd_system_update_request.h"
#include "tlg/emd_cos_request.h"
#include "tlg/emd_edifact.h"
#include "tlg/remote_results.h"
#include "tlg/remote_system_context.h"

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
using namespace AstraLocale;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraEdifact;

#define MAX_TICKETS_IN_TLG 5

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);
  checkDocNum(tick_no);

  int point_id=NodeAsInteger("point_id",reqNode);
  TTripInfo info;
  checkETSInteract(point_id, true, info);

  if(!inTestMode())
  {
    pair<string,string> edi_addrs;
    if (!get_et_addr_set(info.airline,info.flt_no,edi_addrs))
        throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                       LParams() << LParam("flight", ElemIdToCodeNative(etAirline,info.airline) + IntToString(info.flt_no)));

    set_edi_addrs(edi_addrs);
  }

  string oper_carrier=info.airline;
  /*
  try
  {
    TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
    if (!row.code_lat.empty()) oper_carrier=row.code_lat;
  }
  catch(EBaseTableError) {}
  */
  ProgTrace(TRACE5,"ETSearch: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                   oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

  OrigOfRequest org(oper_carrier,*TReqInfo::Instance());

  edifact::KickInfo kickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)),
                             "ETSearchForm");

  XMLDoc xmlCtxt("context");
  if (xmlCtxt.docPtr()==NULL)
    throw EXCEPTIONS::Exception("SearchETByTickNo: CreateXMLDoc failed");
  xmlNodePtr rootNode=NodeAsNode("/context",xmlCtxt.docPtr());
  NewTextChild(rootNode,"point_id",point_id);
  SetProp(rootNode,"req_ctxt_id",kickInfo.reqCtxtId);

  TickDispByNum tickDisp(org, XMLTreeToText(xmlCtxt.docPtr()), kickInfo, tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
  //в лог отсутствие связи
  if (TReqInfo::Instance()->desk.compatible(DEFER_ETSTATUS_VERSION))
  {
    xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
    SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
    NewTextChild(errNode,"message",getLocaleText("MSG.ETS_CONNECT_ERROR"));
  }
  else
  {
    if ( strcmp((char *)reqNode->name, "SearchETByTickNo") == 0 )
    {
      if (!TReqInfo::Instance()->desk.compatible(WEB_CHECKIN_VERSION))
        NewTextChild(resNode,"connect_error");
      else
        AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR"); //это из-за упущения в терминале!
    }
    else
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
  }
}

void ETSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                    xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
              throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", errc));
            }
            else
            {
              ProgTrace(TRACE1, "ETS: %s", err_msg);
              throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", err_msg));
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
        throw AstraLocale::UserException("MSG.ETICK.ET_LIST_VIEW_UNSUPPORTED"); //пока не поддерживается
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
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

  int old_pr_etstatus=sign(Qry.FieldAsInteger("pr_etstatus"));
  if (old_pr_etstatus==0 && new_pr_etstatus<0)
  {
    Qry.SQLText="UPDATE trip_sets SET pr_etstatus=-1 WHERE point_id=:point_id";
    Qry.Execute();
    TReqInfo::Instance()->LocaleToLog("EVT.ETICK.CANCEL_INTERACTIVE_WITH_ETC", evtFlt, point_id);
  }
  else
    if (old_pr_etstatus==new_pr_etstatus)
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_IN_THIS_MODE_ALREADY");
    else
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_CANNOT_BE_SET_IN_THIS_MODE");
}

//----------------------------------------------------------------------------------------------------------------

void EMDSearchInterface::SearchEMDByDocNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "SearchEMDByDocNo";
    Ticketing::TicketNum_t emdNum(NodeAsString("EmdNoEdit",reqNode));

    int pointId = NodeAsInteger("point_id",reqNode);
    TTripInfo info;
    checkEDSInteract(pointId, true, info);

    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);

    OrigOfRequest org(airline, *TReqInfo::Instance());

    edifact::KickInfo kickInfo(ASTRA::NoExists,"EMDSearch");

    try
    {
      edifact::EmdDispByNum emdDispParams(org,
                                          "",
                                          kickInfo,
                                          airline,
                                          flNum,
                                          emdNum);
      edifact::EmdDispRequestByNum ediReq(emdDispParams);
      ediReq.sendTlg();
    }
    catch(RemoteSystemContext::system_not_found)
    {
      throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                       LEvntPrms() << PrmFlight("flight", info.airline, info.flt_no, info.suffix) );
    };
}

void EMDSearchInterface::KickHandler(XMLRequestCtxt *ctxt,
                                     xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);

    using namespace edifact;
    pRemoteResults res = RemoteResults::readSingle();
    if(res->status() != RemoteStatus::Success)
    {
        LogTrace(TRACE1) << "Remote error!";
        throw AstraLocale::UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", "Remote error!"));
        //AddRemoteResultsMessageBox(resNode, *res);
    }
    else
    {
        std::list<Emd> emdList = EmdEdifactReader::readList(res->tlgSource());
        if(emdList.size() == 1)
        {
            LogTrace(TRACE3) << "Show EMD:\n" << emdList.front();
            EmdDisp::doDisplay(EmdXmlView(resNode, emdList.front()));
        }
        else
        {
            LogError(STDLOG) << "Unable to display " << emdList.size() << " EMDs";
            throw AstraLocale::UserException("MSG.ETICK.EMD_LIST_VIEW_UNSUPPORTED");
        }
    }

    FuncOut(KickHandler);
}

//----------------------------------------------------------------------------------------------------------------

void EMDSystemUpdateInterface::SysUpdateEmdCoupon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << __FUNCTION__;
    Ticketing::TicketNum_t etTickNum(NodeAsString("TickNoEdit", reqNode));
    CouponNum_t etCpnNum(NodeAsInteger("TickCpnNo", reqNode));

    Ticketing::TicketNum_t emdDocNum(NodeAsString("EmdNoEdit", reqNode));
    CouponNum_t emdCpnNum(NodeAsInteger("EmdCpnNo", reqNode));

    Ticketing::CpnStatAction::CpnStatAction_t cpnStatAction(CpnStatAction::disassociate);
    if( strcmp((char *)reqNode->name, "AssociateEMD") == 0)
      cpnStatAction=CpnStatAction::associate;

    int pointId = NodeAsInteger("point_id",reqNode);
    TTripInfo info;
    checkEDSInteract(pointId, true, info);

    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);

    OrigOfRequest org(airline, *TReqInfo::Instance());

    edifact::KickInfo kickInfo(ASTRA::NoExists, "EMDSystemUpdate");

    edifact::EmdDisassociateRequestParams disassocParams(org,
                                                         "",
                                                         kickInfo,
                                                         airline,
                                                         flNum,
                                                         Ticketing::TicketCpn_t(etTickNum, etCpnNum),
                                                         Ticketing::TicketCpn_t(emdDocNum, emdCpnNum),
                                                         cpnStatAction);
    edifact::EmdDisassociateRequest ediReq(disassocParams);
    ediReq.sendTlg();
}

string TEMDSystemUpdateItem::traceStr() const
{
  ostringstream s;
  s << "airline_oper: " << airline_oper << " "
       "flt_no_oper: " << flt_no_oper << " "
       "et: " << et << " "
       "emd: " << emd << " "
       "action: " << CpnActionTraceStr(action);
  return s.str();
}

string TEMDChangeStatusKey::traceStr() const
{
  ostringstream s;
  s << "airline_oper: " << airline_oper << " "
       "flt_no_oper: " << flt_no_oper << " "
       "coupon_status: " << coupon_status;
  return s.str();
}

string TEMDChangeStatusItem::traceStr() const
{
  ostringstream s;
  s << "emd: " << emd;
  return s.str();
}

void EMDSystemUpdateInterface::EMDCheckDisassociation(const int point_id,
                                                      TEMDSystemUpdateList &emdList)
{
  emdList.clear();

  AstraEdifact::TFltParams fltParams;
  if (!fltParams.get(point_id)) return;

  list< TEMDCtxtItem > emds;
  GetEMDDisassocList(point_id, fltParams.in_final_status, emds);

  string flight=GetTripName(fltParams.fltInfo,ecNone,true,false);

  if (!emds.empty() && fltParams.eds_no_interact)
    throw AstraLocale::UserException("MSG.EMD.INTERACTIVE_MODE_NOT_ALLOWED");

  for(list< TEMDCtxtItem >::iterator i=emds.begin(); i!=emds.end(); ++i)
  {
    if (i->pax.tkn.no.empty() ||
        i->pax.tkn.coupon==ASTRA::NoExists ||
        i->pax.tkn.rem!="TKNE")
    {
      /*  Невозможно провести операцию ассоциации/диссоциации из-за отсутствия или неполной информации по эл. билету пассажира */
      //!!!vlad возможно надо более тонко
      TLogLocale event;
      event.ev_type=ASTRA::evtPay;
      event.lexema_id= i->action==CpnStatAction::associate?"EVT.EMD_ASSOCIATION_MISTAKE":
                                                           "EVT.EMD_DISASSOCIATION_MISTAKE";
      event.prms << PrmSmpl<std::string>("emd_no", i->asvc.emd_no)
                 << PrmSmpl<int>("emd_coupon", i->asvc.emd_coupon);

      if (i->pax.tkn.rem!="TKNE" || i->pax.tkn.no.empty())
        event.prms << PrmLexema("err", "MSG.ETICK.NUMBER_NOT_SET");
      else
        event.prms << PrmLexema("err", "MSG.ETICK.COUPON_NOT_SET", LEvntPrms() << PrmSmpl<std::string>("etick", i->pax.tkn.no));

      ProcEdiEvent(event, *i, NULL, false);
      continue;
    };

    TEMDocItem EMDocItem;
    EMDocItem.fromDB(i->asvc.emd_no, i->asvc.emd_coupon, false);
    if (!EMDocItem.empty())
    {
      if (EMDocItem.action==i->action) continue;
      if (i->pax.tkn.empty())
      {
        i->pax.tkn.no=EMDocItem.et_no;
        i->pax.tkn.coupon=EMDocItem.et_coupon;
        i->pax.tkn.rem="TKNE";
      }
    }
    else
    {
      //в emdocs нет ничего
      if (i->action==CpnStatAction::associate) continue; //считаем что ассоциация сделана по умолчанию
    }

    emdList.push_back(make_pair(TEMDSystemUpdateItem(), XMLDoc()));

    TEMDSystemUpdateItem &item=emdList.back().first;
    item.airline_oper=getTripAirline(fltParams.fltInfo);
    item.flt_no_oper=getTripFlightNum(fltParams.fltInfo);
    item.et=Ticketing::TicketCpn_t(i->pax.tkn.no, i->pax.tkn.coupon);
    item.emd=Ticketing::TicketCpn_t(i->asvc.emd_no, i->asvc.emd_coupon);
    item.action=i->action;

    XMLDoc &ctxt=emdList.back().second;
    if (ctxt.docPtr()==NULL)
    {
      ctxt.set("context");
      if (ctxt.docPtr()==NULL)
        throw EXCEPTIONS::Exception("%s: CreateXMLDoc failed", __FUNCTION__);
      xmlNodePtr node=NewTextChild(NodeAsNode("/context",ctxt.docPtr()),"emdoc");

      i->flight=flight;
      i->toXML(node);
    }
  }
}

bool EMDSystemUpdateInterface::EMDChangeDisassociation(const edifact::KickInfo &kickInfo,
                                                       const TEMDSystemUpdateList &emdList)
{
  bool result=false;

  TReqInfo& reqInfo = *(TReqInfo::Instance());

  try
  {
    for(TEMDSystemUpdateList::const_iterator i=emdList.begin();i!=emdList.end();i++)
    {

      xmlNodePtr rootNode=NodeAsNode("/context",i->second.docPtr());

      if (kickInfo.reqCtxtId!=ASTRA::NoExists)
        SetProp(rootNode,"req_ctxt_id",kickInfo.reqCtxtId);
      TEdiOriginCtxt::toXML(rootNode);

      string ediCtxt=XMLTreeToText(i->second.docPtr());

      const OrigOfRequest &org=reqInfo.desk.code.empty()?OrigOfRequest(i->first.airline_oper):
                                                         OrigOfRequest(i->first.airline_oper, reqInfo);

      edifact::EmdDisassociateRequestParams disassocParams(org,
                                                           ediCtxt,
                                                           kickInfo,
                                                           i->first.airline_oper,
                                                           i->first.flt_no_oper,
                                                           i->first.et,
                                                           i->first.emd,
                                                           i->first.action);
      edifact::EmdDisassociateRequest ediReq(disassocParams);
      ediReq.sendTlg();

      LogTrace(TRACE5) << __FUNCTION__ << ": " << i->first.traceStr();

      result=true;
    }
  }
  catch(Ticketing::RemoteSystemContext::system_not_found &e)
  {
    throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                     LEvntPrms() << PrmFlight("flight", e.airline(), e.flNum()?e.flNum().get():ASTRA::NoExists, "") );
  };

  return result;
}

void EMDSystemUpdateInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);

    using namespace edifact;
    pRemoteResults res = RemoteResults::readSingle();
    MakeAnAnswer(resNode, res);

    FuncOut(KickHandler);
}

void EMDSystemUpdateInterface::MakeAnAnswer(xmlNodePtr resNode, edifact::pRemoteResults remRes)
{
    using namespace edifact;
    bool success = remRes->status() == RemoteStatus::Success;
    LogTrace(TRACE3) << "Handle " << ( success? "successfull" : "unsuccessfull") << " disassociation";
    xmlNodePtr answerNode = newChild(resNode, "result");
    xmlSetProp(answerNode, "status", remRes->status()->description());
    if(!success)
    {
        xmlSetProp(answerNode, "edi_error_code", remRes->ediErrCode());
        xmlSetProp(answerNode, "remark", remRes->remark());
    }
}

//----------------------------------------------------------------------------------------------------------------

void ChangeAreaStatus(TETCheckStatusArea area, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //эта функция вызывается только при отложенной смене статуса ЭБ, не вызывается при веб-регистрации
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
  }
  TETChangeStatusList mtick;
  for(;segNode!=NULL;segNode=segNode->next)
  {
    set<int> ids;
    switch (area)
    {
      case csaFlt:
        ids.insert(NodeAsInteger("point_id",segNode));
        break;
      case csaGrp:
        ids.insert(NodeAsInteger("grp_id",segNode));
        break;
      case csaPax:
        {
          xmlNodePtr node=GetNode("passengers/pax_id", segNode);
          if (node!=NULL)
          {
            for(; node!=NULL; node=node->next)
              ids.insert(NodeAsInteger(node));
          }
          else ids.insert(NodeAsInteger("pax_id",segNode));
        }
        break;
      default: throw EXCEPTIONS::Exception("ChangeAreaStatus: wrong area");
    }

    if (ids.empty()) throw EXCEPTIONS::Exception("ChangeAreaStatus: ids.empty()");

    xmlNodePtr node=GetNode("check_point_id",segNode);
    int check_point_id=(node==NULL?NoExists:NodeAsInteger(node));

    for(set<int>::const_iterator i=ids.begin(); i!=ids.end(); ++i)
    try
    {
      ETStatusInterface::ETCheckStatus(*i,
                                       area,
                                       (i==ids.begin()?check_point_id:NoExists),
                                       false,
                                       mtick);
    }
    catch(AstraLocale::UserException &e)
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
        }
        Qry.CreateVariable("id",otInteger,*i);
        Qry.Execute();
        if (!Qry.Eof)
        {
          TTripInfo fltInfo(Qry);
          throw AstraLocale::UserException("WRAP.FLIGHT",
                                           LParams()<<LParam("flight",GetTripName(fltInfo,ecNone,true,false))
                                                    <<LParam("text",e.getLexemaData( )));
        }
        else
          throw;
      }
      else
        throw;
    }
    if (!tckin_version) break; //старый терминал
  }

  if (!mtick.empty())
  {
    if (!ETStatusInterface::ETChangeStatus(reqNode,mtick))
      throw EXCEPTIONS::Exception("ChangeAreaStatus: Wrong mtick");

/*  это позже, когда терминалы будут отложенное подтверждение тоже обрабатывать через ets_connect_error!!!
    if (TReqInfo::Instance()->client_type==ctTerm &&
          TReqInfo::Instance()->desk.compatible(DEFER_ETSTATUS_VERSION2))
    {
      xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
      SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
      NewTextChild(errNode,"message",getLocaleText("MSG.ETS_CONNECT_ERROR"));
    }
    else*/
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
  }
}

void ETStatusInterface::ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaPax,ctxt,reqNode,resNode);
}

void ETStatusInterface::ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaGrp,ctxt,reqNode,resNode);
}

void ETStatusInterface::ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeAreaStatus(csaFlt,ctxt,reqNode,resNode);
}

void ETStatusInterface::ETRollbackStatus(xmlDocPtr ediResDocPtr,
                                         bool check_connect)
{
  if (ediResDocPtr==NULL) return;

  vector<int> point_ids;

  //ProgTrace(TRACE5, "ediResDoc=%s", XMLTreeToText(ediResDocPtr).c_str());

  xmlNodePtr ticketNode=GetNode("/context/tickets",ediResDocPtr);
  if (ticketNode!=NULL) ticketNode=ticketNode->children;
  for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
  {
    //цикл по билетам
    xmlNodePtr node2=node->children;

    ProgTrace(TRACE5,"ETRollbackStatus: ticket_no=%s coupon_no=%d",
              NodeAsStringFast("ticket_no",node2),
              NodeAsIntegerFast("coupon_no",node2));

    if (GetNodeFast("coupon_status",node2)==NULL) continue;
    int point_id=NodeAsIntegerFast("prior_point_id",node2,
                                   NodeAsIntegerFast("point_id",node2));
    if (find(point_ids.begin(),point_ids.end(),point_id)==point_ids.end())
      point_ids.push_back(point_id);
  }

  TETChangeStatusList mtick;
  for(vector<int>::iterator i=point_ids.begin();i!=point_ids.end();i++)
  {
    ProgTrace(TRACE5,"ETRollbackStatus: rollback point_id=%d",*i);
    ETStatusInterface::ETCheckStatus(*i,ediResDocPtr,false,mtick);
  }
  ETStatusInterface::ETChangeStatus(NULL,mtick);
}

xmlNodePtr TETChangeStatusList::addTicket(const TETChangeStatusKey &key,
                                        const Ticketing::Ticket &tick)
{
  if ((*this)[key].empty() ||
      (*this)[key].back().first.size()>=MAX_TICKETS_IN_TLG)
  {
    (*this)[key].push_back(TETChangeStatusItem());
  }

  TETChangeStatusItem &ltick=(*this)[key].back();
  if (ltick.second.docPtr()==NULL)
  {
    ltick.second.set("context");
    if (ltick.second.docPtr()==NULL)
      throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
    NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
  }
  ltick.first.push_back(tick);
  return NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
}

void ETStatusInterface::ETCheckStatus(int point_id,
                                      xmlDocPtr ediResDocPtr,
                                      bool check_connect,
                                      TETChangeStatusList &mtick)
{
  //mtick.clear(); добавляем уже к заполненному

  if (ediResDocPtr==NULL) return;
  if (TReqInfo::Instance()->duplicate) return;

  TFltParams fltParams;
  if (fltParams.get(point_id))
  {
    try
    {
      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.ets_no_interact)
      {
        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText=
          "SELECT pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.class, "
          "       pax.ticket_no, pax.coupon_no, "
          "       pax.refuse, pax.pr_brd, "
          "       pax.grp_id, pax.pax_id, pax.reg_no, "
          "       pax.surname, pax.name, pax.pers_type "
          "FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax.ticket_rem='TKNE' AND "
          "      pax.ticket_no=:ticket_no AND "
          "      pax.coupon_no=:coupon_no";
        Qry.DeclareVariable("ticket_no",otString);
        Qry.DeclareVariable("coupon_no",otInteger);

        TETChangeStatusKey key;
        bool init_edi_addrs=false;

        xmlNodePtr ticketNode=NodeAsNode("/context/tickets",ediResDocPtr);
        for(ticketNode=ticketNode->children;ticketNode!=NULL;ticketNode=ticketNode->next)
        {
          //цикл по билетам
          xmlNodePtr node2=ticketNode->children;
          if (GetNodeFast("coupon_status",node2)==NULL) continue;
          if (NodeAsIntegerFast("prior_point_id",node2,
                                NodeAsIntegerFast("point_id",node2))!=point_id) continue;

          string ticket_no=NodeAsStringFast("ticket_no",node2);
          int coupon_no=NodeAsIntegerFast("coupon_no",node2);

          string airp_dep=NodeAsStringFast("prior_airp_dep",node2,
                                           (char*)NodeAsStringFast("airp_dep",node2));
          string airp_arv=NodeAsStringFast("prior_airp_arv",node2,
                                           (char*)NodeAsStringFast("airp_arv",node2));
          CouponStatus status=CouponStatus::fromDispCode(NodeAsStringFast("coupon_status",node2));
          CouponStatus prior_status=CouponStatus::fromDispCode(NodeAsStringFast("prior_coupon_status",node2));
          //надо вычислить реальный статус
          Qry.SetVariable("ticket_no", ticket_no);
          Qry.SetVariable("coupon_no", coupon_no);
          Qry.Execute();

          CouponStatus real_status(CouponStatus::OriginalIssue);
          if (!Qry.Eof)
            real_status=calcPaxCouponStatus(Qry.FieldAsString("refuse"),
                                            Qry.FieldAsInteger("pr_brd")!=0,
                                            fltParams.in_final_status);

          if (status==real_status) continue;

          if (!init_edi_addrs)
          {
            key.airline_oper=fltParams.fltInfo.airline;
            if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
            {
              ostringstream flt;
              flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                  << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
              throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                               LParams() << LParam("flight", flt.str()));
            }
            init_edi_addrs=true;
          }
          key.coupon_status=real_status->codeInt();

          ProgTrace(TRACE5,"status=%s prior_status=%s real_status=%s",
                           status->dispCode(),prior_status->dispCode(),real_status->dispCode());
          Coupon_info ci (coupon_no,real_status);

          TDateTime scd_local=UTCToLocal(fltParams.fltInfo.scd_out,
                                         AirpTZRegion(fltParams.fltInfo.airp));
          ptime scd(DateTimeToBoost(scd_local));
          Itin itin(fltParams.fltInfo.airline,         //marketing carrier
                  "",                                  //operating carrier
                  fltParams.fltInfo.flt_no,0,
                  SubClass(),
                  scd.date(),
                  time_duration(not_a_date_time), // not a date time
                  airp_dep,
                  airp_arv);
          Coupon cpn(ci,itin);

          list<Coupon> lcpn;
          lcpn.push_back(cpn);
          xmlNodePtr node=mtick.addTicket(key, Ticket(ticket_no, lcpn));

          NewTextChild(node,"ticket_no",ticket_no);
          NewTextChild(node,"coupon_no",coupon_no);
          NewTextChild(node,"point_id",point_id);
          NewTextChild(node,"airp_dep",airp_dep);
          NewTextChild(node,"airp_arv",airp_arv);
          NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
          if (GetNodeFast("grp_id",node2)!=NULL)
          {
            NewTextChild(node,"grp_id",NodeAsIntegerFast("grp_id",node2));
            NewTextChild(node,"pax_id",NodeAsIntegerFast("pax_id",node2));
            if (GetNodeFast("reg_no",node2)!=NULL)
              NewTextChild(node,"reg_no",NodeAsIntegerFast("reg_no",node2));
            NewTextChild(node,"pax_full_name",NodeAsStringFast("pax_full_name",node2));
            NewTextChild(node,"pers_type",NodeAsStringFast("pers_type",node2));
          }

          ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                           ticket_no.c_str(),
                           coupon_no,
                           real_status->dispCode());
        }
      }
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    }
  }
}

void TEMDChangeStatusList::addEMD(const TEMDChangeStatusKey &key,
                                  const TEMDCtxtItem &item)
{
  TEMDChangeStatusList::iterator i=find(key);
  if (i==end()) i=insert(make_pair(key, list<TEMDChangeStatusItem>())).first;
  if (i==end()) throw EXCEPTIONS::Exception("%s: i==end()", __FUNCTION__);

  list<TEMDChangeStatusItem>::iterator j=i->second.insert(i->second.end(), TEMDChangeStatusItem());
  j->emd=Ticketing::TicketCpn_t(item.asvc.emd_no, item.asvc.emd_coupon);
  xmlNodePtr emdocsNode=NULL;
  if (j->ctxt.docPtr()==NULL)
  {
    j->ctxt.set("context");
    if (j->ctxt.docPtr()==NULL) throw EXCEPTIONS::Exception("%s: j->ctxt.docPtr()==NULL", __FUNCTION__);
    emdocsNode=NewTextChild(NodeAsNode("/context",j->ctxt.docPtr()),"emdocs");
  }
  else
    emdocsNode=NodeAsNode("/context/emdocs",j->ctxt.docPtr());

  item.toXML(NewTextChild(emdocsNode, "emdoc"));
}


void EMDStatusInterface::EMDCheckStatus(const int grp_id,
                                        const CheckIn::PaidBagEMDList &prior_emds,
                                        TEMDChangeStatusList &emdList)
{
  if (TReqInfo::Instance()->duplicate) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT point_dep FROM pax_grp WHERE pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return;
  int point_id=Qry.FieldAsInteger("point_dep");

  TFltParams fltParams;
  if (!fltParams.get(point_id)) return;
  if (fltParams.eds_no_interact) return;

  list<TEMDCtxtItem> emds;
  GetBoundEMDStatusList(grp_id, fltParams.in_final_status, emds);
  string flight=GetTripName(fltParams.fltInfo,ecNone,true,false);

  for(list<TEMDCtxtItem>::iterator e=emds.begin(); e!=emds.end(); ++e)
  {
    CheckIn::PaidBagEMDList::const_iterator pe=prior_emds.begin();
    for(; pe!=prior_emds.end(); ++pe)
      if (e->asvc.emd_no==pe->first.emd_no &&
          e->asvc.emd_coupon==pe->first.emd_coupon) break;
    if (pe!=prior_emds.end()) continue; //уже был раньше введен EMD

    e->point_id=point_id;
    e->flight=flight;
    if (e->paxUnknown()) throw UserException("MSG.EMD_MANUAL_INPUT_TEMPORARILY_UNAVAILABLE",
                                             LParams() << LParam("emd", e->asvc.no_str()));

    if (e->status==CouponStatus::OriginalIssue ||
        e->status==CouponStatus::Unavailable) continue;  //пассажир разрегистрируется


    TEMDChangeStatusKey key;
    key.airline_oper=fltParams.fltInfo.airline;
    key.flt_no_oper=fltParams.fltInfo.flt_no;
    key.coupon_status=e->status;

    emdList.addEMD(key, *e);
  };

}

bool EMDStatusInterface::EMDChangeStatus(const edifact::KickInfo &kickInfo,
                                         const TEMDChangeStatusList &emdList)
{
  bool result=false;

  TReqInfo& reqInfo = *(TReqInfo::Instance());

  try
  {
    for(TEMDChangeStatusList::const_iterator i=emdList.begin(); i!=emdList.end(); ++i)
    {
      for(list<TEMDChangeStatusItem>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
      {
        xmlNodePtr rootNode=NodeAsNode("/context",j->ctxt.docPtr());

        if (kickInfo.reqCtxtId!=ASTRA::NoExists)
          SetProp(rootNode,"req_ctxt_id",kickInfo.reqCtxtId);
        TEdiOriginCtxt::toXML(rootNode);

        string ediCtxt=XMLTreeToText(j->ctxt.docPtr());
        //ProgTrace(TRACE5, "ediCosCtxt=%s", ediCtxt.c_str());

        const OrigOfRequest &org=reqInfo.desk.code.empty()?OrigOfRequest(i->first.airline_oper):
                                                           OrigOfRequest(i->first.airline_oper, reqInfo);

        edifact::EmdCOSParams cosParams(org,
                                        ediCtxt,
                                        kickInfo,
                                        i->first.airline_oper,
                                        Ticketing::FlightNum_t(i->first.flt_no_oper),
                                        j->emd.ticket(),
                                        j->emd.cpn(),
                                        i->first.coupon_status);

        edifact::EmdCOSRequest ediReq(cosParams);
        ediReq.sendTlg();

        LogTrace(TRACE5) << __FUNCTION__ << ": " << i->first.traceStr() << " " << j->traceStr();

        result=true;
      };
    };
  }
  catch(Ticketing::RemoteSystemContext::system_not_found &e)
  {
    throw AstraLocale::UserException("MSG.EMD.EDS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                     LEvntPrms() << PrmFlight("flight", e.airline(), e.flNum()?e.flNum().get():ASTRA::NoExists, "") );
  };

  return result;
}

void ETStatusInterface::ETCheckStatus(int id,
                                      TETCheckStatusArea area,
                                      int check_point_id,  //м.б. NoExists
                                      bool check_connect,
                                      TETChangeStatusList &mtick,
                                      bool before_checkin)
{
  //mtick.clear(); добавляем уже к заполненному
  if (TReqInfo::Instance()->duplicate) return;
  int point_id=NoExists;
  TQuery Qry(&OraSession);
  Qry.Clear();
  switch (area)
  {
    case csaFlt:
      point_id=id;
      break;
    case csaGrp:
      Qry.SQLText="SELECT point_dep FROM pax_grp WHERE pax_grp.grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,id);
      Qry.Execute();
      if (!Qry.Eof) point_id=Qry.FieldAsInteger("point_dep");
      break;
    case csaPax:
      Qry.SQLText="SELECT point_dep FROM pax_grp,pax WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,id);
      Qry.Execute();
      if (!Qry.Eof) point_id=Qry.FieldAsInteger("point_dep");
      break;
    default: ;
  }

  TFltParams fltParams;
  if (point_id!=NoExists && fltParams.get(point_id))
  {
    try
    {
      if (check_point_id!=NoExists && check_point_id!=point_id) check_point_id=point_id;

      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.ets_no_interact)
      {
        Qry.Clear();
        ostringstream sql;
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
        //из двух пассажиров с одинаковым билетом/купоном приоритетным является неразрегистрированный
        sql << "ORDER BY pax.ticket_no,pax.coupon_no,DECODE(pax.refuse,NULL,0,1)";

        Qry.SQLText=sql.str().c_str();
        Qry.Execute();
        if (!Qry.Eof)
        {
          string ticket_no;
          int coupon_no=-1;
          TETChangeStatusKey key;
          bool init_edi_addrs=false;
          for(;!Qry.Eof;Qry.Next())
          {
            if (ticket_no==Qry.FieldAsString("ticket_no") &&
                coupon_no==Qry.FieldAsInteger("coupon_no")) continue; //дублирование билетов

            ticket_no=Qry.FieldAsString("ticket_no");
            coupon_no=Qry.FieldAsInteger("coupon_no");

            string airp_dep=Qry.FieldAsString("airp_dep");
            string airp_arv=Qry.FieldAsString("airp_arv");

            CouponStatus status;
            if (Qry.FieldIsNULL("coupon_status"))
              status=CouponStatus(CouponStatus::OriginalIssue);//CouponStatus::Notification ???
            else
              status=CouponStatus::fromDispCode(Qry.FieldAsString("coupon_status"));

            CouponStatus real_status;
            real_status=calcPaxCouponStatus(Qry.FieldAsString("refuse"),
                                            Qry.FieldAsInteger("pr_brd")!=0,
                                            fltParams.in_final_status);

            if (status!=real_status ||
                (!Qry.FieldIsNULL("tick_point_id") &&
                 (Qry.FieldAsInteger("tick_point_id")!=point_id ||
                  Qry.FieldAsString("tick_airp_dep")!=airp_dep ||
                  Qry.FieldAsString("tick_airp_arv")!=airp_arv)))
            {
              if (!init_edi_addrs)
              {
                key.airline_oper=fltParams.fltInfo.airline;
                if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
                {
                  ostringstream flt;
                  flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                      << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
                  throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                                   LParams() << LParam("flight", flt.str()));
                }
                init_edi_addrs=true;
              }
              key.coupon_status=real_status->codeInt();

              ProgTrace(TRACE5,"status=%s real_status=%s",status->dispCode(),real_status->dispCode());
              Coupon_info ci (coupon_no,real_status);

              TDateTime scd_local=UTCToLocal(fltParams.fltInfo.scd_out,
                                             AirpTZRegion(fltParams.fltInfo.airp));
              ptime scd(DateTimeToBoost(scd_local));
              Itin itin(fltParams.fltInfo.airline,         //marketing carrier
                        "",                                  //operating carrier
                        fltParams.fltInfo.flt_no,0,
                        SubClass(),
                        scd.date(),
                        time_duration(not_a_date_time), // not a date time
                        airp_dep,
                        airp_arv);
              Coupon cpn(ci,itin);

              list<Coupon> lcpn;
              lcpn.push_back(cpn);
              xmlNodePtr node=mtick.addTicket(key, Ticket(ticket_no, lcpn));

              NewTextChild(node,"ticket_no",ticket_no);
              NewTextChild(node,"coupon_no",coupon_no);
              NewTextChild(node,"point_id",point_id);
              NewTextChild(node,"airp_dep",airp_dep);
              NewTextChild(node,"airp_arv",airp_arv);
              NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
              NewTextChild(node,"grp_id",Qry.FieldAsInteger("grp_id"));
              NewTextChild(node,"pax_id",Qry.FieldAsInteger("pax_id"));
              if (!before_checkin)
                NewTextChild(node,"reg_no",Qry.FieldAsInteger("reg_no"));
              ostringstream pax;
              pax << Qry.FieldAsString("surname")
                  << (Qry.FieldIsNULL("name")?"":" ") << Qry.FieldAsString("name");
              NewTextChild(node,"pax_full_name",pax.str());
              NewTextChild(node,"pers_type",Qry.FieldAsString("pers_type"));

              NewTextChild(node,"prior_coupon_status",status->dispCode());
              if (!Qry.FieldIsNULL("tick_point_id"))
              {
                NewTextChild(node,"prior_point_id",Qry.FieldAsInteger("tick_point_id"));
                NewTextChild(node,"prior_airp_dep",Qry.FieldAsString("tick_airp_dep"));
                NewTextChild(node,"prior_airp_arv",Qry.FieldAsString("tick_airp_arv"));
              }

              ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                               ticket_no.c_str(),
                               coupon_no,
                               real_status->dispCode());
            }
          }
        }
      }
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    }
  }

  if (check_point_id!=NoExists && fltParams.get(check_point_id))
  {
    //проверка билетов, пассажиры которых разрегистрированы (по всему рейсу)
    try
    {
      CouponStatus real_status=CouponStatus(CouponStatus::OriginalIssue);
      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.ets_no_interact)
      {
        Qry.Clear();
        Qry.SQLText=
          "SELECT etickets.ticket_no, etickets.coupon_no, "
          "       etickets.airp_dep, etickets.airp_arv, "
          "       etickets.coupon_status "
          "FROM etickets,pax "
          "WHERE etickets.ticket_no=pax.ticket_no(+) AND "
          "      etickets.coupon_no=pax.coupon_no(+) AND "
          "      etickets.point_id=:point_id AND "
          "      pax.pax_id IS NULL AND "
          "      etickets.coupon_status IS NOT NULL";
        Qry.CreateVariable("point_id",otInteger,check_point_id);
        Qry.Execute();
        if (!Qry.Eof)
        {
          TETChangeStatusKey key;
          key.airline_oper=fltParams.fltInfo.airline;
          if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
          {
            ostringstream flt;
            flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
            throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
                                               LParams() << LParam("flight", flt.str()));
          }
          key.coupon_status=real_status->codeInt();

          for(;!Qry.Eof;Qry.Next())
          {
            string ticket_no=Qry.FieldAsString("ticket_no");
            int coupon_no=Qry.FieldAsInteger("coupon_no");

            CouponStatus status=CouponStatus::fromDispCode(Qry.FieldAsString("coupon_status"));

            Coupon_info ci (coupon_no,real_status);
            TDateTime scd_local=UTCToLocal(fltParams.fltInfo.scd_out,
                                           AirpTZRegion(fltParams.fltInfo.airp));
            ptime scd(DateTimeToBoost(scd_local));
            Itin itin(fltParams.fltInfo.airline,             //marketing carrier
                      "",                                  //operating carrier
                      fltParams.fltInfo.flt_no,0,
                      SubClass(),
                      scd.date(),
                      time_duration(not_a_date_time), // not a date time
                      Qry.FieldAsString("airp_dep"),
                      Qry.FieldAsString("airp_arv"));
            Coupon cpn(ci,itin);
            list<Coupon> lcpn;
            lcpn.push_back(cpn);
            xmlNodePtr node=mtick.addTicket(key, Ticket(ticket_no, lcpn));

            NewTextChild(node,"ticket_no",ticket_no);
            NewTextChild(node,"coupon_no",coupon_no);
            NewTextChild(node,"point_id",check_point_id);
            NewTextChild(node,"airp_dep",Qry.FieldAsString("airp_dep"));
            NewTextChild(node,"airp_arv",Qry.FieldAsString("airp_arv"));
            NewTextChild(node,"flight",GetTripName(fltParams.fltInfo,ecNone,true,false));
            NewTextChild(node,"prior_coupon_status",status->dispCode());

            ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                             ticket_no.c_str(),
                             coupon_no,
                             real_status->dispCode());
          }
        }
      }
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), check_point_id);
    }
  }
}

bool ETStatusInterface::ETChangeStatus(const xmlNodePtr reqNode,
                                       const TETChangeStatusList &mtick)
{
  bool result=false;

  if (!mtick.empty())
  {
    const edifact::KickInfo &kickInfo=
        reqNode!=NULL?edifact::KickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)), "ChangeStatus"):
                      edifact::KickInfo();
    result=ETChangeStatus(kickInfo, mtick);
  }
  return result;
}

bool ETStatusInterface::ETChangeStatus(const edifact::KickInfo &kickInfo,
                                       const TETChangeStatusList &mtick)
{
  bool result=false;

  if (!mtick.empty())
  {
    string oper_carrier;
    for(TETChangeStatusList::const_iterator i=mtick.begin();i!=mtick.end();i++)
    {
      for(vector<TETChangeStatusItem>::const_iterator j=i->second.begin();j!=i->second.end();j++)
      {
        const TTicketList &ltick=j->first;
        if (ltick.empty()) continue;

        if (i->first.airline_oper.empty())
          throw EXCEPTIONS::Exception("ETChangeStatus: unkown operation carrier");
        oper_carrier=i->first.airline_oper;
        /*
      try
      {
        TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
        if (!row.code_lat.empty()) oper_carrier=row.code_lat;
      }
      catch(EBaseTableError) {}
      */
        if (i->first.addrs.first.empty() ||
            i->first.addrs.second.empty())
          throw EXCEPTIONS::Exception("ETChangeStatus: edifact UNB-adresses not defined");
        set_edi_addrs(i->first.addrs);

        ProgTrace(TRACE5,"ETChangeStatus: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                  oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

        TReqInfo& reqInfo = *(TReqInfo::Instance());
        xmlNodePtr rootNode=NodeAsNode("/context",j->second.docPtr());

        if (kickInfo.reqCtxtId!=ASTRA::NoExists)
          SetProp(rootNode,"req_ctxt_id",kickInfo.reqCtxtId);
        TEdiOriginCtxt::toXML(rootNode);

        string ediCtxt=XMLTreeToText(j->second.docPtr());

        if (reqInfo.desk.code.empty())
        {
          //не запрос
          OrigOfRequest org(oper_carrier);
          ChangeStatus::ETChangeStatus(org, ltick, ediCtxt, kickInfo);
        }
        else
        {
          OrigOfRequest org(oper_carrier,reqInfo);
          ChangeStatus::ETChangeStatus(org, ltick, ediCtxt, kickInfo);
        }

        result=true;
      }
    }
  }
  return result;
}

void EMDSearchInterface::EMDTextView(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
  //NewTextChild(resNode,"text","...");
}


void EMDStatusInterface::ChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << __FUNCTION__;

    Ticketing::TicketNum_t emdDocNum(NodeAsString("EmdNoEdit", reqNode));
    Ticketing::CouponNum_t emdCpnNum(NodeAsInteger("CpnNoEdit", reqNode));
    Ticketing::CouponStatus emdCpnStatus;
    emdCpnStatus = Ticketing::CouponStatus::fromDispCode(NodeAsString("CpnStatusEdit", reqNode));
    int pointId = NodeAsInteger("point_id",reqNode);

    TTripInfo info;
    checkEDSInteract(pointId, true, info);
    std::string airline = getTripAirline(info);
    Ticketing::FlightNum_t flNum = getTripFlightNum(info);
    OrigOfRequest org(airline, *TReqInfo::Instance());
    edifact::KickInfo kickInfo(ASTRA::NoExists, "EMDStatus");

    edifact::EmdCOSParams cosParams(org, "", kickInfo, airline, flNum, emdDocNum, emdCpnNum, emdCpnStatus);
    edifact::EmdCOSRequest ediReq(cosParams);
    ediReq.sendTlg();
}

void EMDStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(KickHandler);
    using namespace edifact;
    pRemoteResults res = RemoteResults::readSingle();
    LogTrace(TRACE3) << "RemoteResults::Status: " << res->status();
    // TODO add kick handling
    FuncOut(KickHandler);
}

void ChangeStatusInterface::ChangeStatus(const xmlNodePtr reqNode,
                                         const TChangeStatusList &info)
{
  bool existsET=false;
  bool existsEMD=false;

  if (!info.empty())
  {
    const edifact::KickInfo &kickInfo=
        reqNode!=NULL?edifact::KickInfo(AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc)), "ChangeStatus"):
                      edifact::KickInfo();
    existsET=ETStatusInterface::ETChangeStatus(kickInfo,info.ET);
    existsEMD=EMDStatusInterface::EMDChangeStatus(kickInfo,info.EMD);
  };
  if (existsET)
  {
    if (existsEMD)
      AstraLocale::showProgError("MSG.ETS_EDS_CONNECT_ERROR");
    else
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
  }
  else
  {
    if (existsEMD)
      AstraLocale::showProgError("MSG.EDS_CONNECT_ERROR");
    else
      throw EXCEPTIONS::Exception("ChangeStatusInterface::ChangeStatus: empty info");
  };
}

void ChangeStatusInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string context;
    TReqInfo *reqInfo = TReqInfo::Instance();
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
      //если эмулируем запрос web-регистрации с терминала - то делаем подмену client_type
      xmlNodePtr ifaceNode=GetNode("/term/query/@id",termReqCtxt.docPtr());
      if (ifaceNode!=NULL && strcmp(NodeAsString(ifaceNode), WEB_JXT_IFACE_ID)==0)
      {
        //это web-регистрация
        if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;
      }
      string termReqName=(char*)(termReqNode->name);

      if (reqInfo->client_type==ctWeb ||
          reqInfo->client_type==ctMobile ||
          reqInfo->client_type==ctKiosk) {
        xmlNodePtr node = NodeAsNode("/term/query",reqNode->doc);
        xmlUnlinkNode( reqNode );
        xmlFreeNode( reqNode );
        reqNode = NewTextChild( node, termReqName.c_str() );
      }


      bool defer_etstatus=(termReqName=="ChangePaxStatus" ||
                           termReqName=="ChangeGrpStatus" ||
                           termReqName=="ChangeFltStatus");
      ProgTrace( TRACE5, "termReqName=%s", termReqName.c_str() );

      xmlNodePtr ediResNode=NodeAsNode("/context",ediResCtxt.docPtr());


      map<string, pair< set<string>, vector< pair<string,string> > > > errors; //flight,множество global_error, вектор пар pax+ticket/coupon_error
      map<int, map<int, AstraLocale::LexemaData> > segs;

      xmlNodePtr emdNode=GetNode("emdocs", ediResNode);
      if (emdNode!=NULL) emdNode=emdNode->children;
      for(; emdNode!=NULL; emdNode=emdNode->next)
      {
        TEMDCtxtItem EMDCtxt;
        EMDCtxt.fromXML(emdNode);
        string pax;
        if (!EMDCtxt.paxUnknown())
          pax=getLocaleText("WRAP.PASSENGER",
                            LParams() << LParam("name", EMDCtxt.pax.full_name())
                                      << LParam("pers_type",ElemIdToCodeNative(etPersType,EncodePerson(EMDCtxt.pax.pers_type))));

        EdiErrorList errList;
        GetEdiError(emdNode, errList);
        if (!errList.empty())
        {
          pair< set<string>, vector< pair<string,string> > > &err=errors[EMDCtxt.flight];

          for(EdiErrorList::const_iterator e=errList.begin(); e!=errList.end(); ++e)
            if (e->second) //global
            {
              err.first.insert(getLocaleText(e->first));
              //ошибка уровня сегмента
              segs[EMDCtxt.point_id][ASTRA::NoExists]=e->first;
            }
            else
            {
              err.second.push_back(make_pair(pax,getLocaleText(e->first)));
              //ошибка уровня пассажира
              segs[EMDCtxt.point_id][EMDCtxt.pax.id]=e->first;
            };
        };
      };

      xmlNodePtr ticketNode=GetNode("tickets", ediResNode);
      if (ticketNode!=NULL) ticketNode=ticketNode->children;
      for(;ticketNode!=NULL;ticketNode=ticketNode->next)
      {
        string flight=NodeAsString("flight",ticketNode);
        string pax;
        if (GetNode("pax_full_name",ticketNode)!=NULL&&
            GetNode("pers_type",ticketNode)!=NULL)
          pax=getLocaleText("WRAP.PASSENGER", LParams() << LParam("name",NodeAsString("pax_full_name",ticketNode))
                                                        << LParam("pers_type",ElemIdToCodeNative(etPersType,NodeAsString("pers_type",ticketNode))));
        int point_id=NodeAsInteger("point_id",ticketNode);
        int pax_id=ASTRA::NoExists;
        if (GetNode("pax_id",ticketNode)!=NULL)
          pax_id=NodeAsInteger("pax_id",ticketNode);

        bool tick_event=false;
        for(xmlNodePtr node=ticketNode->children;node!=NULL;node=node->next)
        {
          if (strcmp((const char*)node->name,"coupon_status")==0) tick_event=true;

          if (!(strcmp((const char*)node->name,"global_error")==0 ||
                strcmp((const char*)node->name,"ticket_error")==0 ||
                strcmp((const char*)node->name,"coupon_error")==0)) continue;

          tick_event=true;

          pair< set<string>, vector< pair<string,string> > > &err=errors[flight];

          LexemaData lexemeData;
          LexemeDataFromXML(node, lexemeData);
          if (strcmp((const char*)node->name,"global_error")==0)
          {
            err.first.insert(getLocaleText(lexemeData));
            //ошибка уровня сегмента
            segs[point_id][ASTRA::NoExists]=lexemeData;
          }
          else
          {
            err.second.push_back(make_pair(pax,getLocaleText(lexemeData)));
            //ошибка уровня пассажира
            segs[point_id][pax_id]=lexemeData;
          }
        }
        if (!tick_event)
        {
          ostringstream ticknum;
          ticknum << NodeAsString("ticket_no",ticketNode) << "/"
                  << NodeAsInteger("coupon_no",ticketNode);

          LexemaData lexemeData;
          lexemeData.lexema_id="MSG.ETICK.CHANGE_STATUS_UNKNOWN_RESULT";
          lexemeData.lparams << LParam("ticknum",ticknum.str());
          string err_locale=getLocaleText(lexemeData);

          pair< set<string>, vector< pair<string,string> > > &err=errors[flight];
          err.second.push_back(make_pair(pax,err_locale));
          segs[point_id][pax_id]=lexemeData;
        }
      }

      if (!errors.empty())
      {
        bool use_flight=(GetNode("segments",termReqNode)!=NULL &&
                         NodeAsNode("segments/segment",termReqNode)->next!=NULL);  //определим по запросу TERM_REQUEST;
        map<string, pair< set<string>, vector< pair<string,string> > > >::iterator i;
        if ((reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION) && !defer_etstatus) ||
            reqInfo->client_type == ctWeb ||
            reqInfo->client_type == ctMobile ||
            reqInfo->client_type == ctKiosk)
        {
          if (reqInfo->client_type == ctWeb ||
              reqInfo->client_type == ctMobile ||
              reqInfo->client_type == ctKiosk)
          {
            if (!segs.empty())
              CheckIn::showError(segs);
            else
              AstraLocale::showError( "MSG.ETICK.CHANGE_STATUS_ERROR", LParams() << LParam("ticknum","") << LParam("error","") ); //!!! надо выводить номер билета и ошибку
          }
          else
          {
            ostringstream msg;
            for(i=errors.begin();i!=errors.end();i++)
            {
              if (use_flight)
                msg << getLocaleText("WRAP.FLIGHT", LParams() << LParam("flight",i->first) << LParam("text","") ) << std::endl;
              for(set<string>::const_iterator j=i->second.first.begin(); j!=i->second.first.end(); ++j)
              {
                if (use_flight) msg << "     ";
                msg << *j << std::endl;
              }
              for(vector< pair<string,string> >::const_iterator j=i->second.second.begin(); j!=i->second.second.end(); ++j)
              {
                if (use_flight) msg << "     ";
                if (!(j->first.empty()))
                {
                  msg << j->first << std::endl
                      << "     ";
                  if (use_flight) msg << "     ";
                }
                msg << j->second << std::endl;
              }
            }
            NewTextChild(resNode,"ets_error",msg.str());
          }
          //откат всех подтвержденных статусов
          ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
          return;
        }
        else
        {
          //отката не делаем если раздельное подтверждение или терминал несовместим
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.first.empty())
              throw AstraLocale::UserException(*i->second.first.begin());
          for(i=errors.begin();i!=errors.end();i++)
            if (!i->second.second.empty())
              throw AstraLocale::UserException((i->second.second.begin())->second);
        }
      }

      if (defer_etstatus) return;

      try
      {
        if (reqInfo->client_type==ctTerm)
        {
          if (termReqName=="TCkinSavePax" ||
              termReqName=="TCkinSaveUnaccompBag")
          {
            if (!CheckInInterface::SavePax(termReqNode, ediResNode, resNode))
            {
              //откатываем статусы так как запись группы так и не прошла
              ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
              return;
            }
          }
        }

        if (reqInfo->client_type==ctWeb ||
            reqInfo->client_type==ctMobile ||
            reqInfo->client_type==ctKiosk)
        {
          if (termReqName=="SavePax")
          {
            if (!AstraWeb::WebRequestsIface::SavePax(termReqNode, ediResNode, resNode))
            {
              //откатываем статусы так как запись группы так и не прошла
              ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
              return;
            }
          }
        }
      }
      catch(ServerFramework::Exception &e)
      {
        OraSession.Rollback();
        jxtlib::JXTLib::Instance()->GetCallbacks()->HandleException(&e);
        ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
      }
    }
}

