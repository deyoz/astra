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
        "�� ",
        "����� ",
        "���� ",
        "������ ",
        "������ ",
        "������ ",
        "ᥬ��� ",
        "��ᥬ��� ",
        "�������� "
    };
    static char* teen[] = {
        "������ ",
        "���������� ",
        "��������� ",
        "�ਭ����� ",
        "���ୠ���� ",
        "��⭠���� ",
        "��⭠���� ",
        "ᥬ������ ",
        "��ᥬ������ ",
        "����⭠���� "
    };
    static char* desatki[] = {
        "������� ",
        "�ਤ��� ",
        "�ப ",
        "���줥��� ",
        "���줥��� ",
        "ᥬ줥��� ",
        "��ᥬ줥��� ",
        "���ﭮ�� "
    };
    static char* stuki_g[] = {
        "",
        "���� ",
        "��� ",
        "�� ",
        "���� ",
        "���� ",
        "���� ",
        "ᥬ� ",
        "��ᥬ� ",
        "������ "
    };
    static char* stuki_m[] = {
        "",
        "���� ",
        "��� ",
        "�� ",
        "���� ",
        "���� ",
        "���� ",
        "ᥬ� ",
        "��ᥬ� ",
        "������ "
    };
    static char* dtext[2][3] = {
        {"", "", ""},
        {"����� ", "����� ", "����� "}
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

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airpRow.AsString("name", false));
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
             airps.get_row("code",target).AsString("city")).AsString("country") != "��";
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

    // ������ ��६���� ����
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
}

void RunSZV(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    NewTextChild(dataSetsNode, "events_log");
    // ������ ��६���� ����
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

    // ������ ��६���� ����
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
    // ������ ��६���� ����
    NewTextChild(variablesNode, "paxlist_type", "��ᬮ�� / ��ᠤ��");
    PaxListVars(point_id, pr_lat, variablesNode);
    currNode = variablesNode->children;
    xmlNodePtr totalNode = NodeAsNodeFast("total", currNode);
    NodeSetContent(totalNode, (string)"�⮣�: " + NodeAsString(totalNode));
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

    // ������ ��६���� ����
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

    // ������ ��६���� ����
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

    // ������ ��६���� ����
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
            et = "(��)";
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
            et = "(��)";
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
      ) { //��
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
      ) { //��
        SQLText +=
            "   pr_brd = 1 and "
            "   ((ticket_no is not null and "
            "   coupon_no is not null) or "
            "   report.get_tkno(pax_id, '/', 1) is not null) and ";
    } else if(
            target == "tot" ||
            target == "tpm"
            ) { //�����
    } else { // ᥣ����
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
    // ᫥���騥 2 ��६���� ������� ��� �㦤 FastReport
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
        if(pers_type == "��")
            NewTextChild(rowNode, "pers_type", "ADL");
        else if(pers_type == "��")
            NewTextChild(rowNode, "pers_type", "CHD");
        else if(pers_type == "��")
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
            "FROM "
            "    V_PM_TRFER_TOTAL "
            "WHERE "
            "    POINT_ID = :point_id AND ";
        if(
                target.size() &&
                target != "tot" &&
                target != "etm" &&
                target != "tpm"
          )
            SQLText +=
                "    TARGET = :target AND ";
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
        Qry.CreateVariable("pr_lat", otInteger, pr_lat);
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
        if(target.empty()) { //��
            SQLText +=
                "    V_PM_EL_TOTAL ";
        } else if(
                target == "tot" ||
                target == "tpm"
                ) { //�����
            SQLText +=
                "    V_PM_TOTAL ";
        } else { // ᥣ����
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

    // ������ ��६���� ����
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

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airpRow.AsString("name", false));
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

void RunBM(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);

    //TODO: get_report_form in each report handler, not once!
    if(target.empty()) {
        xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
        // ����� get_report_form ��뢠���� ReplaceTextChild, � ���஬ � ᢮�
        // ��।� ��뢠���� xmlNodeSetContent, �����, ��� �� �ᥬ�, ������ string
        // ���� 祬 xmlNewTextChild, �� ��筮 ���� �� ���ࠨ����. ���⮬� 㤠��� 㧥�
        // form, �⮡� get_report_form ᮧ��� ��� ������. (c) Den. 26.11.07
        xmlNodePtr formNode = NodeAsNode("form", resNode);
        xmlUnlinkNode(formNode);
        xmlFreeNode(formNode);
        get_report_form("BMTotal", resNode);
    }

    TQuery Qry(&OraSession);

    string SQLText =
        "select  "
        "    trip_id, "
        "    target, ";
    if(pr_vip != 2)
        SQLText +=
            "    pr_vip, ";
    SQLText +=
        "    amount, "
        "    weight, "
        "    class, "
        "    DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
        "    lvl, "
        "    tag_type, "
        "    DECODE(:pr_lat,0,color_name,color_name_lat) AS color, "
        "    birk_range, "
        "    num "
        "from ";
    if(pr_vip == 2)
        SQLText +=
            "    v_bm_total  ";
    else
        SQLText +=
            "    v_bm  ";
    SQLText +=
        "where "
        "    trip_id = :point_id ";
    if(target.size())
        SQLText +=
            "    and target = :target ";
    if(pr_vip != 2)
        SQLText +=
            "    and pr_vip = :pr_vip ";
    SQLText +=
        "order by ";
    if(target.empty())
        SQLText +=
            "    target, ";
    if(pr_vip != 2)
        SQLText +=
            "    pr_vip, ";
    SQLText +=
        "    lvl, "
        "    tag_type, "
        "    color_name ";

    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    if(target.size())
        Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("pr_lat", otInteger, pr_lat);
    if(pr_vip != 2)
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_bm");
    string lvl;
    while(!Qry.Eof) {
        string cls = Qry.FieldAsString("class");

        string tmp_lvl = Qry.FieldAsString("lvl");
        int weight = 0;
        if(tmp_lvl != lvl) {
            lvl = tmp_lvl;
            weight = Qry.FieldAsInteger("weight");
        }

        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        string airp_arv = Qry.FieldAsString("target");
        TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp_arv);
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "airp_arv_name", airpRow.AsString("name", pr_lat));
        NewTextChild(rowNode, "birk_range", Qry.FieldAsString("birk_range"));
        NewTextChild(rowNode, "color", Qry.FieldAsString("color"));
        NewTextChild(rowNode, "num", Qry.FieldAsInteger("num"));
        NewTextChild(rowNode, "pr_vip", pr_vip);
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "class_name", Qry.FieldAsString("class_name"));
        NewTextChild(rowNode, "amount", Qry.FieldAsInteger("amount"));
        NewTextChild(rowNode, "weight", weight);
        Qry.Next();
    }
    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
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

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    tst();
    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    tst();
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
//    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airpRow.AsString("name", false));
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
}

void RunBMTrfer(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);

    if(target.empty()) {
        xmlNodePtr resNode = NodeAsNode("/term/answer", formDataNode->doc);
        xmlNodePtr formNode = NodeAsNode("form", resNode);
        xmlUnlinkNode(formNode);
        xmlFreeNode(formNode);
        get_report_form("BMTrferTotal", resNode);
    }

    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);

    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "    target, ";
    if(pr_vip != 2)
        SQLText +=
            "    pr_vip, ";
    SQLText +=
        "   pr_trfer, "
        "   DECODE(:pr_lat,0,last_target,last_target_lat) AS last_target, "
        "   class, "
        "   DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
        "   lvl, "
        "   tag_type, "
        "   DECODE(:pr_lat,0,color_name,color_name_lat) AS color, "
        "   birk_range, "
        "   num, "
        "   NULL null_val, "
        "   DECODE(:pr_lat,0,class_name,class_name_lat) AS class_name, "
        "   amount, "
        "   weight "
        "FROM ";
    if(target.empty())
        SQLText +=
            "    v_bm_trfer_total  ";
    else if(pr_vip == 2)
        SQLText +=
            "    v_bm_trfer_total  ";
    else
        SQLText +=
            "    v_bm_trfer  ";
    SQLText +=
        "WHERE trip_id=:point_id ";
    if(target.size())
        SQLText +=
            "   and target=:target ";
    if(pr_vip != 2)
        SQLText +=
            "    and pr_vip = :pr_vip ";
    SQLText +=
        "ORDER BY ";
    if(target.empty())
        SQLText +=
            "    target, ";
    if(pr_vip != 2)
        SQLText +=
            "    pr_vip, ";
    SQLText +=
        "   pr_trfer, "
        "   last_target, "
        "   class, "
        "   lvl, "
        "   tag_type, "
        "   color_name, "
        "   birk_range ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    if(target.size())
        Qry.CreateVariable("target", otString, target);
    if(pr_vip != 2)
        Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    Qry.CreateVariable("pr_lat", otInteger, pr_lat);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_bm_trfer");
    string last_target, lvl;
    while(!Qry.Eof) {
        string tmp_last_target = Qry.FieldAsString("last_target");
        string tmp_lvl = Qry.FieldAsString("lvl");
        int weight = 0;
        if(tmp_last_target != last_target || tmp_lvl != lvl) {
            last_target = tmp_last_target;
            lvl = tmp_lvl;
            weight = Qry.FieldAsInteger("weight");
        }
        string cls = Qry.FieldAsString("class");
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        string airp_arv = Qry.FieldAsString("target");
        TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp_arv);
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "airp_arv_name", airpRow.AsString("name", pr_lat));
        NewTextChild(rowNode, "pr_vip", pr_vip);
        NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("pr_trfer"));
        NewTextChild(rowNode, "last_target", Qry.FieldAsString("last_target"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "lvl", Qry.FieldAsString("lvl"));
        NewTextChild(rowNode, "tag_type", Qry.FieldAsString("tag_type"));
        NewTextChild(rowNode, "color", Qry.FieldAsString("color"));
        NewTextChild(rowNode, "birk_range", Qry.FieldAsString("birk_range"));
        NewTextChild(rowNode, "num", Qry.FieldAsInteger("num"));
        NewTextChild(rowNode, "null_val", Qry.FieldAsString("null_val"));
        NewTextChild(rowNode, "class_name", Qry.FieldAsString("class_name"));
        NewTextChild(rowNode, "amount", Qry.FieldAsInteger("amount"));
        NewTextChild(rowNode, "weight", weight);
        Qry.Next();
    }
    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
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
    if(Qry.Eof) throw Exception("RunBMTrfer: variables fetch failed for point_id " + IntToString(point_id));

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    tst();
    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",airp);
    tst();
    TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
//    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airpRow.AsString("name", false));
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
    tst();

    if(target.size())
        NewTextChild(variablesNode, "airp_arv_name", base_tables.get("AIRPS").get_row("code",target).AsString("name",pr_lat));

    { // for back compatibility 14.10.07 !!!
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

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
        "      pax_grp.point_dep=:point_id AND ";
    if(target.size())
        SQLText +=
            "      pax_grp.airp_arv=:target AND ";
    SQLText +=
        "      pax_grp.bag_refuse=0 AND "
        "      bag2.pr_cabin=0 ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    if(target.size())
        Qry.CreateVariable("target", otString, target);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    int TotAmount = 0;
    int TotWeight = 0;
    if(Qry.RowCount() > 0) {
        TotAmount += Qry.FieldAsInteger("amount");
        TotWeight += Qry.FieldAsInteger("weight");
    }

    NewTextChild(variablesNode, "TotPcs", TotAmount);
    NewTextChild(variablesNode, "TotWeight", TotWeight);
    NewTextChild(variablesNode, "Tot", (pr_lat ? "" : vs_number(TotAmount)));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
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
    // ������� � �⢥� 蠡��� ����
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

    // ⥯��� ������� ����� ��� ����
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");

    if(name == "test1") RunTest1(formDataNode);
    // ���� test2 ��뢠��� 2-� ����⮢ �� �����让 �����.
    else if(name == "test2") RunTest2(formDataNode);
    // group test
    else if(name == "test3") RunTest3(formDataNode);
    else if(name == "BMTrfer") RunBMTrfer(reqNode, formDataNode);
    else if(name == "BM") RunBM(reqNode, formDataNode);
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
        throw UserException("������ " + name + " ����饭�");
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
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");
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
    if(Qry.Eof) throw UserException("���� �� ������. ������� �����.");

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
                                hall = " (�� VIP)";
                                break;
                            case 1:
                                hall = " (VIP)";
                                break;
                        }
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + hall);
                    } else
#endif
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + " (�࠭���)");
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
                                hall = " (�� VIP)";
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
            NewTextChild(SegNode, "item", "���᮪ ��");
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
            NewTextChild(SegNode, "item", "����");
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
