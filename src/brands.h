#ifndef _BRANDS_H
#define _BRANDS_H

#include <list>
#include <string>
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_elems.h"

struct TBrand
{
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

  std::string name(const AstraLocale::OutputLang& lang) const;

  const TBrand& toWebXML(xmlNodePtr node,
                         const AstraLocale::OutputLang& lang) const;
};

typedef std::list<int> BrandIds;

struct TBrands {
  private:
    std::map<std::pair<std::string, std::string>, BrandIds> secretMap;
    int getsTotal, getsCached;
  public:
    TBrands() : getsTotal(0), getsCached(0) {}
    BrandIds brandIds;
    std::string oper_airline;
    void get(int pax_id);
    void get(const std::string &airline, const std::string &fare_basis);
    TBrand getSingleBrand() const;
    void traceCaching() const;
};

#endif
