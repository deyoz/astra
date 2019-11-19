#include "SWCExchangeIface.h"
#include "xml_unit.h"

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>


namespace SWC
{

class SWCClient : public ExchangeIterface::HTTPClient
{
  private:
  public:
    struct ConnectProps {
      std::string addr;
      unsigned port;
      int timeout;
      bool useSSL;
      std::string Resource;
      std::string Authorization;
      void clear() {
        addr.clear();
        port = 0;
        timeout = 0;
        useSSL = false;
        Authorization.clear();
        Resource.clear();
      }
      ConnectProps() {
        clear();
      }
      void fromDB( ) {
        clear();
        addr = "ws.sirena-travel.ru";
        port = 80;
        timeout = 15000;
      }
    };
    virtual std::string makeHttpPostRequest( const std::string& postbody) const {
      std::stringstream os; //+//not use POST http:// + // + " HTTP/1.1\r\n"
      os << "POST " << resource << " HTTP/1.1\r\n";
      os << "Host: " << m_addr.host << "\r\n";
      os << "Content-Type: application/xml; charset=utf-8\r\n";
      os << authorization << "\r\n";
      os <<  "Content-Length: " << HelpCpp::string_cast(postbody.size()) << "\r\n";
      os << "\r\n";
      os << postbody;
      return os.str();
    }
    SWCClient( const ConnectProps& props ):ExchangeIterface::HTTPClient(props.addr, props.port, props.timeout, props.useSSL, props.Resource, props.Authorization){}
    ~SWCClient(){}
};

void SWCExchange::fromDB(int clientId)
{
  this->clientId = clientId;
  Authorization = "Authorization:Basic " + StrUtils::b64_encode( getTCLParam("SWC_CONNECT","") );
  Resource = "/swc-xml/site";
  LogTrace(TRACE5) << __func__ << " " << Authorization << ",clientId=" << clientId << ", Resource=" <<Resource;
}

void SWCExchange::build(std::string &content) const
{
  content.clear();
  try
  {
    XMLDoc doc;
    doc.set("soapenv:Envelope");
    if (doc.docPtr()==NULL)
      throw EXCEPTIONS::Exception("toSoapReq: CreateXMLDoc failed");
    xmlNodePtr soapNode=xmlDocGetRootElement(doc.docPtr());
    SetProp(soapNode, "xmlns:soapenv", "http://schemas.xmlsoap.org/soap/envelope/");
    xmlNodePtr headerNode = NewTextChild(soapNode, "soapenv:Header");
    SetProp(headerNode, "xmlns:wsa", "http://www.w3.org/2005/08/addressing");
    xmlNodePtr bodyNode = NewTextChild(soapNode, "soapenv:Body");
    xmlNodePtr node = NewTextChild( bodyNode, "ser:sendSirenaXmlRequest" );
    SetProp(node, "xmlns:ser", "http://service.swc.comtech/");
    NewTextChild(node, "sirenaClientId", clientId);
    node = NewTextChild(node,"request");
    node = NewTextChild(node,"sirena");
    node = NewTextChild(node,isRequest()?"query":"answer");
    if (error())
      errorToXML(node);
    else
      toXML(node);
    content = ConvertCodepage( XMLTreeToText( doc.docPtr() ), "CP866", "UTF-8" );
  }
  catch(EXCEPTIONS::Exception &e)
  {
    throw EXCEPTIONS::Exception("SWCExchange::build: %s", e.what());
  };
}

bool SWCExchange::isEmptyAnswer(xmlNodePtr node)
{
  return !(node != nullptr &&
          std::string("answer") == (const char*)node->name &&
          node->children != nullptr &&
          exchangeId() == (const char*)node->children->name);
}

void SWCExchange::parseResponse(xmlNodePtr node)
{
  SirenaExchange::TExchange::parseResponse(node);
}

void SWCExchange::errorFromXML(xmlNodePtr node)
{
  error_code.clear();
  error_message.clear();
  error_reference.clear();

  xmlNodePtr errNode=GetNode("error", node);
  if (errNode==NULL) return;

  error_code=NodeAsString("@code", errNode, "");
  error_message=NodeAsString(errNode);
  LogError(STDLOG) << error_code << "=" << error_message;
}

void SWCExchange::errorToXML(xmlNodePtr node) const
{
  if (node==NULL) return;
  xmlNodePtr n = NewTextChild(node,"command");
  NewTextChild(n, "error", error_message + ",code=" + error_code );
}

void SWCExchangeIface::Request(xmlNodePtr reqNode, int clientId, const std::string& ifaceName, SWCExchange& req)
{
  req.fromDB(clientId);
  SWCClient::ConnectProps props;
  props.fromDB();
  props.Authorization = req.getAuthorization();
  props.Resource = req.getResource();
  SWCClient client( props );
  ExchangeIface* iface=dynamic_cast<ExchangeIface*>(JxtInterfaceMng::Instance()->GetInterface(ifaceName));
  iface->DoRequest(reqNode, nullptr, req, client);
}


}
