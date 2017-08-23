#include "sirena_exchange.h"
#include "astra_context.h"
#include "edi_utils.h"
#include "xp_testing.h"

#include <libtlg/tlg_outbox.h>
#include <serverlib/testmode.h>
#include <serverlib/xml_stuff.h>

#include <boost/asio.hpp>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

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
    parse(node);
  }
  catch(Exception &e)
  {
    throw Exception("TExchange::parse: %s", e.what());
  };
}

void TExchange::parse(xmlNodePtr node)
{
    try
    {
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

void TExchange::setSrcFile(const std::string &_filename)
{
  src_filename=_filename;
}

void TExchange::setDestFile(const std::string &_filename)
{
  dest_filename=_filename;
}

bool TExchange::loadFromFile(std::string &content) const
{
  if (src_filename.empty()) return false;
  std::ifstream f(src_filename.c_str());
  if(!f.is_open()) return false;
  content.clear();
  std::getline(f, content, '\0');
  f.close();
  return !content.empty();
}

bool TExchange::saveToFile(const std::string &content) const
{
  if (dest_filename.empty()) return false;
  std::ofstream f(dest_filename.c_str());
  if(!f.is_open()) return false;
  f << content;
  f.close();
  return true;
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

  bool load_res=response.loadFromFile(responseInfo.content);
  if (!load_res)
  {
    requestInfo.host = SIRENA_HOST();
    requestInfo.port = SIRENA_PORT();
    requestInfo.path = "/astra";
    if (!request.loadFromFile(requestInfo.content))
    {
      request.build(requestInfo.content);
      request.saveToFile(requestInfo.content);
    };
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
  };

  xmlDocPtr doc=TextToXMLTree(responseInfo.content);
  string s=doc!=NULL?XMLTreeToText(doc):responseInfo.content;
  if (!load_res) response.saveToFile(s);
  traceXML(s);

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

  ProgTrace( TRACE5, "response: %s", response.toString().c_str());
  ProgTrace( TRACE5, "response.content=%s", response.content.c_str());
}

//---------------------------------------------------------------------------------------

static std::string makeHttpPostRequest(const std::string& resource,
                                       const std::string& host,
                                       const std::string& postbody)
{
    return "POST " + resource + " HTTP/1.1\r\n"
             "Host: " + host + "\r\n"
             "Content-Type: application/xml; charset=utf-8\r\n"
             "Content-Length: " + HelpCpp::string_cast(postbody.size()) + "\r\n"
             "\r\n" +
             postbody;
}

SirenaClient::SirenaClient()
    : m_addr(SIRENA_HOST(), SIRENA_PORT()),
      m_timeout(SIRENA_REQ_TIMEOUT()),
      m_useSsl(false)
{}

void SirenaClient::sendRequest(const std::string& reqText, const edifact::KickInfo& kickInfo)
{
    std::string desk = kickInfo.desk.empty() ? "SYSPUL" : kickInfo.desk;

    const std::string path = "/astra";
    const std::string httpPost = makeHttpPostRequest(path, m_addr.host, reqText);

    LogTrace(TRACE5) << "HTTP Request from [" << desk << "], text:\n" << reqText;

    const httpsrv::Pult pul(desk);
    const httpsrv::Domain domain("ASTRA");
    const std::string kick(AstraEdifact::make_xml_kick(kickInfo));

    httpsrv::DoHttpRequest req(pul, domain, m_addr, httpPost);
    req.setTimeout(boost::posix_time::seconds(m_timeout))
        .setMaxTryCount(1/*SIRENA_REQ_ATTEMPTS()*/)
        .setSSL(m_useSsl)
        .setPeresprosReq(kick)
        .setDeffered(true);

    const std::string httpPostCP866 = UTF8toCP866(httpPost);
    LogTrace(TRACE1) << "request: " << httpPostCP866;

#ifdef XP_TESTING
    if (inTestMode()) {
        xp_testing::TlgOutbox::getInstance().push(tlgnum_t("httpreq"),
                        StrUtils::replaceSubstrCopy(httpPostCP866, "\r", ""), 0 /* h2h */);
    }
#endif // XP_TESTING

    req();
}

boost::optional<httpsrv::HttpResp> SirenaClient::receive(const std::string& pult)
{
    const httpsrv::Pult pul(pult);
    const httpsrv::Domain domain("ASTRA");

    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(pul, domain);

    for (const httpsrv::HttpResp& httpResp: responses)
    {
        LogTrace(TRACE1) << "httpResp text: '" << httpResp.text << "'";
    }

    const size_t responseCount = responses.size();
    if (responseCount == 0)
    {
        LogTrace(TRACE1) << "FetchHttpResponses: hasn't got any responses yet";
        return {};
    }
    if (responseCount > 1)
    {
        LogError(STDLOG) << "FetchHttpResponses: " << responseCount << " responses!";
    }

    return responses.front();
}

} //namespace SirenaExchange
