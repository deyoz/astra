#include "transfer.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "term_version.h"
#include "alarms.h"

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
      tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
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

  if (type==trferOut)
  {
    //Информация о трансферном багаже/пассажирах, отправляющемся рейсом
    sql << "     pax_grp.point_dep AS point_id, pax_grp.airp_arv, \n"
           "     transfer.airp_arv AS last_airp_arv \n"
           "FROM trfer_trips, transfer, pax_grp, bag2 \n"
           "WHERE trfer_trips.point_id=transfer.point_id_trfer AND \n"
           "      transfer.grp_id=pax_grp.grp_id AND \n"
           "      pax_grp.grp_id=bag2.grp_id(+) AND \n"
           "      trfer_trips.flt_no=:flt_no AND \n"
           "      trfer_trips.scd=:scd AND \n"
           "      trfer_trips.airline=:airline AND \n"
           "      trfer_trips.airp_dep=:airp_dep AND \n"
           "      (trfer_trips.suffix IS NULL AND :suffix IS NULL OR \n"
           "       trfer_trips.suffix=:suffix) AND \n"
           "      transfer.transfer_num=1 AND \n";
    Qry.CreateVariable("flt_no", otInteger, flt.flt_no);
    if (flt.scd_out!=NoExists)
    {
      TDateTime scd=UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
      modf(scd, &scd);
      Qry.CreateVariable("scd", otDate, scd);
    }
    else
      Qry.CreateVariable("scd", otDate, FNull);
    Qry.CreateVariable("airline", otString, flt.airline);
    Qry.CreateVariable("airp_dep", otString, flt.airp);
    Qry.CreateVariable("suffix", otString, flt.suffix);
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

  sql << "      pax_grp.bag_refuse=0 AND pax_grp.status<>'T' \n"
         "GROUP BY pax_grp.point_dep, pax_grp.grp_id, bag2.bag_pool_num, pax_grp.class, \n";
  if (type==trferOut)
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
  if (type==trferOut || type==tckinInbound)
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
  if (type==trferIn || type==trferOut)
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
    if (type==trferOut || type==trferIn)
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
    if (type==trferOut)
      grp.last_airp_arv=Qry.FieldAsString("last_airp_arv");
    if (type==trferOut || type==trferCkin || type==tckinInbound)
      grp.subcl=Qry.FieldAsString("class");
    if (Qry.FieldIsNULL("class"))
    {
      grp.fromDB(Qry, TagQry, false, pr_bag);
      grp.setPaxUnaccomp();
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
        grps_ckin.push_back(grp);
      };
    };
  };

  //ProgTrace(TRACE5, "grps_ckin.size()=%zu", grps_ckin.size());

  if (!(type==trferOut || type==trferIn)) return;

  FltQry.Clear();
  FltQry.SQLText=
    "SELECT airline, flt_no, suffix, scd_out, airp, pr_del "
    "FROM points "
    "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND "
    "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
    "      scd_out >= TO_DATE(:scd)-1 AND scd_out < TO_DATE(:scd)+2 AND "
    "      pr_del>=0 AND "
    "      EXISTS(SELECT pax_grp.point_dep FROM pax_grp WHERE pax_grp.point_dep=points.point_id)";
  FltQry.DeclareVariable("flt_no", otInteger);
  FltQry.DeclareVariable("scd", otDate);
  FltQry.DeclareVariable("airline", otString);
  FltQry.DeclareVariable("airp_dep", otString);
  FltQry.DeclareVariable("suffix", otString);

  Qry.Clear();
  if (type==trferOut)
  {
    //Информация о трансферном багаже/пассажирах, отправляющемся рейсом
    Qry.SQLText=
      "SELECT tlg_trips_in.airline, tlg_trips_in.flt_no, tlg_trips_in.suffix, \n"
      "       tlg_trips_in.airp_dep AS airp, tlg_trips_in.scd AS scd_out, \n"
      "       tlg_trips_in.point_id AS point_id_tlg_in, tlg_trips_in.airp_arv, \n"
      "       tlg_transfer.subcl_in AS subcl, \n"
      "       trfer_grp.grp_id, trfer_grp.seats, \n"
      "       trfer_grp.bag_amount, trfer_grp.bag_weight, trfer_grp.rk_weight, trfer_grp.weight_unit \n"
      "FROM tlg_binding tlg_binding_out, tlg_transfer, tlgs_in, trfer_grp, tlg_trips tlg_trips_in \n"
      "WHERE tlg_binding_out.point_id_tlg=tlg_transfer.point_id_out AND \n"
      "      tlg_binding_out.point_id_spp=:point_id AND \n"
      "      tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND \n"
      "      tlgs_in.type=:tlg_type AND \n"
      "      tlg_transfer.trfer_id=trfer_grp.trfer_id AND \n"
      "      tlg_transfer.point_id_in=tlg_trips_in.point_id \n";
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
      FltQry.SetVariable("flt_no", flt1.flt_no);
      FltQry.SetVariable("scd", flt1.scd_out);
      FltQry.SetVariable("airline", flt1.airline);
      FltQry.SetVariable("airp_dep", flt1.airp);
      FltQry.SetVariable("suffix", flt1.suffix);
      FltQry.Execute();
      for(;!FltQry.Eof;FltQry.Next())
      {
        TFltInfo flt2(FltQry, true);
        if (!(flt1<flt2) && !(flt2<flt1)) break;
      };
      if(FltQry.Eof)
        //не нашли рейс - значит не обслуживается в Астре
        id=flts_from_tlg.insert(flt1).first;
      else
        //данный телеграммный рейс обслуживается в Астре
        flts_from_ckin.insert(flt1);
    };
    if (id==flts_from_tlg.end()) continue;

    TGrpItem grp;
    if (type==trferOut)
      grp.point_id=Qry.FieldAsInteger("point_id_tlg_in");
    if (type==trferIn)
      grp.point_id=Qry.FieldAsInteger("point_id_tlg_out");
    grp.grp_id=Qry.FieldAsInteger("grp_id");
    grp.airp_arv=Qry.FieldAsString("airp_arv");
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

    grps_tlg.push_back(grp);
  };
  //ProgTrace(TRACE5, "grps_tlg.size()=%zu", grps_tlg.size());
}

void TrferToXML(TTrferType type,
                int point_id,
                bool pr_bag,
                const TTripInfo &flt,
                const vector<TGrpItem> &grps_ckin,
                const vector<TGrpItem> &grps_tlg,
                xmlNodePtr resNode)
{
  vector<TGrpViewItem> grps;
  TQuery FltQry(&OraSession);

  for(int from_tlg=0; from_tlg<2; from_tlg++)
  {
    map<int, TFltInfo> point_ids;
    FltQry.Clear();
    if (!from_tlg)
    {
      if (type==trferOut || type==tckinInbound)
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
      if ((type==trferIn || type==trferOut) && pr_bag && g->tags.empty()) continue; //BTM не содержат пассажиров без бирок
      if ((type==trferIn || type==trferOut) && !pr_bag && g->is_unaccomp) continue; //PTM не содержат несопровождаемый багаж

      map<int, TFltInfo>::const_iterator i=point_ids.find(g->point_id);
      if (i==point_ids.end())
      {
        FltQry.SetVariable("point_id", g->point_id);
        FltQry.Execute();
        if (FltQry.Eof) continue;
        bool calc_local_time=!from_tlg && (type==trferOut || type==tckinInbound);
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
      if (type==trferOut)
        grp.tlg_airp_view=grp.flt_view.airp;

      if (type!=trferOut && !g->subcl.empty())
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

      grps.push_back(grp);
    };
  };
  //ProgTrace(TRACE5, "grps.size()=%zu", grps.size());

  InboundTrfer::TUnattachedTagMap unattached_grps;
  if (type==trferOut && pr_bag)
  {
    GetUnattachedTags(point_id, grps_ckin, grps_tlg, unattached_grps);
    //ProgTrace(TRACE5, "unattached_grps.size()=%zu", unattached_grps.size());
  };

  sort(grps.begin(),grps.end());

  TReqInfo *reqInfo = TReqInfo::Instance();
  //формируем XML
  TTripInfo flt_tmp=flt;
  flt_tmp.pr_del=0; //чтобы не выводилось "(отм.)"
  NewTextChild(resNode,"trip",GetTripName(flt_tmp,ecNone));

  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");

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
      if (type==trferIn || type==trferOut)
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
      if (type==trferIn || type==trferOut || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS) ||
          iGrpPrior==grps.end() || iGrpPrior->grp_id!=iGrp->grp_id)
      {
        grpNode=NewTextChild(grpsNode,"grp");
        newPaxsNode=true;
      };
    };

    if (type==trferIn || type==trferOut || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
    {
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

      bool unattached_alarm=false;
      if (type==trferOut && pr_bag)
      {
        //передадим для группы тревогу atUnattachedTrfer, если непривязанные бирки
        InboundTrfer::TGrpId id(iGrp->grp_id, iGrp->bag_pool_num);

        InboundTrfer::TUnattachedTagMap::const_iterator g=unattached_grps.find(id);
        if (g!=unattached_grps.end())
        {
          if (!g->second.empty() && g->second.size()==iGrp->tags.size())
            //все бирки группы непривязаны
            unattached_alarm=true;
        };
      };

      vector<string> tagRanges;
      GetTagRanges(iGrp->tags, tagRanges);
      if (!tagRanges.empty())
      {
        xmlNodePtr node=NewTextChild(grpNode,"tag_ranges");
        for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
        {
          xmlNodePtr rangeNode=NewTextChild(node,"range",*r);
          if (unattached_alarm)
            NewTextChild(rangeNode, "alarm", EncodeAlarmType(atUnattachedTrfer));
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
        if (!(type==trferIn || type==trferOut || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS)))

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

bool trferInExists(int point_arv, int prior_point_arv, TQuery& Qry)
{
  const char *sql =
    "SELECT 1 "
    "FROM pax_grp, transfer, pax "
    "WHERE pax_grp.grp_id=transfer.grp_id AND "
    "      pax_grp.grp_id=pax.grp_id(+) AND "
    "      pax_grp.point_arv=:point_arv AND "
    "      transfer.transfer_num=1 AND "
    "      pax_grp.bag_refuse=0 AND pax_grp.status<>'T' AND "
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

bool trferOutExists(int point_dep, const TTripInfo &flt, TQuery& Qry)
{
  const char *sql =
    "SELECT 1 "
    "FROM trfer_trips, transfer, pax_grp, pax "
    "WHERE trfer_trips.point_id=transfer.point_id_trfer AND "
    "      transfer.grp_id=pax_grp.grp_id AND "
    "      pax_grp.grp_id=pax.grp_id(+) AND "
    "      trfer_trips.flt_no=:flt_no AND "
    "      trfer_trips.scd=:scd AND "
    "      trfer_trips.airline=:airline AND "
    "      trfer_trips.airp_dep=:airp_dep AND "
    "      (trfer_trips.suffix IS NULL AND :suffix IS NULL OR "
    "       trfer_trips.suffix=:suffix) AND "
    "      transfer.transfer_num=1 AND "
    "      pax_grp.bag_refuse=0 AND pax_grp.status<>'T' AND "
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
    Qry.DeclareVariable("flt_no", otInteger);
    Qry.DeclareVariable("scd", otDate);
    Qry.DeclareVariable("airline", otString);
    Qry.DeclareVariable("airp_dep", otString);
    Qry.DeclareVariable("suffix", otString);
  };
  Qry.SetVariable("point_dep", point_dep);
  Qry.SetVariable("flt_no", flt.flt_no);
  if (flt.scd_out!=NoExists)
  {
    TDateTime scd=UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
    modf(scd, &scd);
    Qry.SetVariable("scd", scd);
  }
  else
    Qry.SetVariable("scd", FNull);
  Qry.SetVariable("airline", flt.airline);
  Qry.SetVariable("airp_dep", flt.airp);
  Qry.SetVariable("suffix", flt.suffix);
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
    "      pax_grp.bag_refuse=0 AND pax_grp.status<>'T' AND "
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
  tags.assign(grp.tags.begin(), grp.tags.end());
};

int TGrpItem::equalRate(const TGrpItem &item, int minPaxEqualRate) const
{
  if (is_unaccomp!=item.is_unaccomp) return 0;
  if (airp_arv!=item.airp_arv) return 0;
  int rate=0;
  if (!is_unaccomp)
  {
    //обычная группа пассажиров
    std::list<TPaxItem> paxs1(paxs.begin(), paxs.end());
    std::list<TPaxItem> paxs2(item.paxs.begin(), item.paxs.end());
    for(int r=7; r>0 && r>=minPaxEqualRate; r--)
    {
      for(std::list<TPaxItem>::iterator p1=paxs1.begin(); p1!=paxs1.end();)
      {
        std::list<TPaxItem>::iterator p2=paxs2.begin();
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

typedef map<TBagTagNumber, pair< list<TGrpId>,
                                 list<TGrpId> > > TTagMap;

void FilterUnattachedTags(const map<TGrpId, TGrpItem> &grps_in,
                          const map<TGrpId, TGrpItem> &grps_out,
                          TTagMap &tags)
{
  TDateTime d=NowUTC();
  //ProgTrace(TRACE5, "FilterUnattachedTags: started");
  for(TTagMap::iterator t=tags.begin(); t!=tags.end(); ++t)
  {
    //ProgTrace(TRACE5, "FilterUnattachedTags: no=%s", GetTagRangesStr(vector<TBagTagNumber>(1,t->first)).c_str());
    if (t->second.second.empty()) continue;
    for(int minPaxEqualRate=7; minPaxEqualRate>=6; minPaxEqualRate--)   //6 - полное совпадение фамилии и имени
    {
      //ProgTrace(TRACE5, "FilterUnattachedTags: minPaxEqualRate=%d", minPaxEqualRate);
      while (!t->second.second.empty())
      {
        pair< list<TGrpId>::iterator, list<TGrpId>::iterator > max(t->second.first.end(), t->second.second.end());
        int maxEqualPaxCount=0;
        for(list<TGrpId>::iterator i1=t->second.first.begin(); i1!=t->second.first.end(); ++i1)
          for(list<TGrpId>::iterator i2=t->second.second.begin(); i2!=t->second.second.end(); ++i2)
          {
            map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(*i1);
            if (grp_in==grps_in.end()) throw Exception("FilterUnattachedTags: grp_in=grps_in.end()");
            map<TGrpId, TGrpItem>::const_iterator grp_out=grps_out.find(*i2);
            if (grp_out==grps_out.end()) throw Exception("FilterUnattachedTags: grp_out=grps_out.end()");
            int equalPaxCount=grp_in->second.equalRate(grp_out->second, minPaxEqualRate);
            if (equalPaxCount>maxEqualPaxCount)
            {
              maxEqualPaxCount=equalPaxCount;
              max=make_pair(i1, i2);
            };
          };
        if (maxEqualPaxCount>0)
        {
          //ProgTrace(TRACE5, "FilterUnattachedTags: no=%s minPaxEqualRate=%d [%s]<->[%s]",
          //                  GetTagRangesStr(vector<TBagTagNumber>(1,t->first)).c_str(), minPaxEqualRate,
          //                  max.first->getStr().c_str(), max.second->getStr().c_str());
          t->second.first.erase(max.first);
          t->second.second.erase(max.second);
        }
        else break;
      };
    };
  };

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
                    bool is_point_id,
                    const TTagMap &filter_tags,
                    map<TGrpId, TGrpItem> &grps_out)
{
  grps_out.clear();

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
    "      bag2.num=bag_tags.bag_num AND ";
  if (is_point_id)
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

    grp_out->second.tags.push_back(tag);
  };
};

void GetCheckedTags(int id,  //м.б. point_id или grp_id
                    bool is_point_id,
                    map<TGrpId, TGrpItem> &grps_out)
{
  GetCheckedTags(id, is_point_id, TTagMap(), grps_out);
};




void GetUnattachedTags(int point_id,
                       const vector<TrferList::TGrpItem> &grps_ckin,
                       const vector<TrferList::TGrpItem> &grps_tlg,
                       TUnattachedTagMap &result)
{
  result.clear();

  TTagMap tags;
  map<TGrpId, TGrpItem> grps_in;
  for(int from_tlg=0; from_tlg<2; from_tlg++)
  {
    vector<TrferList::TGrpItem>::const_iterator g=from_tlg?grps_tlg.begin():grps_ckin.begin();
    for(; g!=(from_tlg?grps_tlg.end():grps_ckin.end()); ++g)
    {
      if (g->tags.empty()) continue;
      TGrpId id(g->grp_id, g->bag_pool_num);
      map<TGrpId, TGrpItem>::const_iterator grp_in=grps_in.find(id);
      if (grp_in==grps_in.end())
        grp_in=grps_in.insert(make_pair(id, TGrpItem(*g))).first;
      if (grp_in==grps_in.end()) throw Exception("GetUnattachedTags: grp_in=grps_in.end()");

      for(vector<TBagTagNumber>::const_iterator t=g->tags.begin(); t!=g->tags.end(); ++t)
      {
        TTagMap::iterator tag=tags.find(*t);
        if (tag==tags.end())
          tag=tags.insert(make_pair(*t, pair< list<TGrpId>, list<TGrpId> >())).first;
        if (tag==tags.end()) throw Exception("GetUnattachedTags: tag==tags.end()");
        tag->second.first.push_back(id);
      };
    };
  };

  if (grps_in.empty() || tags.empty()) return;

  map<TGrpId, TGrpItem> grps_out;
  GetCheckedTags(point_id, true, tags, grps_out);
  for(map<TGrpId, TGrpItem>::const_iterator g=grps_out.begin(); g!=grps_out.end(); ++g)
  {
    for(list<TBagTagNumber>::const_iterator t=g->second.tags.begin(); t!=g->second.tags.end(); ++t)
    {
      TTagMap::iterator tag=tags.find(*t);
      if (tag==tags.end()) continue;

      tag->second.second.push_back(g->first);
    };
  };

  FilterUnattachedTags(grps_in, grps_out, tags);

  for(TTagMap::const_iterator t=tags.begin(); t!=tags.end(); ++t)
  {
    for(list<TGrpId>::const_iterator g=t->second.first.begin(); g!=t->second.first.end(); ++g)
    {
      TUnattachedTagMap::iterator r=result.find(*g);
      if (r==result.end())
        r=result.insert(make_pair(*g, vector<TBagTagNumber>())).first;
      if (r==result.end()) throw Exception("GetUnattachedTags: r==result.end()");
      r->second.push_back(t->first);
    };
  };
/*
  for(TUnattachedTagMap::const_iterator r=result.begin(); r!=result.end(); ++r)
  {
    for(vector<TBagTagNumber>::const_iterator t=r->second.begin(); t!=r->second.end(); ++t)
      ProgTrace(TRACE5, "%s: %s",
                        r->first.getStr().c_str(),
                        GetTagRangesStr(vector<TBagTagNumber>(1,*t)).c_str());
  };
*/
};

}; //namespace InboundTrfer

namespace TrferListOld
{

//потом удалить:
class TTransferPaxItem
{
  public:
    string surname, name;
    int seats;
    int bag_amount, bag_weight, rk_weight;
    vector<TBagTagNumber> tags;
    TTransferPaxItem()
    {
      seats=NoExists;
      bag_amount=NoExists;
      bag_weight=NoExists;
      rk_weight=NoExists;
    };
};

class TTransferGrpItem
{
  public:
    int grp_id;
    int inbound_point_dep; //только для GetInboundTCkin
    string inbound_trip;   //только для GetInboundTCkin
    string airline_view;
    int flt_no;
    string suffix_view;
    TDateTime scd_local;
    string airp_dep_view, airp_arv_view, subcl_view;
    string tlg_airp_view;
    int subcl_priority;
    int seats;
    int bag_amount, bag_weight, rk_weight;
    string weight_unit;
    vector<TTransferPaxItem> pax;
    vector<TBagTagNumber> tags;

    bool operator < (const TTransferGrpItem &grp) const
    {
      if (scd_local!=grp.scd_local)
        return scd_local<grp.scd_local;
      if (airline_view!=grp.airline_view)
        return airline_view<grp.airline_view;
      if (flt_no!=grp.flt_no)
        return flt_no<grp.flt_no;
      if (suffix_view!=grp.suffix_view)
        return suffix_view<grp.suffix_view;
      if (airp_dep_view!=grp.airp_dep_view)
        return airp_dep_view<grp.airp_dep_view;
      if (inbound_point_dep!=grp.inbound_point_dep)
        return inbound_point_dep<grp.inbound_point_dep;
      if (airp_arv_view!=grp.airp_arv_view)
        return airp_arv_view<grp.airp_arv_view;
      if (subcl_priority!=grp.subcl_priority)
        return subcl_priority<grp.subcl_priority;
      if (subcl_view!=grp.subcl_view)
        return subcl_view<grp.subcl_view;
      return grp_id<grp.grp_id;
    };

};

void GetTransfer(bool pr_inbound_tckin,
                 bool pr_out,
                 bool pr_tlg,
                 bool pr_bag,
                 int point_id,
                 xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery Qry(&OraSession);
  TQuery PointsQry(&OraSession);
  TQuery PaxQry(&OraSession);
  TQuery RemQry(&OraSession);
  TQuery TagQry(&OraSession);

  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT point_id,airp,airline,flt_no,suffix,craft,bort,scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  PointsQry.DeclareVariable( "point_id", otInteger );

  if (!pr_out)
  {
    //point_id содержит пункт прилета а нам нужен предыдущий пункт вылета
    TTripRouteItem priorAirp;
    TTripRoute().GetPriorAirp(NoExists,point_id,trtNotCancelled,priorAirp);
    if (priorAirp.point_id==NoExists) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
    point_id=priorAirp.point_id;
  };
  PointsQry.SetVariable( "point_id", point_id );
  PointsQry.Execute();
  if (PointsQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
  TTripInfo info(PointsQry);

  Qry.Clear();
  if (pr_tlg)
  {
    if (pr_out)
      //Информация о трансферном багаже/пассажирах, отправляющемся рейсом [flight%s][EOL](данные на основе телеграмм BTM)
      Qry.SQLText=
        "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "       tlg_trips.scd,tlg_trips.airp_dep AS airp,tlg_trips.airp_dep,tlg_trips.airp_arv, "
        "       tlg_transfer.trfer_id,tlg_transfer.subcl_in AS subcl, "
        "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
        "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
        "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
        "      tlgs_in.type=:tlg_type AND "
        "      tlg_transfer.point_id_out=tlg_binding.point_id_tlg AND "
        "      tlg_binding.point_id_spp=:point_id AND "
        "      tlg_trips.point_id=tlg_transfer.point_id_in AND "
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id ";
    else
      //Информация о трансферном багаже/пассажирах, прибывающем рейсом [flight%s][EOL](данные на основе телеграмм BTM)
      Qry.SQLText=
        "SELECT tlg_trips.point_id,tlg_trips.airline,tlg_trips.flt_no,tlg_trips.suffix, "
        "       tlg_trips.scd,tlg_trips.airp_arv AS airp,tlg_trips.airp_dep,tlg_trips.airp_arv, "
        "       tlg_transfer.trfer_id,tlg_transfer.subcl_out AS subcl, "
        "       trfer_grp.grp_id,seats,bag_amount,bag_weight,rk_weight,weight_unit "
        "FROM trfer_grp,tlgs_in,tlg_trips,tlg_transfer,tlg_binding "
        "WHERE tlg_transfer.tlg_id=tlgs_in.id AND tlgs_in.num=1 AND "
        "      tlgs_in.type=:tlg_type AND "
        "      tlg_transfer.point_id_in=tlg_binding.point_id_tlg AND "
        "      tlg_binding.point_id_spp=:point_id AND "
        "      tlg_trips.point_id=tlg_transfer.point_id_out AND "
        "      tlg_transfer.trfer_id=trfer_grp.trfer_id ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    if (pr_bag)
      Qry.CreateVariable("tlg_type",otString,"BTM");
    else
      Qry.CreateVariable("tlg_type",otString,"PTM");

    PaxQry.SQLText="SELECT surname,name FROM trfer_pax WHERE grp_id=:grp_id ORDER BY surname,name";
    PaxQry.DeclareVariable("grp_id",otInteger);

    TagQry.SQLText=
      "SELECT no FROM trfer_tags WHERE grp_id=:grp_id";
    TagQry.DeclareVariable("grp_id",otInteger);
  }
  else
  {
    if (pr_inbound_tckin)
    {
      //Стыковочные рейсы, которыми прибывают пассажиры рейса [flight%s][EOL](данные на основе сквозной регистрации)
      Qry.SQLText=
        "SELECT tckin_pax_grp.tckin_id,tckin_pax_grp.seg_no, "
        "       pax_grp.grp_id,pax_grp.airp_arv,pax_grp.class AS subcl "
        "FROM pax_grp,tckin_pax_grp "
        "WHERE pax_grp.grp_id=tckin_pax_grp.grp_id AND "
        "      pax_grp.point_dep=:point_id AND bag_refuse=0 AND pax_grp.status<>'T' ";
    }
    else
    {
      //Информация о трансферном багаже/пассажирах, отправляющемся рейсом [flight%s][EOL](данные на основе результатов регистрации)
      Qry.SQLText=
        "SELECT trfer_trips.airline,trfer_trips.flt_no,trfer_trips.suffix,trfer_trips.scd, "
        "       trfer_trips.airp_dep,transfer.airp_arv, "
        "       pax_grp.grp_id,pax_grp.class AS subcl "
        "FROM pax_grp,transfer,trfer_trips "
        "WHERE pax_grp.grp_id=transfer.grp_id AND "
        "      transfer.point_id_trfer=trfer_trips.point_id AND "
        "      transfer.transfer_num=1 AND "
        "      pax_grp.point_dep=:point_id AND bag_refuse=0 AND pax_grp.status<>'T' ";
    };
    Qry.CreateVariable("point_id",otInteger,point_id);

    PaxQry.SQLText=
      "SELECT pax_id,surname,name,seats,bag_pool_num, "
      "       ckin.get_bag_pool_pax_id(pax.grp_id,pax.bag_pool_num) AS bag_pool_pax_id, "
      "       NVL(ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
      "       NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
      "       NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight "
      "FROM pax_grp, pax "
      "WHERE pax_grp.grp_id=pax.grp_id(+) AND pax_grp.grp_id=:grp_id AND pax.pr_brd(+) IS NOT NULL "
      "ORDER BY NVL(bag_pool_num,1000), DECODE(seats,0,0,-1), surname, name";  //!!!добавлено специально для тестов
    PaxQry.DeclareVariable("grp_id",otInteger);

    RemQry.SQLText=
      "SELECT rem_code FROM pax_rem "
      "WHERE pax_id=:pax_id AND rem_code IN ('STCR', 'EXST') "
      "ORDER BY DECODE(rem_code,'STCR',0,'EXST',1,2) ";
    RemQry.DeclareVariable("pax_id",otInteger);

    TagQry.SQLText=
      "SELECT bag_tags.no "
      "FROM bag_tags,bag2 "
      "WHERE bag_tags.grp_id=bag2.grp_id(+) AND "
      "      bag_tags.bag_num=bag2.num(+) AND "
      "      bag_tags.grp_id=:grp_id AND "
      "      NVL(bag2.bag_pool_num,1)=:bag_pool_num";
    TagQry.DeclareVariable("bag_pool_num",otInteger);
    TagQry.DeclareVariable("grp_id",otInteger);
  };

  Qry.Execute();
  vector<TTransferGrpItem> grps;
  for(;!Qry.Eof;Qry.Next())
  {
    TTransferGrpItem grp;
    grp.grp_id=Qry.FieldAsInteger("grp_id");
    if (!pr_tlg && pr_inbound_tckin)
    {
      TCkinRouteItem inboundSeg;
      TCkinRoute().GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                               Qry.FieldAsInteger("seg_no"),
                               crtIgnoreDependent, inboundSeg);
      if (inboundSeg.grp_id==NoExists) continue;

      TTripRouteItem priorAirp;
      TTripRoute().GetPriorAirp(NoExists,inboundSeg.point_arv,trtNotCancelled,priorAirp);
      if (priorAirp.point_id==NoExists) continue;

      PointsQry.SetVariable( "point_id", priorAirp.point_id );
      PointsQry.Execute();
      if (PointsQry.Eof) continue;
      TTripInfo inFlt(PointsQry);

      grp.inbound_point_dep=priorAirp.point_id;
      grp.inbound_trip=GetTripName(inFlt,ecNone);
      grp.airline_view=ElemIdToCodeNative(etAirline,inFlt.airline);
      grp.flt_no=inFlt.flt_no;
      grp.suffix_view=ElemIdToCodeNative(etSuffix,inFlt.suffix);
      grp.scd_local=inFlt.scd_out==NoExists?
                    NoExists:
                    UTCToLocal(inFlt.scd_out, AirpTZRegion(inFlt.airp));
      modf(grp.scd_local, &grp.scd_local);//!!!добавлено специально для тестов
      grp.airp_dep_view=ElemIdToCodeNative(etAirp,inboundSeg.airp_dep);
      grp.airp_arv_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_arv"));
    }
    else
    {
      grp.inbound_point_dep=NoExists;
      grp.inbound_trip="";
      grp.airline_view=ElemIdToCodeNative(etAirline,Qry.FieldAsString("airline"));
      grp.flt_no=Qry.FieldAsInteger("flt_no");
      grp.suffix_view=ElemIdToCodeNative(etSuffix,Qry.FieldAsString("suffix"));
      grp.scd_local=Qry.FieldAsDateTime("scd");
      grp.airp_dep_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_dep"));
      grp.airp_arv_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp_arv"));
    };

    if (pr_tlg)
    {
      grp.tlg_airp_view=ElemIdToCodeNative(etAirp,Qry.FieldAsString("airp"));
      grp.seats=!Qry.FieldIsNULL("seats")?Qry.FieldAsInteger("seats"):NoExists;
      grp.bag_amount=!Qry.FieldIsNULL("bag_amount")?Qry.FieldAsInteger("bag_amount"):NoExists;
      grp.bag_weight=!Qry.FieldIsNULL("bag_weight")?Qry.FieldAsInteger("bag_weight"):NoExists;
      grp.rk_weight=!Qry.FieldIsNULL("rk_weight")?Qry.FieldAsInteger("rk_weight"):NoExists;
      grp.weight_unit=Qry.FieldAsString("weight_unit");
      if (pr_bag)
      {
        TagQry.SetVariable("grp_id",grp.grp_id);
        TagQry.Execute();
        for(;!TagQry.Eof;TagQry.Next())
          grp.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
      };
    }
    else
    {
      grp.seats=0;
      grp.bag_amount=0;
      grp.bag_weight=0;
      grp.rk_weight=0;
      grp.weight_unit="K";
    };

    //разберемся с классом
    string subcl=Qry.FieldAsString("subcl");
    if (!subcl.empty())
    {
      grp.subcl_view=ElemIdToCodeNative(etSubcls,subcl); //пустой для несопровождаемого багажа
      grp.subcl_priority=0;
      try
      {
        TSubclsRow &subclsRow=(TSubclsRow&)base_tables.get("subcls").get_row("code",subcl);
        grp.subcl_priority=((TClassesRow&)base_tables.get("classes").get_row("code",subclsRow.cl)).priority;
      }
      catch(EBaseTableError){};
    }
    else
    {
      grp.subcl_priority=pr_tlg?0:10;
    };

    //пассажиры
    PaxQry.SetVariable("grp_id",grp.grp_id);
    PaxQry.Execute();
    if (PaxQry.Eof) continue; //пустая группа - не помещаем в grps
    if (pr_tlg)
    {
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        TTransferPaxItem pax;
        //транслитерация не нужна так как телеграммы PTM, BTM должны приходить на латинском
        //и регистрация (списки) должна проводиться на латинском
        pax.surname=PaxQry.FieldAsString("surname");
        pax.name=PaxQry.FieldAsString("name");
        grp.pax.push_back(pax);
      };
    }
    else
    {
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        TTransferPaxItem pax;
        if (subcl.empty())
        {
          pax.surname="UNACCOMPANIED";
          pax.seats=0;
        }
        else
        {
          pax.surname=PaxQry.FieldAsString("surname");
          pax.name=PaxQry.FieldAsString("name");
          pax.seats=PaxQry.FieldAsInteger("seats");
          if (pax.seats>1)
          {
            RemQry.SetVariable("pax_id",PaxQry.FieldAsInteger("pax_id"));
            RemQry.Execute();
            for(int i=2; i<=pax.seats; i++)
            {
              pax.name+="/";
              if (!RemQry.Eof)
                pax.name+=RemQry.FieldAsString("rem_code");
              else
                pax.name+="EXST";
            };
          };
        };
        //багаж пассажира
        pax.bag_amount=PaxQry.FieldAsInteger("bag_amount");
        pax.bag_weight=PaxQry.FieldAsInteger("bag_weight");
        pax.rk_weight=PaxQry.FieldAsInteger("rk_weight");

        grp.seats+=pax.seats;
        grp.bag_amount+=pax.bag_amount;
        grp.bag_weight+=pax.bag_weight;
        grp.rk_weight+=pax.rk_weight;

        if (pr_bag &&
            (subcl.empty() ||
             (!PaxQry.FieldIsNULL("bag_pool_num") &&
              !PaxQry.FieldIsNULL("pax_id") &&
              !PaxQry.FieldIsNULL("bag_pool_pax_id") &&
              PaxQry.FieldAsInteger("pax_id")==PaxQry.FieldAsInteger("bag_pool_pax_id"))))
        {
          TagQry.SetVariable("grp_id",grp.grp_id);
          if (subcl.empty())
            TagQry.SetVariable("bag_pool_num", 1);
          else
            TagQry.SetVariable("bag_pool_num", PaxQry.FieldAsInteger("bag_pool_num"));
          TagQry.Execute();
          for(;!TagQry.Eof;TagQry.Next())
          {
            pax.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
            grp.tags.push_back(TBagTagNumber("",TagQry.FieldAsFloat("no")));
          };
        };

        grp.pax.push_back(pax);
      };
    };

    grps.push_back(grp);
  };

  sort(grps.begin(),grps.end());

  //формируем XML

  NewTextChild(resNode,"trip",GetTripName(info,ecNone));

  xmlNodePtr trferNode=NewTextChild(resNode,"transfer");

  xmlNodePtr grpsNode;
  vector<TTransferGrpItem>::const_iterator iGrpPrior=grps.end();
  for(vector<TTransferGrpItem>::const_iterator iGrp=grps.begin();iGrp!=grps.end();++iGrp)
  {
    if (iGrpPrior==grps.end() ||
        iGrpPrior->airline_view!=iGrp->airline_view ||
        iGrpPrior->flt_no!=iGrp->flt_no ||
        iGrpPrior->suffix_view!=iGrp->suffix_view ||
        iGrpPrior->scd_local!=iGrp->scd_local ||
        iGrpPrior->airp_dep_view!=iGrp->airp_dep_view ||
        iGrpPrior->inbound_point_dep!=iGrp->inbound_point_dep ||
        iGrpPrior->airp_arv_view!=iGrp->airp_arv_view ||
        iGrpPrior->subcl_view!=iGrp->subcl_view)
    {
      xmlNodePtr node=NewTextChild(trferNode,"trfer_flt");

      ostringstream trip;
      trip << iGrp->airline_view
           << setw(3) << setfill('0') << iGrp->flt_no
           << iGrp->suffix_view
           << "/"
           << (iGrp->scd_local==NoExists?"??":DateTimeToStr(iGrp->scd_local,"dd"));

      NewTextChild(node,"trip",trip.str());
      if (pr_tlg) NewTextChild(node,"airp",iGrp->tlg_airp_view);
      NewTextChild(node,"airp_dep",iGrp->airp_dep_view);
      NewTextChild(node,"airp_arv",iGrp->airp_arv_view);
      NewTextChild(node,"subcl",iGrp->subcl_view);
      NewTextChild(node,"point_dep",iGrp->inbound_point_dep,NoExists); //только для GetInboundTCkin
      NewTextChild(node,"trip2",iGrp->inbound_trip,"");                //только для GetInboundTCkin
      grpsNode=NewTextChild(node,"grps");
    };

    xmlNodePtr grpNode=NewTextChild(grpsNode,"grp");
    if (pr_tlg || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
    {
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
        xmlNodePtr node=NewTextChild(grpNode,"tag_ranges");
        for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
          NewTextChild(node,"range",*r);
      };
    };

    if (!iGrp->pax.empty())
    {
      xmlNodePtr paxsNode=NewTextChild(grpNode,"passengers");
      for(vector<TTransferPaxItem>::const_iterator iPax=iGrp->pax.begin(); iPax!=iGrp->pax.end(); ++iPax)
      {
        xmlNodePtr paxNode=NewTextChild(paxsNode,"pax");
        NewTextChild(paxNode,"surname",iPax->surname);
        NewTextChild(paxNode,"name",iPax->name,"");
        if (!(pr_tlg || !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS)))
        {
          if (iPax->bag_amount!=NoExists)
            NewTextChild(paxNode,"bag_amount",iPax->bag_amount,0);
          if (iPax->bag_weight!=NoExists)
            NewTextChild(paxNode,"bag_weight",iPax->bag_weight,0);
          if (iPax->rk_weight!=NoExists)
            NewTextChild(paxNode,"rk_weight",iPax->rk_weight,0);
          NewTextChild(paxNode,"seats",iPax->seats,1);

          vector<string> tagRanges;
          GetTagRanges(iPax->tags, tagRanges);
          if (!tagRanges.empty())
          {
            xmlNodePtr node=NewTextChild(paxNode,"tag_ranges");
            for(vector<string>::const_iterator r=tagRanges.begin(); r!=tagRanges.end(); ++r)
              NewTextChild(node,"range",*r);
          };
        };
      };
    };

    iGrpPrior=iGrp;
  };
};

}; //namespace TrferListOld