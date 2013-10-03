#ifndef _BAGGAGE_H_
#define _BAGGAGE_H_

#include <set>
#include <list>

#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"
#include "transfer.h"

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

class TBagItem
{
  public:
    int id,num;
    int bag_type;
    bool pr_cabin;
    int amount;
    int weight;
    int hall,user_id; //для old_bag
    int value_bag_num,bag_pool_num; //для new_bag
    bool pr_liab_limit, to_ramp;  //для new_bag
    bool using_scales;
    bool is_trfer;
    TBagItem()
    {
      clear();
    };
    void clear()
    {
      id=ASTRA::NoExists;
      num=ASTRA::NoExists;
      bag_type=ASTRA::NoExists;
      pr_cabin=false;
      amount=ASTRA::NoExists;
      weight=ASTRA::NoExists;
      hall=ASTRA::NoExists;
      user_id=ASTRA::NoExists;
      value_bag_num=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      pr_liab_limit=false;
      to_ramp=false;
      using_scales=false;
      is_trfer=false;
    };
    const TBagItem& toXML(xmlNodePtr node) const;
    TBagItem& fromXML(xmlNodePtr node);
    const TBagItem& toDB(TQuery &Qry) const;
    TBagItem& fromDB(TQuery &Qry);
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
  public:
    std::map<int /*num*/, TValueBagItem> vals;
    std::map<int /*num*/, TBagItem> bags;
    std::map<int /*num*/, TTagItem> tags;
    TGroupBagItem()
    {
      clear();
    };
    void clear()
    {
      vals.clear();
      bags.clear();
      tags.clear();
    };
    bool empty() const
    {
      return vals.empty() &&
             bags.empty() &&
             tags.empty();
    };
    bool fromXML(xmlNodePtr bagtagNode, bool &pr_tag_print);
    void fromXMLadditional(int point_id, int grp_id, int hall, bool pr_tag_print);
    void toDB(int grp_id) const;
    void fromDB(int grp_id, int bag_pool_num, bool without_refused);
    void toXML(xmlNodePtr bagtagNode) const;
    void add(const TGroupBagItem &item);
    void setInboundTrfer(const TrferList::TGrpItem &grp);
    void setPoolNum(int bag_pool_num);
};

void SaveBag(int point_id, int grp_id, int hall, xmlNodePtr bagtagNode);
void LoadBag(int grp_id, xmlNodePtr bagtagNode);

class TNormItem
{
  public:
    std::string norm_type;
    int amount;
    int weight;
    int per_unit;
  TNormItem()
  {
    clear();
  };
  void clear()
  {
    norm_type.clear();
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
    return norm_type.empty() &&
           amount==ASTRA::NoExists &&
           weight==ASTRA::NoExists &&
           per_unit==ASTRA::NoExists;
  };
  const TNormItem& toXML(xmlNodePtr node) const;
  TNormItem& fromDB(TQuery &Qry);
  std::string str() const;
};

class TPaxNormItem
{
  public:
    int bag_type;
    int norm_id;
    bool norm_trfer;
  TPaxNormItem()
  {
    clear();
  };
  void clear()
  {
    bag_type=ASTRA::NoExists;
    norm_id=ASTRA::NoExists;
    norm_trfer=false;
  };
  bool empty() const
  {
    return bag_type==ASTRA::NoExists &&
           norm_id==ASTRA::NoExists &&
           norm_trfer==false;
  };
  const TPaxNormItem& toXML(xmlNodePtr node) const;
  TPaxNormItem& fromXML(xmlNodePtr node);
  const TPaxNormItem& toDB(TQuery &Qry) const;
  TPaxNormItem& fromDB(TQuery &Qry);
};

class TPaidBagItem
{
  public:
    int bag_type;
    int weight;
    int rate_id;
    double rate;
    std::string rate_cur;
    bool rate_trfer;
  TPaidBagItem()
  {
    clear();
  };
  void clear()
  {
    bag_type=ASTRA::NoExists;
    weight=ASTRA::NoExists;
    rate_id=ASTRA::NoExists;
    rate=ASTRA::NoExists;
    rate_cur.clear();
    rate_trfer=false;
  };
  const TPaidBagItem& toXML(xmlNodePtr node) const;
  TPaidBagItem& fromXML(xmlNodePtr node);
  const TPaidBagItem& toDB(TQuery &Qry) const;
  TPaidBagItem& fromDB(TQuery &Qry);
};

bool PaidBagFromXML(xmlNodePtr paidbagNode,
                    std::list<TPaidBagItem> &paid);
void PaidBagToDB(int grp_id,
                 const std::list<TPaidBagItem> &paid);
void SavePaidBag(int grp_id, xmlNodePtr paidbagNode);
void LoadPaidBag(int grp_id, xmlNodePtr paidbagNode);

}; //namespace CheckIn

namespace BagPayment
{
  class TPaxInfo
  {
    public:
      std::string pax_cat;
      std::string target;
      std::string final_target;
      std::string subcl;
      std::string cl;
    TPaxInfo() {clear();};
    void clear()
    {
      pax_cat.clear();
      target.clear();
      final_target.clear();
      subcl.clear();
      cl.clear();
    };
  };

  void GetPaxBagNorm(TQuery &Qry,
                     const bool use_mixed_norms,
                     const TPaxInfo &pax,
                     const bool onlyCategory,
                     std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> &norm);

}; //namespace BagPayment

#endif

