#ifndef _ANNUL_BT_H_
#define _ANNUL_BT_H_

#include "baggage.h"

struct TAnnulBT {
    struct TBagTags {
        CheckIn::TBagItem bag_item;
        std::list<CheckIn::TTagItem> bag_tags;
        void add(const CheckIn::TBagItem &abag_item);
    };
    typedef std::map<std::string, TBagTags> TRFISCMap;
    typedef std::map<int, TRFISCMap> TBagTypeMap;
    typedef std::map<int, TBagTypeMap> TPaxIdMap;

    int grp_id;

    TPaxIdMap items;

    void get(int grp_id);
    void minus(const std::map<int, CheckIn::TBagItem> &bag_items);
    void minus(const TAnnulBT &annul_bt);
    void toDB();
    void dump();
    void clear();
    TAnnulBT() { clear(); }
};

#endif
