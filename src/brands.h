#ifndef _BRANDS_H
#define _BRANDS_H

#include <list>
#include <string>
#include "xml_unit.h"

struct TBrand
{
  std::string oper_airline;
  std::string code;
  std::string name;
  void clear() {
    oper_airline.clear();
    code.clear();
    name.clear();
  }
  void toXML( xmlNodePtr brandNode ) const;
};

struct TBrands {
    typedef std::list<int> TItems; // <brands.id, brands.code>
    TItems items;
    std::string oper_airline;
    void get(int pax_id);
    void get(const std::string &airline, const std::string &fare_basis);
    bool getBrand( TBrand &brand, std::string lang ) const;
    bool getBrand( int id, TBrand &brand, std::string lang ) const;
};

#endif
