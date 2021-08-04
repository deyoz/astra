#include "transfer.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "exceptions.h"
#include "term_version.h"
#include "alarms.h"
#include "qrys.h"
#include "typeb_utils.h"
#include "tlg/typeb_db.h"

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
  DB::TCachedQuery Qry(PgOra::getROSession("TRANSFER_SUBCLS"),
                   "SELECT transfer_num,subclass,subclass_fmt "
                   "FROM transfer_subcls "
                   "WHERE pax_id=:pax_id "
                   "ORDER BY transfer_num",
                   QParams() << QParam("pax_id", otInteger, pax_id),
                   STDLOG);
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

  DB::TQuery TrferDelQry(PgOra::getRWSession("TRANSFER_SUBCLS"),STDLOG);
  TrferDelQry.SQLText="DELETE FROM transfer_subcls "
                      "WHERE pax_id=:pax_id";
  TrferDelQry.CreateVariable("pax_id",otInteger,pax_id);
  TrferDelQry.Execute();

  DB::TQuery TrferInsQry(PgOra::getRWSession("TRANSFER_SUBCLS"),STDLOG);
  TrferInsQry.SQLText=
    "INSERT INTO transfer_subcls(pax_id,transfer_num,subclass,subclass_fmt) "
    "VALUES (:pax_id,:transfer_num,:subclass,:subclass_fmt)";
  TrferInsQry.CreateVariable("pax_id",otInteger,pax_id);
  TrferInsQry.DeclareVariable("transfer_num",otInteger);
  TrferInsQry.DeclareVariable("subclass",otString);
  TrferInsQry.DeclareVariable("subclass_fmt",otInteger);

  int trfer_num=1;
  for(CheckIn::TTransferList::const_iterator t=firstTrfer;t!=trfer.end();t++,trfer_num++)
  {
    const CheckIn::TPaxTransferItem &pax=t->pax.at(pax_no-1);
    TrferInsQry.SetVariable("transfer_num",trfer_num);
    TrferInsQry.SetVariable("subclass",pax.subclass);
    TrferInsQry.SetVariable("subclass_fmt",(int)pax.subclass_fmt);
    TrferInsQry.Execute();
  }
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
    }


    int local_date=NodeAsIntegerFast("local_date",node2);
    try
    {
      TDateTime base_date=local_scd-1; //��⠬��� ����� �� ������ ����� � ���ਪ� �� ���譨� ����
      local_scd=DayToDate(local_date,base_date,false); //�����쭠� ��� �뫥�
      seg.operFlt.scd_out=local_scd;
    }
    catch(const EXCEPTIONS::EConvertError &E)
    {
      throw UserException("MSG.TRANSFER_FLIGHT.INVALID_LOCAL_DATE_DEP",
                          LParams()<<LParam("flight",flt.str()));
    }

    //��ய��� �뫥�
    if (GetNodeFast("airp_dep",node2)!=nullptr)  //����� �/� �뫥�
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

    push_back(seg);
  }
}

void TTransferList::parseSubclasses(xmlNodePtr paxNode)
{
  if (paxNode==nullptr) return;

  //�࠭���� ��������� ���ᠦ�஢
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
      //����� �����: ���-�� �࠭����� �������ᮢ ���ᠦ�� �� ᮢ������ � ���-��� �࠭����� ᥣ���⮢
      throw EXCEPTIONS::Exception("ParseTransfer: Different number of transfer subclasses and transfer segments");
    }
  }
}

FltOperFilter createFltOperFilter(const CheckIn::TTransferItem &item)
{
    return {AirlineCode_t(item.operFlt.airline),
            FlightNumber_t(item.operFlt.flt_no),
            FlightSuffix_t(item.operFlt.suffix),
            AirportCode_t(item.operFlt.airp),
            item.operFlt.scd_out,
            FltOperFilter::DateType::Local,
            {},
            FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn)};
}

TAdvTripInfo routeInfoFromTrfr(const CheckIn::TTransferItem& seg)
{
    std::list<TAdvTripInfo> flt=createFltOperFilter(seg).search();
    if(flt.empty() || flt.size() > 1) {
        LogTrace(TRACE5) << __FUNCTION__ << " Search filter error: ";
        throw Exception("Search error");
    }
    return flt.front();
}

} //namespace CheckIn


namespace TrferList
{

std::ostream& operator << (std::ostream& os, const TrferList::Alarm alarm)
{
  os << std::underlying_type<TrferList::Alarm>::type(alarm);
  return os;
}

TBagItem& TBagItem::setZero()
{
  clear();
  bag_amount=0;
  bag_weight=0;
  rk_weight=0;
  weight_unit='K';
  return *this;
};

TBagItem& TBagItem::fromDB(DB::TQuery &Qry, bool fromTlg, bool loadTags)
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
      DB::TQuery TagQry(PgOra::getROSession("TRFER_TAGS"), STDLOG);
      TagQry.SQLText="SELECT no FROM trfer_tags "
                     "WHERE grp_id=:grp_id";
      TagQry.DeclareVariable("grp_id", otInteger);
      TagQry.SetVariable("grp_id", Qry.FieldAsInteger("grp_id"));
      TagQry.Execute();
      for(;!TagQry.Eof;TagQry.Next())
        tags.insert(TBagTagNumber("",TagQry.FieldAsFloat("no")));
    }
    else
    {
      if (Qry.FieldIsNULL("bag_pool_num")) return *this;

      DB::TQuery TagQry(PgOra::getROSession("ORACLE"), STDLOG);
      TagQry.SQLText="SELECT bag_tags.no "
                     "FROM bag_tags, bag2 "
                     "WHERE bag_tags.grp_id=bag2.grp_id(+) AND "
                     "      bag_tags.bag_num=bag2.num(+) AND "
                     "      bag_tags.grp_id=:grp_id AND "
                     "      COALESCE(bag2.bag_pool_num,1)=:bag_pool_num";
      TagQry.DeclareVariable("grp_id", otInteger);
      TagQry.DeclareVariable("bag_pool_num", otInteger);
      if (!Qry.FieldIsNULL("bag_pool_num"))
        TagQry.SetVariable("bag_pool_num", Qry.FieldAsInteger("bag_pool_num"));
      else
        TagQry.SetVariable("bag_pool_num", FNull);
      TagQry.SetVariable("grp_id", Qry.FieldAsInteger("grp_id"));
      TagQry.Execute();
      for(;!TagQry.Eof;TagQry.Next())
        tags.insert(TBagTagNumber("",TagQry.FieldAsFloat("no")));
    };
  };
  return *this;
};

TPaxItem& TPaxItem::fromDB(DB::TQuery &Qry, bool fromTlg)
{
  clear();
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (!fromTlg)
  {
    seats=Qry.FieldAsInteger("seats");
    if (seats>1)
    {
      DB::TQuery RemQry(PgOra::getROSession("PAX_REM"), STDLOG);
      RemQry.SQLText =
          "SELECT rem_code "
          "FROM pax_rem "
          "WHERE pax_id=:pax_id AND rem_code IN ('STCR', 'EXST') "
          "ORDER BY CASE WHEN rem_code='STCR' THEN 0 "
          "              WHEN rem_code='EXST' THEN 1 "
          "              ELSE 2 END ";
      RemQry.DeclareVariable("pax_id", otInteger);
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

TGrpItem& TGrpItem::paxFromDB(DB::TQuery &PaxQry, bool fromTlg)
{
  paxs.clear();
  for(;!PaxQry.Eof;PaxQry.Next())
    paxs.push_back(TPaxItem().fromDB(PaxQry, fromTlg));

  if (!fromTlg)
  {
    seats=0;
    for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
      seats+=p->seats;

    if (subcl.empty())
    {
      //���᫥��� �������� ����� grp.subcl �� �᭮�� �������ᮢ ���ᠦ�஢
      for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
      {
        try
        {
          //���᫪��� �������� ����� grp.subcl
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
    //� �������� �᫨ ��஢�� PTM, ���ਬ��
    if (paxs.empty()) paxs.push_back(TPaxItem());
  };
  return *this;
};

TGrpItem& TGrpItem::trferFromDB(bool fromTlg)
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

  auto &db_session = fromTlg ? PgOra::getROSession("TLG_TRFER_ONWARDS")
                             : PgOra::getROSession({"TRANSFER","TRFER_TRIPS"});

  DB::TQuery TrferQry(db_session, STDLOG);
  TrferQry.SQLText=fromTlg?trfer_sql_tlg:trfer_sql_ckin;
  TrferQry.DeclareVariable("grp_id", otInteger);
  TrferQry.SetVariable("grp_id", grp_id);
  TrferQry.Execute();
  for(;!TrferQry.Eof;TrferQry.Next())
  {
    CheckIn::TTransferItem t;
    t.operFlt.Init(TrferQry);
    t.airp_arv=TrferQry.FieldAsString("airp_arv");
    //t.subclass=TrferQry.FieldAsString("subclass"); �� �� ����㦠��, ⠪ ��� �� ������ � �ࠢ�����
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

namespace {

struct TlgTransfer
{
  TrferId_t trfer_id;
  TlgId_t tlg_id;
  PointIdTlg_t point_id_in;
  PointIdTlg_t point_id_out;
  std::string subcl_in;
  std::string subcl_out;
};

enum class TrferPointType
{
  IN,
  OUT
};

std::vector<TlgTransfer> loadTlgTransfer(const PointIdTlg_t& point_id,
                                         TrferPointType point_type)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  std::vector<TlgTransfer> result;
  int trfer_id = ASTRA::NoExists;
  int tlg_id = ASTRA::NoExists;
  int point_id_in = ASTRA::NoExists;
  int point_id_out = ASTRA::NoExists;
  std::string subcl_in;
  std::string subcl_out;
  auto cur = make_db_curs(
        "SELECT trfer_id, tlg_id, point_id_in, point_id_out, subcl_in, subcl_out "
        "FROM tlg_transfer "
        "WHERE " +
        std::string(point_type == TrferPointType::IN ? "point_id_in=:point_id "
                                                     : "point_id_out=:point_id "),
        PgOra::getROSession("TLG_TRANSFER"));

  cur.stb()
      .def(trfer_id)
      .def(tlg_id)
      .def(point_id_in)
      .def(point_id_out)
      .defNull(subcl_in, std::string())
      .defNull(subcl_out, std::string())
      .bind(":point_id", point_id.get())
      .exec();

  while (!cur.fen()) {
    result.push_back(
          TlgTransfer{
            TrferId_t(trfer_id),
            TlgId_t(tlg_id),
            PointIdTlg_t(point_id_in),
            PointIdTlg_t(point_id_out),
            subcl_in,
            subcl_out
          });
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

bool existsTlgIn(const TlgId_t& tlg_id, const std::string& tlg_type)
{
  auto cur = make_db_curs(
        "SELECT 1 FROM tlgs_in "
        "WHERE id=:tlg_id "
        "AND type=:tlg_type "
        "FETCH FIRST 1 ROWS ONLY ",
        PgOra::getROSession("TLGS_IN"));
  cur
      .stb()
      .bind(":tlg_id", tlg_id.get())
      .bind(":tlg_type", tlg_type)
      .EXfet();
  return cur.err() != DbCpp::ResultCode::NoDataFound;
}

std::vector<TlgTransfer> loadTlgTransfer(const PointIdTlg_t& point_id,
                                         TrferPointType point_type,
                                         const std::string& tlg_type)
{
  std::vector<TlgTransfer> result;
  const std::vector<TlgTransfer> tlg_transfers = loadTlgTransfer(point_id, point_type);
  for (const TlgTransfer& tlg_transfer: tlg_transfers) {
    if (existsTlgIn(tlg_transfer.tlg_id, tlg_type)) {
      result.push_back(tlg_transfer);
    }
  }
  return result;
}

struct TlgTrip
{
  std::string airline;
  std::string airp_arv;
  std::string airp_dep;
  int bind_type;
  int flt_no;
  PointIdTlg_t point_id;
  int pr_utc;
  TDateTime scd;
  std::string suffix;

  static std::optional<TlgTrip> load(const PointIdTlg_t& point_id);
};

std::optional<TlgTrip> TlgTrip::load(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  std::string airline;
  std::string airp_arv;
  std::string airp_dep;
  int bind_type = ASTRA::NoExists;
  int flt_no = ASTRA::NoExists;
  int pr_utc = ASTRA::NoExists;
  Dates::DateTime_t scd;
  std::string suffix;

  auto cur = make_db_curs(
        "SELECT airline, airp_arv, airp_dep, bind_type, flt_no, pr_utc, scd, suffix "
        "FROM tlg_trips "
        "WHERE point_id=:point_id ",
        PgOra::getROSession("TLG_TRIPS"));

  cur.stb()
      .def(airline)
      .defNull(airp_arv, std::string())
      .defNull(airp_dep, std::string())
      .def(bind_type)
      .def(flt_no)
      .def(pr_utc)
      .def(scd)
      .defNull(suffix, std::string())
      .bind(":point_id", point_id.get())
      .EXfet();

  if (cur.err() == DbCpp::ResultCode::NoDataFound) {
    return {};
  }
  return TlgTrip {
    airline,
    airp_arv,
    airp_dep,
    bind_type,
    flt_no,
    point_id,
    pr_utc,
    scd.is_special() ? ASTRA::NoExists : BoostToDateTime(scd),
    suffix
  };
}

struct TrferGrp
{
  int bag_amount;
  int bag_weight;
  TrferGrpId_t grp_id;
  int rk_weight;
  int seats;
  TrferId_t trfer_id;
  std::string weight_unit;

  static std::vector<TrferGrp> load(const TrferId_t& trfer_id);
};

std::vector<TrferGrp> TrferGrp::load(const TrferId_t& trfer_id)
{
  LogTrace(TRACE6) << __func__
                   << ": trfer_id=" << trfer_id;
  std::vector<TrferGrp> result;
  int bag_amount = ASTRA::NoExists;
  int bag_weight = ASTRA::NoExists;
  int grp_id = ASTRA::NoExists;
  int rk_weight = ASTRA::NoExists;
  int seats = ASTRA::NoExists;
  std::string weight_unit;

  auto cur = make_db_curs(
        "SELECT bag_amount, bag_weight, grp_id, rk_weight, seats, weight_unit "
        "FROM trfer_grp "
        "WHERE trfer_id=:trfer_id ",
        PgOra::getROSession("TRFER_GRP"));

  cur.stb()
      .defNull(bag_amount, ASTRA::NoExists)
      .defNull(bag_weight, ASTRA::NoExists)
      .def(grp_id)
      .defNull(rk_weight, ASTRA::NoExists)
      .defNull(seats, ASTRA::NoExists)
      .defNull(weight_unit, std::string())
      .bind(":trfer_id", trfer_id.get())
      .exec();

  while (!cur.fen()) {
    result.push_back(
          TrferGrp{
            bag_amount,
            bag_weight,
            TrferGrpId_t(grp_id),
            rk_weight,
            seats,
            trfer_id,
            weight_unit
          });
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

struct TlgTransferGrpData {
  TlgTransfer tlg_transfer;
  TlgTrip tlg_trip_in;
  TlgTrip tlg_trip_out;
  TrferGrp trfer_grp;

  static std::vector<TlgTransferGrpData> load(const PointId_t& point_id,
                                              TrferPointType point_type,
                                              const string& tlg_type);
};

std::vector<TlgTransferGrpData> TlgTransferGrpData::load(const PointId_t& point_id,
                                                         TrferPointType point_type,
                                                         const std::string& tlg_type)
{
  std::vector<TlgTransferGrpData> result;
  const std::set<PointIdTlg_t> point_ids_tlg =
      getPointIdTlgByPointIdsSpp(PointId_t(point_id));
  for (const PointIdTlg_t& point_id_tlg: point_ids_tlg) {
    const std::vector<TlgTransfer> tlg_transfers = loadTlgTransfer(PointIdTlg_t(point_id_tlg),
                                                                   point_type);
    for (const TlgTransfer& tlg_transfer: tlg_transfers) {
      if (!existsTlgIn(tlg_transfer.tlg_id, tlg_type)) {
        continue;
      }
      const std::optional<TlgTrip> tlg_trip_in  = TlgTrip::load(tlg_transfer.point_id_in);
      if (!tlg_trip_in) {
        continue;
      }
      const std::optional<TlgTrip> tlg_trip_out = TlgTrip::load(tlg_transfer.point_id_out);
      if (!tlg_trip_out) {
        continue;
      }
      const std::vector<TrferGrp> trfer_grps = TrferGrp::load(tlg_transfer.trfer_id);
      for (const TrferGrp& trfer_grp: trfer_grps) {
        const TlgTransferGrpData item = {
          tlg_transfer,
          *tlg_trip_in,
          *tlg_trip_out,
          trfer_grp
        };
        result.push_back(item);
      }
    }
  }
  return result;
}

std::optional<TrferPointType> getPointType(TTrferType type)
{
  if (type==trferOut || type==trferOutForCkin) {
    return TrferPointType::OUT;
  }
  if (type==trferIn) {
    return TrferPointType::IN;
  }
  return {};
}

TFltInfo makeFltInfo(const TlgTrip& tlg_trip)
{
  TFltInfo result;
  result.airline = tlg_trip.airline;
  result.flt_no = tlg_trip.flt_no;
  result.suffix = tlg_trip.suffix;
  result.airp = tlg_trip.airp_dep;
  result.scd_out = tlg_trip.scd;
  result.point_id = tlg_trip.point_id.get();
  return result;
}

const std::vector<TlgTransferGrpData> filterTlgTransferGrp(
    const std::vector<TlgTransferGrpData> &items,
    TTrferType type,
    std::set<TFltInfo>& flts_from_ckin)
{
  std::vector<TlgTransferGrpData> result;
  std::set<TFltInfo> flts_from_tlg;
  for(const TlgTransferGrpData& item: items)
  {
    TFltInfo flt1 = makeFltInfo(item.tlg_trip_in);
    if (type==trferIn) {
      flt1.point_id = item.tlg_trip_out.point_id.get();
    }
    std::set<TFltInfo>::const_iterator id;
    id=flts_from_ckin.find(flt1);
    if (id!=flts_from_ckin.end())
    {
      //����� ⥫��ࠬ��� ३� ���㦨������ � ����
      continue;
    };

    id=flts_from_tlg.find(flt1);
    if (id==flts_from_tlg.end())
    {
      //���� ���ଠ樨 �� flt1 - �ॡ���� �஢�ઠ ���㦨����� � ����
      FltOperFilter filter(AirlineCode_t(flt1.airline),
                           FlightNumber_t(flt1.flt_no),
                           FlightSuffix_t(flt1.suffix),
                           AirportCode_t(flt1.airp),
                           flt1.scd_out,
                           FltOperFilter::DateType::Local);

      filter.setAdditionalWhere(
        " AND EXISTS(SELECT pax_grp.point_dep "
        "            FROM pax_grp "
        "            WHERE pax_grp.point_dep=points.point_id AND pax_grp.status NOT IN ('E'))");

      //�饬 ३� � ���
      std::list<TAdvTripInfo> flts=filter.search();
      std::list<TAdvTripInfo>::const_iterator f=flts.begin();
      for(; f!=flts.end(); ++f)
      {
        TFltInfo flt2(*f, true);
        if (!(flt1<flt2) && !(flt2<flt1)) break;
      };

      if (f==flts.end())
        //�� ��諨 ३� - ����� �� ���㦨������ � ����
        id=flts_from_tlg.insert(flt1).first;
      else
        //����� ⥫��ࠬ��� ३� ���㦨������ � ����
        flts_from_ckin.insert(flt1);
    };
    if (id==flts_from_tlg.end()) continue;

    result.push_back(item);
  }
  return result;
}

bool isUnaccompGrp(int grp_id)
{
  DB::TQuery ExceptQry(PgOra::getROSession("TLG_TRFER_EXCEPTS"), STDLOG);
  ExceptQry.SQLText=
    "SELECT type "
    "FROM tlg_trfer_excepts "
    "WHERE grp_id=:grp_id "
    "AND type IN ('UNAC') "
    "FETCH FIRST 1 ROWS ONLY ";
  ExceptQry.DeclareVariable("grp_id", otInteger);
  ExceptQry.SetVariable("grp_id", grp_id);
  ExceptQry.Execute();
  return !ExceptQry.Eof;
}

std::vector<TPaxItem> loadTrferPax(const TrferGrpId_t& grp_id)
{
  std::vector<TPaxItem> result;
  DB::TQuery PaxQry(PgOra::getROSession("TRFER_PAX"), STDLOG);
  PaxQry.SQLText=
    "SELECT surname, name "
    "FROM trfer_pax "
    "WHERE grp_id=:grp_id";
  PaxQry.DeclareVariable("grp_id", otInteger);
  PaxQry.SetVariable("grp_id", grp_id.get());
  PaxQry.Execute();
  for(;!PaxQry.Eof;PaxQry.Next()) {
    result.push_back(TPaxItem().fromDB(PaxQry, true));
  }
  //���� �᫨ ����� ��㯯� - ����頥� � paxs ���⮩ surname
  if (result.empty()) {
    result.push_back(TPaxItem());
  }
  return result;
}

std::multiset<TBagTagNumber> loadTrferTag(const TrferGrpId_t& grp_id)
{
  std::multiset<TBagTagNumber> result;
  DB::TQuery TagQry(PgOra::getROSession("TRFER_TAGS"), STDLOG);
  TagQry.SQLText="SELECT no FROM trfer_tags "
                 "WHERE grp_id=:grp_id";
  TagQry.DeclareVariable("grp_id", otInteger);
  TagQry.SetVariable("grp_id", grp_id.get());
  TagQry.Execute();
  for(;!TagQry.Eof;TagQry.Next()) {
    result.insert(TBagTagNumber("",TagQry.FieldAsFloat("no")));
  }
  return result;
}

} // namespace

std::vector<TGrpItem> TlgTransferGrpFromDB(TTrferType type,
                                           int point_id,
                                           bool pr_bag,
                                           std::set<TFltInfo>& flts_from_ckin,
                                           const TTripRouteItem& priorAirp)
{
  //point_id ᮤ�ন� �㭪� �ਫ�� ��� trferIn
  std::vector<TGrpItem> result;
  const std::optional<TrferPointType> point_type = getPointType(type);
  if (!point_type) {
    return {};
  }
  const PointId_t point_id_spp(*point_type == TrferPointType::OUT ? point_id
                                                                  : priorAirp.point_id);
  const std::string tlg_type = pr_bag ? "BTM" : "PTM";
  const std::vector<TlgTransferGrpData> items =
      filterTlgTransferGrp(TlgTransferGrpData::load(point_id_spp, *point_type, tlg_type),
                           type,
                           flts_from_ckin);

  for(const TlgTransferGrpData& item: items)
  {
    TGrpItem grp;
    grp.grp_id=item.trfer_grp.grp_id.get();
    if (type==trferOut || type==trferOutForCkin) {
      grp.point_id=item.tlg_trip_in.point_id.get();
      grp.airp_arv=item.tlg_trip_in.airp_arv;
      grp.last_airp_arv=item.tlg_trip_out.airp_arv;
      grp.subcl=item.tlg_transfer.subcl_in;
    } else if (type==trferIn) {
      grp.point_id=item.tlg_trip_out.point_id.get();
      grp.airp_arv=item.tlg_trip_out.airp_arv;
      grp.subcl=item.tlg_transfer.subcl_out;
    }
    grp.bag_amount=item.trfer_grp.bag_amount;
    grp.bag_weight=item.trfer_grp.bag_weight;
    grp.rk_weight=item.trfer_grp.rk_weight;
    grp.weight_unit=item.trfer_grp.weight_unit;
    grp.seats=item.trfer_grp.seats;
    //���ᠦ��� ��㯯�
    //���� �᫨ ����� ��㯯� - ����頥� � paxs ���⮩ surname
    grp.paxs = loadTrferPax(TrferGrpId_t(grp.grp_id));
    if (pr_bag)
    {
      grp.tags = loadTrferTag(TrferGrpId_t(grp.grp_id));
      //��।��塞 ��ᮯ஢������� ����� - �� ����� ���� ⮫쪮 � BTM
      grp.is_unaccomp = isUnaccompGrp(grp.grp_id);
      if (!grp.is_unaccomp) {
        //�஢�ਬ UNACCOMPANIED
        if (grp.subcl.empty() &&
            grp.paxs.size()==1 &&
            grp.paxs.begin()->surname=="UNACCOMPANIED" &&
            grp.paxs.begin()->name.empty())
        {
          grp.is_unaccomp=true;
        }
      }
    }
    if (type==trferOutForCkin) {
      grp.trferFromDB(true);
    }
    result.push_back(grp);
  };
  return result;
}

DbCpp::Session& getSessionForTrferQry(TTrferType type)
{
  if (type == trferIn || type == trferCkin) {
    return PgOra::getROSession({"PAX","PAX_GRP","BAG2","TRANSFER"});
  }
  if (type == trferOut || type == trferOutForCkin) {
    return PgOra::getROSession({"PAX","PAX_GRP","BAG2","TRANSFER","TRFER_TRIPS"});
  }
  if (type == tckinInbound)
  {
    return PgOra::getROSession({"PAX","PAX_GRP","BAG2","TCKIN_PAX_GRP"});
  }
  return PgOra::getROSession("ORACLE");
}

void TrferFromDB(TTrferType type,
                 int point_id,   //point_id ᮤ�ন� �㭪� �ਫ�� ��� trferIn
                 bool pr_bag,
                 TTripInfo &flt,
                 vector<TGrpItem> &grps_ckin,
                 vector<TGrpItem> &grps_tlg)
{
  flt.Clear();
  grps_ckin.clear();
  grps_tlg.clear();

  set<TFltInfo> flts_from_ckin;
  map<int, TFltInfo> spp_point_ids_from_ckin;

  DB::TQuery Qry(getSessionForTrferQry(type), STDLOG);

  TTripRouteItem priorAirp;
  if (type==trferIn)
  {
    //point_id ᮤ�ন� �㭪� �ਫ��, � ��� �㦥� �।��騩 �㭪� �뫥�
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

  ostringstream sql;
  sql << "SELECT "
         "pax_grp.point_dep, "
         "pax_grp.grp_id, "
         "CASE WHEN pax.grp_id IS NULL THEN bag2.bag_pool_num ELSE pax.bag_pool_num END AS bag_pool_num, "
         "COALESCE(pax.cabin_class, pax_grp.class) AS class, "
         "SUM(CASE WHEN (pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL) THEN 0 "
         "  ELSE CASE "
         "    WHEN bag2.pr_cabin IS NULL THEN 0 "
         "    WHEN bag2.pr_cabin = 0 THEN bag2.amount "
         "    ELSE 0 "
         "  END "
         "END) AS bag_amount, "
         "SUM(CASE WHEN (pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL) THEN 0 "
         "  ELSE CASE "
         "    WHEN bag2.pr_cabin IS NULL THEN 0 "
         "    WHEN bag2.pr_cabin = 0 THEN bag2.weight "
         "    ELSE 0 "
         "  END "
         "END) AS bag_weight, "
         "SUM(CASE WHEN (pax.grp_id IS NOT NULL AND pax.bag_pool_num IS NULL) THEN 0 "
         "  ELSE CASE "
         "    WHEN bag2.pr_cabin IS NULL THEN 0 "
         "    WHEN bag2.pr_cabin = 0 THEN 0 "
         "    ELSE bag2.weight "
         "  END "
         "END) AS rk_weight, ";

  if (type==trferOut || type==trferOutForCkin)
  {
    //���ଠ�� � �࠭��୮� ������/���ᠦ���, ��ࠢ���饬�� ३ᮬ
    sql << "pax_grp.point_dep AS point_id, pax_grp.airp_arv, "
           "transfer.airp_arv AS last_airp_arv "
           "FROM trfer_trips "
           "  JOIN transfer ON trfer_trips.point_id = transfer.point_id_trfer "
           "  JOIN (pax_grp "
           "      LEFT OUTER JOIN bag2 ON pax_grp.grp_id = bag2.grp_id "
           "      LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
           "  ) ON transfer.grp_id = pax_grp.grp_id "
           "WHERE "
           "  (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL) "
           "  AND trfer_trips.point_id_spp = :point_id "
           "  AND transfer.transfer_num = 1 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferIn)
  {
    //���ଠ�� � �࠭��୮� ������/���ᠦ���, �ਡ뢠�饬 ३ᮬ
    sql << "transfer.point_id_trfer AS point_id, transfer.airp_arv "
           "FROM pax_grp "
           "  LEFT OUTER JOIN bag2 ON pax_grp.grp_id = bag2.grp_id "
           "  LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
           "  JOIN transfer ON pax_grp.grp_id = transfer.grp_id "
           "WHERE "
           "  (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL) "
           "  AND pax_grp.point_arv = :point_id "
           "  AND transfer.transfer_num = 1 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferCkin)
  {
    //���ଠ�� � �࠭��୮� ������/���ᠦ���, ��ࠢ���饬�� ३ᮬ (����� �� �᭮�� १���⮢ ॣ����樨)
    sql << "transfer.point_id_trfer AS point_id, transfer.airp_arv "
           "FROM pax_grp "
           "  LEFT OUTER JOIN bag2 ON pax_grp.grp_id = bag2.grp_id "
           "  LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
           "  JOIN transfer ON pax_grp.grp_id = transfer.grp_id "
           "WHERE "
           "  (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL) "
           "  AND pax_grp.point_dep = :point_id "
           "  AND transfer.transfer_num = 1 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };
  if (type==tckinInbound)
  {
    //��몮���� ३��, ����묨 �ਡ뢠�� ���ᠦ��� ३� (����� �� �᭮�� ᪢����� ॣ����樨)
    sql << "tckin_pax_grp.tckin_id, tckin_pax_grp.grp_num, "
           "pax_grp.point_dep AS point_id, pax_grp.airp_arv "
           "FROM pax_grp "
           "  LEFT OUTER JOIN bag2 ON pax_grp.grp_id = bag2.grp_id "
           "  LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
           "  JOIN tckin_pax_grp ON pax_grp.grp_id = tckin_pax_grp.grp_id "
           "WHERE "
           "  tckin_pax_grp.transit_num = 0 "
           "  AND (pax.bag_pool_num IS NULL OR bag2.bag_pool_num IS NULL) "
           "  AND pax_grp.point_dep = :point_id ";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  sql << "AND pax_grp.bag_refuse=0 "
         "AND pax_grp.status NOT IN ('T', 'E') "
         "GROUP BY pax_grp.point_dep, "
         "         pax_grp.grp_id, "
         "         CASE WHEN pax.grp_id IS NULL THEN bag2.bag_pool_num ELSE pax.bag_pool_num END, "
         "         COALESCE(pax.cabin_class, pax_grp.class), ";
  if (type==trferOut || type==trferOutForCkin)
  {
    sql << "         pax_grp.airp_arv, transfer.airp_arv ";
  };
  if (type==trferIn || type==trferCkin)
  {
    sql << "         transfer.point_id_trfer, transfer.airp_arv ";
  };
  if (type==tckinInbound)
  {
    sql << "         tckin_pax_grp.tckin_id, tckin_pax_grp.grp_num, "
        << "         pax_grp.airp_arv ";
  };
  Qry.SQLText=sql.str().c_str();
  LogTrace(TRACE3) << "type=" << type;
  ProgTrace(TRACE5, "point_id=%d\nQry.SQLText=\n%s", point_id, sql.str().c_str());

  DB::TQuery PaxQry(PgOra::getROSession({"PAX","PAX_GRP","TRANSFER_SUBCLS"}), STDLOG);
  sql.str("");
  sql << "SELECT pax.pax_id, pax.surname, pax.name, pax.seats, ";
  if (type==trferOut || type==trferOutForCkin || type==tckinInbound)
  {
    sql << "pax.subclass "
           "FROM pax_grp "
           "JOIN pax ON pax.grp_id = pax_grp.grp_id ";
  };
  if (type==trferIn || type==trferCkin)
  {
    sql << "transfer_subcls.subclass "
           "FROM pax_grp "
           "JOIN pax ON pax.grp_id = pax_grp.grp_id "
           "JOIN transfer_subcls ON transfer_subcls.transfer_num = 1 ";
  };

  sql << "WHERE "
         "  pax.grp_id = :grp_id "
         "  AND COALESCE(pax.cabin_class, pax_grp.class) = :cabin_class "
         "  AND (pax.bag_pool_num = :bag_pool_num OR pax.bag_pool_num IS NULL AND :bag_pool_num IS NULL) ";
  if (type==trferIn || type==trferOut || type==trferOutForCkin)
    sql << "  AND pax.pr_brd = 1 ";
  if (type==trferCkin || type==tckinInbound)
    sql << "  AND pax.pr_brd IS NOT NULL ";
  PaxQry.SQLText=sql.str().c_str();
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
      grp.fromDB(Qry, false, pr_bag);
      grp.setPaxUnaccomp();
      if (type==trferOutForCkin)
        grp.trferFromDB(false);
      grps_ckin.push_back(grp);
    }
    else
    {
      for(int pass=0; pass<2; pass++)
      {
        if (pass==1) grp.bag_pool_num=NoExists;

        if (grp.bag_pool_num==NoExists)
        {
          if (!grpKeys.emplace(grp.grp_id, cabinClass).second) continue;  //㦥 ��ࠡ��뢠�� ��� ������� grp_id+cabin_class bag_pool_num==NoExists
        };

        if (grp.bag_pool_num!=NoExists)
          grp.fromDB(Qry, false, pr_bag);
        else
          grp.setZero();

        //���ᠦ��� ��㯯�
        PaxQry.SetVariable("grp_id", grp.grp_id);
        PaxQry.SetVariable("cabin_class", cabinClass);
        if (grp.bag_pool_num!=NoExists)
          PaxQry.SetVariable("bag_pool_num", grp.bag_pool_num);
        else
          PaxQry.SetVariable("bag_pool_num", FNull);
        PaxQry.Execute();
        if (PaxQry.Eof) continue; //����� ��㯯� - �� ����頥� � grps
        grp.paxFromDB(PaxQry, false);
        if (type==trferOutForCkin)
          grp.trferFromDB(false);
        grps_ckin.push_back(grp);
      };
    };
  };

  //ProgTrace(TRACE5, "grps_ckin.size()=%zu", grps_ckin.size());

  if (!(type==trferOut || type==trferOutForCkin || type==trferIn)) return;

  grps_tlg = TlgTransferGrpFromDB(type, point_id, pr_bag, flts_from_ckin, priorAirp);
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
        //�஢�ਬ �ॢ���
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


  for(int from_tlg=0; from_tlg<2; from_tlg++)
  {
    std::string table_name;
    std::string sql;
    if (!from_tlg)
    {
      if (type==trferOut || type==trferOutForCkin || type==tckinInbound)
      {
        table_name = "POINTS";
        sql =
          "SELECT airline, flt_no, suffix, scd_out, airp "
          "FROM points "
          "WHERE point_id=:point_id";
      };
      if (type==trferIn || type==trferCkin)
      {
        table_name = "TRFER_TRIPS";
        sql =
          "SELECT airline, flt_no, suffix, scd AS scd_out, airp_dep AS airp "
          "FROM trfer_trips "
          "WHERE point_id=:point_id";
      };
    }
    else
    {
      table_name = "TLG_TRIPS";
      sql =
        "SELECT airline, flt_no, suffix, scd AS scd_out, airp_dep AS airp "
        "FROM tlg_trips "
        "WHERE point_id=:point_id";
    };

    map<int, TFltInfo> point_ids;
    DB::TQuery FltQry(PgOra::getROSession(table_name), STDLOG);
    FltQry.SQLText=sql;
    FltQry.DeclareVariable("point_id", otInteger);

    vector<TGrpItem>::const_iterator g=from_tlg?grps_tlg.begin():grps_ckin.begin();
    for(; g!=(from_tlg?grps_tlg.end():grps_ckin.end()); ++g)
    {
      TGrpViewItem grp;
      if (g->point_id==NoExists) continue;
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && pr_bag && g->tags.empty()) continue; //BTM �� ᮤ�ঠ� ���ᠦ�஢ ��� ��ப
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && !pr_bag && g->is_unaccomp) continue; //PTM �� ᮤ�ঠ� ��ᮯ஢������� �����

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
        grp.subcl_view=ElemIdToCodeNative(etSubcls,g->subcl); //���⮩ ��� ��ᮯ஢��������� ������
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
          //��।���� ��� ��㯯� �ॢ��� atUnattachedTrfer, �᫨ ���ਢ易��� ��ન
          InboundTrfer::TGrpId id(iGrp->grp_id, iGrp->bag_pool_num);

          InboundTrfer::TUnattachedTagMap::const_iterator g=unattached_grps.find(id);
          if (g!=unattached_grps.end())
          {
            if (!g->second.empty() && g->second.size()==iGrp->tags.size())
            {
              //�� ��ન ��㯯� ���ਢ易��
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

  //�ନ�㥬 XML
  TTripInfo flt_tmp=flt;
  flt_tmp.pr_del=0; //�⮡� �� �뢮������ "(��.)"
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

bool existsTlgTransfer(int prior_point_arv, TrferPointType point_type)
{
  const std::set<PointIdTlg_t> point_ids_tlg =
      getPointIdTlgByPointIdsSpp(PointId_t(prior_point_arv));
  for (const PointIdTlg_t& point_id_tlg: point_ids_tlg) {
    if (!loadTlgTransfer(PointIdTlg_t(point_id_tlg), point_type).empty()) {
      return true;
    }
  }
  return false;
}

bool trferInExists(int point_arv, int prior_point_arv)
{
  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP", "PAX", "TRANSFER"}), STDLOG);
  Qry.SQLText =
      "SELECT 1 "
      "FROM pax_grp "
      "  LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
      "  JOIN transfer ON pax_grp.grp_id = transfer.grp_id "
      "WHERE pax_grp.point_arv = :point_arv "
      "AND transfer.transfer_num = 1 "
      "AND pax_grp.bag_refuse = 0 "
      "AND pax_grp.status NOT IN ('T', 'E') "
      "AND (pax_grp.class IS NULL OR pax.pr_brd = 1) "
      "FETCH FIRST 1 ROWS ONLY ";
  Qry.CreateVariable("point_arv", otInteger, point_arv);
  Qry.Execute();
  return !Qry.Eof || existsTlgTransfer(prior_point_arv, TrferPointType::IN);
}

bool trferOutExists(int point_dep)
{
  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP", "PAX", "TRANSFER", "TRFER_TRIPS"}), STDLOG);
  Qry.SQLText =
      "SELECT 1 "
      "FROM trfer_trips "
      "  JOIN transfer ON trfer_trips.point_id = transfer.point_id_trfer "
      "  JOIN (pax_grp LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id) "
      "    ON transfer.grp_id = pax_grp.grp_id "
      "WHERE trfer_trips.point_id_spp = :point_dep "
      "AND transfer.transfer_num = 1 "
      "AND pax_grp.bag_refuse = 0 "
      "AND pax_grp.status NOT IN ('T', 'E') "
      "AND (pax_grp.class IS NULL OR pax.pr_brd = 1) "
      "FETCH FIRST 1 ROWS ONLY ";
  Qry.CreateVariable("point_dep", otInteger, point_dep);
  Qry.Execute();
  return !Qry.Eof || existsTlgTransfer(point_dep, TrferPointType::OUT);
}

bool trferCkinExists(int point_dep)
{
  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP", "PAX", "TRANSFER"}), STDLOG);
  Qry.SQLText =
      "SELECT 1 "
      "FROM pax_grp "
      "  LEFT OUTER JOIN pax ON pax_grp.grp_id = pax.grp_id "
      "  JOIN transfer ON pax_grp.grp_id = transfer.grp_id "
      "WHERE pax_grp.point_dep = :point_dep "
      "AND transfer.transfer_num = 1 "
      "AND pax_grp.bag_refuse = 0 "
      "AND pax_grp.status NOT IN ('T', 'E') "
      "AND (pax_grp.class IS NULL OR pax.pr_brd IS NOT NULL) "
      "FETCH FIRST 1 ROWS ONLY ";
  Qry.CreateVariable("point_dep", otInteger, point_dep);
  Qry.Execute();
  return !Qry.Eof;
}

std::set<TrferId_t> loadTrferIdSet(const PointIdTlg_t& point_id)
{
  dbo::Session session;
  const std::vector<int> trfer_ids =
      session.query<int>("SELECT trfer_id")
      .from("tlg_transfer")
      .where("point_id_out = :point_id")
      .setBind({{":point_id", point_id.get()}});
  return algo::transform<std::set>(trfer_ids, [](int id) { return TrferId_t(id); });
}

std::set<TlgId_t> loadTrferTlgIdSet(const PointIdTlg_t& point_id)
{
  dbo::Session session;
  const std::vector<int> tlg_ids =
      session.query<int>("SELECT tlg_id")
      .from("tlg_transfer")
      .where("point_id_out = :point_id")
      .setBind({{":point_id", point_id.get()}});
  return algo::transform<std::set>(tlg_ids, [](int id) { return TlgId_t(id); });
}

std::set<TrferGrpId_t> loadTrferGrpIdSet(const TrferId_t& trfer_id)
{
  dbo::Session session;
  const std::vector<int> grp_ids =
      session.query<int>("SELECT grp_id").from("trfer_grp")
      .where("trfer_id = :trfer_id")
      .setBind({{":trfer_id", trfer_id.get()}});
  return algo::transform<std::set>(grp_ids, [](int id) { return TrferGrpId_t(id); });
}

std::set<TrferGrpId_t> loadTrferGrpIdSet(const PointIdTlg_t& point_id)
{
  dbo::Session session;
  const std::vector<int> grp_ids =
      session.query<int>("SELECT grp_id").from("trfer_grp, tlg_transfer")
      .where("tlg_transfer.trfer_id = trfer_grp.trfer_id AND "
             "tlg_transfer.point_id_out = :point_id")
      .setBind({{":point_id", point_id.get()}});
  return algo::transform<std::set>(grp_ids, [](int id) { return TrferGrpId_t(id); });
}

std::set<TrferId_t> loadTrferIdsByTlgTransferIn(const PointIdTlg_t& point_id_in,
                                                const std::string& tlg_type)
{
  std::set<TrferId_t> result;
  const std::vector<TlgTransfer> tlg_transfers = loadTlgTransfer(PointIdTlg_t(point_id_in),
                                                                 TrferPointType::IN,
                                                                 tlg_type);
  for (const TlgTransfer& tlg_transfer: tlg_transfers) {
    result.insert(tlg_transfer.trfer_id);
  }
  return result;
}

std::set<PointId_t> loadPointIdsSppByTlgTransferIn(const PointIdTlg_t& point_id_in)
{
  std::set<PointIdTlg_t> point_ids_out;
  const std::vector<TlgTransfer> tlg_transfers = loadTlgTransfer(point_id_in,
                                                                 TrferPointType::IN,
                                                                 "BTM");
  for (const TlgTransfer& tlg_transfer: tlg_transfers) {
    point_ids_out.insert(tlg_transfer.point_id_out);
  }
  std::set<PointId_t> point_ids_spp;
  for (const PointIdTlg_t& point_id_out: point_ids_out) {
    getPointIdsSppByPointIdTlg(point_id_out, point_ids_spp, false /*clear*/);
  }
  return point_ids_spp;
}

bool deleteTrferPax(const TrferGrpId_t& grp_id)
{
  auto cur = make_db_curs(
        "DELETE FROM trfer_pax "
        "WHERE grp_id = :grp_id ",
        PgOra::getRWSession("TRFER_PAX"));
  cur.bind(":grp_id", grp_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTrferTags(const TrferGrpId_t& grp_id)
{
  auto cur = make_db_curs(
        "DELETE FROM trfer_tags "
        "WHERE grp_id = :grp_id ",
        PgOra::getRWSession("TRFER_TAGS"));
  cur.bind(":grp_id", grp_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTlgTrferOnwards(const TrferGrpId_t& grp_id)
{
  auto cur = make_db_curs(
        "DELETE FROM tlg_trfer_onwards "
        "WHERE grp_id = :grp_id ",
        PgOra::getRWSession("TLG_TRFER_ONWARDS"));
  cur.bind(":grp_id", grp_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTlgTrferExcepts(const TrferGrpId_t& grp_id)
{
  auto cur = make_db_curs(
        "DELETE FROM tlg_trfer_excepts "
        "WHERE grp_id = :grp_id ",
        PgOra::getRWSession("TLG_TRFER_EXCEPTS"));
  cur.bind(":grp_id", grp_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTrferGrp(const TrferId_t& trfer_id)
{
  auto cur = make_db_curs(
        "DELETE FROM trfer_grp "
        "WHERE trfer_id = :trfer_id ",
        PgOra::getRWSession("TRFER_GRP"));
  cur.bind(":trfer_id", trfer_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTlgTransfer(const TrferId_t& trfer_id)
{
  auto cur = make_db_curs(
        "DELETE FROM tlg_transfer "
        "WHERE trfer_id = :trfer_id ",
        PgOra::getRWSession("TLG_TRANSFER"));
  cur.bind(":trfer_id", trfer_id.get()).exec();
  return cur.rowcount() > 0;
}

bool deleteTlgTransfer(const PointIdTlg_t& point_id)
{
  auto cur = make_db_curs(
        "DELETE FROM tlg_transfer "
        "WHERE point_id_out= :point_id ",
        PgOra::getRWSession("TLG_TRANSFER"));
  cur.bind(":point_id", point_id.get()).exec();
  return cur.rowcount() > 0;
}

int countTlgTransfer(const PointIdTlg_t& point_id)
{
    int result = 0;
    auto cur = make_db_curs(
          "SELECT COUNT(*) FROM tlg_transfer "
          "WHERE point_id_in=:point_id "
          "AND point_id_in<>point_id_out ",
          PgOra::getROSession("TLG_TRANSFER"));
    cur
        .stb()
        .def(result)
        .bind(":point_id", point_id.get())
        .EXfet();
    return result;
}

bool existsTlgTransfer(const PointIdTlg_t& point_id, const TlgId_t& tlg_id)
{
    auto cur = make_db_curs(
          "SELECT 1 FROM tlg_transfer "
          "WHERE point_id_in=:point_id "
          "AND tlg_id=:tlg_id "
          "FETCH FIRST 1 ROWS ONLY ",
          PgOra::getROSession("TLG_TRANSFER"));
    cur
        .stb()
        .bind(":point_id", point_id.get())
        .bind(":tlg_id", tlg_id.get())
        .EXfet();
    return cur.err() != DbCpp::ResultCode::NoDataFound;
}

bool deleteTlgSource(const PointIdTlg_t& point_id, const TlgId_t& tlg_id)
{
  auto cur = make_db_curs(
        "DELETE FROM tlg_source "
        "WHERE point_id_tlg=:point_id "
        "AND tlg_id=:tlg_id ",
        PgOra::getRWSession("TLG_SOURCE"));
  cur
      .stb()
      .bind(":point_id", point_id.get())
      .bind(":tlg_id", tlg_id.get())
      .exec();
  return cur.rowcount() > 0;
}

void deleteTransferData(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  const std::set<TrferId_t> trfer_ids = loadTrferIdSet(point_id);
  const std::set<TrferGrpId_t> grp_ids = loadTrferGrpIdSet(point_id);
  for (const TrferGrpId_t& grp_id: grp_ids) {
      deleteTrferPax(grp_id);
      deleteTrferTags(grp_id);
      deleteTlgTrferOnwards(grp_id);
      deleteTlgTrferExcepts(grp_id);
  }
  for (const TrferId_t& trfer_id: trfer_ids) {
      deleteTrferGrp(trfer_id);
  }
  deleteTlgTransfer(point_id);

  const int count = countTlgTransfer(point_id);
  if (count == 0) {
    TypeB::deleteTlgSource(point_id);
    TypeB::deleteTlgTrips(point_id);
  } else {
    const std::set<TlgId_t> tlg_ids = loadTrferTlgIdSet(point_id);
    for (const TlgId_t& tlg_id: tlg_ids) {
      if (!existsTlgTransfer(point_id, tlg_id)) {
        deleteTlgSource(point_id, tlg_id);
      }
    }
  }
}

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
    //���筠� ��㯯� ���ᠦ�஢
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
          if (p1->equalRate(*p2)>=6) //6 - ������ ᮢ������� 䠬���� � �����
            return true;
    }
    else return true;
  };
  return false;
};

void TGrpItem::normalizeTrfer()
{
  //�᫨ ���� ࠧ�� �࠭��୮�� ������� - 㡥६ ���� ��᫥ ࠧ�뢠
  //�᫨ ���� ����।������ ���� - 㡥६ ���� � ����
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
      for(int minPaxEqualRate=7; minPaxEqualRate>=6; minPaxEqualRate--)   //6 - ������ ᮢ������� 䠬���� � �����
      {
        //ProgTrace(TRACE5, "FilterUnattachedTags: minPaxEqualRate=%d", minPaxEqualRate);
        while (!t->second.second.empty())
        {
          pair< list<TGrpId>::iterator, list<TGrpId>::iterator > max(t->second.first.end(), t->second.second.end());
          int maxEqualPaxCount=0;
          for(list<TGrpId>::iterator i1=t->second.first.begin(); i1!=t->second.first.end(); ++i1)
          {
            if (pass==1 && i1->second==NoExists) continue; //��㯯� �� BTM
            for(list<TGrpId>::iterator i2=t->second.second.begin(); i2!=t->second.second.end(); ++i2)
            {
              map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(*i1);
              if (grp_in==grps_in.end()) throw Exception("FilterUnattachedTags: grp_in=grps_in.end()");
              TGrpItem joint_grp_in(grp_in->second);
              if (pass==1)
              {
                //ᮡ�ࠥ� ��ꥤ������ joint_grp_in.paxs
                for(map<TGrpId, TGrpItem>::const_iterator i3=grps_in.begin(); i3!=grps_in.end(); ++i3)
                {
                  if (i3==grp_in) continue;
                  if (i3->first.first==grp_in->first.first) //ᮢ������ grp_id
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

void GetCheckedTags(int id,  //�.�. point_id ��� grp_id
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

void GetCheckedTags(int id,  //�.�. point_id ��� grp_id
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
  DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","PAX"}),STDLOG);
  Qry.ClearParams();
  Qry.SQLText=
    "SELECT pax_id, surname, name "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND airp_arv=:airp_arv AND "
    "      pax_grp.status NOT IN ('E') AND COALESCE(inbound_confirm,0)=0"; //NVL ��⮬ ����!!!
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("airp_arv", otString, grp_out.airp_arv);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (pax_out_ids.find(Qry.FieldAsInteger("pax_id"))!=pax_out_ids.end()) continue;
    paxs_ckin.push_back(TPaxItem("",Qry.FieldAsString("surname"),Qry.FieldAsString("name")));
  };

  DB::TQuery Qry2(PgOra::getROSession({"TLG_BINDING","CRS_PNR","CRS_PAX","PAX"}),STDLOG);
  Qry2.ClearParams();
  Qry2.SQLText=
    "SELECT crs_pax.pax_id, crs_pax.surname, crs_pax.name "
    "FROM tlg_binding, crs_pnr, crs_pax, pax "
    "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL AND "
    "      tlg_binding.point_id_spp=:point_id AND "
    "      crs_pnr.system='CRS' AND "
    "      crs_pax.pr_del=0 ";
  Qry2.CreateVariable("point_id", otInteger, point_id);
  Qry2.Execute();
  for(;!Qry2.Eof;Qry2.Next())
  {
    if (pax_out_ids.find(Qry2.FieldAsInteger("pax_id"))!=pax_out_ids.end()) continue;
    paxs_crs.push_back(TPaxItem("",Qry2.FieldAsString("surname"),Qry2.FieldAsString("name")));
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

  if (grp_out.is_unaccomp) return; //��� ��ᮯ஢��������� ������ ���� ��楤�� �� ��ᯮᮡ����

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
          if (paxIn->equalRate(*paxOut)>=6) //6 - ������ ᮢ������� 䠬���� � �����
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
              //������� � tag_map TGrpItem
              iTagMap=info.tag_map.insert( make_pair(grp_in_id, make_pair(*g, set<ConflictReason>()) ) ).first;
              if (iTagMap==info.tag_map.end())
                throw Exception("%s: iTagMap==info.tag_map.end()", __FUNCTION__);
              if (!grp_in.similarTrfer(grp_out, false) ||
                  grp_in.trfer.size()>grp_out.trfer.size())
              {
                //��ᮢ������� ���쭥��� �࠭����� ������⮢
                iTagMap->second.second.insert(ConflictReason::InOutRouteDiffer);
                info.conflicts.insert(ConflictReason::InOutRouteDiffer);
              };
              int bag_weight=CalcWeightInKilos(g->bag_weight, g->weight_unit);
              if (bag_weight==NoExists || bag_weight<=0)
              {
                //�� ����� ��� ������ ��� ��� ��ப
                iTagMap->second.second.insert(ConflictReason::WeightNotDefined);
                info.conflicts.insert(ConflictReason::WeightNotDefined);
              };
              if (!grp_in.equalTrfer(first_grp_in))
              {
                //��ᮢ������� ���쭥��� �࠭����� ������⮢ 2-� ����
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
                  if (p1->equalRate(*p2)>=6) //6 - ������ ᮢ������� 䠬���� � �����
                  {
                    //���� ���ᠦ��� � ⠪�� 䠬����� � ������ �।� 㦥 ��ॣ����஢�����
                    iTagMap->second.second.insert(ConflictReason::OutPaxDuplicate);
                    info.conflicts.insert(ConflictReason::OutPaxDuplicate);
                    break;
                  };
                if (p2!=paxs_ckin.end()) break;

                p2=paxs_crs.begin();
                for(; p2!=paxs_crs.end(); ++p2)
                {
                  if (p1->equalRate(*p2)>=6) //6 - ������ ᮢ������� 䠬���� � �����
                  {
                    //���� ���ᠦ��� � ⠪�� 䠬����� � ������ �।� ���஭�஢�����
                    iTagMap->second.second.insert(ConflictReason::OutPaxDuplicate);
                    info.conflicts.insert(ConflictReason::OutPaxDuplicate);
                    break;
                  };
                };
                if (p2!=paxs_crs.end()) break;
              };
            };

            //������� � pax_map
            pair<string, string> pax_out_id(paxOut->surname, paxOut->name);
            TNewGrpPaxMap::iterator iPaxMap=info.pax_map.find(pax_out_id);
            if (iPaxMap==info.pax_map.end())
              iPaxMap=info.pax_map.insert( make_pair(pax_out_id, set<TGrpId>()) ).first;
            if (iPaxMap==info.pax_map.end())
              throw Exception("%s: iPaxMap==pax_map.end()", __FUNCTION__);
            iPaxMap->second.insert(grp_in_id);
            if (iPaxMap->second.size()>1)
            {
              //�㡫�஢���� ���ᠦ�஢ � ᯨ᪥ �室�饣� �࠭���
              if (iPaxMap->second.size()==2)
              {
                //�㦭� ������� �ॢ��� � �����
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
                //�㦭� ������� �ॢ��� � ⥪�饣�
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
    if (grpItem.bag_pool_num!=NoExists) //�������� ⮣�, ��㤠 �����: �� ⥫��ࠬ� ��� ��ॣ����஢���� � ����
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

void GetNextTrferCheckedFlts(const int id, const TIdType id_type, std::set<int>& point_ids)
{
    point_ids.clear();

    DB::TQuery Qry(PgOra::getROSession({"TRANSFER", "TRFER_TRIPS", "TRIP_STAGES", "PAX_GRP"}), STDLOG);
    Qry.SQLText = TIdType::idFlt == id_type
     ? "SELECT trfer_trips.point_id_spp "
         "FROM transfer "
        "INNER JOIN trfer_trips "
           "ON trfer_trips.point_id = transfer.point_id_trfer "
        "INNER JOIN trip_stages "
           "ON trfer_trips.point_id_spp = trip_stages.point_id "
        "INNER JOIN pax_grp "
           "ON transfer.grp_id = pax_grp.grp_id "
        "WHERE pax_grp.point_dep = :id "
          "AND transfer.transfer_num = 1 "
          "AND trip_stages.stage_id = :stage_id "
          "AND trip_stages.act IS NOT NULL"
     : "SELECT trfer_trips.point_id_spp "
         "FROM transfer "
        "INNER JOIN trfer_trips "
           "ON trfer_trips.point_id = transfer.point_id_trfer "
        "INNER JOIN trip_stages "
           "ON trfer_trips.point_id_spp = trip_stages.point_id "
        "WHERE transfer.grp_id = :id "
          "AND transfer.transfer_num = 1 "
          "AND trip_stages.stage_id = :stage_id "
          "AND trip_stages.act IS NOT NULL";

    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("stage_id", otInteger, sCloseCheckIn);

    for (Qry.Execute(); !Qry.Eof; Qry.Next()) {
        point_ids.insert(Qry.FieldAsInteger("point_id_spp"));
    }
/*
  //�� ����� ���஡�� ������, �� �� �ॡ�� ����� �����⥫쭮�� �⭮襭�� � ���⠬ �맮�� ��楤���
  //���⮬� ���� ������ ��� � �ᯮ��㥬 ����� ������ SQL �����
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
}

}; //namespace InboundTrfer


