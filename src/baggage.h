#ifndef _BAGGAGE_H_
#define _BAGGAGE_H_

#include "astra_consts.h"
#include "oralib.h"
#include "xml_unit.h"

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
    bool operator < (const TValueBagItem &item) const
    {
      return num < item.num;
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
    };
    bool operator < (const TBagItem &item) const
    {
      return num < item.num;
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
    int seg_no;
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
      seg_no=ASTRA::NoExists;
    };
    bool operator < (const TTagItem &item) const
    {
      return num < item.num;
    };
    const TTagItem& toXML(xmlNodePtr node) const;
    TTagItem& fromXML(xmlNodePtr node);
    const TTagItem& toDB(TQuery &Qry) const;
    TTagItem& fromDB(TQuery &Qry);
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

