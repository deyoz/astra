#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "edi_utils.h"
#include "misc.h"
#include "tlg/tlg.h"
#include "astra_context.h"
#include "tlg/remote_results.h"
#include <serverlib/internal_msgid.h>
#include <serverlib/ehelpsig.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/testmode.h>
#include <edilib/EdiSessionTimeOut.h>
#include <edilib/edi_session.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace Ticketing;
using namespace BASIC;
using namespace ASTRA;

namespace AstraEdifact
{

CouponStatus calcPaxCouponStatus(const string& refuse,
                                 const bool pr_brd,
                                 const bool in_final_status)
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
  ets_no_interact=GetTripSets(tsETSNoInteract,fltInfo);
  eds_no_interact=GetTripSets(tsEDSNoInteract,fltInfo);
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
}

void checkDocNum(const std::string& doc_no)
{
    std::string docNum = doc_no;
    TrimString(docNum);
    for(string::const_iterator c=docNum.begin(); c!=docNum.end(); ++c)
      if (!IsDigitIsLetter(*c))
        throw AstraLocale::UserException("MSG.ETICK.TICKET_NO_INVALID_CHARS");
}

bool checkInteract(const int point_id,
                   const TTripSetType set_type,
                   const bool with_exception,
                   TTripInfo& info)
{
  info.Clear();
  bool result=true;
  try
  {
    TQuery Qry(&OraSession);
    Qry.SQLText=
        "SELECT airline,flt_no,airp FROM points "
        "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    info.airline=Qry.FieldAsString("airline");
    info.flt_no=Qry.FieldAsInteger("flt_no");
    info.airp=Qry.FieldAsString("airp");
    switch(set_type)
    {
      case tsETSNoInteract:
        if (GetTripSets(tsETSNoInteract,info))
          throw AstraLocale::UserException("MSG.ETICK.INTERACTIVE_MODE_NOT_ALLOWED");
        break;
      case tsEDSNoInteract:
        if (GetTripSets(tsEDSNoInteract,info))
          throw AstraLocale::UserException("MSG.EMD.INTERACTIVE_MODE_NOT_ALLOWED");
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

bool checkETSInteract(const int point_id,
                      const bool with_exception,
                      TTripInfo& info)
{
  return checkInteract(point_id, tsETSNoInteract, with_exception, info);
}

bool checkEDSInteract(const int point_id,
                      const bool with_exception,
                      TTripInfo& info)
{
  return checkInteract(point_id, tsEDSNoInteract, with_exception, info);
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
                      const int flt_no,
                      pair<string,string> &addrs)
{
  int id;
  return get_et_addr_set(airline, flt_no, addrs, id);
};

bool get_et_addr_set( const string &airline,
                      const int flt_no,
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
  if (Qry.Eof||Qry.FieldIsNULL("canon_name"))
    return ETS_CANON_NAME();
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

void confirm_notify_levb(const int edi_sess_id, const bool err_if_not_found)
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
    sethAfter(EdiHelpSignal((const int*)str_msg_id.c_str(),
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

string make_xml_kick(const edifact::KickInfo &kickInfo)
{
  XMLDoc kickDoc("term");
  if (kickDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("%s: kickDoc.docPtr()==NULL", __FUNCTION__);
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr node=NodeAsNode("/term",kickDoc.docPtr());
  node=NewTextChild(node,"query");
  SetProp(node,"handle","0");
  SetProp(node,"id",kickInfo.iface);
  SetProp(node,"ver","1");
  SetProp(node,"opr",reqInfo->user.login);
  SetProp(node,"screen",reqInfo->screen.name);
  SetProp(node,"lang",reqInfo->desk.lang);
  if (reqInfo->desk.term_id!=ASTRA::NoExists)
    SetProp(node,"term_id",FloatToString(reqInfo->desk.term_id,0));
  if (kickInfo.reqCtxtId!=ASTRA::NoExists)
    SetProp(NewTextChild(node,"kick"),"req_ctxt_id",kickInfo.reqCtxtId);
  else
    NewTextChild(node,"kick");
  return ConvertCodepage(XMLTreeToText(kickDoc.docPtr()),"CP866","UTF-8");
};

edifact::KickInfo createKickInfo(const int v_reqCtxtId,
                                 const std::string &v_iface)
{
  return edifact::KickInfo(v_reqCtxtId,
                           v_iface,
                           v_iface.empty()?"":ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString(),
                           v_iface.empty()?"":TReqInfo::Instance()->desk.code);
}

void addToEdiResponseCtxt(const int ctxtId,
                          const xmlNodePtr srcNode,
                          const string &destNodeName)
{
  if (ctxtId==ASTRA::NoExists || srcNode==NULL || destNodeName.empty()) return;
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
    xmlNodePtr rootNode=NodeAsNode("/context",ediResCtxt.docPtr());
    xmlNodePtr destNode=GetNode(destNodeName.c_str(), rootNode);
    if (destNode==NULL)
      destNode=NewTextChild(rootNode, destNodeName.c_str());

    CopyNodeList(destNode,srcNode->parent);
    ctxt=XMLTreeToText(ediResCtxt.docPtr());

    if (!ctxt.empty())
    {
      AstraContext::ClearContext("EDI_RESPONSE",ctxtId);
      AstraContext::SetContext("EDI_RESPONSE",ctxtId,ctxt);
    };
  };
};

void getEdiResponseCtxt(const int ctxtId,
                        const bool clear,
                        const string &where,
                        string &context)
{
  AstraContext::GetContext("EDI_RESPONSE",
                           ctxtId,
                           context);

  if (clear) AstraContext::ClearContext("EDI_RESPONSE", ctxtId);

  if (context.empty())
    throw EXCEPTIONS::Exception("%s: context EDI_RESPONSE empty", where.c_str());
};

void getEdiResponseCtxt(const int ctxtId,
                        const bool clear,
                        const string &where,
                        XMLDoc &xmlCtxt)
{
  string context;
  getEdiResponseCtxt(ctxtId, clear, where, context);
  xmlCtxt.set(ConvertCodepage(context,"CP866","UTF-8"));
  if (xmlCtxt.docPtr()==NULL)
    throw EXCEPTIONS::Exception("%s: context EDI_RESPONSE wrong XML format", where.c_str());

  xml_decode_nodelist(xmlCtxt.docPtr()->children);
};

void getTermRequestCtxt(const int ctxtId,
                        const bool clear,
                        const string &where,
                        XMLDoc &xmlCtxt)
{
  string context;
  AstraContext::GetContext("TERM_REQUEST",
                           ctxtId,
                           context);

  if (clear) AstraContext::ClearContext("TERM_REQUEST", ctxtId);

  if (context.empty())
    throw EXCEPTIONS::Exception("%s: context TERM_REQUEST empty", where.c_str());

  xmlCtxt.set(ConvertCodepage(context,"CP866","UTF-8"));
  if (xmlCtxt.docPtr()==NULL)
    throw EXCEPTIONS::Exception("%s: context TERM_REQUEST wrong XML format", where.c_str());

  xml_decode_nodelist(xmlCtxt.docPtr()->children);
};

void cleanOldRecords(const int min_ago)
{
  if (min_ago<1)
    throw EXCEPTIONS::Exception("%s: wrong min_ago=%d", __FUNCTION__, min_ago);

  TDateTime now=NowUTC();

  AstraContext::ClearContext("EDI_SESSION",now-min_ago/1440.0);
  AstraContext::ClearContext("TERM_REQUEST",now-min_ago/1440.0);
  AstraContext::ClearContext("EDI_RESPONSE",now-min_ago/1440.0);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT ida FROM edisession WHERE sessdatecr<SYSDATE-:min_ago/1440 FOR UPDATE";
  Qry.CreateVariable("min_ago", otInteger, min_ago);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    using namespace edilib;
    EdiSessionId_t edisess(Qry.FieldAsInteger("ida"));
    EdiSession::deleteDb(edisess);
    EdiSessionTimeOut::deleteDb(edisess);
  };

  edifact::RemoteResults::cleanOldRecords(min_ago);
};

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
};

} //namespace AstraEdifact
