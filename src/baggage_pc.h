#ifndef _BAGGAGE_PC_H_
#define _BAGGAGE_PC_H_

#include "astra_misc.h"
#include "passenger.h"

namespace PieceConcept
{

enum TBagStatus { bsUnknown, bsFree, bsPaid, bsNeed };

class TRFISCListItem
{
  public:
    std::string RFIC;
    std::string RFISC;
    std::string service_type;
    std::string emd_type;
    std::string name;
    std::string name_lat;

    TRFISCListItem()
    {
      clear();
    }

    void clear()
    {
      RFIC.clear();
      RFISC.clear();
      service_type.clear();
      emd_type.clear();
      name.clear();
      name_lat.clear();
    }

    bool operator == (const TRFISCListItem &item) const // сравнение
    {
      return RFIC==item.RFIC &&
             RFISC==item.RFISC &&
             service_type==item.service_type &&
             emd_type==item.emd_type &&
             name==item.name &&
             name_lat==item.name_lat;
    }

    TRFISCListItem& fromXML(xmlNodePtr node);
    const TRFISCListItem& toDB(TQuery &Qry) const;
    TRFISCListItem& fromDB(TQuery &Qry);
};

class TRFISCList : public std::map<std::string/*RFISC*/, TRFISCListItem>
{
  public:
    void fromXML(xmlNodePtr node);
    void fromDB(int list_id); // загрузка данных по группе
    void toDB(int list_id) const; // сохранение данных по группе
    int crc() const;
    int toDBAdv() const; //продвинутое сохранение с анализом существующих справочников
};

class TPaxNormTextItem
{
  public:
    std::string lang, text;
};

class TPaxNormItem : public std::map<std::string/*lang*/, TPaxNormTextItem>
{
  public:
    void fromXML(xmlNodePtr node, bool &piece_concept);
    void fromDB(int pax_id); // загрузка данных по пассажиру
    void toDB(int pax_id) const; // сохранение данных по пассажиру
};

void PaxNormsToStream(const CheckIn::TPaxItem &pax, std::ostringstream &s);

class TSimplePaidBagItem
{
  public:
    std::string RFISC;
    TBagStatus status;
    bool pr_cabin;

    TSimplePaidBagItem()
    {
      clear();
    }
    void clear()
    {
      RFISC.clear();
      status=bsUnknown;
      pr_cabin=false;
    }
};

class TPaidBagItem : public TSimplePaidBagItem
{
  public:
    int pax_id, trfer_num;

    TPaidBagItem()
    {
      clear();
    }
    void clear()
    {
      TSimplePaidBagItem::clear();
      pax_id=ASTRA::NoExists;
      trfer_num=ASTRA::NoExists;
    }
};

void PreparePaidBagInfo(int grp_id,
                        int seg_count,
                        std::list<TPaidBagItem> &paid_bag);

void SyncPaidBagEMDToDB(int grp_id,
                        const boost::optional< std::list<CheckIn::TPaidBagEMDItem> > &curr_emd);

} //namespace PieceConcept

namespace SirenaExchange
{

class TSegItem
{
  public:
    int id;
    TTripInfo operFlt;
    boost::optional<TTripInfo> markFlt;
    std::string airp_arv;
    std::string craft;
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
             const TTripInfo &operFlt_,
             const CheckIn::TPaxGrpItem grp,
             const TGrpMktFlight &mktFlight)
    {
      clear();
      id=seg_id;
      operFlt=operFlt_;
      operFlt.scd_out=UTCToLocal(operFlt_.scd_out, AirpTZRegion(operFlt_.airp));
      airp_arv=grp.airp_arv;
      markFlt=boost::none;
      if (!mktFlight.empty())
      {
        markFlt=TTripInfo();
        markFlt.get().Init(mktFlight);
      };
    }

    void clear()
    {
      id=ASTRA::NoExists;
      operFlt.Clear();
      markFlt=boost::none;
      airp_arv.clear();
      craft.clear();
    }

    const TSegItem& toXML(xmlNodePtr node, const std::string &lang) const;
    static std::string flight(const TTripInfo &flt, const std::string &lang);
};

class TPaxSegItem : public TSegItem
{
  public:
    std::string subcl;
    std::list<CheckIn::TPnrAddrItem> pnrs;
    std::vector<CheckIn::TPaxFQTItem> fqts;
    TPaxSegItem()
    {
      TSegItem::clear();
      clear();
    }
    void clear()
    {
      subcl.clear();
      pnrs.clear();
      fqts.clear();
    }
    const TPaxSegItem& toXML(xmlNodePtr node, const std::string &lang) const;
};

typedef std::map<int, TPaxSegItem> TPaxSegMap;

class TPaxItem
{
  public:
    int id;
    std::string name;
    ASTRA::TPerson pers_type;
    int seats;
    CheckIn::TPaxTknItem tkn;
    CheckIn::TPaxDocItem doc;

    TPaxSegMap segs;

    TPaxItem()
    {
      clear();
    }
    void set(const CheckIn::TPaxItem &item)
    {
      clear();
      id=item.id;
      name=item.name;
      pers_type=item.pers_type;
      seats=item.seats;
      tkn=item.tkn;
      doc=item.doc;
    }

    void clear()
    {
      id=ASTRA::NoExists;
      name.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
      tkn.clear();
      doc.clear();
      segs.clear();
    }

    const TPaxItem& toXML(xmlNodePtr node, const std::string &lang) const;
    std::string category() const;
    std::string sex() const;
};

class TBagItem : public PieceConcept::TSimplePaidBagItem
{
  public:
    TBagItem() {}
    TBagItem(const PieceConcept::TPaidBagItem &item) : PieceConcept::TSimplePaidBagItem(item) {}
    const TBagItem& toXML(xmlNodePtr node) const;
    TBagItem& fromXML(xmlNodePtr node);
};

class TPaxSegKey
{
  public:
    int pax_id, seg_id;
    TPaxSegKey()
    {
      clear();
    }
    TPaxSegKey(int _pax_id, int _seg_id)
    {
      pax_id=_pax_id;
      seg_id=_seg_id;
    }
    void clear()
    {
      pax_id=ASTRA::NoExists;
      seg_id=ASTRA::NoExists;
    }
    bool operator < (const TPaxSegKey &key) const
    {
      if (pax_id!=key.pax_id)
        return pax_id<key.pax_id;
      return seg_id<key.seg_id;
    }

    const TPaxSegKey& toXML(xmlNodePtr node) const;
    TPaxSegKey& fromXML(xmlNodePtr node);
};

class TExchange
{
  protected:
    virtual std::string exchangeId() const=0;
    virtual bool isRequest() const=0;
  public:
    virtual void build(std::string &content) const;
    virtual void parse(const std::string &content);
    virtual void toXML(xmlNodePtr node) const;
    virtual void fromXML(xmlNodePtr node);
    virtual ~TExchange() {}
};

class TAvailabilityReq : public TExchange
{
  protected:
    virtual std::string exchangeId() const { return "svc_availability"; }
    virtual bool isRequest() const { return true; };
  public:
    std::list<TPaxItem> paxs;
    virtual void toXML(xmlNodePtr node) const;
};

class TAvailabilityResItem
{
  public:
    bool piece_concept;
    PieceConcept::TRFISCList rfisc_list;
    PieceConcept::TPaxNormItem norm;
    TAvailabilityResItem()
    {
      clear();
    }
    void clear()
    {
      piece_concept=false;
      rfisc_list.clear();
      norm.clear();
    }
};

typedef std::map<TPaxSegKey, TAvailabilityResItem> TAvailabilityResMap;

class TAvailabilityRes : public TExchange, public TAvailabilityResMap
{
  protected:
    virtual std::string exchangeId() const { return "svc_availability"; }
    virtual bool isRequest() const { return false; };
  public:
    virtual void fromXML(xmlNodePtr node);
    bool identical_piece_concept();
    bool identical_rfisc_list();
    void normsToDB(int seg_id);
};

class TPaymentStatusReq : public TExchange
{
  protected:
    virtual std::string exchangeId() const { return "svc_payment_status"; }
    virtual bool isRequest() const { return true; };
  public:
    std::list<TPaxItem> paxs;
    std::list< std::pair<TPaxSegKey, TBagItem> > bags;
    virtual void toXML(xmlNodePtr node) const;
};

typedef std::list< std::pair<TPaxSegKey, TBagItem> > TPaymentStatusResMap;

class TPaymentStatusRes : public TExchange, public TPaymentStatusResMap
{
  protected:
    virtual std::string exchangeId() const { return "svc_payment_status"; }
    virtual bool isRequest() const { return false; };
  public:
    virtual void fromXML(xmlNodePtr node);
};

void SendRequest(const TExchange &request, TExchange &response);

} //namespace SirenaExchange


#endif
