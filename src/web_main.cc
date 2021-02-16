#include "oralib.h"
#include "exceptions.h"
#include "stages.h"
#include "salons.h"
#include "salonform.h"
#include "images.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_context.h"
#include "date_time.h"
#include "misc.h"
#include "astra_misc.h"
#include "print.h"
#include "web_main.h"
#include "web_search.h"
#include "checkin.h"
#include "astra_locale.h"
#include "comp_layers.h"
#include "comp_props.h"
#include "passenger.h"
#include "remarks.h"
#include "points.h"
#include "stages.h"
#include "astra_service.h"
#include "tlg/tlg.h"
#include "qrys.h"
#include "request_dup.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "serverlib/savepoint.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"
#include "checkin_utils.h"
#include "apis_utils.h"
#include "stl_utils.h"
#include "astra_callbacks.h"
#include "meridian.h"
#include "brands.h"
#include "ffp_sirena.h"
#include "seats_utils.h"
#include "rfisc_sirena.h"
#include "web_exchange.h"
#include "ckin_search.h"
#include "web_craft.h"
#include "seatPax.h"
#include "flt_settings.h"
#include "baggage_calc.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC::date_time;
using namespace AstraLocale;

InetClient getInetClient(const string &client_id)
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

InetClient getInetClientByKioskId(const std::string &kiosk_id,const std::string &application)
{
  InetClient client;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT client_id FROM kiosk_clients,kiosk_app_list "
    " WHERE kiosk_app_list.id=kiosk_clients.app_id AND "
    "       kiosk_app_list.code=:app AND "
    "       kiosk_clients.kiosk_id=:kiosk_id";
  Qry.CreateVariable( "app", otString, application );
  Qry.CreateVariable( "kiosk_id", otString, kiosk_id );
  Qry.Execute();
  client.client_id = Qry.Eof?"":Qry.FieldAsString( "client_id");
  if ( !client.client_id.empty() ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT client_type,web_clients.desk,login "
      "FROM web_clients,users2 "
      "WHERE web_clients.client_id=:client_id AND "
      "      web_clients.client_type=:client_type AND "
      "      web_clients.user_id=users2.user_id";
    Qry.CreateVariable( "client_id", otString, client.client_id );
    Qry.CreateVariable( "client_type", otString,  EncodeClientType(ctKiosk) );
    Qry.Execute();
    if ( Qry.Eof ) {
      client.clear();
    }
    else {
      client.pult = Qry.FieldAsString( "desk" );
      client.opr = Qry.FieldAsString( "login" );
      client.client_type = Qry.FieldAsString( "client_type" );
      ProgTrace( TRACE5, "client=%s", client.toString().c_str() );
    }
  }
  if ( client.client_id.empty() ) {
    ProgError(STDLOG, "%s: kiosk_id=%s application=%s not found", __FUNCTION__, kiosk_id.c_str(), application.c_str());
  }
  return client;
}

namespace AstraWeb
{

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

void WebRequestsIface::emulateClientType()
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm)
  {
    reqInfo->client_type=EMUL_CLIENT_TYPE;
    //���� �᪮�����஢��� ⮫쪮 �᫨ ���� �⫠��� �⢥⮢ �� ᠬ�ॣ����樨
    //�� � ���� ��砥 �� �� ࠡ�祬 �ࢥ�! ���� ���� �஡���� �� �뢮�� �訡�� � �ନ��� �����
//    AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
//    if (astra_cb_ptr!=nullptr)
//      astra_cb_ptr->SetPostProcessXMLAnswerCallback(RevertWebResDoc);
  };
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TIdsPnrData {
  WebSearch::TFlightInfo flt;
  map< int/*pnr_id*/, set<int/*pax_id*/> > pnr_ids;
  set<int/*pax_id*/> pax_ids;

  TIdsPnrData(const WebSearch::TFlightInfo& _flt) : flt(_flt) {}

  void add(int pnr_id, boost::optional<int> pax_id);
  TIdsPnrData& fromXML(xmlNodePtr idsParentNode);
  TIdsPnrData& fromXMLMulti(xmlNodePtr idsParentNode);
  int getStrictlySinglePnrId() const;
  bool containAtLeastOnePnrId() const;
  bool paxIdExists(int pax_id) const
  {
    return pax_ids.empty() ||
           pax_ids.find(pax_id)!=pax_ids.end();
  }

  static bool trueMultiRequest(xmlNodePtr reqNode)
  {
    return std::string((const char*)reqNode->name)=="LoadPnrMulti";
  }

  static int VerifyPNRByPaxId( int point_id, int pax_id ); //�����頥� pnr_id
  static int VerifyPNRByPnrId( int point_id, int pnr_id ); //�����頥� pnr_id

  private:
    static int VerifyPNR( int point_id, int id, bool is_pnr_id );
};

struct TIdsPnrDataSegs : public std::list<TIdsPnrData> {
  bool showBPNotAllowedReason;
  TIdsPnrDataSegs(xmlNodePtr reqNode) : showBPNotAllowedReason(false)
  {
    showBPNotAllowedReason=NodeAsBoolean("@showBPNotAllowedReason", reqNode, false);
  }
};

int TIdsPnrData::VerifyPNR( int point_id, int id, bool is_pnr_id )
{
  set<int> point_ids_spp;
  int result=getPointIdsSpp(id, is_pnr_id, point_ids_spp);
  if (result==ASTRA::NoExists)
    throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
  if (isTestPaxId(id)) point_ids_spp.insert(point_id);
  if (point_ids_spp.find(point_id)==point_ids_spp.end())
    throw UserException( "MSG.PASSENGERS.OTHER_FLIGHT" );

  return result;
}

int TIdsPnrData::VerifyPNRByPaxId( int point_id, int pax_id )
{
  return VerifyPNR(point_id, pax_id, false);
}

int TIdsPnrData::VerifyPNRByPnrId( int point_id, int pnr_id )
{
  return VerifyPNR(point_id, pnr_id, true);
}

void TIdsPnrData::add(int pnr_id, boost::optional<int> pax_id)
{
  auto iPnrId=pnr_ids.find(pnr_id);
  if (iPnrId==pnr_ids.end())
    iPnrId=(pnr_ids.emplace(pnr_id, set<int>())).first;
  if (pax_id)
  {
    iPnrId->second.insert(pax_id.get());
    pax_ids.insert(pax_id.get());
  }
}

TIdsPnrData& TIdsPnrData::fromXML(xmlNodePtr idsParentNode)
{
  pnr_ids.clear();
  if (idsParentNode==nullptr) return *this;
  if (GetNode( "pnr_id", idsParentNode )!=NULL)
  {
    int pnr_id = NodeAsInteger( "pnr_id", idsParentNode );
    VerifyPNRByPnrId( flt.oper.point_id, pnr_id );
    add(pnr_id, boost::none);
  }
  else
  {
    int crs_pax_id = NodeAsInteger( "crs_pax_id", idsParentNode );
    add(VerifyPNRByPaxId( flt.oper.point_id, crs_pax_id ), boost::none);
  }

  return *this;
}

TIdsPnrData& TIdsPnrData::fromXMLMulti(xmlNodePtr idsParentNode)
{
  pnr_ids.clear();
  if (idsParentNode==nullptr) return *this;
  if (GetNode("crs_pax_ids", idsParentNode)!=nullptr)
  {
    for(xmlNodePtr idNode=NodeAsNode("crs_pax_ids/crs_pax_id", idsParentNode); idNode!=nullptr; idNode=idNode->next)
      if (string((const char*)idNode->name)=="crs_pax_id")
      {
        int crs_pax_id=NodeAsInteger(idNode);
        add(VerifyPNRByPaxId( flt.oper.point_id, crs_pax_id ), crs_pax_id);
      }
  }
  else
  {
    for(xmlNodePtr idNode=NodeAsNode("crs_pnr_ids/crs_pnr_id", idsParentNode); idNode!=nullptr; idNode=idNode->next)
      if (string((const char*)idNode->name)=="crs_pnr_id")
      {
        int pnr_id=NodeAsInteger(idNode);
        VerifyPNRByPnrId( flt.oper.point_id, pnr_id );
        add(pnr_id, boost::none);
      }
  }

  return *this;
}

int TIdsPnrData::getStrictlySinglePnrId() const
{
  if (pnr_ids.size()!=1)
    throw EXCEPTIONS::Exception("TIdsPnrData::getStrictlySinglePnrId: pnr_ids.size()!=1");

  return pnr_ids.begin()->first;
}

bool TIdsPnrData::containAtLeastOnePnrId() const
{
  return !pnr_ids.empty();
}

static void GetPNRsList(const WebSearch::TPNRFilter &filter,
                        bool isSearchPNRsRequest,
                        list<WebSearch::TPNRs> &PNRsList,
                        list<AstraLocale::LexemaData> &errors)
{
  PNRsList.clear();
  errors.clear();
  if (!(filter.test_paxs.empty() && !filter.from_scan_code)) return;

  CheckIn::TSimplePaxList paxs;
  SurnameFilter surname(filter);

  CheckIn::Search search(paxPnl, WebSearch::TIMEOUT());
  if (!filter.ticket_no.empty())
  {
    CheckIn::TPaxTknItem tkn;
    tkn.no=filter.ticket_no;
    search(paxs, tkn, surname);
  }
  else if (!filter.pnr_addr_normal.empty())
  {
    TPnrAddrInfo pnr;
    pnr.addr=filter.pnr_addr_normal;
    search(paxs, pnr, surname);
  }
  else if (!filter.document.empty())
  {
    CheckIn::TPaxDocItem doc;
    doc.no=filter.document;
    search(paxs, doc); //�� 䠬���� 䨫���㥬 ��⮬, ���� ���� ����� �� ��ࢠ���� �祭� �����
  };

  if (search.timeoutIsReached())
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": paxs.size()=" << paxs.size();
    throw UserException("MSG.TOO_LONG_SEARCH.ADJUST_SEARCH_PARAMS");
  }

  for(bool checked : {false, true})
    for(const CheckIn::TSimplePaxItem& pax : paxs)
    {
      if (checked!=(pax.grp_id!=ASTRA::NoExists)) continue;

//      ProgTrace(TRACE5, "%s: checked=%d pax_id=%d", __FUNCTION__, (int)checked, pax.id);

      TAdvTripInfoList flts;
      if (checked)
      {
        TAdvTripInfo flt;
        if (flt.getByGrpId(pax.grp_id)) flts.push_back(flt);
      }
      else getTripsByCRSPaxId(pax.id, flts);

      if (flts.empty())
      {
//        ProgTrace(TRACE5, "%s: flts.empty()", __FUNCTION__);
        continue;
      }

      WebSearch::TPaxInfo paxInfo;
      if (!paxInfo.setIfSuitable(filter, pax))
      {
//        ProgTrace(TRACE5, "%s: !paxInfo.setIfSuitable", __FUNCTION__);
        continue;
      }

      for(const TAdvTripInfo& flt : flts)
      {
        WebSearch::TPNRSegInfo segInfo;
        if (!segInfo.setIfSuitable(filter, flt, pax))
        {
//          ProgTrace(TRACE5, "%s: !segInfo.setIfSuitable", __FUNCTION__);
          continue;
        }

        if (!segInfo.mktFlight) continue;

        WebSearch::TFlightInfo fltInfo;
        if (!fltInfo.setIfSuitable(filter, flt, segInfo.mktFlight.get()))
        {
//          ProgTrace(TRACE5, "%s: !fltInfo.setIfSuitable", __FUNCTION__);
          continue;
        }

        list<WebSearch::TPNRs>::iterator iPNRs=PNRsList.begin();
        if (!isSearchPNRsRequest)
        {
          for(; iPNRs!=PNRsList.end(); ++iPNRs)
            if (iPNRs->getFirstPNRInfo().getFirstPointDep()==fltInfo.oper.point_id) break;
        }

        if (iPNRs==PNRsList.end())
          iPNRs=PNRsList.emplace(PNRsList.end());
        try
        {
          if (!checked || !iPNRs->isSameTicketInAnotherPNR(segInfo, paxInfo))
            iPNRs->add(fltInfo, segInfo, paxInfo, false);

          if (iPNRs->pnrs.empty()) PNRsList.erase(iPNRs);
        }
        catch(UserException &e)
        {
          if (iPNRs->pnrs.empty()) PNRsList.erase(iPNRs);
          errors.push_back(e.getLexemaData());
          ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
        }
      }
    }

  if (!isSearchPNRsRequest)
  {
    if (PNRsList.empty() && errors.empty())
      errors.push_back(LexemaData("MSG.PASSENGERS.NOT_FOUND"));

    for(WebSearch::TPNRs& PNRs : PNRsList)
      if (PNRs.pnrs.size()>1)
      {
        PNRs.trace(WebSearch::xmlSearchPNRs);
        PNRs.error=LexemaData("MSG.PASSENGERS.FOUND_MORE.ADJUST_SEARCH_PARAMS");
      }

    PNRsList.sort(WebSearch::TPNRsSortOrder(NowUTC()-1));
  }
}

void WebRequestsIface::SearchPNRs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  resNode=NewTextChild(resNode,"SearchPNRs");

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->user.access.airlines().totally_not_permitted() ||
      reqInfo->user.access.airps().totally_not_permitted() )
  {
    ProgError(STDLOG, "WebRequestsIface::SearchPNRs: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
  };

  WebSearch::TPNRFilter filter;
  filter.fromXML(reqNode, reqNode);
  filter.testPaxFromDB();
  filter.trace(TRACE5);

  if (filter.test_paxs.empty() && !filter.from_scan_code)
  {
    list<WebSearch::TPNRs> PNRsList;
    list<AstraLocale::LexemaData> errors;
    GetPNRsList(filter, true, PNRsList, errors);
    if (!PNRsList.empty())
      PNRsList.front().toXML(resNode, true, WebSearch::xmlSearchPNRs);
  }
  else
  {
    WebSearch::TPNRs PNRs;
    WebSearch::findPNRs(filter, PNRs);
    PNRs.toXML(resNode, true, WebSearch::xmlSearchPNRs);
  }
}

static void GetPNRsList(WebSearch::TPNRFilters &filters,
                        list<WebSearch::TPNRs> &PNRsList,
                        list<AstraLocale::LexemaData> &errors)
{
  PNRsList.clear();
  errors.clear();

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

    if (filter.test_paxs.empty() && !filter.from_scan_code)
    {
      if (filters.segs.size()!=1)
        throw EXCEPTIONS::Exception("%s: filters.segs.size()!=1", __FUNCTION__);
      GetPNRsList(filter, false, PNRsList, errors);
      break;
    };

    PNRsList.emplace_back();
    try
    {
      WebSearch::TPNRs &PNRs=PNRsList.back();

      WebSearch::findPNRs(filter, PNRs, false);
      if (PNRs.pnrs.empty())
      {
        if (filter.test_paxs.empty() && filter.from_scan_code)
        {
          WebSearch::findPNRs(filter, PNRs, true);
          if (!PNRs.pnrs.empty())
            throw UserException( "MSG.PASSENGER.CONTACT_CHECKIN_AGENT" ); //��㣮� ॣ. �����
        }
        throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
      }

      if (PNRs.pnrs.size()>1 && filter.test_paxs.empty())
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

static void weedPNRsList(list<WebSearch::TPNRs> &PNRsList)
{
  for(list<WebSearch::TPNRs>::iterator i=PNRsList.begin(); i!=PNRsList.end();)
  {
    list<WebSearch::TPNRs>::const_iterator j=PNRsList.begin();
    for(; j!=PNRsList.end(); ++j)
    {
      if (i==j) continue;
      if (i->partOf(*j)) break;
    }
    if (j!=PNRsList.end() || i->pnrs.empty())
      i=PNRsList.erase(i);
    else
      ++i;
  }
}

void WebRequestsIface::SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  resNode=NewTextChild(resNode, (const char*)reqNode->name);

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->user.access.airlines().totally_not_permitted() ||
      reqInfo->user.access.airps().totally_not_permitted())
  {
    ProgError(STDLOG, "WebRequestsIface::SearchFlt: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    return;
  };

//  <?xml version="1.0" encoding="UTF-8"?>
//  <term>
//    <query handle="0" id="WEB" ver="1" opr="VLAD" screen="MAINDCS.EXE" mode="STAND" lang="RU" term_id="473986836">
//      <SearchFlt>
//        <scan_code>M2IVANOV/IVAN         E01LDGM SGCVKOUT 248  59 Y          49>50000298/2410809150  001B13PS7774441110                                 01LDGM VKOAERUT 249  59 Y          00</scan_code>
//      </SearchFlt>
//    </query>
//  </term>

  WebSearch::TMultiPNRFiltersList multiPNRFiltersList;
  multiPNRFiltersList.fromXML(reqNode);

  if (multiPNRFiltersList.showAllSegments(reqNode) &&
      multiPNRFiltersList.size()==1 &&
      multiPNRFiltersList.front().isExcellentSearchParams())
  {
    //ᠬ� ��ࢮ��砫�� ��� �������� � ���� �����⢥�����
    //� ����⢥ ����� - ����� ����� ��� ����� PNR
    list<WebSearch::TMultiPNRsList> listOfMultiPNRsList;

    WebSearch::TMultiPNRFilters& filters=multiPNRFiltersList.front();

    list<AstraLocale::LexemaData> errors;
    list<WebSearch::TPNRs> PNRsList;

    GetPNRsList(filters, PNRsList, errors);
    weedPNRsList(PNRsList);

    for(const WebSearch::TPNRs& PNRs : PNRsList)
    {
      if (PNRs.pnrs.empty()) continue; //�� ��直� ��砩

      listOfMultiPNRsList.emplace_back();
      if (PNRsList.size()==1 && PNRs.error) //�᫨ ��᪮�쪮 ������⮢, �訡�� �� �뢮���, �����뢠�� �� ��������
      {
        errors.push_back(PNRs.error.get());
        listOfMultiPNRsList.back().add(filters, errors);
      }
      else
        listOfMultiPNRsList.back().add(filters, PNRs);
    }

    if (listOfMultiPNRsList.empty())
    {
      if (errors.empty()) throw EXCEPTIONS::Exception("%s: errors.empty()", __FUNCTION__);
      listOfMultiPNRsList.emplace_back();
      listOfMultiPNRsList.back().add(filters, errors);
    }

    for(WebSearch::TMultiPNRsList& multiPNRsList : listOfMultiPNRsList)
      multiPNRsList.checkAndPutToXML(resNode);

    return;
  }


  class nextPNRs{};
  class nextPNRFilters{};

  WebSearch::TMultiPNRsList multiPNRsList;
  for(WebSearch::TMultiPNRFilters& filters : multiPNRFiltersList)
  try
  {
    list<AstraLocale::LexemaData> errors;
    list<WebSearch::TPNRs> PNRsList;

    GetPNRsList(filters, PNRsList, errors);

    if (multiPNRFiltersList.showAllSegments(reqNode))
      weedPNRsList(PNRsList);

    for(int pass=1; pass<=5; pass++)
    {
      for(const WebSearch::TPNRs& PNRs : PNRsList)
      try
      {
        if (PNRs.pnrs.empty()) continue; //�� ��直� ��砩

        if (PNRsList.size()>1)
        {
          int priority=PNRs.calcStagePriority(PNRs.getFirstSegId());
          ProgTrace(TRACE5, "%s: pass=%d priority=%d ", __FUNCTION__, pass, priority);

          if (pass!=priority) throw nextPNRs();
        }

        if (PNRs.error)
        {
          errors.push_back(PNRs.error.get());
          multiPNRsList.add(filters, errors);
        }
        else
          multiPNRsList.add(filters, PNRs);
        throw nextPNRFilters(); //��室�� �� 横�� ��室�� pass, ⠪ ��� ����ᠫ� � multiPNRsList ᪢���� ᥣ����� � ᠬ� ���室�騬 1-� ᥣ���⮬
      }
      catch(nextPNRs) {};
    };
    if (errors.empty()) throw EXCEPTIONS::Exception("%s: errors.empty()", __FUNCTION__);
    multiPNRsList.add(filters, errors);
  }
  catch(nextPNRFilters) {};

  multiPNRsList.checkAndPutToXML(resNode);
}
/*
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
*/

struct TWebGrp {
  WebSearch::TFlightInfo flt;
  boost::optional<CheckIn::Segment> segment;
  bool transitBortChangingExists=false;
  TCompleteAPICheckInfo checkInfo;
  vector<TWebPax> paxs;

  TWebGrp(const WebSearch::TFlightInfo& _flt) : flt(_flt) {}

  void fromDBadditional(const AirportCode_t& airpArv)
  {
    TPaxSegmentPair segmentPair(flt.oper.point_id, airpArv.get());
    segment=CheckIn::Segment::fromDB(segmentPair.pointDep(),
                                     AirportCode_t(flt.oper.airp),
                                     {},
                                     segmentPair.airpArv());
    if (segment)
      transitBortChangingExists=segment.get().transitBortChangingExists();
    checkInfo.set(segmentPair);
  }

  bool moreThanOnePersonWithSeat() const
  {
    int persons=0;
    for(vector<TWebPax>::const_iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax)
    {
      if (iPax->seats!=0) persons++;
      if (persons>1) return true;
    };
    return persons>1;
  }

  void addPnr(int pnr_id, bool pr_throw, bool afterSave);
  string seat_status(const ASTRA::TCompLayerType& crs_seat_layer) const;
  void toXML(xmlNodePtr segParentNode) const;
};

struct TWebGrpSegs : public list<TWebGrp>
{
  void toXML(xmlNodePtr segsParentNode) const;
};

bool is_valid_pnr_status(const string &pnr_status)
{
  return !(//pax.name=="CBBG" ||  ���� ����� � ��ࣨ����
               pnr_status=="DG2" ||
               pnr_status=="RG2" ||
               pnr_status=="ID2" ||
               pnr_status=="WL");
};

bool is_forbidden_repeated_ckin(const TTripInfo &flt,
                                int pax_id)
{
  if (!GetSelfCkinSets(tsNoRepeatedSelfCkin, flt, TReqInfo::Instance()->client_type)) return false;

  TCachedQuery Qry("SELECT time FROM crs_pax_refuse "
                   "WHERE pax_id=:pax_id AND client_type IN (:ctWeb, :ctMobile, :ctKiosk) AND rownum<2",
                   QParams() << QParam("pax_id", otInteger, pax_id)
                             << QParam("ctWeb", otString, EncodeClientType(ctWeb))
                             << QParam("ctMobile", otString, EncodeClientType(ctMobile))
                             << QParam("ctKiosk", otString, EncodeClientType(ctKiosk)));
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
  return !is_forbidden_repeated_ckin(flt, pax_id) &&
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
                        const CheckIn::TDocaMap &doca_map)
{
  for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
    if (checkInfo.incomplete(d->second)) return false;
  return true;
};

bool is_valid_tkn_info(const TCompleteAPICheckInfo &checkInfo,
                       const CheckIn::TPaxTknItem &tkn)
{
  if (checkInfo.incomplete(tkn)) return false;
  return true;
};

bool is_et_not_displayed(const TTripInfo &flt,
                         const CheckIn::TPaxTknItem &tkn,
                         const TETickItem &etick)
{
  bool pr_etl_only=GetTripSets(tsETSNoExchange, flt);
  if (!pr_etl_only && tkn.validET() && etick.empty()) return true;
  return false;
}

void TWebGrp::addPnr(int pnr_id, bool pr_throw, bool afterSave)
{
  try
  {
    if (!isTestPaxId(pnr_id))
    {
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
          "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
          "       DECODE(pax.pax_id,NULL,crs_pax.seats,pax.seats) AS seats, "
          "       CASE WHEN pax.pax_id IS NULL THEN "+CheckIn::TSimplePaxItem::origClassFromCrsSQL()+" ELSE pax_grp.class END AS class, "
          "       CASE WHEN pax.pax_id IS NULL THEN "+CheckIn::TSimplePaxItem::origSubclassFromCrsSQL()+" ELSE pax.subclass END AS subclass, "
          "       CASE WHEN pax.pax_id IS NULL THEN "+CheckIn::TSimplePaxItem::cabinClassFromCrsSQL()+" ELSE NVL(pax.cabin_class, pax_grp.class) END AS cabin_class, "
          "       CASE WHEN pax.pax_id IS NULL THEN "+CheckIn::TSimplePaxItem::cabinSubclassFromCrsSQL()+" ELSE NVL(pax.cabin_subclass, pax.subclass) END AS cabin_subclass, "
          "       DECODE(pax.pax_id,NULL,crs_pnr.airp_arv,pax_grp.airp_arv) AS airp_arv, "
          "       crs_pnr.status AS pnr_status, "
          "       crs_pax.bag_norm AS crs_bag_norm, crs_pax.bag_norm_unit AS crs_bag_norm_unit, "
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
      if (!Qry.Eof)
      {
        fromDBadditional(AirportCode_t(Qry.FieldAsString("airp_arv")));

        TPnrAddrs pnr_addrs;
        pnr_addrs.getByPnrIdFast(pnr_id);
        TBrands brands; //�����, �⮡� ���஢����� ������
        boost::optional<TRemGrp> forbiddenRemGrp; //����� �⮡� ���஢����� TRemGrp.Load
        for(;!Qry.Eof;Qry.Next())
        {
          TWebPax pax;
          pax.pnr_addrs=pnr_addrs;
          pax.crs_pnr_id = pnr_id;
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
          SeatQry.SetVariable("layer_type", FNull);
          SeatQry.SetVariable("crs_seat_no", FNull);
          SeatQry.Execute();
          pax.seat_status=seat_status(DecodeCompLayerType(SeatQry.GetVariableAsString("layer_type")));
          pax.crs_seat_no=SeatQry.GetVariableAsString("crs_seat_no");
          pax.seats = Qry.FieldAsInteger( "seats" );
          pax.orig_class = Qry.FieldAsString( "class" );
          pax.orig_subclass = Qry.FieldAsString( "subclass" );
          pax.cabin_class = Qry.FieldAsString( "cabin_class" );
          pax.cabin_subclass = Qry.FieldAsString( "cabin_subclass" );
          pax.context.fromDB(pax.crs_pax_id);
          if ( !Qry.FieldIsNULL( "pax_id" ) )
          {
            //���ᠦ�� ��ॣ����஢��
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
            {
              pax.etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
              brands.get(flt.oper.airline,pax.etick);
              pax.brand=brands.getSingleBrand();
            };
            LoadPaxDoc(pax.pax_id, pax.doc);
            LoadPaxDoco(pax.pax_id, pax.doco);
            CheckIn::LoadPaxFQT(pax.pax_id, pax.fqts);
            CheckIn::PaxRemAndASVCFromDB(pax.pax_id, false, pax.fqts, pax.rems_and_asvc);
          }
          else
          {
            pax.checkin_status = "not_checked";
            //�஢�ઠ CBBG (��� ���� ������ � ᠫ���)
            /*CrsPaxRemQry.SetVariable( "pax_id", pax.pax_id );
                CrsPaxRemQry.SetVariable( "rem_code", "CBBG" );
                CrsPaxRemQry.Execute();
                if (!CrsPaxRemQry.Eof)*/
            if (pax.name=="CBBG")
              pax.pers_type_extended = "��"; //CBBG

            CheckIn::LoadCrsPaxTkn(pax.crs_pax_id, pax.tkn);
            if (pax.tkn.validET())
            {
              pax.etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
              brands.get(flt.oper.airline,pax.etick);
              pax.brand=brands.getSingleBrand();
            }
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.tkn.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.tkn.getNotEmptyFieldsMask());
            LoadCrsPaxDoc(pax.crs_pax_id, pax.doc);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doc.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doc.getNotEmptyFieldsMask());
            LoadCrsPaxVisa(pax.crs_pax_id, pax.doco);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doco.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doco.getNotEmptyFieldsMask());
            LoadCrsPaxDoca(pax.crs_pax_id, pax.doca_map);

            CheckIn::LoadCrsPaxFQT(pax.crs_pax_id, pax.fqts);
            CheckIn::PaxRemAndASVCFromDB(pax.crs_pax_id, true, pax.fqts, pax.rems_and_asvc);

            if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")))
              pax.agent_checkin_reasons.insert("pnr_status");
            if (is_forbidden_repeated_ckin(flt.oper, pax.crs_pax_id))
              pax.agent_checkin_reasons.insert("web_cancel");
            if (!is_valid_pax_nationality(flt.oper, pax.crs_pax_id))
              pax.agent_checkin_reasons.insert("pax_nationality");
            if (!is_valid_doc_info(checkInfo, pax.doc))
              pax.agent_checkin_reasons.insert("incomplete_doc");
            if (!is_valid_doco_info(checkInfo, pax.doco))
              pax.agent_checkin_reasons.insert("incomplete_doco");
            if (!is_valid_doca_info(checkInfo, pax.doca_map))
              pax.agent_checkin_reasons.insert("incomplete_doca");
            if (!is_valid_tkn_info(checkInfo, pax.tkn))
              pax.agent_checkin_reasons.insert("incomplete_tkn");
            if (is_et_not_displayed(flt.oper, pax.tkn, pax.etick))
              pax.agent_checkin_reasons.insert("et_not_displayed");
            if (forbiddenRemExists(flt.oper, pax.rems_and_asvc, forbiddenRemGrp))
              pax.agent_checkin_reasons.insert("forbidden_rem_code");
            if (transitBortChangingExists)
              pax.agent_checkin_reasons.insert("transit_bort_changing");

            if (!pax.agent_checkin_reasons.empty())
              pax.checkin_status = "agent_checkin";
          }
          pax.bagNorm=trueBagNorm(CheckIn::TSimplePaxItem::getCrsBagNorm<string>(Qry, "crs_bag_norm", "crs_bag_norm_unit"), pax.etick);
          pax.crs_pnr_tid = Qry.FieldAsInteger( "crs_pnr_tid" );
          pax.crs_pax_tid = Qry.FieldAsInteger( "crs_pax_tid" );
          if ( !Qry.FieldIsNULL( "pax_grp_tid" ) )
            pax.pax_grp_tid = Qry.FieldAsInteger( "pax_grp_tid" );
          if ( !Qry.FieldIsNULL( "pax_tid" ) )
            pax.pax_tid = Qry.FieldAsInteger( "pax_tid" );
          paxs.push_back( pax );
        };
      };
    }
    else
    {
      TQuery Qry(&OraSession);
        Qry.SQLText =
        "SELECT surname, name, subcls.class, subclass, doc_no, tkn_no, "
        "       pnr_airline AS fqt_airline, fqt_no, "
        "       pnr_airline, pnr_addr, "
        "       seat_xname, seat_yname "
        "FROM test_pax, subcls "
        "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
        Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        //��⮢� ���ᠦ��
        TWebPax pax;
        if (!Qry.FieldIsNULL("pnr_airline") && !Qry.FieldIsNULL("pnr_addr"))
        {
          pax.pnr_addrs.emplace_back(Qry.FieldAsString("pnr_airline"),
                                     Qry.FieldAsString("pnr_addr"));
        }
        pax.crs_pnr_id = pnr_id;
        pax.crs_pax_id = pnr_id;
        pax.surname = Qry.FieldAsString("surname");
        pax.name = Qry.FieldAsString("name");
        pax.pers_type_extended = EncodePerson(adult);
        pax.seats = 1;
        pax.orig_class = Qry.FieldAsString( "class" );
        pax.orig_subclass = Qry.FieldAsString( "subclass" );
        pax.cabin_class = Qry.FieldAsString( "class" );
        pax.cabin_subclass = Qry.FieldAsString( "subclass" );

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
          pax.pax_grp_tid = flt.oper.point_id;
          pax.pax_tid = flt.oper.point_id;
        };

        paxs.push_back( pax );
      };
    };
    ProgTrace( TRACE5, "pass count=%zu", paxs.size() );
    if ( paxs.empty() )
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

void IntLoadPnr( const TIdsPnrDataSegs &ids,
                 const boost::optional<WebSearch::TPNRFilter> &filter,
                 TWebGrpSegs &grpSegs,
                 bool afterSave )
{
  grpSegs.clear();
  for(const TIdsPnrData& idsSeg : ids)
  {
    int point_id=idsSeg.flt.oper.point_id;
    try
    {
      TWebGrp grp(idsSeg.flt);
      for(const auto& iPnrIds : idsSeg.pnr_ids)
        grp.addPnr(iPnrIds.first, grpSegs.empty(), afterSave );

      if (grpSegs.empty())
      {
        //ࠡ�⠥� � ���� ᥣ���⮬
        set<int> suitable_ids;
        //������ ��宦�� ������ ��� ������, �ਢ易���� � ��宦�� ������栬
        for(const TWebPax& pax : grp.paxs)
        {
          if ((filter && pax.suitable(filter.get())) ||
              (!filter/* && idsSeg.paxIdExists(pax.crs_pax_id)*/))
            suitable_ids.insert(pax.crs_pax_id_parent!=NoExists?pax.crs_pax_id_parent:pax.crs_pax_id);
        }

        //��䨫���㥬 ���ᠦ�஢
        for(vector<TWebPax>::iterator iPax=grp.paxs.begin();iPax!=grp.paxs.end();)
        {
          if (suitable_ids.find(iPax->crs_pax_id_parent!=NoExists?iPax->crs_pax_id_parent:iPax->crs_pax_id)==suitable_ids.end())
            iPax=grp.paxs.erase(iPax);
          else
            ++iPax;
        };

        //���ᠦ�ࠬ ��ࢮ�� ᥣ���� ���⠢�� pax_no �� ���浪�
        int pax_no=1;
        for(TWebPax& pax : grp.paxs) pax.pax_no=pax_no++;
      }
      else
      {
        //䨫���㥬 ���ᠦ�஢ �� ��ண� � ᫥����� ᥣ���⮢
        for(vector<TWebPax>::iterator iPax=grp.paxs.begin();iPax!=grp.paxs.end();)
        {
          if (find(iPax+1, grp.paxs.end(), *iPax)!=grp.paxs.end())
          {
            //㤠��� �㡫�஢���� ���ᠦ�஢ � ࠬ��� ������� pnr �� grp
            remove(iPax+1, grp.paxs.end(), *iPax);
            iPax=grp.paxs.erase(iPax);
            continue;
          }

          vector<TWebPax>::const_iterator iPaxFirst=find(grpSegs.front().paxs.begin(),grpSegs.front().paxs.end(),*iPax);
          if (iPaxFirst==grpSegs.front().paxs.end())
          {
            //�� ��諨 ᮮ⢥�����饣� ���ᠦ�� �� ��ࢮ�� ᥣ����
            iPax=grp.paxs.erase(iPax);
            continue;
          };

//          if (!idsSeg.paxIdExists(iPax->crs_pax_id))
//          {
//            //�� ��襫 �� ���ᠦ�� � ᥪ樨 <crs_pax_ids> �� ᮮ⢥�����饬 ᥣ����
//            iPax=grp.paxs.erase(iPax);
//            continue;
//          }

          iPax->pax_no=iPaxFirst->pax_no; //���⠢�塞 ���ᠦ��� ᮮ�. ����㠫�� ��. �� ��ࢮ�� ᥣ����
          iPax++;
        };
      }
      if ( ids.showBPNotAllowedReason &&
           GetSelfCkinSets( tsEmergencyExitBPNotAllowed, grp.flt.oper, TReqInfo::Instance()->client_type ) ) {
        SEATPAX::GrpNotAllowedReason<TWebPax> grpNotAllowedReason;
        grpNotAllowedReason.boarding_pass( grp.flt.oper.point_id, grp.paxs );
      }
      grpSegs.push_back(grp);
    }
    catch(CheckIn::UserException &)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), point_id);
    };
  };
}

string TWebGrp::seat_status(const ASTRA::TCompLayerType& crs_seat_layer) const
{
  string result;
  if (flt.pr_paid_ckin)
  {
    switch(crs_seat_layer)
    {
      case cltPNLBeforePay:  result="PNLBeforePay";  break;
      case cltPNLAfterPay:   result="PNLAfterPay";   break;
      case cltProtBeforePay: result="ProtBeforePay"; break;
      case cltProtAfterPay:  result="ProtAfterPay";  break;
      default: break;
    };
  }
  if ( result.empty() && crs_seat_layer == cltProtSelfCkin ) {
     result="ProtSelfCkin";
  }
  return result;
}

void TWebGrpSegs::toXML(xmlNodePtr segsParentNode) const
{
  if (segsParentNode==nullptr) return;

  xmlNodePtr segsNode=NewTextChild(segsParentNode, "segments");
  for(const TWebGrp& grp : *this) grp.toXML(segsNode);
}

void TWebGrp::toXML(xmlNodePtr segParentNode) const
{
  if (segParentNode==nullptr) return;

  xmlNodePtr segNode=NewTextChild(segParentNode, "segment");
  flt.toXMLsimple(segNode, WebSearch::xmlSearchFltMulti);
  NewTextChild(segNode, "scd_out", flt.scd_out_local==NoExists?"":DateTimeToStr(flt.scd_out_local, ServerFormatDateTimeAsString));
  NewTextChild( segNode, "apis", (int)(!checkInfo.apis_formats().empty()) );
  checkInfo.pass().toWebXML(segNode);

  TRemGrp outputRemGrp;
  outputRemGrp.Load(retSELF_CKIN_EXCHANGE, flt.oper.airline);

  xmlNodePtr paxsNode = NewTextChild( segNode, "passengers" );
  for(const TWebPax& pax : paxs) pax.toXML(paxsNode, outputRemGrp);
}

void TWebPax::toXML(xmlNodePtr paxParentNode, const TRemGrp& outputRemGrp) const
{
  if (paxParentNode==nullptr) return;

  xmlNodePtr paxNode = NewTextChild( paxParentNode, "pax" );

  pnr_addrs.toWebXML(paxNode, AstraLocale::OutputLang());

  NewTextChild( paxNode, "pax_no", pax_no, NoExists );
  NewTextChild( paxNode, "crs_pnr_id", crs_pnr_id );
  NewTextChild( paxNode, "crs_pax_id", crs_pax_id );
  NewTextChild( paxNode, "crs_pax_id_parent", crs_pax_id_parent, NoExists );
  NewTextChild( paxNode, "reg_no", reg_no, NoExists );
  NewTextChild( paxNode, "surname", surname );
  NewTextChild( paxNode, "name", name );
  if ( doc.birth_date != NoExists )
    NewTextChild( paxNode, "birth_date", DateTimeToStr( doc.birth_date, ServerFormatDateTimeAsString ) );
  NewTextChild( paxNode, "pers_type", ElemIdToPrefferedElem(etExtendedPersType, pers_type_extended, efmtCodeNative, LANG_RU) ); //ElemIdToCodeNative �������� � ���饬
  NewTextChild( paxNode, "subclass", ElemIdToCodeNative(etSubcls, cabin_subclass) );
  NewTextChild( paxNode, "class", ElemIdToCodeNative(etClass, cabin_class) );
  string seat_no_view;
  if ( !seat_no.empty() )
    seat_no_view = seat_no;
  else if ( !crs_seat_no.empty() )
    seat_no_view = crs_seat_no;
  NewTextChild( paxNode, "seat_no", seat_no_view );
  NewTextChild( paxNode, "seat_status", seat_status );
  NewTextChild( paxNode, "seats", seats );
  NewTextChild( paxNode, "checkin_status", checkin_status );
  xmlNodePtr reasonsNode=NewTextChild( paxNode, "agent_checkin_reasons" );
  for(set<string>::const_iterator r=agent_checkin_reasons.begin();
                                  r!=agent_checkin_reasons.end(); ++r)
    NewTextChild(reasonsNode, "reason", *r);
  reasonsNode=NewTextChild( paxNode, "boarding_pass_not_allowed_reasons" );
  for(set<string>::const_iterator r=bp_not_allowed_reasons.begin();
                                  r!=bp_not_allowed_reasons.end(); ++r)
    NewTextChild(reasonsNode, "reason", *r);


  NewTextChild( paxNode, "eticket", tkn.rem == "TKNE"?"true":"false" );
  NewTextChild( paxNode, "ticket_no", tkn.no );

  doc.toWebXML(paxNode, AstraLocale::OutputLang());
  doco.toWebXML(paxNode, AstraLocale::OutputLang());

  xmlNodePtr fqtsNode = NewTextChild( paxNode, "fqt_rems" );
  for(const CheckIn::TPaxFQTItem& f : fqts)
    if (f.rem=="FQTV") f.toXML(fqtsNode, AstraLocale::OutputLang());

  brand.toWebXML( NewTextChild( paxNode, "brand" ), AstraLocale::OutputLang() );

  if (bagNorm)
  {
    xmlNodePtr bagNormNode=NewTextChild(paxNode, "bag_norm", bagNorm.get().getQuantity());
    if (!bagNorm.get().zero())
      SetProp(bagNormNode, "unit", TBagUnit(bagNorm.get().getUnit()).get_db_form());
  }
  else
    NewTextChild( paxNode, "bag_norm" );

  xmlNodePtr remsNode=nullptr;
  for(const CheckIn::TPaxRemItem& r : rems_and_asvc)
  {
    if (!TWebPaxFromReq::isRemProcessingAllowed(r) &&
        !outputRemGrp.exists(r.code)) continue;
    if (remsNode==nullptr) remsNode=NewTextChild(paxNode, "rems");
    r.toXML(remsNode);
  }

  context.toXML(NewTextChild(paxNode,"context"));

  xmlNodePtr tidsNode = NewTextChild( paxNode, "tids" );
  NewTextChild( tidsNode, "crs_pnr_tid", crs_pnr_tid );
  NewTextChild( tidsNode, "crs_pax_tid", crs_pax_tid );
  NewTextChild( tidsNode, "pax_grp_tid", pax_grp_tid, NoExists );
  NewTextChild( tidsNode, "pax_tid",     pax_tid,     NoExists );
}

void WebRequestsIface::LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  ProgTrace(TRACE1,"WebRequestsIface::LoadPnr");
  xmlNodePtr segsNode = NodeAsNode( "segments", reqNode );
  TIdsPnrDataSegs ids(reqNode);
  for(xmlNodePtr segNode=segsNode->children; segNode!=NULL; segNode=segNode->next)
  {
    int point_id=NodeAsInteger( "point_id", segNode );
    WebSearch::TFlightInfo flt;
    flt.fromDB(point_id, true);
    flt.fromDBadditional(false, true);
    TIdsPnrData idsPnrData(flt);
    if (TIdsPnrData::trueMultiRequest(reqNode))
      idsPnrData.fromXMLMulti(segNode);
    else
      idsPnrData.fromXML(segNode);
    ids.push_back( idsPnrData );
  };

  boost::optional<WebSearch::TPNRFilter> filter;
  bool charter_search=false;
  if (!TIdsPnrData::trueMultiRequest(reqNode))
  {
    charter_search=!ids.empty() &&
                   GetSelfCkinSets(tsSelfCkinCharterSearch, ids.front().flt.oper.point_id, TReqInfo::Instance()->client_type);
    if (charter_search)
    {
      xmlNodePtr searchParamsNode=GetNode("search_params", reqNode);
      if (searchParamsNode!=NULL)
      {
        filter=WebSearch::TPNRFilter();
        filter.get().fromXML(searchParamsNode, searchParamsNode);
      }
    };
  };

  TWebGrpSegs grpSegs;
  IntLoadPnr( ids, filter, grpSegs, false );
  if (charter_search && filter && !grpSegs.empty() && grpSegs.begin()->moreThanOnePersonWithSeat())
    throw UserException("MSG.CHARTER_SEARCH.FOUND_MORE.ADJUST_SEARCH_PARAMS");

  grpSegs.toXML(NewTextChild(resNode, (const char*)reqNode->name));
}

void WebRequestsIface::IntViewCraft(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  if ( SALONS2::isFreeSeating( point_id ) ) { //???
    throw UserException( "MSG.SALONS.FREE_SEATING" );
  }

  WebSearch::TFlightInfo flt;
  flt.fromDB(point_id, true);

  int pnr_id=TIdsPnrData(flt).fromXML(reqNode).getStrictlySinglePnrId();

  TWebGrp grp(flt);
  grp.addPnr(pnr_id, true, false );
  AstraWeb::WebCraft::ViewCraft( grp.paxs, reqNode, resNode );
}


void WebRequestsIface::ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::ViewCraft");
  emulateClientType();

  IntViewCraft( reqNode, resNode );
}

bool CreateEmulCkinDocForCHKD(int crs_pax_id,
                              TMultiPnrDataSegs& multiPnrDataSegs,
                              const XMLDoc &emulDocHeader,
                              XMLDoc &emulCkinDoc,
                              list<int> &crs_pax_ids)  //crs_pax_ids ����砥� �஬� crs_pax_id ��. �ਢ易���� ������楢
{
  crs_pax_ids.clear();
  if (multiPnrDataSegs.size()!=1)
    throw EXCEPTIONS::Exception("%s: multiPnrDataSegs.size()!=1", __FUNCTION__);

  TMultiPnrData& multiPnrData=multiPnrDataSegs.front();
  multiPnrData.segs.clear();
  TWebPaxForSaveSeg seg(multiPnrData.flt.oper.point_id);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT crs_pnr.pnr_id, "
    "       crs_pnr.airp_arv, "+
    CheckIn::TSimplePaxItem::origSubclassFromCrsSQL()+" AS subclass, "+
    CheckIn::TSimplePaxItem::origClassFromCrsSQL()+" AS class, "+
    CheckIn::TSimplePaxItem::cabinSubclassFromCrsSQL()+" AS cabin_subclass, "+
    CheckIn::TSimplePaxItem::cabinClassFromCrsSQL()+" AS cabin_class, "+
    "       NULL AS cabin_class_grp, "
    "       crs_pnr.status, "
    "       crs_pax.pax_id, "
    "       crs_pax.surname, "
    "       crs_pax.name, "
    "       crs_pax.pers_type, "
    "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
    "       crs_pax.seat_type, "
    "       crs_pax.seats, "
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

  map<int/*crs_pax_id*/, int/*reg_no*/> reg_no_map;
  for(;!Qry.Eof;Qry.Next())
  {
    TWebPaxFromReq paxFromReq;
    paxFromReq.fromDB(Qry);

    TWebPaxForCkin paxForCkin;
    paxForCkin.fromDB(Qry);

    if (paxForCkin.reg_no!=NoExists)
    {
      if (paxForCkin.reg_no<1 || paxForCkin.reg_no>999)
      {
        ostringstream reg_no_str;
        reg_no_str << setw(3) << setfill('0') << paxForCkin.reg_no;
        throw UserException("MSG.CHECKIN.REG_NO_NOT_SUPPORTED", LParams() << LParam("reg_no", reg_no_str.str()));

      };

      map<int, int>::iterator i=reg_no_map.insert( make_pair(paxForCkin.paxId(), paxForCkin.reg_no) ).first;
      if (i==reg_no_map.end()) throw EXCEPTIONS::Exception("%s: i==reg_no_map.end()", __FUNCTION__);
      if (i->second!=paxForCkin.reg_no)
        throw UserException("MSG.CHECKIN.DUPLICATE_CHKD_REG_NO");
    };

    seg.paxFromReq.push_back(paxFromReq);
    seg.paxForCkin.push_back(paxForCkin);
    multiPnrData.segs.add(multiPnrData.flt.oper, paxForCkin, true);
  };

  multiPnrData.checkJointCheckInAndComplete();

  int parent_reg_no=NoExists;
  map<int, int>::iterator i=reg_no_map.find(crs_pax_id);
  if (i!=reg_no_map.end()) parent_reg_no=i->second;
  if (parent_reg_no==NoExists) return false; //� த�⥫� ��� ॣ. ����� �� CHKD
  for(TWebPaxForCkinList::iterator p=seg.paxForCkin.begin(); p!=seg.paxForCkin.end(); ++p)
    if (p->reg_no==NoExists) p->reg_no=parent_reg_no;

  if (seg.paxForCkin.infantsMoreThanAdults())
    throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");

  CreateEmulDocs(TWebPaxForSaveSegs(seg),
                 multiPnrDataSegs,
                 emulDocHeader,
                 emulCkinDoc);

  for(TWebPaxForCkinList::const_iterator p=seg.paxForCkin.begin(); p!=seg.paxForCkin.end(); ++p)
    crs_pax_ids.push_back(p->paxId());

  return true;
};

static void VerifyPax(TWebPaxForSaveSegs &segs, const XMLDoc &emulDocHeader,
                      XMLDoc &emulCkinDoc, map<int,XMLDoc> &emulChngDocs, TIdsPnrDataSegs &ids)
{
  ids.clear();

  if (segs.empty()) return;

  //���� ����� �஢��塞, �� ����ॣ����஢���� ���ᠦ��� ᮢ������ �� ���-�� ��� ������� ᥣ����
  //�� ��᫥���� ᥣ����� ���-�� ����ॣ����஢����� ���ᠦ�஢ �.�. �㫥��
  int firstPointIdForCkin=NoExists;
  segs.checkSegmentsFromReq(firstPointIdForCkin);

  TQuery Qry(&OraSession);

  TMultiPnrDataSegs multiPnrDataSegs;
  boost::optional<bool> is_test=false;
  is_test=boost::none;

  for(TWebPaxForSaveSeg& s : segs)
  {
    s.paxForChng.clear();
    s.paxForCkin.clear();
    try
    {
      TMultiPnrData& multiPnrData=multiPnrDataSegs.add(s.point_id,
                                                       s.point_id==firstPointIdForCkin,
                                                       true);
      TIdsPnrData idsPnrData(multiPnrData.flt);

      if (s.paxFromReq.empty())
      {
        //���⮩ ᥣ����, ��� ���ᠦ�஢
        if (s.pnr_id!=ASTRA::NoExists)
          idsPnrData.add(TIdsPnrData::VerifyPNRByPnrId(s.point_id, s.pnr_id), boost::none);
      }
      else
      {
        multiPnrData.flt.isSelfCheckInPossible(s.point_id==firstPointIdForCkin,
                                               s.paxFromReq.notRefusalCount()>0,
                                               s.paxFromReq.refusalCount()>0);
      }

      for(const TWebPaxFromReq& paxFromReq : s.paxFromReq)
      {
        try
        {
          if (!is_test) is_test=paxFromReq.isTest();
          if (is_test.get()!=paxFromReq.isTest())
            throw EXCEPTIONS::Exception("Mixed test and real passengers");

          idsPnrData.add(TIdsPnrData::VerifyPNRByPaxId(s.point_id, paxFromReq.id), paxFromReq.id);

          Qry.Clear();
          if (!paxFromReq.checked())
            Qry.SQLText=TWebPaxForCkin::sql(paxFromReq.isTest());
          else
            Qry.SQLText=TWebPaxForChng::sql();
          Qry.CreateVariable("pax_id", otInteger, paxFromReq.id);
          Qry.Execute();

          if (!paxFromReq.checked())
          {
            //���ᠦ�� �� ��ॣ����஢��
            if (Qry.Eof)
              throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");

            TWebPaxForCkin pax;
            pax.fromDB(Qry);
            if (pax.checked())
              throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");
            if (pax.norec())
              throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

            if (!paxFromReq.refuse)
            {
              //�᫨ <refuse>1</refuse>, � ��祣� �� ������ � ���ᠦ�஬
              if (!is_valid_pnr_status(pax.status) ||
                  !is_valid_pax_status(s.point_id, pax.paxId()))
                throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

              pax.addFromReq(paxFromReq);

              s.paxForCkin.checkUniquenessAndAdd(pax);
              multiPnrData.segs.add(multiPnrData.flt.oper, pax, s.point_id==firstPointIdForCkin);
            }
          }
          else
          {
            //���ᠦ�� ��ॣ����஢��
            if (Qry.Eof)
              throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

            TWebPaxForChng pax;
            pax.fromDB(Qry);
            if (!pax.checked() || pax.norec())
              throw UserException("MSG.PASSENGER.CHANGED.REFRESH_DATA");

            pax.addFromReq(paxFromReq);

            s.paxForChng.checkUniquenessAndAdd(pax);
          }
        }
        catch(CheckIn::UserException &)
        {
          throw;
        }
        catch(UserException &e)
        {
          ProgTrace(TRACE5, ">>>> %s (point_id=%d, paxFromReq.id=%d)",
                    getLocaleText(e.getLexemaData()).c_str(), s.point_id, paxFromReq.id);
          throw CheckIn::UserException(e.getLexemaData(), s.point_id, paxFromReq.id);
        };

      };
      if (s.paxForCkin.infantsMoreThanAdults())
        throw UserException("MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");
      if (!multiPnrData.segs.empty())
        multiPnrData.checkJointCheckInAndComplete();
      else
        multiPnrDataSegs.pop_back();

      if (idsPnrData.containAtLeastOnePnrId())
        ids.push_back( idsPnrData );
    }
    catch(CheckIn::UserException &)
    {
      throw;
    }
    catch(UserException &e)
    {
      throw CheckIn::UserException(e.getLexemaData(), s.point_id);
    }
  }

  segs.checkAndSortPaxForCkin();

  if (!multiPnrDataSegs.empty())
  {
    const TMultiPnrData& firstMultiPnrData=multiPnrDataSegs.front();
    for(const auto& seg : firstMultiPnrData.segs)
    {
      vector<WebSearch::TPnrData> otherTCkinPNRs;
      WebSearch::getTCkinData(firstMultiPnrData.flt, firstMultiPnrData.dest, seg.second, is_test && is_test.get(), otherTCkinPNRs);

      vector<WebSearch::TPnrData>::const_iterator iPnrData=otherTCkinPNRs.begin();
      for(TMultiPnrDataSegs::iterator s=multiPnrDataSegs.begin(); s!=multiPnrDataSegs.end(); ++s)
      {
        try
        {
          if (s==multiPnrDataSegs.begin()) continue; //�ய�᪠�� ���� ᥣ����

          if (iPnrData==otherTCkinPNRs.end()) //��譨� ᥣ����� � ����� �� ॣ������
            throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
          if (iPnrData->flt.oper.point_id!=s->flt.oper.point_id) //��㣮� ३� �� ᪢����� ᥣ����
            throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");

          ++iPnrData;
        }
        catch(CheckIn::UserException &)
        {
          throw;
        }
        catch(UserException &e)
        {
          throw CheckIn::UserException(e.getLexemaData(), s->flt.oper.point_id);
        }
      }
    }

  }

  if (!(is_test && is_test.get()))
  {
    changeSeatsAndUpdateTids(segs);
    CreateEmulDocs(segs, multiPnrDataSegs, emulDocHeader, emulCkinDoc);
    CreateEmulDocs(segs, emulDocHeader, emulChngDocs);
  }
}

void WebRequestsIface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();
  SavePax(reqNode, NULL, resNode);
}

bool WebRequestsIface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE1,"WebRequestsIface::SavePax");

  OciCpp::Savepoint sp("sp_savepax");
  TWebPaxForSaveSegs segs;
  TFlights flightsForLock;
  xmlNodePtr segNode=NodeAsNode("segments", reqNode)->children;
  for(;segNode!=NULL;segNode=segNode->next)
  {
    if (string((const char*)segNode->name)!="segment") continue;

    TWebPaxForSaveSeg seg(NodeAsInteger("point_id", segNode),
                          NodeAsInteger("pnr_id", segNode, ASTRA::NoExists));
    xmlNodePtr paxNode=GetNode("passengers", segNode);
    if (paxNode!=NULL)
    {
      for(paxNode=paxNode->children; paxNode!=NULL; paxNode=paxNode->next)
      {
        if (string((const char*)paxNode->name)!="pax") continue;
        seg.paxFromReq.push_back(TWebPaxFromReq().fromXML(paxNode));
      }
    };

    segs.push_back(seg);
    flightsForLock.Get(seg.point_id, ftTranzit);
  };

  flightsForLock.Lock("WebRequestsIface::SavePax");

  XMLDoc emulDocHeader;
  CreateEmulXMLDoc(reqNode, emulDocHeader);

  XMLDoc emulCkinDoc;
  map<int,XMLDoc> emulChngDocs;
  TIdsPnrDataSegs ids(reqNode);
  VerifyPax(segs, emulDocHeader, emulCkinDoc, emulChngDocs, ids);

  TChangeStatusList ChangeStatusInfo;
  SirenaExchange::TLastExchangeList SirenaExchangeList;
  CheckIn::TAfterSaveInfoList AfterSaveInfoList;
  bool httpWasSent = false;
  bool result=true;
  bool handleAfterSave=false;
  //�����, �� ᭠砫� ��뢠���� CheckInInterface::SavePax ��� emulCkinDoc
  //⮫쪮 �� ���-ॣ����樨 ����� ��㯯� �������� ROLLBACK CHECKIN � SavePax �� ��ॣ�㧪�
  //� ᮮ⢥��⢥��� �����饭�� result=false
  //� ᮮ⢥��⢥��� �맮� ETStatusInterface::ETRollbackStatus ��� ���� ��

  if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (!CheckInInterface::SavePax(emulReqNode, ediResNode, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList, httpWasSent)) result=false;
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
      if (!CheckInInterface::SavePax(emulReqNode, ediResNode, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList, httpWasSent))
      {
        //�� ���� � �� ������� �� ������ �������� (�. �������਩ ���)
        //��� �⮣� ������� �� �����頥� false � ������ ᯥ樠���� ����� � SavePax:
        //�� ����� ��������� ��� � ���᪮� �� �⪠�뢠���� �� ��ॣ�㧪�
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CheckInInterface::SavePax=false");
      };
      handleAfterSave=true;
    };
  };

  if (result)
  {
    if (ediResNode==NULL && !ChangeStatusInfo.empty()) // needSyncEdsEts
    {
      //��� �� ���� ����� �㤥� ��ࠡ��뢠����
      sp.rollback();  //�⪠�
      ChangeStatusInterface::ChangeStatus(reqNode, ChangeStatusInfo);
      SirenaExchangeList.handle(__FUNCTION__);
      return false;
    }
    else
    {
      SirenaExchangeList.handle(__FUNCTION__);
    };

    if (handleAfterSave) {
      CheckIn::TAfterSaveInfoData afterSaveData(reqNode, ediResNode);
      AfterSaveInfoList.handle(afterSaveData, __FUNCTION__); //�᫨ ⮫쪮 ��������� ���� ���ᠦ��, � �� ��뢠��
    }

    boost::optional<WebSearch::TPNRFilter> filter;
    TWebGrpSegs grpSegs;
    IntLoadPnr( ids, filter, grpSegs, true );  //!!! ��� �� � ⮦� ��।����� filter �� �᭮�� <search_params>
    grpSegs.toXML(NewTextChild(resNode, (const char*)reqNode->name));
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
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(TDevOper::PrnBP));
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
    if (pax.pax_id==NoExists) {
      tst();
      throw UserException( "MSG.PASSENGER.NOT_FOUND" );
    }
    else
      throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  };
}

void GetBPPax(xmlNodePtr paxNode, bool is_test, PrintInterface::BPPax &pax)
{
  pax.clear();
  if (paxNode==NULL) throw EXCEPTIONS::Exception("GetBPPax: paxNode==NULL");
  xmlNodePtr node2=paxNode->children;
  int point_dep = NodeIsNULLFast( "point_id", node2, true)?
                  NoExists:
                  NodeAsIntegerFast( "point_id", node2 );
  int pax_id = NodeAsIntegerFast( "pax_id", node2 );

  if (is_test && point_dep==NoExists)
  {
    xmlNodePtr tidsNode = NodeAsNodeFast( "tids", node2 );
    node2=tidsNode->children;
    point_dep = NodeIsNULLFast( "pax_grp_tid", node2, true )?
                NoExists:
                NodeAsIntegerFast( "pax_grp_tid", node2 );
  }

  GetBPPax(point_dep, pax_id, is_test, pax);
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
    if (not scanSections.isBoardingPass())
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");
    //�஢�ਬ ����� ��� ��९���
    const BCBPRepeatedSections &repeated=*(scanSections.repeated.begin());
    BPReprintOptions::check_access(repeated.dateOfFlight(),
                                   repeated.fromCityAirpCode(),
                                   repeated.operatingCarrierDesignator());
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    //�� ����� ���� �������쭮 �஢���� �����
    LogTrace(TRACE5) << '\n' << scanSections;
    TReqInfo::Instance()->traceToMonitor(TRACE5, "GetBPPaxFromScanCode: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_DATA");
  }

  //����⪠ ���� ���ᠦ��
  try
  {
    filters.fromBCBPSections(scanSections);

    list<WebSearch::TPNRs> PNRsList;

    GetPNRsList(filters, PNRsList, pax.errors);
    if (!pax.errors.empty())
      throw UserException(pax.errors.begin()->lexema_id, pax.errors.begin()->lparams);
    //�஢�ਬ �� �� ��ᠤ��� ⠫�� � �� ���ᠦ�� ��⮢�
    if (filters.segs.empty()) throw EXCEPTIONS::Exception("%s: filters.segs.empty()", __FUNCTION__);
    if (filters.segs.front().reg_no==NoExists) //�� �� ��ᠤ��� ⠫��, ��⮬� �� ॣ. ����� �� �����⥭
      throw UserException("MSG.SCAN_CODE.NOT_SUITABLE_FOR_PRINTING_BOARDING_PASS");

    bool is_test=!filters.segs.front().test_paxs.empty();

    if (PNRsList.empty()) throw UserException( "MSG.PASSENGER.NOT_FOUND" );
    const WebSearch::TPNRs &PNRs=PNRsList.front();

    if (PNRs.pnrs.empty())
      throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
    if (!is_test && (PNRs.pnrs.size()>1 || PNRs.getFirstPNRInfo().paxs.size()>1))
      throw UserException( "MSG.PASSENGERS.FOUND_MORE" );
    int point_dep=PNRs.getFirstPNRInfo().getFirstPointDep();
    if (PNRs.getFirstPNRInfo().paxs.empty()) throw EXCEPTIONS::Exception("%s: PNRs.getFirstPNRInfo().paxs.empty()", __FUNCTION__);
    int pax_id=PNRs.getFirstPNRInfo().paxs.begin()->pax_id;
    GetBPPax( point_dep, pax_id, is_test, pax );
  }
  catch(UserException &e)
  {
    pax.errors.push_back(e.getLexemaData());
    LogTrace(TRACE5) << '\n' << scanSections;
    LogTrace(TRACE5) << ">>>> " << e.what();
  }
};

string GetBPGate(int point_id)
{
  tstations stations;
  stations.fromDBGates( point_id );
  if ( stations.size() == 1 ) {
    return stations.front().name;
  }
  return "";
}

void WebRequestsIface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

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
      GetBPPax( paxNode, false, pax );
      pax.time_print=NodeAsDateTime("prn_form_key", paxNode);
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };

  PrintInterface::ConfirmPrintBP(TDevOper::PrnBP, paxs, ue);  //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!

  NewTextChild( resNode, "ConfirmPrintBP" );
};

void WebRequestsIface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  ProgTrace(TRACE1,"WebRequestsIface::GetPrintDataBP");
  PrintInterface::BPParams params;
  params.fromXML(reqNode);

  TReqInfo *reqInfo = TReqInfo::Instance();
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
  Qry.CreateVariable("op_type", otString, DevOperTypes().encode(TDevOper::PrnBP));
  Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.Execute();
  if(Qry.Eof) throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_DESK");
  params.set_form_type(Qry.FieldAsString("bp_type"));
  LogTrace(TRACE5) << "bp_type: " << params.get_form_type();

  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  map<int/*point_dep*/, string/*gate*/> gates;

  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);
  if (scanCodeNode!=NULL)
  {
    reqInfo->user.access.set_total_permit();
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
      pax.from_scan_code = true;
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
        GetBPPax( paxNode, is_test, pax );
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
  PrintInterface::get_pectab(TDevOper::PrnBP, params, data, pectab);
  BIPrintRules::Holder bi_rules(TDevOper::PrnBP);
  boost::optional<AstraLocale::LexemaData> error;
  PrintInterface::GetPrintDataBP(TDevOper::PrnBP, params, data, bi_rules, paxs, error);
  // ���� ��-� ������ � error !!!

  xmlNodePtr BPNode = NewTextChild( resNode, "GetPrintDataBP" );
  NewTextChild(BPNode, "pectab", pectab);
  xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
  for (vector<PrintInterface::BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
  {
    xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id", iPax->pax_id==NoExists?getEmptyPaxId():iPax->pax_id);
    if (!iPax->hex && params.get_prn_params().encoding!="UTF-8")
    {
      iPax->prn_form = ConvertCodepage(iPax->prn_form, "CP866", params.get_prn_params().encoding);
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
    // ��� ���樠����樨 reqInfo ��� �� ��������
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("���");

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
        parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(TDevOper::PrnBP, scan));
    } else {
        cout << "pax found, pax_id: " << pax.pax_id << endl;
        parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(TDevOper::PrnBP, pax.grp_id, pax.pax_id, false, 0, NULL));
    }
    cout << endl;

    cout << parser->parse(pectab) << endl;

    return 1;
}

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  ProgTrace(TRACE1,"WebRequestsIface::GetBPTags");

  TReqInfo *reqInfo = TReqInfo::Instance();

  PrintInterface::BPPax pax;
  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);
  boost::shared_ptr<PrintDataParser> parser;
  if (scanCodeNode!=NULL)
  {
    reqInfo->user.access.set_total_permit();
    string scanCode=NodeAsString(scanCodeNode);
    GetBPPaxFromScanCode(scanCode, pax);

    if(pax.pax_id == NoExists)
      parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(TDevOper::PrnBP, scanCode));
    else
      parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(TDevOper::PrnBP, pax.grp_id, pax.pax_id, false, 0, NULL));
  }
  else
  {
    bool is_test=isTestPaxId(NodeAsInteger("pax_id", reqNode));
    GetBPPax( reqNode, is_test, pax );
    pax.checkBPPrintAllowed();
    checkBeforePrintBP(pax);
    parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(TDevOper::PrnBP, pax.grp_id, pax.pax_id, false, 0, NULL));
  };
  vector<string> tags;
  BPTags::Instance()->getFields( tags );
  parser->pts.get_pectab_tags(tags);
  xmlNodePtr node = NewTextChild( resNode, "GetBPTags" );
  for ( vector<string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
    for(int j = 0; j < 2; j++) {
      string value = parser->pts.get_tag(*i, ServerFormatDateTimeAsString, ((j == 0 and not scanCodeNode) ? "R" : "E"));
      NewTextChild( node, (*i + (j == 0 ? "" : "_lat")).c_str(), value );
      ProgTrace( TRACE5, "field name=%s, value=%s", (*i + (j == 0 ? "" : "_lat")).c_str(), value.c_str() );
    }
  }
  parser->pts.confirm_print(true, TDevOper::PrnBP);

  string gate=GetBPGate(pax.point_dep);
  if (!gate.empty())
    NewTextChild( node, "gate", gate );
}

static void changeLayer(const ProtLayerRequest::SegList& segListReq,
                        ProtLayerResponse::SegList& segListRes)
{
  segListRes.clear();
  segListRes.layer_type=segListReq.layer_type;

  for(const ProtLayerRequest::PaxList& paxListReq : segListReq)
  {
    segListRes.emplace_back();
    ProtLayerResponse::PaxList& paxListRes=segListRes.back();
    try
    {
      paxListRes.point_id=paxListReq.point_id;
      if (segListReq.isLayerAdded())
      {
        paxListRes.checkFlight();
        paxListRes.complete(segListReq);
      }

      for(const ProtLayerRequest::Pax& paxReq : paxListReq)
      {
        paxListRes.emplace_back(paxReq);
        ProtLayerResponse::Pax& paxRes=paxListRes.back();
        try
        {
          if (!paxRes.fromDB())
            throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
          if (paxRes.checked())
            throw UserException("MSG.PASSENGER.CHECKED.REFRESH_DATA");

          if (segListReq.isLayerAdded())
          {
            TIdsPnrData::VerifyPNRByPaxId(paxListRes.point_id, paxRes.id);

            if (!is_valid_pnr_status(paxRes.pnr_status) ||
                !is_valid_pax_status(paxListRes.point_id, paxRes.id))
              throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

            if (paxRes.seat_no.empty())
            {
              if (paxRes.seats>0)
                throw EXCEPTIONS::Exception("%s: empty seat_no (crs_pax_id=%d)", __FUNCTION__, paxRes.id);
              paxListRes.pop_back(); //������楢 ��� ���� �모�뢠��, �� ��ࠡ��뢠��
              continue;
            }
          }
        }
        catch(UserException &e)
        {
          segListRes.ue.addError(e.getLexemaData(), paxListRes.point_id, paxRes.id);
          paxRes.userException=e;
        };
      };

      vector< pair<WebCraft::TWebPlace, LexemaData> > pax_seats;
      if (!paxListRes.empty())
      {
        TPointIdsForCheck point_ids_spp;
        if (segListReq.isLayerAdded())
        {
          vector<TWebPax> webPaxs;
          for(const ProtLayerResponse::Pax& paxRes : paxListRes)
          {
            if (paxRes.userException) continue;
            webPaxs.emplace_back(paxRes);
          }
          WebCraft::GetCrsPaxSeats(paxListRes.point_id, webPaxs, pax_seats );
          BitSet<TChangeLayerFlags> change_layer_flags;
          change_layer_flags.setFlag(flSetPayLayer);
          for(int pass=0; pass<2; pass++)
            for(ProtLayerResponse::Pax& paxRes : paxListRes)
            {
              if (paxRes.userException) continue;
              try
              {
                int tid = paxRes.crs_pax_tid;

                if (pass==0)
                {
                  if (isTestPaxId(paxRes.id)) continue;

                  DeleteTlgSeatRanges( {cltProtBeforePay, cltProtSelfCkin}, paxRes.id, tid, point_ids_spp );
                }
                else
                {
                  vector< pair<WebCraft::TWebPlace, LexemaData> >::const_iterator iSeat=pax_seats.begin();
                  for(;iSeat!=pax_seats.end();iSeat++)
                    if (iSeat->first.getPaxId()==paxRes.id &&
                        iSeat->first.getSeatNo()==paxRes.seat_no) break;
                  if (iSeat==pax_seats.end())
                    throw EXCEPTIONS::Exception("%s: passenger not found in pax_seats (crs_pax_id=%d, crs_seat_no=%s)",
                                                __FUNCTION__, paxRes.id, paxRes.seat_no.c_str());
                  if (!iSeat->second.lexema_id.empty())
                    throw UserException(iSeat->second.lexema_id, iSeat->second.lparams);

                  paxRes.seatTariff=iSeat->first.getTariff();

//                  if ( iSeat->first.SeatTariff.empty() )  //��� ���
//                    throw UserException("MSG.SEATS.NOT_SET_RATE");

                  if (isTestPaxId(paxRes.id)) continue;

                  IntChangeSeatsN( paxListRes.point_id,
                                   paxRes.id,
                                   tid,
                                   iSeat->first.getXName(),
                                   iSeat->first.getYName(),
                                   stSeat,
                                   segListRes.layer_type,
                                   paxListRes.time_limit,
                                   change_layer_flags,
                                   0,
                                   NoExists,
                                   NULL, NULL,
                                   __func__ );
                }
              }
              catch(UserException &e)
              {
                segListRes.ue.addError(e.getLexemaData(), paxListRes.point_id, paxRes.id);
                paxRes.userException=e;
              };
            };
        }
        else
        {
          //RemoveProtLayer
          for(ProtLayerResponse::Pax& paxRes : paxListRes)
          {
            if (paxRes.userException) continue;
            try
            {
              if (isTestPaxId(paxRes.id)) continue;

              LogTrace(TRACE5) << __FUNCTION__
                               << ": segListRes.layer_type=" << EncodeCompLayerType(segListRes.layer_type)
                               << ", paxRes.id=" << paxRes.id;
              DeleteTlgSeatRanges({segListRes.layer_type}, paxRes.id, segListRes.curr_tid, point_ids_spp);
            }
            catch(UserException &e)
            {
              segListRes.ue.addError(e.getLexemaData(), paxListRes.point_id, paxRes.id);
              paxRes.userException=e;
            };
          };
          check_layer_change(point_ids_spp, segListReq.getRequestName() + "::" + __FUNCTION__);
        };
      }; //!paxListRes.empty()
    }
    catch(UserException &e)
    {
      segListRes.pop_back();
      segListRes.ue.addError(e.getLexemaData(), paxListReq.point_id);
    }
  };
}

void WebRequestsIface::ChangeProtLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  ProtLayerRequest::SegList segListReq;
  ProtLayerResponse::SegList segListRes;

  segListReq.fromXML(reqNode);

  segListReq.lockFlights();  //��窠 ३ᮢ

  changeLayer(segListReq, segListRes);

  if (!segListRes.ue.empty()) throw segListRes.ue;

  segListRes.toXML(segListReq.isLayerAdded(),
                   NewTextChild(resNode, segListReq.getRequestName().c_str()));
}

void WebRequestsIface::ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SysReqInterface::ErrorToLog(ctxt, reqNode, resNode);
  NewTextChild(resNode, "ClientError");
}

static void changeStatus(const PaymentStatusRequest::PaxList& paxListReq,
                         PaymentStatusResponse::PaxList& paxListRes);

void WebRequestsIface::PaymentStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  for(int attempts=1; attempts<=3; ++attempts)
  try
  {
    PaymentStatusRequest::PaxList paxListReq;
    PaymentStatusResponse::PaxList paxListRes;

    paxListReq.fromXML(reqNode);

    paxListReq.lockFlights();  //��窠 ३ᮢ

    changeStatus(paxListReq, paxListRes);

    paxListRes.toXML(NewTextChild(resNode, paxListReq.getRequestName().c_str()));
    break;
  }
  catch(const std::exception& e)
  {
    if (attempts>2) throw;
    if (!clearResponseAndRollbackIfDeadlock(e, resNode)) throw;
  }
}


static void changeStatus(const PaymentStatusRequest::PaxList& paxListReq,
                         PaymentStatusResponse::PaxList& paxListRes)
{
  TPointIdsForCheck point_ids_spp;
  for(const PaymentStatusRequest::Pax& paxReq : paxListReq)
  {
    PaymentStatusResponse::Pax paxRes(paxReq);
    if (!paxRes.fromDB()) continue;

    paxRes.okStatus=(paxReq.status==PaymentStatusRequest::NotPaid);
    if ( paxReq.status==PaymentStatusRequest::Paid &&
         !paxRes.checked() ) { // �����塞 ᫮� ������ � cltProtBeforePay �� cltProtAfterPay || cltProtSelfCkin �� cltProtAfterPay
      //��।��塞 ���न���� ���� ���� ��। ����⮩
      TSeatRanges ranges;
      for ( int i=0; i<2; i++ ) {
        TCompLayerType layer_type;
        switch( i ) {
          case 0:
            layer_type = ASTRA::cltProtBeforePay;
            break;
          case 1:
            layer_type = ASTRA::cltProtSelfCkin;
            break;
        }
        GetTlgSeatRanges(layer_type, paxRes.id, ranges);
        TSeatRanges::const_iterator r=ranges.begin();
        for(; r!=ranges.end(); ++r)
        {
          const TSeatRange& seatRange=*r;
          if ( paxRes.seat_no == seatRange.first.denorm_view(true) ||
               paxRes.seat_no == seatRange.first.denorm_view(false) ) break;
        };
        if (r!=ranges.end())
        {
          paxRes.okStatus=true;
          break;
        }
      }

      if ( paxRes.okStatus )
      {
        int curr_tid = paxRes.crs_pax_tid;
        InsertTlgSeatRanges(paxRes.point_id_tlg,
                            paxRes.airp_arv,
                            cltProtAfterPay,
                            ranges,
                            paxRes.id,
                            NoExists,NoExists,false,curr_tid,point_ids_spp);
        DeleteTlgSeatRanges({cltProtBeforePay, cltProtSelfCkin}, paxRes.id, curr_tid, point_ids_spp);
      }
    }

    LogTrace(TRACE5) << __FUNCTION__
                     << ": paxReq.id=" << paxReq.id
                     << ", paxReq.status=" << paxReq.getStatus()
                     << ", paxRes.checked=" << boolalpha << paxRes.checked()
                     << ", paxRes.okStatus=" << boolalpha << paxRes.okStatus;

    paxRes.toLog(paxReq.status);

    paxListRes.push_back(paxRes);
  }
  check_layer_change(point_ids_spp, __FUNCTION__);
}

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

void WebRequestsIface::CheckFFP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr ffpsNode = GetNode( "ffps", reqNode );
  if ( ffpsNode == NULL ) {
    throw EXCEPTIONS::Exception( "tag 'ffps' not found" );
  }
  xmlNodePtr reqffpNode = ffpsNode->children;
  if ( reqffpNode == NULL || string("ffp") != (const char*)reqffpNode->name ) {
    throw EXCEPTIONS::Exception( "tag 'ffp' not found" );
  }
  ffpsNode = NewTextChild( resNode, "ffps" );
  while ( reqffpNode!=NULL && string("ffp") == (const char*)reqffpNode->name ) {
    xmlNodePtr node = reqffpNode->children;

    PaxId_t paxId(NodeAsIntegerFast( "crs_pax_id", node ));

    TElemFmt fmt;
    std::string airline = ElemToElemId( etAirline, NodeAsStringFast( "airline", node ), fmt );
    if (fmt==efmtUnknown)
      throw UserException("MSG.AIRLINE_CODE_NOT_FOUND", LParams() << LParam("airline", NodeAsStringFast( "airline", node )));

    std::string card_number=NodeAsStringFast( "card_number", node );

    CheckIn::Search search;
    CheckIn::TSimplePaxList paxs;
    search(paxs, PaxIdFilter(paxId));
    if (paxs.empty()) throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA" );

    const CheckIn::TSimplePaxItem& pax=paxs.front();
    CheckIn::TPaxDocItem doc;
    if (pax.origin()==paxPnl)
      CheckIn::LoadCrsPaxDoc(pax.id, doc);
    else
      CheckIn::LoadPaxDoc(pax.id, doc);

    SirenaExchange::TFFPInfoReq req;
    SirenaExchange::TFFPInfoRes res;
    req.set(airline, card_number, pax.surname, pax.name, doc.birth_date);

    get_ffp_status(req, res);

    //�ନ�㥬 ᥪ�� � �⢥�
    xmlNodePtr resFfpNode = NewTextChild( ffpsNode, "ffp" );
    NewTextChild( resFfpNode, "crs_pax_id", paxId.get() );
    NewTextChild( resFfpNode, "airline", res.error()?req.company:res.company );
    NewTextChild( resFfpNode, "card_number", res.error()?req.card_number:res.card_number );
    NewTextChild( resFfpNode, "tier_level", res.status );
    NewTextChild( resFfpNode, "error_message", res.error_message );
    reqffpNode = reqffpNode->next;
  }
}

void WebRequestsIface::ParseMessage(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw EXCEPTIONS::Exception("not implemented. insert into ssm_in");
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

void WebRequestsIface::ManagePaxContexts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  emulateClientType();

  ManagePaxContexts::PaxList paxListReq;
  paxListReq.fromXML(reqNode);
  paxListReq.toDB();

  NewTextChild(NewTextChild(resNode, "ManagePaxContexts"), "result", "ok");
}

} //end namespace AstraWeb

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
          // ~Void(){clear(); ASTRA::rollback();};

           int grp_id;
           string desk_code;

//	   Void(string _code, int _id) : grp_id(_id), desk_code(_code) {}
    };
    std::cout<<"Init...\n"; std::cout.flush();
    Void query;
   // TReqInfo::Instance()->desk.grp_id=query.grp_id;
    //TReqInfo::Instance()->desk.code=query.desk_code;
    vector<string> airps = {"VKO", "DME", "", " ", "0000", "000"}; //��᫥���� � �ਭ樯� �����⨬�� ��� ��ய���/������������ ������ ���� ����ᠭ� �� ""
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

void SyncCHKD(int point_id_tlg, int point_id_spp, bool sync_all) //ॣ������ CHKD
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  ASTRA::TClientType prior_client_type=reqInfo->client_type;
  TUser prior_user=reqInfo->user;
  try
  {
    const int maxDelaySecs=5;
    time_t start_time=time(NULL);
    int processed=0;

    reqInfo->client_type=ctPNL;
    reqInfo->user.user_type=utSupport;
    reqInfo->user.access.set_total_permit();

    TQuery UpdQry(&OraSession);
    if (sync_all)
    {
      UpdQry.SQLText=
        "UPDATE crs_pax SET sync_chkd=1 "
        "WHERE pax_id IN "
        "(SELECT crs_pax.pax_id "
        " FROM crs_pnr, crs_pax, crs_pax_chkd"
        " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "       crs_pax.pax_id=crs_pax_chkd.pax_id AND "
        "       crs_pnr.point_id=:point_id_tlg AND "
        "       crs_pnr.system='CRS' AND "
        "       crs_pax.pr_del=0 AND "
        "       crs_pax.sync_chkd=0) ";
      UpdQry.CreateVariable("point_id_tlg", otInteger, point_id_tlg);
      UpdQry.Execute();
    }

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
      "      crs_pax.pr_del=0 AND crs_pax.seats>0 AND "
      "      crs_pax.sync_chkd<>0 "
      "GROUP BY crs_pax.pax_id, pax.pax_id "
      "ORDER BY reg_no, surname, name, crs_pax_id "; //�஡㥬 ��ࠡ��뢠�� � ���浪� ॣ����樮���� ����஢ - ⠪ �ࠢ�������
    Qry.CreateVariable("point_id_tlg", otInteger, point_id_tlg);
    Qry.Execute();
    if (!Qry.Eof)
    {
      TFlights flightsForLock;
      flightsForLock.Get( point_id_spp, ftTranzit );
      flightsForLock.Lock(__FUNCTION__);

      TMultiPnrDataSegs multiPnrDataSegs;
      multiPnrDataSegs.add(point_id_spp, true, true);
      for(;!Qry.Eof;Qry.Next())
      {
        int crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
        list<int> crs_pax_ids;
        if (!Qry.FieldIsNULL("pax_id"))
          //���ᠦ�� ��ॣ����஢��
          crs_pax_ids.push_back(crs_pax_id);
        else
        {
          OciCpp::Savepoint spSyncCHKD("CHKD");
          try
          {
            XMLDoc emulDocHeader;
            CreateEmulXMLDoc(emulDocHeader);
            XMLDoc emulCkinDoc;
            if (!AstraWeb::CreateEmulCkinDocForCHKD(crs_pax_id,
                                                    multiPnrDataSegs,
                                                    emulDocHeader,
                                                    emulCkinDoc,
                                                    crs_pax_ids)) continue;

            if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
            {
              ProgTrace(TRACE5, "SyncCHKD:\n%s", XMLTreeToText(emulCkinDoc.docPtr()).c_str());

              xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
              if (emulReqNode==NULL)
                throw EXCEPTIONS::Exception("emulReqNode=NULL");

              TChangeStatusList ChangeStatusInfo;
              SirenaExchange::TLastExchangeList SirenaExchangeList;
              CheckIn::TAfterSaveInfoList AfterSaveInfoList;
              bool httpWasSent = false;
              if (CheckInInterface::SavePax(emulReqNode, NULL/*ediResNode*/, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList, httpWasSent))
              {
                //� �������� �᫨ �뫠 ॠ�쭠� ॣ������
                CheckIn::TAfterSaveInfoData afterSaveData(emulReqNode, NULL/*ediResNode*/);
                SirenaExchangeList.handle(__FUNCTION__);
                AfterSaveInfoList.handle(afterSaveData, __FUNCTION__);
              };
            };
          }
          catch(const AstraLocale::UserException &e)
          {
            try
            {
              dynamic_cast<const CheckIn::OverloadException&>(e);
            }
            catch (const bad_cast&)
            {
              spSyncCHKD.rollback();
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
          catch(const ServerFramework::Exception &e)
          {
            spSyncCHKD.rollback();
            ProgError(STDLOG, "TypeB::SyncCHKDPax (crs_pax_id=%d): %s", crs_pax_id, e.what());
          };
        };

        for(list<int>::const_iterator i=crs_pax_ids.begin(); i!=crs_pax_ids.end(); ++i)
        {
          UpdQry.SetVariable("pax_id", *i);
          UpdQry.Execute();
        };

        processed+=crs_pax_ids.size();

        int delaySecs=time(NULL)-start_time;
        if (delaySecs>=maxDelaySecs && processed>0)
        {
          ProgTrace(TRACE5, "%s: delaySecs=%d processed=%d", __func__, delaySecs, processed);
          add_trip_task(point_id_spp, SYNC_NEW_CHKD, "");
          break;
        }
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

void SyncNewCHKD(const TTripTaskKey &task)
{
  SyncCHKD(task.point_id, false);
};

void SyncAllCHKD(const TTripTaskKey &task)
{
  SyncCHKD(task.point_id, true);
};

} //namespace TypeB

namespace SirenaExchange
{

void fillProtBeforePaySvcs(const TAdvTripInfo &operFlt,
                           const int pax_id,
                           TExchange &exch)
{
  TPseudoGroupInfoRes *pseudoGroupInfoRes=dynamic_cast<TPseudoGroupInfoRes*>(&exch);
  if (!pseudoGroupInfoRes) return;

  int point_id=operFlt.point_id;
  if (!SALONS2::isFreeSeating(point_id) &&
      !SALONS2::isEmptySalons(point_id))
  {
    //������� RFISC ���������� � ����� ����, ����� ࠧ��祭 �� ����� BeforePay, �᫨ BeforePay ᠬ� �ਮ���� ᫮� ���ᠦ��
    TReqInfo *reqInfo = TReqInfo::Instance();
    ASTRA::TClientType prior_client_type=reqInfo->client_type;
    try
    {
      reqInfo->client_type=ctWeb;
      int point_arv = SALONS2::getCrsPaxPointArv( pax_id, point_id );
      TSalonList salonList;
      salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), "", ASTRA::NoExists );
      ProgTrace(TRACE5, "%s: salonList.ReadFlight (point_dep=%d, point_arv=%d)", __FUNCTION__, point_id, point_arv);
      if (salonList.getRFISCMode()==rRFISC)
      {
        std::set<TPlace*,CompareSeats> seats, seatsTmp;
        for(const TCompLayerType layer_type : {cltProtBeforePay, cltProtSelfCkin})
        {
          salonList.getPaxLayer( point_id, pax_id, layer_type, seatsTmp );
          seats.insert(seatsTmp.begin(), seatsTmp.end());
        }

          //���ᨢ ���� � RFISC���
        vector< pair<TSeat, TRFISC> > seats_with_rfisc;
        for(std::set<TPlace*,CompareSeats>::const_iterator s=seats.begin(); s!=seats.end(); ++s)
        {
          //横� �� ���⠬
          const TPlace &place=**s;

          TRFISC rfisc=place.getRFISC(point_id);
          if (rfisc.empty() || rfisc.code.empty())
          {
            ProgTrace(TRACE5, "%s: pax_id=%d, seat_no=%s, rfisc.empty() || rfisc.code.empty()",
                      __FUNCTION__, pax_id, place.denorm_view(salonList.isCraftLat()).c_str());
            continue;
          };

          ProgTrace(TRACE5, "%s: pax_id=%d, seat_no=%s, rfisc.code=%s",
                    __FUNCTION__, pax_id, place.denorm_view(salonList.isCraftLat()).c_str(), rfisc.code.c_str());

          seats_with_rfisc.push_back(make_pair(place.getTSeat(), rfisc));
        };

        if (!seats_with_rfisc.empty())
        {
          //��㧨� ������⢮ ����祭��� ����
          TSeatRanges ranges;
          GetTlgSeatRanges(cltProtAfterPay, pax_id, ranges);
          if (!ranges.empty())
            ProgTrace(TRACE5, "%s: pax_id=%d, layer_type=%s, ranges: %s",
                      __FUNCTION__, pax_id, EncodeCompLayerType(cltProtAfterPay), ranges.traceStr().c_str());

          if (ranges.empty())
          {
            GetTlgSeatRanges(cltPNLAfterPay, pax_id, ranges);
            if (!ranges.empty())
              ProgTrace(TRACE5, "%s: pax_id=%d, layer_type=%s, ranges: %s",
                        __FUNCTION__, pax_id, EncodeCompLayerType(cltPNLAfterPay), ranges.traceStr().c_str());
          }
          //��㧨� ������⢮ ����祭��� EMD
          vector<CheckIn::TPaxASVCItem> asvc;
          CheckIn::LoadCrsPaxASVC(pax_id, asvc);

          for(vector< pair<TSeat, TRFISC> >::const_iterator i=seats_with_rfisc.begin(); i!=seats_with_rfisc.end(); ++i)
          {
            const TSeat& seat=i->first;
            const TRFISC& rfisc=i->second;

            if (ranges.empty() ||
                (!ranges.empty() && ranges.contains(seat)))
            {
              //�஢���� ᮮ⢥�����騩 RFISC
              bool rfisc_found=false;
              for(vector<CheckIn::TPaxASVCItem>::iterator e=asvc.begin(); e!=asvc.end(); ++e)
                if (e->RFISC==rfisc.code)
                {
                  rfisc_found=true;
                  ProgTrace(TRACE5, "%s: pax_id=%d, seat_no=%s, rfisc.code=%s, %s",
                            __FUNCTION__, pax_id, seat.denorm_view(salonList.isCraftLat()).c_str(),
                            rfisc.code.c_str(),
                            e->rem_text(false, LANG_EN, applyLang).c_str());
                  asvc.erase(e);
                  break;
                };
              if (rfisc_found) continue;
            };

            TRFISCKey RFISCKey;
            RFISCKey.RFISC=rfisc.code;
            RFISCKey.service_type=TServiceType::FlightRelated;
            RFISCKey.airline=operFlt.airline;
            TSvcItem item(TPaxSegRFISCKey(Sirena::TPaxSegKey(pax_id, 0), RFISCKey), TServiceStatus::Need);
            item.ssr_code="SEAT";
            item.ssr_text=seat.denorm_view(salonList.isCraftLat());
            item.getListItemLastSimilar();
            pseudoGroupInfoRes->svcs.push_back(item);
          }
        }
      };
      reqInfo->client_type=prior_client_type;
    }
    catch(...)
    {
      reqInfo->client_type=prior_client_type;
      throw;
    };
  };
}

void fillPaxsSvcs(const TNotCheckedReqPassengers &req_pnrs, TExchange &exch)
{
  TPaxSection *paxSection=dynamic_cast<TPaxSection*>(&exch);
  TSvcSection *svcSection=dynamic_cast<TSvcSection*>(&exch);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT crs_pnr.airp_arv, "
    "       crs_pax.pax_id, crs_pax.surname, crs_pax.name, crs_pax.pers_type, crs_pax.seats "
    "FROM crs_pax, crs_pnr "
    "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
    "      crs_pax.pnr_id=:pnr_id AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pr_del=0 ";
  Qry.DeclareVariable("pnr_id", otInteger);

  for(TNotCheckedReqPassengers::const_iterator iReqPnr=req_pnrs.begin(); iReqPnr!=req_pnrs.end(); ++iReqPnr)
  {
    int pnr_id=iReqPnr->first;
    boost::optional<TAdvTripInfo> operFlt=boost::none;
    TAdvTripInfoList flts;
    getTripsByCRSPnrId(pnr_id, flts);
    for(TAdvTripInfoList::const_iterator f=flts.begin(); f!=flts.end(); ++f)
    {
      if (f->pr_del!=0) continue;
      operFlt=*f;
      break;
    };
    if (!operFlt) continue;

    Qry.SetVariable("pnr_id", pnr_id);
    Qry.Execute();
    if (Qry.Eof) continue;

    string airp_arv=Qry.FieldAsString("airp_arv");

    std::list<TPaxItem> paxs;
    for(;!Qry.Eof;Qry.Next())
    {
      CheckIn::TPaxItem pax;
      pax.id=Qry.FieldAsInteger("pax_id");
      pax.surname=Qry.FieldAsString("surname");
      pax.name=Qry.FieldAsString("name");
      pax.pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
      pax.seats=Qry.FieldAsInteger("seats");
      if (!req_pnrs.pax_included(pnr_id, pax.id)) continue;

      CheckIn::LoadCrsPaxDoc(pax.id, pax.doc);
      CheckIn::LoadCrsPaxTkn(pax.id, pax.tkn);

      TETickItem etick;
      if (pax.tkn.validET())
        etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);

      list<TPaxItem>::iterator iReqPax=paxs.insert(paxs.end(), TPaxItem());
      if (iReqPax==paxs.end()) throw EXCEPTIONS::Exception("%s: strange situation iReqPax==paxs.end()");
      TPaxItem &reqPax=*iReqPax;

      reqPax.set(pax, etick);

      SirenaExchange::TPaxSegMap::iterator iReqSeg=
          reqPax.segs.insert(make_pair(0,SirenaExchange::TPaxSegItem())).first;
      if (iReqSeg==reqPax.segs.end()) throw EXCEPTIONS::Exception("%s: strange situation iReqSeg==reqPax.segs.end()");
      SirenaExchange::TPaxSegItem &reqSeg=iReqSeg->second;
      TMktFlight mktFlight;
      mktFlight.getByCrsPaxId(pax.id);
      reqSeg.set(0, operFlt.get(), airp_arv, mktFlight, operFlt.get().get_scd_in(airp_arv));
      reqSeg.subcl=mktFlight.subcls;
      reqSeg.setTicket(pax.tkn, paxSection);
      CheckIn::LoadPaxFQT(pax.id, reqSeg.fqts);
      reqSeg.pnrAddrs.getByPaxIdFast(pax.id);

      if (svcSection && req_pnrs.include_unbound_svcs)
      {
        svcSection->svcs.addFromCrs(req_pnrs, pnr_id, pax.id);
        fillProtBeforePaySvcs(operFlt.get(), pax.id, exch);
      };
    };
    if (!paxs.empty() && paxSection)
    {
      std::list<TPaxItem> &paxs_ref=paxSection->paxs;
      paxs_ref.insert(paxs_ref.end(), paxs.begin(), paxs.end());
    }
  }

  if (paxSection) paxSection->addMissingTickets();
}

void fillPaxsSvcs(const TEntityList &entities, TExchange &exch)
{
  TCheckedReqPassengers req_grps(false, true, true);
  TNotCheckedReqPassengers req_pnrs(false, true, true);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT pax.grp_id, pax.pax_id, crs_pax.pnr_id AS crs_pnr_id, crs_pax.pax_id AS crs_pax_id "
    "FROM (SELECT grp_id, pax_id FROM pax WHERE pax_id=:pax_id) pax "
    "     FULL OUTER JOIN "
    "     (SELECT pnr_id, pax_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0) crs_pax "
    "ON pax.pax_id=crs_pax.pax_id ";
  Qry.DeclareVariable("pax_id", otInteger);
  for(TEntityList::const_iterator i=entities.begin(); i!=entities.end(); ++i)
  {
    const Sirena::TPaxSegKey &entity=i->first;
    ProgTrace(TRACE5, "%s: entity.pax_id=%d, entity.trfer_num=%d", __FUNCTION__, entity.pax_id, entity.trfer_num);

    Qry.SetVariable("pax_id", entity.pax_id);
    Qry.Execute();
    if (Qry.Eof) continue;

    if (!Qry.FieldIsNULL("grp_id") && !Qry.FieldIsNULL("pax_id"))
    {
      req_grps.add(Qry.FieldAsInteger("grp_id"), Qry.FieldAsInteger("pax_id"));
    }
    else if (!Qry.FieldIsNULL("crs_pnr_id") && !Qry.FieldIsNULL("crs_pax_id"))
    {
      req_pnrs.add(Qry.FieldAsInteger("crs_pnr_id"), Qry.FieldAsInteger("crs_pax_id"));
    };
  };

  TCheckedResPassengers res_grps;
  fillPaxsBags(req_grps, exch, res_grps);
  fillPaxsSvcs(req_pnrs, exch);

  TPaxSection *paxSection=dynamic_cast<TPaxSection*>(&exch);
  TSvcSection *svcSection=dynamic_cast<TSvcSection*>(&exch);
  for(TEntityList::const_iterator i=entities.begin(); i!=entities.end(); ++i)
  {
    if (paxSection) paxSection->updateSeg(i->first, i->second);
    if (svcSection) svcSection->updateSeg(i->first);
  };
}

} //namespace SirenaExchange

