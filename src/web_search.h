#ifndef __WEB_SEARCH_H__
#define __WEB_SEARCH_H__

#include "basic.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "stages.h"

namespace WebSearch
{

struct TPNRAddrInfo
{
  std::string airline, addr;

  void clear()
  {
    airline.clear();
    addr.clear();
  };

	bool operator == (const TPNRAddrInfo &item) const
  {
    return airline==item.airline &&
           convert_pnr_addr(addr, true)==convert_pnr_addr(item.addr, true);
  };
};

struct TTestPaxInfo
{
  std::string airline;
  std::string subcls;
	TPNRAddrInfo pnr_addr;
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
};

class TPNRFilter
{
  private:
    bool vtracing, vtracing_init;
    bool tracing();
  public:
    std::set<std::string> airlines;
    int flt_no;
    std::string suffix;
    std::vector< std::pair<BASIC::TDateTime, BASIC::TDateTime> > scd_out_local_ranges;
    std::vector< std::pair<BASIC::TDateTime, BASIC::TDateTime> > scd_out_utc_ranges;
    std::string surname, name, pnr_addr_normal, ticket_no, document;
    int reg_no;
    std::vector<TTestPaxInfo> test_paxs;
    //BCBP_M
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
      surname_equal_len=ASTRA::NoExists;
      name_equal_len=ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();

      vtracing=true;
      vtracing_init=false;
    };

    TPNRFilter& fromXML(xmlNodePtr node);
    TPNRFilter& fromBCBP_M(const std::string bcbp);
    TPNRFilter& testPaxFromDB();
    void trace( TRACE_SIGNATURE ) const;
    void traceToMonitor( TRACE_SIGNATURE, const char *format,  ...);
};

struct TDestInfo
{
  int point_arv;
  BASIC::TDateTime scd_in_local, est_in_local, act_in_local;
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
  void toXML(xmlNodePtr node, bool old_style=false) const;
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
  BASIC::TDateTime scd_out_local, est_out_local, act_out_local;
  std::string city_dep;
  int dep_utc_offset;
  
  //дополнительно
  std::set<TDestInfo> dests;
  
  std::vector<TTripInfo> mark;
  
  std::map<TStage, BASIC::TDateTime> stages;
  std::map<TStage_Type, TStage> stage_statuses;
  bool pr_paid_ckin;
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
    stages.clear();
    stage_statuses.clear();
    pr_paid_ckin=false;
  };

  bool operator < (const TFlightInfo &item) const
  {
    return point_dep < item.point_dep;
  };
    
  bool fromDB(TQuery &Qry);
  bool fromDB(int point_id, bool first_segment, bool pr_throw);
  bool fromDBadditional(bool first_segment, bool pr_throw);
  void add(const TDestInfo &dest);
  void toXML(xmlNodePtr node, bool old_style=false) const;
};

struct TPNRSegInfo
{
  int point_dep, point_arv, pnr_id;
  std::string cls;
	std::string subcls;
	std::vector<TPNRAddrInfo> pnr_addrs;
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
  bool filterFromDB(const std::vector<TPNRAddrInfo> &filter);
  bool fromTestPax(int point_id, const TTripRoute &route, const TTestPaxInfo &pax);
  void getMarkFlt(const TFlightInfo &flt, bool is_test, TTripInfo &mark) const;
  void toXML(xmlNodePtr node, bool old_style=false) const;
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
    
  bool filterFromDB(const TPNRFilter &filter, TQuery &Qry);
  bool fromTestPax(const TTestPaxInfo &pax);
  void toXML(xmlNodePtr node) const;
};

struct TPNRInfo
{
  std::map< int/*num*/, TPNRSegInfo > segs; //дополнительно, кроме первого сегмента
  std::set<TPaxInfo> paxs;
  //дополнительно
  int bag_norm;
  TPNRInfo():
    bag_norm(ASTRA::NoExists) {};
    
  void add(const TPaxInfo &pax);
  bool fromDBadditional(const TFlightInfo &flt, const TDestInfo &dest, bool is_test);
  void toXML(xmlNodePtr node, bool old_style=false) const;
};

struct TPNRs
{
  std::set<TFlightInfo> flights;
  std::map< int/*pnr_id*/, TPNRInfo > pnrs;
  
  bool add(const TFlightInfo &flt, const TPNRSegInfo &seg, const TPaxInfo &pax, bool is_test);
  void toXML(xmlNodePtr node) const;
};

void findPNRs(const TPNRFilter &filter, TPNRs &PNRs, int pass);

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

