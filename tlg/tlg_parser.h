#ifndef _TLG_PARSER_H_
#define _TLG_PARSER_H_

#include <string>
#include <vector>
#include "astra_consts.h"
#include "basic.h"
#include "exceptions.h"

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

typedef enum {tcUnknown,tcDCS,tcAHM} TTlgCategory;

typedef enum  {Address,
               CommunicationsReference,
               MessageIdentifier,
               FlightElement,
               AssociationNumber,
               BonusPrograms,
               Configuration,
               ClassCodes,
               SpaceAvailableElement,
               TranzitElement,
               TotalsByDestination,
               EndOfMessage} TPnlAdlElement;

extern const char* TPnlAdlElementS[12];

typedef enum {None,ADD,CHG,DEL} TIndicator;

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

class TFltInfo
{
  public:
    char airline[4];
    long flt_no;
    char suffix[2];
    BASIC::TDateTime scd;
    char brd_point[4];
    TFltInfo()
    {
      *airline=0;
      flt_no=0;
      *suffix=0;
      scd=0;
      *brd_point=0;
    };
    void Clear()
    {
      *airline=0;
      flt_no=0;
      *suffix=0;
      scd=0;
      *brd_point=0;
    };
};

class THeadingInfo
{
  public:
    char sender[8];
    char double_signature[3];
    std::string message_identity;
    BASIC::TDateTime time_create;
    char tlg_type[4];
    TFltInfo flt;
    long part_no;
    std::string merge_key; //[39]
    THeadingInfo()
    {
      *sender=0;
      *double_signature=0;
      time_create=0;
      *tlg_type=0;
      part_no=0;
    };
    void Clear()
    {
      *sender=0;
      *double_signature=0;
      message_identity.clear();
      time_create=0;
      *tlg_type=0;
      flt.Clear();
      part_no=0;
      merge_key.clear();
    };
};

class TEndingInfo
{
  public:
    char tlg_type[4];
    long part_no;
    bool pr_final_part;
    TEndingInfo()
    {
      *tlg_type=0;
      part_no=0;
      pr_final_part=true;
    };
    void Clear()
    {
      *tlg_type=0;
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

class TSeat
{
  public:
    long row;
    char line;
    char rem[5];
    TSeat()
    {
      row=0;
      line=0;
      *rem=0;
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

class TPaxItem
{
  public:
    std::string name;
    ASTRA::TPerson pers_type;
    long seats;
    TSeat seat;
    std::vector<TRemItem> rem;
    TPaxItem()
    {
      pers_type=ASTRA::adult;
      seats=1;
    };
};

class TInfItem
{
  public:
    std::string surname,name;
};

class TTransferItem
{
  public:
    long num;
    TFltInfo flt;
    long local_date;
    long local_time;
    char arv_point[4];
    char subcl[2];
    TTransferItem()
    {
      num=0;
      local_date=0;
      *arv_point=0;
      *subcl=0;
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
    std::vector<TInfItem> inf;
    TNameElement()
    {
      indicator=None;
      seats=0;
    };
    void Clear()
    {
      indicator=None;
      seats=0;
      surname.clear();
      pax.clear();
      rem.clear();
      inf.clear();
    };
};

class TPnrItem
{
  public:
    char grp_ref[3];
    long grp_seats;
    char pnr_ref[21];
    std::string grp_name;
    char wl_priority[7];
    std::vector<TNameElement> ne;
    std::vector<TTransferItem> transfer;
    TPnrItem()
    {
      *grp_ref=0;
      grp_seats=0;
      *pnr_ref=0;
      *wl_priority=0;
    };
    void Clear()
    {
      *grp_ref=0;
      grp_seats=0;
      *pnr_ref=0;
      grp_name.clear();
      *wl_priority=0;
      ne.clear();
      transfer.clear();
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

struct TPnlAdlContent
{
  std::vector<TRbdItem> rbd;
  std::vector<TRouteItem> cfg,avail,transit;
  std::vector<TTotalsByDest> resa;
};

class TTlgParser
{
  public:
    char lex[71];
    char* NextLine(char* p);
    char* GetLexeme(char* p);
    char* GetWord(char* p);
    char* GetNameElement(char* p);
};

TTlgCategory GetTlgCategory(char *tlg_type);
TTlgParts GetParts(char* tlg_p);
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo& info);
void ParseEnding(TTlgPartInfo ending, TEndingInfo& info);
void ParsePnlAdlBody(TTlgPartInfo body, THeadingInfo& info, TPnlAdlContent& con);
bool SavePnlAdlContent(int point_id, THeadingInfo& info, TPnlAdlContent& con, bool forcibly,
                       const char* OWN_CANON_NAME, const char* ERR_CANON_NAME);
void PasreAHMFltInfo(TTlgPartInfo body, THeadingInfo& info);                       

#endif

