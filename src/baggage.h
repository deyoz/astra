#ifndef _BAGGAGE_H_
#define _BAGGAGE_H_

#include <set>
#include <list>

#include "astra_consts.h"
#include "astra_misc.h"
#include "oralib.h"
#include "xml_unit.h"
#include "transfer.h"
#include "baggage_wt.h"
#include "rfisc.h"
#include <boost/optional.hpp>
#include <etick/tick_data.h>
#include "date_time.h"

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
    TDateTime scd;
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

class TSimpleBagItem
{
  private:
      std::string get_rem_code_internal(TRFISCListWithPropsCache &lists, bool pr_ldm) const;
  public:
    boost::optional<TRFISCKey> pc;
    boost::optional<TBagTypeKey> wt;
    int amount;
    int weight;

    TSimpleBagItem()
    {
      clear();
    }

    void clear()
    {
      pc=boost::none;
      wt=boost::none;
      amount=ASTRA::NoExists;
      weight=ASTRA::NoExists;
    }

    bool operator == (const TSimpleBagItem &item) const
    {
      return pc==item.pc &&
             wt==item.wt &&
             amount==item.amount &&
             weight==item.weight;
    }

    const TSimpleBagItem& toDB(TQuery &Qry) const;
    TSimpleBagItem& fromDB(TQuery &Qry);
    std::string get_rem_code_lci(TRFISCListWithPropsCache &lists) const;
    std::string get_rem_code_ldm(TRFISCListWithPropsCache &lists) const;
};

class TBagItem : public TSimpleBagItem
{
  public:
    int id,num;
    bool pr_cabin;
    int hall,user_id; //��� old_bag
    std::string desk;
    TDateTime time_create;
    int value_bag_num,bag_pool_num; //��� new_bag
    bool pr_liab_limit, to_ramp;  //��� new_bag
    bool using_scales;
    bool is_trfer;
    bool handmade;
    boost::optional<TUnaccompInfoItem> unaccompInfo;
    TBagItem()
    {
      clear();
    }
    void clear()
    {
      TSimpleBagItem::clear();
      id=ASTRA::NoExists;
      num=ASTRA::NoExists;
      pr_cabin=false;
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
      unaccompInfo=boost::none;
    }
    const TBagItem& toXML(xmlNodePtr node) const;
    TBagItem& fromXML(xmlNodePtr node, bool baggage_pc);
    const TBagItem& toDB(TQuery &Qry) const;
    TBagItem& fromDB(TQuery &Qry);
    bool basicallyEqual(const TBagItem& item) const
    {
      return TSimpleBagItem::operator ==(item) &&
             pr_cabin==item.pr_cabin &&
             is_trfer==item.is_trfer &&
             handmade==item.handmade;
    }
    void check(TRFISCListWithPropsCache &lists) const;
    void check(const TRFISCBagPropsList &list) const;

    std::string key_str(const std::string& lang="") const;
    std::string key_str_compatible() const;

    std::string tag_printer_id(bool is_lat) const;
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

class TValueBagMap : public std::map<int /*num*/, TValueBagItem>
{
  public:
    static std::string clearSQLText();
    void toDB(int grp_id) const;
    void fromDB(int grp_id);
    void toXML(xmlNodePtr bagtagNode) const;
};

class TBagMap : public std::map<int /*num*/, TBagItem>
{
  public:
    static std::string clearSQLText();
    void toDB(int grp_id) const;
    void fromDB(int grp_id);
    void toXML(xmlNodePtr bagtagNode) const;
    void procInboundTrferFromBTM(const TrferList::TGrpItem &grp);
    void dump(const std::string &where);
    void add(const TUnaccompInfoItem& unaccomp, bool throwIfProblem);
};

class TTagMap : public std::map<int /*num*/, TTagItem>
{
  public:
    static std::string clearSQLText();
    void toDB(int grp_id) const;
    void fromDB(int grp_id);
    void toXML(xmlNodePtr bagtagNode) const;
};

class TUnaccompRules : public std::set<TUnaccompRuleItem>
{
  public:
    TUnaccompRules();
    void toXML(xmlNodePtr bagtagNode) const;
};

class TGroupBagItem
{
  private:
    void toLists(std::list<TValueBagItem> &vals_list,
                 std::list<TBagItem> &bags_list,
                 std::list<TTagItem> &tags_list) const;
    void addFromLists(const std::list<TValueBagItem> &vals_list,
                      const std::list<TBagItem> &bags_list,
                      const std::list<TTagItem> &tags_list);
    void replaceFromLists(const std::list<TValueBagItem> &vals_list,
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
    void fromXMLcompletion(int grp_id, int hall, bool is_unaccomp, bool trfer_confirm);
  public:
    TValueBagMap vals;
    TBagMap bags;
    TTagMap tags;
    TTagMap generated_tags;
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
      generated_tags.clear();
      pr_tag_print=false;
    };
    bool empty() const
    {
      return vals.empty() &&
             bags.empty() &&
             tags.empty();
    };
    bool fromXMLsimple(xmlNodePtr bagtagNode, bool baggage_pc);
    bool fromXML(xmlNodePtr bagtagNode, int grp_id, int hall, bool is_unaccomp, bool baggage_pc, bool trfer_confirm);
    void checkAndGenerateTags(int point_id, int grp_id, bool generateAndDefer=false);
    static void clearDB(int grp_id, bool isPayment);
    void toDB(int grp_id) const;
    void fromDB(int grp_id, int bag_pool_num, bool without_refused);
    void generatedToXML(xmlNodePtr bagtagNode) const;
    void toXML(xmlNodePtr bagtagNode, bool is_unaccomp) const;
    void add(const TGroupBagItem &item);
    void add(int point_id, const TBagTagNumber& tag, const TSimpleBagItem& bag, int& pax_pool_num);
    void remove(const TBagTagNumber& tag, int& pax_pool_num);
    void setInboundTrfer(const TrferList::TGrpItem &grp);
    void setPoolNum(int bag_pool_num);
    int getNewPoolNum() const;
    bool isPoolNumUsed(int bag_pool_num) const;
    void convertBag(std::multimap<int, TBagItem> &result) const;
    void getAllListKeys(const int grp_id, const bool is_unaccomp, const int transfer_num);
    void getAllListItems(const int grp_id, const bool is_unaccomp, const int transfer_num);

    static bool completeXMLForIatci(int grp_id, xmlNodePtr bagtagNode, xmlNodePtr firstSegNode);
    static bool tagNumberUsedInGroup(int pax_id, const TBagTagNumber& tag, int& tagOwner/*pax_id*/);
    static void checkTagUniquenessOnFlight(int grp_id);
    static void copyPaxPool(const GrpId_t& src, const GrpId_t& dest);
    static void copyPaxPool(const std::list<TCkinRouteInsertItem> &tckinGroups);
    static void copyDB(const GrpId_t& src, const GrpId_t& dest);
};

bool unknownTagTypeOnFlight(int point_id);

} //namespace CheckIn

void GridInfoToXML(const TTrferRoute &trfer,
                   const TPaidRFISCViewMap &paidRFISC,
                   const TPaidBagViewMap &paidBag,
                   const TPaidBagViewMap &trferBag,
                   xmlNodePtr node);

#endif

