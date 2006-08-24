#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#ifndef __WIN32__
 #include "lwriter.h"
#endif
#include "tlg_parser.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "oralib.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

const char lat_lines[]="ABCDEFGHIJK";
const char rus_lines[]="АБВГДЕЖЗИКЛ";
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

const char* TPnlAdlElementS[] =
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
               "EndOfMessage"};

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

char* TTlgParser::GetWord(char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p!=0&&!(*p>='A'&&*p<='Z'||*p>='А'&&*p<='Я'||*p=='Ё'||*p>='0'&&*p<='9');p++);
  for(len=0;*(p+len)>='A'&&*(p+len)<='Z'||
            *(p+len)>='А'&&*(p+len)<='Я'||*(p+len)=='Ё'||
            *(p+len)>='0'&&*(p+len)<='9';len++);
  if (len>(int)sizeof(lex)-1) len=sizeof(lex)-1;
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

char* TTlgParser::GetNameElement(char* p)
{
  int len;
  if (p==NULL) return NULL;
  for(;*p>0&&*p<=' '&&*p!='\n';p++);
  for(len=0;*(p+len)!=0&&*(p+len)!='\n';len++)
    if (*(unsigned char*)(p+len)<=' '&&*(p+len+1)=='.') break;
  for(len--;len>=0&&*(unsigned char*)(p+len)<=' ';len--);
  len++;
  if (len>(int)sizeof(lex)-1)
    throw ETlgError("Too long lexeme");
  lex[len]=0;
  strncpy(lex,p,len);
  if (len==0) return NULL;
  else return p+len;
};

char* GetPnlAdlElementName(TPnlAdlElement e)
{
  return (char*)TPnlAdlElementS[e];
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

enum TNsiType {ntAirlines,ntAirps,ntSubcls};

char* GetNsiCode(char* value, TNsiType nsi, char* code)
{
  static TQuery Qry(&OraSession);
  if (Qry.SQLText.IsEmpty()) Qry.DeclareVariable("code",otString);
  switch(nsi)
  {
    case ntAirlines:
      Qry.SQLText="SELECT kod_ak AS code FROM avia WHERE :code IN (kod_ak,latkod)";
      break;
    case ntAirps:
      Qry.SQLText="SELECT cod AS code FROM airps WHERE :code IN (cod,lat)";
      break;
    case ntSubcls:
      Qry.SQLText="SELECT class AS code FROM subcls WHERE :code IN (code,code_lat)";
      break;
  };
  Qry.SetVariable("code",value);
  Qry.Execute();
  if (Qry.RowCount()==0)
    switch(nsi)
    {
      case ntAirlines: throw ETlgError("Airline not found (code=%s)",Qry.GetVariableAsString("code"));
      case ntAirps:    throw ETlgError("Airport not found (code=%s)",Qry.GetVariableAsString("code"));
      case ntSubcls:   throw ETlgError("Class not found (code=%s)",Qry.GetVariableAsString("code"));
    };
  strcpy(code,Qry.FieldAsString("code"));
  Qry.Next();
  if (Qry.RowCount()>1)
    switch(nsi)
    {
      case ntAirlines: throw ETlgError("More than one airline found  (code=%s)",Qry.GetVariableAsString("code"));
      case ntAirps:    throw ETlgError("More than one airport found (code=%s)",Qry.GetVariableAsString("code"));
      case ntSubcls:   throw ETlgError("More than one class found (code=%s)",Qry.GetVariableAsString("code"));
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
  return DecodeClass(GetNsiCode(subcl,ntSubcls,subclh));
};

TTlgCategory GetTlgCategory(char *tlg_type)
{
  TTlgCategory cat;
  cat=tcUnknown;
  if (strcmp(tlg_type,"PNL")==0||
      strcmp(tlg_type,"ADL")==0) cat=tcDCS;
  if (strcmp(tlg_type,"MVT")==0||
      strcmp(tlg_type,"LDM")==0) cat=tcAHM;
  return cat;    
};

TTlgPartInfo ParseDCSHeading(TTlgPartInfo heading, THeadingInfo& info)
{
  int line,res;
  char c,*p,*line_p;
  TTlgParser tlg;
  TPnlAdlElement e;
  TTlgPartInfo next;

  time_t curr_time_t=time(NULL);
  struct tm *curr_time;

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
            res=sscanf(tlg.lex,"%8[A-ZА-ЯЁ0-9]/%2lu%3[A-ZА-ЯЁ0-9]%c",flt,&day,month,&c);
            if (c!=0||res!=3||day<=0) throw ETlgError("Wrong flight/local date");
            if (strlen(flt)<3) throw ETlgError("Wrong flight");
            info.flt.suffix[0]=0;
            info.flt.suffix[1]=0;
            c=0;
            if (flt[2]>='0'&&flt[2]<='9')
              res=sscanf(flt,"%2[A-ZА-ЯЁ0-9]%4lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            else
              res=sscanf(flt,"%3[A-ZА-ЯЁ0-9]%4lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            if (c!=0||res<2||info.flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !(info.flt.suffix[0]>='A'&&info.flt.suffix[0]<='Z'||
                  info.flt.suffix[0]>='А'&&info.flt.suffix[0]<='Я'||
                  info.flt.suffix[0]=='Ё')) throw ETlgError("Wrong flight");
            GetAirline(info.flt.airline);

            TDateTime today;
            curr_time=localtime(&curr_time_t);
            curr_time->tm_year+=1900;
            try
            {
              EncodeDate(curr_time->tm_year,curr_time->tm_mon+1,curr_time->tm_mday,today);
              for(curr_time->tm_mon=1;curr_time->tm_mon<=12;curr_time->tm_mon++)
                if (strcmp(month,Months[curr_time->tm_mon-1].lat)==0||
                    strcmp(month,Months[curr_time->tm_mon-1].rus)==0) break;
              EncodeDate(curr_time->tm_year,curr_time->tm_mon,day,info.flt.scd);
              if ((int)today>(int)info.flt.scd+30)  //рейс может задержаться на месяц
                EncodeDate(curr_time->tm_year+1,curr_time->tm_mon,day,info.flt.scd);
            }
            catch(EConvertError)
            {
              throw ETlgError("Wrong local date");
            };
            info.merge_key+=" ";
            info.merge_key+=tlg.lex;
            if ((p=tlg.GetLexeme(p))!=NULL)
            {
              c=0;
              res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",info.flt.brd_point,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong boarding point");
              GetAirp(info.flt.brd_point);
            }
            else throw ETlgError("Wrong boarding point");
            info.merge_key+=" ";
            info.merge_key+=tlg.lex;
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
              char ana[7];
              c=0;
              res=sscanf(tlg.lex,"ANA/%6[0-9]%c",ana,&c);
              if (c!=0||res!=1) throw ETlgError("Wrong association number");
              sprintf(tlg.lex,"ANA/%06s",ana);
              info.merge_key+=" ";
              info.merge_key+=tlg.lex;
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

TTlgPartInfo ParseAHMHeading(TTlgPartInfo heading, THeadingInfo& info)
{
  TTlgPartInfo next;
  next.p=heading.p;
  next.line=heading.line;
  return next;
};

void PasreAHMFltInfo(TTlgPartInfo body, THeadingInfo& info)
{
  int line,res;
  char c,*line_p;
  TTlgParser tlg;
  TPnlAdlElement e;

  time_t curr_time_t=time(NULL);
  struct tm *curr_time;

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
            char lexh[sizeof(tlg.lex)];
            long int day=0;
            char flt[9];
            res=sscanf(tlg.lex,"%11[A-ZА-ЯЁ0-9/].%s",lexh,tlg.lex);
            if (res!=2) throw ETlgError("Wrong flight identifier");
            c=0;
            res=sscanf(lexh,"%8[A-ZА-ЯЁ0-9]%c",flt,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(lexh,"%8[A-ZА-ЯЁ0-9]/%2lu%c",flt,&day,&c);
              if (c!=0||res!=2||day<=0) throw ETlgError("Wrong flight/GMT date");
            };
            if (strlen(flt)<3) throw ETlgError("Wrong flight");
            info.flt.suffix[0]=0;
            info.flt.suffix[1]=0;
            c=0;
            if (flt[2]>='0'&&flt[2]<='9')
              res=sscanf(flt,"%2[A-ZА-ЯЁ0-9]%4lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            else
              res=sscanf(flt,"%3[A-ZА-ЯЁ0-9]%4lu%c%c",
                             info.flt.airline,&info.flt.flt_no,&(info.flt.suffix[0]),&c);
            if (c!=0||res<2||info.flt.flt_no<0) throw ETlgError("Wrong flight");
            if (res==3&&
                !(info.flt.suffix[0]>='A'&&info.flt.suffix[0]<='Z'||
                  info.flt.suffix[0]>='А'&&info.flt.suffix[0]<='Я'||
                  info.flt.suffix[0]=='Ё')) throw ETlgError("Wrong flight");
            GetAirline(info.flt.airline);
            //переведем day в TDateTime
            curr_time=gmtime(&curr_time_t);
            curr_time->tm_year+=1900;
            curr_time->tm_mon+=1;
            if (curr_time->tm_mday+1<day) //м.б. разность системных времен у формирователя и приемщика, поэтому +1!
            {
              if (curr_time->tm_mon==1)
              {
                curr_time->tm_mon=12;
                curr_time->tm_year--;
              }
              else curr_time->tm_mon--;
            };
            try
            {
              EncodeDate(curr_time->tm_year,curr_time->tm_mon,day,info.flt.scd);
            }
            catch(EConvertError)
            {
              throw ETlgError("Wrong GMT date");
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
    //вывести ошибку+номер строки
    throw ETlgError("Line %d: %s",line,E.what());
  };
  return;
};

//возвращает TTlgPartInfo следующей части (body)
TTlgPartInfo ParseHeading(TTlgPartInfo heading, THeadingInfo& info)
{
  info.Clear();

  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  TPnlAdlElement e;
  TTlgPartInfo next;

  time_t curr_time_t=time(NULL);
  struct tm *curr_time;

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
          res=sscanf(tlg.lex,".%7[A-ZА-ЯЁ0-9]%c",info.sender,&c);
          if (c!=0||res!=1||strlen(info.sender)!=7) throw ETlgError("Wrong address");
          info.merge_key=tlg.lex;
          if ((ph=tlg.GetLexeme(p))!=NULL)
          {
            c=0;
            res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%c%s",info.double_signature,&c,tlg.lex);
            if (res<2||c!='/'||strlen(info.double_signature)!=2)
            {
              *info.double_signature=0;
              p=tlg.GetLexeme(p);
            }
            else
            {
              if (res==2) *tlg.lex=0;
              p=ph;
            };
            while (p!=NULL)
            {
              if (info.message_identity.empty())
                info.message_identity=tlg.lex;
              else
              {
                info.message_identity+=' ';
                info.message_identity+=tlg.lex;
              };
              if (info.time_create==0)
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
                  curr_time=gmtime(&curr_time_t);
                  curr_time->tm_year+=1900;
                  curr_time->tm_mon+=1;
                  if (curr_time->tm_mday+1<day) //м.б. разность системных времен у формирователя и приемщика, поэтому +1!
                  {
                    if (curr_time->tm_mon==1)
                    {
                      curr_time->tm_mon=12;
                      curr_time->tm_year--;
                    }
                    else curr_time->tm_mon--;
                  };
                  try
                  {
                    EncodeDate(curr_time->tm_year,curr_time->tm_mon,day,day_create);
                    EncodeTime(hour,min,sec,time_create);
                    info.time_create=day_create+time_create;
                  }
                  catch(EConvertError)
                  {
                    throw ETlgError("Wrong creation time");
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
          next.p=line_p;
          next.line=line;
          //message identifier
          c=0;
          res=sscanf(tlg.lex,"%3[A-Z]%c",info.tlg_type,&c);
          if (c!=0||res!=1||tlg.GetLexeme(p)!=NULL)
          {
            *(info.tlg_type)=0;
          }
          else
          {
            switch (GetTlgCategory(info.tlg_type))
            {
              case tcDCS:
                heading.p=tlg.NextLine(line_p);
                heading.line=line+1;
                next=ParseDCSHeading(heading,info);
                break;
              case tcAHM:
                heading.p=tlg.NextLine(line_p);
                heading.line=line+1;
                next=ParseAHMHeading(heading,info);
                break;
              default:;
            };
          };
          return next;
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

void ParseDCSEnding(TTlgPartInfo ending, TEndingInfo& info)
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
      sprintf(endtlg,"END%s",info.tlg_type);
      if (strcmp(tlg.lex,endtlg)!=0)
      {
        long part_no=info.part_no;
        c=0;
        res=sscanf(tlg.lex+3,"PART%lu%c",&info.part_no,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong end of message");
        if (info.part_no<=0||info.part_no>=1000) throw ETlgError("Wrong part number");
        if (part_no!=0&&info.part_no!=part_no) throw ETlgError("Wrong part number");
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
        long part_no=info.part_no;
        c=0;
        res=sscanf(tlg.lex,"%lu%c",&info.part_no,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong end of message");
        if (info.part_no<=0||info.part_no>=1000) throw ETlgError("Wrong part number");
        if (part_no!=0&&info.part_no!=part_no) throw ETlgError("Wrong part number");
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
void ParseEnding(TTlgPartInfo ending, TEndingInfo& info)
{
  switch (GetTlgCategory(info.tlg_type))
  {
    case tcDCS:
      ParseDCSEnding(ending,info);
      break;
    case tcAHM:
      ParseAHMEnding(ending,info);
      break;
    default:;
  };
  return;
};

void ParseNameElement(TTlgParser &tlg, THeadingInfo& info, TPnrItem &pnr, bool &pr_prev_rem);
void ParseRemarks(TTlgParser &tlg, TNameElement &ne);

void ParsePnlAdlBody(TTlgPartInfo body, THeadingInfo& info, TPnlAdlContent& con)
{
  vector<TRbdItem>::iterator iRbdItem;
  vector<TRouteItem>::iterator iRouteItem;
  vector<TSeatsItem>::iterator iSeatsItem;
  vector<TTotalsByDest>::iterator iTotals;
  vector<TPaxItem>::iterator iPaxItem,iPaxItemPrev;
  vector<TPnrItem>::iterator iPnrItem;

  con.rbd.clear();
  con.cfg.clear();
  con.avail.clear();
  con.transit.clear();
  con.resa.clear();

  int line,res;
  char c,*p,*line_p,*ph;
  TTlgParser tlg;
  char lexh[sizeof(tlg.lex)];
  TPnlAdlElement e;
  TIndicator Indicator;
  int e_part;
  bool pr_prev_rem;
  try
  {
    line_p=body.p;
    line=body.line-1;
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
                strcpy(RouteItem.station,info.flt.brd_point);
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
                  if (strcmp(RouteItem.station,iRouteItem->station)==0)
                    throw ETlgError("Duplicate airport code '%s'",RouteItem.station);
                };
                if (!(con.avail.empty()&&RouteItem.station==info.flt.brd_point))
                  con.avail.push_back(RouteItem);

              }
              while ((p=tlg.GetLexeme(p))!=NULL);
              if (!con.avail.empty())
              {
                iRouteItem=con.avail.begin();
                if (strcmp(iRouteItem->station,info.flt.brd_point)==0)
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
/*            if (pr_parse_ne)
            {
              ParseRemarks(tlg,ne);
              iTotals->ne.push_back(ne); //сохранить, поскольку новый totals
              SaveNameElement(tid,Totals,Indicator,ne);
              //восстановить lex
              tlg.GetLexeme(line_p);
              //очистить старый NameElement
              ne.Clear();
            };*/
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

//          if (!pr_parse_ne) break;  //не разбирать name element

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
//          pr_ne=1;

          if (e_part>1)
          {
            if (!(tlg.lex[0]=='.'&&e_part>2))
            {
/*              ParseRemarks(tlg,ne);
              iTotals->ne.push_back(ne);  //сохранить, поскольку начался новый PNR
              SaveNameElement(tid,Totals,Indicator,ne);
              //очистить старый NameElement
              ne.Clear();*/

              if ((p=tlg.GetNameElement(line_p))==NULL) continue;

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

              lexh[0]=0;
              res=sscanf(tlg.lex,"%3lu%[A-ZА-ЯЁ -]%[^.]",&ne.seats,lexh,tlg.lex);
              if (res<2||lexh[0]==0||ne.seats<=0) throw ETlgError("Wrong format");
              ne.surname=Trim(lexh);
//              if (TrimString(ne.surname).size()<2) throw ETlgError("Too short surname");
              TPaxItem PaxItem;
              ne.pax.assign(ne.seats,PaxItem);

              if (res==3)
              {
                //есть имена
                c=0;
                res=sscanf(tlg.lex,"%[A-ZА-ЯЁ /-]%c",lexh,&c); 
                if (c!=0||res!=1) throw ETlgError("Wrong format");

                iPaxItem=ne.pax.begin();
                do
                {
                  if (iPaxItem==ne.pax.end()) throw ETlgError("Too many names");
                  lexh[0]=0;
                  res=sscanf(tlg.lex,"/%[A-ZА-ЯЁ -]%[A-ZА-ЯЁ /-]",lexh,tlg.lex);
                  if (res<1) throw ETlgError("Wrong format");
                  Trim(lexh);
                  if (lexh[0]!=0)
                  {
                    iPaxItem->name=lexh;
                    iPaxItem++;
                  };
                }
                while (res==2);
                if (ne.surname=="ZZ"&&!ne.pax.empty())
                {
                  TPaxItem& PaxItem=ne.pax.front();
                  if (!PaxItem.name.empty())
                  {
                    for(;iPaxItem!=ne.pax.end();iPaxItem++)
                      iPaxItem->name=PaxItem.name;
                  };
                };

                for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
                {
                  if (iPaxItem->name.find("CHD")!=string::npos)
                    iPaxItem->pers_type=child;
/*                  if (iPaxItem->name=="EXST"||iPaxItem->name=="STCR")
                    iPaxItem->pers_type=NoPerson;*/
                };
              };
            }
            else p=line_p;

            while ((p=tlg.GetNameElement(p))!=NULL)
            {
              ParseNameElement(tlg,info,*iPnrItem,pr_prev_rem);
            };

            e_part=3;
            break;
          };
        default:;
      };
    }
    while ((line_p=tlg.NextLine(line_p))!=NULL);
/*    if (pr_parse_ne)
    {
      ParseRemarks(tlg,ne);
      iTotals->ne.push_back(ne);  //сохранить то, что не сохранили
      SaveNameElement(tid,Totals,Indicator,ne);
    };*/
    //разобрать все ремарки
    vector<TNameElement>::iterator i;
    for(iTotals=con.resa.begin();iTotals!=con.resa.end();iTotals++)
      for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end();iPnrItem++)
        for(i=iPnrItem->ne.begin();i!=iPnrItem->ne.end();i++)
        {
          ParseRemarks(tlg,*i);
          // проставить для EXST и STCR реальное кол-во мест
          for(iPaxItem=i->pax.begin();iPaxItem!=i->pax.end();)
          {
            if (iPaxItem!=i->pax.begin()&&
                (iPaxItem->name=="EXST"||iPaxItem->name=="STCR"))
            {
              iPaxItemPrev->seats++;
              //перебросим ремарки
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
  }
  catch (ETlgError E)
  {
    throw ETlgError("%s, line %d: %s",GetPnlAdlElementName(e),line,E.what());
  };
  return;
};

void ParseNameElement(TTlgParser &tlg, THeadingInfo& info, TPnrItem &pnr, bool &pr_prev_rem)
{
  char c,lexh[sizeof(tlg.lex)];
  int res;
  if (pnr.ne.empty()) return;
  TNameElement& ne=pnr.ne.back();

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
    if (!RemItem.text.empty())
    {
      c=RemItem.text.at(RemItem.text.size()-1);
      if ((c>='A'&&c<='Z'||
           c>='А'&&c<='Я'||c=='Ё'||
           c>='0'&&c<='9')&&
          (tlg.lex[4]>='A'&&tlg.lex[4]<='Z'||
           tlg.lex[4]>='А'&&tlg.lex[4]<='Я'||tlg.lex[4]=='Ё'||
           tlg.lex[4]>='0'&&tlg.lex[4]<='9'))
        RemItem.text+=" ";
    };
    RemItem.text+=tlg.lex+4;
    return;
  };
  pr_prev_rem=false;
  if (strcmp(lexh,"C")==0)
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
    if (PnrAddr.airline[0]==0) strcpy(PnrAddr.airline,info.flt.airline);

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
      //если а/к PNR и PNL совпадают - вставим PNR первым
      if (strcmp(PnrAddr.airline,info.flt.airline)==0)
        pnr.addrs.insert(pnr.addrs.begin(),PnrAddr);
      else
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
      res=sscanf(tlg.lex,".WL/%6[A-ZА-ЯЁ0-9]%c",lexh,&c);
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
      res=sscanf(tlg.lex,".%*c/%[A-ZА-ЯЁ0-9]%c",lexh,&c);
    }  
    else
    {
      c=0;
      res=sscanf(lexh+1,"%1lu%c",&Transfer.num,&c);
      if (res==0) return; //другой элемент
      if (c!=0||res!=1||Transfer.num<=0)
        throw ETlgError("Wrong connection element");
      if (lexh[0]=='I') Transfer.num=-Transfer.num;
      c=0;
      res=sscanf(tlg.lex,".%*c%*1[0-9]/%[A-ZА-ЯЁ0-9]%c",lexh,&c);
    };    
    if (c!=0||res!=1) throw ETlgError("Wrong connection element");
    c=0;
    if (isdigit(lexh[2]))
    {
      res=sscanf(lexh,"%2[A-ZА-ЯЁ0-9]%4lu%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                      Transfer.flt.airline,&Transfer.flt.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%2[A-ZА-ЯЁ0-9]%4lu%1[A-ZА-ЯЁ]%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                        Transfer.flt.airline,&Transfer.flt.flt_no,
                        Transfer.flt.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError("Wrong connection element");
      };
    }
    else
    {
      res=sscanf(lexh,"%3[A-ZА-ЯЁ0-9]%4lu%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                      Transfer.flt.airline,&Transfer.flt.flt_no,
                      Transfer.subcl,&Transfer.local_date,tlg.lex);
      if (res!=5)
      {
        res=sscanf(lexh,"%3[A-ZА-ЯЁ0-9]%4lu%1[A-ZА-ЯЁ]%1[A-ZА-ЯЁ]%2lu%[A-ZА-ЯЁ0-9]",
                        Transfer.flt.airline,&Transfer.flt.flt_no,
                        Transfer.flt.suffix,Transfer.subcl,&Transfer.local_date,tlg.lex);
        if (res!=6) throw ETlgError("Wrong connection element");
      };
    };

    if (Transfer.flt.flt_no<0||Transfer.local_date<=0||Transfer.local_date>31)
      throw ETlgError("Wrong connection element");

    Transfer.flt.brd_point[0]=0;
    Transfer.arv_point[0]=0;
    lexh[0]=0;
    res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%[A-ZА-ЯЁ0-9]", 
                       Transfer.flt.brd_point,Transfer.arv_point,lexh);
    if (res<2||Transfer.flt.brd_point[0]==0||Transfer.arv_point[0]==0)
    {
      Transfer.flt.brd_point[0]=0;
      lexh[0]=0;
      res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%[A-ZА-ЯЁ0-9]",
                         Transfer.flt.brd_point,lexh);
      if (res<1||Transfer.flt.brd_point[0]==0) throw ETlgError("Wrong connection element");
    };
    if (lexh[0]!=0)
    {
      c=0;
      res=sscanf(lexh,"%4lu%2[A-ZА-ЯЁ]%c",&Transfer.local_time,tlg.lex,&c);
      if (c!=0||res!=2)
      {
        c=0;
        res=sscanf(lexh,"%4lu%c",&Transfer.local_time,&c);
        if (c!=0||res!=1) throw ETlgError("Wrong connection element");
      };
    };
    if (Transfer.num>0&&Transfer.arv_point[0]==0)
    {
      strcpy(Transfer.arv_point,Transfer.flt.brd_point);
      Transfer.flt.brd_point[0]=0;
    };
    //анализ на повторение
    vector<TTransferItem>::iterator i;
    for(i=pnr.transfer.begin();i!=pnr.transfer.end();i++)
      if (Transfer.num==i->num) break;
    if (i!=pnr.transfer.end())
    {
      if (strcmp(i->flt.airline,Transfer.flt.airline)!=0||
          i->flt.flt_no!=Transfer.flt.flt_no||
          strcmp(i->flt.suffix,Transfer.flt.suffix)!=0||
          i->flt.brd_point[0]!=0&&Transfer.flt.brd_point[0]!=0&&
          strcmp(i->flt.brd_point,Transfer.flt.brd_point)!=0||
          i->local_date!=Transfer.local_date||
          i->arv_point[0]!=0&&Transfer.arv_point[0]!=0&&
          strcmp(i->arv_point,Transfer.arv_point)!=0||
          strcmp(i->subcl,Transfer.subcl)!=0)
        throw ETlgError("Different inbound/onward connection in group found");
    }
    else pnr.transfer.push_back(Transfer);
    return;
  };
};

void ParseRemarks(TTlgParser &tlg, TNameElement &ne)
{
  char c,lexh[sizeof(tlg.lex)];
  int res,k;
  unsigned int pos,len;
  bool pr_parse;
  string strh;
  char *p;
  char rem_code[7];
  long num;
  vector<TRemItem>::iterator iRemItem;
  vector<TPaxItem>::iterator iPaxItem,iPaxItem2;
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
  {
    if (iRemItem->text.empty()) continue;
    p=tlg.GetWord((char*)iRemItem->text.c_str());
    c=0;
    res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
    if (c!=0||res!=1) continue;

    if (strcmp(rem_code,"TKNO")==0||
        strcmp(rem_code,"TKNA")==0||
        strcmp(rem_code,"TKNE")==0||
        strcmp(rem_code,"TKNM")==0||
        strcmp(rem_code,"TKN")==0||
        strcmp(rem_code,"TKT")==0||
        strcmp(rem_code,"TKTN")==0||
        strcmp(rem_code,"TKTNO")==0||
        strcmp(rem_code,"TTKNR")==0||
        strcmp(rem_code,"TTKNO")==0)
    {
      strcpy(iRemItem->code,"TKNO");
      continue;
    };
    if (strlen(rem_code)<=5) strcpy(iRemItem->code,rem_code);

    num=1;
    sscanf(rem_code,"%3lu%s",&num,rem_code);
    if (num<=0) num=1;
    if (strcmp(rem_code,"CHD")==0||
        strcmp(rem_code,"INF")==0||
        strcmp(rem_code,"VIP")==0) strcpy(iRemItem->code,rem_code);

    if (strcmp(rem_code,"INF")==0)
    {
          //отдельный массив для INF ne.inf
      TInfItem InfItem;
      InfItem.surname=ne.surname;
      vector<TInfItem> inf(num,InfItem);
      vector<TInfItem>::iterator iInfItem;
      if (p==NULL) continue;
      pr_parse=false;
      do
      {
        //поискать фамилию/имя внутри ремарки
        res=sscanf(p,"%*[^A-ZА-ЯЁ0-9]%64[A-ZА-ЯЁ]%64[A-ZА-ЯЁ/]",lexh,tlg.lex);
        if (res==0)
          res=sscanf(p,"%64[A-ZА-ЯЁ]%64[A-ZА-ЯЁ/]",lexh,tlg.lex);
        if (res!=2) continue;
        InfItem.surname=lexh;
        iInfItem=inf.begin();
        do
        {
          lexh[0]=0;
          res=sscanf(tlg.lex,"/%[A-ZА-ЯЁ]%[A-ZА-ЯЁ/]",lexh,tlg.lex); 
          if (res<1) break;
          if (lexh[0]!=0)
          {
            iInfItem->surname=InfItem.surname;
            iInfItem->name=lexh;
            pr_parse=true;
            iInfItem++;
          };
        }
        while(iInfItem!=inf.end()&&res==2);
      }
      while(!pr_parse&&(p=tlg.GetWord(p))!=NULL);
      ne.inf.insert(ne.inf.end(),inf.begin(),inf.end());
      continue;
    };
  };
  //попробовать привязать ремарки к конкретным пассажирам
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();)
  {
    if (iRemItem->text.empty()||
        (pos=iRemItem->text.find_last_of('-'))==string::npos)
    {
      iRemItem++;
      continue;
    };
    strh=iRemItem->text.substr(pos);
    if (strh.size()>=sizeof(tlg.lex))
    {
      iRemItem++;
      continue;
    };

    res=sscanf(strh.c_str(),"-%*3[0-9]%[A-ZА-ЯЁ ]%[A-ZА-ЯЁ /]",lexh,tlg.lex);
    if (res==0)
      res=sscanf(strh.c_str(),"-%[A-ZА-ЯЁ ]%[A-ZА-ЯЁ /]",lexh,tlg.lex);
    if (res!=2||ne.surname!=Trim(lexh))
    {
      iRemItem++;
      continue;
    };

    pr_parse=true;
    do
    {
      lexh[0]=0;
      res=sscanf(tlg.lex,"/%[A-ZА-ЯЁ ]%[A-ZА-ЯЁ /]",lexh,tlg.lex);
      if (res<1)
      {
        pr_parse=false;
        break;
      };
      Trim(lexh);
      if (lexh[0]!=0)
      {
        for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
        {
          if (iPaxItem->name!=lexh) continue;
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
        if (iPaxItem==ne.pax.end())
        {
          pr_parse=false;
          break;
        };
      };
    }
    while (res==2);

    if (pr_parse) iRemItem->text.erase(pos); //убрать окончание ремарки после -

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

  //сначала пробегаем по ремаркам каждого из пассажиров (из-за номеров мест)
  for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
  {
    for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();iRemItem++)
    {
      if (iRemItem->text.empty()) continue;
      p=tlg.GetWord((char*)iRemItem->text.c_str());
      c=0;
      res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
      if (c!=0||res!=1) continue;

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
        TSeat seat;
        if (strcmp(rem_code,"NSST")==0||
            strcmp(rem_code,"NSSA")==0||
            strcmp(rem_code,"NSSB")==0||
            strcmp(rem_code,"NSSW")==0||
            strcmp(rem_code,"SMST")==0||
            strcmp(rem_code,"SMSA")==0||
            strcmp(rem_code,"SMSB")==0||
            strcmp(rem_code,"SMSW")==0)
          strcpy(seat.rem,rem_code);
        else
          strcpy(seat.rem,"");
        while(iPaxItem->seat.line==0&&tlg.GetWord(p)!=NULL)
        {
          pr_parse=true;
          do
          {
            lexh[0]=0;
            res=sscanf(tlg.lex,"%3lu%[A-ZА-ЯЁ]%s",&seat.row,lexh,tlg.lex);
            if (res<2||lexh[0]==0||seat.row<=0)
            {
              pr_parse=false;
              break;
            };
            for(k=0;lexh[k]!=0&&GetSalonLine(lexh[k])!=0;k++);
            if (lexh[k]!=0)
            {
              pr_parse=false;
              break;
            };
          }
          while (res==3);
          p=tlg.GetWord(p);
          if (!pr_parse) continue;
          do
          {
            lexh[0]=0;
            res=sscanf(tlg.lex,"%3lu%[A-ZА-ЯЁ]%s",&seat.row,lexh,tlg.lex);
            if (res<2||lexh[0]==0||seat.row<=0) break;
            for(k=0;lexh[k]!=0&&(/*seat.line=*/GetSalonLine(lexh[k]))!=0;k++)
            {
               seat.line=lexh[k];
               //записать место из seat, если не найдено ни у кого другого
               for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                 if (seat.row==iPaxItem2->seat.row&&
                     seat.line==iPaxItem2->seat.line) break;

               if (iPaxItem2==ne.pax.end())
               {
                 iPaxItem->seat=seat;
                 break;
               };
            };
            if (lexh[k]!=0) break;
          }
          while (iPaxItem->seat.line==0&&res==3);
        };
        continue;
      };

      num=1;
      sscanf(rem_code,"%3lu%s",&num,rem_code);

      if (strcmp(rem_code,"CHD")==0)
      {
        iPaxItem->pers_type=child;
        continue;
      };
    };
  };

  //пробегаем по ремаркам группы
  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
  {
    if (iRemItem->text.empty()) continue;
    p=tlg.GetWord((char*)iRemItem->text.c_str());
    c=0;
    res=sscanf(tlg.lex,"%6[A-ZА-ЯЁ0-9]%c",rem_code,&c);
    if (c!=0||res!=1) continue;

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
      TSeat seat;
      if (strcmp(rem_code,"NSST")==0||
          strcmp(rem_code,"NSSA")==0||
          strcmp(rem_code,"NSSB")==0||
          strcmp(rem_code,"NSSW")==0||
          strcmp(rem_code,"SMST")==0||
          strcmp(rem_code,"SMSA")==0||
          strcmp(rem_code,"SMSB")==0||
          strcmp(rem_code,"SMSW")==0)
        strcpy(seat.rem,rem_code);
      else
        strcpy(seat.rem,"");
      while(tlg.GetWord(p)!=NULL)
      {
        pr_parse=true;
        do
        {
          lexh[0]=0;
          res=sscanf(tlg.lex,"%3lu%[A-ZА-ЯЁ]%s",&seat.row,lexh,tlg.lex);
          if (res<2||lexh[0]==0||seat.row<=0)
          {
            pr_parse=false;
            break;
          };
          for(k=0;lexh[k]!=0&&GetSalonLine(lexh[k])!=0;k++);
          if (lexh[k]!=0)
          {
            pr_parse=false;
            break;
          };
        }
        while (res==3);
        p=tlg.GetWord(p);
        if (!pr_parse) continue;
        do
        {
          lexh[0]=0;
          res=sscanf(tlg.lex,"%3lu%[A-ZА-ЯЁ]%s",&seat.row,lexh,tlg.lex);
          if (res<2||lexh[0]==0||seat.row<=0) break;
          for(k=0;lexh[k]!=0&&(/*seat.line=*/GetSalonLine(lexh[k]))!=0;k++)
          {
             seat.line=lexh[k];
             //записать место из seat, если не найдено ни у кого другого
             for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                 if (seat.row==iPaxItem2->seat.row&&
                     seat.line==iPaxItem2->seat.line) break;

             if (iPaxItem2==ne.pax.end())
               //найти пассажира, которому не проставлено место
               if (strcmp(rem_code,"EXST")==0)
               {
                 for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                   if (iPaxItem2->seat.line==0&&
                       iPaxItem2->name=="EXST")
                   {
                     iPaxItem2->seat=seat;
                     break;
                   };
               };
             if (iPaxItem2==ne.pax.end())
               for(iPaxItem2=ne.pax.begin();iPaxItem2!=ne.pax.end();iPaxItem2++)
                 if (iPaxItem2->seat.line==0)
                 {
                   iPaxItem2->seat=seat;
                   break;
                 };
          };
          if (lexh[k]!=0) break;
        }
        while (res==3);
      };
      continue;
    };

    num=1;
    sscanf(rem_code,"%3lu%s",&num,rem_code);
    if (strcmp(rem_code,"CHD")==0)
    {
      if (ne.pax.size()==1) ne.pax.begin()->pers_type=child; 
      else
      {
        if (p==NULL) continue;
        pr_parse=false;
        do
        {
          //поискать фамилию/имя внутри ремарки
          res=sscanf(p,"%*[^A-ZА-ЯЁ0-9]%*[0-9]%64[A-ZА-ЯЁ ]%64[A-ZА-ЯЁ /]",lexh,tlg.lex); 
          if (res==0)
            res=sscanf(p,"%*[0-9]%64[A-ZА-ЯЁ ]%64[A-ZА-ЯЁ /]",lexh,tlg.lex);
          if (res==0)
            res=sscanf(p,"%64[A-ZА-ЯЁ ]%64[A-ZА-ЯЁ /]",lexh,tlg.lex);
          if (res!=2||ne.surname!=Trim(lexh)) continue;
          do
          {
            lexh[0]=0;
            res=sscanf(tlg.lex,"/%[A-ZА-ЯЁ ]%[A-ZА-ЯЁ /]",lexh,tlg.lex);
            if (res<1) break;
            Trim(lexh);
            if (lexh[0]!=0)
            {
              for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
                if (!iPaxItem->name.empty())
                {
                  len=iPaxItem->name.size();
                  if (strncmp(iPaxItem->name.c_str(),lexh,len)==0&&
                      (lexh[len]==0||lexh[len]==' '))
                  {
                    iPaxItem->pers_type=child;
                    pr_parse=true;
                    break;
                  };
                };
            };
          }
          while (res==2);
        }
        while(!pr_parse&&(p=tlg.GetWord(p))!=NULL);
      };
      continue;
    };
  };
};

TTlgParts GetParts(char* tlg_p)
{
  int line;
  char *p,*line_p;
  TTlgParser tlg;
  THeadingInfo HeadingInfo;
  TTlgParts parts;
  TPnlAdlElement e;

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
          parts.body=ParseHeading(parts.heading,HeadingInfo);  //может вернуть NULL
          line_p=parts.body.p;
          line=parts.body.line;
          e=EndOfMessage;
          if ((p=tlg.GetLexeme(line_p))==NULL) break;
        case EndOfMessage:
          switch (GetTlgCategory(HeadingInfo.tlg_type))
          {
            case tcDCS:
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
  }
  catch(ETlgError E)
  {
    //вывести ошибку
    throw;
  };
  if (parts.addr.p==NULL) throw ETlgError("Address not found");
  if (parts.heading.p==NULL) throw ETlgError("Heading not found");
  if (parts.body.p==NULL) throw ETlgError("Body not found");
  if (GetTlgCategory(HeadingInfo.tlg_type)==tcDCS&&
      parts.ending.p==NULL) throw ETlgError("End of message not found");
  return parts;
};

bool SavePnlAdlContent(int point_id, THeadingInfo& info, TPnlAdlContent& con, bool forcibly,
                       char* OWN_CANON_NAME, char* ERR_CANON_NAME)
{
  vector<TRouteItem>::iterator iRouteItem;
  vector<TSeatsItem>::iterator iSeatsItem;
  vector<TTotalsByDest>::iterator iTotals;
  vector<TPnrItem>::iterator iPnrItem;
  vector<TNameElement>::iterator iNameElement;
  vector<TPaxItem>::iterator iPaxItem;
  vector<TRemItem>::iterator iRemItem;
  vector<TTransferItem>::iterator iTransfer;
  vector<TPnrAddrItem>::iterator iPnrAddr;
  vector<TInfItem>::iterator iInfItem;

  char crs[sizeof(info.sender)];  
  int tid;

  TQuery Qry(&OraSession);

  if (info.time_create==0) throw ETlgError("Creation time not defined");

  //идентифицируем систему бронирования
//  if (strlen(info.sender)>5&&strcmp(info.sender+5,"1M")==0)
    //это Сирена 2.3
    strcpy(crs,info.sender);
/*  else
  {
    strncpy(crs,info.sender,5);
    crs[5]=0;
  };*/
#ifdef __WIN32__
  Qry.Clear();
  Qry.SQLText="SELECT code FROM crs2 WHERE code=:code";
  Qry.CreateVariable("code",otString,crs);
  Qry.Execute();
  if (Qry.RowCount()==0)
  {
#endif
  Qry.Clear();
  Qry.SQLText="INSERT INTO crs2(code,name) VALUES(:code,:code)";
  Qry.CreateVariable("code",otString,crs);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
  };
#ifdef __WIN32__
  };
#endif

  Qry.Clear();
  Qry.SQLText=
    "SELECT priority FROM crs_set\
     WHERE crs=:crs AND airline=:airline AND \
           (flt_no=:flt_no OR flt_no IS NULL) AND\
           (airp_dep=:airp_dep OR airp_dep IS NULL) AND rownum=1";
  Qry.CreateVariable("crs",otString,crs);
  Qry.CreateVariable("airline",otString,info.flt.airline);
  Qry.CreateVariable("flt_no",otInteger,(int)info.flt.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.flt.brd_point);
  Qry.Execute();
  if (Qry.RowCount()==0)
  {
    Qry.SQLText=
      "INSERT INTO crs_set(airline,flt_no,airp_dep,crs,priority)\
       VALUES(:airline,:flt_no,:airp_dep,:crs,0)";
    Qry.Execute();   
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
  if (Qry.RowCount()>0)
  {
    if (!Qry.FieldIsNULL("last_resa"))
      last_resa=Qry.FieldAsDateTime("last_resa");
    if (!Qry.FieldIsNULL("last_tranzit"))
      last_tranzit=Qry.FieldAsDateTime("last_tranzit");
    if (!Qry.FieldIsNULL("last_cfg"))
      last_cfg=Qry.FieldAsDateTime("last_cfg");
    pr_pnl=Qry.FieldAsInteger("pr_pnl");
  };
  
  //pr_pnl=0 - не пришел цифровой PNL
  //pr_pnl=1 - пришел цифровой PNL
  //pr_pnl=2 - пришел нецифровой PNL

  bool pr_save_ne=!(strcmp(info.tlg_type,"PNL")==0&&pr_pnl==2|| //пришел второй нецифровой PNL
                    strcmp(info.tlg_type,"ADL")==0&&pr_pnl!=2); //пришел ADL до обоих PNL

  bool pr_recount=false;
  //записать цифровые данные
  if (!con.resa.empty()&&(last_resa==0||last_resa<=info.time_create))
  {
    pr_recount=true;
    Qry.Clear();
    Qry.SQLText=
      "BEGIN \
         UPDATE crs_data2 SET resa=NULL,pad=NULL WHERE point_id=:point_id AND crs=:crs; \
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
         UPDATE crs_data2 SET resa=NVL(resa+:resa,:resa), pad=NVL(pad+:pad,:pad)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data2(point_id,target,class,crs,resa,pad) \
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
         UPDATE crs_data2 SET tranzit=NULL WHERE point_id=:point_id AND crs=:crs; \
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
         UPDATE crs_data2 SET tranzit=NVL(tranzit+:tranzit,:tranzit)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data2(point_id,target,class,crs,tranzit) \
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
         UPDATE crs_data2 SET cfg=NULL,avail=NULL WHERE point_id=:point_id AND crs=:crs; \
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
         UPDATE crs_data2 SET cfg=NVL(cfg+:cfg,:cfg)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data2(point_id,target,class,crs,cfg) \
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
         UPDATE crs_data2 SET avail=NVL(avail+:avail,:avail)\
         WHERE point_id=:point_id AND target=:target AND class=:class AND crs=:crs;\
         IF SQL%NOTFOUND THEN \
           INSERT INTO crs_data2(point_id,target,class,crs,avail) \
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
  if (pr_recount)
  {
    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         ckin.recount(:point_id);\
       END;";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
  };
  OraSession.Commit();

  try
  {
    if (pr_save_ne)
    {
      //определим, является ли PNL чисто цифровым (с учетом ZZ)
      bool pr_ne=false;
      for(iTotals=con.resa.begin();iTotals!=con.resa.end()&&!pr_ne;iTotals++)
        for(iPnrItem=iTotals->pnr.begin();iPnrItem!=iTotals->pnr.end()&&!pr_ne;iPnrItem++)
        {
          TPnrItem& pnr=*iPnrItem;
          for(iNameElement=pnr.ne.begin();iNameElement!=pnr.ne.end()&&!pr_ne;iNameElement++)
          {
            TNameElement& ne=*iNameElement;
            if (ne.surname!="ZZ") pr_ne=true;
          };
        };
      if (pr_ne)
      {
        //получим идентификатор транзакции
        Qry.Clear();
        Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
        Qry.Execute();
        tid=Qry.FieldAsInteger("tid");

        Qry.Clear();
        Qry.SQLText=
          "INSERT INTO crs_trips(point_id,crs,airline,flt_no,suffix,scd)\
           VALUES(:point_id,:crs,:airline,:flt_no,:suffix,:scd)";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.CreateVariable("crs",otString,crs);
        Qry.CreateVariable("airline",otString,info.flt.airline);
        Qry.CreateVariable("flt_no",otInteger,(int)info.flt.flt_no);
        Qry.CreateVariable("suffix",otString,info.flt.suffix);
        Qry.CreateVariable("scd",otDate,info.flt.scd);
        try
        {
          Qry.Execute();
        }
        catch(EOracleError E)
        {
          if (E.Code!=1) throw;
        };

        TQuery CrsPnrQry(&OraSession);
        CrsPnrQry.Clear();
        CrsPnrQry.SQLText=
          "SELECT crs_pnr.pnr_id FROM pnr_addrs,crs_pnr \
           WHERE crs_pnr.pnr_id=pnr_addrs.pnr_id(+) AND\
                 point_id= :point_id AND crs= :crs AND\
                 target= :target AND subclass= :subclass AND\
                 (pnr_addrs.airline IS NULL AND pnr_ref= :pnr_ref OR\
                  pnr_addrs.airline IS NOT NULL AND\
                  pnr_addrs.airline=:pnr_airline AND pnr_addrs.addr=:pnr_addr)";
        CrsPnrQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrQry.CreateVariable("crs",otString,crs);
        CrsPnrQry.DeclareVariable("target",otString);
        CrsPnrQry.DeclareVariable("subclass",otString);
        CrsPnrQry.DeclareVariable("pnr_ref",otString);
        CrsPnrQry.DeclareVariable("pnr_airline",otString);
        CrsPnrQry.DeclareVariable("pnr_addr",otString);

        TQuery CrsPnrInsQry(&OraSession);
        CrsPnrInsQry.Clear();
        CrsPnrInsQry.SQLText=
          "BEGIN \
             IF :pnr_id IS NULL THEN \
               SELECT crs_pnr__seq.nextval INTO :pnr_id FROM dual; \
               INSERT INTO crs_pnr(pnr_id,point_id,target,subclass,class,pnr_ref,grp_name,wl_priority,crs,tid) \
               VALUES(:pnr_id,:point_id,:target,:subclass,:class,:pnr_ref,:grp_name,:wl_priority,:crs,tid__seq.currval); \
             ELSE \
               UPDATE crs_pnr SET pnr_ref=NVL(:pnr_ref,pnr_ref), \
                                  grp_name=NVL(:grp_name,grp_name), \
                                  wl_priority=NVL(:wl_priority,wl_priority), \
                                  tid=tid__seq.currval \
               WHERE pnr_id= :pnr_id; \
             END IF; \
           END;";
        CrsPnrInsQry.CreateVariable("point_id",otInteger,point_id);
        CrsPnrInsQry.DeclareVariable("target",otString);
        CrsPnrInsQry.DeclareVariable("subclass",otString);
        CrsPnrInsQry.DeclareVariable("class",otString);
        CrsPnrInsQry.DeclareVariable("pnr_ref",otString);
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
                 DECODE(seats,0,0,1)=:seats AND tid<>:tid";
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
               SELECT pnl_id.nextval INTO :pax_id FROM dual;\
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

        TQuery CrsPaxRemQry(&OraSession);
        CrsPaxRemQry.Clear();
        CrsPaxRemQry.SQLText=
          "INSERT INTO crs_pax_rem(pax_id,rem,rem_code)\
           VALUES(:pax_id,:rem,:rem_code)";
        CrsPaxRemQry.DeclareVariable("pax_id",otInteger);
        CrsPaxRemQry.DeclareVariable("rem",otString);
        CrsPaxRemQry.DeclareVariable("rem_code",otString);

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
            //попробовать найти pnr_id по PNR reference
            for(iPnrAddr=pnr.addrs.begin();iPnrAddr!=pnr.addrs.end();iPnrAddr++)
            {
              CrsPnrQry.SetVariable("pnr_ref",iPnrAddr->addr);
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
            if (!pnr.addrs.empty())
              CrsPnrInsQry.SetVariable("pnr_ref",pnr.addrs.begin()->addr);
            else
              CrsPnrInsQry.SetVariable("pnr_ref",FNull);
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
              CrsPaxInsQry.SetVariable("surname",ne.surname);
              for(iPaxItem=ne.pax.begin();iPaxItem!=ne.pax.end();iPaxItem++)
              {
                CrsPaxInsQry.SetVariable("pax_id",FNull);
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
                    sprintf(buf,"%lu%c",iPaxItem->seat.row,iPaxItem->seat.line);
                    CrsPaxInsQry.SetVariable("seat_no",buf);
                  }
                  else
                    CrsPaxInsQry.SetVariable("seat_no",FNull);
                  CrsPaxInsQry.SetVariable("seat_type",iPaxItem->seat.rem);
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
                //ремарки
                if (ne.indicator==CHG||ne.indicator==DEL)
                {
                  //удаляем все ремарки этого пассажира
                  Qry.Clear();
                  Qry.SQLText="DELETE FROM crs_pax_rem WHERE pax_id= :pax_id";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.Execute();
                };
                if (ne.indicator!=DEL)
                {
                  CrsPaxRemQry.SetVariable("pax_id",pax_id);
                  for(iRemItem=iPaxItem->rem.begin();iRemItem!=iPaxItem->rem.end();iRemItem++)
                  {
                    if (iRemItem->text.empty()) continue;
                    if (iRemItem->text.size()>250) iRemItem->text.erase(250);
                    CrsPaxRemQry.SetVariable("rem",iRemItem->text);
                    CrsPaxRemQry.SetVariable("rem_code",iRemItem->code);
                    CrsPaxRemQry.Execute();
                  };
                  for(iRemItem=ne.rem.begin();iRemItem!=ne.rem.end();iRemItem++)
                  {
                    if (iRemItem->text.empty()) continue;
                    if (iRemItem->text.size()>250) iRemItem->text.erase(250);
                    CrsPaxRemQry.SetVariable("rem",iRemItem->text);
                    CrsPaxRemQry.SetVariable("rem_code",iRemItem->code);
                    CrsPaxRemQry.Execute();
                  };
                };

                if (!pr_sync_pnr)
                {
                  //делаем синхронизацию пассажира с розыском
                  Qry.Clear();
                  Qry.SQLText=
                    "BEGIN\
                       mvd.sync_crs_pax(:pax_id);\
                     END;";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.Execute();
                };
              };
              //запишем детей
              CrsPaxQry.SetVariable("seats",0);
              for(iInfItem=ne.inf.begin();iInfItem!=ne.inf.end();iInfItem++)
              {
                CrsPaxInsQry.SetVariable("pax_id",FNull);
                if (ne.indicator==CHG||ne.indicator==DEL)
                {
                  CrsPaxQry.SetVariable("surname",iInfItem->name);
                  CrsPaxQry.SetVariable("name",iInfItem->name);
                  CrsPaxQry.Execute();
                  if (CrsPaxQry.RowCount()>0)
                  {
                    pax_id=CrsPaxQry.FieldAsInteger("pax_id");
                    CrsPaxInsQry.SetVariable("pax_id",pax_id);
                    if (info.time_create<CrsPaxQry.FieldAsDateTime("last_op")) continue;
                  };
                };

                CrsPaxInsQry.SetVariable("surname",iInfItem->surname);
                CrsPaxInsQry.SetVariable("name",iInfItem->name);
                CrsPaxInsQry.SetVariable("pers_type",EncodePerson(baby));
                CrsPaxInsQry.SetVariable("seat_no",FNull);
                CrsPaxInsQry.SetVariable("seat_type",FNull);
                CrsPaxInsQry.SetVariable("seats",0);
                if (ne.indicator==DEL)
                  CrsPaxInsQry.SetVariable("pr_del",1);
                else
                  CrsPaxInsQry.SetVariable("pr_del",0);
                CrsPaxInsQry.SetVariable("last_op",info.time_create);
                CrsPaxInsQry.Execute();
                
                if (!pr_sync_pnr)
                {
                  //делаем синхронизацию пассажира с розыском
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

            //запишем стыковки
            if (!pnr.transfer.empty())
            {
              //удаляем нафиг все стыковки
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
                CrsTransferQry.SetVariable("airline",iTransfer->flt.airline);
                CrsTransferQry.SetVariable("flt_no",(int)iTransfer->flt.flt_no);
                CrsTransferQry.SetVariable("suffix",iTransfer->flt.suffix);
                CrsTransferQry.SetVariable("local_date",(int)iTransfer->local_date);
                CrsTransferQry.SetVariable("airp_dep",iTransfer->flt.brd_point);
                CrsTransferQry.SetVariable("airp_arv",iTransfer->arv_point);
                CrsTransferQry.SetVariable("subclass",iTransfer->subcl);
                CrsTransferQry.Execute();
                //проверка на существование в базе кодов авиакомпаний,аэропортов,подклассов
              /*  try
                {
                  GetAirline(iTransfer->flt.airline);
                }
                catch(ETlgError E)
                {
                  SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Transfer: %s",E.Message);
                };
                try
                {
                  if (iTransfer->flt.brd_point[0]!=0)
                    GetAirp(iTransfer->flt.brd_point);
                }
                catch(ETlgError E)
                {
                  SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Transfer: %s",E.Message);
                };
                try
                {
                  if (iTransfer->arv_point[0]!=0)
                    GetAirp(iTransfer->arv_point);
                }
                catch(ETlgError E)
                {
                  SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Transfer: %s",E.Message);
                };
                try
                {
                  GetClass(iTransfer->subcl);
                }
                catch(ETlgError E)
                {
                  SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Transfer: %s",E.Message);
                };*/
              };
            };
            if (pr_sync_pnr)
            {
              //делаем синхронизацию всей группы с розыском
              Qry.Clear();
              Qry.SQLText=
                "BEGIN\
                   mvd.sync_crs_pnr(:pnr_id);\
                 END;";
              Qry.CreateVariable("pnr_id",otInteger,pnr_id);
              Qry.Execute();
            };

    /*        //проверим, не пуста ли группа
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
        //разметим забронированные места в салоне 
        Qry.Clear();
        Qry.SQLText=
          "BEGIN\
             salons.initcomp(:point_id);\
           END;";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();
      };

      int pr_pnl_new=pr_pnl;
      if (strcmp(info.tlg_type,"PNL")==0)
      {
        //PNL
        if (pr_ne)
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
