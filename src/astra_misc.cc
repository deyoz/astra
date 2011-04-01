#include "astra_misc.h"
#include <string>
#include <vector>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg/tlg_parser.h"
#include "convert.h"
#include "astra_locale.h"

#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace ASTRA;

void TTripInfo::get_client_dates(TDateTime &scd_out_client, TDateTime &real_out_client) const
{
  scd_out_client=ASTRA::NoExists;
  real_out_client=ASTRA::NoExists;

  if (airp.empty() || scd_out==ASTRA::NoExists) return;

  string &tz_region=AirpTZRegion(airp);
  modf(UTCToClient(scd_out,tz_region),&scd_out_client);
  if (real_out!=ASTRA::NoExists)
    modf(UTCToClient(real_out,tz_region),&real_out_client);
  else
    real_out_client=scd_out_client;
};

//для сохранения совместимости вводим AstraLocale::TLocaleType
string GetTripName( const TTripInfo &info, TElemContext ctxt, bool showAirp, bool prList )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TDateTime scd_out_client, real_out_client, desk_time;
  modf(reqInfo->desk.time,&desk_time);

  info.get_client_dates(scd_out_client, real_out_client);

  ostringstream trip;
  if ( ctxt == ecNone )
    trip << info.airline
         << setw(3) << setfill('0') << info.flt_no
         << info.suffix;
  else
    trip << ElemIdToElemCtxt(ctxt, etAirline, info.airline, info.airline_fmt)
         << setw(3) << setfill('0') << info.flt_no
         << ElemIdToElemCtxt(ctxt, etSuffix, info.suffix, info.suffix_fmt);

  if (prList)
  {
    if (info.flt_no<10000) trip << " ";
    if (info.flt_no<1000)  trip << " ";
  };

  if (desk_time!=real_out_client)
  {
    if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(real_out_client,"mm"))
      trip << "/" << DateTimeToStr(real_out_client,"dd");
    else
      trip << "/" << DateTimeToStr(real_out_client,"dd.mm");
  };
  if (scd_out_client!=real_out_client)
    trip << "(" << DateTimeToStr(scd_out_client,"dd") << ")";
  if (!(reqInfo->user.user_type==utAirport &&
        reqInfo->user.access.airps_permit &&
        reqInfo->user.access.airps.size()==1)||showAirp) {
   if ( ctxt == ecNone)
     trip << " " << info.airp;
   else
   	 trip << " " << ElemIdToElemCtxt(ctxt, etAirp, info.airp, info.airp_fmt);
  }
  if(info.pr_del != ASTRA::NoExists and info.pr_del != 0) {
      trip << " " << (info.pr_del < 0 ? string("(")+AstraLocale::getLocaleText("удл.")+")" : string("(")+AstraLocale::getLocaleText("отм.")+")");
  }

  return trip.str();
};

string TLastTrferInfo::str()
{
  ostringstream trip;
  if (IsNULL()) return "";
  trip << ElemIdToCodeNative(etAirp, airp_arv)
       << '('
       << ElemIdToCodeNative(etAirline, airline)
       << setw(3) << setfill('0') << flt_no
       << ElemIdToCodeNative(etSuffix, suffix)
       << ')';
  return trip.str();
};

bool GetTripSets( const TTripSetType setType, const TTripInfo &info )
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  if (setType==tsPaidCheckIn)
  {
    Qry.SQLText=
      "SELECT pr_permit AS pr_misc, "
      "    DECODE(airline,NULL,0,8)+ "
      "    DECODE(flt_no,NULL,0,2)+ "
      "    DECODE(airp_dep,NULL,0,4) AS priority "
      "FROM paid_ckin_sets "
      "WHERE airline=:airline AND "
      "      (flt_no IS NULL OR flt_no=:flt_no) AND "
      "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
      "ORDER BY priority DESC";
  }
  else
  {
    Qry.SQLText=
      "SELECT pr_misc, "
      "    DECODE(airline,NULL,0,8)+ "
      "    DECODE(flt_no,NULL,0,2)+ "
      "    DECODE(airp_dep,NULL,0,4) AS priority "
      "FROM misc_set "
      "WHERE type=:type AND "
      "      (airline IS NULL OR airline=:airline) AND "
      "      (flt_no IS NULL OR flt_no=:flt_no) AND "
      "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
      "ORDER BY priority DESC";
    Qry.CreateVariable("type",otInteger,(int)setType);
  };
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  if (Qry.Eof)
  {
    switch(setType)
    {
      //запрет интерактива с СЭБом
      case tsETLOnly: return false;
              default: return false;
    };
  };
  return Qry.FieldAsInteger("pr_misc")!=0;
};

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs)
{
    string airline;
    return GetPnrAddr(pnr_id, pnrs, airline);
}

string GetPnrAddr(int pnr_id, vector<TPnrAddrItem> &pnrs, string &airline)
{
  pnrs.clear();
  TQuery Qry(&OraSession);
  if (airline.empty())
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT airline "
      "FROM crs_pnr,tlg_trips "
      "WHERE crs_pnr.point_id=tlg_trips.point_id AND pnr_id=:pnr_id"; //pnr_market_flt
    Qry.CreateVariable("pnr_id",otInteger,pnr_id);
    Qry.Execute();
    if (!Qry.Eof) airline=Qry.FieldAsString("airline");
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,addr FROM pnr_addrs "
    "WHERE pnr_id=:pnr_id ORDER BY DECODE(airline,:airline,0,1),airline";
  Qry.CreateVariable("pnr_id",otInteger,pnr_id);
  Qry.CreateVariable("airline",otString,airline);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TPnrAddrItem pnr;
    strcpy(pnr.airline,Qry.FieldAsString("airline"));
    strcpy(pnr.addr,Qry.FieldAsString("addr"));
    pnrs.push_back(pnr);
  };
  if (!pnrs.empty() && pnrs[0].airline==airline)
    return pnrs[0].addr;
  else
    return "";
};

string GetPaxPnrAddr(int pax_id, vector<TPnrAddrItem> &pnrs)
{
    string airline;
    return GetPaxPnrAddr(pax_id, pnrs, airline);
}

string GetPaxPnrAddr(int pax_id, vector<TPnrAddrItem> &pnrs, string &airline)
{
  pnrs.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id AND pr_del=0";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.Execute();
  if (!Qry.Eof)
    return GetPnrAddr(Qry.FieldAsInteger("pnr_id"),pnrs,airline);
  else
    return "";
};

TDateTime DayToDate(int day, TDateTime base_date, bool back)
{
  if (day<1 || day>31) throw EConvertError("DayToDate: wrong day");
  modf(base_date,&base_date);
  int iDay,iMonth,iYear;
  DecodeDate(base_date,iYear,iMonth,iDay);
  TDateTime result;
  if (!back)
  {
    result=base_date-1.0;
    do
    {
      try
      {
        EncodeDate(iYear,iMonth,day,result);
      }
      catch(EConvertError) {};
      if (iMonth==12)
      {
        iMonth=1;
        iYear++;
      }
      else iMonth++;
    }
    while(result<base_date);
  }
  else
  {
    result=base_date+1.0;
    do
    {
      try
      {
        EncodeDate(iYear,iMonth,day,result);
      }
      catch(EConvertError) {};
      if (iMonth==1)
      {
        iMonth=12;
        iYear--;
      }
      else iMonth--;
    }
    while(result>base_date);
  };
  return result;
};

void TTripRoute::GetRoute(int point_id,
                          int point_num,
                          int first_point,
                          bool pr_tranzit,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2,
                          TQuery& Qry)
{
  ostringstream sql;
  sql << "SELECT point_id,point_num,airp,pr_del "
         "FROM points ";
  if (after_current)
  {
    if (route_type1==trtWithCurrent)
      sql << "WHERE :first_point IN (first_point,point_id)"
          << "  AND point_num>=:point_num ";
    else
      sql << "WHERE first_point=:first_point"
             "  AND point_num>:point_num ";
  }
  else
  {
    sql << "WHERE :first_point IN (first_point,point_id) ";
    if (route_type1==trtWithCurrent)
      sql << "AND point_num<=:point_num ";
    else
      sql << "AND point_num<:point_num ";
  };

  if (route_type2==trtWithCancelled)
    sql << "AND pr_del>=0 ";
  else
    sql << "AND pr_del=0 ";

  sql << "ORDER BY point_num";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  if (!pr_tranzit && after_current)
    Qry.CreateVariable("first_point",otInteger,point_id);
  else
    Qry.CreateVariable("first_point",otInteger,first_point);
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTripRouteItem item;
    item.point_id=Qry.FieldAsInteger("point_id");
    item.point_num=Qry.FieldAsInteger("point_num");
    item.airp=Qry.FieldAsString("airp");
    item.pr_cancel=Qry.FieldAsInteger("pr_del")!=0;
    push_back(item);
  };
};

bool TTripRoute::GetRoute(int point_id,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_num,first_point,pr_tranzit "
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  GetRoute(point_id,
           Qry.FieldAsInteger("point_num"),
           Qry.FieldAsInteger("first_point"),
           Qry.FieldAsInteger("pr_tranzit")!=0,
           after_current,route_type1,route_type2,Qry);
  return true;
};

bool TTripRoute::GetRouteAfter(int point_id,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  return GetRoute(point_id,true,route_type1,route_type2);
};

bool TTripRoute::GetRouteBefore(int point_id,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  return GetRoute(point_id,false,route_type1,route_type2);
};

void TTripRoute::GetRouteAfter(int point_id,
                               int point_num,
                               int first_point,
                               bool pr_tranzit,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(point_id,point_num,first_point,pr_tranzit,
           true,route_type1,route_type2,Qry);
};

void TTripRoute::GetRouteBefore(int point_id,
                                int point_num,
                                int first_point,
                                bool pr_tranzit,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(point_id,point_num,first_point,pr_tranzit,
           false,route_type1,route_type2,Qry);
};

void TTripRoute::GetNextAirp(int point_id,
                             int point_num,
                             int first_point,
                             bool pr_tranzit,
                             TTripRouteType2 route_type2,
                             TTripRouteItem& item)
{
  item.Clear();
  clear();
  TQuery Qry(&OraSession);
  GetRoute(point_id,point_num,first_point,pr_tranzit,
           true,trtNotCurrent,route_type2,Qry);
  if (begin()!=end())
  {
    //а лучше бы перегрузить оператор присваивания
    item.point_id=begin()->point_id;
    item.point_num=begin()->point_num;
    item.airp=begin()->airp;
    item.pr_cancel=begin()->pr_cancel;
  };
};

bool TTripRoute::GetNextAirp(int point_id,
                             TTripRouteType2 route_type2,
                             TTripRouteItem& item)
{
  item.Clear();
  if (!GetRoute(point_id,true,trtNotCurrent,route_type2)) return false;
  if (begin()!=end())
  {
    //а лучше бы перегрузить оператор присваивания
    item.point_id=begin()->point_id;
    item.point_num=begin()->point_num;
    item.airp=begin()->airp;
    item.pr_cancel=begin()->pr_cancel;
  };
  return true;
};

void TMktFlight::dump()
{
    ProgTrace(TRACE5, "---TMktFlight::dump()---");
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %d", flt_no);
    ProgTrace(TRACE5, "suffix: %s", suffix.c_str());
    ProgTrace(TRACE5, "subcls: %s", subcls.c_str());
    ProgTrace(TRACE5, "scd_day_local: %d", scd_day_local);
    ProgTrace(TRACE5, "scd_date_local: %s", DateTimeToStr(scd_date_local).c_str());
    ProgTrace(TRACE5, "airp_dep: %s", airp_dep.c_str());
    ProgTrace(TRACE5, "airp_arv: %s", airp_arv.c_str());
    ProgTrace(TRACE5, "---END OF TMktFlight::dump()---");

}

void TMktFlight::clear()
{
  airline.clear();
  flt_no = NoExists;
  suffix.clear();
  subcls.clear();
  scd_day_local = NoExists;
  scd_date_local = NoExists;
  airp_dep.clear();
  airp_arv.clear();
};

bool TMktFlight::IsNULL()
{
    return
        airline.empty() or
        flt_no == NoExists or
        subcls.empty() or
        scd_day_local == NoExists or
        scd_date_local == NoExists or
        airp_dep.empty() or
        airp_arv.empty();
}

void TMktFlight::get(TQuery &Qry, int id)
{
    clear();
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    if(!Qry.Eof)
    {

        if(Qry.FieldIsNULL("pax_airline")) {
            airline = Qry.FieldAsString("tlg_airline");
            flt_no = Qry.FieldAsInteger("tlg_flt_no");
            suffix = Qry.FieldAsString("tlg_suffix");
            subcls = Qry.FieldAsString("tlg_subcls");
            TDateTime tmp_scd = Qry.FieldAsDateTime("tlg_scd");
            int Year, Month, Day;
            DecodeDate(tmp_scd, Year, Month, Day);
            scd_day_local = Day;
            EncodeDate(Year, Month, Day, scd_date_local);
            airp_dep = Qry.FieldAsString("tlg_airp_dep");
            airp_arv = Qry.FieldAsString("tlg_airp_arv");
        } else {
            airline = Qry.FieldAsString("pax_airline");
            flt_no = Qry.FieldAsInteger("pax_flt_no");
            suffix = Qry.FieldAsString("pax_suffix");
            subcls = Qry.FieldAsString("pax_subcls");
            scd_day_local = Qry.FieldAsInteger("pax_scd");
            scd_date_local = DayToDate(scd_day_local,Qry.FieldAsDateTime("tlg_scd"),true);
            airp_dep = Qry.FieldAsString("pax_airp_dep");
            airp_arv = Qry.FieldAsString("pax_airp_arv");
        }
    }
  //  dump();
};

void TMktFlight::getByPaxId(int pax_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    mark_trips.airline mark_airline, "
        "    mark_trips.flt_no mark_flt_no, "
        "    mark_trips.suffix mark_suffix, "
        "    NVL(pax.subclass,pax_grp.class) mark_subcls, "
        "    mark_trips.scd mark_scd, "
        "    mark_trips.airp_dep mark_airp_dep, "
        "    pax_grp.airp_arv mark_airp_arv "
        "from "
        "   pax, "
        "   pax_grp, "
        "   mark_trips "
        "where "
        "    pax.pax_id = :id and "
        "    pax.grp_id = pax_grp.grp_id and "
        "    pax_grp.point_id_mark = mark_trips.point_id ";
    clear();
    Qry.CreateVariable("id",otInteger,pax_id);
    Qry.Execute();
    if(!Qry.Eof)
    {
        airline = Qry.FieldAsString("mark_airline");
        flt_no = Qry.FieldAsInteger("mark_flt_no");
        suffix = Qry.FieldAsString("mark_suffix");
        subcls = Qry.FieldAsString("mark_subcls");
        TDateTime tmp_scd = Qry.FieldAsDateTime("mark_scd");
        int Year, Month, Day;
        DecodeDate(tmp_scd, Year, Month, Day);
        scd_day_local = Day;
        EncodeDate(Year, Month, Day, scd_date_local);
        airp_dep = Qry.FieldAsString("mark_airp_dep");
        airp_arv = Qry.FieldAsString("mark_airp_arv");
    }
}

void TMktFlight::getByCrsPaxId(int pax_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    tlg_trips.airline tlg_airline, "
        "    tlg_trips.flt_no tlg_flt_no, "
        "    tlg_trips.suffix tlg_suffix, "
        "    crs_pnr.subclass tlg_subcls, "
        "    tlg_trips.scd tlg_scd, "
        "    tlg_trips.airp_dep tlg_airp_dep, "
        "    crs_pnr.target tlg_airp_arv, "
        "    pnr_market_flt.airline pax_airline, "
        "    pnr_market_flt.flt_no pax_flt_no, "
        "    pnr_market_flt.suffix pax_suffix, "
        "    pnr_market_flt.subclass pax_subcls, "
        "    pnr_market_flt.local_date pax_scd, "
        "    pnr_market_flt.airp_dep pax_airp_dep, "
        "    pnr_market_flt.airp_arv pax_airp_arv "
        "from "
        "   crs_pax, "
        "   crs_pnr, "
        "   tlg_trips, "
        "   pnr_market_flt "
        "where "
        "    crs_pax.pax_id = :id and crs_pax.pr_del=0 and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id and "
        "    crs_pnr.point_id = tlg_trips.point_id and "
        "    crs_pax.pnr_id = pnr_market_flt.pnr_id(+) ";
    get(Qry, pax_id);
}

void TMktFlight::getByPnrId(int pnr_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    tlg_trips.airline tlg_airline, "
        "    tlg_trips.flt_no tlg_flt_no, "
        "    tlg_trips.suffix tlg_suffix, "
        "    crs_pnr.subclass tlg_subcls, "
        "    tlg_trips.scd tlg_scd, "
        "    tlg_trips.airp_dep tlg_airp_dep, "
        "    crs_pnr.target tlg_airp_arv, "
        "    pnr_market_flt.airline pax_airline, "
        "    pnr_market_flt.flt_no pax_flt_no, "
        "    pnr_market_flt.suffix pax_suffix, "
        "    pnr_market_flt.subclass pax_subcls, "
        "    pnr_market_flt.local_date pax_scd, "
        "    pnr_market_flt.airp_dep pax_airp_dep, "
        "    pnr_market_flt.airp_arv pax_airp_arv "
        "from "
        "   crs_pnr, "
        "   tlg_trips, "
        "   pnr_market_flt "
        "where "
        "    crs_pnr.pnr_id = :id and "
        "    crs_pnr.point_id = tlg_trips.point_id and "
        "    crs_pnr.pnr_id = pnr_market_flt.pnr_id(+) ";
    get(Qry, pnr_id);
}

bool SeparateTCkin(int grp_id,
                   TCkinSegmentSet upd_depend,
                   TCkinSegmentSet upd_tid,
                   int tid,
                   int &tckin_id, int &seg_no)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT tckin_id,seg_no FROM tckin_pax_grp WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return false;

  tckin_id=Qry.FieldAsInteger("tckin_id");
  seg_no=Qry.FieldAsInteger("seg_no");

  if (upd_depend==cssNone) return true;

  ostringstream sql;
  string where_str;

  switch (upd_tid)
  {
    case cssAllPrev:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<:seg_no";
      break;
    case cssAllPrevCurr:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<=:seg_no";
      break;
    case cssAllPrevCurrNext:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<=:seg_no+1";
      break;
    case cssCurr:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no=:seg_no";
      break;
    default:
      where_str="";
  };
  if (!where_str.empty())
  {
    sql.str("");
    sql << "UPDATE pax_grp SET tid=:tid "
        << "WHERE grp_id IN (SELECT grp_id FROM tckin_pax_grp " << where_str << ") ";

    Qry.Clear();
    Qry.SQLText=sql.str().c_str();
    Qry.CreateVariable("tckin_id",otInteger,tckin_id);
    Qry.CreateVariable("seg_no",otInteger,seg_no);
    Qry.CreateVariable("tid",otInteger,tid);
    Qry.Execute();
  };

  switch (upd_depend)
  {
    case cssAllPrev:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<:seg_no";
      break;
    case cssAllPrevCurr:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<=:seg_no";
      break;
    case cssAllPrevCurrNext:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no<=:seg_no+1";
      break;
    case cssCurr:
      where_str=" WHERE tckin_id=:tckin_id AND seg_no=:seg_no";
      break;
    default:
      where_str="";
  };
  if (!where_str.empty())
  {
    sql.str("");
    sql << "UPDATE tckin_pax_grp SET pr_depend=0 " << where_str;

    Qry.Clear();
    Qry.SQLText=sql.str().c_str();
    Qry.CreateVariable("tckin_id",otInteger,tckin_id);
    Qry.CreateVariable("seg_no",otInteger,seg_no);
    Qry.Execute();
  };

  return true;
};

void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms )
{
	Alarms.clearFlags();
	TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT overload_alarm,brd_alarm,waitlist_alarm,pr_etstatus,pr_salon,act,pr_airp_seance "
    " FROM trip_sets, trip_stages, "
    " ( SELECT COUNT(*) pr_salon FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2 ) a "
    " WHERE trip_sets.point_id=:point_id AND "
    "       trip_stages.point_id(+)=trip_sets.point_id AND "
    "       trip_stages.stage_id(+)=:OpenCheckIn ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "OpenCheckIn", otInteger, sOpenCheckIn );
  Qry.Execute();
	if (Qry.Eof) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);
  if ( Qry.FieldAsInteger( "overload_alarm" ) ) {
   	Alarms.setFlag( atOverload );
  }
  if ( Qry.FieldAsInteger( "waitlist_alarm" ) ) {
   	Alarms.setFlag( atWaitlist );
  }
  if ( Qry.FieldAsInteger( "brd_alarm" ) ) {
   	Alarms.setFlag( atBrd );
  }
	if ( !Qry.FieldAsInteger( "pr_salon" ) && !Qry.FieldIsNULL( "act" ) ) {
		Alarms.setFlag( atSalon );
	}
	if ( Qry.FieldAsInteger( "pr_etstatus" ) < 0 ) {
		Alarms.setFlag( atETStatus );
	}
	if (USE_SEANCES())
	{
  	if ( Qry.FieldIsNULL( "pr_airp_seance" ) ) {
  	  Alarms.setFlag( atSeance );
    }
  };
}

string TripAlarmString( TTripAlarmsType &alarm )
{
	string mes;
	switch( alarm ) {
		case atOverload:
			mes = AstraLocale::getLocaleText("Перегрузка");
			break;
		case atWaitlist:
			mes = AstraLocale::getLocaleText("Лист ожидания");
			break;
		case atBrd:
			mes = AstraLocale::getLocaleText("Посадка");
			break;
		case atSalon:
			mes = AstraLocale::getLocaleText("Не назначен салон");
			break;
		case atETStatus:
			mes = AstraLocale::getLocaleText("Нет связи с СЭБ");
			break;
		case atSeance:
		  mes = AstraLocale::getLocaleText("Не определен сеанс");
			break;
		default:;
	}
	return mes;
}
/*
string GetSeatView(const TSeat &seat, const std::string &format, bool pr_lat_seat)
{
  GetSeatRangeView(TSeatRange(seat,seat), pr_lat_seat)
};

string GetSeatRangeView(const TSeatRange &range, const std::string &format, bool pr_lat_seat)
{
};*/

TPaxSeats::TPaxSeats( int point_id )
{
	pr_lat_seat = 1;
	Qry = new TQuery( &OraSession );
	Qry->SQLText =
	  "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
	Qry->CreateVariable( "point_id", otInteger, point_id );
	Qry->Execute();
	if ( !Qry->Eof )
		pr_lat_seat = Qry->FieldAsInteger( "pr_lat_seat" );
	Qry->Clear();
	Qry->SQLText =
    "SELECT first_xname, first_yname "
    " FROM trip_comp_layers, grp_status_types "
    " WHERE trip_comp_layers.point_id=:point_id AND "
    "       trip_comp_layers.pax_id=:pax_id AND "
    "       trip_comp_layers.layer_type=grp_status_types.layer_type "
    "ORDER BY first_yname, first_xname";
  Qry->CreateVariable( "point_id", otInteger, point_id );
  Qry->DeclareVariable( "pax_id", otInteger );
}

std::string TPaxSeats::getSeats( int pax_id, const std::string format )
{
	string res;

	Qry->SetVariable( "pax_id", pax_id );
	Qry->Execute();
	int c=0;
	char* add_ch = NULL;
	if ( format == "_list" || format == "_one" || format == "_seats" )
		add_ch = " ";
 	while ( !Qry->Eof ) {
 		TSeat seat;
    if ( format == "list" && !res.empty() )
    	res += " ";
   	res = denorm_iata_row( Qry->FieldAsString( "first_yname" ), add_ch ) +
          denorm_iata_line( Qry->FieldAsString( "first_xname" ), pr_lat_seat );
    add_ch = NULL;
    if ( format == "one" )
    	break;
   	c++;
    Qry->Next();
  }
  if ( format != "list" && format != "one" && c > 1 )
  	res += "+" + IntToString( c - 1 );
  return res;
}

TPaxSeats::~TPaxSeats()
{
	delete Qry;
}

void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo)
{
  markFltInfo.clear();
  TDateTime scd_local=UTCToLocal(operFltInfo.scd_out, AirpTZRegion(operFltInfo.airp));

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline_mark,flt_no_mark "
    "FROM codeshare_sets "
    "WHERE airline_oper=:airline AND flt_no_oper=:flt_no AND airp_dep=:airp_dep AND "
    "      first_date<=:scd_local AND "
    "      (last_date IS NULL OR last_date>:scd_local) AND "
    "      (days IS NULL OR INSTR(days,TO_CHAR(:wday))<>0) AND pr_del=0 "
    "ORDER BY flt_no_mark,airline_mark";
  Qry.CreateVariable("airline",otString,operFltInfo.airline);
  Qry.CreateVariable("flt_no",otInteger,operFltInfo.flt_no);
  Qry.CreateVariable("airp_dep",otString,operFltInfo.airp);
  Qry.CreateVariable("scd_local",otDate,scd_local);
  Qry.CreateVariable("wday",otInteger,DayOfWeek(scd_local));
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTripInfo flt;
    flt.airline=Qry.FieldAsString("airline_mark");
    flt.flt_no=Qry.FieldAsInteger("flt_no_mark");
    flt.scd_out=scd_local;
    flt.airp=operFltInfo.airp;
    markFltInfo.push_back(flt);
  };
};

TCodeShareSets::TCodeShareSets()
{
  clear();
  Qry = new TQuery( &OraSession );
  Qry->SQLText=
    "SELECT pr_mark_norms, pr_mark_bp, pr_mark_rpt "
    "FROM codeshare_sets "
    "WHERE airline_oper=:airline_oper AND flt_no_oper=:flt_no_oper AND airp_dep=:airp_dep AND "
    "      airline_mark=:airline_mark AND flt_no_mark=:flt_no_mark AND "
    "      first_date<=:scd_local AND "
    "      (last_date IS NULL OR last_date>:scd_local) AND "
    "      (days IS NULL OR INSTR(days,TO_CHAR(:wday))<>0) AND pr_del=0 ";
  Qry->DeclareVariable("airline_oper",otString);
  Qry->DeclareVariable("flt_no_oper",otInteger);
  Qry->DeclareVariable("airp_dep",otString);
  Qry->DeclareVariable("airline_mark",otString);
  Qry->DeclareVariable("flt_no_mark",otInteger);
  Qry->DeclareVariable("scd_local",otDate);
  Qry->DeclareVariable("wday",otInteger);
};

TCodeShareSets::~TCodeShareSets()
{
  delete Qry;
};

void TCodeShareSets::get(const TTripInfo &operFlt, const TTripInfo &markFlt)
{
  clear();
  if (operFlt.airline==markFlt.airline &&
      operFlt.flt_no==markFlt.flt_no) return;

  TDateTime scd_local=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));

  Qry->SetVariable("airline_oper",operFlt.airline);
  Qry->SetVariable("flt_no_oper",operFlt.flt_no);
  Qry->SetVariable("airp_dep",operFlt.airp);
  Qry->SetVariable("airline_mark",markFlt.airline);
  Qry->SetVariable("flt_no_mark",markFlt.flt_no);
  Qry->SetVariable("scd_local",scd_local);
  Qry->SetVariable("wday",DayOfWeek(scd_local));
  Qry->Execute();
  if (!Qry->Eof)
  {
    pr_mark_norms=Qry->FieldAsInteger("pr_mark_norms")!=0;
    pr_mark_bp=Qry->FieldAsInteger("pr_mark_bp")!=0;
    pr_mark_rpt=Qry->FieldAsInteger("pr_mark_rpt")!=0;
  };
};

string GetMktFlightStr( const TTripInfo &operFlt, const TTripInfo &markFlt, bool &equal )
{
  equal=false;
  TDateTime scd_local_oper=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));
  modf(scd_local_oper,&scd_local_oper);
  TDateTime scd_local_mark=markFlt.scd_out;
  modf(scd_local_mark,&scd_local_mark);
  equal=operFlt.airline==markFlt.airline &&
        operFlt.flt_no==markFlt.flt_no &&
        operFlt.suffix==markFlt.suffix &&
        scd_local_oper==scd_local_mark &&
        operFlt.airp==markFlt.airp;

  ostringstream trip;
  trip << ElemIdToCodeNative(etAirline, markFlt.airline)
       << setw(3) << setfill('0') << markFlt.flt_no
       << ElemIdToCodeNative(etSuffix, markFlt.suffix);
  if (scd_local_oper!=scd_local_mark)
    trip << "/" << DateTimeToStr(markFlt.scd_out,"dd");
  if (operFlt.airp!=markFlt.airp)
    trip << " " << ElemIdToCodeNative(etAirp, markFlt.airp);
  return trip.str();
};

bool IsMarkEqualOper( const TTripInfo &operFlt, const TTripInfo &markFlt )
{
  TDateTime scd_local_oper=UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));
  modf(scd_local_oper,&scd_local_oper);
  TDateTime scd_local_mark=markFlt.scd_out;
  modf(scd_local_mark,&scd_local_mark);
  return operFlt.airline==markFlt.airline &&
         operFlt.flt_no==markFlt.flt_no &&
         operFlt.suffix==markFlt.suffix &&
         scd_local_oper==scd_local_mark &&
         operFlt.airp==markFlt.airp;
};

void GetCrsList(int point_id, std::vector<std::string> &crs)
{
  crs.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT crs FROM tlg_binding,crs_data_stat "
    "WHERE tlg_binding.point_id_tlg=crs_data_stat.point_id AND "
    "      tlg_binding.point_id_spp=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    crs.push_back(Qry.FieldAsString("crs"));
};

//bt

// pr_lat с клиента || стыковочные пункты grp_id не в РФ || пункт вылета grp_id не в РФ

// bp

// pr_lat c клиента || пункт вылета grp_id не в РФ || пункт прилета grp_id не в РФ

bool IsTrferInter(string airp_dep, string airp_arv, string &country)
{
	TBaseTable &baseairps = base_tables.get( "airps" );
	TBaseTable &basecities = base_tables.get( "cities" );
    string country_dep = ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_dep, true )).city)).country;
    string country_arv = ((TCitiesRow&)basecities.get_row( "code", ((TAirpsRow&)baseairps.get_row( "code", airp_arv, true )).city)).country;
    country.clear();
    if(country_dep != country_arv) return true;
    country = country_dep;
    return false;
}

bool IsRouteInter(int point_dep, int point_arv, string &country)
{
    country.clear();
    string first_country;
    TBaseTable &airps = base_tables.get("AIRPS");
    TBaseTable &cities = base_tables.get("CITIES");
    TTripRoute route;
    if (!route.GetRouteAfter(point_dep,trtWithCurrent,trtNotCancelled))
        throw Exception("TTripRoute::GetRouteAfter: flight not found for point_dep %d", point_dep);
    for(vector<TTripRouteItem>::iterator iv = route.begin(); iv != route.end(); iv++)
    {
        string c = cities.get_row("code",airps.get_row("code",iv->airp).AsString("city")).AsString("country");
        if(iv == route.begin())
            first_country = c;
        else if(first_country != c) return true;
        if (point_arv!=NoExists && iv->point_id==point_arv) break;
    };
    country = first_country;
    return false;
}

bool IsProtPaidSeatRem(const string &airline_mark, const string &seat_rem)
{
  return (airline_mark=="НН" && (seat_rem=="SEAT" || seat_rem=="RQST"));
};

void GetSeatRemPriority(const string &airline_mark, TSeatRemPriority &rems)
{
  rems.clear();
  int priority=1;
  if (airline_mark=="НН")
  {
    rems.push_back(make_pair("SEAT",priority));
    priority++;
    rems.push_back(make_pair("RQST",priority));
    priority++;
  }
  else
  {
    rems.push_back(make_pair("SEAT",priority));
    rems.push_back(make_pair("RQST",priority));
  };
    
  rems.push_back(make_pair("EXST",priority));
  rems.push_back(make_pair("GPST",priority));
  rems.push_back(make_pair("NSST",priority));
  rems.push_back(make_pair("NSSA",priority));
  rems.push_back(make_pair("NSSB",priority));
  rems.push_back(make_pair("NSSW",priority));
  rems.push_back(make_pair("SMST",priority));
  rems.push_back(make_pair("SMSA",priority));
  rems.push_back(make_pair("SMSB",priority));
  rems.push_back(make_pair("SMSW",priority));
};

