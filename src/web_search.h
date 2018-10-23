#ifndef __WEB_SEARCH_H__
#define __WEB_SEARCH_H__

#include "date_time.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "stages.h"
#include "dev_utils.h"
#include "rbd.h"
#include <boost/optional.hpp>

namespace WebSearch
{

int TIMEOUT();       //миллисекунды

enum XMLStyle {xmlSearchFlt, xmlSearchFltMulti, xmlSearchPNRs};

  using BASIC::date_time::TDateTime;

struct TTestPaxInfo
{
  std::string airline;
  std::string subcls;
  TPnrAddrInfo pnr_addr;
  int pax_id;
  std::string surname, name, ticket_no, document;
  int reg_no;
  TTestPaxInfo() { clear(); };

  void clear()
  {
    airline.clear();
    subcls.clear();
    pnr_addr.clear();
    pax_id=ASTRA::NoExists;
    surname.clear();
    name.clear();
    ticket_no.clear();
    document.clear();
    reg_no=ASTRA::NoExists;
  };

  void trace( TRACE_SIGNATURE ) const;
};

class SurnameFilter
{
  public:
    std::string surname;
    int surname_equal_len;

    void clear()
    {
      surname.clear();
      surname_equal_len=ASTRA::NoExists;
    }

    bool validForSearch() const;
    void addSQLTablesForSearch(const PaxOrigin& origin, std::list<std::string>& tables) const;
    void addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const;
    void addSQLParamsForSearch(QParams& params) const;
};

struct TFlightInfo;

class TPNRFilter : public SurnameFilter
{
  public:
    std::set<std::string> airlines;
    int flt_no;
    std::string suffix;
    std::vector< std::pair<TDateTime, TDateTime> > scd_out_local_ranges;
    std::vector< std::pair<TDateTime, TDateTime> > scd_out_utc_ranges;
    std::string name, pnr_addr_normal, ticket_no, document;
    int reg_no;
    std::vector<TTestPaxInfo> test_paxs;
    //BCBP_M
    bool from_scan_code;
    int name_equal_len;
    std::string airp_dep, airp_arv;

    TPNRFilter() { clear(); };

    void clear()
    {
      SurnameFilter::clear();
      airlines.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
      scd_out_local_ranges.clear();
      name.clear();
      pnr_addr_normal.clear();
      ticket_no.clear();
      document.clear();
      reg_no=ASTRA::NoExists;
      test_paxs.clear();
      //BCBP_M
      from_scan_code=false;
      name_equal_len=ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();
    };

    TPNRFilter& fromXML(xmlNodePtr fltParentNode, xmlNodePtr paxParentNode);
    TPNRFilter& testPaxFromDB();
    void trace( TRACE_SIGNATURE ) const;
    std::string getSurnameSQLFilter(const std::string &field_name, TQuery &Qry) const;
    bool isEqualPnrAddr(const TPnrAddrs &pnr_addrs) const;
    bool isEqualSurname(const std::string &pax_surname) const;
    bool isEqualName(const std::string &pax_name) const;
    bool isEqualTkn(const std::string &pax_ticket_no) const;
    bool isEqualDoc(const std::string &pax_document) const;
    bool isEqualRegNo(const int &pax_reg_no) const;
    bool isEqualFlight(const TAdvTripInfo &oper, const TSimpleMktFlight& mark) const;

    static bool userAccessIsAllowed(const TAdvTripInfo &oper, const boost::optional<TSimpleMktFlight>& mark);
};

class TPNRFilters
{
  public:
    std::list<TPNRFilter> segs;

    void clear()
    {
      segs.clear();
    }

    TPNRFilters& getBCBPSections(const std::string &bcbp, BCBPSections &sections);
    TPNRFilters& fromBCBPSections(const BCBPSections &sections);
    TPNRFilters& fromBCBP_M(const std::string &bcbp);
    TPNRFilters& fromXML(xmlNodePtr fltParentNode, xmlNodePtr paxParentNode);
};

class TMultiPNRFilters : public TPNRFilters
{
  public:
    boost::optional<int> group_id;
    bool is_main;

    void clear()
    {
      TPNRFilters::clear();
      group_id=boost::none;
      is_main=false;
    }
    TMultiPNRFilters& fromXML(xmlNodePtr fltNode, xmlNodePtr grpNode);
};

class TMultiPNRFiltersList : public std::list<TMultiPNRFilters>
{
  public:
    TMultiPNRFiltersList& fromXML(xmlNodePtr reqNode);
    static bool trueMultiRequest(xmlNodePtr reqNode)
    {
      return std::string((const char*)reqNode->name)=="SearchFltMulti";
    }
};

struct TDestInfo
{
  int point_arv;
  TDateTime scd_in_local, est_in_local, act_in_local;
  std::string airp_arv, city_arv;
  int arv_utc_offset;
  TDestInfo() { clear(); };
  TDestInfo(int point_id)
  {
    clear();
    point_arv=point_id;
  };

  void clear()
  {
    point_arv=ASTRA::NoExists;
    scd_in_local=ASTRA::NoExists;
    est_in_local=ASTRA::NoExists;
    act_in_local=ASTRA::NoExists;
    airp_arv.clear();
    city_arv.clear();
    arv_utc_offset=ASTRA::NoExists;
  };

  bool operator < (const TDestInfo &item) const
  {
    return point_arv < item.point_arv;
  };

  bool fromDB(int point_id, bool pr_throw);
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;
};

struct TFlightInfo
{
  TAdvTripInfo oper;
  TDateTime scd_out_local, est_out_local, act_out_local;
  std::string city_dep;
  int dep_utc_offset;

  //дополнительно
  std::set<TDestInfo> dests;

  TSimpleMktFlights mark;

  std::map<TStage, TTripStageTimes> stage_times;
  std::map<TStage_Type, TStage> stage_statuses;
  bool pr_paid_ckin, free_seating, have_to_select_seats;
  TFlightInfo() { clear(); }
  TFlightInfo(int point_id)
  {
    clear();
    oper.point_id=point_id;
  }

  void clear()
  {
    oper.Clear();
    scd_out_local=ASTRA::NoExists;
    est_out_local=ASTRA::NoExists;
    act_out_local=ASTRA::NoExists;
    city_dep.clear();
    dep_utc_offset=ASTRA::NoExists;
    dests.clear();
    mark.clear();
    stage_times.clear();
    stage_statuses.clear();
    pr_paid_ckin=false;
    free_seating=false;
    have_to_select_seats=false;
  };

  bool operator < (const TFlightInfo &item) const
  {
    return oper.point_id < item.oper.point_id;
  };

  void set(const TAdvTripInfo& fltInfo);
  bool fromDB(TQuery &Qry);
  bool fromDB(int point_id, bool pr_throw);
  bool fromDBadditional(bool first_segment, bool pr_throw);
  void add(const TDestInfo &dest);
  const TDestInfo& getDestInfo(int point_arv) const;
  void toXMLsimple(xmlNodePtr node, XMLStyle xmlStyle) const;
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;
  boost::optional<TStage> stage() const;
  int getStagePriority() const;
  void isSelfCheckInPossible(bool first_segment, bool notRefusalExists, bool refusalExists) const;

  static int getStagePriority(const ASTRA::TClientType& client_type,
                              const boost::optional<TStage>& checkInStage,
                              const boost::optional<TStage>& cancelStage);
  static void isSelfCheckInPossible(const ASTRA::TClientType& client_type,
                                    const boost::optional<TStage>& checkInStage,
                                    const boost::optional<TStage>& cancelStage,
                                    bool first_segment,
                                    bool notRefusalExists,
                                    bool refusalExists);
#if 0
  static void isSelfCheckInPossibleTest();
#endif

  bool setIfSuitable(const TPNRFilter &filter,
                     const TAdvTripInfo& oper,
                     const TSimpleMktFlight& mark);

  private:
    boost::optional<TStage> stage(const TStage_Type& type) const;
};

struct TPNRSegInfo
{
    int point_dep, point_arv, pnr_id;
    std::string cls;
    std::string subcls;
    TPnrAddrs pnr_addrs;
    boost::optional<TMktFlight> mktFlight;
    TPNRSegInfo() { clear(); }

  void clear()
  {
    point_dep=ASTRA::NoExists;
    point_arv=ASTRA::NoExists;
    pnr_id=ASTRA::NoExists;
    cls.clear();
    subcls.clear();
    pnr_addrs.clear();
    mktFlight=boost::none;
  }

  bool fromDB(int point_id, const TTripRoute &route, TQuery &Qry);
  bool filterFromDB(const TPNRFilter &filter);
  bool setIfSuitable(const TPNRFilter &filter,
                     const TAdvTripInfo& flt,
                     const CheckIn::TSimplePaxItem& pax);
  bool fromTestPax(int point_id, const TTripRoute &route, const TTestPaxInfo &pax);
  void getMarkFlt(const TFlightInfo &flt, TTripInfo &mark) const;
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;

  static bool isJointCheckInPossible(const TPNRSegInfo& seg1,
                                     const TPNRSegInfo& seg2,
                                     const TFlightRbd& rbds,
                                     std::list<AstraLocale::LexemaData>& errors);
};

struct TPaxInfo
{
  int pax_id;
  std::string surname, name, ticket_no, document;
  int reg_no;
  TPaxInfo() { clear(); };

  void clear()
  {
    pax_id=ASTRA::NoExists;
    surname.clear();
    name.clear();
    ticket_no.clear();
    document.clear();
    reg_no=ASTRA::NoExists;
  };

  bool operator < (const TPaxInfo &item) const
  {
    return pax_id < item.pax_id;
  };

  bool filterFromDB(const TPNRFilter &filter, TQuery &Qry, bool ignore_reg_no);
  bool setIfSuitable(const TPNRFilter& filter, const CheckIn::TSimplePaxItem& pax, bool ignore_reg_no=false);
  bool fromTestPax(const TTestPaxInfo &pax);
  void toXML(xmlNodePtr node) const;
};

struct TPNRInfo
{
  std::map< int/*num*/, TPNRSegInfo > segs;
  std::set<TPaxInfo> paxs;
  //дополнительно
  int bag_norm;
  TPNRInfo():
    bag_norm(ASTRA::NoExists) {};

  void add(const TPaxInfo &pax);
  bool fromDBadditional(const TFlightInfo &flt, const TDestInfo &dest);
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;

  int getFirstPointDep() const;
};

struct TPNRs
{
  std::set<TFlightInfo> flights;
  std::map< int/*pnr_id*/, TPNRInfo > pnrs; //все PNR, которые подходят к критериям поиска
  boost::optional<AstraLocale::LexemaData> error;

  bool add(const TFlightInfo &flt, const TPNRSegInfo &seg, const TPaxInfo &pax, bool is_test);
  const TFlightInfo& getFlightInfo(int point_dep) const;
  const TPNRInfo& getPNRInfo(int pnr_id) const;
  int getFirstPnrId() const;
  const TPNRInfo& getFirstPNRInfo() const;
  TPNRInfo& getFirstPNRInfo();

  int calcStagePriority(int pnr_id) const;
  boost::optional<TDateTime> getFirstSegTime() const;
  void toXML(xmlNodePtr node, bool is_primary, XMLStyle xmlStyle) const;
  void trace(XMLStyle xmlStyle) const;
};

class TPNRsSortOrder
{
  private:
    TDateTime _timePoint;
  public:
    TPNRsSortOrder(const TDateTime& timePoint) : _timePoint(timePoint) {}
    bool operator () (const TPNRs &item1, const TPNRs &item2) const;
};

class TMultiPNRs : public TPNRs
{
  public:
    std::list<AstraLocale::LexemaData> errors;
    std::list<AstraLocale::LexemaData> warnings;
    boost::optional<int> group_id;

    TMultiPNRs(const TMultiPNRFilters& filters,
               const TPNRs& pnrs) :
      TPNRs(pnrs), group_id(filters.group_id) {}

    TMultiPNRs(const TMultiPNRFilters& filters,
               const std::list<AstraLocale::LexemaData> _errors) :
      errors(_errors), group_id(filters.group_id) {}

    void toXML(xmlNodePtr segsParentNode, xmlNodePtr grpsParentNode, bool is_primary, XMLStyle xmlStyle) const;
    void truncateSegments(int numOfSegs);

    static int numberOfCompatibleSegments(const TMultiPNRs& PNRs1,
                                          const TMultiPNRs& PNRs2,
                                          std::map<int/*point_id*/, TFlightRbd>& flightRbdMap,
                                          std::list<AstraLocale::LexemaData>& errors);
};

class TMultiPNRsList
{
  public:
    std::list<TMultiPNRs> primary;
    std::list<TMultiPNRs> secondary;

    void add(const TMultiPNRFilters& filters,
             const TPNRs& pnrs)
    {
      (filters.is_main?primary:secondary).emplace_back(filters, pnrs);
    }

    void add(const TMultiPNRFilters& filters,
             const std::list<AstraLocale::LexemaData> _errors)
    {
      (filters.is_main?primary:secondary).emplace_back(filters, _errors);
    }
    void checkGroups();
    void checkSegmentCompatibility();
    void toXML(xmlNodePtr resNode) const;
    static bool trueMultiResponse(xmlNodePtr resNode)
    {
      return std::string((const char*)resNode->name)=="SearchFltMulti";
    }
};

void findPNRs(const TPNRFilter &filter, TPNRs &PNRs, bool ignore_reg_no=false);

struct TPnrData
{
  TFlightInfo flt;
  TDestInfo dest;
  TPNRSegInfo seg;
};

void getTCkinData( const TFlightInfo& first_flt,
                   const TDestInfo& first_dest,
                   const TPNRSegInfo& first_seg,
                   bool is_test,
                   std::vector<TPnrData> &other);

void getTCkinData( const TPnrData &first,
                   bool is_test,
                   std::vector<TPnrData> &other);

} // namespace WebSearch


#endif // __WEB_SEARCH_H__

