#ifndef _BAGGAGE_PC_H_
#define _BAGGAGE_PC_H_

#include "astra_misc.h"
#include "passenger.h"
#include "term_version.h"
#include "emdoc.h"
namespace PieceConcept
{


struct TNodeList {
    enum ConceptType {ctInitial, ctAll, ctPiece, ctWeight} concept;
    typedef std::vector<std::pair<xmlNodePtr, bool> > TConceptList; // bool: false - weight, true - seat
    TConceptList items;
    bool must_work;
    void set_concept(xmlNodePtr& node, bool val);
    TNodeList(): concept(ctInitial) {
        must_work = true;
        if (TReqInfo::Instance()->client_type != ASTRA::ctTerm || ! TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2))
            must_work = false;
    }
    ~TNodeList()
    {  if(must_work) apply();
        //временно отключено

    }
private:
     void apply();
};


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
    void fromDB(int list_id); // загрузка только списка RFISC
    void toDB(int list_id) const; // сохранение только списка RFISC
    int crc() const;
    void fromDBAdv(int list_id); //продвинутая загрузка данных по группе
    int toDBAdv() const; //продвинутое сохранение с анализом существующих справочников
    void filter_baggage_rfiscs(); //фильтрация только багажных услуг
    std::string localized_name(const std::string& rfisc, const std::string& lang) const; //локализованное описание RFISC
};

class TRFISCSetting
{
  public:
    std::string RFISC;
    int priority;
    int min_weight, max_weight;

    TRFISCSetting()
    {
      clear();
    }
    void clear()
    {
      RFISC.clear();
      priority=ASTRA::NoExists;
      min_weight=ASTRA::NoExists;
      max_weight=ASTRA::NoExists;
    }

    TRFISCSetting& fromDB(TQuery &Qry);
};

class TRFISCSettingList : public std::map<std::string/*RFISC*/, TRFISCSetting>
{
  public:
    TRFISCSettingList()
    {
      clear();
    }

    void fromDB(const std::string &airline);
    void check(const CheckIn::TBagItem &bag) const;
};

class TRFISCListWithSets : public TRFISCList, public TRFISCSettingList
{
  public:
    TRFISCListWithSets()
    {
      clear();
    }
    void clear()
    {
      TRFISCList::clear();
      TRFISCSettingList::clear();
    }

    void fromDB(int list_id);
};

class TPaxNormTextItem
{
  public:
    std::string lang, text;
};

class TPaxNormItem : public std::map<std::string/*lang*/, TPaxNormTextItem>
{
  public:
    void fromXML(xmlNodePtr node);
    void fromXMLAdv(xmlNodePtr node, bool &piece_concept, std::string &airline);
    void fromDB(int pax_id); // загрузка данных по пассажиру
    void toDB(int pax_id) const; // сохранение данных по пассажиру
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
void PaidBagFromDB(int id, bool is_grp_id, std::list<TPaidBagItem> &paid);

std::string GetBagRcptStr(int grp_id, int pax_id);
bool BagPaymentCompleted(int pax_id);

void PreparePaidBagInfo(int grp_id,
                        int seg_count,
                        std::list<TPaidBagItem> &paid_bag);

void TryDelPaidBagEMD(int grp_id,
                      const std::list<PieceConcept::TPaidBagItem> &curr_paid);

bool TryAddPaidBagEMD(std::list<TPaidBagItem> &paid_bag,
                      std::list<CheckIn::TPaidBagEMDItem> &paid_bag_emd,
                      const CheckIn::TPaidBagEMDProps &paid_bag_emd_props);

void PaidBagEMDToDB(int grp_id,
                    const CheckIn::PaidBagEMDList &prior_emds,
                    boost::optional< std::list<CheckIn::TPaidBagEMDItem> > &curr_emds);

void PaidBagViewToXML(const TTrferRoute &trfer,
                      const std::list<TPaidBagItem> &paid,
                      const TRFISCList &rfisc_list,
                      xmlNodePtr node);

} //namespace PieceConcept

namespace SirenaExchange
{

class TSegItem
{
  public:
    int id;
    TTripInfo operFlt;
    bool scd_out_contain_time;
    boost::optional<TTripInfo> markFlt;
    std::string airp_arv;
    BASIC::TDateTime scd_in;
    bool scd_in_contain_time;
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
             const TTripInfo &_operFlt,
             const CheckIn::TPaxGrpItem grp,
             const TGrpMktFlight &mktFlight,
             const BASIC::TDateTime &_scd_in)
    {
      clear();
      id=seg_id;
      operFlt=_operFlt;
      if (operFlt.scd_out!=ASTRA::NoExists)
      {
        operFlt.scd_out=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));
        scd_out_contain_time=true;
      };
      airp_arv=grp.airp_arv;
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
        markFlt.get().Init(mktFlight);
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
    int reg_no;
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

class TErrorReference
{
  public:
    std::string path, value;
    int pax_id, seg_id;
    TErrorReference()
    {
      clear();
    }
    void clear()
    {
      path.clear();
      value.clear();
      pax_id=ASTRA::NoExists;
      seg_id=ASTRA::NoExists;
    }
    bool empty() const
    {
      return path.empty() &&
             value.empty() &&
             pax_id==ASTRA::NoExists &&
             seg_id==ASTRA::NoExists;
    }
    void toXML(xmlNodePtr node) const;
    void fromXML(xmlNodePtr node);
    std::string traceStr() const;
};

class TExchange
{
  public:
    virtual std::string exchangeId() const=0;
  protected:
    virtual bool isRequest() const=0;
  public:
    std::string error_code, error_message;
    TErrorReference error_reference;
    virtual void build(std::string &content) const;
    virtual void parse(const std::string &content);
    virtual void toXML(xmlNodePtr node) const;
    virtual void fromXML(xmlNodePtr node);
    virtual void errorToXML(xmlNodePtr node) const;
    virtual void errorFromXML(xmlNodePtr node);
    bool error() const;
    std::string traceError() const;
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

typedef std::list< std::pair<TPaxSegKey, TBagItem> > TBagList;
typedef std::list< std::pair<TPaxSegKey, PieceConcept::TPaxNormItem> > TPaxNormList;

class TPaymentStatusReq : public TPaymentStatus
{
  protected:
    virtual bool isRequest() const { return true; }
  public:
    std::list<TPaxItem> paxs;
    TBagList bags;
    virtual void clear()
    {
      paxs.clear();
      bags.clear();
    }
    virtual void toXML(xmlNodePtr node) const;
};

class TPaymentStatusRes : public TPaymentStatus
{
  protected:
    virtual bool isRequest() const { return false; }
  public:
    TBagList bags;
    TPaxNormList norms;
    virtual void clear()
    {
      bags.clear();
      norms.clear();
    }
    virtual void fromXML(xmlNodePtr node);
    void normsToDB(int seg_id);
    void convert(std::list<PieceConcept::TPaidBagItem> &paid) const;
    void check_unknown_status(std::set<std::string> &rfiscs) const;
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
    TBagList bags;
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
