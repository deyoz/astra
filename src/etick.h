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
#include "baggage.h"
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
    bool operator < (const ETSearchByTickNoParams &params) const
    {
      if (point_id!=params.point_id)
        return point_id<params.point_id;
      return tick_no<params.tick_no;
    }
};

namespace PaxETList
{

enum TListType {notDisplayedByPointIdTlg,
                notDisplayedByPaxIdTlg};
std::string GetSQL(const TListType ltype);
void GetNotDisplayedET(int point_id_tlg, int id, bool is_pax_id, std::set<ETSearchByTickNoParams> &searchParams);

} //namespace PaxETList

void TlgETDisplay(int point_id_tlg, const std::set<int> &ids, bool is_pax_id);
void TlgETDisplay(int point_id_tlg, int id, bool is_pax_id);

class TETickItem
{
  public:

    enum TEdiAction{Display, ChangeOfStatus};

    std::string et_no;
    int et_coupon;
    BASIC::TDateTime issue_date;
    std::string surname, name;
    std::string fare_basis;
    int bag_norm;
    TBagNormUnit bag_norm_unit;
    Ticketing::CouponStatus status;
    std::string display_error, change_status_error;
    int point_id;
    std::string airp_dep, airp_arv;
    TETickItem()
    {
      clear();
    };

    void clear()
    {
      et_no.clear();
      et_coupon=ASTRA::NoExists;
      issue_date=ASTRA::NoExists;
      surname.clear();
      name.clear();
      fare_basis.clear();
      bag_norm=ASTRA::NoExists;
      bag_norm_unit.clear();
      status=Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable);
      display_error.clear();
      change_status_error.clear();
      point_id=ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();
    };

    bool empty() const
    {
      return et_no.empty() || et_coupon==ASTRA::NoExists;
    };

    std::string no_str() const
    {
      std::ostringstream s;
      s << et_no;
      if (et_coupon!=ASTRA::NoExists)
        s << "/" << et_coupon;
      return s.str();
    };

    const TETickItem& toDB(const TEdiAction ediAction) const;
    TETickItem& fromDB(const TEdiAction ediAction, TQuery &Qry);
    TETickItem& fromDB(const std::string &_et_no,
                       const int _et_coupon,
                       const TEdiAction ediAction,
                       const bool lock);
};

void ETDisplayToDB(const Ticketing::Pnr &pnr);

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
                       const edifact::KickInfo &kickInfo);

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

