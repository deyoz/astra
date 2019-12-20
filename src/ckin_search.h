#pragma once

#include <string>
#include "astra_consts.h"
#include "oralib.h"
#include "passenger.h"
#include "qrys.h"
#include "dev_utils.h"

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

class ClassesList
{
  private:
    std::string cls;
  public:
    void add(const std::string& cl);
    bool noClasses() const { return cls.empty(); }
    bool moreThanOneClass() const { return cls.size()>1; }
    bool strictlyOneClass() const { return cls.size()==1; }
    std::string view() const;
    std::string getStrictlyOneClass() const;

    bool operator == (const ClassesList &classesList) const
    {
      return cls==classesList.cls;
    }
};

std::string getSearchPaxSubquery(const ASTRA::TPaxStatus& pax_status,
                                 const bool& return_pnr_ids,
                                 const bool& exclude_checked,
                                 const bool& exclude_deleted,
                                 const bool& select_pad_with_ok,
                                 const std::string& sql_filter);

void getTCkinSearchPaxQuery(TQuery& Qry);

void executeSearchPaxQuery(const int& pnr_id,
                           TQuery& Qry);

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
    std::list<PaxOrigin> originList;
    boost::optional<int> timeout;

    boost::posix_time::ptime startTime;

    PaxOrigin origin;
    std::set<std::string> tables;
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
        criterion.addSQLParamsForSearch(origin, params);
      }

      getSQLProperties(criterions...);
    }

    void getSQLProperties() {}

    template <class Criterion, class ... Criterions>
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax,
                             const Criterion& criterion,
                             const Criterions& ... criterions)
    {
      if (!criterion.finalPassengerCheck(pax)) return false;

      return finalPassengerCheck(pax, criterions...);
    }

    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) { return true; }

    std::string getSQLText() const;

    bool addPassengers(CheckIn::TSimplePaxList& paxs) const;

    bool timeIsUp() const;

  public:
    Search(const boost::optional<int>& _timeout=boost::none) :
      originList({ paxCheckIn, paxPnl }), timeout(_timeout), incomplete(false) {}
    Search(const PaxOrigin& _origin,
           const boost::optional<int>& _timeout=boost::none) :
      originList(1, _origin), timeout(_timeout), incomplete(false) {}

    template <class ... Criterions>
    void operator () (CheckIn::TSimplePaxList& paxs, const Criterions& ... criterions)
    {
      paxs.clear();

      startTime=boost::posix_time::microsec_clock::local_time();

      foundPaxIds.clear();
      for(const PaxOrigin& o : originList)
      {
        initSimplestSearch(o);

        getSQLProperties(criterions...);

        if (!addPassengers(paxs)) incomplete=true;
      }

      for(CheckIn::TSimplePaxList::iterator p=paxs.begin(); p!=paxs.end();)
        if (finalPassengerCheck(*p, criterions...)) ++p; else p=paxs.erase(p);
    }

    bool timeoutIsReached() const { return incomplete; }
};

} //namespace CheckIn

class PaxIdFilter
{
  private:
    int value;
  public:
    explicit PaxIdFilter(int _value) : value(_value) {}

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const {}
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const { return true; }
};

class SurnameFilter
{
  public:
    std::string surname;
    bool checkSurnameEqualBeginning;

    SurnameFilter() { clear(); }

    void clear()
    {
      surname.clear();
      checkSurnameEqualBeginning=false;
    }

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const {}
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const { return true; }
    bool suitable(const CheckIn::TSimplePaxItem& pax) const;
};

class FullnameFilter : public SurnameFilter
{
  private:
    std::string transformName(const std::string& name) const;
  public:
    std::string name;
    bool checkNameEqualBeginning;
    bool checkFirstNameOnly;

    static std::string firstName(const std::string& name);

    FullnameFilter() { clear(); }

    void clear()
    {
      SurnameFilter::clear();
      name.clear();
      checkNameEqualBeginning=false;
      checkFirstNameOnly=false;
    }

    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const;
    bool suitable(const CheckIn::TSimplePaxItem& pax) const;
};

class BarcodePaxFilter : public FullnameFilter
{
  public:
    int reg_no;

    BarcodePaxFilter() { clear(); }

    void clear()
    {
      FullnameFilter::clear();
      reg_no=ASTRA::NoExists;
      checkFirstNameOnly=true;
    }

    void set(const BCBPUniqueSections& unique,
             const BCBPRepeatedSections& repeated);

    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const;
    bool suitable(const CheckIn::TSimplePaxItem& pax) const;
};

class TCkinPaxFilter : public FullnameFilter
{
  public:
    std::string subclass;
    ASTRA::TPerson pers_type;
    int seats;

    TCkinPaxFilter(const CheckIn::TSimplePaxItem& pax)
    {
      clear();
      surname=pax.surname;
      name=pax.name;
      subclass=pax.getCabinSubclass();
      pers_type=pax.pers_type;
      seats=pax.seats;
    }

    void clear()
    {
      FullnameFilter::clear();
      subclass.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
    }

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const;
    bool suitable(const CheckIn::TSimplePaxItem& pax) const;
};

class FlightFilter : public TTripInfo
{
  public:
    TDateTime min_scd_out, max_scd_out;
    bool scdOutIsLocal;
    std::string airp_arv;

    FlightFilter() { clear(); }
    FlightFilter(const TTripInfo& flt)
    {
      clear();
      TTripInfo::operator = (flt);
      if (flt_no==0) flt_no=ASTRA::NoExists;
    }

    void clear()
    {
      TTripInfo::Clear();
      flt_no=ASTRA::NoExists;
      min_scd_out=ASTRA::NoExists;
      max_scd_out=ASTRA::NoExists;
      scdOutIsLocal=false;
      airp_arv.clear();
    }

    void setLocalDate(TDateTime localDate);

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const;
    bool finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const { return true; }
    bool suitable(const TAdvTripRouteItem& departure,
                  const TAdvTripRouteItem& arrival) const;
};

class BarcodeSegmentFilter
{
  public:
    FlightFilter seg;
    BarcodePaxFilter pax;
    TPnrAddrInfo pnr;

    BarcodeSegmentFilter() { clear(); }

    void clear()
    {
      seg.clear();
      pax.clear();
      pnr.clear();
    }

    void set(const BCBPUniqueSections& unique,
             const BCBPRepeatedSections& repeated);
};

class BarcodeFilter : public std::list<BarcodeSegmentFilter>
{
  public:
    void set(const std::string &barcode);
    void getPassengers(CheckIn::Search &search, CheckIn::TSimplePaxList& paxs, bool checkOnlyFullname) const;
    bool suitable(const TAdvTripRouteItem& departure,
                  const TAdvTripRouteItem& arrival,
                  const CheckIn::TSimplePaxItem& pax,
                  const TPnrAddrs& pnrs,
                  bool checkOnlyFullname) const;
};

