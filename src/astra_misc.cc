
#include "astra_misc.h"
#include <string>
#include <vector>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg/tlg_parser.h"
#include "convert.h"

#define NICKNAME "DEN"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace ASTRA;

string GetTripName( TTripInfo &info, bool showAirp, bool prList )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  TDateTime scd_out_local_date,desk_time;
  string &tz_region=AirpTZRegion(info.airp);
  modf(reqInfo->desk.time,&desk_time);
  modf(UTCToClient(info.scd_out,tz_region),&scd_out_local_date);
  if (info.real_out!=ASTRA::NoExists)
    modf(UTCToClient(info.real_out,tz_region),&info.real_out_local_date);
  else
    info.real_out_local_date=scd_out_local_date;

  ostringstream trip;
  trip << info.airline
       << setw(3) << setfill('0') << info.flt_no
       << info.suffix;

  if (prList)
  {
    if (info.flt_no<10000) trip << " ";
    if (info.flt_no<1000)  trip << " ";
  };

  if (desk_time!=info.real_out_local_date)
  {
    if (DateTimeToStr(desk_time,"mm")==DateTimeToStr(info.real_out_local_date,"mm"))
      trip << "/" << DateTimeToStr(info.real_out_local_date,"dd");
    else
      trip << "/" << DateTimeToStr(info.real_out_local_date,"dd.mm");
  };
  if (scd_out_local_date!=info.real_out_local_date)
    trip << "(" << DateTimeToStr(scd_out_local_date,"dd") << ")";
  if (!(reqInfo->user.user_type==utAirport &&
        reqInfo->user.access.airps_permit &&
        reqInfo->user.access.airps.size()==1)||showAirp)
    trip << " " << info.airp;
  if(info.pr_del != ASTRA::NoExists and info.pr_del != 0)
      trip << " " << (info.pr_del < 0 ? "(㤫.)" : "(��.)");

  return trip.str();
};

bool GetTripSets( TTripSetType setType, TTripInfo &info )
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
      //����� ���ࠪ⨢� � �����
      case tsETLOnly: return false;
              default: return false;
    };
  };
  return Qry.FieldAsInteger("pr_misc")!=0;
};

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
    //� ���� �� ��ॣ�㧨�� ������ ��ᢠ������
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
    //� ���� �� ��ॣ�㧨�� ������ ��ᢠ������
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

void TMktFlight::dump()
{
    ProgTrace(TRACE5, "---TMktFlight::dump()---");
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %d", flt_no);
    ProgTrace(TRACE5, "suffix: %s", suffix.c_str());
    ProgTrace(TRACE5, "subcls: %s", subcls.c_str());
    ProgTrace(TRACE5, "scd: %d", scd);
    ProgTrace(TRACE5, "airp_dep: %s", airp_dep.c_str());
    ProgTrace(TRACE5, "airp_arv: %s", airp_arv.c_str());
    ProgTrace(TRACE5, "---END OF TMktFlight::dump()---");

}

bool TMktFlight::IsNULL()
{
    return
        airline.empty() or
        flt_no == NoExists or
        subcls.empty() or
        scd == NoExists or
        airp_dep.empty() or
        airp_arv.empty();
}

void TMktFlight::get(int pax_id)
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
        "    crs_pax.pax_id = :pax_id and "
        "    crs_pax.pnr_id = crs_pnr.pnr_id and "
        "    crs_pnr.point_id = tlg_trips.point_id and "
        "    crs_pax.pnr_id = pnr_market_flt.pnr_id(+) ";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(!Qry.Eof) {
        if(Qry.FieldIsNULL("pax_airline")) {
            airline = Qry.FieldAsString("tlg_airline");
            flt_no = Qry.FieldAsInteger("tlg_flt_no");
            suffix = Qry.FieldAsString("tlg_suffix");
            subcls = Qry.FieldAsString("tlg_subcls");

            TDateTime tmp_scd = Qry.FieldAsDateTime("tlg_scd");
            int Year, Month, Day;
            DecodeDate(tmp_scd, Year, Month, Day);

            scd = Day;
            airp_dep = Qry.FieldAsString("tlg_airp_dep");
            airp_arv = Qry.FieldAsString("tlg_airp_arv");
        } else {
            airline = Qry.FieldAsString("pax_airline");
            flt_no = Qry.FieldAsInteger("pax_flt_no");
            suffix = Qry.FieldAsString("pax_suffix");
            subcls = Qry.FieldAsString("pax_subcls");
            scd = Qry.FieldAsInteger("pax_scd");
            airp_dep = Qry.FieldAsString("pax_airp_dep");
            airp_arv = Qry.FieldAsString("pax_airp_arv");
        }
    }
    dump();
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
    "SELECT overload_alarm,brd_alarm,waitlist_alarm,pr_etstatus,pr_salon,act "
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
}

string TripAlarmString( TTripAlarmsType &alarm )
{
	string mes;
	switch( alarm ) {
		case atOverload:
			mes = "��ॣ�㧪�";
			break;
		case atWaitlist:
			mes = "���� ��������";
			break;
		case atBrd:
			mes = "��ᠤ��";
			break;
		case atSalon:
			mes = "�� �����祭 ᠫ��";
			break;
		case atETStatus:
			mes = "��� �裡 � ���";
			break;
		default:;
	}
	return mes;
}

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
 	while ( !Qry->Eof ) {
 		TSeat seat;
    if ( format == "list" && !res.empty() )
    	res += " ";
   	res = denorm_iata_row( Qry->FieldAsString( "first_yname" ) ) +
          denorm_iata_line( Qry->FieldAsString( "first_xname" ), pr_lat_seat );
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
    flt.scd_out=operFltInfo.scd_out;
    flt.airp=operFltInfo.airp;
    markFltInfo.push_back(flt);
  };
};



