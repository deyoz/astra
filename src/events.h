#ifndef _EVENTS_H_
#define _EVENTS_H_
#include "astra_consts.h"
#include "passenger.h"
#include "baggage.h"
#include "remarks.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "stat.h"

#include <sstream>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class EventsInterface : public JxtInterface
{
public:
  EventsInterface() : JxtInterface("","Events")
  {
     Handler *evHandle;
     evHandle=JxtHandler<EventsInterface>::CreateHandler(&EventsInterface::GetEvents);
     AddEvent("GetEvents",evHandle);
  };
  void GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

class TEventsBagItem : public CheckIn::TBagItem
{
  public:
    bool refused; //дополнительный признак, символизирующий что багаж принадлежит разрег. пассажирам
                  //не относится непосредственно к информации по багажу
    int pax_id;   //ид. пассажира, которому принадлежит багаж

    void clear()
    {
      CheckIn::TBagItem::clear();
      refused=false;
      pax_id=ASTRA::NoExists;
    }

    bool operator == (const TEventsBagItem &item) const
    {
      return id == item.id &&
             pr_cabin == item.pr_cabin &&
             amount == item.amount &&
             weight == item.weight &&
             pc == item.pc &&
             wt == item.wt &&
             using_scales == item.using_scales &&
             is_trfer == item.is_trfer &&
             handmade == item.handmade;
    }

    TEventsBagItem()
    {
      clear();
    }

    TEventsBagItem& fromDB(TQuery &Qry);
};

class TEventsSumBagKey
{
  public:
    std::string bag_type_view;
    bool is_trfer;
    void clear()
    {
      bag_type_view.clear();
      is_trfer=false;
    }
    TEventsSumBagKey()
    {
      clear();
    }
    bool operator == (const TEventsSumBagKey &item) const
    {
      return bag_type_view==item.bag_type_view &&
             is_trfer==item.is_trfer;
    }
    bool operator < (const TEventsSumBagKey &item) const
    {
      if (bag_type_view!=item.bag_type_view)
        return bag_type_view<item.bag_type_view;
      return (int)is_trfer<(int)item.is_trfer;
    }
};

class TEventsSumBagItem
{
  public:
    int amount, weight, paid;
    void clear()
    {
      amount=0;
      weight=0;
      paid=0;
    }
    bool empty() const
    {
      return amount==0 &&
             weight==0 &&
             paid==0;
    }
    TEventsSumBagItem()
    {
      clear();
    }
    bool operator == (const TEventsSumBagItem &item) const
    {
      return amount==item.amount &&
             weight==item.weight &&
             paid==item.paid;
    }
};

class TPaidToLogInfo
{
  public:
    int excess;
    std::map<TEventsSumBagKey, TEventsSumBagItem> bag;
    std::multiset<CheckIn::TServicePaymentItem> payment;
    void clear()
    {
      excess=0;
      bag.clear();
      payment.clear();
    }
    void add(const CheckIn::TBagItem& item);
    void add(const WeightConcept::TPaidBagItem& item);
    void add(const TPaidRFISCItem& item);
    void clearExcess()
    {
      excess=0;
      for(std::map<TEventsSumBagKey, TEventsSumBagItem>::iterator b=bag.begin(); b!=bag.end(); ++b)
        b->second.paid=0;
    }
    void trace( TRACE_SIGNATURE, const std::string &descr) const;
};

class TPaxToLogInfoKey
{
  public:
    int pax_id, reg_no;
    void clear()
    {
      pax_id=ASTRA::NoExists;
      reg_no=ASTRA::NoExists;
    };
    bool operator < (const TPaxToLogInfoKey &item) const
    {
      if (reg_no!=item.reg_no)
        return reg_no<item.reg_no;
      return pax_id<item.pax_id;
    };
    bool operator == (const TPaxToLogInfoKey &item) const
    {
      return reg_no == item.reg_no &&
             pax_id == item.pax_id;
    };
    TPaxToLogInfoKey()
    {
      clear();
    };
};

class TPaxToLogInfo
{
  public:
    std::string airp_arv, cl, status;
    bool pr_mark_norms;
    std::string surname, name, pers_type, refuse, subcl, seat_no;
    int seats;
    int is_female;
    bool pr_brd, pr_exam;
    CheckIn::TPaxTknItem tkn;
    CheckIn::TAPISItem apis;
    int bag_amount, bag_weight, rk_amount, rk_weight;
    std::string tags;
    std::list< std::pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
    std::map< std::string/*bag_type_view*/, WeightConcept::TNormItem> norms_normal;
    std::multiset<CheckIn::TPaxRemItem> rems;
    std::set<CheckIn::TPaxFQTItem> fqts;
    TPaidToLogInfo paid;
    TPaxToLogInfo()
    {
      clear();
    };
    void clear()
    {
      airp_arv.clear();
      cl.clear();
      status.clear();
      pr_mark_norms=false;
      surname.clear();
      name.clear();
      pers_type.clear();
      refuse.clear();
      subcl.clear();
      seat_no.clear();
      seats=ASTRA::NoExists;
      is_female=ASTRA::NoExists;
      pr_brd=false;
      pr_exam=false;
      tkn.clear();
      apis.clear();
      bag_amount=0;
      bag_weight=0;
      rk_amount=0;
      rk_weight=0;
      tags.clear();
      norms.clear();
      norms_normal.clear();
      rems.clear();
      paid.clear();
    };

    std::string getBagStr() const;
    void getBag(PrmEnum& param) const;
    void getPaxName(LEvntPrms& params) const;
    void getNorm(PrmEnum& param) const;
};

class TGrpToLogInfo
{
  public:
    int grp_id;
    int point_dep;
    bool trfer_confirm, piece_concept;
    std::map<TPaxToLogInfoKey, TPaxToLogInfo> pax;
    std::map<int/*id*/, TEventsBagItem> bag;
    TPaidToLogInfo paid;
    void clear()
    {
      grp_id=ASTRA::NoExists;
      point_dep=ASTRA::NoExists;
      trfer_confirm=false;
      piece_concept=false;
      pax.clear();
      bag.clear();
      paid.clear();
    };
    TGrpToLogInfo()
    {
      clear();
    }

    std::map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator findPax(int pax_id)
    {
      std::map<TPaxToLogInfoKey, TPaxToLogInfo>::iterator i=pax.begin();
      for(; i!=pax.end(); ++i)
        if (i->first.pax_id==pax_id) break;
      return i;
    }

    void clearExcess();
    void setExcess();
    void clearEmd();
    void setEmd();
};

class TAgentStatInfo
{
  public:
    int pax_amount;
    STAT::agent_stat_t dpax_amount;
    STAT::agent_stat_t dbag_amount;
    STAT::agent_stat_t dbag_weight;
    STAT::agent_stat_t drk_amount;
    STAT::agent_stat_t drk_weight;
    void clear()
    {
      pax_amount=0;
      dpax_amount.inc=0;
      dpax_amount.dec=0;
      dbag_amount.inc=0;
      dbag_amount.dec=0;
      dbag_weight.inc=0;
      dbag_weight.dec=0;
      drk_amount.inc=0;
      drk_amount.dec=0;
      drk_weight.inc=0;
      drk_weight.dec=0;
    };
    TAgentStatInfo():pax_amount(0),
                     dpax_amount(0,0),
                     dbag_amount(0,0),
                     dbag_weight(0,0),
                     drk_amount(0,0),
                     drk_weight(0,0) {};
};

std::string logPaxNameStr(const std::string &status,
                          const std::string &surname,
                          const std::string &name,
                          const std::string &pers_type);
void logPaxName(const std::string &status,
                          const std::string &surname,
                          const std::string &name,
                          const std::string &pers_type,
                          LEvntPrms& params);
void GetAPISLogMsgs(const CheckIn::TAPISItem &apisBefore,
                    const CheckIn::TAPISItem &apisAfter,
                    std::list<std::pair<std::string, std::string> > &msgs);

void GetBagToLogInfo(int grp_id, std::map<int/*id*/, TEventsBagItem> &bag);
void GetGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo);
void UpdGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo);
void SaveGrpToLog(const TGrpToLogInfo &grpInfoBefore,
                  const TGrpToLogInfo &grpInfoAfter,
                  const CheckIn::TPaidBagEMDProps &handmadeEMDDiff,
                  TAgentStatInfo &agentStat);
void SavePaidToLog(const TPaidToLogInfo &paidBefore,
                   const TPaidToLogInfo &paidAfter,
                   const TLogLocale &msgPattern,
                   bool piece_concept,
                   bool onlyEMD,
                   const CheckIn::TPaidBagEMDProps &handmadeEMDDiff);
//функция не только возвращает auto_weighing для пульта,
//но и пишет в лог, если для данного пульта изменилась настройка
bool GetAutoWeighing(int point_id, const std::string &work_mode);
bool GetAPISControl(int point_id);


#endif
