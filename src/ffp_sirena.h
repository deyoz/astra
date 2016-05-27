#ifndef _FFP_SIRENA_H_
#define _FFP_SIRENA_H_

#include "sirena_exchange.h"

namespace SirenaExchange
{

//запросы Астры в Сирену
class TFFPInfoExchange : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TFFPItem
{
  public:
    std::string company, card_number;
    void clear()
    {
      company.clear();
      card_number.clear();
    }
    void set(const std::string &_company, const std::string &_card_number)
    {
      company=_company;
      card_number=_card_number;
    }
    const TFFPItem& toXML(xmlNodePtr node, const std::string &lang) const;
    TFFPItem& fromXML(xmlNodePtr node);
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "company='" << company << "' card_number='" << card_number << "'";
      return s.str();
    }
};

class TFFPInfoReq : public TFFPInfoExchange, public TFFPItem
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    virtual void clear()
    {
      TFFPItem::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

class TFFPInfoNameItem
{
  public:
    std::string first, last;
    void clear()
    {
      first.clear();
      last.clear();
    }
    TFFPInfoNameItem& fromXML(xmlNodePtr node);
};

class TFFPInfoRes : public TFFPInfoExchange, public TFFPItem
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    std::string company, card_number, status;
    std::list<TFFPInfoNameItem> names;
    virtual void clear()
    {
      company.clear();
      card_number.clear();
      status.clear();
      names.clear();
    }
    virtual void fromXML(xmlNodePtr node);
    std::string traceStr() const
    {
      std::ostringstream s;
      s << TFFPItem::traceStr() << std::endl
        << "status='" << status << "'";
      for(std::list<TFFPInfoNameItem>::const_iterator i=names.begin(); i!=names.end(); ++i)
        s << std::endl << "name: first='" << i->first << "' last='" << i->last << "'";
      return s.str();
    }
};

} //namespace SirenaExchange

void get_ffp_status(const SirenaExchange::TFFPInfoReq &req, SirenaExchange::TFFPInfoRes &res);

int ffp(int argc,char **argv);
void ffp_help(const char *name);

#endif //_FFP_SIRENA_H_
