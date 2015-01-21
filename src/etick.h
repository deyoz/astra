#ifndef _ETICK_H_
#define _ETICK_H_

#include "jxtlib/JxtInterface.h"
#include "jxtlib/xmllibcpp.h"
#include "astra_utils.h"
#include "astra_ticket.h"
#include "astra_misc.h"
#include "xml_unit.h"
#include "edi_utils.h"
#include "emdoc.h"
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

enum TETCheckStatusArea {csaFlt,csaGrp,csaPax};
typedef std::list<Ticketing::Ticket> TTicketList;
typedef std::pair<TTicketList,XMLDoc> TETChangeStatusItem;

class TETChangeStatusKey
{
  public:
    std::string airline_oper;
    std::pair<std::string, std::string> addrs;
    int coupon_status;
    bool operator < (const TETChangeStatusKey &key) const
    {
      if (airline_oper!=key.airline_oper)
        return airline_oper<key.airline_oper;
      if (addrs.first!=key.addrs.first)
        return addrs.first<key.addrs.first;
      if (addrs.second!=key.addrs.second)
        return addrs.second<key.addrs.second;
      return coupon_status<key.coupon_status;
    }
};

class TETChangeStatusList : public std::map<TETChangeStatusKey, std::vector<TETChangeStatusItem> >
{
  public:
    xmlNodePtr addTicket(const TETChangeStatusKey &key,
                         const Ticketing::Ticket &tick);
};

class TEMDChangeStatusKey
{
  public:
    std::string airline_oper;
    int flt_no_oper;
    Ticketing::CouponStatus coupon_status;
    bool operator < (const TEMDChangeStatusKey &key) const
    {
      if (airline_oper!=key.airline_oper)
        return airline_oper<key.airline_oper;
      if (flt_no_oper!=key.flt_no_oper)
        return flt_no_oper<key.flt_no_oper;
      return coupon_status->codeInt()<key.coupon_status->codeInt();
    }
    std::string traceStr() const;
};

class TEMDChangeStatusItem
{
  public:
    Ticketing::TicketCpn_t emd;
    XMLDoc ctxt;
    std::string traceStr() const;
};

class TEMDChangeStatusList : public std::map<TEMDChangeStatusKey, std::list<TEMDChangeStatusItem> >
{
  public:
    void addEMD(const TEMDChangeStatusKey &key,
                const TEMDCtxtItem &item);
};

class TChangeStatusList
{
  public:
    TETChangeStatusList ET;
    TEMDChangeStatusList EMD;

    bool empty() const
    {
      return ET.empty() && EMD.empty();
    }
};

class ETStatusInterface : public JxtInterface
{
public:
  ETStatusInterface() : JxtInterface("ETStatus","ETStatus")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::SetTripETStatus);
     AddEvent("SetTripETStatus",evHandle);

     AddEvent("ChangePaxStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangePaxStatus));
     AddEvent("ChangeGrpStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeGrpStatus));
     AddEvent("ChangeFltStatus",JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::ChangeFltStatus));
     //AddEvent("kick", JxtHandler<ETStatusInterface>::CreateHandler(&ETStatusInterface::KickHandler));
  }

  void SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  //void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

  static void ETCheckStatus(int point_id,
                            xmlDocPtr ediResDocPtr,
                            bool check_connect,
                            TETChangeStatusList &mtick);
  static void ETCheckStatus(int id,
                            TETCheckStatusArea area,
                            int check_point_id,
                            bool check_connect,
                            TETChangeStatusList &mtick,
                            bool before_checkin=false);
  static void ETRollbackStatus(xmlDocPtr ediResDocPtr,
                               bool check_connect);
  static bool ETChangeStatus(const edifact::KickInfo &kickInfo,
                             const TETChangeStatusList &mtick);
  static bool ETChangeStatus(const xmlNodePtr reqNode,
                             const TETChangeStatusList &mtick);
};

class EMDStatusInterface: public JxtInterface
{
public:
    EMDStatusInterface(): JxtInterface("EMDStatus", "EMDStatus")
    {
        AddEvent("ChangeStatus",    JXT_HANDLER(EMDStatusInterface, ChangeStatus));
        AddEvent("kick",            JXT_HANDLER(EMDStatusInterface, KickHandler));
    }

    void ChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static void EMDCheckStatus(const int grp_id,
                               const CheckIn::PaidBagEMDList &prior_emds,
                               TEMDChangeStatusList &emdList);
    static bool EMDChangeStatus(const edifact::KickInfo &kickInfo,
                                const TEMDChangeStatusList &emdList);
};

class ChangeStatusInterface: public JxtInterface
{
  public:
    ChangeStatusInterface(): JxtInterface("ChangeStatus", "ChangeStatus")
    {
        AddEvent("kick",            JXT_HANDLER(ChangeStatusInterface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static void ChangeStatus(const xmlNodePtr reqNode,
                             const TChangeStatusList &info);
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

