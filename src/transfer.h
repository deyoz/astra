#ifndef _TRANSFER_H_
#define _TRANSFER_H_

#include <vector>
#include <string>
#include <sstream>
#include "basic.h"
#include "astra_consts.h"
#include "astra_misc.h"

namespace TrferList
{

enum TTrferType { trferIn, trferOut, trferCkin, tckinInbound };

class TBagItem
{
  public:
    int bag_amount, bag_weight, rk_weight;
    std::string weight_unit;
    std::vector<TBagTagNumber> tags;
    TBagItem()
    {
      clear();
    };
    void clear()
    {
      bag_amount=ASTRA::NoExists;
      bag_weight=ASTRA::NoExists;
      rk_weight=ASTRA::NoExists;
      weight_unit.clear();
      tags.clear();
    };
    TBagItem& fromDB(TQuery &Qry, TQuery &TagQry, bool fromTlg, bool loadTags);
    TBagItem& setZero();
};

class TPaxItem
{
  public:
    std::string surname, name, name_extra;
    int seats;
    std::string subcl;
    TPaxItem()
    {
      clear();
    };
    void clear()
    {
      surname.clear();
      name.clear();
      name_extra.clear();
      seats=ASTRA::NoExists;
      subcl.clear();
    };
    bool operator < (const TPaxItem &pax) const
    {
      if (seats!=pax.seats)
      {
        int s1=seats==ASTRA::NoExists?1:(seats==0?0:-1);
        int s2=pax.seats==ASTRA::NoExists?1:(pax.seats==0?0:-1);
        if (s1!=s2)
          return s1<s2;
      };
      if (surname!=pax.surname)
        return surname<pax.surname;
      if (name!=pax.name)
        return name<pax.name;
      return name_extra<pax.name_extra;
    };
    TPaxItem& fromDB(TQuery &Qry, TQuery &RemQry, bool fromTlg);
    TPaxItem& setUnaccomp();
};

class TGrpItem : public TBagItem
{
  public:
    int point_id;
    int grp_id;
    int bag_pool_num;
    bool is_unaccomp;
    std::string airp_arv, subcl;
    std::string last_airp_arv; //только ради InboundTrfer::FilterUnattachedTags
    std::vector<TPaxItem> paxs;
    int seats;
    TGrpItem()
    {
      clear();
    };
    void clear()
    {
      TBagItem::clear();
      point_id=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      is_unaccomp=false;
      airp_arv.clear();
      subcl.clear();
      last_airp_arv.clear();
      paxs.clear();
      seats=ASTRA::NoExists;
    };
    TGrpItem& paxFromDB(TQuery &PaxQry, TQuery &RemQry, bool fromTlg);
    TGrpItem& setPaxUnaccomp();
};

class TFltInfo : public TTripInfo
{
  public:
    TFltInfo( TQuery &Qry, bool calc_local_time ) : TTripInfo(Qry)
    {
      if (scd_out!=ASTRA::NoExists)
      {
        if (calc_local_time)
          scd_out=UTCToLocal(scd_out,AirpTZRegion(airp));
        modf(scd_out,&scd_out);
      };
    };
    TFltInfo() : TTripInfo() {};
    bool operator < (const TFltInfo &flt) const
    {
      if (scd_out!=flt.scd_out)
        return scd_out<flt.scd_out;
      if (airline!=flt.airline)
        return airline<flt.airline;
      if (flt_no!=flt.flt_no)
        return flt_no<flt.flt_no;
      if (suffix!=flt.suffix)
        return suffix<flt.suffix;
      return airp<flt.airp;
    };
    bool operator != (const TFltInfo &flt) const
    {
      return (*this<flt || flt<*this);
    };
};

class TGrpViewItem : public TBagItem
{
  public:
    int point_id;
    std::string inbound_trip; //только для tckinInbound
    int grp_id;
    int bag_pool_num;
    TFltInfo flt_view;
    std::string airp_arv_view;
    std::string tlg_airp_view;
    std::string subcl_view;
    int subcl_priority;
    std::vector<TPaxItem> paxs;
    int seats;

    TGrpViewItem() : TBagItem()
    {
      point_id=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      subcl_priority=ASTRA::NoExists;
      seats=ASTRA::NoExists;
    };

    bool operator < (const TGrpViewItem &grp) const
    {
      if (flt_view!=grp.flt_view)
        return flt_view<grp.flt_view;
      if (point_id!=grp.point_id)
        return point_id<grp.point_id;
      if (airp_arv_view!=grp.airp_arv_view)
        return airp_arv_view<grp.airp_arv_view;
      if (subcl_priority!=grp.subcl_priority)
        return subcl_priority<grp.subcl_priority;
      if (subcl_view!=grp.subcl_view)
        return subcl_view<grp.subcl_view;
      if (grp_id!=grp.grp_id)
        return grp_id<grp.grp_id;
      return (bag_pool_num==ASTRA::NoExists?1000:bag_pool_num)<
             (grp.bag_pool_num==ASTRA::NoExists?1000:grp.bag_pool_num);
    };
};

void TrferFromDB(TTrferType type,
                 int point_id,
                 bool pr_bag,
                 TTripInfo &flt,
                 std::vector<TGrpItem> &grps_ckin,
                 std::vector<TGrpItem> &grps_tlg);

void TrferToXML(TTrferType type,
                int point_id,
                bool pr_bag,
                const TTripInfo &flt,
                const std::vector<TGrpItem> &grps_ckin,
                const std::vector<TGrpItem> &grps_tlg,
                xmlNodePtr resNode);

bool trferInExists(int point_arv, int prior_point_arv, TQuery& Qry);
bool trferOutExists(int point_dep, const TTripInfo &flt, TQuery& Qry);
bool trferCkinExists(int point_dep, TQuery& Qry);


}; //namespace TrferList

namespace InboundTrfer
{

class TPaxItem
{
  private:
    std::string translit_surname[2];
    std::string translit_name[2];
  public:
    std::string subcl;
    std::string surname, name;

    TPaxItem(const std::string &p1,
             const std::string &p2,
             const std::string &p3):subcl(p1),surname(p2),name(p3)
    {
      for(int fmt=1; fmt<=2; fmt++)
      {
        translit_surname[fmt-1]=transliter(surname, fmt, true);
        translit_name[fmt-1]=transliter(name, fmt, true);
      };
    };

    TPaxItem(const TrferList::TPaxItem &pax) : subcl(pax.subcl),surname(pax.surname),name(pax.name)
    {
      for(int fmt=1; fmt<=2; fmt++)
      {
        translit_surname[fmt-1]=transliter(surname, fmt, true);
        translit_name[fmt-1]=transliter(name, fmt, true);
      };
    };

    bool operator == (const TPaxItem &item) const
    {
      return subcl == item.subcl &&
             surname == item.surname &&
             name == item.name;
    };

    int equalRate(const TPaxItem &item) const;
};

class TGrpItem
{
  public:
    std::string airp_arv;
    bool is_unaccomp;
    std::vector<TPaxItem> paxs;
    std::list<TBagTagNumber> tags;

    TGrpItem():is_unaccomp(false) {};

    TGrpItem(const TrferList::TGrpItem &grp);

    bool operator == (const TGrpItem &item) const
    {
      return airp_arv == item.airp_arv &&
             is_unaccomp == item.is_unaccomp &&
             tags == item.tags &&
             (is_unaccomp || paxs == item.paxs);
    };

    int equalRate(const TGrpItem &item, int minPaxEqualRate) const;
};

class TGrpId : public std::pair<int/*grp_id*/, int/*bag_pool_num*/>
{
   public:
     TGrpId(int grp_id, int bag_pool_num) : std::pair<int, int>(grp_id, bag_pool_num) {};
     std::string getStr() const
     {
       std::ostringstream s;
       s << std::right << std::setw(10) << first << ":"
         << std::left << std::setw(3) << (second==ASTRA::NoExists?"-":IntToString(second));
       return s.str();
     };
};


typedef std::map<TGrpId, std::vector<TBagTagNumber> > TUnattachedTagMap;

void GetUnattachedTags(int point_id,
                       TUnattachedTagMap &result);

void GetCheckedTags(int id,  //¬.Ў. point_id Ё«Ё grp_id
                    bool is_point_id,
                    std::map<TGrpId, TGrpItem> &grps_out);

void GetUnattachedTags(int point_id,
                       const std::vector<TrferList::TGrpItem> &grps_ckin,
                       const std::vector<TrferList::TGrpItem> &grps_tlg,
                       TUnattachedTagMap &result);

}; //namespace InboundTrfer

namespace TrferListOld
{

void GetTransfer(bool pr_inbound_tckin, bool pr_out, bool pr_tlg, bool pr_bag, int point_id, xmlNodePtr resNode);

};

#endif /*_TRANSFER_H_*/
