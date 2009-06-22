#include "docs.h"
#include "stat.h"
#include "telegram.h"
#include "oralib.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "season.h"
#include "brd.h"
#include "astra_misc.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const string ALL_CKIN_ZONES = " ";

string vsHow_ru(int nmb, int range)
{
    static char* sotni[] = {
        "сто ",
        "двести ",
        "триста ",
        "четыреста ",
        "пятьсот ",
        "шестьсот ",
        "семьсот ",
        "восемьсот ",
        "девятьсот "
    };
    static char* teen[] = {
        "десять ",
        "одиннадцать ",
        "двенадцать ",
        "тринадцать ",
        "четырнадцать ",
        "пятнадцать ",
        "шестнадцать ",
        "семнадцать ",
        "восемнадцать ",
        "девятнадцать "
    };
    static char* desatki[] = {
        "двадцать ",
        "тридцать ",
        "сорок ",
        "пятьдесят ",
        "шестьдесят ",
        "семьдесят ",
        "восемьдесят ",
        "девяносто "
    };
    static char* stuki_g[] = {
        "",
        "одно ",
        "два ",
        "три ",
        "четыре ",
        "пять ",
        "шесть ",
        "семь ",
        "восемь ",
        "девять "
    };
    static char* stuki_m[] = {
        "",
        "одна ",
        "две ",
        "три ",
        "четыре ",
        "пять ",
        "шесть ",
        "семь ",
        "восемь ",
        "девять "
    };
    static char* dtext[2][3] = {
        {"", "", ""},
        {"тысяча ", "тысячи ", "тысяч "}
    };

    string out;
    if(nmb == 0) return out;
    int tmp = nmb / 100;
    if(tmp > 0) out += sotni[tmp - 1];
    tmp = nmb % 100;
    if(tmp >= 10 && tmp < 20) out += teen[tmp - 10];
    else {
        tmp /= 10;
        if(tmp > 1) out += desatki[tmp - 2];
        tmp = (nmb % 100) % 10;
        switch(range) {
            case 0:
            case 1:
            case 2:
            case 4:
                out += stuki_m[tmp];
                break;
            case 3:
            case 5:
                out += stuki_g[tmp];
                break;
            default:
                throw Exception("vsHow: unknown range: " + IntToString(range));
        }
    }
    switch(tmp) {
        case 1:
            out += dtext[range][0];
            break;
        case 2:
        case 3:
        case 4:
            out += dtext[range][1];
            break;
        default:
            out += dtext[range][2];
            break;
    }
    return out;
}

string vsHow_lat(int nmb, int range)
{
    static char* sotni[] = {
        "one hundred ",
        "two hundreds ",
        "three hundreds ",
        "four hundreds ",
        "five hundreds ",
        "six hundreds ",
        "seven hundreds ",
        "eight hundreds ",
        "nine hundreds "
    };
    static char* teen[] = {
        "ten ",
        "eleven ",
        "twelve ",
        "thirteen ",
        "fourteen ",
        "fifteen ",
        "sixteen ",
        "seventeen ",
        "eighteen ",
        "nineteen "
    };
    static char* desatki[] = {
        "twenty ",
        "thirty ",
        "forty ",
        "fifty ",
        "sixty ",
        "seventy ",
        "eighty ",
        "ninety "
    };
    static char* stuki_m[] = {
        "",
        "one ",
        "two ",
        "three ",
        "four ",
        "five ",
        "six ",
        "seven ",
        "eight ",
        "nine "
    };
    static char* dtext[2][3] = {
        {"", "", ""},
        {"thousand ", "thousands ", "thousands "}
    };

    string out;
    if(nmb == 0) return out;
    int tmp = nmb / 100;
    if(tmp > 0) out += sotni[tmp - 1];
    tmp = nmb % 100;
    if(tmp >= 10 && tmp < 20) out += teen[tmp - 10];
    else {
        tmp /= 10;
        if(tmp > 1) out += desatki[tmp - 2];
        tmp = (nmb % 100) % 10;
        switch(range) {
            case 0:
            case 1:
            case 2:
            case 4:
                out += stuki_m[tmp];
                break;
            case 3:
            case 5:
                out += stuki_m[tmp];
                break;
            default:
                throw Exception("vsHow: unknown range: " + IntToString(range));
        }
    }
    switch(tmp) {
        case 1:
            out += dtext[range][0];
            break;
        case 2:
        case 3:
        case 4:
            out += dtext[range][1];
            break;
        default:
            out += dtext[range][2];
            break;
    }
    return out;
}

string vs_number(int number, bool pr_lat)
{
    string result;
    if(number >= 1000000) {
       result = "XXXXXX";
       ProgTrace(TRACE5, "vs_number: value too large (>= 1000000): %d", number);
    } else {
	    int i = number / 1000;
	    result += (pr_lat ? vsHow_lat(i, 1) : vsHow_ru(i, 1));
	    i = number % 1000;
	    result += (pr_lat ? vsHow_lat(i, 0) : vsHow_ru(i, 0));
    }
    return result;
}

enum TState {PMTrfer, PM};

void SeasonListVars(int trip_id, int pr_lat, xmlNodePtr variablesNode, xmlNodePtr reqNode)
{
    NewTextChild(variablesNode, "test_server", get_test_server());
	vector<SEASON::TViewPeriod> viewp;
	SEASON::ReadTripInfo( trip_id, viewp, reqNode );
  for ( vector<SEASON::TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
/*    NewTextChild( variablesNode, "exec", i->exec );
    NewTextChild( variablesNode, "noexec", i->noexec );*/
    for ( vector<SEASON::TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end(); j++ ) {
/*    	xmlNodePtr tripsNode = NULL;
    	if ( !tripsNode )
        tripsNode = NewTextChild( variablesNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "move_id", j->move_id );*/
      NewTextChild( variablesNode, "trip", j->name );
/*      NewTextChild( tripNode, "crafts", j->crafts );
      NewTextChild( tripNode, "ports", j->ports );
      if ( j->land > NoExists )
        NewTextChild( tripNode, "land", DateTimeToStr( j->land ) );
      if ( j->takeoff > NoExists )
        NewTextChild( tripNode, "takeoff", DateTimeToStr( j->takeoff ) );*/
      break;
    }
  }
}

void PaxListVars(int point_id, int pr_lat, xmlNodePtr variablesNode, TDateTime part_key)
{
    TQuery Qry(&OraSession);
    string SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   NVL(act_out,NVL(est_out,scd_out)) real_out, "
        "   scd_out, "
        "   ckin.get_airps(point_id, 1) long_route "
        "from ";
    if(part_key == NoExists)
        SQLText +=
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0 ";
    else {
        SQLText +=
        "   arx_points "
        "where "
        "   part_key = :part_key and "
        "   point_id = :point_id AND pr_del>=0 ";
        Qry.CreateVariable("part_key", otDate, part_key);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    string airline_name;
    if(airline.size()) {
      airline = base_tables.get("AIRLINES").get_row("code",airline).AsString("code",pr_lat);
      TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
      airline_name = airlineRow.AsString("name", pr_lat);
    }

    string trip =
        airline +
        IntToString(Qry.FieldAsInteger("flt_no")) +
        Qry.FieldAsString("suffix");

    NewTextChild(variablesNode, "trip", trip);
    TDateTime scd_out, real_out;
    scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
    real_out= UTCToClient(Qry.FieldAsDateTime("real_out"),tz_region);
    NewTextChild(variablesNode, "scd_out", DateTimeToStr(scd_out, "dd.mm.yyyy", pr_lat));
    NewTextChild(variablesNode, "real_out", DateTimeToStr(real_out, "dd.mm.yyyy", pr_lat));
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
    NewTextChild(variablesNode, "day_issue", DateTimeToStr(issued, "dd.mm.yy", pr_lat));

    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airp_dep_city", airpRow.AsString("city", pr_lat));
    NewTextChild(variablesNode, "airline_name", airline_name);
    NewTextChild(variablesNode, "flt", trip);
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", craft);
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    NewTextChild(variablesNode, "airp_arv_name", airpRow.AsString("name",pr_lat));
    NewTextChild(variablesNode, "airp_arv_city", airpRow.AsString("city",pr_lat));
    NewTextChild(variablesNode, "long_route", Qry.FieldAsString("long_route"));
    NewTextChild(variablesNode, "test_server", get_test_server());
}

enum TRptType {rtPTM, rtBTM, rtREFUSE, rtNOTPRES, rtREM, rtCRS, rtCRSUNREG, rtEXAM, rtUnknown, rtTypeNum};
const char *RptTypeS[rtTypeNum] = {
    "PTM",
    "BTM",
    "REFUSE",
    "NOTPRES",
    "REM",
    "CRS",
    "CRSUNREG",
    "EXAM",
    "?"
};

TRptType DecodeRptType( const string rpt_type )
{
  int i;
  for( i=0; i<(int)rtTypeNum; i++ )
    if ( rpt_type == RptTypeS[ i ] )
      break;
  if ( i == rtTypeNum )
    return rtUnknown;
  else
    return (TRptType)i;
}

struct TRptParams {
    int point_id;
    TRptType rpt_type;
    string airp_arv;
    string ckin_zone;
    bool pr_et;
    bool pr_trfer;
    bool pr_brd;
    TCodeShareInfo mkt_flt;
    TRptParams():
        point_id(NoExists),
        pr_et(false),
        pr_trfer(false),
        pr_brd(false)
    {};
};

int GetRPEncoding(TRptParams &rpt_params)
{
    TBaseTable &airps = base_tables.get("AIRPS");
    TBaseTable &cities = base_tables.get("CITIES");
    int result = 0;
    if(rpt_params.airp_arv.empty()) {
        TTripRoute route;
        if (!route.GetRouteAfter(rpt_params.point_id,trtWithCurrent,trtNotCancelled))
            throw Exception("TTripRoute::GetRouteAfter: flight not found for point_id %d", rpt_params.point_id);
        for(vector<TTripRouteItem>::iterator iv = route.begin(); iv != route.end(); iv++) {
            //ProgTrace(TRACE5, "%s %s", iv->airp.c_str(), iv->city.c_str());
            ProgTrace(TRACE5, "%s", cities.get_row("code",
                        airps.get_row("code",iv->airp).AsString("city")).AsString("country").c_str());
            result = result or cities.get_row("code",
                    airps.get_row("code",iv->airp).AsString("city")).AsString("country") != "РФ";
        }
    } else {
        result = cities.get_row("code",
                airps.get_row("code",rpt_params.airp_arv).AsString("city")).AsString("country") != "РФ";
    }
    return result;
}

int GetRPEncoding(int point_id, string target)
{
    TBaseTable &airps = base_tables.get("AIRPS");
    TBaseTable &cities = base_tables.get("CITIES");
    int result = 0;
    if(
            target.empty() ||
            target == "tot" ||
            target == "etm" ||
            target == "tpm"
      ) {
        TTripRoute route;
        if (!route.GetRouteAfter(point_id,trtWithCurrent,trtNotCancelled))
          throw Exception("TTripRoute::GetRouteAfter: flight not found for point_id %d", point_id);
        for(vector<TTripRouteItem>::iterator iv = route.begin(); iv != route.end(); iv++) {
            //ProgTrace(TRACE5, "%s %s", iv->airp.c_str(), iv->city.c_str());
            ProgTrace(TRACE5, "%s", cities.get_row("code",
                    airps.get_row("code",iv->airp).AsString("city")).AsString("country").c_str());
            result = result or cities.get_row("code",
                    airps.get_row("code",iv->airp).AsString("city")).AsString("country") != "РФ";
        }
    } else {
        result = cities.get_row("code",
                airps.get_row("code",target).AsString("city")).AsString("country") != "РФ";
    }
    return result;
}

void RunCRS(string name, xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "      tlg_binding.point_id_spp AS point_id, "
        "      ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr_ref, "
        "      decode(:pr_lat, 0, (crs_pax.surname||' '||crs_pax.name), system.transliter(crs_pax.surname||' '||crs_pax.name)) family, "
        "      decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "      decode(:pr_lat, 0, classes.code, classes.code_lat) class, "
        "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS seat_no, "
        "      decode(:pr_lat, 0, airps.code, NVL(airps.code_lat,airps.code)) target, "
        "      report.get_trfer_airp(last_target) last_target, "
        "      report.get_TKNO(crs_pax.pax_id) ticket_no, "
        "      report.get_PSPT(crs_pax.pax_id) AS document, "
        "      report.get_crsRemarks(crs_pax.pax_id) AS remarks "
        "FROM crs_pnr,tlg_binding,crs_pax,pers_types,classes,airps ";
    if(name != "crs")
        SQLText += " , pax ";
    SQLText +=
        "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pers_type=pers_types.code AND "
        "      crs_pnr.class=classes.code AND "
        "      crs_pnr.target=airps.code AND "
        "      crs_pax.pr_del=0 and "
        "      tlg_binding.point_id_spp = :point_id ";
    if(name != "crs")
        SQLText +=
            "    and crs_pax.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null ";
    SQLText +=
            "order by "
            "    family ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_crs");

    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");


        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "pnr_ref", Qry.FieldAsString("pnr_ref"));
        NewTextChild(rowNode, "family", Qry.FieldAsString("family"));
        NewTextChild(rowNode, "pers_type", Qry.FieldAsString("pers_type"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "target", Qry.FieldAsString("target"));

        string last_target = Qry.FieldAsString("last_target");
        NewTextChild(rowNode, "last_target", last_target);

        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
        NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
        NewTextChild(rowNode, "remarks", Qry.FieldAsString("remarks"));

        Qry.Next();
    }

    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

void RunSZV(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    NewTextChild(dataSetsNode, "events_log");
    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

void RunExam(xmlNodePtr reqNode, xmlNodePtr &formDataNode)
{
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
    xmlNodePtr resNode = formDataNode->parent;

    xmlUnlinkNode(formDataNode);
    xmlFreeNode(formDataNode);

    BrdInterface::GetPax(reqNode, resNode);
    xmlNodePtr currNode = resNode->children;
    formDataNode = NodeAsNodeFast("form_data", currNode);
    xmlNodePtr dataNode = NodeAsNodeFast("data", currNode);
    currNode = formDataNode->children;
    xmlNodePtr variablesNode = NodeAsNodeFast("variables", currNode);

    xmlUnlinkNode(dataNode);

    xmlNodeSetName(dataNode, (xmlChar *)"datasets");
    xmlAddChild(formDataNode, dataNode);
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    // Теперь переменные отчета
    NewTextChild(variablesNode, "paxlist_type", "Досмотр / Посадка");
    PaxListVars(point_id, pr_lat, variablesNode);
    currNode = variablesNode->children;
    xmlNodePtr totalNode = NodeAsNodeFast("total", currNode);
    NodeSetContent(totalNode, (string)"Итого: " + NodeAsString(totalNode));
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
}

void RunRem(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name)) family, "
        "       decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no, "
        "       report.get_reminfo(pax_id,',') AS info "
        "FROM   pax_grp,pax,pers_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pers_type=pers_types.code AND "
        "       pr_brd IS NOT NULL and "
        "       point_dep = :point_id and "
        "       report.get_reminfo(pax_id,',') is not null "
        "order by "
        "       reg_no ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_rem");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", Qry.FieldAsString("family"));
        NewTextChild(rowNode, "pers_type", Qry.FieldAsString("pers_type"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "info", Qry.FieldAsString("info"));

        Qry.Next();
    }

    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

void RunRef(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name)) family, "
        "       decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "       ticket_no, "
        "       decode(:pr_lat, 0, refusal_types.name, NVL(refusal_types.name_lat,refusal_types.name)) refuse, "
        "       ckin.get_birks(pax.grp_id,pax.pax_id,:pr_lat) AS tags "
        "FROM   pax_grp,pax,pers_types,refusal_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pers_type=pers_types.code AND "
        "       pax.refuse = refusal_types.code AND "
        "       pax.refuse IS NOT NULL and "
        "       point_dep = :point_id "
        "order by "
        "       reg_no ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_ref");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", Qry.FieldAsString("family"));
        NewTextChild(rowNode, "pers_type", Qry.FieldAsString("pers_type"));
        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
        NewTextChild(rowNode, "refuse", Qry.FieldAsString("refuse"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

        Qry.Next();
    }

    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

void RunNotpres(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name)) family, "
        "       decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no, "
        "       ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum) AS bagAmount, "
        "       ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum) AS bagWeight, "
        "       ckin.get_birks(pax.grp_id,pax.pax_id,:pr_lat) AS tags "
        "FROM   pax_grp,pax,pers_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pers_type=pers_types.code AND "
        "       pax.pr_brd=0 and "
        "       point_dep = :point_id "
        "order by "
        "       reg_no ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_notpres");
    TBaseTable &pers_types = base_tables.get("PERS_TYPES");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", Qry.FieldAsString("family"));
        NewTextChild(rowNode, "pers_type",
          pers_types.get_row("code",Qry.FieldAsString("pers_type")).AsString("code",pr_lat));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "bagamount", Qry.FieldAsInteger("bagamount"));
        NewTextChild(rowNode, "bagweight", Qry.FieldAsInteger("bagweight"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

        Qry.Next();
    }

    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

string get_last_target(TQuery &Qry, const int pr_lat)
{
    string result;
    string airline = Qry.FieldAsString("trfer_airline");
    if(!airline.empty()) {
        string airp = Qry.FieldAsString("trfer_airp_arv");
        try {
            TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
            string tmp_airp = airpRow.AsString("name", pr_lat);
            if(tmp_airp.empty())
                tmp_airp = airpRow.AsString("name", 0);
            if(!tmp_airp.empty())
                airp = tmp_airp.substr(0, 50);
        } catch(...) {
        }
        string airline = Qry.FieldAsString("trfer_airline");
        try {
            TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
            string tmp_airline = airlineRow.AsString("code", pr_lat);
            if(!tmp_airline.empty())
                airline = tmp_airline;
        } catch(...) {
        }
        ostringstream buf;
        buf
            << airp
            << "("
            << airline
            << setw(3) << setfill('0') << Qry.FieldAsInteger("trfer_flt_no")
            << convert_suffix(Qry.FieldAsString("trfer_suffix"), pr_lat)
            << ")/" << DateTimeToStr(Qry.FieldAsDateTime("trfer_scd"), "dd");
        result = buf.str();
    }
    return result;
}

string get_hall_list(string airp, string zone, bool pr_lat)
{
    TQuery Qry(&OraSession);
    string SQLText = (string)
        "select " + (pr_lat ? "name_lat" : "name") + " name "
        "from "
        "   halls2 "
        "where "
        "   airp = :airp and "
        "   nvl(rpt_grp, ' ') = nvl(:rpt_grp, ' ') "
        "order by "
        "   name ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("rpt_grp", otString, zone);
    Qry.Execute();
    string result;
    for(; !Qry.Eof; Qry.Next()) {
        if(Qry.FieldIsNULL("name"))
            continue;
        if(!result.empty())
            result += ", ";
        result += Qry.FieldAsString("name");
    }
    ProgTrace(TRACE5, "get_hall_list result: %s", result.c_str());
    return result;
}

void RunPMNew(string name, xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_brd_pax = -1;
    xmlNodePtr prBrdPaxNode = GetNode("pr_brd_pax", reqNode);
    if(prBrdPaxNode)
        pr_brd_pax = NodeAsInteger(prBrdPaxNode);
    string target = NodeAsString("target", reqNode);

    // зал регистрации
    // если NULL, то отчет общий по всем залам
    xmlNodePtr zoneNode = GetNode("zone", reqNode);
    string zone;
    if(zoneNode != NULL)
        zone = NodeAsString(zoneNode);

    int pr_lat = GetRPEncoding(point_id, target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);
    string status = NodeAsString("status", reqNode);

    xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
    if(
            target.empty() ||
            target == "tot"
      ) {
        xmlNodePtr formNode = GetNode("form", resNode);
        if(formNode) {
            xmlUnlinkNode(formNode);
            xmlFreeNode(formNode);
        }
        get_report_form("PMTotalEL", resNode);
        string et, et_lat;
        if(target.empty()) {
            et = "(ЭБ)";
            et_lat = "(ET)";
        }
        NewTextChild(variablesNode, "et", et);
        NewTextChild(variablesNode, "et_lat", et_lat);
    }

    if(
            target == "tpm" ||
            target == "etm"
      ) {
        xmlNodePtr formNode = NodeAsNode("form", resNode);
        xmlUnlinkNode(formNode);
        xmlFreeNode(formNode);
        get_report_form("PMTrferTotalEL", resNode);
        string et, et_lat;
        if(target == "etm") {
            et = "(ЭБ)";
            et_lat = "(ET)";
        }
        NewTextChild(variablesNode, "et", et);
        NewTextChild(variablesNode, "et_lat", et_lat);
    }
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT \n"
        "   pax_grp.point_dep AS trip_id, \n"
        "   pax_grp.airp_arv AS target, \n";
    if(name == "PMTrfer")
        SQLText +=
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, \n"
            "    trfer_trips.airline trfer_airline, \n"
            "    trfer_trips.flt_no trfer_flt_no, \n"
            "    trfer_trips.suffix trfer_suffix, \n"
            "    transfer.airp_arv trfer_airp_arv, \n"
            "    trfer_trips.scd trfer_scd, \n";
    SQLText +=
        "   DECODE(:pr_lat,0,classes.code,nvl(classes.code_lat, classes.code)) AS class, \n"
        "   DECODE(:pr_lat,0,classes.name,nvl(classes.name_lat, classes.name)) AS class_name, \n"
        "   surname||' '||pax.name AS full_name, \n"
        "   pax.pers_type, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum,0) AS seat_no, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum,1) AS seat_no_lat, \n"
        "   pax.seats, \n";
    if(
            target.empty() ||
            target == "etm"
      ) { //ЭБ
        SQLText +=
            "    ticket_no||'/'||coupon_no AS remarks, \n";
    } else {
        SQLText +=
            " SUBSTR(report.get_remarks(pax_id,0),1,250) AS remarks, \n";
    }
    SQLText +=
        "   NVL(ckin.get_rkWeight(pax.grp_id,pax.pax_id),0) AS rk_weight, \n"
        "   NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight(pax.grp_id,pax.pax_id),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) AS excess, \n"
        "   ckin.get_birks(pax.grp_id,pax.pax_id,:pr_lat) AS tags, \n"
        "   reg_no, \n"
        "   pax_grp.grp_id \n"
        "FROM  \n"
        "   pax_grp, \n"
        "   points, \n"
        "   pax, \n"
        "   cls_grp classes, \n"
        "   halls2 \n";
    if(name == "PMTrfer")
        SQLText += ", transfer, trfer_trips \n";
    SQLText +=
        "WHERE \n"
        "   points.pr_del>=0 AND \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.point_arv = points.point_id and \n"
        "   pax_grp.grp_id=pax.grp_id AND \n"
        "   pax_grp.class_grp = classes.id AND \n"
        "   pax_grp.hall = halls2.id and \n"
        "   pr_brd IS NOT NULL and \n";
    if(pr_brd_pax != -1) {
        SQLText +=
            " decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
        Qry.CreateVariable("pr_brd_pax", otInteger, pr_brd_pax);
    }
    if(
            target.empty() ||
            target == "etm"
      ) { //ЭБ
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    } else if(
            target == "tot" ||
            target == "tpm"
            ) { //ОБЩАЯ
    } else { // сегмент
        SQLText +=
            "    pax_grp.airp_arv = :target AND \n";
        if(pr_vip != 2)
            SQLText +=
                "    halls2.pr_vip = :pr_vip AND \n";
        if(zoneNode) {
            SQLText +=
                "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
            Qry.CreateVariable("zone", otString, zone);
        }
    }
    SQLText +=
        "       DECODE(pax_grp.status, 'T', pax_grp.status, 'N') in ";
    if(status.size())
        SQLText +=
            "    (:status) \n";
    else
        SQLText +=
            "    ('T', 'N') \n";
    if(name == "PMTrfer")
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";
    SQLText +=
        "ORDER BY \n";
    if(
            target.empty() ||
            target == "tot" ||
            target == "etm" ||
            target == "tpm"
      )
        SQLText +=
            "   points.point_num, \n";
    if(name == "PMTrfer")
        SQLText +=
            "    PR_TRFER ASC, \n"
            "    TRFER_AIRP_ARV ASC, \n";
    SQLText +=
        "    CLASS ASC, \n"
        "    grp_id, \n"
        "    REG_NO ASC \n";


    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());

    bool pr_target =
        target.size() &&
        target != "tot" &&
        target != "etm" &&
        target != "tpm";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    if(pr_target)
        Qry.CreateVariable("target", otString, target);
    if(status.size())
        Qry.CreateVariable("status", otString, status);
    if(pr_vip != 2)
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    Qry.CreateVariable("pr_lat", otString, pr_lat);

    for(int i = 0; i < Qry.VariablesCount(); i++) {
        ProgTrace(TRACE5, "Qry var name: %s", Qry.VariableName(i));
    }
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer");
    // следующие 2 переменные введены для нужд FastReport
    map<string, int> fr_target_ref;
    int fr_target_ref_idx = 0;

    while(!Qry.Eof) {
        string cls = Qry.FieldAsString("class");
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", Qry.FieldAsString("reg_no"));
        NewTextChild(rowNode, "full_name", transliter(Qry.FieldAsString("full_name"), pr_lat));
        string last_target;
        int pr_trfer = 0;
        if(name == "PMTrfer") {
            last_target = get_last_target(Qry, pr_lat);
            pr_trfer = Qry.FieldAsInteger("pr_trfer");
        }
        NewTextChild(rowNode, "last_target", last_target);
        NewTextChild(rowNode, "pr_trfer", pr_trfer);

        string airp_arv = Qry.FieldAsString("target");
        if(fr_target_ref.find(airp_arv) == fr_target_ref.end())
            fr_target_ref[airp_arv] = fr_target_ref_idx++;
        TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp_arv);
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "fr_target_ref", fr_target_ref[airp_arv]);
        NewTextChild(rowNode, "airp_arv_name", airpRow.AsString("name", pr_lat));

        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "class_name", Qry.FieldAsString("class_name"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "seats", Qry.FieldAsInteger("seats"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        string pers_type = Qry.FieldAsString("pers_type");
        if(pers_type == "ВЗ")
            NewTextChild(rowNode, "pers_type", "ADL");
        else if(pers_type == "РБ")
            NewTextChild(rowNode, "pers_type", "CHD");
        else if(pers_type == "РМ")
            NewTextChild(rowNode, "pers_type", "INF");
        else
            throw Exception("RunPM: unknown pers_type " + pers_type);
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        if (pr_lat==0)
          NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        else
          NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no_lat"));
        NewTextChild(rowNode, "remarks", Qry.FieldAsString("remarks"));
        Qry.Next();
    }

    bool pr_trfer = name == "PMTrfer";
    Qry.Clear();
    SQLText =
        "SELECT \n"
        "       a.point_id, \n";
    if(pr_trfer)
        SQLText +=
            "   a.pr_trfer, \n"
            "   a.airp_arv target, \n";
    SQLText +=
        "       a.status, \n"
        "       classes.code class, \n"
        "       DECODE(:pr_lat,0,classes.name,classes.name_lat) AS class_name, \n"
        "       classes.priority AS lvl, \n"
        "       NVL(a.seats,0) AS seats, \n"
        "       NVL(a.adl,0) AS adl, \n"
        "       NVL(a.chd,0) AS chd, \n"
        "       NVL(a.inf,0) AS inf, \n"
        "       NVL(b.rk_weight,0) AS rk_weight, \n"
        "       NVL(b.bag_amount,0) AS bag_amount, \n"
        "       NVL(b.bag_weight,0) AS bag_weight, \n"
        "       NVL(c.excess,0) AS excess \n"
        "FROM \n"
        "( \n"
        "  SELECT point_dep AS point_id, \n"
        "         pax_grp.class_grp, \n"
        "         DECODE(status,'T','T','N') AS status, \n";
    if(pr_vip != 2)
        SQLText += " halls2.pr_vip, \n";
    if(pr_trfer)
        SQLText +=
            "   pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) AS pr_trfer, \n";
    SQLText +=
        "         SUM(seats) AS seats, \n"
        "         SUM(DECODE(pers_type,'ВЗ',1,0)) AS adl, \n"
        "         SUM(DECODE(pers_type,'РБ',1,0)) AS chd, \n"
        "         SUM(DECODE(pers_type,'РМ',1,0)) AS inf \n"
        "  FROM pax_grp,pax,transfer,halls2 \n"
        "  WHERE \n"
        "       pax_grp.point_dep = :point_id and \n"
        "       pax.grp_id = pax_grp.grp_id and \n";
    if(pr_target) {
        SQLText +=
            "   pax_grp.airp_arv = :target and \n";
        Qry.CreateVariable("target", otString, target);
    }
    if(pr_vip != 2) {
        SQLText +=
            " pr_vip = :pr_vip and \n";
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    }
    if(target.empty() || target == "etm")
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(pr_brd_pax != -1) {
        SQLText +=
            " decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
        Qry.CreateVariable("pr_brd_pax", otInteger, pr_brd_pax);
    }
    if(zoneNode) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
        Qry.CreateVariable("zone", otString, zone);
    }
    SQLText +=
        "       pax_grp.grp_id = transfer.grp_id(+) AND transfer.pr_final(+)<>0 AND \n"
        "       pax_grp.hall = halls2.id and \n"
        "       pr_brd IS NOT NULL and \n"
        "       bag_refuse=0 AND class IS NOT NULL \n"
        "  GROUP BY point_dep, \n"
        "           pax_grp.class_grp, \n"
        "           DECODE(status,'T','T','N') \n";
    if(pr_trfer)
        SQLText +=
            "   ,pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) \n";
    if(pr_vip != 2) {
        SQLText +=
            ", halls2.pr_vip \n";
    }
    SQLText +=
        ") a, \n"
        "( \n"
        "  SELECT point_dep AS point_id, \n"
        "         pax_grp.class_grp, \n"
        "         DECODE(status,'T','T','N') AS status, \n";
    if(pr_vip != 2)
        SQLText += " halls2.pr_vip, \n";
    if(pr_trfer)
        SQLText +=
            "   pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) AS pr_trfer, \n";
    SQLText +=
        "     SUM(DECODE(pr_cabin,1,weight,0)) AS rk_weight, \n"
        "     SUM(DECODE(pr_cabin,0,amount,0)) AS bag_amount, \n"
        "     SUM(DECODE(pr_cabin,0,weight,0)) AS bag_weight \n"
        "  FROM pax_grp,pax,bag2,transfer,halls2 \n"
        "  WHERE \n"
        "       pax_grp.point_dep = :point_id and \n"
        "       pax.pax_id = ckin.get_main_pax_id(pax_grp.grp_id) and \n";
    if(pr_target) {
        SQLText +=
            "   pax_grp.airp_arv = :target and \n";
    }
    if(pr_vip != 2) {
        SQLText +=
            " pr_vip = :pr_vip and \n";
    }
    if(target.empty() || target == "etm")
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(pr_brd_pax != -1)
        SQLText +=
            " decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
    if(zoneNode) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
        Qry.CreateVariable("zone", otString, zone);
    }
    SQLText +=
        "       pax_grp.grp_id=bag2.grp_id AND \n"
        "       pax_grp.grp_id = transfer.grp_id(+) AND transfer.pr_final(+)<>0 AND \n"
        "       pax_grp.hall = halls2.id and \n"
        "       pr_brd IS NOT NULL and \n"
        "       bag_refuse=0 AND class IS NOT NULL \n"
        "  GROUP BY point_dep, \n"
        "         pax_grp.class_grp, \n"
        "        DECODE(status,'T','T','N') \n";
    if(pr_trfer)
        SQLText +=
            "   ,pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) \n";
    if(pr_vip != 2) {
        SQLText +=
            ", halls2.pr_vip \n";
    }
    SQLText +=
        ") b, \n"
        "( \n"
        "  SELECT point_dep AS point_id, \n"
        "         pax_grp.class_grp, \n"
        "         DECODE(status,'T','T','N') AS status, \n";
    if(pr_vip != 2)
        SQLText += " halls2.pr_vip, \n";
    if(pr_trfer)
        SQLText +=
            "   pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) AS pr_trfer, \n";
    SQLText +=
        "         SUM(excess) AS excess \n"
        "  FROM pax_grp,pax,transfer,halls2 \n"
        "  WHERE \n"
        "       pax_grp.point_dep = :point_id and \n"
        "       pax.pax_id = ckin.get_main_pax_id(pax_grp.grp_id) and \n";
    if(pr_target) {
        SQLText +=
            "   pax_grp.airp_arv = :target and \n";
    }
    if(pr_vip != 2) {
        SQLText +=
            " pr_vip = :pr_vip and \n";
    }
    if(target.empty() || target == "etm")
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(pr_brd_pax != -1)
        SQLText +=
            " decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
    if(zoneNode) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
        Qry.CreateVariable("zone", otString, zone);
    }
    SQLText +=
        "       pax_grp.grp_id = transfer.grp_id(+) AND transfer.pr_final(+)<>0 AND \n"
        "       pax_grp.hall = halls2.id and \n"
        "       pr_brd IS NOT NULL and \n"
        "       bag_refuse=0 AND class IS NOT NULL \n"
        "  GROUP BY point_dep, \n"
        "           pax_grp.class_grp, \n"
        "           DECODE(status,'T','T','N') \n";
    if(pr_trfer)
        SQLText +=
            "   ,pax_grp.airp_arv, \n"
            "   DECODE(transfer.grp_id,NULL,0,1) \n";
    if(pr_vip != 2) {
        SQLText +=
            ", halls2.pr_vip \n";
    }
    SQLText +=
        ") c, \n"
        "cls_grp classes \n"
        "WHERE a.class_grp=classes.id AND \n"
        "      a.point_id=b.point_id(+) AND \n"
        "      a.class_grp=b.class_grp(+) AND \n"
        "      a.status=b.status(+) AND \n"
        "      a.point_id=c.point_id(+) AND \n"
        "      a.class_grp=c.class_grp(+) AND \n"
        "      a.status=c.status(+) \n";
    if(pr_trfer)
        SQLText +=
            "   and a.airp_arv=b.airp_arv(+) AND \n"
            "   a.pr_trfer=b.pr_trfer(+) AND \n"
            "   a.airp_arv=c.airp_arv(+) AND \n"
            "   a.pr_trfer=c.pr_trfer(+) \n";
    if(pr_vip != 2)
        SQLText +=
            "      and a.pr_vip = b.pr_vip(+) and \n"
            "      a.pr_vip = c.pr_vip(+) \n";
    if(status.size()) {
        SQLText +=
            "    and a.STATUS = :status \n";
        Qry.CreateVariable("status", otString, status);
    } else
        SQLText +=
            "    and a.STATUS in ('T', 'N') \n";
    SQLText +=
        "order by \n";
    if(pr_trfer)
        SQLText +=
            "   target, \n"
            "   pr_trfer, \n";
    SQLText +=
        "      lvl \n";
    ProgTrace(TRACE5, "RunPMNew totals: SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otInteger, pr_lat);
    Qry.Execute();
    dataSetNode = NewTextChild(dataSetsNode, pr_trfer ? "v_pm_trfer_total" : "v_pm_total");
    while(!Qry.Eof) {
        string cls = Qry.FieldAsString("class");
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("POINT_ID"));
        if(pr_trfer) {
            string airp_arv = Qry.FieldAsString("TARGET");
            NewTextChild(rowNode, "target", airp_arv);
            NewTextChild(rowNode, "fr_target_ref", fr_target_ref[airp_arv]);
            NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("PR_TRFER"));
        }
        NewTextChild(rowNode, "status", Qry.FieldAsString("STATUS"));
        NewTextChild(rowNode, "class_name", Qry.FieldAsString("CLASS_NAME"));
        NewTextChild(rowNode, "lvl", Qry.FieldAsInteger("LVL"));
        NewTextChild(rowNode, "seats", Qry.FieldAsInteger("SEATS"));
        NewTextChild(rowNode, "adl", Qry.FieldAsInteger("ADL"));
        NewTextChild(rowNode, "chd", Qry.FieldAsInteger("CHD"));
        NewTextChild(rowNode, "inf", Qry.FieldAsInteger("INF"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("RK_WEIGHT"));
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("BAG_AMOUNT"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("BAG_WEIGHT"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("EXCESS"));

        Qry.Next();
    }

    // Теперь переменные отчета
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   scd_out, "
        "   airp_fmt, "
        "   airline_fmt, "
        "   suffix_fmt, "
        "   craft_fmt "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunPM: variables fetch failed for point_id " + IntToString(point_id));

    int airline_fmt = Qry.FieldAsInteger("airline_fmt");
    int suffix_fmt = Qry.FieldAsInteger("suffix_fmt");
    int craft_fmt = Qry.FieldAsInteger("craft_fmt");

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string suffix = Qry.FieldAsString("suffix");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
    //    TCrafts crafts;

    if(zoneNode) {
            NewTextChild(variablesNode, "zone", get_hall_list(airp, zone, pr_lat));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlineRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "flt",
            ElemIdToElem(etAirline, airline, airline_fmt) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            ElemIdToElem(etSuffix, suffix, suffix_fmt)
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", ElemIdToElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    string airp_arv_name;
    if(pr_target)
        airp_arv_name = base_tables.get("AIRPS").get_row("code",target).AsString("name",pr_lat);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));

    NewTextChild(variablesNode, "pr_vip", pr_vip);
    string pr_brd_pax_str_lat;
    string pr_brd_pax_str;
    if(pr_brd_pax != -1) {
        if(pr_brd_pax == 0) {
            pr_brd_pax_str_lat = "(checked)";
            pr_brd_pax_str = "(зарег)";
        } else {
            pr_brd_pax_str_lat = "(boarded)";
            pr_brd_pax_str = "(посаж)";
        }
    }
    NewTextChild(variablesNode, "pr_brd_pax_lat", pr_brd_pax_str_lat);
    NewTextChild(variablesNode, "pr_brd_pax", pr_brd_pax_str);
    STAT::set_variables(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
}

struct TBag2PK {
    int grp_id, num;
    bool operator==(const TBag2PK op)
    {
        return (grp_id == op.grp_id) && (num == op.num);
    }
};

struct TBagTagRow {
    int pr_trfer;
    string last_target;
    int point_num;
    int grp_id;
    string airp_arv;
    int class_priority;
    string class_code;
    string class_name;
    int bag_type;
    int bag_name_priority;
    string bag_name;
    int bag_num;
    int amount;
    int weight;
    int pr_liab_limit;
    string tag_type;
    string color;
    double no;
    string tag_range;
    int num;
    TBagTagRow()
    {
        pr_trfer = -1;
        point_num = -1;
        grp_id = -1;
        class_priority = -1;
        bag_type = -1;
        bag_name_priority = -1;
        bag_num = -1;
        amount = -1;
        weight = -1;
        pr_liab_limit = -1;
        no = -1.;
        num = -1;
    }
};

bool lessBagTagRow(const TBagTagRow &p1, const TBagTagRow &p2)
{
    bool result;
    if(p1.point_num == p2.point_num) {
        if(p1.pr_trfer == p2.pr_trfer) {
            if(p1.last_target == p2.last_target) {
                if(p1.class_priority == p2.class_priority) {
                    if(p1.bag_name_priority == p2.bag_name_priority) {
                        if(p1.tag_type == p2.tag_type) {
                            result = p1.color < p2.color;
                        } else
                            result = p1.tag_type < p2.tag_type;
                    } else
                        result = p1.bag_name_priority < p2.bag_name_priority;
                } else
                    result = p1.class_priority < p2.class_priority;
            } else
                result = p1.last_target > p2.last_target;
        } else
            result = p1.pr_trfer < p2.pr_trfer;
    } else
        result = p1.point_num > p2.point_num;
    return result;
}

typedef struct {
    int bag_type;
    string class_code;
    string airp;
    string name;
    string name_lat;
} TBagNameRow;

class t_rpt_bm_bag_name {
    private:
        vector<TBagNameRow> bag_names;
    public:
        void init(string airp);
        string get(string class_code, int bag_type, int pr_lat);
};

string t_rpt_bm_bag_name::get(string class_code, int bag_type, int pr_lat)
{
    string result;
    for(vector<TBagNameRow>::iterator iv = bag_names.begin(); iv != bag_names.end(); iv++)
        if(iv->class_code == class_code && iv->bag_type == bag_type) {
            result = pr_lat ? iv->name_lat : iv->name;
            if(result.empty())
                result = iv->name;
            break;
        }
    return result;
}

void t_rpt_bm_bag_name::init(string airp)
{
    bag_names.clear();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   bag_type, "
        "   class, "
        "   airp, "
        "   name, "
        "   name_lat "
        "from "
        "   rpt_bm_bag_names "
        "where "
        "   airp is null or "
        "   airp = :airp "
        "order by "
        "   airp nulls last ";
    Qry.CreateVariable("airp", otString, airp);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBagNameRow bag_name_row;
        bag_name_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_name_row.class_code = Qry.FieldAsString("class");
        bag_name_row.airp = Qry.FieldAsString("airp");
        bag_name_row.name = Qry.FieldAsString("name");
        bag_name_row.name_lat = Qry.FieldAsString("name_lat");
        bag_names.push_back(bag_name_row);
    }
}

void dump_tag_row(TBagTagRow &tag, bool hdr = true)
{
    ostringstream log;
    if(hdr) {
        log
            << setw(9)  << "pr_trfer"
            << setw(12) << "last_target"
            << setw(10) << "grp_id"
            << setw(9)  << "airp_arv"
            << setw(11) << "class_name"
            << setw(9)  << "bag_type"
            << setw(18) << "bag_name_priority"
            << setw(23) << "bag_name"
            << setw(8)  << "bag_num"
            << setw(7)  << "amount"
            << setw(7)  << "weight"
            << setw(9)  << "tag_type"
            << setw(6)  << "color"
            << setw(10) << "no"
            << setw(30) << "tag_range"
            << setw(4)  << "num"
            << endl;
        ProgTrace(TRACE5, "%s", log.str().c_str());
        log.str("");
    }
    log
        << setw(9)  << tag.pr_trfer
        << setw(12) << tag.last_target
        << setw(10) << tag.grp_id
        << setw(9)  << tag.airp_arv
        << setw(11) << tag.class_name
        << setw(9)  << tag.bag_type
        << setw(18) << tag.bag_name_priority
        << setw(23) << tag.bag_name
        << setw(8)  << tag.bag_num
        << setw(7)  << tag.amount
        << setw(7)  << tag.weight
        << setw(9)  << tag.tag_type
        << setw(6)  << tag.color
        << setw(10) << fixed << setprecision(0) << tag.no
        << setw(30) << tag.tag_range
        << setw(4)  << tag.num
        << endl;
    ProgTrace(TRACE5, "%s", log.str().c_str());
}

void dump_bag_tags(vector<TBagTagRow> &bag_tags)
{
    ostringstream log;
    bool hdr = true;
    for(vector<TBagTagRow>::iterator iv = bag_tags.begin(); iv != bag_tags.end(); iv++) {
        dump_tag_row(*iv, hdr);
        if(hdr) hdr = false;
    }
    ProgTrace(TRACE5, "%s", log.str().c_str());
}

struct t_tag_nos_row {
    int pr_liab_limit;
    double no;
};

bool lessTagNos(const t_tag_nos_row &p1, const t_tag_nos_row &p2)
{
    return p1.no < p2.no;
}

string get_tag_rangeA(vector<t_tag_nos_row> &tag_nos, vector<t_tag_nos_row>::iterator begin, vector<t_tag_nos_row>::iterator end, int pr_lat)
{
    string lim = (pr_lat ? "(lim)" : "(огр)");
    ostringstream result;
    double first_no = -1.;
    double prev_no = -1.;
    int pr_liab_limit = -1;
    for(vector<t_tag_nos_row>::iterator iv = begin; iv != end; iv++) {
        if(result.str().empty() || iv->no - 1 != prev_no || iv->pr_liab_limit != pr_liab_limit) {
            if(!result.str().empty() && prev_no != first_no) {
                double mod = prev_no - (floor(prev_no / 1000) * 1000);
                if(pr_liab_limit) { // delete from stream unneeded first lim
                    long pos = result.tellp();
                    pos -= lim.size();
                    result.seekp(pos);
                }
                result << "-" << fixed << setprecision(0) << setw(3) << setfill('0') << mod;
                if(pr_liab_limit)
                    result << lim;
            }
            if(!result.str().empty()) {
                result << ", ";
            }
            result << fixed << setprecision(0) << setw(10) << setfill('0') << iv->no;
            if(iv->pr_liab_limit)
                result << lim;
            first_no = iv->no;
        }
        prev_no = iv->no;
        pr_liab_limit = iv->pr_liab_limit;
    }
    if(prev_no != first_no) {
        double mod = prev_no - (floor(prev_no / 1000) * 1000);
        if(pr_liab_limit) { // delete from stream unneeded first lim
            long pos = result.tellp();
            pos -= lim.size();
            result.seekp(pos);
        }
        result << "-" << fixed << setprecision(0) << setw(3) << setfill('0') << mod;
        if(pr_liab_limit)
            result << lim;
    }
    return result.str();
}

string get_tag_range(vector<t_tag_nos_row> tag_nos, int pr_lat)
{
    ostringstream result;
    sort(tag_nos.begin(), tag_nos.end(), lessTagNos);
    vector<t_tag_nos_row>::iterator begin = tag_nos.begin();
    double base = -1.;
    for(vector<t_tag_nos_row>::iterator iv = tag_nos.begin(); iv != tag_nos.end(); iv++) {
        double tmp_base = floor(iv->no / 1000);
        if(tmp_base != base) {
            base = tmp_base;
            if(iv != begin) {
                if(!result.str().empty())
                    result << ", ";
                result << get_tag_rangeA(tag_nos, begin, iv, pr_lat);
            }
            begin = iv;
        }
    }
    if(!result.str().empty())
        result << ", ";
    result << get_tag_rangeA(tag_nos, begin, tag_nos.end(), pr_lat);
    return result.str();
}

void RunBMNew(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_brd_pax = -1;
    xmlNodePtr prBrdPaxNode = GetNode("pr_brd_pax", reqNode);
    if(prBrdPaxNode)
        pr_brd_pax = NodeAsInteger(prBrdPaxNode);
    TQuery Qry(&OraSession);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(point_id, target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);
    bool pr_trfer = (string)NodeAsString("name", reqNode) == "BMTrfer";

    // зал регистрации
    // если NULL, то отчет общий по всем залам
    xmlNodePtr zoneNode = GetNode("zone", reqNode);
    string zone;
    if(zoneNode != NULL)
        zone = NodeAsString(zoneNode);

    //TODO: get_report_form in each report handler, not once!
    xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
    if(target.empty()) {
        xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
        // внутри get_report_form вызывается ReplaceTextChild, в котором в свою
        // очередь вызывается xmlNodeSetContent, который, судя по всему, кладет string
        // иначе чем xmlNewTextChild, что лично меня не устраивает. Поэтому удалим узел
        // form, чтобы get_report_form создал его заново. (c) Den. 26.11.07
        xmlNodePtr formNode = NodeAsNode("form", resNode);
        xmlUnlinkNode(formNode);
        xmlFreeNode(formNode);
        if(pr_trfer)
            get_report_form("BMTrferTotal", resNode);
        else
            get_report_form("BMTotal", resNode);
    }

    t_rpt_bm_bag_name bag_names;
    Qry.Clear();
    Qry.SQLText = "select airp from points where point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("RunBMNew: point_id %d not found", point_id);
    string airp = Qry.FieldAsString(0);
    bag_names.init(airp);
    vector<TBagTagRow> bag_tags;
    map<int, vector<TBagTagRow *> > grps;
    Qry.Clear();
    string SQLText =
        "select ";
    if(pr_trfer)
        SQLText +=
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, \n"
            "    trfer_trips.airline trfer_airline, \n"
            "    trfer_trips.flt_no trfer_flt_no, \n"
            "    trfer_trips.suffix trfer_suffix, \n"
            "    transfer.airp_arv trfer_airp_arv, \n"
            "    trfer_trips.scd trfer_scd, \n";
    else
        SQLText +=
            "    0 pr_trfer, "
            "    null trfer_airline, "
            "    null trfer_flt_no, "
            "    null trfer_suffix, "
            "    null trfer_airp_arv, "
            "    null trfer_scd, ";
    SQLText +=
        "    points.point_num, "
        "    pax_grp.grp_id, "
        "    pax_grp.airp_arv, "
        "    nvl(classes.priority, 100) class_priority, "
        "    classes.code class_code, "
        "    classes.name class_name, "
        "    bag2.bag_type, "
        "    bag2.num bag_num, "
        "    bag2.amount, "
        "    bag2.weight, "
        "    bag2.pr_liab_limit "
        "from "
        "    pax_grp, "
        "    points, "
        "    classes, "
        "    bag2, "
        "    halls2 ";
    if(pr_trfer)
        SQLText += ", transfer, trfer_trips ";
    if(pr_brd_pax != -1)
        SQLText += ", pax ";
    SQLText +=
        "where "
        "    points.pr_del>=0 AND "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.point_arv = points.point_id and "
        "    pax_grp.class = classes.code(+) and "
        "    pax_grp.grp_id = bag2.grp_id and "
        "    pax_grp.bag_refuse = 0 and "
        "    bag2.pr_cabin = 0 and "
        "    pax_grp.hall = halls2.id ";
    if(!target.empty()) {
        SQLText += " and pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, target);
    }
    if(pr_vip != 2) {
        SQLText +=
            " and halls2.pr_vip = :pr_vip ";
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    }
    if(zoneNode) {
        SQLText +=
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') ";
        Qry.CreateVariable("zone", otString, zone);
    }
    if(pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";
    if(pr_brd_pax != -1) {
        SQLText +=
            "   and pax.pax_id(+) = ckin.get_main_pax_id(pax_grp.grp_id) and "
            "   decode(:pr_brd_pax, 0, nvl2(pax.pr_brd(+), 0, -1), pax.pr_brd(+))  = :pr_brd_pax and "
            "   (pax_grp.class is not null and pax.pax_id is not null or pax_grp.class is null) ";
        Qry.CreateVariable("pr_brd_pax", otInteger, pr_brd_pax);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        int cur_grp_id = Qry.FieldAsInteger("grp_id");
        int cur_bag_num = Qry.FieldAsInteger("bag_num");

        TBagTagRow bag_tag_row;
        bag_tag_row.pr_trfer = Qry.FieldAsInteger("pr_trfer");
        bag_tag_row.last_target = get_last_target(Qry, pr_lat);
        bag_tag_row.point_num = Qry.FieldAsInteger("point_num");
        bag_tag_row.grp_id = cur_grp_id;
        bag_tag_row.airp_arv = Qry.FieldAsString("airp_arv");

        bag_tag_row.class_priority = Qry.FieldAsInteger("class_priority");
        bag_tag_row.class_code = Qry.FieldAsString("class_code");
        bag_tag_row.class_name = Qry.FieldAsString("class_name");
        bag_tag_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_tag_row.bag_name = bag_names.get(bag_tag_row.class_code, bag_tag_row.bag_type, pr_lat);
        if(!bag_tag_row.bag_name.empty())
            bag_tag_row.bag_name_priority = bag_tag_row.bag_type;
        bag_tag_row.bag_num = cur_bag_num;
        bag_tag_row.amount = Qry.FieldAsInteger("amount");
        bag_tag_row.weight = Qry.FieldAsInteger("weight");
        bag_tag_row.pr_liab_limit = Qry.FieldAsInteger("pr_liab_limit");

        TQuery tagsQry(&OraSession);
        tagsQry.SQLText =
            "select "
            "   bag_tags.tag_type, "
            "   bag_tags.color, "
            "   to_char(bag_tags.no) no "
            "from "
            "   bag_tags "
            "where "
            "   bag_tags.grp_id = :grp_id and "
            "   bag_tags.bag_num = :bag_num ";
        tagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
        tagsQry.CreateVariable("bag_num", otInteger, cur_bag_num);
        tagsQry.Execute();
        int bound_tags_amount = 0;
        for(; !tagsQry.Eof; tagsQry.Next()) {
            bag_tags.push_back(bag_tag_row);
            bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
            bag_tags.back().color = tagsQry.FieldAsString("color");
            bag_tags.back().no = tagsQry.FieldAsFloat("no");
            bound_tags_amount++;
        }

        if(bound_tags_amount < bag_tag_row.amount) { // остались непривязанные бирки
            if(grps.find(cur_grp_id) == grps.end()) {
                grps[cur_grp_id]; // Создаем пустой вектор, чтобы в след разы if который выше не срабатывал
                // ищем непривязанные бирки для каждой группы
                TQuery tagsQry(&OraSession);
                tagsQry.SQLText =
                    "select "
                    "   bag_tags.tag_type, "
                    "   bag_tags.color, "
                    "   to_char(bag_tags.no) no "
                    "from "
                    "   bag_tags "
                    "where "
                    "   bag_tags.grp_id = :grp_id and "
                    "   bag_tags.bag_num is null";
                tagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
                tagsQry.Execute();
                for(; !tagsQry.Eof; tagsQry.Next()) {
                    bag_tags.push_back(bag_tag_row);
                    bag_tags.back().bag_name_priority = -1;
                    bag_tags.back().bag_name = "";
                    bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
                    bag_tags.back().color = tagsQry.FieldAsString("color");
                    bag_tags.back().no = tagsQry.FieldAsFloat("no");
                    // запоминаем список ссылок на непривязанные бирки для данной группы
                    grps[cur_grp_id].push_back(&bag_tags.back());
                }
            } else if(grps[cur_grp_id].size()) {
                // если встретилась группа со списком непривязанных бирок
                // привязываем текущий багаж к одной из бирок и удаляем ее
                // (на самом деле в базе она не привязана, просто чтоб багажка правильно выводилась)
                grps[cur_grp_id].back()->bag_num = bag_tag_row.bag_num;
                grps[cur_grp_id].back()->amount = bag_tag_row.amount;
                grps[cur_grp_id].back()->weight = bag_tag_row.weight;
                grps[cur_grp_id].pop_back();
            }
        }
    }
    sort(bag_tags.begin(), bag_tags.end(), lessBagTagRow);
    dump_bag_tags(bag_tags);

    TBagTagRow bag_tag_row;
    TBagTagRow bag_sum_row;
    bag_sum_row.amount = 0;
    bag_sum_row.weight = 0;
    vector<t_tag_nos_row> tag_nos;
    vector<TBagTagRow> bm_table;
    int bag_sum_idx = -1;
    vector<TBag2PK> bag2_pks;
    for(vector<TBagTagRow>::iterator iv = bag_tags.begin(); iv != bag_tags.end(); iv++) {
        if(
                !(
                    bag_tag_row.last_target == iv->last_target &&
                    bag_tag_row.airp_arv == iv->airp_arv &&
                    bag_tag_row.class_code == iv->class_code &&
                    bag_tag_row.bag_name == iv->bag_name &&
                    bag_tag_row.tag_type == iv->tag_type &&
                    bag_tag_row.color == iv->color
                 )
          ) {
            if(!tag_nos.empty()) {
                bm_table.back().tag_range = get_tag_range(tag_nos, pr_lat);
                bm_table.back().num = tag_nos.size();
                tag_nos.clear();
            }
            bag_tag_row.last_target = iv->last_target;
            bag_tag_row.airp_arv = iv->airp_arv;
            bag_tag_row.class_code = iv->class_code;
            bag_tag_row.bag_name = iv->bag_name;
            bag_tag_row.tag_type = iv->tag_type;
            bag_tag_row.color = iv->color;
            bm_table.push_back(*iv);
            if(iv->class_code.empty())
                bm_table.back().class_name = pr_lat ? "Unacompanied baggage" : "Несопровождаемый багаж";
            if(iv->color.empty())
                bm_table.back().color = "-";
            else
                bm_table.back().color = base_tables.get("tag_colors").get_row("code", iv->color).AsString("name", pr_lat);
            bm_table.back().amount = 0;
            bm_table.back().weight = 0;
            if(
                    !(
                        bag_sum_row.last_target == iv->last_target &&
                        bag_sum_row.airp_arv == iv->airp_arv &&
                        bag_sum_row.class_code == iv->class_code
                     )
              ) {
                if(bag_sum_row.amount != 0) {
                    bm_table[bag_sum_idx].amount = bag_sum_row.amount;
                    bm_table[bag_sum_idx].weight = bag_sum_row.weight;
                    bag_sum_row.amount = 0;
                    bag_sum_row.weight = 0;
                    bag2_pks.clear();
                }
                bag_sum_row.airp_arv = iv->airp_arv;
                bag_sum_row.class_code = iv->class_code;
                bag_sum_idx = bm_table.size() - 1;
            }
        }
        TBag2PK bag2__pk;
        bag2__pk.grp_id = iv->grp_id;
        bag2__pk.num = iv->bag_num;
        if(find(bag2_pks.begin(), bag2_pks.end(), bag2__pk) == bag2_pks.end()) {
            bag2_pks.push_back(bag2__pk);
            bag_sum_row.amount += iv->amount;
            bag_sum_row.weight += iv->weight;
        }
        t_tag_nos_row tag_nos_row;
        tag_nos_row.pr_liab_limit = iv->pr_liab_limit;
        tag_nos_row.no = iv->no;
        tag_nos.push_back(tag_nos_row);
    }
    if(!tag_nos.empty()) {
        bm_table.back().tag_range = get_tag_range(tag_nos, pr_lat);
        bm_table.back().num = tag_nos.size();
        tag_nos.clear();
    }
    if(bag_sum_row.amount != 0) {
        bm_table[bag_sum_idx].amount = bag_sum_row.amount;
        bm_table[bag_sum_idx].weight = bag_sum_row.weight;
        bag_sum_row.amount = 0;
        bag_sum_row.weight = 0;
    }
    dump_bag_tags(bm_table);


    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, pr_trfer ? "v_bm_trfer" : "v_bm");

    for(vector<TBagTagRow>::iterator iv = bm_table.begin(); iv != bm_table.end(); iv++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        string airp_arv = iv->airp_arv;
        TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp_arv);
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "airp_arv_name", airpRow.AsString("name", pr_lat));
        NewTextChild(rowNode, "pr_trfer", iv->pr_trfer);
        NewTextChild(rowNode, "last_target", iv->last_target);
        NewTextChild(rowNode, "bag_name", iv->bag_name);
        NewTextChild(rowNode, "birk_range", iv->tag_range);
        NewTextChild(rowNode, "color", iv->color);
        NewTextChild(rowNode, "num", iv->num);
        NewTextChild(rowNode, "pr_vip", pr_vip);

        if(!iv->class_code.empty()) {
            TBaseTableRow &classesRow = base_tables.get("CLASSES").get_row("code",iv->class_code);
            iv->class_code = classesRow.AsString("code", pr_lat);
            iv->class_name = classesRow.AsString("name", pr_lat);
        }

        NewTextChild(rowNode, "class", iv->class_code);
        NewTextChild(rowNode, "class_name", iv->class_name);
        NewTextChild(rowNode, "amount", iv->amount);
        NewTextChild(rowNode, "weight", iv->weight);
        Qry.Next();
    }
    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "pr_lat", pr_lat);
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   scd_out, "
        "   airp_fmt, "
        "   airline_fmt, "
        "   suffix_fmt, "
        "   craft_fmt "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(point_id));

    int airline_fmt = Qry.FieldAsInteger("airline_fmt");
    int suffix_fmt = Qry.FieldAsInteger("suffix_fmt");
    int craft_fmt = Qry.FieldAsInteger("craft_fmt");

    string airline = Qry.FieldAsString("airline");
    string suffix = Qry.FieldAsString("suffix");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
    //    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlineRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "flt",
            ElemIdToElem(etAirline, airline, airline_fmt) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            ElemIdToElem(etSuffix, suffix, suffix_fmt)
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", ElemIdToElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    string airp_arv_name;
    if(target.size())
        airp_arv_name = base_tables.get("AIRPS").get_row("code",target).AsString("name",pr_lat);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    {
        // delete in future 14.10.07 !!!
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    int TotAmount = 0;
    int TotWeight = 0;
    Qry.Clear();
    SQLText =
        "SELECT NVL(SUM(amount),0) AS amount, "
        "       NVL(SUM(weight),0) AS weight "
        "FROM pax_grp,bag2 "
        "   ,halls2 ";
    if(pr_brd_pax != -1)
        SQLText +=
            "   ,pax ";
    SQLText +=
        "WHERE pax_grp.grp_id=bag2.grp_id AND "
        "      pax_grp.point_dep=:point_id and "
        "      pax_grp.bag_refuse=0 AND "
        "      pax_grp.hall=halls2.id AND ";
    if(pr_vip != 2) {
        SQLText +=
            "      halls2.pr_vip=:pr_vip AND ";
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    }
    if(zoneNode) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
        Qry.CreateVariable("zone", otString, zone);
    }
    if(pr_brd_pax != -1) {
        SQLText +=
            "   pax.pax_id(+) = ckin.get_main_pax_id(pax_grp.grp_id) and "
            "   decode(:pr_brd_pax, 0, nvl2(pax.pr_brd(+), 0, -1), pax.pr_brd(+))  = :pr_brd_pax and "
            "   (pax_grp.class is not null and pax.pax_id is not null or pax_grp.class is null) and ";
        Qry.CreateVariable("pr_brd_pax", otInteger, pr_brd_pax);
    }
    if(target.size())
        SQLText +=
            "      pax_grp.airp_arv=:target and ";
    SQLText +=
        "      bag2.pr_cabin=0 ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    if(target.size())
        Qry.CreateVariable("target", otString, target);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    if(Qry.RowCount() > 0) {
        TotAmount += Qry.FieldAsInteger("amount");
        TotWeight += Qry.FieldAsInteger("weight");
    }

    NewTextChild(variablesNode, "TotPcs", TotAmount);
    NewTextChild(variablesNode, "TotWeight", TotWeight);
    NewTextChild(variablesNode, "Tot", (pr_lat ? "" : vs_number(TotAmount)));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
    string pr_brd_pax_str_lat;
    string pr_brd_pax_str;
    if(pr_brd_pax != -1) {
        if(pr_brd_pax == 0) {
            pr_brd_pax_str_lat = "(checked)";
            pr_brd_pax_str = "(зарег)";
        } else {
            pr_brd_pax_str_lat = "(boarded)";
            pr_brd_pax_str = "(посаж)";
        }
    }
    NewTextChild(variablesNode, "pr_brd_pax_lat", pr_brd_pax_str_lat);
    NewTextChild(variablesNode, "pr_brd_pax", pr_brd_pax_str);
    if(zoneNode) {
            NewTextChild(variablesNode, "zone", get_hall_list(airp, zone, pr_lat));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    STAT::set_variables(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
}

void RunTest3(xmlNodePtr formDataNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    trips.trip, "
        "    pax.grp_id, "
        "    pax.surname "
        "from "
        "    pax, "
        "    pax_grp, "
        "    trips "
        "where "
        "    pax.grp_id in ( "
        "            2408, "
        "            2141, "
        "            2142, "
        "            2499, "
        "            2152, "
        "            2161, "
        "            2163, "
        "            2167, "
        "            2169 "
        "    ) and "
        "    pax.grp_id = pax_grp.grp_id and "
        "    pax_grp.point_id = trips.trip_id "
        "order by "
        "    trips.trip, "
        "    pax.grp_id, "
        "    pax.surname ";
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "pax_list");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "trip", Qry.FieldAsString("trip"));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "surname", Qry.FieldAsString("surname"));
        Qry.Next();
    }
}

void RunTest2(xmlNodePtr formDataNode)
{
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");

    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "Customers");

    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "CustNo", "1221");
    NewTextChild(rowNode, "Company", "Kauai Dive Shoppe");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "CustNo", "1231");
    NewTextChild(rowNode, "Company", "Unisco");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "CustNo", "1351");
    NewTextChild(rowNode, "Company", "Sight Diver");

    dataSetNode = NewTextChild(dataSetsNode, "Orders");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1003");
    NewTextChild(rowNode, "CustNo", "1351");
    NewTextChild(rowNode, "SaleDate", "12.04.1988");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1023");
    NewTextChild(rowNode, "CustNo", "1221");
    NewTextChild(rowNode, "SaleDate", "01.07.1988");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1052");
    NewTextChild(rowNode, "CustNo", "1351");
    NewTextChild(rowNode, "SaleDate", "06.01.1989");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1055");
    NewTextChild(rowNode, "CustNo", "1351");
    NewTextChild(rowNode, "SaleDate", "04.02.1989");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1060");
    NewTextChild(rowNode, "CustNo", "1231");
    NewTextChild(rowNode, "SaleDate", "28.02.1989");

    rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "OrderNo", "1123");
    NewTextChild(rowNode, "CustNo", "1221");
    NewTextChild(rowNode, "SaleDate", "24.08.1993");
}

void RunTest1(xmlNodePtr formDataNode)
{
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    TQuery Qry(&OraSession);
    Qry.SQLText = "select kod_ak, ak_name from avia order by kod_ak, ak_name";
    Qry.Execute();
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "avia");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "kod_ak", Qry.FieldAsString("kod_ak"));
        NewTextChild(rowNode, "ak_name", Qry.FieldAsString("ak_name"));
        Qry.Next();
    }
    Qry.SQLText = "select code, name from persons order by code, name";
    Qry.Execute();
    dataSetNode = NewTextChild(dataSetsNode, "persons");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "code", Qry.FieldAsString("code"));
        NewTextChild(rowNode, "name", Qry.FieldAsString("name"));
        Qry.Next();
    }
    Qry.SQLText = "select cod, name from cities order by cod, name";
    Qry.Execute();
    dataSetNode = NewTextChild(dataSetsNode, "cities");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "cod", Qry.FieldAsString("cod"));
        NewTextChild(rowNode, "name", Qry.FieldAsString("name"));
        Qry.Next();
    }
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "den_var", "Dennis\n\"Zakharoff\"");
    NewTextChild(variablesNode, "hello", "Hello world!!!");
}

void get_report_form(const string name, xmlNodePtr node)
{
    string form;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select form from fr_forms where name = :name";
    Qry.CreateVariable("name", otString, name);
    Qry.Execute();
    if(Qry.Eof) {
        NewTextChild(node, "FormNotExists", name);
        SetProp(NewTextChild(node, "form"), "name", name);
        return;
    }
    // положим в ответ шаблон отчета
    int len = Qry.GetSizeLongField("form");
    void *data = malloc(len);
    if ( data == NULL )
        throw Exception("DocsInterface::RunReport malloc failed");
    try {
        Qry.FieldAsLong("form", data);
        form.clear();
        form.append((char *)data, len);
    } catch(...) {
        free(data);
    }
    free(data);
    SetProp(ReplaceTextChild(node, "form", form), "name", name);
}

void RunRpt(string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string form;
    get_report_form(name, resNode);

    // теперь положим данные для отчета
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");

    if(name == "test1") RunTest1(formDataNode);
    // отчет test2 связывание 2-х датасетов это бааалшой вопрос.
    else if(name == "test2") RunTest2(formDataNode);
    // group test
    else if(name == "test3") RunTest3(formDataNode);
    else if(name == "BMTrfer") RunBMNew(reqNode, formDataNode);
    else if(name == "BM") RunBMNew(reqNode, formDataNode);
    else if(name == "PMTrfer" || name == "PM" || name == "PMDMD") RunPMNew(name, reqNode, formDataNode);
    else if(name == "notpres") RunNotpres(reqNode, formDataNode);
    else if(name == "ref") RunRef(reqNode, formDataNode);
    else if(name == "rem") RunRem(reqNode, formDataNode);
    else if(name == "crs" || name == "crsUnreg") RunCRS(name, reqNode, formDataNode);
//    else if(name == "EventsLog") RunEventsLog(reqNode, formDataNode);
    else if(name == "SZV") RunSZV(reqNode, formDataNode);
    else if(name == "FullStat") ;
    else if(name == "ShortStat") ;
    else if(name == "DetailStat") ;
    else if(name == "ArxPaxList") ;
    else if(name == "ArxPaxLog") ;
    else if(name == "PNLPaxList") ;
    else if(name == "SeasonList") ;
    else if(name == "SeasonEventsLog") ;
    else if(name == "exam") RunExam(reqNode, formDataNode);
    else
        throw UserException("data handler not found for " + name);
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "test_server", get_test_server());
}

struct TPMTotalsKey {
    int point_id;
    int pr_trfer;
    string target;
    string status;
    string cls;
    string cls_name;
    int lvl;
    TPMTotalsKey():
        point_id(NoExists),
        pr_trfer(NoExists),
        lvl(NoExists)
    {
    };
};

struct TPMTotalsCmp {
    bool operator() (const TPMTotalsKey &l, const TPMTotalsKey &r) const
    {
        if(l.point_id == r.point_id)
            if(l.target == r.target)
                if(l.pr_trfer == r.pr_trfer)
                    if(l.lvl == r.lvl)
                        if(l.status == r.status)
                            return l.cls < r.cls;
                        else
                            return l.status < r.status;
                    else
                        return l.lvl < r.lvl;
                else
                    return l.pr_trfer < r.pr_trfer;
            else
                return l.target < r.target;
        else
            return l.point_id < r.point_id;
    }
};

struct TPMTotalsRow {
    int seats, adl, chd, inf, rk_weight, bag_amount, bag_weight, excess;
    TPMTotalsRow():
        seats(0),
        adl(0),
        chd(0),
        inf(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        excess(0)
    {};
};

typedef map<TPMTotalsKey, TPMTotalsRow, TPMTotalsCmp> TPMTotals;

void PTM(TRptParams &rpt_params, xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    int pr_lat = GetRPEncoding(rpt_params);
    if(rpt_params.airp_arv.empty()) {
        if(rpt_params.pr_trfer)
            get_report_form("PMTrferTotalEL", resNode);
        else
            get_report_form("PMTotalEL", resNode);
    } else {
        if(rpt_params.pr_trfer)
            get_report_form("PMTrfer", resNode);
        else
            get_report_form("PM", resNode);
    }
    {
        string et, et_lat;
        if(rpt_params.pr_et) {
            et = "(ЭБ)";
            et_lat = "(ET)";
        }
        NewTextChild(variablesNode, "et", et);
        NewTextChild(variablesNode, "et_lat", et_lat);
    }
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT \n"
        "   pax.pax_id, \n"
        "   pax_grp.point_dep AS trip_id, \n"
        "   pax_grp.airp_arv AS target, \n";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, \n"
            "    trfer_trips.airline trfer_airline, \n"
            "    trfer_trips.flt_no trfer_flt_no, \n"
            "    trfer_trips.suffix trfer_suffix, \n"
            "    transfer.airp_arv trfer_airp_arv, \n"
            "    trfer_trips.scd trfer_scd, \n";
    SQLText +=
        "   DECODE(:pr_lat,0,classes.code,nvl(classes.code_lat, classes.code)) AS class, \n"
        "   DECODE(:pr_lat,0,classes.name,nvl(classes.name_lat, classes.name)) AS class_name, \n"
        "   DECODE(pax_grp.status, 'T', pax_grp.status, 'N') status, \n"
        "   classes.priority lvl, \n"
        "   surname||' '||pax.name AS full_name, \n"
        "   pax.pers_type, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum,0) AS seat_no, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum,1) AS seat_no_lat, \n"
        "   pax.seats, \n";
    if(rpt_params.pr_et) { //ЭБ
        SQLText +=
            "    ticket_no||'/'||coupon_no AS remarks, \n";
    } else {
        SQLText +=
            " SUBSTR(report.get_remarks(pax_id,0),1,250) AS remarks, \n";
    }
    SQLText +=
        "   NVL(ckin.get_rkWeight(pax.grp_id,pax.pax_id),0) AS rk_weight, \n"
        "   NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight(pax.grp_id,pax.pax_id),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) AS excess, \n"
        "   ckin.get_birks(pax.grp_id,pax.pax_id,:pr_lat) AS tags, \n"
        "   reg_no, \n"
        "   pax_grp.grp_id \n"
        "FROM  \n"
        "   pax_grp, \n"
        "   points, \n"
        "   pax, \n"
        "   cls_grp classes, \n"
        "   halls2 \n";
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips \n";
    SQLText +=
        "WHERE \n"
        "   points.pr_del>=0 AND \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.point_arv = points.point_id and \n"
        "   pax_grp.grp_id=pax.grp_id AND \n"
        "   pax_grp.class_grp = classes.id AND \n"
        "   pax_grp.hall = halls2.id and \n"
        "   pr_brd IS NOT NULL and \n"
        "   decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
    Qry.CreateVariable("pr_brd_pax", otInteger, rpt_params.pr_brd);
    if(rpt_params.pr_et) //ЭБ
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(not rpt_params.airp_arv.empty()) { // сегмент
        SQLText +=
            "    pax_grp.airp_arv = :target AND \n";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    SQLText +=
        "       DECODE(pax_grp.status, 'T', pax_grp.status, 'N') in ('T', 'N') \n";
    if(rpt_params.pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";
    SQLText +=
        "ORDER BY \n";
    if(rpt_params.airp_arv.empty())
        SQLText +=
            "   points.point_num, \n";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    PR_TRFER ASC, \n"
            "    TRFER_AIRP_ARV ASC, \n";
    SQLText +=
        "    CLASS ASC, \n"
        "    grp_id, \n"
        "    REG_NO ASC \n";
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer");
    // следующие 2 переменные введены для нужд FastReport
    map<string, int> fr_target_ref;
    int fr_target_ref_idx = 0;

    TPMTotals PMTotals;
    for(; !Qry.Eof; Qry.Next()) {
        if(not rpt_params.mkt_flt.IsNULL()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(Qry.FieldAsInteger("pax_id"));
            ProgTrace(TRACE5, "PAX_ID: %d", Qry.FieldAsInteger("pax_id"));
            mkt_flt.dump();
            rpt_params.mkt_flt.dump();
            ProgTrace(TRACE5, "--------------------------------------------------");
            if(mkt_flt.IsNULL() or not(rpt_params.mkt_flt == mkt_flt))
                continue;
        }

        TPMTotalsKey key;
        key.point_id = Qry.FieldAsInteger("trip_id");
        key.target = Qry.FieldAsString("target");
        key.status = Qry.FieldAsString("status");
        key.cls = Qry.FieldAsString("class");
        key.cls_name = Qry.FieldAsString("class_name");
        key.lvl = Qry.FieldAsInteger("lvl");
        if(rpt_params.pr_trfer) {
            key.pr_trfer = Qry.FieldAsInteger("pr_trfer");
        }
        TPMTotalsRow &row = PMTotals[key];
        row.seats += Qry.FieldAsInteger("seats");
        {
            TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type"));
            switch(pers_type) {
                case adult:
                    row.adl++;
                    break;
                case child:
                    row.chd++;
                    break;
                case baby:
                    row.inf++;
                    break;
                default:
                    throw Exception("DecodePerson failed");
            }
        }
        row.rk_weight += Qry.FieldAsInteger("rk_weight");
        row.bag_amount += Qry.FieldAsInteger("bag_amount");
        row.bag_weight += Qry.FieldAsInteger("bag_weight");
        row.excess += Qry.FieldAsInteger("excess");


        string cls = Qry.FieldAsString("class");
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", Qry.FieldAsString("reg_no"));
        NewTextChild(rowNode, "full_name", transliter(Qry.FieldAsString("full_name"), pr_lat));
        string last_target;
        int pr_trfer = 0;
        if(rpt_params.pr_trfer) {
            last_target = get_last_target(Qry, pr_lat);
            pr_trfer = Qry.FieldAsInteger("pr_trfer");
        }
        NewTextChild(rowNode, "last_target", last_target);
        NewTextChild(rowNode, "pr_trfer", pr_trfer);

        string airp_arv = Qry.FieldAsString("target");
        if(fr_target_ref.find(airp_arv) == fr_target_ref.end())
            fr_target_ref[airp_arv] = fr_target_ref_idx++;
        TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp_arv);
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "fr_target_ref", fr_target_ref[airp_arv]);
        NewTextChild(rowNode, "airp_arv_name", airpRow.AsString("name", pr_lat));

        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "class_name", Qry.FieldAsString("class_name"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "seats", Qry.FieldAsInteger("seats"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        string pers_type = Qry.FieldAsString("pers_type");
        if(pers_type == "ВЗ")
            NewTextChild(rowNode, "pers_type", "ADL");
        else if(pers_type == "РБ")
            NewTextChild(rowNode, "pers_type", "CHD");
        else if(pers_type == "РМ")
            NewTextChild(rowNode, "pers_type", "INF");
        else
            throw Exception("RunPM: unknown pers_type " + pers_type);
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        if (pr_lat==0)
            NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        else
            NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no_lat"));
        NewTextChild(rowNode, "remarks", Qry.FieldAsString("remarks"));
    }

    dataSetNode = NewTextChild(dataSetsNode, rpt_params.pr_trfer ? "v_pm_trfer_total" : "v_pm_total");

    for(TPMTotals::iterator im = PMTotals.begin(); im != PMTotals.end(); im++) {
        const TPMTotalsKey &key = im->first;
        TPMTotalsRow &row = im->second;

        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", key.point_id);
        if(rpt_params.pr_trfer) {
            NewTextChild(rowNode, "target", key.target);
            NewTextChild(rowNode, "fr_target_ref", fr_target_ref[key.target]);
            NewTextChild(rowNode, "pr_trfer", key.pr_trfer);
        }
        NewTextChild(rowNode, "status", key.status);
        NewTextChild(rowNode, "class_name", key.cls_name);
        NewTextChild(rowNode, "lvl", key.lvl);
        NewTextChild(rowNode, "seats", row.seats);
        NewTextChild(rowNode, "adl", row.adl);
        NewTextChild(rowNode, "chd", row.chd);
        NewTextChild(rowNode, "inf", row.inf);
        NewTextChild(rowNode, "rk_weight", row.rk_weight);
        NewTextChild(rowNode, "bag_amount", row.bag_amount);
        NewTextChild(rowNode, "bag_weight", row.bag_weight);
        NewTextChild(rowNode, "excess", row.excess);
    }

    // Теперь переменные отчета
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   scd_out, "
        "   airp_fmt, "
        "   airline_fmt, "
        "   suffix_fmt, "
        "   craft_fmt "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunPM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    int airline_fmt = Qry.FieldAsInteger("airline_fmt");
    int suffix_fmt = Qry.FieldAsInteger("suffix_fmt");
    int craft_fmt = Qry.FieldAsInteger("craft_fmt");

    string airp = Qry.FieldAsString("airp");
    string airline, suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.IsNULL()) {
        airline = Qry.FieldAsString("airline");
        flt_no = Qry.FieldAsInteger("flt_no");
        suffix = Qry.FieldAsString("suffix");
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
    }
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
    //    TCrafts crafts;

    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params.ckin_zone, pr_lat));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlineRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "flt",
            ElemIdToElem(etAirline, airline, airline_fmt) +
            IntToString(flt_no) +
            ElemIdToElem(etSuffix, suffix, suffix_fmt)
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", ElemIdToElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    string airp_arv_name;
    if(not rpt_params.airp_arv.empty())
        airp_arv_name = base_tables.get("AIRPS").get_row("code",rpt_params.airp_arv).AsString("name",pr_lat);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));

    NewTextChild(variablesNode, "pr_vip", 2);
    string pr_brd_pax_str_lat;
    string pr_brd_pax_str;
    if(rpt_params.pr_brd) {
        pr_brd_pax_str_lat = "(boarded)";
        pr_brd_pax_str = "(посаж)";
    } else {
        pr_brd_pax_str_lat = "(checked)";
        pr_brd_pax_str = "(зарег)";
    }
    NewTextChild(variablesNode, "pr_brd_pax_lat", pr_brd_pax_str_lat);
    NewTextChild(variablesNode, "pr_brd_pax", pr_brd_pax_str);
    STAT::set_variables(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
}

void  DocsInterface::RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    TRptParams rpt_params;
    rpt_params.point_id = NodeAsIntegerFast("point_id", node);
    rpt_params.rpt_type = DecodeRptType(NodeAsStringFast("rpt_type", node));
    rpt_params.airp_arv = NodeAsStringFast("airp_arv", node, "");
    rpt_params.ckin_zone = NodeAsStringFast("ckin_zone", node, " ");
    rpt_params.pr_et = NodeAsIntegerFast("pr_et", node, 0) != 0;
    rpt_params.pr_trfer = NodeAsIntegerFast("pr_trfer", node, 0) != 0;
    rpt_params.pr_brd = NodeAsIntegerFast("pr_brd", node, 0) != 0;
    xmlNodePtr mktFltNode = GetNodeFast("mkt_flight", node);
    if(mktFltNode != NULL) {
        xmlNodePtr node = mktFltNode->children;
        rpt_params.mkt_flt.airline = NodeAsStringFast("airline", node);
        rpt_params.mkt_flt.flt_no = NodeAsIntegerFast("flt_no", node);
        rpt_params.mkt_flt.suffix = NodeAsStringFast("suffix", node, "");
    }
    switch(rpt_params.rpt_type) {
        case rtPTM:
            PTM(rpt_params, resNode);
            break;
        case rtBTM:
        case rtREFUSE:
        case rtNOTPRES:
        case rtREM:
        case rtCRS:
        case rtCRSUNREG:
        case rtEXAM:
        case rtUnknown:
        case rtTypeNum:
            throw UserException("Отчет временно не поддерживается системой");
    }
}

void  DocsInterface::RunReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);
    try {
        RunRpt(name, reqNode, resNode);
    } catch( EOracleError E ) {
        if ( E.Code >= 20000 ) {
            string str = E.what();
            EOracleError2UserException(str);
            throw UserException( str.c_str() );
        } else
            throw;
    }
}

void  DocsInterface::SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);

    /*
    if(
            name == "BMTrfer" ||
            name == "BM" ||
            name == "PMTrfer" ||
            name == "PM" ||
            name == "notpres" ||
            name == "ref" ||
            name == "rem" ||
            name == "crs" ||
            name == "crsUnreg"
            )
        throw UserException("Запись " + name + " запрещена");
        */

    string form = NodeAsString("form", reqNode);
    Qry.SQLText = "update fr_forms set form = :form where name = :name";
    Qry.CreateVariable("name", otString, name);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.Execute();
    if(!Qry.RowsProcessed()) {
        Qry.SQLText = "insert into fr_forms(id, name, form) values(id__seq.nextval, :name, :form)";
        Qry.Execute();
    }
}

void  DocsInterface::LoadForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int id = NodeAsInteger("id", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText = "select form from fr_forms where id = :id";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("form not found, id = " + IntToString(id));
    int len = Qry.GetSizeLongField("form");
    void *data = malloc(len);
    if ( data == NULL )
        throw Exception("DocsInterface::LoadForm malloc failed");
    try {
        Qry.FieldAsLong("form", data);
        string form((char *)data, len);
        NewTextChild(resNode, "form", form);
    } catch(...) {
        free(data);
    }
    free(data);
}

void  DocsInterface::SaveForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string form = NodeAsString("form", reqNode);
    ProgTrace(TRACE5, "%s", form.c_str());
    int id = NodeAsInteger("id", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
//        "insert into fr_forms(id, form) values(id__seq.nextval, :form)";
        "update fr_forms set form = :form where id = :id";
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
}

void DocsInterface::GetFltInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT "
    "  point_id, "
    "  airline||TO_CHAR(flt_no)||suffix AS trip, "
    "  scd_out, "
    "  airline,flt_no,suffix,airp, "
    "  craft, "
    "  bort, "
    "  trip_type, "
    "  ckin.get_pr_tranz_reg(point_id) AS pr_tranz_reg, "
    "  park_out "
    "FROM  points "
    "WHERE point_id= :point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable("point_id",otInteger,NodeAsInteger("point_id",reqNode));
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");
  NewTextChild(resNode,"point_id",Qry.FieldAsInteger("point_id"));
  NewTextChild(resNode,"trip",Qry.FieldAsString("trip"));
  NewTextChild(resNode,"airline",Qry.FieldAsString("airline"));
  NewTextChild(resNode,"flt_no",Qry.FieldAsInteger("flt_no"));
  NewTextChild(resNode,"suffix",Qry.FieldAsString("suffix"));
  NewTextChild(resNode,"airp",Qry.FieldAsString("airp"));
  NewTextChild(resNode,"craft",Qry.FieldAsString("craft"));
  NewTextChild(resNode,"bort",Qry.FieldAsString("bort"));
  NewTextChild(resNode,"trip_type",Qry.FieldAsString("trip_type"));
  NewTextChild(resNode,"pr_tranz_reg",(int)(Qry.FieldAsInteger("pr_tranz_reg")!=0));
  NewTextChild(resNode,"park_out",Qry.FieldAsString("park_out"));

  TDateTime scd_out;
  string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));
  scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
  NewTextChild( resNode, "scd_out", DateTimeToStr(scd_out) );
};



vector<string> get_grp_zone_list(int point_id)
{
    vector<string> result;
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select distinct  "
        "   rpt_grp  "
        "from  "
        "   points,  "
        "   halls2  "
        "where  "
        "   points.point_id = :point_id and "
        "   halls2.airp = points.airp "
        "order by  "
        "   rpt_grp ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
        result.push_back(Qry.FieldAsString("rpt_grp"));
    if(result.size() == 1 and result[0].empty())
        result[0] = " "; // группа залов "все залы"
    if(result.size() > 1 or result.empty())
        result.push_back(" ");
    return result;
}

void DocsInterface::GetZoneList(int point_id, xmlNodePtr dataNode)
{
    vector<string> zone_list = get_grp_zone_list(point_id);
    xmlNodePtr ckin_zonesNode = NewTextChild(dataNode, "ckin_zones");
    for(vector<string>::iterator iv = zone_list.begin(); iv != zone_list.end(); iv++) {
        xmlNodePtr itemNode = NewTextChild(ckin_zonesNode, "item");
        if(iv->empty()) {
            NewTextChild(itemNode, "code");
            NewTextChild(itemNode, "name", "Др. залы");
        } else if(*iv == " ") {
            NewTextChild(itemNode, "code", *iv);
            NewTextChild(itemNode, "name", "Все залы");
        } else {
            NewTextChild(itemNode, "code", *iv);
            NewTextChild(itemNode, "name", *iv);
        }
    }
}

void DocsInterface::GetSegList2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int get_tranzit = NodeAsInteger("get_tranzit", reqNode);
    string rpType = NodeAsString("rpType", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp,point_num, "
        "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
        "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Рейс не найден. Обновите данные.");

    int first_point = Qry.FieldAsInteger("first_point");
    int point_num = Qry.FieldAsInteger("point_num");
    string own_airp = Qry.FieldAsString("airp");
    string prev_airp, curr_airp;

    vector<string> zone_list = get_grp_zone_list(point_id);
    xmlNodePtr SegListNode = NewTextChild(resNode, "SegList");

    for(int j = -1; j <= 1; j++) {
        if(j == 0) {
            curr_airp = own_airp;
            continue;
        }
        Qry.Clear();
        string SQLText = "select airp from points ";
        if(j == -1)
            SQLText +=
                "where :first_point in(first_point,point_id) and point_num<:point_num and pr_del=0 "
                "order by point_num desc ";
        else
            SQLText +=
                "where first_point=:first_point and point_num>:point_num and pr_del=0 "
                "order by point_num asc ";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("first_point", otInteger, first_point);
        Qry.CreateVariable("point_num", otInteger, point_num);
        Qry.Execute();
        while(!Qry.Eof) {
            string airp = Qry.FieldAsString("airp");
            if(j == -1) {
                if(get_tranzit) prev_airp = airp;
                break;
            }

            for(vector<string>::iterator iv = zone_list.begin(); iv != zone_list.end(); iv++) {
                if(prev_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "T");
                    NewTextChild(SegNode, "airp_dep_code", prev_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", 2);
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                      ) {
                        string hall;
                        if(iv->empty()) {
                            NewTextChild(SegNode, "zone");
                            hall = " (др. залы)";
                        } else if(*iv != " ") {
                            NewTextChild(SegNode, "zone", *iv);
                            hall = " (" + *iv + ")";
                        }
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + hall);
                    }
                }
                if(curr_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "N");
                    NewTextChild(SegNode, "airp_dep_code", curr_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", 2);
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                      ) {
                        string hall;
                        if(iv->empty()) {
                            NewTextChild(SegNode, "zone");
                            hall = " (др. залы)";
                        } else if(*iv != " ") {
                            NewTextChild(SegNode, "zone", *iv);
                            hall = " (" + *iv + ")";
                        }
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp + hall);
                    }
                }
                if(!(rpType == "BM" || rpType == "TBM" || rpType == "PM" || rpType == "TPM"))
                    break;
            }
            Qry.Next();
        }
    }
    if(
            rpType == "PM" ||
            rpType == "TPM" ||
            rpType == "TBM" ||
            rpType == "BM"
      ) {
        xmlNodePtr SegNode;
        if(
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "TPM")
                item = "etm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", 2);
            NewTextChild(SegNode, "item", "Список ЭБ");
        }

        if(
                rpType == "BM" ||
                rpType == "TBM" ||
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "PM")
                item = "tot";
            else if(rpType == "TPM")
                item = "tpm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", 2);
            NewTextChild(SegNode, "item", "Общая");
        }
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DocsInterface::GetSegList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int get_tranzit = NodeAsInteger("get_tranzit", reqNode);
    string rpType = NodeAsString("rpType", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp,point_num, "
        "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
        "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Рейс не найден. Обновите данные.");

    int first_point = Qry.FieldAsInteger("first_point");
    int point_num = Qry.FieldAsInteger("point_num");
    string own_airp = Qry.FieldAsString("airp");
    string prev_airp, curr_airp;

    xmlNodePtr SegListNode = NewTextChild(resNode, "SegList");

    for(int j = -1; j <= 1; j++) {
        if(j == 0) {
            curr_airp = own_airp;
            continue;
        }
        Qry.Clear();
        string SQLText = "select airp from points ";
        if(j == -1)
            SQLText +=
                "where :first_point in(first_point,point_id) and point_num<:point_num and pr_del=0 "
                "order by point_num desc ";
        else
            SQLText +=
                "where first_point=:first_point and point_num>:point_num and pr_del=0 "
                "order by point_num asc ";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("first_point", otInteger, first_point);
        Qry.CreateVariable("point_num", otInteger, point_num);
        Qry.Execute();
        while(!Qry.Eof) {
            string airp = Qry.FieldAsString("airp");
            if(j == -1) {
                if(get_tranzit) prev_airp = airp;
                break;
            }

            for(int pr_vip = 0; pr_vip <= 2; pr_vip++) {
                if(prev_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "T");
                    NewTextChild(SegNode, "airp_dep_code", prev_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", pr_vip);
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                            ) {
                        string hall;
                        switch(pr_vip) {
                            case 0:
                                hall = " (не VIP)";
                                break;
                            case 1:
                                hall = " (VIP)";
                                break;
                        }
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + hall);
                    } else
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + " (транзит)");
                }
                if(curr_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "N");
                    NewTextChild(SegNode, "airp_dep_code", curr_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", pr_vip);
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                            ) {
                        string hall;
                        switch(pr_vip) {
                            case 0:
                                hall = " (не VIP)";
                                break;
                            case 1:
                                hall = " (VIP)";
                                break;
                        }
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp + hall);
                    } else
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp);
                }
                if(!(rpType == "BM" || rpType == "TBM" || rpType == "PM" || rpType == "TPM"))
                    break;
            }
            Qry.Next();
        }
    }
    if(
            rpType == "PM" ||
            rpType == "TPM" ||
            rpType == "TBM" ||
            rpType == "BM"
      ) {
        xmlNodePtr SegNode;
        if(
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "TPM")
                item = "etm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", 2);
            NewTextChild(SegNode, "item", "Список ЭБ");
        }

        if(
                rpType == "BM" ||
                rpType == "TBM" ||
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "PM")
                item = "tot";
            else if(rpType == "TPM")
                item = "tpm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", 2);
            NewTextChild(SegNode, "item", "Общая");
        }
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DocsInterface::GetFonts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	tst();
  NewTextChild(resNode,"fonts","");
  tst();
}
