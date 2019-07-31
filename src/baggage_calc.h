#ifndef _BAGGAGE_CALC_H_
#define _BAGGAGE_CALC_H_

#include <string>
#include <set>

#include "astra_consts.h"
#include "stl_utils.h"
#include "baggage_wt.h"
#include "events.h"
#include "passenger.h"
#include "astra_misc.h"

namespace WeightConcept
{

class TFltInfo
{
  public:
    int point_id;
    std::string airline_mark;
    int flt_no_mark;
    bool use_mark_flt;
    TFltInfo() { clear(); }
    void clear()
    {
      point_id=ASTRA::NoExists;
      airline_mark.clear();
      flt_no_mark=ASTRA::NoExists;
      use_mark_flt=false;
    }
    bool operator == (const TFltInfo &info) const
    {
      return point_id==info.point_id &&
          airline_mark==info.airline_mark &&
          flt_no_mark==info.flt_no_mark &&
          use_mark_flt==info.use_mark_flt;
    };
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "point_id=" << (point_id==ASTRA::NoExists?"NoExists":IntToString(point_id)) << ", "
        << "airline_mark=" << airline_mark << ", "
        << "flt_no_mark=" << (flt_no_mark==ASTRA::NoExists?"NoExists":IntToString(flt_no_mark)) << ", "
        << "use_mark_flt=" << (use_mark_flt?"true":"false");
      return s.str();
    }
};

class TNormFltInfo : public TFltInfo
{
  public:
    bool use_mixed_norms;
    TNormFltInfo() { clear(); }
    void clear()
    {
      use_mixed_norms=false;
    }
    bool operator == (const TNormFltInfo &info) const
    {
      return TFltInfo::operator ==(info) &&
          use_mixed_norms==info.use_mixed_norms;
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << TFltInfo::traceStr() << ", "
        << "use_mixed_norms=" << (use_mixed_norms?"true":"false");
      return s.str();
    }
};

class TPaxInfo
{
  public:
    std::set<std::string> pax_cats;
    std::string target;
    std::string final_target;
    std::string subcl;
    std::string cl;
    TPaxInfo() { clear(); }
    void clear()
    {
      pax_cats.clear();
      target.clear();
      final_target.clear();
      subcl.clear();
      cl.clear();
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "pax_cats=";
      for(std::set<std::string>::const_iterator i=pax_cats.begin(); i!=pax_cats.end(); ++i)
      {
        if (i!=pax_cats.begin()) s << "/";
        s << *i;
      };
      s << ", "
        << "target=" << target << ", "
        << "final_target=" << final_target << ", "
        << "subcl=" << subcl << ", "
        << "cl=" << cl;
      return s.str();
    }
};

class TBagNormInfo : public TPaxNormItem, public TNormItem
{
  public:
    static const int min_priority=100;
    TBagNormInfo():
      TPaxNormItem(),
      TNormItem() {}
    TBagNormInfo(const std::pair<TPaxNormItem, TNormItem> &norm):
      TPaxNormItem(norm.first),
      TNormItem(norm.second) {}
    void clear()
    {
      TPaxNormItem::clear();
      TNormItem::clear();
    }

    bool empty() const
    {
      return TPaxNormItem::empty() &&
             TNormItem::empty();
    }

    int priority() const;
};

class TAirlines : public std::set<std::string>
{
  public:
    TAirlines(const std::string &airline);
    TAirlines(int grp_id, const std::string &airline, const std::string& where);
    const std::string single() const;

};

void CheckOrGetPaxBagNorm(const TNormFltInfo &flt,
                          const TPaxInfo &pax,
                          const bool only_category,
                          const std::string &bag_type,
                          const TPaxNormItem &norm,
                          TBagNormInfo &result);

void CalcPaidBagView(const TAirlines &airlines,
                     const std::map<int/*id*/, TEventsBagItem> &bag,
                     const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                     const TPaidBagList &paid,
                     const CheckIn::TServicePaymentListWithAuto &payment,
                     const std::string &used_airline_mark,
                     TPaidBagViewMap &paid_view,
                     TPaidBagViewMap &trfer_view);

void PaidBagViewToXML(const TPaidBagViewMap &paid_view,
                      const TPaidBagViewMap &trfer_view,
                      xmlNodePtr node);

void RecalcPaidBagToDB(const TAirlines &airlines,
                       const std::map<int/*id*/, TEventsBagItem> &prior_bag, //TEventsBagItem а не CheckIn::TBagItem потому что есть refused
                       const std::map<int/*id*/, TEventsBagItem> &curr_bag,
                       const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                       const TNormFltInfo &flt,
                       const std::vector<CheckIn::TTransferItem> &trfer,
                       const CheckIn::TPaxGrpItem &grp,
                       const CheckIn::TPaxList &curr_paxs,
                       const TPaidBagList &prior_paid,
                       bool pr_unaccomp,
                       bool use_traces,
                       TPaidBagList &result_paid);

int test_norms(int argc,char **argv);

std::string GetBagRcptStr(int grp_id, int pax_id);
bool BagPaymentCompleted(int grp_id, int *value_bag_count=NULL);

std::string GetBagAirline(const TTripInfo &operFlt, const TTripInfo &markFlt, bool is_local_scd_out);

} //namespace WeightConcept

namespace PieceConcept
{

std::string GetBagRcptStr(int grp_id, int pax_id);

} //namespace PieceConcept

std::string GetBagRcptStr(const std::vector<std::string> &rcpts);

#endif

