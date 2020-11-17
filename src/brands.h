#ifndef _BRANDS_H
#define _BRANDS_H

#include <list>
#include <string>
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_elems.h"
#include "etick.h"
#include "date_time.h"

using BASIC::date_time::TDateTime;

namespace ASTRA
{

template<typename T>
class Range
{
  private:
    boost::optional<T> lower_bound;
    boost::optional<T> upper_open_bound;

  public:
    Range(const boost::optional<T>& _lower_bound, const boost::optional<T>& _upper_open_bound) :
      lower_bound(_lower_bound), upper_open_bound(_upper_open_bound) {}

    bool contains(const T& value) const
    {
      return (!lower_bound || value>=lower_bound.get()) &&
             (!upper_open_bound || value<upper_open_bound.get());
    }
};

}

class TBrand
{
  public:
    int id;
    std::string oper_airline;

    TBrand() { clear(); }
    TBrand(int _id, const std::string& _oper_airline) :
      id(_id), oper_airline(_oper_airline) {}

    void clear()
    {
      id=ASTRA::NoExists;
      oper_airline.clear();
    }

    bool empty() const { return id==ASTRA::NoExists; }

    std::string code() const;
    std::string name(const AstraLocale::OutputLang& lang) const;

    const TBrand& toWebXML(xmlNodePtr node,
                           const AstraLocale::OutputLang& lang) const;

    class Key
    {
      public:
        AirlineCode_t airlineOper;
        std::string code;

        Key(const AirlineCode_t& airlineOper_, const std::string& code_) :
          airlineOper(airlineOper_), code(code_) {}

        bool operator == (const Key &key) const
        {
          return airlineOper==key.airlineOper && code==key.code;
        }
    };

    Key key() const
    {
      return Key(AirlineCode_t(oper_airline), code());
    }
};

std::ostream& operator<<(std::ostream& os, const TBrand::Key& brand);

typedef std::list<std::pair<int, ASTRA::Range<TDateTime> > > BrandIdsWithDateRanges;

class TBrands : public std::list<TBrand>
{
  private:
    std::map<std::pair<std::string, std::string>, BrandIdsWithDateRanges> secretMap;
    int getsTotal, getsCached;
  public:
    TBrands() : getsTotal(0), getsCached(0) {}
    void get(int pax_id);
    void get(const std::string &airline, const TETickItem &etick);
    TBrand getSingleBrand() const;
    void traceCaching() const;
};

#endif
