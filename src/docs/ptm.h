#ifndef _DOCS_PTM_H_
#define _DOCS_PTM_H_

#include "pax_list.h"
#include <string>
#include "common.h"

namespace REPORTS {

    struct TPMPaxList: public TPaxList {
        TRptParams &rpt_params;
        TPaxPtr getPaxPtr();
        TPMPaxList(TRptParams &_rpt_params):
            TPaxList(_rpt_params.point_id),
            rpt_params(_rpt_params)
        {
            options.rem_event_type = retRPT_PM;
            options.mkt_flt = _rpt_params.mkt_flt;
            options.flags.setFlag(oeSeatNo);
            options.flags.setFlag(oeTags);
        };
    };

    struct TPMPax: public TPax {


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

        void clear()
        {
            TPax::clear();
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
        }

        TPMPax(TPaxList &_pax_list):
            TPax(_pax_list)
        {
            clear();
        }
        std::string rems() const;
    };

    std::string get_last_target(TQuery &Qry, TRptParams &rpt_params);
    int nosir_cbbg(int argc, char** argv);
    bool pax_compare(TPaxPtr pax1, TPaxPtr pax2);

    void PTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
}

#endif
