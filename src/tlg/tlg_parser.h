#ifndef _TLG_PARSER_H_
#define _TLG_PARSER_H_

#include <string>
#include <vector>
#include "astra_consts.h"
#include "astra_misc.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "seats.h"
#include "memory_manager.h"
#include "tlg_binding.h"

namespace TypeB
{

class ETlgError:public EXCEPTIONS::Exception
{
  public:
    ETlgError(const char *format, ...):EXCEPTIONS::Exception("")
    {
      va_list ap;
      va_start(ap, format);
      vsnprintf(Message, sizeof(Message), format, ap);
      Message[sizeof(Message)-1]=0;
      va_end(ap);
    };

    ETlgError(std::string msg):EXCEPTIONS::Exception(msg) {};
};

enum TTlgCategory{tcUnknown,tcDCS,tcBSM,tcAHM,tcSSM,tcASM};

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
               //общие
               EndOfMessage};

extern const char* TTlgElementS[];

enum TIndicator{None,ADD,CHG,DEL};

enum TElemPresence{epMandatory,epOptional,epNone};

class TTlgPartInfo
{
  public:
    char *p;
    int line;
    TTlgPartInfo()
    {
      p=NULL;
      line=0;
    };
};

struct TTlgParts
{
  TTlgPartInfo addr,heading,body,ending;
};

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

class TDocItem
{
  public:
    char rem_code[6],rem_status[3],type[3],issue_country[4],no[16];
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
      *rem_code=0;
      *rem_status=0;
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
      return  *type==0 &&
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

class TDocoItem
{
  public:
    char rem_code[6],rem_status[3],type[3],no[16],applic_country[4];
    BASIC::TDateTime issue_date;
    std::string birth_place, issue_place;
    bool pr_inf;
    TDocoItem()
    {
      Clear();
    };
    void Clear()
    {
      *rem_code=0;
      *rem_status=0;
      *type=0;
      *no=0;
      *applic_country=0;
      issue_date=ASTRA::NoExists;
      birth_place.clear();
      issue_place.clear();
      pr_inf=false;
    };
    bool Empty() const
    {
      return  *type==0 &&
              *no==0 &&
              *applic_country==0 &&
              issue_date==ASTRA::NoExists &&
              birth_place.empty() &&
              issue_place.empty() &&
              pr_inf==false;
    };
};

class TTKNItem
{
  public:
    char rem_code[6],rem_status[3],ticket_no[16];
    int coupon_no;
    bool pr_inf;
    TTKNItem()
    {
      Clear();
    };
    void Clear()
    {
      *rem_code=0;
      *rem_status=0;
      *ticket_no=0;
      coupon_no=0;
      pr_inf=false;
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
    std::vector<TTKNItem> tkn;
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
    std::vector<TDocoItem> doco;
    std::vector<TTKNItem> tkn;
    std::vector<TFQTItem> fqt;
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
    long local_time;
    char subcl[2];
    TSegmentItem() : TFltInfo()
    {
      Clear();
    };
    void Clear()
    {
      TFltInfo::Clear();
      local_date=0;
      local_time=0;
      *subcl=0;
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

class TPtmOutFltInfo : public TTransferItem
{
  public:
    std::vector<TPtmTransferData> data;
    TPtmOutFltInfo() : TTransferItem() {};
};

class TPtmContent
{
  public:
    TTransferItem InFlt;
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
};

class TBtmOutFltInfo : public TTransferItem
{
  public:
    std::vector<TBtmGrpItem> grp;
    TBtmOutFltInfo() : TTransferItem() {};
};

class TBtmTransferInfo
{
  public:
    TTransferItem InFlt;
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

#define MAX_LEXEME_SIZE 70

class TTlgParser
{
  public:
    char lex[MAX_LEXEME_SIZE+1];
    char* NextLine(char* p);
    char* GetLexeme(char* p);
    char* GetSlashedLexeme(char* p);
    char* GetToEOLLexeme(char* p);
    char* GetWord(char* p);
    char* GetNameElement(char* p, bool trimRight);
};

extern char lexh[];
extern const TMonthCode Months[];

char* TlgElemToElemId(TElemType type, const char* elem, char* id, bool with_icao=false);
char GetSuffix(char &suffix);
char* GetAirline(char* airline, bool with_icao=false);
char* GetTlgElementName(TTlgElement e);
TTlgCategory GetTlgCategory(char *tlg_type);
TTlgParts GetParts(char* tlg_p, TMemoryManager &mem);
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo* &info, TMemoryManager &mem);
void ParseEnding(TTlgPartInfo ending, THeadingInfo *headingInfo, TEndingInfo* &info, TMemoryManager &mem);
void ParsePNLADLPRLContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPNLADLPRLContent& con);
void ParsePTMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPtmContent& con);
void ParseBTMContent(TTlgPartInfo body, TBSMHeadingInfo& info, TBtmContent& con, TMemoryManager &mem);
void ParseSOMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TSOMContent& con);

bool SavePNLADLPRLContent(int tlg_id, TDCSHeadingInfo& info, TPNLADLPRLContent& con, bool forcibly);
void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con);
void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, TBtmContent& con);
void SaveSOMContent(int tlg_id, TDCSHeadingInfo& info, TSOMContent& con);

void ParseAHMFltInfo(TTlgPartInfo body, const TAHMHeadingInfo &info, TFltInfo& flt, TBindType &bind_type);
int SaveFlt(int tlg_id, TFltInfo& flt, TBindType bind_type);

void ParseSeatRange(std::string str, std::vector<TSeatRange> &ranges, bool usePriorContext);

}

#endif

