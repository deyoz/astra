#include "transfer.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "exceptions.h"
#include "term_version.h"
#include "alarms.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;
using namespace EXCEPTIONS;

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
      //вычисление базового класса grp.subcl на основе подклассов пассажиров
      for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
      {
        try
        {
          //вычислкние базового класса grp.subcl
          string cl=((TSubclsRow&)base_tables.get( "subcls" ).get_row( "code", p->subcl, true )).cl;
          if (subcl.empty())
            subcl=cl;
          else
            if (cl!=subcl)
            {
              subcl.clear();
              break;
            };
        }
        catch(EBaseTableError)
        {
          subcl.clear();
          break;
        };
      };
    };
  }
  else
  {
    //сюда попадаем если цифровая PTM, например
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
    //t.subclass=TrferQry.FieldAsString("subclass"); это не загружаем, так как не участвует в сравнении
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
                 int point_id,   //point_id содержит пункт прилета для trferIn
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

  TQuery FltQry(&OraSession);
  FltQry.Clear();
  FltQry.SQLText=
    "SELECT airline, flt_no, suffix, scd_out, airp, pr_del, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    "FROM points "
    "WHERE point_id=:point_id";
  FltQry.DeclareVariable("point_id", otInteger);
  TTripRouteItem priorAirp;
  if (type==trferIn)
  {
    //point_id содержит пункт прилета, а нам нужен предыдущий пункт вылета
    TTripRoute().GetPriorAirp(NoExists,point_id,trtNotCancelled,priorAirp);
    if (priorAirp.point_id==NoExists) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
    FltQry.SetVariable("point_id", priorAirp.point_id);
  }
  else
    FltQry.SetVariable("point_id", point_id);
  FltQry.Execute();
  if (FltQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
  flt.Init(FltQry);
  if (flt.pr_del==NoExists) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");

  Qry.Clear();
  ostringstream sql;
  sql << "SELECT pax_grp.point_dep, pax_grp.grp_id, bag2.bag_pool_num, pax_grp.class, \n"
         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,bag2.amount,0)) AS bag_amount, \n"
         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,bag2.weight,0)) AS bag_weight, \n"
         "       SUM(DECODE(bag2.pr_cabin,NULL,0,0,0,bag2.weight)) AS rk_weight, \n";

  if (type==trferOut || type==trferOutForCkin)
  {
    //Информация о трансферном багаже/пассажирах, отправляющемся рейсом
    sql << "     pax_grp.point_dep AS point_id, pax_grp.airp_arv, \n"
           "     transfer.airp_arv AS last_airp_arv \n"
           "FROM trfer_trips, transfer, pax_grp, bag2 \n"
           "WHERE trfer_trips.point_id=transfer.point_id_trfer AND \n"
           "      transfer.grp_id=pax_grp.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      trfer_trips.point_id_spp=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferIn)
  {
    //Информация о трансферном багаже/пассажирах, прибывающем рейсом
    sql << "      transfer.point_id_trfer AS point_id, transfer.airp_arv \n"
           "FROM pax_grp, transfer, bag2 \n"
           "WHERE pax_grp.grp_id=transfer.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.point_arv=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  if (type==trferCkin)
  {
    //Информация о трансферном багаже/пассажирах, отправляющемся рейсом (данные на основе результатов регистрации)
    sql << "      transfer.point_id_trfer AS point_id, transfer.airp_arv \n"
           "FROM pax_grp, transfer, bag2 \n"
           "WHERE pax_grp.grp_id=transfer.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.point_dep=:point_id AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };
  if (type==tckinInbound)
  {
    //Стыковочные рейсы, которыми прибывают пассажиры рейса (данные на основе сквозной регистрации)
    sql << "      tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no, \n"
           "      pax_grp.point_dep AS point_id, pax_grp.airp_arv \n"
           "FROM pax_grp, tckin_pax_grp, bag2 \n"
           "WHERE pax_grp.grp_id=tckin_pax_grp.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      pax_grp.point_dep=:point_id AND \n";
    Qry.CreateVariable("point_id", otInteger, point_id);
  };

  sql << "      pax_grp.bag_refuse=0 AND pax_grp.status NOT IN ('T', 'E') \n"
         "GROUP BY pax_grp.point_dep, pax_grp.grp_id, bag2.bag_pool_num, pax_grp.class, \n";
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
    sql << "         tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no, \n"
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
           "FROM pax \n"
           "WHERE \n";
  };
  if (type==trferIn || type==trferCkin)
  {
    sql << "       transfer_subcls.subclass \n"
           "FROM pax, transfer_subcls \n"
           "WHERE pax.pax_id=transfer_subcls.pax_id(+) AND transfer_subcls.transfer_num(+)=1 AND \n";
  };

  sql << "      pax.grp_id=:grp_id AND \n"
         "      (pax.bag_pool_num=:bag_pool_num OR pax.bag_pool_num IS NULL AND :bag_pool_num IS NULL) AND \n";
  if (type==trferIn || type==trferOut || type==trferOutForCkin)
    sql << "      pax.pr_brd=1 \n";
  if (type==trferCkin || type==tckinInbound)
    sql << "      pax.pr_brd IS NOT NULL \n";
  PaxQry.SQLText=sql.str().c_str();
  //ProgTrace(TRACE5, "PaxQry.SQLText=\n%s", sql.str().c_str());
  PaxQry.DeclareVariable("grp_id", otInteger);
  PaxQry.DeclareVariable("bag_pool_num", otInteger);

  set<int> grp_ids;

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
        FltQry.SetVariable("point_id", point_dep);
        FltQry.Execute();
        if (FltQry.Eof) continue;
        TFltInfo flt1(FltQry, true);
        if (flt1.pr_del==NoExists) continue;
        id=spp_point_ids_from_ckin.insert( make_pair(point_dep, flt1) ).first;
        flts_from_ckin.insert(flt1);
      };
      if (id==spp_point_ids_from_ckin.end()) continue;
    };
    TGrpItem grp;
    if (type==tckinInbound)
    {
      TCkinRouteItem inboundSeg;
      TCkinRoute().GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                               Qry.FieldAsInteger("seg_no"),
                               crtIgnoreDependent, inboundSeg);
      if (inboundSeg.grp_id==NoExists) continue;

      TTripRouteItem priorAirp;
      TTripRoute().GetPriorAirp(NoExists,inboundSeg.point_arv,trtNotCancelled,priorAirp);
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
    if (Qry.FieldIsNULL("class"))
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
          if (!grp_ids.insert(grp.grp_id).second) continue;  //уже обрабатывали для данного grp_id bag_pool_num==NoExists
        };

        if (grp.bag_pool_num!=NoExists)
          grp.fromDB(Qry, TagQry, false, pr_bag);
        else
          grp.setZero();

        //пассажиры группы
        PaxQry.SetVariable("grp_id", grp.grp_id);
        if (grp.bag_pool_num!=NoExists)
          PaxQry.SetVariable("bag_pool_num", grp.bag_pool_num);
        else
          PaxQry.SetVariable("bag_pool_num", FNull);
        PaxQry.Execute();
        if (PaxQry.Eof) continue; //пустая группа - не помещаем в grps
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
    //Информация о трансферном багаже/пассажирах, отправляющемся рейсом
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
    //Информация о трансферном багаже/пассажирах, прибывающем рейсом
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
      //данный телеграммный рейс обслуживается в Астре
      continue;
    };

    id=flts_from_tlg.find(flt1);
    if (id==flts_from_tlg.end())
    {
      //нету информации про flt1 - требуется проверка обслуживания в Астре
      TSearchFltInfo filter;
      filter.airline=flt1.airline;
      filter.flt_no=flt1.flt_no;
      filter.suffix=flt1.suffix;
      filter.airp_dep=flt1.airp;
      filter.scd_out=flt1.scd_out;
      filter.scd_out_in_utc=false;
      filter.only_with_reg=false;
      filter.additional_where=
        " AND EXISTS(SELECT pax_grp.point_dep "
        "            FROM pax_grp "
        "            WHERE pax_grp.point_dep=points.point_id AND pax_grp.status NOT IN ('E'))";

      //ищем рейс в СПП
      list<TAdvTripInfo> flts;
      SearchFlt(filter, flts);
      list<TAdvTripInfo>::const_iterator f=flts.begin();
      for(; f!=flts.end(); ++f)
      {
        TFltInfo flt2(*f, true);
        if (!(flt1<flt2) && !(flt2<flt1)) break;
      };

      if (f==flts.end())
        //не нашли рейс - значит не обслуживается в Астре
        id=flts_from_tlg.insert(flt1).first;
      else
        //данный телеграммный рейс обслуживается в Астре
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
    //пассажиры группы
    PaxQry.SetVariable("grp_id", grp.grp_id);
    PaxQry.Execute();
    //даже если пустая группа - помещаем в paxs пустой surname
    grp.paxFromDB(PaxQry, RemQry, true);
    if (pr_bag)
    {
      //определяем несопровождаемый багаж - он может быть только в BTM
      ExceptQry.SetVariable("grp_id", grp.grp_id);
      ExceptQry.Execute();
      if (!ExceptQry.Eof)
        grp.is_unaccomp=true;
      else
      {
        //проверим UNACCOMPANIED
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

const char *AlarmTypeS[] = {
    "TRFER_UNATTACHED",
    "TRFER_IN_PAX_DUPLICATE",
    "TRFER_OUT_PAX_DUPLICATE",
    "TRFER_IN_ROUTE_INCOMPLETE",
    "TRFER_IN_ROUTE_DIFFER",
    "TRFER_OUT_ROUTE_DIFFER",
    "TRFER_IN_OUT_ROUTE_DIFFER",
    "TRFER_WEIGHT_NOT_DEFINED"
};

string EncodeAlarmType(const TAlarmType alarm )
{
    if(alarm < 0 or alarm >= atLength)
        throw Exception("InboundTrfer::EncodeAlarmType: wrong alarm type %d", alarm);
    return AlarmTypeS[ alarm ];
};


void TrferToXML(TTrferType type,
                bool bag_pool_compatible,
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
          !bag_pool_compatible ||
          iGrpPrior==grps.end() || iGrpPrior->grp_id!=iGrp->grp_id)
      {
        grpNode=NewTextChild(grpsNode,"grp");
        newPaxsNode=true;
      };
    };

    if (type==trferIn || type==trferOut || type==trferOutForCkin ||
        !bag_pool_compatible)
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
        //проверим тревоги
        xmlNodePtr node=NewTextChild(grpNode,"tag_ranges");
        for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
        {
          xmlNodePtr rangeNode=NewTextChild(node,"range",*r);
          if (!iGrp->alarms.empty())
          {
            if (iGrp->alarms.size()==1)
              NewTextChild(rangeNode, "alarm", EncodeAlarmType(*(iGrp->alarms.begin())));
            else
            {
              xmlNodePtr alarmsNode=NewTextChild(rangeNode, "alarms");
              for(set<TAlarmType>::const_iterator a=iGrp->alarms.begin(); a!=iGrp->alarms.end(); ++a)
                NewTextChild(alarmsNode, "alarm", EncodeAlarmType(*a));
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
        if (!(type==trferIn || type==trferOut || type==trferOutForCkin ||
              !bag_pool_compatible))

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
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && pr_bag && g->tags.empty()) continue; //BTM не содержат пассажиров без бирок
      if ((type==trferIn || type==trferOut || type==trferOutForCkin) && !pr_bag && g->is_unaccomp) continue; //PTM не содержат несопровождаемый багаж

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
        grp.subcl_view=ElemIdToCodeNative(etSubcls,g->subcl); //пустой для несопровождаемого багажа
        grp.subcl_priority=0;
        try
        {
          TSubclsRow &subclsRow=(TSubclsRow&)base_tables.get("subcls").get_row("code",g->subcl);
          grp.subcl_priority=((TClassesRow&)base_tables.get("classes").get_row("code",subclsRow.cl)).priority;
        }
        catch(EBaseTableError){};
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
      set<TAlarmType> unattached_alarm;
      unattached_alarm.insert(atUnattached);
      for(int from_tlg=0; from_tlg<2; from_tlg++)
      {
        vector<TGrpItem>::const_iterator iGrp=from_tlg?grps_tlg.begin():grps_ckin.begin();
        for(; iGrp!=(from_tlg?grps_tlg.end():grps_ckin.end()); ++iGrp)
        {
          //передадим для группы тревогу atUnattachedTrfer, если непривязанные бирки
          InboundTrfer::TGrpId id(iGrp->grp_id, iGrp->bag_pool_num);

          InboundTrfer::TUnattachedTagMap::const_iterator g=unattached_grps.find(id);
          if (g!=unattached_grps.end())
          {
            if (!g->second.empty() && g->second.size()==iGrp->tags.size())
            {
              //все бирки группы непривязаны
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

  //формируем XML
  TTripInfo flt_tmp=flt;
  flt_tmp.pr_del=0; //чтобы не выводилось "(отм.)"
  NewTextChild(resNode,"trip",GetTripName(flt_tmp,ecNone));

  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");

  TrferToXML(type,
             TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS),
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
    //обычная группа пассажиров
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
};

bool TGrpItem::similarTrfer(const TGrpItem &item) const
{
  map<int, CheckIn::TTransferItem>::const_iterator s1=trfer.begin();
  map<int, CheckIn::TTransferItem>::const_iterator s2=item.trfer.begin();
  for(;s1!=trfer.end()&&s2!=item.trfer.end();++s1,++s2)
    if (s1->first!=s2->first ||
        !s1->second.equalSeg(s2->second)) break;
  return (s1==trfer.end() || s2==item.trfer.end());
};

void TGrpItem::print() const
{
  ProgTrace(TRACE5, "airp_arv=%s is_unaccomp=%s", airp_arv.c_str(), is_unaccomp?"true":"false");
  ProgTrace(TRACE5, "paxs:");
  for(vector<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    ProgTrace(TRACE5, "  surname=%s name=%s subcl=%s", p->surname.c_str(), p->name.c_str(), p->subcl.c_str());
  ProgTrace(TRACE5, "tags:");
  for(multiset<TBagTagNumber>::const_iterator t=tags.begin(); t!=tags.end(); ++t)
    ProgTrace(TRACE5, "  %15.0f", t->numeric_part);
  ProgTrace(TRACE5, "trfer:");
  for(map<int, CheckIn::TTransferItem>::const_iterator t=trfer.begin(); t!=trfer.end(); ++t)
    ProgTrace(TRACE5, "  %d: %s%d%s/%s %s-%s", t->first,
                      t->second.operFlt.airline.c_str(), t->second.operFlt.flt_no, t->second.operFlt.suffix.c_str(),
                      DateTimeToStr(t->second.operFlt.scd_out, "ddmmm").c_str(), t->second.operFlt.airp.c_str(), t->second.airp_arv.c_str());
};

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
    "      bag_tags.no=:no AND bag2.is_trfer<>0 AND pax_grp.point_dep=:point_id";

  const char *tags_sql=
    "SELECT no FROM bag_tags WHERE grp_id=:grp_id AND bag_num=:bag_num";
  const char *paxs_sql=
    "SELECT subclass, surname, name "
    "FROM pax "
    "WHERE grp_id=:grp_id AND bag_pool_num=:bag_pool_num AND seats>0";

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
          if (p1->equalRate(*p2)>=6) //6 - полное совпадение фамилии и имени
            return true;
    }
    else return true;
  };
  return false;
};

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
      for(int minPaxEqualRate=7; minPaxEqualRate>=6; minPaxEqualRate--)   //6 - полное совпадение фамилии и имени
      {
        //ProgTrace(TRACE5, "FilterUnattachedTags: minPaxEqualRate=%d", minPaxEqualRate);
        while (!t->second.second.empty())
        {
          pair< list<TGrpId>::iterator, list<TGrpId>::iterator > max(t->second.first.end(), t->second.second.end());
          int maxEqualPaxCount=0;
          for(list<TGrpId>::iterator i1=t->second.first.begin(); i1!=t->second.first.end(); ++i1)
          {
            if (pass==1 && i1->second==NoExists) continue; //группа из BTM
            for(list<TGrpId>::iterator i2=t->second.second.begin(); i2!=t->second.second.end(); ++i2)
            {
              map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(*i1);
              if (grp_in==grps_in.end()) throw Exception("FilterUnattachedTags: grp_in=grps_in.end()");
              TGrpItem joint_grp_in(grp_in->second);
              if (pass==1)
              {
                //собираем объединенный joint_grp_in.paxs
                for(map<TGrpId, TGrpItem>::const_iterator i3=grps_in.begin(); i3!=grps_in.end(); ++i3)
                {
                  if (i3==grp_in) continue;
                  if (i3->first.first==grp_in->first.first) //совпадают grp_id
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

void GetCheckedTags(int id,  //м.б. point_id или grp_id
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
    "WHERE grp_id=:grp_id AND bag_pool_num=:bag_pool_num AND seats>0";
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

void GetCheckedTags(int id,  //м.б. point_id или grp_id
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
    "      pax_grp.status NOT IN ('E') AND NVL(inbound_confirm,0)=0";
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

string GetConflictStr(const set<TConflictReason> &conflicts)
{
  ostringstream s;
  for(set<TConflictReason>::const_iterator c=conflicts.begin(); c!=conflicts.end(); ++c)
  {
    switch(*c)
    {
      case conflictInPaxDuplicate:
        s << "InPaxDuplicate; ";
        break;
      case conflictOutPaxDuplicate:
        s << "OutPaxDuplicate; ";
        break;
      case conflictInRouteIncomplete:
        s << "InRouteIncomplete; ";
        break;
      case conflictInRouteDiffer:
        s << "InRouteDiffer; ";
        break;
      case conflictOutRouteDiffer:
        s << "OutRouteDiffer; ";
        break;
      case conflictInOutRouteDiffer:
        s << "InOutRouteDiffer; ";
        break;
      case conflictWeightNotDefined:
        s << "WeightNotDefined; ";
        break;
    };
  };
  return s.str();
};

bool isGlobalConflict(TConflictReason c)
{
   return c==conflictInPaxDuplicate ||
          c==conflictInRouteIncomplete ||
          c==conflictInRouteDiffer ||
          c==conflictOutRouteDiffer;
};

TrferList::TAlarmType GetConflictAlarm(TConflictReason c)
{
  switch(c)
  {
    case conflictInPaxDuplicate:    return TrferList::atInPaxDuplicate;
    case conflictOutPaxDuplicate:   return TrferList::atOutPaxDuplicate;
    case conflictInRouteIncomplete: return TrferList::atInRouteIncomplete;
    case conflictInRouteDiffer:     return TrferList::atInRouteDiffer;
    case conflictOutRouteDiffer:    return TrferList::atOutRouteDiffer;
    case conflictInOutRouteDiffer:  return TrferList::atInOutRouteDiffer;
    case conflictWeightNotDefined:  return TrferList::atWeightNotDefined;
  };
  return TrferList::atLength;
};


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

  for(set<TConflictReason>::const_iterator c=conflicts.begin();
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

  if (grp_out.is_unaccomp) return; //для несопровождаемого багажа пока процедура не приспособлена

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
          if (paxIn->equalRate(*paxOut)>=6) //6 - полное совпадение фамилии и имени
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
                  info.conflicts.insert(conflictInRouteIncomplete);
                  break;
                };
            };
            TNewGrpTagMap::iterator iTagMap=info.tag_map.find(grp_in_id);
            if (iTagMap==info.tag_map.end())
            {
              //добавим в tag_map TGrpItem
              iTagMap=info.tag_map.insert( make_pair(grp_in_id, make_pair(*g, set<TConflictReason>()) ) ).first;
              if (iTagMap==info.tag_map.end())
                throw Exception("GetNewGrpTags: iTagMap==info.tag_map.end()");
              if (!grp_in.similarTrfer(grp_out) ||
                  grp_in.trfer.size()>grp_out.trfer.size())
              {
                //несовпадение дальнейших трансферных маршрутов
                iTagMap->second.second.insert(conflictInOutRouteDiffer);
                info.conflicts.insert(conflictInOutRouteDiffer);
              };
              int bag_weight=CalcWeightInKilos(g->bag_weight, g->weight_unit);
              if (bag_weight==NoExists || bag_weight<=0)
              {
                //не задан вес багажа или нет бирок
                iTagMap->second.second.insert(conflictWeightNotDefined);
                info.conflicts.insert(conflictWeightNotDefined);
              };
              if (!grp_in.equalTrfer(first_grp_in))
              {
                //несовпадение дальнейших трансферных маршрутов 2-х НДТБ
                info.conflicts.insert(conflictInRouteDiffer);
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
                  if (p1->equalRate(*p2)>=6) //6 - полное совпадение фамилии и имени
                  {
                    //есть пассажиры с такой фамилией и именем среди уже зарегистрированных
                    iTagMap->second.second.insert(conflictOutPaxDuplicate);
                    info.conflicts.insert(conflictOutPaxDuplicate);
                    break;
                  };
                if (p2!=paxs_ckin.end()) break;

                p2=paxs_crs.begin();
                for(; p2!=paxs_crs.end(); ++p2)
                {
                  if (p1->equalRate(*p2)>=6) //6 - полное совпадение фамилии и имени
                  {
                    //есть пассажиры с такой фамилией и именем среди забронированных
                    iTagMap->second.second.insert(conflictOutPaxDuplicate);
                    info.conflicts.insert(conflictOutPaxDuplicate);
                    break;
                  };
                };
                if (p2!=paxs_crs.end()) break;
              };
            };

            //добавим в pax_map
            pair<string, string> pax_out_id(paxOut->surname, paxOut->name);
            TNewGrpPaxMap::iterator iPaxMap=info.pax_map.find(pax_out_id);
            if (iPaxMap==info.pax_map.end())
              iPaxMap=info.pax_map.insert( make_pair(pax_out_id, set<TGrpId>()) ).first;
            if (iPaxMap==info.pax_map.end())
              throw Exception("GetNewGrpTags: iPaxMap==pax_map.end()");
            iPaxMap->second.insert(grp_in_id);
            if (iPaxMap->second.size()>1)
            {
              //дублирование пассажиров в списке входящего трансфера
              info.conflicts.insert(conflictInPaxDuplicate);
            };
          };
        };
        if (grpAlreadyCheckedIn) break;
      };
    };
  };

  if (!info.conflicts.empty())
  {
    ProgTrace(TRACE5, "GetNewGrpTags returned conflicts: %s", GetConflictStr(info.conflicts).c_str());
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
  set<TrferList::TAlarmType> global_alarms;
  for(set<TConflictReason>::const_iterator c=inbound_trfer.conflicts.begin();
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
    if (grpItem.bag_pool_num!=NoExists) //индикатор того, откуда багаж: из телеграмм или зарегистрированный в Астре
      grps_ckin.push_back(grpItem);
    else
      grps_tlg.push_back(grpItem);
    if (!global_alarms.empty() || !g->second.second.empty())
    {
      TrferList::TAlarmTagMap::iterator a=alarms.insert(make_pair(grpId, set<TrferList::TAlarmType>())).first;
      a->second.insert(global_alarms.begin(), global_alarms.end());
      for(set<TConflictReason>::const_iterator c=g->second.second.begin();
                                               c!=g->second.second.end(); ++c)
      {
        a->second.insert(GetConflictAlarm(*c));
      };
    };
  };

  TrferList::GrpsToGrpsView(TrferList::trferOutForCkin, true, grps_ckin, grps_tlg, alarms, grps);
};

void ConflictReasonsToLog(const set<TConflictReason> &conflicts,
                          TLogMsg &msg)
{
  for(set<TConflictReason>::const_iterator c=conflicts.begin(); c!=conflicts.end(); ++c)
  {
    ostringstream s;
    s << "Не привязан входящий трансферный багаж. Причина: ";
    switch(*c)
    {
      case conflictInPaxDuplicate:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.IN_PAX_DUPLICATE", AstraLocale::LANG_RU);
        break;
      case conflictOutPaxDuplicate:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.OUT_PAX_DUPLICATE", AstraLocale::LANG_RU);
        break;
      case conflictInRouteIncomplete:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.IN_ROUTE_INCOMPLETE", AstraLocale::LANG_RU);
        break;
      case conflictInRouteDiffer:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.IN_ROUTE_DIFFER", AstraLocale::LANG_RU);
        break;
      case conflictOutRouteDiffer:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.OUT_ROUTE_DIFFER", AstraLocale::LANG_RU);
        break;
      case conflictInOutRouteDiffer:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.IN_OUT_ROUTE_DIFFER", AstraLocale::LANG_RU);
        break;
      case conflictWeightNotDefined:
        s << AstraLocale::getLocaleText("MSG.TRFER_CONFLICT_REASON.WEIGHT_NOT_DEFINED", AstraLocale::LANG_RU);
        break;
    };
    msg.msg=s.str();
    TReqInfo::Instance()->MsgToLog(msg);
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
  //это более подробный анализ, но он требует более внимательного отношения к местам вызова процедуры
  //поэтому пока делаем проще и используем более быстрый SQL запрос
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


