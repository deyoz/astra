#ifndef _EVENTS_H_
#define _EVENTS_H_
#include "astra_consts.h"

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

class TBagToLogPaxInfoKey
{
  public:
    int pax_id, reg_no;
    void clear()
    {
      pax_id=ASTRA::NoExists;
      reg_no=ASTRA::NoExists;
    };
    bool operator < (const TBagToLogPaxInfoKey &item) const
    {
      if (reg_no!=item.reg_no)
        return reg_no<item.reg_no;
      return pax_id<item.pax_id;
    };
    bool operator == (const TBagToLogPaxInfoKey &item) const
    {
      return reg_no == item.reg_no &&
             pax_id == item.pax_id;
    };
    TBagToLogPaxInfoKey()
    {
      clear();
    };
};

class TBagToLogPaxInfo
{
  public:
    std::string surname, name, pers_type, refuse;
    int bag_amount, bag_weight, rk_amount, rk_weight;
    std::string tags;
    void clear()
    {
      refuse.clear();
      bag_amount=0;
      bag_weight=0;
      rk_amount=0;
      rk_weight=0;
      tags.clear();
    };
    bool operator == (const TBagToLogPaxInfo &item) const
    {
      return refuse == item.refuse &&
             bag_amount == item.bag_amount &&
             bag_weight == item.bag_weight &&
             rk_amount == item.rk_amount &&
             rk_weight == item.rk_weight &&
             tags == item.tags;
    };
    TBagToLogPaxInfo()
    {
      clear();
    };
    std::string getBagStr() const
    {
      std::ostringstream msg;
      if (bag_amount!=0 || bag_weight!=0)
      {
        if (!msg.str().empty()) msg << ", ";
        msg << "багаж " << bag_amount << "/" << bag_weight;
      };
      if (rk_amount!=0 || rk_weight!=0)
      {
        if (!msg.str().empty()) msg << ", ";
        msg << "р/кладь " << rk_amount << "/" << rk_weight;
      };
      if (!tags.empty())
      {
        if (!msg.str().empty()) msg << ", ";
        msg << "бирки " << tags;
      };
      return msg.str();
    };
    std::string getPaxStr() const
    {
      std::ostringstream msg;
      if (pers_type.empty())
        msg << "Багаж без сопровождения";
      else
        msg << "Пассажир " << surname << (name.empty()?"":" ") << name
                           << " (" << pers_type << ")";
      return msg.str();
    };
};

class TBagToLogPaidInfo
{
  public:
    int bag_type, bag_amount, bag_weight, paid_weight;
    void clear()
    {
      bag_type=ASTRA::NoExists;
      bag_amount=0;
      bag_weight=0;
      paid_weight=0;
    };
    bool operator == (const TBagToLogPaidInfo &item) const
    {
      return bag_type == item.bag_type &&
             bag_amount== item.bag_amount &&
             bag_weight== item.bag_weight &&
             paid_weight== item.paid_weight;
    };
    TBagToLogPaidInfo()
    {
      clear();
    };
};

class TBagToLogGrpInfo
{
  public:
    int grp_id, excess;
    std::map< TBagToLogPaxInfoKey, TBagToLogPaxInfo> pax;
    std::map< int/*bag_type*/, TBagToLogPaidInfo> paid;
    void clear()
    {
      grp_id=ASTRA::NoExists;
      excess=0;
      pax.clear();
      paid.clear();
    };
    bool operator == (const TBagToLogGrpInfo &item) const
    {
      return grp_id == item.grp_id &&
             excess == item.excess &&
             pax == item.pax &&
             paid == item.paid;
    };
    TBagToLogGrpInfo()
    {
      clear();
    };
};

void GetBagToLogInfo(int grp_id, TBagToLogGrpInfo &grpInfo);
void SaveBagToLog(int point_id, const TBagToLogGrpInfo &grpInfoBefore,
                                const TBagToLogGrpInfo &grpInfoAfter);


#endif
