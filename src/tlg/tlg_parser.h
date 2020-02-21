#ifndef _TLG_PARSER_H_
#define _TLG_PARSER_H_

#include <string>
#include <vector>
#include "astra_consts.h"
#include "astra_misc.h"
#include "date_time.h"
#include "exceptions.h"
#include "oralib.h"
#include "seats_utils.h"
#include "memory_manager.h"
#include "flt_binding.h"

using BASIC::date_time::TDateTime;

namespace TypeB
{

enum ETlgErrorType{tlgeNotError,
                   tlgeNotMonitorNotAlarm,
                   tlgeYesMonitorNotAlarm,
                   tlgeNotMonitorYesAlarm,
                   tlgeYesMonitorYesAlarm};  //порядок enum важен и определяет приоритет

class ETlgError:public EXCEPTIONS::Exception
{
  private:
    ETlgErrorType type;
    int pos, len, line;
    std::string text;
  public:
    ETlgError(const char *format, ...):EXCEPTIONS::Exception(""),
                                       type(tlgeYesMonitorYesAlarm),
                                       pos(ASTRA::NoExists),
                                       len(ASTRA::NoExists),
                                       line(ASTRA::NoExists)
    {
      va_list ap;
      va_start(ap, format);
      vsnprintf(Message, sizeof(Message), format, ap);
      Message[sizeof(Message)-1]=0;
      va_end(ap);
    }

    ETlgError(const std::string &msg) :EXCEPTIONS::Exception(msg),
                                       type(tlgeYesMonitorYesAlarm),
                                       pos(ASTRA::NoExists),
                                       len(ASTRA::NoExists),
                                       line(ASTRA::NoExists)
    {}

    ETlgError(ETlgErrorType t,
              const std::string &msg) :EXCEPTIONS::Exception(msg),
                                       type(t),
                                       pos(ASTRA::NoExists),
                                       len(ASTRA::NoExists),
                                       line(ASTRA::NoExists)
    {}

    ETlgError(int p, int l, const std::string &t, int ln,
              const std::string &msg) :EXCEPTIONS::Exception(msg),
                                       type(tlgeYesMonitorYesAlarm),
                                       pos(p),
                                       len(l),
                                       line(ln),
                                       text(t)
    {}

    ~ETlgError() throw(){}

    ETlgErrorType error_type() const { return type; }
    int error_pos() const { return pos; }
    int error_len() const { return len; }
    int error_line() const { return line; }
    std::string error_text() const { return text; }
};

enum TTlgCategory{
    tcUnknown,
    tcDCS,
    tcBSM,
    tcAHM,
    tcSSM,
    tcASM,
    tcLCI,
    tcUCM,
    tcCPM,
    tcSLS,
    tcLDM,
    tcNTM,
    tcIFM,
};

enum TTlgElement
              {//общие
               Address,
               CommunicationsReference,
               MessageIdentifier,
               FlightElement,
               //PNL и ADL
               AssociationNumber,
               BonusPrograms,
               //PNL, ADL, PRL
               Configuration,
               ClassCodes,
               //PNL и ADL
               SpaceAvailableElement,
               TranzitElement,
               //PNL, ADL, PRL
               TotalsByDestination,
               //PTM
               TransferPassengerData,
               //SOM
               SeatingCategories,
               SeatsByDestination,
               SupplementaryInfo,
               //MVT
               AircraftMovementInfo,
               //LDM
               LoadInfoAndRemarks,
               //SSM
               TimeModeElement,
               MessageSequenceReference,
               ActionIdentifier,
               PeriodFrequency,
               NewFlight,
               Equipment,
               Routing,
               Segment,
               SubSI,
               SubSIMore,
               SubSeparator,
               SI,
               SIMore,
               Reject,
               RejectBody,
               RepeatOfRejected,
               //LCI
               ActionCode,
               LCIData,
               //общие
               EndOfMessage};

extern const char* TTlgElementS[];

enum TIndicator{None,ADD,CHG,DEL};

enum TElemPresence{epMandatory,epOptional,epNone};

class TTlgPartInfo
{
  public:
    const char *p;
    int EOL_count, offset;
    TTlgPartInfo()
    {
      p=NULL;
      EOL_count=0;
      offset=0;
    };
};

struct TTlgParts
{
  TTlgPartInfo addr,heading,body,ending;
};

struct TTlgPartsText
{
  std::string addr,heading,body,ending;
  void clear()
  {
    addr.clear();
    heading.clear();
    body.clear();
    ending.clear();
  };
};

class THeadingInfo
{
  public:
    char sender[8];
    char double_signature[3];
    std::string message_identity;
    TDateTime time_create;
    char tlg_type[4];
    TTlgCategory tlg_cat;

    THeadingInfo()
    {
      *sender=0;
      *double_signature=0;
      time_create=0;
      *tlg_type=0;
      tlg_cat=tcUnknown;
    };
    THeadingInfo(THeadingInfo &info)
    {
      strcpy(sender,info.sender);
      strcpy(double_signature,info.double_signature);
      message_identity=info.message_identity;
      time_create=info.time_create;
      strcpy(tlg_type,info.tlg_type);
      tlg_cat=info.tlg_cat;
    };
    virtual ~THeadingInfo(){};
};

class TDCSHeadingInfo : public THeadingInfo
{
  public:
    TFltInfo flt;
    long part_no;
    long association_number;

    TDCSHeadingInfo() : THeadingInfo()
    {
      part_no=0;
      association_number=ASTRA::NoExists;
    };
    TDCSHeadingInfo(THeadingInfo &info) : THeadingInfo(info)
    {
      part_no=0;
      association_number=ASTRA::NoExists;
    };
};


class TBSMHeadingInfo : public THeadingInfo
{
  public:
    long part_no;
    char airp[4];
    std::string reference_number;
    TBSMHeadingInfo() : THeadingInfo()
    {
      *airp=0;
      part_no=0;
    };
    TBSMHeadingInfo(THeadingInfo &info) : THeadingInfo(info)
    {
      *airp=0;
      part_no=0;
    };
};

class TAHMHeadingInfo : public THeadingInfo
{
  public:
    TFltInfo flt;
    TBindType bind_type;
    TAHMHeadingInfo() : THeadingInfo() {};
    TAHMHeadingInfo(THeadingInfo &info) : THeadingInfo(info)  {};
};

class TEndingInfo
{
  public:
    long part_no;
    bool pr_final_part;
    TEndingInfo()
    {
      part_no=0;
      pr_final_part=true;
    };
};

typedef struct
{
  char subcl[2];
  long seats;
  ASTRA::TClass cl;
} TSeatsItem;

typedef struct
{
  char subcl[2];
  std::string rbds;
  ASTRA::TClass cl;
} TRbdItem;

typedef struct
{
  char station[4];
  std::vector<TSeatsItem> seats;
} TRouteItem;

class TDocExtraItem
{
  public:
    char type_rcpt[4],no[16];
    TDocExtraItem()
    {
      Clear();
    };
    void Clear()
    {
      *type_rcpt=0;
      *no=0;
    };
    bool Empty() const
    {
      return  *type_rcpt==0 &&
              *no==0;
    };
};

class TDetailRemAncestor
{
  public:
    char rem_code[6],rem_status[3];
    bool pr_inf;
    TDetailRemAncestor()
    {
      Clear();
    };
    virtual ~TDetailRemAncestor() {};

    bool isHKReservationsStatus() const
    {
      return strcmp(rem_status, "HK")==0;
    }

    bool infIndicatorExists() const
    {
      return pr_inf;
    }

  protected:
    void Clear()
    {
      *rem_code=0;
      *rem_status=0;
      pr_inf=false;
    };
    bool Empty() const
    {
      return pr_inf==false;
    };
    bool operator == (const TDetailRemAncestor &item) const
    {
      return strcmp(rem_code, item.rem_code)==0 &&
             strcmp(rem_status, item.rem_status)==0 &&
             pr_inf==item.pr_inf;
    }
};

class TDocItem : public TDetailRemAncestor
{
  public:
    char type[3],issue_country[4],no[16];
    char nationality[4],gender[3];
    TDateTime birth_date,expiry_date;
    std::string surname,first_name,second_name;
    bool pr_multi;
    TDocItem()
    {
      Clear();
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *issue_country=0;
      *no=0;
      *nationality=0;
      *gender=0;
      birth_date=ASTRA::NoExists;
      expiry_date=ASTRA::NoExists;
      surname.clear();
      first_name.clear();
      second_name.clear();
      pr_multi=false;
    };
    bool Empty() const
    {
      return TDetailRemAncestor::Empty() &&
             *type==0 &&
             *issue_country==0 &&
             *no==0 &&
             *nationality==0 &&
             *gender==0 &&
             birth_date==ASTRA::NoExists &&
             expiry_date==ASTRA::NoExists &&
             surname.empty() &&
             first_name.empty() &&
             second_name.empty() &&
             pr_multi==false;
    };
};

class TDocoItem : public TDetailRemAncestor
{
  public:
    char type[3],no[26],applic_country[4];
    TDateTime issue_date;
    std::string birth_place, issue_place;
    TDocoItem()
    {
      Clear();
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *no=0;
      *applic_country=0;
      issue_date=ASTRA::NoExists;
      birth_place.clear();
      issue_place.clear();
    };
    bool Empty() const
    {
      return TDetailRemAncestor::Empty() &&
             *type==0 &&
             *no==0 &&
             *applic_country==0 &&
             issue_date==ASTRA::NoExists &&
             birth_place.empty() &&
             issue_place.empty();
    };
};

class TDocaItem : public TDetailRemAncestor
{
  public:
    char type[2],country[4];
    std::string address, city, region, postal_code;
    TDocaItem()
    {
      Clear();
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *type=0;
      *country=0;
      address.clear();
      city.clear();
      region.clear();
      postal_code.clear();
    };
    bool Empty() const
    {
      return TDetailRemAncestor::Empty() &&
             *type==0 &&
             *country==0 &&
             address.empty() &&
             city.empty() &&
             region.empty() &&
             postal_code.empty();
    };
};

class TTKNItem : public TDetailRemAncestor
{
  public:
    char ticket_no[16];
    int coupon_no;
    TTKNItem()
    {
      Clear();
    }
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *ticket_no=0;
      coupon_no=0;
    }
    void toDB(TQuery &Qry) const;
};

class TFQTItem
{
  public:
    char rem_code[6],airline[4],no[26];
    std::string extra;
    TFQTItem()
    {
      Clear();
    };
    void Clear()
    {
      *rem_code=0;
      *airline=0;
      *no=0;
      extra.clear();
    };
    bool operator < (const TFQTItem &item) const
    {
      int res;
      res=strcmp(no, item.no);
      if (res!=0) return res<0;
      res=strcmp(airline, item.airline);
      return res<0;
    }
};

class TFQTExtraItem
{
  public:
    std::string tier_level;
    TFQTExtraItem()
    {
      Clear();
    }
    void Clear()
    {
      tier_level.clear();
    }
    bool Empty() const
    {
      return tier_level.empty();
    }
    bool operator < (const TFQTExtraItem &item) const
    {
      return tier_level<item.tier_level;
    }
};

class TCHKDItem : public TDetailRemAncestor
{
  public:
    long int reg_no;
    TCHKDItem()
    {
      Clear();
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      reg_no=ASTRA::NoExists;
    };
    bool Empty() const
    {
      return TDetailRemAncestor::Empty() &&
             reg_no==ASTRA::NoExists;
    };
};

class TASVCItem : public TDetailRemAncestor
{
  public:
    char RFIC[2], RFISC[16], ssr_code[5], service_name[31];
    int service_quantity;
    char emd_type[2], emd_no[14];
    int emd_coupon;
    TASVCItem()
    {
      Clear();
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *RFIC=0;
      *RFISC=0;
      service_quantity=ASTRA::NoExists;
      *ssr_code=0;
      *service_name=0;
      *emd_type=0;
      *emd_no=0;
      emd_coupon=ASTRA::NoExists;
    };
    bool Empty() const
    {
      return TDetailRemAncestor::Empty() &&
             *RFIC==0 &&
             *RFISC==0 &&
             service_quantity==ASTRA::NoExists &&
             *ssr_code==0 &&
             *service_name==0 &&
             *emd_type==0 &&
             *emd_no==0 &&
             emd_coupon==ASTRA::NoExists;
    };
    bool emdRequired() const
    {
      return strncmp(rem_status, "HD", 2)==0;
    }

    bool operator == (const TASVCItem &item) const
    {
      return TDetailRemAncestor::operator ==(item) &&
             strcmp(RFIC, item.RFIC)==0 &&
             strcmp(RFISC, item.RFISC)==0 &&
             service_quantity==item.service_quantity &&
             strcmp(ssr_code, item.ssr_code)==0 &&
             strcmp(service_name, item.service_name)==0 &&
             strcmp(emd_type, item.emd_type)==0 &&
             strcmp(emd_no, item.emd_no)==0 &&
             emd_coupon==item.emd_coupon;
    }
};

class TSeatBlockingRem : public TDetailRemAncestor
{
  public:
    int numberOfSeats;
    TSeatBlockingRem() { clear(); }
    void clear()
    {
      TDetailRemAncestor::Clear();
      numberOfSeats=0;
    }
    bool isSTCR() const
    {
      return strcmp(rem_code, "STCR")==0;
    }
};

class TRemItem
{
  public:
    std::string text;
    char code[6];
    TRemItem()
    {
      *code=0;
    }
    bool isPDRem() const
    {
      return strlen(code)==4 && strncmp(code, "PD", 2)==0;
    }
};

class PassengerSystemId
{
  public:
    std::string uniqueReference;
    bool infantIndicator=false;

    bool infIndicatorExists() const { return infantIndicator; }
};

class TPDRemItem : public TRemItem
{
  public:
    TPDRemItem(const TRemItem &item) : TRemItem(item) {}
    TPDRemItem() : TRemItem() {}
    bool operator < (const TPDRemItem &item) const
    {
      return text<item.text;
    }
    bool operator == (const TPDRemItem &item) const
    {
      return text==item.text;
    }
};

typedef std::pair<std::string,std::string> TChdItem;

class PersonAncestor
{
  public:
    std::vector<TRemItem> rem;
    std::vector<TDocItem> doc;
    std::map<std::string/*no*/, TDocExtraItem> doc_extra;
    std::vector<TDocoItem> doco;
    std::vector<TDocaItem> doca;
    std::vector<TTKNItem> tkn;
    std::vector<TCHKDItem> chkd;
    boost::optional<PassengerSystemId> systemId;

    void add(const TRemItem& item)  { rem.push_back(item); }
    void add(const TDocItem& item)  { doc.push_back(item); }
    void add(const TDocoItem& item) { doco.push_back(item); }
    void add(const TDocaItem& item) { doca.push_back(item); }
    void add(const TTKNItem& item)  { tkn.push_back(item); }
    void add(const TCHKDItem& item) { chkd.push_back(item); }
    void add(const PassengerSystemId& id);

    std::string uniqueReference() const { return systemId?systemId.get().uniqueReference:""; }

    void clear()
    {
      rem.clear();
      doc.clear();
      doc_extra.clear();
      doco.clear();
      doca.clear();
      tkn.clear();
      chkd.clear();
    }
};

class TInfItem : public PersonAncestor
{
  public:
    std::string surname,name;
    long age;
    TInfItem()
    {
      Clear();
    }
    void Clear()
    {
      PersonAncestor::clear();
      surname.clear();
      name.clear();
      age=0;
    }
    bool Empty() const
    {
      return surname.empty() && name.empty();
    }

    using PersonAncestor::add;
    void add(const TASVCItem& item)
    {
      throw EXCEPTIONS::Exception("TInfItem::add(TASVCItem) not applicable");
    }
};

class TInfList : public std::vector<TInfItem>
{
  public:
    void removeIfExistsIn(const TInfList &infs);
    void removeEmpty();
    void removeDup();
    void setSurnameIfEmpty(const std::string &surname);
};

class TSeatsBlockingItem;

class TSeatsBlockingList : public std::vector<TSeatsBlockingItem>
{
  public:
    void toDB(const int& paxId) const;
    void fromDB(const int& paxId);
    void replace(const TSeatsBlockingList& src, bool isSpecial, long& seats);
};

class TNameElement;
class TTlgParser;

class TPaxItem : public PersonAncestor
{
  public:
    std::string name;
    ASTRA::TPerson pers_type;
    long seats;
    TSeatRanges seatRanges;
    TSeat seat; //это место, назначенное разборщиком на основе tlg_comp_layers
    char seat_rem[5];
    TInfList inf;
    std::vector<TFQTItem> fqt;
    std::set<TFQTExtraItem> fqt_extra;
    std::vector<TASVCItem> asvc;
    TSeatsBlockingList seatsBlocking;
    std::list<TSeatBlockingRem> seatBlockingRemList;
    std::vector<PassengerSystemId> systemIds;
    TPaxItem()
    {
      pers_type=ASTRA::adult;
      seats=1;
      *seat_rem=0;
    }
    using PersonAncestor::add;
    void add(const TASVCItem& item) { asvc.push_back(item); }

    bool emdRequired(const std::string& ssr_code) const;
    void removeNotConfimedSSRs();
    bool isSeatBlocking() const { return isSeatBlockingRem(name); }
    bool isCBBG() const { return name=="CBBG"; }
    void bindSeatsBlocking(const TNameElement& ne,
                           const std::string& remCode,
                           TSeatsBlockingList& srcSeatsBlocking);
    void setSomeDataForSeatsBlocking(const int& paxId, const TNameElement& ne);
    void setSomeDataForSeatsBlocking(const TNameElement& ne);
    void getTknNumbers(std::list<std::string>& result) const;
    void moveTknWithNumber(const std::string& no, std::vector<TTKNItem>& dest);
    void fillSeatBlockingRemList(TTlgParser &tlg);
    bool dontSaveToDB(const TNameElement& ne) const;

    static bool isSeatBlockingRem(const std::string &rem_code);
    static int getNotUsedSeatBlockingId(const int& paxId);
    static void getAndLockSeatBlockingIds(const int& paxId, std::set<int>& seatIds);
};

class TSeatsBlockingItem : public TPaxItem
{
  public:
    std::string surname;
    TSeatsBlockingItem(const std::string& _surname, const TPaxItem& paxItem) :
      TPaxItem(paxItem), surname(_surname) {}
    TSeatsBlockingItem(const std::string& _surname, const std::string& _name) :
      surname(_surname)
    {
      name=_name;
      pers_type=ASTRA::NoPerson;
    }
    bool isSpecial() const { return surname=="ZZ"; }
};

class TSegmentItem : public TFltInfo
{
  public:
    long local_date;
    char local_time_dep[5];
    char local_time_arv[5];
    char subcl[2];
    int date_variation;
    char status[3];
    TSegmentItem() : TFltInfo()
    {
      Clear();
    };
    void Clear()
    {
      TFltInfo::Clear();
      local_date=0;
      *local_time_dep=0;
      *local_time_arv=0;
      *subcl=0;
      date_variation=ASTRA::NoExists;
      *status=0;
    };
};

class TTransferItem : public TSegmentItem
{
  public:
    long num;
    TTransferItem() : TSegmentItem()
    {
      Clear();
    };
    void Clear()
    {
      TSegmentItem::Clear();
      num=0;
    };
};

class TTagItem
{
  public:
    char alpha_no[4];
    double numeric_no;
    int num;
    char airp_arv_final[4];
    TTagItem()
    {
      *alpha_no=0;
      numeric_no=0;
      num=0;
      *airp_arv_final=0;
    };
};

class TBSMTagItem
{
  public:
    double first_no;
    int num;
    TBSMTagItem()
    {
      first_no=0.0;
      num=0;
    };
};

class TBagItem
{
  public:
    long bag_amount,bag_weight,rk_weight;
    char weight_unit[2];
    TBagItem()
    {
      Clear();
    };
    void Clear()
    {
      bag_amount=0;
      bag_weight=0;
      rk_weight=0;
      *weight_unit=0;
    };
    bool Empty() const
    {
      return *weight_unit==0;

    };
};

class TNameElement
{
  public:
    TIndicator indicator;
    long seats;
    std::string surname;
    std::vector<TPaxItem> pax;
    std::vector<TRemItem> rem;
    TSeatRanges seatRanges;
    TBagItem bag;
    std::vector<TTagItem> tags;
    int bag_pool;
    TNameElement()
    {
      Clear();
    }
    void Clear()
    {
      indicator=None;
      seats=0;
      surname.clear();
      pax.clear();
      rem.clear();
      seatRanges.clear();
      bag.Clear();
      tags.clear();
      bag_pool=ASTRA::NoExists;
    }

    void removeNotConfimedSSRs();
    bool containsSinglePassenger() const;
    void separateSeatsBlocking(TSeatsBlockingList& dest);
    void bindSeatsBlocking(const std::string& remCode,
                           TSeatsBlockingList& srcSeatsBlocking);
    bool seatIsUsed(const TSeat& seat) const;
    void setNotUsedSeat(TSeatRanges& seats, TPaxItem& paxItem, bool moveSeat) const;
    bool isSpecial() const { return surname=="ZZ"; }
    void fillSeatBlockingRemList(TTlgParser &tlg);
    typedef std::vector<TPaxItem>::iterator PaxItemsIterator;
    bool parsePassengerIDs(std::string& paxLevelElement, std::set<PaxItemsIterator>& applicablePaxItems);
    void bindSystemIds();
};

class TPnrAddrItem
{
  public:
    char airline[4];
    char addr[21];
    TPnrAddrItem()
    {
      *airline=0;
      *addr=0;
    }
};

class TTransferRoute : public std::vector<TTransferItem>
{
  private:
    void getById(int id, bool isPnrId);
  public:
    void getByPaxId(int pax_id);
    void getByPnrId(int pnr_id);
};

class TPnrItem
{
  public:
    char grp_ref[3];
    long grp_seats;
    std::vector<TPnrAddrItem> addrs;
    std::string grp_name;
    char status[4];
    std::string priority;
    std::vector<TNameElement> ne;
    std::vector<TTransferItem> transfer;
    TSegmentItem market_flt;
    std::map<TIndicator, TSeatsBlockingList> seatsBlocking;
    TPnrItem()
    {
      *grp_ref=0;
      grp_seats=0;
      *status=0;
    }
    void Clear()
    {
      *grp_ref=0;
      grp_seats=0;
      addrs.clear();
      grp_name.clear();
      *status=0;
      priority.clear();
      ne.clear();
      transfer.clear();
      market_flt.Clear();
    }

    void separateSeatsBlocking();
    void bindSeatsBlocking();
    bool seatIsUsed(const TSeat& seat) const;

    enum IndicatorsPresence
    {
      WithoutDEL,
      OnlyDEL,
      Various
    };

    IndicatorsPresence getIndicatorsPresence() const
    {
      std::set<TIndicator> indicators;
      for(const TNameElement& elem : ne)
        indicators.insert(elem.indicator);
      if (indicators.find(DEL)==indicators.end()) return WithoutDEL;
      if (indicators.size()==1) return OnlyDEL;
      return Various;
    }
};

class TTotalsByDest
{
  public:
    char dest[4];
    long seats;
    char subcl[2];
    long pad;
    ASTRA::TClass cl;
    std::vector<TPnrItem> pnr;
    TTotalsByDest()
    {
      *dest=0;
      seats=0;
      *subcl=0;
      pad=0;
      cl=ASTRA::NoClass;
    };
};

class TPNLADLPRLContent
{
  public:
    TFltInfo flt;
    std::vector<TRbdItem> rbd;
    std::vector<TRouteItem> cfg,avail,transit;
    std::vector<TTotalsByDest> resa;
    void Clear()
    {
      flt.Clear();
      rbd.clear();
      cfg.clear();
      avail.clear();
      transit.clear();
      resa.clear();
    };
};

class TPtmTransferData : public TBagItem
{
  public:
    long seats;
    std::string surname;
    std::vector<std::string> name;
    TPtmTransferData()
    {
      seats=0;
    };
};

class TPtmOutFltInfo : public TSegmentItem
{
  public:
    std::vector<TPtmTransferData> data;
    TPtmOutFltInfo() : TSegmentItem() {};
};

class TPtmContent
{
  public:
    TSegmentItem InFlt;
    std::vector<TPtmOutFltInfo> OutFlt;
    void Clear()
    {
      InFlt.Clear();
      OutFlt.clear();
    };
};

class TSeatsByDest
{
  public:
    char airp_arv[4];
    TSeatRanges ranges;
    TSeatsByDest()
    {
      Clear();
    };
    void Clear()
    {
      *airp_arv=0;
      ranges.clear();
    };
};

class TSOMContent
{
  public:
    TFltInfo flt;
    std::vector<TSeatsByDest> seats;
    void Clear()
    {
      flt.Clear();
      seats.clear();
    };
};

class TBtmPaxItem
{
  public:
    std::string surname;
    std::vector<std::string> name;
};

class TBtmGrpItem : public TBagItem
{
  public:
    std::vector<TBSMTagItem> tags;
    std::vector<TBtmPaxItem> pax;
    std::vector<TSegmentItem> OnwardFlt;
    std::set<std::string> excepts;
};

class TBtmOutFltInfo : public TSegmentItem
{
  public:
    std::vector<TBtmGrpItem> grp;
    TBtmOutFltInfo() : TSegmentItem() {};
};

class TBtmTransferInfo
{
  public:
    TSegmentItem InFlt;
    std::vector<TBtmOutFltInfo> OutFlt;
};

class TBtmContent
{
  public:
    std::vector<TBtmTransferInfo> Transfer;
    void Clear()
    {
      Transfer.clear();
    };
};

#define MAX_LEXEME_SIZE 100

class TTlgParser
{
  public:
    char lex[MAX_LEXEME_SIZE+1];
    const char* NextLine(const char* p);
    const char* GetLexeme(const char* p);
    const char* GetSlashedLexeme(const char* p);
    const char* GetToEOLLexeme(const char* p);
    const char* GetWord(const char* p);
    const char* GetNameElement(const char* p, bool trimRight);
};

extern char lexh[];

int CalcEOLCount(const char* p);
char* GetAirline(char* airline, bool with_icao=true, bool throwIfUnknown=true);
char GetSuffix(char &suffix, bool throwIfUnknown=true);
const char* GetTlgElementName(TTlgElement e);
TTlgCategory GetTlgCategory(char *tlg_type);
void GetParts(const char* tlg_p, TTlgPartsText &text, THeadingInfo* &info, TFlightsForBind &flts, TMemoryManager &mem);
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo* &info, TFlightsForBind &flts, TMemoryManager &mem);
void ParseEnding(TTlgPartInfo ending, THeadingInfo *headingInfo, TEndingInfo* &info, TMemoryManager &mem);
void ParsePNLADLPRLContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPNLADLPRLContent& con);
void ParsePTMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPtmContent& con);
void ParseBTMContent(TTlgPartInfo body, const TBSMHeadingInfo& info, TBtmContent& con, TMemoryManager &mem);
void ParseSOMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TSOMContent& con);

bool SavePNLADLPRLContent(int tlg_id, TDCSHeadingInfo& info, TPNLADLPRLContent& con, bool forcibly);
void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con);
void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, const TBtmContent& con);
void SaveSOMContent(int tlg_id, TDCSHeadingInfo& info, TSOMContent& con);

void ParseAHMFltInfo(TTlgPartInfo body, const TAHMHeadingInfo &info, TFltInfo& flt, TBindType &bind_type);
int SaveFlt(int tlg_id, const TFltInfo& flt, TBindType bind_type, TSearchFltInfoPtr search_params, ETlgErrorType error_type=tlgeNotError);

void ParseSeatRange(std::string str, TSeatRanges &ranges, bool usePriorContext);

void TestBSMElemOrder(const std::string &s);

void NormalizeFltInfo(TFltInfo &flt);

struct TFlightIdentifier {
    std::string airline;
    int flt_no;
    char suffix;
    TDateTime date;
    void parse(const char *val);
    void dump();
    TFlightIdentifier(): flt_no(ASTRA::NoExists), suffix(0), date(ASTRA::NoExists) {};
};

TTlgPartInfo nextPart(const TTlgPartInfo &curr, const char* line_p);
void throwTlgError(const char* msg, const TTlgPartInfo &curr, const char* line_p);
void split(std::vector<std::string> &result, const std::string val, char c);
int monthAsNum(const std::string &smonth);

// на входе строка формата nn(aaa(nn))
TDateTime ParseDate(const std::string &buf);
TDateTime ParseDate(int day);

namespace regex {
    static const std::string  m = "[А-ЯЁA-Z0-9]";
    static const std::string a = "[А-ЯЁA-Z]";

    static const std::string airline = "(" + m + "{2}" + a + "?)";
    static const std::string airp = "(" + a + "{3})";
    static const std::string flt_no = "(\\d{1,4})(" + a + "?)";
    static const std::string date = "(\\d{2})";
    static const std::string date_month = "(\\d{2}" + a + "{3})";
    static const std::string date_month_year = "(\\d{2}" + a + "{3}\\d{2})";
    static const std::string bort = "(" + m + "{2,10})";
    static const std::string aircraft_vers = "(" + m + "{1,12})";
    static const std::string full_stop = "\\.";
}


} //namespace TypeB

class PaxASVCCallbacks
{
    public:
        virtual ~PaxASVCCallbacks() {}
        virtual void onSyncPaxASVC(TRACE_SIGNATURE, int pax_id) = 0;

};

#endif

