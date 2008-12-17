#include "checkin.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "seats.h"
#include "stages.h"
#include "tripinfo.h"
#include "telegram.h"
#include "misc.h"
#include "payment.h"
#include "astra_misc.h"
#include "base_tables.h"
#include "convert.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;

class OverloadException: public UserException
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
    throw UserException(pos+100,"Ошибка в запросе");
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
  if (grp.fams.empty()) throw UserException(100,"Неверный запрос");
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
      throw UserException(100,"Фамилия или имя пассажира в запросе слишком большой длины");

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
              throw UserException(100,"Кол-во РМ без мест превышает общее кол-во РМ для одной из указанных фамилий");
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
              throw UserException(100,"Кол-во РМ с местами превышает общее кол-во РМ для одной из указанных фамилий");
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
        throw UserException(100,"Кол-во пассажиров с местами превышает суммарное кол-во пассажиров по типам");
      if (info.n[baby]<info.nPax-info.nPaxWithPlace)
        throw UserException(100,"Кол-во РМ без мест превышает общее кол-во РМ");
    };
    for(int p=0;p<NoPerson;p++)
      sum.n[p]+=info.n[p];
    sum.nPax+=info.nPax;
    sum.nPaxWithPlace+=info.nPaxWithPlace;
    sum.fams.push_back(info);
  };
  if (grp.large && sum.nPaxWithPlace<=9) grp.large=false;
  if (sum.nPaxWithPlace==0)
    throw UserException(100,"Кол-во пассажиров с местами должно быть больше нуля");
  if (sum.nPax<sum.nPaxWithPlace)
    throw UserException(100,"Кол-во пассажиров с местами превышает суммарное кол-во пассажиров по типам");
  if (sum.nPax-sum.nPaxWithPlace>sum.n[adult])
    throw UserException(100,"Кол-во РМ без мест в группе превышает кол-во ВЗ");

  if (sum.persCountFmt==1 && sum.nPax<(int)sum.fams.size())
    throw UserException(100,"Кол-во фамилий превышает кол-во пассажиров в группе");

  if (grp.prefix=='+')
    throw UserException(100,"Используйте выбор статуса 'Подсадка' вместо знака '+' в начале запроса");
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
                      string airline_out, int flt_no_out)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_permit "
    "FROM trfer_set,trfer_set_flts flts_in,trfer_set_flts flts_out "
    "WHERE trfer_set.id=flts_in.id(+) AND flts_in.pr_onward(+)=0 AND "
    "      trfer_set.id=flts_out.id(+) AND flts_out.pr_onward(+)=1 AND "
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
  return (!Qry.Eof && Qry.FieldAsInteger("pr_permit")!=0);
};

int CreateSearchResponse(int point_dep, TQuery &PaxQry,  xmlNodePtr resNode)
{
  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText =
    "SELECT transfer_num,airline,flt_no,suffix, "
    "       local_date,airp_arv,subclass "
    "FROM crs_transfer WHERE pnr_id=:pnr_id AND transfer_num>0";
  TrferQry.DeclareVariable("pnr_id",otInteger);

  TQuery PnrAddrQry(&OraSession);
  PnrAddrQry.SQLText =
    "SELECT airline,addr FROM pnr_addrs WHERE pnr_id=:pnr_id";
  PnrAddrQry.DeclareVariable("pnr_id",otInteger);

  TQuery RemQry(&OraSession);
  RemQry.SQLText =
    "SELECT rem_code,rem FROM crs_pax_rem WHERE pax_id=:pax_id";
  RemQry.DeclareVariable("pax_id",otInteger);

  int point_id=-1;
  int pnr_id=-1, pax_id;
  xmlNodePtr tripNode,pnrNode,paxNode,node;

  int count=0;
  tripNode=NewTextChild(resNode,"trips");
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    count++;
    if (PaxQry.FieldAsInteger("point_id")!=point_id)
    {
      node=NewTextChild(tripNode,"trip");
      point_id=PaxQry.FieldAsInteger("point_id");
      NewTextChild(node,"point_id",point_id);
      NewTextChild(node,"airline",PaxQry.FieldAsString("airline"));
      NewTextChild(node,"flt_no",PaxQry.FieldAsInteger("flt_no"));
      NewTextChild(node,"scd",DateTimeToStr(PaxQry.FieldAsDateTime("scd")));
      NewTextChild(node,"airp_dep",PaxQry.FieldAsString("airp_dep"));
      pnrNode=NewTextChild(node,"groups");
    };
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      node=NewTextChild(pnrNode,"pnr");
      pnr_id=PaxQry.FieldAsInteger("pnr_id");
      NewTextChild(node,"pnr_id",pnr_id);
      NewTextChild(node,"airp_arv",PaxQry.FieldAsString("target"));
      NewTextChild(node,"subclass",PaxQry.FieldAsString("subclass"));
      NewTextChild(node,"class",PaxQry.FieldAsString("class"));
      paxNode=NewTextChild(node,"passengers");

      TrferQry.SetVariable("pnr_id",pnr_id);
      TrferQry.Execute();
      if (!TrferQry.Eof)
      {
        //есть трансфер
        string airp_dep=PaxQry.FieldAsString("airp_dep");
        string airline_in=PaxQry.FieldAsString("airline");
        int flt_no_in=PaxQry.FieldAsInteger("flt_no");
        string airp_trfer=PaxQry.FieldAsString("target");
        string airline_out;
        int flt_no_out;

        TTripInfo fltInfo;
        fltInfo.airline=airline_in;
        fltInfo.flt_no=flt_no_in;
        fltInfo.airp=airp_dep;
        bool without_trfer_set=GetTripSets( tsIgnoreTrferSet, fltInfo );

        bool pr_permit=true;
        xmlNodePtr trferNode=NewTextChild(node,"transfer");
        for(;!TrferQry.Eof;TrferQry.Next())
        {
          if (pr_permit && !without_trfer_set)
          {
            //проверим оформление трансфера
            try
            {
              TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code/code_lat",TrferQry.FieldAsString("airline"));
              airline_out=row.code;
              flt_no_out=TrferQry.FieldAsInteger("flt_no");
              if (!CheckTrferPermit(airline_in,flt_no_in,airp_trfer,airline_out,flt_no_out))
                pr_permit=false;
            }
            catch(EBaseTableError &e)
            {
              pr_permit=false;
            };
          };

          xmlNodePtr node2=NewTextChild(trferNode,"segment");
          NewTextChild(node2,"num",TrferQry.FieldAsInteger("transfer_num"));
          NewTextChild(node2,"airline",TrferQry.FieldAsString("airline"));
          NewTextChild(node2,"flt_no",TrferQry.FieldAsInteger("flt_no"));
          NewTextChild(node2,"suffix",TrferQry.FieldAsString("suffix"),"");
          NewTextChild(node2,"local_date",TrferQry.FieldAsInteger("local_date"));
          NewTextChild(node2,"airp_arv",TrferQry.FieldAsString("airp_arv"));
          NewTextChild(node2,"subclass",TrferQry.FieldAsString("subclass"));
          NewTextChild(node2,"pr_permit",(int)pr_permit);

          if (pr_permit)
          {
            try
            {
              airline_in=airline_out;
              flt_no_in=flt_no_out;
              TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code/code_lat",TrferQry.FieldAsString("airp_arv"));
              airp_trfer=row.code;
              if (airp_dep==airp_trfer)
              {
                //обратный сегмент, а не стыковочный
                pr_permit=false;
                ReplaceTextChild(node2,"pr_permit",(int)pr_permit);
              };
            }
            catch(EBaseTableError &e)
            {
              pr_permit=false;
            };
          };
        };
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
        if (!PnrAddrQry.Eof||airline!=PaxQry.FieldAsString("airline"))
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
      }
     else NewTextChild(node,"pnr_addrs"); //это времмено пока не обновлен терминал 07.08.2008 !!!
    };
    node=NewTextChild(paxNode,"pax");
    pax_id=PaxQry.FieldAsInteger("pax_id");
    NewTextChild(node,"pax_id",pax_id);
    NewTextChild(node,"surname",PaxQry.FieldAsString("surname"));
    NewTextChild(node,"name",PaxQry.FieldAsString("name"),"");
    NewTextChild(node,"pers_type",PaxQry.FieldAsString("pers_type"),EncodePerson(ASTRA::adult));
    NewTextChild(node,"seat_no",PaxQry.FieldAsString("seat_no"),"");
    NewTextChild(node,"preseat_no",PaxQry.FieldAsString("preseat_no"),"");
    NewTextChild(node,"seat_type",PaxQry.FieldAsString("seat_type"),"");
    NewTextChild(node,"seats",PaxQry.FieldAsInteger("seats"),1);
    NewTextChild(node,"document",PaxQry.FieldAsString("document"),"");
    //обработка билетов
    string ticket_no;
    if (!PaxQry.FieldIsNULL("eticket"))
    {
      //билет TKNE
      ticket_no=PaxQry.FieldAsString("eticket");
      NewTextChild(node,"ticket",ticket_no);  // потом убрать 31.10.08
      NewTextChild(node,"is_tkne",(int)true); // потом убрать 31.10.08

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
        NewTextChild(node,"ticket",ticket_no); //потом убрать 31.10.08
        NewTextChild(node,"is_tkne",(int)false); // потом убрать 31.10.08

        NewTextChild(node,"ticket_no",ticket_no);
        NewTextChild(node,"ticket_rem","TKNA");
      };
    };

    RemQry.SetVariable("pax_id",pax_id);
    RemQry.Execute();
    if (!RemQry.Eof)
    {
      xmlNodePtr remNode=NewTextChild(node,"rems");
      for(;!RemQry.Eof;RemQry.Next())
      {
        xmlNodePtr node2=NewTextChild(remNode,"rem");
        NewTextChild(node2,"rem_code",RemQry.FieldAsString("rem_code"),"");
        NewTextChild(node2,"rem_text",RemQry.FieldAsString("rem"));
      };
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
      "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.target,crs_pnr.subclass, "
      "       crs_pnr.class,crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS preseat_no, "
      "       crs_pax.seat_type, "
      "       crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       tlg_trips.airline,tlg_trips.flt_no,tlg_trips.scd,tlg_trips.airp_dep, "
      "       report.get_PSPT(crs_pax.pax_id) AS document, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket "
      "FROM tlg_trips,crs_pnr,crs_pax,pax "
      "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
      "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      crs_pnr.pnr_id=:pnr_id AND "
      "      crs_pax.pr_del=0 AND "
      "      pax.pax_id IS NULL "
      "ORDER BY tlg_trips.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";
    PaxQry.CreateVariable("pnr_id",otInteger,NodeAsInteger("pnr_id",reqNode));
    PaxQry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType(ASTRA::cltProtCkin) );
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

void CheckInInterface::SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep = NodeAsInteger("point_dep",reqNode);
  TPaxStatus pax_status = DecodePaxStatus(NodeAsString("pax_status",reqNode));
  string query= NodeAsString("query",reqNode);
  bool pr_unaccomp=query=="0";

  TInquiryGroup grp;
  TInquiryGroupSummary sum;
  if (!pr_unaccomp)
  {
    readTripCounters(point_dep,resNode);

    ParseInquiryStr(query,grp);
    TInquiryFormat fmt;
    fmt.persCountFmt=0;
    fmt.infSeatsFmt=0;


    TQuery Qry(&OraSession);
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

  if (pax_status==psTransit)
  {
    TQuery Qry(&OraSession);
    Qry.SQLText="SELECT pr_tranz_reg FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_dep);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
    if (Qry.FieldIsNULL("pr_tranz_reg")||Qry.FieldAsInteger("pr_tranz_reg")==0)
      throw UserException("Перерегистрация транзита на данный рейс не производится");
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
    sql=  "SELECT crs_pnr.pnr_id, "
          "       MIN(crs_pnr.grp_name) AS grp_name, "
          "       MIN(DECODE(crs_pax.pers_type,'ВЗ', "
          "                  crs_pax.surname||' '||crs_pax.name,'')) AS pax_name, "
          "       COUNT(*) AS seats_all, "
          "       SUM(DECODE(pax.pax_id,NULL,1,0)) AS seats "
          "FROM crs_pnr,crs_pax,pax, "
          "  (SELECT DISTINCT crs_pnr.pnr_id "
          "   FROM crs_pnr, "
          "    (SELECT b2.point_id_tlg, "
          "            airp_arv_tlg,class_tlg,status "
          "     FROM crs_displace2,tlg_binding b1,tlg_binding b2 "
          "     WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND "
          "           b1.point_id_spp=b2.point_id_spp AND "
          "           crs_displace2.point_id_spp=:point_id AND "
          "           b1.point_id_spp<>:point_id) crs_displace "
          "   WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND "
          "         crs_pnr.target=crs_displace.airp_arv_tlg AND "
          "         crs_pnr.class=crs_displace.class_tlg AND "
          "         crs_displace.status= :status AND "
          "         crs_pnr.wl_priority IS NULL ";

    if (pax_status==psOk)
      sql+="  UNION ";
    else
      sql+="  MINUS ";

    sql+= "   SELECT DISTINCT crs_pnr.pnr_id "
          "   FROM crs_pnr,tlg_binding "
          "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
          "         tlg_binding.point_id_spp= :point_id AND "
          "         crs_pnr.wl_priority IS NULL) ids "
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND "
          "      ids.pnr_id=crs_pnr.pnr_id AND "
          "      crs_pax.pr_del=0 AND "
          "      crs_pax.seats>0 "
          "GROUP BY crs_pnr.pnr_id "
          "HAVING COUNT(*)>= :seats ";

    PaxQry.SQLText = sql;
    PaxQry.CreateVariable("point_id",otInteger,point_dep);
    PaxQry.CreateVariable("status",otString,EncodePaxStatus(pax_status));
    PaxQry.CreateVariable("seats",otInteger,sum.nPaxWithPlace);
    PaxQry.Execute();
    if (!PaxQry.Eof)
    {
      TQuery PnrAddrQry(&OraSession);
      PnrAddrQry.SQLText =
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
      "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.target,crs_pnr.subclass, "
      "       crs_pnr.class,crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
      "       salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, "
      "       salons.get_crs_seat_no(crs_pax.pax_id,:protckin_layer,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS preseat_no, "
      "       crs_pax.seat_type,crs_pax.seats, "
      "       crs_pnr.pnr_id, "
      "       tlg_trips.airline,tlg_trips.flt_no,tlg_trips.scd,tlg_trips.airp_dep, "
      "       report.get_PSPT(crs_pax.pax_id) AS document, "
      "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, "
      "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket "
      "FROM tlg_trips,crs_pnr,crs_pax,pax, ";
    if (sum.nPax>1)
      sql+=
          "  (SELECT DISTINCT crs_pnr.pnr_id ";
    else
      sql+=
          "  (SELECT DISTINCT crs_pax.pax_id ";

    sql+= "   FROM crs_pnr,crs_pax,pax, "
          "    (SELECT b2.point_id_tlg, "
          "            airp_arv_tlg,class_tlg,status "
          "     FROM crs_displace2,tlg_binding b1,tlg_binding b2 "
          "     WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND "
          "           b1.point_id_spp=b2.point_id_spp AND "
          "           crs_displace2.point_id_spp=:point_id AND "
          "           b1.point_id_spp<>:point_id) crs_displace "
          "   WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND "
          "         crs_pnr.target=crs_displace.airp_arv_tlg AND "
          "         crs_pnr.class=crs_displace.class_tlg AND "
          "         crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "         crs_displace.status= :status AND "
          "         crs_pnr.wl_priority IS NULL AND "
          "         crs_pax.pr_del=0 AND "
          "         crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL AND "
          "         ("+surnames+") ";

    if (pax_status==psOk)
      sql+="  UNION ";
    else
      sql+="  MINUS ";

    if (sum.nPax>1)
      sql+=
          "   SELECT DISTINCT crs_pnr.pnr_id ";
      else
      sql+=
          "   SELECT DISTINCT crs_pax.pax_id ";

    sql+=
          "   FROM crs_pnr,tlg_binding,crs_pax,pax "
          "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
          "         crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "         tlg_binding.point_id_spp= :point_id AND "
          "         crs_pnr.wl_priority IS NULL AND"
          "         crs_pax.pr_del=0 AND "
          "         crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL AND "
          "         ("+surnames+")) ids  "
          "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
          "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND ";
    if (sum.nPax>1)
      sql+=
          "      ids.pnr_id=crs_pnr.pnr_id AND ";
    else
      sql+=
          "      ids.pax_id=crs_pax.pax_id AND ";

    sql+= "      crs_pax.pr_del=0 AND "
          "      pax.pax_id IS NULL "
          "ORDER BY tlg_trips.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id ";

    PaxQry.SQLText = sql;
    PaxQry.CreateVariable("point_id",otInteger,point_dep);
    PaxQry.CreateVariable("status",otString,EncodePaxStatus(pax_status));
    PaxQry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType(ASTRA::cltProtCkin) );
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
      ProgError(STDLOG,"counters2 empty! (point_dep=%d point_arv=%d cl=%s)",point_dep,point_arv,cl);
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
      case psOk:
      	        if (ckin_stage==sOpenCheckIn)
                   free=Qry.FieldAsInteger("free_ok");
                 else
                   free=Qry.FieldAsInteger("nooccupy");
                 break;
      default:   if (ckin_stage==sOpenCheckIn)
                   free=Qry.FieldAsInteger("free_goshow");
                 else
                   free=Qry.FieldAsInteger("nooccupy");
    };
    if (free>=0)
      return free;
    else
      return 0;
};

bool CheckInInterface::CheckFltOverload(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.SQLText=
    "SELECT scd_out,airp FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
  bool prSummer=is_dst(Qry.FieldAsDateTime("scd_out"),
                       AirpTZRegion(Qry.FieldAsString("airp")));

  Qry.SQLText=
    "SELECT pr_check_load,pr_overload_reg,max_commerce FROM trip_sets WHERE point_id=:point_id";
  Qry.Execute();
  if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  if (Qry.FieldIsNULL("max_commerce")) return false;
  int max_commerce=Qry.FieldAsInteger("max_commerce");
  bool pr_check_load=Qry.FieldAsInteger("pr_check_load")!=0;
  bool pr_overload_reg=Qry.FieldAsInteger("pr_overload_reg")!=0;

  if (!pr_check_load && pr_overload_reg) return false;

  int load=0;
  Qry.SQLText=
    "SELECT NVL(SUM(weight_win),0) AS weight_win, "
    "       NVL(SUM(weight_sum),0) AS weight_sum "
    "FROM pax_grp,pax,pers_types "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax.pers_type=pers_types.code AND "
    "      pax_grp.point_dep=:point_id AND pax.refuse IS NULL";
  Qry.Execute();
  if (!Qry.Eof)
  {
    if (prSummer)
      load+=Qry.FieldAsInteger("weight_sum");
    else
      load+=Qry.FieldAsInteger("weight_win");
  };

  Qry.SQLText=
    "SELECT NVL(SUM(weight),0) AS weight "
    "FROM pax_grp,bag2 "
    "WHERE pax_grp.grp_id=bag2.grp_id AND "
    "      pax_grp.point_dep=:point_id AND pax_grp.bag_refuse=0";
  Qry.Execute();
  if (!Qry.Eof)
    load+=Qry.FieldAsInteger("weight");

  Qry.SQLText=
    "SELECT NVL(SUM(cargo),0) AS cargo, "
    "       NVL(SUM(mail),0) AS mail "
    "FROM trip_load "
    "WHERE point_dep=:point_id";
  Qry.Execute();
  if (!Qry.Eof)
    load+=Qry.FieldAsInteger("cargo")+Qry.FieldAsInteger("mail");

  ProgTrace(TRACE5,"max_commerce=%d load=%d",max_commerce,load);

  if (load>max_commerce)
  {
    if (pr_overload_reg)
    {
      showErrorMessage("Превышение максимальной коммерческой загрузки на рейс");
      return true;
    }
    else
      throw OverloadException("Превышение максимальной коммерческой загрузки на рейс");
  };
  return false;
};

void CheckInInterface::PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readPaxLoad( point_id, reqNode, resNode );

  TQuery Qry(&OraSession);

  ostringstream sql;
  sql <<
    "SELECT "
    "  reg_no,surname,name,pax_grp.airp_arv, "
    "  report.get_last_trfer(pax.grp_id) AS last_trfer, "
    "  class,pax.subclass, "
    "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no, "
    "  seats,pers_type,document, "
    "  ticket_no||DECODE(coupon_no,NULL,NULL,'/'||coupon_no) AS ticket_no, "
    "  ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum) AS bag_amount, "
    "  ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum) AS bag_weight, "
    "  ckin.get_rkWeight(pax.grp_id,pax.pax_id,rownum) AS rk_weight, "
    "  ckin.get_excess(pax.grp_id,pax.pax_id) AS excess, "
    "  ckin.get_birks(pax.grp_id,pax.pax_id) AS tags, "
    "  report.get_remarks(pax_id,0) AS rems, "
    "  pax.grp_id, "
    "  pax.pax_id, "
    "  pax_grp.class_grp AS cl_grp_id,pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id ";

  if (strcmp((char *)reqNode->name, "BagPaxList")==0)
    sql <<
    " ,ckin.get_receipts(pax.grp_id,pax.pax_id) AS receipts, "
    "  kassa.pr_payment(pax.grp_id) AS pr_payment ";

  sql <<
    "FROM pax_grp,pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      point_dep=:point_id AND pr_brd IS NOT NULL ";
  if (strcmp((char *)reqNode->name, "BagPaxList")==0)
    sql <<
    "  AND pax_grp.excess>0 ";

  sql <<
    "ORDER BY reg_no"; //в будущем убрать ORDER BY

  ProgTrace(TRACE5, "%s", sql.str().c_str());

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );
  Qry.Execute();

  int col_reg_no=Qry.FieldIndex("reg_no");
  int col_surname=Qry.FieldIndex("surname");
  int col_name=Qry.FieldIndex("name");
  int col_airp_arv=Qry.FieldIndex("airp_arv");
  int col_last_trfer=Qry.FieldIndex("last_trfer");
  int col_class=Qry.FieldIndex("class");
  int col_subclass=Qry.FieldIndex("subclass");
  int col_seat_no=Qry.FieldIndex("seat_no");
  int col_pers_type=Qry.FieldIndex("pers_type");
  int col_document=Qry.FieldIndex("document");
  int col_ticket_no=Qry.FieldIndex("ticket_no");
  int col_bag_amount=Qry.FieldIndex("bag_amount");
  int col_bag_weight=Qry.FieldIndex("bag_weight");
  int col_rk_weight=Qry.FieldIndex("rk_weight");
  int col_excess=Qry.FieldIndex("excess");
  int col_tags=Qry.FieldIndex("tags");
  int col_rems=Qry.FieldIndex("rems");
  int col_grp_id=Qry.FieldIndex("grp_id");
  int col_cl_grp_id=Qry.FieldIndex("cl_grp_id");
  int col_hall_id=Qry.FieldIndex("hall_id");
  int col_point_arv=Qry.FieldIndex("point_arv");
  int col_user_id=Qry.FieldIndex("user_id");
  int col_receipts=-1;
  int col_pr_payment=-1;
  if (strcmp((char *)reqNode->name, "BagPaxList")==0)
  {
    col_receipts=Qry.FieldIndex("receipts");
    col_pr_payment=Qry.FieldIndex("pr_payment");
  };

  int grp_id = -1;
  bool rcpt_exists = false;
  xmlNodePtr node=NewTextChild(resNode,"passengers");
  vector<xmlNodePtr> v_rcpt_complete;
  // В вектор v_rcpt_complete записываются указатели на узлы, в которых хранится признак
  // все ли квитанции распечатаны 0 - частично напечатаны, 1 - все напечатаны, 2 - нет ни одной квитанции
  // Поскольку инфа по квитанциям есть только у одного пассажира из группы, понять значение
  // признака можно только пробежав группу до конца. Отсюда такой гемор.
  // При смене группы (и после отработки цикла) происходит инициализация признака у всех пассажиров предыдущей группы.
  int rcpt_complete = 0;
  for(;!Qry.Eof;Qry.Next())
  {
    int tmp_grp_id = Qry.FieldAsInteger("grp_id");
    if(grp_id != tmp_grp_id) {
        if(!v_rcpt_complete.empty()) {
            for(vector<xmlNodePtr>::iterator iv = v_rcpt_complete.begin(); iv != v_rcpt_complete.end(); iv++)
                NodeSetContent(*iv, rcpt_exists ? rcpt_complete : 2);
            v_rcpt_complete.clear();
            rcpt_exists = false;
        }
        grp_id = tmp_grp_id;
    }

    xmlNodePtr paxNode=NewTextChild(node,"pax");
    NewTextChild(paxNode,"reg_no",Qry.FieldAsInteger(col_reg_no));
    NewTextChild(paxNode,"surname",Qry.FieldAsString(col_surname));
    NewTextChild(paxNode,"name",Qry.FieldAsString(col_name));
    NewTextChild(paxNode,"airp_arv",Qry.FieldAsString(col_airp_arv));
    NewTextChild(paxNode,"last_trfer",
                 convertLastTrfer(Qry.FieldAsString(col_last_trfer)),"");
    NewTextChild(paxNode,"class",Qry.FieldAsString(col_class));
    NewTextChild(paxNode,"subclass",Qry.FieldAsString(col_subclass),"");
    NewTextChild(paxNode,"seat_no",Qry.FieldAsString(col_seat_no));
    NewTextChild(paxNode,"pers_type",Qry.FieldAsString(col_pers_type));
    NewTextChild(paxNode,"document",Qry.FieldAsString(col_document));
    NewTextChild(paxNode,"ticket_no",Qry.FieldAsString(col_ticket_no),"");
    NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger(col_bag_amount),0);
    NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger(col_bag_weight),0);
    NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger(col_rk_weight),0);
    NewTextChild(paxNode,"excess",Qry.FieldAsInteger(col_excess),0);
    NewTextChild(paxNode,"tags",Qry.FieldAsString(col_tags),"");
    NewTextChild(paxNode,"rems",Qry.FieldAsString(col_rems),"");
    if(col_receipts != -1)
    {
      NewTextChild(paxNode,"rcpt_no_list",Qry.FieldAsString(col_receipts));
      v_rcpt_complete.push_back(NewTextChild(paxNode,"rcpt_complete"));
      rcpt_complete = Qry.FieldAsInteger(col_pr_payment);
      rcpt_exists = rcpt_exists || !Qry.FieldIsNULL(col_receipts);
    };
    //идентификаторы
    NewTextChild(paxNode,"grp_id",Qry.FieldAsInteger(col_grp_id));
    NewTextChild(paxNode,"cl_grp_id",Qry.FieldAsInteger(col_cl_grp_id));
    NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger(col_hall_id));
    NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger(col_point_arv));
    NewTextChild(paxNode,"user_id",Qry.FieldAsInteger(col_user_id));
  };
  if(!v_rcpt_complete.empty()) {
      for(vector<xmlNodePtr>::iterator iv = v_rcpt_complete.begin(); iv != v_rcpt_complete.end(); iv++)
          NodeSetContent(*iv, rcpt_exists ? rcpt_complete : 2);
      v_rcpt_complete.clear();
      rcpt_exists = false;
  }

  //несопровождаемый багаж
  sql.str("");

  sql <<
    "SELECT "
    "  pax_grp.airp_arv, "
    "  report.get_last_trfer(pax_grp.grp_id) AS last_trfer, "
    "  ckin.get_bagAmount(pax_grp.grp_id,NULL) AS bag_amount, "
    "  ckin.get_bagWeight(pax_grp.grp_id,NULL) AS bag_weight, "
    "  ckin.get_rkWeight(pax_grp.grp_id,NULL) AS rk_weight, "
    "  ckin.get_excess(pax_grp.grp_id,NULL) AS excess, "
    "  ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
    "  pax_grp.grp_id, "
    "  pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id ";
  if (strcmp((char *)reqNode->name, "BagPaxList")==0)
    sql <<
    " ,ckin.get_receipts(pax_grp.grp_id,NULL) AS receipts, "
    "  kassa.pr_payment(pax_grp.grp_id) AS pr_payment ";

  sql <<
    "FROM pax_grp "
    "WHERE point_dep=:point_id AND class IS NULL ";

  if (strcmp((char *)reqNode->name, "BagPaxList")==0)
    sql <<
    "  AND pax_grp.excess>0 ";

  ProgTrace(TRACE5, "%s", sql.str().c_str());

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();

  node=NewTextChild(resNode,"unaccomp_bag");
  for(;!Qry.Eof;Qry.Next())
  {
    xmlNodePtr paxNode=NewTextChild(node,"bag");
    NewTextChild(paxNode,"airp_arv",Qry.FieldAsString("airp_arv"));
    NewTextChild(paxNode,"last_trfer",
                 convertLastTrfer(Qry.FieldAsString("last_trfer")),"");
    NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger("bag_amount"),0);
    NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger("bag_weight"),0);
    NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger("rk_weight"),0);
    NewTextChild(paxNode,"excess",Qry.FieldAsInteger("excess"),0);
    NewTextChild(paxNode,"tags",Qry.FieldAsString("tags"),"");
    if (strcmp((char *)reqNode->name, "BagPaxList")==0)
    {
      NewTextChild(paxNode,"rcpt_no_list",Qry.FieldAsString("receipts"));
      // все ли квитанции распечатаны 0 - частично напечатаны, 1 - все напечатаны, 2 - нет ни одной квитанции
      if (!Qry.FieldIsNULL("receipts"))
        NewTextChild(paxNode,"rcpt_complete",Qry.FieldAsInteger("pr_payment"));
      else
        NewTextChild(paxNode,"rcpt_complete",2);
    };
    //идентификаторы
    NewTextChild(paxNode,"grp_id",Qry.FieldAsInteger("grp_id"));
    NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger("hall_id"));
    NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger("point_arv"));
    NewTextChild(paxNode,"user_id",Qry.FieldAsInteger("user_id"));
  };

  Qry.Close();
};

bool GetUsePS()
{
	return false; //!!!
}

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_dep,point_arv,grp_id,hall;
  string cl,airp_dep,airp_arv;
  TReqInfo *reqInfo = TReqInfo::Instance();
  //reqInfo->user.check_access(amPartialWrite);
  TQuery Qry(&OraSession);
  //определим, открыт ли рейс для регистрации

  point_dep=NodeAsInteger("point_dep",reqNode);
  airp_dep=NodeAsString("airp_dep",reqNode);
  //лочим рейс
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,airp,point_num, "
    "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points "
    "WHERE point_id=:point_id AND airp=:airp AND pr_del=0 AND pr_reg<>0 FOR UPDATE";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.CreateVariable("airp",otString,airp_dep);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
  string airline=Qry.FieldAsString("airline");
  int flt_no=Qry.FieldAsInteger("flt_no");

  TTypeBSendInfo sendInfo;
  sendInfo.airline=Qry.FieldAsString("airline");
  sendInfo.flt_no=Qry.FieldAsInteger("flt_no");
  sendInfo.airp_dep=Qry.FieldAsString("airp");
  sendInfo.point_num=Qry.FieldAsInteger("point_num");
  sendInfo.first_point=Qry.FieldAsInteger("first_point");
  sendInfo.tlg_type="BSM";

  //savepoint для отката при превышении загрузки
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SAVEPOINT CHECKIN; "
    "END;";
  Qry.Execute();

  //BSM
  map<bool,string> BSMaddrs;
  TBSMContent BSMContentBefore;
  bool BSMsend=TelegramInterface::IsBSMSend(sendInfo,BSMaddrs);

  point_arv=NodeAsInteger("point_arv",reqNode);
  airp_arv=NodeAsString("airp_arv",reqNode);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airp FROM points "
    "WHERE point_id=:point_id AND airp=:airp AND pr_del=0";
  Qry.CreateVariable("point_id",otInteger,point_arv);
  Qry.CreateVariable("airp",otString,airp_arv);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");

  //map для норм
  map<int,string> norms;

  bool pr_unaccomp=strcmp((char *)reqNode->name, "SaveUnaccompBag") == 0;

  TQuery CrsQry(&OraSession);
  CrsQry.Clear();
  CrsQry.SQLText=
    "DECLARE "
    "  CURSOR cur1 IS "
    "    SELECT type,issue_country,no,nationality, "
    "           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi "
    "    FROM crs_pax_doc "
    "    WHERE pax_id=:pax_id "
    "    ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no; "
    "  row1 cur1%ROWTYPE; "
    "  CURSOR cur2 IS "
    "    SELECT airline,no,extra "
    "    FROM crs_pax_fqt "
    "    WHERE pax_id=:pax_id "
    "    ORDER BY airline,no,extra; "
    "  row2 cur2%ROWTYPE; "
    "  prior_airline crs_pax_fqt.airline%TYPE; "
    "BEGIN "
    "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
    "  DELETE FROM pax_fqt WHERE pax_id=:pax_id; "
    "  OPEN cur1; "
    "  FETCH cur1 INTO row1; "
    "  IF cur1%FOUND THEN "
    "    INSERT INTO pax_doc "
    "      (pax_id,type,issue_country,no,nationality, "
    "       birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "    VALUES "
    "      (:pax_id,row1.type,row1.issue_country,row1.no,row1.nationality, "
    "       row1.birth_date,row1.gender,row1.expiry_date,row1.surname,row1.first_name,row1.second_name,row1.pr_multi); "
    "  END IF; "
    "  CLOSE cur1; "
    "  prior_airline:=NULL; "
    "  FOR row2 IN cur2 LOOP "
    "    IF prior_airline=row2.airline THEN "
    "      NULL; "
    "    ELSE "
    "      INSERT INTO pax_fqt "
    "        (pax_id,airline,no,extra) "
    "      VALUES "
    "        (:pax_id,row2.airline,row2.no,row2.extra); "
    "      prior_airline:=row2.airline; "
    "    END IF; "
    "  END LOOP; "
    "END;";
  CrsQry.DeclareVariable("pax_id",otInteger);

  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_tranz_reg,pr_reg_with_tkn,pr_reg_with_doc,pr_etstatus "
    "FROM trip_sets WHERE point_id=:point_id ";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");

  bool pr_tranz_reg=!Qry.FieldIsNULL("pr_tranz_reg")&&Qry.FieldAsInteger("pr_tranz_reg")!=0;
  bool pr_reg_with_tkn=Qry.FieldAsInteger("pr_reg_with_tkn")!=0;
  bool pr_reg_with_doc=Qry.FieldAsInteger("pr_reg_with_doc")!=0;
  int pr_etstatus=Qry.FieldAsInteger("pr_etstatus");

  xmlNodePtr node,node2,remNode;
  //проверим номера документов и билетов
  if (!pr_unaccomp&&
      (pr_reg_with_tkn||pr_reg_with_doc))
  {
    node=NodeAsNode("passengers",reqNode);
    for(node=node->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (NodeIsNULLFast("pers_type",node2,true)) continue;
      if (NodeAsStringFast("pers_type",node2)=="РМ") continue;

      if (pr_reg_with_tkn&&NodeIsNULLFast("ticket_no",node2,true))
        throw UserException("При регистрации необходимо указывать номера билетов пассажиров");
      if (pr_reg_with_doc&&NodeIsNULLFast("document",node2,true))
        throw UserException("При регистрации необходимо указывать документы пассажиров");
    };
  };

  //определим - новая регистрация или запись изменений
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    cl=NodeAsString("class",reqNode);

    hall=NodeAsInteger("hall",reqNode);
    Qry.Clear();
    Qry.SQLText="SELECT pr_vip FROM halls2 WHERE id=:hall";
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Неверно указан зал регистрации");
    bool addVIP=Qry.FieldAsInteger("pr_vip")!=0;

    //новая регистрация
    //проверка наличия свободных мест
    TPaxStatus grp_status=DecodePaxStatus(NodeAsString("status",reqNode));
    if (grp_status==psTransit && !pr_tranz_reg)
      throw UserException("Перерегистрация транзита на данный рейс не производится");

    TSalons Salons( point_dep, rTripSalons );

    if (!pr_unaccomp)
    {
      int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
      string place_status;
      if (grp_status==psTransit)
        place_status="TR";
      else
        place_status="FP";

      //простановка ремарок VIP,EXST, если нужно
      //подсчет seats
      bool adultwithbaby = false;
      int seats,seats_sum=0;
      string rem_code;
      node=NodeAsNode("passengers",reqNode);
      for(node=node->children;node!=NULL;node=node->next)
      {
        node2=node->children;
        seats=NodeAsIntegerFast("seats",node2);
        if ( !seats )
        	adultwithbaby = true;
        seats_sum+=seats;
        bool flagVIP=false, flagSTCR=false, flagEXST=false;
        remNode=GetNodeFast("rems",node2);
        if (remNode!=NULL)
          for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
          {
            node2=remNode->children;
            rem_code=NodeAsStringFast("rem_code",node2);
            if (rem_code=="VIP") flagVIP=true;
            if (rem_code=="STCR") flagSTCR=true;
            if (rem_code=="EXST") flagEXST=true;
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
      };
      if (free<seats_sum)
        throw UserException("Доступных мест осталось %d",free);

      node=NodeAsNode("passengers",reqNode);
      Passengers.Clear();
      //заполним массив для рассадки
      for(node=node->children;node!=NULL;node=node->next)
      {
          node2=node->children;
          if (NodeAsIntegerFast("seats",node2)==0)
          	continue;
          const char *subclass=NodeAsStringFast("subclass",node2);
          TPassenger pas;
          pas.clname=cl;
          if (place_status=="FP"&&
              !NodeIsNULLFast("pax_id",node2)&&
              !NodeIsNULLFast("seat_no",node2)) {
            pas.placeStatus="BR";
            pas.pax_id = NodeAsIntegerFast( "pax_id", node2 );
          }
          else {
            pas.placeStatus=place_status;
            pas.pax_id = 0;
          }
          pas.agent_seat=NodeAsStringFast("seat_no",node2); // crs or hand made
          pas.preseat=NodeAsStringFast("preseat_no",node2);
          pas.countPlace=NodeAsIntegerFast("seats",node2);
          pas.placeRem=NodeAsStringFast("seat_type",node2);
          remNode=GetNodeFast("rems",node2);
          bool flagMCLS=false;
          pas.pers_type = NodeAsStringFast("pers_type",node2);
          bool flagCHIN=pas.pers_type != "ВЗ";
          if (remNode!=NULL)
          {
            for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
            {
              node2=remNode->children;
              const char *rem_code=NodeAsStringFast("rem_code",node2);
              if (airline=="ЮТ" && strcmp(rem_code,"MCLS")==0 ||
                  airline=="ПО" && strcmp(rem_code,"MCLS")==0) flagMCLS=true;
              if ( strcmp(rem_code,"BLND")==0 ||
              	   strcmp(rem_code,"STCR")==0 ||
              	   strcmp(rem_code,"UMNR")==0 ||
              	   strcmp(rem_code,"WCHS")==0 ||
              	   strcmp(rem_code,"MEDA")==0 ) flagCHIN=true;
              if (strcmp(rem_code,"MCLS")==0) continue; //добавим ремарку MCLS позже
              pas.rems.push_back(rem_code);
            };
          };
          //ProgTrace(TRACE5,"airline=%s, subclass=%s, flagMCLS=%d",airline.c_str(),subclass,flagMCLS!=0);
          if (!flagMCLS &&
              (airline=="ЮТ" && strcmp(subclass,"М")==0 ||
               airline=="ПО" && strcmp(subclass,"Ю")==0 ))
            pas.rems.push_back("MCLS");
          if ( flagCHIN || adultwithbaby ) {
          	tst();
          	adultwithbaby = false;
          	pas.rems.push_back( "CHIN" );
          }
          Passengers.Add(pas);
          tst();
      };
      // начитка салона
      Salons.ClName = cl;
      Salons.Read();
      //определим алгоритм рассадки
      int algo=SEATS::GetSeatAlgo(Qry,airline,flt_no,airp_dep);
      //рассадка
      SEATS::SeatsPassengers( &Salons, GetUsePS(), algo );
        /*!!! иногда True - возможна рассажка на забронированные места, когда */
      	/* есть право на регистрацию, статус рейса окончание, есть право сажать на чужие заброн. места */

    };

    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  SELECT pax_grp__seq.nextval INTO :grp_id FROM dual; "
      "  INSERT INTO pax_grp(grp_id,point_dep,point_arv,airp_dep,airp_arv,class, "
      "                      status,excess,hall,bag_refuse,user_id,tid) "
      "  VALUES(:grp_id,:point_dep,:point_arv,:airp_dep,:airp_arv,:class, "
      "         :status,:excess,:hall,0,:user_id,tid__seq.nextval); "
      "END;";
    Qry.CreateVariable("grp_id",otInteger,FNull);
    Qry.CreateVariable("point_dep",otInteger,point_dep);
    Qry.CreateVariable("point_arv",otInteger,point_arv);
    Qry.CreateVariable("airp_dep",otString,airp_dep);
    Qry.CreateVariable("airp_arv",otString,airp_arv);
    Qry.CreateVariable("class",otString,cl);
    Qry.CreateVariable("status",otString,NodeAsString("status",reqNode));
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
    Qry.CreateVariable("hall",otInteger,hall);
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    grp_id=Qry.GetVariableAsInteger("grp_id");
    ReplaceTextChild(reqNode,"grp_id",grp_id);

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

      Qry.Clear();
      Qry.SQLText=
        "BEGIN "
        "  IF :pax_id IS NULL THEN "
        "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
        "  END IF; "
        "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,seat_type,seats,pr_brd, "
        "                  refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm, "
        "                  document,pr_exam,doc_check,subclass,tid) "
        "  VALUES(:pax_id,pax_grp__seq.currval,:surname,:name,:pers_type,:seat_type,:seats,:pr_brd, "
        "         NULL,:reg_no,:ticket_no,:coupon_no,:ticket_rem,:ticket_confirm, "
        "         :document,:pr_exam,0,:subclass,tid__seq.currval); "
        "END;";
      Qry.DeclareVariable("pax_id",otInteger);
      Qry.DeclareVariable("surname",otString);
      Qry.DeclareVariable("name",otString);
      Qry.DeclareVariable("pers_type",otString);
      Qry.DeclareVariable("seat_type",otString);
      Qry.DeclareVariable("seats",otInteger);
      Qry.DeclareVariable("pr_brd",otInteger);
      Qry.DeclareVariable("pr_exam",otInteger);
      Qry.DeclareVariable("reg_no",otInteger);
      Qry.DeclareVariable("ticket_no",otString);
      Qry.DeclareVariable("coupon_no",otInteger);
      Qry.DeclareVariable("ticket_rem",otString);
      Qry.DeclareVariable("ticket_confirm",otInteger);
      Qry.DeclareVariable("document",otString);
      Qry.DeclareVariable("subclass",otString);
      int i=0;
      bool change_agent_seat_no = false;
      bool change_preseat_no = false;
      bool exists_preseats = false;
      bool invalid_seat_no = false;
      for(int k=0;k<=1;k++)
      {
        node=NodeAsNode("passengers",reqNode);
        for(node=node->children;node!=NULL;node=node->next)
        {
          node2=node->children;
          int seats=NodeAsIntegerFast("seats",node2);
          if (seats<=0&&k==0||seats>0&&k==1) continue;
          const char* surname=NodeAsStringFast("surname",node2);
          const char* name=NodeAsStringFast("name",node2);
          const char* pers_type=NodeAsStringFast("pers_type",node2);
          if (!NodeIsNULLFast("pax_id",node2))
            Qry.SetVariable("pax_id",NodeAsIntegerFast("pax_id",node2));
          else
            Qry.SetVariable("pax_id",FNull);
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
          Qry.SetVariable("reg_no",reg_no);
          Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
          if (!NodeIsNULLFast("coupon_no",node2))
            Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
          else
            Qry.SetVariable("coupon_no",FNull);
          if (GetNodeFast("ticket_rem",node2)!=NULL)
          {
            Qry.SetVariable("ticket_rem",NodeAsStringFast("ticket_rem",node2));
            Qry.SetVariable("ticket_confirm",
                            (int)(NodeAsIntegerFast("ticket_confirm",node2)!=0));
          }
          else
          { //потом убрать 31.10.08
            if (!NodeIsNULLFast("ticket_no",node2))
            {
              if (!NodeIsNULLFast("coupon_no",node2))
              {
                Qry.SetVariable("ticket_rem","TKNE");
                Qry.SetVariable("ticket_confirm",(int)true);
              }
              else
              {
                Qry.SetVariable("ticket_rem","TKNA");
                Qry.SetVariable("ticket_confirm",(int)false);
              };
            }
            else
            {
              Qry.SetVariable("ticket_rem",FNull);
              Qry.SetVariable("ticket_confirm",(int)false);
            };
          };
          Qry.SetVariable("document",NodeAsStringFast("document",node2));
          Qry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
          try
          {
            Qry.Execute();
          }
          catch(EOracleError E)
          {
            if (E.Code==1)
              throw UserException((string)"Пассажир "+surname+(*name!=0?" ":"")+name+
                                          " уже зарегистрирован с другой стойки");
            else
              throw;
          };
          int pax_id=Qry.GetVariableAsInteger("pax_id");
          ReplaceTextChild(node,"pax_id",pax_id);

          ostringstream seat_no_str;
          //запись номеров мест
          if (seats>0 && i<Passengers.getCount())
          {
            TPassenger pas = Passengers.Get(i);
            if (pas.seat_no.empty()) throw Exception("SeatsPassengers: empty seat_no");
            	string pas_seat_no;
            	bool pr_found_agent_seat_no = false, pr_found_preseat_no = false;
            	for( std::vector<TSeat>::iterator iseat=pas.seat_no.begin(); iseat!=pas.seat_no.end(); iseat++ ) {
            		if ( !pas.agent_seat.empty() ) { //было из crs или введено агентом
            		  pas_seat_no = denorm_iata_row( iseat->row ) + denorm_iata_line( iseat->line, Salons.getLatSeat() );
            		  if ( pas_seat_no == pas.agent_seat )
            	  		pr_found_agent_seat_no = true;
            		  if ( !pas.preseat.empty() )
            		    exists_preseats = true;
            		  if ( pas.preseat.empty() || pas_seat_no == pas.preseat )
            		  	pr_found_preseat_no = true;
            		}
            		if ( !pas.isValidPlace )
            			invalid_seat_no = true;
              } // end for
              if ( !pas.agent_seat.empty() && !pr_found_agent_seat_no ) // есть место и оно изменилось
              	change_agent_seat_no = true;
              if ( !pas.preseat.empty() && !pr_found_preseat_no ) // есть предварительное место и оно изменилось
              	change_preseat_no = true;

          /*  if (!pas.agent_seat.empty() && pas.agent_seat != pas.placeName) //было из crs или введено агентом, но оно не рассадилось
            {
            	if (!pas.preseat.empty() && pas.preseat == pas.placeName) //если была предв рассадка и она сработала при рассадке
            		showErrorMessage("Пассажиры посажены на предварительно назначенные места");
            	else
                showErrorMessage("Часть запрашиваемых мест недоступны. Пассажиры посажены на свободные");
            }
            else
            	if ( !pas.isValidPlace )
            		showErrorMessage("Пассажиры посажены на запрещенные места");  !!!*/

            vector<TSeatRange> ranges;
            for(vector<TSeat>::iterator iSeat=pas.seat_no.begin();iSeat!=pas.seat_no.end();iSeat++)
            {
              seat_no_str << " "
                          << denorm_iata_row(iSeat->row)
                          << denorm_iata_line(iSeat->line,Salons.getLatSeat());

              TSeatRange range(*iSeat,*iSeat);
              ranges.push_back(range);
            };
            ProgTrace( TRACE5, "ranges.size=%d", ranges.size() );
            //запись в базу
           /* TCompLayerType layer_type;
            switch( grp_status ) {
            	case psCheckin:
            		layer_type = cltCheckin;
            		break;
            	case psTCheckin:
            		layer_type = cltTCheckin;
            		break;
            	case psGoshow:
            		layer_type = cltCheckin;
            		break;
            	case psTransit:
            		layer_type = cltTranzit;
            		break;
            }*/
            SEATS::SaveTripSeatRanges( point_dep, cltCheckin, ranges, pax_id, point_dep, point_arv );
            //SEATS::SaveTripSeatRanges( point_dep, layer_type, ranges, pax_id, point_dep, point_arv );
            //seat_no=pas.seat_no.begin()->
            i++;
          };
          if ( invalid_seat_no )
            showErrorMessage("Пассажиры посажены на запрещенные места");
          else
        		if ( change_agent_seat_no && exists_preseats && !change_preseat_no )
       	  		showErrorMessage("Пассажиры посажены на предварительно назначенные места");
          	else
          	  if ( change_agent_seat_no || change_preseat_no )
            		showErrorMessage("Часть запрашиваемых мест недоступны. Пассажиры посажены на свободные");

          if (seat_no_str.str().empty()) seat_no_str << " нет";


          //запись pax_doc и pax_fqt
          CrsQry.SetVariable("pax_id",pax_id);
          CrsQry.Execute();
          //запись ремарок
          SavePaxRem(node);
          //запись норм
          string normStr=SavePaxNorms(node,norms,pr_unaccomp);
          //запись информации по пассажиру в лог
          TLogMsg msg;
          msg.ev_type=ASTRA::evtPax;
          msg.id1=point_dep;
          msg.id2=reg_no;
          msg.id3=grp_id;
          msg.msg=(string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") зарегистрирован"+
                  ((pr_brd_with_reg && pr_exam_with_brd)?", прошел досмотр":"")+
                  (pr_brd_with_reg?" , прошел посадку":"")+
                  ". "+
                  "П/н: "+airp_arv+", класс: "+cl+", статус: "+EncodePaxStatus(grp_status)+
                  ", место:"+seat_no_str.str()+". Баг.нормы: "+normStr;
          reqInfo->MsgToLog(msg);
          reg_no++;
        };
      };
    }
    else
    {
      //несопровождаемый багаж
      //запись норм
      string normStr=SavePaxNorms(reqNode,norms,pr_unaccomp);
      //запись информации по пассажиру в лог
      TLogMsg msg;
      msg.ev_type=ASTRA::evtPax;
      msg.id1=point_dep;
      msg.id2=0;
      msg.id3=grp_id;
      msg.msg=(string)"Багаж без сопровождения зарегистрирован. "+
              "П/н: "+airp_arv+", статус: "+EncodePaxStatus(grp_status)+
              ". Баг.нормы: "+normStr;
      reqInfo->MsgToLog(msg);
    };
    TLogMsg msg;
    msg.ev_type=ASTRA::evtPax;
    msg.id1=point_dep;
    msg.id2=0;
    msg.id3=grp_id;
    msg.msg=SaveTransfer(reqNode,pr_unaccomp);
    if (!msg.msg.empty()) reqInfo->MsgToLog(msg);
  }
  else
  {
    grp_id=NodeAsInteger(node);
    bool bag_refuse=(GetNode("bag_refuse",reqNode)!=NULL &&
                     !NodeIsNULL("bag_refuse",reqNode));

    Qry.Clear();
    Qry.SQLText=
      "UPDATE pax_grp "
      "SET excess=:excess, "
      "    bag_refuse=NVL(:bag_refuse,bag_refuse), "
      "    tid=tid__seq.nextval "
      "WHERE grp_id=:grp_id AND tid=:tid";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("tid",otInteger,NodeAsInteger("tid",reqNode));
    Qry.CreateVariable("excess",otInteger,NodeAsInteger("excess",reqNode));
    if (GetNode("bag_refuse",reqNode)!=NULL) //потом убрать!!!
      Qry.CreateVariable("bag_refuse",otInteger,(int)bag_refuse);
    else
      Qry.CreateVariable("bag_refuse",otInteger,FNull);

    Qry.Execute();
    if (Qry.RowsProcessed()<=0)
      throw UserException("Изменения в группе производились с другой стойки. Обновите данные");

    //BSM
    if (BSMsend)
      TelegramInterface::LoadBSMContent(grp_id,BSMContentBefore);

    if (!pr_unaccomp)
    {
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
                     "    document=:document, "
                     "    subclass=:subclass, "
                     "    pr_brd=DECODE(:refuse,NULL,pr_brd,NULL), "
                     "    pr_exam=DECODE(:refuse,NULL,pr_exam,0), "
                     "    tid=tid__seq.currval "
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
      PaxQry.DeclareVariable("document",otString);
      PaxQry.DeclareVariable("subclass",otString);

      TQuery LayerQry(&OraSession);
      LayerQry.Clear();
      LayerQry.SQLText=
        "DELETE FROM trip_comp_layers WHERE pax_id=:pax_id AND layer_type=:layer_type";
      LayerQry.DeclareVariable("pax_id",otInteger);
      LayerQry.CreateVariable("layer_type",otString,EncodeCompLayerType(cltCheckin));

      node=GetNode("passengers",reqNode);
      if (node!=NULL)
      {
        for(node=node->children;node!=NULL;node=node->next)
        {
          node2=node->children;
          int pax_id=NodeAsIntegerFast("pax_id",node2);
          const char* surname=NodeAsStringFast("surname",node2);
          const char* name=NodeAsStringFast("name",node2);
          Qry.Clear();
          Qry.SQLText="SELECT refuse,reg_no FROM pax WHERE pax_id=:pax_id";
          Qry.CreateVariable("pax_id",otInteger,pax_id);
          Qry.Execute();
          string old_refuse=Qry.FieldAsString("refuse");
          int reg_no=Qry.FieldAsInteger("reg_no");
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
            PaxQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
            if (!NodeIsNULLFast("coupon_no",node2))
              PaxQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
            else
              PaxQry.SetVariable("coupon_no",FNull);
            if (GetNodeFast("ticket_rem",node2)!=NULL)
            {
              PaxQry.SetVariable("ticket_rem",NodeAsStringFast("ticket_rem",node2));
              PaxQry.SetVariable("ticket_confirm",
                            (int)(NodeAsIntegerFast("ticket_confirm",node2)!=0));
            }
            else
            { //потом убрать 31.10.08
              if (!NodeIsNULLFast("ticket_no",node2))
              {
                if (!NodeIsNULLFast("coupon_no",node2))
                {
                  PaxQry.SetVariable("ticket_rem","TKNE");
                  PaxQry.SetVariable("ticket_confirm",(int)true);
                }
                else
                {
                  PaxQry.SetVariable("ticket_rem","TKNA");
                  PaxQry.SetVariable("ticket_confirm",(int)false);
                };
              }
              else
              {
                PaxQry.SetVariable("ticket_rem",FNull);
                PaxQry.SetVariable("ticket_confirm",(int)false);
              };
            };
            PaxQry.SetVariable("document",NodeAsStringFast("document",node2));
            PaxQry.SetVariable("subclass",NodeAsStringFast("subclass",node2));
            PaxQry.Execute();
            if (PaxQry.RowsProcessed()<=0)
              throw UserException((string)"Изменения по пассажиру "+surname+(*name!=0?" ":"")+name+
                                          " производились с другой стойки. Обновите данные");

            //запись информации по пассажиру в лог
            if (old_refuse!=refuse)
            {
              if (old_refuse=="")
                reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+") разрегистрирован. "+
                                  "Причина отказа в регистрации: "+refuse+". ",
                                  ASTRA::evtPax,point_dep,reg_no,grp_id);
              else
                reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                                  "Изменена причина отказа в регистрации: "+refuse+". ",
                                  ASTRA::evtPax,point_dep,reg_no,grp_id);
            }
            else
            {
              //проверить на PaxUpdatespending!!!
              reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+" ("+pers_type+"). "+
                                "Изменены данные пассажира.",
                                ASTRA::evtPax,point_dep,reg_no,grp_id);
            };
          }
          else
          {
            Qry.Clear();
            Qry.SQLText="UPDATE pax SET tid=tid__seq.currval WHERE pax_id=:pax_id AND tid=:tid";
            Qry.CreateVariable("pax_id",otInteger,pax_id);
            Qry.CreateVariable("tid",otInteger,NodeAsIntegerFast("tid",node2));
            Qry.Execute();
            if (Qry.RowsProcessed()<=0)
              throw UserException((string)"Изменения по пассажиру "+surname+(*name!=0?" ":"")+name+
                                          " производились с другой стойки. Обновите данные");
          };
          CrsQry.SetVariable("pax_id",pax_id);
          CrsQry.Execute();
          //запись ремарок
          SavePaxRem(node);
          //запись норм
          string normStr=SavePaxNorms(node,norms,pr_unaccomp);
          if (normStr!="")
            reqInfo->MsgToLog((string)"Пассажир "+surname+(*name!=0?" ":"")+name+". "+
                              "Баг.нормы: "+normStr,ASTRA::evtPax,point_dep,reg_no,grp_id);
        };
      };
    }
    else
    {
      //запись норм
      string normStr=SavePaxNorms(reqNode,norms,pr_unaccomp);
      if (normStr!="")
        reqInfo->MsgToLog((string)"Багаж без сопровождения. "+
                          "Баг.нормы: "+normStr,
                          ASTRA::evtPax,point_dep,0,grp_id);
      if (bag_refuse)
        reqInfo->MsgToLog("Багаж без сопровождения удален",
                          ASTRA::evtPax,point_dep,0,grp_id);
    };
  };

  CheckInInterface::SaveBag(reqNode);
  CheckInInterface::SavePaidBag(reqNode);
  SaveBagToLog(reqNode);
  SaveTagPacks(reqNode);

  //проверим дублирование билетов
  if (!pr_unaccomp)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT ticket_no,coupon_no FROM pax "
      "WHERE refuse IS NULL AND ticket_no=:ticket_no AND coupon_no=:coupon_no "
      "GROUP BY ticket_no,coupon_no HAVING COUNT(*)>1";
    Qry.DeclareVariable("ticket_no",otString);
    Qry.DeclareVariable("coupon_no",otInteger);
    node=NodeAsNode("passengers",reqNode);
    for(node=node->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (NodeIsNULLFast("ticket_no",node2,true)||
          NodeIsNULLFast("coupon_no",node2,true)) continue;

      Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
      Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
      Qry.Execute();
      if (!Qry.Eof)
        throw UserException("Эл. билет №%s/%s дублируется",
                            NodeAsStringFast("ticket_no",node2),
                            NodeAsStringFast("coupon_no",node2));
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

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  IF :rollback<>0 THEN ROLLBACK TO CHECKIN; END IF; "
    "  UPDATE trip_sets SET overload_alarm=1 WHERE point_id=:point_id; "
    "END;";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.DeclareVariable("rollback",otInteger);

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
      //проверим максимальную загрузку
      try
      {
        if (CheckFltOverload(point_dep))
        {
          Qry.SetVariable("rollback",0);
          Qry.Execute();
        };
      }
      catch(OverloadException &E)
      {
        Qry.SetVariable("rollback",1);
        Qry.Execute();
        showErrorMessage(E.what());
        return;
      };

      Qry.Clear();
      Qry.SQLText=
        "SELECT MAX(DECODE(airline,NULL,0,4)+ "
        "           DECODE(airp,NULL,0,2)) AS priority "
        "FROM cls_grp "
        "WHERE (airline IS NULL OR airline=:airline) AND "
        "      (airp IS NULL OR airp=:airp) AND "
        "      pr_del=0";
      Qry.CreateVariable("airline",otString,airline);
      Qry.CreateVariable("airp",otString,airp_dep);
      Qry.Execute();
      if (Qry.Eof||Qry.FieldIsNULL("priority"))
      {
        ProgError(STDLOG,"Class group not found (airline=%s, airp=%s)",airline.c_str(),airp_dep.c_str());
        throw UserException("На данный рейс регистрация ни в одном из классов не производится");

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
          Qry.CreateVariable("airline",otString,airline);
          Qry.CreateVariable("airp",otString,airp_dep);
          Qry.CreateVariable("priority",otInteger,priority);
          Qry.CreateVariable("subclass",otString,PaxQry.FieldAsString("subclass"));
          Qry.Execute();
          if (!Qry.Eof)
          {
            if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
            else
              if (class_grp!=Qry.FieldAsInteger("id"))
                throw UserException("Невозможно зарегистрировать пассажиров с указанными подклассами одной группой");
            Qry.Next();
            if (!Qry.Eof)
              throw Exception("More than one class group found (airline=%s, airp=%s, subclass=%s)",
                              airline.c_str(),airp_dep.c_str(),PaxQry.FieldAsString("subclass"));
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
        Qry.CreateVariable("airline",otString,airline);
        Qry.CreateVariable("airp",otString,airp_dep);
        Qry.CreateVariable("priority",otInteger,priority);
        Qry.CreateVariable("class",otString,PaxQry.FieldAsString("class"));
        Qry.Execute();
        if (Qry.Eof)
        {
          if (!PaxQry.FieldIsNULL("subclass"))
            throw UserException("На данный рейс регистрация в подклассе %s не производится",
                                PaxQry.FieldAsString("subclass"));
          else
            throw UserException("На данный рейс регистрация в классе %s не производится",
                                PaxQry.FieldAsString("class"));
        };
        if (class_grp==-1) class_grp=Qry.FieldAsInteger("id");
        else
          if (class_grp!=Qry.FieldAsInteger("id"))
            throw UserException("Невозможно зарегистрировать пассажиров с указанными подклассами одной группой");
        Qry.Next();
        if (!Qry.Eof)
          throw Exception("More than one class group found (airline=%s, airp=%s, class=%s)",
                          airline.c_str(),airp_dep.c_str(),PaxQry.FieldAsString("class"));
      };
      Qry.Clear();
      Qry.SQLText="UPDATE pax_grp SET class_grp=:class_grp WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.CreateVariable("class_grp",otInteger,class_grp);
      Qry.Execute();
    };
  }
  else
  {
    //несопровождаемый багаж
    //проверим максимальную загрузку
    try
    {
      if (CheckFltOverload(point_dep))
      {
        Qry.SetVariable("rollback",0);
        Qry.Execute();
      };
    }
    catch(OverloadException &E)
    {
      Qry.SetVariable("rollback",1);
      Qry.Execute();
      showErrorMessage(E.what());
      return;
    };
  };

  //BSM
  if (BSMsend) TelegramInterface::SendBSM(point_dep,grp_id,BSMContentBefore,BSMaddrs);

  //пересчитать данные по группе и отправить на клиент
  LoadPax(ctxt,reqNode,resNode);
  //отправить на клиент счетчики
  readTripCounters(point_dep,resNode);
  //pr_etl_only
  TTripInfo info;
  info.airline=airline;
  info.flt_no=flt_no;
  info.airp=airp_dep;

  node=NewTextChild(resNode,"trip_sets");
  NewTextChild( node, "pr_etl_only", (int)GetTripSets(tsETLOnly,info) );
  NewTextChild( node, "pr_etstatus", pr_etstatus );
};

void CheckInInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node,paxNode;
  int grp_id;
  TQuery Qry(&OraSession);
  TQuery PaxQry(&OraSession);
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
      if (Qry.Eof) throw UserException(1,"Регистрационный номер не найден");
      grp_id=Qry.FieldAsInteger("grp_id");
      Qry.Next();
      if (!Qry.Eof) throw Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
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
        throw UserException("Пассажир не зарегистрирован");
      if (Qry.FieldAsInteger("point_dep")!=point_id)
        throw UserException("Пассажир с другого рейса");
      grp_id=Qry.FieldAsInteger("grp_id");
    };
  }
  else grp_id=NodeAsInteger(node);

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_dep,point_arv,airp_arv,airps.city AS city_arv, "
    "       class,status,hall,bag_refuse,pax_grp.tid "
    "FROM pax_grp,airps "
    "WHERE pax_grp.airp_arv=airps.code AND grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return; //это бывает когда разрегистрация всей группы по ошибке агента
  NewTextChild(resNode,"grp_id",grp_id);
  NewTextChild(resNode,"point_arv",Qry.FieldAsInteger("point_arv"));
  NewTextChild(resNode,"airp_arv",Qry.FieldAsString("airp_arv"));
  NewTextChild(resNode,"city_arv",Qry.FieldAsString("city_arv"));
  NewTextChild(resNode,"class",Qry.FieldAsString("class"));
  NewTextChild(resNode,"status",Qry.FieldAsString("status"));
  NewTextChild(resNode,"hall",Qry.FieldAsInteger("hall"));
  if (Qry.FieldAsInteger("bag_refuse")!=0)
    NewTextChild(resNode,"bag_refuse","А");
  else
    NewTextChild(resNode,"bag_refuse");
  NewTextChild(resNode,"tid",Qry.FieldAsInteger("tid"));
  int point_dep=Qry.FieldAsInteger("point_dep");

  bool pr_unaccomp=Qry.FieldIsNULL("class");
  if (!pr_unaccomp)
  {
    Qry.Clear();
    Qry.SQLText="SELECT pax_id FROM bp_print WHERE pax_id=:pax_id AND pr_print=1 AND rownum=1";
    Qry.DeclareVariable("pax_id",otInteger);
    PaxQry.Clear();
    PaxQry.SQLText=
      "SELECT pax.pax_id,pax.surname,pax.name,pax.pers_type,"
      "       salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,:point_dep,'one',rownum) AS seat_no, "
      "       pax.seat_type, "
      "       pax.seats,pax.refuse,pax.reg_no, "
      "       pax.ticket_no,pax.coupon_no,pax.ticket_rem,pax.ticket_confirm, "
      "       pax.document,pax.subclass,pax.tid, "
      "       crs_pax.pax_id AS crs_pax_id "
      "FROM pax,crs_pax "
      "WHERE pax.pax_id=crs_pax.pax_id(+) AND pax.grp_id=:grp_id ORDER BY pax.reg_no";
    PaxQry.CreateVariable("grp_id",otInteger,grp_id);
    PaxQry.CreateVariable("point_dep",otInteger,point_dep);
    PaxQry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );
    PaxQry.Execute();
    node=NewTextChild(resNode,"passengers");
    for(;!PaxQry.Eof;PaxQry.Next())
    {
      paxNode=NewTextChild(node,"pax");
      int pax_id=PaxQry.FieldAsInteger("pax_id");
      NewTextChild(paxNode,"pax_id",pax_id);
      NewTextChild(paxNode,"surname",PaxQry.FieldAsString("surname"));
      NewTextChild(paxNode,"name",PaxQry.FieldAsString("name"));
      NewTextChild(paxNode,"pers_type",PaxQry.FieldAsString("pers_type"));
      NewTextChild(paxNode,"seat_no",PaxQry.FieldAsString("seat_no"));
      NewTextChild(paxNode,"seat_type",PaxQry.FieldAsString("seat_type"));
      NewTextChild(paxNode,"seats",PaxQry.FieldAsInteger("seats"));
      NewTextChild(paxNode,"refuse",PaxQry.FieldAsString("refuse"));
      NewTextChild(paxNode,"reg_no",PaxQry.FieldAsInteger("reg_no"));
      NewTextChild(paxNode,"ticket_no",PaxQry.FieldAsString("ticket_no"));
      if (!PaxQry.FieldIsNULL("coupon_no"))
        NewTextChild(paxNode,"coupon_no",PaxQry.FieldAsInteger("coupon_no"));
      else
        NewTextChild(paxNode,"coupon_no");
      NewTextChild(paxNode,"ticket_rem",PaxQry.FieldAsString("ticket_rem"));
      NewTextChild(paxNode,"ticket_confirm",
                           (int)(PaxQry.FieldAsInteger("ticket_confirm")!=0));

      NewTextChild(paxNode,"is_tkne",
                   (int)(strcmp(PaxQry.FieldAsString("ticket_rem"),"TKNE")==0)); //потом убрать 31.10.08

      NewTextChild(paxNode,"document",PaxQry.FieldAsString("document"));
      NewTextChild(paxNode,"subclass",PaxQry.FieldAsString("subclass"));
      NewTextChild(paxNode,"tid",PaxQry.FieldAsInteger("tid"));

      NewTextChild(paxNode,"pr_norec",(int)PaxQry.FieldIsNULL("crs_pax_id"));

      Qry.SetVariable("pax_id",pax_id);
      Qry.Execute();
      NewTextChild(paxNode,"pr_bp_print",(int)(!Qry.Eof));
      LoadPaxRem(paxNode);
      LoadPaxNorms(paxNode,pr_unaccomp);
    };
  }
  else
  {
    //несопровождаемый багаж
    LoadPaxNorms(resNode,pr_unaccomp);
  };
  LoadTransfer(resNode);
  CheckInInterface::LoadBag(resNode);
  CheckInInterface::LoadPaidBag(resNode);

  Qry.Close();
  PaxQry.Close();
};



void CheckInInterface::SavePaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr remNode=GetNodeFast("rems",node2);
  if (remNode==NULL) return;

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="DELETE FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES (:pax_id,:rem,:rem_code)";
  RemQry.DeclareVariable("rem",otString);
  RemQry.DeclareVariable("rem_code",otString);
  for(remNode=remNode->children;remNode!=NULL;remNode=remNode->next)
  {
    node2=remNode->children;
    RemQry.SetVariable("rem",NodeAsStringFast("rem_text",node2));
    RemQry.SetVariable("rem_code",NodeAsStringFast("rem_code",node2));
    RemQry.Execute();
  };
  RemQry.Close();
};

void CheckInInterface::LoadPaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  xmlNodePtr node=NewTextChild(paxNode,"rems");
  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="SELECT rem_code,rem FROM pax_rem WHERE pax_id=:pax_id";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.Execute();
  for(;!RemQry.Eof;RemQry.Next())
  {
    xmlNodePtr remNode=NewTextChild(node,"rem");
    NewTextChild(remNode,"rem_text",RemQry.FieldAsString("rem"));
    NewTextChild(remNode,"rem_code",RemQry.FieldAsString("rem_code"));
  };
  RemQry.Close();
};

void CheckInInterface::LoadPaxNorms(xmlNodePtr paxNode, bool pr_unaccomp)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;

  xmlNodePtr node=NewTextChild(paxNode,"norms");
  TQuery NormQry(&OraSession);
  NormQry.Clear();
  if (!pr_unaccomp)
  {
    int pax_id=NodeAsIntegerFast("pax_id",node2);
    NormQry.SQLText=
      "SELECT norm_id,pax_norms.bag_type,norm_type,amount,weight,per_unit "
      "FROM pax_norms,bag_norms "
      "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";
    NormQry.CreateVariable("pax_id",otInteger,pax_id);
  }
  else
  {
    int grp_id=NodeAsIntegerFast("grp_id",node2);
    NormQry.SQLText=
      "SELECT norm_id,grp_norms.bag_type,norm_type,amount,weight,per_unit "
      "FROM grp_norms,bag_norms "
      "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";
    NormQry.CreateVariable("grp_id",otInteger,grp_id);
  };
  NormQry.Execute();
  for(;!NormQry.Eof;NormQry.Next())
  {
    xmlNodePtr normNode=NewTextChild(node,"norm");
    if (!NormQry.FieldIsNULL("bag_type"))
      NewTextChild(normNode,"bag_type",NormQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(normNode,	"bag_type");
    if (!NormQry.FieldIsNULL("norm_id"))
      NewTextChild(normNode,"norm_id",NormQry.FieldAsInteger("norm_id"));
    else
      NewTextChild(normNode,"norm_id");
    NewTextChild(normNode,"norm_type",NormQry.FieldAsString("norm_type"));
    if (!NormQry.FieldIsNULL("amount"))
      NewTextChild(normNode,"amount",NormQry.FieldAsInteger("amount"));
    else
      NewTextChild(normNode,"amount");
    if (!NormQry.FieldIsNULL("weight"))
      NewTextChild(normNode,"weight",NormQry.FieldAsInteger("weight"));
    else
      NewTextChild(normNode,"weight");
    if (!NormQry.FieldIsNULL("per_unit"))
      NewTextChild(normNode,"per_unit",(int)(NormQry.FieldAsInteger("per_unit")!=0));
    else
      NewTextChild(normNode,"per_unit");
  };
  NormQry.Close();
};

string CheckInInterface::SavePaxNorms(xmlNodePtr paxNode, map<int,string> &norms, bool pr_unaccomp)
{
  if (paxNode==NULL) return "";
  xmlNodePtr node2=paxNode->children;

  xmlNodePtr normNode=GetNodeFast("norms",node2);
  if (normNode==NULL) return "";

  TQuery NormQry(&OraSession);
  NormQry.Clear();
  if (!pr_unaccomp)
  {
    int pax_id=NodeAsIntegerFast("pax_id",node2);
    NormQry.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
    NormQry.CreateVariable("pax_id",otInteger,pax_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO pax_norms(pax_id,bag_type,norm_id) VALUES(:pax_id,:bag_type,:norm_id)";
  }
  else
  {
    int grp_id=NodeAsIntegerFast("grp_id",node2);
    NormQry.SQLText="DELETE FROM grp_norms WHERE grp_id=:grp_id";
    NormQry.CreateVariable("grp_id",otInteger,grp_id);
    NormQry.Execute();
    NormQry.SQLText=
      "INSERT INTO grp_norms(grp_id,bag_type,norm_id) VALUES(:grp_id,:bag_type,:norm_id)";
  };
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  string logStr;
  for(normNode=normNode->children;normNode!=NULL;normNode=normNode->next)
  {
    node2=normNode->children;
    //ищем норму в norms
    if (!NodeIsNULLFast("norm_id",node2) && norms.find(NodeAsIntegerFast("norm_id",node2))==norms.end())
    {
        int norm_id=NodeAsIntegerFast("norm_id",node2);
        TQuery Qry(&OraSession);
        Qry.Clear();
        Qry.SQLText="SELECT norm_type,amount,weight,per_unit FROM bag_norms WHERE id=:norm_id";
        Qry.CreateVariable("norm_id",otInteger,norm_id);
        Qry.Execute();
        if (Qry.Eof) throw Exception("Baggage norm not found (norm_id=%d)",norm_id);
        string normStr=lowerc(Qry.FieldAsString("norm_type"));
        if (!Qry.FieldIsNULL("weight"))
        {
          if (!Qry.FieldIsNULL("amount"))
            normStr=normStr+" "+Qry.FieldAsString("amount")+"м"+Qry.FieldAsString("weight");
          else
            normStr=normStr+" "+Qry.FieldAsString("weight");
          if (!Qry.FieldIsNULL("per_unit")&&Qry.FieldAsInteger("per_unit")>0)
            normStr+="кг/м";
          else
            normStr+="кг";
        }
        else
        {
          if (!Qry.FieldIsNULL("amount"))
            normStr=normStr+" "+Qry.FieldAsString("amount")+"м";
        };
        norms[norm_id]=normStr;
        ProgTrace(TRACE5,"Added baggage norm %s",normStr.c_str());
    };
    if (!NodeIsNULLFast("bag_type",node2))
      NormQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
    else
      NormQry.SetVariable("bag_type",FNull);
    if (!NodeIsNULLFast("norm_id",node2))
      NormQry.SetVariable("norm_id",NodeAsIntegerFast("norm_id",node2));
    else
      NormQry.SetVariable("norm_id",FNull);
    NormQry.Execute();
    //составим строчку для записи в журнал операций
    if (!NodeIsNULLFast("bag_type",node2))
    {
      if (!logStr.empty()) logStr+=", ";
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": "+norms[NodeAsIntegerFast("norm_id",node2)];
      else
        logStr=logStr+NodeAsStringFast("bag_type",node2)+": нет";
    }
    else
    {
      if (!logStr.empty()) logStr=", "+logStr;
      if (!NodeIsNULLFast("norm_id",node2))
        logStr=norms[NodeAsIntegerFast("norm_id",node2)]+logStr;
      else
        logStr="нет"+logStr;
    };
  };
  NormQry.Close();
  if (logStr=="") logStr="нет";
  return logStr;
};

void CheckInInterface::ConvertArxTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT arx_pax_grp.point_dep, "
    "       arx_pax_grp.airp_arv AS grp_airp_arv, "
    "       arx_pax_grp.class, "
    "       arx_transfer.grp_id, "
    "       arx_transfer.transfer_num, "
    "       arx_transfer.part_key, "
    "       arx_transfer.airline, "
    "       arx_transfer.suffix, "
    "       arx_transfer.local_date, "
    "       arx_transfer.airp_arv, "
    "       arx_transfer.subclass "
    "FROM arx_pax_grp,arx_transfer "
    "WHERE arx_pax_grp.part_key=arx_transfer.part_key AND "
    "      arx_pax_grp.grp_id=arx_transfer.grp_id "
    "ORDER BY grp_id,transfer_num";
  Qry.Execute();

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText=
    "UPDATE arx_transfer "
    "SET scd=:scd, "
    "    airline=:airline, airline_fmt=:airline_fmt, "
    "    suffix=:suffix, suffix_fmt=:suffix_fmt, "
    "    airp_dep=:airp_dep, airp_dep_fmt=:airp_dep_fmt, "
    "    airp_arv=:airp_arv, airp_arv_fmt=:airp_arv_fmt, "
    "    subclass=:subclass, subclass_fmt=:subclass_fmt, "
    "    pr_final=:pr_final "
    "WHERE grp_id=:grp_id AND transfer_num=:transfer_num";
  UpdQry.DeclareVariable("scd",otDate);
  UpdQry.DeclareVariable("airline",otString);
  UpdQry.DeclareVariable("airline_fmt",otInteger);
  UpdQry.DeclareVariable("suffix",otString);
  UpdQry.DeclareVariable("suffix_fmt",otInteger);
  UpdQry.DeclareVariable("airp_dep",otString);
  UpdQry.DeclareVariable("airp_dep_fmt",otInteger);
  UpdQry.DeclareVariable("airp_arv",otString);
  UpdQry.DeclareVariable("airp_arv_fmt",otInteger);
  UpdQry.DeclareVariable("subclass",otString);
  UpdQry.DeclareVariable("subclass_fmt",otInteger);
  UpdQry.DeclareVariable("pr_final",otInteger);
  UpdQry.DeclareVariable("grp_id",otInteger);
  UpdQry.DeclareVariable("transfer_num",otInteger);

  TQuery FltQry(&OraSession);
  FltQry.Clear();
  FltQry.SQLText=
    "SELECT airp,scd_out FROM arx_points WHERE point_id=:point_id AND part_key=:part_key";
  FltQry.DeclareVariable("point_id",otInteger);
  FltQry.DeclareVariable("part_key",otDate);


  int grp_id=-1,i;
  string cl,str,strh,airp_arv;
  TDateTime local_scd,base_date;
  int fmt;
  if (!Qry.Eof)
  {
    for(;!Qry.Eof;Qry.Next())
    {
      if (grp_id!=Qry.FieldAsInteger("grp_id"))
      {
        if (grp_id!=-1)
        {
          UpdQry.SetVariable("pr_final",1);
          UpdQry.Execute();
        };
        grp_id=Qry.FieldAsInteger("grp_id");
        FltQry.SetVariable("point_id",Qry.FieldAsInteger("point_dep"));
        FltQry.SetVariable("part_key",Qry.FieldAsDateTime("part_key"));
        FltQry.Execute();
        if (FltQry.Eof)
          throw Exception("Flight not found point_id=%d",Qry.FieldAsInteger("point_dep"));

        local_scd=UTCToLocal(FltQry.FieldAsDateTime("scd_out"),
                             AirpTZRegion(FltQry.FieldAsString("airp")));

        base_date=local_scd-1;
        airp_arv=Qry.FieldAsString("grp_airp_arv");
      }
      else
      {
        UpdQry.SetVariable("pr_final",0);
        UpdQry.Execute();
      };

      //авиакомпания
      strh=Qry.FieldAsString("airline");
      str=ElemToElemId(etAirline,strh,fmt);
      if (fmt==0 || fmt==1)
      {
        UpdQry.SetVariable("airline",str);
        UpdQry.SetVariable("airline_fmt",fmt);
      }
      else
      {
        UpdQry.SetVariable("airline",strh);
        UpdQry.SetVariable("airline_fmt",-1);
      };


      if (!Qry.FieldIsNULL("suffix"))
      {
        strh=Qry.FieldAsString("suffix");
        str=ElemToElemId(etSuffix,strh,fmt);
        if (fmt==0 || fmt==1)
        {
          UpdQry.SetVariable("suffix",str);
          UpdQry.SetVariable("suffix_fmt",fmt);
        }
        else
        {
          UpdQry.SetVariable("suffix",strh);
          UpdQry.SetVariable("suffix_fmt",-1);
        };
      }
      else
      {
        UpdQry.SetVariable("suffix",FNull);
        UpdQry.SetVariable("suffix_fmt",FNull);
      };

      i=Qry.FieldAsInteger("local_date");
      try
      {
        local_scd=DayToDate(i,base_date);
        UpdQry.SetVariable("scd",local_scd);
        base_date=local_scd-1; //патамушта можем из Японии лететь в Америку во вчерашний день
      }
      catch(EConvertError &E)
      {
        throw Exception("Wrong local_date grp_id=%d num=%d",
                        Qry.FieldAsInteger("grp_id"),
                        Qry.FieldAsInteger("transfer_num"));
        //UpdQry.SetVariable("scd",FNull);
      };

      //аэропорт вылета
      strh=airp_arv;
      str=ElemToElemId(etAirp,strh,fmt);
      if (fmt==0 || fmt==1)
      {
        UpdQry.SetVariable("airp_dep",str);
        UpdQry.SetVariable("airp_dep_fmt",fmt);
      }
      else
      {
        UpdQry.SetVariable("airp_dep",strh);
        UpdQry.SetVariable("airp_dep_fmt",-1);
      };

      //аэропорт прилета
      airp_arv=Qry.FieldAsString("airp_arv");
      str=ElemToElemId(etAirp,airp_arv,fmt);
      if (fmt==0 || fmt==1)
      {
        UpdQry.SetVariable("airp_arv",str);
        UpdQry.SetVariable("airp_arv_fmt",fmt);
      }
      else
      {
        UpdQry.SetVariable("airp_arv",airp_arv);
        UpdQry.SetVariable("airp_arv_fmt",-1);
      };

      if (!Qry.FieldIsNULL("class"))
      {
        //подкласс
        strh=Qry.FieldAsString("subclass");
        str=ElemToElemId(etSubcls,strh,fmt);
        if (fmt==0 || fmt==1)
        {
          UpdQry.SetVariable("subclass",str);
          UpdQry.SetVariable("subclass_fmt",fmt);
        }
        else
        {
          UpdQry.SetVariable("subclass",strh);
          UpdQry.SetVariable("subclass_fmt",-1);
        };
      }
      else
      {
        UpdQry.SetVariable("subclass",FNull);
        UpdQry.SetVariable("subclass_fmt",FNull);
      };
      UpdQry.SetVariable("grp_id",Qry.FieldAsInteger("grp_id"));
      UpdQry.SetVariable("transfer_num",Qry.FieldAsInteger("transfer_num"));
    };
    UpdQry.SetVariable("pr_final",1);
    UpdQry.Execute();
  };
};

void CheckInInterface::ConvertTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.class, "
    "       drop_transfer.grp_id, "
    "       drop_transfer.airline, "
    "       drop_transfer.flt_no, "
    "       drop_transfer.suffix, "
    "       drop_transfer.local_date, "
    "       drop_transfer.airp_dep, "
    "       drop_transfer.airp_arv, "
    "       drop_transfer.subclass "
    "FROM drop_transfer,pax_grp "
    "WHERE pax_grp.grp_id=drop_transfer.grp_id "
    "ORDER BY grp_id,transfer_num";
  Qry.Execute();
  int grp_id=-1;
  string cl;
  xmlNodePtr trferNode=NULL,segNode;
  if (!Qry.Eof)
  {
    for(;!Qry.Eof;Qry.Next())
    {
      if (grp_id!=Qry.FieldAsInteger("grp_id"))
      {
        if (grp_id!=-1)
        try
        {
          SaveTransfer(resNode,cl.empty());
        }
        catch(UserException &E)
        {
          ProgError(STDLOG,"ConvertTransfer (grp_id=%d): %s",grp_id,E.what());
        };

        grp_id=Qry.FieldAsInteger("grp_id");
        cl=Qry.FieldAsString("class");

        ReplaceTextChild(resNode,"grp_id",grp_id);
        trferNode=GetNode("transfer",resNode);
        if (trferNode!=NULL)
        {
          xmlUnlinkNode(trferNode);
          xmlFreeNode(trferNode);
          trferNode=NULL;
        };
      };
      if (trferNode==NULL)
        trferNode=NewTextChild(resNode,"transfer");
      segNode=NewTextChild(trferNode,"segment");
      NewTextChild(segNode,"airline",Qry.FieldAsString("airline"));
      NewTextChild(segNode,"flt_no",Qry.FieldAsInteger("flt_no"));
      NewTextChild(segNode,"suffix",Qry.FieldAsString("suffix"));
      NewTextChild(segNode,"local_date",Qry.FieldAsInteger("local_date"));
      NewTextChild(segNode,"airp_arv",Qry.FieldAsString("airp_arv"));
      NewTextChild(segNode,"subclass",Qry.FieldAsString("subclass"));
    };
    try
    {
      SaveTransfer(resNode,cl.empty());
    }
    catch(UserException &E)
    {
      ProgError(STDLOG,"ConvertTransfer (grp_id=%d): %s",grp_id,E.what());
    };
  };
};

string CheckInInterface::SaveTransfer(xmlNodePtr grpNode, bool pr_unaccomp)
{
  if (grpNode==NULL) return "";
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr trferNode=GetNodeFast("transfer",node2);
  if (trferNode==NULL) return "";

  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText=
    "BEGIN "
    "  :rows:=ckin.delete_grp_trfer(:grp_id); "
    "END;";
  TrferQry.CreateVariable("rows",otInteger,0);
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();

  trferNode=trferNode->children;
  if (trferNode==NULL)
  {
    if (TrferQry.GetVariableAsInteger("rows")>0)
      return "Отменен трансфер пассажиров и багажа";
    else
      return "";
  };

  TrferQry.Clear();
  TrferQry.SQLText=
    "SELECT airline,flt_no,scd_out,airp AS airp_dep,airp_arv, "
    "       point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points,pax_grp "
    "WHERE points.point_id=pax_grp.point_dep AND grp_id=:grp_id AND points.pr_del>=0";
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();
  if (TrferQry.Eof) throw Exception("Passenger group not found (grp_id=%d)",grp_id);

  string airline_in=TrferQry.FieldAsString("airline");
  int flt_no_in=TrferQry.FieldAsInteger("flt_no");

  TTypeBSendInfo sendInfo;
  sendInfo.airline=TrferQry.FieldAsString("airline");
  sendInfo.flt_no=TrferQry.FieldAsInteger("flt_no");
  sendInfo.airp_dep=TrferQry.FieldAsString("airp_dep");
  sendInfo.first_point=TrferQry.FieldAsInteger("first_point");
  sendInfo.point_num=TrferQry.FieldAsInteger("point_num");

  TTypeBAddrInfo addrInfo;
  addrInfo.airline=sendInfo.airline;
  addrInfo.flt_no=sendInfo.flt_no;
  addrInfo.airp_dep=sendInfo.airp_dep;
  addrInfo.first_point=sendInfo.first_point;
  addrInfo.point_num=sendInfo.point_num;
  addrInfo.airp_trfer=sendInfo.airp_arv;
  addrInfo.pr_lat=true;

  TTripInfo fltInfo;
  fltInfo.airline=sendInfo.airline;
  fltInfo.flt_no=sendInfo.flt_no;
  fltInfo.airp=sendInfo.airp_dep;
  bool without_trfer_set=GetTripSets( tsIgnoreTrferSet, fltInfo );

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

  string airp_arv=TrferQry.FieldAsString("airp_arv");
  TDateTime local_scd=UTCToLocal(TrferQry.FieldAsDateTime("scd_out"),
                                 AirpTZRegion(TrferQry.FieldAsString("airp_dep")));

  TrferQry.Clear();
  TrferQry.SQLText=
    "BEGIN "
    "  BEGIN "
    "    SELECT point_id INTO :point_id_trfer FROM trfer_trips "
    "    WHERE scd=:scd AND airline=:airline AND flt_no=:flt_no AND airp_dep=:airp_dep AND "
    "          (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) for UPDATE; "
    "  EXCEPTION "
    "    WHEN NO_DATA_FOUND THEN "
    "      SELECT id__seq.nextval INTO :point_id_trfer FROM dual; "
    "      INSERT INTO trfer_trips(point_id,airline,flt_no,suffix,scd,airp_dep,point_id_spp) "
    "      VALUES (:point_id_trfer,:airline,:flt_no,:suffix,:scd,:airp_dep,NULL); "
    "  END; "
    "  INSERT INTO transfer(grp_id,transfer_num,point_id_trfer, "
    "    airline_fmt,suffix_fmt,airp_dep_fmt,airp_arv,airp_arv_fmt,subclass,subclass_fmt,pr_final) "
    "  VALUES(:grp_id,:transfer_num,:point_id_trfer, "
    "    :airline_fmt,:suffix_fmt,:airp_dep_fmt,:airp_arv,:airp_arv_fmt,:subclass,:subclass_fmt,:pr_final); "
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
  TrferQry.DeclareVariable("subclass",otString);
  TrferQry.DeclareVariable("subclass_fmt",otInteger);
  TrferQry.DeclareVariable("pr_final",otInteger);
  int i,num=1,fmt;
  string str,strh;
  TDateTime base_date=local_scd-1; //патамушта можем из Японии лететь в Америку во вчерашний день
  ostringstream msg;
  msg << "Оформлен трансфер по маршруту:";
  for(;trferNode!=NULL;trferNode=trferNode->next,num++)
  {
    msg << " -> ";

    node2=trferNode->children;
    TrferQry.SetVariable("transfer_num",num);

    ostringstream flt;
    flt << NodeAsStringFast("airline",node2)
        << setw(3) << setfill('0') << NodeAsIntegerFast("flt_no",node2)
        << NodeAsStringFast("suffix",node2) << "/"
        << setw(2) << setfill('0') << NodeAsIntegerFast("local_date",node2);

    //авиакомпания
    strh=NodeAsStringFast("airline",node2);
    str=ElemToElemId(etAirline,strh,fmt);
    if (!(fmt==0 || fmt==1))
      throw UserException("Неизвестный код а/к %s стыковочного рейса %s",strh.c_str(),flt.str().c_str());
    if (checkType==checkAllSeg ||
        checkType==checkFirstSeg && i==1)
    {
      TAirlinesRow& row=(TAirlinesRow&)base_tables.get("airlines").get_row("code",str);
      if (row.code_lat.empty())
        throw UserException("Не найден лат. код а/к %s стыковочного рейса %s",str.c_str(),flt.str().c_str());
    };
    TrferQry.SetVariable("airline",str);
    TrferQry.SetVariable("airline_fmt",fmt);
    msg << str;

    i=NodeAsIntegerFast("flt_no",node2);
    TrferQry.SetVariable("flt_no",i);
    msg << setw(3) << setfill('0') << i;

    if (!NodeIsNULLFast("suffix",node2))
    {
      strh=NodeAsStringFast("suffix",node2);
      str=ElemToElemId(etSuffix,strh,fmt);
      if (!(fmt==0 || fmt==1))
        throw UserException("Неверно указан суффикс %s стыковочного рейса %s",strh.c_str(),flt.str().c_str());
      TrferQry.SetVariable("suffix",str);
      TrferQry.SetVariable("suffix_fmt",fmt);
    }
    else
    {
      TrferQry.SetVariable("suffix",FNull);
      TrferQry.SetVariable("suffix_fmt",FNull);
    };

    msg << str << "/";

    i=NodeAsIntegerFast("local_date",node2);
    try
    {
      local_scd=DayToDate(i,base_date);
    }
    catch(EConvertError &E)
    {
      throw UserException("Неверно указана локальная дата вылета стыковочного рейса %s",flt.str().c_str());
    };
    TrferQry.SetVariable("scd",local_scd);
    base_date=local_scd-1; //патамушта можем из Японии лететь в Америку во вчерашний день
    msg << setw(2) << setfill('0') << i;

    //аэропорт вылета
    strh=NodeAsStringFast("airp_dep",node2,(char*)airp_arv.c_str());
    str=ElemToElemId(etAirp,strh,fmt);
    if (!(fmt==0 || fmt==1))
      throw UserException("Неизвестный код а/п вылета %s стыковочного рейса %s",strh.c_str(),flt.str().c_str());
    if (checkType==checkAllSeg ||
        checkType==checkFirstSeg && i==1)
    {
      TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",str);
      if (row.code_lat.empty())
        throw UserException("Не найден лат. код а/п вылета %s стыковочного рейса %s",str.c_str(),flt.str().c_str());
    };
    TrferQry.SetVariable("airp_dep",str);
    TrferQry.SetVariable("airp_dep_fmt",fmt);
    msg << ":" << str;

    //аэропорт прилета
    airp_arv=NodeAsStringFast("airp_arv",node2);
    str=ElemToElemId(etAirp,airp_arv,fmt);
    if (!(fmt==0 || fmt==1))
      throw UserException("Неизвестный код а/п прилета %s стыковочного рейса %s",airp_arv.c_str(),flt.str().c_str());
    if (checkType==checkAllSeg ||
        checkType==checkFirstSeg && i==1)
    {
      TAirpsRow& row=(TAirpsRow&)base_tables.get("airps").get_row("code",str);
      if (row.code_lat.empty())
        throw UserException("Не найден лат. код а/п прилета %s стыковочного рейса %s",str.c_str(),flt.str().c_str());
    };
    TrferQry.SetVariable("airp_arv",str);
    TrferQry.SetVariable("airp_arv_fmt",fmt);
    msg << "-" << str;

    if (!pr_unaccomp)
    {
      //подкласс
      strh=NodeAsStringFast("subclass",node2);
      str=ElemToElemId(etSubcls,strh,fmt);
      if (!(fmt==0 || fmt==1))
        throw UserException("Неизвестный код подкласса %s стыковочного рейса %s",strh.c_str(),flt.str().c_str());
      if (checkType==checkAllSeg ||
          checkType==checkFirstSeg && i==1)
      {
        TSubclsRow& row=(TSubclsRow&)base_tables.get("subcls").get_row("code",str);
        if (row.code_lat.empty())
          throw UserException("Не найден лат. код подкласса %s стыковочного рейса %s",str.c_str(),flt.str().c_str());
      };
      TrferQry.SetVariable("subclass",str);
      TrferQry.SetVariable("subclass_fmt",fmt);
      msg << ":" << str;
    }
    else
    {
      TrferQry.SetVariable("subclass",FNull);
      TrferQry.SetVariable("subclass_fmt",FNull);
    };

    TrferQry.SetVariable("pr_final",(int)(trferNode->next==NULL));

    if (!without_trfer_set)
    {
      //проверим разрешено ли оформление трансфера
      if (!CheckTrferPermit(airline_in,
                            flt_no_in,
                            TrferQry.GetVariableAsString("airp_dep"),
                            TrferQry.GetVariableAsString("airline"),
                            TrferQry.GetVariableAsInteger("flt_no")))
        throw UserException("Запрещено оформление трансфера на стыковочный рейс %s",flt.str().c_str());
    };

    TrferQry.Execute();

    airline_in=TrferQry.GetVariableAsString("airline");
    flt_no_in=TrferQry.GetVariableAsInteger("flt_no");

  };

  return msg.str();
};

void CheckInInterface::LoadTransfer(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr node=NewTextChild(grpNode,"transfer");
  TQuery TrferQry(&OraSession);
  TrferQry.Clear();
  TrferQry.SQLText=
    "SELECT airline,airline_fmt,flt_no,suffix,suffix_fmt,scd, "
    "       airp_dep,airp_dep_fmt,airp_arv,airp_arv_fmt,subclass,subclass_fmt "
    "FROM transfer,trfer_trips "
    "WHERE transfer.point_id_trfer=trfer_trips.point_id AND "
    "      grp_id=:grp_id AND transfer_num>0 "
    "ORDER BY transfer_num";
  TrferQry.CreateVariable("grp_id",otInteger,grp_id);
  TrferQry.Execute();
  int iDay,iMonth,iYear;
  for(;!TrferQry.Eof;TrferQry.Next())
  {
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"airline",
                 ElemIdToElem(etAirline,
                              TrferQry.FieldAsString("airline"),
                              TrferQry.FieldAsInteger("airline_fmt")));
    NewTextChild(trferNode,"flt_no",TrferQry.FieldAsInteger("flt_no"));

    if (!TrferQry.FieldIsNULL("suffix"))
      NewTextChild(trferNode,"suffix",
                   ElemIdToElem(etSuffix,
                                TrferQry.FieldAsString("suffix"),
                                TrferQry.FieldAsInteger("suffix_fmt")));
    else
      NewTextChild(trferNode,"suffix");

    //дата
    DecodeDate(TrferQry.FieldAsDateTime("scd"),iYear,iMonth,iDay);
    NewTextChild(trferNode,"local_date",iDay);

    NewTextChild(trferNode,"airp_dep",
                 ElemIdToElem(etAirp,
                              TrferQry.FieldAsString("airp_dep"),
                              TrferQry.FieldAsInteger("airp_dep_fmt")));
    NewTextChild(trferNode,"airp_arv",
                 ElemIdToElem(etAirp,
                              TrferQry.FieldAsString("airp_arv"),
                              TrferQry.FieldAsInteger("airp_arv_fmt")));

    if (!TrferQry.FieldIsNULL("subclass"))
      NewTextChild(trferNode,"subclass",
                   ElemIdToElem(etSubcls,
                                TrferQry.FieldAsString("subclass"),
                                TrferQry.FieldAsInteger("subclass_fmt")));
    else
      NewTextChild(trferNode,"subclass");

  };
  TrferQry.Close();
};

void CheckInInterface::SaveBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node,node2;
  int point_id=NodeAsInteger("point_dep",grpNode);
  int grp_id=NodeAsInteger("grp_id",grpNode);

  xmlNodePtr valueBagNode=GetNode("value_bags",grpNode);
  xmlNodePtr bagNode=GetNode("bags",grpNode);
  xmlNodePtr tagNode=GetNode("tags",grpNode);

  TReqInfo *reqInfo = TReqInfo::Instance();

  if ( reqInfo->screen.name == "AIR.EXE" )
  {

    if (bagNode==NULL&&tagNode==NULL) return;
    //подсчитаем кол-во багажа и баг. бирок
    int bagAmount=0,tagCount=0;
    if (bagNode!=NULL)
      for(node=bagNode->children;node!=NULL;node=node->next)
      {
        node2=node->children;
        if (NodeAsIntegerFast("pr_cabin",node2)==0) bagAmount+=NodeAsIntegerFast("amount",node2);
      };
    if (tagNode!=NULL)
      for(node=tagNode->children;node!=NULL;node=node->next,tagCount++);

    ProgTrace(TRACE5,"bagAmount=%d tagCount=%d",bagAmount,tagCount);
    bool pr_tag_print=NodeAsInteger("@pr_print",tagNode)!=0;
    TQuery Qry(&OraSession);
    if (bagAmount!=tagCount)
    {
      if (pr_tag_print && tagCount<bagAmount )
      {
        Qry.Clear();
        Qry.SQLText=
          "SELECT tag_type FROM trip_bt WHERE point_id=:point_id";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();
        if (Qry.Eof) throw UserException("На рейс не назначен бланк печатаемой багажной бирки");
        string tag_type = Qry.FieldAsString("tag_type");
        //получим номера печатаемых бирок
        Qry.Clear();
        Qry.SQLText=
          "DECLARE "
          "  vairline airlines.code%TYPE; "
          "  vaircode airlines.aircode%TYPE; "
          "BEGIN "
          "  BEGIN "
          "    SELECT airline INTO vairline FROM points WHERE point_id=:point_id AND pr_del>=0; "
          "    SELECT aircode INTO vaircode FROM airlines WHERE code=vairline; "
          "  EXCEPTION "
          "    WHEN OTHERS THEN vaircode:=NULL; "
          "  END; "
          "  ckin.get__tag_no(:desk,vaircode,:tag_count,:first_no,:last_no); "
          "END;";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.CreateVariable("desk",otString,reqInfo->desk.code);
        Qry.CreateVariable("tag_count",otInteger,bagAmount-tagCount);
        Qry.DeclareVariable("first_no",otInteger);
        Qry.DeclareVariable("last_no",otInteger);
        Qry.Execute();
        int first_no=Qry.GetVariableAsInteger("first_no");
        int last_no=Qry.GetVariableAsInteger("last_no");
        if (tagNode==NULL) tagNode=NewTextChild(grpNode,"tags");
        if ((first_no/1000)==(last_no/1000))
        {
          //первый и последний номер из одного диапазона
          for(int i=first_no;i<=last_no;i++,tagCount++)
          {
            node=NewTextChild(tagNode,"tag");
            NewTextChild(node,"num",tagCount+1);
            NewTextChild(node,"tag_type",tag_type);
            NewTextChild(node,"no",i);
            NewTextChild(node,"color");
            NewTextChild(node,"bag_num");
            NewTextChild(node,"pr_print",(int)false);
          };
        }
        else
        {
          int j;
          j=(first_no/1000)*1000+999;
          for(int i=first_no;i<=j;i++,tagCount++)
          {
            node=NewTextChild(tagNode,"tag");
            NewTextChild(node,"num",tagCount+1);
            NewTextChild(node,"tag_type",tag_type);
            NewTextChild(node,"no",i);
            NewTextChild(node,"color");
            NewTextChild(node,"bag_num");
            NewTextChild(node,"pr_print",(int)false);
          }
          j=(last_no/1000)*1000;
          if ((j%1000000)==0) j++;
          for(int i=j;i<=last_no;i++,tagCount++)
          {
            node=NewTextChild(tagNode,"tag");
            NewTextChild(node,"num",tagCount+1);
            NewTextChild(node,"tag_type",tag_type);
            NewTextChild(node,"no",i);
            NewTextChild(node,"color");
            NewTextChild(node,"bag_num");
            NewTextChild(node,"pr_print",(int)false);
          };
        };
        xmlNodePtr bNode,tNode;
        int bag_num,bag_amount;

        //пробуем привязать к багажу
        if (bagNode!=NULL && tagNode!=NULL)
        {
          tNode=tagNode->last;
          for(bNode=bagNode->last;bNode!=NULL;bNode=bNode->prev)
          {
            if (tNode==NULL) break;
            node2=bNode->children;
            bag_num=NodeAsIntegerFast("num",node2);
            bag_amount=NodeAsIntegerFast("amount",node2);
            if (NodeAsIntegerFast("pr_cabin",node2)!=0) continue;

            //проверим чтобы на этот багаж не было назначено ни одной бирки
            for(node=tagNode->children;node!=NULL&&node!=tNode->next;node=node->next)
            {
              node2=node->children;
              if (!NodeIsNULLFast("bag_num",node2) &&
                  bag_num==NodeAsIntegerFast("bag_num",node2)) break;
            };
            if (node!=NULL&&node!=tNode->next) break; //выйдем, если на текущий багаж ссылается бирка

            int k=0;
            for(;k<bag_amount;k++)
            {
              if (tNode==NULL) break;
              node2=tNode->children;
              if (/*NodeAsIntegerFast("printable",node2)==0 ||*/ !NodeIsNULLFast("bag_num",node2)) break;
              ReplaceTextChild(tNode,"bag_num",bag_num);
              tNode=tNode->prev;
            };
            if (k<bag_amount) break; //выйдем, если текущая бирка ссылается на багаж или она не печатаемая
          };
        };
      }
      else throw UserException(1,"Кол-во бирок и мест багажа не совпадает");
    };
  };

  TQuery BagQry(&OraSession);
  if (valueBagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM value_bag WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax) "
      "VALUES(:grp_id,:num,:value,:value_cur,:tax_id,:tax)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("value",otFloat);
    BagQry.DeclareVariable("value_cur",otString);
    BagQry.DeclareVariable("tax_id",otInteger);
    BagQry.DeclareVariable("tax",otFloat);
    for(node=valueBagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      BagQry.SetVariable("value",NodeAsFloatFast("value",node2));
      BagQry.SetVariable("value_cur",NodeAsStringFast("value_cur",node2));
      if (!NodeIsNULLFast("tax_id",node2))
      {
        BagQry.SetVariable("tax_id",NodeAsIntegerFast("tax_id",node2));
        BagQry.SetVariable("tax",NodeAsFloatFast("tax",node2));
      }
      else
      {
        BagQry.SetVariable("tax_id",FNull);
        BagQry.SetVariable("tax",FNull);
      };
      BagQry.Execute();
    };
  };
  if (bagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag2 WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag2 (grp_id,num,bag_type,pr_cabin,amount,weight,value_bag_num,pr_liab_limit) "
      "VALUES (:grp_id,:num,:bag_type,:pr_cabin,:amount,:weight,:value_bag_num,:pr_liab_limit)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("bag_type",otInteger);
    BagQry.DeclareVariable("pr_cabin",otInteger);
    BagQry.DeclareVariable("amount",otInteger);
    BagQry.DeclareVariable("weight",otInteger);
    BagQry.DeclareVariable("value_bag_num",otInteger);
    BagQry.DeclareVariable("pr_liab_limit",otInteger);
    for(node=bagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      if (!NodeIsNULLFast("bag_type",node2))
        BagQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
      else
        BagQry.SetVariable("bag_type",FNull);
      BagQry.SetVariable("pr_cabin",NodeAsIntegerFast("pr_cabin",node2));
      BagQry.SetVariable("amount",NodeAsIntegerFast("amount",node2));
      BagQry.SetVariable("weight",NodeAsIntegerFast("weight",node2));
      if (!NodeIsNULLFast("value_bag_num",node2))
        BagQry.SetVariable("value_bag_num",NodeAsIntegerFast("value_bag_num",node2));
      else
        BagQry.SetVariable("value_bag_num",FNull);
      if (GetNodeFast("pr_liab_limit",node2)!=NULL)  //потом убрать!!!
        BagQry.SetVariable("pr_liab_limit",NodeAsIntegerFast("pr_liab_limit",node2));
      else
        BagQry.SetVariable("pr_liab_limit",(int)0);
      BagQry.Execute();
    };
  };
  if (tagNode!=NULL)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag_tags WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag_tags(grp_id,num,tag_type,no,color,bag_num,pr_print) "
      "VALUES (:grp_id,:num,:tag_type,:no,:color,:bag_num,:pr_print)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("tag_type",otString);
    BagQry.DeclareVariable("no",otFloat);
    BagQry.DeclareVariable("color",otString);
    BagQry.DeclareVariable("bag_num",otInteger);
    BagQry.DeclareVariable("pr_print",otInteger);
    for(node=tagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      const char* tag_type = NodeAsStringFast("tag_type",node2);
      const char* color = NodeAsStringFast("color",node2);
      double no = NodeAsFloatFast("no",node2);
      BagQry.SetVariable("num",NodeAsIntegerFast("num",node2));
      BagQry.SetVariable("tag_type",tag_type);
      BagQry.SetVariable("no",no);
      BagQry.SetVariable("color",color);
      if (!NodeIsNULLFast("bag_num",node2))
        BagQry.SetVariable("bag_num",NodeAsIntegerFast("bag_num",node2));
      else
        BagQry.SetVariable("bag_num",FNull);
      BagQry.SetVariable("pr_print",NodeAsIntegerFast("pr_print",node2));
      try
      {
        BagQry.Execute();
      }
      catch(EOracleError E)
      {
        if (E.Code==1)
          throw UserException("Бирка %s %s%010.f уже зарегистрирована.",tag_type,color,no);
        else
          throw;
      };
    };
  };
};

void CheckInInterface::LoadBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  TQuery BagQry(&OraSession);

  xmlNodePtr node=NewTextChild(grpNode,"value_bags");
  BagQry.Clear();
  BagQry.SQLText="SELECT num,value,value_cur,tax_id,tax FROM value_bag "
                 "WHERE grp_id=:grp_id ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr valueBagNode=NewTextChild(node,"value_bag");
    NewTextChild(valueBagNode,"num",BagQry.FieldAsInteger("num"));
    NewTextChild(valueBagNode,"value",BagQry.FieldAsFloat("value"));
    NewTextChild(valueBagNode,"value_cur",BagQry.FieldAsString("value_cur"));
    if (!BagQry.FieldIsNULL("tax_id"))
    {
      NewTextChild(valueBagNode,"tax_id",BagQry.FieldAsInteger("tax_id"));
      NewTextChild(valueBagNode,"tax",BagQry.FieldAsFloat("tax"));
    }
    else
    {
      NewTextChild(valueBagNode,"tax_id");
      NewTextChild(valueBagNode,"tax");
    };
  };
  node=NewTextChild(grpNode,"bags");
  BagQry.Clear();
  BagQry.SQLText="SELECT num,bag_type,pr_cabin,amount,weight,value_bag_num,pr_liab_limit "
                 "FROM bag2 WHERE grp_id=:grp_id ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr bagNode=NewTextChild(node,"bag");
    NewTextChild(bagNode,"num",BagQry.FieldAsInteger("num"));
    if (!BagQry.FieldIsNULL("bag_type"))
      NewTextChild(bagNode,"bag_type",BagQry.FieldAsInteger("bag_type"));
    else
      NewTextChild(bagNode,"bag_type");
    NewTextChild(bagNode,"pr_cabin",(int)(BagQry.FieldAsInteger("pr_cabin")!=0));
    NewTextChild(bagNode,"amount",BagQry.FieldAsInteger("amount"));
    NewTextChild(bagNode,"weight",BagQry.FieldAsInteger("weight"));
    if (!BagQry.FieldIsNULL("value_bag_num"))
      NewTextChild(bagNode,"value_bag_num",BagQry.FieldAsInteger("value_bag_num"));
    else
      NewTextChild(bagNode,"value_bag_num");
    NewTextChild(bagNode,"pr_liab_limit",(int)(BagQry.FieldAsInteger("pr_liab_limit")!=0));
  };
  node=NewTextChild(grpNode,"tags");
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT num,tag_type,no_len,no,color,bag_num,printable,pr_print "
    "FROM bag_tags,tag_types "
    "WHERE bag_tags.tag_type=tag_types.code AND grp_id=:grp_id "
    "ORDER BY num";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr tagNode=NewTextChild(node,"tag");
    NewTextChild(tagNode,"num",BagQry.FieldAsInteger("num"));
    NewTextChild(tagNode,"tag_type",BagQry.FieldAsString("tag_type"));
    NewTextChild(tagNode,"no_len",BagQry.FieldAsInteger("no_len"));
    NewTextChild(tagNode,"no",BagQry.FieldAsFloat("no"));
    NewTextChild(tagNode,"color",BagQry.FieldAsString("color"));
    if (!BagQry.FieldIsNULL("bag_num"))
      NewTextChild(tagNode,"bag_num",BagQry.FieldAsInteger("bag_num"));
    else
      NewTextChild(tagNode,"bag_num");
    NewTextChild(tagNode,"printable",(int)(BagQry.FieldAsInteger("printable")!=0));
    NewTextChild(tagNode,"pr_print",(int)(BagQry.FieldAsInteger("pr_print")!=0));
  };
  BagQry.Close();
};

void CheckInInterface::SavePaidBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node,node2;
  int grp_id=NodeAsInteger("grp_id",grpNode);
  xmlNodePtr paidBagNode=GetNode("paid_bags",grpNode);
  if (paidBagNode!=NULL)
  {
    TQuery BagQry(&OraSession);
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id) "
      "VALUES(:grp_id,:bag_type,:weight,:rate_id)";
    BagQry.DeclareVariable("bag_type",otInteger);
    BagQry.DeclareVariable("weight",otInteger);
    BagQry.DeclareVariable("rate_id",otInteger);
    for(node=paidBagNode->children;node!=NULL;node=node->next)
    {
      node2=node->children;
      if (!NodeIsNULLFast("bag_type",node2))
        BagQry.SetVariable("bag_type",NodeAsIntegerFast("bag_type",node2));
      else
        BagQry.SetVariable("bag_type",FNull);
      BagQry.SetVariable("weight",NodeAsIntegerFast("weight",node2));
      if (!NodeIsNULLFast("rate_id",node2))
        BagQry.SetVariable("rate_id",NodeAsIntegerFast("rate_id",node2));
      else
        BagQry.SetVariable("rate_id",FNull);
      BagQry.Execute();
    };
    BagQry.Close();
  };
};

void CheckInInterface::LoadPaidBag(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  xmlNodePtr node2=grpNode->children;
  int grp_id=NodeAsIntegerFast("grp_id",node2);

  xmlNodePtr node=NewTextChild(grpNode,"paid_bags");
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT NVL(paid_bag.bag_type,-1) AS bag_type,paid_bag.weight, "
    "       NVL(rate_id,-1) AS rate_id,rate,rate_cur "
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
    }
    else
    {
      NewTextChild(paidBagNode,"rate_id");
      NewTextChild(paidBagNode,"rate");
      NewTextChild(paidBagNode,"rate_cur");
    };
  };
  BagQry.Close();
};

//запись багажа в лог
void CheckInInterface::SaveBagToLog(xmlNodePtr grpNode)
{
  if (grpNode==NULL) return;
  int point_id=NodeAsInteger("point_dep",grpNode);
  int grp_id=NodeAsInteger("grp_id",grpNode);
  xmlNodePtr paidBagNode=GetNode("paid_bags",grpNode);
  xmlNodePtr bagNode=GetNode("bags",grpNode);
  xmlNodePtr tagNode=GetNode("tags",grpNode);
  TReqInfo* reqInfo = TReqInfo::Instance();
  TLogMsg msg;
  msg.ev_type=ASTRA::evtPax;
  msg.id1=point_id;
  msg.id2=0;
  msg.id3=grp_id;
  TQuery Qry(&OraSession);
  if (bagNode!=NULL || tagNode!=NULL)
  {
    //строка по общему кол-ву багажа
    Qry.Clear();
    Qry.SQLText=
      "SELECT "
      "       NVL(ckin.get_bagAmount(grp_id,NULL),0) AS bagAmount, "
      "       NVL(ckin.get_bagWeight(grp_id,NULL),0) AS bagWeight, "
      "       NVL(ckin.get_rkAmount(grp_id,NULL),0) AS rkAmount, "
      "       NVL(ckin.get_rkWeight(grp_id,NULL),0) AS rkWeight, "
      "       ckin.get_birks(grp_id,NULL) AS tags, "
      "       excess "
      "FROM pax_grp where grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      ostringstream msgh;
      msgh << "Багаж: " << Qry.FieldAsInteger("bagAmount") << "/" << Qry.FieldAsInteger("bagWeight") << ", "
           << "р/кладь: " << Qry.FieldAsInteger("rkAmount") << "/" << Qry.FieldAsInteger("rkWeight") << ". ";
      if (Qry.FieldAsInteger("excess")!=0)
        msgh << "Опл. вес: " << Qry.FieldAsInteger("excess") << " кг. ";
      if (!Qry.FieldIsNULL("tags"))
        msgh << "Бирки: " << Qry.FieldAsString("tags") << ". ";
      msg.msg=msgh.str();
      reqInfo->MsgToLog(msg);
    };
  };
  if (bagNode!=NULL || paidBagNode!=NULL)
  {
    //строка по типам багажа и оплачиваемому багажу
    Qry.Clear();
    Qry.SQLText=
      "SELECT LPAD(paid_bag.bag_type,2,'0' ) AS bag_type, "
      "       MAX(paid_bag.weight) AS paid_weight, "
      "       NVL(SUM(bag2.amount),0) AS bag_amount, "
      "       NVL(SUM(bag2.weight),0) AS bag_weight "
      "FROM paid_bag,bag2 "
      "WHERE paid_bag.grp_id=bag2.grp_id(+) AND  "
      "      NVL(paid_bag.bag_type,-1)=NVL(bag2.bag_type(+),-1) AND  "
      "      paid_bag.grp_id=:grp_id "
      "GROUP BY paid_bag.bag_type "
      "ORDER BY DECODE(paid_bag.bag_type,NULL,0,1),paid_bag.bag_type ";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    ostringstream msgh1,msgh2;
    for(;!Qry.Eof;Qry.Next())
    {
      if (Qry.FieldAsInteger("bag_amount")==0 &&
          Qry.FieldAsInteger("bag_weight")==0 &&
          Qry.FieldAsInteger("paid_weight")==0) continue;
      if (Qry.FieldIsNULL("bag_type"))
      {
        msgh1 << ", "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      }
      else
      {
        msgh2 << ", "
              << Qry.FieldAsInteger("bag_type") << ": "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      };
    };
    if (!msgh2.str().empty())
    {
      msgh1 << msgh2.str();
      msg.msg="Багаж по типам (мест/вес/опл): "+msgh1.str().substr(2);
      reqInfo->MsgToLog(msg);

    };
  };
  Qry.Close();
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

void CheckInInterface::TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
 // TReqInfo* reqInfo = TReqInfo::Instance();
  NewTextChild(resNode,"LocalDateTime");
  NewTextChild(resNode,"UTCDateTime");
  if (!NodeIsNULL("UTCDateTime",reqNode))
  {
    try
    {
      TDateTime ud = NodeAsDateTime("UTCDateTime",reqNode);
      ud = NowLocal();
      //ud = UTCToLocal(ud,reqInfo->desk.tz_region);
      ReplaceTextChild(resNode,"LocalDateTime",DateTimeToStr(ud,"dd.mm.yyyy hh:nn:ss"));
    }
    catch(std::logic_error e)
    {
      showErrorMessage(e.what());
    };
  };

  if (!NodeIsNULL("LocalDateTime",reqNode))
  {
    try
    {
      TDateTime ld = NodeAsDateTime("LocalDateTime",reqNode);
      ld = NowUTC();
      //ld = LocalToUTC(ld,reqInfo->desk.tz_region);
      ReplaceTextChild(resNode,"UTCDateTime",DateTimeToStr(ld,"dd.mm.yyyy hh:nn:ss"));
    }
    catch(std::logic_error e)
    {
      showErrorMessage(e.what());
    };
  };
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
    "SELECT airp, point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
  int first_point=Qry.FieldAsInteger("first_point");
  int point_num=Qry.FieldAsInteger("point_num");
  string airp_dep=Qry.FieldAsString("airp");

  Qry.Clear();
  Qry.SQLText =
    "SELECT points.point_id, "
    "       airps.code AS airp_code, "
    "       airps.name AS airp_name, "
    "       cities.code AS city_code, "
    "       cities.name AS city_name "
    "FROM points,airps,cities "
    "WHERE points.first_point=:first_point AND points.point_num>:point_num AND points.pr_del=0 AND "
    "      points.airp=airps.code AND airps.city=cities.code "
    "ORDER BY point_num";
  Qry.CreateVariable("first_point",otInteger,first_point);
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(;!Qry.Eof;Qry.Next())
  {
    //проверим на дублирование кодов аэропортов в рамках одного рейса
    for(i=airps.begin();i!=airps.end();i++)
      if (*i==Qry.FieldAsString( "airp_code" )) break;
    if (i!=airps.end()) continue;

    itemNode = NewTextChild( node, "airp" );
    NewTextChild( itemNode, "point_id", Qry.FieldAsInteger( "point_id" ) );
    NewTextChild( itemNode, "airp_code", Qry.FieldAsString( "airp_code" ) );
    NewTextChild( itemNode, "airp_name", Qry.FieldAsString( "airp_name" ) );
    NewTextChild( itemNode, "city_code", Qry.FieldAsString( "city_code" ) );
    NewTextChild( itemNode, "city_name", Qry.FieldAsString( "city_name" ) );
    airps.push_back(Qry.FieldAsString( "airp_code" ));
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT class AS class_code, "
    "       name AS class_name, "
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
    NewTextChild( itemNode, "code", Qry.FieldAsString( "class_code" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "class_name" ) );
    NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT name AS gate_name "
    "FROM stations,trip_stations "
    "WHERE stations.desk=trip_stations.desk AND "
    "      stations.work_mode=trip_stations.work_mode AND "
    "      trip_stations.point_id=:point_id AND "
    "      trip_stations.work_mode='П' ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "gates" );
  for(;!Qry.Eof;Qry.Next())
  {
    NewTextChild( node, "gate_name", Qry.FieldAsString( "gate_name" ) );
  };

  Qry.Clear();
  Qry.SQLText =
    "SELECT id,name FROM halls2 WHERE airp=:airp_dep";
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.Execute();
  node = NewTextChild( tripdataNode, "halls" );
  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "hall" );
    NewTextChild( itemNode, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( itemNode, "name", Qry.FieldAsString( "name" ) );
  };
}

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

}




