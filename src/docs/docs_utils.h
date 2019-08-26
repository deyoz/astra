#ifndef _DOCS_UTILS_H_
#define _DOCS_UTILS_H_

#include <string>
#include "oralib.h"
#include "remarks.h"
#include "docs_common.h"

std::string get_test_str(int page_width, std::string lang);
bool old_cbbg();

namespace REPORT_PAX_REMS {
    void get(TQuery &Qry, const std::string &lang, const std::map< TRemCategory, std::vector<std::string> > &filter, std::multiset<CheckIn::TPaxRemItem> &final_rems);
    void get(TQuery &Qry, const std::string &lang, std::multiset<CheckIn::TPaxRemItem> &final_rems);
    void get_rem_codes(TQuery &Qry, const std::string &lang, std::set<std::string> &rem_codes);
}

std::string get_last_target(TQuery &Qry, TRptParams &rpt_params);

#endif
