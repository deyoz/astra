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
    TFltInfo(const int point_dep,
             const TGrpMktFlight& grpMktFlight) :
      point_id(point_dep),
      airline_mark(grpMktFlight.airline),
      flt_no_mark(grpMktFlight.flt_no),
      use_mark_flt(grpMktFlight.pr_mark_norms) {}

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
    TNormFltInfo(const TTripInfo& operFlight,
                 const TGrpMktFlight& grpMktFlight) :
      TFltInfo(operFlight.point_id, grpMktFlight)
    {
      use_mixed_norms=GetTripSets(tsMixedNorms, operFlight);
    }

    void clear()
    {
      TFltInfo::clear();
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
    CheckIn::TPaxTknItem tkn;
    TPaxInfo() { clear(); }
    void clear()
    {
      pax_cats.clear();
      target.clear();
      final_target.clear();
      subcl.clear();
      cl.clear();
      tkn.clear();
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
        << "cl=" << cl <<", "
        << "tkn=" << tkn.logStr();
      return s.str();
    }

    boost::optional<TNormItem> etickNormFromDB() const;
};

class TBagInfo : public CheckIn::TSimpleBagItem
{
  public:
    bool is_trfer;
    bool refused;

    TBagInfo(const CheckIn::TSimpleBagItem& bag,
             bool is_trfer_,
             bool refused_) :
      CheckIn::TSimpleBagItem(bag),
      is_trfer(is_trfer_),
      refused(refused_) {}
};

class TBagList : public std::list<TBagInfo>
{
  public:
    TBagList(const std::map<int/*id*/, TEventsBagItem>& bag)
    {
      for(const auto& b : bag)
        emplace_back(b.second, b.second.is_trfer, b.second.refused);
    }
};

class TAirlines : public std::set<std::string>
{
  public:
    TAirlines(const std::string &airline);
    TAirlines(int grp_id, const std::string &airline, const std::string& where);
    TAirlines(int grp_id,
              const TTripInfo& operFlight,
              const TGrpMktFlight& grpMktFlight,
              const std::string& where) :
      TAirlines(grp_id,
                grpMktFlight.pr_mark_norms?grpMktFlight.airline:operFlight.airline,
                where) {}
    const std::string single() const;
};

typedef std::list<TPaxNormComplex> AllPaxNormContainer;

void CalcPaidBagView(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                     const TPaidBagList &paid,
                     const CheckIn::TServicePaymentListWithAuto &payment,
                     const std::string &used_airline_mark,
                     TPaidBagViewMap &paid_view,
                     TPaidBagViewMap &trfer_view);

void PaidBagViewToXML(const TPaidBagViewMap &paid_view,
                      const TPaidBagViewMap &trfer_view,
                      xmlNodePtr node);

void RecalcPaidBagToDB(const TAirlines &airlines,
                       const TBagList &prior_bag,
                       const TBagList &curr_bag,
                       const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                       const TNormFltInfo &flt,
                       const CheckIn::TTransferList &trfer,
                       const CheckIn::TPaxGrpItem &grp,
                       const CheckIn::TPaxList &curr_paxs,
                       const TPaidBagList &prior_paid,
                       bool pr_unaccomp,
                       bool use_traces,
                       TPaidBagList &result_paid);

void RecalcPaidBag(const TTripInfo& flt,
                   const CheckIn::TSimplePaxGrpItem& grp,
                   const std::list< std::pair<int, CheckIn::TSimpleBagItem> >& additionalBaggage,
                   TPaidBagList& prior_paid,
                   TPaidBagList& result_paid);

int test_norms(int argc,char **argv);

std::string GetBagRcptStr(int grp_id, int pax_id);
bool BagPaymentCompleted(int grp_id, int *value_bag_count=NULL);

std::string GetBagAirline(const TTripInfo &operFlt, const TTripInfo &markFlt, bool is_local_scd_out);

boost::optional<TBagTotals> getBagAllowance(const CheckIn::TSimplePaxItem& pax);
boost::optional<TBagTotals> calcBagAllowance(const CheckIn::TSimplePaxItem& pax,
                                             const CheckIn::TSimplePaxGrpItem& grp,
                                             const TTripInfo& flt);

} //namespace WeightConcept

namespace PieceConcept
{

std::string GetBagRcptStr(int grp_id, int pax_id);

boost::optional<TBagTotals> getBagAllowance(const CheckIn::TSimplePaxItem& pax);

} //namespace PieceConcept

std::string GetBagRcptStr(const std::vector<std::string> &rcpts);

#endif

