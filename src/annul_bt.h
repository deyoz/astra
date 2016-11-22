#ifndef _ANNUL_BT_H_
#define _ANNUL_BT_H_

#include "baggage.h"

struct TAnnulBT {
    struct TBagTags {
        CheckIn::TBagItem bag_item;
        std::list<CheckIn::TTagItem> bag_tags;
    };
    typedef std::map<int, TBagTags> TBagNumMap;

    int point_id;
    std::string trfer_airline;
    int trfer_flt_no;
    std::string trfer_suffix;
    TDateTime trfer_scd;

    TBagNumMap items;

    void get(int grp_id);
    void minus(const std::map<int, CheckIn::TBagItem> &bag_items);
    void minus(const TAnnulBT &annul_bt);
    void toDB();
    void dump();
    void clear();
    TAnnulBT() { clear(); }
};

#endif
