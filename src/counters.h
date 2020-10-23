#pragma once

#include "oralib.h"
#include "passenger.h"
#include "astra_misc.h"
#include "flt_settings.h"

namespace CheckIn
{

class AvailableByClassKey
{
  public:
    std::string cl;
    bool is_jmp;

  AvailableByClassKey(const std::string& _cl, bool _is_jmp) :
    cl(_cl), is_jmp(_is_jmp) {}

  bool operator < (const AvailableByClassKey &key) const
  {
    return (is_jmp?"":cl) < (key.is_jmp?"":key.cl);
  }
};

class AvailableByClass : public AvailableByClassKey
{
  public:
    int need;
    int avail;

  AvailableByClass(const AvailableByClassKey& key) :
    AvailableByClassKey(key), need(0), avail(ASTRA::NoExists) {}
};

class AvailableByClasses : public std::map<AvailableByClassKey, AvailableByClass>
{
  public:
    AvailableByClasses(const CheckIn::TPaxList& paxs)
    {
      for(const CheckIn::TPaxListItem& p : paxs)
        add(p.pax.cabin.cl, p.pax.is_jmp, p.pax.seats);
    }
    AvailableByClasses() {}

    void add(const std::string& cl, bool is_jmp, int seats)
    {
      AvailableByClassKey key(cl, is_jmp);
      emplace(key, key).first->second.need+=seats;
    }

    void getSummaryResult(int& need, int& avail) const;

    void dump() const;
};

void CheckCounters(const CheckIn::TPaxGrpItem& grp,
                   bool free_seating,
                   AvailableByClasses& availableByClasses);

void CheckCounters(int point_dep,
                   int point_arv,
                   ASTRA::TPaxStatus grp_status,
                   const TCFG &cfg,
                   bool free_seating,
                   AvailableByClasses& availableByClasses);

class TCrsCountersKey
{
  public:
    int point_dep;
    std::string airp_arv;
    std::string cl;

    TCrsCountersKey() { clear(); }

    void clear()
    {
      point_dep=ASTRA::NoExists;
      airp_arv.clear();
      cl.clear();
    }

    bool operator < (const TCrsCountersKey &key) const
    {
      if (point_dep!=key.point_dep)
        return point_dep<key.point_dep;
      if (airp_arv!=key.airp_arv)
        return airp_arv<key.airp_arv;
      return cl<key.cl;
    }

    bool operator == (const TCrsCountersKey &key) const
    {
      return point_dep==key.point_dep &&
             airp_arv==key.airp_arv &&
             cl==key.cl;
    }

    const TCrsCountersKey& toDB(TQuery &Qry) const;
    TCrsCountersKey& fromDB(TQuery &Qry);
};

class TCountersKey
{
  public:
    int point_dep;
    int point_arv;
    std::string cl;

    TCountersKey() { clear(); }

    void clear()
    {
      point_dep=ASTRA::NoExists;
      point_arv=ASTRA::NoExists;
      cl.clear();
    }

    bool operator < (const TCountersKey &key) const
    {
      if (point_dep!=key.point_dep)
        return point_dep<key.point_dep;
      if (point_arv!=key.point_arv)
        return point_arv<key.point_arv;
      return cl<key.cl;
    }

    const TCountersKey& toDB(TQuery &Qry) const;
    TCountersKey& fromDB(TQuery &Qry);
};

class TCrsCountersData
{
  public:
    int tranzit, ok;

    TCrsCountersData() { clear(); }

    void clear()
    {
      tranzit=0;
      ok=0;
    }

    bool operator == (const TCrsCountersData &data) const
    {
      return tranzit==data.tranzit &&
             ok==data.ok;
    }

    const TCrsCountersData& toDB(TQuery &Qry) const;
    TCrsCountersData& fromDB(TQuery &Qry);
};

class TRegCountersData
{
  public:
    int tranzit, ok, goshow;
    int jmp_tranzit, jmp_ok, jmp_goshow;

    TRegCountersData() { clear(); }

    void clear()
    {
      tranzit=0;
      ok=0;
      goshow=0;
      jmp_tranzit=0;
      jmp_ok=0;
      jmp_goshow=0;
    }

    bool operator == (const TRegCountersData &data) const
    {
      return tranzit==data.tranzit &&
             ok==data.ok &&
             goshow==data.goshow &&
             jmp_tranzit==data.jmp_tranzit &&
             jmp_ok==data.jmp_ok &&
             jmp_goshow==data.jmp_goshow;
    }

    bool isZero() const
    {
      return operator ==(TRegCountersData());
    }

    const TRegCountersData& toDB(TQuery &Qry) const;
    TRegCountersData& fromDB(TQuery &Qry);
};

class TCrsCountersMap : public std::map<TCrsCountersKey, TCrsCountersData>
{
  public:
    TCrsCountersMap(const TAdvTripInfo &flt) : _flt(flt) {}

    boost::optional<int> getMaxCrsPriority() const;

    void loadCrsDataOnly();
    void loadSummary();
    void loadCrsCountersOnly();
    void saveCrsCountersOnly() const;

    static void deleteCrsCountersOnly(int point_id);

  private:
    TAdvTripInfo _flt;
};

class TCrsFieldsMap : public std::map<TCountersKey, TCrsCountersData>
{
  public:
    void convertFrom(const TCrsCountersMap &src, const TAdvTripInfo &flt, const TTripRoute &routeAfter);
    void apply(const TAdvTripInfo &flt, const bool pr_tranz_reg) const;
};

class TRegDifferenceMap : public std::map<TCountersKey, TRegCountersData>
{
  public:
    static void trace(const CheckIn::TPaxGrpItem &grp,
                      const TSimplePaxList &prior_paxs,
                      const TSimplePaxList &curr_paxs);
    void getDifference(const CheckIn::TPaxGrpItem &grp,
                       const TSimplePaxList &prior_paxs,
                       const TSimplePaxList &curr_paxs);
    void apply(const TAdvTripInfo &flt, const bool pr_tranz_reg) const;
};

class TCounters
{
  public:
    enum RecountType { Total, CrsCounters };

  private:
    TAdvTripInfo _flt;
    boost::optional<TTripSetList> _fltSettings;
    boost::optional<TTripRoute> _fltRouteAfter;
    boost::optional<bool> _pr_tranz_reg;
    boost::optional<bool> _cfg_exists;

    void clear()
    {
      _flt.Clear();
      _fltSettings=boost::none;
      _fltRouteAfter=boost::none;
      _pr_tranz_reg=boost::none;
      _cfg_exists=boost::none;
    }

    bool setFlt(int point_id);
    const TAdvTripInfo& flt();
    const TTripSetList& fltSettings();
    const TTripRoute& fltRouteAfter();
    const bool& pr_tranz_reg();
    const bool& cfg_exists();

    void recountInitially();
    void recountFinally();
    void recountCrsFields();

    static void deleteInitially(int point_id);
    static void lockInitially(int point_id);

  public:
    const TCounters& recount(int point_id, RecountType type, const std::string& whence);
    const TCounters& recount(const CheckIn::TPaxGrpItem& grp,
                             const TSimplePaxList& prior_paxs,
                             const TSimplePaxList& curr_paxs,
                             const std::string& whence);
    static int totalRegisteredPassengers(int point_id);
};

} //namespace CheckIn

namespace Timing
{

class Point
{
  public:
    std::string what;
    boost::optional<int> seg_no;

    Point(const std::string& _what, const boost::optional<int>& _seg_no=boost::none) : what(_what), seg_no(_seg_no) {}

    bool operator < (const Point &point) const
    {
      if (what!=point.what)
        return what<point.what;
      if (seg_no && point.seg_no)
        return seg_no.get() < point.seg_no.get();
      else
        return seg_no<point.seg_no;
    }

};

class Intervals : public std::list< std::pair<boost::posix_time::ptime, boost::posix_time::ptime> > {};

class Points : public std::map<Point, Intervals>
{
    std::string _traceTitle;
  public:
    void start(const std::string& _what, const boost::optional<int>& _seg_no=boost::none);
    void finish(const std::string& _what, const boost::optional<int>& _seg_no=boost::none);
    Points(const std::string& traceTitle) : _traceTitle(traceTitle) {}
    ~Points();
};

} //namespace Timing



