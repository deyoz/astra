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
#include "astra_callbacks.h"
#include "tlg/tlg.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/query_runner.h"
#include "jxtlib/xmllibcpp.h"
#include "jxtlib/xml_stuff.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

namespace AstraWeb
{

using namespace std;
using namespace ASTRA;
using namespace SEATS2;
using namespace BASIC;
using namespace AstraLocale;

const string PARTITION_ELEM_TYPE = "П";
const string ARMCHAIR_ELEM_TYPE = "К";

int readInetClientId(const char *head)
{
  short grp;
  memcpy(&grp,head+45,2);
  return ntohs(grp);
}

InetClient getInetClient(int client_id)
{
  InetClient client;
  client.client_id = client_id;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT client_type,web_clients.desk,login "
    "FROM web_clients,users2 "
    "WHERE web_clients.id=:client_id AND "
    "      web_clients.user_id=users2.user_id";
  Qry.CreateVariable( "client_id", otInteger, client_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
    client.pult = Qry.FieldAsString( "desk" );
    client.opr = Qry.FieldAsString( "login" );
    client.client_type = Qry.FieldAsString( "client_type" );
  }
  return client;
}

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len)
{
  InitLogTime(0);
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
    InetClient client=getInetClient(client_id);
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


    static ServerFramework::ApplicationCallbacks *ac=
             ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();
    newlen=ac->jxt_proc((const char *)new_body.data(),new_body.size(),(const char *)new_header.data(),new_header.size(),res,len);
    ProgTrace(TRACE1,"newlen=%i",newlen);

    memcpy(*res,head,hlen);


  }
  catch(...)
  {
    answer="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<error/>";
    newlen=answer.size()+hlen; // размер ответа (с заголовком)
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

  InitLogTime(0);
  return newlen;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

struct TIdsPnrData {
  int point_id;
  int pnr_id;
  bool pr_paid_ckin;
};

void VerifyPNR( int point_id, int pnr_id )
{
	TQuery Qry(&OraSession);
  if (!isTestPaxId(pnr_id))
  {
  	Qry.SQLText =
      "SELECT point_id_spp "
      "FROM crs_pnr,crs_pax,tlg_binding "
      "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
      "      crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND "
      "      crs_pnr.pnr_id=:pnr_id AND crs_pax.pr_del=0 AND "
      "      tlg_binding.point_id_spp(+)=:point_id AND rownum<2";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
  	Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
    if ( Qry.FieldIsNULL( "point_id_spp" ) )
    	throw UserException( "MSG.PASSENGERS.OTHER_FLIGHT" );
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:pnr_id";
    Qry.CreateVariable( "pnr_id", otInteger, pnr_id );
  	Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );
  };
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
      filter.traceToMonitor(TRACE5, "WebRequestsIface::SearchFlt: <pnr_addr>, <ticket_no>, <document> not defined");
      throw UserException("MSG.NOTSET.SEARCH_PARAMS");
    };
  };
  filter.testPaxFromDB();
  filter.trace(TRACE5);
  
  WebSearch::TPNRs PNRs;
  findPNRs(filter, PNRs, 1);
  if (PNRs.pnrs.empty() && scanCodeNode==NULL)  //если сканирование штрих-кода, тогда только поиск по оперирующему перевозчику
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
1. Если кто-то уже начал работать с pnr (агент,разборщик PNL)
2. Если пассажир зарегистрировался, а разборщик PNL ставит признак удаления
*/

struct TWebPnr {
  TCheckDocInfo checkDocInfo;
  TCheckDocTknInfo checkTknInfo;
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
  if ((checkDocInfo.first.required_fields&doc.getNotEmptyFieldsMask())!=checkDocInfo.first.required_fields) return false;
  return true;
};

bool is_valid_doco_info(const TCheckDocInfo &checkDocInfo,
                        const CheckIn::TPaxDocoItem &doco)
{
  if ((checkDocInfo.second.required_fields&doco.getNotEmptyFieldsMask())!=checkDocInfo.second.required_fields) return false;
  return true;
};

bool is_valid_tkn_info(const TCheckDocTknInfo &checkTknInfo,
                       const CheckIn::TPaxTknItem &tkn)
{
  if ((checkTknInfo.required_fields&tkn.getNotEmptyFieldsMask())!=checkTknInfo.required_fields) return false;
  return true;
};

void checkDocInfoToXML(const TCheckDocInfo &checkDocInfo,
                       const xmlNodePtr node)
{
  if (node==NULL) return;
  xmlNodePtr fieldsNode=NewTextChild(node, "doc_required_fields");
  SetProp(fieldsNode, "is_inter", checkDocInfo.first.is_inter);
  if ((checkDocInfo.first.required_fields&DOC_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkDocInfo.first.required_fields&DOC_ISSUE_COUNTRY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_country");
  if ((checkDocInfo.first.required_fields&DOC_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkDocInfo.first.required_fields&DOC_NATIONALITY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "nationality");
  if ((checkDocInfo.first.required_fields&DOC_BIRTH_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_date");
  if ((checkDocInfo.first.required_fields&DOC_GENDER_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "gender");
  if ((checkDocInfo.first.required_fields&DOC_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkDocInfo.first.required_fields&DOC_SURNAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "surname");
  if ((checkDocInfo.first.required_fields&DOC_FIRST_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "first_name");
  if ((checkDocInfo.first.required_fields&DOC_SECOND_NAME_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "second_name");
    
  fieldsNode=NewTextChild(node, "doco_required_fields");
  SetProp(fieldsNode, "is_inter", checkDocInfo.second.is_inter);
  if ((checkDocInfo.second.required_fields&DOCO_BIRTH_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "birth_place");
  if ((checkDocInfo.second.required_fields&DOCO_TYPE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "type");
  if ((checkDocInfo.second.required_fields&DOCO_NO_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "no");
  if ((checkDocInfo.second.required_fields&DOCO_ISSUE_PLACE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_place");
  if ((checkDocInfo.second.required_fields&DOCO_ISSUE_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "issue_date");
  if ((checkDocInfo.second.required_fields&DOCO_EXPIRY_DATE_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "expiry_date");
  if ((checkDocInfo.second.required_fields&DOCO_APPLIC_COUNTRY_FIELD) != 0x0000)
    NewTextChild(fieldsNode, "field", "applic_country");
};

string PaxDocCountryToXML(const string &pax_doc_country)
{
  string result;
  if (!pax_doc_country.empty())
  {
    try
    {
      result=getBaseTable(etPaxDocCountry).get_row("code",pax_doc_country).AsString("country");
    }
    catch (EBaseTableError) {};
    if (result.empty()) result=pax_doc_country;
  };
  return result;
};

string ElemToPaxDocCountryId(const string &elem, TElemFmt &fmt)
{
  string result=ElemToElemId(etPaxDocCountry,elem,fmt);
  if (fmt==efmtUnknown)
  {
    //проверим countries
    string country=ElemToElemId(etCountry,elem,fmt);
    if (fmt!=efmtUnknown)
    {
      fmt=efmtUnknown;
      //найдем в pax_doc_countries.country
      try
      {
        result=ElemToElemId(etPaxDocCountry,
                            getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code"),
                            fmt);
      }
      catch (EBaseTableError) {};
    };
  };
  return result;
};

bool CheckDocNumber(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' )
         ))
      return false;
  errorIdx=string::npos;
  return true;
};

bool CheckDocSurname(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || *i==' ' || *i=='-' )
         ))
      return false;
  errorIdx=string::npos;
  return true;
};

bool CheckDocPlace(const string &str, const TCheckDocTknInfo &checkDocInfo, string::size_type &errorIdx)
{
  errorIdx=0;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i, errorIdx++)
    if (!(( !checkDocInfo.is_inter || IsAscii7(*i) ) &&
          ( IsUpperLetter(*i) || IsDigit(*i) || *i==' ' || *i=='-' )
         ))
      return false;
  errorIdx=string::npos;
  return true;
};

void CheckDoc(const CheckIn::TPaxDocItem &doc,
              const TCheckDocTknInfo &checkDocInfo,
              TDateTime nowLocal)
{
  string::size_type errorIdx;
  
  modf(nowLocal, &nowLocal);
  
  if (doc.birth_date!=NoExists && doc.birth_date>nowLocal)
    throw UserException("MSG.CHECK_DOC.INVALID_BIRTH_DATE", LParams()<<LParam("fieldname", "document/birth_date" ));
    
  if (doc.expiry_date!=NoExists && doc.expiry_date<nowLocal)
    throw UserException("MSG.CHECK_DOC.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "document/expiry_date" ));
  
  if (!CheckDocNumber(doc.no, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" ));

  if (!CheckDocSurname(doc.surname, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" ));
    
  if (!CheckDocSurname(doc.first_name, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" ));
    
  if (!CheckDocSurname(doc.second_name, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" ));
};

CheckIn::TPaxDocItem NormalizeDoc(const CheckIn::TPaxDocItem &doc)
{
  CheckIn::TPaxDocItem result;
  TElemFmt fmt;

  if (!doc.type.empty())
  {
    result.type=ElemToElemId(etPaxDocType, upperc(doc.type), fmt);
    if (fmt==efmtUnknown || result.type=="V")
      throw UserException("MSG.CHECK_DOC.INVALID_TYPE", LParams()<<LParam("fieldname", "document/type" ));
  };
  if (!doc.issue_country.empty())
  {
    result.issue_country=ElemToPaxDocCountryId(upperc(doc.issue_country), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_ISSUE_COUNTRY", LParams()<<LParam("fieldname", "document/issue_country" ));
  };
  
  result.no=upperc(doc.no);
  if (result.no.size()>15)
    throw UserException("MSG.CHECK_DOC.INVALID_NO", LParams()<<LParam("fieldname", "document/no" ));
  
  if (!doc.nationality.empty())
  {
    result.nationality=ElemToPaxDocCountryId(upperc(doc.nationality), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_NATIONALITY", LParams()<<LParam("fieldname", "document/nationality" ));
  };
  
  if (doc.birth_date!=NoExists)
    modf(doc.birth_date, &result.birth_date);
  
  if (!doc.gender.empty())
  {
    result.gender=ElemToElemId(etGenderType, upperc(doc.gender), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOC.INVALID_GENDER", LParams()<<LParam("fieldname", "document/gender" ));
  };
  
  if (doc.expiry_date!=NoExists)
    modf(doc.expiry_date, &result.expiry_date);
    
  result.surname=upperc(doc.surname);
  if (result.surname.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_SURNAME", LParams()<<LParam("fieldname", "document/surname" ));
  
  result.first_name=upperc(doc.first_name);
  if (result.first_name.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_FIRST_NAME", LParams()<<LParam("fieldname", "document/first_name" ));
  
  result.second_name=upperc(doc.second_name);
  if (result.second_name.size()>64)
    throw UserException("MSG.CHECK_DOC.INVALID_SECOND_NAME", LParams()<<LParam("fieldname", "document/second_name" ));

  return result;
};

void CheckDoco(const CheckIn::TPaxDocoItem &doc,
               const TCheckDocTknInfo &checkDocInfo,
               TDateTime nowLocal)
{
  string::size_type errorIdx;

  modf(nowLocal, &nowLocal);
  
  if (doc.issue_date!=NoExists && doc.issue_date>nowLocal)
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_DATE", LParams()<<LParam("fieldname", "doco/issue_date" ));

  if (doc.expiry_date!=NoExists && doc.expiry_date<nowLocal)
    throw UserException("MSG.CHECK_DOCO.INVALID_EXPIRY_DATE", LParams()<<LParam("fieldname", "doco/expiry_date" ));
    
  if (!CheckDocPlace(doc.birth_place, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" ));

  if (!CheckDocNumber(doc.no, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" ));
    
  if (!CheckDocPlace(doc.issue_place, checkDocInfo, errorIdx))
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" ));
};

CheckIn::TPaxDocoItem NormalizeDoco(const CheckIn::TPaxDocoItem &doc)
{
  CheckIn::TPaxDocoItem result;
  TElemFmt fmt;
  
  result.birth_place=upperc(doc.birth_place);
  if (result.birth_place.size()>35)
    throw UserException("MSG.CHECK_DOCO.INVALID_BIRTH_PLACE", LParams()<<LParam("fieldname", "doco/birth_place" ));
  
  if (!doc.type.empty())
  {
    result.type=ElemToElemId(etPaxDocType, upperc(doc.type), fmt);
    if (fmt==efmtUnknown || result.type!="V")
      throw UserException("MSG.CHECK_DOCO.INVALID_TYPE", LParams()<<LParam("fieldname", "doco/type" ));
  };
  
  result.no=upperc(doc.no);
  if (result.no.size()>15)
    throw UserException("MSG.CHECK_DOCO.INVALID_NO", LParams()<<LParam("fieldname", "doco/no" ));
  
  result.issue_place=upperc(doc.issue_place);
  if (result.issue_place.size()>35)
    throw UserException("MSG.CHECK_DOCO.INVALID_ISSUE_PLACE", LParams()<<LParam("fieldname", "doco/issue_place" ));
  
  if (doc.issue_date!=NoExists)
    modf(doc.issue_date, &result.issue_date);
    
  if (doc.expiry_date!=NoExists)
    modf(doc.expiry_date, &result.expiry_date);
  
  if (!doc.applic_country.empty())
  {
    result.applic_country=ElemToPaxDocCountryId(upperc(doc.applic_country), fmt);
    if (fmt==efmtUnknown)
      throw UserException("MSG.CHECK_DOCO.INVALID_APPLIC_COUNTRY", LParams()<<LParam("fieldname", "doco/applic_country" ));
  };
  return result;
};


void PaxDocToXML(const CheckIn::TPaxDocItem &doc,
                 const xmlNodePtr node)
{
  if (node==NULL) return;
  xmlNodePtr docNode=NewTextChild(node,"document");
  NewTextChild(docNode, "type", doc.type);
  NewTextChild(docNode, "issue_country", PaxDocCountryToXML(doc.issue_country));
  NewTextChild(docNode, "no", doc.no);
  NewTextChild(docNode, "nationality", PaxDocCountryToXML(doc.nationality));
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
  NewTextChild(docNode, "applic_country", PaxDocCountryToXML(doco.applic_country));
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
    	TQuery PaxDocQry(&OraSession);
    	TQuery PaxDocoQry(&OraSession);
    	TQuery CrsPaxDocQry(&OraSession);
    	TQuery GetPSPT2Qry(&OraSession);
    	TQuery CrsPaxDocoQry(&OraSession);
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
        pnr.checkDocInfo=GetCheckDocInfo(point_id, Qry.FieldAsString("airp_arv"), pnr.apis_formats);
        pnr.checkTknInfo=GetCheckTknInfo(point_id);
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
        	  //пассажир зарегистрирован
        	  if ( !Qry.FieldIsNULL( "refuse" ) )
        	    pax.checkin_status = "refused";
        	  else
        	  {
        	    pax.checkin_status = "agent_checked";

          	  switch(DecodeClientType(Qry.FieldAsString( "client_type" )))
          	  {
          	    case ctWeb:
          	    case ctKiosk:
        		    	pax.checkin_status = "web_checked";
        		  		break;
        		  	default: ;
          	  };
        	  };
        		pax.pax_id = Qry.FieldAsInteger( "pax_id" );
        		pax.tkn.fromDB(Qry);
        		LoadPaxDoc(pax.pax_id, pax.doc, PaxDocQry);
        		LoadPaxDoco(pax.pax_id, pax.doco, PaxDocoQry);
         		FQTQry.SQLText=PaxFQTQrySQL;
         		FQTQry.SetVariable( "pax_id", pax.pax_id );
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
         		if (!CrsTKNQry.Eof) pax.tkn.fromDB(CrsTKNQry);
         		//ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.tkn.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.tkn.getNotEmptyFieldsMask());
        		LoadCrsPaxDoc(pax.crs_pax_id, pax.doc, CrsPaxDocQry, GetPSPT2Qry);
            //ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doc.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doc.getNotEmptyFieldsMask());
        	  LoadCrsPaxVisa(pax.crs_pax_id, pax.doco, CrsPaxDocoQry);
        		//ProgTrace(TRACE5, "getPnr: pax.crs_pax_id=%d pax.doco.getNotEmptyFieldsMask=%ld", pax.crs_pax_id, pax.doco.getNotEmptyFieldsMask());

         		if (!is_valid_pnr_status(Qry.FieldAsString("pnr_status")))
         		  pax.agent_checkin_reasons.insert("pnr_status");
         		if (!is_valid_pax_status(pax.crs_pax_id, PaxStatusQry))
         		  pax.agent_checkin_reasons.insert("web_cancel");
         		if (!is_valid_doc_info(pnr.checkDocInfo, pax.doc))
         		  pax.agent_checkin_reasons.insert("incomplete_doc");
         		if (!is_valid_doco_info(pnr.checkDocInfo, pax.doco))
         		  pax.agent_checkin_reasons.insert("incomplete_doco");
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
        //тестовый пассажир
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
        //пассажиров первого сегмента проставим pax_no по порядку
        int pax_no=1;
        for(vector<TWebPax>::iterator iPax=pnr.paxs.begin();iPax!=pnr.paxs.end();iPax++,pax_no++) iPax->pax_no=pax_no;
      };

      pnrs.push_back(pnr);

      if (segsNode==NULL) continue;

      xmlNodePtr segNode=NewTextChild(segsNode, "segment");
      NewTextChild( segNode, "point_id", point_id );
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
    int pnr_id=NodeAsInteger( "pnr_id", node );
    WebSearch::TFlightInfo flt;
    flt.fromDB(point_id, false, true);
    flt.fromDBadditional(false, true);
    VerifyPNR( point_id, pnr_id );
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=point_id;
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
  	pr_CHIN=(pr_CHIN || p==ASTRA::child || p==ASTRA::baby); //среди типов может быть БГ (CBBG) который приравнивается к взрослому
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
    salonList.ReadFlight( SALONS2::TFilterRoutesSets( point_id, point_arv ), crs_class );
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
    bool drop_not_web_passes = false;
    salonList.CreateSalonsForAutoSeats( SalonsN,
                                        filterRoutes,
                                        pr_departure_tariff_only,
                                        grp_layers,
                                        pnr,
                                        drop_not_web_passes );
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
     	wp.WebTariff.color = place->WebTariff.color;
     	wp.WebTariff.value = place->WebTariff.value;
     	wp.WebTariff.currency_id = place->WebTariff.currency_id;
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
  int status;
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
  int pnr_id = NodeAsInteger( "pnr_id", reqNode );
  TWebPnr pnr;
  WebSearch::TFlightInfo flt;
  flt.fromDB(point_id, false, true);
  VerifyPNR( point_id, pnr_id );
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
      if ( wp->WebTariff.value != 0.0 ) { // если платная регистрация отключена, value=0.0 в любом случае
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

void CreateEmulXMLDoc(xmlNodePtr reqNode, XMLDoc &emulDoc)
{
  emulDoc.set("UTF-8","term");
  if (emulDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CreateEmulXMLDoc: CreateXMLDoc failed");
  CopyNode(NodeAsNode("/term",emulDoc.docPtr()),
           NodeAsNode("/term/query",reqNode->doc), true); //копируем полностью тег query
  xmlNodePtr node=NodeAsNode("/term/query",emulDoc.docPtr())->children;
  if (node!=NULL)
  {
    xmlUnlinkNode(node);
    xmlFreeNode(node);
  };
};

void CopyEmulXMLDoc(XMLDoc &srcDoc, XMLDoc &destDoc)
{
  destDoc.set("UTF-8","term");
  if (destDoc.docPtr()==NULL)
    throw EXCEPTIONS::Exception("CopyEmulXMLDoc: CreateXMLDoc failed");
  xmlNodePtr destNode=NodeAsNode("/term",destDoc.docPtr());
  xmlNodePtr srcNode=NodeAsNode("/term",srcDoc.docPtr())->children;
  for(;srcNode!=NULL;srcNode=srcNode->next)
    CopyNode(destNode, srcNode, true); //копируем полностью XML
};

void CreateEmulRems(xmlNodePtr paxNode, TQuery &RemQry, const vector<string> &fqtv_rems)
{
  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  for(;!RemQry.Eof;RemQry.Next())
  {
    const char* rem_code=RemQry.FieldAsString("rem_code");
    const char* rem_text=RemQry.FieldAsString("rem");
    if (isDisabledRem(rem_code, rem_text)) continue;
    if (strcmp(rem_code,"FQTV")==0) continue;
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code",rem_code);
    NewTextChild(remNode,"rem_text",rem_text);
  };
  //добавим переданные fqtv_rems
  for(vector<string>::const_iterator r=fqtv_rems.begin();r!=fqtv_rems.end();r++)
  {
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code","FQTV");
    NewTextChild(remNode,"rem_text",*r);
  };
};

struct TWebPaxFromReq
{
  int crs_pax_id;
  string seat_no;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  vector<string> fqt_rems;
  bool doc_present, doco_present, fqt_rems_present;
  bool refuse;
  int crs_pnr_tid;
	int crs_pax_tid;
	int pax_grp_tid;
	int pax_tid;
  TWebPaxFromReq() {
		crs_pax_id = NoExists;
    doc_present = false;
    doco_present = false;
		fqt_rems_present = false;
		refuse = false;
		crs_pnr_tid = NoExists;
		crs_pax_tid	= NoExists;
		pax_grp_tid = NoExists;
		pax_tid = NoExists;
	};
};

struct TWebPaxForChng
{
  int crs_pax_id;
  int grp_id;
  int point_dep;
  int point_arv;
  string airp_dep;
  string airp_arv;
  string cl;
  int excess;
  bool bag_refuse;
  
  string surname;
  string name;
  string pers_type;
  string seat_no;
  int seats;

  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
};

struct TWebPaxForCkin
{
  int crs_pax_id;
  
  string surname;
  string name;
  string pers_type;
  string seat_no;
  string seat_type;
  int seats;
  string eticket;
  string ticket;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  string subclass;
  
  bool operator == (const TWebPaxForCkin &pax) const
	{
  	return transliter_equal(surname,pax.surname) &&
           transliter_equal(name,pax.name) &&
           pers_type==pax.pers_type &&
           ((seats==0 && pax.seats==0) || (seats!=0 && pax.seats!=0));
  };
};

struct TWebPnrForSave
{
  int pnr_id;
  vector<TWebPaxFromReq> paxFromReq;
  unsigned int refusalCountFromReq;
  list<TWebPaxForChng> paxForChng;
  list<TWebPaxForCkin> paxForCkin;

  TWebPnrForSave() {
    pnr_id = NoExists;
    refusalCountFromReq = 0;
  };
};



void VerifyPax(vector< pair<int, TWebPnrForSave > > &segs, XMLDoc &emulDocHeader,
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
  TDateTime now_local=UTCToLocal(NowUTC(), AirpTZRegion(first.flt.oper.airp));
	
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

  const char* PaxRemQrySQL=
      "SELECT rem_code,rem FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";

  const char* CrsPaxRemQrySQL=
      "SELECT rem_code,rem FROM crs_pax_rem "
      "WHERE pax_id=:pax_id AND rem_code NOT IN ('FQTV')";
  TQuery Qry(&OraSession);

  TQuery RemQry(&OraSession);
  RemQry.DeclareVariable("pax_id",otInteger);
  
  TQuery PaxDocQry(&OraSession);
  TQuery PaxDocoQry(&OraSession);
  TQuery GetPSPT2Qry(&OraSession);
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
                    !is_valid_pax_status(iPax->crs_pax_id, PaxStatusQry))
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
              VerifyPNR(point_id,pnr_id);
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
              if (iPax->doc_present)
                //проверка всех реквизитов документа
                pax.doc=NormalizeDoc(iPax->doc);
              if (iPax->doco_present)
                //проверка всех реквизитов визы
                pax.doco=NormalizeDoco(iPax->doco);

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
                pax.doc.clear();
                pax.doc.no = Qry.FieldAsString("doc_no");
                pax.doco.clear();
              }
              else
              {
                if (iPax->doc_present)
                  //проверка всех реквизитов документа
                  pax.doc=NormalizeDoc(iPax->doc);
                else
                  LoadCrsPaxDoc(pax.crs_pax_id, pax.doc, PaxDocQry, GetPSPT2Qry);
                  
                if (iPax->doco_present)
                  //проверка всех реквизитов визы
                  pax.doco=NormalizeDoco(iPax->doco);
                else
                  LoadCrsPaxVisa(pax.crs_pax_id, pax.doco, PaxDocoQry);
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
        VerifyPNR(point_id,s->second.pnr_id);
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
  
  try
  {
    if (!is_test)
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT crs_pnr.pnr_id, airp_arv, subclass, class "
        "FROM crs_pnr, crs_pax "
        "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
        "      crs_pnr.pnr_id=:pnr_id AND "
        "      crs_pax.pr_del=0 AND rownum<2";
      Qry.CreateVariable("pnr_id", otInteger, segs.begin()->second.pnr_id);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

      TTripRoute route; //маршрут рейса
      route.GetRouteAfter( NoExists,
                           first.flt.point_dep,
                           first.flt.point_num,
                           first.flt.first_point,
                           first.flt.pr_tranzit,
                           trtNotCurrent,
                           trtNotCancelled );
                           
      if (!first.seg.fromDB(first.flt.point_dep, route, Qry))
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    }
    else
    {
      TTripRouteItem next;
      TTripRoute().GetNextAirp(NoExists,
                               first.flt.point_dep,
                               first.flt.point_num,
                               first.flt.first_point,
                               first.flt.pr_tranzit,
                               trtNotCancelled,
                               next);
      if (next.point_id==NoExists || next.airp.empty())
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

      TQuery Qry(&OraSession);
      Qry.Clear();
    	Qry.SQLText=
    	  "SELECT subcls.class, subclass, "
    	  "       pnr_airline, pnr_addr "
    	  "FROM test_pax, subcls "
    	  "WHERE test_pax.subclass=subcls.code AND test_pax.id=:pnr_id";
    	Qry.CreateVariable("pnr_id", otInteger, segs.begin()->second.pnr_id);
    	Qry.Execute();
    	if (Qry.Eof)
        throw UserException( "MSG.PASSENGERS.INFO_NOT_FOUND" );

    	first.seg.point_dep=first.flt.point_dep;
      first.seg.point_arv=next.point_id;
      first.seg.pnr_id=segs.begin()->second.pnr_id;
      first.seg.cls=Qry.FieldAsString("class");
      first.seg.subcls=Qry.FieldAsString("subclass");
      WebSearch::TPNRAddrInfo pnr_addr;
      pnr_addr.airline=Qry.FieldAsString("pnr_airline");
      pnr_addr.addr=Qry.FieldAsString("pnr_addr");
      first.seg.pnr_addrs.push_back(pnr_addr);
    };

    first.dest.fromDB(first.seg.point_arv, true);
  }
  catch(CheckIn::UserException)
  {
    throw;
  }
  catch(UserException &e)
  {
    throw CheckIn::UserException(e.getLexemaData(), first.flt.point_dep);
  };


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
  {
    //составляем XML-запрос
    iPnrData=PNRs.begin();
    seg_no=1;
    for(vector< pair<int, TWebPnrForSave > >::const_iterator s=segs.begin(); s!=segs.end(); s++, iPnrData++, seg_no++)
    {
      try
      {
        if (iPnrData==PNRs.end()) //лишние сегменты в запросе на регистрацию
          throw EXCEPTIONS::Exception("VerifyPax: iPnrData==PNRs.end() (seg_no=%d)", seg_no);
          
        TCheckDocInfo checkDocInfo=GetCheckDocInfo(iPnrData->flt.point_dep, iPnrData->dest.airp_arv);

        const TWebPnrForSave &currPnr=s->second;
        //пассажиры для регистрации
        if (!currPnr.paxForCkin.empty())
        {
          if (emulCkinDoc.docPtr()==NULL)
          {
            CopyEmulXMLDoc(emulDocHeader, emulCkinDoc);
            xmlNodePtr emulCkinNode=NodeAsNode("/term/query",emulCkinDoc.docPtr());
            emulCkinNode=NewTextChild(emulCkinNode,"TCkinSavePax");
          	NewTextChild(emulCkinNode,"transfer"); //пустой тег - трансфера нет
            NewTextChild(emulCkinNode,"segments");
            NewTextChild(emulCkinNode,"excess",(int)0);
            NewTextChild(emulCkinNode,"hall");
          };
          xmlNodePtr segsNode=NodeAsNode("/term/query/TCkinSavePax/segments",emulCkinDoc.docPtr());

          xmlNodePtr segNode=NewTextChild(segsNode, "segment");
          NewTextChild(segNode,"point_dep",iPnrData->flt.point_dep);
          NewTextChild(segNode,"point_arv",iPnrData->dest.point_arv);
          NewTextChild(segNode,"airp_dep",iPnrData->flt.oper.airp);
          NewTextChild(segNode,"airp_arv",iPnrData->dest.airp_arv);
          NewTextChild(segNode,"class",iPnrData->seg.cls);
          NewTextChild(segNode,"status",EncodePaxStatus(psCheckin));
          NewTextChild(segNode,"wl_type");

          //коммерческий рейс PNR
          TTripInfo pnrMarkFlt;
          iPnrData->seg.getMarkFlt(iPnrData->flt, is_test, pnrMarkFlt);
          TCodeShareSets codeshareSets;
          codeshareSets.get(iPnrData->flt.oper,pnrMarkFlt);
          
          xmlNodePtr node=NewTextChild(segNode,"mark_flight");
          NewTextChild(node,"airline",pnrMarkFlt.airline);
          NewTextChild(node,"flt_no",pnrMarkFlt.flt_no);
          NewTextChild(node,"suffix",pnrMarkFlt.suffix);
          NewTextChild(node,"scd",DateTimeToStr(pnrMarkFlt.scd_out));  //локальная дата
          NewTextChild(node,"airp_dep",pnrMarkFlt.airp);
          NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);

          xmlNodePtr paxsNode=NewTextChild(segNode,"passengers");
          for(list<TWebPaxForCkin>::const_iterator iPaxForCkin=currPnr.paxForCkin.begin();iPaxForCkin!=currPnr.paxForCkin.end();iPaxForCkin++)
          {
            try
            {
              vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
              for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
                if (iPaxFromReq->crs_pax_id==iPaxForCkin->crs_pax_id) break;
              if (iPaxFromReq==currPnr.paxFromReq.end())
                throw EXCEPTIONS::Exception("VerifyPax: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForCkin->crs_pax_id);

              xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
              NewTextChild(paxNode,"pax_id",iPaxForCkin->crs_pax_id);
              NewTextChild(paxNode,"surname",iPaxForCkin->surname);
              NewTextChild(paxNode,"name",iPaxForCkin->name);
              NewTextChild(paxNode,"pers_type",iPaxForCkin->pers_type);
              if (!iPaxFromReq->seat_no.empty())
                NewTextChild(paxNode,"seat_no",iPaxFromReq->seat_no);
              else
                NewTextChild(paxNode,"seat_no",iPaxForCkin->seat_no);
              NewTextChild(paxNode,"seat_type",iPaxForCkin->seat_type);
              NewTextChild(paxNode,"seats",iPaxForCkin->seats);
              //обработка билетов
              string ticket_no;
              if (!iPaxForCkin->eticket.empty())
              {
                //билет TKNE
                ticket_no=iPaxForCkin->eticket;

                int coupon_no=0;
                string::size_type pos=ticket_no.find_last_of('/');
                if (pos!=string::npos)
                {
                  if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
                      coupon_no>=1 && coupon_no<=4)
                    ticket_no.erase(pos);
                  else
                    coupon_no=0;
                };

                if (ticket_no.empty())
                  throw UserException("MSG.ETICK.NUMBER_NOT_SET");
                NewTextChild(paxNode,"ticket_no",ticket_no);
                if (coupon_no<=0)
                  throw UserException("MSG.ETICK.COUPON_NOT_SET", LParams()<<LParam("etick", ticket_no ) );
                NewTextChild(paxNode,"coupon_no",coupon_no);
                NewTextChild(paxNode,"ticket_rem","TKNE");
                NewTextChild(paxNode,"ticket_confirm",(int)false);
              }
              else
              {
                ticket_no=iPaxForCkin->ticket;

                NewTextChild(paxNode,"ticket_no",ticket_no);
                NewTextChild(paxNode,"coupon_no");
                if (!ticket_no.empty())
                  NewTextChild(paxNode,"ticket_rem","TKNA");
                else
                  NewTextChild(paxNode,"ticket_rem");
                NewTextChild(paxNode,"ticket_confirm",(int)false);
              };
              
              if (iPaxFromReq->doc_present)
                CheckDoc(iPaxForCkin->doc, checkDocInfo.first, now_local);
              iPaxForCkin->doc.toXML(paxNode);
              
              if (iPaxFromReq->doco_present)
                CheckDoco(iPaxForCkin->doco, checkDocInfo.second, now_local);
              iPaxForCkin->doco.toXML(paxNode);

              NewTextChild(paxNode,"subclass",iPaxForCkin->subclass);
              NewTextChild(paxNode,"transfer"); //пустой тег - трансфера нет
              NewTextChild(paxNode,"bag_pool_num");

              //ремарки
              RemQry.SQLText=CrsPaxRemQrySQL;
              RemQry.SetVariable("pax_id",iPaxForCkin->crs_pax_id);
              RemQry.Execute();
              CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);

              NewTextChild(paxNode,"norms"); //пустой тег - норм нет
            }
            catch(CheckIn::UserException)
            {
              throw;
            }
            catch(UserException &e)
            {
              throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForCkin->crs_pax_id);
            };
          };
        };

        bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( iPnrData->flt.point_dep );
        //пассажиры для изменения
        for(list<TWebPaxForChng>::const_iterator iPaxForChng=currPnr.paxForChng.begin();iPaxForChng!=currPnr.paxForChng.end();iPaxForChng++)
        {
          try
          {
            vector<TWebPaxFromReq>::const_iterator iPaxFromReq=currPnr.paxFromReq.begin();
            for(;iPaxFromReq!=currPnr.paxFromReq.end();iPaxFromReq++)
              if (iPaxFromReq->crs_pax_id==iPaxForChng->crs_pax_id) break;
            if (iPaxFromReq==currPnr.paxFromReq.end())
              throw EXCEPTIONS::Exception("VerifyPax: iPaxFromReq==currPnr.paxFromReq.end() (seg_no=%d, crs_pax_id=%d)", seg_no, iPaxForChng->crs_pax_id);

            int pax_tid=iPaxFromReq->pax_tid;
            //пассажир зарегистрирован
            if (!iPaxFromReq->refuse &&!iPaxFromReq->seat_no.empty() && iPaxForChng->seats > 0)
            {
            	string prior_xname, prior_yname;
            	string curr_xname, curr_yname;
            	// надо номализовать старое и новое место, сравнить их, если изменены, то вызвать пересадку
            	getXYName( iPnrData->flt.point_dep, iPaxForChng->seat_no, prior_xname, prior_yname );
            	getXYName( iPnrData->flt.point_dep, iPaxFromReq->seat_no, curr_xname, curr_yname );
            	if ( curr_xname.empty() && curr_yname.empty() )
            		throw UserException( "MSG.SEATS.SEAT_NO.NOT_FOUND" );
            	if ( prior_xname + prior_yname != curr_xname + curr_yname ) {
                if ( isTranzitSalonsVersion ) {
                  IntChangeSeatsN( iPnrData->flt.point_dep,
                                    iPaxForChng->crs_pax_id,
                                    pax_tid,
                                    curr_xname, curr_yname,
                                    SEATS2::stReseat,
      	                            cltUnknown,
                                    false, false,
                                    NULL );
                }
                else {
                  IntChangeSeats( iPnrData->flt.point_dep,
                                  iPaxForChng->crs_pax_id,
                                  pax_tid,
                                  curr_xname, curr_yname,
      	                          SEATS2::stReseat,
      	                          cltUnknown,
                                  false, false,
                                  NULL );
            	  }
              }
            };
            
            bool DocUpdatesPending=false;
            if (iPaxFromReq->doc_present) //тег <document> пришел
            {
              CheckDoc(iPaxForChng->doc, checkDocInfo.first, now_local);
              CheckIn::TPaxDocItem prior_doc;
              LoadPaxDoc(iPaxForChng->crs_pax_id, prior_doc, PaxDocQry);
              DocUpdatesPending=!(prior_doc==iPaxForChng->doc);
            };
            
            bool DocoUpdatesPending=false;
            if (iPaxFromReq->doco_present) //тег <doco> пришел
            {
              CheckDoco(iPaxForChng->doco, checkDocInfo.second, now_local);
              CheckIn::TPaxDocoItem prior_doco;
              LoadPaxDoco(iPaxForChng->crs_pax_id, prior_doco, PaxDocoQry);
              DocoUpdatesPending=!(prior_doco==iPaxForChng->doco);
            };

            bool FQTRemUpdatesPending=false;
            if (iPaxFromReq->fqt_rems_present) //тег <fqt_rems> пришел
            {
              vector<string> prior_fqt_rems;
              //читаем уже записанные ремарки FQTV
              RemQry.SQLText="SELECT rem FROM pax_rem WHERE pax_id=:pax_id AND rem_code='FQTV'";
              RemQry.SetVariable("pax_id", iPaxForChng->crs_pax_id);
              RemQry.Execute();
              for(;!RemQry.Eof;RemQry.Next()) prior_fqt_rems.push_back(RemQry.FieldAsString("rem"));
              //сортируем и сравниваем
              sort(prior_fqt_rems.begin(),prior_fqt_rems.end());
              FQTRemUpdatesPending=prior_fqt_rems!=iPaxFromReq->fqt_rems;
            };

            if (iPaxFromReq->refuse ||
                DocUpdatesPending ||
                DocoUpdatesPending ||
                FQTRemUpdatesPending)
            {
              //придется вызвать транзакцию на запись изменений
              XMLDoc &emulChngDoc=emulChngDocs[iPaxForChng->grp_id];
              if (emulChngDoc.docPtr()==NULL)
              {
                CopyEmulXMLDoc(emulDocHeader, emulChngDoc);

                xmlNodePtr emulChngNode=NodeAsNode("/term/query",emulChngDoc.docPtr());
                emulChngNode=NewTextChild(emulChngNode,"TCkinSavePax");

                xmlNodePtr segNode=NewTextChild(NewTextChild(emulChngNode,"segments"),"segment");
                NewTextChild(segNode,"point_dep",iPaxForChng->point_dep);
                NewTextChild(segNode,"point_arv",iPaxForChng->point_arv);
                NewTextChild(segNode,"airp_dep",iPaxForChng->airp_dep);
                NewTextChild(segNode,"airp_arv",iPaxForChng->airp_arv);
                NewTextChild(segNode,"class",iPaxForChng->cl);
                NewTextChild(segNode,"grp_id",iPaxForChng->grp_id);
                NewTextChild(segNode,"tid",iPaxFromReq->pax_grp_tid);
                NewTextChild(segNode,"passengers");

                NewTextChild(emulChngNode,"excess",iPaxForChng->excess);
                NewTextChild(emulChngNode,"hall");
                if (iPaxForChng->bag_refuse)
                  NewTextChild(emulChngNode,"bag_refuse",refuseAgentError);
                else
                  NewTextChild(emulChngNode,"bag_refuse");
              };
              xmlNodePtr paxsNode=NodeAsNode("/term/query/TCkinSavePax/segments/segment/passengers",emulChngDoc.docPtr());

              xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
              NewTextChild(paxNode,"pax_id",iPaxForChng->crs_pax_id);
              NewTextChild(paxNode,"surname",iPaxForChng->surname);
              NewTextChild(paxNode,"name",iPaxForChng->name);
              if (iPaxFromReq->refuse ||
                  DocUpdatesPending ||
                  DocoUpdatesPending)
              {
                //были ли изменения по пассажиру CheckInInterface::SavePax определяет по наличию тега refuse
                NewTextChild(paxNode,"refuse",iPaxFromReq->refuse?refuseAgentError:"");
                NewTextChild(paxNode,"pers_type",iPaxForChng->pers_type);
              };
              NewTextChild(paxNode,"tid",pax_tid);

              if (DocUpdatesPending)
                iPaxForChng->doc.toXML(paxNode);

              if (DocoUpdatesPending)
                iPaxForChng->doco.toXML(paxNode);

              if (FQTRemUpdatesPending)
              {
                //ремарки
                RemQry.SQLText=PaxRemQrySQL;
                RemQry.SetVariable("pax_id",iPaxForChng->crs_pax_id);
                RemQry.Execute();
                CreateEmulRems(paxNode, RemQry, iPaxFromReq->fqt_rems);
              };
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            throw CheckIn::UserException(e.getLexemaData(), s->first, iPaxForChng->crs_pax_id);
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
  };
  
  //возвращаем ids
  for(iPnrData=PNRs.begin();iPnrData!=PNRs.end();iPnrData++)
  {
    TIdsPnrData idsPnrData;
    idsPnrData.point_id=iPnrData->flt.point_dep;
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
        pax.doc_present=(docNode!=NULL);
        if (docNode!=NULL) PaxDocFromXML(docNode, pax.doc);

        xmlNodePtr docoNode = GetNode("doco", paxNode);
        pax.doco_present=(docoNode!=NULL);
        if (docoNode!=NULL) PaxDocoFromXML(docoNode, pax.doco);
        
        xmlNodePtr fqtNode = GetNode("fqt_rems", paxNode);
        pax.fqt_rems_present=(fqtNode!=NULL); //если тег <fqt_rems> пришел, то изменяем и перезаписываем ремарки FQTV
        if (fqtNode!=NULL)
        {
          //читаем пришедшие ремарки
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
  TChangeStatusList ETInfo;
  set<int> tckin_ids;
  bool result=true;
  //важно, что сначала вызывается CheckInInterface::SavePax для emulCkinDoc
  //только при веб-регистрации НОВОЙ группы возможен ROLLBACK CHECKIN в SavePax при перегрузке
  //и соответственно возвращение result=false
  //и соответственно вызов ETStatusInterface::ETRollbackStatus для ВСЕХ ЭБ
  
  if (emulCkinDoc.docPtr()!=NULL) //регистрация новой группы
  {
    xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
    if (emulReqNode==NULL)
      throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: emulReqNode=NULL");
    if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ETInfo, tckin_id))
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
      if (CheckInInterface::SavePax(emulReqNode, ediResNode, first_grp_id, ETInfo, tckin_id))
      {
        if (tckin_id!=NoExists) tckin_ids.insert(tckin_id);
      }
      else
      {
        //по идее сюда мы никогда не должны попадать (см. комментарий выше)
        //для этого никогда не возвращаем false и делаем специальную защиту в SavePax:
        //при записи изменений веб и киосков не откатываемся при перегрузке
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: CheckInInterface::SavePax=false");
        //result=false;
        //break;
      };

    };
  };

  if (result)
  {
    if (ediResNode==NULL && !ETInfo.empty())
    {
      //хотя бы один билет будет обрабатываться
      OraSession.Rollback();  //откат

      int req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(reqNode->doc));
      if (!ETStatusInterface::ETChangeStatus(req_ctxt,ETInfo))
        throw EXCEPTIONS::Exception("WebRequestsIface::SavePax: Wrong ETInfo");
      AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
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
  	 "WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id";
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
      //не надо прокидывать ue в терминал - подтверждаем все что можем!
      ue.addError(e.getLexemaData(), pax.point_dep, pax.pax_id);
    };
  };

  if (!is_test)
  {
    PrintInterface::ConfirmPrintBP(paxs, ue);  //не надо прокидывать ue в терминал - подтверждаем все что можем!
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

          //проверим на дублирование
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
          "  vpoint_id    tlg_comp_layers.point_id%TYPE; "
          "  vairp_arv    tlg_comp_layers.airp_arv%TYPE; "
          "  vfirst_xname tlg_comp_layers.first_xname%TYPE; "
          "  vfirst_yname tlg_comp_layers.first_yname%TYPE; "
          "  vlast_xname  tlg_comp_layers.last_xname%TYPE; "
          "  vlast_yname  tlg_comp_layers.last_yname%TYPE; "
          "  vrem_code    tlg_comp_layers.rem_code%TYPE; "
          "BEGIN "
          "  :delete_seat_ranges:=1; "
          "  BEGIN "
          "    SELECT range_id, point_id, airp_arv, "
          "           first_xname, first_yname, last_xname, last_yname, rem_code "
          "    INTO vrange_id, vpoint_id, vairp_arv, "
          "         vfirst_xname, vfirst_yname, vlast_xname, vlast_yname, vrem_code "
          "    FROM tlg_comp_layers "
          "    WHERE crs_pax_id=:crs_pax_id AND layer_type=:layer_type FOR UPDATE; "
          "    IF :point_id=vpoint_id AND :airp_arv=vairp_arv AND "
          "       :first_xname=vfirst_xname AND :first_yname=vfirst_yname AND "
          "       :last_xname=vlast_xname AND :last_yname=vlast_yname THEN "
          "      :delete_seat_ranges:=0; "
          "      UPDATE tlg_comp_layers "
          "      SET time_remove=SYSTEM.UTCSYSDATE+:timeout/1440 "
          "      WHERE range_id=vrange_id; "
          "    END IF; "
          "  EXCEPTION "
          "    WHEN NO_DATA_FOUND THEN NULL; "
          "    WHEN TOO_MANY_ROWS THEN NULL; "
          "  END; "
          "END; ";
        LayerQry.DeclareVariable("delete_seat_ranges", otInteger);
        LayerQry.DeclareVariable("point_id", otInteger);
        LayerQry.DeclareVariable("airp_arv", otString);
        LayerQry.DeclareVariable("layer_type", otString);
        LayerQry.DeclareVariable("first_xname", otString);
        LayerQry.DeclareVariable("last_xname", otString);
        LayerQry.DeclareVariable("first_yname", otString);
        LayerQry.DeclareVariable("last_yname", otString);
        LayerQry.DeclareVariable("crs_pax_id", otInteger);
        if (time_limit!=NoExists)
          LayerQry.CreateVariable("timeout", otInteger, time_limit);
        else
          LayerQry.CreateVariable("timeout", otInteger, FNull);

        VerifyPNR(point_id, pnr_id);
        GetCrsPaxSeats(point_id, pnr, pax_seats );
        bool UsePriorContext=false;
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
              
/*            if ( iSeat->first.WebTariff.value == 0.0 )  //нет тарифа
              throw UserException("MSG.SEATS.NOT_SET_RATE");*/
            
            if (isTestPaxId(iPax->crs_pax_id)) continue;

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
            };
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
      };
      check_layer_change(point_ids_spp);
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
            if ( iSeat->first.WebTariff.value != 0.0 ) { // если платная регистрация отключена, value=0.0 в любом случае
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

////////////////////////////////////MERIDIAN SYSTEM/////////////////////////////
inline void CreateXMLStage( const TCkinClients &CkinClients, TStage stage_id, const TTripStage &stage,
                            xmlNodePtr node, const string &region )
{
  TStagesRules *sr = TStagesRules::Instance();
  if ( sr->isClientStage( (int)stage_id ) && !sr->canClientStage( CkinClients, (int)stage_id ) )
    return;
  xmlNodePtr node1 = NewTextChild( node, "stage" );
  SetProp( node1, "stage_id", stage_id );
  NewTextChild( node1, "scd", DateTimeToStr( UTCToClient( stage.scd, region ), "dd.mm.yyyy hh:nn" ) );
  if ( stage.est != ASTRA::NoExists )
    NewTextChild( node1, "est", DateTimeToStr( UTCToClient( stage.est, region ), "dd.mm.yyyy hh:nn" ) );
  if ( stage.act != ASTRA::NoExists )
    NewTextChild( node1, "act", DateTimeToStr( UTCToClient( stage.act, region ), "dd.mm.yyyy hh:nn" ) );
}

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
    {"ЮТ", "ЮР", "QU" };

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
  map<int,Tids> Paxs; // pax_id, <pax_tid, grp_tid> список пассажиров переданных ранее
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
      if ( vpriordate == vdate ) { // разбор дерева при условии, что предыдущий запрос не передал всех пассажиров за заданный момент времени
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
    "WHERE time >= :time AND time > system.UTCSYSDATE - 1"
    "ORDER BY time, pax_id, work_mode ";
  Qry.CreateVariable( "time", otDate, vdate );
  Qry.Execute();
  TQuery PaxQry(&OraSession);
  PaxQry.SQLText =
	   "SELECT pax.pax_id,pax.reg_no,pax.surname||RTRIM(' '||pax.name) name,"
     "       pax_grp.grp_id,"
	   "       pax_grp.airp_arv,pax_grp.airp_dep,"
     "       pax_grp.class,pax.refuse,"
	   "       pax.pers_type, "
	   "       pax.subclass, "
	   "       NVL(pax_doc.gender,'F') as gender, "
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
  // пробег по всем пассажирам у которых время больше или равно текущему
  for ( ;!Qry.Eof && pax_count<=100; Qry.Next() ) {
    int pax_id = Qry.FieldAsInteger( "pax_id" );
    if ( pax_id == prior_pax_id ) // удаляем дублирование строки с одним и тем же pax_id для регистрации и посадки
      continue; // предыдущий пассажир он же и текущий
    ProgTrace( TRACE5, "pax_id=%d", pax_id );
    prior_pax_id = pax_id;
    FltQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
    FltQry.Execute();
    if ( FltQry.Eof )
      throw EXCEPTIONS::Exception("WebRequestsIface::GetPaxsInfo: flight not found, (point_id=%d)", Qry.FieldAsInteger( "point_id" ) );
    TTripInfo tripInfo( FltQry );
    if ( TReqInfo::Instance()->client_type != ctWeb ||
         !is_sync_meridian( tripInfo ) ) {
      continue;
    }

    if ( max_time != NoExists && max_time != Qry.FieldAsDateTime( "time" ) ) { // сравнение времени с пред. значением, если изменилось, то
      ProgTrace( TRACE5, "Paxs.clear(), vdate=%s, max_time=%s",
                 DateTimeToStr( vdate, ServerFormatDateTimeAsString ).c_str(),
                 DateTimeToStr( max_time, ServerFormatDateTimeAsString ).c_str() );
      Paxs.clear(); // изменилось время - удаляем всех предыдущих пассажиров с пред. временем
    }
    max_time = Qry.FieldAsDateTime( "time" );
    PaxQry.SetVariable( "pax_id", pax_id );
    PaxQry.Execute();
    if ( !PaxQry.Eof ) {
      tids.pax_tid = PaxQry.FieldAsInteger( "pax_tid" );
      tids.grp_tid = PaxQry.FieldAsInteger( "grp_tid" );
    }
    else {
      tids.pax_tid = -1; // пассажир был удален в пред. раз
      tids.grp_tid = -1;
    }
    // пассажир передавался и не изменился
    if ( Paxs.find( pax_id ) != Paxs.end() &&
         Paxs[ pax_id ].pax_tid == tids.pax_tid &&
         Paxs[ pax_id ].grp_tid == tids.grp_tid )
      continue; // уже передавали пассажира
    // пассажира не передавали или он изменился
    if ( node == NULL )
      node = NewTextChild( resNode, "passengers" );
    Paxs[ pax_id ] = tids; // изменения
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
      NewTextChild( paxNode, "gender", PaxQry.FieldAsString( "gender" ) );
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
    if ( ckinRoute.GetRouteAfter( PaxQry.FieldAsInteger( "grp_id" ), crtNotCurrent, crtIgnoreDependent ) ) { // есть сквозная регистрация
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
  }
  if ( max_time == NoExists )
    max_time = vdate; // не выбрано ни одного пассажира - передаем время, которые пришло с терминала
  SetProp( resNode, "time", DateTimeToStr( max_time, ServerFormatDateTimeAsString ) );
  AstraContext::ClearContext( "meridian_sync", 0 );
  if ( !Paxs.empty() ) { // есть пассажиры - сохраняем всех переданных
    paxsDoc = CreateXMLDoc( "UTF-8", "paxs" );
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

