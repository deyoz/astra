#include "checkin.h"
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
#include "typeb_utils.h"
#include "misc.h"
#include "astra_misc.h"
#include "base_tables.h"
#include "convert.h"
#include "tripinfo.h"
#include "aodb.h"
#include "salons.h"
#include "seats.h"
#include "tlg/tlg_parser.h"
#include "docs.h"
#include "stat.h"
#include "etick.h"
#include "events.h"
#include "term_version.h"
#include "baggage.h"
#include "baggage_calc.h"
#include "passenger.h"
#include "remarks.h"
#include "alarms.h"
#include "sopp.h"
#include "points.h"
#include "pers_weights.h"
#include "rozysk.h"
#include "flt_binding.h"
#include "apis.h"
#include "qrys.h"
#include "emdoc.h"
#include "jxtlib/jxt_cont.h"
#include "apis_utils.h"
#include "astra_callbacks.h"
#include "apps_interaction.h"
#include "astra_elem_utils.h"
#include "baggage_wt.h"
#include "payment_base.h"
#include "rfisc.h"
#include "rfisc_sirena.h"
#include "ffp_sirena.h"
#include "annul_bt.h"
#include "tlg/AgentWaitsForRemote.h"
#include <boost/algorithm/string.hpp>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace AstraLocale;

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

class TInquiryPrefix
{
  public:
    enum Enum
    {
      NoRec,
      Crew,
      ExtraCrew,
      DeadHeadCrew,
      MiscOperStaff,
      Empty,
      CrewOldStyle
    };

    typedef std::list< std::pair<Enum, std::string> > Pairs;

    static const Pairs& pairs()
    {
      static Pairs l;
      if (l.empty())
      {
        l.push_back(std::make_pair(NoRec,         "-"));
        l.push_back(std::make_pair(Crew,          "CREW-"));
        l.push_back(std::make_pair(ExtraCrew,     "XXXCREW-"));
        l.push_back(std::make_pair(DeadHeadCrew,  "XXXDHC-"));
        l.push_back(std::make_pair(MiscOperStaff, "XXXMOS-"));
        l.push_back(std::make_pair(Empty,         ""));
        l.push_back(std::make_pair(CrewOldStyle,  "CREW"));
      }
      return l;
    }
};

class TInquiryPrefixes : public ASTRA::PairList<TInquiryPrefix::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TInquiryPrefixes"; }
  public:
    TInquiryPrefixes() : ASTRA::PairList<TInquiryPrefix::Enum, std::string>(TInquiryPrefix::pairs(),
                                                                            boost::none,
                                                                            boost::none) {}
};

const TInquiryPrefixes& InquiryPrefixes()
{
  static TInquiryPrefixes inquiryPrefixes;
  return inquiryPrefixes;
}

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
  TPaxStatus status;
  TInquiryPrefix::Enum prefix;
  vector<TInquiryFamily> fams;
  bool digCkin,large;
  void Clear()
  {
    status=psCheckin;
    prefix=TInquiryPrefix::Empty;
    fams.clear();
    digCkin=false;
    large=false;
  };
  TInquiryGroup()
  {
    Clear();
  };
};

void ParseInquiryStr(const string &query, const TPaxStatus status, TInquiryGroup &grp)
{
  TInquiryFamily fam;
  string strh;

  grp.Clear();
  grp.status=status;
  try
  {
    char c;
    int state=1;
    int pos=1;
    string::const_iterator i=query.begin();
    for(TInquiryPrefix::Pairs::const_iterator p=TInquiryPrefix::pairs().begin(); p!=TInquiryPrefix::pairs().end(); ++p)
    {
      if (p->second.empty()) continue;
      if (query.substr(0,p->second.size())!=p->second) continue;
      grp.prefix=p->first;
      if (grp.prefix==TInquiryPrefix::ExtraCrew ||
          grp.prefix==TInquiryPrefix::DeadHeadCrew ||
          grp.prefix==TInquiryPrefix::MiscOperStaff)
      {
        if (!TReqInfo::Instance()->desk.compatible(XXXCREW_VERSION))
          throw UserException(100, "MSG.REQUEST_ERROR.NOT_SUPPORTED_BY_THE_TERM_VERSION",
                                   LParams() << LParam("request", InquiryPrefixes().encode(grp.prefix)));
        if (grp.status==psCrew)
          throw UserException(100, "MSG.REQUEST_ERROR.NOT_APPLICABLE_FOR_CHECKIN_STATUS",
                                   LParams() << LParam("request", InquiryPrefixes().encode(grp.prefix))
                                             << LParam("status", ElemIdToClientElem(etGrpStatusType, EncodePaxStatus(grp.status), efmtNameLong)));
      }
      state=2;
      pos+=p->second.size();
      i+=p->second.size();
      break;
    };
    for(;;)
    {
      if (i!=query.end()) c=*i; else c='\0';
      switch (state)
      {
        case 1:
          switch (c)
          {
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
          //ࠧ����� surname �� surname � name
            i->name=SeparateNames(i->surname);
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
  TInquiryPrefix::Enum prefix;
  int n[NoPerson];
  int nPaxWithPlace,nPax;
  vector<TInquiryFamilySummary> fams;
  int persCountFmt;

  void Clear()
  {
    prefix=TInquiryPrefix::Empty;
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
    //0-���-�� �� ⨯�� 㪠��. ���砫� ������ �� 䠬����
    //1-��饥 ���-�� �� ⨯�� 㪠��. ���砫� ���᪮��� ��ப�,
    //  �᫨ �� 㪠���� ��� �� � ����� �� 䠬����
    infSeatsFmt=0;
    //0-��᫥ '-' 㪠�뢠���� ���-�� �� ��� ���� �� 㪠������� ���-�� ��
    //  ��᫥ '+' 㪠�뢠���� ���-�� �� ��� ���� � ���������� � ���-�� �� c ���⠬�
    //1-��᫥ '-' 㪠�뢠���� ���-�� �� c ���⠬� �� 㪠������� ���-�� ��
    //  ��᫥ '+' 㪠�뢠���� ���-�� �� c ���⠬� � ���������� � ���-�� �� ��� ����
  };
};

void GetInquiryInfo(TInquiryGroup &grp, TInquiryFormat &fmt, TInquiryGroupSummary &sum)
{
  sum.Clear();
  sum.prefix=grp.prefix;
  if (grp.fams.empty()) throw UserException(100,"MSG.REQUEST.INVALID");
  vector<TInquiryFamily>::iterator i;
  if (!grp.large)
  {
    for(i=grp.fams.begin();i!=grp.fams.end();i++)
    {
      //�஢�ਬ �ࠢ��쭮��� ��஢�� ������ ��। ������ 䠬�����
      if (i==grp.fams.begin()) continue;
      if (i->n[adult]!=-1 || i->n[child]!=-1 || i->n[baby]!=-1 || i->seats!=-1)
      {
        fmt.persCountFmt=0; //��������! �����塞 �ଠ�
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

  if (grp.prefix==TInquiryPrefix::CrewOldStyle)
    throw UserException(100,"MSG.REQUEST_ERROR.USE_CREW_MINUS");
};

void CheckTrferPermit(const pair<CheckIn::TTransferItem, TCkinSegFlts> &in,
                      const pair<CheckIn::TTransferItem, TCkinSegFlts> &out,
                      const bool outboard_trfer,
                      TTrferSetsInfo &sets)
{
  sets.Clear();
  if (!in.first.Valid() || !out.first.Valid()) return;
  if (in.first.airp_arv!=out.first.operFlt.airp) return; //ࠧ�� �/� �ਫ�� � �뫥�

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT trfer_sets.trfer_permit, trfer_sets.trfer_outboard, "
    "       trfer_sets.tckin_permit, trfer_sets.tckin_waitlist, trfer_sets.tckin_norec, "
    "       trfer_sets.min_interval, trfer_sets.max_interval "
    "FROM trfer_sets,trfer_set_flts flts_in,trfer_set_flts flts_out "
    "WHERE trfer_sets.id=flts_in.trfer_set_id(+) AND flts_in.pr_onward(+)=0 AND "
    "      trfer_sets.id=flts_out.trfer_set_id(+) AND flts_out.pr_onward(+)=1 AND "
    "      airline_in=:airline_in AND airp=:airp AND airline_out=:airline_out AND "
    "      (flts_in.flt_no IS NULL OR flts_in.flt_no=:flt_no_in) AND "
    "      (flts_out.flt_no IS NULL OR flts_out.flt_no=:flt_no_out) "
    "ORDER BY flts_out.flt_no,flts_in.flt_no ";
  Qry.CreateVariable("airline_in",otString,in.first.operFlt.airline);
  Qry.CreateVariable("flt_no_in",otInteger,in.first.operFlt.flt_no);
  Qry.CreateVariable("airp",otString,out.first.operFlt.airp);
  Qry.CreateVariable("airline_out",otString,out.first.operFlt.airline);
  Qry.CreateVariable("flt_no_out",otInteger,out.first.operFlt.flt_no);
  Qry.Execute();
  if (Qry.Eof)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT id FROM trfer_sets "
      "WHERE (airline_in=:airline_in AND airline_out=:airline_out OR "
      "       airline_in=:airline_out AND airline_out=:airline_in) AND rownum<2";
    Qry.CreateVariable("airline_in",otString,in.first.operFlt.airline);
    Qry.CreateVariable("airline_out",otString,out.first.operFlt.airline);
    Qry.Execute();
    if (Qry.Eof)
    {
      sets.trfer_permit=outboard_trfer;
      sets.trfer_outboard=outboard_trfer;
    };
  }
  else
  {
    TTrferSetsInfo qrySets;
    qrySets.trfer_permit=Qry.FieldAsInteger("trfer_permit")!=0;
    qrySets.trfer_outboard=Qry.FieldAsInteger("trfer_outboard")!=0;
    qrySets.tckin_permit=Qry.FieldAsInteger("tckin_permit")!=0;
    if (qrySets.tckin_permit)
    {
      qrySets.tckin_waitlist=Qry.FieldAsInteger("tckin_waitlist")!=0;
      qrySets.tckin_norec=Qry.FieldAsInteger("tckin_norec")!=0;
    };

    sets.trfer_outboard=qrySets.trfer_outboard; //� �� ��砥 ������塞 ��� ᫥���饩 ���樨

    int min_interval=Qry.FieldIsNULL("min_interval")?NoExists:Qry.FieldAsInteger("min_interval");
    int max_interval=Qry.FieldIsNULL("max_interval")?NoExists:Qry.FieldAsInteger("max_interval");

    if (in.second.is_edi || out.second.is_edi ||
        in.second.flts.size()!=1 || out.second.flts.size()!=1)
    {
      if (min_interval==NoExists && max_interval==NoExists)
      {
        sets.trfer_permit=qrySets.trfer_permit;
      };
      return;
    };

    const TSegInfo &inSeg=*(in.second.flts.begin());
    const TSegInfo &outSeg=*(out.second.flts.begin());
    if (inSeg.fltInfo.pr_del!=0 || outSeg.fltInfo.pr_del!=0 ||
        inSeg.point_arv==ASTRA::NoExists || outSeg.point_arv==ASTRA::NoExists ||
        inSeg.airp_arv!=outSeg.airp_dep)
    {
      if (min_interval==NoExists && max_interval==NoExists)
      {
        sets.trfer_permit=qrySets.trfer_permit;
      };
      return;
    };

    if ((qrySets.trfer_permit || qrySets.tckin_permit) &&
        (min_interval!=NoExists || max_interval!=NoExists))
    {
      //���� �஢���� �६� �ਫ�� inSeg
      Qry.Clear();
      Qry.SQLText=
        "SELECT NVL(act_in,NVL(est_in,scd_in)) AS real_in FROM points WHERE point_id=:point_id";
      Qry.CreateVariable("point_id",otInteger,inSeg.point_arv);
      Qry.Execute();
      if (!Qry.Eof && !Qry.FieldIsNULL("real_in"))
      {
        if (outSeg.fltInfo.real_out==NoExists)
          throw EXCEPTIONS::Exception("CheckTrferPermit: outSeg.fltInfo.real_out==NoExists");
        double interval=round((outSeg.fltInfo.real_out-Qry.FieldAsDateTime("real_in"))*1440);
        //ProgTrace(TRACE5, "interval=%f, min_interval=%d, max_interval=%d", interval, min_interval, max_interval);
        if ((min_interval!=NoExists && interval<min_interval) ||
            (max_interval!=NoExists && interval>max_interval)) return;
      }
      else return;
    };

    sets=qrySets;
  };
};

struct TSearchResponseInfo //ᥩ�� �� �ᯮ������, ��� ࠧ����!
{
  int resCountOk,resCountPAD;
};

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const map<int, CheckIn::TTransferItem> &trfer )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(map<int, CheckIn::TTransferItem>::const_iterator iTrfer=trfer.begin();iTrfer!=trfer.end();++iTrfer)
  {
    ostringstream str;

    if (iTrfer==trfer.begin())
    {
      str << setw(3) << right << "num" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(4) << right << "date" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv" << " "
          << setw(3) << left  << "scl";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    str << setw(2) << right << iTrfer->first << ": "
        << setw(3) << left  << iTrfer->second.operFlt.airline << " "
        << setw(5) << right << (iTrfer->second.operFlt.flt_no !=NoExists ? IntToString(iTrfer->second.operFlt.flt_no) : " ")
        << setw(1) << left  << iTrfer->second.operFlt.suffix << " "
        << setw(4) << right << (iTrfer->second.operFlt.scd_out !=NoExists ? DateTimeToStr(iTrfer->second.operFlt.scd_out,"dd") : " ") << " "
        << setw(3) << left  << iTrfer->second.operFlt.airp << " "
        << setw(3) << left  << iTrfer->second.airp_arv << " "
        << setw(3) << left  << iTrfer->second.subclass;

    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
  };

  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const map<int, pair<TCkinSegFlts, TTrferSetsInfo> > &segs )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator iSeg=segs.begin();iSeg!=segs.end();++iSeg)
  {
    ostringstream str;

    if (iSeg==segs.begin())
    {
      str << setw(3) << right << "seg" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(3) << left  << "a/p" << " "
          << setw(9) << left  << "scd_out" << " "
          << setw(9) << right << "point_dep" << " "
          << setw(9) << right << "point_arv" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv" << "|"
          << setw(10) << left  << "trfer_perm" << " "
          << setw(10) << left  << "trfer_outb" << " "
          << setw(10) << left  << "tckin_perm" << " "
          << setw(10) << left  << "tckin_wait" << " "
          << setw(10) << left  << "tckin_nore";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    if (!iSeg->second.first.flts.empty())
    {
      for(vector<TSegInfo>::const_iterator f=iSeg->second.first.flts.begin();f!=iSeg->second.first.flts.end();++f)
      {
        if (f==iSeg->second.first.flts.begin())
          str << setw(2) << right << iSeg->first << ": ";
        else
          str << setw(2) << right << " " << "  ";

        str << setw(3) << left  << f->fltInfo.airline << " "
            << setw(5) << right << (f->fltInfo.flt_no != NoExists ? IntToString(f->fltInfo.flt_no) : " ")
            << setw(1) << left  << f->fltInfo.suffix << " "
            << setw(3) << left  << f->fltInfo.airp << " "
            << setw(9) << left  << DateTimeToStr(f->fltInfo.scd_out,"ddmm hhnn") << " "
            << setw(9) << right << (f->point_dep != NoExists ? IntToString(f->point_dep) : "NoExists") << " "
            << setw(9) << right << (f->point_arv != NoExists ? IntToString(f->point_arv) : "NoExists") << " "
            << setw(3) << left  << f->airp_dep << " "
            << setw(3) << left  << f->airp_arv << "|"
            << setw(10) << left  << (iSeg->second.second.trfer_permit   ? "true" : "false") << " "
            << setw(10) << left  << (iSeg->second.second.trfer_outboard ? "true" : "false") << " "
            << setw(10) << left  << (iSeg->second.second.tckin_permit   ? "true" : "false") << " "
            << setw(10) << left  << (iSeg->second.second.tckin_waitlist ? "true" : "false") << " "
            << setw(10) << left  << (iSeg->second.second.tckin_norec    ? "true" : "false");
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

        str.str("");
      };
    }
    else
    {
      str << setw(2) << right << iSeg->first << ": "
          << setw(52) << left << "not found!" << "|"
          << setw(10) << left  << (iSeg->second.second.trfer_permit   ? "true" : "false") << " "
          << setw(10) << left  << (iSeg->second.second.trfer_outboard ? "true" : "false") << " "
          << setw(10) << left  << (iSeg->second.second.tckin_permit   ? "true" : "false") << " "
          << setw(10) << left  << (iSeg->second.second.tckin_waitlist ? "true" : "false") << " "
          << setw(10) << left  << (iSeg->second.second.tckin_norec    ? "true" : "false");

      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };
  };
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const map<int, TCkinSegFlts > &segs )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(map<int, TCkinSegFlts >::const_iterator iSeg=segs.begin();iSeg!=segs.end();++iSeg)
  {
    ostringstream str;

    if (iSeg==segs.begin())
    {
      str << setw(3) << right << "seg" << " "
          << setw(3) << left  << "a/l" << " "
          << setw(6) << right << "flt_no" << " "
          << setw(3) << left  << "a/p" << " "
          << setw(9) << left  << "scd_out" << " "
          << setw(9) << right << "point_dep" << " "
          << setw(9) << right << "point_arv" << " "
          << setw(3) << left  << "dep" << " "
          << setw(3) << left  << "arv";
      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };

    if (!iSeg->second.flts.empty())
    {
      for(vector<TSegInfo>::const_iterator f=iSeg->second.flts.begin();f!=iSeg->second.flts.end();++f)
      {
        if (f==iSeg->second.flts.begin())
          str << setw(2) << right << iSeg->first << ": ";
        else
          str << setw(2) << right << " " << "  ";

        str << setw(3) << left  << f->fltInfo.airline << " "
            << setw(5) << right << (f->fltInfo.flt_no != NoExists ? IntToString(f->fltInfo.flt_no) : " ")
            << setw(1) << left  << f->fltInfo.suffix << " "
            << setw(3) << left  << f->fltInfo.airp << " "
            << setw(9) << left  << DateTimeToStr(f->fltInfo.scd_out,"ddmm hhnn") << " "
            << setw(9) << right << (f->point_dep != NoExists ? IntToString(f->point_dep) : "NoExists") << " "
            << setw(9) << right << (f->point_arv != NoExists ? IntToString(f->point_arv) : "NoExists") << " "
            << setw(3) << left  << f->airp_dep << " "
            << setw(3) << left  << f->airp_arv;
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

        str.str("");
      };
    }
    else
    {
      str << setw(2) << right << iSeg->first << ": " << "not found!";

      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    };
  };
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
};

void CheckInInterface::GetTrferSets(const TTripInfo &operFlt,
                                    const string &oper_airp_arv,
                                    const string &tlg_airp_dep,
                                    const map<int, CheckIn::TTransferItem> &trfer,
                                    const bool get_trfer_permit_only,
                                    map<int, pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs)
{
  trfer_segs.clear();
  if (trfer.empty()) return;
  if (trfer.begin()->first<=0)
    throw EXCEPTIONS::Exception("CheckInInterface::GetTrferSets: wrong trfer");

  bool without_trfer_set=GetTripSets( tsIgnoreTrferSet, operFlt );
  if (!without_trfer_set || !get_trfer_permit_only)
  {
    map<int, CheckIn::TTransferItem> trfer_tmp(trfer);
    CheckIn::TTransferItem firstSeg;
    firstSeg.operFlt=operFlt;
    firstSeg.operFlt.scd_out=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));
    modf(firstSeg.operFlt.scd_out,&firstSeg.operFlt.scd_out); //���㡠�� ���
    firstSeg.airp_arv=oper_airp_arv;
    trfer_tmp[0]=firstSeg;

    //traceTrfer( TRACE5, "GetTrferSets: trfer_tmp", trfer_tmp );

    map<int, pair<CheckIn::TTransferItem, TCkinSegFlts> > trfer_segs_tmp;
    GetTCkinFlights(trfer_tmp, trfer_segs_tmp);

    //traceTrfer( TRACE5, "GetTrferSets: trfer_segs_tmp", trfer_segs_tmp );

    bool outboard_trfer=false;

    pair<int, pair<CheckIn::TTransferItem, TCkinSegFlts> > prior_trfer_seg;
    for(map<int, pair<CheckIn::TTransferItem, TCkinSegFlts> >::const_iterator s=trfer_segs_tmp.begin();
                                                                              s!=trfer_segs_tmp.end(); ++s)
    {
      if (s!=trfer_segs_tmp.begin())
      {
        TTrferSetsInfo sets;
        if (prior_trfer_seg.first+1==s->first)
          CheckTrferPermit(prior_trfer_seg.second, s->second, outboard_trfer, sets);
        outboard_trfer=sets.trfer_outboard;
        trfer_segs[s->first]=make_pair(s->second.second, sets);
      };
      prior_trfer_seg=*s;
    };
  };
  if (without_trfer_set)
  {
    //������㥬 ����ன�� �࠭���
    //��������㥬 �������� ������� ⮫쪮 �᫨ ����������� ����ன�� �࠭���
    //(� ���� � ��த��)
    for(map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin(); t!=trfer.end(); ++t)
    {
      if (/*!tlg_airp_dep.empty() && tlg_airp_dep==t->second.airp_arv ||  //�� �� ��� �맮��� ��।��� tlg_airp_dep,
           tlg_airp_dep.empty() &&*/ operFlt.airp==t->second.airp_arv)    //�� ���� �⮡� �� ��� �맮��� ��ࠡ��뢠�� ���������
        trfer_segs[t->first].second.trfer_permit=false;
      else
        trfer_segs[t->first].second.trfer_permit=true;
    };
  };
};

void CheckInInterface::GetOnwardCrsTransfer(int pnr_id, TQuery &Qry,
                                            const TTripInfo &operFlt,
                                            const string &oper_airp_arv,
                                            map<int, CheckIn::TTransferItem> &trfer)
{
  trfer.clear();
  vector<TypeB::TTransferItem> crs_trfer;
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

  if (crs_trfer.empty()) return;

  TDateTime local_scd=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));

  int prior_transfer_num=0;
  string prior_airp_arv=oper_airp_arv;
  TElemFmt prior_airp_arv_fmt=efmtCodeNative;
  for(vector<TypeB::TTransferItem>::const_iterator t=crs_trfer.begin();t!=crs_trfer.end();++t)
  {
    if ((prior_transfer_num+1!=t->num && *(t->airp_dep)==0) || *(t->airp_arv)==0)
      //��묨 ᫮����, �� ����� ���� �ਫ�� � �।��騥� ��몮��� ���
      //���� �� �� ����� ���� �뫥�
      //�� �訡�� � PNL/ADL - ���� �� ��ࠢ�� ����
      break;

    CheckIn::TTransferItem trferItem;
    //�஢�ਬ ��ଫ���� �࠭���

    trferItem.operFlt.airline=ElemToElemId(etAirline,t->airline,trferItem.operFlt.airline_fmt);
    if (trferItem.operFlt.airline_fmt==efmtUnknown)
      trferItem.operFlt.airline=t->airline;

    trferItem.operFlt.flt_no=t->flt_no;

    if (*(t->suffix)!=0)
    {
      trferItem.operFlt.suffix=ElemToElemId(etSuffix,t->suffix,trferItem.operFlt.suffix_fmt);
      if (trferItem.operFlt.suffix_fmt==efmtUnknown)
        trferItem.operFlt.suffix=t->suffix;
    };

    trferItem.local_date=t->local_date;
    try
    {
      TDateTime base_date=local_scd-1; //��⠬��� ����� �� ������ ����� � ���ਪ� �� ���譨� ����
      local_scd=DayToDate(t->local_date,base_date,false); //�����쭠� ��� �뫥�
      trferItem.operFlt.scd_out=local_scd;
    }
    catch(EXCEPTIONS::EConvertError &E) {};

    if (*(t->airp_dep)!=0)
    {
      trferItem.operFlt.airp=ElemToElemId(etAirp,t->airp_dep,trferItem.operFlt.airp_fmt);
      if (trferItem.operFlt.airp_fmt==efmtUnknown)
        trferItem.operFlt.airp=t->airp_dep;
    }
    else
    {
      trferItem.operFlt.airp=prior_airp_arv;
      trferItem.operFlt.airp_fmt=prior_airp_arv_fmt;
    };


    trferItem.airp_arv=ElemToElemId(etAirp,t->airp_arv,trferItem.airp_arv_fmt);
    if (trferItem.airp_arv_fmt==efmtUnknown)
      trferItem.airp_arv=t->airp_arv;

    trferItem.subclass=ElemToElemId(etSubcls,t->subcl,trferItem.subclass_fmt);
    if (trferItem.subclass_fmt==efmtUnknown)
      trferItem.subclass=t->subcl;

    trfer[t->num]=trferItem;

    prior_transfer_num=t->num;
    prior_airp_arv=trferItem.airp_arv;
    prior_airp_arv_fmt=trferItem.airp_arv_fmt;
  };
};

void CheckInInterface::LoadOnwardCrsTransfer(const map<int, CheckIn::TTransferItem> &trfer,
                                             const map<int, pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs,
                                             xmlNodePtr trferNode)
{
  map<int, pair<CheckIn::TTransferItem, TTrferSetsInfo> > trfer_sets;
  map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin();
  map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator s=trfer_segs.begin();
  for(;t!=trfer.end() && s!=trfer_segs.end(); ++t, ++s)
  {
    if (t->first!=s->first) throw EXCEPTIONS::Exception("LoadOnwardCrsTransfer: wrong trfer_segs");
    trfer_sets[t->first]=make_pair(t->second, s->second.second);
  };
  LoadOnwardCrsTransfer(trfer_sets, trferNode);
};

void CheckInInterface::LoadOnwardCrsTransfer(const map<int, pair<CheckIn::TTransferItem, TTrferSetsInfo> > &trfer,
                                             xmlNodePtr trferNode)
{
  if (trferNode==NULL) return;
  for(map<int, pair<CheckIn::TTransferItem, TTrferSetsInfo> >::const_iterator t=trfer.begin(); t!=trfer.end(); ++t)
  {
    if (t->first<=0) continue;

    xmlNodePtr node2=NewTextChild(trferNode,"segment");
    NewTextChild(node2,"num",t->first);
    if (t->second.first.operFlt.airline_fmt!=efmtUnknown)
      NewTextChild(node2,"airline",ElemIdToClientElem(etAirline,
                                                      t->second.first.operFlt.airline,
                                                      t->second.first.operFlt.airline_fmt));
    else
      NewTextChild(node2,"airline",t->second.first.operFlt.airline);

    NewTextChild(node2,"flt_no",t->second.first.operFlt.flt_no);

    if (t->second.first.operFlt.suffix_fmt!=efmtUnknown)
      NewTextChild(node2,"suffix",ElemIdToClientElem(etSuffix,
                                                     t->second.first.operFlt.suffix,
                                                     t->second.first.operFlt.suffix_fmt),"");
    else
      NewTextChild(node2,"suffix",t->second.first.operFlt.suffix,"");

    NewTextChild(node2,"local_date",t->second.first.local_date);

    if (t->second.first.operFlt.airp_fmt!=efmtUnknown)
      NewTextChild(node2,"airp_dep",ElemIdToClientElem(etAirp,
                                                       t->second.first.operFlt.airp,
                                                       t->second.first.operFlt.airp_fmt),"");
    else
      NewTextChild(node2,"airp_dep",t->second.first.operFlt.airp,"");

    if (t->second.first.airp_arv_fmt!=efmtUnknown)
      NewTextChild(node2,"airp_arv",ElemIdToClientElem(etAirp,
                                                       t->second.first.airp_arv,
                                                       t->second.first.airp_arv_fmt));
    else
      NewTextChild(node2,"airp_arv",t->second.first.airp_arv);

    if (t->second.first.subclass_fmt!=efmtUnknown)
      NewTextChild(node2,"subclass",ElemIdToClientElem(etSubcls,
                                                       t->second.first.subclass,
                                                       t->second.first.subclass_fmt));
    else
      NewTextChild(node2,"subclass",t->second.first.subclass);

    NewTextChild(node2,"trfer_permit",(int)t->second.second.trfer_permit);
  };
};

bool EqualCrsTransfer(const map<int, CheckIn::TTransferItem> &trfer1,
                      const map<int, CheckIn::TTransferItem> &trfer2)
{
  map<int, CheckIn::TTransferItem>::const_iterator i1=trfer1.begin();
  map<int, CheckIn::TTransferItem>::const_iterator i2=trfer2.begin();
  for(;i1!=trfer1.end() && i2!=trfer2.end(); i1++,i2++)
  {
    if (i1->first!=i2->first ||
        i1->second.operFlt.airline!=i2->second.operFlt.airline ||
        i1->second.operFlt.flt_no!=i2->second.operFlt.flt_no ||
        i1->second.operFlt.suffix!=i2->second.operFlt.suffix ||
        i1->second.local_date!=i2->second.local_date ||
        i1->second.operFlt.airp!=i2->second.operFlt.airp ||
        i1->second.airp_arv!=i2->second.airp_arv ||
        i1->second.subclass!=i2->second.subclass) return false;
  };

  return i1==trfer1.end() && i2==trfer2.end();
};

bool LoadUnconfirmedTransfer(const vector<CheckIn::TTransferItem> &segs, xmlNodePtr transferNode)
{
  if (segs.empty() || transferNode==NULL) return false;

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

  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > > crs_trfer, trfer; //����� ��� <tlg_airp_dep+�࠭���� �������, ����� ��. ���ᠦ�஢>

  int pnr_id=NoExists;
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      pnr_id=PaxQry.FieldAsInteger("pnr_id");

      crs_trfer.push_back( make_pair( make_pair( string(), map<int, CheckIn::TTransferItem>() ), vector<int>() ) );

      pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();

      last_crs_trfer.first.first=PaxQry.FieldAsString("tlg_airp_dep");
      CheckInInterface::GetOnwardCrsTransfer(pnr_id, TrferQry, firstSeg.operFlt, firstSeg.airp_arv, last_crs_trfer.first.second); //���⠥� �� ⠡���� crs_transfer
    };

    if (crs_trfer.empty()) continue;
    pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();
    last_crs_trfer.second.push_back(PaxQry.FieldAsInteger("pax_id"));
  };

  ProgTrace(TRACE5,"LoadUnconfirmedTransfer: crs_trfer - step 1");
  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::const_iterator iCrsTrfer=crs_trfer.begin();
  for(;iCrsTrfer!=crs_trfer.end();++iCrsTrfer)
  {
    ProgTrace(TRACE5,"tlg_airp_arv=%s map<int, CheckIn::TTransferItem>.size()=%zu vector<int>.size()=%zu",
                     iCrsTrfer->first.first.c_str(), iCrsTrfer->first.second.size(), iCrsTrfer->second.size());
  };

  //�஡�� �� ���ᠦ�ࠬ ��ࢮ�� ᥣ����
  int pax_no=0;
  int iYear,iMonth,iDay;
  for(vector<CheckIn::TPaxTransferItem>::const_iterator p=firstSeg.pax.begin();p!=firstSeg.pax.end();++p,pax_no++)
  {
    string tlg_airp_dep=firstSeg.operFlt.airp;
    map<int, CheckIn::TTransferItem> pax_trfer;
    //����ࠥ� ����� �࠭���
    int seg_no=1;
    for(vector<CheckIn::TTransferItem>::const_iterator s=segs.begin();s!=segs.end();++s,seg_no++)
    {
      if (s==segs.begin()) continue; //���� ᥣ���� ����뢠��
      CheckIn::TTransferItem trferItem=*s;
      trferItem.operFlt.airline_fmt = efmtCodeNative;
      trferItem.operFlt.suffix_fmt = efmtCodeNative;
      trferItem.operFlt.airp_fmt = efmtCodeNative;
      trferItem.operFlt.scd_out=UTCToLocal(trferItem.operFlt.scd_out,AirpTZRegion(trferItem.operFlt.airp));
      modf(trferItem.operFlt.scd_out,&trferItem.operFlt.scd_out); //���㡠�� ���
      DecodeDate(trferItem.operFlt.scd_out,iYear,iMonth,iDay);
      trferItem.local_date=iDay;
      trferItem.subclass=s->pax.at(pax_no).subclass;
      trferItem.subclass_fmt=s->pax.at(pax_no).subclass_fmt;
      pax_trfer[seg_no-1]=trferItem;
    };
    ProgTrace(TRACE5,"LoadUnconfirmedTransfer: pax_trfer.size()=%zu",pax_trfer.size());
    //⥯��� pax_trfer ᮤ�ন� ᥣ����� ᪢����� ॣ����樨 � �������ᮬ ���ᠦ��
    //���஡㥬 �������� ᥣ����� �� crs_transfer
    vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::const_iterator iCrsTrfer=crs_trfer.begin();
    for(;iCrsTrfer!=crs_trfer.end();++iCrsTrfer)
    {
      if (find(iCrsTrfer->second.begin(),iCrsTrfer->second.end(),p->pax_id)!=iCrsTrfer->second.end())
      {
        //iCrsTrfer 㪠�뢠�� �� �࠭��� ���ᠦ��
        tlg_airp_dep=iCrsTrfer->first.first;
        for(map<int, CheckIn::TTransferItem>::const_iterator iCrsTrferItem=iCrsTrfer->first.second.begin();
                                                             iCrsTrferItem!=iCrsTrfer->first.second.end();++iCrsTrferItem)
        {
          if (iCrsTrferItem->first>=seg_no-1) pax_trfer[iCrsTrferItem->first]=iCrsTrferItem->second; //������塞 ���. ᥣ����� �� ⠡���� crs_transfer
        };
        break;
      };
    };
    ProgTrace(TRACE5,"LoadUnconfirmedTransfer: pax_trfer.size()=%zu",pax_trfer.size());
    //⥯��� pax_trfer ᮤ�ন� ᥣ����� ᪢����� ॣ����樨 � �������ᮬ ���ᠦ��
    //���� �������⥫�� ᥣ����� �࠭��� �� ⠡���� crs_transfer
    //�� TTransferItem � pax_trfer ���஢��� �� ������ �࠭��� (TTransferItem.num)
    vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::iterator iTrfer=trfer.begin();
    for(;iTrfer!=trfer.end();++iTrfer)
    {
      if (iTrfer->first.first==tlg_airp_dep &&
          EqualCrsTransfer(iTrfer->first.second,pax_trfer)) break;
    };
    if (iTrfer!=trfer.end())
      //��諨 �� �� �������
      iTrfer->second.push_back(p->pax_id);
    else
      trfer.push_back( make_pair( make_pair( tlg_airp_dep, pax_trfer ), vector<int>(1,p->pax_id) ) );
  };
  //�ନ�㥬 XML
  xmlNodePtr itemsNode=NewTextChild(transferNode,"unconfirmed_transfer");

  bool result=false;

  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::const_iterator iTrfer=trfer.begin();
  for(;iTrfer!=trfer.end();++iTrfer)
  {
    if (iTrfer->first.second.empty()) continue;
    xmlNodePtr itemNode=NewTextChild(itemsNode,"item");
    result=true;

    map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
    traceTrfer(TRACE5, "LoadUnconfirmedTransfer: trfer", iTrfer->first.second);
    CheckInInterface::GetTrferSets(firstSeg.operFlt,
                                   firstSeg.airp_arv,
                                   iTrfer->first.first,
                                   iTrfer->first.second,
                                   true,
                                   trfer_segs);
    traceTrfer(TRACE5, "LoadUnconfirmedTransfer: trfer_segs", trfer_segs);

    CheckInInterface::LoadOnwardCrsTransfer(iTrfer->first.second, trfer_segs, NewTextChild(itemNode,"transfer"));

    xmlNodePtr paxNode=NewTextChild(itemNode,"passengers");
    for(vector<int>::const_iterator pax_id=iTrfer->second.begin();
                                    pax_id!=iTrfer->second.end();pax_id++)
      NewTextChild( NewTextChild(paxNode, "pax"),"pax_id", *pax_id);
  };
  return result;
};

void GetPaxNoRecResponse(const TInquiryGroupSummary &sum,
                         list<CheckIn::TPaxItem> &paxs)
{
  paxs.clear();
  if (sum.persCountFmt==0)
  {
    for(vector<TInquiryFamilySummary>::const_iterator f=sum.fams.begin();f!=sum.fams.end();f++)
    {
      int n=1;
      for(int p=0;p<NoPerson;p++)
      {
        for(int i=0;i<f->n[p];i++,n++)
        {
          CheckIn::TPaxItem pax;
          pax.surname=f->surname;
          pax.name=f->name;
          pax.pers_type=(TPerson)p;
          pax.seats=1;
          if (n>f->nPaxWithPlace)
            pax.seats=0;
          paxs.push_back(pax);
        };
      };
    };
  }
  else
  {
    int n=1;
    vector<TInquiryFamilySummary>::const_iterator f=sum.fams.begin();
    for(int p=0;p<NoPerson;p++)
    {
      for(int i=0;i<sum.n[p];i++,n++)
      {
        CheckIn::TPaxItem pax;
        if (f!=sum.fams.end())
        {
          pax.surname=f->surname;
          pax.name=f->name;
          f++;
        };
        pax.pers_type=(TPerson)p;
        pax.seats=1;
        if (n>sum.nPaxWithPlace)
          pax.seats=0;
        paxs.push_back(pax);
      };
    };
  };
};

void CreateNoRecResponse(const TInquiryGroupSummary &sum, xmlNodePtr resNode)
{
  list<CheckIn::TPaxItem> paxs;
  GetPaxNoRecResponse(sum, paxs);
  xmlNodePtr paxNode=NewTextChild(resNode,"norec");
  for(list<CheckIn::TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    if (sum.prefix==TInquiryPrefix::ExtraCrew ||
        sum.prefix==TInquiryPrefix::DeadHeadCrew ||
        sum.prefix==TInquiryPrefix::MiscOperStaff)
    {
      if (p->pers_type!=adult || p->seats<1)
        throw UserException("MSG.PASSENGER.IS_ADULT_WITH_AT_LEAST_ONE_SEAT");
    };

    xmlNodePtr node=NewTextChild(paxNode,"pax");
    NewTextChild(node,"surname",p->surname,"");
    NewTextChild(node,"name",p->name,"");
    NewTextChild(node,"pers_type",EncodePerson(p->pers_type),EncodePerson(ASTRA::adult));
    NewTextChild(node,"seats",p->seats,1);

    string rem_code, rem_text;
    switch(sum.prefix)
    {
      case TInquiryPrefix::ExtraCrew:     rem_code=CrewTypes().encode(TCrewType::ExtraCrew); rem_text=rem_code+" 2"; break;
      case TInquiryPrefix::DeadHeadCrew:  rem_code=CrewTypes().encode(TCrewType::DeadHeadCrew); rem_text=rem_code; break;
      case TInquiryPrefix::MiscOperStaff: rem_code=CrewTypes().encode(TCrewType::MiscOperStaff); rem_text=rem_code; break;
      default: break;
    }
    if (!rem_code.empty())
    {
      xmlNodePtr remsNode=NewTextChild(node,"rems");
      xmlNodePtr remNode=NewTextChild(remsNode,"rem");
      NewTextChild(remNode,"rem_code",rem_code);
      NewTextChild(remNode,"rem_text",rem_text);
    }
  };
};

void CreateCrewResponse(int point_dep, const TInquiryGroupSummary &sum, xmlNodePtr resNode)
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

  xmlNodePtr tripNode=GetNode("trips",resNode);
  if (tripNode==NULL)
    tripNode=NewTextChild(resNode,"trips");
  xmlNodePtr node=NewTextChild(tripNode,"trip");
  NewTextChild(node,"point_id",point_dep);
  NewTextChild(node,"airline",operFlt.airline);
  NewTextChild(node,"flt_no",operFlt.flt_no);
  NewTextChild(node,"suffix",operFlt.suffix,"");
  TDateTime local_scd=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));
  modf(local_scd,&local_scd); //���㡠�� ���
  NewTextChild(node,"scd",DateTimeToStr(local_scd));
  NewTextChild(node,"airp_dep",operFlt.airp);

  TTripRoute route;
  route.GetRouteAfter(NoExists,
                      point_dep,
                      trtNotCurrent,trtNotCancelled);

  xmlNodePtr pnrNode=NewTextChild(node,"groups");
  node=NewTextChild(pnrNode,"pnr");
  NewTextChild(node,"pnr_id",-1); //crew compatible
  if (route.empty())
    NewTextChild(node,"airp_arv");
  else
    NewTextChild(node,"airp_arv",route.back().airp);
  NewTextChild(node,"subclass"," "); //crew compatible
  NewTextChild(node,"class",EncodeClass(Y)); //crew compatible

  list<CheckIn::TPaxItem> paxs;
  GetPaxNoRecResponse(sum, paxs);

  xmlNodePtr paxNode=NewTextChild(node,"passengers");
  for(list<CheckIn::TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    if (p->pers_type!=adult || p->seats!=1)
      throw UserException("MSG.CREW.MEMBER_IS_ADULT_WITH_ONE_SEAT");
    if (p->surname.empty())
      throw UserException("MSG.CHECKIN.SURNAME_LESS_PASSENGERS_FOR_GRP");

    node=NewTextChild(paxNode,"pax");
    NewTextChild(node,"pax_id",-1); //crew compatible
    NewTextChild(node,"surname",p->surname);
    NewTextChild(node,"name",p->name,"");
    NewTextChild(node,"pers_type",EncodePerson(p->pers_type),EncodePerson(ASTRA::adult));
    NewTextChild(node,"seats",p->seats,1);

    NewTextChild(node,"ticket_no","-");      //crew compatible
    NewTextChild(node,"ticket_rem","TKNA");  //crew compatible

    xmlNodePtr remsNode=NewTextChild(node,"rems");
    xmlNodePtr remNode=NewTextChild(remsNode,"rem");
    NewTextChild(remNode,"rem_code","CREW");
    NewTextChild(remNode,"rem_text","CREW");
  };
};

void LoadPaxRemAndASVC(int pax_id, xmlNodePtr node, bool from_crs)
{
  if (node==NULL) return;

  multiset<CheckIn::TPaxRemItem> rems_and_asvc;
  CheckIn::PaxRemAndASVCFromDB(pax_id, from_crs, rems_and_asvc);
  CheckIn::TPaxRemItem apps_satus_rem = getAPPSRem( pax_id, TReqInfo::Instance()->desk.lang );
  if ( !apps_satus_rem.empty() )
   rems_and_asvc.insert( apps_satus_rem );
  CheckIn::PaxRemAndASVCToXML(rems_and_asvc, node);

  multiset<TPaxEMDItem> emds;
  if (!from_crs)
  {
    GetPaxEMD(pax_id, emds);
    xmlNodePtr asvcNode=NewTextChild(node,"asvc_rems");
    for(multiset<TPaxEMDItem>::const_iterator e=emds.begin(); e!=emds.end(); ++e)
      e->toXML(asvcNode);
  };
}

void LoadPaxFQT(int pax_id, xmlNodePtr node, bool from_crs)
{
  if (node==NULL) return;

  set<CheckIn::TPaxFQTItem> fqts;
  CheckIn::PaxFQTFromDB(pax_id, from_crs, fqts);
  CheckIn::PaxFQTToXML(fqts, node);
}

int CreateSearchResponse(int point_dep, TQuery &PaxQry, xmlNodePtr resNode)
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

  int point_id=-1;
  int pnr_id=-1, pax_id;
  xmlNodePtr tripNode = NULL,pnrNode = NULL,paxNode = NULL,node = NULL;

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
      modf(local_scd,&local_scd); //���㡠�� ���
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
      if (!mktFlt.empty())
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
          //䠪��᪨� �� ᮢ������ � �������᪨�
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

      map<int, CheckIn::TTransferItem> trfer;
      CheckInInterface::GetOnwardCrsTransfer(pnr_id, TrferQry,
                                             operFlt,
                                             PaxQry.FieldAsString("airp_arv"),
                                             trfer);
      traceTrfer( TRACE5, "CreateSearchResponse: trfer", trfer );
      if (!trfer.empty())
      {
        map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
        CheckInInterface::GetTrferSets(operFlt,
                                       PaxQry.FieldAsString("airp_arv"),
                                       tlgTripsFlt.airp,
                                       trfer,
                                       true,
                                       trfer_segs);
        traceTrfer( TRACE5, "CreateSearchResponse: trfer_segs", trfer_segs );
        CheckInInterface::LoadOnwardCrsTransfer(trfer, trfer_segs, NewTextChild(node,"transfer"));
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
    //��ࠡ�⪠ ���㬥�⮢
    CheckIn::TPaxDocItem doc;
    if (LoadCrsPaxDoc(pax_id, doc)) doc.toXML(node);
    //��ࠡ�⪠ ���
    CheckIn::TPaxDocoItem doco;
    if (LoadCrsPaxVisa(pax_id, doco)) doco.toXML(node);
    //��ࠡ�⪠ ���ᮢ
    if (TReqInfo::Instance()->desk.compatible(DOCA_VERSION))
    {
      list<CheckIn::TPaxDocaItem> doca;
      if (LoadCrsPaxDoca(pax_id, doca))
      {
        xmlNodePtr docaNode=NewTextChild(node, "addresses");
        for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
          d->toXML(docaNode);
      };
    };

    //��ࠡ�⪠ ����⮢
    string ticket_no;
    if (!PaxQry.FieldIsNULL("eticket"))
    {
      //����� TKNE
      ticket_no=PaxQry.FieldAsString("eticket");
      int coupon_no=NoExists;
      string::size_type pos=ticket_no.find_last_of('/');
      if (pos!=string::npos)
      {
        if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
            coupon_no>=1 && coupon_no<=4)
          ticket_no.erase(pos);
        else
          coupon_no=NoExists;
      };

      NewTextChild(node,"ticket_no",ticket_no);
      NewTextChild(node,"coupon_no",coupon_no,NoExists);
      NewTextChild(node,"ticket_rem","TKNE");
    }
    else
    {
      if (!PaxQry.FieldIsNULL("ticket"))
      {
        //����� TKNA
        ticket_no=PaxQry.FieldAsString("ticket");
        NewTextChild(node,"ticket_no",ticket_no);
        NewTextChild(node,"ticket_rem","TKNA");
      };
    };

    LoadPaxRemAndASVC(pax_id, node, true);
    LoadPaxFQT(pax_id, node, true);
  }
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

  //2 ��室�:
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

  //2 ��室�:
  for(int pass=1;pass<=2;pass++)
  {
    if ((pass==1 && pax_status!=psCheckin && pax_status!=psGoshow) ||
        (pass==2 && pax_status==psCheckin)) continue;
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

    if ((pass==1 && pax_status==psCheckin && !select_pad_with_ok) ||
        (pass==2 && pax_status==psGoshow))
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
  TReqInfo *reqInfo = TReqInfo::Instance();

  int point_dep = NodeAsInteger("point_dep",reqNode);
  TPaxStatus pax_status = DecodePaxStatus(NodeAsString("pax_status",reqNode));
  string query= NodeAsString("query",reqNode);
  bool pr_unaccomp=query=="0";
  bool charter_search=false;

  TTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_dep))
    throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripSetList setList;
  setList.fromDB(point_dep);
  if (setList.empty())
    throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(PIECE_CONCEPT_VERSION) &&
      setList.value(tsPieceConcept))
    throw UserException("MSG.TERM_VERSION.PIECE_CONCEPT_NOT_SUPPORTED");

  TInquiryGroup grp;
  TInquiryGroupSummary sum;
  TQuery Qry(&OraSession);
  if (!pr_unaccomp)
  {
    readTripCounters(point_dep,resNode);

    ParseInquiryStr(query, pax_status, grp);
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

    charter_search=GetTripSets(tsCharterSearch,fltInfo);

    GetInquiryInfo(grp,fmt,sum);
  };

  xmlNodePtr inquiryNode=NewTextChild(resNode,"inquiry_summary");
  NewTextChild(inquiryNode,"count",sum.nPax);
  NewTextChild(inquiryNode,"seats",sum.nPaxWithPlace);
  NewTextChild(inquiryNode,"adult",sum.n[adult]);
  NewTextChild(inquiryNode,"child",sum.n[child]);
  NewTextChild(inquiryNode,"infant",sum.n[baby]);
  NewTextChild(inquiryNode,"pax_status",EncodePaxStatus(pax_status));
  if (grp.large && !sum.fams.empty())
    NewTextChild(inquiryNode,"grp_name",sum.fams.begin()->surname);
  else
    NewTextChild(inquiryNode,"grp_name");

  xmlNodePtr node;

  if (pax_status==psTransit)
  {
    Qry.Clear();
    Qry.SQLText="SELECT pr_tranz_reg FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_dep);
    Qry.Execute();
    if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    if (Qry.FieldIsNULL("pr_tranz_reg")||Qry.FieldAsInteger("pr_tranz_reg")==0)
      throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");
  };

  if (pr_unaccomp)
  {
    NewTextChild(resNode,"ckin_state","BagBeforeReg");
    return;
  };

  if ((grp.prefix==TInquiryPrefix::Empty && grp.digCkin) ||
      grp.prefix==TInquiryPrefix::NoRec ||
      grp.prefix==TInquiryPrefix::ExtraCrew ||
      grp.prefix==TInquiryPrefix::DeadHeadCrew ||
      grp.prefix==TInquiryPrefix::MiscOperStaff)
  {
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  };

  if (pax_status==psCrew || grp.prefix==TInquiryPrefix::Crew)
  {
    if (pax_status!=psCrew)
    {
      pax_status=psCrew;
      ReplaceTextChild(inquiryNode,"pax_status",EncodePaxStatus(pax_status));
    };
    CreateCrewResponse(point_dep,sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  };

  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  string sql;

  if (grp.large)
  {
    //������ ��㯯�
    sql=  "SELECT crs_pnr.pnr_id, \n"
          "       MIN(crs_pnr.grp_name) AS grp_name, \n"
          "       MIN(DECODE(crs_pax.pers_type,'��', \n"
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
                      //break �� ����!
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
      //��ப� NoRec
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

    //����� ����
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
                      //break �� ����!
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
    //����ந� ����� ��१����� 䠬���� ���᪮���� �����
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
      //横� �� PNR
      for(;pnrNode!=NULL;pnrNode=pnrNode->next)
      {
        //�஢�ਬ �� ���-�� 祫���� � ��㯯�
        int n=0;
        paxNode=NodeAsNode("passengers",pnrNode)->children;
        for(node=paxNode;node!=NULL;node=node->next) n++;
        if (n!=sum.nPax) continue;

        //�஢�ਬ �� � �⮡� �� 䠬���� ������⢮���� � ��㯯�
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
          if (node==NULL) break; //�� ������� 䠬����
        };
        if (i==surnames.end())
        {
          //�� 䠬���� �������
          if (foundPnr!=NULL)
          {
            // ������ 2 ���室��� ��㯯�
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
      //������� ⮫쪮 ���� ���室��� ��㯯�
      //�஢�ਬ �� ��� �� PAD �� ���᪥ �஭�
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
    showErrorMessage("����㯭�� ���� ��⠫��� "+IntToString(free));*/

};

int CheckInInterface::CheckCounters(int point_dep,
                                    int point_arv,
                                    const string &cl,
                                    TPaxStatus grp_status,
                                    const TCFG &cfg,
                                    bool free_seating)
{
    if (cfg.empty() && free_seating) return ASTRA::NoExists;
    //�஢�ઠ ������ ᢮������ ����
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
      ProgTrace(TRACE0,"counters2 empty! (point_dep=%d point_arv=%d cl=%s)",point_dep,point_arv,cl.c_str());
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

  TTripSetList setList;
  setList.fromDB(point_id);
  if (setList.empty()) throw EXCEPTIONS::Exception("Flight not found in trip_sets (point_id=%d)",point_id);

  if (!setList.value(tsCheckLoad) && setList.value(tsOverloadReg)) return false;

  if (setList.value(tsOverloadReg))
  {
    AstraLocale::showErrorMessage("MSG.FLIGHT.MAX_COMMERCE");
    return true;
  }
  else
    throw CheckIn::OverloadException("MSG.FLIGHT.MAX_COMMERCE");
  return false;
};

void CheckInInterface::ArrivalPaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id=NodeAsInteger("point_id",reqNode); //�� point_arv
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
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TTripInfo operFlt(Qry);
  bool free_seating=SALONS2::isFreeSeating(point_id);

  NewTextChild(resNode,"flight",GetTripName(operFlt,ecCkin,true,false));

  bool createDefaults=false;
  string def_pers_type=ElemIdToCodeNative(etPersType, EncodePerson(ASTRA::adult));
  string def_class=ElemIdToCodeNative(etClass, EncodeClass(ASTRA::Y));
  int def_client_type_id=(int)ctTerm;
  int def_status_id=(int)psCheckin;

  bool with_rcpt_info=(strcmp((char *)reqNode->name, "BagPaxList")==0);

  bool check_pay_on_tckin_segs=false;
  if (with_rcpt_info) check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, operFlt);

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
    "  pax_grp.piece_concept, "
    "  ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
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
    "      point_dep=:point_id AND "
    "      pr_brd IS NOT NULL ";
  if (with_rcpt_info)
    sql <<
    "  AND pax_grp.status NOT IN ('E') "
    "  AND ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, "
    "                            pax_grp.piece_concept, pax_grp.excess, pax.pax_id)<>0 ";
  sql <<
    "ORDER BY pax.reg_no, pax.seats DESC"; //� ���饬 ���� ORDER BY

  ProgTrace(TRACE5, "%s", sql.str().c_str());

  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.CreateVariable("lang",otString,reqInfo->desk.lang);
  Qry.Execute();
  xmlNodePtr node=NewTextChild(resNode,"passengers");
  TExcessNodeList excessNodeList;
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
    int col_piece_concept=Qry.FieldIndex("piece_concept");
    int col_tags=Qry.FieldIndex("tags");

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

    map< pair<int/*grp_id*/, int/*pax_id*/>, pair<bool/*pr_payment*/, bool/*pr_receipts*/> > rcpt_complete_map;
    TPaxSeats priorSeats(point_id);
    TQuery PaxDocQry(&OraSession);
    TRemGrp rem_grp;
    rem_grp.Load(retCKIN_VIEW, operFlt.airline);
    for(;!Qry.Eof;Qry.Next())
    {
      int grp_id = Qry.FieldAsInteger(col_grp_id);
      int pax_id = Qry.FieldAsInteger(col_pax_id);
      int reg_no = Qry.FieldAsInteger(col_reg_no);
      string cl = Qry.FieldAsString(col_class);

      xmlNodePtr paxNode=NewTextChild(node,"pax");
      NewTextChild(paxNode,"pax_id",pax_id);
      NewTextChild(paxNode,"reg_no",reg_no);
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

      TPaxStatus status_id=DecodePaxStatus(Qry.FieldAsString(col_status));

      if (status_id!=psCrew)
      {
        if (!cl.empty())
        {
          if (reqInfo->desk.compatible(LATIN_VERSION))
            NewTextChild(paxNode,"class",ElemIdToCodeNative(etClass, cl), def_class);
          else
            NewTextChild(paxNode,"class",ElemIdToCodeNative(etClass, cl));
        }
      }
      else NewTextChild(paxNode,"class",CREW_CLASS_VIEW); //crew compatible

      NewTextChild(paxNode,"subclass",ElemIdToCodeNative(etSubcls, Qry.FieldAsString(col_subclass)));

      if (!free_seating)
      {
        if (Qry.FieldIsNULL(col_wl_type))
        {
          //�� �� ���� ��������, �� �������� ����� ���� �� ᬥ�� ����������
          if (!cl.empty() && Qry.FieldIsNULL(col_seat_no) && Qry.FieldAsInteger(col_seats)>0)
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
          NewTextChild(paxNode,"seat_no_str","��");
          NewTextChild(paxNode,"seat_no_alarm",(int)true);
        };
      };
      string seat_no = Qry.FieldAsString(col_seat_no);
      if ( !TReqInfo::Instance()->desk.compatible(SORT_SEAT_NO_VERSION) )
        seat_no = LTrimString( seat_no );
      NewTextChild(paxNode,"seat_no",seat_no);
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger(col_seats),1);

      if (reqInfo->desk.compatible(LATIN_VERSION))
      {
        NewTextChild(paxNode,"pers_type",ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)), def_pers_type);
        NewTextChild(paxNode,"document", CheckIn::GetPaxDocStr(NoExists, pax_id, true), "");
      }
      else
      {
        NewTextChild(paxNode,"pers_type",ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)));
        NewTextChild(paxNode,"document", CheckIn::GetPaxDocStr(NoExists, pax_id, true));
      };

      NewTextChild(paxNode,"ticket_rem",Qry.FieldAsString(col_ticket_rem),"");
      NewTextChild(paxNode,"ticket_no",Qry.FieldAsString(col_ticket_no),"");
      NewTextChild(paxNode,"bag_amount",Qry.FieldAsInteger(col_bag_amount),0);
      NewTextChild(paxNode,"bag_weight",Qry.FieldAsInteger(col_bag_weight),0);
      NewTextChild(paxNode,"rk_weight",Qry.FieldAsInteger(col_rk_weight),0);

      xmlNodePtr excessNode = NewTextChild(paxNode,"excess",Qry.FieldAsInteger(col_excess),0);
      bool piece_concept=Qry.FieldAsInteger(col_piece_concept)!=0;
      excessNodeList.set_concept(excessNode, piece_concept);

      NewTextChild(paxNode,"tags",Qry.FieldAsString(col_tags),"");
      NewTextChild(paxNode,"rems",GetRemarkStr(rem_grp, pax_id, reqInfo->desk.lang),"");

      //�������᪨� ३�
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
        string receipts=piece_concept?PieceConcept::GetBagRcptStr(grp_id, pax_id):
                                      WeightConcept::GetBagRcptStr(grp_id, pax_id);
        NewTextChild(paxNode,"rcpt_no_list",receipts,"");

        pair<int, int> rcpt_complete_key=piece_concept?make_pair(grp_id, pax_id):make_pair(grp_id, NoExists);
        map< pair<int, int>, pair<bool, bool> >::iterator i=rcpt_complete_map.find(rcpt_complete_key);
        if (i==rcpt_complete_map.end())
        {
          bool pr_payment=RFISCPaymentCompleted(grp_id, pax_id, check_pay_on_tckin_segs) &&
                          WeightConcept::BagPaymentCompleted(grp_id);
          rcpt_complete_map[rcpt_complete_key]=make_pair(pr_payment, !receipts.empty());
        }
        else
        {
          i->second.second= i->second.second || !receipts.empty();
        };
      };
      //�����䨪����
      NewTextChild(paxNode,"grp_id",grp_id);
      if (status_id!=psCrew)
        NewTextChild(paxNode,"cl_grp_id",Qry.FieldAsInteger(col_cl_grp_id));
      else
        NewTextChild(paxNode,"cl_grp_id",CREW_CLS_GRP_ID); //crew compatible
      if (!Qry.FieldIsNULL(col_hall_id))
        NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger(col_hall_id));
      else
        NewTextChild(paxNode,"hall_id",-1);
      NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger(col_point_arv));
      if (!Qry.FieldIsNULL(col_user_id))
        NewTextChild(paxNode,"user_id",Qry.FieldAsInteger(col_user_id));
      else
        NewTextChild(paxNode,"user_id",-1);
      if (reqInfo->desk.compatible(LATIN_VERSION))
      {
        NewTextChild(paxNode,"client_type_id",
                             (int)DecodeClientType(Qry.FieldAsString(col_client_type)),
                             def_client_type_id);
        NewTextChild(paxNode,"status_id",(int)status_id,def_status_id);
      }
      else
      {
        NewTextChild(paxNode,"client_type_id",(int)DecodeClientType(Qry.FieldAsString(col_client_type)));
        NewTextChild(paxNode,"status_id",(int)status_id);
      };
    };
    if (with_rcpt_info)
    {
      for(xmlNodePtr paxNode=node->children;paxNode!=NULL;paxNode=paxNode->next)
      {
        xmlNodePtr node2=paxNode->children;
        int grp_id=NodeAsIntegerFast("grp_id",node2);
        int pax_id=NodeAsIntegerFast("pax_id",node2);
        map< pair<int, int>, pair<bool, bool> >::iterator i;
        i=rcpt_complete_map.find(make_pair(grp_id, pax_id));
        if (i==rcpt_complete_map.end())
          i=rcpt_complete_map.find(make_pair(grp_id, NoExists));
        if (i==rcpt_complete_map.end())
          throw EXCEPTIONS::Exception("CheckInInterface::PaxList: grp_id=%d not found", grp_id);
        // �� �� ���⠭樨 �ᯥ�⠭�:
        // 0 - ���筮 �����⠭�
        // 1 - �� �����⠭�
        // 2 - ��� �� ����� ���⠭樨
        int rcpt_complete=2;
        if (i->second.second) rcpt_complete=(int)i->second.first;
        if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
        else
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,0);
      };
    };
  };

  //��ᮯ஢������� �����
  sql.str("");

  sql <<
    "SELECT "
    "  pax_grp.airp_arv,pax_grp.status, "
    "  pax_grp.piece_concept, "
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
    "      point_dep=:point_id AND class IS NULL AND status NOT IN ('E') ";

  if (with_rcpt_info)
    sql <<
    "  AND ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, "
    "                            pax_grp.piece_concept, pax_grp.excess, NULL)<>0 ";

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

      xmlNodePtr excessNode = NewTextChild(paxNode,"excess",Qry.FieldAsInteger("excess"),0);
      bool piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
      excessNodeList.set_concept(excessNode, piece_concept);

      NewTextChild(paxNode,"tags",Qry.FieldAsString("tags"),"");
      if (with_rcpt_info)
      {
        string receipts=piece_concept?PieceConcept::GetBagRcptStr(grp_id, NoExists):
                                      WeightConcept::GetBagRcptStr(grp_id, NoExists);
        NewTextChild(paxNode,"rcpt_no_list",receipts,"");
        // �� �� ���⠭樨 �ᯥ�⠭�
        //0 - ���筮 �����⠭�
        //1 - �� �����⠭�
        //2 - ��� �� ����� ���⠭樨
        int rcpt_complete=2;
        if (!receipts.empty()) rcpt_complete=piece_concept?(int)true:
                                                           (int)WeightConcept::BagPaymentCompleted(grp_id);
        if (reqInfo->desk.compatible(BAG_RCPT_KITS_VERSION))
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
        else
          NewTextChild(paxNode,"rcpt_complete",rcpt_complete,0);
      };
      //�����䨪����
      NewTextChild(paxNode,"grp_id",grp_id);
      if (!Qry.FieldIsNULL("hall_id"))
        NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger("hall_id"));
      else
        NewTextChild(paxNode,"hall_id",-1);
      NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger("point_arv"));
      if (!Qry.FieldIsNULL("user_id"))
        NewTextChild(paxNode,"user_id",Qry.FieldAsInteger("user_id"));
      else
        NewTextChild(paxNode,"user_id",-1);

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
    //�� ��� ��ᮯ஢��������� ������
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
    //�����䨪����
    NewTextChild(defNode, "client_type_id", def_client_type_id);
    NewTextChild(defNode, "status_id", def_status_id);
  };

  get_new_report_form("ArrivalPaxList", reqNode, resNode);
  xmlNodePtr variablesNode = STAT::set_variables(resNode);
  TRptParams rpt_params(reqInfo->desk.lang);
  PaxListVars(point_id, rpt_params, variablesNode);
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.ARRIVAL_PAX_LIST", LParams() << LParam("flight", get_flight(variablesNode))));
};

bool GetUsePS()
{
    return false; //!!!
}

bool CheckInInterface::CheckCkinFlight(const int point_dep,
                                       const string& airp_dep,
                                       const int point_arv,
                                       const string& airp_arv,
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

bool CheckInInterface::CheckFQTRem(const CheckIn::TPaxRemItem &rem, CheckIn::TPaxFQTItem &fqt)
{
  CheckIn::TPaxRemItem rem2=rem;
  //�஢�ਬ ���४⭮��� ६�ન FQT...
  string rem_code2=rem2.text.substr(0,4);
  if (rem_code2=="FQTV" ||
      rem_code2=="FQTU" ||
      rem_code2=="FQTR" ||
      rem_code2=="FQTS" )
  {
    if (rem2.code!=rem_code2)
    {
      rem2.code=rem_code2;
      if (rem2.text.size()>4) rem2.text.insert(4," ");
    };

    TypeB::TTlgParser parser;
    return ParseFQTRem(parser,rem2.text,fqt);
  };
  return false;
};

bool CheckInInterface::ParseFQTRem(TypeB::TTlgParser &tlg, string &rem_text, CheckIn::TPaxFQTItem &fqt)
{
  fqt.clear();

  char c;
  int res,k;

  const char *p=rem_text.c_str();

  TypeB::TFQTItem fqth;

  if (rem_text.empty()) return false;
  p=tlg.GetWord(p);
  c=0;
  res=sscanf(tlg.lex,"%5[A-Z�-��0-9]%c",fqth.rem_code,&c);
  if (c!=0||res!=1) return false;

  if (strcmp(fqth.rem_code,"FQTV")==0||
      strcmp(fqth.rem_code,"FQTR")==0||
      strcmp(fqth.rem_code,"FQTU")==0||
      strcmp(fqth.rem_code,"FQTS")==0)
  {
    try
    {
      //�஢�ਬ �⮡� ६�ઠ ᮤ�ঠ�� ⮫쪮 �㪢�, ����, �஡��� � ���
      for(string::const_iterator i=rem_text.begin();i!=rem_text.end();i++)
        if (!(IsUpperLetter(*i) ||
              IsDigit(*i) ||
              (*i>0 && *i<=' ') ||
              *i=='/' ||
              *i=='#' ||
              *i=='!')) throw UserException("MSG.INVALID_SYMBOL",
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
            res=sscanf(tlg.lex,"%3[A-Z�-��]%c",fqth.airline,&c);
            if (c!=0||res!=1)
            {
              c=0;
              res=sscanf(tlg.lex,"%2[A-Z�-��0-9]%c",fqth.airline,&c);
              if (c!=0||res!=1)
                throw UserException("MSG.AIRLINE.INVALID_INPUT_VALUE",
                                    LParams()<<LParam("airline", string(tlg.lex))); //WEB
            };

            try
            {
              TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("code/code_lat",fqth.airline));
              strcpy(fqth.airline,row.code.c_str());
            }
            catch (EBaseTableError)
            {
              throw UserException("MSG.AIRLINE.INVALID_INPUT_VALUE",
                                  LParams()<<LParam("airline", string(fqth.airline))); //WEB
            };
            break;
          case 1:
            res=sscanf(tlg.lex,"%25[A-Z�-��0-9]%c",fqth.no,&c);
            if (c!=0||res!=1||strlen(fqth.no)<2)
              throw UserException("MSG.PASSENGER.INVALID_IDENTIFIER",
                                   LParams()<<LParam("ident", string(tlg.lex))); //WEB
            for(;*p!=0;p++)
              if (IsDigitIsLetter(*p)) break;
            fqth.extra=p;
            TrimString(fqth.extra);
            break;
        };
      }
      catch(std::exception)
      {
        switch(k)
        {
          case 0:
            *fqth.airline=0;
            throw;
          case 1:
            *fqth.no=0;
            throw;
        };
      };
    }
    catch(UserException &E)
    {
      throw UserException("WRAP.REMARK_ERROR",
                          LParams()<<LParam("rem_code", string(fqth.rem_code))
                                   <<LParam("text", E.getLexemaData( ) )); //WEB
    };
    fqt.rem=fqth.rem_code;
    fqt.airline=fqth.airline;
    fqt.no=fqth.no;
    fqt.extra=fqth.extra;
    return true;
  };
  return false;
};

bool CheckInInterface::CheckAPPSRems(const std::multiset<CheckIn::TPaxRemItem> &rems, std::string& override, bool& is_forced)
{
  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
  {
    // �� ���� ࠧ ����� ��ࠢ��� ⮫쪮 ���� ����� ��� ���ᠦ��
    if ( r->code == "RSIA" )
      is_forced = true;
    else if ( ( ( r->code == "ATH" || r->code == "GTH" ) && override.find("TH") == std::string::npos ) ||
              ( ( r->code == "AAE" || r->code == "GAE" ) && override.find("AE") == std::string::npos ) ) {
      override += r->code;
      is_forced = true;
    }
    else if ( r->code == "GTH" )
      boost::replace_all(override, "ATH", "GTH");
    else if ( r->code == "GAE" )
      boost::replace_all(override, "AAE", "GAE");
  };
  return is_forced;
}

LexemaData GetLexemeDataWithFlight(const LexemaData &data, const TTripInfo &fltInfo)
{
  LexemaData result;
  result.lexema_id="WRAP.FLIGHT";
  result.lparams << LParam("flight",GetTripName(fltInfo,ecCkin,true,false))
                 << LParam("text",data);
  return result;
};

bool CheckRefusability(int point_dep, int pax_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!(reqInfo->client_type==ctWeb ||
        reqInfo->client_type==ctKiosk ||
        reqInfo->client_type==ctMobile)) return true;
  //�⬥�� ॣ����樨 � ᠩ�, ���᪠ ��� �����쭮��
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.grp_id, pax_grp.client_type, pax.pr_brd "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  if (Qry.Eof ||
      Qry.FieldIsNULL("pr_brd") ||     //�⬥�� ॣ����樨
      Qry.FieldAsInteger("pr_brd")!=0) return false;
  int grp_id=Qry.FieldAsInteger("grp_id");
  TClientType ckinClientType=DecodeClientType(Qry.FieldAsString("client_type"));
  if (!(ckinClientType==ctWeb ||
        ckinClientType==ctKiosk ||
        ckinClientType==ctMobile)) return false; //ॣ����樨 �� � ᠩ�, ���᪠ ��� �����쭮��

  Qry.Clear();
  Qry.SQLText=
    "SELECT events_bilingual.station "
    "FROM "
    "  (SELECT station FROM events_bilingual "
    "   WHERE lang=:lang AND type=:evtPax AND id1=:point_dep AND id3=:grp_id) events_bilingual, "
    "  web_clients "
    "WHERE events_bilingual.station=web_clients.desk(+) AND "
    "      web_clients.desk IS NULL AND rownum<2";
  Qry.CreateVariable("lang", otString, AstraLocale::LANG_RU);
  Qry.CreateVariable("evtPax", otString, EncodeEventType(ASTRA::evtPax));
  Qry.CreateVariable("point_dep", otInteger, point_dep);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  if (!Qry.Eof) return false;

  return true;
};

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SavePax(reqNode, NULL, resNode);
};

//��楤�� ������ �������� true ⮫쪮 � ⮬ ��砥 �᫨ �ந������� ॠ�쭠� ॣ������
bool CheckInInterface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
  TChangeStatusList ChangeStatusInfo;
  SirenaExchange::TLastExchangeList SirenaExchangeList;
  CheckIn::TAfterSaveInfoList AfterSaveInfoList;
  bool result=SavePax(reqNode, ediResNode, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList);
  if (result)
  {
    if (ediResNode==NULL && !ChangeStatusInfo.empty())
    {
      //��� �� ���� ����� �㤥� ��ࠡ��뢠����
      OraSession.Rollback();  //�⪠�
      ChangeStatusInterface::ChangeStatus(reqNode, ChangeStatusInfo);
      SirenaExchangeList.handle(__FUNCTION__);
      return false;
    }
    else
    {
      SirenaExchangeList.handle(__FUNCTION__);
    };

    AfterSaveInfoList.handle(__FUNCTION__);

    if (!AfterSaveInfoList.empty() &&
        !AfterSaveInfoList.front().segs.empty())
    {
      int grp_id=AfterSaveInfoList.front().segs.front().grp_id;

      if (AfterSaveInfoList.front().action==CheckIn::actionRefreshPaidBagPC)
      {
        EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), reqNode);
      };

      LoadPax(grp_id, resNode, true);
    }
  };
  return result;
};

void CheckIn::TAfterSaveInfo::toLog(const string& where)
{
  TDateTime agent_stat_ondate=NowUTC();
  for(list<TAfterSaveSegInfo>::iterator s=segs.begin(); s!=segs.end(); ++s)
  {
    if (s->point_dep==ASTRA::NoExists)
      throw EXCEPTIONS::Exception("%s: unknown TAfterSaveSegInfo.point_dep!", where.c_str());
    if (s->grp_id==ASTRA::NoExists)
      throw EXCEPTIONS::Exception("%s: unknown TAfterSaveSegInfo.grp_id!", where.c_str());

    UpdGrpToLogInfo(s->grp_id, s->grpInfoAfter);
    TAgentStatInfo agentStat;
    SaveGrpToLog(s->grpInfoBefore, s->grpInfoAfter, handmadeEMDDiff, agentStat);
    recountBySubcls(s->point_dep, s->grpInfoBefore, s->grpInfoAfter);

    if (agent_stat_period==ASTRA::NoExists) continue;
    bool first_segment=(s==segs.begin());

    //ᮡ�ࠥ� �������� ����⨪�
    STAT::agent_stat_delta(segs.front().point_dep,
                           TReqInfo::Instance()->user.user_id,
                           TReqInfo::Instance()->desk.code,
                           agent_stat_ondate,
                           first_segment?(agent_stat_period<1?1:agent_stat_period):0,
                           agentStat.pax_amount,
                           agentStat.dpax_amount,
                           first_segment?STAT::agent_stat_t(0,0):agentStat.dpax_amount,
                           first_segment?agentStat.dbag_amount:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.dbag_weight:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.drk_amount:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.drk_weight:STAT::agent_stat_t(0,0));
  };
}

void CheckIn::TAfterSaveInfoList::handle(const string& where)
{
  if (empty())
    throw EXCEPTIONS::Exception("%s: empty TAfterSaveInfoList!", where.c_str());
  set<int> tckin_ids;
  for(TAfterSaveInfoList::const_iterator i=begin(); i!=end(); ++i)
    if (i->tckin_id!=ASTRA::NoExists) tckin_ids.insert(i->tckin_id);
  CheckTCkinIntegrity(tckin_ids, NoExists);
  for(TAfterSaveInfoList::iterator i=begin(); i!=end(); ++i)
  {
    if (i->segs.empty())
      throw EXCEPTIONS::Exception("%s: empty TAfterSaveInfo.segs!", where.c_str());
    if (i->segs.front().grp_id==ASTRA::NoExists)
      throw EXCEPTIONS::Exception("%s: unknown TAfterSaveSegInfo.grp_id!", where.c_str());
    CheckInInterface::AfterSaveAction(i->segs.front().grp_id, i->action);
    i->toLog(where);
  };
}

struct TSegListItem
{
  TTripInfo flt;
  CheckIn::TPaxGrpItem grp;
  CheckIn::TPaxList paxs;
};
typedef list<TSegListItem> TSegList;

void GetInboundGroupBag(const InboundTrfer::TNewGrpInfo &inbound_trfer,
                        bool inbound_confirmation,
                        TSegList &segList,
                        CheckIn::TGroupBagItem &inbound_group_bag)
{
  inbound_group_bag.clear();
  if (inbound_trfer.pax_map.empty()) return;

  int bag_pool_num=1;
  map<InboundTrfer::TGrpId, int/*bag_pool_num*/> bag_pools;
  for(CheckIn::TPaxList::iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
  {
    if (!p->trferAttachable()) continue;

    pair<string, string> pax_key(p->pax.surname, p->pax.name);
    InboundTrfer::TNewGrpPaxMap::const_iterator iPaxMap=inbound_trfer.pax_map.find(pax_key);
    if (iPaxMap!=inbound_trfer.pax_map.end() && !iPaxMap->second.empty())
    {
      if (iPaxMap->second.size()>1 && !inbound_confirmation)
        throw EXCEPTIONS::Exception("%s: more than one TrferList::TGrpItem for %s/%s found",
                                    __FUNCTION__,
                                    p->pax.surname.c_str(), p->pax.name.c_str());

      set<InboundTrfer::TGrpId>::const_iterator iGrpId=iPaxMap->second.begin();
      for(;iGrpId!=iPaxMap->second.end(); ++iGrpId)
        if (bag_pools.find(*iGrpId)==bag_pools.end()) break;

      if (iGrpId!=iPaxMap->second.end())
      {
        const InboundTrfer::TGrpId &grp_key=*iGrpId;
        //�ਢ�뢠�� ����� ⮫쪮 � ��ࢮ�� ���ᠦ���, ���஬� ᮮ⢥����� grp_key
        //� ��饬 ��砥 ��᪮�쪨� ���ᠦ�ࠬ ����� ᮮ⢥��⢮���� ����� grp_key
        //� �� ���饥 ࠧ��⨥ ��⥬�, ����� �� ᮤ�ন� ��᪮�쪮 ���ᠦ�஢
        InboundTrfer::TNewGrpTagMap::const_iterator iTagMap=inbound_trfer.tag_map.find(grp_key);
        if (iTagMap==inbound_trfer.tag_map.end())
          throw EXCEPTIONS::Exception("%s: TrferList::TGrpItem for %s/%s not found",
                                      __FUNCTION__,
                                      p->pax.surname.c_str(), p->pax.name.c_str());
        CheckIn::TGroupBagItem gbag;
        if (iTagMap->second.first.bag_pool_num!=NoExists)  //�������� ⮣�, ��㤠 �����: �� ⥫��ࠬ� ��� ��ॣ����஢���� � ����
        {
          gbag.fromDB(iTagMap->second.first.grp_id,
                      iTagMap->second.first.bag_pool_num, true);
          gbag.bags.procInboundTrferFromDBTmp();
        }
        else
        {
          gbag.setInboundTrfer(iTagMap->second.first);
          gbag.bags.procInboundTrferFromBTM(iTagMap->second.first);
        };
        if (!gbag.empty())
        {
          p->pax.bag_pool_num=bag_pool_num;
          gbag.setPoolNum(bag_pool_num);
          bag_pools.insert(make_pair(grp_key, bag_pool_num));
          inbound_group_bag.add(gbag);
          bag_pool_num++;
        };
      };
    };
  };

  //��᫥ ⮣� ��� �᪨���� �஢�ਬ, � �� �� �᪨����?
  for(InboundTrfer::TNewGrpPaxMap::const_iterator iPaxMap=inbound_trfer.pax_map.begin();
                                                  iPaxMap!=inbound_trfer.pax_map.end(); ++iPaxMap)
  {
    for(set<InboundTrfer::TGrpId>::const_iterator iGrpId=iPaxMap->second.begin();
                                                  iGrpId!=iPaxMap->second.end(); ++iGrpId)
    {
      if (bag_pools.find(*iGrpId)==bag_pools.end())
      {
        if (inbound_confirmation)
          throw UserException("MSG.PASSENGER.MORE_THAN_ONE_INBOUND_TRFER_CONFIRMED",
                              LParams() << LParam("surname", iPaxMap->first.first+(iPaxMap->first.second.empty()?"":" ")+iPaxMap->first.second));
        else
          throw EXCEPTIONS::Exception("%s: not all tags attached", __FUNCTION__);
      };
    };
  };
};

bool FilterInboundConfirmation(xmlNodePtr trferNode,
                               const InboundTrfer::TNewGrpInfo &inbound_trfer,
                               InboundTrfer::TNewGrpInfo &filtered_trfer)
{
  filtered_trfer.clear();
  if (trferNode==NULL) return false;

  map<TrferList::TGrpId, TrferList::TGrpConfirmItem> grps;
  TrferConfirmFromXML(trferNode, grps);
  map<TrferList::TGrpId, TrferList::TGrpConfirmItem>::const_iterator i1=grps.begin();
  InboundTrfer::TNewGrpTagMap::const_iterator i2=inbound_trfer.tag_map.begin();
  list<TrferList::TGrpId> notCkinIds;
  list< pair<TrferList::TGrpId, int> > bagWeightChanges;
  for(;i1!=grps.end() && i2!=inbound_trfer.tag_map.end(); ++i1, ++i2)
  {
    if (i1->first!=i2->first) return false; //�஢��塞 �� ��㯯� ��ப
    const TrferList::TGrpConfirmItem &grpConfirm=i1->second;
    const TrferList::TGrpItem &grpActual=i2->second.first;

    //�஢�ਬ �����筮��� ���������� ��ப
    vector<string> actual_tag_ranges;
    GetTagRanges(grpActual.tags, actual_tag_ranges);
    sort(actual_tag_ranges.begin(), actual_tag_ranges.end());
    if (grpConfirm.tag_ranges!=actual_tag_ranges) return false;
    //�஢�ਬ �����筮��� ����塞�� ����ᮢ
    int actual_status=inbound_trfer.calc_status(i2->first);
    if (grpConfirm.calc_status!=actual_status) return false;
    //�஢�ਬ ���
    if (grpActual.bag_weight!=NoExists && grpActual.bag_weight!=0 &&
        grpActual.bag_weight!=grpConfirm.bag_weight) return false; //��� ���������
    //�஢�ਬ ���⢥ত���� �����
    if (grpConfirm.conf_status==NoExists) return false; //��㯯� �� ���⢥ত���
    if (actual_status!=NoExists &&
        actual_status!=grpConfirm.conf_status) return false;

    if (grpConfirm.conf_status==0)
    {
      //�� ॣ�����㥬
      notCkinIds.push_back(i2->first); //��ॣ�����㥬� ��㯯� ��ப
    }
    else
    {
      //ॣ�����㥬, �஢�ਬ �� ������ ���
      if (grpConfirm.bag_weight==NoExists || grpConfirm.bag_weight<=0) return false; //��� �� ������
      if (grpActual.bag_weight==NoExists || grpActual.bag_weight==0)
        bagWeightChanges.push_back( make_pair(i2->first, grpConfirm.bag_weight) );
    };
  };
  if (i1!=grps.end() || i2!=inbound_trfer.tag_map.end()) return false;

  //�ᤨ � ���ࠫ���, ����� �� �஢�ਫ� � ����� ᮧ���� filtered_trfer
  filtered_trfer=inbound_trfer;
  for(list<TrferList::TGrpId>::const_iterator i=notCkinIds.begin(); i!=notCkinIds.end(); ++i)
    filtered_trfer.erase(*i);
  for(list< pair<TrferList::TGrpId, int> >::const_iterator i=bagWeightChanges.begin();
                                                           i!=bagWeightChanges.end(); ++i)
  {
    InboundTrfer::TNewGrpTagMap::iterator j=filtered_trfer.tag_map.find(i->first);
    if (j!=filtered_trfer.tag_map.end())
      j->second.first.bag_weight=i->second;
  };
  return true;
};

bool CompareGrpViewItem( const TrferList::TGrpViewItem &item1,
                         const TrferList::TGrpViewItem &item2 )
{
  if ((item1.calc_status==NoExists)!=(item2.calc_status==NoExists))
    return item1.calc_status==NoExists;
  return item1<item2;
};

void GetInboundTransferForTerm(const vector<CheckIn::TTransferItem> &trfer,
                               TSegList &segList,
                               bool pr_unaccomp,
                               InboundTrfer::TNewGrpInfo &inbound_trfer)
{
  inbound_trfer.clear();

  if (segList.empty()) return;

  InboundTrfer::TGrpItem inbound_trfer_grp_out;
  set<int> inbound_trfer_pax_out_ids;
  inbound_trfer_grp_out.airp_arv=segList.begin()->grp.airp_arv;
  inbound_trfer_grp_out.is_unaccomp=pr_unaccomp;
  //��⠭�������� ������� �࠭���
  int seg_no=2;
  for(vector<CheckIn::TTransferItem>::const_iterator t=trfer.begin(); t!=trfer.end(); ++t, seg_no++)
    inbound_trfer_grp_out.trfer.insert(make_pair(seg_no-1, *t));

  for(CheckIn::TPaxList::iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
  {
    if (p->pax.id!=NoExists)
    {
      inbound_trfer_pax_out_ids.insert(p->pax.id);
      if (p->pax.seats==NoExists || p->pax.pers_type==NoPerson)
      {
        //�� ��᫥ �����⢥ত����� ���-ॣ����樨
        QParams QryParams;
        QryParams << QParam("pax_id", otInteger, p->pax.id);
        TCachedQuery Qry("SELECT pers_type, seats FROM pax WHERE pax_id=:pax_id", QryParams);
        Qry.get().Execute();
        if (!Qry.get().Eof)
        {
          if (p->pax.seats==NoExists)
            p->pax.seats=Qry.get().FieldAsInteger("seats");
          if (p->pax.pers_type==NoPerson)
            p->pax.pers_type=DecodePerson(Qry.get().FieldAsString("pers_type"));
        };
      };
    };

    if (!p->trferAttachable()) continue;
    //�� ��� �� � ���⮬:
    inbound_trfer_grp_out.paxs.push_back(InboundTrfer::TPaxItem("", p->pax.surname, p->pax.name));
  };

  //�室�騩 �࠭���
  GetNewGrpInfo(segList.begin()->grp.point_dep,
                inbound_trfer_grp_out,
                inbound_trfer_pax_out_ids,
                inbound_trfer);
};

void GetInboundTransferForWeb(TSegList &segList,
                              bool pr_unaccomp,
                              InboundTrfer::TNewGrpInfo &inbound_trfer,
                              vector<CheckIn::TTransferItem> &inbound_trfer_route)
{
  inbound_trfer.clear();
  inbound_trfer_route.clear();

  if (segList.empty()) return;

  InboundTrfer::TGrpItem inbound_trfer_grp_out;
  set<int> inbound_trfer_pax_out_ids;
  inbound_trfer_grp_out.airp_arv=segList.begin()->grp.airp_arv;
  inbound_trfer_grp_out.is_unaccomp=pr_unaccomp;
  //��⠭�������� ������� �࠭��� �� �᭮�� ᥣ���⮢ ᪢����� ॣ����樨
  int seg_no=1;
  for(TSegList::const_iterator iSegListItem=segList.begin();
      iSegListItem!=segList.end();
      seg_no++,++iSegListItem)
  {
    if (seg_no<=1) continue;
    CheckIn::TTransferItem trfer;
    trfer.operFlt=iSegListItem->flt;
    trfer.operFlt.airline_fmt=efmtCodeNative;
    trfer.operFlt.suffix_fmt=efmtCodeNative;
    trfer.operFlt.airp_fmt=efmtCodeNative;
    trfer.operFlt.scd_out=UTCToLocal(trfer.operFlt.scd_out,AirpTZRegion(trfer.operFlt.airp));
    modf(trfer.operFlt.scd_out,&trfer.operFlt.scd_out);
    trfer.airp_arv=iSegListItem->grp.airp_arv;
    trfer.airp_arv_fmt=efmtCodeNative;
    for(CheckIn::TPaxList::const_iterator p=iSegListItem->paxs.begin(); p!=iSegListItem->paxs.end(); ++p)
    {
      CheckIn::TPaxTransferItem paxTrfer;
      paxTrfer.subclass=p->pax.subcl;
      paxTrfer.subclass_fmt=efmtCodeNative;
      trfer.pax.push_back(paxTrfer);
    };

    inbound_trfer_grp_out.trfer.insert(make_pair(seg_no-1, trfer));
  };

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0";
  Qry.DeclareVariable("pax_id", otInteger);
  TQuery TrferQry(&OraSession);

  set<int> pnr_ids;
  for(CheckIn::TPaxList::const_iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
  {
    if (p->pax.id!=NoExists)
    {
      inbound_trfer_pax_out_ids.insert(p->pax.id);

      Qry.SetVariable("pax_id", p->pax.id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        int pnr_id=Qry.FieldAsInteger("pnr_id");
        if (pnr_ids.insert(pnr_id).second)
        {
          InboundTrfer::TGrpItem tmpGrpItem;
          //���⠥� �࠭���� �������
          CheckInInterface::GetOnwardCrsTransfer(pnr_id,
                                                 TrferQry,
                                                 segList.begin()->flt,
                                                 segList.begin()->grp.airp_arv,
                                                 tmpGrpItem.trfer);
          //�᫨ ���� ࠧ�� �࠭��୮�� ������� - 㡥६ ���� ��᫥ ࠧ�뢠
          //�᫨ ���� ����।������ ���� - 㡥६ ���� � ����
          map<int, CheckIn::TTransferItem>::iterator t=tmpGrpItem.trfer.begin();
          int num=1;
          for(;t!=tmpGrpItem.trfer.end();++t,num++)
            if (num!=t->first ||
                t->second.operFlt.airline_fmt==efmtUnknown ||
                (!t->second.operFlt.suffix.empty() && t->second.operFlt.suffix_fmt==efmtUnknown) ||
                t->second.operFlt.airp_fmt==efmtUnknown ||
                t->second.airp_arv_fmt==efmtUnknown ||
                t->second.subclass_fmt==efmtUnknown) break;
          tmpGrpItem.trfer.erase(t, tmpGrpItem.trfer.end());

          if (inbound_trfer_grp_out.similarTrfer(tmpGrpItem))
          {
            if (tmpGrpItem.trfer.size() > inbound_trfer_grp_out.trfer.size())
            {
              //�������� �������⥫쭮 ���� �࠭��୮�� ������� � ��⠭����� �������ᮢ ��� ���ᠦ�஢
              size_t tmp_size=segList.begin()->paxs.size();
              map<int, CheckIn::TTransferItem>::const_iterator t1=tmpGrpItem.trfer.begin();
              map<int, CheckIn::TTransferItem>::iterator t2=inbound_trfer_grp_out.trfer.begin();
              for(;t1!=tmpGrpItem.trfer.end() && t2!=inbound_trfer_grp_out.trfer.end();++t1,++t2);
              for(;t1!=tmpGrpItem.trfer.end();++t1)
              {
                t2=inbound_trfer_grp_out.trfer.insert(*t1).first;
                CheckIn::TPaxTransferItem paxTrfer;
                paxTrfer.subclass=t1->second.subclass;
                paxTrfer.subclass_fmt=t1->second.subclass_fmt;
                t2->second.pax.assign(tmp_size, paxTrfer);
              };
            };
          }
          else
          {
            inbound_trfer.conflicts.insert(InboundTrfer::conflictOutRouteDiffer);
            break;  //�� ��� ��室 �� ��楤���
          };
        };
      };
    };

    if (!p->trferAttachable()) continue;
    //�� ��� �� � ���⮬:
    inbound_trfer_grp_out.paxs.push_back(InboundTrfer::TPaxItem("", p->pax.surname, p->pax.name));
  };
  //�室�騩 �࠭���
  if (inbound_trfer.conflicts.empty())
  {
    GetNewGrpInfo(segList.begin()->grp.point_dep,
                  inbound_trfer_grp_out,
                  inbound_trfer_pax_out_ids,
                  inbound_trfer);
  };
  if (!inbound_trfer.tag_map.empty() && inbound_trfer.conflicts.empty())
  {
    map<int, CheckIn::TTransferItem>::const_iterator t1=inbound_trfer_grp_out.trfer.begin();
    map<int, CheckIn::TTransferItem>::const_iterator t2=inbound_trfer.tag_map.begin()->second.first.trfer.begin();
    for(;t1!=inbound_trfer_grp_out.trfer.end() && t2!=inbound_trfer.tag_map.begin()->second.first.trfer.end(); ++t1,++t2)
      //楫��⭮��� ������� �஢�ਫ� � GetNewGrpTags
      inbound_trfer_route.push_back(t1->second); //�����, �� ��६ �� �᭮�� inbound_trfer_grp_out � ���������묨 �������ᠬ� ���ᠦ�஢
  };
};

void CheckBagWeightControl(const CheckIn::TBagItem &bag)
{
  //  GRISHA
  const int MAX_WEIGHT_PIECE_BAG = 50;
  const int MAX_WEIGHT_PIECE_CAB = 15;
  //  �����
  if (not bag.pr_cabin and bag.weight > MAX_WEIGHT_PIECE_BAG * bag.amount)
    throw UserException("MSG.LUGGAGE.BAG_EXCEEDS_WEIGHT", LParams() << LParam("maxweight", MAX_WEIGHT_PIECE_BAG));
  //  ��筠� �����
  if (bag.pr_cabin and bag.amount > 1)
    throw UserException("MSG.LUGGAGE.CAB_EXCEEDS_PCS");
  if (bag.pr_cabin and bag.weight > MAX_WEIGHT_PIECE_CAB)
    throw UserException("MSG.LUGGAGE.CAB_EXCEEDS_WEIGHT", LParams() << LParam("maxweight", MAX_WEIGHT_PIECE_CAB));
}

void CheckBagChanges(const TGrpToLogInfo &prev, const CheckIn::TPaxGrpItem &grp)
{
  if (!grp.group_bag) return;

  //  ����஫� ����� ��� ������ � ��筮� �����
  TTripInfo info;
  info.getByPointId(grp.point_dep);
  const bool check_bag_weight = GetTripSets(tsWeightControl, info);

  TReqInfo *reqInfo = TReqInfo::Instance();

  TRFISCListWithPropsCache lists;

  const map< int/*id*/, TEventsBagItem> &bagBefore=prev.bag;
  multimap< int/*id*/, CheckIn::TBagItem> bagAfter;  //����� ����ঠ�� ������⢮ id=NoExists;
  grp.group_bag.get().convertBag(bagAfter);

  std::map< int/*id*/, CheckIn::TBagItem>::const_iterator a=bagAfter.begin();
  std::map< int/*id*/, TEventsBagItem>::const_iterator b=bagBefore.begin();
  for(;a!=bagAfter.end() || b!=bagBefore.end();)
  {
    std::multimap< int/*id*/, CheckIn::TBagItem>::const_iterator aBag=bagAfter.end();
    std::map< int/*id*/, TEventsBagItem>::const_iterator bBag=bagBefore.end();
    if (a==bagAfter.end() ||
        (a!=bagAfter.end() && b!=bagBefore.end() && b->first < a->first))
    {
      //㤠����� �����
      bBag=b;
      ++b;
    } else
    if (b==bagBefore.end() ||
        (a!=bagAfter.end() && b!=bagBefore.end() && a->first < b->first))
    {
      //���������� �����
      aBag=a;
      ++a;
    } else
    if (a!=bagAfter.end() && b!=bagBefore.end() && a->first==b->first)
    {
      if (!a->second.basicallyEqual(b->second))
      {
        //��������� �����
        aBag=a;
        bBag=b;
      };
      ++a;
      ++b;
    };

    if (bBag!=bagBefore.end())
    {
      if (bBag->second.is_trfer && !bBag->second.handmade &&
          !reqInfo->user.access.rights().permitted(347))
        throw UserException("MSG.NO_PERM_MODIFY_INBOUND_TRFER");
    }
    if (aBag!=bagAfter.end())
    {
      aBag->second.check(lists);
      if (check_bag_weight)
        CheckBagWeightControl(aBag->second);
    }
  }
};

//�� ��楤�� ���⠢��� pax_id
void ServicePaymentToDBAdv(int grp_id,
                           bool is_unaccomp,
                           const CheckIn::TServicePaymentList &prior_payment,
                           boost::optional< CheckIn::TServicePaymentList > &curr_payment)
{
  if (!curr_payment) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText= PaxASVCList::GetSQL(PaxASVCList::oneWithTknByGrpId);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  for(CheckIn::TServicePaymentList::iterator i=curr_payment.get().begin(); i!=curr_payment.get().end(); ++i)
  {
    if (!i->isEMD()) continue;
    CheckIn::TServicePaymentItem &item=*i;
    CheckIn::TServicePaymentList::const_iterator j=prior_payment.begin();
    for(; j!=prior_payment.end(); ++j)
      if (j->similar(item)) break;
    if (j!=prior_payment.end() && j->pax_id!=ASTRA::NoExists)
    {
      //EMD 㦥 �뫠 ࠭��
      item.pax_id=j->pax_id;
    }
    else
    {
      //EMD �����
      Qry.SetVariable("emd_no", item.doc_no);
      Qry.SetVariable("emd_coupon", item.doc_coupon);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("MSG.EMD_MANUAL_INPUT_TEMPORARILY_UNAVAILABLE",
                            LParams() << LParam("emd", item.no_str()));
      item.pax_id=Qry.FieldAsInteger("pax_id");
      TPaxEMDItem asvc;
      asvc.fromDB(Qry);
      if (item.trfer_num!=asvc.trfer_num)
        throw UserException("MSG.EMD_WRONG_ATTACHMENT_DIFFERENT_SEG",
                            LParams() << LParam("emd", item.no_str()));
      if (item.pc && item.pc.get().RFISC!=asvc.RFISC)
        throw UserException("MSG.EMD_WRONG_ATTACHMENT_DIFFERENT_RFISC",
                            LParams() << LParam("emd", item.no_str()));
    };
  };
  curr_payment.get().getAllListItems(grp_id, is_unaccomp);
  curr_payment.get().toDB(grp_id);
}

//��楤�� ������ �������� true ⮫쪮 � ⮬ ��砥 �᫨ �ந������� ॠ�쭠� ॣ������
bool CheckInInterface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode,
                               TChangeStatusList &ChangeStatusInfo,
                               SirenaExchange::TLastExchangeList &SirenaExchangeList,
                               CheckIn::TAfterSaveInfoList &AfterSaveInfoList)
{
  AfterSaveInfoList.push_back(CheckIn::TAfterSaveInfo());
  CheckIn::TAfterSaveInfo &AfterSaveInfo=AfterSaveInfoList.back();

  TReqInfo *reqInfo = TReqInfo::Instance();

  map<int,TSegInfo> segs;

  xmlNodePtr segNode=NodeAsNode("segments/segment",reqNode);
  bool only_one=segNode->next==NULL;
  if (reqInfo->client_type == ctPNL && !only_one)
    //��� ctPNL ������� ᪢����� ॣ����樨!
    throw EXCEPTIONS::Exception("SavePax: through check-in not supported for ctPNL");

  bool defer_etstatus=false;

  TQuery Qry(&OraSession);
  if (ediResNode==NULL && reqInfo->client_type == ctTerm) //��� web-ॣ����樨 ��ࠧ���쭮� ���⢥ত���� ��
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

  vector<int> point_ids;
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
    point_ids.push_back(segInfo.point_dep);
  };

  TFlights flightsForLock;
  flightsForLock.Get( point_ids, ftTranzit );
  //����� ३�� ���� �� �����⠭�� poind_dep ���� ����� ���� deadlock
  flightsForLock.Lock();

  for(map<int,TSegInfo>::iterator s=segs.begin();s!=segs.end();++s)
  {
    if (!CheckCkinFlight(s->second.point_dep,
                         s->second.airp_dep,
                         s->second.point_arv,
                         s->second.airp_arv, s->second))
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

  //savepoint ��� �⪠� �� �ॢ�襭�� ����㧪� (����� �� ��᫥ ��窨)
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SAVEPOINT CHECKIN; "
    "END;";
  Qry.Execute();

  bool pr_unaccomp=strcmp((char *)reqNode->name, "TCkinSaveUnaccompBag") == 0;

  //reqInfo->user.check_access(amPartialWrite);
  //��।����, ����� �� ३� ��� ॣ����樨

  segNode=NodeAsNode("segments/segment",reqNode);
  TSegList segList;
  bool new_checkin=false;
  bool save_trfer=false;
  bool need_apps=false;
  vector<CheckIn::TTransferItem> trfer;
  for(bool first_segment=true;
      segNode!=NULL;
      segNode=segNode->next,first_segment=false)
  {
    segList.push_back( TSegListItem() );
    CheckIn::TPaxGrpItem &grp=segList.back().grp;
    CheckIn::TPaxList &paxs=segList.back().paxs;

    if (!grp.fromXML(segNode))
      throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

    if (grp.id!=NoExists && grp.need_upgrade_db)
      UpgradeDBForServices(grp.id);

    if (first_segment)
    {
      grp.fromXMLadditional(reqNode, pr_unaccomp);
      //��।���� - ����� ॣ������ ��� ������ ���������
      new_checkin=(grp.id==NoExists);
    }
    else
    {
      if (new_checkin!=(grp.id==NoExists))
        throw EXCEPTIONS::Exception("CheckInInterface::SavePax: impossible to determine 'new_checkin'");
    };
    grp.status=first_segment?grp.status:psTCheckin;
    grp.hall=segList.front().grp.hall;
    grp.bag_refuse=segList.front().grp.bag_refuse;

    if (!pr_unaccomp)
    {
      xmlNodePtr paxNode=NodeAsNode("passengers",segNode)->children;
      for(; paxNode!=NULL; paxNode=paxNode->next)
        paxs.push_back(CheckIn::TPaxListItem().fromXML(paxNode));
    };

    map<int,TSegInfo>::const_iterator s=segs.find(grp.point_dep);
    if (s==segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SavePax: point_id not found in map segs");

    if (reqInfo->client_type == ctPNL)
    {
      TTripStages tripStages( grp.point_dep );
      TStage ckin_stage =  tripStages.getStage( stCheckIn );
      if (ckin_stage!=sNoActive &&
            ckin_stage!=sPrepCheckIn &&
            ckin_stage!=sOpenCheckIn)
        throw UserException("MSG.REGISTRATION_CLOSED");
    };

    if (first_segment)
    {
      //����稬 �࠭���
      if (new_checkin)
        save_trfer=true;
      else
        save_trfer=GetNode("transfer",reqNode)!=NULL;

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
      };
    };

    segList.back().flt=s->second.fltInfo;

    if (!pr_unaccomp && checkAPPSSets(segList.back().grp.point_dep, segList.back().grp.point_arv))
        need_apps = true;
  };

  //�஢�ਬ, ॣ������ �� �� �����
  if (!segList.empty() && reqInfo->client_type == ctTerm && !pr_unaccomp)
  {
    if (new_checkin)
    {
      bool allPaxNorec=true;
      bool crewRemExists=false;
      for(CheckIn::TPaxList::const_iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
      {
        if (p->pax.id!=NoExists)
          allPaxNorec=false;
        if (!crewRemExists)
        {
          for(multiset<CheckIn::TPaxRemItem>::const_iterator r=p->rems.begin(); r!=p->rems.end(); ++r)
            if (r->code=="CREW")
            {
              crewRemExists=true;
              break;
            };
        };
      };
      if (crewRemExists)
      {
        if (!allPaxNorec)
          throw UserException("MSG.CREW.CANT_CONSIST_OF_BOOKED_PASSENGERS");
        segList.begin()->grp.status=psCrew;
      };
    };
    if (segList.begin()->grp.status==psCrew)
    {
      segList.begin()->grp.cl.clear();
      for(CheckIn::TPaxList::iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
      {
        if ((new_checkin && (p->pax.pers_type!=adult || p->pax.seats!=1)) ||
            (!new_checkin && p->pax.PaxUpdatesPending && p->pax.pers_type!=adult))
          throw UserException("MSG.CREW.MEMBER_IS_ADULT_WITH_ONE_SEAT");
        p->pax.subcl.clear();
        p->pax.tkn.clear();
      };

      if (segList.begin()->grp.group_bag)
      {
        //�஢�ਬ �⮡� ��� ����� �� ॣ����஢���� ����� � ��������
        for(CheckIn::TBagMap::const_iterator b=segList.begin()->grp.group_bag.get().bags.begin();
                                             b!=segList.begin()->grp.group_bag.get().bags.end();++b)
          if (!b->second.pr_cabin)
            throw UserException("MSG.CREW.CAN_CHECKIN_ONLY_CABIN_BAGGAGE");
      };
    };

    if (segList.size()>1 && segList.begin()->grp.status==psCrew)
      throw UserException("MSG.CREW.THROUGH_CHECKIN_NOT_PERFORMED");
  };

  //�஢�ਬ �室�騩 �࠭��� ⮫쪮 �� ��ࢮ��砫쭮� ॣ����樨
  set<InboundTrfer::TConflictReason> inbound_trfer_conflicts;
  CheckIn::TGroupBagItem inbound_group_bag;
  vector<CheckIn::TTransferItem> inbound_trfer_route;

  if (!segList.empty() && new_checkin &&
      reqInfo->client_type != ctTerm &&
      segList.begin()->grp.status!=psCrew)
  {
    InboundTrfer::TNewGrpInfo info;
    GetInboundTransferForWeb(segList,
                             pr_unaccomp,
                             info,
                             inbound_trfer_route);
    if (!info.tag_map.empty() && info.conflicts.empty())
    {
      GetInboundGroupBag(info,
                         false,
                         segList,
                         inbound_group_bag);
    };

    inbound_trfer_conflicts=info.conflicts;
    if (save_trfer && !inbound_group_bag.empty())
    {
      //�ਢ離� �室�饣� �࠭��� ��� ��� � ���� ॣ����樨
      trfer=inbound_trfer_route;
    };
  };

  bool inbound_confirm=false;
  if (!segList.empty() && save_trfer &&
      reqInfo->client_type == ctTerm &&
      segList.begin()->grp.status!=psCrew)
  {
    //�஢�ਬ �室�騩 �࠭���
    InboundTrfer::TNewGrpInfo info;
    GetInboundTransferForTerm(trfer,
                              segList,
                              pr_unaccomp,
                              info);

    if (!info.tag_map.empty())
    {
      if (reqInfo->desk.compatible(INBOUND_TRFER_VERSION))
      {
        try
        {
          InboundTrfer::TNewGrpInfo filtered_info;
          //���� �࠭���� ��ન
          xmlNodePtr confirmReqNode=GetNode("inbound_confirmation", reqNode);
          if (!FilterInboundConfirmation(confirmReqNode, info, filtered_info))
          {
            if (confirmReqNode!=NULL)
              AstraLocale::showErrorMessage( "MSG.CHANGE_INBOUND_TRFER_MUST_REVERIFY" );
            throw UserException2();
          };

          try
          {
            GetInboundGroupBag(filtered_info,
                               true,
                               segList,
                               inbound_group_bag);
          }
          catch(UserException &e)
          {
            AstraLocale::showErrorMessage(e.getLexemaData());
            throw UserException2();
          };
        }
        catch(UserException2)
        {
          XMLRequestCtxt *xmlRC = getXmlCtxt();
          if (xmlRC->resDoc==NULL) throw EXCEPTIONS::Exception("CheckInInterface::SavePax: xmlRC->resDoc=NULL;");
          xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);

          vector<TrferList::TGrpViewItem> grps;
          NewGrpInfoToGrpsView(info, grps);
          sort(grps.begin(), grps.end(), CompareGrpViewItem);
          TrferToXML(TrferList::trferOutForCkin,
                     reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS),
                     grps,
                     NewTextChild(resNode, "inbound_confirmation"));
          throw; //�� �����, �⮡� � ����� ��஭� ������ �⪠� �� � � ��㣮� ��஭� �� ������ � xml inbound_confirmation
        };
      }
      else
      {
        if (info.conflicts.empty())
        {
          GetInboundGroupBag(info,
                             false,
                             segList,
                             inbound_group_bag);
        };
        inbound_trfer_conflicts=info.conflicts;
      };
    };
    inbound_confirm=reqInfo->desk.compatible(INBOUND_TRFER_VERSION);
  };

  if (!inbound_group_bag.empty())
  {
    for(CheckIn::TBagMap::iterator b=inbound_group_bag.bags.begin();
                                   b!=inbound_group_bag.bags.end(); ++b)
    {
      b->second.to_ramp=false;
      b->second.is_trfer=true;
      b->second.handmade=false;
    };
    for(map<int, CheckIn::TTagItem>::iterator t=inbound_group_bag.tags.begin();
                                              t!=inbound_group_bag.tags.end(); ++t)
    {
      if (t->second.printable) t->second.pr_print=true;
    };
  };

  segNode=NodeAsNode("segments/segment",reqNode);
  int seg_no=1;
  bool first_segment=true;
  bool notCheckAPI=false;
  vector<CheckIn::TTransferItem>::const_iterator iTrfer;
  map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;

  for(TSegList::iterator iSegListItem=segList.begin();
      segNode!=NULL && iSegListItem!=segList.end();
      segNode=segNode->next,seg_no++,++iSegListItem,first_segment=false)
  {
    CheckIn::TPaxGrpItem &grp=iSegListItem->grp;
    CheckIn::TPaxList &paxs=iSegListItem->paxs;

    TAnnulBT annul_bt;
    annul_bt.get(grp.id);

    map<int,TSegInfo>::const_iterator s=segs.find(grp.point_dep);
    if (s==segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SavePax: point_id not found in map segs");

    const TTripInfo &fltInfo=s->second.fltInfo;

    if (first_segment &&
        grp.status!=psCrew &&
        reqInfo->client_type==ctTerm &&
        reqInfo->desk.compatible(AGENT_STAT_VERSION))
      AfterSaveInfo.agent_stat_period=NodeAsInteger("agent_stat_period",reqNode);
    AfterSaveInfo.segs.push_back(CheckIn::TAfterSaveSegInfo());
    AfterSaveInfo.segs.back().point_dep=grp.point_dep;
    TGrpToLogInfo &grpInfoBefore=AfterSaveInfo.segs.back().grpInfoBefore;
    TGrpToLogInfo &grpInfoAfter=AfterSaveInfo.segs.back().grpInfoAfter;
    CheckIn::TPaidBagEMDProps &handmadeEMDDiff=AfterSaveInfo.handmadeEMDDiff;
    TTripInfo markFltInfo=fltInfo;
    markFltInfo.scd_out=UTCToLocal(markFltInfo.scd_out,AirpTZRegion(markFltInfo.airp));
    modf(markFltInfo.scd_out,&markFltInfo.scd_out);
    bool pr_mark_norms=false;

    if ( need_apps ) {
      TAdvTripRoute route;
      route.GetRouteAfter(NoExists, grp.point_dep, trtWithCurrent, trtNotCancelled);
      if ( route.front().act_out != NoExists )
        throw UserException( "MSG.PASSENGER.CHANGES_NOT_PERMITTED" );
    }

    try
    {
      TAdvTripInfo fltAdvInfo(fltInfo,
                              s->second.point_dep,
                              s->second.point_num,
                              s->second.first_point,
                              s->second.pr_tranzit);
      //BSM
      BSM::TBSMAddrs BSMaddrs;
      BSM::TTlgContent BSMContentBefore;
      bool BSMsend=grp.status==psCrew?false:
                                      BSM::IsSend(fltAdvInfo, BSMaddrs);

      set<int> nextTrferSegs;

      TQuery CrsQry(&OraSession);
      CrsQry.Clear();
      CrsQry.SQLText=
        "BEGIN "
        "  ckin.save_pax_docs(:pax_id, :document, :full_insert); "
        "END;";
      CrsQry.DeclareVariable("pax_id",otInteger);
      CrsQry.DeclareVariable("document",otString);
      CrsQry.DeclareVariable("full_insert",otInteger);

      TQuery PaxDocQry(&OraSession);
      TQuery PaxDocoQry(&OraSession);
      TQuery PaxDocaQry(&OraSession);

      Qry.Clear();
      Qry.SQLText=
        "SELECT pr_tranz_reg, pr_etstatus "
        "FROM trip_sets WHERE point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,grp.point_dep);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB

      TTripSetList setList;
      setList.fromDB(grp.point_dep);
      if (setList.empty())
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB

      bool pr_tranz_reg=!Qry.FieldIsNULL("pr_tranz_reg")&&Qry.FieldAsInteger("pr_tranz_reg")!=0;
      int pr_etstatus=Qry.FieldAsInteger("pr_etstatus");
      bool pr_etl_only=GetTripSets(tsETSNoInteract,fltInfo);
      bool pr_mintrans_file=GetTripSets(tsMintransFile,fltInfo);
      bool pr_mixed_norms=GetTripSets(tsMixedNorms,fltInfo);
      if (first_segment)
        notCheckAPI=GetTripSets(tsAPISControlOnFirstSegOnly, fltInfo);

      bool addVIP=false;
      if (first_segment)
      {
        if (grp.status==psTransit && !pr_tranz_reg)
          throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");

        if (reqInfo->client_type == ctTerm)
        {
          Qry.Clear();
          Qry.SQLText="SELECT pr_vip FROM halls2 WHERE id=:hall";
          Qry.CreateVariable("hall",otInteger,grp.hall);
          Qry.Execute();
          if (Qry.Eof) throw UserException("MSG.CHECKIN.INVALID_HALL");
          addVIP=Qry.FieldAsInteger("pr_vip")!=0;
        };
      };

      //�஢�ਬ ����� ���㬥�⮢ � ����⮢, ६�ન
      if (!pr_unaccomp)
      {
        TCompleteAPICheckInfo checkInfo;
        if (first_segment || !notCheckAPI)
          checkInfo.set(grp.point_dep, grp.airp_arv);
        Qry.Clear();
        Qry.SQLText="SELECT pax.*, NULL AS seat_no FROM pax WHERE pax_id=:pax_id";
        Qry.DeclareVariable("pax_id", otInteger);
        for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p)
        {
          CheckIn::TPaxItem &pax=p->pax;
          try
          {
            CheckIn::TSimplePaxItem priorPax;
            if (!new_checkin)
            {
              Qry.SetVariable("pax_id",pax.id);
              Qry.Execute();
              if (Qry.Eof) throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
              priorPax.fromDB(Qry);
              pax.crew_type=priorPax.crew_type;
            };

            if (pax.name.empty() && pr_mintrans_file)
              throw UserException("MSG.CHECKIN.PASSENGERS_NAMES_NOT_SET");

            if (pax.tkn.rem=="TKNE" && !pr_etl_only && pr_etstatus>=0) //�� �� � ���� ��� � ���
            {
              if (!pax.tkn.equalAttrs(priorPax.tkn))
              {
                //��ࢮ��砫쭠� ॣ������ ��� ����� �⫨砥��� �� ࠭�� ����ᠭ����
                if (pax.tkn.no.empty())
                  throw UserException("MSG.ETICK.NUMBER_NOT_SET");
                if (pax.tkn.coupon==NoExists)
                  throw UserException("MSG.ETICK.COUPON_NOT_SET", LParams()<<LParam("etick", pax.tkn.no ) );

                if (reqInfo->desk.compatible(DEFER_ETSTATUS_VERSION) &&
                    defer_etstatus && !pax.tkn.confirm)
                  //�������� �� �ந��諮 � ���樨, ����� ��������� � ���� defer_etstatus � true,
                  //� ���� �� �ᯥ� ������� ��� ����ன��
                  throw UserException("MSG.ETICK.NOT_CONFIRM.NEED_RELOGIN");

                TETickItem etick;
                etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
                if (etick.empty())
                  throw UserException("MSG.ETICK.NEED_DISPLAY", LParams()<<LParam("etick", pax.tkn.no_str()));
              };
            };
            //�஢�ઠ refusability ��� ���᪮� � ����
            if (!new_checkin && pax.PaxUpdatesPending &&
                !pax.refuse.empty() &&
                !CheckRefusability(grp.point_dep, pax.id))
              throw UserException("MSG.PASSENGER.UNREGISTRATION_DENIAL");

            //�����
            if (pax.TknExists)
            {
              if (setList.value(tsRegWithoutTKNA) &&
                  pax.tkn.rem!="TKNE" &&
                  !pax.tkn.equalAttrs(priorPax.tkn))
                throw UserException("MSG.CHECKIN.TKNA_DENIAL");

              if (pax.tkn.no.size()>15)
              {
                string ticket_no=pax.tkn.no;
                if (ticket_no.size()>20) ticket_no.erase(20).append("...");
                throw UserException("MSG.CHECKIN.TICKET_LARGE_MAX_LEN", LParams()<<LParam("ticket_no",ticket_no));
              };
              if (pax.refuse.empty())
              {
                if (checkInfo.incomplete(pax.tkn, grp.status))
                  throw UserException("MSG.CHECKIN.PASSENGERS_TICKETS_NOT_SET"); //WEB
              };

              //�஢��塞 䠬���� � �����
              if (!pax.tkn.no.empty() && !pax.surname.empty() && pax.api_doc_applied())
              {
                list<TETickItem> eticks;
                TETickItem::fromDB(pax.tkn.no, TETickItem::Display, eticks);
                if (!eticks.empty() && !eticks.front().surname.empty())
                {
                  string tick_surname=eticks.front().surname;
                  //㤠�塞 �஡��� �� ����⭮� 䠬����
                  for(string::iterator i=tick_surname.begin(); i!=tick_surname.end();)
                    if (*i==' ') i=tick_surname.erase(i); else ++i;
                  if (best_transliter_similarity(pax.surname, tick_surname)<70)
                    throw UserException("MSG.CHECKIN.TICKET_SURNAME_DIFFERS_FROM_PAX_SURNAME",
                                        LParams()<<LParam("ticket_no", pax.tkn.no_str()));
                }
              };
            };

            TDateTime checkDate=UTCToLocal(NowUTC(), AirpTZRegion(grp.airp_dep));
            //���㬥��
            if (pax.DocExists)
              HandleDoc(grp, pax, checkInfo, checkDate, pax.doc);
            //����
            if (pax.DocoExists)
              HandleDoco(grp, pax, checkInfo, checkDate, pax.doco);

            if (pax.DocaExists)
              HandleDoca(grp, pax, checkInfo, pax.doca);

            if (new_checkin || p->remsExists)
            {
              multiset<CheckIn::TPaxRemItem> &rems=p->rems;

              bool flagVIP=false,
                   flagSTCR=false,
                   flagEXST=false;
              for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end();)
              {
                TRemCategory cat=getRemCategory(r->code, r->text);

                if (r->code=="VIP")  flagVIP=true;
                if (r->code=="STCR") flagSTCR=true;
                if (r->code=="EXST") flagEXST=true;

                if (!(reqInfo->client_type==ctTerm && reqInfo->desk.compatible(FQT_TIER_LEVEL_VERSION)))
                {
                  //�஢�ਬ ���४⭮��� ६�ન FQT...
                  CheckIn::TPaxFQTItem fqt;
                  if (CheckFQTRem(*r,fqt)) //�� ��楤�� ��ଠ����� ६�ન FQT
                  {
                    p->addFQT(fqt);
                    r=Erase(rems, r);
                    continue;
                  };
                };
                //�஢�ਬ ����饭�� ��� ����� ६�ન...
                if (isDisabledRemCategory(cat))
                  throw UserException("MSG.REMARK.INPUT_CODE_DENIAL",
                                      LParams() << LParam("remark", r->code.empty()?r->text.substr(0,5):r->code));
                ++r;
              }
              //ᨭ�஭����� tier_level ��� FQT �� ����� � ����� �ନ�����
              CheckIn::SyncFQTTierLevel(pax.id, new_checkin, p->fqts);
              //�஢�ઠ readonly-६�ப
              multiset<CheckIn::TPaxRemItem> prior_rems;
              CheckIn::PaxRemAndASVCFromDB(pax.id, new_checkin, prior_rems);
              multiset<CheckIn::TPaxRemItem> added;
              multiset<CheckIn::TPaxRemItem> deleted;
              CheckIn::GetPaxRemDifference(boost::none, prior_rems, rems, added, deleted);
              for(int pass=0; pass<2; pass++)
              {
                for(multiset<CheckIn::TPaxRemItem>::const_iterator r=(pass==0?added:deleted).begin();
                                                                   r!=(pass==0?added:deleted).end(); ++r)
                {
                  if (r->code.empty()) continue;
                  TRemCategory cat=getRemCategory(r->code, r->text);
                  if (cat==remASVC) continue; //�ய�᪠�� ASVC
                  if (IsReadonlyRem(r->code, r->text))
                    throw UserException(pass==0?"MSG.REMARK.ADD_OR_CHANGE_DENIAL":"MSG.REMARK.CHANGE_OR_DEL_DENIAL",
                                        LParams() << LParam("remark", r->code.empty()?r->text.substr(0,5):r->code));
                };
              };

              p->checkCrewType(new_checkin, grp.status);

              //���⠭���� ६�ப VIP,EXST, �᫨ �㦭�
              //����� ६�ப FQT
              int seats=pax.seats!=NoExists?pax.seats:priorPax.seats;

              if (new_checkin && addVIP && !flagVIP)
                rems.insert(CheckIn::TPaxRemItem("VIP","VIP"));

              if (seats!=NoExists && seats>1 && !flagEXST && !flagSTCR)
                rems.insert(CheckIn::TPaxRemItem("EXST","EXST"));

              if (flagEXST && ((seats!=NoExists && seats<=1) || flagSTCR))
                for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end();)
                {
                  if (r->code=="EXST") r=Erase(rems, r); else ++r;
                };
            };
            if (p->pax.crew_type!=TCrewType::Unknown)
            {
              if ((new_checkin && (p->pax.pers_type!=adult || p->pax.seats<1)) ||
                  (!new_checkin && p->pax.PaxUpdatesPending && p->pax.pers_type!=adult))
                throw UserException("MSG.PASSENGER.IS_ADULT_WITH_AT_LEAST_ONE_SEAT");
            };
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            if (pax.id!=NoExists)
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            else
              throw;
          };
        };
      };

      if (reqInfo->client_type == ctTerm && grp.hall==NoExists)
        throw EXCEPTIONS::Exception("CheckInInterface::SavePax: grp.hall not defined");

      //�࠭���
      if (first_segment)
      {
        if (save_trfer)
        {
          if (!trfer.empty())
          {
            map<int, CheckIn::TTransferItem> trfer_tmp;
            int trfer_num=1;
            for(vector<CheckIn::TTransferItem>::const_iterator t=trfer.begin();t!=trfer.end();++t, trfer_num++)
              trfer_tmp[trfer_num]=*t;
            traceTrfer(TRACE5, "SavePax: trfer_tmp", trfer_tmp);
            GetTrferSets(fltInfo, grp.airp_arv, "", trfer_tmp, true, trfer_segs);
            traceTrfer(TRACE5, "SavePax: trfer_segs", trfer_segs);
          };
        }
        else
        {
          //�������� trfer �� ����
          CheckIn::LoadTransfer(grp.id, trfer);
        };
        iTrfer=trfer.begin();
      }
      else
      {
        if (iTrfer!=trfer.end())
        {
          TDateTime scd_local=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));
          modf(scd_local,&scd_local); //���㡠�� ���

          //�஢�ਬ ᮢ������� ᥣ���⮢ �࠭��� � ᪢����� ॣ����樨
          if (iTrfer->operFlt.airline!=fltInfo.airline ||
              iTrfer->operFlt.flt_no!=fltInfo.flt_no ||
              iTrfer->operFlt.suffix!=fltInfo.suffix ||
              iTrfer->operFlt.scd_out!=scd_local || //fltInfo.scd_out �ਢ���� � �����쭮� ���
              iTrfer->operFlt.airp!=fltInfo.airp ||
              iTrfer->airp_arv!=grp.airp_arv)
            throw UserException("MSG.CHECKIN.DIFFERENT_TCKIN_AND_TRFER_ROUTES");
          if (!pr_unaccomp && save_trfer)
          {
            //�஢�ਬ ���������
            CheckIn::TPaxList::const_iterator p=paxs.begin();
            vector<CheckIn::TPaxTransferItem>::const_iterator iPaxTrfer=iTrfer->pax.begin();
            for(;p!=paxs.end()&&iPaxTrfer!=iTrfer->pax.end(); ++p,++iPaxTrfer)
            {
              const CheckIn::TPaxItem &pax=p->pax;
              try
              {
                if (iPaxTrfer->subclass!=pax.subcl)
                  throw UserException("MSG.CHECKIN.DIFFERENT_TCKIN_AND_TRFER_SUBCLASSES");
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                if (pax.id!=NoExists)
                  throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
                else
                  throw;
              };
            };
            if (p!=paxs.end() || iPaxTrfer!=iTrfer->pax.end())
              throw EXCEPTIONS::Exception("SavePax: Different number of transfer subclasses and passengers");
          };
          iTrfer++;
        };
      };

      if (!trfer.empty() && grp.status==psCrew)
        throw UserException("MSG.CREW.TRANSFER_CHECKIN_NOT_PERFORMED");

      bool needCheckUnattachedTrferAlarm=need_check_u_trfer_alarm_for_grp(grp.point_dep);
      map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> grpTagsBefore;
      CheckIn::TServicePaymentList paymentBefore;
      bool first_pax_on_flight = false;
      bool isTranzitSalonsVersion = SALONS2::isTranzitSalons( grp.point_dep );
      bool pr_do_check_wait_list_alarm = true;
      std::set<int> paxs_external_logged;


      if (new_checkin)
      {
        SALONS2::TSalonList salonList;
        SALONS2::TAutoSeats autoSeats;

        //����� ॣ������
        string wl_type=NodeAsString("wl_type",segNode);
        int first_reg_no=NoExists;
        bool pr_lat_seat=false;
        if (!pr_unaccomp)
        {
          //�஢�ઠ ���稪��
          if (wl_type.empty() && grp.status!=psCrew)
          {
            //������ ��饣� ���-�� ����, �ॡ㥬�� ��㯯�
            int seats_sum=0;
            for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p) seats_sum+=p->pax.seats;
            //�஢�ઠ ������ ᢮������ ����
            int free=CheckCounters(grp.point_dep,grp.point_arv,grp.cl,grp.status,TCFG(grp.point_dep),setList.value(tsFreeSeating));
            if (free!=NoExists && free<seats_sum)
              throw UserException("MSG.CHECKIN.AVAILABLE_SEATS",
                                  LParams()<<LParam("count",free)); //WEB

            if (setList.value(tsFreeSeating)) wl_type="F";
          };

          if (grp.status!=psCrew)
          {
            //����襬 �������᪨� ३�
            xmlNodePtr markFltNode=GetNode("mark_flight",segNode);
            if (markFltNode!=NULL)
            {
              TGrpMktFlight mktFlight;
              mktFlight.fromXML(markFltNode);
              markFltInfo.Init(mktFlight);
              pr_mark_norms=mktFlight.pr_mark_norms;
              //����稬 �������� ���� �믮������ 䠪��᪮�� ३�
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

          if (wl_type.empty() && grp.status!=psCrew)
          {
            //��㯯� ॣ��������� �� �� ���� ��������

            //ࠧ��⪠ ��⥩ �� �����
            vector<TInfantAdults> InfItems, AdultItems;
            Qry.Clear();
            Qry.SQLText = "SELECT pax_id FROM crs_inf WHERE inf_id=:inf_id";
            Qry.DeclareVariable("inf_id", otInteger);
            for(int k=0;k<=1;k++)
            {
              int pax_no=1;
              for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
              {
                const CheckIn::TPaxItem &pax=p->pax;
                if ((pax.seats<=0&&k==0)||(pax.seats>0&&k==1)) continue;
                TInfantAdults pass;       // infant �� ⠡���� crs_pax, pax, crs_inf
                pass.grp_id = 1;
                if ( pax.id!=NoExists )
                  pass.pax_id = pax.id; //NULL - �� �� �஭�஢���� - norec
                else
                  pass.pax_id = 0 - pax_no; // ��� 㭨���쭮��
                pass.reg_no = pax_no;
                pass.surname = pax.surname;
                if (pax.seats<=0)
                {
                  // ��� ����
                  if ( pax.id!=NoExists )
                  {
                    Qry.SetVariable("inf_id", pax.id);
                    Qry.Execute();
                    if ( !Qry.Eof )
                      pass.parent_pax_id = Qry.FieldAsInteger( "pax_id" );
                  };
                  InfItems.push_back( pass );
                }
                else
                {
                  if (pax.pers_type == ASTRA::adult) AdultItems.push_back( pass );
                };                                                
              };
            }
            SALONS2::TSeatTariffMap tariffMap;
            //ProgTrace( TRACE5, "InfItems.size()=%zu, AdultItems.size()=%zu", InfItems.size(), AdultItems.size() );
            SetInfantsToAdults( InfItems, AdultItems );
            SEATS2::Passengers.Clear();
            SEATS2::TSublsRems subcls_rems( fltInfo.airline );
            // ���⪠ ᠫ���
            SALONS2::TSalons Salons( grp.point_dep, SALONS2::rTripSalons );
            if ( isTranzitSalonsVersion ) {
              salonList.ReadFlight( SALONS2::TFilterRoutesSets( grp.point_dep, grp.point_arv ), SALONS2::rfTranzitVersion, grp.cl, ASTRA::NoExists );
            }
            else {
              Salons.FilterClass = grp.cl;
              Salons.Read();
            }
            //�������� ���ᨢ ��� ��ᠤ��
            for(int k=0;k<=1;k++)
            {
              int pax_no=1;
              for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
              {
                const CheckIn::TPaxItem &pax=p->pax;
                try
                {
                  if (pax.seats<=0||(pax.seats>0&&k==1)) continue;
                  SEATS2::TPassenger pas;

                  pas.clname=grp.cl;
                  int pax_id;
                  if ( pax.id!=NoExists ) {
                    pax_id = pax.id; //NULL - �� �� �஭�஢���� - norec
                  }
                  else {
                    pax_id = 0 - pax_no; // ��� 㭨���쭮��
                  }
                  pas.paxId = pax_id;
                  switch ( grp.status )  {
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
                    case psCrew:
                      throw EXCEPTIONS::Exception("SavePax: Not applied for crew");
                  }
                  pas.preseat_no=pax.seat_no; // crs or hand made
                  pas.countPlace=pax.seats;
                  pas.placeRem=pax.seat_type;
                  pas.pers_type = EncodePerson(pax.pers_type);
                  bool flagCHIN=pax.pers_type != ASTRA::adult;
                  bool flagINFT = false;
                  for(multiset<CheckIn::TPaxRemItem>::const_iterator r=p->rems.begin(); r!=p->rems.end(); ++r)
                  {
                    if (r->code=="BLND" ||
                        r->code=="STCR" ||
                        r->code=="UMNR" ||
                        r->code=="WCHS" ||
                        r->code=="MEDA") flagCHIN=true;
                    pas.add_rem(r->code);
                  };
                  string pass_rem;
                  if ( subcls_rems.IsSubClsRem( pax.subcl, pass_rem ) )  pas.add_rem(pass_rem);
                  for ( vector<TInfantAdults>::iterator infItem=InfItems.begin(); infItem!= InfItems.end(); infItem++ ) {
                    if ( infItem->parent_pax_id == pax_id ) {
                      //ProgTrace( TRACE5, "infItem->parent_pax_id=%d", infItem->parent_pax_id );
                      flagCHIN = true;
                      flagINFT = true;
                      break;
                    }
                  }
                  if ( flagCHIN ) {
                    pas.add_rem("CHIN");
                  }
                  if ( flagINFT ) {
                    pas.add_rem("INFT");
                  }

                  //����� ����ࠥ�
                  if ( SALONS2::selfckin_client() ) {
                    tariffMap.get_rfisc_colors( fltAdvInfo.airline );
                    SALONS2::addAirlineSelfCkinTariff( fltAdvInfo.airline, tariffMap );
                  }
                  else {
                    tariffMap.get(fltAdvInfo, markFltInfo, pax.tkn);
                  }
                  pas.tariffs=tariffMap;
                  pas.tariffStatus = tariffMap.status();
                  tariffMap.trace(TRACE5);

                  if ( isTranzitSalonsVersion ) {
                    SEATS2::Passengers.Add(salonList,pas);
                  }
                  else {
                    SEATS2::Passengers.Add(Salons,pas);
                  }
                }
                catch(CheckIn::UserException)
                {
                  throw;
                }
                catch(UserException &e)
                {
                  if (pax.id!=NoExists)
                    throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
                  else
                    throw;
                };
              };
            };
            //��।���� ������ ��ᠤ��
            SEATS2::TSeatAlgoParams algo=SEATS2::GetSeatAlgo(Qry,fltInfo.airline,fltInfo.flt_no,fltInfo.airp);
            //��ᠤ��
            if ( isTranzitSalonsVersion ) { //!!!djek
              SEATS2::SeatsPassengers( salonList, algo, reqInfo->client_type, SEATS2::Passengers, autoSeats );
              pr_do_check_wait_list_alarm = salonList.check_waitlist_alarm_on_tranzit_routes( autoSeats );
              //!!! ������ True - �������� ��ᠤ�� �� ���஭�஢���� ����, �����
              // ���� �ࠢ� �� ॣ������, ����� ३� ����砭��, ���� �ࠢ� ᠦ��� �� �㦨� ���஭. ����
              pr_lat_seat=salonList.isCraftLat();
            }
            else {
            //!!! ������ True - �������� ��ᠤ�� �� ���஭�஢���� ����, �����
            // ���� �ࠢ� �� ॣ������, ����� ३� ����砭��, ���� �ࠢ� ᠦ��� �� �㦨� ���஭. ����
              SEATS2::SeatsPassengers( &Salons, algo, reqInfo->client_type, SEATS2::Passengers );
              pr_lat_seat=Salons.getLatSeat();
            }
          }; //if (wl_type.empty())

          if (reqInfo->client_type!=ctPNL)
          {
            //����稬 ���� ॣ. �����
            Qry.Clear();
            if (grp.status!=psCrew)
              Qry.SQLText=
                "SELECT NVL(MAX(reg_no)+1,1) AS reg_no FROM pax_grp,pax "
                "WHERE pax_grp.grp_id=pax.grp_id AND reg_no>0 AND point_dep=:point_dep ";
            else
              Qry.SQLText=
                "SELECT NVL(MIN(reg_no)-1,-1) AS reg_no FROM pax_grp,pax "
                "WHERE pax_grp.grp_id=pax.grp_id AND reg_no<0 AND point_dep=:point_dep ";
            Qry.CreateVariable("point_dep",otInteger,grp.point_dep);
            Qry.Execute();
            first_reg_no = Qry.FieldAsInteger("reg_no");
            first_pax_on_flight = first_reg_no==1;
            //�������� pax.reg_no ॣ����樮��묨 ����ࠬ� �� ���浪�
            int reg_no=first_reg_no;
            for(int k=0;k<=1;k++)
            {
              for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p)
              {
                CheckIn::TPaxItem &pax=p->pax;
                if ((pax.seats<=0&&k==0)||(pax.seats>0&&k==1)) continue;
                pax.reg_no=reg_no;
                if (grp.status!=psCrew)
                  reg_no++;
                else
                  reg_no--;
              }
            };
          }
          else
          {
            if (grp.status!=psCrew)
            {
              Qry.Clear();
              Qry.SQLText=
                "SELECT reg_no FROM pax_grp,pax "
                "WHERE pax_grp.grp_id=pax.grp_id AND reg_no>0 AND point_dep=:point_dep ";
              Qry.CreateVariable("point_dep",otInteger,grp.point_dep);
              Qry.Execute();
              set<int> checked_reg_no;
              for(;!Qry.Eof;Qry.Next())
                checked_reg_no.insert(Qry.FieldAsInteger("reg_no"));
              first_pax_on_flight = checked_reg_no.empty();
              //�஢�ਬ 㭨���쭮���
              set<int> reg_no_with_seats;
              set<int> reg_no_without_seats;
              for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p)
              {
                CheckIn::TPaxItem &pax=p->pax;
                if (pax.reg_no==NoExists) throw EXCEPTIONS::Exception("SavePax: empty seat_no");
                if ((pax.seats>0 && !reg_no_with_seats.insert(pax.reg_no).second) ||
                    (pax.seats<=0 && !reg_no_without_seats.insert(pax.reg_no).second))
                {
                  ostringstream reg_no_str;
                  reg_no_str << setw(3) << setfill('0') << pax.reg_no;
                  throw UserException("MSG.CHECKIN.DUPLICATE_REG_NO", LParams() << LParam("reg_no", reg_no_str.str()));
                };
              };
              if (!checked_reg_no.empty() && !reg_no_with_seats.empty())
              {
                set<int>::const_iterator i=find_first_of(checked_reg_no.begin(), checked_reg_no.end(),
                                                         reg_no_with_seats.begin(), reg_no_with_seats.end());
                if (i!=checked_reg_no.end())
                {
                  ostringstream reg_no_str;
                  reg_no_str << setw(3) << setfill('0') << *i;
                  throw UserException("MSG.CHECKIN.DUPLICATE_REG_NO", LParams() << LParam("reg_no", reg_no_str.str()));
                };
              };
              if (!checked_reg_no.empty() && !reg_no_without_seats.empty())
              {
                set<int>::const_iterator i=find_first_of(checked_reg_no.begin(), checked_reg_no.end(),
                                                         reg_no_without_seats.begin(), reg_no_without_seats.end());
                if (i!=checked_reg_no.end())
                {
                  ostringstream reg_no_str;
                  reg_no_str << setw(3) << setfill('0') << *i;
                  throw UserException("MSG.CHECKIN.DUPLICATE_REG_NO", LParams() << LParam("reg_no", reg_no_str.str()));
                };
              };
            }
            else
              throw UserException("MSG.CREW.CANT_CONSIST_OF_BOOKED_PASSENGERS");
          };
        }; //if (!pr_unaccomp)

        Qry.Clear();
        Qry.SQLText=
          "DECLARE "
          "  pass BINARY_INTEGER; "
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
          "    status,excess,excess_wt,excess_pc,hall,bag_refuse,trfer_confirm,user_id,desk,time_create,client_type, "
          "    point_id_mark,pr_mark_norms,trfer_conflict,inbound_confirm,tid) "
          "  VALUES(:grp_id,:point_dep,:point_arv,:airp_dep,:airp_arv,:class, "
          "    :status,0,0,0,:hall,0,:trfer_confirm,:user_id,:desk,:time_create,:client_type, "
          "    :point_id_mark,:pr_mark_norms,:trfer_conflict,:inbound_confirm,cycle_tid__seq.nextval); "
          "  IF :seg_no IS NOT NULL THEN "
          "    IF :seg_no=1 THEN :tckin_id:=:grp_id; END IF; "
          "    INSERT INTO tckin_pax_grp(tckin_id,seg_no,grp_id,first_reg_no,pr_depend) "
          "    VALUES(:tckin_id,:seg_no,:grp_id,:first_reg_no,DECODE(:seg_no,1,0,1)); "
          "  END IF; "
          "END;";
        Qry.DeclareVariable("grp_id",otInteger);
        Qry.DeclareVariable("point_dep",otInteger);
        Qry.DeclareVariable("point_arv",otInteger);
        Qry.DeclareVariable("airp_dep",otString);
        Qry.DeclareVariable("airp_arv",otString);
        Qry.DeclareVariable("class",otString);
        Qry.DeclareVariable("status",otString);
        Qry.DeclareVariable("hall",otInteger);
        grp.toDB(Qry);

        if (GetNode("generated_grp_id",segNode)!=NULL)
          Qry.SetVariable("grp_id",NodeAsInteger("generated_grp_id",segNode));
        bool trfer_confirm=(reqInfo->client_type==ctTerm || !inbound_group_bag.empty());
        if (trfer_confirm) AfterSaveInfo.action=CheckIn::actionCheckPieceConcept;
        Qry.CreateVariable("trfer_confirm",otInteger,(int)trfer_confirm);
        Qry.CreateVariable("trfer_conflict",otInteger,(int)(!inbound_trfer_conflicts.empty() && reqInfo->client_type != ctTerm));  //�������� �ॢ��� ⮫쪮 ��� ���� � ���᪠
        Qry.CreateVariable("inbound_confirm",otInteger,(int)inbound_confirm); //��⠭�������� ⮫쪮 ��� �ନ����� compatible(INBOUND_TRFER_VERSION)
        if (reqInfo->client_type!=ctPNL)
          Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
        else
          Qry.CreateVariable("user_id",otInteger,FNull);
        Qry.CreateVariable("desk",otString,reqInfo->desk.code);
        Qry.CreateVariable("time_create",otDate,NowUTC());
        Qry.CreateVariable("client_type",otString,EncodeClientType(reqInfo->client_type));
        if (first_segment)
          Qry.CreateVariable("tckin_id",otInteger,FNull);
        else
          Qry.CreateVariable("tckin_id",otInteger,AfterSaveInfo.tckin_id);
        if (only_one) //�� ᪢����� ॣ������
          Qry.CreateVariable("seg_no",otInteger,FNull);
        else
          Qry.CreateVariable("seg_no",otInteger,seg_no);
        if (first_reg_no!=NoExists)
          Qry.CreateVariable("first_reg_no",otInteger,first_reg_no);
        else
          Qry.CreateVariable("first_reg_no",otInteger,FNull);

        if (IsMarkEqualOper(fltInfo, markFltInfo))
          Qry.CreateVariable("point_id_mark",otInteger,grp.point_dep);
        else
          Qry.CreateVariable("point_id_mark",otInteger,FNull);
        Qry.CreateVariable("airline_mark",otString,markFltInfo.airline);
        Qry.CreateVariable("flt_no_mark",otInteger,markFltInfo.flt_no);
        Qry.CreateVariable("suffix_mark",otString,markFltInfo.suffix);
        Qry.CreateVariable("scd_mark",otDate,markFltInfo.scd_out);
        Qry.CreateVariable("airp_dep_mark",otString,markFltInfo.airp);
        Qry.CreateVariable("pr_mark_norms",otInteger,(int)pr_mark_norms);

        Qry.Execute();
        grp.id=Qry.GetVariableAsInteger("grp_id");
        if (first_segment && !Qry.VariableIsNULL("tckin_id"))
          AfterSaveInfo.tckin_id=Qry.GetVariableAsInteger("tckin_id");

        ReplaceTextChild(segNode,"generated_grp_id",grp.id);

        if (!pr_unaccomp)
        {
          bool pr_brd_with_reg=false,pr_exam_with_brd=false;
          if (first_segment && reqInfo->client_type == ctTerm && grp.status!=psCrew)
          {
            //�� ᪢����� ॣ����樨 ᮢ���⭠� ॣ������ � ��ᠤ��� �.�. ⮫쪮 �� ��ࢮ� ३�
            //�� web-ॣ����樨 ��ᠤ�� ��ண� ࠧ���쭠�
            Qry.Clear();
            Qry.SQLText=
              "SELECT pr_misc FROM trip_hall "
              "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
              "ORDER BY DECODE(hall,NULL,1,0)";
            Qry.CreateVariable("point_id",otInteger,grp.point_dep);
            Qry.CreateVariable("hall",otInteger,grp.hall);
            Qry.DeclareVariable("type",otInteger);

            Qry.SetVariable("type",(int)tsBrdWithReg);
            Qry.Execute();
            if (!Qry.Eof) pr_brd_with_reg=Qry.FieldAsInteger("pr_misc")!=0;
            Qry.SetVariable("type",(int)tsExamWithBrd);
            Qry.Execute();
            if (!Qry.Eof) pr_exam_with_brd=Qry.FieldAsInteger("pr_misc")!=0;
          };

          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  IF :pax_id IS NULL THEN "
            "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
            "  END IF; "
            "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,crew_type,is_female,seat_type,seats,pr_brd, "
            "                  wl_type,refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm,doco_confirm, "
            "                  pr_exam,subclass,bag_pool_num,tid) "
            "  VALUES(:pax_id,pax_grp__seq.currval,:surname,:name,:pers_type,:crew_type,:is_female,:seat_type,:seats,:pr_brd, "
            "         :wl_type,NULL,:reg_no,:ticket_no,:coupon_no,:ticket_rem,:ticket_confirm,0, "
            "         :pr_exam,:subclass,:bag_pool_num,cycle_tid__seq.currval); "
            "END;";
          Qry.DeclareVariable("pax_id",otInteger);
          Qry.DeclareVariable("surname",otString);
          Qry.DeclareVariable("name",otString);
          Qry.DeclareVariable("pers_type",otString);
          Qry.DeclareVariable("crew_type",otString);
          Qry.DeclareVariable("is_female",otInteger);
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
          bool exists_preseats = false; //���� �� � ��㯯� ���ᠦ�஢ �।���⥫�� ����
          bool invalid_seat_no = false; //���� ����饭�� ����
          for(int k=0;k<=1;k++)
          {
            int pax_no=1;
            for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
            {
              CheckIn::TPaxItem &pax=p->pax;
              try
              {
                if ((pax.seats<=0&&k==0)||(pax.seats>0&&k==1)) continue;
                if (pax.seats>0)
                {
                  pax.wl_type=wl_type;
                }
                else
                {
                  pax.seat_type.clear();
                  pax.wl_type.clear();
                };
                pax.pr_brd=pr_brd_with_reg;
                pax.pr_exam=pr_brd_with_reg && pr_exam_with_brd;
                pax.toDB(Qry);
                int is_female=pax.is_female();
                if (is_female!=NoExists)
                  Qry.SetVariable("is_female", is_female);
                else
                  Qry.SetVariable("is_female", FNull);
                if (pax.id==NoExists)
                {
                  xmlNodePtr node2=p->node->children;
                  if (GetNodeFast("generated_pax_id",node2)!=NULL)
                    Qry.SetVariable("pax_id",NodeAsIntegerFast("generated_pax_id",node2));
                };

                try
                {
                  Qry.Execute();
                }
                catch(EOracleError E)
                {
                  if (E.Code==1)
                    throw UserException("MSG.PASSENGER.CHECKED.ALREADY_OTHER_DESK",
                                        LParams()<<LParam("surname",pax.full_name())); //WEB
                  else
                    throw;
                };
                int pax_id=Qry.GetVariableAsInteger("pax_id"); //ᯥ樠�쭮 ������ �������⥫��� ��६����� �⮡� �� �������� pax.id
                ReplaceTextChild(p->node,"generated_pax_id",pax_id);
                if (pax.id==NoExists) p->generated_pax_id=pax_id; //���������� ⮫쪮 �� ��ࢮ��砫쭮� ॣ����樨 (new_checkin) � ⮫쪮 ��� NOREC

                if (wl_type.empty() && grp.status!=psCrew)
                {
                  //������ ����஢ ����
                  if (pax.seats>0 && i<SEATS2::Passengers.getCount())
                  {
                    SEATS2::TPassenger pas = SEATS2::Passengers.Get(i);
                    ProgTrace( TRACE5, "pas.pax_id=%d, pax.id=%d, pax_id=%d", pas.paxId, pax.id, pax_id );
                    if ( isTranzitSalonsVersion ) {
                      for ( SALONS2::TAutoSeats::iterator iseat=autoSeats.begin();
                            iseat!=autoSeats.end(); iseat++ ) {
                        ProgTrace( TRACE5, "pas.paxId=%d, iseat->pax_id=%d, pax_id=%d",
                                   pas.paxId, iseat->pax_id, pax_id );
                        if ( pas.paxId == iseat->pax_id ) {
                          iseat->pax_id = pax_id;
                          break;
                        }
                      }
                      paxs_external_logged.insert( pax_id );
                    }

                      if ( pas.preseat_pax_id > 0 )
                        exists_preseats = true;
                        if ( !pas.isValidPlace )
                            invalid_seat_no = true;

                    if (pas.seat_no.empty()) throw EXCEPTIONS::Exception("SeatsPassengers: empty seat_no");
                        string pas_seat_no;
                        bool pr_found_preseat_no = false;
                        bool pr_found_agent_no = false;
                        for( std::vector<TSeat>::iterator iseat=pas.seat_no.begin(); iseat!=pas.seat_no.end(); iseat++ ) {
                          pas_seat_no = denorm_iata_row( iseat->row, NULL ) + denorm_iata_line( iseat->line, pr_lat_seat );
                        if ( pas_seat_no == pas.preseat_no ) {
                          pr_found_preseat_no = true;
                        }
                        if ( pas_seat_no == pax.seat_no )
                          pr_found_agent_no = true;
                      }
                      if ( !pax.seat_no.empty() &&
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
                    ProgTrace( TRACE5, "ranges.size=%zu", ranges.size() );
                    //������ � ����
                    TCompLayerType layer_type = cltCheckin;
                    switch( grp.status ) {
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
                      case psCrew:
                        throw EXCEPTIONS::Exception("SavePax: Not applied for crew");
                    }
                    SEATS2::SaveTripSeatRanges( grp.point_dep, layer_type, ranges, pax_id, grp.point_dep, grp.point_arv, NowUTC() );
                    if ( isTranzitSalonsVersion &&
                         !pr_do_check_wait_list_alarm ) {
                      autoSeats.WritePaxSeats( grp.point_dep, pax_id );
                    }
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
                }; //wl_type.empty()

                //������ pax_doc
                if (pax.DocExists) CheckIn::SavePaxDoc(pax_id,pax.doc,PaxDocQry);
                if (pax.DocoExists) CheckIn::SavePaxDoco(pax_id,pax.doco,PaxDocoQry);
                if (pax.DocExists)
                {
                  CrsQry.SetVariable("pax_id",pax_id);
                  CrsQry.SetVariable("document",pax.doc.no);
                  CrsQry.SetVariable("full_insert",(int)false); //��⮬ ���� ��ࠬ��� �� �������������
                  CrsQry.Execute();
                };

                if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCA_VERSION))
                {
                  if (pax.DocaExists) CheckIn::SavePaxDoca(pax_id, pax.doca, PaxDocaQry, true);
                }
                else
                {
                  list<CheckIn::TPaxDocaItem> doca;
                  if (LoadCrsPaxDoca(pax_id, doca))
                    SavePaxDoca(pax_id, doca, PaxDocaQry, true);
                };

                if (save_trfer)
                {
                  //������ �������ᮢ �࠭���
                  SavePaxTransfer(pax_id,pax_no,trfer,seg_no);
                };
                //������ ६�ப
                if (p->remsExists)
                {
                  CheckIn::SavePaxRem(pax_id, p->rems);
                  CheckIn::SavePaxFQT(pax_id, p->fqts);
                };
                //������ ���
                if (first_segment &&
                    !(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION)))
                  WeightConcept::PaxNormsToDB(pax_id, p->norms);

                if ( need_apps ) {
                  // ��� ����� ���ᠤ�஢ ६�ન APPS �� �஢��塞
                  processPax( pax_id );
                }
              }
              catch(CheckIn::UserException)
              {
                throw;
              }
              catch(UserException &e)
              {
                if (pax.id!=NoExists)
                  throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
                else
                  throw;
              };

            }; // end for paxs
          }; //end for k
          CheckIn::SyncPaxASVC(grp.id, true); //ᨭ�஭����㥬 ASVC ⮫쪮 �� ��ࢮ� ॣ����樨
        }
        else
        {
          //��ᮯ஢������� �����
          //������ ���
          if (first_segment &&
              !(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION)))
            WeightConcept::GrpNormsToDB(grp.id, grp.norms);
        };

        TLogLocale tlocale;
        tlocale.ev_type=ASTRA::evtPax;
        tlocale.id1=grp.point_dep;
        tlocale.id2=NoExists;
        tlocale.id3=grp.id;
        if (save_trfer)
        {
          SaveTransfer(grp.id,trfer,trfer_segs,pr_unaccomp,seg_no, tlocale);
          if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        };
        SaveTCkinSegs(grp.id,reqNode,segs,seg_no, tlocale);
        if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        if (!inbound_trfer_conflicts.empty())
          ConflictReasonsToLog(inbound_trfer_conflicts, tlocale);
      } //new_checkin
      else
      {
        if (reqInfo->client_type == ctPNL)
          throw EXCEPTIONS::Exception("SavePax: changes not supported for ctPNL");

        //������ ���������
        GetGrpToLogInfo(grp.id, grpInfoBefore); //��� ��� ᥣ���⮢
        if (needCheckUnattachedTrferAlarm)
          InboundTrfer::GetCheckedTags(grp.id, idGrp, grpTagsBefore); //��� ��� ᥣ���⮢
        paymentBefore.fromDB(grp.id);
        //BSM
        if (BSMsend)
          BSM::LoadContent(grp.id,BSMContentBefore);

        InboundTrfer::GetNextTrferCheckedFlts(grp.id, idGrp, nextTrferSegs);

        bool save_trfer=GetNode("transfer",reqNode)!=NULL;

        if (first_segment)
        {
          if (reqInfo->client_type == ctTerm)
          {
            //��� �ନ���� �ᥣ�� ���뢠��
            AfterSaveInfo.tckin_id=SeparateTCkin(grp.id,cssAllPrevCurr,cssNone,NoExists);
          }
          else
          {
            //��� ��� � ���᪠ ���� ����砥� tckin_id
            AfterSaveInfo.tckin_id=SeparateTCkin(grp.id,cssNone,cssNone,NoExists); //ctPNL???
          };
        };

        Qry.Clear();
        Qry.SQLText=
          "UPDATE pax_grp "
          "SET bag_refuse=:bag_refuse, "
          "    trfer_confirm=NVL(:trfer_confirm,trfer_confirm), "
          "    trfer_conflict=NVL(:trfer_conflict,trfer_conflict), "
          "    inbound_confirm=NVL(:inbound_confirm,inbound_confirm), "
          "    tid=cycle_tid__seq.nextval "
          "WHERE grp_id=:grp_id AND tid=:tid";
        Qry.DeclareVariable("grp_id",otInteger);
        Qry.DeclareVariable("tid",otInteger);
        Qry.DeclareVariable("bag_refuse",otInteger);
        grp.toDB(Qry);
        if (reqInfo->client_type==ctTerm)
        {
          if (!grpInfoBefore.trfer_confirm) AfterSaveInfo.action=CheckIn::actionCheckPieceConcept;
          Qry.CreateVariable("trfer_confirm",otInteger,(int)true);
          Qry.CreateVariable("trfer_conflict",otInteger,(int)false);
        }
        else
        {
          Qry.CreateVariable("trfer_confirm",otInteger,FNull);
          Qry.CreateVariable("trfer_conflict",otInteger,FNull);
        };
        if (inbound_confirm)
          Qry.CreateVariable("inbound_confirm",otInteger,(int)true);
        else
          Qry.CreateVariable("inbound_confirm",otInteger,FNull);

        Qry.Execute();
        if (Qry.RowsProcessed()<=0)
          throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

        if (!pr_unaccomp)
        {
          //�⠥� �������᪨� ३�
          Qry.Clear();
          Qry.SQLText=
            "SELECT mark_trips.airline,mark_trips.flt_no,mark_trips.suffix, "
            "       mark_trips.scd AS scd_out,mark_trips.airp_dep AS airp,pr_mark_norms "
            "FROM pax_grp,mark_trips "
            "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id";
          Qry.CreateVariable("grp_id",otInteger,grp.id);
          Qry.Execute();
          if (!Qry.Eof)
          {
            markFltInfo.Init(Qry);
            pr_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
          };

          TQuery PaxQry(&OraSession);
          PaxQry.Clear();
          ostringstream sql;
          sql << "UPDATE pax SET ";
          if (reqInfo->client_type==ctTerm)
          {
            sql << "    surname=:surname, "
                   "    name=:name, "
                   "    pers_type=:pers_type, "
                   "    ticket_no=:ticket_no, "
                   "    coupon_no=:coupon_no, "
                   "    ticket_rem=:ticket_rem, "
                   "    ticket_confirm=:ticket_confirm, "
                   "    subclass=:subclass, "
                   "    bag_pool_num=:bag_pool_num, ";
            PaxQry.DeclareVariable("surname",otString);
            PaxQry.DeclareVariable("name",otString);
            PaxQry.DeclareVariable("pers_type",otString);
            PaxQry.DeclareVariable("ticket_no",otString);
            PaxQry.DeclareVariable("coupon_no",otInteger);
            PaxQry.DeclareVariable("ticket_rem",otString);
            PaxQry.DeclareVariable("ticket_confirm",otInteger);
            PaxQry.DeclareVariable("subclass",otString);
            PaxQry.DeclareVariable("bag_pool_num",otInteger);
          };
          sql << "    is_female=DECODE(:doc_exists,0,is_female,:is_female), "
                 "    refuse=:refuse, "
                 "    pr_brd=DECODE(:refuse,NULL,pr_brd,NULL), "
                 "    pr_exam=DECODE(:refuse,NULL,pr_exam,0), "
                 "    tid=cycle_tid__seq.currval "
                 "WHERE pax_id=:pax_id AND tid=:tid";
          PaxQry.DeclareVariable("doc_exists",otInteger);
          PaxQry.DeclareVariable("is_female",otInteger);
          PaxQry.DeclareVariable("pax_id",otInteger);
          PaxQry.DeclareVariable("tid",otInteger);
          PaxQry.DeclareVariable("refuse",otString);

          PaxQry.SQLText=sql.str().c_str();


          TQuery LayerQry(&OraSession);
          LayerQry.Clear();
          LayerQry.SQLText=
            "DELETE FROM trip_comp_layers WHERE pax_id=:pax_id AND layer_type="
            "(SELECT layer_type FROM pax,pax_grp,grp_status_types "
            "  WHERE pax.pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND "
            "        pax_grp.status=grp_status_types.code)";
          LayerQry.DeclareVariable("pax_id",otInteger);

          int pax_no=1;
          for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
          {
            const CheckIn::TPaxItem &pax=p->pax;
            try
            {
              if (pax.PaxUpdatesPending)
              {
                if (!pax.refuse.empty())
                {
                  LayerQry.SetVariable("pax_id",pax.id);
                  LayerQry.Execute();
                };
                pax.toDB(PaxQry);
                PaxQry.SetVariable("doc_exists", (int)pax.DocExists);
                int is_female=pax.is_female();
                if (pax.DocExists && is_female!=NoExists)
                  PaxQry.SetVariable("is_female", is_female);
                else
                  PaxQry.SetVariable("is_female", FNull);
                PaxQry.Execute();
                if (PaxQry.RowsProcessed()<=0)
                  throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                      LParams()<<LParam("surname",pax.full_name())); //WEB
                //������ pax_doc
                if (pax.DocExists) CheckIn::SavePaxDoc(pax.id,pax.doc,PaxDocQry);
                if (pax.DocoExists) CheckIn::SavePaxDoco(pax.id,pax.doco,PaxDocoQry);
                if (pax.DocExists)
                {
                  CrsQry.SetVariable("pax_id",pax.id);
                  CrsQry.SetVariable("document",pax.doc.no);
                  CrsQry.SetVariable("full_insert",(int)false);
                  CrsQry.Execute();
                };

                if (reqInfo->client_type!=ctTerm || reqInfo->desk.compatible(DOCA_VERSION))
                {
                  if (pax.DocaExists) CheckIn::SavePaxDoca(pax.id, pax.doca, PaxDocaQry, false);
                };

                if (reqInfo->client_type!=ctTerm && pax.refuse==refuseAgentError) //ctPNL???
                {
                  //��� � ���� ॣ������
                  Qry.Clear();
                  Qry.SQLText=
                    "INSERT INTO crs_pax_refuse(pax_id, client_type, time) "
                    "SELECT pax_id, :client_type, SYSTEM.UTCSYSDATE "
                    "FROM crs_pax WHERE pax_id=:pax_id";
                  Qry.CreateVariable("pax_id", otInteger, pax.id);
                  Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
                  Qry.Execute();
                };
              }
              else
              {
                Qry.Clear();
                sql.str("");
                sql << "UPDATE pax SET ";
                if (!inbound_group_bag.empty())
                {
                  //�� ����� ���� ���⢥ত���� �室�饣� �࠭��� ��᫥ ���-ॣ���樨
                  sql << "    bag_pool_num=:bag_pool_num, ";
                  Qry.DeclareVariable("bag_pool_num",otInteger);
                };
                sql << "    tid=cycle_tid__seq.currval "
                       "WHERE pax_id=:pax_id AND tid=:tid";
                Qry.SQLText=sql.str().c_str();
                Qry.DeclareVariable("pax_id",otInteger);
                Qry.DeclareVariable("tid",otInteger);
                pax.toDB(Qry);
                Qry.Execute();
                if (Qry.RowsProcessed()<=0)
                  throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                                      LParams()<<LParam("surname",pax.full_name())); //WEB
              };

              if (save_trfer)
              {
                //������ �������ᮢ �࠭���
                SavePaxTransfer(pax.id,pax_no,trfer,seg_no);
              };
              //������ ६�ப
              if (p->remsExists)
              {
                CheckIn::SavePaxRem(pax.id, p->rems);
                CheckIn::SavePaxFQT(pax.id, p->fqts);
              };
              //������ ���
              if (first_segment &&
                  !(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION)))
                WeightConcept::PaxNormsToDB(pax.id, p->norms);

              if ( need_apps ) {
                string override;
                bool is_forced = false;
                // �஢�ਬ ६�ન ⮫쪮 � ��砥 ����� ���������
                CheckInInterface::CheckAPPSRems( p->rems, override, is_forced );
                processPax( pax.id, override, is_forced );
              }
            }
            catch(CheckIn::UserException)
            {
              throw;
            }
            catch(UserException &e)
            {
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            };
          };
        }
        else
        {
          //������ ���
          if (first_segment &&
              !(reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION)))
            WeightConcept::GrpNormsToDB(grp.id, grp.norms);
        };

        if (save_trfer)
        {
          TLogLocale tlocale;
          tlocale.ev_type=ASTRA::evtPax;
          tlocale.id1=grp.point_dep;
          tlocale.id2=NoExists;
          tlocale.id3=grp.id;
          SaveTransfer(grp.id,trfer,trfer_segs,pr_unaccomp,seg_no, tlocale);
          if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        };
      };

      if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(PIECE_CONCEPT_VERSION) &&
          AfterSaveInfo.action==CheckIn::actionCheckPieceConcept && setList.value(tsPieceConcept))
        throw UserException("MSG.TERM_VERSION.PIECE_CONCEPT_NOT_SUPPORTED");

      AfterSaveInfo.segs.back().grp_id=grp.id;

      if (first_segment)
      {
        if (!inbound_group_bag.empty())
        {
          inbound_group_bag.toDB(grp.id);
          if (grp.group_bag)
            showErrorMessage("MSG.CHECKIN.BAGGAGE_NOT_REGISTERED_DUE_INBOUND_TRFER");
        }
        else
        {
          if (reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION) &&
              AfterSaveInfo.action==CheckIn::actionCheckPieceConcept && setList.value(tsPieceConcept))
          {
            //�� �����뢠�� ����� �� �⮬ �⠯�, ⠪ ��� �� �� ����� ��⥬� ����
            if (grp.group_bag)
              showErrorMessage("MSG.CHECKIN.BAGGAGE_NOT_REGISTERED_DUE_CHECKING_PIECE_CONCEPT");
          }
          else
          {
            //����� ��⥬� ����
            if (grp.group_bag)
            {
              grp.group_bag.get().checkAndGenerateTags(grp.point_dep, grp.id);
              grp.group_bag.get().getAllListItems(grp.id, pr_unaccomp, 0);
              CheckBagChanges(grpInfoBefore, grp);
              grp.group_bag.get().toDB(grp.id);
            };

            boost::optional< CheckIn::TServicePaymentList> payment;
            CheckIn::ServicePaymentFromXML(segNode, grp.id, pr_unaccomp, grp.baggage_pc, paymentBefore, payment);
            CheckIn::TPaidBagEMDProps paid_bag_emd_props;
            CheckIn::CalcPaidBagEMDProps(paymentBefore, payment, handmadeEMDDiff, paid_bag_emd_props);
            CheckIn::PaidBagEMDPropsToDB(grp.id, paid_bag_emd_props);

            if (grp.svc)
            {
              grp.svc.get().getAllListItems();
              grp.svc.get().toDB(grp.id);
            }
            if (grp.wt)
            {
              if (reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION))
              {
                //���� � ������ ��� � ���⭮�� ������
                WeightConcept::TNormFltInfo normFltInfo;
                normFltInfo.point_id=grp.point_dep;
                normFltInfo.airline_mark=markFltInfo.airline;
                normFltInfo.flt_no_mark=markFltInfo.flt_no;
                normFltInfo.use_mark_flt=pr_mark_norms;
                normFltInfo.use_mixed_norms=pr_mixed_norms;
                WeightConcept::TAirlines airlines(grp.id,
                                                  pr_mark_norms?markFltInfo.airline:fltInfo.airline,
                                                  "RecalcPaidBagToDB");
                map<int/*id*/, TEventsBagItem> curr_bag;
                GetBagToLogInfo(grp.id, curr_bag);
                WeightConcept::TPaidBagList prior_paid;
                WeightConcept::PaidBagFromDB(NoExists, grp.id, prior_paid);
                WeightConcept::TPaidBagList result_paid;
                WeightConcept::RecalcPaidBagToDB(airlines, grpInfoBefore.bag, curr_bag, grpInfoBefore.pax, normFltInfo, trfer, grp, paxs, prior_paid, pr_unaccomp, true, result_paid);
                CheckIn::TryCleanServicePayment(result_paid, paymentBefore, payment);
              }
              else
              {
                if (grp.paid)
                {
                  WeightConcept::PaidBagToDB(grp.id, pr_unaccomp, grp.paid.get());
                  CheckIn::TryCleanServicePayment(grp.paid.get(), paymentBefore, payment);
                };
              };
            }

            if (grp.rfisc_used && AfterSaveInfo.action==CheckIn::actionNone)
            {
              if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION) || grp.pc)
                AfterSaveInfo.action=CheckIn::actionRefreshPaidBagPC;
            }

            ServicePaymentToDBAdv(grp.id, pr_unaccomp, paymentBefore, payment);

            SaveTagPacks(reqNode);
          }
        };
      }
      else
      {
        //����ᮬ �����뢠�� � ��⠫�� ��㯯� �����, ��ન, 業�� �����
        if (seg_no-1<=(int)trfer.size())
        {
          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  DELETE FROM value_bag WHERE grp_id=:grp_id; "
            "  DELETE FROM unaccomp_bag_info WHERE grp_id=:grp_id; "
            "  DELETE FROM bag2 WHERE grp_id=:grp_id; "
            "  DELETE FROM paid_bag WHERE grp_id=:grp_id; "
            "  DELETE FROM bag_tags WHERE grp_id=:grp_id; "
            "  INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax,tax_trfer) "
            "  SELECT :grp_id ,num,value,value_cur,NULL,NULL,NULL "
            "  FROM value_bag WHERE grp_id=:first_grp_id; "
            "  INSERT INTO bag2(grp_id,num,id,bag_type,rfisc,pr_cabin,amount,weight,value_bag_num, "
            "    pr_liab_limit,to_ramp,using_scales,bag_pool_num,hall,user_id,desk,time_create,is_trfer,handmade,"
            "    list_id, bag_type_str, service_type, airline) "
            "  SELECT :grp_id,num,id,bag_type,rfisc,pr_cabin,amount,weight,value_bag_num, "
            "    pr_liab_limit,0,using_scales,bag_pool_num,hall,user_id,desk,time_create,1,0, "
            "    list_id, bag_type_str, service_type, airline "
            "  FROM bag2 WHERE grp_id=:first_grp_id; "
            "  INSERT INTO bag_tags(grp_id,num,tag_type,no,color,bag_num,pr_print) "
            "  SELECT :grp_id,num,tag_type,no,color,bag_num,pr_print "
            "  FROM bag_tags WHERE grp_id=:first_grp_id; "
            "  INSERT INTO unaccomp_bag_info(grp_id,num,original_tag_no,surname,name,airline,flt_no,suffix,scd) "
            "  SELECT :grp_id,num,original_tag_no,surname,name,airline,flt_no,suffix,scd "
            "  FROM unaccomp_bag_info WHERE grp_id=:first_grp_id; "
            "  MERGE INTO pax "
            "  USING "
            "  ( "
            "  SELECT pax1.bag_pool_num, pax2.pax_id "
            "  FROM pax pax1, tckin_pax_grp tckin_pax_grp1, "
            "       pax pax2, tckin_pax_grp tckin_pax_grp2 "
            "  WHERE pax1.grp_id=tckin_pax_grp1.grp_id AND "
            "        pax2.grp_id=tckin_pax_grp2.grp_id AND "
            "        tckin_pax_grp1.first_reg_no-pax1.reg_no=tckin_pax_grp2.first_reg_no-pax2.reg_no AND "
            "        tckin_pax_grp1.grp_id=:first_grp_id AND "
            "        tckin_pax_grp2.grp_id=:grp_id "
            "  ) src "
            "  ON (pax.pax_id=src.pax_id) "
            "  WHEN MATCHED THEN "
            "  UPDATE SET pax.bag_pool_num=src.bag_pool_num, "
            "             pax.tid=DECODE(pax.bag_pool_num, src.bag_pool_num, pax.tid, cycle_tid__seq.currval); "
            "END; ";
          Qry.CreateVariable("grp_id",otInteger,grp.id);
          Qry.CreateVariable("first_grp_id",otInteger,AfterSaveInfo.segs.front().grp_id);
          Qry.Execute();
        };
      };
      if (reqInfo->client_type==ctTerm && !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
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
          Qry.CreateVariable("grp_id",otInteger,grp.id);
          Qry.Execute();
        };
      };

      if (!pr_unaccomp)
      {
        //�஢�ਬ �㡫�஢���� ����⮢
        Qry.Clear();
        Qry.SQLText=
          "SELECT ticket_no,coupon_no FROM pax "
          "WHERE refuse IS NULL AND ticket_no=:ticket_no AND coupon_no=:coupon_no "
          "GROUP BY ticket_no,coupon_no HAVING COUNT(*)>1";
        Qry.DeclareVariable("ticket_no",otString);
        Qry.DeclareVariable("coupon_no",otInteger);
        for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
        {
          const CheckIn::TPaxItem &pax=p->pax;
          try
          {
            if (!pax.TknExists) continue;
            if (pax.tkn.no.empty() || pax.tkn.coupon==NoExists) continue;
            Qry.SetVariable("ticket_no",pax.tkn.no);
            Qry.SetVariable("coupon_no",pax.tkn.coupon);
            Qry.Execute();
            if (!Qry.Eof)
              throw UserException("MSG.CHECKIN.DUPLICATED_ETICKET",
                                  LParams()<<LParam("eticket",pax.tkn.no)
                                           <<LParam("coupon",IntToString(pax.tkn.coupon))); //WEB
          }
          catch(CheckIn::UserException)
          {
            throw;
          }
          catch(UserException &e)
          {
            if (pax.id!=NoExists)
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            else
              throw;
          };
        };
        if (ediResNode!=NULL)
        {
          //������� ticket_confirm � events_bilingual �� �᭮�� ���⢥ত����� ����ᮢ
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
            "      DELETE FROM events_bilingual WHERE time=:ev_time AND ev_order=:ev_order; "
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

          xmlNodePtr ticketNode=GetNode("tickets",ediResNode);
          if (ticketNode!=NULL) ticketNode=ticketNode->children;
          for(;ticketNode!=NULL;ticketNode=ticketNode->next)
          {
            xmlNodePtr ticketNode2=ticketNode->children;
            if (GetNodeFast("coupon_status",ticketNode2)==NULL) continue;
            if (GetNodeFast("pax_id",ticketNode2)==NULL)
              throw EXCEPTIONS::Exception("CheckInInterface::SavePax: pax_id not defined in ediRes");

            int ticket_pax_id=NodeAsIntegerFast("pax_id",ticketNode2);

            CheckIn::TPaxList::const_iterator p=paxs.begin();
            for(; p!=paxs.end(); ++p)
            {
              xmlNodePtr node2=p->node->children;
              int pax_id;
              if (GetNodeFast("generated_pax_id",node2)!=NULL)
                pax_id=NodeAsIntegerFast("generated_pax_id",node2);
              else
                pax_id=p->pax.id;
              if (ticket_pax_id==pax_id) break;
            };
            if (p!=paxs.end())
            {
              Qry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",ticketNode2));
              Qry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",ticketNode2));
              Qry.SetVariable("pax_id",ticket_pax_id);
              Qry.SetVariable("grp_id",FNull);
              Qry.SetVariable("reg_no",FNull);

              xmlNodePtr eventNode=GetNode("coupon_status/event",ticketNode);
              if (eventNode!=NULL &&
                  GetNodeFast("reg_no",ticketNode2)==NULL &&
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
                  GetNodeFast("reg_no",ticketNode2)==NULL &&
                  !Qry.VariableIsNULL("reg_no") &&
                  !Qry.VariableIsNULL("grp_id"))
              {
                TLogLocale locale;
                locale.ev_type=ASTRA::evtPax;
                locale.id1=grp.point_dep;
                locale.id2=Qry.GetVariableAsInteger("reg_no");
                locale.id3=Qry.GetVariableAsInteger("grp_id");
                LocaleFromXML(eventNode, locale.lexema_id, locale.prms);
                reqInfo->LocaleToLog(locale);
              };

            };
          };

          xmlNodePtr emdNode=GetNode("emdocs",ediResNode);
          if (emdNode!=NULL) emdNode=emdNode->children;
          for(;emdNode!=NULL;emdNode=emdNode->next)
          {
            TEMDCtxtItem EMDCtxt;
            EMDCtxt.fromXML(emdNode);
            if (EMDCtxt.paxUnknown())
              throw EXCEPTIONS::Exception("CheckInInterface::SavePax: EMDCtxt.paxUnknown() in ediRes");

            for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
            {
              xmlNodePtr node2=p->node->children;
              int pax_id;
              if (GetNodeFast("generated_pax_id",node2)!=NULL)
                pax_id=NodeAsIntegerFast("generated_pax_id",node2);
              else
                pax_id=p->pax.id;
              if (EMDCtxt.pax.id==pax_id)
              {
                TLogLocale actual_event;
                if (ActualEMDEvent(EMDCtxt, emdNode, actual_event))
                  reqInfo->LocaleToLog(actual_event);
                break;
              };
            };
          };
        };
      };

      //��� ����� ETCheckStatus::CheckGrpStatus
      //��易⥫쭮 �� ckin.check_grp
      if (ediResNode==NULL && reqInfo->client_type!=ctPNL)
      {
        if (!defer_etstatus)
          ETStatusInterface::ETCheckStatus(grp.id, csaGrp, NoExists, false, ChangeStatusInfo.ET, true);
        EMDStatusInterface::EMDCheckStatus(grp.id, paymentBefore, ChangeStatusInfo.EMD);
      };

      if (!pr_unaccomp && grp.status!=psCrew)
      {
        if ( isTranzitSalonsVersion ) {
          //!!!⮫쪮 ��� ॣ����樨 ���ᠦ�஢
          //��।���� �� ���⠬ ���ᠦ�஢ �㦭� �� ������ ������� �ॢ��� �� �
          //�᫨ �㦭� ������ �������
          if ( pr_do_check_wait_list_alarm ) {
            SALONS2::check_waitlist_alarm_on_tranzit_routes( grp.point_dep, paxs_external_logged );
          }
        }
        else {
          check_waitlist_alarm( grp.point_dep );
        }
      };

      if (ChangeStatusInfo.empty())
      {
        //�����뢠�� � ��� ⮫쪮 �᫨ �� �㤥� �⪠� �࠭���樨 ��-�� ���饭�� � ���
        GetGrpToLogInfo(grp.id, grpInfoAfter);
      };

      if (!pr_unaccomp)
      {
        rozysk::sync_pax_grp(grp.id, reqInfo->desk.code, reqInfo->user.descr);
      };

      Qry.Clear();
      Qry.SQLText=
        "BEGIN "
        "  ckin.check_grp(:grp_id); "
        "  ckin.recount(:point_id); "
        "END;";
      Qry.CreateVariable("point_id",otInteger,grp.point_dep);
      Qry.CreateVariable("grp_id",otInteger,grp.id);
      Qry.Execute();
      Qry.Close();

      TAnnulBT annul_bt_after;
      annul_bt_after.get(grp.id);
      annul_bt.minus(annul_bt_after);

      annul_bt.toDB();

      //�஢�ਬ ���ᨬ����� ����㧪�
      bool overload_alarm = calc_overload_alarm( grp.point_dep ); // ���᫨�� �ਧ��� ��ॣ�㧪�
      if (overload_alarm)
      {
        //�஢��塞, � �� �뫮 �� ������ �⬥�� ॣ����樨 ��㯯� �� �訡�� �����
        Qry.Clear();
        Qry.SQLText="SELECT grp_id FROM pax_grp WHERE grp_id=:grp_id";
        Qry.CreateVariable("grp_id",otInteger,grp.id);
        Qry.Execute();
        if (!Qry.Eof)
        {
          try
          {
            if (CheckFltOverload(grp.point_dep,fltInfo,overload_alarm))
            {
              //ࠡ�⠥� �᫨ ࠧ�襭� ॣ������ �� �ॢ�襭�� ����㧪�
              //�த������ ॣ������ � �������� �ॢ���
              Set_AODB_overload_alarm( grp.point_dep, true );
            };
          }
          catch(CheckIn::OverloadException &E)
          {
            if (!new_checkin && reqInfo->client_type!=ctTerm && reqInfo->client_type!=ctPNL)
              //������ ᯥ樠���� ����� � SavePax:
              //�� ����� ��������� ��� � ���᪮� �� �⪠�뢠���� �� ��ॣ�㧪�
              Set_AODB_overload_alarm( grp.point_dep, true );
            else
            {

              //ࠡ�⠥� �᫨ ����饭� ॣ������ �� �ॢ�襭�� ����㧪�
              //�⪠�뢠�� ॣ������ �� ᭨��� ��窨 � ३� � �������� �ॢ���
              //�� ᪢����� ॣ����樨 �㤥� ������� �ॢ��� �� ��ࢮ� ��ॣ�㦥���� ᥣ����
              Qry.Clear();
              Qry.SQLText=
                "BEGIN "
                "  ROLLBACK TO CHECKIN; "
                "END;";
              Qry.Execute();

              if (reqInfo->client_type!=ctPNL)
              {
                if (reqInfo->client_type==ctTerm)
                {
                  if (!only_one)
                    showError( GetLexemeDataWithFlight(E.getLexemaData( ), fltInfo) );
                  else
                    showError( E.getLexemaData( ) );
                }
                else
                {
                  //���, ���᪨
                  CheckIn::UserException ce(E.getLexemaData(), grp.point_dep);
                  CheckIn::showError(ce.segs);
                };
              };

              set_alarm( grp.point_dep, atOverload, true ); // ��⠭����� �ਧ��� ��ॣ�㧪� ��ᬮ��� �� � �� ॠ�쭮� ��ॣ�㧪� ���
              Set_AODB_overload_alarm( grp.point_dep, true );

              if (reqInfo->client_type==ctPNL) throw;
              return false;
            };
          };
        };
      };

      set_alarm( grp.point_dep, atOverload, overload_alarm ); // ��⠭����� �ਧ��� ��ॣ�㧪�

      if (!pr_unaccomp && grp.status!=psCrew)
      {
        //����⠥� ������ ��㯯� �������ᮢ
        TQuery PaxQry(&OraSession);
        PaxQry.Clear();
        PaxQry.SQLText=
          "SELECT subclass,class FROM pax_grp,pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax.grp_id=:grp_id "
          "ORDER BY pax.reg_no, pax.seats DESC";
        PaxQry.CreateVariable("grp_id",otInteger,grp.id);
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
          Qry.CreateVariable("grp_id",otInteger,grp.id);
          Qry.CreateVariable("class_grp",otInteger,class_grp);
          Qry.Execute();
        };
        //����塞 � �����뢠�� �ਧ��� waitlist_alarm � brd_alarm � spec_service_alarm
        check_brd_alarm( grp.point_dep );
        check_spec_service_alarm( grp.point_dep );
        check_unbound_emd_alarm( grp.point_dep );
        check_conflict_trfer_alarm( grp.point_dep );
        if ( first_pax_on_flight ) {
          SALONS2::setManualCompChg( grp.point_dep );
        }
        check_apis_alarms( grp.point_dep );
        check_apps_alarm( grp.point_dep );
      };
      if (!pr_unaccomp && grp.status==psCrew)
      {
        check_crew_alarms( grp.point_dep );
        check_apis_alarms( grp.point_dep );
        check_apps_alarm( grp.point_dep );
      };

      check_TrferExists( grp.point_dep );
      if (needCheckUnattachedTrferAlarm)
        check_u_trfer_alarm_for_grp( grp.point_dep, grp.id, grpTagsBefore);
      if (new_checkin) //��� ����� ��������� 㦥 ��࠭�� ����稫� nextTrferSegs
        InboundTrfer::GetNextTrferCheckedFlts(grp.id, idGrp, nextTrferSegs);
      check_unattached_trfer_alarm(nextTrferSegs);

      //BSM
      if (BSMsend) BSM::Send(grp.point_dep,grp.id,BSMContentBefore,BSMaddrs);

      if (grp.status!=psCrew)
      {
        Qry.Clear();
        Qry.SQLText=
          "BEGIN "
          "  UPDATE utg_prl SET last_flt_change_tid=cycle_tid__seq.currval WHERE point_id=:point_id; "
          "  IF SQL%NOTFOUND THEN "
          "    INSERT INTO utg_prl(point_id, last_tlg_create_tid, last_flt_change_tid) "
          "    VALUES (:point_id, NULL, cycle_tid__seq.currval); "
          "  END IF; "
          "END;";
        Qry.CreateVariable("point_id", otInteger, grp.point_dep);
        Qry.Execute();
      };
    }
    catch(UserException &e)
    {
      if (reqInfo->client_type==ctPNL) throw;
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
        //���, ���᪨
        try
        {
          dynamic_cast<CheckIn::UserException&>(e);
          throw; //�� 㦥 CheckIn::UserException - �ப��뢠�� �����
        }
        catch (bad_cast)
        {
          throw CheckIn::UserException(e.getLexemaData(), grp.point_dep);
        };
      };
    };

  }; //横� �� ᥣ���⠬

  if (AfterSaveInfo.action==CheckIn::actionRefreshPaidBagPC)
  try
  {

    if (reqInfo->client_type==ASTRA::ctTerm &&
        !reqInfo->desk.compatible(PIECE_CONCEPT_VERSION))
    {
      showErrorMessage("MSG.TERM_VERSION.PIECE_CONCEPT_NOT_SUPPORTED");
      throw 1;
    };
    if (AfterSaveInfo.segs.empty()) throw 1;
    int first_grp_id=AfterSaveInfo.segs.front().grp_id;
    TCkinGrpIds tckin_grp_ids;
    CheckIn::TPaxGrpCategory::Enum grp_cat;

    SirenaExchange::TPaymentStatusReq req;
    SirenaExchange::fillPaxsBags(first_grp_id, req, grp_cat, tckin_grp_ids);

    if (tckin_grp_ids.empty() || grp_cat!=CheckIn::TPaxGrpCategory::Passenges) throw 1;

    TPaidRFISCList paid;
    ProgTrace(TRACE5, "actionRefreshPaidBagPC req.svcs.size()=%zu", req.svcs.size());
    if (!req.svcs.empty())
    {
      try
      {
        SirenaExchange::TPaymentStatusRes res;
//        res.setSrcFile("svc_payment_status_res.xml");
//        res.setDestFile("svc_payment_status_res.xml");

        SirenaExchange::TLastExchangeInfo prior, curr;
        prior.fromDB(first_grp_id);
        req.build(curr.pc_payment_req);
        if (prior.pc_payment_req_created==NoExists ||
            prior.pc_payment_res_created==NoExists ||
            prior.pc_payment_req.empty() ||
            prior.pc_payment_res.empty() ||
            prior.pc_payment_req!=curr.pc_payment_req)
        {
          RequestInfo requestInfo;
          ResponseInfo responseInfo;
          SirenaExchange::SendRequest(req, res, requestInfo, responseInfo);
          curr.grp_id=first_grp_id;
          curr.pc_payment_res=responseInfo.content;
          SirenaExchangeList.push_back(curr);
        }
        else
          res.parse(prior.pc_payment_res);

        if (req.paxs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: req.paxs.empty()", __FUNCTION__);
        const SirenaExchange::TPaxSegMap &segs=req.paxs.front().segs;
        if (segs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: segs.empty()", __FUNCTION__);
        for(SirenaExchange::TPaxSegMap::const_iterator s=segs.begin(); s!=segs.end(); ++s)
        {
          string flight_view=GetTripName(s->second.operFlt, ecCkin);

          set<TRFISCListKey> rfiscs;
          res.check_unknown_status(s->second.id, rfiscs);
          if (!rfiscs.empty())
            throw UserException("MSG.CHECKIN.UNKNOWN_PAYMENT_STATUS_FOR_BAG_TYPE_ON_SEGMENT",
                                LParams() << LParam("flight", flight_view)
                                          << LParam("bag_type", rfiscs.begin()->str()));
        };
        //�� �� ������ �������� ������ � �� ����� unknown
        for(SirenaExchange::TSvcList::const_iterator i=res.svcs.begin(); i!=res.svcs.end(); ++i)
          if (i->status==TServiceStatus::Unknown)
            throw EXCEPTIONS::Exception("%s: strange situation: TServiceStatus::Unknown for trfer_num=%d", __FUNCTION__, i->trfer_num);

        res.normsToDB(tckin_grp_ids);
        res.svcs.get(paid);
      }
      catch(UserException &e)
      {
        throw;
      }
      catch(std::exception &e)
      {
        ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
        throw UserException("MSG.CHECKIN.UNABLE_CALC_PAID_BAG_TRY_RE_CHECKIN");
      }
    };
    CheckIn::TServicePaymentList paymentBefore, payment;
    paymentBefore.fromDB(first_grp_id);
    payment=paymentBefore;
    paid.getAllListItems();
    paid.toDB(first_grp_id);
    if (CheckIn::TryCleanServicePayment(paid, payment))
      payment.toDB(first_grp_id);

    if (ediResNode==NULL && reqInfo->client_type!=ctPNL)
    {
      EMDStatusInterface::EMDCheckStatus(first_grp_id, paymentBefore, ChangeStatusInfo.EMD);
    };
  }
  catch(int) {};

  return true;
};

void CheckInInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node;
  int grp_id=NoExists;
  int point_id=NodeAsInteger("point_id",reqNode);
  TQuery Qry(&OraSession);
  node = GetNode("grp_id",reqNode);
  if (node==NULL||NodeIsNULL(node))
  {
    node = GetNode("reg_no",reqNode);
    if (node==NULL||NodeIsNULL(node))
    {
      int pax_id=NoExists;
      node = GetNode("pax_id",reqNode);
      if (node==NULL||NodeIsNULL(node))
      {
         int pax_point_id, reg_no;
         SearchPaxByScanData(reqNode, pax_point_id, reg_no, pax_id);
         if (pax_point_id==NoExists || reg_no==NoExists || pax_id==NoExists)
           throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
      }
      else pax_id=NodeAsInteger(node);

      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id,point_dep,class FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND pax_id=:pax_id";
      Qry.CreateVariable("pax_id",otInteger,pax_id);
      Qry.Execute();
      if (Qry.Eof)
        throw UserException("MSG.PASSENGER.NOT_CHECKIN");
      grp_id=Qry.FieldAsInteger("grp_id");
      bool pr_unaccomp=Qry.FieldIsNULL("class");
      int point_dep=Qry.FieldAsInteger("point_dep");
      if (point_dep!=point_id)
      {
        if (!TReqInfo::Instance()->desk.compatible(CKIN_SCAN_BP_VERSION))
          throw UserException("MSG.PASSENGER.OTHER_FLIGHT");
        point_id=point_dep;
        xmlNodePtr dataNode=NewTextChild( resNode, "data" );
        NewTextChild( dataNode, "point_id", point_id );
        if (!TripsInterface::readTripHeader( point_id, dataNode ))
        {
          TQuery FltQry(&OraSession);
          FltQry.Clear();
          FltQry.SQLText=
            "SELECT airline,flt_no,suffix,airp,scd_out, "
            "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
            "FROM points WHERE point_id=:point_id AND pr_del>=0";
          FltQry.CreateVariable("point_id",otInteger,point_id);
          FltQry.Execute();
          string msg;
          if (!FltQry.Eof)
          {
            TTripInfo info(FltQry);
            if (!pr_unaccomp)
              msg=getLocaleText("MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
            else
              msg=getLocaleText("MSG.BAGGAGE.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
          }
          else
          {
            if (!pr_unaccomp)
              msg=getLocaleText("MSG.PASSENGER.FROM_OTHER_FLIGHT");
            else
              msg=getLocaleText("MSG.BAGGAGE.FROM_OTHER_FLIGHT");
          };
          throw AstraLocale::UserException(msg);
        }
        else
        {
          readTripCounters( point_id, dataNode );
          readTripData( point_id, point_id, dataNode );
          readTripSets( point_id, dataNode );
          TripsInterface::PectabsResponse(point_id, reqNode, dataNode);
        };
      };
    }
    else
    {
      int reg_no=NodeAsInteger(node);
      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id, pax.seats FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      point_dep=:point_id AND reg_no=:reg_no ";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.CreateVariable("reg_no",otInteger,reg_no);
      Qry.Execute();
      if (Qry.Eof) throw UserException(1,"MSG.CHECKIN.REG_NO_NOT_FOUND");
      grp_id=Qry.FieldAsInteger("grp_id");
      bool exists_with_seat=false;
      bool exists_without_seat=false;
      for(;!Qry.Eof;Qry.Next())
      {
        if (grp_id!=Qry.FieldAsInteger("grp_id"))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        int seats=Qry.FieldAsInteger("seats");
        if ((seats>0 && exists_with_seat) ||
            (seats<=0 && exists_without_seat))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",point_id,reg_no);
        if (seats>0)
          exists_with_seat=true;
        else
          exists_without_seat=true;
      };
    };
  }
  else grp_id=NodeAsInteger(node);

  EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), reqNode);

  LoadPax(grp_id,resNode,false);
};

void AddPaxCategory(const CheckIn::TPaxItem &pax, set<string> &cats)
{
  if (pax.pers_type==child && pax.seats==0) cats.insert("CHC");
  if (pax.pers_type==baby && pax.seats==0) cats.insert("INA");
};

void ShowPaxCatWarning(const string &airline, const set<string> &cats)
{
  if (cats.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT airline FROM bag_norms WHERE airline=:airline AND pax_cat=:pax_cat AND rownum<2";
  Qry.CreateVariable("airline", otString, airline);
  Qry.DeclareVariable("pax_cat", otString);
  set<string>::const_iterator i=cats.begin();
  for(; i!=cats.end(); ++i)
  {
    Qry.SetVariable("pax_cat", *i);
    Qry.Execute();
    if (!Qry.Eof) break;
  };
  if (i==cats.end()) return;
  if (*i=="CHC") showErrorMessage("MSG.NEED_SELECT_BAG_NORMS_MANUALLY.CHC");
  if (*i=="INA") showErrorMessage("MSG.NEED_SELECT_BAG_NORMS_MANUALLY.INA");
};

const char* pax_sql=
    "SELECT pax.pax_id,pax.surname,pax.name,pax.pers_type,crew_type,"
    "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'one',rownum) AS seat_no, "
    "       pax.seat_type, "
    "       pax.seats,pax.refuse,pax.reg_no, "
    "       pax.ticket_no,pax.coupon_no,pax.ticket_rem,pax.ticket_confirm, "
    "       pax.subclass,pax.tid,pax.bag_pool_num, "
    "       crs_pax.pax_id AS crs_pax_id "
    "FROM pax,crs_pax "
    "WHERE pax.pax_id=crs_pax.pax_id(+) AND crs_pax.pr_del(+)=0 AND "
    "      pax.grp_id=:grp_id "
    "ORDER BY ABS(pax.reg_no), pax.seats DESC";

namespace SirenaExchange
{

void fillPaxsBags(int first_grp_id, TExchange &exch, CheckIn::TPaxGrpCategory::Enum &grp_cat, TCkinGrpIds &tckin_grp_ids,
                  bool include_refused)
{
  grp_cat=CheckIn::TPaxGrpCategory::Unknown;
  tckin_grp_ids.clear();

  TAvailabilityReq  *availabilityReq=dynamic_cast<TAvailabilityReq*>(&exch);
  TPaymentStatusReq *paymentStatusReq=dynamic_cast<TPaymentStatusReq*>(&exch);
  TGroupInfoRes     *groupInfoRes=dynamic_cast<TGroupInfoRes*>(&exch);

  TCkinRoute tckin_route;
  tckin_route.GetRouteAfter(first_grp_id, crtNotCurrent, crtOnlyDependent);
  tckin_route.get(tckin_grp_ids);
  tckin_grp_ids.insert(tckin_grp_ids.begin(), first_grp_id);

  TTrferRoute trfer;
  int seg_no=0;
  for(TCkinGrpIds::const_iterator grp_id=tckin_grp_ids.begin();grp_id!=tckin_grp_ids.end();grp_id++,seg_no++)
  {
    CheckIn::TPaxGrpItem grp;
    if (!grp.fromDB(*grp_id))
    {
      tckin_grp_ids.clear();
      return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
    };

    TTripInfo operFlt;
    operFlt.getByGrpId(*grp_id);

    TDateTime scd_in=TTripInfo::get_scd_in(grp.point_arv);

    if (grp_id==tckin_grp_ids.begin())
      grp_cat=grp.grpCategory();

    if (grp_cat!=CheckIn::TPaxGrpCategory::UnnacompBag)
    {
      if (grp_id==tckin_grp_ids.begin())
        trfer.GetRoute(grp.id, trtNotFirstSeg);

      if ((paymentStatusReq || groupInfoRes) && grp_id==tckin_grp_ids.begin())
      {
        int trfer_seg_count_pc=0;
        for(TTrferRoute::const_iterator t=trfer.begin(); t!=trfer.end(); ++t, trfer_seg_count_pc++)
          if (!t->piece_concept || !t->piece_concept.get()) break; //������뢠�� ⮫쪮 ���-�� �࠭����� ᥣ���⮢ � ���⮢�� ��⥬�� ����

        TSvcList &svcs_ref=(paymentStatusReq?paymentStatusReq->svcs:groupInfoRes->svcs);
        svcs_ref.set(*grp_id, tckin_route.size()+1, trfer_seg_count_pc+1);
      };

      TGrpMktFlight mktFlight;
      mktFlight.getByGrpId(grp.id);

      TCachedQuery PaxQry(pax_sql, QParams() << QParam("grp_id", otInteger, grp.id));
      PaxQry.get().Execute();


      if (availabilityReq || paymentStatusReq || groupInfoRes)
      {
        std::list<TPaxItem> &paxs_ref=(availabilityReq?availabilityReq->paxs:(paymentStatusReq?paymentStatusReq->paxs:groupInfoRes->paxs));

        list<TPaxItem>::iterator iReqPax=paxs_ref.begin();
        for(;!PaxQry.get().Eof; PaxQry.get().Next())
        {
          CheckIn::TPaxItem pax;
          pax.fromDB(PaxQry.get());
          if (!pax.refuse.empty() && !include_refused) continue;

          if (grp_id==tckin_grp_ids.begin())
            iReqPax=paxs_ref.insert(paxs_ref.end(), TPaxItem());
          if (iReqPax==paxs_ref.end()) throw EXCEPTIONS::Exception("%s: strange situation iReqPax==paxs.end()");
          TPaxItem &reqPax=*iReqPax;

          if (grp_id==tckin_grp_ids.begin())
          {
            std::list<CheckIn::TPaxTransferItem> pax_trfer;
            CheckIn::PaxTransferFromDB(pax.id, pax_trfer);

            TETickItem etick;
            if (pax.tkn.validET())
              etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);

            reqPax.set(pax, etick);
            TTrferRoute::const_iterator s=trfer.begin();
            list<CheckIn::TPaxTransferItem>::const_iterator p=pax_trfer.begin();
            for(int trfer_num=1; s!=trfer.end() && p!=pax_trfer.end(); ++s, ++p, trfer_num++)
            {
              pair< SirenaExchange::TPaxSegMap::iterator, bool > res=
                  reqPax.segs.insert(make_pair(trfer_num,SirenaExchange::TPaxSegItem()));
              if (!res.second) continue;
              SirenaExchange::TPaxSegItem &reqSeg=res.first->second;
              reqSeg.set(trfer_num, *s);
              reqSeg.subcl=p->subclass;
            }
            if (s!=trfer.end()) throw EXCEPTIONS::Exception("%s: strange situation s!=trfer.end()", __FUNCTION__);
            if (p!=pax_trfer.end()) throw EXCEPTIONS::Exception("%s: strange situation p!=pax_trfer.end()", __FUNCTION__);
          };

          pair< SirenaExchange::TPaxSegMap::iterator, bool > res=
              reqPax.segs.insert(make_pair(seg_no,SirenaExchange::TPaxSegItem()));
          SirenaExchange::TPaxSegItem &reqSeg=res.first->second;
          reqSeg.set(seg_no, operFlt, grp.airp_arv, mktFlight, scd_in);
          reqSeg.subcl=pax.subcl;
          reqSeg.tkn=pax.tkn;
          CheckIn::LoadPaxFQT(pax.id, reqSeg.fqts);
          CheckIn::LoadCrsPaxPNRs(pax.id, reqSeg.pnrs);
          ++iReqPax;
        };
      };
    }
    else break;
  };

}

} //namespace SirenaExchange

void CheckInInterface::AfterSaveAction(int first_grp_id, CheckIn::TAfterSaveActionType action)
{
  if (action!=CheckIn::actionCheckPieceConcept) return;

  ProgTrace(TRACE5, "%s started with actionCheckPieceConcept (first_grp_id=%d)", __FUNCTION__, first_grp_id);

  TCachedQuery Qry("SELECT point_dep, piece_concept "
                   "FROM pax_grp "
                   "WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, first_grp_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
  int point_id=Qry.get().FieldAsInteger("point_dep");

  TTripSetList setList;
  setList.fromDB(point_id);
  if (setList.empty()) return;

  if (!Qry.get().FieldIsNULL("piece_concept")) return; //piece_c�ncept 㦥 ��⠭�����

  int bag_types_id=ASTRA::NoExists;
  TCkinGrpIds tckin_grp_ids;
  map<int/*seg_no*/,TBagConcept::Enum> bag_concept_by_seg;
  CheckIn::TPaxGrpCategory::Enum grp_cat;
  TLogLocale event;
  SirenaExchange::TAvailabilityReq req;
  SirenaExchange::fillPaxsBags(first_grp_id, req, grp_cat, tckin_grp_ids);

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (setList.value(tsPieceConcept))
  {
    if (reqInfo->client_type!=ASTRA::ctTerm  ||
        reqInfo->desk.compatible(PIECE_CONCEPT_VERSION))
    {
      if (tckin_grp_ids.empty()) return;

      if (grp_cat==CheckIn::TPaxGrpCategory::Passenges)
      {
        SirenaExchange::TAvailabilityRes res;
//        res.setSrcFile("svc_availability_res.xml");
//        res.setDestFile("svc_availability_res.xml");
        try
        {
          if (!req.paxs.empty())
          {
            SirenaExchange::SendRequest(req, res);
            if (res.empty()) throw EXCEPTIONS::Exception("%s: strange situation: res.empty()", __FUNCTION__);
            if (req.paxs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: req.paxs.empty()", __FUNCTION__);
            const SirenaExchange::TPaxSegMap &segs=req.paxs.front().segs;
            if (segs.empty()) throw EXCEPTIONS::Exception("%s: strange situation: segs.empty()", __FUNCTION__);
            boost::optional<TBagConcept::Enum> bag_concept=TBagConcept::Unknown;
            bag_concept=boost::none; //���檮� �襭��, �� �� ��� ⮣� �⮡� �� ��뢠�� �訡�� error: ?*((void*)& bag_concept +4)? may be used uninitialized in this function
            for(SirenaExchange::TPaxSegMap::const_iterator s=segs.begin(); s!=segs.end(); ++s)
            {
              string flight_view=GetTripName(s->second.operFlt, ecCkin);
              //�஡������� �� ᥣ���⠬
              boost::optional<TBagConcept::Enum> seg_concept;
              if (!res.identical_concept(s->second.id, false, seg_concept)) //�� ��ࢮ� �⠯� ����஫��㥬 ⮫쪮 ���楯� ������ !!!��⮬ ����
              {
                if ((unsigned)s->second.id<tckin_grp_ids.size())
                  //⮫쪮 �᫨ ᪢����� ᥣ����
                  throw UserException("MSG.CHECKIN.DIFFERENT_BAG_CONCEPT_ON_SEGMENT",
                                      LParams()<<LParam("flight", flight_view));
                seg_concept=TBagConcept::Unknown;
              }
              if (!seg_concept) throw EXCEPTIONS::Exception("%s: strange situation: unknown concept for seg_id=%d", __FUNCTION__, s->second.id);
              if (seg_concept.get()==TBagConcept::No)
              {
                //��।��塞 piece ��� weight �� ������/������⢨� RFISC
                seg_concept=res.exists_rfisc(s->second.id, TServiceType::BaggageCharge)?TBagConcept::Piece:TBagConcept::Weight;
              }
              bag_concept_by_seg.insert(make_pair(s->second.id, seg_concept.get()));

              if ((unsigned)s->second.id<tckin_grp_ids.size())
              {
                //⮫쪮 �᫨ ᪢����� ᥣ����
                if (!bag_concept)
                  bag_concept=seg_concept.get();
                else
                {
                  if (seg_concept.get()==TBagConcept::Unknown)
                    throw UserException("MSG.TRANSFER_FLIGHT.NOT_MADE_TCHECKIN.UNKNOWN_BAG_CONCEPT",
                                        LParams()<<LParam("flight", flight_view));

                  if (seg_concept.get()!=bag_concept.get())
                    throw UserException("MSG.TRANSFER_FLIGHT.NOT_MADE_TCHECKIN.DIFFERENT_BAG_CONCEPT",
                                        LParams()<<LParam("flight", flight_view));
                };
              };
            };
            if (!bag_concept) throw EXCEPTIONS::Exception("%s: strange situation: unknown bag_concept", __FUNCTION__);

            if (bag_concept.get()==TBagConcept::Piece)
            {
              if (req.paxs.size()>9)
                throw UserException("MSG.CHECKIN.PIECE_CONCEPT_NOT_SUPPORT_MORE_9_PASSENGERS");
              boost::optional<TRFISCList> rfisc_list;
              if (!res.identical_rfisc_list(segs.begin()->second.id, rfisc_list))  //�஢��塞 �����筮��� ⮫쪮 �������� RFISC � ⮫쪮 �� ��ࢮ� ᥣ���� !!!��⮬ ����
              {
                string flight_view=GetTripName(segs.begin()->second.operFlt, ecCkin);
                throw UserException("MSG.CHECKIN.DIFFERENT_RFISC_LISTS_ON_SEGMENT",
                                    LParams()<<LParam("flight", flight_view));
              };
              if (!rfisc_list)
                throw EXCEPTIONS::Exception("%s: strange situation: unknown rfisc_list!", __FUNCTION__);
              bag_types_id=rfisc_list.get().toDBAdv(true);
            };
            res.rfiscsToDB(tckin_grp_ids, true); //�� ��ࢮ� �⠯� �ਬ��塞 ⮫쪮 ���楯� ������ (old_version=true) !!!��⮬ ����
            res.normsToDB(tckin_grp_ids);
            res.brandsToDB(tckin_grp_ids);
          }
        }
        catch(UserException &e)
        {
          throw;
        }
        catch(std::exception &e)
        {
          bag_concept_by_seg.clear();
          bag_types_id=ASTRA::NoExists;
          if (res.error() &&
              (res.error_code=="1" || res.error_message=="Incorrect format") &&
              res.error_reference.path=="passenger/ticket/number" &&
              !res.error_reference.value.empty())
          {
            event.lexema_id="EVT.DUE_TO_TICKET_NUMBER_WEIGHT_CONCEPT_APPLIED";
            event.prms << PrmSmpl<string>("ticket_no", res.error_reference.value);
            event.ev_type=ASTRA::evtPax;

            showErrorMessage("MSG.DUE_TO_TICKET_NUMBER_WEIGHT_CONCEPT_APPLIED",
                             LParams()<<LParam("ticket_no", res.error_reference.value));
          }
          else
          {
            ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());

            event.lexema_id="EVT.ERROR_CHECKING_PIECE_CONCEPT_WEIGHT_CONCEPT_APPLIED";
            event.ev_type=ASTRA::evtPax;

            showErrorMessage("MSG.ERROR_CHECKING_PIECE_CONCEPT_WEIGHT_CONCEPT_APPLIED");
          };
        }
      }
    }
    else throw UserException("MSG.TERM_VERSION.PIECE_CONCEPT_NOT_SUPPORTED");
  };

  if (grp_cat!=CheckIn::TPaxGrpCategory::UnnacompBag)
    req.bagTypesToDB(tckin_grp_ids);  //������塞 ��ᮢ묨 ⨯��� ������
  else
    unaccBagTypesToDB(first_grp_id);

  TCachedQuery GrpQry("BEGIN "
                      "  UPDATE pax_grp "
                      "  SET piece_concept=:piece_concept, bag_types_id=:bag_types_id "
                      "  WHERE grp_id=:grp_id "
                      "  RETURNING point_dep INTO :point_id; "
                      "END;",
                      QParams() << QParam("grp_id", otInteger)
                                << QParam("piece_concept", otInteger)
                                << QParam("bag_types_id", otInteger)
                                << QParam("point_id", otInteger));
  TCachedQuery TrferQry("UPDATE transfer SET piece_concept=:piece_concept WHERE grp_id=:grp_id AND transfer_num=:transfer_num",
                        QParams() << QParam("grp_id", otInteger)
                                  << QParam("transfer_num", otInteger)
                                  << QParam("piece_concept", otInteger));

  TCkinGrpIds::const_iterator grp_id=tckin_grp_ids.begin();
  if (bag_concept_by_seg.empty())
  {
    for(; grp_id!=tckin_grp_ids.end(); ++grp_id)
    {
      GrpQry.get().SetVariable("grp_id", *grp_id);
      GrpQry.get().SetVariable("piece_concept", (int)false);
      GrpQry.get().SetVariable("bag_types_id", FNull);
      GrpQry.get().SetVariable("point_id", FNull);
      GrpQry.get().Execute();

      if (!event.lexema_id.empty() &&
          !GrpQry.get().VariableIsNULL("point_id"))
      {
        event.id1=GrpQry.get().GetVariableAsInteger("point_id");
        event.id3=*grp_id;
        reqInfo->LocaleToLog(event);
      };
    }
  }
  else
  {
    map<int/*seg_no*/,TBagConcept::Enum>::const_iterator i=bag_concept_by_seg.begin();
    for(int seg_no=0; grp_id!=tckin_grp_ids.end(); ++grp_id, seg_no++)
    {
      if (i==bag_concept_by_seg.end())
        throw EXCEPTIONS::Exception("%s: strange situation: i==bag_concept_by_seg.end()!", __FUNCTION__);
      if (i->first!=seg_no)
        throw EXCEPTIONS::Exception("%s: strange situation: i->first=%d seg_no=%d!", __FUNCTION__, i->first, seg_no);
      GrpQry.get().SetVariable("grp_id", *grp_id);
      GrpQry.get().SetVariable("piece_concept", (int)(i->second==TBagConcept::Piece));
      bag_types_id==ASTRA::NoExists?GrpQry.get().SetVariable("bag_types_id", FNull):
                                    GrpQry.get().SetVariable("bag_types_id", bag_types_id);
      GrpQry.get().SetVariable("point_id", FNull);
      GrpQry.get().Execute();
      ++i;

      for(map<int/*seg_no*/,TBagConcept::Enum>::const_iterator j=i; j!=bag_concept_by_seg.end(); ++j)
      {
        TrferQry.get().SetVariable("grp_id", *grp_id);
        TrferQry.get().SetVariable("transfer_num", j->first-seg_no);
        if (j->second==TBagConcept::Unknown)
          TrferQry.get().SetVariable("piece_concept", FNull);
        else
          TrferQry.get().SetVariable("piece_concept", (int)(j->second==TBagConcept::Piece));
        TrferQry.get().Execute();
      };

      if (!event.lexema_id.empty() &&
          !GrpQry.get().VariableIsNULL("point_id"))
      {
        event.id1=GrpQry.get().GetVariableAsInteger("point_id");
        event.id3=*grp_id;
        reqInfo->LocaleToLog(event);
      };
    };
  }
}

void CheckInInterface::LoadPax(int grp_id, xmlNodePtr resNode, bool afterSavePax)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr node;
  TCkinGrpIds tckin_grp_ids;
  TCkinRoute tckin_route;
  tckin_route.GetRouteAfter(grp_id, crtNotCurrent, crtOnlyDependent);
  tckin_route.get(tckin_grp_ids);
  tckin_grp_ids.insert(tckin_grp_ids.begin(), grp_id);

  bool trfer_confirm=true;
  string pax_cat_airline;
  set<string> pax_cats;
  vector<CheckIn::TTransferItem> segs;
  xmlNodePtr segsNode=NewTextChild(resNode,"segments");
  int first_point_dep=NoExists;
  for(TCkinGrpIds::const_iterator grp_id=tckin_grp_ids.begin();grp_id!=tckin_grp_ids.end();grp_id++)
  {
    list<WeightConcept::TBagNormInfo> all_norms;
    ostringstream norms_view;
    string used_norms_airline_mark;

    CheckIn::TPaxGrpItem grp;
    if (!grp.fromDB(*grp_id))
    {
      if (grp_id==tckin_grp_ids.begin() && !afterSavePax)
        throw AstraLocale::UserException("MSG.PAX_GRP_OR_LUGGAGE_NOT_CHECKED_IN");
      return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
    };

    if (grp.pc && !reqInfo->desk.compatible(PIECE_CONCEPT_VERSION))
      throw UserException("MSG.TERM_VERSION.PIECE_CONCEPT_NOT_SUPPORTED");

    if (grp_id==tckin_grp_ids.begin())
    {
      first_point_dep=grp.point_dep;
      trfer_confirm=grp.trfer_confirm;
    }

    TTripInfo operFlt;
    operFlt.getByGrpId(*grp_id);

    CheckIn::TTransferItem seg;
    seg.operFlt=operFlt;
    seg.grp_id=grp.id;
    seg.airp_arv=grp.airp_arv;

    xmlNodePtr segNode=NewTextChild(segsNode,"segment");
    xmlNodePtr operFltNode;
    if (reqInfo->desk.compatible(PAD_VERSION))
      operFltNode=NewTextChild(segNode,"tripheader");
    else
      operFltNode=segNode;
    TripsInterface::readOperFltHeader( operFlt, operFltNode );
    readTripData( grp.point_dep, first_point_dep, segNode );

    if (grp.cl.empty() && grp.status==psCrew)
      grp.cl=EncodeClass(Y);  //crew compatible
    grp.toXML(segNode);
    //��த �����祭��
    NewTextChild(segNode,"city_arv");
    try
    {
      TAirpsRow& airpsRow=(TAirpsRow&)base_tables.get("airps").get_row("code",grp.airp_arv);
      TCitiesRow& citiesRow=(TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
      ReplaceTextChild( segNode, "city_arv", citiesRow.code );
      if (!reqInfo->desk.compatible(PAD_VERSION))
        ReplaceTextChild( segNode, "city_arv_name", citiesRow.name );
    }
    catch(EBaseTableError) {};
    //�����
    if (!reqInfo->desk.compatible(PAD_VERSION))
    {
      try
      {
        TClassesRow& classesRow=(TClassesRow&)base_tables.get("classes").get_row("code",grp.cl);
        ReplaceTextChild( segNode, "class_name", classesRow.name );
      }
      catch(EBaseTableError) {};
    };

    CheckIn::TGroupBagItem group_bag;
    TGrpServiceList svc;
    if (grp_id==tckin_grp_ids.begin())
    {
      group_bag.fromDB(grp.id, ASTRA::NoExists, !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS));
      svc.fromDB(grp.id);
    };

    TTrferRoute trfer;
    if (!grp.is_unaccomp())
    {
      TGrpMktFlight mktFlight;
      if (mktFlight.getByGrpId(grp.id))
      {
        mktFlight.toXML(segNode);
        if (mktFlight.pr_mark_norms && mktFlight.airline!=seg.operFlt.airline)
          used_norms_airline_mark=mktFlight.airline;
      };

      TQuery PaxQry(&OraSession);
      PaxQry.Clear();
      PaxQry.SQLText=pax_sql;
      PaxQry.CreateVariable("grp_id",otInteger,grp.id);

      PaxQry.Execute();

      if (grp_id==tckin_grp_ids.begin())
      {
        trfer.GetRoute(grp.id, trtWithFirstSeg);
        BuildTransfer(trfer, trtNotFirstSeg, resNode);
        BuildTCkinSegments(grp.id, resNode);
        NewTextChild(resNode, "show_wt_details", (int)grp.wt, (int)false);
      }

      TPrPrint prPrint;
      node=NewTextChild(segNode,"passengers");
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        CheckIn::TPaxItem pax;
        pax.fromDB(PaxQry);

        CheckIn::TPaxTransferItem paxTrferItem;
        paxTrferItem.pax_id=pax.id;
        paxTrferItem.subclass=pax.subcl;
        seg.pax.push_back(paxTrferItem);

        xmlNodePtr paxNode=NewTextChild(node, "pax");
        if (grp.status==psCrew)
        {
          pax.subcl=" ";       //crew compatible
          pax.tkn.no="-";      //crew compatible
          pax.tkn.rem="TKNA";  //crew compatible
        };
        pax.toXML(paxNode);
        if (pax.tkn.validET())
          NewTextChild(paxNode, "ticket_bag_norm",
                       lowerc(TETickItem().fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false).bag_norm_view()), "");
        NewTextChild(paxNode,"pr_norec",(int)PaxQry.FieldIsNULL("crs_pax_id"));

        bool pr_bp_print, pr_bi_print;
        prPrint.get_pr_print(*grp_id, pax.id, pr_bp_print, pr_bi_print);
        NewTextChild(paxNode,"pr_bp_print",pr_bp_print);
        NewTextChild(paxNode,"pr_bi_print",pr_bi_print);
        if (grp_id==tckin_grp_ids.begin())
        {
          std::list<CheckIn::TPaxTransferItem> pax_trfer;
          CheckIn::PaxTransferFromDB(pax.id, pax_trfer);
          CheckIn::PaxTransferToXML(pax_trfer, paxNode);
          TPaxServiceLists().toXML(pax.id, false, tckin_grp_ids.size(), NewTextChild(paxNode, "service_lists"));
        };
        LoadPaxRem(paxNode);
        if (grp_id==tckin_grp_ids.begin())
        {
          if (grp.wt)
          {
            list< pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
            WeightConcept::PaxNormsFromDB(NoExists, pax.id, norms);
            WeightConcept::NormsToXML(norms, group_bag.trferExists(), paxNode);
            if (pax.refuse.empty())
              all_norms.insert(all_norms.end(), norms.begin(), norms.end());
          };
          Sirena::PaxBrandsNormsToStream(trfer, pax, norms_view);

          pax_cat_airline=seg.operFlt.airline;
          AddPaxCategory(pax, pax_cats);
        };
      };
    }
    else
    {
      //��ᮯ஢������� �����
      if (grp_id==tckin_grp_ids.begin())
      {
        trfer.GetRoute(grp.id, trtWithFirstSeg);
        BuildTransfer(trfer, trtNotFirstSeg, resNode);
        BuildTCkinSegments(grp.id, resNode);
        TPaxServiceLists().toXML(grp.id, true, tckin_grp_ids.size(), NewTextChild(segNode, "service_lists"));
        if (grp.wt)
        {
          list< pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
          WeightConcept::GrpNormsFromDB(NoExists, grp.id, norms);
          WeightConcept::NormsToXML(norms, group_bag.trferExists(), segNode);
          all_norms.insert(all_norms.end(), norms.begin(), norms.end());
        };
      };
    };
    if (grp_id==tckin_grp_ids.begin())
    {
      group_bag.toXML(resNode);

      CheckIn::TServicePaymentList payment;
      payment.fromDB(grp.id);
      payment.toXML(segNode);

      TPaidRFISCViewMap PaidRFISCViewMap;
      if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION) || grp.pc)
      {
        //��㣨 + pc
        svc.toXML(resNode);
        TPaidRFISCList PaidRFISCList;
        PaidRFISCList.fromDB(grp.id, true);
        CalcPaidRFISCView(PaidRFISCList, PaidRFISCViewMap);
        PaidRFISCViewToXML(PaidRFISCViewMap, resNode);
        NewTextChild(resNode, "norms_view", norms_view.str());
      };

      //wt
      TPaidBagViewMap PaidBagViewMap;
      TPaidBagViewMap TrferBagViewMap;
      if (grp.wt)
      {
        WeightConcept::TPaidBagList paid;
        WeightConcept::PaidBagFromDB(NoExists, grp.id, paid);
        if (reqInfo->client_type==ASTRA::ctTerm && reqInfo->desk.compatible(PIECE_CONCEPT_VERSION))
        {
          map<int/*id*/, TEventsBagItem> tmp_bag;
          GetBagToLogInfo(grp.id, tmp_bag);
          WeightConcept::TAirlines airlines(grp.id,
                                            used_norms_airline_mark.empty()?seg.operFlt.airline:used_norms_airline_mark,
                                            "CalcPaidBagView");
          WeightConcept::CalcPaidBagView(airlines, tmp_bag, all_norms, paid, payment, used_norms_airline_mark,
                                         PaidBagViewMap, TrferBagViewMap);
          WeightConcept::PaidBagViewToXML(PaidBagViewMap, TrferBagViewMap, resNode);
        }
        else
          WeightConcept::PaidBagToXML(paid, group_bag.trferExists(), resNode);
      };

      //⠡��窠 ���⭮��
      GridInfoToXML(trfer, PaidRFISCViewMap, PaidBagViewMap, TrferBagViewMap, resNode);

      readTripCounters(grp.point_dep, segNode);

      if (afterSavePax)
      {
        //⮫쪮 ��᫥ ����� ���������
        readTripCounters(grp.point_dep, resNode);
        if (!reqInfo->desk.compatible(PAD_VERSION))
        {
          //pr_etl_only
          readTripSets( grp.point_dep, operFlt, NewTextChild(resNode,"trip_sets") );
        };
      };
    };

    readTripSets( grp.point_dep, operFlt, operFltNode );

    segs.push_back(seg);
  };
  if (!trfer_confirm)
  {
    if (!reqInfo->desk.compatible(INA_BUGFIX_VERSION))
      ShowPaxCatWarning(pax_cat_airline, pax_cats);
    //ᮡ�ࠥ� ���ଠ�� � �����⢥ত����� �࠭���
    if (!LoadUnconfirmedTransfer(segs, resNode) &&
        !reqInfo->desk.compatible(NOT_F9_WARNING_VERSION))
      showErrorMessage("MSG.BAGGAGE_INPUT_ONLY_AFTER_CHECKIN");  
  };
};

void CheckInInterface::LoadPaxRem(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  LoadPaxRemAndASVC(pax_id, paxNode, false);
  LoadPaxFQT(pax_id, paxNode, false);
}

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

void CheckInInterface::SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const map<int,TSegInfo> &segs, int seg_no, TLogLocale& tlocale)
{
  tlocale.lexema_id.clear();
  tlocale.prms.clearPrms();
  if (segsNode==NULL) return;

  xmlNodePtr segNode=GetNode("segments",segsNode);
  if (segNode==NULL) return;

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
    if (Qry.GetVariableAsInteger("rows")>0) {
      tlocale.lexema_id = "EVT.THROUGH_CHECKIN_CANCEL";
      return;
    }
    else
      return;
  };

  for(;segNode!=NULL&&seg_no>1;segNode=segNode->next,seg_no--);
  if (segNode==NULL) return;
  segNode=segNode->next;
  if (segNode==NULL) return;

  Qry.Clear();
  Qry.SQLText=
    "DECLARE "
    "  pass BINARY_INTEGER; "
    "BEGIN "
    "  :bind_flt:=0; "
    "  FOR pass IN 1..2 LOOP "
    "    BEGIN "
    "      SELECT point_id INTO :point_id_trfer FROM trfer_trips "
    "      WHERE scd=:scd AND airline=:airline AND flt_no=:flt_no AND airp_dep=:airp_dep AND "
    "            (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) FOR UPDATE; "
    "      EXIT; "
    "    EXCEPTION "
    "      WHEN NO_DATA_FOUND THEN "
    "        SELECT cycle_id__seq.nextval INTO :point_id_trfer FROM dual; "
    "        BEGIN "
    "          INSERT INTO trfer_trips(point_id,airline,flt_no,suffix,scd,airp_dep,point_id_spp) "
    "          VALUES (:point_id_trfer,:airline,:flt_no,:suffix,:scd,:airp_dep,NULL); "
    "          :bind_flt:=1; "
    "          EXIT; "
    "        EXCEPTION "
    "          WHEN DUP_VAL_ON_INDEX THEN "
    "            IF pass=1 THEN NULL; ELSE RAISE; END IF; "
    "        END; "
    "    END; "
    "  END LOOP; "
    "  INSERT INTO tckin_segments(grp_id,seg_no,point_id_trfer,airp_arv,pr_final) "
    "  VALUES(:grp_id,:seg_no,:point_id_trfer,:airp_arv,:pr_final); "
    "END;";
  Qry.DeclareVariable("bind_flt",otInteger);
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

  tlocale.lexema_id = "EVT.THROUGH_CHECKIN_PERFORMED";
  PrmEnum route("route", "");
  for(seg_no=1;segNode!=NULL;segNode=segNode->next,seg_no++)
  {
    map<int,TSegInfo>::const_iterator s=segs.find(NodeAsInteger("point_dep",segNode));
    if (s==segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SaveTCkinSegs: point_id not found in map segs");

    const TTripInfo &fltInfo=s->second.fltInfo;
    TDateTime local_scd=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));
    modf(local_scd,&local_scd);

    Qry.SetVariable("seg_no",seg_no);
    Qry.SetVariable("airline",fltInfo.airline);
    Qry.SetVariable("flt_no",fltInfo.flt_no);
    Qry.SetVariable("suffix",fltInfo.suffix);
    Qry.SetVariable("scd",local_scd);
    Qry.SetVariable("airp_dep",fltInfo.airp);
    Qry.SetVariable("airp_arv",s->second.airp_arv);
    Qry.SetVariable("pr_final",(int)(segNode->next==NULL));
    Qry.Execute();
    if (Qry.GetVariableAsInteger("bind_flt")!=0)
    {
      int point_id_trfer=Qry.GetVariableAsInteger("point_id_trfer");
      TTrferBinding().bind_flt(point_id_trfer);
    };

    route.prms << PrmSmpl<string>("", " -> ") << PrmFlight("", fltInfo.airline, fltInfo.flt_no, fltInfo.suffix)
               << PrmSmpl<string>("", "/") << PrmDate("", local_scd,"dd") << PrmSmpl<string>("", ":")
               << PrmElem<string>("",  etAirp, fltInfo.airp) << PrmSmpl<string>("", "-")
               << PrmElem<string>("", etAirp, s->second.airp_arv);
  };
  tlocale.prms << route;
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

    //������������
    strh=NodeAsStringFast("airline",node2);
    seg.operFlt.airline=ElemToElemId(etAirline,strh,seg.operFlt.airline_fmt);
    if (seg.operFlt.airline_fmt==efmtUnknown)
      throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRLINE",
                          LParams()<<LParam("airline",strh)
                                   <<LParam("flight",flt.str()));

    //����� ३�
    seg.operFlt.flt_no=NodeAsIntegerFast("flt_no",node2);

    //���䨪�
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
      TDateTime base_date=local_scd-1; //��⠬��� ����� �� ������ ����� � ���ਪ� �� ���譨� ����
      local_scd=DayToDate(local_date,base_date,false); //�����쭠� ��� �뫥�
      seg.operFlt.scd_out=local_scd;
    }
    catch(EXCEPTIONS::EConvertError &E)
    {
      throw UserException("MSG.TRANSFER_FLIGHT.INVALID_LOCAL_DATE_DEP",
                          LParams()<<LParam("flight",flt.str()));
    };

    //��ய��� �뫥�
    if (GetNodeFast("airp_dep",node2)!=NULL)  //����� �/� �뫥�
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

    //��ய��� �ਫ��
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

  //�࠭���� ��������� ���ᠦ�஢
  for(paxNode=paxNode->children;paxNode!=NULL;paxNode=paxNode->next)
  {
    xmlNodePtr paxTrferNode=NodeAsNode("transfer",paxNode)->children;
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
      //����� �����: ���-�� �࠭����� �������ᮢ ���ᠦ�� �� ᮢ������ � ���-��� �࠭����� ᥣ���⮢
      throw EXCEPTIONS::Exception("ParseTransfer: Different number of transfer subclasses and transfer segments");
    };
  };
};

void CheckInInterface::SaveTransfer(int grp_id,
                                      const vector<CheckIn::TTransferItem> &trfer,
                                      const map<int, pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs,
                                      bool pr_unaccomp, int seg_no, TLogLocale& tlocale)
{
  tlocale.lexema_id.clear();
  tlocale.prms.clearPrms();

  map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator s=trfer_segs.find(seg_no);
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

  if (firstTrfer==trfer.end()) //��祣� �� �����뢠�� � ����
  {
    if (TrferQry.GetVariableAsInteger("rows")>0) {
        tlocale.lexema_id = "EVT.CHECKIN.TRANSFER_BAGGAGE_CANCEL";
        return;
    }
    else return;
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

  TAdvTripInfo fltInfo(TrferQry);

  //�஢�ઠ �ନ஢���� �࠭��� � ��⨭᪨� ⥫��ࠬ���
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
        (iTlgs->second==checkFirstSeg && checkType==checkFirstSeg)) continue;

    TypeB::TCreator creator(fltInfo);
    creator << iTlgs->first;
    vector<TypeB::TCreateInfo> createInfo;
    creator.getInfo(createInfo);
    vector<TypeB::TCreateInfo>::const_iterator i=createInfo.begin();
    for(; i!=createInfo.end(); ++i)
      if (i->get_options().is_lat) break;
    if (i==createInfo.end()) continue;

    checkType=iTlgs->second;
  };

  TrferQry.Clear();
  TrferQry.SQLText=
    "DECLARE "
    "  pass BINARY_INTEGER; "
    "BEGIN "
    "  :bind_flt:=0; "
    "  FOR pass IN 1..2 LOOP "
    "    BEGIN "
    "      SELECT point_id INTO :point_id_trfer FROM trfer_trips "
    "      WHERE scd=:scd AND airline=:airline AND flt_no=:flt_no AND airp_dep=:airp_dep AND "
    "            (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) FOR UPDATE; "
    "      EXIT; "
    "    EXCEPTION "
    "      WHEN NO_DATA_FOUND THEN "
    "        SELECT cycle_id__seq.nextval INTO :point_id_trfer FROM dual; "
    "        BEGIN "
    "          INSERT INTO trfer_trips(point_id,airline,flt_no,suffix,scd,airp_dep,point_id_spp) "
    "          VALUES (:point_id_trfer,:airline,:flt_no,:suffix,:scd,:airp_dep,NULL); "
    "          :bind_flt:=1; "
    "          EXIT; "
    "        EXCEPTION "
    "          WHEN DUP_VAL_ON_INDEX THEN "
    "            IF pass=1 THEN NULL; ELSE RAISE; END IF; "
    "        END; "
    "    END; "
    "  END LOOP; "
    "  INSERT INTO transfer(grp_id,transfer_num,point_id_trfer, "
    "    airline_fmt,suffix_fmt,airp_dep_fmt,airp_arv,airp_arv_fmt,pr_final) "
    "  VALUES(:grp_id,:transfer_num,:point_id_trfer, "
    "    :airline_fmt,:suffix_fmt,:airp_dep_fmt,:airp_arv,:airp_arv_fmt,:pr_final); "
    "END;";
  TrferQry.DeclareVariable("bind_flt",otInteger);
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
  int trfer_num=1;
  PrmEnum route("route", "");
  for(vector<CheckIn::TTransferItem>::const_iterator t=firstTrfer;t!=trfer.end();++t,trfer_num++)
  {
    if (checkType==checkAllSeg ||
        (checkType==checkFirstSeg && t==firstTrfer))
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
    //�஢�ਬ ࠧ�襭� �� ��ଫ���� �࠭���
    if (s==trfer_segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SaveTransfer: wrong trfer_segs");
    if (!s->second.second.trfer_permit)
      throw UserException("MSG.TRANSFER_FLIGHT.NOT_MADE_TRANSFER",
                          LParams()<<LParam("flight",t->flight_view));
    s++;

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
    if (TrferQry.GetVariableAsInteger("bind_flt")!=0)
    {
      int point_id_trfer=TrferQry.GetVariableAsInteger("point_id_trfer");
      TTrferBinding().bind_flt(point_id_trfer);
    };

    tlocale.lexema_id = "EVT.CHECKIN.TRANSFER_BAGGAGE_REGISTER";
    route.prms << PrmSmpl<string>("", " -> ") << PrmFlight("", t->operFlt.airline, t->operFlt.flt_no, t->operFlt.suffix)
               << PrmSmpl<string>("", "/") << PrmDate("", t->operFlt.scd_out,"dd") << PrmSmpl<string>("", ":")
               << PrmElem<string>("",  etAirp, t->operFlt.airp) << PrmSmpl<string>("", "-") << PrmElem<string>("", etAirp, t->airp_arv);

    airline_in=t->operFlt.airline;
    flt_no_in=t->operFlt.flt_no;
  };
  tlocale.prms << route;
};

void CheckInInterface::BuildTransfer(const TTrferRoute &trfer, TTrferRouteType route_type, xmlNodePtr transferNode)
{
  if (transferNode==NULL) return;

  xmlNodePtr node=NewTextChild(transferNode,"transfer");

  int iDay,iMonth,iYear;
  for(TTrferRoute::const_iterator t=trfer.begin();t!=trfer.end();++t)
  {
    if (t==trfer.begin() && route_type==trtNotFirstSeg) continue;
    xmlNodePtr trferNode=NewTextChild(node,"segment");
    NewTextChild(trferNode,"flight_short",t->operFlt.flight_view());
    NewTextChild(trferNode,"airline",
                 ElemIdToClientElem(etAirline, t->operFlt.airline, t->operFlt.airline_fmt));

    NewTextChild(trferNode,"flt_no",t->operFlt.flt_no);

    if (!t->operFlt.suffix.empty())
      NewTextChild(trferNode,"suffix",
                   ElemIdToClientElem(etSuffix, t->operFlt.suffix, t->operFlt.suffix_fmt));
    else
      NewTextChild(trferNode,"suffix");

    //���
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

void CheckInInterface::BuildTCkinSegments(int grp_id, xmlNodePtr tckinNode)
{
  if (tckinNode==NULL) return;

  xmlNodePtr node=NewTextChild(tckinNode,"tckin_segments");

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline, flt_no, suffix, scd AS scd_out, "
    "       airp_dep AS airp, airp_arv, seg_no "
    "FROM tckin_segments,trfer_trips "
    "WHERE tckin_segments.point_id_trfer=trfer_trips.point_id AND "
    "      grp_id=:grp_id AND seg_no>0 "
    "ORDER BY seg_no";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTripInfo flt(Qry);
    int seg_no=Qry.FieldAsInteger("seg_no");

    xmlNodePtr segNode=NewTextChild(node,"segment");
    SetProp(segNode, "num", seg_no);
    NewTextChild(segNode, "flight_short", flt.flight_view());
  };
}

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
    NewTextChild( itemNode, "free_ok", Qry.FieldAsInteger( "free_ok" ) );         //�� �ᯮ������
    NewTextChild( itemNode, "free_goshow", Qry.FieldAsInteger( "free_goshow" ) ); //�� �ᯮ������
    NewTextChild( itemNode, "nooccupy", Qry.FieldAsInteger( "nooccupy" ) );       //�� �ᯮ������
  };

  int load_residue=getCommerceWeight( point_id, onlyCheckin, CWResidual );
  if (load_residue!=NoExists)
    NewTextChild(dataNode,"load_residue",load_residue);
  else
    NewTextChild(dataNode,"load_residue");
}

void CheckInInterface::readTripData(int point_id, int first_point_id, xmlNodePtr dataNode)
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode,node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out, "
    "       point_num, first_point, pr_tranzit "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
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

  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(TTripRoute::iterator r=route.begin();r!=route.end();r++)
  {
    //�஢�ਬ �� �㡫�஢���� ����� ��ய��⮢ � ࠬ��� ������ ३�
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

      bool notCheckAPI=false;
      if (point_id!=first_point_id)
      {
        TTripInfo firstOperFlt;
        firstOperFlt.getByPointId(first_point_id);
        notCheckAPI=GetTripSets(tsAPISControlOnFirstSegOnly, firstOperFlt);
      };

      TCompleteAPICheckInfo checkInfo;
      if (point_id==first_point_id || !notCheckAPI)
        checkInfo.set(point_id, airpsRow.code);
      if (TReqInfo::Instance()->desk.compatible(DOCA_VERSION))
      {
        checkInfo.toXML(NewTextChild(itemNode, "check_info"));
      }
      else
      {
        checkInfo.pass().get(apiDoc).toXML(NewTextChild( itemNode, "check_doc_info"));
        checkInfo.pass().get(apiDoco).toXML(NewTextChild( itemNode, "check_doco_info"));
        checkInfo.pass().get(apiTkn).toXML(NewTextChild( itemNode, "check_tkn_info"));
      };
      airps.push_back(airpsRow.code);
    }
    catch(EBaseTableError) {};
  };

  TCFG cfg(point_id);
  bool cfg_exists=!cfg.empty();
  bool free_seating=false;
  if (!cfg_exists)
    free_seating=SALONS2::isFreeSeating(point_id);
  if (!cfg_exists && free_seating) cfg.get(NoExists);
  node = NewTextChild( tripdataNode, "classes" );
  for(TCFG::const_iterator c=cfg.begin(); c!=cfg.end(); ++c)
  {
    itemNode = NewTextChild( node, "class" );
    NewTextChild( itemNode, "code", c->cls );
    if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
      NewTextChild( itemNode, "class_view", ElemIdToNameLong(etClass, c->cls) );
    else
      NewTextChild( itemNode, "name", ElemIdToNameLong(etClass, c->cls) );
    NewTextChild( itemNode, "cfg", c->cfg );
  };

  //��室� �� ��ᠤ��, �����祭�� �� ३�
  node = NewTextChild( tripdataNode, "gates" );
  vector<string> gates;
  TripsInterface::readGates(point_id, gates);
  for(vector<string>::iterator iv = gates.begin(); iv != gates.end(); iv++)
    NewTextChild( node, "gate_name", *iv);

  //����
  TripsInterface::readHalls(operFlt.airp, "�", tripdataNode);

  //��室� �� ��ᠤ�� ��� ������� �� ����� ॣ����樨, �����祭�� �� ३�
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
  Qry.CreateVariable("work_mode",otString,"�");
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
    //��⠢�� � ��砫� ���ᨢ� �������᪨� ३ᮢ 䠪��᪨� ३�
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

    NewTextChild( tripSetsNode, "pr_etl_only", (int)GetTripSets(tsETSNoInteract,fltInfo) );
    NewTextChild( tripSetsNode, "pr_etstatus", Qry.FieldAsInteger("pr_etstatus") );
    NewTextChild( tripSetsNode, "pr_no_ticket_check", (int)GetTripSets(tsNoTicketCheck,fltInfo) );
    NewTextChild( tripSetsNode, "pr_auto_pt_print", (int)GetTripSets(tsAutoPTPrint,fltInfo) );
    NewTextChild( tripSetsNode, "pr_auto_pt_print_reseat", (int)GetTripSets(tsAutoPTPrintReseat,fltInfo) );
};

void CheckInInterface::GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readTripCounters(point_id,resNode);
};


//////////// ��� ��⥬� ���ନ஢����

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
  Qry.CreateVariable( "work_mode", otString, "�" );
  Qry.Execute();
  if ( open == 1 )
    TReqInfo::Instance()->LocaleToLog("EVT.CHECKIN_OPEN_FOR_INFO_SYS", evtFlt, point_id);
  else
    TReqInfo::Instance()->LocaleToLog("EVT.CHECKIN_CLOSE_FOR_INFO_SYS", evtFlt, point_id);

};

struct TCkinPaxInfo
{
  string surname,name,pers_type,subclass;
  int seats,reqCount,resCount;
  xmlNodePtr node;
};

void CheckInInterface::GetTCkinFlights(const map<int, CheckIn::TTransferItem> &trfer,
                                       map<int, pair<CheckIn::TTransferItem, TCkinSegFlts> > &segs)
{
  segs.clear();
  if (trfer.empty()) return;

  bool is_edi=false;
  for(map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin();t!=trfer.end();t++)
  {
    TCkinSegFlts seg;

    if (t->second.Valid())
    {
      //�஢�ਬ ���㦨������ �� ३� � ��㣮� DCS
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
        Qry.CreateVariable("airline",otString,t->second.operFlt.airline);
        Qry.CreateVariable("flt_no",otInteger,(const int)(t->second.operFlt.flt_no));
        Qry.Execute();
        if (!Qry.Eof) is_edi=true;
      };

      seg.is_edi=is_edi;

      if (!is_edi)
      {
        TSearchFltInfo filter;
        filter.airline=t->second.operFlt.airline;
        filter.flt_no=t->second.operFlt.flt_no;
        filter.suffix=t->second.operFlt.suffix;
        filter.airp_dep=t->second.operFlt.airp;
        filter.scd_out=t->second.operFlt.scd_out;
        filter.scd_out_in_utc=false;
        filter.only_with_reg=true;

        //�饬 ३� � ���
        list<TAdvTripInfo> flts;
        SearchFlt(filter, flts);

        for(list<TAdvTripInfo>::const_iterator f=flts.begin(); f!=flts.end(); ++f)
        {
          TSegInfo segSPPInfo;
          CheckInInterface::CheckCkinFlight(f->point_id,
                                            f->airp,
                                            ASTRA::NoExists,
                                            t->second.airp_arv,
                                            segSPPInfo);

          if (segSPPInfo.fltInfo.pr_del==ASTRA::NoExists) continue; //�� ��諨 �� point_dep
          seg.flts.push_back(segSPPInfo);
        };
      };
    };
    segs[t->first]=make_pair(t->second, seg);
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
                       firstSeg))
  {
    if (firstSeg.fltInfo.pr_del==0)
      throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  };
  if (firstSeg.fltInfo.pr_del==ASTRA::NoExists ||
      firstSeg.fltInfo.pr_del!=0)
    throw UserException("MSG.FLIGHT.CANCELED.REFRESH_DATA");

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
  //if (!reqInfo->desk.compatible(PAD_VERSION)) � ���쭥�襬 ���� ���� ��ࠡ��뢠�� PAD �� ᪢����� ᥣ�����
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
  map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
  ParseTransfer(trferNode,
                NodeAsNode("passengers",reqNode),
                firstSeg, trfer);
  if (!trfer.empty())
  {
    map<int, CheckIn::TTransferItem> trfer_tmp;
    int trfer_num=1;
    for(vector<CheckIn::TTransferItem>::const_iterator t=trfer.begin();t!=trfer.end();++t, trfer_num++)
      trfer_tmp[trfer_num]=*t;
    traceTrfer(TRACE5, "CheckTCkinRoute: trfer_tmp", trfer_tmp);
    GetTrferSets(firstSeg.fltInfo, firstSeg.airp_arv, "", trfer_tmp, false, trfer_segs);
    traceTrfer(TRACE5, "CheckTCkinRoute: trfer_segs", trfer_segs);
  };

  if (!reqInfo->desk.compatible(PAD_VERSION))
    NewTextChild(resNode,"flight",GetTripName(firstSeg.fltInfo,ecCkin,true,false));
  xmlNodePtr routeNode=NewTextChild(resNode,"tckin_route");
  xmlNodePtr segsNode=NewTextChild(resNode,"tckin_segments");

  vector< pair<TCkinSegmentItem, TTrferSetsInfo> > segs;
  for(trferNode=trferNode->children;trferNode!=NULL;trferNode=trferNode->next)
  {
    xmlNodePtr node2=trferNode->children;

    TCkinSegmentItem segItem;
    segItem.conf_status=(NodeAsIntegerFast("conf_status",node2,0)!=0);
    segItem.calc_status=NodeAsStringFast("calc_status",node2,"");
    segs.push_back(make_pair(segItem,TTrferSetsInfo()));
  };

  if (segs.size()!=trfer_segs.size() ||
      segs.size()!=trfer.size())
    throw EXCEPTIONS::Exception("CheckInInterface::CheckTCkinRoute: different array sizes "
                                "(segs.size()=%zu, trfer_segs.size()=%zu, trfer.size()=%zu",
                                segs.size(),trfer_segs.size(),trfer.size());

  vector< pair<TCkinSegmentItem, TTrferSetsInfo> >::iterator s=segs.begin();
  map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator s2=trfer_segs.begin();
  for(;s!=segs.end() && s2!=trfer_segs.end();s++,s2++)
  {
    s->first.flts.assign(s2->second.first.flts.begin(),s2->second.first.flts.end());
    s->first.is_edi=s2->second.first.is_edi;
    s->second=s2->second.second;
  };

  bool irrelevant_data=false; //���ॢ訥 �����
  bool total_permit=true;
  bool tckin_route_confirm=true;

  //横� �� ��몮���� ᥣ���⠬ � �� �࠭���� ३ᠬ
  vector<CheckIn::TTransferItem>::const_iterator f=trfer.begin();
  for(s=segs.begin();s!=segs.end() && f!=trfer.end();s++,f++)
  {
    if (!s->first.conf_status) tckin_route_confirm=false;


    //�������� ���� ���������� �� ������������ ��������
    xmlNodePtr segNode=NewTextChild(routeNode,"route_segment");
    xmlNodePtr seg2Node=NULL;
    if (tckin_route_confirm)
      seg2Node=NewTextChild(segsNode,"tckin_segment");

    //����������� ��ଫ���� �࠭��୮�� ������
    if (!s->second.trfer_permit)
    {
      SetProp(NewTextChild(segNode,"trfer_permit",AstraLocale::getLocaleText("���")),"value",(int)false);
      if (tckin_route_confirm)
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",(int)false);
    }
    else
    {
      SetProp(NewTextChild(segNode,"trfer_permit","+"),"value",(int)true);
      if (tckin_route_confirm)
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",(int)true);
    };

    //����������� ᪢����� ॣ����樨
    if (!s->second.tckin_permit)
      NewTextChild(segNode,"tckin_permit",AstraLocale::getLocaleText("���"));
    else
      NewTextChild(segNode,"tckin_permit","+");
    if (!s->first.is_edi)
    {
      if (!s->second.tckin_waitlist)
        NewTextChild(segNode,"tckin_waitlist",AstraLocale::getLocaleText("���"));
      else
        NewTextChild(segNode,"tckin_waitlist","+");
      if (!s->second.tckin_norec)
        NewTextChild(segNode,"tckin_norec",AstraLocale::getLocaleText("���"));
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

    //�뢮� ३�
    ostringstream flight;
    flight << ElemIdToCodeNative(etAirline, f->operFlt.airline)
           << setw(3) << setfill('0') << f->operFlt.flt_no
           << ElemIdToCodeNative(etSuffix, f->operFlt.suffix) << "/"
           << DateTimeToStr(f->operFlt.scd_out,"dd") << " "
           << ElemIdToCodeNative(etAirp, f->operFlt.airp) << "-"
           << ElemIdToCodeNative(etAirp, f->airp_arv);

    if (!s->first.flts.empty())
    {
      if (s->first.flts.size()>1)
      {
        //��諨 � ��� ��᪮�쪮 ३ᮢ, ᮮ⢥������� �࠭��୮�� ᥣ�����
        SetProp(ReplaceTextChild(segNode,"classes",AstraLocale::getLocaleText("�㡫� � ���")),"error","CRITICAL");
        SetProp(ReplaceTextChild(segNode,"pnl"),"error");
        SetProp(ReplaceTextChild(segNode,"class"),"error");
        SetProp(ReplaceTextChild(segNode,"free"),"error");
        break;
      }
      else
      {
        //��諨 � ��� ���� ३�, ᮮ⢥�����騩 �࠭��୮�� ᥣ�����

        TQuery Qry(&OraSession);
        const TSegInfo &currSeg=*(s->first.flts.begin());

        NewTextChild(segNode,"flight",flight.str());
        if (tckin_route_confirm)
        {
          xmlNodePtr operFltNode;
          if (reqInfo->desk.compatible(PAD_VERSION))
            operFltNode=NewTextChild(seg2Node,"tripheader");
          else
            operFltNode=seg2Node;
          TripsInterface::readOperFltHeader( currSeg.fltInfo, operFltNode );
          readTripData( currSeg.point_dep, firstSeg.point_dep, seg2Node );

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

        //���⠥� ������
        TCFG cfg(currSeg.point_dep);
        if (cfg.empty())
        {
          //���-�� �� ����ᠬ �������⭮
          xmlNodePtr wlNode=NewTextChild(segNode,"classes",AstraLocale::getLocaleText("���"));
          SetProp(wlNode,"error","WL");
          SetProp(wlNode,"wl_type","C");
        }
        else
        {
          NewTextChild(segNode,"classes",cfg.str());
        };


        //����� ���ᠦ�஢
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
          //���-�� ����襭��� ���ᠦ�஢ � ��������묨 䠬�����, ������, ⨯��,
          //����稥�/������⢨�� ����, �������ᮬ
          paxInfo.reqCount=1;
          paxInfo.resCount=0;
          paxInfo.node=NULL;

          //�饬 ᮮ⢥�����騩 ᥣ����� �������� ���ᠦ��
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
        //���ᨢ pax �� ����� ������ ᮤ�ন� ���ᠦ�஢ � 㭨����묨:
        //1. 䠬�����,
        //2. ������
        //3. ⨯�� ���ᠦ��
        //4. ����稥�/������⢨�� ����
        //5. �������ᮬ
        //pax.reqCount ᮤ�ন� ���-�� ����������� ���ᠦ�஢


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
              //�� 㦥 ��᫥ ���⢥ত���� �������
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
            //㧭��� ������ ������ �ਭ������� �������� �� ⥫��ࠬ�
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



          //resCount ᮤ�ন� ���-�� ॠ�쭮 ��������� ���ᠦ�஢
          if (iPax->resCount<=iPax->reqCount)
          {
            //��諨 ����� ��� ࠢ��� ���-�� ���ᠦ�஢, ᮮ⢥������� ���-�� ����������� ���ᠦ�஢
            paxCountInPNL+=iPax->resCount;
          }
          else
          {
            //��諨 ��譨� ��
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
            //�饬 ᮮ⢥�����騩 ᥣ����� �������� ���ᠦ��
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
                  //�������� norec
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

        //����稥 � PNL
        if (paxCountInPNL<paxCount)
          SetProp(NewTextChild(segNode,"pnl",
                               IntToString(paxCountInPNL)+" "+AstraLocale::getLocaleText("��")+" "+IntToString(paxCount)),
                  "error","NOREC");
        else
          if (doublePax)
            SetProp(NewTextChild(segNode,"pnl",AstraLocale::getLocaleText("�㡫� � PNL")),"error","DOUBLE");
          else
            NewTextChild(segNode,"pnl","+");


        if (cl.size()==1)
        {
          NewTextChild(segNode,"class",ElemIdToCodeNative(etClass,cl));

          //����襬 ����� �᫨ ���⢥ত���� �������
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
            SetProp(NewTextChild(segNode,"class",AstraLocale::getLocaleText("���")),"error","CRITICAL");
          else
          {
            string cl_view;
            for(string::iterator c=cl.begin();c!=cl.end();c++)
             cl_view.append(ElemIdToCodeNative(etClass,*c));
            SetProp(NewTextChild(segNode,"class",cl_view),"error","CRITICAL");
          };
        };

        //����稥 ����
        if (currSeg.fltInfo.pr_del!=0)
        {
          SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("���� �⬥���")),"error","CRITICAL"); //???
        }
        else
          if (currSeg.point_arv==ASTRA::NoExists)
          {
            //�� ��諨 �㭪� � �������
            SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("��� �/�")),"error","CRITICAL");
          }
          else
            if (cl.size()==1)
            {
              bool free_seating=SALONS2::isFreeSeating(currSeg.point_dep);

              //��⠥� ���-�� ᢮������ ����
              //(��᫥ ���᪠ ���ᠦ�஢, ⠪ ��� ����室��� ��।����� ������ �����)
              int free=CheckCounters(currSeg.point_dep,currSeg.point_arv,(char*)cl.c_str(),psTCheckin,cfg,free_seating);
              if (free!=NoExists && free<seatsSum)
              {
                xmlNodePtr wlNode;
                if (free<=0)
                  wlNode=NewTextChild(segNode,"free",AstraLocale::getLocaleText("���"));
                else
                  wlNode=NewTextChild(segNode,"free",
                                      IntToString(free)+" "+AstraLocale::getLocaleText("��")+" "+IntToString(seatsSum));
                SetProp(wlNode,"error","WL");
                SetProp(wlNode,"wl_type","O");
              }
              else
                NewTextChild(segNode,"free","+");
            }
            else
              SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("����।. ��.")),"error","CRITICAL");

      };
    }
    else
    {
      //�� ��諨 � ��� ३�, ᮮ⢥�����騩 �࠭��୮�� ᥣ�����
      NewTextChild(segNode,"flight",flight.str());

      if (!s->first.is_edi)
        SetProp(NewTextChild(segNode,"classes",AstraLocale::getLocaleText("��� � ���")),"error","CRITICAL");
      else
        NewTextChild(segNode,"classes","EDIFACT"); //�� ���� �訡���
      NewTextChild(segNode,"pnl");
      NewTextChild(segNode,"class");
      NewTextChild(segNode,"free");
    };

    if (!s->second.tckin_permit || s->first.is_edi) total_permit=false;
    bool total_waitlist=false;
    string wl_type;
    if (total_permit)
      for(xmlNodePtr node=segNode->children;node!=NULL;node=node->next)
      {
        xmlNodePtr errNode=GetNode("@error",node);
        if (errNode==NULL || NodeIsNULL(errNode)) continue;
        string error=NodeAsString(errNode);
        if (error=="CRITICAL" ||
            (error=="NOREC" && !s->second.tckin_norec) ||
            (error=="WL" && !s->second.tckin_waitlist))
        {
          total_permit=false;
          break;
        };
        if (error=="WL" && s->second.tckin_waitlist)
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

      if (s->first.flts.size()==1)
        readTripSets( s->first.flts.begin()->point_dep, s->first.flts.begin()->fltInfo, operFltNode );
    };

    if (total_permit)
    {
      if (total_waitlist)
      {
        NewTextChild(segNode,"total",AstraLocale::getLocaleText("��"));
        NewTextChild(segNode,"calc_status","WL");
      }
      else
      {
        NewTextChild(segNode,"total",AstraLocale::getLocaleText("���"));
        NewTextChild(segNode,"calc_status","CHECKIN");
      };
    }
    else
    {
      NewTextChild(segNode,"total",AstraLocale::getLocaleText("���"));
      NewTextChild(segNode,"calc_status","NONE");
    };

    if (NodeAsString("calc_status",segNode) != s->first.calc_status)
      //����� ���५� �� �ࠢ����� � �।��騬 ����ᮬ ᪢������ �������
      irrelevant_data=true;
  };
  if (irrelevant_data)
  {
    //����� ���५�
    xmlUnlinkNode(segsNode);
    xmlFreeNode(segsNode);
    return;
  };
  //����� �� ���५� - ���� ���뢠�� ���஡�� ����� ���ᠦ�஢ �� ᥣ���⠬




  xmlUnlinkNode(routeNode);
  xmlFreeNode(routeNode);

};

void CheckInInterface::ParseScanDocData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw AstraLocale::UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");
};

void CheckInInterface::CrewCheckin(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr flightNode = NodeAsNode("FLIGHT", reqNode);
    TSearchFltInfo filter;
    filter.airline = airl_fromXML(NodeAsNode("AIRLINE", flightNode), cfErrorIfEmpty, __FUNCTION__);
    filter.flt_no = flt_no_fromXML(NodeAsString("FLT_NO", flightNode));
    filter.suffix = suffix_fromXML(NodeAsString("SUFFIX", flightNode,""));
    filter.airp_dep = airp_fromXML(NodeAsNode("AIRP_DEP", flightNode), cfErrorIfEmpty, __FUNCTION__);
    filter.scd_out = scd_out_fromXML(NodeAsString("SCD", flightNode), "dd.mm.yyyy");
    filter.scd_out_in_utc = false;
    filter.only_with_reg = true;

    //�饬 ३� � ���
    list<TAdvTripInfo> flts;
    SearchFlt(filter, flts);
    if (flts.size() == 0) {
        throw AstraLocale::UserException("MSG.FLIGHTS_NOT_FOUND");
    }
    else if (flts.size() > 1) {
        throw AstraLocale::UserException("MSG.FLIGHT.FOUND_MORE");
    }
    try
    {
        const TAdvTripInfo &flt=*(flts.begin());
        TFlights flightsForLock;
        flightsForLock.Get(flt.point_id, ftTranzit);
        flightsForLock.Lock();

        WebSearch::TPnrData pnrData;
        pnrData.flt.fromDB(flt.point_id, true, true);
        vector<WebSearch::TPnrData> PNRs;
        PNRs.insert(PNRs.begin(), pnrData); //��⠢�塞 � ��砫� ���� ᥣ����
        xmlNodePtr crew_groups = NodeAsNode("CREW_GROUPS", reqNode);

        map<int,TAdvTripInfo> segs; // ����� ३ᮢ
        TDeletePaxFilter filter;
        filter.status=EncodePaxStatus(psCrew);
        filter.with_crew=true;
        DeletePassengers( flt.point_id, filter, segs );
        string commander;
        int cockpit = 0;
        int cabin = 0;
        bool is_commander = false;

        for(xmlNodePtr crew_group = crew_groups->children; crew_group != NULL; crew_group = crew_group->next) {
            xmlNodePtr crew_members = NodeAsNode("CREW_MEMBERS", crew_group);
            TWebPnrForSave pnr;
            pnr.status = psCrew;
            pnr.paxFromReq.push_back(TWebPaxFromReq());
            for(xmlNodePtr crew_member = crew_members->children; crew_member != NULL; crew_member = crew_member->next) {
                if(NodeAsString("DUTY", crew_member, "") == string("PIC") && NodeAsInteger("ORDER", crew_member, ASTRA::NoExists) == 1) {
                    if (!commander.empty())
                        throw AstraLocale::UserException("MSG.CHECK_XML_COMMANDER_DUPLICATED",
                                                   LEvntPrms() << PrmSmpl<string>("fieldname", "DUTY"));
                    is_commander = true;
                }
                TWebPaxForCkin paxForCkin;
                paxForCkin.crew_type = NodeAsString("CREW_TYPE", crew_member, "");
                if (paxForCkin.crew_type == "CR2" || paxForCkin.crew_type == "CR4" ||
                    paxForCkin.crew_type == "CR5" || paxForCkin.crew_type == "CR3")
                    cabin++;
                else if (paxForCkin.crew_type == "CR1")
                    cockpit++;
                else throw AstraLocale::UserException("MSG.CHECK_XML_INVALID_CREW_TYPE",
                                                LEvntPrms() << PrmSmpl<string>("fieldname", "CREW_TYPE"));
                xmlNodePtr pers_data = NodeAsNode("PERSONAL_DATA", crew_member);

                for (xmlNodePtr document = pers_data->children; document != NULL; document = document->next) {
                    xmlNodePtr doc=document->children;
                    if (doc==NULL) continue;
                    if (strcmp((const char*)document->name,"DOCS") == 0) {
                        if (!paxForCkin.present_in_req.insert(apiDoc).second)
                            throw AstraLocale::UserException("MSG.SECTION_DUPLICATED",
                                                              LEvntPrms() << PrmSmpl<string>("airline", "D"));
                        paxForCkin.surname = upperc(NodeAsStringFast("SURNAME", doc, ""));
                        string name = NodeAsStringFast("FIRST_NAME", doc, "");
                        string second_name;
                        second_name = NodeAsStringFast("SECOND_NAME", doc, "");
                        paxForCkin.apis.doc.surname=paxForCkin.surname;
                        paxForCkin.apis.doc.first_name=name;
                        paxForCkin.apis.doc.second_name=second_name;
                        if (!second_name.empty())
                            name = name + " " + second_name;
                        paxForCkin.name = upperc(name);
                        if (is_commander) {
                            commander = paxForCkin.surname + " " + upperc(name.substr(0, 1)) + ".";
                            if (!second_name.empty())
                                commander = commander + upperc(second_name.substr(0, 1)) + ".";
                            is_commander = false;
                        }
                        paxForCkin.pers_type = EncodePerson(ASTRA::adult);
                        paxForCkin.seats = 1;
                        paxForCkin.apis.doc.type=NodeAsStringFast("TYPE",doc,"");
                        paxForCkin.apis.doc.issue_country=CheckIn::PaxDocCountryFromTerm(NodeAsStringFast("ISSUE_COUNTRY",doc,""));
                        paxForCkin.apis.doc.no=NodeAsStringFast("NO",doc,"");
                        paxForCkin.apis.doc.nationality=CheckIn::PaxDocCountryFromTerm(NodeAsStringFast("NATIONALITY",doc,""));
                        if (!NodeIsNULLFast("BIRTH_DATE",doc, true))
                            paxForCkin.apis.doc.birth_date = date_fromXML(NodeAsStringFast("BIRTH_DATE",doc,""));
                        paxForCkin.apis.doc.gender=CheckIn::PaxDocGenderNormalize(NodeAsStringFast("GENDER",doc,""));
                        if (!NodeIsNULLFast("EXPIRY_DATE",doc, true))
                            paxForCkin.apis.doc.expiry_date = date_fromXML(NodeAsStringFast("EXPIRY_DATE",doc,""));
                        paxForCkin.apis.doc=NormalizeDoc(paxForCkin.apis.doc);
                    }
                    else if (strcmp((const char*)document->name,"DOCO") == 0) {
                        if (!paxForCkin.present_in_req.insert(apiDoco).second)
                            throw AstraLocale::UserException("MSG.SECTION_DUPLICATED",
                                                              LEvntPrms() << PrmSmpl<string>("name", "doco"));

                        TReqInfo *reqInfo = TReqInfo::Instance();
                        if (!(reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION)))
                            paxForCkin.apis.doco.doco_confirm=true;
                        paxForCkin.apis.doco.birth_place=NodeAsStringFast("BIRTH_PLACE",doc,"");
                        paxForCkin.apis.doco.type=NodeAsStringFast("TYPE",doc,"");
                        paxForCkin.apis.doco.no=NodeAsStringFast("NO",doc,"");
                        paxForCkin.apis.doco.issue_place=NodeAsStringFast("ISSUE_PLACE",doc,"");
                        if (!NodeIsNULLFast("ISSUE_DATE",doc,true))
                            paxForCkin.apis.doco.issue_date=date_fromXML(NodeAsStringFast("ISSUE_DATE",doc,""));
                        if (!NodeIsNULLFast("EXPIRY_DATE",doc,true))
                            paxForCkin.apis.doco.expiry_date=date_fromXML(NodeAsStringFast("EXPIRY_DATE",doc,""));
                        paxForCkin.apis.doco.applic_country=CheckIn::PaxDocCountryFromTerm(NodeAsStringFast("APPLIC_COUNTRY",doc,""));
                        if (reqInfo->client_type==ASTRA::ctTerm && !reqInfo->desk.compatible(DOCO_CONFIRM_VERSION))
                        {
                            if (paxForCkin.apis.doco.type==CheckIn::DOCO_PSEUDO_TYPE && paxForCkin.apis.doco.getNotEmptyFieldsMask()==DOCO_TYPE_FIELD)
                                paxForCkin.apis.doco.doco_confirm=true;
                            else
                                paxForCkin.apis.doco.doco_confirm=(paxForCkin.apis.doco.getNotEmptyFieldsMask()!=NO_FIELDS);
                            if (paxForCkin.apis.doco.type==CheckIn::DOCO_PSEUDO_TYPE) paxForCkin.apis.doco.type.clear();
                        }
                        else paxForCkin.apis.doco.doco_confirm=true;
                        paxForCkin.apis.doco = NormalizeDoco(paxForCkin.apis.doco);
                    }
                    else if (strcmp((const char*)document->name,"DOCA") == 0) {
                        string type = upperc(NodeAsStringFast("TYPE", doc, ""));

                        if (!(type == "B" || type == "R" || type == "D"))
                            throw AstraLocale::UserException("MSG.INVALID_ADDRES_TYPE");

                        if ((type == "B" && !paxForCkin.present_in_req.insert(apiDocaB).second) ||
                            (type == "R" && !paxForCkin.present_in_req.insert(apiDocaR).second) ||
                            (type == "D" && !paxForCkin.present_in_req.insert(apiDocaD).second))
                            throw AstraLocale::UserException("MSG.SECTION_DUPLICATED_WITH_TYPE",
                                                              LEvntPrms() << PrmSmpl<string>("name", "DOCA")
                                                              << PrmSmpl<string>("type", type));

                        CheckIn::TPaxDocaItem doca;
                        doca.type=type;
                        doca.country=CheckIn::PaxDocCountryFromTerm(NodeAsStringFast("COUNTRY",doc,""));
                        doca.address=NodeAsStringFast("ADDRESS",doc,"");
                        doca.city=NodeAsStringFast("CITY",doc,"");
                        doca.region=NodeAsStringFast("REGION",doc,"");
                        doca.postal_code=NodeAsStringFast("POSTAL_CODE",doc,"");
                        paxForCkin.apis.doca.push_back(NormalizeDoca(doca));
                    }
                }

                if (paxForCkin.present_in_req.find(apiDoc) == paxForCkin.present_in_req.end())
                    throw AstraLocale::UserException("MSG.SECTION_NOT_FOUND", LEvntPrms() << PrmSmpl<string>("name", "DOCS"));
                pnr.paxForCkin.push_back(paxForCkin);
            }

            WebSearch::TPnrData &pnrData=*(PNRs.begin());
            string airp_arv = airp_fromXML(NodeAsNode("AIRP_ARV", crew_group), cfErrorIfEmpty, __FUNCTION__);
            CompletePnrDataForCrew(airp_arv, pnrData);
            XMLDoc emulDocHeader;
            CreateEmulXMLDoc(emulDocHeader);
            XMLDoc emulCkinDoc;
            map<int,XMLDoc> emulChngDocs;
            CreateEmulDocs(vector< pair<int, TWebPnrForSave > >(1, make_pair(pnrData.flt.point_dep, pnr)),
                           PNRs, emulDocHeader, emulCkinDoc, emulChngDocs);

            TChangeStatusList ChangeStatusInfo;
            SirenaExchange::TLastExchangeList SirenaExchangeList;
            CheckIn::TAfterSaveInfoList AfterSaveInfoList;

            if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
            {
              xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
              if (emulReqNode==NULL)
                throw EXCEPTIONS::Exception("CheckInInterface::CrewCheckin: emulReqNode=NULL");
              if (SavePax(emulReqNode, NULL, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList))
              {
                //� �������� �᫨ �뫠 ॠ�쭠� ॣ������
                SirenaExchangeList.handle(__FUNCTION__);
                AfterSaveInfoList.handle(__FUNCTION__);
              }
            }
        }
        UpdateCrew(flt.point_id, commander, cockpit, cabin);

    }
    catch(...)
    {
        throw;
    }
}


namespace CheckIn
{

void showError(const std::map<int, std::map<int, AstraLocale::LexemaData> > &segs)
{
  if (segs.empty()) throw EXCEPTIONS::Exception("CheckIn::showError: empty segs!");
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  if (xmlRC->resDoc==NULL) return;
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

} //namespace CheckIn

void CheckInInterface::GetFQTTierLevel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  CheckIn::TPaxFQTItem fqt;
  fqt.fromXML(NodeAsNode("fqt_rem", reqNode));

  SirenaExchange::TFFPInfoReq req;
  SirenaExchange::TFFPInfoRes res;
  req.set(fqt.airline, fqt.no);

  get_ffp_status(req, res);

  fqt.tier_level=res.status;
  fqt.tier_level_confirm=true;

  fqt.toXML(resNode);
}

