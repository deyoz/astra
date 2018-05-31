#pragma once

#include <string>
#include "astra_consts.h"
#include "oralib.h"
#include "passenger.h"

class PaxInfoForSearch
{
  public:
    CheckIn::TSimplePaxItem pax;
    boost::optional<TAdvTripInfo> flt;
    boost::optional<int> stagePriority;
    boost::optional<double> timeOutAbsDistance;

    PaxInfoForSearch(const CheckIn::TSimplePaxItem& _pax,
                     const boost::optional<TAdvTripInfo>& _flt,
                     const TDateTime& timePoint);
    static int calcStagePriority(const TAdvTripInfo& flt);

    bool operator < (const PaxInfoForSearch &info) const;
};

class PaxInfoForSearchList : public std::multiset<PaxInfoForSearch>
{
  public:
    PaxInfoForSearchList(const CheckIn::TSimplePaxList& paxs);
    void trace() const;
};

std::string getSearchPaxSubquery(const ASTRA::TPaxStatus& pax_status,
                                 const bool& return_pnr_ids,
                                 const bool& exclude_checked,
                                 const bool& exclude_deleted,
                                 const bool& select_pad_with_ok,
                                 const std::string& sql_filter);

void executeSearchPaxQuery(const int& point_dep,
                           const ASTRA::TPaxStatus& pax_status,
                           const bool& return_pnr_ids,
                           const std::string& sql_filter,
                           TQuery& Qry);
