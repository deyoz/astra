#ifndef _BRANDS_H
#define _BRANDS_H

#include <list>
#include <string>

struct TBrands {
    std::list<std::string> items;
    void get(int pax_id);
    void get(const std::string &airline, const std::string &fare_basis);
};

#endif
