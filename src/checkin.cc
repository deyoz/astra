#include "checkin.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "db_tquery.h"
#include "db_savepoint.h"
#include "PgOraConfig.h"
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
#include "astra_api.h"
#include "base_tables.h"
#include "tripinfo.h"
#include "aodb.h"
#include "salons.h"
#include "crafts/ComponCreator.h"
#include "seats.h"
#include "docs/docs_common.h"
#include "docs/docs_vouchers.h"
#include "dev_utils.h"
#include "checkin_utils.h"
#include "stat/stat_utils.h"
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
#include "qrys.h"
#include "emdoc.h"
#include "iatci.h"
#include "iatci_help.h"
#include "apis_utils.h"
#include "astra_callbacks.h"
#include "apps_interaction.h"
#include "astra_elem_utils.h"
#include "baggage_wt.h"
#include "payment_base.h"
#include "edi_utils.h"
#include "rfisc.h"
#include "ffp_sirena.h"
#include "annul_bt.h"
#include "counters.h"
#include "comp_layers.h"
#include "AirportControl.h"
#include "pax_events.h"
#include "rbd.h"
#include "tlg/AgentWaitsForRemote.h"
#include "tlg/tlg_parser.h"
#include "ckin_search.h"
#include "rfisc_calc.h"
#include "service_eval.h"
#include "base_callbacks.h"
#include "pax_calc_data.h"
#include "grp_db.h"
#include "check_grp_unification.h"
#include "tlg/typeb_db.h"
#include "baggage_ckin.h"

#include <jxtlib/jxt_cont.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/savepoint.h>
#include <serverlib/testmode.h>
#include <serverlib/dump_table.h>
#include <etick/tick_data.h>
#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace AstraLocale;
using namespace AstraEdifact;
using namespace EXCEPTIONS;
using astra_api::xml_entities::ReqParams;
using Ticketing::RemoteSystemContext::DcsSystemContext;

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
  }
}

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
  }
  TInquiryFamily()
  {
    Clear();
  }
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
  }
  TInquiryGroup()
  {
    Clear();
  }
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
    }
    for(;;)
    {
      if (i!=query.end()) c=*i; else c='\0';
      switch (state)
      {
        case 1:
          switch (c)
          {
            default:  state=2; continue;
          }
          state=2;
          break;
        case 2:
          fam.Clear();
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          }
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
            }
            strh.clear();
            switch (c)
            {
              case '-':
              case '+':  fam.seats_prefix=c; state=3; break;
              case '\0': grp.fams.push_back(fam); state=5; break;
              default:   fam.surname.push_back(c); state=5; break;
            }
            break;
          }
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
            }
            strh.clear();
            state=7;
            break;
          }
          throw pos;
        case 3:
          if (IsDigit(c))
          {
            strh=c;
            if (StrToInt(strh.c_str(),fam.seats)==EOF) throw pos;
            strh.clear();
            state=4;
            break;
          }
          throw pos;
        case 4:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            state=5;
            break;
          }
          if (c=='\0')
          {
            if (grp.fams.empty())
            {
              grp.fams.push_back(fam);
              state=5;
            }
            else throw pos;
            break;
          }
          throw pos;
        case 5:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            break;
          }
          if (c=='/' || c=='\0')
          {
            if (c=='/') state=6;
            TrimString(fam.surname);
            if (fam.surname.empty() && !grp.fams.empty()) throw pos;
            grp.fams.push_back(fam);
            fam.Clear();
            break;
          }
          throw pos;
        case 6:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          }
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
            }
            strh.clear();
            switch (c)
            {
              case '-':
              case '+': fam.seats_prefix=c; state=3; break;
              default:  fam.surname.push_back(c); state=5; break;
            }
            break;
          }
          throw pos;
        case 7:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          }
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
            }
            strh.clear();
            state=8;
            break;
          }
          throw pos;
        case 8:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          }
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
            }
            strh.clear();
            state=9;
            break;
          }
          throw pos;
        case 9:
          if (IsDigit(c))
          {
            strh.push_back(c);
            state=10;
            break;
          }
          [[fallthrough]];
        case 11:
          if (IsLetter(c) || c==' ')
          {
            fam.surname.push_back(c);
            state=12;
            break;
          }
          if (c=='\0')
          {
            grp.fams.push_back(fam);
            state=12;
            break;
          }
          throw pos;
        case 10:
          if (IsDigit(c))
          {
            strh.push_back(c);
            break;
          }
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
            }
            strh.clear();
            state=11;
            break;
          }
          throw pos;
        case 12:
          if (IsLetter(c) || c==' ')
          {
             fam.surname.push_back(c);
            break;
          }
          if (c=='\0')
          {
            TrimString(fam.surname);
            if (fam.surname.empty() && !grp.fams.empty()) throw pos;
            grp.fams.push_back(fam);
            break;
          }
          throw pos;
      }
      if (i==query.end()) break;
      i++;
      pos++;
    }

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
          }
        if (grp.fams.size()==1 && grp.fams.begin()->surname.empty())
        {
          grp.fams.begin()->surname='X';
          grp.digCkin=true;
        }
      }
      else throw 0;
    }
    else throw pos;
  }
  catch(int pos)
  {
    throw UserException(pos+100,"MSG.ERROR_IN_REQUEST");
  }

}

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
  }
  TInquiryFamilySummary()
  {
    Clear();
  }
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
  }
  TInquiryGroupSummary()
  {
    Clear();
  }
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
  }
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
      }
    }
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
      }
    }

    if (i->n[adult]==-1 && i->n[child]==-1 && i->n[baby]==-1 && i->seats==-1)
      info.n[adult]=1;
    else
    {
      for(int p=0;p<NoPerson;p++)
        if (i->n[p]!=-1) info.n[p]=i->n[p]; else info.n[p]=0;

    }
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
        }
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
          }
          if (i->seats_prefix=='+')
          {
            info.nPaxWithPlace=info.n[adult]+info.n[child]+info.n[baby];
            info.n[baby]+=i->seats;
          }
        }
        else
        {
          if (i->seats_prefix=='-')
          {
            if (info.n[baby]<i->seats)
              throw UserException(100,"MSG.CHECKIN.BABY_WITH_SEATS_MORE_COUNT_BABY_FOR_SURNAME");
            info.nPaxWithPlace=info.n[adult]+info.n[child]+i->seats;
          }
          if (i->seats_prefix=='+')
          {
            info.nPaxWithPlace=info.n[adult]+info.n[child]+i->seats;
            info.n[baby]+=i->seats;
          }
        }
        info.nPax=info.n[adult]+info.n[child]+info.n[baby];
      }
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
    }
    for(int p=0;p<NoPerson;p++)
      sum.n[p]+=info.n[p];
    sum.nPax+=info.nPax;
    sum.nPaxWithPlace+=info.nPaxWithPlace;
    sum.fams.push_back(info);
  }
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
}

void CheckTrferPermit(const pair<CheckIn::TTransferItem, CheckIn::Segments> &in,
                      const pair<CheckIn::TTransferItem, CheckIn::Segments> &out,
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
    }
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
    }

    sets.trfer_outboard=qrySets.trfer_outboard; //� �� ��砥 ������塞 ��� ᫥���饩 ���樨

    int min_interval=Qry.FieldIsNULL("min_interval")?NoExists:Qry.FieldAsInteger("min_interval");
    int max_interval=Qry.FieldIsNULL("max_interval")?NoExists:Qry.FieldAsInteger("max_interval");

    if (in.second.is_edi || out.second.is_edi ||
        in.second.segs.size()!=1 || out.second.segs.size()!=1)
    {
      if (min_interval==NoExists && max_interval==NoExists)
      {
        sets.trfer_permit=qrySets.trfer_permit;
      }
      return;
    }

    const CheckIn::Segment &inSeg=*(in.second.segs.begin());
    const CheckIn::Segment &outSeg=*(out.second.segs.begin());
    if (!inSeg.flt.match(FlightProps::NotCancelled) ||
        !outSeg.flt.match(FlightProps::NotCancelled) ||
        inSeg.route.empty() || outSeg.route.empty() ||
        inSeg.airpArv()!=outSeg.airpDep())
    {
      if (min_interval==NoExists && max_interval==NoExists)
      {
        sets.trfer_permit=qrySets.trfer_permit;
      }
      return;
    }

    if ((qrySets.trfer_permit || qrySets.tckin_permit) &&
        (min_interval!=NoExists || max_interval!=NoExists))
    {
      //���� �஢���� �६� �ਫ�� inSeg
      Qry.Clear();
      Qry.SQLText=
        "SELECT NVL(act_in,NVL(est_in,scd_in)) AS real_in FROM points WHERE point_id=:point_id";
      Qry.CreateVariable("point_id",otInteger,inSeg.pointArv().get());
      Qry.Execute();
      if (!Qry.Eof && !Qry.FieldIsNULL("real_in"))
      {
        if (outSeg.flt.act_est_scd_out()==NoExists)
          throw EXCEPTIONS::Exception("CheckTrferPermit: outSeg.fltInfo.act_est_scd_out()==NoExists");
        double interval=round((outSeg.flt.act_est_scd_out()-Qry.FieldAsDateTime("real_in"))*1440);
        //ProgTrace(TRACE5, "interval=%f, min_interval=%d, max_interval=%d", interval, min_interval, max_interval);
        if ((min_interval!=NoExists && interval<min_interval) ||
            (max_interval!=NoExists && interval>max_interval)) return;
      }
      else return;
    }

    sets=qrySets;
  }
}

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
    }

    str << setw(2) << right << iTrfer->first << ": "
        << setw(3) << left  << iTrfer->second.operFlt.airline << " "
        << setw(5) << right << (iTrfer->second.operFlt.flt_no !=NoExists ? IntToString(iTrfer->second.operFlt.flt_no) : " ")
        << setw(1) << left  << iTrfer->second.operFlt.suffix << " "
        << setw(4) << right << (iTrfer->second.operFlt.scd_out !=NoExists ? DateTimeToStr(iTrfer->second.operFlt.scd_out,"dd") : " ") << " "
        << setw(3) << left  << iTrfer->second.operFlt.airp << " "
        << setw(3) << left  << iTrfer->second.airp_arv << " "
        << setw(3) << left  << iTrfer->second.subclass;

    ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());
  }

  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
}

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const map<int, pair<CheckIn::Segments, TTrferSetsInfo> > &segs )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(map<int, pair<CheckIn::Segments, TTrferSetsInfo> >::const_iterator iSeg=segs.begin();iSeg!=segs.end();++iSeg)
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
    }

    if (!iSeg->second.first.segs.empty())
    {
      for(vector<CheckIn::Segment>::const_iterator f=iSeg->second.first.segs.begin();f!=iSeg->second.first.segs.end();++f)
      {
        if (f==iSeg->second.first.segs.begin())
          str << setw(2) << right << iSeg->first << ": ";
        else
          str << setw(2) << right << " " << "  ";

        const CheckIn::Segment& segment=*f;
        const TAdvTripInfo& flt=f->flt;

        str << boolalpha
            << setw(3) << left  << flt.airline << " "
            << setw(5) << right << (flt.flt_no != NoExists ? IntToString(flt.flt_no) : " ")
            << setw(1) << left  << flt.suffix << " "
            << setw(3) << left  << flt.airp << " "
            << setw(9) << left  << DateTimeToStr(flt.scd_out,"ddmm hhnn") << " "
            << setw(9) << right << (!segment.route.empty() ? IntToString(segment.pointDep().get()) : "") << " "
            << setw(9) << right << (!segment.route.empty() ? IntToString(segment.pointArv().get()) : "") << " "
            << setw(3) << left  << (!segment.route.empty() ? segment.airpDep().get() : "") << " "
            << setw(3) << left  << (!segment.route.empty() ? segment.airpArv().get() : "") << "|"
            << setw(10) << left  << iSeg->second.second.trfer_permit    << " "
            << setw(10) << left  << iSeg->second.second.trfer_outboard  << " "
            << setw(10) << left  << iSeg->second.second.tckin_permit    << " "
            << setw(10) << left  << iSeg->second.second.tckin_waitlist  << " "
            << setw(10) << left  << iSeg->second.second.tckin_norec;
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

        str.str("");
      }
    }
    else
    {
      str << boolalpha
          << setw(2) << right << iSeg->first << ": "
          << setw(52) << left << "not found!" << "|"
          << setw(10) << left  << iSeg->second.second.trfer_permit    << " "
          << setw(10) << left  << iSeg->second.second.trfer_outboard  << " "
          << setw(10) << left  << iSeg->second.second.tckin_permit    << " "
          << setw(10) << left  << iSeg->second.second.tckin_waitlist  << " "
          << setw(10) << left  << iSeg->second.second.tckin_norec;

      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    }
  }
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
}

void traceTrfer( TRACE_SIGNATURE,
                 const string &descr,
                 const map<int, CheckIn::Segments > &segs )
{
  ProgTrace(TRACE_PARAMS, "============ %s ============", descr.c_str());
  for(map<int, CheckIn::Segments >::const_iterator iSeg=segs.begin();iSeg!=segs.end();++iSeg)
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
    }

    if (!iSeg->second.segs.empty())
    {
      for(vector<CheckIn::Segment>::const_iterator f=iSeg->second.segs.begin();f!=iSeg->second.segs.end();++f)
      {
        if (f==iSeg->second.segs.begin())
          str << setw(2) << right << iSeg->first << ": ";
        else
          str << setw(2) << right << " " << "  ";

        const CheckIn::Segment& segment=*f;
        const TAdvTripInfo& flt=f->flt;

        str << setw(3) << left  << flt.airline << " "
            << setw(5) << right << (flt.flt_no != NoExists ? IntToString(flt.flt_no) : " ")
            << setw(1) << left  << flt.suffix << " "
            << setw(3) << left  << flt.airp << " "
            << setw(9) << left  << DateTimeToStr(flt.scd_out,"ddmm hhnn") << " "
            << setw(9) << right << (!segment.route.empty() ? IntToString(segment.pointDep().get()) : "") << " "
            << setw(9) << right << (!segment.route.empty() ? IntToString(segment.pointArv().get()) : "") << " "
            << setw(3) << left  << (!segment.route.empty() ? segment.airpDep().get() : "") << " "
            << setw(3) << left  << (!segment.route.empty() ? segment.airpArv().get() : "");
        ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

        str.str("");
      }
    }
    else
    {
      str << setw(2) << right << iSeg->first << ": " << "not found!";

      ProgTrace(TRACE_PARAMS, "%s", str.str().c_str());

      str.str("");
    }
  }
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ %s ^^^^^^^^^^^^", descr.c_str());
}

void CheckInInterface::GetTrferSets(const TTripInfo &operFlt,
                                    const string &oper_airp_arv,
                                    const string &tlg_airp_dep,
                                    const map<int, CheckIn::TTransferItem> &trfer,
                                    const bool get_trfer_permit_only,
                                    map<int, pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs)
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

    map<int, pair<CheckIn::TTransferItem, CheckIn::Segments> > trfer_segs_tmp;
    GetTCkinFlights(operFlt, trfer_tmp, trfer_segs_tmp);

    //traceTrfer( TRACE5, "GetTrferSets: trfer_segs_tmp", trfer_segs_tmp );

    bool outboard_trfer=false;

    pair<int, pair<CheckIn::TTransferItem, CheckIn::Segments> > prior_trfer_seg;
    for(map<int, pair<CheckIn::TTransferItem, CheckIn::Segments> >::const_iterator s=trfer_segs_tmp.begin();
                                                                                   s!=trfer_segs_tmp.end(); ++s)
    {
      if (s!=trfer_segs_tmp.begin())
      {
        TTrferSetsInfo sets;
        if (prior_trfer_seg.first+1==s->first)
          CheckTrferPermit(prior_trfer_seg.second, s->second, outboard_trfer, sets);
        outboard_trfer=sets.trfer_outboard;
        trfer_segs[s->first]=make_pair(s->second.second, sets);
      }
      prior_trfer_seg=*s;
    }
  }
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
    }
  }
}

// trferItem.operFlt.point_id ��⠥��� �� �ந��樠����஢����
void CheckInInterface::GetOnwardCrsTransfer(int id, bool isPnrId,
                                            const TTripInfo &operFlt,
                                            const string &oper_airp_arv,
                                            map<int, CheckIn::TTransferItem> &trfer)
{
  trfer.clear();
  TypeB::TTransferRoute crs_trfer;
  isPnrId?crs_trfer.getByPnrId(id):
          crs_trfer.getByPaxId(id);

  if (crs_trfer.empty()) return;

  TDateTime local_scd=UTCToLocal(operFlt.scd_out,AirpTZRegion(operFlt.airp));

  int prior_transfer_num=0;
  string prior_airp_arv=oper_airp_arv;
  TElemFmt prior_airp_arv_fmt=efmtCodeNative;
  for(TypeB::TTransferRoute::const_iterator t=crs_trfer.begin();t!=crs_trfer.end();++t)
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
    }

    trferItem.local_date=t->local_date;
    try
    {
      TDateTime base_date=local_scd-1; //��⠬��� ����� �� ������ ����� � ���ਪ� �� ���譨� ����
      local_scd=DayToDate(t->local_date,base_date,false); //�����쭠� ��� �뫥�
      trferItem.operFlt.scd_out=local_scd;
    }
    catch(const EXCEPTIONS::EConvertError &E) {}

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
    }


    trferItem.airp_arv=ElemToElemId(etAirp,t->airp_arv,trferItem.airp_arv_fmt);
    if (trferItem.airp_arv_fmt==efmtUnknown)
      trferItem.airp_arv=t->airp_arv;

    trferItem.subclass=ElemToElemId(etSubcls,t->subcl,trferItem.subclass_fmt);
    if (trferItem.subclass_fmt==efmtUnknown)
      trferItem.subclass=t->subcl;

    LogTrace(TRACE5) << __FUNCTION__ << " trferItem point_id: "<< trferItem.operFlt.point_id;
    trfer[t->num]=trferItem;

    prior_transfer_num=t->num;
    prior_airp_arv=trferItem.airp_arv;
    prior_airp_arv_fmt=trferItem.airp_arv_fmt;
  }
}

//�⥭�� ���� ⮫쪮 Onward, ⮫쪮 ��᫥���騥 �࠭����
std::map<int, CheckIn::TTransferItem> CheckInInterface::getCrsTransferMap(const PaxId_t& pax_id)
{
    int pnr_id = 0, point_id = 0;
    std::string airp_arv;
    auto cur = make_db_curs(
        "select POINT_ID_SPP, CRS_PAX.PNR_ID, AIRP_ARV "
        ", PERS_TYPE "
        "from CRS_PAX, CRS_PNR, TLG_BINDING "
        "where PAX_ID = :pax_id and CRS_PAX.PNR_ID = CRS_PNR.PNR_ID and "
        "      CRS_PNR.POINT_ID = TLG_BINDING.POINT_ID_TLG",
        PgOra::getROSession({"CRS_PAX", "CRS_PNR", "TLG_BINDING"}));
    cur
        .stb()
        .def(point_id)
        .def(pnr_id)
        .def(airp_arv)
        .bind(":pax_id", pax_id.get())
        .EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        LogTrace(TRACE5) << __FUNCTION__ << " No data found";
        return {};
    }
    map<int, CheckIn::TTransferItem> trfer;
    CheckInInterface::GetOnwardCrsTransfer(pnr_id, true, *getPointInfo(PointId_t(point_id)), airp_arv, trfer);
    return trfer;
}

void CheckInInterface::LoadOnwardCrsTransfer(const map<int, CheckIn::TTransferItem> &trfer,
                                             const map<int, pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs,
                                             xmlNodePtr trferNode)
{
  map<int, pair<CheckIn::TTransferItem, TTrferSetsInfo> > trfer_sets;
  map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin();
  map<int, pair<CheckIn::Segments, TTrferSetsInfo> >::const_iterator s=trfer_segs.begin();
  for(;t!=trfer.end() && s!=trfer_segs.end(); ++t, ++s)
  {
    if (t->first!=s->first) throw EXCEPTIONS::Exception("LoadOnwardCrsTransfer: wrong trfer_segs");
    trfer_sets[t->first]=make_pair(t->second, s->second.second);
  }
  LoadOnwardCrsTransfer(trfer_sets, trferNode);
}

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
      NewTextChild(node2,"airline",ElemIdToCodeNative(etAirline,
                                                      t->second.first.operFlt.airline));
    else
      NewTextChild(node2,"airline",t->second.first.operFlt.airline);

    NewTextChild(node2,"flt_no",t->second.first.operFlt.flt_no);

    if (t->second.first.operFlt.suffix_fmt!=efmtUnknown)
      NewTextChild(node2,"suffix",ElemIdToCodeNative(etSuffix,
                                                     t->second.first.operFlt.suffix),"");
    else
      NewTextChild(node2,"suffix",t->second.first.operFlt.suffix,"");

    NewTextChild(node2,"local_date",t->second.first.local_date);

    if (t->second.first.operFlt.airp_fmt!=efmtUnknown)
      NewTextChild(node2,"airp_dep",ElemIdToCodeNative(etAirp,
                                                       t->second.first.operFlt.airp),"");
    else
      NewTextChild(node2,"airp_dep",t->second.first.operFlt.airp,"");

    if (t->second.first.airp_arv_fmt!=efmtUnknown)
      NewTextChild(node2,"airp_arv",ElemIdToCodeNative(etAirp,
                                                       t->second.first.airp_arv));
    else
      NewTextChild(node2,"airp_arv",t->second.first.airp_arv);

    if (t->second.first.subclass_fmt!=efmtUnknown)
      NewTextChild(node2,"subclass",ElemIdToCodeNative(etSubcls,
                                                       t->second.first.subclass));
    else
      NewTextChild(node2,"subclass",t->second.first.subclass);

    NewTextChild(node2,"trfer_permit",(int)t->second.second.trfer_permit);
  }
}

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
  }

  return i1==trfer1.end() && i2==trfer2.end();
}

bool LoadUnconfirmedTransfer(const CheckIn::TTransferList &segs, xmlNodePtr transferNode)
{
  if (segs.empty() || transferNode==NULL) return false;

  const CheckIn::TTransferItem &firstSeg=*segs.begin();

  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
   "SELECT crs_pnr.pnr_id, crs_pnr.point_id, crs_pax.pax_id "
   "FROM pax,crs_pax,crs_pnr "
   "WHERE crs_pax.pax_id=pax.pax_id AND "
   "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
   "      crs_pax.pr_del=0 AND "
   "      pax.grp_id=:grp_id "
   "ORDER BY crs_pnr.pnr_id";
  PaxQry.CreateVariable("grp_id", otInteger, firstSeg.grp_id);
  PaxQry.Execute();

  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > > crs_trfer, trfer; //����� ��� <tlg_airp_dep+�࠭���� �������, ����� ��. ���ᠦ�஢>

  int pnr_id=NoExists;
  TlgTripsPnlCache tlg_trips_pnl_cached;
  for(;!PaxQry.Eof;PaxQry.Next())
  {
    if (PaxQry.FieldAsInteger("pnr_id")!=pnr_id)
    {
      const std::optional<TlgTripsData> tlg_trips_data =
          tlg_trips_pnl_cached.get(PointIdTlg_t(PaxQry.FieldAsInteger("point_id")));
      if (!tlg_trips_data) {
        continue;
      }
      pnr_id=PaxQry.FieldAsInteger("pnr_id");

      crs_trfer.push_back( make_pair( make_pair( string(), map<int, CheckIn::TTransferItem>() ), vector<int>() ) );

      pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();

      last_crs_trfer.first.first=tlg_trips_data->airpDep.value().get();
      CheckInInterface::GetOnwardCrsTransfer(pnr_id, true, firstSeg.operFlt, firstSeg.airp_arv, last_crs_trfer.first.second); //���⠥� �� ⠡���� crs_transfer
    }

    if (crs_trfer.empty()) continue;
    pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > &last_crs_trfer=crs_trfer.back();
    last_crs_trfer.second.push_back(PaxQry.FieldAsInteger("pax_id"));
  }

  ProgTrace(TRACE5,"LoadUnconfirmedTransfer: crs_trfer - step 1");
  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::const_iterator iCrsTrfer=crs_trfer.begin();
  for(;iCrsTrfer!=crs_trfer.end();++iCrsTrfer)
  {
    ProgTrace(TRACE5,"tlg_airp_arv=%s map<int, CheckIn::TTransferItem>.size()=%zu vector<int>.size()=%zu",
                     iCrsTrfer->first.first.c_str(), iCrsTrfer->first.second.size(), iCrsTrfer->second.size());
  }

  //�஡�� �� ���ᠦ�ࠬ ��ࢮ�� ᥣ����
  int pax_no=0;
  int iYear,iMonth,iDay;
  for(vector<CheckIn::TPaxTransferItem>::const_iterator p=firstSeg.pax.begin();p!=firstSeg.pax.end();++p,pax_no++)
  {
    string tlg_airp_dep=firstSeg.operFlt.airp;
    map<int, CheckIn::TTransferItem> pax_trfer;
    //����ࠥ� ����� �࠭���
    int seg_no=1;
    for(CheckIn::TTransferList::const_iterator s=segs.begin();s!=segs.end();++s,seg_no++)
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
    }
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
        }
        break;
      }
    }
    ProgTrace(TRACE5,"LoadUnconfirmedTransfer: pax_trfer.size()=%zu",pax_trfer.size());
    //⥯��� pax_trfer ᮤ�ন� ᥣ����� ᪢����� ॣ����樨 � �������ᮬ ���ᠦ��
    //���� �������⥫�� ᥣ����� �࠭��� �� ⠡���� crs_transfer
    //�� TTransferItem � pax_trfer ���஢��� �� ������ �࠭��� (TTransferItem.num)
    vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::iterator iTrfer=trfer.begin();
    for(;iTrfer!=trfer.end();++iTrfer)
    {
      if (iTrfer->first.first==tlg_airp_dep &&
          EqualCrsTransfer(iTrfer->first.second,pax_trfer)) break;
    }
    if (iTrfer!=trfer.end())
      //��諨 �� �� �������
      iTrfer->second.push_back(p->pax_id);
    else
      trfer.push_back( make_pair( make_pair( tlg_airp_dep, pax_trfer ), vector<int>(1,p->pax_id) ) );
  }
  //�ନ�㥬 XML
  xmlNodePtr itemsNode=NewTextChild(transferNode,"unconfirmed_transfer");

  bool result=false;

  vector< pair< pair< string, map<int, CheckIn::TTransferItem> >, vector<int> > >::const_iterator iTrfer=trfer.begin();
  for(;iTrfer!=trfer.end();++iTrfer)
  {
    if (iTrfer->first.second.empty()) continue;
    xmlNodePtr itemNode=NewTextChild(itemsNode,"item");
    result=true;

    map<int, pair<CheckIn::Segments, TTrferSetsInfo> > trfer_segs;
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
  }
  return result;
}

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
        }
      }
    }
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
        }
        pax.pers_type=(TPerson)p;
        pax.seats=1;
        if (n>sum.nPaxWithPlace)
          pax.seats=0;
        paxs.push_back(pax);
      }
    }
  }
}

struct TCkinPaxInfo
{
  string surname,name,pers_type,subclass;
  int seats,reqCount,resCount;
  xmlNodePtr node;
};

static void CreateEdiTCkinResponse(const CheckIn::TTransferItem &ti,
                                   const TCkinPaxInfo& pax,
                                   int paxId)
{
    xmlNodePtr tripsNode = GetNode("trips", pax.node);
    if(!tripsNode) {
        tripsNode = NewTextChild(pax.node, "trips");
    }

    xmlNodePtr tripNode = NewTextChild(tripsNode, "trip");
    NewTextChild(tripNode, "airline",  ti.operFlt.airline);
    NewTextChild(tripNode, "point_id", -1);
    NewTextChild(tripNode, "airp_dep", ti.operFlt.airp);
    NewTextChild(tripNode, "flt_no",   ti.operFlt.flt_no);
    NewTextChild(tripNode, "scd",      DateTimeToStr(ti.operFlt.scd_out)); // local? utc? TODO

    xmlNodePtr groupsNode = NewTextChild(tripNode, "groups");
    xmlNodePtr pnrNode = NewTextChild(groupsNode, "pnr");
    NewTextChild(pnrNode, "pnr_id", -1);
    NewTextChild(pnrNode, "airp_arv", ti.airp_arv);

    Ticketing::SubClass subcls(pax.subclass);
    Language lang = TReqInfo::Instance()->desk.lang == AstraLocale::LANG_RU ? RUSSIAN
                                                                            : ENGLISH;
    NewTextChild(pnrNode, "subclass", subcls->code(lang));
    NewTextChild(pnrNode, "class",    subcls->baseClass()->code(lang));
    NewTextChild(pnrNode, "cabin_class",    subcls->baseClass()->code(lang));

    xmlNodePtr paxesNode = NewTextChild(pnrNode, "passengers");
    xmlNodePtr paxNode = NewTextChild(paxesNode, "pax");
    NewTextChild(paxNode, "pax_id",    paxId);
    NewTextChild(paxNode, "surname",   pax.surname);
    NewTextChild(paxNode, "name",      pax.name);
    NewTextChild(paxNode, "seats",     pax.seats);
    NewTextChild(paxNode, "pers_type", pax.pers_type);

    NewTextChild(paxNode, "seat_no");
    NewTextChild(paxNode, "document");
    NewTextChild(paxNode, "ticket_no", "");
    NewTextChild(paxNode, "ticket_rem", "");
    NewTextChild(paxNode, "rems");

    NewTextChild(pnrNode, "transfer");
    NewTextChild(pnrNode, "pnr_addrs");
}

static void CreateNoRecResponse(const TInquiryGroupSummary &sum, xmlNodePtr resNode)
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
  }
}

static void CreateCrewResponse(int point_dep, const TInquiryGroupSummary &sum, xmlNodePtr resNode)
{
  DB::TQuery FltQry(PgOra::getROSession("POINTS"), STDLOG);
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
  NewTextChild(node,"cabin_class",EncodeClass(Y)); //crew compatible

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
  }
}

void LoadPaxRemAndASVC(int pax_id, xmlNodePtr node, bool from_crs)
{
  if (node==NULL) return;

  multiset<CheckIn::TPaxRemItem> rems_and_asvc;
  CheckIn::PaxRemAndASVCFromDB(pax_id, from_crs, boost::none, rems_and_asvc);
  CheckIn::TPaxRemItem apps_satus_rem = getAPPSRem( pax_id, TReqInfo::Instance()->desk.lang );
  if ( !apps_satus_rem.empty() )
   rems_and_asvc.insert( apps_satus_rem );
  CheckIn::PaxRemAndASVCToXML(rems_and_asvc, node);

  if (!from_crs)
  {
    TPaxEMDList emds;
    emds.getAllPaxEMD(pax_id, false);
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

static int CreateSearchResponse(int point_dep,
                                const std::vector<SearchPaxResult>& searchPaxResults,
                                const set<int>& selectedPaxIds,
                                xmlNodePtr resNode)
{
  TTripInfo operFlt;
  if (!operFlt.getByPointId(point_dep, FlightProps(FlightProps::WithCancelled,
                                                   FlightProps::WithCheckIn)))
    throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

  DB::TQuery FltQry(PgOra::getROSession("TLG_TRIPS"), STDLOG);
  FltQry.SQLText=
    "SELECT airline,flt_no,suffix,airp_dep AS airp,scd AS scd_out "
    "FROM tlg_trips WHERE point_id=:point_id";
  FltQry.DeclareVariable("point_id",otInteger);

  int prior_point_id=NoExists;
  int prior_pnr_id=NoExists;
  std::string prior_orig_subclass;
  std::string prior_orig_class;
  int pax_id;
  xmlNodePtr tripNode = NULL,pnrNode = NULL,paxNode = NULL,node = NULL;

  TMktFlight mktFlt;
  TTripInfo tlgTripsFlt;
  TCodeShareSets codeshareSets;
  TPnrAddrs pnrAddrs;
  map<int, CheckIn::TTransferItem> trfer;
  map<int, pair<CheckIn::Segments, TTrferSetsInfo> > trfer_segs;

  int count=0;
  tripNode=GetNode("trips",resNode);
  if (tripNode==NULL)
    tripNode=NewTextChild(resNode,"trips");
  for(const SearchPaxResult& searchResult: searchPaxResults)
  {
    count++;
    int curr_point_id=searchResult.point_id.get();
    int curr_pnr_id=searchResult.pnr_id.get();
    std::string curr_orig_subclass=searchResult.subclass;
    std::string curr_orig_class=searchResult.cls;

    if (curr_point_id!=prior_point_id)
    {
      FltQry.SetVariable("point_id",curr_point_id);
      FltQry.Execute();
      if (FltQry.Eof)
        throw EXCEPTIONS::Exception("Flight not found in tlg_trips (point_id=%d)",curr_point_id);
      tlgTripsFlt.Init(FltQry);

      TDateTime scd_tmp;
      modf(tlgTripsFlt.scd_out, &scd_tmp);
      tlgTripsFlt.scd_out = scd_tmp;
      node=NewTextChild(tripNode,"trip");
      NewTextChild(node,"point_id",curr_point_id);
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
      }

      pnrNode=NewTextChild(node,"groups");
    }

    if (curr_pnr_id!=prior_pnr_id)
    {
      mktFlt.getByPnrId(curr_pnr_id);

      trfer.clear();
      trfer_segs.clear();
      CheckInInterface::GetOnwardCrsTransfer(curr_pnr_id, true,
                                             operFlt,
                                             searchResult.airp_arv,
                                             trfer);
      traceTrfer( TRACE5, "CreateSearchResponse: trfer", trfer );
      if (!trfer.empty())
      {
        CheckInInterface::GetTrferSets(operFlt,
                                       searchResult.airp_arv,
                                       tlgTripsFlt.airp,
                                       trfer,
                                       true,
                                       trfer_segs);
        traceTrfer( TRACE5, "CreateSearchResponse: trfer_segs", trfer_segs );
      };

      pnrAddrs.getByPnrIdFast(curr_pnr_id);
    }

    if (curr_pnr_id!=prior_pnr_id ||
        curr_orig_subclass!=prior_orig_subclass ||
        curr_orig_class!=prior_orig_class)
    {
      node=NewTextChild(pnrNode,"pnr");
      NewTextChild(node,"pnr_id",curr_pnr_id);
      NewTextChild(node,"airp_arv",searchResult.airp_arv);
      NewTextChild(node,"subclass",curr_orig_subclass);
      NewTextChild(node,"class",curr_orig_class);
      NewTextChild(node,"cabin_class",searchResult.cabin_class);
      string pnr_status=searchResult.pnr_status;
      NewTextChild(node,"pnr_status",pnr_status,"");
      NewTextChild(node,"pnr_priority",searchResult.pnr_priority,"");
      NewTextChild(node,"pad",(int)(pnr_status=="DG2"||
                                    pnr_status=="RG2"||
                                    pnr_status=="ID2"||
                                    pnr_status=="WL"),0);

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
        }
      }

      paxNode=NewTextChild(node,"passengers");

      if (!trfer.empty())
        CheckInInterface::LoadOnwardCrsTransfer(trfer, trfer_segs, NewTextChild(node,"transfer"));

      pnrAddrs.toXML(node);
    }
    node=NewTextChild(paxNode,"pax");
    pax_id=searchResult.pax_id.get();
    NewTextChild(node,"pax_id",pax_id);
    NewTextChild(node,"surname",searchResult.surname);
    NewTextChild(node,"name",searchResult.name,"");
    NewTextChild(node,"pers_type",searchResult.pers_type,EncodePerson(ASTRA::adult));
    NewTextChild(node,"seat_no",searchResult.seat_no,"");
    NewTextChild(node,"seat_type",searchResult.seat_type,"");
    NewTextChild(node,"seats",searchResult.seats,1);
    //��ࠡ�⪠ ���㬥�⮢
    CheckIn::TPaxDocItem doc;
    if (LoadCrsPaxDoc(pax_id, doc)) doc.toXML(node);
    //��ࠡ�⪠ ���
    CheckIn::TPaxDocoItem doco;
    if (LoadCrsPaxVisa(pax_id, doco)) doco.toXML(node);
    //��ࠡ�⪠ ���ᮢ
    CheckIn::TDocaMap doca_map;
    if (LoadCrsPaxDoca(pax_id, doca_map)) doca_map.toXML(node);

    //��ࠡ�⪠ ����⮢
    string ticket_no;
    if (!searchResult.eticket.empty())
    {
      //����� TKNE
      ticket_no=searchResult.eticket;
      int coupon_no=NoExists;
      string::size_type pos=ticket_no.find_last_of('/');
      if (pos!=string::npos)
      {
        if (StrToInt(ticket_no.substr(pos+1).c_str(),coupon_no)!=EOF &&
            coupon_no>=1 && coupon_no<=4)
          ticket_no.erase(pos);
        else
          coupon_no=NoExists;
      }

      NewTextChild(node,"ticket_no",ticket_no);
      NewTextChild(node,"coupon_no",coupon_no,NoExists);
      NewTextChild(node,"ticket_rem","TKNE");
    }
    else
    {
      if (!searchResult.ticket.empty())
      {
        //����� TKNA
        ticket_no=searchResult.ticket;
        NewTextChild(node,"ticket_no",ticket_no);
        NewTextChild(node,"ticket_rem","TKNA");
      }
    }

    LoadPaxRemAndASVC(pax_id, node, true);
    LoadPaxFQT(pax_id, node, true);

    NewTextChild(node, "selected", (int)(selectedPaxIds.find(pax_id)!=selectedPaxIds.end()), (int)false);

    prior_point_id=curr_point_id;
    prior_pnr_id=curr_pnr_id;
    prior_orig_subclass=curr_orig_subclass;
    prior_orig_class=curr_orig_class;
  }
  return count;
}

static int CreateSearchResponse(int point_dep,
                                const std::vector<SearchPaxResult>& searchResults,
                                xmlNodePtr resNode)
{
  return CreateSearchResponse(point_dep, searchResults, set<int>(), resNode);
}

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
    const std::vector<SearchPaxResult> searchPaxResult =
        runSearchPax(PnrId_t(NodeAsInteger("pnr_id",reqNode)));
    CreateSearchResponse(point_dep,searchPaxResult,resNode);
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","Choice");
    return;
  }
  CreateNoRecResponse(sum,resNode);
  NewTextChild(resNode,"ckin_state","BeforeReg");
  return;
}

static bool readTripHeaderAndOther(int point_id, xmlNodePtr reqNode, xmlNodePtr dataNode)
{
  if (TripsInterface::readTripHeader( point_id, dataNode ))
  {
    CheckInInterface::readTripCounters( point_id, dataNode );
    CheckInInterface::readTripData( point_id, dataNode );
    CheckInInterface::readTripSets( point_id, dataNode );
    TripsInterface::PectabsResponse(point_id, reqNode, dataNode);
    return true;
  };
  return false;
}

void CheckInInterface::SearchPaxByDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int term_point_id=NodeAsInteger("point_id", reqNode);
  ScanDocInfo doc;
  doc.fromXML(reqNode);
  if (doc.no.empty())
  {
    try
    {
      doc.parse(NowUTC());
    }
    catch(const EConvertError &e)
    {
      LogTrace(TRACE5) << ">>>>" << e.what();
      doc.bluntParsePNRUSDocNo();
    }
  }

  if (doc.no.empty())
  {
    string text=doc.getScanCodeForErrorMsg();
    if (!text.empty())
      throw UserException("WRAP.DEVICE.INVALID_SCAN_FORMAT",
                          LParams() << LParam("text", text));
    else
      throw UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");
  }

  CheckIn::TSimplePaxList paxs;
  CheckIn::Search()(paxs, doc);


  PaxInfoForSearchList list(paxs);

  list.trace();

  PaxInfoForSearchList::const_iterator iFound=list.end();

  xmlNodePtr dataNode=NewTextChild( resNode, "data" );

  for(int pass=0; pass<3; pass++)
  {
    //���� ��室: �饬 �� ⥪�饬 ��࠭��� ३�
    //��ன ��室: �饬 �� ३� �� ����� ����� ��४�������
    //��⨩ ��室: �� ��⠫쭮�
    for(PaxInfoForSearchList::const_iterator i=list.begin(); i!=list.end(); ++i)
    {
      const PaxInfoForSearch& info=*i;
      if (pass==0 || pass==1)
      {
        if (info.flt)
        {
          int point_id=info.flt.get().point_id;

          if (pass==0 && point_id!=term_point_id) continue;

          ReplaceTextChild( dataNode, "point_id", point_id );
          if (readTripHeaderAndOther( point_id, reqNode, dataNode ))
          {
            iFound=i;
            break;
          }
        }
      }
      else
      {
        if (info.flt)
        {
          ReplaceTextChild( dataNode, "point_id", info.flt.get().point_id );
          throw AstraLocale::UserException("MSG.PASSENGER.FROM_FLIGHT",
                                           LParams() << LParam("flt", GetTripName(info.flt.get(),ecCkin)));
        }
      }
    }
    if (iFound!=list.end()) break;
  }

  if (iFound==list.end()) throw UserException("MSG.PASSENGER.NOT_FOUND_BY_DOC_NUMBER",
                                              LParams() << LParam("doc_no", doc.getTrueNo()));

  const PaxInfoForSearch& found=*iFound;

  if (found.pax.grp_id!=ASTRA::NoExists)
  {
    LoadPaxByGrpId(GrpId_t(found.pax.grp_id), reqNode, resNode, false);
    return;
  }

  //���ᠦ�� �� ��ॣ����஢��
  if (!found.flt) throw UserException("MSG.PASSENGER.NOT_FOUND_BY_DOC_NUMBER",
                                      LParams() << LParam("doc_no", doc.getTrueNo()));
  int point_id=found.flt.get().point_id;


  ostringstream sql_filter;
  sql_filter << "crs_pax.pax_id=" << found.pax.id;
  const std::vector<SearchPaxResult> searchPaxResults =
      runSearchPax(PointId_t(point_id), ASTRA::psCheckin, true, sql_filter.str());
  if (searchPaxResults.empty())
    throw AstraLocale::UserException("MSG.PASSENGER.FROM_FLIGHT",
                                     LParams() << LParam("flt", GetTripName(found.flt.get(),ecCkin)));

  set<int> selectedPaxIds;
  selectedPaxIds.insert(found.pax.id);

  int count=CreateSearchResponse(point_id, searchPaxResults, selectedPaxIds, resNode);
  if (count>1)
    NewTextChild(resNode, "ckin_state", "Choice");
  else
    NewTextChild(resNode, "ckin_state", "BeforeReg");
}

struct SearchGroupResult
{
  PnrId_t pnr_id;
  std::string grp_name;
  std::string pax_name;
  int seats_all;
  int seats;
};

std::vector<SearchGroupResult> fetchSearchGroupResult(DB::TQuery& Qry)
{
  std::vector<SearchGroupResult> result;
  for(;!Qry.Eof;Qry.Next()) {
    const SearchGroupResult data = {
      PnrId_t(Qry.FieldAsInteger("pnr_id")),
      Qry.FieldAsString("grp_name"),
      Qry.FieldAsString("pax_name"),
      Qry.FieldAsInteger("seats_all"),
      Qry.FieldAsInteger("seats")
    };
    result.push_back(data);
  }
  return result;
}

std::vector<SearchGroupResult> runSearchGroup(const PointId_t& point_dep, int seats,
                                              const TPaxStatus& pax_status)
{
  DB::TQuery Qry(PgOra::getROSession({"CRS_PNR", "CRS_PAX", "PAX"}), STDLOG);
  string sql;
  sql=  "SELECT crs_pnr.pnr_id, \n"
        "       MIN(crs_pnr.grp_name) AS grp_name, \n"
        "       MIN(DECODE(crs_pax.pers_type,'��', \n"
        "                  crs_pax.surname||' '||crs_pax.name,'')) AS pax_name, \n"
        "       COUNT(*) AS seats_all, \n"
        "       SUM(DECODE(pax.pax_id,NULL,1,0)) AS seats \n"
        "FROM crs_pnr,crs_pax,pax,( \n";

  sql+= getSearchPaxSubquery(pax_status,
                             true,
                             false,
                             false,
                             true,
                             "",
                             true);

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

  Qry.SQLText = sql;
  Qry.CreateVariable("seats",otInteger,seats);
  bindSearchPaxQuery(Qry, PointId_t(point_dep), pax_status);
  const std::vector<CrsDisplaceData> items = CrsDisplaceData::load(PointId_t(point_dep));
  if (items.empty()) {
    Qry.Execute();
    return fetchSearchGroupResult(Qry);
  }
  std::vector<SearchGroupResult> result;
  for (const CrsDisplaceData& item: items) {
    Qry.SetVariable("point_id_tlg", item.point_id_tlg.get());
    Qry.SetVariable("airp_arv_tlg", item.airp_arv_tlg);
    Qry.SetVariable("class_tlg", item.class_tlg);
    Qry.SetVariable("status", item.status);
    Qry.Execute();
    const std::vector<SearchGroupResult> data = fetchSearchGroupResult(Qry);
    result.insert(result.end(), data.begin(), data.end());
  }
  return result;
}

void CheckInInterface::SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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

  TInquiryGroup grp;
  TInquiryGroupSummary sum;
  if (!pr_unaccomp)
  {
    readTripCounters(point_dep,resNode);

    ParseInquiryStr(query, pax_status, grp);
    TInquiryFormat fmt;
    fmt.persCountFmt=0;
    fmt.infSeatsFmt=0;

    DB::TQuery QryDesk(PgOra::getROSession({"DESKS","DESK_GRP_SETS"}), STDLOG);
    QryDesk.SQLText=
      "SELECT pers_count_fmt,inf_seats_fmt "
      "FROM desks,desk_grp_sets "
      "WHERE desks.grp_id=desk_grp_sets.grp_id AND desks.code=:desk ";
    QryDesk.CreateVariable("desk",otString,TReqInfo::Instance()->desk.code);
    QryDesk.Execute();
    if (!QryDesk.Eof)
    {
      if (!QryDesk.FieldIsNULL("pers_count_fmt"))
      {
        fmt.persCountFmt=QryDesk.FieldAsInteger("pers_count_fmt");
        if (fmt.persCountFmt!=0) fmt.persCountFmt=1;
      }
      if (!QryDesk.FieldIsNULL("inf_seats_fmt"))
      {
        fmt.infSeatsFmt=QryDesk.FieldAsInteger("inf_seats_fmt");
        if (fmt.infSeatsFmt!=0) fmt.infSeatsFmt=1;
      }
    }

    charter_search=GetTripSets(tsCharterSearch,fltInfo);

    GetInquiryInfo(grp,fmt,sum);
  }

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
    DB::TQuery Qry(PgOra::getROSession("TRIP_SETS"), STDLOG);
    Qry.SQLText="SELECT pr_tranz_reg FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable("point_id",otInteger,point_dep);
    Qry.Execute();
    if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
    if (Qry.FieldIsNULL("pr_tranz_reg")||Qry.FieldAsInteger("pr_tranz_reg")==0)
      throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");
  }

  if (pr_unaccomp)
  {
    NewTextChild(resNode,"ckin_state","BagBeforeReg");
    return;
  }

  if ((grp.prefix==TInquiryPrefix::Empty && grp.digCkin) ||
      grp.prefix==TInquiryPrefix::NoRec ||
      grp.prefix==TInquiryPrefix::ExtraCrew ||
      grp.prefix==TInquiryPrefix::DeadHeadCrew ||
      grp.prefix==TInquiryPrefix::MiscOperStaff)
  {
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  }

  if (pax_status==psCrew || grp.prefix==TInquiryPrefix::Crew)
  {
    if (pax_status!=psCrew)
    {
      pax_status=psCrew;
      ReplaceTextChild(inquiryNode,"pax_status",EncodePaxStatus(pax_status));
    }
    CreateCrewResponse(point_dep,sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  }

  if (grp.large)
  {
    //������ ��㯯�
    const std::vector<SearchGroupResult> searchResults = runSearchGroup(PointId_t(point_dep),
                                                                        sum.nPaxWithPlace,
                                                                        pax_status);
    if (!searchResults.empty())
    {
      xmlNodePtr pnrNode=NewTextChild(resNode,"groups");
      for(const SearchGroupResult& searchResult: searchResults)
      {
        node=NewTextChild(pnrNode,"pnr");
        NewTextChild(node,"pnr_id",searchResult.pnr_id.get());
        NewTextChild(node,"pax_name",searchResult.pax_name);
        NewTextChild(node,"grp_name",searchResult.grp_name);
        NewTextChild(node,"seats",searchResult.seats);
        NewTextChild(node,"seats_all",searchResult.seats_all);

        //�뢮��� � �ନ��� ��ࢮ� �ਮ��⭮� PNR �ᥣ�� � ����� ��������
        std::string tlg_airline;
        TPnrAddrs pnrs;
        pnrs.getByPnrId(searchResult.pnr_id.get(), tlg_airline);
        if (!pnrs.empty())
        {
          const TPnrAddrInfo& pnr=pnrs.front();
          node=NewTextChild(node,"pnr_addr");
          NewTextChild(node,"airline",pnr.airline);
          NewTextChild(node,"addr",pnr.addr);
        }
      }
      //��ப� NoRec
      node=NewTextChild(pnrNode,"pnr");
      if (!sum.fams.empty())
        NewTextChild(node,"grp_name",sum.fams.begin()->surname);
      NewTextChild(node,"seats",sum.nPaxWithPlace);
      NewTextChild(node,"seats_all",sum.nPaxWithPlace);

      NewTextChild(resNode,"ckin_state","GrpChoice");
      return;
    }
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  }

  std::vector<SearchPaxResult> searchPaxResults;
  for(int pass=0;pass<2;pass++)
  {
    string surnames;
    for(vector<TInquiryFamily>::iterator f=grp.fams.begin();f!=grp.fams.end();f++)
    {
      if (!surnames.empty()) surnames+=" OR ";
      surnames=surnames+"crs_pax.surname LIKE '"+f->surname.substr(0,pass==0?4:1)+"%'";
    }

    const std::vector<SearchPaxResult> results =
        runSearchPax(PointId_t(point_dep),
                     pax_status,
                     sum.nPax>1 && !charter_search,
                     surnames);

    if (!results.empty()) {
      searchPaxResults = results;
      break;
    }
  }
  if (searchPaxResults.empty())
  {
    CreateNoRecResponse(sum,resNode);
    NewTextChild(resNode,"ckin_state","BeforeReg");
    return;
  }

  CreateSearchResponse(point_dep,searchPaxResults,resNode);


//  if (fmt.persCountFmt==0)
  {
    //����ந� ����� ��१����� 䠬���� ���᪮���� �����
    xmlNodePtr foundPnr=NULL,foundTrip=NULL;
    xmlNodePtr tripNode,pnrNode,paxNode;
    vector<string> surnames;
    for(vector<TInquiryFamily>::iterator f=grp.fams.begin();f!=grp.fams.end();f++)
    {
      surnames.push_back(f->surname.substr(0,4));
    }

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
            }
          }
          if (node==NULL) break; //�� ������� 䠬����
        }
        if (i==surnames.end())
        {
          //�� 䠬���� �������
          if (foundPnr!=NULL)
          {
            // ������ 2 ���室��� ��㯯�
            foundPnr=NULL;
            foundTrip=NULL;
            break;
          }
          foundPnr=pnrNode;
          foundTrip=tripNode;
        }
      }
      if (pnrNode!=NULL) break;
    }

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
      }
    }
  }

  CreateNoRecResponse(sum,resNode);
  NewTextChild(resNode,"ckin_state","Choice");

 /* int free=CheckCounters(point_dep,point_arv,(char*)cl.c_str(),grp_status);
  if (free<sum.nPaxWithPlace)
    showErrorMessage("����㯭�� ���� ��⠫��� "+IntToString(free));*/

}

bool CheckFltOverload(int point_id, const TTripInfo &fltInfo, bool overload_alarm )
{
  if ( !overload_alarm ) return false;

  TTripSetList setList;
  setList.fromDB(point_id);
  if (setList.empty()) throw EXCEPTIONS::Exception("Flight not found in trip_sets (point_id=%d)",point_id);

  if (!setList.value<bool>(tsCheckLoad) && setList.value<bool>(tsOverloadReg)) return false;

  if (setList.value<bool>(tsOverloadReg))
  {
    AstraLocale::showErrorMessage("MSG.FLIGHT.MAX_COMMERCE");
    return true;
  }
  else
    throw CheckIn::OverloadException("MSG.FLIGHT.MAX_COMMERCE");
  return false;
}

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
}

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

  bool with_rcpt_info=(strcmp((const char *)reqNode->name, "BagPaxList")==0);

  bool check_pay_on_tckin_segs=false;
  if (with_rcpt_info) check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, operFlt);

  ostringstream sql;
  sql <<
    "SELECT "
    "  bag_pool_num,reg_no,surname,name,pax_grp.airp_arv, "
    "  last_trfer.airline AS trfer_airline, "
    "  last_trfer.flt_no AS trfer_flt_no, "
    "  last_trfer.suffix AS trfer_suffix, "
    "  last_trfer.airp_arv AS trfer_airp_arv, "
    "  last_tckin_seg.airline AS tckin_seg_airline, "
    "  last_tckin_seg.flt_no AS tckin_seg_flt_no, "
    "  last_tckin_seg.suffix AS tckin_seg_suffix, "
    "  last_tckin_seg.airp_arv AS tckin_seg_airp_arv, "
    "  pax_grp.class, "
    "  NVL(pax.cabin_class, pax_grp.class) AS cabin_class, "
    "  pax.subclass,pax_grp.status, "
    "  salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum) AS seat_no, "
    "  seats,wl_type,pers_type,ticket_rem, "
    "  ticket_no||DECODE(coupon_no,NULL,NULL,'/'||coupon_no) AS ticket_no, "
    "  pax_grp.excess_wt AS excess_wt_raw, "
    "  pax_grp.piece_concept, "
    "  mark_trips.airline AS airline_mark, "
    "  mark_trips.flt_no AS flt_no_mark, "
    "  mark_trips.suffix AS suffix_mark, "
    "  mark_trips.scd AS scd_local_mark, "
    "  mark_trips.airp_dep AS airp_dep_mark, "
    "  pax.grp_id, "
    "  pax.pax_id, "
    "  NVL(pax.cabin_class_grp, pax_grp.class_grp) AS cl_grp_id, "
    "  pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id,pax_grp.client_type, "
    "  pax_grp.bag_refuse "
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
    "      pr_brd IS NOT NULL "
    "ORDER BY pax.reg_no, pax.seats DESC"; //� ���饬 ���� ORDER BY

  //ProgTrace(TRACE5, "%s", sql.str().c_str());
  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);

  Qry.Execute();
  xmlNodePtr node=NewTextChild(resNode,"passengers");
  TComplexBagExcessNodeList excessNodeList(OutputLang(), {TComplexBagExcessNodeList::ContainsOnlyNonZeroExcess});

  using namespace CKIN;
  BagReader bag_reader(PointId_t(point_id), std::nullopt, READ::BAGS_AND_TAGS);
  MainPax viewEx;
  if (!Qry.Eof)
  {
    createDefaults=true;

    int col_pax_id=Qry.FieldIndex("pax_id");
    int col_bag_pool_num= Qry.FieldIndex("bag_pool_num");
    int col_reg_no=Qry.FieldIndex("reg_no");
    int col_surname=Qry.FieldIndex("surname");
    int col_name=Qry.FieldIndex("name");
    int col_airp_arv=Qry.FieldIndex("airp_arv");
    int col_class=Qry.FieldIndex("class");
    int col_cabin_class=Qry.FieldIndex("cabin_class");
    int col_subclass=Qry.FieldIndex("subclass");
    int col_status=Qry.FieldIndex("status");
    int col_seat_no=Qry.FieldIndex("seat_no");
    int col_seats=Qry.FieldIndex("seats");
    int col_wl_type=Qry.FieldIndex("wl_type");
    int col_pers_type=Qry.FieldIndex("pers_type");
    int col_ticket_rem=Qry.FieldIndex("ticket_rem");
    int col_ticket_no=Qry.FieldIndex("ticket_no");
    int col_bag_refuse=Qry.FieldIndex("bag_refuse");
    int col_piece_concept=Qry.FieldIndex("piece_concept");

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
    int col_excess_wt_raw=Qry.FieldIndex("excess_wt_raw");

    map< pair<int/*grp_id*/, int/*pax_id*/>, pair<bool/*pr_payment*/, bool/*pr_receipts*/> > rcpt_complete_map;
    TPaxSeats priorSeats(point_id);
    TQuery PaxDocQry(&OraSession);
    TRemGrp rem_grp;
    rem_grp.Load(retCKIN_VIEW, operFlt.airline);

    for(;!Qry.Eof;Qry.Next())
    {
      int grp_id = Qry.FieldAsInteger(col_grp_id);
      int pax_id = Qry.FieldAsInteger(col_pax_id);
      int bag_refuse = Qry.FieldAsInteger(col_bag_refuse);

      std::optional<int> bag_pool_num = std::nullopt;
      if(!Qry.FieldIsNULL(col_bag_pool_num)) {
          bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);
          viewEx.saveMainPax(GrpId_t(grp_id), PaxId_t(pax_id), bag_refuse!=0);
      }
      int reg_no = Qry.FieldAsInteger(col_reg_no);
      string cl = Qry.FieldAsString(col_class);
      string cabin_cl = Qry.FieldAsString(col_cabin_class);
      TPaxStatus status_id=DecodePaxStatus(Qry.FieldAsString(col_status));
      bool piece_concept=Qry.FieldAsInteger(col_piece_concept)!=0;
      int excess_wt_raw = Qry.FieldAsInteger(col_excess_wt_raw);

      if (with_rcpt_info) {
        if (status_id == psCrew) {
          continue;
        }
        if (!need_for_payment(GrpId_t(grp_id), cl, bag_refuse,
                              piece_concept, excess_wt_raw,
                              pax_id > 0 ? std::optional<PaxId_t>(pax_id)
                                         : std::optional<PaxId_t>()))
        {
          continue;
        }
      }

      xmlNodePtr paxNode=NewTextChild(node,"pax");
      NewTextChild(paxNode,"pax_id",pax_id);
      NewTextChild(paxNode,"reg_no",reg_no);
      NewTextChild(paxNode,"surname",Qry.FieldAsString(col_surname));
      NewTextChild(paxNode,"name",Qry.FieldAsString(col_name));

      NewTextChild(paxNode,"airp_arv",ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));

      TLastTrferInfo trferInfo(Qry);
      NewTextChild(paxNode,"last_trfer",trferInfo.str(),"");
      TLastTCkinSegInfo tckinSegInfo(Qry);
      NewTextChild(paxNode,"last_tckin_seg",tckinSegInfo.str(),"");

      std::string class_change_str;
      if (status_id!=psCrew)
      {
        if (!cabin_cl.empty())
        {
          NewTextChild(paxNode,"class",ElemIdToCodeNative(etClass, cabin_cl), def_class);
          if (cl!=cabin_cl)
            class_change_str=" "+classIdsToCodeNative(cl, cabin_cl);
        }
      }
      else NewTextChild(paxNode,"class",CREW_CLASS_VIEW); //crew compatible

      NewTextChild(paxNode,"subclass",ElemIdToCodeNative(etSubcls, Qry.FieldAsString(col_subclass)));

      ostringstream seat_no_str;
      ostringstream seat_no;
      bool seat_no_alarm=false;

      if (!free_seating)
      {
        if (Qry.FieldIsNULL(col_wl_type))
        {
          //�� �� ���� ��������, �� �������� ����� ���� �� ᬥ�� ����������
          if (!cabin_cl.empty() && Qry.FieldIsNULL(col_seat_no) && Qry.FieldAsInteger(col_seats)>0)
          {

            seat_no_str << "("
                        << priorSeats.getSeats(pax_id,"seats")
                        << ")"
                        << class_change_str;
            seat_no_alarm=true;
          }
        }
        else
        {
          seat_no_str << "��" << class_change_str;
          seat_no_alarm=true;
        }
      }

      seat_no << Qry.FieldAsString(col_seat_no) << class_change_str;

      NewTextChild(paxNode,"seat_no_str",seat_no_str.str(),"");
      NewTextChild(paxNode,"seat_no_alarm",(int)seat_no_alarm, (int)false);
      NewTextChild(paxNode,"seat_no",seat_no.str());
      NewTextChild(paxNode,"seats",Qry.FieldAsInteger(col_seats),1);
      NewTextChild(paxNode,"pers_type",ElemIdToCodeNative(etPersType, Qry.FieldAsString(col_pers_type)), def_pers_type);
      NewTextChild(paxNode,"document", CheckIn::GetPaxDocStr(std::nullopt, pax_id, true), "");

      NewTextChild(paxNode,"ticket_rem",Qry.FieldAsString(col_ticket_rem),"");
      NewTextChild(paxNode,"ticket_no",Qry.FieldAsString(col_ticket_no),"");

      NewTextChild(paxNode,"bag_amount",bag_reader.bagAmount(GrpId_t(grp_id), bag_pool_num ),0);
      NewTextChild(paxNode,"bag_weight",bag_reader.bagWeight(GrpId_t(grp_id), bag_pool_num),0);
      NewTextChild(paxNode,"rk_weight",bag_reader.rkWeight(GrpId_t(grp_id), bag_pool_num),0);

      excessNodeList.add(paxNode, "excess", TBagPieces(countPaidExcessPC(PaxId_t(Qry.FieldAsInteger( col_pax_id )))),
        TBagKilos(viewEx.excessWt(GrpId_t(grp_id), PaxId_t(pax_id), excess_wt_raw)));
      NewTextChild(paxNode,"tags",bag_reader.tags(GrpId_t(grp_id), bag_pool_num, reqInfo->desk.lang),"");
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
        }
      }
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

      NewTextChild(paxNode,"client_type_id",
                           (int)DecodeClientType(Qry.FieldAsString(col_client_type)),
                           def_client_type_id);
      NewTextChild(paxNode,"status_id",(int)status_id,def_status_id);
    }
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
        NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
      }
    }
  }

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
    "  pax_grp.grp_id, "
    "  pax_grp.hall AS hall_id, "
    "  pax_grp.point_arv,pax_grp.user_id,pax_grp.client_type, "
    "  pax_grp.class, "
    "  pax_grp.bag_refuse, "
    "  pax_grp.excess_wt AS excess_wt_raw "
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
    "      point_dep=:point_id AND class IS NULL AND status NOT IN ('E') "
    "ORDER BY pax_grp.grp_id";

  //ProgTrace(TRACE5, "%s", sql.str().c_str());
  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  node=NewTextChild(resNode,"unaccomp_bag");
  MainPax viewExUnac(true);
  if (!Qry.Eof)
  {
    createDefaults=true;
    for(;!Qry.Eof;Qry.Next())
    {
      GrpId_t grp_id{Qry.FieldAsInteger("grp_id")};
      const std::string cl = Qry.FieldAsString("class");
      int bag_refuse = Qry.FieldAsInteger("bag_refuse");
      bool piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
      int excess_wt_raw = Qry.FieldAsInteger("excess_wt_raw");

      if (with_rcpt_info) {
        if (!need_for_payment(GrpId_t(grp_id), cl, bag_refuse,
                              piece_concept, excess_wt_raw, {}))
        {
          continue;
        }
      }

      xmlNodePtr paxNode=NewTextChild(node,"bag");
      NewTextChild(paxNode,"airp_arv",ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp_arv")));

      TLastTrferInfo trferInfo(Qry);
      NewTextChild(paxNode,"last_trfer",trferInfo.str(),"");
      TLastTCkinSegInfo tckinSegInfo(Qry);
      NewTextChild(paxNode,"last_tckin_seg",tckinSegInfo.str(),"");

      NewTextChild(paxNode,"bag_amount",bag_reader.bagAmountUnaccomp(grp_id),0);
      NewTextChild(paxNode,"bag_weight",bag_reader.bagWeightUnaccomp(grp_id),0);
      NewTextChild(paxNode,"rk_weight",bag_reader.rkWeightUnaccomp(grp_id),0);

      excessNodeList.add(paxNode, "excess", TBagPieces(0),
        TBagKilos(viewExUnac.excessWtUnnacomp(grp_id, excess_wt_raw, bag_refuse!=0)));

      NewTextChild(paxNode,"tags", bag_reader.tagsUnaccomp(grp_id, reqInfo->desk.lang),"");
      if (with_rcpt_info)
      {
        string receipts=piece_concept?PieceConcept::GetBagRcptStr(grp_id.get(), NoExists):
                                      WeightConcept::GetBagRcptStr(grp_id.get(), NoExists);
        NewTextChild(paxNode,"rcpt_no_list",receipts,"");
        // �� �� ���⠭樨 �ᯥ�⠭�
        //0 - ���筮 �����⠭�
        //1 - �� �����⠭�
        //2 - ��� �� ����� ���⠭樨
        int rcpt_complete=2;
        if (!receipts.empty()) rcpt_complete=piece_concept?(int)true:
                                                           (int)WeightConcept::BagPaymentCompleted(grp_id.get());
        NewTextChild(paxNode,"rcpt_complete",rcpt_complete,2);
      }
      //�����䨪����
      NewTextChild(paxNode,"grp_id",grp_id.get());
      if (!Qry.FieldIsNULL("hall_id"))
        NewTextChild(paxNode,"hall_id",Qry.FieldAsInteger("hall_id"));
      else
        NewTextChild(paxNode,"hall_id",-1);
      NewTextChild(paxNode,"point_arv",Qry.FieldAsInteger("point_arv"));
      if (!Qry.FieldIsNULL("user_id"))
        NewTextChild(paxNode,"user_id",Qry.FieldAsInteger("user_id"));
      else
        NewTextChild(paxNode,"user_id",-1);

      NewTextChild(paxNode,"client_type_id",
                           (int)DecodeClientType(Qry.FieldAsString("client_type")),
                           def_client_type_id);
      NewTextChild(paxNode,"status_id",
                           (int)DecodePaxStatus(Qry.FieldAsString("status")),
                           def_status_id);
    }
  }



  Qry.Close();

  if (createDefaults)
  {
    xmlNodePtr defNode = NewTextChild( resNode, "defaults" );
    NewTextChild(defNode, "last_trfer", "");
    NewTextChild(defNode, "last_tckin_seg", "");
    NewTextChild(defNode, "bag_amount", 0);
    NewTextChild(defNode, "bag_weight", 0);
    NewTextChild(defNode, "rk_weight", 0);
    NewTextChild(defNode, "excess", "0");
    NewTextChild(defNode, "tags", "");
    //�� ��� ��ᮯ஢��������� ������
    NewTextChild(defNode, "name", "");
    NewTextChild(defNode, "class", def_class);
    NewTextChild(defNode, "brand", "");
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
    }
    //�����䨪����
    NewTextChild(defNode, "client_type_id", def_client_type_id);
    NewTextChild(defNode, "status_id", def_status_id);
  }

  get_new_report_form("ArrivalPaxList", reqNode, resNode);
  xmlNodePtr variablesNode = STAT::set_variables(resNode);
  TRptParams rpt_params(reqInfo->desk.lang);
  PaxListVars(point_id, rpt_params, variablesNode);
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.ARRIVAL_PAX_LIST", LParams() << LParam("flight", get_flight(variablesNode))));
}

bool GetUsePS()
{
    return false; //!!!
}

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
    }

    TypeB::TTlgParser parser;
    return ParseFQTRem(parser,rem2.text,fqt);
  }
  return false;
}

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
        }

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
            }

            try
            {
              const TAirlinesRow &row=(const TAirlinesRow&)(base_tables.get("airlines").get_row("code/code_lat",fqth.airline));
              strcpy(fqth.airline,row.code.c_str());
            }
            catch (const EBaseTableError&)
            {
              throw UserException("MSG.AIRLINE.INVALID_INPUT_VALUE",
                                  LParams()<<LParam("airline", string(fqth.airline))); //WEB
            }
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
        }
      }
      catch(const std::exception&)
      {
        switch(k)
        {
          case 0:
            *fqth.airline=0;
            throw;
          case 1:
            *fqth.no=0;
            throw;
        }
      }
    }
    catch(const UserException &E)
    {
      throw UserException("WRAP.REMARK_ERROR",
                          LParams()<<LParam("rem_code", string(fqth.rem_code))
                                   <<LParam("text", E.getLexemaData( ) )); //WEB
    }
    fqt.rem=fqth.rem_code;
    fqt.airline=fqth.airline;
    fqt.no=fqth.no;
    fqt.extra=fqth.extra;
    return true;
  }
  return false;
}

/*
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
*/

string CountryCodeFromRemText(string text)
{
  string result;
  size_t delim_pos = text.find_first_of(" /");
  if (delim_pos != string::npos && delim_pos < text.length())
  {
    string tail = text.substr(delim_pos + 1);
    TElemFmt fmt;
    string country_code = ElemToElemId(etCountry, tail, fmt);
    if (fmt != efmtUnknown)
    {
      const TCountriesRow& countryRow = static_cast<const TCountriesRow&>(base_tables.get("countries").get_row("code", country_code));
      result = countryRow.code_lat;
    }
  }
  LogTrace(TRACE5) << __func__ << " text = '" << text << "' result = '" << result << "'";
  return result;
}

void VerifyAPPSOverrideRem(const CheckIn::TPaxRemItem &rem)
{
  if (rem.code == "OVRA" or rem.code == "OVRG")
    if (CountryCodeFromRemText(rem.text).empty())
      throw UserException("MSG.APPS_OVERRIDE_COUNTRY_INVALID", LParams() << LParam("code", rem.code));
}

void HandleAPPSRems(const std::multiset<CheckIn::TPaxRemItem> &rems, std::string& override, bool& is_forced)
{
  const string REM_RSIA = "RSIA";
  const string REM_OVRA = "OVRA";
  const string REM_OVRG = "OVRG";
  struct TAppsOverrideCode { bool A = false; bool G = false; };
  map<string, TAppsOverrideCode> code_map; // country, override code
  for (const auto& rem : rems)
  {
    if (rem.code == REM_RSIA)
      is_forced = true;
    if (rem.code == REM_OVRA || rem.code == REM_OVRG)
    {
      string country_code = CountryCodeFromRemText(rem.text);
      if (!country_code.empty())
      {
        is_forced = true;
        if (rem.code == REM_OVRA) code_map[country_code].A = true;
        if (rem.code == REM_OVRG) code_map[country_code].G = true;
      }
    }
  }
  string result;
  for (const auto& code_pair : code_map)
  {
    string code;
    if (code_pair.second.A) code = "A";
    if (code_pair.second.G) code = "G";
    string country = code_pair.first;
    if (!code.empty() && !country.empty()) result += (code + country);
  }
  if (!result.empty())
    override = result.substr(0, 15);
  LogTrace(TRACE5) << __func__ << " override = '" << override << "'";
}

boost::optional<std::string> CheckRefusability(const TAdvTripInfo& fltInfo, int pax_id)
{
  class UnregDenial {};
  class UnregDenialTerm {};
  class UnregAllowed {};
  try
  {
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (reqInfo->client_type==ctWeb ||
        reqInfo->client_type==ctKiosk ||
        reqInfo->client_type==ctMobile)
    {
      //�⬥�� ॣ����樨 � ᠩ�, ���᪠ ��� �����쭮��
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
          "SELECT pax_grp.grp_id, pax_grp.client_type, pax.* "
          "FROM pax_grp, pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:pax_id";
      Qry.CreateVariable("pax_id", otInteger, pax_id);
      Qry.Execute();
      if (Qry.Eof) throw UnregDenial();

      CheckIn::TSimplePaxItem pax;
      pax.fromDB(Qry);
      if (!pax.refuse.empty() || //�⬥�� ॣ����樨
          pax.pr_brd) throw UnregDenial();
      int grp_id=Qry.FieldAsInteger("grp_id");
      TClientType ckinClientType=DecodeClientType(Qry.FieldAsString("client_type"));
      if (!(ckinClientType==ctWeb ||
            ckinClientType==ctKiosk ||
            ckinClientType==ctMobile)) throw UnregDenial(); //ॣ����樨 �� � ᠩ�, ���᪠ ��� �����쭮��

      if (pax.HaveBaggage()) throw UnregDenial();

      if (GetSelfCkinSets(tsAllowCancelSelfCkin, fltInfo, reqInfo->client_type)) throw UnregAllowed();

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
      Qry.CreateVariable("point_dep", otInteger, fltInfo.point_id);
      Qry.CreateVariable("grp_id", otInteger, grp_id);
      Qry.Execute();
      if (!Qry.Eof) throw UnregDenial();
    }
    if (reqInfo->client_type==ctTerm)
    {
      CheckIn::TSimplePaxItem pax;
      pax.getByPaxId(pax_id);
      if (!pax.refuse.empty()) throw UnregAllowed();
      if (GetTripSets(tsNoRefuseIfBrd, fltInfo) && pax.pr_brd) throw UnregDenialTerm();
    }
  }
  catch(const UnregDenial&)
  {
    return std::string("MSG.PASSENGER.UNREGISTRATION_DENIAL");
  }
  catch(const UnregDenialTerm&)
  {
    return std::string("MSG.PASSENGER.UNREGISTRATION_DENIAL_BRD");
  }
  catch(const UnregAllowed&) {}

  return boost::none;
}

static xmlNodePtr findIatciSegNode(xmlNodePtr reqNode)
{
    xmlNodePtr result = NULL;
    xmlNodePtr segsNode = findNodeR(reqNode, "segments");
    for(xmlNodePtr segNode = segsNode->children; segNode != NULL; segNode = segNode->next)
    {
        if(getIntFromXml(segNode, "point_dep") < 0 &&
           getIntFromXml(segNode, "point_arv") < 0)
        {
            result = segNode;
            break;
        }
    }

    return result;
}

static boost::optional<TGrpMktFlight> LoadIatciMktFlight(const GrpId_t& grpId)
{
    using namespace astra_api::xml_entities;
    ASSERT(grpId.get() > 0);
    std::string xmlData = iatci::IatciXmlDb::load(grpId);
    if(!xmlData.empty())
    {
        XMLDoc xmlDoc = ASTRA::createXmlDoc(xmlData);
        xmlNodePtr tripHeaderNode = findNodeR(xmlDoc.docPtr()->children, "tripheader");
        if(tripHeaderNode)
        {
            tst();
            XmlTripHeader th = XmlEntityReader::readTripHeader(tripHeaderNode);

            TGrpMktFlight grpMktFlt;
            grpMktFlt.airline        = th.airline;
            grpMktFlt.flt_no         = th.flt_no;
            grpMktFlt.suffix         = th.suffix;
            grpMktFlt.scd_date_local = th.scd_out_local;
            grpMktFlt.airp_dep       = th.airp;
            return grpMktFlt;
        }
    }

    tst();
    return boost::none;
}

static void transformSavePaxRequestByIatci(xmlNodePtr reqNode)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    xmlNodePtr iatciSegsNode = NULL;
    xmlNodePtr segNode = findIatciSegNode(reqNode);
    while(segNode != NULL)
    {
        if(iatciSegsNode == NULL)
            iatciSegsNode = newChild(reqNode, "iatci_segments");
        CopyNode(iatciSegsNode, segNode, true/*recursive*/);
        RemoveNode(segNode);
        segNode = findIatciSegNode(reqNode);
    }
}

static void transformSavePaxRequestByIatci(xmlNodePtr reqNode, const GrpId_t& grpId)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " grpId=" << grpId;

    xmlNodePtr node = findNodeR(reqNode, "iatci_segments");
    if(node)
    {
        // ���� ���� iatci-ᥣ����
        xmlNodePtr segNode = node->children;
        if(!findNode(segNode, "mark_flight"))
        {
            xmlNodePtr pseudoMarkFlightNode = findNode(segNode, "pseudo_mark_flight");
            if(!pseudoMarkFlightNode)
                pseudoMarkFlightNode = newChild(segNode, "pseudo_mark_flight");

            boost::optional<TGrpMktFlight> mktFlt = LoadIatciMktFlight(grpId);
            if(mktFlt)
            {
                NewTextChild(pseudoMarkFlightNode, "airline", mktFlt->airline);
                NewTextChild(pseudoMarkFlightNode, "flt_no", mktFlt->flt_no);
                NewTextChild(pseudoMarkFlightNode, "suffix", mktFlt->suffix);
                NewTextChild(pseudoMarkFlightNode, "scd", DateTimeToStr(mktFlt->scd_date_local));
                NewTextChild(pseudoMarkFlightNode, "airp_dep", mktFlt->airp_dep);

                LogTrace(TRACE3) << "mktFlt->airline= " << mktFlt->airline << "; "
                                 << "mktFlt->flt_no= " << mktFlt->flt_no << "; "
                                 << "mktFlt->suffix= " << mktFlt->suffix << "; "
                                 << "mktFlt->scd_date_local= " << DateTimeToStr(mktFlt->scd_date_local )<< "; "
                                 << "mktFlt->airp_dep= " << mktFlt->airp_dep;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace iatci {

std::vector<CheckIn::IatciSegment> readIatciSegs(int grpId, xmlNodePtr ediResNode)
{
    using namespace astra_api::xml_entities;
    LogTrace(TRACE3) << __FUNCTION__ << " by grp_id:" << grpId;
    if(ediResNode == NULL) {
        tst();
        return {};
    }
    xmlNodePtr iatciSegsNode = findNodeR(ediResNode, "segments_for_log");
    if(iatciSegsNode == NULL) {
        tst();
        return {};
    }

    XmlCheckInTabs tabs(iatciSegsNode);

    std::vector<CheckIn::IatciSegment> iatciSegs;
    for(const XmlCheckInTab& tab: tabs.ediTabs()) {
        iatciSegs.emplace_back(tab.xmlSeg());
    }
    return iatciSegs;
}

}//namespace iatci

/////////////////////////////////////////////////////////////////////////////////////////

void CheckInInterface::SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("segments/segment/point_dep",reqNode);
    TTripStage ts;
    TTripStages::LoadStage(point_id, sCloseCheckIn, ts);
    if(
            ts.act != NoExists and
            TReqInfo::Instance()->user.access.rights().permitted(997)
      )
        throw AstraLocale::UserException(ElemIdToNameLong(etRight, 997));
    SavePax(reqNode, NULL, resNode);
}

static bool needSyncEdsEts(xmlNodePtr answerResNode)
{
    return (answerResNode == NULL ||
            (answerResNode != NULL && !findNodeR(answerResNode, "tickets")
                                   && !findNodeR(answerResNode, "emdocs")));
}

// ��楤�� ������ �������� true ⮫쪮 � ⮬ ��砥 �᫨ �ந������� ॠ�쭠� ॣ������
// ��� � ��砥 iatci-ॣ����樨
bool CheckInInterface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode)
{
//  LogTrace(TRACE5) << XMLTreeToText(reqNode->doc);

    if(ediResNode == NULL && IatciInterface::MayNeedSendIatci(reqNode))
    {
      tryGenerateBagTags(reqNode);
      ASTRA::commit();
    }

    DB::Savepoint sp("sp_savepax");
    if(ediResNode == NULL && IatciInterface::MayNeedSendIatci(reqNode))
    {
        if(!ReqParams(reqNode).getBoolParam("may_need_send_iatci"))
        {
            // ����������, �� �������� ���� ��᫠�� iatci-�����
            // � "���祬" edifact-ᥣ���� �� ���筮�� SavePax
            ReqParams(reqNode).setBoolParam("may_need_send_iatci", true);
            transformSavePaxRequestByIatci(reqNode);
        }
    }

    TChangeStatusList ChangeStatusInfo;
    SirenaExchange::TLastExchangeList SirenaExchangeList;
    CheckIn::TAfterSaveInfoList AfterSaveInfoList;
    bool httpWasSent = false;
    bool result=SavePax(reqNode, ediResNode, ChangeStatusInfo,
                        SirenaExchangeList, AfterSaveInfoList,
                        httpWasSent);
    if (result)
    {
        if(httpWasSent) {
            return false;
        }

        if (needSyncEdsEts(ediResNode) && !ChangeStatusInfo.empty())
        {
            //��� �� ���� ����� �㤥� ��ࠡ��뢠����
            sp.rollback();//�⪠�
            ChangeStatusInterface::ChangeStatus(reqNode, ChangeStatusInfo);
            SirenaExchangeList.handle(__FUNCTION__);
            return false;
        }
        else
        {
            SirenaExchangeList.handle(__FUNCTION__);
        }

        CheckIn::TAfterSaveInfoData data(reqNode, ediResNode);
        AfterSaveInfoList.handle(data, __FUNCTION__);
        if(data.httpWasSent) {
            return true;
        }

        // �᫨ ��� �������� �ॡ���� ��᫠�� iatci-�����, �
        // ��⠥��� ��᫠�� ��� �, �᫨ ��᫠�, 䨪��㥬 ��� 䠪�
        // � �� "was_sent_iatci"
        if(ReqParams(reqNode).getBoolParam("may_need_send_iatci"))
        {
            bool willSentIatci = IatciInterface::WillBeSentCheckInRequest(reqNode, ediResNode);
            if(willSentIatci) {
                sp.rollback(); // ����� �⪠�, �᫨ �㤥� ��᫠� iatci-�����
            }

            // ᭨��� 䫠� ����室����� ���뫪� iatci-�����
            ReqParams(reqNode).setBoolParam("may_need_send_iatci", false);
            GrpId_t grpId(AfterSaveInfoList.front().segs.back().grp_id);
            transformSavePaxRequestByIatci(reqNode, grpId);
            // ���⠢�� 䫠� 䠪� ���뫪� iatci-�����. ����� ���� ������ � false,
            // �᫨ update �� ���஭�� edifact-�������
            ReqParams(reqNode).setBoolParam("was_sent_iatci", willSentIatci);
            bool wasSentIatci = IatciInterface::DispatchCheckInRequest(reqNode, ediResNode);
            ASSERT(wasSentIatci == willSentIatci);
            if(wasSentIatci) {
                return true; // ��᫠�� iatci-����� - ��室��. ������ ��᫥ ����祭�� �⢥�
            }
        }

        if (!AfterSaveInfoList.empty() &&
            !AfterSaveInfoList.front().segs.empty())
        {
            GrpId_t grpId(AfterSaveInfoList.front().segs.front().grp_id);

            if (AfterSaveInfoList.front().action==CheckIn::actionSvcPaymentStatus)
            {
                if(!ReqParams(reqNode).getBoolParam("was_sent_iatci")) {
                    EMDAutoBoundInterface::refreshEmd(EMDAutoBoundGrpId(grpId), reqNode);
                }
            }

            if (resNode!=nullptr)
              LoadPaxByGrpId(grpId, reqNode, resNode, true);
        }
    }
    return result;
}

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
                           TReqInfo::Instance()->api_mode ? "IATCIP" : TReqInfo::Instance()->desk.code,
                           agent_stat_ondate,
                           first_segment?(agent_stat_period<1?1:agent_stat_period):0,
                           agentStat.pax_amount,
                           agentStat.dpax_amount,
                           first_segment?STAT::agent_stat_t(0,0):agentStat.dpax_amount,
                           first_segment?agentStat.dbag_amount:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.dbag_weight:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.drk_amount:STAT::agent_stat_t(0,0),
                           first_segment?agentStat.drk_weight:STAT::agent_stat_t(0,0));
  }
}

bool CheckIn::TAfterSaveInfoData::needSync() const
{
    return SirenaExchange::needSync(answerResNode);
}

xmlNodePtr CheckIn::TAfterSaveInfoData::getAnswerNode() const
{
    xmlNodePtr answerNode = SirenaExchange::findAnswerNode(answerResNode);
    ASSERT(answerNode != NULL);
    return answerNode;
}

void CheckIn::TAfterSaveInfoList::handle(TAfterSaveInfoData& data, const string& where)
{
  LogTrace(TRACE3) << __FUNCTION__ << " " << where;
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
    data.grpId = i->segs.front().grp_id;
    data.action = i->action;
    CheckInInterface::AfterSaveAction(data);
    if (data.httpWasSent) break;
    i->toLog(where);
  }
}

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
        }
      }
    }
  }

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
      }
    }
  }
}

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
    }
  }
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
    {
      j->second.first.bag_weight=i->second;
      j->second.first.weight_unit="K";
    }
  }
  return true;
}

bool CompareGrpViewItem( const TrferList::TGrpViewItem &item1,
                         const TrferList::TGrpViewItem &item2 )
{
  if ((item1.calc_status==NoExists)!=(item2.calc_status==NoExists))
    return item1.calc_status==NoExists;
  return item1<item2;
}

void GetInboundTransferForTerm(const CheckIn::TTransferList &trfer,
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
  for(CheckIn::TTransferList::const_iterator t=trfer.begin(); t!=trfer.end(); ++t, seg_no++)
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
        }
      }
    }

    if (!p->trferAttachable()) continue;

    inbound_trfer_grp_out.paxs.push_back(InboundTrfer::TPaxItem("", p->pax.surname, p->pax.name));
  }

  //�室�騩 �࠭���
  GetNewGrpInfo(segList.begin()->grp.point_dep,
                inbound_trfer_grp_out,
                inbound_trfer_pax_out_ids,
                inbound_trfer);
}

void checkTransferRouteForErrors(const TSegList &segList,
                                 const CheckIn::TTransferList &inbound_trfer_route,
                                 InboundTrfer::TNewGrpInfo &inbound_trfer)
{
  try
  {
    int seg_no=1;
    for(TSegList::const_iterator iSegListItem=segList.begin();
        iSegListItem!=segList.end();
        seg_no++,++iSegListItem)
      inbound_trfer_route.check(iSegListItem->flt.point_id, false, seg_no);
  }
  catch(const UserException&)
  {
    inbound_trfer.conflicts.insert(InboundTrfer::ConflictReason::OutRouteWithErrors);
  }
}

void GetInboundTransferForWeb(const TSegList &segList,
                              bool pr_unaccomp,
                              InboundTrfer::TNewGrpInfo &inbound_trfer,
                              CheckIn::TTransferList &inbound_trfer_route)
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
      paxTrfer.subclass=p->pax.cabin.subcl;
      paxTrfer.subclass_fmt=efmtCodeNative;
      trfer.pax.push_back(paxTrfer);
    }

    inbound_trfer_grp_out.trfer.insert(make_pair(seg_no-1, trfer));
  }

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0";
  Qry.DeclareVariable("pax_id", otInteger);

  map<int, InboundTrfer::TGrpItem> grpItems;
  InboundTrfer::TGrpItem jointGrpItem;
  bool trferTruncated=false;
  for(CheckIn::TPaxList::const_iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
  {
    if (p->pax.id==NoExists)
      throw Exception("%s: strange situation: p->pax.id==NoExists", __func__);

    inbound_trfer_pax_out_ids.insert(p->pax.id);

    int pnr_id=NoExists;

    Qry.SetVariable("pax_id", p->pax.id);
    Qry.Execute();
    if (!Qry.Eof) pnr_id=Qry.FieldAsInteger("pnr_id");

    const auto& res=grpItems.emplace(pnr_id, InboundTrfer::TGrpItem());
    InboundTrfer::TGrpItem& tmpGrpItem=res.first->second;
    if (res.second && pnr_id!=NoExists)
    {
      //���⠥� �࠭���� �������
      CheckInInterface::GetOnwardCrsTransfer(pnr_id,
                                             true,
                                             segList.begin()->flt,
                                             segList.begin()->grp.airp_arv,
                                             tmpGrpItem.trfer);
      tmpGrpItem.normalizeTrfer();

      map<int, pair<CheckIn::Segments, TTrferSetsInfo> > trfer_segs;

      CheckInInterface::GetTrferSets(segList.begin()->flt,
                                     segList.begin()->grp.airp_arv,
                                     "",
                                     tmpGrpItem.trfer,
                                     true,
                                     trfer_segs);
      for(const auto& s : trfer_segs)
        if (!s.second.second.trfer_permit)
        {
          tmpGrpItem.trfer.erase(tmpGrpItem.trfer.find(s.first), tmpGrpItem.trfer.end());
          trferTruncated=true;
          break;
        }
    }

    if (p==segList.begin()->paxs.begin())
      jointGrpItem.trfer=tmpGrpItem.trfer;

    if (!jointGrpItem.addSubclassesForEqualTrfer(tmpGrpItem))
    {
      jointGrpItem.printTrfer("jointGrpItem", true);
      tmpGrpItem.printTrfer("tmpGrpItem", true);
      inbound_trfer.conflicts.insert(InboundTrfer::ConflictReason::OutRouteDiffer);
      break;  //�� ��� ��室 �� ��楤���
    }

    if (!p->trferAttachable()) continue;

    inbound_trfer_grp_out.paxs.push_back(InboundTrfer::TPaxItem("", p->pax.surname, p->pax.name));
  }

  for(bool checkSubclassesEquality : {false, true})
  {
    if (!inbound_trfer_grp_out.similarTrfer(jointGrpItem, checkSubclassesEquality))
    {
      inbound_trfer_grp_out.printTrfer("inbound_trfer_grp_out", true);
      jointGrpItem.printTrfer("jointGrpItem", true);
      inbound_trfer.conflicts.insert(InboundTrfer::ConflictReason::OutRouteDiffer);
      if (checkSubclassesEquality)
        LogError(STDLOG) << __func__ << " warning: strange situation - different subclasses";
      break;
    }
  }

  inbound_trfer_grp_out.trfer=jointGrpItem.trfer;

  //�室�騩 �࠭���
  if (inbound_trfer.conflicts.empty())
  {
    InboundTrfer::GetNewGrpInfo(segList.begin()->grp.point_dep,
                                inbound_trfer_grp_out,
                                inbound_trfer_pax_out_ids,
                                inbound_trfer);
  }
  if (inbound_trfer.conflicts.empty())
  {
    if (!inbound_trfer.tag_map.empty())
    {
      map<int, CheckIn::TTransferItem>::const_iterator t1=inbound_trfer_grp_out.trfer.begin();
      map<int, CheckIn::TTransferItem>::const_iterator t2=inbound_trfer.tag_map.begin()->second.first.trfer.begin();
      for(;t1!=inbound_trfer_grp_out.trfer.end() && t2!=inbound_trfer.tag_map.begin()->second.first.trfer.end(); ++t1,++t2)
        //楫��⭮��� ������� �஢�ਫ� � GetNewGrpTags
        inbound_trfer_route.push_back(t1->second); //�����, �� ��६ �� �᭮�� inbound_trfer_grp_out � ���������묨 �������ᠬ� ���ᠦ�஢
    }
    else
    {
      map<int, CheckIn::TTransferItem>::const_iterator t1=inbound_trfer_grp_out.trfer.begin();
      for(;t1!=inbound_trfer_grp_out.trfer.end(); ++t1)
        //楫��⭮��� ������� �஢�ਫ� � GetNewGrpTags
        inbound_trfer_route.push_back(t1->second); //�����, �� ��६ �� �᭮�� inbound_trfer_grp_out � ���������묨 �������ᠬ� ���ᠦ�஢
    }
  }

  checkTransferRouteForErrors(segList, inbound_trfer_route, inbound_trfer);

  if (trferTruncated &&
      inbound_trfer.conflicts.empty() &&
      inbound_trfer.tag_map.empty())
    inbound_trfer.conflicts.insert(InboundTrfer::ConflictReason::OutRouteTruncated);
}

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
      }
      ++a;
      ++b;
    }

    if (bBag!=bagBefore.end())
    {
      if (bBag->second.is_trfer && !bBag->second.handmade &&
          !reqInfo->user.access.profiledRightPermitted(PointId_t(grp.point_dep), 347))
        throw UserException("MSG.NO_PERM_MODIFY_INBOUND_TRFER");
    }
    if (aBag!=bagAfter.end())
    {
      aBag->second.check(lists);
      if (check_bag_weight)
        CheckBagWeightControl(aBag->second);
    }
  }
}

//�� ��楤�� ���⠢��� pax_id
void CheckServicePayment(int grp_id,
                         const CheckIn::TServicePaymentList &prior_payment,
                         boost::optional< CheckIn::TServicePaymentList > &curr_payment)
{
  if (!curr_payment) return;

  if (TReqInfo::Instance()->client_type==ctTerm &&
      !TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    CheckIn::TServicePaymentList prior_compatible, prior_not_compatible, prior_other_svc;
    prior_payment.getCompatibleWithPriorTermVersions(prior_compatible, prior_not_compatible, prior_other_svc);
    if (!prior_compatible.equal(curr_payment.get()))
    {
      //�뫨 ��������� �ਢ離� EMD � ��ண� �ନ����
      if (!prior_not_compatible.empty())
      {
        prior_compatible.dump("ServicePaymentFromXML: prior_compatible:");
        curr_payment.get().dump("ServicePaymentFromXML: payment:");
        throw UserException("MSG.TERM_VERSION.EMD_CHANGES_NOT_SUPPORTED");
      };

      curr_payment.get().insert(curr_payment.get().end(), prior_other_svc.begin(), prior_other_svc.end());
    }
    else curr_payment=boost::none;
  };

  if (!curr_payment) return;

  DB::TQuery Qry(PaxASVCList::GetSession(PaxASVCList::oneWithTknByGrpId), STDLOG);
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
      TPaxEMDItem asvc;
      asvc.fromDB(Qry);
      item.pax_id=asvc.pax_id;
      if (item.trfer_num!=asvc.trfer_num)
        throw UserException("MSG.EMD_WRONG_ATTACHMENT_DIFFERENT_SEG",
                            LParams() << LParam("emd", item.no_str()));
      if (item.pc && item.pc->RFISC!=asvc.RFISC)
        throw UserException("MSG.EMD_WRONG_ATTACHMENT_DIFFERENT_RFISC",
                            LParams() << LParam("emd", item.no_str()));
    }
  }
}

#ifdef XP_TESTING
static int LastGeneratedPaxId = ASTRA::NoExists;

int lastGeneratedPaxId()              { return LastGeneratedPaxId;  }
void setLastGeneratedPaxId(int lgpid) { LastGeneratedPaxId = lgpid; }
#endif/*XP_TESTING*/

static bool rollbackBeforeSvcAvailability(const CheckIn::TAfterSaveInfo& info,
                                          const xmlNodePtr& externalSysResNode,
                                          const TTripSetList& setList,
                                          const CheckIn::TSimplePaxGrpItem& grp,
                                          const bool& isNewCheckIn);


static void doCheckGrp(int grp_id)
{
    checkGroupUnification(GrpId_t(grp_id));
}

static bool saveCrsPaxRefuse(PaxId_t pax_id, const std::string& client_type)
{
  CheckIn::TSimplePaxItem crsPax;
  const bool crsPaxFound = crsPax.getCrsByPaxId(pax_id);
  if (not crsPaxFound) {
    return false;
  }
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id
                   << ", client_type=" << client_type;

  auto cur = make_db_curs(
        "INSERT INTO crs_pax_refuse( "
        "pax_id, client_type, time "
        ") VALUES ( "
        ":pax_id, :client_type, :time "
        ") ",
        PgOra::getRWSession("CRS_PAX_REFUSE"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .bind(":client_type", client_type)
      .bind(":time", DateTimeToBoost(NowUTC()))
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

std::optional<bool> getTripSetsPrTranzReg(const int point_id)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT pr_tranz_reg "
       "FROM trip_sets "
       "WHERE point_id = :point_id",
        PgOra::getROSession("TRIP_SETS")
    );

    int pr_tranz_reg;

    cur.stb()
       .defNull(pr_tranz_reg, ASTRA::NoExists)
       .bind(":point_id", point_id)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        return std::nullopt;
    }

    return dbo::isNotNull(pr_tranz_reg) && pr_tranz_reg;
}

std::optional<bool> getHalls2PrVip(const int hall)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT pr_vip "
       "FROM halls2 "
       "WHERE id = :hall",
        PgOra::getROSession("HALLS2")
    );

    int pr_vip;

    cur.stb()
       .def(pr_vip)
       .bind(":hall", hall)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        return std::nullopt;
    }

    return pr_vip;
}

//��楤�� ������ �������� true ⮫쪮 � ⮬ ��砥 �᫨ �ந������� ॠ�쭠� ॣ������
bool CheckInInterface::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode,
                               TChangeStatusList &ChangeStatusInfo,
                               SirenaExchange::TLastExchangeList &SirenaExchangeList,
                               CheckIn::TAfterSaveInfoList &AfterSaveInfoList,
                               bool& httpWasSent)
{
  LogTrace(TRACE6) << "CheckInInterface::" << __func__;
  Timing::Points timing("Timing::SavePax");

  timing.start("SavePax");

  timing.start("BeforeLock");

  AfterSaveInfoList.push_back(CheckIn::TAfterSaveInfo());
  CheckIn::TAfterSaveInfo &AfterSaveInfo=AfterSaveInfoList.back();

  TReqInfo *reqInfo = TReqInfo::Instance();

  CheckIn::SegmentMap segs(reqNode);
  set<PointId_t> pointIds=segs.getPointIds();

  if (reqInfo->client_type == ctPNL && pointIds.size()>1)
    //��� ctPNL ������� ᪢����� ॣ����樨!
    throw EXCEPTIONS::Exception("SavePax: through check-in not supported for ctPNL");


  timing.finish("BeforeLock");

  TFlights flightsForLock;
  flightsForLock.Get( pointIds, ftTranzit );
  flightsForLock.Lock(__FUNCTION__);

  timing.start("AfterLock");

  boost::optional<CheckIn::Segment> firstSegmentWithTransitBortChanging;
  for(segs.setFirstGrp(); !segs.noMoreGrp(); segs.setNextGrp())
  {
    CheckIn::Segment segment=CheckIn::Segment::fromDB(segs.grpNode());
    if (ediResNode==nullptr && !segs.isTransitGrp())
    {
      if (segment.addTransitLegsIfNeeded(segs.grpNode()))
      {
        if (!firstSegmentWithTransitBortChanging)
          firstSegmentWithTransitBortChanging=boost::in_place(segment);
      }
    }
    segs.add(segment);
  }

  //savepoint ��� �⪠� �� �ॢ�襭�� ����㧪� (����� �� ��᫥ ��窨)
  DB::Savepoint sp("sp_checkin");

  bool pr_unaccomp=strcmp((const char*)reqNode->name, "TCkinSaveUnaccompBag") == 0;

  TSegList segList;
  bool new_checkin=false;
  bool save_trfer=false;
  bool need_apps=false;
  CheckIn::TTransferList trfer;
  for(segs.setFirstGrp(); !segs.noMoreGrp(); segs.setNextGrp())
  {
    segList.push_back( TSegListItem() );
    CheckIn::TPaxGrpItem &grp=segList.back().grp;
    CheckIn::TPaxList &paxs=segList.back().paxs;

    if (!grp.fromXML(segs.grpNode()))
      throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

    if (segs.isFirstGrp())
    {
      grp.fromXMLadditional(reqNode, segs.grpNode(), pr_unaccomp);
      //��।���� - ����� ॣ������ ��� ������ ���������
      new_checkin=(grp.id==NoExists);
    }
    else
    {
      if (new_checkin!=(grp.id==NoExists))
        throw EXCEPTIONS::Exception("CheckInInterface::SavePax: impossible to determine 'new_checkin'");
    }
    if (!segs.isFirstGrp() && grp.status!=psTransit && grp.status!=psCrew)
      grp.status=psTCheckin;
    grp.hall=segList.front().grp.hall;
    grp.bag_refuse=segList.front().grp.bag_refuse;

    if (!pr_unaccomp)
    {
      xmlNodePtr paxNode=NodeAsNode("passengers",segs.grpNode())->children;
      for(; paxNode!=NULL; paxNode=paxNode->next)
        paxs.push_back(CheckIn::TPaxListItem().fromXML(paxNode, grp.trfer_confirm));
    }

    const CheckIn::Segment& segment=segs.get(PointId_t(grp.point_dep), __func__);

    if (reqInfo->client_type == ctPNL)
    {
      TTripStages tripStages( grp.point_dep );
      TStage ckin_stage =  tripStages.getStage( stCheckIn );
      if (ckin_stage!=sNoActive &&
            ckin_stage!=sPrepCheckIn &&
            ckin_stage!=sOpenCheckIn)
        throw UserException("MSG.REGISTRATION_CLOSED");
    }

    if (segs.isFirstGrp())
    {
      //����稬 �࠭���
      if (new_checkin)
        save_trfer=true;
      else
        save_trfer=GetNode("transfer",reqNode)!=NULL;

      if (save_trfer)
      {
        const CheckIn::Segment& segment=segs.get(PointId_t(grp.point_dep), __func__);
        trfer.parseSegments(GetNode("transfer",reqNode),
                            segment.airpArv(),
                            segment.scdOutLocal());

        if (!pr_unaccomp)
          trfer.parseSubclasses(NodeAsNode("passengers",segs.grpNode()));
      }
    }

    segList.back().flt=segment.flt;
    if (new_checkin)
      segList.back().setCabinClassAndSubclass(segs.isTransitGrp());

    if (!pr_unaccomp && APPS::checkAPPSSets(PointId_t(segList.back().grp.point_dep),
                                            PointId_t(segList.back().grp.point_arv)))
        need_apps = true;
  }

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
            }
        }
      }
      if (crewRemExists)
      {
        if (!allPaxNorec)
          throw UserException("MSG.CREW.CANT_CONSIST_OF_BOOKED_PASSENGERS");
        segList.begin()->grp.status=psCrew;
      }
    }
    if (segList.begin()->grp.status==psCrew)
    {
      segList.begin()->grp.cl.clear();
      for(CheckIn::TPaxList::iterator p=segList.begin()->paxs.begin(); p!=segList.begin()->paxs.end(); ++p)
      {
        if ((new_checkin && (p->pax.pers_type!=adult || p->pax.seats!=1)) ||
            (!new_checkin && p->pax.PaxUpdatesPending && p->pax.pers_type!=adult))
          throw UserException("MSG.CREW.MEMBER_IS_ADULT_WITH_ONE_SEAT");
        p->pax.subcl.clear();
        p->pax.cabin.clear();
        p->pax.tkn.clear();
      }

      if (segList.begin()->grp.group_bag)
      {
        //�஢�ਬ �⮡� ��� ����� �� ॣ����஢���� ����� � ��������
          bool pr_bag_ckin_crew = GetTripSets(tsBaggageCheckInCrew, segList.begin()->flt);
        for(CheckIn::TBagMap::const_iterator b=segList.begin()->grp.group_bag.get().bags.begin();
                                             b!=segList.begin()->grp.group_bag.get().bags.end();++b)
          if (not pr_bag_ckin_crew and !b->second.pr_cabin)
            throw UserException("MSG.CREW.CAN_CHECKIN_ONLY_CABIN_BAGGAGE");
      }
    }

    if (segList.size()>1 && segList.begin()->grp.status==psCrew)
    {
      if (pointIds.size()==1 && firstSegmentWithTransitBortChanging)
      {
        const TAdvTripInfo& flt=firstSegmentWithTransitBortChanging.get().flt;
        throw UserException("MSG.CREW_CKIN_DENIAL_DUE_TO_TRANSIT_BORT_CHANGING",
                            LParams()<<LParam("flight",GetTripName(flt,ecCkin,true,false)));
      }
      else
        throw UserException("MSG.CREW.THROUGH_CHECKIN_NOT_PERFORMED");
    }
  }

  if (pr_unaccomp && segList.size()>1)
  {
    if (pointIds.size()==1 && firstSegmentWithTransitBortChanging)
    {
      const TAdvTripInfo& flt=firstSegmentWithTransitBortChanging.get().flt;
      throw UserException("MSG.UNACCOMP_BAGGAGE_CKIN_DENIAL_DUE_TO_TRANSIT_BORT_CHANGING",
                          LParams()<<LParam("flight",GetTripName(flt,ecCkin,true,false)));
    }
    else
      throw UserException("MSG.UNACCOMP_BAGGAGE_THROUGH_CHECKIN_NOT_PERFORMED");
  }

  bool trfer_confirm=reqInfo->client_type==ctTerm;
  bool inbound_confirm=false;

  //�஢�ਬ �室�騩 �࠭��� ⮫쪮 �� ��ࢮ��砫쭮� ॣ����樨
  InboundTrfer::ConflictReasons trferConflicts;
  CheckIn::TGroupBagItem inbound_group_bag;

  if (!segList.empty() && new_checkin &&
      reqInfo->isSelfCkinClientType() &&
      segList.begin()->grp.status!=psCrew)
  {
    InboundTrfer::TNewGrpInfo info;
    CheckIn::TTransferList inbound_trfer_route;
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
    }

    trferConflicts.set(info);
    if (save_trfer && info.conflicts.empty())
    {
      //�ਢ離� �室�饣� �࠭��� ��� ��� � ���� ॣ����樨
      trfer=inbound_trfer_route;
      trfer_confirm=true;
      inbound_confirm=true;
    }
  }

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
        }

        try
        {
          GetInboundGroupBag(filtered_info,
                             true,
                             segList,
                             inbound_group_bag);
        }
        catch(const UserException &e)
        {
          AstraLocale::showErrorMessage(e.getLexemaData());
          throw UserException2();
        }
      }
      catch(const UserException2&)
      {
        XMLRequestCtxt *xmlRC = getXmlCtxt();
        if (xmlRC->resDoc==NULL) throw EXCEPTIONS::Exception("CheckInInterface::SavePax: xmlRC->resDoc=NULL;");
        xmlNodePtr resNode = NodeAsNode("/term/answer", xmlRC->resDoc);

        vector<TrferList::TGrpViewItem> grps;
        NewGrpInfoToGrpsView(info, grps);
        sort(grps.begin(), grps.end(), CompareGrpViewItem);
        TrferToXML(TrferList::trferOutForCkin,
                   grps,
                   NewTextChild(resNode, "inbound_confirmation"));
        throw; //�� �����, �⮡� � ����� ��஭� ������ �⪠� �� � � ��㣮� ��஭� �� ������ � xml inbound_confirmation
      }
    }
    inbound_confirm=true;
  }

  if (!inbound_group_bag.empty())
  {
    for(CheckIn::TBagMap::iterator b=inbound_group_bag.bags.begin();
                                   b!=inbound_group_bag.bags.end(); ++b)
    {
      b->second.to_ramp=false;
      b->second.is_trfer=true;
      b->second.handmade=false;
    }
    for(CheckIn::TTagMap::iterator t=inbound_group_bag.tags.begin();
                                   t!=inbound_group_bag.tags.end(); ++t)
    {
      if (t->second.printable) t->second.pr_print=true;
    }
  }

  timing.finish("AfterLock");

  if ((pointIds.size()>1 || !trfer.empty() || reqInfo->isSelfCkinClientType()) &&
      firstSegmentWithTransitBortChanging)
  {
    const TAdvTripInfo& flt=firstSegmentWithTransitBortChanging.get().flt;
    if (reqInfo->isSelfCkinClientType())
      throw UserException("MSG.SELF_CKIN_DENIAL_DUE_TO_TRANSIT_BORT_CHANGING",
                          LParams()<<LParam("flight",GetTripName(flt,ecCkin,true,false)));

     else
      throw UserException("MSG.TCKIN_AND_TRFER_DENIAL_DUE_TO_TRANSIT_BORT_CHANGING",
                          LParams()<<LParam("flight",GetTripName(flt,ecCkin,true,false)));
  }

  CheckIn::TTransferList::const_iterator iTrfer;
  map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> > trfer_segs;
  bool rollbackGuaranteedOnFirstSegment=false;
  APPS::AppsCollector    appsCollector;
  ModifiedPaxRem modifiedPaxRem(paxCheckIn);
  ModifiedPax newOrCancelledPax(paxCheckIn);
  std::list<TCkinRouteInsertItem> tckinGroupsWhenNewCheckIn;

  vector<int> segs_grp_ids;
  segs.setFirstGrp();
  for(TSegList::iterator iSegListItem=segList.begin();
      !segs.noMoreGrp() && iSegListItem!=segList.end();
      segs.setNextGrp(), ++iSegListItem)
  {
    CheckIn::TPaxGrpItem &grp=iSegListItem->grp;
    CheckIn::TPaxList &paxs=iSegListItem->paxs;
    CheckIn::TSimplePaxList priorSimplePaxList, currSimplePaxList;

    timing.start("annul_bt_before", grp.point_dep);

    TAnnulBT annul_bt;
    annul_bt.get(grp.id);

    timing.finish("annul_bt_before", grp.point_dep);

    timing.start("BeforeCheck", grp.point_dep);

    const CheckIn::Segment& segment=segs.get(PointId_t(grp.point_dep), __func__);

    const TAdvTripInfo &fltInfo=segment.flt;
    boost::optional<TFlightRbd> flightRbd;

    if (segs.isFirstGrp() &&
        grp.status!=psCrew &&
        reqInfo->client_type==ctTerm)
      AfterSaveInfo.agent_stat_period=NodeAsInteger("agent_stat_period",reqNode);
    AfterSaveInfo.segs.push_back(CheckIn::TAfterSaveSegInfo());
    AfterSaveInfo.segs.back().point_dep=grp.point_dep;
    TGrpToLogInfo &grpInfoBefore=AfterSaveInfo.segs.back().grpInfoBefore;
    TGrpToLogInfo &grpInfoAfter=AfterSaveInfo.segs.back().grpInfoAfter;
    CheckIn::TGrpEMDProps &handmadeEMDDiff=AfterSaveInfo.handmadeEMDDiff;
    TGrpMktFlight grpMktFlight=fltInfo.grpMktFlight();

    if ( need_apps ) {
      TAdvTripRoute route;
      route.GetRouteAfter(NoExists, grp.point_dep, trtWithCurrent, trtNotCancelled);
      if ( route.front().act_out != NoExists )
        throw UserException( "MSG.PASSENGER.CHANGES_NOT_PERMITTED" );
    }

    timing.finish("BeforeCheck", grp.point_dep);

    try
    {
      timing.start("Check", grp.point_dep);

      //BSM
      BSM::TBSMAddrs BSMaddrs;
      BSM::TTlgContent BSMContentBefore;
      bool BSMsend=grp.status==psCrew?false:
                                      BSM::IsSend(fltInfo, BSMaddrs, false);

      set<int> nextTrferSegs;
      bool pr_tranz_reg;

      if (std::optional<bool> pr_tranz_reg_opt = getTripSetsPrTranzReg(grp.point_dep)) {
          pr_tranz_reg = pr_tranz_reg_opt.value();
      } else {
          throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB
      }

      TTripSetList setList;
      setList.fromDB(grp.point_dep);
      if (setList.empty())
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB

      AstraEdifact::TFltParams ediFltParams;
      if (!ediFltParams.get(grp.point_dep))
        throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA"); //WEB

      bool pr_mintrans_file=GetTripSets(tsMintransFile,fltInfo);

      bool addVIP=false;
      if (segs.isFirstGrp())
      {
        if (grp.status==psTransit && !pr_tranz_reg)
          throw UserException("MSG.CHECKIN.NOT_RECHECKIN_MODE_FOR_TRANZIT");

        if (reqInfo->client_type == ctTerm)
        {
          std::optional<bool> addVip_opt = getHalls2PrVip(grp.hall);
          if (addVip_opt) {
            addVIP = addVip_opt.value();
          } else {
            throw UserException("MSG.CHECKIN.INVALID_HALL");
          }
        }
      }

      TCompleteAPICheckInfo checkInfo;
      if (!pr_unaccomp)
        checkInfo.set(grp.getSegmentPair());

      //�஢�ਬ ����� ���㬥�⮢ � ����⮢, ६�ન
      if (!pr_unaccomp)
      {
        boost::optional<TRemGrp> forbiddenRemGrp;
        for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p)
        {
          CheckIn::TPaxItem &pax=p->pax;
          try
          {
            CheckIn::TSimplePaxItem priorPax;
            if (!new_checkin)
            {
              if (!priorPax.getByPaxId(pax.id))
                throw UserException("MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
              pax.seats=priorPax.seats;
              pax.crew_type=priorPax.crew_type;
              pax.is_jmp=priorPax.is_jmp;
              pax.cabin=priorPax.cabin;
              pax.reg_no=priorPax.reg_no;
              if (!new_checkin && !pax.PaxUpdatesPending)
              {
                pax.pers_type=priorPax.pers_type;
                pax.refuse=priorPax.refuse;
                pax.subcl=priorPax.subcl;
              }
              priorSimplePaxList.push_back(priorPax);
            }
            else
            {
              if (grp.status!=psCrew)
              {
                if (!flightRbd)
                  flightRbd=boost::in_place(fltInfo);
                CheckIn::setComplexClassGrp(flightRbd.get(), pax.cabin);
              }
            }

            if (new_checkin && grp.status!=psCrew)
            {
              if (pax.id!=NoExists && checkInfo.pnrAddrRequired())
              {
                if (TPnrAddrs().getByPaxIdFast(pax.id).empty())
                  throw UserException("MSG.CHECKIN.PNR_ADDR_REQUIRED");
              }
              if (pax.id==NoExists && checkInfo.norecNotAllowed())
                throw UserException("MSG.CHECKIN.NOREC_NOT_ALLOWED");
            }




            if (pax.name.empty() && pr_mintrans_file)
              throw UserException("MSG.CHECKIN.PASSENGERS_NAMES_NOT_SET");

            if (pax.tkn.rem=="TKNE" && !ediFltParams.ets_no_exchange &&
                (ediFltParams.control_method || ediFltParams.ets_exchange_status!=ETSExchangeStatus::NotConnected)) //�� �� � ���� ����� � ��� (���� ��� � ��� ���� ����஫�� ��⮤)
            {
              if (!pax.tkn.equalAttrs(priorPax.tkn))
              {
                //��ࢮ��砫쭠� ॣ������ ��� ����� �⫨砥��� �� ࠭�� ����ᠭ����
                if (pax.tkn.no.empty())
                  throw UserException("MSG.ETICK.NUMBER_NOT_SET");
                if (pax.tkn.coupon==NoExists)
                  throw UserException("MSG.ETICK.COUPON_NOT_SET", LParams()<<LParam("etick", pax.tkn.no ) );

                if (ediFltParams.control_method && ediFltParams.in_final_status) //�� ������塞 ������ ॣ����஢��� �᫨ ��室���� � 䨭��쭮� ����� �� ����஫쭮� ��⮤�
                  throw UserException("MSG.ETICK.FLIGHT_IN_FINAL_STATUS_CHECKIN_DENIAL",
                                      LParams()<<LParam("etick", pax.tkn.no_str()));

                TETickItem etick;
                etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
                if (etick.empty())
                  throw UserException("MSG.ETICK.NEED_DISPLAY", LParams()<<LParam("etick", pax.tkn.no_str()));

                if (ediFltParams.control_method &&
                    !Ticketing::existsAirportControl(pax.tkn.no,
                                                     pax.tkn.coupon,
                                                     false))
                  throw UserException("MSG.ETICK.NEED_DISPLAY", LParams()<<LParam("etick", pax.tkn.no_str()));
              }
            }
            //�஢�ઠ refusability ��� ���᪮� � ����, � ⥯��� � ��� �ନ����
            if (!new_checkin && pax.PaxUpdatesPending && !pax.refuse.empty())
            {
              if (priorPax.refuse.empty())
                newOrCancelledPax.add(PaxChanges::Cancel, PaxIdWithSegmentPair(PaxId_t(pax.id), grp.getSegmentPair()));
              boost::optional<std::string> unreg_denial = CheckRefusability(fltInfo, pax.id);
              if (unreg_denial) throw UserException(*unreg_denial);
            }

            //�����
            if (pax.TknExists)
            {
              if (!new_checkin) //pax.id ���樠����஢�� ⮫쪮 � �⮬ ��砥, � �� ����� ��� modifiedPaxRem.add
              {
                if (!pax.tkn.equalAttrs(priorPax.tkn))
                  modifiedPaxRem.add(remTKN, PaxIdWithSegmentPair(PaxId_t(pax.id), grp.getSegmentPair()));
              }

              if (setList.value<bool>(tsRegWithoutTKNA) &&
                  pax.tkn.rem!="TKNE" &&
                  !pax.tkn.equalAttrs(priorPax.tkn))
                throw UserException("MSG.CHECKIN.TKNA_DENIAL");

              if (pax.tkn.no.size()>15)
              {
                string ticket_no=pax.tkn.no;
                if (ticket_no.size()>20) ticket_no.erase(20).append("...");
                throw UserException("MSG.CHECKIN.TICKET_LARGE_MAX_LEN", LParams()<<LParam("ticket_no",ticket_no));
              }
              if (pax.refuse.empty())
              {
                if (checkInfo.incomplete(pax.tkn, grp.status, pax.crew_type))
                  throw UserException("MSG.CHECKIN.PASSENGERS_TICKETS_NOT_SET"); //WEB
              }

              //�஢��塞 䠬���� � �����
              if (!pax.tkn.no.empty() && !pax.surname.empty() && pax.api_doc_applied())
              {
                list<TETickItem> eticks = TETickItem::fromDB(pax.tkn.no, TETickItem::Display);
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
              }
            }

            if (new_checkin || p->remsExists)
            {
              multiset<CheckIn::TPaxRemItem> &rems=p->rems;

              if (new_checkin && forbiddenRemExists(fltInfo, rems, forbiddenRemGrp))
                throw UserException("MSG.PASSENGER.CHECKIN_DENIAL");

              bool flagVIP=false,
                   flagSTCR=false,
                   flagEXST=false;
              for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin(); r!=rems.end();)
              {
                TRemCategory cat=getRemCategory(*r);

                if (r->code=="VIP")  flagVIP=true;
                if (r->code=="STCR") flagSTCR=true;
                if (r->code=="EXST") flagEXST=true;

                VerifyAPPSOverrideRem(*r);

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
              CheckIn::PaxRemAndASVCFromDB(pax.id, new_checkin, boost::none, prior_rems);
              multiset<CheckIn::TPaxRemItem> added;
              multiset<CheckIn::TPaxRemItem> deleted;
              CheckIn::GetPaxRemDifference(boost::none, prior_rems, rems, added, deleted);
              for(int pass=0; pass<2; pass++)
              {
                for(multiset<CheckIn::TPaxRemItem>::const_iterator r=(pass==0?added:deleted).begin();
                                                                   r!=(pass==0?added:deleted).end(); ++r)
                {
                  if (r->code.empty()) continue;
                  TRemCategory cat=getRemCategory(*r);
                  if (cat==remASVC) continue; //�ய�᪠�� ASVC
                  if (isReadonlyRemCategory(cat) || isReadonlyRem(*r)) {
                    if(!inTestMode()) {
                        throw UserException(pass==0?"MSG.REMARK.ADD_OR_CHANGE_DENIAL":"MSG.REMARK.CHANGE_OR_DEL_DENIAL",
                                        LParams() << LParam("remark", r->code.empty()?r->text.substr(0,5):r->code));
                    }
                  }
                }
              }

              p->checkImportantRems(new_checkin, grp.status);

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
                }
            }
            if (p->pax.crew_type!=TCrewType::Unknown)
            {
              if ((new_checkin && (p->pax.pers_type!=adult || p->pax.seats<1)) ||
                  (!new_checkin && p->pax.PaxUpdatesPending && p->pax.pers_type!=adult))
                throw UserException("MSG.PASSENGER.IS_ADULT_WITH_AT_LEAST_ONE_SEAT");
            };

            TDateTime checkDate=UTCToLocal(NowUTC(), AirpTZRegion(grp.airp_dep));
            //���㬥��
            if (pax.DocExists)
              HandleDoc(grp, pax, checkInfo, checkDate, pax.doc);
            //����
            if (pax.DocoExists)
              HandleDoco(grp, pax, checkInfo, checkDate, pax.doco);

            if (pax.DocaExists)
              HandleDoca(grp, pax, checkInfo, pax.doca_map);

            currSimplePaxList.push_back(pax);

          }
          catch(const CheckIn::UserException&)
          {
            throw;
          }
          catch(const UserException &e)
          {
            if (pax.id!=NoExists)
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            else
              throw;
          }
        }
      }

      if (reqInfo->client_type == ctTerm && grp.hall==NoExists)
        throw EXCEPTIONS::Exception("CheckInInterface::SavePax: grp.hall not defined");

      //�࠭���
      if (segs.isFirstGrp())
      {
        if (save_trfer)
        {
          if (!trfer.empty())
          {
            map<int, CheckIn::TTransferItem> trfer_tmp;
            int trfer_num=1;
            for(CheckIn::TTransferList::const_iterator t=trfer.begin();t!=trfer.end();++t, trfer_num++)
              trfer_tmp[trfer_num]=*t;
            traceTrfer(TRACE5, "SavePax: trfer_tmp", trfer_tmp);
            GetTrferSets(fltInfo, grp.airp_arv, "", trfer_tmp, true, trfer_segs);
            traceTrfer(TRACE5, "SavePax: trfer_segs", trfer_segs);
          }
        }
        else
        {
          //�������� trfer �� ����
          trfer.load(grp.id);
        }
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
                if (iPaxTrfer->subclass!=pax.getCabinSubclass())
                  throw UserException("MSG.CHECKIN.DIFFERENT_TCKIN_AND_TRFER_SUBCLASSES");
              }
              catch(const CheckIn::UserException&)
              {
                throw;
              }
              catch(const UserException &e)
              {
                if (pax.id!=NoExists)
                  throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
                else
                  throw;
              }
            }
            if (p!=paxs.end() || iPaxTrfer!=iTrfer->pax.end())
              throw EXCEPTIONS::Exception("SavePax: Different number of transfer subclasses and passengers");
          }
          iTrfer++;
        }
      }

      if (!trfer.empty() && grp.status==psCrew)
        throw UserException("MSG.CREW.TRANSFER_CHECKIN_NOT_PERFORMED");

      bool needCheckUnattachedTrferAlarm=need_check_u_trfer_alarm_for_grp(grp.point_dep);
      map<InboundTrfer::TGrpId, InboundTrfer::TGrpItem> grpTagsBefore;
      CheckIn::TServicePaymentList paymentBefore;
      CheckIn::TServicePaymentListWithAuto paymentBeforeWithAuto;
      bool first_pax_on_flight = false;
      string wl_type;

      timing.finish("Check", grp.point_dep);

      if (new_checkin)
      {
        //����� ॣ������
        wl_type=NodeAsString("wl_type", segs.grpNode());
        int first_reg_no=NoExists;
        if (!pr_unaccomp)
        {
          timing.start("CheckCounters", grp.point_dep);

          //�஢�ઠ ���稪��
          if (wl_type.empty() && grp.status!=psCrew)
          {
            //������ ��饣� ���-�� ����, �ॡ㥬�� ��㯯�
            CheckIn::AvailableByClasses availableByClasses(paxs);

            CheckIn::CheckCounters(grp,
                                   setList.value<bool>(tsFreeSeating),
                                   availableByClasses);

            for(const auto& i : availableByClasses)
            {
              const CheckIn::AvailableByClass& item=i.second;
              if (item.avail!=NoExists && item.avail<item.need)
              {
                if (!item.is_jmp)
                  throw UserException("MSG.CHECKIN.AVAILABLE_SEATS",
                                      LParams() << LParam("count",item.avail)
                                                << LParam("class",ElemIdToCodeNative(etClass,item.cl)));
                else
                  throw UserException("MSG.CHECKIN.AVAILABLE_JUMP_SEATS",
                                      LParams() << LParam("count",item.avail));

              }
            }
            //availableByClasses.dump();

            if (setList.value<bool>(tsFreeSeating)) wl_type="F";
          }

          if (grp.status!=psCrew)
            //����襬 �������᪨� ३�
            CheckIn::grpMktFlightFromXML(GetNode("mark_flight", segs.grpNode()), grpMktFlight);

          timing.finish("CheckCounters", grp.point_dep);

          timing.start("RegNoGenerator", grp.point_dep);

          if (reqInfo->client_type!=ctPNL)
          {
            //����稬 ���� ॣ. �����
            boost::optional<CheckIn::RegNoRange> regNoRange=
              CheckIn::RegNoGenerator(grp.point_dep,
                                      grp.status!=psCrew?
                                        CheckIn::RegNoGenerator::Positive:
                                        CheckIn::RegNoGenerator::Negative).getRange(paxs.size(), CheckIn::RegNoGenerator::DefragAtLast);
            if (!regNoRange)
              throw UserException(paxs.size()>1?"MSG.REG_NUMBERS_ENDED_FOR_GROUP":
                                                "MSG.REG_NUMBERS_ENDED_FOR_PASSENGER");

            first_reg_no=regNoRange.get().first_no;
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
                if (regNoRange.get().first_no>0)
                  reg_no++;
                else
                  reg_no--;
              }
            }
          }
          else
          {
            if (grp.status!=psCrew)
            {
              DB::TQuery Qry(PgOra::getROSession({"PAX_GRP", "PAX"}), STDLOG);
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
                }
              }
              if (!checked_reg_no.empty() && !reg_no_with_seats.empty())
              {
                set<int>::const_iterator i=find_first_of(checked_reg_no.begin(), checked_reg_no.end(),
                                                         reg_no_with_seats.begin(), reg_no_with_seats.end());
                if (i!=checked_reg_no.end())
                {
                  ostringstream reg_no_str;
                  reg_no_str << setw(3) << setfill('0') << *i;
                  throw UserException("MSG.CHECKIN.DUPLICATE_REG_NO", LParams() << LParam("reg_no", reg_no_str.str()));
                }
              }
              if (!checked_reg_no.empty() && !reg_no_without_seats.empty())
              {
                set<int>::const_iterator i=find_first_of(checked_reg_no.begin(), checked_reg_no.end(),
                                                         reg_no_without_seats.begin(), reg_no_without_seats.end());
                if (i!=checked_reg_no.end())
                {
                  ostringstream reg_no_str;
                  reg_no_str << setw(3) << setfill('0') << *i;
                  throw UserException("MSG.CHECKIN.DUPLICATE_REG_NO", LParams() << LParam("reg_no", reg_no_str.str()));
                }
              }
            }
            else
              throw UserException("MSG.CREW.CANT_CONSIST_OF_BOOKED_PASSENGERS");
          }

          timing.finish("RegNoGenerator", grp.point_dep);
        } //if (!pr_unaccomp)


        timing.start("CheckInPassengers", grp.point_dep);

        TQuery Qry(&OraSession);
        Qry.SQLText=
          "DECLARE "
          "  pass BINARY_INTEGER; "
          "BEGIN "
          "  FOR pass IN 1..2 LOOP "
          "    BEGIN "
          "      SELECT point_id INTO :mark_point_id FROM mark_trips "
          "      WHERE scd=:mark_scd AND airline=:mark_airline AND flt_no=:mark_flt_no AND airp_dep=:mark_airp_dep AND "
          "            (suffix IS NULL AND :mark_suffix IS NULL OR suffix=:mark_suffix) FOR UPDATE; "
          "      EXIT; "
          "    EXCEPTION "
          "      WHEN NO_DATA_FOUND THEN "
          "        IF :mark_point_id IS NULL OR pass=2 THEN "
          "          SELECT cycle_id__seq.nextval INTO :mark_point_id FROM dual; "
          "        END IF; "
          "        BEGIN "
          "          INSERT INTO mark_trips(point_id,airline,flt_no,suffix,scd,airp_dep) "
          "          VALUES (:mark_point_id,:mark_airline,:mark_flt_no,:mark_suffix,:mark_scd,:mark_airp_dep); "
          "          EXIT; "
          "        EXCEPTION "
          "          WHEN DUP_VAL_ON_INDEX THEN "
          "            IF pass=1 THEN NULL; ELSE RAISE; END IF; "
          "        END; "
          "    END; "
          "  END LOOP; "
          "  IF :grp_id IS NULL THEN "
          "    SELECT pax_grp__seq.nextval INTO :grp_id FROM dual; "
          "  END IF; "
          "  INSERT INTO pax_grp(grp_id,point_dep,point_arv,airp_dep,airp_arv,class, "
          "    status,excess_wt,excess_pc,hall,bag_refuse,trfer_confirm,user_id,desk,time_create,client_type, "
          "    point_id_mark,pr_mark_norms,trfer_conflict,inbound_confirm,tid) "
          "  VALUES(:grp_id,:point_dep,:point_arv,:airp_dep,:airp_arv,:class, "
          "    :status,0,0,:hall,0,:trfer_confirm,:user_id,:desk,:time_create,:client_type, "
          "    :mark_point_id,:pr_mark_norms,:trfer_conflict,:inbound_confirm,cycle_tid__seq.nextval); "
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

        if (GetNode("generated_grp_id", segs.grpNode())!=NULL)
          Qry.SetVariable("grp_id",NodeAsInteger("generated_grp_id", segs.grpNode()));
        if (trfer_confirm) AfterSaveInfo.action=CheckIn::actionSvcAvailability;
        Qry.CreateVariable("trfer_confirm",otInteger,(int)trfer_confirm);
        Qry.CreateVariable("trfer_conflict",otInteger,(int)(trferConflicts.isInboundBaggageConflict() &&
                                                            reqInfo->client_type != ctTerm));  //�������� �ॢ��� ⮫쪮 ��� ���� � ���᪠ � ⮫쪮 �᫨ ���� �室�騩 �࠭���
        Qry.CreateVariable("inbound_confirm",otInteger,(int)inbound_confirm);
        if (reqInfo->client_type!=ctPNL && !reqInfo->api_mode)
          Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
        else
          Qry.CreateVariable("user_id",otInteger,FNull);
        Qry.CreateVariable("desk",otString,reqInfo->desk.code);
        Qry.CreateVariable("time_create",otDate,NowUTC());
        Qry.CreateVariable("client_type",otString,EncodeClientType(reqInfo->client_type));

        if (grpMktFlight.equalFlight(fltInfo.grpMktFlight()))
          Qry.CreateVariable("mark_point_id",otInteger,grp.point_dep);
        else
          Qry.CreateVariable("mark_point_id",otInteger,FNull);
        grpMktFlight.toDB(Qry);
        Qry.Execute();
        grp.id=Qry.GetVariableAsInteger("grp_id");

        boost::optional<RegNo_t> regNo = boost::make_optional(false, RegNo_t(0));
        if (first_reg_no!=NoExists) regNo=boost::in_place(first_reg_no);

        tckinGroupsWhenNewCheckIn.emplace_back(GrpId_t(grp.id), regNo, grp.status);

        ReplaceTextChild(segs.grpNode(), "generated_grp_id", grp.id);

        if (!pr_unaccomp)
        {
          bool pr_brd_with_reg=false,pr_exam_with_brd=false;
          if (segs.isFirstGrp() && reqInfo->client_type == ctTerm && grp.status!=psCrew)
          {
            //�� ᪢����� ॣ����樨 ᮢ���⭠� ॣ������ � ��ᠤ��� �.�. ⮫쪮 �� ��ࢮ� ३�
            //�� web-ॣ����樨 ��ᠤ�� ��ண� ࠧ���쭠�
            DB::TQuery Qry(PgOra::getROSession("TRIP_HALL"), STDLOG);
            Qry.SQLText=
              "SELECT pr_misc FROM trip_hall "
              "WHERE point_id=:point_id AND type=:type AND (hall=:hall OR hall IS NULL) "
              "ORDER BY CASE hall WHEN NULL THEN 1 ELSE 0 END";
            Qry.CreateVariable("point_id",otInteger,grp.point_dep);
            Qry.CreateVariable("hall",otInteger,grp.hall);
            Qry.DeclareVariable("type",otInteger);

            Qry.SetVariable("type",(int)tsBrdWithReg);
            Qry.Execute();
            if (!Qry.Eof) pr_brd_with_reg=Qry.FieldAsInteger("pr_misc")!=0;
            Qry.SetVariable("type",(int)tsExamWithBrd);
            Qry.Execute();
            if (!Qry.Eof) pr_exam_with_brd=Qry.FieldAsInteger("pr_misc")!=0;
          }

          if (segs.isTransitGrp())
            pr_brd_with_reg = setList.value<bool>(tsTransitBrdWithAutoreg);

          Qry.Clear();
          Qry.SQLText=
            "BEGIN "
            "  IF :pax_id IS NULL THEN "
            "    SELECT pax_id.nextval INTO :pax_id FROM dual; "
            "  END IF; "
            "  INSERT INTO pax(pax_id,grp_id,surname,name,pers_type,crew_type,is_jmp,is_female,seat_type,seats,pr_brd, "
            "                  wl_type,refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm,doco_confirm, "
            "                  pr_exam,subclass,cabin_subclass,cabin_class,cabin_class_grp,bag_pool_num,tid) "
            "  VALUES(:pax_id,:grp_id,:surname,:name,:pers_type,:crew_type,:is_jmp,:is_female,:seat_type,:seats,:pr_brd, "
            "         :wl_type,NULL,:reg_no,:ticket_no,:coupon_no,:ticket_rem,:ticket_confirm,0, "
            "         :pr_exam,:subclass,:cabin_subclass,:cabin_class,:cabin_class_grp,:bag_pool_num,cycle_tid__seq.currval); "
            "END;";
          Qry.DeclareVariable("pax_id",otInteger);
          Qry.DeclareVariable("grp_id",otInteger);
          Qry.DeclareVariable("surname",otString);
          Qry.DeclareVariable("name",otString);
          Qry.DeclareVariable("pers_type",otString);
          Qry.DeclareVariable("crew_type",otString);
          Qry.DeclareVariable("is_jmp",otInteger);
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
          Qry.DeclareVariable("cabin_subclass",otString);
          Qry.DeclareVariable("cabin_class",otString);
          Qry.DeclareVariable("cabin_class_grp",otInteger);
          Qry.DeclareVariable("bag_pool_num",otInteger);
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
                }
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
                }
                Qry.SetVariable("grp_id", grp.id);

                try
                {
                  Qry.Execute();
                }
                catch(const EOracleError& E)
                {
                  if (E.Code==1)
                    throw UserException("MSG.PASSENGER.CHECKED.ALREADY_OTHER_DESK",
                                        LParams()<<LParam("surname",pax.full_name())); //WEB
                  else
                    throw;
                }
                PaxIdWithSegmentPair paxId(PaxId_t(Qry.GetVariableAsInteger("pax_id")), grp.getSegmentPair());
                newOrCancelledPax.add(PaxChanges::New, paxId);
                ReplaceTextChild(p->node,"generated_pax_id",paxId().get());
#ifdef XP_TESTING
                if(inTestMode()) {
                    setLastGeneratedPaxId(paxId().get());
                }
#endif/*XP_TESTING*/
                if (pax.id==NoExists) {
                    p->generatedPaxId=paxId(); //���������� ⮫쪮 �� ��ࢮ��砫쭮� ॣ����樨 (new_checkin) � ⮫쪮 ��� NOREC
                }

                //������ pax_doc
                if (pax.DocExists) CheckIn::SavePaxDoc(paxId, !new_checkin, pax.doc, modifiedPaxRem);
                if (pax.DocoExists) CheckIn::SavePaxDoco(paxId, !new_checkin, pax.doco, modifiedPaxRem);
                if (pax.DocaExists) CheckIn::SavePaxDoca(paxId, !new_checkin, pax.doca_map, modifiedPaxRem);

                if (save_trfer)
                {
                  //������ �������ᮢ �࠭���
                  CheckIn::PaxTransferToDB(paxId().get(), pax_no, trfer, segs.segNo());
                }
                //������ ६�ப
                if (p->remsExists)
                {
                  CheckIn::SavePaxRem(paxId, !new_checkin, p->rems, modifiedPaxRem);
                  CheckIn::SavePaxFQT(paxId, !new_checkin, p->fqts, modifiedPaxRem);
                }

                if (need_apps) {
                    // ��� ����� ���ᠦ�஢ ६�ન APPS �� �஢��塞
                    appsCollector.addPaxItem(paxId());
                }

                // ������ � pax_events
                if(pax.pr_brd)
                    TPaxEvent().toDB(paxId().get(), TPaxEventTypes::BRD);
              }
              catch(const CheckIn::UserException&)
              {
                throw;
              }
              catch(const UserException &e)
              {
                if (pax.id!=NoExists)
                  throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
                else
                  throw;
              }

            } // end for paxs
          } //end for k
          CheckIn::AddPaxASVC(grp.id, true); //ᨭ�஭����㥬 ASVC ⮫쪮 �� ��ࢮ� ॣ����樨
          grp.SyncServiceAuto(fltInfo);
        }

        TLogLocale tlocale;
        tlocale.ev_type=ASTRA::evtPax;
        tlocale.id1=grp.point_dep;
        tlocale.id2=NoExists;
        tlocale.id3=grp.id;
        if (save_trfer)
        {
          SaveTransfer(grp.id, trfer, trfer_segs, segs.segNo(), tlocale);
          if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        }
        std::vector<CheckIn::IatciSegment> iatciSegs = iatci::readIatciSegs(grp.id, ediResNode);
        SaveTCkinSegs(grp.id, segs, iatciSegs, tlocale);
        if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        trferConflicts.toLog(tlocale);

        timing.finish("CheckInPassengers", grp.point_dep);
      } //new_checkin
      else
      {
        timing.start("ChangePassengers", grp.point_dep);

        if (reqInfo->client_type == ctPNL)
          throw EXCEPTIONS::Exception("SavePax: changes not supported for ctPNL");

        //������ ���������
        GetGrpToLogInfo(grp.id, grpInfoBefore); //��� ��� ᥣ���⮢
        if (needCheckUnattachedTrferAlarm)
          InboundTrfer::GetCheckedTags(grp.id, idGrp, grpTagsBefore); //��� ��� ᥣ���⮢
        paymentBefore.fromDB(GrpId_t(grp.id));
        paymentBeforeWithAuto.fromDB(grp.id);
        //BSM
        if (BSMsend)
          BSM::LoadContent(grp.id,true,BSMContentBefore);

        InboundTrfer::GetNextTrferCheckedFlts(grp.id, idGrp, nextTrferSegs);

        bool save_trfer=GetNode("transfer",reqNode)!=NULL;

        if (segs.isFirstGrp())
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
          }
        }

        TQuery Qry(&OraSession);
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
          if (!grpInfoBefore.trfer_confirm) AfterSaveInfo.action=CheckIn::actionSvcAvailability;
          Qry.CreateVariable("trfer_confirm",otInteger,(int)true);
          Qry.CreateVariable("trfer_conflict",otInteger,(int)false);
        }
        else
        {
          Qry.CreateVariable("trfer_confirm",otInteger,FNull);
          Qry.CreateVariable("trfer_conflict",otInteger,FNull);
        }
        if (inbound_confirm)
          Qry.CreateVariable("inbound_confirm",otInteger,(int)true);
        else
          Qry.CreateVariable("inbound_confirm",otInteger,FNull);

        Qry.Execute();
        if (Qry.RowsProcessed()<=0)
          throw UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //WEB

        //�⠥� �������᪨� ३�
        if (!grpMktFlight.getByGrpId(grp.id))
          throw Exception("%s: !grpMktFlight.getByGrpId(%d)", __func__, grp.id);


        if (!pr_unaccomp)
        {
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
                   "    subclass=:subclass, ";
            PaxQry.DeclareVariable("surname",otString);
            PaxQry.DeclareVariable("name",otString);
            PaxQry.DeclareVariable("pers_type",otString);
            PaxQry.DeclareVariable("ticket_no",otString);
            PaxQry.DeclareVariable("coupon_no",otInteger);
            PaxQry.DeclareVariable("ticket_rem",otString);
            PaxQry.DeclareVariable("ticket_confirm",otInteger);
            PaxQry.DeclareVariable("subclass",otString);
          }

          if (reqInfo->client_type==ctTerm ||
              isTagConfirmRequestSBDO(reqNode) ||
              isTagRevokeRequestSBDO(reqNode))
          {
            sql << "    bag_pool_num=:bag_pool_num, ";
            PaxQry.DeclareVariable("bag_pool_num",otInteger);
          }

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
          for(CheckIn::TPaxList::iterator p=paxs.begin(); p!=paxs.end(); ++p,pax_no++)
          {
            const CheckIn::TPaxItem &pax=p->pax;
            PaxIdWithSegmentPair paxId(PaxId_t(pax.id), grp.getSegmentPair());
            try
            {
              if (pax.PaxUpdatesPending)
              {
                if (!pax.refuse.empty())
                {
                  LayerQry.SetVariable("pax_id",pax.id);
                  LayerQry.Execute();
                  int tid = pax.tid;
                  DeleteTlgSeatRanges( {ASTRA::cltProtSelfCkin,
                                        ASTRA::cltProtBeforePay,
                                        ASTRA::cltProtAfterPay}, pax.id, tid);
                }
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
                if (pax.DocExists) CheckIn::SavePaxDoc(paxId, !new_checkin, pax.doc, modifiedPaxRem);
                if (pax.DocoExists) CheckIn::SavePaxDoco(paxId, !new_checkin, pax.doco, modifiedPaxRem);
                if (pax.DocaExists) CheckIn::SavePaxDoca(paxId, !new_checkin, pax.doca_map, modifiedPaxRem);

                if (reqInfo->client_type!=ctTerm && pax.refuse==refuseAgentError) //ctPNL???
                {
                  //��� � ���� ॣ������
                  saveCrsPaxRefuse(PaxId_t(pax.id), EncodeClientType(reqInfo->client_type));
                }
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
                }
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
              }

              if (save_trfer)
              {
                //������ �������ᮢ �࠭���
                CheckIn::PaxTransferToDB(pax.id, pax_no, trfer, segs.segNo());
              }
              //������ ६�ப
              if (p->remsExists)
              {
                CheckIn::SavePaxRem(paxId, !new_checkin, p->rems, modifiedPaxRem);
                CheckIn::SavePaxFQT(paxId, !new_checkin, p->fqts, modifiedPaxRem);
              }

              if (need_apps) {
                  std::string override;
                  bool is_forced = false;
                  HandleAPPSRems(p->rems, override, is_forced);
                  appsCollector.addPaxItem(PaxId_t(pax.id), override, is_forced);
              }
            }
            catch(const CheckIn::UserException&)
            {
              throw;
            }
            catch(const UserException &e)
            {
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            }
          }
        }

        if (save_trfer)
        {
          TLogLocale tlocale;
          tlocale.ev_type=ASTRA::evtPax;
          tlocale.id1=grp.point_dep;
          tlocale.id2=NoExists;
          tlocale.id3=grp.id;
          SaveTransfer(grp.id, trfer, trfer_segs, segs.segNo(), tlocale);
          if (!tlocale.lexema_id.empty()) reqInfo->LocaleToLog(tlocale);
        }

        timing.finish("ChangePassengers", grp.point_dep);
      }

      grp.checkInfantsCount(priorSimplePaxList, currSimplePaxList);

      timing.start("ChangeBaggage", grp.point_dep);

      bool unknown_concept=!inbound_group_bag.empty() ||
                           (reqInfo->client_type==ASTRA::ctTerm &&
                            AfterSaveInfo.action==CheckIn::actionSvcAvailability && setList.value<bool>(tsPieceConcept));

      AfterSaveInfo.segs.back().grp_id=grp.id;

      if (segs.isFirstGrp())
      {
        if (unknown_concept)
        {
          if (!inbound_group_bag.empty())
          {
            inbound_group_bag.toDB(grp.id, grp.point_dep);
            if (grp.group_bag)
              showErrorMessage("MSG.CHECKIN.BAGGAGE_NOT_REGISTERED_DUE_INBOUND_TRFER");
          }
          else
          {
            //�� �����뢠�� ����� �� �⮬ �⠯�, ⠪ ��� �� �� ����� ��⥬� ����
            if (grp.group_bag)
              showErrorMessage("MSG.CHECKIN.BAGGAGE_NOT_REGISTERED_DUE_CHECKING_PIECE_CONCEPT");
          }
        }
        else
        {
          //����� ��⥬� ����
          if (grp.group_bag)
          {
            grp.group_bag.get().checkAndGenerateTags(grp.point_dep, grp.id);
            grp.group_bag.get().getAllListItems(grp.id, pr_unaccomp, 0);
            CheckBagChanges(grpInfoBefore, grp);
            grp.group_bag.get().toDB(grp.id, grp.point_dep);
          }

          CheckServicePayment(grp.id, paymentBefore, grp.payment);
          CheckIn::TGrpEMDProps emdProps;
          CheckIn::CalcGrpEMDProps<CheckIn::TServicePaymentList>(paymentBefore, grp.payment, handmadeEMDDiff, emdProps);
          CheckIn::TGrpEMDProps::toDB(grp.id, emdProps);

          if (grp.svc)
          {
            grp.svc.get().getAllListItems();
            grp.svc.get().toDB(GrpId_t(grp.id));
          }

          if (grp.svc_auto && !new_checkin)
          {
            TGrpServiceAutoList svcsAutoBefore;
            svcsAutoBefore.fromDB(GrpId_t(grp.id));
            grp.svc_auto.get().replaceWithoutEMDFrom(svcsAutoBefore); //ASVC ��� EMD 㤠������ ⮫쪮 �१ ADL, ������ �� ���� ����

            CheckIn::TGrpEMDProps handmadeAutoEMDDiff;
            CheckIn::TGrpEMDProps emdProps;
            CheckIn::CalcGrpEMDProps<TGrpServiceAutoList>(svcsAutoBefore, grp.svc_auto,  handmadeAutoEMDDiff, emdProps);
            CheckIn::TGrpEMDProps::toDB(grp.id, emdProps);

            handmadeEMDDiff.insert(handmadeAutoEMDDiff.begin(), handmadeAutoEMDDiff.end());

            grp.svc_auto.get().toDB(GrpId_t(grp.id));
          }

          if (grp.wt)
          {
            if (TReqInfo::Instance()->client_type==ctTerm ||
                isTagConfirmRequestSBDO(reqNode) ||
                isTagRevokeRequestSBDO(reqNode))
            {
              //���� � ������ ��� � ���⭮�� ������
              //�������� ���� �������뢠�� �� ���������� �� ����, �᫨, ���ਬ��, 㤠����� ���ᠦ�� ???
              WeightConcept::TNormFltInfo normFltInfo(fltInfo, grpMktFlight);
              WeightConcept::TAirlines airlines(grp.id,
                                                grpMktFlight.pr_mark_norms?grpMktFlight.airline:fltInfo.airline,
                                                "RecalcPaidBagToDB");
              map<int/*id*/, TEventsBagItem> curr_bag;
              GetBagToLogInfo(grp.id, curr_bag);
              WeightConcept::TPaidBagList prior_paid;
              WeightConcept::PaidBagFromDB(NoExists, grp.id, prior_paid);
              WeightConcept::TPaidBagList result_paid;
              WeightConcept::RecalcPaidBagToDB(airlines, grpInfoBefore.bag, curr_bag, grpInfoBefore.pax, normFltInfo, trfer, grp, paxs, prior_paid, pr_unaccomp, true, result_paid);
              CheckIn::TryCleanServicePayment(result_paid, paymentBefore, grp.payment);

              if (isTagConfirmRequestSBDO(reqNode) ||
                  isTagRevokeRequestSBDO(reqNode))
              {
                if (result_paid.becamePaid(prior_paid))
                  throw UserException("MSG.SBDO.SERVICES_BECAME_PAID");
              }
            }
          }

          if (grp.rfisc_used && AfterSaveInfo.action==CheckIn::actionNone &&
              (TReqInfo::Instance()->client_type==ctTerm ||
               isTagConfirmRequestSBDO(reqNode) ||
               isTagRevokeRequestSBDO(reqNode)))
          {
            if (!(TReqInfo::Instance()->client_type==ctTerm &&
                  !TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION)) || grp.pc)
              AfterSaveInfo.action=CheckIn::actionSvcPaymentStatus;
          }

          if (grp.payment)
          {
            grp.payment.get().getAllListItems(grp.id, pr_unaccomp);
            grp.payment.get().toDB(GrpId_t(grp.id), PointId_t(grp.point_dep));
          }

          SaveTagPacks(reqNode);
        }
      }
      else
      {
        if (segList.needCopyBaggage(GrpId_t(grp.id), trfer.size()))
          //����ᮬ �����뢠�� � ��⠫�� ��㯯� �����, ��ન, 業�� �����
          CheckIn::TGroupBagItem::copyDB(GrpId_t(AfterSaveInfo.segs.front().grp_id), GrpId_t(grp.id), PointId_t(grp.point_dep));

        if (!unknown_concept)
        {
          //����� ��⥬� ����
          if (segList.front().grp.svc)
            TGrpServiceList::copyDB(GrpId_t(AfterSaveInfo.segs.front().grp_id), GrpId_t(grp.id));
          if (segList.front().grp.svc_auto)
            TGrpServiceAutoList::copyDB(GrpId_t(AfterSaveInfo.segs.front().grp_id), GrpId_t(grp.id));
          if (segList.front().grp.payment)
            CheckIn::TServicePaymentList::copyDB(GrpId_t(AfterSaveInfo.segs.front().grp_id), GrpId_t(grp.id));
        }
      }

      timing.finish("ChangeBaggage", grp.point_dep);

      timing.start("ETProcessing", grp.point_dep);

      if (grp.status!=psTransit && !pr_unaccomp)
      {
        //�஢�ਬ �㡫�஢���� ����⮢
        for(CheckIn::TPaxList::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
        {
          const CheckIn::TPaxItem &pax=p->pax;
          try
          {
            if (!pax.TknExists) continue;
            if (pax.tkn.checkedInETCount()>1)
              throw UserException("MSG.CHECKIN.DUPLICATED_ETICKET",
                                  LParams()<<LParam("eticket",pax.tkn.no)
                                           <<LParam("coupon",IntToString(pax.tkn.coupon))); //WEB
          }
          catch(const CheckIn::UserException&)
          {
            throw;
          }
          catch(const UserException &e)
          {
            if (pax.id!=NoExists)
              throw CheckIn::UserException(e.getLexemaData(), grp.point_dep, pax.id);
            else
              throw;
          }
        }
        if (!needSyncEdsEts(ediResNode))
        {
          //������� ticket_confirm � events_bilingual �� �᭮�� ���⢥ত����� ����ᮢ
          TQuery Qry(&OraSession);
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
            }
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
              }
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
              }

            }
          }

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
              }
            }
          }
        }

        //��� ����� ETCheckStatus::CheckGrpStatus
        //��易⥫쭮 �� ckin.check_grp
        if (needSyncEdsEts(ediResNode) && reqInfo->client_type!=ctPNL)
        {
          ETStatusInterface::ETCheckStatus(grp.id, csaGrp, NoExists, false, ChangeStatusInfo.ET, true);
          EMDStatusInterface::EMDCheckStatus(grp.id, paymentBeforeWithAuto, ChangeStatusInfo.EMD);
        }
      }

      if (segs.isFirstGrp())
        rollbackGuaranteedOnFirstSegment=rollbackBeforeSvcAvailability(AfterSaveInfo,
                                                                       ediResNode,
                                                                       setList,
                                                                       grp,
                                                                       new_checkin);

      bool rollbackGuaranteed = (needSyncEdsEts(ediResNode) && !ChangeStatusInfo.empty()) ||
                                rollbackGuaranteedOnFirstSegment;

      CheckIn::TPaxGrpItem::setRollbackGuaranteedTo(grp.id, rollbackGuaranteed);


      timing.finish("ETProcessing", grp.point_dep);

      if (!rollbackGuaranteed)
      {
        if (new_checkin && !pr_unaccomp && wl_type.empty() && grp.status!=psCrew)
        {
          timing.start("SeatsPassengers", grp.point_dep);
          CheckIn::seatingWhenNewCheckIn(*iSegListItem, fltInfo, grpMktFlight);
          timing.finish("SeatsPassengers", grp.point_dep);
        }
      }

      if (!rollbackGuaranteed)
      {
        timing.start("GetGrpToLogInfo", grp.point_dep);
        //�����뢠�� � ��� ⮫쪮 �᫨ �� �㤥� �⪠� �࠭���樨 ��-�� ���饭�� � ���
        GetGrpToLogInfo(grp.id, grpInfoAfter);
        if(new_checkin && reqInfo->client_type==ctTerm) {
         grpInfoAfter.setTermAgentSeatNo(paxs);
        }
        timing.finish("GetGrpToLogInfo", grp.point_dep);
      }

      if (!rollbackGuaranteed && !pr_unaccomp)
      {
        timing.start("sync_pax_grp", grp.point_dep);
        rozysk::sync_pax_grp(grp.id, reqInfo->desk.code, reqInfo->user.descr);
        timing.finish("sync_pax_grp", grp.point_dep);
      }

      timing.start("del_vo", grp.point_dep);
      TVouchers().fromDB(grp.point_dep, grp.id).to_deleted();
      timing.finish("del_vo", grp.point_dep);

      timing.start("check_grp", grp.point_dep);

      segs_grp_ids.push_back(grp.id);
      // ࠭�� ����� �� �맮� doCheckGrp(grp.id);

      timing.finish("check_grp", grp.point_dep);

      if (!rollbackGuaranteed)
      {
        timing.start("recount", grp.point_dep);

        CheckIn::TCounters().recount(grp, priorSimplePaxList, currSimplePaxList, __FUNCTION__);

        timing.finish("recount", grp.point_dep);
      }

      if (!rollbackGuaranteed)
      {
        timing.start("annul_bt_after", grp.point_dep);

        TAnnulBT annul_bt_after;
        annul_bt_after.get(grp.id);
        annul_bt.minus(annul_bt_after);

        annul_bt.toDB();

        timing.finish("annul_bt_after", grp.point_dep);
      }

      timing.start("overload_alarm", grp.point_dep);

      //�஢�ਬ ���ᨬ����� ����㧪�
      bool overload_alarm = calc_overload_alarm( grp.point_dep ); // ���᫨�� �ਧ��� ��ॣ�㧪�
      if (overload_alarm)
      {
        //�஢��塞, � �� �뫮 �� ������ �⬥�� ॣ����樨 ��㯯� �� �訡�� �����
        TQuery Qry(&OraSession);
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
            }
          }
          catch(const CheckIn::OverloadException &E)
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
              sp.rollback();

              if (reqInfo->client_type!=ctPNL)
              {
                if (reqInfo->client_type==ctTerm)
                {
                  if (segs.size()>1)
                    showError( GetLexemeDataWithFlight(E.getLexemaData( ), fltInfo) );
                  else
                    showError( E.getLexemaData( ) );
                }
                else
                {
                  //���, ���᪨
                  CheckIn::UserException ce(E.getLexemaData(), grp.point_dep);
                  CheckIn::showError(ce.segs);
                }
              }

              set_alarm( grp.point_dep, Alarm::Overload, true ); // ��⠭����� �ਧ��� ��ॣ�㧪� ��ᬮ��� �� � �� ॠ�쭮� ��ॣ�㧪� ���
              Set_AODB_overload_alarm( grp.point_dep, true );

              if (reqInfo->client_type==ctPNL) throw;
              return false;
            }
          }
        }
      }

      set_alarm( grp.point_dep, Alarm::Overload, overload_alarm ); // ��⠭����� �ਧ��� ��ॣ�㧪�

      timing.finish("overload_alarm", grp.point_dep);

      timing.start("getSubclassGroup", grp.point_dep);

      if (!pr_unaccomp && grp.status!=psCrew)
      {
        //����⠥� ������ ��㯯� �������ᮢ
        TQuery PaxQry(&OraSession);
        PaxQry.SQLText=
          "SELECT pax.subclass, pax_grp.class "
          "FROM pax_grp, pax "
          "WHERE pax_grp.grp_id=pax.grp_id AND pax.grp_id=:grp_id "
          "ORDER BY pax.reg_no, pax.seats DESC";
        PaxQry.CreateVariable("grp_id",otInteger,grp.id);
        PaxQry.Execute();
        if (!PaxQry.Eof)
        {
          if (!flightRbd)
            flightRbd=boost::in_place(fltInfo);
          const TFlightRbd& rbds=flightRbd.get();
          if (rbds.empty())
            throw UserException("MSG.CHECKIN.NOT_MADE_IN_ONE_CLASSES"); //WEB

          auto subclass_grp = boost::make_optional<TSubclassGroup>(false, 0); // subclass_grp=TSubclassGroup(0);
          for(;!PaxQry.Eof;PaxQry.Next())
          {
            boost::optional<TSubclassGroup> pax_subclass_grp=rbds.getSubclassGroup(PaxQry.FieldAsString("subclass"),
                                                                                   PaxQry.FieldAsString("class"));
            if (!pax_subclass_grp)
              throw UserException("MSG.CHECKIN.NOT_MADE_IN_CLASS",
                                  LParams()<<LParam("class",ElemIdToCodeNative(etClass,PaxQry.FieldAsString("class"))));
            if (!subclass_grp)
              subclass_grp=pax_subclass_grp;
            else if (!(subclass_grp==pax_subclass_grp))
              throw UserException("MSG.CHECKIN.INPOSSIBLE_SUBCLASS_IN_GROUP");
          }

          if (!subclass_grp)
            throw EXCEPTIONS::Exception("%s: !subclass_grp!!!", __FUNCTION__);

          TQuery Qry(&OraSession);
          Qry.SQLText="UPDATE pax_grp SET class_grp=:class_grp WHERE grp_id=:grp_id";
          Qry.CreateVariable("grp_id",otInteger,grp.id);
          Qry.CreateVariable("class_grp",otInteger,subclass_grp.get().value());
          Qry.Execute();
        }

        if ( first_pax_on_flight ) {
          ComponCreator::setManualCompChg( grp.point_dep );
        }
      }

      timing.start("getSubclassGroup", grp.point_dep);

      if (!rollbackGuaranteed)
      {
        timing.start("other_alarms", grp.point_dep);

        if (!pr_unaccomp)
        {
          if (grp.status!=psCrew)
          {
            //���ᠦ���
            //����塞 � �����뢠�� �ਧ��� waitlist_alarm � brd_alarm � spec_service_alarm
            TTripAlarmHook::set(Alarm::Brd, grp.point_dep);
            check_spec_service_alarm( grp.point_dep );
            TTripAlarmHook::set(Alarm::UnboundEMD, grp.point_dep);
            check_conflict_trfer_alarm( grp.point_dep );
          }
          else
          {
            //����
            check_crew_alarms( grp.point_dep );
          }
        }

        check_TrferExists( grp.point_dep );
        if (needCheckUnattachedTrferAlarm)
          check_u_trfer_alarm_for_grp( grp.point_dep, grp.id, grpTagsBefore);
        if (new_checkin) //��� ����� ��������� 㦥 ��࠭�� ����稫� nextTrferSegs
          InboundTrfer::GetNextTrferCheckedFlts(grp.id, idGrp, nextTrferSegs);
        check_unattached_trfer_alarm(nextTrferSegs);

        timing.finish("other_alarms", grp.point_dep);
      }

      timing.start("BSM", grp.point_dep);

      //BSM
      if (BSMsend) BSM::Send(grp.point_dep,grp.id,true,BSMContentBefore,BSMaddrs);

      timing.finish("BSM", grp.point_dep);


      if (!rollbackGuaranteed)
      {
        timing.start("utg_prl", grp.point_dep);

        if (grp.status!=psCrew)
        {
          QParams QryParams;
          QryParams << QParam("point_id", otInteger, grp.point_dep)
                    << QParam("last_flt_change_tid", otInteger, PgOra::getSeqCurrVal_int("CYCLE_TID__SEQ"));

          const std::string UpdSql =
              "UPDATE utg_prl SET last_flt_change_tid=:last_flt_change_tid "
              "WHERE point_id=:point_id ";

          const std::string InsSql =
              "INSERT INTO utg_prl(point_id, last_tlg_create_tid, last_flt_change_tid) "
              "VALUES (:point_id, NULL, :last_flt_change_tid) ";

          DB::TCachedQuery UpdQry(PgOra::getRWSession("UTG_PRL"), UpdSql, QryParams, STDLOG);
          DB::TCachedQuery InsQry(PgOra::getRWSession("UTG_PRL"), InsSql, QryParams, STDLOG);

          UpdQry.get().Execute();
          if(UpdQry.get().RowsProcessed() == 0) {
              InsQry.get().Execute();
          }
        }

        timing.finish("utg_prl", grp.point_dep);
      }
    }
    catch(const UserException &e)
    {
      if (reqInfo->client_type==ctPNL) throw;
      if (reqInfo->client_type==ctTerm)
      {
        if (segs.size()>1)
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
          dynamic_cast<const CheckIn::UserException&>(e);
          throw; //�� 㦥 CheckIn::UserException - �ப��뢠�� �����
        }
        catch (const bad_cast&)
        {
          throw CheckIn::UserException(e.getLexemaData(), grp.point_dep);
        }
      }
    }

  } //横� �� ᥣ���⠬

  boost::optional<GrpId_t> tckinId=TCkinRoute::toDB(tckinGroupsWhenNewCheckIn);
  if (tckinId)
  {
    AfterSaveInfo.tckin_id=tckinId.get().get();
    CheckIn::TGroupBagItem::copyPaxPool(tckinGroupsWhenNewCheckIn);
  }

  bool rollbackGuaranteed = (needSyncEdsEts(ediResNode) && !ChangeStatusInfo.empty()) ||
                            rollbackGuaranteedOnFirstSegment;
  if(!rollbackGuaranteed)
  {
      appsCollector.send();
      synchronizePaxEvents(newOrCancelledPax, modifiedPaxRem);
      modifiedPaxRem.executeCallbacksByPaxId(TRACE5);
      modifiedPaxRem.executeCallbacksByCategory(TRACE5);
      newOrCancelledPax.executeCallbacksByPaxId(TRACE5);
      newOrCancelledPax.executeCallbacksByCategory(TRACE5);

      PaxCalcData::ChangesHolder changesHolder(paxCheckIn);
      PaxCalcData::addChanges(newOrCancelledPax, changesHolder);
      PaxCalcData::addChanges(modifiedPaxRem, changesHolder);
      changesHolder.executeCallbacksByPaxId(TRACE5);
  }

  for(auto grp_id: segs_grp_ids) {
      doCheckGrp(grp_id);
  }

  timing.start("svc_auto");

  if (new_checkin)
  {
    TSegList::reverse_iterator iNextSegListItem=segList.rend();
    for(TSegList::reverse_iterator iSegListItem=segList.rbegin(); iSegListItem!=segList.rend(); ++iSegListItem)
    {
      if (iSegListItem->grp.svc_auto)
        iSegListItem->grp.svc_auto.get().toDB(GrpId_t(iSegListItem->grp.id));
      if (iNextSegListItem!=segList.rend())
        TGrpServiceAutoList::copyDB(GrpId_t(iNextSegListItem->grp.id), GrpId_t(iSegListItem->grp.id), false /*clear*/);
      iNextSegListItem=iSegListItem;
    }
  };

  timing.finish("svc_auto");

  if (AfterSaveInfo.action==CheckIn::actionSvcPaymentStatus)
  try
  {
    timing.start("svc_payment_status");

    if (AfterSaveInfo.segs.empty())
      throw SvcPaymentStatusNotApplicable("AfterSaveInfo.segs.empty()");

    TCkinGrpIds tckinGrpIds;
    TPaidRFISCList paid;
    if (getSvcPaymentStatus(AfterSaveInfo.segs.front().grp_id,
                            boost::none,
                            reqNode,
                            ediResNode,
                            ASTRA::rollbackSavePax,
                            SirenaExchangeList,
                            tckinGrpIds,
                            paid,
                            httpWasSent))
    {
      //paid ����稫�
      if (isTagConfirmRequestSBDO(reqNode) ||
          isTagRevokeRequestSBDO(reqNode))
      {
        if (paid.becamePaid(AfterSaveInfo.segs.front().grp_id))
          throw UserException("MSG.SBDO.SERVICES_BECAME_PAID");
      }
      paid.getAllListItems();
      bool update_payment=false;
      bool check_emd_status=(needSyncEdsEts(ediResNode) && reqInfo->client_type!=ctPNL);
      for(TCkinGrpIds::const_iterator iGrpId=tckinGrpIds.begin(); iGrpId!=tckinGrpIds.end(); ++iGrpId)
      {
        const GrpId_t& grpId=*iGrpId;
        CheckIn::TServicePaymentListWithAuto paymentBeforeWithAuto;
        if (check_emd_status)
          paymentBeforeWithAuto.fromDB(grpId.get());
        if (iGrpId==tckinGrpIds.begin())
        {
          paid.toDB(grpId);
          CheckIn::TServicePaymentList payment;
          payment.fromDB(grpId);
          update_payment=CheckIn::TryCleanServicePayment(paid, payment);
          if (update_payment) {
            for (const TSegListItem& segItem: segList) {
              if (segItem.grp.id == grpId.get()) {
                LogTrace(TRACE6) << "payment.toDB";
                payment.toDB(grpId, PointId_t(segItem.grp.point_dep));
                break;
              }
            }
          }
        }
        else
        {
          TPaidRFISCList::copyDB(tckinGrpIds.front(), grpId);
          if (update_payment)
            CheckIn::TServicePaymentList::copyDB(tckinGrpIds.front(), grpId);
        };
        if (check_emd_status)
          EMDStatusInterface::EMDCheckStatus(grpId.get(), paymentBeforeWithAuto, ChangeStatusInfo.EMD);
      }
    }

    timing.finish("svc_payment_status");
  }
  catch(const SvcPaymentStatusNotApplicable &e)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": " << e.what();
  }

  timing.finish("SavePax");

  if (!isDoomedToWait() &&
      reqInfo->client_type==ctTerm &&  //⮫쪮 � �ନ����, ������� ctPNL � ctHTTP
      reqInfo->desk.compatible(PAX_CONFIRMATIONS_VERSION))
  {
    timing.start("PaxConfirmations");

    if (PaxConfirmations::Messages::checkNeeded(reqNode) &&
        !PaxConfirmations::AppliedMessages::exists(reqNode))
    {
      PaxConfirmations::Messages messages(DCSAction::CheckInOnDesk,
                                          segList.transformForPaxConfirmations(new_checkin),
                                          !new_checkin);
      xmlNodePtr resNode=NodeAsNode("/term/answer", getXmlCtxt()->resDoc);
      if (messages.toXML(resNode, AstraLocale::OutputLang())) throw UserException2();
      PaxConfirmations::Messages::checkCompleted(reqNode);
    }
    if(!rollbackGuaranteed &&
       PaxConfirmations::AppliedMessages::exists(reqNode))
    {
      PaxConfirmations::Segments segments=segList.transformForPaxConfirmations(new_checkin);
      PaxConfirmations::AppliedMessages appliedMessages(reqNode);
      appliedMessages.toDB(segments);
      appliedMessages.toLog(segments);
    }

    timing.finish("PaxConfirmations");
  }

  return true;
}

void CheckInInterface::tryChangeFlight(const CheckIn::TSimplePaxGrpItem& grp,
                                       xmlNodePtr reqNode, xmlNodePtr resNode)
{
  const int term_point_id=NodeAsInteger("point_id",reqNode);
  if (grp.point_dep!=term_point_id)
  {
    if (GetNode("data", resNode)!=nullptr) return; //㦥 ���� ३�

    xmlNodePtr dataNode=NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "point_id", grp.point_dep );
    if (!readTripHeaderAndOther( grp.point_dep, reqNode, dataNode ))
    {
      string msg;
      TTripInfo info;
      if (info.getByPointId(grp.point_dep))
      {
        if (!grp.is_unaccomp())
          msg=getLocaleText("MSG.PASSENGER.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_FLIGHT", LParams() << LParam("flt", GetTripName(info,ecCkin)));
      }
      else
      {
        if (!grp.is_unaccomp())
          msg=getLocaleText("MSG.PASSENGER.FROM_OTHER_FLIGHT");
        else
          msg=getLocaleText("MSG.BAGGAGE.FROM_OTHER_FLIGHT");
      }
      throw AstraLocale::UserException(msg);
    }
  }
}

void CheckInInterface::LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node;
  int grp_id=NoExists;
  int term_point_id=NodeAsInteger("point_id",reqNode);
  bool EMDRefresh=NodeAsBoolean("emd_refresh",reqNode,true);
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

      CheckIn::TSimplePaxGrpItem grp;
      if (!grp.getByPaxId(pax_id))
        throw UserException("MSG.PASSENGER.NOT_CHECKIN");
      grp_id=grp.id;

      tryChangeFlight(grp, reqNode, resNode);
    }
    else
    {
      int reg_no=NodeAsInteger(node);
      Qry.Clear();
      Qry.SQLText=
        "SELECT pax_grp.grp_id, pax.seats FROM pax_grp,pax "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      point_dep=:point_id AND reg_no=:reg_no ";
      Qry.CreateVariable("point_id",otInteger,term_point_id);
      Qry.CreateVariable("reg_no",otInteger,reg_no);
      Qry.Execute();
      if (Qry.Eof) throw UserException(1,"MSG.CHECKIN.REG_NO_NOT_FOUND");
      grp_id=Qry.FieldAsInteger("grp_id");
      bool exists_with_seat=false;
      bool exists_without_seat=false;
      for(;!Qry.Eof;Qry.Next())
      {
        if (grp_id!=Qry.FieldAsInteger("grp_id"))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",term_point_id,reg_no);
        int seats=Qry.FieldAsInteger("seats");
        if ((seats>0 && exists_with_seat) ||
            (seats<=0 && exists_without_seat))
          throw EXCEPTIONS::Exception("Duplicate reg_no (point_id=%d reg_no=%d)",term_point_id,reg_no);
        if (seats>0)
          exists_with_seat=true;
        else
          exists_without_seat=true;
      }
    }
  }
  else grp_id=NodeAsInteger(node);

  if ( EMDRefresh ) {
    EMDAutoBoundInterface::refreshEmd(EMDAutoBoundGrpId(GrpId_t(grp_id)), reqNode);
  }

  LoadPaxByGrpId(GrpId_t(grp_id),reqNode,resNode,false);
}

void AddPaxCategory(const CheckIn::TPaxItem &pax, set<string> &cats)
{
  if (pax.pers_type==child && pax.seats==0) cats.insert("CHC");
  if (pax.pers_type==baby && pax.seats==0) cats.insert("INA");
}

const char* pax_sql=
    "SELECT pax.*, "
    "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,NULL,NULL,'one',rownum) AS seat_no, "
    "       crs_pax.pax_id AS crs_pax_id, "
    "       crs_pax.bag_norm AS crs_bag_norm, "
    "       crs_pax.bag_norm_unit AS crs_bag_norm_unit "
    "FROM pax,crs_pax "
    "WHERE pax.pax_id=crs_pax.pax_id(+) AND crs_pax.pr_del(+)=0 AND "
    "      pax.grp_id=:grp_id "
    "ORDER BY ABS(pax.reg_no), pax.seats DESC";

namespace SirenaExchange
{

void fillPaxsBags(int first_grp_id, TExchange &exch, CheckIn::TPaxGrpCategory::Enum &grp_cat, TCkinGrpIds &tckinGrpIds,
                  bool include_refused)
{
  TCheckedReqPassengers req_grps(first_grp_id, include_refused, false);
  TCheckedResPassengers res_grps;
  fillPaxsBags(req_grps, exch, res_grps);
  TCheckedResPassengers::const_iterator iResGrp=res_grps.find(first_grp_id);
  if (iResGrp==res_grps.end()) throw EXCEPTIONS::Exception("%s: strange situation iResGrp==res_grps.end()");
  grp_cat=iResGrp->second.grp_cat;
  tckinGrpIds=iResGrp->second.tckinGrpIds;
}

void fillPaxsBags(const TCheckedReqPassengers &req_grps, TExchange &exch, TCheckedResPassengers &res_grps)
{
  TPaxSection *paxSection=dynamic_cast<TPaxSection*>(&exch);
  TSvcSection *svcSection=dynamic_cast<TSvcSection*>(&exch);

  for(TCheckedReqPassengers::const_iterator iReqGrp=req_grps.begin(); iReqGrp!=req_grps.end(); ++iReqGrp)
  {
    int first_grp_id=iReqGrp->first;
    TCheckedResPassengers::iterator iResGrp=res_grps.insert(make_pair(first_grp_id, TCheckedResPassengersItem())).first;
    CheckIn::TPaxGrpCategory::Enum &grp_cat=iResGrp->second.grp_cat;

    TCkinRoute tckin_route;
    tckin_route.getRouteAfter(GrpId_t(first_grp_id),
                              TCkinRoute::NotCurrent,
                              TCkinRoute::OnlyDependent,
                              TCkinRoute::WithoutTransit);
    iResGrp->second.tckinGrpIds=tckin_route.getTCkinGrpIds();
    iResGrp->second.tckinGrpIds.insert(iResGrp->second.tckinGrpIds.begin(), GrpId_t(first_grp_id));

    TCkinGrpIds &tckinGrpIds=iResGrp->second.tckinGrpIds;

    TTrferRoute trfer;
    int seg_no=0;
    std::list<TPaxItem> paxs;
    for(TCkinGrpIds::const_iterator iGrpId=tckinGrpIds.begin(); iGrpId!=tckinGrpIds.end(); ++iGrpId,seg_no++)
    {
      if (req_grps.only_first_segment && iGrpId!=tckinGrpIds.begin()) continue;

      const GrpId_t& grpId=*iGrpId;

      CheckIn::TSimplePaxGrpItem grp;
      if (!grp.getByGrpId(grpId.get()))
      {
        tckinGrpIds.clear();
        return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
      }

      TAdvTripInfo operFlt;
      operFlt.getByGrpId(grpId.get());

      const auto times_in=TTripInfo::get_times_in(grp.point_arv);

      if (iGrpId==tckinGrpIds.begin())
        grp_cat=grp.grpCategory();

      if (grp_cat!=CheckIn::TPaxGrpCategory::UnnacompBag)
      {
        if (iGrpId==tckinGrpIds.begin())
          trfer.GetRoute(grp.id, trtNotFirstSeg);

        if (svcSection && iGrpId==tckinGrpIds.begin())
        {
          int trfer_seg_count_pc=0;
          for(TTrferRoute::const_iterator t=trfer.begin(); t!=trfer.end(); ++t, trfer_seg_count_pc++)
            if (!t->piece_concept || !t->piece_concept.get()) break; //������뢠�� ⮫쪮 ���-�� �࠭����� ᥣ���⮢ � ���⮢�� ��⥬�� ����

          svcSection->svcs.addChecked(req_grps, grpId.get(), tckin_route.size()+1, trfer_seg_count_pc+1);
        }

        TGrpMktFlight mktFlight;
        mktFlight.getByGrpId(grp.id);

        TCachedQuery PaxQry(pax_sql, QParams() << QParam("grp_id", otInteger, grp.id));
        PaxQry.get().Execute();


        if (paxSection)
        {
          list<TPaxItem>::iterator iReqPax=paxs.begin();
          for(;!PaxQry.get().Eof; PaxQry.get().Next())
          {
            CheckIn::TPaxItem pax;
            pax.fromDB(PaxQry.get());
            if (!pax.refuse.empty() && !req_grps.include_refused) continue;

            if (svcSection && req_grps.include_unbound_svcs)
            {
              svcSection->svcs.addUnbound(req_grps, grpId.get(), pax.id);
              fillProtBeforePaySvcs(operFlt, pax.id, exch);
            }

            if (iGrpId==tckinGrpIds.begin())
              iReqPax=paxs.insert(paxs.end(), TPaxItem());
            if (iReqPax==paxs.end()) throw EXCEPTIONS::Exception("%s: strange situation iReqPax==paxs.end()");
            TPaxItem &reqPax=*iReqPax;

            if (iGrpId==tckinGrpIds.begin())
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
                reqSeg.subcl=p->subclass; //����� ��।��� �������� �࠭��୮�� ������� (��᫥ ��������� �����)
              }
              if (s!=trfer.end()) throw EXCEPTIONS::Exception("%s: strange situation s!=trfer.end()", __FUNCTION__);
              if (p!=pax_trfer.end()) throw EXCEPTIONS::Exception("%s: strange situation p!=pax_trfer.end()", __FUNCTION__);
            }

            pair< SirenaExchange::TPaxSegMap::iterator, bool > res=
                reqPax.segs.insert(make_pair(seg_no,SirenaExchange::TPaxSegItem()));
            SirenaExchange::TPaxSegItem &reqSeg=res.first->second;
            reqSeg.set(seg_no, operFlt, grp.airp_arv, mktFlight, times_in);
            reqSeg.subcl=pax.getCabinSubclass(); //����� �⥫� ��।��� ��������, ����� �� ����砫쭮 � ����� (�� ��������� �����), �� ��७� ᪠���� ��।����� �������� ��᫥ ���������
            reqSeg.setTicket(pax.tkn, paxSection);
            CheckIn::LoadPaxFQT(pax.id, reqSeg.fqts);
            reqSeg.pnrAddrs.getByPaxIdFast(pax.id);
            ++iReqPax;
          }
        }
      }
      else break;
    }
    //䨫���㥬 ���ᠦ�஢
    for(std::list<TPaxItem>::iterator i=paxs.begin(); i!=paxs.end();)
    {
      if (req_grps.pax_included(first_grp_id, i->id))
        ++i;
      else
        i=paxs.erase(i);
    }
    if (!paxs.empty() && paxSection)
    {
      std::list<TPaxItem> &paxs_ref=paxSection->paxs;
      paxs_ref.insert(paxs_ref.end(), paxs.begin(), paxs.end());
    }
  }

  if (paxSection) paxSection->addMissingTickets();
}

} //namespace SirenaExchange

static bool rollbackBeforeSvcAvailability(const CheckIn::TAfterSaveInfo& info,
                                          const xmlNodePtr& externalSysResNode,
                                          const TTripSetList& setList,
                                          const CheckIn::TSimplePaxGrpItem& grp,
                                          const bool& isNewCheckIn)
{
  bool result=info.action==CheckIn::actionSvcAvailability &&
              setList.value<bool>(tsPieceConcept) &&
              grp.grpCategory()==CheckIn::TPaxGrpCategory::Passenges &&
              !TReqInfo::Instance()->api_mode &&
              SirenaExchange::needSync(externalSysResNode);

  if (result && !isNewCheckIn)
  {
    //�஢�ਬ, �� �� �ᥬ �⬥���� ॣ������ � ��㯯�
    result=!CheckIn::TPaxGrpItem::allPassengersRefused(grp.id);
  }

  return result;
}

void CheckInInterface::AfterSaveAction(CheckIn::TAfterSaveInfoData& data)
{
  if (data.action!=CheckIn::actionSvcAvailability) return;

  ProgTrace(TRACE5, "%s started with actionSvcAvailability (first_grp_id=%d)", __FUNCTION__, data.grpId);

  TCachedQuery Qry("SELECT point_dep, piece_concept, NVL(rollback_guaranteed,0) AS rollback_guaranteed "
                   "FROM pax_grp "
                   "WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, data.grpId));
  Qry.get().Execute();
  if (Qry.get().Eof) {
      LogTrace(TRACE1) << "pax_grp not found";
      return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
  }

  class SomethingHasChanged {};

  try
  {
    if (!Qry.get().FieldIsNULL("piece_concept"))
      //piece_c�ncept 㦥 ��⠭�����
      throw SomethingHasChanged();

    int point_id=Qry.get().FieldAsInteger("point_dep");
    bool rollbackGuaranteed=Qry.get().FieldAsInteger("rollback_guaranteed")!=0;

    TTripSetList setList;
    setList.fromDB(point_id);
    if (setList.empty())
      throw SomethingHasChanged();

    TCkinGrpIds tckinGrpIds;
    map<int/*seg_no*/,TBagConcept::Enum> bag_concept_by_seg;
    CheckIn::TPaxGrpCategory::Enum grp_cat;
    TLogLocale event;

    SirenaExchange::TAvailabilityReq req;
    SirenaExchange::fillPaxsBags(data.grpId, req, grp_cat, tckinGrpIds);
    if (tckinGrpIds.empty())
      throw SomethingHasChanged();

    TReqInfo *reqInfo = TReqInfo::Instance();
    if (setList.value<bool>(tsPieceConcept))
    {
      if (grp_cat==CheckIn::TPaxGrpCategory::Passenges)
      {
        SirenaExchange::TAvailabilityRes res;
//        res.setSrcFile("svc_availability_res.xml");
//        res.setDestFile("svc_availability_res.xml");
        try
        {
          if (!req.paxs.empty() && !reqInfo->api_mode)
          {
//#define SVC_AVAILABILITY_SYNC_MODE
#ifndef SVC_AVAILABILITY_SYNC_MODE
            if(data.needSync()) {
              if (data.httpWasSent)
                throw Exception("%s: very bad situation! needSync again!", __FUNCTION__);

              ASTRA::rollbackSavePax();
              SvcSirenaInterface::AvailabilityRequest(data.reqNode,
                                                      data.answerResNode,
                                                      req);
              data.httpWasSent = true;
              return;
            } else {
              res.parseResponse(data.getAnswerNode());
              if (res.error()) throw Exception("SIRENA ERROR: %s", res.traceError().c_str());
            }
#else
            SirenaExchange::SendRequest(req, res); //ᨭ�஭�� ��⮤ ������ � �७��
#endif
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
                if ((unsigned)s->second.id<tckinGrpIds.size())
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
              if (seg_concept.get()==TBagConcept::Piece)
              {
                //��।��塞 piece ��� weight �� ������/������⢨� �������� RFISC
                seg_concept=res.exists_rfisc(s->second.id, false)?TBagConcept::Piece:TBagConcept::Weight;
              }
              bag_concept_by_seg.insert(make_pair(s->second.id, seg_concept.get()));

              if ((unsigned)s->second.id<tckinGrpIds.size())
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
            }
            if (!bag_concept) throw EXCEPTIONS::Exception("%s: strange situation: unknown bag_concept", __FUNCTION__);

            if (bag_concept.get()==TBagConcept::Piece)
            {
              if (req.paxs.size()>9)
                throw UserException("MSG.CHECKIN.PIECE_CONCEPT_NOT_SUPPORT_MORE_9_PASSENGERS");
            };
            res.rfiscsToDB(tckinGrpIds, bag_concept.get(), true); //�� ��ࢮ� �⠯� �ਬ��塞 ⮫쪮 ���楯� ������ (old_version=true) !!!��⮬ ����
            res.normsToDB(tckinGrpIds);
            res.brandsToDB(tckinGrpIds);
            res.setAdditionalListId(tckinGrpIds); //��易⥫쭮 ��᫥ rfiscsToDB � normsToDB
          }
        }
        catch(const UserException &e)
        {
          throw;
        }
        catch(const std::exception &e)
        {
#ifndef SVC_AVAILABILITY_SYNC_MODE
          if (data.needSync()) throw;
#endif
          bag_concept_by_seg.clear();
          if (res.error() &&
              (res.error_code=="1" || res.error_message=="Incorrect format") &&
              (res.error_reference.path=="passenger/ticket/number" ||
               res.error_reference.path=="passenger/segment/ticket/number") &&
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
          }
        }
      }
    }

    if (grp_cat!=CheckIn::TPaxGrpCategory::UnnacompBag) {
      req.bagTypesToDB(tckinGrpIds);  //������塞 ��ᮢ묨 ⨯��� ������
    } else {
      unaccBagTypesToDB(data.grpId);
    }

    TCachedQuery GrpQry("BEGIN "
                        "  UPDATE pax_grp "
                        "  SET piece_concept=:piece_concept "
                        "  WHERE grp_id=:grp_id "
                        "  RETURNING point_dep INTO :point_id; "
                        "END;",
                        QParams() << QParam("grp_id", otInteger)
                        << QParam("piece_concept", otInteger)
                        << QParam("point_id", otInteger));

    TCachedQuery TrferQry("UPDATE transfer SET piece_concept=:piece_concept WHERE grp_id=:grp_id AND transfer_num=:transfer_num",
                          QParams() << QParam("grp_id", otInteger)
                          << QParam("transfer_num", otInteger)
                          << QParam("piece_concept", otInteger));

    TCkinGrpIds::const_iterator iGrpId=tckinGrpIds.begin();
    if (bag_concept_by_seg.empty())
    {
      for(; iGrpId!=tckinGrpIds.end(); ++iGrpId)
      {
        const GrpId_t& grpId=*iGrpId;
        GrpQry.get().SetVariable("grp_id", grpId.get());
        GrpQry.get().SetVariable("piece_concept", (int)false);
        GrpQry.get().SetVariable("point_id", FNull);
        GrpQry.get().Execute();

        if (!event.lexema_id.empty() &&
            !GrpQry.get().VariableIsNULL("point_id"))
        {
          event.id1=GrpQry.get().GetVariableAsInteger("point_id");
          event.id3=grpId.get();
          reqInfo->LocaleToLog(event);
        };
      }
    }
    else
    {
      map<int/*seg_no*/,TBagConcept::Enum>::const_iterator i=bag_concept_by_seg.begin();
      for(int seg_no=0; iGrpId!=tckinGrpIds.end(); ++iGrpId, seg_no++)
      {
        const GrpId_t& grpId=*iGrpId;
        if (i==bag_concept_by_seg.end())
          throw EXCEPTIONS::Exception("%s: strange situation: i==bag_concept_by_seg.end()!", __FUNCTION__);
        if (i->first!=seg_no)
          throw EXCEPTIONS::Exception("%s: strange situation: i->first=%d seg_no=%d!", __FUNCTION__, i->first, seg_no);
        GrpQry.get().SetVariable("grp_id", grpId.get());
        GrpQry.get().SetVariable("piece_concept", (int)(i->second==TBagConcept::Piece));
        GrpQry.get().SetVariable("point_id", FNull);
        GrpQry.get().Execute();
        ++i;

        for(map<int/*seg_no*/,TBagConcept::Enum>::const_iterator j=i; j!=bag_concept_by_seg.end(); ++j)
        {
          TrferQry.get().SetVariable("grp_id", grpId.get());
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
          event.id3=grpId.get();
          reqInfo->LocaleToLog(event);
        };
      };
    }
    if (!isDoomedToWait() && rollbackGuaranteed)
    {
      ProgError(STDLOG, "Warning: !isDoomedToWait() && rollbackGuaranteed (grp_id=%d)", data.grpId);
      throw SomethingHasChanged();
    }
  }
  catch(const SomethingHasChanged&)
  {
    ProgTrace(TRACE5, "%s: SomethingHasChanged", __func__);
    throw UserException("MSG.FLT_OR_PAX_INFO_CHANGED.REFRESH_DATA");
  }
}

static void CloneServiceLists(xmlNodePtr segsNode, int numToClone)
{
    using namespace astra_api::xml_entities;
    xmlNodePtr passengersNode = findNodeR(segsNode, "passengers");
    int maxSegNo = -1, listIdCat1 = 0, listIdCat2 = 0;
    for(xmlNodePtr paxNode = passengersNode->children; paxNode != NULL;
        paxNode = paxNode->next)
    {
        xmlNodePtr svcListsNode = findNodeR(paxNode, "service_lists");
        if(svcListsNode) {
            const auto svcLists = XmlEntityReader::readServiceLists(svcListsNode);
            for(const auto& svcList: svcLists) {
                if(svcList.seg_no > maxSegNo) {
                    maxSegNo = svcList.seg_no;
                }

                if(svcList.category == 1) {
                    if(listIdCat1 && svcList.list_id != listIdCat1) {
                        LogError(STDLOG) << "Service list_ids of category 1 are not equal. "
                                         << listIdCat1 << " <> " << svcList.list_id;
                    }
                    listIdCat1 = svcList.list_id;
                }

                if(svcList.category == 2) {
                    if(listIdCat2 && svcList.list_id != listIdCat2) {
                        LogError(STDLOG) << "Service list_ids of category 2 are not equal. "
                                         << listIdCat2 << " <> " << svcList.list_id;
                    }
                    listIdCat2 = svcList.list_id;
                }
            }
        }
    }

    if(maxSegNo < 0) {
        tst();
        return;
    }
    for(xmlNodePtr paxNode = passengersNode->children; paxNode != NULL;
        paxNode = paxNode->next)
    {
        xmlNodePtr svcListsNode = findNodeR(paxNode, "service_lists");
        if(svcListsNode) {
            for(int i = 1; i <= numToClone; i++) {
                XmlEntityViewer::viewServiceList(svcListsNode,
                                                 XmlServiceList(maxSegNo + i, 1, listIdCat1));
                XmlEntityViewer::viewServiceList(svcListsNode,
                                                 XmlServiceList(maxSegNo + i, 2, listIdCat2));
            }
        }
    }
}

void CheckInInterface::LoadIatciPax(xmlNodePtr reqNode, xmlNodePtr resNode,
                                    const GrpId_t& grpId, bool needSync)
{
    LogTrace(TRACE3) << __FUNCTION__ << " grpId:" << grpId << "; needSync:" << needSync;

    std::string xmlData = iatci::IatciXmlDb::load(grpId);
    if(!xmlData.empty())
    {
//   ���������஢���, �.�. PLF ���� �� �����ন����
//        if(needSync && reqNode != NULL) {
//            IatciInterface::PasslistRequest(reqNode, grpId);
//            return AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR"); // TODO #25409
//        }
        XMLDoc xmlDoc = ASTRA::createXmlDoc(xmlData);

        xmlNodePtr srcSegsNode = findNodeR(xmlDoc.docPtr()->children, "segments");
        ASSERT(srcSegsNode != NULL);

        xmlNodePtr destSegsNode = findNodeR(resNode, "segments");
        int iatciSegsCount = 0;
        for(xmlNodePtr srcSegNode = srcSegsNode->children; srcSegNode != NULL;
            srcSegNode = srcSegNode->next, iatciSegsCount++)
        {
            CopyNode(destSegsNode, srcSegNode, true/*recursive*/);
        }

        // ������� �ନ���
        CloneServiceLists(destSegsNode, iatciSegsCount);
    }
}

void CheckInInterface::LoadPaxByGrpId(const GrpId_t& grpId, xmlNodePtr reqNode, xmlNodePtr resNode, bool afterSavePax)
{
  LogTrace(TRACE6) << "CheckInInterface::" << __func__
                   << ": grp_id=" << grpId;
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr node;
  TCkinRoute tckin_route;
  tckin_route.getRouteAfter(grpId,
                            TCkinRoute::NotCurrent,
                            TCkinRoute::OnlyDependent,
                            TCkinRoute::WithTransit);
  TCkinGrpIds tckinGrpIds = tckin_route.getTCkinGrpIds();
  tckinGrpIds.insert(tckinGrpIds.begin(), grpId);

  bool trfer_confirm=true;
  string pax_cat_airline;
  set<string> pax_cats;
  CheckIn::TTransferList segs;
  xmlNodePtr segsNode=NewTextChild(resNode,"segments");
  TBrands brands; //�����, �⮡� ���஢����� ������
  AstraLocale::OutputLang outputLang;
  for(TCkinGrpIds::const_iterator iGrpId=tckinGrpIds.begin(); iGrpId!=tckinGrpIds.end(); ++iGrpId)
  {
    const GrpId_t& grpId=*iGrpId;

    WeightConcept::AllPaxNormContainer all_norms;
    ostringstream norms_view;
    string used_norms_airline_mark;

    CheckIn::TPaxGrpItem grp;
    if (!grp.getByGrpIdWithBagConcepts(grpId.get()))
    {
      if (iGrpId==tckinGrpIds.begin() && !afterSavePax)
        throw AstraLocale::UserException("MSG.PAX_GRP_OR_LUGGAGE_NOT_CHECKED_IN");
      return; //�� �뢠�� ����� ࠧॣ������ �ᥩ ��㯯� �� �訡�� �����
    }

    if (iGrpId==tckinGrpIds.begin())
    {
      trfer_confirm=grp.trfer_confirm;
    }

    TTripInfo operFlt;
    operFlt.getByGrpId(grpId.get());

    CheckIn::TTransferItem seg;
    seg.operFlt=operFlt;
    seg.grp_id=grp.id;
    seg.airp_arv=grp.airp_arv;

    if (iGrpId==tckinGrpIds.begin() && !afterSavePax && !reqInfo->api_mode)
      tryChangeFlight(grp, reqNode, resNode);

    xmlNodePtr segNode=NewTextChild(segsNode,"segment");
    xmlNodePtr operFltNode=NewTextChild(segNode,"tripheader");
    TripsInterface::readOperFltHeader( operFlt, operFltNode );
    readTripData( grp.point_dep, segNode );

    if (grp.cl.empty() && grp.status==psCrew)
      grp.cl=EncodeClass(Y);  //crew compatible
    grp.toXML(segNode);
    //��த �����祭��
    NewTextChild(segNode,"city_arv");
    try
    {
      const TAirpsRow& airpsRow=(const TAirpsRow&)base_tables.get("airps").get_row("code",grp.airp_arv);
      const TCitiesRow& citiesRow=(const TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
      ReplaceTextChild( segNode, "city_arv", citiesRow.code );
    }
    catch(const EBaseTableError&) {}

    CheckIn::TGroupBagItem group_bag;
    TGrpServiceListWithAuto svc;
    if (iGrpId==tckinGrpIds.begin())
    {
      group_bag.fromDB(grp.id, ASTRA::NoExists, false);
      svc.fromDB(GrpId_t(grp.id));
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
      }

      TQuery PaxQry(&OraSession);
      PaxQry.Clear();
      PaxQry.SQLText=pax_sql;
      PaxQry.CreateVariable("grp_id",otInteger,grp.id);

      PaxQry.Execute();

      if (iGrpId==tckinGrpIds.begin())
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
        paxTrferItem.subclass=pax.getCabinSubclass();
        seg.pax.push_back(paxTrferItem);

        xmlNodePtr paxNode=NewTextChild(node, "pax");
        if (grp.status==psCrew)
        {
          pax.subcl=" ";       //crew compatible
          pax.tkn.no="-";      //crew compatible
          pax.tkn.rem="TKNA";  //crew compatible
        }
        pax.toXML(paxNode);

        std::string bagNormAndBrand;

        boost::optional<TBagQuantity> crsBagNorm=CheckIn::TSimplePaxItem::getCrsBagNorm<TQuery, string>(PaxQry, "crs_bag_norm", "crs_bag_norm_unit");

        TETickItem etick;
        if (pax.tkn.validET())
        {
          etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
          if (grp.pc)
          {
            brands.get(operFlt.airline, etick);
            bagNormAndBrand=brands.getSingleBrand().name(outputLang);
          }
        }

        bagNormAndBrand=trueBagNormView(crsBagNorm, etick, outputLang) + " " + bagNormAndBrand;

        NewTextChild(paxNode, "ticket_bag_norm", TrimString(bagNormAndBrand), "");
        NewTextChild(paxNode,"pr_norec",(int)PaxQry.FieldIsNULL("crs_pax_id"));

        bool pr_bp_print, pr_bi_print;
        prPrint.get_pr_print(grpId.get(), pax.id, pr_bp_print, pr_bi_print);
        NewTextChild(paxNode,"pr_bp_print",pr_bp_print);
        NewTextChild(paxNode,"pr_bi_print",pr_bi_print);
        if (iGrpId==tckinGrpIds.begin())
        {
          std::list<CheckIn::TPaxTransferItem> pax_trfer;
          CheckIn::PaxTransferFromDB(pax.id, pax_trfer);
          CheckIn::PaxTransferToXML(pax_trfer, paxNode);
          TPaxServiceLists().toXML(pax.id, false, tckinGrpIds.size(), tckin_route, NewTextChild(paxNode, "service_lists"));
        }
        PaxRemToXML(paxNode);
        if (iGrpId==tckinGrpIds.begin())
        {
          if (grp.wt)
          {
            WeightConcept::TPaxNormComplexContainer norms;
            WeightConcept::PaxNormsFromDB(NoExists, pax.id, norms);
            WeightConcept::NormsToXML(norms, paxNode);
            if (pax.refuse.empty())
              all_norms.insert(all_norms.end(), norms.begin(), norms.end());
          };
          Sirena::PaxBrandsNormsToStream(trfer, pax, norms_view);

          pax_cat_airline=seg.operFlt.airline;
          AddPaxCategory(pax, pax_cats);
        }
      }
    }
    else
    {
      //��ᮯ஢������� �����
      if (iGrpId==tckinGrpIds.begin())
      {
        trfer.GetRoute(grp.id, trtWithFirstSeg);
        BuildTransfer(trfer, trtNotFirstSeg, resNode);
        BuildTCkinSegments(grp.id, resNode);
        TPaxServiceLists().toXML(grp.id, true, tckinGrpIds.size(), tckin_route, NewTextChild(segNode, "service_lists"));
        if (grp.wt)
        {
          WeightConcept::TPaxNormComplexContainer norms;
          WeightConcept::GrpNormsFromDB(NoExists, grp.id, norms);
          WeightConcept::NormsToXML(norms, segNode);
          all_norms.insert(all_norms.end(), norms.begin(), norms.end());
        }
      }
    }
    if (iGrpId==tckinGrpIds.begin())
    {
      group_bag.toXML(resNode, grp.is_unaccomp());

      CheckIn::TServicePaymentListWithAuto payment;
      payment.fromDB(grp.id);
      payment.toXML(segNode);

      TPaidRFISCViewMap PaidRFISCViewMap;
      if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION) || grp.pc)
      {
        //��㣨 + pc
        svc.toXML(resNode);
        TPaidRFISCList PaidRFISCList;
        PaidRFISCList.fromDB(grp.id, true);
        TGrpServiceAutoList svcsAuto;
        svcsAuto.fromDB(GrpId_t(grp.id));

        CalcPaidRFISCView(TPaidRFISCListWithAuto().set(PaidRFISCList, svcsAuto), PaidRFISCViewMap);
        PaidRFISCViewToXML(PaidRFISCViewMap, resNode);
        NewTextChild(resNode, "norms_view", norms_view.str());

        CalcPaidRFISCView(TPaidRFISCListWithAuto().set(PaidRFISCList, svcsAuto, true), PaidRFISCViewMap);
      };

      //wt
      TPaidBagViewMap PaidBagViewMap;
      TPaidBagViewMap TrferBagViewMap;
      if (grp.wt)
      {
        WeightConcept::TPaidBagList paid;
        WeightConcept::PaidBagFromDB(NoExists, grp.id, paid);

        map<int/*id*/, TEventsBagItem> tmp_bag;
        GetBagToLogInfo(grp.id, tmp_bag);
        WeightConcept::TAirlines airlines(grp.id,
                                          used_norms_airline_mark.empty()?seg.operFlt.airline:used_norms_airline_mark,
                                          "CalcPaidBagView");
        WeightConcept::CalcPaidBagView(airlines, tmp_bag, all_norms, paid, payment, used_norms_airline_mark,
                                       PaidBagViewMap, TrferBagViewMap);
        WeightConcept::PaidBagViewToXML(PaidBagViewMap, TrferBagViewMap, resNode);
      };

      //⠡��窠 ���⭮��
      GridInfoToXML(trfer, PaidRFISCViewMap, PaidBagViewMap, TrferBagViewMap, resNode);

      readTripCounters(grp.point_dep, segNode);

      if (afterSavePax)
      {
        //⮫쪮 ��᫥ ����� ���������
        readTripCounters(grp.point_dep, resNode);
      }
    }

    readTripSets( grp.point_dep, operFlt, operFltNode );

    segs.push_back(seg);
  }
  if (!trfer_confirm)
  {
    //ᮡ�ࠥ� ���ଠ�� � �����⢥ত����� �࠭���
    if (!LoadUnconfirmedTransfer(segs, resNode) &&
        !reqInfo->desk.compatible(NOT_F9_WARNING_VERSION))
      showErrorMessage("MSG.BAGGAGE_INPUT_ONLY_AFTER_CHECKIN");
  }

  // �᫨ ���뫠��� iatci-����� �� checkin/update,
  // � LoadIatciPax �맮����� � kick-������,
  // ���� - �����
  if(reqNode != NULL && !ReqParams(reqNode).getBoolParam("was_sent_iatci"))
  {
      bool afterKick = ReqParams(reqNode).getBoolParam("after_kick", false);
      bool needSync = !afterKick && !reqInfo->api_mode && !afterSavePax;
      LoadIatciPax(reqNode, resNode, tckinGrpIds.back(), needSync);
  }
}

void CheckInInterface::LoadPax(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CheckInInterface::instance()->LoadPax(NULL, reqNode, resNode);
}

void CheckInInterface::PaxRemToXML(xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;
  xmlNodePtr node2=paxNode->children;
  int pax_id=NodeAsIntegerFast("pax_id",node2);

  LoadPaxRemAndASVC(pax_id, paxNode, false);
  LoadPaxFQT(pax_id, paxNode, false);
}

void CheckInInterface::SaveTCkinSegs(int grp_id,
                                     const CheckIn::SegmentMap &segments,
                                     const std::vector<CheckIn::IatciSegment>& iatciSegs,
                                     TLogLocale& tlocale)
{
  tlocale.lexema_id.clear();
  tlocale.prms.clearPrms();

  CheckIn::SegmentMap segs(segments);
  if (segs.empty() || segs.noMoreSeg()) return;

  segs.setNextSeg();

  if (!segs.noMoreSeg() || !iatciSegs.empty())
  {
      tlocale.lexema_id = "EVT.THROUGH_CHECKIN_PERFORMED";
      PrmEnum route("route", "");

      if(!segs.noMoreSeg())
      {
          TQuery Qry(&OraSession);
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

          for(int seg_no=1; !segs.noMoreSeg(); segs.setNextSeg(), seg_no++)
          {
            const CheckIn::Segment& segment=segs.get(PointId_t(NodeAsInteger("point_dep", segs.segNode())), __func__);

            const TTripInfo &fltInfo=segment.flt;
            TDateTime local_scd=UTCToLocal(fltInfo.scd_out,AirpTZRegion(fltInfo.airp));
            modf(local_scd,&local_scd);

            Qry.SetVariable("seg_no",seg_no);
            Qry.SetVariable("airline",fltInfo.airline);
            Qry.SetVariable("flt_no",fltInfo.flt_no);
            Qry.SetVariable("suffix",fltInfo.suffix);
            Qry.SetVariable("scd",local_scd);
            Qry.SetVariable("airp_dep",fltInfo.airp);
            Qry.SetVariable("airp_arv",segment.airpArv().get());
            Qry.SetVariable("pr_final",(int)(segs.segNode()->next==nullptr));
            Qry.Execute();
            if (Qry.GetVariableAsInteger("bind_flt")!=0)
            {
              int point_id_trfer=Qry.GetVariableAsInteger("point_id_trfer");
              TTrferBinding().bind_flt_by_point_id(point_id_trfer);
            }

            segment.formatToLog(route);
          }
      }

      for(auto iatciSeg: iatciSegs) {
          iatciSeg.formatToLog(route);
      }

      tlocale.prms << route;
  }
}

void CheckInInterface::SaveTransfer(int grp_id,
                                    const CheckIn::TTransferList &trfer,
                                    const map<int, pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs,
                                    int seg_no, TLogLocale& tlocale)
{
  tlocale.lexema_id.clear();
  tlocale.prms.clearPrms();

  trfer.check(grp_id, true, seg_no);

  map<int, pair<CheckIn::Segments, TTrferSetsInfo> >::const_iterator s=trfer_segs.find(seg_no);
  CheckIn::TTransferList::const_iterator firstTrfer=trfer.begin();
  for(;firstTrfer!=trfer.end()&&seg_no>1;firstTrfer++,seg_no--);

  const bool transfer_deleted = deleteTransfers(GrpId_t(grp_id));

  if (firstTrfer==trfer.end()) //��祣� �� �����뢠�� � ����
  {
    if (transfer_deleted) {
        tlocale.lexema_id = "EVT.CHECKIN.TRANSFER_BAGGAGE_CANCEL";
        return;
    }
    else return;
  }

  TQuery TrferQry(&OraSession);
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

  int trfer_num=1;
  PrmEnum route("route", "");
  for(CheckIn::TTransferList::const_iterator t=firstTrfer;t!=trfer.end();++t,trfer_num++)
  {
    //�஢�ਬ ࠧ�襭� �� ��ଫ���� �࠭���
    if (s==trfer_segs.end())
      throw EXCEPTIONS::Exception("CheckInInterface::SaveTransfer: wrong trfer_segs");
    if (!s->second.second.trfer_permit)
#ifdef XP_TESTING
        if(!inTestMode())
#endif//XP_TESTING
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
    }
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
      TTrferBinding().bind_flt_by_point_id(point_id_trfer);
    }

    tlocale.lexema_id = "EVT.CHECKIN.TRANSFER_BAGGAGE_REGISTER";
    route.prms << PrmSmpl<string>("", " -> ") << PrmFlight("", t->operFlt.airline, t->operFlt.flt_no, t->operFlt.suffix)
               << PrmSmpl<string>("", "/") << PrmDate("", t->operFlt.scd_out,"dd") << PrmSmpl<string>("", ":")
               << PrmElem<string>("",  etAirp, t->operFlt.airp) << PrmSmpl<string>("", "-") << PrmElem<string>("", etAirp, t->airp_arv);
  }
  tlocale.prms << route;
}

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
                 ElemIdToCodeNative(etAirline, t->operFlt.airline));

    NewTextChild(trferNode,"flt_no",t->operFlt.flt_no);

    if (!t->operFlt.suffix.empty())
      NewTextChild(trferNode,"suffix",
                   ElemIdToCodeNative(etSuffix, t->operFlt.suffix));
    else
      NewTextChild(trferNode,"suffix");

    //���
    DecodeDate(t->operFlt.scd_out,iYear,iMonth,iDay);
    NewTextChild(trferNode,"local_date",iDay);

    NewTextChild(trferNode,"airp_dep",
                 ElemIdToCodeNative(etAirp, t->operFlt.airp));

    NewTextChild(trferNode,"airp_arv",
                 ElemIdToCodeNative(etAirp, t->airp_arv));

    NewTextChild(trferNode,"city_dep",
                 base_tables.get("airps").get_row("code",t->operFlt.airp).AsString("city"));
    NewTextChild(trferNode,"city_arv",
                 base_tables.get("airps").get_row("code",t->airp_arv).AsString("city"));
  }
}

void CheckInInterface::BuildTCkinSegments(int grp_id, xmlNodePtr tckinNode)
{
  if (tckinNode==NULL) return;

  xmlNodePtr node=NewTextChild(tckinNode,"tckin_segments");

  DB::TQuery Qry(PgOra::getROSession({"TCKIN_SEGMENTS", "TRFER_TRIPS"}), STDLOG);
  Qry.SQLText =
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
  }
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
  }
  Qry.Close();
}

void CheckInInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  xmlNodePtr itemNode;

  DB::TQuery Qry(PgOra::getROSession("COUNTERS2"), STDLOG);
  Qry.SQLText =
     "SELECT point_arv, class, "
     "       (crs_ok - ok - jmp_ok) AS noshow, "
     "       (crs_tranzit - tranzit - jmp_tranzit) AS trnoshow, "
     "       (tranzit + ok + goshow) AS show, "
     "       (jmp_tranzit + jmp_ok + jmp_goshow) AS jmp_show, "
     "       jmp_nooccupy "
     "FROM counters2 "
     "WHERE point_dep = :point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();

  for(;!Qry.Eof;Qry.Next())
  {
    itemNode = NewTextChild( node, "item" );
    NewTextChild( itemNode, "point_arv", Qry.FieldAsInteger( "point_arv" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "noshow", Qry.FieldAsInteger( "noshow" ) );
    NewTextChild( itemNode, "trnoshow", Qry.FieldAsInteger( "trnoshow" ) );
    NewTextChild( itemNode, "show", Qry.FieldAsInteger( "show" ) );
    if (!TReqInfo::Instance()->desk.compatible(JMP_VERSION))
    {
      NewTextChild( itemNode, "free_ok", 0 );        //�� �ᯮ������
      NewTextChild( itemNode, "free_goshow", 0 );    //�� �ᯮ������
      NewTextChild( itemNode, "nooccupy", 0 );       //�� �ᯮ������
    };
    NewTextChild( itemNode, "jmp_show", Qry.FieldAsInteger( "jmp_show" ), 0 );
    NewTextChild( itemNode, "jmp_nooccupy", Qry.FieldAsInteger( "jmp_nooccupy" ), 0 );
  }

  int load_residue=getCommerceWeight( point_id, onlyCheckin, CWResidual );
  if (load_residue!=NoExists)
    NewTextChild(dataNode,"load_residue",load_residue);
  else
    NewTextChild(dataNode,"load_residue");
}

void CheckInInterface::readTripData(int point_id, xmlNodePtr dataNode)
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr itemNode,node;

  TAdvTripInfo operFlt;
  if (!operFlt.getByPointId(point_id, FlightProps(FlightProps::NotCancelled,
                                                  FlightProps::WithCheckIn)))
    throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TTripRoute route;
  route.GetRouteAfter(operFlt, trtNotCurrent, trtNotCancelled);

  node = NewTextChild( tripdataNode, "airps" );
  set<string> airps;
  for(const TTripRouteItem& r : route)
  {
    //�஢�ਬ �� �㡫�஢���� ����� ��ய��⮢ � ࠬ��� ������ ३�
    if (!airps.insert(r.airp).second) continue;

    try
    {
      const TAirpsRow& airpsRow=(const TAirpsRow&)base_tables.get("airps").get_row("code",r.airp);

      itemNode = NewTextChild( node, "airp" );
      NewTextChild( itemNode, "point_id", r.point_id );
      NewTextChild( itemNode, "airp_code", airpsRow.code );
      NewTextChild( itemNode, "city_code", airpsRow.city );

      ostringstream target_view;
      target_view << ElemIdToNameLong(etCity, airpsRow.city)
                  << " (" << ElemIdToCodeNative(etAirp, airpsRow.code) << ")";
      NewTextChild( itemNode, "target_view", target_view.str() );

      TCompleteAPICheckInfo(TPaxSegmentPair(point_id, r.airp)).toXML(NewTextChild(itemNode, "check_info"));
    }
    catch(const EBaseTableError&) {}
  }

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
    NewTextChild( itemNode, "class_view", ElemIdToNameLong(etClass, c->cls) );
    NewTextChild( itemNode, "cfg", c->cfg );
  }

  //��室� �� ��ᠤ��, �����祭�� �� ३�
  node = NewTextChild( tripdataNode, "gates" );
  vector<string> gates;
  TripsInterface::readGates(point_id, gates);
  for(vector<string>::iterator iv = gates.begin(); iv != gates.end(); iv++)
    NewTextChild( node, "gate_name", *iv);

  //����
  TripsInterface::readHalls(operFlt.airp, "�", tripdataNode);

  //��室� �� ��ᠤ�� ��� ������� �� ����� ॣ����樨, �����祭�� �� ३�
  DB::TQuery Qry(PgOra::getROSession({"TRIP_STATIONS", "STATIONS", "STATION_HALLS"}), STDLOG);
  Qry.SQLText =
    "SELECT name AS gate_name "
      "FROM stations "
      "JOIN trip_stations "
        "ON stations.desk = trip_stations.desk "
       "AND stations.work_mode = trip_stations.work_mode "
      "JOIN station_halls "
        "ON stations.airp = station_halls.airp "
       "AND stations.name = station_halls.station "
     "WHERE trip_stations.point_id = :point_id "
       "AND trip_stations.work_mode = :work_mode "
       "AND station_halls.hall = :hall";
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
    }
  }

  vector<TTripInfo> markFlts;
  GetMktFlights(operFlt,markFlts);
  //��⠢�� � ��砫� ���ᨢ� �������᪨� ३ᮢ 䠪��᪨� ३�
  TTripInfo markFlt;
  markFlt=operFlt;
  markFlt.scd_out=UTCToLocal(markFlt.scd_out, AirpTZRegion(markFlt.airp));
  markFlts.insert(markFlts.begin(),markFlt);

  if (!markFlts.empty())
  {
    TCodeShareSets codeshareSets;
    node = NewTextChild( tripdataNode, "mark_flights" );
    for(vector<TTripInfo>::iterator f=markFlts.begin();f!=markFlts.end();f++)
    {
      xmlNodePtr markFltNode=NewTextChild(node,"flight");

      NewTextChild(markFltNode,"airline",f->airline);
      NewTextChild(markFltNode,"flt_no",f->flt_no);
      NewTextChild(markFltNode,"suffix",f->suffix);
      NewTextChild(markFltNode,"scd",DateTimeToStr(f->scd_out));
      NewTextChild(markFltNode,"airp_dep",f->airp);

      if (f==markFlts.begin())
        NewTextChild(markFltNode,"pr_mark_norms",(int)false);
      else
      {
        codeshareSets.get(operFlt,*f);
        NewTextChild(markFltNode,"pr_mark_norms",(int)codeshareSets.pr_mark_norms);
      }
    }
  }
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
}

void CheckInInterface::readTripSets( int point_id,
                                     const TTripInfo &fltInfo,
                                     xmlNodePtr tripSetsNode )
{
    DB::TQuery Qry(PgOra::getROSession("TRIP_SETS"), STDLOG);
    Qry.SQLText=
        "SELECT pr_etstatus FROM trip_sets WHERE point_id=:point_id ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

    TTripSetList setList;
    setList.fromDB(point_id);
    if (setList.empty()) throw UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");

    NewTextChild( tripSetsNode, "pr_etl_only", (int)GetTripSets(tsETSNoExchange,fltInfo) );
    NewTextChild( tripSetsNode, "pr_etstatus", Qry.FieldAsInteger("pr_etstatus") );
    NewTextChild( tripSetsNode, "pr_no_ticket_check", (int)GetTripSets(tsNoTicketCheck,fltInfo) );
    NewTextChild( tripSetsNode, "pr_auto_pt_print", (int)GetTripSets(tsAutoPTPrint,fltInfo) );
    NewTextChild( tripSetsNode, "pr_auto_pt_print_reseat", (int)GetTripSets(tsAutoPTPrintReseat,fltInfo) );
    NewTextChild( tripSetsNode, "use_jmp", (int)setList.value<bool>(tsUseJmp) );
    NewTextChild( tripSetsNode, "pr_payment_at_desk", isPaymentAtDesk(point_id) );
}

void CheckInInterface::GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  readTripCounters(point_id,resNode);
}


//////////// ��� ��⥬� ���ନ஢����
void CheckInInterface::OpenDESKInfo(xmlNodePtr reqNode,
                                    xmlNodePtr resNode,
                                    const std::string& work_mode)
{
  int open = NodeAsInteger("Open",reqNode);
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace( TRACE5, "Open=%d", open );
  DB::TQuery Qry(PgOra::getRWSession("TRIP_STATIONS"), STDLOG);
  Qry.SQLText =
   "UPDATE trip_stations "
      "SET start_time = CASE :open WHEN 1 THEN :utc ELSE NULL END "
    "WHERE point_id = :point_id "
      "AND desk = :desk "
      "AND work_mode = :work_mode";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "open", otInteger, open );
  Qry.CreateVariable( "utc", otDate, NowUTC() );
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "work_mode", otString, work_mode );
  Qry.Execute();
  if ( open == 1 )
    TReqInfo::Instance()->LocaleToLog("EVT.CHECKIN_OPEN_FOR_INFO_SYS", evtFlt, point_id);
  else
    TReqInfo::Instance()->LocaleToLog("EVT.CHECKIN_CLOSE_FOR_INFO_SYS", evtFlt, point_id);
}

void CheckInInterface::OpenCheckInInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  CheckInInterface::OpenDESKInfo( reqNode, resNode, "�" );
}

void CheckInInterface::GetTCkinFlights(const TTripInfo &operFlt,
                                       const map<int, CheckIn::TTransferItem> &trfer,
                                       map<int, pair<CheckIn::TTransferItem, CheckIn::Segments> > &segs)
{
  segs.clear();
  if (trfer.empty()) return;

  std::string             operAirl = operFlt.airline;
  Ticketing::FlightNum_t operFlNum = Ticketing::getFlightNum(operFlt.flt_no);

  bool is_edi=false;
  for(map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin();t!=trfer.end();t++)
  {
    CheckIn::Segments seg;

    if (t->second.Valid())
    {
      //�஢�ਬ ���㦨������ �� ३� � ��㣮� DCS
      if (!is_edi)
      {
        std::string currOperAirl = t->second.operFlt.airline;
        Ticketing::FlightNum_t currOperFlNum = Ticketing::getFlightNum(t->second.operFlt.flt_no);

        std::unique_ptr<DcsSystemContext> dcs(DcsSystemContext::read(currOperAirl,
                                                                     currOperFlNum,
                                                                     operAirl,
                                                                     operFlNum,
                                                                     false));
        if(dcs) is_edi=true;
      }

      seg.is_edi=is_edi;

      if (!is_edi)
      {
        FltOperFilter filter(AirlineCode_t(t->second.operFlt.airline),
                             FlightNumber_t(t->second.operFlt.flt_no),
                             FlightSuffix_t(t->second.operFlt.suffix),
                             AirportCode_t(t->second.operFlt.airp),
                             t->second.operFlt.scd_out,
                             FltOperFilter::DateType::Local,
                             {},
                             FlightProps(FlightProps::WithCancelled, FlightProps::WithCheckIn));

        //�饬 ३� � ���
        list<TAdvTripInfo> flts=filter.search();

        for(list<TAdvTripInfo>::const_iterator f=flts.begin(); f!=flts.end(); ++f)
        {
          boost::optional<CheckIn::Segment> segSPPInfo=
              CheckIn::Segment::fromDB(PointId_t(f->point_id),
                                       AirportCode_t(f->airp),
                                       boost::none,
                                       AirportCode_t(t->second.airp_arv));

          if (!segSPPInfo) continue; //�� ��諨 �� point_dep
          seg.segs.push_back(segSPPInfo.get());
        }
      }
    }
    segs[t->first]=make_pair(t->second, seg);
  }
}

CheckInInterface* CheckInInterface::instance()
{
    static CheckInInterface* inst = 0;
    if(!inst) {
        inst = new CheckInInterface();
    }
    return inst;
}


class TCkinSegmentItem : public CheckIn::Segments
{
  public:
    bool conf_status;
    string calc_status;
};

static void ParseTCkinSegmentItems(xmlNodePtr trferNode,
                                   std::vector< std::pair<TCkinSegmentItem, TTrferSetsInfo> >& segs)
{
    for(trferNode=trferNode->children;trferNode!=NULL;trferNode=trferNode->next)
    {
      xmlNodePtr node2=trferNode->children;

      TCkinSegmentItem segItem;
      segItem.conf_status=(NodeAsIntegerFast("conf_status",node2,0)!=0);
      segItem.calc_status=NodeAsStringFast("calc_status",node2,"");
      segs.push_back(make_pair(segItem,TTrferSetsInfo()));
    }
}

static void CheckTransfer(const CheckIn::TTransferList& trfer,
                          const std::map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> >& trfer_segs,
                          const std::vector< std::pair<TCkinSegmentItem, TTrferSetsInfo> >& tckin_segs)
{
    if (tckin_segs.size() != trfer_segs.size() ||
        tckin_segs.size() != trfer.size())
    {
        throw EXCEPTIONS::Exception("CheckInInterface::CheckTCkinRoute: different array sizes "
                                    "(segs.size()=%zu, trfer_segs.size()=%zu, trfer.size()=%zu",
                                     tckin_segs.size(), trfer_segs.size(), trfer.size());
    }
}

static std::string TransferItem2Str(const CheckIn::TTransferItem& f)
{
    std::ostringstream flight;
    flight << ElemIdToCodeNative(etAirline, f.operFlt.airline)
           << setw(3) << setfill('0') << f.operFlt.flt_no
           << ElemIdToCodeNative(etSuffix, f.operFlt.suffix) << "/"
           << DateTimeToStr(f.operFlt.scd_out,"dd") << " "
           << ElemIdToCodeNative(etAirp, f.operFlt.airp) << "-"
           << ElemIdToCodeNative(etAirp, f.airp_arv);
    return flight.str();
}

static std::vector<TCkinPaxInfo> GetRequestPaxes(xmlNodePtr paxNode, const CheckIn::TTransferItem& f)
{
    vector<TCkinPaxInfo> vPax;
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
      paxInfo.subclass=f.pax.at(pax_no-1).subclass;

      for(iPax=vPax.begin();iPax!=vPax.end();iPax++)
      {
        if (transliter_equal(paxInfo.surname,iPax->surname) &&
            transliter_equal(paxInfo.name,iPax->name) &&
            paxInfo.pers_type==iPax->pers_type &&
            paxInfo.seats==iPax->seats &&
            paxInfo.subclass==iPax->subclass)
        {
          iPax->reqCount++;
          break;
        }
      }
      if (iPax==vPax.end()) vPax.push_back(paxInfo);
    }

    return vPax;
}

static void FillEdiTCkinPaxesNode(xmlNodePtr reqNode, xmlNodePtr tckinPaxesNode,
                                  std::vector<TCkinPaxInfo>& vPax,
                                  const CheckIn::TTransferItem& f)
{
    std::vector<TCkinPaxInfo>::iterator iPax = vPax.begin();
    int paxId = 0;
    for(iPax=vPax.begin(); iPax != vPax.end(); iPax++)
    {
        for(int i=0; i < iPax->reqCount; ++i)
        {
            iPax->node = NewTextChild(tckinPaxesNode,"tckin_pax");
            CreateEdiTCkinResponse(f, *iPax, --paxId);


            ProgTrace(TRACE5,"<<<<< subclass=%s pers_type=%s surname=%s name=%s seats=%d",
                             iPax->subclass.c_str(),
                             iPax->pers_type.c_str(),
                             iPax->surname.c_str(),
                             iPax->name.c_str(),
                             iPax->seats);
        }
    }
}

static TTripInfo makeTripInfo(const CheckIn::TTransferItem& f)
{
    TTripInfo tripInfo = f.operFlt;
    return tripInfo;
}

void CheckInInterface::CheckTCkinRoute(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  CheckIn::Segment firstSeg=CheckIn::Segment::fromDB(reqNode);

  xmlNodePtr trferNode=NodeAsNode("transfer",reqNode);
  CheckIn::TTransferList trfer;
  map<int, pair<CheckIn::Segments, TTrferSetsInfo> > trfer_segs;
  trfer.parseSegments(trferNode, firstSeg.airpArv(), firstSeg.scdOutLocal());
  trfer.parseSubclasses(NodeAsNode("passengers",reqNode));
  if (!trfer.empty())
  {
    map<int, CheckIn::TTransferItem> trfer_tmp;
    int trfer_num=1;
    for(CheckIn::TTransferList::const_iterator t=trfer.begin();t!=trfer.end();++t, trfer_num++)
      trfer_tmp[trfer_num]=*t;
    traceTrfer(TRACE5, "CheckTCkinRoute: trfer_tmp", trfer_tmp);
    GetTrferSets(firstSeg.flt, firstSeg.airpArv().get(), "", trfer_tmp, false, trfer_segs);
    traceTrfer(TRACE5, "CheckTCkinRoute: trfer_segs", trfer_segs);
  }

  xmlNodePtr routeNode=NewTextChild(resNode,"tckin_route");
  xmlNodePtr segsNode=NewTextChild(resNode,"tckin_segments");

  vector< pair<TCkinSegmentItem, TTrferSetsInfo> > segs;
  ParseTCkinSegmentItems(trferNode, segs);

  CheckTransfer(trfer, trfer_segs, segs);

  std::vector< pair<TCkinSegmentItem, TTrferSetsInfo> >::iterator s=segs.begin();
  std::map<int, pair<CheckIn::Segments, TTrferSetsInfo> >::const_iterator s2=trfer_segs.begin();
  for(;s!=segs.end() && s2!=trfer_segs.end();s++,s2++)
  {
    s->first.segs.assign(s2->second.first.segs.begin(),s2->second.first.segs.end());
    s->first.is_edi=s2->second.first.is_edi;
    s->second=s2->second.second;
  }

  bool irrelevant_data=false; //���ॢ訥 �����
  bool total_permit=true;
  bool tckin_route_confirm=true;

  bool edi_seg_found = false;

  //横� �� ��몮���� ᥣ���⠬ � �� �࠭���� ३ᠬ
  CheckIn::TTransferList::const_iterator f=trfer.begin();
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
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",0/*false*/);
    }
    else
    {
      SetProp(NewTextChild(segNode,"trfer_permit","+"),"value",1/*true*/);
      if (tckin_route_confirm)
        SetProp(NewTextChild(seg2Node,"trfer_permit"),"value",1/*true*/);
    }

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
    }

    //�뢮� ३�
    std::string flightStr = TransferItem2Str(*f);

    if (!s->first.segs.empty())
    {
      if (s->first.segs.size()>1)
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
        const CheckIn::Segment &currSeg=*(s->first.segs.begin());

        NewTextChild(segNode,"flight",flightStr);
        if (tckin_route_confirm)
        {
          xmlNodePtr operFltNode=NewTextChild(seg2Node,"tripheader");
          TripsInterface::readOperFltHeader( currSeg.flt, operFltNode );
          readTripData( currSeg.flt.point_id, seg2Node );

          NewTextChild( seg2Node, "point_dep", currSeg.pointDep().get());
          NewTextChild( seg2Node, "airp_dep",  currSeg.airpDep().get());
          NewTextChild( seg2Node, "point_arv", currSeg.pointArv().get());
          NewTextChild( seg2Node, "airp_arv",  currSeg.airpArv().get());
          try
          {
            const TAirpsRow& airpsRow=(const TAirpsRow&)base_tables.get("airps").get_row("code",currSeg.airpArv().get());
            const TCitiesRow& citiesRow=(const TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
            NewTextChild( seg2Node, "city_arv_code", citiesRow.code );
          }
          catch(const EBaseTableError&) {}
        }

        //���⠥� ������
        TCFG cfg(currSeg.flt.point_id);
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
        }

        DB::TQuery CrsQry(PgOra::getROSession({"TLG_BINDING","CRS_PNR","CRS_PAX,PAX"}), STDLOG);
        getTCkinSearchPaxQuery(CrsQry);
        //����� ���ᠦ�஢
        CrsQry.SetVariable("point_id", currSeg.pointDep().get());
        CrsQry.SetVariable("airp_arv", currSeg.airpArv().get());

        xmlNodePtr paxNode=NodeAsNode("passengers",reqNode)->children;
        xmlNodePtr pax2Node=NewTextChild(segNode,"temp_passengers");
        int paxCount=0,paxCountInPNL=0;
        ClassesList origCls, cabinCls;
        CheckIn::AvailableByClasses availableByClasses;
        bool doublePax=false;
        std::vector<TCkinPaxInfo> pax = GetRequestPaxes(paxNode, *f);
        //���ᨢ pax �� ����� ������ ᮤ�ন� ���ᠦ�஢ � 㭨����묨:
        //1. 䠬�����,
        //2. ������
        //3. ⨯�� ���ᠦ��
        //4. ����稥�/������⢨�� ����
        //5. �������ᮬ
        //pax.reqCount ᮤ�ন� ���-�� ����������� ���ᠦ�஢


        std::vector<TCkinPaxInfo>::iterator iPax;
        for(iPax=pax.begin();iPax!=pax.end();iPax++)
        {
          paxCount+=iPax->reqCount;

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

          std::string cabinClassForAvailability;
          if (!CrsQry.Eof)
          {
            cabinClassForAvailability=CrsQry.FieldAsString("cabin_class");
            if (tckin_route_confirm)
            {
              //�� 㦥 ��᫥ ���⢥ত���� �������
              iPax->resCount=CreateSearchResponse(currSeg.flt.point_id,fetchSearchPaxResults(CrsQry),iPax->node);
              if (iPax->resCount>0)
              {
                for(xmlNodePtr tripNode=NodeAsNode("trips",iPax->node)->children;
                               tripNode!=NULL;
                               tripNode=tripNode->next)
                  for(xmlNodePtr pnrNode=NodeAsNode("groups",tripNode)->children;
                                 pnrNode!=NULL;
                                 pnrNode=pnrNode->next)
                  {
                    origCls.add(NodeAsString("class",pnrNode));
                    cabinCls.add(NodeAsString("cabin_class",pnrNode));
                  }
              }
            }
            else
            {
              iPax->resCount=0;
              for(;!CrsQry.Eof;CrsQry.Next())
              {
                iPax->resCount++;
                origCls.add(CrsQry.FieldAsString("class"));
                cabinCls.add(CrsQry.FieldAsString("cabin_class"));
              }
            }
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
              "      crs_pnr.subclass=:subclass "
              "ORDER BY class";
            Qry.CreateVariable("point_id",otInteger,currSeg.flt.point_id);
            Qry.CreateVariable("subclass",otString,iPax->subclass);
            Qry.Execute();
            if (!Qry.Eof)
            {
              cabinClassForAvailability=Qry.FieldAsString("class");
              for(;!Qry.Eof;Qry.Next())
              {
                origCls.add(Qry.FieldAsString("class"));
                cabinCls.add(Qry.FieldAsString("class"));
              }
            }
            else
            {
              try
              {
                const TSubclsRow& row=(const TSubclsRow&)base_tables.get("subcls").get_row("code/code_lat",iPax->subclass);
                cabinClassForAvailability=row.cl;
                origCls.add(row.cl);
                cabinCls.add(row.cl);
              }
              catch(const EBaseTableError&) {}
            }
          }
          if (!cabinClassForAvailability.empty())
            availableByClasses.add(cabinClassForAvailability, false, iPax->seats * iPax->reqCount);


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
          }
        }

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

            std::vector<TCkinPaxInfo>::iterator iPax;
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
                }
                break;
              }
            }
          }
        }

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


        if (origCls.strictlyOneClass())
        {
          if (origCls==cabinCls)
            NewTextChild(segNode,"class",origCls.view());
          else
            NewTextChild(segNode,"class",origCls.view()+"->"+cabinCls.view());

          //����襬 ����� �᫨ ���⢥ত���� �������
          if (tckin_route_confirm)
          {
            try
            {
              const TClassesRow& classesRow=(const TClassesRow&)base_tables.get("classes").get_row("code",origCls.getStrictlyOneClass());
              NewTextChild( seg2Node, "class_code", classesRow.code );
            }
            catch(const EBaseTableError&) {}
          }
        }
        else
        {
          SetProp(NewTextChild(segNode,"class", origCls.noClasses()?AstraLocale::getLocaleText("���"):origCls.view()),"error","CRITICAL");
        }

        //����稥 ����
        if (!currSeg.flt.match(FlightProps::NotCancelled))
        {
          SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("���� �⬥���")),"error","CRITICAL"); //???
        }
        else
          if (currSeg.route.empty())
          {
            //�� ��諨 �㭪� � �������
            SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("��� �/�")),"error","CRITICAL");
          }
          else
            if (origCls.strictlyOneClass())
            {
              bool free_seating=SALONS2::isFreeSeating(currSeg.flt.point_id);

              //��⠥� ���-�� ᢮������ ����
              //(��᫥ ���᪠ ���ᠦ�஢, ⠪ ��� ����室��� ��।����� ������ �����)

              CheckIn::CheckCounters(currSeg.pointDep().get(),
                                     currSeg.pointArv().get(),
                                     psTCheckin,
                                     cfg,
                                     free_seating,
                                     availableByClasses);

              int needSum, availSum;
              availableByClasses.getSummaryResult(needSum, availSum);
              if (availSum<needSum)
              {
                xmlNodePtr wlNode;
                if (availSum<=0)
                  wlNode=NewTextChild(segNode,"free",AstraLocale::getLocaleText("���"));
                else
                  wlNode=NewTextChild(segNode,"free",
                                      IntToString(availSum)+" "+AstraLocale::getLocaleText("��")+" "+IntToString(needSum));
                SetProp(wlNode,"error","WL");
                SetProp(wlNode,"wl_type","O");
              }
              else
                NewTextChild(segNode,"free","+");
            }
            else
              SetProp(NewTextChild(segNode,"free",AstraLocale::getLocaleText("����।. ��.")),"error","CRITICAL");

      }
    }
    else
    {
      //�� ��諨 � ��� ३�, ᮮ⢥�����騩 �࠭��୮�� ᥣ�����
      NewTextChild(segNode,"flight",flightStr);

      if(s->first.is_edi && tckin_route_confirm)
      {
          edi_seg_found = true;
          // TODO ��������� ᮤ�ন��� ������� �࠭��୮�� ᥣ����
          xmlNodePtr operFltNode=NewTextChild(seg2Node,"tripheader");

          // ������塞 ᮤ�ন��� tripheader

          TTripInfo tripInfo = makeTripInfo(*f);
          std::ostringstream tripName;
          tripName << GetTripName(tripInfo, ecNone, true, false) << " (EDI)";

          // ३�
          NewTextChild(operFltNode, "flight", tripName.str());
          // ���
          NewTextChild(operFltNode, "airline", f->operFlt.airline);
          // ������ ��� ���
          const TAirlinesRow &row = (const TAirlinesRow&)base_tables.get("airlines").get_row("code",f->operFlt.airline);
          NewTextChild(operFltNode, "aircode", row.aircode);
          // ����� ३�
          NewTextChild(operFltNode, "flt_no", f->operFlt.flt_no);
          // ���䨪�
          NewTextChild(operFltNode, "suffix", f->operFlt.suffix);
          // ��ய���
          NewTextChild(operFltNode, "airp", f->operFlt.airp);
          // �����쭠� ��� �뫥�
          NewTextChild(operFltNode, "scd_out_local", DateTimeToStr(UTCToLocal(f->operFlt.scd_out, AirpTZRegion(f->operFlt.airp))));
          // �ਧ���� (��� ᪢������ ������� �� �⮬ �⠯� �� �� ���??)
          NewTextChild(operFltNode, "pr_etl_only", 1);        // TODO
          NewTextChild(operFltNode, "pr_etstatus", 0);        // TODO
          NewTextChild(operFltNode, "pr_no_ticket_check", 1); // TODO
          NewTextChild(operFltNode, "pr_auto_pt_print", 0);   // TODO
          NewTextChild(operFltNode, "pr_auto_pt_print_reseat", 0); // TODO
          NewTextChild(operFltNode, "use_jmp", 0); // TODO
          NewTextChild(operFltNode, "pr_payment_at_desk", 0);

          xmlNodePtr tripdataNode = NewTextChild(seg2Node, "tripdata");
          xmlNodePtr node = NewTextChild(tripdataNode, "airps");

          xmlNodePtr airpNode = NewTextChild(node, "airp");
          NewTextChild(airpNode, "point_id", -1);

          xmlNodePtr checkInfoNode = NewTextChild(airpNode, "check_info");
          NewTextChild(checkInfoNode, "pass");
          NewTextChild(checkInfoNode, "crew");

          //xmlNodePtr classNode = NewTextChild(classesNode, "class");
          //NewTextChild(classNode, "code", EncodeClass(Y)); // TODO
          //NewTextChild(classNode, "class_view", "������"); // TODO
          //NewTextChild(classNode, "cfg", -1); // TODO

          node = NewTextChild(tripdataNode, "classes"); // ???
          node = NewTextChild(tripdataNode, "gates");   // ???
          node = NewTextChild(tripdataNode, "halls");   // ???

          // ���ଠ�� � �������᪮� ३� ����� �� ������
          /*node = NewTextChild(tripdataNode, "mark_flights");
          xmlNodePtr markFlightNode = NewTextChild(node, "flight");
          NewTextChild(markFlightNode, "airline", f->operFlt.airline);
          NewTextChild(markFlightNode, "flt_no",  f->operFlt.flt_no);
          NewTextChild(markFlightNode, "suffix",  f->operFlt.suffix);
          NewTextChild(markFlightNode, "scd",     DateTimeToStr(f->operFlt.scd_out));
          NewTextChild(markFlightNode, "airp_dep",f->operFlt.airp);
          NewTextChild(markFlightNode, "pr_mark_norms", 0); // ???*/

          NewTextChild(seg2Node, "point_dep", -1);
          NewTextChild(seg2Node, "airp_dep", f->operFlt.airp);
          NewTextChild(seg2Node, "point_arv", -1);
          NewTextChild(seg2Node, "airp_arv", f->airp_arv);

          try
          {
              const TAirpsRow& airpsRow=(const TAirpsRow&)base_tables.get("airps").get_row("code",f->airp_arv);
              const TCitiesRow& citiesRow=(const TCitiesRow&)base_tables.get("cities").get_row("code",airpsRow.city);
              NewTextChild(seg2Node, "city_arv_code", citiesRow.code);

              std::ostringstream target_view;
              target_view << ElemIdToNameLong(etCity, airpsRow.city)
                          << " (" << ElemIdToCodeNative(etAirp, airpsRow.code) << ")";
              NewTextChild(airpNode, "target_view", target_view.str() );

              NewTextChild(airpNode, "airp_code", airpsRow.code);
              NewTextChild(airpNode, "city_code", airpsRow.city);
          }
          catch(const EBaseTableError&) {}

          std::vector<TCkinPaxInfo> vPax = GetRequestPaxes(NodeAsNode("passengers",reqNode)->children, *f);
          xmlNodePtr tckinPaxesNode = NewTextChild(seg2Node, "tckin_passengers");
          FillEdiTCkinPaxesNode(reqNode, tckinPaxesNode, vPax, *f);

          if (!vPax.empty())
          try
          {
            const TSubclsRow& row=(const TSubclsRow&)base_tables.get("subcls").get_row("code/code_lat",vPax.front().subclass);
            NewTextChild(seg2Node, "class_code", row.cl);
          }
          catch(const EBaseTableError&) {}
      }
      else
      {
          if (!s->first.is_edi)
              SetProp(NewTextChild(segNode,"classes",AstraLocale::getLocaleText("��� � ���")),"error","CRITICAL");
          else {
              NewTextChild(segNode,"classes","EDIFACT"); //�� ���� �訡���
          }
          NewTextChild(segNode,"pnl");
          NewTextChild(segNode,"class");
          NewTextChild(segNode,"free");
      }
    }

    if (!s->second.tckin_permit && !s->first.is_edi) total_permit=false;
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
        }
        if (error=="WL" && s->second.tckin_waitlist)
        {
          total_waitlist=true;
          wl_type=NodeAsString("@wl_type",node);
        }
      }

    if (tckin_route_confirm)
    {
      if (total_permit && total_waitlist)
        NewTextChild(seg2Node,"wl_type",wl_type);
      xmlNodePtr operFltNode=NodeAsNode("tripheader",seg2Node);

      if (s->first.segs.size()==1)
        readTripSets( s->first.segs.begin()->flt.point_id, s->first.segs.begin()->flt, operFltNode );
    }

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
      }
    }
    else
    {
      NewTextChild(segNode,"total",AstraLocale::getLocaleText("���"));
      NewTextChild(segNode,"calc_status","NONE");
    }

    if (NodeAsString("calc_status",segNode) != s->first.calc_status)
      //����� ���५� �� �ࠢ����� � �।��騬 ����ᮬ ᪢������ �������
      irrelevant_data=true;

    if(edi_seg_found) {
        LogTrace(TRACE3) << "only one edifact segment possible here...break";
        break;
    }
  }
  if (irrelevant_data)
  {
    //����� ���५�
    xmlUnlinkNode(segsNode);
    xmlFreeNode(segsNode);
    return;
  }
  //����� �� ���५� - ���� ���뢠�� ���஡�� ����� ���ᠦ�஢ �� ᥣ���⠬




  xmlUnlinkNode(routeNode);
  xmlFreeNode(routeNode);

}

void CheckInInterface::ParseScanDocData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw AstraLocale::UserException("MSG.DEVICE.INVALID_SCAN_FORMAT");
}

void CheckInInterface::CrewCheckin(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr flightNode = NodeAsNode("FLIGHT", reqNode);

    AirportCode_t airpDep(airp_fromXML(NodeAsNode("AIRP_DEP", flightNode), cfErrorIfEmpty, __FUNCTION__, "MERIDIAN"));

    FltOperFilter filter(AirlineCode_t(airl_fromXML(NodeAsNode("AIRLINE", flightNode), cfErrorIfEmpty, __FUNCTION__, "MERIDIAN")),
                         FlightNumber_t(flt_no_fromXML(NodeAsString("FLT_NO", flightNode), cfErrorIfEmpty)),
                         FlightSuffix_t(suffix_fromXML(NodeAsString("SUFFIX", flightNode,""))),
                         airpDep,
                         scd_out_fromXML(NodeAsString("SCD", flightNode), "dd.mm.yyyy"),
                         FltOperFilter::DateType::Local,
                         {},
                         FlightProps(FlightProps::WithCancelled, FlightProps::WithCheckIn));

    TDateTime checkDate = UTCToLocal(NowUTC(), AirpTZRegion(airpDep.get())); // for HTTP

    //�饬 ३� � ���
    list<TAdvTripInfo> flts=filter.search();
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
        flightsForLock.Lock(__FUNCTION__);

        TMultiPnrDataSegs multiPnrDataSegs;
        TMultiPnrData& multiPnrData=multiPnrDataSegs.add(flt.point_id, true, false);

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

        for(xmlNodePtr crew_group = crew_groups->children; crew_group != NULL; crew_group = crew_group->next)
        {
            multiPnrData.segs.clear();

            // for HTTP
            string airp_arv = airp_fromXML(NodeAsNode("AIRP_ARV", crew_group), cfErrorIfEmpty, __FUNCTION__, "MERIDIAN");
            TCompleteAPICheckInfo checkInfo(TPaxSegmentPair(flt.point_id, airp_arv));

            xmlNodePtr crew_members = NodeAsNode("CREW_MEMBERS", crew_group);
            TWebPaxForSaveSeg seg(multiPnrData.flt.oper.point_id);
            seg.paxForCkin.status = psCrew;
            seg.paxFromReq.emplace_back();
            for(xmlNodePtr crew_member = crew_members->children; crew_member != NULL; crew_member = crew_member->next)
            {
                if(NodeAsString("DUTY", crew_member, "") == string("PIC") && NodeAsInteger("ORDER", crew_member, ASTRA::NoExists) == 1)
                {
                    if (!commander.empty())
                        throw AstraLocale::UserException("MSG.CHECK_XML_COMMANDER_DUPLICATED",
                                                   LEvntPrms() << PrmSmpl<string>("fieldname", "DUTY"));
                    is_commander = true;
                }
                TWebPaxForCkin paxForCkin;
                paxForCkin.initFromMeridian(airp_arv, CrewTypes().decode(NodeAsString("CREW_TYPE", crew_member, "")));
                ASTRA::TPaxTypeExt pax_type_ext(ASTRA::psCrew, paxForCkin.crew_type); // for HTTP
                switch (paxForCkin.crew_type)
                {
                  case ASTRA::TCrewType::Crew2:
                  case ASTRA::TCrewType::Crew4:
                  case ASTRA::TCrewType::Crew5:
                  case ASTRA::TCrewType::Crew3:
                    cabin++;
                    break;
                  case ASTRA::TCrewType::Crew1:
                    cockpit++;
                    break;
                  default:
                    throw AstraLocale::UserException("MSG.CHECK_XML_INVALID_CREW_TYPE", LEvntPrms() << PrmSmpl<string>("fieldname", "CREW_TYPE"));
                }

                xmlNodePtr pers_data = NodeAsNode("PERSONAL_DATA", crew_member);

                // for HTTP
                string full_name = NodeAsString("DOCS/SURNAME", pers_data, "") + string(" ") + NodeAsString("DOCS/FIRST_NAME", pers_data, "");
                full_name = TrimString(full_name);

                for (xmlNodePtr document = pers_data->children; document != NULL; document = document->next)
                {
                    if (document->children==nullptr) continue;
                    if (strcmp((const char*)document->name,"DOCS") == 0)
                    {
                        if (paxForCkin.apis.isPresent(apiDoc))
                            throw AstraLocale::UserException("MSG.SECTION_DUPLICATED",
                                                              LEvntPrms() << PrmSmpl<string>("name", "DOCS"));

                        CheckIn::TPaxDocItem doc;
                        doc.fromMeridianXML(document);
                        if (is_commander)
                        {
                          commander=doc.getSurnameWithInitials();
                          is_commander = false;
                        }

                        doc = NormalizeDocHttp(doc, full_name);
                        CheckDocHttp(doc, pax_type_ext, doc.surname, checkInfo, checkDate, full_name);
                        paxForCkin.setFromMeridian(doc);
                    }
                    else if (strcmp((const char*)document->name,"DOCO") == 0)
                    {
                        if (paxForCkin.apis.isPresent(apiDoco))
                            throw AstraLocale::UserException("MSG.SECTION_DUPLICATED",
                                                              LEvntPrms() << PrmSmpl<string>("name", "DOCO"));

                        CheckIn::TPaxDocoItem doco;
                        doco.fromMeridianXML(document);

                        doco = NormalizeDocoHttp(doco, full_name);
                        CheckDocoHttp(doco, pax_type_ext, checkInfo, checkDate, full_name);
                        paxForCkin.apis.set(doco);
                    }
                    else if (strcmp((const char*)document->name,"DOCA") == 0)
                    {
                        CheckIn::TPaxDocaItem doca;
                        doca.fromMeridianXML(document);

                        if (doca.apiType()==apiUnknown)
                          throw AstraLocale::UserException("MSG.INVALID_ADDRES_TYPE");

                        if (paxForCkin.apis.isPresent(doca.apiType()))
                          throw AstraLocale::UserException("MSG.SECTION_DUPLICATED_WITH_TYPE",
                                                           LEvntPrms() << PrmSmpl<string>("name", "DOCA")
                                                                       << PrmSmpl<string>("type", doca.type));

                        doca = NormalizeDocaHttp(doca, full_name);
                        CheckDocaHttp(doca, pax_type_ext, checkInfo, full_name);
                        paxForCkin.apis.set(doca);
                    }
                }

                if (!paxForCkin.apis.isPresent(apiDoc))
                  throw AstraLocale::UserException("MSG.SECTION_NOT_FOUND", LEvntPrms() << PrmSmpl<string>("name", "DOCS"));
                seg.paxForCkin.push_back(paxForCkin);
                multiPnrData.segs.add(multiPnrData.flt.oper, paxForCkin, true);
            }

            multiPnrData.checkJointCheckInAndComplete();

            XMLDoc emulDocHeader;
            CreateEmulXMLDoc(emulDocHeader);
            XMLDoc emulCkinDoc;
            CreateEmulDocs(TWebPaxForSaveSegs(seg),
                           multiPnrDataSegs,
                           emulDocHeader,
                           emulCkinDoc);

            TChangeStatusList ChangeStatusInfo;
            SirenaExchange::TLastExchangeList SirenaExchangeList;
            CheckIn::TAfterSaveInfoList AfterSaveInfoList;

            if (emulCkinDoc.docPtr()!=NULL) //ॣ������ ����� ��㯯�
            {
              xmlNodePtr emulReqNode=NodeAsNode("/term/query",emulCkinDoc.docPtr())->children;
              if (emulReqNode==NULL)
                throw EXCEPTIONS::Exception("CheckInInterface::CrewCheckin: emulReqNode=NULL");
              bool httpWasSent = false;
              if (SavePax(emulReqNode, NULL, ChangeStatusInfo, SirenaExchangeList, AfterSaveInfoList, httpWasSent))
              {
                //� �������� �᫨ �뫠 ॠ�쭠� ॣ������
                CheckIn::TAfterSaveInfoData afterSaveData(emulReqNode, NULL);
                SirenaExchangeList.handle(__FUNCTION__);
                AfterSaveInfoList.handle(afterSaveData, __FUNCTION__);
              }
            }
        }
        UpdateCrew(flt.point_id, commander, cockpit, cabin, ownerDisp);

    }
    catch(...)
    {
        throw;
    }
}

void CheckInInterface::GetFQTTierLevel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  CheckIn::TPaxFQTItem fqt;
  fqt.fromXML(NodeAsNode("fqt_rem", reqNode));

  std::string surname=NodeAsString("surname", reqNode);
  std::string name=NodeAsString("name", reqNode);
  CheckIn::TPaxDocItem doc;
  doc.fromXML(NodeAsNode("document", reqNode));

  SirenaExchange::TFFPInfoReq req;
  SirenaExchange::TFFPInfoRes res;
  req.set(fqt.airline, fqt.no, surname, name, doc.birth_date);

  get_ffp_status(req, res);
  if (res.error())
  {
    if (res.error_code=="6")
    {
      if (doc.birth_date!=ASTRA::NoExists)
        throw UserException("MSG.TIER_LEVEL_NOT_RECEIVED");
      else
        throw UserException("MSG.TIER_LEVEL_NOT_RECEIVED.TRY_SPECIFY_BIRTH_DATE");
    }
    else
      throw UserException(res.error_message);
  }

  fqt.tier_level=res.status;
  fqt.tier_level_confirm=true;

  fqt.toXML(resNode);
}
