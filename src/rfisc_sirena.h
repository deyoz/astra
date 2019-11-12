#ifndef _RFISC_SIRENA_H_
#define _RFISC_SIRENA_H_

#include "sirena_exchange.h"
#include "astra_misc.h"
#include "etick.h"
#include "passenger.h"

namespace SirenaExchange
{

using namespace BASIC::date_time;

template<typename GRP, typename PAX>
class TVariousReqPassengers : public std::map< GRP, std::set<PAX> >
{
  public:
    bool include_refused;
    bool include_unbound_svcs;
    bool only_first_segment;
    TVariousReqPassengers(const GRP &_grp_id, bool _include_refused, bool _include_unbound_svcs) :
      include_refused(_include_refused),
      include_unbound_svcs(_include_unbound_svcs),
      only_first_segment(false)
    {
      if (_grp_id!=ASTRA::NoExists)
        this->insert(make_pair(_grp_id, std::set<PAX>()));
    }
    TVariousReqPassengers(bool _include_refused, bool _include_unbound_svcs, bool _only_first_segment) :
      include_refused(_include_refused),
      include_unbound_svcs(_include_unbound_svcs),
      only_first_segment(_only_first_segment)
    {}
    void add(const GRP &_grp_id, const PAX &_pax_id)
    {
      if (_grp_id==ASTRA::NoExists || _pax_id==ASTRA::NoExists) return;
      typename TVariousReqPassengers::iterator i=this->insert(make_pair(_grp_id, std::set<PAX>())).first;
      if (i!=this->end()) i->second.insert(_pax_id);
    }
    bool pax_included(const GRP &_grp_id, const PAX &_pax_id) const
    {
      typename TVariousReqPassengers::const_iterator i=this->find(_grp_id);
      if (i==this->end()) return false;
      if (i->second.empty()) return true;
      return (i->second.find(_pax_id)!=i->second.end());
    }
};

typedef TVariousReqPassengers<int, int> TCheckedReqPassengers;
typedef TVariousReqPassengers<int, int> TNotCheckedReqPassengers;

class TCheckedResPassengersItem
{
  public:
    CheckIn::TPaxGrpCategory::Enum grp_cat;
    TCkinGrpIds tckin_grp_ids;
    TCheckedResPassengersItem() : grp_cat(CheckIn::TPaxGrpCategory::Unknown) {}
};

class TCheckedResPassengers : public std::map<int, TCheckedResPassengersItem> {};

class TSegItem
{
  public:
    int id;
    TTripInfo operFlt;
    bool scd_out_contain_time;
    boost::optional<TTripInfo> markFlt;
    std::string airp_arv;
    TDateTime scd_in;
    bool scd_in_contain_time;
    TSegItem()
    {
      clear();
    }
    void set(int seg_id,
             const TTrferRouteItem &item)
    {
      clear();
      id=seg_id;
      operFlt=item.operFlt;
      airp_arv=item.airp_arv;
    }
    void set(int seg_id,
             const TTripInfo &_operFlt,
             const std::string &_airp_arv,
             const TSimpleMktFlight &mktFlight,
             const TDateTime &_scd_in)
    {
      clear();
      id=seg_id;
      operFlt=_operFlt;
      if (operFlt.scd_out!=ASTRA::NoExists)
      {
        operFlt.scd_out=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));
        scd_out_contain_time=true;
      };
      airp_arv=_airp_arv;
      scd_in=_scd_in;
      if (scd_in!=ASTRA::NoExists)
      {
        scd_in=UTCToLocal(scd_in, AirpTZRegion(airp_arv));
        scd_in_contain_time=true;
      };
      markFlt=boost::none;
      if (!mktFlight.empty())
      {
        markFlt=TTripInfo();
        try
        {
          markFlt.get().Init(dynamic_cast<const TGrpMktFlight&>(mktFlight));
        }
        catch (std::bad_cast &)
        {
          markFlt.get().Init(dynamic_cast<const TMktFlight&>(mktFlight));
        };
      };
    }

    void clear()
    {
      id=ASTRA::NoExists;
      operFlt.Clear();
      scd_out_contain_time=false;
      markFlt=boost::none;
      airp_arv.clear();
      scd_in=ASTRA::NoExists;
      scd_in_contain_time=false;
    }

    const TSegItem& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    static std::string flight(const TTripInfo &flt, const AstraLocale::OutputLang &lang);
};

class TPaxSection;

class TPaxSegItem : public TSegItem
{
  public:
    std::string subcl;
    CheckIn::TPaxTknItem tkn;
    int display_id;
    TPnrAddrs pnrAddrs;
    std::set<CheckIn::TPaxFQTItem> fqts;
    TPaxSegItem()
    {
      TSegItem::clear();
      clear();
    }
    void clear()
    {
      subcl.clear();
      tkn.clear();
      display_id=ASTRA::NoExists;
      pnrAddrs.clear();
      fqts.clear();
    }
    using TSegItem::set;
    void set(const CheckIn::TPaxTknItem& _tkn, TPaxSection* paxSection);

    const TPaxSegItem& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
};

typedef std::map<int, TPaxSegItem> TPaxSegMap;

class category {
  public:
    static std::string value( ASTRA::TPerson pers_type, int seats ) {
      switch(pers_type)
      {
        case ASTRA::adult: return "ADT";
        case ASTRA::child: return "CNN";
         case ASTRA::baby: return seats>0?"INS":"INF";
                  default: return "";
      }
    }
};

class TPaxItem
{
  public:
    int id;
    std::string surname, name;
    ASTRA::TPerson pers_type;
    int seats;
    CheckIn::TPaxDocItem doc;

    TPaxSegMap segs;

    TPaxItem()
    {
      clear();
    }
    void set(const CheckIn::TPaxItem &item, const TETickItem &etick)
    {
      clear();
      id=item.id;
      if (!etick.surname.empty())
      {
        surname=etick.surname;
        name=etick.name;
      }
      else
      {
        surname=item.surname;
        name=item.name;
      };
      pers_type=item.pers_type;
      seats=item.seats;
      doc=item.doc;
    }

    void clear()
    {
      id=ASTRA::NoExists;
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
      doc.clear();
      segs.clear();
    }

    const TPaxItem& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    std::string category() const {
      return category::value( pers_type, seats );
    }
    std::string sex() const;
};

class TPaxItem2
{
  public:
    std::string surname;
    std::string name;
    ASTRA::TPerson pers_type;
    int seats;
    int reg_no;
    int grp_id;
    TPnrAddrs pnrAddrs;
    TPaxItem2()
    {
      clear();
    }
    void set(int _grp_id, const CheckIn::TSimplePaxItem &item, const TETickItem &etick)
    {
      clear();
      if (!etick.surname.empty())
      {
        surname=etick.surname;
        name=etick.name;
      }
      else
      {
        surname=item.surname;
        name=item.name;
      };
      pers_type=item.pers_type;
      seats=item.seats;
      reg_no=item.reg_no;
      grp_id=_grp_id;
    }

    void clear()
    {
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
      reg_no=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      pnrAddrs.clear();
    }

    const TPaxItem2& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    std::string category() const {
      return category::value( pers_type, seats );
    }
};

class TSvcItem : public TPaxSegRFISCKey
{
  public:
    TServiceStatus::Enum status;
    std::string ssr_code, ssr_text;
    TSvcItem() { clear(); }
    TSvcItem(const TPaxSegRFISCKey& _item, const TServiceStatus::Enum _status) :
      TPaxSegRFISCKey(_item), status(_status) {}
    void clear()
    {
      TPaxSegRFISCKey::clear();
      status=TServiceStatus::Unknown;
      ssr_code.clear();
      ssr_text.clear();
    }
    const TSvcItem& toSirenaXML(xmlNodePtr node, const AstraLocale::OutputLang &lang) const;
    TSvcItem& fromSirenaXML(xmlNodePtr node);
};

class TDisplayItem : public Ticketing::EdiPnr
{
  public:
    int id;
    TDisplayItem(const Ticketing::EdiPnr& ediPnr, int _id) : Ticketing::EdiPnr(ediPnr), id(_id) {}
    bool operator < (const TDisplayItem &item) const
    {
      return ediText() < item.ediText();
    }
    const TDisplayItem& toSirenaXML(xmlNodePtr displayParentNode) const;
};

class TDisplayList : public std::set<TDisplayItem>
{
  public:
    int add(const boost::optional<Ticketing::EdiPnr>& ediPnr)
    {
      if (!ediPnr) return ASTRA::NoExists;
      return emplace(ediPnr.get(), size()+1).first->id;
    }
};


class TPaxSection
{
  private:
    bool throw_if_empty;
  public:
    TPaxSection(bool _throw_if_empty) : throw_if_empty(_throw_if_empty) {}
    std::list<TPaxItem> paxs;
    TDisplayList displays;
    void clear()
    {
      paxs.clear();
    }
    void toXML(xmlNodePtr node) const;
    void updateSeg(const Sirena::TPaxSegKey &key);
};

class TSvcList : public std::list<TSvcItem>
{
  private:
    std::list<TSvcItem> _autoChecked;
    TGrpServiceList _additionalBagList;
  public:
    void clear()
    {
      std::list<TSvcItem>::clear();
      _autoChecked.clear();
      _additionalBagList.clear();
    }

    void addBaggageOrCarryOn(int pax_id, const TRFISCKey& key);
    void addChecked(const TCheckedReqPassengers &req_grps, int grp_id, int tckin_seg_count, int trfer_seg_count);
    void addASVCs(int pax_id, const std::vector<CheckIn::TPaxASVCItem> &asvc);
    void addUnbound(const TCheckedReqPassengers &req_grps, int grp_id, int pax_id);
    void addFromCrs(const TNotCheckedReqPassengers &req_pnrs, int pnr_id, int pax_id);
    void get(const std::list<TSvcItem> &svcsAuto, TPaidRFISCList &paid) const;
    const std::list<TSvcItem>& autoChecked() { return _autoChecked; }
};

class TSvcSection
{
  private:
    bool throw_if_empty;
  public:
    TSvcSection(bool _throw_if_empty) : throw_if_empty(_throw_if_empty) {}
    TSvcList svcs;
    void clear()
    {
      svcs.clear();
    }
    void toXML(xmlNodePtr node) const;
    void updateSeg(const Sirena::TPaxSegKey &key);
};

//запросы Астры в Сирену
class TAvailability : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TPaymentStatus : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TGroupInfo : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TPseudoGroupInfo : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TPassengers : public TExchange
{
  public:
    static const std::string id;
    virtual std::string exchangeId() const { return id; }
};

class TAvailabilityReq : public TAvailability, public TPaxSection
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    TAvailabilityReq() : TPaxSection(true) {}
    virtual void clear()
    {
      TPaxSection::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
    void bagTypesToDB(const TCkinGrpIds &tckin_grp_ids, bool copy_all_segs=true) const;
};

class TAvailabilityResItem
{
  public:
    TRFISCList rfisc_list;
    Sirena::TSimplePaxNormItem baggage_norm;
    Sirena::TSimplePaxNormItem carry_on_norm;
    Sirena::TSimplePaxBrandItem brand;
    TAvailabilityResItem()
    {
      clear();
    }
    void clear()
    {
      rfisc_list.clear();
      baggage_norm.clear();
      carry_on_norm.clear();
      brand.clear();
    }
    void remove_unnecessary();
};

typedef std::map<Sirena::TPaxSegKey, TAvailabilityResItem> TAvailabilityResMap;

class TAvailabilityRes : public TAvailability, public TAvailabilityResMap
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    virtual void clear()
    {
      TAvailabilityResMap::clear();
    }
    virtual void fromXML(xmlNodePtr node);
    bool identical_concept(int seg_id, bool carry_on, boost::optional<TBagConcept::Enum> &concept) const;
    template<typename T>
    bool exists_rfisc(int seg_id, const T& criterion) const
    {
      for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
      {
        if (i->first.trfer_num!=seg_id) continue;
        if (i->second.rfisc_list.exists(criterion)) return true;
      };
      return false;
    }
    void rfiscsToDB(const TCkinGrpIds &tckin_grp_ids, TBagConcept::Enum bag_concept, bool old_version) const;
    void normsToDB(const TCkinGrpIds &tckin_grp_ids) const;
    void brandsToDB(const TCkinGrpIds &tckin_grp_ids) const;
    void setAdditionalListId(const TCkinGrpIds &tckin_grp_ids) const;
};

class TPaymentStatusReq : public TPaymentStatus, public TPaxSection, public TSvcSection
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    TPaymentStatusReq() : TPaxSection(true), TSvcSection(true) {}
    virtual void clear()
    {
      TPaxSection::clear();
      TSvcSection::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

class TPaymentStatusRes : public TPaymentStatus
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    TSvcList svcs;
    Sirena::TPaxNormList norms;
    virtual void clear()
    {
      svcs.clear();
      norms.clear();
    }
    virtual void fromXML(xmlNodePtr node);
    void normsToDB(const TCkinGrpIds &tckin_grp_ids) const;
    void check_unknown_status(int seg_id, std::set<TRFISCListKey> &rfiscs) const;
};

//запросы Сирены в Астру

class TPassengersReq : public TPassengers, public TTripInfo
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    virtual void clear()
    {
      TTripInfo::Clear();
    }
    virtual void fromXML(xmlNodePtr node);
};

typedef std::list<TPaxItem2> TPassengerList;

class TPassengersRes : public TPassengers, public TPassengerList
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    virtual void clear()
    {
      TPassengerList::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

class TGroupInfoReq : public TGroupInfo
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::string pnr_addr;
    int grp_id;
    TGroupInfoReq()
    {
      clear();
    }
    void clear()
    {
      pnr_addr.clear();
      grp_id = ASTRA::NoExists;
    }
    virtual void fromXML(xmlNodePtr node);
    void toDB(TQuery &Qry);
};

class TGroupInfoRes : public TGroupInfo, public TPaxSection, public TSvcSection
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    TGroupInfoRes() : TPaxSection(true), TSvcSection(true) {}
    virtual void clear()
    {
      TPaxSection::clear();
      TSvcSection::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

typedef std::set<Sirena::TPaxSegKey> TEntityList;

class TPseudoGroupInfoReq : public TPseudoGroupInfo
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::string pnr_addr;
    TEntityList entities;
    TPseudoGroupInfoReq()
    {
      clear();
    }
    void clear()
    {
      pnr_addr.clear();
      entities.clear();
    }
    virtual void fromXML(xmlNodePtr node);
};

class TPseudoGroupInfoRes : public TPseudoGroupInfo, public TPaxSection, public TSvcSection
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    TPseudoGroupInfoRes() : TPaxSection(false), TSvcSection(false) {}
    virtual void clear()
    {
      TPaxSection::clear();
      TSvcSection::clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

void SendRequest(const TExchange &request, TExchange &response,
                 RequestInfo &requestInfo, ResponseInfo &responseInfo);
void SendRequest(const TExchange &request, TExchange &response);

void fillPaxsBags(int first_grp_id, TExchange &exch, CheckIn::TPaxGrpCategory::Enum &grp_cat, TCkinGrpIds &tckin_grp_ids,
                  bool include_refused=false);
void fillPaxsBags(const TCheckedReqPassengers &req_grps, TExchange &exch, TCheckedResPassengers &res_grps);
void fillPaxsSvcs(const TNotCheckedReqPassengers &req_grps, TExchange &exch);
void fillPaxsSvcs(const TEntityList &entities, TExchange &exch);
void fillProtBeforePaySvcs(const TAdvTripInfo &operFlt,
                           const int pax_id,
                           TExchange &exch);

bool needSync(xmlNodePtr answerResNode);
xmlNodePtr findAnswerNode(xmlNodePtr answerResNode);

} //namespace SirenaExchange

void unaccBagTypesToDB(int grp_id, bool ignore_unaccomp_sets=false);
void CopyPaxServiceLists(int grp_id_src, int grp_id_dest, bool is_grp_id, bool rfisc_used);

class ServicePaymentInterface : public JxtInterface
{
public:
  ServicePaymentInterface() : JxtInterface("","ServicePayment")
  {
     Handler *evHandle;
     evHandle=JxtHandler<ServicePaymentInterface>::CreateHandler(&ServicePaymentInterface::LoadServiceLists);
     AddEvent("LoadServiceLists",evHandle);
  };
  void LoadServiceLists(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

typedef std::function<void(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)> SvcSirenaResponseHandler;

class SvcSirenaInterface : public JxtInterface
{
  private:
    std::list<SvcSirenaResponseHandler> resHandlers;
    bool addResponseHandler(const SvcSirenaResponseHandler&);
    void handleResponse(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode) const;
    static bool equal(const SvcSirenaResponseHandler& handler1,
                      const SvcSirenaResponseHandler& handler2);

  protected:
    static void DoRequest(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange&);

  public:
    static std::string name() { return "SvcSirena"; }

    SvcSirenaInterface() : JxtInterface("", name())
    {
      AddEvent("kick", JXT_HANDLER(SvcSirenaInterface, KickHandler));
      AddEvent("piece_concept", JXT_HANDLER(SvcSirenaInterface, procRequestsFromSirena));

      addResponseHandler(ContinueCheckin);
    }

    static void AvailabilityRequest(xmlNodePtr reqNode,
                                    xmlNodePtr externalSysResNode,
                                    const SirenaExchange::TAvailabilityReq& req)
    {
      DoRequest(reqNode, externalSysResNode, req);
    }

    static void PaymentStatusRequest(xmlNodePtr reqNode,
                                     xmlNodePtr externalSysResNode,
                                     const SirenaExchange::TPaymentStatusReq& req)
    {
      DoRequest(reqNode, externalSysResNode, req);
    }

    void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    void procRequestsFromSirena(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    static void procPassengers( const SirenaExchange::TPassengersReq &req, SirenaExchange::TPassengersRes &res );
    static void procGroupInfo( const SirenaExchange::TGroupInfoReq &req, SirenaExchange::TGroupInfoRes &res );
    static void procPseudoGroupInfo( const SirenaExchange::TPseudoGroupInfoReq &req, SirenaExchange::TPseudoGroupInfoRes &res );
};

#endif
