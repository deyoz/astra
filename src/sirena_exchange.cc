#include "sirena_exchange.h"
#include "astra_context.h"
#include <boost/asio.hpp>
#include <serverlib/xml_stuff.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

namespace SirenaExchange
{

const char* SIRENA_HOST()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("SIRENA_HOST",NULL);
  return VAR.c_str();
}

int SIRENA_PORT()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_PORT", ASTRA::NoExists, ASTRA::NoExists, ASTRA::NoExists);
  return VAR;
};

int SIRENA_REQ_TIMEOUT()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_REQ_TIMEOUT", 1000, 30000, 10000);
  return VAR;
};

int SIRENA_REQ_ATTEMPTS()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_REQ_ATTEMPTS", 1, 10, 1);
  return VAR;
};

void TExchange::build(std::string &content) const
{
  try
  {
    XMLDoc doc(isRequest()?"query":"answer");
    xmlNodePtr node=NewTextChild(NodeAsNode(isRequest()?"/query":"/answer", doc.docPtr()), exchangeId().c_str());
    if (error())
      errorToXML(node);
    else
      toXML(node);
    content = ConvertCodepage( XMLTreeToText( doc.docPtr() ), "CP866", "UTF-8" );
  }
  catch(Exception &e)
  {
    throw Exception("TExchange::build: %s", e.what());
  };
}

void TExchange::parse(const std::string &content)
{
  try
  {
    XMLDoc doc(content);
    if (doc.docPtr()==NULL)
    {
      if (content.size()>500)
        throw Exception("Wrong XML %s...", content.substr(0,500).c_str());
      else
        throw Exception("Wrong XML %s", content.c_str());
    };
    xml_decode_nodelist(doc.docPtr()->children);
    xmlNodePtr node=NodeAsNode(isRequest()?"/query":"/answer", doc.docPtr());
    node=NodeAsNode(exchangeId().c_str(), node);
    errorFromXML(node);
    if (!error())
      fromXML(node);
  }
  catch(Exception &e)
  {
    throw Exception("TExchange::parse: %s", e.what());
  };
}

void TExchange::toXML(xmlNodePtr node) const
{
  throw Exception("TExchange::toXML: %s <%s> not implemented", isRequest()?"Request":"Response", exchangeId().c_str());
}

void TExchange::fromXML(xmlNodePtr node)
{
  throw Exception("TExchange::fromXML: %s <%s> not implemented", isRequest()?"Request":"Response", exchangeId().c_str());
}

void TErrorReference::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  xmlNodePtr refNode=NewTextChild(node, "reference");
  SetProp(refNode, "path", path, "");
  SetProp(refNode, "value", value, "");
  SetProp(refNode, "passenger_id", pax_id, ASTRA::NoExists);
  SetProp(refNode, "segment_id", seg_id, ASTRA::NoExists);
}

void TErrorReference::fromXML(xmlNodePtr node)
{
  clear();

  xmlNodePtr refNode=GetNode("reference", node);

  path=NodeAsString("@path", refNode, "");
  value=NodeAsString("@value", refNode, "");
  pax_id=NodeAsInteger("@passenger_id", refNode, ASTRA::NoExists);
  seg_id=NodeAsInteger("@segment_id", refNode, ASTRA::NoExists);
}

std::string TErrorReference::traceStr() const
{
  ostringstream s;
  if (!path.empty())
    s << " path='" << path << "'";
  if (!value.empty())
    s << " value='" << value << "'";
  if (pax_id!=ASTRA::NoExists)
    s << " pax_id=" << pax_id;
  if (seg_id!=ASTRA::NoExists)
    s << " seg_id=" << seg_id;
  return s.str();
}

void TExchange::errorToXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  xmlNodePtr errNode=NewTextChild(node, "error");
  SetProp(errNode, "code", error_code);
  SetProp(errNode, "message", error_message);
  if (!error_reference.empty())
    error_reference.toXML(node);
}

void TExchange::errorFromXML(xmlNodePtr node)
{
  error_code.clear();
  error_message.clear();
  error_reference.clear();

  xmlNodePtr errNode=GetNode("error", node);
  if (errNode==NULL) return;

  error_code=NodeAsString("@code", errNode, "");
  error_message=NodeAsString("@message", errNode, "");
  error_reference.fromXML(errNode);
}

bool TExchange::error() const
{
  return !error_code.empty() ||
         !error_message.empty() ||
         !error_reference.empty();
}

std::string TExchange::traceError() const
{
  ostringstream s;
  if (!error_code.empty())
    s << " code=" << error_code;
  if (!error_message.empty())
    s << " message='" << error_message << "'";
  if (!error_reference.empty())
    s << error_reference.traceStr();
  return s.str();
}

void traceXML(const string& xml)
{
  size_t len=xml.size();
  int portion=4000;
  for(size_t pos=0; pos<len; pos+=portion)
    ProgTrace(TRACE5, "%s", xml.substr(pos,portion).c_str());
}

void SendRequest(const TExchange &request, TExchange &response,
                 RequestInfo &requestInfo, ResponseInfo &responseInfo)
{
  time_t start_time=time(NULL);
  requestInfo.host = SIRENA_HOST();
  requestInfo.port = SIRENA_PORT();
  requestInfo.path = "/astra";
  request.build(requestInfo.content);
  requestInfo.using_ssl = false;
  requestInfo.timeout = SIRENA_REQ_TIMEOUT();
  int request_count = SIRENA_REQ_ATTEMPTS();
  traceXML(requestInfo.content);
  for(int pass=0; pass<request_count; pass++)
  {
    httpClient_main(requestInfo, responseInfo);
    if (!(!responseInfo.completed && responseInfo.error_code==boost::asio::error::eof && responseInfo.error_operation==toStatus))
      break;
    ProgTrace( TRACE5, "SIRENA DEADED, next request!" );
  };
  if (!responseInfo.completed ) throw Exception("%s: responseInfo.completed()=false", __FUNCTION__);

  xmlDocPtr doc=TextToXMLTree(responseInfo.content);
  if (doc!=NULL)
    traceXML(XMLTreeToText(doc));
  else
    traceXML(responseInfo.content);
  response.parse(responseInfo.content);
  if (response.error()) throw Exception("SIRENA ERROR: %s", response.traceError().c_str());

  ProgTrace(TRACE5, "%s: processing time %ld secs", __FUNCTION__, time(NULL)-start_time);
}

void SendRequest(const TExchange &request, TExchange &response)
{
  RequestInfo requestInfo;
  ResponseInfo responseInfo;
  SendRequest(request, response, requestInfo, responseInfo);
}

void TLastExchangeInfo::toDB()
{
  if (grp_id==ASTRA::NoExists) return;
  AstraContext::ClearContext("pc_payment_req", grp_id);
  AstraContext::SetContext("pc_payment_req", grp_id, ConvertCodepage(pc_payment_req, "UTF-8", "CP866"));
  pc_payment_req_created=NowUTC();
  AstraContext::ClearContext("pc_payment_res", grp_id);
  AstraContext::SetContext("pc_payment_res", grp_id, ConvertCodepage(pc_payment_res, "UTF-8", "CP866"));
  pc_payment_res_created=NowUTC();
}

void TLastExchangeInfo::fromDB(int grp_id)
{
  clear();
  if (grp_id==ASTRA::NoExists) return;
  pc_payment_req_created=AstraContext::GetContext("pc_payment_req", grp_id, pc_payment_req);
  pc_payment_res_created=AstraContext::GetContext("pc_payment_res", grp_id, pc_payment_res);
  pc_payment_req=ConvertCodepage(pc_payment_req, "CP866", "UTF-8");
  pc_payment_res=ConvertCodepage(pc_payment_res, "CP866", "UTF-8");
}

void TLastExchangeInfo::cleanOldRecords()
{
  TDateTime d=NowUTC()-15/1440.0;
  AstraContext::ClearContext("pc_payment_req", d);
  AstraContext::ClearContext("pc_payment_res", d);
}

void TLastExchangeList::handle(const string& where)
{
  for(TLastExchangeList::iterator i=begin(); i!=end(); ++i)
    i->toDB();
}

void SendTestRequest(const string &req)
{
  RequestInfo request;
  std::string proto;
  std::string query;
  request.host = SIRENA_HOST();
  request.port = SIRENA_PORT();
  request.timeout = SIRENA_REQ_TIMEOUT();
  request.headers.insert(make_pair("CLIENT-ID", "SIRENA"));
  request.headers.insert(make_pair("OPERATION", "piece_concept"));

  request.content = req;
  ProgTrace( TRACE5, "request.content=%s", request.content.c_str());
  request.using_ssl = false;
  ResponseInfo response;
  for(int pass=0; pass<SIRENA_REQ_ATTEMPTS(); pass++)
  {
    httpClient_main(request, response);
    if (!(!response.completed && response.error_code==boost::asio::error::eof && response.error_operation==toStatus))
      break;
    ProgTrace( TRACE5, "SIRENA DEADED, next request!" );
  };
  if (!response.completed ) throw Exception("%s: responseInfo.completed()=false", __FUNCTION__);

  ProgTrace( TRACE5, "response=%s", response.toString().c_str());
}

string airlineToXML(const std::string &code, const std::string &lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang);
  if (result.size()==3) //⨯� ����
    result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang==LANG_EN?LANG_RU:LANG_EN);
  return result;
}

string airpToXML(const std::string &code, const std::string &lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang);
  if (result.size()==4) //⨯� ����
    result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang==LANG_EN?LANG_RU:LANG_EN);
  return result;
}

} //namespace SirenaExchange

