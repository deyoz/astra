#ifndef _BAGGAGE_H_
#define _BAGGAGE_H_

#include <set>
#include <list>

#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "transfer.h"
#include <boost/optional.hpp>
#include <etick/tick_data.h>

enum TBagConcept { bcUnknown, bcPiece, bcWeight, bcNo };

class TBagNormUnit
{
  private:
    Ticketing::Baggage::Baggage_t unit;
  public:
    TBagNormUnit() : unit(Ticketing::Baggage::Nil) {}
    TBagNormUnit(const Ticketing::Baggage::Baggage_t &value) : unit(value) {}
    TBagNormUnit(const std::string &value) { set(value); }
    void clear()
    {
      unit=Ticketing::Baggage::Nil;
    }
    bool empty() const
    {
      return unit==Ticketing::Baggage::Nil;
    }
    void set(const Ticketing::Baggage::Baggage_t &value);
    void set(const std::string &value);
    Ticketing::Baggage::Baggage_t get() const;
    std::string get_db_form() const;
    std::string get_lexeme_form() const;
};

namespace CheckIn
{

class TValueBagItem
{
  public:
    int num;
    double value;
    std::string value_cur;
    int tax_id;
    double tax;
    bool tax_trfer;
    TValueBagItem()
    {
      clear();
    };
    void clear()
    {
      num=ASTRA::NoExists;
      value=ASTRA::NoExists;
      value_cur.clear();
      tax_id=ASTRA::NoExists;
      tax=ASTRA::NoExists;
      tax_trfer=false;
    };
    const TValueBagItem& toXML(xmlNodePtr node) const;
    TValueBagItem& fromXML(xmlNodePtr node);
    const TValueBagItem& toDB(TQuery &Qry) const;
    TValueBagItem& fromDB(TQuery &Qry);
};

class TUnaccompInfoItem
{
  public:
    int num;
    std::string original_tag_no;
    std::string surname;
    std::string name;
    std::string airline;
    int flt_no;
    std::string suffix;
    BASIC::TDateTime scd;
    TUnaccompInfoItem() {
      clear();
    }
    void clear() {
      num=ASTRA::NoExists;
      original_tag_no.clear();
      surname.clear();
      name.clear();
      airline.clear();
      flt_no = ASTRA::NoExists;
      suffix.clear();
      scd = ASTRA::NoExists;
    }
    bool isEmpty() const {
      return ( original_tag_no.empty() &&
               surname.empty() &&
               name.empty() &&
               airline.empty() &&
               flt_no == ASTRA::NoExists &&
               suffix.empty() &&
               scd == ASTRA::NoExists &&
               num == ASTRA::NoExists);
    }
    const TUnaccompInfoItem& toXML(xmlNodePtr node) const;
    TUnaccompInfoItem& fromXML(xmlNodePtr node);
    const TUnaccompInfoItem& toDB(TQuery &Qry) const;
    TUnaccompInfoItem& fromDB(TQuery &Qry);
};

class TUnaccompRuleItem
{
  public:
    std::string fieldname;
    int max_len;
    int min_len;
    bool empty;
    void clear() {
      fieldname.clear();
      empty = true;
    }
    TUnaccompRuleItem( const std::string &vfieldname, bool isempty, int vmin_len, int vmax_len ) {
      fieldname = vfieldname;
      empty = isempty;
      max_len = vmax_len;
      min_len = vmin_len;
    }
    bool operator < (const TUnaccompRuleItem &item) const
    {
      return fieldname < item.fieldname;
    }
    const TUnaccompRuleItem& toXML(xmlNodePtr node) const;
};

class TBagItem
{
  public:
    int id,num;
    int bag_type;
    std::string rfisc;
    bool pr_cabin;
    int amount;
    int weight;
    int hall,user_id; //для old_bag
    std::string desk;
    BASIC::TDateTime time_create;
    int value_bag_num,bag_pool_num; //для new_bag
    bool pr_liab_limit, to_ramp;  //для new_bag
    bool using_scales;
    bool is_trfer;
    bool handmade;
    TBagItem()
    {
      clear();
    };
    void clear()
    {
      id=ASTRA::NoExists;
      num=ASTRA::NoExists;
      bag_type=ASTRA::NoExists;
      rfisc.clear();
      pr_cabin=false;
      amount=ASTRA::NoExists;
      weight=ASTRA::NoExists;
      hall=ASTRA::NoExists;
      user_id=ASTRA::NoExists;
      desk.clear();
      time_create=ASTRA::NoExists;
      value_bag_num=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      pr_liab_limit=false;
      to_ramp=false;
      using_scales=false;
      is_trfer=false;
      handmade=true;
    };
    const TBagItem& toXML(xmlNodePtr node) const;
    TBagItem& fromXML(xmlNodePtr node, bool piece_concept);
    const TBagItem& toDB(TQuery &Qry) const;
    TBagItem& fromDB(TQuery &Qry);
    std::string bag_type_str() const;
    bool basicallyEqual(const TBagItem& item) const
    {
      return bag_type==item.bag_type &&
             rfisc==item.rfisc &&
             pr_cabin==item.pr_cabin &&
             amount==item.amount &&
             weight==item.weight &&
             is_trfer==item.is_trfer &&
             handmade==item.handmade;
    }
};

class TTagItem
{
  public:
    int num;
    std::string tag_type;
    int no_len;
    double no;
    std::string color;
    int bag_num;
    bool printable;
    bool pr_print;
    TTagItem()
    {
      clear();
    };
    void clear()
    {
      num=ASTRA::NoExists;
      tag_type.clear();
      no_len=ASTRA::NoExists;
      no=ASTRA::NoExists;
      color.clear();
      bag_num=ASTRA::NoExists;
      printable=false;
      pr_print=false;
    };
    const TTagItem& toXML(xmlNodePtr node) const;
    TTagItem& fromXML(xmlNodePtr node);
    const TTagItem& toDB(TQuery &Qry) const;
    TTagItem& fromDB(TQuery &Qry);
};

class TGroupBagItem
{
  private:
    void toLists(std::list<TValueBagItem> &vals_list,
                 std::list<TBagItem> &bags_list,
                 std::list<TTagItem> &tags_list) const;
    void fromLists(const std::list<TValueBagItem> &vals_list,
                   const std::list<TBagItem> &bags_list,
                   const std::list<TTagItem> &tags_list);
    static void normalizeLists(int vals_first_num,
                               int bags_first_num,
                               int tags_first_num,
                               std::list<TValueBagItem> &vals_list,
                               std::list<TBagItem> &bags_list,
                               std::list<TTagItem> &tags_list);
    void filterPools(const std::set<int/*bag_pool_num*/> &pool_nums,
                     bool pool_nums_for_keep);
    void fromXMLcompletion(int grp_id, int hall);
  public:
    std::map<int /*num*/, TValueBagItem> vals;
    std::map<int /*num*/, TBagItem> bags;
    std::map<int /*num*/, TTagItem> tags;
    std::map<int /*num*/, TUnaccompInfoItem> unaccomps;
    std::set<TUnaccompRuleItem> unaccomp_rules;
    bool pr_tag_print;
    TGroupBagItem()
    {
      clear();
    };
    void clear()
    {
      vals.clear();
      bags.clear();
      tags.clear();
      unaccomps.clear();
      unaccomp_rules.clear();
      pr_tag_print=false;
    };
    bool empty() const
    {
      return vals.empty() &&
             bags.empty() &&
             tags.empty();
    };
    bool fromXML(xmlNodePtr bagtagNode, int grp_id, int hall, bool piece_concept);
    void checkAndGenerateTags(int point_id, int grp_id);
    void toDB(int grp_id) const;
    void fromDB(int grp_id, int bag_pool_num, bool without_refused);
    void toXML(xmlNodePtr bagtagNode) const;
    void add(const TGroupBagItem &item);
    void setInboundTrfer(const TrferList::TGrpItem &grp);
    void setPoolNum(int bag_pool_num);
    bool trferExists() const;
    void convertBag(std::multimap<int, TBagItem> &result) const;
    void fillUnaccompRules();
};

class TPaidBagEMDItem
{
  public:
    int bag_type;
    std::string rfisc;
    int trfer_num;
    std::string emd_no;
    int emd_coupon;
    int weight;
    int pax_id;
  TPaidBagEMDItem()
  {
    clear();
  };
  void clear()
  {
    bag_type=ASTRA::NoExists;
    rfisc.clear();
    trfer_num=ASTRA::NoExists;
    emd_no.clear();
    emd_coupon=ASTRA::NoExists;
    weight=ASTRA::NoExists;
    pax_id=ASTRA::NoExists;
  };
  const TPaidBagEMDItem& toXML(xmlNodePtr node) const;
  TPaidBagEMDItem& fromXML(xmlNodePtr node, bool piece_concept);
  const TPaidBagEMDItem& toDB(TQuery &Qry) const;
  TPaidBagEMDItem& fromDB(TQuery &Qry);
  std::string no_str() const;
  std::string bag_type_str() const;
};

void PaidBagEMDFromXML(xmlNodePtr emdNode,
                       boost::optional<std::list<TPaidBagEMDItem> > &emd, bool piece_concept);
void PaidBagEMDToDB(int grp_id,
                    const boost::optional< std::list<TPaidBagEMDItem> > &emd);

void PaidBagEMDFromDB(int grp_id,
                      std::list<TPaidBagEMDItem> &emd);
void PaidBagEMDToXML(const std::list<TPaidBagEMDItem> &emd,
                     xmlNodePtr emdNode);

class TPaidBagEMDPropsItem
{
  public:
    std::string emd_no;
    int emd_coupon;
    bool manual_bind;
  TPaidBagEMDPropsItem(const std::string &_emd_no,
                       const int &_emd_coupon,
                       bool _manual_bind=false)
  {
    emd_no=_emd_no;
    emd_coupon=_emd_coupon;
    manual_bind=_manual_bind;
  }
  TPaidBagEMDPropsItem(const TPaidBagEMDItem &item, bool _manual_bind=false)
  {
    emd_no=item.emd_no;
    emd_coupon=item.emd_coupon;
    manual_bind=_manual_bind;
  }
  bool operator < (const TPaidBagEMDPropsItem &item) const
  {
    if(emd_no != item.emd_no)
      return emd_no < item.emd_no;
    return emd_coupon < item.emd_coupon;
  }
};

class TPaidBagEMDProps : public std::set<TPaidBagEMDPropsItem>
{
  public:
    TPaidBagEMDPropsItem get(const std::string &_emd_no,
                             const int &_emd_coupon) const
    {
      TPaidBagEMDPropsItem item(_emd_no, _emd_coupon);
      TPaidBagEMDProps::const_iterator i=find(item);
      return i!=end()?*i:item;
    }
};

void PaidBagEMDPropsFromDB(int grp_id, TPaidBagEMDProps &props);
void PaidBagEMDPropsToDB(int grp_id, const TPaidBagEMDProps &props);

} //namespace CheckIn

namespace WeightConcept
{

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
  };
  void clear()
  {
    norm_type=ASTRA::bntUnknown;
    amount=ASTRA::NoExists;
    weight=ASTRA::NoExists;
    per_unit=ASTRA::NoExists;
  };
  bool operator == (const TNormItem &item) const
  {
    return norm_type == item.norm_type &&
           amount == item.amount &&
           weight == item.weight &&
           per_unit == item.per_unit;
  };
  bool empty() const
  {
    return norm_type==ASTRA::bntUnknown &&
           amount==ASTRA::NoExists &&
           weight==ASTRA::NoExists &&
           per_unit==ASTRA::NoExists;
  };
  const TNormItem& toXML(xmlNodePtr node) const;
  TNormItem& fromDB(TQuery &Qry);
  std::string str(const std::string& lang) const;
  void GetNorms(PrmEnum& prmenum) const;
};

class TPaxNormItem
{
  public:
    int bag_type;
    int norm_id;
    bool norm_trfer;
    int handmade;
  TPaxNormItem()
  {
    clear();
  };
  void clear()
  {
    bag_type=ASTRA::NoExists;
    norm_id=ASTRA::NoExists;
    norm_trfer=false;
    handmade=ASTRA::NoExists;
  };
  bool empty() const
  {
    return bag_type==ASTRA::NoExists &&
           norm_id==ASTRA::NoExists &&
           norm_trfer==false;
  };
  bool operator == (const TPaxNormItem &item) const
  {
    return bag_type==item.bag_type &&
           norm_id==item.norm_id &&
           norm_trfer==item.norm_trfer;
  };
  const TPaxNormItem& toXML(xmlNodePtr node) const;
  TPaxNormItem& fromXML(xmlNodePtr node);
  const TPaxNormItem& toDB(TQuery &Qry) const;
  TPaxNormItem& fromDB(TQuery &Qry);
  std::string bag_type_str() const;
};

bool PaxNormsFromDB(BASIC::TDateTime part_key, int pax_id, std::list< std::pair<TPaxNormItem, TNormItem> > &norms);
bool GrpNormsFromDB(BASIC::TDateTime part_key, int grp_id, std::list< std::pair<TPaxNormItem, TNormItem> > &norms);
void NormsToXML(const std::list< std::pair<TPaxNormItem, TNormItem> > &norms, const CheckIn::TGroupBagItem &group_bag, xmlNodePtr node);
void PaxNormsToDB(int pax_id, const boost::optional< std::list<TPaxNormItem> > &norms);
void GrpNormsToDB(int grp_id, const boost::optional< std::list<TPaxNormItem> > &norms);
void ConvertNormsList(const std::list< std::pair<TPaxNormItem, TNormItem> > &norms,
                      std::map< std::string/*bag_type_view*/, TNormItem> &result);

class TPaidBagItem
{
  public:
    int bag_type;
    int weight;
    int rate_id;
    double rate;
    std::string rate_cur;
    bool rate_trfer;
    int handmade;
  TPaidBagItem()
  {
    clear();
  };
  void clear()
  {
    bag_type=ASTRA::NoExists;
    weight=0;
    rate_id=ASTRA::NoExists;
    rate=ASTRA::NoExists;
    rate_cur.clear();
    rate_trfer=false;
    handmade=ASTRA::NoExists;
  };
  bool operator == (const TPaidBagItem &item) const
  {
    return bag_type==item.bag_type &&
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
  std::string bag_type_str() const;
};

void PaidBagFromXML(xmlNodePtr paidbagNode,
                    boost::optional< std::list<TPaidBagItem> > &paid);
void PaidBagToDB(int grp_id,
                 const boost::optional< std::list<TPaidBagItem> > &paid);
void PaidBagFromDB(BASIC::TDateTime part_key, int grp_id, std::list<TPaidBagItem> &paid);
void PaidBagToXML(const std::list<TPaidBagItem> &paid, const CheckIn::TGroupBagItem &group_bag, xmlNodePtr paidbagNode);

} //namespace WeightConcept

#endif

