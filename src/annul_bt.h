#ifndef _ANNUL_BT_H_
#define _ANNUL_BT_H_

#include "baggage.h"

struct TAnnulBT {
    private:
        struct TBagTags {
            int pax_id;
            TDateTime time_annul;
            CheckIn::TBagItem bag_item;
            std::list<CheckIn::TTagItem> bag_tags;
            void clear();
            TBagTags() { clear(); }
        };
        typedef std::map<int, TBagTags> TBagNumMap;
        int grp_id;
        TBagNumMap items;

        TBagNumMap backup_items;

        void backup();
        void toDB(const TBagNumMap &items, TDateTime time_annul);
    public:

        int get_grp_id() const { return grp_id; }
        void get(int grp_id);
        void minus(const std::map<int, CheckIn::TBagItem> &bag_items);
        void minus(const TAnnulBT &annul_bt);
        void toDB();
        void dump();
        void clear();
        TAnnulBT() { clear(); }
};

#endif
