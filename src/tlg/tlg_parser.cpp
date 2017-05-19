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
#include "ssm_parser.h"
#include "astra_consts.h"
#include "../astra_misc.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "oralib.h"
#include "misc.h"
#include "tlg.h"
#include "convert.h"
#include "seats_utils.h"
#include "salons.h"
#include "memory_manager.h"
#include "comp_layers.h"
#include "flt_binding.h"
#include "rozysk.h"
#include "alarms.h"
#include "trip_tasks.h"
#include "remarks.h"
#include "apps_interaction.h"
#include "etick.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

namespace TypeB
{

const TMonthCode Months[12] =
    {{"���","JAN"},
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
bool ParseCHDRem(TTlgParser &tlg,string &rem_text,vector<TChdItem> &chd);
bool ParseINFRem(TTlgParser &tlg,string &rem_text,vector<TInfItem> &inf);
bool ParseSEATRem(TTlgParser &tlg,string &rem_text,TSeatRanges &seats);
bool ParseTKNRem(TTlgParser &tlg,string &rem_text,TTKNItem &tkn);
bool ParseFQTRem(TTlgParser &tlg,string &rem_text,TFQTItem &fqt);

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
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
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
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
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
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
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

char* TlgElemToElemId(TElemType type, const char* elem, char* id, bool with_icao)
{
  TElemFmt fmt;
  string id2;

  id2=ElemToElemId(type, elem, fmt, false);
  if (fmt==efmtUnknown)
    throw EBaseTableError("TlgElemToElemId: elem not found (type=%s, elem=%s)",
                          EncodeElemType(type),elem);
  if (id2.empty())
    throw EBaseTableError("TlgElemToElemId: id is empty (type=%s, elem=%s)",
                          EncodeElemType(type),elem);
  if (!with_icao && (fmt==efmtCodeICAONative || fmt==efmtCodeICAOInter))
    throw EBaseTableError("TlgElemToElemId: ICAO only elem found (type=%s, elem=%s)",
                          EncodeElemType(type),elem);

  strcpy(id,id2.c_str());
  return id;
};

char* GetAirline(char* airline, bool with_icao)
{
  try
  {
    return TlgElemToElemId(etAirline,airline,airline,with_icao);
  }
  catch (EBaseTableError)
  {
    throw ETlgError("Unknown airline code '%s'", airline);
  };
};

char* GetAirp(char* airp, bool with_icao=false)
{
  try
  {
    return TlgElemToElemId(etAirp,airp,airp,with_icao);
  }
  catch (EBaseTableError)
  {
    throw ETlgError("Unknown airport code '%s'", airp);
  };
};

TClass GetClass(const char* subcl)
{
  try
  {
    char subclh[2];
    TlgElemToElemId(etSubcls,subcl,subclh);
    return DecodeClass(getBaseTable(etSubcls).get_row("code",subclh).AsString("cl").c_str());
  }
  catch (EBaseTableError)
  {
    throw ETlgError("Unknown subclass '%s'", subcl);
  };
};

char* GetSubcl(char* subcl)
{
  try
  {
    return TlgElemToElemId(etSubcls,subcl,subcl);
  }
  catch (EBaseTableError)
  {
    throw ETlgError("Unknown subclass '%s'", subcl);
  };
};

char GetSuffix(char &suffix)
{
  if (suffix!=0)
  {
    char suffixh[2];
    suffixh[0]=suffix;
    suffixh[1]=0;
    try
    {
      TlgElemToElemId(etSuffix,suffixh,suffixh);
    }
    catch (EBaseTableError)
    {
      throw ETlgError("Unknown flight number suffix '%c'", suffix);
    };
    suffix=suffixh[0];
  };
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
    catch (EBaseTableError)
    {
      char country[3];
      TlgElemToElemId(etCountry,elem,country);
      //�饬 � pax_doc_countries
      strcpy(id,getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code").c_str());
    };
  }
  catch (EBaseTableError)
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
  if (strcmp(tlg_type,"MVT")==0) cat=tcAHM;
  if (strcmp(tlg_type,"SSM")==0) cat=tcSSM;
  if (strcmp(tlg_type,"ASM")==0) cat=tcASM;
  if (strcmp(tlg_type,"LCI")==0) cat=tcLCI;
  if (strcmp(tlg_type,"UCM")==0) cat=tcUCM;
  if (strcmp(tlg_type,"CPM")==0) cat=tcCPM;
  if (strcmp(tlg_type,"SLS")==0) cat=tcSLS;
  if (strcmp(tlg_type,"LDM")==0) cat=tcLDM;
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

            TDateTime today = Now();
            int year,mon,currday;
            DecodeDate(today,year,mon,currday);
            try
            {
              for(mon=1;mon<=12;mon++)
                if (strcmp(month,Months[mon-1].lat)==0||
                    strcmp(month,Months[mon-1].rus)==0) break;
              EncodeDate(year,mon,day,flt.scd);
              if ((int)today>(int)IncMonth(flt.scd,6))  //���-����
                EncodeDate(year+1,mon,day,flt.scd);
            }
            catch(EConvertError)
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
    catch (ETlgError E)
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
  catch(ETlgError E)
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
  catch (ETlgError E)
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

            TDateTime today=Now();
            int year,mon,currday;
            DecodeDate(today,year,mon,currday);
            try
            {
              for(mon=1;mon<=12;mon++)
                if (strcmp(month,Months[mon-1].lat)==0||
                    strcmp(month,Months[mon-1].rus)==0) break;
              EncodeDate(year,mon,day,info.flt.scd);
              if ((int)today>(int)IncMonth(info.flt.scd,6))  //���-����
                EncodeDate(year+1,mon,day,info.flt.scd);
              info.flt.pr_utc=false;
            }
            catch(EConvertError)
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
  catch(ETlgError E)
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
            //��ॢ���� day � TDateTime
            //�.�. ࠧ����� ��⥬��� �६�� � �ନ஢�⥫� � �ਥ�騪�, ���⮬� +1!
            flt.scd = DayToDate(day, NowUTC() + 1, true);
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
  catch(ETlgError E)
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
                  catch(EConvertError)
                  {
                    throw ETlgError("Can't convert creation time");
                  };
                }
                catch(ETlgError) {};
              };
              p=tlg.GetLexeme(p);
            };
          };
          e=MessageIdentifier;
          break;
        case MessageIdentifier:
          //message identifier
          c=0;
          res=sscanf(tlg.lex,"%3[A-Z]%c",infoh.tlg_type,&c);
          if (c!=0||res!=1||tlg.GetLexeme(p)!=NULL)
          {
            *(infoh.tlg_type)=0;
          }
          else
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
            case tcSSM:
              info = new TSSMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseSSMHeading(heading,*(TSSMHeadingInfo*)info);
              break;
            case tcASM:
              info = new TSSMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseSSMHeading(heading,*(TSSMHeadingInfo*)info);
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
              info = new TUCMHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseUCMHeading(heading,*(TUCMHeadingInfo*)info,flts);
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
  catch(ETlgError E)
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
  char endtlg[7];

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
  catch(ETlgError E)
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
  catch(ETlgError E)
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
            catch(EConvertError)
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
  catch (ETlgError E)
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
                catch(EConvertError)
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
  catch (ETlgError E)
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

bool isBlockSeatsRem(const string &rem_code)
{
  return rem_code=="EXST" ||
         rem_code=="STCR";
};

bool isNotAdditionalSeatRem(const string &rem_code)
{
  return rem_code=="DOCA" ||
         rem_code=="DOCO" ||
         rem_code=="DOCS" ||
         rem_code=="PSPT" ||
         rem_code=="CHLD" ||
         rem_code=="CHD" ||
         rem_code=="INFT" ||
         rem_code=="INF" ||
         rem_code=="TKNM" ||
         rem_code=="TKNA" ||
         rem_code=="TKNE" ||
         rem_code=="TKNO" ||
         rem_code=="FQTR" ||
         rem_code=="FQTV" ||
         rem_code=="FQTU" ||
         rem_code=="FQTS" ||
         rem_code=="CHKD" ||
         rem_code=="ASVC";
};

bool isAdditionalSeat(const TPaxItem &pax)
{
  if (!isBlockSeatsRem(pax.name)) return false;
  for(vector<TRemItem>::const_iterator iRemItem=pax.rem.begin();
                                       iRemItem!=pax.rem.end();
                                       ++iRemItem)
    if (isNotAdditionalSeatRem(iRemItem->code)) return false;
  return true;
};

vector<TPaxItem>::const_iterator findPaxForBlockSeats(vector<TPaxItem>::const_iterator zzPax,
                                                      const TPnrItem &pnr,
                                                      vector<TNameElement>::const_iterator ne)
{
  vector<TPaxItem>::const_iterator minSeatsPax=zzPax;
  for(vector<TNameElement>::const_iterator iNameElement=pnr.ne.begin();
                                           iNameElement!=pnr.ne.end();
                                           ++iNameElement)
  {
    if (ne!=pnr.ne.end() && ne!=iNameElement) continue;
    if (iNameElement->surname=="ZZ") continue;
    for(vector<TPaxItem>::const_iterator iPaxItem=iNameElement->pax.begin();
                                         iPaxItem!=iNameElement->pax.end();
                                         ++iPaxItem)
    {
      if (iPaxItem==zzPax) continue;
      if (isAdditionalSeat(*iPaxItem)) continue; //�ய�᪠�� �������� �������⥫�� ����
      for(vector<TRemItem>::const_iterator iRemItem=iPaxItem->rem.begin();
                                           iRemItem!=iPaxItem->rem.end();
                                           ++iRemItem)
        if (iRemItem->code==zzPax->name)
        {
          if (minSeatsPax==zzPax || iPaxItem->seats < minSeatsPax->seats) minSeatsPax=iPaxItem;
          break;
        };
    };
  };
  return minSeatsPax;
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
              if (ne.surname=="ZZ")
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
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();++iPnrItem)
        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
          BindRemarks(tlg,*i);

    //B. �஠������஢��� �������⥫�� ���� ���ᠦ�஢ (��᫥ �ਢ離�, �� �� ࠧ��� ६�ப!)
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
    {
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();++iPnrItem)
      {
        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
        {
          //��� ��室� � ࠬ��� ���ᠦ�஢ ������ NameElement:
          //1. ���⠢�塞 EXST � STCR ��� �� ���ᠦ�஢, � ������ ᮮ⢥�����騥 ���� ६�ન
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (isAdditionalSeat(*iPaxItem))
            {
              vector<TPaxItem>::const_iterator minSeatsPax=findPaxForBlockSeats(iPaxItem, *iPnrItem, i);
              if (minSeatsPax!=iPaxItem)
              {
                TPaxItem &p=(TPaxItem&)*minSeatsPax;
                if (p.seats>=3) break; //����� ��� ����� �ॢ���� ���-�� ���� 3
                p.seats++;
                p.rem.insert(p.rem.end(),
                             iPaxItem->rem.begin(),iPaxItem->rem.end());
                iPaxItem=i->pax.erase(iPaxItem);
                continue;
              };
            };
            ++iPaxItem;
          };
          //2. ���⠢�塞 EXST � STCR ��� ���ᠦ�஢ �����।�⢥��� ��। EXST/STCR
          iPaxItemPrev=i->pax.end();
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (isAdditionalSeat(*iPaxItem))
            {
              if (iPaxItemPrev!=i->pax.end() && iPaxItemPrev->seats<3)
              {
                //EXST/STCR �� ���� � NameElement � �।��騩 ���ᠦ�� �������� ����� 3 ����
                iPaxItemPrev->seats++;
                //��ॡ�ᨬ ६�ન
                iPaxItemPrev->rem.insert(iPaxItemPrev->rem.end(),
                                         iPaxItem->rem.begin(),iPaxItem->rem.end());
                iPaxItem=i->pax.erase(iPaxItem);
                continue;
              };
            }
            else
            {
              iPaxItemPrev=iPaxItem;
            };
            ++iPaxItem;
          };
        };
        // ���⠢�塞 EXST � STCR ��� �� ���ᠦ�஢, � ������ ᮮ⢥�����騥 ���� ६�ન
        // � ࠬ��� ������ PNR??? � PRL???
        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
        {
          if (i->surname!="ZZ") continue;
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (isAdditionalSeat(*iPaxItem))
            {
              vector<TPaxItem>::const_iterator minSeatsPax=findPaxForBlockSeats(iPaxItem, *iPnrItem, iPnrItem->ne.end());
              if (minSeatsPax!=iPaxItem)
              {
                TPaxItem &p=(TPaxItem&)*minSeatsPax;
                if (p.seats>=3) break; //����� ��� ����� �ॢ���� ���-�� ���� 3
                p.seats++;
                p.rem.insert(p.rem.end(),
                             iPaxItem->rem.begin(),iPaxItem->rem.end());
                iPaxItem=i->pax.erase(iPaxItem);
                continue;
              };
            };
            ++iPaxItem;
          };
        };
      };
    };

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

        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
        {
          ParseRemarks(seat_rem_priority,tlg,info,*iPnrItem,*i);
        };
      };
    };
  }
  catch (ETlgError E)
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
                catch(ETlgError)
                {
                  ProgTrace(TRACE5, "%-30s: ETlgError!", s.str().c_str());
                };
              };
*/
    if (elementID=='M')
    {
      GetAirline(Transfer.airline);
      GetAirp(Transfer.airp_dep);
      GetAirp(Transfer.airp_arv);
      GetSubcl(Transfer.subcl);
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

void BindRemarks(TTlgParser &tlg, TNameElement &ne)
{
  char c;
  int res,k;
  string::size_type pos;
  bool pr_parse;
  string strh;
  char rem_code[7],numh[4];
  int num;
  vector<TRemItem>::iterator iRemItem;
  vector<TPaxItem>::iterator iPaxItem;

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
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();)
  {
    if (iRemItem->text.empty())
    {
      ++iRemItem;
      continue;
    };

    //�饬 ��뫪� �� ���ᠦ�� �� ᮢ������� 䠬����
    vector<string> names;
    pos=iRemItem->text.find_last_of('-');
    while(pos!=string::npos)
    {
      strh=iRemItem->text.substr(pos+1);
      try
      {
        ParseNameElement(strh.c_str(),names,epOptional);
        if (!names.empty()&&*(names.begin())==ne.surname) break; //��諨
      }
      catch(ETlgError &E) {};

      if (pos!=0)
        pos=iRemItem->text.find_last_of('-',pos-1);
      else
        pos=string::npos;
    };

    pr_parse=false;
    if (pos!=string::npos)
    {
      pr_parse=true;
      for(vector<string>::iterator i=names.begin();i!=names.end();++i)
      {
        if (i!=names.begin())
        {
          for(k=0;k<=1;k++)
          {
            for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();++iPaxItem)
            {
              if ((k==0&&iPaxItem->name!=*i)||
                  (k!=0&&OnlyAlphaInLexeme(iPaxItem->name)!=OnlyAlphaInLexeme(*i))) continue;
              if (iPaxItem->rem.empty())
              {
                iPaxItem->rem.push_back(TRemItem());
                break;
              }
              else
              {
                TRemItem& RemItem=iPaxItem->rem.back();
                if (RemItem.text.empty()) continue;
                else
                {
                  iPaxItem->rem.push_back(TRemItem());
                  break;
                };
              };
            };
            if (iPaxItem!=ne.pax.end()) break;
          };
          if (k>1&&iPaxItem==ne.pax.end())
          {
            pr_parse=false;
            break;
          };
        };
      };

      if (pr_parse) iRemItem->text.erase(pos); //���� ����砭�� ६�ન ��᫥ -
    }
    else
    {
      //�� ��諨 ��뫪� �� ���ᠦ��
      if (ne.pax.size()!=1)
      {
        ++iRemItem;
        continue;
      };

      iPaxItem=ne.pax.begin();
      if (iPaxItem->rem.empty() ||
          !iPaxItem->rem.back().text.empty())
      {
        iPaxItem->rem.push_back(TRemItem());
      };
      pr_parse=true;
    };

    for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();++iPaxItem)
    {
      if (iPaxItem->rem.empty()) continue;
      TRemItem& RemItem=iPaxItem->rem.back();
      if (RemItem.text.empty())
      {
        if (pr_parse)
        {
          RemItem.text=iRemItem->text;
          strcpy(RemItem.code,iRemItem->code);
        }
        else
          iPaxItem->rem.pop_back();
      };
    };
    if (pr_parse)
      iRemItem=ne.rem.erase(iRemItem);
    else
      ++iRemItem;
  };
};

bool ParseDOCSRem(TTlgParser &tlg, TDateTime scd_local, std::string &rem_text, TDocItem &doc);
bool ParseDOCORem(TTlgParser &tlg, TDateTime scd_local, std::string &rem_text, TDocoItem &doc);
bool ParseDOCARem(TTlgParser &tlg, string &rem_text, TDocaItem &doca);
bool ParseCHKDRem(TTlgParser &tlg, string &rem_text, TCHKDItem &chkd);
bool ParseASVCRem(TTlgParser &tlg, string &rem_text, TASVCItem &asvc);
bool ParseOTHS_DOCSRem(TTlgParser &tlg, string &rem_text, TDocExtraItem &doc);
bool ParseOTHS_FQTSTATUSRem(TTlgParser &tlg, string &rem_text, TFQTExtraItem &fqt);
void BindDetailRem(TRemItem &remItem, bool isGrpRem, TPaxItem &paxItem,
                   const TDetailRemAncestor &item)
{
  if (item.pr_inf)
  {
    if (paxItem.inf.size()==1)
    {
      for(int pass=0; pass<6 ;pass++)
      try
      {
        switch(pass)
        {
          case 0: paxItem.inf.begin()->doc.push_back(dynamic_cast<const TDocItem&>(item)); break;
          case 1: paxItem.inf.begin()->doco.push_back(dynamic_cast<const TDocoItem&>(item)); break;
          case 2: paxItem.inf.begin()->doca.push_back(dynamic_cast<const TDocaItem&>(item)); break;
          case 3: paxItem.inf.begin()->tkn.push_back(dynamic_cast<const TTKNItem&>(item)); break;
          case 4: paxItem.inf.begin()->chkd.push_back(dynamic_cast<const TCHKDItem&>(item)); break;
          case 5: break;
          default: return;
        };
        break;
      }
      catch(std::bad_cast) {};
      paxItem.inf.begin()->rem.push_back(remItem);
      remItem.text.clear();
    };
  }
  else
  {
    for(int pass=0; pass<6 ;pass++)
    try
    {
      switch(pass)
      {
        case 0: paxItem.doc.push_back(dynamic_cast<const TDocItem&>(item)); break;
        case 1: paxItem.doco.push_back(dynamic_cast<const TDocoItem&>(item)); break;
        case 2: paxItem.doca.push_back(dynamic_cast<const TDocaItem&>(item)); break;
        case 3: paxItem.tkn.push_back(dynamic_cast<const TTKNItem&>(item)); break;
        case 4: paxItem.chkd.push_back(dynamic_cast<const TCHKDItem&>(item)); break;
        case 5: paxItem.asvc.push_back(dynamic_cast<const TASVCItem&>(item)); break;
        default: return;
      };
      break;
    }
    catch(std::bad_cast) {};
    if (isGrpRem)
    {
      paxItem.rem.push_back(remItem);
      remItem.text.clear();
    };
  };

};

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
              vector<TInfItem> inf;
              if (ParseINFRem(tlg,iRemItem->text,inf))
              {
                for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
                {
                  if (i->surname.empty()) i->surname=ne.surname;
                  //�஢�ਬ ���� �� 㦥 ॡ���� � ⠪�� ������ � 䠬����� � iPaxItem->inf
                  //(���� �� �㡫�஢���� INF � INFT)
                  vector<TInfItem>::iterator j;
                  for(j=iPaxItem->inf.begin();j!=iPaxItem->inf.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=iPaxItem->inf.end())
                  {
                    //�㡫�஢���� INF. 㤠��� �� inf
                    i=inf.erase(i);
                    continue;
                  };
                  i++;
                };
                iPaxItem->inf.insert(iPaxItem->inf.end(),inf.begin(),inf.end());
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
                if (*(doc.no)!=0)
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

            if (iRemItem->code!=r->first) continue;
            TSeatRanges seats;
            TSeat seat;
            if (ParseSEATRem(tlg,iRemItem->text,seats))
            {
              sort(seats.begin(),seats.end());
              iPaxItem->seatRanges.insert(iPaxItem->seatRanges.end(),seats.begin(),seats.end());
              if (iPaxItem->seat.Empty())
              {
                //�᫨ ���� �� ��।�����, ���஡����� �ਢ易��
                for(TSeatRanges::iterator i=seats.begin();i!=seats.end();i++)
                {
                  seat=i->first;
                  do
                  {
                    //������� ���� �� seat, �᫨ �� ������� �� � ���� ��㣮��
                    for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                      if (!iPaxItem2->seat.Empty() && iPaxItem2->seat==seat) break;
                    if (iPaxItem2==ne.pax.end())
                    {
                      iPaxItem->seat=seat;
                      strcpy(iPaxItem->seat_rem,i->rem);
                      break;
                    };
                  }
                  while (NextSeatInRange(*i,seat));
                };
              };
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

  vector<TInfItem> infs;
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
            vector<TInfItem> inf;
            if (ParseINFRem(tlg,iRemItem->text,inf))
            {
              for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
              {
                if (i->surname.empty()) i->surname=ne.surname;
                if (ne.pax.size()==1 && ne.pax.begin()->inf.empty())
                {
                  //�஢�ਬ ���� �� 㦥 ॡ���� � ⠪�� ������ � 䠬����� � ne.pax.begin()->inf
                  //(���� �� �㡫�஢���� INF � INFT)
                  vector<TInfItem>::iterator j;
                  for(j=ne.pax.begin()->inf.begin();j!=ne.pax.begin()->inf.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=ne.pax.begin()->inf.end())
                  {
                    //�㡫�஢���� INF. 㤠��� �� inf
                    i=inf.erase(i);
                    continue;
                  };
                };
                i++;
              };

              if (ne.pax.size()==1 && ne.pax.begin()->inf.empty() &&
                  inf.size()==1)
              {
                //�������筠� �ਢ離� � �����⢥����� ���ᠦ���
                ne.pax.begin()->inf.insert(ne.pax.begin()->inf.end(),inf.begin(),inf.end());
              }
              else
              {
                for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
                {
                  //�஢�ਬ ���� �� 㦥 ॡ���� � ⠪�� ������ � 䠬����� � infs
                  //(���� �� �㡫�஢���� INF � INFT)
                  vector<TInfItem>::iterator j;
                  for(j=infs.begin();j!=infs.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=infs.end())
                  {
                    //�㡫�஢���� INF. 㤠��� �� inf
                    i=inf.erase(i);
                    continue;
                  };
                  i++;
                };
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
                if (*(doc.no)!=0)
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
          TSeat seat;
          if (ParseSEATRem(tlg,iRemItem->text,seats))
          {
            sort(seats.begin(),seats.end());
            ne.seatRanges.insert(ne.seatRanges.end(),seats.begin(),seats.end());

            //���஡㥬 �ਢ易��
            for(TSeatRanges::iterator i=seats.begin();i!=seats.end();i++)
            {
              seat=i->first;
              do
              {
                //������� ���� �� seat, �᫨ �� ������� �� � ���� ��㣮�� �� ��㯯�
                for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
                {
                  for(iPaxItem2=iNameElement->pax.begin();iPaxItem2!=iNameElement->pax.end();iPaxItem2++)
                    if (!iPaxItem2->seat.Empty() && iPaxItem2->seat==seat) break;
                  if (iPaxItem2!=iNameElement->pax.end()) break;
                };
                if (iNameElement==pnr.ne.end())
                {
                  //���� �� � ���� �� ��㯯� �� �������
                  //���� ���ᠦ��, ���஬� �� ���⠢���� ����
                  if (strcmp(iRemItem->code,"EXST")==0)
                  {
                    for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                      if (iPaxItem2->seat.Empty()&&
                          iPaxItem2->name=="EXST")
                      {
                        iPaxItem2->seat=seat;
                        break;
                      };
                    if (iPaxItem2!=ne.pax.end()) continue;
                  };

                  //� ����� ��।� ��।��塞 ���� ⥪�饬� NameElement
                  for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                    if (iPaxItem2->seat.Empty())
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
                      if (iPaxItem2->seat.Empty())
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
    for(int k=0;k<=3;k++)
    {
      //0. �ਢ�뢠�� � ��ࢮ�� �� ᢮������� �� inf
      //1. �ਢ�뢠�� � ��ࢮ�� ��
      //2. �ਢ�뢠�� � ��ࢮ�� ᢮������� �� inf
      //3. �ਢ�뢠�� � ��ࢮ��
      for(vector<TPaxItem>::iterator i=ne.pax.begin();i!=ne.pax.end();i++)
      {
        if ((k==0 && i->pers_type==adult && i->inf.empty()) ||
            (k==1 && i->pers_type==adult) ||
            (k==2 && i->inf.empty()) ||
            k==3)
        {
          i->inf.push_back(*iInfItem);
          break;
        };
      };
    };
  };
};

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

bool ParseSEATRem(TTlgParser &tlg,string &rem_text,TSeatRanges &seats)
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
      catch(EConvertError)
      {
        //�������� �� ࠧ��ࠫ�� - �� � �७ � ���
        usePriorContext=false;
      };
    };
    return true;
  };
  return false;
};

bool ParseCHDRem(TTlgParser &tlg,string &rem_text,vector<TChdItem> &chd)
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
      catch(ETlgError &E)
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


bool ParseINFRem(TTlgParser &tlg,string &rem_text,vector<TInfItem> &inf)
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
      catch(ETlgError &E)
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

bool ParseDOCSRem(TTlgParser &tlg, TDateTime scd_local, string &rem_text, TDocItem &doc)
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
      catch(exception &E)
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
    catch(ETlgError &E)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseDOCORem(TTlgParser &tlg, TDateTime scd_local, string &rem_text, TDocoItem &doc)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseDOCARem(TTlgParser &tlg, string &rem_text, TDocaItem &doca)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",doca.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseCHKDRem(TTlgParser &tlg, string &rem_text, TCHKDItem &chkd)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",chkd.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseASVCRem(TTlgParser &tlg, string &rem_text, TASVCItem &asvc)
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
      catch(exception &E)
      {
        switch(k)
        {
          case 0:
            *asvc.rem_status=0;
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",asvc.rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseOTHS_DOCSRem(TTlgParser &tlg, string &rem_text, TDocExtraItem &doc)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseOTHS_FQTSTATUSRem(TTlgParser &tlg, string &rem_text, TFQTExtraItem &fqt)
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
      catch(exception &E)
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
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",rem_code,E.what(),rem_text.c_str());
    };
    return true;
  };

  return false;
};

bool ParseTKNRem(TTlgParser &tlg,string &rem_text,TTKNItem &tkn)
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
      catch(exception &E)
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
            tkn.coupon_no=0;
            throw ETlgError("coupon number: %s",E.what());
        };
      };
    }
    catch(ETlgError &E)
    {
      ProgTrace(TRACE0,"Non-critical .R/%s error: %s (%s)",tkn.rem_code,E.what(),rem_text.c_str());
      return false;
    };

    return true;
  };
  return false;
};

bool ParseFQTRem(TTlgParser &tlg,string &rem_text,TFQTItem &fqt)
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
      catch(exception &E)
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
    catch(ETlgError &E)
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
  catch(ETlgError E)
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
  int point_id;
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "  SELECT point_id FROM tlg_trips "
    "  WHERE airline=:airline AND flt_no=:flt_no AND "
    "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
    "      scd=:scd AND pr_utc=:pr_utc AND "
    "      (airp_dep IS NULL AND :airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (airp_arv IS NULL AND :airp_arv IS NULL OR airp_arv=:airp_arv) AND "
    "      bind_type=:bind_type";
  Qry.CreateVariable("airline",otString,flt.airline);
  Qry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
  Qry.CreateVariable("suffix",otString,flt.suffix);
  Qry.CreateVariable("scd",otDate,flt.scd);
  Qry.CreateVariable("pr_utc",otInteger,(int)flt.pr_utc);
  Qry.CreateVariable("airp_dep",otString,flt.airp_dep);
  Qry.CreateVariable("airp_arv",otString,flt.airp_arv);
  Qry.CreateVariable("bind_type",otInteger,(int)bind_type);
  Qry.Execute();
  if (Qry.Eof)
  {
    Qry.SQLText=
      "BEGIN "
      "  SELECT point_id.nextval INTO :point_id FROM dual; "
      "  INSERT INTO tlg_trips(point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type) "
      "  VALUES(:point_id,:airline,:flt_no,:suffix,:scd,:pr_utc,:airp_dep,:airp_arv,:bind_type); "
      "END;";
    Qry.DeclareVariable("point_id",otInteger);
    Qry.Execute();
    point_id=Qry.GetVariableAsInteger("point_id");
    TTlgBinding(false, search_params).bind_flt(point_id);

    /* ����� �஢�ਬ ���ਢ易��� ᥣ����� �� crs_displace � �ਢ殮� */
    /* �� ⮫쪮 ��� PNL/ADL */
    if (!flt.pr_utc && bind_type==btFirstSeg && *flt.airp_arv==0)
    {
      Qry.Clear();
      Qry.SQLText=
        "UPDATE crs_displace2 SET point_id_tlg=:point_id "
        "WHERE airline=:airline AND flt_no=:flt_no AND "
        "      NVL(suffix,' ')=NVL(:suffix,' ') AND "
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
  else {
      point_id=Qry.FieldAsInteger("point_id");
      TTlgBinding(false, search_params).bind_flt(point_id);
  }


  bool has_errors=error_type!=tlgeNotError;
  bool has_alarm_errors=error_type==tlgeNotMonitorYesAlarm ||
                        error_type==tlgeYesMonitorYesAlarm;
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tlg_source(point_id_tlg,tlg_id,has_errors,has_alarm_errors) "
    "VALUES(:point_id_tlg,:tlg_id,:has_errors,:has_alarm_errors)";
  Qry.CreateVariable("point_id_tlg",otInteger,point_id);
  Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  Qry.CreateVariable("has_errors",otInteger,(int)has_errors);
  Qry.CreateVariable("has_alarm_errors",otInteger,(int)has_alarm_errors);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
    if (has_errors || has_alarm_errors)
    {
      Qry.SQLText=
          "UPDATE tlg_source "
          "SET has_errors=DECODE(:has_errors,0,has_errors,:has_errors), "
          "    has_alarm_errors=DECODE(:has_alarm_errors,0,has_alarm_errors,:has_alarm_errors) "
          "WHERE point_id_tlg=:point_id_tlg AND tlg_id=:tlg_id ";
      Qry.Execute();
    };
  };
  check_tlg_in_alarm(point_id, NoExists);
  return point_id;
};

bool isDeleteTypeBContent(int point_id, const THeadingInfo& info)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (strcmp(info.tlg_type,"SOM")==0)
  {
    Qry.SQLText=
      "SELECT MAX(time_create) AS max_time_create "
      "FROM tlgs_in,tlg_source "
      "WHERE tlg_source.tlg_id=tlgs_in.id AND "
      "      tlg_source.point_id_tlg=:point_id AND "
      "      NVL(tlg_source.has_errors,0)=0 AND "
      "      tlgs_in.type=:tlg_type";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("tlg_type",otString,info.tlg_type);
  } else
  if (strcmp(info.tlg_type,"BTM")==0 ||
      strcmp(info.tlg_type,"PTM")==0)
  {
    Qry.SQLText=
      "SELECT MAX(time_create) AS max_time_create "
      "FROM tlgs_in,tlg_source "
      "WHERE tlg_source.tlg_id=tlgs_in.id AND "
      "      tlg_source.point_id_tlg=:point_id_in AND "
      "      NVL(tlg_source.has_errors,0)=0 AND "
      "      tlgs_in.type=:tlg_type";
    Qry.CreateVariable("point_id_in",otInteger,point_id);
    Qry.CreateVariable("tlg_type",otString,info.tlg_type);
  } else
  if (strcmp(info.tlg_type,"PRL")==0)
  {
    Qry.SQLText=
      "SELECT last_data AS max_time_create "
      "FROM typeb_data_stat "
      "WHERE point_id=:point_id AND system=:system AND sender=:sender";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,"DCS");
    Qry.CreateVariable("sender",otString,info.sender);
  } else return false;
  Qry.Execute();
  return Qry.Eof ||
         Qry.FieldIsNULL("max_time_create") ||
         Qry.FieldAsDateTime("max_time_create") <= info.time_create;
};

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
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  ckin.delete_typeb_data(:point_id, :system, :sender, TRUE); "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,"DCS");
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.Execute();
    return true;
  };
  return false;
};

bool DeletePTMBTMContent(int point_id_in, const THeadingInfo& info)
{
  if (isDeleteTypeBContent(point_id_in, info))
  {
    TQuery TrferQry(&OraSession);
    TrferQry.SQLText=
      "SELECT tlg_transfer.trfer_id "
      "FROM tlgs_in,tlg_transfer "
      "WHERE tlgs_in.id=tlg_transfer.tlg_id AND tlgs_in.type=:tlg_type AND "
      "      tlg_transfer.point_id_in=:point_id_in ";
    TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
    TrferQry.CreateVariable("tlg_type",otString,info.tlg_type);
    TrferQry.Execute();

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  DELETE FROM "
      "    (SELECT * FROM trfer_grp,trfer_pax "
      "     WHERE trfer_grp.grp_id=trfer_pax.grp_id AND trfer_grp.trfer_id=:trfer_id); "
      "  DELETE FROM "
      "    (SELECT * FROM trfer_grp,trfer_tags "
      "     WHERE trfer_grp.grp_id=trfer_tags.grp_id AND trfer_grp.trfer_id=:trfer_id); "
      "  DELETE FROM "
      "    (SELECT * FROM trfer_grp,tlg_trfer_onwards "
      "     WHERE trfer_grp.grp_id=tlg_trfer_onwards.grp_id AND trfer_grp.trfer_id=:trfer_id); "
      "  DELETE FROM "
      "    (SELECT * FROM trfer_grp,tlg_trfer_excepts "
      "     WHERE trfer_grp.grp_id=tlg_trfer_excepts.grp_id AND trfer_grp.trfer_id=:trfer_id); "
      "  DELETE FROM trfer_grp WHERE trfer_id=:trfer_id; "
      "  DELETE FROM tlg_transfer WHERE trfer_id=:trfer_id; "
      "END;";
    Qry.DeclareVariable("trfer_id",otInteger);

    for(;!TrferQry.Eof;TrferQry.Next())
    {
      Qry.SetVariable("trfer_id",TrferQry.FieldAsInteger("trfer_id"));
      Qry.Execute();
    };
    return true;
  };
  return false;
};

void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, const TBtmContent& con)
{
  TQuery TrferQry(&OraSession);
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(cycle_id__seq.nextval,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.DeclareVariable("point_id_in",otInteger);
  TrferQry.DeclareVariable("subcl_in",otString);
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  TQuery GrpQry(&OraSession);
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(pax_grp__seq.nextval,cycle_id__seq.currval,NULL,:bag_amount,:bag_weight,:rk_weight,:weight_unit) ";
  GrpQry.DeclareVariable("bag_amount",otInteger);
  GrpQry.DeclareVariable("bag_weight",otInteger);
  GrpQry.DeclareVariable("rk_weight",otInteger);
  GrpQry.DeclareVariable("weight_unit",otString);

  TQuery PaxQry(&OraSession);
  PaxQry.SQLText=
    "INSERT INTO trfer_pax(grp_id,surname,name) VALUES(pax_grp__seq.currval,:surname,:name)";
  PaxQry.DeclareVariable("surname",otString);
  PaxQry.DeclareVariable("name",otString);

  TQuery TagQry(&OraSession);
  TagQry.SQLText=
    "INSERT INTO trfer_tags(grp_id,no) VALUES(pax_grp__seq.currval,:no)";
  TagQry.DeclareVariable("no",otFloat);

  TQuery OnwardQry(&OraSession);
  OnwardQry.SQLText=
    "INSERT INTO tlg_trfer_onwards(grp_id, num, airline, flt_no, suffix, local_date, airp_dep, airp_arv, subclass) "
    "VALUES(pax_grp__seq.currval, :num, :airline, :flt_no, :suffix, :local_date, :airp_dep, :airp_arv, :subclass)";
  OnwardQry.DeclareVariable("num", otInteger);
  OnwardQry.DeclareVariable("airline", otString);
  OnwardQry.DeclareVariable("flt_no", otInteger);
  OnwardQry.DeclareVariable("suffix", otString);
  OnwardQry.DeclareVariable("local_date", otDate);
  OnwardQry.DeclareVariable("airp_dep", otString);
  OnwardQry.DeclareVariable("airp_arv", otString);
  OnwardQry.DeclareVariable("subclass", otString);

  TQuery ExceptQry(&OraSession);
  ExceptQry.SQLText=
    "INSERT INTO tlg_trfer_excepts(grp_id, type) VALUES(pax_grp__seq.currval, :type)";
  ExceptQry.DeclareVariable("type", otString);

  TQuery AlarmQry(&OraSession);
  AlarmQry.Clear();
  AlarmQry.SQLText=
    "SELECT DISTINCT tlg_binding_out.point_id_spp "
    "FROM tlgs_in, tlg_transfer, tlg_binding tlg_binding_out "
    "WHERE tlgs_in.id=tlg_transfer.tlg_id AND tlgs_in.type=:tlg_type AND "
    "      tlg_transfer.point_id_in=:point_id_in AND "
    "      tlg_transfer.point_id_out=tlg_binding_out.point_id_tlg ";
  AlarmQry.CreateVariable("tlg_type", otString, "BTM");
  AlarmQry.DeclareVariable("point_id_in", otInteger);

  int point_id_in,point_id_out;
  set<int> alarm_point_ids;
  for(vector<TBtmTransferInfo>::const_iterator iIn=con.Transfer.begin();iIn!=con.Transfer.end();++iIn)
  {
    point_id_in=SaveFlt(tlg_id,iIn->InFlt,btLastSeg,TSearchFltInfoPtr());
    //������ point_id_spp ३ᮢ, ����� ��ࠢ����� �࠭���
    set<int> point_ids;
    AlarmQry.SetVariable("point_id_in", point_id_in);
    AlarmQry.Execute();
    for(;!AlarmQry.Eof;AlarmQry.Next())
      point_ids.insert(AlarmQry.FieldAsInteger("point_id_spp"));

    if (!DeletePTMBTMContent(point_id_in,info)) continue;
    TrferQry.SetVariable("point_id_in",point_id_in);
    TrferQry.SetVariable("subcl_in",iIn->InFlt.subcl);
    for(vector<TBtmOutFltInfo>::const_iterator iOut=iIn->OutFlt.begin();iOut!=iIn->OutFlt.end();++iOut)
    {
      point_id_out=SaveFlt(tlg_id,*iOut,btFirstSeg,TSearchFltInfoPtr());
      TrferQry.SetVariable("point_id_out",point_id_out);
      TrferQry.SetVariable("subcl_out",iOut->subcl);
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
        GrpQry.Execute();
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
        for(vector<TBSMTagItem>::const_iterator iTag=iGrp->tags.begin();iTag!=iGrp->tags.end();++iTag)
          for(int j=0;j<iTag->num;j++)
          {
            TagQry.SetVariable("no",iTag->first_no+j);
            TagQry.Execute();
          };
        int num=1;
        string prior_airp_arv=iOut->airp_arv;
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
        for(set<string>::const_iterator iExcept=iGrp->excepts.begin();iExcept!=iGrp->excepts.end();++iExcept)
        {
          ExceptQry.SetVariable("type", *iExcept);
          ExceptQry.Execute();
        };
      };
    };

    //������ point_id_spp ३ᮢ, ����� ��ࠢ����� �࠭���
    AlarmQry.SetVariable("point_id_in", point_id_in);
    AlarmQry.Execute();
    for(;!AlarmQry.Eof;AlarmQry.Next())
      point_ids.insert(AlarmQry.FieldAsInteger("point_id_spp"));

    alarm_point_ids.insert(point_ids.begin(), point_ids.end());
  };

  for(set<int>::const_iterator id=alarm_point_ids.begin(); id!=alarm_point_ids.end(); ++id)
  {
    check_unattached_trfer_alarm(*id);
    //ProgTrace(TRACE5, "SaveBTMContent: check_unattached_trfer_alarm(%d)", *id);
  };
};

void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con)
{
  vector<TPtmOutFltInfo>::iterator iOut;
  vector<TPtmTransferData>::iterator iData;
  vector<string>::iterator i;
  int point_id_in,point_id_out;

  point_id_in=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(con.InFlt),btLastSeg,TSearchFltInfoPtr());
  if (!DeletePTMBTMContent(point_id_in,info)) return;

  TQuery TrferQry(&OraSession);
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(cycle_id__seq.nextval,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
  TrferQry.CreateVariable("subcl_in",otString,FNull); //�������� �ਫ�� �������⥭
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  TQuery GrpQry(&OraSession);
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(pax_grp__seq.nextval,cycle_id__seq.currval,:seats,:bag_amount,:bag_weight,NULL,:weight_unit) ";
  GrpQry.DeclareVariable("seats",otInteger);
  GrpQry.DeclareVariable("bag_amount",otInteger);
  GrpQry.DeclareVariable("bag_weight",otInteger);
  GrpQry.DeclareVariable("weight_unit",otString);

  TQuery PaxQry(&OraSession);
  PaxQry.SQLText=
    "INSERT INTO trfer_pax(grp_id,surname,name) VALUES(pax_grp__seq.currval,:surname,:name)";
  PaxQry.DeclareVariable("surname",otString);
  PaxQry.DeclareVariable("name",otString);

  for(iOut=con.OutFlt.begin();iOut!=con.OutFlt.end();iOut++)
  {
    point_id_out=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(*iOut),btFirstSeg,TSearchFltInfoPtr());
    TrferQry.SetVariable("point_id_out",point_id_out);
    TrferQry.SetVariable("subcl_out",iOut->subcl);
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
      GrpQry.Execute();
      if (iData->surname.empty()) continue;
      PaxQry.SetVariable("surname",iData->surname);
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

void SavePNLADLRemarks(int pax_id, vector<TRemItem> &rem)
{
  if (rem.empty()) return;
  TQuery CrsPaxRemQry(&OraSession);
  CrsPaxRemQry.Clear();
  CrsPaxRemQry.SQLText=
    "INSERT INTO crs_pax_rem(pax_id,rem,rem_code) "
    "VALUES(:pax_id,:rem,:rem_code)";
  CrsPaxRemQry.DeclareVariable("pax_id",otInteger);
  CrsPaxRemQry.DeclareVariable("rem",otString);
  CrsPaxRemQry.DeclareVariable("rem_code",otString);
  //६�ન ���ᠦ��
  CrsPaxRemQry.SetVariable("pax_id",pax_id);
  vector<TRemItem>::iterator iRemItem;
  for(iRemItem=rem.begin();iRemItem!=rem.end();iRemItem++)
  {
    if (iRemItem->text.empty()) continue;
    if (iRemItem->text.size()>250) iRemItem->text.erase(250);
    CrsPaxRemQry.SetVariable("rem",iRemItem->text);
    CrsPaxRemQry.SetVariable("rem_code",iRemItem->code);
    CrsPaxRemQry.Execute();
  };
};

void SaveDOCSRem(int pax_id, const vector<TDocItem> &doc, const map<string, TDocExtraItem> &doc_extra)
{
  if (doc.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_doc "
    "  (pax_id,rem_code,rem_status,type,issue_country,no,nationality, "
    "   birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi, "
    "   type_rcpt) "
    "VALUES "
    "  (:pax_id,:rem_code,:rem_status,:type,:issue_country,:no,:nationality, "
    "   :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi, "
    "   :type_rcpt) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("issue_country",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("nationality",otString);
  Qry.DeclareVariable("birth_date",otDate);
  Qry.DeclareVariable("gender",otString);
  Qry.DeclareVariable("expiry_date",otDate);
  Qry.DeclareVariable("surname",otString);
  Qry.DeclareVariable("first_name",otString);
  Qry.DeclareVariable("second_name",otString);
  Qry.DeclareVariable("pr_multi",otInteger);
  Qry.DeclareVariable("type_rcpt",otString);
  for(vector<TDocItem>::const_iterator i=doc.begin();i!=doc.end();i++)
  {
    if (i->Empty()) continue;
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("type",i->type);
    Qry.SetVariable("issue_country",i->issue_country);
    Qry.SetVariable("no",i->no);
    Qry.SetVariable("nationality",i->nationality);
    if (i->birth_date!=NoExists)
      Qry.SetVariable("birth_date",i->birth_date);
    else
      Qry.SetVariable("birth_date",FNull);
    Qry.SetVariable("gender",i->gender);
    if (i->expiry_date!=NoExists)
      Qry.SetVariable("expiry_date",i->expiry_date);
    else
      Qry.SetVariable("expiry_date",FNull);
    Qry.SetVariable("surname",i->surname.substr(0,64));
    Qry.SetVariable("first_name",i->first_name.substr(0,64));
    Qry.SetVariable("second_name",i->second_name.substr(0,64));
    Qry.SetVariable("pr_multi",(int)i->pr_multi);
    Qry.SetVariable("type_rcpt", FNull);
    if (*(i->no)!=0)
    {
      map<string, TDocExtraItem>::const_iterator j=doc_extra.find(i->no);
      if (j!=doc_extra.end()) Qry.SetVariable("type_rcpt", j->second.type_rcpt);
    };
    Qry.Execute();
  };
};

void SaveDOCORem(int pax_id, const vector<TDocoItem> &doc)
{
  if (doc.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_doco "
    "  (pax_id,rem_code,rem_status,birth_place,type,no,issue_place,issue_date, "
    "   applic_country) "
    "VALUES "
    "  (:pax_id,:rem_code,:rem_status,:birth_place,:type,:no,:issue_place,:issue_date, "
    "   :applic_country) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("birth_place",otString);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("issue_place",otString);
  Qry.DeclareVariable("issue_date",otDate);
  Qry.DeclareVariable("applic_country",otString);
  for(vector<TDocoItem>::const_iterator i=doc.begin();i!=doc.end();i++)
  {
    if (i->Empty()) continue;
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("birth_place",i->birth_place.substr(0,35));
    Qry.SetVariable("type",i->type);
    Qry.SetVariable("no",i->no);
    Qry.SetVariable("issue_place",i->issue_place.substr(0,35));
    if (i->issue_date!=NoExists)
      Qry.SetVariable("issue_date",i->issue_date);
    else
      Qry.SetVariable("issue_date",FNull);
    Qry.SetVariable("applic_country",i->applic_country);
    Qry.Execute();
  };
};

void SaveDOCARem(int pax_id, const vector<TDocaItem> &doca)
{
  if (doca.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_doca "
    "  (pax_id,rem_code,rem_status,type,country,address,city,region,postal_code) "
    "VALUES "
    "  (:pax_id,:rem_code,:rem_status,:type,:country,:address,:city,:region,:postal_code) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("country",otString);
  Qry.DeclareVariable("address",otString);
  Qry.DeclareVariable("city",otString);
  Qry.DeclareVariable("region",otString);
  Qry.DeclareVariable("postal_code",otString);
  for(vector<TDocaItem>::const_iterator i=doca.begin();i!=doca.end();i++)
  {
    if (i->Empty()) continue;
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("type",i->type);
    Qry.SetVariable("country",i->country);
    Qry.SetVariable("address",i->address.substr(0,35));
    Qry.SetVariable("city",i->city.substr(0,35));
    Qry.SetVariable("region",i->region.substr(0,35));
    Qry.SetVariable("postal_code",i->postal_code.substr(0,17));
    Qry.Execute();
  };
};

void SaveTKNRem(int pax_id, vector<TTKNItem> &tkn)
{
  if (tkn.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_tkn "
    "  (pax_id,rem_code,ticket_no,coupon_no) "
    "VALUES "
    "  (:pax_id,:rem_code,:ticket_no,:coupon_no) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("ticket_no",otString);
  Qry.DeclareVariable("coupon_no",otInteger);
  for(vector<TTKNItem>::iterator i=tkn.begin();i!=tkn.end();i++)
  {
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("ticket_no",i->ticket_no);
    if (i->coupon_no!=0)
      Qry.SetVariable("coupon_no",i->coupon_no);
    else
      Qry.SetVariable("coupon_no",FNull);
    Qry.Execute();
  };
};

bool SaveCHKDRem(int pax_id, const vector<TCHKDItem> &chkd)
{
  bool result=false;
  if (chkd.empty()) return result;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_chkd "
    "  (pax_id,rem_status,reg_no) "
    "VALUES "
    "  (:pax_id,:rem_status,:reg_no) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("reg_no",otInteger);
  for(vector<TCHKDItem>::const_iterator i=chkd.begin();i!=chkd.end();++i)
  {
    if (i->Empty() || string(i->rem_status).empty()) continue;
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("reg_no",(int)i->reg_no);
    Qry.Execute();
    result=true;
  };
  if (result)
  {
    Qry.Clear();
    Qry.SQLText="UPDATE crs_pax SET sync_chkd=1 WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id",otInteger,pax_id);
    Qry.Execute();
  };
  return result;
};

void SaveASVCRem(int pax_id, const vector<TASVCItem> &asvc, bool &sync_pax_asvc)
{
  sync_pax_asvc=false;
  if (asvc.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_asvc "
    "  (pax_id,rem_status,rfic,rfisc,service_quantity,ssr_code,service_name,emd_type,emd_no,emd_coupon) "
    "VALUES "
    "  (:pax_id,:rem_status,:rfic,:rfisc,:service_quantity,:ssr_code,:service_name,:emd_type,:emd_no,:emd_coupon) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("rfic",otString);
  Qry.DeclareVariable("rfisc",otString);
  Qry.DeclareVariable("service_quantity",otInteger);
  Qry.DeclareVariable("ssr_code",otString);
  Qry.DeclareVariable("service_name",otString);
  Qry.DeclareVariable("emd_type",otString);
  Qry.DeclareVariable("emd_no",otString);
  Qry.DeclareVariable("emd_coupon",otInteger);
  for(vector<TASVCItem>::const_iterator i=asvc.begin();i!=asvc.end();++i)
  {
    if (i->Empty() || string(i->rem_status).empty()) continue;
    Qry.SetVariable("rem_status",i->rem_status);
    Qry.SetVariable("rfic",i->RFIC);
    Qry.SetVariable("rfisc",i->RFISC);
    Qry.SetVariable("service_quantity",i->service_quantity);
    Qry.SetVariable("ssr_code",i->ssr_code);
    Qry.SetVariable("service_name",i->service_name);
    Qry.SetVariable("emd_type",i->emd_type);
    Qry.SetVariable("emd_no",i->emd_no);
    i->emd_coupon!=NoExists?Qry.SetVariable("emd_coupon",i->emd_coupon):
                            Qry.SetVariable("emd_coupon",FNull);
    Qry.Execute();
  };

  sync_pax_asvc=CheckIn::SyncPaxASVC(pax_id, false);
};

void SaveFQTRem(int pax_id, const vector<TFQTItem> &fqt, const set<TFQTExtraItem> &fqt_extra)
{
  if (fqt.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_fqt "
    "  (pax_id, rem_code, airline, no, extra, tier_level) "
    "VALUES "
    "  (:pax_id, :rem_code, :airline, :no, :extra, :tier_level) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("extra",otString);
  set<TFQTItem> unique_fqt;
  for(vector<TFQTItem>::const_iterator i=fqt.begin();i!=fqt.end();i++)
    unique_fqt.insert(*i);
  if (unique_fqt.size()==1 && fqt_extra.size()==1)
    Qry.CreateVariable("tier_level", otString, fqt_extra.begin()->tier_level);
  else
    Qry.CreateVariable("tier_level", otString, FNull);
  for(vector<TFQTItem>::const_iterator i=fqt.begin();i!=fqt.end();i++)
  {
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("airline",i->airline);
    Qry.SetVariable("no",i->no);
    if (i->extra.size()>250)
      Qry.SetVariable("extra", i->extra.substr(0,250));
    else
      Qry.SetVariable("extra", i->extra);
    Qry.Execute();
  };
};

void SaveDCSBaggage(int pax_id, const TNameElement &ne)
{
  if (ne.bag.Empty() && ne.tags.empty()) return;
  TQuery Qry(&OraSession);
  if (!ne.bag.Empty())
  {
    Qry.Clear();
    Qry.SQLText=
      "INSERT INTO dcs_bag(pax_id, bag_amount, bag_weight, rk_weight, weight_unit) "
      "VALUES(:pax_id, :bag_amount, :bag_weight, :rk_weight, :weight_unit)";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("bag_amount", otInteger, (int)ne.bag.bag_amount);
    Qry.CreateVariable("bag_weight", otInteger, (int)ne.bag.bag_weight);
    Qry.CreateVariable("rk_weight", otInteger, (int)ne.bag.rk_weight);
    Qry.CreateVariable("weight_unit", otString, ne.bag.weight_unit);
    Qry.Execute();
  };
  if (!ne.tags.empty())
  {
    Qry.Clear();
    Qry.SQLText=
      "INSERT INTO dcs_tags(pax_id, alpha_no, numeric_no, airp_arv_final) "
      "VALUES(:pax_id, :alpha_no, :numeric_no, :airp_arv_final)";
    Qry.CreateVariable("pax_id",otInteger,pax_id);
    Qry.DeclareVariable("alpha_no", otString);
    Qry.DeclareVariable("numeric_no", otFloat);
    Qry.DeclareVariable("airp_arv_final", otString);
    for(vector<TTagItem>::const_iterator iTag=ne.tags.begin(); iTag!=ne.tags.end(); ++iTag)
    {
      Qry.SetVariable("alpha_no",iTag->alpha_no);
      Qry.SetVariable("airp_arv_final",iTag->airp_arv_final);
      for(int j=0;j<iTag->num;j++)
      {
        Qry.SetVariable("numeric_no",iTag->numeric_no+j);
        Qry.Execute();
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
    check_layer_change(point_ids_spp);
    usePriorContext=true;
  };
};

void UpdateCrsDataStat(const TTlgElement elem,
                       const int point_id,
                       const string &system,
                       const TDCSHeadingInfo &info)
{
   string crs_data_set_null, crs_data_stat_field;
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
   };


   ostringstream sql;
   sql << "BEGIN ";

   if (elem!=ClassCodes)
     sql << "  UPDATE crs_data "
         << crs_data_set_null
         << "  WHERE point_id=:point_id AND system=:system AND sender=:sender; ";

   else
     sql << "  DELETE FROM crs_rbd WHERE point_id=:point_id AND system=:system AND sender=:sender; ";

   sql << "  IF :system='CRS' THEN "
          "    UPDATE crs_data_stat SET " << crs_data_stat_field << "=:time_create WHERE point_id=:point_id AND crs=:sender; "
          "    IF SQL%NOTFOUND THEN "
          "      INSERT INTO crs_data_stat(point_id,crs," << crs_data_stat_field << ") "
          "      VALUES(:point_id,:sender,:time_create); "
          "    END IF; "
          "  END IF; "
          "END;";

   TQuery Qry(&OraSession);
   Qry.Clear();
   Qry.SQLText=sql.str().c_str();
   Qry.CreateVariable("point_id",otInteger,point_id);
   Qry.CreateVariable("system",otString,system);
   Qry.CreateVariable("sender",otString,info.sender);
   Qry.CreateVariable("time_create",otDate,info.time_create);
   Qry.Execute();
};

bool SavePNLADLPRLContent(int tlg_id, TDCSHeadingInfo& info, TPNLADLPRLContent& con, bool forcibly)
{
  vector<TRouteItem>::iterator iRouteItem;
  vector<TSeatsItem>::iterator iSeatsItem;
  vector<TTotalsByDest>::iterator iTotals;
  vector<TPnrItem>::iterator iPnrItem;
  vector<TNameElement>::iterator iNameElement;
  vector<TPaxItem>::iterator iPaxItem;
  vector<TTransferItem>::iterator iTransfer;
  vector<TPnrAddrItem>::iterator iPnrAddr;
  vector<TInfItem>::iterator iInfItem;

  int tid;

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
  catch(EOracleError &E)
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
  catch(EOracleError &E)
  {
    if (E.Code!=1) throw;
  };

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  UPDATE typeb_data_stat SET last_data=GREATEST(last_data,:time_create) "
    "  WHERE point_id=:point_id AND system=:system AND sender=:sender; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO typeb_data_stat(point_id,system,sender,last_data) "
    "    VALUES(:point_id,:system,:sender,:time_create); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("system",otString,system);
  Qry.CreateVariable("sender",otString,info.sender);
  Qry.CreateVariable("time_create",otDate,info.time_create);
  Qry.Execute();

  bool pr_numeric_pnl=false;
  TDateTime last_resa=NoExists,
            last_tranzit=NoExists,
            last_avail=NoExists,
            last_cfg=NoExists,
            last_rbd=NoExists;
  int pr_pnl=0;

  if (!isPRL)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT pr_numeric_pnl FROM crs_set "
      "WHERE crs=:crs AND airline=:airline AND "
      "      (flt_no=:flt_no OR flt_no IS NULL) AND "
      "      (airp_dep=:airp_dep OR airp_dep IS NULL) "
      "ORDER BY flt_no,airp_dep";
    Qry.CreateVariable("crs",otString,info.sender);
    Qry.CreateVariable("airline",otString,con.flt.airline);
    Qry.CreateVariable("flt_no",otInteger,(int)con.flt.flt_no);
    Qry.CreateVariable("airp_dep",otString,con.flt.airp_dep);
    Qry.Execute();
    if (!Qry.Eof)
    {
      pr_numeric_pnl=Qry.FieldAsInteger("pr_numeric_pnl")!=0;
    }
    else
    {
      Qry.SQLText=
        "DECLARE "
        "  vid crs_set.id%TYPE; "
        "BEGIN "
        "  INSERT INTO crs_set(id,airline,flt_no,airp_dep,crs,priority,pr_numeric_pnl) "
        "  VALUES(id__seq.nextval,:airline,:flt_no,:airp_dep,:crs,0,0) RETURNING id INTO vid; "
        "  hist.synchronize_history('crs_set',vid,:SYS_user_descr,:SYS_desk_code); "
        "END;";
      Qry.SetVariable("flt_no",FNull);
      Qry.CreateVariable("SYS_user_descr", otString, TReqInfo::Instance()->user.descr);
      Qry.CreateVariable("SYS_desk_code", otString, TReqInfo::Instance()->desk.code);
      try
      {
        Qry.Execute();
      }
      catch(EOracleError &E)
      {
        if (E.Code!=1) throw;
      };
    };

    Qry.Clear();
    Qry.SQLText=
      "SELECT last_resa,last_tranzit,last_avail,last_cfg,last_rbd,pr_pnl FROM crs_data_stat "
      "WHERE point_id=:point_id AND crs=:crs FOR UPDATE";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("crs",otString,info.sender);
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
    UpdateCrsDataStat(TotalsByDestination, point_id, system, info);

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET resa=NVL(resa+:resa,:resa), pad=NVL(pad+:pad,:pad) "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender AND "
      "        airp_arv=:airp_arv AND class=:class; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO crs_data(point_id,system,sender,airp_arv,class,resa,pad) "
      "    VALUES(:point_id,:system,:sender,:airp_arv,:class,:resa,:pad); "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("system",otString,system);
    Qry.DeclareVariable("airp_arv",otString);
    Qry.DeclareVariable("class",otString);
    Qry.DeclareVariable("resa",otInteger);
    Qry.DeclareVariable("pad",otInteger);
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
    {
      Qry.SetVariable("airp_arv",iTotals->dest);
      Qry.SetVariable("class",EncodeClass(iTotals->cl));
      Qry.SetVariable("resa",(int)iTotals->seats);
      Qry.SetVariable("pad",(int)iTotals->pad);
      Qry.Execute();
    };
  };

  if (!con.transit.empty()&&(last_tranzit==NoExists||last_tranzit<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(TranzitElement, point_id, system, info);

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET tranzit=NVL(tranzit+:tranzit,:tranzit) "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender AND "
      "        airp_arv=:airp_arv AND class=:class; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO crs_data(point_id,system,sender,airp_arv,class,tranzit) "
      "    VALUES(:point_id,:system,:sender,:airp_arv,:class,:tranzit); "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("system",otString,system);
    Qry.DeclareVariable("airp_arv",otString);
    Qry.DeclareVariable("class",otString);
    Qry.DeclareVariable("tranzit",otInteger);
    for(iRouteItem=con.transit.begin();iRouteItem!=con.transit.end();iRouteItem++)
    {
      Qry.SetVariable("airp_arv",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("tranzit",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };
  };

  if (!con.avail.empty()&&(last_avail==NoExists||last_avail<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(SpaceAvailableElement, point_id, system, info);

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET avail=NVL(avail+:avail,:avail) "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender AND "
      "        airp_arv=:airp_arv AND class=:class; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO crs_data(point_id,system,sender,airp_arv,class,avail) "
      "    VALUES(:point_id,:system,:sender,:airp_arv,:class,:avail); "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("system",otString,system);
    Qry.DeclareVariable("airp_arv",otString);
    Qry.DeclareVariable("class",otString);
    Qry.DeclareVariable("avail",otInteger);
    for(iRouteItem=con.avail.begin();iRouteItem!=con.avail.end();iRouteItem++)
    {
      Qry.SetVariable("airp_arv",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("avail",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };
  };

  if (!con.cfg.empty()&&(last_cfg==NoExists||last_cfg<=info.time_create))
  {
    pr_recount=true;
    UpdateCrsDataStat(Configuration, point_id, system, info);

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET cfg=NVL(cfg+:cfg,:cfg) "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender AND "
      "        airp_arv=:airp_arv AND class=:class; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO crs_data(point_id,system,sender,airp_arv,class,cfg) "
      "    VALUES(:point_id,:system,:sender,:airp_arv,:class,:cfg); "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("system",otString,system);
    Qry.DeclareVariable("airp_arv",otString);
    Qry.DeclareVariable("class",otString);
    Qry.DeclareVariable("cfg",otInteger);
    for(iRouteItem=con.cfg.begin();iRouteItem!=con.cfg.end();iRouteItem++)
    {
      Qry.SetVariable("airp_arv",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("cfg",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };
  };

  if (!con.rbd.empty()&&(last_rbd==NoExists||last_rbd<=info.time_create))
  {
    UpdateCrsDataStat(ClassCodes, point_id, system, info);
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

  OraSession.Commit();

  try
  {
    if (pr_save_ne)
    {
      //��।����, ���� �� PNL ��� ��஢� (� ��⮬ ZZ)
      bool pr_ne=false;
      int seats=0;
      for(iTotals=con.resa.begin();iTotals!=con.resa.end()&&!pr_ne;iTotals++)
      {
        seats+=iTotals->seats+iTotals->pad;
        for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end()&&!pr_ne;iPnrItem++)
        {
          TPnrItem& pnr=*iPnrItem;
          for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end()&&!pr_ne;iNameElement++)
          {
            TNameElement& ne=*iNameElement;
            if (ne.surname!="ZZ") pr_ne=true;
          };
        };
      };
      if (pr_ne)
      {
        //����稬 �����䨪��� �࠭���樨
        Qry.Clear();
        Qry.SQLText="SELECT cycle_tid__seq.nextval AS tid FROM dual";
        Qry.Execute();
        tid=Qry.FieldAsInteger("tid");

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

        TQuery CrsPaxQry(&OraSession);
        CrsPaxQry.Clear();
        CrsPaxQry.SQLText=
          "SELECT pax_id,pr_del,last_op FROM crs_pax "
          "WHERE pnr_id= :pnr_id AND "
          "      surname= :surname AND (name= :name OR :name IS NULL AND name IS NULL) AND "
          "      DECODE(seats,0,0,1)=:seats AND tid<>:tid "
          "ORDER BY last_op DESC,pax_id DESC";
        CrsPaxQry.DeclareVariable("pnr_id",otInteger);
        CrsPaxQry.DeclareVariable("surname",otString);
        CrsPaxQry.DeclareVariable("name",otString);
        CrsPaxQry.DeclareVariable("seats",otInteger);
        CrsPaxQry.CreateVariable("tid",otInteger,tid);

        TQuery CrsPaxInsQry(&OraSession);
        CrsPaxInsQry.Clear();
        CrsPaxInsQry.SQLText=
          "BEGIN "
          "  IF :pax_id IS NULL THEN "
          "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
          "    INSERT INTO crs_pax(pax_id,pnr_id,surname,name,pers_type,seat_xname,seat_yname,seat_rem,seat_type,seats,bag_pool,sync_chkd,pr_del,last_op,tid,need_apps) "
          "    VALUES(:pax_id,:pnr_id,:surname,:name,:pers_type,:seat_xname,:seat_yname,:seat_rem,:seat_type,:seats,:bag_pool,0,:pr_del,:last_op,cycle_tid__seq.currval,:need_apps); "
          "  ELSE "
          "    UPDATE crs_pax "
          "    SET pers_type= :pers_type, seat_xname= :seat_xname, seat_yname= :seat_yname, seat_rem= :seat_rem, "
          "        seat_type= :seat_type, bag_pool= :bag_pool, pr_del= :pr_del, last_op= :last_op, tid=cycle_tid__seq.currval,need_apps=:need_apps "
          "    WHERE pax_id=:pax_id; "
          "  END IF; "
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
        CrsPaxInsQry.DeclareVariable("need_apps",otInteger);

        TQuery CrsInfInsQry(&OraSession);
        CrsInfInsQry.Clear();
        CrsInfInsQry.SQLText=
          "INSERT INTO crs_inf(inf_id,pax_id) VALUES(:inf_id,:pax_id)";
        CrsInfInsQry.DeclareVariable("pax_id",otInteger);
        CrsInfInsQry.DeclareVariable("inf_id",otInteger);

        TQuery CrsTransferQry(&OraSession);
        CrsTransferQry.Clear();
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

        int pnr_id,pax_id;
        bool pr_sync_pnr;
        bool UsePriorContext=false;
        TPointIdsForCheck point_ids_spp;
        set<int> et_display_pax_ids;
        bool chkd_exists=false;
        bool apps_pax_exists=false;
        int point_id_spp = ASTRA::NoExists;
        TAdvTripInfoList trips;
        if ( !isPRL ) {
          getTripsByPointIdTlg(point_id, trips);
          if (!trips.empty())
            point_id_spp = trips.front().point_id;
        }
        for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
        {
          CrsPnrQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrInsQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("class",EncodeClass(iTotals->cl));
          bool is_need_apps = false;
          if ( point_id_spp != ASTRA::NoExists )
            is_need_apps = checkAPPSSets(point_id_spp, iTotals->dest);
          for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
          {
            TPnrItem& pnr=*iPnrItem;
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
                        else throw 0;
                        break;
                      };
                    };
                    if (iPaxItem!=ne.pax.end()) break;
                  };
                };
                if (pr_chg_del&&pnr_id==0) throw 0;
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
            CrsPnrInsQry.SetVariable("grp_name",pnr.grp_name);
            CrsPnrInsQry.SetVariable("status",pnr.status);
            CrsPnrInsQry.SetVariable("priority",pnr.priority);
            if (pnr_id==0)
              CrsPnrInsQry.SetVariable("pnr_id",FNull);
            else
              CrsPnrInsQry.SetVariable("pnr_id",pnr_id);
            CrsPnrInsQry.Execute();
            pnr_id=CrsPnrInsQry.GetVariableAsInteger("pnr_id");
            CrsPaxQry.SetVariable("pnr_id",pnr_id);
            CrsPaxInsQry.SetVariable("pnr_id",pnr_id);
            PnrAddrsQry.SetVariable("pnr_id",pnr_id);


            for(iPnrAddr=pnr.addrs.begin();iPnrAddr!=pnr.addrs.end();iPnrAddr++)
            {
              PnrAddrsQry.SetVariable("airline",iPnrAddr->airline);
              PnrAddrsQry.SetVariable("addr",iPnrAddr->addr);
              PnrAddrsQry.Execute();
            };

            bool onlyDEL=true;
            for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
            {
              TNameElement& ne=*iNameElement;
              if (ne.indicator!=DEL) onlyDEL=false;
              CrsPaxQry.SetVariable("surname",ne.surname);
              CrsPaxQry.SetVariable("seats",1);
              for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
              {
                CrsPaxInsQry.SetVariable("pax_id",FNull);
                CrsPaxInsQry.SetVariable("surname",ne.surname);
                if (ne.indicator==ADD||ne.indicator==CHG||ne.indicator==DEL)
                {
                  CrsPaxQry.SetVariable("name",iPaxItem->name);
                  CrsPaxQry.Execute();
                  if (CrsPaxQry.RowCount()>0)
                  {
                    if (info.time_create<CrsPaxQry.FieldAsDateTime("last_op")) continue;
                    if (ne.indicator==CHG||ne.indicator==DEL)
                    {
                      pax_id=CrsPaxQry.FieldAsInteger("pax_id");
                      CrsPaxInsQry.SetVariable("pax_id",pax_id);
                    };
                  }
                  else
                  {
                    if (ne.indicator==CHG||ne.indicator==DEL)
                    {
                      if (!forcibly) throw 0;
                    };
                  };
                };

                CrsPaxInsQry.SetVariable("name",iPaxItem->name);
                CrsPaxInsQry.SetVariable("pers_type",EncodePerson(iPaxItem->pers_type));
                if (ne.indicator!=DEL && !iPaxItem->seat.Empty())
                {
                  CrsPaxInsQry.SetVariable("seat_xname",iPaxItem->seat.line);
                  CrsPaxInsQry.SetVariable("seat_yname",iPaxItem->seat.row);
                  CrsPaxInsQry.SetVariable("seat_rem",iPaxItem->seat_rem);
                  if (strcmp(iPaxItem->seat_rem,"NSST")==0||
                      strcmp(iPaxItem->seat_rem,"NSSA")==0||
                      strcmp(iPaxItem->seat_rem,"NSSW")==0||
                      strcmp(iPaxItem->seat_rem,"SMST")==0||
                      strcmp(iPaxItem->seat_rem,"SMSA")==0||
                      strcmp(iPaxItem->seat_rem,"SMSW")==0)
                    CrsPaxInsQry.SetVariable("seat_type",iPaxItem->seat_rem);
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

                CrsPaxInsQry.SetVariable("seats",(int)iPaxItem->seats);
                if (isPRL && ne.bag_pool!=NoExists)
                  CrsPaxInsQry.SetVariable("bag_pool", ne.bag_pool);
                else
                  CrsPaxInsQry.SetVariable("bag_pool", FNull);
                if (ne.indicator==DEL)
                  CrsPaxInsQry.SetVariable("pr_del",1);
                else
                  CrsPaxInsQry.SetVariable("pr_del",0);
                CrsPaxInsQry.SetVariable("last_op",info.time_create);
                if(is_need_apps) {
                  CrsPaxInsQry.SetVariable("need_apps",1);
                  apps_pax_exists=true;
                }
                else
                  CrsPaxInsQry.SetVariable("need_apps",0);
                CrsPaxInsQry.Execute();
                pax_id=CrsPaxInsQry.GetVariableAsInteger("pax_id");
                if (ne.indicator==CHG||ne.indicator==DEL)
                {
                  //㤠�塞 �� �� ���ᠦ���
                  Qry.Clear();
                  Qry.SQLText=
                    "DECLARE "
                    "  CURSOR cur IS "
                    "    SELECT inf_id FROM crs_inf WHERE pax_id=:pax_id FOR UPDATE; "
                    "BEGIN "
                    "  FOR curRow IN cur LOOP "
                    "    DELETE FROM crs_inf WHERE inf_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_rem WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_doc WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_doco WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_doca WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_tkn WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_fqt WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_chkd WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_asvc WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM pax_asvc WHERE pax_id=curRow.inf_id; "
                    "    :sync_pax_asvc_rows:=:sync_pax_asvc_rows+SQL%ROWCOUNT; "
                    "    DELETE FROM crs_pax_refuse WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_alarms WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax WHERE pax_id=curRow.inf_id; "
                    "  END LOOP; "
                    "  DELETE FROM crs_pax_rem WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doc WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doco WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doca WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_tkn WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_fqt WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_chkd WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_asvc WHERE pax_id=:pax_id; "
                    "  DELETE FROM pax_asvc WHERE pax_id=:pax_id; "
                    "  :sync_pax_asvc_rows:=:sync_pax_asvc_rows+SQL%ROWCOUNT; "
                    "END;";
                  Qry.CreateVariable("pax_id", otInteger, pax_id);
                  Qry.CreateVariable("sync_pax_asvc_rows", otInteger, 0);
                  Qry.Execute();
                  if (Qry.GetVariableAsInteger("sync_pax_asvc_rows")>0)
                    TPaxAlarmHook::set(Alarm::UnboundEMD, pax_id);

                  DeleteTlgSeatRanges(cltPNLCkin, pax_id, tid, point_ids_spp);
                  DeleteTlgSeatRanges(cltPNLBeforePay, pax_id, tid, point_ids_spp);
                  DeleteTlgSeatRanges(cltPNLAfterPay, pax_id, tid, point_ids_spp);
                  if (ne.indicator==DEL)
                  {
                    DeleteTlgSeatRanges(cltProtCkin, pax_id, tid, point_ids_spp);
                    DeleteTlgSeatRanges(cltProtBeforePay, pax_id, tid, point_ids_spp);
                    DeleteTlgSeatRanges(cltProtAfterPay, pax_id, tid, point_ids_spp);
                    if(is_need_apps) {
                      deleteAPPSAlarms(pax_id);
                      deleteAPPSData(pax_id);
                    }
                  };
                };
                if (ne.indicator!=DEL)
                {
                  //��ࠡ�⪠ ������楢
                  int inf_id;

                  CrsPaxInsQry.SetVariable("pers_type",EncodePerson(baby));
                  CrsPaxInsQry.SetVariable("seat_xname",FNull);
                  CrsPaxInsQry.SetVariable("seat_yname",FNull);
                  CrsPaxInsQry.SetVariable("seats",0);
                  CrsPaxInsQry.SetVariable("pr_del",0);
                  CrsPaxInsQry.SetVariable("last_op",info.time_create);
                  //�������� ���ᠦ��
                  CrsInfInsQry.SetVariable("pax_id",pax_id);
                  for(iInfItem=iPaxItem->inf.begin();iInfItem!=iPaxItem->inf.end();iInfItem++)
                  {
                    CrsPaxInsQry.SetVariable("pax_id",FNull);
                    CrsPaxInsQry.SetVariable("surname",iInfItem->surname);
                    CrsPaxInsQry.SetVariable("name",iInfItem->name);
                    CrsPaxInsQry.Execute();
                    inf_id=CrsPaxInsQry.GetVariableAsInteger("pax_id");
                    CrsInfInsQry.SetVariable("inf_id",inf_id);
                    CrsInfInsQry.Execute();
                    SavePNLADLRemarks(inf_id,iInfItem->rem);
                    SaveDOCSRem(inf_id,iInfItem->doc,iPaxItem->doc_extra);
                    SaveDOCORem(inf_id,iInfItem->doco);
                    SaveDOCARem(inf_id,iInfItem->doca);
                    SaveTKNRem(inf_id,iInfItem->tkn);
                    if (SaveCHKDRem(inf_id,iInfItem->chkd)) chkd_exists=true;
                    et_display_pax_ids.insert(inf_id);
                  };

                  //६�ન ���ᠦ��
                  SavePNLADLRemarks(pax_id,iPaxItem->rem);
                  SaveDOCSRem(pax_id,iPaxItem->doc,iPaxItem->doc_extra);
                  SaveDOCORem(pax_id,iPaxItem->doco);
                  SaveDOCARem(pax_id,iPaxItem->doca);
                  SaveTKNRem(pax_id,iPaxItem->tkn);
                  SaveFQTRem(pax_id,iPaxItem->fqt,iPaxItem->fqt_extra);
                  if (SaveCHKDRem(pax_id,iPaxItem->chkd)) chkd_exists=true;
                  et_display_pax_ids.insert(pax_id);

                  bool sync_pax_asvc;
                  SaveASVCRem(pax_id,iPaxItem->asvc,sync_pax_asvc);
                  if (sync_pax_asvc) TPaxAlarmHook::set(Alarm::UnboundEMD, pax_id);
                  //ࠧ��⪠ ᫮��
                  InsertTlgSeatRanges(point_id,iTotals->dest,isPRL?cltPRLTrzt:cltPNLCkin,iPaxItem->seatRanges,
                                      pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  if (!isPRL)
                  {
                    TCompLayerType rem_layer=GetSeatRemLayer(pnr.market_flt.Empty()?con.flt.airline:
                                                                                    pnr.market_flt.airline,
                                                             iPaxItem->seat_rem);
                    if (rem_layer!=cltPNLCkin && rem_layer!=cltUnknown)
                      InsertTlgSeatRanges(point_id,iTotals->dest,rem_layer,
                                          TSeatRanges(iPaxItem->seat),
                                          pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  };
                  UsePriorContext=true;
                  //६�ન, �� �ਢ易��� � ���ᠦ���
                  SavePNLADLRemarks(pax_id,ne.rem);
                  InsertTlgSeatRanges(point_id,iTotals->dest,isPRL?cltPRLTrzt:cltPNLCkin,ne.seatRanges,
                                      pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  //����⠭���� ����� ���� �।���⥫쭮� ��ᠤ��
                };

                if (!isPRL && !pr_sync_pnr)
                {
                  //������ ᨭ�஭����� ���ᠦ�� � ஧�᪮�
                  rozysk::sync_crs_pax(pax_id, "", "");
                };

                if (isPRL && iPaxItem==ne.pax.begin())
                {
                  //����襬 �����
                  if (!ne.bag.Empty() || !ne.tags.empty()) SaveDCSBaggage(pax_id, ne);
                };
              };
            }; //for(iNameElement=pnr.ne.begin()

            //����襬 ��몮���
            if (!onlyDEL)
            {
              //㤠�塞 ��䨣 �� ��몮���
              Qry.Clear();
              Qry.SQLText=
                "BEGIN "
                "  DELETE FROM crs_transfer WHERE pnr_id= :pnr_id; "
                "  UPDATE crs_pnr SET tid=cycle_tid__seq.currval WHERE pnr_id= :pnr_id; "
                "END;";
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
              Qry.Clear();
              Qry.SQLText=
                "UPDATE crs_pnr SET airp_arv_final= "
                "  (SELECT airp_arv FROM crs_transfer WHERE pnr_id=:pnr_id AND transfer_num= "
                "    (SELECT MAX(transfer_num) FROM crs_transfer WHERE pnr_id=:pnr_id AND transfer_num>0)) "
                "WHERE pnr_id=:pnr_id";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
            };
            //����襬 �������᪨� ३�
            if (!onlyDEL)
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
        check_layer_change(point_ids_spp);
        TlgETDisplay(point_id, et_display_pax_ids, true);
        if (!isPRL && chkd_exists)
        {
          for(TAdvTripInfoList::const_iterator it = trips.begin(); it != trips.end(); it++)
            add_trip_task((*it).point_id, SYNC_NEW_CHKD, "");
        };
        if(!isPRL && apps_pax_exists) {
          TDateTime start_time;
          bool result = checkTime( point_id_spp, start_time );
          if ( result || start_time != ASTRA::NoExists )
            add_trip_task( point_id_spp, SEND_NEW_APPS_INFO, "", start_time );
        }
      };

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
          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  UPDATE crs_data_stat SET pr_pnl=:pr_pnl WHERE point_id=:point_id AND crs=:sender; "
            "  IF SQL%NOTFOUND THEN "
            "    INSERT INTO crs_data_stat(point_id,crs,pr_pnl) "
            "    VALUES(:point_id,:sender,:pr_pnl); "
            "  END IF; "
            "END;";
          Qry.CreateVariable("point_id",otInteger,point_id);
          Qry.CreateVariable("sender",otString,info.sender);
          Qry.CreateVariable("pr_pnl",otInteger,pr_pnl_new);
          Qry.Execute();
        };
      };
    }
    else
    {
      if (strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2) return false;//throw 0; //�⫮��� ࠧ���
    };
    return true;
  }
  catch (int)
  {
    OraSession.Rollback();  //�⫮��� ࠧ���
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

}

