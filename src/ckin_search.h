#pragma once

#include <string>
#include "astra_consts.h"
#include "oralib.h"
#include "passenger.h"
#include "qrys.h"

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

namespace CheckIn
{

class Search
{
  private:
    boost::optional<int> timeout;

    boost::posix_time::ptime startTime;

    PaxOrigin origin;
    std::list<std::string> tables;
    std::list<std::string> conditions;
    QParams params;
    mutable std::set<int> foundPaxIds;

    bool incomplete;

    void initSimplestSearch(const PaxOrigin& _origin)
    {
      origin=_origin;
      tables.clear();
      conditions.clear();
      params.clear();
    }

    template <class Criterion, class ... Criterions>
    void getSQLProperties(const Criterion& criterion,
                          const Criterions& ... criterions)
    {
      if (criterion.validForSearch())
      {
        criterion.addSQLTablesForSearch(origin, tables);
        criterion.addSQLConditionsForSearch(origin, conditions);
        criterion.addSQLParamsForSearch(params);
      }

      getSQLProperties(criterions...);
    }

    void getSQLProperties() {}

    std::string getSQLText() const;

    bool addPassengers(CheckIn::TSimplePaxList& paxs) const;

    bool timeIsUp() const;

  public:
    Search(const boost::optional<int>& _timeout=boost::none) :
      timeout(_timeout), incomplete(false) {}

    template <class ... Criterions>
    void operator () (CheckIn::TSimplePaxList& paxs, const Criterions& ... criterions)
    {
      paxs.clear();

      startTime=boost::posix_time::microsec_clock::local_time();

      foundPaxIds.clear();
      for(const PaxOrigin& o : { paxCheckIn, paxPnl })
      {
        initSimplestSearch(o);

        getSQLProperties(criterions...);

        if (!addPassengers(paxs)) incomplete=true;
      }
    }

    bool timeoutIsReached() const { return incomplete; }
};

} //namespace CheckIn


