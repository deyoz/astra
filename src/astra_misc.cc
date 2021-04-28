#include "astra_misc.h"
#include <string>
#include <vector>
#include <set>
#include "astra_date_time.h"
#include "exceptions.h"
#include "oralib.h"
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
#include <serverlib/cursctl.h>

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace ASTRA;
using namespace ASTRA::date_time;

std::optional<TTripInfo> getPointInfo(const PointId_t point_dep)
{
    TTripInfo point_info;
    if (!point_info.getByPointId(point_dep.get())) {
        return std::nullopt;
    }
    return point_info;
}

void checkRouteSuffix(const TAdvTripRoute &route)
{
    if (!route.front().suffix_out.empty()) {
        const TTripSuffixesRow &suffixRow = (const TTripSuffixesRow&)base_tables.get("trip_suffixes").
                get_row("code", route.front().suffix_out);
        if (suffixRow.code_lat.empty()) {
            throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
        }
    }
}

TAdvTripRoute getTransitRoute(const TPaxSegmentPair& flight)
{
    TAdvTripRoute route;
    LogTrace(TRACE5)<< __FUNCTION__ << " point: " << flight.point_dep << " airp_arv: "<< flight.airp_arv;
    if(flight.airp_arv.empty()) {
        route.GetRouteAfter(NoExists, flight.point_dep, trtWithCurrent, trtNotCancelled);
    } else {
        route.getRouteBetween(flight);
    }
//    if(route.empty()) {
//        throw Exception("Empty route, point_id %d", flight.point_dep);
//    }
    if(!route.empty()) {
        checkRouteSuffix(route);
    }
    return route;
}

std::vector<TPaxSegmentPair> transitLegs(const TAdvTripRoute& route)
{
    std::vector<TPaxSegmentPair> res;
    if(route.size() < 2) {
        LogTrace(TRACE5) << " Route can't be processed! Size TAdvTripRoute < 2";
        return res;
    }
    int point_dep = route.front().point_id;
    for(size_t i = 1; i < route.size(); i++) {
        std::string airp_arv = route[i].airp;
        res.emplace_back(point_dep, airp_arv);
        point_dep = route[i].point_id;
    }
    return res;
}

std::vector<std::string> segAirps(const TPaxSegmentPair & flight)
{
    std::vector<std::string> res;
    TAdvTripRoute route = getTransitRoute(flight);
    for(const auto &item : route) {
        res.push_back(item.airp);
    }
    return res;
}

std::vector<int> segPoints(const TPaxSegmentPair & flight)
{
    std::vector<int> res;
    TAdvTripRoute route = getTransitRoute(flight);
    for(const auto &item : route) {
        res.push_back(item.point_id);
    }
    return res;
}

static bool match(int pr_del, int pr_reg, const FlightProps& props)
{
  if (props.cancellation()==FlightProps::NotCancelled && pr_del!=0) return false;
  if (props.checkin_ability()==FlightProps::WithCheckIn && pr_reg==0) return false;
  return true;
}

void TTripInfo::fromArxPoint(const dbo::Arx_Points & arx_point)
{
    airline=arx_point.airline;
    flt_no = dbo::isNull(arx_point.flt_no) ? 0 : arx_point.flt_no;
    suffix=arx_point.suffix;
    airp=arx_point.airp;
    craft=arx_point.craft;
    bort=arx_point.bort;
    trip_type=arx_point.trip_type;
    scd_out = dbo::isNull(arx_point.scd_out) ? ASTRA::NoExists:
                                               BoostToDateTime(arx_point.scd_out);
    est_out = dbo::isNull(arx_point.est_out) ? ASTRA::NoExists:
                                         BoostToDateTime(arx_point.est_out);
    act_out = dbo::isNull(arx_point.act_out) ? ASTRA::NoExists:
                                         BoostToDateTime(arx_point.act_out);
    pr_del = arx_point.pr_del;
    pr_reg = arx_point.pr_reg != 0;
    airline_fmt = static_cast<TElemFmt>(arx_point.airline_fmt);
    suffix_fmt = static_cast<TElemFmt>(arx_point.suffix_fmt);
    airp_fmt = static_cast<TElemFmt>(arx_point.airp_fmt);
    craft_fmt = static_cast<TElemFmt>(arx_point.craft_fmt);
    point_id = arx_point.point_id;
};

bool TTripInfo::getByPointId ( const TDateTime part_key,
                               const int point_id,
                               const FlightProps &props )
{
    if (part_key==NoExists)
    {
        return getByPointId(point_id, props);
    }
    dbo::Session session;
    std::optional<dbo::Arx_Points> arx_point = session.query<dbo::Arx_Points>()
            .where(" part_key=:part_key AND point_id=:point_id AND pr_del>=0")
            .setBind({{":point_id",point_id}, {":part_key", DateTimeToBoost(part_key)}});
    if(!arx_point) {
        LogTrace5 << " not found arx_point by point_id: " << point_id << " part_key: " << DateTimeToBoost(part_key);
        return false;
    }
    if (!::match(arx_point->pr_del, arx_point->pr_reg, props)) return false;
    fromArxPoint(*arx_point);
    return true;
}

bool TTripInfo::getByPointId ( const int point_id,
                               const FlightProps &props )
{
    TQuery Qry( &OraSession );
    Qry.SQLText =
      "SELECT " + selectedFields() +
      "FROM points "
      "WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();

    if ( Qry.Eof ) return false;
    if (!match(Qry, props)) return false;

    init( Qry );
    return true;
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

std::optional<PointId_t> getPointIdByPaxId(const PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
    TCachedQuery Qry(
            "select point_dep from pax_grp, pax "
            "where "
            "   pax.pax_id = :pax_id and "
            "   pax.grp_id = pax_grp.grp_id ",
            QParams() << QParam("pax_id", otInteger, pax_id.get()));
    Qry.get().Execute();
    if(Qry.get().Eof) return {};
    LogTrace(TRACE6) << __func__
                     << ": result=" << Qry.get().FieldAsInteger("point_dep");
    return PointId_t(Qry.get().FieldAsInteger("point_dep"));
}

bool TTripInfo::getByPaxId ( const int pax_id )
{
    const std::optional<PointId_t> point_id = getPointIdByPaxId(PaxId_t(pax_id));
    if (!point_id) {
      return false;
    }
    return getByPointId( point_id->get() );
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

void getPointIdsSppByPointIdTlg(const PointIdTlg_t& point_id_tlg,
                                std::set<PointId_t>& point_ids_spp,
                                bool clear)
{
  if (clear) {
    point_ids_spp.clear();
  }

  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg = :point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id_tlg.get() );
  Qry.Execute();
  for( ; !Qry.Eof; Qry.Next() )
    point_ids_spp.insert(PointId_t(Qry.FieldAsInteger( "point_id_spp" )));
}

std::set<PointId_t> getPointIdsSppByPointIdTlg(const PointIdTlg_t& point_id_tlg)
{
  std::set<PointId_t> point_ids_spp;
  getPointIdsSppByPointIdTlg(point_id_tlg, point_ids_spp);
  return point_ids_spp;
}

void getTripsByPointIdTlg( const int point_id_tlg, TAdvTripInfoList &trips )
{
  trips.clear();

  std::set<PointId_t> point_ids_spp;
  getPointIdsSppByPointIdTlg(PointIdTlg_t(point_id_tlg), point_ids_spp);

  for(const PointId_t& id : point_ids_spp)
  {
    TAdvTripInfo info;
    if (info.getByPointId( id.get() ))
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

std::set<PointIdTlg_t> getPointIdTlgByPointIdsSpp(const PointId_t point_id_spp)
{
  std::set<PointIdTlg_t> result;
  TCachedQuery Qry("SELECT point_id_tlg "
                   "FROM tlg_binding "
                   "WHERE point_id_spp = :point_id ",
                   QParams() << QParam("point_id", otInteger, point_id_spp.get()));
  Qry.get().Execute();
  for(;!Qry.get().Eof;Qry.get().Next()) {
    result.insert(PointIdTlg_t(Qry.get().FieldAsInteger("point_id_tlg")));
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

std::optional<PointIdTlg_t> getPointIdTlgByPaxId(const PaxId_t pax_id, bool with_deleted)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  TCachedQuery Qry("SELECT point_id "
                   "FROM crs_pnr, crs_pax "
                   "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:pax_id AND "
                   "      crs_pnr.system='CRS' "
                   + std::string(with_deleted ? "" : "AND crs_pax.pr_del=0 "),
                   QParams() << QParam("pax_id", otInteger, pax_id.get()));
  Qry.get().Execute();
  if(Qry.get().Eof) return {};

  LogTrace(TRACE6) << __func__
                   << ": result=" << Qry.get().FieldAsInteger("point_id");
  return PointIdTlg_t(Qry.get().FieldAsInteger("point_id"));
}

void getTripsByCRSPaxId(const int pax_id, TAdvTripInfoList &trips)
{
  trips.clear();
  const std::optional<PointIdTlg_t> point_id_tlg = getPointIdTlgByPaxId(PaxId_t(pax_id), false /*with_deleted*/);
  if(!point_id_tlg) return;

  getTripsByPointIdTlg(point_id_tlg->get(), trips);
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

std::tuple<TDateTime, TDateTime, TDateTime> TTripInfo::get_times_in(const int &point_arv)
{
  TDateTime scd_in=ASTRA::NoExists;
  TDateTime est_in=ASTRA::NoExists;
  TDateTime act_in=ASTRA::NoExists;

  TCachedQuery PointsQry("SELECT scd_in, est_in, act_in FROM points WHERE point_id=:point_arv AND pr_del>=0",
                         QParams() << QParam("point_arv", otInteger, point_arv));
  PointsQry.get().Execute();
  if (!PointsQry.get().Eof)
  {
    if (!PointsQry.get().FieldIsNULL("scd_in"))
      scd_in=PointsQry.get().FieldAsDateTime("scd_in");
    if (!PointsQry.get().FieldIsNULL("est_in"))
      est_in=PointsQry.get().FieldAsDateTime("est_in");
    if (!PointsQry.get().FieldIsNULL("act_in"))
      act_in=PointsQry.get().FieldAsDateTime("act_in");
  }

  return {scd_in, est_in, act_in};
}

TDateTime TTripInfo::get_scd_in(const int &point_arv)
{
  return std::get<0>(get_times_in(point_arv));
}

TDateTime TTripInfo::act_est_scd_in(const int &point_arv)
{
  TDateTime scd_in, est_in, act_in;
  std::tie(scd_in, est_in, act_in) = get_times_in(point_arv);
  return act_in!=ASTRA::NoExists?act_in:
         est_in!=ASTRA::NoExists?est_in:
                                 scd_in;
}

std::tuple<TDateTime, TDateTime, TDateTime> TTripInfo::get_times_in(const std::string &airp_arv) const
{
  if (point_id!=ASTRA::NoExists)
  {
    TTripRoute route;
    route.GetRouteAfter( NoExists,
                         point_id,
                         trtNotCurrent, trtWithCancelled );
    for( TTripRoute::const_iterator iroute=route.begin(); iroute!=route.end(); ++iroute )
      if ( iroute->airp == airp_arv )
        return TTripInfo::get_times_in(iroute->point_id);
  }
  return {ASTRA::NoExists, ASTRA::NoExists, ASTRA::NoExists};
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

TGrpMktFlight TTripInfo::grpMktFlight() const
{
  TGrpMktFlight result;
  result.airline=airline;
  result.flt_no=flt_no;
  result.suffix=suffix;
  if (scd_out!=ASTRA::NoExists)
  {
    result.scd_date_local=UTCToLocal(scd_out,AirpTZRegion(airp));
    modf(result.scd_date_local,&result.scd_date_local);
  }
  result.airp_dep=airp;
  result.pr_mark_norms=false;
  return result;
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
    if (part_key==NoExists)
    {
        return getByPointId(point_id, props);
    }
    dbo::Session session;
    std::optional<dbo::Arx_Points> arx_point = session.query<dbo::Arx_Points>()
          .where(" part_key=:part_key AND point_id=:point_id AND pr_del>=0")
          .setBind({{":point_id",point_id}, {":part_key", DateTimeToBoost(part_key)}});
    if(!arx_point) return false;
    if (!::match(arx_point->pr_del, arx_point->pr_reg, props)) return false;

    fromArxPoint(*arx_point);
    point_num = arx_point->point_num;
    first_point = dbo::isNull(arx_point->first_point) ? ASTRA::NoExists : arx_point->first_point;
    pr_tranzit = arx_point->pr_tranzit != 0;

    return true;
}

bool TAdvTripInfo::getByPointId(const int point_id, const FlightProps &props)
{
    TQuery Qry( &OraSession );
    Qry.SQLText =
    "SELECT " + selectedFields() +
    "FROM points "
    "WHERE point_id=:point_id AND pr_del>=0 ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();

    if ( Qry.Eof ) return false;
    if (!match(Qry, props)) return false;

    Init( Qry );
    return true;
}

bool TAdvTripInfo::transitable(const PointId_t& pointId)
{
  auto cur = make_curs("SELECT 1 AS transitable "
                       "FROM points a, points b "
                       "WHERE b.move_id=a.move_id AND "
                       "      a.point_id=:point_id AND a.pr_del=0 AND "
                       "      b.point_num<a.point_num AND b.pr_del=0 AND "
                       "      rownum<2");

  bool result=false;

  cur.def(result)
     .bind(":point_id", pointId.get())
     .EXfet();

  return result;
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
}

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

std::string routeFilterConditions(bool after_current, TTripRouteType1 route_type1,
                                  TTripRouteType2 route_type2)
{
    ostringstream sql;
    if (after_current)
    {
      if (route_type1==trtWithCurrent)
        sql << " :first_point IN (first_point,point_id)"
            << "  AND point_num>=:point_num ";
      else
        sql << " first_point=:first_point"
               "  AND point_num>:point_num ";
    }
    else
    {
      sql << " :first_point IN (first_point,point_id) ";
      if (route_type1==trtWithCurrent)
        sql << "AND point_num<=:point_num ";
      else
        sql << "AND point_num<:point_num ";
    };

    if (route_type2==trtWithCancelled)
      sql << "AND pr_del>=0 ";
    else
      sql << "AND pr_del=0 ";
    sql << " ORDER BY point_num";
    return sql.str();
}

void TTripRoute::GetArxRoute(TDateTime part_key,
                          int point_id,
                          int point_num,
                          int first_point,
                          bool pr_tranzit,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
    std::map<std::string, dbo::bindedTypes> binds{{":part_key", DateTimeToBoost(part_key)},{":point_num", point_num}};
    if (!pr_tranzit && after_current){
        binds[":first_point"] = point_id;
    } else {
        if(first_point!=NoExists)
            binds[":first_point"] = first_point;
        else
            binds[":first_point"] = point_id;
    }

    dbo::Session session;
    std::vector<dbo::Arx_Points> arx_points = session.query<dbo::Arx_Points>()
            .where(" part_key=:part_key AND " + routeFilterConditions(after_current, route_type1, route_type2))
            .setBind(binds);

    for(const auto & point : arx_points) {
        TTripRouteItem item;
          item.part_key = BoostToDateTime(point.part_key);
          item.point_id = point.point_id;
          item.point_num = point.point_num;
          item.airp = point.airp;
          item.pr_cancel = point.pr_del!=0;
        push_back(item);
    }
}
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
    tst();
  clear();
  ostringstream sql;

  if (part_key!=NoExists)
  {
      GetArxRoute(part_key, point_id, point_num, first_point, pr_tranzit, after_current,
                  route_type1, route_type2);
  }
  else
    sql << "SELECT point_id,point_num,airp,pr_del "
        << "FROM points ";

  sql << " WHERE " << routeFilterConditions(after_current, route_type1, route_type2);

  LogTrace5 << " RESULT QUERY: " << sql.str();
  Qry.Clear();
  Qry.SQLText= sql.str().c_str();

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
    item.point_id=Qry.FieldAsInteger("point_id");
    item.point_num=Qry.FieldAsInteger("point_num");
    item.airp=Qry.FieldAsString("airp");
    item.pr_cancel=Qry.FieldAsInteger("pr_del")!=0;
    push_back(item);
  };
};

void TAdvTripRoute::GetArxRoute(TDateTime part_key,
                    int point_id,
                    int point_num,
                    int first_point,
                    bool pr_tranzit,
                    bool after_current,
                    TTripRouteType1 route_type1,
                    TTripRouteType2 route_type2)
{
  std::map<std::string, dbo::bindedTypes> binds{{":part_key", DateTimeToBoost(part_key)},{":point_num", point_num}};
  if (!pr_tranzit && after_current){
      binds[":first_point"] = point_id;
  } else {
      if(first_point!=NoExists)
          binds[":first_point"] = first_point;
      else
          binds[":first_point"] = point_id;
  }
  dbo::Session session;
  std::vector<dbo::Arx_Points> arx_points = session.query<dbo::Arx_Points>()
          .where(" part_key=:part_key AND " + routeFilterConditions(after_current, route_type1, route_type2))
          .setBind(binds);

  for(const auto & point : arx_points)
  {
    TAdvTripRouteItem item;
    item.part_key= BoostToDateTime(point.part_key);
    item.point_id = point.point_id;
    item.point_num = point.point_num;
    item.airp = point.airp;
    item.pr_cancel = point.pr_del!=0;
    if (!dbo::isNull(point.airline))
      item.airline_out = point.airline;
    if (!dbo::isNull(point.suffix))
      item.suffix_out = point.suffix;
    if (!dbo::isNull(point.flt_no))
      item.flt_num_out = point.flt_no;
    if (!dbo::isNull(point.scd_in))
      item.scd_in = BoostToDateTime(point.scd_in);
    if (!dbo::isNull(point.scd_out))
      item.scd_out = BoostToDateTime(point.scd_out);
    if (!dbo::isNull(point.act_out))
      item.act_out = BoostToDateTime(point.act_out);
    push_back(item);
  };
}

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
  if (part_key!=NoExists) {
      return GetArxRoute(part_key, point_id, point_num, first_point, pr_tranzit, after_current,
                            route_type1, route_type2);
  }
  else
    sql << "SELECT point_id,point_num,airp,pr_del, "
           "airline,suffix,flt_no,scd_in,scd_out, act_out "
        << "FROM points ";

  sql << " WHERE " << routeFilterConditions(after_current, route_type1, route_type2);

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
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
    item.point_id=Qry.FieldAsInteger("point_id");
    item.point_num=Qry.FieldAsInteger("point_num");
    item.airp=Qry.FieldAsString("airp");
    item.pr_cancel=Qry.FieldAsInteger("pr_del")!=0;
    if (!Qry.FieldIsNULL("airline"))
      item.airline_out=Qry.FieldAsString("airline");
    if (!Qry.FieldIsNULL("suffix"))
      item.suffix_out=Qry.FieldAsString("suffix");
    if (!Qry.FieldIsNULL("flt_no"))
      item.flt_num_out=Qry.FieldAsInteger("flt_no");
    if (!Qry.FieldIsNULL("scd_in"))
      item.scd_in=Qry.FieldAsDateTime("scd_in");
    if (!Qry.FieldIsNULL("scd_out"))
      item.scd_out=Qry.FieldAsDateTime("scd_out");
    if (!Qry.FieldIsNULL("act_out"))
      item.act_out=Qry.FieldAsDateTime("act_out");
    push_back(item);
  };
}

bool TTripRoute::GetArxRoute(TDateTime part_key,
                          int point_id,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
    tst();
    dbo::Session session;
    std::optional<dbo::Arx_Points> arx_point = session.query<dbo::Arx_Points>()
            .where(" part_key=:part_key AND point_id=:point_id AND pr_del>=0")
            .setBind({{":part_key", DateTimeToBoost(part_key)}, {":point_id", point_id}});

  if (!arx_point) return false;
  GetArxRoute(part_key,
           point_id,
           arx_point->point_num,
           arx_point->first_point,
           arx_point->pr_tranzit != 0,
           after_current,route_type1,route_type2);
  return true;
}

bool TTripRoute::GetRoute(TDateTime part_key,
                          int point_id,
                          bool after_current,
                          TTripRouteType1 route_type1,
                          TTripRouteType2 route_type2)
{
    if (part_key!=NoExists) {
        return GetArxRoute(part_key, point_id, after_current, route_type1, route_type2);
    }
  clear();
  TQuery Qry(&OraSession);
  ostringstream sql;
  sql << "SELECT point_num,first_point,pr_tranzit FROM points \n"
         "WHERE point_id=:point_id AND pr_del>=0";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
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

bool TAdvTripRoute::GetArxRoute(TDateTime part_key,
                    int point_id,
                    bool after_current,
                    TTripRouteType1 route_type1,
                    TTripRouteType2 route_type2)
{
    dbo::Session session;
    std::optional<dbo::Arx_Points> arx_point = session.query<dbo::Arx_Points>()
            .where(" part_key=:part_key AND point_id=:point_id AND pr_del>=0")
            .setBind({{":part_key", DateTimeToBoost(part_key)}, {":point_id", point_id}});

  if (!arx_point) return false;

  GetArxRoute(part_key,
           point_id,
           arx_point->point_num,
           arx_point->first_point,
           arx_point->pr_tranzit != 0,
           after_current,route_type1,route_type2);
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
  if (part_key!=NoExists) {
      return GetArxRoute(part_key, point_id, after_current, route_type1, route_type2);
  }
  else
    sql << "FROM points ";

  sql << "WHERE point_id=:point_id AND pr_del>=0";

  Qry.Clear();
  Qry.SQLText= sql.str().c_str();
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

TCkinRouteItem::TCkinRouteItem(TQuery &Qry)
{
  grp_num=Qry.FieldAsInteger("grp_num");
  seg_no=Qry.FieldAsInteger("seg_no");
  transit_num=Qry.FieldAsInteger("transit_num");
  pr_depend=Qry.FieldAsInteger("pr_depend")!=0;

  grp_id=Qry.FieldAsInteger("grp_id");
  point_dep=Qry.FieldAsInteger("point_dep");
  point_arv=Qry.FieldAsInteger("point_arv");
  airp_dep=Qry.FieldAsString("airp_dep");
  airp_arv=Qry.FieldAsString("airp_arv");
  status=DecodePaxStatus(Qry.FieldAsString("status"));
  operFlt.Init(Qry);
}

std::string TCkinRoute::getSelectSQL(const Direction direction,
                                     const std::string& subselect)
{
  ostringstream sql;
  sql << "SELECT " << TTripInfo::selectedFields("points") << ", \n"
         "       pax_grp.grp_id, pax_grp.point_dep, pax_grp.point_arv, \n"
         "       pax_grp.airp_dep, pax_grp.airp_arv, pax_grp.status, \n"
         "       tckin_pax_grp.grp_num, tckin_pax_grp.seg_no, \n"
         "       tckin_pax_grp.transit_num, tckin_pax_grp.pr_depend, \n"
         "       key.grp_num AS current_grp_num \n"
         "FROM points, pax_grp, tckin_pax_grp, \n"
         "(" + subselect + ") key \n"
         "WHERE points.point_id=pax_grp.point_dep AND \n"
         "      pax_grp.grp_id=tckin_pax_grp.grp_id AND \n"
         "      tckin_pax_grp.tckin_id=key.tckin_id \n";
  switch(direction)
  {
    case After:
      sql << "      AND tckin_pax_grp.grp_num>=key.grp_num \n";
      break;
    case Before:
      sql << "      AND tckin_pax_grp.grp_num<=key.grp_num \n";
      break;
    default:
      break;
  }
  sql << "ORDER BY grp_num ASC";

  return sql.str();
}

boost::optional<int> TCkinRoute::getRoute(const int tckin_id,
                                          const int grp_num,
                                          const Direction direction)
{
  boost::optional<int> current_grp_num;

  clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=getSelectSQL(direction,
                           "SELECT :tckin_id AS tckin_id, :grp_num AS grp_num FROM dual");
  Qry.CreateVariable("tckin_id", otInteger, tckin_id);
  Qry.CreateVariable("grp_num", otInteger, grp_num);
  Qry.Execute();
  if (!Qry.Eof) current_grp_num=Qry.FieldAsInteger("current_grp_num");
  for(; !Qry.Eof; Qry.Next())
    emplace_back(Qry);

  return current_grp_num;
}

boost::optional<int> TCkinRoute::getRoute(const GrpId_t& grpId,
                                          const Direction direction)
{
  boost::optional<int> current_grp_num;

  clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=getSelectSQL(direction,
                           "SELECT tckin_id, grp_num FROM tckin_pax_grp WHERE grp_id=:grp_id");
  Qry.CreateVariable("grp_id", otInteger, grpId.get());
  Qry.Execute();
  if (!Qry.Eof) current_grp_num=Qry.FieldAsInteger("current_grp_num");
  for(; !Qry.Eof; Qry.Next())
    emplace_back(Qry);

  return current_grp_num;
}

boost::optional<int> TCkinRoute::getRoute(const PaxId_t& paxId,
                                          const Direction direction)
{
  boost::optional<int> current_grp_num;

  clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=getSelectSQL(direction,
                           "SELECT tckin_pax_grp.tckin_id, tckin_pax_grp.grp_num "
                           "FROM pax, tckin_pax_grp "
                           "WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.pax_id=:pax_id");
  Qry.CreateVariable("pax_id", otInteger, paxId.get());
  Qry.Execute();
  if (!Qry.Eof) current_grp_num=Qry.FieldAsInteger("current_grp_num");
  for(; !Qry.Eof; Qry.Next())
    emplace_back(Qry);

  return current_grp_num;
}

void TCkinRoute::applyFilter(const boost::optional<int>& current_grp_num,
                             const Currentity currentity,
                             const Dependence dependence,
                             const GroupStatus groupStatus)
{
  if (!current_grp_num)
  {
    clear();
    return;
  }

  if (dependence==OnlyDependent)
  {
    TCkinRoute::iterator endDependent=begin();
    TCkinRoute::iterator beginDependent=endDependent;
    for(; endDependent!=end(); ++endDependent)
    {
      if (!endDependent->pr_depend) beginDependent=endDependent;

      if (endDependent->grp_num==current_grp_num.get()) break;
    }
    if (endDependent==end())
      beginDependent=endDependent; //не нашли current_grp_num;
    else
      ++endDependent;
    for(; endDependent!=end(); ++endDependent)
    {
      if (!endDependent->pr_depend) break;
    }

    erase(endDependent, end());
    erase(begin(), beginDependent);
  }

  if (groupStatus==WithoutTransit)
    erase(std::remove_if(begin(), end(), [](const auto& item) { return item.transit_num!=0; }), end());

  TCkinRoute::iterator curr=std::find_if(begin(), end(), [&](const auto& item) { return item.grp_num==current_grp_num.get(); });
  if (curr==end())
  {
    clear();
    return;
  }

  if (currentity==NotCurrent) erase(curr);
}

bool TCkinRoute::getRoute(const PaxId_t& paxId)
{
  const auto current_grp_num=getRoute(paxId, Full);
  applyFilter(current_grp_num, WithCurrent, IgnoreDependence, WithoutTransit);
  return current_grp_num;
}

bool TCkinRoute::getRoute(const GrpId_t& grpId,
                          const Currentity currentity,
                          const Dependence dependence,
                          const GroupStatus groupStatus)
{
  const auto current_grp_num=getRoute(grpId, Full);
  applyFilter(current_grp_num, currentity, dependence, groupStatus);
  return current_grp_num;
}

bool TCkinRoute::getRouteAfter(const GrpId_t& grpId,
                               const Currentity currentity,
                               const Dependence dependence,
                               const GroupStatus groupStatus)
{
  const auto current_grp_num=getRoute(grpId, After);
  applyFilter(current_grp_num, currentity, dependence, groupStatus);
  return current_grp_num;
}

bool TCkinRoute::getRouteBefore(const GrpId_t& grpId,
                                const Currentity currentity,
                                const Dependence dependence,
                                const GroupStatus groupStatus)
{
  const auto current_grp_num=getRoute(grpId, Before);
  applyFilter(current_grp_num, currentity, dependence, groupStatus);
  return current_grp_num;
};

boost::optional<TCkinRouteItem> TCkinRoute::getPriorGrp(const int tckin_id,
                                                        const int grp_num,
                                                        const Dependence dependence,
                                                        const GroupStatus groupStatus)
{
  TCkinRoute route;
  const auto current_grp_num=route.getRoute(tckin_id, grp_num, Before);
  route.applyFilter(current_grp_num, NotCurrent, dependence, groupStatus);
  if (route.empty()) return {};
  return route.back();
}

boost::optional<TCkinRouteItem> TCkinRoute::getPriorGrp(const GrpId_t& grpId,
                                                        const Dependence dependence,
                                                        const GroupStatus groupStatus)
{
  TCkinRoute route;
  const auto current_grp_num=route.getRoute(grpId, Before);
  route.applyFilter(current_grp_num, NotCurrent, dependence, groupStatus);
  if (route.empty()) return {};
  return route.back();
}

boost::optional<TCkinRouteItem> TCkinRoute::getNextGrp(const GrpId_t& grpId,
                                                       const Dependence dependence,
                                                       const GroupStatus groupStatus)
{
  TCkinRoute route;
  const auto current_grp_num=route.getRoute(grpId, After);
  route.applyFilter(current_grp_num, NotCurrent, dependence, groupStatus);
  if (route.empty()) return {};
  return route.front();
}

boost::optional<GrpId_t> TCkinRoute::toDB(const std::list<TCkinRouteInsertItem> &tckinGroups)
{
  if (tckinGroups.size()<=1) return {};

  auto cur=make_curs("INSERT INTO tckin_pax_grp "
                     "  (tckin_id, grp_num, seg_no, transit_num, grp_id, first_reg_no, pr_depend) "
                     "VALUES "
                     "  (:tckin_id, :grp_num, :seg_no, :transit_num, :grp_id, :first_reg_no, :pr_depend)");

  bool firstGrp=true;
  int grp_num=1;
  int seg_no=1;
  int transit_num=0;

  short null = -1, nnull = 0;
  for(const auto& i : tckinGroups)
  {
    if (!firstGrp && i.status!=psTransit)
    {
      seg_no++;
      transit_num=0;
    }

    cur.bind(":tckin_id", tckinGroups.front().grpId.get())
       .bind(":grp_num", grp_num++)
       .bind(":seg_no", seg_no)
       .bind(":transit_num", transit_num++)
       .bind(":grp_id", i.grpId.get())
       .bind(":first_reg_no", i.firstRegNo?i.firstRegNo.get().get():0, i.firstRegNo?&nnull:&null)
       .bind(":pr_depend", !firstGrp)
       .exec();

    firstGrp=false;
  }

  return tckinGroups.front().grpId;
}

std::string TCkinRoute::copySubselectSQL(const std::string& mainTable,
                                         const std::initializer_list<std::string>& otherTables,
                                         const bool forEachPassenger)
{
  std::ostringstream result;
  result << "FROM " << mainTable << ", ";
  for(const auto& tab : otherTables)
    result << tab << ", ";

  if (forEachPassenger)
    result << "(SELECT pax.pax_id, "
              "        tckin_pax_grp.grp_id, "
              "        tckin_pax_grp.tckin_id, "
              "        tckin_pax_grp.seg_no, "
              "        tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
              " FROM pax, tckin_pax_grp "
              " WHERE pax.grp_id=tckin_pax_grp.grp_id AND "
              "       tckin_pax_grp.transit_num=0 AND "
              "       pax.grp_id=:grp_id_src) src, "
              "(SELECT pax.pax_id, "
              "        tckin_pax_grp.grp_id, "
              "        tckin_pax_grp.tckin_id, "
              "        tckin_pax_grp.seg_no, "
              "        tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
              " FROM pax, tckin_pax_grp "
              " WHERE pax.grp_id=tckin_pax_grp.grp_id AND "
              "       tckin_pax_grp.transit_num=0 AND "
              "       pax.grp_id=:grp_id_dest) dest "
              "WHERE src.tckin_id=dest.tckin_id AND "
              "      src.distance=dest.distance AND "
              "      " << mainTable << ".pax_id=src.pax_id AND "
              "      " << mainTable << ".transfer_num+src.seg_no-dest.seg_no>=0 ";
  else
    result << "(SELECT tckin_pax_grp.grp_id, "
              "        tckin_pax_grp.tckin_id, "
              "        tckin_pax_grp.seg_no "
              " FROM tckin_pax_grp "
              " WHERE tckin_pax_grp.transit_num=0 AND "
              "       tckin_pax_grp.grp_id=:grp_id_src) src, "
              "(SELECT tckin_pax_grp.grp_id, "
              "        tckin_pax_grp.tckin_id, "
              "        tckin_pax_grp.seg_no "
              " FROM tckin_pax_grp "
              " WHERE tckin_pax_grp.transit_num=0 AND "
              "       tckin_pax_grp.grp_id=:grp_id_dest) dest "
              "WHERE src.tckin_id=dest.tckin_id AND "
              "      " << mainTable << ".grp_id=src.grp_id AND "
              "      " << mainTable << ".transfer_num+src.seg_no-dest.seg_no>=0 ";

  return result.str();
}

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

void TMktFlight::get(DB::TQuery &Qry, int id)
{
  clear();
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  if(Qry.Eof) {
    return;
  }
  const int point_id = Qry.FieldAsInteger("point_id");
  DB::TQuery QryTrips(PgOra::getROSession("TLG_TRIPS"), STDLOG);
  QryTrips.SQLText =
      "SELECT airline, flt_no, suffix, scd, airp_dep "
      "FROM tlg_trips "
      "WHERE point_id=:point_id ";
  QryTrips.CreateVariable("point_id",otInteger,point_id);
  QryTrips.Execute();
  if(QryTrips.Eof) {
    return;
  }

  const int pnr_id = Qry.FieldAsInteger("pnr_id");
  DB::TQuery QryMrkt(PgOra::getROSession("PNR_MARKET_FLT"), STDLOG);
  QryMrkt.SQLText =
      "SELECT airline, flt_no, suffix, subclass, "
      "local_date, airp_dep, airp_arv "
      "FROM pnr_market_flt "
      "WHERE pnr_id = :pnr_id ";
  QryMrkt.CreateVariable("pnr_id",otInteger,pnr_id);
  QryMrkt.Execute();

  if(QryMrkt.Eof) {
    airline = QryTrips.FieldAsString("airline");
    flt_no = QryTrips.FieldAsInteger("flt_no");
    suffix = QryTrips.FieldAsString("suffix");
    subcls = Qry.FieldAsString("tlg_subcls");
    TDateTime tmp_scd = QryTrips.FieldAsDateTime("scd");
    int Year, Month, Day;
    DecodeDate(tmp_scd, Year, Month, Day);
    scd_day_local = Day;
    EncodeDate(Year, Month, Day, scd_date_local);
    airp_dep = QryTrips.FieldAsString("airp_dep");
    airp_arv = Qry.FieldAsString("tlg_airp_arv");
  } else {
    airline = QryMrkt.FieldAsString("airline");
    flt_no = QryMrkt.FieldAsInteger("flt_no");
    suffix = QryMrkt.FieldAsString("suffix");
    subcls = QryMrkt.FieldAsString("subclass");
    scd_day_local = QryMrkt.FieldAsInteger("local_date");
    scd_date_local = DayToDate(scd_day_local,QryTrips.FieldAsDateTime("scd"),true);
    airp_dep = QryMrkt.FieldAsString("airp_dep");
    airp_arv = QryMrkt.FieldAsString("airp_arv");
  }
}

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
    DB::TQuery Qry(PgOra::getROSession("ORACLE"));
    Qry.SQLText =
        "select "
        "    crs_pnr.pnr_id, "
        "    crs_pnr.point_id, "
        "    crs_pnr.subclass tlg_subcls, "
        "    crs_pnr.airp_arv tlg_airp_arv "
        "from "
        "   crs_pax, "
        "   crs_pnr "
        "where "
        "    crs_pax.pax_id = :id and crs_pax.pr_del=0 and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id ";
    get(Qry, pax_id);
}

void TMktFlight::getByPnrId(int pnr_id)
{
    DB::TQuery Qry(PgOra::getROSession("CRS_PNR"));
    Qry.SQLText =
        "select "
        "    pnr_id, "
        "    point_id, "
        "    subclass tlg_subcls, "
        "    airp_arv tlg_airp_arv "
        "from "
        "   crs_pnr "
        "where "
        "    pnr_id = :id ";
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

static string whereSQL(const TCkinSegmentSet& setting)
{
  switch (setting)
  {
    case cssAllPrev:
      return " WHERE tckin_id=:tckin_id AND grp_num<:grp_num";
    case cssAllPrevCurr:
      return " WHERE tckin_id=:tckin_id AND grp_num<=:grp_num";
    case cssAllPrevCurrNext:
      return " WHERE tckin_id=:tckin_id AND grp_num<=:grp_num+1";
    case cssCurr:
      return " WHERE tckin_id=:tckin_id AND grp_num=:grp_num";
    default:
      return "";
  };
}

int SeparateTCkin(int grp_id,
                  TCkinSegmentSet upd_depend,
                  TCkinSegmentSet upd_tid,
                  int tid)
{
  int tckin_id=NoExists;
  int grp_num=NoExists;

  auto cur=make_curs("SELECT tckin_id, grp_num FROM tckin_pax_grp WHERE grp_id=:grp_id");

  cur.bind(":grp_id", grp_id)
     .def(tckin_id)
     .def(grp_num)
     .EXfet();

  if (cur.err() == NO_DATA_FOUND) return NoExists;

  if (upd_depend==cssNone) return tckin_id;

  if (tid!=NoExists && upd_tid!=cssNone)
  {
    auto upd=make_curs("UPDATE pax_grp SET tid=:tid "
                  "WHERE grp_id IN (SELECT grp_id FROM tckin_pax_grp " + whereSQL(upd_tid) + ") ");
    upd.bind(":tckin_id", tckin_id)
       .bind(":grp_num", grp_num)
       .bind(":tid", tid)
       .exec();
  }


  auto upd=make_curs("UPDATE tckin_pax_grp SET pr_depend=0 " + whereSQL(upd_depend));
  upd.bind(":tckin_id", tckin_id)
     .bind(":grp_num", grp_num)
     .exec();

  return tckin_id;
}

class TCkinIntegritySeg
{
  public:
    int grp_num;
    int grp_id;
    bool pr_depend;
    map<int/*pax_no=reg_no-first_reg_no*/, string/*refuse*/> pax;
    TCkinIntegritySeg(int vgrp_num, int vgrp_id, bool vpr_depend):
      grp_num(vgrp_num),
      grp_id(vgrp_id),
      pr_depend(vpr_depend) {}
};

void CheckTCkinIntegrity(const set<int> &tckin_ids, int tid)
{
  if (tckin_ids.empty()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax.reg_no, pax.refuse, "
    "       tckin_pax_grp.grp_num, tckin_pax_grp.grp_id, "
    "       tckin_pax_grp.first_reg_no, tckin_pax_grp.pr_depend "
    "FROM pax, tckin_pax_grp "
    "WHERE tckin_pax_grp.grp_id=pax.grp_id AND tckin_id=:tckin_id "
    "ORDER BY grp_num ";
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

    int prior_grp_num=NoExists;
    int first_reg_no=NoExists;
    map<int/*grp_num*/, TCkinIntegritySeg> segs;
    map<int/*grp_num*/, TCkinIntegritySeg>::iterator iSeg=segs.end();
    for(;!Qry.Eof;Qry.Next())
    {
      int curr_grp_num=Qry.FieldAsInteger("grp_num");
      if (prior_grp_num==NoExists || prior_grp_num!=curr_grp_num)
      {
        iSeg=segs.emplace(curr_grp_num,
                          TCkinIntegritySeg(curr_grp_num,
                                            Qry.FieldAsInteger("grp_id"),
                                            Qry.FieldAsInteger("pr_depend")!=0)).first;
        first_reg_no=Qry.FieldIsNULL("first_reg_no")?NoExists:Qry.FieldAsInteger("first_reg_no");
        prior_grp_num=curr_grp_num;
      };

      if (first_reg_no!=NoExists && iSeg!=segs.end())
      {
        iSeg->second.pax[Qry.FieldAsInteger("reg_no")-first_reg_no]=Qry.FieldAsString("refuse");
      };
    };

    map<int/*grp_num*/, TCkinIntegritySeg>::const_iterator iPriorSeg=segs.end();
    for(map<int/*grp_num*/, TCkinIntegritySeg>::const_iterator iCurrSeg=segs.begin(); iCurrSeg!=segs.end(); ++iCurrSeg)
    {
      /*
      ProgTrace(TRACE5,"CheckTCkinIntegrity: tckin_id=%d grp_num=%d grp_id=%d pr_depend=%d",
                       *tckin_id,
                       iCurrSeg->second.grp_num,
                       iCurrSeg->second.grp_id,
                       (int)iCurrSeg->second.pr_depend);
      for(map<int, string>::const_iterator p=iCurrSeg->second.pax.begin();p!=iCurrSeg->second.pax.end();++p)
      {
        ProgTrace(TRACE5,"CheckTCkinIntegrity: tckin_id=%d grp_num=%d pax_no=%d refuse=%s",
                         *tckin_id,
                         iCurrSeg->second.grp_num,
                         p->first,
                         p->second.c_str());
      };
      */
      if (iCurrSeg->second.pr_depend)
      {

        if (iPriorSeg==segs.end() || //первый сегмент
            iPriorSeg->second.grp_num+1!=iCurrSeg->second.grp_num ||
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

std::set<std::string> GetCrsList(const PointId_t& point_id)
{
  std::set<std::string> result;
  const std::set<PointIdTlg_t> point_id_tlgs = getPointIdTlgByPointIdsSpp(point_id);
  for (const PointIdTlg_t& point_id_tlg: point_id_tlgs) {
    DB::TQuery Qry(PgOra::getROSession("CRS_DATA_STAT"));
    Qry.SQLText=
        "SELECT DISTINCT crs FROM crs_data_stat "
        "WHERE point_id=:point_id_tlg";
    Qry.CreateVariable("point_id_tlg",otInteger,point_id_tlg.get());
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next()) {
      result.insert(Qry.FieldAsString("crs"));
    }
  }
  return result;
}

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
         is_sync_FileParamSets( tripInfo, FILE_AODB_OUT_TYPE ) ||
         is_sync_FileParamSets( tripInfo, MQRABBIT_TRANSPORT::MQRABBIT_CHECK_IN_RESULT_OUT_TYPE );
}

bool is_sync_flights( int point_id )
{
  TTripInfo tripInfo;
  if (!tripInfo.getByPointId(point_id, FlightProps(FlightProps::NotCancelled,
                                                   FlightProps::WithCheckIn))) return false;
  return is_sync_FileParamSets( tripInfo, MQRABBIT_TRANSPORT::MQRABBIT_FLIGHTS_RESULT_OUT_TYPE );
}

bool is_sync_FileParamSets( const TTripInfo &tripInfo, const std::string& syncType )
{
  DB::TQuery Qry(PgOra::getROSession("FILE_PARAM_SETS"));
  Qry.SQLText =
      "SELECT id FROM file_param_sets "
      " WHERE ( file_param_sets.airp IS NULL OR file_param_sets.airp=:airp ) AND "
      "       ( file_param_sets.airline IS NULL OR file_param_sets.airline=:airline ) AND "
      "       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=:flt_no ) AND "
      "       file_param_sets.type=:type AND pr_send=1 AND own_point_addr=:own_point_addr AND rownum<2";
  Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "type", otString, syncType );
  Qry.CreateVariable( "airline", otString, tripInfo.airline );
  Qry.CreateVariable( "airp", otString, tripInfo.airp );
  Qry.CreateVariable( "flt_no", otInteger, tripInfo.flt_no );
  Qry.Execute();
  return ( !Qry.Eof );
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

void TCFG::get_arx(int point_id, TDateTime part_key)
{
    dbo::Session session;
    std::vector<dbo::CLASSES> classes = session.query<dbo::CLASSES>().order_by("priority");
    for(const auto &cl : classes) {
        std::vector<dbo::ARX_TRIP_CLASSES> arx_trip_classes = session.query<dbo::ARX_TRIP_CLASSES>()
                .where("point_id = :point_id AND cfg>0 AND part_key=:part_key AND class=:code")
                .setBind({{"point_id", point_id}, {":part_key", DateTimeToBoost(part_key)},
                          {":code", cl.code}});
        for(const auto & atc : arx_trip_classes) {
            TCFGItem item;
            item.priority = cl.priority;
            item.cls =  atc.m_class;
            item.cfg = atc.cfg;
            item.block = atc.block;
            item.prot = atc.prot;
            push_back(item);
        }
    }
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
        if(part_key != NoExists) {
            return get_arx(point_id, part_key);
        }
        QryParams << QParam("point_id", otInteger, point_id);
        SQLText =
          "SELECT priority, class, cfg, block, prot "
          "FROM trip_classes, classes "
          "WHERE trip_classes.class=classes.code AND "
          "      point_id=:point_id AND cfg>0 "
          "ORDER BY priority ";
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

FltMarkFilter::FltMarkFilter(const AirlineCode_t& airline,
                             const FlightNumber_t& fltNumber,
                             const FlightSuffix_t& suffix,
                             const AirportCode_t& airpDep,
                             const TDateTime scdDateDep,
                             const DateType scdDateDepInUTC) :
  airline_(airline),
  fltNumber_(fltNumber),
  suffix_(suffix),
  airpDep_(airpDep)
{
  if (scdDateDep==NoExists)
    throw Exception("%s: scdDateDep==NoExists!", __func__);
  if (scdDateDepInUTC)
    throw Exception("%s: scdDateDepInUTC=true not supported!", __func__);
  modf(scdDateDep, &scdDateDepLocalTruncated_);
}

std::set<PointIdMkt_t> FltMarkFilter::search() const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT DISTINCT point_id FROM mark_trips "
    "WHERE airline=:airline AND flt_no=:flt_no AND "
    "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND "
    "      scd=:scd AND airp_dep=:airp_dep";
  Qry.CreateVariable( "airline", otString, airline_.get() );
  Qry.CreateVariable( "flt_no", otInteger, fltNumber_.get() );
  Qry.CreateVariable( "suffix", otString, suffix_.get() );
  Qry.CreateVariable( "scd", otDate, scdDateDepLocalTruncated_ );
  Qry.CreateVariable( "airp_dep", otString, airpDep_.get() );
  Qry.Execute();
  set<PointIdMkt_t> result;
  for ( ; !Qry.Eof; Qry.Next() ) {
    result.emplace( Qry.FieldAsInteger( "point_id") );
  }

  return result;
}

FltOperFilter::FltOperFilter(const AirlineCode_t& airline,
                             const FlightNumber_t& fltNumber,
                             const FlightSuffix_t& suffix,
                             const std::optional<AirportCode_t>& airpDep,
                             const TDateTime dateDep,
                             const DateType dateDepType,
                             const std::set<DateFlag>& dateDepFlags,
                             const FlightProps& fltProps) :
  airline_(airline),
  fltNumber_(fltNumber),
  suffix_(suffix),
  airpDepOpt_(airpDep),
  dateDepInUTC_(dateDepType==UTC),
  dateDepFlags_(dateDepFlags),
  fltProps_(fltProps)
{
  if (dateDep==NoExists)
    throw Exception("%s: dateDep==NoExists!", __func__);
  modf(dateDep, &dateDepTruncated_);
}

bool FltOperFilter::suitable(const AirportCode_t& airpDep,
                             const TDateTime timeDepInUTC) const
{
  if (timeDepInUTC==NoExists) return false;

  TDateTime dateForCompare=timeDepInUTC;
  if (!dateDepInUTC_)
  {
    string region=AirpTZRegion(airpDep.get(), false);
    if (region.empty()) return false;
    dateForCompare=UTCToLocal(timeDepInUTC, region);
  }
  modf(dateForCompare, &dateForCompare);

  return dateDepTruncated_==dateForCompare;
}

std::list<TAdvTripInfo> FltOperFilter::search() const
{
  std::list<TAdvTripInfo> result;

  QParams QryParams;
  QryParams << QParam("airline", otString, airline_.get())
            << QParam("flt_no", otInteger, fltNumber_.get())
            << QParam("suffix", otString, suffix_.get())
            << QParam("airp_dep", otString, airpDepOpt_?airpDepOpt_.value().get():"")
            << QParam("date_dep", otDate, dateDepTruncated_);

  ostringstream sql;
  sql <<
    "SELECT " << TAdvTripInfo::selectedFields() << " FROM points \n";

  if (!dateDepInUTC_)
  {
    //перевод UTCToLocal(points.scd)
    sql <<
      "WHERE airline=:airline AND flt_no=:flt_no AND airp=:airp_dep AND \n"
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND \n"
      "      (scd_out >= TO_DATE(:date_dep)-1 AND scd_out < TO_DATE(:date_dep)+2 \n ";
    if(dateDepFlags_.count(Est)>0)
    sql <<
      "      or est_out >= TO_DATE(:date_dep)-1 AND est_out < TO_DATE(:date_dep)+2 \n ";
    if(dateDepFlags_.count(Act)>0)
    sql <<
      "      or act_out >= TO_DATE(:date_dep)-1 AND act_out < TO_DATE(:date_dep)+2 \n ";
    sql <<
      "      ) AND \n"
      "      pr_del>=0 \n";
  }
  else
  {
    sql <<
      "WHERE airline=:airline AND flt_no=:flt_no AND \n"
      "      (:airp_dep IS NULL OR airp=:airp_dep) AND \n"
      "      (suffix IS NULL AND :suffix IS NULL OR suffix=:suffix) AND \n"
      "      (scd_out >= TO_DATE(:date_dep) AND scd_out < TO_DATE(:date_dep)+1 \n";
    if(dateDepFlags_.count(Est)>0)
    sql <<
      "      or est_out >= TO_DATE(:date_dep) AND est_out < TO_DATE(:date_dep)+1 \n";
    if(dateDepFlags_.count(Act)>0)
    sql <<
      "      or act_out >= TO_DATE(:date_dep) AND act_out < TO_DATE(:date_dep)+1 \n";
    sql <<
      "      ) AND \n"
      "      pr_del>=0 \n";
  };

  sql << " " << additionalWhere_;

  TCachedQuery PointsQry(sql.str(), QryParams);
  PointsQry.get().Execute();
  for(;!PointsQry.get().Eof;PointsQry.get().Next())
  {
    if (!TAdvTripInfo::match(PointsQry.get(), fltProps_)) continue;
    TAdvTripInfo flt(PointsQry.get());

    if (suitable(AirportCode_t(flt.airp), flt.scd_out))
    {
      result.push_back(flt);
      continue;
    }

    if (dateDepFlags_.count(Est)>0 && flt.est_out &&
        suitable(AirportCode_t(flt.airp), flt.est_out.get()))
    {
      result.push_back(flt);
      continue;
    }

    if (dateDepFlags_.count(Act)>0 && flt.act_out &&
        suitable(AirportCode_t(flt.airp), flt.act_out.get()))
    {
      result.push_back(flt);
      continue;
    }
  }

  return result;
}

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
