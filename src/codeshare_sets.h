#pragma once

#include <set>
#include <string>
#include "astra_types.h"
#include "astra_misc.h"

namespace CodeshareSet {
    int add(int id, const std::string& airline_oper, int flt_no_oper, const std::string& suffix_oper,
        const std::string& airp_dep, const std::string& airline_mark, int flt_no_mark, const std::string& suffix_mark,
        bool pr_mark_norms, bool pr_mark_bp, bool pr_mark_rpt,
        const std::string& days, TDateTime first_date, TDateTime last_date, bool pr_denial, int tid);

    void modifyById(int id, TDateTime last_date, int tid = ASTRA::NoExists);

    void deleteById(int id, int tid = ASTRA::NoExists);
}