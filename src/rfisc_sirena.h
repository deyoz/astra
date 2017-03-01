#ifndef _RFISC_SIRENA_H_
#define _RFISC_SIRENA_H_

#include "sirena_exchange.h"
#include "astra_misc.h"
#include "etick.h"
#include "passenger.h"

namespace SirenaExchange
{

using namespace BASIC::date_time;

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
        catch (std::bad_cast)
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

    const TSegItem& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
    static std::string flight(const TTripInfo &flt, const std::string &lang);
};

class TPaxSegItem : public TSegItem
{
  public:
    std::string subcl;
    CheckIn::TPaxTknItem tkn;
    std::list<CheckIn::TPnrAddrItem> pnrs;
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
      pnrs.clear();
      fqts.clear();
    }
    const TPaxSegItem& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
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

    const TPaxItem& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
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
    }

    const TPaxItem2& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
    std::string category() const {
      return category::value( pers_type, seats );
    }
};

class TSvcItem : public TPaxSegRFISCKey
{
  public:
    TServiceStatus::Enum status;
    std::string ssr_text;
    TSvcItem() { clear(); }
    TSvcItem(const TPaxSegRFISCKey& _item, const TServiceStatus::Enum _status) :
      TPaxSegRFISCKey(_item), status(_status) {}
    void clear()
    {
      TPaxSegRFISCKey::clear();
      status=TServiceStatus::Unknown;
      ssr_text.clear();
    }
    const TSvcItem& toSirenaXML(xmlNodePtr node, const std::string &lang) const;
    TSvcItem& fromSirenaXML(xmlNodePtr node);
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

class TAvailabilityReq : public TAvailability
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::list<TPaxItem> paxs;
    virtual void clear()
    {
      paxs.clear();
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
    bool identical_rfisc_list(int seg_id, boost::optional<TRFISCList> &rfisc_list) const;
    bool exists_rfisc(int seg_id, TServiceType::Enum service_type) const;
    void rfiscsToDB(const TCkinGrpIds &tckin_grp_ids, bool old_version) const;
    void normsToDB(const TCkinGrpIds &tckin_grp_ids) const;
    void brandsToDB(const TCkinGrpIds &tckin_grp_ids) const;
};

class TSvcList : public std::list<TSvcItem>
{
  public:
    void set(int grp_id, int tckin_seg_count, int trfer_seg_count);
    void get(TPaidRFISCList &paid) const;
};

class TPaymentStatusReq : public TPaymentStatus
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::list<TPaxItem> paxs;
    TSvcList svcs;
    virtual void clear()
    {
      paxs.clear();
      svcs.clear();
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

class TGroupInfoRes : public TGroupInfo
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    std::list<TPaxItem> paxs;
    TSvcList svcs;
    virtual void clear()
    {
      paxs.clear();
      svcs.clear();
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

class TPseudoGroupInfoRes : public TPseudoGroupInfo
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    std::list<TPaxItem> paxs;
    TSvcList svcs;
    virtual void clear()
    {
      paxs.clear();
      svcs.clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

void SendRequest(const TExchange &request, TExchange &response,
                 RequestInfo &requestInfo, ResponseInfo &responseInfo);
void SendRequest(const TExchange &request, TExchange &response);

void fillPaxsBags(int first_grp_id, TExchange &exch, CheckIn::TPaxGrpCategory::Enum &grp_cat, TCkinGrpIds &tckin_grp_ids,
                  bool include_refused=false);
void fillPaxsSvcs(const TEntityList &entities, TExchange &exch);

} //namespace SirenaExchange

void unaccBagTypesToDB(int grp_id, bool ignore_unaccomp_sets=false);
void CopyPaxServiceLists(int grp_id_src, int grp_id_dest, bool is_grp_id, bool rfisc_used);
void UpgradeDBForServices(int grp_id); //!!!потом удалить

class PieceConceptInterface : public JxtInterface
{
public:
  PieceConceptInterface() : JxtInterface("","PieceConcept")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PieceConceptInterface>::CreateHandler(&PieceConceptInterface::procPieceConcept);
     AddEvent("piece_concept",evHandle);
  };
  void procPieceConcept(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void procPassengers( const SirenaExchange::TPassengersReq &req, SirenaExchange::TPassengersRes &res );
  static void procGroupInfo( const SirenaExchange::TGroupInfoReq &req, SirenaExchange::TGroupInfoRes &res );
  static void procPseudoGroupInfo( const SirenaExchange::TPseudoGroupInfoReq &req, SirenaExchange::TPseudoGroupInfoRes &res );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

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

#endif
