#ifndef _BAGGAGE_CALC_H_
#define _BAGGAGE_CALC_H_

#include <string>

#include "passenger.h"
#include "baggage.h"
#include "events.h"
#include "astra_misc.h"

namespace BagPayment
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
      std::string pax_cat;
      std::string target;
      std::string final_target;
      std::string subcl;
      std::string cl;
    TPaxInfo() { clear(); }
    void clear()
    {
      pax_cat.clear();
      target.clear();
      final_target.clear();
      subcl.clear();
      cl.clear();
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "pax_cat=" << pax_cat << ", "
        << "target=" << target << ", "
        << "final_target=" << final_target << ", "
        << "subcl=" << subcl << ", "
        << "cl=" << cl;
      return s.str();
    }
  };

  class TBagNormInfo : public CheckIn::TPaxNormItem, public CheckIn::TNormItem
  {
    public:
      static const int min_priority=100;
      TBagNormInfo():
        CheckIn::TPaxNormItem(),
        CheckIn::TNormItem() {}
      TBagNormInfo(const std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> &norm):
        CheckIn::TPaxNormItem(norm.first),
        CheckIn::TNormItem(norm.second) {}
      void clear()
      {
        CheckIn::TPaxNormItem::clear();
        CheckIn::TNormItem::clear();
      }

      bool empty() const
      {
        return CheckIn::TPaxNormItem::empty() &&
               CheckIn::TNormItem::empty();
      }

      int priority() const;
  };

  void CheckOrGetPaxBagNorm(const TNormFltInfo &flt,
                            const TPaxInfo &pax,
                            const bool only_category,
                            const int bag_type,
                            const CheckIn::TPaxNormItem &norm,
                            TBagNormInfo &result);

  void PaidBagViewToXML(const std::map<int/*id*/, TBagToLogInfo> &bag,
                        const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                        const std::list<CheckIn::TPaidBagItem> &paid,
                        const std::list<CheckIn::TPaidBagEMDItem> &emd,
                        const std::string &used_airline_mark,
                        xmlNodePtr node);

  void RecalcPaidBag(const std::map<int/*id*/, TBagToLogInfo> &prior_bag, //TBagToLogInfo а не CheckIn::TBagItem потому что есть refused
                     const std::map<int/*id*/, TBagToLogInfo> &curr_bag,
                     const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                     const TNormFltInfo &flt,
                     const std::vector<CheckIn::TTransferItem> &trfer,
                     const CheckIn::TPaxGrpItem &grp,
                     const CheckIn::TPaxList &curr_paxs,
                     bool pr_unaccomp);

  void PaidBagViewToXMLTest(xmlNodePtr node);

}; //namespace BagPayment

#endif

