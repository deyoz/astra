#ifndef _ETICK_H_
#define _ETICK_H_

#include "date_time.h"
#include "astra_utils.h"
#include "astra_ticket.h"
#include "astra_misc.h"
#include "astra_iface.h"
#include "xml_unit.h"
#include "edi_utils.h"
#include "emdoc.h"
#include "baggage.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "astra_pnr.h"
#include "tlg/EdifactRequest.h"

#include <jxtlib/xmllibcpp.h>

using BASIC::date_time::TDateTime;

namespace edifact{
    class RemoteResults;
    class KickInfo;
}//namespace edifact

class ETSearchParams
{
  public:
    int point_id;
    ETSearchParams(const int& _point_id) : point_id(_point_id) {}
    virtual ~ETSearchParams() {}
    bool operator != (const ETSearchParams &params) const
    {
      return point_id!=params.point_id;
    }
    bool operator < (const ETSearchParams &params) const
    {
      return point_id<params.point_id;
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << "point_id=" << point_id;
      return s.str();
    }

};

class ETWideSearchParams : public ETSearchParams
{
  public:
    std::string airline, airp_dep;
    int flt_no;
    ETWideSearchParams(const int& _point_id) : ETSearchParams(_point_id), flt_no(ASTRA::NoExists) {}
    bool existsAdditionalFltInfo() const
    {
      return !airline.empty() &&
             flt_no!=ASTRA::NoExists &&
             !airp_dep.empty();
    }
    void set(TTripInfo &info) const
    {
      info.Clear();
      info.airline=airline;
      info.flt_no=flt_no;
      info.airp=airp_dep;
    }
    bool operator != (const ETWideSearchParams &params) const
    {
      return ETSearchParams::operator !=(params) ||
             existsAdditionalFltInfo()!=params.existsAdditionalFltInfo() ||
             airline!=params.airline ||
             flt_no!=params.flt_no ||
             airp_dep!=params.airp_dep;
    }
    bool operator < (const ETWideSearchParams &params) const
    {
      if (ETSearchParams::operator !=(params))
        return ETSearchParams::operator <(params);
      if (existsAdditionalFltInfo()!=params.existsAdditionalFltInfo())
        return existsAdditionalFltInfo()<params.existsAdditionalFltInfo();
      if (airline!=params.airline)
        return airline<params.airline;
      if (flt_no!=params.flt_no)
        return flt_no<params.flt_no;
      return airp_dep<params.airp_dep;
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << ETSearchParams::traceStr();
      if (existsAdditionalFltInfo())
        s << ", airline=" << airline
          << ", flt_no=" << flt_no;
      return s.str();
    }
};

class ETSearchByTickNoParams : public ETWideSearchParams
{
  public:
    std::string tick_no;
    ETSearchByTickNoParams(const int& _point_id,
                           const std::string& _tick_no) :
      ETWideSearchParams(_point_id), tick_no(_tick_no) {}
    bool operator < (const ETSearchByTickNoParams &params) const
    {
      if (ETWideSearchParams::operator !=(params))
        return ETWideSearchParams::operator <(params);
      return tick_no<params.tick_no;
    }
    std::string traceStr() const
    {
      std::ostringstream s;
      s << ETWideSearchParams::traceStr()
        << ", tick_no=" << tick_no;
      return s.str();
    }
};

class ETRequestControlParams : public ETWideSearchParams
{
  public:
    std::string tick_no;
    int coupon_no;
    ETRequestControlParams(const int& _point_id,
                           const std::string& _tick_no,
                           const int& _coupon_no) :
      ETWideSearchParams(_point_id), tick_no(_tick_no), coupon_no(_coupon_no) {}
    std::string traceStr() const
    {
      std::ostringstream s;
      s << ETWideSearchParams::traceStr()
        << ", tick_no=" << tick_no
        << ", coupon_no=" << (coupon_no==ASTRA::NoExists?"NoExists":IntToString(coupon_no));
      return s.str();
    }
};

namespace PaxETList
{

enum TListType {notDisplayedByPointIdTlg,
                notDisplayedByPaxIdTlg,
                allStatusesByPointIdFromTlg,
                allByPointIdAndTickNoFromTlg,
                allByTickNoAndCouponNoFromTlg,
                allNotCheckedStatusesByPointId,
                allCheckedByTickNoAndCouponNo};
std::string GetSQL(const TListType ltype);
void GetNotDisplayedET(int point_id_tlg, int id, bool is_pax_id, std::set<ETSearchByTickNoParams> &searchParams);

} //namespace PaxETList

void TlgETDisplay(int point_id_tlg, const std::set<int> &ids, bool is_pax_id);
void TlgETDisplay(int point_id_tlg, int id, bool is_pax_id);
void TlgETDisplay(int point_id_spp);

class TETCoupon : public AstraEdifact::TCoupon
{
  public:
    TETCoupon& fromDB(TQuery &Qry);
};

class TETCtxtItem : public AstraEdifact::TCtxtItem
{
  public:
    TETCoupon et;
    TETCtxtItem()
    {
      clear();
    }

    TETCtxtItem(const TReqInfo &reqInfo, int _point_id)
    {
      clear();
      TOriginCtxt::operator=(TOriginCtxt(reqInfo));
      point_id=_point_id;
    }

    void clear()
    {
      TCtxtItem::clear();
      et.clear();
    }

    bool operator < (const TETCtxtItem &ctxt) const
    {
      return et < ctxt.et;
    }

    TETCtxtItem& fromDB(TQuery &Qry, bool from_crs);
};

class TETickItem
{
  public:

    enum TEdiAction{DisplayTlg, Display, ChangeOfStatus};

    TETCoupon et;
    TDateTime issue_date;
    std::string surname, name;
    std::string fare_basis;
    std::string fare_class;
    boost::optional<TBagQuantity> bagNorm;
    std::string display_error, change_status_error;
    int point_id;
    std::string airp_dep, airp_arv;
    boost::optional<Ticketing::EdiPnr> ediPnr;
    TETickItem()
    {
      clear();
    }
    TETickItem(const TETickItem &item, const Ticketing::CouponStatus &_status) :
      TETickItem(item) { et.status=_status; }
    TETickItem(const std::string &_et_no,
               const int &_et_coupon,
               const int &_point_id,
               const std::string &_airp_dep,
               const std::string &_airp_arv,
               const Ticketing::CouponStatus &_status)
    {
      clear();
      et.no=_et_no;
      et.coupon=_et_coupon;
      point_id=_point_id;
      airp_dep=_airp_dep;
      airp_arv=_airp_arv;
      et.status=_status;
    }

    void clear()
    {
      et.clear();
      issue_date=ASTRA::NoExists;
      surname.clear();
      name.clear();
      fare_basis.clear();
      fare_class.clear();
      bagNorm=boost::none;
      display_error.clear();
      change_status_error.clear();
      point_id=ASTRA::NoExists;
      airp_dep.clear();
      airp_arv.clear();
      ediPnr=boost::none;
    }

    bool empty() const
    {
      return et.empty();
    }

    bool operator < (const TETickItem &item) const
    {
      return et < item.et;
    }

    std::string bagNormView(const AstraLocale::OutputLang &lang) const
    {
      if (empty()) return "";

      if (bagNorm)
        return bagNorm.get().view(lang);
      else
        return lowerc(AstraLocale::getLocaleText("…’", lang.get()));
    }

    const TETickItem& toDB(const TEdiAction ediAction) const;
    TETickItem& fromDB(const TEdiAction ediAction, TQuery &Qry);
    TETickItem& fromDB(const std::string &_et_no,
                       const int _et_coupon,
                       const TEdiAction ediAction,
                       const bool lock);
    static void fromDB(const std::string &_et_no,
                       const TEdiAction ediAction,
                       std::list<TETickItem> &eticks);

    Ticketing::Ticket makeTicket(const AstraEdifact::TFltParams& fltParams,
                                 const std::string &subclass,
                                 const Ticketing::CouponStatus& real_status) const;
    static void syncOriginalSubclass(const TETCoupon& et);
    static bool syncOriginalSubclass(int pax_id);
};

std::string airpDepToPrefferedCode(const AirportCode_t& airp,
                                   const boost::optional<CheckIn::TPaxTknItem>& tkn,
                                   const AstraLocale::OutputLang &lang);

std::string airpArvToPrefferedCode(const AirportCode_t& airp,
                                   const boost::optional<CheckIn::TPaxTknItem>& tkn,
                                   const AstraLocale::OutputLang &lang);

void ETDisplayToDB(const Ticketing::EdiPnr &ediPnr);

class ETSearchInterface : public AstraJxtIface
{
public:
  enum SearchPurpose {spETDisplay, spTlgETDisplay, spEMDDisplay, spEMDRefresh, spETRequestControl};

  ETSearchInterface() : AstraJxtIface("ETSearchForm")
  {
     AddEvent("SearchETByTickNo", JXT_HANDLER(ETSearchInterface, SearchETByTickNo));
     AddEvent("kick",             JXT_HANDLER(ETSearchInterface, KickHandler));
  }

  void SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

  static void SearchET(const ETSearchParams& searchParams,
                       const SearchPurpose searchPurpose,
                       const edifact::KickInfo &kickInfo);

};

class ETRequestControlInterface: public AstraJxtIface
{
public:
    ETRequestControlInterface(): AstraJxtIface("ETRequestControl")
    {
        AddEvent("kick",           JXT_HANDLER(ETRequestControlInterface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class ETRequestACInterface: public AstraJxtIface
{
public:
    ETRequestACInterface(): AstraJxtIface("RequestAC")
    {
        AddEvent("RequestControl", JXT_HANDLER(ETRequestACInterface, RequestControl));
        AddEvent("kick",           JXT_HANDLER(ETRequestACInterface, KickHandler));
    }

    void RequestControl(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class EMDSearchInterface : public AstraJxtIface
{
public:
    EMDSearchInterface() : AstraJxtIface("EMDSearch")
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

class EMDDisplayInterface : public AstraJxtIface
{
public:
    EMDDisplayInterface() : AstraJxtIface("EMDDisplay")
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

class EMDSystemUpdateInterface : public AstraJxtIface
{
public:
    EMDSystemUpdateInterface() : AstraJxtIface("EMDSystemUpdate")
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
    int flt_no_oper;
    std::pair<std::string, std::string> addrs;
    int coupon_status;
    TETChangeStatusKey() : flt_no_oper(ASTRA::NoExists) {}
    bool operator < (const TETChangeStatusKey &key) const
    {
      if (airline_oper!=key.airline_oper)
        return airline_oper<key.airline_oper;
      if (flt_no_oper!=key.flt_no_oper)
        return flt_no_oper<key.flt_no_oper;
      if (addrs.first!=key.addrs.first)
        return addrs.first<key.addrs.first;
      if (addrs.second!=key.addrs.second)
        return addrs.second<key.addrs.second;
      return coupon_status<key.coupon_status;
    }
};

class TETChangeStatusList : public std::map<TETChangeStatusKey, std::vector<TETChangeStatusItem> >
{
  private:
    xmlNodePtr addTicket(const TETChangeStatusKey &key,
                         const Ticketing::Ticket &tick,
                         bool onlySingleTicketInTlg);
  public:
    xmlNodePtr addTicket(const TETChangeStatusKey &key,
                         const TETickItem& ETItem,
                         const AstraEdifact::TFltParams& fltParams,
                         const std::string& subclass="");

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
                const TEMDCtxtItem &item,
                bool control_method);
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

class ETStatusInterface : public AstraJxtIface
{
public:
  ETStatusInterface() : AstraJxtIface("ETStatus")
  {

     AddEvent("SetTripETStatus", JXT_HANDLER(ETStatusInterface, SetTripETStatus));
     AddEvent("ChangePaxStatus", JXT_HANDLER(ETStatusInterface, ChangePaxStatus));
     AddEvent("ChangeGrpStatus", JXT_HANDLER(ETStatusInterface, ChangeGrpStatus));
     AddEvent("ChangeFltStatus", JXT_HANDLER(ETStatusInterface, ChangeFltStatus));
  }

  void SetTripETStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangePaxStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeGrpStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeFltStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

  static bool ChangeStatusLocallyOnly(const AstraEdifact::TFltParams& fltParams,
                                      const TETickItem& item,
                                      const TETCtxtItem &ctxt);

  static bool ToDoNothingWhenChangingStatus(const AstraEdifact::TFltParams& fltParams,
                                            const TETickItem& item);
  static void AfterReceiveAirportControl(const Ticketing::WcCoupon& cpn);
  static void AfterReturnAirportControl(const Ticketing::WcCoupon& cpn);
  static bool ReturnAirportControl(const AstraEdifact::TFltParams& fltParams,
                                   const TETickItem& item);

  static void ETCheckStatusForRollback(int point_id,
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

class EMDStatusInterface: public AstraJxtIface
{
public:
    EMDStatusInterface(): AstraJxtIface("EMDStatus")
    {
        AddEvent("ChangeStatus",    JXT_HANDLER(EMDStatusInterface, ChangeStatus));
        AddEvent("kick",            JXT_HANDLER(EMDStatusInterface, KickHandler));
    }

    void ChangeStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static void EMDCheckStatus(const int grp_id,
                               const CheckIn::TServicePaymentListWithAuto &prior_payment,
                               TEMDChangeStatusList &emdList);
    static bool EMDChangeStatus(const edifact::KickInfo &kickInfo,
                                const TEMDChangeStatusList &emdList);
};

class ChangeStatusInterface: public AstraJxtIface
{
  public:
    ChangeStatusInterface(): AstraJxtIface("ChangeStatus")
    {
        AddEvent("kick",            JXT_HANDLER(ChangeStatusInterface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static void ChangeStatus(const xmlNodePtr reqNode,
                             const TChangeStatusList &info);

  protected:
    void KickOnAnswer(xmlNodePtr reqNode, xmlNodePtr resNode);
    void KickOnTimeout(xmlNodePtr reqNode, xmlNodePtr resNode);
};

void transformKickRequest(xmlNodePtr termReqNode, xmlNodePtr &kickReqNode);
void ContinueCheckin(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);

class EMDAutoBoundInterface: public AstraJxtIface
{
  private:
    static bool BeforeLock(const EMDAutoBoundId &id, int &point_id, GrpIds &grpIds);
  public:
    EMDAutoBoundInterface(): AstraJxtIface("EMDAutoBound")
    {
        AddEvent("kick", JXT_HANDLER(EMDAutoBoundInterface, KickHandler));
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

    static bool Lock(const EMDAutoBoundId &id, int &point_id, GrpIds &grpIds, const std::string &whence);
    static bool Lock(const EMDAutoBoundId &id, int &point_id, TCkinGrpIds &tckin_grp_ids, const std::string &whence);
    static void EMDRefresh(const EMDAutoBoundId &id, xmlNodePtr reqNode);
    static void EMDTryBind(const TCkinGrpIds &tckin_grp_ids,
                           xmlNodePtr termReqNode,
                           xmlNodePtr ediResNode,
                           const boost::optional<edifact::TripTaskForPostpone> &task=boost::none);
    static void EMDTryBind(const TCkinGrpIds &tckin_grp_ids,
                           const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd,
                           TEMDChangeStatusList &emdList);
    static void EMDSearch(const EMDAutoBoundId &id,
                          xmlNodePtr reqNode,
                          int point_id,
                          const boost::optional< std::set<int> > &pax_ids);
};

void emd_refresh_task(const TTripTaskKey &task);
void emd_refresh_by_grp_task(const TTripTaskKey &task);
void emd_try_bind_task(const TTripTaskKey &task);

inline xmlNodePtr astra_iface(xmlNodePtr resNode, const std::string &iface_id)
{

    xmlSetProp(resNode,"handle","1");

    xmlNodePtr ifaceNode=getNode(resNode,"interface");
    xmlSetProp(ifaceNode,"id",iface_id);
    xmlSetProp(ifaceNode,"ver","0");

    return ifaceNode;
}

void handleEtDispResponse(const edifact::RemoteResults& remRes);
void handleEtCosResponse(const edifact::RemoteResults& remRes);
void handleEtRacResponse(const edifact::RemoteResults& remRes);

void handleEtUac(const std::string& uac);

#endif

