#ifndef _TRANSFER_H_
#define _TRANSFER_H_

#include <vector>
#include <string>
#include <sstream>
#include "date_time.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "baggage_tags.h"

namespace CheckIn
{
class TPaxTransferItem;
class TTransferItem;
class TTransferList;

TAdvTripInfo routeInfoFromTrfr(const CheckIn::TTransferItem& seg);

class TPaxTransferItem
{
  public:
    int pax_id;
    std::string subclass;
    TElemFmt subclass_fmt;
    TPaxTransferItem()
    {
      pax_id=ASTRA::NoExists;
      subclass_fmt=efmtUnknown;
    }
    TPaxTransferItem(const std::string& _subclass,
                     TElemFmt _subclass_fmt) :
      pax_id(ASTRA::NoExists),
      subclass(_subclass),
      subclass_fmt(_subclass_fmt) {}
};

class TTransferItem
{
  public:
    std::string flight_view;
    TTripInfo operFlt;
    int local_date;
    int grp_id;
    std::string airp_arv;
    TElemFmt airp_arv_fmt;
    std::string subclass;
    TElemFmt subclass_fmt;
    std::vector<TPaxTransferItem> pax;
    TTransferItem()
    {
      local_date=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      airp_arv_fmt=efmtUnknown;
      subclass_fmt=efmtUnknown;
    }
    bool Valid() const
    {
      return !operFlt.airline.empty() &&
             operFlt.flt_no!=0 &&
             !operFlt.airp.empty() &&
             operFlt.scd_out!=ASTRA::NoExists &&
             !airp_arv.empty();
    }
    bool equalSeg(const TTransferItem &item) const
    {
      return operFlt.airline==item.operFlt.airline &&
             operFlt.flt_no==item.operFlt.flt_no &&
             operFlt.suffix==item.operFlt.suffix &&
             operFlt.scd_out==item.operFlt.scd_out &&
             operFlt.airp==item.operFlt.airp &&
             airp_arv==item.airp_arv;
    }
    bool equalSubclasses(const TTransferItem &item) const;
};

class TTransferList : public std::vector<TTransferItem>
{
  private:
    enum CheckType {checkNone,checkFirstSeg,checkAllSeg};
  public:
    void load(int grp_id);
    void check(int id, bool isGrpId, int seg_no) const;
    void parseSegments(xmlNodePtr trferNode,
                       const AirportCode_t& airpArv,
                       const TDateTime scd_out_local);
    void parseSubclasses(xmlNodePtr paxNode);
};

void PaxTransferFromDB(int pax_id, std::list<TPaxTransferItem> &trfer);
void PaxTransferToXML(const std::list<TPaxTransferItem> &trfer, xmlNodePtr paxNode);
void PaxTransferToDB(int pax_id, int pax_no, const CheckIn::TTransferList &trfer, int seg_no);

FltOperFilter createFltOperFilter(const CheckIn::TTransferItem &item);

}; //namespace CheckIn

namespace TrferList
{

enum TTrferType { trferIn, trferOut, trferOutForCkin, trferCkin, tckinInbound };

/*
trferIn
���ଠ�� � �࠭����� ���ᠦ���, �ਡ뢠��� ३ᮬ [flight%s][EOL]
���ଠ�� � �࠭��୮� ������, �ਡ뢠�饬 ३ᮬ [flight%s][EOL]

trferOut
���ଠ�� � �࠭����� ���ᠦ���, ��ࠢ������� ३ᮬ [flight%s][EOL]
���ଠ�� � �࠭��୮� ������, ��ࠢ���饬�� ३ᮬ [flight%s][EOL]

trferCkin
���ଠ�� � �࠭����� ���ᠦ���, ��ࠢ������� ३ᮬ [flight%s][EOL](����� �� �᭮�� १���⮢ ॣ����樨)
���ଠ�� � �࠭��୮� ������, ��ࠢ���饬�� ३ᮬ [flight%s][EOL](����� �� �᭮�� १���⮢ ॣ����樨)

tckinInbound
��몮���� ३��, ����묨 �ਡ뢠�� ���ᠦ��� ३� [flight%s][EOL](����� �� �᭮�� ᪢����� ॣ����樨)

trferOutForCkin
�ᯮ������ � GetNewGrpTags
*/

class TBagItem
{
  public:
    int bag_amount, bag_weight, rk_weight;
    std::string weight_unit;
    std::multiset<TBagTagNumber> tags;
    std::map<int, CheckIn::TTransferItem> trfer;
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
      trfer.clear();
    };
    TBagItem& fromDB(DB::TQuery &Qry, bool fromTlg, bool loadTags);
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
    TPaxItem& fromDB(DB::TQuery &Qry, bool fromTlg);
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
    std::string last_airp_arv; //⮫쪮 ࠤ� InboundTrfer::FilterUnattachedTags
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
    TGrpItem& paxFromDB(DB::TQuery &PaxQry, bool fromTlg);
    TGrpItem& trferFromDB(bool fromTlg);
    TGrpItem& setPaxUnaccomp();
};

class TFltInfo : public TTripInfo
{
  private:
    void trunc_scd_out( bool calc_local_time )
    {
      if (scd_out!=ASTRA::NoExists)
      {
        if (calc_local_time)
          scd_out = BASIC::date_time::UTCToLocal(scd_out,AirpTZRegion(airp));
        modf(scd_out,&scd_out);
      };
    }
  public:
    TFltInfo( const TAdvTripInfo &flt, bool calc_local_time  ) : TTripInfo(flt)
    {
      trunc_scd_out(calc_local_time);
    }
    TFltInfo( TQuery &Qry, bool calc_local_time ) : TTripInfo(Qry)
    {
      trunc_scd_out(calc_local_time);
    }
    TFltInfo( DB::TQuery &Qry, bool calc_local_time ) : TTripInfo(Qry)
    {
      trunc_scd_out(calc_local_time);
    }
    TFltInfo() : TTripInfo() {}
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

enum class Alarm
{
    Unattached,
    InPaxDuplicate,
    OutPaxDuplicate,
    InRouteIncomplete,
    InRouteDiffer,
    InOutRouteDiffer,
    WeightNotDefined,
};

class AlarmTypes : public ASTRA::PairList<Alarm, std::string>
{
  private:
    virtual std::string className() const { return "TrferList::AlarmTypes"; }
  public:
    AlarmTypes() : ASTRA::PairList<Alarm, std::string>(
                     {{Alarm::Unattached,        "TRFER_UNATTACHED"          },
                      {Alarm::InPaxDuplicate,    "TRFER_IN_PAX_DUPLICATE"    },
                      {Alarm::OutPaxDuplicate,   "TRFER_OUT_PAX_DUPLICATE"   },
                      {Alarm::InRouteIncomplete, "TRFER_IN_ROUTE_INCOMPLETE" },
                      {Alarm::InRouteDiffer,     "TRFER_IN_ROUTE_DIFFER"     },
                      {Alarm::InOutRouteDiffer,  "TRFER_IN_OUT_ROUTE_DIFFER" },
                      {Alarm::WeightNotDefined,  "TRFER_WEIGHT_NOT_DEFINED"  }},
                     boost::none,
                     boost::none) {}
    static AlarmTypes& instance()
    {
      static AlarmTypes alarmTypes;
      return alarmTypes;
    }
};

typedef std::map<TGrpId, std::set<Alarm> > TAlarmTagMap;

class TGrpConfirmItem
{
  public:
    int grp_id;
    int bag_pool_num;
    int bag_weight;
    std::vector<std::string> tag_ranges;
    int calc_status;
    int conf_status;
    TGrpConfirmItem()
    {
      grp_id=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      bag_weight=ASTRA::NoExists;
      calc_status=ASTRA::NoExists;
      conf_status=ASTRA::NoExists;
    };
};

class TGrpViewItem : public TBagItem
{
  public:
    int point_id;
    std::string inbound_trip; //⮫쪮 ��� tckinInbound
    int grp_id;
    int bag_pool_num;
    TFltInfo flt_view;
    std::string airp_arv_view;
    std::string tlg_airp_view;
    std::string subcl_view;
    int subcl_priority;
    std::vector<TPaxItem> paxs;
    int seats;
    std::set<Alarm> alarms;
    int calc_status;

    TGrpViewItem() : TBagItem()
    {
      point_id=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      bag_pool_num=ASTRA::NoExists;
      subcl_priority=ASTRA::NoExists;
      seats=ASTRA::NoExists;
      calc_status=ASTRA::NoExists;
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

void GrpsToGrpsView(TTrferType type,
                    bool pr_bag,
                    const std::vector<TGrpItem> &grps_ckin,
                    const std::vector<TGrpItem> &grps_tlg,
                    const TAlarmTagMap &alarms,
                    std::vector<TGrpViewItem> &grps);

void TrferToXML(TTrferType type,
                const std::vector<TGrpViewItem> &grps,
                xmlNodePtr trferNode);

void TrferToXML(TTrferType type,
                int point_id,
                bool pr_bag,
                const TTripInfo &flt,
                const std::vector<TGrpItem> &grps_ckin,
                const std::vector<TGrpItem> &grps_tlg,
                xmlNodePtr resNode);

void TrferConfirmFromXML(xmlNodePtr trferNode,
                         std::map<TGrpId, TGrpConfirmItem> &grps);

bool trferInExists(int point_arv, int prior_point_arv);
bool trferOutExists(int point_dep, TQuery& Qry);
bool trferCkinExists(int point_dep, TQuery& Qry);

std::set<TrferId_t> loadTrferIdSet(const PointIdTlg_t& point_id);
std::set<TrferGrpId_t> loadTrferGrpIdSet(const TrferId_t& trfer_id);
std::set<TrferGrpId_t> loadTrferGrpIdSet(const PointIdTlg_t& point_id);

std::set<TrferId_t> loadTrferIdsByTlgTransferIn(const PointIdTlg_t& point_id_in,
                                                const std::string& tlg_type);
std::set<PointId_t> loadPointIdsSppByTlgTransferIn(const PointIdTlg_t& point_id_in);

bool deleteTrferPax(const TrferGrpId_t& grp_id);
bool deleteTrferTags(const TrferGrpId_t& grp_id);
bool deleteTlgTrferOnwards(const TrferGrpId_t& grp_id);
bool deleteTlgTrferExcepts(const TrferGrpId_t& grp_id);
bool deleteTrferGrp(const TrferId_t& trfer_id);
bool deleteTlgTransfer(const TrferId_t& trfer_id);
bool deleteTlgTransfer(const PointIdTlg_t& point_id);

void deleteTransferData(const PointIdTlg_t& point_id);

}; //namespace TrferList

namespace InboundTrfer
{

typedef TrferList::TGrpId TGrpId;

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
    std::multiset<TBagTagNumber> tags;
    std::map<int, CheckIn::TTransferItem> trfer;

    TGrpItem():is_unaccomp(false) {};

    TGrpItem(const TrferList::TGrpItem &grp);

    bool operator == (const TGrpItem &item) const
    {
      //�-�� �ᯮ������ ⮫쪮 � check_u_trfer_alarm_for_grp �� �ࠢ����� ᫥��� ��ப �� � ��᫥
      return airp_arv == item.airp_arv &&
             is_unaccomp == item.is_unaccomp &&
             tags == item.tags &&
//             trfer == item.trfer &&     �� �ࠢ����� ᫥���� ���쭥�訩 �࠭���� ������� ��ப �� ���뢠��,
//                                        �� �����, �ଠ�쭮, ������ �ࠢ������ ��ꥪ�� ���������, �������� � ���饬
             (is_unaccomp || paxs == item.paxs);
    };

    int equalRate(const TGrpItem &item, int minPaxEqualRate) const;
    bool equalTrfer(const TGrpItem &item) const;
    bool similarTrfer(const TGrpItem &item, bool checkSubclassesEquality) const;
    bool addSubclassesForEqualTrfer(const TGrpItem &item);
    void printTrfer(const std::string &title, bool printSubclasses=false) const;
    void print() const;
    bool alreadyCheckedIn(int point_id) const;
    void normalizeTrfer();
};

enum class ConflictReason { InPaxDuplicate,
                            OutPaxDuplicate,
                            InRouteIncomplete,
                            InRouteDiffer,
                            OutRouteDiffer,
                            InOutRouteDiffer,
                            WeightNotDefined,
                            OutRouteWithErrors,
                            OutRouteTruncated,
                          };

bool isGlobalConflict(ConflictReason c);
TrferList::Alarm GetConflictAlarm(ConflictReason c);

typedef std::map<TGrpId, std::list<TBagTagNumber> > TUnattachedTagMap;

typedef std::map<TGrpId, std::pair<TrferList::TGrpItem, std::set<ConflictReason> > > TNewGrpTagMap;
typedef std::map<std::pair<std::string/*surname*/, std::string/*name*/>, std::set<TGrpId> > TNewGrpPaxMap;

class TNewGrpInfo
{
  public:
    TNewGrpTagMap tag_map;
    TNewGrpPaxMap pax_map;
    std::set<ConflictReason> conflicts;
    void clear()
    {
      tag_map.clear();
      pax_map.clear();
      conflicts.clear();
    };
    void erase(const TGrpId &id);
    int calc_status(const TGrpId &id) const;
};

class ConflictReasons
{
  private:
    std::set<ConflictReason> conflicts;
    bool emptyInboundBaggage;
  public:
    void set(const TNewGrpInfo& info)
    {
      conflicts=info.conflicts;
      emptyInboundBaggage=info.tag_map.empty();
    }

    bool isInboundBaggageConflict() const
    {
      return !conflicts.empty() && !emptyInboundBaggage;
    }

    void toLog(const TLogLocale &pattern) const;
};


void GetUnattachedTags(int point_id,
                       TUnattachedTagMap &result);

void GetCheckedTags(int id,  //�.�. point_id ��� grp_id
                    ASTRA::TIdType id_type,
                    std::map<TGrpId, TGrpItem> &grps_out);

void GetUnattachedTags(int point_id,
                       const std::vector<TrferList::TGrpItem> &grps_ckin,
                       const std::vector<TrferList::TGrpItem> &grps_tlg,
                       TUnattachedTagMap &result);

void GetNextTrferCheckedFlts(int id,  //�.�. point_id ��� grp_id
                             ASTRA::TIdType id_type,
                             std::set<int> &point_ids);

void GetNewGrpInfo(int point_id,
                   const TGrpItem &grp_out,
                   const std::set<int> &pax_out_ids,
                   TNewGrpInfo &info);

void NewGrpInfoToGrpsView(const TNewGrpInfo &inbound_trfer,
                          std::vector<TrferList::TGrpViewItem> &grps);

}; //namespace InboundTrfer

#endif /*_TRANSFER_H_*/
