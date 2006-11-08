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

class TCities: public TBaseTable {
    char *get_cache_name() { return "cities"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat "
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
            "   classes";
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
    static const char* teen[] = {
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
    static const char* desatki[] = {
        "������� ",
        "�ਤ��� ",
        "�ப ",
        "���줥��� ",
        "���줥��� ",
        "ᥬ줥��� ",
        "��ᥬ줥��� ",
        "���ﭮ�� "
    };
    static const char* stuki_m[] = {
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
    static const char* stuki_g[] = {
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
    static const char* dtext[2][3] = {
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

void RunPMTrfer(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
    string status = NodeAsString("status", reqNode);

    TState State = PMTrfer;
    string SQLText =
#ifdef SALEK
    "SELECT "
    "  reg_no,DECODE(:pr_lat,0,full_name,full_name_lat),NULL, "
    "  DECODE(pers_type,'��','X',NULL), "
    "  DECODE(pers_type,'��','X',NULL), "
    "  DECODE(:pr_lat,0,class,class_lat), "
    "  seat_no,document, "
    "  DECODE(bag_amount,0,NULL,bag_amount), "
    "  DECODE(bag_weight,0,NULL,bag_weight), "
    "  DECODE(rk_weight,0,NULL,rk_weight), "
    "  DECODE(excess,0,NULL,excess), "
    "  DECODE(:pr_lat,0,tags,tags_lat), ";
    if(State == PMTrfer)
        SQLText += "  DECODE(:pr_lat,0,last_target,last_target_lat) AS last_target, ";
    SQLText += "  grp_id ";
#endif
#ifdef CHITA
    "SELECT "
    "  reg_no,DECODE(:pr_lat,0,full_name,full_name_lat),NULL, "
    "  DECODE(pers_type,''��'',''X'',NULL), "
    "  DECODE(pers_type,''��'',''X'',NULL), "
    "  DECODE(pers_type,''��'',''X'',NULL), "
    "  DECODE(:pr_lat,0,class,class_lat), "
    "  seat_no,document, "
    "  DECODE(rk_weight,0,NULL,rk_weight), "
    "  DECODE(bag_amount,0,NULL,bag_amount), "
    "  DECODE(bag_weight,0,NULL,bag_weight), "
    "  DECODE(excess,0,NULL,excess), "
    "  DECODE(:pr_lat,0,tags,tags_lat), ";
    if State=PMTrfer then
        SQLText += "  DECODE(:pr_lat,0,last_target,last_target_lat) AS last_target, ";
    SQLText += "  grp_id ";
#endif
#ifdef CHEL
    "SELECT "
    "  reg_no,DECODE(:pr_lat,0,full_name,full_name_lat),NULL, "
    "  DECODE(:pr_lat,0,class,class_lat), "
    "  DECODE(pers_type,''��'',''CHD '',''��'',''INF '',NULL)||remarks, "
    "  document,seat_no, "
    "  DECODE(rk_weight,0,NULL,rk_weight), "
    "  DECODE(bag_amount,0,NULL,bag_amount), "
    "  DECODE(bag_weight,0,NULL,bag_weight), "
    "  DECODE(rk_weight+bag_weight,0,NULL,rk_weight+bag_weight), "
    "  DECODE(excess,0,NULL,excess), "
    "  DECODE(:pr_lat,0,tags,tags_lat), ";
    if State=PMTrfer then
        SQLText += "  DECODE(:pr_lat,0,last_target,last_target_lat) AS last_target, ";
    SQLText += "  grp_id ";
#endif
    if(State == PM) {
        SQLText += 
            "FROM v_pm "
            "WHERE trip_id=:trip_id AND status=:status AND target=:target "
            "ORDER BY lvl,reg_no ";
    } else {
        SQLText += 
            "FROM v_pm_trfer "
            "WHERE trip_id=:trip_id AND status=:status AND target=:target "
            "ORDER BY pr_trfer,last_target,lvl,reg_no ";
    }
    TQuery Qry(&OraSession);        
    Qry.SQLText = SQLText;
    Qry.DeclareVariable("trip_id",otInteger);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("pr_lat",otInteger);
    Qry.DeclareVariable("status",otString);
    Qry.SetVariable("trip_id", point_id);
    Qry.SetVariable("target", target);
    Qry.SetVariable("pr_lat", pr_lat);
    Qry.SetVariable("status", status);

    Qry.Execute();

    Qry.Clear();
    Qry.SQLText =
#ifdef SALEK
    "SELECT NVL(SUM(seats),0), "
    "       NVL(SUM(adl),0),NVL(SUM(chd),0), "
    "       NVL(SUM(inf),0), "
    "       NVL(SUM(bag_amount),0),NVL(SUM(bag_weight),0), "
    "       NVL(SUM(rk_weight),0),NVL(SUM(excess),0) "
#endif
#ifdef CHITA
    "SELECT NVL(SUM(seats),0), "
    "       NVL(SUM(adl),0),NVL(SUM(chd),0), "
    "       NVL(SUM(inf),0), "
    "       NVL(SUM(bag_amount),0),NVL(SUM(bag_weight),0), "
    "       NVL(SUM(rk_weight),0),NVL(SUM(excess),0) "
#endif
#ifdef CHEL
    "SELECT NVL(SUM(adl),0),NVL(SUM(chd),0), "
    "       NVL(SUM(inf),0),NVL(SUM(rk_weight),0), "
    "       NVL(SUM(bag_amount),0),NVL(SUM(bag_weight),0), "
    "       NVL(SUM(excess),0) "
#endif
    "FROM v_pm_trfer_total "
    "WHERE point_id=:trip_id AND target=:target AND status=:status ";

    Qry.DeclareVariable("trip_id",otInteger);
    Qry.DeclareVariable("target",otString);
    Qry.DeclareVariable("status",otString);
    Qry.SetVariable("trip_id", point_id);
    Qry.SetVariable("target", target);
    Qry.SetVariable("status", status);

    Qry.Execute();






    /*
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
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
    // ������ ��६���� ����
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

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airps.get(airp, "name", false));
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
    */
}

void RunBM(xmlNodePtr reqNode, xmlNodePtr formDataNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string target = NodeAsString("target", reqNode);
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
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
    // ������ ��६���� ����
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

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airps.get(airp, "name", false));
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
    int pr_lat = NodeAsInteger("pr_lat", reqNode);
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
    // ������ ��६���� ����
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

    NewTextChild(variablesNode, "own_airp_name", "�������� " + airps.get(airp, "name", false));
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
    // ������� � �⢥� 蠡��� ����
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

    // ⥯��� ������� ����� ��� ����
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    if(name == "test1") RunTest1(formDataNode);
    // ���� test2 ��뢠��� 2-� ����⮢ �� �����让 �����.
    else if(name == "test2") RunTest2(formDataNode);
    // group test
    else if(name == "test3") RunTest3(formDataNode);
    else if(name == "BMTrfer") RunBMTrfer(reqNode, formDataNode);
    else if(name == "BM") RunBM(reqNode, formDataNode);
    else if(name == "PMTrfer") RunPMTrfer(reqNode, formDataNode);
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

    if(name == "BMTrfer" || name == "BM") throw UserException("������ " + name + " ����饭�");

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

void DocsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
