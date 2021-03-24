#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "date_time.h"
#include "tlg_parser.h"
#include "lci_parser.h"
#include "ucm_parser.h"
#include "loadsheet_parser.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "oralib.h"
#include "misc.h"
#include "tlg.h"
#include "seats_utils.h"
#include "salons.h"
#include "memory_manager.h"
#include "comp_layers.h"
#include "flt_binding.h"
#include "rozysk.h"
#include "alarms.h"
#include "trip_tasks.h"
#include "remarks.h"
#include "etick.h"
#include "qrys.h"
#include "points.h"
#include "counters.h"
#include "pax_calc_data.h"
#include "typeb_db.h"
#include <boost/regex.hpp>

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

namespace TypeB
{

const char* TTlgElementS[] =
              {//��騥
               "Address",
               "CommunicationsReference",
               "MessageIdentifier",
               "FlightElement",
               //PNL � ADL
               "AssociationNumber",
               "BonusPrograms",
               "Configuration",
               "ClassCodes",
               "SpaceAvailableElement",
               "TranzitElement",
               "TotalsByDestination",
               //PTM
               "TransferPassengerData",
               //SOM
               "SeatingCategories",
               "SeatsByDestination",
               "SupplementaryInfo",
               //MVT
               "AircraftMovementInfo",
               //LDM
               "LoadInfoAndRemarks",
               //SSM
               "TimeModeElement",
               "MessageSequenceReference",
               "ActionIdentifier",
               "PeriodFrequency",
               "NewFlight",
               "Equipment",
               "Routing",
               "Segment",
               "SubSI",
               "SubSIMore",
               "SubSeparator",
               "SI",
               "SIMore",
               "Reject",
               "RejectBody",
               "RepeatOfRejected",
               //LCI
               "ActionCode",
               "LCIData",
               //��騥
               "EndOfMessage"};


char lexh[MAX_LEXEME_SIZE+1];

void ParseNameElement(const char* p, vector<string> &names, TElemPresence num_presence);
void ParsePaxLevelElement(TTlgParser &tlg, TFltInfo& flt, TPnrItem &pnr, bool &pr_prev_rem, bool &pr_bag_info);
void BindRemarks(TTlgParser &tlg, TNameElement &ne);
void ParseRemarks(const vector< pair<string,int> > &seat_rem_priority,
                  TTlgParser &tlg, const TDCSHeadingInfo &info, TPnrItem &pnr, TNameElement &ne);
bool ParseCHDRem(TTlgParser &tlg, const string &rem_text, vector<TChdItem> &chd);
bool ParseINFRem(TTlgParser &tlg, const string &rem_text, TInfList &inf);
bool ParseSEATRem(TTlgParser &tlg, const string &rem_text, TSeatRanges &seats);
bool ParseTKNRem(TTlgParser &tlg, const string &rem_text, TTKNItem &tkn);
bool ParseFQTRem(TTlgParser &tlg, const string &rem_text, TFQTItem &fqt);

void parseAndBindSystemId(TTlgParser &tlg, TNameElement &ne, const string &paxLevelElement);

const char* TTlgParser::NextLine(const char* p)
{
  if (p==NULL) return NULL;
  if ((p=strchr(p,'\n'))==NULL) return NULL;
  else return p+1;
};

const char* TTlgParser::GetLexeme(const char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(const unsigned char*)(p+len)>' ';len++);
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  if (len==0) return NULL;

  strncpy(lex,p,len);
  return p+len;
};

const char* TTlgParser::GetSlashedLexeme(const char* p)
{
  int len;
  const char *res;
  if (p==NULL) return NULL;
  //㤠��� �� �।�����騥 �஡���
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n'&&*(p+len)!='/';len++);
  //�������� ������ ���
  res=p+len;
  if (*res=='/') res++;
  else
    if (len==0) res=NULL;
  //㤠��� �� ��᫥���騥 �஡���
  for(len--;len>=0&&*(const unsigned char*)(p+len)<=' ';len--);
  len++;
  //�������� tlg.lex
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  return res;
};

const char* TTlgParser::GetWord(const char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p!=0&&!(IsUpperLetter(*p)||IsDigit(*p));p++);
  for(len=0;IsUpperLetter(*(p+len))||IsDigit(*(p+len));len++);
  if (len>(int)sizeof(lex)-1) len=sizeof(lex)-1;
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

const char* TTlgParser::GetNameElement(const char* p, bool trimRight) //�� ��ࢮ� �窨 �� ��ப�
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++)
    if (*(const unsigned char*)(p+len)<=' '&&*(p+len+1)=='.') break;
  if (trimRight)
    //㤠�塞 ����� ᨬ���� � ���� ���ᥬ�
    for(len--;len>=0&&*(const unsigned char*)(p+len)<=' ';len--);
  else
    //㤠�塞 ᨬ���� � ���� ���ᥬ� �஬� �஡����
    for(len--;len>=0&&*(const unsigned char*)(p+len)<' ';len--);

  len++;
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  if (len==0) return NULL;

  strncpy(lex,p,len);
  return p+len;
};

const char* TTlgParser::GetToEOLLexeme(const char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++);
  for(len--;len>=0&&*(const unsigned char*)(p+len)<=' ';len--);
  len++;
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  if (len==0) return NULL;

  strncpy(lex,p,len);
  return p+len;
};

const char* GetTlgElementName(TTlgElement e)
{
  return TTlgElementS[e];
};

int CalcEOLCount(const char* p)
{
  int i=0;
  if (p!=NULL)
  {
    while ((p=strchr(p,'\n'))!=NULL)
    {
      i++;
      p++;
    };
  };
  return i;
};

string TlgElemToElemId(TElemType type, const string &elem, bool with_icao = false)
{
    TElemFmt fmt;
    string id2;

    id2=ElemToElemId(type, elem, fmt, false);
    if (fmt==efmtUnknown)
        throw EBaseTableError("TlgElemToElemId: elem not found (type=%s, elem=%s)",
                EncodeElemType(type),elem.c_str());
    if (id2.empty())
        throw EBaseTableError("TlgElemToElemId: id is empty (type=%s, elem=%s)",
                EncodeElemType(type),elem.c_str());
    if (!with_icao && (fmt==efmtCodeICAONative || fmt==efmtCodeICAOInter))
        throw EBaseTableError("TlgElemToElemId: ICAO only elem found (type=%s, elem=%s)",
                EncodeElemType(type),elem.c_str());

    return id2;
}

char* TlgElemToElemId(TElemType type, const char* elem, char* id, bool with_icao=false)
{
  strcpy(id,TlgElemToElemId(type, elem, with_icao).c_str());
  return id;
};

string GetAirp(const string &airp, bool with_icao, bool throwIfUnknown)
{
    try
    {
        return TlgElemToElemId(etAirp,airp,with_icao);
    }
    catch (EBaseTableError)
    {
        if (throwIfUnknown)
            throw ETlgError("Unknown airport code '%s'", airp.c_str());
    };
    return airp;
};

string GetAirline(const string &airline, bool with_icao, bool throwIfUnknown)
{
    try
    {
        return TlgElemToElemId(etAirline,airline,with_icao);
    }
    catch (EBaseTableError)
    {
        if (throwIfUnknown)
            throw ETlgError("Unknown airline code '%s'", airline.c_str());
    };
    return airline;
};

string GetSuffix(const string &suffix, bool throwIfUnknown)
{
    string result;
    if (not suffix.empty())
    {
        try
        {
            result = TlgElemToElemId(etSuffix,suffix);
        }
        catch (EBaseTableError)
        {
            if (throwIfUnknown)
                throw ETlgError("Unknown flight number suffix '%s'", suffix.c_str());
        };
    };
    return result;
};

char* GetAirline(char* airline, bool with_icao, bool throwIfUnknown)
{
  strcpy(airline, GetAirline((string)airline, with_icao, throwIfUnknown).c_str());
  return airline;
};

char* GetAirp(char* airp, bool with_icao=false, bool throwIfUnknown=true)
{
  strcpy(airp, GetAirp((string)airp, with_icao, throwIfUnknown).c_str());
  return airp;
};

TClass GetClass(const char* subcl)
{
  try
  {
    char subclh[2];
    TlgElemToElemId(etSubcls,subcl,subclh);
    return DecodeClass(getBaseTable(etSubcls).get_row("code",subclh).AsString("cl").c_str());
  }
  catch (const EBaseTableError&)
  {
    throw ETlgError("Unknown subclass '%s'", subcl);
  };
};

char* GetSubcl(char* subcl, bool throwIfUnknown=true)
{
  try
  {
    return TlgElemToElemId(etSubcls,subcl,subcl);
  }
  catch (const EBaseTableError&)
  {
    if (throwIfUnknown)
      throw ETlgError("Unknown subclass '%s'", subcl);
  };
  return subcl;
};

char GetSuffix(char &suffix, bool throwIfUnknown)
{
    suffix = GetSuffix(suffix ? string(1, suffix) : string(), throwIfUnknown)[0];
    return suffix;
};

void GetPaxDocCountry(const char* elem, char* id)
{
  try
  {
    try
    {
      TlgElemToElemId(etPaxDocCountry,elem,id);
    }
    catch (const EBaseTableError&)
    {
      char country[3];
      TlgElemToElemId(etCountry,elem,country);
      //�饬 � pax_doc_countries
      strcpy(id,getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code").c_str());
    };
  }
  catch (const EBaseTableError&)
  {
    throw ETlgError("Unknown country code '%s'", elem);
  };
};

TTlgCategory GetTlgCategory(char *tlg_type)
{
  TTlgCategory cat;
  cat=tcUnknown;
  if (strcmp(tlg_type,"PNL")==0||
      strcmp(tlg_type,"ADL")==0||
      strcmp(tlg_type,"PTM")==0||
      strcmp(tlg_type,"SOM")==0||
      strcmp(tlg_type,"PRL")==0) cat=tcDCS;
  if (strcmp(tlg_type,"BTM")==0) cat=tcBSM;
  if (strcmp(tlg_type,"MVT")==0||
      strcmp(tlg_type,"UWS")==0) cat=tcAHM;
  if (strcmp(tlg_type,"SSM")==0) cat=tcSSM;
  if (strcmp(tlg_type,"ASM")==0) cat=tcASM;
  if (strcmp(tlg_type,"LCI")==0) cat=tcLCI;
  if (strcmp(tlg_type,"UCM")==0) cat=tcUCM;
  if (strcmp(tlg_type,"CPM")==0) cat=tcCPM;
  if (strcmp(tlg_type,"SLS")==0) cat=tcSLS;
  if (strcmp(tlg_type,"LDM")==0) cat=tcLDM;
  if (strcmp(tlg_type,"NTM")==0) cat=tcNTM;
  if (strcmp(tlg_type,"IFM")==0) cat=tcIFM;
  if (strcmp(tlg_type,"LDS")==0) cat=tcLOADSHEET;
  return cat;
};

class TBSMInfo
{
  public:
    virtual ~TBSMInfo(){};
};

class TBSMVersionInfo : public TBSMInfo
{
  public:
    char ver_no[2];
    char src_indicator[2];
    char airp[4];
    long part_no;
    std::string message_number;
    TBSMVersionInfo()
    {
      *ver_no=0;
      *src_indicator=0;
      *airp=0;
      part_no=0;
    };
};

class TBSMFltInfo : public TBSMInfo
{
  public:
    char airline[4];
    long flt_no;
    char suffix[2];
    TDateTime scd;
    char airp[4];
    char subcl[2];
    TBSMFltInfo()
    {
      *airline=0;
      flt_no=0;
      *suffix=0;
      scd=0;
      *airp=0;
      *subcl=0;
    };
};

class TBSMTag : public TBSMInfo
{
  public:
    char first_no[11];
    char num[4];
    TBSMTag()
    {
      *first_no=0;
      *num=0;
    };
};

class TBSMBag : public TBSMInfo
{
  public:
    char weight_unit[2];
    long bag_amount,bag_weight,rk_weight;
    TBSMBag()
    {
      bag_amount=0;
      bag_weight=0;
      rk_weight=0;
      *weight_unit=0;
    };
};

class TBSMPax : public TBSMInfo
{
  public:
    std::string surname;
    std::vector<std::string> name;
};

class TBSMBagException : public TBSMInfo
{
  public:
    char type[5];
    TBSMBagException()
    {
      *type=0;
    };
};

char ParseBSMElement(const char *p, TTlgParser &tlg, TBSMInfo* &data, TMemoryManager &mem)
{
  char c,id[2];
  int res;
  *id=0;
  data=NULL;
  try
  {
    p=tlg.GetSlashedLexeme(p);
    c=0;
    res=sscanf(tlg.lex,".%1[A-Z]%c",id,&c);
    if (c!=0||res!=1)
      throw ETlgError("Wrong element identifier");
    try
    {
      if (*id!='P')
      {
        //� tlg.lex ��ࢠ� ���᪬�
        if ((p=tlg.GetSlashedLexeme(p))==NULL) throw ETlgError("Wrong format");
      };
      switch (*id)
      {
        case 'V':
          {
            data = new TBSMVersionInfo;
            mem.create(data, STDLOG);
            TBSMVersionInfo& info = *(TBSMVersionInfo*)data;

            c=0;
            res=sscanf(tlg.lex,"%1[A-Z0-9]%1[LTXR]%3[A-Z�-��]%c",info.ver_no,info.src_indicator,info.airp,&c);
            if (c!=0||res!=3) throw ETlgError("Wrong format");
            if ((p=tlg.GetSlashedLexeme(p))!=NULL&&*tlg.lex!=0)
            {
              c=0;
              res=sscanf(tlg.lex,"PART%lu%c",&info.part_no,&c);
              if (c!=0||res!=1||
                  info.part_no<=0||info.part_no>=100) throw ETlgError("Wrong part number");
            }
            else info.part_no=1;
            if ((p=tlg.GetSlashedLexeme(p))!=NULL&&*tlg.lex!=0)
            {
              c=0;
              res=sscanf(tlg.lex,"%10[A-Z0-9]%c",lexh,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong message reference number");
              info.message_number=lexh;
            };
            break;
          }
        case 'I':
        case 'F':
        case 'O':
          {
            data = new TBSMFltInfo;
            mem.create(data, STDLOG);
            TBSMFltInfo& flt = *(TBSMFltInfo*)data;

            if (strlen(tlg.lex)<3||strlen(tlg.lex)>8) throw ETlgError("Wrong flight number");
            flt.suffix[0]=0;
            flt.suffix[1]=0;
            c=0;
            if (IsDigit(tlg.lex[2]))
              res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(tlg.lex,"%3[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong flight number");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong flight number");
            GetAirline(flt.airline);
            GetSuffix(flt.suffix[0]);

            if ((p=tlg.GetSlashedLexeme(p))==NULL) throw ETlgError("Flight date not found");
            long int day;
            char month[4];
            c=0;
            res=sscanf(tlg.lex,"%2lu%3[A-Z�-��]%c",&day,month,&c);
            if (c!=0||res!=2||day<=0) throw ETlgError("Wrong flight date");


            try
            {
              flt.scd = DayMonthToDate(day, monthAsNum(month), Now(), dateEverywhere);
            }
            catch(const EConvertError&)
            {
              throw ETlgError("Can't convert flight date");
            };

            if ((p=tlg.GetSlashedLexeme(p))==NULL) throw ETlgError("Airport code not found");
            c=0;
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",flt.airp,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong airport code");
            GetAirp(flt.airp);

            if ((p=tlg.GetSlashedLexeme(p))!=NULL)
            {
              c=0;
              res=sscanf(tlg.lex,"%1[A-Z�-��]%c",flt.subcl,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong class");
              GetSubcl(flt.subcl);
            };
            if (tlg.GetSlashedLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            break;
          }
        case 'N':
          {
            data = new TBSMTag;
            mem.create(data, STDLOG);
            TBSMTag& tag = *(TBSMTag*)data;

            c=0;
            res=sscanf(tlg.lex,"%10[0-9]%3[0-9]%c",tag.first_no,tag.num,&c);
            if (c!=0||res!=2||
                strlen(tag.first_no)!=10||strlen(tag.num)!=3) throw ETlgError("Wrong format");
            if (tlg.GetSlashedLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            break;
          }
        case 'W':
          {
            data = new TBSMBag;
            mem.create(data, STDLOG);
            TBSMBag& bag = *(TBSMBag*)data;

            if (strcmp(tlg.lex,"L")!=0&&
                strcmp(tlg.lex,"K")!=0&&
                strcmp(tlg.lex,"P")!=0) throw ETlgError("Wrong pieces/weight indicator");
            strcpy(bag.weight_unit,tlg.lex);

            if ((p=tlg.GetSlashedLexeme(p))!=NULL&&*tlg.lex!=0)
            {
              c=0;
              res=sscanf(tlg.lex,"%3lu%c",&bag.bag_amount,&c);
              if (c!=0||res!=1||bag.bag_amount<0) throw ETlgError("Wrong number of checked bags");
            };
            if ((p=tlg.GetSlashedLexeme(p))!=NULL&&*tlg.lex!=0)
            {
              c=0;
              res=sscanf(tlg.lex,"%4lu%c",&bag.bag_weight,&c);
              if (c!=0||res!=1||bag.bag_weight<0) throw ETlgError("Wrong checked weight");
            };
            if ((p=tlg.GetSlashedLexeme(p))!=NULL&&*tlg.lex!=0)
            {
              c=0;
              res=sscanf(tlg.lex,"%3lu%c",&bag.rk_weight,&c);
              if (c!=0||res!=1||bag.rk_weight<0) throw ETlgError("Wrong unchecked weight");
            };
            if (tlg.GetSlashedLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            break;
          }
        case 'P':
          {
            data = new TBSMPax;
            mem.create(data, STDLOG);
            TBSMPax& pax = *(TBSMPax*)data;

            vector<string> names;
            ParseNameElement(p,names,epOptional);
            if (names.empty()) throw ETlgError("Wrong format");
            for(vector<string>::iterator i=names.begin();i!=names.end();i++)
            {
              if (i==names.begin())
                pax.surname=*i;
              else
                pax.name.push_back(*i);
            };
            break;
          }
        case 'E':
          {
            data = new TBSMBagException;
            mem.create(data, STDLOG);
            TBSMBagException& exc = *(TBSMBagException*)data;

            c=0;
            res=sscanf(tlg.lex,"%4[A-Z�-��]%c",exc.type,&c);
            if (c!=0||res!=1||strlen(exc.type)<3) throw ETlgError("Wrong format");
            if (tlg.GetSlashedLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            break;
          }
        default: data=NULL;
      };
      return (*id);
    }
    catch (const ETlgError& E)
    {
      switch (*id)
      {
        case 'V': throw ETlgError("Version and Supplementary Data: %s",E.what());
        case 'I': throw ETlgError("Inbound Flight Information: %s",E.what());
        case 'F': throw ETlgError("Operational Outbound Flight Information: %s",E.what());
        case 'N': throw ETlgError("Baggage Tag Details: %s",E.what());
        case 'W': throw ETlgError("Pieces and Weight Dimensions and Type Data: %s",E.what());
        case 'O': throw ETlgError("Onward Flight Information: %s",E.what());
        case 'P': throw ETlgError("Passenger Name: %s",E.what());
        case 'E': throw ETlgError("Baggage Exception Data: %s",E.what());
        default: throw;
      };
    };
  }
  catch(...)
  {
    mem.destroy(data, STDLOG);
    if (data!=NULL) delete data;
    data=NULL;
    throw;
  };
};

TTlgPartInfo nextPart(const TTlgPartInfo &curr, const char* line_p)
{
  TTlgPartInfo next;
  next.p=line_p;
  if (line_p!=NULL)
  {
    int EOL_count=0;
    const char* p=curr.p;
    for(;*p!=0&&p!=line_p;p++)
      if (*p=='\n') EOL_count++;
    if (p!=line_p)
      throw Exception("nextPart: wrong param line_p");
    next.EOL_count=curr.EOL_count+EOL_count;
    next.offset=curr.offset+(line_p-curr.p);
  }
  else
  {
    next.EOL_count=curr.EOL_count+CalcEOLCount(curr.p);
    next.offset=curr.offset+strlen(curr.p);
  };
  return next;
};

void throwTlgError(const char* msg, const TTlgPartInfo &curr, const char* line_p)
{
  if (line_p==NULL)
    throw ETlgError(ASTRA::NoExists, ASTRA::NoExists, "", curr.EOL_count+CalcEOLCount(curr.p)+1, msg);

  int EOL_count=0;
  const char* p=curr.p;
  for(;*p!=0&&p!=line_p;p++)
    if (*p=='\n') EOL_count++;
  if (p!=line_p)
    throw Exception("throwTlgError: wrong param line_p");

  for(;*p!=0&&*p!='\n';p++);

  throw ETlgError(curr.offset+(line_p-curr.p), p-line_p, string(line_p, p-line_p), curr.EOL_count+EOL_count+1, msg);
};

TTlgPartInfo ParseBSMHeading(TTlgPartInfo heading, TBSMHeadingInfo &info, TMemoryManager &mem)
{
  char c;
  const char* line_p;
  TTlgParser tlg;

  line_p=heading.p;
  try
  {
    do
    {
      if (tlg.GetLexeme(line_p)==NULL) continue;

      TBSMInfo *data=NULL;
      c=ParseBSMElement(line_p,tlg,data,mem);
      try
      {
        if (c!='V') throw ETlgError("Version and supplementary data not found");
        TBSMVersionInfo &verInfo = *(dynamic_cast<TBSMVersionInfo*>(data));
        info.part_no=verInfo.part_no;
        strcpy(info.airp,verInfo.airp);
        info.reference_number=verInfo.message_number;
        mem.destroy(data, STDLOG);
        if (data!=NULL) delete data;
      }
      catch(...)
      {
        mem.destroy(data, STDLOG);
        if (data!=NULL) delete data;
        throw;
      };
      line_p=tlg.NextLine(line_p);
      return nextPart(heading, line_p);
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(const ETlgError& E)
  {
    throwTlgError(E.what(), heading, line_p);
  };

  return nextPart(heading, line_p);
};

class TBSMElemOrderRules : public map<char, string>
{
  private:
    set<string> combinations;
    void travel(string &s, int last_idx)
    {
      if (last_idx<=0)
      {
        if (check(s)) combinations.insert(s);
        return;
      };
      for(int i=last_idx; i>=0; i--)
      {
        char last_char=s[last_idx];
        s[last_idx]=s[i];
        s[i]=last_char;
        travel(s, last_idx-1);
        s[i]=s[last_idx];
        s[last_idx]=last_char;
      };
    };
  public:
    TBSMElemOrderRules()
    {
      insert(make_pair('V',""));
      insert(make_pair('I',"VPE"));
      insert(make_pair('F',"IPE"));
      insert(make_pair('N',"FNPE"));
      insert(make_pair('W',"N"));
      insert(make_pair('O',"NWO"));
      insert(make_pair('P',"NWOP"));
      insert(make_pair('E',"PE"));
    };
    bool check(const string &s) const
    {
      char prior='V';
      string::const_iterator i=s.begin();
      do
      {
        char c;
        if (i!=s.end()) c=*i; else c='F';
        map<char, string>::const_iterator rule=find(c);
        if (rule!=end())
        {
          if (strchr(rule->second.c_str(),prior)==NULL) return false;
          prior=c;
        };
        if (i==s.end()) break;
        ++i;
      }
      while(true);
      return true;
    };

    size_t calcCharCombinations(const string &s, bool print)
    {
      combinations.clear();
      string ss(s);
      travel(ss, ss.size()-1);
      if (print)
      {
        for(set<string>::const_iterator i=combinations.begin(); i!=combinations.end(); ++i)
          printf("%s\n", i->c_str());
      };
      return combinations.size();
    };
};

void TestBSMElemOrder(const string &s)
{
  TBSMElemOrderRules rules;
  printf("calcCharCombinations for %s\n", s.c_str());
  rules.calcCharCombinations(s, true);
};

void ParseBTMContent(TTlgPartInfo body, const TBSMHeadingInfo& info, TBtmContent& con, TMemoryManager &mem)
{
  vector<TBtmTransferInfo>::iterator iIn;
  vector<TBtmOutFltInfo>::iterator iOut;
  vector<TBtmGrpItem>::iterator iGrp;
  vector<TBSMTagItem>::iterator iTag;
  vector<string>::iterator i;

  con.Clear();

  char prior;
  const char* line_p;
  TTlgParser tlg;
  static TBSMElemOrderRules elemOrderRules;

  TBSMInfo *data=NULL;

  line_p=body.p;
  try
  {
    char airp[4]; //��ய��� � ���஬ �ந�室�� �࠭���
    strcpy(airp,info.airp);
    GetAirp(airp);
    prior='V';

    do
    {
      if (tlg.GetLexeme(line_p)==NULL) continue;

      char e=ParseBSMElement(line_p,tlg,data,mem);
      if (data==NULL) continue;
      try
      {
        TBSMElemOrderRules::const_iterator rule=elemOrderRules.find(e);
        if (rule!=elemOrderRules.end())
        {
          if (strchr(rule->second.c_str(),prior)==NULL)
            throw ETlgError("Wrong element order");
          switch (e)
          {
            case 'I':
              {
                TBSMFltInfo &flt = *(dynamic_cast<TBSMFltInfo*>(data));
                TBtmTransferInfo trfer;
                strcpy(trfer.InFlt.airline,flt.airline);
                trfer.InFlt.flt_no=flt.flt_no;
                strcpy(trfer.InFlt.suffix,flt.suffix);
                trfer.InFlt.scd=flt.scd;
                trfer.InFlt.pr_utc=false;
                strcpy(trfer.InFlt.airp_dep,flt.airp);
                strcpy(trfer.InFlt.airp_arv,airp);
                strcpy(trfer.InFlt.subcl,flt.subcl);
                for(iIn=con.Transfer.begin();iIn!=con.Transfer.end();iIn++)
                  if (strcmp(iIn->InFlt.airline,trfer.InFlt.airline)==0&&
                      iIn->InFlt.flt_no==trfer.InFlt.flt_no&&
                      strcmp(iIn->InFlt.suffix,trfer.InFlt.suffix)==0&&
                      iIn->InFlt.scd==trfer.InFlt.scd&&
                      iIn->InFlt.pr_utc==trfer.InFlt.pr_utc&&
                      strcmp(iIn->InFlt.airp_dep,trfer.InFlt.airp_dep)==0&&
                      strcmp(iIn->InFlt.airp_arv,trfer.InFlt.airp_arv)==0&&
                      strcmp(iIn->InFlt.subcl,trfer.InFlt.subcl)==0) break;

                if (iIn==con.Transfer.end())
                  iIn=con.Transfer.insert(con.Transfer.end(),trfer);
                break;
              }
            case 'F':
              {
                TBSMFltInfo &flt = *(dynamic_cast<TBSMFltInfo*>(data));
                TBtmOutFltInfo OutFlt;
                strcpy(OutFlt.airline,flt.airline);
                OutFlt.flt_no=flt.flt_no;
                strcpy(OutFlt.suffix,flt.suffix);
                OutFlt.scd=flt.scd;
                OutFlt.pr_utc=false;
                strcpy(OutFlt.airp_dep,airp);
                strcpy(OutFlt.airp_arv,flt.airp);
                strcpy(OutFlt.subcl,flt.subcl);
                for(iOut=iIn->OutFlt.begin();iOut!=iIn->OutFlt.end();iOut++)
                  if (strcmp(iOut->airline,OutFlt.airline)==0&&
                    iOut->flt_no==OutFlt.flt_no&&
                    strcmp(iOut->suffix,OutFlt.suffix)==0&&
                    iOut->scd==OutFlt.scd&&
                    iOut->pr_utc==OutFlt.pr_utc&&
                    strcmp(iOut->airp_dep,OutFlt.airp_dep)==0&&
                    strcmp(iOut->airp_arv,OutFlt.airp_arv)==0&&
                    strcmp(iOut->subcl,OutFlt.subcl)==0) break;

                if (iOut==iIn->OutFlt.end())
                  iOut=iIn->OutFlt.insert(iIn->OutFlt.end(),OutFlt);
                break;
              }
            case 'N':
              {
                TBSMTag &BSMTag = *(dynamic_cast<TBSMTag*>(data));
                TBSMTagItem tag;
                StrToFloat(BSMTag.first_no,tag.first_no);
                StrToInt(BSMTag.num,tag.num);
                if (prior!='N')
                {
                  TBtmGrpItem grp;
                  iGrp=iOut->grp.insert(iOut->grp.end(),grp);
                };
                iGrp->tags.push_back(tag);
                break;
              }
            case 'W':
              {
                TBSMBag &bag = *(dynamic_cast<TBSMBag*>(data));
                strcpy(iGrp->weight_unit,bag.weight_unit);
                iGrp->bag_amount=bag.bag_amount;
                iGrp->bag_weight=bag.bag_weight;
                iGrp->rk_weight=bag.rk_weight;
                break;
              }
            case 'O':
              {
                TBSMFltInfo &flt = *(dynamic_cast<TBSMFltInfo*>(data));
                TSegmentItem OnwardFlt;
                strcpy(OnwardFlt.airline,flt.airline);
                OnwardFlt.flt_no=flt.flt_no;
                strcpy(OnwardFlt.suffix,flt.suffix);
                OnwardFlt.scd=flt.scd;
                OnwardFlt.pr_utc=false;
                strcpy(OnwardFlt.airp_dep,"");
                strcpy(OnwardFlt.airp_arv,flt.airp);
                strcpy(OnwardFlt.subcl,flt.subcl);
                iGrp->OnwardFlt.push_back(OnwardFlt);
                break;
              }
            case 'P':
              {
                TBSMPax &BSMPax = *(dynamic_cast<TBSMPax*>(data));
                TBtmPaxItem pax;
                pax.surname=BSMPax.surname;
                pax.name=BSMPax.name;
                iGrp->pax.push_back(pax);
                break;
              }
            case 'E':
              {
                TBSMBagException &BSMBagException = *(dynamic_cast<TBSMBagException*>(data));
                if (*BSMBagException.type!=0)
                  iGrp->excepts.insert(string(BSMBagException.type));
                break;
              }
            default: ;
          };
        };
        mem.destroy(data, STDLOG);
        if (data!=NULL) delete data;
        prior=e;
      }
      catch(...)
      {
        mem.destroy(data, STDLOG);
        if (data!=NULL) delete data;
        throw;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);

    TBSMElemOrderRules::const_iterator rule=elemOrderRules.find('F');
    if (rule!=elemOrderRules.end())
    {
      if (strchr(rule->second.c_str(),prior)==NULL)
        throw ETlgError("Wrong final element");
    };
  }
  catch (const ETlgError& E)
  {
    throwTlgError(E.what(), body, line_p);
  };
  return;
};

TTlgPartInfo ParseDCSHeading(TTlgPartInfo heading, TDCSHeadingInfo &info, TFlightsForBind &flts)
{
  int res;
  char c;
  const char *p, *line_p;
  TTlgParser tlg;
  TTlgElement e;
  TTlgPartInfo next;

  line_p=heading.p;
  try
  {
    e=FlightElement;
    do
    {
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case FlightElement:
          {
            //message flight
            long int day;
            char month[4],flt[9];
            c=0;
            res=sscanf(tlg.lex,"%8[A-Z�-��0-9]/%2lu%3[A-Z�-��]%c",flt,&day,month,&c);
            if (c!=0||res!=3||day<=0) throw ETlgError("Wrong flight/local date");
            if (strlen(flt)<3) throw ETlgError("Wrong flight");
            info.flt.suffix[0]=0;
            info.flt.suffix[1]=0;
            c=0;
            if (IsDigit(flt[2]))
              res=sscanf(flt,"%2[A-Z�-��0-9]%5lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            else
              res=sscanf(flt,"%3[A-Z�-��0-9]%5lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            if (c!=0||res<2||info.flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !IsUpperLetter(info.flt.suffix[0])) throw ETlgError("Wrong flight");

            try
            {
              info.flt.scd = DayMonthToDate(day, monthAsNum(month), Now(), dateEverywhere);
              info.flt.pr_utc=false;
            }
            catch(const EConvertError&)
            {
              throw ETlgError("Can't convert local date");
            };

            if (strcmp(info.tlg_type,"PNL")==0||
                strcmp(info.tlg_type,"ADL")==0||
                strcmp(info.tlg_type,"SOM")==0||
                strcmp(info.tlg_type,"PRL")==0)
            {
              if ((p=tlg.GetLexeme(p))!=NULL)
              {
                c=0;
                res=sscanf(tlg.lex,"%3[A-Z�-��]%c",info.flt.airp_dep,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong boarding point");
              }
              else throw ETlgError("Wrong boarding point");
              flts.push_back(TFltForBind(info.flt,  btFirstSeg, TSearchFltInfoPtr()));
            };
            if (strcmp(info.tlg_type,"PTM")==0)
            {
              if ((p=tlg.GetLexeme(p))!=NULL)
              {
                c=0;
                res=sscanf(tlg.lex,"%3[A-Z�-��]%3[A-Z�-��]%c",info.flt.airp_dep,info.flt.airp_arv,&c);
                if (c!=0||res!=2) throw ETlgError("Wrong boarding/transfer point");
              }
              else throw ETlgError("Wrong bording/transfer point");
              flts.push_back(TFltForBind(info.flt,  btLastSeg, TSearchFltInfoPtr()));
            };

            if ((p=tlg.GetLexeme(p))!=NULL)
            {
              c=0;
              res=sscanf(tlg.lex,"PART%lu%c",&info.part_no,&c);
              if (c!=0||res!=1||
                  info.part_no<=0||info.part_no>=1000) throw ETlgError("Wrong part number");
            }
            else throw ETlgError("Wrong part number");
            if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            e=AssociationNumber;
            break;
          };
        case AssociationNumber:
          {
            if (strncmp(tlg.lex,"ANA/",4)==0)
            {
              char assoc_number_str[7];
              *assoc_number_str=0;
              int i=NoExists;
              c=0;
              res=sscanf(tlg.lex,"ANA/%6[0-9]%c",assoc_number_str,&c);
              if (c!=0||res!=1||
                  strlen(assoc_number_str)<3||
                  StrToInt(assoc_number_str, i)==EOF)
                throw ETlgError("Wrong association number");
              info.association_number=i;
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              line_p=tlg.NextLine(line_p);
            };
            return nextPart(heading, line_p);
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(const ETlgError& E)
  {
    throwTlgError(E.what(), heading, line_p);
  };
  return nextPart(heading, line_p);
};

TTlgPartInfo ParseAHMHeading(TTlgPartInfo heading, TAHMHeadingInfo &info)
{
  TTlgPartInfo next;
  next=heading;
  return next;
};

void ParseAHMFltInfo(TTlgPartInfo body, const TAHMHeadingInfo &info, TFltInfo& flt, TBindType &bind_type)
{
  flt.Clear();
  bind_type=btFirstSeg;

  int res;
  char c;
  const char* line_p;
  TTlgParser tlg;
  TTlgElement e;

  line_p=body.p;
  try
  {
    e=FlightElement;
    do
    {
      tlg.GetLexeme(line_p);
      switch (e)
      {
        case FlightElement:
          {
            //message flight
            long int day=0;
            char trip[9];
            res=sscanf(tlg.lex,"%11[A-Z�-��0-9/].%s",lexh,tlg.lex);
            if (res!=2) throw ETlgError("Wrong flight identifier");

            {
                string src = tlg.lex;
                static const boost::regex e_airp("^" + regex::airp + "$");
                boost::match_results<std::string::const_iterator> results;
                if(boost::regex_match(src, results, e_airp)) {
                    strcpy(flt.airp_dep, tlg.lex);
                    GetAirp(flt.airp_dep);
                }
            }

            c=0;
            res=sscanf(lexh,"%8[A-Z�-��0-9]%c",trip,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(lexh,"%8[A-Z�-��0-9]/%2lu%c",trip,&day,&c);
              if (c!=0||res!=2||day<=0) throw ETlgError("Wrong flight/UTC date");
            };
            if (strlen(trip)<3) throw ETlgError("Wrong flight");
            flt.suffix[0]=0;
            flt.suffix[1]=0;
            c=0;
            if (IsDigit(trip[2]))
              res=sscanf(trip,"%2[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(trip,"%3[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong flight");
            GetAirline(flt.airline);
            GetSuffix(flt.suffix[0]);
            flt.scd = ParseDate(day);
            flt.pr_utc=true;

            if (strcmp(info.tlg_type,"MVT")==0)
            {
              c=0;
              res=sscanf(tlg.lex,"%[^.].%3[A-Z�-��]%c",lexh,flt.airp_dep,&c);
              if (c!=0||res!=2) throw ETlgError("Wrong airport of movement");
              GetAirp(flt.airp_dep,true);
              e=AircraftMovementInfo;
              break;
            };
            if (strcmp(info.tlg_type,"LDM")==0)
            {
              e=LoadInfoAndRemarks;
              break;
            };
            return;
          };
        case AircraftMovementInfo:
          {
            if (strncmp(tlg.lex,"AA",2)==0||
                strncmp(tlg.lex,"EA",2)==0)
            {
              strcpy(flt.airp_arv, flt.airp_dep);
              *flt.airp_dep=0;
              bind_type=btLastSeg;
              return;
            };
            if (strncmp(tlg.lex,"AD",2)==0||
                strncmp(tlg.lex,"ED",2)==0||
                strncmp(tlg.lex,"FR",2)==0)
            {
              bind_type=btFirstSeg;
              return;
            };
            throw ETlgError("Wrong movement information identifier");
          };
        case LoadInfoAndRemarks:
          {
            c=0;
            res=sscanf(tlg.lex,"-%3[A-Z�-��]%c",flt.airp_arv,&c);
            if (res!=2||c!='.') throw ETlgError("Wrong destination");
            GetAirp(flt.airp_arv,true);
            bind_type=btLastSeg;
            return;
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
    switch (e)
    {
      case FlightElement:        throw ETlgError("Flight identifier not found");
      case AircraftMovementInfo: throw ETlgError("Movement information identifier not found");
      case LoadInfoAndRemarks:   throw ETlgError("Destination not found");
      default:;
    };
  }
  catch(const ETlgError& E)
  {
    //�뢥�� �訡��+����� ��ப�
    throwTlgError(E.what(), body, line_p);
  };
  return;
};

//�����頥� TTlgPartInfo ᫥���饩 ��� (body)
TTlgPartInfo ParseHeading(TTlgPartInfo heading,
                          THeadingInfo* &info,
                          TFlightsForBind &flts,
                          TMemoryManager &mem)
{
  if (info!=NULL)
  {
    mem.destroy(info, STDLOG);
    delete info;
    info = NULL;
  };

  flts.clear();

  int res;
  char c;
  const char *p, *line_p, *ph;
  TTlgParser tlg;
  TTlgElement e;
  TTlgPartInfo next;
  THeadingInfo infoh;

  line_p=heading.p;
  try
  {
    e=CommunicationsReference;
    do
    {
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case CommunicationsReference:
          c=0;
          res=sscanf(tlg.lex,".%7[A-Z�-��0-9]%c",infoh.sender,&c);
          if (c!=0||res!=1||strlen(infoh.sender)!=7) throw ETlgError("Wrong address");
          if ((ph=tlg.GetLexeme(p))!=NULL)
          {
            c=0;
            res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%c%s",infoh.double_signature,&c,tlg.lex);
            if (res<2||c!='/'||strlen(infoh.double_signature)!=2)
            {
              *infoh.double_signature=0;
              p=tlg.GetLexeme(p);
            }
            else
            {
              if (res==2) *tlg.lex=0;
              p=ph;
            };
            while (p!=NULL)
            {
              if (infoh.message_identity.empty())
                infoh.message_identity=tlg.lex;
              else
              {
                infoh.message_identity+=' ';
                infoh.message_identity+=tlg.lex;
              };
              if (infoh.time_create==0)
              {
                try
                {
                  //���஡㥬 ࠧ����� date/time group
                  long int day,hour,min,sec=0;
                  res=sscanf(tlg.lex,"%2lu%2lu%2lu%s",&day,&hour,&min,tlg.lex);
                  if (res<3||day<0||hour<0||min<0) throw ETlgError("Wrong creation time");
                  else
                  {
                    if (res!=3)
                    {
                      c=0;
                      res=sscanf(tlg.lex,"%2lu%c",&sec,&c);
                      if (c!=0||res!=1||sec<0) throw ETlgError("Wrong creation time");
                    };
                  };
                  TDateTime day_create,time_create;
                  int year,mon,currday;
                  DecodeDate(NowUTC()+1,year,mon,currday); //�.�. ࠧ����� ��⥬��� �६�� � �ନ஢�⥫� � �ਥ�騪�, ���⮬� +1!
                  if (currday<day)
                  {
                    if (mon==1)
                    {
                      mon=12;
                      year--;
                    }
                    else mon--;
                  };
                  try
                  {
                    EncodeDate(year,mon,day,day_create);
                    EncodeTime(hour,min,sec,time_create);
                    infoh.time_create=day_create+time_create;
                  }
                  catch(const EConvertError&)
                  {
                    throw ETlgError("Can't convert creation time");
                  };
                }
                catch(const ETlgError&) {};
              };
              p=tlg.GetLexeme(p);
            };
          };
          e=MessageIdentifier;
          break;
        case MessageIdentifier:
          //message identifier
          if((string)tlg.lex == "LOADSHEET") {
              strcpy(infoh.tlg_type, "LDS");
              tlg.GetToEOLLexeme(line_p);
              infoh.MsgId = tlg.lex;
          } else {
              c=0;
              res=sscanf(tlg.lex,"%9[A-Z]%c",infoh.tlg_type,&c); // LOADSHEET
              if (c!=0||res!=1) {
                  c=0;
                  res=sscanf(tlg.lex,"%3[A-Z]%c",infoh.tlg_type,&c);
                  if (c!=0||res!=1||tlg.GetLexeme(p)!=NULL)
                  {
                      *(infoh.tlg_type)=0;
                  }
              }
          }

          if(infoh.tlg_type)
          {
            line_p=tlg.NextLine(line_p); //�஢���� line_p=NULL
          };

          heading=nextPart(heading, line_p);

          LogTrace(TRACE5) << "tlg_type: " << infoh.tlg_type;
          infoh.tlg_cat=GetTlgCategory(infoh.tlg_type);
          LogTrace(TRACE5) << "tlg_cat: " << infoh.tlg_cat;

          switch (infoh.tlg_cat)
          {
            case tcDCS:
              info = new TDCSHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseDCSHeading(heading,*(TDCSHeadingInfo*)info,flts);
              break;
            case tcBSM:
              info = new TBSMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseBSMHeading(heading,*(TBSMHeadingInfo*)info,mem);
              break;
            case tcAHM:
              info = new TAHMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseAHMHeading(heading,*(TAHMHeadingInfo*)info);
              break;
            case tcLCI:
              info = new TLCIHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseLCIHeading(heading,*(TLCIHeadingInfo*)info,flts);
              break;
            case tcUCM:
            case tcCPM:
            case tcSLS:
            case tcLDM:
            case tcNTM:
              info = new TUCMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseUCMHeading(heading,*(TUCMHeadingInfo*)info,flts);
              break;
            case tcLOADSHEET:
              info = new TUCMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseLOADSHEETHeading(heading,*(TUCMHeadingInfo*)info);
              break;
            default:
              info = new THeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=heading;
              break;
          };
          return next;
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
    if (info==NULL)
    {
      info = new THeadingInfo(infoh);
      mem.create(info, STDLOG);
    };
  }
  catch(const ETlgError& E)
  {
    mem.destroy(info, STDLOG);
    if (info!=NULL) delete info;
    info=NULL;
    if (E.error_line()==NoExists)
      throwTlgError(E.what(), heading, line_p);
    else
      throw;
  }
  catch(...)
  {
    mem.destroy(info, STDLOG);
    if (info!=NULL) delete info;
    info=NULL;
    throw;
  };
  return nextPart(heading, line_p);
};


void ParseDCSEnding(TTlgPartInfo ending, THeadingInfo &headingInfo, TEndingInfo &info)
{
  int res;
  char c;
  const char* p;
  TTlgParser tlg;
  char endtlg[13];

  try
  {
    if ((p=tlg.GetLexeme(ending.p))!=NULL)
    {
      sprintf(endtlg,"END%s",headingInfo.tlg_type);
      if (strcmp(tlg.lex,endtlg)!=0)
      {
        c=0;
        res=sscanf(tlg.lex+3,"PART%lu%c",&info.part_no,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong end of message");
        if (info.part_no<=0||info.part_no>=1000) throw ETlgError("Wrong part number");
        long part_no;
        if (headingInfo.tlg_cat==tcDCS)
          part_no=(dynamic_cast<TDCSHeadingInfo&>(headingInfo)).part_no;
        else
          part_no=(dynamic_cast<TBSMHeadingInfo&>(headingInfo)).part_no;
        if (info.part_no!=part_no) throw ETlgError("Wrong part number");
        info.pr_final_part=false;
      }
      else
        info.pr_final_part=true;
      if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
    }
    else throw ETlgError("Wrong end of message");
  }
  catch(const ETlgError& E)
  {
    throwTlgError(E.what(), ending, ending.p);
  };
  return;
};

void ParseAHMEnding(TTlgPartInfo ending, TEndingInfo& info)
{
  int res;
  char c;
  const char* p;
  TTlgParser tlg;

  try
  {
    if ((p=tlg.GetLexeme(ending.p))!=NULL)
    {
      if (strcmp(tlg.lex,"PART")!=0) throw ETlgError("Wrong end of message");
      if ((p=tlg.GetLexeme(p))!=NULL)
      {
        c=0;
        res=sscanf(tlg.lex,"%lu%c",&info.part_no,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong end of message");
        if (info.part_no<=0||info.part_no>=1000) throw ETlgError("Wrong part number");
      }
      else throw ETlgError("Wrong end of message");
      if ((p=tlg.GetLexeme(p))!=NULL)
      {
        if (!(strcmp(tlg.lex,"END")==0||
              strcmp(tlg.lex,"CONTINUED")==0)) throw ETlgError("Wrong end of message");
        info.pr_final_part=strcmp(tlg.lex,"END")==0;
      }
      else throw ETlgError("Wrong end of message");
      if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
    };
  }
  catch(const ETlgError& E)
  {
    throwTlgError(E.what(), ending, ending.p);
  };
  return;
};

//����� �⮡� ������﫨�� ����砫쭮
//info.tlg_type=HeadingInfo.tlg_type � info.part_no=HeadingInfo.part_no
void ParseEnding(TTlgPartInfo ending, THeadingInfo *headingInfo, TEndingInfo* &info, TMemoryManager &mem)
{
  if (headingInfo==NULL) throw ETlgError("headingInfo not defined");
  if (info!=NULL)
  {
    mem.destroy(info, STDLOG);
    delete info;
    info = NULL;
  };
  info = new TEndingInfo;
  mem.create(info, STDLOG);
  try
  {
    switch (headingInfo->tlg_cat)
    {
      case tcDCS:
        ParseDCSEnding(ending,*dynamic_cast<TDCSHeadingInfo*>(headingInfo), *info);
        break;
      case tcBSM:
        ParseDCSEnding(ending,*dynamic_cast<TBSMHeadingInfo*>(headingInfo), *info);
        break;
      case tcAHM:
        ParseAHMEnding(ending, *info);
        break;
      default:;
    };
  }
  catch(...)
  {
    mem.destroy(info, STDLOG);
    if (info!=NULL) delete info;
    info=NULL;
    throw;
  };
  return;
};

void NormalizeFltInfo(TFltInfo &flt)
{
  if (flt.airline[0]!=0) GetAirline(flt.airline);
  if (flt.suffix[0]!=0) GetSuffix(flt.suffix[0]);
  if (flt.airp_dep[0]!=0) GetAirp(flt.airp_dep);
  if (flt.airp_arv[0]!=0) GetAirp(flt.airp_arv);
};

void ParseSOMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TSOMContent& con)
{
  con.Clear();

  int res;
  char c;
  const char *p, *line_p;
  TTlgParser tlg;
  TTlgElement e;

  line_p=body.p;
  e=FlightElement;
  try
  {
    con.flt=info.flt;
    NormalizeFltInfo(con.flt);

    vector<TSeatsByDest>::iterator iSeats;
    char cat[5];
    *cat=0;
    bool usePriorContext=false;
    TSeatRanges ranges;

    e=SeatingCategories;
    do
    {
      if (tlg.GetToEOLLexeme(line_p)==NULL) continue;
      strcpy(lexh,tlg.lex);
      switch (e)
      {
        case SeatingCategories:
          if (lexh[0]=='-')
          {
            e=SeatsByDestination;
            [[fallthrough]];
          }
          else throw ETlgError("Seats by destination expected");
        case SeatsByDestination:
          if (strcmp(lexh,"PROT EX")==0)
          {
            strcpy(cat,"PROT");
            e=SeatingCategories;
            break;
          };
          if (strcmp(lexh,"SI")==0)
          {
            e=SupplementaryInfo;
            break;
          };
          if (lexh[0]=='-')
          {
            TSeatsByDest seats;
            iSeats=con.seats.insert(con.seats.end(),seats);
            //ࠧ���
            p=strchr(lexh,'.');
            if (p==NULL) throw ETlgError("Wrong seats by destination");
            //����� lexh �� ��� ��ப�
            lexh[p-lexh]=0;
            c=0;
            res=sscanf(lexh,"-%3[A-Z�-��]%c",iSeats->airp_arv,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong airport code");
            GetAirp(iSeats->airp_arv);
            usePriorContext=false;
            p++;
          }
          else
            p=lexh;

          while((p=tlg.GetLexeme(p))!=NULL)
          {
            try
            {
              ParseSeatRange(tlg.lex,ranges,usePriorContext);
              for(TSeatRanges::iterator i=ranges.begin();i!=ranges.end();i++)
              {
                strcpy(i->rem,cat);
              };
              iSeats->ranges.insert(iSeats->ranges.end(),ranges.begin(),ranges.end());
              usePriorContext=true;
            }
            catch(const EConvertError&)
            {
              //�������� �� ࠧ��ࠫ��
              if (lexh[0]=='-')
                throw ETlgError("Wrong seats element %s",tlg.lex);
              else
                break;
            };
          };
          if (lexh[0]!='-' && p!=NULL)
          {
            //�� ����� ���� ⮫쪮 ��⥣���
            c=0;
            res=sscanf(lexh,"%3[A-Z]%c",cat,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong seating category");
            e=SeatingCategories;
          };
          break;
        case SupplementaryInfo: break;
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch (const ETlgError& E)
  {
    throwTlgError(E.what(), body, line_p);
  };
  return;
};

void ParsePTMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPtmContent& con)
{
  vector<TPtmOutFltInfo>::iterator iOut;

  con.Clear();

  int res;
  char c;
  const char *p, *line_p, *ph;
  TTlgParser tlg;
  TTlgElement e;

  line_p=body.p;
  e=FlightElement;
  try
  {
    dynamic_cast<TFltInfo&>(con.InFlt)=info.flt;
    NormalizeFltInfo(con.InFlt);

    bool NILpossible=true;
    e=TransferPassengerData;
    do
    {
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case TransferPassengerData:
          {
            if (NILpossible && strcmp(tlg.lex,"NIL")==0)
            {
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              e=EndOfMessage;
              break;
            };
            NILpossible=false;
            TPtmOutFltInfo flt;
            TPtmTransferData data;
            strcpy(lexh,tlg.lex);
            if ((ph=tlg.GetSlashedLexeme(lexh))==NULL)
              throw ETlgError("Connecting flight not found");

            if (strlen(tlg.lex)<3||strlen(tlg.lex)>8) throw ETlgError("Wrong connecting flight");
            flt.suffix[0]=0;
            flt.suffix[1]=0;
            c=0;
            if (IsDigit(tlg.lex[2]))
              res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(tlg.lex,"%3[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong connecting flight");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong connecting flight");
            GetAirline(flt.airline);
            GetSuffix(flt.suffix[0]);
            if ((ph=tlg.GetSlashedLexeme(ph))!=NULL)
            {
              //���� ���, ���� �ਧ��� ����饣�
              long int day=0;
              c=0;
              res=sscanf(tlg.lex,"%2lu%c",&day,&c);
              if (c!=0||res!=1)
              {
                //�� �� ���
                flt.scd=info.flt.scd;
              }
              else
              {
                //�� ���
                if (day<=0) throw ETlgError("Wrong connecting flight date");
                int year,mon,currday;
                DecodeDate(info.flt.scd,year,mon,currday);
                if (day<currday)
                {
                  if (mon==12)
                  {
                    mon=1;
                    year++;
                  }
                  else mon++;
                };
                try
                {
                  EncodeDate(year,mon,day,flt.scd);
                }
                catch(const EConvertError&)
                {
                  throw ETlgError("Can't convert connecting flight date");
                };
                ph=tlg.GetSlashedLexeme(ph);
              };
              if (ph!=NULL)
              {
                if (strcmp(tlg.lex,"S")!=0&&
                    strcmp(tlg.lex,"N")!=0)
                  throw ETlgError("Wrong smoking indicator");
              };
              if ((ph=tlg.GetSlashedLexeme(ph))!=NULL) throw ETlgError("Unknown lexeme");
            }
            else flt.scd=info.flt.scd;
            flt.pr_utc=false;

            if ((p=tlg.GetLexeme(p))==NULL)
              throw ETlgError("Destination of the connecting flight not found");
            c=0;
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",flt.airp_arv,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong destination of the connecting flight");
            GetAirp(flt.airp_arv);
            strcpy(flt.airp_dep,con.InFlt.airp_arv);

            if ((p=tlg.GetLexeme(p))==NULL)
              throw ETlgError("Number of seats not found");
            flt.subcl[0]=0;
            c=0;
            res=sscanf(tlg.lex,"%3lu%1[A-Z�-��]%c",&data.seats,flt.subcl,&c);
            if (c==0&&res==1&&flt.subcl[0]==0)
            {
              //ࠧ��६ �������� � ᫥���饩 ���ᥬ�
              if ((p=tlg.GetLexeme(p))==NULL)
                throw ETlgError("Class of service not found");
              c=0;
              res=sscanf(tlg.lex,"%1[A-Z�-��]%c",flt.subcl,&c);
              if (c!=0||res!=1)
                throw ETlgError("Class of service: wrong format");
            }
            else
            {
              if (c!=0||res!=2)
                throw ETlgError("Number of seats/class of service: wrong format");
            };
            if (data.seats<0) throw ETlgError("Number of seats: wrong format");
            GetSubcl(flt.subcl);

            if (tlg.GetToEOLLexeme(p)==NULL) //��⠥� �� ���� ��ப�
              throw ETlgError("Checked baggage not found");

            lexh[0]=0;
            sscanf(tlg.lex,"%[^.]%s",lexh,tlg.lex);  //��०�� �� �窨

            if ((ph=tlg.GetLexeme(lexh))==NULL)
              throw ETlgError("Checked baggage not found");
            char ch;
            c=0;
            ch=0;
            res=sscanf(tlg.lex,"%3lu%c%4lu%1[KL]%c",
                               &data.bag_amount,&ch,&data.bag_weight,data.weight_unit,&c);
            if (c!=0||res!=4||ch!='B')
            {
              data.bag_weight=0;
              *(data.weight_unit)=0;
              c=0;
              ch=0;
              res=sscanf(tlg.lex,"%3lu%c%c",&data.bag_amount,&ch,&c);
              if (c!=0||res!=2||ch!='B')
                throw ETlgError("Checked baggage: wrong format");
            };
            if (data.bag_amount<0||data.bag_weight<0)
              throw ETlgError("Checked baggage: wrong format");

            //ph 㪠�뢠�� �� ��砫� NAME-����� � ���� lexh
            vector<string> names;
            ParseNameElement(ph,names,epNone);
            for(vector<string>::iterator i=names.begin();i!=names.end();i++)
            {
              if (i==names.begin())
                data.surname=*i;
              else
                data.name.push_back(*i);
            };

            //�饬 ३� flt � con.OutFlt
            for(iOut=con.OutFlt.begin();iOut!=con.OutFlt.end();iOut++)
              if (strcmp(iOut->airline,flt.airline)==0&&
                  iOut->flt_no==flt.flt_no&&
                  strcmp(iOut->suffix,flt.suffix)==0&&
                  iOut->scd==flt.scd&&
                  iOut->pr_utc==flt.pr_utc&&
                  strcmp(iOut->airp_dep,flt.airp_dep)==0&&
                  strcmp(iOut->airp_arv,flt.airp_arv)==0&&
                  strcmp(iOut->subcl,flt.subcl)==0) break;
            if (iOut==con.OutFlt.end())
              iOut=con.OutFlt.insert(con.OutFlt.end(),flt);
            iOut->data.push_back(data);
            break;
          }
        case EndOfMessage:
          {
            throw ETlgError("Unknown lexeme");
          }
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch (const ETlgError& E)
  {
    throwTlgError(E.what(), body, line_p);
  };
  return;
};

bool isSeatRem(const string &rem_code)
{
  return rem_code=="EXST" ||
         rem_code=="GPST" ||
         rem_code=="RQST" ||
         rem_code=="SEAT" ||
         rem_code=="NSST" ||
         rem_code=="NSSA" ||
         rem_code=="NSSB" ||
         rem_code=="NSSW" ||
         rem_code=="SMST" ||
         rem_code=="SMSA" ||
         rem_code=="SMSB" ||
         rem_code=="SMSW";
};

template <typename T>
void CompartmentToFareClass(const vector<TRbdItem> &rbd, T &pattern)
{
  pattern.cl=NoClass;
  for(vector<TRbdItem>::const_iterator iRbdItem=rbd.begin();iRbdItem!=rbd.end();++iRbdItem)
    if (iRbdItem->rbds.find_first_of(pattern.subcl[0])!=string::npos)
    {
      if (pattern.cl!=NoClass)
        throw ETlgError("Duplicate RBD subclass '%s'",pattern.subcl);
      if (iRbdItem->cl==NoClass)
        throw ETlgError("Unknown RBD class code '%s'",iRbdItem->subcl);
      pattern.cl=iRbdItem->cl;
    };
  if (pattern.cl==NoClass&&(pattern.cl=GetClass(pattern.subcl))==NoClass)
    throw ETlgError("Unknown subclass '%s'",pattern.subcl);
};

void CheckDCSBagPool(const TPNLADLPRLContent& con)
{
  if (con.resa.empty() || con.resa.back().pnr.empty() || con.resa.back().pnr.back().ne.empty()) return;
  vector<TNameElement>::const_reverse_iterator iNameElemLast=con.resa.rbegin()->pnr.rbegin()->ne.rbegin();
  if (iNameElemLast->bag_pool==NoExists) return;
  for(vector<TTotalsByDest>::const_reverse_iterator iTotals=con.resa.rbegin(); iTotals!=con.resa.rend(); ++iTotals)
    for(vector<TPnrItem>::const_reverse_iterator iPnr=iTotals->pnr.rbegin(); iPnr!=iTotals->pnr.rend(); ++iPnr)
       for(vector<TNameElement>::const_reverse_iterator iNameElem=iPnr->ne.rbegin();iNameElem!=iPnr->ne.rend(); ++iNameElem)
       {
         if (iNameElem->bag_pool==NoExists || iNameElemLast->bag_pool!=iNameElem->bag_pool) continue;
         if (iTotals==con.resa.rbegin())
         {
           if (iNameElemLast!=iNameElem &&
               (!iNameElemLast->tags.empty() || !iNameElemLast->bag.Empty()) &&
               (!iNameElem->tags.empty() || !iNameElem->bag.Empty()))
            throw ETlgError("More than one head of pooled baggage %03d", iNameElemLast->bag_pool);
         }
         else
           throw ETlgError("Duplicate baggage pooling element %03d", iNameElemLast->bag_pool);
       };
};

void ParsePNLADLPRLContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPNLADLPRLContent& con)
{
  vector<TRbdItem>::iterator iRbdItem;
  vector<TRouteItem>::iterator iRouteItem;
  vector<TSeatsItem>::iterator iSeatsItem;
  vector<TTotalsByDest>::iterator iTotals;
  vector<TPaxItem>::iterator iPaxItem,iPaxItemPrev;
  vector<TPnrItem>::iterator iPnrItem;

  con.Clear();

  int res;
  char c;
  const char *p, *line_p;
  TTlgParser tlg;
  TTlgElement e;
  TIndicator Indicator = None;
  int e_part = 0;
  bool pr_prev_rem;

  line_p=body.p;
  e=FlightElement;
  try
  {
    con.flt=info.flt;
    NormalizeFltInfo(con.flt);

    bool isPRL=strcmp(info.tlg_type,"PRL")==0;

    bool NILpossible=isPRL;
    if (isPRL)
      e=Configuration;
    else
      e=BonusPrograms;
    do
    {
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case BonusPrograms:
          if (strcmp(tlg.lex,"FQT")==0)
          {
            e=Configuration;
            break;
          };
          e=Configuration;
          [[fallthrough]];
        case Configuration:
          if (strncmp(tlg.lex,"CFG/",4)==0)
          {
            do
            {
              TRouteItem RouteItem;
              RouteItem.station[0]=0;
              res=sscanf(tlg.lex,"%3[A-Z�-��]/%s",RouteItem.station,tlg.lex);
              if (res!=2)
              {
                if (con.cfg.empty() && res==1 && strcmp(RouteItem.station,"CFG")==0) break;
                throw ETlgError("Wrong format");
              };
              if (con.cfg.empty())
                strcpy(RouteItem.station,con.flt.airp_dep);
              else
                GetAirp(RouteItem.station);
              for(iRouteItem=con.cfg.begin();iRouteItem!=con.cfg.end();iRouteItem++)
              {
                if (strcmp(RouteItem.station,iRouteItem->station)==0)
                  throw ETlgError("Duplicate airport code '%s'",RouteItem.station);
              };
              res=sscanf(tlg.lex,"%[^/]/%s",lexh,tlg.lex);
              if (res==0) throw ETlgError("Wrong format");
              do
              {
                TSeatsItem SeatsItem;
                SeatsItem.subcl[0]=0;
                res=sscanf(lexh,"%3lu%1[A-Z�-��]%s",&SeatsItem.seats,SeatsItem.subcl,lexh);
                if (res<2||SeatsItem.seats<0||SeatsItem.subcl[0]==0)
                  throw ETlgError("Wrong format");
                if ((SeatsItem.cl=GetClass(SeatsItem.subcl))==NoClass)
                  throw ETlgError("Unknown subclass '%s'",SeatsItem.subcl);
                for(iSeatsItem=RouteItem.seats.begin();iSeatsItem!=RouteItem.seats.end();iSeatsItem++)
                {
                  if (strcmp(SeatsItem.subcl,iSeatsItem->subcl)==0)
                    throw ETlgError("Duplicate subclass '%s'",SeatsItem.subcl);
                };
                RouteItem.seats.push_back(SeatsItem);
              }
              while (res==3);
              con.cfg.push_back(RouteItem);
            }
            while (0);//((p=get_lexeme(p))!=NULL);???
            e=ClassCodes;
            break;
          };
          e=ClassCodes;
          [[fallthrough]];
        case ClassCodes:
          if (strcmp(tlg.lex,"RBD")==0)
          {
            while ((p=tlg.GetLexeme(p))!=NULL)
            {
              TRbdItem RbdItem;
              c=0;
              res=sscanf(tlg.lex,"%1[A-Z�-��]/%[A-Z�-��?]%c",RbdItem.subcl,lexh,&c);
              if (c!=0||res!=2) throw ETlgError("Wrong format");
              RbdItem.cl=GetClass(RbdItem.subcl);
              RbdItem.rbds=lexh;
              con.rbd.push_back(RbdItem);
            };
            if (isPRL)
              e=TotalsByDestination;
            else
            e=SpaceAvailableElement;
            e_part=1;
            break;
          };
          if (isPRL)
            e=TotalsByDestination;
          else
          e=SpaceAvailableElement;
          e_part=1;
          [[fallthrough]];
        case SpaceAvailableElement:
          if (e_part==1)
          {
            if (strcmp(tlg.lex,"AVAIL")==0)
            {
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              e_part=2;
              break;
            };
          }
          else
            if (e_part==2)
            {
              // ��������� routing element
              do
              {
                TRouteItem RouteItem;
                c=0;
                res=sscanf(tlg.lex,"%3[A-Z�-��]%c",RouteItem.station,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong station");
                GetAirp(RouteItem.station);
                for(iRouteItem=con.avail.begin();iRouteItem!=con.avail.end();iRouteItem++)
                {
                  if (iRouteItem==con.avail.begin()&&
                      strcmp(iRouteItem->station,con.flt.airp_dep)==0) continue; //���� �㭪� ����� ᮢ������ � ����� �� ��᫥����� � AVAIL
                  if (strcmp(RouteItem.station,iRouteItem->station)==0)
                    throw ETlgError("Duplicate airport code '%s'",RouteItem.station);
                };
                con.avail.push_back(RouteItem);
              }
              while ((p=tlg.GetLexeme(p))!=NULL);
              if (!con.avail.empty())
              {
                iRouteItem=con.avail.begin();
                if (strcmp(iRouteItem->station,con.flt.airp_dep)==0)
                  con.avail.erase(iRouteItem);
              };
              e_part=3;
              break;
            }
            else
            {
              //��������� space available element
              TSeatsItem SeatsItem;
              char minus;
              bool pr_error;
              for(pr_error=true;pr_error;pr_error=false)
              {
                res=sscanf(tlg.lex,"%1[A-Z�-��]%s",SeatsItem.subcl,tlg.lex);
                if (res!=2)
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong seats format");
                };
                minus=0;
                c=0;
                res=sscanf(tlg.lex,"%3lu%c%c",&SeatsItem.seats,&minus,&c);
                if (c!=0||res<1||SeatsItem.seats<0||(minus!=0&&minus!='-'))
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong number of seats");
                };

                //�஢�ਬ � RBD
                CompartmentToFareClass<TSeatsItem>(con.rbd, SeatsItem);
                iRouteItem=con.avail.begin();
                do
                {
                  minus=0;
                  c=0;
                  res=sscanf(tlg.lex,"%3lu%c%c",&SeatsItem.seats,&minus,&c);
                  if (c!=0||res<1||SeatsItem.seats<0) throw ETlgError("Wrong number of seats");
                  if (minus!=0)
                  {
                    if (minus=='-') SeatsItem.seats=-SeatsItem.seats;
                    else throw ETlgError("Wrong number of seats");
                  };
                  if (iRouteItem==con.avail.end())
                    throw ETlgError("Too few stations");
                  iRouteItem->seats.push_back(SeatsItem);
                  iRouteItem++;
                }
                while ((p=tlg.GetLexeme(p))!=NULL);
                if (iRouteItem!=con.avail.end())
                  throw ETlgError("Too many stations");
                e_part=4;
              };
              if (pr_error) tlg.GetLexeme(line_p); else break;
            };
          e=TranzitElement;
          e_part=1;
          [[fallthrough]];
        case TranzitElement:
          if (e_part==1)
          {
            if (strcmp(tlg.lex,"TRANSIT")==0)
            {
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              e_part=2;
              break;
            };
          }
          else
            if (e_part==2)
            {
              // ��������� routing element
              do
              {
                TRouteItem RouteItem;
                c=0;
                res=sscanf(tlg.lex,"%3[A-Z�-��]%c",RouteItem.station,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong station");
                GetAirp(RouteItem.station);
                for(iRouteItem=con.transit.begin();iRouteItem!=con.transit.end();iRouteItem++)
                {
                  if (strcmp(RouteItem.station,iRouteItem->station)==0)
                    throw ETlgError("Duplicate airport code '%s'",RouteItem.station);
                };
                con.transit.push_back(RouteItem);
              }
              while ((p=tlg.GetLexeme(p))!=NULL);
              e_part=3;
              break;
            }
            else
            {
              //��������� transit element
              TSeatsItem SeatsItem;
              bool pr_error;
              for(pr_error=true;pr_error;pr_error=false)
              {
                res=sscanf(tlg.lex,"%1[A-Z�-��]%s",SeatsItem.subcl,tlg.lex);
                if (res!=2)
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong seats format");
                };
                c=0;
                res=sscanf(tlg.lex,"%3lu%c",&SeatsItem.seats,&c);
                if (c!=0||res<1||SeatsItem.seats<0)
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong number of seats");
                };

                //�஢�ਬ � RBD
                CompartmentToFareClass<TSeatsItem>(con.rbd, SeatsItem);
                iRouteItem=con.transit.begin();
                do
                {
                  c=0;
                  res=sscanf(tlg.lex,"%3lu%c",&SeatsItem.seats,&c);
                  if (c!=0||res<1||SeatsItem.seats<0) throw ETlgError("Wrong number of seats");
                  if (iRouteItem==con.transit.end())
                    throw ETlgError("Too few stations");
                  iRouteItem->seats.push_back(SeatsItem);
                  iRouteItem++;
                }
                while ((p=tlg.GetLexeme(p))!=NULL);
                if (iRouteItem!=con.transit.end())
                  throw ETlgError("Too many stations");
                e_part=4;
              };
              if (pr_error) tlg.GetLexeme(line_p); else break;
            };
          e=TotalsByDestination;
          e_part=1;
          [[fallthrough]];
        case TotalsByDestination:
          if (NILpossible && strcmp(tlg.lex,"NIL")==0)
          {
            if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            e=EndOfMessage;
            break;
          };
          NILpossible=false;

          if (tlg.lex[0]=='-')
          {
            TTotalsByDest Totals;
            char minus=0;
            c=0;
            res=sscanf(tlg.lex,"-%3[A-Z�-��]%3lu%1[A-Z�-��?]%cPAD%3lu%c",
                       Totals.dest,&Totals.seats,Totals.subcl,&minus,&Totals.pad,&c);
            if (c!=0||res<3||Totals.seats<0) throw ETlgError("Wrong format");
            if (minus!=0)
            {
              if (minus!='-'||res!=5||Totals.pad<0)
                throw ETlgError("Wrong format");
            }
            else Totals.pad=0;
            GetAirp(Totals.dest);

            for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
              if (strcmp(iTotals->dest,Totals.dest)==0&&
                  strcmp(iTotals->subcl,Totals.subcl)==0) break;

            if (iTotals!=con.resa.end())
            {
              if (iTotals->seats!=Totals.seats)
                throw ETlgError("Duplicate: destination '%s', subclass '%s'",
                                Totals.dest,Totals.subcl);
              if ((iTotals+1)==con.resa.end()) break;
            }
            else
            {
              CompartmentToFareClass<TTotalsByDest>(con.rbd, Totals);
              iTotals=con.resa.insert(con.resa.end(),Totals);
            };

            if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
            Indicator=None;
            e_part=2;
            break;
          }
          else
            if (e_part==1) throw ETlgError("Totals by destination expected");

          if (strcmp(info.tlg_type,"ADL")==0)
          {
            if (e_part>1&&
                (strcmp(tlg.lex,"ADD")==0||
                 strcmp(tlg.lex,"CHG")==0||
                 strcmp(tlg.lex,"DEL")==0))
            {
              if (strcmp(tlg.lex,"ADD")==0) Indicator=ADD;
              if (strcmp(tlg.lex,"CHG")==0) Indicator=CHG;
              if (strcmp(tlg.lex,"DEL")==0) Indicator=DEL;
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              e_part=2;
              break;
            }
            else
              if (Indicator==None) throw ETlgError("ADD/CHG/DEL indicator expected");
          };

          if (e_part>1)
          {
            if (!(tlg.lex[0]=='.'&&e_part>2))
            {
              if ((p=tlg.GetNameElement(line_p,true))==NULL) continue;

              char grp_ref[3];
              long grp_seats;
              *grp_ref=0;
              grp_seats=0;
              iPnrItem=iTotals->pnr.end();
              char *ph;
              if ((ph=strrchr(tlg.lex,'-'))!=NULL)
              {
                //���஡㥬 ࠧ����� �����䨪��� ��㯯�
                c=0;
                res=sscanf(ph,"-%2[A-Z]%3lu%c",grp_ref,&grp_seats,&c);
                if (c!=0||res!=2||grp_ref[0]==0||grp_seats<=0)
                {
                  //�����䨪��� ��㯯� �� ������
                  *grp_ref=0;
                  grp_seats=0;
                }
                else
                {
                  for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
                  {
                    if (strcmp(iPnrItem->grp_ref,grp_ref)==0)
                    {
                      //�� ���窨 ��������祭� ��⮬� ��
                      //����᪠��, �� grp_seats ����� ���� ࠧ�� � ࠬ��� ����� ⥫��ࠬ��
                      //��, ��� �������� �ࠪ⨪�, ⠪�� ������ �뢠��
                      //if (iPnrItem->grp_seats!=grp_seats)
                      //  throw ETlgError("Different number of seats in same group identifier");
                      break;
                    };
                  };
                  *ph=0;
                };
              };
              if (iPnrItem==iTotals->pnr.end())
              {
                iPnrItem=iTotals->pnr.insert(iPnrItem,TPnrItem());
                strcpy(iPnrItem->grp_ref,grp_ref);
                iPnrItem->grp_seats=grp_seats;
              };

              iPnrItem->ne.push_back(TNameElement());
              TNameElement& ne=iPnrItem->ne.back();
              pr_prev_rem=false;
              ne.indicator=Indicator;

              vector<string> names;
              ParseNameElement(tlg.lex,names,epMandatory);
              if (names.empty()) throw ETlgError("Wrong format");
              ne.pax.clear();
              for(vector<string>::iterator i=names.begin();i!=names.end();i++)
              {
                if (i==names.begin())
                  ne.surname=*i;
                else
                {
                  TPaxItem PaxItem;
                  PaxItem.name=*i;
                  /*if (i->find("CHD")!=string::npos)
                    PaxItem.pers_type=child;*/

                  ne.pax.push_back(PaxItem);
                };
              };
              ne.seats=ne.pax.size();
              if (ne.isSpecial())
              {
                for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
                  if (iPaxItem->name.empty()) iPaxItem->name=ne.pax.begin()->name;
              };
            }
            else p=line_p;

            while ((p=tlg.GetNameElement(p,false))!=NULL)
            {
              bool pr_bag_info=false;
              ParsePaxLevelElement(tlg,con.flt,*iPnrItem,pr_prev_rem,pr_bag_info);
              if (pr_bag_info) CheckDCSBagPool(con);
            };

            e_part=3;
            break;
          };
          [[fallthrough]];
        case EndOfMessage:
          {
            throw ETlgError("Unknown lexeme");
          }
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);

    //3 ��室� ��ࠡ�⪨ ६�ப
    //A. �ਢ易�� ६�ન � ���ᠦ�ࠬ
    for(TTotalsByDest& totals : con.resa)
      for(TPnrItem& pnr : totals.pnr)
        for(TNameElement& ne : pnr.ne)
        {
          BindRemarks(tlg, ne);
          ne.fillSeatBlockingRemList(tlg);
        }

    //B. �஠������஢��� �������⥫�� ���� ���ᠦ�஢ (��᫥ �ਢ離�, �� �� ࠧ��� ६�ப!)
    for(TTotalsByDest& totals : con.resa)
      for(TPnrItem& pnr : totals.pnr)
      {
        pnr.separateSeatsBlocking();
        pnr.bindSeatsBlocking();
      }

    //��ଠ���㥬 RBD ��� ����� � ����
    for(vector<TRbdItem>::iterator i=con.rbd.begin(); i!=con.rbd.end(); ++i)
    {
      GetSubcl(i->subcl);
      string normal_rbds;
      for(string::const_iterator j=i->rbds.begin(); j!=i->rbds.end(); ++j)
      {
        char subclh[2];
        subclh[0]=*j;
        subclh[1]=0;
        GetSubcl(subclh);

        //�஢�ਬ �� �㡫�஢����
        vector<TRbdItem>::const_iterator k=con.rbd.begin();
        for(; k!=i; ++k)
        {
          if (k->rbds.find_first_of(subclh[0])==string::npos) continue;
          if (strcmp(i->subcl, k->subcl)==0) break;
          throw ETlgError("Duplicate RBD subclass '%s'",subclh);
        };
        if (k!=i) continue; //㦥 ���� ⠪�� ��� compartment/fare_class

        if (normal_rbds.find_first_of(subclh[0])==string::npos) normal_rbds.append(subclh);
      };
      i->rbds=normal_rbds;
    };

    //C. ࠧ����� ६�ન
    TSeatRemPriority seat_rem_priority;
    string airline_mark; //�� ����� �������� ����७ seat_rem_priority

    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
    {
      GetSubcl(iTotals->subcl); //��������� � ���� �࠭�� ���᪨�
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();++iPnrItem)
      {
        string pnr_airline_mark=iPnrItem->market_flt.Empty()?con.flt.airline:iPnrItem->market_flt.airline;
        if (airline_mark.empty() || pnr_airline_mark!=airline_mark)
        {
          GetSeatRemPriority(pnr_airline_mark, seat_rem_priority);
          airline_mark=pnr_airline_mark;
        };

        for(TNameElement& ne : iPnrItem->ne)
        {
          ParseRemarks(seat_rem_priority,tlg,info,*iPnrItem,ne);
          ne.removeNotConfimedSSRs();
          ne.bindSystemIds();
        };
      };
    };
  }
  catch (const ETlgError& E)
  {
    throwTlgError(E.what(), body, line_p);
  };
  return;
};

void ParsePaxLevelElement(TTlgParser &tlg, TFltInfo& flt, TPnrItem &pnr, bool &pr_prev_rem, bool &pr_bag_info)
{
  char c;
  int res;
  if (pnr.ne.empty()) return;
  TNameElement& ne=pnr.ne.back();

  pr_bag_info=false;
  //ࠧ��� ६�ப
  c=0;
  res=sscanf(tlg.lex,".%[A-Z0-9]%c",lexh,&c);
  if (c!='/'||res!=2)
  {
    if (!pr_prev_rem||ne.rem.empty())
      throw ETlgError("Wrong identifying code format");

    //�������� �� �⭮���� � ६�થ ᢮������� �ଠ�
    TRemItem& RemItem=ne.rem.back();
    RemItem.text+=" ";
    RemItem.text+=tlg.lex;
    return;
  };
  pr_prev_rem=true;
  if (strcmp(lexh,"R")==0)
  {
    TRemItem RemItem;
    RemItem.text=tlg.lex+3;
    ne.rem.push_back(RemItem);
    return;
  };
  if (strcmp(lexh,"RN")==0)
  {
    if (ne.rem.empty())
      ne.rem.push_back(TRemItem());

    TRemItem& RemItem=ne.rem.back();
    RemItem.text+=tlg.lex+4;
    return;
  };

  //ᤥ���� trimRight �᫨ �� �⭮���� � ६�ઠ�
  for(int len=strlen(tlg.lex);len>=0&&*(const unsigned char*)(tlg.lex+len)<=' ';len--)
    *(tlg.lex+len)=0;

  pr_prev_rem=false;
  if (strcmp(lexh,"C")==0 && strcmp(tlg.lex,".C/")!=0)
  {
    c=0;
    res=sscanf(tlg.lex,".C/%[A-Z�-��0-9/ ]%c",lexh,&c);
    if (c!=0||res!=1) throw ETlgError("Wrong corporate or group name element");
    if (!pnr.grp_name.empty())
    {
      if (pnr.grp_name!=lexh)
        throw ETlgError("Different corporate or group name element in group found");
    }
    else
      pnr.grp_name=lexh;
    return;
  };
  if (strcmp(lexh,"L")==0)
  {
    TPnrAddrItem PnrAddr;
    lexh[0]=0;
    res=sscanf(tlg.lex,".L/%20[A-Z�-��0-9]%[^.]",PnrAddr.addr,tlg.lex);
    if (res<1||PnrAddr.addr[0]==0||
        strlen(PnrAddr.addr)<5 ||
        strlen(PnrAddr.addr)>8) throw ETlgError("Wrong PNR address");
    if (res==2)
    {
      c=0;
      res=sscanf(tlg.lex,"/%3[A-Z�-��0-9]%c",PnrAddr.airline,&c);
      if (c!=0||res!=1) throw ETlgError("Wrong PNR address");
      GetAirline(PnrAddr.airline);
    };
    if (PnrAddr.airline[0]==0) strcpy(PnrAddr.airline,flt.airline);

    if (strlen(PnrAddr.addr)<=6)
    {
      //������ �� ����७��
      vector<TPnrAddrItem>::iterator i;
      for(i=pnr.addrs.begin();i!=pnr.addrs.end();i++)
        if (strcmp(PnrAddr.airline,i->airline)==0) break;
      if (i!=pnr.addrs.end())
      {
        if (strcmp(i->addr,PnrAddr.addr)!=0)
          throw ETlgError("Different PNR address in group found");
      }
      else
      {
        pnr.addrs.push_back(PnrAddr);
      };
    }
    return;
  };
  if (strcmp(lexh,"WL")==0 ||
      strcmp(lexh,"DG1")==0 ||
      strcmp(lexh,"DG2")==0 ||
      strcmp(lexh,"RG1")==0 ||
      strcmp(lexh,"RG2")==0 ||
      strcmp(lexh,"ID1")==0 ||
      strcmp(lexh,"ID2")==0)
  {
    if (strcmp(pnr.status,"")!=0)
    {
      if (strcmp(pnr.status,lexh)!=0)
        throw ETlgError("Different PNR status in group found (%s|%s)",pnr.status,lexh);
    }
    else strcpy(pnr.status,lexh);
    //�⠥� priority
    c=0;
    res=sscanf(tlg.lex,".%*[A-Z0-9]%c%s",&c,tlg.lex);
    if (c!='/'||res==0)
      throw ETlgError("Wrong identifying code format");
    if (res==2)
    {
      //���� priority designator
      if (!pnr.priority.empty())
      {
        if (strcmp(pnr.priority.c_str(),tlg.lex)!=0)
          throw ETlgError("Different PNR status priority in group found (%s|%s)",pnr.priority.c_str(),tlg.lex);
      }
      else pnr.priority=tlg.lex;
    };
    return;
  };

  if (lexh[0]=='I'||lexh[0]=='O'||strcmp(lexh,"M")==0)
  {
    char elementID=lexh[0];
/*  �� ��� ���஢����
    for(int pass_airp_dep=0; pass_airp_dep<=1; pass_airp_dep++)
      for(int pass_airp_arv=0; pass_airp_arv<=1; pass_airp_arv++)
        for(int pass_time_dep=0; pass_time_dep<=1; pass_time_dep++)
          for(int pass_time_arv=0; pass_time_arv<=1; pass_time_arv++)
            for(int pass_date_variation=-1; pass_date_variation<=1; pass_date_variation++)
              for(int pass_status=0; pass_status<=1; pass_status++)
              {
                ostringstream s;
                s << "." << elementID << "/UT575V23"
                  << (pass_airp_dep==0?"":"VKO")
                  << (pass_airp_arv==0?"":"KRR")
                  << (pass_time_dep==0?"":"0100")
                  << (pass_time_arv==0?"":"0200")
                  << (pass_date_variation==0?"":(pass_date_variation==-1?"/M1":"/2"))
                  << (pass_status==0?"":"HK");
                strcpy(tlg.lex, s.str().c_str());
                lexh[0]=elementID;
                lexh[1]=0;
                try
                {
*/
    TTransferItem Transfer;
    const char *errMsg;
    if (elementID=='M')
    {
      errMsg="Wrong marketing flight element";
      c=0;
      res=sscanf(tlg.lex,".M/%[A-Z�-��0-9]%c",lexh,&c);
    }
    else
    {
      errMsg="Wrong connection element";
      if (lexh[1]==0)
      {
        Transfer.num=1;
        if (lexh[0]=='I') Transfer.num=-Transfer.num;
        c=0;
        res=sscanf(tlg.lex,".%*c/%[A-Z�-��0-9/]%c",lexh,&c);
      }
      else
      {
        c=0;
        res=sscanf(lexh+1,"%1lu%c",&Transfer.num,&c);
        if (res==0) return; //��㣮� �����
        if (c!=0||res!=1||Transfer.num<=0)
          throw ETlgError(errMsg);
        if (lexh[0]=='I') Transfer.num=-Transfer.num;
        c=0;
        res=sscanf(tlg.lex,".%*c%*1[0-9]/%[A-Z�-��0-9/]%c",lexh,&c);
      };
    };
    if (c!=0||res!=1||strlen(lexh)<3) throw ETlgError(errMsg);
    c=0;
    if (IsDigit(lexh[2]))
    {
      res=sscanf(lexh,"%2[A-Z�-��0-9]%5lu%1[A-Z�-��]%2lu%[A-Z�-��0-9/]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%2[A-Z�-��0-9]%5lu%1[A-Z�-��]%1[A-Z�-��]%2lu%[A-Z�-��0-9/]",
                        Transfer.airline,&Transfer.flt_no,
                        Transfer.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError(errMsg);
        GetSuffix(Transfer.suffix[0]);
      };
    }
    else
    {
      res=sscanf(lexh,"%3[A-Z�-��0-9]%5lu%1[A-Z�-��]%2lu%[A-Z�-��0-9/]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%3[A-Z�-��0-9]%5lu%1[A-Z�-��]%1[A-Z�-��]%2lu%[A-Z�-��0-9/]",
                        Transfer.airline,&Transfer.flt_no,
                        Transfer.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError(errMsg);
        GetSuffix(Transfer.suffix[0]);
      };
    };

    if (Transfer.flt_no<0||Transfer.local_date<=0||Transfer.local_date>31)
      throw ETlgError(errMsg);

    Transfer.airp_dep[0]=0;
    Transfer.airp_arv[0]=0;
    lexh[0]=0;
    res=sscanf(tlg.lex,"%3[A-Z�-��]%3[A-Z�-��]%[A-Z�-��0-9/]",
                       Transfer.airp_dep,Transfer.airp_arv,lexh);
    if (res<2||strlen(Transfer.airp_dep)!=3||strlen(Transfer.airp_arv)!=3)
    {
      if (elementID=='M') throw ETlgError(errMsg);
      Transfer.airp_dep[0]=0;
      Transfer.airp_arv[0]=0;
      lexh[0]=0;
      res=sscanf(tlg.lex,"%3[A-Z�-��]%[A-Z�-��0-9/]",
                         Transfer.airp_dep,lexh);
      if (res<1||strlen(Transfer.airp_dep)!=3) throw ETlgError(errMsg);
    };
    if (lexh[0]!=0)
    {
      Transfer.local_time_dep[0]=0;
      Transfer.local_time_arv[0]=0;
      tlg.lex[0]=0;
      res=sscanf(lexh,"%4[0-9]%4[0-9]%[A-Z�-��0-9/]",
                      Transfer.local_time_dep, Transfer.local_time_arv, tlg.lex);
      if (res<2||
          strlen(Transfer.local_time_dep)!=4||
          strlen(Transfer.local_time_arv)!=4)
      {
        Transfer.local_time_dep[0]=0;
        Transfer.local_time_arv[0]=0;
        tlg.lex[0]=0;
        res=sscanf(lexh,"%4[0-9]%[A-Z�-��0-9/]",
                        Transfer.local_time_dep, tlg.lex);
        if (res<1||
            strlen(Transfer.local_time_dep)!=4)
        {
          Transfer.local_time_dep[0]=0;
          strcpy(tlg.lex, lexh);
        };
      }
      else
      {
        //�᫨ .M ��� .I � �訡��, ⠪ ��� ��� ��� ⮫쪮 ���� ���
        if (elementID=='M' || elementID=='I') throw ETlgError(errMsg);
      };
      if (tlg.lex[0]!=0)
      {
        int d=0;
        if (tlg.lex[d]=='/')
        {
          d++;
          int sign=1;
          if (tlg.lex[d]=='M')
          {
            sign=-1;
            d++;
          };
          if (IsDigit(tlg.lex[d]) &&
              StrToInt(string(1,tlg.lex[d]).c_str(),Transfer.date_variation)!=EOF)
          {
            Transfer.date_variation*=sign;
            d++;
          }
          else throw ETlgError(errMsg);
        };
        if (tlg.lex[d]!=0)
        {
          c=0;
          res=sscanf(tlg.lex+d,"%2[A-Z�-��]%c",Transfer.status,&c);
          if (c!=0||res!=1||strlen(Transfer.status)!=2) throw ETlgError(errMsg);
        };
      };
    };
    if (elementID=='I')
    {
      if (Transfer.local_time_arv[0]==0)
      {
        strcpy(Transfer.local_time_arv, Transfer.local_time_dep);
        Transfer.local_time_dep[0]=0;
      };
      if (Transfer.date_variation!=ASTRA::NoExists &&
          Transfer.local_time_arv[0]==0) throw ETlgError(errMsg);
    };
    if (elementID=='O')
    {
      if (Transfer.airp_arv[0]==0)
      {
        strcpy(Transfer.airp_arv, Transfer.airp_dep);
        Transfer.airp_dep[0]=0;
      };
      if (Transfer.date_variation!=ASTRA::NoExists &&
          Transfer.local_time_arv[0]==0)
      {
        strcpy(Transfer.local_time_arv, Transfer.local_time_dep);
        Transfer.local_time_dep[0]=0;
      };
      if (Transfer.date_variation!=ASTRA::NoExists &&
          Transfer.local_time_arv[0]==0) throw ETlgError(errMsg);
    };
    if (elementID=='M')
    {
      if (Transfer.date_variation!=ASTRA::NoExists) throw ETlgError(errMsg);
    };
/*  �� ��� ���஢����
                  ProgTrace(TRACE5, "%-30s: |%-3s|%-3s|%-4s|%-4s|%3s|%-2s",
                                    s.str().c_str(),
                                    (Transfer.airp_dep[0]==0?"":Transfer.airp_dep),
                                    (Transfer.airp_arv[0]==0?"":Transfer.airp_arv),
                                    (Transfer.local_time_dep[0]==0?"":Transfer.local_time_dep),
                                    (Transfer.local_time_arv[0]==0?"":Transfer.local_time_arv),
                                    (Transfer.date_variation==ASTRA::NoExists?"":IntToString(Transfer.date_variation).c_str()),
                                    (Transfer.status[0]==0?"":Transfer.status));
                }
                catch(const ETlgError&)
                {
                  ProgTrace(TRACE5, "%-30s: ETlgError!", s.str().c_str());
                };
              };
*/
    bool throwIfUnknownCode=(elementID=='M');
    GetAirline(Transfer.airline, true, throwIfUnknownCode);
    GetAirp(Transfer.airp_dep, false, throwIfUnknownCode);
    GetAirp(Transfer.airp_arv, false, throwIfUnknownCode);
    GetSubcl(Transfer.subcl, throwIfUnknownCode);

    if (elementID=='M')
    {
      //������ �� ����७��
      if (!pnr.market_flt.Empty())
      {
        if (strcmp(pnr.market_flt.airline,Transfer.airline)!=0||
            pnr.market_flt.flt_no!=Transfer.flt_no||
            strcmp(pnr.market_flt.suffix,Transfer.suffix)!=0||
            pnr.market_flt.local_date!=Transfer.local_date||
            strcmp(pnr.market_flt.airp_dep,Transfer.airp_dep)!=0||
            strcmp(pnr.market_flt.airp_arv,Transfer.airp_arv)!=0||
            strcmp(pnr.market_flt.subcl,Transfer.subcl)!=0)
          throw ETlgError("Different marketing flight in group found");
      };
      strcpy(pnr.market_flt.airline,Transfer.airline);
      pnr.market_flt.flt_no=Transfer.flt_no;
      strcpy(pnr.market_flt.suffix,Transfer.suffix);
      pnr.market_flt.local_date=Transfer.local_date;
      strcpy(pnr.market_flt.airp_dep,Transfer.airp_dep);
      strcpy(pnr.market_flt.airp_arv,Transfer.airp_arv);
      strcpy(pnr.market_flt.subcl,Transfer.subcl);
    }
    else
    {
      //������ �� ����७��
      vector<TTransferItem>::const_iterator i;
      for(i=pnr.transfer.begin();i!=pnr.transfer.end();++i)
        if (Transfer.num==i->num) break;
      if (i!=pnr.transfer.end())
      {
        if (strcmp(i->airline,Transfer.airline)!=0||
            i->flt_no!=Transfer.flt_no||
            strcmp(i->suffix,Transfer.suffix)!=0||
            (i->airp_dep[0]!=0&&Transfer.airp_dep[0]!=0&&strcmp(i->airp_dep,Transfer.airp_dep)!=0)||
            i->local_date!=Transfer.local_date||
            (i->airp_arv[0]!=0&&Transfer.airp_arv[0]!=0&&strcmp(i->airp_arv,Transfer.airp_arv)!=0)||
            strcmp(i->subcl,Transfer.subcl)!=0)
          throw ETlgError("Different inbound/onward connection in group found");
      }
      else pnr.transfer.push_back(Transfer);
    };
    return;
  };
  if (strcmp(lexh,"W")==0)
  {
    if (!ne.bag.Empty())
      throw ETlgError("Duplicate pieces/weight data element");

    c=0;
    ne.bag.weight_unit[1]=0;
    res=sscanf(tlg.lex,".W/%c%[^.]",&ne.bag.weight_unit[0],tlg.lex);
    if (c!=0||res!=2)
      throw ETlgError("Wrong pieces/weight data element");
    if (strcmp(ne.bag.weight_unit,"L")!=0 &&
        strcmp(ne.bag.weight_unit,"K")!=0 &&
        strcmp(ne.bag.weight_unit,"P")!=0) throw ETlgError("Wrong pieces/weight indicator");
    if (strcmp(ne.bag.weight_unit,"P")==0)
    {
      c=0;
      res=sscanf(tlg.lex,"/%3lu%c",&ne.bag.bag_amount,&c);
      if (c!=0||res!=1||ne.bag.bag_amount<0) throw ETlgError("Wrong pieces/weight data element");
    }
    else
    {
      c=0;
      res=sscanf(tlg.lex,"/%3lu/%4lu%[^.]",&ne.bag.bag_amount,&ne.bag.bag_weight,tlg.lex);
      if (c!=0||res<2||ne.bag.bag_amount<0||ne.bag.bag_weight<0)
      {
        c=0;
        res=sscanf(tlg.lex,"/%3lu%c",&ne.bag.bag_amount,&c);
        if (c!=0||res!=1||ne.bag.bag_amount!=0)
          throw ETlgError("Wrong pieces/weight data element");
      };
      if (res==3)
      {
        c=0;
        res=sscanf(tlg.lex,"/%3lu%c",&ne.bag.rk_weight,&c);
        if (c!=0||res!=1||ne.bag.rk_weight<0) throw ETlgError("Wrong pieces/weight data element");

      };
    };
    pr_bag_info=true;
    return;
  };
  if (strcmp(lexh,"N")==0)
  {
    TTagItem tag;
    TBSMTag tagh;
    c=0;
    res=sscanf(tlg.lex,".N/%10[0-9]%3[0-9]/%3[A-Z�-��]%c",tagh.first_no,tagh.num,tag.airp_arv_final,&c);
    if (c!=0||res!=3)
    {
      c=0;
      res=sscanf(tlg.lex,".N/%9[A-Z�-��0-9]/%3[0-9]/%3[A-Z�-��]%c",tagh.first_no,tagh.num,tag.airp_arv_final,&c);
      if (c!=0||res!=3)
        throw ETlgError("Wrong baggage tag details element");
      if (strlen(tagh.first_no)<8)
        throw ETlgError("Wrong baggage tag ID number");

      lexh[0]=0;
      c=0;
      if (IsDigit(tagh.first_no[2]))
        res=sscanf(tagh.first_no,"%2[A-Z�-��0-9]%6[0-9]%c",tag.alpha_no,lexh,&c);
      else
        res=sscanf(tagh.first_no,"%3[A-Z�-��0-9]%6[0-9]%c",tag.alpha_no,lexh,&c);
      if (c!=0||res!=2||strlen(lexh)!=6)
        throw ETlgError("Wrong baggage tag ID number");
      StrToFloat(lexh,tag.numeric_no);
      StrToInt(tagh.num,tag.num);
      if (tag.num<1 || tag.num>999 || tag.numeric_no+tag.num-1>=1E6)
        throw ETlgError("Wrong number of consecutive tags");
    }
    else
    {
      if (strlen(tagh.first_no)!=10||strlen(tagh.num)!=3)
      {
        lexh[0]=0;
        c=0;
        res=sscanf(tlg.lex,".N/%12[0-9]/%3[A-Z�-��]%c",lexh,tag.airp_arv_final,&c); //����� �� �� ᮢᥬ �� �⠭�����
        if (c!=0||res!=2||strlen(lexh)<9) //�᫨ � ������ � ����� lexh �.�. ⮫쪮 11 ��� 12
          throw ETlgError("Wrong baggage tag details element");
        int first_no_len=strlen(lexh)-3;
        strncpy(tagh.first_no,lexh,first_no_len);
        tagh.first_no[first_no_len]=0;
        strcpy(tagh.num,lexh+first_no_len);
      };
      StrToFloat(tagh.first_no,tag.numeric_no);
      StrToInt(tagh.num,tag.num);
      if (tag.num<1 || tag.num>999 || tag.numeric_no+tag.num-1>=1E10)
        throw ETlgError("Wrong number of consecutive tags");
    };
    ne.tags.push_back(tag);
    pr_bag_info=true;
    return;
  };
  if (strcmp(lexh,"BG")==0)
  {
    lexh[0]=0;
    c=0;
    res=sscanf(tlg.lex,".BG/%3[0-9]%c",lexh,&c);
    if (c!=0||res!=1||strlen(lexh)!=3)
      throw ETlgError("Wrong baggage pooling element");
    StrToInt(lexh,ne.bag_pool);
    pr_bag_info=true;
    return;
  };
  if (strcmp(lexh,"U")==0)
  {
    parseAndBindSystemId(tlg, ne, tlg.lex+3);
  }
};

void ParseNameElement(const char* p, vector<string> &names, TElemPresence num_presence)
{
  char c,numh[4];
  int res,num;
  bool pr_num;
  TTlgParser tlg;
  char lexh[MAX_LEXEME_SIZE+1];

  names.clear();
  if (p==NULL) return;

  p=tlg.GetSlashedLexeme(p);
  if (p==NULL) return;
  //�롨ࠥ� ����� ���ᥬ�

  if (*tlg.lex!=0)
  {
    c=0;
    res=sscanf(tlg.lex,"%3[0-9]%[A-Z�-�� -]%c",numh,lexh,&c);
    if (c!=0||res!=2)
    {
      if (num_presence==epMandatory) throw ETlgError("Wrong format");
      c=0;
      res=sscanf(tlg.lex,"%[A-Z�-�� -]%c",lexh,&c);
      if (c!=0||res!=1) throw ETlgError("Wrong format");
      num=1;
      pr_num=false;
    }
    else
    {
      if (num_presence==epNone) throw ETlgError("Wrong format");
      if (StrToInt(numh,num)==EOF) throw ETlgError("Wrong format");
      if (num<1||num>999) throw ETlgError("Wrong format");
      pr_num=true;
    };
    names.push_back(lexh);
    p=tlg.GetSlashedLexeme(p);
  }
  else
  {
    p=tlg.GetSlashedLexeme(p);
    if (p==NULL) return;
    throw ETlgError("Wrong format");
  };

  while(p!=NULL)
  {
    if (*tlg.lex==0) throw ETlgError("Wrong format");
    c=0;
    res=sscanf(tlg.lex,"%*[A-Z�-�� -]%c",&c);
    if (c!=0||res>0) throw ETlgError("Wrong format");
    names.push_back(tlg.lex);
    num--;
    p=tlg.GetSlashedLexeme(p);
  };

  for(;num>0;num--) names.push_back("");

  if (pr_num && num<0) throw ETlgError("Too many names");
};

string OnlyAlphaInLexeme(string lex)
{
  string::iterator i=lex.begin();
  while(i!=lex.end())
  {
    if (IsLetter(*i))
      i++;
    else
      i=lex.erase(i);
  };
  return lex;
};


bool TNameElement::parsePassengerIDs(std::string& paxLevelElement, std::set<PaxItemsIterator>& applicablePaxItems)
{
  applicablePaxItems.clear();
  if (paxLevelElement.empty()) return false;
  //�饬 ��뫪� �� ���ᠦ�� �� ᮢ������� 䠬����
  vector<string> names;
  string::size_type pos=paxLevelElement.find_last_of('-');
  while(pos!=string::npos)
  {
    try
    {
      ParseNameElement(paxLevelElement.substr(pos+1).c_str(), names, epOptional);
      if (!names.empty() && names.front()==surname) break; //��諨
    }
    catch(const ETlgError &E) {};

    if (pos!=0)
      pos=paxLevelElement.find_last_of('-',pos-1);
    else
      pos=string::npos;
  }

  if (pos!=string::npos)
  {
    //��諨 ��뫪� �� ���ᠦ��
    for(vector<string>::const_iterator i=names.begin();i!=names.end();++i)
    {
      if (i==names.begin()) continue;  //�ய�᪠�� surname

      int k=0;
      for(;k<=1;k++)
      {
        PaxItemsIterator iPaxItem=pax.begin();
        for(; iPaxItem!=pax.end(); ++iPaxItem)
        {
          if ((k==0&&iPaxItem->name!=*i)||
              (k!=0&&OnlyAlphaInLexeme(iPaxItem->name)!=OnlyAlphaInLexeme(*i))) continue;
          if (applicablePaxItems.insert(iPaxItem).second) break;
        };
        if (iPaxItem!=pax.end()) break;
      };
      if (k>1)
      {
        //�� ��諨 name � ne
        applicablePaxItems.clear();
        return false;
      }
    }

    paxLevelElement.erase(pos); //���� ����砭�� ६�ન ��᫥ -
    return true;
  }
  else
  {
    //�� ��諨 ��뫪� �� ���ᠦ��
    if (pax.size()==1)
      applicablePaxItems.insert(pax.begin());
    else
      if (containsSinglePassenger())
      {
        for(PaxItemsIterator iPaxItem=pax.begin(); iPaxItem!=pax.end();++iPaxItem)
          if (!iPaxItem->isSeatBlocking())
          {
            applicablePaxItems.insert(iPaxItem);
            break;
          }
      }
    return !applicablePaxItems.empty();
  }
}

void BindRemarks(TTlgParser &tlg, TNameElement &ne)
{
  char c;
  int res;
  char rem_code[7],numh[4];
  int num;
  vector<TRemItem>::iterator iRemItem;

  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();++iRemItem)
  {
    TrimString(iRemItem->text);
    if (iRemItem->text.empty()) continue;
    tlg.GetWord(iRemItem->text.c_str());
    c=0;
    res=sscanf(tlg.lex,"%6[A-Z�-��0-9]%c",rem_code,&c);
    if (c!=0||res!=1) continue;

    if (strlen(rem_code)<=5) strcpy(iRemItem->code,rem_code);

    num=1;
    *numh=0;
    sscanf(rem_code,"%3[0-9]%s",numh,rem_code);
    if (*numh!=0)
    {
      StrToInt(numh,num);
      if (num<=0) num=1;
    };

    if (strcmp(rem_code,"CHD")==0||
        strcmp(rem_code,"INF")==0||
        strcmp(rem_code,"VIP")==0) strcpy(iRemItem->code,rem_code);
  };
  //���஡����� �ਢ易�� ६�ન � ������� ���ᠦ�ࠬ
  set<TNameElement::PaxItemsIterator> paxItems;
  for(iRemItem=ne.rem.begin(); iRemItem!=ne.rem.end();)
  {
    if (ne.parsePassengerIDs(iRemItem->text, paxItems))
    {
      for(const auto& i : paxItems) i->rem.push_back(*iRemItem);
      iRemItem=ne.rem.erase(iRemItem);
    }
    else
      ++iRemItem;
  };
};

bool ParseDOCSRem(TTlgParser &tlg, TDateTime scd_local, const std::string &rem_text, TDocItem &doc);
bool ParseDOCORem(TTlgParser &tlg, TDateTime scd_local, const std::string &rem_text, TDocoItem &doc);
bool ParseDOCARem(TTlgParser &tlg, const string &rem_text, TDocaItem &doca);
bool ParseCHKDRem(TTlgParser &tlg, const string &rem_text, TCHKDItem &chkd);
bool ParseASVCRem(TTlgParser &tlg, const string &rem_text, TASVCItem &asvc);
bool ParseOTHS_DOCSRem(TTlgParser &tlg, const string &rem_text, TDocExtraItem &doc);
bool ParseOTHS_FQTSTATUSRem(TTlgParser &tlg, const string &rem_text, TFQTExtraItem &fqt);
bool ParseSeatBlockingRem(TTlgParser &tlg, const string &rem_text, TSeatBlockingRem &rem);

void bindSystemId(const PassengerSystemId& item, TPaxItem& paxItem)
{
  if (item.infIndicatorExists())
  {
    if (paxItem.inf.size()==1)
    {
      paxItem.inf.front().add(item);
    }
  }
  else
  {
    paxItem.add(item);
  }
}

template<class ItemT>
void BindDetailRem(TRemItem &remItem, bool isGrpRem, TPaxItem &paxItem,
                   const ItemT &item)
{
  if (item.infIndicatorExists())
  {
    if (paxItem.inf.size()==1)
    {
      paxItem.inf.front().add(item);
      paxItem.inf.front().add(remItem);
      remItem.text.clear();
    }
  }
  else
  {
    paxItem.add(item);
    if (isGrpRem)
    {
      paxItem.add(remItem);
      remItem.text.clear();
    };
  }
}

void TInfList::removeIfExistsIn(const TInfList &infs)
{
  for(TInfList::iterator i=begin();i!=end();)
  {
    //�஢�ਬ ���� �� 㦥 ॡ���� � ⠪�� ������ � 䠬����� � infs
    //(���� �� �㡫�஢���� INF � INFT)
    TInfList::const_iterator j;
    for(j=infs.begin();j!=infs.end();++j)
      if (i->surname==j->surname && i->name==j->name) break;
    if (j!=infs.end())
    {
      //�㡫�஢���� INF. 㤠��� �� inf
      i=erase(i);
      continue;
    };
    ++i;
  };
}

void TInfList::removeEmpty()
{
  for(TInfList::iterator i=begin();i!=end();)
  {
    if (i->Empty())
    {
      i=erase(i);
      continue;
    }
    ++i;
  }
}

void TInfList::removeDup()
{
  bool notEmptyExists=false;
  for(const TInfItem &i : *this)
    if (!i.Empty())
    {
      notEmptyExists=true;
      break;
    }

  for(TInfList::iterator i=begin();i!=end();)
  {
    if (i->Empty() && notEmptyExists)
    {
      i=erase(i);
      continue;
    }
    ++i;
  }
}

void TInfList::setSurnameIfEmpty(const std::string &surname)
{
  for(TInfItem &i : *this)
    if (i.Empty()) i.surname=surname;
}

void TSeatsBlockingList::toDB(const int& paxId) const
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "BEGIN "
    "  UPDATE crs_seats_blocking SET pr_del=0 "
    "  WHERE pax_id=:pax_id AND surname=:surname AND name=:name AND pr_del<>0 AND rownum<2; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO crs_seats_blocking(seat_id, surname, name, pax_id, pr_del) "
    "    VALUES(pax_id.nextval, :surname, :name, :pax_id, 0);"
    "  END IF; "
    "END; ";
  Qry.DeclareVariable("surname", otString);
  Qry.DeclareVariable("name", otString);
  paxId!=NoExists?
    Qry.CreateVariable("pax_id", otInteger, paxId):
    Qry.CreateVariable("pax_id", otInteger, FNull);
  for(const TSeatsBlockingItem& i : *this)
  {
    Qry.SetVariable("surname", i.surname);
    Qry.SetVariable("name", i.name);
    Qry.Execute();
  }
}

void TSeatsBlockingList::fromDB(const int& paxId)
{
  clear();

  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT * FROM crs_seats_blocking WHERE pax_id=:pax_id AND pr_del=0 ORDER BY seat_id";
  Qry.CreateVariable("pax_id", otInteger, paxId);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    emplace_back(Qry.FieldAsString("surname"),
                 Qry.FieldAsString("name"));
}

void TSeatsBlockingList::replace(const TSeatsBlockingList& src, bool isSpecial, long& seats)
{
  for(TSeatsBlockingList::iterator i=begin(); i!=end();)
    if (i->isSpecial()==isSpecial)
    {
      i=erase(i);
      if (!i->isCBBG()) seats--;
    }
    else
      ++i;

  for(const TSeatsBlockingItem& i : src)
  {
    if (i.isSpecial()==isSpecial)
    {
      push_back(i);
      if (!i.isCBBG()) seats++;
    }
  }
}

void ParseRemarks(const vector< pair<string,int> > &seat_rem_priority,
                  TTlgParser &tlg, const TDCSHeadingInfo &info, TPnrItem &pnr, TNameElement &ne)
{
  vector<TRemItem>::iterator iRemItem;
  vector<TPaxItem>::iterator iPaxItem,iPaxItem2;
  vector<TNameElement>::iterator iNameElement;

  //᭠砫� �஡����� �� ६�ઠ� ������� �� ���ᠦ�஢ (��-�� ����஢ ����)
  for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
  {
    for(int pass=0;pass<=2;pass++)
    {
      if (pass==0 || pass==1)
      {
        for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();iRemItem++)
        {
          if (iRemItem->text.empty()) continue;

          if (pass==0)
          {
            if (strcmp(iRemItem->code,"CHD")==0||
                strcmp(iRemItem->code,"CHLD")==0)
            {
              iPaxItem->pers_type=child;
              continue;
            };
            if (strcmp(iRemItem->code,"INF")==0||
                strcmp(iRemItem->code,"INFT")==0)
            {
                  //�⤥��� ���ᨢ ��� INF
              TInfList inf;
              if (ParseINFRem(tlg,iRemItem->text,inf))
              {
                inf.removeEmpty();
                inf.removeIfExistsIn(iPaxItem->inf);
                iPaxItem->inf.insert(iPaxItem->inf.end(),inf.begin(),inf.end());
                iPaxItem->inf.removeDup();
              };
              continue;
            };
          };
          if (pass==1)
          {
            if (strcmp(iRemItem->code,"DOCS")==0 ||
                strcmp(iRemItem->code,"PSPT")==0)
            {
              TDocItem doc;
              if (ParseDOCSRem(tlg,info.flt.scd,iRemItem->text,doc))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, doc);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"DOCO")==0)
            {
              TDocoItem doc;
              if (ParseDOCORem(tlg,info.flt.scd,iRemItem->text,doc))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, doc);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"DOCA")==0)
            {
              TDocaItem doca;
              if (ParseDOCARem(tlg,iRemItem->text,doca))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, doca);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"TKNA")==0||
                strcmp(iRemItem->code,"TKNE")==0)
            {
              TTKNItem tkn;
              if (ParseTKNRem(tlg,iRemItem->text,tkn))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, tkn);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"CHKD")==0)
            {
              TCHKDItem chkd;
              if (ParseCHKDRem(tlg,iRemItem->text,chkd))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, chkd);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"ASVC")==0)
            {
              TASVCItem asvc;
              if (ParseASVCRem(tlg,iRemItem->text,asvc))
              {
                BindDetailRem(*iRemItem, false, *iPaxItem, asvc);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"FQTV")==0||
                strcmp(iRemItem->code,"FQTR")==0||
                strcmp(iRemItem->code,"FQTU")==0||
                strcmp(iRemItem->code,"FQTS")==0)
            {
              TFQTItem fqt;
              if (ParseFQTRem(tlg,iRemItem->text,fqt))
                iPaxItem->fqt.push_back(fqt);
              continue;
            };

            if (strcmp(iRemItem->code,"OTHS")==0)
            {
              TDocExtraItem doc;
              if (ParseOTHS_DOCSRem(tlg,iRemItem->text,doc))
              {
                if (doc.valid())
                  iPaxItem->doc_extra.insert(make_pair(doc.no, doc));
                continue;
              };
              TFQTExtraItem fqt;
              if (ParseOTHS_FQTSTATUSRem(tlg,iRemItem->text,fqt))
              {
                if (!fqt.tier_level.empty())
                  iPaxItem->fqt_extra.insert(fqt);
                continue;
              };
              continue;
            };
          };
        };
      };
      if (pass==2)
      {
        for(vector< pair<string,int> >::const_iterator r=seat_rem_priority.begin();
                                                       r!=seat_rem_priority.end(); r++ )
        {
          for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();iRemItem++)
          {
            if (iRemItem->text.empty()) continue;
            if (iPaxItem->emdRequired(iRemItem->code)) continue;

            if (iRemItem->code!=r->first) continue;
            TSeatRanges seats;
            if (ParseSEATRem(tlg,iRemItem->text,seats))
            {
              sort(seats.begin(),seats.end());
              iPaxItem->seatRanges.insert(iPaxItem->seatRanges.end(),seats.begin(),seats.end());

              //�᫨ ���� �� ��।�����, ���஡����� �ਢ易��
              ne.setNotUsedSeat(seats, *iPaxItem, false);
            };

          };
        };
      };

    };

    for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();)
    {
      if (iRemItem->text.empty())
        iRemItem=iPaxItem->rem.erase(iRemItem);
      else
        iRemItem++;
    };
  };

  TInfList infs;
  for(int pass=0;pass<=2;pass++)
  {
    if (pass==0||pass==1)
    {
      //�஡����� �� ६�ઠ� ��㯯�
      for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
      {
        if (iRemItem->text.empty()) continue;

        if (pass==0)
        {
          if (strcmp(iRemItem->code,"CHD")==0||
              strcmp(iRemItem->code,"CHLD")==0)
          {
            vector<TChdItem> chd;
            if (ParseCHDRem(tlg,iRemItem->text,chd))
            {
              for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
              {
                for(vector<TChdItem>::iterator i=chd.begin();i!=chd.end();i++)
                  if (i->first==ne.surname &&
                      (i->second==iPaxItem2->name ||
                       OnlyAlphaInLexeme(i->second)==OnlyAlphaInLexeme(iPaxItem2->name)))
                  {
                    iPaxItem2->pers_type=child;
                    break;
                  };
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"INF")==0||
              strcmp(iRemItem->code,"INFT")==0)
          {
                //�⤥��� ���ᨢ ��� INF
            TInfList inf;
            if (ParseINFRem(tlg,iRemItem->text,inf))
            {
              inf.removeEmpty();
              if (ne.pax.size()==1 && ne.pax.begin()->inf.empty() &&
                  inf.size()==1)
              {
                //�������筠� �ਢ離� � �����⢥����� ���ᠦ���
                ne.pax.begin()->inf.insert(ne.pax.begin()->inf.end(),inf.begin(),inf.end());
              }
              else
              {
                inf.removeIfExistsIn(infs);
                infs.insert(infs.end(),inf.begin(),inf.end());
              };
            };
            continue;
          };
        };
        if (pass==1)
        {
          if (strcmp(iRemItem->code,"DOCS")==0 ||
              strcmp(iRemItem->code,"PSPT")==0)
          {
            if (ne.pax.size()==1)
            {
              TDocItem doc;
              if (ParseDOCSRem(tlg,info.flt.scd,iRemItem->text,doc))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), doc);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"DOCO")==0)
          {
            if (ne.pax.size()==1)
            {
              TDocoItem doc;
              if (ParseDOCORem(tlg,info.flt.scd,iRemItem->text,doc))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), doc);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"DOCA")==0)
          {
            if (ne.pax.size()==1)
            {
              TDocaItem doca;
              if (ParseDOCARem(tlg,iRemItem->text,doca))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), doca);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"TKNA")==0||
              strcmp(iRemItem->code,"TKNE")==0)
          {
            if (ne.pax.size()==1)
            {
              TTKNItem tkn;
              if (ParseTKNRem(tlg,iRemItem->text,tkn))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), tkn);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"CHKD")==0)
          {
            if (ne.pax.size()==1)
            {
              TCHKDItem chkd;
              if (ParseCHKDRem(tlg,iRemItem->text,chkd))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), chkd);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"ASVC")==0)
          {
            if (ne.pax.size()==1)
            {
              TASVCItem asvc;
              if (ParseASVCRem(tlg,iRemItem->text,asvc))
              {
                BindDetailRem(*iRemItem, true, *(ne.pax.begin()), asvc);
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"FQTV")==0||
              strcmp(iRemItem->code,"FQTR")==0||
              strcmp(iRemItem->code,"FQTU")==0||
              strcmp(iRemItem->code,"FQTS")==0)
          {
            if (ne.pax.size()==1)
            {
              TFQTItem fqt;
              if (ParseFQTRem(tlg,iRemItem->text,fqt))
              {
                ne.pax.begin()->fqt.push_back(fqt);
                ne.pax.begin()->rem.push_back(*iRemItem);
                iRemItem->text.clear();
              };
            };
            continue;
          };

          if (strcmp(iRemItem->code,"OTHS")==0)
          {
            if (ne.pax.size()==1)
            {
              TDocExtraItem doc;
              if (ParseOTHS_DOCSRem(tlg,iRemItem->text,doc))
              {
                if (doc.valid())
                  ne.pax.begin()->doc_extra.insert(make_pair(doc.no, doc));
                ne.pax.begin()->rem.push_back(*iRemItem);
                iRemItem->text.clear();
                continue;
              };
              TFQTExtraItem fqt;
              if (ParseOTHS_FQTSTATUSRem(tlg,iRemItem->text,fqt))
              {
                if (!fqt.tier_level.empty())
                  ne.pax.begin()->fqt_extra.insert(fqt);
                ne.pax.begin()->rem.push_back(*iRemItem);
                iRemItem->text.clear();
                continue;
              };
            };
            continue;
          };
        };
      };
    };
    if (pass==2)
    {
      for(vector< pair<string,int> >::const_iterator r=seat_rem_priority.begin();
                                                     r!=seat_rem_priority.end(); r++ )
      {
        //�஡����� �� ६�ઠ� ��㯯�
        for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
        {
          if (iRemItem->text.empty()) continue;

          if (iRemItem->code!=r->first) continue;
          TSeatRanges seats;
          if (ParseSEATRem(tlg,iRemItem->text,seats))
          {
            sort(seats.begin(),seats.end());
            ne.seatRanges.insert(ne.seatRanges.end(),seats.begin(),seats.end());

            //���஡㥬 �ਢ易��
            for(TSeatRanges::iterator i=seats.begin();i!=seats.end();i++)
            {
              TSeat seat=i->first;
              do
              {
                //������� ���� �� seat, �᫨ �� ������� �� � ���� ��㣮�� �� ��㯯�
                if (!pnr.seatIsUsed(seat))
                {
                  //���� �� � ���� �� ��㯯� �� �������
                  //���� ���ᠦ��, ���஬� �� ���⠢���� ����
                  if (strcmp(iRemItem->code,"EXST")==0)
                  {
                    for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                      if (iPaxItem2->seat.Empty() &&
                          iPaxItem2->name=="EXST" &&
                          !iPaxItem2->emdRequired(iRemItem->code))
                      {
                        iPaxItem2->seat=seat;
                        break;
                      };
                    if (iPaxItem2!=ne.pax.end()) continue;
                  };

                  //� ����� ��।� ��।��塞 ���� ⥪�饬� NameElement
                  for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                    if (iPaxItem2->seat.Empty() &&
                        !iPaxItem2->emdRequired(iRemItem->code))
                    {
                      iPaxItem2->seat=seat;
                      strcpy(iPaxItem2->seat_rem,i->rem);
                      break;
                    };
                  if (iPaxItem2!=ne.pax.end()) continue;

                  //�஡������ �� �ᥩ ��㯯�
                  for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
                  {
                    for(iPaxItem2=iNameElement->pax.begin();iPaxItem2!=iNameElement->pax.end();iPaxItem2++)
                      if (iPaxItem2->seat.Empty() &&
                          !iPaxItem2->emdRequired(iRemItem->code))
                      {
                        iPaxItem2->seat=seat;
                        strcpy(iPaxItem2->seat_rem,i->rem);
                        break;
                      };
                    if (iPaxItem2!=iNameElement->pax.end()) break;
                  };
                };
              }
              while (NextSeatInRange(*i,seat));
            };
          };
        };
      };
    };
  };
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();)
  {
    if (iRemItem->text.empty())
      iRemItem=ne.rem.erase(iRemItem);
    else
      iRemItem++;
  };

  //�᪨���� ���ਢ易���� ��
  for(vector<TInfItem>::iterator iInfItem=infs.begin();iInfItem!=infs.end();iInfItem++)
  {
    for(int k=0;k<=1;k++)
    {
      //0. �ਢ�뢠�� � ��ࢮ�� �� ᢮������� �� inf
      //1. �ਢ�뢠�� � ��ࢮ�� ��
      vector<TPaxItem>::iterator i=ne.pax.begin();
      for(;i!=ne.pax.end();i++)
      {
        if ((k==0 && i->pers_type==adult && i->inf.empty()) ||
            (k==1 && i->pers_type==adult))
        {
          i->inf.push_back(*iInfItem);
          break;
        };
      };
      if (i!=ne.pax.end()) break;
    };
  };
  for(TPaxItem &i : ne.pax)
  {
    i.inf.removeDup();
    i.inf.setSurnameIfEmpty(ne.surname);
  };
};

bool TPaxItem::emdRequired(const std::string& ssr_code) const
{
  if (ssr_code.empty()) return false;
  for(const TASVCItem& i : asvc)
    if (i.emdRequired() && ssr_code==i.ssr_code) return true;

  return false;
}

void TPaxItem::removeNotConfimedSSRs()
{
  for(vector<TRemItem>::const_iterator iRemItem=rem.begin(); iRemItem!=rem.end(); )
    if (emdRequired(iRemItem->code))
    {
      LogTrace(TRACE5) << __FUNCTION__ << ": " << iRemItem->text;
      iRemItem=rem.erase(iRemItem);
    }
    else
      ++iRemItem;
}

void TNameElement::removeNotConfimedSSRs()
{
  for(TPaxItem& p : pax) p.removeNotConfimedSSRs();
}

bool TPaxItem::isSeatBlockingRem(const string &rem_code)
{
  return rem_code=="EXST" ||
         rem_code=="STCR" ||
         rem_code=="CBBG";
}

boost::optional<PaxId_t> TPaxItem::getNotUsedSeatBlockingId(const PaxId_t& paxId)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT seat_id "
    "FROM crs_seats_blocking "
    "WHERE pax_id=:pax_id AND pr_del=0 AND "
    "      NOT EXISTS(SELECT crs_pax.pax_id FROM crs_pax WHERE crs_pax.pax_id=crs_seats_blocking.seat_id AND crs_pax.pr_del=0) "
    "ORDER BY seat_id ";
  Qry.CreateVariable("pax_id", otInteger, paxId.get());
  Qry.Execute();
  if (Qry.Eof) return boost::none;
  return PaxId_t(Qry.FieldAsInteger("seat_id"));
}

std::map<PaxId_t, TSeatsBlockingItem> TPaxItem::getAndLockSeatBlockingItems(const PaxId_t& paxId)
{
  std::map<PaxId_t, TSeatsBlockingItem> result;

  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT * FROM crs_seats_blocking WHERE pax_id=:pax_id AND pr_del=0 FOR UPDATE";
  Qry.CreateVariable("pax_id", otInteger, paxId.get());
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    result.emplace(PaxId_t(Qry.FieldAsInteger("seat_id")),
                   TSeatsBlockingItem(Qry.FieldAsString("surname"),
                                      Qry.FieldAsString("name")));

  return result;
}

std::set<PaxId_t> TPaxItem::getAndLockInfantIds(const PaxId_t &paxId)
{
  std::set<PaxId_t> result;

  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT inf_id FROM crs_inf WHERE pax_id=:pax_id FOR UPDATE";
  Qry.CreateVariable("pax_id", otInteger, paxId.get());
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    result.emplace(Qry.FieldAsInteger("inf_id"));

  return result;
}

void TPaxItem::bindSeatsBlocking(const TNameElement& ne,
                                 const std::string& remCode,
                                 TSeatsBlockingList& srcSeatsBlocking)
{
  if (!isSeatBlockingRem(remCode)) return;
  for(bool isSpecialPass : {false, true})
  {
    for(TSeatsBlockingList::iterator i=srcSeatsBlocking.begin(); i!=srcSeatsBlocking.end(); ++i)
    {
      TSeatsBlockingItem& seatsBlockingItem=*i;

      if (seatsBlockingItem.isSpecial()!=isSpecialPass) continue;

      if ((seatsBlockingItem.isSpecial() || seatsBlockingItem.surname==ne.surname) &&
          seatsBlockingItem.name==remCode)
      {
        if (!seatsBlockingItem.isCBBG())
        {
          if (seats>=3) return; //����� ��� ����� �ॢ���� ���-�� ���� 3
          seats++;
        }

        if (!seatsBlockingItem.isSpecial())
        {
          //��ॡ�ᨬ ६�ન �� ���ᠦ��
          rem.insert(rem.end(),
                     seatsBlockingItem.rem.begin(),seatsBlockingItem.rem.end());
        }
        seatsBlockingItem.rem.clear();

        seatsBlocking.push_back(seatsBlockingItem);
        srcSeatsBlocking.erase(i);
        return;
      }
    }
  }
}

void TPaxItem::getTknNumbers(std::list<std::string>& result) const
{
  for(const TTKNItem& i : tkn)
    if (*i.ticket_no!=0 &&
        find(result.begin(), result.end(), i.ticket_no)==result.end())
      result.push_back(i.ticket_no);
}

void TPaxItem::moveTknWithNumber(const std::string& no, std::vector<TTKNItem>& dest)
{
  dest.clear();

  for(vector<TTKNItem>::iterator iTkn=tkn.begin(); iTkn!=tkn.end();)
  {
    if (iTkn->ticket_no==no)
    {
      dest.push_back(*iTkn);
      iTkn=tkn.erase(iTkn);
      continue;
    }
    ++iTkn;
  }
}

bool TNameElement::containsSinglePassenger() const
{
  int count=0;
  for(const TPaxItem& paxItem : pax)
    if (!paxItem.isSeatBlocking()) count++;
  return count==1;
}

void TPaxItem::fillSeatBlockingRemList(TTlgParser &tlg)
{
  for(const TRemItem& remItem : rem)
  {
    if (!TPaxItem::isSeatBlockingRem(remItem.code)) continue;

    TSeatBlockingRem seatBlockingRem;
    if (ParseSeatBlockingRem(tlg,remItem.text,seatBlockingRem))
      seatBlockingRemList.push_back(seatBlockingRem);
  }
}

bool TPaxItem::dontSaveToDB(const TNameElement& ne) const
{
  return (ne.surname=="NONAMES" && name.empty()) ||
         (ne.isSpecial() && name=="NONAMES");
}

void TNameElement::bindSystemIds()
{
  for(TPaxItem& p : pax)
    for(const PassengerSystemId& systemId : p.systemIds)
      bindSystemId(systemId, p);
}

void TNameElement::fillSeatBlockingRemList(TTlgParser &tlg)
{
  for(TPaxItem& paxItem : pax)
    paxItem.fillSeatBlockingRemList(tlg);
}

void TNameElement::separateSeatsBlocking(TSeatsBlockingList& dest)
{
  for(vector<TPaxItem>::iterator iPax=pax.begin();iPax!=pax.end();)
  {
    if (iPax->isSeatBlocking())
    {
      iPax->pers_type=NoPerson;
      dest.emplace_back(surname, *iPax);
      iPax=pax.erase(iPax);
      continue;
    }
    ++iPax;
  }
}

void TPnrItem::separateSeatsBlocking()
{
  for(TNameElement& nameElement : ne)
  {
    TSeatsBlockingList& dest=
      seatsBlocking.emplace(nameElement.indicator, TSeatsBlockingList()).first->second;

    nameElement.separateSeatsBlocking(dest);
  }
}

void TNameElement::bindSeatsBlocking(const std::string& remCode,
                                     TSeatsBlockingList& srcSeatsBlocking)
{
  for(TPaxItem& paxItem : pax)
    for(const TSeatBlockingRem& remItem : paxItem.seatBlockingRemList)
    {
      if (remItem.rem_code!=remCode) continue;
      if (!remItem.isHKReservationsStatus()) continue;
      if (remItem.numberOfSeats==NoExists) continue;
      for(int i=0; i<remItem.numberOfSeats; i++)
      {
        paxItem.bindSeatsBlocking(*this, remItem.rem_code, srcSeatsBlocking);
        if (remItem.isSTCR()) break; //���� �����뢠�� �� STCR HK1, �� ���㬠�� �᫨ HK �㤥� �⮡ࠦ��� ॠ�쭮� ���-�� ���. ����
      }
    }
}

void TPaxItem::setSomeDataForSeatsBlocking(const boost::optional<PaxId_t>& paxId, const TNameElement& ne)
{
  if (ne.indicator==CHG && paxId)
  {
    TSeatsBlockingList seatsBlockingFromDB;
    seatsBlockingFromDB.fromDB(paxId.get().get());
    long newSeats=seats;
    seatsBlocking.replace(seatsBlockingFromDB, true, newSeats);
    if (newSeats>=1 && newSeats<=3)
      seats=newSeats;
    else
      LogError(STDLOG) << __FUNCTION__
                       << ": paxId=" << paxId.get()
                       << ", seats=" << seats
                       << ", newSeats=" << newSeats;
  }
  setSomeDataForSeatsBlocking(ne);
}

void TPaxItem::setSomeDataForSeatsBlocking(const TNameElement& ne)
{
  if (seatsBlocking.empty()) return;

  list<std::string> tknNumbers;
  getTknNumbers(tknNumbers);

  list<std::string>::const_iterator iTkn=tknNumbers.begin();
  if (iTkn!=tknNumbers.end()) ++iTkn; //���� ����� � ᯨ᪥ ��� ���ᠦ��

  for(TSeatsBlockingItem& seatsBlockingItem : seatsBlocking)
  {
    if (!seatsBlockingItem.isCBBG()) continue;
    if (seatsBlockingItem.tkn.empty() && iTkn!=tknNumbers.end())
    {
      moveTknWithNumber(*iTkn, seatsBlockingItem.tkn);
      ++iTkn;
    }
    ne.setNotUsedSeat(seatRanges, seatsBlockingItem, true);
  }
}

bool TNameElement::seatIsUsed(const TSeat& seat) const
{
  for(const TPaxItem& p : pax)
    if (!p.seat.Empty() && p.seat==seat) return true;
  return false;
}

void TNameElement::setNotUsedSeat(TSeatRanges& seats, TPaxItem& paxItem, bool moveSeat) const
{
  if (!paxItem.seat.Empty()) return;

  TSeatRanges singleSeats;

  for(const TSeatRange& range : seats)
  {
    TSeat seat=range.first;
    do
    {
      if (moveSeat)
        singleSeats.emplace_back(seat, seat, range.rem);
      else
        if (!seatIsUsed(seat))
        {
          paxItem.seat=seat;
          strcpy(paxItem.seat_rem, range.rem);
          return;
        }
    }
    while (NextSeatInRange(range, seat));
  }

  if (moveSeat)
  {
    seats.clear();
    for(const TSeatRange& range : singleSeats)
    {
      TSeat seat=range.first;
      if (paxItem.seat.Empty() && !seatIsUsed(seat))
      {
        paxItem.seat=seat;
        strcpy(paxItem.seat_rem, range.rem);
        paxItem.seatRanges.push_back(range);
      }
      else
        seats.push_back(range);
    }
  }
}

void TPnrItem::bindSeatsBlocking()
{
  for(const string& remCode : {"STCR", "STCR", "EXST", "CBBG"}) //2 ࠧ� STCR �� �� �訡��
    for(TNameElement& nameElement : ne)
    {
      TSeatsBlockingList& srcSeatsBlocking=seatsBlocking.emplace(nameElement.indicator, TSeatsBlockingList()).first->second;
      nameElement.bindSeatsBlocking(remCode, srcSeatsBlocking);
    }
}

bool TPnrItem::seatIsUsed(const TSeat& seat) const
{
  for(const TNameElement& nameElement : ne)
    if (nameElement.seatIsUsed(seat)) return true;
  return false;
}

void ParseSeatRange(string str, TSeatRanges &ranges, bool usePriorContext)
{
  char c;
  int res;
  TSeatRange seatr,seatr2;
  char lexh[MAX_LEXEME_SIZE+1];

  static TSeat priorMaxSeat;

  ranges.clear();

  do
  {
    //�஢�ਬ �������� (����. 21C-24F )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]%1[A-Z�-��]-%3[0-9]%1[A-Z�-��]%c",
               seatr.first.row,seatr.first.line,
               seatr.second.row,seatr.second.line,
               &c);
    if (c==0 && res==4)
    {
      NormalizeSeatRange(seatr);
      if (strcmp(seatr.first.row,seatr.second.row)>0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());

      //ࠧ��ꥬ �� ������������
      if (strcmp(seatr.first.row,seatr.second.row)==0)
      {
        //���� ��
        if (strcmp(seatr.first.line,seatr.second.line)>0)
          throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());

        ranges.push_back(seatr);
      }
      else
      {
        //����� �冷�
        if (strcmp(seatr.first.line,FirstNormSeatLine(seatr2.first).line)!=0)
        {
          //���� ��
          strcpy(seatr2.first.row,seatr.first.row);
          strcpy(seatr2.second.row,seatr.first.row);
          strcpy(seatr2.first.line,seatr.first.line);
          LastNormSeatLine(seatr2.second);

          if (!NextNormSeatRow(seatr.first))
            throw EConvertError("ParseSeatRange: error in procedure norm_iata_row (range %s)", str.c_str());

          ranges.push_back(seatr2);
        };
        if (strcmp(seatr.second.line,LastNormSeatLine(seatr2.first).line)!=0)
        {
          //��᫥���� ��
          strcpy(seatr2.first.row,seatr.second.row);
          strcpy(seatr2.second.row,seatr.second.row);
          FirstNormSeatLine(seatr2.first);
          strcpy(seatr2.second.line,seatr.second.line);

          if (!PriorNormSeatRow(seatr.second))
            throw EConvertError("ParseSeatRange: error in procedure norm_iata_row (range %s)", str.c_str());

          ranges.push_back(seatr2);
        };
        if (strcmp(seatr.first.row,seatr.second.row)<=0)
        {
          strcpy(seatr2.first.row,seatr.first.row);
          strcpy(seatr2.second.row,seatr.second.row);
          FirstNormSeatLine(seatr2.first);
          LastNormSeatLine(seatr2.second);

          ranges.push_back(seatr2);
        };
      };
      break;
    };

    //�஢�ਬ ���� ���� (����. 21-24A-D )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]-%3[0-9]%1[A-Z�-��]-%1[A-Z�-��]%c",
               seatr.first.row,seatr.second.row,
               seatr.first.line,seatr.second.line,
               &c);
    if (c==0 && res==4)
    {
      NormalizeSeatRange(seatr);
      if (strcmp(seatr.first.row,seatr.second.row)>0 ||
          strcmp(seatr.first.line,seatr.second.line)>0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());
      ranges.push_back(seatr);
      break;
    };

    //�஢�ਬ ���� ���� ����� ����� (����. 21-23A )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]-%3[0-9]%1[A-Z�-��]%c",
               seatr.first.row,seatr.second.row,
               seatr.first.line,
               &c);
    if (c==0 && res==3)
    {
      strcpy(seatr.second.line,seatr.first.line);
      NormalizeSeatRange(seatr);
      if (strcmp(seatr.first.row,seatr.second.row)>0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());
      ranges.push_back(seatr);
      break;
    };

    //�஢�ਬ ���� ���� ������ �鸞 (����. 21A-C )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]%1[A-Z�-��]-%1[A-Z�-��]%c",
               seatr.first.row,
               seatr.first.line,seatr.second.line,
               &c);
    if (c==0 && res==3)
    {
      strcpy(seatr.second.row,seatr.first.row);
      NormalizeSeatRange(seatr);
      if (strcmp(seatr.first.line,seatr.second.line)>0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());
      ranges.push_back(seatr);
      break;
    };

    //�஢�ਬ ROW
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]%3[ROW]%c",seatr.first.row,lexh,&c);
    if (c==0 && res==2 && strcmp(lexh,"ROW")==0)
    {
      strcpy(seatr.second.row,seatr.first.row);
      FirstNormSeatLine(seatr.first);
      LastNormSeatLine(seatr.second);
      NormalizeSeatRange(seatr);
      ranges.push_back(seatr);
      break;
    };

    if (str=="NIL") break;

    if (str=="ALL")
    {
      FirstNormSeatRow(seatr.first);
      FirstNormSeatLine(seatr.first);
      LastNormSeatRow(seatr.second);
      LastNormSeatLine(seatr.second);
      NormalizeSeatRange(seatr);
      ranges.push_back(seatr);
      break;
    };

    if (str=="REST")
    {
      if (usePriorContext &&
          !priorMaxSeat.Empty()&&
          NextNormSeat(priorMaxSeat) )
      {
        seatr.first=priorMaxSeat;
        LastNormSeatRow(seatr.second);
        LastNormSeatLine(seatr.second);
        NormalizeSeatRange(seatr);
        ranges.push_back(seatr);
      };
      break;
    };

    //�஢�ਬ ����᫥��� (����. 4C5A6B20AC21ABD)
    //�����, �� ��� ���� ࠧ��� ���� ��᫥���� � ParseSeatRange!
    if (str.size()>=sizeof(lexh))
      throw EConvertError("ParseSeatRange: too long range lexeme %s", str.c_str());
    strcpy(lexh,str.c_str());

    vector<TSeat> seats;
    TSeat seat;
    char *p=lexh;
    do
    {
      res=sscanf(p,"%3[0-9]%s",seat.row,lexh);
      if (res!=2)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());

      for(p=lexh;IsUpperLetter(*p)&&*p!=0;p++)
      {
        seat.line[0]=*p;
        seat.line[1]=0;
        NormalizeSeat(seat);
        seats.push_back(seat);
      };

      if (p==lexh&&*p!=0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());
    }
    while(*p!=0);
    //㯮�冷稬 seats � ���஡㥬 �८�ࠧ����� � ���������
    sort(seats.begin(),seats.end());
    seatr.second.Clear();
    for(vector<TSeat>::iterator i=seats.begin();i!=seats.end();i++)
    {
      strcpy(seat.row,i->row);
      strcpy(seat.line,i->line);

      if (seatr.second.Empty()||
          strcmp(seatr.second.row,seat.row)!=0||
          (strcmp(seatr.second.line,seat.line)!=0&&
           (!PriorNormSeatLine(seat)||
            strcmp(seatr.second.line,seat.line)!=0)))
      {
        if (!seatr.second.Empty()) ranges.push_back(seatr);
        strcpy(seatr.first.row,i->row);
        strcpy(seatr.first.line,i->line);
      };
      strcpy(seatr.second.row,i->row);
      strcpy(seatr.second.line,i->line);
    };
    if (!seatr.second.Empty()) ranges.push_back(seatr);
  }
  while (false);

  priorMaxSeat.Clear();
  for(TSeatRanges::iterator i=ranges.begin();i!=ranges.end();i++)
  {
    if (priorMaxSeat<i->second) priorMaxSeat=i->second;
  };
};

bool ParseSEATRem(TTlgParser &tlg, const string &rem_text, TSeatRanges &seats)
{
  char c;
  int res;
  char rem_code[6];
  pair<char[4],char[4]> row;
  pair<char,char> line;

  const char *p=rem_text.c_str();

  seats.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  if (isSeatRem(rem_code))
  {
    bool usePriorContext=false;
    TSeatRanges ranges;
    while((p=tlg.GetLexeme(p))!=NULL)
    {
      try
      {
        ParseSeatRange(tlg.lex,ranges,usePriorContext);
        for(TSeatRanges::iterator i=ranges.begin();i!=ranges.end();i++)
        {
          strcpy(i->rem,rem_code);
        };
        seats.insert(seats.end(),ranges.begin(),ranges.end());
        usePriorContext=true;
      }
      catch(const EConvertError&)
      {
        //�������� �� ࠧ��ࠫ�� - �� � �७ � ���
        usePriorContext=false;
      };
    };
    return true;
  };
  return false;
};

bool ParseCHDRem(TTlgParser &tlg, const string &rem_text, vector<TChdItem> &chd)
{
  char c;
  int res;
  char rem_code[7],numh[4];

  const char *p=rem_text.c_str();

  chd.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%6[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  int num=1;
  if (!(strcmp(rem_code,"CHD")==0||
        strcmp(rem_code,"CHLD")==0))
  {
    *numh=0;
    sscanf(rem_code,"%3[0-9]%s",numh,rem_code);
    if (*numh!=0)
    {
      StrToInt(numh,num);
      if (num<=0) num=1;
    };
  };

  if (strcmp(rem_code,"CHD")==0||
      strcmp(rem_code,"CHLD")==0)
  {
    vector<string> names;
    vector<string>::iterator i;
    while((p=tlg.GetLexeme(p))!=NULL)
    {
      try
      {
        ParseNameElement(tlg.lex,names,epNone);
      }
      catch(const ETlgError &E)
      {
        continue;
      };
      for(i=names.begin();i!=names.end();i++)
        if (i->empty()) break;
      if (i!=names.end()) continue;
      for(i=names.begin();i!=names.end();i++)
      {
        if (i!=names.begin())
        {
          TChdItem chdItem(*names.begin(),*i);
          chd.push_back(chdItem);
        };
      };
    };
    return true;
  };

  return false;
};


bool ParseINFRem(TTlgParser &tlg, const string &rem_text, TInfList &inf)
{
  char c;
  int res;
  char rem_code[7],numh[4];

  const char *p=rem_text.c_str();

  inf.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%6[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;


  int num=1;
  if (!(strcmp(rem_code,"INF")==0||
        strcmp(rem_code,"INFT")==0))
  {
    *numh=0;
    sscanf(rem_code,"%3[0-9]%s",numh,rem_code);
    if (*numh!=0)
    {
      StrToInt(numh,num);
      if (num<=0) num=1;
    };
  };

  if (strcmp(rem_code,"INF")==0||
      strcmp(rem_code,"INFT")==0)
  {
    vector<string> names;
    vector<string>::iterator i;
    while((p=tlg.GetLexeme(p))!=NULL)
    {
      try
      {
        ParseNameElement(tlg.lex,names,epNone);
      }
      catch(const ETlgError &E)
      {
        continue;
      };

      if ((int)names.size()!=num+1) continue;
      for(i=names.begin();i!=names.end();i++)
      {
        if (i->empty()) break;
      };
      if (i!=names.end()) continue;
      for(i=names.begin();i!=names.end();i++)
      {
        if (i!=names.begin())
        {
          TInfItem infItem;
          infItem.surname=*names.begin();
          infItem.name=*i;
          inf.push_back(infItem);
        };
      };
      break;
    };

    for(int i=inf.size();i<num;i++)
    {
      //������� ������ ������楢 �� ���-�� num
      TInfItem infItem;
      inf.push_back(infItem);
    };
    return true;
  };

  return false;
};

bool ParseDOCSRem(TTlgParser &tlg, TDateTime scd_local, const string &rem_text, TDocItem &doc)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",doc.rem_code,&c);
  if (c!=0||res!=1) return false;

  TDateTime now=(scd_local!=0 && scd_local!=NoExists)?scd_local:NowUTC();

  if (strcmp(doc.rem_code,"DOCS")==0)
  {
    for(k=0;k<=11;k++)
    try
    {
      try
      {
        if (k==0)
        {
          p=tlg.GetWord(p);       //�� �� ᮮ⢥����� �⠭�����, �� ������� ��㣨�(�� ��७�) ⠪ �ନ��� :(
          if (*p=='/') p++;
        }
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL && k>=10) break;
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",doc.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.type,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            TlgElemToElemId(etPaxDocType,doc.type,doc.type);
            break;
          case 2:
          case 4:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==2)
              GetPaxDocCountry(lexh,doc.issue_country);
            else
              GetPaxDocCountry(lexh,doc.nationality);
            break;
          case 3:
            res=sscanf(tlg.lex,"%15[A-Z�-��0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 5:
            if (StrToDateTime(tlg.lex,"ddmmmyy",now,doc.birth_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",now,doc.birth_date,false)==EOF)
              throw ETlgError("Wrong format");
            if (doc.birth_date!=NoExists && doc.birth_date>now) throw ETlgError("Strange data");
            break;
          case 6:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.gender,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            doc.pr_inf=false;
            {
              int len=strlen(doc.gender);
              if (len>0 && doc.gender[len-1]=='I')
              {
                doc.pr_inf=true;
                doc.gender[len-1]=0;
              };
            };
            TlgElemToElemId(etGenderType,doc.gender,doc.gender);
            break;
          case 7:
            if (StrToDateTime(tlg.lex,"ddmmmyy",doc.expiry_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",doc.expiry_date,false)==EOF)
              throw ETlgError("Wrong format");
            if (doc.expiry_date!=NoExists && doc.expiry_date<now) throw ETlgError("Strange data");
            break;
          case 8:
          case 9:
          case 10:
            res=sscanf(tlg.lex,"%[A-Z�-�� -]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==8)  doc.surname=lexh;
            if (k==9)  doc.first_name=lexh;
            if (k==10) doc.second_name=lexh;
            break;
          case 11:
            res=sscanf(tlg.lex,"%1[H]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"H")!=0) throw ETlgError("Wrong format");
            doc.pr_multi=true;
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *doc.rem_status=0;
            throw ETlgError("action/status code: %s",E.what());
          case 1:
            *doc.type=0;
            throw ETlgError("document type: %s",E.what());
          case 2:
            *doc.issue_country=0;
            throw ETlgError("document issuing country/state: %s",E.what());
          case 3:
            *doc.no=0;
            throw ETlgError("travel document number: %s",E.what());
          case 4:
            *doc.nationality=0;
            throw ETlgError("passenger nationality: %s",E.what());
          case 5:
            doc.birth_date=NoExists;
            throw ETlgError("date of birth: %s",E.what());
          case 6:
            *doc.gender=0;
            throw ETlgError("gender code: %s",E.what());
          case 7:
            doc.expiry_date=NoExists;
            throw ETlgError("travel document expiry date: %s",E.what());
          case 8:
            doc.surname.clear();
            throw ETlgError("travel document surname: %s",E.what());
          case 9:
            doc.first_name.clear();
            throw ETlgError("travel document first given name: %s",E.what());
          case 10:
            doc.second_name.clear();
            throw ETlgError("travel document second given name: %s",E.what());
          case 11:
            doc.pr_multi=false;
            throw ETlgError("multi-passenger passport holder indicator: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  if (strcmp(doc.rem_code,"PSPT")==0)
  {
    for(k=0;k<=7;k++)
    try
    {
      try
      {
        if (k==0)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL && k>=7) break;
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",doc.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"%15[A-Z�-��0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 2:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            GetPaxDocCountry(lexh,doc.issue_country);
            break;
          case 3:
            if (StrToDateTime(tlg.lex,"ddmmmyy",now,doc.birth_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",now,doc.birth_date,false)==EOF)
              throw ETlgError("Wrong format");
            if (doc.birth_date!=NoExists && doc.birth_date>now) throw ETlgError("Strange data");
            break;
          case 4:
          case 5:
            res=sscanf(tlg.lex,"%[A-Z�-�� -]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==4) doc.surname=lexh;
            if (k==5) doc.first_name=lexh;
            break;
          case 6:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.gender,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            doc.pr_inf=false;
            {
              int len=strlen(doc.gender);
              if (len>0 && doc.gender[len-1]=='I')
              {
                doc.pr_inf=true;
                doc.gender[len-1]=0;
              };
            }
            TlgElemToElemId(etGenderType,doc.gender,doc.gender);
            break;
          case 7:
            res=sscanf(tlg.lex,"%1[H]%c",lexh,&c);
            if (c!=0||res!=2||strcmp(lexh,"H")!=0) throw ETlgError("Wrong format");
            doc.pr_multi=true;
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *doc.rem_status=0;
            throw ETlgError("action/status code: %s",E.what());
          case 1:
            *doc.no=0;
            throw ETlgError("passport number: %s",E.what());
          case 2:
            *doc.issue_country=0;
            throw ETlgError("country code: %s",E.what());
          case 3:
            doc.birth_date=NoExists;
            throw ETlgError("date of birth: %s",E.what());
          case 4:
            doc.surname.clear();
            throw ETlgError("surname: %s",E.what());
          case 5:
            doc.first_name.clear();
            throw ETlgError("name: %s",E.what());
          case 6:
            *doc.gender=0;
            throw ETlgError("gender code: %s",E.what());
          case 7:
            doc.pr_multi=false;
            throw ETlgError("multi-passenger passport holder indicator: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseDOCORem(TTlgParser &tlg, TDateTime scd_local, const string &rem_text, TDocoItem &doc)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",doc.rem_code,&c);
  if (c!=0||res!=1) return false;

  TDateTime now=(scd_local!=0 && scd_local!=NoExists)?scd_local:NowUTC();

  if (strcmp(doc.rem_code,"DOCO")==0)
  {
    for(k=0;k<=7;k++)
    try
    {
      try
      {
        if (k==0)
        {
          p=tlg.GetWord(p);       //�� �� ᮮ⢥����� �⠭�����, �� ��७� ⠪ �ନ��� :(
          if (*p=='/') p++;
        }
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL && k>=7) break;
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",doc.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
          case 4:
            res=sscanf(tlg.lex,"%[A-Z�-��0-9 -]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==1)  doc.birth_place=lexh;
            if (k==4)  doc.issue_place=lexh;
            break;
          case 2:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.type,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            TlgElemToElemId(etPaxDocType,doc.type,doc.type);
            break;
          case 3:
            res=sscanf(tlg.lex,"%25[A-Z�-��0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 5:
            if (StrToDateTime(tlg.lex,"ddmmmyy",now,doc.issue_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",now,doc.issue_date,false)==EOF)
              throw ETlgError("Wrong format");
            if (doc.issue_date!=NoExists && doc.issue_date>now) throw ETlgError("Strange data");
            break;
          case 6:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            GetPaxDocCountry(lexh,doc.applic_country);
            break;
          case 7:
            res=sscanf(tlg.lex,"%1[I]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"I")!=0) throw ETlgError("Wrong format");
            doc.pr_inf=true;
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *doc.rem_status=0;
            throw ETlgError("action/status code: %s",E.what());
          case 1:
            doc.birth_place.clear();
            throw ETlgError("place of birth: %s",E.what());
          case 2:
            *doc.type=0;
            throw ETlgError("travel document type: %s",E.what());
          case 3:
            *doc.no=0;
            throw ETlgError("visa document number: %s",E.what());
          case 4:
            doc.issue_place.clear();
            throw ETlgError("visa document place of issue: %s",E.what());
          case 5:
            doc.issue_date=NoExists;
            throw ETlgError("visa document issue date: %s",E.what());
          case 6:
            *doc.applic_country=0;
            throw ETlgError("country/state for which visa is applicable: %s",E.what());
          case 7:
            doc.pr_inf=false;
            throw ETlgError("infant indicator: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseDOCARem(TTlgParser &tlg, const string &rem_text, TDocaItem &doca)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  doca.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",doca.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(doca.rem_code,"DOCA")==0)
  {
    for(k=0;k<=7;k++)
    try
    {
      try
      {
        if (k==0)
        {
          p=tlg.GetWord(p);       //�� �� ᮮ⢥����� �⠭�����, �� ��७� ⠪ �ନ��� :(
          if (*p=='/') p++;
        }
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL && k>=7) break;
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1-3]%c",doca.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"%1[RD]%c",doca.type,&c);
            if (c!=0||res!=1||
                (strcmp(doca.type,"R")!=0&&strcmp(doca.type,"D")!=0)) throw ETlgError("Wrong format");
            break;
          case 2:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            GetPaxDocCountry(lexh,doca.country);
            break;
          case 3:
          case 6:
            res=sscanf(tlg.lex,"%[A-Z�-��0-9 -]%c",lexh,&c);   //����� ��ꥤ����� � 4 � 5
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==3)
            {
              doca.address=lexh;
              if (doca.address.size()>35) throw ETlgError("Wrong format");
            };
            if (k==6)
            {
              doca.postal_code=lexh;
              if (doca.postal_code.size()>17) throw ETlgError("Wrong format");
            };
            break;
          case 4:
          case 5:
            res=sscanf(tlg.lex,"%[A-Z�-��0-9 -]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==4)
            {
              doca.city=lexh;
              if (doca.city.size()>35) throw ETlgError("Wrong format");
            };
            if (k==5)
            {
              doca.region=lexh;
              if (doca.region.size()>35) throw ETlgError("Wrong format");
            };
            break;
          case 7:
            res=sscanf(tlg.lex,"%1[I]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"I")!=0) throw ETlgError("Wrong format");
            doca.pr_inf=true;
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *doca.rem_status=0;
            throw ETlgError("action/status code: %s",E.what());
          case 1:
            *doca.type=0;
            throw ETlgError("type of address: %s",E.what());
          case 2:
            *doca.country=0;
            throw ETlgError("country: %s",E.what());
          case 3:
            doca.address.clear();
            throw ETlgError("address details: %s",E.what());
          case 4:
            doca.city.clear();
            throw ETlgError("city: %s",E.what());
          case 5:
            doca.region.clear();
            throw ETlgError("state/province/county: %s",E.what());
          case 6:
            doca.postal_code.clear();
            throw ETlgError("zip/postal code: %s",E.what());
          case 7:
            doca.pr_inf=false;
            throw ETlgError("infant indicator: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doca.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

PassengerSystemId parseSystemId(TTlgParser &tlg, const string &paxLevelElement)
{
  PassengerSystemId result;

  char c;
  int res,k;

  const char *p=paxLevelElement.c_str();

  for(k=0;k<=1;k++)
  try
  {
    try
    {
      p=tlg.GetSlashedLexeme(p);
      if (p==NULL && k>=1) break;
      if (p==NULL) throw ETlgError("Lexeme not found");
      if (*tlg.lex==0) continue;
      c=0;
      switch(k)
      {
        case 0:
          res=sscanf(tlg.lex,"%25[A-Z0-9]%c",lexh,&c);
          if (c!=0||res!=1) throw ETlgError("Wrong format");
          result.uniqueReference=lexh;
          break;
        case 1:
          res=sscanf(tlg.lex,"%1[I]%c",lexh,&c);
          if (c!=0||res!=1||strcmp(lexh,"I")!=0) throw ETlgError("Wrong format");
          result.infantIndicator=true;
          break;
      }
    }
    catch(const exception &E)
    {
      switch(k)
      {
        case 0:
          throw ETlgError("unique reference: %s",E.what());
        case 1:
          throw ETlgError("infant indicator: %s",E.what());
      };
    };
  }
  catch(const ETlgError &E)
  {
    //ProgTrace(TRACE0,"Critical .U/ error: %s (%s)",E.what(),paxLevelElement.c_str());
    throw;
  };

  return result;
}

void parseAndBindSystemId(TTlgParser &tlg, TNameElement &ne, const string &paxLevelElement)
{
  try
  {
    string systemId(paxLevelElement);
    //���஡����� �ਢ易�� ६�ન � ������� ���ᠦ�ࠬ
    if (!systemId.empty())
    {
      set<TNameElement::PaxItemsIterator> paxItems;
      ne.parsePassengerIDs(systemId, paxItems);
      if (paxItems.size()!=1)
        throw ETlgError("Must relate strictly to one passenger");
      (*paxItems.begin())->systemIds.push_back(parseSystemId(tlg, systemId));
    }
    else parseSystemId(tlg, systemId);
  }
  catch(const ETlgError &e)
  {
    throw ETlgError(".U/ error: %s", e.what());
  }
}

void PersonAncestor::add(const PassengerSystemId& id)
{
  if (systemId)
  {
    if (systemId.get().uniqueReference!=id.uniqueReference)
      throw ETlgError("Duplicate unique reference .U/%s", id.uniqueReference.c_str());
  }
  else systemId=id;
}

bool ParseCHKDRem(TTlgParser &tlg, const string &rem_text, TCHKDItem &chkd)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  chkd.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",chkd.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(chkd.rem_code,"CHKD")==0)
  {
    for(k=0;k<=2;k++)
    try
    {
      try
      {
        p=tlg.GetLexeme(p);
        if (p==NULL && k>=2) break;
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1-3]%c",chkd.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"%ld%c",&chkd.reg_no,&c);
            if (c!=0||res!=1||
                chkd.reg_no<0||chkd.reg_no>9999) throw ETlgError("Wrong format");
            break;
          case 2:
            res=sscanf(tlg.lex,"%1[I]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"I")!=0) throw ETlgError("Wrong format");
            chkd.pr_inf=true;
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *chkd.rem_status=0;
            throw ETlgError("action/status code: %s",E.what());
          case 1:
            chkd.reg_no=NoExists;
            throw ETlgError("sequence number: %s",E.what());
          case 2:
            chkd.pr_inf=false;
            throw ETlgError("infant indicator: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",chkd.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseASVCRem(TTlgParser &tlg, const string &rem_text, TASVCItem &asvc)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  asvc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",asvc.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(asvc.rem_code,"ASVC")==0)
  {
    for(k=0;k<=6;k++)
    try
    {
      try
      {
        if (k==0)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL) throw ETlgError("Lexeme not found");
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%d%c",asvc.rem_status,&asvc.service_quantity,&c);
            if (c!=0||res!=2||
                asvc.service_quantity<1||asvc.service_quantity>999) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"%1[A-Z�-��0-9]%c",asvc.RFIC,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 2:
            res=sscanf(tlg.lex,"%15[A-Z�-��0-9]%c",asvc.RFISC,&c);
            if (c!=0||res!=1||strlen(asvc.RFISC)<3) throw ETlgError("Wrong format");
            break;
          case 3:
            res=sscanf(tlg.lex,"%4[A-Z�-��]%c",asvc.ssr_code,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 4:
            res=sscanf(tlg.lex,"%30[A-Z�-��0-9 ]%*[A-Z�-��0-9 ]%c",asvc.service_name,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 5:
            res=sscanf(tlg.lex,"%1[AS]%c",asvc.emd_type,&c);
            if (c!=0||res!=1||
                (strcmp(asvc.emd_type,"A")!=0&&strcmp(asvc.emd_type,"S")!=0)) throw ETlgError("Wrong format");
            break;
          case 6:
            res=sscanf(tlg.lex,"%13[A-Z�-��0-9]%1[C]%d%c",asvc.emd_no,lexh,&asvc.emd_coupon,&c);
            if (c!=0||res!=3||
                strlen(asvc.emd_no)!=13||
                strcmp(lexh,"C")!=0||
                asvc.emd_coupon<0||asvc.emd_coupon>9) throw ETlgError("Wrong format");
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *asvc.rem_status=0;
            asvc.service_quantity=NoExists;
            throw ETlgError("status code: %s",E.what());
          case 1:
            *asvc.RFIC=0;
            throw ETlgError("reason for issuance code: %s",E.what());
          case 2:
            *asvc.RFISC=0;
            throw ETlgError("reason for issuance sub code: %s",E.what());
          case 3:
            *asvc.ssr_code=0;
            throw ETlgError("SSR code: %s",E.what());
          case 4:
            *asvc.service_name=0;
            throw ETlgError("commercial name of service: %s",E.what());
          case 5:
            *asvc.emd_type=0;
            throw ETlgError("EMD type: %s",E.what());
          case 6:
            *asvc.emd_no=0;
            asvc.emd_coupon=NoExists;
            throw ETlgError("EMD number: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",asvc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
}

bool ParseSeatBlockingRem(TTlgParser &tlg, const string &rem_text, TSeatBlockingRem &rem)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  rem.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",rem.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (!TPaxItem::isSeatBlockingRem(rem.rem_code)) return false;

  for(k=0;k<=0;k++)
  try
  {
    try
    {
      p=tlg.GetLexeme(p);
      if (p==NULL) throw ETlgError("Lexeme not found");
      if (*tlg.lex==0) continue;
      c=0;
      switch(k)
      {
        case 0:
          res=sscanf(tlg.lex,"%2[A-Z]%d%c",rem.rem_status,&rem.numberOfSeats,&c);
          if (c!=0||res!=2||
              rem.numberOfSeats<1||rem.numberOfSeats>3) throw ETlgError("Wrong format");
          break;
      }
    }
    catch(const exception &E)
    {
      switch(k)
      {
        case 0:
          *rem.rem_status=0;
          rem.numberOfSeats=0;
          throw ETlgError("action/status code: %s",E.what());
      };
    };
  }
  catch(const ETlgError &E)
  {
    ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",rem.rem_code,E.what(),rem_text.c_str());
  };

  return true;
}

bool ParseOTHS_DOCSRem(TTlgParser &tlg, const string &rem_text, TDocExtraItem &doc)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  char rem_code[6],rem_status[3];
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(rem_code,"OTHS")==0)
  {
    for(k=0;k<=3;k++)
    try
    {
      try
      {
        if (k==0)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL) break;
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",rem_status,lexh,&c);
            if (c!=0||res!=2) return false;
            break;
          case 1:
            res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"DOCS")!=0) return false;
            break;
          case 2:
            res=sscanf(tlg.lex,"%15[A-Z�-��0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 3:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",doc.type_rcpt,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            TlgElemToElemId(etRcptDocType,doc.type_rcpt,doc.type_rcpt);
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
          case 1:
            return false;
          case 2:
            *doc.no=0;
            throw ETlgError("travel document number: %s",E.what());
          case 3:
            *doc.type_rcpt=0;
            throw ETlgError("document type: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseOTHS_FQTSTATUSRem(TTlgParser &tlg, const string &rem_text, TFQTExtraItem &fqt)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  fqt.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  char rem_code[6],rem_status[3];
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(rem_code,"OTHS")==0)
  {
    for(k=0;k<=2;k++)
    try
    {
      try
      {
        if (k<2)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetToEOLLexeme(p);
        if (p==NULL) break;
        if (*tlg.lex==0) continue;
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",rem_status,lexh,&c);
            if (c!=0||res!=2) return false;
            break;
          case 1:
            res=sscanf(tlg.lex,"%9[A-Z�-��0-9]%c",lexh,&c);
            if (c!=0||res!=1||strcmp(lexh,"FQTSTATUS")!=0) return false;
            break;
          case 2:
            fqt.tier_level=tlg.lex;
            TrimString(fqt.tier_level);
            if (fqt.tier_level.size()>50) throw ETlgError("Wrong format");
            break;
        }
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
          case 1:
            return false;
          case 2:
            fqt.tier_level.clear();
            throw ETlgError("tier level: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseTKNRem(TTlgParser &tlg, const string &rem_text, TTKNItem &tkn)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  tkn.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",tkn.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(tkn.rem_code,"TKNA")==0||
      strcmp(tkn.rem_code,"TKNE")==0)
  {
    for(k=0;k<=2;k++)
    try
    {
      try
      {
        if (k==0)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL && k>=2 && strcmp(tkn.rem_code,"TKNA")==0) break; //�㯮� �� ��易⥫�� ��� TKNA
        if (p==NULL) throw ETlgError("Lexeme not found");
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",tkn.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"INF%15[A-Z�-��0-9]%c",tkn.ticket_no,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%15[A-Z�-��0-9]%c",tkn.ticket_no,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong format");
              tkn.pr_inf=false;
            }
            else
              tkn.pr_inf=true;
            break;
          case 2:
            res=sscanf(tlg.lex,"%1[0-9]%c",lexh,&c);
            if (c!=0||res!=1||StrToInt(lexh,tkn.coupon_no)==EOF) throw ETlgError("Wrong format");
            break;
        };
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *tkn.rem_status=0;
            throw ETlgError("status code: %s",E.what());
          case 1:
            *tkn.ticket_no=0;
            tkn.pr_inf=false;
            throw ETlgError("ticket number: %s",E.what());
          case 2:
            tkn.coupon_no=NoExists;
            throw ETlgError("coupon number: %s",E.what());
        };
      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",tkn.rem_code,E.what(),rem_text.c_str());
      return false;
    };

    return true;
  };
  return false;
};

bool ParseFQTRem(TTlgParser &tlg, const string &rem_text, TFQTItem &fqt)
{
  char c;
  int res,k;

  const char *p=rem_text.c_str();

  fqt.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",fqt.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(fqt.rem_code,"FQTV")==0||
      strcmp(fqt.rem_code,"FQTR")==0||
      strcmp(fqt.rem_code,"FQTU")==0||
      strcmp(fqt.rem_code,"FQTS")==0)
  {
    for(k=0;k<=1;k++)
    try
    {
      try
      {
        if (k==0)
          p=tlg.GetLexeme(p);
        else
          p=tlg.GetSlashedLexeme(p);
        if (p==NULL) throw ETlgError("Lexeme not found");
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",fqt.airline,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%c",fqt.airline,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong format");
            };
            GetAirline(fqt.airline);
            break;
          case 1:
            res=sscanf(tlg.lex,"%25[A-Z�-��0-9]%c",fqt.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            fqt.extra=p;
            TrimString(fqt.extra);
            break;
        };
      }
      catch(const exception &E)
      {
        switch(k)
        {
          case 0:
            *fqt.airline=0;
            throw ETlgError("airline: %s",E.what());
          case 1:
            *fqt.no=0;
            throw ETlgError("frequent traveller number/identification: %s",E.what());
        };

      };
    }
    catch(const ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",fqt.rem_code,E.what(),rem_text.c_str());
      return false;
    };

    return true;
  };
  return false;
};

void GetParts(const char* tlg_p, TTlgPartsText &text, THeadingInfo* &info, TFlightsForBind &flts, TMemoryManager &mem)
{
  text.clear();
  if (info!=NULL)
  {
    mem.destroy(info, STDLOG);
    delete info;
    info = NULL;
  };
  flts.clear();

  const char *p,*line_p;
  TTlgParser tlg;
  TTlgParts parts;
  TTlgElement e;

  parts.addr.p=tlg_p;
  parts.addr.EOL_count=0;
  parts.addr.offset=0;

  line_p=parts.addr.p;
  try
  {
    e=Address;
    do
    {
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case Address:
          if (tlg.lex[0]=='.')
          {
            parts.heading=nextPart(parts.addr, line_p);
            e=CommunicationsReference;
          }
          else break;
        case CommunicationsReference:
          parts.body=ParseHeading(parts.heading,info,flts,mem);  //����� ������ NULL
          line_p=parts.body.p;
          e=EndOfMessage;
          if ((p=tlg.GetLexeme(line_p))==NULL) break;
        case EndOfMessage:
          switch (info->tlg_cat)
          {
            case tcDCS:
            case tcBSM:
              if (strstr(tlg.lex,"END")==tlg.lex)
              {
                parts.ending=nextPart(parts.body, line_p);
                e=EndOfMessage;
              };
              break;
            case tcAHM:
              if (strcmp(tlg.lex,"PART")==0)
              {
                parts.ending=nextPart(parts.body, line_p);
                e=EndOfMessage;
              };
              break;
            default:;
          };
          break;
        default: ;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(const ETlgError& E)
  {
    if (E.error_line()==NoExists)
      throwTlgError(E.what(), parts.addr, line_p);
    else
      throw;
  };

  if (parts.addr.p==NULL) throw ETlgError("Address not found");
  if (parts.heading.p==NULL) throw ETlgError("Heading not found");
  if (parts.body.p==NULL) throw ETlgError("Body not found");
  if (parts.ending.p==NULL&&
      (info->tlg_cat==tcDCS||
       info->tlg_cat==tcBSM)) throw ETlgError("End of message not found");

  text.addr.assign(parts.addr.p, parts.heading.p-parts.addr.p);
  text.heading.assign(parts.heading.p, parts.body.p-parts.heading.p);
  if (parts.ending.p!=NULL)
  {
    text.body.assign(parts.body.p, parts.ending.p-parts.body.p);
    text.ending.assign(parts.ending.p);
  }
  else
  {
    text.body.assign(parts.body.p);
  };
};

int SaveFlt(int tlg_id, const TFltInfo& flt, TBindType bind_type, TSearchFltInfoPtr search_params, ETlgErrorType error_type)
{
  const auto res=flt.getPointId(bind_type);
  int point_id=res.first;
  TTlgBinding(false, search_params).bind_flt(point_id);

  if (res.second)
  {
    /* ����� �஢�ਬ ���ਢ易��� ᥣ����� �� crs_displace � �ਢ殮� */
    /* �� ⮫쪮 ��� PNL/ADL */
    if (!flt.pr_utc && bind_type==btFirstSeg && *flt.airp_arv==0)
    {
      DB::TQuery Qry(PgOra::getRWSession("CRS_DISPLACE2"));
      Qry.SQLText=
        "UPDATE crs_displace2 SET point_id_tlg=:point_id "
        "WHERE airline=:airline AND flt_no=:flt_no AND "
        "      COALESCE(suffix,' ')=COALESCE(:suffix,' ') AND "
        "      TRUNC(scd)=TRUNC(:scd) AND "
        "      airp_dep=:airp_dep AND "
        "      point_id_tlg IS NULL";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.CreateVariable("airline",otString,flt.airline);
      Qry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
      Qry.CreateVariable("suffix",otString,flt.suffix);
      Qry.CreateVariable("scd",otDate,flt.scd);
      Qry.CreateVariable("airp_dep",otString,flt.airp_dep);
      Qry.Execute();
    };
  }

  bool has_errors=error_type!=tlgeNotError;
  bool has_alarm_errors=error_type==tlgeNotMonitorYesAlarm ||
                        error_type==tlgeYesMonitorYesAlarm;

  TlgSource(point_id, tlg_id, has_errors, has_alarm_errors).toDB();

  if (bind_type!=btNone)
    check_tlg_in_alarm(point_id, NoExists);
  return point_id;
}

bool isDeleteTypeBContent(int point_id, const THeadingInfo& info)
{
  if (strcmp(info.tlg_type,"SOM")==0)
  {
    DB::TQuery Qry(PgOra::getROSession("ORACLE"));
    Qry.SQLText=
      "SELECT MAX(time_create) AS max_time_create "
      "FROM tlgs_in,tlg_source "
      "WHERE tlg_source.tlg_id=tlgs_in.id AND "
      "      tlg_source.point_id_tlg=:point_id AND "
      "      COALESCE(tlg_source.has_errors,0)=0 AND "
      "      tlgs_in.type=:tlg_type";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("tlg_type",otString,info.tlg_type);
    Qry.Execute();
    return Qry.Eof ||
           Qry.FieldIsNULL("max_time_create") ||
           Qry.FieldAsDateTime("max_time_create") <= info.time_create;
  }

  if (strcmp(info.tlg_type,"BTM")==0 ||
      strcmp(info.tlg_type,"PTM")==0)
  {
    DB::TQuery Qry(PgOra::getROSession("ORACLE"));
    Qry.SQLText=
      "SELECT MAX(time_create) AS max_time_create "
      "FROM tlgs_in,tlg_source "
      "WHERE tlg_source.tlg_id=tlgs_in.id AND "
      "      tlg_source.point_id_tlg=:point_id_in AND "
      "      COALESCE(tlg_source.has_errors,0)=0 AND "
      "      tlgs_in.type=:tlg_type";
    Qry.CreateVariable("point_id_in",otInteger,point_id);
    Qry.CreateVariable("tlg_type",otString,info.tlg_type);
    Qry.Execute();
    return Qry.Eof ||
           Qry.FieldIsNULL("max_time_create") ||
           Qry.FieldAsDateTime("max_time_create") <= info.time_create;
  }

  if (strcmp(info.tlg_type,"PRL")==0)
  {
    DB::TQuery Qry(PgOra::getROSession("TYPEB_DATA_STAT"));
    Qry.SQLText=
      "SELECT last_data AS max_time_create "
      "FROM typeb_data_stat "
      "WHERE point_id=:point_id AND system=:system AND sender=:sender";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,"DCS");
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.Execute();
    return Qry.Eof ||
           Qry.FieldIsNULL("max_time_create") ||
           Qry.FieldAsDateTime("max_time_create") <= info.time_create;
  }

  return false;
}

bool DeleteSOMContent(int point_id, const THeadingInfo& info)
{
  if (isDeleteTypeBContent(point_id, info))
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  DELETE FROM "
      "  (SELECT * FROM tlg_comp_layers,trip_comp_layers "
      "   WHERE tlg_comp_layers.range_id=trip_comp_layers.range_id AND "
      "         tlg_comp_layers.point_id=:point_id AND "
      "         tlg_comp_layers.layer_type=:layer_type); "
      "  DELETE FROM tlg_comp_layers "
      "  WHERE point_id=:point_id AND layer_type=:layer_type; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("layer_type",otString,EncodeCompLayerType(cltSOMTrzt));
    Qry.Execute();
    return true;
  };
  return false;
};

bool DeletePRLContent(int point_id, const THeadingInfo& info)
{
  if (isDeleteTypeBContent(point_id, info))
  {
    deleteTypeBData(PointIdTlg_t(point_id), "DCS", CrsSender_t(info.sender), true /*delete_trip_comp_layers*/);
    return true;
  };
  return false;
};

bool DeletePTMBTMContent(int point_id_in, const THeadingInfo& info)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id_in;
  if (isDeleteTypeBContent(point_id_in, info))
  {
    const std::set<TrferId_t> trfer_ids =
        TrferList::loadTrferIdsByTlgTransferIn(PointIdTlg_t(point_id_in),
                                               info.tlg_type);

    for(const TrferId_t& trfer_id: trfer_ids)
    {
      const std::set<TrferGrpId_t> grp_ids = TrferList::loadTrferGrpIdSet(trfer_id);
      for(const TrferGrpId_t& grp_id: grp_ids) {
        TrferList::deleteTrferPax(grp_id);
        TrferList::deleteTrferTags(grp_id);
        TrferList::deleteTlgTrferOnwards(grp_id);
        TrferList::deleteTlgTrferExcepts(grp_id);
      }
      TrferList::deleteTrferGrp(trfer_id);
      TrferList::deleteTlgTransfer(trfer_id);
    }
    return true;
  }
  return false;
}

void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, const TBtmContent& con)
{
  DB::TQuery TrferQry(PgOra::getRWSession("TLG_TRANSFER"));
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(:trfer_id,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.DeclareVariable("trfer_id",otInteger);
  TrferQry.DeclareVariable("point_id_in",otInteger);
  TrferQry.DeclareVariable("subcl_in",otString);
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  DB::TQuery GrpQry(PgOra::getRWSession("TRFER_GRP"));
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(:grp_id,:trfer_id,NULL,:bag_amount,:bag_weight,:rk_weight,:weight_unit) ";
  GrpQry.DeclareVariable("bag_amount",otInteger);
  GrpQry.DeclareVariable("bag_weight",otInteger);
  GrpQry.DeclareVariable("rk_weight",otInteger);
  GrpQry.DeclareVariable("weight_unit",otString);
  GrpQry.DeclareVariable("trfer_id",otInteger);
  GrpQry.DeclareVariable("grp_id",otInteger);

  DB::TQuery PaxQry(PgOra::getRWSession("TRFER_PAX"));
  PaxQry.SQLText=
    "INSERT INTO trfer_pax(grp_id,surname,name) VALUES(:grp_id,:surname,:name)";
  PaxQry.DeclareVariable("surname",otString);
  PaxQry.DeclareVariable("name",otString);
  PaxQry.DeclareVariable("grp_id",otInteger);

  DB::TQuery TagQry(PgOra::getRWSession("TRFER_TAGS"));
  TagQry.SQLText=
    "INSERT INTO trfer_tags(grp_id,no) VALUES(:grp_id,:no)";
  TagQry.DeclareVariable("no",otFloat);
  TagQry.DeclareVariable("grp_id",otInteger);

  DB::TQuery OnwardQry(PgOra::getRWSession("TLG_TRFER_ONWARDS"));
  OnwardQry.SQLText=
    "INSERT INTO tlg_trfer_onwards(grp_id, num, airline, flt_no, suffix, local_date, airp_dep, airp_arv, subclass) "
    "VALUES(:grp_id, :num, :airline, :flt_no, :suffix, :local_date, :airp_dep, :airp_arv, :subclass)";
  OnwardQry.DeclareVariable("num", otInteger);
  OnwardQry.DeclareVariable("airline", otString);
  OnwardQry.DeclareVariable("flt_no", otInteger);
  OnwardQry.DeclareVariable("suffix", otString);
  OnwardQry.DeclareVariable("local_date", otDate);
  OnwardQry.DeclareVariable("airp_dep", otString);
  OnwardQry.DeclareVariable("airp_arv", otString);
  OnwardQry.DeclareVariable("subclass", otString);
  OnwardQry.DeclareVariable("grp_id",otInteger);

  DB::TQuery ExceptQry(PgOra::getRWSession("TLG_TRFER_EXCEPTS"));
  ExceptQry.SQLText=
    "INSERT INTO tlg_trfer_excepts(grp_id, type) VALUES(:grp_id, :type)";
  ExceptQry.DeclareVariable("type", otString);
  ExceptQry.DeclareVariable("grp_id",otInteger);

  int point_id_in,point_id_out;
  std::set<PointId_t> alarm_point_ids;
  for(vector<TBtmTransferInfo>::const_iterator iIn=con.Transfer.begin();iIn!=con.Transfer.end();++iIn)
  {
    point_id_in=SaveFlt(tlg_id,iIn->InFlt,btLastSeg,TSearchFltInfoPtr());
    //������ point_id_spp ३ᮢ, ����� ��ࠢ����� �࠭���
    std::set<PointId_t> point_ids_spp
        = TrferList::loadPointIdsSppByTlgTransferIn(PointIdTlg_t(point_id_in));
    alarm_point_ids.insert(point_ids_spp.begin(), point_ids_spp.end());

    if (!DeletePTMBTMContent(point_id_in,info)) continue;
    TrferQry.SetVariable("point_id_in",point_id_in);
    TrferQry.SetVariable("subcl_in",iIn->InFlt.subcl);
    for(vector<TBtmOutFltInfo>::const_iterator iOut=iIn->OutFlt.begin();iOut!=iIn->OutFlt.end();++iOut)
    {
      point_id_out=SaveFlt(tlg_id,*iOut,btFirstSeg,TSearchFltInfoPtr());
      TrferQry.SetVariable("point_id_out",point_id_out);
      TrferQry.SetVariable("subcl_out",iOut->subcl);
      const int new_trfer_id = PgOra::getSeqNextVal_int("CYCLE_ID__SEQ");
      TrferQry.SetVariable("trfer_id", new_trfer_id);
      TrferQry.Execute();
      for(vector<TBtmGrpItem>::const_iterator iGrp=iOut->grp.begin();iGrp!=iOut->grp.end();++iGrp)
      {
        if (*(iGrp->weight_unit)!=0)
        {
          GrpQry.SetVariable("bag_amount",(int)iGrp->bag_amount);
          GrpQry.SetVariable("bag_weight",(int)iGrp->bag_weight);
          GrpQry.SetVariable("rk_weight",(int)iGrp->rk_weight);
        }
        else
        {
          GrpQry.SetVariable("bag_amount",FNull);
          GrpQry.SetVariable("bag_weight",FNull);
          GrpQry.SetVariable("rk_weight",FNull);
        };
        GrpQry.SetVariable("weight_unit",iGrp->weight_unit);
        GrpQry.SetVariable("trfer_id",new_trfer_id);
        const int new_grp_id = PgOra::getSeqNextVal_int("PAX_GRP__SEQ");
        GrpQry.SetVariable("grp_id",new_grp_id);
        GrpQry.Execute();
        PaxQry.SetVariable("grp_id",new_grp_id);
        for(vector<TBtmPaxItem>::const_iterator iPax=iGrp->pax.begin();iPax!=iGrp->pax.end();++iPax)
        {
          PaxQry.SetVariable("surname",iPax->surname);
          if (!iPax->name.empty())
          {
            for(vector<string>::const_iterator i=iPax->name.begin();i!=iPax->name.end();i++)
            {
              PaxQry.SetVariable("name",*i);
              PaxQry.Execute();
            };
          }
          else
          {
            PaxQry.SetVariable("name",FNull);
            PaxQry.Execute();
          };
        };
        TagQry.SetVariable("grp_id",new_grp_id);
        for(vector<TBSMTagItem>::const_iterator iTag=iGrp->tags.begin();iTag!=iGrp->tags.end();++iTag)
          for(int j=0;j<iTag->num;j++)
          {
            TagQry.SetVariable("no",iTag->first_no+j);
            TagQry.Execute();
          };
        int num=1;
        string prior_airp_arv=iOut->airp_arv;
        OnwardQry.SetVariable("grp_id",new_grp_id);
        for(vector<TSegmentItem>::const_iterator iOnward=iGrp->OnwardFlt.begin();iOnward!=iGrp->OnwardFlt.end();++iOnward,num++)
        {
          OnwardQry.SetVariable("num", num);
          OnwardQry.SetVariable("airline", iOnward->airline);
          OnwardQry.SetVariable("flt_no", (int)iOnward->flt_no);
          OnwardQry.SetVariable("suffix", iOnward->suffix);
          OnwardQry.SetVariable("local_date", iOnward->scd);
          OnwardQry.SetVariable("airp_dep", prior_airp_arv);
          OnwardQry.SetVariable("airp_arv", iOnward->airp_arv);
          OnwardQry.SetVariable("subclass", iOnward->subcl);
          OnwardQry.Execute();
          prior_airp_arv=iOnward->airp_arv;
        };
        ExceptQry.SetVariable("grp_id",new_grp_id);
        for(set<string>::const_iterator iExcept=iGrp->excepts.begin();iExcept!=iGrp->excepts.end();++iExcept)
        {
          ExceptQry.SetVariable("type", *iExcept);
          ExceptQry.Execute();
        }
      }
    }

    //������ point_id_spp ३ᮢ, ����� ��ࠢ����� �࠭���
    point_ids_spp = TrferList::loadPointIdsSppByTlgTransferIn(PointIdTlg_t(point_id_in));
    alarm_point_ids.insert(point_ids_spp.begin(), point_ids_spp.end());
  };

  for(std::set<PointId_t>::const_iterator id=alarm_point_ids.begin(); id!=alarm_point_ids.end(); ++id)
  {
    check_unattached_trfer_alarm(id->get());
    //ProgTrace(TRACE5, "SaveBTMContent: check_unattached_trfer_alarm(%d)", *id);
  }
}

void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con)
{
  vector<TPtmOutFltInfo>::iterator iOut;
  vector<TPtmTransferData>::iterator iData;
  vector<string>::iterator i;
  int point_id_in,point_id_out;

  point_id_in=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(con.InFlt),btLastSeg,TSearchFltInfoPtr());
  if (!DeletePTMBTMContent(point_id_in,info)) return;

  DB::TQuery TrferQry(PgOra::getRWSession("TLG_TRANSFER"));
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(:trfer_id,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.DeclareVariable("trfer_id",otInteger);
  TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
  TrferQry.CreateVariable("subcl_in",otString,FNull); //�������� �ਫ�� �������⥭
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  DB::TQuery GrpQry(PgOra::getRWSession("TRFER_GRP"));
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(:grp_id,:trfer_id,:seats,:bag_amount,:bag_weight,NULL,:weight_unit) ";
  GrpQry.DeclareVariable("seats",otInteger);
  GrpQry.DeclareVariable("bag_amount",otInteger);
  GrpQry.DeclareVariable("bag_weight",otInteger);
  GrpQry.DeclareVariable("weight_unit",otString);
  GrpQry.DeclareVariable("grp_id",otInteger);
  GrpQry.DeclareVariable("trfer_id",otInteger);

  DB::TQuery PaxQry(PgOra::getRWSession("TRFER_PAX"));
  PaxQry.SQLText=
    "INSERT INTO trfer_pax(grp_id,surname,name) VALUES(:grp_id,:surname,:name)";
  PaxQry.DeclareVariable("surname",otString);
  PaxQry.DeclareVariable("name",otString);
  PaxQry.DeclareVariable("grp_id",otInteger);

  for(iOut=con.OutFlt.begin();iOut!=con.OutFlt.end();iOut++)
  {
    point_id_out=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(*iOut),btFirstSeg,TSearchFltInfoPtr());
    TrferQry.SetVariable("point_id_out",point_id_out);
    TrferQry.SetVariable("subcl_out",iOut->subcl);
    const int new_trfer_id = PgOra::getSeqNextVal_int("CYCLE_ID__SEQ");
    TrferQry.SetVariable("trfer_id", new_trfer_id);
    TrferQry.Execute();
    for(iData=iOut->data.begin();iData!=iOut->data.end();iData++)
    {
      GrpQry.SetVariable("seats",(int)iData->seats);
      GrpQry.SetVariable("bag_amount",(int)iData->bag_amount);
      if (*(iData->weight_unit)!=0)
        GrpQry.SetVariable("bag_weight",(int)iData->bag_weight);
      else
        GrpQry.SetVariable("bag_weight",FNull);
      GrpQry.SetVariable("weight_unit",iData->weight_unit);
      GrpQry.SetVariable("trfer_id",new_trfer_id);
      const int new_grp_id = PgOra::getSeqNextVal_int("PAX_GRP__SEQ");
      GrpQry.SetVariable("grp_id",new_grp_id);
      GrpQry.Execute();
      if (iData->surname.empty()) continue;
      PaxQry.SetVariable("surname",iData->surname);
      PaxQry.SetVariable("grp_id",new_grp_id);
      if (!iData->name.empty())
      {
        for(i=iData->name.begin();i!=iData->name.end();i++)
        {
          PaxQry.SetVariable("name",*i);
          PaxQry.Execute();
        };
      }
      else
      {
        PaxQry.SetVariable("name",FNull);
        PaxQry.Execute();
      };
    };
  };
};

static void onChangeClass(int pax_id, ASTRA::TClass cl)
{
  addAlarmByPaxId(PaxId_t(pax_id), {Alarm::SyncCabinClass}, {paxCheckIn});
  TPaxAlarmHook::set(Alarm::SyncCabinClass, pax_id);
}

void SaveDCSBaggage(int pax_id, const TNameElement &ne)
{
  if (ne.bag.Empty() && ne.tags.empty()) return;
  if (!ne.bag.Empty())
  {
    DB::TQuery QryBag(PgOra::getRWSession("DCS_BAG"));
    QryBag.SQLText=
      "INSERT INTO dcs_bag(pax_id, bag_amount, bag_weight, rk_weight, weight_unit) "
      "VALUES(:pax_id, :bag_amount, :bag_weight, :rk_weight, :weight_unit)";
    QryBag.CreateVariable("pax_id", otInteger, pax_id);
    QryBag.CreateVariable("bag_amount", otInteger, (int)ne.bag.bag_amount);
    QryBag.CreateVariable("bag_weight", otInteger, (int)ne.bag.bag_weight);
    QryBag.CreateVariable("rk_weight", otInteger, (int)ne.bag.rk_weight);
    QryBag.CreateVariable("weight_unit", otString, ne.bag.weight_unit);
    QryBag.Execute();
  };
  if (!ne.tags.empty())
  {
    DB::TQuery QryTags(PgOra::getRWSession("DCS_TAGS"));
    QryTags.SQLText=
      "INSERT INTO dcs_tags(pax_id, alpha_no, numeric_no, airp_arv_final) "
      "VALUES(:pax_id, :alpha_no, :numeric_no, :airp_arv_final)";
    QryTags.CreateVariable("pax_id",otInteger,pax_id);
    QryTags.DeclareVariable("alpha_no", otString);
    QryTags.DeclareVariable("numeric_no", otFloat);
    QryTags.DeclareVariable("airp_arv_final", otString);
    for(vector<TTagItem>::const_iterator iTag=ne.tags.begin(); iTag!=ne.tags.end(); ++iTag)
    {
      QryTags.SetVariable("alpha_no",iTag->alpha_no);
      QryTags.SetVariable("airp_arv_final",iTag->airp_arv_final);
      for(int j=0;j<iTag->num;j++)
      {
        QryTags.SetVariable("numeric_no",iTag->numeric_no+j);
        QryTags.Execute();
      };
    };
  };
};

void SaveSOMContent(int tlg_id, TDCSHeadingInfo& info, TSOMContent& con)
{
  //vector<TSeatsByDest>::iterator iSeats;
  int point_id=SaveFlt(tlg_id,con.flt,btFirstSeg,TSearchFltInfoPtr());
  if (!DeleteSOMContent(point_id,info)) return;

  bool usePriorContext=false;
  int curr_tid=NoExists;
  for(vector<TSeatsByDest>::iterator i=con.seats.begin();i!=con.seats.end();i++)
  {
    //����� ���� 㤠���� �� ᫮� ⥫��ࠬ� SOM �� ����� ࠭��� �㭪⮢ �� trip_comp_layers
    TPointIdsForCheck point_ids_spp;
    InsertTlgSeatRanges(point_id,i->airp_arv,cltSOMTrzt,i->ranges,NoExists,tlg_id,NoExists,usePriorContext,curr_tid,point_ids_spp);
    check_layer_change(point_ids_spp, __FUNCTION__);
    usePriorContext=true;
  };
};

static void makeCrsDataStatSqlSet(const TTlgElement elem,
                                  std::string& crs_data_set_null,
                                  std::string& crs_data_stat_field)
{
  switch (elem)
  {
    case TotalsByDestination:
      crs_data_set_null = "  SET resa=NULL, pad=NULL ";
      crs_data_stat_field = "last_resa";
      break;
    case TranzitElement:
      crs_data_set_null = "  SET tranzit=NULL ";
      crs_data_stat_field = "last_tranzit";
      break;
    case SpaceAvailableElement:
      crs_data_set_null = "  SET avail=NULL ";
      crs_data_stat_field = "last_avail";
      break;
    case Configuration:
      crs_data_set_null = "  SET cfg=NULL ";
      crs_data_stat_field = "last_cfg";
      break;
    case ClassCodes:
      crs_data_stat_field = "last_rbd";
      break;
    default: return;
  }
}

bool insertCrsDataStat(const PointIdTlg_t& point_id,
                       const TDCSHeadingInfo& info,
                       const std::string& crs_data_stat_field)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << info.sender
                   << ", time_create=" << info.time_create;
  auto cur = make_db_curs(
        "INSERT INTO crs_data_stat(point_id, crs, " + crs_data_stat_field + ") "
        "VALUES(:point_id, :sender, :time_create)",
        PgOra::getRWSession("CRS_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":sender", info.sender)
      .bind(":time_create", DateTimeToBoost(info.time_create))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool updateCrsDataStat(const PointIdTlg_t& point_id,
                       const TDCSHeadingInfo& info,
                       const std::string& crs_data_stat_field)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << info.sender
                   << ", time_create=" << info.time_create;
  auto cur = make_db_curs(
        "UPDATE crs_data_stat "
        "SET "
        + crs_data_stat_field + "=:time_create "
        "WHERE point_id=:point_id "
        "AND crs=:sender",
        PgOra::getRWSession("CRS_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":sender", info.sender)
      .bind(":time_create", DateTimeToBoost(info.time_create))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void saveCrsDataStat(const PointIdTlg_t& point_id,
                     const TDCSHeadingInfo& info,
                     const std::string& crs_data_stat_field)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << info.sender
                   << ", time_create=" << info.time_create;
  const bool updated = updateCrsDataStat(point_id, info,
                                         crs_data_stat_field);
  if (not updated) {
    insertCrsDataStat(point_id, info,
                      crs_data_stat_field);
  }
}

bool insertCrsDataStat_PrPnl(const PointIdTlg_t& point_id,
                             const std::string& sender,
                             int pr_pnl_new)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", pr_pnl_new=" << pr_pnl_new;
  auto cur = make_db_curs(
        "INSERT INTO crs_data_stat(point_id,crs,pr_pnl) "
        "VALUES(:point_id, :sender, :pr_pnl)",
        PgOra::getRWSession("CRS_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":sender", sender)
      .bind(":pr_pnl", pr_pnl_new)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool updateCrsDataStat_PrPnl(const PointIdTlg_t& point_id,
                             const std::string& sender,
                             int pr_pnl_new)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", pr_pnl_new=" << pr_pnl_new;
  auto cur = make_db_curs(
        "UPDATE crs_data_stat SET pr_pnl=:pr_pnl "
        "WHERE point_id=:point_id AND crs=:sender ",
        PgOra::getRWSession("CRS_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":sender", sender)
      .bind(":pr_pnl", pr_pnl_new)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void saveCrsDataStat_PrPnl(const PointIdTlg_t& point_id,
                           const std::string& sender,
                           int pr_pnl_new)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", pr_pnl_new=" << pr_pnl_new;
  const bool updated = updateCrsDataStat_PrPnl(point_id, sender, pr_pnl_new);
  if (not updated) {
    insertCrsDataStat_PrPnl(point_id, sender, pr_pnl_new);
  }
}

bool updateCrsData(const PointIdTlg_t& point_id,
                   const std::string& sender,
                   const std::string& system,
                   const std::string& crs_data_set_null)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", system=" << system;
  auto cur = make_db_curs(
        "UPDATE crs_data "
        + crs_data_set_null + " "
        "WHERE point_id=:point_id "
        "AND system=:system "
        "AND sender=:sender",
        PgOra::getRWSession("CRS_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool updateCrsData_Resa(const PointIdTlg_t& point_id,
                        const CrsSender_t& sender,
                        const std::string& system,
                        const TTotalsByDest& totals)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", system=" << system
                   << ", airp_arv=" << totals.dest
                   << ", class=" << EncodeClass(totals.cl)
                   << ", resa=" << totals.seats
                   << ", pad= " << totals.pad;
  auto cur = make_db_curs(
        "UPDATE crs_data "
        "SET resa=COALESCE(resa+:resa,:resa), "
        "pad=COALESCE(pad+:pad,:pad) "
        "WHERE point_id=:point_id "
        "AND system=:system "
        "AND sender=:sender "
        "AND airp_arv=:airp_arv "
        "AND class=:class ",
        PgOra::getRWSession("CRS_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender.get())
      .bind(":airp_arv",totals.dest)
      .bind(":class",EncodeClass(totals.cl))
      .bind(":resa",int(totals.seats))
      .bind(":pad",int(totals.pad))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool insertCrsData_Resa(const PointIdTlg_t& point_id,
                        const CrsSender_t& sender,
                        const std::string& system,
                        const TTotalsByDest& totals)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", system=" << system
                   << ", airp_arv=" << totals.dest
                   << ", class=" << EncodeClass(totals.cl)
                   << ", resa=" << totals.seats
                   << ", pad= " << totals.pad;
  auto cur = make_db_curs(
        "INSERT INTO crs_data( "
        "point_id,system,sender,airp_arv,class,resa,pad "
        ") VALUES ( "
        ":point_id,:system,:sender,:airp_arv,:class,:resa,:pad "
        ")",
        PgOra::getRWSession("CRS_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender.get())
      .bind(":airp_arv",totals.dest)
      .bind(":class",EncodeClass(totals.cl))
      .bind(":resa",int(totals.seats))
      .bind(":pad",int(totals.pad))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

static std::string makeCrsDataSqlField(const TTlgElement element)
{
  switch (element)
  {
    case TranzitElement:
      return "tranzit";
    case SpaceAvailableElement:
      return "avail";
    case Configuration:
      return "cfg";
    default: return std::string();
  }
  return std::string();
}

bool updateCrsData(const TTlgElement element,
                   const PointIdTlg_t& point_id,
                   const CrsSender_t& sender,
                   const std::string& system,
                   const AirportCode_t& airp_arv,
                   const TSeatsItem& seat)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", system=" << system
                   << ", airp_arv=" << airp_arv
                   << ", class=" << EncodeClass(seat.cl)
                   << ", seats=" << seat.seats;
  const std::string field_name = makeCrsDataSqlField(element);
  auto cur = make_db_curs(
        "UPDATE crs_data "
        "SET " + field_name +  "=COALESCE("+ field_name +"+:seats,:seats) "
        "WHERE point_id=:point_id "
        "AND system=:system "
        "AND sender=:sender "
        "AND airp_arv=:airp_arv "
        "AND class=:class ",
        PgOra::getRWSession("CRS_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender.get())
      .bind(":airp_arv",airp_arv.get())
      .bind(":class",EncodeClass(seat.cl))
      .bind(":seats",int(seat.seats));
  cur.exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool insertCrsData(const TTlgElement element,
                   const PointIdTlg_t& point_id,
                   const CrsSender_t& sender,
                   const std::string& system,
                   const AirportCode_t& airp_arv,
                   const TSeatsItem& seat)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", sender=" << sender
                   << ", system=" << system
                   << ", airp_arv=" << airp_arv
                   << ", class=" << EncodeClass(seat.cl)
                   << ", seats=" << seat.seats;
  const std::string field_name = makeCrsDataSqlField(element);
  auto cur = make_db_curs(
        "INSERT INTO crs_data( "
        "point_id,system,sender,airp_arv,class," + field_name + " "
        ") VALUES ( "
        ":point_id,:system,:sender,:airp_arv,:class,:seats "
        ")",
        PgOra::getRWSession("CRS_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender.get())
      .bind(":airp_arv",airp_arv.get())
      .bind(":class",EncodeClass(seat.cl))
      .bind(":seats",int(seat.seats))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void UpdateCrsDataStat(const TTlgElement elem,
                       const PointIdTlg_t& point_id,
                       const std::string& system,
                       const TDCSHeadingInfo &info)
{
   std::string crs_data_set_null, crs_data_stat_field;
   makeCrsDataStatSqlSet(elem, crs_data_set_null, crs_data_stat_field);

   if (elem!=ClassCodes) {
     updateCrsData(point_id, info.sender, system, crs_data_set_null);
   } else {
     TypeB::deleteCrsRbd(point_id, system, CrsSender_t(info.sender));
   }

   if (system=="CRS") {
     saveCrsDataStat(point_id, info, crs_data_stat_field);
   }
}

void UpdateCrsData_Resa(const PointIdTlg_t& point_id,
                        const CrsSender_t& sender,
                        const std::string& system,
                        const std::vector<TTotalsByDest>& resa)
{
  for (const TTotalsByDest& totals: resa) {
    const bool updated = updateCrsData_Resa(point_id, sender, system, totals);
    if (!updated) {
      insertCrsData_Resa(point_id, sender, system, totals);
    }
  }
}

void UpdateCrsData(TTlgElement element,
                   const PointIdTlg_t& point_id,
                   const CrsSender_t& sender,
                   const std::string& system,
                   const std::vector<TRouteItem>& avail)
{
  for (const TRouteItem& route: avail) {
    for(const TSeatsItem& seat: route.seats) {
      const bool updated = updateCrsData(element, point_id, sender, system,
                                         AirportCode_t(route.station), seat);
      if (!updated) {
        insertCrsData(element, point_id, sender, system,
                      AirportCode_t(route.station), seat);
      }
    }
  }
}

class SuitablePax
{
  public:
    PaxId_t paxId;
    boost::optional<PaxId_t> parentPaxId;
    bool deleted;
    TDateTime last_op;
    bool classChanged;

    SuitablePax(TCachedQuery &Qry) : paxId(Qry.get().FieldAsInteger("pax_id"))
    {
      if (!Qry.get().FieldIsNULL("parent_pax_id"))
        parentPaxId=boost::in_place(Qry.get().FieldAsInteger("parent_pax_id"));
      deleted=Qry.get().FieldAsInteger("pr_del")!=0;
      last_op=Qry.get().FieldAsDateTime("last_op");
      classChanged=false;
    }

    SuitablePax(TCachedQuery &Qry, TClass cl) : SuitablePax(Qry)
    {
      classChanged=(cl!=DecodeClass(Qry.get().FieldAsString("class")));
    }
};

class SuitablePaxList : public list<SuitablePax>
{
  public:
    void get(const int& pnr_id,
             const std::string& surname,
             const std::string& name,
             const bool& isInfant);
    void get(const std::vector<TTKNItem>& tkn,
             const int& point_id,
             const std::string& system,
             const std::string& sender,
             const TTotalsByDest& totalsByDest,
             const std::string& surname,
             const std::string& name,
             const bool& isInfant);
};

void SuitablePaxList::get(const int& pnr_id,
                          const std::string& surname,
                          const std::string& name,
                          const bool& isInfant)
{
  clear();

  TCachedQuery Qry("SELECT crs_pax.pax_id, crs_pax.pr_del, crs_pax.last_op, "
                   "       crs_inf_deleted.pax_id AS parent_pax_id "
                   "FROM crs_pax, crs_inf_deleted "
                   "WHERE crs_pax.pax_id=crs_inf_deleted.inf_id(+) AND "
                   "      crs_pax.pnr_id= :pnr_id AND "
                   "      crs_pax.surname= :surname AND "
                   "      (crs_pax.name= :name OR :name IS NULL AND crs_pax.name IS NULL) AND "
                   "      DECODE(crs_pax.seats,0,0,1)=:seats AND "
                   "      NOT EXISTS(SELECT seat_id FROM crs_seats_blocking WHERE crs_seats_blocking.seat_id=crs_pax.pax_id) "
                   "ORDER BY crs_pax.last_op DESC, crs_pax.pax_id DESC",
                   QParams() << QParam("pnr_id", otInteger, pnr_id)
                             << QParam("surname", otString, surname)
                             << QParam("name", otString, name)
                             << QParam("seats", otInteger, isInfant?0:1));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    emplace_back(Qry);
}

void SuitablePaxList::get(const std::vector<TTKNItem>& tkn,
                          const int& point_id,
                          const std::string& system,
                          const std::string& sender,
                          const TTotalsByDest& totalsByDest,
                          const std::string& surname,
                          const std::string& name,
                          const bool& isInfant)
{
  clear();
  if (tkn.empty()) return;

  TCachedQuery Qry("SELECT crs_pax.pax_id, crs_pax.pr_del, crs_pax.last_op, "
                   "       crs_pnr.class, "
                   "       crs_inf_deleted.pax_id AS parent_pax_id "
                   "FROM crs_pnr, crs_pax, crs_pax_tkn, crs_inf_deleted "
                   "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
                   "      crs_pax.pax_id=crs_pax_tkn.pax_id AND "
                   "      crs_pax.pax_id=crs_inf_deleted.inf_id(+) AND "
                   "      crs_pnr.point_id=:point_id AND "
                   "      crs_pnr.system=:system AND "
                   "      crs_pnr.sender=:sender AND "
                   "      crs_pnr.airp_arv=:airp_arv AND "
                   "      crs_pax_tkn.rem_code=:rem_code AND "
                   "      crs_pax_tkn.ticket_no=:ticket_no AND "
                   "      (crs_pax_tkn.coupon_no IS NULL AND :coupon_no IS NULL OR crs_pax_tkn.coupon_no=:coupon_no) AND "
                   "      crs_pax.surname= :surname AND "
                   "      (crs_pax.name= :name OR :name IS NULL AND crs_pax.name IS NULL) AND "
                   "      DECODE(crs_pax.seats,0,0,1)=:seats AND "
                   "      NOT EXISTS(SELECT seat_id FROM crs_seats_blocking WHERE crs_seats_blocking.seat_id=crs_pax.pax_id) "
                   "ORDER BY crs_pax.last_op DESC, crs_pax.pax_id DESC",
                   QParams() << QParam("point_id", otInteger, point_id)
                             << QParam("system", otString, system)
                             << QParam("sender", otString, sender)
                             << QParam("airp_arv", otString, totalsByDest.dest)
                             << QParam("rem_code", otString)
                             << QParam("ticket_no", otString)
                             << QParam("coupon_no", otInteger)
                             << QParam("surname", otString, surname)
                             << QParam("name", otString, name)
                             << QParam("seats", otInteger, isInfant?0:1));
  for(const TTKNItem& t : tkn)
  {
    t.toDB(Qry.get());
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
      emplace_back(Qry, totalsByDest.cl);
    if (!empty()) break;
  }
}

void loadCrsDataStat(int point_id, const std::string& sender,
                     TDateTime& last_resa,
                     TDateTime& last_tranzit,
                     TDateTime& last_avail,
                     TDateTime& last_cfg,
                     TDateTime& last_rbd,
                     int& pr_pnl)
{
  DB::TQuery Qry(PgOra::getRWSession("CRS_DATA_STAT"));
  Qry.SQLText=
    "SELECT last_resa,last_tranzit,last_avail,last_cfg,last_rbd,pr_pnl "
    "FROM crs_data_stat "
    "WHERE point_id=:point_id "
    "AND crs=:crs "
    "FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("crs",otString,sender);
  Qry.Execute();
  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("last_resa"))
      last_resa=Qry.FieldAsDateTime("last_resa");
    if (!Qry.FieldIsNULL("last_tranzit"))
      last_tranzit=Qry.FieldAsDateTime("last_tranzit");
    if (!Qry.FieldIsNULL("last_avail"))
      last_avail=Qry.FieldAsDateTime("last_avail");
    if (!Qry.FieldIsNULL("last_cfg"))
      last_cfg=Qry.FieldAsDateTime("last_cfg");
    if (!Qry.FieldIsNULL("last_rbd"))
      last_rbd=Qry.FieldAsDateTime("last_rbd");
    pr_pnl=Qry.FieldAsInteger("pr_pnl");
  };
}

std::optional<bool> getCrsSet_pr_numeric_pnl(const std::string& sender,
                                             const TFltInfo& flt)
{
  DB::TQuery Qry(PgOra::getROSession("CRS_SET"));
  Qry.SQLText=
    "SELECT pr_numeric_pnl FROM crs_set "
    "WHERE crs=:crs AND airline=:airline AND "
    "      (flt_no=:flt_no OR flt_no IS NULL) AND "
    "      (airp_dep=:airp_dep OR airp_dep IS NULL) "
    "ORDER BY flt_no,airp_dep";
  Qry.CreateVariable("crs",otString,sender);
  Qry.CreateVariable("airline",otString,flt.airline);
  Qry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
  Qry.CreateVariable("airp_dep",otString,flt.airp_dep);
  Qry.Execute();
  if (!Qry.Eof)
  {
    return Qry.FieldAsInteger("pr_numeric_pnl")!=0;
  }
  return {};
}

bool insertCrsSet(int new_id,
                  const std::string& sender,
                  const TFltInfo& flt)
{
  LogTrace(TRACE6) << __func__
                   << ": new_id=" << new_id;
  auto cur = make_db_curs(
        "INSERT INTO crs_set( "
        "id,airline,flt_no,airp_dep,crs,priority,pr_numeric_pnl "
        ") VALUES ( "
        ":new_id,:airline,null,:airp_dep,:crs,0,0 "
        ")",
        PgOra::getRWSession("CRS_SET"));
  cur.stb()
      .bind(":new_id", new_id)
      .bind(":crs", sender)
      .bind(":airline", flt.airline)
      .bind(":airp_dep",flt.airp_dep)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

std::string getCrsTransfer_airp_arv_final(const PnrId_t& pnr_id)
{
  DB::TQuery Qry(PgOra::getROSession("CRS_TRANSFER"));
  Qry.SQLText=
      "SELECT airp_arv "
      "FROM crs_transfer "
      "WHERE pnr_id=:pnr_id AND transfer_num= "
      "  (SELECT MAX(transfer_num) "
      "   FROM crs_transfer "
      "   WHERE pnr_id=:pnr_id "
      "   AND transfer_num>0) ";
  Qry.CreateVariable("pnr_id",otInteger,pnr_id.get());
  Qry.Execute();
  if (!Qry.Eof)
  {
    return Qry.FieldAsString("airp_arv");
  }
  return {};
}

bool saveTypeBDataStat(const PointIdTlg_t& point_id, const std::string& system,
                       const std::string& sender,
                       const TDateTime& time_create)
{
  QParams QryParams;
  QryParams << QParam("point_id", otInteger, point_id.get())
            << QParam("system", otString, system)
            << QParam("sender", otString, sender)
            << QParam("time_create", otDate, time_create);

  DB::TCachedQuery update(
        PgOra::getRWSession("TYPEB_DATA_STAT"),
        "UPDATE typeb_data_stat "
        "SET last_data=GREATEST(last_data,:time_create) "
        "WHERE point_id=:point_id "
        "AND system=:system "
        "AND sender=:sender ",
        QryParams);
  update.get().Execute();

  if (update.get().RowsProcessed()) {
    return true;
  }
  DB::TCachedQuery insert(
        PgOra::getRWSession("TYPEB_DATA_STAT"),
        "INSERT INTO typeb_data_stat(point_id,system,sender,last_data) "
        "VALUES(:point_id,:system,:sender,:time_create) ",
        QryParams);
  insert.get().Execute();
  return bool(insert.get().RowsProcessed());
}

bool SavePNLADLPRLContent(int tlg_id, TDCSHeadingInfo& info, TPNLADLPRLContent& con, bool forcibly)
{
  vector<TTotalsByDest>::iterator iTotals;
  vector<TPnrItem>::iterator iPnrItem;
  vector<TNameElement>::iterator iNameElement;
  vector<TPaxItem>::iterator iPaxItem;
  vector<TTransferItem>::iterator iTransfer;
  vector<TPnrAddrItem>::iterator iPnrAddr;

  TQuery Qry(&OraSession);

  if (info.time_create==0) throw ETlgError("Creation time not defined");

  int point_id=SaveFlt(tlg_id,con.flt,btFirstSeg,TSearchFltInfoPtr());

  //��������㥬 ��⥬� �஭�஢����
  bool isPRL=strcmp(info.tlg_type,"PRL")==0;
  if (isPRL && !DeletePRLContent(point_id,info)) return true;

  string system=isPRL?"DCS":"CRS";

  Qry.Clear();
  Qry.SQLText="INSERT INTO typeb_senders(code,name) VALUES(:code,:code)";
  Qry.CreateVariable("code",otString,info.sender);
  try
  {
    Qry.Execute();
  }
  catch(const EOracleError &E)
  {
    if (E.Code!=1) throw;
  };

  Qry.Clear();
  Qry.SQLText="INSERT INTO typeb_sender_systems(sender,system) VALUES(:sender,:system)";
  Qry.CreateVariable("sender",otString,info.sender);
  Qry.CreateVariable("system",otString,system);
  try
  {
    Qry.Execute();
  }
  catch(const EOracleError &E)
  {
    if (E.Code!=1) throw;
  };
  saveTypeBDataStat(PointIdTlg_t(point_id), system, info.sender, info.time_create);

  bool pr_numeric_pnl=false;
  TDateTime last_resa=NoExists,
            last_tranzit=NoExists,
            last_avail=NoExists,
            last_cfg=NoExists,
            last_rbd=NoExists;
  int pr_pnl=0;

  if (!isPRL)
  {
    const std::optional<bool> found = getCrsSet_pr_numeric_pnl(info.sender, con.flt);
    if (found) {
      pr_numeric_pnl=*found;
    }
    else
    {
      const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
      insertCrsSet(new_id, info.sender, con.flt);
      Qry.Clear();
      Qry.SQLText=
        "BEGIN "
        "  hist.synchronize_history('crs_set',:new_id,:SYS_user_descr,:SYS_desk_code); "
        "END;";
      Qry.CreateVariable("new_id", otInteger, new_id);
      Qry.CreateVariable("SYS_user_descr", otString, TReqInfo::Instance()->user.descr);
      Qry.CreateVariable("SYS_desk_code", otString, TReqInfo::Instance()->desk.code);
      try
      {
        Qry.Execute();
      }
      catch(const EOracleError &E)
      {
        if (E.Code!=1) throw;
      };
    };

    loadCrsDataStat(point_id, info.sender,
                    last_resa,
                    last_tranzit,
                    last_avail,
                    last_cfg,
                    last_rbd,
                    pr_pnl);

    //pr_pnl=0 - �� ��襫 ��஢�� PNL
    //pr_pnl=1 - ��襫 ��஢�� PNL
    //pr_pnl=2 - ��襫 ����஢�� PNL
  };

  bool pr_save_ne=isPRL ||
                  !((strcmp(info.tlg_type,"PNL")==0&&pr_pnl==2)|| //��襫 ��ன ����஢�� PNL
                    (strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2)); //��襫 ADL �� ����� PNL

  bool pr_recount=false;
  //������� ��஢� �����
  if (!con.resa.empty()&&(last_resa==NoExists||last_resa<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(TotalsByDestination, PointIdTlg_t(point_id), system, info);
    UpdateCrsData_Resa(PointIdTlg_t(point_id), CrsSender_t(info.sender), system, con.resa);
  };

  if (!con.transit.empty()&&(last_tranzit==NoExists||last_tranzit<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(TranzitElement, PointIdTlg_t(point_id), system, info);
    UpdateCrsData(TranzitElement, PointIdTlg_t(point_id), CrsSender_t(info.sender),
                  system, con.transit);
  };

  if (!con.avail.empty()&&(last_avail==NoExists||last_avail<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(SpaceAvailableElement, PointIdTlg_t(point_id), system, info);
    UpdateCrsData(SpaceAvailableElement, PointIdTlg_t(point_id), CrsSender_t(info.sender),
                  system, con.avail);
  };

  if (!con.cfg.empty()&&(last_cfg==NoExists||last_cfg<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(Configuration, PointIdTlg_t(point_id), system, info);
    UpdateCrsData(Configuration, PointIdTlg_t(point_id), CrsSender_t(info.sender),
                  system, con.cfg);
  };

  if (!con.rbd.empty()&&(last_rbd==NoExists||last_rbd<=info.time_create))
  {
    UpdateCrsDataStat(ClassCodes, PointIdTlg_t(point_id), system, info);
    Qry.Clear();
    Qry.SQLText=
      "INSERT INTO crs_rbd(point_id, sender, system, fare_class, compartment, view_order) "
      "VALUES(:point_id, :sender, :system, :fare_class, :compartment, :view_order) ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("system",otString,system);
    Qry.DeclareVariable("fare_class",otString);
    Qry.DeclareVariable("compartment",otString);
    Qry.DeclareVariable("view_order",otInteger);

    int view_order=1;
    for(vector<TRbdItem>::const_iterator i=con.rbd.begin(); i!=con.rbd.end(); ++i)
    {
      for(string::const_iterator j=i->rbds.begin(); j!=i->rbds.end(); ++j)
      {
        Qry.SetVariable("fare_class", string(1,*j));
        Qry.SetVariable("compartment", i->subcl);
        Qry.SetVariable("view_order", view_order++);
        Qry.Execute();
      };
    };
  };

  if (pr_recount) crs_recount(point_id,NoExists,true);

  ASTRA::commit();

  class ParseLater {};

  try
  {
    if (pr_save_ne)
    {
      //��।����, ���� �� PNL ��� ��஢�
      bool pr_ne=false;
      int seats=0;
      for(iTotals=con.resa.begin();iTotals!=con.resa.end()&&!pr_ne;iTotals++)
      {
        seats+=iTotals->seats+iTotals->pad;
        for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end()&&!pr_ne;iPnrItem++)
        {
          TPnrItem& pnr=*iPnrItem;
          if (!pnr.ne.empty()) pr_ne=true;
        };
      };
      if (pr_ne)
      {
        //����稬 �����䨪��� �࠭���樨
        Qry.Clear();
        Qry.SQLText="SELECT cycle_tid__seq.nextval AS tid FROM dual";
        Qry.Execute();
        int tid=Qry.FieldAsInteger("tid");

        TQuery CrsPnrQry(&OraSession);
        CrsPnrQry.Clear();
        CrsPnrQry.SQLText=
          "SELECT crs_pnr.pnr_id FROM pnr_addrs,crs_pnr "
          "WHERE crs_pnr.pnr_id=pnr_addrs.pnr_id AND "
          "      point_id=:point_id AND system=:system AND sender=:sender AND "
          "      airp_arv=:airp_arv AND subclass=:subclass AND "
          "      pnr_addrs.airline=:pnr_airline AND pnr_addrs.addr=:pnr_addr";
        CrsPnrQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrQry.CreateVariable("sender",otString,info.sender);
        CrsPnrQry.CreateVariable("system",otString,system);
        CrsPnrQry.DeclareVariable("airp_arv",otString);
        CrsPnrQry.DeclareVariable("subclass",otString);
        CrsPnrQry.DeclareVariable("pnr_airline",otString);
        CrsPnrQry.DeclareVariable("pnr_addr",otString);

        TQuery CrsPnrInsQry(&OraSession);
        CrsPnrInsQry.Clear();
        CrsPnrInsQry.SQLText=
          "BEGIN "
          "  IF :pnr_id IS NULL THEN "
          "    SELECT crs_pnr__seq.nextval INTO :pnr_id FROM dual; "
          "    INSERT INTO crs_pnr(pnr_id,point_id,system,sender,airp_arv,subclass,class,grp_name,status,priority,tid) "
          "    VALUES(:pnr_id,:point_id,:system,:sender,:airp_arv,:subclass,:class,:grp_name,:status,:priority,cycle_tid__seq.currval); "
          "  ELSE "
          "    UPDATE crs_pnr SET grp_name=NVL(:grp_name,grp_name), "
          "                       status=:status, priority=:priority, "
          "                       tid=cycle_tid__seq.currval "
          "    WHERE pnr_id= :pnr_id; "
          "  END IF; "
          "END;";
        CrsPnrInsQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrInsQry.CreateVariable("sender",otString,info.sender);
        CrsPnrInsQry.CreateVariable("system",otString,system);
        CrsPnrInsQry.DeclareVariable("airp_arv",otString);
        CrsPnrInsQry.DeclareVariable("subclass",otString);
        CrsPnrInsQry.DeclareVariable("class",otString);
        CrsPnrInsQry.DeclareVariable("grp_name",otString);
        CrsPnrInsQry.DeclareVariable("status",otString);
        CrsPnrInsQry.DeclareVariable("priority",otString);
        CrsPnrInsQry.DeclareVariable("pnr_id",otInteger);

        TQuery CrsPaxInsQry(&OraSession);
        CrsPaxInsQry.Clear();
        CrsPaxInsQry.SQLText=
          "BEGIN "
          "  IF :pax_id IS NULL THEN "
          "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
          "  ELSE "
          "    UPDATE crs_pax "
          "    SET pnr_id=:pnr_id, pers_type= :pers_type, seat_xname= :seat_xname, seat_yname= :seat_yname, seat_rem= :seat_rem, "
          "        seat_type= :seat_type, seats=:seats, bag_pool= :bag_pool, pr_del= :pr_del, last_op= :last_op, tid=cycle_tid__seq.currval, "
          "        unique_reference=:unique_reference "
          "    WHERE pax_id=:pax_id; "
          "    IF SQL%FOUND THEN RETURN; END IF; "
          "  END IF; "
          "  INSERT INTO crs_pax(pax_id,pnr_id,surname,name,pers_type,seat_xname,seat_yname,seat_rem, "
          "    seat_type,seats,bag_pool,sync_chkd,pr_del,last_op,tid,unique_reference,orig_subclass,orig_class) "
          "  SELECT :pax_id,:pnr_id,:surname,:name,:pers_type,:seat_xname,:seat_yname,:seat_rem,:seat_type, "
          "    :seats,:bag_pool,0,:pr_del,:last_op,cycle_tid__seq.currval,:unique_reference,subclass,class "
          "  FROM crs_pnr WHERE pnr_id=:pnr_id; "
          "END;";
        CrsPaxInsQry.DeclareVariable("pax_id",otInteger);
        CrsPaxInsQry.DeclareVariable("pnr_id",otInteger);
        CrsPaxInsQry.DeclareVariable("surname",otString);
        CrsPaxInsQry.DeclareVariable("name",otString);
        CrsPaxInsQry.DeclareVariable("pers_type",otString);
        CrsPaxInsQry.DeclareVariable("seat_xname",otString);
        CrsPaxInsQry.DeclareVariable("seat_yname",otString);
        CrsPaxInsQry.DeclareVariable("seat_rem",otString);
        CrsPaxInsQry.DeclareVariable("seat_type",otString);
        CrsPaxInsQry.DeclareVariable("seats",otInteger);
        CrsPaxInsQry.DeclareVariable("bag_pool",otString);
        CrsPaxInsQry.DeclareVariable("pr_del",otInteger);
        CrsPaxInsQry.DeclareVariable("last_op",otDate);
        CrsPaxInsQry.DeclareVariable("unique_reference",otString);

        TQuery CrsInfInsQry(&OraSession);
        CrsInfInsQry.Clear();
        CrsInfInsQry.SQLText=
          "BEGIN"
          "  INSERT INTO crs_inf(inf_id, pax_id) VALUES(:inf_id, :pax_id); "
          "  UPDATE crs_pax SET inf_id=:inf_id WHERE pax_id=:pax_id; "
          "  DELETE FROM crs_inf_deleted WHERE inf_id=:inf_id; "
          "END;";
        CrsInfInsQry.DeclareVariable("pax_id",otInteger);
        CrsInfInsQry.DeclareVariable("inf_id",otInteger);

        DB::TQuery CrsTransferQry(PgOra::getRWSession("CRS_TRANSFER"));
        CrsTransferQry.SQLText=
          "INSERT INTO crs_transfer(pnr_id,transfer_num,airline,flt_no,suffix,local_date,airp_dep,airp_arv,subclass) "
          "VALUES(:pnr_id,:transfer_num,:airline,:flt_no,:suffix,:local_date,:airp_dep,:airp_arv,:subclass)";
        CrsTransferQry.DeclareVariable("pnr_id",otInteger);
        CrsTransferQry.DeclareVariable("transfer_num",otInteger);
        CrsTransferQry.DeclareVariable("airline",otString);
        CrsTransferQry.DeclareVariable("flt_no",otInteger);
        CrsTransferQry.DeclareVariable("suffix",otString);
        CrsTransferQry.DeclareVariable("local_date",otInteger);
        CrsTransferQry.DeclareVariable("airp_dep",otString);
        CrsTransferQry.DeclareVariable("airp_arv",otString);
        CrsTransferQry.DeclareVariable("subclass",otString);

        TQuery PnrMarketFltQry(&OraSession);
        PnrMarketFltQry.Clear();
        PnrMarketFltQry.SQLText=
          "INSERT INTO pnr_market_flt(pnr_id,airline,flt_no,suffix,local_date,airp_dep,airp_arv,subclass) "
          "VALUES(:pnr_id,:airline,:flt_no,:suffix,:local_date,:airp_dep,:airp_arv,:subclass)";
        PnrMarketFltQry.DeclareVariable("pnr_id",otInteger);
        PnrMarketFltQry.DeclareVariable("airline",otString);
        PnrMarketFltQry.DeclareVariable("flt_no",otInteger);
        PnrMarketFltQry.DeclareVariable("suffix",otString);
        PnrMarketFltQry.DeclareVariable("local_date",otInteger);
        PnrMarketFltQry.DeclareVariable("airp_dep",otString);
        PnrMarketFltQry.DeclareVariable("airp_arv",otString);
        PnrMarketFltQry.DeclareVariable("subclass",otString);

        TQuery PnrAddrsQry(&OraSession);
        PnrAddrsQry.Clear();
        PnrAddrsQry.SQLText=
          "BEGIN "
          "  UPDATE pnr_addrs SET addr=:addr WHERE pnr_id=:pnr_id AND airline=:airline; "
          "  IF SQL%NOTFOUND THEN "
          "    INSERT INTO pnr_addrs(pnr_id,airline,addr) VALUES(:pnr_id,:airline,:addr); "
          "  END IF; "
          "END;";
        PnrAddrsQry.DeclareVariable("pnr_id",otInteger);
        PnrAddrsQry.DeclareVariable("airline",otString);
        PnrAddrsQry.DeclareVariable("addr",otString);

        int pnr_id;
        bool pr_sync_pnr;
        set<int> paxIdsModified;
        ModifiedPaxRem modifiedPaxRem(paxPnl);
        ModifiedPaxRem modifiedASVCAndPD(paxPnl);
        ModifiedPax newOrCancelledPax(paxPnl);
        PaxCalcData::ChangesHolder paxCalcDataChanges(paxPnl);
        PaxIdsForDeleteTlgSeatRanges paxIdsForDeleteTlgSeatRanges;
        PaxIdsForInsertTlgSeatRanges paxIdsForInsertTlgSeatRanges;
        bool chkd_exists=false;
        int point_id_spp = ASTRA::NoExists;
        TAdvTripInfoList trips;
        if ( !isPRL ) {
          getTripsByPointIdTlg(point_id, trips);
          if (!trips.empty())
            point_id_spp = trips.front().point_id;
        }
       for(bool delIndicatorPass : {true, false})
        for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
        {
          boost::optional<TPaxSegmentPair> segmentPair;
          if (point_id_spp != ASTRA::NoExists)
            segmentPair=boost::in_place(point_id_spp, iTotals->dest);

          CrsPnrQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrInsQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("class",EncodeClass(iTotals->cl));

          for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
          {
            TPnrItem& pnr=*iPnrItem;
            TPnrItem::IndicatorsPresence indicatorsPresence=pnr.getIndicatorsPresence();
            if (!(indicatorsPresence==TPnrItem::Various ||
                  (indicatorsPresence==TPnrItem::OnlyDEL && delIndicatorPass) ||
                  (indicatorsPresence==TPnrItem::WithoutDEL && !delIndicatorPass))) continue;

            pr_sync_pnr=true;
            pnr_id=0;
            //���஡����� ���� pnr_id �� PNR reference
            for(iPnrAddr=pnr.addrs.begin();iPnrAddr!=pnr.addrs.end();iPnrAddr++)
            {
              CrsPnrQry.SetVariable("pnr_airline",iPnrAddr->airline);
              CrsPnrQry.SetVariable("pnr_addr",iPnrAddr->addr);
              CrsPnrQry.Execute();
              if (CrsPnrQry.RowCount()>0)
              {
                pr_sync_pnr=false;
                if (pnr_id!=0&&pnr_id!=CrsPnrQry.FieldAsInteger("pnr_id"))
                  throw ETlgError("More than one group found (PNR=%s/%s)",
                                  iPnrAddr->airline,iPnrAddr->addr);
                pnr_id=CrsPnrQry.FieldAsInteger("pnr_id");
                CrsPnrQry.Next();
                if (!CrsPnrQry.Eof)
                  throw ETlgError("More than one group found (PNR=%s/%s)",
                                  iPnrAddr->airline,iPnrAddr->addr);
              };
            };
            if (pnr_id==0)
            {
              list<int> ids;
              list<int>::iterator i;
              bool pr_chg_del=false;
              Qry.Clear();
              Qry.SQLText=
                "SELECT DISTINCT crs_pnr.pnr_id "
                "FROM crs_pnr,crs_pax "
                "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
                "      point_id=:point_id AND system=:system AND sender=:sender AND "
                "      airp_arv=:airp_arv AND subclass=:subclass AND "
                "      surname=:surname AND (name= :name OR :name IS NULL AND name IS NULL) AND "
                "      seats>0 "
                "ORDER BY crs_pnr.pnr_id";
              Qry.CreateVariable("point_id",otInteger,point_id);
              Qry.CreateVariable("sender",otString,info.sender);
              Qry.CreateVariable("system",otString,system);
              Qry.CreateVariable("airp_arv",otString,iTotals->dest);
              Qry.CreateVariable("subclass",otString,iTotals->subcl);
              Qry.DeclareVariable("surname",otString);
              Qry.DeclareVariable("name",otString);
              //���� ��㯯� �� �ᥬ ���ᠦ�ࠬ ������ ��㯯�
              if (!forcibly)
              {
                for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
                {
                  TNameElement& ne=*iNameElement;
                  if (ne.indicator==CHG||ne.indicator==DEL)
                  {
                    pr_chg_del=true;
                    Qry.SetVariable("surname",ne.surname);
                    for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
                    {
                      Qry.SetVariable("name",iPaxItem->name);
                      Qry.Execute();
                      if (ids.empty())
                      {
                        for(;!Qry.Eof;Qry.Next()) ids.push_back(Qry.FieldAsInteger("pnr_id"));
                      }
                      else
                      {
                        for(i=ids.begin();i!=ids.end()&&!Qry.Eof;)
                        {
                          if (*i<Qry.FieldAsInteger("pnr_id")) i=ids.erase(i);
                          else
                          {
                            if (*i==Qry.FieldAsInteger("pnr_id")) i++;
                            Qry.Next();
                          };
                        };
                      };
                      if (ids.size()<=1)
                      {
                        if (!ids.empty()) pnr_id=*ids.begin();
                        else throw ParseLater();
                        break;
                      };
                    };
                    if (iPaxItem!=ne.pax.end()) break;
                  };
                };
                if (pr_chg_del&&pnr_id==0) throw ParseLater();
              }
              else
              {
                for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
                {
                  TNameElement& ne=*iNameElement;
                  if (ne.indicator==CHG||ne.indicator==DEL)
                  {
                    pr_chg_del=true;
                    Qry.SetVariable("surname",ne.surname);
                    for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
                    {
                      Qry.SetVariable("name",iPaxItem->name);
                      Qry.Execute();
                      if (ids.empty())
                      {
                        for(;!Qry.Eof;Qry.Next()) ids.push_back(Qry.FieldAsInteger("pnr_id"));
                      }
                      else
                      {
                        for(i=ids.begin();i!=ids.end()&&!Qry.Eof;)
                        {
                          if (*i<Qry.FieldAsInteger("pnr_id")) i=ids.erase(i);
                          else
                          {
                            if (*i==Qry.FieldAsInteger("pnr_id")) i++;
                            Qry.Next();
                          };
                        };
                        if (ids.empty()) break; //���ᠦ��� � ࠧ��� ��㯯��
                      };
                    };
                    if (iPaxItem!=ne.pax.end()) break;
                  };
                };
                if (!ids.empty()) pnr_id=*ids.begin();  //��६ ����� ���室���� ��㯯�
              };
            };

            //ᮧ���� ����� ��㯯� ��� �஠������� �����
            CrsPnrInsQry.SetVariable("grp_name",pnr.grp_name.substr(0,64));
            CrsPnrInsQry.SetVariable("status",pnr.status);
            CrsPnrInsQry.SetVariable("priority",pnr.priority);
            if (pnr_id==0)
              CrsPnrInsQry.SetVariable("pnr_id",FNull);
            else
              CrsPnrInsQry.SetVariable("pnr_id",pnr_id);
            CrsPnrInsQry.Execute();
            pnr_id=CrsPnrInsQry.GetVariableAsInteger("pnr_id");
            CrsPaxInsQry.SetVariable("pnr_id",pnr_id);
            PnrAddrsQry.SetVariable("pnr_id",pnr_id);


            for(iPnrAddr=pnr.addrs.begin();iPnrAddr!=pnr.addrs.end();iPnrAddr++)
            {
              PnrAddrsQry.SetVariable("airline",iPnrAddr->airline);
              PnrAddrsQry.SetVariable("addr",iPnrAddr->addr);
              PnrAddrsQry.Execute();
            };

            for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
            {
              TNameElement& ne=*iNameElement;
              if ((ne.indicator==DEL && !delIndicatorPass) ||
                  (ne.indicator!=DEL && delIndicatorPass)) continue;
              boost::optional<PaxId_t> seatsBlockingPaxId;
              for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
              {
                if (iPaxItem->dontSaveToDB(ne)) continue;

                boost::optional<SuitablePax> suitablePax;

                if (ne.indicator==ADD||ne.indicator==CHG||ne.indicator==DEL)
                {
                  SuitablePaxList suitablePaxList;
                  suitablePaxList.get(pnr_id, ne.surname, iPaxItem->name, false);
                  if (!suitablePaxList.empty())
                  {
                    //��諨 � PNR
                    const SuitablePax& pax = suitablePaxList.front();
                    if (pax.last_op > info.time_create) continue;
                    if ((ne.indicator==ADD && pax.deleted) ||
                        ne.indicator==CHG ||
                        ne.indicator==DEL)
                    {
                      suitablePax=pax;
                    };
                  }
                  else
                  {
                    if (ne.indicator==ADD)
                    {
                      suitablePaxList.get(iPaxItem->tkn, point_id, system, info.sender, *iTotals, ne.surname, iPaxItem->name, false);
                      if (!suitablePaxList.empty())
                      {
                        //��諨 �� ������ (᪮॥ �ᥣ� � ��㣮� PNR)
                        const SuitablePax& pax = suitablePaxList.front();
                        if (pax.last_op <= info.time_create &&
                            pax.deleted)
                        {
                          suitablePax=pax;
                          ProgTrace(TRACE5, "suitablePax: pax_id=%d", pax.paxId.get());
                        }
                      }
                    }

                    if (ne.indicator==CHG||ne.indicator==DEL)
                    {
                      if (!forcibly) throw ParseLater();
                    };
                  };
                };

                if (suitablePax)
                  iPaxItem->setSomeDataForSeatsBlocking(suitablePax.get().paxId, ne);
                else
                  iPaxItem->setSomeDataForSeatsBlocking(boost::none, ne);

                TSeatsBlockingList::iterator iSeatsBlocking=iPaxItem->seatsBlocking.begin();
                for(bool seatsBlockingPass=false;
                    !seatsBlockingPass || iSeatsBlocking!=iPaxItem->seatsBlocking.end();
                    seatsBlockingPass?++iSeatsBlocking:iSeatsBlocking,
                    seatsBlockingPass=true)
                {
                  if (seatsBlockingPass && (ne.indicator==DEL || !iSeatsBlocking->isCBBG())) continue;

                  const TPaxItem& paxItem=seatsBlockingPass?*iSeatsBlocking:*iPaxItem;

                  boost::optional<PaxId_t> suitablePaxIdOrSeatId;
                  if (seatsBlockingPass)
                  {
                    if (!seatsBlockingPaxId)
                      throw Exception("%s: seatsBlockingPaxId=boost::none!", __func__);
                    suitablePaxIdOrSeatId=TPaxItem::getNotUsedSeatBlockingId(seatsBlockingPaxId.get());
                    if (!suitablePaxIdOrSeatId)
                      throw Exception("%s: getNotUsedSeatBlockingId returned boost::none! (seatsBlockingPaxId=%d)",
                                      __FUNCTION__, seatsBlockingPaxId.get().get());
                  }
                  else
                  {
                    if (suitablePax) suitablePaxIdOrSeatId=boost::in_place(suitablePax.get().paxId.get());
                  }

                  if (!suitablePaxIdOrSeatId)
                    CrsPaxInsQry.SetVariable("pax_id",FNull);
                  else
                    CrsPaxInsQry.SetVariable("pax_id",suitablePaxIdOrSeatId.get().get());
                  CrsPaxInsQry.SetVariable("surname",ne.surname);
                  CrsPaxInsQry.SetVariable("name",paxItem.name);
                  CrsPaxInsQry.SetVariable("pers_type",EncodePerson(seatsBlockingPass?adult:paxItem.pers_type));
                  if (ne.indicator!=DEL && !paxItem.seat.Empty())
                  {
                    CrsPaxInsQry.SetVariable("seat_xname",paxItem.seat.line);
                    CrsPaxInsQry.SetVariable("seat_yname",paxItem.seat.row);
                    CrsPaxInsQry.SetVariable("seat_rem",paxItem.seat_rem);
                    if (strcmp(paxItem.seat_rem,"NSST")==0||
                        strcmp(paxItem.seat_rem,"NSSA")==0||
                        strcmp(paxItem.seat_rem,"NSSW")==0||
                        strcmp(paxItem.seat_rem,"SMST")==0||
                        strcmp(paxItem.seat_rem,"SMSA")==0||
                        strcmp(paxItem.seat_rem,"SMSW")==0)
                      CrsPaxInsQry.SetVariable("seat_type",paxItem.seat_rem);
                    else
                      CrsPaxInsQry.SetVariable("seat_type",FNull);
                  }
                  else
                  {
                    CrsPaxInsQry.SetVariable("seat_xname",FNull);
                    CrsPaxInsQry.SetVariable("seat_yname",FNull);
                    CrsPaxInsQry.SetVariable("seat_rem",FNull);
                    CrsPaxInsQry.SetVariable("seat_type",FNull);
                  };

                  CrsPaxInsQry.SetVariable("seats",(int)paxItem.seats);
                  if (isPRL && ne.bag_pool!=NoExists)
                    CrsPaxInsQry.SetVariable("bag_pool", ne.bag_pool);
                  else
                    CrsPaxInsQry.SetVariable("bag_pool", FNull);
                  if (ne.indicator==DEL)
                    CrsPaxInsQry.SetVariable("pr_del",1);
                  else
                    CrsPaxInsQry.SetVariable("pr_del",0);
                  CrsPaxInsQry.SetVariable("last_op",info.time_create);
                  CrsPaxInsQry.SetVariable("unique_reference", paxItem.uniqueReference());
                  CrsPaxInsQry.Execute();

                  PaxIdWithSegmentPair paxId(PaxId_t(CrsPaxInsQry.GetVariableAsInteger("pax_id")), segmentPair);

                  addPaxEvent(paxId,
                              seatsBlockingPass?true:(!suitablePax || suitablePax.get().deleted),
                              ne.indicator==DEL,
                              newOrCancelledPax);

                  if (!seatsBlockingPass) seatsBlockingPaxId=paxId();
                  if (ne.indicator==CHG||ne.indicator==DEL)
                  {
                    map<PaxId_t, TSeatsBlockingItem> seatBlockingItems=TPaxItem::getAndLockSeatBlockingItems(paxId());
                    set<PaxId_t> infantIds=TPaxItem::getAndLockInfantIds(paxId());

                    set<PaxId_t> seatOccupiedIds;
                    seatOccupiedIds.insert(paxId());
                    for(const auto& item : seatBlockingItems)
                    {
                      const PaxId_t& id=item.first;
                      seatOccupiedIds.insert(id);
                      if (item.second.isCBBG())
                        addPaxEvent(PaxIdWithSegmentPair(id, segmentPair), false, true, newOrCancelledPax);
                    }
                    for(const PaxId_t& id : infantIds)
                      addPaxEvent(PaxIdWithSegmentPair(id, segmentPair), false, true, newOrCancelledPax);

                    if (ne.indicator==DEL)
                    {
                      for(const PaxId_t& id : seatOccupiedIds)
                      {
                        paxIdsForDeleteTlgSeatRanges.add({cltPNLCkin,
                                                          cltPNLBeforePay,
                                                          cltPNLAfterPay,
                                                          cltProtCkin,
                                                          cltProtSelfCkin,
                                                          cltProtBeforePay,
                                                          cltProtAfterPay},
                                                         id.get());
                      }
                    }

                    const std::set<PaxId_t> infIdSet = CheckIn::loadInfIdSet(paxId(), true /*lock*/);
                    for (PaxId_t inf_id: infIdSet) {
                      TypeB::deleteCrsPaxChkd(inf_id);
                      DeleteFreeRem(inf_id.get());
                    }
                    const std::set<PaxId_t> seatIdSet = CheckIn::loadSeatIdSet(paxId(), true /*lock*/);
                    for (PaxId_t seat_id: seatIdSet) {
                      TypeB::deleteCrsPaxChkd(seat_id);
                      DeleteFreeRem(seat_id.get());
                    }
                    TypeB::deleteCrsPaxChkd(paxId());
                    DeleteFreeRem(paxId().get());

                    //㤠�塞 �� �� ���ᠦ���
                    Qry.Clear();
                    Qry.SQLText=
                      "DECLARE "
                      "  CURSOR cur IS "
                      "    SELECT inf_id FROM crs_inf WHERE pax_id=:pax_id FOR UPDATE; "
                      "  CURSOR cur2 IS "
                      "    SELECT seat_id FROM crs_seats_blocking WHERE pax_id=:pax_id AND pr_del=0 FOR UPDATE; "
                      "BEGIN "
                      "  FOR curRow IN cur LOOP "
                      "    INSERT INTO crs_inf_deleted(inf_id, pax_id) "
                      "    SELECT inf_id, pax_id FROM crs_inf WHERE inf_id=curRow.inf_id; "
                      "    DELETE FROM crs_inf WHERE inf_id=curRow.inf_id; "
                      "    UPDATE crs_pax "
                      "    SET pr_del=1, last_op=:last_op, tid=cycle_tid__seq.currval "
                      "    WHERE pax_id=curRow.inf_id AND pr_del=0; "
                      "  END LOOP; "
                      "  FOR curRow IN cur2 LOOP "
                      "    UPDATE crs_seats_blocking SET pr_del=1 WHERE seat_id=curRow.seat_id; "
                      "    UPDATE crs_pax "
                      "    SET pr_del=1, last_op=:last_op, tid=cycle_tid__seq.currval "
                      "    WHERE pax_id=curRow.seat_id AND pr_del=0; "
                      "  END LOOP; "
                      "  UPDATE crs_pax SET inf_id=NULL WHERE pax_id=:pax_id; "
                      "END;";
                    Qry.CreateVariable("pax_id", otInteger, paxId().get());
                    Qry.CreateVariable("last_op", otDate, info.time_create);
                    Qry.Execute();
                  };
                  if (ne.indicator!=DEL)
                  {
                    //��ࠡ�⪠ ������楢
                    CrsPaxInsQry.SetVariable("pers_type",EncodePerson(baby));
                    CrsPaxInsQry.SetVariable("seat_xname",FNull);
                    CrsPaxInsQry.SetVariable("seat_yname",FNull);
                    CrsPaxInsQry.SetVariable("seats",0);
                    CrsPaxInsQry.SetVariable("pr_del",0);
                    CrsPaxInsQry.SetVariable("last_op",info.time_create);
                    //�������� ���ᠦ��
                    CrsInfInsQry.SetVariable("pax_id",paxId().get());
                    for(TInfList::const_iterator iInfItem=paxItem.inf.begin();iInfItem!=paxItem.inf.end();++iInfItem)
                    {
                      boost::optional<SuitablePax> suitableInf;

                      if (ne.indicator==ADD||ne.indicator==CHG)
                      {
                        SuitablePaxList suitablePaxList;
                        for(bool searchInOwnPnr : {true, false})
                        {
                          searchInOwnPnr?
                                suitablePaxList.get(pnr_id, iInfItem->surname, iInfItem->name, true):
                                suitablePaxList.get(iInfItem->tkn, point_id, system, info.sender, *iTotals, iInfItem->surname, iInfItem->name, true);
                          for(const SuitablePax& pax : suitablePaxList)
                          {
                            if ((!searchInOwnPnr ||
                                 (pax.parentPaxId && pax.parentPaxId.get()==paxId())) &&
                                pax.last_op <= info.time_create &&
                                pax.deleted)
                            {
                              suitableInf=pax;
                              if (!searchInOwnPnr)
                                ProgTrace(TRACE5, "suitablePax: inf_id=%d", pax.paxId.get());
                              break;
                            }
                          }
                          if (suitableInf) break;
                        }
                      }

                      if (!suitableInf)
                        CrsPaxInsQry.SetVariable("pax_id",FNull);
                      else
                        CrsPaxInsQry.SetVariable("pax_id",suitableInf.get().paxId.get());
                      CrsPaxInsQry.SetVariable("surname",iInfItem->surname);
                      CrsPaxInsQry.SetVariable("name",iInfItem->name);
                      CrsPaxInsQry.SetVariable("unique_reference", iInfItem->uniqueReference());
                      CrsPaxInsQry.Execute();

                      PaxIdWithSegmentPair infId(PaxId_t(CrsPaxInsQry.GetVariableAsInteger("pax_id")), segmentPair);

                      addPaxEvent(infId,
                                  !suitableInf || suitableInf.get().deleted,
                                  false,
                                  newOrCancelledPax);

                      CrsInfInsQry.SetVariable("inf_id",infId().get());
                      CrsInfInsQry.Execute();
                      SavePNLADLRemarks(infId,iInfItem->rem);
                      SaveDOCSRem(infId,suitableInf,iInfItem->doc,paxItem.doc_extra, modifiedPaxRem);
                      SaveDOCORem(infId,suitableInf,iInfItem->doco, modifiedPaxRem);
                      SaveDOCARem(infId,suitableInf,iInfItem->doca, modifiedPaxRem);
                      SaveTKNRem(infId,suitableInf,iInfItem->tkn, modifiedPaxRem);
                      if (SaveCHKDRem(infId,iInfItem->chkd)) chkd_exists=true;
                      paxIdsModified.insert(infId().get());
                      if (suitableInf && suitableInf.get().classChanged)
                        onChangeClass(infId().get(), iTotals->cl);
                    };

                    DeletePDRem(paxId, paxItem.rem, ne.rem, modifiedASVCAndPD); //��易⥫쭮 �� ����� ६�ப

                    //६�ન ���ᠦ��
                    SavePNLADLRemarks(paxId,paxItem.rem);
                    SaveDOCSRem(paxId,suitablePax,paxItem.doc,paxItem.doc_extra, modifiedPaxRem);
                    SaveDOCORem(paxId,suitablePax,paxItem.doco, modifiedPaxRem);
                    SaveDOCARem(paxId,suitablePax,paxItem.doca, modifiedPaxRem);
                    SaveTKNRem(paxId,suitablePax,paxItem.tkn, modifiedPaxRem);
                    SaveFQTRem(paxId,suitablePax,paxItem.fqt,paxItem.fqt_extra, modifiedPaxRem);
                    if (SaveCHKDRem(paxId,paxItem.chkd)) chkd_exists=true;
                    paxIdsModified.insert(paxId().get());
                    if (suitablePax && suitablePax.get().classChanged)
                      onChangeClass(paxId().get(), iTotals->cl);

                    if (!seatsBlockingPass) paxItem.seatsBlocking.toDB(paxId().get());

                    SaveASVCRem(paxId, paxItem.asvc, modifiedASVCAndPD);

                    //६�ન, �� �ਢ易��� � ���ᠦ���
                    SavePNLADLRemarks(paxId,ne.rem);

                    paxIdsForDeleteTlgSeatRanges.erase(paxId().get()); //����� ࠭�� ��ࠡ������ ᥪ�� DEL � १���� 祣� �� �� 㤠���� �� ⥫��ࠬ��� ᫮�
                    if (!isPRL)
                    {
                      //⥪�騥 ᫮�
                      TlgSeatRanges currTlgSeatRanges;
                      currTlgSeatRanges.add(cltPNLCkin, paxItem.seatRanges);
                      TCompLayerType rem_layer=GetSeatRemLayer(pnr.market_flt.Empty()?con.flt.airline:
                                                                                      pnr.market_flt.airline,
                                                               paxItem.seat_rem);
                      if (rem_layer==cltPNLBeforePay ||
                          rem_layer==cltPNLAfterPay)
                        currTlgSeatRanges.add(rem_layer, TSeatRange(paxItem.seat));
                      //᫮�, �� �ਢ易��� � ���ᠦ���
                      currTlgSeatRanges.add(cltPNLCkin, ne.seatRanges);

                      //�।��騥 ᫮�
                      TlgSeatRanges priorTlgSeatRanges;
                      priorTlgSeatRanges.get({cltPNLCkin, cltPNLBeforePay, cltPNLAfterPay}, paxId().get());

                      if (currTlgSeatRanges!=priorTlgSeatRanges)
                      {
                        if (!priorTlgSeatRanges.empty())
                        {
                          priorTlgSeatRanges.dump("priorTlgSeatRanges (pax_id="+IntToString(paxId().get())+"):");
                          currTlgSeatRanges.dump("currTlgSeatRanges (pax_id="+IntToString(paxId().get())+"):");
                        }
                        //᫮� ࠧ�������, 㤠�塞 ����, ������塞 ����
                        paxIdsForDeleteTlgSeatRanges.add({cltPNLCkin, cltPNLBeforePay, cltPNLAfterPay}, paxId().get());
                        paxIdsForInsertTlgSeatRanges.add(iTotals->dest, currTlgSeatRanges, paxId().get());
                      }
                    }
                    else
                    {
                      TlgSeatRanges currTlgSeatRanges;
                      currTlgSeatRanges.add(cltPRLTrzt, paxItem.seatRanges);
                      //᫮�, �� �ਢ易��� � ���ᠦ���
                      currTlgSeatRanges.add(cltPRLTrzt, ne.seatRanges);
                      paxIdsForInsertTlgSeatRanges.add(iTotals->dest, currTlgSeatRanges, paxId().get());
                    }
                  };

                  if (!isPRL && !pr_sync_pnr)
                  {
                    //������ ᨭ�஭����� ���ᠦ�� � ஧�᪮�
                    rozysk::sync_crs_pax(paxId().get(), "", "");
                  };

                  if (isPRL && iPaxItem==ne.pax.begin())
                  {
                    //����襬 �����
                    if (!ne.bag.Empty() || !ne.tags.empty()) SaveDCSBaggage(paxId().get(), ne);
                  };
                }
              };
            }; //for(iNameElement=pnr.ne.begin()

            //����襬 ��몮���
            if (!pnr.ne.empty() && indicatorsPresence!=TPnrItem::OnlyDEL)
            {
              TypeB::deleteCrsTransfer(PnrId_t(pnr_id));
              //㤠�塞 ��䨣 �� ��몮���
              Qry.Clear();
              Qry.SQLText=
                "UPDATE crs_pnr SET tid=cycle_tid__seq.currval WHERE pnr_id= :pnr_id ";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
              CrsTransferQry.SetVariable("pnr_id",pnr_id);
              for(iTransfer=pnr.transfer.begin();iTransfer!=pnr.transfer.end();iTransfer++)
              {
                CrsTransferQry.SetVariable("transfer_num",(int)iTransfer->num);
                CrsTransferQry.SetVariable("airline",iTransfer->airline);
                CrsTransferQry.SetVariable("flt_no",(int)iTransfer->flt_no);
                CrsTransferQry.SetVariable("suffix",iTransfer->suffix);
                CrsTransferQry.SetVariable("local_date",(int)iTransfer->local_date);
                CrsTransferQry.SetVariable("airp_dep",iTransfer->airp_dep);
                CrsTransferQry.SetVariable("airp_arv",iTransfer->airp_arv);
                CrsTransferQry.SetVariable("subclass",iTransfer->subcl);
                CrsTransferQry.Execute();
              };
              const std::string airp_arv_final = getCrsTransfer_airp_arv_final(PnrId_t(pnr_id));
              Qry.Clear();
              Qry.SQLText=
                "UPDATE crs_pnr SET airp_arv_final=:airp_arv_final "
                "WHERE pnr_id=:pnr_id";
              Qry.CreateVariable("airp_arv_final",otString,airp_arv_final);
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
            };
            //����襬 �������᪨� ३�
            if (!pnr.ne.empty() && indicatorsPresence!=TPnrItem::OnlyDEL)
            {
              //㤠�塞 ��䨣 �।��騩
              Qry.Clear();
              Qry.SQLText=
                "BEGIN "
                "  DELETE FROM pnr_market_flt WHERE pnr_id= :pnr_id; "
                "  UPDATE crs_pnr SET tid=cycle_tid__seq.currval WHERE pnr_id= :pnr_id; "
                "END;";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();

              if (!pnr.market_flt.Empty())
              {
                PnrMarketFltQry.SetVariable("pnr_id",pnr_id);
                PnrMarketFltQry.SetVariable("airline",pnr.market_flt.airline);
                PnrMarketFltQry.SetVariable("flt_no",(int)pnr.market_flt.flt_no);
                PnrMarketFltQry.SetVariable("suffix",pnr.market_flt.suffix);
                PnrMarketFltQry.SetVariable("local_date",(int)pnr.market_flt.local_date);
                PnrMarketFltQry.SetVariable("airp_dep",pnr.market_flt.airp_dep);
                PnrMarketFltQry.SetVariable("airp_arv",pnr.market_flt.airp_arv);
                PnrMarketFltQry.SetVariable("subclass",pnr.market_flt.subcl);
                PnrMarketFltQry.Execute();
              };
            };

            if (!isPRL && pr_sync_pnr)
            {
              //������ ᨭ�஭����� �ᥩ ��㯯� � ஧�᪮�
              rozysk::sync_crs_pnr(pnr_id, "", "");
            };
          };//for(iPnrItem=iTotals->pnr.begin()
        };

       if (!isPRL)
       {
         TlgETDisplay(point_id, paxIdsModified, true);
         if (chkd_exists)
         {
           for(TAdvTripInfoList::const_iterator it = trips.begin(); it != trips.end(); it++)
             add_trip_task((*it).point_id, SYNC_NEW_CHKD, "");
         };

         PaxCalcData::addChanges(newOrCancelledPax, paxCalcDataChanges);
         PaxCalcData::addChanges(modifiedPaxRem, paxCalcDataChanges);

         //�����, �� PaxCalcData::addChanges ��뢠�� �� synchronizePaxEvents!
         synchronizePaxEvents(newOrCancelledPax, modifiedPaxRem);
         modifiedPaxRem.executeCallbacksByPaxId(TRACE5);
         newOrCancelledPax.executeCallbacksByPaxId(TRACE5);

         for(int id : paxIdsModified)
           TETickItem::syncOriginalSubclass(id);
       }

       bool lock=!paxIdsForDeleteTlgSeatRanges.empty() ||
                 !paxIdsForInsertTlgSeatRanges.empty() ||
                 (!isPRL && !modifiedASVCAndPD.empty());
       if (lock)
       {
         TFlights flightsForLock;
         flightsForLock.GetByPointIdTlg(point_id, ftTranzit);
         flightsForLock.Lock(__func__);
       }

       Timing::Points timing(__func__);

       if (!paxIdsForDeleteTlgSeatRanges.empty() ||
           !paxIdsForInsertTlgSeatRanges.empty())
       {
         TPointIdsForCheck point_ids_spp;
         timing.start("SyncTlgSeatRanges");
         paxIdsForDeleteTlgSeatRanges.handle(tid, point_ids_spp);
         paxIdsForInsertTlgSeatRanges.handle(point_id, tlg_id, tid, point_ids_spp);
         timing.finish("SyncTlgSeatRanges");
         check_layer_change(point_ids_spp, __func__);
       }

       if (!isPRL)
       {
         modifiedASVCAndPD.executeCallbacksByCategory(TRACE5);
         paxCalcDataChanges.executeCallbacksByPaxId(TRACE5);
       }
      } //if (pr_ne)

      if (!isPRL)
      {
        int pr_pnl_new=pr_pnl;
        if (strcmp(info.tlg_type,"PNL")==0)
        {
          //PNL
          if (pr_ne || !pr_numeric_pnl)
            //����஢��
            pr_pnl_new=2;
          else
            //��஢��
            if (pr_pnl!=2) pr_pnl_new=1; //�᫨ �� �⮣� �� �뫮 ����஢��� PNL
        };

        if (pr_pnl!=pr_pnl_new)
        {
          saveCrsDataStat_PrPnl(PointIdTlg_t(point_id), info.sender, pr_pnl_new);
        };
      };
    }
    else
    {
      if (strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2) return false;//throw ParseLater(); //�⫮��� ࠧ���
    };
    return true;
  }
  catch (const ParseLater&)
  {
    ASTRA::rollback();  //�⫮��� ࠧ���
    return false;
  };
};

// �᫨ ��� ࠧ����⥫�, � result �������� ���� val;
void split(vector<string> &result, const string val, char c)
{
    result.clear();
    size_t idx = val.find(c);
    if(idx != string::npos) {
        size_t idx1 = 0;
        while(idx != string::npos) {
            result.push_back(val.substr(idx1, idx - idx1));
            idx1 = idx + 1;
            idx = val.find(c, idx1);
        }
        result.push_back(val.substr(idx1, idx - idx1));
    } else
        result.push_back(val);
}

int monthAsNum(const std::string &smonth)
{

    typedef struct { const char *rus, *lat; } TMonthCode;

    static const TMonthCode Months[12] = {
        {"���","JAN"},
        {"���","FEB"},
        {"���","MAR"},
        {"���","APR"},
        {"���","MAY"},
        {"���","JUN"},
        {"���","JUL"},
        {"���","AUG"},
        {"���","SEP"},
        {"���","OCT"},
        {"���","NOV"},
        {"���","DEC"}};

    int mon = NoExists;
    for(mon=1;mon<=12;mon++)
        if (strcmp(smonth.c_str(),Months[mon-1].lat)==0||
                strcmp(smonth.c_str(),Months[mon-1].rus)==0) break;
    if(mon == NoExists)
        throw Exception("%s: cant convert month %s to num", smonth.c_str());
    return mon;
}

void TFlightIdentifier::parse(const char *val)
{
    string buf = val;
    size_t idx = buf.find('/');
    if(idx == string::npos)
        throw ETlgError("Flight Identifier: wrong format: %s", val);
    TFltInfo flt_info;
    flt_info.parse(buf.substr(0, idx).c_str());
    airline = flt_info.airline;
    flt_no = flt_info.flt_no;
    suffix = *flt_info.suffix;
    buf.erase(0, idx + 1);
    date = ParseDate(buf);

}

void TFlightIdentifier::dump()
{
    ProgTrace(TRACE5, "TFlightIdentifier::dump");
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %d", flt_no);
    ProgTrace(TRACE5, "suffix: %c", suffix);
    ProgTrace(TRACE5, "date: %s", DateTimeToStr(date, "dd.mm.yyyy").c_str());

}

TDateTime ParseDate(int day)
{
    return DayToDate(day, NowUTC() + 15, true);
}

// �� �室� ��ப� �ଠ� nn(aaa(nn))
TDateTime ParseDate(const string &buf)
{
    char sday[3];
    char smonth[4];
    char syear[3];
    char c;
    int res;
    int fmt = 1;
    while(fmt) {
        *sday = 0;
        *smonth = 0;
        *syear = 0;
        c = 0;
        switch(fmt) {
            case 1:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%3[A-Z�-��]%2[0-9]%c", sday, smonth, syear, &c);
                if(c != 0 or res != 3) fmt = 2;
                break;
            case 2:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%3[A-Z�-��]%c", sday, smonth, &c);
                if(c != 0 or res != 2) fmt = 3;
                break;
            case 3:
                fmt = 0;
                res = sscanf(buf.c_str(), "%2[0-9]%c", sday, &c);
                if(c != 0 or res != 1)
                    throw ETlgError("Flight Identifier: wrong date format: %s", buf.c_str());
                break;
        }
    }
    if(strlen(sday) != 2)
        throw ETlgError("Flight Identifier: wrond day %s", sday);
    if(*smonth and strlen(smonth) != 3)
        throw ETlgError("Flight Identifier: wrond month %s", smonth);
    if(*syear and strlen(syear) != 2)
        throw ETlgError("Flight Identifier: wrond year %s", syear);

    TDateTime today=NowUTC();

    TDateTime date;
    int year(NoExists), mon(NoExists), day(NoExists);
    StrToInt(sday, day);
    if(*smonth) mon = monthAsNum(smonth);
    if(*syear) {
        StrToInt(syear, year);
        EncodeDate(year,mon,day,date);
    } else if(*smonth) {
        date = DayMonthToDate(day, mon, today, dateEverywhere);
    } else {
        date = ParseDate(day);
    }

    return date;
}

std::optional<PnrId_t> getPnrIdByPaxId(const PaxId_t& pax_id)
{
  CheckIn::TSimplePaxItem pax_item;
  if (pax_item.getCrsByPaxId(pax_id)) {
    return PnrId_t(pax_item.pnr_id);
  }
  return {};
}

void TTransferRoute::getById(int id, bool isPnrId)
{
  clear();

  const std::optional<PnrId_t> pnr_id = isPnrId ? PnrId_t(id)
                                                : getPnrIdByPaxId(PaxId_t(id));
  if (!pnr_id) {
    return;
  }

  DB::TCachedQuery Qry(PgOra::getROSession("CRS_TRANSFER"),
        "SELECT * FROM crs_transfer "
        "WHERE pnr_id=:id "
        "AND transfer_num>0 "
        "ORDER BY transfer_num",
        QParams() << QParam("id", otInteger, pnr_id->get()));

  Qry.get().Execute();
  for(;!Qry.get().Eof;Qry.get().Next())
  {
    TypeB::TTransferItem trferItem;
    trferItem.num=Qry.get().FieldAsInteger("transfer_num");
    strcpy(trferItem.airline, Qry.get().FieldAsString("airline").c_str());
    trferItem.flt_no=Qry.get().FieldAsInteger("flt_no");
    strcpy(trferItem.suffix, Qry.get().FieldAsString("suffix").c_str());
    trferItem.local_date=Qry.get().FieldAsInteger("local_date");
    strcpy(trferItem.airp_dep, Qry.get().FieldAsString("airp_dep").c_str());
    strcpy(trferItem.airp_arv, Qry.get().FieldAsString("airp_arv").c_str());
    strcpy(trferItem.subcl, Qry.get().FieldAsString("subclass").c_str());
    push_back(trferItem);
  }
}

void TTransferRoute::getByPaxId(int pax_id)
{
  getById(pax_id, false);
}

void TTransferRoute::getByPnrId(int pnr_id)
{
  getById(pnr_id, true);
}

}

