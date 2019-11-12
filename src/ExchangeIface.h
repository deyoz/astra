#ifndef EXCHANGEIFACE_H
#define EXCHANGEIFACE_H

#include <string>
#include "astra_consts.h"
#include "xml_unit.h"
#include "httpClient.h"
#include "sirena_exchange.h"
#include "jxtlib/JxtInterface.h"

#include <serverlib/httpsrv.h>

namespace ExchangeIterface
{


class HTTPClient
{
protected:
    httpsrv::HostAndPort m_addr;
    int                  m_timeout;
    httpsrv::UseSSLFlag  m_useSsl;
    std::string resource;
    std::string authorization;
public:
  HTTPClient( const std::string&_host,
              unsigned _port,
              int _req_timeout,
              bool _useSSL,
              const std::string& _resource,
              const std::string& _authorization ):m_addr(_host,_port),m_timeout(_req_timeout/1000),m_useSsl(_useSSL),
                                                  resource(_resource),authorization(_authorization) {}
  virtual std::string makeHttpPostRequest(const std::string& postbody) const  = 0;
  void sendRequest(const std::string& reqText, const edifact::KickInfo& kickInfo, const std::string &domainName) const;
  static boost::optional<httpsrv::HttpResp> receive(const std::string& pult, const std::string &domainName);
  virtual ~HTTPClient(){}
};


typedef std::function<void(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)> ExchangeResponseHandler;

class ExchangeIface : public JxtInterface
{
  private:
    std::map<std::string,ExchangeResponseHandler> resHandlers;
    void handleResponse(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode) const;
    static bool equal(const ExchangeResponseHandler& handler1,
                      const ExchangeResponseHandler& handler2);
  protected:
    std::string domainName;
    bool addResponseHandler( const std::string& funcName, const ExchangeResponseHandler&);
    virtual void BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req) = 0;
    virtual void BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId ) = 0;
  public:
    void DoRequest(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req, const HTTPClient &client);
    ExchangeIface(const std::string &_iface) : JxtInterface("", _iface)
    {
      domainName = "ASTRA";
      AddEvent("kick", JXT_HANDLER(ExchangeIface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual ~ExchangeIface() {}
};

}


#endif // EXCHANGEIFACE_H
