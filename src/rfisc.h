#ifndef _RFISC_H_
#define _RFISC_H_

#include <list>
#include <string>
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "oralib.h"
#include "baggage_base.h"
#include "remarks.h"

//какая-то фигня с определением кодировки этого файла, поэтому я вставил эту фразу

class TServiceType
{
  public:
    enum Enum
    {
      FlightRelated,
      RuleBuster,
      TicketRelated,
      Merchendaise,
      BaggageCharge,
      BaggagePrepaid,
      Unknown
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(FlightRelated,  "F"));
        l.push_back(std::make_pair(RuleBuster,     "R"));
        l.push_back(std::make_pair(TicketRelated,  "T"));
        l.push_back(std::make_pair(Merchendaise,   "M"));
        l.push_back(std::make_pair(BaggageCharge,  "C"));
        l.push_back(std::make_pair(BaggagePrepaid, "P"));
        l.push_back(std::make_pair(Unknown,        ""));
      }
      return l;
    }

    static const std::list< std::pair<Enum, std::string> >& view_pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(FlightRelated,  "FlightRelated"));
        l.push_back(std::make_pair(RuleBuster,     "RuleBuster"));
        l.push_back(std::make_pair(TicketRelated,  "TicketRelated"));
        l.push_back(std::make_pair(Merchendaise,   "Merchendaise"));
        l.push_back(std::make_pair(BaggageCharge,  "BaggageCharge"));
        l.push_back(std::make_pair(BaggagePrepaid, "BaggagePrepaid"));
        l.push_back(std::make_pair(Unknown,        "Unknown"));
      }
      return l;
    }
};

class TServiceTypes : public ASTRA::PairList<TServiceType::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TServiceTypes"; }
  public:
    TServiceTypes() : ASTRA::PairList<TServiceType::Enum, std::string>(TServiceType::pairs(),
                                                                       TServiceType::Unknown,
                                                                       boost::none) {}
};

class TServiceTypesView : public ASTRA::PairList<TServiceType::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TServiceTypesView"; }
  public:
    TServiceTypesView() : ASTRA::PairList<TServiceType::Enum, std::string>(TServiceType::view_pairs(),
                                                                           boost::none,
                                                                           boost::none) {}
};

const TServiceTypes& ServiceTypes();

class TServiceStatus
{
  public:
    enum Enum
    {
      Unknown,
      Free,
      Paid,
      Need,
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(Unknown, "unknown"));
        l.push_back(std::make_pair(Free,    "free"));
        l.push_back(std::make_pair(Paid,    "paid"));
        l.push_back(std::make_pair(Need,    "need"));
      }
      return l;
    }
};

class TServiceStatuses : public ASTRA::PairList<TServiceStatus::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TServiceStatuses"; }
  public:
    TServiceStatuses() : ASTRA::PairList<TServiceStatus::Enum, std::string>(TServiceStatus::pairs(),
                                                                            boost::none,//TServiceStatus::None,
                                                                            boost::none) {}
};

const TServiceStatuses& ServiceStatuses();

class TRFISCListKey
{
  public:
    std::string RFISC;
    TServiceType::Enum service_type;
    std::string airline;

    TRFISCListKey() { clear(); }

    TRFISCListKey(const std::string &_RFISC,
                  const TServiceType::Enum &_service_type,
                  const std::string &_airline) :
      RFISC(_RFISC), service_type(_service_type), airline(_airline) {}

    void clear()
    {
      RFISC.clear();
      service_type=TServiceType::Unknown;
      airline.clear();
    }

    bool operator == (const TRFISCListKey &key) const // сравнение
    {
      return RFISC==key.RFISC &&
             service_type==key.service_type &&
             airline==key.airline;
    }

    bool operator < (const TRFISCListKey &key) const
    {
      if (RFISC!=key.RFISC)
        return RFISC<key.RFISC;
      if (service_type!=key.service_type)
        return service_type<key.service_type;
      return airline<key.airline;
    }

    bool empty() const
    {
      return *this==TRFISCListKey();
    }

    std::string str(const std::string& lang="") const;

    const TRFISCListKey& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    TRFISCListKey& fromSirenaXML(xmlNodePtr node);

    const TRFISCListKey& toXML(xmlNodePtr node) const;
    TRFISCListKey& fromXML(xmlNodePtr node);
    TRFISCListKey& fromXMLcompatible(xmlNodePtr node);
    const TRFISCListKey& toDB(TQuery &Qry) const;
    TRFISCListKey& fromDB(TQuery &Qry);
    const TRFISCListKey& key() const { return *this; }
    void key(const TRFISCListKey& _key) { *this=_key; }
};

class TRFISCListItem : public TRFISCListKey
{
  public:
    std::string RFIC;
    std::string emd_type;
    std::string grp;
    std::string subgrp;
    std::string name;
    std::string name_lat;
    std::string descr1;
    std::string descr2;
    TServiceCategory::Enum category;
    bool visible;
    boost::optional<int> priority;

    TRFISCListItem()
    {
      clear();
    }
    TRFISCListItem(const CheckIn::TPaxASVCItem& item)
    {
      clear();
      RFISC=item.RFISC;
      RFIC=item.RFIC;
      name=item.service_name;
      name_lat=item.service_name;
      category=TServiceCategory::Other;
      visible=true;
    }

    void clear()
    {
      TRFISCListKey::clear();
      RFIC.clear();
      emd_type.clear();
      grp.clear();
      subgrp.clear();
      name.clear();
      name_lat.clear();
      descr1.clear();
      descr2.clear();
      category=TServiceCategory::Other;
      visible=false;
      priority=boost::none;
    }

    bool operator == (const TRFISCListItem &item) const // сравнение
    {
      return TRFISCListKey::operator ==(item) &&
             RFIC==item.RFIC &&
             emd_type==item.emd_type &&
             grp==item.grp &&
             subgrp==item.subgrp &&
             name==item.name &&
             name_lat==item.name_lat &&
             descr1==item.descr1 &&
             descr2==item.descr2 &&
             category==item.category &&
             visible==item.visible;
    }

    bool isCarryOn() const;
    bool isBaggageOrCarryOn() const;
    TServiceCategory::Enum calc_category() const;
    bool isBaggageInCabinOrCarryOn() const;
    bool isBaggageInHold() const;
    const std::string& name_view(const std::string& lang="") const;
    const std::string descr_view(const std::string& lang="") const;

    const TRFISCListItem& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    TRFISCListItem& fromSirenaXML(xmlNodePtr node);

    const TRFISCListItem& toXML(xmlNodePtr node, const boost::optional<TRFISCListItem> &def=boost::none) const;
    const TRFISCListItem& toDB(TQuery &Qry) const;
    TRFISCListItem& fromDB(TQuery &Qry);
    std::string traceStr() const;
};

typedef std::list<TRFISCListItem> TRFISCListItems;

class TRFISCKey : public TRFISCListKey
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
    boost::optional<TRFISCListItem> list_item;

    TRFISCKey() { clear(); }

    void clear()
    {
      TRFISCListKey::clear();
      list_id=ASTRA::NoExists;
      list_item=boost::none;
    }

    const TRFISCKey& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;

    const TRFISCKey& toXML(xmlNodePtr node) const;
    const TRFISCKey& toDB(TQuery &Qry) const;
    TRFISCKey& fromDB(TQuery &Qry);
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
    bool isBaggageOrCarryOn(const std::string &where) const;

    bool is_auto_service() const
    {
      return service_type==TServiceType::Unknown;
    }

    void getListItemsAuto(int pax_id, int transfer_num, const std::string& rfic,
                          TRFISCListItems& items) const;

    void getListItemLastSimilar();
};

class ServiceListId
{
  private:
    int term_list_id;
  public:
    int list_id;
    int additional;

    ServiceListId() { clear(); }
    ServiceListId(int _list_id)
    {
      clear();
      list_id=_list_id;
    }

    void clear()
    {
      list_id=ASTRA::NoExists;
      additional=ASTRA::NoExists;
      term_list_id=ASTRA::NoExists;
    }

    int primary() const { return list_id; }
    int forTerminal() const { return term_list_id!=ASTRA::NoExists?-term_list_id:list_id; }

    const ServiceListId& toDB(TQuery &Qry) const;
    ServiceListId& fromDB(TQuery &Qry);
    const ServiceListId& toXML(xmlNodePtr node) const;
    ServiceListId& fromXML(xmlNodePtr node);
};

typedef std::map<TRFISCListKey, TRFISCListItem> TRFISCListMap;

class TRFISCList : public TRFISCListMap
{
  protected:
    Statistic<TServiceType::Enum> service_type_stat;
    Statistic<std::string> airline_stat;
    Statistic<TServiceCategory::Enum> category_stat;
    Statistic< boost::optional<int> > priority_stat;
    void update_stat(const TRFISCListItem &item);
    void recalc_stat();
  public:
    void fromSirenaXML(xmlNodePtr node);
    void toXML(int list_id, xmlNodePtr node) const;
    void fromDB(const ServiceListId& list_id, bool only_visible=false); // загрузка только списка RFISC
    void toDB(int list_id) const; // сохранение только списка RFISC
    int crc() const;
    int toDBAdv() const; //продвинутое сохранение с анализом существующих справочников
    std::string localized_name(const TRFISCListKey &key, const std::string& lang) const; //локализованное описание RFISC
    bool exists(const TServiceType::Enum service_type) const;
    bool exists(const bool carryOn) const;
    bool equal(const TRFISCList &list) const;
    void dump() const;
};

class TRFISCBagProps
{
  public:
    std::string airline;
    std::string RFISC;
    int priority;
    int min_weight, max_weight;
    std::string rem_code;

    TRFISCBagProps()
    {
      clear();
    }
    void clear()
    {
      airline.clear();
      RFISC.clear();
      priority=ASTRA::NoExists;
      min_weight=ASTRA::NoExists;
      max_weight=ASTRA::NoExists;
      rem_code.clear();
    }

    TRFISCBagProps& fromDB(TQuery &Qry);
};

class TRFISCBagPropsList : public std::map<TRFISCListKey, TRFISCBagProps>
{
  public:
    TRFISCBagPropsList()
    {
      clear();
    }

    void fromDB(const std::set<std::string> &airlines);
};

class TRFISCListWithProps : public TRFISCList
{
  private:
    boost::optional<TRFISCBagPropsList> bagPropsList;
  public:
    TRFISCListWithProps()
    {
      clear();
    }
    void clear()
    {
      TRFISCList::clear();
      bagPropsList=boost::none;
    }
    void bagPropsFromDB();
    void fromDB(const ServiceListId& list_id, bool only_visible=false);
    void setPriority();

    const TRFISCBagPropsList& getBagPropsList();
    boost::optional<TRFISCBagProps> getBagProps(const TRFISCListKey& key) const;
};

class TRFISCListWithPropsCache : std::map<int/*list_id*/, TRFISCListWithProps>
{
  public:
    const TRFISCBagPropsList& getBagPropsList(int list_id);
};

class TPaxServiceListsKey : public Sirena::TPaxSegKey
{
  public:
    TServiceCategory::Enum category;

    TPaxServiceListsKey() { clear(); }
    void clear()
    {
      TPaxSegKey::clear();
      category=TServiceCategory::Other;
    }
    bool operator < (const TPaxServiceListsKey &key) const
    {
      if (!TPaxSegKey::operator ==(key))
        return TPaxSegKey::operator <(key);
      return category<key.category;
    }

    const TPaxServiceListsKey& toDB(TQuery &Qry) const;
    TPaxServiceListsKey& fromDB(TQuery &Qry);
};

class TPaxServiceListsItem : public TPaxServiceListsKey, public ServiceListId
{
  public:
    TPaxServiceListsItem() { clear(); }
    void clear()
    {
      TPaxServiceListsKey::clear();
      ServiceListId::clear();
    }

    const TPaxServiceListsItem& toDB(TQuery &Qry) const;
    TPaxServiceListsItem& fromDB(TQuery &Qry);
    const TPaxServiceListsItem& toXML(xmlNodePtr node) const;
    std::string traceStr() const;
};

class TPaxServiceLists : public std::set<TPaxServiceListsItem>
{
  public:
    void fromDB(int id, bool is_unaccomp);
    void toDB(bool is_unaccomp) const;
    void toXML(int id, bool is_unaccomp, int tckin_seg_count, xmlNodePtr node);
};

class TPaxSegRFISCKey : public Sirena::TPaxSegKey, public TRFISCKey
{
  public:
    TPaxSegRFISCKey() { clear(); }
    TPaxSegRFISCKey(const TPaxSegKey &key1, const TRFISCKey &key2) : TPaxSegKey(key1), TRFISCKey(key2) {}
    void clear()
    {
      TPaxSegKey::clear();
      TRFISCKey::clear();
    }
    bool operator < (const TPaxSegRFISCKey &key) const
    {
      if (!TPaxSegKey::operator ==(key))
        return TPaxSegKey::operator <(key);
      return TRFISCKey::operator <(key);
    }
    bool operator == (const TPaxSegRFISCKey &key) const
    {
      return TPaxSegKey::operator == (key) &&
             TRFISCKey::operator ==(key);
    }

    const TPaxSegRFISCKey& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    TPaxSegRFISCKey& fromSirenaXML(xmlNodePtr node);

    const TPaxSegRFISCKey& toXML(xmlNodePtr node) const;
    TPaxSegRFISCKey& fromXML(xmlNodePtr node);
    const TPaxSegRFISCKey& toDB(TQuery &Qry) const;
    TPaxSegRFISCKey& fromDB(TQuery &Qry);
    std::string traceStr() const;
};

class TGrpServiceAutoItem : public Sirena::TPaxSegKey, public CheckIn::TPaxASVCItem
{
  public:
    void clear()
    {
      TPaxSegKey::clear();
      TPaxASVCItem::clear();
    }
    TGrpServiceAutoItem() { clear(); }
    TGrpServiceAutoItem(const TPaxSegKey &key, const TPaxASVCItem &item) : TPaxSegKey(key), TPaxASVCItem(item) {}
    TGrpServiceAutoItem& fromDB(TQuery &Qry);
    const TGrpServiceAutoItem& toDB(TQuery &Qry) const;
    bool isSuitableForAutoCheckin() const
    {
      return RFIC!="C";
    }
    bool isEMD() const { return withEMD(); }
    bool permittedForAutoCheckin(const TTripInfo &flt) const;
    bool equalWithoutEMD(const TGrpServiceAutoItem& item) const;
    TServiceStatus::Enum serviceStatus() const { return withEMD()?TServiceStatus::Paid:TServiceStatus::Free; }
};

class TGrpServiceItem : public TPaxSegRFISCKey
{
  public:
    int service_quantity;

    TGrpServiceItem() { clear(); }
    TGrpServiceItem(const TGrpServiceAutoItem& item);
    TGrpServiceItem(const TPaxSegRFISCKey& _item, const int _service_quantity) :
      TPaxSegRFISCKey(_item), service_quantity(_service_quantity) {}
    void clear()
    {
      TPaxSegRFISCKey::clear();
      service_quantity=ASTRA::NoExists;
    }

    const TGrpServiceItem& toXML(xmlNodePtr node) const;
    TGrpServiceItem& fromXML(xmlNodePtr node);
    const TGrpServiceItem& toDB(TQuery &Qry) const;
    TGrpServiceItem& fromDB(TQuery &Qry);
    bool service_quantity_valid() const { return service_quantity!=ASTRA::NoExists && service_quantity>0; }
    bool similar(const TGrpServiceAutoItem& item) const
    {
      TGrpServiceItem tmp(item);
      return TPaxSegKey::operator == (tmp) &&
             RFISC == tmp.RFISC &&
             list_item && tmp.list_item && list_item.get().RFIC==tmp.list_item.get().RFIC;
    }
};

class TGrpServiceAutoList : public std::list<TGrpServiceAutoItem>
{
  public:
    TGrpServiceAutoList& fromDB(int id, bool is_grp_id, bool without_refused=false);
    void toDB(int grp_id) const;
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest, bool not_clear=false);
    bool sameDocExists(const CheckIn::TPaxASVCItem& asvc) const;
    bool removeEqualWithoutEMD(const TGrpServiceAutoItem& item);
    void replaceWithoutEMDFrom(const TGrpServiceAutoList& list);
};

class TGrpServiceList : public std::list<TGrpServiceItem>
{
  public:
    void fromDB(int grp_id, bool without_refused=false);
    void toDB(int grp_id) const;

    void addBagInfo(int grp_id,
                    int tckin_seg_count,
                    int trfer_seg_count,
                    bool include_refused);
    void addBagList(const TGrpServiceList& bagList,
                    int tckin_seg_count,
                    int trfer_seg_count);
    void addTrueBagInfo(const TGrpServiceItem& item);
    void getAllListItems();
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
    void mergeWith(const TGrpServiceAutoList& list);
    void moveFrom(TGrpServiceAutoList& list);
};

class TGrpServiceListWithAuto : public std::list<TGrpServiceItem>
{
  public:
    void addItem(const TGrpServiceAutoItem& svcAuto);
    void fromDB(int grp_id, bool without_refused=false);
    bool fromXML(xmlNodePtr node);
    void toXML(xmlNodePtr node) const;
    void split(int grp_id, TGrpServiceList& list1, TGrpServiceAutoList& list2) const;
};

class TPaidRFISCStatus : public TPaxSegRFISCKey
{
  public:
    TServiceStatus::Enum status;

    TPaidRFISCStatus() { clear(); }
    TPaidRFISCStatus(const TPaxSegRFISCKey& _item, const TServiceStatus::Enum _status) :
      TPaxSegRFISCKey(_item), status(_status) {}
    void clear()
    {
      TPaxSegRFISCKey::clear();
      status=TServiceStatus::Unknown;
    }
    bool operator == (const TPaidRFISCStatus &item) const
    {
      return TPaxSegRFISCKey::operator == (item) &&
             status == item.status;
    }
    std::string traceStr() const;
};

class TPaidRFISCItem : public TGrpServiceItem
{
  public:
    int paid;
    int need;

    TPaidRFISCItem() { clear(); }
    TPaidRFISCItem(const TGrpServiceAutoItem& item) :
      TGrpServiceItem(item), paid(item.withEMD()?item.service_quantity:0), need(0) {}
    TPaidRFISCItem(const TGrpServiceItem& item):TGrpServiceItem(item)
    {
      paid=ASTRA::NoExists;
      need=ASTRA::NoExists;
    }

    void clear()
    {
      TGrpServiceItem::clear();
      paid=ASTRA::NoExists;
      need=ASTRA::NoExists;
    }

    const TPaidRFISCItem& toXML(xmlNodePtr node) const;
    const TPaidRFISCItem& toDB(TQuery &Qry) const;
    TPaidRFISCItem& fromDB(TQuery &Qry);
    bool paid_positive() const { return paid!=ASTRA::NoExists && paid>0; }
    bool need_positive() const { return need!=ASTRA::NoExists && need>0; }
};

class TPaidRFISCStatusList : public std::list<TPaidRFISCStatus>
{
  public:
    void add(const TPaidRFISCItem& item);
    bool deleteIfFound(const TPaidRFISCStatus& item);
    template <class T>
    void set(const T& container)
    {
      clear();
      for(const auto& i : container) add(i.second);
    }
};

class TPaidRFISCList : public std::map<TPaxSegRFISCKey, TPaidRFISCItem>
{
  public:
    void fromDB(int id, bool is_grp_id);
    void toDB(int grp_id) const;
    void inc(const TPaxSegRFISCKey& key, const TServiceStatus::Enum status);
    boost::optional<TRFISCKey> getKeyIfSingleRFISC(int pax_id, int trfer_num, const std::string &rfisc) const;
    void getAllListItems();
    bool becamePaid(int grp_id) const;
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
    static void updateExcess(int grp_id);
};

class TRFISCListItemsCache
{
  private:
    mutable std::map<TPaxSegRFISCKey, TRFISCListItems> secret_map; //:)
  protected:
    bool isRFISCGrpExists(const TPaxSegRFISCKey& key,
                          const std::string &grp, const std::string &subgrp) const;
    std::string getRFISCNameIfUnambiguous(const TPaxSegRFISCKey &key,
                                          const std::string& lang) const;
  public:
    void getRFISCListItems(const TPaxSegRFISCKey& key,
                           TRFISCListItems& items) const;
    void dumpCache() const;
    void clearCache() { secret_map.clear(); }
    std::string getRFISCName(const TPaxSegRFISCKey& item, const std::string& lang="") const;
};

class TPaidRFISCListWithAuto : public std::map<TPaxSegRFISCKey, TPaidRFISCItem>, public TRFISCListItemsCache
{
  public:
    void addItem(const TGrpServiceAutoItem& svcAuto, bool squeeze);
    TPaidRFISCListWithAuto &set(const TPaidRFISCList& list1,
                                const TGrpServiceAutoList& list2,
                                bool squeeze=false);
    void fromDB(int id, bool is_grp_id, bool squeeze=false);
    bool isRFISCGrpExists(int pax_id, const std::string &grp, const std::string &subgrp) const;
    bool isRFISCGrpNeedForPayment(int pax_id, const std::string &grp, const std::string &subgrp) const;
    void getUniqRFISCSs(int pax_id, std::set<std::string> &rfisc_set) const;
};

class TPaidRFISCViewKey : public TRFISCKey
{
  public:
    int trfer_num;
    TPaidRFISCViewKey(const TPaidRFISCItem &item) : TRFISCKey(item), trfer_num(item.trfer_num) {}
    bool operator < (const TPaidRFISCViewKey &key) const
    {
      if (!TRFISCKey::operator ==(key))
        return TRFISCKey::operator <(key);
      return trfer_num<key.trfer_num;
    }
    bool operator == (const TPaidRFISCViewKey &key) const
    {
      return TRFISCKey::operator ==(key) &&
             trfer_num==key.trfer_num;
    }
    const TPaidRFISCViewKey& toXML(xmlNodePtr node) const;
};

class TPaidRFISCViewItem : public TPaidRFISCViewKey
{
  public:
    int service_quantity;
    int paid;
    int paid_rcpt;
    TPaidRFISCViewItem(const TPaidRFISCItem &item) : TPaidRFISCViewKey(item)
    {
      service_quantity=0;
      paid=0;
      paid_rcpt=0;
      add(item);
    }
    void add(const TPaidRFISCItem &item)
    {
      if (!TPaidRFISCViewKey::operator ==(item)) return;
      service_quantity+=(item.service_quantity!=ASTRA::NoExists?item.service_quantity:0);
      if (item.paid!=ASTRA::NoExists)
        paid+=item.paid;
      if (item.paid!=ASTRA::NoExists && item.need!=ASTRA::NoExists)
        paid_rcpt+=item.paid>=item.need?item.paid-item.need:0;
    }
    std::string name_view(const std::string& lang="") const;
    const TPaidRFISCViewItem& toXML(xmlNodePtr node) const;
    const TPaidRFISCViewItem& GridInfoToXML(xmlNodePtr node,
                                            const TTrferRoute &trfer) const;
    int priority() const;
};

typedef std::map<TPaidRFISCViewKey, TPaidRFISCViewItem> TPaidRFISCViewMap;

void CalcPaidRFISCView(const TPaidRFISCListWithAuto &paid,
                       TPaidRFISCViewMap &paid_view);

void PaidRFISCViewToXML(const TPaidRFISCViewMap &paid_view, xmlNodePtr node);

bool RFISCPaymentCompleted(int grp_id, int pax_id, bool only_tckin_segs);

void GetBagConcepts(int grp_id, bool &pc, bool &wt, bool &rfisc_used);

class RFISCCallbacks
{
    public:
        virtual ~RFISCCallbacks() {}
        virtual void afterRFISCChange(int grp_id) = 0;
};

#endif
