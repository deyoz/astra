#include "docs.h"
#include "oralib.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "str_utils.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "season.h"
#include "brd.h"
#include "xml_stuff.h"

#define SALEK

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

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

void PaxListVars(int point_id, int pr_lat, xmlNodePtr variablesNode, double f)
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
    if(f == NoExists)
        SQLText +=
        "   points "
        "where "
        "   point_id = :point_id ";
    else {
        SQLText +=
        "   arx_points "
        "where "
        "   part_key >= :f and "
        "   point_id = :point_id ";
        Qry.CreateVariable("f", otDate, f);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("PaxListVars: variables fetch failed for point_id " + IntToString(point_id));

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

int GetRPEncoding(string target)
{
    if(
            target.empty() ||
            target == "tot" ||
            target == "etm" ||
            target == "tpm"
            ) return 0;
    TBaseTable &airps = base_tables.get("AIRPS");
    TBaseTable &cities = base_tables.get("CITIES");
    return cities.get_row("code",
             airps.get_row("code",target).AsString("city")).AsString("country") != "РФ";
}

void RunCRS(string name, xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);
    string SQLText =
        "select  "
        "    v_crs.point_id, "
        "    v_crs.pnr_ref, "
        "    decode(:pr_lat, 0, v_crs.family, v_crs.family_lat) family, "
        "    decode(:pr_lat, 0, v_crs.pers_type, v_crs.pers_type_lat) pers_type, "
        "    decode(:pr_lat, 0, v_crs.class, v_crs.class_lat) class, "
        "    v_crs.seat_no, "
        "    decode(:pr_lat, 0, v_crs.target, v_crs.target_lat) target, "
        "    v_crs.last_target, "
        "    v_crs.ticket_no, "
        "    v_crs.document, "
        "    v_crs.remarks ";
    if(name == "crs")
        SQLText +=
            "from "
            "    v_crs "
            "where "
            "    v_crs.point_id = :point_id "
            "order by "
            "    v_crs.family ";
    else
        SQLText +=
            "from "
            "    v_crs, "
            "    pax "
            "where "
            "    v_crs.point_id = :point_id and "
            "    v_crs.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null "
            "order by "
            "    v_crs.family ";
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

void RunEventsLog(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
        "SELECT events.type type, msg, time, id1 AS point_id, "
        "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
        "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
        "       ev_user, station, ev_order "
        "FROM events "
        "WHERE type=:evtDisp AND events.id1=:move_id AND events.id2=:point_id "
        "UNION "
        "SELECT events.type type, msg, time, id1 AS point_id, "
        "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, "
        "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, "
        "       ev_user, station, ev_order "
        "FROM events "
        "WHERE type IN(:evtSeason,:evtFlt,:evtGraph,:evtPax,:evtPay,:evtComp,:evtTlg) AND events.id1=:point_id "
        " ORDER BY ev_order";
    //events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND
    int point_id = NodeAsInteger("point_id",reqNode);
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
    Qry.CreateVariable("evtSeason",otString,EncodeEventType(ASTRA::evtSeason));
    Qry.CreateVariable("evtFlt",otString,EncodeEventType(ASTRA::evtFlt));
    Qry.CreateVariable("evtGraph",otString,EncodeEventType(ASTRA::evtGraph));
    Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
    Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
    Qry.CreateVariable("evtComp",otString,EncodeEventType(ASTRA::evtComp));
    Qry.CreateVariable("evtTlg",otString,EncodeEventType(ASTRA::evtTlg));
    xmlNodePtr etNode = GetNode( "EventsTypes", reqNode );
    vector<string> eventsTypes;
    if ( etNode ) {
        etNode = etNode->children;
        while ( etNode ) {
            eventsTypes.push_back( NodeAsString( etNode->children ) );
            etNode = etNode->next;
        }
    }
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "events_log");

    for(;!Qry.Eof;Qry.Next())
    {
        if ( !eventsTypes.empty() &&
                find( eventsTypes.begin(), eventsTypes.end(), Qry.FieldAsString( "type" ) ) == eventsTypes.end() )
            continue;

        xmlNodePtr rowNode=NewTextChild(dataSetNode,"row");
        NewTextChild(rowNode,"point_id",Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode,"ev_user",Qry.FieldAsString("ev_user"));
        NewTextChild(rowNode,"station",Qry.FieldAsString("station"));

        TDateTime time;
        if (reqInfo->user.sets.time==ustTimeUTC)
            time = Qry.FieldAsDateTime("time");
        else
            time = UTCToLocal(Qry.FieldAsDateTime("time"), reqInfo->desk.tz_region);

        NewTextChild(rowNode,"time",DateTimeToStr(time));
        NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
        NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger("grp_id"),-1);
        NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger("reg_no"),-1);
        NewTextChild(rowNode,"msg",Qry.FieldAsString("msg"));
        NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger("ev_order"));
    };

    // Теперь переменные отчета
    PaxListVars(point_id, 0, NewTextChild(formDataNode, "variables"));
}

void RunExam(xmlNodePtr reqNode, xmlNodePtr &formDataNode)
{
    tst();
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

    tst();
    xmlNodeSetName(dataNode, (xmlChar *)"datasets");
    xmlAddChild(formDataNode, dataNode);
    tst();
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
        "select  "
        "    point_id, "
        "    reg_no, "
        "    decode(:pr_lat, 0, family, family_lat) family, "
        "    decode(:pr_lat, 0, pers_type, pers_type_lat) pers_type, "
        "    seat_no, "
        "    info "
        "from "
        "    v_rem "
        "where "
        "    point_id = :point_id and "
        "    info is not null "
        "order by "
        "    reg_no ";
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
        "select  "
        "    point_id, "
        "    reg_no, "
        "    decode(:pr_lat, 0, family, family_lat) family, "
        "    decode(:pr_lat, 0, pers_type, pers_type_lat) pers_type, "
        "    ticket_no, "
        "    decode(:pr_lat, 0, refuse, refuse_lat) refuse, "
        "    decode(:pr_lat, 0, tags, tags_lat) tags "
        "from "
        "    v_ref "
        "where "
        "    point_id = :point_id "
        "order by "
        "    reg_no ";
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
        "select  "
        "   point_id, "
        "   reg_no, "
        "   decode(:pr_lat, 0, family, family_lat) family, "
        "   pers_type, "
        "   seat_no, "
        "   bagamount, "
        "   bagweight, "
        "   decode(:pr_lat, 0, tags, tags_lat) tags "
        "from "
        "   v_notpres "
        "where "
        "   point_id = :point_id "
        "order by "
        "   reg_no ";
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

void RunPM(string name, xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);
    string status = NodeAsString("status", reqNode);

    if(
            target.empty() ||
            target == "tot"
      ) {
        xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
        xmlNodePtr formNode = NodeAsNode("form", resNode);
        xmlUnlinkNode(formNode);
        xmlFreeNode(formNode);
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
        xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
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
        "SELECT  "
        "    TRIP_ID, "
        "    TARGET, ";
    if(name == "PMTrfer")
        SQLText +=
            "    PR_TRFER, "
            "    decode(:pr_lat, 0, last_target, last_target_lat) last_target, ";
    SQLText +=
        "    class, "
        "    DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
        "    STATUS, "
        "    decode(:pr_lat, 0, full_name, full_name_lat) full_name, "
        "    PERS_TYPE, "
        "    SEAT_NO, "
        "    SEATS, ";
    if(
            target.empty() ||
            target == "etm"
      ) { //ЭБ
        SQLText +=
            "    nvl(decode(coupon_no, null, null, ticket_no||'/'||coupon_no), report.get_tkno(pax_id, '/', 1)) remarks, ";
    } else {
        SQLText +=
            "    remarks, ";
    }
    SQLText +=
        "    RK_WEIGHT, "
        "    BAG_AMOUNT, "
        "    BAG_WEIGHT, "
        "    EXCESS, "
        "    decode(:pr_lat, 0, tags, tags_lat) tags, "
        "    REG_NO, "
        "    GRP_ID "
        "FROM ";
    if(name == "PMTrfer")
        SQLText +=
            "    V_PM_TRFER ";
    else
        SQLText +=
            "    V_PM ";
    SQLText +=
        "WHERE "
        "    TRIP_ID = :point_id AND ";
    if(
            target.empty() ||
            target == "etm"
      ) { //ЭБ
        SQLText +=
            "   pr_brd = 1 and "
            "   ((ticket_no is not null and "
            "   coupon_no is not null) or "
            "   report.get_tkno(pax_id, '/', 1) is not null) and ";
    } else if(
            target == "tot" ||
            target == "tpm"
            ) { //ОБЩАЯ
    } else { // сегмент
        SQLText +=
            "    TARGET = :target AND ";
        if(pr_vip != 2)
            SQLText +=
                "    pr_vip = :pr_vip AND ";
    }
    if(status.size())
        SQLText +=
            "    STATUS = :status ";
    else
        SQLText +=
            "    STATUS in ('T', 'N') ";
    SQLText +=
        "ORDER BY ";
    if(
            target.empty() ||
            target == "tot" ||
            target == "etm" ||
            target == "tpm"
      )
        SQLText +=
            "   target, ";
    if(name == "PMTrfer")
        SQLText +=
            "    PR_TRFER ASC, "
            "    LAST_TARGET ASC, ";
    SQLText +=
        "    CLASS ASC, "
        "    grp_id, "
        "    REG_NO ASC ";


    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;

    Qry.CreateVariable("point_id", otInteger, point_id);
    if(
            target.size() &&
            target != "tot" &&
            target != "etm" &&
            target != "tpm"
      )
        Qry.CreateVariable("target", otString, target);
    if(status.size())
        Qry.CreateVariable("status", otString, status);
    if(pr_vip != 2)
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    Qry.CreateVariable("pr_lat", otString, pr_lat);

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
        NewTextChild(rowNode, "full_name", Qry.FieldAsString("full_name"));
        string last_target;
        int pr_trfer = 0;
        if(name == "PMTrfer") {
            last_target = Qry.FieldAsString("last_target");
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
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "remarks", Qry.FieldAsString("remarks"));
        Qry.Next();
    }

    if(name == "PMTrfer") {
        Qry.Clear();
        SQLText =
            "SELECT  "
            "    POINT_ID, "
            "    TARGET, "
            "    PR_TRFER, "
            "    STATUS, "
            "    CLASS, "
            "    DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
            "    LVL, "
            "    SEATS, "
            "    ADL, "
            "    CHD, "
            "    INF, "
            "    RK_WEIGHT, "
            "    BAG_AMOUNT, "
            "    BAG_WEIGHT, "
            "    EXCESS "
            "FROM ";
        if(pr_vip == 2)
            SQLText +=
                "    V_PM_TRFER_TOTAL ";
        else
            SQLText +=
                "    V_VIP_PM_TRFER_TOTAL ";
        SQLText +=
            "WHERE "
            "    POINT_ID = :point_id AND ";
        if(
                target.size() &&
                target != "tot" &&
                target != "etm" &&
                target != "tpm"
          ) {
            SQLText +=
                "    TARGET = :target AND ";
            if(pr_vip != 2)
                SQLText +=
                    "    pr_vip = :pr_vip AND ";
        }
        if(status.size())
            SQLText +=
                "    STATUS = :status ";
        else
            SQLText +=
                "    STATUS in ('T', 'N') ";
        SQLText +=
            "ORDER BY ";
        if(
                target.empty() ||
                target == "tot" ||
                target == "etm" ||
                target == "tpm"
          )
            SQLText +=
                "   target, ";
        SQLText +=
            "    PR_TRFER, "
            "    LVL ";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, point_id);
        if(
                target.size() &&
                target != "tot" &&
                target != "etm" &&
                target != "tpm"
          )
            Qry.CreateVariable("target", otString, target);
        if(status.size())
            Qry.CreateVariable("status", otString, status);
        if(pr_vip != 2)
            Qry.CreateVariable("pr_vip", otInteger, pr_vip);
        Qry.CreateVariable("pr_lat", otInteger, pr_lat);
        ProgTrace(TRACE5, "V_VIP_PM_TRFER_TOTAL SQLText: %s", SQLText.c_str());
        Qry.Execute();
        dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer_total");
        while(!Qry.Eof) {
            string cls = Qry.FieldAsString("class");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("POINT_ID"));
            string airp_arv = Qry.FieldAsString("TARGET");
            NewTextChild(rowNode, "target", airp_arv);
            NewTextChild(rowNode, "fr_target_ref", fr_target_ref[airp_arv]);
            NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("PR_TRFER"));
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
    } else {
        Qry.Clear();
        SQLText =
            "SELECT  "
            "    POINT_ID, "
            "    STATUS, "
            "    CLASS, "
            "    DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
            "    LVL, "
            "    SEATS, "
            "    ADL, "
            "    CHD, "
            "    INF, "
            "    RK_WEIGHT, "
            "    BAG_AMOUNT, "
            "    BAG_WEIGHT, "
            "    EXCESS "
            "FROM ";
        if(target.empty()) { //ЭБ
            SQLText +=
                "    V_PM_EL_TOTAL ";
        } else if(
                target == "tot" ||
                target == "tpm"
                ) { //ОБЩАЯ
            SQLText +=
                "    V_PM_TOTAL ";
        } else { // сегмент
            if(pr_vip == 2)
                SQLText +=
                    "    V_PM_TARGET_TOTAL ";
            else
                SQLText +=
                    "    V_VIP_PM_TARGET_TOTAL ";
        }
        SQLText +=
            "WHERE "
            "    POINT_ID = :point_id ";
        if(
                target.size() &&
                target != "tot" &&
                target != "tpm"
          ) {
            SQLText +=
                "   and airp_arv = :target ";
            if(pr_vip != 2)
                SQLText +=
                    " and pr_vip = :pr_vip ";
        }
        if(status.size())
            SQLText +=
                "    and STATUS = :status ";
        else
            SQLText +=
                "    and STATUS in ('T', 'N') ";
        SQLText +=
            "ORDER BY "
            "    LVL ";
        ProgTrace(TRACE5, "V_PM_TARGET_TOTAL SQLText: %s", SQLText.c_str());
        Qry.SQLText = SQLText;
        Qry.CreateVariable("point_id", otInteger, point_id);
        if(
                target.size() &&
                target != "tot" &&
                target != "tpm"
          )
            Qry.CreateVariable("target", otString, target);
        if(status.size())
            Qry.CreateVariable("status", otString, status);
        if(pr_vip != 2)
            Qry.CreateVariable("pr_vip", otInteger, pr_vip);
        Qry.CreateVariable("pr_lat", otInteger, pr_lat);
        Qry.Execute();
        dataSetNode = NewTextChild(dataSetsNode, "v_pm_total");
        while(!Qry.Eof) {
            string cls = Qry.FieldAsString("class");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("POINT_ID"));
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
        "   scd_out "
        "from "
        "   points "
        "where "
        "   point_id = :point_id ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunPM: variables fetch failed for point_id " + IntToString(point_id));

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
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
            airlineRow.AsString("code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", craft);
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    string airp_arv_name;
    if(
            target.size() &&
            target != "tot" &&
            target != "etm" &&
            target != "tpm"
      )
        airp_arv_name = base_tables.get("AIRPS").get_row("code",target).AsString("name",pr_lat);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));

    NewTextChild(variablesNode, "pr_vip", pr_vip);
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

string get_tag_range(vector<t_tag_nos_row> tag_nos, int pr_lat)
{
    string lim = (pr_lat ? "(lim)" : "(огр)");
    ostringstream result;
    sort(tag_nos.begin(), tag_nos.end(), lessTagNos);
    double first_no = -1.;
    double prev_no = -1.;
    double base = -1.;
    int pr_liab_limit = -1;
    ostringstream buf;
    for(vector<t_tag_nos_row>::iterator iv = tag_nos.begin(); iv != tag_nos.end(); iv++) {
        buf << fixed << setprecision(0) << iv->no <<  " ";
        double tmp_base = floor(iv->no / 1000);
        if(tmp_base != base) {
            base = tmp_base;
            first_no = -1.;
            prev_no = -1.;
            pr_liab_limit = -1;
        }
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
            result << fixed << setprecision(0) << setw(3) << setfill('0') << iv->no;
            if(iv->pr_liab_limit)
                result << lim;
            first_no = iv->no;
        }
        prev_no = iv->no;
        pr_liab_limit = iv->pr_liab_limit;
    }
    if(prev_no != first_no) {
        double mod = prev_no - (floor(prev_no / 1000) * 1000);
        result << "-" << fixed << setprecision(0) << setw(3) << setfill('0') << mod;
        if(pr_liab_limit)
            result << lim;
    }
    return result.str();
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
            << Qry.FieldAsString("trfer_suffix")
            << ")";
        result = buf.str();
    }
    return result;
}

void RunBMNew(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    TQuery Qry(&OraSession);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);
    bool pr_trfer = (string)NodeAsString("name", reqNode) == "BMTrfer";

    //TODO: get_report_form in each report handler, not once!
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
    Qry.SQLText = "select airp from points where point_id = :point_id ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("RunBMNew: point_id %d not found", point_id);
    string airp = Qry.FieldAsString(0);
    bag_names.init(airp);
    vector<TBagTagRow> bag_tags;
    vector<int> grps;
    Qry.Clear();
    string SQLText =
        "select ";
    if(pr_trfer)
        SQLText +=
            "    nvl2(v_last_trfer.last_trfer, 1, 0) pr_trfer, "
            "    v_last_trfer.airline trfer_airline, "
            "    v_last_trfer.flt_no trfer_flt_no, "
            "    v_last_trfer.suffix trfer_suffix, "
            "    v_last_trfer.airp_arv trfer_airp_arv, ";
    else
        SQLText +=
            "    0 pr_trfer, "
            "    null trfer_airline, "
            "    null trfer_flt_no, "
            "    null trfer_suffix, "
            "    null trfer_airp_arv, ";
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
        "    bag2 ";
    if(pr_vip != 2)
        SQLText += ", halls2 ";
    if(pr_trfer)
        SQLText += ", v_last_trfer ";
    SQLText +=
        "where "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.point_arv = points.point_id and "
        "    pax_grp.class = classes.code(+) and "
        "    pax_grp.grp_id = bag2.grp_id and "
        "    bag2.pr_cabin = 0 ";
    if(!target.empty()) {
        SQLText += " and pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, target);
    }
    if(pr_vip != 2) {
        SQLText +=
            " and pax_grp.hall = halls2.id and "
            " halls2.pr_vip = :pr_vip ";
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    }
    if(pr_trfer)
        SQLText +=
            " and pax_grp.grp_id = v_last_trfer.grp_id(+) ";
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

        if(find(grps.begin(), grps.end(), cur_grp_id) == grps.end()) {
            grps.push_back(cur_grp_id);
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
            }
        }

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
        for(; !tagsQry.Eof; tagsQry.Next()) {
            bag_tags.push_back(bag_tag_row);
            bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
            bag_tags.back().color = tagsQry.FieldAsString("color");
            bag_tags.back().no = tagsQry.FieldAsFloat("no");
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
        "   scd_out "
        "from "
        "   points "
        "where "
        "   point_id = :point_id ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(point_id));

    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    tst();
    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    tst();
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
    //    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlineRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "flt",
            airlineRow.AsString("code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", craft);
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
        "FROM pax_grp,bag2 ";
    if(pr_vip != 2)
        SQLText +=
            "   ,halls2 ";
    SQLText +=
        "WHERE pax_grp.grp_id=bag2.grp_id AND ";
    if(pr_vip != 2) {
        SQLText +=
            "      pax_grp.hall=halls2.id AND "
            "      halls2.pr_vip=:pr_vip AND ";
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    }
    SQLText +=
        "      pax_grp.point_dep=:point_id and ";
    if(target.size())
        SQLText +=
            "      pax_grp.airp_arv=:target and ";
    SQLText +=
        "      pax_grp.bag_refuse=0 AND "
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
    else if(name == "PMTrfer" || name == "PM" || name == "PMDMD") RunPM(name, reqNode, formDataNode);
    else if(name == "notpres") RunNotpres(reqNode, formDataNode);
    else if(name == "ref") RunRef(reqNode, formDataNode);
    else if(name == "rem") RunRem(reqNode, formDataNode);
    else if(name == "crs" || name == "crsUnreg") RunCRS(name, reqNode, formDataNode);
    else if(name == "EventsLog") RunEventsLog(reqNode, formDataNode);
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
    tst();
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "test_server", get_test_server());
    tst();
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
    "  ckin.get_pr_tranz_reg(point_id,pr_tranzit) AS pr_tranz_reg, "
    "  park_out "
    "FROM  points "
    "WHERE point_id= :point_id AND pr_del=0";
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

void DocsInterface::GetSegList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int get_tranzit = NodeAsInteger("get_tranzit", reqNode);
    string rpType = NodeAsString("rpType", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp,point_num, "
        "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
        "FROM points WHERE point_id=:point_id ";
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
#ifdef SALEK
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
#endif
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + " (транзит)");
                }
                if(curr_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "N");
                    NewTextChild(SegNode, "airp_dep_code", curr_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", pr_vip);
#ifdef SALEK
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
#endif
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp);
                }
#ifdef SALEK
                if(!(rpType == "BM" || rpType == "TBM" || rpType == "PM" || rpType == "TPM"))
#endif
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
