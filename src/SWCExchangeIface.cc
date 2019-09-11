#include "SWCExchangeIface.h"
#include "xml_unit.h"
#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

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
      void clear() {
        addr.clear();
        port = 0;
        timeout = 0;
        useSSL = false;
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
    virtual std::string makeHttpPostRequest(const std::string& resource,
                                            const std::string& host,
                                            const std::string& authorization,
                                            const std::string& postbody) const {
      return std::string("POST /swc-xml/site HTTP/1.1\r\n") + //+//not use POST http:// + // + " HTTP/1.1\r\n"
             "Host: ws.sirena-travel.ru\r\n" +
             "Content-Type: application/xml; charset=utf-8\r\n" +
             "Authorization:Basic eG1sX2FzdHJhX0dSVF9VVDpXMFJ5M0VEQng0\r\n" +
             "Content-Length: " + HelpCpp::string_cast(postbody.size()) + "\r\n" +
             "\r\n" +
             postbody;
    }
    SWCClient( const ConnectProps& props ):ExchangeIterface::HTTPClient(props.addr, props.port, props.timeout, props.useSSL){}
    ~SWCClient(){}
};


void SWCExchange::fromDB()
{
  clientId = 105;
  Authorization = "Authorization:Basic eG1sX2FzdHJhX0dSVF9VVDpXMFJ5M0VEQng0";
}

void SWCExchange::build(std::string &content) const
{
  content.clear();
  static int counter=0;

counter++;
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
    SetProp(node, "counter",counter);
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
  //LogTrace(TRACE5) << __func__ << std::endl << content;
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
  tst();
  error_code.clear();
  error_message.clear();
  error_reference.clear();

  xmlNodePtr errNode=GetNode("error", node);
  if (errNode==NULL) return;

  error_code=NodeAsString("@code", errNode, "");
  error_message=NodeAsString(errNode);
  LogError(STDLOG) << "SWC exchange error: code=" << error_code << ",message=" << error_message;
}

void SWCExchange::errorToXML(xmlNodePtr node) const
{
  if (node==NULL) return;
  xmlNodePtr n = NewTextChild(node,"command");
  NewTextChild(n, "error", error_message + ",code=" + error_code );
}

void SWCExchangeIface::Request(xmlNodePtr reqNode, const std::string& ifaceName, const SWCExchange& req)
{
  tst();
  SWCClient::ConnectProps props;
  props.fromDB();
  tst();
  SWCClient client( props );
  ExchangeIface* iface=dynamic_cast<ExchangeIface*>(JxtInterfaceMng::Instance()->GetInterface(ifaceName));
  tst();
  iface->DoRequest(reqNode, nullptr, req, client);
}

/*bool SWCExchangeIface::isResponseHandler( const std::string& name, xmlNodePtr node )
{
  return ( node != nullptr &&
           std::string("answer") == (const char*)node->name &&
           node->children != nullptr &&
           name == (const char*)node->children->name );
}*/

}
