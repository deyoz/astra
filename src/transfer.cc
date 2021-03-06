#include "transfer.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "exceptions.h"
#include "term_version.h"
#include "alarms.h"
#include "qrys.h"
#include "typeb_utils.h"

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;

namespace CheckIn
{

bool TTransferItem::equalSubclasses(const TTransferItem &item) const
{
  vector<TPaxTransferItem>::const_iterator p1=pax.begin();
  vector<TPaxTransferItem>::const_iterator p2=item.pax.begin();
  for(;p1!=pax.end()&&p2!=item.pax.end();++p1,++p2)
    if (p1->subclass!=p2->subclass) break;
  return (p1==pax.end() && p2==item.pax.end());
}

void PaxTransferFromDB(int pax_id, list<TPaxTransferItem> &trfer)
{
  trfer.clear();
  TCachedQuery Qry("SELECT transfer_num,subclass,subclass_fmt FROM transfer_subcls "
                   "WHERE pax_id=:pax_id ORDER BY transfer_num",
                   QParams() << QParam("pax_id", otInteger, pax_id));
  Qry.get().Execute();
  int trfer_num=1;
  for(; !Qry.get().Eof; Qry.get().Next(), trfer_num++)
  {
    if (trfer_num!=Qry.get().FieldAsInteger("transfer_num"))
      throw Exception("PaxTransferFromDB: transfer_subcls.transfer_num=%d not found (pax_id=%d)", trfer_num, pax_id);
    TPaxTransferItem item;
    item.pax_id=pax_id;
    item.subclass=Qry.get().FieldAsString("subclass");
    item.subclass_fmt=(TElemFmt)Qry.get().FieldAsInteger("subclass_fmt");
    trfer.push_back(item);
  };
};

void PaxTransferToXML(const list<TPaxTransferItem> &trfer, xmlNodePtr paxNode)
{
  if (paxNode==NULL) return;

  if (!trfer.empty())
  {
    xmlNodePtr node=NewTextChild(paxNode,"transfer");
    string str;
    for(list<TPaxTransferItem>::const_iterator i=trfer.begin(); i!=trfer.end(); ++i)
    {
      xmlNodePtr trferNode=NewTextChild(node,"segment");
      str=ElemIdToCodeNative(etSubcls, i->subclass);
      NewTextChild(trferNode,"subclass",str);
    };
  };
};

void PaxTransferToDB(int pax_id, int pax_no, const CheckIn::TTransferList &trfer, int seg_no)
{
  int seg_no_tmp=seg_no;

  CheckIn::TTransferList::const_iterator firstTrfer=trfer.begin();
  for(;firstTrfer!=trfer.end()&&seg_no_tmp>1;firstTrfer++,seg_no_tmp--);

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
  for(CheckIn::TTransferList::const_iterator t=firstTrfer;t!=trfer.end();t++,trfer_num++)
  {
    const CheckIn::TPaxTransferItem &pax=t->pax.at(pax_no-1);
    TrferQry.SetVariable("transfer_num",trfer_num);
    TrferQry.SetVariable("subclass",pax.subclass);
    TrferQry.SetVariable("subclass_fmt",(int)pax.subclass_fmt);
    TrferQry.Execute();
  }
  TrferQry.Close();
}

void TTransferList::load(int grp_id)
{
  clear();
  TTrferRoute route;
  route.GetRoute(grp_id, trtNotFirstSeg);
  for(TTrferRoute::const_iterator r=route.begin(); r!=route.end(); ++r)
  {
    emplace_back();
    TTransferItem &t=back();
    t.operFlt=r->operFlt;
    t.airp_arv=r->airp_arv;
    t.airp_arv_fmt=r->airp_arv_fmt;
  }
}

void TTransferList::check(int id, bool isGrpId, int seg_no) const
{
  int seg_no_tmp=seg_no;

  TTransferList::const_iterator firstTrfer=begin();
  for(;firstTrfer!=end()&&seg_no_tmp>1;firstTrfer++,seg_no_tmp--);
  if (firstTrfer==end()) return;

  TAdvTripInfo fltInfo;
  if (isGrpId)
  {
    if (!fltInfo.getByGrpId(id))
      throw EXCEPTIONS::Exception("Passenger group not found (grp_id=%d)", id);
  }
  else
  {
    if (!fltInfo.getByPointId(id))
      throw EXCEPTIONS::Exception("Flight not found (point_id=%d)", id);
  }

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
  }

  string strh;
  int trfer_num=1;
  for(TTransferList::const_iterator t=firstTrfer;t!=end();++t,trfer_num++)
  {
    if (checkType==checkAllSeg ||
        (checkType==checkFirstSeg && t==firstTrfer))
    {
      {
        const TAirlinesRow& row=(const TAirlinesRow&)base_tables.get("airlines").get_row("code",t->operFlt.airline);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirline, t->operFlt.airline, t->operFlt.airline_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRLINE_NOT_FOUND",
                              LParams()<<LParam("airline",strh)
                                       <<LParam("flight",t->flight_view));

        }
      }
      {
        const TAirpsRow& row=(const TAirpsRow&)base_tables.get("airps").get_row("code",t->operFlt.airp);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirp, t->operFlt.airp, t->operFlt.airp_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRP_DEP_NOT_FOUND",
                              LParams()<<LParam("airp",strh)
                                       <<LParam("flight",t->flight_view));
        }
      }
      {
        const TAirpsRow& row=(const TAirpsRow&)base_tables.get("airps").get_row("code",t->airp_arv);
        if (row.code_lat.empty())
        {
          strh=ElemIdToClientElem(etAirp, t->airp_arv, t->airp_arv_fmt);
          throw UserException("MSG.TRANSFER_FLIGHT.LAT_AIRP_ARR_NOT_FOUND",
                              LParams()<<LParam("airp",strh)
                                       <<LParam("flight",t->flight_view));
        }
      }
    }
  }
}

void TTransferList::parseSegments(xmlNodePtr trferNode,
                                  const AirportCode_t& airpArv,
                                  const TDateTime scd_out_local)
{
  clear();
  if (trferNode==nullptr) return;

  int trfer_num=1;
  string strh;
  string prior_airp_arv_id=airpArv.get();
  TDateTime local_scd=scd_out_local;
  for(xmlNodePtr node=trferNode->children;node!=nullptr;node=node->next,trfer_num++)
  {
    xmlNodePtr node2=node->children;

    ostringstream flt;
    flt << NodeAsStringFast("airline",node2)
        << setw(3) << setfill('0') << NodeAsIntegerFast("flt_no",node2)
        << NodeAsStringFast("suffix",node2) << "/"
        << setw(2) << setfill('0') << NodeAsIntegerFast("local_date",node2);

    CheckIn::TTransferItem seg;

    //????????????
    strh=NodeAsStringFast("airline",node2);
    seg.operFlt.airline=ElemToElemId(etAirline,strh,seg.operFlt.airline_fmt);
    if (seg.operFlt.airline_fmt==efmtUnknown)
      throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRLINE",
                          LParams()<<LParam("airline",strh)
                                   <<LParam("flight",flt.str()));

    //????? ?????
    seg.operFlt.flt_no=NodeAsIntegerFast("flt_no",node2);

    //???????
    if (!NodeIsNULLFast("suffix",node2))
    {
      strh=NodeAsStringFast("suffix",node2);
      seg.operFlt.suffix=ElemToElemId(etSuffix,strh,seg.operFlt.suffix_fmt);
      if (seg.operFlt.suffix_fmt==efmtUnknown)
        throw UserException("MSG.TRANSFER_FLIGHT.INVALID_SUFFIX",
                            LParams()<<LParam("suffix",strh)
                                     <<LParam("flight",flt.str()));
    }


    int local_date=NodeAsIntegerFast("local_date",node2);
    try
    {
      TDateTime base_date=local_scd-1; //????????? ????? ?? ?????? ?????? ? ??????? ?? ????????? ????
      local_scd=DayToDate(local_date,base_date,false); //????????? ???? ??????
      seg.operFlt.scd_out=local_scd;
    }
    catch(const EXCEPTIONS::EConvertError &E)
    {
      throw UserException("MSG.TRANSFER_FLIGHT.INVALID_LOCAL_DATE_DEP",
                          LParams()<<LParam("flight",flt.str()));
    }

    //???????? ??????
    if (GetNodeFast("airp_dep",node2)!=nullptr)  //????? ?/? ??????
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

    //???????? ???????
    strh=NodeAsStringFast("airp_arv",node2);
    seg.airp_arv=ElemToElemId(etAirp,strh,seg.airp_arv_fmt);
    if (seg.airp_arv_fmt==efmtUnknown)
      throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_AIRP_ARR",
                          LParams()<<LParam("airp",strh)
                                   <<LParam("flight",flt.str()));

    seg.flight_view=flt.str();


    prior_airp_arv_id=seg.airp_arv;

    push_back(seg);
  }
}

void TTransferList::parseSubclasses(xmlNodePtr paxNode)
{
  if (paxNode==nullptr) return;

  //??????????? ????????? ??????????
  for(paxNode=paxNode->children;paxNode!=nullptr;paxNode=paxNode->next)
  {
    xmlNodePtr paxTrferNode=NodeAsNode("transfer",paxNode)->children;
    CheckIn::TTransferList::iterator s=begin();
    for(;s!=end() && paxTrferNode!=nullptr; s++,paxTrferNode=paxTrferNode->next)
    {
      s->pax.push_back(CheckIn::TPaxTransferItem());
      CheckIn::TPaxTransferItem &pax=s->pax.back();

      string strh=NodeAsString("subclass",paxTrferNode);
      if (strh.empty())
        throw UserException("MSG.TRANSFER_FLIGHT.SUBCLASS_NOT_SET",
                            LParams()<<LParam("flight",s->flight_view));
      pax.subclass=ElemToElemId(etSubcls,strh,pax.subclass_fmt);
      if (pax.subclass_fmt==efmtUnknown)
        throw UserException("MSG.TRANSFER_FLIGHT.UNKNOWN_SUBCLASS",
                            LParams()<<LParam("subclass",strh)
                                     <<LParam("flight",s->flight_view));
    }
    if (s!=end() || paxTrferNode!=nullptr)
    {
      //?????? ????????: ???-?? ??????????? ?????????? ????????? ?? ????????? ? ???-??? ??????????? ?????????
      throw EXCEPTIONS::Exception("ParseTransfer: Different number of transfer subclasses and transfer segments");
    }
  }
}

TSearchFltInfo createSearchFlt(const CheckIn::TTransferItem &item)
{
    TSearchFltInfo filter;
    filter.airline  = item.operFlt.airline;
    filter.suffix   = item.operFlt.suffix;
    filter.airp_dep = item.operFlt.airp;
    filter.flt_no   = item.operFlt.flt_no;
    filter.scd_out  = item.operFlt.scd_out;
    filter.scd_out_in_utc  = false; //????????? ????? ?????
    filter.flightProps     = FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn);
    return filter;
}

TAdvTripInfo routeInfoFromTrfr(const CheckIn::TTransferItem& seg)
{
    TSearchFltInfo searchFilter = CheckIn::createSearchFlt(seg);
    std::list<TAdvTripInfo> flt;
    SearchFlt(searchFilter, flt);
    if(flt.empty() || flt.size() > 1) {
        LogTrace(TRACE5) << __FUNCTION__ << " Search filter error: ";
        throw Exception("Search error");
    }
    return flt.front();
}

} //namespace CheckIn

std::ostream& operator << (std::ostream& os, const TrferList::Alarm alarm)
{
  os << std::underlying_type<TrferList::Alarm>::type(alarm);
  return os;
}

namespace TrferList
{

TBagItem& TBagItem::setZero()
{
  clear();
  bag_amount=0;
  bag_weight=0;
  rk_weight=0;
  weight_unit='K';
  return *this;
};

TBagItem& TBagItem::fromDB(TQuery &Qry, TQuery &TagQry, bool fromTlg, bool loadTags)
{
  clear();
  bag_amount=!Qry.FieldIsNULL("bag_amount")?Qry.FieldAsInteger("bag_amount"):NoExists;
  bag_weight=!Qry.FieldIsNULL("bag_weight")?Qry.FieldAsInteger("bag_weight"):NoExists;
  rk_weight=!Qry.FieldIsNULL("rk_weight")?Qry.FieldAsInteger("rk_weight"):NoExists;
  if (fromTlg)
    weight_unit=Qry.FieldAsString("weight_unit");
  else
    weight_unit='K';
  if (loadTags)
  {
    if (fromTlg)
    {
      const char* tag_sql=
        "SELECT no FROM trfer_tags WHERE grp_id=:grp_id";
      if (strcmp(TagQry.SQLText.SQLText(),tag_sql)!=0)
      {
        TagQry.Clear();
        TagQry.SQLText=tag_sql;
        TagQry.DeclareVariable("grp_id", otInteger);
      };
    }
    else
    {
      if (Qry.FieldIsNULL("bag_pool_num")) return *this;

      const char* tag_sql=
        "SELECT bag_tags.no "
        "FROM bag_tags, bag2 "
        "WHERE bag_tags.grp_id=bag2.grp_id(+) AND "
        "      bag_tags.bag_num=bag2.num(+) AND "
        "      bag_tags.grp_id=:grp_id AND "
        "      NVL(bag2.bag_pool_num,1)=:bag_pool_num";
      if (strcmp(TagQry.SQLText.SQLText(),tag_sql)!=0)
      {
        TagQry.Clear();
        TagQry.SQLText=tag_sql;
        TagQry.DeclareVariable("grp_id", otInteger);
        TagQry.DeclareVariable("bag_pool_num", otInteger);
      };
      if (!Qry.FieldIsNULL("bag_pool_num"))
        TagQry.SetVariable("bag_pool_num", Qry.FieldAsInteger("bag_pool_num"));
      else
        TagQry.SetVariable("bag_pool_num", FNull);
    };
    TagQry.SetVariable("grp_id", Qry.FieldAsInteger("grp_id"));
    TagQry.Execute();
    for(;!TagQry.Eof;TagQry.Next())
      tags.insert(TBagTagNumber("",TagQry.FieldAsFloat("no")));
  };
  return *this;
};

TPaxItem& TPaxItem::fromDB(TQuery &Qry, TQuery &RemQry, bool fromTlg)
{
  clear();
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (!fromTlg)
  {
    seats=Qry.FieldAsInteger("seats");
    if (seats>1)
    {
      const char* rem_sql=
        "SELECT rem_code FROM pax_rem "
        "WHERE pax_id=:pax_id AND rem_code IN ('STCR', 'EXST') "
        "ORDER BY DECODE(rem_code,'STCR',0,'EXST',1,2)";
      if (strcmp(RemQry.SQLText.SQLText(),rem_sql)!=0)
      {
        RemQry.Clear();
        RemQry.SQLText=rem_sql;
        RemQry.DeclareVariable("pax_id", otInteger);
      };
      RemQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      RemQry.Execute();
      for(int i=2; i<=seats; i++)
      {
        name_extra+="/";
        if (!RemQry.Eof)
          name_extra+=RemQry.FieldAsString("rem_code");
        else
          name_extra+="EXST";
      };
    };
    subcl=Qry.FieldAsString("subclass");
  };
  return *this;
};

TPaxItem& TPaxItem::setUnaccomp()
{
  clear();
  surname="UNACCOMPANIED";
  seats=0;
  return *this;
};

TGrpItem& TGrpItem::paxFromDB(TQuery &PaxQry, TQuery &RemQry, bool fromTlg)
{
  paxs.clear();
  for(;!PaxQry.Eof;PaxQry.Next())
    paxs.push_back(TPaxItem().fromDB(PaxQry, RemQry, fromTlg));

  if (!fromTlg)
  {
    seats=0;
    for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
      seats+=p->seats;

    if (subcl.empty())
    {
      //?????????? ???????? ?????? grp.subcl ?? ?????? ?????????? ??????????
      for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
      {
        try
        {
          //?????????? ???????? ?????? grp.subcl
          string cl=((const TSubclsRow&)base_tables.get( "subcls" ).get_row( "code", p->subcl, true )).cl;
          if (subcl.empty())
            subcl=cl;
          else
            if (cl!=subcl)
            {
              subcl.clear();
              break;
            };
        }
        catch(const EBaseTableError&)
        {
          subcl.clear();
          break;
        };
      };
    };
  }
  else
  {
    //???? ???????? ???? ???????? PTM, ????????
    if (paxs.empty()) paxs.push_back(TPaxItem());
  };
  return *this;
};

TGrpItem& TGrpItem::trferFromDB(TQuery &TrferQry, bool fromTlg)
{
  trfer.clear();

  const char* trfer_sql_tlg=
    "SELECT num, airline, flt_no, suffix, local_date AS scd_out, "
    "       airp_dep AS airp, airp_arv "
    "FROM tlg_trfer_onwards "
    "WHERE grp_id=:grp_id AND num>0 ";
  const char* trfer_sql_ckin=
      "SELECT transfer_num-1 AS num, airline, flt_no, suffix, scd AS scd_out, "
      "       airp_dep AS airp, airp_arv "
      "FROM transfer,trfer_trips "
      "WHERE transfer.point_id_trfer=trfer_trips.point_id AND "
      "      grp_id=:grp_id AND transfer_num>1";

  if (strcmp(TrferQry.SQLText.SQLText(),fromTlg?trfer_sql_tlg:trfer_sql_ckin)!=0)
  {
    TrferQry.Clear();
    TrferQry.SQLText=fromTlg?trfer_sql_tlg:trfer_sql_ckin;
    TrferQry.DeclareVariable("grp_id", otInteger);
  };
  TrferQry.SetVariable("grp_id", grp_id);
  TrferQry.Execute();
  for(;!TrferQry.Eof;TrferQry.Next())
  {
    CheckIn::TTransferItem t;
    t.operFlt.Init(TrferQry);
    t.airp_arv=TrferQry.FieldAsString("airp_arv");
    //t.subclass=TrferQry.FieldAsString("subclass"); ??? ?? ?????????, ??? ??? ?? ????????? ? ?????????
    trfer.insert(make_pair(TrferQry.FieldAsInteger("num"), t));
  };
  return *this;
};

TGrpItem& TGrpItem::setPaxUnaccomp()
{
  paxs.clear();
  is_unaccomp=true;
  paxs.push_back(TPaxItem().setUnaccomp());
  seats=0;
  return *this;
};

void TrferFromDB(TTrferType type,
                 int point_id,   //point_id ???????? ????? ??????? ??? trferIn
                 bool pr_bag,
                 TTripInfo &flt,
                 vector<TGrpItem> &grps_ckin,
                 vector<TGrpItem> &grps_tlg)
{
  flt.Clear();
  grps_ckin.clear();
  grps_tlg.clear();

  set<TFltInfo> flts_from_ckin;
  set<TFltInfo> flts_from_tlg;
  map<int, TFltInfo> spp_point_ids_from_ckin;

  TQuery Qry(&OraSession);
  TQuery PaxQry(&OraSession);
  TQuery RemQry(&OraSession);
  TQuery TagQry(&OraSession);
  TQuery TrferQry(&OraSession);

  TTripRouteItem priorAirp;
  if (type==trferIn)
  {
    //point_id ???????? ????? ???????, ? ??? ????? ?????????? ????? ??????
    TTripRoute().GetPriorAirp(NoExists,point_id,trtNotCancelled,priorAirp);
    if (priorAirp.point_id==NoExists) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
    if (!flt.getByPointId(priorAirp.point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
  }
  else
  {
    if (!flt.getByPointId(point_id))
      throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
  };

  Qry.Clear();
  ostringstream sql;
  sql << "SELECT pax_grp.point_dep, pax_grp.grp_id, DECODE(pax.grp_id, NULL, bag2.bag_pool_num, pax.bag_pool_num) AS bag_pool_num, NVL(pax.cabin_class, pax_grp.class) AS class, \n"
//         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,bag2.amount,0)) AS bag_amount, \n" //????? ?? ???? ?????, ?? ??? ??-?? 11-?? ??????
//         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,bag2.weight,0)) AS bag_weight, \n"
//         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,0,bag2.weight)) AS rk_weight, \n"
         "       SUM(CASE WHEN pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL THEN 0 ELSE DECODE(bag2.pr_cabin,NULL,0,0,bag2.amount,0) END) AS bag_amount, \n"
         "       SUM(CASE WHEN pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL THEN 0 ELSE DECODE(bag2.pr_cabin,NULL,0,0,bag2.weight,0) END) AS bag_weight, \n"
         "       SUM(CASE WHEN pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL THEN 0 ELSE DECODE(bag2.pr_cabin,NULL,0,0,0,bag2.weight) END) AS rk_weight, \n";

  if (type==trferOut || type==trferOutForCkin)
  {
    //?????????? ? ??????????? ??????/??????????, ?????????????? ??????
    sql << "     pax_grp.point_dep AS point_id, pax_grp.airp_arv, \n"
           "     transfer.airp_arv AS last_airp_arv \n"
           "FROM trfer_trips, transfer, pax_grp, bag2, pax \n"
           "WHERE trfer_trips.point_id=transfer.point_id_trfer AND \n"
           "      transfer.grp_id=pax_grp.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.grp_id=pax.grp_id(+) AND \n"
//           "      DECODE(pax.grp_id, NULL, 1, pax.bag_pool_num)=DECODE(pax.grp_id, NULL, 1, bag2.bag_pool_num(+)) AND \n"
           "      (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL OR pax.bag_pool_num=bag2.bag_pool_num) AND \n"
           "      trfer_trips.point_id_spp=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferIn)
  {
    //?????????? ? ??????????? ??????/??????????, ??????????? ??????
    sql << "      transfer.point_id_trfer AS point_id, transfer.airp_arv \n"
           "FROM pax_grp, transfer, bag2, pax \n"
           "WHERE pax_grp.grp_id=transfer.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.grp_id=pax.grp_id(+) AND \n"
//           "      DECODE(pax.grp_id, NULL, 1, pax.bag_pool_num)=DECODE(pax.grp_id, NULL, 1, bag2.bag_pool_num(+)) AND \n"
           "      (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL OR pax.bag_pool_num=bag2.bag_pool_num) AND \n"
           "      pax_grp.point_arv=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferCkin)
  {
    //?????????? ? ??????????? ??????/??????????, ?????????????? ?????? (?????? ?? ?????? ??????????? ???????????)
    sql << "      transfer.point_id_trfer AS point_id, transfer.airp_arv \n"
           "FROM pax_grp, transfer, bag2, pax \n"
           "WHERE pax_grp.grp_id=transfer.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.grp_id=pax.grp_id(+) AND \n"
//           "      DECODE(pax.grp_id, NULL, 1, pax.bag_pool_num)=DECODE(pax.grp_id, NULL, 1, bag2.bag_pool_num(+)) AND \n"
           "      (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL OR pax.bag_pool_num=bag2.bag_pool_num) AND \n"
           "      pax_grp.point_dep=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };
  if (type==tckinInbound)
  {
    //??????????? ?????, ???????? ????????? ????????? ????? (?????? ?? ?????? ???????? ???????????)
    sql << "      tckin_pax_grp.tckin_id, tckin_pax_grp.grp_num, \n"
           "      pax_grp.point_dep AS point_id, pax_grp.airp_arv \n"
           "FROM pax_grp, tckin_pax_grp, bag2, pax \n"
           "WHERE pax_grp.grp_id=tckin_pax_grp.grp_id AND tckin_pax_grp.transit_num=0 AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.grp_id=pax.grp_id(+) AND \n"
//           "      DECODE(pax.grp_id, NULL, 1, pax.bag_pool_num)=DECODE(pax.grp_id, NULL, 1, bag2.bag_pool_num(+)) AND \n"
           "      (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL OR pax.bag_pool_num=bag2.bag_pool_num) AND \n"
           "      pax_grp.point_dep=:point_id AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  sql << "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') \n"
         "GROUP BY pax_grp.point_dep, pax_grp.grp_id, DECODE(pax.grp_id, NULL, bag2.bag_pool_num, pax.bag_pool_num), NVL(pax.cabin_class, pax_grp.class), \n";
  if (type==trferOut || type==trferOutForCkin)
  {
    sql << "         pax_grp.airp_arv, transfer.airp_arv \n";
  };
  if (type==trferIn || type==trferCkin)
  {
    sql << "         transfer.point_id_trfer, transfer.airp_arv \n";
  };
  if (type==tckinInbound)
  {
    sql << "         tckin_pax_grp.tckin_id, tckin_pax_grp.grp_num, \n"
        << "         pax_grp.airp_arv \n";
  };
  Qry.SQLText=sql.str().c_str();
  //ProgTrace(TRACE5, "point_id=%d\nQry.SQLText=\n%s", point_id, sql.str().c_str());

  PaxQry.Clear();
  sql.str("");
  sql << "SELECT pax.pax_id, pax.surname, pax.name, pax.seats, \n";
  if (type==trferOut || type==trferOutForCkin || type==tckinInbound)
  {
    sql << "       pax.subclass \n"
           "FROM pax_grp, pax \n"
           "WHERE \n";
  };
  if (type==trferIn || type==trferCkin)
  {
    sql << "       transfer_subcls.subclass \n"
           "FROM pax_grp, pax, transfer_subcls \n"
           "WHERE pax.pax_id=transfer_subcls.pax_id(+) AND transfer_subcls.transfer_num(+)=1 AND \n";
  };

  sql << "      pax.grp_id=pax_grp.grp_id AND \n"
         "      pax.grp_id=:grp_id AND \n"
         "      NVL(pax.cabin_class, pax_grp.class)=:cabin_class AND \n"
         "      (pax.bag_pool_num=:bag_pool_num OR pax.bag_pool_num IS NULL AND :bag_pool_num IS NULL) AND \n";
  if (type==trferIn || type==trferOut || type==trferOutForCkin)
    sql << "      pax.pr_brd=1 \n";
  if (type==trferCkin || type==tckinInbound)
    sql << "      pax.pr_brd IS NOT NULL \n";
  PaxQry.SQLText=sql.str().c_str();
  //ProgTrace(TRACE5, "PaxQry.SQLText=\n%s", sql.str().c_str());
  PaxQry.DeclareVariable("grp_id", otInteger);
  PaxQry.DeclareVariable("cabin_class", otString);
  PaxQry.DeclareVariable("bag_pool_num", otInteger);

  set<pair<int/*grp_id*/,string/*cabin_class*/>> grpKeys;

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (type==trferOut || type==trferOutForCkin || type==trferIn)
    {
      map<int, TFltInfo>::const_iterator id;
      int point_dep=Qry.FieldAsInteger("point_dep");
      id=spp_point_ids_from_ckin.find(point_dep);
      if (id==spp_point_ids_from_ckin.end())
      {
        TAdvTripInfo tmp;
        if (!tmp.getByPointId(point_dep)) continue;
        TFltInfo flt1(tmp, true);
        if (flt1.pr_del==NoExists) continue;
        id=spp_point_ids_from_ckin.insert( make_pair(point_dep, flt1) ).first;
        flts_from_ckin.insert(flt1);
      };
      if (id==spp_point_ids_from_ckin.end()) continue;
    };
    TGrpItem grp;
    if (type==tckinInbound)
    {
      auto inboundGrp=TCkinRoute::getPriorGrp(Qry.FieldAsInteger("tckin_id"),
                                              Qry.FieldAsInteger("grp_num"),
                                              TCkinRoute::IgnoreDependence,
                                              TCkinRoute::WithoutTransit);
      if (!inboundGrp) continue;

      TTripRouteItem priorAirp;
      TTripRoute().GetPriorAirp(NoExists,inboundGrp.get().point_arv,trtNotCancelled,priorAirp);
      if (priorAirp.point_id==NoExists) continue;
      grp.point_id=priorAirp.point_id;
    }
    else
      grp.point_id=Qry.FieldAsInteger("point_id");
    grp.grp_id=Qry.FieldAsInteger("grp_id");
    grp.bag_pool_num=Qry.FieldIsNULL("bag_pool_num")?NoExists:Qry.FieldAsInteger("bag_pool_num");
    grp.airp_arv=Qry.FieldAsString("airp_arv");
    if (type==trferOut || type==trferOutForCkin)
      grp.last_airp_arv=Qry.FieldAsString("last_airp_arv");
    if (type==trferOut || type==trferOutForCkin || type==trferCkin || type==tckinInbound)
      grp.subcl=Qry.FieldAsString("class");

    std::string cabinClass=Qry.FieldAsString("class");
    if (cabinClass.empty())
    {
      grp.fromDB(Qry, TagQry, false, pr_bag);
      grp.setPaxUnaccomp();
      if (type==trferOutForCkin)
        grp.trferFromDB(TrferQry, false);
      grps_ckin.push_back(grp);
    }
    else
    {
      for(int pass=0; pass<2; pass++)
      {
        if (pass==1) grp.bag_pool_num=NoExists;

        if (grp.bag_pool_num==NoExists)
        {
          if (!grpKeys.emplace(grp.grp_id, cabinClass).second) continue;  //??? ???????????? ??? ??????? grp_id+cabin_class bag_pool_num==NoExists
        };

        if (grp.bag_pool_num!=NoExists)
          grp.fromDB(Qry, TagQry, false, pr_bag);
        else
          grp.setZero();

        //????????? ??????
        PaxQry.SetVariable("grp_id", grp.grp_id);
        PaxQry.SetVariable("cabin_class", cabinClass);
        if (grp.bag_pool_num!=NoExists)
          PaxQry.SetVariable("bag_pool_num", grp.bag_pool_num);
        else
          PaxQry.SetVariable("bag_pool_num", FNull);
        PaxQry.Execute();
        if (PaxQry.Eof) continue; //?????? ?????? - ?? ???????? ? grps
        grp.paxFromDB(PaxQry, RemQry, false);
        if (type==trferOutForCkin)
          grp.trferFromDB(TrferQry, false);
        grps_ckin.push_back(grp);
      };
    };
  };

  //ProgTrace(TRACE5, "grps_ckin.size()=%zu", grps_ckin.size());

  if (!(type==trferOut || type==trferOutForCkin || type==trferIn)) return;

  Qry.Clear();
  if (type==trferOut || type==trferOutForCkin)
  {
    //?????????? ? ??????????? ??????/??????????, ?????????????? ??????
    Qry.SQLText=
      "SELECT tlg_trips_in.airline, tlg_trips_in.flt_no, tlg_trips_in.suffix, \n"
      "       tlg_trips_in.airp_dep AS airp, tlg_trips_in.scd AS scd_out, \n"
      "       tlg_trips_in.point_id AS point_id_tlg_in, tlg_trips_in.airp_arv, \n"
      "       tlg_trips_out.airp_arv AS last_airp_arv, \n"
      "       tlg_transfer.subcl_in AS subcl, \n"
      "       trfer_grp.grp_id, trfer_grp.seats, \n"
      "       trfer_grp.bag_amount, trfer_grp.bag_weight, trfer_grp.rk_weight, trfer_grp.weight_unit \n"
      "FROM tlg_binding tlg_binding_out, tlg_transfer, tlgs_in, trfer_grp, \n"
      "     tlg_trips tlg_trips_in, tlg_trips tlg_trips_out \n"
      "WHERE tlg_binding_out.point_id_tlg=tlg_transfer.point_id_out AND \n"
      "      tlg_binding_out.point_id_spp=:point_id AND \n"
      "      tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND \n"
      "      tlgs_in.type=:tlg_type AND \n"
      "      tlg_transfer.trfer_id=trfer_grp.trfer_id AND \n"
      "      tlg_transfer.point_id_in=tlg_trips_in.point_id AND \n"
      "      tlg_transfer.point_id_out=tlg_trips_out.point_id \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
    //ProgTrace(TRACE5, "point_id=%d\nQry.SQLText=\n%s", point_id, Qry.SQLText.SQLText());
  };
  if (type==trferIn)
  {
    //?????????? ? ??????????? ??????/??????????, ??????????? ??????
    Qry.SQLText=
      "SELECT tlg_trips_in.airline, tlg_trips_in.flt_no, tlg_trips_in.suffix, \n"
      "       tlg_trips_in.airp_dep AS airp, tlg_trips_in.scd AS scd_out, \n"
      "       tlg_trips_out.point_id AS point_id_tlg_out, tlg_trips_out.airp_arv, \n"
      "       tlg_transfer.subcl_out AS subcl, \n"
      "       trfer_grp.grp_id, trfer_grp.seats, \n"
      "       trfer_grp.bag_amount, trfer_grp.bag_weight, trfer_grp.rk_weight, trfer_grp.weight_unit \n"
      "FROM tlg_binding tlg_binding_in, tlg_transfer, tlgs_in, trfer_grp, tlg_trips tlg_trips_out, \n"
      "     tlg_trips tlg_trips_in \n"
      "WHERE tlg_binding_in.point_id_tlg=tlg_transfer.point_id_in AND \n"
      "      tlg_binding_in.point_id_spp=:point_id AND \n"
      "      tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND \n"
      "      tlgs_in.type=:tlg_type AND \n"
      "      tlg_transfer.trfer_id=trfer_grp.trfer_id AND \n"
      "      tlg_transfer.point_id_out=tlg_trips_out.point_id AND \n"
      "      tlg_transfer.point_id_in=tlg_trips_in.point_id \n";
    Qry.CreateVariable("point_id", otInteger, priorAirp.point_id);
    //ProgTrace(TRACE5, "point_id=%d\nQry.SQLText=\n%s", priorAirp.point_id, Qry.SQLText.SQLText());
  };
  if (pr_bag)
    Qry.CreateVariable("tlg_type", otString, "BTM");
  else
    Qry.CreateVariable("tlg_type", otString, "PTM");

  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT surname, name FROM trfer_pax WHERE grp_id=:grp_id";
  PaxQry.DeclareVariable("grp_id", otInteger);

  TQuery ExceptQry(&OraSession);
  ExceptQry.SQLText=
    "SELECT type FROM tlg_trfer_excepts WHERE grp_id=:grp_id AND type IN ('UNAC') AND rownum<2";
  ExceptQry.DeclareVariable("grp_id", otInteger);

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TFltInfo flt1(Qry, false);
    set<TFltInfo>::const_iterator id;
    id=flts_from_ckin.find(flt1);
    if (id!=flts_from_ckin.end())
    {
      //?????? ???????????? ???? ????????????? ? ?????
      continue;
    };

    id=flts_from_tlg.find(flt1);
    if (id==flts_from_tlg.end())
    {
      //???? ?????????? ??? flt1 - ????????? ???????? ???????????? ? ?????
      TSearchFltInfo filter;
      filter.airline=flt1.airline;
      filter.flt_no=flt1.flt_no;
      filter.suffix=flt1.suffix;
      filter.airp_dep=flt1.airp;
      filter.scd_out=flt1.scd_out;
      filter.scd_out_in_utc=false;
      filter.additional_where=
        " AND EXISTS(SELECT pax_grp.point_dep "
        "            FROM pax_grp "
        "            WHERE pax_grp.point_dep=points.point_id AND pax_grp.status NOT IN ('E'))";

      //???? ???? ? ???
      list<TAdvTripInfo> flts;
      SearchFlt(filter, flts);
      list<TAdvTripInfo>::const_iterator f=flts.begin();
      for(; f!=flts.end(); ++f)
      {
        TFltInfo flt2(*f, true);
        if (!(flt1<flt2) && !(flt2<flt1)) break;
      };

      if (f==flts.end())
        //?? ????? ???? - ?????? ?? ????????????? ? ?????
        id=flts_from_tlg.insert(flt1).first;
      else
        //?????? ???????????? ???? ????????????? ? ?????
        flts_from_ckin.insert(flt1);
    };
    if (id==flts_from_tlg.end()) continue;

    TGrpItem grp;
    if (type==trferOut || type==trferOutForCkin)
      grp.point_id=Qry.FieldAsInteger("point_id_tlg_in");
    if (type==trferIn)
      grp.point_id=Qry.FieldAsInteger("point_id_tlg_out");
    grp.grp_id=Qry.FieldAsInteger("grp_id");
    grp.airp_arv=Qry.FieldAsString("airp_arv");
    if (type==trferOut || type==trferOutForCkin)
      grp.last_airp_arv=Qry.FieldAsString("last_airp_arv");
    grp.subcl=Qry.FieldAsString("subcl");
    grp.fromDB(Qry, TagQry, true, pr_bag);
    grp.seats=Qry.FieldIsNULL("seats")?NoExists:Qry.FieldAsInteger("seats");
    //????????? ??????
    PaxQry.SetVariable("grp_id", grp.grp_id);
    PaxQry.Execute();
    //???? ???? ?????? ?????? - ???????? ? paxs ?????? surname
    grp.paxFromDB(PaxQry, RemQry, true);
    if (pr_bag)
    {
      //?????????? ???????????????? ????? - ?? ????? ???? ?????? ? BTM
      ExceptQry.SetVariable("grp_id", grp.grp_id);
      ExceptQry.Execute();
      if (!ExceptQry.Eof)
        grp.is_unaccomp=true;
      else
      {
        //???????? UNACCOMPANIED
        if (grp.subcl.empty() &&
            grp.paxs.size()==1 &&
            grp.paxs.begin()->surname=="UNACCOMPANIED" &&
            grp.paxs.begin()->name.empty())
          grp.is_unaccomp=true;
      };
    };
    if (type==trferOutForCkin)
      grp.trferFromDB(TrferQry, true);
    grps_tlg.push_back(grp);
  };
  //ProgTrace(TRACE5, "grps_tlg.size()=%zu", grps_tlg.size());
}

void TrferToXML(TTrferType type,
                const vector<TGrpViewItem> &grps,
                xmlNodePtr trferNode)
{
  xmlNodePtr grpsNode=NULL;
  xmlNodePtr grpNode=NULL;
  xmlNodePtr paxsNode=NULL;
  bool newPaxsNode=true;
  vector<TGrpViewItem>::const_iterator iGrpPrior=grps.end();
  for(vector<TGrpViewItem>::const_iterator iGrp=grps.begin();iGrp!=grps.end();++iGrp)
  {
    if (iGrpPrior==grps.end() ||
        iGrpPrior->point_id!=iGrp->point_id ||
        iGrpPrior->flt_view!=iGrp->flt_view ||
        iGrpPrior->airp_arv_view!=iGrp->airp_arv_view ||
        iGrpPrior->subcl_view!=iGrp->subcl_view)
    {
      xmlNodePtr node=NewTextChild(trferNode,"trfer_flt");

      ostringstream trip;
      trip << iGrp->flt_view.airline
           << setw(3) << setfill('0') << iGrp->flt_view.flt_no
           << iGrp->flt_view.suffix << "/"
           << (iGrp->flt_view.scd_out==NoExists?"??":DateTimeToStr(iGrp->flt_view.scd_out,"dd"));

      NewTextChild(node,"trip",trip.str());
      if (type==trferIn || type==trferOut || type==trferOutForCkin)
        NewTextChild(node,"airp",iGrp->tlg_airp_view);
      NewTextChild(node,"airp_dep",iGrp->flt_view.airp);
      NewTextChild(node,"airp_arv",iGrp->airp_arv_view);
      NewTextChild(node,"subcl",iGrp->subcl_view);
      if (type==tckinInbound)
      {
        NewTextChild(node,"point_dep",iGrp->point_id);
        NewTextChild(node,"trip2",iGrp->inbound_trip,"");
      };
      grpsNode=NewTextChild(node,"grps");
      grpNode=NewTextChild(grpsNode,"grp");
      newPaxsNode=true;
    }
    else
    {
      if (type==trferIn || type==trferOut || type==trferOutForCkin ||
          iGrpPrior==grps.end() || iGrpPrior->grp_id!=iGrp->grp_id)
      {
        grpNode=NewTextChild(grpsNode,"grp");
        newPaxsNode=true;
      };
    };

    if (type==trferIn || type==trferOut || type==trferOutForCkin)
    {
      if (type==trferOutForCkin)
      {
        NewTextChild(grpNode,"grp_id",iGrp->grp_id);
        NewTextChild(grpNode,"bag_pool_num",iGrp->bag_pool_num,NoExists);
        if (iGrp->calc_status!=NoExists)
          NewTextChild(grpNode,"calc_status",(int)(iGrp->calc_status!=0));
        else
          NewTextChild(grpNode,"calc_status");
      };
      if (iGrp->bag_amount!=NoExists)
        NewTextChild(grpNode,"bag_amount",iGrp->bag_amount);
      else
        NewTextChild(grpNode,"bag_amount");
      if (iGrp->bag_weight!=NoExists)
        NewTextChild(grpNode,"bag_weight",iGrp->bag_weight);
      else
        NewTextChild(grpNode,"bag_weight");
      if (iGrp->rk_weight!=NoExists)
        NewTextChild(grpNode,"rk_weight",iGrp->rk_weight);
      else
        NewTextChild(grpNode,"rk_weight");
      NewTextChild(grpNode,"weight_unit",iGrp->weight_unit);

      NewTextChild(grpNode,"seats",iGrp->seats);

      vector<string> tagRanges;
      GetTagRanges(iGrp->tags, tagRanges);
      if (!tagRanges.empty())
      {
        //???????? ???????
        xmlNodePtr node=NewTextChild(grpNode,"tag_ranges");
        for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
        {
          xmlNodePtr rangeNode=NewTextChild(node,"range",*r);
          if (!iGrp->alarms.empty())
          {
            if (iGrp->alarms.size()==1)
              NewTextChild(rangeNode, "alarm", AlarmTypes::instance().encode(*(iGrp->alarms.begin())));
            else
            {
              xmlNodePtr alarmsNode=NewTextChild(rangeNode, "alarms");
              for(set<Alarm>::const_iterator a=iGrp->alarms.begin(); a!=iGrp->alarms.end(); ++a)
                NewTextChild(alarmsNode, "alarm", AlarmTypes::instance().encode(*a));
            };
          };
        };
      };
    };

    if (!iGrp->paxs.empty())
    {
      if (newPaxsNode)
      {
        paxsNode=NewTextChild(grpNode,"passengers");
        newPaxsNode=false;
      };
      for(vector<TPaxItem>::const_iterator iPax=iGrp->paxs.begin(); iPax!=iGrp->paxs.end(); ++iPax)
      {
        xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
        NewTextChild(paxNode,"surname",iPax->surname);
        NewTextChild(paxNode,"name",iPax->name+iPax->name_extra,"");
        if (!(type==trferIn || type==trferOut || type==trferOutForCkin))

        {
          if (iPax==iGrp->paxs.begin())
          {
            if (iGrp->bag_amount!=NoExists)
              NewTextChild(paxNode,"bag_amount",iGrp->bag_amount,0);
            if (iGrp->bag_weight!=NoExists)
              NewTextChild(paxNode,"bag_weight",iGrp->bag_weight,0);
            if (iGrp->rk_weight!=NoExists)
              NewTextChild(paxNode,"rk_weight",iGrp->rk_weight,0);
          };
          NewTextChild(paxNode,"seats",iPax->seats,1);

          if (iPax==iGrp->paxs.begin())
          {
            vector<string> tagRanges;
            GetTagRanges(iGrp->tags, tagRanges);
            if (!tagRanges.empty())
            {
              xmlNodePtr node=NewTextChild(paxNode,"tag_ranges");
              for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
                NewTextChild(node,"range",*r);
            };
          };
        };
      };
    };

    iGrpPrior=iGrp;
  };
};

void GrpsToGrpsView(TTrferType type,
                    bool pr_bag,
                    const vector<TGrpItem> &grps_ckin,
                    const vector<TGrpItem> &grps_tlg,
                    const TAlarmTagMap &alarms,
                    vector<TGrpViewItem> &grps)
{
  grps.clear();

  TQuery FltQry(&OraSession);

  for(int from_tlg=0; from_tlg<2; from_tlg++)
  {
    map<int, TFltInfo> point_ids;
    FltQry.Clear();
    if (!from_tlg)
    {
      if (type==trferOut || type==trferOutForCkin || type==tckinInbound)
      {
        FltQry.SQLText=
          "SELECT airline, flt_no, suffix, scd_out, airp "
          "FROM points "
          "WHERE point_id=:point_id";
      };
      if (type==trferIn || type==trferCkin)
      {
        FltQry.SQLText=
          "SELECT airline, flt_no, suffix, scd AS scd_out, airp_dep AS airp "
          "FROM trfer_trips "
          "WHERE point_id=:point_id";
      };
    }
    else
    {
      FltQry.SQLText=
        "SELECT airline, flt_no, suffix, scd AS scd_out, airp_dep AS airp "
        "FROM tlg_trips "
        "WHERE point_id=:point_id";
    };

    FltQry.DeclareVariable("point_id", otInteger);

    vector<TGrpItem>::const_iterator g=from_tlg?grps_tlg.begin():grps_ckin.begin();
    for(; g!=(from_tlg?grps_tlg.end():grps_ckin.end()); ++g)
    {
      TGrpViewItem grp;
      if (g->point_id==NoExists) continue;
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && pr_bag && g->tags.empty()) continue; //BTM ?? ???????? ?????????? ??? ?????
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && !pr_bag && g->is_unaccomp) continue; //PTM ?? ???????? ???????????????? ?????

      map<int, TFltInfo>::const_iterator i=point_ids.find(g->point_id);
      if (i==point_ids.end())
      {
        FltQry.SetVariable("point_id", g->point_id);
        FltQry.Execute();
        if (FltQry.Eof) continue;
        bool calc_local_time=!from_tlg && (type==trferOut || type==trferOutForCkin || type==tckinInbound);
        i=point_ids.insert(make_pair(g->point_id, TFltInfo(FltQry, calc_local_time))).first;
      };
      if (i==point_ids.end()) continue;
      //if (i->second.pr_del==NoExists) continue;
      grp.point_id=g->point_id;
      if (type==tckinInbound)
        grp.inbound_trip=GetTripName(i->second,ecNone);
      grp.grp_id=g->grp_id;
      grp.bag_pool_num=g->bag_pool_num;
      grp.flt_view=i->second;
      grp.flt_view.airline=ElemIdToCodeNative(etAirline,grp.flt_view.airline);
      grp.flt_view.suffix=ElemIdToCodeNative(etSuffix,grp.flt_view.suffix);
      grp.flt_view.airp=ElemIdToCodeNative(etAirp,grp.flt_view.airp);
      grp.airp_arv_view=ElemIdToCodeNative(etAirp,g->airp_arv);
      if (type==trferIn)
        grp.tlg_airp_view=grp.airp_arv_view;
      if (type==trferOut || type==trferOutForCkin)
        grp.tlg_airp_view=grp.flt_view.airp;

      if (type!=trferOut && type!=trferOutForCkin && !g->subcl.empty())
      {
        grp.subcl_view=ElemIdToCodeNative(etSubcls,g->subcl); //?????? ??? ????????????????? ??????
        grp.subcl_priority=0;
        try
        {
          const TSubclsRow &subclsRow=(const TSubclsRow&)base_tables.get("subcls").get_row("code",g->subcl);
          grp.subcl_priority=((const TClassesRow&)base_tables.get("classes").get_row("code",subclsRow.cl)).priority;
        }
        catch(const EBaseTableError&){};
      }
      else
      {
        grp.subcl_priority=from_tlg?0:10;
      };

      grp.paxs=g->paxs;
      sort(grp.paxs.begin(),grp.paxs.end());
      grp.seats=g->seats;

      grp.bag_amount=g->bag_amount;
      grp.bag_weight=g->bag_weight;
      grp.rk_weight=g->rk_weight;
      grp.weight_unit=g->weight_unit;
      grp.tags=g->tags;

      TGrpId id(grp.grp_id, grp.bag_pool_num);
      TAlarmTagMap::const_iterator a=alarms.find(id);
      if (a!=alarms.end())
        grp.alarms=a->second;

      if (!grp.alarms.empty())
        grp.calc_status=NoExists;
      else
        grp.calc_status=(int)true;

      grps.push_back(grp);
    };
  };
  //ProgTrace(TRACE5, "grps.size()=%zu", grps.size());
};

void TrferToXML(TTrferType type,
                int point_id,
                bool pr_bag,
                const TTripInfo &flt,
                const vector<TGrpItem> &grps_ckin,
                const vector<TGrpItem> &grps_tlg,
                xmlNodePtr resNode)
{
  vector<TGrpViewItem> grps;


  TAlarmTagMap alarms;
  if ((type==trferOut || type==trferOutForCkin) && pr_bag)
  {
    InboundTrfer::TUnattachedTagMap unattached_grps;
    InboundTrfer::GetUnattachedTags(point_id, grps_ckin, grps_tlg, unattached_grps);
    //ProgTrace(TRACE5, "unattached_grps.size()=%zu", unattached_grps.size());

    if (!unattached_grps.empty())
    {
      set<Alarm> unattached_alarm;
      unattached_alarm.insert(Alarm::Unattached);
      for(int from_tlg=0; from_tlg<2; from_tlg++)
      {
        vector<TGrpItem>::const_iterator iGrp=from_tlg?grps_tlg.begin():grps_ckin.begin();
        for(; iGrp!=(from_tlg?grps_tlg.end():grps_ckin.end()); ++iGrp)
        {
          //????????? ??? ?????? ??????? atUnattachedTrfer, ???? ????????????? ?????
          InboundTrfer::TGrpId id(iGrp->grp_id, iGrp->bag_pool_num);

          InboundTrfer::TUnattachedTagMap::const_iterator g=unattached_grps.find(id);
          if (g!=unattached_grps.end())
          {
            if (!g->second.empty() && g->second.size()==iGrp->tags.size())
            {
              //??? ????? ?????? ???????????
              alarms.insert(make_pair(id, unattached_alarm));
            };
          };
        };
      };
    };
  };

  GrpsToGrpsView(type,
                 pr_bag,
                 grps_ckin,
                 grps_tlg,
                 alarms,
                 grps);
  sort(grps.begin(),grps.end());

  //????????? XML
  TTripInfo flt_tmp=flt;
  flt_tmp.pr_del=0; //????? ?? ?????????? "(???.)"
  NewTextChild(resNode,"trip",GetTripName(flt_tmp,ecNone));

  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");

  TrferToXML(type,
             grps,
             trferNode);
};

void TrferConfirmFromXML(xmlNodePtr trferNode,
                         map<TGrpId, TGrpConfirmItem> &grps)
{
  grps.clear();
  if (trferNode==NULL) return;
  for(xmlNodePtr node=trferNode->children; node!=NULL; node=node->next)
  {
    xmlNodePtr node2=node->children;

    TGrpId id(NodeAsIntegerFast("grp_id", node2),
              NodeAsIntegerFast("bag_pool_num", node2, NoExists));
    pair< map<TGrpId, TGrpConfirmItem>::iterator, bool > res=grps.insert(make_pair(id, TGrpConfirmItem()));
    if (!res.second) throw Exception("%s: TGrpId(%d, %s) duplicated",
                                     __FUNCTION__,
                                     id.first,
                                     id.second==NoExists?"NoExists":IntToString(id.second).c_str());
    TGrpConfirmItem &grp=res.first->second;
    grp.grp_id=id.first;
    grp.bag_pool_num=id.second;
    grp.bag_weight=NodeIsNULLFast("bag_weight", node2)?NoExists:NodeAsIntegerFast("bag_weight", node2);
    grp.calc_status=NodeIsNULLFast("calc_status", node2)?NoExists:(NodeAsIntegerFast("calc_status", node2)!=0);
    grp.conf_status=NodeIsNULLFast("conf_status", node2)?NoExists:(NodeAsIntegerFast("conf_status", node2)!=0);
    for(xmlNodePtr rangeNode=NodeAsNodeFast("tag_ranges", node2)->children; rangeNode!=NULL; rangeNode=rangeNode->next)
      grp.tag_ranges.push_back(NodeAsString(rangeNode));
    sort(grp.tag_ranges.begin(), grp.tag_ranges.end());
  };
};

bool trferInExists(int point_arv, int prior_point_arv, TQuery& Qry)
{
  const char *sql =
    "SELECT 1 "
    "FROM pax_grp, transfer, pax "
    "WHERE pax_grp.grp_id=transfer.grp_id AND "
    "      pax_grp.grp_id=pax.grp_id(+) AND "
    "      pax_grp.point_arv=:point_arv AND "
    "      transfer.transfer_num=1 AND "
    "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') AND "
    "      (pax_grp.class IS NULL OR pax.pr_brd=1) AND "
    "      rownum<2 "
    "UNION "
    "SELECT 2 "
    "FROM tlg_binding tlg_binding_in, tlg_transfer "
    "WHERE tlg_binding_in.point_id_tlg=tlg_transfer.point_id_in AND "
    "      tlg_binding_in.point_id_spp=:prior_point_arv AND "
    "      rownum<2 ";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("point_arv", otInteger);
    Qry.DeclareVariable("prior_point_arv", otInteger);
  };
  Qry.SetVariable("point_arv", point_arv);
  Qry.SetVariable("prior_point_arv", prior_point_arv);
  Qry.Execute();
  return !Qry.Eof;
};

bool trferOutExists(int point_dep, TQuery& Qry)
{
  const char *sql =
    "SELECT 1 "
    "FROM trfer_trips, transfer, pax_grp, pax "
    "WHERE trfer_trips.point_id=transfer.point_id_trfer AND "
    "      transfer.grp_id=pax_grp.grp_id AND "
    "      pax_grp.grp_id=pax.grp_id(+) AND "
    "      trfer_trips.point_id_spp=:point_dep AND "
    "      transfer.transfer_num=1 AND "
    "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') AND "
    "      (pax_grp.class IS NULL OR pax.pr_brd=1) AND "
    "      rownum<2 "
    "UNION "
    "SELECT 2 "
    "FROM tlg_binding tlg_binding_out, tlg_transfer "
    "WHERE tlg_binding_out.point_id_tlg=tlg_transfer.point_id_out AND "
    "      tlg_binding_out.point_id_spp=:point_dep AND "
    "      rownum<2 ";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("point_dep", otInteger);
  };
  Qry.SetVariable("point_dep", point_dep);
  Qry.Execute();
  return !Qry.Eof;
};

bool trferCkinExists(int point_dep, TQuery& Qry)
{
  const char *sql =
    "SELECT 1 "
    "FROM pax_grp, transfer, pax "
    "WHERE pax_grp.grp_id=transfer.grp_id AND "
    "      pax_grp.grp_id=pax.grp_id(+) AND "
    "      pax_grp.point_dep=:point_dep AND "
    "      transfer.transfer_num=1 AND "
    "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') AND "
    "      (pax_grp.class IS NULL OR pax.pr_brd IS NOT NULL) AND "
    "      rownum<2";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("point_dep", otInteger);
  };
  Qry.SetVariable("point_dep", point_dep);
  Qry.Execute();
  return !Qry.Eof;
};

}; //namespace TrferList

namespace InboundTrfer
{

int TPaxItem::equalRate(const TPaxItem &item) const
{
  int rate, max_rate=0;
  for(int i=0; i<2; i++)
  {
    rate=0;
    if (!surname.empty() && translit_surname[i]==item.translit_surname[i]) rate+=4;
    if (!name.empty() && translit_name[i]==item.translit_name[i]) rate+=2;
    if (rate>max_rate) max_rate=rate;
  };
  rate=max_rate;
  if (!subcl.empty() && subcl==item.subcl) rate+=1;

  //(TRACE5, "TPaxItem::equalRate: %s %s VS %s %s rate=%d",
  //                  surname.c_str(), name.c_str(), item.surname.c_str(), item.name.c_str(), rate);
  return rate;
};

TGrpItem::TGrpItem(const TrferList::TGrpItem &grp)
{
  airp_arv=grp.last_airp_arv;
  is_unaccomp=grp.is_unaccomp;
  if (!is_unaccomp)
  {
    for(vector<TrferList::TPaxItem>::const_iterator p=grp.paxs.begin(); p!=grp.paxs.end(); ++p)
      paxs.push_back(TPaxItem(*p));
  };
  tags=grp.tags;
  trfer=grp.trfer;
};

int TGrpItem::equalRate(const TGrpItem &item, int minPaxEqualRate) const
{
  if (is_unaccomp!=item.is_unaccomp) return 0;
  if (airp_arv!=item.airp_arv) return 0;
  int rate=0;
  if (!is_unaccomp)
  {
    //??????? ?????? ??????????
    list<TPaxItem> paxs1(paxs.begin(), paxs.end());
    list<TPaxItem> paxs2(item.paxs.begin(), item.paxs.end());
    for(int r=7; r>0 && r>=minPaxEqualRate; r--)
    {
      for(list<TPaxItem>::iterator p1=paxs1.begin(); p1!=paxs1.end();)
      {
        list<TPaxItem>::iterator p2=paxs2.begin();
        for(; p2!=paxs2.end(); ++p2)
          if (r==p1->equalRate(*p2)) break;
        if (p2!=paxs2.end())
        {
          rate++;
          p1=paxs1.erase(p1);
          p2=paxs2.erase(p2);
        }
        else ++p1;
      };
    };
  }
  else rate=1;
  return rate;
};

bool TGrpItem::equalTrfer(const TGrpItem &item) const
{
  map<int, CheckIn::TTransferItem>::const_iterator s1=trfer.begin();
  map<int, CheckIn::TTransferItem>::const_iterator s2=item.trfer.begin();
  for(;s1!=trfer.end()&&s2!=item.trfer.end();++s1,++s2)
    if (s1->first!=s2->first ||
        !s1->second.equalSeg(s2->second)) break;
  return (s1==trfer.end() && s2==item.trfer.end());
}

bool TGrpItem::similarTrfer(const TGrpItem &item, bool checkSubclassesEquality) const
{
  map<int, CheckIn::TTransferItem>::const_iterator s1=trfer.begin();
  map<int, CheckIn::TTransferItem>::const_iterator s2=item.trfer.begin();
  for(;s1!=trfer.end()&&s2!=item.trfer.end();++s1,++s2)
    if (s1->first!=s2->first ||
        !s1->second.equalSeg(s2->second) ||
        (checkSubclassesEquality && !s1->second.equalSubclasses(s2->second))) break;
  return (s1==trfer.end() || s2==item.trfer.end());
}

bool TGrpItem::addSubclassesForEqualTrfer(const TGrpItem &item)
{
  if (equalTrfer(item))
  {
    map<int, CheckIn::TTransferItem>::const_iterator t1=item.trfer.begin();
    map<int, CheckIn::TTransferItem>::iterator t2=trfer.begin();
    for(;t1!=item.trfer.end() && t2!=trfer.end();++t1,++t2)
      t2->second.pax.emplace_back(t1->second.subclass, t1->second.subclass_fmt);
    return true;
  }

  return false;
}

void TGrpItem::printTrfer(const std::string& title, bool printSubclasses) const
{
  LogTrace(TRACE5) << title << ":";

  for(map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin(); t!=trfer.end(); ++t)
  {
    ostringstream s;
    if (printSubclasses)
    {
      s << "/";
      for(const CheckIn::TPaxTransferItem& p : t->second.pax)
        s << p.subclass;
    }

    ProgTrace(TRACE5, "  %d: %s%d%s/%s %s-%s%s", t->first,
                      t->second.operFlt.airline.c_str(), t->second.operFlt.flt_no, t->second.operFlt.suffix.c_str(),
                      DateTimeToStr(t->second.operFlt.scd_out, "ddmmm").c_str(), t->second.operFlt.airp.c_str(), t->second.airp_arv.c_str(),
                      s.str().c_str());
  }
}

void TGrpItem::print() const
{
  ProgTrace(TRACE5, "airp_arv=%s is_unaccomp=%s", airp_arv.c_str(), is_unaccomp?"true":"false");
  ProgTrace(TRACE5, "paxs:");
  for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    ProgTrace(TRACE5, "  surname=%s name=%s subcl=%s", p->surname.c_str(), p->name.c_str(), p->subcl.c_str());
  ProgTrace(TRACE5, "tags:");
  for(multiset<TBagTagNumber>::const_iterator t=tags.begin(); t!=tags.end(); ++t)
    ProgTrace(TRACE5, "  %15.0f", t->numeric_part);
  printTrfer("trfer");
}

bool TGrpItem::alreadyCheckedIn(int point_id) const
{
  if (tags.empty()) return false;

  const char *sql=
    "SELECT pax_grp.grp_id, bag2.bag_pool_num, "
    "       pax_grp.airp_arv, pax_grp.class, "
    "       bag2.num AS bag_num "
    "FROM bag_tags, bag2, pax_grp "
    "WHERE bag_tags.grp_id=bag2.grp_id AND "
    "      bag_tags.bag_num=bag2.num AND "
    "      bag2.grp_id=pax_grp.grp_id AND "
    "      bag_tags.no=:no AND "
    "      pax_grp.point_dep=:point_id AND "
    "      bag2.is_trfer<>0 AND bag2.handmade=0 ";

  const char *tags_sql=
    "SELECT no FROM bag_tags WHERE grp_id=:grp_id AND bag_num=:bag_num";
  const char *paxs_sql=
    "SELECT subclass, surname, name "
    "FROM pax "
    "WHERE grp_id=:grp_id AND bag_pool_num=:bag_pool_num";

  QParams QryParams;
  QryParams << QParam("no", otFloat, tags.begin()->numeric_part);
  QryParams << QParam("point_id", otInteger, point_id);
  TCachedQuery CachedQry(sql, QryParams);
  TQuery &Qry=CachedQry.get();

  QParams TagQryParams;
  TagQryParams << QParam("grp_id", otInteger);
  TagQryParams << QParam("bag_num", otInteger);
  TCachedQuery CachedTagQry(tags_sql, TagQryParams);
  TQuery &TagQry=CachedTagQry.get();

  QParams PaxQryParams;
  PaxQryParams << QParam("grp_id", otInteger);
  PaxQryParams << QParam("bag_pool_num", otInteger);
  TCachedQuery CachedPaxQry(paxs_sql, PaxQryParams);
  TQuery &PaxQry=CachedPaxQry.get();

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TGrpItem grp;
    grp.airp_arv=Qry.FieldAsString("airp_arv");
    grp.is_unaccomp=Qry.FieldIsNULL("class");
    if (airp_arv!=grp.airp_arv) continue;
    if (is_unaccomp!=grp.is_unaccomp) continue;

    int grp_id=Qry.FieldAsInteger("grp_id");
    int bag_pool_num=Qry.FieldAsInteger("bag_pool_num");
    int bag_num=Qry.FieldAsInteger("bag_num");

    TagQry.SetVariable("grp_id", grp_id);
    TagQry.SetVariable("bag_num", bag_num);
    TagQry.Execute();
    for(;!TagQry.Eof;TagQry.Next())
      grp.tags.insert(TBagTagNumber("", TagQry.FieldAsFloat("no")));

    if (tags!=grp.tags) continue;

    if (!grp.is_unaccomp)
    {
      PaxQry.SetVariable("grp_id", grp_id);
      PaxQry.SetVariable("bag_pool_num", bag_pool_num);
      PaxQry.Execute();
      for(;!PaxQry.Eof;PaxQry.Next())
        grp.paxs.push_back(TPaxItem(PaxQry.FieldAsString("subclass"),
                                    PaxQry.FieldAsString("surname"),
                                    PaxQry.FieldAsString("name")));
      for(vector<TPaxItem>::const_iterator p1=paxs.begin(); p1!=paxs.end(); ++p1)
        for(vector<TPaxItem>::const_iterator p2=grp.paxs.begin(); p2!=grp.paxs.end(); ++p2)
          if (p1->equalRate(*p2)>=6) //6 - ?????? ?????????? ??????? ? ?????
            return true;
    }
    else return true;
  };
  return false;
};

void TGrpItem::normalizeTrfer()
{
  //???? ???? ?????? ???????????? ???????? - ?????? ????? ????? ???????
  //???? ???? ?????????????? ???? - ?????? ????? ? ????
  map<int, CheckIn::TTransferItem>::iterator t=trfer.begin();
  int num=1;
  for(;t!=trfer.end();++t,num++)
    if (num!=t->first ||
        t->second.operFlt.airline_fmt==efmtUnknown ||
        (!t->second.operFlt.suffix.empty() && t->second.operFlt.suffix_fmt==efmtUnknown) ||
        t->second.operFlt.airp_fmt==efmtUnknown ||
        t->second.airp_arv_fmt==efmtUnknown ||
        t->second.subclass_fmt==efmtUnknown) break;
  trfer.erase(t, trfer.end());
}

typedef map<TBagTagNumber, pair< list<TGrpId>,
                                 list<TGrpId> > > TTagMap;

void TraceTagMap(const TTagMap &tags)
{
  ProgTrace(TRACE5, "========== tags: ==========");
  for(TTagMap::const_iterator t=tags.begin(); t!=tags.end(); ++t)
  {
    if (t!=tags.begin())
      ProgTrace(TRACE5, "------------------------------");
    ProgTrace(TRACE5, "%15.0f:", t->first.numeric_part);
    ostringstream s;
    s.str("");
    s << "grps_in: ";
    for(list<TGrpId>::const_iterator g=t->second.first.begin(); g!=t->second.first.end(); ++g)
      s << "(" << g->first << ", " << (g->second==NoExists?"NoExists":IntToString(g->second)) << "); ";
    ProgTrace(TRACE5, "%s", s.str().c_str());
    s.str("");
    s << "grps_out: ";
    for(list<TGrpId>::const_iterator g=t->second.second.begin(); g!=t->second.second.end(); ++g)
      s << "(" << g->first << ", " << (g->second==NoExists?"NoExists":IntToString(g->second)) << "); ";
    ProgTrace(TRACE5, "%s", s.str().c_str());
  };
};

void FilterUnattachedTags(const map<TGrpId, TGrpItem> &grps_in,
                          const map<TGrpId, TGrpItem> &grps_out,
                          TTagMap &tags)
{
  /*
  ProgTrace(TRACE5, "========== grps_in: ==========");
  for(map<TGrpId, TGrpItem>::const_iterator g=grps_in.begin(); g!=grps_in.end(); ++g)
  {
    if (g!=grps_in.begin())
      ProgTrace(TRACE5, "------------------------------");
    ProgTrace(TRACE5, "grp_in(%d, %s):",
                      g->first.first,
                      g->first.second==NoExists?"NoExists":IntToString(g->first.second).c_str());
    g->second.print();
  };

  ProgTrace(TRACE5, "========== grps_out: ==========");
  for(map<TGrpId, TGrpItem>::const_iterator g=grps_out.begin(); g!=grps_out.end(); ++g)
  {
    if (g!=grps_out.begin())
      ProgTrace(TRACE5, "------------------------------");
    ProgTrace(TRACE5, "grp_out(%d, %s):",
                      g->first.first,
                      g->first.second==NoExists?"NoExists":IntToString(g->first.second).c_str());
    g->second.print();
  };

  TraceTagMap(tags);
  */

  for(int pass=0; pass<2; pass++)
  {
    //TDateTime d=NowUTC();
    //ProgTrace(TRACE5, "FilterUnattachedTags: started");
    for(TTagMap::iterator t=tags.begin(); t!=tags.end(); ++t)
    {
      //ProgTrace(TRACE5, "FilterUnattachedTags: no=%s", GetTagRangesStr(t->first).c_str());
      if (t->second.second.empty()) continue;
      for(int minPaxEqualRate=7; minPaxEqualRate>=6; minPaxEqualRate--)   //6 - ?????? ?????????? ??????? ? ?????
      {
        //ProgTrace(TRACE5, "FilterUnattachedTags: minPaxEqualRate=%d", minPaxEqualRate);
        while (!t->second.second.empty())
        {
          pair< list<TGrpId>::iterator, list<TGrpId>::iterator > max(t->second.first.end(), t->second.second.end());
          int maxEqualPaxCount=0;
          for(list<TGrpId>::iterator i1=t->second.first.begin(); i1!=t->second.first.end(); ++i1)
          {
            if (pass==1 && i1->second==NoExists) continue; //?????? ?? BTM
            for(list<TGrpId>::iterator i2=t->second.second.begin(); i2!=t->second.second.end(); ++i2)
            {
              map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(*i1);
              if (grp_in==grps_in.end()) throw Exception("FilterUnattachedTags: grp_in=grps_in.end()");
              TGrpItem joint_grp_in(grp_in->second);
              if (pass==1)
              {
                //???????? ???????????? joint_grp_in.paxs
                for(map<TGrpId, TGrpItem>::const_iterator i3=grps_in.begin(); i3!=grps_in.end(); ++i3)
                {
                  if (i3==grp_in) continue;
                  if (i3->first.first==grp_in->first.first) //????????? grp_id
                  {
                    if (i3->first.second==NoExists)
                      throw Exception("FilterUnattachedTags: i3->first.second=NoExists");
                    if (i3->second.airp_arv!=joint_grp_in.airp_arv ||
                        i3->second.is_unaccomp!=joint_grp_in.is_unaccomp)
                      throw Exception("FilterUnattachedTags: different airp_arv or is_unaccomp");
                    joint_grp_in.paxs.insert(joint_grp_in.paxs.end(),i3->second.paxs.begin(),i3->second.paxs.end());
                  };
                };
              };
              map<TGrpId, TGrpItem>::const_iterator grp_out=grps_out.find(*i2);
              if (grp_out==grps_out.end()) throw Exception("FilterUnattachedTags: grp_out=grps_out.end()");
              int equalPaxCount=joint_grp_in.equalRate(grp_out->second, minPaxEqualRate);
              if (equalPaxCount>maxEqualPaxCount)
              {
                maxEqualPaxCount=equalPaxCount;
                max=make_pair(i1, i2);
              };
            };
          };
          if (maxEqualPaxCount>0)
          {
            //ProgTrace(TRACE5, "FilterUnattachedTags: no=%s minPaxEqualRate=%d [%s]<->[%s]",
            //                  GetTagRangesStr(t->first).c_str(), minPaxEqualRate,
            //                  max.first->getStr().c_str(), max.second->getStr().c_str());
            t->second.first.erase(max.first);
            t->second.second.erase(max.second);
          }
          else break;
        };
      };
    };
  };

  //TraceTagMap(tags);

  //ProgTrace(TRACE5, "FilterUnattachedTags: finished time=%f msecs", (NowUTC()-d)*MSecsPerDay);
};

void GetUnattachedTags(int point_id,
                       TUnattachedTagMap &result)
{
  result.clear();
  TTripInfo flt;
  vector<TrferList::TGrpItem> grps_ckin;
  vector<TrferList::TGrpItem> grps_tlg;
  TrferList::TrferFromDB(TrferList::trferOut, point_id, true, flt, grps_ckin, grps_tlg);

  GetUnattachedTags(point_id, grps_ckin, grps_tlg, result);
};

void GetCheckedTags(int id,  //?.?. point_id ??? grp_id
                    TIdType id_type,
                    const TTagMap &filter_tags,
                    map<TGrpId, TGrpItem> &grps_out)
{
  grps_out.clear();

  if (id_type!=idFlt && id_type!=idGrp)
    throw Exception("GetCheckedTags: wrong id_type");

  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT subclass, surname, name "
    "FROM pax "
    "WHERE grp_id=:grp_id AND bag_pool_num=:bag_pool_num";
  PaxQry.DeclareVariable("grp_id", otInteger);
  PaxQry.DeclareVariable("bag_pool_num", otInteger);

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT pax_grp.grp_id, pax_grp.airp_arv, pax_grp.class, "
    "       bag2.bag_pool_num, bag_tags.no "
    "FROM pax_grp, bag2, bag_tags "
    "WHERE pax_grp.grp_id=bag2.grp_id AND "
    "      bag2.grp_id=bag_tags.grp_id AND "
    "      bag2.num=bag_tags.bag_num AND "
    "      pax_grp.status NOT IN ('E') AND ";
  if (id_type==idFlt)
    sql <<
      "      pax_grp.point_dep=:id ";
  else
    sql <<
      "      pax_grp.grp_id=:id ";
  Qry.SQLText=sql.str();
  Qry.CreateVariable("id", otInteger, id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TGrpId id(Qry.FieldAsInteger("grp_id"),
              Qry.FieldIsNULL("bag_pool_num")?NoExists:Qry.FieldAsInteger("bag_pool_num"));
    if (id.second==NoExists) continue;
    TBagTagNumber tag("", Qry.FieldAsFloat("no"));

    if (!filter_tags.empty() &&
        filter_tags.find(tag)==filter_tags.end()) continue;

    map<TGrpId, TGrpItem>::iterator grp_out=grps_out.find(id);
    if (grp_out==grps_out.end())
    {
      TGrpItem grp;
      grp.airp_arv=Qry.FieldAsString("airp_arv");
      grp.is_unaccomp=Qry.FieldIsNULL("class");
      if (!grp.is_unaccomp)
      {
        PaxQry.SetVariable("grp_id", id.first);
        PaxQry.SetVariable("bag_pool_num", id.second);
        PaxQry.Execute();
        for(;!PaxQry.Eof;PaxQry.Next())
          grp.paxs.push_back(TPaxItem(PaxQry.FieldAsString("subclass"),
                                      PaxQry.FieldAsString("surname"),
                                      PaxQry.FieldAsString("name")));
      };
      grp_out=grps_out.insert(make_pair(id, grp)).first;
    };
    if (grp_out==grps_out.end()) throw Exception("GetCheckedTags: grp_out=grps_out.end()");

    grp_out->second.tags.insert(tag);
  };
};

void GetCheckedTags(int id,  //?.?. point_id ??? grp_id
                    TIdType id_type,
                    map<TGrpId, TGrpItem> &grps_out)
{
  GetCheckedTags(id, id_type, TTagMap(), grps_out);
};

void PrepareTagMapIn(const vector<TrferList::TGrpItem> &grps_ckin,
                     const vector<TrferList::TGrpItem> &grps_tlg,
                     map<TGrpId, TGrpItem> &grps_in,
                     TTagMap &tags)
{
  grps_in.clear();
  tags.clear();
  for(int fromTlg=0; fromTlg<2; fromTlg++)
  {
    vector<TrferList::TGrpItem>::const_iterator g=fromTlg?grps_tlg.begin():grps_ckin.begin();
    for(; g!=(fromTlg?grps_tlg.end():grps_ckin.end()); ++g)
    {
      if (g->tags.empty()) continue;
      TGrpId id(g->grp_id, g->bag_pool_num);
      map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(id);
      if (grp_in==grps_in.end())
        grp_in=grps_in.insert(make_pair(id, TGrpItem(*g))).first;
      if (grp_in==grps_in.end()) throw Exception("PrepareTagMapIn: grp_in=grps_in.end()");

      for(multiset<TBagTagNumber>::const_iterator t=g->tags.begin(); t!=g->tags.end(); ++t)
      {
        TTagMap::iterator tag=tags.find(*t);
        if (tag==tags.end())
          tag=tags.insert(make_pair(*t, pair< list<TGrpId>, list<TGrpId> >())).first;
        if (tag==tags.end()) throw Exception("PrepareTagMapIn: tag==tags.end()");
        tag->second.first.push_back(id);
      };
    };
  };
};

void AddTagMapOut(const map<TGrpId, TGrpItem> &grps_out,
                  TTagMap &tags)
{
  for(map<TGrpId, TGrpItem>::const_iterator g=grps_out.begin(); g!=grps_out.end(); ++g)
  {
    for(multiset<TBagTagNumber>::const_iterator t=g->second.tags.begin(); t!=g->second.tags.end(); ++t)
    {
      TTagMap::iterator tag=tags.find(*t);
      if (tag==tags.end()) continue;

      tag->second.second.push_back(g->first);
    };
  };
};

void LoadPaxLists(int point_id,
                  const TGrpItem &grp_out,
                  const set<int> &pax_out_ids,
                  list<TPaxItem> &paxs_ckin,
                  list<TPaxItem> &paxs_crs)
{
  paxs_ckin.clear();
  paxs_crs.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_id, surname, name "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND airp_arv=:airp_arv AND "
    "      pax_grp.status NOT IN ('E') AND NVL(inbound_confirm,0)=0"; //NVL ????? ??????!!!
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("airp_arv", otString, grp_out.airp_arv);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (pax_out_ids.find(Qry.FieldAsInteger("pax_id"))!=pax_out_ids.end()) continue;
    paxs_ckin.push_back(TPaxItem("",Qry.FieldAsString("surname"),Qry.FieldAsString("name")));
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT crs_pax.pax_id, crs_pax.surname, crs_pax.name "
    "FROM tlg_binding, crs_pnr, crs_pax, pax "
    "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL AND "
    "      tlg_binding.point_id_spp=:point_id AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pr_del=0 ";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (pax_out_ids.find(Qry.FieldAsInteger("pax_id"))!=pax_out_ids.end()) continue;
    paxs_crs.push_back(TPaxItem("",Qry.FieldAsString("surname"),Qry.FieldAsString("name")));
  };
};

string GetConflictStr(const set<ConflictReason> &conflicts)
{
  ostringstream s;
  for(set<ConflictReason>::const_iterator c=conflicts.begin(); c!=conflicts.end(); ++c)
  {
    switch(*c)
    {
      case ConflictReason::InPaxDuplicate:
        s << "InPaxDuplicate; ";
        break;
      case ConflictReason::OutPaxDuplicate:
        s << "OutPaxDuplicate; ";
        break;
      case ConflictReason::InRouteIncomplete:
        s << "InRouteIncomplete; ";
        break;
      case ConflictReason::InRouteDiffer:
        s << "InRouteDiffer; ";
        break;
      case ConflictReason::OutRouteDiffer:
        s << "OutRouteDiffer; ";
        break;
      case ConflictReason::InOutRouteDiffer:
        s << "InOutRouteDiffer; ";
        break;
      case ConflictReason::WeightNotDefined:
        s << "WeightNotDefined; ";
        break;
      case ConflictReason::OutRouteWithErrors:
        s << "OutRouteWithErrors; ";
        break;
      case ConflictReason::OutRouteTruncated:
        s << "OutRouteTruncated; ";
        break;
    };
  };
  return s.str();
};

bool isGlobalConflict(ConflictReason c)
{
   return c==ConflictReason::InRouteIncomplete ||
          c==ConflictReason::InRouteDiffer ||
          c==ConflictReason::OutRouteDiffer ||
          c==ConflictReason::OutRouteWithErrors ||
          c==ConflictReason::OutRouteTruncated;
};

TrferList::Alarm GetConflictAlarm(ConflictReason c)
{
  switch(c)
  {
    case ConflictReason::InPaxDuplicate:      return TrferList::Alarm::InPaxDuplicate;
    case ConflictReason::OutPaxDuplicate:     return TrferList::Alarm::OutPaxDuplicate;
    case ConflictReason::InRouteIncomplete:   return TrferList::Alarm::InRouteIncomplete;
    case ConflictReason::InRouteDiffer:       return TrferList::Alarm::InRouteDiffer;
    case ConflictReason::InOutRouteDiffer:    return TrferList::Alarm::InOutRouteDiffer;
    case ConflictReason::WeightNotDefined:    return TrferList::Alarm::WeightNotDefined;
    case ConflictReason::OutRouteDiffer:      throw Exception("%s: OutRouteDiffer not supported", __func__);
    case ConflictReason::OutRouteWithErrors:  throw Exception("%s: OutRouteWithErrors not supported", __func__);
    case ConflictReason::OutRouteTruncated:   throw Exception("%s: OutRouteTruncated not supported", __func__);
  }
  throw Exception("%s: strange situation!", __func__);
}


void TNewGrpInfo::erase(const TGrpId &id)
{
  tag_map.erase(id);
  for(TNewGrpPaxMap::iterator i=pax_map.begin(); i!=pax_map.end(); ++i)
    i->second.erase(id);
};

int TNewGrpInfo::calc_status(const TGrpId &id) const
{
  TNewGrpTagMap::const_iterator g=tag_map.find(id);
  if (g==tag_map.end()) return NoExists;
  if (!g->second.second.empty()) return NoExists;

  for(set<ConflictReason>::const_iterator c=conflicts.begin();
                                           c!=conflicts.end(); ++c)
  {
    if (isGlobalConflict(*c)) return NoExists;
  };
  return (int)true;
};

void GetNewGrpInfo(int point_id,
                   const TGrpItem &grp_out,
                   const set<int> &pax_out_ids,
                   TNewGrpInfo &info)
{
  info.clear();

  if (grp_out.is_unaccomp) return; //??? ????????????????? ?????? ???? ????????? ?? ?????????????

  TTripInfo flt;
  vector<TrferList::TGrpItem> grps_ckin;
  vector<TrferList::TGrpItem> grps_tlg;
  TrferList::TrferFromDB(TrferList::trferOutForCkin, point_id, true, flt, grps_ckin, grps_tlg);

  if (grps_ckin.empty() && grps_tlg.empty()) return;

  TQuery Qry(&OraSession);
  list<TPaxItem> paxs_ckin, paxs_crs;
  bool paxListsInit=false;
  TGrpItem first_grp_in;
  for(int fromTlg=0; fromTlg<2; fromTlg++)
  {
    vector<TrferList::TGrpItem>::iterator g=fromTlg?grps_tlg.begin():grps_ckin.begin();
    for(; g!=(fromTlg?grps_tlg.end():grps_ckin.end()); ++g)
    {
      if (g->tags.empty()) continue;

      TGrpId grp_in_id(g->grp_id, g->bag_pool_num);
      TGrpItem grp_in(*g);
      if (grp_in.is_unaccomp!=grp_out.is_unaccomp) continue;
      if (grp_in.airp_arv!=grp_out.airp_arv) continue;

      bool grpAlreadyCheckedIn=false;
      for(vector<TPaxItem>::const_iterator paxIn=grp_in.paxs.begin(); paxIn!=grp_in.paxs.end(); ++paxIn)
      {
        for(vector<TPaxItem>::const_iterator paxOut=grp_out.paxs.begin(); paxOut!=grp_out.paxs.end(); ++paxOut)
        {
          if (paxIn->equalRate(*paxOut)>=6) //6 - ?????? ?????????? ??????? ? ?????
          {
            if (grp_in.alreadyCheckedIn(point_id))
            {
              grpAlreadyCheckedIn=true;
              break;
            };

            if (info.tag_map.empty())
            {
              first_grp_in=grp_in;
              int num=1;
              for(map<int, CheckIn::TTransferItem>::const_iterator t=grp_in.trfer.begin(); t!=grp_in.trfer.end(); ++t, num++)
                if (t->first!=num)
                {
                  info.conflicts.insert(ConflictReason::InRouteIncomplete);
                  break;
                };
            };
            TNewGrpTagMap::iterator iTagMap=info.tag_map.find(grp_in_id);
            if (iTagMap==info.tag_map.end())
            {
              //??????? ? tag_map TGrpItem
              iTagMap=info.tag_map.insert( make_pair(grp_in_id, make_pair(*g, set<ConflictReason>()) ) ).first;
              if (iTagMap==info.tag_map.end())
                throw Exception("%s: iTagMap==info.tag_map.end()", __FUNCTION__);
              if (!grp_in.similarTrfer(grp_out, false) ||
                  grp_in.trfer.size()>grp_out.trfer.size())
              {
                //???????????? ?????????? ??????????? ?????????
                iTagMap->second.second.insert(ConflictReason::InOutRouteDiffer);
                info.conflicts.insert(ConflictReason::InOutRouteDiffer);
              };
              int bag_weight=CalcWeightInKilos(g->bag_weight, g->weight_unit);
              if (bag_weight==NoExists || bag_weight<=0)
              {
                //?? ????? ??? ?????? ??? ??? ?????
                iTagMap->second.second.insert(ConflictReason::WeightNotDefined);
                info.conflicts.insert(ConflictReason::WeightNotDefined);
              };
              if (!grp_in.equalTrfer(first_grp_in))
              {
                //???????????? ?????????? ??????????? ????????? 2-? ????
                info.conflicts.insert(ConflictReason::InRouteDiffer);
              };
              if (!paxListsInit)
              {
                LoadPaxLists(point_id, grp_out, pax_out_ids, paxs_ckin, paxs_crs);
                paxListsInit=true;
              };
              for(vector<TPaxItem>::const_iterator p1=grp_in.paxs.begin(); p1!=grp_in.paxs.end(); ++p1)
              {
                list<TPaxItem>::const_iterator p2;
                p2=paxs_ckin.begin();
                for(; p2!=paxs_ckin.end(); ++p2)
                  if (p1->equalRate(*p2)>=6) //6 - ?????? ?????????? ??????? ? ?????
                  {
                    //???? ????????? ? ????? ???????? ? ?????? ????? ??? ??????????????????
                    iTagMap->second.second.insert(ConflictReason::OutPaxDuplicate);
                    info.conflicts.insert(ConflictReason::OutPaxDuplicate);
                    break;
                  };
                if (p2!=paxs_ckin.end()) break;

                p2=paxs_crs.begin();
                for(; p2!=paxs_crs.end(); ++p2)
                {
                  if (p1->equalRate(*p2)>=6) //6 - ?????? ?????????? ??????? ? ?????
                  {
                    //???? ????????? ? ????? ???????? ? ?????? ????? ???????????????
                    iTagMap->second.second.insert(ConflictReason::OutPaxDuplicate);
                    info.conflicts.insert(ConflictReason::OutPaxDuplicate);
                    break;
                  };
                };
                if (p2!=paxs_crs.end()) break;
              };
            };

            //??????? ? pax_map
            pair<string, string> pax_out_id(paxOut->surname, paxOut->name);
            TNewGrpPaxMap::iterator iPaxMap=info.pax_map.find(pax_out_id);
            if (iPaxMap==info.pax_map.end())
              iPaxMap=info.pax_map.insert( make_pair(pax_out_id, set<TGrpId>()) ).first;
            if (iPaxMap==info.pax_map.end())
              throw Exception("%s: iPaxMap==pax_map.end()", __FUNCTION__);
            iPaxMap->second.insert(grp_in_id);
            if (iPaxMap->second.size()>1)
            {
              //???????????? ?????????? ? ?????? ????????? ?????????
              if (iPaxMap->second.size()==2)
              {
                //????? ???????? ??????? ? ?????
                for(set<TGrpId>::const_iterator iGrpId=iPaxMap->second.begin();
                                                iGrpId!=iPaxMap->second.end(); ++iGrpId)
                {
                  TNewGrpTagMap::iterator iTagMap2=info.tag_map.find(*iGrpId);
                  if (iTagMap2==info.tag_map.end())
                    throw Exception("%s: iTagMap2==info.tag_map.end()", __FUNCTION__);
                  iTagMap2->second.second.insert(ConflictReason::InPaxDuplicate);
                };
              }
              else
              {
                //????? ???????? ??????? ? ????????
                iTagMap->second.second.insert(ConflictReason::InPaxDuplicate);
              };
              info.conflicts.insert(ConflictReason::InPaxDuplicate);
            };
          };
        };
        if (grpAlreadyCheckedIn) break;
      };
    };
  };

  if (!info.conflicts.empty())
  {
    ProgTrace(TRACE5, "%s returned conflicts: %s", __FUNCTION__, GetConflictStr(info.conflicts).c_str());
    ProgTrace(TRACE5, "point_id: %d", point_id);
    ProgTrace(TRACE5, "========== grp_out: ==========");
    grp_out.print();

    ProgTrace(TRACE5, "========== tag_map: ==========");
    for(TNewGrpTagMap::const_iterator i=info.tag_map.begin(); i!=info.tag_map.end(); ++i)
    {
      if (i!=info.tag_map.begin())
        ProgTrace(TRACE5, "------------------------------");
      ProgTrace(TRACE5, "grp_in(%d, %s):",
                        i->second.first.grp_id,
                        i->second.first.bag_pool_num==NoExists?"NoExists":IntToString(i->second.first.bag_pool_num).c_str());
      TGrpItem(i->second.first).print();
      ProgTrace(TRACE5, "conflicts: %s", GetConflictStr(i->second.second).c_str());
    };
    ProgTrace(TRACE5, "========== pax_map: ==========");
    for(TNewGrpPaxMap::const_iterator i=info.pax_map.begin(); i!=info.pax_map.end(); ++i)
    {
      ostringstream s;
      for(set<TGrpId>::const_iterator g=i->second.begin(); g!=i->second.end(); ++g)
        s << "(" << g->first << ", " << (g->second==NoExists?"NoExists":IntToString(g->second)) << "); ";
      ProgTrace(TRACE5, "  %s/%s: %s", i->first.first.c_str(), i->first.second.c_str(), s.str().c_str());
    };
  };
};

void NewGrpInfoToGrpsView(const TNewGrpInfo &inbound_trfer,
                          vector<TrferList::TGrpViewItem> &grps)
{
  set<TrferList::Alarm> global_alarms;
  for(set<ConflictReason>::const_iterator c=inbound_trfer.conflicts.begin();
                                           c!=inbound_trfer.conflicts.end(); ++c)
  {
    if (isGlobalConflict(*c))
      global_alarms.insert(GetConflictAlarm(*c));
  };

  vector<TrferList::TGrpItem> grps_ckin;
  vector<TrferList::TGrpItem> grps_tlg;
  TrferList::TAlarmTagMap alarms;
  for(TNewGrpTagMap::const_iterator g=inbound_trfer.tag_map.begin();
                                    g!=inbound_trfer.tag_map.end(); ++g)
  {
    const TrferList::TGrpId &grpId=g->first;
    const TrferList::TGrpItem &grpItem=g->second.first;
    if (grpItem.bag_pool_num!=NoExists) //????????? ????, ?????? ?????: ?? ????????? ??? ?????????????????? ? ?????
      grps_ckin.push_back(grpItem);
    else
      grps_tlg.push_back(grpItem);
    if (!global_alarms.empty() || !g->second.second.empty())
    {
      TrferList::TAlarmTagMap::iterator a=alarms.insert(make_pair(grpId, set<TrferList::Alarm>())).first;
      a->second.insert(global_alarms.begin(), global_alarms.end());
      for(set<ConflictReason>::const_iterator c=g->second.second.begin();
                                               c!=g->second.second.end(); ++c)
      {
        a->second.insert(GetConflictAlarm(*c));
      };
    };
  };

  TrferList::GrpsToGrpsView(TrferList::trferOutForCkin, true, grps_ckin, grps_tlg, alarms, grps);
};

void ConflictReasons::toLog(const TLogLocale &pattern) const
{
  TLogLocale tlocale(pattern);
  for(std::set<ConflictReason>::const_iterator c=conflicts.begin(); c!=conflicts.end(); ++c)
  {
    tlocale.lexema_id.clear();
    switch(*c)
    {
      case ConflictReason::InPaxDuplicate:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.IN_PAX_DUPLICATE";
        break;
      case ConflictReason::OutPaxDuplicate:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.OUT_PAX_DUPLICATE";
        break;
      case ConflictReason::InRouteIncomplete:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.IN_ROUTE_INCOMPLETE";
        break;
      case ConflictReason::InRouteDiffer:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.IN_ROUTE_DIFFER";
        break;
      case ConflictReason::OutRouteDiffer:
        tlocale.lexema_id = emptyInboundBaggage?
                              "EVT.TRFER_CONFLICT_REASON.OUT_ROUTE_DIFFER":
                              "EVT.INBOUND_TRFER_CONFLICT_REASON.OUT_ROUTE_DIFFER";
        break;
      case ConflictReason::InOutRouteDiffer:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.IN_OUT_ROUTE_DIFFER";
        break;
      case ConflictReason::WeightNotDefined:
        tlocale.lexema_id = "EVT.INBOUND_TRFER_CONFLICT_REASON.WEIGHT_NOT_DEFINED";
        break;
      case ConflictReason::OutRouteWithErrors:
        tlocale.lexema_id = emptyInboundBaggage?
                              "EVT.TRFER_CONFLICT_REASON.OUT_ROUTE_WITH_ERRORS":
                              "EVT.INBOUND_TRFER_CONFLICT_REASON.OUT_ROUTE_WITH_ERRORS";
        break;
      case ConflictReason::OutRouteTruncated:
        tlocale.lexema_id = "EVT.TRFER_CONFLICT_REASON.OUT_ROUTE_TRUNCATED";
        break;
    };
    TReqInfo::Instance()->LocaleToLog(tlocale);
  };
};

void GetUnattachedTags(int point_id,
                       const vector<TrferList::TGrpItem> &grps_ckin,
                       const vector<TrferList::TGrpItem> &grps_tlg,
                       TUnattachedTagMap &result)
{
  result.clear();

  TTagMap tags;
  map<TGrpId, TGrpItem> grps_in;

  PrepareTagMapIn(grps_ckin, grps_tlg, grps_in, tags);

  if (grps_in.empty() || tags.empty()) return;

  map<TGrpId, TGrpItem> grps_out;
  GetCheckedTags(point_id, idFlt, tags, grps_out);

  AddTagMapOut(grps_out, tags);

  FilterUnattachedTags(grps_in, grps_out, tags);

  for(TTagMap::const_iterator t=tags.begin(); t!=tags.end(); ++t)
  {
    for(list<TGrpId>::const_iterator g=t->second.first.begin(); g!=t->second.first.end(); ++g)
    {
      TUnattachedTagMap::iterator r=result.find(*g);
      if (r==result.end())
        r=result.insert(make_pair(*g, list<TBagTagNumber>())).first;
      if (r==result.end()) throw Exception("GetUnattachedTags: r==result.end()");
      r->second.push_back(t->first);
    };
  };
/*
  for(TUnattachedTagMap::const_iterator r=result.begin(); r!=result.end(); ++r)
  {
    for(list<TBagTagNumber>::const_iterator t=r->second.begin(); t!=r->second.end(); ++t)
      ProgTrace(TRACE5, "%s: %s",
                        r->first.getStr().c_str(),
                        GetTagRangesStr(*t).c_str());
  };
*/
};

void GetNextTrferCheckedFlts(int id,
                             TIdType id_type,
                             set<int> &point_ids)
{
  point_ids.clear();

  if (id_type!=idFlt && id_type!=idGrp)
    throw Exception("GetNextTrferCheckedFlts: wrong id_type");

  TQuery Qry(&OraSession);
  Qry.Clear();

  ostringstream sql;
  sql << "SELECT trfer_trips.point_id_spp "
         "FROM transfer, trfer_trips, trip_stages ";
  if (id_type==idFlt)
    sql << ", pax_grp ";

  sql << "WHERE trfer_trips.point_id=transfer.point_id_trfer AND "
         "      trfer_trips.point_id_spp=trip_stages.point_id AND ";

  if (id_type==idFlt)
    sql << "      transfer.grp_id=pax_grp.grp_id AND "
           "      pax_grp.point_dep=:id AND ";
  if (id_type==idGrp)
    sql << "      transfer.grp_id=:id AND ";

  sql << "      transfer.transfer_num=1 AND "
         "      trip_stages.stage_id=:stage_id AND "
         "      trip_stages.act IS NOT NULL ";
/*
  //??? ????? ????????? ??????, ?? ?? ??????? ????? ????????????? ????????? ? ?????? ?????? ?????????
  //??????? ???? ?????? ????? ? ?????????? ????? ??????? SQL ??????
  sql << "SELECT DISTINCT trfer_trips.point_id_spp "
         "FROM trfer_trips, transfer, pax_grp, pax "
         "WHERE trfer_trips.point_id=transfer.point_id_trfer AND "
         "      transfer.grp_id=pax_grp.grp_id AND ";
  if (id_type==idFlt)
    sql << "      pax_grp.grp_id=pax.grp_id(+) AND "
           "      pax_grp.point_dep=:id AND ";
  if (id_type==idGrp)
    sql << "      pax_grp.grp_id=pax.grp_id(+) AND "
           "      pax_grp.grp_id=:id AND ";
  if (id_type==idPax)
    sql << "      pax_grp.grp_id=pax.grp_id AND "
           "      pax.pax_id=:id AND ";
  sql << "      transfer.transfer_num=1 AND "
         "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') AND "
         "      (pax_grp.class IS NULL OR pax.pr_brd=1 AND pax.bag_pool_num IS NOT NULL) ";
*/
  Qry.SQLText=sql.str();
  Qry.CreateVariable("id", otInteger, id);
  Qry.CreateVariable("stage_id", otInteger, sCloseCheckIn);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    point_ids.insert(Qry.FieldAsInteger("point_id_spp"));

};

}; //namespace InboundTrfer


