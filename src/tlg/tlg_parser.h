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

enum TTimeMode{tmLT, tmUTC, tmUnknown};
enum TTlgCategory{tcUnknown,tcDCS,tcBSM,tcAHM,tcSSM};

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
               Reject,
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

struct TMsgSeqRef {
    char day[3], month[4], grp[6], type, num[4], creator[36];
    TMsgSeqRef() { creator[0] = 0; }
};

class TSSMHeadingInfo : public THeadingInfo
{
    public:
        TTimeMode time_mode;
        TMsgSeqRef msr;

        void dump();

        TSSMHeadingInfo() : THeadingInfo(), time_mode(tmUnknown) {};
        TSSMHeadingInfo(THeadingInfo &info) : THeadingInfo(info), time_mode(tmUnknown)  {};
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
  ASTRA::TClass cl;
  char *rus, *lat;
} TSubclassCode;

typedef struct
{
  char *rus, *lat;
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
    bool Empty()
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

enum TActionIdentifier {
    aiNEW,
    aiCNL,
    aiRPL,
    aiSKD,
    aiACK,
    aiADM,
    aiCON,
    aiEQT,
    aiFLT,
    aiNAC,
    aiREV,
    aiRSD,
    aiTIM,
    aiUnknown
};

extern const char *TActionIdentifierS[];

struct TSSMDate {
    BASIC::TDateTime date;
    void parse(const char *val);
    TSSMDate(): date(ASTRA::NoExists) {};
};

enum TFrequencyRate {frW, frW2}; // Every week, Every two weeks (fortnightly)

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

struct TPeriodOfOper {
    private:
        void parse(bool pr_from, const char *val);
    public:
        TSSMDate from, to;
        char oper_days[8]; // Day(s) of operation
        TFrequencyRate rate;
        void parse(char *&ph, TTlgParser &tlg);
        TPeriodOfOper():
            rate(frW)
    {
        *oper_days = 0;
    }
};

struct TDEI {
    int id;
    TDEI(int aid) { id = aid; };
    virtual ~TDEI() {};
    virtual void parse(const char *val) = 0;
    virtual void dump() = 0;
    virtual bool empty() = 0;
};

struct TMealItem {
    std::string cls;
    std::string meal;
};

struct TDEI_7:TDEI {
    private:
        void insert(bool default_meal, std::string &meal);
    public:
        std::vector<TMealItem> meal_service;
        void parse(const char *val);
        void dump();
        bool empty() { return meal_service.empty(); };
        TDEI_7(): TDEI(7) {};
};

struct TDEI_6:TDEI {
    char airline[4];
    char suffix[2];
    int flt_no, layover;
    void parse(const char *val);
    void dump();
    bool empty() { return *airline == 0; };
    TDEI_6(): TDEI(6), flt_no(ASTRA::NoExists), layover(ASTRA::NoExists)
    {
        *airline = 0;
        *suffix = 0;
    };
};

struct TDEI_1:TDEI {
    std::vector<std::string> airlines;
    void parse(const char *val);
    void dump();
    bool empty() { return airlines.empty(); };
    TDEI_1(): TDEI(1) {};
};

struct TDEI_airline:TDEI {
    char airline[4];
    void parse(const char *val);
    void dump();
    bool empty() { return strlen(airline) == 0; };
    TDEI_airline(int aid): TDEI(aid) { *airline = 0; };
};

struct TDEIHolder:std::vector<TDEI *> {
    private:
        void parse(const char *val);
    public:
        void parse(TActionIdentifier ai, char *&ph, TTlgParser &tlg);
};

struct TSSMFltInfo: TFltInfo {
    void parse(const char *val);
};

struct TFlightInformation {
    TSSMFltInfo flt;
    TPeriodOfOper oper_period;  // Existing Period of Operation;
    TDEI_1 dei1;                // Joint Operation Airline Designators
    TDEI_airline dei2;          // Code Sharing - Commercial duplicate
    TDEI_airline dei3;          // Aircraft Owner
    TDEI_airline dei4;          // Cockpit Crew Employer
    TDEI_airline dei5;          // Cabin Crew Employer
    TDEI_airline dei9;          // Code Sharing - Shared Airline Designation or
                                // Wet Lease Airline Designation
    TDEIHolder dei_list;
    TFlightInformation():
        dei2(2),
        dei3(3),
        dei4(4),
        dei5(5),
        dei9(9)
    {
        dei_list.push_back(&dei1);
        dei_list.push_back(&dei2);
        dei_list.push_back(&dei3);
        dei_list.push_back(&dei4);
        dei_list.push_back(&dei5);
        dei_list.push_back(&dei9);
    }

};

struct TPeriodFrequency {
    TSSMDate effective_date, discontinue_date; //Schedule Validity Effective/Discontinue Dates
    TPeriodOfOper oper_period;
    TDEI_1 dei1;                // Joint Operation Airline Designators
    TDEI_airline dei2;          // Code Sharing - Commercial duplicate
    TDEI_airline dei3;          // Aircraft Owner
    TDEI_airline dei4;          // Cockpit Crew Employer
    TDEI_airline dei5;          // Cabin Crew Employer
    TDEI_6 dei6;                // Onward Flight
    TDEI_airline dei9;          // Code Sharing - Shared Airline Designation or
                                // Wet Lease Airline Designation
    TDEIHolder dei_list;
    TPeriodFrequency():
        dei2(2),
        dei3(3),
        dei4(4),
        dei5(5),
        dei9(9)
    {
        dei_list.push_back(&dei1);
        dei_list.push_back(&dei2);
        dei_list.push_back(&dei3);
        dei_list.push_back(&dei4);
        dei_list.push_back(&dei5);
        dei_list.push_back(&dei6);
        dei_list.push_back(&dei9);
    }
};

struct TEquipment {
    char service_type;
    char aircraft[4];
    std::string PRBD;   //Passenger Reservations Booking Designator
                        //or Aircraft Configuration/Version
    std::string PRBM;   //Passenger Reservations Booking Modifier
    std::string craft_cfg;
    TDEI_airline dei2;          // Code Sharing - Commercial duplicate
    TDEI_airline dei3;          // Aircraft Owner
    TDEI_airline dei4;          // Cockpit Crew Employer
    TDEI_airline dei5;          // Cabin Crew Employer
    TDEI_6 dei6;                // Onward Flight
    TDEI_airline dei9;          // Code Sharing - Shared Airline Designation or
                                // Wet Lease Airline Designation
    TDEIHolder dei_list;
    TEquipment():
        dei2(2),
        dei3(3),
        dei4(4),
        dei5(5),
        dei9(9)
    {
        dei_list.push_back(&dei2);
        dei_list.push_back(&dei3);
        dei_list.push_back(&dei4);
        dei_list.push_back(&dei5);
        dei_list.push_back(&dei6);
        dei_list.push_back(&dei9);
    }
};

struct TRouteStation {
    char airp[4];
    BASIC::TDateTime scd, pax_scd;
    int date_variation;
    void dump();
    void parse(const char *val);
    TRouteStation() {
        *airp = 0;
        scd = ASTRA::NoExists;
        pax_scd = ASTRA::NoExists;
        date_variation = 0;
    }
};

struct TRouting {
    std::vector<std::string> leg_airps;
    TRouteStation station_dep;
    TRouteStation station_arv;
    TDEI_1 dei1;                // Joint Operation Airline Designators
    TDEI_airline dei2;          // Code Sharing - Commercial duplicate
    TDEI_airline dei3;          // Aircraft Owner
    TDEI_airline dei4;          // Cockpit Crew Employer
    TDEI_airline dei5;          // Cabin Crew Employer
    TDEI_6 dei6;                // Onward Flight
    TDEI_7 dei7;                // Meal Service Note
    TDEI_airline dei9;          // Code Sharing - Shared Airline Designation or
    TDEIHolder dei_list;
    TRouting():
        dei2(2),
        dei3(3),
        dei4(4),
        dei5(5),
        dei9(9)
    {
        dei_list.push_back(&dei1);
        dei_list.push_back(&dei2);
        dei_list.push_back(&dei3);
        dei_list.push_back(&dei4);
        dei_list.push_back(&dei5);
        dei_list.push_back(&dei6);
        dei_list.push_back(&dei7);
        dei_list.push_back(&dei9);
    }
    void parse_leg_airps(std::string buf);
};

struct TDEI_8:TDEI {
    char TRC;
    int DEI;
    std::string data;
    void parse(const char *val);
    void dump() {};
    bool empty() { return true; };
    TDEI_8(): TDEI(8), TRC(0), DEI(ASTRA::NoExists) {};
};

struct TOther {
    int DEI;
    std::string data;
    void parse(const char *val);
    TOther():DEI(ASTRA::NoExists) {};
};

struct TSegment {
    std::string airp_dep, airp_arv;
    TDEI_8 dei8; // Traffic Restriction Note
    TOther other;
    void parse(const char *val);
};

class TSSMContent
{
    public:
        TActionIdentifier action_identifier;
        bool xasm;
        TFlightInformation flt_info;
        TPeriodFrequency period_frequency;
        TSSMFltInfo new_flt; // for FLT message only
        TEquipment equipment;
        TRouting routing; // Routing or Leg Information
        std::vector<TSegment> segs;
        void Clear() {};
        void dump();
        TSSMContent(): action_identifier(aiUnknown), xasm(false) {}
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

TTlgCategory GetTlgCategory(char *tlg_type);
TTlgParts GetParts(char* tlg_p, TMemoryManager &mem);
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo* &info, TMemoryManager &mem);
void ParseEnding(TTlgPartInfo ending, THeadingInfo *headingInfo, TEndingInfo* &info, TMemoryManager &mem);
void ParsePNLADLPRLContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPNLADLPRLContent& con);
void ParsePTMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPtmContent& con);
void ParseBTMContent(TTlgPartInfo body, TBSMHeadingInfo& info, TBtmContent& con, TMemoryManager &mem);
void ParseSSMContent(TTlgPartInfo body, TSSMHeadingInfo& info, TSSMContent& con, TMemoryManager &mem);
void ParseSOMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TSOMContent& con);

bool ParseDOCSRem(TTlgParser &tlg,BASIC::TDateTime scd_local,std::string &rem_text,TDocItem &doc);

bool SavePNLADLPRLContent(int tlg_id, TDCSHeadingInfo& info, TPNLADLPRLContent& con, bool forcibly);
void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con);
void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, TBtmContent& con);
void SaveSSMContent(int tlg_id, TSSMHeadingInfo& info, TSSMContent& con);
void SaveSOMContent(int tlg_id, TDCSHeadingInfo& info, TSOMContent& con);

void ParseAHMFltInfo(TTlgPartInfo body, const TAHMHeadingInfo &info, TFltInfo& flt, TBindType &bind_type);
int SaveFlt(int tlg_id, TFltInfo& flt, TBindType bind_type);

void ParseSeatRange(std::string str, std::vector<TSeatRange> &ranges, bool usePriorContext);

int ssm(int argc,char **argv);

}

#endif

