#ifndef _ETICK_H_
#define _ETICK_H_

#include "jxtlib/JxtInterface.h"
#include "jxtlib/xmllibcpp.h"
#include "astra_utils.h"
#include "astra_ticket.h"
#include "astra_misc.h"
#include "xml_unit.h"
#include "tlg/EdifactRequest.h"

namespace edifact{
    class RemoteResults;
}//namespace edifact


class ETSearchInterface : public JxtInterface
{
public:
  ETSearchInterface() : JxtInterface("ETSearchForm","ETSearchForm")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::SearchETByTickNo);
     AddEvent("SearchETByTickNo",evHandle);
     AddEvent("TickPanel",evHandle);
     AddEvent("kick", JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::KickHandler));
  }

  void SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ETChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
};

class EMDSearchInterface : public JxtInterface
{
public:
    EMDSearchInterface() : JxtInterface("", "EMDSearch")
    {
        AddEvent("EMDTextView",      JXT_HANDLER(EMDSearchInterface, EMDTextView));
        AddEvent("SearchEMDByDocNo", JXT_HANDLER(EMDSearchInterface, SearchEMDByDocNo));
        AddEvent("kick",             JXT_HANDLER(EMDSearchInterface, KickHandler));
    }

    void EMDTextView(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void SearchEMDByDocNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
};

class TEMDSystemUpdateItem
{
  public:
    std::string airline_oper;
    Ticketing::FlightNum_t flt_no_oper;
    Ticketing::TicketCpn_t et;
    Ticketing::TicketCpn_t emd;
    Ticketing::CpnStatAction::CpnStatAction_t action;
    std::string traceStr() const;
};

typedef std::list< std::pair<TEMDSystemUpdateItem, XMLDoc> > TEMDSystemUpdateList;

class EMDSystemUpdateInterface : public JxtInterface
{
public:
    EMDSystemUpdateInterface() : JxtInterface("", "EMDSystemUpdate")
    {
        AddEvent("DisassociateEMD", JXT_HANDLER(EMDSystemUpdateInterface, SysUpdateEmdCoupon));
        AddEvent("AssociateEMD",    JXT_HANDLER(EMDSystemUpdateInterface, SysUpdateEmdCoupon));        
        AddEvent("kick",            JXT_HANDLER(EMDSystemUpdateInterface, KickHandler));        
    }

    void SysUpdateEmdCoupon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void MakeAnAnswer(xmlNodePtr resNode, boost::shared_ptr<edifact::RemoteResults> remRes);    
    static void EMDCheckDisassociation(const int point_id,
                                       TEMDSystemUpdateList &emdList);
    static bool EMDChangeDisassociation(const edifact::KickInfo &kickInfo,
                                        const TEMDSystemUpdateList &emdList);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}    
};

Ticketing::CouponStatus calcPaxCouponStatus(const std::string& refuse,
                                            const bool pr_brd,
                                            const bool in_final_status);

enum TETCheckStatusArea {csaFlt,csaGrp,csaPax};
typedef std::list<Ticketing::Ticket> TTicketList;
typedef std::pair<TTicketList,XMLDoc> TTicketListCtxt;

class TTicketListKey
{
  public:
    std::string airline_oper;
    std::pair<std::string, std::string> addrs;
    int coupon_status;
    bool operator < (const TTicketListKey &key) const
    {
      if (airline_oper!=key.airline_oper)
        return airline_oper<key.airline_oper;
      if (addrs.first!=key.addrs.first)
        return addrs.first<key.addrs.first;
      if (addrs.second!=key.addrs.second)
        return addrs.second<key.addrs.second;
      return coupon_status<key.coupon_status;
    };
};

class TChangeStatusList : public std::map<TTicketListKey, std::vector<TTicketListCtxt> >
{
  public:
    xmlNodePtr addTicket(const TTicketListKey &key,
                         const Ticketing::Ticket &tick);
};

class ETStatusInterface : public JxtInterface
{
public:
  class TFltParams
  {
    public:
      TTripInfo fltInfo;
      bool etl_only, in_final_status;
      int pr_etstatus, et_final_attempt;
      void clear()
      {
        fltInfo.Clear();
        etl_only=false;
        in_final_status=false;
        pr_etstatus=ASTRA::NoExists;
        et_final_attempt=ASTRA::NoExists;
      };
      bool get(int point_id);
  };

  ETStatusInterface() : JxtInterface("ETStatus","ETStatus")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::SetTripETStatus);
     AddEvent("SetTripETStatus",evHandle);

     AddEvent("ChangePaxStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangePaxStatus));
     AddEvent("ChangeGrpStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeGrpStatus));
     AddEvent("ChangeFltStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeFltStatus));
     AddEvent("kick", JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::KickHandler));
  };  

  void SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static void ETCheckStatus(int point_id,
                            xmlDocPtr ediResDocPtr,
                            bool check_connect,
                            TChangeStatusList &mtick);
  static void ETCheckStatus(int id,
                            TETCheckStatusArea area,
                            int check_point_id,
                            bool check_connect,
                            TChangeStatusList &mtick,
                            bool before_checkin=false);
  static void ETRollbackStatus(xmlDocPtr ediResDocPtr,
                               bool check_connect);
  static bool ETChangeStatus(xmlNodePtr reqNode,
                             const TChangeStatusList &mtick);
};


inline xmlNodePtr astra_iface(xmlNodePtr resNode, const std::string &iface_id)
{

    xmlSetProp(resNode,"handle","1");

    xmlNodePtr ifaceNode=getNode(resNode,"interface");
    xmlSetProp(ifaceNode,"id",iface_id);
    xmlSetProp(ifaceNode,"ver","0");

    return ifaceNode;
}

#endif

