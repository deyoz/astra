#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "edi_utils.h"
#include "misc.h"
#include "date_time.h"
#include "astra_context.h"
#include "tlg/tlg.h"
#include "tlg/remote_results.h"
#include "tlg/request_params.h"
#include <serverlib/internal_msgid.h>
#include <serverlib/ehelpsig.h>
#include <serverlib/posthooks.h>
#include <serverlib/cursctl.h>
#include <serverlib/rip_oci.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/testmode.h>
#include <serverlib/internal_msgid.h>
#include <edilib/EdiSessionTimeOut.h>
#include <edilib/edi_session.h>
#include <edilib/edilib_db_callbacks.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace Ticketing;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace AstraLocale;

namespace AstraEdifact
{

CouponStatus calcPaxCouponStatus(const string& refuse,
                                 bool pr_brd,
                                 bool in_final_status)
{
  CouponStatus real_status;
  if (!refuse.empty())
    //разрегистрирован
    real_status=CouponStatus(CouponStatus::OriginalIssue);
  else
  {
    if (!pr_brd)
      //не посажен
      real_status=CouponStatus(CouponStatus::Checked);
    else
    {
      if (!in_final_status)
        real_status=CouponStatus(CouponStatus::Boarded);
      else
        real_status=CouponStatus(CouponStatus::Flown);
    }
  }
  return real_status;
}

bool TFltParams::get(int point_id)
{
  clear();
  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_id)) return false;
  return get(fltInfo);
}

bool TFltParams::get(const TAdvTripInfo& _fltInfo)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_etstatus, et_final_attempt FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, _fltInfo.point_id);
  Qry.Execute();
  if (Qry.Eof) return false;

  fltInfo=_fltInfo;
  ets_no_exchange=GetTripSets(tsETSNoExchange,fltInfo);
  eds_no_exchange=GetTripSets(tsEDSNoExchange,fltInfo);
  changeETStatusWhileBoarding=GetTripSets(tsChangeETStatusWhileBoarding,fltInfo);
  ets_exchange_status=ETSExchangeStatus::fromDB(Qry);
  et_final_attempt=Qry.FieldAsInteger("et_final_attempt");
  return get(fltInfo, control_method, in_final_status);
}

bool TFltParams::strictlySingleTicketInTlg() const
{
  return control_method || fltInfo.airline=="EL";
}

bool TFltParams::equalETStatus(const Ticketing::CouponStatus& status1,
                               const Ticketing::CouponStatus& status2) const
{
  if (!changeETStatusWhileBoarding &&
      (status1==CouponStatus::Checked || status1==CouponStatus::Boarded) &&
      (status2==CouponStatus::Checked || status2==CouponStatus::Boarded))
    return true;
  return status1==status2;
}

bool TFltParams::get(const TAdvTripInfo& fltInfo,
                     bool &control_method,
                     bool &in_final_status)
{
  control_method=GetTripSets(tsETSControlMethod,fltInfo);
  in_final_status=false;
  if (control_method)
    in_final_status=fltInfo.act_out && fltInfo.act_out.get()!=NoExists &&
                    fltInfo.act_out<NowUTC();
  else
  {
    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        fltInfo.point_id,
                        fltInfo.point_num,
                        fltInfo.first_point,
                        fltInfo.pr_tranzit,
                        trtNotCurrent,
                        trtNotCancelled);

    if (route.empty()) return false;
    //время прибытия в конечный пункт маршрута
    TDateTime real_in=TTripInfo::act_est_scd_in(route.back().point_id);
    in_final_status=fltInfo.act_out && fltInfo.act_out.get()!=NoExists &&
                    real_in!=NoExists && real_in<NowUTC();
  }
  return true;
}

void TFltParams::incFinalAttempts(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="UPDATE trip_sets SET et_final_attempt=et_final_attempt+1 WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
}

void TFltParams::finishFinalAttempts(int point_id)
{
  setETSExchangeStatus(point_id, ETSExchangeStatus::Finalized);
}

void TFltParams::setETSExchangeStatus(int point_id, ETSExchangeStatus::Enum status)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="UPDATE trip_sets SET pr_etstatus=:pr_etstatus WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("pr_etstatus", otInteger, FNull);
  ETSExchangeStatus::toDB(status, Qry);
  Qry.Execute();
}

bool TFltParams::returnOnlineStatus(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "UPDATE trip_sets SET pr_etstatus=0 WHERE point_id=:point_id AND pr_etstatus<0 ";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  return Qry.RowsProcessed()>0;
}


Ticketing::TicketNum_t checkDocNum(const std::string& doc_no, bool is_et)
{
    if (doc_no.empty())
      throw AstraLocale::UserException(is_et?"MSG.ETICK.NOT_SET_NUMBER":
                                             "MSG.EMD.NOT_SET_NUMBER");
    std::string docNum = doc_no;
    TrimString(docNum);
    for(string::const_iterator c=docNum.begin(); c!=docNum.end(); ++c)
      if (!IsDigitIsLetter(*c))
        throw AstraLocale::UserException(is_et?"MSG.ETICK.TICKET_NO_INVALID_CHARS":
                                               "MSG.EMD.TICKET_NO_INVALID_CHARS");
    try {
        return Ticketing::TicketNum_t(docNum);
    } catch (const TickExceptions::Exception&) {
        throw AstraLocale::UserException(is_et?"MSG.ETICK.INVALID_NUMBER":
                                               "MSG.EMD.INVALID_NUMBER");
    }
}

bool inverseETSEDSSet(const TTripInfo& info,
                      const TTripSetType set_type,
                      bool with_exception)
{
  bool result=true;
  try
  {
    switch(set_type)
    {
      case tsETSNoExchange:
        if (GetTripSets(tsETSNoExchange,info))
          throw AstraLocale::UserException("MSG.ETICK.INTERACTIVE_MODE_NOT_ALLOWED");
        break;
      case tsEDSNoExchange:
        if (GetTripSets(tsEDSNoExchange,info))
          throw AstraLocale::UserException("MSG.EMD.INTERACTIVE_MODE_NOT_ALLOWED");
        break;
      case tsETSControlMethod:
        if (GetTripSets(tsETSControlMethod, info))
          throw AstraLocale::UserException("MSG.ETICK.NOT_SUPPORTED_DUE_TO_CONTROL_METHOD");
        break;
      default:
        throw EXCEPTIONS::Exception("%s: wrong set_type=%d", __FUNCTION__, (int)set_type );
    };
  }
  catch(AstraLocale::UserException &e)
  {
    if (with_exception) throw;
    result=false;
  }

  return result;
}

bool inverseETSEDSSet(int point_id,
                      const TTripSetType set_type,
                      bool with_exception,
                      TTripInfo& info)
{
  info.Clear();
  bool result=true;
  try
  {
    if (!info.getByPointId(point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    result=inverseETSEDSSet(info, set_type, with_exception);
  }
  catch(AstraLocale::UserException &e)
  {
    if (with_exception) throw;
    result=false;
  }

  return result;
}

bool checkETSExchange(const TTripInfo& info,
                      bool with_exception)
{
  return inverseETSEDSSet(info, tsETSNoExchange, with_exception);
}

bool checkETSInteract(const TTripInfo& info,
                      bool with_exception)
{
  return inverseETSEDSSet(info, tsETSControlMethod, with_exception);
}

bool checkEDSExchange(const TTripInfo& info,
                      bool with_exception)
{
  return inverseETSEDSSet(info, tsEDSNoExchange, with_exception);
}

bool checkETSExchange(int point_id,
                      bool with_exception,
                      TTripInfo& info)
{
  return inverseETSEDSSet(point_id, tsETSNoExchange, with_exception, info);
}

bool checkETSInteract(int point_id,
                      bool with_exception,
                      TTripInfo& info)
{
  return inverseETSEDSSet(point_id, tsETSControlMethod, with_exception, info);
}

bool checkEDSExchange(int point_id,
                      bool with_exception,
                      TTripInfo& info)
{
  return inverseETSEDSSet(point_id, tsEDSNoExchange, with_exception, info);
}

std::string getTripAirline(const TTripInfo& ti)
{
    return ti.airline;
}

Ticketing::FlightNum_t getTripFlightNum(const TTripInfo& ti)
{
    if(ti.flt_no < 1 || ti.flt_no == NoExists)
        return Ticketing::FlightNum_t();
    return Ticketing::FlightNum_t(ti.flt_no);
}

bool get_et_addr_set( const string &airline,
                      int flt_no,
                      pair<string,string> &addrs)
{
  int id;
  return get_et_addr_set(airline, flt_no, addrs, id);
}

bool get_et_addr_set(const string &airline,
                     int flt_no,
                     pair<string,string> &addrs,
                     int &id)
{
  addrs.first.clear();
  addrs.second.clear();
  id=ASTRA::NoExists;
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT id, edi_addr, edi_own_addr, "
    "       DECODE(airline,NULL,0,2)+ "
    "       DECODE(flt_no,NULL,0,1) AS priority "
    "FROM et_addr_set "
    "WHERE airline=:airline AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,airline);
  if (flt_no!=0 && flt_no!=ASTRA::NoExists)
    Qry.CreateVariable("flt_no",otInteger,flt_no);
  else
    Qry.CreateVariable("flt_no",otInteger,FNull);
  Qry.Execute();
  if (!Qry.Eof)
  {
    addrs.first=Qry.FieldAsString("edi_addr");
    addrs.second=Qry.FieldAsString("edi_own_addr");
    id=Qry.FieldAsInteger("id");
    return true;
  };
  return false;
}

std::string get_canon_name(const std::string& edi_addr)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT canon_name FROM edi_addrs WHERE addr=:addr";
  Qry.CreateVariable("addr",otString,edi_addr);
  Qry.Execute();
  if (Qry.Eof||Qry.FieldIsNULL("canon_name")) {
    LogTrace(TRACE3) << "get_canon_name by " << edi_addr << " return default canon_name";
    return ETS_CANON_NAME();
  }
  return Qry.FieldAsString("canon_name");
}
void copy_notify_levb(const int src_edi_sess_id,
                      const int dest_edi_sess_id,
                      const bool err_if_not_found)
{
  ProgTrace(TRACE2,"copy_notify_levb: called with src_edi_sess_id=%d dest_edi_sess_id=%d",
                   src_edi_sess_id, dest_edi_sess_id);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO edi_help(address, date1, intmsgid, pult, text, timeout, session_id, instance) "
    "SELECT address, date1, intmsgid, pult, text, timeout, :dest_sess_id, instance "
    "FROM edi_help "
    "WHERE session_id=:src_sess_id";
  Qry.CreateVariable("src_sess_id", otInteger, src_edi_sess_id);
  Qry.CreateVariable("dest_sess_id", otInteger, dest_edi_sess_id);
  Qry.Execute();
  if (Qry.RowsProcessed()==0)
  {
    if (err_if_not_found)
      throw EXCEPTIONS::Exception("copy_notify_levb: nothing in edi_help for session_id=%d", src_edi_sess_id);
  };
}

void confirm_notify_levb(int edi_sess_id, bool err_if_not_found)
{
  ProgTrace(TRACE2,"confirm_notify_levb: called with edi_sess_id=%d",edi_sess_id);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM edi_help WHERE session_id=:id AND rownum<2 "
    "    RETURNING address,text,RAWTOHEX(intmsgid) INTO :address,:text,:intmsgid; "
    "  SELECT COUNT(*) INTO :remained FROM edi_help "
    "    WHERE intmsgid=HEXTORAW(:intmsgid) AND rownum<2; "
    "END;";

  Qry.CreateVariable("id",otString,edi_sess_id);
  Qry.CreateVariable("intmsgid",otString,FNull);
  Qry.CreateVariable("address",otString,FNull);
  Qry.CreateVariable("text",otString,FNull);
  Qry.CreateVariable("remained",otInteger,FNull);
  Qry.Execute();
  if (Qry.VariableIsNULL("intmsgid"))
  {
    if (err_if_not_found)
      throw EXCEPTIONS::Exception("confirm_notify_levb: nothing in edi_help for session_id=%d", edi_sess_id);
    return;
  };
  string hex_msg_id=Qry.GetVariableAsString("intmsgid");
  if (Qry.GetVariableAsInteger("remained")==0)
  {
    string txt=Qry.GetVariableAsString("text");
    string str_msg_id;
    if (!HexToString(hex_msg_id,str_msg_id) || str_msg_id.size()!=sizeof(int)*3)
      throw EXCEPTIONS::Exception("confirm_notify_levb: wrong intmsgid=%s", hex_msg_id.c_str());
    ProgTrace(TRACE2,"confirm_notify_levb: prepare signal %s",txt.c_str());
    std::array<uint32_t,3> a;
    memcpy(&a, str_msg_id.c_str(), 12);
    sethAfter(EdiHelpSignal(ServerFramework::InternalMsgId(a),
                            Qry.GetVariableAsString("address"),
                            txt.c_str()));
#ifdef XP_TESTING
    if(inTestMode()) {
        ServerFramework::setRedisplay(txt);
    }
#endif /* XP_TESTING */
  }
  else
  {
    ProgTrace(TRACE2,"confirm_notify_levb: more records in edi_help for intmsgid=%s", hex_msg_id.c_str());
  };
};

static std::string transformKickIfHttp(const std::string& kickText)
{
  //доклеим HTTP-заголовок, если обработка HTTP-запроса
  ServerFramework::HTTP::request currRequest=ServerFramework::HTTP::get_cur_http_request();
  if (currRequest.headers.empty()) return kickText;
  const auto& contentLength=std::find(currRequest.headers.begin(),
                                      currRequest.headers.end(),
                                      "Content-Length");
  if (contentLength!=currRequest.headers.end())
    contentLength->value = std::to_string(kickText.size());

  return currRequest.to_string() + kickText;
}

string make_xml_kick(const edifact::KickInfo &kickInfo)
{
  if (!kickInfo.jxt)
    throw EXCEPTIONS::Exception("%s: kickInfo.jxt not initialized", __FUNCTION__);

  XMLDoc kickDoc("term");
  if (kickDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("%s: kickDoc.docPtr()==NULL", __FUNCTION__);
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr node=NodeAsNode("/term",kickDoc.docPtr());
  node=NewTextChild(node,"query");
  SetProp(node,"handle",kickInfo.jxt.get().handle);
  SetProp(node,"id",kickInfo.jxt.get().iface);
  SetProp(node,"ver","1");
  SetProp(node,"opr",reqInfo->user.login);
  SetProp(node,"screen",reqInfo->screen.name);
  SetProp(node,"lang",reqInfo->desk.lang);
  if (reqInfo->desk.term_id!=ASTRA::NoExists)
    SetProp(node,"term_id",FloatToString(reqInfo->desk.term_id,0));
  SetProp(NewTextChild(node,"kick"), "req_ctxt_id", kickInfo.reqCtxtId, ASTRA::NoExists);
  std::string redisplay = ConvertCodepage(XMLTreeToText(kickDoc.docPtr()),"CP866","UTF-8");
#ifdef XP_TESTING
  if(inTestMode()) {
      if(kickInfo.jxt.get().iface == "SvcSirena") {
          ServerFramework::setRedisplay(redisplay);
      }
  }
  return redisplay;
#else
  return transformKickIfHttp(redisplay);
#endif//XP_TESTING
};

edifact::KickInfo createKickInfo(const int v_reqCtxtId,
                                 const std::string &v_iface)
{
  return edifact::KickInfo(v_reqCtxtId,
                           v_iface,
                           v_iface.empty()?"":ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString(),
                           v_iface.empty()?"":TReqInfo::Instance()->desk.code);
}

edifact::KickInfo createKickInfo(const int v_reqCtxtId,
                                 const int v_point_id,
                                 const std::string &v_task_name)
{
  return edifact::KickInfo(v_reqCtxtId,
                           v_point_id,
                           v_task_name,
                           "",
                           "");
}

void addToEdiResponseCtxt(int ctxtId,
                          const xmlNodePtr srcNode,
                          const string &destNodeName)
{
  LogTrace(TRACE3) << __FUNCTION__ << " ctxtId=" << ctxtId;
  if (ctxtId==ASTRA::NoExists) return;
  string ctxt;
  AstraContext::GetContext("EDI_RESPONSE",
                           ctxtId,
                           ctxt);
  XMLDoc ediResCtxt;
  if (!ctxt.empty())
  {
    ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");
    ediResCtxt.set(ctxt);
    if (ediResCtxt.docPtr()!=NULL)
      xml_decode_nodelist(ediResCtxt.docPtr()->children);
  }
  else
  {
    ediResCtxt.set("context");
  };
  if (ediResCtxt.docPtr()!=NULL)
  {
    if (srcNode!=NULL)
    {
      xmlNodePtr rootNode=NodeAsNode("/context",ediResCtxt.docPtr());
      if (!destNodeName.empty())
      {
        xmlNodePtr destNode=GetNode(destNodeName.c_str(), rootNode);
        if (destNode==NULL)
          destNode=NewTextChild(rootNode, destNodeName.c_str());

        CopyNodeList(destNode,srcNode->parent);
      }
      else
      {
        CopyNodeList(rootNode,srcNode->parent);
      };
    };
    ctxt=XMLTreeToText(ediResCtxt.docPtr());

    if (!ctxt.empty())
    {
      AstraContext::ClearContext("EDI_RESPONSE",ctxtId);
      AstraContext::SetContext("EDI_RESPONSE",ctxtId,ctxt);
    };
  };
};

static void getCtxt(const std::string& ctxtName,
                    const int ctxtId,
                    const bool clear,
                    const string &where,
                    const bool throwIfEmpty,
                    string &context)
{
  AstraContext::GetContext(ctxtName,
                           ctxtId,
                           context);

  if (clear) AstraContext::ClearContext(ctxtName, ctxtId);

  if (context.empty()) {
    if(throwIfEmpty)
      throw EXCEPTIONS::Exception("%s: context %s empty", where.c_str(), ctxtName.c_str());
  }
}

static void getCtxt(const std::string& ctxtName,
                    const int ctxtId,
                    const bool clear,
                    const std::string& where,
                    const bool throwIfEmpty,
                    XMLDoc &xmlCtxt)
{
  std::string context;
  getCtxt(ctxtName, ctxtId, clear, where, throwIfEmpty, context);

  if (!context.empty())
  {
    xmlCtxt.set(ConvertCodepage(context, "CP866", "UTF-8"));
    if(xmlCtxt.docPtr()==NULL)
      throw EXCEPTIONS::Exception("%s: context %s has wrong XML format", where.c_str(), ctxtName.c_str());

    xml_decode_nodelist(xmlCtxt.docPtr()->children);
  };
}

static void traceCtxt(const std::string& ctxtName,
                      const int ctxtId,
                      const std::string& where)
{
  std::string ctxt;
  getCtxt(ctxtName, ctxtId, false, where, false, ctxt);
  LogTrace(TRACE5) << where << (where.empty()?"":": ") << ctxtName << "(" << ctxtId << ") context: \n" << ctxt;
}

void getEdiResponseCtxt(int ctxtId,
                        bool clear,
                        const string &where,
                        XMLDoc &xmlCtxt,
                        bool throwIfEmpty)
{
  getCtxt("EDI_RESPONSE", ctxtId, clear, where, throwIfEmpty, xmlCtxt);
}

void getTermRequestCtxt(int ctxtId,
                        bool clear,
                        const string &where,
                        XMLDoc &xmlCtxt)
{
  getCtxt("TERM_REQUEST", ctxtId, clear, where, true, xmlCtxt);
}

void getEdiSessionCtxt(int sessIda,
                       bool clear,
                       const std::string& where,
                       XMLDoc &xmlCtxt,
                       bool throwIfEmpty)
{
  getCtxt("EDI_SESSION", sessIda, clear, where, throwIfEmpty, xmlCtxt);
}

void traceEdiSessionCtxt(int sessIda, const std::string &whence)
{
  traceCtxt("EDI_SESSION", sessIda, whence);
}

void traceTermRequestCtxt(int sessIda, const std::string &whence)
{
  XMLDoc ediSessCtxt;
  getCtxt("EDI_SESSION", sessIda, false, whence, false, ediSessCtxt);

  if(ediSessCtxt.docPtr()!=NULL)
  {
    xmlNodePtr rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
    int req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode,ASTRA::NoExists);
    if (req_ctxt_id!=ASTRA::NoExists)
      traceCtxt("TERM_REQUEST", req_ctxt_id, whence);
    else
      LogTrace(TRACE5) << whence << ": req_ctxt_id==ASTRA::NoExists";
  }
  else
  {
    LogTrace(TRACE5) << whence << ": EDI_SESSION has wrong XML format";
    traceCtxt("EDI_SESSION", sessIda, whence);
  };
}

void cleanOldRecords(int min_ago)
{
  if (min_ago<1)
    throw EXCEPTIONS::Exception("%s: wrong min_ago=%d", __FUNCTION__, min_ago);

  TDateTime now=NowUTC();
  TDateTime min_time=now-min_ago/1440.0;

  AstraContext::ClearContext("EDI_SESSION", min_time);
  AstraContext::ClearContext("TERM_REQUEST",min_time);
  AstraContext::ClearContext("EDI_RESPONSE",min_time);
  AstraContext::ClearContext("LCI",min_time);

  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="DELETE FROM tlg_queue WHERE status='SEND' AND time<:time";
  Qry.CreateVariable("time", otDate, now-(min_ago+60)/1440.0);
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText="SELECT ida FROM edisession WHERE sessdatecr<SYSDATE-:min_ago/1440 FOR UPDATE";
  Qry.CreateVariable("min_ago", otInteger, min_ago);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    using namespace edilib;
    EdiSessionId_t edisess(Qry.FieldAsInteger("ida"));
    EdilibDbCallbacks::instance()->ediSessionDeleteDb(edisess);
    EdilibDbCallbacks::instance()->ediSessionToDeleteDb(edisess);
  };

  edifact::RemoteResults::cleanOldRecords(min_ago);
}

void HandleNotSuccessEtsResult(const edifact::RemoteResults& res, xmlNodePtr resNode)
{
  if (res.status() == edifact::RemoteStatus::Success) return;
  if (res.status() == edifact::RemoteStatus::Timeout)
  {
    xmlNodePtr errNode=NewTextChild(resNode,"ets_connect_error");
    SetProp(errNode,"internal_msgid",get_internal_msgid_hex());
    NewTextChild(errNode,"message",getLocaleText("MSG.ETS_CONNECT_ERROR"));
    return;
  }

  if (res.status() == edifact::RemoteStatus::CommonError)
  {
    ProgTrace(TRACE1, "ETS: ERROR %s", res.ediErrCode().c_str());
    if (res.remark().empty())
    {
      throw UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", res.ediErrCode()));
    }
    else
    {
      ProgTrace(TRACE1, "ETS: %s", res.remark().c_str());
      throw UserException("MSG.ETICK.ETS_ERROR", LParams() << LParam("msg", res.remark()));
    }
  }
  else
  {
    throw UserException("Error from remote ETS");
  }
}

void ProcEdiError(const AstraLocale::LexemaData &error,
                  const xmlNodePtr errorCtxtNode,
                  const bool isGlobal)
{
  if (errorCtxtNode==NULL) return;
  xmlNodePtr errorNode=NewTextChild(errorCtxtNode, "error");
  LexemeDataToXML(error, errorNode);
  SetProp(errorNode, "global", (int)isGlobal);
}

void GetEdiError(const xmlNodePtr errorCtxtNode,
                 EdiErrorList &errors)
{
  errors.clear();
  for(xmlNodePtr node=errorCtxtNode->children; node!=NULL; node=node->next)
  {
    if (strcmp((const char*)node->name,"error")!=0) continue;
    EdiErrorList::iterator i=errors.insert(errors.end(), make_pair(AstraLocale::LexemaData(), false));
    LexemeDataFromXML(node, i->first);
    i->second=NodeAsInteger("@global", node)!=0;
  }
}

void WritePostponedContext(tlgnum_t tnum, int reqCtxtId)
{
    ASSERT(reqCtxtId != ASTRA::NoExists);
    LogTrace(TRACE1) << __FUNCTION__ << " ctxt_id=" << reqCtxtId << "; msg_id=" << tnum;
    OciCpp::CursCtl cur = make_curs(
"insert into POSTPONED_TLG_CONTEXT (MSG_ID, REQ_CTXT_ID) "
"values (:msg_id, :req_ctxt_id)");

    cur.bind(":msg_id",      tnum.num)
       .bind(":req_ctxt_id", reqCtxtId)
       .exec();
}

int ReadPostponedContext(tlgnum_t tnum, bool clear)
{
    OciCpp::CursCtl cur = make_curs(
"select REQ_CTXT_ID from POSTPONED_TLG_CONTEXT "
"where MSG_ID=:msg_id");

    int reqCtxtId = 0;
    cur.bind(":msg_id", tnum.num)
       .def(reqCtxtId)
       .exfet();

    if(clear) {
        ClearPostponedContext(tnum);
    }

    return reqCtxtId;
}

void ClearPostponedContext(tlgnum_t tnum)
{
    LogTrace(TRACE1) << __FUNCTION__ << ". msg_id=" << tnum;
    make_curs(
"delete from POSTPONED_TLG_CONTEXT "
"where MSG_ID=:msg_id")
       .bind(":msg_id", tnum.num)
       .exec();
}

void TOriginCtxt::toXML(xmlNodePtr node)
{
  if (node==NULL) return;
  TReqInfo *reqInfo = TReqInfo::Instance();
  SetProp(node,"desk",reqInfo->desk.code);
  SetProp(node,"user",reqInfo->user.descr);
  SetProp(node,"screen",reqInfo->screen.name);
}

TOriginCtxt& TOriginCtxt::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  screen=NodeAsString("@screen", node);
  user_descr=NodeAsString("@user", node);
  desk_code=NodeAsString("@desk", node);
  return *this;
}

const TPaxCtxt& TPaxCtxt::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "flight", flight);
  point_id       !=NoExists?NewTextChild(node, "point_id", point_id):
                            NewTextChild(node, "point_id");
  pax.grp_id     !=NoExists?NewTextChild(node, "grp_id", pax.grp_id):
                            NewTextChild(node, "grp_id");
  pax.id         !=NoExists?NewTextChild(node, "pax_id", pax.id):
                            NewTextChild(node, "pax_id");
  pax.reg_no     !=NoExists?NewTextChild(node, "reg_no", pax.reg_no):
                            NewTextChild(node, "reg_no");
  NewTextChild(node, "surname", pax.surname);
  NewTextChild(node, "name", pax.name);
  NewTextChild(node, "pers_type", EncodePerson(pax.pers_type));
  return *this;
}

TPaxCtxt& TPaxCtxt::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  flight=NodeAsStringFast("flight", node2);
  point_id=NodeAsIntegerFast("point_id", node2, NoExists);
  pax.grp_id=NodeAsIntegerFast("grp_id", node2, NoExists);
  pax.id=NodeAsIntegerFast("pax_id", node2, NoExists);
  pax.reg_no=NodeAsIntegerFast("reg_no", node2, NoExists);
  pax.surname=NodeAsStringFast("surname", node2);
  pax.name=NodeAsStringFast("name", node2);
  pax.pers_type=DecodePerson(NodeAsStringFast("pers_type", node2));
  return *this;
}

TPaxCtxt& TPaxCtxt::paxFromDB(TQuery &Qry, bool from_crs)
{
  pax.clear();
  pax.id=Qry.FieldAsInteger("pax_id");
  pax.surname=Qry.FieldAsString("surname");
  pax.name=Qry.FieldAsString("name");
  pax.pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
  if (!from_crs)
  {
    pax.grp_id=Qry.FieldAsInteger("grp_id");
    pax.refuse=Qry.FieldAsString("refuse");
    pax.reg_no=Qry.FieldAsInteger("reg_no");
    pax.pr_brd=!Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
    pax.tkn.fromDB(Qry);
  }
  return *this;
};

void ProcEvent(const TLogLocale &event,
               const TCtxtItem &ctxt,
               const xmlNodePtr eventCtxtNode,
               const bool repeated)
{
  TLogLocale eventWithPax;
  eventWithPax.ev_type=event.ev_type;
  eventWithPax.id1=ctxt.point_id;
  eventWithPax.id3=ctxt.pax.grp_id;
  if (!ctxt.paxUnknown())
  {
    eventWithPax.id2=ctxt.pax.reg_no;

    eventWithPax.lexema_id = "EVT.PASSENGER_DATA";
    eventWithPax.prms << PrmSmpl<string>("pax_name", ctxt.pax.full_name())
                      << PrmElem<string>("pers_type", etPersType, EncodePerson(ctxt.pax.pers_type))
                      << PrmLexema("param", event.lexema_id, event.prms);
  }
  else
  {
    eventWithPax.lexema_id=event.lexema_id;
    eventWithPax.prms=event.prms;
  };

  if (!repeated) eventWithPax.toDB(ctxt.screen, ctxt.user_descr, ctxt.desk_code);

  eventWithPax.toXML(eventCtxtNode); //важно, что после toDB, потому что инициализируются ev_time и ev_order
};

} //namespace AstraEdifact

bool isTermCheckinRequest(xmlNodePtr reqNode)
{
  return reqNode!=nullptr &&
         TReqInfo::Instance()->client_type==ctTerm &&
         (strcmp((const char*)reqNode->name, "TCkinSavePax") == 0 ||
          strcmp((const char*)reqNode->name, "TCkinSaveUnaccompBag") == 0);
}

bool isWebCheckinRequest(xmlNodePtr reqNode)
{
  return reqNode!=nullptr &&
         (TReqInfo::Instance()->client_type==ctWeb ||
          TReqInfo::Instance()->client_type==ctMobile ||
          TReqInfo::Instance()->client_type==ctKiosk) &&
         strcmp((const char*)reqNode->name, "SavePax") == 0;
}

bool isTagAddRequestSBDO(xmlNodePtr reqNode)
{
  return reqNode!=nullptr &&
         TReqInfo::Instance()->client_type==ctHTTP &&
         GetNode("/term/query/PassengerBaggageTagAdd", reqNode->doc)!=nullptr;
}

bool isTagConfirmRequestSBDO(xmlNodePtr reqNode)
{
  return reqNode!=nullptr &&
         TReqInfo::Instance()->client_type==ctHTTP &&
         GetNode("/term/query/PassengerBaggageTagConfirm", reqNode->doc)!=nullptr;
}

bool isTagRevokeRequestSBDO(xmlNodePtr reqNode)
{
  return reqNode!=nullptr &&
         TReqInfo::Instance()->client_type==ctHTTP &&
         GetNode("/term/query/PassengerBaggageTagRevoke", reqNode->doc)!=nullptr;
}
