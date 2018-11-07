#include <set>
#include "docs.h"
#include "stat_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_date_time.h"
#include "base_tables.h"
#include "season.h"
#include "brd.h"
#include "aodb.h"
#include "astra_misc.h"
#include "term_version.h"
#include "load_fr.h"
#include "passenger.h"
#include "remarks.h"
#include "telegram.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/str_utils.h"
#include <boost/shared_array.hpp>
#include "baggage_calc.h"
#include "salons.h"
#include "franchise.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace boost;

const string ALL_CKIN_ZONES = " ";

string vsHow_ru(int nmb, int range)
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

void SeasonListVars(int trip_id, int pr_lat, xmlNodePtr variablesNode, xmlNodePtr reqNode)
{
  NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
  if(STAT::bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");
  vector<SEASON::TViewPeriod> viewp;
  SEASON::ReadTripInfo( trip_id, viewp, GetNode( "seasonvars", reqNode ) );
  bool pr_find = false;
  for ( vector<SEASON::TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
    for ( vector<SEASON::TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end(); j++ ) {
      NewTextChild( variablesNode, "trip", j->name );
      pr_find = true;
      break;
    }
  }
  if ( !pr_find ) {
    throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  }
}

void populate_doc_cap(xmlNodePtr variablesNode, string lang)
{
    NewTextChild(variablesNode, "doc_cap_no", getLocaleText("�", lang));
    NewTextChild(variablesNode, "doc_cap_name", getLocaleText("�.�.�.", lang));
    NewTextChild(variablesNode, "doc_cap_surname", getLocaleText("�������", lang));
    NewTextChild(variablesNode, "doc_cap_doc", getLocaleText("���㬥��", lang));
    NewTextChild(variablesNode, "doc_cap_tkt", getLocaleText("�����", lang));
    NewTextChild(variablesNode, "doc_cap_tkt_no", getLocaleText("� �����", lang));
    NewTextChild(variablesNode, "doc_cap_ref_type", getLocaleText("��稭� ���뫥�", lang));
    NewTextChild(variablesNode, "doc_cap_bag", getLocaleText("���.", lang));
    NewTextChild(variablesNode, "doc_cap_baggage", getLocaleText("�����", lang));
    NewTextChild(variablesNode, "doc_cap_rk", getLocaleText("�/�", lang));
    NewTextChild(variablesNode, "doc_cap_pay", getLocaleText("���.", lang));
    NewTextChild(variablesNode, "doc_cap_tags", getLocaleText("�� ���. ��ப", lang));
    NewTextChild(variablesNode, "doc_cap_tags_short", getLocaleText("� �/�", lang));
    NewTextChild(variablesNode, "doc_cap_rem", getLocaleText("���.", lang));
    NewTextChild(variablesNode, "doc_cap_pas", getLocaleText("���", lang));
    NewTextChild(variablesNode, "doc_cap_type", getLocaleText("���", lang));
    NewTextChild(variablesNode, "doc_cap_seat_no", getLocaleText("� �", lang));
    NewTextChild(variablesNode, "doc_cap_old_seat_no", getLocaleText("���.", lang));
    NewTextChild(variablesNode, "doc_cap_new_seat_no", getLocaleText("���.", lang));
    NewTextChild(variablesNode, "doc_cap_remarks", getLocaleText("����ન", lang));
    NewTextChild(variablesNode, "doc_cap_cl", getLocaleText("��", lang));
    NewTextChild(variablesNode, "doc_cap_seats", getLocaleText("���", lang));
    NewTextChild(variablesNode, "doc_cap_dest", getLocaleText("�/�", lang));
    NewTextChild(variablesNode, "doc_cap_to", getLocaleText("CAP.DOC.TO", lang));
    NewTextChild(variablesNode, "doc_cap_ex", getLocaleText("��", lang));
    NewTextChild(variablesNode, "doc_cap_brd", getLocaleText("��", lang));
    NewTextChild(variablesNode, "doc_cap_test", (STAT::bad_client_img_version() and not get_test_server()) ? " " : getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "doc_cap_user_descr", getLocaleText("������", lang));
    NewTextChild(variablesNode, "doc_cap_emd_no", getLocaleText("� EMD", lang));
    NewTextChild(variablesNode, "doc_cap_total", getLocaleText("�⮣�:", lang));
    NewTextChild(variablesNode, "doc_cap_overall", getLocaleText("�ᥣ�:", lang));
    NewTextChild(variablesNode, "doc_cap_vou_quantity", getLocaleText("���-�� ����஢", lang));
    NewTextChild(variablesNode, "doc_cap_descr", getLocaleText("���ᠭ��", lang));
    NewTextChild(variablesNode, "doc_cap_rcpt_no", getLocaleText("� ���⠭樨", lang));
    NewTextChild(variablesNode, "doc_cap_service_code", getLocaleText("��� ��㣨", lang));
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
        "   act_out, "
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
        // ᭠砫� airline_name, ��⮬ airline
        // ���� � ��� ��ਠ�� �㤥� ���������� 㦥 ᪮����祭�� airline
        airline_name = rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong);
        airline = rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative);
    }

    ostringstream trip;
    trip
        << airline
        << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no")
        << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("suffix"), efmtCodeNative);

    NewTextChild(variablesNode, "trip", trip.str());
    TDateTime scd_out, real_out;
    scd_out= UTCToClient(getReportSCDOut(point_id),tz_region);
    if(Qry.FieldIsNULL("act_out"))
        real_out = scd_out;
    else
        real_out= UTCToClient(Qry.FieldAsDateTime("act_out"),tz_region);
    NewTextChild(variablesNode, "scd_out", DateTimeToStr(scd_out, "dd.mm.yyyy"));
    NewTextChild(variablesNode, "real_out", DateTimeToStr(real_out, "dd.mm.yyyy"));
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm"));
    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn"));
    NewTextChild(variablesNode, "day_issue", DateTimeToStr(issued, "dd.mm.yy"));


    NewTextChild(variablesNode, "lang", TReqInfo::Instance()->desk.lang );
    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",airp);
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
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT", rpt_params.GetLang()));
}

void TRptParams::Init(xmlNodePtr node)
{
    prn_params.get_prn_params(node->parent);
    point_id = NodeAsIntegerFast("point_id", node);
    rpt_type = DecodeRptType(NodeAsStringFast("rpt_type", node));
    orig_rpt_type = rpt_type;
    cls = NodeAsStringFast("cls", node, "");
    subcls = NodeAsStringFast("subcls", node, "");
    airp_arv = NodeAsStringFast("airp_arv", node, "");
    ckin_zone = NodeAsStringFast("ckin_zone", node, ALL_CKIN_ZONES.c_str());
    pr_et = NodeAsIntegerFast("pr_et", node, 0) != 0;
    text = NodeAsIntegerFast("text", node, NoExists);
    pr_trfer = NodeAsIntegerFast("pr_trfer", node, 0) != 0;
    pr_brd = NodeAsIntegerFast("pr_brd", node, 0) != 0;
    sort = (TSortType)NodeAsIntegerFast("sort", node, 0);
    if(text != NoExists and text != 0 and
            // � ��� ���⮢ ��� ⥪�⮢��� ��ਠ��
            // �� �ନ஢���� ���⮢ ��-��� DCP CUTE, ��।����� ⥣ text = 1
            // ���� �� visibl = false � �ନ����
            rpt_type != rtBDOCS and
            rpt_type != rtLOADSHEET and
            rpt_type != rtNOTOC and
            rpt_type != rtLIR and
            rpt_type != rtANNUL_TAGS and
            rpt_type != rtVOUCHERS and
            rpt_type != rtKOMPLEKT
            )
        rpt_type = TRptType((int)rpt_type + 1);
    string route_country;
    route_inter = IsRouteInter(point_id, NoExists, route_country);
    if(route_country == "��")
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
    set<int> rem_grps;
    if(remsNode != NULL) {
        TBaseTable &base_rems = base_tables.get("ckin_rem_types");
        xmlNodePtr currNode = remsNode->children;
        for(; currNode; currNode = currNode->next)
        {
            string rem = NodeAsString(currNode);
            rem_grps.insert(((const TCkinRemTypesRow&)base_rems.get_row("code", rem)).grp_id);
            TRemCategory cat=getRemCategory(rem, "");
            rems[cat].push_back(NodeAsString(currNode));
        };
    }

    xmlNodePtr remGrpNode = GetNodeFast("rem_grp", node);
    if(remGrpNode != NULL) {
        xmlNodePtr currNode = remGrpNode->children;
        for(; currNode; currNode = currNode->next)
        {
            int rem_grp_id = NodeAsInteger(currNode);
            // �᫨ �� �뫮 ��࠭� ६�ப ��� ⥪. ��㯯�, ������塞 �� ६�ન ⥪. ��㯯�
            if(rem_grps.find(rem_grp_id) == rem_grps.end()) {
                TCachedQuery Qry("select code from ckin_rem_types where grp_id = :grp_id",
                        QParams() << QParam("grp_id", otInteger, rem_grp_id));
                Qry.get().Execute();
                for(; not Qry.get().Eof; Qry.get().Next()) {
                    string rem = Qry.get().FieldAsString("code");
                    TRemCategory cat=getRemCategory(rem, "");
                    rems[cat].push_back(rem);
                }
            }
        };
    }

    xmlNodePtr rficNode = GetNodeFast("rfic", node);
    if(rficNode != NULL) {
        xmlNodePtr currNode = rficNode->children;
        for(; currNode; currNode = currNode->next)
        {
            rfic.push_back(NodeAsString(currNode));
        };
    }
    if(IsInter()) req_lang = AstraLocale::LANG_EN;
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

  string lang=GetLang(fmt, firm_lang); //ᯥ樠�쭮 �뭥ᥭ�, ⠪ ��� fmt � ��楤�� ����� ����������

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};

string TRptParams::ElemIdToReportElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
{
  if (id==ASTRA::NoExists) return "";

  string lang=GetLang(fmt, firm_lang); //ᯥ樠�쭮 �뭥ᥭ�, ⠪ ��� fmt � ��楤�� ����� ����������

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

const int TO_RAMP_PRIORITY = 1000;

void t_rpt_bm_bag_name::get(string class_code, TBagTagRow &bag_tag_row, TRptParams &rpt_params)
{
    string &result = bag_tag_row.bag_name;
    for(vector<TBagNameRow>::iterator iv = bag_names.begin(); iv != bag_names.end(); iv++) {
        bool eval = false;
        if(class_code == iv->class_code) {
            if(bag_tag_row.rfisc.empty()) {
                if(bag_tag_row.bag_type != NoExists and bag_tag_row.bag_type == iv->bag_type) {
                    eval = true;
                }
            } else if(bag_tag_row.rfisc == iv->rfisc) {
                eval = true;
            }
        }
        if(eval) {
            result = rpt_params.IsInter() ? iv->name_lat : iv->name;
            if(result.empty())
                result = iv->name;
            break;
        }
    }
    if(not result.empty())
        bag_tag_row.bag_name_priority = bag_tag_row.bag_type;
}

void t_rpt_bm_bag_name::init(const string &airp, const string &airline, bool pr_stat_fv)
{
    bag_names.clear();
    TQuery Qry(&OraSession);
    string SQLText = (string)
        "select "
        "   bag_type, "
        "   rfisc, "
        "   class, "
        "   airp, "
        "   airline, "
        "   name, "
        "   name_lat "
        "from " +
        (pr_stat_fv ? "stat_fv_bag_names" : "rpt_bm_bag_names ") +
        " where "
        "   (airp is null or "
        "   airp = :airp) and "
        "   (airline is null or "
        "   airline = :airline) "
        "order by "
        "   airline nulls last, "
        "   airp nulls last, "
        "   name, name_lat ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBagNameRow bag_name_row;
        if(not Qry.FieldIsNULL("bag_type"))
            bag_name_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_name_row.rfisc = Qry.FieldAsString("rfisc");
        bag_name_row.class_code = Qry.FieldAsString("class");
        bag_name_row.airp = Qry.FieldAsString("airp");
        bag_name_row.airline = Qry.FieldAsString("airline");
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

bool lessTagNos(const t_tag_nos_row &p1, const t_tag_nos_row &p2)
{
    return p1.no < p2.no;
}

string get_tag_rangeA(vector<t_tag_nos_row> &tag_nos, vector<t_tag_nos_row>::iterator begin, vector<t_tag_nos_row>::iterator end, string lang)
{
    string lim = getLocaleText("(���)", lang);
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
    int seats, adl_m, adl_f, chd, inf, rk_weight, bag_amount, bag_weight, excess_wt, excess_pc;
    int xcr, dhc, mos, jmp;
    TPMTotalsRow():
        seats(0),
        adl_m(0),
        adl_f(0),
        chd(0),
        inf(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        excess_wt(0),
        excess_pc(0),
        xcr(0),
        dhc(0),
        mos(0),
        jmp(0)
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

TDateTime getReportSCDOut(int point_id)
{
  TDateTime res = AODB_POINTS::getSCD_OUT( point_id );
  if ( res == ASTRA::NoExists ) {
    TTripInfo tripInfo;
    if ( tripInfo.getByPointId ( point_id ) ) {
      res = tripInfo.scd_out;
    }
  }
  return res;
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
            et = getLocaleText("(��)", rpt_params.dup_lang());
            et_lat = getLocaleText("(��)", AstraLocale::LANG_EN);
        }
        NewTextChild(variablesNode, "ptm", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et), rpt_params.dup_lang()));
        NewTextChild(variablesNode, "ptm_lat", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et_lat), AstraLocale::LANG_EN));
    }
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT \n"
        "   pax.*, \n"
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
        "   salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, \n";
    if(rpt_params.pr_et) { //��
        SQLText +=
            "    ticket_no||'/'||coupon_no AS remarks, \n";
    };
    SQLText +=
        "   NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS rk_weight, \n"
        "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse),0) AS excess_wt, \n"
        "   NVL(ckin.get_excess_pc(pax.grp_id, pax.pax_id),0) AS excess_pc, \n"
        "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, \n"
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

    if(not rpt_params.subcls.empty()) {
        SQLText +=
            "   pax.subclass = :subcls and \n";
        Qry.CreateVariable("subcls", otString, rpt_params.subcls);
    }

    if(not rpt_params.cls.empty()) {
        SQLText +=
            "   pax_grp.class = :cls and \n";
        Qry.CreateVariable("cls", otString, rpt_params.cls);
    }


    if(rpt_params.pr_et) //��
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(not rpt_params.airp_arv.empty()) { // ᥣ����
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
        case stServiceCode:
        case stRegNo:
            SQLText +=
                "    pax.reg_no ASC, \n"
                "    pax.seats DESC \n";
            break;
        case stSurname:
            SQLText +=
                "    pax.surname ASC, \n"
                "    pax.name ASC, \n"
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
    // ᫥���騥 2 ��६���� ������� ��� �㦤 FastReport
    map<string, int> fr_target_ref;
    int fr_target_ref_idx = 0;

    TPMTotals PMTotals;
    TRemGrp rem_grp;
    bool rem_grp_loaded = false;
    for(; !Qry.Eof; Qry.Next()) {
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry);

        if(not rpt_params.mkt_flt.empty()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(pax.id);
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
        key.lvl = ((const TClsGrpRow&)base_tables.get("cls_grp").get_row( "id", class_grp, true)).priority;
        if(rpt_params.pr_trfer) {
            key.pr_trfer = Qry.FieldAsInteger("pr_trfer");
        }
        TPMTotalsRow &row = PMTotals[key];
        row.seats += pax.seats;
        switch(pax.getTrickyGender())
        {
          case TTrickyGender::Male:
            row.adl_m++;
            break;
          case TTrickyGender::Female:
            row.adl_f++;
            break;
          case TTrickyGender::Child:
            row.chd++;
            break;
          case TTrickyGender::Infant:
            row.inf++;
            break;
          default:
            throw Exception("DecodePerson failed");
        }

        row.rk_weight += Qry.FieldAsInteger("rk_weight");
        row.bag_amount += Qry.FieldAsInteger("bag_amount");
        row.bag_weight += Qry.FieldAsInteger("bag_weight");
        int excess_wt=Qry.FieldAsInteger("excess_wt");
        int excess_pc=Qry.FieldAsInteger("excess_pc");

        row.excess_wt += excess_wt;
        row.excess_pc += excess_pc;

        switch(pax.crew_type) {
            case TCrewType::ExtraCrew:
                row.xcr++;
                break;
            case TCrewType::DeadHeadCrew:
                row.dhc++;
                break;
            case TCrewType::MiscOperStaff:
                row.mos++;
                break;
            default:
                break;
        }
        if(pax.is_jmp) row.jmp++;

        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", pax.reg_no);
        NewTextChild(rowNode, "full_name", transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
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
        NewTextChild(rowNode, "seats", pax.seats);
        NewTextChild(rowNode, "crew_type", CrewTypes().encode(pax.crew_type));
        NewTextChild(rowNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        NewTextChild(rowNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(rowNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));

        NewTextChild(rowNode, "excess", TComplexBagExcess(TBagKilos(excess_wt),
                                                          TBagPieces(excess_pc)).
                                          view(OutputLang(rpt_params.GetLang()), true));
        // ��� �㬬� �� ��㯯� �ᥣ� � �����
        NewTextChild(rowNode, "excess_pc", excess_pc);
        NewTextChild(rowNode, "excess_kg", excess_wt);

        {
          string gender;
          switch(pax.getTrickyGender())
          {
            case TTrickyGender::Male:
              gender = "M";
              break;
            case TTrickyGender::Female:
              gender = "F";
              break;
            default:
              break;
          };
          NewTextChild(rowNode, "pers_type", DocTrickyGenders().encode(pax.getTrickyGender()));
          NewTextChild(rowNode, "gender", gender);
        }
        NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

        // seat_no ���⠥��� � �����묨 ᫥�� �஡�����, �⮡� order by
        // �ࠢ��쭮 ��ࠡ��뢠�, ����� �� �஡��� ��� �� � 祬�
        // (� ��⭮�� ��� ������� � ⥪�⮢�� ����).
        NewTextChild(rowNode, "seat_no", TrimString(pax.seat_no));

        if(not rem_grp_loaded) {
            rem_grp_loaded = true;
            rem_grp.Load(retRPT_PM, rpt_params.point_id);
        }
        NewTextChild(rowNode, "remarks",
                (rpt_params.pr_et ? Qry.FieldAsString("remarks") : GetRemarkStr(rem_grp, pax.id, rpt_params.GetLang())));
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
        NewTextChild(rowNode, "adl", row.adl_m+row.adl_f);
        NewTextChild(rowNode, "adl_f", row.adl_f);
        NewTextChild(rowNode, "chd", row.chd);
        NewTextChild(rowNode, "inf", row.inf);
        NewTextChild(rowNode, "rk_weight", row.rk_weight);
        NewTextChild(rowNode, "bag_amount", row.bag_amount);
        NewTextChild(rowNode, "bag_weight", row.bag_weight);
        NewTextChild(rowNode, "excess", TComplexBagExcess(TBagKilos(row.excess_wt),
                                                          TBagPieces(row.excess_pc)).
                                          view(OutputLang(rpt_params.GetLang()), true));
        NewTextChild(rowNode, "xcr", row.xcr);
        NewTextChild(rowNode, "dhc", row.dhc);
        NewTextChild(rowNode, "mos", row.mos);
        NewTextChild(rowNode, "jmp", row.jmp);
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
        "   scd_out, "
        "   act_out, "
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
        Franchise::TProp franchise_prop;
        if(
                franchise_prop.get(rpt_params.point_id, Franchise::TPropType::paxManifest) and
                franchise_prop.val == Franchise::pvNo
          ) {
            airline = franchise_prop.franchisee.airline;
            flt_no = franchise_prop.franchisee.flt_no;
            suffix = franchise_prop.franchisee.suffix;
        } else {
            airline = Qry.FieldAsString("airline");
            flt_no = Qry.FieldAsInteger("flt_no");
            suffix = Qry.FieldAsString("suffix");
        }
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
        NewTextChild(variablesNode, "zone"); // ���⮩ ⥣ - ��� ��⠫���樨 �� ����
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
    TDateTime scd_out = UTCToLocal(getReportSCDOut(rpt_params.point_id), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", rpt_params.IsInter()));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", rpt_params.IsInter()));
    NewTextChild(variablesNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, rpt_params.airp_arv, efmtNameLong));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", rpt_params.IsInter()));

    NewTextChild(variablesNode, "pr_vip", 2);
    string pr_brd_pax_str = (string)"��������� " + (rpt_params.pr_brd ? "(��ᠦ)" : "(��ॣ)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "pr_group", rpt_params.sort == stRegNo); // �᫨ ���஢�� �� ॣ. ��., � �뤥�塞 ��㯯� ���ᠦ�஢ � fr-����
    NewTextChild(variablesNode, "kg", getLocaleText("��", rpt_params.GetLang()));
    NewTextChild(variablesNode, "pc", getLocaleText("�", rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);

    TDateTime takeoff = NoExists;
    if(not Qry.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(Qry.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
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
    Qry.SQLText = "select airp, airline from points where point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("RunBMNew: point_id %d not found", rpt_params.point_id);
    string airp = Qry.FieldAsString(0);
    string airline = Qry.FieldAsString(1);
    bag_names.init(airp, airline);
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
        "    bag2.rfisc, "
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
        if(not Qry.FieldIsNULL("bag_type"))
            bag_tag_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_tag_row.to_ramp = Qry.FieldAsInteger("to_ramp");
        if(bag_tag_row.to_ramp)
            bag_tag_row.to_ramp_str = getLocaleText("� �����", rpt_params.GetLang());

        string class_code = Qry.FieldAsString("class");
        bag_tag_row.rfisc = Qry.FieldAsString("rfisc");
        bag_names.get(class_code, bag_tag_row, rpt_params);
        if(Qry.FieldIsNULL("class"))
            bag_tag_row.class_priority = 100;
        else {
            bag_tag_row.class_priority = ((const TClassesRow&)base_tables.get("classes").get_row( "code", class_code)).priority;
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
            // �饬 ���ਢ易��� ��ન ��� ������ ��㯯�
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
                bm_table.back().class_name = getLocaleText("��ᮯ஢������� �����", rpt_params.GetLang());
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
    // ������ ��६���� ����
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
        "   act_out, "
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

    string suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.empty()) {
        Franchise::TProp franchise_prop;
        if(
                franchise_prop.get(rpt_params.point_id, Franchise::TPropType::bagManifest) and
                franchise_prop.val == Franchise::pvNo
          ) {
            airline = franchise_prop.franchisee.airline;
            flt_no = franchise_prop.franchisee.flt_no;
            suffix = franchise_prop.franchisee.suffix;
        } else {
            airline = Qry.FieldAsString("airline");
            flt_no = Qry.FieldAsInteger("flt_no");
            suffix = Qry.FieldAsString("suffix");
        }
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
    TDateTime scd_out = UTCToLocal(getReportSCDOut(rpt_params.point_id), tz_region);
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
    string pr_brd_pax_str = (string)"����� �������� ��ப " + (rpt_params.pr_brd ? "(��ᠦ)" : "(��ॣ)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // ���⮩ ⥣ - ��� ��⠫���樨 �� ����
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);
    TDateTime takeoff = NoExists;
    if(not Qry.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(Qry.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
}

string get_test_str(int page_width, string lang)
{
    string result;
    for(int i=0;i<page_width/6;i++) result += " " + ( STAT::bad_client_img_version() and not get_test_server() ? " " : getLocaleText("CAP.TEST", lang)) + " ";
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
  //⥪�⮢� �ଠ�
  int page_width=75;
  //ᯥ樠�쭮 ������ ��� ��ਫ���᪨� ᨬ�����, ⠪ ��� � �ନ���� �� �ᯮ�� �஡����
  //���ᨬ��쭠� ����� ��ப� �� �ᯮ�� � �����! �� ������ �ॢ���� ~147 (65 ��� + 15 ���)
  int max_symb_count= rpt_params.IsInter() ? page_width : 60;
  NewTextChild(variablesNode, "page_width", page_width);
  NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
  if(STAT::bad_client_img_version())
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
    str.assign(getLocaleText("�������� ���������", rpt_params.GetLang()));

  s << setfill(' ')
    << str
    << right << setw(page_width-str.size())
    << string(NodeAsString((rpt_params.IsInter() ? "own_airp_name_lat" : "own_airp_name"),variablesNode)).substr(0,max_symb_count-str.size());
  NewTextChild(variablesNode, "page_header_top", s.str());


  s.str("");
  str.assign(getLocaleText("�������� ��� ������: ", rpt_params.GetLang()));
  s << left
    << str
    << string(NodeAsString("airline_name",variablesNode)).substr(0,max_symb_count-str.size()) << endl
    << setw(10) << getLocaleText("� ३�", rpt_params.GetLang());
  if (rpt_params.IsInter())
    s << setw(19) << "Aircraft";
  else
    s << setw(9)  << "� ��"
      << setw(10) << "����� ��. ";

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << getLocaleText("�/� �뫥�", rpt_params.GetLang())
      << setw(20) << getLocaleText("�/� �����祭��", rpt_params.GetLang());
  else
    s << setw(35) << getLocaleText("�/� �뫥�", rpt_params.GetLang());
  s << setw(6)  << getLocaleText("���", rpt_params.GetLang())
    << setw(5)  << getLocaleText("�६�", rpt_params.GetLang()) << endl;

  s << setw(10) << NodeAsString("flt",variablesNode)
    << setw(11) << NodeAsString("bort",variablesNode)
    << setw(4)  << NodeAsString("craft",variablesNode)
    << setw(4)  << NodeAsString("park",variablesNode);

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,20-1)
      << setw(20) << string(NodeAsString("airp_arv_name",variablesNode)).substr(0,20-1);
  else
    s << setw(35) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,40-1);

  s << setw(6) << NodeAsString("scd_date",variablesNode)
    << setw(5) << NodeAsString("scd_time",variablesNode);
  string departure = NodeAsString("takeoff", variablesNode);
  if(not departure.empty())
      s << endl << getLocaleText("�뫥�", rpt_params.GetLang()) << ": " << departure;
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
      << setw(rpt_params.IsInter()?13:14) << (getLocaleText("�������", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("���", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?4:3)   << (getLocaleText("��", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("CAP.DOC.SEAT_NO", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("��", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("��", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("CAP.DOC.BAG", rpt_params.GetLang()))
      << setw(6)  << (getLocaleText("�/��", rpt_params.GetLang()))
      << setw(15) << (getLocaleText("CAP.DOC.BAG_TAG_NOS", rpt_params.GetLang()))
      << setw(9)  << (getLocaleText("����ન", rpt_params.GetLang()));
  else
    s << left
      << setw(29) << (getLocaleText("����� �������� ��ப", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("����", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("����", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("���", rpt_params.GetLang()))
      << setw(8)  << (getLocaleText("� ����.", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("CAP.DOC.HOLD", rpt_params.GetLang()))
      << setw(11) << (getLocaleText("��ᥪ", rpt_params.GetLang()));

  NewTextChild(variablesNode, "page_header_bottom", s.str() );

  if (rpt_params.rpt_type==rtPTMTXT)
  {
    s.str("");
    s
        << setw(17)
        << (getLocaleText("�ᥣ� � �����", rpt_params.GetLang()))
        << setw(9)
        << "M/F"
        << setw(4)
        << getLocaleText("���", rpt_params.GetLang())
        << right
        << setw(3)
        << getLocaleText("��", rpt_params.GetLang()) << " "
        << setw(3)
        << getLocaleText("��", rpt_params.GetLang()) << " "
        << left
        << setw(7)
        << getLocaleText("���.", rpt_params.GetLang())
        << right
        << setw(5)
        << getLocaleText("�/��", rpt_params.GetLang())
        << setw(7) << " " // ���������� 7 �஡���� (��易�. �.�. ���. 䫠� right, �. ���)
        << "XCR DHC MOS JMP"
        << endl
        // ����� �����, �� ����� � �/�� (%2u/%-4u%5u) �ᯮ������ �������, �� �� ���� ���.
        << "%-16s %-7s  %3u %3u %3u %2u/%-4u%5u       %3u %3u %3u %3u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("�ᥣ�", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("�������", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << (getLocaleText("�ᥣ�", rpt_params.GetLang())) << endl;

    s << setw(7) << (getLocaleText("��ᥫ", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("��/�", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("��", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("��", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("����", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("���", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("�/��", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()))
      << setw(16) << "XCR DHC MOS JMP" << endl
      << "%-6u %-7s %-6u %-6u %-6u %-6u %-6u %-6s  %-3u %-3u %-3u %-3u" << endl
      << (getLocaleText("�������", rpt_params.GetLang())) << endl
      << setw(30) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 30) << endl
      << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));

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
        str.assign(getLocaleText("��", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //ࠡ����� 䠬����, ��ન, ६�ન
      SeparateString(NodeAsString("full_name",rowNode),13,rows);
      fields["full_name"]=rows;
      SeparateString(NodeAsString("tags",rowNode),15,rows);
      fields["tags"]=rows;
      SeparateString(NodeAsString("remarks",rowNode),9,rows);
      fields["remarks"]=rows;

      string gender = NodeAsString("gender",rowNode);

      row=0;
      string pers_type=NodeAsString("pers_type",rowNode);
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << right << setw(3) << (row==0?NodeAsString("reg_no",rowNode):"") << " "
          << left << setw(13) << (!fields["full_name"].empty()?*(fields["full_name"].begin()):"") << " "
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
        s << setw(15) << (getLocaleText("�ᥣ� ������", rpt_params.GetLang()));
      else
      {
        if (k==0)
          s << setw(19) << (getLocaleText("�ᥣ� ����. ���.", rpt_params.GetLang()));
        else
          s << setw(19) << (getLocaleText("�ᥣ� ��. ���.", rpt_params.GetLang()));
      };

      s << setw(7) << (getLocaleText("��ᥫ", rpt_params.GetLang()))
        << setw(8) << (getLocaleText("��/�", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("��", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("��", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("����", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("���", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("�/��", rpt_params.GetLang()))
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
      adl_fem << NodeAsInteger("adl", rowNode) << '/' << NodeAsInteger("adl_f", rowNode);

      s.str("");
      s << setw(rpt_params.pr_trfer?19:15) << NodeAsString("class_name",rowNode)
        << setw(7) << NodeAsInteger("seats",rowNode)
        << setw(8) << adl_fem.str()
        << setw(7) << NodeAsInteger("chd",rowNode)
        << setw(7) << NodeAsInteger("inf",rowNode)
        << setw(7) << NodeAsInteger("bag_amount",rowNode)
        << setw(7) << NodeAsInteger("bag_weight",rowNode)
        << setw(7) << NodeAsInteger("rk_weight",rowNode)
        << setw(7) << NodeAsString("excess",rowNode) << endl
        << "XCR/DHC/MOS/JMP: "
        << NodeAsInteger("xcr",rowNode) << "/"
        << NodeAsInteger("dhc",rowNode) << "/"
        << NodeAsInteger("mos",rowNode) << "/"
        << NodeAsInteger("jmp",rowNode);

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
        str.assign(getLocaleText("��", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //ࠧ������ ��������� ��ப, 梥�
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
        s << setw(offset) << "" //�����
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
          s << setw(39) << (getLocaleText("�ᥣ� ������, �᪫��� �࠭����", rpt_params.GetLang()));
        else
          s << setw(39) << (getLocaleText("�ᥣ� �࠭��୮�� ������", rpt_params.GetLang()));
        s << "%4u %6u";
        if (k==0)
          NewTextChild(variablesNode, "total_not_trfer_fmt", s.str() );
        else
          NewTextChild(variablesNode, "total_trfer_fmt", s.str() );
      };

      s.str("");
      s << setw(39) << (getLocaleText("�ᥣ� ������", rpt_params.GetLang()))
        << "%4u %6u" << endl
        << setw(39) << (getLocaleText("�࠭��୮�� ������", rpt_params.GetLang()))
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
        << setw(6) << (getLocaleText("�ᥣ�", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("������� ����� ����", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << setw(56) << (getLocaleText("�ᥣ�", rpt_params.GetLang()))
        << (getLocaleText("������� ����� ����", rpt_params.GetLang())) << endl;

    s << setw(6)  << (getLocaleText("����", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("���", rpt_params.GetLang()))
      << setw(43) << (getLocaleText("������⢮ ���� �ய����", rpt_params.GetLang()))
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
        case stServiceCode:
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

    // ������ ��६���� ����
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
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(21) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(10) << getLocaleText("� �����", rpt_params.GetLang())
        << setw(24)  << getLocaleText("��稭� ���뫥�", rpt_params.GetLang())
        << setw(16) << getLocaleText("� �/�", rpt_params.GetLang());
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

void EMD(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtEMDTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("EMD", reqNode, resNode);
    std::map<int, std::vector<std::string> > tab;
    size_t total = 0;
    EMDReport(rpt_params.point_id, tab, total);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_emd");

    if(tab.size() != 0) {
        for( std::map<int, std::vector<std::string> >::iterator i = tab.begin(); i != tab.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "reg_no", i->first);
            const vector<string> &fields = i->second;
            for(size_t j = 0; j < fields.size(); j++) {
                switch(j) {
                    case 0:
                        NewTextChild(rowNode, "full_name", transliter(fields[j], 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                        break;
                    case 1:
                        NewTextChild(rowNode, "etkt_no", fields[j]);
                        break;
                    case 2:
                        NewTextChild(rowNode, "emd_no", fields[j]);
                        break;
                }
            }
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no");
        NewTextChild(rowNode, "full_name", getLocaleText("�⮣�:", rpt_params.GetLang()));
        NewTextChild(rowNode, "etkt_no");
        NewTextChild(rowNode, "emd_no", (int)total);
    }

    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.EMD",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void EMDTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EMD(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(37) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(20)  << getLocaleText("� �����", rpt_params.GetLang())
        << setw(20)  << getLocaleText("� EMD", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_emd", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 36, rows);
        fields["full_name"]=rows;

        SeparateString(NodeAsString("emd_no", rowNode), 19, rows);
        fields["emd_no"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(36) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(19) << (row == 0 ? NodeAsString("etkt_no", rowNode) : "") << " " << col_sym
                << left << setw(19) << (!fields["emd_no"].empty() ? *(fields["emd_no"].begin()) : "");

            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() or
                !fields["emd_no"].empty()
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
        "       pax.*, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no, "
        "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bagAmount, "
        "       ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags "
        "FROM   pax_grp,pax "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pax.pr_brd=0 and "
        "       point_dep = :point_id and "
        "       pax_grp.status NOT IN ('E') "
        "order by ";
    switch(rpt_params.sort) {
        case stServiceCode:
        case stRegNo:
            SQLText += " pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            SQLText += " pax.surname, pax.name, pax.reg_no, pax.seats DESC ";
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
      CheckIn::TSimplePaxItem pax;
      pax.fromDB(Qry);
      xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

      NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
      NewTextChild(rowNode, "reg_no", pax.reg_no);
      NewTextChild(rowNode, "family", transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
      NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
      NewTextChild(rowNode, "seat_no", pax.seat_no);
      NewTextChild(rowNode, "bagamount", Qry.FieldAsInteger("bagamount"));
      NewTextChild(rowNode, "tags", Qry.FieldAsString("tags"));

      Qry.Next();
    }

    // ������ ��६���� ����
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
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(38) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(8)  << getLocaleText("� �", rpt_params.GetLang())
        << setw(6) << getLocaleText("���.", rpt_params.GetLang())
        << " " << setw(19) << getLocaleText("� �/�", rpt_params.GetLang());
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
                << right <<  setw(3) << (row == 0 ? NodeAsString("pers_type", rowNode, "��") : "") << " " << col_sym
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

bool getPaxRem(const string &lang, const CheckIn::TPaxRemBasic &basic, bool inf_indicator, CheckIn::TPaxRemItem &rem)
{
  if (basic.empty()) return false;
  rem=CheckIn::TPaxRemItem(basic, inf_indicator, lang, applyLangForTranslit, CheckIn::TPaxRemBasic::outputReport);
  return true;
}

namespace REPORT_PAX_REMS {
    void get(TQuery &Qry, const string &lang, const map< TRemCategory, vector<string> > &filter, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        CheckIn::TPaxTknItem tkn;
        CheckIn::TPaxDocItem doc;
        CheckIn::TPaxDocoItem doco;
        CheckIn::TDocaMap doca_map;
        set<CheckIn::TPaxFQTItem> fqts;
        vector<CheckIn::TPaxASVCItem> asvc;
        multiset<CheckIn::TPaxRemItem> rems;
        map< TRemCategory, bool > cats;
        cats[remTKN]=false;
        cats[remDOC]=false;
        cats[remDOCO]=false;
        cats[remDOCA]=false;
        cats[remFQT]=false;
        cats[remASVC]=false;
        cats[remUnknown]=false;
        int pax_id=Qry.FieldAsInteger("pax_id");
        bool inf_indicator=DecodePerson(Qry.FieldAsString("pers_type"))==ASTRA::baby && Qry.FieldAsInteger("seats")==0;
        bool pr_find=true;
        if (!filter.empty())
        {
            pr_find=false;
            //䨫��� �� ������� ६�ઠ�
            map< TRemCategory, vector<string> >::const_iterator iRem=filter.begin();
            for(; iRem!=filter.end(); iRem++)
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
                            if (LoadPaxDoca(pax_id, doca_map)) pr_find=true;
                            cats[remDOCA]=true;
                        };
                        break;
                    case remFQT:
                        LoadPaxFQT(pax_id, fqts);
                        for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();f++)
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
                    case remASVC:
                        if (find(iRem->second.begin(),iRem->second.end(),"ASVC")!=iRem->second.end())
                        {
                            if (LoadPaxASVC(pax_id, asvc)) pr_find=true;
                            cats[remASVC]=true;
                        };
                        break;
                    default:
                        LoadPaxRem(pax_id, rems);
                        for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin();r!=rems.end();r++)
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
        };

        if(pr_find) {
            CheckIn::TPaxRemItem rem;
            //����� ६�ન (��易⥫쭮 ��ࠡ��뢠�� ���묨)
            if (!cats[remUnknown]) LoadPaxRem(pax_id, rems);
            for(multiset<CheckIn::TPaxRemItem>::iterator r=rems.begin();r!=rems.end();++r)
                if (getPaxRem(lang, *r, inf_indicator, rem)) final_rems.insert(rem);

            //�����
            if (!cats[remTKN]) tkn.fromDB(Qry);
            if (getPaxRem(lang, tkn, inf_indicator, rem)) final_rems.insert(rem);
            //���㬥��
            if (!cats[remDOC]) LoadPaxDoc(pax_id, doc);
            if (getPaxRem(lang, doc, inf_indicator, rem)) final_rems.insert(rem);
            //����
            if (!cats[remDOCO]) LoadPaxDoco(pax_id, doco);
            if (getPaxRem(lang, doco, inf_indicator, rem)) final_rems.insert(rem);
            //����
            if (!cats[remDOCA]) LoadPaxDoca(pax_id, doca_map);
            for(CheckIn::TDocaMap::const_iterator d = doca_map.begin(); d != doca_map.end(); ++d)
                if (getPaxRem(lang, d->second, inf_indicator, rem)) final_rems.insert(rem);
            //�����-�ணࠬ��
            if (!cats[remFQT]) LoadPaxFQT(pax_id, fqts);
            for(set<CheckIn::TPaxFQTItem>::const_iterator f=fqts.begin();f!=fqts.end();++f)
                if (getPaxRem(lang, *f, inf_indicator, rem)) final_rems.insert(rem);
            //��㣨
            if (!cats[remASVC]) LoadPaxASVC(pax_id, asvc);
            for(vector<CheckIn::TPaxASVCItem>::const_iterator a=asvc.begin();a!=asvc.end();++a)
                if (getPaxRem(lang, *a, inf_indicator, rem)) final_rems.insert(rem);
        }
    }

    void get(TQuery &Qry, const string &lang, multiset<CheckIn::TPaxRemItem> &final_rems)
    {
        return get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
    }

    void get_rem_codes(TQuery &Qry, const string &lang, set<string> &rem_codes)
    {
        multiset<CheckIn::TPaxRemItem> final_rems;
        get(Qry, lang, map< TRemCategory, vector<string> >(), final_rems);
        for(multiset<CheckIn::TPaxRemItem>::iterator i = final_rems.begin(); i != final_rems.end(); i++)
            rem_codes.insert(i->code);
    }
}

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
        "       pax.*, "
        "       salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no "
        "FROM   pax_grp,pax "
        "WHERE  pax_grp.grp_id=pax.grp_id AND "
        "       pr_brd IS NOT NULL and "
        "       point_dep = :point_id and "
        "       pax_grp.status NOT IN ('E') "
        "order by ";
    switch(rpt_params.sort) {
        case stServiceCode:
        case stRegNo:
            SQLText += " pax.reg_no, pax.seats DESC ";
            break;
        case stSurname:
            SQLText += " pax.surname, pax.name, pax.reg_no, pax.seats DESC ";
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
      CheckIn::TSimplePaxItem pax;
      pax.fromDB(Qry);

      multiset<CheckIn::TPaxRemItem> final_rems;
      REPORT_PAX_REMS::get(Qry, rpt_params.GetLang(), rpt_params.rems, final_rems);

      if(rpt_params.rpt_type == rtSPEC or rpt_params.rpt_type == rtSPECTXT)
      {
        for(multiset<CheckIn::TPaxRemItem>::iterator r=final_rems.begin();r!=final_rems.end();)
        {
          if (!spec_rems.exists(r->code)) r=Erase(final_rems, r); else ++r;
        };
      }

      if (final_rems.empty()) continue;

      xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
      NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
      NewTextChild(rowNode, "reg_no", pax.reg_no);
      NewTextChild(rowNode, "family", transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
      NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
      NewTextChild(rowNode, "seat_no", pax.seat_no);

      ostringstream rem_info;
      for(multiset<CheckIn::TPaxRemItem>::const_iterator r=final_rems.begin();r!=final_rems.end();++r)
      {
        rem_info << ".R/" << r->text << " ";
      };
      NewTextChild(rowNode, "info", rem_info.str());
    }

    // ������ ��६���� ����
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
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(38) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(8)  << getLocaleText("� �", rpt_params.GetLang())
        << setw(25) << getLocaleText("����ન", rpt_params.GetLang());
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
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "��") : "") << col_sym
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

void WB_MSG(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    TCachedQuery Qry("select id, time_receive from wb_msg where point_id = :point_id and msg_type = :msg_type order by id desc",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id)
            << QParam("msg_type", otString, EncodeRptType(rpt_params.rpt_type))
            );
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw UserException("MSG.NOT_DATA");

    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    int id = Qry.get().FieldAsInteger("id");
    // TDateTime time_receive = Qry.get().FieldAsDateTime("time_receive");
    TCachedQuery txtQry("select text from wb_msg_text where id = :id order by page_no",
            QParams() << QParam("id", otInteger, id));
    txtQry.get().Execute();
    string text;
    for(; not txtQry.get().Eof; txtQry.get().Next())
        text += txtQry.get().FieldAsString("text");
    NewTextChild(rowNode, "text", text);

    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
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
        case stServiceCode:
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
    //६�ન ���ᠦ�஢

    TRemGrp rem_grp;
    if(rpt_params.rpt_type != rtBDOCS)
      rem_grp.Load(retPNL_SEL, rpt_params.point_id);
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
            string pnr_addr=TPnrAddrs().firstAddrByPnrId(Qry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrOnly); //���� �� ���� �뢮���� ��������, ����� ���� ��⮬...
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

    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if(pr_unreg)
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRSUNREG",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRS",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void CRSTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CRS(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << left
        << setw(7)  << "PNR"
        << setw(22) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(3) << getLocaleText("��", rpt_params.GetLang())
        << setw(8)  << getLocaleText("� �", rpt_params.GetLang())
        << setw(4)  << getLocaleText("CAP.DOC.AIRP_ARV", rpt_params.GetLang())
        << setw(7)  << getLocaleText("CAP.DOC.TO", rpt_params.GetLang())
        << setw(10) << getLocaleText("�����", rpt_params.GetLang())
        << setw(10) << getLocaleText("���㬥��", rpt_params.GetLang());
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
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "��") : "") << col_sym
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
    if(!Qry.Eof)
    {
      rem_grp.Load(retBRD_VIEW, rpt_params.point_id);

      bool check_pay_on_tckin_segs=false;
      TTripInfo fltInfo;
      if (fltInfo.getByPointId(rpt_params.point_id))
        check_pay_on_tckin_segs=GetTripSets(tsCheckPayOnTCkinSegs, fltInfo);

      TComplexBagExcessNodeList excessNodeList(false, OutputLang(rpt_params.GetLang()), false, "+");
      for( ; !Qry.Eof; Qry.Next()) {
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry);

        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        int grp_id = Qry.FieldAsInteger("grp_id");
        NewTextChild(paxNode, "reg_no", pax.reg_no);
        NewTextChild(paxNode, "surname", transliter(pax.surname, 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(paxNode, "name", transliter(pax.name, 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        if(pr_web)
          NewTextChild(paxNode, "user_descr", transliter(Qry.FieldAsString("user_descr"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(paxNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, EncodePerson(pax.pers_type), efmtCodeNative));
        NewTextChild(paxNode, "pr_exam", (int)pax.pr_exam, (int)false);
        NewTextChild(paxNode, "pr_brd", (int)pax.pr_brd, (int)false);
        NewTextChild(paxNode, "seat_no", pax.seat_no);
        NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(NoExists, pax.id, false, rpt_params.GetLang()));
        NewTextChild(paxNode, "ticket_no", pax.tkn.no);
        NewTextChild(paxNode, "coupon_no", pax.tkn.coupon);
        NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
        NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
        NewTextChild(paxNode, "rk_amount", Qry.FieldAsInteger("rk_amount"));
        NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
        excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger("excess_pc")),
                                              TBagKilos(Qry.FieldAsInteger("excess_wt")));
        bool pr_payment=RFISCPaymentCompleted(grp_id, pax.id, check_pay_on_tckin_segs) &&
                        WeightConcept::BagPaymentCompleted(grp_id);
        NewTextChild(paxNode, "pr_payment", (int)pr_payment);
        NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
        NewTextChild(paxNode, "remarks", GetRemarkStr(rem_grp, pax.id, rpt_params.GetLang()));
      }
    }

    // ������ ��६���� ����
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
    NewTextChild(variablesNode, "kg", getLocaleText("��", rpt_params.GetLang()));
    NewTextChild(variablesNode, "pc", getLocaleText("�", rpt_params.GetLang()));
}

void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EXAM(rpt_params, reqNode, resNode);
    const char col_sym = ' '; //ᨬ��� ࠧ����⥫� �⮫�殢
    bool pr_web = rpt_params.rpt_type == rtWEBTXT;

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
        << right << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << col_sym
        << left << setw(pr_web ? 19 : 30) << getLocaleText("�������", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("������", rpt_params.GetLang());
    s << setw(4)  << getLocaleText("���", rpt_params.GetLang());
    if(pr_web)
        s
            << setw(9)  << getLocaleText("� �", rpt_params.GetLang());
    else
        s
            << setw(3)  << getLocaleText("��", rpt_params.GetLang())
            << setw(4)  << getLocaleText("��", rpt_params.GetLang());
    s
        << setw(11) << getLocaleText("���㬥��", rpt_params.GetLang())
        << setw(14) << getLocaleText("�����", rpt_params.GetLang())
        << setw(10) << getLocaleText("� �/�", rpt_params.GetLang());
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
        //ࠡ����� 䠬����, ���㬥��, �����, ��ન, ६�ન
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

// VOUCHERS BEGIN

void VOUCHERS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("vouchers", reqNode, resNode);

    TCachedQuery Qry(
            "select "
            "    confirm_print.pax_id, "
            "    pax.surname||' '||pax.name AS full_name, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.ticket_no, "
            "    pax.coupon_no, "
            "    pax.ticket_rem, "
            "    pax.ticket_confirm, "
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher, "
            "    count(*) total "
            "from "
            "    pax_grp, "
            "    pax, "
            "    confirm_print "
            "where "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.grp_id = pax.grp_id and "
            "    pax.pax_id = confirm_print.pax_id and "
            "    confirm_print.voucher is not null and "
            "    confirm_print.pr_print <> 0 "
            "group by "
            "    confirm_print.pax_id, "
            "    pax.surname||' '||pax.name, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.ticket_no, "
            "    pax.coupon_no, "
            "    pax.ticket_rem, "
            "    pax.ticket_confirm, "
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher "
            "order by "
            "    confirm_print.voucher, "
            "    pax.reg_no ",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id));
    Qry.get().Execute();

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    // ��६���� �����
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // ��������� �����
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.VOUCHERS",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    if(not Qry.get().Eof) {
        xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_vouchers");
        // ��ப� �����

        map<string, int> totals;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string voucher = Qry.get().FieldAsString("voucher");
            int reg_no = Qry.get().FieldAsInteger("reg_no");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            string category = rpt_params.ElemIdToReportElem(etVoucherType, voucher, efmtNameLong);
            NewTextChild(rowNode, "category", category);
            NewTextChild(rowNode, "reg_no", reg_no);
            NewTextChild(rowNode, "fio", transliter(Qry.get().FieldAsString("full_name"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "type", getLocaleText(Qry.get().FieldAsString("pers_type"), rpt_params.GetLang()));

            string ticket_no = Qry.get().FieldAsString("ticket_no");
            string coupon_no = Qry.get().FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            NewTextChild(rowNode, "tick_no", ticket.str());

            int quantity = Qry.get().FieldAsInteger("total");
            totals[category] += quantity;
            NewTextChild(rowNode, "quantity", quantity);

            set<string> rems;
            REPORT_PAX_REMS::get_rem_codes(Qry.get(), rpt_params.GetLang(), rems);
            ostringstream rems_list;
            for(set<string>::iterator i = rems.begin(); i != rems.end(); i++) {
                if(rems_list.str().size() != 0)
                    rems_list << " ";
                rems_list << *i;
            }
            NewTextChild(rowNode, "rem", rems_list.str());
        }
        dataSetNode = NewTextChild(dataSetsNode, "totals");
        int total_sum = 0;
        for(map<string, int>::iterator i = totals.begin(); i != totals.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            ostringstream buf;
            buf << i->first << " " << i->second;
            total_sum += i->second;
            NewTextChild(rowNode, "item", buf.str());
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        ostringstream buf;
        buf << getLocaleText("�ᥣ�:", rpt_params.GetLang()) << " " << total_sum;
        NewTextChild(rowNode, "item", buf.str());
    }

    LogTrace(TRACE5) << GetXMLDocText(resNode->doc); //!!!
}

// VOUCHERS END

// SERVICES BEGIN

enum EServiceSortOrder
{
    by_reg_no,
    by_family,
    by_seat_no,
    by_service_code
};

struct TServiceRow
{
    string seat_no;
    string family;
    int reg_no;
    string RFIC;
    string RFISC;
    string desc;
    string num;

    const EServiceSortOrder mSortOrder;

    TServiceRow(EServiceSortOrder sortOrder = by_reg_no)
        : reg_no(NoExists), mSortOrder(sortOrder) {}

    bool operator < (const TServiceRow &other) const
    {
        switch (mSortOrder)
        {
            case by_reg_no: return reg_no < other.reg_no;
            case by_family: return family < other.family;
            case by_seat_no: return seat_no < other.seat_no;
            case by_service_code: return RFISC < other.RFISC;
            default: throw Exception("TServiceRow::operator < : unexpected value");
        }
    }

    void toXML(xmlNodePtr dataSetNode) const
    {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "seat_no", seat_no);
        NewTextChild(rowNode, "family", family);
        NewTextChild(rowNode, "reg_no", reg_no);
        NewTextChild(rowNode, "RFIC", RFIC);
        NewTextChild(rowNode, "RFISC", RFISC);
        NewTextChild(rowNode, "desc", desc);
        NewTextChild(rowNode, "num", num);
    }
};

class TServiceFilter
{
    set<string> filterIncludeRFIC;
    set<string> filterExcludeRFIC;
public:
    void AddRFIC(string RFIC) { filterIncludeRFIC.insert(RFIC); }
    void ExcludeRFIC(string RFIC) { filterExcludeRFIC.insert(RFIC); }
    bool Check(const TServiceRow& row) const
    {
        if (filterIncludeRFIC.empty()) { if (filterExcludeRFIC.count(row.RFIC)) return false; else return true; }
        if (filterIncludeRFIC.count(row.RFIC)) return true; else return false;
    }
};

void SERVICES(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtSERVICESTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("services", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_services");
    // ��६���� �����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // ��������� �����
    NewTextChild(variablesNode, "caption",
        getLocaleText("CAP.DOC.SERVICES", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    // ��ப� �����
    TQuery Qry(&OraSession);
    string SQLText =
    "select "
    "    pax.*, "
    "    salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,'_seats',rownum,:pr_lat) AS seat_no "
    "from "
    "    pax_grp, "
    "    pax "
    "where "
    "    pax_grp.point_dep = :point_id and "
    "    pax_grp.grp_id = pax.grp_id ";
    /*"order by "
    "    seat_no, pax.reg_no DESC ";*/
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.CreateVariable("pr_lat", otInteger, rpt_params.IsInter());
    Qry.Execute();
    list<TServiceRow> rows;
    //  ���樠������ ���஢��
    EServiceSortOrder sortOrder = by_reg_no;
    switch(rpt_params.sort)
    {
        case stRegNo: sortOrder = by_reg_no; break;
        case stSurname: sortOrder = by_family; break;
        case stSeatNo: sortOrder = by_seat_no; break;
        case stServiceCode: sortOrder = by_service_code; break;
    }
    //  ���樠������ 䨫���
    TServiceFilter filter;
    if (TReqInfo::Instance()->desk.compatible( RFIC_FILTER_VERSION ))
    for (list<string>::const_iterator iRFIC = rpt_params.rfic.begin(); iRFIC != rpt_params.rfic.end(); ++iRFIC) filter.AddRFIC(*iRFIC);
    else filter.ExcludeRFIC("C"); // ��� ����� �ନ����� � ���� ������ �������� �� ��㣨, �஬� RFIC=C
    //  横� ��� ������� ���� � �롮થ
    for (; !Qry.Eof; Qry.Next())
    {
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry);
        int grp_id = Qry.FieldAsInteger("grp_id");
        //  ������� ᯨ᮪ ��� ��� ����
        TPaidRFISCListWithAuto prList;
        prList.fromDB(pax.id, false);
        //  ������� ᯨ᮪ ���⠭権 ��� ��㯯�
        CheckIn::TServicePaymentListWithAuto spList;
        spList.fromDB(grp_id);
        spList.sort();
        //  横� ��� ������ ��㣨
        for (TPaidRFISCListWithAuto::const_iterator pr = prList.begin(); pr != prList.end(); ++pr)
        {
            const TPaidRFISCItem item = pr->second;
            //  ��ᬠ�ਢ��� ⮫쪮 ���� ᥣ���� �࠭���
            if (item.trfer_num != 0) continue;
            //  横� �� ��������� ��㣠�
            for (int service = 0; service < item.service_quantity; ++service)
            {
                TServiceRow row(sortOrder); // sortOrder ��� ��� ��ப � ���⥩��� ������ ���� ��������!
                //  ���� � ᠫ���
                row.seat_no = pax.seat_no;
                //  ��� ���ᠦ��
                row.family = transliter(pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
                //  ���. �
                row.reg_no = pax.reg_no;
                if (item.list_item)
                {
                    //  RFIC
                    row.RFIC = item.list_item->RFIC;
                    //  ��� ��㣨
                    row.RFISC = item.list_item->RFISC;
                    //  ���ᠭ��
                    row.desc = prList.getRFISCName(item, rpt_params.GetLang());
                }
                //  横� �� �ᥬ ���⠭�� ��㯯�
                for (CheckIn::TServicePaymentListWithAuto::iterator sp = spList.begin(); sp != spList.end(); ++ sp)
                {
                    /* ���� ⮫쪮 EMD */
                    if (sp->pc &&
                        sp->pax_id != NoExists &&
                        item.pax_id != NoExists &&
                        sp->pax_id == item.pax_id &&
                        sp->trfer_num == item.trfer_num &&
                        sp->pc->key() == item.key())
                    {
                        //  ����� ���⠭樨 (������� ��ࢠ� ���室���)
                        row.num = sp->no_str();
                        spList.erase(sp);
                        break;
                    }
                }
                if (filter.Check(row)) rows.push_back(row);
            }
        }
    }
    rows.sort();
    for (list<TServiceRow>::const_iterator irow = rows.begin(); irow != rows.end(); ++irow)
        irow->toXML(dataSetNode);
    //  LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}

void SERVICESTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    SERVICES(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
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
    s   << left
        << setw(8)  << getLocaleText("� �", rpt_params.GetLang())
        << setw(20) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(3)  << getLocaleText("�", rpt_params.GetLang()) << " "
        << setw(5)  << getLocaleText("RFIC", rpt_params.GetLang())
        << setw(5)  << getLocaleText("���", rpt_params.GetLang())
        << setw(20)  << getLocaleText("���ᠭ��", rpt_params.GetLang())
        << setw(20) << getLocaleText("� ���⠭樨", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_services", dataSetsNode);
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
        SeparateString(NodeAsString("desc", rowNode), 19, rows);
        fields["desc"]=rows;
        SeparateString(NodeAsString("num", rowNode), 19, rows);
        fields["num"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s   << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << setw(19) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << setw(4) << (row == 0 ? NodeAsString("RFIC", rowNode) : "") << col_sym
                << setw(4) << (row == 0 ? NodeAsString("RFISC", rowNode) : "") << col_sym
                << setw(19) << (!fields["desc"].empty() ? *(fields["desc"].begin()) : "") << col_sym
                << setw(19) << (!fields["num"].empty() ? *(fields["num"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() ||
                !fields["desc"].empty() ||
                !fields["num"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

// SERVICES END


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
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
}

void RESEAT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtRESEATTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("reseat", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_reseat");
    // ��६���� �����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // ��������� �����
    NewTextChild(variablesNode, "caption",
        getLocaleText("CAP.DOC.RESEAT", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    map<bool,map <int,TSeatRanges> > seats; // true - ����, false - ���� ����
    SALONS2::getPaxSeatsWL(rpt_params.point_id, seats);

    // �ਢ���� seats � ��ଠ�쭮�� ���� prepared_seats
    map<int, pair<string, string> > prepared_seats;

    // �஡�� �� ���ᠬ � ���묨 ���⠬� (� �� seats[true])
    for(map <int,TSeatRanges>::iterator old_seats = seats[true].begin();
            old_seats != seats[true].end(); old_seats++) {

        pair<string, string> &pair_seats = prepared_seats[old_seats->first];

        // ���� ���� �뢮���
        pair_seats.first = GetSeatRangeView(old_seats->second, "list", rpt_params.GetLang() != AstraLocale::LANG_RU);

        // �饬, ���� �� � ���� ���� ���� (� �� seats[false])
        // �᫨ ����, �뢮���
        map<int,TSeatRanges>::iterator new_seats = seats[false].find(old_seats->first);
        if(new_seats != seats[false].end())
            pair_seats.second = GetSeatRangeView(new_seats->second, "list", rpt_params.GetLang() != AstraLocale::LANG_RU);
    }

    struct TSortedPax {
        CheckIn::TSimplePaxItem pax;
        pair<string, string> seats;
    };

    map<int, TSortedPax> sorted_pax;
    for(map<int, pair<string, string> >::iterator i = prepared_seats.begin();
            i != prepared_seats.end(); i++) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(i->first);
        sorted_pax[pax.reg_no].pax = pax;
        sorted_pax[pax.reg_no].seats = i->second;
    }

    // ��ப� �����
    map<int, string> classes;
    for(map<int, TSortedPax>::iterator i = sorted_pax.begin();
            i != sorted_pax.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", i->second.pax.reg_no);
        NewTextChild(rowNode, "full_name", transliter(i->second.pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "pr_brd", i->second.pax.pr_brd);
        NewTextChild(rowNode, "seats", i->second.pax.seats);
        NewTextChild(rowNode, "old_seat_no", i->second.seats.first);
        NewTextChild(rowNode, "new_seat_no", i->second.seats.second);

        map<int, string>::iterator i_cls = classes.find(i->second.pax.grp_id);
        if(i_cls == classes.end()) {
            CheckIn::TSimplePaxGrpItem grp;
            grp.getByGrpId(i->second.pax.grp_id);
            pair<map<int, string>::iterator, bool> ret;
            ret = classes.insert(make_pair(i->second.pax.grp_id, grp.cl));
            i_cls = ret.first;
        }
        NewTextChild(rowNode, "cls", rpt_params.ElemIdToReportElem(etClass, i_cls->second, efmtCodeNative));

        NewTextChild(rowNode, "document", CheckIn::GetPaxDocStr(NoExists, i->second.pax.id, false, rpt_params.GetLang()));

        ostringstream ticket_no;
        ticket_no << i->second.pax.tkn.no;
        if(i->second.pax.tkn.coupon != NoExists)
            ticket_no << "/" << i->second.pax.tkn.coupon;

        NewTextChild(rowNode, "ticket_no", ticket_no.str());
    }
}

void RESEATTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    RESEAT(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, page_width);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << left
        << setw(3)  << getLocaleText("�", rpt_params.GetLang())
        << setw(26) << getLocaleText("�.�.�.", rpt_params.GetLang())
        << setw(4)  << getLocaleText("��", rpt_params.GetLang())
        << setw(3)  << (getLocaleText("��", rpt_params.GetLang()))
        << setw(4)  << (getLocaleText("���", rpt_params.GetLang()))
        << setw(6)  << (getLocaleText("���.", rpt_params.GetLang()))
        << setw(8)  << (getLocaleText("���.", rpt_params.GetLang()))
        << setw(11) << getLocaleText("���㬥��", rpt_params.GetLang())
        << setw(15) << getLocaleText("�����", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_reseat", dataSetsNode);
    xmlNodeSetName(dataSetNode, BAD_CAST "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 25, rows);
        fields["full_name"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(2) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(25) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? (strcmp(NodeAsString("pr_brd", rowNode, "0"), "0") == 0 ? "-" : "+") : "") << col_sym
                << left <<  setw(2) << (row == 0 ? NodeAsString("cls", rowNode) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? NodeAsString("seats", rowNode) : "") << col_sym
                << left <<  setw(5) << (row == 0 ? NodeAsString("old_seat_no", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("new_seat_no", rowNode) : "") << col_sym
                << left <<  setw(10) << (row == 0 ? NodeAsString("document", rowNode) : "") << col_sym
                << left <<  setw(15) << (row == 0 ? NodeAsString("ticket_no", rowNode) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(!fields["full_name"].empty());
        NewTextChild(rowNode,"str",s.str());
    }
}

void KOMPLEKT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(not TReqInfo::Instance()->desk.compatible(KOMPLEKT_VERSION))
        throw UserException("MSG.KOMPLEKT_SUPPORT",
                LParams()
                << LParam("rpt", ElemIdToNameLong(etReportType, EncodeRptType(rpt_params.rpt_type)))
                << LParam("vers", KOMPLEKT_VERSION));
  struct TReportItem {
      string code;
      int num;
      int sort_order;
      bool operator < (const TReportItem &other) const
      {
          return sort_order < other.sort_order;
      }
      TReportItem(const TReportItem &other)
      {
          code = other.code;
          num = other.num;
          sort_order = other.sort_order;
      }
      TReportItem(
              const string &_code,
              int _num,
              int _sort_order):
          code(_code),
          num(_num),
          sort_order(_sort_order)
      {}
  };

  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT report_type, "
    "       num, "
    "       report_types.sort_order, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies, report_types "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) and "
    "      doc_num_copies.report_type = report_types.code ";

  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();

  // ����砥� ��� ������� ⨯� ����� �� ��ਠ��� ������⢠
  map< TReportItem, map<int, int> > temp;
  while (!Qry.Eof)
  {
      TReportItem item(
              Qry.FieldAsString("report_type"),
              Qry.FieldAsInteger("num"),
              Qry.FieldAsInteger("sort_order"));
      int priority = Qry.FieldAsInteger("priority");
//    LogTrace(TRACE5) << "report_type=" << report_type
//                     << " priority=" << priority
//                     << " num=" << num;
      temp[item][priority] = item.num;
      Qry.Next();
  }


  // �롨ࠥ� ��� ������� ⨯� ����� ������⢮ � ������訬 �ਮ��⮬
  set<TReportItem> nums;
  for (auto r : temp) {
      TReportItem item(r.first);
      item.num = r.second.rbegin()->second;
      nums.insert(item);
  }
  // �ନ஢���� �����
  get_compatible_report_form("komplekt", reqNode, resNode);
  xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
  xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
  xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_komplekt");
  // ��६���� �����
  xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
  PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
  // ��������� �����
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.KOMPLEKT", rpt_params.GetLang()));
  populate_doc_cap(variablesNode, rpt_params.GetLang());
  NewTextChild(variablesNode, "doc_cap_komplekt",
      getLocaleText("CAP.DOC.KOMPLEKT_HEADER",
                    LParams() << LParam("airline", NodeAsString("airline_name", variablesNode))
                              << LParam("flight", get_flight(variablesNode))
                              << LParam("route", NodeAsString("long_route", variablesNode)),
                    rpt_params.GetLang()));
  NewTextChild(variablesNode, "komplekt_empty", (nums.empty() ? getLocaleText("MSG.KOMPLEKT_EMPTY", rpt_params.GetLang()) : ""));
  for (auto r : nums)
  {
    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "code", r.code);
    NewTextChild(rowNode, "name", ElemIdToNameLong(etReportType, r.code));
    NewTextChild(rowNode, "num", r.num);
  }
  LogTrace(TRACE5) << GetXMLDocText(resNode->doc); //!!!
}

int GetNumCopies(TRptParams rpt_params)
{
  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies "
    "WHERE report_type = :report_type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("report_type",otString,EncodeRptType(rpt_params.orig_rpt_type));
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  return Qry.Eof ? NoExists : Qry.FieldAsInteger("num");
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
        case rtEMD:
            EMD(rpt_params, reqNode, resNode);
            break;
        case rtEMDTXT:
            EMDTXT(rpt_params, reqNode, resNode);
            break;
        case rtLOADSHEET:
        case rtNOTOC:
        case rtLIR:
            WB_MSG(rpt_params, reqNode, resNode);
            break;
        case rtANNUL_TAGS:
            ANNUL_TAGS(rpt_params, reqNode, resNode);
            break;
        case rtVOUCHERS:
            VOUCHERS(rpt_params, reqNode, resNode);
            break;
        case rtSERVICES:
            SERVICES(rpt_params, reqNode, resNode);
            break;
        case rtSERVICESTXT:
            SERVICESTXT(rpt_params, reqNode, resNode);
            break;
        case rtRESEAT:
            RESEAT(rpt_params, reqNode, resNode);
            break;
        case rtRESEATTXT:
            RESEATTXT(rpt_params, reqNode, resNode);
            break;
        case rtKOMPLEKT:
            KOMPLEKT(rpt_params, reqNode, resNode);
            break;
        default:
            throw AstraLocale::UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
    }
    NewTextChild(resNode, "copies", GetNumCopies(rpt_params), NoExists);
}

void  DocsInterface::LogPrintEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int copies = NodeAsInteger("copies", reqNode, NoExists);
    int printed_copies = NodeAsInteger("printed_copies", reqNode, NoExists);
    if(printed_copies == NoExists) { // ���� ����� �ନ���� �� ���뫠�� ⥣ printed_copies
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT", LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", ""),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    } else {
        ostringstream str;
        str << getLocaleText("�����⠭� �����") << ": " << printed_copies;
        if(copies != NoExists and copies != printed_copies)
            str << "; " << getLocaleText("������ �����") << ": " << copies;
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT",
                LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", str.str()),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    }
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
        result[0] = ALL_CKIN_ZONES; // ��㯯� ����� "�� ����"
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
            NewTextChild(itemNode, "name", getLocaleText("��. ����"));
        } else if(*iv == ALL_CKIN_ZONES) {
            NewTextChild(itemNode, "code", *iv);
            NewTextChild(itemNode, "name");
        } else {
            NewTextChild(itemNode, "code", *iv);
            NewTextChild(itemNode, "name", *iv);
        }
    }
}

void DocsInterface::print_komplekt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr codesNode = NodeAsNode("codes", reqNode);
    codesNode = codesNode->children;
    xmlNodePtr reportListNode = NULL;
    for(; codesNode; codesNode = codesNode->next) {
        xmlNodePtr currNode = codesNode->children;
        string code = NodeAsStringFast("code", currNode);

        xmlNodePtr LoadFormNode = GetNodeFast("LoadForm", currNode);
        xmlNodePtr reqLoadFormNode = GetNode("LoadForm", reqNode);

        if(reqLoadFormNode) {
            if(not LoadFormNode)
                RemoveNode(reqLoadFormNode);
        } else {
            if(LoadFormNode)
                NewTextChild(reqNode, "LoadForm");
        }

        xmlNodePtr rptTypeNode = GetNode("rpt_type", reqNode);
        if(rptTypeNode)
            NodeSetContent(rptTypeNode, code);
        else
            NewTextChild(reqNode, "rpt_type", code);

        // ������� �����, ����. GOSHO �ॡ���, �⮡� �������
        // XML �뫠 ᫥�����: /term/answer
        XMLDoc doc("term");
        xmlNodePtr reportNode = doc.docPtr()->children;
        xmlNodePtr answerNode = NewTextChild(reportNode, "answer");
        SetProp(reportNode, "code", code);

        try {
            RunReport2(ctxt, reqNode, answerNode);

            if(not reportListNode)
                reportListNode = NewTextChild(resNode, "report_list");

            CopyNode(reportListNode, reportNode);
        } catch(Exception &E) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << code << " failed: " << E.what();
        }

        // break;
    }
    LogTrace(TRACE5) << GetXMLDocText(resNode->doc); //!!!
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
    reqInfo->Initialize("���");
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
    // ������� � �⢥� 蠡��� ����
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
    get_new_report_form(name, reqNode, resNode);
}

void get_new_report_form(const string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(GetNode("LoadForm", reqNode) == NULL)
        return;
    get_report_form(name, resNode);
}
