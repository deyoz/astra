#ifndef _DOCS_PAX_LIST_H_
#define _DOCS_PAX_LIST_H_

#include "pax_list.h"
#include <string>
#include "docs.h"

namespace REPORTS {

    struct TPMPaxList: public TPaxList {
        TRptParams &rpt_params;
        TPaxPtr getPaxPtr();
        TPMPaxList(TRptParams &_rpt_params):
            TPaxList(_rpt_params.point_id, retRPT_PM),
            rpt_params(_rpt_params)
        {};
    };

    struct TPMPax: public TPax {
        std::string _seat_no; // # места с пробелом в начале, для сортировки


        void fromDB(TQuery &Qry);
        void trace(TRACE_SIGNATURE);
        TPMPaxList &get_pax_list() const;
        TPMPax &pm_pax(TPaxPtr val) const;

        std::string target;
        std::string last_target;
        std::string status;
        int point_id;
        int class_grp;
        int priority;
        std::string cls;

        int pr_trfer;
        std::string trfer_airline;
        int trfer_flt_no;
        std::string trfer_suffix;
        std::string trfer_airp_arv;
        TDateTime trfer_scd;

        int _rk_weight;
        int _bag_amount;
        int _bag_weight;
        TBagKilos excess_wt;
        TBagPieces excess_pc;
        std::multiset<TBagTagNumber> _tags;
        std::multiset<CheckIn::TPaxRemItem> _rems;

        void clear()
        {
            TPax::clear();
            _seat_no.clear();
            target.clear();
            last_target.clear();
            status.clear();
            point_id = ASTRA::NoExists;
            class_grp = ASTRA::NoExists;
            priority = ASTRA::NoExists;
            cls.clear();

            pr_trfer = 0;
            trfer_airline.clear();
            trfer_flt_no = ASTRA::NoExists;
            trfer_suffix.clear();
            trfer_airp_arv.clear();
            trfer_scd = ASTRA::NoExists;

            _rk_weight = ASTRA::NoExists;
            _bag_amount = ASTRA::NoExists;
            _bag_weight = ASTRA::NoExists;
            excess_wt = ASTRA::NoExists;
            excess_pc = ASTRA::NoExists;
            _tags.clear();
            _rems.clear();
        }

        TPMPax(TPaxList &_pax_list):
            TPax(_pax_list)
            ,excess_wt(ASTRA::NoExists)
            ,excess_pc(ASTRA::NoExists)
        {
            clear();
        }

        int seats() const;
        std::string seat_no() const;
        int bag_amount() const;
        int bag_weight() const;
        int rk_weight() const;
        std::string get_tags() const;
        std::string rems() const;
    };

    std::string get_last_target(TQuery &Qry, TRptParams &rpt_params);
    int nosir_cbbg(int argc, char** argv);
    bool pax_compare(TPaxPtr pax1, TPaxPtr pax2);
}

#endif
