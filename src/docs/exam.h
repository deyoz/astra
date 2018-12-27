#ifndef _DOCS_EXAM_H_
#define _DOCS_EXAM_H_

#include "pax_list.h"
#include "common.h"

namespace REPORTS {

    struct TEXAMPaxList: public TPaxList {
        TRptParams &rpt_params;
        TEXAMPaxList(TRptParams &_rpt_params):
            TPaxList(_rpt_params.point_id),
            rpt_params(_rpt_params)
        {
            rem_event_type = retBRD_VIEW;
            lang = _rpt_params.GetLang();
        };
    };
    void EXAM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
    void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
}

#endif
