#include "astra_misc.h"
#include <string>
#include <vector>
#include <set>
#include "astra_date_time.h"
#include "exceptions.h"
#include "oralib.h"
#include "convert.h"
#include "astra_locale.h"
#include "seats_utils.h"
#include "aodb.h"
#include "meridian.h"
#include "basel_aero.h"
#include "exch_checkin_result.h"
#include "qrys.h"
#include "emdoc.h"
#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"
#include "apis_utils.h"

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;

bool TTripInfo::getByPointId ( const TDateTime part_key,
                               const int point_id,
                               const FlightProps &props )
{
  TQuery Qry( &OraSession );
  if (part_key==NoExists)
  {
    Qry.SQLText =
      "SELECT " + selectedFields() +
      "FROM points "
      "WHERE point_id=:point_id AND pr_del>=0";
  }
  else
  {
    Qry.SQLText =
      "SELECT " + selectedFields() +
      "FROM arx_points "
      "WHERE part_key=:part_key AND point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable( "part_key", otDate, part_key );
  };
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  if ( Qry.Eof ) return false;
  if (!match(Qry, props)) return false;

  init( Qry );
  return true;
}

bool TTripInfo::getByPointId ( const int point_id,
                               const FlightProps &props )
{
  return getByPointId(NoExists, point_id, props);
}

bool TTripInfo::getByPointIdTlg ( const int point_id_tlg )
{
  TAdvTripInfoList trips;
  getTripsByPointIdTlg(point_id_tlg, trips);
  if (trips.empty()) return false;
  *this=trips.front();
  return true;
}

bool TTripInfo::getByCRSPnrId(const int pnr_id)
{
  TAdvTripInfoList trips;
  getTripsByCRSPnrId(pnr_id, trips);
  if (trips.empty()) return false;
  *this=trips.front();
  return true;
}

bool TTripInfo::getByCRSPaxId(const int pax_id)
{
  TAdvTripInfoList trips;
  getTripsByCRSPaxId(pax_id, trips);
  if (trips.empty()) return false;
  *this=trips.front();
  return true;
}

bool TTripInfo::getByPaxId ( const int pax_id )
{
    TCachedQuery Qry(
            "select point_dep from pax_grp, pax where "
            "   pax.pax_id = :pax_id and "
            "   pax.grp_id = pax_grp.grp_id ",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    if(Qry.get().Eof) return false;
    return getByPointId( Qry.get().FieldAsInteger( "point_dep" ) );
}

bool TTripInfo::getByGrpId ( const int grp_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_dep FROM pax_grp WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();

  if ( Qry.Eof ) return false;

  return getByPointId( Qry.FieldAsInteger( "point_dep" ) );
}

void getPointIdsSppByPointIdTlg(const int point_id_tlg, std::set<int>& point_ids_spp)
{
  point_ids_spp.clear();

  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg = :point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id_tlg );
  Qry.Execute();
  for( ; !Qry.Eof; Qry.Next() )
    point_ids_spp.insert(Qry.FieldAsInteger( "point_id_spp" ));
}

void getTripsByPointIdTlg( const int point_id_tlg, TAdvTripInfoList &trips )
{
  trips.clear();

  set<int> point_ids_spp;
  getPointIdsSppByPointIdTlg(point_id_tlg, point_ids_spp);

  for(const int& id : point_ids_spp)
  {
    TAdvTripInfo info;
    if (info.getByPointId( id ))
      trips.push_back( info );
  }
}

void getTripsByCRSPnrId(const int pnr_id, TAdvTripInfoList &trips)
{
  trips.clear();
  TCachedQuery Qry("SELECT point_id FROM crs_pnr WHERE pnr_id=:pnr_id AND system='CRS'",
                   QParams() << QParam("pnr_id", otInteger, pnr_id));
  Qry.get().Execute();
  if(Qry.get().Eof) return;

  getTripsByPointIdTlg(Qry.get().FieldAsInteger("point_id"), trips);
}

namespace ASTRA
{

template<> const TAdvTripInfoList& PnrFlightsCache::add(const int& pnrId) const
{
  TAdvTripInfoList& flts=items.emplace(pnrId, TAdvTripInfoList()).first->second;

  getTripsByCRSPnrId(pnrId, flts);

  return flts;
}

template<> std::string PnrFlightsCache::traceTitle()
{
  return "PnrFlightsCache";
}

} //namespace ASTRA

void getTripsByCRSPaxId(const int pax_id, TAdvTripInfoList &trips)
{
  trips.clear();
  TCachedQuery Qry("SELECT point_id "
                   "FROM crs_pnr, crs_pax "
                   "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:pax_id AND "
                   "      crs_pnr.system='CRS' AND crs_pax.pr_del=0 ",
                   QParams() << QParam("pax_id", otInteger, pax_id));
  Qry.get().Execute();
  if(Qry.get().Eof) return;

  getTripsByPointIdTlg(Qry.get().FieldAsInteger("point_id"), trips);
}

void TTripInfo::get_client_dates(TDateTime &scd_out_client, TDateTime &real_out_client, bool trunc_time) const
{
  scd_out_client=ASTRA::NoExists;
  real_out_client=ASTRA::NoExists;

  if (airp.empty() || scd_out==ASTRA::NoExists) return;

  const string &tz_region=AirpTZRegion(airp);
  scd_out_client=UTCToClient(scd_out,tz_region);
  if (trunc_time)
    modf(scd_out_client,&scd_out_client);
  if (act_est_scd_out()!=ASTRA::NoExists)
  {
    real_out_client=UTCToClient(act_est_scd_out(),tz_region);
    if (trunc_time)
      modf(real_out_client,&real_out_client);
  }
  else
    real_out_client=scd_out_client;
};

void TTripInfo::get_times_in(const int &point_arv,
                             TDateTime &scd_in,
                             TDateTime &est_in,
                             TDateTime &act_in)
{
  scd_in=ASTRA::NoExists;
  est_in=ASTRA::NoExists;
  act_in=ASTRA::NoExists;

  TCachedQuery PointsQry("SELECT scd_in, est_in, act_in FROM points WHERE point_id=:point_arv AND pr_del>=0",
                         QParams() << QParam("point_arv", otInteger, point_arv));
  PointsQry.get().Execute();
  if (PointsQry.get().Eof) return;
  if (!PointsQry.get().FieldIsNULL("scd_in"))
    scd_in=PointsQry.get().FieldAsDateTime("scd_in");
  if (!PointsQry.get().FieldIsNULL("est_in"))
    est_in=PointsQry.get().FieldAsDateTime("est_in");
  if (!PointsQry.get().FieldIsNULL("act_in"))
    act_in=PointsQry.get().FieldAsDateTime("act_in");
}

TDateTime TTripInfo::get_scd_in(const int &point_arv)
{
  TDateTime scd_in, est_in, act_in;
  get_times_in(point_arv, scd_in, est_in, act_in);
  return scd_in;
}

TDateTime TTripInfo::act_est_scd_in(const int &point_arv)
{
  TDateTime scd_in, est_in, act_in;
  get_times_in(point_arv, scd_in, est_in, act_in);
  return act_in!=ASTRA::NoExists?act_in:
         est_in!=ASTRA::NoExists?est_in:
                                 scd_in;
}

TDateTime TTripInfo::get_scd_in(const std::string &airp_arv) const
{
  if (point_id==ASTRA::NoExists) return ASTRA::NoExists;
  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       point_id,
                       trtNotCurrent, trtWithCancelled );
  for( TTripRoute::const_iterator iroute=route.begin(); iroute!=route.end(); ++iroute )
    if ( iroute->airp == airp_arv )
      return TTripInfo::get_scd_in(iroute->point_id);
  return ASTRA::NoExists;
}

std::string TTripInfo::flight_view(TElemContext ctxt, bool showScdOut, bool showAirp) const
{
  ostringstream s;
  s << ElemIdToElemCtxt(ctxt, etAirline, airline, ctxt==ecNone?efmtCodeNative:airline_fmt)
    << setw(3) << setfill('0') << flt_no
    << ElemIdToElemCtxt(ctxt, etSuffix, suffix, ctxt==ecNone?efmtCodeNative:suffix_fmt);
  if (showScdOut)
  {
    if (scd_out!=NoExists)
      s << "/" << DateTimeToStr(scd_out,"dd");
    else
      s << "/??";
  };
  if (showAirp)
    s << " " << ElemIdToElemCtxt(ctxt, etAirp, airp, ctxt==ecNone?efmtCodeNative:airp_fmt);
  return s.str();
}

string GetTripDate( const TTripInfo &info, const string &separator, const bool advanced_trip_list )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TDateTime scd_out_client, real_out_client, desk_time;
  modf(reqInfo->desk.time,&desk_time);

  info.get_client_dates(scd_out_client, real_out_client);

  ostringstream date;

  if (desk_time!=real_out_client)
  {
    if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(real_out_client,"mm"))
      date << separator << DateTimeToStr(real_out_client,"dd");
    else
      date << separator << DateTimeToStr(real_out_client,"dd.mm");
  };
  if (scd_out_client!=real_out_client)
  {
    if (!advanced_trip_list ||
        DateTimeToStr(desk_time,"mm")==DateTimeToStr(scd_out_client,"mm"))
      date << "(" << DateTimeToStr(scd_out_client,"dd") << ")";
    else
      date << "(" << DateTimeToStr(scd_out_client,"dd.mm") << ")";
  };

  return date.str();
};

//для сохранения совместимости вводим AstraLocale::TLocaleType
string GetTripName( const TTripInfo &info, TElemContext ctxt, bool showAirp, bool prList )
{
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

  trip << GetTripDate(info, "/", false);

  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!(reqInfo->user.user_type==utAirport &&
        reqInfo->user.access.airps().only_single_permit())||showAirp) {
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

bool TAdvTripInfo::getByPointId ( const TDateTime part_key,
                                  const int point_id,
                                  const FlightProps &props )
{
  TQuery Qry( &OraSession );
  if (part_key==NoExists)
  {
    Qry.SQLText =
      "SELECT " + selectedFields() +
      "FROM points "
      "WHERE point_id=:point_id AND pr_del>=0 ";
  }
  else
  {
    Qry.SQLText =
      "SELECT "  + selectedFields() +
      "FROM arx_points "
      "WHERE part_key=:part_key AND point_id=:point_id AND pr_del>=0 ";
    Qry.CreateVariable( "part_key", otDate, part_key );
  };
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  if ( Qry.Eof ) return false;
  if (!match(Qry, props)) return false;

  Init( Qry );
  return true;
}

bool TAdvTripInfo::getByPointId ( const int point_id,
                                  const FlightProps &props )
{
  return getByPointId(NoExists, point_id, props);
}

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

bool DefaultTripSets( const TTripSetType setType )
{
  return setType==tsOverloadReg ||
         setType==tsAPISControl;
}

bool GetTripSets( const TTripSetType setType,
                  const TTripInfo &info )
{
  if (!(setType>=0 && setType<100))
    throw Exception("%s: wrong setType=%d", __FUNCTION__, (int)setType);

  TQuery Qry( &OraSession );
  Qry.Clear();
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
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  if (Qry.Eof) return DefaultTripSets(setType);
  return Qry.FieldAsInteger("pr_misc")!=0;
};

bool GetSelfCkinSets( const TTripSetType setType,
                      const int point_id,
                      const ASTRA::TClientType client_type )
{
  if (point_id==ASTRA::NoExists)
    throw Exception("%s: wrong point_id=NoExists", __FUNCTION__);
  TCachedQuery Qry("SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id AND pr_del>=0",
                   QParams() << QParam("point_id", otInteger, point_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  TTripInfo info(Qry.get());
  return GetSelfCkinSets(setType, info, client_type);
};

bool GetSelfCkinSets(const TTripSetType setType,
                     const TTripInfo &info,
                     const ASTRA::TClientType client_type )
{
  if (!(setType>=200 && setType<300))
    throw Exception("%s: wrong setType=%d", __FUNCTION__, (int)setType);
  if (!(client_type==ctWeb ||
        client_type==ctKiosk ||
        client_type==ctMobile))
    throw Exception("%s: wrong client_type=%s (setType=%d)", __FUNCTION__, EncodeClientType(client_type), (int)setType);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT value, "
    "    DECODE(client_type,NULL,0,1)+ "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM self_ckin_set "
    "WHERE type=:type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
    "      (client_type IS NULL OR client_type=:client_type) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("type",otInteger,(int)setType);
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.CreateVariable("client_type",otString,EncodeClientType(client_type));
  Qry.Execute();
  if (Qry.Eof)
  {
    switch(setType)
    {
      default: return false;
    };
  };
  return Qry.FieldAsInteger("value")!=0;
};

TDateTime DayMonthToDate(int day, int month, TDateTime base_date, TDateDirection direction)
{
    if (day<1 || day>31) throw EConvertError("%s: wrong day: %d", __FUNCTION__, day);
    if (month<1 || month>12) throw EConvertError("%s: wrong month: %d", __FUNCTION__, month);
    boost::optional<TDateTime> result = 0.;
    result=boost::none;
    int Year, Month, Day;
    DecodeDate(base_date, Year, Month, Day);

    int lower_offset = 0, upper_offset = 0;

    //+-8 лет достаточно, чтобы гарантированно попасть на високосный год
    if (direction == dateBefore || direction == dateEverywhere)
        lower_offset = (day == 29 and month == 2 ? 8 : 1);

    if (direction == dateAfter || direction == dateEverywhere)
        upper_offset = (day == 29 and month == 2 ? 8 : 1);

    for (int y = Year - lower_offset; y <= Year + upper_offset; ++y)
    {
        TDateTime d;
        try {
            EncodeDate(y, month, day, d);
        }
        catch(const EConvertError&) { continue; };

        if ((direction == dateBefore && d > base_date) || (direction == dateAfter && d < base_date))
            continue;

        if (!result || fabs(result.get() - base_date) > fabs(d - base_date))
            result = d;
    }
    if (!result) throw EConvertError("impossible");
    return result.get();
}

TDateTime DayToDate(int day, TDateTime base_date, bool back)
{
  if (day<1 || day>31) throw EConvertError("DayToDate: wrong day: %d", day);
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
      catch(const EConvertError&) {};
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
      catch(const EConvertError&) {};
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

void TTripRoute::GetRoute(TDateTime part_key,
                          int point_id,
                          int point_num,
                          int first_point,
                          bool pr_tranzit,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2,
                          TQuery& Qry)
{
  clear();
  ostringstream sql;
  if (part_key!=NoExists)
    sql << "SELECT part_key,point_id,point_num,airp,pr_del "
           "FROM arx_points ";
  else
    sql << "SELECT point_id,point_num,airp,pr_del "
        << "FROM points ";

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

  if (part_key!=NoExists)
    sql << "AND part_key=:part_key ";

  sql << "ORDER BY point_num";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  if (part_key!=NoExists)
    Qry.CreateVariable("part_key",otDate,part_key);
  if (!pr_tranzit && after_current)
    Qry.CreateVariable("first_point",otInteger,point_id);
  else
  {
    if (first_point!=NoExists)
      Qry.CreateVariable("first_point",otInteger,first_point);
    else
      Qry.CreateVariable("first_point",otInteger,point_id);
  };
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTripRouteItem item;
    if (part_key!=NoExists)
      item.part_key=Qry.FieldAsDateTime("part_key");
    item.point_id=Qry.FieldAsInteger("point_id");
    item.point_num=Qry.FieldAsInteger("point_num");
    item.airp=Qry.FieldAsString("airp");
    item.pr_cancel=Qry.FieldAsInteger("pr_del")!=0;
    push_back(item);
  };
};

void TAdvTripRoute::GetRoute(TDateTime part_key,
                          int point_id,
                          int point_num,
                          int first_point,
                          bool pr_tranzit,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2,
                          TQuery& Qry)
{
  clear();
  ostringstream sql;
  if (part_key!=NoExists)
    sql << "SELECT part_key,point_id,point_num,airp,pr_del, "
           "airline,suffix,flt_num,scd_in,scd_out, act_out "
           "FROM arx_points ";
  else
    sql << "SELECT point_id,point_num,airp,pr_del, "
           "airline,suffix,flt_no,scd_in,scd_out, act_out "
        << "FROM points ";

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

  if (part_key!=NoExists)
    sql << "AND part_key=:part_key ";

  sql << "ORDER BY point_num";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  if (part_key!=NoExists)
    Qry.CreateVariable("part_key",otDate,part_key);
  if (!pr_tranzit && after_current)
    Qry.CreateVariable("first_point",otInteger,point_id);
  else
  {
    if (first_point!=NoExists)
      Qry.CreateVariable("first_point",otInteger,first_point);
    else
      Qry.CreateVariable("first_point",otInteger,point_id);
  };
  Qry.CreateVariable("point_num",otInteger,point_num);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TAdvTripRouteItem item;
    if (part_key!=NoExists)
      item.part_key=Qry.FieldAsDateTime("part_key");
    item.point_id=Qry.FieldAsInteger("point_id");
    item.point_num=Qry.FieldAsInteger("point_num");
    item.airp=Qry.FieldAsString("airp");
    item.pr_cancel=Qry.FieldAsInteger("pr_del")!=0;
    if (!Qry.FieldIsNULL("airline"))
      item.airline=Qry.FieldAsString("airline");
    if (!Qry.FieldIsNULL("suffix"))
      item.suffix=Qry.FieldAsString("suffix");
    if (!Qry.FieldIsNULL("flt_no"))
      item.flt_num=Qry.FieldAsInteger("flt_no");
    if (!Qry.FieldIsNULL("scd_in"))
      item.scd_in=Qry.FieldAsDateTime("scd_in");
    if (!Qry.FieldIsNULL("scd_out"))
      item.scd_out=Qry.FieldAsDateTime("scd_out");
    if (!Qry.FieldIsNULL("act_out"))
      item.act_out=Qry.FieldAsDateTime("act_out");
    push_back(item);
  };
}

bool TTripRoute::GetRoute(TDateTime part_key,
                          int point_id,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);

  ostringstream sql;
  sql << "SELECT point_num,first_point,pr_tranzit ";
  if (part_key!=NoExists)
    sql << "FROM arx_points ";
  else
    sql << "FROM points ";

  sql << "WHERE point_id=:point_id AND pr_del>=0";

  if (part_key!=NoExists)
    sql << "AND part_key=:part_key ";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  if (part_key!=NoExists)
    Qry.CreateVariable("part_key",otDate,part_key);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  GetRoute(part_key,
           point_id,
           Qry.FieldAsInteger("point_num"),
           Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
           Qry.FieldAsInteger("pr_tranzit")!=0,
           after_current,route_type1,route_type2,Qry);
  return true;
}

bool TAdvTripRoute::GetRoute(TDateTime part_key,
                          int point_id,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
  clear();

  TQuery Qry(&OraSession);

  ostringstream sql;
  sql << "SELECT point_num,first_point,pr_tranzit ";
  if (part_key!=NoExists)
    sql << "FROM arx_points ";
  else
    sql << "FROM points ";

  sql << "WHERE point_id=:point_id AND pr_del>=0";

  if (part_key!=NoExists)
    sql << "AND part_key=:part_key ";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  if (part_key!=NoExists)
    Qry.CreateVariable("part_key",otDate,part_key);
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  GetRoute(part_key,
           point_id,
           Qry.FieldAsInteger("point_num"),
           Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
           Qry.FieldAsInteger("pr_tranzit")!=0,
           after_current,route_type1,route_type2,Qry);
  return true;
}

bool TTripBase::GetRouteAfter(TDateTime part_key,
                               int point_id,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  return GetRoute(part_key,point_id,true,route_type1,route_type2);
};

bool TTripBase::GetRouteBefore(TDateTime part_key,
                                int point_id,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  return GetRoute(part_key,point_id,false,route_type1,route_type2);
};

//возвращает истину, когда рейс найден, но не факт, что есть маршрут
void TTripBase::GetRouteAfter(TDateTime part_key,
                               int point_id,
                               int point_num,
                               int first_point,
                               bool pr_tranzit,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
           true,route_type1,route_type2,Qry);
};

void TTripBase::GetRouteAfter(const TAdvTripInfo& fltInfo,
                              TTripRouteType1 route_type1,
                              TTripRouteType2 route_type2)
{
  TQuery Qry(&OraSession);
  GetRoute(NoExists,
           fltInfo.point_id,
           fltInfo.point_num,
           fltInfo.first_point,
           fltInfo.pr_tranzit,
           true,route_type1,route_type2,Qry);
}

void TTripBase::GetRouteBefore(TDateTime part_key,
                                int point_id,
                                int point_num,
                                int first_point,
                                bool pr_tranzit,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
           false,route_type1,route_type2,Qry);
}

void TTripBase::GetRouteBefore(const TAdvTripInfo& fltInfo,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  TQuery Qry(&OraSession);
  GetRoute(NoExists,
           fltInfo.point_id,
           fltInfo.point_num,
           fltInfo.first_point,
           fltInfo.pr_tranzit,
           false,route_type1,route_type2,Qry);
};

void TTripRoute::GetNextAirp(TDateTime part_key,
                             int point_id,
                             int point_num,
                             int first_point,
                             bool pr_tranzit,
                             TTripRouteType2 route_type2,
                             TTripRouteItem& item)
{
  item.Clear();
  clear();
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
           true,trtNotCurrent,route_type2,Qry);
  if (!empty()) item=front();
};

bool TTripRoute::GetNextAirp(TDateTime part_key,
                             int point_id,
                             TTripRouteType2 route_type2,
                             TTripRouteItem& item)
{
  item.Clear();
  if (!GetRoute(part_key,point_id,true,trtNotCurrent,route_type2)) return false;
  if (!empty()) item=front();
  return true;
};

void TTripRoute::GetPriorAirp(TDateTime part_key,
                              int point_id,
                              int point_num,
                              int first_point,
                              bool pr_tranzit,
                              TTripRouteType2 route_type2,
                              TTripRouteItem& item)
{
  item.Clear();
  clear();
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
           false,trtNotCurrent,route_type2,Qry);
  if (!empty()) item=back();
};

bool TTripRoute::GetPriorAirp(TDateTime part_key,
                              int point_id,
                              TTripRouteType2 route_type2,
                              TTripRouteItem& item)
{
  item.Clear();
  if (!GetRoute(part_key,point_id,false,trtNotCurrent,route_type2)) return false;
  if (!empty()) item=back();
  return true;
};

string TTripRoute::GetStr() const
{
  ostringstream res;
  for(TTripRoute::const_iterator r=begin();r!=end();r++)
  {
    if (r!=begin()) res << " -> ";
    res << r->point_num << ":" << r->airp
        << "(" << (r->part_key==NoExists?"":DateTimeToStr(r->part_key, "dd.mm.yy hh:nn:ss + "))
        << r->point_id << ")";
  };
  return res.str();
}

bool TTrferRoute::GetRoute(int grp_id,
                           TTrferRouteType route_type)
{
  clear();
  TQuery Qry(&OraSession);
  if (route_type==trtWithFirstSeg)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT " + TTripInfo::selectedFields("points") + ", "
      "       pax_grp.airp_arv, pax_grp.piece_concept "
      "FROM pax_grp, points "
      "WHERE points.point_id=pax_grp.point_dep AND "
      "      pax_grp.grp_id = :grp_id AND points.pr_del>=0";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if (Qry.Eof) return false;
    push_back(TTrferRouteItem());
    TTrferRouteItem &item=back();
    item.operFlt.Init(Qry);
    if (item.operFlt.scd_out!=NoExists)
      item.operFlt.scd_out=UTCToLocal(item.operFlt.scd_out, AirpTZRegion(item.operFlt.airp));
    if (item.operFlt.est_out && item.operFlt.est_out.get()!=NoExists)
      item.operFlt.est_out=UTCToLocal(item.operFlt.est_out.get(), AirpTZRegion(item.operFlt.airp));
    if (item.operFlt.act_out && item.operFlt.act_out.get()!=NoExists)
      item.operFlt.act_out=UTCToLocal(item.operFlt.act_out.get(), AirpTZRegion(item.operFlt.airp));
    item.airp_arv=Qry.FieldAsString("airp_arv");
    if (!Qry.FieldIsNULL("piece_concept"))
      item.piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,airline_fmt,flt_no,suffix,suffix_fmt,scd AS scd_out, "
    "       airp_dep AS airp,airp_dep_fmt AS airp_fmt,airp_arv,airp_arv_fmt, "
    "       transfer.piece_concept "
    "FROM transfer,trfer_trips "
    "WHERE transfer.point_id_trfer=trfer_trips.point_id AND "
    "      grp_id=:grp_id AND transfer_num>0 "
    "ORDER BY transfer_num";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    push_back(TTrferRouteItem());
    TTrferRouteItem &item=back();
    item.operFlt.Init(Qry);
    item.airp_arv=Qry.FieldAsString("airp_arv");
    item.airp_arv_fmt=(TElemFmt)Qry.FieldAsInteger("airp_arv_fmt");
    if (!Qry.FieldIsNULL("piece_concept"))
      item.piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
  };
  return true;
};

std::string flight_view(int grp_id, int seg_no)
{
  if (seg_no<1) return "";
  if (seg_no>1)
  {
    TTrferRoute trfer;
    trfer.GetRoute(grp_id, trtNotFirstSeg);
    if (seg_no-2<(int)trfer.size())
      return trfer[seg_no-2].operFlt.flight_view();
  }
  else
  {
    TTripInfo operFlt;
    if (operFlt.getByGrpId(grp_id))
      return operFlt.flight_view();
  };
  return "";
}

/*
GetRouteAfterByET() {}

GetRouteByET(int pax_id)
{
  "SELECT ticket_no, coupon_no FROM pax WHERE pax_id=  "
}
*/

bool TCkinRoute::GetRouteByET(
        int pax_id,
        bool after_current,
        TCkinRouteType1 route_type1
        )
{
    clear();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT ticket_no, coupon_no FROM pax WHERE pax_id=:pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if (Qry.Eof) return false;
    GetRouteByET(Qry.FieldAsString("ticket_no"),
            Qry.FieldAsInteger("coupon_no"),
            after_current,route_type1,Qry);
    return true;
}

void TCkinRoute::GetRouteByET(
        const string &tick_no,
        int coupon_no,
        bool after_current,
        TCkinRouteType1 route_type1,
        TQuery& Qry
        )
{
    ostringstream sql;
    sql << "SELECT " + TTripInfo::selectedFields("points") + ", "
        "       pax_grp.grp_id, pax_grp.point_dep, pax_grp.point_arv, "
        "       pax_grp.airp_dep, pax_grp.airp_arv "
        "FROM points, pax_grp, pax "
        "WHERE points.point_id=pax_grp.point_dep AND "
        "      pax_grp.grp_id=pax.grp_id AND "
        "      pax.ticket_no=:tick_no ";
    if (after_current)
        sql << "AND coupon_no>=:coupon_no "
            << "ORDER BY coupon_no ASC ";
    else
        sql << "AND coupon_no<=:coupon_no "
            << "ORDER BY coupon_no DESC ";
    Qry.Clear();
    Qry.SQLText = sql.str().c_str();
    Qry.CreateVariable("tick_no", otString, tick_no);
    Qry.CreateVariable("coupon_no", otInteger, coupon_no);
    Qry.Execute();
    if (!Qry.Eof && Qry.FieldAsInteger("coupon_no")==coupon_no)
    {
        for(;!Qry.Eof;)
        {
            TCkinRouteItem item;
            item.fromDB(Qry);

            Qry.Next();

            if (route_type1==crtNotCurrent && item.coupon_no==coupon_no) continue;
            push_back(item);
        };
        if (!after_current) reverse(begin(),end());
    };
}

void TCkinRoute::GetRoute(int tckin_id,
                          int seg_no,
                          bool after_current,
                          TCkinRouteType1 route_type1,
                          TCkinRouteType2 route_type2,
                          TQuery& Qry)
{
  ostringstream sql;
  sql << "SELECT " + TTripInfo::selectedFields("points") + ", "
         "       pax_grp.grp_id, pax_grp.point_dep, pax_grp.point_arv, "
         "       pax_grp.airp_dep, pax_grp.airp_arv, "
         "       tckin_pax_grp.seg_no, tckin_pax_grp.pr_depend "
         "FROM points, pax_grp, tckin_pax_grp "
         "WHERE points.point_id=pax_grp.point_dep AND "
         "      pax_grp.grp_id=tckin_pax_grp.grp_id AND "
         "      tckin_pax_grp.tckin_id=:tckin_id ";
  if (after_current)
  {
    sql << "AND tckin_pax_grp.seg_no>=:seg_no "
        << "ORDER BY seg_no ASC";
  }
  else
  {

    sql << "AND tckin_pax_grp.seg_no<=:seg_no "
        << "ORDER BY seg_no DESC";
  };

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
  Qry.CreateVariable("tckin_id",otInteger,tckin_id);
  Qry.CreateVariable("seg_no",otInteger,seg_no);
  Qry.Execute();
  if (!Qry.Eof && Qry.FieldAsInteger("seg_no")==seg_no)
  {
    bool pr_depend=true;
    for(;!Qry.Eof;)
    {
      if (route_type2==crtOnlyDependent && !pr_depend) break;

      TCkinRouteItem item;
      item.fromDB(Qry);

      if (!after_current) pr_depend=Qry.FieldAsInteger("pr_depend")!=0;

      Qry.Next();

      if (!Qry.Eof)
      {
        if (after_current) pr_depend=Qry.FieldAsInteger("pr_depend")!=0;
      };

      if (route_type1==crtNotCurrent && item.seg_no==seg_no) continue;
      push_back(item);
    };
    if (!after_current) reverse(begin(),end());
  };
};

bool TCkinRoute::GetRoute(int grp_id,
                          bool after_current,
                          TCkinRouteType1 route_type1,
                          TCkinRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT tckin_id,seg_no FROM tckin_pax_grp WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  GetRoute(Qry.FieldAsInteger("tckin_id"),
           Qry.FieldAsInteger("seg_no"),
           after_current,route_type1,route_type2,Qry);
  return true;
};

bool TCkinRoute::GetRouteAfterByET(int pax_id,
                               TCkinRouteType1 route_type1)
{
  return GetRouteByET(pax_id,true,route_type1);
};

bool TCkinRoute::GetRouteBeforeByET(int pax_id,
                               TCkinRouteType1 route_type1)
{
  return GetRouteByET(pax_id,false,route_type1);
};

void TCkinRoute::GetRouteAfterByET(const string &tick_no,
                               int coupon_no,
                               TCkinRouteType1 route_type1)
{
  clear();
  TQuery Qry(&OraSession);
  return GetRouteByET(tick_no,coupon_no,true,route_type1,Qry);
};

void TCkinRoute::GetRouteBeforeByET(const string &tick_no,
                               int coupon_no,
                               TCkinRouteType1 route_type1)
{
  clear();
  TQuery Qry(&OraSession);
  return GetRouteByET(tick_no,coupon_no,false,route_type1,Qry);
};


bool TCkinRoute::GetRouteAfter(int grp_id,
                               TCkinRouteType1 route_type1,
                               TCkinRouteType2 route_type2)
{
  return GetRoute(grp_id,true,route_type1,route_type2);
};

bool TCkinRoute::GetRouteBefore(int grp_id,
                                TCkinRouteType1 route_type1,
                                TCkinRouteType2 route_type2)
{
  return GetRoute(grp_id,false,route_type1,route_type2);
};

void TCkinRoute::GetRouteAfter(int tckin_id,
                               int seg_no,
                               TCkinRouteType1 route_type1,
                               TCkinRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(tckin_id,seg_no,
           true,route_type1,route_type2,Qry);
};

void TCkinRoute::GetRouteBefore(int tckin_id,
                                int seg_no,
                                TCkinRouteType1 route_type1,
                                TCkinRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(tckin_id,seg_no,
           false,route_type1,route_type2,Qry);
};

void TCkinRoute::GetNextSeg(int tckin_id,
                            int seg_no,
                            TCkinRouteType2 route_type2,
                            TCkinRouteItem& item)
{
  item.Clear();
  clear();
  TQuery Qry(&OraSession);
  GetRoute(tckin_id,seg_no,
           true,crtNotCurrent,route_type2,Qry);
  if (!empty()) item=front();
};

bool TCkinRoute::GetNextSeg(int grp_id,
                            TCkinRouteType2 route_type2,
                            TCkinRouteItem& item)
{
  item.Clear();
  if (!GetRoute(grp_id,true,crtNotCurrent,route_type2)) return false;
  if (!empty()) item=front();
  return true;
};

void TCkinRoute::GetPriorSeg(int tckin_id,
                             int seg_no,
                             TCkinRouteType2 route_type2,
                             TCkinRouteItem& item)
{
  item.Clear();
  clear();
  TQuery Qry(&OraSession);
  GetRoute(tckin_id,seg_no,
           false,crtNotCurrent,route_type2,Qry);
  if (!empty()) item=back();
};

bool TCkinRoute::GetPriorSeg(int grp_id,
                             TCkinRouteType2 route_type2,
                             TCkinRouteItem& item)
{
  item.Clear();
  if (!GetRoute(grp_id,false,crtNotCurrent,route_type2)) return false;
  if (!empty()) item=back();
  return true;
};

const TSimpleMktFlight& TSimpleMktFlight::toXML(xmlNodePtr node,
                                                const boost::optional<AstraLocale::OutputLang>& lang) const
{
  if (node==NULL) return *this;
  NewTextChild(node, "airline", lang?airlineToPrefferedCode(airline, lang.get()):airline);
  NewTextChild(node, "flt_no", flt_no);
  NewTextChild(node, "suffix", lang?ElemIdToPrefferedElem(etSuffix, suffix, efmtCodeNative, lang->get()):suffix);
  return *this;
}

void TMktFlight::dump() const
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
    QParams QryParams;
    QryParams << QParam("id", otInteger, pax_id);
    TCachedQuery Qry(
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
            "    pax_grp.point_id_mark = mark_trips.point_id ",
            QryParams
            );

    clear();
    Qry.get().Execute();
    if(!Qry.get().Eof)
    {
        airline = Qry.get().FieldAsString("mark_airline");
        flt_no = Qry.get().FieldAsInteger("mark_flt_no");
        suffix = Qry.get().FieldAsString("mark_suffix");
        subcls = Qry.get().FieldAsString("mark_subcls");
        TDateTime tmp_scd = Qry.get().FieldAsDateTime("mark_scd");
        int Year, Month, Day;
        DecodeDate(tmp_scd, Year, Month, Day);
        scd_day_local = Day;
        EncodeDate(Year, Month, Day, scd_date_local);
        airp_dep = Qry.get().FieldAsString("mark_airp_dep");
        airp_arv = Qry.get().FieldAsString("mark_airp_arv");
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
        "    crs_pnr.airp_arv tlg_airp_arv, "
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
        "    crs_pnr.airp_arv tlg_airp_arv, "
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

const TGrpMktFlight& TGrpMktFlight::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  xmlNodePtr markFltNode=NewTextChild(node, "mark_flight");
  NewTextChild(markFltNode, "airline", airline);
  NewTextChild(markFltNode, "flt_no", flt_no);
  NewTextChild(markFltNode, "suffix", suffix);
  NewTextChild(markFltNode, "scd", DateTimeToStr(scd_date_local));
  NewTextChild(markFltNode, "airp_dep", airp_dep);
  NewTextChild(markFltNode, "pr_mark_norms", (int)pr_mark_norms);
  return *this;
}

TSimpleMktFlight& TSimpleMktFlight::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  airline=NodeAsStringFast("airline",node2);
  flt_no=NodeAsIntegerFast("flt_no",node2);
  suffix=NodeAsStringFast("suffix",node2);
  return *this;
}

TGrpMktFlight& TGrpMktFlight::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (node2==NULL) return *this;
  airline=NodeAsStringFast("airline",node2);
  flt_no=NodeAsIntegerFast("flt_no",node2);
  suffix=NodeAsStringFast("suffix",node2);
  scd_date_local=NodeAsDateTimeFast("scd",node2);
  modf(scd_date_local,&scd_date_local);
  airp_dep=NodeAsStringFast("airp_dep",node2);
  pr_mark_norms=NodeAsIntegerFast("pr_mark_norms",node2)!=0;
  return *this;
}

const TGrpMktFlight& TGrpMktFlight::toDB(TQuery &Qry) const
{
  Qry.CreateVariable("mark_airline", otString, airline);
  Qry.CreateVariable("mark_flt_no", otInteger, flt_no);
  Qry.CreateVariable("mark_suffix", otString, suffix);
  Qry.CreateVariable("mark_scd", otDate, scd_date_local);
  Qry.CreateVariable("mark_airp_dep", otString, airp_dep);
  Qry.CreateVariable("pr_mark_norms", otInteger,(int)pr_mark_norms);
  return *this;
}

TCkinRouteItem& TCkinRouteItem::fromDB(TQuery &Qry)
{
    Clear();
    grp_id=Qry.FieldAsInteger("grp_id");
    point_dep=Qry.FieldAsInteger("point_dep");
    point_arv=Qry.FieldAsInteger("point_arv");
    airp_dep=Qry.FieldAsString("airp_dep");
    airp_arv=Qry.FieldAsString("airp_arv");
    if(Qry.GetFieldIndex("seg_no") >= 0)
        seg_no=Qry.FieldAsInteger("seg_no");
    if(Qry.GetFieldIndex("coupon_no") >= 0)
        coupon_no=Qry.FieldAsInteger("coupon_no");
    operFlt.Init(Qry);
    return *this;
}

TGrpMktFlight& TGrpMktFlight::fromDB(TQuery &Qry)
{
  clear();
  airline=Qry.FieldAsString("mark_airline");
  flt_no=Qry.FieldAsInteger("mark_flt_no");
  suffix=Qry.FieldAsString("mark_suffix");
  scd_date_local=Qry.FieldAsDateTime("mark_scd");
  airp_dep=Qry.FieldAsString("mark_airp_dep");
  pr_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
  return *this;
}

bool TGrpMktFlight::getByGrpId(int grp_id)
{
  QParams QryParams;
  QryParams << QParam("grp_id", otInteger, grp_id);
  TCachedQuery Qry(
    "SELECT mark_trips.airline AS mark_airline, "
    "       mark_trips.flt_no AS mark_flt_no, "
    "       mark_trips.suffix AS mark_suffix, "
    "       mark_trips.scd AS mark_scd, "
    "       mark_trips.airp_dep AS mark_airp_dep, "
    "       pax_grp.pr_mark_norms "
    "FROM pax_grp, mark_trips "
    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id", QryParams);
  Qry.get().Execute();
  if(Qry.get().Eof) return false;
  fromDB(Qry.get());
  return true;
}

int SeparateTCkin(int grp_id,
                  TCkinSegmentSet upd_depend,
                  TCkinSegmentSet upd_tid,
                  int tid)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT tckin_id,seg_no FROM tckin_pax_grp WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return NoExists;

  int tckin_id=Qry.FieldAsInteger("tckin_id");
  int seg_no=Qry.FieldAsInteger("seg_no");

  if (upd_depend==cssNone) return tckin_id;

  ostringstream sql;
  string where_str;

  if (tid!=NoExists)
  {
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

  return tckin_id;
};

class TCkinIntegritySeg
{
  public:
    int seg_no;
    int grp_id;
    bool pr_depend;
    map<int/*pax_no=reg_no-first_reg_no*/, string/*refuse*/> pax;
    TCkinIntegritySeg(int vseg_no, int vgrp_id, bool vpr_depend):
      seg_no(vseg_no),
      grp_id(vgrp_id),
      pr_depend(vpr_depend) {};
};

void CheckTCkinIntegrity(const set<int> &tckin_ids, int tid)
{
  if (tckin_ids.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax.reg_no, pax.refuse, "
    "       tckin_pax_grp.seg_no, tckin_pax_grp.grp_id, "
    "       tckin_pax_grp.first_reg_no, tckin_pax_grp.pr_depend "
    "FROM pax, tckin_pax_grp "
    "WHERE tckin_pax_grp.grp_id=pax.grp_id AND tckin_id=:tckin_id "
    "ORDER BY seg_no ";
  Qry.DeclareVariable("tckin_id", otInteger);

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText=
    "BEGIN "
    "  UPDATE tckin_pax_grp SET pr_depend=0 WHERE grp_id=:grp_id AND pr_depend<>0; "
    "  IF SQL%FOUND AND :tid IS NOT NULL THEN "
    "    UPDATE pax_grp SET tid=:tid WHERE grp_id=:grp_id; "
    "  END IF; "
    "END;";
  UpdQry.DeclareVariable("grp_id", otInteger);
  if (tid!=NoExists)
    UpdQry.CreateVariable("tid", otInteger, tid);
  else
    UpdQry.CreateVariable("tid", otInteger, FNull);

  for(set<int>::const_iterator tckin_id=tckin_ids.begin(); tckin_id!=tckin_ids.end(); ++tckin_id)
  {
    if (*tckin_id==NoExists) continue;

    Qry.SetVariable("tckin_id", *tckin_id);
    Qry.Execute();
    if (Qry.Eof) continue;

    int prior_seg_no=NoExists;
    int first_reg_no=NoExists;
    map<int/*seg_no*/, TCkinIntegritySeg> segs;
    map<int/*seg_no*/, TCkinIntegritySeg>::iterator iSeg=segs.end();
    for(;!Qry.Eof;Qry.Next())
    {
      int curr_seg_no=Qry.FieldAsInteger("seg_no");
      if (prior_seg_no==NoExists || prior_seg_no!=curr_seg_no)
      {
        iSeg=segs.insert(make_pair(curr_seg_no,
                                   TCkinIntegritySeg(curr_seg_no,
                                                     Qry.FieldAsInteger("grp_id"),
                                                     Qry.FieldAsInteger("pr_depend")!=0))).first;
        first_reg_no=Qry.FieldIsNULL("first_reg_no")?NoExists:Qry.FieldAsInteger("first_reg_no");
        prior_seg_no=curr_seg_no;
      };

      if (first_reg_no!=NoExists && iSeg!=segs.end())
      {
        iSeg->second.pax[Qry.FieldAsInteger("reg_no")-first_reg_no]=Qry.FieldAsString("refuse");
      };
    };

    map<int/*seg_no*/, TCkinIntegritySeg>::const_iterator iPriorSeg=segs.end();
    for(map<int/*seg_no*/, TCkinIntegritySeg>::const_iterator iCurrSeg=segs.begin(); iCurrSeg!=segs.end(); ++iCurrSeg)
    {
      /*
      ProgTrace(TRACE5,"CheckTCkinIntegrity: tckin_id=%d seg_no=%d grp_id=%d pr_depend=%d",
                       *tckin_id,
                       iCurrSeg->second.seg_no,
                       iCurrSeg->second.grp_id,
                       (int)iCurrSeg->second.pr_depend);
      for(map<int, string>::const_iterator p=iCurrSeg->second.pax.begin();p!=iCurrSeg->second.pax.end();++p)
      {
        ProgTrace(TRACE5,"CheckTCkinIntegrity: tckin_id=%d seg_no=%d pax_no=%d refuse=%s",
                         *tckin_id,
                         iCurrSeg->second.seg_no,
                         p->first,
                         p->second.c_str());
      };
      */
      if (iCurrSeg->second.pr_depend)
      {

        if (iPriorSeg==segs.end() || //первый сегмент
            iPriorSeg->second.seg_no+1!=iCurrSeg->second.seg_no ||
            iPriorSeg->second.pax!=iCurrSeg->second.pax)
        {
          UpdQry.SetVariable("grp_id", iCurrSeg->second.grp_id);
          UpdQry.Execute();
        };
      };
      iPriorSeg=iCurrSeg;
    };
  };
};

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
    "SELECT first_xname, first_yname, last_xname, last_yname "
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
  Qry->SetVariable( "pax_id", pax_id );
  Qry->Execute();
  TSeatRanges ranges;
  for(;!Qry->Eof;Qry->Next())
  {
    ranges.push_back(TSeatRange(TSeat(Qry->FieldAsString("first_yname"),
                                      Qry->FieldAsString("first_xname")),
                                TSeat(Qry->FieldAsString("last_yname"),
                                      Qry->FieldAsString("last_xname"))));
  };
  return GetSeatRangeView(ranges, format, pr_lat_seat);
}

TPaxSeats::~TPaxSeats()
{
    delete Qry;
}

void GetMktFlights(const TTripInfo &operFltInfo, TSimpleMktFlights &simpleMktFlights)
{
  simpleMktFlights.clear();
  if (operFltInfo.scd_out==NoExists) return;
  TDateTime scd_local=UTCToLocal(operFltInfo.scd_out, AirpTZRegion(operFltInfo.airp));

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline_mark, flt_no_mark, suffix_mark "
    "FROM codeshare_sets "
    "WHERE airline_oper=:airline AND flt_no_oper=:flt_no AND airp_dep=:airp_dep AND "
    "      (suffix_oper IS NULL AND :suffix IS NULL OR suffix_oper=:suffix) AND "
    "      first_date<=:scd_local AND "
    "      (last_date IS NULL OR last_date>:scd_local) AND "
    "      (days IS NULL OR INSTR(days,TO_CHAR(:wday))<>0) AND pr_del=0 "
    "ORDER BY flt_no_mark, suffix_mark, airline_mark";
  Qry.CreateVariable("airline",otString,operFltInfo.airline);
  Qry.CreateVariable("flt_no",otInteger,operFltInfo.flt_no);
  Qry.CreateVariable("suffix",otString,operFltInfo.suffix);
  Qry.CreateVariable("airp_dep",otString,operFltInfo.airp);
  Qry.CreateVariable("scd_local",otDate,scd_local);
  Qry.CreateVariable("wday",otInteger,DayOfWeek(scd_local));
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    simpleMktFlights.emplace_back(Qry.FieldAsString("airline_mark"),
                                  Qry.FieldAsInteger("flt_no_mark"),
                                  Qry.FieldAsString("suffix_mark"));
}

void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo, bool return_scd_utc)
{
  markFltInfo.clear();
  if (operFltInfo.scd_out==NoExists) return;
  TDateTime scd_local=UTCToLocal(operFltInfo.scd_out, AirpTZRegion(operFltInfo.airp));

  TSimpleMktFlights simpleMktFlights;
  GetMktFlights(operFltInfo, simpleMktFlights);

  for(const TSimpleMktFlight& simpleMktFlight : simpleMktFlights)
  {
    TTripInfo flt;
    flt.airline=simpleMktFlight.airline;
    flt.flt_no=simpleMktFlight.flt_no;
    flt.suffix=simpleMktFlight.suffix;
    if (return_scd_utc)
      flt.scd_out=operFltInfo.scd_out;
    else
      flt.scd_out=scd_local;
    flt.airp=operFltInfo.airp;
    markFltInfo.push_back(flt);
  }
}

TCodeShareSets::TCodeShareSets()
{
  clear();
  Qry = new TQuery( &OraSession );
  Qry->SQLText=
    "SELECT pr_mark_norms, pr_mark_bp, pr_mark_rpt "
    "FROM codeshare_sets "
    "WHERE airline_oper=:airline_oper AND flt_no_oper=:flt_no_oper AND airp_dep=:airp_dep AND "
    "      airline_mark=:airline_mark AND flt_no_mark=:flt_no_mark AND "
    "      (suffix_oper IS NULL AND :suffix_oper IS NULL OR suffix_oper=:suffix_oper) AND "
    "      (suffix_mark IS NULL AND :suffix_mark IS NULL OR suffix_mark=:suffix_mark) AND "
    "      first_date<=:scd_local AND "
    "      (last_date IS NULL OR last_date>:scd_local) AND "
    "      (days IS NULL OR INSTR(days,TO_CHAR(:wday))<>0) AND pr_del=0 ";
  Qry->DeclareVariable("airline_oper",otString);
  Qry->DeclareVariable("flt_no_oper",otInteger);
  Qry->DeclareVariable("suffix_oper",otString);
  Qry->DeclareVariable("airp_dep",otString);
  Qry->DeclareVariable("airline_mark",otString);
  Qry->DeclareVariable("flt_no_mark",otInteger);
  Qry->DeclareVariable("suffix_mark",otString);
  Qry->DeclareVariable("scd_local",otDate);
  Qry->DeclareVariable("wday",otInteger);
}

TCodeShareSets::~TCodeShareSets()
{
  delete Qry;
}

void TCodeShareSets::get(const TTripInfo &operFlt, const TTripInfo &markFlt, bool is_local_scd_out)
{
  clear();
  if (operFlt.airline==markFlt.airline &&
      operFlt.flt_no==markFlt.flt_no &&
      operFlt.suffix==markFlt.suffix) return;

  TDateTime scd_local=is_local_scd_out?operFlt.scd_out:UTCToLocal(operFlt.scd_out, AirpTZRegion(operFlt.airp));

  Qry->SetVariable("airline_oper",operFlt.airline);
  Qry->SetVariable("flt_no_oper",operFlt.flt_no);
  Qry->SetVariable("suffix_oper",operFlt.suffix);
  Qry->SetVariable("airp_dep",operFlt.airp);
  Qry->SetVariable("airline_mark",markFlt.airline);
  Qry->SetVariable("flt_no_mark",markFlt.flt_no);
  Qry->SetVariable("suffix_mark",markFlt.suffix);
  Qry->SetVariable("scd_local",scd_local);
  Qry->SetVariable("wday",DayOfWeek(scd_local));
  Qry->Execute();
  if (!Qry->Eof)
  {
    pr_mark_norms=Qry->FieldAsInteger("pr_mark_norms")!=0;
    pr_mark_bp=Qry->FieldAsInteger("pr_mark_bp")!=0;
    pr_mark_rpt=Qry->FieldAsInteger("pr_mark_rpt")!=0;
  }
}

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
}

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
}

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
    string country_dep = ((const TCitiesRow&)basecities.get_row( "code", ((const TAirpsRow&)baseairps.get_row( "code", airp_dep, true )).city)).country;
    string country_arv = ((const TCitiesRow&)basecities.get_row( "code", ((const TAirpsRow&)baseairps.get_row( "code", airp_arv, true )).city)).country;
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
    if (!route.GetRouteAfter(NoExists,point_dep,trtWithCurrent,trtNotCancelled))
        throw Exception("TTripRoute::GetRouteAfter: flight not found for point_dep %d", point_dep);
    for(TTripRoute::iterator iv = route.begin(); iv != route.end(); iv++)
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

string GetRouteAfterStr(TDateTime part_key,
                        int point_id,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2,
                        const string &lang,
                        bool show_city_name,
                        const string &separator)
{
  ostringstream result;
  TTripRoute route;
  route.GetRouteAfter(part_key, point_id, route_type1, trtWithCancelled);

  std::string language = lang;

  if (lang.empty())
      language = TReqInfo::Instance()->desk.lang;

  for(TTripRoute::iterator r = route.begin(); r != route.end(); r++)
  {
    if (r->point_id!=point_id && route_type2==trtNotCancelled && r->pr_cancel) continue;
    if (!result.str().empty()) result << separator;
    string city;
    if (show_city_name)
    try
    {
      city=base_tables.get("airps").get_row("code",r->airp).AsString("city");
    }
    catch (const EBaseTableError&) {};

    if (!city.empty())
      result << ElemIdToPrefferedElem(etCity, city, efmtNameLong, language, true)
             << "(" << ElemIdToPrefferedElem(etAirp, r->airp, efmtCodeNative, language, true) << ")";
    else
      result << ElemIdToPrefferedElem(etAirp, r->airp, efmtCodeNative, language, true);
  };

  return result.str();
};

bool is_sync_paxs( int point_id )
{
  TTripInfo tripInfo;
  if (!tripInfo.getByPointId(point_id, FlightProps(FlightProps::NotCancelled,
                                                   FlightProps::WithCheckIn))) return false;

  return MERIDIAN::is_sync_meridian( tripInfo ) ||
         is_sync_basel_pax( tripInfo ) ||
         is_sync_aodb_pax( tripInfo ) ||
         MQRABBIT_TRANSPORT::is_sync_exch_checkin_result_mqrabbit( tripInfo );
}

bool is_sync_flights( int point_id )
{
  TTripInfo tripInfo;
  if (!tripInfo.getByPointId(point_id, FlightProps(FlightProps::NotCancelled,
                                                   FlightProps::WithCheckIn))) return false;
  return MQRABBIT_TRANSPORT::is_sync_exch_flights_result_mqrabbit( tripInfo );
}

void update_pax_change( int point_id, int pax_id, int reg_no, const string &work_mode )
{
    TTripInfo tripInfo;
    if (tripInfo.getByPointId ( point_id ) )
        update_pax_change(tripInfo, pax_id, reg_no, work_mode);
}

void update_pax_change( const TTripInfo &fltInfo, int pax_id, int reg_no, const string &work_mode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
     "BEGIN "
     " UPDATE aodb_pax_change "
     "  SET point_id=:point_id, airline=:airline, airp=:airp, desk=:desk, client_type=NVL(:client_type,client_type), time=:time "
     " WHERE pax_id=:pax_id AND reg_no=:reg_no AND work_mode=:work_mode; "
     " IF SQL%NOTFOUND AND :client_type IS NOT NULL THEN "
     "  INSERT INTO aodb_pax_change(pax_id,reg_no,work_mode,point_id,desk,client_type,time,airline,airp) "
     "   VALUES(:pax_id,:reg_no,:work_mode,:point_id,:desk,:client_type,:time,:airline,:airp); "
     " END IF; "
     "END;";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.CreateVariable( "reg_no", otInteger, reg_no );
  Qry.CreateVariable( "work_mode", otString, work_mode );
  Qry.CreateVariable( "point_id", otInteger, fltInfo.point_id );
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "client_type", otString,  EncodeClientType(TReqInfo::Instance()->client_type) );
  Qry.CreateVariable( "time", otDate, NowUTC() );
  Qry.CreateVariable( "airline", otString, fltInfo.airline );
  Qry.CreateVariable( "airp", otString, fltInfo.airp );
  Qry.Execute();
}

void update_flights_change( int point_id )
{
  TTripInfo tripInfo;
  if ( !tripInfo.getByPointId ( point_id ) ) {
    return;
  }
  TQuery Qry( &OraSession );
  Qry.SQLText =
     "BEGIN "
     " UPDATE exch_flights "
     "  SET time=:time, tid=exch_flights__seq.nextval "
     " WHERE point_id=:point_id; "
     " IF SQL%NOTFOUND THEN "
     "  INSERT INTO exch_flights(point_id,time,tid) "
     "   SELECT :point_id,:time,exch_flights__seq.nextval FROM DUAL; "
     " END IF; "
     "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "time", otDate, NowUTC() );
  Qry.Execute();
}


const map<string, TPaxNameTitle>& pax_name_titles()
{
  static map<string, TPaxNameTitle> titles;
  if (titles.empty())
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText = "SELECT title, is_female FROM pax_name_titles";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TPaxNameTitle info;
      info.title=Qry.FieldAsString("title");
      info.is_female=Qry.FieldAsInteger("is_female")!=0;
      titles.insert( make_pair(info.title, info) );
    };
    ProgTrace(TRACE5, "titles loaded");
  };
  return titles;
};

bool GetPaxNameTitle(string &name, bool truncate, TPaxNameTitle &info)
{
  const map<string, TPaxNameTitle> &titles=pax_name_titles();

  info.clear();
  string name_tmp(name);
  RTrimString(name_tmp);
  for(map<string, TPaxNameTitle>::const_iterator i=titles.begin(); i!=titles.end(); ++i)
  {
    string::size_type pos=name_tmp.rfind(i->second.title);
    if (pos!=string::npos)
    {
      if (name_tmp.substr(pos)==i->second.title)
      {
        info=i->second;
        if (truncate)
        {
          name_tmp.erase(pos);
          RTrimString(name_tmp);
          name=name_tmp;
        };
        break;
      };
    };
  };
  return !info.empty();
};

string TruncNameTitles(const string &name)
{
  string name_tmp(name);
  TPaxNameTitle info;
  GetPaxNameTitle(name_tmp, true, info);
  return name_tmp;
};

string SeparateNames(string &names)
{
  string result;
  TrimString(names);
  string::size_type pos=names.find(' ');
  if ( pos != string::npos)
  {
    result=names.substr(pos);
    names=names.substr(0,pos);
  };
  TrimString(names);
  TrimString(result);
  return result;
};

const double PoundRatio = 0.454; //1 фунт = 0.454 грамма

int CalcWeightInKilos(int weight, std::string weight_unit)
{
  double result=ASTRA::NoExists;
  if ((weight_unit=="K" || weight_unit=="L") && weight!=ASTRA::NoExists)
  {
    if (weight_unit=="L")
      modf(weight*PoundRatio, &result);
    else
      result=weight;
  };
  return result;
};

string TCFG::str(const string &lang, const string &separator)
{
    std::string language = lang;
    if (lang.empty())
        language = TReqInfo::Instance()->desk.lang;
    ostringstream result;
    for(vector<TCFGItem>::iterator iv = begin(); iv != end(); ++iv) {
        if (!result.str().empty()) result << separator;
        result
            << ElemIdToPrefferedElem(etClass, iv->cls, efmtCodeNative, language, true)
            << iv->cfg;
    }
    return result.str();
}

void TCFG::param(LEvntPrms& params)
{
    PrmEnum prmenum("cls", "");
    for(vector<TCFGItem>::iterator iv = begin(); iv != end(); ++iv) {
        prmenum.prms << PrmSmpl<std::string>("", " ");
        prmenum.prms << PrmElem<std::string>("", etClass, iv->cls) << PrmSmpl<int>("", iv->cfg);
    }
    params << prmenum;
}

void TCFG::get(int point_id, TDateTime part_key)
{
    clear();
    QParams QryParams;
    string SQLText;
    if (point_id == NoExists)
    {
        SQLText =
            "SELECT priority, code AS class, 0 AS cfg, 0 AS block, 0 AS prot "
            "FROM classes "
            "ORDER BY priority ";
    }
    else
    {
      QryParams << QParam("point_id", otInteger, point_id);
      if(part_key == NoExists)
          SQLText =
              "SELECT priority, class, cfg, block, prot "
              "FROM trip_classes, classes "
              "WHERE trip_classes.class=classes.code AND "
              "      point_id=:point_id AND cfg>0 "
              "ORDER BY priority ";
      else
      {
          SQLText =
              "SELECT priority, class, cfg, block, prot "
              "FROM arx_trip_classes, classes "
              "WHERE arx_trip_classes.class=classes.code(+) AND "
              "      part_key=:part_key AND point_id=:point_id AND cfg>0 "
              "ORDER BY priority";
          QryParams << QParam("part_key", otDate, part_key);
      };
    };
    TCachedQuery Qry(SQLText, QryParams);
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next()) {
        TCFGItem item;
        item.priority = Qry.get().FieldAsInteger("priority");
        item.cls = Qry.get().FieldAsString("class");
        item.cfg = Qry.get().FieldAsInteger("cfg");
        item.block = Qry.get().FieldAsInteger("block");
        item.prot = Qry.get().FieldAsInteger("prot");
        push_back(item);
    }
}

void SearchMktFlt(const TSearchFltInfo &filter, set<int/*mark_trips.point_id*/> &point_ids)
{
  if ( filter.scd_out_in_utc ) {
    throw Exception("%s: filter.scd_out_in_utc=true not supported", __FUNCTION__);
  }
  point_ids.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT DISTINCT point_id FROM mark_trips "
    " WHERE airline=:airline AND flt_no=:flt_no AND "
    "       (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
    "       scd=:scd AND airp_dep=:airp_dep";
  Qry.CreateVariable( "airline", otString, filter.airline );
  Qry.CreateVariable( "flt_no", otInteger, filter.flt_no );
  Qry.CreateVariable( "suffix", otString, filter.suffix );
  Qry.CreateVariable( "scd", otDate, filter.scd_out );
  Qry.CreateVariable( "airp_dep", otString, filter.airp_dep );
  Qry.Execute();
  for ( ; !Qry.Eof; Qry.Next() ) {
    point_ids.insert( Qry.FieldAsInteger( "point_id") );
  }
}

void SearchFlt(const TSearchFltInfo &filter, list<TAdvTripInfo> &flts)
{
  flts.clear();

  QParams QryParams;
  QryParams << QParam("airline", otString, filter.airline)
            << (filter.flt_no!=NoExists?QParam("flt_no", otInteger, (int)filter.flt_no):
                                        QParam("flt_no", otInteger, FNull))
            << QParam("suffix", otString, filter.suffix)
            << QParam("airp_dep", otString, filter.airp_dep)
            << (filter.scd_out!=NoExists?QParam("scd", otDate, filter.scd_out):
                                         QParam("scd", otDate, FNull))
            << QParam("only_with_reg", otInteger, (int)filter.only_with_reg);

  ostringstream sql;
  sql <<
    "SELECT " << TAdvTripInfo::selectedFields() << " FROM points \n";

  if (!filter.scd_out_in_utc)
  {
    //перевод UTCToLocal(points.scd)
    sql <<
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND \n"
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND \n"
      "      (scd_out >= TO_DATE(:scd)-1 AND scd_out < TO_DATE(:scd)+2 \n ";
    if(filter.dep_date_flags.isFlag(ddtEST))
    sql <<
      "      or est_out >= TO_DATE(:scd)-1 AND est_out < TO_DATE(:scd)+2 \n ";
    if(filter.dep_date_flags.isFlag(ddtACT))
    sql <<
      "      or act_out >= TO_DATE(:scd)-1 AND act_out < TO_DATE(:scd)+2 \n ";
    sql <<
      "      ) AND \n"
      "      pr_del>=0 AND (:only_with_reg=0 OR pr_reg<>0) \n";
  }
  else
  {
    sql <<
      "WHERE airline=:airline AND flt_no=:flt_no AND \n"
      "      (:airp_dep IS NULL OR airp=:airp_dep) AND \n"
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND \n"
      "      (scd_out >= TO_DATE(:scd) AND scd_out < TO_DATE(:scd)+1 \n";
    if(filter.dep_date_flags.isFlag(ddtEST))
    sql <<
      "      or est_out >= TO_DATE(:scd) AND est_out < TO_DATE(:scd)+1 \n";
    if(filter.dep_date_flags.isFlag(ddtACT))
    sql <<
      "      or act_out >= TO_DATE(:scd) AND act_out < TO_DATE(:scd)+1 \n";
    sql <<
      "      ) AND \n"
      "      pr_del>=0 AND (:only_with_reg=0 OR pr_reg<>0) \n";
  };

  sql << " " << filter.additional_where;

  TCachedQuery PointsQry(sql.str(), QryParams);
  PointsQry.get().Execute();
  for(;!PointsQry.get().Eof;PointsQry.get().Next())
  {
    TAdvTripInfo flt(PointsQry.get());
    if (!filter.scd_out_in_utc)
    {
      TDateTime scd=flt.scd_out;
      string tz_region=AirpTZRegion(flt.airp,false);
      if (tz_region.empty()) continue;
      scd=UTCToLocal(scd,tz_region);
      modf(scd,&scd);
      if (scd!=filter.scd_out) continue;
    };
    if(filter.before_add(flt))
        flts.push_back(flt);
  };
  filter.before_exit(flts);
};

TDateTime getTimeTravel(const string &craft, const string &airp, const string &airp_last)
{
    TCachedQuery Qry(
    "SELECT time_out_in "
    "FROM place_calc "
    "WHERE bc=:bc and cod_out=:airp and cod_in=:airp_last ",
    QParams()
    << QParam("bc", otString, craft)
    << QParam("airp", otString, airp)
    << QParam("airp_last", otString, airp_last)
    );
  Qry.get().Execute();
  TDateTime result = NoExists;
  if(!Qry.get().Eof && !Qry.get().FieldIsNULL("time_out_in"))
      result = Qry.get().FieldAsDateTime("time_out_in");
  return result;
}

const char* units[] = {"B", "K", "M", "G", "T", "P", "E", "Z", "Y"};
const size_t units_size = sizeof(units) / sizeof(units[0]);

double getFileSizeDouble(const string &str)
{
    double Result = NoExists;
    if(
            not str.empty() and
            StrToFloat( str.substr(0, str.size() - 1).c_str(), Result ) != EOF
      ) {
        char c = *(upperc(str).end() - 1);
        size_t i = 0;
        for(; i < units_size; i++)
            if(c == units[i][0]) break;
        Result = Result * pow(1024, i);
    }
    return Result;
}

string getFileSizeStr(double size)
{
    if(size == 0) return "0";
    if(size == NoExists) return string();

    ostringstream result;

    for(int i = 8; i >= 0; i--) {
        double val = size / pow(1024, i);
        if(round(val) != 0) {
            double integral;
            int fract = (int)(modf(val, &integral) * 100);
            int precision = ((fract == 0 or i == 1) ? 0 : 2);
            ostringstream adjusted, rounded;
            adjusted << fixed << setprecision(precision) << val;
            rounded << fixed << setprecision(precision) << round(val);
            if(adjusted.str() == rounded.str())
                precision = 0;
            if(precision != 0) {
                ostringstream tmp1, tmp2;
                tmp1 << fixed << setprecision(precision) << val;
                tmp2 << fixed << setprecision(1) << val << '0';
                if(tmp1.str() == tmp2.str())
                    result << fixed << setprecision(1) << val;
                else
                    result << tmp1.str();
            } else
                result << fixed << setprecision(precision) << val;
            result << units[i];
            break;
        }
    }
    return result.str();
}

AstraLocale::LexemaData GetLexemeDataWithRegNo(const AstraLocale::LexemaData &data, int reg_no)
{
  AstraLocale::LexemaData result;
  result.lexema_id="WRAP.REG_NO";
  result.lparams << AstraLocale::LParam("reg_no", reg_no)
                 << AstraLocale::LParam("text",data);
  return result;
}

AstraLocale::LexemaData GetLexemeDataWithFlight(const AstraLocale::LexemaData &data, const TTripInfo &fltInfo)
{
  AstraLocale::LexemaData result;
  result.lexema_id="WRAP.FLIGHT";
  result.lparams << AstraLocale::LParam("flight",GetTripName(fltInfo,ecCkin,true,false))
                 << AstraLocale::LParam("text",data);
  return result;
}

TInfantAdults::TInfantAdults(TQuery &Qry)
{
    fromDB(Qry);
}

void TInfantAdults::fromDB(TQuery &Qry)
{
    clear();
    grp_id = Qry.FieldAsInteger("grp_id");
    pax_id = Qry.FieldAsInteger("pax_id");
    reg_no = Qry.FieldAsInteger("reg_no");
    surname = Qry.FieldAsString("surname");
    if(Qry.GetFieldIndex("crs_inf_pax_id") >= 0)
        parent_pax_id = Qry.FieldAsInteger("crs_inf_pax_id");
}

void TInfantAdults::clear()
{
   grp_id = NoExists;
   pax_id = NoExists;
   reg_no = NoExists;
   surname.clear();
   parent_pax_id = NoExists;
   temp_parent_id = NoExists;
}

void TAdvTripRoute::getRouteBetween(int point_dep, const string& airp_arv)
{
  clear();
  GetRouteAfter(NoExists, point_dep, trtWithCurrent, trtNotCancelled);
  TAdvTripRoute::iterator i=begin();
  for(; i!=end(); ++i)
    if (i->airp==airp_arv) break;
  if (i!=end())
    erase(++i, end());
  else
   clear();
}
