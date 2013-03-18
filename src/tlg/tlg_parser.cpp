#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "tlg_parser.h"
#include "ssm_parser.h"
#include "astra_consts.h"
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
#include "tlg_binding.h"
#include "rozysk.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

namespace TypeB
{

const TMonthCode Months[12] =
    {{"ЯНВ","JAN"},
     {"ФЕВ","FEB"},
     {"МАР","MAR"},
     {"АПР","APR"},
     {"МАЙ","MAY"},
     {"ИЮН","JUN"},
     {"ИЮЛ","JUL"},
     {"АВГ","AUG"},
     {"СЕН","SEP"},
     {"ОКТ","OCT"},
     {"НОЯ","NOV"},
     {"ДЕК","DEC"}};

const char* TTlgElementS[] =
              {//общие
               "Address",
               "CommunicationsReference",
               "MessageIdentifier",
               "FlightElement",
               //PNL и ADL
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
               //общие
               "EndOfMessage"};


char lexh[MAX_LEXEME_SIZE+1];

void ParseNameElement(char* p, vector<string> &names, TElemPresence num_presence);
void ParsePaxLevelElement(TTlgParser &tlg, TFltInfo& flt, TPnrItem &pnr, bool &pr_prev_rem, bool &pr_bag_info);
void BindRemarks(TTlgParser &tlg, TNameElement &ne);
void ParseRemarks(const vector< pair<string,int> > &seat_rem_priority,
                  TTlgParser &tlg, const TDCSHeadingInfo &info, TPnrItem &pnr, TNameElement &ne);
bool ParseCHDRem(TTlgParser &tlg,string &rem_text,vector<TChdItem> &chd);
bool ParseINFRem(TTlgParser &tlg,string &rem_text,vector<TInfItem> &inf);
bool ParseSEATRem(TTlgParser &tlg,string &rem_text,vector<TSeatRange> &seats);
bool ParseTKNRem(TTlgParser &tlg,string &rem_text,TTKNItem &tkn);
bool ParseFQTRem(TTlgParser &tlg,string &rem_text,TFQTItem &fqt);

char* TTlgParser::NextLine(char* p)
{
  if (p==NULL) return NULL;
  if ((p=strchr(p,'\n'))==NULL) return NULL;
  else return p+1;
};

char* TTlgParser::GetLexeme(char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(unsigned char*)(p+len)>' ';len++);
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

char* TTlgParser::GetSlashedLexeme(char* p)
{
  int len;
  char *res;
  if (p==NULL) return NULL;
  //удалим все предшествующие пробелы
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n'&&*(p+len)!='/';len++);
  //запомним позицию слэша
  res=p+len;
  if (*res=='/') res++;
  else
    if (len==0) res=NULL;
  //удалим все последующие пробелы
  for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  len++;
  //заполним tlg.lex
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  return res;
};

char* TTlgParser::GetWord(char* p)
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

char* TTlgParser::GetNameElement(char* p, bool trimRight) //до первой точки на строке
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++)
    if (*(unsigned char*)(p+len)<=' '&&*(p+len+1)=='.') break;
  if (trimRight)
    //удаляем пустые символы с конца лексемы
    for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  else
    //удаляем символы с конца лексемы кроме пробелов
    for(len--;len>=0&&*(unsigned char*)(p+len)<' ';len--);

  len++;
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

char* TTlgParser::GetToEOLLexeme(char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++);
  for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  len++;
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

char* GetTlgElementName(TTlgElement e)
{
  return (char*)TTlgElementS[e];
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
  return TlgElemToElemId(etAirline,airline,airline,with_icao);
};

char* GetAirp(char* airp, bool with_icao=false)
{
  return TlgElemToElemId(etAirp,airp,airp,with_icao);
};

TClass GetClass(char* subcl)
{
  char subclh[2];
  TlgElemToElemId(etSubcls,subcl,subclh);
  return DecodeClass(getBaseTable(etSubcls).get_row("code",subclh).AsString("cl").c_str());
};

char* GetSubcl(char* subcl)
{
  return TlgElemToElemId(etSubcls,subcl,subcl);
};

char GetSuffix(char &suffix)
{
  if (suffix!=0)
  {
    char suffixh[2];
    suffixh[0]=suffix;
    suffixh[1]=0;
    TlgElemToElemId(etSuffix,suffixh,suffixh);
    suffix=suffixh[0];
  };
  return suffix;
};

void GetPaxDocCountry(const char* elem, char* id)
{
  try
  {
    TlgElemToElemId(etPaxDocCountry,elem,id);
  }
  catch (EBaseTableError)
  {
    char country[3];
    TlgElemToElemId(etCountry,elem,country);
    //ищем в pax_doc_countries
    strcpy(id,getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code").c_str());
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
      strcmp(tlg_type,"LDM")==0) cat=tcAHM;
  if (strcmp(tlg_type,"SSM")==0) cat=tcSSM;
  if (strcmp(tlg_type,"ASM")==0) cat=tcASM;
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

char ParseBSMElement(char *p, TTlgParser &tlg, TBSMInfo* &data, TMemoryManager &mem)
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
        //в tlg.lex первая лекскма
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
            res=sscanf(tlg.lex,"%1[A-Z0-9]%1[LTXR]%3[A-ZА-ЯЁ]%c",info.ver_no,info.src_indicator,info.airp,&c);
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
          {
            data = new TBSMFltInfo;
            mem.create(data, STDLOG);
            TBSMFltInfo& flt = *(TBSMFltInfo*)data;

            if (strlen(tlg.lex)<3||strlen(tlg.lex)>8) throw ETlgError("Wrong flight number");
            flt.suffix[0]=0;
            flt.suffix[1]=0;
            c=0;
            if (IsDigit(tlg.lex[2]))
              res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ0-9]%5lu%c%c",
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
            res=sscanf(tlg.lex,"%2lu%3[A-ZА-ЯЁ]%c",&day,month,&c);
            if (c!=0||res!=2||day<=0) throw ETlgError("Wrong flight date");

            TDateTime today=NowLocal();
            int year,mon,currday;
            DecodeDate(today,year,mon,currday);
            try
            {
              for(mon=1;mon<=12;mon++)
                if (strcmp(month,Months[mon-1].lat)==0||
                    strcmp(month,Months[mon-1].rus)==0) break;
              EncodeDate(year,mon,day,flt.scd);
              if ((int)today>(int)IncMonth(flt.scd,6))  //пол-года
                EncodeDate(year+1,mon,day,flt.scd);
            }
            catch(EConvertError)
            {
              throw ETlgError("Can't convert flight date");
            };

            if ((p=tlg.GetSlashedLexeme(p))==NULL) throw ETlgError("Airport code not found");
            c=0;
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",flt.airp,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong airport code");
            GetAirp(flt.airp);

            if ((p=tlg.GetSlashedLexeme(p))!=NULL)
            {
              c=0;
              res=sscanf(tlg.lex,"%1[A-ZА-ЯЁ]%c",flt.subcl,&c);
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
        default: data=NULL;
      };
      return (*id);
    }
    catch (ETlgError E)
    {
      switch (*id)
      {
        case 'V': throw ETlgError("Version and supplementary data: %s",E.what());
        case 'I': throw ETlgError("Inbound flight information: %s",E.what());
        case 'F': throw ETlgError("Outbound flight information: %s",E.what());
        case 'N': throw ETlgError("Baggage tag details: %s",E.what());
        case 'W': throw ETlgError("Pieces and weight data: %s",E.what());
        case 'P': throw ETlgError("Passenger name: %s",E.what());
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

TTlgPartInfo ParseBSMHeading(TTlgPartInfo heading, TBSMHeadingInfo &info, TMemoryManager &mem)
{
  int line;
  char c,*line_p;
  TTlgParser tlg;
  TTlgPartInfo next;

  try
  {
    line_p=heading.p;
    line=heading.line-1;
    do
    {
      line++;
      if (tlg.GetLexeme(line_p)==NULL) continue;

      TBSMInfo *data=NULL;
      c=ParseBSMElement(line_p,tlg,data,mem);
      try
      {
        if (c!='V') throw ("Version and supplementary data not found");
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
      next.p=tlg.NextLine(line_p);
      next.line=line+1;
      return next;
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(ETlgError E)
  {
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  next.p=line_p;
  next.line=line;
  return next;
};

void ParseBTMContent(TTlgPartInfo body, TBSMHeadingInfo& info, TBtmContent& con, TMemoryManager &mem)
{
  vector<TBtmTransferInfo>::iterator iIn;
  vector<TBtmOutFltInfo>::iterator iOut;
  vector<TBtmGrpItem>::iterator iGrp;
  vector<TBSMTagItem>::iterator iTag;
  vector<string>::iterator i;

  con.Clear();

  int line;
  char e,prior,*line_p;
  TTlgParser tlg;

  TBSMInfo *data=NULL;

  line_p=body.p;
  line=body.line-1;
  try
  {
    char airp[4]; //аэропорт в котором происходит трансфер
    strcpy(airp,info.airp);
    GetAirp(airp);
    prior='V';

    do
    {
      line++;
      if (tlg.GetLexeme(line_p)==NULL) continue;

      e=ParseBSMElement(line_p,tlg,data,mem);
      if (data==NULL) continue;
      try
      {
        switch (e)
        {
          case 'V': throw ETlgError("Wrong element order");
          case 'I':
            {
              if (strchr("VP",prior)==NULL) throw ETlgError("Wrong element order");
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
              if (strchr("IP",prior)==NULL) throw ETlgError("Wrong element order");
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
              if (strchr("FNP",prior)==NULL) throw ETlgError("Wrong element order");
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
              if (strchr("N",prior)==NULL) throw ETlgError("Wrong element order");
              TBSMBag &bag = *(dynamic_cast<TBSMBag*>(data));
              strcpy(iGrp->weight_unit,bag.weight_unit);
              iGrp->bag_amount=bag.bag_amount;
              iGrp->bag_weight=bag.bag_weight;
              iGrp->rk_weight=bag.rk_weight;
              break;
            }
          case 'P':
            {
              if (strchr("NWP",prior)==NULL) throw ETlgError("Wrong element order");
              TBSMPax &BSMPax = *(dynamic_cast<TBSMPax*>(data));
              TBtmPaxItem pax;
              pax.surname=BSMPax.surname;
              pax.name=BSMPax.name;
              iGrp->pax.push_back(pax);
              break;
            }
          default: ;
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
  }
  catch (ETlgError E)
  {
    if (tlg.GetToEOLLexeme(line_p)!=NULL)
      throw ETlgError("%s\n>>>>>LINE %d: %s",E.what(),line,tlg.lex);
    else
      throw ETlgError("%s\n>>>>>LINE %d",E.what(),line);
  };
  return;
};

TTlgPartInfo ParseDCSHeading(TTlgPartInfo heading, TDCSHeadingInfo &info)
{
  int line,res;
  char c,*p,*line_p;
  TTlgParser tlg;
  TTlgElement e;
  TTlgPartInfo next;

  try
  {
    line_p=heading.p;
    line=heading.line-1;
    e=FlightElement;
    do
    {
      line++;
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case FlightElement:
          {
            //message flight
            long int day;
            char month[4],flt[9];
            c=0;
            res=sscanf(tlg.lex,"%8[A-ZА-ЯЁ0-9]/%2lu%3[A-ZА-ЯЁ]%c",flt,&day,month,&c);
            if (c!=0||res!=3||day<=0) throw ETlgError("Wrong flight/local date");
            if (strlen(flt)<3) throw ETlgError("Wrong flight");
            info.flt.suffix[0]=0;
            info.flt.suffix[1]=0;
            c=0;
            if (IsDigit(flt[2]))
              res=sscanf(flt,"%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            else
              res=sscanf(flt,"%3[A-ZА-ЯЁ0-9]%5lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            if (c!=0||res<2||info.flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !IsUpperLetter(info.flt.suffix[0])) throw ETlgError("Wrong flight");
            GetSuffix(info.flt.suffix[0]);

            TDateTime today=NowLocal();
            int year,mon,currday;
            DecodeDate(today,year,mon,currday);
            try
            {
              for(mon=1;mon<=12;mon++)
                if (strcmp(month,Months[mon-1].lat)==0||
                    strcmp(month,Months[mon-1].rus)==0) break;
              EncodeDate(year,mon,day,info.flt.scd);
              if ((int)today>(int)IncMonth(info.flt.scd,6))  //пол-года
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
                res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",info.flt.airp_dep,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong boarding point");
              }
              else throw ETlgError("Wrong boarding point");
            };
            if (strcmp(info.tlg_type,"PTM")==0)
            {
              if ((p=tlg.GetLexeme(p))!=NULL)
              {
                c=0;
                res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%c",info.flt.airp_dep,info.flt.airp_arv,&c);
                if (c!=0||res!=2) throw ETlgError("Wrong boarding/transfer point");
              }
              else throw ETlgError("Wrong bording/transfer point");
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
              c=0;
              res=sscanf(tlg.lex,"ANA/%lu%c",&info.association_number,&c);
              if (c!=0||res!=1||
                  info.association_number<100||info.association_number>999999)
                throw ETlgError("Wrong association number");
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              next.p=tlg.NextLine(line_p);
              next.line=line+1;
            }
            else
            {
              next.p=line_p;
              next.line=line;
            };
            return next;
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(ETlgError E)
  {
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  next.p=line_p;
  next.line=line;
  return next;
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

  int line,res;
  char c,*line_p;
  TTlgParser tlg;
  TTlgElement e;

  try
  {
    line_p=body.p;
    line=body.line-1;
    e=FlightElement;
    do
    {
      line++;
      tlg.GetLexeme(line_p);
      switch (e)
      {
        case FlightElement:
          {
            //message flight
            long int day=0;
            char trip[9];
            res=sscanf(tlg.lex,"%11[A-ZА-ЯЁ0-9/].%s",lexh,tlg.lex);
            if (res!=2) throw ETlgError("Wrong flight identifier");
            c=0;
            res=sscanf(lexh,"%8[A-ZА-ЯЁ0-9]%c",trip,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(lexh,"%8[A-ZА-ЯЁ0-9]/%2lu%c",trip,&day,&c);
              if (c!=0||res!=2||day<=0) throw ETlgError("Wrong flight/UTC date");
            };
            if (strlen(trip)<3) throw ETlgError("Wrong flight");
            flt.suffix[0]=0;
            flt.suffix[1]=0;
            c=0;
            if (IsDigit(trip[2]))
              res=sscanf(trip,"%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(trip,"%3[A-ZА-ЯЁ0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong flight");
            GetAirline(flt.airline,true);
            GetSuffix(flt.suffix[0]);
            //переведем day в TDateTime
            int year,mon,currday;
            DecodeDate(NowUTC()+1,year,mon,currday); //м.б. разность системных времен у формирователя и приемщика, поэтому +1!
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
              EncodeDate(year,mon,day,flt.scd);
              flt.pr_utc=true;
            }
            catch(EConvertError)
            {
              throw ETlgError("Can't convert UTC date");
            };
            if (strcmp(info.tlg_type,"MVT")==0)
            {
              c=0;
              res=sscanf(tlg.lex,"%[^.].%3[A-ZА-ЯЁ]%c",lexh,flt.airp_dep,&c);
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
            res=sscanf(tlg.lex,"-%3[A-ZА-ЯЁ]%c",flt.airp_arv,&c);
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
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

//возвращает TTlgPartInfo следующей части (body)
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo* &info, TMemoryManager &mem)
{
  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  TTlgElement e;
  TTlgPartInfo next;

  if (info!=NULL)
  {
    mem.destroy(info, STDLOG);
    delete info;
    info = NULL;
  };
  THeadingInfo infoh;
  try
  {
    line_p=heading.p;
    line=heading.line-1;
    e=CommunicationsReference;
    do
    {
      line++;
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case CommunicationsReference:
          c=0;
          res=sscanf(tlg.lex,".%7[A-ZА-ЯЁ0-9]%c",infoh.sender,&c);
          if (c!=0||res!=1||strlen(infoh.sender)!=7) throw ETlgError("Wrong address");
          if ((ph=tlg.GetLexeme(p))!=NULL)
          {
            c=0;
            res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%c%s",infoh.double_signature,&c,tlg.lex);
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
                  //попробуем разобрать date/time group
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
                  DecodeDate(NowUTC()+1,year,mon,currday); //м.б. разность системных времен у формирователя и приемщика, поэтому +1!
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
            heading.p=line_p;
            heading.line=line;
          }
          else
          {
            heading.p=tlg.NextLine(line_p);
            heading.line=line+1;
          };

          infoh.tlg_cat=GetTlgCategory(infoh.tlg_type);

          switch (infoh.tlg_cat)
          {
            case tcDCS:
              info = new TDCSHeadingInfo(infoh);
              mem.create(info, STDLOG);
              next=ParseDCSHeading(heading,*(TDCSHeadingInfo*)info);
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
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  }
  catch(...)
  {
    mem.destroy(info, STDLOG);
    if (info!=NULL) delete info;
    info=NULL;
    throw;
  };
  next.p=line_p;
  next.line=line;
  return next;
};


void ParseDCSEnding(TTlgPartInfo ending, THeadingInfo &headingInfo, TEndingInfo &info)
{
  int line,res;
  char c,*p;
  TTlgParser tlg;
  char endtlg[7];

  try
  {
    line=ending.line;
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
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

void ParseAHMEnding(TTlgPartInfo ending, TEndingInfo& info)
{
  int line,res;
  char c,*p;
  TTlgParser tlg;

  try
  {
    line=ending.line;
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
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

//важно чтобы заполнялись изначально
//info.tlg_type=HeadingInfo.tlg_type и info.part_no=HeadingInfo.part_no
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

void ParseSOMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TSOMContent& con)
{
  con.Clear();

  int line,res;
  char c,*p,*line_p;
  TTlgParser tlg;
  TTlgElement e;

  line_p=body.p;
  line=body.line-1;
  e=FlightElement;
  try
  {
    strcpy(con.flt.airline,info.flt.airline);
    GetAirline(con.flt.airline);
    con.flt.flt_no=info.flt.flt_no;
    strcpy(con.flt.suffix,info.flt.suffix);
    con.flt.scd=info.flt.scd;
    con.flt.pr_utc=info.flt.pr_utc;
    strcpy(con.flt.airp_dep,info.flt.airp_dep);
    GetAirp(con.flt.airp_dep);

    vector<TSeatsByDest>::iterator iSeats;
    char cat[5];
    *cat=0;
    bool usePriorContext=false;
    vector<TSeatRange> ranges;

    e=SeatingCategories;
    do
    {
      line++;
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
            //разбор
            p=strchr(lexh,'.');
            if (p==NULL) throw ETlgError("Wrong seats by destination");
            //делим lexh на две строки
            *p=0;
            c=0;
            res=sscanf(lexh,"-%3[A-ZА-ЯЁ]%c",iSeats->airp_arv,&c);
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
              for(vector<TSeatRange>::iterator i=ranges.begin();i!=ranges.end();i++)
              {
                strcpy(i->rem,cat);
              };
              iSeats->ranges.insert(iSeats->ranges.end(),ranges.begin(),ranges.end());
              usePriorContext=true;
            }
            catch(EConvertError)
            {
              //диапазон не разобрался
              if (lexh[0]=='-')
                throw ETlgError("Wrong seats element %s",tlg.lex);
              else
                break;
            };
          };
          if (lexh[0]!='-' && p!=NULL)
          {
            //это может быть только категория
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
    if (tlg.GetToEOLLexeme(line_p)!=NULL)
      throw ETlgError("%s: %s\n>>>>>LINE %d: %s",GetTlgElementName(e),E.what(),line,tlg.lex);
    else
      throw ETlgError("%s: %s\n>>>>>LINE %d",GetTlgElementName(e),E.what(),line);


  };
  return;
};

void ParsePTMContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPtmContent& con)
{
  vector<TPtmOutFltInfo>::iterator iOut;

  con.Clear();

  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  TTlgElement e;

  line_p=body.p;
  line=body.line-1;
  e=FlightElement;
  try
  {
    strcpy(con.InFlt.airline,info.flt.airline);
    GetAirline(con.InFlt.airline);
    con.InFlt.flt_no=info.flt.flt_no;
    strcpy(con.InFlt.suffix,info.flt.suffix);
    con.InFlt.scd=info.flt.scd;
    con.InFlt.pr_utc=info.flt.pr_utc;
    strcpy(con.InFlt.airp_dep,info.flt.airp_dep);
    GetAirp(con.InFlt.airp_dep);
    strcpy(con.InFlt.airp_arv,info.flt.airp_arv);
    GetAirp(con.InFlt.airp_arv);

    e=TransferPassengerData;
    do
    {
      line++;
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case TransferPassengerData:
          {
            if (strcmp(tlg.lex,"NIL")==0)
            {
              if (tlg.GetLexeme(p)!=NULL) throw ETlgError("Unknown lexeme");
              break;
            };
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
              res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong connecting flight");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong connecting flight");
            GetAirline(flt.airline);
            GetSuffix(flt.suffix[0]);
            if ((ph=tlg.GetSlashedLexeme(ph))!=NULL)
            {
              //либо дата, либо признак курящего
              long int day=0;
              c=0;
              res=sscanf(tlg.lex,"%2lu%c",&day,&c);
              if (c!=0||res!=1)
              {
                //это не дата
                flt.scd=info.flt.scd;
              }
              else
              {
                //это дата
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
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",flt.airp_arv,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong destination of the connecting flight");
            GetAirp(flt.airp_arv);
            strcpy(flt.airp_dep,con.InFlt.airp_arv);

            if ((p=tlg.GetLexeme(p))==NULL)
              throw ETlgError("Number of seats not found");
            flt.subcl[0]=0;
            c=0;
            res=sscanf(tlg.lex,"%3lu%1[A-ZА-ЯЁ]%c",&data.seats,flt.subcl,&c);
            if (c==0&&res==1&&flt.subcl[0]==0)
            {
              //разберем подкласс в следующей лексеме
              if ((p=tlg.GetLexeme(p))==NULL)
                throw ETlgError("Class of service not found");
              c=0;
              res=sscanf(tlg.lex,"%1[A-ZА-ЯЁ]%c",flt.subcl,&c);
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

            if (tlg.GetToEOLLexeme(p)==NULL) //считаем до конца строки
              throw ETlgError("Checked baggage not found");

            lexh[0]=0;
            sscanf(tlg.lex,"%[^.]%s",lexh,tlg.lex);  //обрежим до точки

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

            //ph указывает на начало NAME-элемента в буфере lexh
            vector<string> names;
            ParseNameElement(ph,names,epNone);
            for(vector<string>::iterator i=names.begin();i!=names.end();i++)
            {
              if (i==names.begin())
                data.surname=*i;
              else
                data.name.push_back(*i);
            };

            //ищем рейс flt в con.OutFlt
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
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch (ETlgError E)
  {
    if (tlg.GetToEOLLexeme(line_p)!=NULL)
      throw ETlgError("%s: %s\n>>>>>LINE %d: %s",GetTlgElementName(e),E.what(),line,tlg.lex);
    else
      throw ETlgError("%s: %s\n>>>>>LINE %d",GetTlgElementName(e),E.what(),line);
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
         rem_code=="FQTS";
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
      if (isAdditionalSeat(*iPaxItem)) continue; //пропускаем возможные дополнительные места
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

  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  TTlgElement e;
  TIndicator Indicator;
  int e_part;
  bool pr_prev_rem;

  line_p=body.p;
  line=body.line-1;
  e=FlightElement;
  try
  {
    strcpy(con.flt.airline,info.flt.airline);
    GetAirline(con.flt.airline);
    con.flt.flt_no=info.flt.flt_no;
    strcpy(con.flt.suffix,info.flt.suffix);
    con.flt.scd=info.flt.scd;
    con.flt.pr_utc=info.flt.pr_utc;
    strcpy(con.flt.airp_dep,info.flt.airp_dep);
    GetAirp(con.flt.airp_dep);

    bool isPRL=strcmp(info.tlg_type,"PRL")==0;

    if (isPRL)
      e=Configuration;
    else
    e=BonusPrograms;
    do
    {
      line++;
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
              res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]/%s",RouteItem.station,tlg.lex);
              if (res!=2) throw ETlgError("Wrong format");
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
                res=sscanf(lexh,"%3lu%1[A-ZА-ЯЁ]%s",&SeatsItem.seats,SeatsItem.subcl,lexh);
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
              res=sscanf(tlg.lex,"%1[A-ZА-ЯЁ]/%[A-ZА-ЯЁ?]%c",RbdItem.subcl,lexh,&c);
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
              // ожидается routing element
              do
              {
                TRouteItem RouteItem;
                c=0;
                res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",RouteItem.station,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong station");
                GetAirp(RouteItem.station);
                for(iRouteItem=con.avail.begin();iRouteItem!=con.avail.end();iRouteItem++)
                {
                  if (iRouteItem==con.avail.begin()&&
                      strcmp(iRouteItem->station,con.flt.airp_dep)==0) continue; //первый пункт может совпадать с одним из последующих в AVAIL
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
              //ожидается space available element
              TSeatsItem SeatsItem;
              char minus;
              bool pr_error;
              for(pr_error=true;pr_error;pr_error=false)
              {
                res=sscanf(tlg.lex,"%1[A-ZА-ЯЁ]%s",SeatsItem.subcl,tlg.lex);
                if (res!=2)
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong seats format");
                };
                minus=0;
                c=0;
                res=sscanf(tlg.lex,"%3lu%c%c",&SeatsItem.seats,&minus,&c);
                if (c!=0||res<1||SeatsItem.seats<0||minus!=0&&minus!='-')
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong number of seats");
                };

                //проверим в RBD
                SeatsItem.cl=NoClass;
                for(iRbdItem=con.rbd.begin();iRbdItem!=con.rbd.end();iRbdItem++)
                  if (iRbdItem->rbds.find_first_of(SeatsItem.subcl[0])!=string::npos)
                  {
                    if (SeatsItem.cl!=NoClass)
                      throw ETlgError("Duplicate RBD subclass '%s'",SeatsItem.subcl);
                    if (iRbdItem->cl==NoClass)
                      throw ETlgError("Unknown RBD class code '%s'",iRbdItem->subcl);
                    SeatsItem.cl=iRbdItem->cl;
                 };
                if (SeatsItem.cl==NoClass&&(SeatsItem.cl=GetClass(SeatsItem.subcl))==NoClass)
                  throw ETlgError("Unknown subclass '%s'",SeatsItem.subcl);
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
              // ожидается routing element
              do
              {
                TRouteItem RouteItem;
                c=0;
                res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",RouteItem.station,&c);
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
              //ожидается transit element
              TSeatsItem SeatsItem;
              bool pr_error;
              for(pr_error=true;pr_error;pr_error=false)
              {
                res=sscanf(tlg.lex,"%1[A-ZА-ЯЁ]%s",SeatsItem.subcl,tlg.lex);
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

                //проверим в RBD
                SeatsItem.cl=NoClass;
                for(iRbdItem=con.rbd.begin();iRbdItem!=con.rbd.end();iRbdItem++)
                  if (iRbdItem->rbds.find_first_of(SeatsItem.subcl[0])!=string::npos)
                  {
                    if (SeatsItem.cl!=NoClass)
                      throw ETlgError("Duplicate RBD subclass '%s'",SeatsItem.subcl);
                    if (iRbdItem->cl==NoClass)
                      throw ETlgError("Unknown RBD class code '%s'",iRbdItem->subcl);
                    SeatsItem.cl=iRbdItem->cl;
                 };
                if (SeatsItem.cl==NoClass&&(SeatsItem.cl=GetClass(SeatsItem.subcl))==NoClass)
                  throw ETlgError("Unknown subclass '%s'",SeatsItem.subcl);
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
          if (tlg.lex[0]=='-')
          {
            TTotalsByDest Totals;
            char minus=0;
            c=0;
            res=sscanf(tlg.lex,"-%3[A-ZА-ЯЁ]%3lu%1[A-ZА-ЯЁ?]%cPAD%3lu%c",
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
              Totals.cl=NoClass;
              for(iRbdItem=con.rbd.begin();iRbdItem!=con.rbd.end();iRbdItem++)
                if (iRbdItem->rbds.find_first_of(Totals.subcl[0])!=string::npos)
                {
                  if (Totals.cl!=NoClass)
                    throw ETlgError("Duplicate RBD subclass '%s'",Totals.subcl);
                  if (iRbdItem->cl==NoClass)
                    throw ETlgError("Unknown RBD class code '%s'",iRbdItem->subcl);
                  Totals.cl=iRbdItem->cl;
                };
              if (Totals.cl==NoClass&&(Totals.cl=GetClass(Totals.subcl))==NoClass)
                throw ETlgError("Unknown subclass '%s'",Totals.subcl);
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
              if ((ph=strrchr(tlg.lex,'-'))!=NULL)
              {
                //попробуем разобрать идентификатор группы
                c=0;
                res=sscanf(ph,"-%2[A-Z]%3lu%c",grp_ref,&grp_seats,&c);
                if (c!=0||res!=2||grp_ref[0]==0||grp_seats<=0)
                {
                  //идентификатор группы не найден
                  *grp_ref=0;
                  grp_seats=0;
                }
                else
                {
                  for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
                  {
                    if (strcmp(iPnrItem->grp_ref,grp_ref)==0)
                    {
                      if (iPnrItem->grp_seats!=grp_seats)
                        throw ETlgError("Different number of seats in same group identifier");
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
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
    
    //3 прохода обработки ремарок
    //A. привязать ремарки к пассажирам
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();++iPnrItem)
        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
          BindRemarks(tlg,*i);
    
    //B. проанализировать дополнительные места пассажиров (после привязки, но до разбора ремарок!)
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
    {
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();++iPnrItem)
      {
        for(vector<TNameElement>::iterator i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();++i)
        {
          //два прохода в рамках пассажиров одного NameElement:
          //1. Проставляем EXST и STCR для тех пассажиров, у которых соответствующие личные ремарки
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (isAdditionalSeat(*iPaxItem))
            {
              vector<TPaxItem>::const_iterator minSeatsPax=findPaxForBlockSeats(iPaxItem, *iPnrItem, i);
              if (minSeatsPax!=iPaxItem)
              {
                TPaxItem &p=(TPaxItem&)*minSeatsPax;
                if (p.seats>=3) break; //нельзя для астры превышать кол-во мест 3
                p.seats++;
                p.rem.insert(p.rem.end(),
                             iPaxItem->rem.begin(),iPaxItem->rem.end());
                iPaxItem=i->pax.erase(iPaxItem);
                continue;
              };
            };
            ++iPaxItem;
          };
          //2. Проставляем EXST и STCR для пассажиров непосредственно перед EXST/STCR
          iPaxItemPrev=i->pax.end();
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (isAdditionalSeat(*iPaxItem))
            {
              if (iPaxItemPrev!=i->pax.end() && iPaxItemPrev->seats<3)
              {
                //EXST/STCR не первые в NameElement и предыдущий пассажир занимает менее 3 мест
                iPaxItemPrev->seats++;
                //перебросим ремарки
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
        // Проставляем EXST и STCR для тех пассажиров, у которых соответствующие личные ремарки
        // в рамках одного PNR??? а PRL???
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
                if (p.seats>=3) break; //нельзя для астры превышать кол-во мест 3
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
    
    //C. разобрать ремарки
    TSeatRemPriority seat_rem_priority;
    string airline_mark; //по какой компании посторен seat_rem_priority

    for(iTotals=con.resa.begin();iTotals!=con.resa.end();++iTotals)
    {
      GetSubcl(iTotals->subcl); //подклассы в базе храним русские
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
    if (tlg.GetToEOLLexeme(line_p)!=NULL)
      throw ETlgError("%s: %s\n>>>>>LINE %d: %s",GetTlgElementName(e),E.what(),line,tlg.lex);
    else
      throw ETlgError("%s: %s\n>>>>>LINE %d",GetTlgElementName(e),E.what(),line);
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
  //разбор ремарок
  c=0;
  res=sscanf(tlg.lex,".%[A-Z0-9]%c",lexh,&c);
  if (c!='/'||res!=2)
  {
    if (!pr_prev_rem||ne.rem.empty())
      throw ETlgError("Wrong identifying code format");

    //возможно это относится к ремарке свободного формата
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

  //сделаем trimRight если не относится к ремаркам
  for(int len=strlen(tlg.lex);len>=0&&*(unsigned char*)(tlg.lex+len)<=' ';len--)
    *(tlg.lex+len)=0;

  pr_prev_rem=false;
  if (strcmp(lexh,"C")==0 && strcmp(tlg.lex,".C/")!=0)
  {
    c=0;
    res=sscanf(tlg.lex,".C/%[A-ZА-ЯЁ0-9/ ]%c",lexh,&c);
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
    res=sscanf(tlg.lex,".L/%20[A-ZА-ЯЁ0-9]%[^.]",PnrAddr.addr,tlg.lex);
    if (res<1||PnrAddr.addr[0]==0) throw ETlgError("Wrong PNR address");
    if (res==2)
    {
      c=0;
      res=sscanf(tlg.lex,"/%3[A-ZА-ЯЁ0-9]%c",PnrAddr.airline,&c);
      if (c!=0||res!=1) throw ETlgError("Wrong PNR address");
      GetAirline(PnrAddr.airline);
    };
    if (PnrAddr.airline[0]==0) strcpy(PnrAddr.airline,flt.airline);

    //анализ на повторение
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
    //читаем priority
    c=0;
    res=sscanf(tlg.lex,".%*[A-Z0-9]%c%s",&c,tlg.lex);
    if (c!='/'||res==0)
      throw ETlgError("Wrong identifying code format");
    if (res==2)
    {
      //есть priority designator
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
    TTransferItem Transfer;
    const char *errMsg;
    bool pr_connection=strcmp(lexh,"M")!=0;
    if (!pr_connection)
    {
      errMsg="Wrong marketing flight element";
      c=0;
      res=sscanf(tlg.lex,".M/%[A-ZА-ЯЁ0-9]%c",lexh,&c);
    }
    else
    {
      errMsg="Wrong connection element";
      if (lexh[1]==0)
      {
        Transfer.num=1;
        if (lexh[0]=='I') Transfer.num=-Transfer.num;
        c=0;
        res=sscanf(tlg.lex,".%*c/%[A-ZА-ЯЁ0-9]%c",lexh,&c);
      }
      else
      {
        c=0;
        res=sscanf(lexh+1,"%1lu%c",&Transfer.num,&c);
        if (res==0) return; //другой элемент
        if (c!=0||res!=1||Transfer.num<=0)
          throw ETlgError(errMsg);
        if (lexh[0]=='I') Transfer.num=-Transfer.num;
        c=0;
        res=sscanf(tlg.lex,".%*c%*1[0-9]/%[A-ZА-ЯЁ0-9]%c",lexh,&c);
      };
    };
    if (c!=0||res!=1||strlen(lexh)<3) throw ETlgError(errMsg);
    c=0;
    if (IsDigit(lexh[2]))
    {
      res=sscanf(lexh,"%2[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%2[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                        Transfer.airline,&Transfer.flt_no,
                        Transfer.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError(errMsg);
        GetSuffix(Transfer.suffix[0]);
      };
    }
    else
    {
      res=sscanf(lexh,"%3[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%3[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
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
    res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%[A-ZА-ЯЁ0-9]",
                       Transfer.airp_dep,Transfer.airp_arv,lexh);
    if (res<2||Transfer.airp_dep[0]==0||Transfer.airp_arv[0]==0)
    {
      if (!pr_connection) throw ETlgError(errMsg);
      Transfer.airp_dep[0]=0;
      lexh[0]=0;
      res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%[A-ZА-ЯЁ0-9]",
                         Transfer.airp_dep,lexh);
      if (res<1||Transfer.airp_dep[0]==0) throw ETlgError(errMsg);
    };
    if (lexh[0]!=0)
    {
      c=0;
      res=sscanf(lexh,"%4lu%2[A-ZА-ЯЁ]%c",&Transfer.local_time,tlg.lex,&c);
      if (c!=0||res!=2)
      {
        c=0;
        res=sscanf(lexh,"%4lu%c",&Transfer.local_time,&c);
        if (c!=0||res!=1) throw ETlgError(errMsg);
      };
    };
    if (!pr_connection)
    {
      GetAirline(Transfer.airline);
      GetAirp(Transfer.airp_dep);
      GetAirp(Transfer.airp_arv);
      GetSubcl(Transfer.subcl);
      //анализ на повторение
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
      if (Transfer.num>0&&Transfer.airp_arv[0]==0)
      {
        strcpy(Transfer.airp_arv,Transfer.airp_dep);
        Transfer.airp_dep[0]=0;
      };
      //анализ на повторение
      vector<TTransferItem>::iterator i;
      for(i=pnr.transfer.begin();i!=pnr.transfer.end();i++)
        if (Transfer.num==i->num) break;
      if (i!=pnr.transfer.end())
      {
        if (strcmp(i->airline,Transfer.airline)!=0||
            i->flt_no!=Transfer.flt_no||
            strcmp(i->suffix,Transfer.suffix)!=0||
            i->airp_dep[0]!=0&&Transfer.airp_dep[0]!=0&&
            strcmp(i->airp_dep,Transfer.airp_dep)!=0||
            i->local_date!=Transfer.local_date||
            i->airp_arv[0]!=0&&Transfer.airp_arv[0]!=0&&
            strcmp(i->airp_arv,Transfer.airp_arv)!=0||
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
      if (c!=0||res<2||ne.bag.bag_amount<0||ne.bag.bag_weight<0) throw ETlgError("Wrong pieces/weight data element");
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
    res=sscanf(tlg.lex,".N/%10[0-9]%3[0-9]/%3[A-ZА-ЯЁ]%c",tagh.first_no,tagh.num,tag.airp_arv_final,&c);
    if (c!=0||res!=3)
    {
      c=0;
      res=sscanf(tlg.lex,".N/%9[A-ZА-ЯЁ0-9]/%3[0-9]/%3[A-ZА-ЯЁ]%c",tagh.first_no,tagh.num,tag.airp_arv_final,&c);
      if (c!=0||res!=3)
        throw ETlgError("Wrong baggage tag details element");
      if (strlen(tagh.first_no)<8)
        throw ETlgError("Wrong baggage tag ID number");
        
      lexh[0]=0;
      c=0;
      if (IsDigit(tagh.first_no[2]))
        res=sscanf(tagh.first_no,"%2[A-ZА-ЯЁ0-9]%6[0-9]%c",tag.alpha_no,lexh,&c);
      else
        res=sscanf(tagh.first_no,"%3[A-ZА-ЯЁ0-9]%6[0-9]%c",tag.alpha_no,lexh,&c);
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
        throw ETlgError("Wrong baggage tag details element");
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

void ParseNameElement(char* p, vector<string> &names, TElemPresence num_presence)
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
  //выбираем первую лексему

  if (*tlg.lex!=0)
  {
    c=0;
    res=sscanf(tlg.lex,"%3[0-9]%[A-ZА-ЯЁ -]%c",numh,lexh,&c);
    if (c!=0||res!=2)
    {
      if (num_presence==epMandatory) throw ETlgError("Wrong format");
      c=0;
      res=sscanf(tlg.lex,"%[A-ZА-ЯЁ -]%c",lexh,&c);
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
    res=sscanf(tlg.lex,"%*[A-ZА-ЯЁ -]%c",&c);
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
  unsigned int pos;
  bool pr_parse;
  string strh;
  char *p;
  char rem_code[7],numh[4];
  int num;
  vector<TRemItem>::iterator iRemItem;
  vector<TPaxItem>::iterator iPaxItem;

  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();++iRemItem)
  {
    TrimString(iRemItem->text);
    if (iRemItem->text.empty()) continue;
    p=tlg.GetWord((char*)iRemItem->text.c_str());
    c=0;
    res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
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
  //попробовать привязать ремарки к конкретным пассажирам
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();)
  {
    if (iRemItem->text.empty())
    {
      ++iRemItem;
      continue;
    };

    //ищем ссылку на пассажира до совпадения фамилий
    vector<string> names;
    pos=iRemItem->text.find_last_of('-');
    while(pos!=string::npos)
    {
      strh=iRemItem->text.substr(pos+1);
      try
      {
        ParseNameElement((char*)strh.c_str(),names,epOptional);
        if (!names.empty()&&*(names.begin())==ne.surname) break; //нашли
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
              if (k==0&&iPaxItem->name!=*i||
                  k!=0&&OnlyAlphaInLexeme(iPaxItem->name)!=OnlyAlphaInLexeme(*i)) continue;
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

      if (pr_parse) iRemItem->text.erase(pos); //убрать окончание ремарки после -
    }
    else
    {
      //не нашли ссылку на пассажира
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

bool ParseDOCSRem(TTlgParser &tlg, BASIC::TDateTime scd_local, std::string &rem_text, TDocItem &doc);
bool ParseDOCORem(TTlgParser &tlg, BASIC::TDateTime scd_local, std::string &rem_text, TDocoItem &doc);

void ParseRemarks(const vector< pair<string,int> > &seat_rem_priority,
                  TTlgParser &tlg, const TDCSHeadingInfo &info, TPnrItem &pnr, TNameElement &ne)
{
  vector<TRemItem>::iterator iRemItem;
  vector<TPaxItem>::iterator iPaxItem,iPaxItem2;
  vector<TNameElement>::iterator iNameElement;

  //сначала пробегаем по ремаркам каждого из пассажиров (из-за номеров мест)
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
                  //отдельный массив для INF
              vector<TInfItem> inf;
              if (ParseINFRem(tlg,iRemItem->text,inf))
              {
                for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
                {
                  if (i->surname.empty()) i->surname=ne.surname;
                  //проверим есть ли уже ребенок с таким именем и фамилией в iPaxItem->inf
                  //(защита от дублирования INF и INFT)
                  vector<TInfItem>::iterator j;
                  for(j=iPaxItem->inf.begin();j!=iPaxItem->inf.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=iPaxItem->inf.end())
                  {
                    //дублирование INF. удалим из inf
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
                //проверим документ РМ
                try
                {
                  TGenderTypesRow &row=(TGenderTypesRow&)(getBaseTable(etGenderType).get_row("code/code_lat",doc.gender));
                  if (row.pr_inf)
                  {
                    if (iPaxItem->inf.size()==1)
                    {
                      iPaxItem->inf.begin()->doc.push_back(doc);
                      iPaxItem->inf.begin()->rem.push_back(*iRemItem);
                      iRemItem->text.clear();
                    };
                  }
                  else
                    iPaxItem->doc.push_back(doc);
                }
                catch(EBaseTableError)
                {
                  iPaxItem->doc.push_back(doc);
                };
              };
              continue;
            };
            
            if (strcmp(iRemItem->code,"DOCO")==0)
            {
              TDocoItem doc;
              if (ParseDOCORem(tlg,info.flt.scd,iRemItem->text,doc))
              {
                //проверим документ РМ
                if (doc.pr_inf)
                {
                  if (iPaxItem->inf.size()==1)
                  {
                    iPaxItem->inf.begin()->doco.push_back(doc);
                    iPaxItem->inf.begin()->rem.push_back(*iRemItem);
                    iRemItem->text.clear();
                  };
                }
                else
                  iPaxItem->doco.push_back(doc);
              };
              continue;
            };

            if (strcmp(iRemItem->code,"TKNA")==0||
                strcmp(iRemItem->code,"TKNE")==0)
            {
              TTKNItem tkn;
              if (ParseTKNRem(tlg,iRemItem->text,tkn))
              {
                //проверим билет РМ
                if (tkn.pr_inf)
                {
                  if (iPaxItem->inf.size()==1)
                  {
                    //только если у пассажира один infant - знаем что билет относится к нему
                    iPaxItem->inf.begin()->tkn.push_back(tkn);
                    iPaxItem->inf.begin()->rem.push_back(*iRemItem);
                    iRemItem->text.clear();
                  };
                }
                else
                  iPaxItem->tkn.push_back(tkn);
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
            vector<TSeatRange> seats;
            TSeat seat;
            if (ParseSEATRem(tlg,iRemItem->text,seats))
            {
              sort(seats.begin(),seats.end());
              iPaxItem->seatRanges.insert(iPaxItem->seatRanges.end(),seats.begin(),seats.end());
              if (iPaxItem->seat.Empty())
              {
                //если место не определено, попробовать привязать
                for(vector<TSeatRange>::iterator i=seats.begin();i!=seats.end();i++)
                {
                  seat=i->first;
                  do
                  {
                    //записать место из seat, если не найдено ни у кого другого
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
      //пробегаем по ремаркам группы
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
                //отдельный массив для INF
            vector<TInfItem> inf;
            if (ParseINFRem(tlg,iRemItem->text,inf))
            {
              for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
              {
                if (i->surname.empty()) i->surname=ne.surname;
                if (ne.pax.size()==1 && ne.pax.begin()->inf.empty())
                {
                  //проверим есть ли уже ребенок с таким именем и фамилией в ne.pax.begin()->inf
                  //(защита от дублирования INF и INFT)
                  vector<TInfItem>::iterator j;
                  for(j=ne.pax.begin()->inf.begin();j!=ne.pax.begin()->inf.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=ne.pax.begin()->inf.end())
                  {
                    //дублирование INF. удалим из inf
                    i=inf.erase(i);
                    continue;
                  };
                };
                i++;
              };

              if (ne.pax.size()==1 && ne.pax.begin()->inf.empty() &&
                  inf.size()==1)
              {
                //однозначная привязка к единственному пассажиру
                ne.pax.begin()->inf.insert(ne.pax.begin()->inf.end(),inf.begin(),inf.end());
              }
              else
              {
                for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();)
                {
                  //проверим есть ли уже ребенок с таким именем и фамилией в infs
                  //(защита от дублирования INF и INFT)
                  vector<TInfItem>::iterator j;
                  for(j=infs.begin();j!=infs.end();j++)
                    if (i->surname==j->surname && i->name==j->name) break;
                  if (j!=infs.end())
                  {
                    //дублирование INF. удалим из inf
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
                //проверим документ РМ
                try
                {
                  TGenderTypesRow &row=(TGenderTypesRow&)(getBaseTable(etGenderType).get_row("code/code_lat",doc.gender));
                  if (row.pr_inf)
                  {
                    if (ne.pax.begin()->inf.size()==1)
                    {
                      ne.pax.begin()->inf.begin()->doc.push_back(doc);
                      ne.pax.begin()->inf.begin()->rem.push_back(*iRemItem);
                      iRemItem->text.clear();
                    };
                  }
                  else
                  {
                    ne.pax.begin()->doc.push_back(doc);
                    ne.pax.begin()->rem.push_back(*iRemItem);
                    iRemItem->text.clear();
                  };
                }
                catch(EBaseTableError)
                {
                  if (*doc.gender==0)
                  {
                    //неизвестный тип пассажира
                    ne.pax.begin()->doc.push_back(doc);
                    ne.pax.begin()->rem.push_back(*iRemItem);
                    iRemItem->text.clear();
                  }
                };
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
                //проверим билет РМ
                if (tkn.pr_inf)
                {
                  if (ne.pax.begin()->inf.size()==1)
                  {
                    //только если у пассажира один infant - знаем что билет относится к нему
                    ne.pax.begin()->inf.begin()->tkn.push_back(tkn);
                    ne.pax.begin()->inf.begin()->rem.push_back(*iRemItem);
                    iRemItem->text.clear();
                  };
                }
                else
                {
                  ne.pax.begin()->tkn.push_back(tkn);
                  ne.pax.begin()->rem.push_back(*iRemItem);
                  iRemItem->text.clear();
                };

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
        };
      };
    };
    if (pass==2)
    {
      for(vector< pair<string,int> >::const_iterator r=seat_rem_priority.begin();
                                                     r!=seat_rem_priority.end(); r++ )
      {
        //пробегаем по ремаркам группы
        for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
        {
          if (iRemItem->text.empty()) continue;
          
          if (iRemItem->code!=r->first) continue;
          vector<TSeatRange> seats;
          TSeat seat;
          if (ParseSEATRem(tlg,iRemItem->text,seats))
          {
            sort(seats.begin(),seats.end());
            ne.seatRanges.insert(ne.seatRanges.end(),seats.begin(),seats.end());

            //попробуем привязать
            for(vector<TSeatRange>::iterator i=seats.begin();i!=seats.end();i++)
            {
              seat=i->first;
              do
              {
                //записать место из seat, если не найдено ни у кого другого из группы
                for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
                {
                  for(iPaxItem2=iNameElement->pax.begin();iPaxItem2!=iNameElement->pax.end();iPaxItem2++)
                    if (!iPaxItem2->seat.Empty() && iPaxItem2->seat==seat) break;
                  if (iPaxItem2!=iNameElement->pax.end()) break;
                };
                if (iNameElement==pnr.ne.end())
                {
                  //место ни у кого из группы не найдено
                  //найти пассажира, которому не проставлено место
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

                  //в первую очередь распределяем места текущему NameElement
                  for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                    if (iPaxItem2->seat.Empty())
                    {
                      iPaxItem2->seat=seat;
                      strcpy(iPaxItem2->seat_rem,i->rem);
                      break;
                    };
                  if (iPaxItem2!=ne.pax.end()) continue;

                  //пробежимся по всей группе
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

  //раскидаем непривязанных РМ
  for(vector<TInfItem>::iterator iInfItem=infs.begin();iInfItem!=infs.end();iInfItem++)
  {
    for(int k=0;k<=3;k++)
    {
      //0. привязываем к первому ВЗ свободному от inf
      //1. привязываем к первому ВЗ
      //2. привязываем к первому свободному от inf
      //3. привязываем к первому
      for(vector<TPaxItem>::iterator i=ne.pax.begin();i!=ne.pax.end();i++)
      {
        if (k==0 && i->pers_type==adult && i->inf.empty() ||
            k==1 && i->pers_type==adult ||
            k==2 && i->inf.empty() ||
            k==3)
        {
          i->inf.push_back(*iInfItem);
          break;
        };
      };
    };
  };
};

void ParseSeatRange(string str, vector<TSeatRange> &ranges, bool usePriorContext)
{
  char c;
  int res;
  TSeatRange seatr,seatr2;
  char lexh[MAX_LEXEME_SIZE+1];

  static TSeat priorMaxSeat;

  ranges.clear();

  do
  {
    //проверим диапазон (напр. 21C-24F )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]%1[A-ZА-ЯЁ]-%3[0-9]%1[A-ZА-ЯЁ]%c",
               seatr.first.row,seatr.first.line,
               seatr.second.row,seatr.second.line,
               &c);
    if (c==0 && res==4)
    {
      NormalizeSeatRange(seatr);
      if (strcmp(seatr.first.row,seatr.second.row)>0)
        throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());

      //разобъем на поддиапазоны
      if (strcmp(seatr.first.row,seatr.second.row)==0)
      {
        //один ряд
        if (strcmp(seatr.first.line,seatr.second.line)>0)
          throw EConvertError("ParseSeatRange: wrong seat range %s", str.c_str());

        ranges.push_back(seatr);
      }
      else
      {
        //много рядов
        if (strcmp(seatr.first.line,FirstNormSeatLine(seatr2.first).line)!=0)
        {
          //первый ряд
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
          //последний ряд
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

    //проверим блок мест (напр. 21-24A-D )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]-%3[0-9]%1[A-ZА-ЯЁ]-%1[A-ZА-ЯЁ]%c",
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

    //проверим блок мест одной линии (напр. 21-23A )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]-%3[0-9]%1[A-ZА-ЯЁ]%c",
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

    //проверим блок мест одного ряда (напр. 21A-C )
    c=0;
    res=sscanf(str.c_str(),"%3[0-9]%1[A-ZА-ЯЁ]-%1[A-ZА-ЯЁ]%c",
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

    //проверим ROW
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

    //проверим перечисление (напр. 4C5A6B20AC21ABD)
    //важно, что этот блок разбора идет последним в ParseSeatRange!
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
    //упорядочим seats и попробуем преобразовать в диапазоны
    sort(seats.begin(),seats.end());
    seatr.second.Clear();
    for(vector<TSeat>::iterator i=seats.begin();i!=seats.end();i++)
    {
      strcpy(seat.row,i->row);
      strcpy(seat.line,i->line);

      if (seatr.second.Empty()||
          strcmp(seatr.second.row,seat.row)!=0||
          strcmp(seatr.second.line,seat.line)!=0&&
          (!PriorNormSeatLine(seat)||
           strcmp(seatr.second.line,seat.line)!=0))
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
  for(vector<TSeatRange>::iterator i=ranges.begin();i!=ranges.end();i++)
  {
    if (priorMaxSeat<i->second) priorMaxSeat=i->second;
  };
};

bool ParseSEATRem(TTlgParser &tlg,string &rem_text,vector<TSeatRange> &seats)
{
  char c;
  int res;
  char rem_code[6];
  pair<char[4],char[4]> row;
  pair<char,char> line;

  char *p=(char*)rem_text.c_str();

  seats.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-ZА-ЯЁ0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  if (isSeatRem(rem_code))
  {
    bool usePriorContext=false;
    vector<TSeatRange> ranges;
    while((p=tlg.GetLexeme(p))!=NULL)
    {
      try
      {
        ParseSeatRange(tlg.lex,ranges,usePriorContext);
        for(vector<TSeatRange>::iterator i=ranges.begin();i!=ranges.end();i++)
        {
          strcpy(i->rem,rem_code);
        };
        seats.insert(seats.end(),ranges.begin(),ranges.end());
        usePriorContext=true;
      }
      catch(EConvertError)
      {
        //диапазон не разобрался - ну и хрен с ним
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

  char *p=(char*)rem_text.c_str();

  chd.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
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

  char *p=(char*)rem_text.c_str();

  inf.clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
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
      //добавим пустых младенцев по кол-ву num
      TInfItem infItem;
      inf.push_back(infItem);
    };
    return true;
  };

  return false;
};

bool ParseDOCORem(TTlgParser &tlg, TDateTime scd_local, string &rem_text, TDocoItem &doc)
{
  char c;
  int res,k;

  char *p=(char*)rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-ZА-ЯЁ0-9]%c",doc.rem_code,&c);
  if (c!=0||res!=1) return false;

  TDateTime now=(scd_local!=0 && scd_local!=NoExists)?scd_local:NowUTC();

  if (strcmp(doc.rem_code,"DOCO")==0)
  {
    for(k=0;k<=7;k++)
    try
    {
      try
      {
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
          case 4:
            res=sscanf(tlg.lex,"%[A-ZА-ЯЁ0-9 -]%c",lexh,&c);
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
            res=sscanf(tlg.lex,"%15[A-ZА-ЯЁ0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 5:
            if (StrToDateTime(tlg.lex,"ddmmmyy",now,doc.issue_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",now,doc.issue_date,false)==EOF)
              throw ETlgError("Wrong format");
            if (doc.issue_date!=NoExists && doc.issue_date>now) throw ETlgError("Strange data");
            break;
          case 6:
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",lexh,&c);
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

bool ParseDOCSRem(TTlgParser &tlg, TDateTime scd_local, string &rem_text, TDocItem &doc)
{
  char c;
  int res,k;

  char *p=(char*)rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-ZА-ЯЁ0-9]%c",doc.rem_code,&c);
  if (c!=0||res!=1) return false;

  TDateTime now=(scd_local!=0 && scd_local!=NoExists)?scd_local:NowUTC();

  if (strcmp(doc.rem_code,"DOCS")==0)
  {
    for(k=0;k<=11;k++)
    try
    {
      try
      {
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
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==2)
              GetPaxDocCountry(lexh,doc.issue_country);
            else
              GetPaxDocCountry(lexh,doc.nationality);
            break;
          case 3:
            res=sscanf(tlg.lex,"%15[A-ZА-ЯЁ0-9 ]%c",doc.no,&c);
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
            res=sscanf(tlg.lex,"%[A-ZА-ЯЁ -]%c",lexh,&c);
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
            res=sscanf(tlg.lex,"%15[A-ZА-ЯЁ0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 2:
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",lexh,&c);
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
            res=sscanf(tlg.lex,"%[A-ZА-ЯЁ -]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==4) doc.surname=lexh;
            if (k==5) doc.first_name=lexh;
            break;
          case 6:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.gender,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
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

bool ParseTKNRem(TTlgParser &tlg,string &rem_text,TTKNItem &tkn)
{
  char c;
  int res,k;

  char *p=(char*)rem_text.c_str();

  tkn.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-ZА-ЯЁ0-9]%c",tkn.rem_code,&c);
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
        if (p==NULL && k>=2 && strcmp(tkn.rem_code,"TKNA")==0) break; //купон не обязателен для TKNA
        if (p==NULL) throw ETlgError("Lexeme not found");
        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%2[A-Z]%1[1]%c",tkn.rem_status,lexh,&c);
            if (c!=0||res!=2) throw ETlgError("Wrong format");
            break;
          case 1:
            res=sscanf(tlg.lex,"INF%15[A-ZА-ЯЁ0-9]%c",tkn.ticket_no,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%15[A-ZА-ЯЁ0-9]%c",tkn.ticket_no,&c);
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

  char *p=(char*)rem_text.c_str();

  fqt.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-ZА-ЯЁ0-9]%c",fqt.rem_code,&c);
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
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",fqt.airline,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%c",fqt.airline,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong format");
            };
            GetAirline(fqt.airline);
            break;
          case 1:
            res=sscanf(tlg.lex,"%25[A-ZА-ЯЁ0-9]%c",fqt.no,&c);
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

TTlgParts GetParts(char* tlg_p, TMemoryManager &mem)
{
  int line;
  char *p,*line_p;
  TTlgParser tlg;
  THeadingInfo *HeadingInfo=NULL;
  TTlgParts parts;
  TTlgElement e;

  parts.addr.p=tlg_p;
  parts.addr.line=1;
  try
  {
    line_p=parts.addr.p;
    line=parts.addr.line-1;
    e=Address;
    do
    {
      line++;
      if ((p=tlg.GetLexeme(line_p))==NULL) continue;
      switch (e)
      {
        case Address:
          if (tlg.lex[0]=='.')
          {
            parts.heading.p=line_p;
            parts.heading.line=line;
            e=CommunicationsReference;
          }
          else break;
        case CommunicationsReference:
          parts.body=ParseHeading(parts.heading,HeadingInfo,mem);  //может вернуть NULL
          line_p=parts.body.p;
          line=parts.body.line;
          e=EndOfMessage;
          if ((p=tlg.GetLexeme(line_p))==NULL) break;
        case EndOfMessage:
          switch (HeadingInfo->tlg_cat)
          {
            case tcDCS:
            case tcBSM:
              if (strstr(tlg.lex,"END")==tlg.lex)
              {
                parts.ending.p=line_p;
                parts.ending.line=line;
                e=EndOfMessage;
              };
              break;
            case tcAHM:
              if (strcmp(tlg.lex,"PART")==0)
              {
                parts.ending.p=line_p;
                parts.ending.line=line;
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

    if (parts.addr.p==NULL) throw ETlgError("Address not found");
    if (parts.heading.p==NULL) throw ETlgError("Heading not found");
    if (parts.body.p==NULL) throw ETlgError("Body not found");
    if (parts.ending.p==NULL&&
        (HeadingInfo->tlg_cat==tcDCS||
         HeadingInfo->tlg_cat==tcBSM)) throw ETlgError("End of message not found");

    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
  }
  catch(...)
  {
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    throw;
  };
  return parts;
};

int SaveFlt(int tlg_id, TFltInfo& flt, TBindType bind_type)
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
    bind_tlg(point_id, false);

    /* здесь проверим непривязанные сегменты из crs_displace и привяжем */
    /* но только для PNL/ADL */
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
  else point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tlg_source(point_id_tlg,tlg_id) "
    "VALUES(:point_id_tlg,:tlg_id)";
  Qry.CreateVariable("point_id_tlg",otInteger,point_id);
  Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
  };
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
      "      tlg_source.point_id_tlg=:point_id AND tlgs_in.type=:tlg_type";
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
      "      tlg_source.point_id_tlg=:point_id_in AND tlgs_in.type=:tlg_type";
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

void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, TBtmContent& con)
{
  vector<TBtmTransferInfo>::iterator iIn;
  vector<TBtmOutFltInfo>::iterator iOut;
  vector<TBtmGrpItem>::iterator iGrp;
  vector<TBtmPaxItem>::iterator iPax;
  vector<TBSMTagItem>::iterator iTag;
  vector<string>::iterator i;
  int point_id_in,point_id_out;

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

  for(iIn=con.Transfer.begin();iIn!=con.Transfer.end();iIn++)
  {
    point_id_in=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(iIn->InFlt),btLastSeg);
    if (!DeletePTMBTMContent(point_id_in,info)) continue;
    TrferQry.SetVariable("point_id_in",point_id_in);
    TrferQry.SetVariable("subcl_in",iIn->InFlt.subcl);
    for(iOut=iIn->OutFlt.begin();iOut!=iIn->OutFlt.end();iOut++)
    {
      point_id_out=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(*iOut),btFirstSeg);
      TrferQry.SetVariable("point_id_out",point_id_out);
      TrferQry.SetVariable("subcl_out",iOut->subcl);
      TrferQry.Execute();
      for(iGrp=iOut->grp.begin();iGrp!=iOut->grp.end();iGrp++)
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
        for(iPax=iGrp->pax.begin();iPax!=iGrp->pax.end();iPax++)
        {
          PaxQry.SetVariable("surname",iPax->surname);
          if (!iPax->name.empty())
          {
            for(i=iPax->name.begin();i!=iPax->name.end();i++)
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
        for(iTag=iGrp->tags.begin();iTag!=iGrp->tags.end();iTag++)
          for(int j=0;j<iTag->num;j++)
          {
            TagQry.SetVariable("no",iTag->first_no+j);
            TagQry.Execute();
          };
      };
    };
  };
};

void SavePTMContent(int tlg_id, TDCSHeadingInfo& info, TPtmContent& con)
{
  vector<TPtmOutFltInfo>::iterator iOut;
  vector<TPtmTransferData>::iterator iData;
  vector<string>::iterator i;
  int point_id_in,point_id_out;

  point_id_in=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(con.InFlt),btLastSeg);
  if (!DeletePTMBTMContent(point_id_in,info)) return;

  TQuery TrferQry(&OraSession);
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(cycle_id__seq.nextval,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
  TrferQry.CreateVariable("subcl_in",otString,FNull); //подкласс прилета неизвестен
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
    point_id_out=SaveFlt(tlg_id,dynamic_cast<TFltInfo&>(*iOut),btFirstSeg);
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
  //ремарки пассажира
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

void SaveDOCSRem(int pax_id, const vector<TDocItem> &doc)
{
  if (doc.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_doc "
    "  (pax_id,rem_code,rem_status,type,issue_country,no,nationality, "
    "   birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "VALUES "
    "  (:pax_id,:rem_code,:rem_status,:type,:issue_country,:no,:nationality, "
    "   :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi) ";
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
    "   applic_country,pr_inf) "
    "VALUES "
    "  (:pax_id,:rem_code,:rem_status,:birth_place,:type,:no,:issue_place,:issue_date, "
    "   :applic_country,:pr_inf) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("rem_status",otString);
  Qry.DeclareVariable("birth_place",otString);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("issue_place",otString);
  Qry.DeclareVariable("issue_date",otDate);
  Qry.DeclareVariable("applic_country",otString);
  Qry.DeclareVariable("pr_inf",otInteger);
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
    Qry.SetVariable("pr_inf",(int)i->pr_inf);
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
    "  (pax_id,rem_code,ticket_no,coupon_no,pr_inf) "
    "VALUES "
    "  (:pax_id,:rem_code,:ticket_no,:coupon_no,:pr_inf) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("ticket_no",otString);
  Qry.DeclareVariable("coupon_no",otInteger);
  Qry.DeclareVariable("pr_inf",otInteger);
  for(vector<TTKNItem>::iterator i=tkn.begin();i!=tkn.end();i++)
  {
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("ticket_no",i->ticket_no);
    if (i->coupon_no!=0)
      Qry.SetVariable("coupon_no",i->coupon_no);
    else
      Qry.SetVariable("coupon_no",FNull);
    Qry.SetVariable("pr_inf",(int)i->pr_inf);
    Qry.Execute();
  };
};

void SaveFQTRem(int pax_id, vector<TFQTItem> &fqt)
{
  if (fqt.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO crs_pax_fqt "
    "  (pax_id,rem_code,airline,no,extra) "
    "VALUES "
    "  (:pax_id,:rem_code,:airline,:no,:extra) ";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.DeclareVariable("rem_code",otString);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("extra",otString);
  for(vector<TFQTItem>::iterator i=fqt.begin();i!=fqt.end();i++)
  {
    Qry.SetVariable("rem_code",i->rem_code);
    Qry.SetVariable("airline",i->airline);
    Qry.SetVariable("no",i->no);
    if (i->extra.size()>250) i->extra.erase(250);
    Qry.SetVariable("extra",i->extra);
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
  int point_id=SaveFlt(tlg_id,con.flt,btFirstSeg);
  if (!DeleteSOMContent(point_id,info)) return;

  bool usePriorContext=false;
  int curr_tid=NoExists;
  for(vector<TSeatsByDest>::iterator i=con.seats.begin();i!=con.seats.end();i++)
  {
    //здесь надо удалить все слои телеграмм SOM из более ранних пунктов из trip_comp_layers
    TPointIdsForCheck point_ids_spp;
    InsertTlgSeatRanges(point_id,i->airp_arv,cltSOMTrzt,i->ranges,NoExists,tlg_id,NoExists,usePriorContext,curr_tid,point_ids_spp);
    check_layer_change(point_ids_spp);
    usePriorContext=true;
  };
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

  int point_id=SaveFlt(tlg_id,con.flt,btFirstSeg);

  //идентифицируем систему бронирования
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
            last_cfg=NoExists;
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
        "INSERT INTO crs_set(id,airline,flt_no,airp_dep,crs,priority,pr_numeric_pnl) "
        "VALUES(id__seq.nextval,:airline,:flt_no,:airp_dep,:crs,0,0)";
      Qry.SetVariable("flt_no",FNull);
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
      "SELECT last_resa,last_tranzit,last_avail,last_cfg,pr_pnl FROM crs_data_stat "
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
      pr_pnl=Qry.FieldAsInteger("pr_pnl");
    };

    //pr_pnl=0 - не пришел цифровой PNL
    //pr_pnl=1 - пришел цифровой PNL
    //pr_pnl=2 - пришел нецифровой PNL
  };
  
  bool pr_save_ne=isPRL ||
                  !(strcmp(info.tlg_type,"PNL")==0&&pr_pnl==2|| //пришел второй нецифровой PNL
                    strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2); //пришел ADL до обоих PNL

  bool pr_recount=false;
  //записать цифровые данные
  if (!con.resa.empty()&&(last_resa==NoExists||last_resa<=info.time_create))
  {
    pr_recount=true;
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET resa=NULL,pad=NULL "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender; "
      "  IF :system='CRS' THEN "
      "    UPDATE crs_data_stat SET last_resa=:time_create WHERE point_id=:point_id AND crs=:sender; "
      "    IF SQL%NOTFOUND THEN "
      "      INSERT INTO crs_data_stat(point_id,crs,last_resa) "
      "      VALUES(:point_id,:sender,:time_create); "
      "    END IF; "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,system);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

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
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET tranzit=NULL "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender; "
      "  IF :system='CRS' THEN "
      "    UPDATE crs_data_stat SET last_tranzit=:time_create WHERE point_id=:point_id AND crs=:sender; "
      "    IF SQL%NOTFOUND THEN "
      "      INSERT INTO crs_data_stat(point_id,crs,last_tranzit) "
      "      VALUES(:point_id,:sender,:time_create); "
      "    END IF; "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,system);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

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
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET avail=NULL "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender; "
      "  IF :system='CRS' THEN "
      "    UPDATE crs_data_stat SET last_avail=:time_create WHERE point_id=:point_id AND crs=:sender; "
      "    IF SQL%NOTFOUND THEN "
      "      INSERT INTO crs_data_stat(point_id,crs,last_avail) "
      "      VALUES(:point_id,:sender,:time_create); "
      "    END IF; "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,system);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

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
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE crs_data SET cfg=NULL "
      "  WHERE point_id=:point_id AND system=:system AND sender=:sender; "
      "  IF :system='CRS' THEN "
      "    UPDATE crs_data_stat SET last_cfg=:time_create WHERE point_id=:point_id AND crs=:sender; "
      "    IF SQL%NOTFOUND THEN "
      "      INSERT INTO crs_data_stat(point_id,crs,last_cfg) "
      "      VALUES(:point_id,:sender,:time_create); "
      "    END IF; "
      "  END IF; "
      "END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("system",otString,system);
    Qry.CreateVariable("sender",otString,info.sender);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

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
  if (pr_recount) crs_recount(point_id,NoExists,true);

  OraSession.Commit();

  try
  {
    if (pr_save_ne)
    {
      //определим, является ли PNL чисто цифровым (с учетом ZZ)
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
        //получим идентификатор транзакции
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
          "    INSERT INTO crs_pax(pax_id,pnr_id,surname,name,pers_type,seat_xname,seat_yname,seat_rem,seat_type,seats,bag_pool,pr_del,last_op,tid) "
          "    VALUES(:pax_id,:pnr_id,:surname,:name,:pers_type,:seat_xname,:seat_yname,:seat_rem,:seat_type,:seats,:bag_pool,:pr_del,:last_op,cycle_tid__seq.currval); "
          "  ELSE "
          "    UPDATE crs_pax "
          "    SET pers_type= :pers_type, seat_xname= :seat_xname, seat_yname= :seat_yname, seat_rem= :seat_rem, "
          "        seat_type= :seat_type, bag_pool= :bag_pool, pr_del= :pr_del, last_op= :last_op, tid=cycle_tid__seq.currval "
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
        for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
        {
          CrsPnrQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("airp_arv",iTotals->dest);
          CrsPnrInsQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("class",EncodeClass(iTotals->cl));
          for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
          {
            TPnrItem& pnr=*iPnrItem;
            pr_sync_pnr=true;
            pnr_id=0;
            //попробовать найти pnr_id по PNR reference
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
              //поиск группы по всем пассажирам данной группы
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
                        if (ids.empty()) break; //пассажиры в разных группах
                      };
                    };
                    if (iPaxItem!=ne.pax.end()) break;
                  };
                };
                if (!ids.empty()) pnr_id=*ids.begin();  //берем первую подходящую группу
              };
            };

            //создать новую группу или проапдейтить старую
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

            for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end();iNameElement++)
            {
              TNameElement& ne=*iNameElement;
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
                CrsPaxInsQry.Execute();
                pax_id=CrsPaxInsQry.GetVariableAsInteger("pax_id");
                if (ne.indicator==CHG||ne.indicator==DEL)
                {
                  //удаляем все по пассажиру
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
                    "    DELETE FROM crs_pax_tkn WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_fqt WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_refuse WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax WHERE pax_id=curRow.inf_id; "
                    "  END LOOP; "
                    "  DELETE FROM crs_pax_rem WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doc WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doco WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_tkn WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_fqt WHERE pax_id=:pax_id; "
                    "END;";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.Execute();
                  
                  DeleteTlgSeatRanges(cltPNLCkin, pax_id, tid, point_ids_spp);
                  DeleteTlgSeatRanges(cltPNLBeforePay, pax_id, tid, point_ids_spp);
                  DeleteTlgSeatRanges(cltPNLAfterPay, pax_id, tid, point_ids_spp);
                  if (ne.indicator==DEL)
                  {
                    DeleteTlgSeatRanges(cltProtCkin, pax_id, tid, point_ids_spp);
                    DeleteTlgSeatRanges(cltProtBeforePay, pax_id, tid, point_ids_spp);
                    DeleteTlgSeatRanges(cltProtAfterPay, pax_id, tid, point_ids_spp);
                  };
                };
                if (ne.indicator!=DEL)
                {
                  //обработка младенцев
                  int inf_id;

                  CrsPaxInsQry.SetVariable("pers_type",EncodePerson(baby));
                  CrsPaxInsQry.SetVariable("seat_xname",FNull);
                  CrsPaxInsQry.SetVariable("seat_yname",FNull);
                  CrsPaxInsQry.SetVariable("seats",0);
                  CrsPaxInsQry.SetVariable("pr_del",0);
                  CrsPaxInsQry.SetVariable("last_op",info.time_create);
                  //младенцы пассажира
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
                    SaveDOCSRem(inf_id,iInfItem->doc);
                    SaveDOCORem(inf_id,iInfItem->doco);
                    SaveTKNRem(inf_id,iInfItem->tkn);
                  };

                  //ремарки пассажира
                  SavePNLADLRemarks(pax_id,iPaxItem->rem);
                  SaveDOCSRem(pax_id,iPaxItem->doc);
                  SaveDOCORem(pax_id,iPaxItem->doco);
                  SaveTKNRem(pax_id,iPaxItem->tkn);
                  SaveFQTRem(pax_id,iPaxItem->fqt);
                  //разметка слоев
                  InsertTlgSeatRanges(point_id,iTotals->dest,isPRL?cltPRLTrzt:cltPNLCkin,iPaxItem->seatRanges,
                                      pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  if (!isPRL)
                  {
                    TCompLayerType rem_layer=GetSeatRemLayer(pnr.market_flt.Empty()?con.flt.airline:
                                                                                    pnr.market_flt.airline,
                                                             iPaxItem->seat_rem);
                    if (rem_layer!=cltPNLCkin && rem_layer!=cltUnknown)
                      InsertTlgSeatRanges(point_id,iTotals->dest,rem_layer,
                                          vector<TSeatRange>(1,TSeatRange(iPaxItem->seat,iPaxItem->seat)),
                                          pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  };
                  UsePriorContext=true;
                  //ремарки, не привязанные к пассажиру
                  SavePNLADLRemarks(pax_id,ne.rem);
                  InsertTlgSeatRanges(point_id,iTotals->dest,isPRL?cltPRLTrzt:cltPNLCkin,ne.seatRanges,
                                      pax_id,tlg_id,NoExists,UsePriorContext,tid,point_ids_spp);
                  //восстановим номер места предварительной рассадки
                };

                if (!isPRL && !pr_sync_pnr)
                {
                  //делаем синхронизацию пассажира с розыском
                  rozysk::sync_crs_pax(pax_id, "");
                };
                
                if (isPRL && iPaxItem==ne.pax.begin())
                {
                  //запишем багаж
                  if (!ne.bag.Empty() || !ne.tags.empty()) SaveDCSBaggage(pax_id, ne);
                };
              };
            }; //for(iNameElement=pnr.ne.begin()

            //запишем стыковки
            if (/*ne.indicator==ADD && */!pnr.transfer.empty() /*||
                ne.indicator==CHG*/)
            {
              //удаляем нафиг все стыковки
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
            //запишем коммерческий рейс
            if (/*ne.indicator==ADD && */!pnr.market_flt.Empty() /*||
                ne.indicator==CHG*/)
            {
              //удаляем нафиг предыдущий
              Qry.Clear();
              Qry.SQLText=
                "BEGIN "
                "  DELETE FROM pnr_market_flt WHERE pnr_id= :pnr_id; "
                "  UPDATE crs_pnr SET tid=cycle_tid__seq.currval WHERE pnr_id= :pnr_id; "
                "END;";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();

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

            if (!isPRL && pr_sync_pnr)
            {
              //делаем синхронизацию всей группы с розыском
              rozysk::sync_crs_pnr(pnr_id, "");
            };
          };//for(iPnrItem=iTotals->pnr.begin()
        };
        check_layer_change(point_ids_spp);
      };

      if (!isPRL)
      {
        int pr_pnl_new=pr_pnl;
        if (strcmp(info.tlg_type,"PNL")==0)
        {
          //PNL
          if (pr_ne || !pr_numeric_pnl)
            //нецифровой
            pr_pnl_new=2;
          else
            //цифровой
            if (pr_pnl!=2) pr_pnl_new=1; //если до этого не было нецифрового PNL
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
      if (strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2) return false;//throw 0; //отложим разбор
    };
    return true;
  }
  catch (int)
  {
    OraSession.Rollback();  //отложим разбор
    return false;
  };
};

}

