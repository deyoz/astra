#include "checkin.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_context.h"
#include "stages.h"
#include "telegram.h"
#include "misc.h"
#include "astra_misc.h"
#include "base_tables.h"
#include "convert.h"
#include "tripinfo.h"
#include "aodb.h"
#include "tlg/tlg_parser.h"
#include "docs.h"
#include "stat.h"
#include "etick.h"
#include "events.h"
#include "term_version.h"
#include "baggage.h"
#include "passenger.h"
#include "remarks.h"
#include "jxtlib/jxt_cont.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace AstraLocale;

class OverloadException: public AstraLocale::UserException
{
  public:
    OverloadException(const std::string &msg):UserException(msg) {};
};

void CheckInInterface::LoadTagPacks(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //load tag packs
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT airline,target,tag_type,no,color FROM tag_packs WHERE desk=:desk";
  Qry.CreateVariable("desk",otString,TReqInfo::Instance()->desk.code);
  Qry.Execute();
  xmlNodePtr node,packNode;
  packNode=NewTextChild(resNode,"tag_packs");
  for(;!Qry.Eof;Qry.Next())
  {
    node=NewTextChild(packNode,"tag_pack");
    NewTextChild(node,"airline",Qry.FieldAsString("airline"));
    NewTextChild(node,"target",Qry.FieldAsString("target"));
    NewTextChild(node,"tag_type",Qry.FieldAsString("tag_type"));
    NewTextChild(node,"no",Qry.FieldAsFloat("no"));
    NewTextChild(node,"color",Qry.FieldAsString("color"));
  };
};

struct TInquiryFamily
{
  string surname,name;
  int n[NoPerson];
  char seats_prefix;
  int seats;
  void Clear()
  {
    surname.clear();
    name.clear();
    for(int i=0;i<NoPerson;i++) n[i]=-1;
    seats_prefix=' ';
    seats=-1;
  };
  TInquiryFamily()
  {
    Clear();
  };
};

struct TInquiryGroup
{
  char prefix;
  vector<TInquiryFamily> fams;
  bool digCkin,large;
  void Clear()
  {
    prefix=' ';
    fams.clear();
    digCkin=false;
    large=false;
  };
  TInquiryGroup()
  {
    Clear();
  };
};

void ParseInquiryStr(string query, TInquiryGroup &grp)
{
  TInquiryFamily fam;
  string strh;

  grp.Clear();
  try
  {
    int state=1;
    char c;
    int pos=1;
    string::iterator i=query.begin();
    for(;;)
    {
      if (i!=query.end()) c=*i; else c='\0';
      switch (state)
      {
        case 1:
          switch (c)
          {
            case '+': grp.prefix='+'; break;
            case '-': grp.prefix='-'; break;
            default:  state=2; continue;
          };
          state=2;
          break;
        case 2:
          fam.Clear();
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          };
          if (IsLetter(c) || c==' ' || c=='+' || c=='-' || c=='\0')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2: if (StrToInt(strh.c_str(),fam.n[adult])==EOF) throw pos-1;
                      break;
              case 3: if (StrToInt(strh.substr(0,1).c_str(),fam.n[adult])==EOF ||
                          StrToInt(strh.substr(1,1).c_str(),fam.n[child])==EOF ||
                          StrToInt(strh.substr(2,1).c_str(),fam.n[baby])==EOF)
                        throw pos-1;
                      break;
              case 4: if (StrToInt(strh.substr(0,2).c_str(),fam.n[adult])==EOF ||
                          StrToInt(strh.substr(2,1).c_str(),fam.n[child])==EOF ||
                          StrToInt(strh.substr(3,1).c_str(),fam.n[baby])==EOF)
                        throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            switch (c)
            {
              case '-':
              case '+':  fam.seats_prefix=c; state=3; break;
              case '\0': grp.fams.push_back(fam); state=5; break;
              default:   fam.surname.push_back(c); state=5; break;
            };
            break;
          };
          if (c=='/')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2:
              case 3: if (StrToInt(strh.c_str(),fam.n[adult])==EOF) throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            state=7;
            break;
          };
          throw pos;
        case 3:
          if (IsDigit(c))
          {
            strh=c;
            if (StrToInt(strh.c_str(),fam.seats)==EOF) throw pos;
            strh.clear();
            state=4;
            break;
          };
          throw pos;
        case 4:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            state=5;
            break;
          };
          if (c=='\0')
          {
            if (grp.fams.empty())
            {
              grp.fams.push_back(fam);
              state=5;
            }
            else throw pos;
            break;
          };
          throw pos;
        case 5:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            break;
          };
          if (c=='/' || c=='\0')
          {
            if (c=='/') state=6;
            TrimString(fam.surname);
            if (fam.surname.empty() && !grp.fams.empty()) throw pos;
            grp.fams.push_back(fam);
            fam.Clear();
            break;
          };
          throw pos;
        case 6:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          };
          if (IsLetter(c) || c==' ' || c=='+' || c=='-')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2: if (StrToInt(strh.c_str(),fam.n[adult])==EOF) throw pos-1;
                      break;
              case 3: if (StrToInt(strh.substr(0,1).c_str(),fam.n[adult])==EOF ||
                          StrToInt(strh.substr(1,1).c_str(),fam.n[child])==EOF ||
                          StrToInt(strh.substr(2,1).c_str(),fam.n[baby])==EOF)
                        throw pos-1;
                      break;
              case 4: if (StrToInt(strh.substr(0,2).c_str(),fam.n[adult])==EOF ||
                          StrToInt(strh.substr(2,1).c_str(),fam.n[child])==EOF ||
                          StrToInt(strh.substr(3,1).c_str(),fam.n[baby])==EOF)
                        throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            switch (c)
            {
              case '-':
              case '+': fam.seats_prefix=c; state=3; break;
              default:  fam.surname.push_back(c); state=5; break;
            };
            break;
          };
          throw pos;
        case 7:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          };
          if (c=='/')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2:
              case 3: if (StrToInt(strh.c_str(),fam.n[child])==EOF) throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            state=8;
            break;
          };
          throw pos;
        case 8:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          };
          if (c=='/')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2:
              case 3: if (StrToInt(strh.c_str(),fam.n[baby])==EOF) throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            state=9;
            break;
          };
          throw pos;
        case 9:
          if (IsDigit(c))
          {
            strh.push_back(c);
            state=10;
            break;
          };
        case 11:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            state=12;
            break;
          };
          if (c=='\0')
          {
            grp.fams.push_back(fam);
            state=12;
            break;
          };
          throw pos;
        case 10:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          };
          if (c=='/')
          {
            switch (strh.size())
            {
              case 0: break;
              case 1:
              case 2:
              case 3: if (StrToInt(strh.c_str(),fam.seats)==EOF) throw pos-1;
                      break;
              default: throw pos-1;
            };
            strh.clear();
            state=11;
            break;
          };
          throw pos;
        case 12:
          if (IsLetter(c) || c==' ')
          {
             fam.surname.push_back(c);
            break;
          };
          if (c=='\0')
          {
            TrimString(fam.surname);
            if (fam.surname.empty() && !grp.fams.empty()) throw pos;
            grp.fams.push_back(fam);
            break;
          };
          throw pos;
      };
      if (i==query.end()) break;
      i++;
      pos++;
    };

    if (i==query.end())
    {
      if (state==5 || state==12)
      {
        grp.large=(state==12);
        if (!grp.large)
          for(vector<TInquiryFamily>::iterator i=grp.fams.begin();i!=grp.fams.end();i++)
          {
          //разделим surname на surname и name
            TrimString(i->surname);
            string::size_type pos=i->surname.find(' ');
            if ( pos != string::npos)
            {
              i->name=i->surname.substr(pos);
              i->surname=i->surname.substr(0,pos);
            };
            TrimString(i->surname);
            TrimString(i->name);
            ProgTrace(TRACE5,"surname=%s name=%s adult=%d child=%d baby=%d seats=%d",
                             i->surname.c_str(),i->name.c_str(),i->n[adult],i->n[child],i->n[baby],i->seats);
          };
        if (grp.fams.size()==1 && grp.fams.begin()->surname.empty())
        {
          grp.fams.begin()->surname='X';
          grp.digCkin=true;
        };
      }
      else throw 0;
    }
    else throw pos;
  }
  catch(int pos)
  {
    throw UserException(pos+100,"MSG.ERROR_IN_REQUEST");
  };

};

struct TInquiryFamilySummary
{
  string surname,name;
  int n[NoPerson];
  int nPaxWithPlace,nPax;
  void Clear()
  {
    surname.clear();
    name.clear();
    for(int i=0;i<NoPerson;i++) n[i]=0;
    nPaxWithPlace=0;
    nPax=0;
  };
  TInquiryFamilySummary()
  {
    Clear();
  };
};

struct TInquiryGroupSummary
{
  int n[NoPerson];
  int nPaxWithPlace,nPax;
  vector<TInquiryFamilySummary> fams;
  int persCountFmt;

  void Clear()
  {
    for(int i=0;i<NoPerson;i++) n[i]=0;
    nPaxWithPlace=0;
    nPax=0;
    fams.clear();
    persCountFmt=0;
  };
  TInquiryGroupSummary()
  {
    Clear();
  };
};

struct TInquiryFormat
{
  int persCountFmt;
  int infSeatsFmt;
  TInquiryFormat()
  {
    persCountFmt=0;
    //0-кол-во по типам указыв. вначале каждой из фамилий
    //1-общее кол-во по типам указыв. вначале поисковой строки,
    //  если не указано хотя бы у одной из фамилий
    infSeatsFmt=0;
    //0-после '-' указывается кол-во РМ без мест из указанного кол-ва РМ
    //  после '+' указывается кол-во РМ без мест в добавление к кол-ву РМ c местами
    //1-после '-' указывается кол-во РМ c местами из указанного кол-ва РМ
    //  после '+' указывается кол-во РМ c местами в добавление к кол-ву РМ без мест
  };
};

void GetInquiryInfo(TInquiryGroup &grp, TInquiryFormat &fmt, TInquiryGroupSummary &sum)
{
  sum.Clear();
  if (grp.fams.empty()) throw UserException(100,"MSG.REQUEST.INVALID");
  vector<TInquiryFamily>::iterator i;
  if (!grp.large)
  {
    for(i=grp.fams.begin();i!=grp.fams.end();i++)
    {
      //проверим правильность цифровых данных перед каждой фамилией
      if (i==grp.fams.begin()) continue;
      if (i->n[adult]!=-1 || i->n[child]!=-1 || i->n[baby]!=-1 || i->seats!=-1)
      {
        fmt.persCountFmt=0; //внимание! изменяем формат
        break;
      };
    };
    sum.persCountFmt=fmt.persCountFmt;
  }
  else sum.persCountFmt=0;

  for(i=grp.fams.begin();i!=grp.fams.end();i++)
  {
    TInquiryFamilySummary info;

    if (i->surname.size()>64 || i->name.size()>64 )
      throw UserException(100,"MSG.PASSENGER.NAME_MORE_LENGTH");

    if (!grp.large)
    {
      info.surname=i->surname;
      info.name=i->name;

      if (sum.persCountFmt==1 && i!=grp.fams.begin())
      {
        sum.fams.push_back(info);
        continue;
      };
    };

    if (i->n[adult]==-1 && i->n[child]==-1 && i->n[baby]==-1 && i->seats==-1)
      info.n[adult]=1;
    else
    {
      for(int p=0;p<NoPerson;p++)
        if (i->n[p]!=-1) info.n[p]=i->n[p]; else info.n[p]=0;

    };
    if (!grp.large)
    {
      if (i->seats==-1)
      {
        if (fmt.infSeatsFmt==0)
        {
          info.nPax=info.n[adult]+info.n[child]+info.n[baby];
          info.nPaxWithPlace=info.nPax;
        }
        else
        {
          info.nPax=info.n[adult]+info.n[child]+info.n[baby];
          info.nPaxWithPlace=info.n[adult]+info.n[child];
        };
      }
      else
      {
        if (fmt.infSeatsFmt==0)
        {
          if (i->seats_prefix=='-')
          {
            if (info.n[baby]<i->seats)
              throw UserException(100,"MSG.CHECKIN.BABY_WO_SEATS_MORE_BABY_FOR_SURNAME");
            info.nPaxWithPlace=info.n[adult]+info.n[child]+info.n[baby]-i->seats;
          };
          if (i->seats_prefix=='+')
          {
            info.nPaxWithPlace=info.n[adult]+info.n[child]+info.n[baby];
            info.n[baby]+=i->seats;
          };
        }
        else
        {
          if (i->seats_prefix=='-')
          {
            if (info.n[baby]<i->seats)
              throw UserException(100,"MSG.CHECKIN.BABY_WITH_SEATS_MORE_COUNT_BABY_FOR_SURNAME");
            info.nPaxWithPlace=info.n[adult]+info.n[child]+i->seats;
          };
          if (i->seats_prefix=='+')
          {
            info.nPaxWithPlace=info.n[adult]+info.n[child]+i->seats;
            info.n[baby]+=i->seats;
          };
        };
        info.nPax=info.n[adult]+info.n[child]+info.n[baby];
      };
    }
    else
    {
      info.surname=i->surname;
      info.name=i->name;
      info.nPax=info.n[adult]+info.n[child]+info.n[baby];
      if (i->seats==-1)
        info.nPaxWithPlace=info.nPax;
      else
        info.nPaxWithPlace=i->seats;
      if (info.nPax<info.nPaxWithPlace)
        throw UserException(100,"MSG.CHECKIN.PASSENGERS_WITH_SEATS_MORE_TYPE_PASSENGERS");
      if (info.n[baby]<info.nPax-info.nPaxWithPlace)
        throw UserException(100,"MSG.CHECKIN.BABY_WO_SEATS_MORE_BABY");
    };
    for(int p=0;p<NoPerson;p++)
      sum.n[p]+=info.n[p];
    sum.nPax+=info.nPax;
    sum.nPaxWithPlace+=info.nPaxWithPlace;
    sum.fams.push_back(info);
  };
   if (grp.large && sum.nPaxWithPlace<=9) grp.large=false;
  if (sum.nPaxWithPlace==0)
    throw UserException(100,"MSG.CHECKIN.PASSENGERS_WITH_SEATS_MORE_ZERO");
  if (sum.nPax<sum.nPaxWithPlace)
    throw UserException(100,"MSG.CHECKIN.PASSENGERS_WITH_SEATS_MORE_TYPE_PASSENGERS");
  if (sum.nPax-sum.nPaxWithPlace>sum.n[adult])
    throw UserException(100,"MSG.CHECKIN.BABY_WO_SEATS_MORE_ADULT_FOR_GRP");

  if (sum.persCountFmt==1 && sum.nPax<(int)sum.fams.size())
    throw UserException(100,"MSG.CHECKIN.SURNAME_MORE_PASSENGERS_FOR_GRP");

  if (grp.prefix=='+')
    throw UserException(100,"MSG.REQUEST_ERROR.USE_STATUS_CHOICE");
};

void CreateNoRecResponse(TInquiryGroupSummary &sum, xmlNodePtr resNode)
{
  xmlNodePtr node,paxNode=NewTextChild(resNode,"norec");
  if (sum.persCountFmt==0)
  {
    for(vector<TInquiryFamilySummary>::iterator f=sum.fams.begin();f!=sum.fams.end();f++)
    {
      int n=1;
      for(int p=0;p<NoPerson;p++)
      {
        for(int i=0;i<f->n[p];i++,n++)
        {
          node=NewTextChild(paxNode,"pax");
          NewTextChild(node,"surname",f->surname,"");
          NewTextChild(node,"name",f->name,"");
          NewTextChild(node,"pers_type",EncodePerson((TPerson)p),EncodePerson(ASTRA::adult));
          if (n>f->nPaxWithPlace)
            NewTextChild(node,"seats",0);
        };
      };
    };
  }
  else
  {
    int n=1;
    vector<TInquiryFamilySummary>::iterator f=sum.fams.begin();
    for(int p=0;p<NoPerson;p++)
    {
      for(int i=0;i<sum.n[p];i++,n++)
      {
        node=NewTextChild(paxNode,"pax");
        if (f!=sum.fams.end())
        {
          NewTextChild(node,"surname",f->surname,"");
          NewTextChild(node,"name",f->name,"");
          f++;
        };
        NewTextChild(node,"pers_type",EncodePerson((TPerson)p),EncodePerson(ASTRA::adult));
        if (n>sum.nPaxWithPlace)
          NewTextChild(node,"seats",0);
      };
    };
  };
};

bool CheckTrferPermit(string airline_in, int flt_no_in,
                      string airp,
                      string airline_out, int flt_no_out,
                      bool outboard_trfer)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT trfer_set.pr_permit "
    "FROM trfer_set,trfer_set_airps,trfer_set_flts flts_in,trfer_set_flts flts_out "
    "WHERE trfer_set.id=trfer_set_airps.id AND "
    "      trfer_set_airps.id=flts_in.id(+) AND flts_in.pr_onward(+)=0 AND "
    "      trfer_set_airps.id=flts_out.id(+) AND flts_out.pr_onward(+)=1 AND "
    "      airline_in=:airline_in AND airp=:airp AND airline_out=:airline_out AND "
    "      (flts_in.flt_no IS NULL OR flts_in.flt_no=:flt_no_in) AND "
    "      (flts_out.flt_no IS NULL OR flts_out.flt_no=:flt_no_out) "
    "ORDER BY flts_out.flt_no,flts_in.flt_no ";
  Qry.CreateVariable("airline_in",otString,airline_in);
  Qry.CreateVariable("flt_no_in",otInteger,flt_no_in);
  Qry.CreateVariable("airp",otString,airp);
  Qry.CreateVariable("airline_out",otString,airline_out);
  Qry.CreateVariable("flt_no_out",otInteger,flt_no_out);
  Qry.Execute();
  if (Qry.Eof)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT id FROM trfer_set_airps "
      "WHERE (airline_in=:airline_in AND airline_out=:airline_out OR "
      "       airline_in=:airline_out AND airline_out=:airline_in) AND rownum<2";
    Qry.CreateVariable("airline_in",otString,airline_in);
    Qry.CreateVariable("airline_out",otString,airline_out);
    Qry.Execute();
    if (Qry.Eof)
      return outboard_trfer;
    else
      return false;
  };
  return (Qry.FieldAsInteger("pr_permit")!=0);
};

bool CheckInInterface::CheckTCkinPermit(const string &airline_in,
                                        const int flt_no_in,
                                        const string &airp,
                                        const string &airline_out,
                                        const int flt_no_out,
                                        TCkinSetsInfo &sets)
{
  sets.Clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT tckin_set.pr_permit,tckin_set.pr_waitlist,tckin_set.pr_norec "
    "FROM tckin_set,trfer_set_airps,trfer_set_flts flts_in,trfer_set_flts flts_out "
    "WHERE tckin_set.id=trfer_set_airps.id AND "
    "      trfer_set_airps.id=flts_in.id(+) AND flts_in.pr_onward(+)=0 AND "
    "      trfer_set_airps.id=flts_out.id(+) AND flts_out.pr_onward(+)=1 AND "
    "      airline_in=:airline_in AND airp=:airp AND airline_out=:airline_out AND "
    "      (flts_in.flt_no IS NULL OR flts_in.flt_no=:flt_no_in) AND "
    "      (flts_out.flt_no IS NULL OR flts_out.flt_no=:flt_no_out) "
    "ORDER BY flts_out.flt_no,flts_in.flt_no ";
  Qry.CreateVariable("airline_in",otString,airline_in);
  Qry.CreateVariable("flt_no_in",otInteger,flt_no_in);
  Qry.CreateVariable("airp",otString,airp);
  Qry.CreateVariable("airline_out",otString,airline_out);
  Qry.CreateVariable("flt_no_out",otInteger,flt_no_out);
  Qry.Execute();
  if (!Qry.Eof)
  {
    sets.pr_permit=Qry.FieldAsInteger("pr_permit")!=0;
    if (sets.pr_permit)
    {
      sets.pr_waitlist=Qry.FieldAsInteger("pr_waitlist")!=0;
      sets.pr_norec=Qry.FieldAsInteger("pr_norec")!=0;
    };
  };
  return (!Qry.Eof && Qry.FieldAsInteger("pr_permit")!=0);
};

struct TSearchResponseInfo //сейчас не используется, для развития!
{
  int resCountOk,resCountPAD;
};

void CheckInInterface::GetOnwardCrsTransfer(int pnr_id, TQuery &Qry, vector<TypeB::TTransferItem> &crs_trfer)
{
  crs_trfer.clear();
  const char* sql=
    "SELECT transfer_num,airline,flt_no,suffix, "
    "       local_date,airp_dep,airp_arv,subclass "
    "FROM crs_transfer "
    "WHERE pnr_id=:pnr_id AND transfer_num>0 "
    "ORDER BY transfer_num";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("pnr_id", otInteger);
  };
  Qry.SetVariable("pnr_id", pnr_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TypeB::TTransferItem trferItem;
    trferItem.num=Qry.FieldAsInteger("transfer_num");
    strcpy(trferItem.airline, Qry.FieldAsString("airline"));
    trferItem.flt_no=Qry.FieldAsInteger("flt_no");
    strcpy(trferItem.suffix, Qry.FieldAsString("suffix"));
    trferItem.local_date=Qry.FieldAsInteger("local_date");
    strcpy(trferItem.airp_dep, Qry.FieldAsString("airp_dep"));
    strcpy(trferItem.airp_arv, Qry.FieldAsString("airp_arv"));
    strcpy(trferItem.subcl, Qry.FieldAsString("subclass"));
    crs_trfer.push_back(trferItem) ;
  };
};

void CheckInInterface::LoadOnwardCrsTransfer(const TTripInfo &operFlt,
                                             const string &oper_airp_arv,
                                             const string &tlg_airp_dep,
                                             const vector<TypeB::TTransferItem> &crs_trfer, //содержит представления кодов из crs_transfer
                                             vector<CheckIn::TTransferItem> &trfer,
                                             xmlNodePtr trferNode)
{
  trfer.clear();
  if (crs_trfer.empty()) return;
  
  //данные рейса на прилет
  string airline_in=operFlt.airline;
  int flt_no_in=operFlt.flt_no;
  string airp_in=oper_airp_arv;
  //данные рейса на вылет
  string airline_out;
  int flt_no_out;
  string suffix_out;
  string airp_out;
  string subclass;

  string airline_view,suffix_view,airp_dep_view,airp_arv_view,subclass_view;

  bool without_trfer_set=true;
  bool outboard_trfer=true;
  
  if (trferNode!=NULL)
  {
    without_trfer_set=GetTripSets( tsIgnoreTrferSet, operFlt );
    outboard_trfer=GetTripSets( tsOutboardTrfer, operFlt );
  };

  bool pr_permit=true;   //этот тэг потом удалить потому как он не будет реально отрабатывать
  bool trfer_permit=true;
  bool vector_completed=false;
  int prior_transfer_num=0;
  TDateTime local_scd=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));
  
  for(vector<TypeB::TTransferItem>::const_iterator t=crs_trfer.begin();t!=crs_trfer.end();t++)
  {
    if (prior_transfer_num+1!=t->num && *(t->airp_dep)==0 || *(t->airp_arv)==0)
      //иными словами, не знаем порта прилета и предыдущией стыковки нет
      //либо же не знаем порта вылета
      //это ошибка в PNL/ADL - просто не отправим часть
      break;
      
    //проверим оформление трансфера
    TElemFmt fmt;
    airline_view=t->airline;
    airline_out=ElemToElemId(etAirline,airline_view,fmt);
    if (fmt!=efmtUnknown)
      airline_view=ElemIdToClientElem(etAirline,airline_out,fmt);
    else
      vector_completed=true;

    flt_no_out=t->flt_no;

    suffix_view=t->suffix;
    if (suffix_view.empty())
      suffix_out="";
    else
    {
      suffix_out=ElemToElemId(etSuffix,suffix_view,fmt);
      if (fmt!=efmtUnknown)
        suffix_view=ElemIdToClientElem(etSuffix,suffix_out,fmt);
      else
        vector_completed=true;
    };
    
    try
    {
      TDateTime base_date=local_scd-1; //патамушта можем из Японии лететь в Америку во вчерашний день
      local_scd=DayToDate(t->local_date,base_date,false); //локальная дата вылета
    }
    catch(EXCEPTIONS::EConvertError &E)
    {
      vector_completed=true;
    };

    airp_dep_view=t->airp_dep;
    if (airp_dep_view.empty())
    {
      airp_out=airp_in;
    }
    else
    {
      airp_out=ElemToElemId(etAirp,airp_dep_view,fmt);
      if (fmt!=efmtUnknown)
        airp_dep_view=ElemIdToClientElem(etAirp,airp_out,fmt);
      else
        vector_completed=true;
    };

    if (trferNode!=NULL && !without_trfer_set)
    {
      //ProgTrace(TRACE5, "CreateSearchResponse: airline_in=%s flt_no_id=%d airp_out=%s airline_out=%s flt_no_out=%d outboard_trfer=%d",
      //                  airline_in.c_str(), flt_no_in, airp_out.c_str(), airline_out.c_str(), flt_no_out, (int)outboard_trfer);
      if (prior_transfer_num+1==t->num &&
          (airline_in.empty() || airp_out.empty() || airline_out.empty() ||
          !CheckTrferPermit(airline_in,flt_no_in,airp_out,airline_out,flt_no_out,outboard_trfer)))
      {
        trfer_permit=false;
        pr_permit=false;
      };
    };

    airp_arv_view=t->airp_arv;
    airp_in=ElemToElemId(etAirp,airp_arv_view,fmt);
    if (fmt!=efmtUnknown)
      airp_arv_view=ElemIdToClientElem(etAirp,airp_in,fmt);
    else
      vector_completed=true;

    subclass_view=t->subcl;
    subclass=ElemToElemId(etSubcls,subclass_view,fmt);
    if (fmt!=efmtUnknown)
      subclass_view=ElemIdToClientElem(etSubcls,subclass,fmt);
    else
      vector_completed=true;

    if (!tlg_airp_dep.empty() && trferNode!=NULL && without_trfer_set && tlg_airp_dep==airp_in)
    {
      //анализируем замкнутый маршрут только если игнорируются настройки трансфера
      //(то бишь в городах)
      pr_permit=false;
      trfer_permit=false;
    };

    if (trferNode!=NULL)
    {
      xmlNodePtr node2=NewTextChild(trferNode,"segment");
      NewTextChild(node2,"num",(int)t->num);
      NewTextChild(node2,"airline",airline_view);
      NewTextChild(node2,"flt_no",flt_no_out);
      NewTextChild(node2,"suffix",suffix_view,"");
      NewTextChild(node2,"local_date",(int)t->local_date);
      NewTextChild(node2,"airp_dep",airp_dep_view,"");
      NewTextChild(node2,"airp_arv",airp_arv_view);
      NewTextChild(node2,"subclass",subclass_view);
      NewTextChild(node2,"pr_permit",(int)pr_permit);
      NewTextChild(node2,"trfer_permit",(int)trfer_permit);
    };

    if (!vector_completed)
    {
      CheckIn::TTransferItem trferItem;
      trferItem.operFlt.airline=airline_out;
      trferItem.operFlt.flt_no=flt_no_out;
      trferItem.operFlt.suffix=suffix_out;
      trferItem.operFlt.scd_out=local_scd;
      trferItem.operFlt.airp=airp_out;
      trferItem.airp_arv=airp_in;
      trferItem.subclass=subclass;
      trfer.push_back(trferItem);
    };

    prior_transfer_num=t->num;
    trfer_permit=true; //для следующей итерации
    airline_in=airline_out;
    flt_no_in=flt_no_out;
  };
};


bool EqualCrsTransfer(const vector<TypeB::TTransferItem> &trfer1,
                      const vector<TypeB::TTransferItem> &trfer2)
{
  vector<TypeB::TTransferItem>::const_iterator i1=trfer1.begin();
  vector<TypeB::TTransferItem>::const_iterator i2=trfer2.begin();
  for(;i1!=trfer1.end() && i2!=trfer2.end(); i1++,i2++)
  {
    if (i1->num!=i2->num ||
        strcmp(i1->airline,i2->airline)!=0 ||
        i1->flt_no!=i2->flt_no ||
        strcmp(i1->suffix,i2->suffix)!=0 ||
        i1->local_date!=i2->local_date ||
        strcmp(i1->airp_dep,i2->airp_dep)!=0 ||
        strcmp(i1->airp_arv,i2->airp_arv)!=0 ||
        strcmp(i1->subcl,i2->subcl)!=0) return false;
  };
  
  return i1==trfer1.end() && i2==trfer2.end();
};

void LoadUnconfirmedTransfer(const vector<CheckIn::TTransferItem> &segs, xmlNodePtr transferNode)
{
  if (segs.empty() || transferNode==NULL) return;

  const CheckIn::TTransferItem &firstSeg=*segs.begin();
  
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
   "SELECT crs_pnr.pnr_id, tlg_trips.airp_dep AS tlg_airp_dep, crs_pax.pax_id "
   "FROM pax,crs_pax,crs_pnr,tlg_trips "
   "WHERE crs_pax.pax_id=pax.pax_id AND "
   "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
   "      tlg_trips.point_id=crs_pnr.point_id AND "
   "      crs_pax.pr_del=0 AND "
   "      pax.grp_id=:grp_id "
   "ORDER BY crs_pnr.pnr_id";
  PaxQry.CreateVariable("grp_id", otInteger, firstSeg.grp_id);
  PaxQry.Execute();
  
  TQuery TrferQry(&OraSession);
  
  vector< pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > > crs_trfer, trfer; //вектор пар <tlg_airp_dep+трансферный маршрут, вектор ид. пассажиров>
  
  int pnr_id=NoExists;
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      pnr_id=PaxQry.FieldAsInteger("pnr_id");

      crs_trfer.push_back( make_pair( make_pair( string(), vector<TypeB::TTransferItem>() ), vector<int>() ) );

      pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();

      last_crs_trfer.first.first=PaxQry.FieldAsString("tlg_airp_dep");
      CheckInInterface::GetOnwardCrsTransfer(pnr_id, TrferQry, last_crs_trfer.first.second); //зачитаем из таблицы crs_transfer
    };

    if (crs_trfer.empty()) continue;
    pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();
    last_crs_trfer.second.push_back(PaxQry.FieldAsInteger("pax_id"));
  };
  
  ProgTrace(TRACE5,"LoadUnconfirmedTransfer: crs_trfer - step 1");
  vector< pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > >::const_iterator iCrsTrfer=crs_trfer.begin();
  for(;iCrsTrfer!=crs_trfer.end();iCrsTrfer++)
  {
    ProgTrace(TRACE5,"tlg_airp_arv=%s vector<TypeB::TTransferItem>.size()=%d vector<int>.size()=%d",
                     iCrsTrfer->first.first.c_str(), iCrsTrfer->first.second.size(), iCrsTrfer->second.size());
  };
    
  //пробег по пассажирам первого сегмента
  int pax_no=0;
  int iYear,iMonth,iDay;
  for(vector<CheckIn::TPaxTransferItem>::const_iterator p=firstSeg.pax.begin();p!=firstSeg.pax.end();p++,pax_no++)
  {
    string tlg_airp_dep=firstSeg.operFlt.airp;
    vector<TypeB::TTransferItem> pax_trfer;
    //набираем вектор трансфера
    int seg_no=1;
    for(vector<CheckIn::TTransferItem>::const_iterator s=segs.begin();s!=segs.end();s++,seg_no++)
    {
      if (s==segs.begin()) continue; //первый сегмент отбрасываем
      TypeB::TTransferItem trferItem;
      trferItem.num=seg_no-1;
      strcpy(trferItem.airline,s->operFlt.airline.c_str());
      trferItem.flt_no=s->operFlt.flt_no;
      strcpy(trferItem.suffix,s->operFlt.suffix.c_str());
      DecodeDate(s->operFlt.scd_out,iYear,iMonth,iDay);
      trferItem.local_date=iDay;
      strcpy(trferItem.airp_dep,s->operFlt.airp.c_str());
      strcpy(trferItem.airp_arv,s->airp_arv.c_str());
      strcpy(trferItem.subcl,s->pax.at(pax_no).subclass.c_str());
      pax_trfer.push_back(trferItem);
    };
    ProgTrace(TRACE5,"LoadUnconfirmedTransfer: pax_trfer.size()=%d",pax_trfer.size());
    //теперь pax_trfer содержит сегменты сквозной регистрации с подклассом пассажира
    //попробуем добавить сегменты из crs_transfer
    vector< pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > >::const_iterator iCrsTrfer=crs_trfer.begin();
    for(;iCrsTrfer!=crs_trfer.end();iCrsTrfer++)
    {
      if (find(iCrsTrfer->second.begin(),iCrsTrfer->second.end(),p->pax_id)!=iCrsTrfer->second.end())
      {
        //iCrsTrfer указывает на трансфер пассажира
        tlg_airp_dep=iCrsTrfer->first.first;
        for(vector<TypeB::TTransferItem>::const_iterator iCrsTrferItem=iCrsTrfer->first.second.begin();
                                                         iCrsTrferItem!=iCrsTrfer->first.second.end();iCrsTrferItem++)
        {
          if (iCrsTrferItem->num>=seg_no-1) pax_trfer.push_back(*iCrsTrferItem); //добавляем доп. сегменты из таблицы crs_transfer
        };
        break;
      };
    };
    ProgTrace(TRACE5,"LoadUnconfirmedTransfer: pax_trfer.size()=%d",pax_trfer.size());
    //теперь pax_trfer содержит сегменты сквозной регистрации с подклассом пассажира
    //плюс дополнительные сегменты трансфера из таблицы crs_transfer
    //все TTransferItem в pax_trfer сортированы по номеру трансфера (TTransferItem.num)
    vector< pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > >::iterator iTrfer=trfer.begin();
    for(;iTrfer!=trfer.end();iTrfer++)
    {
      if (iTrfer->first.first==tlg_airp_dep &&
          EqualCrsTransfer(iTrfer->first.second,pax_trfer)) break;
    };
    if (iTrfer!=trfer.end())
      //нашли тот же маршрут
      iTrfer->second.push_back(p->pax_id);
    else
      trfer.push_back( make_pair( make_pair( tlg_airp_dep, pax_trfer ), vector<int>(1,p->pax_id) ) );
  };
  //формируем XML
  xmlNodePtr itemsNode=NewTextChild(transferNode,"unconfirmed_transfer");
  
  vector< pair< pair< string, vector<TypeB::TTransferItem> >, vector<int> > >::const_iterator iTrfer=trfer.begin();
  for(;iTrfer!=trfer.end();iTrfer++)
  {
    if (iTrfer->first.second.empty()) continue;
    xmlNodePtr itemNode=NewTextChild(itemsNode,"item");
    
    xmlNodePtr trferNode=NewTextChild(itemNode,"transfer");
    vector<CheckIn::TTransferItem> dummy;
    CheckInInterface::LoadOnwardCrsTransfer(firstSeg.operFlt,
                                            firstSeg.airp_arv,
                                            iTrfer->first.first,
                                            iTrfer->first.second,
                                            dummy, trferNode);
    xmlNodePtr paxNode=NewTextChild(itemNode,"passengers");
    for(vector<int>::const_iterator pax_id=iTrfer->second.begin();
                                    pax_id!=iTrfer->second.end();pax_id++)
      NewTextChild( NewTextChild(paxNode, "pax"),"pax_id", *pax_id);
  };
};

int CreateSearchResponse(int point_dep, TQuery &PaxQry,  xmlNodePtr resNode)
{
  TQuery FltQry(&OraSession);
  FltQry.Clear();
  FltQry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out FROM points "
    "WHERE point_id=:point_id AND pr_del>=0 AND pr_reg<>0";
  FltQry.CreateVariable("point_id",otInteger,point_dep);
  FltQry.Execute();
  if (FltQry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  TTripInfo operFlt(FltQry);

  FltQry.Clear();
  FltQry.SQLText=
    "SELECT airline,flt_no,suffix,airp_dep AS airp,TRUNC(scd) AS scd_out "
    "FROM tlg_trips WHERE point_id=:point_id";
  FltQry.DeclareVariable("point_id",otInteger);

  TQuery TrferQry(&OraSession);

  TQuery PnrAddrQry(&OraSession);
  PnrAddrQry.SQLText =
    "SELECT airline,addr FROM pnr_addrs WHERE pnr_id=:pnr_id";
  PnrAddrQry.DeclareVariable("pnr_id",otInteger);

  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem_code,rem FROM crs_pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable("pax_id",otInteger);
  
  TQuery PaxDocQry(&OraSession);
  PaxDocQry.SQLText =
    "SELECT type, issue_country, no, nationality, birth_date, gender, expiry_date, "
    "       surname, first_name, second_name, pr_multi "
    "FROM crs_pax_doc "
    "WHERE pax_id=:pax_id AND no IS NOT NULL "
    "ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no ";
  PaxDocQry.DeclareVariable("pax_id",otInteger);
  
  TQuery PaxDocoQry(&OraSession);
  PaxDocoQry.SQLText =
    "SELECT birth_place, type, no, issue_place, issue_date, NULL AS expiry_date, applic_country, pr_inf "
    "FROM crs_pax_doco "
    "WHERE pax_id=:pax_id AND rem_code='DOCO' AND type='V' "
    "ORDER BY no ";
  PaxDocoQry.DeclareVariable("pax_id",otInteger);
  
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  TQuery GetPSPT2Qry(&OraSession);
  GetPSPT2Qry.SQLText =
    "SELECT report.get_PSPT2(:pax_id) AS no FROM dual";
  GetPSPT2Qry.DeclareVariable("pax_id",otInteger);

  int point_id=-1;
  int pnr_id=-1, pax_id;
  xmlNodePtr tripNode,pnrNode,paxNode,node;

  TMktFlight mktFlt;
  TTripInfo tlgTripsFlt;
  TCodeShareSets codeshareSets;

  int count=0;
  tripNode=GetNode("trips",resNode);
  if (tripNode==NULL)
    tripNode=NewTextChild(resNode,"trips");
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    count++;
    if (PaxQry.FieldAsInteger("point_id")!=point_id)
    {
      node=NewTextChild(tripNode,"trip");
      point_id=PaxQry.FieldAsInteger("point_id");
      FltQry.SetVariable("point_id",point_id);
      FltQry.Execute();
      if (FltQry.Eof)
        throw EXCEPTIONS::Exception("Flight not found in tlg_trips (point_id=%d)",point_id);
      tlgTripsFlt.Init(FltQry);

      NewTextChild(node,"point_id",point_id);
      NewTextChild(node,"airline",tlgTripsFlt.airline);
      NewTextChild(node,"flt_no",tlgTripsFlt.flt_no);
      NewTextChild(node,"suffix",tlgTripsFlt.suffix,"");
      NewTextChild(node,"scd",DateTimeToStr(tlgTripsFlt.scd_out));
      NewTextChild(node,"airp_dep",tlgTripsFlt.airp);

      TDateTime local_scd=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));
      modf(local_scd,&local_scd); //обрубаем часы
      if (operFlt.airline!=tlgTripsFlt.airline ||
          operFlt.flt_no!=tlgTripsFlt.flt_no ||
          operFlt.suffix!=tlgTripsFlt.suffix ||
          operFlt.airp!=tlgTripsFlt.airp ||
          local_scd!=tlgTripsFlt.scd_out)
      {
        codeshareSets.get(operFlt,tlgTripsFlt);
        NewTextChild(node,"pr_mark_norms",(int)codeshareSets.pr_mark_norms,(int)false);
      };

      pnrNode=NewTextChild(node,"groups");
    };
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      node=NewTextChild(pnrNode,"pnr");
      pnr_id=PaxQry.FieldAsInteger("pnr_id");
      NewTextChild(node,"pnr_id",pnr_id);
      NewTextChild(node,"airp_arv",PaxQry.FieldAsString("airp_arv"));
      NewTextChild(node,"subclass",PaxQry.FieldAsString("subclass"));
      NewTextChild(node,"class",PaxQry.FieldAsString("class"));
      string pnr_status=PaxQry.FieldAsString("pnr_status");
      NewTextChild(node,"pnr_status",pnr_status,"");
      NewTextChild(node,"pnr_priority",PaxQry.FieldAsString("pnr_priority"),"");
      NewTextChild(node,"pad",(int)(pnr_status=="DG2"||
                                    pnr_status=="RG2"||
                                    pnr_status=="ID2"||
                                    pnr_status=="WL"),0);
      mktFlt.getByPnrId(pnr_id);
      if (!mktFlt.IsNULL())
      {
        TTripInfo pnrMarkFlt;
        pnrMarkFlt.airline=mktFlt.airline;
        pnrMarkFlt.flt_no=mktFlt.flt_no;
        pnrMarkFlt.suffix=mktFlt.suffix;
        pnrMarkFlt.airp=mktFlt.airp_dep;
        pnrMarkFlt.scd_out=mktFlt.scd_date_local;

        if (pnrMarkFlt.airline!=tlgTripsFlt.airline ||
            pnrMarkFlt.flt_no!=tlgTripsFlt.flt_no ||
            pnrMarkFlt.suffix!=tlgTripsFlt.suffix ||
            pnrMarkFlt.airp!=tlgTripsFlt.airp ||
            pnrMarkFlt.scd_out!=tlgTripsFlt.scd_out)
        {
          //фактический не совпадает с коммерческим
          xmlNodePtr markFltNode=NewTextChild(node,"mark_flight");
          NewTextChild(markFltNode,"airline",pnrMarkFlt.airline,tlgTripsFlt.airline);
          NewTextChild(markFltNode,"flt_no",pnrMarkFlt.flt_no,tlgTripsFlt.flt_no);
          NewTextChild(markFltNode,"suffix",pnrMarkFlt.suffix,tlgTripsFlt.suffix);
          NewTextChild(markFltNode,"scd",DateTimeToStr(pnrMarkFlt.scd_out),DateTimeToStr(tlgTripsFlt.scd_out));
          NewTextChild(markFltNode,"airp_dep",pnrMarkFlt.airp,tlgTripsFlt.airp);
          codeshareSets.get(operFlt,pnrMarkFlt);
          NewTextChild(markFltNode,"pr_mark_norms",(int)codeshareSets.pr_mark_norms,(int)false);
        };
      };

      paxNode=NewTextChild(node,"passengers");
      
      vector<TypeB::TTransferItem> crs_trfer;
      CheckInInterface::GetOnwardCrsTransfer(pnr_id, TrferQry, crs_trfer);

      if (!crs_trfer.empty())
      {
        xmlNodePtr trferNode=NewTextChild(node,"transfer");
        vector<CheckIn::TTransferItem> dummy;
        CheckInInterface::LoadOnwardCrsTransfer(operFlt,
                                                PaxQry.FieldAsString("airp_arv"),
                                                tlgTripsFlt.airp,
                                                crs_trfer, dummy, trferNode);
      };

      PnrAddrQry.SetVariable("pnr_id",pnr_id);
      PnrAddrQry.Execute();
      if (!PnrAddrQry.Eof)
      {
        string airline=PnrAddrQry.FieldAsString("airline");
        string addr=PnrAddrQry.FieldAsString("addr");
        PnrAddrQry.Next();

        xmlNodePtr pnrAddrNode,node2;
        if (!PnrAddrQry.Eof)
          pnrAddrNode=NewTextChild(node,"pnr_addrs");
        else
          pnrAddrNode=node;
        if (!PnrAddrQry.Eof||airline!=tlgTripsFlt.airline)
        {
          node2=NewTextChild(pnrAddrNode,"pnr_addr");
          NewTextChild(node2,"airline",airline);
          NewTextChild(node2,"addr",addr);
        }
        else
          NewTextChild(pnrAddrNode,"pnr_addr",addr);

        for(;!PnrAddrQry.Eof;PnrAddrQry.Next())
        {
          node2=NewTextChild(pnrAddrNode,"pnr_addr");
          NewTextChild(node2,"airline",PnrAddrQry.FieldAsString("airline"));
          NewTextChild(node2,"addr",PnrAddrQry.FieldAsString("addr"));
        };
      };
    };
    node=NewTextChild(paxNode,"pax");
    pax_id=PaxQry.FieldAsInteger("pax_id");
    NewTextChild(node,"pax_id",pax_id);
    NewTextChild(node,"surname",PaxQry.FieldAsString("surname"));
    NewTextChild(node,"name",PaxQry.FieldAsString("name"),"");
    NewTextChild(node,"pers_type",PaxQry.FieldAsString("pers_type"),EncodePerson(ASTRA::adult));
    NewTextChild(node,"seat_no",PaxQry.FieldAsString("seat_no"),"");
    NewTextChild(node,"seat_type",PaxQry.FieldAsString("seat_type"),"");
    NewTextChild(node,"seats",PaxQry.FieldAsInteger("seats"),1);
    //обработка документов
    PaxDocQry.SetVariable("pax_id", pax_id);
    PaxDocQry.Execute();
    if (!PaxDocQry.Eof)
    {
      if (TReqInfo::Instance()->desk.compatible(DOCS_VERSION))
      {
        CheckIn::LoadPaxDoc(PaxDocQry, node);
      }
      else
      {
        NewTextChild(node, "document", PaxDocQry.FieldAsString("no"), "");
      };
    }
    else
    {
      GetPSPT2Qry.SetVariable("pax_id", pax_id);
      GetPSPT2Qry.Execute();
      if (!GetPSPT2Qry.Eof && !GetPSPT2Qry.FieldIsNULL("no"))
      {
        if (reqInfo->desk.compatible(DOCS_VERSION))
        {
          xmlNodePtr docNode=NewTextChild(node,"document");
          NewTextChild(docNode, "no", GetPSPT2Qry.FieldAsString("no"), "");
        }
        else
          NewTextChild(node, "document", GetPSPT2Qry.FieldAsString("no"), "");
      };
    };
    //обработка виз
    if (TReqInfo::Instance()->desk.compatible(DOCS_VERSION))
    {
      PaxDocoQry.SetVariable("pax_id", pax_id);
      PaxDocoQry.Execute();
      if (!PaxDocoQry.Eof)
        CheckIn::LoadPaxDoco(PaxDocoQry, node);
    };
    //обработка билетов
    string ticket_no;
    if (!PaxQry.FieldIsNULL("eticket"))
    {
      //билет TKNE
      ticket_no=PaxQry.FieldAsString("eticket");
      int coupon_no=0;
      string::size_type pos=ticket_no.find_last_of('/');
      if (pos!=string::npos)
      {
        if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
            coupon_no>=1 && coupon_no<=4)
          ticket_no.erase(pos);
        else
          coupon_no=0;
      };

      NewTextChild(node,"ticket_no",ticket_no);
      NewTextChild(node,"coupon_no",coupon_no,0);
      NewTextChild(node,"ticket_rem","TKNE");
    }
    else
    {
      if (!PaxQry.FieldIsNULL("ticket"))
      {
        //билет TKNA
        ticket_no=PaxQry.FieldAsString("ticket");
        NewTextChild(node,"ticket_no",ticket_no);
        NewTextChild(node,"ticket_rem","TKNA");
      };
    };

    RemQry.SetVariable("pax_id",pax_id);
    RemQry.Execute();
    xmlNodePtr remsNode=NULL;
    for(;!RemQry.Eof;RemQry.Next())
    {
      const char* rem_code=RemQry.FieldAsString("rem_code");
      const char* rem_text=RemQry.FieldAsString("rem");
      if (isDisabledRem(rem_code, rem_text)) continue;
      
      if (remsNode==NULL) remsNode=NewTextChild(node,"rems");
      xmlNodePtr remNode=NewTextChild(remsNode,"rem");
      NewTextChild(remNode,"rem_code",rem_code,"");
      NewTextChild(remNode,"rem_text",rem_text);
    };
  };
  return count;
};

void CheckInInterface::SearchGrp(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep = NodeAsInteger("point_dep",reqNode);
  readTripCounters(point_dep,resNode);

  TInquiryFamilySummary fam;
  xmlNodePtr node=NodeAsNode("inquiry_summary",reqNode);
  fam.nPax=NodeAsInteger("count",node);
  fam.nPaxWithPlace=NodeAsInteger("seats",node);
  fam.n[adult]=NodeAsInteger("adult",node);
  fam.n[child]=NodeAsInteger("child",node);
  fam.n[baby]=NodeAsInteger("infant",node);
  fam.surname=NodeAsString("grp_name",node);

  TInquiryGroupSummary sum;
  sum.nPax=fam.nPax;
  sum.nPaxWithPlace=fam.nPaxWithPlace;
  sum.n[adult]=fam.n[adult];
  sum.n[child]=fam.n[child];
  sum.n[baby]=fam.n[baby];
  sum.persCountFmt=0;
  sum.fams.push_back(fam);

  if (GetNode("pnr_id",reqNode)!=NULL)
  {
    TQuery PaxQry(&OraSession);
    PaxQry.Clear();
    PaxQry.SQLText=
      "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv,crs_pnr.subclass, "
      "       crs_pnr.class, crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, "
      "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket "
      "FROM crs_pnr,crs_pax,pax "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pnr.pnr_id=:pnr_id AND "
      "      crs_pax.pr_del=0 AND "
      "      pax.pax_id IS NULL "
      "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";
    PaxQry.CreateVariable("pnr_id",otInteger,NodeAsInteger("pnr_id",reqNode));
    PaxQry.Execute();
    CreateSearchResponse(point_dep,PaxQry,resNode);
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","Choice");
    return;
  };
  CreateNoRecResponse(sum,resNode);
  NewTextChild(resNode,"ckin_state","BeforeReg");
  return;
};

string CheckInInterface::GetSearchPaxSubquery(TPaxStatus pax_status,
                                              bool return_pnr_ids,
                                              bool exclude_checked,
                                              bool exclude_deleted,
                                              bool select_pad_with_ok,
                                              string sql_filter)
{
  ostringstream sql;
  string status_param;
  switch (pax_status)
  {
    case psTransit: status_param=":ps_transit"; break;
     case psGoshow: status_param=":ps_goshow";  break;
           default: status_param=":ps_ok";      break;
  };

  //2 прохода:
  for(int pass=1;pass<=2;pass++)
  {
    if (pass==2 && pax_status!=psGoshow) continue;
    if (pass==2)
      sql << "   UNION \n";

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status \n";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status \n";

    sql <<   "   FROM crs_pnr";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << ", crs_pax";

    if (exclude_checked)
      sql << ", pax";

    sql <<   ", \n";

    sql <<   "    (SELECT b2.point_id_tlg, \n"
             "            airp_arv_tlg,class_tlg,status \n"
             "     FROM crs_displace2,tlg_binding b1,tlg_binding b2 \n"
             "     WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND \n"
             "           b1.point_id_spp=b2.point_id_spp AND \n"
             "           crs_displace2.point_id_spp=:point_id AND \n"
             "           b1.point_id_spp<>:point_id) crs_displace \n"
             "   WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND \n"
             "         crs_pnr.system='CRS' AND \n"
             "         crs_pnr.airp_arv=crs_displace.airp_arv_tlg AND \n"
             "         crs_pnr.class=crs_displace.class_tlg \n";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << "         AND crs_pnr.pnr_id=crs_pax.pnr_id \n";

    if (pass==1)
      sql << "         AND crs_displace.status= " << status_param << " \n";

    if (pass==1 && pax_status==psCheckin && !select_pad_with_ok)
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) \n";

    if (pass==2 && pax_status==psGoshow)
      sql << "         AND crs_displace.status= :ps_ok \n"
             "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') \n";

    if (exclude_checked)
      sql << "         AND crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL \n";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 \n";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") \n";

  };

  //2 прохода:
  for(int pass=1;pass<=2;pass++)
  {
    if (pass==1 && pax_status!=psCheckin && pax_status!=psGoshow ||
        pass==2 && pax_status==psCheckin) continue;
    if (pass==1)
      sql << "   UNION \n";
    else
      sql << "   MINUS \n";

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status \n";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status \n";

    sql <<   "   FROM crs_pnr, tlg_binding";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << ", crs_pax";

    if (exclude_checked)
      sql << ", pax \n";
    else
      sql << " \n";

    sql   << "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND \n"
             "         crs_pnr.system='CRS' AND \n"
             "         tlg_binding.point_id_spp= :point_id \n";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << "         AND crs_pnr.pnr_id=crs_pax.pnr_id \n";

    if (pass==1 && pax_status==psCheckin && !select_pad_with_ok ||
        pass==2 && pax_status==psGoshow)
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) \n";

    if (pass==1 && pax_status==psGoshow)
      sql << "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') \n";

    if (exclude_checked)
      sql << "         AND crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL \n";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 \n";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") \n";
  };
  return sql.str();
};





void CheckInInterface::SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep = NodeAsInteger("point_dep",reqNode);
  TPaxStatus pax_status = DecodePaxStatus(NodeAsString("pax_status",reqNode));
  string query= NodeAsString("query",reqNode);
  bool pr_unaccomp=query=="0";
  bool charter_search=false;

  TInquiryGroup grp;
  TInquiryGroupSummary sum;
  TQuery Qry(&OraSession);
  if (!pr_unaccomp)
  {
    readTripCounters(point_dep,resNode);

    ParseInquiryStr(query,grp);
    TInquiryFormat fmt;
    fmt.persCountFmt=0;
    fmt.infSeatsFmt=0;

    Qry.Clear();
    Qry.SQLText=
      "SELECT pers_count_fmt,inf_seats_fmt "
      "FROM desks,desk_grp_sets "
      "WHERE desks.grp_id=desk_grp_sets.grp_id AND desks.code=:desk ";
    Qry.CreateVariable("desk",otString,TReqInfo::Instance()->desk.code);
    Qry.Execute();
    if (!Qry.Eof)
    {
      if (!Qry.FieldIsNULL("pers_count_fmt"))
      {
        fmt.persCountFmt=Qry.FieldAsInteger("pers_count_fmt");
        if (fmt.persCountFmt!=0) fmt.persCountFmt=1;
      };
      if (!Qry.FieldIsNULL("inf_seats_fmt"))
      {
        fmt.infSeatsFmt=Qry.FieldAsInteger("inf_seats_fmt");
        if (fmt.infSeatsFmt!=0) fmt.infSeatsFmt=1;
      };
    };
    Qry.Clear();
    Qry.SQLText=
      "SELECT airline,flt_no,suffix,airp,scd_out "
      "FROM points WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable("point_id",otInteger,point_dep);
    Qry.Execute();
    if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    TTripInfo fltInfo(Qry);
    charter_search=GetTripSets(tsCharterSearch,fltInfo);

    GetInquiryInfo(grp,fmt,sum);
  };

  xmlNodePtr node;
  node=NewTextChild(resNode,"inquiry_summary");
  NewTextChild(node,"count",sum.nPax);
  NewTextChild(node,"seats",sum.nPaxWithPlace);
  NewTextChild(node,"adult",sum.n[adult]);
  NewTextChild(node,"child",sum.n[child]);
  NewTextChild(node,"infant",sum.n[baby]);
  if (grp.large && !sum.fams.empty())
    NewTextChild(node,"grp_name",sum.fams.begin()->surname);
  else
    NewTextChild(node,"grp_name");

  Qry.Clear();
  Qry.SQLText="SELECT pr_tranz_reg,pr_airp_seance FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  if (USE_SEANCES())
	{
    if (Qry.FieldIsNULL("pr_airp_seance"))
      throw UserException("MSG.FLIGHT.SET_CHECKIN_MODE_IN_SEANCE");
  };

  if (pax_status==psTransit)
  {
    if (Qry.FieldIsNULL("pr_tranz_reg")||Qry.FieldAsInteger("pr_tranz_reg")==0)
      throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");
  };

  if (pr_unaccomp)
  {
    NewTextChild(resNode,"ckin_state","BagBeforeReg");
    return;
  };

  if (/*pax_status==psTransit ||*/ grp.digCkin ||grp.prefix=='-')
  {
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  };

  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  string sql;

  if (grp.large)
  {
    //большая группа
    sql=  "SELECT crs_pnr.pnr_id, \n"
          "       MIN(crs_pnr.grp_name) AS grp_name, \n"
          "       MIN(DECODE(crs_pax.pers_type,'ВЗ', \n"
          "                  crs_pax.surname||' '||crs_pax.name,'')) AS pax_name, \n"
          "       COUNT(*) AS seats_all, \n"
          "       SUM(DECODE(pax.pax_id,NULL,1,0)) AS seats \n"
          "FROM crs_pnr,crs_pax,pax,( \n";

    sql+= GetSearchPaxSubquery(pax_status,
                               true,
                               false,
                               false,
                               TReqInfo::Instance()->desk.compatible(PAD_VERSION),
                               "");

    sql+= "  ) ids  \n"
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND \n"
          "      crs_pax.pax_id=pax.pax_id(+) AND \n"
          "      ids.pnr_id=crs_pnr.pnr_id AND \n"
          "      crs_pax.pr_del=0 AND \n"
          "      crs_pax.seats>0 \n"
          "GROUP BY crs_pnr.pnr_id \n"
          "HAVING COUNT(*)>= :seats \n";

    ProgTrace(TRACE5,"CheckInInterface::SearchPax (large): status=%s",EncodePaxStatus(pax_status));
    ProgTrace(TRACE5,"CheckInInterface::SearchPax (large): sql=\n%s",sql.c_str());

    PaxQry.SQLText = sql;
    PaxQry.CreateVariable("point_id",otInteger,point_dep);
    switch (pax_status)
    {
      case psTransit: PaxQry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
                      break;
       case psGoshow: PaxQry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
                      //break не надо!
             default: PaxQry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
    };
    PaxQry.CreateVariable("seats",otInteger,sum.nPaxWithPlace);
    PaxQry.Execute();
    if (!PaxQry.Eof)
    {
      TQuery PnrAddrQry(&OraSession);
      PnrAddrQry.SQLText = //pnr_market_flt
        "SELECT DECODE(pnr_addrs.airline,tlg_trips.airline,NULL,pnr_addrs.airline) AS airline, "
        "       pnr_addrs.addr "
        "FROM tlg_trips,crs_pnr,pnr_addrs "
        "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
        "      crs_pnr.pnr_id=pnr_addrs.pnr_id AND "
        "      pnr_addrs.pnr_id=:pnr_id "
        "ORDER BY DECODE(pnr_addrs.airline,tlg_trips.airline,0,1),pnr_addrs.airline";
      PnrAddrQry.DeclareVariable("pnr_id",otInteger);

      xmlNodePtr pnrNode=NewTextChild(resNode,"groups");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        node=NewTextChild(pnrNode,"pnr");
        NewTextChild(node,"pnr_id",PaxQry.FieldAsInteger("pnr_id"));
        NewTextChild(node,"pax_name",PaxQry.FieldAsString("pax_name"));
        NewTextChild(node,"grp_name",PaxQry.FieldAsString("grp_name"));
        NewTextChild(node,"seats",PaxQry.FieldAsInteger("seats"));
        NewTextChild(node,"seats_all",PaxQry.FieldAsInteger("seats_all"));

        PnrAddrQry.SetVariable("pnr_id",PaxQry.FieldAsInteger("pnr_id"));
        PnrAddrQry.Execute();
        if (!PnrAddrQry.Eof)
        {
          if (!PnrAddrQry.FieldIsNULL("airline"))
          {
            node=NewTextChild(node,"pnr_addr");
            NewTextChild(node,"airline",PnrAddrQry.FieldAsString("airline"));
            NewTextChild(node,"addr",PnrAddrQry.FieldAsString("addr"));
          }
          else
            NewTextChild(node,"pnr_addr",PnrAddrQry.FieldAsString("addr"));
        };
      };
      //строка NoRec
      node=NewTextChild(pnrNode,"pnr");
      if (!sum.fams.empty())
        NewTextChild(node,"grp_name",sum.fams.begin()->surname);
      NewTextChild(node,"seats",sum.nPaxWithPlace);
      NewTextChild(node,"seats_all",sum.nPaxWithPlace);

      NewTextChild(resNode,"ckin_state","GrpChoice");
      return;
    };
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  };

  for(int i=0;i<2;i++)
  {
    string surnames;
    for(vector<TInquiryFamily>::iterator f=grp.fams.begin();f!=grp.fams.end();f++)
    {
      if (!surnames.empty()) surnames+=" OR ";
      surnames=surnames+"crs_pax.surname LIKE '"+f->surname.substr(0,i==0?4:1)+"%'";
    };

    //обычный поиск
    sql =
      "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv,crs_pnr.subclass, \n"
      "       crs_pnr.class, crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, \n"
      "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, \n"
      "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, \n"
      "       crs_pax.seat_type,crs_pax.seats, \n"
      "       crs_pnr.pnr_id, \n"
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, \n"
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket \n"
      "FROM crs_pnr,crs_pax,pax,( \n";


    sql+= GetSearchPaxSubquery(pax_status,
                               sum.nPax>1 && !charter_search,
                               true,
                               true,
                               TReqInfo::Instance()->desk.compatible(PAD_VERSION),
                               surnames);

    sql+= "  ) ids  \n"
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND \n"
          "      crs_pax.pax_id=pax.pax_id(+) AND \n";
    if (sum.nPax>1 && !charter_search)
      sql+=
          "      ids.pnr_id=crs_pnr.pnr_id AND \n";
    else
      sql+=
          "      ids.pax_id=crs_pax.pax_id AND \n";

    sql+= "      crs_pax.pr_del=0 AND \n"
          "      pax.pax_id IS NULL \n"
          "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id \n";

//    ProgTrace(TRACE5,"CheckInInterface::SearchPax: status=%s",EncodePaxStatus(pax_status));
//    ProgTrace(TRACE5,"CheckInInterface::SearchPax: sql=\n%s",sql.c_str());

    PaxQry.SQLText = sql;
    PaxQry.CreateVariable("point_id",otInteger,point_dep);
    switch (pax_status)
    {
      case psTransit: PaxQry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
                      break;
       case psGoshow: PaxQry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
                      //break не надо!
             default: PaxQry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
    };
    PaxQry.Execute();
    if (!PaxQry.Eof) break;
  };
  if (PaxQry.Eof)
  {
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  };

  CreateSearchResponse(point_dep,PaxQry,resNode);


//  if (fmt.persCountFmt==0)
  {
    //построим вектор обрезанных фамилий поискового запроса
    xmlNodePtr foundPnr=NULL,foundTrip=NULL;
    xmlNodePtr tripNode,pnrNode,paxNode;
    vector<string> surnames;
    for(vector<TInquiryFamily>::iterator f=grp.fams.begin();f!=grp.fams.end();f++)
    {
      surnames.push_back(f->surname.substr(0,4));
    };

    vector<string>::iterator i;
    tripNode=NodeAsNode("trips",resNode)->children;
    for(;tripNode!=NULL;tripNode=tripNode->next)
    {
      pnrNode=NodeAsNode("groups",tripNode)->children;
      //цикл по PNR
      for(;pnrNode!=NULL;pnrNode=pnrNode->next)
      {
        //проверим на кол-во человек в группе
        int n=0;
        paxNode=NodeAsNode("passengers",pnrNode)->children;
        for(node=paxNode;node!=NULL;node=node->next) n++;
        if (n!=sum.nPax) continue;

        //проверим на то чтобы все фамилии присутствовали в группе
        for(i=surnames.begin();i!=surnames.end();i++)
        {
          for(node=paxNode;node!=NULL;node=node->next)
          {
            xmlNodePtr node2 = node->children;
            if (i->size()<3)
            {
              if (strcmp(i->c_str(),NodeAsStringFast("surname",node2))==0) break;
            }
            else
            {
              if (strncmp(i->c_str(),NodeAsStringFast("surname",node2),i->size())==0) break;
            };
          };
          if (node==NULL) break; //не найдена фамилия
        };
        if (i==surnames.end())
        {
          //все фамилии найдены
          if (foundPnr!=NULL)
          {
            // минимум 2 подходящих группы
            foundPnr=NULL;
            foundTrip=NULL;
            break;
          };
          foundPnr=pnrNode;
          foundTrip=tripNode;
        };
      };
      if (pnrNode!=NULL) break;
    };

    if (foundPnr!=NULL&&foundTrip!=NULL)
    {
      //наидена только одна подходящая группа
      //проверим что она не PAD при поиске брони
      xmlNodePtr node2=foundPnr->children;
      if (!(pax_status==psCheckin && NodeAsIntegerFast("pad",node2,0)!=0 ))
      {
        xmlUnlinkNode(foundTrip);
        tripNode=NodeAsNode("trips",resNode);
        xmlUnlinkNode(tripNode);
        xmlFreeNode(tripNode);
        tripNode=NewTextChild(resNode,"trips");
        xmlAddChild(tripNode,foundTrip);

        xmlUnlinkNode(foundPnr);
        pnrNode=NodeAsNode("groups",foundTrip);
        xmlUnlinkNode(pnrNode);
        xmlFreeNode(pnrNode);
        pnrNode=NewTextChild(foundTrip,"groups");
        xmlAddChild(pnrNode,foundPnr);

        NewTextChild(resNode,"ckin_state","BeforeReg");
        return;
      };
    };
  };

  CreateNoRecResponse(sum,resNode);
  NewTextChild(resNode,"ckin_state","Choice");

 /* int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
  if (free<sum.nPaxWithPlace)
    showErrorMessage("Доступных мест осталось "+IntToString(free));*/

};

int CheckInInterface::CheckCounters(int point_dep, int point_arv, char* cl, TPaxStatus grp_status)
{
    //проверка наличия свободных мест
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT free_ok,free_goshow,nooccupy FROM counters2 "
      "WHERE point_dep=:point_dep AND point_arv=:point_arv AND class=:class ";
    Qry.CreateVariable("point_dep", otInteger, point_dep);
    Qry.CreateVariable("point_arv", otInteger, point_arv);
    Qry.CreateVariable("class", otString, cl);
    Qry.Execute();
    if (Qry.Eof)
    {
      ProgTrace(TRACE0,"counters2 empty! (point_dep=%d point_arv=%d cl=%s)",point_dep,point_arv,cl);
      TQuery RecountQry(&OraSession);
      RecountQry.Clear();
      RecountQry.SQLText=
        "BEGIN "
        "  ckin.recount(:point_dep); "
        "END;";
      RecountQry.CreateVariable("point_dep", otInteger, point_dep);
      RecountQry.Execute();
      Qry.Execute();
    };
    if (Qry.Eof) return 0;
    TTripStages tripStages( point_dep );
    TStage ckin_stage =  tripStages.getStage( stCheckIn );
    int free;
    switch (grp_status)
    {
      case psTransit:
                 free=Qry.FieldAsInteger("nooccupy");
                 break;
      case psCheckin:
      case psTCheckin:
      	         if (ckin_stage==sNoActive ||
      	             ckin_stage==sPrepCheckIn ||
      	             ckin_stage==sOpenCheckIn)
                   free=Qry.FieldAsInteger("free_ok");
                 else
                   free=Qry.FieldAsInteger("nooccupy");
                 break;
      default:   if (ckin_stage==sNoActive ||
      	             ckin_stage==sPrepCheckIn ||
      	             ckin_stage==sOpenCheckIn)
                   free=Qry.FieldAsInteger("free_goshow");
                 else
                   free=Qry.FieldAsInteger("nooccupy");
    };
    if (free>=0)
      return free;
    else
      return 0;
};

bool CheckFltOverload(int point_id, const TTripInfo &fltInfo, bool overload_alarm )
{
	if ( !overload_alarm ) return false;
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT pr_check_load,pr_overload_reg FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) throw EXCEPTIONS::Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  bool pr_check_load=Qry.FieldAsInteger("pr_check_load")!=0;
  bool pr_overload_reg=Qry.FieldAsInteger("pr_overload_reg")!=0;

  if (!pr_check_load && pr_overload_reg) return false;

  if (pr_overload_reg)
  {
    AstraLocale::showErrorMessage("MSG.FLIGHT.MAX_COMMERCE");
    return true;
  }
  else
    throw OverloadException("MSG.FLIGHT.MAX_COMMERCE");
  return false;
};

void CheckInInterface::ArrivalPaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id=NodeAsInteger("point_id",reqNode); //это point_arv
    TTripRoute route;
    if (!route.GetRouteBefore(NoExists,point_id,trtNotCurrent,trtNotCancelled))
        throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    if (route.empty())
        throw UserException("MSG.NOT_ARRIVAL_PASSENGERS_LIST");
    TTripRouteItem& routeItem=route.back();
    ReplaceTextChild(reqNode,"point_id",routeItem.point_id);
    PaxList(ctxt,reqNode,resNode);
};

void CheckInInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_id=NodeAsInteger("point_id",reqNode);
  readPaxLoad( point_id, reqNode, resNode );

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out,airline_fmt,suffix_fmt,airp_fmt "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo operFlt(Qry);

  NewTextChild(resNode,"flight",GetTripName(operFlt,ecCkin,true,false));

  bool createDefaults=false;
  string def_pers_type=ElemIdToCodeNative(etPersType, EncodePerson(ASTRA::adult));
  string def_class=ElemIdToCodeNative(etClass, EncodeClass(ASTRA::Y));
  int def_client_type_id=(int)ctTerm;
  int def_status_id=(int)psCheckin;
  
  bool with_rcpt_info=(strcmp((char *)reqNode->name, "BagPaxList")==0);

  ostringstream sql;
  sql <<
    "SELECT "
    "  reg_no,surname,name,pax_grp.airp_arv, "
    "  last_trfer.airline AS trfer_airline, "
    "  last_trfer.flt_no AS trfer_flt_no, "
    "  last_trfer.suffix AS trfer_suffix, "
    "  last_trfer.airp_arv AS trfer_airp_arv, "
    "  last_tckin_seg.airline AS tckin_seg_airline, "
    "  last_tckin_seg.flt_no AS tckin_seg_flt_no, "
    "  last_tckin_seg.suffix AS tckin_seg_suffix, "
    "  last_tckin_seg.airp_arv AS tckin_seg_airp_arv, "
    "  class,pax.subclass,pax_grp.status, "
    "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'_seats',rownum) AS seat_no, "
    "  seats,wl_type,pers_type,ticket_rem, "
    "  ticket_no||DECODE(coupon_no,NULL,NULL,'/'||coupon_no) AS ticket_no, "
    "  ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
    "  ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
    "  ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
    "  ckin.get_excess(pax.grp_id,pax.pax_id) AS excess, "
    "  ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
    "  ckin.get_remarks(pax.pax_id,' ',0) AS rems, "
    "  mark_trips.airline AS airline_mark, "
    "  mark_trips.flt_no AS flt_no_mark, "
    "  mark_trips.suffix AS suffix_mark, "
    "  mark_trips.scd AS scd_local_mark, "
    "  mark_trips.airp_dep AS airp_dep_mark, "
    "  pax.grp_id, "
    "  pax.pax_id, "
    "  pax_grp.class_grp AS cl_grp_id,pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id,pax_grp.client_type "
    "FROM pax_grp,pax,mark_trips, "
    "     (SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,transfer.airp_arv, "
    "             transfer.grp_id "
    "      FROM pax_grp,transfer,trfer_trips "
    "      WHERE pax_grp.grp_id=transfer.grp_id AND "
    "            transfer.point_id_trfer=trfer_trips.point_id AND "
    "            transfer.pr_final<>0 AND "
    "            pax_grp.point_dep=:point_id) last_trfer, "
    "     (SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,tckin_segments.airp_arv, "
    "             tckin_segments.grp_id"
    "      FROM pax_grp,tckin_segments,trfer_trips "
    "      WHERE pax_grp.grp_id=tckin_segments.grp_id AND "
    "            tckin_segments.point_id_trfer=trfer_trips.point_id AND "
    "            tckin_segments.pr_final<>0 AND "
    "            pax_grp.point_dep=:point_id) last_tckin_seg "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_id_mark=mark_trips.point_id AND "
    "      pax_grp.grp_id=last_trfer.grp_id(+) AND "
    "      pax_grp.grp_id=last_tckin_seg.grp_id(+) AND "
    "      point_dep=:point_id AND pr_brd IS NOT NULL ";
  if (with_rcpt_info)
    sql <<
    "  AND ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, pax_grp.excess)<>0 ";
  sql <<
    "ORDER BY reg_no"; //в будущем убрать ORDER BY

  ProgTrace(TRACE5, "%s", sql.str().c_str());

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("lang",otString,reqInfo->desk.lang);
  Qry.Execute();
  xmlNodePtr node=NewTextChild(resNode,"passengers");
  if (!Qry.Eof)
  {
    createDefaults=true;

    int col_pax_id=Qry.FieldIndex("pax_id");
    int col_reg_no=Qry.FieldIndex("reg_no");
    int col_surname=Qry.FieldIndex("surname");
    int col_name=Qry.FieldIndex("name");
    int col_airp_arv=Qry.FieldIndex("airp_arv");
    int col_class=Qry.FieldIndex("class");
    int col_subclass=Qry.FieldIndex("subclass");
    int col_status=Qry.FieldIndex("status");
    int col_seat_no=Qry.FieldIndex("seat_no");
    int col_seats=Qry.FieldIndex("seats");
    int col_wl_type=Qry.FieldIndex("wl_type");
    int col_pers_type=Qry.FieldIndex("pers_type");
    int col_ticket_rem=Qry.FieldIndex("ticket_rem");
    int col_ticket_no=Qry.FieldIndex("ticket_no");
    int col_bag_amount=Qry.FieldIndex("bag_amount");
    int col_bag_weight=Qry.FieldIndex("bag_weight");
    int col_rk_weight=Qry.FieldIndex("rk_weight");
    int col_excess=Qry.FieldIndex("excess");
    int col_tags=Qry.FieldIndex("tags");
    int col_rems=Qry.FieldIndex("rems");

    int col_airline_mark=Qry.FieldIndex("airline_mark");
    int col_flt_no_mark=Qry.FieldIndex("flt_no_mark");
    int col_suffix_mark=Qry.FieldIndex("suffix_mark");
    int col_scd_local_mark=Qry.FieldIndex("scd_local_mark");
    int col_airp_dep_mark=Qry.FieldIndex("airp_dep_mark");

    int col_grp_id=Qry.FieldIndex("grp_id");
    int col_cl_grp_id=Qry.FieldIndex("cl_grp_id");
    int col_hall_id=Qry.FieldIndex("hall_id");
    int col_point_arv=Qry.FieldIndex("point_arv");
    int col_user_id=Qry.FieldIndex("user_id");
    int col_client_type=Qry.FieldIndex("client_type");

    map< int/*grp_id*/, pair<bool/*pr_payment*/, bool/*pr_receipts*/> > rcpt_complete;
    TPaxSeats priorSeats(point_id);
    TQuery PaxDocQry(&OraSession);
    for(;!Qry.Eof;Qry.Next())
    {
      int grp_id = Qry.FieldAsInteger(col_grp_id);
      int pax_id = Qry.FieldAsInteger(col_pax_id);

      xmlNodePtr paxNode=NewTextChild(node,"pax");
      NewTextChild(paxNode,"reg_no",Qry.FieldAsInteger(col_reg_no));
      NewTextChild(paxNode,"surname",Qry.FieldAsString(col_surname));

      if (reqInfo->desk.compatible(LATIN_VERSION))
        NewTextChild(paxNode,"name",Qry.FieldAsString(col_name));
      else
        NewTextChild(paxNode,"name",Qry.FieldAsString(col_name),"");

      NewTextChild(paxNode,"airp_arv",ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));

      TLastTrferInfo trferInfo(Qry);
      NewTextChild(paxNode,"last_trfer",trferInfo.str(),"");
      TLastTCkinSegInfo tckinSegInfo(Qry);
      NewTextChild(paxNode,"last_tckin_seg",tckinSegInfo.str(),"");

      if (reqInfo->desk.compatible(LATIN_VERSION))
        NewTextChild(paxNode,"class",ElemIdToCodeNative(etClass, Qry.FieldAsString(col_class)), def_class);
      else
        NewTextChild(paxNode,"class",ElemIdToCodeNative(etClass, Qry.FieldAsString(col_class)));

      NewTextChild(paxNode,"subclass",ElemIdToCodeNative(etSubcls, Qry.FieldAsString(col_subclass)));

      if (Qry.FieldIsNULL(col_wl_type))
      {
        //не на листе ожидания, но возможно потерял место при смене компоновки
        if (Qry.FieldIsNULL(col_seat_no) && Qry.FieldAsInteger(col_seats)>0)
        {
          ostringstream seat_no_str;
          seat_no_str << "("
                      << priorSeats.getSeats(pax_id,"seats")
                      << ")";
          NewTextChild(paxNode,"seat_no_str",seat_no_str.str());
          NewTextChild(paxNode,"seat_no_alarm",(int)true);
        };
      }
      else
      {
        NewTextChild(paxNode,"seat_no_str","ЛО");
        NewTextChild(paxNode,"seat_no_alarm",(int)true);
      };
      string seat_no = Qry.FieldAsString(col_seat_no);
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
      	seat_no = LTrimString( seat_no );
      NewTextChild(paxNode,"seat_no",seat_no);
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger(col_seats),1);

      if (reqInfo->desk.compatible(LATIN_VERSION))
      {
        NewTextChild(paxNode,"pers_type",ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)), def_pers_type);
        NewTextChild(paxNode,"document", GetPaxDocStr(NoExists, pax_id, PaxDocQry, true), "");
      }
      else
      {
        NewTextChild(paxNode,"pers_type",ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)));
        NewTextChild(paxNode,"document", GetPaxDocStr(NoExists, pax_id, PaxDocQry, true));
      };

      NewTextChild(paxNode,"ticket_rem",Qry.FieldAsString(col_ticket_rem),"");
      NewTextChild(paxNode,"ticket_no",Qry.FieldAsString(col_ticket_no),"");
      NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger(col_bag_amount),0);
      NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger(col_bag_weight),0);
      NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger(col_rk_weight),0);
      NewTextChild(paxNode,"excess",Qry.FieldAsInteger(col_excess),0);
      NewTextChild(paxNode,"tags",Qry.FieldAsString(col_tags),"");
      NewTextChild(paxNode,"rems",Qry.FieldAsString(col_rems),"");


      //коммерческий рейс
      TTripInfo markFlt;
      markFlt.airline=Qry.FieldAsString(col_airline_mark);
      markFlt.flt_no=Qry.FieldAsInteger(col_flt_no_mark);
      markFlt.suffix=Qry.FieldAsString(col_suffix_mark);
      markFlt.scd_out=Qry.FieldAsDateTime(col_scd_local_mark);
      markFlt.airp=Qry.FieldAsString(col_airp_dep_mark);
      
      bool mark_equal_oper=false;
      string mark_flt_str=GetMktFlightStr(operFlt,markFlt,mark_equal_oper);

      if (!mark_equal_oper)
        NewTextChild(paxNode,"mark_flt_str",mark_flt_str);

      if (with_rcpt_info)
      {
        string receipts=GetBagRcptStr(grp_id, pax_id);
        NewTextChild(paxNode,"rcpt_no_list",receipts,"");

        map<int, pair<bool, bool> >::iterator i=rcpt_complete.find(grp_id);
        if (i==rcpt_complete.end())
        {
          bool pr_payment=BagPaymentCompleted(grp_id);
          rcpt_complete[grp_id]=make_pair(pr_payment, !receipts.empty());
        }
        else
        {
          i->second.second= i->second.second || !receipts.empty();
        };
      };
      //идентификаторы
      NewTextChild(paxNode,"grp_id",grp_id);
      NewTextChild(paxNode,"cl_grp_id",Qry.FieldAsInteger(col_cl_grp_id));
      if (!Qry.FieldIsNULL(col_hall_id))
        NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger(col_hall_id));
      else
        NewTextChild(paxNode,"hall_id",-1);
      NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger(col_point_arv));
      NewTextChild(paxNode,"user_id",Qry.FieldAsInteger(col_user_id));
      if (reqInfo->desk.compatible(LATIN_VERSION))
      {
        NewTextChild(paxNode,"client_type_id",
                             (int)DecodeClientType(Qry.FieldAsString(col_client_type)),
                             def_client_type_id);
        NewTextChild(paxNode,"status_id",
                             (int)DecodePaxStatus(Qry.FieldAsString(col_status)),
                             def_status_id);
      }
      else
      {
        NewTextChild(paxNode,"client_type_id",(int)DecodeClientType(Qry.FieldAsString(col_client_type)));
        NewTextChild(paxNode,"status_id",(int)DecodePaxStatus(Qry.FieldAsString(col_status)));
      };
    };
    if (with_rcpt_info)
    {
      for(xmlNodePtr paxNode=node->children;paxNode!=NULL;paxNode=paxNode->next)
      {
        xmlNodePtr node2=paxNode->children;
        int grp_id=NodeAsIntegerFast("grp_id",node2);
        map<int, pair<bool, bool> >::iterator i=rcpt_complete.find(grp_id);
        if (i==rcpt_complete.end())
          throw EXCEPTIONS::Exception("CheckInInterface::PaxList: grp_id=%d not found", grp_id);
        // все ли квитанции распечатаны:
        // 0 - частично напечатаны
        // 1 - все напечатаны
        // 2 - нет ни одной квитанции
        int rcpt_complete=2;
        if (i->second.second) rcpt_complete=(int)i->second.first;
        if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
        else
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,0);
      };
    };
  };

  //несопровождаемый багаж
  sql.str("");

  sql <<
    "SELECT "
    "  pax_grp.airp_arv,pax_grp.status, "
    "  last_trfer.airline AS trfer_airline, "
    "  last_trfer.flt_no AS trfer_flt_no, "
    "  last_trfer.suffix AS trfer_suffix, "
    "  last_trfer.airp_arv AS trfer_airp_arv, "
    "  last_tckin_seg.airline AS tckin_seg_airline, "
    "  last_tckin_seg.flt_no AS tckin_seg_flt_no, "
    "  last_tckin_seg.suffix AS tckin_seg_suffix, "
    "  last_tckin_seg.airp_arv AS tckin_seg_airp_arv, "
    "  report.get_last_tckin_seg(pax_grp.grp_id) AS last_tckin_seg, "
    "  ckin.get_bagAmount2(pax_grp.grp_id,NULL,NULL) AS bag_amount, "
    "  ckin.get_bagWeight2(pax_grp.grp_id,NULL,NULL) AS bag_weight, "
    "  ckin.get_rkWeight2(pax_grp.grp_id,NULL,NULL) AS rk_weight, "
    "  ckin.get_excess(pax_grp.grp_id,NULL) AS excess, "
    "  ckin.get_birks2(pax_grp.grp_id,NULL,NULL,:lang) AS tags, "
    "  pax_grp.grp_id, "
    "  pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id,pax_grp.client_type "
    "FROM pax_grp, "
    "     (SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,transfer.airp_arv, "
    "             transfer.grp_id "
    "      FROM pax_grp,transfer,trfer_trips "
    "      WHERE pax_grp.grp_id=transfer.grp_id AND "
    "            transfer.point_id_trfer=trfer_trips.point_id AND "
    "            transfer.pr_final<>0 AND "
    "            pax_grp.point_dep=:point_id AND pax_grp.class IS NULL) last_trfer, "
    "     (SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,tckin_segments.airp_arv, "
    "             tckin_segments.grp_id"
    "      FROM pax_grp,tckin_segments,trfer_trips "
    "      WHERE pax_grp.grp_id=tckin_segments.grp_id AND "
    "            tckin_segments.point_id_trfer=trfer_trips.point_id AND "
    "            tckin_segments.pr_final<>0 AND "
    "            pax_grp.point_dep=:point_id AND pax_grp.class IS NULL) last_tckin_seg "
    "WHERE pax_grp.grp_id=last_trfer.grp_id(+) AND "
    "      pax_grp.grp_id=last_tckin_seg.grp_id(+) AND "
    "      point_dep=:point_id AND class IS NULL ";

  if (with_rcpt_info)
    sql <<
    "  AND ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, pax_grp.excess)<>0 ";

  ProgTrace(TRACE5, "%s", sql.str().c_str());

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("lang",otString,reqInfo->desk.lang);
  Qry.Execute();
  node=NewTextChild(resNode,"unaccomp_bag");
  if (!Qry.Eof)
  {
    createDefaults=true;
    for(;!Qry.Eof;Qry.Next())
    {
      int grp_id=Qry.FieldAsInteger("grp_id");
    
      xmlNodePtr paxNode=NewTextChild(node,"bag");
      NewTextChild(paxNode,"airp_arv",ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp_arv")));

      TLastTrferInfo trferInfo(Qry);
      NewTextChild(paxNode,"last_trfer",trferInfo.str(),"");
      TLastTCkinSegInfo tckinSegInfo(Qry);
      NewTextChild(paxNode,"last_tckin_seg",tckinSegInfo.str(),"");

      NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
      NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
      NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger("rk_weight"),0);
      NewTextChild(paxNode,"excess",Qry.FieldAsInteger("excess"),0);
      NewTextChild(paxNode,"tags",Qry.FieldAsString("tags"),"");
      if (with_rcpt_info)
      {
        string receipts=GetBagRcptStr(grp_id, NoExists);
        NewTextChild(paxNode,"rcpt_no_list",receipts,"");
        // все ли квитанции распечатаны
        //0 - частично напечатаны
        //1 - все напечатаны
        //2 - нет ни одной квитанции
        int rcpt_complete=2;
        if (!receipts.empty()) rcpt_complete=(int)BagPaymentCompleted(grp_id);
        if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
        else
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,0);
      };
      //идентификаторы
      NewTextChild(paxNode,"grp_id",grp_id);
      if (!Qry.FieldIsNULL("hall_id"))
        NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger("hall_id"));
      else
        NewTextChild(paxNode,"hall_id",-1);
      NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger("point_arv"));
      NewTextChild(paxNode,"user_id",Qry.FieldAsInteger("user_id"));
      if (reqInfo->desk.compatible(LATIN_VERSION))
      {
        NewTextChild(paxNode,"client_type_id",
                             (int)DecodeClientType(Qry.FieldAsString("client_type")),
                             def_client_type_id);
        NewTextChild(paxNode,"status_id",
                             (int)DecodePaxStatus(Qry.FieldAsString("status")),
                             def_status_id);
      }
      else
      {
        NewTextChild(paxNode,"client_type_id",(int)DecodeClientType(Qry.FieldAsString("client_type")));
        NewTextChild(paxNode,"status_id",(int)DecodePaxStatus(Qry.FieldAsString("status")));
      };
    };
  };

  Qry.Close();

  if (createDefaults)
  {
    xmlNodePtr defNode = NewTextChild( resNode, "defaults" );
    NewTextChild(defNode, "last_trfer", "");
    NewTextChild(defNode, "last_tckin_seg", "");
    NewTextChild(defNode, "bag_amount", 0);
    NewTextChild(defNode, "bag_weight", 0);
    NewTextChild(defNode, "rk_weight", 0);
    NewTextChild(defNode, "excess", 0);
    NewTextChild(defNode, "tags", "");
    //не для несопровождаемого багажа
    NewTextChild(defNode, "name", "");
    NewTextChild(defNode, "class", def_class);
    NewTextChild(defNode, "seats", 1);
    NewTextChild(defNode, "seat_no_alarm", (int)false);
    NewTextChild(defNode, "pers_type", def_pers_type);
    NewTextChild(defNode, "document", "");
    NewTextChild(defNode, "ticket_rem", "");
    NewTextChild(defNode, "ticket_no", "");
    NewTextChild(defNode, "rems", "");
    NewTextChild(defNode, "mark_flt_str", "");
    if (with_rcpt_info)
    {
      NewTextChild(defNode, "rcpt_no_list", "");
      NewTextChild(defNode, "rcpt_complete", 2);
    };
    //идентификаторы
    NewTextChild(defNode, "client_type_id", def_client_type_id);
    NewTextChild(defNode, "status_id", def_status_id);
  };

  get_new_report_form("ArrivalPaxList", reqNode, resNode);
  xmlNodePtr variablesNode = STAT::set_variables(resNode);
  TRptParams rpt_params(reqInfo->desk.lang);
  PaxListVars(point_id, rpt_params, variablesNode);
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.ARRIVAL_PAX_LIST", LParams() << LParam("flight", get_flight(variablesNode)))); //!!!den param 100%русский
};

bool GetUsePS()
{
	return false; //!!!
}

bool CheckInInterface::CheckCkinFlight(const int point_dep,
                                       const string& airp_dep,
                                       const int point_arv,
                                       const string& airp_arv,
                                       bool lock,
                                       TSegInfo& segInfo)
{
  segInfo.point_dep=point_dep;
  segInfo.airp_dep=airp_dep;
  segInfo.point_arv=point_arv;
  segInfo.airp_arv=airp_arv;
  segInfo.point_num=ASTRA::NoExists;
  segInfo.first_point=ASTRA::NoExists;
  segInfo.pr_tranzit=false;
  segInfo.fltInfo.Clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT airline,flt_no,suffix,airp,scd_out, "
         "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
         "       point_num,first_point,pr_tranzit, "
         "       pr_del "
         "FROM points "
         "WHERE point_id=:point_id AND airp=:airp AND pr_del>=0 AND pr_reg<>0";
  if (lock)
    sql << " FOR UPDATE";

  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.CreateVariable("airp",otString,airp_dep);
  Qry.Execute();
  if (Qry.Eof) return false;

  segInfo.point_num=Qry.FieldAsInteger("point_num");
  segInfo.first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  segInfo.pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
  segInfo.fltInfo.Init(Qry);

  Qry.Clear();
  if (point_arv==ASTRA::NoExists)
  {
    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        segInfo.point_dep,
                        segInfo.point_num,
                        segInfo.first_point,
                        segInfo.pr_tranzit,
                        trtNotCurrent,trtNotCancelled);

    TTripRoute::iterator r;
    for(r=route.begin();r!=route.end();r++)
      if (r->airp==airp_arv) break;
    if (r==route.end()) return false;
    segInfo.point_arv=r->point_id;
  }
  else
  {
    Qry.SQLText=
      "SELECT point_id FROM points "
      "WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
    Qry.CreateVariable("point_id",otInteger,point_arv);
    Qry.CreateVariable("airp",otString,airp_arv);
    Qry.Execute();
    if (Qry.Eof) return false;
  };
  return true;
};

bool CheckInInterface::CheckFQTRem(xmlNodePtr remNode, TypeB::TFQTItem &fqt)
{
  if (remNode==NULL) return false;
  xmlNodePtr node2=remNode->children;
  string rem_code=NodeAsStringFast("rem_code",node2);
  string rem_text=NodeAsStringFast("rem_text",node2);
  //проверим корректность ремарки FQT...
  string rem_code2=rem_text.substr(0,4);
  if (rem_code2=="FQTV" ||
      rem_code2=="FQTU" ||
      rem_code2=="FQTR" ||
      rem_code2=="FQTS" )
  {
    if (rem_code!=rem_code2)
    {
      rem_code=rem_code2;
      if (rem_text.size()>4) rem_text.insert(4," ");
      ReplaceTextChild(remNode, "rem_code", rem_code);
      ReplaceTextChild(remNode, "rem_text", rem_text);
    };

    TypeB::TTlgParser parser;
    return ParseFQTRem(parser,rem_text,fqt);
  };
  return false;
};

bool CheckInInterface::ParseFQTRem(TypeB::TTlgParser &tlg,string &rem_text,TypeB::TFQTItem &fqt)
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
    try
    {
      //проверим чтобы ремарка содержала только буквы, цифры, пробелы и слэши
      for(string::const_iterator i=rem_text.begin();i!=rem_text.end();i++)
        if (!(IsUpperLetter(*i) ||
              IsDigit(*i) ||
              *i>0 && *i<=' ' ||
              *i=='/')) throw UserException("MSG.INVALID_SYMBOL",
                                            LParams()<<LParam("symbol",string(1,*i))); //WEB

      for(k=0;k<=1;k++)
      try
      {
        p=tlg.GetWord(p);
        if (p==NULL)
        {
          if (k==0)
            throw UserException("MSG.AIRLINE.CODE_NOT_SET"); //WEB
          else
            throw UserException("MSG.PASSENGER.NO_IDENTIFIER"); //WEB
        };

        c=0;
        switch(k)
        {
          case 0:
            res=sscanf(tlg.lex,"%3[A-ZА-ЯЁ]%c",fqt.airline,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%2[A-ZА-ЯЁ0-9]%c",fqt.airline,&c);
              if (c!=0||res!=1)
                throw UserException("MSG.AIRLINE.INVALID_INPUT_VALUE",
                                    LParams()<<LParam("airline", string(tlg.lex))); //WEB
            };

            try
            {
              TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("code/code_lat",fqt.airline));
              strcpy(fqt.airline,row.code.c_str());
            }
            catch (EBaseTableError)
            {
              throw UserException("MSG.AIRLINE.INVALID_INPUT_VALUE",
                                  LParams()<<LParam("airline", string(fqt.airline))); //WEB
            };
            break;
          case 1:
            res=sscanf(tlg.lex,"%25[A-ZА-ЯЁ0-9]%c",fqt.no,&c);
            if (c!=0||res!=1||strlen(fqt.no)<2)
              throw UserException("MSG.PASSENGER.INVALID_IDENTIFIER",
                                   LParams()<<LParam("ident", string(tlg.lex))); //WEB
            for(;*p!=0;p++)
              if (IsDigitIsLetter(*p)) break;
            fqt.extra=p;
            TrimString(fqt.extra);
            break;
        };
      }
      catch(std::exception)
      {
        switch(k)
        {
          case 0:
            *fqt.airline=0;
            throw;
          case 1:
            *fqt.no=0;
            throw;
        };
      };
    }
    catch(UserException &E)
    {
      throw UserException("WRAP.REMARK_ERROR",
                          LParams()<<LParam("rem_code", string(fqt.rem_code))
                                   <<LParam("text", E.getLexemaData( ) )); //WEB
    };
    return true;
  };
  return false;
};

LexemaData GetLexemeDataWithFlight(const LexemaData &data, const TTripInfo &fltInfo)
{
  LexemaData result;
  result.lexema_id="WRAP.FLIGHT";
  result.lparams << LParam("flight",GetTripName(fltInfo,ecCkin,true,false))
                 << LParam("text",data);
  return result;
};

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SavePax(reqNode, reqNode, NULL, resNode);
};

//процедура должна возвращать true только в том случае если произведена реальная регистрация
bool CheckInInterface::SavePax(xmlNodePtr termReqNode, xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  map<int,TSegInfo> segs;
  TChangeStatusList ETInfo;
  bool et_processed=false;

  xmlNodePtr segNode=NodeAsNode("segments/segment",reqNode);
  bool only_one=segNode->next==NULL;

  bool defer_etstatus=false;

  TQuery Qry(&OraSession);
  if (ediResNode==NULL && reqInfo->client_type == ctTerm) //для web-регистрации нераздельное подтверждение ЭБ
  {
    if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION))
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT defer_etstatus FROM desk_grp_sets WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,reqInfo->desk.grp_id);
      Qry.Execute();
      if (!Qry.Eof && !Qry.FieldIsNULL("defer_etstatus"))
        defer_etstatus=Qry.FieldAsInteger("defer_etstatus")!=0;
      else
        defer_etstatus=true;
    }
    else defer_etstatus=true;
  };

  Qry.Clear();
  for(;segNode!=NULL;segNode=segNode->next)
  {
    TSegInfo segInfo;
    segInfo.point_dep=NodeAsInteger("point_dep",segNode);
    segInfo.airp_dep=NodeAsString("airp_dep",segNode);
    segInfo.point_arv=NodeAsInteger("point_arv",segNode);
    segInfo.airp_arv=NodeAsString("airp_arv",segNode);
    if (segs.find(segInfo.point_dep)!=segs.end())
      throw UserException("MSG.CHECKIN.DUPLICATED_FLIGHT_IN_ROUTE"); //WEB
    segs[segInfo.point_dep]=segInfo;
  };

  //лочить рейсы надо по возрастанию poind_dep иначе может быть deadlock
  for(map<int,TSegInfo>::iterator s=segs.begin();s!=segs.end();s++)
  {
    if (!CheckCkinFlight(s->second.point_dep,
                         s->second.airp_dep,
                         s->second.point_arv,
                         s->second.airp_arv, true, s->second))
    {
    	if (s->second.fltInfo.pr_del==0)
    	{
        if (!only_one && !s->second.fltInfo.airline.empty())
          throw UserException("MSG.FLIGHT.CHANGED_NAME.REFRESH_DATA", //WEB
                              LParams()<<LParam("flight", GetTripName(s->second.fltInfo,ecCkin,true,false)));
        else
          throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB
      };
    };
    if (s->second.fltInfo.pr_del==ASTRA::NoExists ||
        s->second.fltInfo.pr_del!=0)
    {
      if (!only_one && !s->second.fltInfo.airline.empty())
        throw UserException("MSG.FLIGHT.CANCELED_NAME.REFRESH_DATA", //WEB
                            LParams()<<LParam("flight",GetTripName(s->second.fltInfo,ecCkin,true,false)));
      else
        throw UserException("MSG.FLIGHT.CANCELED.REFRESH_DATA"); //WEB
    };
  };

  //savepoint для отката при превышении загрузки (важно что после лочки)
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SAVEPOINT CHECKIN; "
    "END;";
  Qry.Execute();
  
  bool pr_unaccomp=strcmp((char *)reqNode->name, "TCkinSaveUnaccompBag") == 0;

  //reqInfo->user.check_access(amPartialWrite);
  //определим, открыт ли рейс для регистрации

  int tckin_id=-1,first_grp_id=-1;
  int agent_stat_point_id=NoExists;
  TDateTime agent_stat_ondate=NowUTC();

  segNode=NodeAsNode("segments/segment",reqNode);
  bool first_segment=true;
  int seg_no=1,tckin_seg_no=1;
  vector<CheckIn::TTransferItem> trfer;
  vector<CheckIn::TTransferItem>::const_iterator iTrfer;
  bool save_trfer=false;
  for(;segNode!=NULL;segNode=segNode->next,seg_no++,tckin_seg_no++,first_segment=false)
  {
    int point_dep,point_arv,grp_id,hall=ASTRA::NoExists;
    string cl,airp_dep,airp_arv;

    map<int,TSegInfo>::const_iterator s=segs.find(NodeAsInteger("point_dep",segNode));
    if (s==segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SavePax: point_id not found in map segs");

    point_dep=s->second.point_dep;
    airp_dep=s->second.airp_dep;
    point_arv=s->second.point_arv;
    airp_arv=s->second.airp_arv;

    const TTripInfo &fltInfo=s->second.fltInfo;
    TTripInfo markFltInfo=fltInfo;
    markFltInfo.scd_out=UTCToLocal(markFltInfo.scd_out,AirpTZRegion(markFltInfo.airp));
    modf(markFltInfo.scd_out,&markFltInfo.scd_out);
    bool pr_mark_norms=false;

    try
    {

      TTypeBSendInfo sendInfo(fltInfo);
      sendInfo.point_id=s->second.point_dep;
      sendInfo.point_num=s->second.point_num;
      sendInfo.first_point=s->second.first_point;
      sendInfo.pr_tranzit=s->second.pr_tranzit;
      sendInfo.tlg_type="BSM";



      //BSM
      BSM::TBSMAddrs BSMaddrs;
      map<string, string> HTTPGETparams;
      BSM::TTlgContent BSMContentBefore;
      bool BSMsend=BSM::IsSend(sendInfo,BSMaddrs);

      TQuery CrsQry(&OraSession);
      CrsQry.Clear();
      CrsQry.SQLText=
        "BEGIN "
        "  ckin.save_pax_docs(:pax_id, :document); "
        "END;";
      CrsQry.DeclareVariable("pax_id",otInteger);
      CrsQry.DeclareVariable("document",otString);
      
      TQuery PaxDocQry(&OraSession);
      TQuery PaxDocoQry(&OraSession);

      Qry.Clear();
      Qry.SQLText=
        "SELECT pr_tranz_reg,pr_etstatus,pr_airp_seance "
        "FROM trip_sets WHERE point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,point_dep);
      Qry.Execute();
      if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB
      if (USE_SEANCES())
     	{
        if (Qry.FieldIsNULL("pr_airp_seance"))
          throw UserException("MSG.FLIGHT.SET_CHECKIN_MODE_IN_SEANCE"); //WEB
      };
      
      //определим - новая регистрация или запись изменений
      bool new_checkin;
      xmlNodePtr node = GetNode("grp_id",segNode);
      if (node==NULL||NodeIsNULL(node))
      {
        grp_id=ASTRA::NoExists;
        new_checkin=true;
      }
      else
      {
        grp_id=NodeAsInteger(node);
        new_checkin=false;
      };

      bool pr_tranz_reg=!Qry.FieldIsNULL("pr_tranz_reg")&&Qry.FieldAsInteger("pr_tranz_reg")!=0;
      static map<string, long int> doc_node_names;
      if (doc_node_names.empty())
      {
        doc_node_names["type"]=DOC_TYPE_FIELD;
        doc_node_names["issue_country"]=DOC_ISSUE_COUNTRY_FIELD;
        doc_node_names["no"]=DOC_NO_FIELD;
        doc_node_names["nationality"]=DOC_NATIONALITY_FIELD;
        doc_node_names["birth_date"]=DOC_BIRTH_DATE_FIELD;
        doc_node_names["gender"]=DOC_GENDER_FIELD;
        doc_node_names["expiry_date"]=DOC_EXPIRY_DATE_FIELD;
        doc_node_names["surname"]=DOC_SURNAME_FIELD;
        doc_node_names["first_name"]=DOC_FIRST_NAME_FIELD;
        doc_node_names["second_name"]=DOC_SECOND_NAME_FIELD;
      };
      static map<string, long int> doco_node_names;
      if (doco_node_names.empty())
      {
        doco_node_names["birth_place"]=DOCO_BIRTH_PLACE_FIELD;
        doco_node_names["type"]=DOCO_TYPE_FIELD;
        doco_node_names["no"]=DOCO_NO_FIELD;
        doco_node_names["issue_place"]=DOCO_ISSUE_PLACE_FIELD;
        doco_node_names["issue_date"]=DOCO_ISSUE_DATE_FIELD;
        doco_node_names["expiry_date"]=DOCO_EXPIRY_DATE_FIELD;
        doco_node_names["applic_country"]=DOCO_APPLIC_COUNTRY_FIELD;
      };
      TCheckDocInfo checkDocInfo=GetCheckDocInfo(point_dep, airp_arv);
      TCheckDocTknInfo checkTknInfo=GetCheckTknInfo(point_dep);
      int pr_etstatus=Qry.FieldAsInteger("pr_etstatus");
      bool pr_etl_only=GetTripSets(tsETLOnly,fltInfo);

      xmlNodePtr node2,remNode;
      //проверим номера документов и билетов
      if (!pr_unaccomp)
      {
        Qry.Clear();
        Qry.SQLText=
          "SELECT ticket_rem, ticket_no, coupon_no, refuse FROM pax WHERE pax_id=:pax_id";
        Qry.DeclareVariable("pax_id", otInteger);
        node=NodeAsNode("passengers",segNode);
        for(node=node->children;node!=NULL;node=node->next)
        {
          node2=node->children;
          try
          {
            if (strlen(NodeAsStringFast("ticket_no",node2,""))>15)
            {
              string ticket_no=NodeAsStringFast("ticket_no",node2,"");
              if (ticket_no.size()>20) ticket_no.erase(20).append("...");
              throw UserException("MSG.CHECKIN.TICKET_LARGE_MAX_LEN", LParams()<<LParam("ticket_no",ticket_no));
            };
            
            string document;
            if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCS_VERSION))
            {
              xmlNodePtr docNode=GetNodeFast("document",node2);
              if (docNode!=NULL) docNode=docNode->children;
              if (docNode!=NULL)
                document=NodeAsStringFast("no",docNode,"");
            }
            else
            {
              document=NodeAsStringFast("document",node2,"");
            };
            if (document.size()>15)
            {
              if (document.size()>25) document.erase(25).append("...");
              throw UserException("MSG.CHECKIN.DOCUMENT_LARGE_MAX_LEN", LParams()<<LParam("document",document));
            };
          
            if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION) &&
                defer_etstatus && !pr_etl_only && pr_etstatus>=0 && //раздельное изменение статуса и есть связь с СЭБ
                strcmp(NodeAsStringFast("ticket_rem",node2,""),"TKNE")==0 &&
                NodeAsIntegerFast("ticket_confirm",node2)==0)
            {
              //возможно это произошло в ситуации, когда изменился у пульта defer_etstatus в true,
              //а пульт не успел перечитать эту настройку
              if (!new_checkin)
              {
                Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
                Qry.Execute();
                if (Qry.Eof ||
                    !(strcmp(NodeAsStringFast("ticket_rem",node2), Qry.FieldAsString("ticket_rem"))==0 &&
                      strcmp(NodeAsStringFast("ticket_no",node2), Qry.FieldAsString("ticket_no"))==0 &&
                      (NodeIsNULLFast("coupon_no",node2) && Qry.FieldIsNULL("coupon_no") ||
                       !NodeIsNULLFast("coupon_no",node2) && !Qry.FieldIsNULL("coupon_no") &&
                       NodeAsIntegerFast("coupon_no",node2)==Qry.FieldAsInteger("coupon_no"))
                     ))
                {
                  //билет отличается от ранее записанного
                  throw UserException("MSG.ETICK.NOT_CONFIRM.NEED_RELOGIN");
                };
              }
              else
                throw UserException("MSG.ETICK.NOT_CONFIRM.NEED_RELOGIN");
            };

            if (NodeIsNULLFast("pers_type",node2,true)) continue;
            if (strcmp(NodeAsStringFast("pers_type",node2),EncodePerson(ASTRA::baby))==0) continue; //младенцев не анализируем
            if (!new_checkin)
            {
              if (GetNodeFast("refuse",node2)!=NULL)
              {
                //были изменения в информации по пассажиру
                if (!NodeIsNULLFast("refuse",node2)) continue; //пассажир разрегистрирован
              }
              else
              {
                Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
                Qry.Execute();
                if (Qry.Eof || !Qry.FieldIsNULL("refuse")) continue; //пассажир разрегистрирован
              };
            };

            //билет
            if (checkTknInfo.required_fields!=0x0000)
            {
              //вычисляем маску по присутствию данных
              long int not_empty_fields=0x0000;
              if (!NodeIsNULLFast("ticket_no",node2,true)) not_empty_fields|=TKN_TICKET_NO_FIELD;
            
              if ((checkTknInfo.required_fields&not_empty_fields)!=checkTknInfo.required_fields)
                throw UserException("MSG.CHECKIN.PASSENGERS_TICKETS_NOT_SET"); //WEB
            };

            //документ
            if (checkDocInfo.first.required_fields!=0x0000 ||
                checkDocInfo.second.required_fields!=0x0000)
            {
              bool flagCBBG=strcmp(NodeAsStringFast("name",node2),"CBBG")==0;
              /*bool flagCBBG=false;
              remNode=GetNodeFast("rems",node2);
              if (remNode!=NULL)
                for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
                  if (strcmp(NodeAsStringFast("rem_code",remNode),"CBBG")==0)
                  {
                    flagCBBG=true;
                    break;
                  };*/
               if (!flagCBBG)
               {
                 if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCS_VERSION))
                 {
                   if (checkDocInfo.first.required_fields!=0x0000)
                   {
                     xmlNodePtr docNode=GetNodeFast("document",node2);
                     if (docNode!=NULL) docNode=docNode->children;

                     //вычисляем маску по присутствию данных
                     long int not_empty_fields=0x0000;
                     for(;docNode!=NULL;docNode=docNode->next)
                     {
                       if (NodeIsNULL(docNode)) continue;
                       map<string, long int>::const_iterator iMask=doc_node_names.find((char*)docNode->name);
                       if (iMask!=doc_node_names.end()) not_empty_fields|=iMask->second;
                     };

                     if ((checkDocInfo.first.required_fields&not_empty_fields)!=checkDocInfo.first.required_fields)
                     {

                       if (checkDocInfo.first.required_fields==DOC_NO_FIELD)
                         throw UserException("MSG.CHECKIN.PASSENGERS_DOCUMENTS_NOT_SET"); //WEB
                       else
                         throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOC_INFO_NOT_SET"); //WEB
                     };
                   };
                   if (checkDocInfo.second.required_fields!=0x0000)
                   {
                     if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCS_VERSION))
                     {
                       xmlNodePtr docNode=GetNodeFast("doco",node2);
                       if (docNode!=NULL) docNode=docNode->children;

                       //вычисляем маску по присутствию данных
                       long int not_empty_fields=0x0000;
                       for(;docNode!=NULL;docNode=docNode->next)
                       {
                         if (NodeIsNULL(docNode)) continue;
                         map<string, long int>::const_iterator iMask=doco_node_names.find((char*)docNode->name);
                         if (iMask!=doco_node_names.end()) not_empty_fields|=iMask->second;
                       };
                       if (not_empty_fields!=0x0000)
                       {
                         //пришла непустая информация о визе
                         if ((checkDocInfo.second.required_fields&not_empty_fields)!=checkDocInfo.second.required_fields)
                           throw UserException("MSG.CHECKIN.PASSENGERS_COMPLETE_DOCO_INFO_NOT_SET"); //WEB
                       };
                     };
                   };
                 }
                 else
                 {
                   //вычисляем маску по присутствию данных
                   long int not_empty_fields=0x0000;
                   if (!NodeIsNULLFast("document",node2,true)) not_empty_fields|=DOC_NO_FIELD;
                   
                   if ((checkDocInfo.first.required_fields&DOC_NO_FIELD&not_empty_fields)!=(checkDocInfo.first.required_fields&DOC_NO_FIELD))
                     throw UserException("MSG.CHECKIN.PASSENGERS_DOCUMENTS_NOT_SET"); //WEB
                 };
               };
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            node2=node->children;
            if (!NodeIsNULLFast("pax_id",node2))
              throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
            else
              throw;
          };
        };
      };
      
      if (reqInfo->client_type == ctTerm)
      {
        if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
        {
          hall=NodeAsInteger("hall",reqNode);
        }
        else
        {
          if (new_checkin)
          {
            hall=NodeAsInteger("hall",reqNode);
            JxtContext::getJxtContHandler()->sysContext()->write("last_hall_id", hall);
          }
          else
          {
            //попробуем считать зал из контекста пульта
            hall=JxtContext::getJxtContHandler()->sysContext()->readInt("last_hall_id", ASTRA::NoExists);
            if (hall==ASTRA::NoExists)
            {
              //ничего в контексте нет - берем зал из station_halls, но только если он один
              Qry.Clear();
              Qry.SQLText=
                "SELECT station_halls.hall "
                "FROM station_halls,stations "
                "WHERE station_halls.airp=stations.airp AND "
                "      station_halls.station=stations.name AND "
                "      stations.desk=:desk AND stations.work_mode=:work_mode";
              Qry.CreateVariable("desk",otString, TReqInfo::Instance()->desk.code);
              Qry.CreateVariable("work_mode",otString,"Р");
              Qry.Execute();
              if (!Qry.Eof)
              {
                hall=Qry.FieldAsInteger("hall");
                Qry.Next();
                if (!Qry.Eof) hall=ASTRA::NoExists;
              };
            };
            if (hall==ASTRA::NoExists)
            {
              //берем зал из pax_grp для этой группы
              Qry.Clear();
              Qry.SQLText="SELECT hall FROM pax_grp WHERE grp_id=:grp_id";
              Qry.CreateVariable("grp_id", otInteger, grp_id);
              Qry.Execute();
              if (!Qry.Eof && !Qry.FieldIsNULL("hall"))
                hall=Qry.FieldAsInteger("hall");
            };
          };
        };
      };
      
      //трансфер
      if (first_segment)
      {
        //получим трансфер
        if (new_checkin)
          save_trfer=true;
        else
        {
          save_trfer=false;
          if (reqInfo->desk.compatible(TRFER_CONFIRM_VERSION))
            save_trfer=GetNode("transfer",reqNode)!=NULL;
        };
        if (save_trfer)
        {
          if (!pr_unaccomp)
            ParseTransfer(GetNode("transfer",reqNode),
                          NodeAsNode("passengers",segNode),
                          s->second, trfer);
          else
            ParseTransfer(GetNode("transfer",reqNode),
                          NULL,
                          s->second, trfer);
        }
        else
        {
          //заполним trfer из базы
          LoadTransfer(grp_id, trfer);
        };
        iTrfer=trfer.begin();
      }
      else
      {
        if (iTrfer!=trfer.end())
        {
          TDateTime scd_local=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));
          modf(scd_local,&scd_local); //обрубаем часы
          
          //проверим совпадение сегментов трансфера и сквозной регистрации
          if (iTrfer->operFlt.airline!=fltInfo.airline ||
              iTrfer->operFlt.flt_no!=fltInfo.flt_no ||
              iTrfer->operFlt.suffix!=fltInfo.suffix ||
              iTrfer->operFlt.scd_out!=scd_local || //fltInfo.scd_out приводим к локальной дате
              iTrfer->operFlt.airp!=fltInfo.airp ||
              iTrfer->airp_arv!=airp_arv)
            throw UserException("MSG.CHECKIN.DIFFERENT_TCKIN_AND_TRFER_ROUTES");
          if (!pr_unaccomp && save_trfer)
          {
            //проверим подклассы
            xmlNodePtr paxNode=NodeAsNode("passengers",segNode)->children;
            vector<CheckIn::TPaxTransferItem>::const_iterator iPaxTrfer=iTrfer->pax.begin();
            for(;paxNode!=NULL && iPaxTrfer!=iTrfer->pax.end();paxNode=paxNode->next,iPaxTrfer++)
            {
              node2=paxNode->children;
              try
              {
                if (iPaxTrfer->subclass!=NodeAsStringFast("subclass",node2))
                  throw UserException("MSG.CHECKIN.DIFFERENT_TCKIN_AND_TRFER_SUBCLASSES");
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                node2=paxNode->children;
                if (!NodeIsNULLFast("pax_id",node2))
                  throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
                else
                  throw;
              };
            };
            if (paxNode!=NULL || iPaxTrfer!=iTrfer->pax.end())
              throw EXCEPTIONS::Exception("SavePax: Different number of transfer subclasses and passengers");
          };
          iTrfer++;
        };
      };

      TGrpToLogInfo grpInfoBefore;
      if (new_checkin)
      {
        cl=NodeAsString("class",segNode);

        bool addVIP=false;
        if (reqInfo->client_type == ctTerm)
        {
          if (first_segment)
          {
            Qry.Clear();
            Qry.SQLText="SELECT pr_vip FROM halls2 WHERE id=:hall";
            Qry.CreateVariable("hall",otInteger,hall);
            Qry.Execute();
            if (Qry.Eof) throw UserException("MSG.CHECKIN.INVALID_HALL");
            addVIP=Qry.FieldAsInteger("pr_vip")!=0;
          };
        };

        //новая регистрация
        //проверка наличия свободных мест
        TPaxStatus grp_status;
        if (first_segment)
        {
          grp_status=DecodePaxStatus(NodeAsString("status",segNode));
          if (grp_status==psTransit && !pr_tranz_reg)
            throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");
        }
        else
          grp_status=psTCheckin;

        string wl_type=NodeAsString("wl_type",segNode);

        SEATS2::TSalons Salons( point_dep, SEATS2::rTripSalons );

        if (!pr_unaccomp)
        {
          //простановка ремарок VIP,EXST, если нужно
          //подсчет seats
          bool adultwithbaby = false;
          int seats,seats_sum=0;
          string rem_code, rem_text;
          node=NodeAsNode("passengers",segNode);
          for(node=node->children;node!=NULL;node=node->next)
          {
            node2=node->children;
            try
            {
              seats=NodeAsIntegerFast("seats",node2);
              if ( !seats )
              	adultwithbaby = true;
              seats_sum+=seats;
              bool flagVIP=false,
                   flagSTCR=false,
                   flagEXST=false,
                   flagCBBG=false;
              remNode=GetNodeFast("rems",node2);
              if (remNode!=NULL)
                for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
                {
                  node2=remNode->children;
                  rem_code=NodeAsStringFast("rem_code",node2);
                  rem_text=NodeAsStringFast("rem_text",node2);
                  if (rem_code=="VIP") flagVIP=true;
                  if (rem_code=="STCR") flagSTCR=true;
                  if (rem_code=="EXST") flagEXST=true;
                  if (rem_code=="CBBG") flagCBBG=true;
                  //проверим корректность ремарки FQT...
                  TypeB::TFQTItem FQTItem;
                  CheckFQTRem(remNode,FQTItem);
                  //проверим запрещенные для ввода ремарки...
                  if (isDisabledRem(rem_code, rem_text))
                    throw UserException("MSG.REMARK.INPUT_CODE_DENIAL", LParams()<<LParam("remark",rem_code.empty()?rem_text.substr(0,5):rem_code));
                };
              if (addVIP && !flagVIP)
              {
                node2=node->children;
                if ((remNode=GetNodeFast("rems",node2))==NULL) remNode=NewTextChild(node,"rems");
                remNode=NewTextChild(remNode,"rem");
                NewTextChild(remNode,"rem_code","VIP");
                NewTextChild(remNode,"rem_text","VIP");
              };
              if (seats>1 && !flagEXST && !flagSTCR)
              {
                node2=node->children;
                if ((remNode=GetNodeFast("rems",node2))==NULL) remNode=NewTextChild(node,"rems");
                remNode=NewTextChild(remNode,"rem");
                NewTextChild(remNode,"rem_code","EXST");
                NewTextChild(remNode,"rem_text","EXST");
              };
              if (flagEXST && (seats<=1 || flagSTCR))
              {
                node2=node->children;
                remNode=GetNodeFast("rems",node2);
                if (remNode!=NULL)
                {
                  remNode=remNode->children;
                  while (remNode!=NULL)
                  {
                    node2=remNode->children;
                    rem_code=NodeAsStringFast("rem_code",node2);
                    if (rem_code=="EXST")
                    {
                      node2=remNode;
                      remNode=remNode->next;
                      xmlUnlinkNode(node2);
                      xmlFreeNode(node2);
                    }
                    else
                      remNode=remNode->next;
                  };
                };
              };
            }
            catch(CheckIn::UserException)
            {
              throw;
            }
            catch(UserException &e)
            {
              node2=node->children;
              if (!NodeIsNULLFast("pax_id",node2))
                throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
              else
                throw;
            };
          };
          if (wl_type.empty())
          {
            //группа регистрируется не на лист ожидания
            int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
            if (free<seats_sum)
              throw UserException("MSG.CHECKIN.AVAILABLE_SEATS",
                                  LParams()<<LParam("count",free)); //WEB

            node=NodeAsNode("passengers",segNode);

            SEATS2::Passengers.Clear();
            SEATS2::TSublsRems subcls_rems( fltInfo.airline );
            // начитка салона
            Salons.ClName = cl;
            Salons.Read();

            //заполним массив для рассадки
            for(node=node->children;node!=NULL;node=node->next)
            {
              node2=node->children;
              try
              {
                if (NodeAsIntegerFast("seats",node2)==0)
                	continue;
                const char *subclass=NodeAsStringFast("subclass",node2);

                SEATS2::TPassenger pas;

                pas.clname=cl;
                if ( !NodeIsNULLFast("pax_id",node2) )
                  pas.paxId = NodeAsIntegerFast( "pax_id", node2 );
                /*01.04.11 djek
                if (grp_status!=psTransit&&
                    !NodeIsNULLFast("pax_id",node2)&&
                    !NodeIsNULLFast("seat_no",node2)) {
                  pas.layer = cltPNLCkin;
                  pas.pax_id = NodeAsIntegerFast( "pax_id", node2 );
                }
                else {
                  pas.pax_id = 0;
                	if ( grp_status==psTransit ) {
                    pas.layer = cltProtTrzt;
                	}
                	else {
              		  pas.layer = cltUnknown;
                 }
                }*/
                switch ( grp_status )  {
                	case psCheckin:
                		pas.grp_status = cltCheckin;
                		break;
                	case psTCheckin:
                		pas.grp_status = cltTCheckin;
                		break;
                	case psTransit:
                		pas.grp_status = cltTranzit;
                		break;
                	case psGoshow:
                		pas.grp_status = cltGoShow;
                		break;
                }
                //pas.agent_seat=NodeAsStringFast("seat_no",node2); // crs or hand made
                pas.preseat_no=NodeAsStringFast("seat_no",node2); // crs or hand made
                pas.countPlace=NodeAsIntegerFast("seats",node2);
                pas.placeRem=NodeAsStringFast("seat_type",node2);
                remNode=GetNodeFast("rems",node2);
                pas.pers_type = NodeAsStringFast("pers_type",node2);
                bool flagCHIN=DecodePerson(pas.pers_type.c_str()) != ASTRA::adult;
                if (remNode!=NULL) {
                	for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next) {
               		  node2=remNode->children;
                    const char *rem_code=NodeAsStringFast("rem_code",node2);
                    if ( strcmp(rem_code,"BLND")==0 ||
                         strcmp(rem_code,"STCR")==0 ||
                         strcmp(rem_code,"UMNR")==0 ||
                         strcmp(rem_code,"WCHS")==0 ||
                         strcmp(rem_code,"MEDA")==0 ) flagCHIN=true;
                    pas.add_rem(rem_code);
                	}
                }
                string pass_rem;
                if ( subcls_rems.IsSubClsRem( subclass, pass_rem ) )  pas.add_rem(pass_rem);

                if ( flagCHIN || adultwithbaby ) {
                	adultwithbaby = false;
                	pas.add_rem("CHIN");
                }
                SEATS2::Passengers.Add(Salons,pas);
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                node2=node->children;
                if (!NodeIsNULLFast("pax_id",node2))
                  throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
                else
                  throw;
              };
            };
            //определим алгоритм рассадки
            SEATS2::TSeatAlgoParams algo=SEATS2::GetSeatAlgo(Qry,fltInfo.airline,fltInfo.flt_no,fltInfo.airp);
            //рассадка
            SEATS2::SeatsPassengers( &Salons, algo, SEATS2::Passengers );
            /*!!! иногда True - возможна рассажка на забронированные места, когда */
            /* есть право на регистрацию, статус рейса окончание, есть право сажать на чужие заброн. места */
          };
        };
        
        if (!pr_unaccomp)
        {
          //запишем коммерческий рейс
          xmlNodePtr markFltNode=GetNode("mark_flight",segNode);
          if (markFltNode!=NULL)
          {
            node2=markFltNode->children;

            markFltInfo.airline=NodeAsStringFast("airline",node2);
            markFltInfo.flt_no=NodeAsIntegerFast("flt_no",node2);
            markFltInfo.suffix=NodeAsStringFast("suffix",node2);
            markFltInfo.scd_out=NodeAsDateTimeFast("scd",node2);
            modf(markFltInfo.scd_out,&markFltInfo.scd_out);
            markFltInfo.airp=NodeAsStringFast("airp_dep",node2);
            pr_mark_norms=NodeAsIntegerFast("pr_mark_norms",node2)!=0;
            //получим локальную дату выполнения фактического рейса
            TDateTime scd_local=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));
            modf(scd_local,&scd_local);

            ostringstream flt;
            flt << markFltInfo.airline
                << setw(3) << setfill('0') << markFltInfo.flt_no
                << markFltInfo.suffix;
            if (markFltInfo.scd_out!=scd_local)
              flt << "/" << DateTimeToStr(markFltInfo.scd_out,"dd");
            if (markFltInfo.airp!=fltInfo.airp)
              flt << "/" << markFltInfo.airp;

            string str;
            TElemFmt fmt;

            str=ElemToElemId(etAirline,markFltInfo.airline,fmt);
            if (fmt==efmtUnknown)
              throw UserException("MSG.COMMERCIAL_FLIGHT.AIRLINE.UNKNOWN_CODE",
                                  LParams()<<LParam("airline",markFltInfo.airline)
                                           <<LParam("flight", flt.str()));  //WEB
            markFltInfo.airline=str;

            if (!markFltInfo.suffix.empty())
            {
              str=ElemToElemId(etSuffix,markFltInfo.suffix,fmt);
              if (fmt==efmtUnknown)
                throw UserException("MSG.COMMERCIAL_FLIGHT.SUFFIX.INVALID",
                                    LParams()<<LParam("suffix",markFltInfo.suffix)
                                             <<LParam("flight",flt.str())); //WEB
              markFltInfo.suffix=str;
            };

            str=ElemToElemId(etAirp,markFltInfo.airp,fmt);
            if (fmt==efmtUnknown)
              throw UserException("MSG.COMMERCIAL_FLIGHT.UNKNOWN_AIRP",
                                  LParams()<<LParam("airp",markFltInfo.airp)
                                           <<LParam("flight",flt.str())); //WEB
            markFltInfo.airp=str;
          };
        };

        Qry.Clear();
        Qry.SQLText=
          "DECLARE "
          "pass BINARY_INTEGER; "
          "BEGIN "
          "  FOR pass IN 1..2 LOOP "
          "    BEGIN "
          "      SELECT point_id INTO :point_id_mark FROM mark_trips "
          "      WHERE scd=:scd_mark AND airline=:airline_mark AND flt_no=:flt_no_mark AND airp_dep=:airp_dep_mark AND "
          "            (suffix IS NULL AND :suffix_mark IS NULL OR suffix=:suffix_mark) FOR UPDATE; "
          "      EXIT; "
          "    EXCEPTION "
          "      WHEN NO_DATA_FOUND THEN "
          "        IF :point_id_mark IS NULL OR pass=2 THEN "
          "          SELECT cycle_id__seq.nextval INTO :point_id_mark FROM dual; "
          "        END IF; "
          "        BEGIN "
          "          INSERT INTO mark_trips(point_id,airline,flt_no,suffix,scd,airp_dep) "
          "          VALUES (:point_id_mark,:airline_mark,:flt_no_mark,:suffix_mark,:scd_mark,:airp_dep_mark); "
          "          EXIT; "
          "        EXCEPTION "
          "          WHEN DUP_VAL_ON_INDEX THEN "
          "            IF pass=1 THEN NULL; ELSE RAISE; END IF; "
          "        END; "
          "    END; "
          "  END LOOP; "
          "  SELECT pax_grp__seq.nextval INTO :grp_id FROM dual; "
          "  INSERT INTO pax_grp(grp_id,point_dep,point_arv,airp_dep,airp_arv,class, "
          "                      status,excess,hall,bag_refuse,trfer_confirm,user_id,client_type, "
          "                      point_id_mark,pr_mark_norms,tid) "
          "  VALUES(:grp_id,:point_dep,:point_arv,:airp_dep,:airp_arv,:class, "
          "         :status,:excess,:hall,0,:trfer_confirm,:user_id,:client_type, "
          "         :point_id_mark,:pr_mark_norms,cycle_tid__seq.nextval); "
          "  IF :seg_no IS NOT NULL THEN "
          "    IF :seg_no=1 THEN :tckin_id:=:grp_id; END IF; "
          "    INSERT INTO tckin_pax_grp(tckin_id,seg_no,grp_id,pr_depend) "
          "    VALUES(:tckin_id,:seg_no,:grp_id,DECODE(:seg_no,1,0,1)); "
          "  END IF; "
          "END;";
        if (GetNode("generated_grp_id",segNode)!=NULL)
          Qry.CreateVariable("grp_id",otInteger,NodeAsInteger("generated_grp_id",segNode));
        else
          Qry.CreateVariable("grp_id",otInteger,FNull);
        Qry.CreateVariable("point_dep",otInteger,point_dep);
        Qry.CreateVariable("point_arv",otInteger,point_arv);
        Qry.CreateVariable("airp_dep",otString,airp_dep);
        Qry.CreateVariable("airp_arv",otString,airp_arv);
        Qry.CreateVariable("class",otString,cl);
        Qry.CreateVariable("status",otString,EncodePaxStatus(grp_status));
        if (first_segment)
          Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
        else
          Qry.CreateVariable("excess",otInteger,(int)0);
        if (hall!=ASTRA::NoExists)
          Qry.CreateVariable("hall",otInteger,hall);
        else
          Qry.CreateVariable("hall",otInteger,FNull);
        Qry.CreateVariable("trfer_confirm",otInteger,(int)(reqInfo->client_type==ctTerm));
        Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
       	Qry.CreateVariable("client_type",otString,EncodeClientType(reqInfo->client_type));
        if (first_segment)
          Qry.CreateVariable("tckin_id",otInteger,FNull);
        else
          Qry.CreateVariable("tckin_id",otInteger,tckin_id);
        if (only_one) //не сквозная регистрация
          Qry.CreateVariable("seg_no",otInteger,FNull);
        else
          Qry.CreateVariable("seg_no",otInteger,seg_no);
          
        if (IsMarkEqualOper(fltInfo, markFltInfo))
          Qry.CreateVariable("point_id_mark",otInteger,point_dep);
        else
          Qry.CreateVariable("point_id_mark",otInteger,FNull);
        Qry.CreateVariable("airline_mark",otString,markFltInfo.airline);
        Qry.CreateVariable("flt_no_mark",otInteger,markFltInfo.flt_no);
        Qry.CreateVariable("suffix_mark",otString,markFltInfo.suffix);
        Qry.CreateVariable("scd_mark",otDate,markFltInfo.scd_out);
        Qry.CreateVariable("airp_dep_mark",otString,markFltInfo.airp);
        Qry.CreateVariable("pr_mark_norms",otInteger,(int)pr_mark_norms);
          
        Qry.Execute();
        grp_id=Qry.GetVariableAsInteger("grp_id");
        if (first_segment)
          tckin_id=Qry.GetVariableAsInteger("tckin_id");

        ReplaceTextChild(segNode,"generated_grp_id",grp_id);


        if (!pr_unaccomp)
        {
          //получим рег. номера и признак совместной регистрации и посадки
          Qry.Clear();
          Qry.SQLText=
            "SELECT NVL(MAX(reg_no)+1,1) AS reg_no FROM pax_grp,pax "
            "WHERE pax_grp.grp_id=pax.grp_id AND point_dep=:point_dep";
          Qry.CreateVariable("point_dep",otInteger,point_dep);
          Qry.Execute();
          int reg_no = Qry.FieldAsInteger("reg_no");
          bool pr_brd_with_reg=false,pr_exam_with_brd=false;
          if (first_segment && reqInfo->client_type == ctTerm)
          {
            //при сквозной регистрации совместная регистрация с посадкой м.б. только на первом рейса
            //при web-регистрации посадка строго раздельная
            Qry.Clear();
            Qry.SQLText=
              "SELECT pr_misc FROM trip_hall "
              "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
              "ORDER BY DECODE(hall,NULL,1,0)";
            Qry.CreateVariable("point_id",otInteger,point_dep);
            Qry.CreateVariable("hall",otInteger,hall);
            Qry.DeclareVariable("type",otInteger);

            Qry.SetVariable("type",1);
            Qry.Execute();
            if (!Qry.Eof) pr_brd_with_reg=Qry.FieldAsInteger("pr_misc")!=0;
            Qry.SetVariable("type",2);
            Qry.Execute();
            if (!Qry.Eof) pr_exam_with_brd=Qry.FieldAsInteger("pr_misc")!=0;
          };

          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  IF :pax_id IS NULL THEN "
            "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
            "  END IF; "
            "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,seat_type,seats,pr_brd, "
            "                  wl_type,refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm, "
            "                  pr_exam,subclass,bag_pool_num,tid) "
            "  VALUES(:pax_id,pax_grp__seq.currval,:surname,:name,:pers_type,:seat_type,:seats,:pr_brd, "
            "         :wl_type,NULL,:reg_no,:ticket_no,:coupon_no,:ticket_rem,:ticket_confirm, "
            "         :pr_exam,:subclass,:bag_pool_num,cycle_tid__seq.currval); "
            "END;";
          Qry.DeclareVariable("pax_id",otInteger);
          Qry.DeclareVariable("surname",otString);
          Qry.DeclareVariable("name",otString);
          Qry.DeclareVariable("pers_type",otString);
          Qry.DeclareVariable("seat_type",otString);
          Qry.DeclareVariable("seats",otInteger);
          Qry.DeclareVariable("pr_brd",otInteger);
          Qry.DeclareVariable("pr_exam",otInteger);
          Qry.DeclareVariable("wl_type",otString);
          Qry.DeclareVariable("reg_no",otInteger);
          Qry.DeclareVariable("ticket_no",otString);
          Qry.DeclareVariable("coupon_no",otInteger);
          Qry.DeclareVariable("ticket_rem",otString);
          Qry.DeclareVariable("ticket_confirm",otInteger);
          Qry.DeclareVariable("subclass",otString);
          Qry.DeclareVariable("bag_pool_num",otInteger);
          int i=0;
          bool change_agent_seat_no = false;
          bool change_preseat_no = false;
          bool exists_preseats = false; //есть ли у группы пассажиров предварительные места
          bool invalid_seat_no = false; //есть запрещенные места
          for(int k=0;k<=1;k++)
          {
            node=NodeAsNode("passengers",segNode);
            int pax_no=1;
            for(node=node->children;node!=NULL;node=node->next,pax_no++)
            {
              node2=node->children;
              try
              {
                int seats=NodeAsIntegerFast("seats",node2);
                if (seats<=0&&k==0||seats>0&&k==1) continue;
                const char* surname=NodeAsStringFast("surname",node2);
                const char* name=NodeAsStringFast("name",node2);
                const char* pers_type=NodeAsStringFast("pers_type",node2);
                if (!NodeIsNULLFast("pax_id",node2))
                  Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
                else
                {
                  if (GetNodeFast("generated_pax_id",node2)!=NULL)
                    Qry.SetVariable("pax_id",NodeAsIntegerFast("generated_pax_id",node2));
                  else
                    Qry.SetVariable("pax_id",FNull);
                };
                Qry.SetVariable("surname",surname);
                Qry.SetVariable("name",name);
                Qry.SetVariable("pers_type",pers_type);
                if (seats>0)
                  Qry.SetVariable("seat_type",NodeAsStringFast("seat_type",node2));
                else
                  Qry.SetVariable("seat_type",FNull);
                Qry.SetVariable("seats",seats);
                Qry.SetVariable("pr_brd",(int)pr_brd_with_reg);
                Qry.SetVariable("pr_exam",(int)(pr_brd_with_reg && pr_exam_with_brd));
                if (seats>0)
                  Qry.SetVariable("wl_type",wl_type);
                else
                  Qry.SetVariable("wl_type",FNull);
                Qry.SetVariable("reg_no",reg_no);
                CheckIn::TPaxTknItem().fromXML(node).toDB(Qry);
                Qry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
                if (reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
                {
                  if (!NodeIsNULLFast("bag_pool_num",node2))
                    Qry.SetVariable("bag_pool_num",NodeAsIntegerFast("bag_pool_num",node2));
                  else
                    Qry.SetVariable("bag_pool_num",FNull);
                }
                else Qry.SetVariable("bag_pool_num",FNull);

                try
                {
                  Qry.Execute();
                }
                catch(EOracleError E)
                {
                  if (E.Code==1)
                    throw UserException("MSG.PASSENGER.CHECKED.ALREADY_OTHER_DESK",
                                        LParams()<<LParam("surname",string(surname)+(*name!=0?" ":"")+name)); //WEB
                  else
                    throw;
                };
                int pax_id=Qry.GetVariableAsInteger("pax_id");
                ReplaceTextChild(node,"generated_pax_id",pax_id);

                if (wl_type.empty())
                {
                  //запись номеров мест
                  if (seats>0 && i<SEATS2::Passengers.getCount())
                  {
                    SEATS2::TPassenger pas = SEATS2::Passengers.Get(i);
               		  if ( pas.preseat_pax_id > 0 )
               		    exists_preseats = true;
                		if ( !pas.isValidPlace )
                 			invalid_seat_no = true;

                    if (pas.seat_no.empty()) throw EXCEPTIONS::Exception("SeatsPassengers: empty seat_no");
                    	string pas_seat_no;
                    	bool pr_found_preseat_no = false;
                    	bool pr_found_agent_no = false;
                    	for( std::vector<TSeat>::iterator iseat=pas.seat_no.begin(); iseat!=pas.seat_no.end(); iseat++ ) {
                    	  pas_seat_no = denorm_iata_row( iseat->row, NULL ) + denorm_iata_line( iseat->line, Salons.getLatSeat() );
                        if ( pas_seat_no == pas.preseat_no ) {
                          pr_found_preseat_no = true;
                        }
                        if ( pas_seat_no == NodeAsStringFast("seat_no",node2) )
                          pr_found_agent_no = true;
                      }
                      if ( !string(NodeAsStringFast("seat_no",node2)).empty() &&
                           !pr_found_agent_no ) {
                        change_agent_seat_no = true;
                      }
                      if ( pas.preseat_pax_id > 0 &&
                           !pas.preseat_no.empty() && !pr_found_preseat_no ) {
                        change_preseat_no = true;
                      }

                    vector<TSeatRange> ranges;
                    for(vector<TSeat>::iterator iSeat=pas.seat_no.begin();iSeat!=pas.seat_no.end();iSeat++)
                    {
                      TSeatRange range(*iSeat,*iSeat);
                      ranges.push_back(range);
                    };
                    ProgTrace( TRACE5, "ranges.size=%d", ranges.size() );
                    //запись в базу
                    TCompLayerType layer_type;
                    switch( grp_status ) {
                    	case psCheckin:
                    		layer_type = cltCheckin;
                    		break;
                    	case psTCheckin:
                    		layer_type = cltTCheckin;
                    		break;
                    	case psGoshow:
                    		layer_type = cltGoShow;
                    		break;
                    	case psTransit:
                    		layer_type = cltTranzit;
                    		break;
                    }
                    SEATS2::SaveTripSeatRanges( point_dep, layer_type, ranges, pax_id, point_dep, point_arv );
                    i++;
                  };
                  if ( invalid_seat_no )
                      AstraLocale::showErrorMessage("MSG.SEATS.PASSENGERS_FORBIDDEN_PLACES");
                  else
                		if ( change_agent_seat_no && exists_preseats && !change_preseat_no )
               	  		AstraLocale::showErrorMessage("MSG.SEATS.PASSENGERS_PRESEAT_PLACES");
                  	else
                  	  if ( change_agent_seat_no || change_preseat_no )
                    		  AstraLocale::showErrorMessage("MSG.SEATS.PART_REQUIRED_PLACES_NOT_AVAIL");
                };

                //запись pax_doc
                if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCS_VERSION))
                {
                  CheckIn::SavePaxDoc(pax_id,GetNodeFast("document",node2),PaxDocQry);
                  CheckIn::SavePaxDoco(pax_id,GetNodeFast("doco",node2),PaxDocoQry);
                }
                else
                {
                  CrsQry.SetVariable("pax_id",pax_id);
                  CrsQry.SetVariable("document",NodeAsStringFast("document",node2));
                  CrsQry.Execute();
                };

                if (save_trfer)
                {
                  //запись подклассов трансфера
                  SavePaxTransfer(pax_id,pax_no,trfer,seg_no);
                };
                //запись ремарок
                SavePaxRem(node);
                //запись норм
                if (first_segment) CheckIn::SaveNorms(node,pr_unaccomp);
                reg_no++;
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                node2=node->children;
                if (!NodeIsNULLFast("pax_id",node2))
                  throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
                else
                  throw;
              };
            };
          };
        }
        else
        {
          //несопровождаемый багаж
          //запись норм
          if (first_segment) CheckIn::SaveNorms(segNode,pr_unaccomp);
        };
        
        TLogMsg msg;
        msg.ev_type=ASTRA::evtPax;
        msg.id1=point_dep;
        msg.id2=NoExists;
        msg.id3=grp_id;
        if (save_trfer)
        {
          msg.msg=SaveTransfer(grp_id,trfer,pr_unaccomp,seg_no);
          if (!msg.msg.empty()) reqInfo->MsgToLog(msg);
        };
        msg.msg=SaveTCkinSegs(grp_id,reqNode,segs,seg_no);
        if (!msg.msg.empty()) reqInfo->MsgToLog(msg);
      }
      else
      {
        GetGrpToLogInfo(grp_id, grpInfoBefore); //для всех сегментов
        //BSM
        if (BSMsend)
          BSM::LoadContent(grp_id,BSMContentBefore);
      
        //запись изменений
        bool save_trfer=false;
        if (reqInfo->desk.compatible(TRFER_CONFIRM_VERSION))
          save_trfer=GetNode("transfer",reqNode)!=NULL;

        if (first_segment && reqInfo->client_type == ctTerm)
        {
          SeparateTCkin(grp_id,cssAllPrevCurr,cssNone,-1,tckin_id,tckin_seg_no);
        };
        
        Qry.Clear();
        Qry.SQLText=
          "UPDATE pax_grp "
          "SET excess=:excess, "
          "    bag_refuse=:bag_refuse, "
          "    trfer_confirm=NVL(:trfer_confirm,trfer_confirm), "
          "    tid=cycle_tid__seq.nextval "
          "WHERE grp_id=:grp_id AND tid=:tid";
        Qry.CreateVariable("grp_id",otInteger,grp_id);
        Qry.CreateVariable("tid",otInteger,NodeAsInteger("tid",segNode));
        if (first_segment)
          Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
        else
          Qry.CreateVariable("excess",otInteger,(int)0);
        Qry.CreateVariable("bag_refuse",otInteger,(int)(!NodeIsNULL("bag_refuse",reqNode)));
        if (reqInfo->client_type==ctTerm)
          Qry.CreateVariable("trfer_confirm",otInteger,(int)true);
        else
          Qry.CreateVariable("trfer_confirm",otInteger,FNull);
        Qry.Execute();
        if (Qry.RowsProcessed()<=0)
          throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

        if (!pr_unaccomp)
        {
          //читаем коммерческий рейс
          Qry.Clear();
          Qry.SQLText=
            "SELECT mark_trips.airline,mark_trips.flt_no,mark_trips.suffix, "
            "       mark_trips.scd AS scd_out,mark_trips.airp_dep AS airp,pr_mark_norms "
            "FROM pax_grp,mark_trips "
            "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id";
          Qry.CreateVariable("grp_id",otInteger,grp_id);
          Qry.Execute();
          if (!Qry.Eof)
          {
            markFltInfo.Init(Qry);
            pr_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
          };

          TQuery PaxQry(&OraSession);
          PaxQry.Clear();
          PaxQry.SQLText="UPDATE pax "
                         "SET surname=:surname, "
                         "    name=:name, "
                         "    pers_type=:pers_type, "
                         "    refuse=:refuse, "
                         "    ticket_no=:ticket_no, "
                         "    coupon_no=:coupon_no, "
                         "    ticket_rem=:ticket_rem, "
                         "    ticket_confirm=:ticket_confirm, "
                         "    subclass=:subclass, "
                         "    bag_pool_num=:bag_pool_num, "
                         "    pr_brd=DECODE(:refuse,NULL,pr_brd,NULL), "
                         "    pr_exam=DECODE(:refuse,NULL,pr_exam,0), "
                         "    tid=cycle_tid__seq.currval "
                         "WHERE pax_id=:pax_id AND tid=:tid";
          PaxQry.DeclareVariable("pax_id",otInteger);
          PaxQry.DeclareVariable("tid",otInteger);
          PaxQry.DeclareVariable("surname",otString);
          PaxQry.DeclareVariable("name",otString);
          PaxQry.DeclareVariable("pers_type",otString);
          PaxQry.DeclareVariable("refuse",otString);
          PaxQry.DeclareVariable("ticket_no",otString);
          PaxQry.DeclareVariable("coupon_no",otInteger);
          PaxQry.DeclareVariable("ticket_rem",otString);
          PaxQry.DeclareVariable("ticket_confirm",otInteger);
          PaxQry.DeclareVariable("subclass",otString);
          PaxQry.DeclareVariable("bag_pool_num",otInteger);

          TQuery LayerQry(&OraSession);
          LayerQry.Clear();
          LayerQry.SQLText=
            "DELETE FROM trip_comp_layers WHERE pax_id=:pax_id AND layer_type="
            "(SELECT layer_type FROM pax,pax_grp,grp_status_types "
            "  WHERE pax.pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND "
            "        pax_grp.status=grp_status_types.code)";
          /*а можно так:
          DELETE FROM
          (SELECT *
           FROM trip_comp_layers,pax,pax_grp,grp_status_types
           WHERE trip_comp_layers.pax_id=pax.pax_id AND
                 trip_comp_layers.layer_type=grp_status_types.layer_type AND
                 pax.grp_id=pax_grp.grp_id AND
                 pax_grp.status=grp_status_types.code AND
                 pax.pax_id=:pax_id)*/

          LayerQry.DeclareVariable("pax_id",otInteger);

          if (reqInfo->desk.compatible(TRFER_CONFIRM_VERSION))
            node=NodeAsNode("passengers",segNode);
          else
            node=GetNode("passengers",segNode);
          if (node!=NULL)
          {
            //только в случае записи трансфера мы передаем всех пассажиров всех сегментов с терминала
            int pax_no=1;
            for(node=node->children;node!=NULL;node=node->next,pax_no++)
            {
              node2=node->children;
              int pax_id=NodeAsIntegerFast("pax_id",node2);
              try
              {
                const char* surname=NodeAsStringFast("surname",node2);
                const char* name=NodeAsStringFast("name",node2);
                if (GetNodeFast("refuse",node2)!=NULL)
                {
                  //были изменения в информации по пассажиру
                  if (!NodeIsNULLFast("refuse",node2))
                  {
                    LayerQry.SetVariable("pax_id",pax_id);
                    LayerQry.Execute();
                  };
                  const char* pers_type=NodeAsStringFast("pers_type",node2);
                  const char* refuse=NodeAsStringFast("refuse",node2);
                  PaxQry.SetVariable("pax_id",pax_id);
                  PaxQry.SetVariable("tid",NodeAsIntegerFast("tid",node2));
                  PaxQry.SetVariable("surname",surname);
                  PaxQry.SetVariable("name",name);
                  PaxQry.SetVariable("pers_type",pers_type);
                  PaxQry.SetVariable("refuse",refuse);
                  CheckIn::TPaxTknItem().fromXML(node).toDB(PaxQry);
                  PaxQry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
                  if (reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
                  {
                    if (!NodeIsNULLFast("bag_pool_num",node2))
                      PaxQry.SetVariable("bag_pool_num",NodeAsIntegerFast("bag_pool_num",node2));
                    else
                      PaxQry.SetVariable("bag_pool_num",FNull);
                  }
                  else PaxQry.SetVariable("bag_pool_num",FNull);
                  PaxQry.Execute();
                  if (PaxQry.RowsProcessed()<=0)
                    throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                        LParams()<<LParam("surname",string(surname)+(*name!=0?" ":"")+name)); //WEB

                  //запись pax_doc
                  if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCS_VERSION))
                  {
                    CheckIn::SavePaxDoc(pax_id,GetNodeFast("document",node2),PaxDocQry);
                    CheckIn::SavePaxDoco(pax_id,GetNodeFast("doco",node2),PaxDocoQry);
                  }
                  else
                  {
                    CrsQry.SetVariable("pax_id",pax_id);
                    CrsQry.SetVariable("document",NodeAsStringFast("document",node2));
                    CrsQry.Execute();
                  };
                }
                else
                {
                  Qry.Clear();
                  Qry.SQLText="UPDATE pax SET tid=cycle_tid__seq.currval WHERE pax_id=:pax_id AND tid=:tid";
                  Qry.CreateVariable("pax_id",otInteger,pax_id);
                  Qry.CreateVariable("tid",otInteger,NodeAsIntegerFast("tid",node2));
                  Qry.Execute();
                  if (Qry.RowsProcessed()<=0)
                    throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                        LParams()<<LParam("surname",string(surname)+(*name!=0?" ":"")+name)); //WEB
                };
                
                if (save_trfer)
                {
                  //запись подклассов трансфера
                  SavePaxTransfer(pax_id,pax_no,trfer,seg_no);
                };
                //запись ремарок
                SavePaxRem(node);
                //запись норм
                if (first_segment) CheckIn::SaveNorms(node,pr_unaccomp);
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                throw CheckIn::UserException(e.getLexemaData(), point_dep, pax_id);
              };
            };
          };
        }
        else
        {
          //запись норм
          if (first_segment) CheckIn::SaveNorms(segNode,pr_unaccomp);
        };
        
        if (save_trfer)
        {
          TLogMsg msg;
          msg.ev_type=ASTRA::evtPax;
          msg.id1=point_dep;
          msg.id2=NoExists;
          msg.id3=grp_id;
          msg.msg=SaveTransfer(grp_id,trfer,pr_unaccomp,seg_no);
          if (!msg.msg.empty()) reqInfo->MsgToLog(msg);
        };
      };

      if (first_segment)
      {
        CheckIn::SaveBag(point_dep,grp_id,hall,reqNode);
        CheckInInterface::SavePaidBag(grp_id,reqNode);
        SaveTagPacks(reqNode);
        first_grp_id=grp_id;
      }
      else
      {
        //запросом записываем в остальные группы багаж, бирки, ценный багаж
        if (seg_no-1<=(int)trfer.size())
        {
          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  DELETE FROM value_bag WHERE grp_id=:grp_id; "
            "  DELETE FROM bag2 WHERE grp_id=:grp_id; "
            "  DELETE FROM paid_bag WHERE grp_id=:grp_id; "
            "  DELETE FROM bag_tags WHERE grp_id=:grp_id; "
            "  INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax,tax_trfer) "
            "  SELECT :grp_id ,num,value,value_cur,NULL,NULL,NULL "
            "  FROM value_bag WHERE grp_id=:first_grp_id; "
            "  INSERT INTO bag2(grp_id,num,id,bag_type,pr_cabin,amount,weight,value_bag_num,pr_liab_limit,bag_pool_num,hall,user_id) "
            "  SELECT :grp_id,num,id,99,pr_cabin,amount,weight,value_bag_num,pr_liab_limit,bag_pool_num,hall,user_id "
            "  FROM bag2 WHERE grp_id=:first_grp_id; "
            "  IF SQL%FOUND THEN "
            "    INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id,rate_trfer) "
            "    VALUES(:grp_id,99,0,NULL,NULL); "
            "  END IF; "
            "  INSERT INTO bag_tags(grp_id,num,tag_type,no,color,seg_no,bag_num,pr_print) "
            "  SELECT :grp_id,num,tag_type,no,color,:seg_no,bag_num,pr_print "
            "  FROM bag_tags WHERE grp_id=:first_grp_id; "
            "END; ";
          Qry.CreateVariable("grp_id",otInteger,grp_id);
          Qry.CreateVariable("first_grp_id",otInteger,first_grp_id);
          Qry.CreateVariable("seg_no",otInteger,tckin_seg_no);
          Qry.Execute();
        };
      };
      if (!reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
      {
        if (!pr_unaccomp)
        {
          Qry.Clear();
          Qry.SQLText=
            "DECLARE "
            "  new_main_pax_id pax.pax_id%TYPE; "
            "BEGIN "
            "  UPDATE bag2 SET bag_pool_num=1 WHERE grp_id=:grp_id; "
            "  IF SQL%FOUND THEN "
            "    SELECT ckin.get_main_pax_id(:grp_id) INTO new_main_pax_id FROM dual; "
            "  ELSE "
            "    new_main_pax_id:=NULL; "
            "  END IF; "
            "  UPDATE pax "
            "  SET bag_pool_num=DECODE(pax_id,new_main_pax_id,1,NULL), "
            "      tid=DECODE(bag_pool_num,DECODE(pax_id,new_main_pax_id,1,NULL),tid,cycle_tid__seq.currval) "
            "  WHERE grp_id=:grp_id; "
            "END;";
          Qry.CreateVariable("grp_id",otInteger,grp_id);
          Qry.Execute();
        };
      };

      if (!pr_unaccomp)
      {
        //проверим дублирование билетов
        Qry.Clear();
        Qry.SQLText=
          "SELECT ticket_no,coupon_no FROM pax "
          "WHERE refuse IS NULL AND ticket_no=:ticket_no AND coupon_no=:coupon_no "
          "GROUP BY ticket_no,coupon_no HAVING COUNT(*)>1";
        Qry.DeclareVariable("ticket_no",otString);
        Qry.DeclareVariable("coupon_no",otInteger);
        node=NodeAsNode("passengers",segNode);
        for(node=node->children;node!=NULL;node=node->next)
        {
          node2=node->children;
          try
          {
            if (NodeIsNULLFast("ticket_no",node2,true)||
                NodeIsNULLFast("coupon_no",node2,true)) continue;

            Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
            Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
            Qry.Execute();
            if (!Qry.Eof)
              throw UserException("MSG.CHECKIN.DOUPLICATED_ETICKET",
                                  LParams()<<LParam("eticket",NodeAsStringFast("ticket_no",node2))
                                           <<LParam("coupon",NodeAsStringFast("coupon_no",node2))); //WEB
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            node2=node->children;
            if (!NodeIsNULLFast("pax_id",node2))
              throw CheckIn::UserException(e.getLexemaData(), point_dep, NodeAsIntegerFast("pax_id",node2));
            else
              throw;
          };
        };
        if (ediResNode!=NULL)
        {
          //изменим ticket_confirm и events на основе подтвержденных статусов
          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  IF :pax_id IS NOT NULL THEN "
            "    UPDATE pax SET ticket_confirm=1,tid=cycle_tid__seq.currval "
            "    WHERE pax_id=:pax_id AND "
            "          ticket_rem=:ticket_rem AND ticket_no=:ticket_no AND coupon_no=:coupon_no "
            "    RETURNING grp_id,reg_no INTO :grp_id,:reg_no; "
            "    IF :grp_id IS NOT NULL AND :reg_no IS NOT NULL AND "
            "       :ev_time IS NOT NULL AND :ev_order IS NOT NULL THEN "
            "      DELETE FROM events WHERE time=:ev_time AND ev_order=:ev_order; "
            "    END IF; "
            "  END IF; "
            "END; ";
          Qry.DeclareVariable("pax_id",otInteger);
          Qry.DeclareVariable("grp_id",otInteger);
          Qry.DeclareVariable("reg_no",otInteger);
          Qry.DeclareVariable("ticket_no",otString);
          Qry.DeclareVariable("coupon_no",otInteger);
          Qry.DeclareVariable("ev_time",otDate);
          Qry.DeclareVariable("ev_order",otInteger);
          Qry.CreateVariable("ticket_rem",otString,"TKNE");

          xmlNodePtr ticketNode=NodeAsNode("tickets",ediResNode)->children;
          for(;ticketNode!=NULL;ticketNode=ticketNode->next)
          {
            xmlNodePtr node2=ticketNode->children;
            if (GetNodeFast("coupon_status",node2)==NULL) continue;
            if (NodeAsIntegerFast("point_id",node2)!=point_dep) continue;

            Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
            Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));

            if (GetNodeFast("pax_id",node2)!=NULL)
              Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
            else
              Qry.SetVariable("pax_id",FNull);

            Qry.SetVariable("grp_id",FNull);
            Qry.SetVariable("reg_no",FNull);

            xmlNodePtr eventNode=GetNode("coupon_status/event",ticketNode);
            if (eventNode!=NULL &&
                GetNodeFast("reg_no",node2)==NULL &&
                GetNode("@ev_time",eventNode)!=NULL &&
                GetNode("@ev_order",eventNode)!=NULL)
            {
              Qry.SetVariable("ev_time",NodeAsDateTime("@ev_time",eventNode));
              Qry.SetVariable("ev_order",NodeAsInteger("@ev_order",eventNode));
            }
            else
            {
              Qry.SetVariable("ev_time",FNull);
              Qry.SetVariable("ev_order",FNull);
            };
            Qry.Execute();
            if (eventNode!=NULL &&
                GetNodeFast("reg_no",node2)==NULL &&
                !Qry.VariableIsNULL("reg_no") &&
                !Qry.VariableIsNULL("grp_id"))
            {
              TLogMsg msg;
              msg.ev_type=ASTRA::evtPax;
              msg.id1=point_dep;
              msg.id2=Qry.GetVariableAsInteger("reg_no");
              msg.id3=Qry.GetVariableAsInteger("grp_id");
              msg.msg=NodeAsString(eventNode);
              reqInfo->MsgToLog(msg);
            };
          };
        };
      };

      //вот здесь ETCheckStatus::CheckGrpStatus
      //обязательно до ckin.check_grp
      if (ediResNode==NULL && !defer_etstatus &&
          (new_checkin || reqInfo->client_type==ctTerm)) //не производим изменение статуса при записи изменений веб регистрации!
      {
        if (ETStatusInterface::ETCheckStatus(grp_id,csaGrp,-1,false,ETInfo,true))
        {
          et_processed=true; //хотя бы один билет будет обрабатываться
        };
      };

      if (!et_processed)
      {
        if (agent_stat_point_id==NoExists) agent_stat_point_id=point_dep;
      
        //записываем в лог только если не будет отката транзакции из-за обращения к СЭБ
        TGrpToLogInfo grpInfoAfter;
        GetGrpToLogInfo(grp_id, grpInfoAfter);
        TAgentStatInfo agentStat;
        SaveGrpToLog(point_dep, fltInfo, markFltInfo, grpInfoBefore, grpInfoAfter, agentStat);
        if (reqInfo->client_type==ctTerm &&
            reqInfo->desk.compatible(AGENT_STAT_VERSION))
        {
          int agent_stat_period=NodeAsInteger("agent_stat_period",reqNode);
          if (agent_stat_period<1) agent_stat_period=1;
          //собираем агентскую статистику
          STAT::agent_stat_delta(agent_stat_point_id,
                                 reqInfo->user.user_id,
                                 reqInfo->desk.code,
                                 agent_stat_ondate,
                                 first_segment?agent_stat_period:0,
                                 agentStat.pax_amount,
                                 agentStat.dpax_amount,
                                 first_segment?STAT::agent_stat_t(0,0):agentStat.dpax_amount,
                                 first_segment?agentStat.dbag_amount:STAT::agent_stat_t(0,0),
                                 first_segment?agentStat.dbag_weight:STAT::agent_stat_t(0,0),
                                 first_segment?agentStat.drk_amount:STAT::agent_stat_t(0,0),
                                 first_segment?agentStat.drk_weight:STAT::agent_stat_t(0,0));
        };
      };

      //обновление counters
      Qry.Clear();
      Qry.SQLText=
        "BEGIN "
        "  mvd.sync_pax_grp(:grp_id,:desk); "
        "  ckin.check_grp(:grp_id); "
        "  ckin.recount(:point_id); "
        "END;";
      Qry.CreateVariable("point_id",otInteger,point_dep);
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.CreateVariable("desk",otString,reqInfo->desk.code);
      Qry.Execute();
      Qry.Close();

      //проверим максимальную загрузку
      bool overload_alarm = Calc_overload_alarm( point_dep, fltInfo ); // вычислили признак перегрузки
      
      if (overload_alarm)
      {
        //проверяем, а не было ли полной отмены регистрации группы по ошибке агента
        Qry.Clear();
        Qry.SQLText="SELECT grp_id FROM pax_grp WHERE grp_id=:grp_id";
        Qry.CreateVariable("grp_id",otInteger,grp_id);
        Qry.Execute();
        if (!Qry.Eof)
        {
          try
          {
            if (CheckFltOverload(point_dep,fltInfo,overload_alarm))
            {
              //работает если разрешена регистрация при превышении загрузки
              //продолжаем регистрацию и зажигаем тревогу
              Set_AODB_overload_alarm( point_dep, true );
            };
          }
          catch(OverloadException &E)
          {
            //работает если запрещена регистрация при превышении загрузки
            //откатываем регистрацию не снимая лочки с рейса и зажигаем тревогу
            //при сквозной регистрации будет зажжена тревога на первом перегруженном сегменте
            Qry.Clear();
            Qry.SQLText=
              "BEGIN "
              "  ROLLBACK TO CHECKIN; "
              "END;";
            Qry.Execute();

            if (reqInfo->client_type==ctTerm)
            {
              if (!only_one)
                showError( GetLexemeDataWithFlight(E.getLexemaData( ), fltInfo) );
              else
                showError( E.getLexemaData( ) );
            }
            else
            {
              //веб, киоски
              CheckIn::UserException ce(E.getLexemaData(), point_dep);
              CheckIn::showError(ce.segs);
            };
            
            Set_overload_alarm( point_dep, true ); // установили признак перегрузки несмотря на то что реальной перегрузки нет
            Set_AODB_overload_alarm( point_dep, true );
            return false;
          };
        };
      };
      
      Set_overload_alarm( point_dep, overload_alarm ); // установили признак перегрузки
      

      if (!pr_unaccomp)
      {
        //посчитаем индекс группы подклассов
        TQuery PaxQry(&OraSession);
        PaxQry.Clear();
        PaxQry.SQLText=
          "SELECT subclass,class FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax.grp_id=:grp_id "
          "ORDER BY reg_no";
        PaxQry.CreateVariable("grp_id",otInteger,grp_id);
        PaxQry.Execute();
        if (!PaxQry.Eof)
        {
          Qry.Clear();
          Qry.SQLText=
            "SELECT MAX(DECODE(airline,NULL,0,4)+ "
            "           DECODE(airp,NULL,0,2)) AS priority "
            "FROM cls_grp "
            "WHERE (airline IS NULL OR airline=:airline) AND "
            "      (airp IS NULL OR airp=:airp) AND "
            "      pr_del=0";
          Qry.CreateVariable("airline",otString,fltInfo.airline);
          Qry.CreateVariable("airp",otString,fltInfo.airp);
          Qry.Execute();
          if (Qry.Eof||Qry.FieldIsNULL("priority"))
          {
            ProgError(STDLOG,"Class group not found (airline=%s, airp=%s)",fltInfo.airline.c_str(),fltInfo.airp.c_str());
            throw UserException("MSG.CHECKIN.NOT_MADE_IN_ONE_CLASSES"); //WEB

          };
          int priority=Qry.FieldAsInteger("priority");


          int class_grp=-1;
          for(;!PaxQry.Eof;PaxQry.Next())
          {
            if (!PaxQry.FieldIsNULL("subclass"))
            {
              Qry.Clear();
              Qry.SQLText=
                "SELECT cls_grp.id "
                "FROM cls_grp,subcls_grp "
                "WHERE cls_grp.id=subcls_grp.id AND "
                "      (airline IS NULL OR airline=:airline) AND "
                "      (airp IS NULL OR airp=:airp) AND "
                "      DECODE(airline,NULL,0,4)+ "
                "      DECODE(airp,NULL,0,2) = :priority AND "
                "      subcls_grp.subclass=:subclass AND "
                "      cls_grp.pr_del=0";
              Qry.CreateVariable("airline",otString,fltInfo.airline);
              Qry.CreateVariable("airp",otString,fltInfo.airp);
              Qry.CreateVariable("priority",otInteger,priority);
              Qry.CreateVariable("subclass",otString,PaxQry.FieldAsString("subclass"));
              Qry.Execute();
              if (!Qry.Eof)
              {
                if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
                else
                  if (class_grp!=Qry.FieldAsInteger("id"))
                    throw UserException("MSG.CHECKIN.INPOSSIBLE_SUBCLASS_IN_GROUP");
                Qry.Next();
                if (!Qry.Eof)
                  throw EXCEPTIONS::Exception("More than one class group found (airline=%s, airp=%s, subclass=%s)",
                                              fltInfo.airline.c_str(),fltInfo.airp.c_str(),PaxQry.FieldAsString("subclass"));
                continue;
              };
            };

            Qry.Clear();
            Qry.SQLText=
              "SELECT cls_grp.id "
              "FROM cls_grp "
              "WHERE (airline IS NULL OR airline=:airline) AND "
              "      (airp IS NULL OR airp=:airp) AND "
              "      DECODE(airline,NULL,0,4)+ "
              "      DECODE(airp,NULL,0,2) = :priority AND "
              "      class=:class AND pr_del=0";
            Qry.CreateVariable("airline",otString,fltInfo.airline);
            Qry.CreateVariable("airp",otString,fltInfo.airp);
            Qry.CreateVariable("priority",otInteger,priority);
            Qry.CreateVariable("class",otString,PaxQry.FieldAsString("class"));
            Qry.Execute();
            if (Qry.Eof)
            {
              if (!PaxQry.FieldIsNULL("subclass"))
                throw UserException("MSG.CHECKIN.NOT_MADE_IN_SUBCLASS",
                                    LParams()<<LParam("subclass",ElemIdToCodeNative(etSubcls,PaxQry.FieldAsString("subclass"))));
              else
                throw UserException("MSG.CHECKIN.NOT_MADE_IN_CLASS",
                                    LParams()<<LParam("class",ElemIdToCodeNative(etClass,PaxQry.FieldAsString("class"))));
            };
            if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
            else
              if (class_grp!=Qry.FieldAsInteger("id"))
                throw UserException("MSG.CHECKIN.INPOSSIBLE_SUBCLASS_IN_GROUP");
            Qry.Next();
            if (!Qry.Eof)
              throw EXCEPTIONS::Exception("More than one class group found (airline=%s, airp=%s, class=%s)",
                                          fltInfo.airline.c_str(),fltInfo.airp.c_str(),PaxQry.FieldAsString("class"));
          };
          Qry.Clear();
          Qry.SQLText="UPDATE pax_grp SET class_grp=:class_grp WHERE grp_id=:grp_id";
          Qry.CreateVariable("grp_id",otInteger,grp_id);
          Qry.CreateVariable("class_grp",otInteger,class_grp);
          Qry.Execute();
        };
        //вычисляем и записываем признак waitlist_alarm и brd_alarm
        check_waitlist_alarm( point_dep );
        check_brd_alarm( point_dep );
      };

      //BSM
      if (BSMsend) BSM::Send(point_dep,grp_id,BSMContentBefore,BSMaddrs);

      if (first_segment && reqInfo->client_type==ctTerm)
      {
        //отправить на клиент счетчики
        readTripCounters(point_dep,resNode);
        if (!reqInfo->desk.compatible(PAD_VERSION))
        {
          //pr_etl_only
          readTripSets( fltInfo, pr_etstatus, NewTextChild(resNode,"trip_sets") );
        };
      };
    }
    catch(UserException &e)
    {
      if (reqInfo->client_type==ctTerm)
      {
        if (!only_one)
        {
          LexemaData lexemeData=GetLexemeDataWithFlight(e.getLexemaData(), fltInfo);
          throw UserException(lexemeData.lexema_id, lexemeData.lparams);
        }
        else
          throw;
      }
      else
      {
        //веб, киоски
        try
        {
          dynamic_cast<CheckIn::UserException&>(e);
          throw; //это уже CheckIn::UserException - прокидываем дальше
        }
        catch (bad_cast)
        {
          throw CheckIn::UserException(e.getLexemaData(), point_dep);
        };
      };
    };

  }; //цикл по сегментам

  if (et_processed)
  {
    OraSession.Rollback();  //откат

    int req_ctxt=AstraContext::SetContext("TERM_REQUEST",XMLTreeToText(termReqNode->doc));
    if (!ETStatusInterface::ETChangeStatus(req_ctxt,ETInfo))
      throw EXCEPTIONS::Exception("CheckInInterface::SavePax: Wrong variable 'et_processed'");
    AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
    return false;
  };

  if (reqInfo->client_type==ctTerm)
	{
    //пересчитать данные по группе и отправить на клиент
    LoadPax(first_grp_id,resNode);
  };
  return true;
};

void CheckInInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node;
  int grp_id;
  TQuery Qry(&OraSession);
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    int point_id=NodeAsInteger("point_id",reqNode);
    node = GetNode("pax_id",reqNode);
    if (node==NULL||NodeIsNULL(node))
    {
      int reg_no=NodeAsInteger("reg_no",reqNode);
      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "point_dep=:point_id AND reg_no=:reg_no";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.CreateVariable("reg_no",otInteger,reg_no);
      Qry.Execute();
      if (Qry.Eof) throw UserException(1,"MSG.CHECKIN.REG_NO_NOT_FOUND");
      grp_id=Qry.FieldAsInteger("grp_id");
      Qry.Next();
      if (!Qry.Eof) throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
    }
    else
    {
      int pax_id=NodeAsInteger(node);
      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id,point_dep FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,pax_id);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("MSG.PASSENGER.NOT_CHECKIN");
      if (Qry.FieldAsInteger("point_dep")!=point_id)
        throw UserException("MSG.PASSENGER.OTHER_FLIGHT");
      grp_id=Qry.FieldAsInteger("grp_id");
    };
  }
  else grp_id=NodeAsInteger(node);

  LoadPax(grp_id,resNode);
};

void CheckInInterface::LoadPax(int grp_id, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr node,paxNode;
  TQuery Qry(&OraSession);
  TQuery NormQry(&OraSession);
  vector<int> grp_ids;
  grp_ids.push_back(grp_id);

  TCkinRoute tckin_route;
  tckin_route.GetRouteAfter(grp_id, crtNotCurrent, crtOnlyDependent);
  for(TCkinRoute::const_iterator r=tckin_route.begin(); r!=tckin_route.end(); r++)
    grp_ids.push_back(r->grp_id);

  bool trfer_confirm=true;
  vector<CheckIn::TTransferItem> segs;
  xmlNodePtr segsNode=NewTextChild(resNode,"segments");
  for(vector<int>::const_iterator grp_id=grp_ids.begin();grp_id!=grp_ids.end();grp_id++)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT points.airline,points.flt_no,points.suffix,points.airp,points.scd_out, "
      "       point_dep,airp_dep,point_arv,airp_arv, "
      "       class,status,hall,bag_refuse,trfer_confirm,pax_grp.tid "
      "FROM pax_grp,points "
      "WHERE pax_grp.point_dep=points.point_id AND grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,*grp_id);
    Qry.Execute();
    if (Qry.Eof) return; //это бывает когда разрегистрация всей группы по ошибке агента

    int point_dep=Qry.FieldAsInteger("point_dep");
    
    if (grp_id==grp_ids.begin())
      trfer_confirm=Qry.FieldAsInteger("trfer_confirm")!=0;

    xmlNodePtr segNode=NewTextChild(segsNode,"segment");

    TTripInfo operFlt(Qry);
    
    CheckIn::TTransferItem seg;
    seg.operFlt=operFlt;
    seg.grp_id=*grp_id;
    seg.airp_arv=Qry.FieldAsString("airp_arv");

    xmlNodePtr operFltNode;
    if (reqInfo->desk.compatible(PAD_VERSION))
      operFltNode=NewTextChild(segNode,"tripheader");
    else
      operFltNode=segNode;
    TripsInterface::readOperFltHeader( operFlt, operFltNode );
    readTripData( point_dep, segNode );

    NewTextChild(segNode,"grp_id",*grp_id);
    NewTextChild(segNode,"point_dep",Qry.FieldAsInteger("point_dep"));
    NewTextChild(segNode,"airp_dep",Qry.FieldAsString("airp_dep"));
    //инфа по группе формлен трансфер
    NewTextChild(segNode,"point_arv",Qry.FieldAsInteger("point_arv"));
    NewTextChild(segNode,"airp_arv",Qry.FieldAsString("airp_arv"));
    //город назначения
    NewTextChild(segNode,"city_arv");
    try
    {
      TAirpsRow& airpsRow=(TAirpsRow&)base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv"));
      TCitiesRow& citiesRow=(TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
      ReplaceTextChild( segNode, "city_arv", citiesRow.code );
      if (!reqInfo->desk.compatible(PAD_VERSION))
        ReplaceTextChild( segNode, "city_arv_name", citiesRow.name );
    }
    catch(EBaseTableError) {};
    //класс
    NewTextChild(segNode,"class",Qry.FieldAsString("class"));
    if (!reqInfo->desk.compatible(PAD_VERSION))
    {
      try
      {
        TClassesRow& classesRow=(TClassesRow&)base_tables.get("classes").get_row("code",Qry.FieldAsString("class"));
        ReplaceTextChild( segNode, "class_name", classesRow.name );
      }
      catch(EBaseTableError) {};
    };

    NewTextChild(segNode,"status",Qry.FieldAsString("status"));
    NewTextChild(segNode,"hall",Qry.FieldAsInteger("hall"));
    if (Qry.FieldAsInteger("bag_refuse")!=0)
      NewTextChild(segNode,"bag_refuse",refuseAgentError);
    else
      NewTextChild(segNode,"bag_refuse");
    NewTextChild(segNode,"tid",Qry.FieldAsInteger("tid"));
    
    bool pr_unaccomp=Qry.FieldIsNULL("class");
    if (!pr_unaccomp)
    {
      Qry.Clear();
      Qry.SQLText=
        "SELECT mark_trips.airline,mark_trips.flt_no,mark_trips.suffix, "
        "       mark_trips.scd AS scd_out,mark_trips.airp_dep AS airp,pr_mark_norms "
        "FROM pax_grp,mark_trips "
        "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,*grp_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        TTripInfo markFlt(Qry);
        xmlNodePtr markFltNode=NewTextChild(segNode,"mark_flight");
        NewTextChild(markFltNode,"airline",markFlt.airline);
        NewTextChild(markFltNode,"flt_no",markFlt.flt_no);
        NewTextChild(markFltNode,"suffix",markFlt.suffix);
        NewTextChild(markFltNode,"scd",DateTimeToStr(markFlt.scd_out));
        NewTextChild(markFltNode,"airp_dep",markFlt.airp);
        NewTextChild(markFltNode,"pr_mark_norms",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
      };

      Qry.Clear();
      Qry.SQLText="SELECT pax_id FROM bp_print WHERE pax_id=:pax_id AND pr_print<>0 AND rownum=1";
      Qry.DeclareVariable("pax_id",otInteger);
      TQuery PaxQry(&OraSession);
      PaxQry.Clear();
      PaxQry.SQLText=
        "SELECT pax.pax_id,pax.surname,pax.name,pax.pers_type,"
        "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'one',rownum) AS seat_no, "
        "       pax.seat_type, "
        "       pax.seats,pax.refuse,pax.reg_no, "
        "       pax.ticket_no,pax.coupon_no,pax.ticket_rem,pax.ticket_confirm, "
        "       pax.subclass,pax.tid,pax.bag_pool_num, "
        "       crs_pax.pax_id AS crs_pax_id "
        "FROM pax,crs_pax "
        "WHERE pax.pax_id=crs_pax.pax_id(+) AND crs_pax.pr_del(+)=0 AND "
        "      pax.grp_id=:grp_id "
        "ORDER BY pax.reg_no";
      PaxQry.CreateVariable("grp_id",otInteger,*grp_id);
      
      TQuery PaxDocQry(&OraSession);
      TQuery PaxDocoQry(&OraSession);
      
      PaxQry.Execute();

      if (grp_id==grp_ids.begin())
      {
        TTrferRoute trfer;
        trfer.GetRoute(*grp_id, trtNotFirstSeg);
        BuildTransfer(trfer,resNode);
      };

      node=NewTextChild(segNode,"passengers");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        paxNode=NewTextChild(node,"pax");
        int pax_id=PaxQry.FieldAsInteger("pax_id");
        
        CheckIn::TPaxTransferItem pax;
        pax.pax_id=pax_id;
        pax.subclass=PaxQry.FieldAsString("subclass");
        seg.pax.push_back(pax);
        
        NewTextChild(paxNode,"pax_id",pax_id);
        NewTextChild(paxNode,"surname",PaxQry.FieldAsString("surname"));
        NewTextChild(paxNode,"name",PaxQry.FieldAsString("name"));
        NewTextChild(paxNode,"pers_type",PaxQry.FieldAsString("pers_type"));
        NewTextChild(paxNode,"seat_no",PaxQry.FieldAsString("seat_no"));
        NewTextChild(paxNode,"seat_type",PaxQry.FieldAsString("seat_type"));
        NewTextChild(paxNode,"seats",PaxQry.FieldAsInteger("seats"));
        NewTextChild(paxNode,"refuse",PaxQry.FieldAsString("refuse"));
        NewTextChild(paxNode,"reg_no",PaxQry.FieldAsInteger("reg_no"));
        CheckIn::TPaxTknItem().fromDB(PaxQry).toXML(paxNode);

        if (TReqInfo::Instance()->desk.compatible(DOCS_VERSION))
        {
          CheckIn::TPaxDocItem doc;
          if (CheckIn::LoadPaxDoc(pax_id, doc, PaxDocQry)) doc.toXML(paxNode);
          CheckIn::TPaxDocoItem doco;
          if (CheckIn::LoadPaxDoco(pax_id, doco, PaxDocoQry)) doco.toXML(paxNode);
        }
        else
        {
          CheckIn::TPaxDocItem doc;
          if (CheckIn::LoadPaxDoc(pax_id, doc, PaxDocQry))
            NewTextChild(paxNode, "document", doc.no);
          else
            NewTextChild(paxNode, "document");
        };

        NewTextChild(paxNode,"subclass",PaxQry.FieldAsString("subclass"));
        NewTextChild(paxNode,"tid",PaxQry.FieldAsInteger("tid"));

        NewTextChild(paxNode,"pr_norec",(int)PaxQry.FieldIsNULL("crs_pax_id"));

        Qry.SetVariable("pax_id",pax_id);
        Qry.Execute();
        NewTextChild(paxNode,"pr_bp_print",(int)(!Qry.Eof));

        if (!PaxQry.FieldIsNULL("bag_pool_num"))
          NewTextChild(paxNode,"bag_pool_num",PaxQry.FieldAsInteger("bag_pool_num"));
        else
          NewTextChild(paxNode,"bag_pool_num");

        if (grp_id==grp_ids.begin())
          LoadPaxTransfer(pax_id,paxNode);
        LoadPaxRem(paxNode);
        if (grp_id==grp_ids.begin())
          CheckIn::LoadNorms(paxNode,pr_unaccomp,NormQry);
      };
    }
    else
    {
      //несопровождаемый багаж
      if (grp_id==grp_ids.begin())
      {
        TTrferRoute trfer;
        trfer.GetRoute(*grp_id, trtNotFirstSeg);
        BuildTransfer(trfer,resNode);
        CheckIn::LoadNorms(segNode,pr_unaccomp,NormQry);
      };
    };
    if (grp_id==grp_ids.begin())
    {
      CheckIn::LoadBag(*grp_id,resNode);
      CheckInInterface::LoadPaidBag(*grp_id,resNode);
      readTripCounters(point_dep, segNode);
    };

    readTripSets( point_dep, operFlt, operFltNode );
    
    segs.push_back(seg);
  };
  if (!trfer_confirm &&
      reqInfo->desk.compatible(TRFER_CONFIRM_VERSION))
  {
    //собираем информацию о неподтвержденном трансфере
    LoadUnconfirmedTransfer(segs, resNode);
  };
};

void CheckInInterface::SavePaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id;
  if (GetNodeFast("generated_pax_id",node2)!=NULL)
    pax_id=NodeAsIntegerFast("generated_pax_id",node2);
  else
    pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr remNode=GetNodeFast("rems",node2);
  if (remNode==NULL) return;

  TQuery FQTQry(&OraSession);
  FQTQry.Clear();
  FQTQry.SQLText=
    "INSERT INTO pax_fqt(pax_id,rem_code,airline,no,extra) "
    "VALUES(:pax_id,:rem_code,:airline,:no,:extra)";
  FQTQry.CreateVariable("pax_id",otInteger,pax_id);
  FQTQry.DeclareVariable("rem_code",otString);
  FQTQry.DeclareVariable("airline",otString);
  FQTQry.DeclareVariable("no",otString);
  FQTQry.DeclareVariable("extra",otString);

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText=
    "BEGIN "
    "  DELETE FROM pax_rem WHERE pax_id=:pax_id; "
    "  DELETE FROM pax_fqt WHERE pax_id=:pax_id; "
    "END;";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES (:pax_id,:rem,:rem_code)";
  RemQry.DeclareVariable("rem",otString);
  RemQry.DeclareVariable("rem_code",otString);
  string rem_code, rem_text;
  for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
  {
    TypeB::TFQTItem FQTItem;
    if (CheckFQTRem(remNode,FQTItem))
    {
      FQTQry.SetVariable("rem_code",FQTItem.rem_code);
      FQTQry.SetVariable("airline",FQTItem.airline);
      FQTQry.SetVariable("no",FQTItem.no);
      FQTQry.SetVariable("extra",FQTItem.extra);
      FQTQry.Execute();
    };

    node2=remNode->children;
    //важно что зачитываем после CheckFQTRem, так как функция может менять код и текст ремарки
    rem_code=NodeAsStringFast("rem_code",node2);
    rem_text=NodeAsStringFast("rem_text",node2);
    if (isDisabledRem(rem_code, rem_text))
      throw UserException("MSG.REMARK.INPUT_CODE_DENIAL", LParams()<<LParam("remark",rem_code.empty()?rem_text.substr(0,5):rem_code));
    RemQry.SetVariable("rem",rem_text.c_str());
    RemQry.SetVariable("rem_code",rem_code.c_str());
    RemQry.Execute();
  };
  RemQry.Close();
};

void CheckInInterface::LoadPaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr remsNode=NewTextChild(paxNode,"rems");
  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="SELECT rem_code,rem FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();
  for(;!RemQry.Eof;RemQry.Next())
  {
    const char* rem_code=RemQry.FieldAsString("rem_code");
    const char* rem_text=RemQry.FieldAsString("rem");
    if (isDisabledRem(rem_code, rem_text)) continue;
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_text",rem_text);
    NewTextChild(remNode,"rem_code",rem_code);
  };
  RemQry.Close();
};

void CheckInInterface::SavePaxTransfer(int pax_id, int pax_no, const vector<CheckIn::TTransferItem> &trfer, int seg_no)
{
  vector<CheckIn::TTransferItem>::const_iterator firstTrfer=trfer.begin();
  for(;firstTrfer!=trfer.end()&&seg_no>1;firstTrfer++,seg_no--);

  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText="DELETE FROM transfer_subcls WHERE pax_id=:pax_id";
  TrferQry.CreateVariable("pax_id",otInteger,pax_id);
  TrferQry.Execute();

  TrferQry.SQLText=
    "INSERT INTO transfer_subcls(pax_id,transfer_num,subclass,subclass_fmt) "
    "VALUES (:pax_id,:transfer_num,:subclass,:subclass_fmt)";
  TrferQry.DeclareVariable("transfer_num",otInteger);
  TrferQry.DeclareVariable("subclass",otString);
  TrferQry.DeclareVariable("subclass_fmt",otInteger);

  int trfer_num=1;
  for(vector<CheckIn::TTransferItem>::const_iterator t=firstTrfer;t!=trfer.end();t++,trfer_num++)
  {
    const CheckIn::TPaxTransferItem &pax=t->pax.at(pax_no-1);
    TrferQry.SetVariable("transfer_num",trfer_num);
    TrferQry.SetVariable("subclass",pax.subclass);
    TrferQry.SetVariable("subclass_fmt",(int)pax.subclass_fmt);
    TrferQry.Execute();
  };
  TrferQry.Close();
};

void CheckInInterface::LoadPaxTransfer(int pax_id, xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;

  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText=
    "SELECT subclass,subclass_fmt FROM transfer_subcls "
    "WHERE pax_id=:pax_id ORDER BY transfer_num";
  TrferQry.CreateVariable("pax_id",otInteger,pax_id);
  TrferQry.Execute();
  if (!TrferQry.Eof)
  {
    xmlNodePtr node=NewTextChild(paxNode,"transfer");
    string str;
    for(;!TrferQry.Eof;TrferQry.Next())
    {
      xmlNodePtr trferNode=NewTextChild(node,"segment");
      str=ElemIdToClientElem(etSubcls,TrferQry.FieldAsString("subclass"),
                                (TElemFmt)TrferQry.FieldAsInteger("subclass_fmt"));
      NewTextChild(trferNode,"subclass",str);
    };
  };
  TrferQry.Close();
};

string CheckInInterface::SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const map<int,TSegInfo> &segs, int seg_no)
{
  if (segsNode==NULL) return "";

  xmlNodePtr segNode=GetNode("segments",segsNode);
  if (segNode==NULL) return "";

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  :rows:=ckin.delete_grp_tckin_segs(:grp_id); "
    "END;";
  Qry.CreateVariable("rows",otInteger,0);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();

  segNode=segNode->children;
  if (segNode==NULL)
  {
    if (Qry.GetVariableAsInteger("rows")>0)
      return "Отменена сквозная регистрация пассажиров и багажа";
    else
      return "";
  };

  for(;segNode!=NULL&&seg_no>1;segNode=segNode->next,seg_no--);
  if (segNode==NULL) return "";
  segNode=segNode->next;
  if (segNode==NULL) return "";

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  BEGIN "
    "    SELECT point_id INTO :point_id_trfer FROM trfer_trips "
    "    WHERE scd=:scd AND airline=:airline AND flt_no=:flt_no AND airp_dep=:airp_dep AND "
    "          (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) FOR UPDATE; "
    "  EXCEPTION "
    "    WHEN NO_DATA_FOUND THEN "
    "      SELECT cycle_id__seq.nextval INTO :point_id_trfer FROM dual; "
    "      INSERT INTO trfer_trips(point_id,airline,flt_no,suffix,scd,airp_dep,point_id_spp) "
    "      VALUES (:point_id_trfer,:airline,:flt_no,:suffix,:scd,:airp_dep,NULL); "
    "  END; "
    "  INSERT INTO tckin_segments(grp_id,seg_no,point_id_trfer,airp_arv,pr_final) "
    "  VALUES(:grp_id,:seg_no,:point_id_trfer,:airp_arv,:pr_final); "
    "END;";
  Qry.DeclareVariable("point_id_trfer",otInteger);
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.DeclareVariable("seg_no",otInteger);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("flt_no",otInteger);
  Qry.DeclareVariable("suffix",otString);
  Qry.DeclareVariable("scd",otDate);
  Qry.DeclareVariable("airp_dep",otString);
  Qry.DeclareVariable("airp_arv",otString);
  Qry.DeclareVariable("pr_final",otInteger);

  ostringstream msg;
  msg << "Произведена сквозная регистрация по маршруту:";
  for(seg_no=1;segNode!=NULL;segNode=segNode->next,seg_no++)
  {
    map<int,TSegInfo>::const_iterator s=segs.find(NodeAsInteger("point_dep",segNode));
    if (s==segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SaveTCkinSegs: point_id not found in map segs");

    const TTripInfo &fltInfo=s->second.fltInfo;
    TDateTime local_scd=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));

    Qry.SetVariable("seg_no",seg_no);
    Qry.SetVariable("airline",fltInfo.airline);
    Qry.SetVariable("flt_no",fltInfo.flt_no);
    Qry.SetVariable("suffix",fltInfo.suffix);
    Qry.SetVariable("scd",local_scd);
    Qry.SetVariable("airp_dep",fltInfo.airp);
    Qry.SetVariable("airp_arv",s->second.airp_arv);
    Qry.SetVariable("pr_final",(int)(segNode->next==NULL));
    Qry.Execute();

    msg << " -> "
        << fltInfo.airline
        << setw(3) << setfill('0') << fltInfo.flt_no
        << fltInfo.suffix << "/"
        << DateTimeToStr(local_scd,"dd")
        << ":" << fltInfo.airp << "-" << s->second.airp_arv;
  };

  return msg.str();
};

void CheckInInterface::ParseTransfer(xmlNodePtr trferNode,
                                     xmlNodePtr paxNode,
                                     const TSegInfo &firstSeg,
                                     vector<CheckIn::TTransferItem> &segs)
{
  TDateTime local_scd=UTCToLocal(firstSeg.fltInfo.scd_out,AirpTZRegion(firstSeg.fltInfo.airp));

  ParseTransfer(trferNode, paxNode, firstSeg.airp_arv, local_scd, segs);
};

void CheckInInterface::ParseTransfer(xmlNodePtr trferNode,
                                     xmlNodePtr paxNode,
                                     const string &airp_arv,
                                     const TDateTime scd_out_local,
                                     vector<CheckIn::TTransferItem> &segs)
{
  segs.clear();
  if (trferNode==NULL) return;
  
  int trfer_num=1;
  string strh;
  string prior_airp_arv_id=airp_arv;
  TDateTime local_scd=scd_out_local;
  for(xmlNodePtr node=trferNode->children;node!=NULL;node=node->next,trfer_num++)
  {
    xmlNodePtr node2=node->children;
    
    ostringstream flt;
    flt << NodeAsStringFast("airline",node2)
        << setw(3) << setfill('0') << NodeAsIntegerFast("flt_no",node2)
        << NodeAsStringFast("suffix",node2) << "/"
        << setw(2) << setfill('0') << NodeAsIntegerFast("local_date",node2);

    CheckIn::TTransferItem seg;
        
    //авиакомпания
    strh=NodeAsStringFast("airline",node2);
    seg.operFlt.airline=ElemToElemId(etAirline,strh,seg.operFlt.airline_fmt);
    if (seg.operFlt.airline_fmt==efmtUnknown)
      throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRLINE",
                          LParams()<<LParam("airline",strh)
                                   <<LParam("flight",flt.str()));

    //номер рейса
    seg.operFlt.flt_no=NodeAsIntegerFast("flt_no",node2);
    
    //суффикс
    if (!NodeIsNULLFast("suffix",node2))
    {
      strh=NodeAsStringFast("suffix",node2);
      seg.operFlt.suffix=ElemToElemId(etSuffix,strh,seg.operFlt.suffix_fmt);
      if (seg.operFlt.suffix_fmt==efmtUnknown)
        throw UserException("MSG.TRANSFER_FLIGHT.INVALID_SUFFIX",
                            LParams()<<LParam("suffix",strh)
                                     <<LParam("flight",flt.str()));
    };
    
    
    int local_date=NodeAsIntegerFast("local_date",node2);
    try
    {
      TDateTime base_date=local_scd-1; //патамушта можем из Японии лететь в Америку во вчерашний день
      local_scd=DayToDate(local_date,base_date,false); //локальная дата вылета
      seg.operFlt.scd_out=local_scd;
    }
    catch(EXCEPTIONS::EConvertError &E)
    {
      throw UserException("MSG.TRANSFER_FLIGHT.INVALID_LOCAL_DATE_DEP",
                          LParams()<<LParam("flight",flt.str()));
    };
    
    //аэропорт вылета
    if (GetNodeFast("airp_dep",node2)!=NULL)  //задан а/п вылета
    {
      strh=NodeAsStringFast("airp_dep",node2);
      seg.operFlt.airp=ElemToElemId(etAirp,strh,seg.operFlt.airp_fmt);
      if (seg.operFlt.airp_fmt==efmtUnknown)
        throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRP_DEP",
                            LParams()<<LParam("airp",strh)
                                     <<LParam("flight",flt.str()));
    }
    else
      seg.operFlt.airp=prior_airp_arv_id;

    //аэропорт прилета
    strh=NodeAsStringFast("airp_arv",node2);
    seg.airp_arv=ElemToElemId(etAirp,strh,seg.airp_arv_fmt);
    if (seg.airp_arv_fmt==efmtUnknown)
      throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRP_ARR",
                          LParams()<<LParam("airp",strh)
                                   <<LParam("flight",flt.str()));
                                   
    seg.flight_view=flt.str();
                                   

    prior_airp_arv_id=seg.airp_arv;
    
    segs.push_back(seg);
  };
  
  if (paxNode==NULL) return;
  
  //трансферные подклассы пассажиров
  for(paxNode=paxNode->children;paxNode!=NULL;paxNode=paxNode->next)
  {
    xmlNodePtr paxTrferNode=GetNode("transfer",paxNode);
    if (paxTrferNode==NULL) paxTrferNode=trferNode; //для совместимости со старым форматом 27.01.09 !!!

    paxTrferNode=paxTrferNode->children;
    vector<CheckIn::TTransferItem>::iterator s=segs.begin();
    for(;s!=segs.end() && paxTrferNode!=NULL; s++,paxTrferNode=paxTrferNode->next)
    {
      s->pax.push_back(CheckIn::TPaxTransferItem());
      CheckIn::TPaxTransferItem &pax=s->pax.back();
      
      strh=NodeAsString("subclass",paxTrferNode);
      if (strh.empty())
        throw UserException("MSG.TRANSFER_FLIGHT.SUBCLASS_NOT_SET",
                            LParams()<<LParam("flight",s->flight_view));
      pax.subclass=ElemToElemId(etSubcls,strh,pax.subclass_fmt);
      if (pax.subclass_fmt==efmtUnknown)
        throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_SUBCLASS",
                            LParams()<<LParam("subclass",strh)
                                     <<LParam("flight",s->flight_view));
    };
    if (s!=segs.end() || paxTrferNode!=NULL)
    {
      //плохая ситуация: кол-во трансферных подклассов пассажира не совпадает с кол-вом трансферных сегментов
      throw EXCEPTIONS::Exception("ParseTransfer: Different number of transfer subclasses and transfer segments");
    };
  };
};

string CheckInInterface::SaveTransfer(int grp_id, const vector<CheckIn::TTransferItem> &trfer, bool pr_unaccomp, int seg_no)
{
  vector<CheckIn::TTransferItem>::const_iterator firstTrfer=trfer.begin();
  for(;firstTrfer!=trfer.end()&&seg_no>1;firstTrfer++,seg_no--);
  
  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText=
    "BEGIN "
    "  :rows:=ckin.delete_grp_trfer(:grp_id); "
    "END;";
  TrferQry.CreateVariable("rows",otInteger,0);
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();
  
  if (firstTrfer==trfer.end()) //ничего не записываем в базу
  {
    if (TrferQry.GetVariableAsInteger("rows")>0)
      return "Отменен трансфер пассажиров и багажа";
    else
      return "";
  };

  TrferQry.Clear();
  TrferQry.SQLText=
    "SELECT airline,flt_no,suffix,scd_out,airp,airp_arv, "
    "       points.point_id,point_num,first_point,pr_tranzit "
    "FROM points,pax_grp "
    "WHERE points.point_id=pax_grp.point_dep AND grp_id=:grp_id AND points.pr_del>=0";
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();
  if (TrferQry.Eof) throw EXCEPTIONS::Exception("Passenger group not found (grp_id=%d)",grp_id);

  string airline_in=TrferQry.FieldAsString("airline");
  int flt_no_in=TrferQry.FieldAsInteger("flt_no");

  TTripInfo fltInfo(TrferQry);

  TTypeBSendInfo sendInfo(fltInfo);
  sendInfo.point_id=TrferQry.FieldAsInteger("point_id");
  sendInfo.first_point=TrferQry.FieldIsNULL("first_point")?NoExists:TrferQry.FieldAsInteger("first_point");
  sendInfo.point_num=TrferQry.FieldAsInteger("point_num");
  sendInfo.pr_tranzit=TrferQry.FieldAsInteger("pr_tranzit")!=0;

  TTypeBAddrInfo addrInfo(sendInfo);
  addrInfo.airp_trfer=TrferQry.FieldAsString("airp_arv");
  addrInfo.pr_lat=true;

  bool without_trfer_set=GetTripSets( tsIgnoreTrferSet, fltInfo );
  bool outboard_trfer=GetTripSets( tsOutboardTrfer, fltInfo );

  //проверка формирования трансфера в латинских телеграммах
  enum TCheckType {checkNone,checkFirstSeg,checkAllSeg};

  vector< pair<string,int> > tlgs;
  vector< pair<string,int> >::iterator iTlgs;

  tlgs.push_back( pair<string,int>("BTM",checkAllSeg) );
  tlgs.push_back( pair<string,int>("BSM",checkAllSeg) );
  tlgs.push_back( pair<string,int>("PRL",checkAllSeg) );
  tlgs.push_back( pair<string,int>("PTM",checkFirstSeg) );
  tlgs.push_back( pair<string,int>("PTMN",checkFirstSeg) );
  tlgs.push_back( pair<string,int>("PSM",checkFirstSeg) );

  int checkType=checkNone;

  for(iTlgs=tlgs.begin();iTlgs!=tlgs.end();iTlgs++)
  {
    if (checkType==checkAllSeg) break;
    if (iTlgs->second==checkNone ||
        iTlgs->second==checkFirstSeg && checkType==checkFirstSeg) continue;
    sendInfo.tlg_type=iTlgs->first;
    addrInfo.tlg_type=iTlgs->first;
    if (!TelegramInterface::GetTypeBAddrs(addrInfo).empty()&&
        TelegramInterface::IsTypeBSend(sendInfo)) checkType=iTlgs->second;
  };

  TrferQry.Clear();
  TrferQry.SQLText=
    "BEGIN "
    "  BEGIN "
    "    SELECT point_id INTO :point_id_trfer FROM trfer_trips "
    "    WHERE scd=:scd AND airline=:airline AND flt_no=:flt_no AND airp_dep=:airp_dep AND "
    "          (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) FOR UPDATE; "
    "  EXCEPTION "
    "    WHEN NO_DATA_FOUND THEN "
    "      SELECT cycle_id__seq.nextval INTO :point_id_trfer FROM dual; "
    "      INSERT INTO trfer_trips(point_id,airline,flt_no,suffix,scd,airp_dep,point_id_spp) "
    "      VALUES (:point_id_trfer,:airline,:flt_no,:suffix,:scd,:airp_dep,NULL); "
    "  END; "
    "  INSERT INTO transfer(grp_id,transfer_num,point_id_trfer, "
    "    airline_fmt,suffix_fmt,airp_dep_fmt,airp_arv,airp_arv_fmt,pr_final) "
    "  VALUES(:grp_id,:transfer_num,:point_id_trfer, "
    "    :airline_fmt,:suffix_fmt,:airp_dep_fmt,:airp_arv,:airp_arv_fmt,:pr_final); "
    "END;";
  TrferQry.DeclareVariable("point_id_trfer",otInteger);
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.DeclareVariable("transfer_num",otInteger);
  TrferQry.DeclareVariable("airline",otString);
  TrferQry.DeclareVariable("airline_fmt",otInteger);
  TrferQry.DeclareVariable("flt_no",otInteger);
  TrferQry.DeclareVariable("suffix",otString);
  TrferQry.DeclareVariable("suffix_fmt",otInteger);
  TrferQry.DeclareVariable("scd",otDate);
  TrferQry.DeclareVariable("airp_dep",otString);
  TrferQry.DeclareVariable("airp_dep_fmt",otInteger);
  TrferQry.DeclareVariable("airp_arv",otString);
  TrferQry.DeclareVariable("airp_arv_fmt",otInteger);
  TrferQry.DeclareVariable("pr_final",otInteger);
  
  string strh;
  ostringstream msg;
  msg << "Оформлен багаж трансфером по маршруту:  ";
  int trfer_num=1;
  for(vector<CheckIn::TTransferItem>::const_iterator t=firstTrfer;t!=trfer.end();t++,trfer_num++)
  {
    if (checkType==checkAllSeg ||
        checkType==checkFirstSeg && t==firstTrfer)
    {
      {
        TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",t->operFlt.airline);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirline, t->operFlt.airline, t->operFlt.airline_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRLINE_NOT_FOUND",
                              LParams()<<LParam("airline",strh)
                                       <<LParam("flight",t->flight_view));

        };
      };
      {
        TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",t->operFlt.airp);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirp, t->operFlt.airp, t->operFlt.airp_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRP_DEP_NOT_FOUND",
                              LParams()<<LParam("airp",strh)
                                       <<LParam("flight",t->flight_view));
        };
      };
      {
        TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",t->airp_arv);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirp, t->airp_arv, t->airp_arv_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRP_ARR_NOT_FOUND",
                              LParams()<<LParam("airp",strh)
                                       <<LParam("flight",t->flight_view));
        };
      };
    };
    if (!without_trfer_set)
    {
      //проверим разрешено ли оформление трансфера
      if (!CheckTrferPermit(airline_in,
                            flt_no_in,
                            t->operFlt.airp,
                            t->operFlt.airline,
                            t->operFlt.flt_no,
                            outboard_trfer))
        throw UserException("MSG.TRANSFER_FLIGHT.NOT_MADE_TRANSFER",
                            LParams()<<LParam("flight",t->flight_view));
    };

    TrferQry.SetVariable("transfer_num",trfer_num);
    TrferQry.SetVariable("airline",t->operFlt.airline);
    TrferQry.SetVariable("airline_fmt",(int)t->operFlt.airline_fmt);
    TrferQry.SetVariable("flt_no",t->operFlt.flt_no);
    if (!t->operFlt.suffix.empty())
    {
      TrferQry.SetVariable("suffix",t->operFlt.suffix);
      TrferQry.SetVariable("suffix_fmt",(int)t->operFlt.suffix_fmt);
    }
    else
    {
      TrferQry.SetVariable("suffix",FNull);
      TrferQry.SetVariable("suffix_fmt",FNull);
    };
    TrferQry.SetVariable("scd",t->operFlt.scd_out);
    TrferQry.SetVariable("airp_dep",t->operFlt.airp);
    TrferQry.SetVariable("airp_dep_fmt",(int)t->operFlt.airp_fmt);
    TrferQry.SetVariable("airp_arv",t->airp_arv);
    TrferQry.SetVariable("airp_arv_fmt",(int)t->airp_arv_fmt);
    TrferQry.SetVariable("pr_final",(int)(t+1==trfer.end()));
    TrferQry.Execute();

    msg << " -> "
        << t->operFlt.airline
        << setw(3) << setfill('0') << t->operFlt.flt_no
        << t->operFlt.suffix << "/"
        << DateTimeToStr(t->operFlt.scd_out,"dd")
        << ":" << t->operFlt.airp << "-" << t->airp_arv;

    airline_in=t->operFlt.airline;
    flt_no_in=t->operFlt.flt_no;
  };

  return msg.str();
};

void CheckInInterface::LoadTransfer(int grp_id, vector<CheckIn::TTransferItem> &trfer)
{
  trfer.clear();
  TTrferRoute route;
  route.GetRoute(grp_id, trtNotFirstSeg);
  for(TTrferRoute::const_iterator r=route.begin(); r!=route.end(); ++r)
  {
    trfer.push_back(CheckIn::TTransferItem());
    CheckIn::TTransferItem &t=trfer.back();
    t.operFlt=r->operFlt;
    t.airp_arv=r->airp_arv;
    t.airp_arv_fmt=r->airp_arv_fmt;
  };
};

void CheckInInterface::BuildTransfer(const TTrferRoute &trfer, xmlNodePtr transferNode)
{
  if (transferNode==NULL) return;

  xmlNodePtr node=NewTextChild(transferNode,"transfer");

  int iDay,iMonth,iYear;
  for(TTrferRoute::const_iterator t=trfer.begin();t!=trfer.end();t++)
  {
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"airline",
                 ElemIdToClientElem(etAirline, t->operFlt.airline, t->operFlt.airline_fmt));
                 
    NewTextChild(trferNode,"flt_no",t->operFlt.flt_no);

    if (!t->operFlt.suffix.empty())
      NewTextChild(trferNode,"suffix",
                   ElemIdToClientElem(etSuffix, t->operFlt.suffix, t->operFlt.suffix_fmt));
    else
      NewTextChild(trferNode,"suffix");

    //дата
    DecodeDate(t->operFlt.scd_out,iYear,iMonth,iDay);
    NewTextChild(trferNode,"local_date",iDay);

    NewTextChild(trferNode,"airp_dep",
                 ElemIdToClientElem(etAirp, t->operFlt.airp, t->operFlt.airp_fmt));

    NewTextChild(trferNode,"airp_arv",
                 ElemIdToClientElem(etAirp, t->airp_arv, t->airp_arv_fmt));

    NewTextChild(trferNode,"city_dep",
                 base_tables.get("airps").get_row("code",t->operFlt.airp).AsString("city"));
    NewTextChild(trferNode,"city_arv",
                 base_tables.get("airps").get_row("code",t->airp_arv).AsString("city"));
  };
};

void CheckInInterface::SavePaidBag(int grp_id, xmlNodePtr paidbagNode)
{
  if (paidbagNode==NULL) return;
  xmlNodePtr node,node2;
  xmlNodePtr paidBagNode=GetNode("paid_bags",paidbagNode);
  if (paidBagNode!=NULL)
  {
    TQuery BagQry(&OraSession);
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id,rate_trfer) "
      "VALUES(:grp_id,:bag_type,:weight,:rate_id,:rate_trfer)";
    BagQry.DeclareVariable("bag_type",otInteger);
    BagQry.DeclareVariable("weight",otInteger);
    BagQry.DeclareVariable("rate_id",otInteger);
    BagQry.DeclareVariable("rate_trfer",otInteger);
    for(node=paidBagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (!NodeIsNULLFast("bag_type",node2))
        BagQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
      else
        BagQry.SetVariable("bag_type",FNull);
      BagQry.SetVariable("weight",NodeAsIntegerFast("weight",node2));
      if (!NodeIsNULLFast("rate_id",node2))
      {
        BagQry.SetVariable("rate_id",NodeAsIntegerFast("rate_id",node2));
        BagQry.SetVariable("rate_trfer",(int)(NodeAsIntegerFast("rate_trfer",node2,0)!=0));
      }
      else
      {
        BagQry.SetVariable("rate_id",FNull);
        BagQry.SetVariable("rate_trfer",FNull);
      };
      BagQry.Execute();
    };
    BagQry.Close();
  };
};

void CheckInInterface::LoadPaidBag(int grp_id, xmlNodePtr paidbagNode)
{
  if (paidbagNode==NULL) return;

  xmlNodePtr node=NewTextChild(paidbagNode,"paid_bags");
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT NVL(paid_bag.bag_type,-1) AS bag_type,paid_bag.weight, "
    "       NVL(rate_id,-1) AS rate_id,rate,rate_cur,rate_trfer "
    "FROM paid_bag,bag_rates "
    "WHERE paid_bag.rate_id=bag_rates.id(+) AND grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    if (!BagQry.FieldIsNULL("bag_type"))
      NewTextChild(paidBagNode,"bag_type",BagQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(paidBagNode,"bag_type");
    NewTextChild(paidBagNode,"weight",BagQry.FieldAsInteger("weight"));
    if (!BagQry.FieldIsNULL("rate_id"))
    {
      NewTextChild(paidBagNode,"rate_id",BagQry.FieldAsInteger("rate_id"));
      NewTextChild(paidBagNode,"rate",BagQry.FieldAsFloat("rate"));
      NewTextChild(paidBagNode,"rate_cur",BagQry.FieldAsString("rate_cur"));
      NewTextChild(paidBagNode,"rate_trfer",(int)(BagQry.FieldAsInteger("rate_trfer")!=0));
    }
    else
    {
      NewTextChild(paidBagNode,"rate_id");
      NewTextChild(paidBagNode,"rate");
      NewTextChild(paidBagNode,"rate_cur");
      NewTextChild(paidBagNode,"rate_trfer");
    };
  };
  BagQry.Close();
};

void CheckInInterface::SaveTagPacks(xmlNodePtr node)
{
  if (node==NULL) return;
  node=GetNode("tag_packs",node);
  if (node==NULL) return;
  TReqInfo* reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  IF :no IS NULL THEN "
    "    DELETE FROM tag_packs "
    "    WHERE desk=:desk AND airline=:airline AND target=:target; "
    "  ELSE "
    "    UPDATE tag_packs "
    "    SET tag_type=:tag_type, color=:color, no=:no "
    "    WHERE desk=:desk AND airline=:airline AND target=:target; "
    "    IF SQL%NOTFOUND THEN "
    "      INSERT INTO tag_packs(desk,airline,target,tag_type,color,no) "
    "      VALUES (:desk,:airline,:target,:tag_type,:color,:no); "
    "    END IF; "
    "  END IF;   "
    "END; ";
  Qry.CreateVariable("desk",otString,reqInfo->desk.code);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("target",otString);
  Qry.DeclareVariable("tag_type",otString);
  Qry.DeclareVariable("color",otString);
  Qry.DeclareVariable("no",otFloat);
  for(node=node->children;node!=NULL;node=node->next)
  {
    xmlNodePtr node2=node->children;
    Qry.SetVariable("airline",NodeAsStringFast("airline",node2));
    Qry.SetVariable("target",NodeAsStringFast("target",node2));
    Qry.SetVariable("tag_type",NodeAsStringFast("tag_type",node2));
    Qry.SetVariable("color",NodeAsStringFast("color",node2));
    if (!NodeIsNULLFast("no",node2))
      Qry.SetVariable("no",NodeAsFloatFast("no",node2));
    else
      Qry.SetVariable("no",FNull);
    Qry.Execute();
  };
  Qry.Close();
};

void CheckInInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  xmlNodePtr itemNode;

  TQuery Qry( &OraSession );
  Qry.SQLText =
     "SELECT point_arv, class, "
     "       crs_ok-ok AS noshow, "
     "       crs_tranzit-tranzit AS trnoshow, "
     "       tranzit+ok+goshow AS show, "
     "       free_ok, "
     "       free_goshow, "
     "       nooccupy "
     "FROM counters2 "
     "WHERE point_dep=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();

  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "item" );
    NewTextChild( itemNode, "point_arv", Qry.FieldAsInteger( "point_arv" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "noshow", Qry.FieldAsInteger( "noshow" ) );
    NewTextChild( itemNode, "trnoshow", Qry.FieldAsInteger( "trnoshow" ) );
    NewTextChild( itemNode, "show", Qry.FieldAsInteger( "show" ) );
    NewTextChild( itemNode, "free_ok", Qry.FieldAsInteger( "free_ok" ) );
    NewTextChild( itemNode, "free_goshow", Qry.FieldAsInteger( "free_goshow" ) );
    NewTextChild( itemNode, "nooccupy", Qry.FieldAsInteger( "nooccupy" ) );
  };
}

void CheckInInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode,node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out, "
    "       point_num, first_point, pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TTripInfo operFlt(Qry);
  TTripRoute route;

  route.GetRouteAfter(NoExists,
                      point_id,
                      Qry.FieldAsInteger("point_num"),
                      Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                      Qry.FieldAsInteger("pr_tranzit")!=0,
                      trtNotCurrent,trtNotCancelled);
                      
  TCheckDocTknInfo checkTknInfo=GetCheckTknInfo(point_id);

  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(TTripRoute::iterator r=route.begin();r!=route.end();r++)
  {
    //проверим на дублирование кодов аэропортов в рамках одного рейса
    for(i=airps.begin();i!=airps.end();i++)
      if (*i==r->airp) break;
    if (i!=airps.end()) continue;

    try
    {
      TAirpsRow& airpsRow=(TAirpsRow&)base_tables.get("airps").get_row("code",r->airp);

      itemNode = NewTextChild( node, "airp" );
      NewTextChild( itemNode, "point_id", r->point_id );
      NewTextChild( itemNode, "airp_code", airpsRow.code );
      NewTextChild( itemNode, "city_code", airpsRow.city );
      if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
      {
        ostringstream target_view;
        target_view << ElemIdToNameLong(etCity, airpsRow.city)
                    << " (" << ElemIdToCodeNative(etAirp, airpsRow.code) << ")";
        NewTextChild( itemNode, "target_view", target_view.str() );
      }
      else
      {
        NewTextChild( itemNode, "airp_name", ElemIdToNameLong(etAirp, airpsRow.code) );
        NewTextChild( itemNode, "city_name", ElemIdToNameLong(etCity, airpsRow.city) );
      };
      TCheckDocInfo checkDocInfo=GetCheckDocInfo(point_id, airpsRow.code);
      
      checkDocInfo.first.ToXML(NewTextChild( itemNode, "check_doc_info" ));
      checkDocInfo.second.ToXML(NewTextChild( itemNode, "check_doco_info" ));
      checkTknInfo.ToXML(NewTextChild( itemNode, "check_tkn_info" ));
      airps.push_back(airpsRow.code);
    }
    catch(EBaseTableError) {};
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT class AS class_code, "
    "       cfg "
    "FROM trip_classes,classes "
    "WHERE classes.code=trip_classes.class AND point_id= :point_id "
    "ORDER BY priority";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "classes" );
  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "class" );
    const char* cl=Qry.FieldAsString( "class_code" );
    NewTextChild( itemNode, "code", cl );
    if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
      NewTextChild( itemNode, "class_view", ElemIdToNameLong(etClass, cl) );
    else
      NewTextChild( itemNode, "name", ElemIdToNameLong(etClass, cl) );
    NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
  };

  //выходы на посадку, назначенные на рейс
  node = NewTextChild( tripdataNode, "gates" );
  vector<string> gates;
  TripsInterface::readGates(point_id, gates);
  for(vector<string>::iterator iv = gates.begin(); iv != gates.end(); iv++)
    NewTextChild( node, "gate_name", *iv);

  //залы
  TripsInterface::readHalls(operFlt.airp, "Р", tripdataNode);

  //выходы на посадку для каждого из залов регистрации, назначенные на рейс
  Qry.Clear();
  Qry.SQLText =
    "SELECT name AS gate_name "
    "FROM stations,trip_stations,station_halls "
    "WHERE stations.desk=trip_stations.desk AND "
    "      stations.work_mode=trip_stations.work_mode AND "
    "      station_halls.airp=stations.airp AND "
    "      station_halls.station=stations.name AND "
    "      trip_stations.point_id=:point_id AND "
    "      trip_stations.work_mode=:work_mode AND "
    "      station_halls.hall=:hall ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("work_mode",otString,"П");
  Qry.DeclareVariable("hall",otInteger);

  xmlNodePtr hallNode = NodeAsNode("halls",tripdataNode)->children;
  for(;hallNode!=NULL;hallNode=hallNode->next)
  {
    Qry.SetVariable("hall",NodeAsInteger("id",hallNode));
    Qry.Execute();
    if (Qry.Eof) continue;
    node = NewTextChild( hallNode, "gates" );
    for(;!Qry.Eof;Qry.Next())
    {
      NewTextChild( node, "gate_name", Qry.FieldAsString( "gate_name" ) );
    };
  };

  vector<TTripInfo> markFlts;
  GetMktFlights(operFlt,markFlts);
  if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
  {
    //вставим в начало массива коммерческих рейсов фактический рейс
    TTripInfo markFlt;
    markFlt=operFlt;
    markFlt.scd_out=UTCToLocal(markFlt.scd_out, AirpTZRegion(markFlt.airp));
    markFlts.insert(markFlts.begin(),markFlt);
  };

  if (!markFlts.empty())
  {
    TCodeShareSets codeshareSets;
    node = NewTextChild( tripdataNode, "mark_flights" );
    for(vector<TTripInfo>::iterator f=markFlts.begin();f!=markFlts.end();f++)
    {
      xmlNodePtr markFltNode=NewTextChild(node,"flight");

      NewTextChild(markFltNode,"airline",f->airline);
      if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
        try
        {
          TAirlinesRow& airlinesRow=(TAirlinesRow&)base_tables.get("airlines").get_row("code",f->airline);
          NewTextChild(markFltNode,"airline_lat",airlinesRow.code_lat);
        }
        catch(EBaseTableError) {};
      NewTextChild(markFltNode,"flt_no",f->flt_no);
      NewTextChild(markFltNode,"suffix",f->suffix);
      NewTextChild(markFltNode,"scd",DateTimeToStr(f->scd_out));
      NewTextChild(markFltNode,"airp_dep",f->airp);

      if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION) && f==markFlts.begin())
        NewTextChild(markFltNode,"pr_mark_norms",(int)false);
      else
      {
        codeshareSets.get(operFlt,*f);
        NewTextChild(markFltNode,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);
      };
    };
  };
}

void CheckInInterface::readTripSets( int point_id,
                                     xmlNodePtr dataNode)
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo fltInfo(Qry);
  readTripSets(point_id, fltInfo, NewTextChild(dataNode,"tripsets"));
};

void CheckInInterface::readTripSets( int point_id,
                                     const TTripInfo &fltInfo,
                                     xmlNodePtr tripSetsNode )
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  readTripSets(fltInfo, Qry.FieldAsInteger("pr_etstatus"), tripSetsNode);
};

void CheckInInterface::readTripSets( const TTripInfo &fltInfo,
                                     int pr_etstatus,
                                     xmlNodePtr tripSetsNode)
{
  NewTextChild( tripSetsNode, "pr_etl_only", (int)GetTripSets(tsETLOnly,fltInfo) );
  NewTextChild( tripSetsNode, "pr_etstatus", pr_etstatus );
  NewTextChild( tripSetsNode, "pr_no_ticket_check", (int)GetTripSets(tsNoTicketCheck,fltInfo) );
};


void CheckInInterface::GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readTripCounters(point_id,resNode);
};


//////////// для системы информирования

void CheckInInterface::OpenCheckInInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int open = NodeAsInteger("Open",reqNode);
	int point_id = NodeAsInteger( "point_id", reqNode );
	ProgTrace( TRACE5, "Open=%d", open );
  TQuery Qry(&OraSession);
  Qry.SQLText =
   "UPDATE trip_stations SET start_time=DECODE(:open, 1, system.UTCSYSDATE, NULL) "
   " WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "open", otInteger, open );
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "work_mode", otString, "Р" );
  Qry.Execute();
  if ( open == 1 )
    TReqInfo::Instance()->MsgToLog( "Открытие регистрации для системы информирования", evtFlt, point_id );
  else
  	TReqInfo::Instance()->MsgToLog( "Закрытие регистрации для системы информирования", evtFlt, point_id );

};

struct TCkinPaxInfo
{
  string surname,name,pers_type,subclass;
  int seats,reqCount,resCount;
  xmlNodePtr node;
};

void CheckInInterface::GetTCkinFlights(const vector<CheckIn::TTransferItem> &trfer,
                                       vector< TCkinSegFlts > &segs)
{
  segs.clear();
  if (trfer.empty()) return;

  TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
      "SELECT point_id,scd_out AS scd,airp "
      "FROM points "
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND "
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
      "      scd_out >= TO_DATE(:scd)-1 AND scd_out < TO_DATE(:scd)+2 AND "
      "      pr_del>=0 AND pr_reg<>0";
  PointsQry.DeclareVariable("airline",otString);
  PointsQry.DeclareVariable("flt_no",otInteger);
  PointsQry.DeclareVariable("airp_dep",otString);
  PointsQry.DeclareVariable("suffix",otString);
  PointsQry.DeclareVariable("scd",otDate);

  bool is_edi=false;
  for(vector<CheckIn::TTransferItem>::const_iterator f=trfer.begin();f!=trfer.end();f++)
  {
    TCkinSegFlts seg;

    //проверим обслуживается ли рейс в другой DCS
    if (!is_edi)
    {
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
        "SELECT edi_addr,edi_own_addr, "
        "       DECODE(airline,NULL,0,2)+ "
        "       DECODE(flt_no,NULL,0,1) AS priority "
        "FROM dcs_addr_set "
        "WHERE airline=:airline AND "
        "      (flt_no IS NULL OR flt_no=:flt_no) "
        "ORDER BY priority DESC";
      Qry.CreateVariable("airline",otString,f->operFlt.airline);
      Qry.CreateVariable("flt_no",otInteger,(const int)(f->operFlt.flt_no));
      Qry.Execute();
      if (!Qry.Eof) is_edi=true;
    };

    seg.is_edi=is_edi;

    if (!is_edi)
    {
      //ищем рейс в СПП
      PointsQry.SetVariable("airline",f->operFlt.airline);
      PointsQry.SetVariable("flt_no",(const int)(f->operFlt.flt_no));
      PointsQry.SetVariable("airp_dep",f->operFlt.airp);
      PointsQry.SetVariable("suffix",f->operFlt.suffix);
      PointsQry.SetVariable("scd",f->operFlt.scd_out);
      PointsQry.Execute();
      TDateTime scd;
      string tz_region;

      for(;!PointsQry.Eof;PointsQry.Next())
      {
        //цикл по рейсам в СПП
        scd=PointsQry.FieldAsDateTime("scd");
        tz_region=AirpTZRegion(PointsQry.FieldAsString("airp"),false);
        if (tz_region.empty()) continue;
        scd=UTCToLocal(scd,tz_region);
        modf(scd,&scd);
        if (scd!=f->operFlt.scd_out) continue;

        TSegInfo segSPPInfo;
        //int point_arv=ASTRA::NoExists;
        CheckInInterface::CheckCkinFlight(PointsQry.FieldAsInteger("point_id"),
                        PointsQry.FieldAsString("airp"),
                        ASTRA::NoExists/*point_arv*/,
                        f->airp_arv,
                        false,
                        segSPPInfo);

        if (segSPPInfo.fltInfo.pr_del==ASTRA::NoExists) continue; //не нашли по point_dep
        seg.flts.push_back(segSPPInfo);
      };
    };
    segs.push_back(seg);
  };
};

class TCkinSegmentItem : public TCkinSegFlts
{
  public:
    bool conf_status;
    string calc_status;
};

void CheckInInterface::CheckTCkinRoute(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo* reqInfo = TReqInfo::Instance();

  TSegInfo firstSeg;
  if (!CheckCkinFlight(NodeAsInteger("point_dep",reqNode),
                       NodeAsString("airp_dep",reqNode),
                       NodeAsInteger("point_arv",reqNode),
                       NodeAsString("airp_arv",reqNode),
                       false, firstSeg))
  {
    if (firstSeg.fltInfo.pr_del==0)
      throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  };
  if (firstSeg.fltInfo.pr_del==ASTRA::NoExists ||
      firstSeg.fltInfo.pr_del!=0)
    throw UserException("MSG.FLIGHT.CANCELED.REFRESH_DATA");

  bool without_trfer_set=GetTripSets( tsIgnoreTrferSet, firstSeg.fltInfo );
  bool outboard_trfer=GetTripSets( tsOutboardTrfer, firstSeg.fltInfo );

  string airline_in=firstSeg.fltInfo.airline;
  int flt_no_in=firstSeg.fltInfo.flt_no;

  TQuery CrsQry(&OraSession);
  CrsQry.Clear();

  ostringstream sql;

  sql <<
      "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv,crs_pnr.subclass, "
      "       crs_pnr.class, crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, "
      "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket "
      "FROM tlg_binding,crs_pnr,crs_pax,pax "
      "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      tlg_binding.point_id_spp=:point_id AND "
      "      crs_pnr.system='CRS' AND "
      "      crs_pnr.airp_arv=:airp_arv AND "
      "      crs_pnr.subclass=:subclass AND ";
  //if (!reqInfo->desk.compatible(PAD_VERSION)) в дальнейшем надо лучше обрабатывать PAD на сквозных сегментах
    sql << " (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) AND ";

  sql <<
      "      crs_pax.pers_type=:pers_type AND "
      "      DECODE(crs_pax.seats,0,0,1)=:seats AND "
      "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
      "      system.transliter_equal(crs_pax.name,:name)<>0 AND "
      "      crs_pax.pr_del=0 AND "
      "      pax.pax_id IS NULL "
      "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";

  CrsQry.SQLText=sql.str().c_str();
  CrsQry.DeclareVariable("point_id",otInteger);
  CrsQry.DeclareVariable("airp_arv",otString);
  CrsQry.DeclareVariable("subclass",otString);
  CrsQry.DeclareVariable("pers_type",otString);
  CrsQry.DeclareVariable("seats",otInteger);
  CrsQry.DeclareVariable("surname",otString);
  CrsQry.DeclareVariable("name",otString);

  xmlNodePtr trferNode=NodeAsNode("transfer",reqNode);
  vector<CheckIn::TTransferItem> trfer;
  ParseTransfer(trferNode,
                NodeAsNode("passengers",reqNode),
                firstSeg, trfer);

  if (!reqInfo->desk.compatible(PAD_VERSION))
    NewTextChild(resNode,"flight",GetTripName(firstSeg.fltInfo,ecCkin,true,false));
  xmlNodePtr routeNode=NewTextChild(resNode,"tckin_route");
  xmlNodePtr segsNode=NewTextChild(resNode,"tckin_segments");

  vector<TCkinSegmentItem> segs;
  for(trferNode=trferNode->children;trferNode!=NULL;trferNode=trferNode->next)
  {
    xmlNodePtr node2=trferNode->children;
    
    TCkinSegmentItem segItem;
    segItem.conf_status=(NodeAsIntegerFast("conf_status",node2,0)!=0);
    segItem.calc_status=NodeAsStringFast("calc_status",node2,"");
    segs.push_back(segItem);
  };

  vector<TCkinSegFlts> segs2;
  GetTCkinFlights(trfer, segs2);

  if (segs.size()!=segs2.size() ||
      segs.size()!=trfer.size())
    throw EXCEPTIONS::Exception("CheckInInterface::CheckTCkinRoute: different array sizes "
                                "(segs.size()=%d, segs2.size()=%d, trfer.size()=%d",
                                segs.size(),segs2.size(),trfer.size());

  vector<TCkinSegmentItem>::iterator s=segs.begin();
  vector<TCkinSegFlts>::iterator s2=segs2.begin();
  for(;s!=segs.end() && s2!=segs2.end();s++,s2++)
  {
    s->flts.assign(s2->flts.begin(),s2->flts.end());
    s->is_edi=s2->is_edi;
  };

  bool irrelevant_data=false; //устаревшие данные
  bool total_permit=true;
  bool tckin_route_confirm=true;

  //цикл по стыковочным сегментам и по трансферным рейсам
  vector<CheckIn::TTransferItem>::const_iterator f=trfer.begin();
  for(s=segs.begin();s!=segs.end() && f!=trfer.end();s++,f++)
  {
    if (!s->conf_status) tckin_route_confirm=false;


    //НАЧИНАЕМ СБОР ИНФОРМАЦИИ ПО СТЫКОВОЧНОМУ СЕГМЕНТУ
    xmlNodePtr segNode=NewTextChild(routeNode,"route_segment");
    xmlNodePtr seg2Node=NULL;
    if (tckin_route_confirm)
      seg2Node=NewTextChild(segsNode,"tckin_segment");

    //возможность оформления трансферного багажа
    if (!without_trfer_set &&
        !CheckTrferPermit(airline_in,
                          flt_no_in,
                          f->operFlt.airp,
                          f->operFlt.airline,
                          f->operFlt.flt_no,
                          outboard_trfer))
    {
      SetProp(NewTextChild(segNode,"trfer_permit",AstraLocale::getLocaleText("Нет")),"value",(int)false);
      if (tckin_route_confirm)
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",(int)false);
    }
    else
    {
      SetProp(NewTextChild(segNode,"trfer_permit","+"),"value",(int)true);
      if (tckin_route_confirm)
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",(int)true);
    };

    //возможность сквозной регистрации
    TCkinSetsInfo tckinSets;
    CheckTCkinPermit(airline_in,
                     flt_no_in,
                     f->operFlt.airp,
                     f->operFlt.airline,
                     f->operFlt.flt_no,
                     tckinSets);
    if (!tckinSets.pr_permit)
      NewTextChild(segNode,"tckin_permit",AstraLocale::getLocaleText("Нет"));
    else
      NewTextChild(segNode,"tckin_permit","+");
    if (!s->is_edi)
    {
      if (!tckinSets.pr_waitlist)
        NewTextChild(segNode,"tckin_waitlist",AstraLocale::getLocaleText("Нет"));
      else
        NewTextChild(segNode,"tckin_waitlist","+");
      if (!tckinSets.pr_norec)
        NewTextChild(segNode,"tckin_norec",AstraLocale::getLocaleText("Нет"));
      else
        NewTextChild(segNode,"tckin_norec","+");
    }
    else
    {
      NewTextChild(segNode,"tckin_waitlist");
      NewTextChild(segNode,"tckin_norec");
    };

    airline_in=f->operFlt.airline;
    flt_no_in=f->operFlt.flt_no;

    //вывод рейса
    ostringstream flight;
    flight << ElemIdToCodeNative(etAirline, f->operFlt.airline)
           << setw(3) << setfill('0') << f->operFlt.flt_no
           << ElemIdToCodeNative(etSuffix, f->operFlt.suffix) << "/"
           << DateTimeToStr(f->operFlt.scd_out,"dd") << " "
           << ElemIdToCodeNative(etAirp, f->operFlt.airp) << "-"
           << ElemIdToCodeNative(etAirp, f->airp_arv);
           
    if (!s->flts.empty())
    {
      if (s->flts.size()>1)
      {
        //нашли в СПП несколько рейсов, соответствующих трансферному сегменту
        SetProp(ReplaceTextChild(segNode,"classes",AstraLocale::getLocaleText("Дубль в СПП")),"error","CRITICAL");
        SetProp(ReplaceTextChild(segNode,"pnl"),"error");
        SetProp(ReplaceTextChild(segNode,"class"),"error");
        SetProp(ReplaceTextChild(segNode,"free"),"error");
        break;
      }
      else
      {
        //нашли в СПП один рейс, соответствующий трансферному сегменту

        TQuery Qry(&OraSession);
        const TSegInfo &currSeg=*(s->flts.begin());

        NewTextChild(segNode,"flight",flight.str());
        if (tckin_route_confirm)
        {
          xmlNodePtr operFltNode;
          if (reqInfo->desk.compatible(PAD_VERSION))
            operFltNode=NewTextChild(seg2Node,"tripheader");
          else
            operFltNode=seg2Node;
          TripsInterface::readOperFltHeader( currSeg.fltInfo, operFltNode );
          readTripData( currSeg.point_dep, seg2Node );

          NewTextChild( seg2Node, "point_dep", currSeg.point_dep);
          NewTextChild( seg2Node, "airp_dep", currSeg.airp_dep);
          NewTextChild( seg2Node, "point_arv", currSeg.point_arv);
          NewTextChild( seg2Node, "airp_arv", currSeg.airp_arv);
          try
          {
            TAirpsRow& airpsRow=(TAirpsRow&)base_tables.get("airps").get_row("code",currSeg.airp_arv);
            TCitiesRow& citiesRow=(TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
            NewTextChild( seg2Node, "city_arv_code", citiesRow.code );
            if (!reqInfo->desk.compatible(PAD_VERSION))
              NewTextChild( seg2Node, "city_arv_name", citiesRow.name );
          }
          catch(EBaseTableError) {};
        };

        //начитаем компоновку
        string cfg=GetCfgStr(NoExists, currSeg.point_dep);
        if (cfg.empty())
        {
          //компоновка не назначена
          xmlNodePtr wlNode=NewTextChild(segNode,"classes",AstraLocale::getLocaleText("Нет"));
          SetProp(wlNode,"error","WL");
          SetProp(wlNode,"wl_type","C");
        }
        else
        {
          NewTextChild(segNode,"classes",cfg);
        };


        //класс пассажиров
        CrsQry.SetVariable("point_id",currSeg.point_dep);
        CrsQry.SetVariable("airp_arv",currSeg.airp_arv);

        xmlNodePtr paxNode=NodeAsNode("passengers",reqNode)->children;
        xmlNodePtr pax2Node=NewTextChild(segNode,"temp_passengers");
        int paxCount=0,paxCountInPNL=0,seatsSum=0;
        string cl;
        bool doublePax=false;
        vector<TCkinPaxInfo> pax;
        vector<TCkinPaxInfo>::iterator iPax;
        for(int pax_no=1;paxNode!=NULL;paxNode=paxNode->next,pax_no++)
        {
          TCkinPaxInfo paxInfo;
          paxInfo.surname=NodeAsString("surname",paxNode);
          paxInfo.name=NodeAsString("name",paxNode);
          paxInfo.pers_type=NodeAsString("pers_type",paxNode);
          paxInfo.seats=NodeAsInteger("seats",paxNode)==0?0:1;
          paxInfo.subclass.clear();
          //кол-во запрошенных пассажиров с одинаковыми фамилией, именем, типом,
          //наличием/отсутствием мест, подклассом
          paxInfo.reqCount=1;
          paxInfo.resCount=0;
          paxInfo.node=NULL;

          //ищем соответствующий сегменту подкласс пассажира
          paxInfo.subclass=f->pax.at(pax_no-1).subclass;

          for(iPax=pax.begin();iPax!=pax.end();iPax++)
          {
            if (transliter_equal(paxInfo.surname,iPax->surname) &&
                transliter_equal(paxInfo.name,iPax->name) &&
                paxInfo.pers_type==iPax->pers_type &&
                paxInfo.seats==iPax->seats &&
                paxInfo.subclass==iPax->subclass)
            {
              iPax->reqCount++;
              break;
            };
          };
          if (iPax==pax.end()) pax.push_back(paxInfo);
        };
        //массив pax на данный момент содержит пассажиров с уникальными:
        //1. фамилией,
        //2. именем
        //3. типом пассажира
        //4. наличием/отсутствием мест
        //5. подклассом
        //pax.reqCount содержит кол-во повторяющихся пассажиров


        for(iPax=pax.begin();iPax!=pax.end();iPax++)
        {
          paxCount+=iPax->reqCount;
          seatsSum+=iPax->seats * iPax->reqCount;

          CrsQry.SetVariable("subclass", iPax->subclass);
          CrsQry.SetVariable("pers_type",iPax->pers_type);
          CrsQry.SetVariable("seats",    iPax->seats);
          CrsQry.SetVariable("surname",  iPax->surname);
          CrsQry.SetVariable("name",     iPax->name);
          CrsQry.Execute();
          ProgTrace(TRACE5,"<<<<< subclass=%s pers_type=%s surname=%s name=%s seats=%d",
                           iPax->subclass.c_str(),
                           iPax->pers_type.c_str(),
                           iPax->surname.c_str(),
                           iPax->name.c_str(),
                           iPax->seats);

          iPax->node=NewTextChild(pax2Node,"tckin_pax");

          if (!CrsQry.Eof)
          {
            if (tckin_route_confirm)
            {
              //это уже после подтверждения маршрута
              iPax->resCount=CreateSearchResponse(currSeg.point_dep,CrsQry,iPax->node);
              if (iPax->resCount>0)
              {
                for(xmlNodePtr tripNode=NodeAsNode("trips",iPax->node)->children;
                               tripNode!=NULL;
                               tripNode=tripNode->next)
                  for(xmlNodePtr pnrNode=NodeAsNode("groups",tripNode)->children;
                                 pnrNode!=NULL;
                                 pnrNode=pnrNode->next)
                  {
                    string pax_cl=NodeAsString("class",pnrNode);
                    if (cl.find(pax_cl)==string::npos) cl.append(pax_cl);
                  };
              };
            }
            else
            {
              iPax->resCount=0;
              for(;!CrsQry.Eof;CrsQry.Next())
              {
                iPax->resCount++;
                string pax_cl=CrsQry.FieldAsString("class");
                if (cl.find(pax_cl)==string::npos) cl.append(pax_cl);
              };
            };
          }
          else
          {
            //узнаем какому классу принадлежит подкласс из телеграмм
            Qry.Clear();
            Qry.SQLText=
              "SELECT DISTINCT class "
              "FROM tlg_binding,crs_pnr "
              "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
              "      tlg_binding.point_id_spp=:point_id AND "
              "      crs_pnr.system='CRS' AND "
              "      crs_pnr.subclass=:subclass ";
            Qry.CreateVariable("point_id",otInteger,currSeg.point_dep);
            Qry.CreateVariable("subclass",otString,iPax->subclass);
            Qry.Execute();
            if (!Qry.Eof)
            {
              for(;!Qry.Eof;Qry.Next())
              {
                string pax_cl=Qry.FieldAsString("class");
                if (cl.find(pax_cl)==string::npos) cl.append(pax_cl);
              };
            }
            else
            {
              try
              {
                TSubclsRow& row=(TSubclsRow&)base_tables.get("subcls").get_row("code/code_lat",iPax->subclass);
                if (cl.find(row.cl)==string::npos) cl.append(row.cl);
              }
              catch(EBaseTableError) {};
            };
          };



          //resCount содержит кол-во реально найденных пассажиров
          if (iPax->resCount<=iPax->reqCount)
          {
            //нашли меньше или равное кол-во пассажиров, соответствующих кол-ву повторяющихся пассажиров
            paxCountInPNL+=iPax->resCount;
          }
          else
          {
            //нашли лишних людей
            paxCountInPNL+=iPax->reqCount;
            doublePax=true;
          };
        };

        if (tckin_route_confirm)
        {
          paxNode=NodeAsNode("passengers",reqNode)->children;
          pax2Node=NewTextChild(seg2Node,"tckin_passengers");
          xmlNodePtr node;
          for(int pax_no=1;paxNode!=NULL;paxNode=paxNode->next,pax_no++)
          {
            TCkinPaxInfo paxInfo;
            paxInfo.surname=NodeAsString("surname",paxNode);
            paxInfo.name=NodeAsString("name",paxNode);
            paxInfo.pers_type=NodeAsString("pers_type",paxNode);
            paxInfo.seats=NodeAsInteger("seats",paxNode)==0?0:1;
            paxInfo.subclass.clear();
           /* paxInfo.reqCount=1;
            paxInfo.resCount=0;
            paxInfo.node=NULL;*/
            //ищем соответствующий сегменту подкласс пассажира
            paxInfo.subclass=f->pax.at(pax_no-1).subclass;

            for(iPax=pax.begin();iPax!=pax.end();iPax++)
            {
              if (transliter_equal(paxInfo.surname,iPax->surname) &&
                  transliter_equal(paxInfo.name,iPax->name) &&
                  paxInfo.pers_type==iPax->pers_type &&
                  paxInfo.seats==iPax->seats &&
                  paxInfo.subclass==iPax->subclass &&
                  iPax->node!=NULL)
              {
                node=CopyNode(pax2Node,iPax->node,true);
                if (iPax->resCount<iPax->reqCount)
                {
                  //добавить norec
                  xmlNodePtr norecNode=NewTextChild(node,"norec");
                  norecNode=NewTextChild(norecNode,"pax");

                  NewTextChild(norecNode,"surname",NodeAsString("surname",paxNode),"");
                  NewTextChild(norecNode,"name",NodeAsString("name",paxNode),"");
                  NewTextChild(norecNode,"pers_type",NodeAsString("pers_type",paxNode),
                                                     EncodePerson(ASTRA::adult));
                  NewTextChild(norecNode,"seats",NodeAsInteger("seats",paxNode),1);
                  NewTextChild(norecNode,"subclass",iPax->subclass);
                };
                break;
              };
            };
          };
        };

        pax2Node=NodeAsNode("temp_passengers",segNode);
        xmlUnlinkNode(pax2Node);
        xmlFreeNode(pax2Node);

        //наличие в PNL
        if (paxCountInPNL<paxCount)
          SetProp(NewTextChild(segNode,"pnl",
                               IntToString(paxCountInPNL)+" "+AstraLocale::getLocaleText("из")+" "+IntToString(paxCount)),
                  "error","NOREC");
        else
          if (doublePax)
            SetProp(NewTextChild(segNode,"pnl",AstraLocale::getLocaleText("Дубль в PNL")),"error","DOUBLE");
          else
            NewTextChild(segNode,"pnl","+");


        if (cl.size()==1)
        {
          NewTextChild(segNode,"class",ElemIdToCodeNative(etClass,cl));

          //запишем класс если подтверждение маршрута
          if (tckin_route_confirm)
          {
            try
            {
              TClassesRow& classesRow=(TClassesRow&)base_tables.get("classes").get_row("code",cl);
              NewTextChild( seg2Node, "class_code", classesRow.code );
              if (!reqInfo->desk.compatible(PAD_VERSION))
                NewTextChild( seg2Node, "class_name", classesRow.name );
            }
            catch(EBaseTableError) {};
          };
        }
        else
        {
          if (cl.empty())
            SetProp(NewTextChild(segNode,"class",AstraLocale::getLocaleText("Нет")),"error","CRITICAL");
          else
          {
            string cl_view;
            for(string::iterator c=cl.begin();c!=cl.end();c++)
             cl_view.append(ElemIdToCodeNative(etClass,*c));
            SetProp(NewTextChild(segNode,"class",cl_view),"error","CRITICAL");
          };
        };

        //наличие мест
        if (currSeg.fltInfo.pr_del!=0)
        {
          SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("Рейс отменен")),"error","CRITICAL"); //???
        }
        else
          if (currSeg.point_arv==ASTRA::NoExists)
          {
            //не нашли пункта в маршруте
            SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("Нет п/н")),"error","CRITICAL");
          }
          else
            if (cl.size()==1)
            {
              //считаем кол-во свободных мест
              //(после поиска пассажиров, так как необходимо определить базовый класс)
              int free=CheckCounters(currSeg.point_dep,currSeg.point_arv,(char*)cl.c_str(),psTCheckin);
              if (free<seatsSum)
              {
                xmlNodePtr wlNode;
                if (free<=0)
                  wlNode=NewTextChild(segNode,"free",AstraLocale::getLocaleText("Нет"));
                else
                  wlNode=NewTextChild(segNode,"free",
                                      IntToString(free)+" "+AstraLocale::getLocaleText("из")+" "+IntToString(seatsSum));
                SetProp(wlNode,"error","WL");
                SetProp(wlNode,"wl_type","O");
              }
              else
                NewTextChild(segNode,"free","+");
            }
            else
              SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("Неопред. кл.")),"error","CRITICAL");

      };
    }
    else
    {
      //не нашли в СПП рейс, соответствующий трансферному сегменту
      NewTextChild(segNode,"flight",flight.str());

      if (!s->is_edi)
        SetProp(NewTextChild(segNode,"classes",AstraLocale::getLocaleText("Нет в СПП")),"error","CRITICAL");
      else
        NewTextChild(segNode,"classes","EDIFACT"); //не является ошибкой
      NewTextChild(segNode,"pnl");
      NewTextChild(segNode,"class");
      NewTextChild(segNode,"free");
    };

    if (!tckinSets.pr_permit || s->is_edi) total_permit=false;
    bool total_waitlist=false;
    string wl_type;
    if (total_permit)
      for(xmlNodePtr node=segNode->children;node!=NULL;node=node->next)
      {
        xmlNodePtr errNode=GetNode("@error",node);
        if (errNode==NULL || NodeIsNULL(errNode)) continue;
        string error=NodeAsString(errNode);
        if (error=="CRITICAL" ||
            error=="NOREC" && !tckinSets.pr_norec ||
            error=="WL" && !tckinSets.pr_waitlist)
        {
          total_permit=false;
          break;
        };
        if (error=="WL" && tckinSets.pr_waitlist)
        {
          total_waitlist=true;
          wl_type=NodeAsString("@wl_type",node);
        };
      };

    if (tckin_route_confirm)
    {
      if (total_permit && total_waitlist)
        NewTextChild(seg2Node,"wl_type",wl_type);
      xmlNodePtr operFltNode;
      if (reqInfo->desk.compatible(PAD_VERSION))
        operFltNode=NodeAsNode("tripheader",seg2Node);
      else
        operFltNode=seg2Node;

      if (s->flts.size()==1)
        readTripSets( s->flts.begin()->point_dep, s->flts.begin()->fltInfo, operFltNode );
    };

    if (total_permit)
    {
      if (total_waitlist)
      {
        NewTextChild(segNode,"total",AstraLocale::getLocaleText("ЛО"));
        NewTextChild(segNode,"calc_status","WL");
      }
      else
      {
        NewTextChild(segNode,"total",AstraLocale::getLocaleText("Рег"));
        NewTextChild(segNode,"calc_status","CHECKIN");
      };
    }
    else
    {
      NewTextChild(segNode,"total",AstraLocale::getLocaleText("Нет"));
      NewTextChild(segNode,"calc_status","NONE");
    };

    if (NodeAsString("calc_status",segNode) != s->calc_status)
      //данные устарели по сравнению с предыдущим запросом сквозного маршрута
      irrelevant_data=true;
  };
  if (irrelevant_data)
  {
    //данные устарели
    xmlUnlinkNode(segsNode);
    xmlFreeNode(segsNode);
    return;
  };
  //данные не устарели - надо считывать подробно данные пассажиров по сегментам




  xmlUnlinkNode(routeNode);
  xmlFreeNode(routeNode);

};

void CheckInInterface::ParseScanDocData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw AstraLocale::UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");
};

namespace CheckIn
{

void showError(const std::map<int, std::map<int, AstraLocale::LexemaData> > &segs)
{
  if (segs.empty()) throw EXCEPTIONS::Exception("CheckIn::showError: empty segs!");
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);
  xmlNodePtr cmdNode = GetNode( "command", resNode );
  if (cmdNode==NULL) cmdNode=NewTextChild( resNode, "command" );
  resNode = ReplaceTextChild( cmdNode, "checkin_user_error" );
  xmlNodePtr segsNode = NewTextChild(resNode, "segments");
  for(std::map<int, std::map<int, AstraLocale::LexemaData> >::const_iterator s=segs.begin(); s!=segs.end(); s++)
  {
    xmlNodePtr segNode = NewTextChild(segsNode, "segment");
    if (s->first!=NoExists)
      NewTextChild(segNode, "point_id", s->first);
    xmlNodePtr paxsNode=NewTextChild(segNode, "passengers");
    for(std::map<int, AstraLocale::LexemaData>::const_iterator pax=s->second.begin(); pax!=s->second.end(); pax++)
    {
      string text, master_lexema_id;
      getLexemaText( pax->second, text, master_lexema_id );
      if (pax->first!=NoExists)
      {
        xmlNodePtr paxNode=NewTextChild(paxsNode, "pax");
        NewTextChild(paxNode, "crs_pax_id", pax->first);
        NewTextChild(paxNode, "error_code", master_lexema_id);
        NewTextChild(paxNode, "error_message", text);
      }
      else
      {
        NewTextChild(segNode, "error_code", master_lexema_id);
        NewTextChild(segNode, "error_message", text);
      };
    };
  };
};

}; //namespace CheckIn





