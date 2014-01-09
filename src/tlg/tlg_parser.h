#ifndef _TLG_PARSER_H_
#define _TLG_PARSER_H_

#include <string>
#include <vector>
#include "astra_consts.h"
#include "astra_misc.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "seats_utils.h"
#include "memory_manager.h"
#include "flt_binding.h"

namespace TypeB
{

class ETlgError:public EXCEPTIONS::Exception
{
  private:
    int pos, len, line;
    std::string text;
  public:
    ETlgError(const char *format, ...):EXCEPTIONS::Exception(""),
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
                                       pos(ASTRA::NoExists),
                                       len(ASTRA::NoExists),
                                       line(ASTRA::NoExists)
    {}

    ETlgError(int p, int l, const std::string &t, int ln,
              const std::string &msg) :EXCEPTIONS::Exception(msg),
                                       pos(p),
                                       len(l),
                                       line(ln),
                                       text(t)
    {}

    ~ETlgError() throw(){}

    int error_pos() const { return pos; }
    int error_len() const { return len; }
    int error_line() const { return line; }
    std::string error_text() const { return text; }
};

enum TTlgCategory{tcUnknown,tcDCS,tcBSM,tcAHM,tcSSM,tcASM,tcLCI};

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

typedef std::list< std::pair<TFltInfo, TBindType> > TFlightsForBind;

class THeadingInfo
{
  public:
    char sender[8];
    char double_signature[3];
    std::string message_identity;
    BASIC::TDateTime time_create;
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
      association_number=0;
    };
    TDCSHeadingInfo(THeadingInfo &info) : THeadingInfo(info)
    {
      part_no=0;
      association_number=0;
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
  const char *rus, *lat;
} TMonthCode;

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
};

class TDocItem : public TDetailRemAncestor
{
  public:
    char type[3],issue_country[4],no[16];
    char nationality[4],gender[3];
    BASIC::TDateTime birth_date,expiry_date;
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
    BASIC::TDateTime issue_date;
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
    };
    void Clear()
    {
      TDetailRemAncestor::Clear();
      *ticket_no=0;
      coupon_no=0;
    };
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

class TRemItem
{
  public:
    std::string text;
    char code[6];
    TRemItem()
    {
      *code=0;
    };
};

typedef std::pair<std::string,std::string> TChdItem;

class TInfItem
{
  public:
    std::string surname,name;
    long age;
    std::vector<TRemItem> rem;
    std::vector<TDocItem> doc;
    std::vector<TDocoItem> doco;
    std::vector<TDocaItem> doca;
    std::vector<TTKNItem> tkn;
    std::vector<TCHKDItem> chkd;
    TInfItem()
    {
      Clear();
    };
    void Clear()
    {
      surname.clear();
      name.clear();
      age=0;
      doc.clear();
      tkn.clear();
    };
};

class TPaxItem
{
  public:
    std::string name;
    ASTRA::TPerson pers_type;
    long seats;
    std::vector<TSeatRange> seatRanges;
    TSeat seat; //это место, назначенное разборщиком на основе tlg_comp_layers
    char seat_rem[5];
    std::vector<TRemItem> rem;
    std::vector<TInfItem> inf;
    std::vector<TDocItem> doc;
    std::map<std::string/*no*/, TDocExtraItem> doc_extra;
    std::vector<TDocoItem> doco;
    std::vector<TDocaItem> doca;
    std::vector<TTKNItem> tkn;
    std::vector<TFQTItem> fqt;
    std::vector<TCHKDItem> chkd;
    TPaxItem()
    {
      pers_type=ASTRA::adult;
      seats=1;
      *seat_rem=0;
    };
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
    std::vector<TSeatRange> seatRanges;
    TBagItem bag;
    std::vector<TTagItem> tags;
    int bag_pool;
    TNameElement()
    {
      Clear();
    };
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
    };
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
    TPnrItem()
    {
      *grp_ref=0;
      grp_seats=0;
      *status=0;
    };
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
    };
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
    std::vector<TSeatRange> ranges;
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
extern const TMonthCode Months[];

int CalcEOLCount(const char* p);
char* TlgElemToElemId(TElemType type, const char* elem, char* id, bool with_icao=false);
ASTRA::TClass GetClass(const char* subcl);
char GetSuffix(char &suffix);
char* GetAirline(char* airline, bool with_icao=true);
const char* GetTlgElementName(TTlgElement e);
TTlgCategory GetTlgCategory(char *tlg_type);
void GetParts(const char* tlg_p, TTlgPartsText &text, TFlightsForBind &flts, TMemoryManager &mem);
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
int SaveFlt(int tlg_id, const TFltInfo& flt, TBindType bind_type, bool has_errors=false);

void ParseSeatRange(std::string str, std::vector<TSeatRange> &ranges, bool usePriorContext);

void TestBSMElemOrder(const std::string &s);

void NormalizeFltInfo(TFltInfo &flt);

struct TFlightIdentifier {
    std::string airline;
    int flt_no;
    char suffix;
    BASIC::TDateTime date;
    void parse(const char *val);
    void dump();
    TFlightIdentifier(): flt_no(ASTRA::NoExists), suffix(0), date(ASTRA::NoExists) {};
};

TTlgPartInfo nextPart(const TTlgPartInfo &curr, const char* line_p);
void throwTlgError(const char* msg, const TTlgPartInfo &curr, const char* line_p);

} //namespace TypeB

#endif

