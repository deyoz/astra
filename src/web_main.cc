#include <arpa/inet.h>
#include <memory.h>
#include <string>

#include "oralib.h"
#include "exceptions.h"
#include "stages.h"
#include "salons.h"
#include "salonform.h"
#include "images.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_context.h"
#include "convert.h"
#include "date_time.h"
#include "misc.h"
#include "astra_misc.h"
#include "print.h"
#include "web_main.h"
#include "web_search.h"
#include "checkin.h"
#include "astra_locale.h"
#include "comp_layers.h"
#include "passenger.h"
#include "remarks.h"
#include "sopp.h"
#include "points.h"
#include "stages.h"
#include "astra_service.h"
#include "tlg/tlg.h"
#include "qrys.h"
#include "request_dup.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"
#include "checkin_utils.h"
#include "apis_utils.h"
#include "stl_utils.h"
#include "astra_callbacks.h"
#include "baggage_pc.h"
#include "meridian.h"



#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC::date_time;
using namespace AstraLocale;

InetClient getInetClient(string client_id)
{
  InetClient client;
  client.client_id = client_id;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT client_type,web_clients.desk,login "
    "FROM web_clients,users2 "
    "WHERE web_clients.client_id=:client_id AND "
    "      web_clients.user_id=users2.user_id";
  Qry.CreateVariable( "client_id", otString, client_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    client.pult = Qry.FieldAsString( "desk" );
    client.opr = Qry.FieldAsString( "login" );
    client.client_type = Qry.FieldAsString( "client_type" );
  }
  else ProgError(STDLOG, "%s: client_id=%s not found", __FUNCTION__, client_id.c_str());
  return client;
}

namespace AstraWeb
{

const string PARTITION_ELEM_TYPE = "П";
const string ARMCHAIR_ELEM_TYPE = "К";

int readInetClientId(const char *head)
{
  short grp;
  memcpy(&grp,head+45,2);
  return ntohs(grp);
}

void RevertWebResDoc();

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> internet_main(const std::vector<uint8_t>& body, const char *head, size_t hlen)
{
  InitLogTime(NULL);
  PerfomInit();
  int client_id=readInetClientId(head);
  ProgTrace(TRACE1,"new web request received from client %i",client_id);

  try
  {
    if (SEND_REQUEST_DUP() &&
        hlen>0 && *head==char(2))
    {
      const std::string b(body.begin(),body.end());
      if ( b.find("<kick") == string::npos )
      {
        std::string msg;
        if (BuildMsgForWebRequestDup(client_id, b, msg))
        {
          /*std::string msg_hex;
          StringToHex(msg, msg_hex);
          ProgTrace(TRACE5, "internet_main: msg_hex=%s", msg_hex.c_str());*/
          sendCmd("REQUEST_DUP", msg.c_str(), msg.size());
        };
      };
    };
  }
  catch(...) {};

  try
  {
    InetClient client=getInetClient(IntToString(client_id));
    string new_header=(string(head,45)+client.pult+"  "+client.opr+string(100,0)).substr(0,100)+string(head+100,hlen-100);

    string new_body(body.begin(),body.end());
    string sss("<query");
    string::size_type pos=new_body.find(sss);
    if(pos!=string::npos)
    {
        if ( new_body.find("<kick") == string::npos )
        new_body=new_body.substr(0,pos+sss.size())+" id='"+WEB_JXT_IFACE_ID/*client.client_type*/+"' screen='AIR.EXE' opr='"+CP866toUTF8(client.opr)+"'"+new_body.substr(pos+sss.size());
    }
    else
      ProgTrace(TRACE1,"Unable to find <query> tag!");

    InitLogTime(client.pult.c_str());

    static ServerFramework::ApplicationCallbacks *ac=
             ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();

    AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
    astra_cb_ptr->SetPostProcessXMLAnswerCallback(RevertWebResDoc);

    char* res = nullptr;
    auto newlen=ac->jxt_proc(new_body.data(),new_body.size(),new_header.data(),new_header.size(),&res,0);
    std::unique_ptr<char, void (*)(void*)> res_holder(res,free);
    ProgTrace(TRACE1,"newlen=%i",newlen);
//    if(newlen < hlen)
//        throw
    return std::make_tuple(std::vector<uint8_t>(head,head+hlen), std::vector<uint8_t>(res+hlen,res+newlen));
  }
  catch(...)
  {
      constexpr unsigned char err_xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<error/>";
      return std::make_tuple(std::vector<uint8_t>(head,head+hlen), std::vector<uint8_t>(err_xml, err_xml -1 + sizeof err_xml));
  }
  //InitLogTime(NULL); -- why??
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TIdsPnrData {
  int point_id;
  string airline;
  int flt_no;
  string suffix;
  int pnr_id;
  bool pr_paid_ckin;
  TIdsPnrData()
  {
    point_id=NoExists;
    flt_no=NoExists;
    pnr_id=NoExists;
    pr_paid_ckin=false;
  };
};

int VerifyPNR( int point_id, int id, bool is_pnr_id )
{
    TQuery Qry(&OraSession);
  if (!isTestPaxId(id))
  {
    ostringstream sql;
    sql << "SELECT point_id_spp, crs_pnr.pnr_id "
           "FROM crs_pnr,crs_pax,tlg_binding "
           "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
           "      crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND ";
    if (is_pnr_id)
      sql << "      crs_pnr.pnr_id=:id AND ";
    else
      sql << "      crs_pax.pax_id=:id AND ";
    sql << "      crs_pax.pr_del=0 AND "
           "      tlg_binding.point_id_spp(+)=:point_id AND rownum<2";

    Qry.SQLText = sql.str().c_str();
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    if ( Qry.Eof )
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
    if ( Qry.FieldIsNULL( "point_id_spp" ) )
        throw UserException( "MSG.PASSENGERS.OTHER_FLIGHT" );
    return Qry.FieldAsInteger("pnr_id");
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:id";
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    if ( Qry.Eof )
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
    return id;
  };
}

int VerifyPNRByPaxId( int point_id, int pax_id ) //возвращает pnr_id
{
  return VerifyPNR(point_id, pax_id, false);
}

void VerifyPNRByPnrId( int point_id, int pnr_id )
{
  VerifyPNR(point_id, pnr_id, true);
}

int VerifyPNRById( int point_id, xmlNodePtr node ) //возвращает pnr_id
{
  int pnr_id=NoExists;
  if (GetNode( "pnr_id", node )!=NULL)
  {
    pnr_id = NodeAsInteger( "pnr_id", node );
    VerifyPNRByPnrId( point_id, pnr_id );
  }
  else
  {
    pnr_id = VerifyPNRByPaxId( point_id, NodeAsInteger( "crs_pax_id", node ) );
  };
  return pnr_id;
}

void WebRequestsIface::SearchPNRs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  resNode=NewTextChild(resNode,"SearchPNRs");

  if (reqInfo->user.access.airlines().totally_not_permitted() ||
      reqInfo->user.access.airps().totally_not_permitted() )
  {
    ProgError(STDLOG, "WebRequestsIface::SearchPNRs: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    return;
  };

  WebSearch::TPNRFilter filter;
  filter.fromXML(reqNode);
  filter.testPaxFromDB();
  filter.trace(TRACE5);

  WebSearch::TPNRs PNRs;
  WebSearch::findPNRs(filter, PNRs, 1);
  WebSearch::findPNRs(filter, PNRs, 2);
  WebSearch::findPNRs(filter, PNRs, 3);
  PNRs.toXML(resNode);
};

void GetPNRsList(WebSearch::TPNRFilters &filters,
                 list<WebSearch::TPNRs> &PNRsList,
                 list<AstraLocale::LexemaData> &errors)
{
  for(list<WebSearch::TPNRFilter>::iterator f=filters.segs.begin(); f!=filters.segs.end(); ++f)
  {
    WebSearch::TPNRFilter &filter=*f;

    if (!filter.from_scan_code)
    {
      if (filter.document.empty() &&
          filter.ticket_no.empty() &&
          filter.pnr_addr_normal.empty())
      {
        TReqInfo::Instance()->traceToMonitor(TRACE5, "WebRequestsIface::SearchFlt: <pnr_addr>, <ticket_no>, <document> not defined");
        throw UserException("MSG.NOTSET.SEARCH_PARAMS");
      };
    };

    filter.testPaxFromDB();
    filter.trace(TRACE5);

    PNRsList.push_back(WebSearch::TPNRs());
    try
    {
      WebSearch::TPNRs &PNRs=PNRsList.back();
      if (!filter.from_scan_code)
      {
        //это не сканирование штрих-кода
        WebSearch::findPNRs(filter, PNRs, 1);
        if (PNRs.pnrs.empty())
        {
          WebSearch::findPNRs(filter, PNRs, 2);
          WebSearch::findPNRs(filter, PNRs, 3);
        };
      }
      else
      {
        //если сканирование штрих-кода, тогда только поиск по оперирующему перевозчику
        WebSearch::findPNRs(filter, PNRs, 1, false);
        if (filter.test_paxs.empty() && PNRs.pnrs.empty())
        {
          WebSearch::findPNRs(filter, PNRs, 1, true);
          if (!PNRs.pnrs.empty())
            throw UserException( "MSG.PASSENGER.CONTACT_CHECKIN_AGENT" );
        };
      };

      if (PNRs.pnrs.empty())
        throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
      if (filter.test_paxs.empty() && PNRs.pnrs.size()>1)
        filter.from_scan_code?
          throw UserException( "MSG.PASSENGERS.FOUND_MORE" ):
          throw UserException( "MSG.PASSENGERS.FOUND_MORE.ADJUST_SEARCH_PARAMS" );
    }
    catch(UserException &e)
    {
      PNRsList.pop_back();
      errors.push_back(e.getLexemaData());
      ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
    };
  };
};

void WebRequestsIface::SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  resNode=NewTextChild(resNode,"SearchFlt");

  if (reqInfo->user.access.airlines().totally_not_permitted() ||
      reqInfo->user.access.airps().totally_not_permitted())
  {
    ProgError(STDLOG, "WebRequestsIface::SearchFlt: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    return;
  };

  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);
  WebSearch::TPNRFilters filters;
  if (scanCodeNode!=NULL)
    filters.fromBCBP_M(NodeAsString(scanCodeNode));
  else
    filters.fromXML(reqNode);

  list<AstraLocale::LexemaData> errors;
  list<WebSearch::TPNRs> PNRsList;

  GetPNRsList(filters, PNRsList, errors);

  xmlNodePtr segsNode=NewTextChild(resNode, "segments");

  class nextPNR{};

  for(int pass=1; pass<=5; pass++)
  {
    for(list<WebSearch::TPNRs>::const_iterator iPNRs=PNRsList.begin(); iPNRs!=PNRsList.end(); ++iPNRs)
    try
    {
      const WebSearch::TPNRs &PNRs=*iPNRs;
      if (PNRs.pnrs.empty()) continue; //на всякий случай

      const map< int/*num*/, WebSearch::TPNRSegInfo > &segs=PNRs.pnrs.begin()->second.segs;
      for(map< int/*num*/, WebSearch::TPNRSegInfo >::const_iterator iSeg=segs.begin(); iSeg!=segs.end(); ++iSeg)
      {
        const set<WebSearch::TFlightInfo>::const_iterator &iFlt=PNRs.flights.find(WebSearch::TFlightInfo(iSeg->second.point_dep));
        if (iFlt==PNRs.flights.end())
          throw EXCEPTIONS::Exception("WebRequestsIface::SearchFlt: flight not found in PNRs (point_dep=%d)", iSeg->second.point_dep);

        if (PNRsList.size()>1 && iSeg==segs.begin())
        {
          int priority=5;
          if (boost::optional<TStage> opt_stage=iFlt->stage())
          {
            switch ( opt_stage.get() ) {
              case sNoActive:
                priority=2;
                break;
              case sOpenWEBCheckIn:
              case sOpenKIOSKCheckIn:
                priority=1;
                break;
              case sCloseWEBCheckIn:
              case sCloseKIOSKCheckIn:
                priority=3;
                break;
              case sTakeoff:
                priority=4;
                break;
              default:
                break;
            };
            ProgTrace(TRACE5, "%s: pass=%d priority=%d stage=%d ",
                              __FUNCTION__, pass, priority, opt_stage.get());
          }
          else ProgTrace(TRACE5, "%s: pass=%d priority=%d stage=unknown ",
                                 __FUNCTION__, pass, priority);

          if (pass!=priority) throw nextPNR();
        };

        const set<WebSearch::TDestInfo>::const_iterator &iDest=iFlt->dests.find(WebSearch::TDestInfo(iSeg->second.point_arv));
        if (iDest==iFlt->dests.end())
          throw EXCEPTIONS::Exception("WebRequestsIface::SearchFlt: dest not found in PNRs (point_arv=%d)", iSeg->second.point_arv);

        xmlNodePtr segNode=NewTextChild(segsNode, "segment");
        iFlt->toXML(segNode, true);
        iDest->toXML(segNode, true);
        iSeg->second.toXML(segNode, true);
        PNRs.pnrs.begin()->second.toXML(segNode, true);  //пишет только <bag_norm>
      };
      return; //выходим из поиска, так как записали в XML сквозные сегменты с самым подходящим 1-м сегментом
    }
    catch(nextPNR) {};
  };

  //если сюда дошли, то выводим первый UserException
  if (errors.empty()) throw EXCEPTIONS::Exception("%s: errors.empty()", __FUNCTION__);
  throw UserException(errors.begin()->lexema_id, errors.begin()->lparams);

};
/*
1. Если кто-то уже начал работать с pnr (агент,разборщик PNL)
2. Если пассажир зарегистрировался, а разборщик PNL ставит признак удаления
*/

struct TWebPnr {
  TCompleteAPICheckInfo checkInfo;
  vector<TWebPax> paxs;

  void clear()
  {
    checkInfo.clear();
    paxs.clear();
  };
};

void verifyPaxTids( int pax_id, int crs_pnr_tid, int crs_pax_tid, int pax_grp_tid, int pax_tid )
{
  TQuery Qry(&OraSession);
  if (!isTestPaxId(pax_id))
  {
    if (pax_grp_tid==NoExists || pax_tid==NoExists)
      throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" ); //это бывает когда перед печатью произошла разрегистрация
    Qry.SQLText =
      "SELECT crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       pax_grp.tid AS pax_grp_tid, "
      "       pax.tid AS pax_tid, "
      "       pax.pax_id "
      " FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id AND "
      "      crs_pax.pr_del=0";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( Qry.Eof ||
           crs_pnr_tid != Qry.FieldAsInteger( "crs_pnr_tid" ) ||
           crs_pax_tid != Qry.FieldAsInteger( "crs_pax_tid" ) ||
           pax_grp_tid != Qry.FieldAsInteger( "pax_grp_tid" ) ||
           pax_tid != Qry.FieldAsInteger( "pax_tid" ) )
        throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" );
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( Qry.Eof ||
         crs_pnr_tid != Qry.FieldAsInteger( "id" ) ||
           crs_pax_tid != Qry.FieldAsInteger( "id" ) ||
         pax_grp_tid != pax_tid)
      throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" );
  };
}

bool is_valid_pnr_status(const string &pnr_status)
{
  return !(//pax.name=="CBBG" ||  надо спросить у Сергиенко
               pnr_status=="DG2" ||
               pnr_status=="RG2" ||
               pnr_status=="ID2" ||
               pnr_status=="WL");
};

bool is_web_cancel(int pax_id)
{
  TCachedQuery Qry("SELECT time FROM crs_pax_refuse "
                   "WHERE pax_id=:pax_id AND client_type=:client_type AND rownum<2",
                   QParams() << QParam("pax_id", otInteger, pax_id)
                             << QParam("client_type", otString, EncodeClientType(TReqInfo::Instance()->client_type)));
  Qry.get().Execute();
  if (!Qry.get().Eof) return true;
  return false;
};

bool is_valid_pax_nationality(const TTripInfo &flt,
                              int pax_id)
{
  if (!GetSelfCkinSets(tsRegRUSNationOnly, flt, TReqInfo::Instance()->client_type)) return true;
  CheckIn::TPaxDocItem doc;
  CheckIn::LoadCrsPaxDoc(pax_id, doc);
  if (doc.nationality=="RUS") return true;
  return false;
};

bool is_valid_pax_status(int point_id, int pax_id)
{
  TTripInfo flt;
  flt.getByPointId(point_id);
  return !is_web_cancel(pax_id) &&
         is_valid_pax_nationality(flt, pax_id);
};

bool is_valid_doc_info(const TCompleteAPICheckInfo &checkInfo,
                       const CheckIn::TPaxDocItem &doc)
{
  if (checkInfo.incomplete(doc)) return false;
  return true;
};

bool is_valid_doco_info(const TCompleteAPICheckInfo &checkInfo,
                        const CheckIn::TPaxDocoItem &doco)
{
  if (checkInfo.incomplete(doco)) return false;
  return true;
};

bool is_valid_doca_info(const TCompleteAPICheckInfo &checkInfo,
                        const list<CheckIn::TPaxDocaItem> &doca)
{
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
    if (checkInfo.incomplete(*d)) return false;
  return true;
};

bool is_valid_tkn_info(const TCompleteAPICheckInfo &checkInfo,
                       const CheckIn::TPaxTknItem &tkn)
{
  if (checkInfo.incomplete(tkn)) return false;
  return true;
};

bool is_valid_rem_codes(const TTripInfo &flt,
                        const std::multiset<CheckIn::TPaxRemItem> &rems)
{
  TRemGrp rem_grp;
  switch (TReqInfo::Instance()->client_type)
  {
    case ctWeb:
      rem_grp.Load(retWEB, flt.airline);
      break;
    case ctKiosk:
      rem_grp.Load(retKIOSK, flt.airline);
      break;
    case ctMobile:
      rem_grp.Load(retMOB, flt.airline);
      break;
    default:
      return true;
  }
  bool result=true;
  for(multiset<CheckIn::TPaxRemItem>::const_iterator i=rems.begin(); i!=rems.end(); ++i)
    if (rem_grp.exists(i->code))
    {
      ProgTrace(TRACE5, "%s: airline=%s forbidden rem code %s", __FUNCTION__, flt.airline.c_str(), i->code.c_str());
      result=false;
    }
  return result;
}

void checkDocInfoToXML(const TCompleteAPICheckInfo &checkInfo,
                       const xmlNodePtr node)
{
  if (node==NULL) return;
  xmlNodePtr fieldsNode=NewTextChild(node, "doc_required_fields");
  SetProp(fieldsNode, "is_inter", checkInfo.pass().get(apiDoc).is_inter);
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_ISSUE_COUNTRY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_country");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_NATIONALITY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "nationality");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_BIRTH_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_date");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_GENDER_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "gender");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_SURNAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "surname");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_FIRST_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "first_name");
  if ((checkInfo.pass().get(apiDoc).required_fields&DOC_SECOND_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "second_name");

  fieldsNode=NewTextChild(node, "doco_required_fields");
  SetProp(fieldsNode, "is_inter", checkInfo.pass().get(apiDoco).is_inter);
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_BIRTH_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_place");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_ISSUE_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_place");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_ISSUE_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_date");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkInfo.pass().get(apiDoco).required_fields&DOCO_APPLIC_COUNTRY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "applic_country");
};

string PaxDocCountryToXML(const string &pax_doc_country, const TClientType client_type)
{
  string result;
  if (!pax_doc_country.empty())
  {
    try
    {
      if (client_type != ctKiosk)
        result=getBaseTable(etPaxDocCountry).get_row("code",pax_doc_country).AsString("country");
    }
    catch (EBaseTableError) {};
    if (result.empty()) result=pax_doc_country;
  };
  return result;
};

void PaxDocToXML(const CheckIn::TPaxDocItem &doc,
                 const xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr docNode=NewTextChild(node,"document");
  NewTextChild(docNode, "type", doc.type);
  NewTextChild(docNode, "issue_country", PaxDocCountryToXML(doc.issue_country, TReqInfo::Instance()->client_type));
  NewTextChild(docNode, "no", doc.no);
  NewTextChild(docNode, "nationality", PaxDocCountryToXML(doc.nationality, TReqInfo::Instance()->client_type));
  if (doc.birth_date!=ASTRA::NoExists)
    NewTextChild(docNode, "birth_date", DateTimeToStr(doc.birth_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "birth_date");
  NewTextChild(docNode, "gender", doc.gender);
  if (doc.expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(doc.expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "surname", doc.surname);
  NewTextChild(docNode, "first_name", doc.first_name);
  NewTextChild(docNode, "second_name", doc.second_name);
};

void PaxDocoToXML(const CheckIn::TPaxDocoItem &doco,
                  const xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr docNode=NewTextChild(node,"doco");
  NewTextChild(docNode, "birth_place", doco.birth_place);
  NewTextChild(docNode, "type", doco.type);
  NewTextChild(docNode, "no", doco.no);
  NewTextChild(docNode, "issue_place", doco.issue_place);
  if (doco.issue_date!=ASTRA::NoExists)
    NewTextChild(docNode, "issue_date", DateTimeToStr(doco.issue_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "issue_date");
  if (doco.expiry_date!=ASTRA::NoExists)
    NewTextChild(docNode, "expiry_date", DateTimeToStr(doco.expiry_date, ServerFormatDateTimeAsString));
  else
    NewTextChild(docNode, "expiry_date");
  NewTextChild(docNode, "applic_country", PaxDocCountryToXML(doco.applic_country, TReqInfo::Instance()->client_type));
};

void PaxDocFromXML(const xmlNodePtr node,
                   CheckIn::TPaxDocItem &doc)
{
  doc.clear();
  if (node==NULL) return;
  xmlNodePtr node2=NodeAsNode("type", node);

  doc.type=NodeAsStringFast("type",node2);
  doc.issue_country=NodeAsStringFast("issue_country",node2);
  doc.no=NodeAsStringFast("no",node2);
  doc.nationality=NodeAsStringFast("nationality",node2);
  if (!NodeIsNULLFast("birth_date",node2))
    doc.birth_date=NodeAsDateTimeFast("birth_date",node2);
  doc.gender=NodeAsStringFast("gender",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    doc.expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  doc.surname=NodeAsStringFast("surname",node2);
  doc.first_name=NodeAsStringFast("first_name",node2);
  doc.second_name=NodeAsStringFast("second_name",node2);
};

void PaxDocoFromXML(const xmlNodePtr node,
                    CheckIn::TPaxDocoItem &doco)
{
  doco.clear();
  if (node==NULL) return;
  xmlNodePtr node2=NodeAsNode("birth_place", node);

  doco.birth_place=NodeAsStringFast("birth_place",node2);
  doco.type=NodeAsStringFast("type",node2);
  doco.no=NodeAsStringFast("no",node2);
  doco.issue_place=NodeAsStringFast("issue_place",node2);
  if (!NodeIsNULLFast("issue_date",node2))
    doco.issue_date=NodeAsDateTimeFast("issue_date",node2);
  if (!NodeIsNULLFast("expiry_date",node2))
    doco.expiry_date=NodeAsDateTimeFast("expiry_date",node2);
  doco.applic_country=NodeAsStringFast("applic_country",node2);
};

void getPnr( int point_id, int pnr_id, TWebPnr &pnr, bool pr_throw, bool afterSave )
{
  try
  {
    pnr.clear();

    if (!isTestPaxId(pnr_id))
    {
      TQuery CrsTKNQry(&OraSession);
      CrsTKNQry.SQLText =
          "SELECT rem_code AS ticket_rem, "
          "       ticket_no, "
          "       DECODE(rem_code,'TKNE',coupon_no,NULL) AS coupon_no, "
          "       0 AS ticket_confirm "
          "FROM crs_pax_tkn "
          "WHERE pax_id=:pax_id "
          "ORDER BY DECODE(rem_code,'TKNE',0,'TKNA',1,'TKNO',2,3),ticket_no,coupon_no";
      CrsTKNQry.DeclareVariable( "pax_id", otInteger );

      TQuery SeatQry(&OraSession);
      SeatQry.SQLText=
          "BEGIN "
          "  :crs_seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'one',:crs_row); "
          "  :crs_row:=:crs_row+1; "
          "END;";
      SeatQry.DeclareVariable("pax_id", otInteger);
      SeatQry.DeclareVariable("xname", otString);
      SeatQry.DeclareVariable("yname", otString);
      SeatQry.DeclareVariable("seats", otInteger);
      SeatQry.DeclareVariable("point_id", otInteger);
      SeatQry.DeclareVariable("layer_type", otString);
      SeatQry.DeclareVariable("crs_row", otInteger);
      SeatQry.DeclareVariable("crs_seat_no", otString);

      TQuery Qry(&OraSession);
      Qry.SQLText =
          "SELECT crs_pax.pax_id AS crs_pax_id, "
          "       crs_inf.pax_id AS crs_pax_id_parent, "
          "       DECODE(pax.pax_id,NULL,crs_pax.surname,pax.surname) AS surname, "
          "       DECODE(pax.pax_id,NULL,crs_pax.name,pax.name) AS name, "
          "       DECODE(pax.pax_id,NULL,crs_pax.pers_type,pax.pers_type) AS pers_type, "
          "       crs_pax.seat_xname, crs_pax.seat_yname, crs_pax.seats AS crs_seats, crs_pnr.point_id AS point_id_tlg, "
          "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
          "       DECODE(pax.pax_id,NULL,crs_pax.seats,pax.seats) AS seats, "
          "       DECODE(pax_grp.class,NULL,crs_pnr.class,pax_grp.class) AS class, "
          "       DECODE(pax.subclass,NULL,crs_pnr.subclass,pax.subclass) AS subclass, "
          "       DECODE(pax.pax_id,NULL,crs_pnr.airp_arv,pax_grp.airp_arv) AS airp_arv, "
          "       crs_pnr.status AS pnr_status, "
          "       crs_pnr.tid AS crs_pnr_tid, "
          "       crs_pax.tid AS crs_pax_tid, "
          "       pax_grp.tid AS pax_grp_tid, "
          "       pax.tid AS pax_tid, "
          "       pax.pax_id, "
          "       pax_grp.client_type, "
          "       pax.refuse, "
          "       pax.ticket_rem, pax.ticket_no, pax.coupon_no, pax.ticket_confirm, "
          "       pax.reg_no "
          "FROM crs_pnr,crs_pax,pax,pax_grp,crs_inf "
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND "
          "      pax.grp_id=pax_grp.grp_id(+) AND "
          "      crs_pax.pax_id=crs_inf.inf_id(+) AND "
          "      crs_pnr.pnr_id=:pnr_id AND "
          "      crs_pax.pr_del=0";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
      Qry.Execute();
      SeatQry.SetVariable("crs_row", 1);
      SeatQry.SetVariable("layer_type", FNull);
      SeatQry.SetVariable("crs_seat_no", FNull);
      if (!Qry.Eof)
      {
        pnr.checkInfo.set(point_id, Qry.FieldAsString("airp_arv"));
        for(;!Qry.Eof;Qry.Next())
        {
          TWebPax pax;
          pax.crs_pax_id = Qry.FieldAsInteger( "crs_pax_id" );
          if ( !Qry.FieldIsNULL( "crs_pax_id_parent" ) )
            pax.crs_pax_id_parent = Qry.FieldAsInteger( "crs_pax_id_parent" );
          if ( !Qry.FieldIsNULL( "reg_no" ) )
            pax.reg_no = Qry.FieldAsInteger( "reg_no" );
          pax.surname = Qry.FieldAsString( "surname" );
          pax.name = Qry.FieldAsString( "name" );
          pax.pers_type_extended = Qry.FieldAsString( "pers_type" );
          pax.seat_no = Qry.FieldAsString( "seat_no" );

          SeatQry.SetVariable("pax_id", Qry.FieldAsInteger("crs_pax_id"));
          SeatQry.SetVariable("xname", Qry.FieldAsString("seat_xname"));
          SeatQry.SetVariable("yname", Qry.FieldAsString("seat_yname"));
          SeatQry.SetVariable("seats", Qry.FieldAsInteger("crs_seats"));
          SeatQry.SetVariable("point_id", Qry.FieldAsInteger("point_id_tlg"));
          SeatQry.Execute();
          pax.crs_seat_layer=DecodeCompLayerType(SeatQry.GetVariableAsString("layer_type"));
          pax.crs_seat_no=SeatQry.GetVariableAsString("crs_seat_no");
          pax.seats = Qry.FieldAsInteger( "seats" );
          pax.pass_class = Qry.FieldAsString( "class" );
          pax.pass_subclass = Qry.FieldAsString( "subclass" );
          if ( !Qry.FieldIsNULL( "pax_id" ) )
          {
            //пассажир зарегистрирован
            if ( !Qry.FieldIsNULL( "refuse" ) )
              pax.checkin_status = "refused";
            else
            {
              pax.checkin_status = "agent_checked";

              switch(DecodeClientType(Qry.FieldAsString( "client_type" )))
              {
                case ctWeb:
                case ctMobile:
                case ctKiosk:
                  pax.checkin_status = "web_checked";
                  break;
                default: ;
              };
            };
            pax.pax_id = Qry.FieldAsInteger( "pax_id" );
            pax.tkn.fromDB(Qry);
            if (pax.tkn.validET())
              pax.etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
            LoadPaxDoc(pax.pax_id, pax.doc);
            LoadPaxDoco(pax.pax_id, pax.doco);
            CheckIn::LoadPaxFQT(pax.pax_id, pax.fqts);
          }
          else
          {
            pax.checkin_status = "not_checked";
            //проверка CBBG (доп место багажа в салоне)
            /*CrsPaxRemQry.SetVariable( "pax_id", pax.pax_id );
                CrsPaxRemQry.SetVariable( "rem_code", "CBBG" );
                CrsPaxRemQry.Execute();
                if (!CrsPaxRemQry.Eof)*/
            if (pax.name=="CBBG")
              pax.pers_type_extended = "БГ"; //CBBG

            CrsTKNQry.SetVariable( "pax_id", pax.crs_pax_id );
            CrsTKNQry.Execute();
            if (!CrsTKNQry.Eof)
            {
              pax.tkn.fromDB(CrsTKNQry);
              if (pax.tkn.validET())
                pax.etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
            };
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.tkn.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.tkn.getNotEmptyFieldsMask());
            LoadCrsPaxDoc(pax.crs_pax_id, pax.doc, true);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doc.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doc.getNotEmptyFieldsMask());
            LoadCrsPaxVisa(pax.crs_pax_id, pax.doco);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doco.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doco.getNotEmptyFieldsMask());
            LoadCrsPaxDoca(pax.crs_pax_id, pax.doca);

            CheckIn::LoadCrsPaxRem(pax.crs_pax_id, pax.rems);
            CheckIn::LoadCrsPaxFQT(pax.crs_pax_id, pax.fqts);

            TTripInfo flt;
            flt.getByPointId(point_id);

            if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")))
              pax.agent_checkin_reasons.insert("pnr_status");
            if (is_web_cancel(pax.crs_pax_id))
              pax.agent_checkin_reasons.insert("web_cancel");
            if (!is_valid_pax_nationality(flt, pax.crs_pax_id))
              pax.agent_checkin_reasons.insert("pax_nationality");
            if (!is_valid_doc_info(pnr.checkInfo, pax.doc))
              pax.agent_checkin_reasons.insert("incomplete_doc");
            if (!is_valid_doco_info(pnr.checkInfo, pax.doco))
              pax.agent_checkin_reasons.insert("incomplete_doco");
            if (!is_valid_doca_info(pnr.checkInfo, pax.doca))
              pax.agent_checkin_reasons.insert("incomplete_doca");
            if (!is_valid_tkn_info(pnr.checkInfo, pax.tkn))
              pax.agent_checkin_reasons.insert("incomplete_tkn");
            if (!is_valid_rem_codes(flt, pax.rems))
              pax.agent_checkin_reasons.insert("forbidden_rem_code");

            if (!pax.agent_checkin_reasons.empty())
              pax.checkin_status = "agent_checkin";
          }
          pax.crs_pnr_tid = Qry.FieldAsInteger( "crs_pnr_tid" );
          pax.crs_pax_tid = Qry.FieldAsInteger( "crs_pax_tid" );
          if ( !Qry.FieldIsNULL( "pax_grp_tid" ) )
            pax.pax_grp_tid = Qry.FieldAsInteger( "pax_grp_tid" );
          if ( !Qry.FieldIsNULL( "pax_tid" ) )
            pax.pax_tid = Qry.FieldAsInteger( "pax_tid" );
          pnr.paxs.push_back( pax );
        };
      };
    }
    else
    {
      TQuery Qry(&OraSession);
        Qry.SQLText =
        "SELECT surname, name, subcls.class, subclass, doc_no, tkn_no, "
        "       pnr_airline AS fqt_airline, fqt_no, "
        "       seat_xname, seat_yname "
        "FROM test_pax, subcls "
        "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
        Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        //тестовый пассажир
        TWebPax pax;
        pax.crs_pax_id = pnr_id;
        pax.surname = Qry.FieldAsString("surname");
        pax.name = Qry.FieldAsString("name");
        pax.pers_type_extended = EncodePerson(adult);
        pax.seats = 1;
        pax.pass_class = Qry.FieldAsString( "class" );
        pax.pass_subclass = Qry.FieldAsString( "subclass" );

        if (afterSave)
        {
          pax.checkin_status = "web_checked";
          if (!Qry.FieldIsNULL("seat_yname") && !Qry.FieldIsNULL("seat_xname"))
          {
            TSeat seat(Qry.FieldAsString("seat_yname"),Qry.FieldAsString("seat_xname"));
            pax.seat_no=GetSeatView(seat, "one", false);
          };
        }
        else
          pax.checkin_status = "not_checked";

        pax.tkn.no=Qry.FieldAsString("tkn_no");
        pax.tkn.coupon=1;
        pax.tkn.rem="TKNE";

        pax.doc.no=Qry.FieldAsString("doc_no");

        if (!Qry.FieldIsNULL("fqt_airline") && !Qry.FieldIsNULL("fqt_no"))
        {
          CheckIn::TPaxFQTItem fqt;
          fqt.rem="FQTV";
          fqt.airline=Qry.FieldAsString("fqt_airline");
          fqt.no=Qry.FieldAsString("fqt_no");
          pax.fqts.insert(fqt);
        };

        pax.crs_pnr_tid = pnr_id;
        pax.crs_pax_tid = pnr_id;
        if (afterSave)
        {
          pax.pax_grp_tid = point_id;
            pax.pax_tid = point_id;
        };

        pnr.paxs.push_back( pax );
      };
    };
    ProgTrace( TRACE5, "pass count=%zu", pnr.paxs.size() );
    if ( pnr.paxs.empty() )
      throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
    }
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return;
  };
}

bool MoreThanOnePersonWithSeat(const TWebPnr &pnr)
{
  int persons=0;
  for(vector<TWebPax>::const_iterator iPax=pnr.paxs.begin(); iPax!=pnr.paxs.end(); ++iPax)
  {
    if (iPax->seats!=0) persons++;
    if (persons>1) return true;
  };
  return persons>1;
}

void IntLoadPnr( const vector<TIdsPnrData> &ids,
                 const boost::optional<WebSearch::TPNRFilter> &filter,
                 vector< TWebPnr > &pnrs,
                 xmlNodePtr segsNode,
                 bool afterSave )
{
  pnrs.clear();
  for(vector<TIdsPnrData>::const_iterator i=ids.begin();i!=ids.end();i++)
  {
    int point_id=i->point_id;
    int pnr_id=i->pnr_id;

    try
    {
      TWebPnr pnr;
      getPnr( point_id, pnr_id, pnr, pnrs.empty(), afterSave );
      if (pnrs.begin()!=pnrs.end())
      {
        //фильтруем пассажиров из второго и следующих сегментов
        const TWebPnr &firstPnr=*(pnrs.begin());
        for(vector<TWebPax>::iterator iPax=pnr.paxs.begin();iPax!=pnr.paxs.end();)
        {
          //удалим дублирование фамилия+имя из pnr
          bool pr_double=false;
          for(vector<TWebPax>::iterator iPax2=iPax+1;iPax2!=pnr.paxs.end();)
          {
            if (transliter_equal(iPax->surname,iPax2->surname) &&
                transliter_equal(iPax->name,iPax2->name))
            {
              pr_double=true;
              iPax2=pnr.paxs.erase(iPax2);
              continue;
            };
            iPax2++;
          };
          if (pr_double)
          {
            iPax=pnr.paxs.erase(iPax);
            continue;
          };

          vector<TWebPax>::const_iterator iPaxFirst=find(firstPnr.paxs.begin(),firstPnr.paxs.end(),*iPax);
          if (iPaxFirst==firstPnr.paxs.end())
          {
            //не нашли соответствующего пассажира из первого сегмента
            iPax=pnr.paxs.erase(iPax);
            continue;
          };
          iPax->pax_no=iPaxFirst->pax_no; //проставляем пассажиру соотв. виртуальный ид. из первого сегмента
          iPax++;
        };
      }
      else
      {
        if (filter)
        {
          set<int> suitable_ids;
          for(int pass=0; pass<2; pass++)
          {
            //отфильтруем пассажиров
            for(vector<TWebPax>::iterator iPax=pnr.paxs.begin();iPax!=pnr.paxs.end();)
            {
              if ((pass==0 && iPax->seats==0) || (pass!=0 && iPax->seats!=0))
              {
                ++iPax;
                continue;
              };
              if (iPax->seats!=0)
              {
                //не младенцы без мест
                if (iPax->suitable(filter.get()))
                {
                  suitable_ids.insert(iPax->crs_pax_id);
                  ++iPax;
                }
                else
                  iPax=pnr.paxs.erase(iPax);
              }
              else
              {
                //младенцы без мест
                //оставляем только младенцев, привязанных к отфильтрованным взрослым
                if (iPax->crs_pax_id_parent!=NoExists &&
                    suitable_ids.find(iPax->crs_pax_id_parent)!=suitable_ids.end())
                  ++iPax;
                else
                  iPax=pnr.paxs.erase(iPax);
              };
            };
          };
        };

        //пассажирам первого сегмента проставим pax_no по порядку
        int pax_no=1;
        for(vector<TWebPax>::iterator iPax=pnr.paxs.begin();iPax!=pnr.paxs.end();iPax++,pax_no++) iPax->pax_no=pax_no;
      };

      pnrs.push_back(pnr);

      if (segsNode==NULL) continue;

      xmlNodePtr segNode=NewTextChild(segsNode, "segment");
      NewTextChild( segNode, "point_id", point_id );
      NewTextChild( segNode, "airline", i->airline );
      i->flt_no==NoExists?NewTextChild( segNode, "flt_no" ):
                          NewTextChild( segNode, "flt_no", i->flt_no );
      NewTextChild( segNode, "suffix", i->suffix );

      NewTextChild( segNode, "apis", (int)(!pnr.checkInfo.apis_formats().empty()) );
      checkDocInfoToXML(pnr.checkInfo, segNode);

      xmlNodePtr node = NewTextChild( segNode, "passengers" );
      for ( vector<TWebPax>::const_iterator iPax=pnr.paxs.begin(); iPax!=pnr.paxs.end(); iPax++ )
      {
        xmlNodePtr paxNode = NewTextChild( node, "pax" );
        if (iPax->pax_no!=NoExists)
          NewTextChild( paxNode, "pax_no", iPax->pax_no );
        NewTextChild( paxNode, "crs_pax_id", iPax->crs_pax_id );
        if ( iPax->crs_pax_id_parent != NoExists )
            NewTextChild( paxNode, "crs_pax_id_parent", iPax->crs_pax_id_parent );
        if ( iPax->reg_no != NoExists )
            NewTextChild( paxNode, "reg_no", iPax->reg_no );
        NewTextChild( paxNode, "surname", iPax->surname );
        NewTextChild( paxNode, "name", iPax->name );
        if ( iPax->doc.birth_date != NoExists )
            NewTextChild( paxNode, "birth_date", DateTimeToStr( iPax->doc.birth_date, ServerFormatDateTimeAsString ) );
        NewTextChild( paxNode, "pers_type", iPax->pers_type_extended );
        string seat_no_view;
        if ( !iPax->seat_no.empty() )
          seat_no_view = iPax->seat_no;
        else
                if ( !iPax->crs_seat_no.empty() )
                  seat_no_view = iPax->crs_seat_no;
        NewTextChild( paxNode, "seat_no", seat_no_view );
        string seat_status;
        if (i->pr_paid_ckin)
        {
          switch(iPax->crs_seat_layer)
          {
            case cltPNLBeforePay:  seat_status="PNLBeforePay";  break;
            case cltPNLAfterPay:   seat_status="PNLAfterPay";   break;
            case cltProtBeforePay: seat_status="ProtBeforePay"; break;
            case cltProtAfterPay:  seat_status="ProtAfterPay";  break;
            default: break;
          };
        };
        NewTextChild( paxNode, "seat_status", seat_status );
        NewTextChild( paxNode, "seats", iPax->seats );
        NewTextChild( paxNode, "checkin_status", iPax->checkin_status );
        xmlNodePtr reasonsNode=NewTextChild( paxNode, "agent_checkin_reasons" );
        for(set<string>::const_iterator r=iPax->agent_checkin_reasons.begin();
                                        r!=iPax->agent_checkin_reasons.end(); ++r)
          NewTextChild(reasonsNode, "reason", *r);

        if ( iPax->tkn.rem == "TKNE" )
          NewTextChild( paxNode, "eticket", "true" );
        else
            NewTextChild( paxNode, "eticket", "false" );
        NewTextChild( paxNode, "ticket_no", iPax->tkn.no );

        PaxDocToXML(iPax->doc, paxNode);
        PaxDocoToXML(iPax->doco, paxNode);

        xmlNodePtr fqtsNode = NewTextChild( paxNode, "fqt_rems" );
        for(set<CheckIn::TPaxFQTItem>::const_iterator f=iPax->fqts.begin(); f!=iPax->fqts.end(); ++f)
          if (f->rem=="FQTV") f->toXML(fqtsNode);

        if (!iPax->etick.empty())
        {
          if (iPax->etick.bag_norm==ASTRA::NoExists || iPax->etick.bag_norm==0)
            NewTextChild( paxNode, "bag_norm", 0 );
          else
            SetProp(NewTextChild( paxNode, "bag_norm", iPax->etick.bag_norm ), "unit", iPax->etick.bag_norm_unit.get_db_form() );

        }
        else NewTextChild( paxNode, "bag_norm" );

        xmlNodePtr tidsNode = NewTextChild( paxNode, "tids" );
        NewTextChild( tidsNode, "crs_pnr_tid", iPax->crs_pnr_tid );
        NewTextChild( tidsNode, "crs_pax_tid", iPax->crs_pax_tid );
        if ( iPax->pax_grp_tid != NoExists )
            NewTextChild( tidsNode, "pax_grp_tid", iPax->pax_grp_tid );
        if ( iPax->pax_tid != NoExists )
            NewTextChild( tidsNode, "pax_tid", iPax->pax_tid );
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };
}

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::LoadPnr");
  xmlNodePtr segsNode = NodeAsNode( "segments", reqNode );
  vector<TIdsPnrData> ids;
  for(xmlNodePtr node=segsNode->children; node!=NULL; node=node->next)
  {
    int point_id=NodeAsInteger( "point_id", node );
    WebSearch::TFlightInfo flt;
    flt.fromDB(point_id, false, true);
    flt.fromDBadditional(false, true);
    int pnr_id=VerifyPNRById( point_id, node );
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=point_id;
    idsPnrData.airline=flt.oper.airline;
    idsPnrData.flt_no=flt.oper.flt_no;
    idsPnrData.suffix=flt.oper.suffix;
    idsPnrData.pnr_id=pnr_id;
    idsPnrData.pr_paid_ckin=flt.pr_paid_ckin;
    ids.push_back( idsPnrData );
  };

  bool charter_search=!ids.empty() &&
                      GetSelfCkinSets(tsSelfCkinCharterSearch, ids.front().point_id, TReqInfo::Instance()->client_type);
  boost::optional<WebSearch::TPNRFilter> filter;
  if (charter_search)
  {
    xmlNodePtr searchParamsNode=GetNode("search_params", reqNode);
    if (searchParamsNode!=NULL)
    {
      filter=WebSearch::TPNRFilter();
      filter.get().fromXML(searchParamsNode);
    }
  };

  vector< TWebPnr > pnrs;
  segsNode = NewTextChild( NewTextChild( resNode, "LoadPnr" ), "segments" );
  IntLoadPnr( ids, filter, pnrs, segsNode, false );
  if (charter_search && !pnrs.empty() && MoreThanOnePersonWithSeat(*(pnrs.begin())))
    throw UserException("MSG.CHARTER_SEARCH.FOUND_MORE.ADJUST_SEARCH_PARAMS");
}

bool isOwnerFreePlace( int pax_id, const vector<TWebPax> &pnr )
{
  bool res = false;
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
    if ( i->pax_id != NoExists )
        continue;
    if ( i->crs_pax_id == pax_id ) {
        res = true;
        break;
    }
  }
  return res;
}

bool isOwnerPlace( int pax_id, const vector<TWebPax> &pnr )
{
  bool res = false;
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
    if ( i->pax_id != NoExists && pax_id == i->pax_id ) {
        res = true;
        break;
    }
  }
  return res;
}


struct TWebPlace {
    int x, y;
    string xname;
    string yname;
    string seat_no;
    string elem_type;
    int pr_free;
    int pr_CHIN;
    int pax_id;
    SALONS2::TSeatTariff SeatTariff;
    TWebPlace() {
      pr_free = 0;
      pr_CHIN = 0;
    pax_id = NoExists;
  }
};

typedef std::vector<TWebPlace> TWebPlaces;

struct TWebPlaceList {
    TWebPlaces places;
    int xcount, ycount;
};

void ReadWebSalons( int point_id, vector<TWebPax> pnr, map<int, TWebPlaceList> &web_salons, bool &pr_find_free_subcls_place )
{
  bool isTranzitSalonsVersion = isTranzitSalons( point_id );
  int point_arv = NoExists;
  string crs_class, crs_subclass;
  web_salons.clear();
  bool pr_CHIN = false;
  bool pr_INFT = false;
  /*TSeatTariffMap passTariffs, firstTariffs;*/

  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
    if ( !i->pass_class.empty() )
      crs_class = i->pass_class;
    if ( !i->pass_subclass.empty() )
      crs_subclass = i->pass_subclass;
    TPerson p=DecodePerson(i->pers_type_extended.c_str());
    pr_CHIN=(pr_CHIN || p==ASTRA::child || p==ASTRA::baby); //среди типов может быть БГ (CBBG) который приравнивается к взрослому
    pr_INFT=(pr_INFT || p==ASTRA::baby);
    if ( isTranzitSalonsVersion ) {
      if ( point_arv == ASTRA::NoExists ) {
        point_arv = SALONS2::getCrsPaxPointArv( i->crs_pax_id, point_id );
      }
/*      TMktFlight flight;
      flight.getByCrsPaxId( i->crs_pax_id );
      TTripInfo markFlt;
      markFlt.airline = flight.airline;
      CheckIn::TPaxTknItem tkn;
      CheckIn::LoadCrsPaxTkn( i->crs_pax_id, tkn);*/
    }
  }
  ProgTrace( TRACE5, "ReadWebSalons: point_dep=%d, point_arv=%d, pr_CHIN=%d, pr_INFT=%d",
             point_id, point_arv, pr_CHIN, pr_INFT );
  if ( crs_class.empty() )
    throw UserException( "MSG.CLASS.NOT_SET" );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  TSublsRems subcls_rems( Qry.FieldAsString("airline") );
  SALONS2::TSalonList salonList;
  SALONS2::TSalons SalonsO( point_id, SALONS2::rTripSalons );
  if ( isTranzitSalonsVersion ) {
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), SALONS2::rfTranzitVersion, crs_class, NoExists );
  }
  else {
    SalonsO.FilterClass = crs_class;
    SalonsO.Read();
  }
  // получим признак того, что в салоне есть свободные места с данным подклассом
  pr_find_free_subcls_place=false;
  string pass_rem;

  subcls_rems.IsSubClsRem( crs_subclass, pass_rem );
  TSalons *Salons;
  TSalons SalonsN;
  if ( isTranzitSalonsVersion ) {
    //задаем все возможные статусы для разметки группы
    vector<ASTRA::TCompLayerType> grp_layers;
    grp_layers.push_back( cltCheckin );
    grp_layers.push_back( cltTCheckin );
    grp_layers.push_back( cltTranzit );
    grp_layers.push_back( cltProtBeforePay );
    grp_layers.push_back( cltProtAfterPay );
    grp_layers.push_back( cltPNLBeforePay );
    grp_layers.push_back( cltPNLAfterPay );
    TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
    bool pr_departure_tariff_only = true;
    TDropLayersFlags dropLayersFlags;
    salonList.CreateSalonsForAutoSeats( SalonsN,
                                        filterRoutes,
                                        pr_departure_tariff_only,
                                        grp_layers,
                                        pnr,
                                        dropLayersFlags );
   Salons = &SalonsN;
   if ( salonList.getRFISCMode() != rTariff  ) {
     Salons->SetTariffsByRFISC(point_id);
   }
  }
  else {
    Salons = &SalonsO;
  }
  for( vector<TPlaceList*>::iterator placeList = Salons->placelists.begin();
       placeList != Salons->placelists.end(); placeList++ ) {
    TWebPlaceList web_place_list;
    web_place_list.xcount=0;
    web_place_list.ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) { // пробег по салонам
      if ( !place->visible )
       continue;
      TWebPlace wp;
      wp.x = place->x;
      wp.y = place->y;
      wp.xname = place->xname;
      wp.yname = place->yname;
      if ( place->x > web_place_list.xcount )
        web_place_list.xcount = place->x;
      if ( place->y > web_place_list.ycount )
        web_place_list.ycount = place->y;
      wp.seat_no = denorm_iata_row( place->yname, NULL ) + denorm_iata_line( place->xname, Salons->getLatSeat() );
      if ( !place->elem_type.empty() ) {
        if ( place->elem_type != PARTITION_ELEM_TYPE )
            wp.elem_type = ARMCHAIR_ELEM_TYPE;
          else
            wp.elem_type = PARTITION_ELEM_TYPE;
        }
        wp.pr_free = 0;
        wp.pr_CHIN = false;
        wp.pax_id = NoExists;
        wp.SeatTariff = place->SeatTariff;
        if ( place->isplace && !place->clname.empty() && place->clname == crs_class ) {
            bool pr_first = true;
        for( std::vector<TPlaceLayer>::iterator ilayer=place->layers.begin(); ilayer!=place->layers.end(); ilayer++ ) { // сортировка по приоритетам
          ProgTrace( TRACE5, "%s, %s", EncodeCompLayerType(ilayer->layer_type), string(place->yname+place->xname).c_str() );
            if ( pr_first &&
                   ilayer->layer_type != cltUncomfort &&
                   ilayer->layer_type != cltSmoke &&
                   ilayer->layer_type != cltUnknown ) {
                pr_first = false;
                    wp.pr_free = ( ( ilayer->layer_type == cltPNLCkin ||
                             SALONS2::isUserProtectLayer( ilayer->layer_type ) ) && isOwnerFreePlace( ilayer->pax_id, pnr ) );
            ProgTrace( TRACE5, "l->layer_type=%s, l->pax_id=%d, isOwnerFreePlace( l->pax_id, pnr )=%d, pr_first=%d",
                       EncodeCompLayerType(ilayer->layer_type), ilayer->pax_id, isOwnerFreePlace( ilayer->pax_id, pnr ), pr_first );
                if ( wp.pr_free )
                    break;
            }

            if ( ilayer->layer_type == cltCheckin ||
                   ilayer->layer_type == cltTCheckin ||
                   ilayer->layer_type == cltGoShow ||
                   ilayer->layer_type == cltTranzit ) {
                pr_first = false;
          if ( isOwnerPlace( ilayer->pax_id, pnr ) )
                  wp.pax_id = ilayer->pax_id;
            }
      }

          wp.pr_free = ( wp.pr_free || pr_first ); // 0 - занято, 1 - свободно, 2 - частично занято

        if ( wp.pr_free ) {
          //место свободно
          //вычисляем подкласс места
          string seat_subcls;
            for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
                if ( isREM_SUBCLS( i->rem ) && !i->pr_denial ) {
              seat_subcls = i->rem;
                    break;
                  }
            }
          if ( !pass_rem.empty() ) {
            //у пассажира есть подкласс
            if ( pass_rem == seat_subcls ) {
              wp.pr_free = 3;  // свободно с учетом подкласса
              pr_find_free_subcls_place = true;
            }
            else
              if ( seat_subcls.empty() ) { // нет подкласса у места
                wp.pr_free = 2; // свободно без учета подкласса
              }
              else { // подклассы не совпали
                wp.pr_free = 0; // занято
              }
          }
          else {
            // у пассажира нет подкласса
            if ( !seat_subcls.empty() ) { // подкласс у места
                wp.pr_free = 0; // занято
            }
          }
        }
        if ( pr_CHIN ) { // встречаются в группе пассажиры с детьми
            if ( place->elem_type == "А" ) { // место у аварийного выхода
                wp.pr_CHIN = true;
          }
          else {
              for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
                if ( i->pr_denial && i->rem == "CHIN" ) {
                      wp.pr_CHIN = true;
                    break;
                  }
              }
            }
        }
        if ( pr_INFT ) {
          for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
            if ( i->pr_denial && i->rem == "INFT" ) {
                  wp.pr_CHIN = true;
                break;
              }
          }
        }
      } // end if place->isplace && !place->clname.empty() && place->clname == crs_class
      web_place_list.places.push_back( wp );
    }
    if ( !web_place_list.places.empty() ) {
        web_salons[ (*placeList)->num ] = web_place_list;
    }
  }
}

int get_seat_status( TWebPlace &wp, bool pr_find_free_subcls_place )
{
  int status = 0;
  switch( wp.pr_free ) {
    case 0: // занято
        status = 1;
        break;
    case 1: // свободно
        status = 0;
        break;
    case 2: // свободно без учета подкласса
        status = pr_find_free_subcls_place;
        break;
    case 3: // свободно с учетом подкласса
        status = !pr_find_free_subcls_place;
        break;
  };
  if ( status == 0 && wp.pr_CHIN ) {
    status = 2;
  }
  if ( TReqInfo::Instance()->client_type == ctKiosk && status != 1 && !wp.SeatTariff.empty() ) {
    status = 1;
  }
  return status;
}

// передается заполненные поля crs_pax_id, crs_seat_no, class, subclass
// на выходе заполнено TWebPlace по пассажиру
void GetCrsPaxSeats( int point_id, const vector<TWebPax> &pnr,
                     vector< pair<TWebPlace, LexemaData> > &pax_seats )
{
  pax_seats.clear();
  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  ReadWebSalons( point_id, pnr, web_salons, pr_find_free_subcls_place );
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) { // пробег по пассажирам
    LexemaData ld;
    bool pr_find = false;
    for( map<int, TWebPlaceList>::iterator isal=web_salons.begin(); isal!=web_salons.end(); isal++ ) {
      for ( TWebPlaces::iterator wp = isal->second.places.begin();
            wp != isal->second.places.end(); wp++ ) {
        if ( i->crs_seat_no == wp->seat_no ) {
          if ( get_seat_status( *wp, pr_find_free_subcls_place ) == 1 )
            ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_AVAIL";
          wp->pax_id = i->crs_pax_id;
          pr_find = true;
          pax_seats.push_back( make_pair( *wp, ld ) );
            break;
        }
      } // пробег по салону
      if ( pr_find )
        break;
    } // пробег по салону
    if ( !pr_find ) {
      TWebPlace wp;
      wp.pax_id = i->crs_pax_id;
      wp.seat_no = i->crs_seat_no;
      ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_FOUND";
      pax_seats.push_back( make_pair( wp, ld ) );
    }
  } // пробег по пассажирам
}

/*
1. Что делать если пассажир имеет спец. подкласс (ремарки MCLS) - Пока выбираем только места с ремарками нужного подкласса.
Что делать , если салон не размечен?
2. Есть группа пассажиров, некоторые уже зарегистрированы, некоторые нет.
*/
void WebRequestsIface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::ViewCraft");
  int point_id = NodeAsInteger( "point_id", reqNode );
  if ( SALONS2::isFreeSeating( point_id ) ) { //???
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }
  TWebPnr pnr;
  WebSearch::TFlightInfo flt;
  flt.fromDB(point_id, false, true);

  int pnr_id=VerifyPNRById(point_id, reqNode);

  getPnr( point_id, pnr_id, pnr, true, false );

  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  ReadWebSalons( point_id, pnr.paxs, web_salons, pr_find_free_subcls_place );

  xmlNodePtr node = NewTextChild( resNode, "ViewCraft" );
  node = NewTextChild( node, "salons" );
  for( map<int, TWebPlaceList>::iterator isal=web_salons.begin(); isal!=web_salons.end(); isal++ ) {
    xmlNodePtr placeListNode = NewTextChild( node, "placelist" );
    SetProp( placeListNode, "num", isal->first );
    SetProp( placeListNode, "xcount", isal->second.xcount + 1 );
    SetProp( placeListNode, "ycount", isal->second.ycount + 1 );
    for ( TWebPlaces::iterator wp = isal->second.places.begin();
          wp != isal->second.places.end(); wp++ ) {
      xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );
      NewTextChild( placeNode, "x", wp->x );
      NewTextChild( placeNode, "y", wp->y );
      NewTextChild( placeNode, "seat_no", wp->seat_no );
      NewTextChild( placeNode, "elem_type", wp->elem_type );
      NewTextChild( placeNode, "status", get_seat_status( *wp, pr_find_free_subcls_place ) );
      if ( wp->pax_id != NoExists )
        NewTextChild( placeNode, "pax_id", wp->pax_id );
      if ( !wp->SeatTariff.empty() ) { // если платная регистрация отключена, value=0.0 в любом случае
        xmlNodePtr rateNode = NewTextChild( placeNode, "rate" );
        NewTextChild( rateNode, "color", wp->SeatTariff.color );
        NewTextChild( rateNode, "value", wp->SeatTariff.rateView() );
        NewTextChild( rateNode, "currency", wp->SeatTariff.currencyView(reqInfo->desk.lang) );
      }
    }
  }
}

bool CreateEmulCkinDocForCHKD(int crs_pax_id,
                              vector<WebSearch::TPnrData> &PNRs,
                              const XMLDoc &emulDocHeader,
                              XMLDoc &emulCkinDoc,
                              list<int> &crs_pax_ids)  //crs_pax_ids включает кроме crs_pax_id ид. привязанных младенцев
{
  crs_pax_ids.clear();
  if (PNRs.size()!=1)
    throw EXCEPTIONS::Exception("CreateEmulCkinDocForCHKD: PNRs.size()!=1");

  TWebPnrForSave pnr;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT crs_pnr.subclass, "
    "       crs_pax.pnr_id, "
    "       crs_pax.pax_id, "
    "       crs_pax.surname, "
    "       crs_pax.name, "
    "       crs_pax.pers_type, "
    "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
    "       crs_pax.seat_type, "
    "       crs_pax.seats, "
    "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
    "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket, "
    "       crs_pnr.tid AS crs_pnr_tid, "
    "       crs_pax.tid AS crs_pax_tid, "
    "       crs_pax_ids.reg_no "
    "FROM crs_pnr, crs_pax, pax, "
    "     (SELECT crs_pax.pax_id, crs_pax_chkd.reg_no "
    "      FROM crs_pax, crs_pax_chkd "
    "      WHERE crs_pax.pax_id=crs_pax_chkd.pax_id(+) AND "
    "            crs_pax.pax_id=:pax_id "
    "      UNION "
    "      SELECT crs_inf.inf_id AS pax_id, crs_pax_chkd.reg_no "
    "      FROM crs_inf, crs_pax_chkd "
    "      WHERE crs_inf.inf_id=crs_pax_chkd.pax_id(+) AND "
    "            crs_inf.pax_id=:pax_id) crs_pax_ids "
    "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pax.pax_id=crs_pax_ids.pax_id AND "
    "      crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pr_del=0 ";
  Qry.CreateVariable("pax_id", otInteger, crs_pax_id);
  Qry.Execute();
  if (Qry.Eof) return false;

  int adult_count=0, without_seat_count=0;
  map<int/*crs_pax_id*/, int/*reg_no*/> reg_no_map;
  for(;!Qry.Eof;Qry.Next())
  {
    if (pnr.pnr_id==NoExists) pnr.pnr_id=Qry.FieldAsInteger("pnr_id");
    else
    {
      if (pnr.pnr_id!=Qry.FieldAsInteger("pnr_id"))
        throw EXCEPTIONS::Exception("CreateEmulCkinDocForCHKD: different pnr_id");
    };

    TWebPaxFromReq paxFromReq;
    paxFromReq.crs_pax_id=Qry.FieldAsInteger("pax_id");
    paxFromReq.crs_pnr_tid=Qry.FieldAsInteger("crs_pnr_tid");
    paxFromReq.crs_pax_tid=Qry.FieldAsInteger("crs_pax_tid");

    TWebPaxForCkin paxForCkin;
    paxForCkin.crs_pax_id=Qry.FieldAsInteger("pax_id");
    paxForCkin.surname = Qry.FieldAsString("surname");
    paxForCkin.name = Qry.FieldAsString("name");
    paxForCkin.pers_type = Qry.FieldAsString("pers_type");
    paxForCkin.seat_no = Qry.FieldAsString("seat_no");
    paxForCkin.seat_type = Qry.FieldAsString("seat_type");
    paxForCkin.seats = Qry.FieldAsInteger("seats");
    paxForCkin.eticket = Qry.FieldAsString("eticket");
    paxForCkin.ticket = Qry.FieldAsString("ticket");
    LoadCrsPaxDoc(paxForCkin.crs_pax_id, paxForCkin.apis.doc, true);
    LoadCrsPaxVisa(paxForCkin.crs_pax_id, paxForCkin.apis.doco);
    paxForCkin.subclass = Qry.FieldAsString("subclass");
    paxForCkin.reg_no = Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");

    if (paxForCkin.reg_no!=NoExists)
    {
      if (paxForCkin.reg_no<1 || paxForCkin.reg_no>999)
      {
        ostringstream reg_no_str;
        reg_no_str << setw(3) << setfill('0') << paxForCkin.reg_no;
        throw UserException("MSG.CHECKIN.REG_NO_NOT_SUPPORTED", LParams() << LParam("reg_no", reg_no_str.str()));

      };

      map<int, int>::iterator i=reg_no_map.insert( make_pair(paxForCkin.crs_pax_id, paxForCkin.reg_no) ).first;
      if (i==reg_no_map.end()) throw EXCEPTIONS::Exception("CreateEmulCkinDocForCHKD: i==reg_no_map.end()");
      if (i->second!=paxForCkin.reg_no)
        throw UserException("MSG.CHECKIN.DUPLICATE_CHKD_REG_NO");
    };

    pnr.paxFromReq.push_back(paxFromReq);
    pnr.paxForCkin.push_back(paxForCkin);

    TPerson p=DecodePerson(paxForCkin.pers_type.c_str());
    if (p==ASTRA::adult) adult_count++;
    if (p==ASTRA::baby && paxForCkin.seats==0) without_seat_count++;
  };

  int parent_reg_no=NoExists;
  map<int, int>::iterator i=reg_no_map.find(crs_pax_id);
  if (i!=reg_no_map.end()) parent_reg_no=i->second;
  if (parent_reg_no==NoExists) return false; //у родителя нет рег. номера из CHKD
  for(list<TWebPaxForCkin>::iterator p=pnr.paxForCkin.begin(); p!=pnr.paxForCkin.end(); ++p)
    if (p->reg_no==NoExists) p->reg_no=parent_reg_no;

  if (without_seat_count>adult_count)
    throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
  if (pnr.pnr_id==NoExists)
    throw EXCEPTIONS::Exception("CreateEmulCkinDocForCHKD: unknown pnr_id (crs_pax_id=%d)", crs_pax_id);

  WebSearch::TPnrData &pnrData=*(PNRs.begin());
  CompletePnrData(false, pnr.pnr_id, pnrData);

  map<int,XMLDoc> emulChngDocs;
  CreateEmulDocs(vector< pair<int/*point_id*/, TWebPnrForSave > >(1, make_pair(pnrData.flt.point_dep, pnr)),
                 PNRs,
                 emulDocHeader,
                 emulCkinDoc,
                 emulChngDocs);

  for(list<TWebPaxForCkin>::const_iterator p=pnr.paxForCkin.begin(); p!=pnr.paxForCkin.end(); ++p)
    crs_pax_ids.push_back(p->crs_pax_id);

  return true;
};

void VerifyPax(vector< pair<int, TWebPnrForSave > > &segs, const XMLDoc &emulDocHeader,
               XMLDoc &emulCkinDoc, map<int,XMLDoc> &emulChngDocs, vector<TIdsPnrData> &ids)
{
  ids.clear();

  if (segs.empty()) return;

  TReqInfo *reqInfo = TReqInfo::Instance();

  //первым делом проверяем, что незарегистрированные пассажиры совпадают по кол-ву для каждого сегмента
  //на последних сегментах кол-во незарегистрированных пассажиров м.б. нулевым
  int prevNotCheckedCount=NoExists;
  int seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    int currNotCheckedCount=0;
    for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
    {
      if (iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists)
        //пассажир не зарегистрирован
        currNotCheckedCount++;
    };

    if (prevNotCheckedCount!=NoExists && currNotCheckedCount!=0)
    {
      if (prevNotCheckedCount!=currNotCheckedCount)
        throw EXCEPTIONS::Exception("VerifyPax: different number of passengers for through check-in (seg_no=%d)", seg_no);
    };

    prevNotCheckedCount=currNotCheckedCount;
  };

  WebSearch::TPnrData first;
  first.flt.fromDB(segs.begin()->first, true, true);
  first.flt.fromDBadditional(true, true);

  const char* PaxQrySQL=
        "SELECT point_dep,point_arv,airp_dep,airp_arv,class,excess,bag_refuse, "
        "       pax_grp.grp_id,pax.surname,pax.name,pax.pers_type,pax.seats, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
        "       pax_grp.tid AS pax_grp_tid, "
        "       pax.tid AS pax_tid, "
        "       crs_pax.pnr_id, crs_pax.pr_del "
        "FROM pax_grp,pax,crs_pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax.pax_id=crs_pax.pax_id(+) AND "
        "      pax.pax_id=:pax_id";

  const char* CrsPaxQrySQL=
      "SELECT tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
      "       tlg_trips.scd AS scd_out,tlg_trips.airp_dep AS airp, "
      "       crs_pnr.point_id,crs_pnr.subclass, "
      "       crs_pnr.status AS pnr_status, "
      "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket, "
      "       crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       DECODE(pax.pax_id,NULL,0,1) AS checked "
      "FROM tlg_trips,crs_pnr,crs_pax,pax "
      "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pax.pax_id=:crs_pax_id AND "
      "      crs_pax.pr_del=0 ";

  const char* TestPaxQrySQL=
      "SELECT subclass, NULL AS pnr_status, "
      "       surname, NULL AS name, :adult AS pers_type, "
      "       NULL AS seat_no, NULL AS seat_type, 1 AS seats, "
      "       id AS pnr_id, "
      "       NULL AS ticket, tkn_no AS eticket, doc_no, "
      "       id AS crs_pnr_tid, id AS crs_pax_tid, 0 AS checked "
      "FROM test_pax "
      "WHERE id=:crs_pax_id";

  TQuery Qry(&OraSession);

  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    s->second.paxForChng.clear();
    s->second.paxForCkin.clear();
    int point_id=s->first;
    try
    {
      if (!s->second.paxFromReq.empty())
      {
        int pnr_id=NoExists; //попробуем определить из секции passengers
        int adult_count=0, without_seat_count=0;
        for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
        {
          try
          {
            bool not_checked=isTestPaxId(iPax->crs_pax_id) ||
                             iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists;

            Qry.Clear();
            try
            {
              if (not_checked)
              {
                //пассажир не зарегистрирован
                if (isTestPaxId(iPax->crs_pax_id))
                {
                  Qry.SQLText=TestPaxQrySQL;
                  Qry.CreateVariable("adult", otString, EncodePerson(adult));
                }
                else
                  Qry.SQLText=CrsPaxQrySQL;
                Qry.CreateVariable("crs_pax_id", otInteger, iPax->crs_pax_id);
                Qry.Execute();
                if (Qry.Eof)
                  throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
                if (Qry.FieldAsInteger("checked")!=0)
                  throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
                if (iPax->crs_pnr_tid!=Qry.FieldAsInteger("crs_pnr_tid") ||
                    iPax->crs_pax_tid!=Qry.FieldAsInteger("crs_pax_tid"))
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

                if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
                    !is_valid_pax_status(point_id, iPax->crs_pax_id))
                  throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");
              }
              else
              {
                //пассажир зарегистрирован
                Qry.SQLText=PaxQrySQL;
                Qry.CreateVariable("pax_id", otInteger, iPax->crs_pax_id);
                Qry.Execute();
                if (Qry.Eof)
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");
                if (Qry.FieldIsNULL("pnr_id") || Qry.FieldAsInteger("pr_del")!=0)
                  throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
                if (iPax->pax_grp_tid!=Qry.FieldAsInteger("pax_grp_tid") ||
                    iPax->pax_tid!=Qry.FieldAsInteger("pax_tid"))
                  throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");
              };
            }
            catch(UserException &E)
            {
              ProgTrace(TRACE5, ">>>> %s (seg_no=%d, crs_pax_id=%d)",
                                getLocaleText(E.getLexemaData()).c_str(), seg_no, iPax->crs_pax_id);
              throw;
            };

            if (pnr_id==ASTRA::NoExists)
            {
              //первый пассажир
              pnr_id=Qry.FieldAsInteger("pnr_id");
              //проверим, что данное PNR привязано к рейсу
              VerifyPNRByPnrId(point_id,pnr_id);
            }
            else
            {
              if (Qry.FieldAsInteger("pnr_id")!=pnr_id)
                throw EXCEPTIONS::Exception("VerifyPax: passengers from different PNR (seg_no=%d)", seg_no);
            };

            if (!not_checked)
            {
              TWebPaxForChng pax;
              pax.crs_pax_id = iPax->crs_pax_id;
              pax.grp_id = Qry.FieldAsInteger("grp_id");
              pax.point_dep = Qry.FieldAsInteger("point_dep");
              pax.point_arv = Qry.FieldAsInteger("point_arv");
              pax.airp_dep = Qry.FieldAsString("airp_dep");
              pax.airp_arv = Qry.FieldAsString("airp_arv");
              pax.cl = Qry.FieldAsString("class");
              pax.excess = Qry.FieldAsInteger("excess");
              pax.bag_refuse = Qry.FieldAsInteger("bag_refuse")!=0;
              pax.surname = Qry.FieldAsString("surname");
              pax.name = Qry.FieldAsString("name");
              pax.pers_type = Qry.FieldAsString("pers_type");
              pax.seat_no = Qry.FieldAsString("seat_no");
              pax.seats = Qry.FieldAsInteger("seats");
              if (iPax->present_in_req.find(apiDoc) !=  iPax->present_in_req.end())
              {
                //проверка всех реквизитов документа
                pax.doc=NormalizeDoc(iPax->doc);
                pax.present_in_req.insert(apiDoc);
              };
              if (iPax->present_in_req.find(apiDoco) !=  iPax->present_in_req.end())
              {
                //проверка всех реквизитов визы
                pax.doco=NormalizeDoco(iPax->doco);
                pax.present_in_req.insert(apiDoco);
              };

              s->second.paxForChng.push_back(pax);
            }
            else
            {
              TWebPaxForCkin pax;
              pax.crs_pax_id = iPax->crs_pax_id;
              pax.surname = Qry.FieldAsString("surname");
              pax.name = Qry.FieldAsString("name");
              pax.pers_type = Qry.FieldAsString("pers_type");
              pax.seat_no = Qry.FieldAsString("seat_no");
              pax.seat_type = Qry.FieldAsString("seat_type");
              pax.seats = Qry.FieldAsInteger("seats");
              pax.eticket = Qry.FieldAsString("eticket");
              pax.ticket = Qry.FieldAsString("ticket");
              //обработка документов
              if (isTestPaxId(iPax->crs_pax_id))
              {
                pax.apis.doc.clear();
                pax.apis.doc.no = Qry.FieldAsString("doc_no");
                pax.apis.doco.clear();
              }
              else
              {
                if (iPax->present_in_req.find(apiDoc) !=  iPax->present_in_req.end())
                {
                  //проверка всех реквизитов документа
                  pax.apis.doc=NormalizeDoc(iPax->doc);
                  pax.present_in_req.insert(apiDoc);
                }
                else
                  LoadCrsPaxDoc(pax.crs_pax_id, pax.apis.doc, true);

                if (iPax->present_in_req.find(apiDoco) !=  iPax->present_in_req.end())
                {
                  //проверка всех реквизитов визы
                  pax.apis.doco=NormalizeDoco(iPax->doco);
                  pax.present_in_req.insert(apiDoco);
                }
                else
                  LoadCrsPaxVisa(pax.crs_pax_id, pax.apis.doco);
              };

              pax.subclass = Qry.FieldAsString("subclass");

              TPerson p=DecodePerson(pax.pers_type.c_str());
              if (p==ASTRA::adult) adult_count++;
              if (p==ASTRA::baby && pax.seats==0) without_seat_count++;

              s->second.paxForCkin.push_back(pax);
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            throw CheckIn::UserException(e.getLexemaData(), point_id, iPax->crs_pax_id);
          };

        };
        if (without_seat_count>adult_count)
          throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
        if (pnr_id==NoExists)
          throw EXCEPTIONS::Exception("VerifyPax: unknown pnr_id (seg_no=%d)", seg_no);
        s->second.pnr_id=pnr_id;
      }
      else
      {
        //проверим лишь соответствие point_id и pnr_id
        VerifyPNRByPnrId(point_id,s->second.pnr_id);
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };

  //в любом случае дочитываем сквозной маршрут
  bool is_test = isTestPaxId(segs.begin()->second.pnr_id);

  CompletePnrData(is_test, segs.begin()->second.pnr_id, first);

  vector<WebSearch::TPnrData> PNRs;
  getTCkinData(first, is_test, PNRs);
  PNRs.insert(PNRs.begin(), first); //вставляем в начало первый сегмент

  const TWebPnrForSave &firstPnr=segs.begin()->second;
  //проверка всего сквозного маршрута на совпадение point_id, pnr_id и соответствие фамилий/имен
  vector<WebSearch::TPnrData>::const_iterator iPnrData=PNRs.begin();
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData!=PNRs.end() &&
          (!s->second.paxForCkin.empty() ||
           (!s->second.paxForChng.empty() && s->second.paxForChng.size()>s->second.refusalCountFromReq))) //типа есть пассажиры
      {
        //проверяем на сегменте вылет рейса и состояние соответствующего этапа
        if ( iPnrData->flt.act_out_local != NoExists )
          throw UserException( "MSG.FLIGHT.TAKEOFF" );

        if ( reqInfo->client_type == ctKiosk )
        {
          map<TStage_Type, TStage>::const_iterator iStatus=iPnrData->flt.stage_statuses.find(stKIOSKCheckIn);
          if (iStatus==iPnrData->flt.stage_statuses.end())
            throw EXCEPTIONS::Exception("VerifyPax: iPnrData->flt.stage_statuses[stKIOSKCheckIn] not defined (seg_no=%d)", seg_no);

          if (!(iStatus->second == sOpenKIOSKCheckIn ||
                (iStatus->second == sNoActive && s!=segs.begin()))) //для сквозных сегментов регистрация может быть еще не открыта
          {
            if (iStatus->second == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
          };
        }
        else
        {
          map<TStage_Type, TStage>::const_iterator iStatus=iPnrData->flt.stage_statuses.find(stWEBCheckIn);
          if (iStatus==iPnrData->flt.stage_statuses.end())
            throw EXCEPTIONS::Exception("VerifyPax: iPnrData->flt.stage_statuses[stWEBCheckIn] not defined (seg_no=%d)", seg_no);
          if (!(iStatus->second == sOpenWEBCheckIn ||
                (iStatus->second == sNoActive && s!=segs.begin()))) //для сквозных сегментов регистрация может быть еще не открыта
          {
            if (iStatus->second == sNoActive)
              throw UserException( "MSG.CHECKIN.NOT_OPEN" );
            else
              throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
          };
        };
      };

      if (iPnrData!=PNRs.end() && s->second.refusalCountFromReq>0)
      {
        if ( reqInfo->client_type != ctKiosk )
        {
          map<TStage_Type, TStage>::const_iterator iStatus=iPnrData->flt.stage_statuses.find(stWEBCancel);
          if (iStatus==iPnrData->flt.stage_statuses.end())
            throw EXCEPTIONS::Exception("VerifyPax: iPnrData->flt.stage_statuses[stWEBCancel] not defined (seg_no=%d)", seg_no);
          if (!(iStatus->second == sOpenWEBCheckIn ||
                (iStatus->second == sNoActive && s!=segs.begin()))) //для сквозных сегментов регистрация может быть еще не открыта
            throw UserException("MSG.PASSENGER.UNREGISTRATION_DENIAL");
        };
      };

      if (s==segs.begin()) continue; //пропускаем первый сегмент

      TWebPnrForSave &currPnr=s->second;
      if (iPnrData==PNRs.end()) //лишние сегменты в запросе на регистрацию
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->flt.point_dep!=s->first) //другой рейс на сквозном сегменте
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->seg.pnr_id!=currPnr.pnr_id) //другой pnr_id на сквозном сегменте
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");

      if (!currPnr.paxForCkin.empty() && firstPnr.paxForCkin.size()!=currPnr.paxForCkin.size())
        throw EXCEPTIONS::Exception("VerifyPax: different number of passengers for through check-in (seg_no=%d)", seg_no);

      if (!currPnr.paxForCkin.empty())
      {
        list<TWebPaxForCkin>::const_iterator iPax=firstPnr.paxForCkin.begin();
        for(;iPax!=firstPnr.paxForCkin.end();iPax++)
        {
          list<TWebPaxForCkin>::iterator iPax2=find(currPnr.paxForCkin.begin(),currPnr.paxForCkin.end(),*iPax);
          if (iPax2==currPnr.paxForCkin.end())
            throw EXCEPTIONS::Exception("VerifyPax: passenger not found (seg_no=%d, surname=%s, name=%s, pers_type=%s, seats=%d)",
                                        seg_no, iPax->surname.c_str(), iPax->name.c_str(), iPax->pers_type.c_str(), iPax->seats);

          list<TWebPaxForCkin>::iterator iPax3=iPax2;
          if (find(++iPax3,currPnr.paxForCkin.end(),*iPax)!=currPnr.paxForCkin.end())
            throw EXCEPTIONS::Exception("VerifyPax: passengers are duplicated (seg_no=%d, surname=%s, name=%s, pers_type=%s, seats=%d)",
                                        seg_no, iPax->surname.c_str(), iPax->name.c_str(), iPax->pers_type.c_str(), iPax->seats);

          currPnr.paxForCkin.splice(currPnr.paxForCkin.end(),currPnr.paxForCkin,iPax2,iPax3); //перемещаем найденного пассажира в конец
        };
      };
    }
    catch(CheckIn::UserException)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), s->first);
    };
  };

  if (!is_test)
    CreateEmulDocs(segs, PNRs, emulDocHeader, emulCkinDoc, emulChngDocs);

  //возвращаем ids
  for(iPnrData=PNRs.begin();iPnrData!=PNRs.end();iPnrData++)
  {
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=iPnrData->flt.point_dep;
    idsPnrData.airline=iPnrData->flt.oper.airline;
    idsPnrData.flt_no=iPnrData->flt.oper.flt_no;
    idsPnrData.suffix=iPnrData->flt.oper.suffix;
    idsPnrData.pnr_id=iPnrData->seg.pnr_id;
    idsPnrData.pr_paid_ckin=iPnrData->flt.pr_paid_ckin;
    ids.push_back( idsPnrData );
  };
};

void WebRequestsIface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;
    SavePax(reqNode, NULL, resNode);
};

bool WebRequestsIface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SavePax");
  vector< pair<int, TWebPnrForSave > > segs;
  xmlNodePtr segNode=NodeAsNode("segments", reqNode)->children;
  for(;segNode!=NULL;segNode=segNode->next)
  {
    TWebPnrForSave pnr;
    xmlNodePtr paxNode=GetNode("passengers", segNode);
    if (paxNode!=NULL) paxNode=paxNode->children;
    if (paxNode!=NULL)
    {
      for(;paxNode!=NULL;paxNode=paxNode->next)
      {
        xmlNodePtr node2=paxNode->children;
        TWebPaxFromReq pax;

        pax.crs_pax_id=NodeAsIntegerFast("crs_pax_id", node2);
        pax.seat_no=NodeAsStringFast("seat_no", node2, "");

        xmlNodePtr docNode = GetNode("document", paxNode);
        if (docNode!=NULL) {
            pax.present_in_req.insert(apiDoc);
            PaxDocFromXML(docNode, pax.doc);
        }

        xmlNodePtr docoNode = GetNode("doco", paxNode);
        if (docoNode!=NULL) {
            pax.present_in_req.insert(apiDoco);
            PaxDocoFromXML(docoNode, pax.doco);
        }

        xmlNodePtr fqtNode = GetNode("fqt_rems", paxNode);
        if (fqtNode!=NULL) {
          //если тег <fqt_rems> пришел, то изменяем и перезаписываем ремарки FQTV
          pax.fqtv_rems_present=true;
          //читаем пришедшие ремарки
          for(fqtNode=fqtNode->children; fqtNode!=NULL; fqtNode=fqtNode->next)
          {
            CheckIn::TPaxFQTItem fqt;
            fqt.rem="FQTV";
            TElemFmt fmt;
            fqt.airline = ElemToElemId( etAirline, NodeAsString("airline",fqtNode), fmt );
            if (fmt==efmtUnknown)
              fqt.airline=NodeAsString("airline",fqtNode);
            fqt.no=NodeAsString("no",fqtNode);
            pax.fqtv_rems.insert(fqt);
          };
        };

        pax.refuse=NodeAsIntegerFast("refuse", node2, 0)!=0;
        if (pax.refuse) pnr.refusalCountFromReq++;

        xmlNodePtr tidsNode=NodeAsNode("tids", paxNode);
        pax.crs_pnr_tid=NodeAsInteger("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsInteger("crs_pax_tid",tidsNode);
        xmlNodePtr node;
        node=GetNode("pax_grp_tid",tidsNode);
        if (node!=NULL && !NodeIsNULL(node)) pax.pax_grp_tid=NodeAsInteger(node);
        node=GetNode("pax_tid",tidsNode);
        if (node!=NULL && !NodeIsNULL(node)) pax.pax_tid=NodeAsInteger(node);

        pnr.paxFromReq.push_back(pax);
      };
    }
    else
      pnr.pnr_id=NodeAsInteger("pnr_id", segNode);

    segs.push_back(make_pair( NodeAsInteger("point_id", segNode), pnr ));
  };

  XMLDoc emulDocHeader;
  CreateEmulXMLDoc(reqNode, emulDocHeader);

  XMLDoc emulCkinDoc;
  map<int,XMLDoc> emulChngDocs;
  vector<TIdsPnrData> ids;
  VerifyPax(segs, emulDocHeader, emulCkinDoc, emulChngDocs, ids);

  TChangeStatusList ChangeStatusInfo;
  SirenaExchange::TLastExchangeList SirenaExchangeList;
  CheckIn::TAfterSaveInfoList AfterSaveInfoList;
  bool result=true;
  bool handleAfterSave=false;
  //важно, что сначала вызывается CheckInInterface::SavePax для emulCkinDoc
  //только при веб-регистрации НОВОЙ группы возможен ROLLBACK CHECKIN в SavePax при перегрузке
  //и соответственно возвращение result=false
  //и соответственно вызов ETStatusInterface::ETRollbackStatus для ВСЕХ ЭБ

  if (emulCkinDoc.docPtr()!=NULL) //регистрация новой группы
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (!CheckInInterface::SavePax(emulReqNode, ediResNode, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList)) result=false;
    handleAfterSave=true;
  };
  if (result)
  {
    for(map<int,XMLDoc>::iterator i=emulChngDocs.begin();i!=emulChngDocs.end();i++)
    {
      XMLDoc &emulChngDoc=i->second;
      xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulChngDoc.docPtr())->children;
      if (emulReqNode==NULL)
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
      if (!CheckInInterface::SavePax(emulReqNode, ediResNode, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList))
      {
        //по идее сюда мы никогда не должны попадать (см. комментарий выше)
        //для этого никогда не возвращаем false и делаем специальную защиту в SavePax:
        //при записи изменений веб и киосков не откатываемся при перегрузке
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CheckInInterface::SavePax=false");
      };
      handleAfterSave=true;
    };
  };

  if (result)
  {
    if (ediResNode==NULL && !ChangeStatusInfo.empty())
    {
      //хотя бы один билет будет обрабатываться
      OraSession.Rollback();  //откат
      ChangeStatusInterface::ChangeStatus(reqNode, ChangeStatusInfo);
      SirenaExchangeList.handle(__FUNCTION__);
      return false;
    }
    else
    {
      SirenaExchangeList.handle(__FUNCTION__);
    };

    if (handleAfterSave)
      AfterSaveInfoList.handle(__FUNCTION__); //если только изменение места пассажира, то не вызываем

    boost::optional<WebSearch::TPNRFilter> filter;
    vector< TWebPnr > pnrs;
    xmlNodePtr segsNode = NewTextChild( NewTextChild( resNode, "SavePax" ), "segments" );
    IntLoadPnr( ids, filter, pnrs, segsNode, true );  //!!! хорошо бы сюда тоже передавать filter на основе <search_params>
  };
  return result;
};

class BPTags {
    private:
        std::vector<std::string> tags;
        BPTags();
    public:
        void getFields( std::vector<std::string> &atags );
        static BPTags *Instance() {
            static BPTags *instance_ = 0;
            if ( !instance_ )
                instance_ = new BPTags();
            return instance_;
        }
};

BPTags::BPTags()
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select code from prn_tag_props where op_type = :op_type order by code";
    Qry.CreateVariable("op_type", otString, EncodeDevOperType(dotPrnBP));
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        if(TAG::GATE == Qry.FieldAsString("code")) continue;
        tags.push_back(lowerc(Qry.FieldAsString("code")));
    }
}

void BPTags::getFields( vector<string> &atags )
{
    atags.clear();
    atags = tags;
}

void GetBPPax(int point_dep, int pax_id, bool is_test, PrintInterface::BPPax &pax)
{
  pax.clear();
  int test_point_dep=NoExists;
  if (is_test)
  {
    if (point_dep==NoExists) throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    test_point_dep=point_dep;
  };
  if (!pax.fromDB(pax_id, test_point_dep))
  {
    if (pax.pax_id==NoExists)
      throw UserException( "MSG.PASSENGER.NOT_FOUND" );
    else
      throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  };
}

void GetBPPax(xmlNodePtr paxNode, bool is_test, bool check_tids, PrintInterface::BPPax &pax)
{
  pax.clear();
  if (paxNode==NULL) throw EXCEPTIONS::Exception("GetBPPax: paxNode==NULL");
  xmlNodePtr node2=paxNode->children;
  int point_dep = NodeIsNULLFast( "point_id", node2, true)?
        NoExists:
        NodeAsIntegerFast( "point_id", node2 );
  int pax_id = NodeAsIntegerFast( "pax_id", node2 );
  xmlNodePtr node = NodeAsNodeFast( "tids", node2 );
  node2=node->children;
  int crs_pnr_tid = NodeAsIntegerFast( "crs_pnr_tid", node2 );
  int crs_pax_tid = NodeAsIntegerFast( "crs_pax_tid", node2 );
  int pax_grp_tid = NodeIsNULLFast( "pax_grp_tid", node2, true )?
        NoExists:
        NodeAsIntegerFast( "pax_grp_tid", node2 );
  int pax_tid =     NodeIsNULLFast( "pax_tid", node2, true )?
        NoExists:
        NodeAsIntegerFast( "pax_tid", node2 ) ;
  if (check_tids) verifyPaxTids( pax_id, crs_pnr_tid, crs_pax_tid, pax_grp_tid, pax_tid );

  GetBPPax(is_test && point_dep==NoExists?pax_grp_tid:point_dep, pax_id, is_test, pax);
};

class BPReprintOptions
{
  public:
    static int check_date(int lower_shift, int upper_shift, int julian_date_of_flight, const string &airp);
    static void check_access(int julian_date_of_flight, const string &airp, const string &airline);
};

int BPReprintOptions::check_date(int lower_shift, int upper_shift, int julian_date_of_flight, const string &airp)
{
  JulianDate d(julian_date_of_flight, NowUTC(), JulianDate::TDirection::everywhere);
  d.trace(__FUNCTION__);

  TDateTime ret = NowUTC();
  try
  {
    ret=UTCToLocal(ret, AirpTZRegion(airp));
  }
  catch(EXCEPTIONS::Exception& e) {};

  modf(ret, &ret);
  int date_diff = (int)(ret - d.getDateTime());
  if(date_diff > 0 && date_diff > lower_shift)
    return -1;
  if(date_diff < 0 && -date_diff > upper_shift)
    return 1;
  return 0;
}

void BPReprintOptions::check_access(int julian_date_of_flight, const string &airp, const string &airline)
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);
  TElemFmt fmt;
  string airp_id = ElemToElemId(etAirp, airp, fmt);
  if (fmt == efmtUnknown) airp_id = airp;
  string airline_id = ElemToElemId(etAirline, airline, fmt);
  if (fmt == efmtUnknown) airline_id = airline;
  if(airp_id.empty() || airline_id.empty()) throw UserException("MSG.REPRINT_ACCESS_ERR");

  TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT lower_shift, upper_shift, "
    "       DECODE(airline,NULL,0,:airline,2,-100)+ "
    "       DECODE(airp,NULL,0,:airp,1,-100) AS priority "
    "FROM bcbp_reprint_options "
    "WHERE desk_grp=:desk_grp AND "
    "      (desk IS NULL OR desk=:desk) "
    "ORDER BY desk NULLS LAST, priority DESC ";
  Qry.CreateVariable("desk_grp", otInteger, reqInfo->desk.grp_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.CreateVariable("airline", otString, airline_id);
  Qry.CreateVariable("airp", otString, airp_id);
  Qry.Execute();

  if (!Qry.Eof)
  {
    if (Qry.FieldAsInteger("priority")<0) throw UserException("MSG.REPRINT_ACCESS_ERR");
    switch(check_date(Qry.FieldAsInteger("lower_shift"),
                      Qry.FieldAsInteger("upper_shift"),
                      julian_date_of_flight, airp_id))
    { case -1:throw UserException("MSG.REPRINT_WRONG_DATE_BEFORE");
      case  1:throw UserException("MSG.REPRINT_WRONG_DATE_AFTER");
    }
  }
}

void GetBPPaxFromScanCode(const string &scanCode, PrintInterface::BPPax &pax)
{
  pax.clear();

  WebSearch::TPNRFilters filters;
  BCBPSections scanSections;
  filters.getBCBPSections(scanCode, scanSections);
  try
  {
    //внимание!!
    //процедура заточена только на односегментный посадочный талон
    //многосегментные посадочные талоны пока не печатаются
    if (scanSections.repeated.size()!=1)
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");
    const BCBPRepeatedSections &repeated=*(scanSections.repeated.begin());
    //проверим что это посадочный талон
    if (repeated.checkinSeqNumber().first==NoExists) //это не посадочный талон, потому что рег. номер не известен
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");
    boost::optional<BCBPSectionsEnums::DocType> doc_type = scanSections.doc_type();
    if (doc_type && doc_type.get() == BCBPSectionsEnums::itenirary_receipt)
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");
    //проверим доступ для перепечати
    BPReprintOptions::check_access(repeated.dateOfFlight(),
                                   repeated.fromCityAirpCode(),
                                   repeated.operatingCarrierDesignator());
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    //не можем даже минимально проверить доступ
    LogTrace(TRACE5) << '\n' << scanSections;
    TReqInfo::Instance()->traceToMonitor(TRACE5, "GetBPPaxFromScanCode: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_DATA");
  }

  //попытка найти пассажира
  try
  {
    filters.fromBCBPSections(scanSections);

    list<AstraLocale::LexemaData> errors;
    list<WebSearch::TPNRs> PNRsList;

    GetPNRsList(filters, PNRsList, errors);
    if (!errors.empty())
      throw UserException(errors.begin()->lexema_id, errors.begin()->lparams);
    //проверим что это посадочный талон и что пассажир тестовый
    if (filters.segs.empty()) throw EXCEPTIONS::Exception("%s: filters.segs.empty()", __FUNCTION__);
    if (filters.segs.front().reg_no==NoExists) //это не посадочный талон, потому что рег. номер не известен
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");

    bool is_test=!filters.segs.front().test_paxs.empty();

    if (PNRsList.empty()) throw UserException( "MSG.PASSENGER.NOT_FOUND" );
    const WebSearch::TPNRs &PNRs=PNRsList.front();

    if (PNRs.pnrs.empty())
      throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
    if (!is_test && (PNRs.pnrs.size()>1 || PNRs.pnrs.begin()->second.paxs.size()>1))
      throw UserException( "MSG.PASSENGERS.FOUND_MORE" );
    if (PNRs.pnrs.begin()->second.segs.empty()) throw EXCEPTIONS::Exception("%s: PNRs.pnrs.begin()->second.segs.empty()", __FUNCTION__);
    int point_dep=PNRs.pnrs.begin()->second.segs.begin()->second.point_dep;
    if (PNRs.pnrs.begin()->second.paxs.empty()) throw EXCEPTIONS::Exception("%s: PNRs.pnrs.begin()->second.paxs.empty()", __FUNCTION__);
    int pax_id=PNRs.pnrs.begin()->second.paxs.begin()->pax_id;
    GetBPPax( point_dep, pax_id, is_test, pax );
  }
  catch(UserException &e)
  {
    LogTrace(TRACE5) << '\n' << scanSections;
    LogTrace(TRACE5) << ">>>> " << e.what();
  }
};

string GetBPGate(int point_id)
{
  string gate;
  TQuery Qry(&OraSession);
  Qry.Clear();
    Qry.SQLText =
    "SELECT stations.name FROM stations,trip_stations "
    " WHERE point_id=:point_id AND "
    "       stations.desk=trip_stations.desk AND "
    "       stations.work_mode=trip_stations.work_mode AND "
    "       stations.work_mode=:work_mode";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "work_mode", otString, "П" );
    Qry.Execute();
    if ( !Qry.Eof ) {
        gate = Qry.FieldAsString( "name" );
        Qry.Next();
        if ( !Qry.Eof )
      gate.clear();
    };
    return gate;
};

void WebRequestsIface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::ConfirmPrintBP");
  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  xmlNodePtr paxNode = NodeAsNode("passengers", reqNode)->children;
  for(;paxNode!=NULL;paxNode=paxNode->next)
  {
    int pax_id=NodeAsInteger("pax_id", paxNode);
    if (isTestPaxId(pax_id) || isEmptyPaxId(pax_id)) continue;
    PrintInterface::BPPax pax;
    try
    {
      GetBPPax( paxNode, false, false, pax );
      pax.time_print=NodeAsDateTime("prn_form_key", paxNode);
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      //не надо прокидывать ue в терминал - подтверждаем все что можем!
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };

  PrintInterface::ConfirmPrintBP(dotPrnBP, paxs, ue);  //не надо прокидывать ue в терминал - подтверждаем все что можем!

  NewTextChild( resNode, "ConfirmPrintBP" );
};

void WebRequestsIface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::GetPrintDataBP");
  PrintInterface::BPParams params;
  params.dev_model = NodeAsString("dev_model", reqNode);
  params.fmt_type = NodeAsString("fmt_type", reqNode);
  params.prnParams.get_prn_params(reqNode);
  params.clientDataNode = NULL;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
      "SELECT bp_type, "
      "       DECODE(desk_grp_id,NULL,0,2)+ "
      "       DECODE(desk,NULL,0,4) AS priority "
      "FROM desk_bp_set "
      "WHERE op_type=:op_type AND "
      "      (desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
      "      (desk IS NULL OR desk=:desk) "
      "ORDER BY priority DESC ";
  Qry.CreateVariable("op_type", otString, EncodeDevOperType(dotPrnBP));
  Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.Execute();
  if(Qry.Eof) throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_DESK");
  params.form_type = Qry.FieldAsString("bp_type");
  ProgTrace(TRACE5, "bp_type: %s", params.form_type.c_str());

  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  map<int/*point_dep*/, string/*gate*/> gates;

  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);
  if (scanCodeNode!=NULL)
  {
    string scanCode=NodeAsString(scanCodeNode);
    PrintInterface::BPPax pax;
    try
    {
      GetBPPaxFromScanCode(scanCode, pax);
      if (pax.point_dep!=NoExists)
      {
        if (gates.find(pax.point_dep)==gates.end()) gates[pax.point_dep]=GetBPGate(pax.point_dep);
        pax.gate=make_pair(gates[pax.point_dep], true);
      };
      if (pax.pax_id==NoExists) pax.scan=scanCode;
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  }
  else
  {
    bool is_test=isTestPaxId(NodeAsInteger("passengers/pax/pax_id", reqNode));
    xmlNodePtr paxNode = NodeAsNode("passengers", reqNode)->children;
    for(;paxNode!=NULL;paxNode=paxNode->next)
    {
      PrintInterface::BPPax pax;
      try
      {
        GetBPPax( paxNode, is_test, true, pax );
        if (gates.find(pax.point_dep)==gates.end()) gates[pax.point_dep]=GetBPGate(pax.point_dep);
        pax.gate=make_pair(gates[pax.point_dep], true);
        paxs.push_back(pax);
      }
      catch(UserException &e)
      {
        ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
      };
    };
  };

  if (!ue.empty()) throw ue;

  string pectab, data;
  BIPrintRules::Holder bi_rules;
  PrintInterface::GetPrintDataBP(dotPrnBP, params, data, pectab, bi_rules, paxs);

  xmlNodePtr BPNode = NewTextChild( resNode, "GetPrintDataBP" );
  NewTextChild(BPNode, "pectab", pectab);
  xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
  for (vector<PrintInterface::BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
  {
    xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id", iPax->pax_id==NoExists?getEmptyPaxId():iPax->pax_id);
    if (!iPax->hex && params.prnParams.encoding!="UTF-8")
    {
      iPax->prn_form = ConvertCodepage(iPax->prn_form, "CP866", params.prnParams.encoding);
      StringToHex( string(iPax->prn_form), iPax->prn_form );
      iPax->hex=true;
    };
    SetProp(NewTextChild(paxNode, "prn_form", iPax->prn_form),"hex",(int)iPax->hex);
    NewTextChild(paxNode, "prn_form_key", DateTimeToStr(iPax->time_print));
  };
};

#include <fstream>

int bcbp_test(int argc,char **argv)
{
    // Без инициализации reqInfo пас не найдется
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");

    vector<string> tags;
    BPTags::Instance()->getFields( tags );
    string pectab;
    for(vector<string>::iterator iv = tags.begin(); iv != tags.end(); iv++) {
        int pad = 20 - iv->size();
        if(pad < 0) pad = 0;
        pectab += *iv + string(pad, ' ');
        string fp = "(40,,)";

        if(
                upperc(*iv) == TAG::BCBP_M_2 or
                upperc(*iv) == TAG::BCBP_V_5
          ) fp.clear();

        if(
                upperc(*iv) == TAG::SCD or
                upperc(*iv) == TAG::TIME_PRINT
          ) {
            fp = "(,,dd.mm)";
            pectab += "'[<" + *iv + fp + ">]'\n";
            fp = "(,,hh:nn)";
            pectab += *iv + string(pad, ' ');
            pectab += "'[<" + *iv + fp + ">]'\n";
        } else
            pectab += "'[<" + *iv + fp + ">]'\n";
    }

    cout << pectab << endl;

    ifstream ifs("bcbp");
    std::string scan((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    //string scan = "M1ZAKHAROV/DENIS YUREV        DMEAER UT0001 264Y005A0001 128>2180OO    B                000028787007";

    PrintInterface::BPPax pax;
    GetBPPaxFromScanCode(scan, pax);

    boost::shared_ptr<PrintDataParser> parser;
    if(pax.pax_id == NoExists) {
        cout << "pax not found." << endl;
        parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(scan));
    } else {
        cout << "pax found, pax_id: " << pax.pax_id << endl;
        parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(pax.grp_id, pax.pax_id, 0, NULL));
    }
    cout << endl;

    cout << parser->parse(pectab) << endl;

    return 1;
}

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  ProgTrace(TRACE1,"WebRequestsIface::GetBPTags");

  PrintInterface::BPPax pax;
  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);
  boost::shared_ptr<PrintDataParser> parser;
  if (scanCodeNode!=NULL)
  {
    string scanCode=NodeAsString(scanCodeNode);
    GetBPPaxFromScanCode(scanCode, pax);

    if(pax.pax_id == NoExists)
      parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(scanCode));
    else
      parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(pax.grp_id, pax.pax_id, 0, NULL));
  }
  else
  {
    bool is_test=isTestPaxId(NodeAsInteger("pax_id", reqNode));
    GetBPPax( reqNode, is_test, true, pax );
    parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(pax.grp_id, pax.pax_id, 0, NULL));
  };
  vector<string> tags;
  BPTags::Instance()->getFields( tags );
  xmlNodePtr node = NewTextChild( resNode, "GetBPTags" );
  for ( vector<string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
    for(int j = 0; j < 2; j++) {
      string value = parser->pts.get_tag(*i, ServerFormatDateTimeAsString, ((j == 0 and not scanCodeNode) ? "R" : "E"));
      NewTextChild( node, (*i + (j == 0 ? "" : "_lat")).c_str(), value );
      ProgTrace( TRACE5, "field name=%s, value=%s", (*i + (j == 0 ? "" : "_lat")).c_str(), value.c_str() );
    }
  }
  parser->pts.confirm_print(true, dotPrnBP);

  string gate=GetBPGate(pax.point_dep);
  if (!gate.empty())
    NewTextChild( node, "gate", gate );
}

void ChangeProtPaidLayer(xmlNodePtr reqNode, xmlNodePtr resNode,
                         bool pr_del, int time_limit,
                         int &curr_tid, CheckIn::UserException &ue)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  int point_id=NoExists;
  bool error_exists=false;
  try
  {
    if (NodeAsNode("passengers", reqNode)->children==NULL) return;

    if (!pr_del)
    {
      point_id=NodeAsInteger("point_id", reqNode);
      //проверим признак платной регистрации и
      Qry.Clear();
      Qry.SQLText =
          "SELECT pr_permit, prot_timeout FROM trip_paid_ckin WHERE point_id=:point_id";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.Execute();
        if ( Qry.Eof || Qry.FieldAsInteger("pr_permit")==0 )
          throw UserException( "MSG.CHECKIN.NOT_PAID_CHECKIN_MODE" );

        if (time_limit==NoExists)
        {
          //получим prot_timeout
        if (!Qry.FieldIsNULL("prot_timeout"))
          time_limit=Qry.FieldAsInteger("prot_timeout");
        };
        if (time_limit==NoExists)
          throw UserException( "MSG.PROT_TIMEOUT_NOT_DEFINED" );
    };

    Qry.Clear();
    Qry.DeclareVariable("crs_pax_id", otInteger);
    const char* PaxQrySQL=
      "SELECT crs_pnr.pnr_id, crs_pnr.status AS pnr_status, "
      "       crs_pnr.point_id, crs_pnr.airp_arv, "
      "       crs_pnr.subclass, crs_pnr.class, "
      "       crs_pax.seats, "
      "       crs_pnr.tid AS crs_pnr_tid, "
      "       crs_pax.tid AS crs_pax_tid, "
      "       DECODE(pax.pax_id,NULL,0,1) AS checked "
      "FROM crs_pnr, crs_pax, pax "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=:crs_pax_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pax.pr_del=0";
    const char* TestPaxQrySQL=
      "SELECT id AS pnr_id, NULL AS pnr_status, "
      "       subclass, subcls.class, "
      "       1 AS seats, "
      "       id AS crs_pnr_tid, "
      "       id AS crs_pax_tid, "
      "       0 AS checked "
      "FROM test_pax, subcls "
      "WHERE test_pax.subclass=subcls.code AND test_pax.id=:crs_pax_id";

    int pnr_id=NoExists;
    vector<TWebPax> pnr;
    xmlNodePtr node=NodeAsNode("passengers", reqNode)->children;
    for(;node!=NULL;node=node->next)
    {
      xmlNodePtr node2=node->children;
      TWebPax pax;

      pax.crs_pax_id=NodeAsIntegerFast("crs_pax_id", node2);
      try
      {
        if (!pr_del)
        {
          pax.crs_seat_no=NodeAsStringFast("seat_no", node2, ""); //для РМ без мест тег может не передаваться - это заглушка из-за баги веб-клиента

          //проверим на дублирование
          for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
          {
            if (iPax->crs_pax_id==pax.crs_pax_id)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: crs_pax_id duplicated (crs_pax_id=%d)",
                                          pax.crs_pax_id);
            if (!pax.crs_seat_no.empty() && iPax->crs_seat_no==pax.crs_seat_no)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: seat_no duplicated (crs_pax_id=%d, seat_no=%s)",
                                          pax.crs_pax_id, pax.crs_seat_no.c_str());
          };
        };

        xmlNodePtr tidsNode=NodeAsNodeFast("tids", node2);
        pax.crs_pnr_tid=NodeAsInteger("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsInteger("crs_pax_tid",tidsNode);

        //проверим tids пассажира
        if (!isTestPaxId(pax.crs_pax_id))
          Qry.SQLText=PaxQrySQL;
        else
          Qry.SQLText=TestPaxQrySQL;
        Qry.SetVariable("crs_pax_id",pax.crs_pax_id);
        Qry.Execute();
        if (Qry.Eof)
          throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
        if (Qry.FieldAsInteger("checked")!=0)
          throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
        if (pax.crs_pnr_tid!=Qry.FieldAsInteger("crs_pnr_tid") ||
            pax.crs_pax_tid!=Qry.FieldAsInteger("crs_pax_tid"))
          throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

        if (!pr_del)
        {
          if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
              !is_valid_pax_status(point_id, pax.crs_pax_id))
            throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

          if (pnr_id==NoExists)
          {
            pnr_id=Qry.FieldAsInteger("pnr_id");
          }
          else
          {
            if (pnr_id!=Qry.FieldAsInteger("pnr_id"))
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: passengers from different PNR (crs_pax_id=%d)", pax.crs_pax_id);
          };
        };
        pax.pass_class=Qry.FieldAsString("class");
        pax.pass_subclass=Qry.FieldAsString("subclass");
        pax.seats=Qry.FieldAsInteger("seats");
        if (!pr_del && pax.crs_seat_no.empty())
        {
          if (pax.seats>0)
            throw EXCEPTIONS::Exception("ChangeProtPaidLayer: empty seat_no (crs_pax_id=%d)", pax.crs_pax_id);
          continue;
        };
        pnr.push_back(pax);
      }
      catch(UserException &e)
      {
        ue.addError(e.getLexemaData(), point_id, pax.crs_pax_id);
        error_exists=true;
      };
    };


    vector< pair<TWebPlace, LexemaData> > pax_seats;
    if (!pnr.empty())
    {
      TPointIdsForCheck point_ids_spp;
      if (!pr_del)
      {
        TQuery LayerQry(&OraSession);
        LayerQry.Clear();
        LayerQry.SQLText=
          "UPDATE tlg_comp_layers "
          "SET time_remove=SYSTEM.UTCSYSDATE+:timeout/1440 "
          "WHERE crs_pax_id=:crs_pax_id AND layer_type=:layer_type ";
        LayerQry.DeclareVariable("layer_type", otString);
        LayerQry.DeclareVariable("crs_pax_id", otInteger);
        if (time_limit!=NoExists)
          LayerQry.CreateVariable("timeout", otInteger, time_limit);
        else
          LayerQry.CreateVariable("timeout", otInteger, FNull);
        VerifyPNRByPnrId(point_id, pnr_id);
        GetCrsPaxSeats(point_id, pnr, pax_seats );
        //bool UsePriorContext=false;
        bool isTranzitSalonsVersion = isTranzitSalons( point_id );
        BitSet<TChangeLayerFlags> change_layer_flags;
        change_layer_flags.setFlag(flSetPayLayer);
        for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
        {
          try
          {
            vector< pair<TWebPlace, LexemaData> >::const_iterator iSeat=pax_seats.begin();
            for(;iSeat!=pax_seats.end();iSeat++)
              if (iSeat->first.pax_id==iPax->crs_pax_id &&
                  iSeat->first.seat_no==iPax->crs_seat_no) break;
            if (iSeat==pax_seats.end())
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: passenger not found in pax_seats (crs_pax_id=%d, crs_seat_no=%s)",
                                          iPax->crs_pax_id, iPax->crs_seat_no.c_str());
            if (!iSeat->second.lexema_id.empty())
              throw UserException(iSeat->second.lexema_id, iSeat->second.lparams);

/*            if ( iSeat->first.SeatTariff.empty() )  //нет тарифа
              throw UserException("MSG.SEATS.NOT_SET_RATE");*/

            if (isTestPaxId(iPax->crs_pax_id)) continue;

            //проверки + запись!!!
            int tid = iPax->crs_pax_tid;
            if ( isTranzitSalonsVersion ) {
              IntChangeSeatsN( point_id,
                               iPax->crs_pax_id,
                               tid,
                               iSeat->first.xname,
                               iSeat->first.yname,
                               stSeat,
                               cltProtBeforePay,
                               change_layer_flags,
                               0,
                               NoExists,
                               NULL );
            }
            else {
              IntChangeSeats( point_id,
                              iPax->crs_pax_id,
                              tid,
                              iSeat->first.xname,
                              iSeat->first.yname,
                              stSeat,
                              cltProtBeforePay,
                              change_layer_flags,
                              NULL );
            }
            //в любом случае устанавливаем tlg_comp_layers.time_remove
            LayerQry.SetVariable("layer_type", EncodeCompLayerType(cltProtBeforePay));
            LayerQry.SetVariable("crs_pax_id", iPax->crs_pax_id);
            LayerQry.Execute();

          }
          catch(UserException &e)
          {
            ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
            error_exists=true;
          };
        };
      }
      else
      {
        //RemoveProtPaidLayer
        for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
        {
          try
          {
            if (isTestPaxId(iPax->crs_pax_id)) continue;
            DeleteTlgSeatRanges(cltProtBeforePay, iPax->crs_pax_id, curr_tid, point_ids_spp);
          }
          catch(UserException &e)
          {
            ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
            error_exists=true;
          };
        };
        check_layer_change(point_ids_spp);
      };
    }; //!pnr.empty()
    if (error_exists) return; //если есть ошибки, выйти из обработки сегмента

    //формирование ответа
    NewTextChild(resNode,"point_id",point_id,NoExists);
    NewTextChild(resNode,"time_limit",time_limit,NoExists);

    xmlNodePtr paxsNode=NewTextChild(resNode, "passengers");
    for(vector<TWebPax>::iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
    {
      try
      {
        if (!isTestPaxId(iPax->crs_pax_id))
          Qry.SQLText=PaxQrySQL;
        else
          Qry.SQLText=TestPaxQrySQL;
        Qry.SetVariable("crs_pax_id",iPax->crs_pax_id);
        Qry.Execute();
        if (Qry.Eof)
          throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
        iPax->crs_pnr_tid=Qry.FieldAsInteger("crs_pnr_tid");
        iPax->crs_pax_tid=Qry.FieldAsInteger("crs_pax_tid");

        xmlNodePtr paxNode=NewTextChild(paxsNode, "pax");
        NewTextChild(paxNode, "crs_pax_id", iPax->crs_pax_id);
        xmlNodePtr tidsNode=NewTextChild(paxNode, "tids");
        NewTextChild(tidsNode, "crs_pnr_tid", iPax->crs_pnr_tid);
        NewTextChild(tidsNode, "crs_pax_tid", iPax->crs_pax_tid);
        if (!pr_del)
        {
          vector< pair<TWebPlace, LexemaData> >::const_iterator iSeat=pax_seats.begin();
          for(;iSeat!=pax_seats.end();iSeat++)
            if (iSeat->first.pax_id==iPax->crs_pax_id &&
                iSeat->first.seat_no==iPax->crs_seat_no) break;
          if (iSeat!=pax_seats.end())
          {
            if ( !iSeat->first.SeatTariff.empty() ) { // если платная регистрация отключена, value=0.0 в любом случае
                xmlNodePtr rateNode = NewTextChild( paxNode, "rate" );
                NewTextChild( rateNode, "color", iSeat->first.SeatTariff.color );
                NewTextChild( rateNode, "value", iSeat->first.SeatTariff.rateView() );
                NewTextChild( rateNode, "currency", iSeat->first.SeatTariff.currencyView(reqInfo->desk.lang) );
            };
          };
        };
      }
      catch(UserException &e)
      {
        ue.addError(e.getLexemaData(), point_id, iPax->crs_pax_id);
      };
    };
  }
  catch(UserException &e)
  {
    ue.addError(e.getLexemaData(), point_id);
  };
};

void WebRequestsIface::AddProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  resNode=NewTextChild(resNode,"AddProtPaidLayer");
  int time_limit=NoExists;
  int curr_tid=NoExists;
  CheckIn::UserException ue;
  xmlNodePtr node=GetNode("time_limit",reqNode);
  if (node!=NULL && !NodeIsNULL(node))
  {
    time_limit=NodeAsInteger(node);
    if (time_limit<=0 || time_limit>999)
      throw EXCEPTIONS::Exception("AddProtPaidLayer: wrong time_limit %d min", time_limit);
  };

  //здесь лочка рейсов в порядке сортировки point_id
  vector<int> point_ids;
  node=NodeAsNode("segments", reqNode)->children;
  for(;node!=NULL;node=node->next)
    point_ids.push_back(NodeAsInteger("point_id", node));
  sort(point_ids.begin(),point_ids.end());
  //lock flights
  TFlights flights;
  flights.Get( point_ids, ftTranzit );
  flights.Lock();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id, pr_del, pr_reg "
    "FROM points "
    "WHERE point_id=:point_id";// FOR UPDATE";
  Qry.DeclareVariable("point_id", otInteger);
  for(vector<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); i++)
  {
    try
    {
      Qry.SetVariable("point_id", *i);
      Qry.Execute();
      if ( Qry.Eof )
            throw UserException( "MSG.FLIGHT.NOT_FOUND" );
        if ( Qry.FieldAsInteger( "pr_del" ) < 0 )
            throw UserException( "MSG.FLIGHT.DELETED" );
        if ( Qry.FieldAsInteger( "pr_del" ) > 0 )
            throw UserException( "MSG.FLIGHT.CANCELED" );
        if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
            throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
    }
    catch(UserException &e)
    {
      ue.addError(e.getLexemaData(), *i);
    };
  };
  if (!ue.empty()) throw ue;

  node=NodeAsNode("segments", reqNode)->children;
  xmlNodePtr segsNode=NewTextChild(resNode, "segments");
  for(;node!=NULL;node=node->next)
  {
    xmlNodePtr segNode=NewTextChild(segsNode, "segment");
    ChangeProtPaidLayer(node, segNode, false, time_limit, curr_tid, ue );
  };
  if (!ue.empty()) throw ue;
};

void WebRequestsIface::RemoveProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  resNode=NewTextChild(resNode,"RemoveProtPaidLayer");
  int curr_tid=NoExists;
  CheckIn::UserException ue;
  ChangeProtPaidLayer(reqNode, resNode, true, NoExists, curr_tid, ue);
  if (!ue.empty()) throw ue;
};

void WebRequestsIface::ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SysReqInterface::ErrorToLog(ctxt, reqNode, resNode);
  NewTextChild(resNode, "ClientError");
};

void WebRequestsIface::PaymentStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  const char* pax_sql=
    "SELECT pax_grp.point_dep, pax_grp.grp_id, "
    "       pax.surname, pax.name, pax.pers_type, pax.reg_no "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";

  const char* crs_pax_sql=
    "SELECT tlg_binding.point_id_spp AS point_dep, NULL AS grp_id, "
    "       crs_pax.surname, crs_pax.name, crs_pax.pers_type, NULL AS reg_no "
    "FROM crs_pax, crs_pnr, tlg_binding "
    "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pax.pax_id=:pax_id";

  TQuery Qry(&OraSession);
  Qry.DeclareVariable("pax_id", otInteger);

  TLogLocale msg;
  msg.ev_type=ASTRA::evtPax;
  msg.lexema_id = "EVT.PASSENGER_DATA";

  xmlNodePtr paxNode=GetNode("passengers", reqNode);
  if (paxNode!=NULL) paxNode=paxNode->children;
  for(; paxNode!=NULL; paxNode=paxNode->next)
  {
    xmlNodePtr node2=paxNode->children;
    int crs_pax_id=NodeAsIntegerFast("crs_pax_id", node2);
    string status=NodeAsStringFast("status", node2);
    if (!(status=="PAID" ||
          status=="NOT_PAID"))
      throw EXCEPTIONS::Exception("%s: wrong payment status '%s'", __FUNCTION__, status.c_str());

    Qry.SetVariable("pax_id", crs_pax_id);
    for(int pass=0; pass<2; pass++)
    {
      Qry.SQLText=(pass==0?pax_sql:crs_pax_sql);
      Qry.Execute();
      if (Qry.Eof) continue;
      msg.id1=Qry.FieldAsInteger("point_dep");
      msg.id2=Qry.FieldIsNULL("reg_no")?NoExists:
                                        Qry.FieldAsInteger("reg_no");
      msg.id3=Qry.FieldIsNULL("grp_id")?NoExists:
                                        Qry.FieldAsInteger("grp_id");
      string full_name=Qry.FieldAsString("surname");
      if (!Qry.FieldIsNULL("name"))
      {
        full_name+=" ";
        full_name+=Qry.FieldAsString("name");
      };
      string pers_type=Qry.FieldAsString("pers_type");

      msg.prms.clearPrms();
      msg.prms << PrmSmpl<string>("pax_name", full_name)
               << PrmElem<string>("pers_type", etPersType, pers_type)
               << PrmLexema("param", "EVT.PAYMENT_SYSTEM_STATUS_" + status);

      TReqInfo::Instance()->LocaleToLog(msg);
      break;
    };
  }
  NewTextChild( resNode, "PaymentStatus" );
};

void RevertWebResDoc()
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
  const char* answer_tag = (const char*)xmlRC->reqDoc->children->children->children->name;
  std::string error_code, error_message;
  xmlNodePtr errNode = selectPriorityMessage(resNode, error_code, error_message);

  if (errNode!=NULL)
  {
    resNode=NewTextChild( resNode, answer_tag );

    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"checkin_user_error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      NewTextChild( resNode, "error_code", error_code );
      NewTextChild( resNode, "error_message", error_message );
    };

    if (strcmp((const char*)errNode->name,"checkin_user_error")==0)
    {
      xmlNodePtr segsNode=NodeAsNode("segments",errNode);
      if (segsNode!=NULL)
      {
        xmlUnlinkNode(segsNode);
        xmlAddChild( resNode, segsNode);
      };
    };
    xmlFreeNode(errNode);
  };
}

bool TWebPax::suitable(const WebSearch::TPNRFilter &filter) const
{
  return filter.isEqualSurname(surname) &&
         filter.isEqualName(name) &&
         filter.isEqualTkn(tkn.no) &&
         filter.isEqualDoc(doc.no) &&
         filter.isEqualRegNo(reg_no);
}

void WebRequestsIface::GetCacheTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr n = GetNode( "table_name", reqNode );
  if ( n == NULL ) {
    throw EXCEPTIONS::Exception( "tag 'table_name' not found'" );
  }
  string table_name = NodeAsString( n );
  table_name = lowerc( table_name );
  if ( table_name != "rcpt_doc_types" &&
       table_name != "pax_doc_countries" &&
       table_name != "pax_doc_countries_ext") {
    throw EXCEPTIONS::Exception( "invalid table_name %s", table_name.c_str() );
  }
  if ( TReqInfo::Instance()->client_type == ctKiosk && table_name == "rcpt_doc_types" ) {
    table_name = "pax_doc_countries"; //old bug fix
  }
  n = GetNode( "tid", reqNode );
  int tid;
  if ( n ) {
    tid = NodeAsInteger( n );
  }
  else {
    tid = ASTRA::NoExists;
  }
  ProgTrace( TRACE5, "WebRequestsIface::GetCacheTable: table_name=%s, tid=%d", table_name.c_str(), tid );
  n = NewTextChild( resNode, "GetCacheTable" );
  NewTextChild( n, "table_name", table_name );
  TQuery Qry(&OraSession);
  if ( tid != ASTRA::NoExists ) {
    Qry.SQLText =
      string("SELECT tid FROM ")  + table_name + " WHERE tid>:tid AND rownum<2";
    Qry.CreateVariable( "tid", otInteger, tid );
    Qry.Execute();
    if ( Qry.Eof ) {
      NewTextChild( n, "tid", tid );
      return;
    }
    tid = ASTRA::NoExists;
  }
  Qry.Clear();
  string sql;
  if ( table_name == "pax_doc_countries_ext" ) {
    sql =
      "SELECT p.code,p.code code_lat,p.name,p.name_lat,p.country,c.code_lat country_lat,p.pr_del,p.tid FROM pax_doc_countries p, countries c "
      "WHERE p.country=c.code(+) AND p.pr_del=0 "
      "ORDER BY code";
  }
  else {
    sql =
      "SELECT id,code,code code_lat,name,name_lat,pr_del,tid FROM " + table_name + " WHERE pr_del=0 ORDER BY code";
  }
  Qry.SQLText = sql;
  Qry.Execute();
  xmlNodePtr node = NewTextChild( n, "data" );
  for ( ; !Qry.Eof; Qry.Next() ) {
    xmlNodePtr rowNode = NewTextChild( node, "row" );
    NewTextChild( rowNode, "code", Qry.FieldAsString( "code" ) );
    NewTextChild( rowNode, "code_lat", Qry.FieldAsString( "code_lat" ) );
    NewTextChild( rowNode, "name", Qry.FieldAsString( "name" ) );
    NewTextChild( rowNode, "name_lat", Qry.FieldAsString( "name_lat" ) );
    if ( table_name == "pax_doc_countries_ext" ) {
      NewTextChild( rowNode, "country", Qry.FieldAsString( "country" ) );
      NewTextChild( rowNode, "country_lat", Qry.FieldAsString( "country_lat" ) );
    }
    if ( tid < Qry.FieldAsInteger( "tid" ) ) {
      tid = Qry.FieldAsInteger( "tid" );
    }
  }
  NewTextChild( n, "tid", tid );
}

void WebRequestsIface::ParseMessage(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "@type", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag '@type' not found" );
  string stype = NodeAsString( node );
  string body = NodeAsString( reqNode );
  ProgTrace( TRACE5, "ParseMessage: stype=%s, body=|%s|", stype.c_str(), body.c_str() );
  //разборка телеграммы Дена ssm
  TQuery Qry(&OraSession);
  Qry.SQLText = "insert into ssm_in(id, type, data) values(ssm_in__seq.nextval, :type, :data)";
  Qry.CreateVariable("data", otString, body);
  Qry.CreateVariable("type", otString, stype);
  Qry.Execute();
}

/////////////////// MERIDIAN //////////////////////////////////
void WebRequestsIface::GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  MERIDIAN::GetFlightInfo( ctxt, reqNode, resNode );
};

void WebRequestsIface::GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  MERIDIAN::GetPaxsInfo( ctxt, reqNode, resNode );
};
//////////////////END MERIDIAN //////////////////////////////////

} //end namespace AstraWeb

int nosir_parse_bcbp(int argc, char **argv)
{
    string scanCode = "M1PLESKACH/EKATERINA  E19LC26 SGCVKOUT 0296 133L          3D>10B0      IUT 2C2982986148311439 0UT                        ";
    PrintInterface::BPPax pax;
    AstraWeb::GetBPPaxFromScanCode(scanCode, pax);
    /*
    WebSearch::TPNRFilters filters;
    BCBPSections scanSections;
    filters.getBCBPSections("M1PLESKACH/EKATERINA  E19LC26 SGCVKOUT 0296 133L          3D>10B0      IUT 2C2982986148311439 0UT                        ", scanSections);
    */
    return 1;
}

#if 0

bool test_check_reprint_access()
{   std::cout<<"Started Test check_reprint_access()...\n";
     struct Record
            { int grp;
          string desk, airp, airline;
              int lower_shift, upper_shift;
              string str()
              { return string(" with airp: ") + airp + " airline: " +  airline  + " days before: " +
                     std::to_string(lower_shift) + " days after: " +  std::to_string(upper_shift)  + " kiosk: " + desk + " kiosk group: " + std::to_string(grp)  + "\n";
              }
            };
    struct TestRecord
    {

              int grp;
              string desk, airp, airline;
              int _time;
              string err;
              /*string str()
              { return string(" with airp: ") + airp + " airline: " +  airline  + " days before: " +
                     std::to_string(lower_shift) + " days after: " +  std::to_string(upper_shift)  + " kiosk: " + desk + " kiosk group: " + std::to_string(grp)  + "\n";
              }*/

              TDateTime time(){
                TElemFmt fmt;
                string airp_id = ElemToElemId(etAirp, airp, fmt); ;
                TDateTime t ;
                try{
                   t = UTCToLocal(NowUTC(), AirpTZRegion(airp_id, false)) + _time;
                }
                catch(...)
                {  t = NowUTC() + _time;
                }
        modf(t, &t) ;
                return t;
              }

   };
    class Void
    {
        public:
            static int get_last_good(vector<string>& x)
            {int ret = 0; for(unsigned int i = 0; i<x.size(); i++, ret++)
                if(x[i].empty()) return i; return ret;
            }
           vector<Record> table;
           void set_bd(vector<Record>& new_table)
           {    std::cout<<"clear() start\n";
                std::cout.flush();

                clear();
                std::cout<<"clear() end\n";
                std::cout.flush();
                table = new_table;
                TElemFmt fmt;
                TQuery Qry(&OraSession);
        for(auto &i : table)
                {  Qry.SQLText =
              " INSERT INTO bcbp_reprint_options (id, desk_grp, desk, airline, airp, upper_shift, lower_shift) "
           " VALUES(id__seq.nextval, :desk_grp, :desk, :airline, :airp, :upper_shift, :lower_shift) " ;
            Qry.CreateVariable("desk_grp", otInteger, i.grp);
            Qry.CreateVariable("desk", otString, i.desk);
            Qry.CreateVariable("airline", otString, ElemToElemId(etAirline, i.airline, fmt));
            Qry.CreateVariable("airp", otString, ElemToElemId(etAirp, i.airp, fmt));
                    Qry.CreateVariable("upper_shift", otInteger, i.upper_shift);
                    Qry.CreateVariable("lower_shift", otInteger, i.lower_shift);
            Qry.Execute();
                    Qry.Clear();
              }
              std::cout<<"set bd end\n";
              std::cout.flush();
           }

           void clear()
           {
               std::cout<<"start session()\n";
               std::cout.flush();

           TQuery Qry(&OraSession);
               std::cout<<"set text\n";
               std::cout.flush();

               Qry.SQLText =
                   "DELETE FROM  bcbp_reprint_options  ";
                    Qry.Execute();
            std::cout<<"execute\n";
               std::cout.flush();

                    Qry.Clear();
        std::cout<<"Qry.Clear()\n";
              std::cout.flush();

       }
          // ~Void(){clear(); OraSession.Rollback();};

           int grp_id;
           string desk_code;

//	   Void(string _code, int _id) : grp_id(_id), desk_code(_code) {}
    };
    std::cout<<"Init...\n"; std::cout.flush();
    Void query;
   // TReqInfo::Instance()->desk.grp_id=query.grp_id;
    //TReqInfo::Instance()->desk.code=query.desk_code;
    vector<string> airps = {"VKO", "DME", "", " ", "0000", "000"}; //последнее в принципе допустимое имя аэропорта/авиакомпании должно быть написано до ""
    vector<string>airlines = {"AU", "", " ", "0000", "000"};
    vector<int> grps = {1};
    vector<string> desks = {"KIOSKB"};
    vector<TDateTime> times;
//    int final_good_airps = Void::get_last_good(airps);
//    int final_good_airlines = Void::get_last_good(airlines);
//    int final_good_grps = 2;
//    int final_good_desks = Void::get_last_good(desks);


    for(int i = -20; i < 21; i++)
        times.push_back(NowUTC() + i);
    for(int i = -400; i <= 400; i+=50)
        times.push_back(NowUTC() + i);
    bool got_reprint_access_err = false;

    vector<pair<vector<Record>, vector<TestRecord> > > tables =
    {	{ {{ 1, "KIOSKB",  "VKO", "UT", 250, 1},
           { 1, "KIOSKB",  "", "UT", 50, 50},
           { 1, "KIOSKB", "VKO", "CX", 1, 1},
           { 1, "KIOSKB", "DME", "", 0, 0}
          },
          {{ 1, "KIOSKB",  "ZZA", "UT", +0, "wrong airp"},
           { 1, "KIOSKB",  "VKO", "ZZA", +0, "wrong airline"},
       { 1, "KIOSKB",  "ZZA", "ZZA", +0, "wrong airp and airline"},
           { 1, "KIOSKB",  "DME", "ZZA", +0, ""},
           { 1, "KIOSKB",  "VKO", "UT", -5, ""},
           { 1, "KIOSKB",  "VKO", "UT", -300, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", +300, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", -4, ""},
           { 1, "KIOSKB",  "VKO", "UT", +50, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", +1, ""},
           { 1, "KIOSKB",  "VKO", "UT", +0, ""},
           { 1, "KIOSKB",  "VKO", "UT", -300, "Time"},
           { 1, "KIOSKB",  "DME", "UT", -5, ""},
           { 1, "KIOSKB",  "DME", "UT", +0, ""},
       { 1, "KIOSKB",  "DME", "UT", +0, ""},
           { 1, "KIOSKB",  "DME", "CX", -51, "Time"},
           { 1, "KIOSKB",  "DME", "UT", +10, ""},
           { 1, "KIOSKB",  "VKO", "CX", +0, ""},
           { 1, "KIOSKB",  "VKO", "YC", -300, "Time"},
           { 1, "KIOSKB",  "VKO", "CX", +1, ""},
           { 1, "KIOSKB",  "DME", "CX", +0, ""},
           { 1, "KIOSKB",  "DME", "YC", +2, "Time"},
           { 1, "KIOSKB",  "DME", "YC", +1, "Time"},
           { 1, "KIOSKB",  "DME", "CX", +1, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", -5, ""}},
           //{ 1, "KIOSKB",  "", "UT", -10,  "bad data: airport cant be null inside the ticket"},
           //{ 1, "KIOSKB",  "", "", 0, "bad data: airline cant be null inside the ticket"}},
         },

         { {},

       {{ 1, "KIOSKB",  "VKO", "UT", -5, ""},
           { 9000, "K",  "VKO", "UT", -300, ""}, //wrong kiosk and kiosk_group
           { 1, "KIOSKB",  "ZZA", "UT", +300, ""}, //unexisted airport
           { 1, "KIOSKB",  "VKO", "ZZA", -4, ""}, //unexisted airline
           { 1, "KIOSKB",  "ZZA", "ZZA", +50, ""}, //unexisted airp airline
           { 1, "KIOSKB",  "VKO", "UT", +1, ""},
           { 1, "KIOSKB",  "VKO", "UT", +0, ""},
           { 1, "KIOSKB",  "VKO", "UT", -300, ""},
           { 1, "KIOSKB",  "DME", "UT", -5, ""},
           { 1, "KIOSKB",  "DME", "UT", +0, ""},
           { 1, "KIOSKB",  "DME", "UT", +0, ""},
           { 1, "KIOSKB",  "DME", "CX", -51, ""},
           { 1, "KIOSKB",  "DME", "UT", +10, ""},
           { 1, "KIOSKB",  "VKO", "CX", +0, ""},
           { 1, "KIOSKB",  "VKO", "YC", -300, ""},
           { 1, "KIOSKB",  "VKO", "CX", +1, ""},
           { 1, "KIOSKB",  "DME", "CX", +0, ""},
           { 1, "KIOSKB",  "DME", "YC", +2, ""},
           { 1, "KIOSKB",  "DME", "YC", +1, ""},
           { 1, "KIOSKB",  "DME", "CX", +1, ""},
           { 1, "KIOSKB",  "VKO", "UT", -5, ""}}
         },

         { {{ 1, "",  "VKO", "UT", 250, 1},
           { 1, "KIOSKB",  "", "UT", 50, 50},
           { 1, "KIOSKB", "VKO", "CX", 1, 1},
           { 1, "KIOSKB", "DME", "", 0, 0}
          },
          {{ 1, "K",  "VKO", "UT", -5,  ""},
           { 1, "K",  "VKO", "UT", +2, "Time"},
           { 1, "K",  "DME", "UT", +0, "Airp"},
           { 1,	"K",  "VKO", "UT", +0, ""},
           { 1, "KIOSKB",  "VKO", "UT", -300, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", +300, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", -51, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", +50, ""},
           { 1, "KIOSKB",  "VKO", "UT", +51, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", +0, ""},
           { 1, "KIOSKB",  "VKO", "UT", -300, "Time"},
           { 1, "KIOSKB",  "DME", "UT", -5, ""},
           { 1, "KIOSKB",  "DME", "UT", +0, ""},
           { 1, "KIOSKB",  "DME", "UT", +0, ""},
           { 1, "KIOSKB",  "DME", "CX", -51, "Time"},
           { 1, "KIOSKB",  "DME", "UT", +10, ""},
           { 1, "KIOSKB",  "VKO", "CX", +0, ""},
           { 1, "KIOSKB",  "VKO", "YC", -300, "Time"},
           { 1, "KIOSKB",  "VKO", "CX", +1, ""},
           { 1, "KIOSKB",  "DME", "CX", +0, ""},
           { 1, "KIOSKB",  "DME", "YC", +2, "Time"},
           { 1, "KIOSKB",  "DME", "YC", +1, "Time"},
           { 1, "KIOSKB",  "DME", "CX", +1, "Time"},
           { 1, "KIOSKB",  "VKO", "UT", -5, ""}},
         }

    };
    std::cout<<"Run...\n"; std::cout.flush();
    string exception_err_str;
    for(auto &t :tables)
    {   query.set_bd(t.first);
       for(auto &i : t.second)
              {
        got_reprint_access_err = false;
                  try
                  { TReqInfo::Instance()->desk.grp_id=i.grp;
            TReqInfo::Instance()->desk.code=i.desk;
                    std::cout << DateTimeToStr(i.time(), "dd.mm.yy hh:nn", true) << std::endl;
                    TPrnTagStore::check_reprint_access(i.time(), i.airp, i.airline);
                  }
                  catch(UserException &e)
                  {   exception_err_str = e.what();
                      if (e.getLexemaData().lexema_id == "MSG.REPRINT_ACCESS_ERR")
                         got_reprint_access_err = true;
                      else
                      if(e.getLexemaData().lexema_id == "MSG.REPRINT_WRONG_DATE_AFTER" || e.getLexemaData().lexema_id == "MSG.REPRINT_WRONG_DATE_BEFORE")  got_reprint_access_err = true;
                      else
                      {std::cout<<"Test failed: err with wrong description\n";
               ProgError(STDLOG, "Undefined error while test_check_reprint_access() in prn_tag_store.cc");
                       return false;
                      }
                  }
                  catch(std::exception &e)
                  {   exception_err_str = e.what();
                      //std::cout << "Test failed: " << e.what() << "\n";
                      got_reprint_access_err = true;

                  }
                  catch(...)
                  {   std::cout<<"Test failed: uknown err\n";
              ProgError(STDLOG, "Undefined error while test_check_reprint_access() in prn_tag_store.cc");
                      return false;
                  }
                  if(!got_reprint_access_err)
                  {   if(!i.err.empty()){
                         string err = string("Condition of test failed, waited ")  + i.err  + "\n with airp = "
                + i.airp
                           + " airline = " + i.airline + " time " +
                                 DateTimeToStr(i.time(), "dd.mm.yy", true) +  " kiosk " + i.desk + " kiosk group " + std::to_string(i.grp) + "\n"
            ;
                         err+= "\nException message: "  + exception_err_str +  " \n";
                         err+="for table {\n";
                           for(auto &z: t.first)
                             err+="\t\t" + z.str() + "\n";
                         err+="}\n";
                         ProgError(STDLOG, err.c_str());
                         std::cout<<err;
             return false;
                      }
                      /*if(i.airp >= (unsigned int)final_good_airps)
                      { std::cout<<"Test failed: bad airport name "<<airps[iairp]<<"didnt filtered\n";
            ProgError(STDLOG, "Failed test check_reprint_access() in prn_tag_store.cc, test didnt passed, because bad name airp didnt catched");
                        return false;
                      }
                      if(iairline >= (unsigned int)final_good_airlines)
                      { std::cout<<"Test failed: bad airline name "<<airlines[iairline]<<"didnt filtered\n";
                    ProgError(STDLOG, "Failed test check_reprint_access() in prn_tag_store.cc because bad name airline ");
                        return false;
                      }
              if(igrp >= (unsigned int)final_good_grps)
                      { std::cout<<"Test failed: bad grp_name "<<grps[igrp]<<"didnt filtered\n";
                        ProgError(STDLOG, "Failed test check_reprint_access() in prn_tag_store.cc because bad name grp ");
                        return false;
                      }
              if(idesk >= (unsigned int)final_good_desks)
                      { std::cout<<"Test failed: bad grp_name "<<desks[idesk]<<"didnt filtered\n";
                        ProgError(STDLOG, "Failed test check_reprint_access() in prn_tag_store.cc because bad name desk" );
                        return false;
                      }*/

                  }
                  else
                  { if(i.err.empty()){
                         string err = string("Condition of test failed in test with airp = ") + i.airp
                           + " airline = " + i.airline + " time " +
                 DateTimeToStr(i.time(), "dd.mm.yy", true) +  " kiosk " + i.desk + " kiosk group " + std::to_string(i.grp) +
                        "\nException message: "  + exception_err_str +  " \n"  +
            " in table: {\n";
                         for(auto &z: t.first)
                     err+="\t\t" + z.str() + "\n";
                         err += "}\n";
                         ProgError(STDLOG, err.c_str());
                         std::cout<<err;
                         return false;
                      }
                  }
                  exception_err_str = "";

              }
    }
    std::cout<<"Test passed\n";
    return true;
}

#endif

int test_reprint(int argc,char **argv)
{
#if 0
    test_check_reprint_access();
#endif
    return 1;
}

namespace TypeB
{

void SyncCHKD(int point_id_tlg, int point_id_spp, bool sync_all) //регистрация CHKD
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  ASTRA::TClientType prior_client_type=reqInfo->client_type;
  TUser prior_user=reqInfo->user;
  try
  {
    reqInfo->client_type=ctPNL;
    reqInfo->user.user_type=utSupport;
    reqInfo->user.access.set_total_permit();

    TQuery SavePointQry(&OraSession);

    TQuery UpdQry(&OraSession);
    UpdQry.Clear();
    UpdQry.SQLText="UPDATE crs_pax SET sync_chkd=0 WHERE pax_id=:pax_id AND sync_chkd<>0";
    UpdQry.DeclareVariable("pax_id", otInteger);

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "SELECT crs_pax.pax_id AS crs_pax_id, pax.pax_id, "
      "       MIN(crs_pax.surname) AS surname, "
      "       MIN(crs_pax.name) AS name, "
      "       MIN(crs_pax_chkd.reg_no) AS reg_no "
      "FROM crs_pnr, crs_pax, crs_pax_chkd, pax "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=crs_pax_chkd.pax_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pnr.point_id=:point_id_tlg AND "
      "      crs_pnr.system='CRS' AND "
      "      crs_pax.seats>0 AND crs_pax.pr_del=0 AND "
      "      (crs_pax.sync_chkd<>0 OR :sync_all<>0) "
      "GROUP BY crs_pax.pax_id, pax.pax_id "
      "ORDER BY reg_no, surname, name, crs_pax_id "; //пробуем обрабатывать в порядке регистрационных номеров - так справедливей
    Qry.CreateVariable("point_id_tlg", otInteger, point_id_tlg);
    Qry.CreateVariable("sync_all", otInteger, (int)sync_all);
    Qry.Execute();
    if (!Qry.Eof)
    {
      TFlights flightsForLock;
      flightsForLock.Get( point_id_spp, ftTranzit );
      flightsForLock.Lock();

      WebSearch::TPnrData pnrData;
      pnrData.flt.fromDB(point_id_spp, true, true);
      pnrData.flt.fromDBadditional(true, true);
      vector<WebSearch::TPnrData> PNRs;
      PNRs.insert(PNRs.begin(), pnrData); //вставляем в начало первый сегмент
      for(;!Qry.Eof;Qry.Next())
      {
        int crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
        list<int> crs_pax_ids;
        if (!Qry.FieldIsNULL("pax_id"))
          //пассажир зарегистрирован
          crs_pax_ids.push_back(crs_pax_id);
        else
        {
          SavePointQry.SQLText=
            "BEGIN "
            "  SAVEPOINT CHKD; "
            "END;";
          SavePointQry.Execute();
          try
          {
            XMLDoc emulDocHeader;
            CreateEmulXMLDoc(emulDocHeader);
            XMLDoc emulCkinDoc;
            if (!AstraWeb::CreateEmulCkinDocForCHKD(crs_pax_id,
                                                    PNRs,
                                                    emulDocHeader,
                                                    emulCkinDoc,
                                                    crs_pax_ids)) continue;

            if (emulCkinDoc.docPtr()!=NULL) //регистрация новой группы
            {
              ProgTrace(TRACE5, "SyncCHKD:\n%s", XMLTreeToText(emulCkinDoc.docPtr()).c_str());

              xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
              if (emulReqNode==NULL)
                throw EXCEPTIONS::Exception("emulReqNode=NULL");

              TChangeStatusList ChangeStatusInfo;
              SirenaExchange::TLastExchangeList SirenaExchangeList;
              CheckIn::TAfterSaveInfoList AfterSaveInfoList;
              if (CheckInInterface::SavePax(emulReqNode, NULL/*ediResNode*/, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList))
              {
                //сюда попадаем если была реальная регистрация
                SirenaExchangeList.handle(__FUNCTION__);
                AfterSaveInfoList.handle(__FUNCTION__);
              };
            };
          }
          catch(AstraLocale::UserException &e)
          {
            try
            {
              dynamic_cast<CheckIn::OverloadException&>(e);
            }
            catch (bad_cast)
            {
              SavePointQry.SQLText=
                "BEGIN "
                "  ROLLBACK TO CHKD; "
                "END;";
              SavePointQry.Execute();
            };

            string surname=Qry.FieldAsString("surname");
            string name=Qry.FieldAsString("name");
            string err_id;
            LEvntPrms err_prms;
            e.getAdvParams(err_id, err_prms);

            TLogLocale locale;
            locale.ev_type=ASTRA::evtPax;
            locale.id1=point_id_spp;
            locale.id2=NoExists;
            locale.id3=NoExists;
            locale.lexema_id = "EVT.PAX.REG_ERROR";

            locale.prms << PrmSmpl<string>("name", surname+(name.empty()?"":" ")+name) << PrmLexema("what", err_id, err_prms);
            TReqInfo::Instance()->LocaleToLog(locale);
          }
          catch(EXCEPTIONS::Exception &e)
          {
            SavePointQry.SQLText=
              "BEGIN "
              "  ROLLBACK TO CHKD; "
              "END;";
            SavePointQry.Execute();
            ProgError(STDLOG, "TypeB::SyncCHKDPax (crs_pax_id=%d): %s", crs_pax_id, e.what());
          };
        };

        for(list<int>::const_iterator i=crs_pax_ids.begin(); i!=crs_pax_ids.end(); ++i)
        {
          UpdQry.SetVariable("pax_id", *i);
          UpdQry.Execute();
        };
      };
    };
    reqInfo->client_type=prior_client_type;
    reqInfo->user=prior_user;
  }
  catch(...)
  {
    reqInfo->client_type=prior_client_type;
    reqInfo->user=prior_user;
    throw;
  };
};

void SyncCHKD(int point_id_spp, bool sync_all)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id_tlg "
    "FROM tlg_binding, points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      point_id_spp=:point_id_spp AND "
    "      points.pr_del=0 AND points.pr_reg<>0";
  Qry.CreateVariable("point_id_spp", otInteger, point_id_spp);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    SyncCHKD(Qry.FieldAsInteger("point_id_tlg"), point_id_spp, sync_all);
};

void SyncNewCHKD(int point_id_spp, const string& task_name, const string& params)
{
  SyncCHKD(point_id_spp, false);
};

void SyncAllCHKD(int point_id_spp, const string& task_name, const string& params)
{
  SyncCHKD(point_id_spp, true);
};

} //namespace TypeB
