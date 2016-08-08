#ifndef _SIRENA_EXCHANGE_H_
#define _SIRENA_EXCHANGE_H_

#include <string>
#include "astra_consts.h"
#include "date_time.h"
#include "xml_unit.h"
#include "httpClient.h"

namespace SirenaExchange
{

using BASIC::date_time::TDateTime;

class TErrorReference
{
  public:
    std::string path, value;
    int pax_id, seg_id;
    TErrorReference()
    {
      clear();
    }
    void clear()
    {
      path.clear();
      value.clear();
      pax_id=ASTRA::NoExists;
      seg_id=ASTRA::NoExists;
    }
    bool empty() const
    {
      return path.empty() &&
             value.empty() &&
             pax_id==ASTRA::NoExists &&
             seg_id==ASTRA::NoExists;
    }
    void toXML(xmlNodePtr node) const;
    void fromXML(xmlNodePtr node);
    std::string traceStr() const;
};

class TExchange
{
  public:
    virtual std::string exchangeId() const=0;
  protected:
    virtual bool isRequest() const=0;
  public:
    std::string error_code, error_message;
    TErrorReference error_reference;
    virtual void build(std::string &content) const;
    virtual void parse(const std::string &content);
    virtual void toXML(xmlNodePtr node) const;
    virtual void fromXML(xmlNodePtr node);
    virtual void errorToXML(xmlNodePtr node) const;
    virtual void errorFromXML(xmlNodePtr node);
    bool error() const;
    std::string traceError() const;
    virtual void clear()=0;
    virtual ~TExchange() {}
};

class TErrorRes : public TExchange
{
  public:
    virtual std::string exchangeId() const { return id; }
  protected:
    virtual bool isRequest() const { return false; }
  public:
    std::string id;
    TErrorRes(const std::string &_id) : id(_id) {}
    virtual void clear() {}
};


void SendRequest(const TExchange &request, TExchange &response,
                 RequestInfo &requestInfo, ResponseInfo &responseInfo);
void SendRequest(const TExchange &request, TExchange &response);

class TLastExchangeInfo
{
  public:
    int grp_id;
    std::string pc_payment_req, pc_payment_res;
    TDateTime pc_payment_req_created, pc_payment_res_created;
    void clear()
    {
      grp_id=ASTRA::NoExists;
      pc_payment_req.clear();
      pc_payment_res.clear();
      pc_payment_req_created=ASTRA::NoExists;
      pc_payment_res_created=ASTRA::NoExists;
    }
    TLastExchangeInfo()
    {
      clear();
    }
    void toDB();
    void fromDB(int grp_id);
    static void cleanOldRecords();
};



class TLastExchangeList : public std::list<TLastExchangeInfo>
{
  public:
    void handle(const std::string& where);
};

void SendTestRequest(const std::string &req);

std::string airlineToXML(const std::string &code, const std::string &lang);
std::string airpToXML(const std::string &code, const std::string &lang);

} //namespace SirenaExchange

#endif //_SIRENA_EXCHANGE_H_



