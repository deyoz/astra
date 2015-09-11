#include "baggage_calc.h"
#include "qrys.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "events.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;

namespace BagPayment
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
    bool use_basic, is_trfer, only_category;
    int bag_type;
    TBagNormFilterSets() : use_basic(false),
                           is_trfer(false),
                           only_category(false),
                           bag_type(NoExists) {}
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
      case bntOrdinary: if (bag_type!=NoExists) return 10; else break;
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
      case bntFreeOrdinary: if (bag_type!=NoExists) return 2; else break;
          case bntFreePaid: return 11;
      case bntOrdinaryPaid: if (bag_type!=NoExists) return 15; else break;
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
      case bntFreeOrdinary: if (bag_type!=NoExists) return 4; else break;
          case bntFreePaid: return 12;
      case bntOrdinaryPaid: if (bag_type!=NoExists) return 16; else break;
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
      case bntFreeOrdinary: if (bag_type!=NoExists) return 6; else break;
          case bntFreePaid: return 13;
      case bntOrdinaryPaid: if (bag_type!=NoExists) return 17; else break;
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
      case bntFreeOrdinary: if (bag_type!=NoExists) return 8; else break;
          case bntFreePaid: return 14;
      case bntOrdinaryPaid: if (bag_type!=NoExists) return 18; else break;
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
};

TBagNormWideInfo& TBagNormWideInfo::fromDB(TQuery &Qry)
{
  clear();
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  norm_id=Qry.FieldAsInteger("id");
  //norm_trfer не заполняется на данном этапе!
  CheckIn::TNormItem::fromDB(Qry);
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
  if (filter.bag_type!=bag_type) return NoExists;

  if ((filter.use_basic && !airline.empty()) ||
      (!filter.use_basic && airline.empty())) return NoExists;

  if ((filter.is_trfer?pax.final_target:pax.target).empty()) return NoExists;
  if (pr_trfer!=NoExists && (filter.is_trfer?pr_trfer==0:pr_trfer!=0)) return NoExists;
  if (!city_arv.empty())
  {
    if ((filter.is_trfer?pax.final_target:pax.target)!=city_arv) return NoExists;
    result+=field_amounts.city_arv;
  };

  if (filter.only_category && pax.pax_cat!=pax_cat) return NoExists;
  if (!pax_cat.empty() && !pax.pax_cat.empty())
  {
    if (pax.pax_cat!=pax_cat) return NoExists;
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
    TCachedQuery CacheTablesQry("SELECT select_sql FROM cache_tables WHERE code='TRIP_BAG_NORMS'", QParams());
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
                          const int bag_type,
                          const CheckIn::TPaxNormItem &norm,
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
      curr.similarity_cost=n->similarity_cost(pax, filter, field_amounts);
      if (curr.similarity_cost==NoExists) continue; //норма ну никак не подходит
      curr.priority=n->priority();
      if (curr.priority==NoExists) continue; //норма введена неправильно
      curr.norm=*n;

      if (!norm.empty() &&
          norm.bag_type==curr.norm.bag_type &&
          norm.norm_id==curr.norm.norm_id &&
          norm.norm_trfer==filter.is_trfer)
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
                          const int bag_type,
                          const CheckIn::TPaxNormItem &norm,
                          TBagNormInfo &result)
{
  list<TBagNormWideInfo> trip_bag_norms;
  LoadTripBagNorms(flt, trip_bag_norms);
  CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, only_category, bag_type, norm, result);
}

string CalcPaxCategory(const CheckIn::TPaxListItem &pax)
{
  if (pax.pax.pers_type==ASTRA::child && pax.pax.seats==0) return "CHC";
  if (pax.pax.pers_type==ASTRA::baby && pax.pax.seats==0) return "INA";
  return "";
}

string CalcPaxCategory(const TPaxToLogInfo &pax)
{
  if (pax.pers_type==EncodePerson(ASTRA::child) && pax.seats==0) return "CHC";
  if (pax.pers_type==EncodePerson(ASTRA::baby) && pax.seats==0) return "INA";
  return "";
}

enum TNormsTrferType { nttNone, nttNotTransfer, nttTransfer, nttMixed };

class TNormWideItem : public CheckIn::TNormItem
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

class TSumBagItem
{
  public:
    int bag_type;
    int amount, weight;
    TSumBagItem(int _bag_type):
      bag_type(_bag_type),
      amount(0),
      weight(0) {}
    TSumBagItem(const CheckIn::TBagItem &item):
      bag_type(item.bag_type),
      amount(item.amount),
      weight(item.weight) {}
    TSumBagItem(const TBagToLogInfo &item):
      bag_type(item.bag_type),
      amount(item.amount),
      weight(item.weight) {}
    bool operator < (const TSumBagItem &item) const
    {
      if (weight!=item.weight)
        return weight>item.weight;
      return amount<item.amount;
    }
    bool operator == (const TSumBagItem &item) const
    {
      return amount==item.amount &&
             weight==item.weight;
    }
};

class TPaidBagWideItem : public CheckIn::TPaidBagItem
{
  public:
    int bag_amount, bag_weight;
    list<TNormWideItem> norms;
    TNormsTrferType norms_trfer;
    int weight_calc;
    TPaidBagWideItem() { clear(); }
    TPaidBagWideItem(int _bag_type)
    {
      clear();
      bag_type=_bag_type;
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
};

bool TPaidBagWideItem::AddNorm(const TBagNormInfo &norm)
{
  if (bag_type!=norm.bag_type) return false;
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

class TPaidBagCalcItem : public TPaidBagWideItem
{
  public:
    list<TSumBagItem> bag;
    bool norm_per_unit, norm_ordinary;
    TPaidBagCalcItem(int _bag_type,
                     const list<TSumBagItem> &_bag) :
      TPaidBagWideItem(_bag_type),
      bag(_bag)
    {
      for(list<TSumBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
      {
        bag_amount+=b->amount;
        bag_weight+=b->weight;
      };
      norm_per_unit=false;
      norm_ordinary=(bag_type!=NoExists);
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

void CalcPaidBag(const std::map<int, TBagToLogInfo> &bag,
                 const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                 map<int/*bag_type*/, TPaidBagCalcItem> &paid);

void PrepareNotRefusedSimpleBag(const std::map<int/*id*/, TBagToLogInfo> &bag,
                                std::map< int/*bag_type*/, std::list<TSumBagItem> > &bag_simple)
{
  bag_simple.clear();
  for(std::map<int/*id*/, TBagToLogInfo>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
  {
    if (b->second.refused) continue;
    std::map< int/*bag_type*/, std::list<TSumBagItem> >::iterator i=bag_simple.find(b->second.bag_type);
    if (i==bag_simple.end())
      i=bag_simple.insert(make_pair(b->second.bag_type, std::list<TSumBagItem>())).first;
    if (i==bag_simple.end()) throw Exception("%s: i==bag_simple.end()", __FUNCTION__);
    i->second.push_back(TSumBagItem(b->second));
  };
};

void TracePaidBagWide(const map<int/*bag_type*/, TPaidBagWideItem> &paid, const string &where)
{
  for(map<int/*bag_type*/, TPaidBagWideItem>::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    const TPaidBagWideItem &item=i->second;

    ProgTrace(TRACE5, "%s: bag_type=%s", where.c_str(), item.bag_type_str().c_str());
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
      << "weight=" << (item.weight==NoExists?"NoExists":IntToString(item.weight));
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

string NormsTraceStr(const map<int/*bag_type*/, CheckIn::TPaxNormItem > &norms)
{
  ostringstream s;
  for(map<int/*bag_type*/, CheckIn::TPaxNormItem >::const_iterator n=norms.begin(); n!=norms.end(); ++n)
  {
    if (n!=norms.begin()) s << "; ";
    if (n->second.bag_type!=ASTRA::NoExists)
      s << setw(2) << setfill('0') << n->second.bag_type;
    else
      s << "NULL";
    s << ": ";
    if (n->second.norm_id!=ASTRA::NoExists)
      s << n->second.norm_id;
    else
      s << "NULL";
  }
  return s.str();
}

class TWidePaxInfo : public TPaxInfo
{
  public:
    bool refused;
    bool only_category;
    map<int/*bag_type*/, CheckIn::TPaxNormItem > prior_norms;
    map<int/*bag_type*/, CheckIn::TPaxNormItem > curr_norms;
    map<int/*bag_type*/, CheckIn::TPaxNormItem > result_norms;

    std::string traceStr() const
    {
      std::ostringstream s;
      s << TPaxInfo::traceStr() << ", "
           "refused=" << (refused?"true":"false") << ", "
           "only_category=" << (only_category?"true":"false");
      s << endl
        << "prior_norms: " << NormsTraceStr(prior_norms) << endl
        << "curr_norms: " << NormsTraceStr(curr_norms) << endl
        << "result_norms: " << NormsTraceStr(result_norms);
      return s.str();
    }
};


void ConvertNormsList(const set<int/*bag_type*/> &bag_types,
                      const boost::optional< std::list<CheckIn::TPaxNormItem> > &norms,
                      map<int/*bag_type*/, CheckIn::TPaxNormItem > &result)
{
  result.clear();
  if (!norms) return;
  for(std::list<CheckIn::TPaxNormItem>::const_iterator n=norms.get().begin(); n!=norms.get().end(); ++n)
  {
    if (bag_types.find(n->bag_type)==bag_types.end()) continue; //отфильтровываем нормы

    if (!result.insert(make_pair(n->bag_type, *n)).second)
      throw Exception("%s: n->bag_type=%s already exists in result!", __FUNCTION__, (n->bag_type==NoExists?"NoExists":IntToString(n->bag_type).c_str()));
  };
}

void ConvertNormsList(const map<int/*bag_type*/, CheckIn::TPaxNormItem > &norms,
                      std::list<CheckIn::TPaxNormItem> &result)
{
  result.clear();
  for(map<int/*bag_type*/, CheckIn::TPaxNormItem >::const_iterator n=norms.begin(); n!=norms.end(); ++n)
    result.push_back(n->second);
}

void RecalcPaidBag(const std::map<int/*id*/, TBagToLogInfo> &prior_bag,
                   const std::map<int/*id*/, TBagToLogInfo> &curr_bag,
                   const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                   const std::list<CheckIn::TPaidBagItem> &paid,
                   map<int/*bag_type*/, TPaidBagWideItem> &paid_wide)
{
  paid_wide.clear();

  std::map< int/*bag_type*/, std::list<TSumBagItem> > prior_bag_simple;
  PrepareNotRefusedSimpleBag(prior_bag, prior_bag_simple);
  std::map< int/*bag_type*/, std::list<TSumBagItem> > curr_bag_simple;
  PrepareNotRefusedSimpleBag(curr_bag, curr_bag_simple);

  map<int/*bag_type*/, TPaidBagCalcItem> paid_calc;
  CalcPaidBag(curr_bag, norms, paid_calc);

  bool ordinaryClearWeight=false;
  map<int/*bag_type*/, TPaidBagCalcItem>::iterator iOrdinary=paid_calc.find(NoExists);
  if (iOrdinary==paid_calc.end()) throw Exception("%s: iOrdinary==paid_calc.end()", __FUNCTION__);
  TPaidBagCalcItem &itemOrdinary=iOrdinary->second;
  for(int pass=0; pass<2; pass++)
  {
    for(map<int/*bag_type*/, TPaidBagCalcItem>::iterator i=paid_calc.begin(); i!=paid_calc.end(); ++i)
    {
      TPaidBagCalcItem &item=i->second;
      if ((pass==0 && item.bag_type==NoExists) ||
          (pass!=0 && item.bag_type!=NoExists)) continue; //обычный багаж всегда в последней итерации

      multiset<TSumBagItem> prior_bag_sorted;
      std::map< int/*bag_type*/, std::list<TSumBagItem> >::const_iterator iPriorBag=prior_bag_simple.find(item.bag_type);
      if (iPriorBag!=prior_bag_simple.end()) prior_bag_sorted.insert(iPriorBag->second.begin(), iPriorBag->second.end());

      multiset<TSumBagItem> curr_bag_sorted;
      std::map< int/*bag_type*/, std::list<TSumBagItem> >::const_iterator iCurrBag=curr_bag_simple.find(item.bag_type);
      if (iCurrBag!=curr_bag_simple.end()) curr_bag_sorted.insert(iCurrBag->second.begin(), iCurrBag->second.end());

      //заполним весом платного багажа, введенным вручную
      item.weight=item.weight_calc;
      for(std::list<CheckIn::TPaidBagItem>::const_iterator j=paid.begin(); j!=paid.end(); ++j)
        if (j->bag_type==item.bag_type)
        {
          item.rate_id=j->rate_id;
          item.rate=j->rate;
          item.rate_cur=j->rate_cur;
          item.rate_trfer=j->rate_trfer;
          if (item.bag_type!=NoExists &&
              curr_bag_sorted!=prior_bag_sorted && item.norm_ordinary)
            ordinaryClearWeight=true;
          //если не было изменений в багаже и расчетный платный отличается от фактического
          if ((item.bag_type!=NoExists || (item.bag_type==NoExists && !ordinaryClearWeight)) &&
              curr_bag_sorted==prior_bag_sorted &&
              j->weight!=NoExists &&
              (item.weight_calc==NoExists || j->weight!=item.weight_calc)) //j->weight!=item.weight_calc - правильно ли это?
            item.weight=j->weight;
          break;
        };

      if (item.norm_per_unit || (item.norm_ordinary && itemOrdinary.norm_per_unit))
      {
        //проверим что багаж заводился ровно по одному месту
        list<TSumBagItem> added_bag;
        set_difference(curr_bag_sorted.begin(), curr_bag_sorted.end(),
                       prior_bag_sorted.begin(), prior_bag_sorted.end(),
                       inserter(added_bag, added_bag.end()));
        for(list<TSumBagItem>::const_iterator b=added_bag.begin(); b!=added_bag.end(); ++b)
          if (b->amount>1)
          {
            if (item.bag_type!=NoExists)
              throw UserException("MSG.LUGGAGE.EACH_SEAT_SHOULD_WEIGHTED_SEPARATELY",
                                  LParams() << LParam("bagtype", item.bag_type_str()));
            else
              throw UserException("MSG.LUGGAGE.EACH_COMMON_SEAT_SHOULD_WEIGHTED_SEPARATELY");
          };
      };
    };
  };

  paid_wide.insert(paid_calc.begin(), paid_calc.end());
}

void RecalcPaidBag(const map<int/*id*/, TBagToLogInfo> &prior_bag, //TBagToLogInfo а не CheckIn::TBagItem потому что есть refused
                   const map<int/*id*/, TBagToLogInfo> &curr_bag,
                   const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                   const TNormFltInfo &flt,
                   const std::vector<CheckIn::TTransferItem> &trfer,
                   const CheckIn::TPaxGrpItem &grp,
                   const CheckIn::TPaxList &curr_paxs,
                   bool pr_unaccomp)
{
   ProgTrace(TRACE5, "%s: flt:%s", __FUNCTION__, flt.traceStr().c_str());

   map<int/*pax_id*/, TWidePaxInfo> paxs;

   string target=((TAirpsRow&)base_tables.get("airps").get_row("code",grp.airp_arv,true)).city;
   string final_target;
   if (!trfer.empty())
     final_target=((TAirpsRow&)base_tables.get("airps").get_row("code",trfer.back().airp_arv,true)).city;

   set<int> bag_types;
   for(map<int/*id*/, TBagToLogInfo>::const_iterator b=curr_bag.begin(); b!=curr_bag.end(); ++b)
     bag_types.insert(b->second.bag_type);
   bag_types.insert(NoExists); //всегда добавляем обычный багаж

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
         pax.pax_cat=CalcPaxCategory(*pCurr);
         pax.cl=grp.cl;
         pax.subcl=pCurr->pax.subcl;
         pax.refused=!pCurr->pax.refuse.empty();
         pax.only_category=!pax.pax_cat.empty();
         ConvertNormsList(bag_types, pCurr->norms, pax.curr_norms);

         int pax_id=(pCurr->pax.id==NoExists?pCurr->generated_pax_id:pCurr->pax.id);
         if (pax_id==NoExists) throw Exception("%s: pax_id==NoExists!", __FUNCTION__);
         if (!paxs.insert(make_pair(pax_id, pax)).second)
           throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax_id);
         ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
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
         pax.pax_cat=CalcPaxCategory(pPrior->second);
         pax.cl=pPrior->second.cl;
         pax.subcl=pPrior->second.subcl;
         pax.refused=!pPrior->second.refuse.empty();
         pax.only_category=!pax.pax_cat.empty();
         std::list<CheckIn::TPaxNormItem> tmp_norms;
         for(std::list< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> >::const_iterator n=pPrior->second.norms.begin();
                                                                                               n!=pPrior->second.norms.end(); ++n)
           tmp_norms.push_back(n->first);
         ConvertNormsList(bag_types, tmp_norms, pax.prior_norms);

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
             pax.pax_cat=CalcPaxCategory(*pCurr);
             pax.cl=grp.cl;
             pax.subcl=pCurr->pax.subcl;
             pax.refused=!pCurr->pax.refuse.empty();
             pax.only_category=!pax.pax_cat.empty();
           };
           ConvertNormsList(bag_types, pCurr->norms, pax.curr_norms);

           if (!paxs.insert(make_pair(pCurr->pax.id, pax)).second)
             throw Exception("%s: pCurr->pax.id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
           ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());

         }
         else
         {
           //пассажир не менялся
           if (!paxs.insert(make_pair(pPrior->first.pax_id, pax)).second)
             throw Exception("%s: pPrior->first.pax_id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
           ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
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
     pax.only_category=!pax.pax_cat.empty();
     ConvertNormsList(bag_types, grp.norms, pax.curr_norms);
     if (!prior_paxs.empty())
     {
       //запись изменений несопровождаемого багажа
       if (prior_paxs.size()!=1)
         throw Exception("%s: prior_paxs.size()!=1!", __FUNCTION__);

       std::list<CheckIn::TPaxNormItem> tmp_norms;
       for(std::list< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> >::const_iterator n=prior_paxs.begin()->second.norms.begin();
                                                                                             n!=prior_paxs.begin()->second.norms.end(); ++n)
         tmp_norms.push_back(n->first);
       ConvertNormsList(bag_types, tmp_norms, pax.prior_norms);
     };
     paxs.insert(make_pair(grp.id, pax));
     ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
   };

   //расчет/проверка багажных норм
   list<TBagNormWideInfo> trip_bag_norms;
   LoadTripBagNorms(flt, trip_bag_norms);

   std::list<TBagNormInfo> all_norms;
   for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=paxs.begin(); p!=paxs.end(); ++p)
   {
     TWidePaxInfo &pax=p->second;
     for(set<int>::const_iterator bag_type=bag_types.begin(); bag_type!=bag_types.end(); ++bag_type)
     {
       TBagNormInfo result;

       try
       {
         map<int/*bag_type*/, CheckIn::TPaxNormItem >::const_iterator n;
         //1 проход: новые нормы
         if ((n=pax.curr_norms.find(*bag_type))!=pax.curr_norms.end())
         {
           if (n->second.norm_id==NoExists) throw 1;
           CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, *bag_type, n->second, result);
           if (!result.empty() && n->second==result) throw 1;
         }
         //2 проход: старые нормы
         if ((n=pax.prior_norms.find(*bag_type))!=pax.prior_norms.end())
         {
           if (n->second.norm_id==NoExists) throw 1;
           CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, *bag_type, n->second, result);
           if (!result.empty() && n->second==result) throw 1;
         }
         //3 проход: отсутствующие нормы
         CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, *bag_type, TBagNormInfo(), result);
       }
       catch(int) {};

       if (result.empty()) result.bag_type=*bag_type;
       pax.result_norms.insert(make_pair(*bag_type, result));
       //здесь имеем сформированный result
       if (!pax.refused) all_norms.push_back(result); //добавляем нормы неразрегистрированных пассажиров
     };
     if (!pr_unaccomp)
       ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
     else
       ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
   };

   //собственно расчет платного багажа
   map<int/*bag_type*/, TPaidBagWideItem> paid_wide;
   const list<CheckIn::TPaidBagItem> &paid=grp.paid?grp.paid.get():list<CheckIn::TPaidBagItem>();
   RecalcPaidBag(prior_bag, curr_bag, all_norms, paid, paid_wide);

   std::list<CheckIn::TPaidBagItem> result_paid;
   for(map<int/*bag_type*/, TPaidBagWideItem>::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
   {
     const TPaidBagWideItem &item=i->second;
     if (item.weight==NoExists)
     {
       if (item.bag_type!=NoExists)
         throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_BAG_TYPE",
                             LParams() << LParam("bagtype", item.bag_type_str()));
       else
         throw UserException("MSG.PAID_BAG.UNKNOWN_PAID_WEIGHT_ORDINARY");
     }
     result_paid.push_back(item);
   };

   CheckIn::PaidBagToDB(grp.id, result_paid);

   for(map<int/*pax_id*/, TWidePaxInfo>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
   {
     const TWidePaxInfo &pax=p->second;
     if (pax.prior_norms==pax.result_norms) continue;//не было изменения норм у пассжира

     std::list<CheckIn::TPaxNormItem> norms;
     ConvertNormsList(pax.result_norms, norms);
     if (!pr_unaccomp)
       CheckIn::PaxNormsToDB(p->first, norms);
     else
       CheckIn::GrpNormsToDB(p->first, norms);
   }
};

void CalcPaidBag(const std::map<int/*id*/, TBagToLogInfo> &bag,
                 const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                 const std::list<CheckIn::TPaidBagItem> &paid,
                 map<int/*bag_type*/, TPaidBagWideItem> &paid_wide)
{
  paid_wide.clear();

  map<int/*bag_type*/, TPaidBagCalcItem> paid_calc;
  CalcPaidBag(bag, norms, paid_calc);

  for(map<int/*bag_type*/, TPaidBagCalcItem>::const_iterator p=paid_calc.begin(); p!=paid_calc.end(); ++p)
  {
    map<int/*bag_type*/, TPaidBagWideItem>::iterator i=paid_wide.insert(*p).first;
    TPaidBagWideItem &item=i->second;
    item.weight=item.weight_calc;
    for(std::list<CheckIn::TPaidBagItem>::const_iterator j=paid.begin(); j!=paid.end(); ++j)
      if (j->bag_type==item.bag_type)
      {
        item.weight=j->weight;
        break;
      };
  };

  ProgTrace(TRACE5, "%s: %s", __FUNCTION__, NormsTraceStr(norms).c_str());
  TracePaidBagWide(paid_wide, __FUNCTION__);
}

void CalcPaidBag(const std::map<int/*id*/, TBagToLogInfo> &bag,
                 const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                 map<int/*bag_type*/, TPaidBagCalcItem> &paid)
{
  //в платный багаж не попадает привязанный к разрегистрированным пассажирам

  paid.clear();

  std::map< int/*bag_type*/, std::list<TSumBagItem> > bag_simple;
  PrepareNotRefusedSimpleBag(bag, bag_simple);

  for(std::map< int/*bag_type*/, std::list<TSumBagItem> >::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  //в массиве всегда обычный багаж:
  if (paid.find(NoExists)==paid.end())
    paid.insert(make_pair(NoExists, TPaidBagCalcItem(NoExists, std::list<TSumBagItem>())));

  //сложим все нормы:
  for(list<TBagNormInfo>::const_iterator k=norms.begin(); k!=norms.end(); ++k)
  {
    map<int/*bag_type*/, TPaidBagCalcItem>::iterator i=paid.find(k->bag_type);
    if (i==paid.end()) continue;
    i->second.AddNorm(*k);
  };

  bool ordinaryUnknown=false;
  map<int/*bag_type*/, TPaidBagCalcItem>::iterator iOrdinary=paid.find(NoExists);
  if (iOrdinary==paid.end()) throw Exception("%s: iOrdinary==paid.end()", __FUNCTION__);
  TPaidBagCalcItem &itemOrdinary=iOrdinary->second;
  for(int pass=0; pass<2; pass++)
  {
    for(map<int/*bag_type*/, TPaidBagCalcItem>::iterator i=paid.begin(); i!=paid.end(); ++i)
    {
      TPaidBagCalcItem &item=i->second;
      if ((pass==0 && item.bag_type==NoExists) ||
          (pass!=0 && item.bag_type!=NoExists)) continue; //обычный багаж всегда в последней итерации

      if (item.bag_type==NoExists && ordinaryUnknown) item.weight_calc=NoExists;
      else
      {
        //проверим задана хотя бы одна норма
        if (item.norms.empty())
        {
          if (item.bag_type!=NoExists)
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
            for(list<TSumBagItem>::const_iterator j=item.bag.begin(); j!=item.bag.end(); ++j) item.weight_calc+=j->weight;
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
              for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
              {
                bool inc=true;
                switch(norm.norm_type)
                {
                  case bntOrdinary:
                    if (item.bag_type!=NoExists)
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
              for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
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
                      if (item.bag_type!=NoExists)
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
                      if (item.bag_type!=NoExists)
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
              for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();k++)
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
                      if (item.bag_type!=NoExists)
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
                      if (item.bag_type!=NoExists)
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
                for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end(); ++j) k+=j->weight;
                if (k>norm.weight)
                {
                  switch(norm.norm_type)
                  {
                    case bntFreeExcess:
                      item.weight_calc+=k-norm.weight;
                      break;
                    case bntFreeOrdinary:
                      if (item.bag_type!=NoExists)
                      {
                        TSumBagItem sum(item.bag_type);
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
                for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end(); ++j)
                  if (k+j->weight<=norm.weight) k+=j->weight;
                //максимальное кол-во мест
                int l=0;
                for(list<TSumBagItem>::reverse_iterator j=item.bag.rbegin(); j!=item.bag.rend(); ++j)
                  if (l+j->weight<=norm.weight) l+=j->weight;
                if (k>l)
                {
                  k=0;
                  for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
                  {
                    bool inc=true;
                    if (k+j->weight>norm.weight)
                    {
                      if (item.bag_type!=NoExists)
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
                  for(list<TSumBagItem>::reverse_iterator j=item.bag.rbegin(); j!=item.bag.rend();)
                  {
                    bool inc=true;
                    if (l+j->weight>norm.weight)
                    {
                      if (item.bag_type!=NoExists)
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
              for(list<TSumBagItem>::iterator j=item.bag.begin(); j!=item.bag.end();)
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
                        if (item.bag_type!=NoExists)
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
                          if (item.bag_type!=NoExists)
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
                        if (item.bag_type!=NoExists)
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
                        if (item.bag_type!=NoExists)
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
      if (item.bag_type==NoExists && item.weight_calc==NoExists && item.norm_ordinary) ordinaryUnknown=true;

      item.weight=NoExists;
    };
  };

}

void PaidBagViewToXML(const std::map<int/*id*/, TBagToLogInfo> &bag,
                      const std::list<TBagNormInfo> &norms, //вообще список всевозможных норм для всех пассажиров вперемешку
                      const std::list<CheckIn::TPaidBagItem> &paid,
                      const std::list<CheckIn::TPaidBagEMDItem> &emd,
                      const std::string &used_airline_mark,
                      xmlNodePtr node)
{
  map<int/*bag_type*/, TPaidBagWideItem> paid_wide;
  CalcPaidBag(bag, norms, paid, paid_wide);

  if (node==NULL) return;

  node=NewTextChild(node, "paid_bags");

  for(int pass=0; pass<2; pass++)
  {
    for(map<int/*bag_type*/, TPaidBagWideItem>::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
    {
      const TPaidBagWideItem &item=i->second;
      if ((pass==0 && item.bag_type!=NoExists) ||
          (pass!=0 && item.bag_type==NoExists)) continue; //обычный багаж всегда первой строкой

      xmlNodePtr rowNode=NewTextChild(node,"paid_bag");
      if (item.weight==NoExists)
        throw Exception("%s: item.weight==NoExists!", __FUNCTION__);
      item.toXML(rowNode);
      if (item.weight_calc!=NoExists)
        NewTextChild(rowNode, "weight_calc", item.weight_calc);
      else
        NewTextChild(rowNode, "weight_calc");

      ostringstream s;
      if (item.bag_type!=NoExists)
        s << item.bag_type_str() << ": " << ElemIdToNameLong(etBagType, item.bag_type);
      else
        s << getLocaleText("Обычный багаж или р/кладь");
      NewTextChild(rowNode, "bag_type_view", s.str());

      s.str("");
      if (item.bag_amount!=0 || item.bag_weight!=0)
        s << item.bag_amount << "/" << item.bag_weight;
      else
        s << "-";
      NewTextChild(rowNode, "bag_number_view", s.str());

      s.str("");
      if (item.norms.size()==1)
        s << lowerc(item.norms.front().str(TReqInfo::Instance()->desk.lang));
      else
        getLocaleText("см. подробно");
      NewTextChild(rowNode, "norms_view", s.str());

      s.str("");
      switch(item.norms_trfer)
      {
        case nttNone: break;
        case nttNotTransfer: s << getLocaleText("НЕТ"); break;
        case nttTransfer: s << getLocaleText("ДА"); break;
        case nttMixed: s << getLocaleText("СМЕШ"); break;
      };
      NewTextChild(rowNode, "norms_trfer_view", s.str());

      s.str("");
      if (item.weight!=NoExists)
        s << item.weight;
      else
        s << "?";
      NewTextChild(rowNode, "weight_view", s.str());

      s.str("");
      if (item.weight_calc!=NoExists)
        s << item.weight_calc;
      else
        s << "?";
      NewTextChild(rowNode, "weight_calc_view", s.str());

      int emd_weight=0;
      for(list<CheckIn::TPaidBagEMDItem>::const_iterator e=emd.begin(); e!=emd.end(); ++e)
        if (i->second.bag_type==e->bag_type)
        {
          if (e->weight==NoExists)
          {
            emd_weight=NoExists;
            break;
          }
          emd_weight+=e->weight;
        };

      s.str("");
      if (emd_weight!=NoExists)
        s << emd_weight;
      else
        s << "?";
      NewTextChild(rowNode, "emd_weight_view", s.str());


    };

  };


//  const string taLeftJustify="taLeftJustify";  //!!!vlad в astra_consts
//  const string taRightJustify="taRightJustify";
//  const string taCenter="taCenter";

//  node=NewTextChild(node, "paid_bag_view");
//  //описание заголовка
//  xmlNodePtr headerNode=NewTextChild(node, "header");
//  xmlNodePtr colNode;
//  colNode=NewTextChild(headerNode, "col", getLocaleText("Тип багажа"));
//  SetProp(colNode, "width", 130);
//  SetProp(colNode, "align", taLeftJustify);
//  colNode=NewTextChild(headerNode, "col", getLocaleText("Кол."));
//  SetProp(colNode, "width", 35);
//  SetProp(colNode, "align", taCenter);
//  colNode=NewTextChild(headerNode, "col", getLocaleText("Норма"));
//  SetProp(colNode, "width", 80);
//  SetProp(colNode, "align", taLeftJustify);
//  colNode=NewTextChild(headerNode, "col", getLocaleText("Трфр"));
//  SetProp(colNode, "width", 25);
//  SetProp(colNode, "align", taCenter);
//  colNode=NewTextChild(headerNode, "col", getLocaleText("Опл."));
//  SetProp(colNode, "width", 30);
//  SetProp(colNode, "align", taRightJustify);
//  colNode=NewTextChild(headerNode, "col", getLocaleText("EMD"));
//  SetProp(colNode, "width", 30);
//  SetProp(colNode, "align", taRightJustify);


//  xmlNodePtr rowsNode = NewTextChild(node, "rows");
//  for(int pass=0; pass<2; pass++)
//  {
//    for(map<int/*bag_type*/, TPaidBagWideItem>::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
//    {
//      const TPaidBagWideItem &item=i->second;
//      if ((pass==0 && item.bag_type!=NoExists) ||
//          (pass!=0 && item.bag_type==NoExists)) continue; //обычный багаж всегда первой строкой

//      xmlNodePtr rowNode=NewTextChild(rowsNode, "row");
//      ostringstream s;
//      if (item.bag_type!=NoExists)
//        s << item.bag_type_str() << ": " << ElemIdToNameLong(etBagType, item.bag_type);
//      else
//        s << getLocaleText("Обычный багаж или р/кладь");
//      NewTextChild(rowNode, "col", s.str());

//      s.str("");
//      if (item.bag_amount!=0 || item.bag_weight!=0)
//        s << item.bag_amount << "/" << item.bag_weight;
//      else
//        s << "-";
//      NewTextChild(rowNode, "col", s.str());

//      s.str("");
//      if (item.norms.size()==1)
//        s << item.norms.front().str(TReqInfo::Instance()->desk.lang);
//      else
//        getLocaleText("см. подробно");
//      NewTextChild(rowNode, "col", s.str());

//      s.str("");
//      switch(item.norms_trfer)
//      {
//        case nttNone: break;
//        case nttNotTransfer: s << getLocaleText("НЕТ"); break;
//        case nttTransfer: s << getLocaleText("ДА"); break;
//        case nttMixed: s << getLocaleText("СМЕШ"); break;
//      };
//      NewTextChild(rowNode, "col", s.str());

//      s.str("");
//      if (item.weight!=NoExists)
//        s << item.weight;
//      else
//        s << "?";
//      NewTextChild(rowNode, "col", s.str());

//      int emd_weight=0;
//      for(list<CheckIn::TPaidBagEMDItem>::const_iterator e=emd.begin(); e!=emd.end(); ++e)
//        if (i->second.bag_type==e->bag_type)
//        {
//          if (e->bag_type==NoExists)
//          {
//            emd_weight=NoExists;
//            break;
//          }
//          emd_weight+=e->weight;
//        };

//      s.str("");
//      if (emd_weight!=NoExists)
//        s << emd_weight;
//      else
//        s << "?";
//      NewTextChild(rowNode, "col", s.str());
//    };
//  };
};

}; //namespace BagPayment
