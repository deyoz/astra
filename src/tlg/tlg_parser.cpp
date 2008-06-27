#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "tlg_parser.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "oralib.h"
#include "misc.h"
#include "tlg.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

const char lat_lines[]="ABCDEFGHIJK";
const char rus_lines[]="�����������";
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
              {"Address",
               "CommunicationsReference",
               "MessageIdentifier",
               "FlightElement",
               "AssociationNumber",
               "BonusPrograms",
               "Configuration",
               "ClassCodes",
               "SpaceAvailableElement",
               "TranzitElement",
               "TotalsByDestination",
               "TransferPassengerData",
               "EndOfMessage"};

char lexh[MAX_LEXEME_SIZE+1];

void ParseNameElement(char* p, vector<string> &names, TElemPresence num_presence);
void ParsePaxLevelElement(TTlgParser &tlg, TFltInfo& flt, TPnrItem &pnr, bool &pr_prev_rem);
void ParseRemarks(TTlgParser &tlg, TNameElement &ne);
bool ParseCHDRem(TTlgParser &tlg,string &rem_text,vector<TChdItem> &chd);
bool ParseINFRem(TTlgParser &tlg,string &rem_text,vector<TInfItem> &inf);
bool ParseDOCSRem(TTlgParser &tlg,string &rem_text,TDocItem &doc);
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
  //㤠��� �� �।�����騥 �஡���
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n'&&*(p+len)!='/';len++);
  //�������� ������ ���
  res=p+len;
  if (*res=='/') res++;
  else
    if (len==0) res=NULL;
  //㤠��� �� ��᫥���騥 �஡���
  for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  len++;
  //�������� tlg.lex
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

char* TTlgParser::GetNameElement(char* p, bool trimRight) //�� ��ࢮ� �窨 �� ��ப�
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++)
    if (*(unsigned char*)(p+len)<=' '&&*(p+len+1)=='.') break;
  if (trimRight)
    //㤠�塞 ����� ᨬ���� � ���� ���ᥬ�
    for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  else
    //㤠�塞 ᨬ���� � ���� ���ᥬ� �஬� �஡����
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

char GetSalonLine(char line)
{
  const char *p;
  p=strchr(rus_lines,line);
  if (p==NULL) p=strchr(lat_lines,line);
  else p=lat_lines+(p-rus_lines);
  if (p==NULL) return 0;
  else return *p;
};

enum TNsiType {ntCountries,ntAirlines,ntAirps,ntClass,ntSubcls,ntGenderTypes,ntPaxDocTypes};

char* GetNsiCode(char* value, TNsiType nsi, char* code)
{
  switch(nsi)
  {
    case ntCountries:
      try
      {
        TCountriesRow &row=(TCountriesRow&)(base_tables.get("countries").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      catch (EBaseTableError)
      {
        TCountriesRow &row=(TCountriesRow&)(base_tables.get("countries").get_row("code_iso",value));
        strcpy(code,row.code.c_str());
      };
      break;
    case ntAirlines:
      {
        TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      break;
    case ntAirps:
      {
        TAirpsRow &row=(TAirpsRow&)(base_tables.get("airps").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      break;
    case ntSubcls:
      {
        TSubclsRow &row=(TSubclsRow&)(base_tables.get("subcls").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      break;
    case ntClass:
      {
        TSubclsRow &row=(TSubclsRow&)(base_tables.get("subcls").get_row("code/code_lat",value));
        strcpy(code,row.cl.c_str());
      }
      break;
    case ntGenderTypes:
      {
        TGenderTypesRow &row=(TGenderTypesRow&)(base_tables.get("gender_types").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      break;
    case ntPaxDocTypes:
      {
        TPaxDocTypesRow &row=(TPaxDocTypesRow&)(base_tables.get("pax_doc_types").get_row("code/code_lat",value));
        strcpy(code,row.code.c_str());
      }
      break;
  };
  return code;
};

char* GetAirline(char* airline)
{
  return GetNsiCode(airline,ntAirlines,airline);
};

char* GetAirp(char* airp)
{
  return GetNsiCode(airp,ntAirps,airp);
};

TClass GetClass(char* subcl)
{
  char subclh[2];
  return DecodeClass(GetNsiCode(subcl,ntClass,subclh));
};

char* GetSubcl(char* subcl)
{
  return GetNsiCode(subcl,ntSubcls,subcl);
};

TTlgCategory GetTlgCategory(char *tlg_type)
{
  TTlgCategory cat;
  cat=tcUnknown;
  if (strcmp(tlg_type,"PNL")==0||
      strcmp(tlg_type,"ADL")==0||
      strcmp(tlg_type,"PTM")==0) cat=tcDCS;
  if (strcmp(tlg_type,"BTM")==0) cat=tcBSM;
  if (strcmp(tlg_type,"MVT")==0||
      strcmp(tlg_type,"LDM")==0) cat=tcAHM;
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

char ParseBSMElement(char *p, TTlgParser &tlg, TBSMInfo* &data)
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
          {
            data = new TBSMFltInfo;
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

            if ((p=tlg.GetSlashedLexeme(p))==NULL) throw ETlgError("Flight date not found");
            long int day;
            char month[4];
            c=0;
            res=sscanf(tlg.lex,"%2lu%3[A-Z�-��]%c",&day,month,&c);
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
              if ((int)today>(int)flt.scd+30)  //३� ����� ����ঠ���� �� �����
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
    if (data!=NULL) delete data;
    data=NULL;
    throw;
  };
};

TTlgPartInfo ParseBSMHeading(TTlgPartInfo heading, TBSMHeadingInfo &info)
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
      c=ParseBSMElement(line_p,tlg,data);
      try
      {
        if (c!='V') throw ("Version and supplementary data not found");
        TBSMVersionInfo &verInfo = *(dynamic_cast<TBSMVersionInfo*>(data));
        info.part_no=verInfo.part_no;
        strcpy(info.airp,verInfo.airp);
        info.reference_number=verInfo.message_number;
        if (data!=NULL) delete data;
      }
      catch(...)
      {
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
    //�뢥�� �訡��+����� ��ப�
    throw ETlgError("Line %d: %s",line,E.what());
  };
  next.p=line_p;
  next.line=line;
  return next;
};

void ParseBTMContent(TTlgPartInfo body, TBSMHeadingInfo& info, TBtmContent& con)
{
  vector<TBtmTransferInfo>::iterator iIn;
  vector<TBtmOutFltInfo>::iterator iOut;
  vector<TBtmGrpItem>::iterator iGrp;
  vector<TBtmTagItem>::iterator iTag;
  vector<string>::iterator i;

  con.Clear();

  int line;
  char e,prior,*line_p;
  TTlgParser tlg;

  TBSMInfo *data=NULL;

  try
  {
    line_p=body.p;
    line=body.line-1;

    char airp[4]; //��ய��� � ���஬ �ந�室�� �࠭���
    strcpy(airp,info.airp);
    GetAirp(airp);
    prior='V';

    do
    {
      line++;
      if (tlg.GetLexeme(line_p)==NULL) continue;

      e=ParseBSMElement(line_p,tlg,data);
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
              TBtmTagItem tag;
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
        if (data!=NULL) delete data;
        prior=e;
      }
      catch(...)
      {
        if (data!=NULL) delete data;
        throw;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch (ETlgError E)
  {
    throw ETlgError("Line %d: %s",line,E.what());
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

            TDateTime today=NowLocal();
            int year,mon,currday;
            DecodeDate(today,year,mon,currday);
            try
            {
              for(mon=1;mon<=12;mon++)
                if (strcmp(month,Months[mon-1].lat)==0||
                    strcmp(month,Months[mon-1].rus)==0) break;
              EncodeDate(year,mon,day,info.flt.scd);
              if ((int)today>(int)info.flt.scd+30)  //३� ����� ����ঠ���� �� �����
                EncodeDate(year+1,mon,day,info.flt.scd);
              info.flt.pr_utc=false;
            }
            catch(EConvertError)
            {
              throw ETlgError("Can't convert local date");
            };
            if (strcmp(info.tlg_type,"PNL")==0||
                strcmp(info.tlg_type,"ADL")==0)
            {
              if ((p=tlg.GetLexeme(p))!=NULL)
              {
                c=0;
                res=sscanf(tlg.lex,"%3[A-Z�-��]%c",info.flt.airp_dep,&c);
                if (c!=0||res!=1) throw ETlgError("Wrong boarding point");
              }
              else throw ETlgError("Wrong boarding point");
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
    //�뢥�� �訡��+����� ��ப�
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

void ParseAHMFltInfo(TTlgPartInfo body, TFltInfo& flt)
{
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
            //��ॢ���� day � TDateTime
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
              EncodeDate(year,mon,day,flt.scd);
              flt.pr_utc=true;
            }
            catch(EConvertError)
            {
              throw ETlgError("Can't convert UTC date");
            };
            return;
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch(ETlgError E)
  {
    //�뢥�� �訡��+����� ��ப�
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

//�����頥� TTlgPartInfo ᫥���饩 ��� (body)
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo* &info)
{
  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  TTlgElement e;
  TTlgPartInfo next;

  if (info!=NULL)
  {
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
              next=ParseDCSHeading(heading,*(TDCSHeadingInfo*)info);
              break;
            case tcBSM:
              info = new TBSMHeadingInfo(infoh);
              next=ParseBSMHeading(heading,*(TBSMHeadingInfo*)info);
              break;
            case tcAHM:
              info = new TAHMHeadingInfo(infoh);
              next=ParseAHMHeading(heading,*(TAHMHeadingInfo*)info);
              break;
            default:
              info = new THeadingInfo(infoh);
              next=heading;
              break;
          };
          return next;
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
    if (info==NULL) info = new THeadingInfo(infoh);
  }
  catch(ETlgError E)
  {
    if (info!=NULL) delete info;
    info=NULL;
    //�뢥�� �訡��+����� ��ப�
    throw ETlgError("Line %d: %s",line,E.what());
  }
  catch(...)
  {
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
    //�뢥�� �訡��+����� ��ப�
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
    //�뢥�� �訡��+����� ��ப�
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

//����� �⮡� ������﫨�� ����砫쭮
//info.tlg_type=HeadingInfo.tlg_type � info.part_no=HeadingInfo.part_no
void ParseEnding(TTlgPartInfo ending, THeadingInfo *headingInfo, TEndingInfo* &info)
{
  if (headingInfo==NULL) throw ETlgError("headingInfo not defined");
  if (info!=NULL)
  {
    delete info;
    info = NULL;
  };
  info = new TEndingInfo;
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
    delete info;
    throw;
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
  try
  {
    line_p=body.p;
    line=body.line-1;
    e=FlightElement;

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
              res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            else
              res=sscanf(tlg.lex,"%3[A-Z�-��0-9]%5lu%c%c",
                             flt.airline,&flt.flt_no,&(flt.suffix[0]),&c);
            if (c!=0||res<2||flt.flt_no<0) throw ETlgError("Wrong connecting flight");
            if (res==3&&
                !IsUpperLetter(flt.suffix[0])) throw ETlgError("Wrong connecting flight");
            GetAirline(flt.airline);
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
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
  }
  catch (ETlgError E)
  {
    throw ETlgError("%s, line %d: %s",GetTlgElementName(e),line,E.what());
  };
  return;
};

void ParsePNLADLContent(TTlgPartInfo body, TDCSHeadingInfo& info, TPnlAdlContent& con)
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
  try
  {
    line_p=body.p;
    line=body.line-1;
    e=FlightElement;

    strcpy(con.flt.airline,info.flt.airline);
    GetAirline(con.flt.airline);
    con.flt.flt_no=info.flt.flt_no;
    strcpy(con.flt.suffix,info.flt.suffix);
    con.flt.scd=info.flt.scd;
    con.flt.pr_utc=info.flt.pr_utc;
    strcpy(con.flt.airp_dep,info.flt.airp_dep);
    GetAirp(con.flt.airp_dep);

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
              res=sscanf(tlg.lex,"%3[A-Z�-��]/%s",RouteItem.station,tlg.lex);
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
            e=SpaceAvailableElement;
            e_part=1;
            break;
          };
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
                  if (strcmp(RouteItem.station,iRouteItem->station)==0)
                    throw ETlgError("Duplicate airport code '%s'",RouteItem.station);
                };
                if (!(con.avail.empty()&&RouteItem.station==con.flt.airp_dep))
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
                if (c!=0||res<1||SeatsItem.seats<0||minus!=0&&minus!='-')
                {
                  if (e_part==4) break;
                  throw ETlgError("Wrong number of seats");
                };

                //�஢�ਬ � RBD
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
              ParsePaxLevelElement(tlg,con.flt,*iPnrItem,pr_prev_rem);
            };

            e_part=3;
            break;
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
    //ࠧ����� �� ६�ન
    vector<TNameElement>::iterator i;
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
    {
      GetSubcl(iTotals->subcl); //��������� � ���� �࠭�� ���᪨�
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
        for(i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();i++)
        {
          ParseRemarks(tlg,*i);
          // ���⠢��� ��� EXST � STCR ॠ�쭮� ���-�� ����
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (iPaxItem!=i->pax.begin()&&
                (iPaxItem->name=="EXST"||iPaxItem->name=="STCR"))
            {
              iPaxItemPrev->seats++;
              //��ॡ�ᨬ ६�ન
              iPaxItemPrev->rem.insert(iPaxItemPrev->rem.end(),
                                       iPaxItem->rem.begin(),iPaxItem->rem.end());
              iPaxItem=i->pax.erase(iPaxItem);
            }
            else
            {
              iPaxItemPrev=iPaxItem;
              iPaxItem++;
            };
          };
        };
    };
  }
  catch (ETlgError E)
  {
    throw ETlgError("%s, line %d: %s",GetTlgElementName(e),line,E.what());
  };
  return;
};

void ParsePaxLevelElement(TTlgParser &tlg, TFltInfo& flt, TPnrItem &pnr, bool &pr_prev_rem)
{
  char c;
  int res;
  if (pnr.ne.empty()) return;
  TNameElement& ne=pnr.ne.back();

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
  for(int len=strlen(tlg.lex);len>=0&&*(unsigned char*)(tlg.lex+len)<=' ';len--)
    *(tlg.lex+len)=0;

  pr_prev_rem=false;
  if (strcmp(lexh,"C")==0)
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
    if (res<1||PnrAddr.addr[0]==0) throw ETlgError("Wrong PNR address");
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
  if (strcmp(lexh,"WL")==0)
  {
    if (strcmp(tlg.lex,".WL/")==0) strcpy(lexh,"ZZZZZZ");
    else
    {
      c=0;
      res=sscanf(tlg.lex,".WL/%6[A-Z�-��0-9]%c",lexh,&c);
      if (c!=0||res!=1) throw ETlgError("Wrong WL priority identification");
    };
    if (strcmp(pnr.wl_priority,"")!=0)
    {
      if (strcmp(pnr.wl_priority,lexh)!=0)
        throw ETlgError("Different WL priority identification in group found");
    }
    else strcpy(pnr.wl_priority,lexh);
    return;
  };
  if (lexh[0]=='I'||lexh[0]=='O')
  {
    TTransferItem Transfer;
    if (lexh[1]==0)
    {
      Transfer.num=1;
      if (lexh[0]=='I') Transfer.num=-Transfer.num;
      c=0;
      res=sscanf(tlg.lex,".%*c/%[A-Z�-��0-9]%c",lexh,&c);
    }
    else
    {
      c=0;
      res=sscanf(lexh+1,"%1lu%c",&Transfer.num,&c);
      if (res==0) return; //��㣮� �����
      if (c!=0||res!=1||Transfer.num<=0)
        throw ETlgError("Wrong connection element");
      if (lexh[0]=='I') Transfer.num=-Transfer.num;
      c=0;
      res=sscanf(tlg.lex,".%*c%*1[0-9]/%[A-Z�-��0-9]%c",lexh,&c);
    };
    if (c!=0||res!=1) throw ETlgError("Wrong connection element");
    c=0;
    if (isdigit(lexh[2]))
    {
      res=sscanf(lexh,"%2[A-Z�-��0-9]%5lu%1[A-Z�-��]%2lu%[A-Z�-��0-9]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%2[A-Z�-��0-9]%5lu%1[A-Z�-��]%1[A-Z�-��]%2lu%[A-Z�-��0-9]",
                        Transfer.airline,&Transfer.flt_no,
                        Transfer.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError("Wrong connection element");
      };
    }
    else
    {
      res=sscanf(lexh,"%3[A-Z�-��0-9]%5lu%1[A-Z�-��]%2lu%[A-Z�-��0-9]",
                      Transfer.airline,&Transfer.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%3[A-Z�-��0-9]%5lu%1[A-Z�-��]%1[A-Z�-��]%2lu%[A-Z�-��0-9]",
                        Transfer.airline,&Transfer.flt_no,
                        Transfer.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError("Wrong connection element");
      };
    };

    if (Transfer.flt_no<0||Transfer.local_date<=0||Transfer.local_date>31)
      throw ETlgError("Wrong connection element");

    Transfer.airp_dep[0]=0;
    Transfer.airp_arv[0]=0;
    lexh[0]=0;
    res=sscanf(tlg.lex,"%3[A-Z�-��]%3[A-Z�-��]%[A-Z�-��0-9]",
                       Transfer.airp_dep,Transfer.airp_arv,lexh);
    if (res<2||Transfer.airp_dep[0]==0||Transfer.airp_arv[0]==0)
    {
      Transfer.airp_dep[0]=0;
      lexh[0]=0;
      res=sscanf(tlg.lex,"%3[A-Z�-��]%[A-Z�-��0-9]",
                         Transfer.airp_dep,lexh);
      if (res<1||Transfer.airp_dep[0]==0) throw ETlgError("Wrong connection element");
    };
    if (lexh[0]!=0)
    {
      c=0;
      res=sscanf(lexh,"%4lu%2[A-Z�-��]%c",&Transfer.local_time,tlg.lex,&c);
      if (c!=0||res!=2)
      {
        c=0;
        res=sscanf(lexh,"%4lu%c",&Transfer.local_time,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong connection element");
      };
    };
    if (Transfer.num>0&&Transfer.airp_arv[0]==0)
    {
      strcpy(Transfer.airp_arv,Transfer.airp_dep);
      Transfer.airp_dep[0]=0;
    };
    //������ �� ����७��
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

void ParseRemarks(TTlgParser &tlg, TNameElement &ne)
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
  vector<TPaxItem>::iterator iPaxItem,iPaxItem2;
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
  {
    TrimString(iRemItem->text);
    if (iRemItem->text.empty()) continue;
    p=tlg.GetWord((char*)iRemItem->text.c_str());
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
      iRemItem++;
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
        ParseNameElement((char*)strh.c_str(),names,epOptional);
        if (!names.empty()&&*(names.begin())==ne.surname) break; //��諨
      }
      catch(ETlgError &E) {};

      if (pos!=0)
        pos=iRemItem->text.find_last_of('-',pos-1);
      else
        pos=string::npos;
    };

    if (pos==string::npos)
    {
      //�� ��諨 ��뫪� �� ���ᠦ��
      iRemItem++;
      continue;
    };

    pr_parse=true;
    for(vector<string>::iterator i=names.begin();i!=names.end();i++)
    {
      if (i!=names.begin())
      {
        for(k=0;k<=1;k++)
        {
          for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
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

    if (pr_parse) iRemItem->text.erase(pos); //���� ����砭�� ६�ન ��᫥ -

    for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
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
      iRemItem++;
  };

  //᭠砫� �஡����� �� ६�ઠ� ������� �� ���ᠦ�஢ (��-�� ����஢ ����)
  for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
  {
    for(k=0;k<=1;k++)
    {
      for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();iRemItem++)
      {
        if (iRemItem->text.empty()) continue;

        if (k==0)
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
              for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();i++)
                if (i->surname.empty()) i->surname=ne.surname;
              iPaxItem->inf.insert(iPaxItem->inf.end(),inf.begin(),inf.end());
            };
            continue;
          };
        }
        else
        {
          if (strcmp(iRemItem->code,"DOCS")==0)
          {
            TDocItem doc;
            if (ParseDOCSRem(tlg,iRemItem->text,doc))
            {
              //�஢�ਬ ���㬥�� ��
              try
              {
                TGenderTypesRow &row=(TGenderTypesRow&)(base_tables.get("gender_types").get_row("code/code_lat",doc.gender));
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

          if (strcmp(iRemItem->code,"TKNA")==0||
              strcmp(iRemItem->code,"TKNE")==0)
          {
            TTKNItem tkn;
            if (ParseTKNRem(tlg,iRemItem->text,tkn))
            {
              //�஢�ਬ ����� ��
              if (tkn.pr_inf)
              {
                if (iPaxItem->inf.size()==1)
                {
                  //⮫쪮 �᫨ � ���ᠦ�� ���� infant - ����� �� ����� �⭮���� � ����
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

          if (strcmp(iRemItem->code,"EXST")==0||
              strcmp(iRemItem->code,"GPST")==0||
              strcmp(iRemItem->code,"RQST")==0||
              strcmp(iRemItem->code,"SEAT")==0||
              strcmp(iRemItem->code,"NSST")==0||
              strcmp(iRemItem->code,"NSSA")==0||
              strcmp(iRemItem->code,"NSSB")==0||
              strcmp(iRemItem->code,"NSSW")==0||
              strcmp(iRemItem->code,"SMST")==0||
              strcmp(iRemItem->code,"SMSA")==0||
              strcmp(iRemItem->code,"SMSB")==0||
              strcmp(iRemItem->code,"SMSW")==0)
          {
            vector<TSeatRange> seats;
            if (ParseSEATRem(tlg,iRemItem->text,seats))
            {
              for(vector<TSeatRange>::iterator i=seats.begin();i!=seats.end();i++)
              {
                if (i->first.line!=0 && i->first.row>0)
                {
                  //������� ���� �� seat, �᫨ �� ������� �� � ���� ��㣮��
                   for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                     if (i->first.row==iPaxItem2->seat.row&&
                         i->first.line==iPaxItem2->seat.line) break;

                   if (iPaxItem2==ne.pax.end())
                   {
                     iPaxItem->seat=i->first;
                     break;
                   };
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
            TFQTItem fqt;
            if (ParseFQTRem(tlg,iRemItem->text,fqt))
              iPaxItem->fqt.push_back(fqt);
            continue;
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
  for(k=0;k<=1;k++)
  {
    //�஡����� �� ६�ઠ� ��㯯�
    for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
    {
      if (iRemItem->text.empty()) continue;

      if (k==0)
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
            for(vector<TInfItem>::iterator i=inf.begin();i!=inf.end();i++)
              if (i->surname.empty()) i->surname=ne.surname;

            if (ne.pax.size()==1 && ne.pax.begin()->inf.empty() &&
                inf.size()==1)
              ne.pax.begin()->inf.insert(ne.pax.begin()->inf.end(),inf.begin(),inf.end());
            else
              infs.insert(infs.end(),inf.begin(),inf.end());
          };
          continue;
        };
      }
      else
      {
        if (strcmp(iRemItem->code,"EXST")==0||
            strcmp(iRemItem->code,"GPST")==0||
            strcmp(iRemItem->code,"RQST")==0||
            strcmp(iRemItem->code,"SEAT")==0||
            strcmp(iRemItem->code,"NSST")==0||
            strcmp(iRemItem->code,"NSSA")==0||
            strcmp(iRemItem->code,"NSSB")==0||
            strcmp(iRemItem->code,"NSSW")==0||
            strcmp(iRemItem->code,"SMST")==0||
            strcmp(iRemItem->code,"SMSA")==0||
            strcmp(iRemItem->code,"SMSB")==0||
            strcmp(iRemItem->code,"SMSW")==0)
        {
          vector<TSeatRange> seats;
          if (ParseSEATRem(tlg,iRemItem->text,seats))
          {
            for(vector<TSeatRange>::iterator i=seats.begin();i!=seats.end();i++)
            {
              if (i->first.line!=0 && i->first.row>0)
              {
                //��� �� ���᪠�� �।� �ᥣ� pnr


                //������� ���� �� seat, �᫨ �� ������� �� � ���� ��㣮��
                 for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                   if (i->first.row==iPaxItem2->seat.row&&
                       i->first.line==iPaxItem2->seat.line) break;

                 if (iPaxItem2==ne.pax.end())
                   //���� ���ᠦ��, ���஬� �� ���⠢���� ����
                   if (strcmp(iRemItem->code,"EXST")==0)
                   {
                     for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                       if (iPaxItem2->seat.line==0&&
                           iPaxItem2->name=="EXST")
                       {
                         iPaxItem->seat=i->first;
                         break;
                       };
                   };
                 if (iPaxItem2==ne.pax.end())
                   for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                     if (iPaxItem2->seat.line==0)
                     {
                       iPaxItem2->seat=i->first;
                       break;
                     };
              };
            };

          };
          continue;
        };

        if (strcmp(iRemItem->code,"DOCS")==0)
        {
          if (ne.pax.size()==1)
          {
            TDocItem doc;
            if (ParseDOCSRem(tlg,iRemItem->text,doc))
            {
              //�஢�ਬ ���㬥�� ��
              try
              {
                TGenderTypesRow &row=(TGenderTypesRow&)(base_tables.get("gender_types").get_row("code/code_lat",doc.gender));
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
                  //��������� ⨯ ���ᠦ��
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
              //�஢�ਬ ����� ��
              if (tkn.pr_inf)
              {
                if (ne.pax.begin()->inf.size()==1)
                {
                  //⮫쪮 �᫨ � ���ᠦ�� ���� infant - ����� �� ����� �⭮���� � ����
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
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(rem_code,"EXST")==0||
      strcmp(rem_code,"GPST")==0||
      strcmp(rem_code,"RQST")==0||
      strcmp(rem_code,"SEAT")==0||
      strcmp(rem_code,"NSST")==0||
      strcmp(rem_code,"NSSA")==0||
      strcmp(rem_code,"NSSB")==0||
      strcmp(rem_code,"NSSW")==0||
      strcmp(rem_code,"SMST")==0||
      strcmp(rem_code,"SMSA")==0||
      strcmp(rem_code,"SMSB")==0||
      strcmp(rem_code,"SMSW")==0)
  {
    while((p=tlg.GetLexeme(p))!=NULL)
    {

      //�஢�ਬ ��������
      c=0;
      res=sscanf(tlg.lex,"%3[0-9]%c-%3[0-9]%c%c",
                 row.first,&line.first,
                 row.second,&line.second,
                 &c);
      if (c==0&&res==4)
      {
        TSeatRange seatr;
        if (StrToInt(row.first,seatr.first.row)==EOF ||
            StrToInt(row.second,seatr.second.row)==EOF ||
            (seatr.first.line=GetSalonLine(line.first))==0 ||
            (seatr.second.line=GetSalonLine(line.second))==0) continue;

        strcpy(seatr.first.rem,rem_code);
        strcpy(seatr.second.rem,rem_code);
        seats.push_back(seatr);
        continue;
      };

      //�஢�ਬ ROW
      c=0;
      *lexh=0;
      res=sscanf(tlg.lex,"%3[0-9]%[ROW]%c",row.first,lexh,&c);
      if (c==0&&res==2&&strcmp(lexh,"ROW")==0)
      {
        TSeat seat;
        strcpy(seat.rem,rem_code);
        if (StrToInt(row.first,seat.row)==EOF) continue;

        TSeatRange seatr(seat,seat);
        seats.push_back(seatr);
        continue;
      };

      //�஢�ਬ ����᫥���
      bool pr_parse=true;
      vector<TSeat> seatsh;
      do
      {
        TSeat seat;
        strcpy(seat.rem,rem_code);
        *row.first=0;
        *lexh=0;
        res=sscanf(tlg.lex,"%3[0-9]%[A-Z�-��]%s",row.first,lexh,tlg.lex);
        if (res<2||*row.first==0||*lexh==0||
            StrToInt(row.first,seat.row)==EOF)
        {
          pr_parse=false;
          break;
        };
        char *pline=lexh;
        for(;*pline!=0;pline++)
        {
          if ((seat.line=GetSalonLine(*pline))==0) break;
          seatsh.push_back(seat);
        };
        if (*pline!=0)
        {
          pr_parse=false;
          break;
        };
      }
      while(res==3);
      if (pr_parse)
      {
        for(vector<TSeat>::iterator i=seatsh.begin();i!=seatsh.end();i++)
        {
          TSeatRange seatr(*i,*i);
          seats.push_back(seatr);
        };
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

  char *p=(char*)rem_text.c_str();

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

bool ParseDOCSRem(TTlgParser &tlg,string &rem_text,TDocItem &doc)
{
  char c;
  int res,k;

  char *p=(char*)rem_text.c_str();

  doc.Clear();

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",doc.rem_code,&c);
  if (c!=0||res!=1) return false;

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
            GetNsiCode(doc.type,ntPaxDocTypes,doc.type);
            break;
          case 2:
          case 4:
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",lexh,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            if (k==2)
              GetNsiCode(lexh,ntCountries,doc.issue_country);
            else
              GetNsiCode(lexh,ntCountries,doc.nationality);
            break;
          case 3:
            res=sscanf(tlg.lex,"%15[A-Z�-��0-9 ]%c",doc.no,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            break;
          case 5:
            if (StrToDateTime(tlg.lex,"ddmmmyy",doc.birth_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",doc.birth_date,false)==EOF)
              throw ETlgError("Wrong format");
            break;
          case 6:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.gender,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            GetNsiCode(doc.gender,ntGenderTypes,doc.gender);
            break;
          case 7:
            if (StrToDateTime(tlg.lex,"ddmmmyy",doc.expiry_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",doc.expiry_date,false)==EOF)
              throw ETlgError("Wrong format");
            break;
          case 8:
            doc.surname=tlg.lex;
            break;
          case 9:
            doc.first_name=tlg.lex;
            break;
          case 10:
            doc.second_name=tlg.lex;
            break;
          case 11:
            res=sscanf(tlg.lex,"%1[H]%c",lexh,&c);
            if (c!=0||res!=2||strcmp(lexh,"H")!=0) throw ETlgError("Wrong format");
            doc.pr_multi=true;
            break;
        }
      }
      catch(logic_error &E)
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
            doc.expiry_date=NoExists;;
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
            GetNsiCode(lexh,ntCountries,doc.issue_country);
            break;
          case 3:
            if (StrToDateTime(tlg.lex,"ddmmmyy",doc.birth_date,true)==EOF &&
                StrToDateTime(tlg.lex,"ddmmmyy",doc.birth_date,false)==EOF)
              throw ETlgError("Wrong format");
            break;
          case 4:
            doc.surname=tlg.lex;
            break;
          case 5:
            doc.first_name=tlg.lex;
            break;
          case 6:
            res=sscanf(tlg.lex,"%2[A-Z]%c",doc.gender,&c);
            if (c!=0||res!=1) throw ETlgError("Wrong format");
            GetNsiCode(doc.gender,ntGenderTypes,doc.gender);
            break;
          case 7:
            res=sscanf(tlg.lex,"%1[H]%c",lexh,&c);
            if (c!=0||res!=2||strcmp(lexh,"H")!=0) throw ETlgError("Wrong format");
            doc.pr_multi=true;
            break;
        }
      }
      catch(logic_error &E)
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
      catch(logic_error &E)
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
            fqt.extra=Trim(p);
            break;
        };
      }
      catch(logic_error &E)
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

TTlgParts GetParts(char* tlg_p)
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
          parts.body=ParseHeading(parts.heading,HeadingInfo);  //����� ������ NULL
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

    if (HeadingInfo!=NULL) delete HeadingInfo;
  }
  catch(...)
  {
    if (HeadingInfo!=NULL) delete HeadingInfo;
    throw;
  };
  return parts;
};

void bind_tlg(int point_id_tlg, int point_id_spp)
{
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "INSERT INTO tlg_binding(point_id_tlg,point_id_spp) VALUES(:point_id_tlg,:point_id_spp)";
  BindQry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  BindQry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  try
  {
    BindQry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
  };
};

bool bind_tlg(int point_id, TFltInfo &flt, TBindType bind_type)
{
  bool res=false;
  if (*flt.airp_dep==0) return res;
  TQuery TripsQry(&OraSession);
  TripsQry.CreateVariable("airline",otString,flt.airline);
  TripsQry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
  TripsQry.CreateVariable("suffix",otString,flt.suffix);
  TripsQry.CreateVariable("airp_dep",otString,flt.airp_dep);
  TripsQry.CreateVariable("scd",otDate,flt.scd);

  if (!flt.pr_utc)
  {
    //��ॢ�� UTCToLocal(points.scd)
    TripsQry.SQLText=
      "SELECT point_id,scd_out AS scd,airp, "
      "       point_num,DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
      "FROM points "
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND "
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
      "      scd_out >= TO_DATE(:scd)-1 AND scd_out < TO_DATE(:scd)+2 AND "
      "      pr_del>=0 ";
  }
  else
  {
    TripsQry.SQLText=
      "SELECT point_id, "
      "       point_num,DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
      "FROM points "
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND "
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
      "      scd_out >= TO_DATE(:scd) AND scd_out < TO_DATE(:scd)+1 AND pr_del>=0 ";
  };
  TripsQry.Execute();
  TDateTime scd;
  string tz_region;
  for(;!TripsQry.Eof;TripsQry.Next())
  {
    if (!flt.pr_utc)
    {
      scd=TripsQry.FieldAsDateTime("scd");
      tz_region=AirpTZRegion(TripsQry.FieldAsString("airp"),false);
      if (tz_region.empty()) continue;
      scd=UTCToLocal(scd,tz_region);
      modf(scd,&scd);
      if (scd!=flt.scd) continue;
    };
    switch (bind_type)
    {
      case btFirstSeg:
        bind_tlg(point_id,TripsQry.FieldAsInteger("point_id"));
        res=true;
        break;
      case btLastSeg:
      case btAllSeg:
        {
          TQuery SegQry(&OraSession);
          SegQry.Clear();
          SegQry.CreateVariable("first_point",otInteger,TripsQry.FieldAsInteger("first_point"));
          SegQry.CreateVariable("point_num",otInteger,TripsQry.FieldAsInteger("point_num"));
          if (!(*flt.airp_arv==0))
          {
            //�饬 point_num ��ࢮ�� �����襣��� � ������� airp_arv
            SegQry.SQLText=
              "SELECT MIN(point_num) AS last_point_num FROM points "
              "WHERE first_point=:first_point AND point_num>:point_num AND pr_del>=0 AND "
              "      airp=:airp_arv ";
            SegQry.CreateVariable("airp_arv",otString,flt.airp_arv);
          }
          else
          {
            //�饬 point_num ��᫥����� �㭪� � �������
            SegQry.SQLText=
              "SELECT MAX(point_num) AS last_point_num FROM points "
              "WHERE first_point=:first_point AND point_num>:point_num AND pr_del>=0 ";
          };
          SegQry.Execute();
          if (SegQry.Eof||SegQry.FieldIsNULL("last_point_num"))
          {
            //�᫨ ��祣� �� ��諨
            if (bind_type==btAllSeg)
            {
              bind_tlg(point_id,TripsQry.FieldAsInteger("point_id"));
              res=true;
            };
            break;
          };

          int last_point_num = SegQry.FieldAsInteger("last_point_num");
          SegQry.Clear();
          SegQry.SQLText=
            "SELECT point_id FROM points "
            "WHERE DECODE(pr_tranzit,0,point_id,first_point)=:first_point AND "
            "      point_num>=:point_num AND "
            "      point_num<:last_point_num AND pr_del>=0 "
            "ORDER BY point_num DESC";
          SegQry.CreateVariable("first_point",otInteger,TripsQry.FieldAsInteger("first_point"));
          SegQry.CreateVariable("point_num",otInteger,TripsQry.FieldAsInteger("point_num"));
          SegQry.CreateVariable("last_point_num",otInteger,last_point_num);
          SegQry.Execute();
          //�஡������ �� �㭪⠬ ��ᠤ�� � ���⭮� ���浪� ��稭�� �
          //�㭪� ��। last_point_num
          for(;!SegQry.Eof;SegQry.Next())
          {
            bind_tlg(point_id,SegQry.FieldAsInteger("point_id"));
            res=true;
            //�᫨ btLastSeg - �ਢ殮� ⮫쪮 ��᫥���� ᥣ����
            if (bind_type==btLastSeg) break;
          };
        };
    };
  };
  return res;
};

bool bind_tlg(TQuery &Qry)
{
  if (Qry.Eof) return false;
  int point_id=Qry.FieldAsInteger("point_id");
  TFltInfo flt;
  strcpy(flt.airline,Qry.FieldAsString("airline"));
  flt.flt_no=Qry.FieldAsInteger("flt_no");
  strcpy(flt.suffix,Qry.FieldAsString("suffix"));
  flt.scd=Qry.FieldAsDateTime("scd");
  modf(flt.scd,&flt.scd);
  flt.pr_utc=Qry.FieldAsInteger("pr_utc")!=0;
  strcpy(flt.airp_dep,Qry.FieldAsString("airp_dep"));
  strcpy(flt.airp_arv,Qry.FieldAsString("airp_arv"));
  TBindType bind_type;
  switch (Qry.FieldAsInteger("bind_type"))
  {
    case 0: bind_type=btFirstSeg;
            break;
    case 1: bind_type=btLastSeg;
            break;
   default: bind_type=btAllSeg;
  };
  if (bind_tlg(point_id,flt,bind_type)) return true;
  //�� ᪫����� �ਢ易�� � ३�� �� ����� - �ਢ殮� �१ ⠡���� CRS_CODE_SHARE
  bool res=false;
  TQuery CodeShareQry(&OraSession);
  CodeShareQry.Clear();
  CodeShareQry.SQLText=
    "SELECT airline,flt_no FROM crs_code_share "
    "WHERE airline_crs=:airline AND "
    "      (flt_no_crs=:flt_no OR flt_no_crs IS NULL AND :flt_no IS NULL) "
    "ORDER BY flt_no_crs,airline,flt_no";
  CodeShareQry.CreateVariable("airline",otString,flt.airline);
  CodeShareQry.DeclareVariable("flt_no",otInteger);
  for(int i=0;i<2;i++)
  {
    if (i==0)
      //᭠砫� �஢�ਬ �� �/� � ������ ३�
      CodeShareQry.SetVariable("flt_no",(int)flt.flt_no);
    else
      //��⮬ �஢�ਬ ⮫쪮 �� �/�
      CodeShareQry.SetVariable("flt_no",FNull);
    CodeShareQry.Execute();
    if (CodeShareQry.Eof) continue;
    for(;!CodeShareQry.Eof;CodeShareQry.Next())
    {
      strcpy(flt.airline,CodeShareQry.FieldAsString("airline"));
      if (!CodeShareQry.FieldIsNULL("flt_no"))
        flt.flt_no=CodeShareQry.FieldAsInteger("flt_no");
      if (bind_tlg(point_id,flt,bind_type)) res=true;
    };
    break;
  };
  return res;
};

bool bind_tlg(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type "
    "FROM tlg_trips "
    "WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  return bind_tlg(Qry);
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
    bind_tlg(point_id);

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

bool DeletePTMBTMContent(int point_id_in, THeadingInfo& info)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT MAX(time_create) AS max_time_create "
    "FROM tlgs_in,tlg_source "
    "WHERE tlg_source.tlg_id=tlgs_in.id AND "
    "      tlg_source.point_id_tlg=:point_id_in AND tlgs_in.type=:tlg_type";
  Qry.CreateVariable("point_id_in",otInteger,point_id_in);
  Qry.CreateVariable("tlg_type",otString,info.tlg_type);
  Qry.Execute();
  if (!Qry.Eof && !Qry.FieldIsNULL("max_time_create"))
  {
    if (Qry.FieldAsDateTime("max_time_create") > info.time_create) return false;
    TQuery TrferQry(&OraSession);
    TrferQry.SQLText=
      "SELECT tlg_transfer.trfer_id "
      "FROM tlgs_in,tlg_transfer "
      "WHERE tlgs_in.id=tlg_transfer.tlg_id AND tlgs_in.type=:tlg_type AND "
      "      tlg_transfer.point_id_in=:point_id_in ";
    TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
    TrferQry.CreateVariable("tlg_type",otString,info.tlg_type);
    TrferQry.Execute();

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
  };
  return true;
};

void SaveBTMContent(int tlg_id, TBSMHeadingInfo& info, TBtmContent& con)
{
  vector<TBtmTransferInfo>::iterator iIn;
  vector<TBtmOutFltInfo>::iterator iOut;
  vector<TBtmGrpItem>::iterator iGrp;
  vector<TBtmPaxItem>::iterator iPax;
  vector<TBtmTagItem>::iterator iTag;
  vector<string>::iterator i;
  int point_id_in,point_id_out;

  TQuery TrferQry(&OraSession);
  TrferQry.SQLText=
    "INSERT INTO tlg_transfer(trfer_id,point_id_in,subcl_in,point_id_out,subcl_out,tlg_id) "
    "VALUES(id__seq.nextval,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.DeclareVariable("point_id_in",otInteger);
  TrferQry.DeclareVariable("subcl_in",otString);
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  TQuery GrpQry(&OraSession);
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(pax_grp__seq.nextval,id__seq.currval,NULL,:bag_amount,:bag_weight,:rk_weight,:weight_unit) ";
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
    "VALUES(id__seq.nextval,:point_id_in,:subcl_in,:point_id_out,:subcl_out,:tlg_id)";
  TrferQry.CreateVariable("point_id_in",otInteger,point_id_in);
  TrferQry.CreateVariable("subcl_in",otString,FNull); //�������� �ਫ�� �������⥭
  TrferQry.DeclareVariable("point_id_out",otInteger);
  TrferQry.DeclareVariable("subcl_out",otString);
  TrferQry.CreateVariable("tlg_id",otInteger,tlg_id);

  TQuery GrpQry(&OraSession);
  GrpQry.SQLText=
    "INSERT INTO trfer_grp(grp_id,trfer_id,seats,bag_amount,bag_weight,rk_weight,weight_unit) "
    "VALUES(pax_grp__seq.nextval,id__seq.currval,:seats,:bag_amount,:bag_weight,NULL,:weight_unit) ";
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

void SaveDOCSRem(int pax_id, vector<TDocItem> &doc)
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
  for(vector<TDocItem>::iterator i=doc.begin();i!=doc.end();i++)
  {
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
    if (i->surname.size()>64) i->surname.erase(64);
    Qry.SetVariable("surname",i->surname);
    if (i->first_name.size()>64) i->first_name.erase(64);
    Qry.SetVariable("first_name",i->first_name);
    if (i->second_name.size()>64) i->second_name.erase(64);
    Qry.SetVariable("second_name",i->second_name);
    Qry.SetVariable("pr_multi",(int)i->pr_multi);
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

void crs_recount(int point_id_tlg)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "DECLARE "
    "  CURSOR cur IS "
    "    SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id; "
    "curRow cur%ROWTYPE; "
    "BEGIN "
    "  FOR curRow IN cur LOOP "
    "    ckin.crs_recount(curRow.point_id_spp); "
    "  END LOOP; "
    "END;";
  Qry.CreateVariable("point_id",otInteger,point_id_tlg);
  Qry.Execute();
};

bool SavePNLADLContent(int tlg_id, TDCSHeadingInfo& info, TPnlAdlContent& con, bool forcibly)
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

  char crs[sizeof(info.sender)];
  int tid;

  TQuery Qry(&OraSession);

  if (info.time_create==0) throw ETlgError("Creation time not defined");

  int point_id=SaveFlt(tlg_id,con.flt,btFirstSeg);

  //��������㥬 ��⥬� �஭�஢����
  strcpy(crs,info.sender);
  Qry.Clear();
  Qry.SQLText="INSERT INTO crs2(code,name) VALUES(:code,:code)";
  Qry.CreateVariable("code",otString,crs);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError &E)
  {
    if (E.Code!=1) throw;
  };

  bool pr_numeric_pnl=true;
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_numeric_pnl FROM crs_set "
    "WHERE crs=:crs AND airline=:airline AND "
    "      (flt_no=:flt_no OR flt_no IS NULL) AND "
    "      (airp_dep=:airp_dep OR airp_dep IS NULL) "
    "ORDER BY flt_no,airp_dep";
  Qry.CreateVariable("crs",otString,crs);
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
      "VALUES(id__seq.nextval,:airline,:flt_no,:airp_dep,:crs,0,1)";
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

  TDateTime last_resa=0,last_tranzit=0,last_cfg=0;
  int pr_pnl=0;

  Qry.Clear();
  Qry.SQLText=
    "SELECT last_resa,last_tranzit,last_cfg,pr_pnl FROM crs_data_stat \
     WHERE point_id=:point_id AND crs=:crs FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("crs",otString,crs);
  Qry.Execute();
  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("last_resa"))
      last_resa=Qry.FieldAsDateTime("last_resa");
    if (!Qry.FieldIsNULL("last_tranzit"))
      last_tranzit=Qry.FieldAsDateTime("last_tranzit");
    if (!Qry.FieldIsNULL("last_cfg"))
      last_cfg=Qry.FieldAsDateTime("last_cfg");
    pr_pnl=Qry.FieldAsInteger("pr_pnl");
  };

  //pr_pnl=0 - �� ��襫 ��஢�� PNL
  //pr_pnl=1 - ��襫 ��஢�� PNL
  //pr_pnl=2 - ��襫 ����஢�� PNL

  bool pr_save_ne=!(strcmp(info.tlg_type,"PNL")==0&&pr_pnl==2|| //��襫 ��ன ����஢�� PNL
                    strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2); //��襫 ADL �� ����� PNL

  bool pr_recount=false;
  //������� ��஢� �����
  if (!con.resa.empty()&&(last_resa==0||last_resa<=info.time_create))
  {
    pr_recount=true;
    Qry.Clear();
    Qry.SQLText=
      "BEGIN \
         UPDATE crs_data SET resa=NULL,pad=NULL WHERE point_id=:point_id AND crs=:crs; \
         UPDATE crs_data_stat SET last_resa=:time_create WHERE point_id=:point_id AND crs=:crs; \
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data_stat(point_id,crs,last_resa) \
           VALUES(:point_id,:crs,:time_create); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("crs",otString,crs);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         UPDATE crs_data SET resa=NVL(resa+:resa,:resa), pad=NVL(pad+:pad,:pad)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data(point_id,target,class,crs,resa,pad) \
           VALUES(:point_id,:target,:class,:crs,:resa,:pad); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("class",otString);
    Qry.CreateVariable("crs",otString,crs);
    Qry.DeclareVariable("resa",otInteger);
    Qry.DeclareVariable("pad",otInteger);
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
    {
      Qry.SetVariable("target",iTotals->dest);
      Qry.SetVariable("class",EncodeClass(iTotals->cl));
      Qry.SetVariable("resa",(int)iTotals->seats);
      Qry.SetVariable("pad",(int)iTotals->pad);
      Qry.Execute();
    };
  };

  if (!con.transit.empty()&&(last_tranzit==0||last_tranzit<=info.time_create))
  {
    pr_recount=true;
    Qry.Clear();
    Qry.SQLText=
      "BEGIN \
         UPDATE crs_data SET tranzit=NULL WHERE point_id=:point_id AND crs=:crs; \
         UPDATE crs_data_stat SET last_tranzit=:time_create WHERE point_id=:point_id AND crs=:crs; \
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data_stat(point_id,crs,last_tranzit) \
           VALUES(:point_id,:crs,:time_create); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("crs",otString,crs);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         UPDATE crs_data SET tranzit=NVL(tranzit+:tranzit,:tranzit)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data(point_id,target,class,crs,tranzit) \
           VALUES(:point_id,:target,:class,:crs,:tranzit); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("class",otString);
    Qry.CreateVariable("crs",otString,crs);
    Qry.DeclareVariable("tranzit",otInteger);
    for(iRouteItem=con.transit.begin();iRouteItem!=con.transit.end();iRouteItem++)
    {
      Qry.SetVariable("target",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("tranzit",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };
  };

  if (!con.cfg.empty()&&!con.avail.empty()&&(last_cfg==0||last_cfg<=info.time_create))
  {
    pr_recount=true;
    Qry.Clear();
    Qry.SQLText=
      "BEGIN \
         UPDATE crs_data SET cfg=NULL,avail=NULL WHERE point_id=:point_id AND crs=:crs; \
         UPDATE crs_data_stat SET last_cfg=:time_create WHERE point_id=:point_id AND crs=:crs; \
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data_stat(point_id,crs,last_cfg) \
           VALUES(:point_id,:crs,:time_create); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("crs",otString,crs);
    Qry.CreateVariable("time_create",otDate,info.time_create);
    Qry.Execute();

    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         UPDATE crs_data SET cfg=NVL(cfg+:cfg,:cfg)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data(point_id,target,class,crs,cfg) \
           VALUES(:point_id,:target,:class,:crs,:cfg); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("class",otString);
    Qry.CreateVariable("crs",otString,crs);
    Qry.DeclareVariable("cfg",otInteger);
    for(iRouteItem=con.cfg.begin();iRouteItem!=con.cfg.end();iRouteItem++)
    {
      Qry.SetVariable("target",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("cfg",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };

    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         UPDATE crs_data SET avail=NVL(avail+:avail,:avail)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data(point_id,target,class,crs,avail) \
           VALUES(:point_id,:target,:class,:crs,:avail); \
         END IF; \
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("class",otString);
    Qry.CreateVariable("crs",otString,crs);
    Qry.DeclareVariable("avail",otInteger);
    for(iRouteItem=con.avail.begin();iRouteItem!=con.avail.end();iRouteItem++)
    {
      Qry.SetVariable("target",iRouteItem->station);
      for(iSeatsItem=iRouteItem->seats.begin();iSeatsItem!=iRouteItem->seats.end();iSeatsItem++)
      {
        Qry.SetVariable("class",EncodeClass(iSeatsItem->cl));
        Qry.SetVariable("avail",(int)iSeatsItem->seats);
        Qry.Execute();
      };
    };
  };
  if (pr_recount) crs_recount(point_id);

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
        Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
        Qry.Execute();
        tid=Qry.FieldAsInteger("tid");

        TQuery CrsPnrQry(&OraSession);
        CrsPnrQry.Clear();
        CrsPnrQry.SQLText=
          "SELECT crs_pnr.pnr_id FROM pnr_addrs,crs_pnr "
          "WHERE crs_pnr.pnr_id=pnr_addrs.pnr_id AND "
          "      point_id= :point_id AND crs= :crs AND "
          "      target= :target AND subclass= :subclass AND "
          "      pnr_addrs.airline=:pnr_airline AND pnr_addrs.addr=:pnr_addr";
        CrsPnrQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrQry.CreateVariable("crs",otString,crs);
        CrsPnrQry.DeclareVariable("target",otString);
        CrsPnrQry.DeclareVariable("subclass",otString);
        CrsPnrQry.DeclareVariable("pnr_airline",otString);
        CrsPnrQry.DeclareVariable("pnr_addr",otString);

        TQuery CrsPnrInsQry(&OraSession);
        CrsPnrInsQry.Clear();
        CrsPnrInsQry.SQLText=
          "BEGIN \
             IF :pnr_id IS NULL THEN \
               SELECT crs_pnr__seq.nextval INTO :pnr_id FROM dual; \
               INSERT INTO crs_pnr(pnr_id,point_id,target,subclass,class,grp_name,wl_priority,crs,tid) \
               VALUES(:pnr_id,:point_id,:target,:subclass,:class,:grp_name,:wl_priority,:crs,tid__seq.currval); \
             ELSE \
               UPDATE crs_pnr SET grp_name=NVL(:grp_name,grp_name), \
                                  wl_priority=NVL(:wl_priority,wl_priority), \
                                  tid=tid__seq.currval \
               WHERE pnr_id= :pnr_id; \
             END IF; \
           END;";
        CrsPnrInsQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrInsQry.DeclareVariable("target",otString);
        CrsPnrInsQry.DeclareVariable("subclass",otString);
        CrsPnrInsQry.DeclareVariable("class",otString);
        CrsPnrInsQry.DeclareVariable("grp_name",otString);
        CrsPnrInsQry.DeclareVariable("wl_priority",otString);
        CrsPnrInsQry.CreateVariable("crs",otString,crs);
        CrsPnrInsQry.DeclareVariable("pnr_id",otInteger);

        TQuery CrsPaxQry(&OraSession);
        CrsPaxQry.Clear();
        CrsPaxQry.SQLText=
          "SELECT pax_id,pr_del,last_op FROM crs_pax \
           WHERE pnr_id= :pnr_id AND \
                 surname= :surname AND (name= :name OR :name IS NULL AND name IS NULL) AND \
                 DECODE(seats,0,0,1)=:seats AND tid<>:tid \
           ORDER BY last_op DESC,pax_id DESC";
        CrsPaxQry.DeclareVariable("pnr_id",otInteger);
        CrsPaxQry.DeclareVariable("surname",otString);
        CrsPaxQry.DeclareVariable("name",otString);
        CrsPaxQry.DeclareVariable("seats",otInteger);
        CrsPaxQry.CreateVariable("tid",otInteger,tid);

        TQuery CrsPaxInsQry(&OraSession);
        CrsPaxInsQry.Clear();
        CrsPaxInsQry.SQLText=
          "BEGIN\
             IF :pax_id IS NULL THEN\
               SELECT pax_id.nextval INTO :pax_id FROM dual;\
               INSERT INTO crs_pax(pax_id,pnr_id,surname,name,pers_type,seat_no,seat_type,seats,pr_del,last_op,tid)\
               VALUES(:pax_id,:pnr_id,:surname,:name,:pers_type,:seat_no,:seat_type,:seats,:pr_del,:last_op,tid__seq.currval);\
             ELSE\
               UPDATE crs_pax\
               SET pers_type= :pers_type, seat_no= :seat_no, seat_type= :seat_type,\
                   pr_del= :pr_del, last_op= :last_op, tid=tid__seq.currval\
               WHERE pax_id=:pax_id;\
             END IF;\
           END;";
        CrsPaxInsQry.DeclareVariable("pax_id",otInteger);
        CrsPaxInsQry.DeclareVariable("pnr_id",otInteger);
        CrsPaxInsQry.DeclareVariable("surname",otString);
        CrsPaxInsQry.DeclareVariable("name",otString);
        CrsPaxInsQry.DeclareVariable("pers_type",otString);
        CrsPaxInsQry.DeclareVariable("seat_no",otString);
        CrsPaxInsQry.DeclareVariable("seat_type",otString);
        CrsPaxInsQry.DeclareVariable("seats",otInteger);
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
          "INSERT INTO crs_transfer(pnr_id,transfer_num,airline,flt_no,suffix,local_date,airp_dep,airp_arv,subclass)\
           VALUES(:pnr_id,:transfer_num,:airline,:flt_no,:suffix,:local_date,:airp_dep,:airp_arv,:subclass)";
        CrsTransferQry.DeclareVariable("pnr_id",otInteger);
        CrsTransferQry.DeclareVariable("transfer_num",otInteger);
        CrsTransferQry.DeclareVariable("airline",otString);
        CrsTransferQry.DeclareVariable("flt_no",otInteger);
        CrsTransferQry.DeclareVariable("suffix",otString);
        CrsTransferQry.DeclareVariable("local_date",otInteger);
        CrsTransferQry.DeclareVariable("airp_dep",otString);
        CrsTransferQry.DeclareVariable("airp_arv",otString);
        CrsTransferQry.DeclareVariable("subclass",otString);

        TQuery PnrAddrsQry(&OraSession);
        PnrAddrsQry.Clear();
        PnrAddrsQry.SQLText=
          "BEGIN\
             UPDATE pnr_addrs SET addr=:addr WHERE pnr_id=:pnr_id AND airline=:airline;\
             IF SQL%NOTFOUND THEN\
               INSERT INTO pnr_addrs(pnr_id,airline,addr) VALUES(:pnr_id,:airline,:addr);\
             END IF;\
           END;";
        PnrAddrsQry.DeclareVariable("pnr_id",otInteger);
        PnrAddrsQry.DeclareVariable("airline",otString);
        PnrAddrsQry.DeclareVariable("addr",otString);

        int pnr_id,pax_id;
        bool pr_sync_pnr;
        for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
        {
          CrsPnrQry.SetVariable("target",iTotals->dest);
          CrsPnrQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("target",iTotals->dest);
          CrsPnrInsQry.SetVariable("subclass",iTotals->subcl);
          CrsPnrInsQry.SetVariable("class",EncodeClass(iTotals->cl));
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
                "SELECT DISTINCT crs_pnr.pnr_id\
                 FROM crs_pnr,crs_pax\
                 WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND\
                       point_id=:point_id AND target= :target AND subclass= :subclass AND\
                       crs=:crs AND\
                       surname=:surname AND (name= :name OR :name IS NULL AND name IS NULL) AND\
                       seats>0\
                 ORDER BY crs_pnr.pnr_id";
              Qry.CreateVariable("point_id",otInteger,point_id);
              Qry.CreateVariable("target",otString,iTotals->dest);
              Qry.CreateVariable("subclass",otString,iTotals->subcl);
              Qry.CreateVariable("crs",otString,crs);
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
            CrsPnrInsQry.SetVariable("wl_priority",pnr.wl_priority);
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
                if (ne.indicator!=DEL)
                {
                  if (iPaxItem->seat.line!=0)
                  {
                    char buf[20];
                    sprintf(buf,"%d%c",iPaxItem->seat.row,iPaxItem->seat.line);
                    CrsPaxInsQry.SetVariable("seat_no",buf);
                  }
                  else
                    CrsPaxInsQry.SetVariable("seat_no",FNull);

                  if (strcmp(iPaxItem->seat.rem,"NSST")==0||
                      strcmp(iPaxItem->seat.rem,"NSSA")==0||
                      strcmp(iPaxItem->seat.rem,"NSSW")==0||
                      strcmp(iPaxItem->seat.rem,"SMST")==0||
                      strcmp(iPaxItem->seat.rem,"SMSA")==0||
                      strcmp(iPaxItem->seat.rem,"SMSW")==0)
                    CrsPaxInsQry.SetVariable("seat_type",iPaxItem->seat.rem);
                  else
                    CrsPaxInsQry.SetVariable("seat_type",FNull);
                }
                else
                {
                  CrsPaxInsQry.SetVariable("seat_no",FNull);
                  CrsPaxInsQry.SetVariable("seat_type",FNull);
                };
                CrsPaxInsQry.SetVariable("seats",(int)iPaxItem->seats);
                if (ne.indicator==DEL)
                  CrsPaxInsQry.SetVariable("pr_del",1);
                else
                  CrsPaxInsQry.SetVariable("pr_del",0);
                CrsPaxInsQry.SetVariable("last_op",info.time_create);
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
                    "    DELETE FROM crs_pax_tkn WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax_fqt WHERE pax_id=curRow.inf_id; "
                    "    DELETE FROM crs_pax WHERE pax_id=curRow.inf_id; "
                    "  END LOOP; "
                    "  DELETE FROM crs_pax_rem WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_doc WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_tkn WHERE pax_id=:pax_id; "
                    "  DELETE FROM crs_pax_fqt WHERE pax_id=:pax_id; "
                    "END;";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.Execute();
                };
                if (ne.indicator!=DEL)
                {
                  //��ࠡ�⪠ ������楢
                  int inf_id;

                  CrsPaxInsQry.SetVariable("pers_type",EncodePerson(baby));
                  CrsPaxInsQry.SetVariable("seat_no",FNull);
                  CrsPaxInsQry.SetVariable("seat_type",FNull);
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
                    SaveDOCSRem(inf_id,iInfItem->doc);
                    SaveTKNRem(inf_id,iInfItem->tkn);
                  };

                  //६�ન ���ᠦ��
                  SavePNLADLRemarks(pax_id,iPaxItem->rem);
                  SaveDOCSRem(pax_id,iPaxItem->doc);
                  SaveTKNRem(pax_id,iPaxItem->tkn);
                  SaveFQTRem(pax_id,iPaxItem->fqt);
                  //६�ન, �� �ਢ易��� � ���ᠦ���
                  SavePNLADLRemarks(pax_id,ne.rem);
                };

                if (!pr_sync_pnr)
                {
                  //������ ᨭ�஭����� ���ᠦ�� � ஧�᪮�
                  Qry.Clear();
                  Qry.SQLText=
                    "BEGIN\
                       mvd.sync_crs_pax(:pax_id);\
                     END;";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.Execute();
                };
              };
            }; //for(iNameElement=pnr.ne.begin()

            //����襬 ��몮���
            if (!pnr.transfer.empty())
            {
              //㤠�塞 ��䨣 �� ��몮���
              Qry.Clear();
              Qry.SQLText=
                "BEGIN\
                   DELETE FROM crs_transfer WHERE pnr_id= :pnr_id;\
                   UPDATE crs_pnr SET tid=tid__seq.currval WHERE pnr_id= :pnr_id;\
                 END;";
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
                //�஢�ઠ �� ����⢮����� � ���� ����� ������������,��ய��⮢,�������ᮢ
              /*  try
                {
                  GetAirline(iTransfer->flt.airline);
                }
                catch(ETlgError E)
                {
                  sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Transfer: %s",E.Message);
                };
                try
                {
                  if (iTransfer->flt.airp_dep[0]!=0)
                    GetAirp(iTransfer->flt.airp_dep);
                }
                catch(ETlgError E)
                {
                  sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Transfer: %s",E.Message);
                };
                try
                {
                  if (iTransfer->flt.airp_arv[0]!=0)
                    GetAirp(iTransfer->flt.airp_arv);
                }
                catch(ETlgError E)
                {
                  sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Transfer: %s",E.Message);
                };
                try
                {
                  GetClass(iTransfer->subcl);
                }
                catch(ETlgError E)
                {
                  sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Transfer: %s",E.Message);
                };*/
              };
              Qry.Clear();
              Qry.SQLText=
                "UPDATE crs_pnr SET last_target= "
                "  (SELECT airp_arv FROM crs_transfer WHERE pnr_id=:pnr_id AND transfer_num= "
                "    (SELECT MAX(transfer_num) FROM crs_transfer WHERE pnr_id=:pnr_id)) "
                "WHERE pnr_id=:pnr_id";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
            };
            if (pr_sync_pnr)
            {
              //������ ᨭ�஭����� �ᥩ ��㯯� � ஧�᪮�
              Qry.Clear();
              Qry.SQLText=
                "BEGIN\
                   mvd.sync_crs_pnr(:pnr_id);\
                 END;";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
            };

    /*        //�஢�ਬ, �� ���� �� ��㯯�
            Qry.Clear();
            Qry.SQLText=
              "DECLARE\
                 c BINARY_INTEGER;\
               BEGIN\
                 SELECT COUNT(*) INTO c FROM crs_pax WHERE pnr_id=:pnr_id AND seats>0;\
                 IF c=0 THEN\
                   UPDATE crs_pax SET pr_del=1,tid=tid__seq.currval WHERE pnr_id=:pnr_id AND seats>0;\
                 END IF;\
               END;";*/


          };//for(iPnrItem=iTotals->pnr.begin()
        };
        //ࠧ��⨬ ���஭�஢���� ���� � ᠫ���
        Qry.Clear();
        Qry.SQLText=
          "DECLARE "
          "  CURSOR cur IS "
          "    SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id; "
          "curRow cur%ROWTYPE; "
          "BEGIN "
          "  FOR curRow IN cur LOOP "
          "    salons.initcomp(CurRow.point_id_spp); "
          "  END LOOP; "
          "END;";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();
      };

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
          "BEGIN\
             UPDATE crs_data_stat SET pr_pnl=:pr_pnl WHERE point_id=:point_id AND crs=:crs;\
             IF SQL%NOTFOUND THEN\
               INSERT INTO crs_data_stat(point_id,crs,pr_pnl)\
               VALUES(:point_id,:crs,:pr_pnl);\
             END IF;\
           END;";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.CreateVariable("crs",otString,crs);
        Qry.CreateVariable("pr_pnl",otInteger,pr_pnl_new);
        Qry.Execute();
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
