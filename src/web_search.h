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

class TPNRFilter
{
  public:
    std::set<std::string> airlines;
    int flt_no;
    std::string suffix;
    std::vector< std::pair<TDateTime, TDateTime> > scd_out_local_ranges;
    std::vector< std::pair<TDateTime, TDateTime> > scd_out_utc_ranges;
    std::string surname, name, pnr_addr_normal, ticket_no, document;
    int reg_no;
    std::vector<TTestPaxInfo> test_paxs;
    //BCBP_M
    bool from_scan_code;
    int surname_equal_len, name_equal_len;
    std::string airp_dep, airp_arv;

    TPNRFilter() { clear(); };

    void clear()
    {
      airlines.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
      scd_out_local_ranges.clear();
      surname.clear();
      name.clear();
      pnr_addr_normal.clear();
      ticket_no.clear();
      document.clear();
      reg_no=ASTRA::NoExists;
      test_paxs.clear();
      //BCBP_M
      from_scan_code=false;
      surname_equal_len=ASTRA::NoExists;
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
  int point_dep;
  int point_num;
  int first_point;
  bool pr_tranzit;
  TTripInfo oper;
  std::string craft;
  int craft_fmt;
  TDateTime scd_out_local, est_out_local, act_out_local;
  std::string city_dep;
  int dep_utc_offset;

  //дополнительно
  std::set<TDestInfo> dests;

  std::vector<TTripInfo> mark;

  std::map<TStage, TTripStageTimes> stage_times;
  std::map<TStage_Type, TStage> stage_statuses;
  bool pr_paid_ckin, free_seating, have_to_select_seats;
  TFlightInfo() { clear(); };
  TFlightInfo(int point_id)
  {
    clear();
    point_dep=point_id;
  };

  void clear()
  {
    point_dep=ASTRA::NoExists;
    oper.Clear();
    craft.clear();
    craft_fmt=ASTRA::NoExists;
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
    return point_dep < item.point_dep;
  };

  bool fromDB(TQuery &Qry);
  bool fromDB(int point_id, bool first_segment, bool pr_throw);
  bool fromDBadditional(bool first_segment, bool pr_throw);
  void add(const TDestInfo &dest);
  const TDestInfo& getDestInfo(int point_arv) const;
  void toXMLsimple(xmlNodePtr node, XMLStyle xmlStyle) const;
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;
  boost::optional<TStage> stage() const;
};

struct TPNRSegInfo
{
  int point_dep, point_arv, pnr_id;
  std::string cls;
    std::string subcls;
    TPnrAddrs pnr_addrs;
    TMktFlight mktFlight;
    TPNRSegInfo() { clear(); };

  void clear()
  {
    point_dep=ASTRA::NoExists;
    point_arv=ASTRA::NoExists;
    pnr_id=ASTRA::NoExists;
    cls.clear();
    subcls.clear();
    pnr_addrs.clear();
  };

  bool fromDB(int point_id, const TTripRoute &route, TQuery &Qry);
  bool filterFromDB(const TPNRFilter &filter);
  bool fromTestPax(int point_id, const TTripRoute &route, const TTestPaxInfo &pax);
  void getMarkFlt(const TFlightInfo &flt, bool is_test, TTripInfo &mark) const;
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
  bool fromDBadditional(const TFlightInfo &flt, const TDestInfo &dest, bool is_test);
  void toXML(xmlNodePtr node, XMLStyle xmlStyle) const;
};

struct TPNRs
{
  std::set<TFlightInfo> flights;
  std::map< int/*pnr_id*/, TPNRInfo > pnrs; //все PNR, которые подходят к критериям поиска

  bool add(const TFlightInfo &flt, const TPNRSegInfo &seg, const TPaxInfo &pax, bool is_test);
  const TFlightInfo& getFlightInfo(int point_dep) const;
  const TPNRInfo& getPNRInfo(int pnr_id) const;
  int calcStagePriority(int pnr_id) const;
  void toXML(xmlNodePtr node, bool is_primary, XMLStyle xmlStyle) const;
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

void findPNRs(const TPNRFilter &filter, TPNRs &PNRs, int pass, bool ignore_reg_no=false);

struct TPnrData
{
  TFlightInfo flt;
  TDestInfo dest;
  TPNRSegInfo seg;
};

void getTCkinData( const TPnrData &first,
                   bool is_test,
                   std::vector<TPnrData> &other);

} // namespace WebSearch


#endif // __WEB_SEARCH_H__

