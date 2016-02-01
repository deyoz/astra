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
#include "qrys.h"
#include "tlg/EdifactRequest.h"

namespace edifact{
    class RemoteResults;
}//namespace edifact


class ETSearchParams
{
  public:
    int point_id;
    ETSearchParams(): point_id(ASTRA::NoExists) {};
    virtual ~ETSearchParams() {};
};

class ETSearchByTickNoParams : public ETSearchParams
{
  public:
    std::string tick_no;
};

class ETSearchInterface : public JxtInterface
{
public:
  enum SearchPurpose {spETDisplay, spEMDDisplay, spEMDRefresh};

  ETSearchInterface() : JxtInterface("ETSearchForm","ETSearchForm")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::SearchETByTickNo);
     AddEvent("SearchETByTickNo",evHandle);
     AddEvent("kick", JxtHandler<ETSearchInterface>::CreateHandler(&ETSearchInterface::KickHandler));
  }

  void SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

  static void SearchET(const ETSearchParams& searchParams,
                       const SearchPurpose searchPurpose,
                       edifact::KickInfo &kickInfo);

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

class EMDDisplayInterface : public JxtInterface
{
public:
    EMDDisplayInterface() : JxtInterface("", "EMDDisplay")
    {
        AddEvent("kick",             JXT_HANDLER(EMDDisplayInterface, KickHandler));
    }

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
  }

  void SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

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

class EMDAutoBoundId
{
  public:
    virtual const char* grpSQL() const=0;
    virtual const char* paxSQL() const=0;
    virtual void setSQLParams(QParams &params) const=0;
    virtual void fromXML(xmlNodePtr node)=0;
    virtual void toXML(xmlNodePtr node) const=0;
    virtual ~EMDAutoBoundId() {};
};

class EMDAutoBoundGrpId : public EMDAutoBoundId
{
  public:
    int grp_id;
    EMDAutoBoundGrpId(int _grp_id) : grp_id(_grp_id) {}
    EMDAutoBoundGrpId(xmlNodePtr node) { fromXML(node); }
    void clear()
    {
      grp_id=ASTRA::NoExists;
    }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id, pax_grp.piece_concept "
             "FROM pax_grp "
             "WHERE pax_grp.grp_id=:grp_id";
    }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax "
             "WHERE pax.grp_id=:grp_id";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << (grp_id!=ASTRA::NoExists?QParam("grp_id", otInteger, grp_id):
                                         QParam("grp_id", otInteger, FNull));
    }
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      if (!NodeIsNULL("emd_auto_bound_grp_id", node))
        grp_id=NodeAsInteger("emd_auto_bound_grp_id", node);
    }
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      grp_id!=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_grp_id", grp_id):
                              NewTextChild(node, "emd_auto_bound_grp_id");
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_grp_id", node)!=NULL;
    }
};

class EMDAutoBoundRegNo : public EMDAutoBoundId
{
  public:
    int point_id;
    int reg_no;
    EMDAutoBoundRegNo(int _point_id, int _reg_no) : point_id(_point_id), reg_no(_reg_no) {}
    EMDAutoBoundRegNo(xmlNodePtr node) { fromXML(node); }
    void clear()
    {
      point_id=ASTRA::NoExists;
      reg_no=ASTRA::NoExists;
    }
    virtual const char* grpSQL() const
    {
      return "SELECT pax_grp.point_dep, pax_grp.grp_id, pax_grp.piece_concept "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no";
    }
    virtual const char* paxSQL() const
    {
      return "SELECT pax.pax_id, pax.refuse, "
             "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
             "FROM pax_grp, pax "
             "WHERE pax_grp.grp_id=pax.grp_id AND "
             "      pax_grp.point_dep=:point_id AND pax.reg_no=:reg_no";
    }
    virtual void setSQLParams(QParams &params) const
    {
      params.clear();
      params << (point_id!=ASTRA::NoExists?QParam("point_id", otInteger, point_id):
                                           QParam("point_id", otInteger, FNull));
      params << (reg_no  !=ASTRA::NoExists?QParam("reg_no", otInteger, reg_no):
                                           QParam("reg_no", otInteger, FNull));
    }
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      if (!NodeIsNULL("emd_auto_bound_point_id", node))
        point_id=NodeAsInteger("emd_auto_bound_point_id", node);
      if (!NodeIsNULL("emd_auto_bound_reg_no", node))
        reg_no=NodeAsInteger("emd_auto_bound_reg_no", node);
    }
    virtual void toXML(xmlNodePtr node) const
    {
      if (node==NULL) return;
      point_id!=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_point_id", point_id):
                                NewTextChild(node, "emd_auto_bound_point_id");
      reg_no  !=ASTRA::NoExists?NewTextChild(node, "emd_auto_bound_reg_no", reg_no):
                                NewTextChild(node, "emd_auto_bound_reg_no");
    }
    static bool exists(xmlNodePtr node)
    {
      if (node==NULL) return false;
      return GetNode("emd_auto_bound_reg_no", node)!=NULL;
    }
};

class EMDAutoBoundInterface: public JxtInterface
{
  public:
    EMDAutoBoundInterface(): JxtInterface("EMDAutoBound", "EMDAutoBound")
    {
        AddEvent("kick",            JXT_HANDLER(EMDAutoBoundInterface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static bool Lock(const EMDAutoBoundId &id, int &point_id, int &grp_id, bool &piece_concept);
    static void EMDRefresh(const EMDAutoBoundId &id, xmlNodePtr reqNode);
    static void EMDTryBind(int grp_id, xmlNodePtr termReqNode, xmlNodePtr ediResNode);
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

