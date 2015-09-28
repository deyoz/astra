#include "baggage_calc.h"
#include "qrys.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "events.h"
#include "transfer.h"
#include "events.h"
#include "astra_misc.h"

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
  //norm_trfer �� ���������� �� ������ �⠯�!
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
  if (!pax_cat.empty()/* && !pax.pax_cat.empty()*/) //!!!vlad
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
     filter.use_basic=false; //���� ��� �� ����
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
      if (curr.similarity_cost==NoExists) continue; //��ଠ �� ����� �� ���室��
      curr.priority=n->priority();
      if (curr.priority==NoExists) continue; //��ଠ ������� ���ࠢ��쭮
      curr.norm=*n;

      if (!norm.empty() &&
          norm.bag_type==curr.norm.bag_type &&
          norm.norm_id==curr.norm.norm_id &&
          norm.norm_trfer==filter.is_trfer)
      {
        //��諨 ���� � ��� �������
        max=curr;
        break;
      }

      if (max.norm.empty() || max<curr) max=curr;
    }
    if (!filter.is_trfer) break;
    if (!max.norm.empty() || !use_mixed_norms) break; //��室�� �᫨ �� ���楯��� �࠭����� ����
    //��� �� �� �� ���楯���, �� ����饭� �ᯮ�짮���� � ��砥 ��ଫ���� �࠭��� ���࠭���� ����
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

class TSimpleBagItem
{
  public:
    int bag_type;
    int amount, weight;
    TSimpleBagItem(int _bag_type):
      bag_type(_bag_type),
      amount(0),
      weight(0) {}
    TSimpleBagItem(const CheckIn::TBagItem &item):
      bag_type(item.bag_type),
      amount(item.amount),
      weight(item.weight) {}
    TSimpleBagItem(const TBagToLogInfo &item):
      bag_type(item.bag_type),
      amount(item.amount),
      weight(item.weight) {}
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
    if (n->TryAddNorm(new_norm)) break;
  if (n==norms.end())
    //�� ��諨 � 祬 �㬬�஢��� ���� - ������塞 � �����
    norms.push_back(new_norm);

  return true;
};

class TPaidBagCalcItem : public TPaidBagWideItem
{
  public:
    list<TSimpleBagItem> bag;
    bool norm_per_unit, norm_ordinary;
    TPaidBagCalcItem(int _bag_type,
                     const list<TSimpleBagItem> &_bag) :
      TPaidBagWideItem(_bag_type),
      bag(_bag)
    {
      for(list<TSimpleBagItem>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
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

//�������� ���� ������ - �� ��室� TPaidBagCalcItem - ᠬ�� ������ ��楤��
void CalcPaidBagBase(const std::map<int, TBagToLogInfo> &bag,
                     const std::list<TBagNormInfo> &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     map<int/*bag_type*/, TPaidBagCalcItem> &paid);

typedef std::map< int/*bag_type*/, std::list<TSimpleBagItem> > TSimpleBagMap;

enum TPrepareSimpleBagType1 { psbtOnlyTrfer, psbtOnlyNotTrfer, psbtTrferAndNotTrfer };
enum TPrepareSimpleBagType2 { psbtOnlyRefused, psbtOnlyNotRefused, psbtRefusedAndNotRefused };

void PrepareSimpleBag(const std::map<int/*id*/, TBagToLogInfo> &bag,
                      const TPrepareSimpleBagType1 trfer_type,
                      const TPrepareSimpleBagType2 refused_type,
                      TSimpleBagMap &bag_simple)
{
  bag_simple.clear();
  for(std::map<int/*id*/, TBagToLogInfo>::const_iterator b=bag.begin(); b!=bag.end(); ++b)
  {
    if ((refused_type==psbtOnlyRefused && !b->second.refused) ||
        (refused_type==psbtOnlyNotRefused && b->second.refused)) continue;
    if ((trfer_type==psbtOnlyTrfer && !b->second.is_trfer) ||
        (trfer_type==psbtOnlyNotTrfer && b->second.is_trfer)) continue;
    TSimpleBagMap::iterator i=bag_simple.find(b->second.bag_type);
    if (i==bag_simple.end())
      i=bag_simple.insert(make_pair(b->second.bag_type, std::list<TSimpleBagItem>())).first;
    if (i==bag_simple.end()) throw Exception("%s: i==bag_simple.end()", __FUNCTION__);
    i->second.push_back(TSimpleBagItem(b->second));
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

    TWidePaxInfo() { clear(); }
    void clear()
    {
      TPaxInfo::clear();
      refused=false;
      only_category=false;
      prior_norms.clear();
      curr_norms.clear();
      result_norms.clear();
    }

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


void ConvertNormsList(const std::map<int/*id*/, TBagToLogInfo> &bag,
                      const boost::optional< std::list<CheckIn::TPaxNormItem> > &norms,
                      map<int/*bag_type*/, CheckIn::TPaxNormItem > &result)
{
  result.clear();
  if (!norms) return;

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //�ᥣ�� ������塞 ����� �����
  not_trfer_bag_simple.insert(make_pair(NoExists, std::list<TSimpleBagItem>()));

  for(std::list<CheckIn::TPaxNormItem>::const_iterator n=norms.get().begin(); n!=norms.get().end(); ++n)
  {
    if (not_trfer_bag_simple.find(n->bag_type)==not_trfer_bag_simple.end()) continue; //��䨫��஢뢠�� ����

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

//������� ������ - �� ��室� TPaidBagWideItem
//�ᯮ������ � RecalcPaidBagToDB
void RecalcPaidBagWide(const std::map<int/*id*/, TBagToLogInfo> &prior_bag,
                       const std::map<int/*id*/, TBagToLogInfo> &curr_bag,
                       const std::list<TBagNormInfo> &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                       const std::list<CheckIn::TPaidBagItem> &prior_paid,
                       const std::list<CheckIn::TPaidBagItem> &curr_paid,
                       map<int/*bag_type*/, TPaidBagWideItem> &paid_wide)
{
  paid_wide.clear();

  TSimpleBagMap prior_bag_simple;
  PrepareSimpleBag(prior_bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, prior_bag_simple);
  TSimpleBagMap curr_bag_simple;
  PrepareSimpleBag(curr_bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, curr_bag_simple);

  map<int/*bag_type*/, TPaidBagCalcItem> paid_calc;
  CalcPaidBagBase(curr_bag, norms, paid_calc);

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
          (pass!=0 && item.bag_type!=NoExists)) continue; //����� ����� �ᥣ�� � ��᫥���� ���樨

      multiset<TSimpleBagItem> prior_bag_sorted;
      TSimpleBagMap::const_iterator iPriorBag=prior_bag_simple.find(item.bag_type);
      if (iPriorBag!=prior_bag_simple.end()) prior_bag_sorted.insert(iPriorBag->second.begin(), iPriorBag->second.end());

      multiset<TSimpleBagItem> curr_bag_sorted;
      TSimpleBagMap::const_iterator iCurrBag=curr_bag_simple.find(item.bag_type);
      if (iCurrBag!=curr_bag_simple.end()) curr_bag_sorted.insert(iCurrBag->second.begin(), iCurrBag->second.end());

      //�������� ���ଠ樥� � ����
      for(std::list<CheckIn::TPaidBagItem>::const_iterator j=prior_paid.begin(); j!=prior_paid.end(); ++j)
        if (j->bag_type==item.bag_type)
        {
          item.rate_id=j->rate_id;
          item.rate=j->rate;
          item.rate_cur=j->rate_cur;
          item.rate_trfer=j->rate_trfer;
          break;
        };

      //�������� ��ᮬ ���⭮�� ������, �������� ������
      item.weight=item.weight_calc;
      for(std::list<CheckIn::TPaidBagItem>::const_iterator j=curr_paid.begin(); j!=curr_paid.end(); ++j)
        if (j->bag_type==item.bag_type)
        {
          if (item.bag_type!=NoExists &&
              curr_bag_sorted!=prior_bag_sorted && item.norm_ordinary)
            ordinaryClearWeight=true;
          //�᫨ �� �뫮 ��������� � ������ � ����� ����� �⫨砥��� �� 䠪��᪮��
          if ((item.bag_type!=NoExists || (item.bag_type==NoExists && !ordinaryClearWeight)) &&
              curr_bag_sorted==prior_bag_sorted &&
              j->weight!=NoExists &&
              (item.weight_calc==NoExists || j->weight!=item.weight_calc)) //j->weight!=item.weight_calc - �ࠢ��쭮 �� ��?
            item.weight=j->weight;
          break;
        };

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

//�ନ�㥬 ���ᨢ TWidePaxInfo ��� ���쭥�襣� �맮�� CheckOrGetWidePaxNorms
//�ᯮ������ � test_norms � RecalcpaidBagToDB
void GetWidePaxInfo(const map<int/*id*/, TBagToLogInfo> &curr_bag,
                    const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                    const std::vector<CheckIn::TTransferItem> &trfer,
                    const CheckIn::TPaxGrpItem &grp,
                    const CheckIn::TPaxList &curr_paxs,
                    bool pr_unaccomp,
                    bool use_traces,
                    map<int/*pax_id*/, TWidePaxInfo> &paxs)
{
  paxs.clear();

  string target=((TAirpsRow&)base_tables.get("airps").get_row("code",grp.airp_arv,true)).city;
  string final_target;
  if (!trfer.empty())
    final_target=((TAirpsRow&)base_tables.get("airps").get_row("code",trfer.back().airp_arv,true)).city;

  if (!pr_unaccomp)
  {
    //ॣ������ ���ᠦ�஢
    if (prior_paxs.empty())
    {
      //����� ॣ������
      for(CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin(); pCurr!=curr_paxs.end(); ++pCurr)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.pax_cat=CalcPaxCategory(*pCurr);
        if (grp.status!=psCrew)
          pax.cl=grp.cl;
        else
          pax.cl=EncodeClass(Y); //!!!vlad

        pax.subcl=pCurr->pax.subcl;
        pax.refused=!pCurr->pax.refuse.empty();
        pax.only_category=!pax.pax_cat.empty();
        ConvertNormsList(curr_bag, pCurr->norms, pax.curr_norms);

        int pax_id=(pCurr->pax.id==NoExists?pCurr->generated_pax_id:pCurr->pax.id);
        if (pax_id==NoExists) throw Exception("%s: pax_id==NoExists!", __FUNCTION__);
        if (!paxs.insert(make_pair(pax_id, pax)).second)
          throw Exception("%s: pax_id=%s already exists in paxs!", __FUNCTION__, pax_id);
        if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
      };
    }
    else
    {
      //������ ���������
      for(map<TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator pPrior=prior_paxs.begin(); pPrior!=prior_paxs.end(); ++pPrior)
      {
        TWidePaxInfo pax;
        pax.target=target;
        pax.final_target=final_target;
        pax.pax_cat=CalcPaxCategory(pPrior->second);
        if (grp.status!=psCrew)
          pax.cl=pPrior->second.cl;
        else
          pax.cl=EncodeClass(Y); //!!!vlad
        pax.subcl=pPrior->second.subcl;
        pax.refused=!pPrior->second.refuse.empty();
        pax.only_category=!pax.pax_cat.empty();
        std::list<CheckIn::TPaxNormItem> tmp_norms;
        for(std::list< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> >::const_iterator n=pPrior->second.norms.begin();
                                                                                              n!=pPrior->second.norms.end(); ++n)
          tmp_norms.push_back(n->first);
        ConvertNormsList(curr_bag, tmp_norms, pax.prior_norms);

        CheckIn::TPaxList::const_iterator pCurr=curr_paxs.begin();
        for(; pCurr!=curr_paxs.end(); ++pCurr)
        {
          if (pCurr->pax.id==NoExists) throw Exception("%s: pCurr->pax.id==NoExists!", __FUNCTION__);
          if (pCurr->pax.tid==NoExists) throw Exception("%s: pCurr->pax.tid==NoExists!", __FUNCTION__);
          if (pCurr->pax.id==pPrior->first.pax_id) break;
        }
        if (pCurr!=curr_paxs.end())
        {
          //���ᠦ�� ������
          if (pCurr->pax.PaxUpdatesPending)
          {
            pax.pax_cat=CalcPaxCategory(*pCurr);
            if (grp.status!=psCrew)
              pax.cl=grp.cl;
            else
              pax.cl=EncodeClass(Y); //!!!vlad
            pax.subcl=pCurr->pax.subcl;
            pax.refused=!pCurr->pax.refuse.empty();
            pax.only_category=!pax.pax_cat.empty();
          };
          ConvertNormsList(curr_bag, pCurr->norms, pax.curr_norms);

          if (!paxs.insert(make_pair(pCurr->pax.id, pax)).second)
            throw Exception("%s: pCurr->pax.id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
          if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());

        }
        else
        {
          //���ᠦ�� �� ������
          if (!paxs.insert(make_pair(pPrior->first.pax_id, pax)).second)
            throw Exception("%s: pPrior->first.pax_id=%s already exists in paxs!", __FUNCTION__, pCurr->pax.id);
          if (use_traces) ProgTrace(TRACE5, "%s: pax:%s", __FUNCTION__, pax.traceStr().c_str());
        }
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
    pax.only_category=!pax.pax_cat.empty();
    ConvertNormsList(curr_bag, grp.norms, pax.curr_norms);
    if (!prior_paxs.empty())
    {
      //������ ��������� ��ᮯ஢��������� ������
      if (prior_paxs.size()!=1)
        throw Exception("%s: prior_paxs.size()!=1!", __FUNCTION__);

      std::list<CheckIn::TPaxNormItem> tmp_norms;
      for(std::list< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> >::const_iterator n=prior_paxs.begin()->second.norms.begin();
                                                                                            n!=prior_paxs.begin()->second.norms.end(); ++n)
        tmp_norms.push_back(n->first);
      ConvertNormsList(curr_bag, tmp_norms, pax.prior_norms);
    };
    paxs.insert(make_pair(grp.id, pax));
    if (use_traces) ProgTrace(TRACE5, "%s: unaccomp:%s", __FUNCTION__, pax.traceStr().c_str());
  };
}

//�������� ���
//�� �室� paxs.prior_norms paxs.curr_norms
//�� ��室� paxs.result_norms � ��騩 ᯨ᮪ ��� ��� ��ࠧॣ����஢����� ���ᠦ�஢
//�ᯮ������ � test_norms � RecalcpaidBagToDB
void CheckOrGetWidePaxNorms(const list<TBagNormWideInfo> &trip_bag_norms,
                            const map<int/*id*/, TBagToLogInfo> &curr_bag,
                            const TNormFltInfo &flt,
                            bool pr_unaccomp,
                            bool use_traces,
                            map<int/*pax_id*/, TWidePaxInfo> &paxs,
                            list<TBagNormInfo> &all_norms)
{
  all_norms.clear();

  TSimpleBagMap not_trfer_bag_simple;
  PrepareSimpleBag(curr_bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
  //�ᥣ�� ������塞 ����� �����
  not_trfer_bag_simple.insert(make_pair(NoExists, std::list<TSimpleBagItem>()));

  for(map<int/*pax_id*/, TWidePaxInfo>::iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    TWidePaxInfo &pax=p->second;
    for(TSimpleBagMap::const_iterator i=not_trfer_bag_simple.begin(); i!=not_trfer_bag_simple.end(); ++i)
    {
      TBagNormInfo result;
      int bag_type=i->first;
      try
      {
        map<int/*bag_type*/, CheckIn::TPaxNormItem >::const_iterator n;
        //1 ��室: ���� ����
        if ((n=pax.curr_norms.find(bag_type))!=pax.curr_norms.end())
        {
          if (n->second.norm_id==NoExists) throw 1;
          CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, bag_type, n->second, result);
          if (!result.empty() && n->second==result) throw 1;
        }
        //2 ��室: ���� ����
        if ((n=pax.prior_norms.find(bag_type))!=pax.prior_norms.end())
        {
          if (n->second.norm_id==NoExists) throw 1;
          CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, bag_type, n->second, result);
          if (!result.empty() && n->second==result) throw 1;
        }
        //3 ��室: ���������騥 ����
        CheckOrGetPaxBagNorm(trip_bag_norms, pax, flt.use_mixed_norms, pax.only_category, bag_type, TBagNormInfo(), result);
      }
      catch(int) {};

      if (result.empty()) result.bag_type=bag_type;
      pax.result_norms.insert(make_pair(bag_type, result));
      //����� ����� ��ନ஢���� result
      if (!pax.refused) all_norms.push_back(result); //������塞 ���� ��ࠧॣ����஢����� ���ᠦ�஢
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

//������� ������ � ������樥� ��� � ������� � ⠡���� grp_norms, pax_norms � paid_bag
//�ਬ������ �� ����� ��������� �� ��㯯� ���ᠦ�஢ (SavePax)
void RecalcPaidBagToDB(const map<int/*id*/, TBagToLogInfo> &prior_bag, //TBagToLogInfo � �� CheckIn::TBagItem ��⮬� �� ���� refused
                       const map<int/*id*/, TBagToLogInfo> &curr_bag,
                       const std::map<TPaxToLogInfoKey, TPaxToLogInfo> &prior_paxs,
                       const TNormFltInfo &flt,
                       const std::vector<CheckIn::TTransferItem> &trfer,
                       const CheckIn::TPaxGrpItem &grp,
                       const CheckIn::TPaxList &curr_paxs,
                       const std::list<CheckIn::TPaidBagItem> &prior_paid,
                       bool pr_unaccomp)
{
   ProgTrace(TRACE5, "%s: flt:%s", __FUNCTION__, flt.traceStr().c_str());

   map<int/*pax_id*/, TWidePaxInfo> paxs;
   GetWidePaxInfo(curr_bag, prior_paxs, trfer, grp, curr_paxs, pr_unaccomp, true, paxs);

   //����/�஢�ઠ �������� ���
   list<TBagNormWideInfo> trip_bag_norms;
   LoadTripBagNorms(flt, trip_bag_norms);

   std::list<TBagNormInfo> all_norms;
   CheckOrGetWidePaxNorms(trip_bag_norms, curr_bag, flt, pr_unaccomp, true, paxs, all_norms);

   //ᮡ�⢥��� ���� ���⭮�� ������
   map<int/*bag_type*/, TPaidBagWideItem> paid_wide;
   const list<CheckIn::TPaidBagItem> &curr_paid=grp.paid?grp.paid.get():list<CheckIn::TPaidBagItem>();
   RecalcPaidBagWide(prior_bag, curr_bag, all_norms, prior_paid, curr_paid, paid_wide);

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
     if (pax.prior_norms==pax.result_norms) continue;//�� �뫮 ��������� ��� � ���ᦨ�

     std::list<CheckIn::TPaxNormItem> norms;
     ConvertNormsList(pax.result_norms, norms);
     if (!pr_unaccomp)
       CheckIn::PaxNormsToDB(p->first, norms);
     else
       CheckIn::GrpNormsToDB(p->first, norms);
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
  TReqInfo::Instance()->Initialize("���");
  TReqInfo::Instance()->desk.lang=LANG_EN;


  TQuery GrpQry(&OraSession);
  GrpQry.Clear();
  GrpQry.SQLText=
    "SELECT point_id, grp_id, airp_arv, class, status, bag_refuse, client_type, "
    "       mark_trips.airline, mark_trips.flt_no, pr_mark_norms "
    "FROM pax_grp, mark_trips "
    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND point_id=:point_id "
    "ORDER BY mark_trips.airline, mark_trips.flt_no, pr_mark_norms"  ;
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


  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out "
    "FROM points "
    "WHERE scd_out>=TO_DATE('19.09.15','DD.MM.YY') AND scd_out<TO_DATE('22.09.15','DD.MM.YY') AND pr_reg=1";
  Qry.Execute();
  list<TBagNormWideInfo> trip_bag_norms;
  TNormFltInfo prior;
  int count=0;
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
      TClientType client_type=DecodeClientType(GrpQry.FieldAsString("client_type"));
      TGrpToLogInfo grpLogInfo;
      std::vector<CheckIn::TTransferItem> trfer;
      GetGrpToLogInfo(grp.id, grpLogInfo);
      CheckIn::LoadTransfer(grp.id, trfer);
      for(map<int/*id*/, TBagToLogInfo>::iterator i=grpLogInfo.bag.begin(); i!=grpLogInfo.bag.end(); ++i)
      {
        if (i->second.bag_type==99) i->second.is_trfer=true;
      }


      bool pr_unaccomp=grp.cl.empty() && grp.status!=psCrew;
      TSimpleBagMap not_trfer_bag_simple;
      PrepareSimpleBag(grpLogInfo.bag, psbtOnlyNotTrfer, psbtRefusedAndNotRefused, not_trfer_bag_simple);
      //�ᥣ�� ������塞 ����� �����
      not_trfer_bag_simple.insert(make_pair(NoExists, std::list<TSimpleBagItem>()));


      map<int/*pax_id*/, TWidePaxInfo> paxs;
      GetWidePaxInfo(grpLogInfo.bag, grpLogInfo.pax, trfer, grp, CheckIn::TPaxList(), pr_unaccomp, false, paxs);

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
        //1 ��室 �஢���� �� �� ���� ����� �ਢ易�� �� ॠ�쭮� �ࢥ� ࠧ�襭� ���� �����⬮�
        //2 ��室 �஢���� �� �� ���� ����� �ਢ易�� �� ॠ�쭮� �ࢥ� ��⮬���᪨ �����뢠���� ���� �����⬮�

        CheckOrGetWidePaxNorms(trip_bag_norms, grpLogInfo.bag, flt, pr_unaccomp, false, (pass==0?paxs:tmp_paxs), all_norms);

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
          if (/*client_type!=ctTerm &&*/ not_trfer_bag_simple.size()==1 && not_trfer_bag_simple.begin()->first==NoExists &&
              pax.prior_norms.empty() && pax.result_norms.size()==1 && pax.result_norms.begin()->first==NoExists) continue;

          if (error_ids.find(p->first)!=error_ids.end()) continue;

          error_exists=true;
          error_ids.insert(p->first);

          ostringstream error;
          ostringstream trace;
          error << (pass==0?"forbidden":"mismatched") << " norms flight=" << GetTripName(fltInfo, ecNone, true, false ).c_str()
                << " grp_id=" << grp.id;
          if (!pr_unaccomp) error << " pax_id=" << p->first;
          trace << pax.traceStr();
          ProgError(STDLOG, "%s: %s", __FUNCTION__, error.str().c_str());
          ProgError(STDLOG, "%s: %s", __FUNCTION__, trace.str().c_str());

          Qry4.SetVariable("point_id", flt.point_id);
          Qry4.SetVariable("grp_id", grp.id);
          if (!pr_unaccomp)
            Qry4.SetVariable("pax_id", p->first);
          else
            Qry4.SetVariable("pax_id", FNull);
          Qry4.SetVariable("error", error.str());
          Qry4.SetVariable("trace", trace.str());
          Qry4.Execute();
        }
      }
      OraSession.Commit();
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

//���� ������ - �� ��室� TPaidBagWideItem
//�ᯮ������ � PaidBagViewToXML
void CalcPaidBagWide(const std::map<int/*id*/, TBagToLogInfo> &bag,
                     const std::list<TBagNormInfo> &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     const std::list<CheckIn::TPaidBagItem> &paid,
                     map<int/*bag_type*/, TPaidBagWideItem> &paid_wide)
{
  paid_wide.clear();

  map<int/*bag_type*/, TPaidBagCalcItem> paid_calc;
  CalcPaidBagBase(bag, norms, paid_calc);

  for(map<int/*bag_type*/, TPaidBagCalcItem>::const_iterator p=paid_calc.begin(); p!=paid_calc.end(); ++p)
  {
    map<int/*bag_type*/, TPaidBagWideItem>::iterator i=paid_wide.insert(*p).first;
    TPaidBagWideItem &item=i->second;
    item.weight=item.weight_calc;
    for(std::list<CheckIn::TPaidBagItem>::const_iterator j=paid.begin(); j!=paid.end(); ++j)
      if (j->bag_type==item.bag_type)
      {
        item.weight=j->weight;
        item.rate_id=j->rate_id;
        item.rate=j->rate;
        item.rate_cur=j->rate_cur;
        item.rate_trfer=j->rate_trfer;
        break;
      };
  };

  ProgTrace(TRACE5, "%s: %s", __FUNCTION__, NormsTraceStr(norms).c_str());
  TracePaidBagWide(paid_wide, __FUNCTION__);
}

void CalcTrferBagWide(const std::map<int/*id*/, TBagToLogInfo> &bag,
                      map<int/*bag_type*/, TPaidBagWideItem> &paid_wide)
{
  paid_wide.clear();

  TSimpleBagMap bag_simple;
  PrepareSimpleBag(bag, psbtOnlyTrfer, psbtOnlyNotRefused, bag_simple);

  map<int/*bag_type*/, TPaidBagCalcItem> paid_calc;
  for(TSimpleBagMap::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid_calc.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  for(map<int/*bag_type*/, TPaidBagCalcItem>::const_iterator p=paid_calc.begin(); p!=paid_calc.end(); ++p)
  {
    map<int/*bag_type*/, TPaidBagWideItem>::iterator i=paid_wide.insert(*p).first;
    TPaidBagWideItem &item=i->second;
    item.weight=item.weight_calc;
  };

  TracePaidBagWide(paid_wide, __FUNCTION__);
}

//���� ������ - �� ��室� TPaidBagCalcItem - ᠬ�� ������ ��楤��
void CalcPaidBagBase(const std::map<int/*id*/, TBagToLogInfo> &bag,
                     const std::list<TBagNormInfo> &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                     map<int/*bag_type*/, TPaidBagCalcItem> &paid)
{
  //� ����� ����� �� �������� �ਢ易��� � ࠧॣ����஢���� ���ᠦ�ࠬ

  paid.clear();

  TSimpleBagMap bag_simple;
  PrepareSimpleBag(bag, psbtOnlyNotTrfer, psbtOnlyNotRefused, bag_simple);

  for(TSimpleBagMap::const_iterator b=bag_simple.begin(); b!=bag_simple.end(); ++b)
    paid.insert(make_pair(b->first, TPaidBagCalcItem(b->first, b->second)));

  //� ���ᨢ� �ᥣ�� ����� �����:
  if (paid.find(NoExists)==paid.end())
    paid.insert(make_pair(NoExists, TPaidBagCalcItem(NoExists, std::list<TSimpleBagItem>())));

  //᫮��� �� ����:
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
          (pass!=0 && item.bag_type!=NoExists)) continue; //����� ����� �ᥣ�� � ��᫥���� ���樨

      if (item.bag_type==NoExists && ordinaryUnknown) item.weight_calc=NoExists;
      else
      {
        //�஢�ਬ ������ ��� �� ���� ��ଠ
        if (item.norms.empty())
        {
          if (item.bag_type!=NoExists)
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
                      if (item.bag_type!=NoExists)
                      {
                        TSimpleBagItem sum(item.bag_type);
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
                  for(list<TSimpleBagItem>::reverse_iterator j=item.bag.rbegin(); j!=item.bag.rend();)
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
                    //���� ���௠�� ��� �� ���室��
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
            //ᬥ襭�� ���
            item.weight_calc=(item.bag.empty()?0:NoExists);
          };
        };
      };

      //��� ���⭮�� ������ �஢�ਬ ���� �� �।� ��� ��
      if (item.bag_type==NoExists && item.weight_calc==NoExists && item.norm_ordinary) ordinaryUnknown=true;

      item.weight=NoExists;
    };
  };

}

void PaidBagViewToXML(const std::map<int/*id*/, TBagToLogInfo> &bag,
                      const std::list<TBagNormInfo> &norms, //����� ᯨ᮪ �ᥢ�������� ��� ��� ��� ���ᠦ�஢ ���६���
                      const std::list<CheckIn::TPaidBagItem> &paid,
                      const std::list<CheckIn::TPaidBagEMDItem> &emd,
                      const std::string &used_airline_mark,
                      xmlNodePtr dataNode)
{
  if (dataNode==NULL) return;

  for(bool is_trfer=false; ;is_trfer=!is_trfer)
  {
    map<int/*bag_type*/, TPaidBagWideItem> paid_wide;
    if (!is_trfer)
      CalcPaidBagWide(bag, norms, paid, paid_wide);
    else
      CalcTrferBagWide(bag, paid_wide);

    xmlNodePtr node=!is_trfer?NewTextChild(dataNode, "paid_bags"):
                              NewTextChild(dataNode, "trfer_bags");

    for(int pass=0; pass<2; pass++)
    {
      for(map<int/*bag_type*/, TPaidBagWideItem>::const_iterator i=paid_wide.begin(); i!=paid_wide.end(); ++i)
      {
        const TPaidBagWideItem &item=i->second;
        if ((pass==0 && item.bag_type!=NoExists) ||
            (pass!=0 && item.bag_type==NoExists)) continue; //����� ����� �ᥣ�� ��ࢮ� ��ப��

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
          s << getLocaleText("����� ����� ��� �/�����");
        NewTextChild(rowNode, "bag_type_view", s.str());

        s.str("");
        if (item.bag_amount!=0 || item.bag_weight!=0)
          s << item.bag_amount << "/" << item.bag_weight;
        else
          s << "-";
        NewTextChild(rowNode, "bag_number_view", s.str());

        s.str("");
        if (!is_trfer)
        {
          if (!item.norms.empty())
          {
            if (!used_airline_mark.empty())
              s << ElemIdToCodeNative(etAirline, used_airline_mark) << ":";
            if (item.norms.size()==1)
              s << lowerc(item.norms.front().str(TReqInfo::Instance()->desk.lang));
            else
              s << getLocaleText("�. ���஡��");
          }
          else s << "-";
        }
        else s << getLocaleText("�࠭���");
        NewTextChild(rowNode, "norms_view", s.str());

        s.str("");
        switch(item.norms_trfer)
        {
          case nttNone: break;
          case nttNotTransfer: s << getLocaleText("���"); break;
          case nttTransfer: s << getLocaleText("��"); break;
          case nttMixed: s << getLocaleText("����"); break;
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

        s.str("");
        if (!is_trfer)
        {
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

          if (emd_weight!=NoExists)
            s << emd_weight;
          else
            s << "?";
        };
        NewTextChild(rowNode, "emd_weight_view", s.str());
      };

    };
    if (is_trfer) break;
  };
};


//<... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...> //ᥪ�� ⠡����
//  <cols>  //ᥪ�� ���ᠭ�� �⮫�殢
//    </col width=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>
//    ...
//    ...
//  </cols>
//  <header height=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>  //ᥪ�� ���������
//    <col color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>⥪��</col>
//    ...
//    ...
//  </header>
//  <rows>  //ᥪ�� ������
//    <row height=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...> //ᥪ�� ��ப�
//      <col color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>⥪��</col> //ᥪ�� �祩��
//      ...
//      ...
//    </row>
//    ...
//    ...
//  </rows>
//</...>

void PaidBagViewToXMLTest(xmlNodePtr node)
{
  ostringstream s;
  s << "���� �� �� ��������" << endl
    << "�������� � ������� ����� �� ����� �� ���� ������" << endl
    << "� ����� �������� �� ��� �����, �� ������� �������� ��������"  << endl
    << "����� ����...." << endl;

  NewTextChild(node, "norms_view", s.str());

  {
  xmlNodePtr paidNode=NewTextChild(node, "paid_bags");

  xmlNodePtr rowNode;

  rowNode=NewTextChild(paidNode,"paid_bag");

  NewTextChild(rowNode,"bag_type","0AA");
  NewTextChild(rowNode,"weight","2");
  NewTextChild(rowNode,"weight_calc","2");
  NewTextChild(rowNode,"rate_id");
  NewTextChild(rowNode,"rate");
  NewTextChild(rowNode,"rate_cur");
  NewTextChild(rowNode,"rate_trfer");

  NewTextChild(rowNode,"bag_type_view","0AA");
  NewTextChild(rowNode,"bag_number_view","4");
  NewTextChild(rowNode,"weight_view","2");
  NewTextChild(rowNode,"weight_calc_view","2");

  rowNode=NewTextChild(paidNode,"paid_bag");

  NewTextChild(rowNode,"bag_type","C12");
  NewTextChild(rowNode,"weight","3");
  NewTextChild(rowNode,"weight_calc","3");
  NewTextChild(rowNode,"rate_id");
  NewTextChild(rowNode,"rate");
  NewTextChild(rowNode,"rate_cur");
  NewTextChild(rowNode,"rate_trfer");

  NewTextChild(rowNode,"bag_type_view","C12");
  NewTextChild(rowNode,"bag_number_view","5");
  NewTextChild(rowNode,"weight_view","3");
  NewTextChild(rowNode,"weight_calc_view","3");

  rowNode=NewTextChild(paidNode,"paid_bag");

  NewTextChild(rowNode,"bag_type","C23");
  NewTextChild(rowNode,"weight","1");
  NewTextChild(rowNode,"weight_calc","1");
  NewTextChild(rowNode,"rate_id");
  NewTextChild(rowNode,"rate");
  NewTextChild(rowNode,"rate_cur");
  NewTextChild(rowNode,"rate_trfer");

  NewTextChild(rowNode,"bag_type_view","C23");
  NewTextChild(rowNode,"bag_number_view","4");
  NewTextChild(rowNode,"weight_view","1");
  NewTextChild(rowNode,"weight_calc_view","1");
  }


  const string taLeftJustify="taLeftJustify";  //!!!vlad � astra_consts
  const string taRightJustify="taRightJustify";
  const string taCenter="taCenter";
  const string fsBold="fsBold";

  xmlNodePtr paidNode=NewTextChild(node, "paid_bag_view");
  SetProp(paidNode, "font_size", 8);

  xmlNodePtr colNode, rowNode;
  //ᥪ�� ����뢠��� �⮫���
  xmlNodePtr colsNode=NewTextChild(paidNode, "cols");
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 130);
  SetProp(colNode, "align", taLeftJustify);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 35);
  SetProp(colNode, "align", taCenter);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 85);
  SetProp(colNode, "align", taLeftJustify);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 40);
  SetProp(colNode, "align", taRightJustify);
  SetProp(colNode, "font_style", "fsBold");
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 40);
  SetProp(colNode, "align", taRightJustify);
  SetProp(colNode, "font_style", "fsBold");

  //ᥪ�� ����뢠��� ���������
  xmlNodePtr headerNode=NewTextChild(paidNode, "header");
  SetProp(headerNode, "font_size", 10);
  SetProp(headerNode, "font_style", "");
  SetProp(headerNode, "align", taLeftJustify);
  NewTextChild(headerNode, "col", getLocaleText("RFISC"));
  NewTextChild(headerNode, "col", getLocaleText("���."));
  NewTextChild(headerNode, "col", getLocaleText("�������"));
  NewTextChild(headerNode, "col", getLocaleText("� ���."));
  NewTextChild(headerNode, "col", getLocaleText("����祭�"));

  xmlNodePtr rowsNode = NewTextChild(paidNode, "rows");
  rowNode=NewTextChild(rowsNode, "row");
  colNode=NewTextChild(rowNode, "col", "AAA: ��������� AAA");
  colNode=NewTextChild(rowNode, "col", 2);
  colNode=NewTextChild(rowNode, "col", "��111 ���");
  colNode=NewTextChild(rowNode, "col", 2);
  SetProp(colNode, "font_color", "clInactiveAlarm");
  SetProp(colNode, "font_color_selected", "clInactiveAlarm");
  colNode=NewTextChild(rowNode, "col", 0);

  rowNode=NewTextChild(rowsNode, "row");
  colNode=NewTextChild(rowNode, "col", "AAA: ��������� AAA");
  colNode=NewTextChild(rowNode, "col", 2);
  colNode=NewTextChild(rowNode, "col", "��222 ���");
  colNode=NewTextChild(rowNode, "col", 2);
  SetProp(colNode, "font_color", "clInactiveBright");
  SetProp(colNode, "font_color_selected", "clInactiveBright");
  colNode=NewTextChild(rowNode, "col", 2);

  rowNode=NewTextChild(rowsNode, "row");
  colNode=NewTextChild(rowNode, "col", "BBB: ��������� BBB");
  colNode=NewTextChild(rowNode, "col", 2);
  colNode=NewTextChild(rowNode, "col", "��111 ���");
  colNode=NewTextChild(rowNode, "col", 1);
  SetProp(colNode, "font_color", "clInactiveAlarm");
  SetProp(colNode, "font_color_selected", "clInactiveAlarm");
  colNode=NewTextChild(rowNode, "col", 0);

  rowNode=NewTextChild(rowsNode, "row");
  colNode=NewTextChild(rowNode, "col", "BBB: ��������� BBB");
  colNode=NewTextChild(rowNode, "col", 2);
  colNode=NewTextChild(rowNode, "col", "��222 ���");
  colNode=NewTextChild(rowNode, "col", 1);
  SetProp(colNode, "font_color", "clInactiveAlarm");
  SetProp(colNode, "font_color_selected", "clInactiveBright");
  colNode=NewTextChild(rowNode, "col", 0);
}

}; //namespace BagPayment
