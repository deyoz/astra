#include "baggage_calc.h"
#include "qrys.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "transfer.h"
#include "astra_misc.h"
#include "term_version.h"
#include "baggage_wt.h"
#include "obrnosir.h"
#include "cache_impl.h"
#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>
#include <boost/utility/in_place_factory.hpp>

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;

namespace WeightConcept
{

class BagNormInfo;

typedef list<BagNormInfo> BagNormList;

void loadBagNorms(const TFltInfo &flt,
                  BagNormList &bagNorms);

}

typedef ASTRA::Cache<int/*norm_id*/, WeightConcept::TNormItem> DirectActionNormCache;
typedef ASTRA::Cache<WeightConcept::TFltInfo, WeightConcept::BagNormList > FlightBagNormCache;

namespace ASTRA
{

template<> const WeightConcept::TNormItem& DirectActionNormCache::add(const int& normId) const
{
  WeightConcept::TNormItem norm;
  bool isDirectActionNorm;
  if (!norm.getByNormId(normId, isDirectActionNorm))
    throw NotFound();
  if (!isDirectActionNorm)
    throw NotFound();

  return items.emplace(normId, norm).first->second;
}

template<> std::string DirectActionNormCache::traceTitle()
{
  return "DirectActionNormCache";
}

template<> const WeightConcept::BagNormList& FlightBagNormCache::add(const WeightConcept::TFltInfo& flt) const
{
  items.clear();  //��� ᮤ�ন� ⮫쪮 ���� (��᫥���� �� �६���) �����

  WeightConcept::BagNormList& result=items.emplace(flt, WeightConcept::BagNormList()).first->second;
  WeightConcept::loadBagNorms(flt, result);

  return result;
}

template<> std::string FlightBagNormCache::traceTitle()
{
  return "FlightBagNormCache";
}

}

namespace WeightConcept
{

class Calculator
{
  private:
    mutable DirectActionNormCache directActionNorms;
    mutable FlightBagNormCache flightBagNorms;
    bool getPaxCrsOrEtickNorm(const TPaxInfo &pax,
                              const TBagTypeListKey &bagTypeKey,
                              const boost::optional<TPaxNormItem> &paxNorm,
                              const bool useEtickNormsSetting,
                              boost::optional<TPaxNormComplex> &result) const;
  public: //������, �᫨ ��७���� �� ���᫥��� � ��� �����
    void checkOrGetPaxBagNorm(const TNormFltInfo& flt,
                              const TPaxInfo &pax,
                              const bool only_category,
                              const TBagTypeListKey &bagTypeKey,
                              const boost::optional<TPaxNormItem> &paxNorm,
                              boost::optional<TPaxNormComplex> &result) const;
    void clearFlightBagNorms() const
    {
      flightBagNorms.clear();
    }
  public:
    static const Calculator& instance()
    {
      static Calculator calculator;
      return calculator;
    }
    void trace() const
    {
      LogTrace(TRACE5) << directActionNorms.traceTotals();
      LogTrace(TRACE5) << flightBagNorms.traceTotals();
    }
};

boost::optional<TBagQuantity> TPaxInfo::crsNormFromDB() const
{
  if (!paxId) return boost::none;

  return CheckIn::TSimplePaxItem::getCrsBagNorm(paxId.get());
}

boost::optional<TBagQuantity> TPaxInfo::etickNormFromDB() const
{
  if (!tkn.validET()) return boost::none;

  TETickItem etick;
  if (etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false).empty()) return boost::none;

  if (!etick.bagNorm) return TBagQuantity(0, TBagUnit());

  return etick.bagNorm;
}

class TBagNormFieldAmounts
{
  public:
    int city_arv, pax_cat, subcl, cl, flt_no, craft;
    TBagNormFieldAmounts() : city_arv(1),
                             pax_cat(100),
                             subcl(50),
                             cl(1),
                             flt_no(1),
                             craft(1) {}
};

class TBagNormFilterSets
{
  public:
    bool use_basic, is_trfer, only_category, check;
    TBagTypeListKey bagTypeKey;
    TBagNormFilterSets() : use_basic(false),
                           is_trfer(false),
                           only_category(false),
                           check(false) {}
};

class TNormWithPriorityInfo : public TNormItem
{
  public:
    TNormWithPriorityInfo(const TNormItem& norm) : TNormItem(norm) {}
    TNormWithPriorityInfo() {}
    virtual ~TNormWithPriorityInfo() {}
    virtual bool isRegularBagType() const = 0;

    int getPriority() const;
};

int TNormWithPriorityInfo::getPriority() const
{
  if (amount==NoExists &&
      weight==NoExists &&
      per_unit==NoExists)
  {
    switch(norm_type)
    {
          case bntFree: return 1;
      case bntOrdinary: if (!isRegularBagType()) return 10; else break;
          case bntPaid: return 19;
               default: break;
    }
  }

  if (amount==NoExists &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit!=0)
  {
    switch(norm_type)
    {
        case bntFreeExcess: return 3;
      case bntFreeOrdinary: if (!isRegularBagType()) return 2; else break;
          case bntFreePaid: return 11;
      case bntOrdinaryPaid: if (!isRegularBagType()) return 15; else break;
                   default: break;
    }
  }

  if (amount!=NoExists && amount>0 &&
      weight==NoExists &&
      per_unit==NoExists)
  {
    switch(norm_type)
    {
        case bntFreeExcess: return 5;
      case bntFreeOrdinary: if (!isRegularBagType()) return 4; else break;
          case bntFreePaid: return 12;
      case bntOrdinaryPaid: if (!isRegularBagType()) return 16; else break;
                   default: break;
    }
  }

  if (amount!=NoExists && amount>0 &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit!=0)
  {
    switch(norm_type)
    {
        case bntFreeExcess: return 7;
      case bntFreeOrdinary: if (!isRegularBagType()) return 6; else break;
          case bntFreePaid: return 13;
      case bntOrdinaryPaid: if (!isRegularBagType()) return 17; else break;
                   default: break;
    }
  }

  if (amount==NoExists &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit==0)
  {
    switch(norm_type)
    {
        case bntFreeExcess: return 9;
      case bntFreeOrdinary: if (!isRegularBagType()) return 8; else break;
          case bntFreePaid: return 14;
      case bntOrdinaryPaid: if (!isRegularBagType()) return 18; else break;
                   default: break;
    }
  }

  return NoExists;
}

class SuitableBagNormInfo;

class BagNormInfo : public TNormWithPriorityInfo
{
  public:
    std::string bag_type222;
    int norm_id;
    std::string airline, city_arv, pax_cat, subcl, cl, craft;
    int pr_trfer, flt_no;
    BagNormInfo()
    {
      clear();
    }

    void clear()
    {
      TNormWithPriorityInfo::clear();
      bag_type222.clear();
      norm_id=ASTRA::NoExists;
      airline.clear();
      city_arv.clear();
      pax_cat.clear();
      subcl.clear();
      cl.clear();
      craft.clear();
      pr_trfer=ASTRA::NoExists;
      flt_no=ASTRA::NoExists;
    }

    BagNormInfo& fromDB(DB::TQuery &Qry);
    boost::optional<SuitableBagNormInfo> getSuitableBagNorm(const TPaxInfo &pax,
                                                            const TBagNormFilterSets &filter,
                                                            const TBagNormFieldAmounts &field_amounts) const;
    static void addDirectActionNorm(const TNormItem& norm);
    static boost::optional<BagNormInfo> getDirectActionNorm(const TNormItem& norm);

    static const set<string>& strictly_limited_cats()
    {
      static set<string> cats = {"CHC", "CHD", "INA", "INF"};
      return cats;
    }

    bool isRegularBagType() const { return bag_type222==REGULAR_BAG_TYPE; }
};

BagNormInfo& BagNormInfo::fromDB(DB::TQuery &Qry)
{
  clear();
  TNormWithPriorityInfo::fromDB(Qry);
  bag_type222=Qry.FieldAsString("bag_type");
  norm_id=Qry.FieldAsInteger("id");
  airline=Qry.FieldAsString("airline");
  city_arv=Qry.FieldAsString("city_arv");
  pax_cat=Qry.FieldAsString("pax_cat");
  subcl=Qry.FieldAsString("subclass");
  cl=Qry.FieldAsString("class");
  craft=Qry.FieldAsString("craft");
  if (!Qry.FieldIsNULL("pr_trfer"))
    pr_trfer=Qry.FieldAsInteger("pr_trfer")!=0;
  if (!Qry.FieldIsNULL("flt_no"))
    flt_no=Qry.FieldAsInteger("flt_no");
  return *this;
}


void BagNormInfo::addDirectActionNorm(const TNormItem& norm)
{
  static TDateTime first_date=NoExists;
  if (first_date==NoExists) StrToDateTime("01.01.2020", "dd.mm.yyyy", first_date);

  DB::TCachedQuery Qry(
        PgOra::getRWSession("BAG_NORMS"),
        "INSERT INTO bag_norms("
        "id, first_date, norm_type, amount, weight, per_unit, pr_del, direct_action, tid "
        ") VALUES("
        ":id, :first_date, :norm_type, :amount, :weight, :per_unit, 0, 1, :tid)",
        QParams() << QParam("id", otInteger, PgOra::getSeqCurrVal_int("ID__SEQ"))
                  << QParam("tid", otInteger, PgOra::getSeqNextVal_int("TID__SEQ"))
                  << QParam("first_date", otDate, first_date)
                  << QParam("norm_type", otString)
                  << QParam("amount", otInteger)
                  << QParam("weight", otInteger)
                  << QParam("per_unit", otInteger),
        STDLOG);
  norm.toDB(Qry.get());
  Qry.get().Execute();
}

boost::optional<BagNormInfo> BagNormInfo::getDirectActionNorm(const TNormItem& norm)
{
  if (norm.weight==NoExists)
    throw Exception("%s: norm.weight==NoExists", __func__);

  DB::TCachedQuery Qry(
        PgOra::getROSession("BAG_NORMS"),
        "SELECT * FROM bag_norms "
        "WHERE direct_action=:direct_action AND "
        "      norm_type=:norm_type AND "
        "      weight=:weight AND "
        "      (amount IS NULL AND :amount IS NULL OR amount=:amount) AND "
        "      (per_unit IS NULL AND :per_unit IS NULL OR per_unit=:per_unit) "
        "ORDER BY id",
        QParams() << QParam("direct_action", otInteger, (int)true)
                  << QParam("norm_type", otString)
                  << QParam("amount", otInteger)
                  << QParam("weight", otInteger)
                  << QParam("per_unit", otInteger),
        STDLOG);
  norm.toDB(Qry.get());
  Qry.get().Execute();
  if (Qry.get().Eof) return boost::none;

  return BagNormInfo().fromDB(Qry.get());
}

class SuitableBagNormInfo : public BagNormInfo
{
  public:
    int similarity_cost, priority;

    SuitableBagNormInfo(const BagNormInfo& norm_,
                        const int similarity_cost_) :
      BagNormInfo(norm_),
      similarity_cost(similarity_cost_),
      priority(norm_.getPriority()) {}

    bool operator < (const SuitableBagNormInfo &norm) const
    {
      if (similarity_cost!=norm.similarity_cost)
        return similarity_cost<norm.similarity_cost;
      if (priority!=norm.priority)
        return priority>norm.priority;
      return (amount!=NoExists && amount>=0?amount:1)*(weight!=NoExists && weight>=0?weight:1)<
             (norm.amount!=NoExists && norm.amount>=0?norm.amount:1)*(norm.weight!=NoExists && norm.weight>=0?norm.weight:1);
    }
};

boost::optional<SuitableBagNormInfo> BagNormInfo::getSuitableBagNorm(const TPaxInfo &pax, const TBagNormFilterSets &filter, const TBagNormFieldAmounts &field_amounts) const
{
  int cost=0;
  if (filter.bagTypeKey.bag_type!=bag_type222) return boost::none;

  if ((filter.use_basic && !airline.empty()) ||
      (!filter.use_basic && airline.empty())) return boost::none;

  if ((filter.is_trfer?pax.final_target:pax.target).empty()) return boost::none;
  if (pr_trfer!=NoExists && (filter.is_trfer?pr_trfer==0:pr_trfer!=0)) return boost::none;
  if (!city_arv.empty())
  {
    if ((filter.is_trfer?pax.final_target:pax.target)!=city_arv) return boost::none;
    cost+=field_amounts.city_arv;
  };

  if (filter.only_category && pax.pax_cats.find(pax_cat)==pax.pax_cats.end()) return boost::none;
  if (!pax_cat.empty() &&
      (!filter.check || !pax.pax_cats.empty() || strictly_limited_cats().find(pax_cat)!=strictly_limited_cats().end()))
  {
    if (pax.pax_cats.find(pax_cat)==pax.pax_cats.end()) return boost::none;
    cost+=field_amounts.pax_cat;
  };
  if (!subcl.empty())
  {
    if (pax.subcl!=subcl) return boost::none;
    cost+=field_amounts.subcl;
  };
  if (!cl.empty())
  {
    if (pax.cl!=cl) return boost::none;
    cost+=field_amounts.cl;
  };
  if (flt_no!=NoExists)
    cost+=field_amounts.flt_no;
  if (!craft.empty())
    cost+=field_amounts.craft;

  SuitableBagNormInfo result(*this, cost);
  if (result.priority==NoExists) return boost::none; //��ଠ ������� ���ࠢ��쭮

  return result;
}

void loadBagNorms(const TFltInfo &flt,
                  BagNormList &bagNorms)
{
  bagNorms.clear();

  CacheTable::TripBagNorms tripBagNorms(false);

  DB::TQuery Qry(PgOra::getROSession("BAG_NORMS"), STDLOG);

  if (!tripBagNorms.prepareSelectQuery(PointId_t(flt.point_id),
                                       flt.use_mark_flt,
                                       AirlineCode_t(flt.airline_mark),
                                       FlightNumber_t(flt.flt_no_mark),
                                       Qry)) return;

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    bagNorms.push_back(BagNormInfo().fromDB(Qry));
}

//१���� �㭪樨 ����砥� ���� �� ����� ������� result �⠭����� ��堭����� �।� flightBagNorms
//true: result ������� - � flightBagNorms �� �����
bool Calculator::getPaxCrsOrEtickNorm(const TPaxInfo &pax,
                                      const TBagTypeListKey &bagTypeKey,
                                      const boost::optional<TPaxNormItem> &paxNorm,
                                      const bool useEtickNormsSetting,
                                      boost::optional<TPaxNormComplex> &result) const
{
  result=boost::none;

  if (!bagTypeKey.isRegular()) return false;

  boost::optional<TNormItem> crsOrEtickNorm=TNormItem::create(pax.crsNormFromDB());
  if (!crsOrEtickNorm && useEtickNormsSetting)
    crsOrEtickNorm=TNormItem::create(pax.etickNormFromDB());
  if (!crsOrEtickNorm) return false;

  if (paxNorm)
  {
    if (!(paxNorm.get().key()==bagTypeKey))
      throw Exception("%s: paxNorm.get().key()!=bagTypeKey", __func__);

    if (paxNorm.get().handmade && paxNorm.get().handmade.get()) return false; //��ଠ ������� ������, �஢��塞 �⠭����� ��堭����� �।� flightBagNorms

    try
    {
      //��ଠ �뫠 ����⠭� ��⮬���᪨
      TNormItem norm;
      if (!paxNorm.get().normNotExists())
        norm=directActionNorms.get(paxNorm.get().norm_id);
      if (crsOrEtickNorm.get()==norm)
        result=boost::in_place(paxNorm.get(), norm);
    }
    catch(const DirectActionNormCache::NotFound&) {}

    return true;
  }

  if (!crsOrEtickNorm.get().isUnknown())
  {
    boost::optional<BagNormInfo> bagNorm;
    bagNorm=BagNormInfo::getDirectActionNorm(crsOrEtickNorm.get());
    if (!bagNorm)
    {
      BagNormInfo::addDirectActionNorm(crsOrEtickNorm.get());
      bagNorm=BagNormInfo::getDirectActionNorm(crsOrEtickNorm.get());
    }

    if (bagNorm)
    {
      TPaxNormItem newPaxNorm(bagTypeKey, bagNorm.get().norm_id, false);
      result=boost::in_place(newPaxNorm, bagNorm.get());
    }
  }

  return true;
}

void Calculator::checkOrGetPaxBagNorm(const TNormFltInfo& flt,
                                      const TPaxInfo &pax,
                                      const bool only_category,
                                      const TBagTypeListKey &bagTypeKey,
                                      const boost::optional<TPaxNormItem> &paxNorm,
                                      boost::optional<TPaxNormComplex> &result) const
{
  result=boost::none;

  if (getPaxCrsOrEtickNorm(pax, bagTypeKey, paxNorm, flt.use_etick_norms, result)) return;

  if (paxNorm)
  {
    if (!(paxNorm.get().key()==bagTypeKey))
      throw Exception("%s: paxNorm.get().key()!=bagTypeKey", __func__);
    if (paxNorm.get().isManuallyDeleted())
    {
      result=boost::in_place(paxNorm.get(), TNormItem());
      return;
    }
  }

  const BagNormList& bagNorms = flightBagNorms.get(flt);

  TBagNormFieldAmounts field_amounts;
  TBagNormFilterSets filter;
  filter.use_basic=algo::none_of(bagNorms, [](const BagNormInfo& n) { return !n.airline.empty(); });
  filter.only_category=only_category;

  boost::optional<SuitableBagNormInfo> curr, max;
  filter.bagTypeKey=bagTypeKey;
  filter.is_trfer=!pax.final_target.empty();
  for(;;filter.is_trfer=false)
  {
    for(const BagNormInfo& norm : bagNorms)
    {
      filter.check=(paxNorm &&
                    paxNorm.get().bag_type==norm.bag_type222 &&
                    paxNorm.get().norm_id==norm.norm_id &&
                    paxNorm.get().norm_trfer==filter.is_trfer);

      curr=norm.getSuitableBagNorm(pax, filter, field_amounts);
      if (!curr) continue;

      if (filter.check)
      {
        //��諨 ���� � ��� �������
        max=curr;
        break;
      }

      if (!max || max.get()<curr.get()) max=curr;
    }
    if (!filter.is_trfer) break;
    if (max || !flt.use_mixed_norms) break; //��室�� �᫨ �� ���楯��� �࠭����� ����
    //��� �� �� �� ���楯���, �� ����饭� �ᯮ�짮���� � ��砥 ��ଫ���� �࠭��� ���࠭���� ����
  };

  if (max)
  {
    TPaxNormItem newPaxNorm(bagTypeKey, max.get().norm_id, filter.is_trfer);
    if (paxNorm)
    {
      if (newPaxNorm.equal(paxNorm.get()))
        result=boost::in_place(paxNorm.get(), max.get()); //�� �⮡� ��࠭��� �ਧ��� handmade �� paxNorm
    }
    else
      result=boost::in_place(newPaxNorm, max.get());
  };
}

enum TNormsTrferType { nttNone, nttNotTransfer, nttTransfer, nttMixed };

class TNormWideItem : public TNormWithPriorityInfo
{
  public:
    bool isRegular;
    int priority;
    TNormWideItem(const TPaxNormComplex &norm):
      TNormWithPriorityInfo(norm),
      isRegular(norm.bag_type==REGULAR_BAG_TYPE),
      priority(ASTRA::NoExists)
    {
      priority=getPriority();
    }
    bool operator < (const TNormWideItem &item) const
    {
      if (priority!=item.priority)
        return priority>item.priority;
      return weight>item.weight;
    }
    bool tryAddNorm(const TNormWideItem &norm);
    bool isRegularBagType() const { return isRegular; }
};

bool TNormWideItem::tryAddNorm(const TNormWideItem &norm)
{
  if (priority==NoExists ||
      norm.priority==NoExists ||
      priority!=norm.priority) return false;

  if (amount!=NoExists && amount>0 &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit!=0 &&
      weight==norm.weight)
  {
    amount+=norm.amount;
    return true;
  };

  if (amount!=NoExists && amount>0 &&
      weight==NoExists &&
      per_unit==NoExists)
  {
    amount+=norm.amount;
    return true;
  };

  if (amount==NoExists &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit!=0)
  {
    if (weight<norm.weight) weight=norm.weight;
    return true;
  };

  if (amount==NoExists &&
      weight!=NoExists && weight>0 &&
      per_unit!=NoExists && per_unit==0)
  {
    weight+=norm.weight;
    return true;
  };

  if (amount==NoExists &&
      weight==NoExists &&
      per_unit==NoExists)
  {
    return true;
  };
  return false;
};

class TSimpleBagItem : public TBagTypeListKey
{
  public:
    int amount, weight;
    TSimpleBagItem(const TBagTypeListKey& _key):
      TBagTypeListKey(_key),
      amount(0),
      weight(0) {}
    TSimpleBagItem(const TBagTypeListKey& _key,
                   const int _amount,
                   const int _weight):
      TBagTypeListKey(_key),
      amount(_amount),
      weight(_weight) {}
    bool operator < (const TSimpleBagItem &item) const
    {
      if (weight!=item.weight)
        return weight>item.weight;
      return amount<item.amount;
    }
    bool operator == (const TSimpleBagItem &item) const
    {
      return amount==item.amount &&
             weight==item.weight;
    }
};

class TPaidBagWideItem : public TPaidBagItem
{
  public:
    int bag_amount, bag_weight;
    std::list<TNormWideItem> norms;
    TNormsTrferType norms_trfer;
    int weight_calc;
    TPaidBagWideItem() { clear(); }
    TPaidBagWideItem(const TBagTypeListKey& _key)
    {
      clear();
      key(_key);
    }
    void clear()
    {
      bag_amount=0;
      bag_weight=0;
      norms.clear();
      norms_trfer=nttNone;
      weight_calc=0;
    }
    bool addNorm(const TPaxNormComplex &norm);
    std::string total_view() const;
    std::string paid_calc_view() const;
    std::string norms_view(const bool is_trfer,
                           const std::string& airline_mark,
                           const std::string& lang="") const;
    std::string norms_trfer_view(const std::string& lang="") const;

    bool isRegularBagType() const { return bag_type==REGULAR_BAG_TYPE; }
};

bool TPaidBagWideItem::addNorm(const TPaxNormComplex &norm)
{
  if (!(key()==norm.key())) return false;
  if (norm.normNotExists()) return false;
  TNormWideItem new_norm(norm);
  if (new_norm.priority==NoExists) return false;
  //��⠥� norms_trfer
  switch(norms_trfer)
  {
           case nttNone: norms_trfer=(norm.norm_trfer?nttTransfer:nttNotTransfer); break;
    case nttNotTransfer: norms_trfer=(norm.norm_trfer?nttMixed:nttNotTransfer); break;
       case nttTransfer: norms_trfer=(norm.norm_trfer?nttTransfer:nttMixed); break;
          case nttMixed: break;
  }
  //�饬 ������� � norms
  list< TNormWideItem >::iterator n=norms.begin();
  for(; n!=norms.end(); ++n)
    if (n->tryAddNorm(new_norm)) break;
  if (n==norms.end())
    //�� ��諨 � 祬 �㬬�஢��� ���� - ������塞 � �����
    norms.push_back(new_norm);

  return true;
};

std::string TPaidBagWideItem::total_view() const
{
  ostringstream s;
  if (bag_amount!=0 || bag_weight!=0)
    s << bag_amount << "/" << bag_weight;
  else
    s << "-";
  return s.str();
}

std::string TPaidBagWideItem::paid_calc_view() const
{
  ostringstream s;
  if (weight_calc!=NoExists)
    s << weight_calc;
  else
    s << "?";
  return s.str();
}

std::string TPaidBagWideItem::norms_view(const bool is_trfer,
                                         const std::string& airline_mark,
                                         const std::string& lang) const
{
  ostringstream s;
  if (!is_trfer)
  {
    if (!norms.empty())
    {
      if (!airline_mark.empty())
        s << ElemIdToPrefferedElem(etAirline, airline_mark, efmtCodeNative,
                                   lang.empty()?TReqInfo::Instance()->desk.lang:lang)
          << ":";
      if (norms.size()==1)
        s << lowerc(norms.front().str(lang.empty()?TReqInfo::Instance()->desk.lang:lang));
      else
        s << getLocaleText("�. ���஡��", lang);
    }
    else s << "-";
  }
  else s << getLocaleText("�࠭���", lang);
  return s.str();
}

std::string TPaidBagWideItem::norms_trfer_view(const std::string& lang) const
{
  ostringstream s;
  switch(norms_trfer)
  {
    case nttNone: break;
    case nttNotTransfer: s << getLocaleText("���", lang); break;
    case nttTransfer: s << getLocaleText("��", lang); break;
    case nttMixed: s << getLocaleText("����", lang); break;
  };
  return s.str();
}

class TPaidBagCalcItem : public TPaidBagWideItem
{
  public:
    list<TSimpleBagItem> bag;
    bool norm_per_unit, norm_ordinary;
    TPaidBagCalcItem(const TBagTypeListKey& _key,
                     const list<TSimpleBagItem> &_bag) :
      TPaidBagWideItem(_key),
      bag(_bag)
    {
      for(list<TSimpleBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
      {
        bag_amount+=b->amount;
        bag_weight+=b->weight;
      };
      norm_per_unit=false;
      norm_ordinary=!isRegular();
    }
    bool addNorm(const TPaxNormComplex &norm);
};

bool TPaidBagCalcItem::addNorm(const TPaxNormComplex &norm)
{
  bool first_addition=norms.empty();

  if (!TPaidBagWideItem::addNorm(norm)) return false;

  if ((norm.amount!=NoExists && norm.amount>0 &&
       norm.weight!=NoExists && norm.weight>0 &&
       norm.per_unit!=NoExists && norm.per_unit!=0)||
      (norm.amount!=NoExists && norm.amount>0 &&
       norm.weight==NoExists &&
       norm.per_unit==NoExists)||
      (norm.amount==NoExists &&
       norm.weight!=NoExists && norm.weight>0 &&
       norm.per_unit!=NoExists && norm.per_unit!=0))
    norm_per_unit=true;
  else
    if (first_addition) norm_per_unit=false;

  if (norm.norm_type==bntOrdinary ||
      norm.norm_type==bntFreeOrdinary ||
      norm.norm_type==bntOrdinaryPaid)
    norm_ordinary=true;
  else
    if (first_addition) norm_ordinary=false;

  return true;
}

typedef std::map< TBagTypeListKey, std::list<TSimpleBagItem> > TSimpleBagMap;
typedef std::map< TBagTypeListKey, TPaxNormItem > TPaxNormMap;
typedef std::map< TBagTypeListKey, TPaidBagItem > TPaidBagMap;
typedef std::map< TBagTypeListKey, TPaidBagWideItem > TPaidBagWideMap;
typedef std::map< TBagTypeListKey, TPaidBagCalcItem > TPaidBagCalcMap;

//�������� ���� ������ - �� ��室� TPaidBagCalcItem - ᠬ�� ������ ��楤��
void CalcPaidBagBase(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     TPaidBagCalcMap &paid);

enum TPrepareSimpleBagType1 { psbtOnlyTrfer, psbtOnlyNotTrfer, psbtTrferAndNotTrfer };
enum TPrepareSimpleBagType2 { psbtOnlyRefused, psbtOnlyNotRefused, psbtRefusedAndNotRefused };

void PrepareSimpleBag(const TBagList &bag,
                      const TPrepareSimpleBagType1 trfer_type,
                      const TPrepareSimpleBagType2 refused_type,
                      TSimpleBagMap &bag_simple)
{
  bag_simple.clear();
  for(const TBagInfo& b : bag)
  {
    if ((refused_type==psbtOnlyRefused && !b.refused) ||
        (refused_type==psbtOnlyNotRefused && b.refused)) continue;
    if ((trfer_type==psbtOnlyTrfer && !b.is_trfer) ||
        (trfer_type==psbtOnlyNotTrfer && b.is_trfer)) continue;
    if (!b.wt) continue;
    const TBagTypeListKey& key=b.wt.get().key();
    TSimpleBagMap::iterator i=bag_simple.find(key);
    if (i==bag_simple.end())
      i=bag_simple.insert(make_pair(key, std::list<TSimpleBagItem>())).first;
    if (i==bag_simple.end()) throw Exception("%s: i==bag_simple.end()", __FUNCTION__);
    i->second.push_back(TSimpleBagItem(key, b.amount, b.weight));
  };
};

void TracePaidBagWide(const TPaidBagWideMap &paid, const string &where)
{
  for(TPaidBagWideMap::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    const TPaidBagWideItem &item=i->second;

    ProgTrace(TRACE5, "%s: %s", where.c_str(), item.traceStr().c_str());
    ProgTrace(TRACE5, "%s: bag_amount=%d bag_weight=%d", where.c_str(), item.bag_amount, item.bag_weight);

    ostringstream s;
    for(list<TNormWideItem>::const_iterator n=item.norms.begin(); n!=item.norms.end(); ++n)
    {
      if (n!=item.norms.begin()) s << "; ";
      s << n->str(AstraLocale::LANG_EN);
    }
    ProgTrace(TRACE5, "%s: norms: %s", where.c_str(), s.str().c_str());

    s.str("");
    switch(item.norms_trfer)
    {
      case nttNone: s << "nttNone"; break;
      case nttNotTransfer: s << "nttNotTransfer"; break;
      case nttTransfer: s << "nttTransfer"; break;
      case nttMixed: s << "nttMixed"; break;
    };
    ProgTrace(TRACE5, "%s: norms_trfer=%s", where.c_str(), s.str().c_str());

    s.str("");
    s << "weight_calc=" << (item.weight_calc==NoExists?"NoExists":IntToString(item.weight_calc)) << " "
      << "weight=" << (item.weight==NoExists?"NoExists":IntToString(item.weight)) << " "
      << "handmade=" << (!item.handmade?"NoExists":IntToString(item.handmade.get()));
    ProgTrace(TRACE5, "%s: %s", where.c_str(), s.str().c_str());
  }
}

std::ostream& operator<<(std::ostream& os, const TPaxNormMap& norms)
{
  std::for_each(norms.cbegin(), norms.cend(), [&os](const auto& n) { os << n.second << "; "; });

  return os;
}

std::ostream& operator<<(std::ostream& os, const TPaidBagMap &paid)
{
  std::for_each(paid.cbegin(), paid.cend(), [&os](const auto& p) { os << p.second << "; "; });

  return os;
}

std::ostream& operator<<(std::ostream& os, const AllPaxNormContainer &norms)
{
  for(const TPaxNormComplex& n : norms)
  {
    if (n.normNotExists()) continue;
    os << n.normStr(LANG_EN) << "; ";
  }

  return os;
}

class TWidePaxInfo : public TPaxInfo
{
  public:
    bool refused;
    TPaxNormMap prior_norms;
    TPaxNormMap curr_norms;
    TPaxNormMap result_norms;

    TWidePaxInfo() { clear(); }
    void clear()
    {
      TPaxInfo::clear();
      refused=false;
      prior_norms.clear();
      curr_norms.clear();
      result_norms.clear();
    }

    static const set<string>& only_category_cats()
    {
      static boost::optional< set<string> > cats;
      if (!cats)
      {
        cats=set<string>();
        cats.get().insert("CHC");
        cats.get().insert("INA");
        cats.get().insert("INF");
      };
      return cats.get();
    }

    bool only_category() const
    {
      for(set<string>::const_iterator i=pax_cats.begin(); i!=pax_cats.end(); ++i)
        if (only_category_cats().find(*i)!=only_category_cats().end()) return true;
      return false;
    }

    void setCategory(bool new_checkin,
                     const CheckIn::TPaxListItem &curr_pax,
                     const TPaxToLogInfo &prior_pax)
    {
      pax_cats.clear();
      const TPerson &pers_type=(new_checkin || curr_pax.pax.PaxUpdatesPending)?curr_pax.pax.pers_type:
                                                                               DecodePerson(prior_pax.pers_type.c_str());
      const int     &seats    =(new_checkin )?curr_pax.pax.seats:
                                              prior_pax.seats;
      if (pers_type==ASTRA::child)
      {
        if (seats==0) pax_cats.insert("CHC");
        else
        {
          const multiset<CheckIn::TPaxRemItem> &rems=
            (new_checkin || (curr_pax.pax.PaxUpdatesPending && curr_pax.remsExists))?curr_pax.rems:
                                                                                     prior_pax.rems;

          std::multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();
          for(; r!=rems.end(); ++r)
            if (r->code=="UMNR") break;
          if (r==rems.end()) pax_cats.insert("CHD");
        };
      };
      if (pers_type==ASTRA::baby && seats==0)
      {
        pax_cats.insert("INA");
        pax_cats.insert("INF");
      };
    }

    std::string traceStr() const
    {
      std::ostringstream s;
      s << std::boolalpha <<
           TPaxInfo::traceStr() << ", "
           "refused=" << refused << ", "
           "only_category=" << only_category();
      s << endl
        << "prior_norms: " << prior_norms << endl
        << "curr_norms: " << curr_norms << endl
        << "result_norms: " << result_norms;
      return s.str();
    }
};

TAirlines::TAirlines(const std::string& airline)
{
  insert(airline);
}

TAirlines::TAirlines(int grp_id, const std::string &airline, const std::string& where)
{
  TBagTypeKey key;
  key.bag_type=REGULAR_BAG_TYPE;
  try
  {
    key.getListKeyByGrpId(grp_id, 0, boost::none, where);
    insert(key.airline);
  }
  catch(const EConvertError&)
  {
    insert(airline);
  };
}

const std::string TAirlines::single() const
{
  if (size()!=1)
    throw Exception("TAirlines::single: size=%zu", size());
  return *begin();
}

void convertNormsList(const TAirlines &airlines,
                      const TBagList &bag,
                      const std::list<TPaxNormItem> &norms,
                      const bool isCurrent,
                      TPaxNormMap &result)
{
  result.clear();

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //�ᥣ�� ������塞 ����� �����
  not_trfer_bag_simple.emplace(RegularBagType(airlines.single()), std::list<TSimpleBagItem>());

  for(const TPaxNormItem& n : norms)
  {
    if (isCurrent &&
        not_trfer_bag_simple.find(n)==not_trfer_bag_simple.end()) continue; //��䨫��஢뢠�� ����

    if (!result.emplace(n, n).second)
      throw Exception("%s: %s already exists in result!", __FUNCTION__, n.key().str(LANG_EN).c_str());
  };
}

void convertCurrentNormsList(const TAirlines &airlines,
                             const TBagList &bag,
                             const boost::optional< std::list<TPaxNormItem> > &norms,
                             TPaxNormMap &result)
{
  result.clear();
  if (!norms) return;

  convertNormsList(airlines, bag, norms.get(), true, result);
}

void convertPriorNormsList(const TAirlines &airlines,
                           const TBagList &bag,
                           const WeightConcept::TPaxNormComplexContainer &norms,
                           TPaxNormMap &result)
{

  const std::list<TPaxNormItem> tmp_norms=algo::transform<std::list>(norms,
                                                                     [](const TPaxNormItem& n) { return n; });
  convertNormsList(airlines, bag, tmp_norms, false, result);
}

void SyncHandmadeProp(const TPaxNormMap &src,
                      bool nvl,
                      TPaxNormMap &dest)
{
  for(TPaxNormMap::iterator i=dest.begin(); i!=dest.end(); ++i)
  {
    if (i->second.handmade) continue;
    TPaxNormMap::const_iterator j=src.find(i->first);
    if (j!=src.end() && i->second.equal(j->second))
      i->second.handmade=j->second.handmade;
    else
      i->second.handmade=nvl;
  }
}

//������� ������ - �� ��室� TPaidBagWideItem
//�ᯮ������ � RecalcPaidBagToDB
void RecalcPaidBagWide(const TAirlines &airlines,
                       const TBagList &prior_bag,
                       const TBagList &curr_bag,
                       const AllPaxNormContainer &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                       const TPaidBagList &prior_paid,
                       const TPaidBagList &curr_paid,
                       TPaidBagWideMap &paid_wide,
                       bool testing=false)
{
  paid_wide.clear();

  TSimpleBagMap prior_bag_simple;
  PrepareSimpleBag(prior_bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, prior_bag_simple);
  TSimpleBagMap curr_bag_simple;
  PrepareSimpleBag(curr_bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, curr_bag_simple);

  TPaidBagCalcMap paid_calc;
  CalcPaidBagBase(airlines, curr_bag, norms, paid_calc);

  TPaidBagCalcMap::iterator iOrdinary=paid_calc.find(RegularBagType(airlines.single()));
  if (iOrdinary==paid_calc.end()) throw Exception("%s: iOrdinary==paid_calc.end()", __FUNCTION__);
  TPaidBagCalcItem &itemOrdinary=iOrdinary->second;
  for(int pass=0; pass<2; pass++)
  {
    for(TPaidBagCalcMap::iterator i=paid_calc.begin(); i!=paid_calc.end(); ++i)
    {
      TPaidBagCalcItem &item=i->second;
      if ((pass==0 && item.isRegular()) ||
          (pass!=0 && !item.isRegular())) continue; //����� ����� �ᥣ�� � ��᫥���� ���樨

      multiset<TSimpleBagItem> prior_bag_sorted;
      TSimpleBagMap::const_iterator iPriorBag=prior_bag_simple.find(item);
      if (iPriorBag!=prior_bag_simple.end()) prior_bag_sorted.insert(iPriorBag->second.begin(), iPriorBag->second.end());

      multiset<TSimpleBagItem> curr_bag_sorted;
      TSimpleBagMap::const_iterator iCurrBag=curr_bag_simple.find(item);
      if (iCurrBag!=curr_bag_simple.end()) curr_bag_sorted.insert(iCurrBag->second.begin(), iCurrBag->second.end());

      //�������� ���ଠ樥� � ����
      for(TPaidBagList::const_iterator j=prior_paid.begin(); j!=prior_paid.end(); ++j)
        if (j->key()==item.key())
        {
          item.rate_id=j->rate_id;
          item.rate=j->rate;
          item.rate_cur=j->rate_cur;
          item.rate_trfer=j->rate_trfer;
          break;
        };

      //�������� ��ᮬ ���⭮�� ������, �������� ������
      item.weight=item.weight_calc;
      item.handmade=false;
      for(TPaidBagList::const_iterator j=curr_paid.begin(); j!=curr_paid.end(); ++j)
        if (j->key()==item.key())
        {
          if (j->weight!=NoExists &&
              (item.weight_calc==NoExists || j->weight!=item.weight_calc))
          {
            item.weight=j->weight;
            item.handmade=true;
          };
          break;
        };

      if (!testing)
      {
        if (item.norm_per_unit || (item.norm_ordinary && itemOrdinary.norm_per_unit))
        {
          //�஢�ਬ �� ����� ��������� ஢�� �� ������ �����
          list<TSimpleBagItem> added_bag;
          set_difference(curr_bag_sorted.begin(), curr_bag_sorted.end(),
                         prior_bag_sorted.begin(), prior_bag_sorted.end(),
                         inserter(added_bag, added_bag.end()));
          for(list<TSimpleBagItem>::const_iterator b=added_bag.begin(); b!=added_bag.end(); ++b)
            if (b->amount>1)
            {
              if (!item.isRegular())
                throw UserException("MSG.LUGGAGE.EACH_SEAT_SHOULD_WEIGHTED_SEPARATELY",
                                    LParams() << LParam("bagtype", item.key().str()));
              else
                throw UserException("MSG.LUGGAGE.EACH_COMMON_SEAT_SHOULD_WEIGHTED_SEPARATELY");
            };
        };
      };
    };
  };

  paid_wide.insert(paid_calc.begin(), paid_calc.end());
}

//�ନ�㥬 ���ᨢ TWidePaxInfo ��� ���쭥�襣� �맮�� CheckOrGetWidePaxNorms
//�ᯮ������ � test_norms � RecalcPaidBagToDB
void GetWidePaxInfo(const TAirlines &airlines,
                    const TBagList &curr_bag,
                    const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                    const CheckIn::TTransferList &trfer,
                    const CheckIn::TPaxGrpItem &grp,
                    const CheckIn::TPaxList &curr_paxs,
                    bool pr_unaccomp,
                    bool use_traces,
                    map<int/*pax_id*/, TWidePaxInfo> &paxs)
{
  paxs.clear();

  string target=((const TAirpsRow&)base_tables.get("airps").get_row("code",grp.airp_arv,true)).city;
  string final_target;
  if (!trfer.empty())
    final_target=((const TAirpsRow&)base_tables.get("airps").get_row("code",trfer.back().airp_arv,true)).city;

  if (!pr_unaccomp)
  {
    //ॣ������ ���ᠦ�஢
    if (prior_paxs.empty())
    {
      //����� ॣ������
      for(const CheckIn::TPaxListItem& pCurr : curr_paxs)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.setCategory(true, pCurr, TPaxToLogInfo());
        pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
        pax.subcl=pCurr.pax.subcl;
        pax.tkn=pCurr.pax.tkn;
        pax.paxId=boost::in_place(pCurr.getExistingPaxIdOrSwear());
        pax.refused=!pCurr.pax.refuse.empty();
        convertCurrentNormsList(airlines, curr_bag, pCurr.norms, pax.curr_norms);

        if (!paxs.emplace(pax.paxId.get().get(), pax).second)
          throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax.paxId.get().get());
        if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      };
    }
    else
    {
      //������ ���������
      for(const auto& i : prior_paxs)
      {
        const TPaxToLogInfo& pPrior=i.second;
        int paxIdPrior=i.first.pax_id;

        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.setCategory(false, CheckIn::TPaxListItem(), pPrior);
        pax.cl=grp.status!=psCrew?pPrior.orig_cl:EncodeClass(Y);
        pax.subcl=pPrior.subcl;
        pax.tkn=pPrior.tkn;
        pax.paxId=boost::in_place(paxIdPrior);
        pax.refused=!pPrior.refuse.empty();
        convertPriorNormsList(airlines, curr_bag, pPrior.norms, pax.prior_norms);

        CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin();
        for(; pCurr!=curr_paxs.end(); ++pCurr)
        {
          if (pCurr->pax.id==NoExists) throw Exception("%s: pCurr->pax.id==NoExists!", __FUNCTION__);
          if (pCurr->pax.tid==NoExists) throw Exception("%s: pCurr->pax.tid==NoExists!", __FUNCTION__);
          if (pCurr->pax.id==paxIdPrior) break;
        }
        if (pCurr!=curr_paxs.end())
        {
          //���ᠦ�� ������
          if (pCurr->pax.PaxUpdatesPending)
          {
            pax.setCategory(false, *pCurr, pPrior);
            pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
            pax.subcl=pCurr->pax.subcl;
            pax.tkn=pCurr->pax.tkn;
            pax.paxId=boost::in_place(pCurr->pax.id);
            pax.refused=!pCurr->pax.refuse.empty();
          };
          convertCurrentNormsList(airlines, curr_bag, pCurr->norms, pax.curr_norms);
        }
        if (!paxs.emplace(pax.paxId.get().get(), pax).second)
          throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax.paxId.get().get());
        if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      };
    };
  }
  else
  {
    //��ᮯ஢������� �����
    TWidePaxInfo pax;
    pax.target=target;
    pax.final_target=final_target;
    pax.cl=grp.cl;
    pax.refused=!grp.bag_refuse.empty();
    convertCurrentNormsList(airlines, curr_bag, grp.norms, pax.curr_norms);
    if (!prior_paxs.empty())
    {
      //������ ��������� ��ᮯ஢��������� ������
      if (prior_paxs.size()!=1)
        throw Exception("%s: prior_paxs.size()!=1!", __FUNCTION__);

      convertPriorNormsList(airlines, curr_bag, prior_paxs.begin()->second.norms, pax.prior_norms);
    };
    paxs.emplace(grp.id, pax);
    if (use_traces) ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
  };
}

//�������� ���
//�� �室� paxs.prior_norms paxs.curr_norms
//�� ��室� paxs.result_norms � ��騩 ᯨ᮪ ��� ��� ��ࠧॣ����஢����� ���ᠦ�஢
//�ᯮ������ � test_norms � RecalcpaidBagToDB
void CheckOrGetWidePaxNorms(const TAirlines &airlines,
                            const TBagList &curr_bag,
                            const TNormFltInfo &flt,
                            bool pr_unaccomp,
                            bool use_traces,
                            map<int/*pax_id*/, TWidePaxInfo> &paxs,
                            AllPaxNormContainer &all_norms)
{
  all_norms.clear();

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(curr_bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //�ᥣ�� ������塞 ����� �����
  not_trfer_bag_simple.emplace(RegularBagType(airlines.single()), std::list<TSimpleBagItem>());

  for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    TWidePaxInfo &pax=p->second;

    SyncHandmadeProp(pax.prior_norms, true, pax.curr_norms);

    for(TSimpleBagMap::const_iterator i=not_trfer_bag_simple.begin(); i!=not_trfer_bag_simple.end(); ++i)
    {
      const TBagTypeListKey& key=i->first;

      boost::optional<TPaxNormComplex> result;
      for(int pass=1; pass<=3 && !result; pass++)
      {
        //1 ��室: ���� � �ନ����
        //2 ��室: ���� ����
        //3 ��室: �������� ���室��� ��ଠ
        boost::optional<TPaxNormItem> paxNorm;
        if (pass==1 || pass==2)
        {
          paxNorm=algo::find_opt<boost::optional>(pass==1?pax.curr_norms:pax.prior_norms, key);
          if (!paxNorm) continue;
        }

        Calculator::instance().checkOrGetPaxBagNorm(flt, pax, pax.only_category(), key, paxNorm, result);
      }

      if (!result)
        result=boost::in_place(key, TNormItem());
      pax.result_norms.emplace(result.get(), result.get());
      //����� ����� ��ନ஢���� result
      if (!pax.refused) all_norms.push_back(result.get()); //������塞 ���� ��ࠧॣ����஢����� ���ᠦ�஢
    };

    if (use_traces)
    {
      if (!pr_unaccomp)
        ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      else
        ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
    };
  };
}

static void normsToDB(bool pr_unaccomp,
                      const map<int/*pax_id*/, TWidePaxInfo>& paxs)
{
  for(map<int/*pax_id*/, TWidePaxInfo>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    const TWidePaxInfo &pax=p->second;
    if (pax.prior_norms==pax.result_norms) continue;//�� �뫮 ��������� ��� � ���ᦨ�

    TPaxNormItemContainer norms=algo::transform<TPaxNormItemContainer>(pax.result_norms, [](const auto& n) { return n.second; });
    if (!pr_unaccomp)
      PaxNormsToDB(p->first, norms);
    else
      GrpNormsToDB(p->first, norms);
  }
}

static void baseRecalcPaidBag(const TAirlines &airlines,
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
                              map<int/*pax_id*/, TWidePaxInfo>& wide_paxs,
                              TPaidBagList &result_paid)
{
  wide_paxs.clear();
  result_paid.clear();
  Calculator::instance().clearFlightBagNorms();

  if (use_traces) ProgTrace(TRACE5, "%s: flt:%s", __FUNCTION__, flt.traceStr().c_str());

  GetWidePaxInfo(airlines, curr_bag, prior_paxs, trfer, grp, curr_paxs, pr_unaccomp, use_traces, wide_paxs);

  AllPaxNormContainer all_norms;
  CheckOrGetWidePaxNorms(airlines, curr_bag, flt, pr_unaccomp, use_traces, wide_paxs, all_norms);

  //ᮡ�⢥��� ���� ���⭮�� ������
  TPaidBagWideMap paid_wide;
  RecalcPaidBagWide(airlines, prior_bag, curr_bag, all_norms, prior_paid, grp.paid?grp.paid.get():TPaidBagList(), paid_wide);

  for(TPaidBagWideMap::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
  {
    const TPaidBagWideItem &item=i->second;
    if (item.weight==NoExists)
    {
      if (!item.isRegular())
        throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_BAG_TYPE",
                            LParams() << LParam("bagtype", item.key().str()));
      else
        throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_ORDINARY");
    }
    result_paid.push_back(item);
  }
}

static void RecalcPaidBag(const TTripInfo& flt,
                          const CheckIn::TSimplePaxGrpItem& grp,
                          const boost::optional< std::list< std::pair<int, CheckIn::TSimpleBagItem> > >& additionalBaggage,
                          const TPaidBagList& prior_paid,
                          map<int/*pax_id*/, TWidePaxInfo>& wide_paxs,
                          TPaidBagList &result_paid)
{
  wide_paxs.clear();
  result_paid.clear();

  TGrpMktFlight grpMktFlight;
  if (!grpMktFlight.getByGrpId(grp.id))
    grpMktFlight=flt.grpMktFlight();

  TAirlines airlines(grp.id, flt, grpMktFlight, __func__);

  TGrpToLogInfo grpInfoBefore;
  GetGrpToLogInfo(grp.id, grpInfoBefore);
  TBagList prior_bag(grpInfoBefore.bag);
  TBagList curr_bag=prior_bag;
  if (additionalBaggage)
  {
    for(const auto& b : additionalBaggage.get())
      curr_bag.emplace_back(b.second, false, false);
  }

  CheckIn::TTransferList trfer;
  trfer.load(grp.id);

  CheckIn::TPaxGrpItem grpItem;
  static_cast<CheckIn::TSimplePaxGrpItem&>(grpItem)=grp;

  baseRecalcPaidBag(airlines,
                    prior_bag,
                    curr_bag,
                    grpInfoBefore.pax,
                    TNormFltInfo(flt, grpMktFlight),
                    trfer,
                    grpItem,
                    CheckIn::TPaxList(),
                    prior_paid,
                    grp.is_unaccomp(),
                    true,
                    wide_paxs,
                    result_paid);
}

void RecalcPaidBag(const TTripInfo& flt,
                   const CheckIn::TSimplePaxGrpItem& grp,
                   const std::list< std::pair<int, CheckIn::TSimpleBagItem> >& additionalBaggage,
                   TPaidBagList& prior_paid,
                   TPaidBagList& result_paid)
{
  PaidBagFromDB(NoExists, grp.id, prior_paid);
  map<int/*pax_id*/, TWidePaxInfo> wide_paxs;
  RecalcPaidBag(flt, grp, additionalBaggage, prior_paid, wide_paxs, result_paid);
}

void RecalcPaidBag(const TTripInfo& flt,
                   const CheckIn::TSimplePaxGrpItem& grp,
                   map<int/*pax_id*/, TWidePaxInfo>& wide_paxs)
{
  TPaidBagList prior_paid, result_paid;
  PaidBagFromDB(NoExists, grp.id, prior_paid);

  RecalcPaidBag(flt, grp, boost::none, prior_paid, wide_paxs, result_paid);
}

//������� ������ � ������樥� ��� � ������� � ⠡���� grp_norms, pax_norms � paid_bag
//�ਬ������ �� ����� ��������� �� ��㯯� ���ᠦ�஢ (SavePax)
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
                       TPaidBagList &result_paid)
{
  map<int/*pax_id*/, TWidePaxInfo> wide_paxs;

  baseRecalcPaidBag(airlines, prior_bag, curr_bag, prior_paxs, flt, trfer, grp, curr_paxs, prior_paid, pr_unaccomp, use_traces, wide_paxs, result_paid);

  PaidBagToDB(grp.id, pr_unaccomp, result_paid);
  normsToDB(pr_unaccomp, wide_paxs);
}

//���� ������ - �� ��室� TPaidBagWideItem
//�ᯮ������ � PaidBagViewToXML
void CalcPaidBagWide(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     const TPaidBagList &paid,
                     TPaidBagWideMap &paid_wide)
{
  paid_wide.clear();

  TPaidBagCalcMap paid_calc;
  CalcPaidBagBase(airlines, bag, norms, paid_calc);

  for(TPaidBagCalcMap::const_iterator p=paid_calc.begin(); p!=paid_calc.end(); ++p)
  {
    TPaidBagWideMap::iterator i=paid_wide.insert(*p).first;
    TPaidBagWideItem &item=i->second;
    item.weight=item.weight_calc;
    item.handmade=false;
    for(TPaidBagList::const_iterator j=paid.begin(); j!=paid.end(); ++j)
      if (j->key()==item.key())
      {
        item.weight=j->weight;
        item.rate_id=j->rate_id;
        item.rate=j->rate;
        item.rate_cur=j->rate_cur;
        item.rate_trfer=j->rate_trfer;
        item.handmade=j->handmade;
        break;
      };
  };

  LogTrace(TRACE5) << __func__ <<  ": " << norms;
  TracePaidBagWide(paid_wide, __FUNCTION__);
}

void CalcTrferBagWide(const TBagList &bag,
                      TPaidBagWideMap &paid_wide)
{
  paid_wide.clear();

  TSimpleBagMap bag_simple;
  PrepareSimpleBag(bag, psbtOnlyTrfer, psbtOnlyNotRefused, bag_simple);

  TPaidBagCalcMap paid_calc;
  for(TSimpleBagMap::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid_calc.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  for(TPaidBagCalcMap::const_iterator p=paid_calc.begin(); p!=paid_calc.end(); ++p)
  {
    TPaidBagWideMap::iterator i=paid_wide.insert(*p).first;
    TPaidBagWideItem &item=i->second;
    item.weight=0;
    item.handmade=false;
  };

  TracePaidBagWide(paid_wide, __FUNCTION__);
}

//���� ������ - �� ��室� TPaidBagCalcItem - ᠬ�� ������ ��楤��
void CalcPaidBagBase(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     TPaidBagCalcMap &paid)
{
  //� ����� ����� �� �������� �ਢ易��� � ࠧॣ����஢���� ���ᠦ�ࠬ
  paid.clear();

  TSimpleBagMap bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, bag_simple);

  for(TSimpleBagMap::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  //� ���ᨢ� �ᥣ�� ����� �����:
  const TBagTypeListKey regularKey=RegularBagType(airlines.single());
  if (paid.find(regularKey)==paid.end())
    paid.insert(make_pair(regularKey, TPaidBagCalcItem(regularKey, std::list<TSimpleBagItem>())));

  //᫮��� �� ����:
  for(const TPaxNormComplex& n : norms)
  {
    TPaidBagCalcMap::iterator i=paid.find(n);
    if (i==paid.end()) continue;
    i->second.addNorm(n);
  };

  bool ordinaryUnknown=false;
  TPaidBagCalcMap::iterator iOrdinary=paid.find(regularKey);
  if (iOrdinary==paid.end()) throw Exception("%s: iOrdinary==paid.end()", __FUNCTION__);
  TPaidBagCalcItem &itemOrdinary=iOrdinary->second;
  for(int pass=0; pass<2; pass++)
  {
    for(TPaidBagCalcMap::iterator i=paid.begin(); i!=paid.end(); ++i)
    {
      TPaidBagCalcItem &item=i->second;
      if ((pass==0 && item.isRegular()) ||
          (pass!=0 && !item.isRegular())) continue; //����� ����� �ᥣ�� � ��᫥���� ���樨

      if (item.isRegularBagType() && ordinaryUnknown) item.weight_calc=NoExists;
      else
      {
        //�஢�ਬ ������ ��� �� ���� ��ଠ
        if (item.norms.empty())
        {
          if (!item.isRegularBagType())
          {
            //�� ����� ���� ��� �뤥������� ⨯� ������ �� ������
            //��ॡ��뢠�� � ����� �����
            itemOrdinary.bag.insert(itemOrdinary.bag.end(), item.bag.begin(), item.bag.end());
            item.bag.clear();
            item.weight_calc=0;
          }
          else
          {
            //�� ����� ���� ��� ���筮�� ������ �� ������
            //��⠥� ����� ����� �����
            item.weight_calc=0;
            for(list<TSimpleBagItem>::const_iterator j=item.bag.begin(); j!=item.bag.end(); ++j) item.weight_calc+=j->weight;
          };
        }
        else
        {
          //����㥬 ᯨ᪨ ������ � ���
          item.bag.sort();
          item.norms.sort();
          TNormWideItem &norm=item.norms.front();
          if (item.norms.front().priority==item.norms.back().priority)
          {
            item.weight_calc=0;
            //��������� ⨯ ��� - ��⠥� ����� �����
            if (norm.amount==NoExists &&
                norm.weight==NoExists &&
                norm.per_unit==NoExists)
            {
              for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
              {
                bool inc=true;
                switch(norm.norm_type)
                {
                  case bntOrdinary:
                    if (!item.isRegularBagType())
                    {
                      itemOrdinary.bag.push_back(*j);
                      j=item.bag.erase(j);
                      inc=false;
                    };
                    break;
                  case bntPaid:
                    item.weight_calc+=j->weight;
                    break;
                  default:
                    break;
                };
                if (inc) ++j;
              };
            };
            if (norm.amount==NoExists &&
                norm.weight!=NoExists && norm.weight>0 &&
                norm.per_unit!=NoExists && norm.per_unit!=0)
            {
              for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
              {
                bool inc=true;
                if (j->weight>norm.weight)
                {
                  if (j->amount>1)
                  {
                    item.weight_calc=NoExists;
                    break;
                  };
                  switch(norm.norm_type)
                  {
                    case bntFreeExcess:
                      item.weight_calc+=j->weight-norm.weight;
                      break;
                    case bntFreeOrdinary:
                      if (!item.isRegularBagType())
                      {
                        if (!itemOrdinary.norm_per_unit) j->weight-=norm.weight;
                        itemOrdinary.bag.push_back(*j);
                        j=item.bag.erase(j);
                        inc=false;
                      };
                      break;
                    case bntFreePaid:
                    case bntOrdinaryPaid:
                      item.weight_calc+=j->weight;
                      break;
                    default:
                      break;
                  };
                }
                else
                {
                  switch(norm.norm_type)
                  {
                    case bntOrdinaryPaid:
                      if (!item.isRegularBagType())
                      {
                        itemOrdinary.bag.push_back(*j);
                        j=item.bag.erase(j);
                        inc=false;
                      };
                      break;
                    default:
                      break;
                  };
                };
                if (inc) ++j;
              };
            };
            if (norm.amount!=NoExists && norm.amount>0 &&
                norm.weight==NoExists &&
                norm.per_unit==NoExists)
            {
              int k=0;
              for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();k++)
              {
                bool inc=true;
                if (k>=norm.amount)
                {
                  switch(norm.norm_type)
                  {
                    case bntFreeExcess:
                    case bntOrdinaryPaid:
                    case bntFreePaid:
                      item.weight_calc+=j->weight;
                      break;
                    case bntFreeOrdinary:
                      if (!item.isRegularBagType())
                      {
                        itemOrdinary.bag.push_back(*j);
                        j=item.bag.erase(j);
                        inc=false;
                      };
                      break;
                    default:
                      break;
                  }
                }
                else
                {
                  if (j->amount>1)
                  {
                    item.weight_calc=NoExists;
                    break;
                  };
                  switch(norm.norm_type)
                  {
                    case bntOrdinaryPaid:
                      if (!item.isRegularBagType())
                      {
                        itemOrdinary.bag.push_back(*j);
                        j=item.bag.erase(j);
                        inc=false;
                      };
                      break;
                    default:
                      break;
                  };
                };
                if (inc) ++j;
              };
            };
            if (norm.amount==NoExists &&
                norm.weight!=NoExists && norm.weight>0 &&
                norm.per_unit!=NoExists && norm.per_unit==0)
            {
              if (!(norm.norm_type==bntFreeOrdinary && itemOrdinary.norm_per_unit))
              {
                //���筠� ��ଠ ������
                //��⠥� ��騩 ��� ������
                int k=0;
                for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end(); ++j) k+=j->weight;
                if (k>norm.weight)
                {
                  switch(norm.norm_type)
                  {
                    case bntFreeExcess:
                      item.weight_calc+=k-norm.weight;
                      break;
                    case bntFreeOrdinary:
                      if (!item.isRegularBagType())
                      {
                        TSimpleBagItem sum(item);
                        sum.amount=1;
                        sum.weight=k-norm.weight;
                        itemOrdinary.bag.push_back(sum);
                      };
                      break;
                    case bntFreePaid:
                    case bntOrdinaryPaid:
                      item.weight_calc=k;
                      break;
                    default:
                      break;
                  };
                }
                else
                {
                  switch(norm.norm_type)
                  {
                    case bntOrdinaryPaid:
                      itemOrdinary.bag.insert(itemOrdinary.bag.end(), item.bag.begin(), item.bag.end());
                      item.bag.clear();
                      break;
                    default:
                      break;
                  };
                };
              }
              else
              {
                //2 ᯮᮡ� ���᫥���: �������쭮� ���-�� ����, ���ᨬ��쭮� ���-�� ����
                //�������쭮� ���-�� ����
                int k=0;    //���, ����� 㬥頥��� � ����
                for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end(); ++j)
                  if (k+j->weight<=norm.weight) k+=j->weight;
                //���ᨬ��쭮� ���-�� ����
                int l=0;
                for(list<TSimpleBagItem>::reverse_iterator j=item.bag.rbegin(); j!=item.bag.rend(); ++j)
                  if (l+j->weight<=norm.weight) l+=j->weight;
                if (k>l)
                {
                  k=0;
                  for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
                  {
                    bool inc=true;
                    if (k+j->weight>norm.weight)
                    {
                      if (!item.isRegularBagType())
                      {
                        itemOrdinary.bag.push_back(*j);
                        j=item.bag.erase(j);
                        inc=false;
                      };
                    }
                    else k+=j->weight;
                    if (inc) ++j;
                  };
                }
                else
                {
                  l=0;
                  for(list<TSimpleBagItem>::reverse_iterator j=item.bag.rbegin(); j!=item.bag.rend();)
                  {
                    bool inc=true;
                    if (l+j->weight>norm.weight)
                    {
                      if (!item.isRegularBagType())
                      {
                        itemOrdinary.bag.push_back(*j);
                        item.bag.erase(--j.base());
                        inc=false;
                      };
                    }
                    else l+=j->weight;
                    if (inc) ++j;
                  };
                };
              };
            };
            if (norm.amount!=NoExists && norm.amount>0 &&
                norm.weight!=NoExists && norm.weight>0 &&
                norm.per_unit!=NoExists && norm.per_unit!=0)
            {
              list<TNormWideItem>::iterator k=item.norms.begin(); //��६����� �� ��ଠ�
              int l=0; //��६����� �� ���-�� ���� � ��ଥ
              for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
              {
                bool inc=true;
                if (norm.norm_type==bntFreeExcess ||
                    (norm.norm_type==bntFreeOrdinary && !itemOrdinary.norm_per_unit))
                {
                  if (k==item.norms.end())
                  {
                    //���� �����稫���, � ����� ���
                    switch(norm.norm_type)
                    {
                      case bntFreeExcess:
                        item.weight_calc+=j->weight;
                        break;
                      case bntFreeOrdinary:
                        if (!item.isRegularBagType())
                        {
                          itemOrdinary.bag.push_back(*j);
                          j=item.bag.erase(j);
                          inc=false;
                        };
                        break;
                      default:
                        break;
                    };
                  }
                  else
                  {
                    if (j->amount>1)
                    {
                      item.weight_calc=NoExists;
                      break;
                    };
                    if (k->weight<j->weight)
                      switch(norm.norm_type)
                      {
                        case bntFreeExcess:
                          item.weight_calc+=j->weight-k->weight;
                          break;
                        case bntFreeOrdinary:
                          if (!item.isRegularBagType())
                          {
                            j->weight-=k->weight;
                            itemOrdinary.bag.push_back(*j);
                            j=item.bag.erase(j);
                            inc=false;
                          }
                          break;
                        default:
                          break;
                      };
                    l++;
                    if (l>=k->amount)
                    {
                      k++;
                      l=0;
                    };
                  };
                }
                else
                {
                  if (k!=item.norms.end() && j->amount>1)
                  {
                    item.weight_calc=NoExists;
                    break;
                  };
                  if (k==item.norms.end() ||
                      k->weight<j->weight)

                  {
                    //���� ���௠�� ��� �� ���室��
                    switch(norm.norm_type)
                    {
                      case bntFreeOrdinary:
                        if (!item.isRegularBagType())
                        {
                          itemOrdinary.bag.push_back(*j);
                          j=item.bag.erase(j);
                          inc=false;
                        }
                        break;
                      case bntOrdinaryPaid:
                      case bntFreePaid:
                        item.weight_calc+=j->weight;
                        break;
                      default:
                        break;
                    };
                  }
                  else
                  {
                    switch(norm.norm_type)
                    {
                      case bntOrdinaryPaid:
                        if (!item.isRegularBagType())
                        {
                          itemOrdinary.bag.push_back(*j);
                          j=item.bag.erase(j);
                          inc=false;
                        }
                        break;
                      default:
                        break;
                    };
                    l++;
                    if (l>=k->amount)
                    {
                      k++;
                      l=0;
                    };
                  };
                };
                if (inc) ++j;
              };
            };
          }
          else
          {
            //ᬥ襭�� ���
            item.weight_calc=(item.bag.empty()?0:NoExists);
          };
        };
      };

      //��� ���⭮�� ������ �஢�ਬ ���� �� �।� ��� ��
      if (item.isRegularBagType() && item.weight_calc==NoExists && item.norm_ordinary) ordinaryUnknown=true;

      item.weight=NoExists;
    };
  };

}

void CalcPaidBagView(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     const TPaidBagList &paid,
                     const CheckIn::TServicePaymentListWithAuto &payment,
                     const std::string &used_airline_mark,
                     TPaidBagViewMap &paid_view,
                     TPaidBagViewMap &trfer_view)
{
  paid_view.clear();
  trfer_view.clear();

  for(bool is_trfer=false; ;is_trfer=!is_trfer)
  {
    TPaidBagWideMap paid_wide;
    if (!is_trfer)
      CalcPaidBagWide(airlines, bag, norms, paid, paid_wide);
    else
      CalcTrferBagWide(bag, paid_wide);

    TPaidBagViewMap &result=!is_trfer?paid_view:trfer_view;
    for(TPaidBagWideMap::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
    {
      TPaidBagViewItem item(i->second,
                            i->second.weight_calc,
                            i->second.total_view(),
                            i->second.norms_view(is_trfer, used_airline_mark),
                            i->second.norms_trfer_view());
      if (item.weight==NoExists)
        throw Exception("%s: item.weight==NoExists!", __FUNCTION__);

      if (!is_trfer)
        item.weight_rcpt=payment.getDocWeight(i->first);

      if (item.list_id==ASTRA::NoExists)
      {
        //��⠭���� list_id � list_item �� �᭮�� bag
        for(const TBagInfo& b : bag)
          if (b.is_trfer==is_trfer && b.wt && b.wt.get().key()==item.key())
          {
            item.list_id=b.wt.get().list_id;
            item.list_item=b.wt.get().list_item;
            break;
          }
      };
      if (item.list_id==ASTRA::NoExists && !is_trfer)
      {
        //��⠭���� list_id � list_item �� �᭮�� paid
        for(TPaidBagList::const_iterator p=paid.begin(); p!=paid.end(); ++p)
          if (p->key()==item.key())
          {
            item.list_id=p->list_id;
            item.list_item=p->list_item;
            break;
          }
      };
      item.getListItemIfNone();

      result.insert(make_pair(item, item));
    };
    if (is_trfer) break;
  };
}

void PaidBagViewToXML(const TPaidBagViewMap &paid_view,
                      const TPaidBagViewMap &trfer_view,
                      xmlNodePtr node)
{
  if (node==NULL) return;

  for(bool is_trfer=false; ;is_trfer=!is_trfer)
  {
    const TPaidBagViewMap &result=!is_trfer?paid_view:trfer_view;

    xmlNodePtr servicesNode=!is_trfer?NewTextChild(node, "paid_bags"):
                                      NewTextChild(node, "trfer_bags");
    for(TPaidBagViewMap::const_iterator i=result.begin(); i!=result.end(); ++i)
      i->second.toXML(NewTextChild(servicesNode, "paid_bag"));

    if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION)) break; //�� ��ࠡ��뢠�� �࠭��� - �ࠧ� ��室��
    if (is_trfer) break;
  };
};

string GetBagRcptStr(int grp_id, int pax_id)
{
  int main_pax_id=NoExists;
  if (pax_id!=NoExists)
  {
    TQuery Qry(&OraSession);
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.SQLText=
      "SELECT ckin.get_main_pax_id2(:grp_id) AS main_p1ax_id FROM dual";
    Qry.Execute();
    if (!Qry.Eof && !Qry.FieldIsNULL("main_pax_id")) main_pax_id=Qry.FieldAsInteger("main_pax_id");
  };
  if (pax_id==NoExists ||
      (main_pax_id!=NoExists && main_pax_id==pax_id))
  {
    vector<string> rcpts;
    CheckIn::TServicePaymentListWithAuto payment;
    payment.fromDB(grp_id);
    for(CheckIn::TServicePaymentListWithAuto::const_iterator i=payment.begin(); i!=payment.end(); ++i)
      if (i->trfer_num==0 && i->wt) rcpts.push_back(i->doc_no); //!!!��祬� ⮫쪮 trfer_num=0?

    DB::TQuery QryPrepay(PgOra::getROSession("BAG_PREPAY"), STDLOG);
    QryPrepay.CreateVariable("grp_id", otInteger, grp_id);
    QryPrepay.SQLText="SELECT no FROM bag_prepay "
                      "WHERE grp_id=:grp_id";
    QryPrepay.Execute();
    for(;!QryPrepay.Eof;QryPrepay.Next()) {
      rcpts.push_back(QryPrepay.FieldAsString("no"));
    }

    DB::TQuery Qry(PgOra::getROSession("BAG_RECEIPTS"), STDLOG);
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.SQLText="SELECT form_type,no FROM bag_receipts "
                "WHERE grp_id=:grp_id "
                "AND annul_date IS NULL";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      int no_len=10;
      try
      {
        no_len=base_tables.get("form_types").get_row("code",Qry.FieldAsString("form_type")).AsInteger("no_len");
      }
      catch(const EBaseTableError&) {};
      ostringstream no_str;
      no_str << fixed << setw(no_len) << setfill('0') << setprecision(0) << Qry.FieldAsFloat("no");
      rcpts.push_back(no_str.str());
    };
    if (!rcpts.empty())
    {
      sort(rcpts.begin(),rcpts.end());
      return ::GetBagRcptStr(rcpts);
    }
  }
  return "";
}

bool BagPaymentCompleted(int grp_id, int *value_bag_count)
{
  vector< pair< string, int> > paid_bag;      //< bag_type, weight > � ��� �� �⮡� �� TBagTypeKey ����� string bag_type!
  vector< pair< double, string > > value_bag; //< value, value_cur >
  DB::TQuery QryPaid(PgOra::getROSession("PAID_BAG"), STDLOG);
  QryPaid.CreateVariable("grp_id", otInteger, grp_id);

  QryPaid.SQLText="SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id AND weight>0";
  QryPaid.Execute();
  for(;!QryPaid.Eof;QryPaid.Next())
  {
    string bag_type=BagTypeFromDB(QryPaid);
    paid_bag.push_back( make_pair(bag_type, QryPaid.FieldAsInteger("weight")) );
  };

  TQuery Qry(&OraSession);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.SQLText=
    "SELECT DISTINCT value_bag.num, value_bag.value, value_bag.value_cur "
    "FROM pax_grp, value_bag, bag2 "
    "WHERE pax_grp.grp_id=value_bag.grp_id AND "
    "      value_bag.grp_id=bag2.grp_id(+) AND "
    "      value_bag.num=bag2.value_bag_num(+) AND "
    "      (bag2.grp_id IS NULL OR "
    "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0) AND "
    "      pax_grp.grp_id=:grp_id AND value_bag.value>0";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    value_bag.push_back( make_pair(Qry.FieldAsFloat("value"), Qry.FieldAsString("value_cur")) );
  };
  if (value_bag_count!=NULL) *value_bag_count=value_bag.size();

  if (paid_bag.empty() && value_bag.empty()) return true;

  DB::TQuery KitQry(PgOra::getROSession({"BAG_RCPT_KITS", "BAG_RECEIPTS"}), STDLOG);
  KitQry.SQLText=
      "SELECT bag_rcpt_kits.kit_id "
      "FROM bag_rcpt_kits "
      "WHERE bag_rcpt_kits.kit_id = :kit_id "
      "AND NOT EXISTS ("
      "  SELECT 1 FROM bag_receipts "
      "  WHERE bag_receipts.kit_id = bag_rcpt_kits.kit_id "
      "  AND bag_receipts.kit_num = bag_rcpt_kits.kit_num "
      "  AND bag_receipts.annul_date IS NULL "
      ")";
  KitQry.DeclareVariable("kit_id", otInteger);

  if (!paid_bag.empty())
  {
    map< string/*bag_type*/, int > rcpt_paid_bag;
    std::string sql;
    std::string table_name;
    for(int pass=0;pass<=2;pass++)
    {
      switch (pass)
      {
        case 0: sql=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND annul_date IS NULL AND service_type IN (1,2) AND kit_id IS NULL";
                table_name = "BAG_RECEIPTS";
                break;
        case 1: sql=
                  "SELECT MIN(bag_type) AS bag_type, MIN(ex_weight) AS ex_weight, kit_id "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND service_type IN (1,2) AND kit_id IS NOT NULL "
                  "GROUP BY kit_id";
                table_name = "BAG_RECEIPTS";
                break;
        case 2: sql=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_prepay "
                  "WHERE grp_id=:grp_id AND value IS NULL";
                table_name = "BAG_PREPAY";
                break;
      };
      DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
      Qry.SQLText = sql;
      Qry.CreateVariable("grp_id", otInteger, grp_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (pass==1)
        {
          KitQry.SetVariable("kit_id", Qry.FieldAsInteger("kit_id"));
          KitQry.Execute();
          if (!KitQry.Eof) continue;
        };
        string bag_type=BagTypeFromDB(Qry);
        if (rcpt_paid_bag.find(bag_type)==rcpt_paid_bag.end())
          rcpt_paid_bag[bag_type]=Qry.FieldAsInteger("ex_weight");
        else
          rcpt_paid_bag[bag_type]+=Qry.FieldAsInteger("ex_weight");
      };
    };
    //EMD
    CheckIn::TServicePaymentListWithAuto payment;
    payment.fromDB(grp_id);
    for(CheckIn::TServicePaymentListWithAuto::const_iterator i=payment.begin(); i!=payment.end(); ++i)
      if (i->trfer_num==0 && i->wt && i->doc_weight!=ASTRA::NoExists)
      {
        string bag_type=i->wt->bag_type;
        if (rcpt_paid_bag.find(bag_type)==rcpt_paid_bag.end())
          rcpt_paid_bag[bag_type]=i->doc_weight;
        else
          rcpt_paid_bag[bag_type]+=i->doc_weight;
      }

    for(vector< pair< string, int> >::const_iterator i=paid_bag.begin();i!=paid_bag.end();++i)
    {
      map< string, int >::const_iterator j=rcpt_paid_bag.find(i->first);
      if (j==rcpt_paid_bag.end()) return false;
      if (j->second<i->second) return false;
    };
  };

  if (!value_bag.empty())
  {
    map< pair< double, string >, int > rcpt_value_bag;
    for(int pass=0;pass<=2;pass++)
    {
      switch (pass)
      {
        case 0: Qry.SQLText=
                  "SELECT rate AS value,rate_cur AS value_cur "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND annul_date IS NULL AND service_type=3 AND kit_id IS NULL";
                  break;
        case 1: Qry.SQLText=
                  "SELECT MIN(rate) AS value, MIN(rate_cur) AS value_cur, kit_id "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND service_type=3 AND kit_id IS NOT NULL "
                  "GROUP BY kit_id";
                  break;
        case 2: Qry.SQLText=
                  "SELECT value,value_cur "
                  "FROM bag_prepay "
                  "WHERE grp_id=:grp_id AND value IS NOT NULL";
                  break;
      };
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (pass==1)
        {
          KitQry.SetVariable("kit_id", Qry.FieldAsInteger("kit_id"));
          KitQry.Execute();
          if (!KitQry.Eof) continue;
        };
        pair< double, string > bag(Qry.FieldAsFloat("value"), Qry.FieldAsString("value_cur"));
        if (rcpt_value_bag.find(bag)==rcpt_value_bag.end())
          rcpt_value_bag[bag]=1;
        else
          rcpt_value_bag[bag]+=1;
      };
    };

    for(vector< pair< double, string > >::const_iterator i=value_bag.begin();i!=value_bag.end();++i)
    {
      map< pair< double, string >, int >::iterator j=rcpt_value_bag.find(*i);
      if (j==rcpt_value_bag.end()) return false;
      if (j->second<=0) return false;
      j->second--;
    };
  };

  return true;
};

std::string GetBagAirline(const TTripInfo &operFlt, const TTripInfo &markFlt, bool is_local_scd_out)
{
  TCodeShareSets codeshareSets;
  codeshareSets.get(operFlt, markFlt, is_local_scd_out);
  return codeshareSets.pr_mark_norms?markFlt.airline:operFlt.airline;
}

boost::optional<BagAllowance> getBagAllowance(const CheckIn::TSimplePaxItem& pax)
{
  boost::optional<TPaxNormComplex> res=pax.getRegularNorm();
  if (!res) return boost::none;

  const TNormItem& norm=res.get();

  if (norm.norm_type==bntFreeExcess ||
      norm.norm_type==bntFreePaid)
    return BagAllowance(norm);

  return BagAllowance();
}

boost::optional<BagAllowance> calcBagAllowance(const CheckIn::TSimplePaxItem& pax,
                                               const CheckIn::TSimplePaxGrpItem& grp,
                                               const TTripInfo& flt)
{
  map<int/*pax_id*/, TWidePaxInfo> paxs;
  TPaidBagList paid;
  RecalcPaidBag(flt, grp, paxs);

  const auto p=paxs.find(pax.id);
  if (p==paxs.end()) return boost::none;

  const auto n=algo::find_opt_if<boost::optional>(p->second.result_norms,
                                                  [](const auto& n) { return n.second.isRegular(); });
  if (!n || n.get().second.normNotExists()) return boost::none;

  TNormItem norm;
  norm.getByNormId(n.get().second.norm_id);

  if (norm.norm_type==bntFreeExcess ||
      norm.norm_type==bntFreePaid)
    return BagAllowance(norm);

  return BagAllowance();
}

} //namespace WeightConcept

namespace PieceConcept
{

string GetBagRcptStr(int grp_id, int pax_id)
{
  vector<string> rcpts;
  CheckIn::TServicePaymentListWithAuto payment;
  payment.fromDB(grp_id);
  for(CheckIn::TServicePaymentListWithAuto::const_iterator i=payment.begin(); i!=payment.end(); ++i)
    if (i->trfer_num==0 && i->pc && //!!!��祬� ⮫쪮 trfer_num=0?
        (i->pax_id==ASTRA::NoExists || pax_id==ASTRA::NoExists || i->pax_id==pax_id)) rcpts.push_back(i->doc_no);
  if (!rcpts.empty())
  {
    sort(rcpts.begin(),rcpts.end());
    return ::GetBagRcptStr(rcpts);
  };
  return "";
}

boost::optional<BagAllowance> getBagAllowance(const CheckIn::TSimplePaxItem& pax)
{
  if (!pax.tkn.validET()) return boost::none;

  TETickItem etick;
  etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
  if (!etick.bagNorm) return boost::none;

  return BagAllowance(etick.bagNorm.get());
}

} //namespace PieceConcept

string GetBagRcptStr(const vector<string> &rcpts)
{
  ostringstream result;
  string prior_no;
  for(vector<string>::const_iterator no=rcpts.begin(); no!=rcpts.end(); ++no)
  {
    int no_len=no->size();
    if (no!=rcpts.begin() &&
        no_len>2 &&
        no_len==(int)prior_no.size() &&
        no->substr(0,no_len-2)==prior_no.substr(0,no_len-2))
    {
      result << "/" << no->substr(no_len-2);
    }
    else
    {
      if (no!=rcpts.begin()) result << "/";
      result << *no;
    };
    prior_no=*no;
  };
  return result.str();
}

boost::optional<TBagQuantity> trueBagNorm(const boost::optional<TBagQuantity>& crsBagNorm,
                                          const TETickItem& etick)
{
  if (etick.bagNorm && etick.bagNorm.get().getUnit()==Ticketing::Baggage::NumPieces)
    return etick.bagNorm;

  if (crsBagNorm)
    return crsBagNorm;

  return etick.bagNorm;
}


std::string trueBagNormView(const boost::optional<TBagQuantity>& crsBagNorm,
                            const TETickItem& etick,
                            const AstraLocale::OutputLang &lang)
{
  //�� ��楤�� �� �᭮�뢠���� �� trueBagNorm, ��⮬� �� etick.bagNormView ����� ������ �������⥫쭮� ���ﭨ� "���"
  //�� ���ﭨ� ����砥� �� � ��ᯫ�� �� ��ଠ �� 㪠���� �����
  if (etick.bagNorm && etick.bagNorm.get().getUnit()==Ticketing::Baggage::NumPieces)
    return etick.bagNormView(lang);

  if (crsBagNorm)
    return crsBagNorm.get().view(lang);

  return etick.bagNormView(lang);
}


