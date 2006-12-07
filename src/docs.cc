#include "docs.h"
#include "oralib.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "str_utils.h"
#include "astra_utils.h"

#define SALEK

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

class TBaseTable {
    private:
        typedef map<string, string> TFields;
        typedef map<string, TFields> TTable;
        TTable table;
    public:
        virtual char *get_cache_name() = 0;
        virtual char *get_sql_text() = 0;
        virtual ~TBaseTable() {};
        string get(string code, string name, bool pr_lat, bool pr_except = false);
};

string TBaseTable::get(string code, string name, bool pr_lat, bool pr_except)
{
    if(table.empty()) {
        TQuery Qry(&OraSession);
        Qry.SQLText = get_sql_text();
        Qry.Execute();
        if(Qry.Eof) throw Exception("TBaseTable::get: table is empty");
        while(!Qry.Eof) {
            TFields fields;
            for(int i = 0; i < Qry.FieldsCount(); i++)
                fields[Qry.FieldName(i)] = Qry.FieldAsString(i);
            table[Qry.FieldAsString(0)] = fields;
            Qry.Next();
        }
    }
    name = upperc(name);
    TTable::iterator ti = table.find(code);
    if(ti == table.end())
        throw Exception((string)"TBaseTable::get data not found in " + get_cache_name() + " for code " + code);
    TFields::iterator fi = ti->second.find(name);
    if(fi == ti->second.end())
        throw Exception("TBaseTable::get: field " + name + " not found for " + get_cache_name());
    if(pr_lat) {
        TFields::iterator fi_lat = ti->second.find(name + "_LAT");
        if(fi_lat != ti->second.end())
            fi = fi_lat;
        else if(pr_except)
            throw Exception("TBaseTable::get: field " + name + "_LAT not found for " + get_cache_name());
    }
    return fi->second;
}

class TAirps: public TBaseTable {
    char *get_cache_name() { return "airps"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   city "
            "from "
            "   airps";
    };
};

class TPersTypes: public TBaseTable {
    char *get_cache_name() { return "pers_types"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat "
            "from "
            "   pers_types";
    };
};

class TCities: public TBaseTable {
    char *get_cache_name() { return "cities"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   country "
            "from "
            "   cities";
    };
};

class TAirlines: public TBaseTable {
    char *get_cache_name() { return "airlines"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   short_name, "
            "   short_name_lat "
            "from "
            "   airlines";
    };
};

class TClasses: public TBaseTable {
    char *get_cache_name() { return "crafts"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat "
            "from "
            "   classes "
            "union "
            "select "
            "   'М', "
            "   'M', "
            "   'КОМФОРТ', "
            "   'COMFORT' "
            "from "
            "   dual";
    };
};

class TCrafts: public TBaseTable {
    char *get_cache_name() { return "crafts"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat "
            "from "
            "   crafts";
    };
};

string vsHow(int nmb, int range)
{
    static const char* sotni[] = {
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
    static const char* teen[] = {
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
    static const char* desatki[] = {
        "двадцать ",
        "тридцать ",
        "сорок ",
        "пятьдесят ",
        "шестьдесят ",
        "семьдесят ",
        "восемьдесят ",
        "девяносто "
    };
    static const char* stuki_m[] = {
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
    static const char* stuki_g[] = {
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
    static const char* dtext[2][3] = {
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

string vs_number(int number)
{
    string result;
    int i = number / 1000;
    result += vsHow(i, 1);
    i = number % 1000;
    result += vsHow(i, 0);
    return result;
}

enum TState {PMTrfer, PM};

void PaxListVars(int point_id, int pr_lat, xmlNodePtr variablesNode)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "select "
        "   airp, "
        "   system.AirpTZRegion(airp) AS tz_region, "
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
    if(Qry.Eof) throw Exception("PaxListVars: variables fetch failed for point_id " + IntToString(point_id));

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = Qry.FieldAsString("tz_region");

    TAirlines airlines;

    NewTextChild(variablesNode, "trip",
            airlines.get(airline, "code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    TDateTime scd_out;
    scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
    NewTextChild(variablesNode, "scd_out", DateTimeToStr(scd_out, "dd.mm.yyyy", pr_lat));
    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
}

int GetRPEncoding(string target)
{
    TAirps airps;
    TCities cities;
    return cities.get(airps.get(target, "city", 0), "country", 0) != "РФ";
}

void RunCRS(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    TQuery Qry(&OraSession);        
    Qry.SQLText = 
        "select  "
        "    point_id, "
        "    pnr_ref, "
        "    decode(:pr_lat, 0, family, family_lat) family, "
        "    decode(:pr_lat, 0, pers_type, pers_type_lat) pers_type, "
        "    decode(:pr_lat, 0, class, class_lat) class, "
        "    seat_no, "
        "    decode(:pr_lat, 0, target, target_lat) target, "
        "    last_target, "
        "    ticket_no, "
        "    document, "
        "    remarks "
        "from "
        "    v_crs "
        "where "
        "    point_id = :point_id "
        "order by "
        "    family ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_crs");
    TPersTypes pers_types;

    TAirps airps;

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
        if(last_target.size()) last_target = airps.get(last_target, "code", pr_lat);
        NewTextChild(rowNode, "last_target", last_target);

        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
        NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
        NewTextChild(rowNode, "remarks", Qry.FieldAsString("remarks"));

        Qry.Next();
    }

    // Теперь переменные отчета
    PaxListVars(point_id, pr_lat, NewTextChild(formDataNode, "variables"));
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
    TPersTypes pers_types;
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
    TPersTypes pers_types;
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
    TPersTypes pers_types;
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", Qry.FieldAsString("family"));
        NewTextChild(rowNode, "pers_type", pers_types.get(Qry.FieldAsString("pers_type"), "code", pr_lat));
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
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(target);
    string status = NodeAsString("status", reqNode);

    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT  "
        "    TRIP_ID, "
        "    TARGET, "
        "    PR_TRFER, "
        "    decode(:pr_lat, 0, last_target, last_target_lat) last_target, "
        "    decode(points.airline, 'ЮТ', decode((select distinct rem_code from pax_rem where pax_id = v_pm_trfer.pax_id and rem_code = 'MCLS'), null, decode(v_pm_trfer.subclass, 'М', 'М', class), 'М'), class) class, "
//        "    CLASS, "
        "    STATUS, "
        "    decode(:pr_lat, 0, full_name, full_name_lat) full_name, "
        "    PERS_TYPE, "
        "    SEAT_NO, "
        "    SEATS, "
        "    DOCUMENT, "
        "    RK_WEIGHT, "
        "    BAG_AMOUNT, "
        "    BAG_WEIGHT, "
        "    EXCESS, "
        "    decode(:pr_lat, 0, tags, tags_lat) tags, "
        "    REG_NO, "
        "    GRP_ID "
        "FROM "
        "    V_PM_TRFER, "
        "    points "
        "WHERE "
        "    TRIP_ID = :point_id AND "
        "    TARGET = :target AND "
        "    STATUS = :status and "
        "    v_pm_trfer.trip_id = points.point_id "
        "ORDER BY ";
    if(name == "PMTrfer")
        SQLText +=
            "    PR_TRFER ASC, "
            "    LAST_TARGET ASC, ";
    SQLText +=
        "    CLASS ASC, "
        "    grp_id, "
        "    REG_NO ASC ";

    Qry.SQLText = SQLText;

    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("status", otString, status);
    Qry.CreateVariable("pr_lat", otString, pr_lat);

    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer");
    TClasses classes;
    while(!Qry.Eof) {
        string cls = Qry.FieldAsString("class");
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", Qry.FieldAsString("reg_no"));
        NewTextChild(rowNode, "full_name", Qry.FieldAsString("full_name"));
        NewTextChild(rowNode, "last_target", Qry.FieldAsString("last_target"));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "class_name", classes.get(cls, "name", pr_lat));
        NewTextChild(rowNode, "class", classes.get(cls, "code", pr_lat));
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
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
        NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("pr_trfer"));
        Qry.Next();
    }


    if(name == "PMTrfer") {
        Qry.Clear();
        Qry.SQLText =
            "SELECT  "
            "    POINT_ID, "
            "    TARGET, "
            "    PR_TRFER, "
            "    STATUS, "
        "    decode(points.airline, 'ЮТ', decode((select distinct rem_code from pax_rem where pax_id = v_pm_trfer.pax_id and rem_code = 'MCLS'), null, decode(v_pm_trfer.subclass, 'М', 'М', class), 'М'), class) class, "
//            "    CLASS, "
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
            "    POINT_ID = :point_id AND "
            "    TARGET = :target AND "
            "    STATUS = :status "
            "ORDER BY "
            "    PR_TRFER, "
            "    LVL ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.CreateVariable("target", otString, target);
        Qry.CreateVariable("status", otString, status);
        Qry.Execute();
        dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer_total");
        while(!Qry.Eof) {
            string cls = Qry.FieldAsString("class");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("POINT_ID"));
            NewTextChild(rowNode, "target", Qry.FieldAsString("TARGET"));
            NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("PR_TRFER"));
            NewTextChild(rowNode, "status", Qry.FieldAsString("STATUS"));
            NewTextChild(rowNode, "class_name", classes.get(cls, "name", pr_lat));
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
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   system.AirpTZRegion(airp) AS tz_region, "
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
    string tz_region = Qry.FieldAsString("tz_region");

    TAirps airps;
    TAirlines airlines;
    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airps.get(airp, "name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airps.get(airp, "name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airps.get(airp, "name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlines.get(airline, "name", pr_lat));
    NewTextChild(variablesNode, "flt",
            airlines.get(airline, "code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", crafts.get(craft, "name", pr_lat));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    NewTextChild(variablesNode, "airp_arv_name", airps.get(target, "name", pr_lat));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
}

void RunBM(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "  lvl, "
        "  birk_range, "
        "  DECODE(:pr_lat,0,color_name,color_name_lat) AS color, "
        "  num,NULL, "
        "  pr_vip, "
        "  class, "
        "  amount,weight "
        "FROM v_bm "
#ifdef SALEK
        "WHERE trip_id=:point_id AND target=:target AND pr_vip=:pr_vip "
        "ORDER BY pr_vip,lvl,tag_type,color_name,birk_range ";
    Qry.CreateVariable("pr_vip", otInteger, pr_vip);
#else
        "WHERE trip_id=:point_id AND target=:target "
        "ORDER BY lvl,tag_type,color_name,birk_range ";
#endif
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("pr_lat", otInteger, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_bm");
    TClasses classes;
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
        NewTextChild(rowNode, "birk_range", Qry.FieldAsString("birk_range"));
        NewTextChild(rowNode, "color", Qry.FieldAsString("color"));
        NewTextChild(rowNode, "num", Qry.FieldAsInteger("num"));
        NewTextChild(rowNode, "pr_vip", Qry.FieldAsInteger("pr_vip"));
        NewTextChild(rowNode, "class", classes.get(cls, "code", pr_lat));
        NewTextChild(rowNode, "class_name", classes.get(cls, "name", pr_lat));
        NewTextChild(rowNode, "amount", Qry.FieldAsInteger("amount"));
        NewTextChild(rowNode, "weight", weight);
        Qry.Next();
    }
    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   system.AirpTZRegion(airp) AS tz_region, "
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
    if(Qry.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(point_id));

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = Qry.FieldAsString("tz_region");

    TAirps airps;
    TAirlines airlines;
    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airps.get(airp, "name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airps.get(airp, "name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airps.get(airp, "name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlines.get(airline, "name", pr_lat));
    NewTextChild(variablesNode, "flt",
            airlines.get(airline, "code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", crafts.get(craft, "name", pr_lat));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    NewTextChild(variablesNode, "airp_arv_name", airps.get(target, "name", pr_lat));

    Qry.Clear();
    Qry.SQLText =
        "SELECT amount,weight,tags FROM unaccomp_bag WHERE point_dep=:point_id AND airp_arv=:target ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.Execute();

    int TotAmount = 0;
    int TotWeight = 0;
    string Tags;

    while(!Qry.Eof) {
        TotAmount += Qry.FieldAsInteger("amount");
        TotWeight += Qry.FieldAsInteger("weight");
        if(Tags.size()) Tags += ", ";
        Tags += Qry.FieldAsString("tags");
        Qry.Next();
    }

    if(!(Tags.empty() && TotAmount == 0 && TotWeight == 0)) {
        NewTextChild(variablesNode, "DosKwit", Tags);
        NewTextChild(variablesNode, "DosPcs", TotAmount);
        NewTextChild(variablesNode, "DosWeight", TotWeight);
    } else {
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    Qry.Clear();
    Qry.SQLText =
        "SELECT NVL(SUM(amount),0) AS amount, "
        "       NVL(SUM(weight),0) AS weight "
        "FROM pax_grp,bag2,halls2 "
        "WHERE pax_grp.grp_id=bag2.grp_id AND "
        "      pax_grp.hall=halls2.id AND "
        "      halls2.pr_vip=:pr_vip AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax_grp.airp_arv=:target AND "
        "      pax_grp.pr_refuse=0 AND "
        "      bag2.pr_cabin=0 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("pr_vip", otInteger, pr_vip);
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
    int pr_lat = GetRPEncoding(target);
    int pr_vip = NodeAsInteger("pr_vip", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "   pr_vip, "
        "   pr_trfer, "
        "   DECODE(:pr_lat,0,last_target,last_target_lat) AS last_target, "
        "   class, "
        "   lvl, "
        "   tag_type, "
        "   DECODE(:pr_lat,0,color_name,color_name_lat) AS color, "
        "   birk_range, "
        "   num, "
        "   NULL null_val, "
        "   DECODE(0,0,class_name,class_name_lat) AS class_name, "
        "   amount, "
        "   weight "
        "FROM v_bm_trfer "
        "WHERE trip_id=:point_id AND target=:target AND pr_vip=:pr_vip "
        "ORDER BY "
        "   pr_vip, "
        "   pr_trfer, "
        "   last_target, "
        "   class, "
        "   lvl, "
        "   tag_type, "
        "   color_name, "
        "   birk_range ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("pr_vip", otInteger, pr_vip);
    Qry.CreateVariable("pr_lat", otInteger, pr_lat);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_bm_trfer");
    string last_target, lvl;
    TClasses classes;
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
        NewTextChild(rowNode, "pr_vip", Qry.FieldAsString("pr_vip"));
        NewTextChild(rowNode, "pr_trfer", Qry.FieldAsInteger("pr_trfer"));
        NewTextChild(rowNode, "last_target", Qry.FieldAsString("last_target"));
        NewTextChild(rowNode, "class", Qry.FieldAsString("class"));
        NewTextChild(rowNode, "lvl", Qry.FieldAsString("lvl"));
        NewTextChild(rowNode, "tag_type", Qry.FieldAsString("tag_type"));
        NewTextChild(rowNode, "color", Qry.FieldAsString("color"));
        NewTextChild(rowNode, "birk_range", Qry.FieldAsString("birk_range"));
        NewTextChild(rowNode, "num", Qry.FieldAsInteger("num"));
        NewTextChild(rowNode, "null_val", Qry.FieldAsString("null_val"));
        NewTextChild(rowNode, "class_name", classes.get(cls, "name", pr_lat));
        NewTextChild(rowNode, "amount", Qry.FieldAsInteger("amount"));
        NewTextChild(rowNode, "weight", weight);
        Qry.Next();
    }
    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   system.AirpTZRegion(airp) AS tz_region, "
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
    if(Qry.Eof) throw Exception("RunBMTrfer: variables fetch failed for point_id " + IntToString(point_id));

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = Qry.FieldAsString("tz_region");

    TAirps airps;
    TAirlines airlines;
    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airps.get(airp, "name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airps.get(airp, "name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airps.get(airp, "name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlines.get(airline, "name", pr_lat));
    NewTextChild(variablesNode, "flt",
            airlines.get(airline, "code", pr_lat) +
            IntToString(Qry.FieldAsInteger("flt_no")) +
            Qry.FieldAsString("suffix")
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", crafts.get(craft, "name", pr_lat));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh.nn", pr_lat));
    NewTextChild(variablesNode, "airp_arv_name", airps.get(target, "name", pr_lat));

    Qry.Clear();
    Qry.SQLText =
        "SELECT amount,weight,tags FROM unaccomp_bag WHERE point_dep=:point_id AND airp_arv=:target ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.Execute();

    int TotAmount = 0;
    int TotWeight = 0;
    string Tags;

    while(!Qry.Eof) {
        TotAmount += Qry.FieldAsInteger("amount");
        TotWeight += Qry.FieldAsInteger("weight");
        if(Tags.size()) Tags += ", ";
        Tags += Qry.FieldAsString("tags");
        Qry.Next();
    }

    if(!(Tags.empty() && TotAmount == 0 && TotWeight == 0)) {
        NewTextChild(variablesNode, "DosKwit", Tags);
        NewTextChild(variablesNode, "DosPcs", TotAmount);
        NewTextChild(variablesNode, "DosWeight", TotWeight);
    } else {
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    Qry.Clear();
    Qry.SQLText =
        "SELECT NVL(SUM(amount),0) AS amount, "
        "       NVL(SUM(weight),0) AS weight "
        "FROM pax_grp,bag2,halls2 "
        "WHERE pax_grp.grp_id=bag2.grp_id AND "
        "      pax_grp.hall=halls2.id AND "
        "      halls2.pr_vip=:pr_vip AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax_grp.airp_arv=:target AND "
        "      pax_grp.pr_refuse=0 AND "
        "      bag2.pr_cabin=0 ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("target", otString, target);
    Qry.CreateVariable("pr_vip", otInteger, pr_vip);
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

void  DocsInterface::RunReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText = "select form from fr_forms where name = :name";
    Qry.CreateVariable("name", otString, name);
    Qry.Execute();
    if(Qry.Eof) throw UserException("form " + name + " not found");
    // положим в ответ шаблон отчета
    int len = Qry.GetSizeLongField("form");
    void *data = malloc(len);
    if ( data == NULL )
        throw Exception("DocsInterface::RunReport malloc failed");
    try {
        Qry.FieldAsLong("form", data);
        string form((char *)data, len);
        NewTextChild(resNode, "form", form);
    } catch(...) {
        free(data);
    }
    free(data);

    // теперь положим данные для отчета
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    if(name == "test1") RunTest1(formDataNode);
    // отчет test2 связывание 2-х датасетов это бааалшой вопрос.
    else if(name == "test2") RunTest2(formDataNode);
    // group test
    else if(name == "test3") RunTest3(formDataNode);
    else if(name == "BMTrfer") RunBMTrfer(reqNode, formDataNode);
    else if(name == "BM") RunBM(reqNode, formDataNode);
    else if(name == "PMTrfer" || name == "PM") RunPM(name, reqNode, formDataNode);
    else if(name == "notpres") RunNotpres(reqNode, formDataNode);
    else if(name == "ref") RunRef(reqNode, formDataNode);
    else if(name == "rem") RunRem(reqNode, formDataNode);
    else if(name == "crs") RunCRS(reqNode, formDataNode);
    else
        throw UserException("data handler not found for " + name);
    ProgTrace(TRACE5, "%s", GetXMLDocText(formDataNode->doc).c_str());
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
            name == "rem"
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
    "  park_out, "
    "  system.AirpTZRegion(points.airp) AS tz_region "
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
  char *tz_region=Qry.FieldAsString("tz_region");
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

            for(int pr_vip = 0; pr_vip <= 1; pr_vip++) {
                if(prev_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "T");
                    NewTextChild(SegNode, "airp_dep_code", prev_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", pr_vip);
#ifdef SALEK
                    if(rpType == "BM" || rpType == "TBM")
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + (pr_vip ? " (VIP)" : " (не VIP)"));
                    else
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
                    if(rpType == "BM" || rpType == "TBM")
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp + (pr_vip ? " (VIP)" : " (не VIP)"));
                    else
#endif
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp);
                }
#ifdef SALEK
                if(!(rpType == "BM" || rpType == "TBM"))
#endif
                    break;
            }
            Qry.Next();
        }
    }
}
