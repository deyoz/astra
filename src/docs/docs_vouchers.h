#ifndef _DOCS_VOUCHERS_H_
#define _DOCS_VOUCHERS_H_

#include <map>
#include "oralib.h"

struct TVouchers {

    struct TPaxInfo {
        std::string full_name;
        std::string pers_type;
        int reg_no;
        std::string ticket_no;
        int coupon_no;
        std::string rem_codes;
        std::string voucher;
        bool pr_del;

        void clear();

        TPaxInfo(TQuery &Qry, bool pr_del) { fromDB(Qry, pr_del); }
        void fromDB(TQuery &Qry, bool pr_del);
        bool operator < (const TPaxInfo &item) const;
    };

    struct TItems: public std::map<TPaxInfo, int> {
        void add(TQuery &Qry, bool pr_del);
    };

    int point_id;
    TItems items;

    void clear();

    const TVouchers &fromDB(int point_id);
    const TVouchers &fromDB(int point_id, int grp_id);
    void to_deleted() const;
    TVouchers() { clear(); }
};

#endif
