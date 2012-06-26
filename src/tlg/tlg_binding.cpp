#include "tlg_binding.h"
#include "oralib.h"
#include "salons.h"
#include "comp_layers.h"
#include "alarms.h"

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

void unbind_tlg(int point_id_tlg, int point_id_spp)
{
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
    crs_recount(point_id_tlg,point_id_spp,false);
    for(int layer=0;layer<(int)cltTypeNum;layer++)
      if (IsTlgCompLayer((TCompLayerType)layer))
      {
        TPointIdsForCheck point_ids_spp;
        SyncTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
        check_alarms(point_ids_spp);
      };
    //попробовать опять привязать point_id_tlg к какому либо рейсу
    bind_tlg(point_id_tlg, true);
  };
};

void bind_tlg(int point_id_tlg, const vector<int> &spp_point_ids, bool check_comp)
{
  if (spp_point_ids.empty()) return;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "INSERT INTO tlg_binding(point_id_tlg,point_id_spp) VALUES(:point_id_tlg,:point_id_spp)";
  BindQry.CreateVariable("point_id_tlg",otInteger,point_id_tlg);
  BindQry.DeclareVariable("point_id_spp",otInteger);
  for(vector<int>::const_iterator i=spp_point_ids.begin();i!=spp_point_ids.end();i++)
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
        crs_recount(point_id_tlg,point_id_spp,check_comp);
        for(int layer=0;layer<(int)cltTypeNum;layer++)
          if (IsTlgCompLayer((TCompLayerType)layer))
          {
            TPointIdsForCheck point_ids_spp;
            SyncTripCompLayers(point_id_tlg, point_id_spp, (TCompLayerType)layer, point_ids_spp);
            check_alarms(point_ids_spp);
          };
      };
    }
    catch(EOracleError E)
    {
      if (E.Code!=1) throw;
    };
  };
};

void bind_tlg(int point_id_tlg, TFltInfo &flt, TBindType bind_type, vector<int> &spp_point_ids)
{
  spp_point_ids.clear();
  if (!flt.pr_utc && *flt.airp_dep==0) return;
  TQuery PointsQry(&OraSession);
  PointsQry.CreateVariable("airline",otString,flt.airline);
  PointsQry.CreateVariable("flt_no",otInteger,(int)flt.flt_no);
  PointsQry.CreateVariable("suffix",otString,flt.suffix);
  PointsQry.CreateVariable("airp_dep",otString,flt.airp_dep);
  PointsQry.CreateVariable("scd",otDate,flt.scd);

  if (!flt.pr_utc)
  {
    //перевод UTCToLocal(points.scd)
    PointsQry.SQLText=
      "SELECT point_id,point_num,first_point,pr_tranzit, "
      "       scd_out AS scd,airp "
      "FROM points "
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND "
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
      "      scd_out >= TO_DATE(:scd)-1 AND scd_out < TO_DATE(:scd)+2 AND "
      "      pr_del>=0 ";
  }
  else
  {
    PointsQry.SQLText=
      "SELECT point_id,point_num,first_point,pr_tranzit "
      "FROM points "
      "WHERE airline=:airline AND flt_no=:flt_no AND "
      "      (:airp_dep IS NULL OR airp=:airp_dep) AND "
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
      "      scd_out >= TO_DATE(:scd) AND scd_out < TO_DATE(:scd)+1 AND pr_del>=0 ";
  };
  PointsQry.Execute();
  TDateTime scd;
  string tz_region;
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    if (!flt.pr_utc)
    {
      scd=PointsQry.FieldAsDateTime("scd");
      tz_region=AirpTZRegion(PointsQry.FieldAsString("airp"),false);
      if (tz_region.empty()) continue;
      scd=UTCToLocal(scd,tz_region);
      modf(scd,&scd);
      if (scd!=flt.scd) continue;
    };
    switch (bind_type)
    {
      case btFirstSeg:
        spp_point_ids.push_back(PointsQry.FieldAsInteger("point_id"));
        break;
      case btLastSeg:
      case btAllSeg:
        {
          TTripRoute route;
          route.GetRouteAfter(NoExists,
                              PointsQry.FieldAsInteger("point_id"),
                              PointsQry.FieldAsInteger("point_num"),
                              PointsQry.FieldIsNULL("first_point")?NoExists:PointsQry.FieldAsInteger("first_point"),
                              PointsQry.FieldAsInteger("pr_tranzit")!=0,
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
              spp_point_ids.push_back(PointsQry.FieldAsInteger("point_id"));
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

bool bind_tlg_or_bind_check(TQuery &Qry, int point_id_spp, bool check_comp)
{
  if (Qry.Eof) return false;
  int point_id_tlg=Qry.FieldAsInteger("point_id");
  TFltInfo flt;
  strcpy(flt.airline,Qry.FieldAsString("airline"));
  flt.flt_no=Qry.FieldAsInteger("flt_no");
  strcpy(flt.suffix,Qry.FieldAsString("suffix"));
  flt.scd=Qry.FieldAsDateTime("scd");
  modf(flt.scd,&flt.scd);
  flt.pr_utc=Qry.FieldAsInteger("pr_utc")!=0;
  strcpy(flt.airp_dep,Qry.FieldAsString("airp_dep"));
  strcpy(flt.airp_arv,Qry.FieldAsString("airp_arv"));
  TBindType bind_type;
  switch (Qry.FieldAsInteger("bind_type"))
  {
    case 0: bind_type=btFirstSeg;
            break;
    case 1: bind_type=btLastSeg;
            break;
   default: bind_type=btAllSeg;
  };
  vector<int> spp_point_ids;
  bind_tlg(point_id_tlg,flt,bind_type,spp_point_ids);
  if (point_id_spp==NoExists)
  {
    //это привязка
    if (!spp_point_ids.empty())
    {
      bind_tlg(point_id_tlg,spp_point_ids,check_comp);
      return true;
    };
  }
  else
  {
    //это проверка актуальности привязки point_id_tlg к point_id_spp
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
      bind_tlg(point_id_tlg,flt,bind_type,spp_point_ids);
      if (point_id_spp==NoExists)
      {
        //это привязка
        if (!spp_point_ids.empty())
        {
          bind_tlg(point_id_tlg,spp_point_ids,check_comp);
          res=true;
        };
      }
      else
      {
        //это проверка актуальности привязки point_id_tlg к point_id_spp
        if (find(spp_point_ids.begin(),spp_point_ids.end(),point_id_spp)!=spp_point_ids.end()) return true;
      };
    };
  };
  return res;
};

bool bind_tlg(TQuery &Qry, bool check_comp)
{
  return bind_tlg_or_bind_check(Qry, NoExists, check_comp);
};

bool bind_check(TQuery &Qry, int point_id_spp)
{
  return bind_tlg_or_bind_check(Qry, point_id_spp, false);
};

bool bind_tlg(int point_id_tlg, bool check_comp)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type "
    "FROM tlg_trips "
    "WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id_tlg);
  Qry.Execute();
  return bind_tlg(Qry, check_comp);
};

void bind_or_unbind_tlg(const vector<TTripInfo> &flts, bool unbind, bool use_scd_utc, bool check_comp)
{
  if (flts.empty()) return;

  ostringstream sql;
  sql << "SELECT point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type ";

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

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=sql.str().c_str();
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
      TDateTime scd_utc=flt->scd_out;
      modf(scd_utc,&scd_utc);
      TDateTime scd_local=UTCToLocal(flt->scd_out,AirpTZRegion(flt->airp));
      modf(scd_local,&scd_local);
      Qry.SetVariable("scd_utc", scd_utc);
      Qry.SetVariable("scd_local", scd_local);
    }
    else
    {
      TDateTime scd_local=flt->scd_out;
      modf(scd_local,&scd_local);
      Qry.SetVariable("scd_local", scd_local);
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
        if (!bind_check(Qry, point_id_spp)) unbind_tlg(Qry.FieldAsInteger("point_id"), point_id_spp);
      }
      else
      {
        bind_tlg(Qry, check_comp);
      };
    };
  };
};

void bind_tlg(const vector<TTripInfo> &flts, bool use_scd_utc, bool check_comp)
{
  bind_or_unbind_tlg(flts, false, use_scd_utc, check_comp);
};

void unbind_tlg(const vector<TTripInfo> &flts, bool use_scd_utc)
{
  bind_or_unbind_tlg(flts, true, use_scd_utc, false);
};

void bind_or_unbind_tlg_oper(const vector<TTripInfo> &operFlts, bool unbind, bool check_comp)
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

  bind_or_unbind_tlg(flts, unbind, true, check_comp);
};

void bind_tlg_oper(const vector<TTripInfo> &operFlts, bool check_comp)
{
  trace_for_bind(operFlts, "bind_tlg_oper");

  bind_or_unbind_tlg_oper(operFlts, false, check_comp);
};

void unbind_tlg_oper(const vector<TTripInfo> &operFlts)
{
  bind_or_unbind_tlg_oper(operFlts, true, false);
};

void unbind_tlg(const vector<int> &spp_point_ids)
{
  trace_for_bind(spp_point_ids, "unbind_tlg");

  if (spp_point_ids.empty()) return;
  TQuery BindQry(&OraSession);
  BindQry.SQLText=
    "SELECT point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type "
    "FROM tlg_trips, tlg_binding "
    "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg AND tlg_binding.point_id_spp=:point_id_spp";
  BindQry.DeclareVariable("point_id_spp",otInteger);
  for(vector<int>::const_iterator i=spp_point_ids.begin();i!=spp_point_ids.end();i++)
  {
    int point_id_spp=*i;
    BindQry.SetVariable("point_id_spp",point_id_spp);
    BindQry.Execute();
    for(;!BindQry.Eof;BindQry.Next())
      if (!bind_check(BindQry, point_id_spp)) unbind_tlg(BindQry.FieldAsInteger("point_id"), point_id_spp);
  };
};

void trace_for_bind(const vector<TTripInfo> &flts, const string &where)
{
  ostringstream trace;
  trace << where << ":";
  for(vector<TTripInfo>::const_iterator flt=flts.begin();flt!=flts.end();flt++)
    trace << " "
          << flt->airline << setw(3) << setfill('0') << flt->flt_no << flt->suffix
          << " " << flt->airp
          << "/" << DateTimeToStr(flt->scd_out, "dd.mm")
          << ";";
  ProgTrace(TRACE5, trace.str().c_str());
};

void trace_for_bind(const vector<int> &point_ids, const string &where)
{
  ostringstream trace;
  trace << where << ":";
  for(vector<int>::const_iterator id=point_ids.begin();id!=point_ids.end();id++)
    trace << " " << *id << ";";
  
  ProgTrace(TRACE5, trace.str().c_str());
};



