#ifndef _EVENTS_H_
#define _EVENTS_H_
#include "astra_consts.h"
#include "passenger.h"
#include "baggage.h"
#include "astra_misc.h"

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
    bool pr_brd, pr_exam;
    CheckIn::TPaxTknItem tkn;
    CheckIn::TPaxDocItem doc;
    CheckIn::TPaxDocoItem doco;
    int bag_amount, bag_weight, rk_amount, rk_weight;
    std::string tags;
    std::map< int/*bag_type*/, CheckIn::TNormItem> norms;
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
      pr_brd=false;
      pr_exam=false;
      tkn.clear();
      doc.clear();
      doco.clear();
      bag_amount=0;
      bag_weight=0;
      rk_amount=0;
      rk_weight=0;
      tags.clear();
      norms.clear();
    };
    std::string getBagStr() const;
    std::string getPaxNameStr() const;
    std::string getNormStr() const;
};

class TPaidToLogInfo
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
    bool operator == (const TPaidToLogInfo &item) const
    {
      return bag_type == item.bag_type &&
             bag_amount== item.bag_amount &&
             bag_weight== item.bag_weight &&
             paid_weight== item.paid_weight;
    };
    TPaidToLogInfo()
    {
      clear();
    };
};

class TGrpToLogInfo
{
  public:
    int grp_id, excess;
    std::map< TPaxToLogInfoKey, TPaxToLogInfo> pax;
    std::map< int/*bag_type*/, TPaidToLogInfo> paid;
    void clear()
    {
      grp_id=ASTRA::NoExists;
      excess=0;
      pax.clear();
      paid.clear();
    };
    TGrpToLogInfo()
    {
      clear();
    };
};

void GetGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo);
void SaveGrpToLog(int point_id,
                  const TTripInfo &operFlt,
                  const TTripInfo &markFlt,
                  const TGrpToLogInfo &grpInfoBefore,
                  const TGrpToLogInfo &grpInfoAfter);


#endif
