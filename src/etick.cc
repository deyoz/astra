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
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "checkin.h"
#include "web_main.h"
#include "term_version.h"
#include "jxtlib/jxtlib.h"
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
using namespace AstraLocale;
using namespace BASIC;
using namespace EXCEPTIONS;

#define MAX_TICKETS_IN_TLG 5

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
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TTripInfo info;
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.airp=Qry.FieldAsString("airp");
  if (GetTripSets(tsETLOnly,info))
    throw AstraLocale::UserException("MSG.ETICK.INTERACTIVE_MODE_NOT_ALLOWED");

  pair<string,string> edi_addrs;
  if (!get_et_addr_set(info.airline,info.flt_no,edi_addrs))
    throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
    	                               LParams() << LParam("flight", ElemIdToCodeNative(etAirline,info.airline) + IntToString(info.flt_no)));

  set_edi_addrs(edi_addrs);

  string oper_carrier=info.airline;
  /*
  try
  {
    TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",oper_carrier);
    if (!row.code_lat.empty()) oper_carrier=row.code_lat;
  }
  catch(EBaseTableError) {};
  */
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
  };
};

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
    TReqInfo::Instance()->MsgToLog( "Установлен режим временной отмены интерактива с СЭБ", evtFlt, point_id );
  }
  else
    if (old_pr_etstatus==new_pr_etstatus)
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_IN_THIS_MODE_ALREADY");
    else
      throw AstraLocale::UserException("MSG.ETICK.FLIGHT_CANNOT_BE_SET_IN_THIS_MODE");
};

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
  };
  bool processed=false;
  TChangeStatusList mtick;
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
      int check_point_id=NoExists;
      if (node!=NULL) check_point_id=NodeAsInteger(node);
      if (ETStatusInterface::ETCheckStatus(id,area,check_point_id,false,mtick))
      {
        processed=true;
      };
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
        };
        Qry.CreateVariable("id",otInteger,id);
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
  };

  if (processed)
  {
    int req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc));
    if (!ETStatusInterface::ETChangeStatus(req_ctxt,mtick))
      throw EXCEPTIONS::Exception("ChangeAreaStatus: Wrong variable 'processed'");

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
      };
      string termReqName=(char*)(termReqNode->name);

      if (reqInfo->client_type==ctWeb ||
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


      map<string, pair< vector<string>, vector< pair<string,string> > > > errors; //flight,вектор global_error, вектор пар pax+ticket/coupon_error
      map<int, map<int, AstraLocale::LexemaData> > segs;
      
      xmlNodePtr ticketNode=NodeAsNode("tickets",ediResNode)->children;
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

          pair< vector<string>, vector< pair<string,string> > > &err=errors[flight];

          if (strcmp((const char*)node->name,"global_error")==0)
          {
            LexemaData lexemeData;
            LexemeDataFromXML(node, lexemeData);
            string err_locale=getLocaleText(lexemeData);
            if (find(err.first.begin(),err.first.end(),err_locale)==err.first.end())
              err.first.push_back(err_locale);
            //ошибка уровня сегмента
            segs[point_id][ASTRA::NoExists]=lexemeData;
          }
          else
          {
            LexemaData lexemeData;
            LexemeDataFromXML(node, lexemeData);
            err.second.push_back(make_pair(pax,getLocaleText(lexemeData)));
            //ошибка уровня пассажира
            segs[point_id][pax_id]=lexemeData;
          };
        };
        if (!tick_event)
        {
          ostringstream ticknum;
          ticknum << NodeAsString("ticket_no",ticketNode) << "/"
                  << NodeAsInteger("coupon_no",ticketNode);
                  
          LexemaData lexemeData;
          lexemeData.lexema_id="MSG.ETICK.CHANGE_STATUS_UNKNOWN_RESULT";
          lexemeData.lparams << LParam("ticknum",ticknum.str());
          string err_locale=getLocaleText(lexemeData);
                  
          pair< vector<string>, vector< pair<string,string> > > &err=errors[flight];
          err.second.push_back(make_pair(pax,err_locale));
          segs[point_id][pax_id]=lexemeData;
        };
      };

      if (!errors.empty())
      {
        bool use_flight=(GetNode("segments",termReqNode)!=NULL &&
                         NodeAsNode("segments/segment",termReqNode)->next!=NULL);  //определим по запросу TERM_REQUEST;
        map<string, pair< vector<string>, vector< pair<string,string> > > >::iterator i;
        if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION) && !defer_etstatus ||
        	  reqInfo->client_type == ctWeb ||
        	  reqInfo->client_type == ctKiosk)
        {
          if (reqInfo->client_type == ctWeb ||
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
                  msg << j->first << std::endl
                      << "     ";
                  if (use_flight) msg << "     ";
                };
                msg << j->second << std::endl;
              };
            };
            NewTextChild(resNode,"ets_error",msg.str());
          };
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
        };
      };

      if (defer_etstatus) return;

      try
      {
      	if (reqInfo->client_type==ctTerm)
      	{
          if (termReqName=="SavePax" ||
              termReqName=="SaveUnaccompBag" ||
              termReqName=="TCkinSavePax" ||
              termReqName=="TCkinSaveUnaccompBag")
          {
            if (!CheckInInterface::SavePax(termReqNode, termReqNode, ediResNode, resNode))
            {
              //откатываем статусы так как запись группы так и не прошла
              ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
              return;
            };
          };
        };

        if (reqInfo->client_type==ctWeb ||
            reqInfo->client_type==ctKiosk)
      	{
          if (termReqName=="SavePax")
          {
            if (!AstraWeb::WebRequestsIface::SavePax(termReqNode, ediResNode, resNode))
            {
              //откатываем статусы так как запись группы так и не прошла
              ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
              return;
            };
          };
        };
      }
      catch(ServerFramework::Exception &e)
      {
        OraSession.Rollback();
        jxtlib::JXTLib::Instance()->GetCallbacks()->HandleException(&e);
        ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(),false);
      };
    };
};

void ETStatusInterface::ETRollbackStatus(xmlDocPtr ediResDocPtr,
                                         bool check_connect)
{
  if (ediResDocPtr==NULL) return;

  vector<int> point_ids;

  xmlNodePtr ticketNode=NodeAsNode("/context/tickets",ediResDocPtr);
  for(xmlNodePtr node=ticketNode->children;node!=NULL;node=node->next)
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
  };

  TChangeStatusList mtick;
  for(vector<int>::iterator i=point_ids.begin();i!=point_ids.end();i++)
  {
    ProgTrace(TRACE5,"ETRollbackStatus: rollback point_id=%d",*i);
    ETStatusInterface::ETCheckStatus(*i,ediResDocPtr,false,mtick);
  };
  ETStatusInterface::ETChangeStatus(ASTRA::NoExists,mtick);
};

bool ETStatusInterface::TFltParams::get(int point_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.point_id, points.point_num, points.first_point, points.pr_tranzit, "
    "       points.airline, points.flt_no, points.suffix, points.airp, points.scd_out, "
    "       points.act_out AS real_out, trip_sets.pr_etstatus, trip_sets.et_final_attempt "
    "FROM points,trip_sets "
    "WHERE trip_sets.point_id=points.point_id AND "
    "      points.point_id=:point_id AND points.pr_del>=0 ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  fltInfo.Init(Qry);
  etl_only=GetTripSets(tsETLOnly,fltInfo);
  pr_etstatus=Qry.FieldAsInteger("pr_etstatus");
  et_final_attempt=Qry.FieldAsInteger("et_final_attempt");
  TTripRoute route;
  route.GetRouteAfter(NoExists,
                      point_id,
                      Qry.FieldAsInteger("point_num"),
                      Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                      Qry.FieldAsInteger("pr_tranzit")!=0,
                      trtNotCurrent,
                      trtNotCancelled);

  if (route.empty()) return false;
  //время прибытия в конечный пункт маршрута
  Qry.Clear();
  Qry.SQLText=
    "SELECT NVL(act_in,NVL(est_in,scd_in)) AS real_in FROM points WHERE point_id=:point_id AND pr_del=0";
  Qry.CreateVariable("point_id",otInteger,route.back().point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  TDateTime real_in=ASTRA::NoExists;
  if (!Qry.FieldIsNULL("real_in")) real_in=Qry.FieldAsDateTime("real_in");
  
  in_final_status=fltInfo.real_out!=NoExists && real_in!=NoExists && real_in<NowUTC();
  return true;
};

bool ETStatusInterface::ETCheckStatus(int point_id,
                                      xmlDocPtr ediResDocPtr,
                                      bool check_connect,
                                      TChangeStatusList &mtick)
{
  bool result=false;

  //mtick.clear(); добавляем уже к заполненному

  if (ediResDocPtr==NULL) return result;

  TFltParams fltParams;
  if (fltParams.get(point_id))
  {
    try
    {
      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.etl_only)
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

        TTicketListKey key;
        bool init_edi_addrs=false;

        xmlNodePtr ticketNode=NodeAsNode("/context/tickets",ediResDocPtr);
        for(xmlNodePtr node=ticketNode->children;node!=NULL;node=node->next)
        {
          //цикл по билетам
          xmlNodePtr node2=node->children;
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
          
          CouponStatus real_status;
          if (Qry.Eof || !Qry.FieldIsNULL("refuse"))
            //разрегистрирован
            real_status=CouponStatus(CouponStatus::OriginalIssue);
          else
          {
            if (Qry.FieldAsInteger("pr_brd")==0)
            	//не посажен
              real_status=CouponStatus(CouponStatus::Checked);
            else
            {
              if (!fltParams.in_final_status)
                real_status=CouponStatus(CouponStatus::Boarded);
              else
                real_status=CouponStatus(CouponStatus::Flown);
            };
          };

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
            };
            init_edi_addrs=true;
          };
          key.coupon_status=real_status->codeInt();

          if (mtick[key].empty() ||
              mtick[key].back().first.size()>=MAX_TICKETS_IN_TLG)
          {
            TTicketListCtxt ltick;
            mtick[key].push_back(ltick);
          };

          TTicketListCtxt &ltick=mtick[key].back();
          if (ltick.second.docPtr()==NULL)
          {
            ltick.second.set("UTF-8","context");
            if (ltick.second.docPtr()==NULL)
              throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
            NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
          };

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

          Ticket tick(ticket_no, lcpn);
          ltick.first.push_back(tick);
          result=true;

          xmlNodePtr node=NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
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
          };

          ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                           ticket_no.c_str(),
                           coupon_no,
                           real_status->dispCode());
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };

  return result;
};

bool ETStatusInterface::ETCheckStatus(int id,
                                      TETCheckStatusArea area,
                                      int check_point_id,  //м.б. NoExists
                                      bool check_connect,
                                      TChangeStatusList &mtick,
                                      bool before_checkin)
{
  bool result=false;

  //mtick.clear(); добавляем уже к заполненному
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
  };
  
  TFltParams fltParams;
  if (point_id!=NoExists && fltParams.get(point_id))
  {
    try
    {
      if (check_point_id!=NoExists && check_point_id!=point_id) check_point_id=point_id;

      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.etl_only)
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
        };
        //из двух пассажиров с одинаковым билетом/купоном приоритетным является неразрегистрированный
        sql << "ORDER BY pax.ticket_no,pax.coupon_no,DECODE(pax.refuse,NULL,0,1)";

        Qry.SQLText=sql.str().c_str();
        Qry.Execute();
        if (!Qry.Eof)
        {
          string ticket_no;
          int coupon_no=-1;
          TTicketListKey key;
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
                if (!fltParams.in_final_status)
                  real_status=CouponStatus(CouponStatus::Boarded);
                else
                  real_status=CouponStatus(CouponStatus::Flown);
              };
            };

            if (status!=real_status ||
                !Qry.FieldIsNULL("tick_point_id") &&
                (Qry.FieldAsInteger("tick_point_id")!=point_id ||
                 Qry.FieldAsString("tick_airp_dep")!=airp_dep ||
                 Qry.FieldAsString("tick_airp_arv")!=airp_arv))
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
                };
                init_edi_addrs=true;
              };
              key.coupon_status=real_status->codeInt();

              if (mtick[key].empty() ||
                  mtick[key].back().first.size()>=MAX_TICKETS_IN_TLG)
              {
                TTicketListCtxt ltick;
                mtick[key].push_back(ltick);
              };

              TTicketListCtxt &ltick=mtick[key].back();
              if (ltick.second.docPtr()==NULL)
              {
                ltick.second.set("UTF-8","context");
                if (ltick.second.docPtr()==NULL)
                  throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
                NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
              };

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

              Ticket tick(ticket_no, lcpn);
              ltick.first.push_back(tick);
              result=true;

              xmlNodePtr node=NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
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
              };

              ProgTrace(TRACE5,"ETCheckStatus %s/%d->%s",
                               ticket_no.c_str(),
                               coupon_no,
                               real_status->dispCode());
            };
          };
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };

  if (check_point_id!=NoExists && fltParams.get(check_point_id))
  {
    //проверка билетов, пассажиры которых разрегистрированы (по всему рейсу)
    try
    {
      CouponStatus real_status=CouponStatus(CouponStatus::OriginalIssue);
      if ((fltParams.pr_etstatus>=0 || check_connect) && !fltParams.etl_only)
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
          TTicketListKey key;
          key.airline_oper=fltParams.fltInfo.airline;
          if (!get_et_addr_set(fltParams.fltInfo.airline,fltParams.fltInfo.flt_no,key.addrs))
          {
            ostringstream flt;
            flt << ElemIdToCodeNative(etAirline,fltParams.fltInfo.airline)
                << setw(3) << setfill('0') << fltParams.fltInfo.flt_no;
            throw AstraLocale::UserException("MSG.ETICK.ETS_ADDR_NOT_DEFINED_FOR_FLIGHT",
             	                               LParams() << LParam("flight", flt.str()));
          };
          key.coupon_status=real_status->codeInt();

          for(;!Qry.Eof;Qry.Next())
          {
            string ticket_no=Qry.FieldAsString("ticket_no");
            int coupon_no=Qry.FieldAsInteger("coupon_no");

            CouponStatus status=CouponStatus::fromDispCode(Qry.FieldAsString("coupon_status"));

            if (mtick[key].empty() ||
                mtick[key].back().first.size()>=MAX_TICKETS_IN_TLG)
            {
              TTicketListCtxt ltick;
              mtick[key].push_back(ltick);
            };

            TTicketListCtxt &ltick=mtick[key].back();
            if (ltick.second.docPtr()==NULL)
            {
              ltick.second.set("UTF-8","context");
              if (ltick.second.docPtr()==NULL)
                throw EXCEPTIONS::Exception("ETCheckStatus: CreateXMLDoc failed");
              NewTextChild(NodeAsNode("/context",ltick.second.docPtr()),"tickets");
            };

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
            Ticket tick(ticket_no, lcpn);
            ltick.first.push_back(tick);
            result=true;

            xmlNodePtr node=NewTextChild(NodeAsNode("/context/tickets",ltick.second.docPtr()),"ticket");
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
          };
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(AstraLocale::UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), check_point_id);
    };
  };
  return result;
}

bool ETStatusInterface::ETChangeStatus(const int reqCtxtId,
                                       const TChangeStatusList &mtick)
{
  bool result=false;

  string oper_carrier;
  for(TChangeStatusList::const_iterator i=mtick.begin();i!=mtick.end();i++)
  {
    for(vector<TTicketListCtxt>::const_iterator j=i->second.begin();j!=i->second.end();j++)
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
      catch(EBaseTableError) {};
      */
      if (i->first.addrs.first.empty() ||
          i->first.addrs.second.empty())
        throw EXCEPTIONS::Exception("ETChangeStatus: edifact UNB-adresses not defined");
      set_edi_addrs(i->first.addrs);

      ProgTrace(TRACE5,"ETChangeStatus: oper_carrier=%s edi_addr=%s edi_own_addr=%s",
                       oper_carrier.c_str(),get_edi_addr().c_str(),get_edi_own_addr().c_str());

      TReqInfo& reqInfo = *(TReqInfo::Instance());
      xmlNodePtr rootNode=NodeAsNode("/context",j->second.docPtr());

      if (reqCtxtId!=ASTRA::NoExists)
        SetProp(rootNode,"req_ctxt_id",reqCtxtId);
      SetProp(rootNode,"desk",reqInfo.desk.code);
      SetProp(rootNode,"user",reqInfo.user.descr);
      SetProp(rootNode,"screen",reqInfo.screen.name);

      string ediCtxt=XMLTreeToText(j->second.docPtr());

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
  };
  return result;
};







