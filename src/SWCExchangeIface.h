#ifndef SWCEXCHANGEIFACE_H
#define SWCEXCHANGEIFACE_H
#include "ExchangeIface.h"

namespace SWC
{

class SWCLogger
{
  private:
    bool isLogging;
  public:
    void toLog( xmlDocPtr doc );
};

class SWCExchange: public SirenaExchange::TExchange
{
  private:
    int clientId;
    std::string Authorization;
    std::string Resource;
  public:
    std::string getAuthorization() {
      return Authorization;
    }
    std::string getResource() {
      return Resource;
    }
    void fromDB();
    virtual void build(std::string &content) const;
    virtual void errorFromXML(xmlNodePtr node);
    virtual void errorToXML(xmlNodePtr node) const;
    virtual void parseResponse(xmlNodePtr node);
    bool isEmptyAnswer(xmlNodePtr);
};

class SWCExchangeIface: public ExchangeIterface::ExchangeIface
{
//protected:
public:
    static void Request(xmlNodePtr reqNode, const std::string& ifaceName, SWCExchange& req);
public:
  SWCExchangeIface(const std::string &_iface):ExchangeIterface::ExchangeIface(_iface){
    domainName = "ASTRA-SWC";
  }
  //static bool isResponseHandler( const std::string& name, xmlNodePtr node );
  virtual ~SWCExchangeIface(){}

};

}

#endif // SWCEXCHANGEIFACE_H
