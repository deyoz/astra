
#include "astra_misc.h"
#include <string>
#include <vector>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg/tlg_parser.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

string GetPnrAddr(int pnr_id, vector<TPnrAddrItem> &pnrs, string airline)
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

string GetPaxPnrAddr(int pax_id, vector<TPnrAddrItem> &pnrs, string airline)
{
  pnrs.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.Execute();
  if (!Qry.Eof)
    return GetPnrAddr(Qry.FieldAsInteger("pnr_id"),pnrs,airline);
  else
    return "";
};

TDateTime DayToDate(int day, TDateTime base_date)
{
  if (day<1 || day>31) throw EConvertError("DayToDate: wrong day");
  modf(base_date,&base_date);
  int iDay,iMonth,iYear;
  DecodeDate(base_date,iYear,iMonth,iDay);
  TDateTime result;
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
  if (!pr_tranzit)
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

/*
void TTripRoute::get(int point_id)
{
    clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_num, DECODE(pr_tranzit,0,point_id,first_point) first_point "
        "FROM points WHERE point_id=:point_id AND pr_del>=0 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("TRoute::get: point_num & first_point fetch failed for point_id %d", point_id);
    int point_num = Qry.FieldAsInteger("point_num");
    int first_point = Qry.FieldAsInteger("first_point");
    Qry.Clear();
    Qry.SQLText =
        "SELECT "
        "  points.airp, "
        "  cities.name city, "
        "  points.point_num "
        "FROM  "
        "  points, "
        "  airps, "
        "  cities "
        "WHERE "
        "  points.point_id=:point_id AND points.pr_del>=0 AND "
        "  points.airp = airps.code and "
        "  airps.city = cities.code "
        "union "
        "SELECT "
        "  points.airp, "
        "  cities.name city, "
        "  points.point_num "
        "FROM  "
        "  points, "
        "  airps, "
        "  cities "
        "WHERE "
        "  points.first_point=:first_point AND "
        "  points.point_num>:point_num AND "
        "  points.pr_del=0 and "
        "  points.airp = airps.code and "
        "  airps.city = cities.code "
        "ORDER by "
        "  point_num ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("first_point", otInteger, first_point);
    Qry.CreateVariable("point_num", otInteger, point_num);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("TRoute::get route fetch failed for point_id %d", point_id);
    for(; !Qry.Eof; Qry.Next()) {
        TTripRouteItem item;
        item.airp = Qry.FieldAsString("airp");
        //item.city = Qry.FieldAsString("city");
        item.point_num = Qry.FieldAsInteger("point_num");
        push_back(item);
    }
}*/

string mkt_airline(int pax_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   tlg_trips.airline "
        "from "
        "   tlg_trips, "
        "   crs_pnr, "
        "   crs_pax "
        "where "
        "   tlg_trips.point_id = crs_pnr.point_id and "
        "   crs_pnr.point_id = crs_pax.point_id and "
        "   crs_pax.pax_id = :pax_id ";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("mkt_airline: pax_id %d not found", pax_id);
    return Qry.FieldAsString(0);
}

