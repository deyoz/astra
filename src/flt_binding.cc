#include "flt_binding.h"
#include "oralib.h"
#include "salons.h"
#include "comp_layers.h"
#include "alarms.h"
#include "trip_tasks.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

void crs_recount(int point_id_tlg, int point_id_spp, bool check_comp)
{
  TQuery ProcQry(&OraSession);
  ProcQry.Clear();
  ProcQry.SQLText=
    "BEGIN "
    "  ckin.crs_recount(:point_id_spp); "
    "END;";
  ProcQry.DeclareVariable("point_id_spp", otInteger);

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (point_id_spp!=NoExists)
  {
    //этот код скорее всего будет работать исключительно для unbind_tlg
    Qry.SQLText=
      "SELECT :point_id_spp AS point_id_spp FROM dual";
    Qry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  }
  else
  {
    Qry.SQLText=
      "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id_tlg";
    Qry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  };
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    ProcQry.SetVariable("point_id_spp", Qry.FieldAsInteger("point_id_spp"));
    ProcQry.Execute();
    ProgTrace(TRACE5, "crs_recount: point_id_spp=%d, check_comp=%s", Qry.FieldAsInteger("point_id_spp"), check_comp?"true":"false");
    if (check_comp)
    {
  	  SALONS2::AutoSetCraft( Qry.FieldAsInteger("point_id_spp") );
  	};
  };
};

void TFltBinding::unbind_flt(int point_id, int point_id_spp)
{
  bool try_bind_again=false;
  unbind_flt_virt(point_id, point_id_spp, try_bind_again);
  if (try_bind_again) bind_flt(point_id);
};

void TTlgBinding::after_bind_or_unbind_flt(int point_id_tlg, int point_id_spp, bool unbind)
{
  crs_recount(point_id_tlg,point_id_spp,unbind?false:check_comp);
  for(int layer=0;layer<(int)cltTypeNum;layer++)
    if (IsTlgCompLayer((TCompLayerType)layer))
    {
      TPointIdsForCheck point_ids_spp;
      SyncTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
      check_layer_change(point_ids_spp);
    };
  if (!unbind)
    add_trip_task(point_id_spp, SYNC_ALL_CHKD, "");
  check_tlg_in_alarm(point_id_tlg, point_id_spp);
};

void TTlgBinding::unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again)
{
  int point_id_tlg=point_id;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "DELETE FROM tlg_binding WHERE point_id_tlg=:point_id_tlg AND point_id_spp=:point_id_spp";
  BindQry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  BindQry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  BindQry.Execute();
  if (BindQry.RowsProcessed()>0)
  {
    ProgTrace(TRACE5, "DELETE FROM tlg_binding WHERE point_id_tlg=%d AND point_id_spp=%d", point_id_tlg, point_id_spp);
    //действия по синхронизации
    after_bind_or_unbind_flt(point_id_tlg,point_id_spp,true);

    try_bind_again=true;
  };
};

void TTrferBinding::after_bind_or_unbind_flt(int point_id_trfer, int point_id_spp, bool unbind)
{
  if (check_alarm) check_unattached_trfer_alarm(point_id_spp);
};

void TTrferBinding::unbind_flt_virt(int point_id, int point_id_spp, bool try_bind_again)
{
  int point_id_trfer=point_id;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "UPDATE trfer_trips SET point_id_spp=NULL "
    "WHERE point_id=:point_id_trfer AND point_id_spp=:point_id_spp ";
  BindQry.CreateVariable("point_id_trfer",otInteger,point_id_trfer);
  BindQry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  BindQry.Execute();
  if (BindQry.RowsProcessed()>0)
  {
    ProgTrace(TRACE5, "UPDATE trfer_trips SET point_id_spp=NULL");
    ProgTrace(TRACE5, "WHERE point_id=%d AND point_id_spp=%d", point_id_trfer, point_id_spp);
    //действия по синхронизации
    after_bind_or_unbind_flt(point_id_trfer,point_id_spp,true);

    try_bind_again=true;
  };
};

string TTlgBinding::bind_flt_sql()
{
  return  "SELECT point_id, airline, flt_no, suffix, scd, pr_utc, "
          "       airp_dep, airp_arv, bind_type "
          "FROM tlg_trips "
          "WHERE point_id=:point_id";
};

string TTrferBinding::bind_flt_sql()
{
  return  "SELECT point_id, airline, flt_no, suffix, scd, 0 AS pr_utc, "
          "       airp_dep, NULL AS airp_arv, 0 AS bind_type "
          "FROM trfer_trips "
          "WHERE point_id=:point_id";
};

bool TFltBinding::bind_flt(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=bind_flt_sql().c_str();
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  return bind_flt(Qry);
};

bool TFltBinding::bind_flt(TQuery &Qry)
{
  return bind_flt_or_bind_check(Qry, NoExists);
};

bool TFltBinding::bind_check(TQuery &Qry, int point_id_spp)
{
  return bind_flt_or_bind_check(Qry, point_id_spp);
};

void TFltBinding::bind_flt(TFltInfo &flt, TBindType bind_type, vector<int> &spp_point_ids)
{
  spp_point_ids.clear();
  if (!flt.pr_utc && *flt.airp_dep==0) return;

  TSearchFltInfo filter;
  filter.airline=flt.airline;
  filter.flt_no=flt.flt_no;
  filter.suffix=flt.suffix;
  filter.airp_dep=flt.airp_dep;
  filter.scd_out=flt.scd;
  filter.scd_out_in_utc=flt.pr_utc;

  list<TAdvTripInfo> flts;
  SearchFlt(filter, flts);

  // Влад !!!
  // Пусть есть рейс с 4 пунктами ДМД-ВРН-МХЛ-ПЛК-СОЧ
  //
  // В Астру пришла след. тлг:
  //
  // LDM
  // UT002/21.DKE.12/116.2/5
  // -MCX.0/0/0.T0.PAX/0/0.PAD/0/0
  // -LED.0/0/0.T0.PAX/0/0.PAD/0/0
  // -AER.0/0/0.T0.PAX/0/0.PAD/0/0
  // SI: EXB0KG
  // SI: BNIL.CNIL.MNIL.DAANIL
  // PART 1 END
  //
  // В этом сл-е bind_flt заполнит вектор spp_point_ids 2-я одинаковыми point_id,
  // (которые соответствуют пункту ВРН).
  // Может имеет смысл выкидывать повторения?

  for(list<TAdvTripInfo>::const_iterator f=flts.begin(); f!=flts.end(); ++f)
  {
    switch (bind_type)
    {
      case btFirstSeg:
        spp_point_ids.push_back(f->point_id);
        break;
      case btLastSeg:
      case btAllSeg:
        {
          TTripRoute route;
          route.GetRouteAfter(NoExists,
                              f->point_id,
                              f->point_num,
                              f->first_point,
                              f->pr_tranzit,
                              trtWithCurrent,trtWithCancelled);
          TTripRoute::const_iterator last_point=route.end();
          for(TTripRoute::const_iterator r=route.begin();r!=route.end();r++)
          {
            if (r==route.begin()) continue; //пропускаем начальный пункт, ищем в последующих
            if (*flt.airp_arv==0)
            {
              //ищем последний пункт в маршруте
              last_point=r;
            }
            else
            {
              //ищем первый попавшийся в маршруте пункт airp_arv
              if (flt.airp_arv==r->airp)
              {
                last_point=r;
                break;
              };
            };

          };
          if (last_point==route.end())
          {
            //если ничего не нашли
            if (bind_type==btAllSeg)
            {
              spp_point_ids.push_back(f->point_id);
            };
          }
          else
          {
            //нашли last_point
            //пробежимся по пунктам посадки до last_point
            int point_id_spp=NoExists;
            for(TTripRoute::const_iterator r=route.begin();r!=last_point;r++)
            {
              if (bind_type==btAllSeg)
              {
                spp_point_ids.push_back(r->point_id);
              };
              if (bind_type==btLastSeg)
                point_id_spp=r->point_id;
            };
            if (point_id_spp!=NoExists)
            {
              //для btLastSeg
              spp_point_ids.push_back(point_id_spp);
            };
          };
          break;
        };
    };
  };
};

bool TFltBinding::bind_flt_or_bind_check(TQuery &Qry, int point_id_spp)
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
  TBindType bind_type=btFirstSeg;
  switch (Qry.FieldAsInteger("bind_type"))
  {
    case 0: bind_type=btFirstSeg;
            break;
    case 1: bind_type=btLastSeg;
            break;
   default: bind_type=btAllSeg;
  };

  vector<int> spp_point_ids;
  bind_flt(flt,bind_type,spp_point_ids);
  if (point_id_spp==NoExists)
  {
    //это привязка
    if (!spp_point_ids.empty())
    {
      bind_flt_virt(point_id, spp_point_ids);
      return true;
    };
  }
  else
  {
    //это проверка актуальности привязки point_id к point_id_spp
    if (find(spp_point_ids.begin(),spp_point_ids.end(),point_id_spp)!=spp_point_ids.end()) return true;
  };

  //не склалось привязать к рейсу на прямую - привяжем через таблицу CODESHARE_SETS
  bool res=false;
  if (!flt.pr_utc)
  {
    TQuery CodeShareQry(&OraSession);
    CodeShareQry.Clear();
    CodeShareQry.SQLText=
        "SELECT airline_oper,flt_no_oper FROM codeshare_sets "
        "WHERE airline_mark=:airline AND flt_no_mark=:flt_no AND "
        "      airp_dep=:airp_dep AND "
        "      first_date<=:scd_local AND "
        "      (last_date IS NULL OR last_date>:scd_local) AND "
        "      (days IS NULL OR INSTR(days,TO_CHAR(:wday))<>0) AND pr_del=0";
    CodeShareQry.CreateVariable("airline",otString,flt.airline);
    CodeShareQry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
    CodeShareQry.CreateVariable("airp_dep",otString,flt.airp_dep);
    CodeShareQry.CreateVariable("scd_local",otDate,flt.scd);
    CodeShareQry.CreateVariable("wday",otInteger,DayOfWeek(flt.scd));
    CodeShareQry.Execute();
    for(;!CodeShareQry.Eof;CodeShareQry.Next())
    {
      strcpy(flt.airline,CodeShareQry.FieldAsString("airline_oper"));
      flt.flt_no=CodeShareQry.FieldAsInteger("flt_no_oper");
      bind_flt(flt,bind_type,spp_point_ids);
      if (point_id_spp==NoExists)
      {
        //это привязка
        if (!spp_point_ids.empty())
        {
          bind_flt_virt(point_id, spp_point_ids);
          res=true;
        };
      }
      else
      {
        //это проверка актуальности привязки point_id к point_id_spp
        if (find(spp_point_ids.begin(),spp_point_ids.end(),point_id_spp)!=spp_point_ids.end()) return true;
      };
    };
  };
  return res;
};

void TTlgBinding::bind_flt_virt(int point_id, const vector<int> &spp_point_ids)
{
  if (spp_point_ids.empty()) return;
  int point_id_tlg=point_id;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "INSERT INTO tlg_binding(point_id_tlg,point_id_spp) VALUES(:point_id_tlg,:point_id_spp)";
  BindQry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  BindQry.DeclareVariable("point_id_spp",otInteger);
  for(vector<int>::const_iterator i=spp_point_ids.begin();i!=spp_point_ids.end();++i)
  {
    int point_id_spp=*i;
    BindQry.SetVariable("point_id_spp",point_id_spp);
    try
    {
      BindQry.Execute();
      if (BindQry.RowsProcessed()>0)
      {
        ProgTrace(TRACE5, "INSERT INTO tlg_binding(point_id_tlg, point_id_spp) VALUES(%d, %d)", point_id_tlg, point_id_spp);
        //действия по синхронизации
        after_bind_or_unbind_flt(point_id_tlg,point_id_spp,false);
      };
    }
    catch(EOracleError E)
    {
      if (E.Code!=1) throw;
    };
  };
};

void TTrferBinding::bind_flt_virt(int point_id, const vector<int> &spp_point_ids)
{
  if (spp_point_ids.empty()) return;
  int point_id_trfer=point_id;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "UPDATE trfer_trips SET point_id_spp=:point_id_spp "
    "WHERE point_id=:point_id_trfer AND (point_id_spp IS NULL OR point_id_spp<>:point_id_spp)";
  BindQry.CreateVariable("point_id_trfer",otInteger,point_id_trfer);
  int point_id_spp=*(spp_point_ids.begin());
  BindQry.CreateVariable("point_id_spp",otInteger,point_id_spp);
  BindQry.Execute();
  if (BindQry.RowsProcessed()>0)
  {
    ProgTrace(TRACE5, "UPDATE trfer_trips SET point_id_spp=%d", point_id_spp);
    ProgTrace(TRACE5, "WHERE point_id=%d AND (point_id_spp IS NULL OR point_id_spp<>%d)", point_id_trfer, point_id_spp);
    //действия по синхронизации
    after_bind_or_unbind_flt(point_id_trfer,point_id_spp,false);
  };
};

string TTlgBinding::bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc)
{
  ostringstream sql;
  sql << "SELECT point_id, airline, flt_no, suffix, scd, pr_utc, "
         "       airp_dep, airp_arv, bind_type ";

  if (unbind)
    sql << ", point_id_spp "
           "FROM tlg_trips, tlg_binding "
           "WHERE tlg_binding.point_id_tlg=tlg_trips.point_id AND ";
  else
    sql << "FROM tlg_trips "
           "WHERE ";
  if (use_scd_utc)
    sql << "      (scd=:scd_utc AND pr_utc=1 OR scd=:scd_local AND pr_utc=0) AND ";
  else
    sql << "      scd=:scd_local AND pr_utc=0 AND ";

  sql << "      airline=:airline AND flt_no=:flt_no AND "
         "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix)";
  return sql.str();
};

string TTrferBinding::bind_or_unbind_flt_sql(bool unbind, bool use_scd_utc)
{
  ostringstream sql;
  sql << "SELECT point_id, airline, flt_no, suffix, scd, 0 AS pr_utc, "
         "       airp_dep, NULL AS airp_arv, 0 AS bind_type ";
  if (unbind)
    sql << ", point_id_spp ";
  sql << "FROM trfer_trips "
         "WHERE scd=:scd_local AND ";
  if (use_scd_utc)
    sql << "      :scd_utc IS NOT NULL AND ";
  sql << "      airline=:airline AND flt_no=:flt_no AND "
         "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix)";
  return sql.str();
};

void TFltBinding::bind_or_unbind_flt(const vector<TTripInfo> &flts, bool unbind, bool use_scd_utc)
{
  if (flts.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=bind_or_unbind_flt_sql(unbind, use_scd_utc).c_str();
  if (use_scd_utc)
    Qry.DeclareVariable("scd_utc", otDate);
  Qry.DeclareVariable("scd_local", otDate);
  Qry.DeclareVariable("airline", otString);
  Qry.DeclareVariable("flt_no", otInteger);
  Qry.DeclareVariable("suffix", otString);
  for(vector<TTripInfo>::const_iterator flt=flts.begin(); flt!=flts.end(); flt++)
  {
    if (use_scd_utc)
    {
      if (flt->scd_out!=NoExists)
      {
        TDateTime scd_utc=flt->scd_out;
        modf(scd_utc,&scd_utc);
        TDateTime scd_local=UTCToLocal(flt->scd_out,AirpTZRegion(flt->airp));
        modf(scd_local,&scd_local);
        Qry.SetVariable("scd_utc", scd_utc);
        Qry.SetVariable("scd_local", scd_local);
      }
      else
      {
        Qry.SetVariable("scd_utc", FNull);
        Qry.SetVariable("scd_local", FNull);
      };
    }
    else
    {
      if (flt->scd_out!=NoExists)
      {
        TDateTime scd_local=flt->scd_out;
        modf(scd_local,&scd_local);
        Qry.SetVariable("scd_local", scd_local);
      }
      else
        Qry.SetVariable("scd_local", FNull);
    };

    Qry.SetVariable("airline", flt->airline);
    Qry.SetVariable("flt_no", flt->flt_no);
    Qry.SetVariable("suffix", flt->suffix);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      if (unbind)
      {
        int point_id_spp=Qry.FieldAsInteger("point_id_spp");
        if (!bind_check(Qry, point_id_spp)) unbind_flt(Qry.FieldAsInteger("point_id"), point_id_spp);
      }
      else
      {
        bind_flt(Qry);
      };
    };
  };
};

void TFltBinding::bind_flt(const vector<TTripInfo> &flts, bool use_scd_utc)
{
  bind_or_unbind_flt(flts, false, use_scd_utc);
};

void TFltBinding::unbind_flt(const vector<TTripInfo> &flts, bool use_scd_utc)
{
  bind_or_unbind_flt(flts, true, use_scd_utc);
};

void TFltBinding::bind_or_unbind_flt_oper(const vector<TTripInfo> &operFlts, bool unbind)
{
  if (operFlts.empty()) return;

  vector<TTripInfo> flts;
  //создаем вектор, включающий помимо оперирующих еще и маркетинговые рейсы
  for(vector<TTripInfo>::const_iterator flt=operFlts.begin(); flt!=operFlts.end(); flt++)
  {
    flts.push_back(*flt);

    vector<TTripInfo> markFlts;
    GetMktFlights(*flt, markFlts, true);
    flts.insert(flts.end(), markFlts.begin(), markFlts.end());
  };

  bind_or_unbind_flt(flts, unbind, true);
};

void TFltBinding::bind_flt_oper(const vector<TTripInfo> &operFlts)
{
  trace_for_bind(operFlts, "bind_flt_oper");

  bind_or_unbind_flt_oper(operFlts, false);
};

void TFltBinding::unbind_flt_oper(const vector<TTripInfo> &operFlts)
{
  bind_or_unbind_flt_oper(operFlts, true);
};

string TTlgBinding::unbind_flt_sql()
{
  return "SELECT point_id, airline, flt_no, suffix, scd, pr_utc, "
         "       airp_dep, airp_arv, bind_type "
         "FROM tlg_trips, tlg_binding "
         "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg AND tlg_binding.point_id_spp=:point_id_spp";
};

string TTrferBinding::unbind_flt_sql()
{
  return "SELECT point_id, airline, flt_no, suffix, scd, 0 AS pr_utc, "
         "       airp_dep, NULL AS airp_arv, 0 AS bind_type "
         "FROM trfer_trips "
         "WHERE point_id_spp=:point_id_spp";
};

void TFltBinding::unbind_flt(const vector<int> &spp_point_ids)
{
  trace_for_bind(spp_point_ids, "unbind_flt");

  if (spp_point_ids.empty()) return;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=unbind_flt_sql().c_str();
  BindQry.DeclareVariable("point_id_spp",otInteger);
  for(vector<int>::const_iterator i=spp_point_ids.begin();i!=spp_point_ids.end();i++)
  {
    int point_id_spp=*i;
    BindQry.SetVariable("point_id_spp",point_id_spp);
    BindQry.Execute();
    for(;!BindQry.Eof;BindQry.Next())
      if (!bind_check(BindQry, point_id_spp)) unbind_flt(BindQry.FieldAsInteger("point_id"), point_id_spp);
  };
};

void TFltBinding::trace_for_bind(const vector<TTripInfo> &flts, const string &where)
{
  ostringstream trace;
  trace << where << ":";
  for(vector<TTripInfo>::const_iterator flt=flts.begin();flt!=flts.end();flt++)
    trace << " "
          << flt->airline << setw(3) << setfill('0') << flt->flt_no << flt->suffix
          << " " << flt->airp
          << "/" << (flt->scd_out==NoExists?"??.??":DateTimeToStr(flt->scd_out, "dd.mm"))
          << ";";
  ProgTrace(TRACE5, trace.str().c_str());
};

void TFltBinding::trace_for_bind(const vector<int> &point_ids, const string &where)
{
  ostringstream trace;
  trace << where << ":";
  for(vector<int>::const_iterator id=point_ids.begin();id!=point_ids.end();id++)
    trace << " " << *id << ";";

  ProgTrace(TRACE5, trace.str().c_str());
};

