#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include <string>
#include "basic.h"
#include "tlg/tlg_parser.h"

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs, std::string airline="");
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string airline="");
BASIC::TDateTime DayToDate(int day, BASIC::TDateTime base_date);

struct TTripRouteItem {
    std::string airp, city;
    int point_num;
    TTripRouteItem() { point_num = ASTRA::NoExists; };
};

struct TTripRoute {
    public:
        std::vector<TTripRouteItem> items;
        void get(int point_id);
};

#endif /*_ASTRA_MISC_H_*/


