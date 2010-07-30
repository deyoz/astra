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
#include "term_version.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/str_utils.h"
#include <boost/shared_array.hpp>

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;
using namespace ASTRA;
using namespace boost;

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

bool ru_desk()
{
    return TReqInfo::Instance()->desk.lang == "RU";
}

std::string translateDocCap(bool pr_lat, const std::string &vlexema)
{
    if(ru_desk())
        return getLocaleText(vlexema, (pr_lat ? "EN" : "RU"));
    else
        return getLocaleText(vlexema);
}

std::string translateDocCap(bool pr_lat, const std::string &vlexema, LParams &aparams)
{
    if(ru_desk())
        return getLocaleText(vlexema, aparams, (pr_lat ? "EN" : "RU"));
    else
        return getLocaleText(vlexema, aparams);
}

void populate_doc_cap(xmlNodePtr variablesNode, bool pr_lat)
{
    NewTextChild(variablesNode, "doc_cap_no", translateDocCap(pr_lat, "№"));
    NewTextChild(variablesNode, "doc_cap_name", translateDocCap(pr_lat, "Ф.И.О."));
    NewTextChild(variablesNode, "doc_cap_surname", translateDocCap(pr_lat, "Фамилия"));
    NewTextChild(variablesNode, "doc_cap_doc", translateDocCap(pr_lat, "Документ"));
    NewTextChild(variablesNode, "doc_cap_tkt", translateDocCap(pr_lat, "Билет"));
    NewTextChild(variablesNode, "doc_cap_tkt_no", translateDocCap(pr_lat, "№ Билета"));
    NewTextChild(variablesNode, "doc_cap_ref_type", translateDocCap(pr_lat, "Причина невылета"));
    NewTextChild(variablesNode, "doc_cap_bag", translateDocCap(pr_lat, "Баг."));
    NewTextChild(variablesNode, "doc_cap_baggage", translateDocCap(pr_lat, "Багаж"));
    NewTextChild(variablesNode, "doc_cap_rk", translateDocCap(pr_lat, "Р/к"));
    NewTextChild(variablesNode, "doc_cap_pay", translateDocCap(pr_lat, "Опл."));
    NewTextChild(variablesNode, "doc_cap_tags", translateDocCap(pr_lat, "№№ баг. бирок"));
    NewTextChild(variablesNode, "doc_cap_tags_short", translateDocCap(pr_lat, "№ б/б"));
    NewTextChild(variablesNode, "doc_cap_rem", translateDocCap(pr_lat, "Рем."));
    NewTextChild(variablesNode, "doc_cap_pas", translateDocCap(pr_lat, "Пас"));
    NewTextChild(variablesNode, "doc_cap_type", translateDocCap(pr_lat, "Тип"));
    NewTextChild(variablesNode, "doc_cap_seat_no", translateDocCap(pr_lat, "№ м"));
    NewTextChild(variablesNode, "doc_cap_remarks", translateDocCap(pr_lat, "Ремарки"));
    NewTextChild(variablesNode, "doc_cap_cl", translateDocCap(pr_lat, "Кл"));
    NewTextChild(variablesNode, "doc_cap_dest", translateDocCap(pr_lat, "П/н"));
    NewTextChild(variablesNode, "doc_cap_to", translateDocCap(pr_lat, "CAP.DOC.TO"));
    NewTextChild(variablesNode, "doc_cap_ex", translateDocCap(pr_lat, "Дс"));
    NewTextChild(variablesNode, "doc_cap_brd", translateDocCap(pr_lat, "Пс"));
    NewTextChild(variablesNode, "doc_cap_test", translateDocCap(pr_lat, "CAP.TEST"));
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
        "   ckin.get_airps(point_id,:vlang,1) long_route "
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
    Qry.CreateVariable( "vlang", otString, TReqInfo::Instance()->desk.lang );
    Qry.Execute();
    if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    string airline_name;
    if(airline.size()) {
      TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
      airline = airlineRow.AsString("code",pr_lat);
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

    NewTextChild(variablesNode, "lang", TReqInfo::Instance()->desk.lang );
    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airp_dep_city", airpRow.AsString("city", pr_lat));
    NewTextChild(variablesNode, "airline_name", airline_name);
    NewTextChild(variablesNode, "flt", trip);
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", craft);
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", pr_lat));
    NewTextChild(variablesNode, "airp_arv_name", airpRow.AsString("name",pr_lat));
    NewTextChild(variablesNode, "airp_arv_city", airpRow.AsString("city",pr_lat));
    NewTextChild(variablesNode, "long_route", Qry.FieldAsString("long_route"));
    NewTextChild(variablesNode, "test_server", get_test_server());
    NewTextChild(variablesNode, "page_number_fmt", translateDocCap(pr_lat, "CAP.PAGE_NUMBER_FMT"));
}

enum TRptType {
    rtPTM,
    rtPTMTXT,
    rtBTM,
    rtBTMTXT,
    rtWEB,
    rtWEBTXT,
    rtREFUSE,
    rtREFUSETXT,
    rtNOTPRES,
    rtNOTPRESTXT,
    rtREM,
    rtREMTXT,
    rtCRS,
    rtCRSTXT,
    rtCRSUNREG,
    rtCRSUNREGTXT,
    rtEXAM,
    rtEXAMTXT,
    rtUnknown,
    rtTypeNum
};

const char *RptTypeS[rtTypeNum] = {
    "PTM",
    "PTMTXT",
    "BTM",
    "BTMTXT",
    "WEB",
    "WEBTXT",
    "REFUSE",
    "REFUSETXT",
    "NOTPRES",
    "NOTPRESTXT",
    "REM",
    "REMTXT",
    "CRS",
    "CRSTXT",
    "CRSUNREG",
    "CRSUNREGTX",
    "EXAM",
    "EXAMTXT",
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

int GetRPEncoding(const TRptParams &rpt_params)
{
    int result = 0;
    if(ru_desk()) {
        TBaseTable &airps = base_tables.get("AIRPS");
        TBaseTable &cities = base_tables.get("CITIES");
        //определяем encoding по всему маршруту! независимо от фильтра по аэропорту
        TTripRoute route;
        if (!route.GetRouteAfter(rpt_params.point_id,trtWithCurrent,trtNotCancelled))
            throw Exception("TTripRoute::GetRouteAfter: flight not found for point_id %d", rpt_params.point_id);
        for(vector<TTripRouteItem>::iterator iv = route.begin(); iv != route.end(); iv++)
        {
            ProgTrace(TRACE5, "%s", cities.get_row("code",
                        airps.get_row("code",iv->airp).AsString("city")).AsString("country").c_str());
            result = result or cities.get_row("code",
                    airps.get_row("code",iv->airp).AsString("city")).AsString("country") != "РФ";
        };
    } else
        result = 1;
    return result;
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
        "SELECT DECODE(:pr_lat,0,name,NVL(name_lat,system.transliter(name,1,1))) name "
        "FROM "
        "   halls2 "
        "WHERE "
        "   airp = :airp AND "
        "   NVL(rpt_grp, ' ') = NVL(:rpt_grp, ' ') "
        "ORDER BY "
        "   name ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("pr_lat", otInteger, (int)pr_lat);
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

string get_report_version(string name)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select version from fr_forms2 where name = :name and version <= :version order by version desc";
    Qry.CreateVariable("name", otString, name);
    Qry.CreateVariable("version", otString, TReqInfo::Instance()->desk.version);
    Qry.Execute();
    string result;
    if(!Qry.Eof)
        result = Qry.FieldAsString("version");
    return result;
}

void get_report_form(const string name, xmlNodePtr node)
{
    string form;
    string version;
    TQuery Qry(&OraSession);
    if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION)) {
        Qry.SQLText = "select form, pr_locale from fr_forms2 where name = :name and version = :version ";
        version = get_report_version(name);
        Qry.CreateVariable("version", otString, version);
    } else
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
    shared_array<char> data (new char[len]);
    Qry.FieldAsLong("form", data.get());
    form.clear();
    form.append(data.get(), len);

    xmlNodePtr formNode = ReplaceTextChild(node, "form", form);
    SetProp(formNode, "name", name);
    SetProp(formNode, "version", version);
    if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION) and Qry.FieldAsInteger("pr_locale") != 0)
        SetProp(formNode, "pr_locale");
}

struct TPMTotalsKey {
    int point_id;
    int pr_trfer;
    string target;
    string status;
    string cls;
    string cls_name;
    int lvl;
    void dump() const;
    TPMTotalsKey():
        point_id(NoExists),
        pr_trfer(NoExists),
        lvl(NoExists)
    {
    };
};

void TPMTotalsKey::dump() const
{
    ProgTrace(TRACE5, "---TPMTotalsKey::dump()---");
    ProgTrace(TRACE5, "point_id: %d", point_id);
    ProgTrace(TRACE5, "pr_trfer: %d", pr_trfer);
    ProgTrace(TRACE5, "target: %s", target.c_str());
    ProgTrace(TRACE5, "status: %s", status.c_str());
    ProgTrace(TRACE5, "cls: %s", cls.c_str());
    ProgTrace(TRACE5, "cls_name: %s", cls_name.c_str());
    ProgTrace(TRACE5, "lvl: %d", lvl);
    ProgTrace(TRACE5, "--------------------------");
}

struct TPMTotalsCmp {
    bool operator() (const TPMTotalsKey &l, const TPMTotalsKey &r) const
    {
        if(l.pr_trfer == NoExists)
            if(l.point_id == r.point_id)
                if(l.lvl == r.lvl)
                    if(l.status == r.status)
                        return l.cls < r.cls;
                    else
                        return l.status < r.status;
                else
                    return l.lvl < r.lvl;
            else
                return l.point_id < r.point_id;
        else
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

void PTM(const TRptParams &rpt_params, xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    int pr_lat = GetRPEncoding(rpt_params);
    string rpt_name;
    if(rpt_params.airp_arv.empty() ||
            rpt_params.rpt_type==rtPTMTXT) {
        if(rpt_params.pr_trfer)
            rpt_name="PMTrferTotalEL";
        else
            rpt_name="PMTotalEL";
    } else {
        if(rpt_params.pr_trfer)
            rpt_name="PMTrfer";
        else
            rpt_name="PM";
    };
    if (rpt_params.rpt_type==rtPTMTXT) rpt_name=rpt_name+"Txt";
    get_report_form(rpt_name, resNode);

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
        "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id, pax.bag_pool_num),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) AS excess, \n"
        "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) AS tags, \n"
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
        "   pax_grp.hall = halls2.id(+) and \n"
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
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and pax_grp.hall IS NOT NULL and ";
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
        NewTextChild(rowNode, "full_name", transliter(Qry.FieldAsString("full_name"), 1, pr_lat));
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
            ElemIdToElem(etAirline, airline, airline_fmt, pr_lat) +
            IntToString(flt_no) +
            ElemIdToElem(etSuffix, suffix, suffix_fmt, pr_lat)
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", ElemIdToElem(etCraft, craft, craft_fmt, pr_lat));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", pr_lat));
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
}

void BTM(const TRptParams &rpt_params, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    int pr_lat = GetRPEncoding(rpt_params);
    string rpt_name;
    if(rpt_params.airp_arv.empty() ||
            rpt_params.rpt_type==rtBTMTXT) {
        if(rpt_params.pr_trfer)
            rpt_name="BMTrferTotal";
        else
            rpt_name="BMTotal";
    } else {
        if(rpt_params.pr_trfer)
            rpt_name="BMTrfer";
        else
            rpt_name="BM";
    };
    if (rpt_params.rpt_type==rtBTMTXT) rpt_name=rpt_name+"Txt";
    get_report_form(rpt_name, resNode);

    t_rpt_bm_bag_name bag_names;
    Qry.Clear();
    Qry.SQLText = "select airp from points where point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("RunBMNew: point_id %d not found", rpt_params.point_id);
    string airp = Qry.FieldAsString(0);
    bag_names.init(airp);
    vector<TBagTagRow> bag_tags;
    map<int, vector<int> > grps;
    Qry.Clear();
    string SQLText =
        "select ";
    if(rpt_params.pr_trfer)
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
        "    pax.pax_id, "
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
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips ";
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
        "    pax_grp.hall = halls2.id(+) ";
    if(!rpt_params.airp_arv.empty()) {
        SQLText += " and pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and pax_grp.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    if(rpt_params.pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";
    SQLText +=
        "   and pax.pax_id(+) = ckin.get_main_pax_id(pax_grp.grp_id) and "
        "   decode(:pr_brd_pax, 0, nvl2(pax.pr_brd(+), 0, -1), pax.pr_brd(+))  = :pr_brd_pax and "
        "   (pax_grp.class is not null and pax.pax_id is not null or pax_grp.class is null) ";
    Qry.CreateVariable("pr_brd_pax", otInteger, rpt_params.pr_brd);
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        if(not rpt_params.mkt_flt.IsNULL()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(Qry.FieldAsInteger("pax_id"));
            if(mkt_flt.IsNULL() or not(rpt_params.mkt_flt == mkt_flt))
                continue;
        }

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
                    grps[cur_grp_id].push_back(bag_tags.size() - 1);
                }
            } else if(grps[cur_grp_id].size()) {
                // если встретилась группа со списком непривязанных бирок
                // привязываем текущий багаж к одной из бирок и удаляем ее
                // (на самом деле в базе она не привязана, просто чтоб багажка правильно выводилась)
                bag_tags[grps[cur_grp_id].back()].bag_num = bag_tag_row.bag_num;
                bag_tags[grps[cur_grp_id].back()].amount = bag_tag_row.amount;
                bag_tags[grps[cur_grp_id].back()].weight = bag_tag_row.weight;
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


    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, rpt_params.pr_trfer ? "v_bm_trfer" : "v_bm");

    int TotAmount = 0;
    int TotWeight = 0;
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
        NewTextChild(rowNode, "pr_vip", 2);

        if(!iv->class_code.empty()) {
            TBaseTableRow &classesRow = base_tables.get("CLASSES").get_row("code",iv->class_code);
            iv->class_code = classesRow.AsString("code", pr_lat);
            iv->class_name = classesRow.AsString("name", pr_lat);
        }

        NewTextChild(rowNode, "class", iv->class_code);
        NewTextChild(rowNode, "class_name", iv->class_name);
        NewTextChild(rowNode, "amount", iv->amount);
        NewTextChild(rowNode, "weight", iv->weight);
        TotAmount += iv->amount;
        TotWeight += iv->weight;
        Qry.Next();
    }
    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "TotPcs", TotAmount);
    NewTextChild(variablesNode, "TotWeight", TotWeight);
    NewTextChild(variablesNode, "Tot", (pr_lat ? "" : vs_number(TotAmount)));
    NewTextChild(variablesNode, "pr_lat", pr_lat);
    Qry.Clear();
    Qry.SQLText =
        "select "
        "  sum(bag2.amount) amount, "
        "  sum(bag2.weight) weight "
        "from "
        "  pax_grp, "
        "  bag2, "
        "  transfer "
        "where "
        "  pax_grp.point_dep = :point_id and "
        "  pax_grp.grp_id = bag2.grp_id and "
        "  pax_grp.grp_id = transfer.grp_id and "
        "  pax_grp.bag_refuse = 0 and "
        "  bag2.pr_cabin = 0 and "
        "  transfer.pr_final <> 0 ";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    int trfer_amount = 0;
    int trfer_weight = 0;
    if(not Qry.Eof) {
        trfer_amount = Qry.FieldAsInteger("amount");
        trfer_weight = Qry.FieldAsInteger("weight");
    }
    NewTextChild(variablesNode, "TotTrferPcs", trfer_amount);
    NewTextChild(variablesNode, "TotTrferWeight", trfer_weight);
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
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    int airline_fmt = Qry.FieldAsInteger("airline_fmt");
    int suffix_fmt = Qry.FieldAsInteger("suffix_fmt");
    int craft_fmt = Qry.FieldAsInteger("craft_fmt");

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

    NewTextChild(variablesNode, "own_airp_name", "АЭРОПОРТ " + airpRow.AsString("name", false));
    NewTextChild(variablesNode, "own_airp_name_lat", airpRow.AsString("name", true) + " AIRPORT");
    NewTextChild(variablesNode, "airp_dep_name", airpRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "airline_name", airlineRow.AsString("name", pr_lat));
    NewTextChild(variablesNode, "flt",
            ElemIdToElem(etAirline, airline, airline_fmt, pr_lat) +
            IntToString(flt_no) +
            ElemIdToElem(etSuffix, suffix, suffix_fmt, pr_lat)
            );
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", ElemIdToElem(etCraft, craft, craft_fmt, pr_lat));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", pr_lat));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", pr_lat));
    string airp_arv_name;
    if(rpt_params.airp_arv.size())
        airp_arv_name = base_tables.get("AIRPS").get_row("code",rpt_params.airp_arv).AsString("name",pr_lat);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    {
        // delete in future 14.10.07 !!!
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", pr_lat));
    string pr_brd_pax_str_lat;
    string pr_brd_pax_str;
    if(rpt_params.pr_brd == 0) {
        pr_brd_pax_str_lat = "(checked)";
        pr_brd_pax_str = "(зарег)";
    } else {
        pr_brd_pax_str_lat = "(boarded)";
        pr_brd_pax_str = "(посаж)";
    }
    NewTextChild(variablesNode, "pr_brd_pax_lat", pr_brd_pax_str_lat);
    NewTextChild(variablesNode, "pr_brd_pax", pr_brd_pax_str);
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params.ckin_zone, pr_lat));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    STAT::set_variables(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void PTMBTMTXT(const TRptParams &rpt_params, xmlNodePtr resNode)
{
  if (rpt_params.rpt_type==rtPTMTXT)
    PTM(rpt_params, resNode);
  else
    BTM(rpt_params, resNode);

  bool lat = GetRPEncoding(rpt_params)!=0;

  xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
  xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);

  string str;
  ostringstream s;
  //текстовый формат
  int page_width=80;
  //специально вводим для кириллических символов, так как в терминале при экспорте проблемы
  //максимальная длина строки при экспорте в байтах! не должна превышать ~147 (65 рус + 15 лат)
  int max_symb_count=lat?page_width:65;
  NewTextChild(variablesNode, "page_width", page_width);
  NewTextChild(variablesNode, "test_server", get_test_server());

  s.str("");
  for(int i=0;i<page_width/6;i++) s << (lat?" TEST ":" ТЕСТ ");
  NewTextChild(variablesNode, "test_str", s.str());


  s.str("");
  if (rpt_params.rpt_type==rtPTMTXT)
  {
    str.assign(lat?"PASSENGER MANIFEST ":"ПАССАЖИРСКАЯ ВЕДОМОСТЬ ");
    str.append(NodeAsString((lat?"et_lat":"et"),variablesNode));
  }
  else
    str.assign(lat?"BAGGAGE MANIFEST ":"БАГАЖНАЯ ВЕДОМОСТЬ ");

  s << setfill(' ')
    << str
    << right << setw(page_width-str.size())
    << string(NodeAsString((lat?"own_airp_name_lat":"own_airp_name"),variablesNode)).substr(0,max_symb_count-str.size());
  NewTextChild(variablesNode, "page_header_top", s.str());


  s.str("");
  str.assign(lat?"Owner or Operator: ":"Владелец или Оператор: ");
  s << left
    << str
    << string(NodeAsString("airline_name",variablesNode)).substr(0,max_symb_count-str.size()) << endl
    << setw(10) << (lat?"Flight №":"№ рейса");
  if (lat)
    s << setw(19) << "Aircraft";
  else
    s << setw(9)  << "№ ВС"
      << setw(10) << "ТипВС Ст. ";

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(20) << (lat?"Embarkation":"А/п вылета")
      << setw(20) << (lat?"Disembarkation":"А/п назначения");
  else
    s << setw(40) << (lat?"Embarkation":"А/п вылета");
  s << setw(6)  << (lat?"Date":"Дата")
    << setw(5)  << (lat?"Time":"Время") << endl;

  s << setw(10) << NodeAsString("flt",variablesNode)
    << setw(11) << NodeAsString("bort",variablesNode)
    << setw(4)  << NodeAsString("craft",variablesNode)
    << setw(4)  << NodeAsString("park",variablesNode);

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(20) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,20-1)
      << setw(20) << string(NodeAsString("airp_arv_name",variablesNode)).substr(0,20-1);
  else
    s << setw(40) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,40-1);

  s << setw(6) << NodeAsString("scd_date",variablesNode)
    << setw(5) << NodeAsString("scd_time",variablesNode);
  NewTextChild(variablesNode, "page_header_center", s.str() );

  s.str("");
  if (rpt_params.rpt_type==rtPTMTXT)
    str.assign(lat?"PASSENGERS ":"ПАССАЖИРЫ ");
  else
    str.assign(lat?"BAGGAGE ":"БАГАЖ ");
  str.append(NodeAsString((lat?"pr_brd_pax_lat":"pr_brd_pax"),variablesNode));
  if (!NodeIsNULL("zone",variablesNode))
  {
    unsigned int zone_len=max_symb_count-str.size()-1;
    string zone;
    zone.assign(lat?"Checking zone: ":"Зал регистрации: ");
    zone.append(NodeAsString("zone",variablesNode));
    if (zone_len<zone.size())
      s << str << right << setw(page_width-str.size()) << zone.substr(0,zone_len-3).append("...") << endl;
    else
      s << str << right << setw(page_width-str.size()) << zone << endl;
  }
  else
    s << str << endl;

  if (rpt_params.rpt_type==rtPTMTXT)
    s << left
      << setw(4)  << (lat?"Sec":"Рег")
      << setw(lat?22:23) << (lat?"Surname":"Фамилия, имя")
      << setw(lat?4:3)   << (lat?"Cls":"Кл")
      << setw(5)  << (lat?"Seat":"№ м")
      << setw(4)  << (lat?"CHD":"РБ")
      << setw(4)  << (lat?"INF":"РМ")
      << setw(7)  << (lat?"Bag.":"Багаж")
      << setw(6)  << (lat?"Cabin":"Р/кл")
      << setw(15) << (lat?"Bag.Tag.No":"№№ баг.бирок")
      << setw(9)  << (lat?"Remarks":"Ремарки");
  else
    s << left
      << setw(29) << (lat?"Baggage tag numbers":"Номера багажных бирок")
      << setw(10) << (lat?"Color":"Цвет")
      << setw(5)  << (lat?"Pcs":"Мест")
      << setw(7)  << (lat?"Weight":"Вес")
      << setw(8)  << (lat?"Cont.№":"№ Конт.")
      << setw(10) << (lat?"Hold":"Багажник")
      << setw(11) << (lat?"Compartment":"Отсек");

  NewTextChild(variablesNode, "page_header_bottom", s.str() );

  if (rpt_params.rpt_type==rtPTMTXT)
  {
    s.str("");
    s << (lat?"Total in class":"Всего в классе") << endl
      << "%-30s%4u %3u %3u %2u/%-4u%4u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (lat?"Total":"Всего")
        << setw(50) << str.substr(0,50-1)
        << (lat?"Prepared by":"Подпись") << endl;
    }
    else
      s << left
        << setw(56) << (lat?"Total":"Всего")
        << (lat?"Prepared by":"Подпись") << endl;

    s << setw(7) << (lat?"SOC":"Кресел")
      << setw(7) << (lat?"ADU/F":"ВЗ/Ж")
      << setw(7) << (lat?"CHD":"РБ")
      << setw(7) << (lat?"INF":"РМ")
      << setw(7) << (lat?"Pcs":"Мест")
      << setw(7) << (lat?"Weight":"Вес")
      << setw(7) << (lat?"Cabin":"Р/кл.")
      << setw(7) << (lat?"Ex.Bag.":"Плат.") << endl
      << "%-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u" << endl
      << (lat?"Issue date ":"Сформировано ") << NodeAsString("date_issue",variablesNode);

    NewTextChild(variablesNode, "page_footer_top", s.str() );


    xmlNodePtr dataSetNode = NodeAsNode("v_pm_trfer", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(lat?"TO: ":"ДО: ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //рабиваем фамилию, бирки, ремарки
      SeparateString(NodeAsString("full_name",rowNode),22,rows);
      fields["full_name"]=rows;
      SeparateString(NodeAsString("tags",rowNode),15,rows);
      fields["tags"]=rows;
      SeparateString(NodeAsString("remarks",rowNode),9,rows);
      fields["remarks"]=rows;

      row=0;
      string pers_type=NodeAsString("pers_type",rowNode);
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << right << setw(3) << (row==0?NodeAsString("reg_no",rowNode):"") << " "
          << left << setw(22) << (!fields["full_name"].empty()?*(fields["full_name"].begin()):"") << " "
          << left <<  setw(3) << (row==0?NodeAsString("class",rowNode):"")
          << right << setw(4) << (row==0?NodeAsString("seat_no",rowNode):"") << " "
          << left <<  setw(4) << (row==0&&pers_type=="CHD"?" X ":"")
          << left <<  setw(4) << (row==0&&pers_type=="INF"?" X ":"");
        if (row!=0 ||
            NodeAsInteger("bag_amount",rowNode)==0 &&
            NodeAsInteger("bag_weight",rowNode)==0)
          s << setw(7) << "";
        else
          s << right << setw(2) << NodeAsInteger("bag_amount",rowNode) << "/"
            << left << setw(4) << NodeAsInteger("bag_weight",rowNode);
        if (row!=0 ||
            NodeAsInteger("rk_weight",rowNode)==0)
          s << setw(5) << "";
        else
          s << right << setw(4) << NodeAsInteger("rk_weight",rowNode) << " ";
        s << left << setw(15) << (!fields["tags"].empty()?*(fields["tags"].begin()):"") << " "
          << left << setw(9) << (!fields["remarks"].empty()?*(fields["remarks"].begin()):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["full_name"].empty() ||
            !fields["tags"].empty() ||
            !fields["remarks"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
    {
      s.str("");
      if (!rpt_params.pr_trfer)
        s << setw(20) << (lat?"Total luggage":"Всего багажа");
      else
      {
        if (k==0)
          s << setw(24) << (lat?"Total not transfer lug.":"Всего нетрансф. багажа");
        else
          s << setw(24) << (lat?"Total transfer luggage":"Всего трансф. багажа");
      };

      s << setw(7) << (lat?"SOC":"Кресел")
        << setw(7) << (lat?"ADU/F":"ВЗ/Ж")
        << setw(7) << (lat?"CHD":"РБ")
        << setw(7) << (lat?"INF":"РМ")
        << setw(7) << (lat?"Pcs":"Мест")
        << setw(7) << (lat?"Weight":"Вес")
        << setw(7) << (lat?"Cabin":"Р/кл.")
        << setw(7) << (lat?"Ex.Bag.":"Плат.");

      if (!rpt_params.pr_trfer)
        NewTextChild(variablesNode, "subreport_header", s.str() );
      else
      {
        if (k==0)
          NewTextChild(variablesNode, "subreport_header", s.str() );
        else
          NewTextChild(variablesNode, "subreport_header_trfer", s.str() );
      };
    };

    dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_pm_trfer_total" : "v_pm_total", dataSetsNode);

    rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      s.str("");
      s << setw(rpt_params.pr_trfer?24:20) << NodeAsString("class_name",rowNode)
        << setw(7) << NodeAsInteger("seats",rowNode)
        << setw(7) << NodeAsInteger("adl",rowNode)
        << setw(7) << NodeAsInteger("chd",rowNode)
        << setw(7) << NodeAsInteger("inf",rowNode)
        << setw(7) << NodeAsInteger("bag_amount",rowNode)
        << setw(7) << NodeAsInteger("bag_weight",rowNode)
        << setw(7) << NodeAsInteger("rk_weight",rowNode)
        << setw(7) << NodeAsInteger("excess",rowNode);

      NewTextChild(rowNode,"str",s.str());
    };
  }
  else
  {
    s.str("");
    s << "%-39s%4u %6u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    xmlNodePtr dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_bm_trfer" : "v_bm", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(lat?"TO: ":"ДО: ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //разбиваем диапазоны бирок, цвет
      string bag_name=NodeAsString("bag_name",rowNode);
      SeparateString(NodeAsString("birk_range",rowNode),bag_name.empty()?26:24,rows);
      fields["birk_range"]=rows;
      SeparateString(NodeAsString("color",rowNode),9,rows);
      fields["color"]=rows;

      row=0;
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << setw(bag_name.empty()?2:4) << "" //отступ
          << left << setw(bag_name.empty()?26:24) << (!fields["birk_range"].empty()?*(fields["birk_range"].begin()):"") << " "
          << left << setw(9) << (!fields["color"].empty()?*(fields["color"].begin()):"") << " "
          << right << setw(4) << (row==0?NodeAsString("num",rowNode):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["birk_range"].empty() ||
            !fields["color"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    if (rpt_params.pr_trfer)
    {
      for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
      {
        s.str("");
        s << left;
        if (k==0)
          s << setw(39) << (lat?"Total luggage exclusive of transfer":"Всего багажа, исключая трансферный");
        else
          s << setw(39) << (lat?"Total transfer luggage":"Всего трансферного багажа");
        s << "%4u %6u";
        if (k==0)
          NewTextChild(variablesNode, "total_not_trfer_fmt", s.str() );
        else
          NewTextChild(variablesNode, "total_trfer_fmt", s.str() );
      };

      s.str("");
      s << setw(39) << (lat?"Total luggage":"Всего багажа")
        << "%4u %6u" << endl
        << setw(39) << (lat?"Transfer luggage":"Трансферного багажа")
        << "%4u %6u";
      NewTextChild(variablesNode, "report_footer", s.str() );
    };

    s.str("");
    s << (lat?"Issue date ":"Сформировано ") << NodeAsString("date_issue",variablesNode);
    NewTextChild(variablesNode, "page_footer_top", s.str() );



    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (lat?"Total":"Всего")
        << setw(50) << str.substr(0,50-1)
        << (lat?"Signature of agent":"Подпись агента СОПП") << endl;
    }
    else
      s << left
        << setw(56) << (lat?"Total":"Всего")
        << (lat?"Signature of agent":"Подпись агента СОПП") << endl;

    s << setw(6)  << (lat?"Pcs":"Мест")
      << setw(7)  << (lat?"Weight":"Вес")
      << setw(43) << (lat?"Pieces in letters":"Количество мест прописью") << endl;

    SeparateString(NodeAsString("Tot",variablesNode),42,rows);
    row=0;
    do
    {
      if (row!=0) s << endl;
      s << setw(6)  << (row==0?NodeAsString("TotPcs",variablesNode):"")
        << setw(7)  << (row==0?NodeAsString("TotWeight",variablesNode):"")
        << setw(42) << (!rows.empty()?*(rows.begin()):"");
      if (!rows.empty()) rows.erase(rows.begin());
      row++;
    }
    while(!rows.empty());
    NewTextChild(variablesNode,"report_summary",s.str());
  };
  ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str()); //!!!
};

string get_flight(xmlNodePtr variablesNode)
{
    return (string)NodeAsString("trip", variablesNode) + "/" + NodeAsString("scd_out", variablesNode);
}

void REFUSE(TRptParams &rpt_params, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREFUSETXT)
        get_report_form("docTxt", resNode);
    else
        get_report_form("ref", resNode);
    int pr_lat = GetRPEncoding(rpt_params);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name,1)) family, "
        "       decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "       ticket_no, "
        "       decode(:pr_lat, 0, refusal_types.name, NVL(refusal_types.name_lat,refusal_types.name)) refuse, "
        "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) AS tags "
        "FROM   pax_grp,pax,pers_types,refusal_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pers_type=pers_types.code AND "
        "       pax.refuse = refusal_types.code AND "
        "       pax.refuse IS NOT NULL and "
        "       point_dep = :point_id "
        "order by "
        "       reg_no ";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
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
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, pr_lat, variablesNode);
    NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.REFUSE", LParams() << LParam("flight", get_flight(variablesNode))));
    populate_doc_cap(variablesNode, pr_lat);
}

string get_test_str(int page_width, bool lat)
{
    string result;
    for(int i=0;i<page_width/6;i++) result += " " + translateDocCap(lat, "CAP.TEST") + " ";
    return result;
}

void REFUSETXT(TRptParams &rpt_params, xmlNodePtr resNode)
{
    REFUSE(rpt_params, resNode);
    bool lat = GetRPEncoding(rpt_params)!=0;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=lat?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", get_test_server());
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, lat));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << translateDocCap(lat, "№") << " "
        << left
        << setw(21) << translateDocCap(lat, "Ф.И.О.")
        << setw(5)  << translateDocCap(lat, "Тип")
        << setw(10) << translateDocCap(lat, "№ Билета")
        << setw(24)  << translateDocCap(lat, "Причина невылета")
        << setw(16) << translateDocCap(lat, "№ б/б");
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            translateDocCap(lat, "CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode))));
    xmlNodePtr dataSetNode = NodeAsNode("v_ref", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("family", rowNode), 20, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("ticket_no", rowNode), 9, rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("refuse", rowNode), 23, rows);
        fields["refuse"]=rows;

        SeparateString(NodeAsString("tags", rowNode), 16, rows);
        fields["tags"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(20) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
//                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode, нафига вот это??? -> "ВЗ") : "") << " " << col_sym
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode) : "") << " " << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(23) << (!fields["refuse"].empty() ? *(fields["refuse"].begin()) : "") << col_sym
                << left << setw(16) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["tkts"].empty() ||
                !fields["refuse"].empty() ||
                !fields["tags"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void NOTPRES(TRptParams &rpt_params, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtNOTPRESTXT)
        get_report_form("docTxt", resNode);
    else
        get_report_form("notpres", resNode);
    int pr_lat = GetRPEncoding(rpt_params);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name,1)) family, "
        "       pers_types.code pers_type, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no, "
        "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bagAmount, "
        "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bagWeight, "
        "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) AS tags "
        "FROM   pax_grp,pax,pers_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pers_type=pers_types.code AND "
        "       pax.pr_brd=0 and "
        "       point_dep = :point_id "
        "order by "
        "       reg_no ";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
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
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, pr_lat, variablesNode);
    NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.NOTPRES",
                LParams() << LParam("flight", get_flight(variablesNode))));
    populate_doc_cap(variablesNode, pr_lat);
}

void NOTPRESTXT(TRptParams &rpt_params, xmlNodePtr resNode)
{
    NOTPRES(rpt_params, resNode);
    bool lat = GetRPEncoding(rpt_params)!=0;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=lat?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", get_test_server());
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, lat));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << translateDocCap(lat, "№") << " "
        << left
        << setw(38) << translateDocCap(lat, "Ф.И.О.")
        << setw(5)  << translateDocCap(lat, "Тип")
        << setw(8)  << translateDocCap(lat, "№ м")
        << setw(6) << translateDocCap(lat, "Баг.")
        << " " << setw(19) << translateDocCap(lat, "№ б/б");
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            translateDocCap(lat, "CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode))));
    xmlNodePtr dataSetNode = NodeAsNode("v_notpres", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("family", rowNode), 37, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("tags", rowNode), 19, rows);
        fields["tags"]=rows;

        row=0;
        s.str("");
        do
        {
            string bagamount = NodeAsString("bagamount", rowNode, "");
            if(bagamount == "0") bagamount.erase();
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(37) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << " " << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left <<  setw(5) << (row == 0 ? bagamount : "") << col_sym
                << left << setw(19) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["tags"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void REM(TRptParams &rpt_params, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREMTXT)
        get_report_form("docTxt", resNode);
    else
        get_report_form("rem", resNode);
    int pr_lat = GetRPEncoding(rpt_params);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       decode(:pr_lat, 0, surname||' '||pax.name, system.transliter(surname||' '||pax.name,1)) family, "
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
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otString, pr_lat);
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
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
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, pr_lat, variablesNode);
    NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.REM",
                LParams() << LParam("flight", get_flight(variablesNode))));
    populate_doc_cap(variablesNode, pr_lat);
}

void REMTXT(TRptParams &rpt_params, xmlNodePtr resNode)
{
    REM(rpt_params, resNode);
    bool lat = GetRPEncoding(rpt_params)!=0;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=lat?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", get_test_server());
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, lat));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << translateDocCap(lat, "№") << " "
        << left
        << setw(38) << translateDocCap(lat, "Ф.И.О.")
        << setw(5)  << translateDocCap(lat, "Тип")
        << setw(8)  << translateDocCap(lat, "№ м")
        << setw(25) << translateDocCap(lat, "Ремарки");
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            translateDocCap(lat, "CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode))));
    xmlNodePtr dataSetNode = NodeAsNode("v_rem", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("family", rowNode), 37, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("info", rowNode), 25, rows);
        fields["rems"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(37) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left << setw(25) << (!fields["rems"].empty() ? *(fields["rems"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["rems"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void CRS(TRptParams &rpt_params, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtCRSTXT or rpt_params.rpt_type == rtCRSUNREGTXT)
        get_report_form("docTxt", resNode);
    else
        get_report_form("crs", resNode);
    bool pr_unreg = rpt_params.rpt_type == rtCRSUNREG or rpt_params.rpt_type == rtCRSUNREGTXT;
    int pr_lat = GetRPEncoding(rpt_params);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "      tlg_binding.point_id_spp AS point_id, "
        "      ckin.get_pnr_addr(crs_pnr.pnr_id) AS pnr_ref, "
        "      decode(:pr_lat, 0, (crs_pax.surname||' '||crs_pax.name), system.transliter(crs_pax.surname||' '||crs_pax.name,1)) family, "
        "      decode(:pr_lat, 0, pers_types.code, pers_types.code_lat) pers_type, "
        "      decode(:pr_lat, 0, classes.code, classes.code_lat) class, "
        "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS seat_no, "
        "      decode(:pr_lat, 0, airps.code, NVL(airps.code_lat,airps.code)) target, "
        "      report.get_trfer_airp(last_target) last_target, "
        "      report.get_TKNO(crs_pax.pax_id) ticket_no, "
        "      report.get_PSPT(crs_pax.pax_id) AS document, "
        "      report.get_crsRemarks(crs_pax.pax_id) AS remarks "
        "FROM crs_pnr,tlg_binding,crs_pax,pers_types,classes,airps ";
    if(pr_unreg)
        SQLText += " , pax ";
    SQLText +=
        "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pers_type=pers_types.code AND "
        "      crs_pnr.class=classes.code AND "
        "      crs_pnr.target=airps.code AND "
        "      crs_pax.pr_del=0 and "
        "      tlg_binding.point_id_spp = :point_id ";
    if(pr_unreg)
        SQLText +=
            "    and crs_pax.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null ";
    SQLText +=
        "order by "
        "    family ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
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
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, pr_lat, variablesNode);
    if(pr_unreg)
        NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.CRSUNREG",
                    LParams() << LParam("flight", get_flight(variablesNode))));
    else
        NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.CRS",
                    LParams() << LParam("flight", get_flight(variablesNode))));
    populate_doc_cap(variablesNode, pr_lat);
}

void CRSTXT(TRptParams &rpt_params, xmlNodePtr resNode)
{
    CRS(rpt_params, resNode);
    bool lat = GetRPEncoding(rpt_params)!=0;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=lat?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", get_test_server());
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, lat));
    ostringstream s;
    vector<string> rows;
    string str;
    SeparateString(NodeAsString("caption", variablesNode), max_symb_count, rows);
    s.str("");
    for(vector<string>::iterator iv = rows.begin(); iv != rows.end(); iv++) {
        if(iv != rows.begin())
            s << endl;
        s << right << setw(((page_width - iv->size()) / 2) + iv->size()) << *iv;
    }
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << translateDocCap(lat, "№") << " "
        << left
        << setw(7)  << "PNR"
        << setw(22) << translateDocCap(lat, "Ф.И.О.")
        << setw(5)  << translateDocCap(lat, "Пас")
        << setw(3) << translateDocCap(lat, "Кл")
        << setw(8)  << translateDocCap(lat, "№ м")
        << setw(4)  << translateDocCap(lat, "CAP.DOC.AIRP_ARV")
        << setw(7)  << translateDocCap(lat, "CAP.DOC.TO")
        << setw(10) << translateDocCap(lat, "Билет")
        << setw(10) << translateDocCap(lat, "Документ");
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            translateDocCap(lat, "CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode))));
    xmlNodePtr dataSetNode = NodeAsNode("v_crs", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    int row_num = 1;
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("pnr_ref", rowNode), 6, rows);
        fields["pnrs"]=rows;

        SeparateString(NodeAsString("family", rowNode), 21, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("ticket_no", rowNode), 9, rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("document", rowNode), 10, rows);
        fields["docs"]=rows;


        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? IntToString(row_num++) : "") << col_sym
                << left << setw(6) << (!fields["pnrs"].empty() ? *(fields["pnrs"].begin()) : "") << col_sym
                << left << setw(21) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym
                << left << setw(2) << (row == 0 ? NodeAsString("class", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left << setw(3) << (row == 0 ? NodeAsString("target", rowNode) : "") << col_sym
                << left << setw(6) << (row == 0 ? NodeAsString("last_target", rowNode) : "") << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(10) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["pnrs"].empty() ||
                !fields["surname"].empty() ||
                !fields["tkts"].empty() ||
                !fields["docs"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void EXAM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_web = GetNode( "web", reqNode ) != NULL;
    if(rpt_params.rpt_type == rtEXAMTXT or rpt_params.rpt_type == rtWEBTXT)
        get_report_form("docTxt", resNode);
    else
        get_report_form(pr_web ? "web" : "exam", resNode);
    int pr_lat = GetRPEncoding(rpt_params);

    BrdInterface::GetPax(reqNode, resNode);
    xmlNodePtr currNode = resNode->children;
    xmlNodePtr formDataNode = NodeAsNodeFast("form_data", currNode);
    xmlNodePtr dataNode = NodeAsNodeFast("data", currNode);
    currNode = formDataNode->children;
    xmlNodePtr variablesNode = NodeAsNodeFast("variables", currNode);

    xmlUnlinkNode(dataNode);

    xmlNodeSetName(dataNode, (xmlChar *)"datasets");
    xmlAddChild(formDataNode, dataNode);
    // Теперь переменные отчета
    PaxListVars(rpt_params.point_id, pr_lat, variablesNode);
    if ( GetNode( "web", reqNode ) )
        NewTextChild(variablesNode, "paxlist_type", translateDocCap(pr_lat, "CAP.PAX_LIST.WEB"));
    else
        NewTextChild(variablesNode, "paxlist_type", translateDocCap(pr_lat, "CAP.PAX_LIST.BRD"));
    NewTextChild(variablesNode, "caption", translateDocCap(pr_lat, "CAP.DOC.PAX_LIST",
                LParams()
                << LParam("list_type", NodeAsString("paxlist_type", variablesNode))
                << LParam("flight", get_flight(variablesNode)))
            );
    currNode = variablesNode->children;
    xmlNodePtr totalNode = NodeAsNodeFast("total", currNode);
    NodeSetContent(totalNode, translateDocCap(pr_lat, "CAP.TOTAL.VAL", LParams() << LParam("total", NodeAsString(totalNode))));
    populate_doc_cap(variablesNode, pr_lat);
}

void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EXAM(rpt_params, reqNode, resNode);
    const char col_sym = ' '; //символ разделителя столбцов
    bool lat = GetRPEncoding(rpt_params)!=0;
    bool pr_web = rpt_params.rpt_type == rtWEBTXT;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=lat?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", get_test_server());
    string str;
    ostringstream s;
    s.str("");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, lat));
    s.str("");
    s << NodeAsString("caption", variablesNode);
    str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << translateDocCap(lat, "№") << col_sym
        << left << setw(pr_web ? 20 : 22) << translateDocCap(lat, "Фамилия")
        << setw(4)  << translateDocCap(lat, "Пас");
    if(pr_web)
        s
            << setw(9)  << translateDocCap(lat, "№ м");
    else
        s
            << setw(3)  << translateDocCap(lat, "Дс")
            << setw(4)  << translateDocCap(lat, "Пс");
    s
        << setw(10) << translateDocCap(lat, "Документ")
        << setw(10) << translateDocCap(lat, "Билет")
        << setw(15) << translateDocCap(lat, "№ б/б")
        << setw(8)  << translateDocCap(lat, "Ремарки");
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    s.str("");
    s
        << NodeAsString("total", variablesNode) << endl
        << translateDocCap(lat, "CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)));
    NewTextChild(variablesNode, "page_footer_top", s.str() );

    xmlNodePtr dataSetNode = NodeAsNode("passengers", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        //рабиваем фамилию, документ, билет, бирки, ремарки
        SeparateString(((string)NodeAsString("surname",rowNode) + " " + NodeAsString("name", rowNode, "")).c_str(),(pr_web ? 19 : 21),rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("document",rowNode, ""),9,rows);
        fields["docs"]=rows;

        SeparateString(NodeAsString("ticket_no",rowNode, ""),9,rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("tags",rowNode, ""),14,rows);
        fields["tags"]=rows;

        SeparateString(NodeAsString("remarks",rowNode, ""),8,rows);
        fields["remarks"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(pr_web ? 19 : 21) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym;
            if(pr_web) {
                s
                    << left <<  setw(8) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym;
            } else {
                s
                    << left <<  setw(3) << (row == 0 ? (NodeAsString("pr_exam", rowNode, "") == "" ? "-" : "+") : "")
                    << left <<  setw(4) << (row == 0 ? (NodeAsString("pr_brd", rowNode, "") == "" ? "-" : "+") : "");
            }
            s
                << left << setw(9) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "") << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(14) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "") << col_sym
                << left << setw(8) << (!fields["remarks"].empty() ? *(fields["remarks"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["docs"].empty() ||
                !fields["tkts"].empty() ||
                !fields["tags"].empty() ||
                !fields["remarks"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void WEB(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "web");
    EXAM(rpt_params, reqNode, resNode);
}

void WEBTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "web");
    EXAMTXT(rpt_params, reqNode, resNode);
}

void  DocsInterface::RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int fmt;
    xmlNodePtr node = reqNode->children;
    TRptParams rpt_params;
    rpt_params.point_id = NodeAsIntegerFast("point_id", node);
    rpt_params.rpt_type = DecodeRptType(NodeAsStringFast("rpt_type", node));
    rpt_params.airp_arv = ElemToElemId(etAirp, NodeAsStringFast("airp_arv", node, ""), fmt);
    rpt_params.ckin_zone = NodeAsStringFast("ckin_zone", node, " ");
    rpt_params.pr_et = NodeAsIntegerFast("pr_et", node, 0) != 0;
    rpt_params.pr_trfer = NodeAsIntegerFast("pr_trfer", node, 0) != 0;
    rpt_params.pr_brd = NodeAsIntegerFast("pr_brd", node, 0) != 0;
    xmlNodePtr mktFltNode = GetNodeFast("mkt_flight", node);
    if(mktFltNode != NULL) {
        xmlNodePtr node = mktFltNode->children;
        rpt_params.mkt_flt.airline = ElemToElemId(etAirline, NodeAsStringFast("airline", node), fmt);
        rpt_params.mkt_flt.flt_no = NodeAsIntegerFast("flt_no", node);
        rpt_params.mkt_flt.suffix = ElemToElemId(etSuffix, NodeAsStringFast("suffix", node, ""), fmt);
    }
    switch(rpt_params.rpt_type) {
        case rtPTM:
            PTM(rpt_params, resNode);
            break;
        case rtPTMTXT:
            PTMBTMTXT(rpt_params, resNode);
            break;
        case rtBTM:
            BTM(rpt_params, resNode);
            break;
        case rtBTMTXT:
            PTMBTMTXT(rpt_params, resNode);
            break;
        case rtWEB:
            WEB(rpt_params, reqNode, resNode);
            break;
        case rtWEBTXT:
            WEBTXT(rpt_params, reqNode, resNode);
            break;
        case rtREFUSE:
            REFUSE(rpt_params, resNode);
            break;
        case rtREFUSETXT:
            REFUSETXT(rpt_params, resNode);
            break;
        case rtNOTPRES:
            NOTPRES(rpt_params, resNode);
            break;
        case rtNOTPRESTXT:
            NOTPRESTXT(rpt_params, resNode);
            break;
        case rtREM:
            REM(rpt_params, resNode);
            break;
        case rtREMTXT:
            REMTXT(rpt_params, resNode);
            break;
        case rtCRS:
        case rtCRSUNREG:
            CRS(rpt_params, resNode);
            break;
        case rtCRSTXT:
        case rtCRSUNREGTXT:
            CRSTXT(rpt_params, resNode);
            break;
        case rtEXAM:
            EXAM(rpt_params, reqNode, resNode);
            break;
        case rtEXAMTXT:
            EXAMTXT(rpt_params, reqNode, resNode);
            break;
        case rtUnknown:
        case rtTypeNum:
            throw AstraLocale::UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
    }
}

void  DocsInterface::SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);
    string version = NodeAsString("version", reqNode, "");
    ProgTrace(TRACE5, "VER. %s", version.c_str());
    if(version == "")
        version = get_report_version(name);

    string form = NodeAsString("form", reqNode);
    if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION)) {
        Qry.SQLText = "update fr_forms2 set form = :form where name = :name and version = :version";
        Qry.CreateVariable("version", otString, version);
    } else
        Qry.SQLText = "update fr_forms set form = :form where name = :name";
    Qry.CreateVariable("name", otString, name);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.Execute();
    if(!Qry.RowsProcessed()) {
        if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION))
            Qry.SQLText = "insert into fr_forms2(name, version, form) values(:name, '000000-0000000', :form)";
        else
            Qry.SQLText = "insert into fr_forms(id, name, form) values(id__seq.nextval, :name, :form)";
        Qry.Execute();
    }
    TReqInfo::Instance()->MsgToLog( (string)"Обновлен шаблон отчета " + name, evtSystem);
}

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
        result.insert(result.begin(), " ");
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
            NewTextChild(itemNode, "name");
        } else {
            NewTextChild(itemNode, "code", *iv);
            NewTextChild(itemNode, "name", *iv);
        }
    }
}

void DocsInterface::GetFonts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	tst();
  NewTextChild(resNode,"fonts","");
  tst();
}
