#include <set>
#include "docs.h"
#include "stat.h"
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
#include "load_fr.h"
#include "passenger.h"
#include "remarks.h"
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
    static const char* stuki_g[] = {
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
    static const char* stuki_m[] = {
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

string vsHow_lat(int nmb, int range)
{
    static const char* sotni[] = {
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
    static const char* teen[] = {
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
    static const char* desatki[] = {
        "twenty ",
        "thirty ",
        "forty ",
        "fifty ",
        "sixty ",
        "seventy ",
        "eighty ",
        "ninety "
    };
    static const char* stuki_m[] = {
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
    static const char* dtext[2][3] = {
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

bool bad_client_img_version()
{
    return TReqInfo::Instance()->desk.compatible("201101-0117116") and not TReqInfo::Instance()->desk.compatible("201101-0118748");
}

void SeasonListVars(int trip_id, int pr_lat, xmlNodePtr variablesNode, xmlNodePtr reqNode)
{
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
	vector<SEASON::TViewPeriod> viewp;
	SEASON::ReadTripInfo( trip_id, viewp, GetNode( "seasonvars", reqNode ) );
  for ( vector<SEASON::TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
    for ( vector<SEASON::TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end(); j++ ) {
      NewTextChild( variablesNode, "trip", j->name );
      break;
    }
  }
}

void populate_doc_cap(xmlNodePtr variablesNode, string lang)
{
    NewTextChild(variablesNode, "doc_cap_no", getLocaleText("№", lang));
    NewTextChild(variablesNode, "doc_cap_name", getLocaleText("Ф.И.О.", lang));
    NewTextChild(variablesNode, "doc_cap_surname", getLocaleText("Фамилия", lang));
    NewTextChild(variablesNode, "doc_cap_doc", getLocaleText("Документ", lang));
    NewTextChild(variablesNode, "doc_cap_tkt", getLocaleText("Билет", lang));
    NewTextChild(variablesNode, "doc_cap_tkt_no", getLocaleText("№ Билета", lang));
    NewTextChild(variablesNode, "doc_cap_ref_type", getLocaleText("Причина невылета", lang));
    NewTextChild(variablesNode, "doc_cap_bag", getLocaleText("Баг.", lang));
    NewTextChild(variablesNode, "doc_cap_baggage", getLocaleText("Багаж", lang));
    NewTextChild(variablesNode, "doc_cap_rk", getLocaleText("Р/к", lang));
    NewTextChild(variablesNode, "doc_cap_pay", getLocaleText("Опл.", lang));
    NewTextChild(variablesNode, "doc_cap_tags", getLocaleText("№№ баг. бирок", lang));
    NewTextChild(variablesNode, "doc_cap_tags_short", getLocaleText("№ б/б", lang));
    NewTextChild(variablesNode, "doc_cap_rem", getLocaleText("Рем.", lang));
    NewTextChild(variablesNode, "doc_cap_pas", getLocaleText("Пас", lang));
    NewTextChild(variablesNode, "doc_cap_type", getLocaleText("Тип", lang));
    NewTextChild(variablesNode, "doc_cap_seat_no", getLocaleText("№ м", lang));
    NewTextChild(variablesNode, "doc_cap_remarks", getLocaleText("Ремарки", lang));
    NewTextChild(variablesNode, "doc_cap_cl", getLocaleText("Кл", lang));
    NewTextChild(variablesNode, "doc_cap_dest", getLocaleText("П/н", lang));
    NewTextChild(variablesNode, "doc_cap_to", getLocaleText("CAP.DOC.TO", lang));
    NewTextChild(variablesNode, "doc_cap_ex", getLocaleText("Дс", lang));
    NewTextChild(variablesNode, "doc_cap_brd", getLocaleText("Пс", lang));
    NewTextChild(variablesNode, "doc_cap_test", (bad_client_img_version() and not get_test_server()) ? " " : getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "doc_cap_user_descr", getLocaleText("Оператор", lang));
}

void PaxListVars(int point_id, TRptParams &rpt_params, xmlNodePtr variablesNode, TDateTime part_key)
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
        "   scd_out "
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
    if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

    string airp = Qry.FieldAsString("airp");
    string airline = Qry.FieldAsString("airline");
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    string airline_name;
    if(airline.size()) {
        airline = rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative);
        airline_name = rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong);
    }

    ostringstream trip;
    trip
        << airline
        << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no")
        << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("suffix"), efmtCodeNative);

    NewTextChild(variablesNode, "trip", trip.str());
    TDateTime scd_out, real_out;
    scd_out= UTCToClient(Qry.FieldAsDateTime("scd_out"),tz_region);
    real_out= UTCToClient(Qry.FieldAsDateTime("real_out"),tz_region);
    NewTextChild(variablesNode, "scd_out", DateTimeToStr(scd_out, "dd.mm.yyyy"));
    NewTextChild(variablesNode, "real_out", DateTimeToStr(real_out, "dd.mm.yyyy"));
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm"));
    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn"));
    NewTextChild(variablesNode, "day_issue", DateTimeToStr(issued, "dd.mm.yy"));


    NewTextChild(variablesNode, "lang", TReqInfo::Instance()->desk.lang );
    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",airp);
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airp_dep_city", rpt_params.ElemIdToReportElem(etCity, airpRow.city, efmtCodeNative));
    NewTextChild(variablesNode, "airline_name", airline_name);
    NewTextChild(variablesNode, "flt", trip.str());
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", craft);
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn"));
    NewTextChild(variablesNode, "long_route", GetRouteAfterStr(part_key,
                                                               point_id,
                                                               trtWithCurrent,
                                                               trtNotCancelled,
                                                               rpt_params.GetLang(),
                                                               true));
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT", rpt_params.GetLang()));
}

void TRptParams::Init(xmlNodePtr node)
{
    prn_params.get_prn_params(node->parent);
    point_id = NodeAsIntegerFast("point_id", node);
    rpt_type = DecodeRptType(NodeAsStringFast("rpt_type", node));
    airp_arv = NodeAsStringFast("airp_arv", node, "");
    ckin_zone = NodeAsStringFast("ckin_zone", node, ALL_CKIN_ZONES.c_str());
    pr_et = NodeAsIntegerFast("pr_et", node, 0) != 0;
    text = NodeAsIntegerFast("text", node, NoExists);
    pr_trfer = NodeAsIntegerFast("pr_trfer", node, 0) != 0;
    pr_brd = NodeAsIntegerFast("pr_brd", node, 0) != 0;
    sort = (TSortType)NodeAsIntegerFast("sort", node, 0);
    if(text != NoExists and text != 0 and rpt_type != rtBDOCS) // т.к. у отчета BDOCS нет текстового варианта
        rpt_type = TRptType((int)rpt_type + 1);
    string route_country;
    route_inter = IsRouteInter(point_id, NoExists, route_country);
    if(route_country == "РФ")
        route_country_lang = AstraLocale::LANG_RU;
    else
        route_country_lang = "";
    xmlNodePtr mktFltNode = GetNodeFast("mkt_flight", node);
    if(mktFltNode != NULL) {
        xmlNodePtr node = mktFltNode->children;
        mkt_flt.airline = NodeAsStringFast("airline", node);
        mkt_flt.flt_no = NodeAsIntegerFast("flt_no", node);
        mkt_flt.suffix = NodeAsStringFast("suffix", node, "");
    }
    xmlNodePtr clientTypeNode = GetNodeFast("client_type", node);
    if(clientTypeNode != NULL)
        client_type = NodeAsString(clientTypeNode);
    if(prn_params.pr_lat)
        req_lang = AstraLocale::LANG_EN;
    else if(
            rpt_type == rtPTM or
            rpt_type == rtPTMTXT or
            rpt_type == rtBTM or
            rpt_type == rtBTMTXT
           )
        req_lang = "";
    else
        req_lang = TReqInfo::Instance()->desk.lang;
    xmlNodePtr remsNode = GetNodeFast("rems", node);
    if(remsNode != NULL) {
        xmlNodePtr currNode = remsNode->children;
        for(; currNode; currNode = currNode->next)
        {
            TRemCategory cat=getRemCategory(NodeAsString(currNode), "");
            rems[cat].push_back(NodeAsString(currNode));
        };
    }
}

bool TRptParams::IsInter() const
{
    return req_lang == AstraLocale::LANG_EN || route_inter || route_country_lang.empty() || route_country_lang!=TReqInfo::Instance()->desk.lang;
}

string TRptParams::GetLang() const
{
  string lang = req_lang;
  if (lang.empty())
  {
    lang = TReqInfo::Instance()->desk.lang;
    if (IsInter()) lang=AstraLocale::LANG_EN;
  }
  return lang;
}

string TRptParams::GetLang(TElemFmt &fmt, string firm_lang) const
{
  string lang = firm_lang.empty()? req_lang : firm_lang;
  if (lang.empty())
  {
     lang = TReqInfo::Instance()->desk.lang;
     if (IsInter())
     {
       if (fmt==efmtNameShort || fmt==efmtNameLong) lang=AstraLocale::LANG_EN;
       if (fmt==efmtCodeNative) fmt=efmtCodeInter;
       if (fmt==efmtCodeICAONative) fmt=efmtCodeICAOInter;
     };
  };
  return lang;
}

string TRptParams::ElemIdToReportElem(TElemType type, const string &id, TElemFmt fmt, string firm_lang) const
{
  if (id.empty()) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};

string TRptParams::ElemIdToReportElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
{
  if (id==ASTRA::NoExists) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};


string get_last_target(TQuery &Qry, TRptParams &rpt_params)
{
    string result;
    string airline = Qry.FieldAsString("trfer_airline");
    if(!airline.empty()) {
        ostringstream buf;
        buf
            << rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("trfer_airp_arv"), efmtNameLong).substr(0, 50)
            << "("
            << rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative)
            << setw(3) << setfill('0') << Qry.FieldAsInteger("trfer_flt_no")
            << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("trfer_suffix"), efmtCodeNative)
            << ")/" << DateTimeToStr(Qry.FieldAsDateTime("trfer_scd"), "dd");
        result = buf.str();
    }
    return result;
}

string get_hall_list(string airp, TRptParams &rpt_params)
{
    TQuery Qry(&OraSession);
    string SQLText = (string)
        "SELECT id FROM halls2 WHERE "
        "   airp = :airp AND "
        "   NVL(rpt_grp, ' ') = NVL(:rpt_grp, ' ') "
        "ORDER BY "
        "   name ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("rpt_grp", otString, rpt_params.ckin_zone);
    Qry.Execute();
    string result;
    for(; !Qry.Eof; Qry.Next()) {
        if(!result.empty())
            result += ", ";
        result += rpt_params.ElemIdToReportElem(etHall, Qry.FieldAsInteger("id"), efmtNameLong);
    }
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
    int to_ramp;
    string to_ramp_str;
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
        to_ramp = -1;
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
                    if(p1.to_ramp == p2.to_ramp) {
                        if(p1.bag_name_priority == p2.bag_name_priority) {
                            if(p1.tag_type == p2.tag_type) {
                                result = p1.color < p2.color;
                            } else
                                result = p1.tag_type > p2.tag_type;
                        } else
                            result = p1.bag_name_priority < p2.bag_name_priority;
                    } else
                        result = p1.to_ramp < p2.to_ramp;
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
        void get(string class_code, TBagTagRow &bag_tag_row, TRptParams &rpt_params);
};

const int TO_RAMP_PRIORITY = 1000;

void t_rpt_bm_bag_name::get(string class_code, TBagTagRow &bag_tag_row, TRptParams &rpt_params)
{
    string &result = bag_tag_row.bag_name;
    for(vector<TBagNameRow>::iterator iv = bag_names.begin(); iv != bag_names.end(); iv++)
        if(iv->class_code == class_code && iv->bag_type == bag_tag_row.bag_type) {
            result = rpt_params.IsInter() ? iv->name_lat : iv->name;
            if(result.empty())
                result = iv->name;
            break;
        }
    if(not result.empty())
        bag_tag_row.bag_name_priority = bag_tag_row.bag_type;
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
            << setw(12) << "to_ramp_str"
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
        << setw(12)  << tag.to_ramp_str
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

string get_tag_rangeA(vector<t_tag_nos_row> &tag_nos, vector<t_tag_nos_row>::iterator begin, vector<t_tag_nos_row>::iterator end, string lang)
{
    string lim = getLocaleText("(огр)", lang);
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

string get_tag_range(vector<t_tag_nos_row> tag_nos, string lang)
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
                result << get_tag_rangeA(tag_nos, begin, iv, lang);
            }
            begin = iv;
        }
    }
    if(!result.str().empty())
        result << ", ";
    result << get_tag_rangeA(tag_nos, begin, tag_nos.end(), lang);
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

void trip_rpt_person(xmlNodePtr resNode, TRptParams &rpt_params)
{
    xmlNodePtr variablesNode = STAT::getVariablesNode(resNode);
    TQuery Qry(&OraSession);
    Qry.SQLText = "select * from trip_rpt_person where point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    string loader, pts_agent;
    if(not Qry.Eof) {
        loader = Qry.FieldAsString("loader");
        pts_agent = Qry.FieldAsString("pts_agent");
    }
    NewTextChild(variablesNode, "loader", transliter(loader, 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
    NewTextChild(variablesNode, "pts_agent", transliter(pts_agent, 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
}

void PTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
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
    get_compatible_report_form(rpt_name, reqNode, resNode);

    {
        string et, et_lat;
        if(rpt_params.pr_et) {
            et = getLocaleText("(ЭБ)", rpt_params.dup_lang());
            et_lat = getLocaleText("(ЭБ)", AstraLocale::LANG_EN);
        }
        NewTextChild(variablesNode, "ptm", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et), rpt_params.dup_lang()));
        NewTextChild(variablesNode, "ptm_lat", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et_lat), AstraLocale::LANG_EN));
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
        "   pax_grp.class_grp, \n"
        "   DECODE(pax_grp.status, 'T', pax_grp.status, 'N') status, \n"
        "   surname||' '||pax.name AS full_name, \n"
        "   pax.pers_type, \n"
        "   pax.is_female, \n"
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, \n"
        "   pax.seats, \n";
    if(rpt_params.pr_et) { //ЭБ
        SQLText +=
            "    ticket_no||'/'||coupon_no AS remarks, \n";
    };
    SQLText +=
        "   NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS rk_weight, \n"
        "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) AS excess, \n"
        "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, \n"
        "   reg_no, \n"
        "   pax_grp.grp_id \n"
        "FROM  \n"
        "   pax_grp, \n"
        "   points, \n"
        "   pax, \n"
        "   cls_grp, \n"
        "   halls2 \n";
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips \n";
    SQLText +=
        "WHERE \n"
        "   points.pr_del>=0 AND \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.point_arv = points.point_id and \n"
        "   pax_grp.grp_id=pax.grp_id AND \n"
        "   pax_grp.class_grp is not null AND \n"
        "   pax_grp.class_grp = cls_grp.id and \n"
        "   pax_grp.hall = halls2.id(+) and \n"
        "   pax_grp.status NOT IN ('E') and \n"
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
        "    cls_grp.priority, \n"
        "    cls_grp.class, \n";
    switch(rpt_params.sort) {
        case stRegNo:
            SQLText +=
                "    pax.reg_no ASC, \n"
                "    pax.seats DESC \n";
            break;
        case stSurname:
            SQLText +=
                "    full_name ASC, \n"
                "    pax.reg_no ASC, \n"
                "    pax.seats DESC \n";
            break;
        case stSeatNo:
            SQLText +=
                "    seat_no ASC, \n"
                "    pax.reg_no ASC, \n"
                "    pax.seats DESC \n";
            break;
    }
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer");
    // следующие 2 переменные введены для нужд FastReport
    map<string, int> fr_target_ref;
    int fr_target_ref_idx = 0;

    TPMTotals PMTotals;
    TRemGrp rem_grp;
    bool rem_grp_loaded = false;
    for(; !Qry.Eof; Qry.Next()) {
        int pax_id = Qry.FieldAsInteger("pax_id");
        if(not rpt_params.mkt_flt.empty()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(pax_id);
            if(mkt_flt.empty() or not(mkt_flt == rpt_params.mkt_flt))
                continue;
        }

        TPMTotalsKey key;
        key.point_id = Qry.FieldAsInteger("trip_id");
        key.target = Qry.FieldAsString("target");
        key.status = Qry.FieldAsString("status");
        int class_grp = Qry.FieldAsInteger("class_grp");
        key.cls = rpt_params.ElemIdToReportElem(etClsGrp, class_grp, efmtCodeNative);
        key.cls_name = rpt_params.ElemIdToReportElem(etClsGrp, class_grp, efmtNameLong);
        key.lvl = ((TClsGrpRow&)base_tables.get("cls_grp").get_row( "id", class_grp, true)).priority;
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


        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", Qry.FieldAsString("reg_no"));
        NewTextChild(rowNode, "full_name", transliter(Qry.FieldAsString("full_name"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        string last_target;
        int pr_trfer = 0;
        if(rpt_params.pr_trfer) {
            last_target = get_last_target(Qry, rpt_params);
            pr_trfer = Qry.FieldAsInteger("pr_trfer");
        }
        NewTextChild(rowNode, "last_target", last_target);
        NewTextChild(rowNode, "pr_trfer", pr_trfer);

        if(fr_target_ref.find(key.target) == fr_target_ref.end())
            fr_target_ref[key.target] = fr_target_ref_idx++;
        NewTextChild(rowNode, "airp_arv", key.target);
        NewTextChild(rowNode, "fr_target_ref", fr_target_ref[key.target]);
        NewTextChild(rowNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, key.target, efmtNameLong));
        NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger("grp_id"));
        NewTextChild(rowNode, "class_name", key.cls_name);
        NewTextChild(rowNode, "class", key.cls);
        NewTextChild(rowNode, "seats", Qry.FieldAsInteger("seats"));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        {
            TPerson pers_type = DecodePerson(Qry.FieldAsString("pers_type"));
            string gender;
            switch(pers_type) {
                case adult:
                    if(not Qry.FieldIsNULL("is_female") and Qry.FieldAsInteger("is_female") != 0)
                        gender = "F";
                    else
                        gender = "M";
                    NewTextChild(rowNode, "pers_type", "ADL");
                    break;
                case child:
                    NewTextChild(rowNode, "pers_type", "CHD");
                    break;
                case baby:
                    NewTextChild(rowNode, "pers_type", "INF");
                    break;
                default:
                    throw Exception("DecodePerson failed");
            }
            NewTextChild(rowNode, "gender", gender);
        }
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        if(not rem_grp_loaded) {
            rem_grp_loaded = true;
            rem_grp.Load(retRPT_PM, rpt_params.point_id);
        }
        NewTextChild(rowNode, "remarks",
                (rpt_params.pr_et ? Qry.FieldAsString("remarks") : GetRemarkStr(rem_grp, pax_id)));
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

    TElemFmt airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)Qry.FieldAsInteger("craft_fmt");

    string airp = Qry.FieldAsString("airp");
    string airline, suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.empty()) {
        airline = Qry.FieldAsString("airline");
        flt_no = Qry.FieldAsInteger("flt_no");
        suffix = Qry.FieldAsString("suffix");
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
        airline_fmt = efmtCodeNative;
        suffix_fmt = efmtCodeNative;
        craft_fmt = efmtCodeNative;
    }
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(airp);

    //    TCrafts crafts;

    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airline_name", rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong));

    ostringstream flt;
    flt
        << rpt_params.ElemIdToReportElem(etAirline, airline, airline_fmt)
        << setw(3) << setfill('0') << flt_no
        << rpt_params.ElemIdToReportElem(etSuffix, suffix, suffix_fmt);
    NewTextChild(variablesNode, "flt", flt.str());
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", rpt_params.IsInter()));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", rpt_params.IsInter()));
    NewTextChild(variablesNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, rpt_params.airp_arv, efmtNameLong));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", rpt_params.IsInter()));

    NewTextChild(variablesNode, "pr_vip", 2);
    string pr_brd_pax_str = (string)"ПАССАЖИРЫ " + (rpt_params.pr_brd ? "(посаж)" : "(зарег)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "pr_group", rpt_params.sort == stRegNo); // Если сортировка по рег. но., то выделяем группы пассажиров в fr-отчете
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);
}

void BTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
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
    get_compatible_report_form(rpt_name, reqNode, resNode);

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
        "    pax_grp.class, "
        "    bag2.bag_type, "
        "    bag2.num bag_num, "
        "    bag2.amount, "
        "    bag2.weight, "
        "    bag2.pr_liab_limit, "
        "    bag2.to_ramp "
        "from "
        "    pax_grp, "
        "    points, "
        "    bag2, "
        "    halls2, "
        "    pax ";
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips ";
    SQLText +=
        "where "
        "    points.pr_del>=0 AND "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.point_arv = points.point_id and "
        "    pax_grp.grp_id = bag2.grp_id and "
        "    pax_grp.status NOT IN ('E') and ";
        
    if (rpt_params.pr_brd)
      SQLText +=
          "    ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0 and ";
    else
      SQLText +=
          "    ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and ";
    SQLText +=
        "    bag2.pr_cabin = 0 and "
        "    bag2.hall = halls2.id(+) and "
        "    ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) = pax.pax_id(+) ";
    if(!rpt_params.airp_arv.empty()) {
        SQLText += " and pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and bag2.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    if(rpt_params.pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";

    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.Execute();
    set<int> grps;
    for(; !Qry.Eof; Qry.Next()) {
        if(not rpt_params.mkt_flt.empty()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(Qry.FieldAsInteger("pax_id"));
            if(mkt_flt.empty() or not(mkt_flt == rpt_params.mkt_flt))
                continue;
        }

        int cur_grp_id = Qry.FieldAsInteger("grp_id");
        int cur_bag_num = Qry.FieldAsInteger("bag_num");

        TBagTagRow bag_tag_row;
        bag_tag_row.pr_trfer = Qry.FieldAsInteger("pr_trfer");
        bag_tag_row.last_target = get_last_target(Qry, rpt_params);
        bag_tag_row.point_num = Qry.FieldAsInteger("point_num");
        bag_tag_row.grp_id = cur_grp_id;
        bag_tag_row.airp_arv = Qry.FieldAsString("airp_arv");
        bag_tag_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_tag_row.to_ramp = Qry.FieldAsInteger("to_ramp");
        if(bag_tag_row.to_ramp)
            bag_tag_row.to_ramp_str = getLocaleText("У ТРАПА", rpt_params.GetLang());

        string class_code = Qry.FieldAsString("class");
        bag_names.get(class_code, bag_tag_row, rpt_params);
        if(Qry.FieldIsNULL("class"))
            bag_tag_row.class_priority = 100;
        else {
            bag_tag_row.class_priority = ((TClassesRow&)base_tables.get("classes").get_row( "code", class_code)).priority;
            bag_tag_row.class_code = rpt_params.ElemIdToReportElem(etClass, class_code, efmtCodeNative);
            bag_tag_row.class_name = rpt_params.ElemIdToReportElem(etClass, class_code, efmtNameLong);
        }
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
        if(tagsQry.Eof) {
            bag_tags.push_back(bag_tag_row);
            bag_tags.back().bag_name_priority = -1;
            bag_tags.back().bag_name.clear();
        } else
            for(; !tagsQry.Eof; tagsQry.Next()) {
                bag_tags.push_back(bag_tag_row);
                bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
                bag_tags.back().color = tagsQry.FieldAsString("color");
                bag_tags.back().no = tagsQry.FieldAsFloat("no");
            }

        if(grps.find(cur_grp_id) == grps.end()) {
            grps.insert(cur_grp_id);
            // ищем непривязанные бирки для каждой группы
            TQuery tagsQry(&OraSession);
            string SQLText =
                "select "
                "   bag_tags.tag_type, "
                "   bag_tags.color, "
                "   to_char(bag_tags.no) no "
                "from ";
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES)
                SQLText += " halls2, pax_grp, ";
            SQLText +=
                "   bag_tags "
                "where "
                "   bag_tags.grp_id = :grp_id and "
                "   bag_tags.bag_num is null";
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
                SQLText +=
                    "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and pax_grp.hall IS NOT NULL and "
                    "   pax_grp.grp_id = bag_tags.grp_id and "
                    "   pax_grp.hall = halls2.id(+) ";
                tagsQry.CreateVariable("zone", otString, rpt_params.ckin_zone);
            }
            tagsQry.SQLText = SQLText;
            tagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
            tagsQry.Execute();
            for(; !tagsQry.Eof; tagsQry.Next()) {
                bag_tags.push_back(bag_tag_row);
                bag_tags.back().bag_num = -1;
                bag_tags.back().amount = 0;
                bag_tags.back().weight = 0;
                bag_tags.back().bag_name_priority = -1;
                bag_tags.back().bag_name = "";
                bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
                bag_tags.back().color = tagsQry.FieldAsString("color");
                bag_tags.back().no = tagsQry.FieldAsFloat("no");
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
                    bag_tag_row.to_ramp == iv->to_ramp &&
                    bag_tag_row.bag_name == iv->bag_name &&
                    bag_tag_row.tag_type == iv->tag_type &&
                    bag_tag_row.color == iv->color
                 )
          ) {
            if(!tag_nos.empty()) {
                bm_table.back().tag_range = get_tag_range(tag_nos, rpt_params.GetLang());
                bm_table.back().num = tag_nos.size();
                tag_nos.clear();
            }
            bag_tag_row.last_target = iv->last_target;
            bag_tag_row.airp_arv = iv->airp_arv;
            bag_tag_row.class_code = iv->class_code;
            bag_tag_row.to_ramp = iv->to_ramp;
            bag_tag_row.bag_name = iv->bag_name;
            bag_tag_row.tag_type = iv->tag_type;
            bag_tag_row.color = iv->color;
            bm_table.push_back(*iv);
            if(iv->class_code.empty())
                bm_table.back().class_name = getLocaleText("Несопровождаемый багаж", rpt_params.GetLang());
            if(iv->color.empty())
                bm_table.back().color = "-";
            else
                bm_table.back().color = rpt_params.ElemIdToReportElem(etTagColor, iv->color, efmtNameLong);
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
                bag_sum_row.last_target = iv->last_target;
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
        bm_table.back().tag_range = get_tag_range(tag_nos, rpt_params.GetLang());
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
        if(iv->tag_type.empty()) continue;
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        string airp_arv = iv->airp_arv;
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, airp_arv, efmtNameLong));
        NewTextChild(rowNode, "pr_trfer", iv->pr_trfer);
        NewTextChild(rowNode, "last_target", iv->last_target);
        NewTextChild(rowNode, "to_ramp", iv->to_ramp_str);
        NewTextChild(rowNode, "bag_name", iv->bag_name);
        NewTextChild(rowNode, "birk_range", iv->tag_range);
        NewTextChild(rowNode, "color", iv->color);
        NewTextChild(rowNode, "num", iv->num);
        NewTextChild(rowNode, "pr_vip", 2);

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
    NewTextChild(variablesNode, "Tot", vs_number(TotAmount, rpt_params.IsInter()));
    Qry.Clear();
    SQLText =
        "select "
        "  sum(bag2.amount) amount, "
        "  sum(bag2.weight) weight "
        "from "
        "  pax_grp, "
        "  bag2, "
        "  transfer ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES)
        SQLText += ", halls2 ";
    SQLText +=
        "where "
        "  pax_grp.point_dep = :point_id and "
        "  pax_grp.grp_id = bag2.grp_id and "
        "  pax_grp.grp_id = transfer.grp_id and "
        "  pax_grp.status NOT IN ('E') and "
        "  ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and "
        "  bag2.pr_cabin = 0 and "
        "  transfer.pr_final <> 0 ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and bag2.hall = halls2.id(+) "
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and bag2.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    Qry.SQLText = SQLText;
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

    TElemFmt airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)Qry.FieldAsInteger("craft_fmt");

    string airline, suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.empty()) {
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

    //    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airline_name", rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong));
    ostringstream flt;
    flt
        << rpt_params.ElemIdToReportElem(etAirline, airline, airline_fmt)
        << setw(3) << setfill('0') << flt_no
        << rpt_params.ElemIdToReportElem(etSuffix, suffix, suffix_fmt);
    NewTextChild(variablesNode, "flt", flt.str());
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", rpt_params.IsInter()));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", rpt_params.IsInter()));
    string airp_arv_name;
    if(rpt_params.airp_arv.size())
        airp_arv_name = rpt_params.ElemIdToReportElem(etAirp, rpt_params.airp_arv, efmtNameLong);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    {
        // delete in future 14.10.07 !!!
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", rpt_params.IsInter()));
    string pr_brd_pax_str = (string)"Номера багажных бирок " + (rpt_params.pr_brd ? "(посаж)" : "(зарег)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);
}

string get_test_str(int page_width, string lang)
{
    string result;
    for(int i=0;i<page_width/6;i++) result += " " + ( bad_client_img_version() and not get_test_server() ? " " : getLocaleText("CAP.TEST", lang)) + " ";
    return result;
}


void PTMBTMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (rpt_params.rpt_type==rtPTMTXT)
    PTM(rpt_params, reqNode, resNode);
  else
    BTM(rpt_params, reqNode, resNode);

  xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
  xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);

  string str;
  ostringstream s;
  //текстовый формат
  int page_width=80;
  //специально вводим для кириллических символов, так как в терминале при экспорте проблемы
  //максимальная длина строки при экспорте в байтах! не должна превышать ~147 (65 рус + 15 лат)
  int max_symb_count= rpt_params.IsInter() ? page_width : 65;
  NewTextChild(variablesNode, "page_width", page_width);
  NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
  if(bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");

  s.str("");
  s << get_test_str(page_width, rpt_params.GetLang());
  NewTextChild(variablesNode, "test_str", s.str());


  s.str("");
  if (rpt_params.rpt_type==rtPTMTXT)
  {
      ProgTrace(TRACE5, "'%s'", NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode)); //!!!
      str.assign(NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode));
  }
  else
    str.assign(getLocaleText("БАГАЖНАЯ ВЕДОМОСТЬ", rpt_params.GetLang()));

  s << setfill(' ')
    << str
    << right << setw(page_width-str.size())
    << string(NodeAsString((rpt_params.IsInter() ? "own_airp_name_lat" : "own_airp_name"),variablesNode)).substr(0,max_symb_count-str.size());
  NewTextChild(variablesNode, "page_header_top", s.str());


  s.str("");
  str.assign(getLocaleText("Владелец или Оператор: ", rpt_params.GetLang()));
  s << left
    << str
    << string(NodeAsString("airline_name",variablesNode)).substr(0,max_symb_count-str.size()) << endl
    << setw(10) << getLocaleText("№ рейса", rpt_params.GetLang());
  if (rpt_params.IsInter())
    s << setw(19) << "Aircraft";
  else
    s << setw(9)  << "№ ВС"
      << setw(10) << "ТипВС Ст. ";

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(20) << getLocaleText("А/п вылета", rpt_params.GetLang())
      << setw(20) << getLocaleText("А/п назначения", rpt_params.GetLang());
  else
    s << setw(40) << getLocaleText("А/п вылета", rpt_params.GetLang());
  s << setw(6)  << getLocaleText("Дата", rpt_params.GetLang())
    << setw(5)  << getLocaleText("Время", rpt_params.GetLang()) << endl;

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
  str.assign(NodeAsString((rpt_params.IsInter()?"pr_brd_pax_lat":"pr_brd_pax"),variablesNode));
  if (!NodeIsNULL("zone",variablesNode))
  {
    unsigned int zone_len=max_symb_count-str.size()-1;
    string zone;
    zone.assign(getLocaleText("CAP.DOC.ZONE", rpt_params.GetLang()) + ": ");
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
      << setw(4)  << (getLocaleText("CAP.DOC.REG", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?18:19) << (getLocaleText("Фамилия", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("Пол", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?4:3)   << (getLocaleText("Кл", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("CAP.DOC.SEAT_NO", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("CAP.DOC.BAG", rpt_params.GetLang()))
      << setw(6)  << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(15) << (getLocaleText("CAP.DOC.BAG_TAG_NOS", rpt_params.GetLang()))
      << setw(9)  << (getLocaleText("Ремарки", rpt_params.GetLang()));
  else
    s << left
      << setw(29) << (getLocaleText("Номера багажных бирок", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("Цвет", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(8)  << (getLocaleText("№ Конт.", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("CAP.DOC.HOLD", rpt_params.GetLang()))
      << setw(11) << (getLocaleText("Отсек", rpt_params.GetLang()));

  NewTextChild(variablesNode, "page_header_bottom", s.str() );

  if (rpt_params.rpt_type==rtPTMTXT)
  {
    s.str("");
    s << setw(21) << (getLocaleText("Всего в классе", rpt_params.GetLang())) << "M/F" << endl
      << "%-17s%7s      %4u %3u %3u %2u/%-4u%4u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << setw(56) << (getLocaleText("Всего", rpt_params.GetLang()))
        << (getLocaleText("Подпись", rpt_params.GetLang())) << endl;

    s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()))
      << setw(24) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 24) << endl
      << "%-6u %-7s %-6u %-6u %-6u %-6u %-6u %-6u" << endl
      << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));

    NewTextChild(variablesNode, "page_footer_top", s.str() );


    xmlNodePtr dataSetNode = NodeAsNode("v_pm_trfer", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    int fem = 0; int male = 0;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //рабиваем фамилию, бирки, ремарки
      SeparateString(NodeAsString("full_name",rowNode),18,rows);
      fields["full_name"]=rows;
      SeparateString(NodeAsString("tags",rowNode),15,rows);
      fields["tags"]=rows;
      SeparateString(NodeAsString("remarks",rowNode),9,rows);
      fields["remarks"]=rows;

      string gender = NodeAsString("gender",rowNode);
      if(gender == "M") male++;
      if(gender == "F") fem++;

      row=0;
      string pers_type=NodeAsString("pers_type",rowNode);
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << right << setw(3) << (row==0?NodeAsString("reg_no",rowNode):"") << " "
          << left << setw(18) << (!fields["full_name"].empty()?*(fields["full_name"].begin()):"") << " "
          << left <<  setw(4) << (row==0?gender:"")
          << left <<  setw(3) << (row==0?NodeAsString("class",rowNode):"")
          << right << setw(4) << (row==0?NodeAsString("seat_no",rowNode):"") << " "
          << left <<  setw(4) << (row==0&&pers_type=="CHD"?" X ":"")
          << left <<  setw(4) << (row==0&&pers_type=="INF"?" X ":"");
        if (row!=0 ||
            (NodeAsInteger("bag_amount",rowNode)==0 &&
            NodeAsInteger("bag_weight",rowNode)==0))
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
        s << setw(20) << (getLocaleText("Всего багажа", rpt_params.GetLang()));
      else
      {
        if (k==0)
          s << setw(24) << (getLocaleText("Всего нетрансф. багажа", rpt_params.GetLang()));
        else
          s << setw(24) << (getLocaleText("Всего трансф. багажа", rpt_params.GetLang()));
      };

      s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
        << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()));

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
      ostringstream adl_fem;
      adl_fem << NodeAsInteger("adl", rowNode) << '/' << fem;
      s.str("");
      s << setw(rpt_params.pr_trfer?24:20) << NodeAsString("class_name",rowNode)
        << setw(7) << NodeAsInteger("seats",rowNode)
        << setw(8) << adl_fem.str()
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
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //разбиваем диапазоны бирок, цвет
      int offset = 2;
      if(not NodeIsNULL("bag_name", rowNode))
          offset += 2;
      if(not NodeIsNULL("to_ramp", rowNode))
          offset += 2;

      SeparateString(NodeAsString("birk_range",rowNode),28 - offset,rows);
      fields["birk_range"]=rows;
      SeparateString(NodeAsString("color",rowNode),9,rows);
      fields["color"]=rows;

      row=0;
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << setw(offset) << "" //отступ
          << left << setw(28 - offset) << (!fields["birk_range"].empty()?*(fields["birk_range"].begin()):"") << " "
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
          s << setw(39) << (getLocaleText("Всего багажа, исключая трансферный", rpt_params.GetLang()));
        else
          s << setw(39) << (getLocaleText("Всего трансферного багажа", rpt_params.GetLang()));
        s << "%4u %6u";
        if (k==0)
          NewTextChild(variablesNode, "total_not_trfer_fmt", s.str() );
        else
          NewTextChild(variablesNode, "total_trfer_fmt", s.str() );
      };

      s.str("");
      s << setw(39) << (getLocaleText("Всего багажа", rpt_params.GetLang()))
        << "%4u %6u" << endl
        << setw(39) << (getLocaleText("Трансферного багажа", rpt_params.GetLang()))
        << "%4u %6u";
      NewTextChild(variablesNode, "report_footer", s.str() );
    };

    s.str("");
    s << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    NewTextChild(variablesNode, "page_footer_top", s.str() );



    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << setw(56) << (getLocaleText("Всего", rpt_params.GetLang()))
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;

    s << setw(6)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(43) << (getLocaleText("Количество мест прописью", rpt_params.GetLang()))
      << setw(24) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 24) << endl;

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
};

string get_flight(xmlNodePtr variablesNode)
{
    return (string)NodeAsString("trip", variablesNode) + "/" + NodeAsString("scd_out", variablesNode);
}

void REFUSE(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREFUSETXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("ref", reqNode, resNode);
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       surname||' '||pax.name family, "
        "       pax.pers_type, "
        "       ticket_no, "
        "       refusal_types.code refuse, "
        "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags "
        "FROM   pax_grp,pax,refusal_types "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.refuse = refusal_types.code AND "
        "       pax_grp.status NOT IN ('E') AND "
        "       pax.refuse IS NOT NULL and "
        "       point_dep = :point_id "
        "order by ";
    switch(rpt_params.sort) {
        case stSeatNo:
        case stRegNo:
            SQLText += " pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            SQLText += " family, pax.reg_no, pax.seats DESC ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_ref");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
        NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
        NewTextChild(rowNode, "refuse", rpt_params.ElemIdToReportElem(etRefusalType, Qry.FieldAsString("refuse"), efmtNameLong));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

        Qry.Next();
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.REFUSE", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())); //!!!param 100%error
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void REFUSETXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    REFUSE(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(21) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Тип", rpt_params.GetLang())
        << setw(10) << getLocaleText("№ Билета", rpt_params.GetLang())
        << setw(24)  << getLocaleText("Причина невылета", rpt_params.GetLang())
        << setw(16) << getLocaleText("№ б/б", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
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

void NOTPRES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtNOTPRESTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("notpres", reqNode, resNode);
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT point_dep AS point_id, "
        "       reg_no, "
        "       surname||' '||pax.name family, "
        "       pax.pers_type, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, "
        "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bagAmount, "
        "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags "
        "FROM   pax_grp,pax "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pr_brd=0 and "
        "       point_dep = :point_id and "
        "       pax_grp.status NOT IN ('E') "
        "order by ";
    switch(rpt_params.sort) {
        case stRegNo:
            SQLText += " pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            SQLText += " family, pax.reg_no, pax.seats DESC ";
            break;
        case stSeatNo:
            SQLText += " seat_no, pax.reg_no, pax.seats DESC ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_notpres");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
        NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(rowNode, "bagamount", Qry.FieldAsInteger("bagamount"));
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

        Qry.Next();
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.NOTPRES",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void NOTPRESTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NOTPRES(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(38) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Тип", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(6) << getLocaleText("Баг.", rpt_params.GetLang())
        << " " << setw(19) << getLocaleText("№ б/б", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
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

bool getPaxRem(const TRptParams &rpt_params, const CheckIn::TPaxTknItem &tkn, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (tkn.empty() || tkn.rem.empty()) return false;
  rem.clear();
  rem.code=tkn.rem;
  ostringstream text;
  text << rem.code << " HK1 " << (inf_indicator?"INF":"") << tkn.no;
  if (tkn.coupon!=ASTRA::NoExists)
    text << "/" << tkn.coupon;
  rem.text=text.str();
  rem.calcPriority();
  return true;
};

bool getPaxRem(const TRptParams &rpt_params, const CheckIn::TPaxDocItem &doc, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doc.empty()) return false;
  rem.clear();
  rem.code="DOCS";
  bool pr_lat=rpt_params.GetLang() != AstraLocale::LANG_RU;
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocType, doc.type, efmtCodeNative)
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocCountry, doc.issue_country, efmtCodeNative)
       << "/" << doc.no
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocCountry, doc.nationality, efmtCodeNative)
       << "/" << (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "ddmmmyy", pr_lat):"")
       << "/" << rpt_params.ElemIdToReportElem(etGenderType, doc.gender, efmtCodeNative) << (inf_indicator?"I":"")
       << "/" << (doc.expiry_date!=ASTRA::NoExists?DateTimeToStr(doc.expiry_date, "ddmmmyy", pr_lat):"")
       << "/" << transliter(doc.surname, 1, pr_lat)
       << "/" << transliter(doc.first_name, 1, pr_lat)
       << "/" << transliter(doc.second_name, 1, pr_lat)
       << "/" << (doc.pr_multi?"H":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

bool getPaxRem(const TRptParams &rpt_params, const CheckIn::TPaxDocoItem &doco, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doco.empty()) return false;
  rem.clear();
  rem.code="DOCO";
  bool pr_lat=rpt_params.GetLang() != AstraLocale::LANG_RU;
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << transliter(doco.birth_place, 1, pr_lat)
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocType, doco.type, efmtCodeNative)
       << "/" << doco.no
       << "/" << transliter(doco.issue_place, 1, pr_lat)
       << "/" << (doco.issue_date!=ASTRA::NoExists?DateTimeToStr(doco.issue_date, "ddmmmyy", pr_lat):"")
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocCountry, doco.applic_country, efmtCodeNative)
       << "/" << (inf_indicator?"I":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

bool getPaxRem(const TRptParams &rpt_params, const CheckIn::TPaxDocaItem &doca, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (doca.empty()) return false;
  rem.clear();
  rem.code="DOCA";
  bool pr_lat=rpt_params.GetLang() != AstraLocale::LANG_RU;
  ostringstream text;
  text << rem.code
       << " " << "HK1"
       << "/" << doca.type
       << "/" << rpt_params.ElemIdToReportElem(etPaxDocCountry, doca.country, efmtCodeNative)
       << "/" << transliter(doca.address, 1, pr_lat)
       << "/" << transliter(doca.city, 1, pr_lat)
       << "/" << transliter(doca.region, 1, pr_lat)
       << "/" << transliter(doca.postal_code, 1, pr_lat)
       << "/" << (inf_indicator?"I":"");
  rem.text=text.str();
  for(int i=rem.text.size()-1;i>=0;i--)
    if (rem.text[i]!='/')
    {
      rem.text.erase(i+1);
      break;
    };
  rem.calcPriority();
  return true;
};

bool getPaxRem(const TRptParams &rpt_params, const CheckIn::TPaxFQTItem &fqt, CheckIn::TPaxRemItem &rem)
{
  if (fqt.empty()) return false;
  rem.clear();
  rem.code=fqt.rem;
  bool pr_lat=rpt_params.GetLang() != AstraLocale::LANG_RU;
  ostringstream text;
  text << rem.code
       << " " << rpt_params.ElemIdToReportElem(etAirline, fqt.airline, efmtCodeNative)
       << " " << fqt.no;
  if (!fqt.extra.empty())
    text << "/" << transliter(fqt.extra, 1, pr_lat);
  rem.text=text.str();
  rem.calcPriority();
  return true;
};

void REM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtREMTXT or rpt_params.rpt_type == rtSPECTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("rem", reqNode, resNode);

    TRemGrp spec_rems;
    string CAP_DOC = "CAP.DOC.REM";
    if(rpt_params.rpt_type == rtSPEC or rpt_params.rpt_type == rtSPECTXT) {
        spec_rems.Load(retRPT_SS, rpt_params.point_id);
        CAP_DOC = "CAP.DOC.SPEC";
    }

    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT pax_grp.point_dep AS point_id, "
        "       pax.pax_id, "
        "       pax.reg_no, "
        "       TRIM(pax.surname||' '||pax.name) AS family, "
        "       pax.pers_type, "
        "       pax.seats, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, "
        "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm "
        "FROM   pax_grp,pax "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pr_brd IS NOT NULL and "
        "       point_dep = :point_id and "
        "       pax_grp.status NOT IN ('E') "
        "order by ";
    switch(rpt_params.sort) {
        case stRegNo:
            SQLText += " pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            SQLText += " family, pax.reg_no, pax.seats DESC ";
            break;
        case stSeatNo:
            SQLText += " seat_no, pax.reg_no, pax.seats DESC ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
    Qry.Execute();

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_rem");
    for(; !Qry.Eof; Qry.Next())
    {
      CheckIn::TPaxTknItem tkn;
      CheckIn::TPaxDocItem doc;
      CheckIn::TPaxDocoItem doco;
      list<CheckIn::TPaxDocaItem> doca;
      vector<CheckIn::TPaxFQTItem> fqts;
      vector<CheckIn::TPaxRemItem> rems;
      map< TRemCategory, bool > cats;
      cats[remTKN]=false;
      cats[remDOC]=false;
      cats[remDOCO]=false;
      cats[remDOCA]=false;
      cats[remFQT]=false;
      cats[remUnknown]=false;
      int pax_id=Qry.FieldAsInteger("pax_id");
      bool inf_indicator=DecodePerson(Qry.FieldAsString("pers_type"))==ASTRA::baby && Qry.FieldAsInteger("seats")==0;
      if (!rpt_params.rems.empty())
      {
        bool pr_find=false;
        //фильтр по конкретным ремаркам
        map< TRemCategory, vector<string> >::const_iterator iRem=rpt_params.rems.begin();
        for(; iRem!=rpt_params.rems.end(); iRem++)
        {
          switch(iRem->first)
          {
            case remTKN:
              tkn.fromDB(Qry);
              if (!tkn.empty() && !tkn.rem.empty() &&
                  find(iRem->second.begin(),iRem->second.end(),tkn.rem)!=iRem->second.end())
                pr_find=true;
              cats[remTKN]=true;
              break;
            case remDOC:
              if (find(iRem->second.begin(),iRem->second.end(),"DOCS")!=iRem->second.end())
              {
                if (LoadPaxDoc(pax_id, doc)) pr_find=true;
                cats[remDOC]=true;
              };
              break;
            case remDOCO:
              if (find(iRem->second.begin(),iRem->second.end(),"DOCO")!=iRem->second.end())
              {
                if (LoadPaxDoco(pax_id, doco)) pr_find=true;
                cats[remDOCO]=true;
              };
              break;
            case remDOCA:
              if (find(iRem->second.begin(),iRem->second.end(),"DOCA")!=iRem->second.end())
              {
                if (LoadPaxDoca(pax_id, doca)) pr_find=true;
                cats[remDOCA]=true;
              };
              break;
            case remFQT:
              LoadPaxFQT(pax_id, fqts);
              for(vector<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();f++)
              {
                if (!f->rem.empty() &&
                    find(iRem->second.begin(),iRem->second.end(),f->rem)!=iRem->second.end())
                {
                  pr_find=true;
                  break;
                };
              };
              cats[remFQT]=true;
              break;
            default:
              LoadPaxRem(pax_id, false, rems);
              for(vector<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();r++)
              {
                if (!r->code.empty() &&
                    find(iRem->second.begin(),iRem->second.end(),r->code)!=iRem->second.end())
                {
                  pr_find=true;
                  break;
                };
              };
              cats[remUnknown]=true;
              break;
          };
          if (pr_find) break;
        };
        if (!pr_find) continue;
      };
      
      CheckIn::TPaxRemItem rem;
      //обычные ремарки (обязательно обрабатываем первыми)
      if (!cats[remUnknown]) LoadPaxRem(pax_id, false, rems);
      for(vector<CheckIn::TPaxRemItem>::iterator r=rems.begin();r!=rems.end();r++)
        r->text=transliter(r->text, 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
      
      //билет
      if (!cats[remTKN]) tkn.fromDB(Qry);
      if (getPaxRem(rpt_params, tkn, inf_indicator, rem)) rems.push_back(rem);
      //документ
      if (!cats[remDOC]) LoadPaxDoc(pax_id, doc);
      if (getPaxRem(rpt_params, doc, inf_indicator, rem)) rems.push_back(rem);
      //виза
      if (!cats[remDOCO]) LoadPaxDoco(pax_id, doco);
      if (getPaxRem(rpt_params, doco, inf_indicator, rem)) rems.push_back(rem);
      //адреса
      if (!cats[remDOCA]) LoadPaxDoca(pax_id, doca);
      for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
        if (getPaxRem(rpt_params, *d, inf_indicator, rem)) rems.push_back(rem);
      //бонус-программа
      if (!cats[remFQT]) LoadPaxFQT(pax_id, fqts);
      for(vector<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();f++)
        if (getPaxRem(rpt_params, *f, rem)) rems.push_back(rem);
        
      if(rpt_params.rpt_type == rtSPEC or rpt_params.rpt_type == rtSPECTXT) {
          vector<CheckIn::TPaxRemItem> tmp_rems;
          for(vector<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();r++)
              if(spec_rems.exists(r->code)) tmp_rems.push_back(*r);
          rems = tmp_rems;
      }
      
      if (rems.empty()) continue;

      xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
      NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
      NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger("reg_no"));
      NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
      NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
      NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
      ostringstream rem_info;
      sort(rems.begin(),rems.end()); //сортировка по priority
      for(vector<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();r++)
      {
        rem_info << ".R/" << r->text << " ";
      };
      NewTextChild(rowNode, "info", rem_info.str());
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText(CAP_DOC,
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void REMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    REM(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(38) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Тип", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(25) << getLocaleText("Ремарки", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
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

void CRS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtCRSTXT or rpt_params.rpt_type == rtCRSUNREGTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form((rpt_params.rpt_type == rtBDOCS ? "bdocs" : "crs"), reqNode, resNode);
    bool pr_unreg = rpt_params.rpt_type == rtCRSUNREG or rpt_params.rpt_type == rtCRSUNREGTXT;
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "      crs_pax.pax_id, "
        "      crs_pax.surname||' '||crs_pax.name family ";
    if(rpt_params.rpt_type != rtBDOCS) {
        SQLText +=
            "      , tlg_binding.point_id_spp AS point_id, "
            "      crs_pax.pers_type, "
            "      crs_pnr.class, "
            "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'_seats',rownum) AS seat_no, "
            "      crs_pnr.airp_arv AS target, "
            "      crs_pnr.airp_arv_final AS last_target, "
            "      crs_pnr.pnr_id, "
            "      report.get_TKNO(crs_pax.pax_id) ticket_no, "
            "      report.get_PSPT(crs_pax.pax_id, 1, :lang) AS document ";
        Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    }
    SQLText +=
        "FROM crs_pnr,tlg_binding,crs_pax ";
    if(pr_unreg)
        SQLText += " , pax ";
    SQLText +=
        "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "      crs_pnr.system='CRS' AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pr_del=0 and "
        "      tlg_binding.point_id_spp = :point_id ";
    if(pr_unreg)
        SQLText +=
            "    and crs_pax.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null ";
    SQLText +=
        "order by ";
    switch(rpt_params.sort) {
        case stRegNo:
        case stSurname:
            SQLText += " family ";
            break;
        case stSeatNo:
            if(rpt_params.rpt_type != rtBDOCS)
                SQLText += " seat_no,";
            SQLText += " family ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_crs");

    TQuery docsQry(&OraSession);
    docsQry.SQLText = "select * from crs_pax_doc where pax_id = :pax_id and rem_code = 'DOCS'";
    docsQry.DeclareVariable("pax_id", otInteger);
    //ремарки пассажиров
    
    TRemGrp rem_grp;
    if(rpt_params.rpt_type != rtBDOCS)
      rem_grp.Load(retPNL_SEL, rpt_params.point_id);
    vector<TPnrAddrItem> pnrs;
    for(; !Qry.Eof; Qry.Next()) {
        int pax_id=Qry.FieldAsInteger("pax_id");
        if(rpt_params.rpt_type == rtBDOCS) {
            docsQry.SetVariable("pax_id", pax_id);
            docsQry.Execute();
            if (!docsQry.Eof)
            {
                if (
                        !docsQry.FieldIsNULL("type") &&
                        !docsQry.FieldIsNULL("issue_country") &&
                        !docsQry.FieldIsNULL("no") &&
                        !docsQry.FieldIsNULL("nationality") &&
                        !docsQry.FieldIsNULL("birth_date") &&
                        !docsQry.FieldIsNULL("gender") &&
                        !docsQry.FieldIsNULL("expiry_date") &&
                        !docsQry.FieldIsNULL("surname") &&
                        !docsQry.FieldIsNULL("first_name")
                   )
                    continue;

                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                NewTextChild(rowNode, "type", rpt_params.ElemIdToReportElem(etPaxDocType, docsQry.FieldAsString("type"), efmtCodeNative));
                NewTextChild(rowNode, "issue_country", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("issue_country"), efmtCodeNative));
                NewTextChild(rowNode, "no", docsQry.FieldAsString("no"));
                NewTextChild(rowNode, "nationality", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("nationality"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("birth_date"))
                    NewTextChild(rowNode, "birth_date", DateTimeToStr(docsQry.FieldAsDateTime("birth_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "gender", rpt_params.ElemIdToReportElem(etGenderType, docsQry.FieldAsString("gender"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("expiry_date"))
                    NewTextChild(rowNode, "expiry_date", DateTimeToStr(docsQry.FieldAsDateTime("expiry_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "surname", docsQry.FieldAsString("surname"));
                NewTextChild(rowNode, "first_name", docsQry.FieldAsString("first_name"));
                NewTextChild(rowNode, "second_name", docsQry.FieldAsString("second_name"));
            } else {
                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            }
        } else {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            GetPnrAddr(Qry.FieldAsInteger("pnr_id"), pnrs);
            string pnr_addr;
            if (!pnrs.empty())
              pnr_addr.append(pnrs.begin()->addr)/*.append("/").append(pnrs.begin()->airline)*/; //пока не надо выводить компанию, может быть потом...
            NewTextChild(rowNode, "pnr_ref", pnr_addr);
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
            NewTextChild(rowNode, "class", rpt_params.ElemIdToReportElem(etClass, Qry.FieldAsString("class"), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
            NewTextChild(rowNode, "target", rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("target"), efmtCodeNative));
            string last_target = Qry.FieldAsString("last_target");
            TElemFmt fmt;
            string last_target_id = ElemToElemId(etAirp, last_target, fmt);
            if(not last_target_id.empty())
                last_target = rpt_params.ElemIdToReportElem(etAirp, last_target_id, efmtCodeNative);

            NewTextChild(rowNode, "last_target", last_target);
            NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
            NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
            NewTextChild(rowNode, "remarks", GetCrsRemarkStr(rem_grp, pax_id));
        }
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if(pr_unreg)
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRSUNREG",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRS",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str()); //!!!
}

void CRSTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CRS(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
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
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(7)  << "PNR"
        << setw(22) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Пас", rpt_params.GetLang())
        << setw(3) << getLocaleText("Кл", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(4)  << getLocaleText("CAP.DOC.AIRP_ARV", rpt_params.GetLang())
        << setw(7)  << getLocaleText("CAP.DOC.TO", rpt_params.GetLang())
        << setw(10) << getLocaleText("Билет", rpt_params.GetLang())
        << setw(10) << getLocaleText("Документ", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
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
    bool pr_web = (rpt_params.rpt_type == rtWEB or rpt_params.rpt_type == rtWEBTXT);
    bool pr_norec = (rpt_params.rpt_type == rtNOREC or rpt_params.rpt_type == rtNORECTXT);
    bool pr_gosho = (rpt_params.rpt_type == rtGOSHO or rpt_params.rpt_type == rtGOSHOTXT);
    if(
            rpt_params.rpt_type == rtEXAMTXT or
            rpt_params.rpt_type == rtWEBTXT or
            rpt_params.rpt_type == rtNORECTXT or
            rpt_params.rpt_type == rtGOSHOTXT
      )
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form(pr_web ? "web" : "exam", reqNode, resNode);

    TQuery Qry(&OraSession);
    BrdInterface::GetPaxQuery(Qry, rpt_params.point_id, NoExists, NoExists, rpt_params.GetLang(), rpt_params.rpt_type, rpt_params.client_type, rpt_params.sort);
    ProgTrace(TRACE5, "Qry: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr datasetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr passengersNode = NewTextChild(datasetsNode, "passengers");
    TRemGrp rem_grp;
    if(not Qry.Eof)
        rem_grp.Load(retBRD_VIEW, rpt_params.point_id);
    for( ; !Qry.Eof; Qry.Next()) {
        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        int pax_id = Qry.FieldAsInteger("pax_id");
        NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger("reg_no"));
        NewTextChild(paxNode, "surname", transliter(Qry.FieldAsString("surname"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(paxNode, "name", transliter(Qry.FieldAsString("name"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        if(pr_web)
            NewTextChild(paxNode, "user_descr", transliter(Qry.FieldAsString("user_descr"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(paxNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
        NewTextChild(paxNode, "pr_exam", Qry.FieldAsInteger("pr_exam"), 0);
        NewTextChild(paxNode, "pr_brd", Qry.FieldAsInteger("pr_brd"), 0);
        NewTextChild(paxNode, "seat_no", Qry.FieldAsString("seat_no"));
        NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(NoExists, pax_id, false, rpt_params.GetLang()));
        NewTextChild(paxNode, "ticket_no", Qry.FieldAsString("ticket_no"));
        NewTextChild(paxNode, "coupon_no", Qry.FieldAsInteger("coupon_no"));
        NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger("rk_amount"));
        NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"));
        bool pr_payment=BagPaymentCompleted(Qry.FieldAsInteger("grp_id"));
        NewTextChild(paxNode, "pr_payment", (int)pr_payment);
        NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(paxNode, "remarks", GetRemarkStr(rem_grp, pax_id));
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    BrdInterface::readTripCounters(rpt_params.point_id, rpt_params, variablesNode, rpt_params.rpt_type, rpt_params.client_type);
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if ( pr_web) {
        if(!rpt_params.client_type.empty()) {
            string ls_type;
            switch(DecodeClientType(rpt_params.client_type.c_str())) {
                case ctWeb:
                    ls_type = getLocaleText("CAP.PAX_LIST.WEB", rpt_params.GetLang());
                    break;
                case ctMobile:
                    ls_type = getLocaleText("CAP.PAX_LIST.MOBILE", rpt_params.GetLang());
                    break;
                case ctKiosk:
                    ls_type = getLocaleText("CAP.PAX_LIST.KIOSK", rpt_params.GetLang());
                    break;
                default:
                    throw Exception("Unexpected client_type: %s", rpt_params.client_type.c_str());
            }
            NewTextChild(variablesNode, "paxlist_type", ls_type);
        }
    } else if(pr_norec)
        NewTextChild(variablesNode, "paxlist_type", "NOREC");
    else if(pr_gosho)
        NewTextChild(variablesNode, "paxlist_type", "GOSHO");
    else
        NewTextChild(variablesNode, "paxlist_type", getLocaleText("CAP.PAX_LIST.BRD", rpt_params.GetLang()));
    if(pr_web and rpt_params.client_type.empty())
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.PAX_LIST.SELF_CKIN",
                    LParams()
                    << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())
                );
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.PAX_LIST",
                    LParams()
                    << LParam("list_type", NodeAsString("paxlist_type", variablesNode))
                    << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang())
                );
    xmlNodePtr currNode = variablesNode->children;
    xmlNodePtr totalNode = NodeAsNodeFast("total", currNode);
    NodeSetContent(totalNode, getLocaleText("CAP.TOTAL.VAL", LParams() << LParam("total", NodeAsString(totalNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EXAM(rpt_params, reqNode, resNode);
    const char col_sym = ' '; //символ разделителя столбцов
    bool pr_web = rpt_params.rpt_type == rtWEBTXT;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    string str;
    ostringstream s;
    s.str("");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    s.str("");
    s << NodeAsString("caption", variablesNode);
    str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << col_sym
        << left << setw(pr_web ? 19 : 30) << getLocaleText("Фамилия", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("Оператор", rpt_params.GetLang());
    s << setw(4)  << getLocaleText("Пас", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("№ м", rpt_params.GetLang());
    else
        s
            << setw(3)  << getLocaleText("Дс", rpt_params.GetLang())
            << setw(4)  << getLocaleText("Пс", rpt_params.GetLang());
    s
        << setw(11) << getLocaleText("Документ", rpt_params.GetLang())
        << setw(14) << getLocaleText("Билет", rpt_params.GetLang())
        << setw(10) << getLocaleText("№ б/б", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    s.str("");
    s
        << NodeAsString("total", variablesNode) << endl
        << getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang());
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
        SeparateString(((string)NodeAsString("surname",rowNode) + " " + NodeAsString("name", rowNode, "")).c_str(),(pr_web ? 18 : 29),rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("document",rowNode, ""),10,rows);
        fields["docs"]=rows;

        SeparateString(NodeAsString("ticket_no",rowNode, ""),13,rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("tags",rowNode, ""),10,rows);
        fields["tags"]=rows;

        SeparateString(NodeAsString("user_descr",rowNode, ""),8,rows);
        fields["user_descr"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(pr_web ? 18 : 29) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym;
            if(pr_web)
                s
                    << left << setw(8) << (!fields["user_descr"].empty() ? *(fields["user_descr"].begin()) : "") << col_sym;
            s
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode) : "") << col_sym;
            if(pr_web) {
                s
                    << left <<  setw(8) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym;
            } else {
                s
                    << left <<  setw(3) << (row == 0 ? (strcmp(NodeAsString("pr_exam", rowNode, ""), "") == 0 ? "-" : "+") : "")
                    << left <<  setw(4) << (row == 0 ? (strcmp(NodeAsString("pr_brd", rowNode, ""), "") == 0 ? "-" : "+") : "");
            }
            s
                << left << setw(10) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "") << col_sym
                << left << setw(13) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(10) << (!fields["tags"].empty() ? *(fields["tags"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["docs"].empty() ||
                !fields["tkts"].empty() ||
                !fields["tags"].empty() ||
                !fields["user_descr"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void WEB(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAM(rpt_params, reqNode, resNode);
}

void WEBTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAMTXT(rpt_params, reqNode, resNode);
}

void SPPCentrovka(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCargo(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCex(TDateTime date, xmlNodePtr resNode)
{
}

void  DocsInterface::RunSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    string name = NodeAsStringFast("name", node);
    get_new_report_form(name, reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
}

void  DocsInterface::RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    TRptParams rpt_params;
    rpt_params.Init(node);
    switch(rpt_params.rpt_type) {
        case rtPTM:
            PTM(rpt_params, reqNode, resNode);
            break;
        case rtPTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtBTM:
            BTM(rpt_params, reqNode, resNode);
            break;
        case rtBTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtWEB:
            WEB(rpt_params, reqNode, resNode);
            break;
        case rtWEBTXT:
            WEBTXT(rpt_params, reqNode, resNode);
            break;
        case rtREFUSE:
            REFUSE(rpt_params, reqNode, resNode);
            break;
        case rtREFUSETXT:
            REFUSETXT(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRES:
            NOTPRES(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRESTXT:
            NOTPRESTXT(rpt_params, reqNode, resNode);
            break;
        case rtSPEC:
        case rtREM:
            REM(rpt_params, reqNode, resNode);
            break;
        case rtSPECTXT:
        case rtREMTXT:
            REMTXT(rpt_params, reqNode, resNode);
            break;
        case rtCRS:
        case rtCRSUNREG:
        case rtBDOCS:
            CRS(rpt_params, reqNode, resNode);
            break;
        case rtCRSTXT:
        case rtCRSUNREGTXT:
            CRSTXT(rpt_params, reqNode, resNode);
            break;
        case rtEXAM:
        case rtNOREC:
        case rtGOSHO:
            EXAM(rpt_params, reqNode, resNode);
            break;
        case rtEXAMTXT:
        case rtNORECTXT:
        case rtGOSHOTXT:
            EXAMTXT(rpt_params, reqNode, resNode);
            break;
        default:
            throw AstraLocale::UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
    }
}

void  DocsInterface::LogPrintEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT", LEvntPrms()
                                       << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong),
                                       ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
}

void  DocsInterface::LogExportEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo::Instance()->LocaleToLog("EVT.EXPORT_REPORT", LEvntPrms()
                                       << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                                       << PrmSmpl<std::string>("fmt", NodeAsString("export_name", reqNode)),
                                       ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
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
    Qry.SQLText = "update fr_forms2 set form = :form where name = :name and version = :version";
    Qry.CreateVariable("version", otString, version);
    Qry.CreateVariable("name", otString, name);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.Execute();
    if(!Qry.RowsProcessed())
      throw UserException("MSG.REPORT_UPDATE_FAILED.NOT_FOUND", LParams() << LParam("report_name", name));
    TReqInfo::Instance()->LocaleToLog("EVT.UPDATE_REPORT", LEvntPrms() << PrmSmpl<std::string>("name", name), evtSystem);
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
        result[0] = ALL_CKIN_ZONES; // группа залов "все залы"
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
            NewTextChild(itemNode, "name", getLocaleText("Др. залы"));
        } else if(*iv == ALL_CKIN_ZONES) {
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

int testbm(int argc,char **argv)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");
    xmlDocPtr resDoc=xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr rootNode=xmlNewDocNode(resDoc,NULL,BAD_CAST "query",NULL);
    TRptParams rpt_params;
    rpt_params.point_id = 603906;
    rpt_params.ckin_zone = "area bis";
    rpt_params.pr_et = false;
    rpt_params.pr_trfer = false;
    rpt_params.pr_brd = false;
    BTM(rpt_params, rootNode, rootNode);
    return 0;
}

void get_report_form(const string name, xmlNodePtr resNode)
{
    string form;
    string version;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select form, pr_locale from fr_forms2 where name = :name and version = :version ";
    version = get_report_version(name);
    Qry.CreateVariable("version", otString, version);
    Qry.CreateVariable("name", otString, name);
    Qry.Execute();
    if(Qry.Eof) {
        NewTextChild(resNode, "FormNotExists", name);
        SetProp(NewTextChild(resNode, "form"), "name", name);
        return;
    }
    // положим в ответ шаблон отчета
    int len = Qry.GetSizeLongField("form");
    shared_array<char> data (new char[len]);
    Qry.FieldAsLong("form", data.get());
    form.clear();
    form.append(data.get(), len);

    xmlNodePtr formNode = ReplaceTextChild(resNode, "form", form);
    SetProp(formNode, "name", name);
    SetProp(formNode, "version", version);
    if (Qry.FieldAsInteger("pr_locale") != 0)
        SetProp(formNode, "pr_locale");
}

void get_compatible_report_form(const string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if (TReqInfo::Instance()->desk.compatible(ADD_FORM_VERSION))
        get_new_report_form(name, reqNode, resNode);
    else
        get_report_form(name, resNode);
}

void get_new_report_form(const string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(GetNode("LoadForm", reqNode) == NULL)
        return;
    get_report_form(name, resNode);
}

