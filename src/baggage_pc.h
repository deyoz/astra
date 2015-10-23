#ifndef _BAGGAGE_PC_H_
#define _BAGGAGE_PC_H_

#include "astra_misc.h"
#include "passenger.h"

namespace PieceConcept
{

enum TBagStatus { bsUnknown, bsFree, bsPaid, bsNeed, bsNone };

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

    bool operator == (const TRFISCListItem &item) const // �ࠢ�����
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

typedef std::map<std::string/*RFISC*/, TRFISCListItem> TRFISCListMap;

class TRFISCList : public TRFISCListMap
{
  public:
    std::string airline;
    TRFISCList()
    {
      clear();
    }
    void clear()
    {
      TRFISCListMap::clear();
      airline.clear();
    }

    void fromXML(xmlNodePtr node);
    void fromDB(int list_id); // ����㧪� ������ �� ��㯯�
    void toDB(int list_id) const; // ��࠭���� ������ �� ��㯯�
    int crc() const;
    int toDBAdv() const; //�த����⮥ ��࠭���� � �������� ��������� �ࠢ�筨���
    void filter_baggage_rfiscs(); //䨫����� ⮫쪮 �������� ���
};

class TPaxNormTextItem
{
  public:
    std::string lang, text;
};

class TPaxNormItem : public std::map<std::string/*lang*/, TPaxNormTextItem>
{
  public:
    void fromXML(xmlNodePtr node, bool &piece_concept, std::string &airline);
    void fromDB(int pax_id); // ����㧪� ������ �� ���ᠦ���
    void toDB(int pax_id) const; // ��࠭���� ������ �� ���ᠦ���
};

void PaxNormsToStream(const CheckIn::TPaxItem &pax, std::ostringstream &s);

std::string DecodeEmdType(const std::string &s);
std::string EncodeEmdType(const std::string &s);
TBagStatus DecodeBagStatus(const std::string &s);
const std::string EncodeBagStatus(const TBagStatus &s);

class TSimplePaidBagItem
{
  public:
    std::string RFISC;
    std::string RFIC;
    std::string emd_type;
    TBagStatus status;
    bool pr_cabin;

    TSimplePaidBagItem()
    {
      clear();
    }
    void clear()
    {
      RFISC.clear();
      RFIC.clear();
      emd_type.clear();
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

    const TPaidBagItem& toDB(TQuery &Qry) const;
    TPaidBagItem& fromDB(TQuery &Qry);
};

void PaidBagToDB(int grp_id, const std::list<TPaidBagItem> &paid);
void PaidBagFromDB(int grp_id, std::list<TPaidBagItem> &paid);

void PreparePaidBagInfo(int grp_id,
                        int seg_count,
                        std::list<TPaidBagItem> &paid_bag);

void SyncPaidBagEMDToDB(int grp_id,
                        const boost::optional< std::list<CheckIn::TPaidBagEMDItem> > &curr_emd);

void PaidBagViewToXML(const TTrferRoute &trfer,
                      const std::list<TPaidBagItem> &paid,
                      xmlNodePtr node);


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
    const TPaxSegItem& toXML(xmlNodePtr node, const int &ticket_coupon, const std::string &lang) const;
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
      surname=item.surname;
      name=item.name;
      pers_type=item.pers_type;
      seats=item.seats;
      tkn=item.tkn;
      doc=item.doc;
    }

    void clear()
    {
      id=ASTRA::NoExists;
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
      tkn.clear();
      doc.clear();
      segs.clear();
    }

    const TPaxItem& toXML(xmlNodePtr node, const std::string &lang) const;
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
    int grp_id;
    TPaxItem2()
    {
      clear();
    }
    void set(int _grp_id, const CheckIn::TPaxItem &item)
    {
      clear();
      surname=item.surname;
      name=item.name;
      pers_type=item.pers_type;
      seats=item.seats;
      grp_id=_grp_id;
    }

    void clear()
    {
      surname.clear();
      name.clear();
      pers_type=ASTRA::NoPerson;
      seats=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
    }

    const TPaxItem2& toXML(xmlNodePtr node, const std::string &lang) const;
    TPaxItem2& fromDB(TQuery &Qry);
    std::string category() const {
      return category::value( pers_type, seats );
    }
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
  public:
    virtual std::string exchangeId() const=0;
  protected:
    virtual bool isRequest() const=0;
  public:
    std::string error_code, error_message;
    virtual void build(std::string &content) const;
    virtual void parse(const std::string &content);
    virtual void toXML(xmlNodePtr node) const;
    virtual void fromXML(xmlNodePtr node);
    virtual void errorToXML(xmlNodePtr node) const;
    virtual void errorFromXML(xmlNodePtr node);
    bool error() const;
    virtual void clear()=0;
    virtual ~TExchange() {}
};

class TErrorRes : public TExchange
{
  public:
    virtual std::string exchangeId() const { return id; }
  protected:
    virtual bool isRequest() const { return false; }
  public:
    std::string id;
    TErrorRes(const std::string &_id) : id(_id) {}
    virtual void clear() {}
};

//������ ����� � ��७�
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
};

class TAvailabilityResItem
{
  public:
    bool piece_concept;
    std::string airline;
    PieceConcept::TRFISCList rfisc_list;
    PieceConcept::TPaxNormItem norm;
    TAvailabilityResItem()
    {
      clear();
    }
    void clear()
    {
      piece_concept=false;
      airline.clear();
      rfisc_list.clear();
      norm.clear();
    }
};

typedef std::map<TPaxSegKey, TAvailabilityResItem> TAvailabilityResMap;

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
    bool identical_piece_concept();
    bool identical_rfisc_list();
    void normsToDB(int seg_id);
};

typedef std::list< std::pair<TPaxSegKey, TBagItem> > TPaymentStatusList;

class TPaymentStatusReq : public TPaymentStatus
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::list<TPaxItem> paxs;
    TPaymentStatusList bags;
    virtual void clear()
    {
      paxs.clear();
      bags.clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

class TPaymentStatusRes : public TPaymentStatus, public TPaymentStatusList
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    virtual void clear()
    {
      TPaymentStatusList::clear();
    }
    virtual void fromXML(xmlNodePtr node);
    void convert(list<PieceConcept::TPaidBagItem> &paid) const;
    void check_unknown_status(std::set<std::string> &rfiscs) const;
};

//������ ��७� � �����

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
    TPaymentStatusList bags;
    virtual void clear()
    {
      paxs.clear();
      bags.clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

void SendRequest(const TExchange &request, TExchange &response);

} //namespace SirenaExchange

class PieceConceptInterface : public JxtInterface
{
private:
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
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};



#endif
