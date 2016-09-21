#ifndef _BRANDS_H
#define _BRANDS_H

#include <list>
#include <string>

struct TBrands {
    typedef std::list<std::pair<int, std::string> > TItems; // <brands.id, brands.code>
    TItems items;
    void get(int pax_id);
    void get(const std::string &airline, const std::string &fare_basis);
};

#endif
