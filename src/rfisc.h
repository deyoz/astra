#ifndef _RFISC_H_
#define _RFISC_H_

#include <list>
#include <string>
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "oralib.h"
#include "baggage_base.h"

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

class TEmdType  //только для совместимости со старым протоколом
{
  public:
    static const std::list< std::pair<std::string, std::string> >& pairs()
    {
      static std::list< std::pair<std::string, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair("A", "EMD-A"));
        l.push_back(std::make_pair("S", "EMD-S"));
        l.push_back(std::make_pair("O", "OTHR"));
      }
      return l;
    }
};

class TEmdTypes : public ASTRA::PairList<std::string, std::string>
{
  private:
    virtual std::string className() const { return "TEmdTypes"; }
  public:
    TEmdTypes() : ASTRA::PairList<std::string, std::string>(TEmdType::pairs(),
                                                            std::string(""),
                                                            std::string("")) {}
};

const TEmdTypes& EmdTypes();

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

    const TRFISCListKey& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
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

    boost::optional<bool> carry_on() const;
    TServiceCategory::Enum calc_category() const;
    const std::string& name_view(const std::string& lang="") const;
    const std::string descr_view(const std::string& lang="") const;

    const TRFISCListItem& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
    TRFISCListItem& fromSirenaXML(xmlNodePtr node);

    const TRFISCListItem& toXML(xmlNodePtr node, const boost::optional<TRFISCListItem> &def=boost::none) const;
    const TRFISCListItem& toDB(TQuery &Qry, bool old_version) const;
    TRFISCListItem& fromDB(TQuery &Qry, bool old_version);
};

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

    const TRFISCKey& toSirenaXML(xmlNodePtr node, const std::string &lang) const;

    const TRFISCKey& toXML(xmlNodePtr node) const;
    const TRFISCKey& toDB(TQuery &Qry) const;
    TRFISCKey& fromDB(TQuery &Qry);
    void getListItemIfNone();
    void getListItem();
    void getListItemInboundTrferTmp(const std::string &where); //!!! потом удалить
    bool getPseudoListIdForInboundTrfer(int grp_id);
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
    boost::optional<TRFISCListKey> getBagRFISCListKey(const std::string &RFISC, const bool &carry_on) const;
    void fromSirenaXML(xmlNodePtr node);
    void toXML(int list_id, xmlNodePtr node) const;
    void fromDB(int list_id, bool old_version, bool only_visible=false); // загрузка только списка RFISC
    void toDB(int list_id, bool old_version) const; // сохранение только списка RFISC
    int crc(bool old_version, const std::string &baggage_airline="") const;
    int toDBAdv(bool old_version) const; //продвинутое сохранение с анализом существующих справочников
    std::string localized_name(const TRFISCListKey &key, const std::string& lang) const; //локализованное описание RFISC
    bool exists(const TServiceType::Enum service_type) const;
    bool equal(const TRFISCList &list, bool old_version, const std::string &baggage_airline="") const;
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
    std::string get_rem_code(const std::string &airline,
                             const std::string &rfisc) const;
};

class TRFISCListWithProps : public TRFISCList
{
  private:
    boost::optional<TRFISCBagPropsList> bagProps;
  public:
    TRFISCListWithProps()
    {
      clear();
    }
    void clear()
    {
      TRFISCList::clear();
      bagProps=boost::none;
    }
    std::string get_rem_code(const std::string &rfisc, const bool pr_cabin);
    const TRFISCBagPropsList& getBagProps();
    void setPriority();
};

class TRFISCListWithPropsCache : std::map<int/*list_id*/, TRFISCListWithProps>
{
  public:
    const TRFISCBagPropsList& getBagProps(int list_id);
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

class TPaxServiceListsItem : public TPaxServiceListsKey
{
  public:
    int list_id;

    TPaxServiceListsItem() { clear(); }
    void clear()
    {
      TPaxServiceListsKey::clear();
      list_id=ASTRA::NoExists;
    }

    const TPaxServiceListsItem& toDB(TQuery &Qry) const;
    TPaxServiceListsItem& fromDB(TQuery &Qry);
    const TPaxServiceListsItem& toXML(xmlNodePtr node) const;
    std::string traceStr() const;
};

class TPaxServiceLists : public std::set<TPaxServiceListsItem>
{
  public:
    void fromDB(int id, bool is_unaccomp, bool emul_nonexistent_lists);
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

    const TPaxSegRFISCKey& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
    TPaxSegRFISCKey& fromSirenaXML(xmlNodePtr node);

    const TPaxSegRFISCKey& toXML(xmlNodePtr node) const;
    TPaxSegRFISCKey& fromXML(xmlNodePtr node);
    const TPaxSegRFISCKey& toDB(TQuery &Qry) const;
    TPaxSegRFISCKey& fromDB(TQuery &Qry);
};

class TGrpServiceItem : public TPaxSegRFISCKey
{
  public:
    int service_quantity;

    TGrpServiceItem() { clear(); }
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
};

class TGrpServiceList : public std::list<TGrpServiceItem>
{
  public:
    void fromDB(int grp_id, bool without_refused=false);
    void toDB(int grp_id) const;
    bool fromXML(xmlNodePtr node);
    void toXML(xmlNodePtr node) const;
    void addBagInfo(int grp_id,
                    int tckin_seg_count,
                    int trfer_seg_count,
                    bool include_refused);
    void prepareForSirena(int grp_id,
                          int tckin_seg_count,
                          int trfer_seg_count,
                          bool include_refused);
    void getAllListItems();
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
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
};

typedef std::list<TPaidRFISCStatus> TPaidRFISCStatusList;

class TPaidRFISCItem : public TGrpServiceItem
{
  public:
    int paid;
    int need;

    TPaidRFISCItem() { clear(); }
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
    void addStatusList(TPaidRFISCStatusList &list) const;
};

class TPaidRFISCList : public std::map<TPaxSegRFISCKey, TPaidRFISCItem>
{
  public:
    void fromDB(int id, bool is_grp_id);
    void toDB(int grp_id) const;
    void inc(const TPaxSegRFISCKey& key, const TServiceStatus::Enum status);
    boost::optional<TRFISCKey> getKeyIfSingleRFISC(int pax_id, const std::string &rfisc) const;
    void getAllListItems();
    void getStatusList(TPaidRFISCStatusList &list) const;
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
    static void updateExcess(int grp_id);
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

void CalcPaidRFISCView(const TPaidRFISCList &paid,
                       TPaidRFISCViewMap &paid_view);

void PaidRFISCViewToXML(const TPaidRFISCViewMap &paid_view, xmlNodePtr node);

bool RFISCPaymentCompleted(int grp_id, int pax_id, bool only_tckin_segs);

void GetBagConcepts(int grp_id, bool &pc, bool &wt, bool &rfisc_used);
void GetBagConceptsCompatible(int grp_id, bool &pc, bool &wt, int &bag_types_id); //!!! потом удалить

#endif
