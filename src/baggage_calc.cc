#include "baggage_calc.h"
#include "qrys.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "events.h"
#include "transfer.h"
#include "events.h"
#include "astra_misc.h"
#include "term_version.h"
#include "baggage_wt.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;

namespace WeightConcept
{

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
    std::string bag_type;
    TBagNormFilterSets() : use_basic(false),
                           is_trfer(false),
                           only_category(false),
                           check(false) {}
};

int TBagNormInfo::priority() const
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

class TBagNormWideInfo : public TBagNormInfo
{
  public:
    static const int dissimilarity_cost=0;
    std::string airline, city_arv, pax_cat, subcl, cl, craft;
    int pr_trfer, flt_no;
    TBagNormWideInfo()
    {
      clear();
    }

    void clear()
    {
      TBagNormInfo::clear();
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
    int similarity_cost(const TPaxInfo &pax, const TBagNormFilterSets &filter, const TBagNormFieldAmounts &field_amounts) const;

    static const set<string>& strictly_limited_cats()
    {
      static boost::optional< set<string> > cats;
      if (!cats)
      {
        cats=set<string>();
        cats.get().insert("CHC");
        cats.get().insert("CHD");
        cats.get().insert("INA");
        cats.get().insert("INF");
      };
      return cats.get();
    }
};

TBagNormWideInfo& TBagNormWideInfo::fromDB(TQuery &Qry)
{
  clear();
  bag_type222=Qry.FieldAsString("bag_type");
  norm_id=Qry.FieldAsInteger("id");
  //norm_trfer не заполняется на данном этапе!
  //handmade не заполняется на данном этапе!
  TNormItem::fromDB(Qry);
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

int TBagNormWideInfo::similarity_cost(const TPaxInfo &pax, const TBagNormFilterSets &filter, const TBagNormFieldAmounts &field_amounts) const
{
  int result=0;
  if (filter.bag_type!=bag_type222) return NoExists;

  if ((filter.use_basic && !airline.empty()) ||
      (!filter.use_basic && airline.empty())) return NoExists;

  if ((filter.is_trfer?pax.final_target:pax.target).empty()) return NoExists;
  if (pr_trfer!=NoExists && (filter.is_trfer?pr_trfer==0:pr_trfer!=0)) return NoExists;
  if (!city_arv.empty())
  {
    if ((filter.is_trfer?pax.final_target:pax.target)!=city_arv) return NoExists;
    result+=field_amounts.city_arv;
  };

  if (filter.only_category && pax.pax_cats.find(pax_cat)==pax.pax_cats.end()) return NoExists;
  if (!pax_cat.empty() &&
      (!filter.check || !pax.pax_cats.empty() || strictly_limited_cats().find(pax_cat)!=strictly_limited_cats().end()))
  {
    if (pax.pax_cats.find(pax_cat)==pax.pax_cats.end()) return NoExists;
    result+=field_amounts.pax_cat;
  };
  if (!subcl.empty())
  {
    if (pax.subcl!=subcl) return NoExists;
    result+=field_amounts.subcl;
  };
  if (!cl.empty())
  {
    if (pax.cl!=cl) return NoExists;
    result+=field_amounts.cl;
  };
  if (flt_no!=NoExists)
    result+=field_amounts.flt_no;
  if (!craft.empty())
    result+=field_amounts.craft;

  return result;
}


class TPaxBagNormTmp
{
  public:
    int similarity_cost, priority;
    TBagNormInfo norm;

    TPaxBagNormTmp()
    {
      similarity_cost=TBagNormWideInfo::dissimilarity_cost;
      priority=TBagNormInfo::min_priority;
    }

    bool operator < (const TPaxBagNormTmp &n) const
    {
      if (similarity_cost!=n.similarity_cost)
        return similarity_cost<n.similarity_cost;
      if (priority!=n.priority)
        return priority>n.priority;
      return (norm.amount!=NoExists && norm.amount>=0?norm.amount:1)*(norm.weight!=NoExists && norm.weight>=0?norm.weight:1)<
             (n.norm.amount!=NoExists && n.norm.amount>=0?n.norm.amount:1)*(n.norm.weight!=NoExists && n.norm.weight>=0?n.norm.weight:1);
    }
};

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

void CheckOrGetPaxBagNorm(const list<TBagNormWideInfo> &trip_bag_norms,
                          const TPaxInfo &pax,
                          const bool use_mixed_norms,
                          const bool only_category,
                          const std::string &bag_type,
                          const TPaxNormItem &norm,
                          TBagNormInfo &result)
{
  result.clear();
  TBagNormFieldAmounts field_amounts;
  TBagNormFilterSets filter;
  filter.use_basic=true;
  filter.only_category=only_category;
  for(list<TBagNormWideInfo>::const_iterator n=trip_bag_norms.begin(); n!=trip_bag_norms.end(); ++n)
   if (!n->airline.empty())
   {
     filter.use_basic=false; //есть хотя бы одна
     break;
   };

  TPaxBagNormTmp curr, max;
  filter.bag_type=bag_type;
  filter.is_trfer=!pax.final_target.empty();
  for(;;filter.is_trfer=false)
  {
    for(list<TBagNormWideInfo>::const_iterator n=trip_bag_norms.begin(); n!=trip_bag_norms.end(); ++n)
    {
      filter.check=(!norm.empty() &&
                    norm.bag_type222==n->bag_type222 &&
                    norm.norm_id==n->norm_id &&
                    norm.norm_trfer==filter.is_trfer);

      curr.similarity_cost=n->similarity_cost(pax, filter, field_amounts);
      if (curr.similarity_cost==NoExists) continue; //норма ну никак не подходит
      curr.priority=n->priority();
      if (curr.priority==NoExists) continue; //норма введена неправильно
      curr.norm=*n;

      if (filter.check)
      {
        //нашли норму и она валидна
        max=curr;
        break;
      }

      if (max.norm.empty() || max<curr) max=curr;
    }
    if (!filter.is_trfer) break;
    if (!max.norm.empty() || !use_mixed_norms) break; //выходим если мы подцепили трансферную норму
    //или мы ее не подцепили, но запрещено использовать в случае оформления трансфера нетрансферные нормы
  };

  if (!max.norm.empty())
  {
    result=max.norm;
    result.norm_trfer=filter.is_trfer;
  };
}

void CheckOrGetPaxBagNorm(const TNormFltInfo &flt,
                          const TPaxInfo &pax,
                          const bool only_category,
                          const std::string &bag_type,
                          const TPaxNormItem &norm,
                          TBagNormInfo &result)
{
  list<TBagNormWideInfo> trip_bag_norms;
  LoadTripBagNorms(flt, trip_bag_norms);
  CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, only_category, bag_type, norm, result);
}

enum TNormsTrferType { nttNone, nttNotTransfer, nttTransfer, nttMixed };

class TNormWideItem : public TNormItem
{
  public:
    int priority;
    TNormWideItem(const TBagNormInfo &norm) : TNormItem(norm)
    {
      priority=norm.priority();
    }
    bool operator < (const TNormWideItem &item) const
    {
      if (priority!=item.priority)
        return priority>item.priority;
      return weight>item.weight;
    }
    bool TryAddNorm(const TNormWideItem &norm);
};

bool TNormWideItem::TryAddNorm(const TNormWideItem &norm)
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
    bool AddNorm(const TBagNormInfo &norm);
    std::string total_view() const;
    std::string paid_calc_view() const;
    std::string norms_view(const bool is_trfer,
                           const std::string& airline_mark,
                           const std::string& lang="") const;
    std::string norms_trfer_view(const std::string& lang="") const;
};

bool TPaidBagWideItem::AddNorm(const TBagNormInfo &norm)
{
  if (bag_type333()!=norm.bag_type222) return false;
  if (norm.norm_type==bntUnknown) return false;
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
    if (n->TryAddNorm(new_norm)) break;
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
      norm_ordinary=!isRegularBagType();
    }
    bool AddNorm(const TBagNormInfo &norm);
};

bool TPaidBagCalcItem::AddNorm(const TBagNormInfo &norm)
{
  bool first_addition=norms.empty();

  if (!TPaidBagWideItem::AddNorm(norm)) return false;

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
typedef std::map< std::string/*bag_type*/, TPaxNormItem > TPaxNormMap;
typedef std::map< TBagTypeListKey, TPaidBagItem > TPaidBagMap;
typedef std::map< TBagTypeListKey, TPaidBagWideItem > TPaidBagWideMap;
typedef std::map< TBagTypeListKey, TPaidBagCalcItem > TPaidBagCalcMap;

//декларация расчета багажа - на выходе TPaidBagCalcItem - самая нижняя процедура
void CalcPaidBagBase(const TAirlines &airlines,
                     const std::map<int, TEventsBagItem> &bag,
                     const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                     TPaidBagCalcMap &paid);

enum TPrepareSimpleBagType1 { psbtOnlyTrfer, psbtOnlyNotTrfer, psbtTrferAndNotTrfer };
enum TPrepareSimpleBagType2 { psbtOnlyRefused, psbtOnlyNotRefused, psbtRefusedAndNotRefused };

void PrepareSimpleBag(const std::map<int/*id*/, TEventsBagItem> &bag,
                      const TPrepareSimpleBagType1 trfer_type,
                      const TPrepareSimpleBagType2 refused_type,
                      TSimpleBagMap &bag_simple)
{
  bag_simple.clear();
  for(std::map<int/*id*/, TEventsBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
  {
    if ((refused_type==psbtOnlyRefused && !b->second.refused) ||
        (refused_type==psbtOnlyNotRefused && b->second.refused)) continue;
    if ((trfer_type==psbtOnlyTrfer && !b->second.is_trfer) ||
        (trfer_type==psbtOnlyNotTrfer && b->second.is_trfer)) continue;
    if (!b->second.wt) continue;
    const TBagTypeListKey& key=b->second.wt.get().key();
    TSimpleBagMap::iterator i=bag_simple.find(key);
    if (i==bag_simple.end())
      i=bag_simple.insert(make_pair(key, std::list<TSimpleBagItem>())).first;
    if (i==bag_simple.end()) throw Exception("%s: i==bag_simple.end()", __FUNCTION__);
    i->second.push_back(TSimpleBagItem(key, b->second.amount, b->second.weight));
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
      << "handmade=" << (item.handmade==NoExists?"NoExists":IntToString(item.handmade));
    ProgTrace(TRACE5, "%s: %s", where.c_str(), s.str().c_str());
  }
}

string NormsTraceStr(const std::list<TBagNormInfo> &norms)
{
  ostringstream s;
  for(list<TBagNormInfo>::const_iterator n=norms.begin(); n!=norms.end(); ++n)
  {
    if (n!=norms.begin()) s << "; ";
    s << n->str(AstraLocale::LANG_EN);
  }
  return s.str();
}

string NormsTraceStr(const TPaxNormMap &norms)
{
  ostringstream s;
  for(TPaxNormMap::const_iterator n=norms.begin(); n!=norms.end(); ++n)
  {
    if (n!=norms.begin()) s << "; ";
    if (!n->second.bag_type222.empty())
      s << n->second.bag_type222;
    else
      s << "--";
    s << ": ";
    if (n->second.norm_id!=ASTRA::NoExists)
      s << n->second.norm_id;
    else
      s << "NULL";
    s << "/" << (int)n->second.norm_trfer;
    if (n->second.handmade!=0 && n->second.handmade!=NoExists)
      s << "/hand";
    if (n->second.handmade==0)
      s << "/auto";
  }
  return s.str();
}

string PaidTraceStr(const TPaidBagMap &paid)
{
  ostringstream s;
  for(TPaidBagMap::const_iterator p=paid.begin(); p!=paid.end(); ++p)
  {
    if (p!=paid.begin()) s << "; ";
    s << p->second.key().str(LANG_EN) << ": " << p->second.weight;
    if (p->second.rate_id!=ASTRA::NoExists)
      s << "/" << p->second.rate_id;
    else
      s << "/NULL";
    if (p->second.handmade!=0 && p->second.handmade!=NoExists)
      s << "/hand";
    if (p->second.handmade==0)
      s << "/auto";
  }
  return s.str();
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
      s << TPaxInfo::traceStr() << ", "
           "refused=" << (refused?"true":"false") << ", "
           "only_category=" << (only_category()?"true":"false");
      s << endl
        << "prior_norms: " << NormsTraceStr(prior_norms) << endl
        << "curr_norms: " << NormsTraceStr(curr_norms) << endl
        << "result_norms: " << NormsTraceStr(result_norms);
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
  catch(EConvertError)
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
                      const std::map<int/*id*/, TEventsBagItem> &bag,
                      const boost::optional< std::list<TPaxNormItem> > &norms,
                      TPaxNormMap &result)
{
  result.clear();
  if (!norms) return;

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //всегда добавляем обычный багаж
  not_trfer_bag_simple.insert(make_pair(RegularBagType(airlines.single()), std::list<TSimpleBagItem>()));

  for(std::list<TPaxNormItem>::const_iterator n=norms.get().begin(); n!=norms.get().end(); ++n)
  {
    if (not_trfer_bag_simple.find(TBagTypeListKey(n->bag_type222, airlines.single()))==not_trfer_bag_simple.end()) continue; //отфильтровываем нормы

    if (!result.insert(make_pair(n->bag_type222, *n)).second) //!!!vlad
      throw Exception("%s: n->bag_type='%s' already exists in result!", __FUNCTION__, n->bag_type222.c_str());
  };
}

void ConvertNormsList(const TPaxNormMap &norms,
                      std::list<TPaxNormItem> &result)
{
  result.clear();
  for(TPaxNormMap::const_iterator n=norms.begin(); n!=norms.end(); ++n)
    result.push_back(n->second);
}

void SyncHandmadeProp(const TPaxNormMap &src,
                      bool nvl,
                      TPaxNormMap &dest)
{
  for(TPaxNormMap::iterator i=dest.begin(); i!=dest.end(); ++i)
  {
    if (i->second.handmade!=NoExists) continue;
    TPaxNormMap::const_iterator j=src.find(i->first);
    if (j!=src.end() && i->second==j->second)
      i->second.handmade=j->second.handmade;
    else
      i->second.handmade=(int)nvl;
  }
}

//перерасчет багажа - на выходе TPaidBagWideItem
//используется в RecalcPaidBagToDB
void RecalcPaidBagWide(const TAirlines &airlines,
                       const std::map<int/*id*/, TEventsBagItem> &prior_bag,
                       const std::map<int/*id*/, TEventsBagItem> &curr_bag,
                       const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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
      if ((pass==0 && item.isRegularBagType()) ||
          (pass!=0 && !item.isRegularBagType())) continue; //обычный багаж всегда в последней итерации

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
              if (!item.isRegularBagType())
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
                    const map<int/*id*/, TEventsBagItem> &curr_bag,
                    const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                    const std::vector<CheckIn::TTransferItem> &trfer,
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
      for(CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin(); pCurr!=curr_paxs.end(); ++pCurr)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.setCategory(true, *pCurr, TPaxToLogInfo());
        pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
        pax.subcl=pCurr->pax.subcl;
        pax.refused=!pCurr->pax.refuse.empty();
        ConvertNormsList(airlines, curr_bag, pCurr->norms, pax.curr_norms);

        int pax_id=(pCurr->pax.id==NoExists?pCurr->generated_pax_id:pCurr->pax.id);
        if (pax_id==NoExists) throw Exception("%s: pax_id==NoExists!", __FUNCTION__);
        if (!paxs.insert(make_pair(pax_id, pax)).second)
          throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax_id);
        if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      };
    }
    else
    {
      //запись изменений
      for(map<TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator pPrior=prior_paxs.begin(); pPrior!=prior_paxs.end(); ++pPrior)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.setCategory(false, CheckIn::TPaxListItem(), pPrior->second);
        pax.cl=grp.status!=psCrew?pPrior->second.cl:EncodeClass(Y);
        pax.subcl=pPrior->second.subcl;
        pax.refused=!pPrior->second.refuse.empty();
        std::list<TPaxNormItem> tmp_norms;
        for(std::list< std::pair<TPaxNormItem, TNormItem> >::const_iterator n=pPrior->second.norms.begin();
                                                                                              n!=pPrior->second.norms.end(); ++n)
          tmp_norms.push_back(n->first);
        ConvertNormsList(airlines, curr_bag, tmp_norms, pax.prior_norms);

        CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin();
        for(; pCurr!=curr_paxs.end(); ++pCurr)
        {
          if (pCurr->pax.id==NoExists) throw Exception("%s: pCurr->pax.id==NoExists!", __FUNCTION__);
          if (pCurr->pax.tid==NoExists) throw Exception("%s: pCurr->pax.tid==NoExists!", __FUNCTION__);
          if (pCurr->pax.id==pPrior->first.pax_id) break;
        }
        if (pCurr!=curr_paxs.end())
        {
          //пассажир менялся
          if (pCurr->pax.PaxUpdatesPending)
          {
            pax.setCategory(false, *pCurr, pPrior->second);
            pax.cl=grp.status!=psCrew?grp.cl:EncodeClass(Y);
            pax.subcl=pCurr->pax.subcl;
            pax.refused=!pCurr->pax.refuse.empty();
          };
          ConvertNormsList(airlines, curr_bag, pCurr->norms, pax.curr_norms);

          if (!paxs.insert(make_pair(pCurr->pax.id, pax)).second)
            throw Exception("%s: pCurr->pax.id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
          if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());

        }
        else
        {
          //пассажир не менялся
          if (!paxs.insert(make_pair(pPrior->first.pax_id, pax)).second)
            throw Exception("%s: pPrior->first.pax_id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
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

      std::list<TPaxNormItem> tmp_norms;
      for(std::list< std::pair<TPaxNormItem, TNormItem> >::const_iterator n=prior_paxs.begin()->second.norms.begin();
                                                                                            n!=prior_paxs.begin()->second.norms.end(); ++n)
        tmp_norms.push_back(n->first);
      ConvertNormsList(airlines, curr_bag, tmp_norms, pax.prior_norms);
    };
    paxs.insert(make_pair(grp.id, pax));
    if (use_traces) ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
  };
}

//валидация норм
//на входе paxs.prior_norms paxs.curr_norms
//на выходе paxs.result_norms и общий список всех норм неразрегистрированных пассажиров
//используется в test_norms и RecalcpaidBagToDB
void CheckOrGetWidePaxNorms(const TAirlines &airlines,
                            const list<TBagNormWideInfo> &trip_bag_norms,
                            const map<int/*id*/, TEventsBagItem> &curr_bag,
                            const TNormFltInfo &flt,
                            bool pr_unaccomp,
                            bool use_traces,
                            map<int/*pax_id*/, TWidePaxInfo> &paxs,
                            list<TBagNormInfo> &all_norms)
{
  all_norms.clear();

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(curr_bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //всегда добавляем обычный багаж
  not_trfer_bag_simple.insert(make_pair(RegularBagType(airlines.single()), std::list<TSimpleBagItem>()));

  for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    TWidePaxInfo &pax=p->second;

    SyncHandmadeProp(pax.prior_norms, true, pax.curr_norms);

    for(TSimpleBagMap::const_iterator i=not_trfer_bag_simple.begin(); i!=not_trfer_bag_simple.end(); ++i)
    {
      TBagNormInfo result;
      const TBagTypeListKey& key=i->first;
      try
      {
        for(int pass=0; pass<2; pass++)
        {
          //1 проход: новые нормы
          //2 проход: старые нормы
          TPaxNormMap &norms=pass==0?pax.curr_norms:pax.prior_norms;
          TPaxNormMap::const_iterator n=norms.find(key.bag_type); //!!!vlad
          if (n!=norms.end())
          {
            if (n->second.norm_id==NoExists && n->second.handmade!=NoExists && n->second.handmade!=0)
            {
              result.handmade=n->second.handmade;
              throw 1;  //вручную удаленная норма
            }
            if (n->second.norm_id!=NoExists)
            {
              CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category(), key.bag_type, n->second, result); //!!!vlad
              if (!result.empty() && n->second==result)
              {
                result.handmade=n->second.handmade;
                throw 1;
              };
            };
          };
        };
        //3 проход: отсутствующие нормы
        CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category(), key.bag_type, TBagNormInfo(), result);
      }
      catch(int) {};

      if (result.empty()) result.bag_type222=key.bag_type;
      pax.result_norms.insert(make_pair(key.bag_type, result));
      //здесь имеем сформированный result
      if (!pax.refused) all_norms.push_back(result); //добавляем нормы неразрегистрированных пассажиров
    };

    SyncHandmadeProp(TPaxNormMap(), false, pax.result_norms);

    if (use_traces)
    {
      if (!pr_unaccomp)
        ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      else
        ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
    };
  };
}

//перерасчет багажа с валидацией норм и записью в таблицы grp_norms, pax_norms и paid_bag
//применяется при записи изменений по группе пассажиров (SavePax)
void RecalcPaidBagToDB(const TAirlines &airlines,
                       const map<int/*id*/, TEventsBagItem> &prior_bag, //TEventsBagItem а не CheckIn::TBagItem потому что есть refused
                       const map<int/*id*/, TEventsBagItem> &curr_bag,
                       const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                       const TNormFltInfo &flt,
                       const std::vector<CheckIn::TTransferItem> &trfer,
                       const CheckIn::TPaxGrpItem &grp,
                       const CheckIn::TPaxList &curr_paxs,
                       const TPaidBagList &prior_paid,
                       bool pr_unaccomp,
                       bool use_traces,
                       TPaidBagList &result_paid)
{
   result_paid.clear();

   if (use_traces) ProgTrace(TRACE5, "%s: flt:%s", __FUNCTION__, flt.traceStr().c_str());

   map<int/*pax_id*/, TWidePaxInfo> paxs;
   GetWidePaxInfo(airlines, curr_bag, prior_paxs, trfer, grp, curr_paxs, pr_unaccomp, use_traces, paxs);

   //расчет/проверка багажных норм
   list<TBagNormWideInfo> trip_bag_norms;
   LoadTripBagNorms(flt, trip_bag_norms);

   std::list<TBagNormInfo> all_norms;
   CheckOrGetWidePaxNorms(airlines, trip_bag_norms, curr_bag, flt, pr_unaccomp, use_traces, paxs, all_norms);

   //собственно расчет платного багажа
   TPaidBagWideMap paid_wide;
   RecalcPaidBagWide(airlines, prior_bag, curr_bag, all_norms, prior_paid, grp.paid?grp.paid.get():TPaidBagList(), paid_wide);

   for(TPaidBagWideMap::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
   {
     const TPaidBagWideItem &item=i->second;
     if (item.weight==NoExists)
     {
       if (!item.isRegularBagType())
         throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_BAG_TYPE",
                             LParams() << LParam("bagtype", item.key().str()));
       else
         throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_ORDINARY");
     }
     result_paid.push_back(item);
   };

   PaidBagToDB(grp.id, pr_unaccomp, result_paid);

   for(map<int/*pax_id*/, TWidePaxInfo>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
   {
     const TWidePaxInfo &pax=p->second;
     if (pax.prior_norms==pax.result_norms) continue;//не было изменения норм у пассжира

     std::list<TPaxNormItem> norms;
     ConvertNormsList(pax.result_norms, norms);
     if (!pr_unaccomp)
       PaxNormsToDB(p->first, norms);
     else
       GrpNormsToDB(p->first, norms);
   }
};

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
      std::vector<CheckIn::TTransferItem> trfer;
      GetGrpToLogInfo(grp.id, grpLogInfo);
      CheckIn::LoadTransfer(grp.id, trfer);

      if (!test_paid)
      {
        TSimpleBagMap not_trfer_bag_simple;
        PrepareSimpleBag(grpLogInfo.bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
        //всегда добавляем обычный багаж
        not_trfer_bag_simple.insert(make_pair(RegularBagType(airlines.single()), std::list<TSimpleBagItem>()));


        map<int/*pax_id*/, TWidePaxInfo> paxs;
        GetWidePaxInfo(airlines, grpLogInfo.bag, grpLogInfo.pax, trfer, grp, CheckIn::TPaxList(), grp.is_unaccomp(), false, paxs);

        map<int/*pax_id*/, TWidePaxInfo> tmp_paxs=paxs;
        for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=tmp_paxs.begin(); p!=tmp_paxs.end(); ++p)
        {
          p->second.prior_norms.clear();
          p->second.curr_norms.clear();
        };

        set<int> error_ids;
        list<TBagNormInfo> all_norms;
        for(int pass=0; pass<2; pass++)
        {
          //1 проход проверяет что все нормы которые привязаны на реальном сервере разрешены новым алгоритмом
          //2 проход проверяет что все нормы которые привязаны на реальном сервере автоматически рассчитываются новым алгоритмом

          CheckOrGetWidePaxNorms(airlines, trip_bag_norms, grpLogInfo.bag, flt, grp.is_unaccomp(), false, (pass==0?paxs:tmp_paxs), all_norms);

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
              if (i1->first!=i2->first ||
                  i1->second.bag_type222!=i2->second.bag_type222 ||
                  i1->second.norm_id!=i2->second.norm_id) break;
            };
            if (i1==pax.prior_norms.end() && i2==pax.result_norms.end()) continue;

            if (/*client_type!=ctTerm &&*/ not_trfer_bag_simple.size()==1 && not_trfer_bag_simple.begin()->first==RegularBagType(airlines.single()) &&
                pax.prior_norms.empty() && pax.result_norms.size()==1 && pax.result_norms.begin()->first==REGULAR_BAG_TYPE) continue;

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
        PrepareSimpleBag(grpLogInfo.bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, bag_simple);
        if (!bag_simple.empty())
        {
          TPaidBagList prior_paid;
          PaidBagFromDB(NoExists, grp.id, prior_paid);

          bool norms_incomplete=false;
          std::list<TBagNormInfo> all_norms;
          for(std::map<TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator p=grpLogInfo.pax.begin(); p!=grpLogInfo.pax.end(); ++p)
          {
            if (!p->second.refuse.empty()) continue;
            TPaxNormMap norms_map;
            for(list< pair<TPaxNormItem, TNormItem> >::const_iterator n=p->second.norms.begin(); n!=p->second.norms.end(); ++n)
            {
              norms_map.insert(make_pair(n->first.bag_type222,n->first));
              all_norms.push_back(TBagNormInfo(*n));
            };
            TSimpleBagMap::const_iterator b=bag_simple.begin();
            for(; b!=bag_simple.end(); ++b)
              if (norms_map.find(b->first.bag_type)==norms_map.end())
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
            RecalcPaidBagWide(airlines, grpLogInfo.bag, grpLogInfo.bag, all_norms, prior_paid, TPaidBagList(), paid_wide, true);

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
                trace << "prior_paid: " << PaidTraceStr(sorted_prior_paid) << endl
                      << "calc_paid: " << PaidTraceStr(sorted_calc_paid);
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
                     const std::map<int/*id*/, TEventsBagItem> &bag,
                     const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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

  ProgTrace(TRACE5, "%s: %s", __FUNCTION__, NormsTraceStr(norms).c_str());
  TracePaidBagWide(paid_wide, __FUNCTION__);
}

void CalcTrferBagWide(const std::map<int/*id*/, TEventsBagItem> &bag,
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
                     const std::map<int/*id*/, TEventsBagItem> &bag,
                     const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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
  for(list<TBagNormInfo>::const_iterator k=norms.begin(); k!=norms.end(); ++k)
  {
    TPaidBagCalcMap::iterator i=paid.find(TBagTypeListKey(k->bag_type222, airlines.single()));
    if (i==paid.end()) continue;
    i->second.AddNorm(*k);
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
      if ((pass==0 && item.isRegularBagType()) ||
          (pass!=0 && !item.isRegularBagType())) continue; //обычный багаж всегда в последней итерации

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
                     const std::map<int/*id*/, TEventsBagItem> &bag,
                     const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
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
        for(std::map<int/*id*/, TEventsBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
          if (b->second.is_trfer==is_trfer && b->second.wt && b->second.wt.get().key()==item.key())
          {
            item.list_id=b->second.wt.get().list_id;
            item.list_item=b->second.wt.get().list_item;
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
      catch(EBaseTableError) {};
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

bool UpdatePaidBag(const CheckIn::TServicePaymentItem &emd,
                   TPaidRFISCList &paid_rfisc,
                   bool only_check)
{
  if (!emd.isEMD() ||
      !emd.pc ||
      !emd.service_quantity_valid()) return false;
  TPaxSegRFISCKey key(Sirena::TPaxSegKey(emd.pax_id, emd.trfer_num), emd.pc.get());
  TPaidRFISCList::iterator p=paid_rfisc.find(key);
  if (p!=paid_rfisc.end() && p->second.need_positive())
  {
    TPaidRFISCItem& item=p->second;
    if (!only_check)
    {
      item.need-=emd.service_quantity;
      if (item.need<0) item.need=0;
    };
    return true;
  };
  return false;
}

bool Confirmed(const CheckIn::TServicePaymentItem &emd,
               const boost::optional< list<TEMDCtxtItem> > &confirmed_emd)
{
  if (!emd.isEMD() ||
      !emd.pc ||
      !emd.service_quantity_valid()) return false;
  if (!confirmed_emd) return true;
  if (emd.trfer_num!=0) return true;
  for(list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    if (emd.doc_no==i->emd.no &&
        emd.doc_coupon==i->emd.coupon &&
        emd.pax_id==i->pax.id) return true;
  return false;
}

bool Confirmed(const TGrpServiceAutoItem &emd,
               const boost::optional< list<TEMDCtxtItem> > &confirmed_emd)
{
  if (!emd.isEMD() ||
      !emd.service_quantity_valid()) return false;
  if (!confirmed_emd) return true;
  if (emd.trfer_num!=0) return true;
  for(list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    if (emd.emd_no==i->emd.no &&
        emd.emd_coupon==i->emd.coupon &&
        emd.pax_id==i->pax.id) return true;
  return false;
}

class TQuantityPerSeg : public map<int/*trfer_num*/, int/*кол-во*/>
{
  public:
    void add(int trfer_num, int quantity)
    {
      TQuantityPerSeg::iterator i=insert(make_pair(trfer_num, 0)).first;
      if (i==end()) throw Exception("%s: i==TQuantityPerSeg.end()", __FUNCTION__);
      i->second+=quantity;
    }
    void remove(int trfer_num, int quantity)
    {
      TQuantityPerSeg::iterator i=find(trfer_num);
      if (i!=end())
      {
        i->second-=quantity;
        if (i->second<=0) erase(i);
      };
    }
    string traceStr() const
    {
      ostringstream s;
      for(TQuantityPerSeg::const_iterator i=begin(); i!=end(); ++i)
      {
        if (i!=begin()) s << ", ";
        s << i->first << ":" << i->second;
      }
      return s.str();
    }
};

bool TryEnlargeServicePayment(TPaidRFISCList &paid_rfisc,
                              CheckIn::TServicePaymentList &payment,
                              const TGrpServiceAutoList &svcsAuto,
                              const TCkinGrpIds &tckin_grp_ids,
                              const CheckIn::TGrpEMDProps &emdProps,
                              const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd)
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  if (confirmed_emd)
  {
    ostringstream s;
    for(std::list<TEMDCtxtItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    {
      if (i!=confirmed_emd.get().begin()) s << ", ";
      s << i->no_str();
    }
    ProgTrace(TRACE5, "%s: confirmed_emd=%s", __FUNCTION__, s.str().c_str());
  };


  bool result=false;

  class TAddedEmdItem
  {
    public:
      string rfisc;
      string emd_no_base;
      int continuous_segs;
      bool manual_bind;
      list<CheckIn::TServicePaymentItem> coupons;

      TAddedEmdItem(): continuous_segs(0), manual_bind(false) {}
      TAddedEmdItem(const string& _rfisc, const string& _emd_no_base):
        rfisc(_rfisc), emd_no_base(_emd_no_base), continuous_segs(0), manual_bind(false) {}

      bool operator < (const TAddedEmdItem &item) const
      {
        if (continuous_segs!=item.continuous_segs)
          return continuous_segs<item.continuous_segs;
        if (manual_bind!=item.manual_bind)
          return manual_bind;
        return coupons.size()>item.coupons.size();
      }
      bool empty() const
      {
        return rfisc.empty() || emd_no_base.empty() || continuous_segs==0 || coupons.empty();
      }
      string traceStr() const
      {
        ostringstream s;
        s << "rfisc=" << rfisc << ", "
             "emd_no_base=" << emd_no_base << ", "
             "continuous_segs=" << continuous_segs << ", "
             "manual_bind=" << (manual_bind?"true":"false") << ", "
             "coupons=";
        for(list<CheckIn::TServicePaymentItem>::const_iterator i=coupons.begin(); i!=coupons.end(); ++i)
        {
          if (i!=coupons.begin()) s << ", ";
          s << i->no_str();
        };
        return s.str();
      }
  };

  class TBaseEmdMap : public map< string/*emd_no_base*/, set<string/*emd_no*/> >
  {
    public:
      string rfisc;
      TBaseEmdMap(const string& _rfisc) : rfisc(_rfisc) {}
      void add(const TPaxEMDItem& emd)
      {
        if (emd.RFISC!=rfisc) return;
        string emd_no_base=emd.emd_no_base.empty()?emd.emd_no:emd.emd_no_base;
        pair< TBaseEmdMap::iterator, bool > res=insert(make_pair(emd_no_base, set<string>()));
        if (res.first==end()) throw Exception("%s: res.first==end()!", __FUNCTION__);
        res.first->second.insert(emd.emd_no);
      }
      string traceStr() const
      {
        ostringstream s;
        for(TBaseEmdMap::const_iterator i=begin(); i!=end(); ++i)
        {
          if (i!=begin()) s << ", ";
          for(set<string>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
          {
            if (j!=i->second.begin()) s << "/";
            s << *j;
          }
        }
        return s.str();
      }
  };

  class TNeedMap : public map<int/*pax_id*/, map<TRFISCListKey, TQuantityPerSeg> >
  {
    public:
      void add(const TPaidRFISCItem &item)
      {
        if (!item.need_positive()) return;
        TNeedMap::iterator i=find(item.pax_id);
        if (i==end())
          i=insert(make_pair(item.pax_id, map<TRFISCListKey, TQuantityPerSeg>())).first;
        if (i==end()) throw Exception("%s: i==TNeedMap.end()!", __FUNCTION__);
        map<TRFISCListKey, TQuantityPerSeg>::iterator j=i->second.insert(make_pair(item, TQuantityPerSeg())).first;
        if (j==i->second.end()) throw Exception("%s: j==i->second.end()!", __FUNCTION__);
        j->second.add(item.trfer_num, item.need);
      }

      void dump(const string &where) const
      {
        ProgTrace(TRACE5, "%s: === TNeedMap dump ===", where.c_str());
        for(TNeedMap::const_iterator i=begin(); i!=end(); ++i)
        {
          ProgTrace(TRACE5, "%s: pax_id=%d", where.c_str(), i->first);
          for(map<TRFISCListKey, TQuantityPerSeg >::const_iterator j=i->second.begin();
                                                                   j!=i->second.end(); ++j)
          {
            ProgTrace(TRACE5, "%s:     rfisc=%s", where.c_str(), j->first.str(LANG_EN).c_str());
            for(TQuantityPerSeg::const_iterator k=j->second.begin();
                                                k!=j->second.end(); ++k)
              ProgTrace(TRACE5, "%s:         trfer_num=%d, quantity=%d", where.c_str(), k->first, k->second);
          }
        }
      }
  };

  TNeedMap need;
  int max_trfer_num=0;
  for(TPaidRFISCList::iterator p=paid_rfisc.begin(); p!=paid_rfisc.end(); ++p)
  {
    const TPaidRFISCItem& item=p->second;
    if (max_trfer_num<item.trfer_num) max_trfer_num=item.trfer_num;
    need.add(item);
  };

  //need.dump(__FUNCTION__);

  for(TNeedMap::iterator i=need.begin(); i!=need.end(); ++i)
  {
    TPaxEMDList emds;
    emds.getAllPaxEMD(i->first, tckin_grp_ids.size()==1);
    for(const TGrpServiceAutoItem& svcAuto : svcsAuto)
      if (svcAuto.withEMD())
        emds.erase(TPaxEMDItem(svcAuto)); //удаляем уже автопривязанные

    if (emds.empty()) continue; //по пассажиру нет ничего
    for(map<TRFISCListKey, TQuantityPerSeg>::iterator irfisc=i->second.begin(); irfisc!=i->second.end(); ++irfisc)
    {
      TBaseEmdMap base_emds(irfisc->first.RFISC);
      for(multiset<TPaxEMDItem>::const_iterator e=emds.begin(); e!=emds.end(); ++e) base_emds.add(*e);

      ProgTrace(TRACE5, "%s: pax_id=%d, rfisc=%s(%s), emds: %s",
                __FUNCTION__,
                i->first,
                irfisc->first.str(LANG_EN).c_str(),
                irfisc->second.traceStr().c_str(),
                base_emds.traceStr().c_str());

      for(int initial_trfer_num=0; initial_trfer_num<=max_trfer_num && !irfisc->second.empty(); initial_trfer_num++)
      {
        for(;!irfisc->second.empty();)
        {
          TAddedEmdItem best_added;

          for(TBaseEmdMap::const_iterator be=base_emds.begin(); be!=base_emds.end(); ++be)
          {
            TAddedEmdItem curr_added(base_emds.rfisc, be->first);

            for(;curr_added.continuous_segs<=max_trfer_num;curr_added.continuous_segs++)
            {
              {
                //ищем среди привязанных
                CheckIn::TServicePaymentList::const_iterator p=payment.begin();
                for(; p!=payment.end(); ++p)
                  if (p->isEMD() &&
                      p->pc &&
                      p->pc.get().RFISC==curr_added.rfisc &&
                      p->trfer_num==curr_added.continuous_segs &&
                      p->pax_id==i->first &&
                      be->second.find(p->doc_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && p==payment.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && p!=payment.end()) continue;
              }
              {
                //ищем среди непривязанных
                multiset<TPaxEMDItem>::const_iterator e=emds.begin();
                for(; e!=emds.end(); ++e)
                  if (e->service_quantity_valid() &&
                      e->RFISC==curr_added.rfisc &&
                      e->trfer_num==curr_added.continuous_segs &&
                      be->second.find(e->emd_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && e==emds.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && e!=emds.end())
                {
                  boost::optional<TRFISCKey> emdKey=paid_rfisc.getKeyIfSingleRFISC(i->first, e->trfer_num, irfisc->first.RFISC);
                  if (emdKey)  //!emdKey - у пассажира как минимум 2 разных услуги с одним RFISC на одном сегменте и мы не можем определить к какой услуге относится ASVC или EMD
                  {
                    CheckIn::TServicePaymentItem item;
                    item.doc_type="EMD"+e->emd_type;
                    item.doc_no=e->emd_no;
                    item.doc_coupon=e->emd_coupon;
                    item.service_quantity=e->service_quantity;
                    item.pax_id=i->first;
                    item.trfer_num=e->trfer_num;
                    item.pc=emdKey.get();

                    if (Confirmed(item, confirmed_emd) &&
                        UpdatePaidBag(item, paid_rfisc, true))
                    {
                      if (emdProps.get(e->emd_no, e->emd_coupon).get_manual_bind())
                        curr_added.manual_bind=true;
                      else
                      {
                        curr_added.coupons.push_back(item);
                        continue;
                      }
                    }
                  }
                }
              }
              break; //сюда дошли - значит к данному сегменту не можем привязать
            }

            if (curr_added.coupons.empty()) continue;
            if (best_added<curr_added) best_added=curr_added;
          }; //emd_no

          if (best_added.empty()) break; //ни одного EMD из непривязанных не можем привязать по текущему RFISC

          ProgTrace(TRACE5, "%s: pax_id=%d, %s", __FUNCTION__, i->first, best_added.traceStr().c_str());

          for(list<CheckIn::TServicePaymentItem>::const_iterator p=best_added.coupons.begin(); p!=best_added.coupons.end(); ++p)
          {
            if (!UpdatePaidBag(*p, paid_rfisc, false)) throw Exception("%s: UpdatePaidBag strange situation!", __FUNCTION__);
            payment.push_back(*p);
            if (p->service_quantity_valid())
              irfisc->second.remove(p->trfer_num, p->service_quantity);
            result=true;
          }
          base_emds.erase(best_added.emd_no_base);
        }
      }; //initial_trfer_num
    } //rfisc
  }

  return result;
}

bool TryCheckinServicesAuto(TGrpServiceAutoList &svcsAuto,
                            const CheckIn::TServicePaymentList &payment,
                            const TCkinGrpIds &tckin_grp_ids,
                            const CheckIn::TGrpEMDProps &emdProps,
                            const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd)
{

  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  bool result=false;

  TPaxEMDList emds;
  emds.getAllEMD(tckin_grp_ids);

  vector< pair< int/*grp_id*/, boost::optional<TTripInfo> > > flts;
  for(const int& grp_id : tckin_grp_ids)
    flts.emplace_back(grp_id, boost::none);

  //попробуем автоматически зарегистрировать
  for(const TPaxEMDItem& emd : emds)
  {
    TGrpServiceAutoItem svcAuto(Sirena::TPaxSegKey(emd.pax_id, emd.trfer_num), emd);

    if (!svcAuto.isSuitableForAutoCheckin()) continue;
    if (svcsAuto.sameDocExists(svcAuto)) continue;
    if (payment.sameDocExists(svcAuto)) continue;
    if (emdProps.get(emd.emd_no, emd.emd_coupon).get_not_auto_checkin()) continue;
    if (!Confirmed(svcAuto, confirmed_emd)) continue;
    if (svcAuto.trfer_num<0 || svcAuto.trfer_num>=(int)flts.size()) continue;
    auto& flt=flts.at(svcAuto.trfer_num);
    if (!flt.second)
    {
      flt.second=boost::in_place();
      flt.second.get().getByGrpId(flt.first);
    }
    if (!svcAuto.permittedForAutoCheckin(flt.second.get())) continue;
    svcsAuto.push_back(svcAuto);
    result=true;
  }

  TGrpServiceAutoList asvcWithoutEMD;
  PaxASVCList::getWithoutEMD(tckin_grp_ids.front(), asvcWithoutEMD, false);
  //формируем новый список с подходящими
  TGrpServiceAutoList actualWithoutEMD;
  for(const TGrpServiceAutoItem& svcAuto : asvcWithoutEMD)
  {
    if (!svcAuto.isSuitableForAutoCheckin()) continue;
    if (svcAuto.trfer_num<0 || svcAuto.trfer_num>=(int)flts.size()) continue;
    auto& flt=flts.at(svcAuto.trfer_num);
    if (!flt.second)
    {
      flt.second=boost::in_place();
      flt.second.get().getByGrpId(flt.first);
    }
    if (!svcAuto.permittedForAutoCheckin(flt.second.get())) continue;

    actualWithoutEMD.push_back(svcAuto);

    if (!svcsAuto.removeEqualWithoutEMD(svcAuto)) result=true;
  }

  if (any_of(svcsAuto.begin(), svcsAuto.end(), not1(mem_fun_ref(&TGrpServiceAutoItem::withEMD)))) result=true;

  svcsAuto.replaceWithoutEMDFrom(actualWithoutEMD);

  return result;
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



