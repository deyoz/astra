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
            void dump(const std::string &file, int line) const;
            TBagTags() { clear(); }
        };
        typedef std::map<int, TBagTags> TBagIdMap;
        int grp_id;
        TBagIdMap items;

        TBagIdMap backup_items;

        void backup();
        void toDB(const TBagIdMap &items, TDateTime time_annul);
        bool find_tag(const CheckIn::TTagItem &tag) const;

        struct TRMExistTags {
            struct TCheck {
                const TAnnulBT &annul_bt;
                bool operator()(const CheckIn::TTagItem &tag);
                TCheck(const TAnnulBT &vannul_bt): annul_bt(vannul_bt) {}
            };
            bool exec(std::list<CheckIn::TTagItem> &bag_tags, const TAnnulBT &annul_bt);
        };

    public:

        int get_grp_id() const { return grp_id; }
        void get(int grp_id);
        void minus(const TAnnulBT &annul_bt);
        void toDB();
        void dump() const;
        void clear();
        TAnnulBT() { clear(); }
};

#endif
