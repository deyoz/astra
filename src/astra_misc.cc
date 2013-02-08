#include "astra_misc.h"
#include <string>
#include <vector>
#include <set>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "convert.h"
#include "astra_locale.h"
#include "seats_utils.h"
#include "aodb.h"
#include "web_main.h"
#include "basel_aero.h"
#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace ASTRA;

void TTripInfo::get_client_dates(TDateTime &scd_out_client, TDateTime &real_out_client, bool trunc_time) const
{
  scd_out_client=ASTRA::NoExists;
  real_out_client=ASTRA::NoExists;

  if (airp.empty() || scd_out==ASTRA::NoExists) return;

  string &tz_region=AirpTZRegion(airp);
  scd_out_client=UTCToClient(scd_out,tz_region);
  if (trunc_time)
    modf(scd_out_client,&scd_out_client);
  if (real_out!=ASTRA::NoExists)
  {
    real_out_client=UTCToClient(real_out,tz_region);
    if (trunc_time)
      modf(real_out_client,&real_out_client);
  }
  else
    real_out_client=scd_out_client;
};

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

const long int DOC_CSV_CZ_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 //DOC_GENDER_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 //DOC_TYPE_FIELD|
                                 DOC_NO_FIELD/*|
                                 DOC_EXPIRY_DATE_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD*/;
                                 
const long int DOC_EDI_CZ_FIELDS=DOC_CSV_CZ_FIELDS;

const long int DOC_CSV_DE_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_GENDER_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD;
                                 
const long int DOCO_CSV_DE_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD|
                                  DOCO_APPLIC_COUNTRY_FIELD;
                                  
const long int DOC_TXT_EE_FIELDS=DOC_SURNAME_FIELD|
                                 DOC_FIRST_NAME_FIELD|
                                 DOC_BIRTH_DATE_FIELD|
                                 DOC_NATIONALITY_FIELD|
                                 DOC_TYPE_FIELD|
                                 DOC_NO_FIELD|
                                 DOC_ISSUE_COUNTRY_FIELD|
                                 DOC_GENDER_FIELD;
                                 
const long int DOCO_TXT_EE_FIELDS=DOCO_TYPE_FIELD|
                                  DOCO_NO_FIELD;

TCheckDocInfo GetCheckDocInfo(const int point_dep, const string& airp_arv)
{
  set<string> apis_formats;
  return GetCheckDocInfo(point_dep, airp_arv, apis_formats);
};

TCheckDocInfo GetCheckDocInfo(const int point_dep, const string& airp_arv, set<string> &apis_formats)
{
  apis_formats.clear();
  TCheckDocInfo result;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.airline, points.airp, trip_sets.pr_reg_with_doc "
    "FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id(+) AND points.point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();
  
  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("pr_reg_with_doc") &&
        Qry.FieldAsInteger("pr_reg_with_doc")!=0) result.first.required_fields|=DOC_NO_FIELD;
    
    try
    {
      string airline, country_dep, country_arv, city;
      airline=Qry.FieldAsString("airline");
      city=base_tables.get("airps").get_row("code", Qry.FieldAsString("airp") ).AsString("city");
      country_dep=base_tables.get("cities").get_row("code", city).AsString("country");
      city=base_tables.get("airps").get_row("code", airp_arv ).AsString("city");
      country_arv=base_tables.get("cities").get_row("code", city).AsString("country");
      
      Qry.Clear();
      Qry.SQLText=
        "SELECT format FROM apis_sets "
        "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND "
        "      pr_denial=0";
      Qry.CreateVariable("airline", otString, airline);
      Qry.CreateVariable("country_dep", otString, country_dep);
      Qry.CreateVariable("country_arv", otString, country_arv);
      Qry.Execute();
      if (!Qry.Eof)
      {
        result.first.is_inter=country_dep!=country_arv;
        result.second.is_inter=country_dep!=country_arv;
        for(;!Qry.Eof;Qry.Next())
        {
          string fmt=Qry.FieldAsString("format");
          apis_formats.insert(fmt);
          if (fmt=="CSV_CZ") result.first.required_fields|=DOC_CSV_CZ_FIELDS;
          if (fmt=="EDI_CZ") result.first.required_fields|=DOC_EDI_CZ_FIELDS;
          if (fmt=="CSV_DE")
          {
            result.first.required_fields|=DOC_CSV_DE_FIELDS;
            result.second.required_fields|=DOCO_CSV_DE_FIELDS;
          };
          if (fmt=="TXT_EE")
          {
            result.first.required_fields|=DOC_TXT_EE_FIELDS;
            result.second.required_fields|=DOCO_TXT_EE_FIELDS;
          };
        };
      };
    }
    catch(EBaseTableError) {};
    
  };
  return result;
};

TCheckDocTknInfo GetCheckTknInfo(const int point_dep)
{
  TCheckDocTknInfo result;
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT pr_reg_with_tkn FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id",otInteger,point_dep);
  Qry.Execute();

  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("pr_reg_with_tkn") &&
        Qry.FieldAsInteger("pr_reg_with_tkn")!=0) result.required_fields|=TKN_TICKET_NO_FIELD;
  };
  return result;
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
};

bool TTripRoute::GetRouteAfter(TDateTime part_key,
                               int point_id,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  return GetRoute(part_key,point_id,true,route_type1,route_type2);
};

bool TTripRoute::GetRouteBefore(TDateTime part_key,
                                int point_id,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  return GetRoute(part_key,point_id,false,route_type1,route_type2);
};

void TTripRoute::GetRouteAfter(TDateTime part_key,
                               int point_id,
                               int point_num,
                               int first_point,
                               bool pr_tranzit,
                               TTripRouteType1 route_type1,
                               TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
           true,route_type1,route_type2,Qry);
};

void TTripRoute::GetRouteBefore(TDateTime part_key,
                                int point_id,
                                int point_num,
                                int first_point,
                                bool pr_tranzit,
                                TTripRouteType1 route_type1,
                                TTripRouteType2 route_type2)
{
  clear();
  TQuery Qry(&OraSession);
  GetRoute(part_key,point_id,point_num,first_point,pr_tranzit,
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
      "SELECT points.airline, points.flt_no, points.suffix, points.airp, points.scd_out, "
      "       NVL(points.act_out, NVL(points.est_out, points.scd_out)) AS real_out, "
      "       points.pr_del, points.airline_fmt, points.suffix_fmt, points.airp_fmt, "
      "       pax_grp.airp_arv "
      "FROM pax_grp, points "
      "WHERE points.point_id=pax_grp.point_dep AND "
      "      pax_grp.grp_id = :grp_id AND points.pr_del>=0";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if (Qry.Eof) return false;
    push_back(TTrferRouteItem());
    TTrferRouteItem &item=back();
    item.operFlt.Init(Qry);
    item.operFlt.scd_out=UTCToLocal(item.operFlt.scd_out, AirpTZRegion(item.operFlt.airp));
    item.operFlt.real_out=UTCToLocal(item.operFlt.real_out, AirpTZRegion(item.operFlt.airp));
    item.airp_arv=Qry.FieldAsString("airp_arv");
  };
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,airline_fmt,flt_no,suffix,suffix_fmt,scd AS scd_out, "
    "       airp_dep AS airp,airp_dep_fmt AS airp_fmt,airp_arv,airp_arv_fmt "
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
  };
  return true;
};

void TCkinRoute::GetRoute(int tckin_id,
                          int seg_no,
                          bool after_current,
                          TCkinRouteType1 route_type1,
                          TCkinRouteType2 route_type2,
                          TQuery& Qry)
{
  ostringstream sql;
  sql << "SELECT points.airline, points.flt_no, points.suffix, points.airp, points.scd_out, "
         "       NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
         "       points.airline_fmt, points.suffix_fmt, points.airp_fmt, points.pr_del, "
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
      item.grp_id=Qry.FieldAsInteger("grp_id");
      item.point_dep=Qry.FieldAsInteger("point_dep");
      item.point_arv=Qry.FieldAsInteger("point_arv");
      item.airp_dep=Qry.FieldAsString("airp_dep");
      item.airp_arv=Qry.FieldAsString("airp_arv");
      item.seg_no=Qry.FieldAsInteger("seg_no");
      item.operFlt.Init(Qry);
      
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
	vector<TSeatRange> ranges;
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

void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo, bool return_scd_utc)
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
    if (return_scd_utc)
      flt.scd_out=operFltInfo.scd_out;
    else
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
  
  vector< pair<TElemFmt,string> > fmts_code, fmts_name;
  if (lang.empty())
  {
    getElemFmts(efmtCodeNative, TReqInfo::Instance()->desk.lang, fmts_code);
    getElemFmts(efmtNameLong, TReqInfo::Instance()->desk.lang, fmts_name);
  }
  else
  {
    getElemFmts(efmtCodeNative, lang, fmts_code);
    getElemFmts(efmtNameLong, lang, fmts_name);
  };
  
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
    catch (EBaseTableError) {};
      
    if (!city.empty())
      result << ElemIdToElem(etCity, city, fmts_name, true)
             << "(" << ElemIdToElem(etAirp, r->airp, fmts_code, true) << ")";
    else
      result << ElemIdToElem(etAirp, r->airp, fmts_code, true);
  };

  return result.str();
};

string GetCfgStr(TDateTime part_key,
                 int point_id,
                 const string &lang,
                 const string &separator)
{
  ostringstream result;

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (part_key!=NoExists)
  {
    Qry.SQLText=
      "SELECT class, cfg FROM arx_trip_classes,classes "
      "WHERE arx_trip_classes.class=classes.code(+) AND "
      "      part_key=:part_key AND point_id=:point_id "
      "ORDER BY priority";
    Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
    Qry.SQLText=
      "SELECT class, cfg FROM trip_classes,classes "
      "WHERE trip_classes.class=classes.code AND point_id=:point_id "
      "ORDER BY priority";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  vector< pair<TElemFmt,string> > fmts_code;
  if (lang.empty())
    getElemFmts(efmtCodeNative, TReqInfo::Instance()->desk.lang, fmts_code);
  else
    getElemFmts(efmtCodeNative, lang, fmts_code);
  for(;!Qry.Eof;Qry.Next())
  {
    if (!result.str().empty()) result << separator;
  
    result << ElemIdToElem(etClass, Qry.FieldAsString("class"), fmts_code, true)
           << Qry.FieldAsInteger("cfg");
  };
  return result.str();
};

void GetTagRanges(const vector<TBagTagNumber> &tags,
                  vector<string> &ranges)
{
  ranges.clear();
  if (tags.empty()) return;

  string first_alpha_part,curr_alpha_part;
  double first_no,first_pack,curr_no,curr_pack;
  first_alpha_part=tags.begin()->alpha_part;
  first_no=fmod(tags.begin()->numeric_part, 1000.0);
  modf(tags.begin()->numeric_part/1000.0,&first_pack);
  int num=0;
  for(std::vector<TBagTagNumber>::const_iterator iTag=tags.begin();; ++iTag)
  {
    if (iTag!=tags.end())
    {
      curr_alpha_part=iTag->alpha_part;
      curr_no=fmod(iTag->numeric_part, 1000.0);
      modf(iTag->numeric_part/1000.0,&curr_pack);
    };
     
    if (iTag==tags.end() ||
        first_alpha_part!=curr_alpha_part||
        first_pack!=curr_pack||
        first_no+num!=curr_no)
    {
      ostringstream range;
      range.setf(ios::fixed);
      range << first_alpha_part << setw(10) << setfill('0') << setprecision(0)
            << (first_pack*1000.0+first_no);
      if (num!=1)
        range << "-"
              << setw(3)  << setfill('0')
              << (first_no+num-1);
              
      ranges.push_back(range.str());
      
      if (iTag==tags.end()) break;
      first_alpha_part=curr_alpha_part;
      first_no=curr_no;
      first_pack=curr_pack;
      num=0;
    };
    num++;
  };
};


string GetTagRangesStr(const vector<TBagTagNumber> &tags)
{
  vector<string> ranges;
  
  GetTagRanges(tags, ranges);

  ostringstream result;
  for(vector<string>::const_iterator r=ranges.begin(); r!=ranges.end(); ++r)
  {
    if (r!=ranges.begin()) result << " ";
    result << *r;
  };
  return result.str();
};

string GetBagRcptStr(const vector<string> &rcpts)
{
  ostringstream result;
  string prior_no;
  for(vector<string>::const_iterator no=rcpts.begin(); no!=rcpts.end(); ++no)
  {
    int no_len=no->size();
    if (no!=rcpts.begin() &&
        no_len>2 &&
        no_len==(int)prior_no.size() &&
        no->substr(0,no_len-2)==prior_no.substr(0,no_len-2))
    {
      result << "/" << no->substr(no_len-2);
    }
    else
    {
      if (no!=rcpts.begin()) result << "/";
      result << *no;
    };
    prior_no=*no;
  };
  return result.str();
};

string GetBagRcptStr(int grp_id, int pax_id)
{
  TQuery Qry(&OraSession);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  
  int main_pax_id=NoExists;
  if (pax_id!=NoExists)
  {
    Qry.SQLText=
      "SELECT ckin.get_main_pax_id2(:grp_id) AS main_pax_id FROM dual";
    Qry.Execute();
    if (!Qry.Eof && !Qry.FieldIsNULL("main_pax_id")) main_pax_id=Qry.FieldAsInteger("main_pax_id");
  };
  if (pax_id==NoExists ||
      main_pax_id!=NoExists && main_pax_id==pax_id)
  {
    vector<string> rcpts;
    Qry.SQLText="SELECT no FROM bag_prepay WHERE grp_id=:grp_id";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
      rcpts.push_back(Qry.FieldAsString("no"));
      
    Qry.SQLText="SELECT form_type,no FROM bag_receipts WHERE grp_id=:grp_id AND annul_date IS NULL";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      int no_len=10;
      try
      {
        no_len=base_tables.get("form_types").get_row("code",Qry.FieldAsString("form_type")).AsInteger("no_len");
      }
      catch(EBaseTableError) {};
      ostringstream no_str;
      no_str << fixed << setw(no_len) << setfill('0') << setprecision(0) << Qry.FieldAsFloat("no");
      rcpts.push_back(no_str.str());
    };
    if (!rcpts.empty())
    {
      sort(rcpts.begin(),rcpts.end());
      return GetBagRcptStr(rcpts);
    };
  };
  return "";
};

bool BagPaymentCompleted(int grp_id, int *value_bag_count)
{
  vector< pair< int, int> > paid_bag;         //< bag_type, weight >
  vector< pair< double, string > > value_bag; //< value, value_cur >
  TQuery Qry(&OraSession);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  
  Qry.SQLText="SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id AND weight>0";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int bag_type=Qry.FieldIsNULL("bag_type")?NoExists:Qry.FieldAsInteger("bag_type");
    paid_bag.push_back( make_pair(bag_type, Qry.FieldAsInteger("weight")) );
  };
  
  Qry.SQLText=
    "SELECT DISTINCT value_bag.num, value_bag.value, value_bag.value_cur "
    "FROM pax_grp, value_bag, bag2 "
    "WHERE pax_grp.grp_id=value_bag.grp_id AND "
    "      value_bag.grp_id=bag2.grp_id(+) AND "
    "      value_bag.num=bag2.value_bag_num(+) AND "
    "      (bag2.grp_id IS NULL OR "
    "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0) AND "
    "      pax_grp.grp_id=:grp_id AND value_bag.value>0";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    value_bag.push_back( make_pair(Qry.FieldAsFloat("value"), Qry.FieldAsString("value_cur")) );
  };
  if (value_bag_count!=NULL) *value_bag_count=value_bag.size();
  
  if (paid_bag.empty() && value_bag.empty()) return true;
  
  TQuery KitQry(&OraSession);
  KitQry.Clear();
  KitQry.SQLText=
    "SELECT bag_rcpt_kits.kit_id "
    "FROM bag_rcpt_kits, bag_receipts "
    "WHERE bag_rcpt_kits.kit_id=bag_receipts.kit_id(+) AND "
    "      bag_rcpt_kits.kit_num=bag_receipts.kit_num(+) AND "
    "      bag_receipts.annul_date(+) IS NULL AND "
    "      bag_rcpt_kits.kit_id=:kit_id AND "
    "      bag_receipts.kit_id IS NULL";
  KitQry.DeclareVariable("kit_id", otInteger);
    
  if (!paid_bag.empty())
  {
    map< int, int > rcpt_paid_bag;
    for(int pass=0;pass<=2;pass++)
    {
      switch (pass)
      {
        case 0: Qry.SQLText=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND annul_date IS NULL AND service_type IN (1,2) AND kit_id IS NULL";
                break;
        case 1: Qry.SQLText=
                  "SELECT MIN(bag_type) AS bag_type, MIN(ex_weight) AS ex_weight, kit_id "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND service_type IN (1,2) AND kit_id IS NOT NULL "
                  "GROUP BY kit_id";
                break;
        case 2: Qry.SQLText=
                  "SELECT bag_type, ex_weight "
                  "FROM bag_prepay "
                  "WHERE grp_id=:grp_id AND value IS NULL";
                break;
      };
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (pass==1)
        {
          KitQry.SetVariable("kit_id", Qry.FieldAsInteger("kit_id"));
          KitQry.Execute();
          if (!KitQry.Eof) continue;
        };
        int bag_type=Qry.FieldIsNULL("bag_type")?NoExists:Qry.FieldAsInteger("bag_type");
        if (rcpt_paid_bag.find(bag_type)==rcpt_paid_bag.end())
          rcpt_paid_bag[bag_type]=Qry.FieldAsInteger("ex_weight");
        else
          rcpt_paid_bag[bag_type]+=Qry.FieldAsInteger("ex_weight");
      };
    };
    
    for(vector< pair< int, int> >::const_iterator i=paid_bag.begin();i!=paid_bag.end();++i)
    {
      map< int, int >::const_iterator j=rcpt_paid_bag.find(i->first);
      if (j==rcpt_paid_bag.end()) return false;
      if (j->second<i->second) return false;
    };
  };
  
  if (!value_bag.empty())
  {
    map< pair< double, string >, int > rcpt_value_bag;
    for(int pass=0;pass<=2;pass++)
    {
      switch (pass)
      {
        case 0: Qry.SQLText=
                  "SELECT rate AS value,rate_cur AS value_cur "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND annul_date IS NULL AND service_type=3 AND kit_id IS NULL";
                  break;
        case 1: Qry.SQLText=
                  "SELECT MIN(rate) AS value, MIN(rate_cur) AS value_cur, kit_id "
                  "FROM bag_receipts "
                  "WHERE grp_id=:grp_id AND service_type=3 AND kit_id IS NOT NULL "
                  "GROUP BY kit_id";
                  break;
        case 2: Qry.SQLText=
                  "SELECT value,value_cur "
                  "FROM bag_prepay "
                  "WHERE grp_id=:grp_id AND value IS NOT NULL";
                  break;
      };
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        if (pass==1)
        {
          KitQry.SetVariable("kit_id", Qry.FieldAsInteger("kit_id"));
          KitQry.Execute();
          if (!KitQry.Eof) continue;
        };
        pair< double, string > bag(Qry.FieldAsFloat("value"), Qry.FieldAsString("value_cur"));
        if (rcpt_value_bag.find(bag)==rcpt_value_bag.end())
          rcpt_value_bag[bag]=1;
        else
          rcpt_value_bag[bag]+=1;
      };
    };

    for(vector< pair< double, string > >::const_iterator i=value_bag.begin();i!=value_bag.end();++i)
    {
      map< pair< double, string >, int >::iterator j=rcpt_value_bag.find(*i);
      if (j==rcpt_value_bag.end()) return false;
      if (j->second<=0) return false;
      j->second--;
    };
  };
  
  return true;
};

string GetPaxDocCountryCode(const string &doc_code)
{
  //на входе либо countries.code либо pax_doc_countries.code
  string pax_doc_country;
  if (!doc_code.empty())
  {
    try
    {
      pax_doc_country=getBaseTable(etPaxDocCountry).get_row("code",doc_code).AsString("code");
    }
    catch (EBaseTableError)
    {
      try
      {
        pax_doc_country=getBaseTable(etPaxDocCountry).get_row("country",doc_code).AsString("code");
      }
      catch (EBaseTableError) {};
    };
  };
  return pax_doc_country;
};

bool isTestPaxId(int id)
{
  return id!=NoExists && id>=TEST_ID_BASE && id<=TEST_ID_BASE+999999999;
}

bool is_sync_paxs( int point_id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    " FROM points "
    " WHERE point_id=:point_id AND pr_reg<>0 AND pr_del=0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return false;
  TTripInfo tripInfo( Qry );
  return AstraWeb::is_sync_meridian( tripInfo ) ||
         is_sync_basel_pax( tripInfo ) ||
         is_sync_aodb_pax( tripInfo );
}

void update_pax_change( int point_id, int pax_id, int reg_no, const string &work_mode )
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
     "BEGIN "
     " UPDATE aodb_pax_change "
     "  SET point_id=:point_id, desk=:desk, client_type=:client_type, time=:time "
     " WHERE pax_id=:pax_id AND reg_no=:reg_no AND work_mode=:work_mode; "
     " IF SQL%NOTFOUND THEN "
     "  INSERT INTO aodb_pax_change(pax_id,reg_no,work_mode,point_id,desk,client_type,time) "
     "   VALUES(:pax_id,:reg_no,:work_mode,:point_id,:desk,:client_type,:time); "
     " END IF; "
     "END;";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.CreateVariable( "reg_no", otInteger, reg_no );
  Qry.CreateVariable( "work_mode", otString, work_mode );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "client_type", otString,  EncodeClientType(TReqInfo::Instance()->client_type) );
  Qry.CreateVariable( "time", otDate, NowUTC() );
  Qry.Execute();
}





