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
#include "basic.h"
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
#include "astra_callbacks.h"
#include "tlg/tlg.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"
#include "checkin_utils.h"
#include "apis_utils.h"
#include "stl_utils.h"
#include "astra_callbacks.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC;
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

const string PARTITION_ELEM_TYPE = "�";
const string ARMCHAIR_ELEM_TYPE = "�";

int readInetClientId(const char *head)
{
  short grp;
  memcpy(&grp,head+45,2);
  return ntohs(grp);
}

void RevertWebResDoc();

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len)
{
  InitLogTime(NULL);
  PerfomInit();
  int client_id=readInetClientId(head);
  ProgTrace(TRACE1,"new web request received from client %i",client_id);

  try
  {
    if (ENABLE_REQUEST_DUP() &&
        hlen>0 && *head==char(2))
    {
      std::string b(body,blen);
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

  string answer;
  int newlen=0;

  try
  {
    InetClient client=getInetClient(IntToString(client_id));
    string new_header=(string(head,45)+client.pult+"  "+client.opr+string(100,0)).substr(0,100)+string(head+100,hlen-100);

    string new_body(body,blen);
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

    newlen=ac->jxt_proc((const char *)new_body.data(),new_body.size(),(const char *)new_header.data(),new_header.size(),res,len);
    ProgTrace(TRACE1,"newlen=%i",newlen);

    memcpy(*res,head,hlen);


  }
  catch(...)
  {
    answer="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<error/>";
    newlen=answer.size()+hlen; // ࠧ��� �⢥� (� ����������)
    ProgTrace(TRACE1,"Outgoing message is %zu bytes long",answer.size());
    if(newlen>len)
    {
      *res=(char *)malloc(newlen*sizeof(char));
      if(*res==NULL)
      {
        ProgError(STDLOG,"malloc failed to allocate %i bytes",newlen);
        return 0;
      }
    }
    memcpy(*res,head,hlen);
    memcpy(*res+hlen,answer.data(),answer.size());
  }

  InitLogTime(NULL);
  return newlen;
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

int VerifyPNRByPaxId( int point_id, int pax_id ) //�����頥� pnr_id
{
  return VerifyPNR(point_id, pax_id, false);
}

void VerifyPNRByPnrId( int point_id, int pnr_id )
{
  VerifyPNR(point_id, pnr_id, true);
}

int VerifyPNRById( int point_id, xmlNodePtr node ) //�����頥� pnr_id
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
  
  if ((reqInfo->user.access.airps_permit && reqInfo->user.access.airps.empty()) ||
      (reqInfo->user.access.airlines_permit && reqInfo->user.access.airlines.empty()))
  {
    ProgError(STDLOG, "WebRequestsIface::SearchPNRs: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    return;
  };

  WebSearch::TPNRFilter filter;
  filter.fromXML(reqNode);
  filter.testPaxFromDB();
  filter.trace(TRACE5);
  
  WebSearch::TPNRs PNRs;
  findPNRs(filter, PNRs, 1);
  findPNRs(filter, PNRs, 2);
  findPNRs(filter, PNRs, 3);
  PNRs.toXML(resNode);
};

void WebRequestsIface::SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

  xmlNodePtr scanCodeNode=GetNode("scan_code", reqNode);

  resNode=NewTextChild(resNode,"SearchFlt");

  if ((reqInfo->user.access.airps_permit && reqInfo->user.access.airps.empty()) ||
      (reqInfo->user.access.airlines_permit && reqInfo->user.access.airlines.empty()))
  {
    ProgError(STDLOG, "WebRequestsIface::SearchFlt: empty user's access (user.descr=%s)", reqInfo->user.descr.c_str());
    return;
  };
  
  WebSearch::TPNRFilter filter;
  if (scanCodeNode!=NULL)
    filter.fromBCBP_M(NodeAsString(scanCodeNode));
  else
  {
    filter.fromXML(reqNode);
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
  
  WebSearch::TPNRs PNRs;
  findPNRs(filter, PNRs, 1);
  if (PNRs.pnrs.empty() && scanCodeNode==NULL)  //�᫨ ᪠��஢���� ����-����, ⮣�� ⮫쪮 ���� �� �������饬� ��ॢ��稪�
  {
    findPNRs(filter, PNRs, 2);
    findPNRs(filter, PNRs, 3);
  };
  
  if (PNRs.pnrs.empty())
    throw UserException( "MSG.PASSENGERS.NOT_FOUND" );
  if (filter.test_paxs.empty() && PNRs.pnrs.size()>1)
    throw UserException( "MSG.PASSENGERS.FOUND_MORE" );

  xmlNodePtr segsNode=NewTextChild(resNode, "segments");

  const map< int/*num*/, WebSearch::TPNRSegInfo > &segs=PNRs.pnrs.begin()->second.segs;
  for(map< int/*num*/, WebSearch::TPNRSegInfo >::const_iterator iSeg=segs.begin(); iSeg!=segs.end(); ++iSeg)
  {
    const set<WebSearch::TFlightInfo>::const_iterator &iFlt=PNRs.flights.find(WebSearch::TFlightInfo(iSeg->second.point_dep));
    if (iFlt==PNRs.flights.end())
      throw EXCEPTIONS::Exception("WebRequestsIface::SearchFlt: flight not found in PNRs (point_dep=%d)", iSeg->second.point_dep);
    const set<WebSearch::TDestInfo>::const_iterator &iDest=iFlt->dests.find(WebSearch::TDestInfo(iSeg->second.point_arv));
    if (iDest==iFlt->dests.end())
      throw EXCEPTIONS::Exception("WebRequestsIface::SearchFlt: dest not found in PNRs (point_arv=%d)", iSeg->second.point_arv);
      
    xmlNodePtr segNode=NewTextChild(segsNode, "segment");
    iFlt->toXML(segNode, true);
    iDest->toXML(segNode, true);
    iSeg->second.toXML(segNode, true);
    if (iSeg==segs.begin())
      PNRs.pnrs.begin()->second.toXML(segNode, true);
  };
};
/*
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
*/

struct TWebPnr {
  TCheckDocInfo checkDocInfo;
  TCheckTknInfo checkTknInfo;
  set<string> apis_formats;
  vector<TWebPax> paxs;
  
  void clear()
  {
    checkDocInfo.clear();
    checkTknInfo.clear();
    apis_formats.clear();
    paxs.clear();
  };
};

void verifyPaxTids( int pax_id, int crs_pnr_tid, int crs_pax_tid, int pax_grp_tid, int pax_tid )
{
  TQuery Qry(&OraSession);
  if (!isTestPaxId(pax_id))
  {
    if (pax_grp_tid==NoExists || pax_tid==NoExists)
      throw UserException( "MSG.PASSENGERS.GROUP_CHANGED.REFRESH_DATA" ); //�� �뢠�� ����� ��। ������ �ந��諠 ࠧॣ������
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
  return !(//pax.name=="CBBG" ||  ���� ����� � ��ࣨ����
     		   pnr_status=="DG2" ||
     		   pnr_status=="RG2" ||
     		   pnr_status=="ID2" ||
     		   pnr_status=="WL");
};

bool is_valid_pax_status(int pax_id, TQuery& PaxQry)
{
  const char* sql=
    "SELECT time FROM crs_pax_refuse "
    "WHERE pax_id=:pax_id AND client_type=:client_type AND rownum<2";
  if (strcmp(PaxQry.SQLText.SQLText(),sql)!=0)
  {
    PaxQry.Clear();
    PaxQry.SQLText=sql;
    PaxQry.DeclareVariable("pax_id", otInteger);
    PaxQry.CreateVariable("client_type", otString, EncodeClientType(TReqInfo::Instance()->client_type));
  };
  PaxQry.SetVariable("pax_id", pax_id);
  PaxQry.Execute();
  if (!PaxQry.Eof) return false;
  return true;
};

bool is_valid_doc_info(const TCheckDocInfo &checkDocInfo,
                       const CheckIn::TPaxDocItem &doc)
{
  if ((checkDocInfo.doc.required_fields&doc.getNotEmptyFieldsMask())!=checkDocInfo.doc.required_fields) return false;
  return true;
};

bool is_valid_doco_info(const TCheckDocInfo &checkDocInfo,
                        const CheckIn::TPaxDocoItem &doco)
{
  if ((checkDocInfo.doco.required_fields&doco.getNotEmptyFieldsMask())!=checkDocInfo.doco.required_fields) return false;
  return true;
};

bool is_valid_doca_info(const TCheckDocInfo &checkDocInfo,
                        const list<CheckIn::TPaxDocaItem> &doca)
{
  CheckIn::TPaxDocaItem docaB, docaR, docaD;
  CheckIn::ConvertDoca(doca, docaB, docaR, docaD);

  if ((checkDocInfo.docaB.required_fields&docaB.getNotEmptyFieldsMask())!=checkDocInfo.docaB.required_fields) return false;
  if ((checkDocInfo.docaR.required_fields&docaR.getNotEmptyFieldsMask())!=checkDocInfo.docaR.required_fields) return false;
  if ((checkDocInfo.docaD.required_fields&docaD.getNotEmptyFieldsMask())!=checkDocInfo.docaD.required_fields) return false;
  return true;
};

bool is_valid_tkn_info(const TCheckTknInfo &checkTknInfo,
                       const CheckIn::TPaxTknItem &tkn)
{
  if ((checkTknInfo.tkn.required_fields&tkn.getNotEmptyFieldsMask())!=checkTknInfo.tkn. required_fields) return false;
  return true;
};

void checkDocInfoToXML(const TCheckDocInfo &checkDocInfo,
                       const xmlNodePtr node)
{
  if (node==NULL) return;
  xmlNodePtr fieldsNode=NewTextChild(node, "doc_required_fields");
  SetProp(fieldsNode, "is_inter", checkDocInfo.doc.is_inter);
  if ((checkDocInfo.doc.required_fields&DOC_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkDocInfo.doc.required_fields&DOC_ISSUE_COUNTRY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_country");
  if ((checkDocInfo.doc.required_fields&DOC_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkDocInfo.doc.required_fields&DOC_NATIONALITY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "nationality");
  if ((checkDocInfo.doc.required_fields&DOC_BIRTH_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_date");
  if ((checkDocInfo.doc.required_fields&DOC_GENDER_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "gender");
  if ((checkDocInfo.doc.required_fields&DOC_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkDocInfo.doc.required_fields&DOC_SURNAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "surname");
  if ((checkDocInfo.doc.required_fields&DOC_FIRST_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "first_name");
  if ((checkDocInfo.doc.required_fields&DOC_SECOND_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "second_name");
    
  fieldsNode=NewTextChild(node, "doco_required_fields");
  SetProp(fieldsNode, "is_inter", checkDocInfo.doco.is_inter);
  if ((checkDocInfo.doco.required_fields&DOCO_BIRTH_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_place");
  if ((checkDocInfo.doco.required_fields&DOCO_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkDocInfo.doco.required_fields&DOCO_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkDocInfo.doco.required_fields&DOCO_ISSUE_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_place");
  if ((checkDocInfo.doco.required_fields&DOCO_ISSUE_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_date");
  if ((checkDocInfo.doco.required_fields&DOCO_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkDocInfo.doco.required_fields&DOCO_APPLIC_COUNTRY_FIELD) != 0x0000)
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
      TQuery PaxStatusQry(&OraSession);

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

      TQuery FQTQry(&OraSession);
      FQTQry.DeclareVariable( "pax_id", otInteger );

      const char* PaxFQTQrySQL=
       "SELECT rem_code, airline, no, extra "
       "FROM pax_fqt WHERE pax_id=:pax_id AND rem_code='FQTV'";

      const char* CrsFQTQrySQL=
       "SELECT rem_code, airline, no, extra "
       "FROM crs_pax_fqt WHERE pax_id=:pax_id AND rem_code='FQTV'";

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
        pnr.checkDocInfo=GetCheckDocInfo(point_id, Qry.FieldAsString("airp_arv"), pnr.apis_formats).pass;
        pnr.checkTknInfo=GetCheckTknInfo(point_id).pass;
        //ProgTrace(TRACE5, "getPnr: point_id=%d airp_arv=%s", point_id, Qry.FieldAsString("airp_arv"));
        //ProgTrace(TRACE5, "getPnr: checkDocInfo.first.required_fields=%ld", pnr.checkDocInfo.first.required_fields);
        //ProgTrace(TRACE5, "getPnr: checkDocInfo.second.required_fields=%ld", pnr.checkDocInfo.second.required_fields);
        //ProgTrace(TRACE5, "getPnr: checkTknInfo.required_fields=%ld", pnr.checkTknInfo.required_fields);
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
        		LoadPaxDoc(pax.pax_id, pax.doc);
        		LoadPaxDoco(pax.pax_id, pax.doco);
         		FQTQry.SQLText=PaxFQTQrySQL;
         		FQTQry.SetVariable( "pax_id", pax.pax_id );
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

         		CrsTKNQry.SetVariable( "pax_id", pax.crs_pax_id );
         		CrsTKNQry.Execute();
         		if (!CrsTKNQry.Eof) pax.tkn.fromDB(CrsTKNQry);
         		//ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.tkn.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.tkn.getNotEmptyFieldsMask());
        		LoadCrsPaxDoc(pax.crs_pax_id, pax.doc, true);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doc.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doc.getNotEmptyFieldsMask());
        	  LoadCrsPaxVisa(pax.crs_pax_id, pax.doco);
        		//ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doco.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doco.getNotEmptyFieldsMask());
            LoadCrsPaxDoca(pax.crs_pax_id, pax.doca);

         		if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")))
         		  pax.agent_checkin_reasons.insert("pnr_status");
         		if (!is_valid_pax_status(pax.crs_pax_id, PaxStatusQry))
         		  pax.agent_checkin_reasons.insert("web_cancel");
         		if (!is_valid_doc_info(pnr.checkDocInfo, pax.doc))
         		  pax.agent_checkin_reasons.insert("incomplete_doc");
         		if (!is_valid_doco_info(pnr.checkDocInfo, pax.doco))
         		  pax.agent_checkin_reasons.insert("incomplete_doco");
            if (!is_valid_doca_info(pnr.checkDocInfo, pax.doca))
         		  pax.agent_checkin_reasons.insert("incomplete_doca");
         		if (!is_valid_tkn_info(pnr.checkTknInfo, pax.tkn))
         		  pax.agent_checkin_reasons.insert("incomplete_tkn");

         		if (!pax.agent_checkin_reasons.empty())
         		  pax.checkin_status = "agent_checkin";

         		FQTQry.SQLText=CrsFQTQrySQL;
         		FQTQry.SetVariable( "pax_id", pax.crs_pax_id );
         	}
         	FQTQry.Execute();
       		for(; !FQTQry.Eof; FQTQry.Next())
       		{
            TypeB::TFQTItem FQTItem;
            strcpy(FQTItem.rem_code, FQTQry.FieldAsString("rem_code"));
            strcpy(FQTItem.airline, FQTQry.FieldAsString("airline"));
            strcpy(FQTItem.no, FQTQry.FieldAsString("no"));
            FQTItem.extra = FQTQry.FieldAsString("extra");
       		  pax.fqt_rems.push_back(FQTItem);
       		};
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
        "SELECT surname, subcls.class, subclass, doc_no, tkn_no, "
        "       pnr_airline AS fqt_airline, fqt_no, "
        "       seat_xname, seat_yname "
        "FROM test_pax, subcls "
        "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
      Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
    	Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        //��⮢� ���ᠦ��
        TWebPax pax;
        pax.crs_pax_id = pnr_id;
        pax.surname = Qry.FieldAsString("surname");
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
          TypeB::TFQTItem FQTItem;
          strcpy(FQTItem.rem_code, "FQTV");
          strcpy(FQTItem.airline, Qry.FieldAsString("fqt_airline"));
          strcpy(FQTItem.no, Qry.FieldAsString("fqt_no"));
       		pax.fqt_rems.push_back(FQTItem);
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

void IntLoadPnr( const vector<TIdsPnrData> &ids, vector< TWebPnr > &pnrs, xmlNodePtr segsNode, bool afterSave )
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
        //䨫���㥬 ���ᠦ�஢ �� ��ண� � ᫥����� ᥣ���⮢
        const TWebPnr &firstPnr=*(pnrs.begin());
        for(vector<TWebPax>::iterator iPax=pnr.paxs.begin();iPax!=pnr.paxs.end();)
        {
          //㤠��� �㡫�஢���� 䠬����+��� �� pnr
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
            //�� ��諨 ᮮ⢥�����饣� ���ᠦ�� �� ��ࢮ�� ᥣ����
            iPax=pnr.paxs.erase(iPax);
            continue;
          };
          iPax->pax_no=iPaxFirst->pax_no; //���⠢�塞 ���ᠦ��� ᮮ�. ����㠫�� ��. �� ��ࢮ�� ᥣ����
          iPax++;
        };
      }
      else
      {
        //���ᠦ�஢ ��ࢮ�� ᥣ���� ���⠢�� pax_no �� ���浪�
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

      NewTextChild( segNode, "apis", (int)(!pnr.apis_formats.empty()) );
      checkDocInfoToXML(pnr.checkDocInfo, segNode);
      
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
       	for ( vector<TypeB::TFQTItem>::const_iterator f=iPax->fqt_rems.begin(); f!=iPax->fqt_rems.end(); f++ )
       	{
          xmlNodePtr fqtNode = NewTextChild( fqtsNode, "fqt_rem" );
          NewTextChild( fqtNode, "airline", f->airline );
          NewTextChild( fqtNode, "no", f->no );
        };

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
  
  vector< TWebPnr > pnrs;
  segsNode = NewTextChild( NewTextChild( resNode, "LoadPnr" ), "segments" );
  IntLoadPnr( ids, pnrs, segsNode, false );
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
	SALONS2::TPlaceWebTariff WebTariff;
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
  for ( vector<TWebPax>::iterator i=pnr.begin(); i!=pnr.end(); i++ ) {
  	if ( !i->pass_class.empty() )
  	  crs_class = i->pass_class;
  	if ( !i->pass_subclass.empty() )
  	  crs_subclass = i->pass_subclass;
  	TPerson p=DecodePerson(i->pers_type_extended.c_str());
  	pr_CHIN=(pr_CHIN || p==ASTRA::child || p==ASTRA::baby); //�।� ⨯�� ����� ���� �� (CBBG) ����� ��ࠢ�������� � ���᫮��
  	if ( isTranzitSalonsVersion ) {
  	  if ( point_arv == ASTRA::NoExists ) {
  	    point_arv = SALONS2::getCrsPaxPointArv( i->crs_pax_id, point_id );
  	  }
    }
  }
  ProgTrace( TRACE5, "ReadWebSalons: point_dep=%d, point_arv=%d", point_id, point_arv );
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
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), SALONS2::rfTranzitVersion, crs_class );
  }
  else {
    SalonsO.FilterClass = crs_class;
    SalonsO.Read();
  }
  // ����稬 �ਧ��� ⮣�, �� � ᠫ��� ���� ᢮����� ���� � ����� �������ᮬ
  pr_find_free_subcls_place=false;
  string pass_rem;

  subcls_rems.IsSubClsRem( crs_subclass, pass_rem );
  TSalons *Salons;
  TSalons SalonsN;
  if ( isTranzitSalonsVersion ) {
    //������ �� �������� ������ ��� ࠧ��⪨ ��㯯�
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
          place != (*placeList)->places.end(); place++ ) { // �஡�� �� ᠫ����
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
     	wp.WebTariff.color = place->WebTariff.color;
     	wp.WebTariff.value = place->WebTariff.value;
     	wp.WebTariff.currency_id = place->WebTariff.currency_id;
     	if ( place->isplace && !place->clname.empty() && place->clname == crs_class ) {
     		bool pr_first = true;
     	for( std::vector<TPlaceLayer>::iterator ilayer=place->layers.begin(); ilayer!=place->layers.end(); ilayer++ ) { // ���஢�� �� �ਮ��⠬
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

    	  wp.pr_free = ( wp.pr_free || pr_first ); // 0 - �����, 1 - ᢮�����, 2 - ���筮 �����

        if ( wp.pr_free ) {
          //���� ᢮�����
          //����塞 �������� ����
          string seat_subcls;
        	for ( vector<TRem>::iterator i=place->rems.begin(); i!=place->rems.end(); i++ ) {
        		if ( isREM_SUBCLS( i->rem ) && !i->pr_denial ) {
              seat_subcls = i->rem;
        		  	break;
        		  }
            }
          if ( !pass_rem.empty() ) {
            //� ���ᠦ�� ���� ��������
            if ( pass_rem == seat_subcls ) {
              wp.pr_free = 3;  // ᢮����� � ��⮬ ��������
              pr_find_free_subcls_place = true;
            }
            else
              if ( seat_subcls.empty() ) { // ��� �������� � ����
                wp.pr_free = 2; // ᢮����� ��� ��� ��������
              }
              else { // ��������� �� ᮢ����
                wp.pr_free = 0; // �����
              }
          }
          else {
            // � ���ᠦ�� ��� ��������
            if ( !seat_subcls.empty() ) { // �������� � ����
              	wp.pr_free = 0; // �����
            }
          }
        }
        if ( pr_CHIN ) { // ��������� � ��㯯� ���ᠦ��� � ���쬨
        	if ( place->elem_type == "�" ) { // ���� � ���਩���� ��室�
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
  	case 0: // �����
   		status = 1;
   		break;
   	case 1: // ᢮�����
   		status = 0;
   		break;
   	case 2: // ᢮����� ��� ��� ��������
   		status = pr_find_free_subcls_place;
   		break;
   	case 3: // ᢮����� � ��⮬ ��������
   		status = !pr_find_free_subcls_place;
   		break;
  };
  if ( status == 0 && wp.pr_CHIN ) {
   	status = 2;
  }
  return status;
}

// ��।����� ���������� ���� crs_pax_id, crs_seat_no, class, subclass
// �� ��室� ��������� TWebPlace �� ���ᠦ���
void GetCrsPaxSeats( int point_id, const vector<TWebPax> &pnr,
                     vector< pair<TWebPlace, LexemaData> > &pax_seats )
{
  pax_seats.clear();
  map<int, TWebPlaceList> web_salons;
  bool pr_find_free_subcls_place=false;
  ReadWebSalons( point_id, pnr, web_salons, pr_find_free_subcls_place );
  for ( vector<TWebPax>::const_iterator i=pnr.begin(); i!=pnr.end(); i++ ) { // �஡�� �� ���ᠦ�ࠬ
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
      } // �஡�� �� ᠫ���
      if ( pr_find )
        break;
    } // �஡�� �� ᠫ���
    if ( !pr_find ) {
      TWebPlace wp;
      wp.pax_id = i->crs_pax_id;
      wp.seat_no = i->crs_seat_no;
      ld.lexema_id = "MSG.SEATS.SEAT_NO.NOT_FOUND";
      pax_seats.push_back( make_pair( wp, ld ) );
    }
  } // �஡�� �� ���ᠦ�ࠬ
}

/*
1. �� ������ �᫨ ���ᠦ�� ����� ᯥ�. �������� (६�ન MCLS) - ���� �롨ࠥ� ⮫쪮 ���� � ६�ઠ�� �㦭��� ��������.
�� ������ , �᫨ ᠫ�� �� ࠧ��祭?
2. ���� ��㯯� ���ᠦ�஢, ������� 㦥 ��ॣ����஢���, ������� ���.
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
      if ( wp->WebTariff.value != 0.0 ) { // �᫨ ���⭠� ॣ������ �⪫�祭�, value=0.0 � �� ��砥
      	xmlNodePtr rateNode = NewTextChild( placeNode, "rate" );
      	NewTextChild( rateNode, "color", wp->WebTariff.color );
      	ostringstream buf;
        buf << std::fixed << setprecision(2) << wp->WebTariff.value;
      	NewTextChild( rateNode, "value", buf.str() );
      	NewTextChild( rateNode, "currency", wp->WebTariff.currency_id );
      }
    }
  }
}

bool CreateEmulCkinDocForCHKD(int crs_pax_id,
                              vector<WebSearch::TPnrData> &PNRs,
                              const XMLDoc &emulDocHeader,
                              XMLDoc &emulCkinDoc,
                              list<int> &crs_pax_ids)  //crs_pax_ids ����砥� �஬� crs_pax_id ��. �ਢ易���� ������楢
{
  crs_pax_ids.clear();
  if (PNRs.size()!=1)
    throw EXCEPTIONS::Exception("CreateEmulCkinDocForCHKD: PNRs.size()!=1");

  TWebPnrForSave pnr;

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="SELECT rem FROM crs_pax_rem WHERE pax_id=:pax_id AND rem_code='FQTV'";
  RemQry.DeclareVariable("pax_id", otInteger);

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
    RemQry.SetVariable("pax_id", paxFromReq.crs_pax_id);
    RemQry.Execute();
    for(;!RemQry.Eof;RemQry.Next()) paxFromReq.fqt_rems.push_back(RemQry.FieldAsString("rem"));

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
  if (parent_reg_no==NoExists) return false; //� த�⥫� ��� ॣ. ����� �� CHKD
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

  //���� ����� �஢��塞, �� ����ॣ����஢���� ���ᠦ��� ᮢ������ �� ���-�� ��� ������� ᥣ����
  //�� ��᫥���� ᥣ����� ���-�� ����ॣ����஢����� ���ᠦ�஢ �.�. �㫥��
  int prevNotCheckedCount=NoExists;
  int seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, seg_no++)
  {
    int currNotCheckedCount=0;
    for(vector<TWebPaxFromReq>::const_iterator iPax=s->second.paxFromReq.begin(); iPax!=s->second.paxFromReq.end(); iPax++)
    {
      if (iPax->pax_grp_tid==NoExists || iPax->pax_tid==NoExists)
        //���ᠦ�� �� ��ॣ����஢��
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
  TQuery PaxStatusQry(&OraSession);
  
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
        int pnr_id=NoExists; //���஡㥬 ��।����� �� ᥪ樨 passengers
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
                //���ᠦ�� �� ��ॣ����஢��
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
                    !is_valid_pax_status(iPax->crs_pax_id, PaxStatusQry))
                  throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");
              }
              else
              {
                //���ᠦ�� ��ॣ����஢��
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
              //���� ���ᠦ��
              pnr_id=Qry.FieldAsInteger("pnr_id");
              //�஢�ਬ, �� ������ PNR �ਢ易�� � ३��
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
              if (iPax->present_in_req.find(ciDoc) !=  iPax->present_in_req.end())
              {
                //�஢�ઠ ��� ४����⮢ ���㬥��
                pax.doc=NormalizeDoc(iPax->doc);
                pax.present_in_req.insert(ciDoc);
              };
              if (iPax->present_in_req.find(ciDoco) !=  iPax->present_in_req.end())
              {
                //�஢�ઠ ��� ४����⮢ ����
                pax.doco=NormalizeDoco(iPax->doco);
                pax.present_in_req.insert(ciDoco);
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
              //��ࠡ�⪠ ���㬥�⮢
              if (isTestPaxId(iPax->crs_pax_id))
              {
                pax.apis.doc.clear();
                pax.apis.doc.no = Qry.FieldAsString("doc_no");
                pax.apis.doco.clear();
              }
              else
              {
                if (iPax->present_in_req.find(ciDoc) !=  iPax->present_in_req.end())
                {
                  //�஢�ઠ ��� ४����⮢ ���㬥��
                  pax.apis.doc=NormalizeDoc(iPax->doc);
                  pax.present_in_req.insert(ciDoc);
                }
                else
                  LoadCrsPaxDoc(pax.crs_pax_id, pax.apis.doc, true);
                  
                if (iPax->present_in_req.find(ciDoco) !=  iPax->present_in_req.end())
                {
                  //�஢�ઠ ��� ४����⮢ ����
                  pax.apis.doco=NormalizeDoco(iPax->doco);
                  pax.present_in_req.insert(ciDoco);
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
        //�஢�ਬ ���� ᮮ⢥��⢨� point_id � pnr_id
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
  
  //� �� ��砥 ����뢠�� ᪢����� �������
  bool is_test = isTestPaxId(segs.begin()->second.pnr_id);
  
  CompletePnrData(is_test, segs.begin()->second.pnr_id, first);

  vector<WebSearch::TPnrData> PNRs;
  getTCkinData(first, is_test, PNRs);
  PNRs.insert(PNRs.begin(), first); //��⠢�塞 � ��砫� ���� ᥣ����
    
  const TWebPnrForSave &firstPnr=segs.begin()->second;
  //�஢�ઠ �ᥣ� ᪢������ ������� �� ᮢ������� point_id, pnr_id � ᮮ⢥��⢨� 䠬����/����
  vector<WebSearch::TPnrData>::const_iterator iPnrData=PNRs.begin();
  seg_no=1;
  for(vector< pair<int, TWebPnrForSave > >::iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
  {
    try
    {
      if (iPnrData!=PNRs.end() &&
          (!s->second.paxForCkin.empty() ||
           (!s->second.paxForChng.empty() && s->second.paxForChng.size()>s->second.refusalCountFromReq))) //⨯� ���� ���ᠦ���
      {
        //�஢��塞 �� ᥣ���� �뫥� ३� � ���ﭨ� ᮮ⢥�����饣� �⠯�
        if ( iPnrData->flt.act_out_local != NoExists )
  	      throw UserException( "MSG.FLIGHT.TAKEOFF" );

  	    if ( reqInfo->client_type == ctKiosk )
        {
          map<TStage_Type, TStage>::const_iterator iStatus=iPnrData->flt.stage_statuses.find(stKIOSKCheckIn);
          if (iStatus==iPnrData->flt.stage_statuses.end())
            throw EXCEPTIONS::Exception("VerifyPax: iPnrData->flt.stage_statuses[stKIOSKCheckIn] not defined (seg_no=%d)", seg_no);
        
          if (!(iStatus->second == sOpenKIOSKCheckIn ||
                (iStatus->second == sNoActive && s!=segs.begin()))) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
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
                (iStatus->second == sNoActive && s!=segs.begin()))) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
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
                (iStatus->second == sNoActive && s!=segs.begin()))) //��� ᪢����� ᥣ���⮢ ॣ������ ����� ���� �� �� �����
            throw UserException("MSG.PASSENGER.UNREGISTRATION_DENIAL");
        };
      };

      if (s==segs.begin()) continue; //�ய�᪠�� ���� ᥣ����

      TWebPnrForSave &currPnr=s->second;
      if (iPnrData==PNRs.end()) //��譨� ᥣ����� � ����� �� ॣ������
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->flt.point_dep!=s->first) //��㣮� ३� �� ᪢����� ᥣ����
        throw UserException("MSG.THROUGH_ROUTE_CHANGED.REFRESH_DATA");
      if (iPnrData->seg.pnr_id!=currPnr.pnr_id) //��㣮� pnr_id �� ᪢����� ᥣ����
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

          currPnr.paxForCkin.splice(currPnr.paxForCkin.end(),currPnr.paxForCkin,iPax2,iPax3); //��६�頥� ���������� ���ᠦ�� � �����
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
  
  //�����頥� ids
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
            pax.present_in_req.insert(ciDoc);
            PaxDocFromXML(docNode, pax.doc);
        }

        xmlNodePtr docoNode = GetNode("doco", paxNode);
        if (docoNode!=NULL) {
            pax.present_in_req.insert(ciDoco);
            PaxDocoFromXML(docoNode, pax.doco);
        }
        
        xmlNodePtr fqtNode = GetNode("fqt_rems", paxNode);
        pax.fqt_rems_present=(fqtNode!=NULL); //�᫨ ⥣ <fqt_rems> ��襫, � �����塞 � ��१����뢠�� ६�ન FQTV
        if (fqtNode!=NULL)
        {
          //�⠥� ��襤訥 ६�ન
          for(fqtNode=fqtNode->children; fqtNode!=NULL; fqtNode=fqtNode->next)
          {
            ostringstream rem_text;
            rem_text << "FQTV "
                     << NodeAsString("airline",fqtNode) << " "
                     << NodeAsString("no",fqtNode);
            pax.fqt_rems.push_back(rem_text.str());
          };
        };
        sort(pax.fqt_rems.begin(),pax.fqt_rems.end());
        
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

  int first_grp_id, tckin_id;
  TChangeStatusList ChangeStatusInfo;
  set<int> tckin_ids;
  bool result=true;
  //�����, �� ᭠砫� ��뢠���� CheckInInterface::SavePax ��� emulCkinDoc
  //⮫쪮 �� ���-ॣ����樨 ����� ��㯯� �������� ROLLBACK CHECKIN � SavePax �� ��ॣ�㧪�
  //� ᮮ⢥��⢥��� �����饭�� result=false
  //� ᮮ⢥��⢥��� �맮� ETStatusInterface::ETRollbackStatus ��� ���� ��
  
  if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ChangeStatusInfo, tckin_id))
    {
      if (tckin_id!=NoExists) tckin_ids.insert(tckin_id);
    }
    else
      result=false;
  };
  if (result)
  {
    for(map<int,XMLDoc>::iterator i=emulChngDocs.begin();i!=emulChngDocs.end();i++)
    {
      XMLDoc &emulChngDoc=i->second;
      xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulChngDoc.docPtr())->children;
      if (emulReqNode==NULL)
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
      if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ChangeStatusInfo, tckin_id))
      {
        if (tckin_id!=NoExists) tckin_ids.insert(tckin_id);
      }
      else
      {
        //�� ���� � �� ������� �� ������ �������� (�. �������਩ ���)
        //��� �⮣� ������� �� �����頥� false � ������ ᯥ樠���� ����� � SavePax:
        //�� ����� ��������� ��� � ���᪮� �� �⪠�뢠���� �� ��ॣ�㧪�
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CheckInInterface::SavePax=false");
        //result=false;
        //break;
      };

    };
  };

  if (result)
  {
    if (ediResNode==NULL && !ChangeStatusInfo.empty())
    {
      //��� �� ���� ����� �㤥� ��ࠡ��뢠����
      OraSession.Rollback();  //�⪠�
      ChangeStatusInterface::ChangeStatus(reqNode, ChangeStatusInfo);
      return false;
    };
    
    CheckTCkinIntegrity(tckin_ids, NoExists);
  
    vector< TWebPnr > pnrs;
    xmlNodePtr segsNode = NewTextChild( NewTextChild( resNode, "SavePax" ), "segments" );
    IntLoadPnr( ids, pnrs, segsNode, true );
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

void GetBPPax(xmlNodePtr paxNode, bool is_test, bool check_tids, PrintInterface::BPPax &pax)
{
  pax.clear();
  if (paxNode==NULL) throw EXCEPTIONS::Exception("GetBPPax: paxNode==NULL");
  xmlNodePtr node2=paxNode->children;
  pax.point_dep = NodeIsNULLFast( "point_id", node2, true)?
                    NoExists:
                    NodeAsIntegerFast( "point_id", node2 );
  pax.pax_id = NodeAsIntegerFast( "pax_id", node2 );
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
	if (check_tids) verifyPaxTids( pax.pax_id, crs_pnr_tid, crs_pax_tid, pax_grp_tid, pax_tid );
	TQuery Qry(&OraSession);
	if (!is_test)
	{
	  Qry.Clear();
  	Qry.SQLText =
  	 "SELECT pax_grp.grp_id, pax_grp.point_dep, "
     "       pax.reg_no, pax.surname||' '||pax.name full_name "
  	 "FROM pax_grp, pax "
  	 "WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND "
     "      pax_grp.status NOT IN ('E')";
  	Qry.CreateVariable( "pax_id", otInteger, pax.pax_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.PASSENGER.NOT_FOUND" );
  	pax.reg_no = Qry.FieldAsInteger( "reg_no" );
    pax.full_name = Qry.FieldAsString( "full_name" );
  }
  else
  {
    Qry.Clear();
    Qry.SQLText =
      "SELECT reg_no, surname||' '||name full_name "
      "FROM test_pax WHERE id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax.pax_id );
  	Qry.Execute();
  	if ( Qry.Eof )
  		throw UserException( "MSG.PASSENGER.NOT_FOUND" );
  	pax.reg_no = Qry.FieldAsInteger( "reg_no" );
    pax.full_name = Qry.FieldAsString( "full_name" );

    Qry.Clear();
    Qry.SQLText =
     "SELECT :grp_id AS grp_id, point_id AS point_dep "
     "FROM points "
     "WHERE point_id=:point_id AND pr_del>=0";
    if (pax.point_dep!=NoExists)
    {
      Qry.CreateVariable( "grp_id", otInteger, pax.point_dep + TEST_ID_BASE );
      Qry.CreateVariable( "point_id", otInteger, pax.point_dep );
    }
    else
    {
      if (pax_grp_tid==NoExists) throw UserException( "MSG.FLIGHT.NOT_FOUND" );
      Qry.CreateVariable( "grp_id", otInteger, pax_grp_tid + TEST_ID_BASE );
      Qry.CreateVariable( "point_id", otInteger, pax_grp_tid );
    };
    Qry.Execute();
  	if ( Qry.Eof )
  	  throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  };
	pax.point_dep = Qry.FieldAsInteger( "point_dep" );
	pax.grp_id = Qry.FieldAsInteger( "grp_id" );
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
	Qry.CreateVariable( "work_mode", otString, "�" );
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
  bool is_test=isTestPaxId(NodeAsInteger("passengers/pax/pax_id", reqNode));
  xmlNodePtr paxNode = NodeAsNode("passengers", reqNode)->children;
  for(;paxNode!=NULL;paxNode=paxNode->next)
  {
    PrintInterface::BPPax pax;
    try
    {
      GetBPPax( paxNode, is_test, false, pax );
      pax.time_print=NodeAsDateTime("prn_form_key", paxNode);
      paxs.push_back(pax);
    }
    catch(UserException &e)
    {
      //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };

  if (!is_test)
  {
    PrintInterface::ConfirmPrintBP(paxs, ue);  //�� ���� �ப��뢠�� ue � �ନ��� - ���⢥ত��� �� �� �����!
  };
  
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
      "WHERE (desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
      "      (desk IS NULL OR desk=:desk) "
      "ORDER BY priority DESC ";
  Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.Execute();
  if(Qry.Eof) throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_DESK");
  params.form_type = Qry.FieldAsString("bp_type");
  ProgTrace(TRACE5, "bp_type: %s", params.form_type.c_str());
  
  CheckIn::UserException ue;
  vector<PrintInterface::BPPax> paxs;
  map<int/*point_dep*/, string/*gate*/> gates;
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
  
  if (!ue.empty()) throw ue;

  string pectab;
  PrintInterface::GetPrintDataBP(params, pectab, paxs);

  xmlNodePtr BPNode = NewTextChild( resNode, "GetPrintDataBP" );
  NewTextChild(BPNode, "pectab", pectab);
  xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
  for (vector<PrintInterface::BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
  {
    xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id", iPax->pax_id);
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

void WebRequestsIface::GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->client_type==ctTerm) reqInfo->client_type=EMUL_CLIENT_TYPE;

	ProgTrace(TRACE1,"WebRequestsIface::GetBPTags");
	PrintInterface::BPPax pax;
	bool is_test=isTestPaxId(NodeAsInteger("pax_id", reqNode));
	GetBPPax( reqNode, is_test, true, pax );
	PrintDataParser parser( pax.grp_id, pax.pax_id, 0, NULL );
	vector<string> tags;
	BPTags::Instance()->getFields( tags );
	xmlNodePtr node = NewTextChild( resNode, "GetBPTags" );
    for ( vector<string>::iterator i=tags.begin(); i!=tags.end(); i++ ) {
        for(int j = 0; j < 2; j++) {
            string value = parser.pts.get_tag(*i, ServerFormatDateTimeAsString, (j == 0 ? "R" : "E"));
            NewTextChild( node, (*i + (j == 0 ? "" : "_lat")), value );
            ProgTrace( TRACE5, "field name=%s, value=%s", (*i + (j == 0 ? "" : "_lat")).c_str(), value.c_str() );
        }
    }
  parser.pts.save_bp_print(true);
    
  string gate=GetBPGate(pax.point_dep);
  if (!gate.empty())
    NewTextChild( node, "gate", gate );
}

void ChangeProtPaidLayer(xmlNodePtr reqNode, xmlNodePtr resNode,
                         bool pr_del, int time_limit,
                         int &curr_tid, CheckIn::UserException &ue)
{
  TQuery Qry(&OraSession);
  int point_id=NoExists;
  bool error_exists=false;
  try
  {
    if (!pr_del)
    {
      point_id=NodeAsInteger("point_id", reqNode);
      //�஢�ਬ �ਧ��� ���⭮� ॣ����樨 �
      Qry.Clear();
      Qry.SQLText =
    	  "SELECT pr_permit, prot_timeout FROM trip_paid_ckin WHERE point_id=:point_id";
    	Qry.CreateVariable( "point_id", otInteger, point_id );
    	Qry.Execute();
    	if ( Qry.Eof || Qry.FieldAsInteger("pr_permit")==0 )
    	  throw UserException( "MSG.CHECKIN.NOT_PAID_CHECKIN_MODE" );

    	if (time_limit==NoExists)
    	{
    	  //����稬 prot_timeout
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
      "       id AS crs_pnr_tid, "
      "       id AS crs_pax_tid, "
      "       0 AS checked "
  	  "FROM test_pax, subcls "
  	  "WHERE test_pax.subclass=subcls.code AND test_pax.id=:crs_pax_id";

    TQuery PaxStatusQry(&OraSession);
    int pnr_id=NoExists, point_id_tlg=NoExists;
    string airp_arv;
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
          pax.crs_seat_no=NodeAsStringFast("seat_no", node2);
          if (pax.crs_seat_no.empty())
            throw EXCEPTIONS::Exception("ChangeProtPaidLayer: empty seat_no (crs_pax_id=%d)", pax.crs_pax_id);

          //�஢�ਬ �� �㡫�஢����
          for(vector<TWebPax>::const_iterator iPax=pnr.begin();iPax!=pnr.end();iPax++)
          {
            if (iPax->crs_pax_id==pax.crs_pax_id)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: crs_pax_id duplicated (crs_pax_id=%d)",
                                          pax.crs_pax_id);
            if (iPax->crs_seat_no==pax.crs_seat_no)
              throw EXCEPTIONS::Exception("ChangeProtPaidLayer: seat_no duplicated (crs_pax_id=%d, seat_no=%s)",
                                          pax.crs_pax_id, pax.crs_seat_no.c_str());
          };
        };
        
        xmlNodePtr tidsNode=NodeAsNodeFast("tids", node2);
        pax.crs_pnr_tid=NodeAsInteger("crs_pnr_tid",tidsNode);
        pax.crs_pax_tid=NodeAsInteger("crs_pax_tid",tidsNode);

        //�஢�ਬ tids ���ᠦ��
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

        if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")) ||
            !is_valid_pax_status(pax.crs_pax_id, PaxStatusQry))
          throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

        if (pnr_id==NoExists)
        {
          pnr_id=Qry.FieldAsInteger("pnr_id");
          if (!isTestPaxId(pax.crs_pax_id))
          {
            point_id_tlg=Qry.FieldAsInteger("point_id");
            airp_arv=Qry.FieldAsString("airp_arv");
          };
        }
        else
        {
          if (pnr_id!=Qry.FieldAsInteger("pnr_id"))
            throw EXCEPTIONS::Exception("ChangeProtPaidLayer: passengers from different PNR (crs_pax_id=%d)", pax.crs_pax_id);
        };
        pax.pass_class=Qry.FieldAsString("class");
        pax.pass_subclass=Qry.FieldAsString("subclass");
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
          "DECLARE "
          "  vrange_id    tlg_comp_layers.range_id%TYPE; "
          "BEGIN "
          "  BEGIN "
          "    SELECT range_id "
          "    INTO vrange_id "
          "    FROM tlg_comp_layers "
          "    WHERE crs_pax_id=:crs_pax_id AND layer_type=:layer_type FOR UPDATE; "
          "      UPDATE tlg_comp_layers "
          "      SET time_remove=SYSTEM.UTCSYSDATE+:timeout/1440 "
          "      WHERE range_id=vrange_id; "
          "    IF :tid IS NULL THEN "
          "     SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
          "    END IF; "
          "    UPDATE crs_pax SET tid=:tid WHERE pax_id=:crs_pax_id; "
          "  EXCEPTION "
          "    WHEN NO_DATA_FOUND THEN NULL; "
          "    WHEN TOO_MANY_ROWS THEN NULL; "
          "  END; "
          "END; ";
        LayerQry.DeclareVariable("layer_type", otString);
        LayerQry.DeclareVariable("crs_pax_id", otInteger);
        if (time_limit!=NoExists)
          LayerQry.CreateVariable("timeout", otInteger, time_limit);
        else
          LayerQry.CreateVariable("timeout", otInteger, FNull);
        LayerQry.DeclareVariable( "tid", otInteger );
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
              
/*            if ( iSeat->first.WebTariff.value == 0.0 )  //��� ���
              throw UserException("MSG.SEATS.NOT_SET_RATE");*/
            
            if (isTestPaxId(iPax->crs_pax_id)) continue;

            //�஢�ન + ������!!!
            int tid = iPax->crs_pax_tid;
            bool res;
            if ( isTranzitSalonsVersion ) {
              res = IntChangeSeatsN( point_id,
                                     iPax->crs_pax_id,
                                     tid,
                                     iSeat->first.xname,
                                     iSeat->first.yname,
                                     stSeat,
                                     cltProtBeforePay,
                                     change_layer_flags,
                                     NULL );
            }
            else {
              res = IntChangeSeats( point_id,
                                    iPax->crs_pax_id,
                                    tid,
                                    iSeat->first.xname,
                                    iSeat->first.yname,
                                    stSeat,
                                    cltProtBeforePay,
                                    change_layer_flags,
                                    NULL );
            }
            if ( res ) { //������ �ਧ��� ⮣�, �� ᫮� ��⠫��� �०���
              tst();
              LayerQry.SetVariable("layer_type", EncodeCompLayerType(cltProtBeforePay));
              LayerQry.SetVariable("crs_pax_id", iPax->crs_pax_id);
              if (curr_tid!=NoExists)
                LayerQry.SetVariable("tid", curr_tid);
              else
                LayerQry.SetVariable("tid", FNull);
              LayerQry.Execute();
              curr_tid=(LayerQry.VariableIsNULL("tid")?NoExists:LayerQry.GetVariableAsInteger("tid"));
              ProgTrace( TRACE5, "curr_tid=%d", curr_tid );
            }
            //UsePriorContext=true; //!!!vlad
            /*


            vector<TSeatRange> ranges(1,TSeatRange(TSeat(iSeat->first.yname,
                                                         iSeat->first.xname),
                                                   TSeat(iSeat->first.yname,
                                                         iSeat->first.xname)));

            LayerQry.SetVariable("delete_seat_ranges", 1);
            LayerQry.SetVariable("point_id", point_id_tlg);
            LayerQry.SetVariable("airp_arv", airp_arv);
            LayerQry.SetVariable("layer_type", EncodeCompLayerType(cltProtBeforePay));
            LayerQry.SetVariable("first_xname", iSeat->first.xname);
            LayerQry.SetVariable("last_xname", iSeat->first.xname);
            LayerQry.SetVariable("first_yname", iSeat->first.yname);
            LayerQry.SetVariable("last_yname", iSeat->first.yname);
            LayerQry.SetVariable("crs_pax_id", iPax->crs_pax_id);
            LayerQry.Execute();
            if (LayerQry.GetVariableAsInteger("delete_seat_ranges")!=0)
            {
              DeleteTlgSeatRanges(cltProtBeforePay, iPax->crs_pax_id, curr_tid, point_ids_spp);
              InsertTlgSeatRanges(point_id_tlg,
                                  airp_arv,
                                  cltProtBeforePay,
                                  ranges,
                                  iPax->crs_pax_id,
                                  NoExists,
                                  time_limit,
                                  UsePriorContext,
                                  curr_tid,
                                  point_ids_spp);
              UsePriorContext=true;
            };*/
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
    if (error_exists) return; //�᫨ ���� �訡��, ��� �� ��ࠡ�⪨ ᥣ����
    
    //�ନ஢���� �⢥�
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
            if ( iSeat->first.WebTariff.value != 0.0 ) { // �᫨ ���⭠� ॣ������ �⪫�祭�, value=0.0 � �� ��砥
            	xmlNodePtr rateNode = NewTextChild( paxNode, "rate" );
            	NewTextChild( rateNode, "color", iSeat->first.WebTariff.color );
            	ostringstream buf;
              buf << std::fixed << setprecision(2) << iSeat->first.WebTariff.value;
            	NewTextChild( rateNode, "value", buf.str() );
            	NewTextChild( rateNode, "currency", iSeat->first.WebTariff.currency_id );
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

  //����� ��窠 ३ᮢ � ���浪� ���஢�� point_id
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

////////////////////////////////////MERIDIAN SYSTEM/////////////////////////////
void WebRequestsIface::GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string airline;
  int flt_no;
  string str_flt_no;
  string suffix;
  string str_scd_out;
  TDateTime scd_out;
  string airp_dep;
  string region;
  TElemFmt fmt;
  
  xmlNodePtr node = GetNode( "airline", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airline' not found" );
  airline = NodeAsString( node );
  airline = ElemToElemId( etAirline, airline, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRLINE.INVALID",
    	                   LParams()<<LParam("airline",NodeAsString(node)) );
  node = GetNode( "flt_no", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'flt_no' not found" );
  str_flt_no =  NodeAsString( node );
	if ( StrToInt( str_flt_no.c_str(), flt_no ) == EOF ||
		   flt_no > 99999 || flt_no <= 0 )
		throw UserException( "MSG.FLT_NO.INVALID",
			                   LParams()<<LParam("flt_no", str_flt_no) );
  node = GetNode( "suffix", reqNode );
  if ( node != NULL ) {
    suffix =  NodeAsString( node );
    if ( !suffix.empty() ) {
      suffix = ElemToElemId( etSuffix, suffix, fmt );
      if ( fmt == efmtUnknown )
        throw UserException( "MSG.SUFFIX.INVALID",
    	                       LParams()<<LParam("suffix",NodeAsString(node)) );
    }
  }
  node = GetNode( "scd_out", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'scd_out' not found" );
  str_scd_out = NodeAsString( node );
  ProgTrace( TRACE5, "str_scd_out=|%s|", str_scd_out.c_str() );
  if ( str_scd_out.empty() )
		throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
	else
		if ( BASIC::StrToDateTime( str_scd_out.c_str(), "dd.mm.yyyy hh:nn", scd_out ) == EOF )
			throw UserException( "MSG.FLIGHT_DATE.INVALID",
				                   LParams()<<LParam("scd_out", str_scd_out) );
	node = GetNode( "airp_dep", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag 'airp_dep' not found" );
  airp_dep = NodeAsString( node );
  airp_dep = ElemToElemId( etAirp, airp_dep, fmt );
  if ( fmt == efmtUnknown )
    throw UserException( "MSG.AIRP.INVALID_INPUT_VALUE",
    	                   LParams()<<LParam("airp",NodeAsString(node)) );
  TReqInfo *reqInfo = TReqInfo::Instance();
  region = AirpTZRegion( airp_dep );
  scd_out = LocalToUTC( scd_out, region );
  ProgTrace( TRACE5, "scd_out=%f", scd_out );
  if ( !reqInfo->CheckAirline( airline ) ||
       !reqInfo->CheckAirp( airp_dep ) )
    throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
    
  int findMove_id, point_id;
  if ( !TPoints::isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp_dep, ASTRA::NoExists, scd_out,
        findMove_id, point_id  ) )
     throw UserException( "MSG.FLIGHT.NOT_FOUND" );

	TFlightStations stations;
	stations.Load( point_id );
	TFlightStages stages;
	stages.Load( point_id );
	TCkinClients CkinClients;
 TTripStages::ReadCkinClients( point_id, CkinClients );
	xmlNodePtr flightNode = NewTextChild( resNode, "trip" );
	airline += str_flt_no + suffix;
	SetProp( flightNode, "flightNumber", airline );
  SetProp( flightNode, "date", DateTimeToStr( UTCToClient( scd_out, region ), "dd.mm.yyyy hh:nn" ) );
  SetProp( flightNode, "departureAirport", airp_dep );
  node = NewTextChild( flightNode, "stages" );
  CreateXMLStage( CkinClients, sPrepCheckIn, stages.GetStage( sPrepCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenCheckIn, stages.GetStage( sOpenCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseCheckIn, stages.GetStage( sCloseCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenBoarding, stages.GetStage( sOpenBoarding ), node, region );
  CreateXMLStage( CkinClients, sCloseBoarding, stages.GetStage( sCloseBoarding ), node, region );
  CreateXMLStage( CkinClients, sOpenWEBCheckIn, stages.GetStage( sOpenWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseWEBCheckIn, stages.GetStage( sCloseWEBCheckIn ), node, region );
  CreateXMLStage( CkinClients, sOpenKIOSKCheckIn, stages.GetStage( sOpenKIOSKCheckIn ), node, region );
  CreateXMLStage( CkinClients, sCloseKIOSKCheckIn, stages.GetStage( sCloseKIOSKCheckIn ), node, region );
  tstations sts;
  stations.Get( sts );
  xmlNodePtr node1 = NULL;
  for ( tstations::iterator i=sts.begin(); i!=sts.end(); i++ ) {
    if ( node1 == NULL )
      node1 = NewTextChild( flightNode, "stations" );
    SetProp( NewTextChild( node1, "station", i->name ), "work_mode", i->work_mode );
  }
}


void WebRequestsIface::GetCacheTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr n = GetNode( "table_name", reqNode );
  if ( n == NULL ) {
    throw EXCEPTIONS::Exception( "tag 'table_name' not found'" );
  }
  string table_name = NodeAsString( n );
  table_name = lowerc( table_name );
  if ( table_name != "rcpt_doc_types" ) {
    throw EXCEPTIONS::Exception( "invalid table_name %s", table_name.c_str() );
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
      "SELECT tid FROM rcpt_doc_types WHERE tid>:tid AND rownum<2";
    Qry.CreateVariable( "tid", otInteger, tid );
    Qry.Execute();
    if ( Qry.Eof ) {
      NewTextChild( n, "tid", tid );
      return;
    }
    tid = ASTRA::NoExists;
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT id,code,code code_lat,name,name_lat,pr_del,tid FROM pax_doc_countries ORDER BY code";
  Qry.Execute();
  xmlNodePtr node = NewTextChild( n, "data" );
  for ( ; !Qry.Eof; Qry.Next() ) {
    xmlNodePtr rowNode = NewTextChild( node, "row" );
    NewTextChild( rowNode, "code", Qry.FieldAsString( "code" ) );
    NewTextChild( rowNode, "code_lat", Qry.FieldAsString( "code_lat" ) );
    NewTextChild( rowNode, "name", Qry.FieldAsString( "name" ) );
    NewTextChild( rowNode, "name_lat", Qry.FieldAsString( "name_lat" ) );
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
  //ࠧ��ઠ ⥫��ࠬ�� ���� ssm
  TQuery Qry(&OraSession);
  Qry.SQLText = "insert into ssm_in(id, type, data) values(ssm_in__seq.nextval, :type, :data)";
  Qry.CreateVariable("data", otString, body);
  Qry.CreateVariable("type", otString, stype);
  Qry.Execute();
}

struct Tids {
  int pax_tid;
  int grp_tid;
  Tids( ) {
    pax_tid = -1;
    grp_tid = -1;
  };
  Tids( int vpax_tid, int vgrp_tid ) {
    pax_tid = vpax_tid;
    grp_tid = vgrp_tid;
  };
};

const char * SyncMeridian_airlines[] =
    {"��", "��", "QU" };

bool is_sync_meridian( const TTripInfo &tripInfo )
{
  for( unsigned int i=0;i<sizeof(SyncMeridian_airlines)/sizeof(SyncMeridian_airlines[0]);i+=1) {
   if ( strcmp(tripInfo.airline.c_str(),SyncMeridian_airlines[i])==0 )
     return true;
  }
  return false;
}

void WebRequestsIface::GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "@time", reqNode );
  if ( node == NULL )
    throw AstraLocale::UserException( "Tag '@time' not found" );
  string str_date = NodeAsString( node );
  TDateTime vdate, vpriordate;
  if ( BASIC::StrToDateTime( str_date.c_str(), "dd.mm.yyyy hh:nn:ss", vdate ) == EOF )
		throw UserException( "Invalid tag value '@time'" );
  bool pr_reset = ( GetNode( "@reset", reqNode ) != NULL );
  string prior_paxs;
  map<int,Tids> Paxs; // pax_id, <pax_tid, grp_tid> ᯨ᮪ ���ᠦ�஢ ��।����� ࠭��
  xmlDocPtr paxsDoc;
  if ( !pr_reset && AstraContext::GetContext( "meridian_sync", 0, prior_paxs ) != NoExists ) {
    paxsDoc = TextToXMLTree( prior_paxs );
    try {
      xmlNodePtr nodePax = paxsDoc->children;
      node = GetNode( "@time", nodePax );
      if ( node == NULL )
        throw AstraLocale::UserException( "Tag '@time' not found in context" );
      if ( BASIC::StrToDateTime( NodeAsString( node ), "dd.mm.yyyy hh:nn:ss", vpriordate ) == EOF )
		    throw UserException( "Invalid tag value '@time' in context" );
      if ( vpriordate == vdate ) { // ࠧ��� ��ॢ� �� �᫮���, �� �।��騩 ����� �� ��।�� ��� ���ᠦ�஢ �� ������� ������ �६���
        nodePax = nodePax->children;
        for ( ; nodePax!=NULL && string((char*)nodePax->name) == "pax"; nodePax=nodePax->next ) {
          Paxs[ NodeAsInteger( "@pax_id", nodePax ) ] = Tids( NodeAsInteger( "@pax_tid", nodePax ), NodeAsInteger( "@grp_tid", nodePax ) );
          ProgTrace( TRACE5, "pax_id=%d, pax_tid=%d, grp_tid=%d",
                     NodeAsInteger( "@pax_id", nodePax ),
                     NodeAsInteger( "@pax_tid", nodePax ),
                     NodeAsInteger( "@grp_tid", nodePax ) );
        }
      }
    }
    catch( ... ) {
      xmlFreeDoc( paxsDoc );
      throw;
    }
    xmlFreeDoc( paxsDoc );
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pax_id,reg_no,work_mode,point_id,desk,client_type,time "
    " FROM aodb_pax_change "
    "WHERE time >= :time AND time <= :uptime "
    "ORDER BY time, pax_id, work_mode ";
  TDateTime nowUTC = NowUTC();
  if ( nowUTC - 1 > vdate )
    vdate = nowUTC - 1;
  Qry.CreateVariable( "time", otDate, vdate );
  Qry.CreateVariable( "uptime", otDate, nowUTC - 1.0/1440.0 );
  Qry.Execute();
  TQuery PaxQry(&OraSession);
  PaxQry.SQLText =
	   "SELECT pax.pax_id,pax.reg_no,pax.surname||RTRIM(' '||pax.name) name,"
     "       pax_grp.grp_id,"
	   "       pax_grp.airp_arv,pax_grp.airp_dep,"
     "       pax_grp.class,pax.refuse,"
	   "       pax.pers_type, "
     "       NVL(pax.is_female,1) as is_female, "
	   "       pax.subclass, "
	   "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'tlg',rownum) AS seat_no, "
	   "       pax.seats seats, "
	   "       ckin.get_excess(pax_grp.grp_id,pax.pax_id) excess,"
	   "       ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkamount,"
	   "       ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkweight,"
	   "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagamount,"
	   "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagweight,"
     "       ckin.get_bag_pool_pax_id(pax.grp_id,pax.bag_pool_num) AS bag_pool_pax_id, "
     "       pax.bag_pool_num, "
	   "       pax.pr_brd, "
	   "       pax_grp.status, "
	   "       pax_grp.client_type, "
	   "       pax_doc.no document, "
	   "       pax.ticket_no, pax.tid pax_tid, pax_grp.tid grp_tid "
	   " FROM pax_grp, pax, pax_doc "
	   " WHERE pax_grp.grp_id=pax.grp_id AND "
	   "       pax.pax_id=:pax_id AND "
	   "       pax.wl_type IS NULL AND "
	   "       pax.pax_id=pax_doc.pax_id(+) ";
  PaxQry.DeclareVariable( "pax_id", otInteger );
  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable( "pax_id", otInteger );
  TQuery FltQry(&OraSession);
  FltQry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  FltQry.DeclareVariable( "point_id", otInteger );
  TDateTime max_time = NoExists;
  node = NULL;
  string res;
  int pax_count = 0;
  int prior_pax_id = -1;
  Tids tids;
  // �஡�� �� �ᥬ ���ᠦ�ࠬ � ������ �६� ����� ��� ࠢ�� ⥪�饬�
  for ( ;!Qry.Eof && pax_count<=500; Qry.Next() ) {
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    if ( pax_id == prior_pax_id ) // 㤠�塞 �㡫�஢���� ��ப� � ����� � ⥬ �� pax_id ��� ॣ����樨 � ��ᠤ��
      continue; // �।��騩 ���ᠦ�� �� �� � ⥪�騩
    ProgTrace( TRACE5, "pax_id=%d", pax_id );
    prior_pax_id = pax_id;
    FltQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
    FltQry.Execute();
    if ( FltQry.Eof )
      throw EXCEPTIONS::Exception("WebRequestsIface::GetPaxsInfo: flight not found, (point_id=%d)", Qry.FieldAsInteger( "point_id" ) );
    TTripInfo tripInfo( FltQry );
    if ( TReqInfo::Instance()->client_type != ctWeb || //��-��襬� ��ਤ��� �������� �⭮襭�� � ���-ॣ����樨 �� �����
         !is_sync_meridian( tripInfo ) ) {             //�� ����뢠���� � ⠡��� web_clients ��� ���-ॣ������!
      continue;
    }

    if ( max_time != NoExists && max_time != Qry.FieldAsDateTime( "time" ) ) { // �ࠢ����� �६��� � �।. ���祭���, �᫨ ����������, �
      ProgTrace( TRACE5, "Paxs.clear(), vdate=%s, max_time=%s",
                 DateTimeToStr( vdate, ServerFormatDateTimeAsString ).c_str(),
                 DateTimeToStr( max_time, ServerFormatDateTimeAsString ).c_str() );
      Paxs.clear(); // ���������� �६� - 㤠�塞 ��� �।���� ���ᠦ�஢ � �।. �६����
    }
    max_time = Qry.FieldAsDateTime( "time" );
    PaxQry.SetVariable( "pax_id", pax_id );
    PaxQry.Execute();
    if ( !PaxQry.Eof ) {
      tids.pax_tid = PaxQry.FieldAsInteger( "pax_tid" );
      tids.grp_tid = PaxQry.FieldAsInteger( "grp_tid" );
    }
    else {
      tids.pax_tid = -1; // ���ᠦ�� �� 㤠��� � �।. ࠧ
      tids.grp_tid = -1;
    }
    // ���ᠦ�� ��।������ � �� ���������
    if ( Paxs.find( pax_id ) != Paxs.end() &&
         Paxs[ pax_id ].pax_tid == tids.pax_tid &&
         Paxs[ pax_id ].grp_tid == tids.grp_tid )
      continue; // 㦥 ��।����� ���ᠦ��
    // ���ᠦ�� �� ��।����� ��� �� ���������
    if ( node == NULL )
      node = NewTextChild( resNode, "passengers" );
    Paxs[ pax_id ] = tids; // ���������
    pax_count++;
    xmlNodePtr paxNode = NewTextChild( node, "pax" );
    NewTextChild( paxNode, "pax_id", pax_id );
    NewTextChild( paxNode, "point_id", Qry.FieldAsInteger( "point_id" ) );
    if ( PaxQry.Eof ) {
      NewTextChild( paxNode, "status", "delete" );
      continue;
    }
    NewTextChild( paxNode, "flight", string(FltQry.FieldAsString( "airline" )) + FltQry.FieldAsString( "flt_no" ) + FltQry.FieldAsString( "suffix" ) );
    NewTextChild( paxNode, "scd_out", DateTimeToStr( FltQry.FieldAsDateTime( "scd_out" ), ServerFormatDateTimeAsString ) );
    NewTextChild( paxNode, "grp_id", PaxQry.FieldAsInteger( "grp_id" ) );
    NewTextChild( paxNode, "name", PaxQry.FieldAsString( "name" ) );
    NewTextChild( paxNode, "class", PaxQry.FieldAsString( "class" ) );
    NewTextChild( paxNode, "subclass", PaxQry.FieldAsString( "subclass" ) );
    NewTextChild( paxNode, "pers_type", PaxQry.FieldAsString( "pers_type" ) );
    if ( DecodePerson( PaxQry.FieldAsString( "pers_type" ) ) == ASTRA::adult ) {
      NewTextChild( paxNode, "gender", (PaxQry.FieldAsInteger("is_female")==0?"M":"F") );
    }
    NewTextChild( paxNode, "airp_dep", PaxQry.FieldAsString("airp_dep") );
    NewTextChild( paxNode, "airp_arv", PaxQry.FieldAsString("airp_arv") );
    NewTextChild( paxNode, "seat_no", PaxQry.FieldAsString("seat_no") );
    NewTextChild( paxNode, "seats", PaxQry.FieldAsInteger("seats") );
    NewTextChild( paxNode, "excess", PaxQry.FieldAsInteger( "excess" ) );
    NewTextChild( paxNode, "rkamount", PaxQry.FieldAsInteger( "rkamount" ) );
    NewTextChild( paxNode, "rkweight", PaxQry.FieldAsInteger( "rkweight" ) );
    NewTextChild( paxNode, "bagamount", PaxQry.FieldAsInteger( "bagamount" ) );
    NewTextChild( paxNode, "bagweight", PaxQry.FieldAsInteger( "bagweight" ) );
    if ( PaxQry.FieldIsNULL( "pr_brd" ) )
      NewTextChild( paxNode, "status", "uncheckin" );
    else
      if ( PaxQry.FieldAsInteger( "pr_brd" ) == 0 )
        NewTextChild( paxNode, "status", "checkin" );
      else
        NewTextChild( paxNode, "status", "boarded" );
    NewTextChild( paxNode, "client_type", PaxQry.FieldAsString( "client_type" ) );
    res.clear();
    TCkinRoute ckinRoute;
    if ( ckinRoute.GetRouteAfter( PaxQry.FieldAsInteger( "grp_id" ), crtNotCurrent, crtIgnoreDependent ) ) { // ���� ᪢����� ॣ������
      xmlNodePtr rnode = NewTextChild( paxNode, "tckin_route" );
      int seg_no=1;
      for ( vector<TCkinRouteItem>::iterator i=ckinRoute.begin(); i!=ckinRoute.end(); i++ ) {
         xmlNodePtr inode = NewTextChild( rnode, "seg" );
         SetProp( inode, "num", seg_no );
         NewTextChild( inode, "flight", i->operFlt.airline + IntToString(i->operFlt.flt_no) + i->operFlt.suffix );
         NewTextChild( inode, "airp_dep", i->airp_dep );
         NewTextChild( inode, "airp_arv", i->airp_arv );
         NewTextChild( inode, "scd_out", DateTimeToStr( i->operFlt.scd_out, ServerFormatDateTimeAsString ) );
         seg_no++;
      }
    }
    RemQry.SetVariable( "pax_id", pax_id );
    RemQry.Execute();
    res.clear();
    for ( ;!RemQry.Eof; RemQry.Next() ) {
      if ( !res.empty() )
        res += "\n";
      res += RemQry.FieldAsString( "rem" );
    }
    if ( !res.empty() )
      NewTextChild( paxNode, "rems", res );
    NewTextChild( paxNode, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
  } //end for
  if ( max_time == NoExists )
    max_time = vdate; // �� ��࠭� �� ������ ���ᠦ�� - ��।��� �६�, ����� ��諮 � �ନ����
  SetProp( resNode, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
  AstraContext::ClearContext( "meridian_sync", 0 );
  if ( !Paxs.empty() ) { // ���� ���ᠦ��� - ��࠭塞 ��� ��।�����
    paxsDoc = CreateXMLDoc( "paxs" );
    try {
      node = paxsDoc->children;
      SetProp( node, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
      for ( map<int,Tids>::iterator p=Paxs.begin(); p!=Paxs.end(); p++ ) {
        xmlNodePtr n = NewTextChild( node, "pax" );
        SetProp( n, "pax_id", p->first );
        SetProp( n, "pax_tid", p->second.pax_tid );
        SetProp( n, "grp_tid", p->second.grp_tid );
      }
      AstraContext::SetContext( "meridian_sync", 0, XMLTreeToText( paxsDoc ) );
      ProgTrace( TRACE5, "xmltreetotext=%s", XMLTreeToText( paxsDoc ).c_str() );
    }
    catch( ... ) {
      xmlFreeDoc( paxsDoc );
      throw;
    }
    xmlFreeDoc( paxsDoc );
  }
}
////////////////////////////////////END MERIDIAN SYSTEM/////////////////////////////


} //end namespace AstraWeb

namespace TypeB
{

void SyncCHKD(int point_id_tlg, int point_id_spp, bool sync_all) //ॣ������ CHKD
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  ASTRA::TClientType prior_client_type=reqInfo->client_type;
  TUser prior_user=reqInfo->user;
  try
  {
    reqInfo->client_type=ctPNL;
    reqInfo->user.user_type=utSupport;
    reqInfo->user.access.airlines.clear();
    reqInfo->user.access.airps.clear();
    reqInfo->user.access.airlines_permit=false;
    reqInfo->user.access.airps_permit=false;


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
      "ORDER BY reg_no, surname, name, crs_pax_id "; //�஡㥬 ��ࠡ��뢠�� � ���浪� ॣ����樮���� ����஢ - ⠪ �ࠢ�������
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
      PNRs.insert(PNRs.begin(), pnrData); //��⠢�塞 � ��砫� ���� ᥣ����
      for(;!Qry.Eof;Qry.Next())
      {
        int crs_pax_id=Qry.FieldAsInteger("crs_pax_id");
        list<int> crs_pax_ids;
        if (!Qry.FieldIsNULL("pax_id"))
          //���ᠦ�� ��ॣ����஢��
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

            if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
            {
              ProgTrace(TRACE5, "SyncCHKD:\n%s", XMLTreeToText(emulCkinDoc.docPtr()).c_str());

              xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
              if (emulReqNode==NULL)
                throw EXCEPTIONS::Exception("emulReqNode=NULL");

              int first_grp_id, tckin_id;
              TChangeStatusList ChangeStatusInfo;
              CheckInInterface::SavePax(emulReqNode, NULL/*ediResNode*/, first_grp_id, ChangeStatusInfo, tckin_id);
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

