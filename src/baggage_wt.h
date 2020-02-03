#ifndef _BAGGAGE_WT_H_
#define _BAGGAGE_WT_H_

#include "astra_consts.h"
#include "astra_locale_adv.h"
#include "xml_unit.h"
#include "oralib.h"
#include "baggage_base.h"

namespace WeightConcept
{

const std::string REGULAR_BAG_TYPE_IN_DB=" ";
const std::string REGULAR_BAG_TYPE="";
const std::string REGULAR_BAG_NAME="Обычный багаж или р/кладь";

}

class TBagTypeListKey
{
  public:
    std::string bag_type;
    std::string airline;

    TBagTypeListKey() { clear(); }

    TBagTypeListKey(const std::string &_bag_type,
                  const std::string &_airline) :
      bag_type(_bag_type), airline(_airline) {}

    void clear()
    {
      bag_type.clear();
      airline.clear();
    }

    bool operator == (const TBagTypeListKey &key) const // сравнение
    {
      return bag_type==key.bag_type &&
             airline==key.airline;
    }

    bool operator < (const TBagTypeListKey &key) const
    {
      if (bag_type!=key.bag_type)
        return bag_type<key.bag_type;
      return airline<key.airline;
    }

    std::string str(const std::string& lang="") const;

    const TBagTypeListKey& toXML(xmlNodePtr node) const;
    TBagTypeListKey& fromXML(xmlNodePtr node);
    TBagTypeListKey& fromXMLcompatible(xmlNodePtr node);
    const TBagTypeListKey& toDB(TQuery &Qry) const;
    TBagTypeListKey& fromDB(TQuery &Qry);
    const TBagTypeListKey& key() const { return *this; }
    void key(const TBagTypeListKey& _key) { *this=_key; }
    bool isRegular() const { return bag_type==WeightConcept::REGULAR_BAG_TYPE; }
};

class TBagTypeListItem : public TBagTypeListKey
{
  public:
    TServiceCategory::Enum category;
    std::string name;
    std::string name_lat;
    std::string descr;
    std::string descr_lat;
    bool visible;
    boost::optional<int> priority;

    TBagTypeListItem()
    {
      clear();
    }

    void clear()
    {
      TBagTypeListKey::clear();
      category=TServiceCategory::Other;
      name.clear();
      name_lat.clear();
      descr.clear();
      descr_lat.clear();
      visible=false;
      priority=boost::none;
    }

    bool operator == (const TBagTypeListItem &item) const // сравнение
    {
      return TBagTypeListKey::operator ==(item) &&
             category==item.category &&
             name==item.name &&
             name_lat==item.name_lat &&
             descr==item.descr &&
             descr_lat==item.descr_lat &&
             visible==item.visible;
    }

    const std::string& name_view(const std::string& lang="") const;
    const std::string& descr_view(const std::string& lang="") const;

    const TBagTypeListItem& toXML(xmlNodePtr node, const boost::optional<TBagTypeListItem> &def=boost::none) const;
    const TBagTypeListItem& toDB(TQuery &Qry) const;
    TBagTypeListItem& fromDB(TQuery &Qry);

    bool isBaggageOrCarryOn() const;
    bool isBaggageInCabinOrCarryOn() const;
    bool isBaggageInHold() const;
};

class TBagTypeKey : public TBagTypeListKey
{
  private:
    enum GetItemWay
    {
      Unaccomp,
      ByGrpId,
      ByPaxId,
      ByBagPool
    };
    void getListKey(GetItemWay way, int id, int transfer_num, int bag_pool_num,
                    boost::optional<TServiceCategory::Enum> category,
                    const std::string &where);
    void getListItem(GetItemWay way, int id, int transfer_num, int bag_pool_num,
                     boost::optional<TServiceCategory::Enum> category,
                     const std::string &where);

  public:
    int list_id;
    boost::optional<TBagTypeListItem> list_item;

    TBagTypeKey() { clear(); }

    void clear()
    {
      TBagTypeListKey::clear();
      list_id=ASTRA::NoExists;
      list_item=boost::none;
    }

    const TBagTypeKey& toXML(xmlNodePtr node) const;
    const TBagTypeKey& toDB(TQuery &Qry) const;
    const TBagTypeKey& toDBcompatible(TQuery &Qry, const std::string &where) const;
    TBagTypeKey& fromDB(TQuery &Qry);
    TBagTypeKey& fromDBcompatible(TQuery &Qry);
    void getListItemIfNone();
    void getListItem();
    void getListItemUnaccomp (int grp_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListItem(Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListItemByGrpId  (int grp_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListItem(ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListItemByPaxId  (int pax_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListItem(ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListItemByBagPool(int grp_id, int transfer_num, int bag_pool_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListItem(ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
    }
    void getListKeyUnaccomp (int grp_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListKey(Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListKeyByGrpId  (int grp_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListKey(ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListKeyByPaxId  (int pax_id, int transfer_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListKey(ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
    }
    void getListKeyByBagPool(int grp_id, int transfer_num, int bag_pool_num, boost::optional<TServiceCategory::Enum> category, const std::string &where)
    {
      getListKey(ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
    }
    std::string traceStr() const;
};

typedef std::map<TBagTypeListKey, TBagTypeListItem> TBagTypeListMap;

class TBagTypeList : public TBagTypeListMap
{
  protected:
    Statistic<std::string> airline_stat;
    Statistic<TServiceCategory::Enum> category_stat;
    Statistic< boost::optional<int> > priority_stat;
    void update_stat(const TBagTypeListItem &item);
  public:
    void toXML(int list_id, xmlNodePtr node) const;
    void fromDB(int list_id, bool only_visible=false); // загрузка только списка
    void toDB(int list_id) const; // сохранение только списка
    int crc() const;
    int toDBAdv() const; //продвинутое сохранение с анализом существующих справочников
    void createWithCurrSegBagAirline(int grp_id);
    void create(const std::string &airline);
    void create(const std::string &airline,
                const std::string &airp,
                const bool is_unaccomp);
    void createForInboundTrferFromBTM(const std::string &airline);
};

namespace WeightConcept
{

TBagTypeListKey RegularBagType(const std::string &airline);

class TNormItem
{
  public:
    ASTRA::TBagNormType norm_type;
    int amount;
    int weight;
    int per_unit;
  TNormItem()
  {
    clear();
  }
  void clear()
  {
    norm_type=ASTRA::bntUnknown;
    amount=ASTRA::NoExists;
    weight=ASTRA::NoExists;
    per_unit=ASTRA::NoExists;
  }
  bool operator == (const TNormItem &item) const
  {
    return norm_type == item.norm_type &&
           amount == item.amount &&
           weight == item.weight &&
           per_unit == item.per_unit;
  }
  bool isUnknown() const
  {
    return norm_type==ASTRA::bntUnknown;
  }
  const TNormItem& toXML(xmlNodePtr node) const;
  TNormItem& fromDB(TQuery &Qry);
  const TNormItem& toDB(TQuery &Qry) const;
  std::string str(const std::string& lang) const;
  void GetNorms(PrmEnum& prmenum) const;
  bool getByNormId(int normId, bool& isDirectActionNorm);
  bool getByNormId(int normId);
};

class TPaxNormItem : public TBagTypeKey
{
  public:
    int norm_id;
    bool norm_trfer;
    boost::optional<bool> handmade;
  TPaxNormItem()
  {
    clear();
  }
  TPaxNormItem(const TBagTypeListKey& key_)
  {
    clear();
    key(key_);
    handmade=false;
  }
  TPaxNormItem(const TBagTypeListKey& key_, const int norm_id_, const bool norm_trfer_)
  {
    clear();
    key(key_);
    norm_id=norm_id_;
    norm_trfer=norm_trfer_;
    handmade=false;
  }

  void clear()
  {
    TBagTypeKey::clear();
    norm_id=ASTRA::NoExists;
    norm_trfer=false;
    handmade=boost::none;
  }
  bool operator ==(const TPaxNormItem &item) const
  {
    return TBagTypeKey::operator ==(item) &&
           norm_id==item.norm_id &&
           norm_trfer==item.norm_trfer &&
           handmade==item.handmade;
  }
  bool equal(const TPaxNormItem &item) const
  {
    return TBagTypeListKey::operator ==(item) &&
           norm_id==item.norm_id &&
           norm_trfer==item.norm_trfer;
  }
  const TPaxNormItem& toXML(xmlNodePtr node) const;
  TPaxNormItem& fromXML(xmlNodePtr node);
  const TPaxNormItem& toDB(TQuery &Qry) const;
  TPaxNormItem& fromDB(TQuery &Qry);

  bool normNotExists() const
  {
    return norm_id==ASTRA::NoExists;
  }

  bool isManuallyDeleted() const
  {
    return norm_id==ASTRA::NoExists && handmade && handmade.get();
  }
};

std::ostream& operator<<(std::ostream& os, const TPaxNormItem&);

class TPaxNormComplex : public TPaxNormItem, public TNormItem
{
  public:
    TPaxNormComplex():
      TPaxNormItem(),
      TNormItem() {}
    TPaxNormComplex(const TPaxNormItem& paxNormItem, const TNormItem& normItem):
      TPaxNormItem(paxNormItem),
      TNormItem(normItem) {}
    TPaxNormComplex(TQuery &Qry)
    {
      fromDB(Qry);
    }

    void clear()
    {
      TPaxNormItem::clear();
      TNormItem::clear();
    }

    TPaxNormComplex& fromDB(TQuery &Qry)
    {
      TPaxNormItem::fromDB(Qry);
      TNormItem::fromDB(Qry);
      return *this;
    }

    const TPaxNormComplex& toXML(xmlNodePtr node) const
    {
      TPaxNormItem::toXML(node);
      TNormItem::toXML(node);
      return *this;
    }

    std::string normStr(const std::string& lang) const
    {
      return TNormItem::str(lang);
    }
};

typedef std::set<TPaxNormComplex> TPaxNormComplexContainer;
typedef std::set<TPaxNormItem> TPaxNormItemContainer;

bool PaxNormsFromDB(TDateTime part_key, int pax_id, TPaxNormComplexContainer &norms);
bool GrpNormsFromDB(TDateTime part_key, int grp_id, TPaxNormComplexContainer &norms);
void NormsToXML(const TPaxNormComplexContainer &norms, xmlNodePtr node);
void PaxNormsToDB(int pax_id, const boost::optional< TPaxNormItemContainer > &norms);
void GrpNormsToDB(int grp_id, const boost::optional< TPaxNormItemContainer > &norms);

class TPaidBagItem : public TBagTypeKey
{
  public:
    int weight;
    int rate_id;
    double rate;
    std::string rate_cur;
    bool rate_trfer;
    boost::optional<bool> handmade;
  TPaidBagItem()
  {
    clear();
  };
  void clear()
  {
    TBagTypeKey::clear();
    weight=0;
    rate_id=ASTRA::NoExists;
    rate=ASTRA::NoExists;
    rate_cur.clear();
    rate_trfer=false;
    handmade=boost::none;
  };
  bool operator == (const TPaidBagItem &item) const
  {
    return key()==item.key() &&
           weight==item.weight &&
           rate_id==item.rate_id &&
           rate==item.rate &&
           rate_cur==item.rate_cur &&
           rate_trfer==item.rate_trfer;
  };
  const TPaidBagItem& toXML(xmlNodePtr node) const;
  TPaidBagItem& fromXML(xmlNodePtr node);
  const TPaidBagItem& toDB(TQuery &Qry) const;
  TPaidBagItem& fromDB(TQuery &Qry);
  int priority() const;
  std::string bag_type_view(const std::string& lang="") const;
  bool paid_positive() const { return weight>0; }
};

std::ostream& operator<<(std::ostream& os, const TPaidBagItem&);

class TPaidBagList : public std::list<TPaidBagItem>
{
  public:
    void getAllListKeys(int grp_id, bool is_unaccomp);
    void getAllListItems(int grp_id, bool is_unaccomp);
    bool becamePaid(const TPaidBagList& paidBefore) const;
};

void PaidBagFromXML(xmlNodePtr paidbagNode,
                    int grp_id, bool is_unaccomp, bool trfer_confirm,
                    boost::optional< TPaidBagList > &paid);
void PaidBagToDB(int grp_id, bool is_unaccomp,
                 TPaidBagList &paid);
void PaidBagFromDB(TDateTime part_key, int grp_id, TPaidBagList &paid);

std::string GetCurrSegBagAirline(int grp_id);

} //namespace WeightConcept

class TPaidBagViewItem : public WeightConcept::TPaidBagItem
{
  public:
    int weight_calc;
    std::string total_view;
    std::string norms_view;
    std::string norms_trfer_view;
    int weight_rcpt;

  TPaidBagViewItem(const TPaidBagItem& item,
                   const int _weight_calc,
                   const std::string& _total_view,
                   const std::string& _norms_view,
                   const std::string& _norms_trfer_view) :
    WeightConcept::TPaidBagItem(item),
    weight_calc(_weight_calc),
    total_view(_total_view),
    norms_view(_norms_view),
    norms_trfer_view(_norms_trfer_view)
  {
    weight_rcpt=0;
  }
  void clear()
  {
    weight_calc=ASTRA::NoExists;
    total_view.clear();
    norms_view.clear();
    norms_trfer_view.clear();
    weight_rcpt=0;
  }
  std::string name_view(const std::string& lang="") const;
  std::string paid_view() const;
  std::string paid_calc_view() const;
  std::string paid_rcpt_view() const;
  const TPaidBagViewItem& toXML(xmlNodePtr node) const;
  const TPaidBagViewItem& GridInfoToXML(xmlNodePtr node, bool wide_format) const;
};

typedef std::map< TBagTypeKey, TPaidBagViewItem > TPaidBagViewMap;

#endif
