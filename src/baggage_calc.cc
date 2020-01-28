#include "baggage_calc.h"
#include "qrys.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "transfer.h"
#include "astra_misc.h"
#include "term_version.h"
#include "baggage_wt.h"
#include "etick.h"
#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;

namespace WeightConcept
{

boost::optional<TNormItem> TPaxInfo::etickNormFromDB() const
{
  if (!tkn.validET()) return boost::none;

  TETickItem etick;
  if (etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false).empty()) return boost::none;

  if (etick.bag_norm_unit.get()!=Ticketing::Baggage::WeightKilo &&
      etick.bag_norm_unit.get()!=Ticketing::Baggage::Nil) return boost::none;

  TNormItem etickNorm;

  if (etick.bag_norm!=ASTRA::NoExists && etick.bag_norm>0)
  {
    //норма известна и она не нулевая
    etickNorm.norm_type=bntFreeExcess;
    etickNorm.weight=etick.bag_norm;
    etickNorm.per_unit=false;
  }

  return etickNorm;
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

class TSuitableBagNormInfo;

class TBagNormWideInfo : public TNormWithPriorityInfo
{
  public:
    std::string bag_type222;
    int norm_id;
    std::string airline, city_arv, pax_cat, subcl, cl, craft;
    int pr_trfer, flt_no;
    TBagNormWideInfo()
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

    TBagNormWideInfo& fromDB(TQuery &Qry);
    boost::optional<TSuitableBagNormInfo> getSuitableBagNorm(const TPaxInfo &pax,
                                                             const TBagNormFilterSets &filter,
                                                             const TBagNormFieldAmounts &field_amounts) const;
    static void addDirectActionNorm(const TNormItem& norm);
    static boost::optional<TBagNormWideInfo> getDirectActionNorm(const TNormItem& norm);

    static const set<string>& strictly_limited_cats()
    {
      static set<string> cats = {"CHC", "CHD", "INA", "INF"};
      return cats;
    }

    bool isRegularBagType() const { return bag_type222==REGULAR_BAG_TYPE; }
};

TBagNormWideInfo& TBagNormWideInfo::fromDB(TQuery &Qry)
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


void TBagNormWideInfo::addDirectActionNorm(const TNormItem& norm)
{
  static TDateTime firstDate=NoExists;
  if (firstDate==NoExists) StrToDateTime("01.01.2020", "dd.mm.yyyy", firstDate);

  TCachedQuery Qry("INSERT INTO bag_norms(id, first_date, norm_type, amount, weight, per_unit, pr_del, direct_action, tid ) "
                   "VALUES(id__seq.nextval, :first_date, :norm_type, :amount, :weight, :per_unit, 0, 1, tid__seq.nextval)",
                   QParams() << QParam("first_date", otDate, firstDate)
                             << QParam("norm_type", otString)
                             << QParam("amount", otInteger)
                             << QParam("weight", otInteger)
                             << QParam("per_unit", otInteger));
  norm.toDB(Qry.get());
  Qry.get().Execute();
}

boost::optional<TBagNormWideInfo> TBagNormWideInfo::getDirectActionNorm(const TNormItem& norm)
{
  if (norm.weight==NoExists)
    throw Exception("%s: norm.weight==NoExists", __func__);

  TCachedQuery Qry("SELECT * FROM bag_norms "
                   "WHERE direct_action=:direct_action AND "
                   "      norm_type=:norm_type AND "
                   "      weight=:weight AND "
                   "      (amount IS NULL AND :amount IS NULL OR amount=:amount) AND "
                   "      (per_unit IS NULL AND :per_unit IS NULL OR per_unit=:per_unit) "
                   "ORDER BY id",
                   QParams() << QParam("direct_action", otInteger, (bool)true)
                             << QParam("norm_type", otString)
                             << QParam("amount", otInteger)
                             << QParam("weight", otInteger)
                             << QParam("per_unit", otInteger));
  norm.toDB(Qry.get());
  Qry.get().Execute();
  if (Qry.get().Eof) return boost::none;

  return TBagNormWideInfo().fromDB(Qry.get());
}

class TSuitableBagNormInfo : public TBagNormWideInfo
{
  public:
    int similarity_cost, priority;

    TSuitableBagNormInfo(const TBagNormWideInfo& norm_,
                         const int similarity_cost_) :
      TBagNormWideInfo(norm_),
      similarity_cost(similarity_cost_),
      priority(norm_.getPriority()) {}

    bool operator < (const TSuitableBagNormInfo &norm) const
    {
      if (similarity_cost!=norm.similarity_cost)
        return similarity_cost<norm.similarity_cost;
      if (priority!=norm.priority)
        return priority>norm.priority;
      return (amount!=NoExists && amount>=0?amount:1)*(weight!=NoExists && weight>=0?weight:1)<
             (norm.amount!=NoExists && norm.amount>=0?norm.amount:1)*(norm.weight!=NoExists && norm.weight>=0?norm.weight:1);
    }
};

boost::optional<TSuitableBagNormInfo> TBagNormWideInfo::getSuitableBagNorm(const TPaxInfo &pax, const TBagNormFilterSets &filter, const TBagNormFieldAmounts &field_amounts) const
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

  TSuitableBagNormInfo result(*this, cost);
  if (result.priority==NoExists) return boost::none; //норма введена неправильно

  return result;
}

void LoadTripBagNorms(const TFltInfo &flt,
                      list<TBagNormWideInfo> &trip_bag_norms)
{
  static string trip_bag_norms_sql;
  if (trip_bag_norms_sql.empty())
  {
    TCachedQuery CacheTablesQry("SELECT select_sql FROM cache_tables WHERE code='TRIP_BAG_NORMS2'", QParams());
    CacheTablesQry.get().Execute();
    if (CacheTablesQry.get().Eof) throw Exception("%s: trip_bag_norms_sql not defined!", __FUNCTION__);
    trip_bag_norms_sql=CacheTablesQry.get().FieldAsString("select_sql");
  };

  TCachedQuery Qry(trip_bag_norms_sql,
                   QParams() << (flt.point_id!=NoExists?QParam("point_id", otInteger, flt.point_id):
                                                        QParam("point_id", otInteger, FNull))
                             << QParam("use_mark_flt", otInteger, (int)flt.use_mark_flt)
                             << QParam("airline_mark", otString, flt.airline_mark)
                             << (flt.flt_no_mark!=NoExists?QParam("flt_no_mark", otInteger, flt.flt_no_mark):
                                                           QParam("flt_no_mark", otInteger, FNull)));
  Qry.get().Execute();
  trip_bag_norms.clear();
  for(;!Qry.get().Eof;Qry.get().Next())
    trip_bag_norms.push_back(TBagNormWideInfo().fromDB(Qry.get()));
}

//результат функции означает надо ли далее получать result стандартным механизмом среди trip_bag_norms
//true: result валидный - в trip_bag_norms не лезем
bool getPaxEtickNorm(const TPaxInfo &pax,
                     const TBagTypeListKey &bagTypeKey,
                     const boost::optional<TPaxNormItem> &paxNorm,
                     boost::optional<TPaxNormComplex> &result)
{
  result=boost::none;

  if (!bagTypeKey.isRegular()) return false;

  boost::optional<TNormItem> etickNorm=pax.etickNormFromDB();
  if (!etickNorm) return false;

  if (paxNorm)
  {
    if (!(paxNorm.get().key()==bagTypeKey))
      throw Exception("%s: paxNorm.get().key()!=bagTypeKey", __func__);

    TNormItem norm;
    bool isDirectActionNorm;
    if (paxNorm.get().normNotExists())
    {
      //считаем что отсутствующая норма прямого действия, если она не введена вручную
      isDirectActionNorm=!paxNorm.get().isManuallyDeleted();
    }
    else
    {
      if (!norm.getByNormId(paxNorm.get().norm_id, isDirectActionNorm)) return true;
    }

    if (etickNorm.get()==norm)
    {
      //по сути нормы совпадают с нормами в билете
      if (!isDirectActionNorm) return false; //проверяем стандартным механизмом среди trip_bag_norms
      result=boost::in_place(paxNorm.get(), norm);
    }
    else
    {
      //по сути нормы не совпадают с нормами в билете
      if (!isDirectActionNorm) return false; //проверяем стандартным механизмом среди trip_bag_norms - это при разрешении выбора нормы агентом
    }
    return true;
  }

  if (!etickNorm.get().isUnknown())
  {
    boost::optional<TBagNormWideInfo> bagNorm;
    bagNorm=TBagNormWideInfo::getDirectActionNorm(etickNorm.get());
    if (!bagNorm)
    {
      TBagNormWideInfo::addDirectActionNorm(etickNorm.get());
      bagNorm=TBagNormWideInfo::getDirectActionNorm(etickNorm.get());
    }

    if (bagNorm)
    {
      TPaxNormItem newPaxNorm(bagTypeKey, bagNorm.get().norm_id, false);
      result=boost::in_place(newPaxNorm, bagNorm.get());
    }
  }

  return true;
}

void CheckOrGetPaxBagNorm(const list<TBagNormWideInfo> &trip_bag_norms,
                          const TPaxInfo &pax,
                          const bool use_mixed_norms,
                          const bool only_category,
                          const TBagTypeListKey &bagTypeKey,
                          const boost::optional<TPaxNormItem> &paxNorm,
                          boost::optional<TPaxNormComplex> &result)
{
  result=boost::none;

  if (getPaxEtickNorm(pax, bagTypeKey, paxNorm, result)) return;

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

  TBagNormFieldAmounts field_amounts;
  TBagNormFilterSets filter;
  filter.use_basic=algo::none_of(trip_bag_norms, [](const TBagNormWideInfo& n) { return !n.airline.empty(); });
  filter.only_category=only_category;

  boost::optional<TSuitableBagNormInfo> curr, max;
  filter.bagTypeKey=bagTypeKey;
  filter.is_trfer=!pax.final_target.empty();
  for(;;filter.is_trfer=false)
  {
    for(const TBagNormWideInfo& norm : trip_bag_norms)
    {
      filter.check=(paxNorm &&
                    paxNorm.get().bag_type==norm.bag_type222 &&
                    paxNorm.get().norm_id==norm.norm_id &&
                    paxNorm.get().norm_trfer==filter.is_trfer);

      curr=norm.getSuitableBagNorm(pax, filter, field_amounts);
      if (!curr) continue;

      if (filter.check)
      {
        //нашли норму и она валидна
        max=curr;
        break;
      }

      if (!max || max.get()<curr.get()) max=curr;
    }
    if (!filter.is_trfer) break;
    if (max || !use_mixed_norms) break; //выходим если мы подцепили трансферную норму
    //или мы ее не подцепили, но запрещено использовать в случае оформления трансфера нетрансферные нормы
  };

  if (max)
  {
    TPaxNormItem newPaxNorm(bagTypeKey, max.get().norm_id, filter.is_trfer);
    if (paxNorm)
    {
      if (newPaxNorm.equal(paxNorm.get()))
        result=boost::in_place(paxNorm.get(), max.get()); //это чтобы сохранить признак handmade из paxNorm
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
  //считаем norms_trfer
  switch(norms_trfer)
  {
           case nttNone: norms_trfer=(norm.norm_trfer?nttTransfer:nttNotTransfer); break;
    case nttNotTransfer: norms_trfer=(norm.norm_trfer?nttMixed:nttNotTransfer); break;
       case nttTransfer: norms_trfer=(norm.norm_trfer?nttTransfer:nttMixed); break;
          case nttMixed: break;
  }
  //ищем подобие в norms
  list< TNormWideItem >::iterator n=norms.begin();
  for(; n!=norms.end(); ++n)
    if (n->tryAddNorm(new_norm)) break;
  if (n==norms.end())
    //не нашли с чем суммировать норму - добавляем в конец
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
        s << getLocaleText("см. подробно", lang);
    }
    else s << "-";
  }
  else s << getLocaleText("Трансфер", lang);
  return s.str();
}

std::string TPaidBagWideItem::norms_trfer_view(const std::string& lang) const
{
  ostringstream s;
  switch(norms_trfer)
  {
    case nttNone: break;
    case nttNotTransfer: s << getLocaleText("НЕТ", lang); break;
    case nttTransfer: s << getLocaleText("ДА", lang); break;
    case nttMixed: s << getLocaleText("СМЕШ", lang); break;
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

//декларация расчета багажа - на выходе TPaidBagCalcItem - самая нижняя процедура
void CalcPaidBagBase(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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

void ConvertNormsList(const TAirlines &airlines,
                      const TBagList &bag,
                      const boost::optional< std::list<TPaxNormItem> > &norms,
                      TPaxNormMap &result)
{
  result.clear();
  if (!norms) return;

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //всегда добавляем обычный багаж
  not_trfer_bag_simple.emplace(RegularBagType(airlines.single()), std::list<TSimpleBagItem>());

  for(const TPaxNormItem& n : norms.get())
  {
    if (not_trfer_bag_simple.find(n)==not_trfer_bag_simple.end()) continue; //отфильтровываем нормы

    if (!result.emplace(n, n).second)
      throw Exception("%s: %s already exists in result!", __FUNCTION__, n.key().str(LANG_EN));
  };
}

void ConvertNormsList(const TAirlines &airlines,
                      const TBagList &bag,
                      const WeightConcept::TPaxNormComplexContainer &norms,
                      TPaxNormMap &result)
{

  const std::list<TPaxNormItem> tmp_norms=algo::transform<std::list>(norms,
                                                                     [](const TPaxNormItem& n) { return n; });
  ConvertNormsList(airlines, bag, tmp_norms, result);
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

//перерасчет багажа - на выходе TPaidBagWideItem
//используется в RecalcPaidBagToDB
void RecalcPaidBagWide(const TAirlines &airlines,
                       const TBagList &prior_bag,
                       const TBagList &curr_bag,
                       const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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
          (pass!=0 && !item.isRegular())) continue; //обычный багаж всегда в последней итерации

      multiset<TSimpleBagItem> prior_bag_sorted;
      TSimpleBagMap::const_iterator iPriorBag=prior_bag_simple.find(item);
      if (iPriorBag!=prior_bag_simple.end()) prior_bag_sorted.insert(iPriorBag->second.begin(), iPriorBag->second.end());

      multiset<TSimpleBagItem> curr_bag_sorted;
      TSimpleBagMap::const_iterator iCurrBag=curr_bag_simple.find(item);
      if (iCurrBag!=curr_bag_simple.end()) curr_bag_sorted.insert(iCurrBag->second.begin(), iCurrBag->second.end());

      //заполним информацией о тарифах
      for(TPaidBagList::const_iterator j=prior_paid.begin(); j!=prior_paid.end(); ++j)
        if (j->key()==item.key())
        {
          item.rate_id=j->rate_id;
          item.rate=j->rate;
          item.rate_cur=j->rate_cur;
          item.rate_trfer=j->rate_trfer;
          break;
        };

      //заполним весом платного багажа, введенным вручную
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
          //проверим что багаж заводился ровно по одному месту
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

//формируем массив TWidePaxInfo для дальнейшего вызова CheckOrGetWidePaxNorms
//используется в test_norms и RecalcPaidBagToDB
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
    //регистрация пассажиров
    if (prior_paxs.empty())
    {
      //новая регистрация
      for(const CheckIn::TPaxListItem& pCurr : curr_paxs)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.setCategory(true, pCurr, TPaxToLogInfo());
        pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
        pax.subcl=pCurr.pax.subcl;
        pax.tkn=pCurr.pax.tkn;
        pax.refused=!pCurr.pax.refuse.empty();
        ConvertNormsList(airlines, curr_bag, pCurr.norms, pax.curr_norms);

        int pax_id=pCurr.getExistingPaxIdOrSwear();
        if (!paxs.emplace(pax_id, pax).second)
          throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax_id);
        if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      };
    }
    else
    {
      //запись изменений
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
        pax.refused=!pPrior.refuse.empty();
        ConvertNormsList(airlines, curr_bag, pPrior.norms, pax.prior_norms);

        CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin();
        for(; pCurr!=curr_paxs.end(); ++pCurr)
        {
          if (pCurr->pax.id==NoExists) throw Exception("%s: pCurr->pax.id==NoExists!", __FUNCTION__);
          if (pCurr->pax.tid==NoExists) throw Exception("%s: pCurr->pax.tid==NoExists!", __FUNCTION__);
          if (pCurr->pax.id==paxIdPrior) break;
        }
        if (pCurr!=curr_paxs.end())
        {
          //пассажир менялся
          if (pCurr->pax.PaxUpdatesPending)
          {
            pax.setCategory(false, *pCurr, pPrior);
            pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
            pax.subcl=pCurr->pax.subcl;
            pax.tkn=pCurr->pax.tkn;
            pax.refused=!pCurr->pax.refuse.empty();
          };
          ConvertNormsList(airlines, curr_bag, pCurr->norms, pax.curr_norms);

          if (!paxs.emplace(pCurr->pax.id, pax).second)
            throw Exception("%s: pCurr->pax.id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
          if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());

        }
        else
        {
          //пассажир не менялся
          if (!paxs.emplace(paxIdPrior, pax).second)
            throw Exception("%s: paxIdPrior=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
          if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
        }
      };
    };
  }
  else
  {
    //несопровождаемый багаж
    TWidePaxInfo pax;
    pax.target=target;
    pax.final_target=final_target;
    pax.cl=grp.cl;
    pax.refused=!grp.bag_refuse.empty();
    ConvertNormsList(airlines, curr_bag, grp.norms, pax.curr_norms);
    if (!prior_paxs.empty())
    {
      //запись изменений несопровождаемого багажа
      if (prior_paxs.size()!=1)
        throw Exception("%s: prior_paxs.size()!=1!", __FUNCTION__);

      ConvertNormsList(airlines, curr_bag, prior_paxs.begin()->second.norms, pax.prior_norms);
    };
    paxs.emplace(grp.id, pax);
    if (use_traces) ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
  };
}

//валидация норм
//на входе paxs.prior_norms paxs.curr_norms
//на выходе paxs.result_norms и общий список всех норм неразрегистрированных пассажиров
//используется в test_norms и RecalcpaidBagToDB
void CheckOrGetWidePaxNorms(const TAirlines &airlines,
                            const list<TBagNormWideInfo> &trip_bag_norms,
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
  //всегда добавляем обычный багаж
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
        //1 проход: нормы с терминала
        //2 проход: старые нормы
        //3 проход: наиболее подходящая норма
        boost::optional<TPaxNormItem> paxNorm;
        if (pass==1 || pass==2)
        {
          paxNorm=algo::find_opt<boost::optional>(pass==1?pax.curr_norms:pax.prior_norms, key);
          if (!paxNorm) continue;
        }

        CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category(), key, paxNorm, result);
      }

      if (!result)
        result=boost::in_place(key, TNormItem());
      pax.result_norms.emplace(result.get(), result.get());
      //здесь имеем сформированный result
      if (!pax.refused) all_norms.push_back(result.get()); //добавляем нормы неразрегистрированных пассажиров
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
    if (pax.prior_norms==pax.result_norms) continue;//не было изменения норм у пассжира

    TPaxNormItemContainer norms=algo::transform<TPaxNormItemContainer>(pax.result_norms, [](const auto& n) { return n.second; });
    if (!pr_unaccomp)
      PaxNormsToDB(p->first, norms);
    else
      GrpNormsToDB(p->first, norms);
  }
}

static void RecalcPaidBag(const TAirlines &airlines,
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

  if (use_traces) ProgTrace(TRACE5, "%s: flt:%s", __FUNCTION__, flt.traceStr().c_str());

  GetWidePaxInfo(airlines, curr_bag, prior_paxs, trfer, grp, curr_paxs, pr_unaccomp, use_traces, wide_paxs);

  //расчет/проверка багажных норм
  list<TBagNormWideInfo> trip_bag_norms;
  LoadTripBagNorms(flt, trip_bag_norms);

  AllPaxNormContainer all_norms;
  CheckOrGetWidePaxNorms(airlines, trip_bag_norms, curr_bag, flt, pr_unaccomp, use_traces, wide_paxs, all_norms);

  //собственно расчет платного багажа
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

  RecalcPaidBag(airlines,
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

//перерасчет багажа с валидацией норм и записью в таблицы grp_norms, pax_norms и paid_bag
//применяется при записи изменений по группе пассажиров (SavePax)
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

  RecalcPaidBag(airlines, prior_bag, curr_bag, prior_paxs, flt, trfer, grp, curr_paxs, prior_paid, pr_unaccomp, use_traces, wide_paxs, result_paid);

  PaidBagToDB(grp.id, pr_unaccomp, result_paid);
  normsToDB(pr_unaccomp, wide_paxs);
}

//CREATE TABLE drop_test_norm_processed
//(
//  point_id NUMBER(9) NOT NULL
//);

//CREATE TABLE drop_test_norm_errors
//(
//    point_id NUMBER(9) NOT NULL,
//    grp_id NUMBER(9) NOT NULL,
//    pax_id NUMBER(9) NULL,
//    error VARCHAR2(2000) NOT NULL,
//    trace VARCHAR2(2000) NOT NULL
//);

//CREATE INDEX drop_test_norm_errors__IDX ON drop_test_norm_errors(point_id);

int test_norms(int argc,char **argv)
{
  bool test_paid=false;
  if (argc>=2 && strcmp(argv[1], "paid")==0)
    test_paid=true;

  TReqInfo::Instance()->Initialize("МОВ");
  TReqInfo::Instance()->desk.lang=LANG_EN;


  TQuery GrpQry(&OraSession);
  GrpQry.Clear();
  ostringstream sql;
  sql << "SELECT point_id, grp_id, airp_arv, class, status, bag_refuse, client_type, "
         "       mark_trips.airline, mark_trips.flt_no, pr_mark_norms "
         "FROM pax_grp, mark_trips "
         "WHERE pax_grp.point_id_mark=mark_trips.point_id AND point_dep=:point_id ";
  if (test_paid)
    sql << "      AND EXISTS(SELECT * FROM bag2 WHERE grp_id=pax_grp.grp_id AND rownum<2) ";
  sql << "ORDER BY mark_trips.airline, mark_trips.flt_no, pr_mark_norms";

  GrpQry.SQLText=sql.str().c_str();
  GrpQry.DeclareVariable("point_id", otInteger);

  TQuery Qry(&OraSession);

  set<int> processed_point_ids;
  Qry.Clear();
  Qry.SQLText="SELECT point_id FROM drop_test_norm_processed";
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next()) processed_point_ids.insert(Qry.FieldAsInteger("point_id"));

  TQuery Qry2(&OraSession);
  Qry2.Clear();
  Qry2.SQLText="INSERT INTO drop_test_norm_processed(point_id) VALUES(:point_id)";
  Qry2.DeclareVariable("point_id", otInteger);

  TQuery Qry3(&OraSession);
  Qry3.Clear();
  Qry3.SQLText="DELETE FROM drop_test_norm_errors WHERE point_id=:point_id";
  Qry3.DeclareVariable("point_id", otInteger);

  TQuery Qry4(&OraSession);
  Qry4.Clear();
  Qry4.SQLText="INSERT INTO drop_test_norm_errors(point_id, grp_id, pax_id, error, trace) "
               "VALUES(:point_id, :grp_id, :pax_id, :error, :trace)";
  Qry4.DeclareVariable("point_id", otInteger);
  Qry4.DeclareVariable("grp_id", otInteger);
  Qry4.DeclareVariable("pax_id", otInteger);
  Qry4.DeclareVariable("error", otString);
  Qry4.DeclareVariable("trace", otString);

  TQuery Qry5(&OraSession);
  Qry5.Clear();
  Qry5.SQLText=
    "SELECT msg FROM events_bilingual "
    "WHERE lang='RU' AND type='ПАС' AND id1=:point_id AND id3=:grp_id AND "
    "      msg like '%мест/вес/опл%' "
    "ORDER BY time DESC, ev_order DESC";
  Qry5.DeclareVariable("point_id", otInteger);
  Qry5.DeclareVariable("grp_id", otInteger);

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out "
    "FROM points "
    "WHERE scd_out>=TO_DATE('02.02.17','DD.MM.YY') AND scd_out<TO_DATE('16.02.17','DD.MM.YY') AND pr_reg=1";
  Qry.Execute();
  list<TBagNormWideInfo> trip_bag_norms;
  TNormFltInfo prior;
  int count=0;
  int incomplete_norms_count=0;
  int complete_norms_count=0;
  for(; !Qry.Eof; Qry.Next())
  {
    TNormFltInfo flt;
    flt.point_id=Qry.FieldAsInteger("point_id");

    if (processed_point_ids.find(flt.point_id)!=processed_point_ids.end()) continue;

    TTripInfo fltInfo(Qry);

    flt.use_mixed_norms=GetTripSets(tsMixedNorms,fltInfo);

    GrpQry.SetVariable("point_id", flt.point_id);
    GrpQry.Execute();
    if (GrpQry.Eof)
    {
      Qry2.SetVariable("point_id", flt.point_id);
      Qry2.Execute();
      OraSession.Commit();
      continue;
    };
    Qry3.SetVariable("point_id", flt.point_id);
    Qry3.Execute();
    bool error_exists=false;
    for(; !GrpQry.Eof; GrpQry.Next())
    {
      flt.use_mark_flt=GrpQry.FieldAsInteger("pr_mark_norms")!=0;
      flt.airline_mark=GrpQry.FieldAsString("airline");
      flt.flt_no_mark=GrpQry.FieldAsInteger("flt_no");
      if (!(prior==flt))
      {
        LoadTripBagNorms(flt, trip_bag_norms);
        ProgTrace(TRACE5, "%s: LoadTripBagNorms", __FUNCTION__);
        prior=flt;
      };

      CheckIn::TPaxGrpItem grp;
      grp.id=GrpQry.FieldAsInteger("grp_id");
      grp.airp_arv=GrpQry.FieldAsString("airp_arv");
      grp.cl=GrpQry.FieldAsString("class");
      grp.status=DecodePaxStatus(GrpQry.FieldAsString("status"));
      grp.bag_refuse=GrpQry.FieldAsInteger("bag_refuse")!=0?refuseAgentError:"";
      //TClientType client_type=DecodeClientType(GrpQry.FieldAsString("client_type"));
      TAirlines airlines(GetCurrSegBagAirline(grp.id)); //test_norms - checked!

      TGrpToLogInfo grpLogInfo;
      CheckIn::TTransferList trfer;
      GetGrpToLogInfo(grp.id, grpLogInfo);
      trfer.load(grp.id);

      if (!test_paid)
      {
        TSimpleBagMap not_trfer_bag_simple;
        PrepareSimpleBag(TBagList(grpLogInfo.bag), psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
        //всегда добавляем обычный багаж
        not_trfer_bag_simple.insert(make_pair(RegularBagType(airlines.single()), std::list<TSimpleBagItem>()));


        map<int/*pax_id*/, TWidePaxInfo> paxs;
        GetWidePaxInfo(airlines, TBagList(grpLogInfo.bag), grpLogInfo.pax, trfer, grp, CheckIn::TPaxList(), grp.is_unaccomp(), false, paxs);

        map<int/*pax_id*/, TWidePaxInfo> tmp_paxs=paxs;
        for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=tmp_paxs.begin(); p!=tmp_paxs.end(); ++p)
        {
          p->second.prior_norms.clear();
          p->second.curr_norms.clear();
        };

        set<int> error_ids;
        AllPaxNormContainer all_norms;
        for(int pass=0; pass<2; pass++)
        {
          //1 проход проверяет что все нормы которые привязаны на реальном сервере разрешены новым алгоритмом
          //2 проход проверяет что все нормы которые привязаны на реальном сервере автоматически рассчитываются новым алгоритмом

          CheckOrGetWidePaxNorms(airlines, trip_bag_norms, TBagList(grpLogInfo.bag), flt, grp.is_unaccomp(), false, (pass==0?paxs:tmp_paxs), all_norms);

          if (pass==1)
          {
            map<int/*pax_id*/, TWidePaxInfo>::iterator p1=paxs.begin();
            map<int/*pax_id*/, TWidePaxInfo>::iterator p2=tmp_paxs.begin();
            for(; p1!=paxs.end() && p2!=tmp_paxs.end(); ++p1, ++p2)
            {
              if (p1->first!=p2->first) throw Exception("%s: p1->first!=p2->first", __FUNCTION__);
              TWidePaxInfo &pax1=p1->second;
              TWidePaxInfo &pax2=p2->second;
              pax2.prior_norms=pax1.prior_norms;
              pax2.curr_norms=pax1.curr_norms;
            };
            if (p1!=paxs.end() || p2!=tmp_paxs.end()) throw Exception("%s: p1!=paxs.end() || p2!=tmp_paxs.end()", __FUNCTION__);
          }

          for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=(pass==0?paxs:tmp_paxs).begin(); p!=(pass==0?paxs:tmp_paxs).end(); ++p)
          {
            TWidePaxInfo &pax=p->second;
            if (pax.prior_norms==pax.result_norms) continue;

            TPaxNormMap::const_iterator i1=pax.prior_norms.begin();
            TPaxNormMap::const_iterator i2=pax.result_norms.begin();
            for(; i1!=pax.prior_norms.end() && i2!=pax.result_norms.end(); ++i1, ++i2)
            {
              if (!(i1->first==i2->first) ||
                  !(i1->second.key()==i2->second.key()) ||
                  i1->second.norm_id!=i2->second.norm_id) break;
            };
            if (i1==pax.prior_norms.end() && i2==pax.result_norms.end()) continue;

            if (/*client_type!=ctTerm &&*/ not_trfer_bag_simple.size()==1 && not_trfer_bag_simple.begin()->first==RegularBagType(airlines.single()) &&
                pax.prior_norms.empty() && pax.result_norms.size()==1 && pax.result_norms.begin()->first.isRegular()) continue;

            if (error_ids.find(p->first)!=error_ids.end()) continue;

            error_exists=true;
            error_ids.insert(p->first);

            ostringstream error;
            ostringstream trace;
            error << (pass==0?"forbidden":"mismatched") << " norms flight=" << GetTripName(fltInfo, ecNone, true, false ).c_str()
                  << " grp_id=" << grp.id;
            if (!grp.is_unaccomp()) error << " pax_id=" << p->first;
            trace << pax.traceStr();
            ProgError(STDLOG, "%s: %s", __FUNCTION__, error.str().c_str());
            ProgError(STDLOG, "%s: %s", __FUNCTION__, trace.str().c_str());

            Qry4.SetVariable("point_id", flt.point_id);
            Qry4.SetVariable("grp_id", grp.id);
            if (!grp.is_unaccomp())
              Qry4.SetVariable("pax_id", p->first);
            else
              Qry4.SetVariable("pax_id", FNull);
            Qry4.SetVariable("error", error.str());
            Qry4.SetVariable("trace", trace.str());
            Qry4.Execute();
          }
        }
      }
      else
      {
        TSimpleBagMap bag_simple;
        PrepareSimpleBag(TBagList(grpLogInfo.bag), psbtOnlyNotTrfer, psbtOnlyNotRefused, bag_simple);
        if (!bag_simple.empty())
        {
          TPaidBagList prior_paid;
          PaidBagFromDB(NoExists, grp.id, prior_paid);

          bool norms_incomplete=false;
          AllPaxNormContainer all_norms;
          for(std::map<TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator p=grpLogInfo.pax.begin(); p!=grpLogInfo.pax.end(); ++p)
          {
            if (!p->second.refuse.empty()) continue;
            TPaxNormMap norms_map;
            for(const TPaxNormComplex& n : p->second.norms)
            {
              norms_map.emplace(n, n);
              all_norms.push_back(n);
            };
            TSimpleBagMap::const_iterator b=bag_simple.begin();
            for(; b!=bag_simple.end(); ++b)
              if (norms_map.find(b->first)==norms_map.end())
              {
                norms_incomplete=true;
                break;
              }
            if (b!=bag_simple.end()) break;
          }

          if (!norms_incomplete)
          {
            complete_norms_count++;
            TPaidBagWideMap paid_wide;
            RecalcPaidBagWide(airlines, TBagList(grpLogInfo.bag), TBagList(grpLogInfo.bag), all_norms, prior_paid, TPaidBagList(), paid_wide, true);

            TPaidBagMap sorted_prior_paid;
            TPaidBagMap sorted_calc_paid;
            for(TPaidBagList::const_iterator i=prior_paid.begin(); i!=prior_paid.end(); ++i)
              sorted_prior_paid.insert(make_pair(i->key(), *i));
            for(TPaidBagWideMap::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
              sorted_calc_paid.insert(*i);

            if (sorted_prior_paid!=sorted_calc_paid)
            {
              Qry5.SetVariable("point_id", flt.point_id);
              Qry5.SetVariable("grp_id", grp.id);
              Qry5.Execute();
              string msg;
              bool print_error=false;
              if (!Qry5.Eof)
              {
                msg=Qry5.FieldAsString("msg");
                for(TPaidBagWideMap::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
                {
                  ostringstream s;
                  s << i->second.bag_amount << "/" << i->second.bag_weight << "/";
                  TPaidBagMap::const_iterator j=sorted_prior_paid.find(i->first);
                  if (j!=sorted_prior_paid.end())
                    s << j->second.weight;
                  else
                    s << "-";
                  if (msg.find(s.str())==string::npos)
                  {
                    ProgTrace(TRACE5, "%s: not found msg=%s %s", __FUNCTION__, msg.c_str(), s.str().c_str());
                    print_error=true;
                    break;
                  }
                  else
                    ProgTrace(TRACE5, "%s: found msg=%s %s", __FUNCTION__, msg.c_str(), s.str().c_str());

                };
              }
              else print_error=true;

              if (print_error)
              {
                error_exists=true;
                ostringstream error;
                ostringstream trace;
                error << "different paid flight=" << GetTripName(fltInfo, ecNone, true, false ).c_str();
                trace << "prior_paid: " << sorted_prior_paid << endl
                      << "calc_paid: " << sorted_calc_paid;
                ProgError(STDLOG, "%s: %s", __FUNCTION__, error.str().c_str());
                ProgError(STDLOG, "%s: %s", __FUNCTION__, trace.str().c_str());

                Qry4.SetVariable("point_id", flt.point_id);
                Qry4.SetVariable("grp_id", grp.id);
                Qry4.SetVariable("pax_id", FNull);
                Qry4.SetVariable("error", error.str());
                Qry4.SetVariable("trace", trace.str());
                Qry4.Execute();
              };
            }
          }
          else incomplete_norms_count++;
        };
      };
      OraSession.Commit();
      ProgTrace(TRACE5, "%s: completed norms: %d; uncompleted norms: %d", __FUNCTION__, complete_norms_count, incomplete_norms_count);
    }
    if (!error_exists)
    {
      Qry2.SetVariable("point_id", flt.point_id);
      Qry2.Execute();
    };
    OraSession.Commit();
    count++;
  }
  ProgTrace(TRACE5, "%s: %d flights processed", __FUNCTION__, count);
  return 1;
}

//расчет багажа - на выходе TPaidBagWideItem
//используется в PaidBagViewToXML
void CalcPaidBagWide(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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

//расчет багажа - на выходе TPaidBagCalcItem - самая нижняя процедура
void CalcPaidBagBase(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                     TPaidBagCalcMap &paid)
{
  //в платный багаж не попадает привязанный к разрегистрированным пассажирам
  paid.clear();

  TSimpleBagMap bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, bag_simple);

  for(TSimpleBagMap::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  //в массиве всегда обычный багаж:
  const TBagTypeListKey regularKey=RegularBagType(airlines.single());
  if (paid.find(regularKey)==paid.end())
    paid.insert(make_pair(regularKey, TPaidBagCalcItem(regularKey, std::list<TSimpleBagItem>())));

  //сложим все нормы:
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
          (pass!=0 && !item.isRegular())) continue; //обычный багаж всегда в последней итерации

      if (item.isRegularBagType() && ordinaryUnknown) item.weight_calc=NoExists;
      else
      {
        //проверим задана хотя бы одна норма
        if (item.norms.empty())
        {
          if (!item.isRegularBagType())
          {
            //ни одной нормы для выделенного типа багажа не задано
            //перебрасываем в обычный багаж
            itemOrdinary.bag.insert(itemOrdinary.bag.end(), item.bag.begin(), item.bag.end());
            item.bag.clear();
            item.weight_calc=0;
          }
          else
          {
            //ни одной нормы для обычного багажа не задано
            //считаем обычный багаж платным
            item.weight_calc=0;
            for(list<TSimpleBagItem>::const_iterator j=item.bag.begin(); j!=item.bag.end(); ++j) item.weight_calc+=j->weight;
          };
        }
        else
        {
          //сортируем списки багажа и норм
          item.bag.sort();
          item.norms.sort();
          TNormWideItem &norm=item.norms.front();
          if (item.norms.front().priority==item.norms.back().priority)
          {
            item.weight_calc=0;
            //одинаковый тип норм - считаем платный багаж
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
                //обычная норма багажа
                //считаем общий вес багажа
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
                //2 способа вычисления: минимальное кол-во мест, максимальное кол-во мест
                //минимальное кол-во мест
                int k=0;    //вес, который умещается в норму
                for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end(); ++j)
                  if (k+j->weight<=norm.weight) k+=j->weight;
                //максимальное кол-во мест
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
              list<TNormWideItem>::iterator k=item.norms.begin(); //переменная по нормам
              int l=0; //переменная по кол-ву мест в норме
              for(list<TSimpleBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
              {
                bool inc=true;
                if (norm.norm_type==bntFreeExcess ||
                    (norm.norm_type==bntFreeOrdinary && !itemOrdinary.norm_per_unit))
                {
                  if (k==item.norms.end())
                  {
                    //нормы закончились, а багаж нет
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
                    //нормы исчерпаны или не подходят
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
            //смешение норм
            item.weight_calc=(item.bag.empty()?0:NoExists);
          };
        };
      };

      //для платного багажа проверим есть ли среди норм ОБ
      if (item.isRegularBagType() && item.weight_calc==NoExists && item.norm_ordinary) ordinaryUnknown=true;

      item.weight=NoExists;
    };
  };

}

void CalcPaidBagView(const TAirlines &airlines,
                     const TBagList &bag,
                     const AllPaxNormContainer &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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
        //установим list_id и list_item на основе bag
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
        //установим list_id и list_item на основе paid
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

    if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION)) break; //не обрабатываем трансфер - сразу выходим
    if (is_trfer) break;
  };
};

string GetBagRcptStr(int grp_id, int pax_id)
{
  TQuery Qry(&OraSession);
  Qry.CreateVariable("grp_id", otInteger, grp_id);

  int main_pax_id=NoExists;
  if (pax_id!=NoExists)
  {
    Qry.SQLText=
      "SELECT ckin.get_main_pax_id2(:grp_id) AS main_pax_id FROM dual";
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
      if (i->trfer_num==0 && i->wt) rcpts.push_back(i->doc_no); //!!!почему только trfer_num=0?

    Qry.SQLText="SELECT no FROM bag_prepay WHERE grp_id=:grp_id";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
      rcpts.push_back(Qry.FieldAsString("no"));

    Qry.SQLText="SELECT form_type,no FROM bag_receipts WHERE grp_id=:grp_id AND annul_date IS NULL";
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
    };
  };
  return "";
};

bool BagPaymentCompleted(int grp_id, int *value_bag_count)
{
  vector< pair< string, int> > paid_bag;      //< bag_type, weight > а хорошо бы чтобы был TBagTypeKey вместо string bag_type!
  vector< pair< double, string > > value_bag; //< value, value_cur >
  TQuery Qry(&OraSession);
  Qry.CreateVariable("grp_id", otInteger, grp_id);

  Qry.SQLText="SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id AND weight>0";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    string bag_type=BagTypeFromDB(Qry);
    paid_bag.push_back( make_pair(bag_type, Qry.FieldAsInteger("weight")) );
  };

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

  TQuery KitQry(&OraSession);
  KitQry.Clear();
  KitQry.SQLText=
    "SELECT bag_rcpt_kits.kit_id "
    "FROM bag_rcpt_kits, bag_receipts "
    "WHERE bag_rcpt_kits.kit_id=bag_receipts.kit_id(+) AND "
    "      bag_rcpt_kits.kit_num=bag_receipts.kit_num(+) AND "
    "      bag_receipts.annul_date(+) IS NULL AND "
    "      bag_rcpt_kits.kit_id=:kit_id AND "
    "      bag_receipts.kit_id IS NULL";
  KitQry.DeclareVariable("kit_id", otInteger);

  if (!paid_bag.empty())
  {
    map< string/*bag_type*/, int > rcpt_paid_bag;
    for(int pass=0;pass<=2;pass++)
    {
      switch (pass)
      {
        case 0: Qry.SQLText=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND annul_date IS NULL AND service_type IN (1,2) AND kit_id IS NULL";
                break;
        case 1: Qry.SQLText=
                  "SELECT MIN(bag_type) AS bag_type, MIN(ex_weight) AS ex_weight, kit_id "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND service_type IN (1,2) AND kit_id IS NOT NULL "
                  "GROUP BY kit_id";
                break;
        case 2: Qry.SQLText=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_prepay "
                  "WHERE grp_id=:grp_id AND value IS NULL";
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
        string bag_type=i->wt.get().bag_type;
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

boost::optional<TBagTotals> getBagAllowance(const CheckIn::TSimplePaxItem& pax)
{
  boost::optional<TPaxNormComplex> res=pax.getRegularNorm();
  if (!res) return boost::none;

  const TNormItem& norm=res.get();

  if (norm.norm_type==bntFreeExcess ||
      norm.norm_type==bntFreePaid)
    return TBagTotals( norm.amount, norm.weight );

  return TBagTotals(ASTRA::NoExists, 0);
}

boost::optional<TBagTotals> calcBagAllowance(const CheckIn::TSimplePaxItem& pax,
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
    return TBagTotals( norm.amount, norm.weight );

  return boost::none;
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
    if (i->trfer_num==0 && i->pc && //!!!почему только trfer_num=0?
        (i->pax_id==ASTRA::NoExists || pax_id==ASTRA::NoExists || i->pax_id==pax_id)) rcpts.push_back(i->doc_no);
  if (!rcpts.empty())
  {
    sort(rcpts.begin(),rcpts.end());
    return ::GetBagRcptStr(rcpts);
  };
  return "";
}

boost::optional<TBagTotals> getBagAllowance(const CheckIn::TSimplePaxItem& pax)
{
  if (!pax.tkn.validET()) return boost::none;

  TETickItem etick;
  etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);

  if (etick.empty()) return boost::none;

  if (etick.bag_norm_unit.get()==Ticketing::Baggage::NumPieces &&
      !(etick.bag_norm==ASTRA::NoExists || etick.bag_norm==0))
    return TBagTotals(etick.bag_norm, ASTRA::NoExists);

  return TBagTotals(0, ASTRA::NoExists);
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



