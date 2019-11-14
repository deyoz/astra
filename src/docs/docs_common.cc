#include "docs_common.h"
#include "astra_date_time.h"
#include "stat/stat_utils.h"
#include "season.h"
#include "aodb.h"
#include <boost/shared_array.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace ASTRA::date_time;
using namespace EXCEPTIONS;

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
            // у этих отчетов нет текстового варианта
            // При формировании отчетов из-под DCP CUTE, передается тег text = 1
            // Хотя он visibl = false в терминале
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
            // Если не было выбрано ремарок для тек. группы, добавляем все ремарки тек. группы
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

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};

string TRptParams::ElemIdToReportElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
{
  if (id==ASTRA::NoExists) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};

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
        // сначала airline_name, потом airline
        // иначе в лат варианте будет конвертится уже сконверченный airline
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
    NewTextChild(variablesNode, "landscape", rpt_params.rpt_type == rtBDOCSTXT ? 1 : 0);
}

string get_flight(xmlNodePtr variablesNode)
{
    return (string)NodeAsString("trip", variablesNode) + "/" + NodeAsString("scd_out", variablesNode);
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

void populate_doc_cap(xmlNodePtr variablesNode, string lang)
{
    NewTextChild(variablesNode, "doc_cap_pax", getLocaleText("Пассажир", lang));
    NewTextChild(variablesNode, "doc_cap_issue_country", getLocaleText("Гос-во выдачи", lang));
    NewTextChild(variablesNode, "doc_cap_number", getLocaleText("Номер", lang));
    NewTextChild(variablesNode, "doc_cap_nation", getLocaleText("Граж.", lang));
    NewTextChild(variablesNode, "doc_cap_birth", getLocaleText("CAP.PAX_DOC.BIRTH_DATE", lang));
    NewTextChild(variablesNode, "doc_cap_sex", getLocaleText("Пол", lang));
    NewTextChild(variablesNode, "doc_cap_expiry", getLocaleText("Оконч. действия", lang));
    NewTextChild(variablesNode, "doc_cap_first_name", getLocaleText("Имя", lang));
    NewTextChild(variablesNode, "doc_cap_second_name", getLocaleText("Отчество", lang));

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
    NewTextChild(variablesNode, "doc_cap_old_seat_no", getLocaleText("Стар.", lang));
    NewTextChild(variablesNode, "doc_cap_new_seat_no", getLocaleText("Нов.", lang));
    NewTextChild(variablesNode, "doc_cap_remarks", getLocaleText("Ремарки", lang));
    NewTextChild(variablesNode, "doc_cap_cl", getLocaleText("Кл", lang));
    NewTextChild(variablesNode, "doc_cap_seats", getLocaleText("Крс", lang));
    NewTextChild(variablesNode, "doc_cap_dest", getLocaleText("П/н", lang));
    NewTextChild(variablesNode, "doc_cap_to", getLocaleText("CAP.DOC.TO", lang));
    NewTextChild(variablesNode, "doc_cap_ex", getLocaleText("Дс", lang));
    NewTextChild(variablesNode, "doc_cap_brd", getLocaleText("Пс", lang));
    NewTextChild(variablesNode, "doc_cap_test", (STAT::bad_client_img_version() and not get_test_server()) ? " " : getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "doc_cap_user_descr", getLocaleText("Оператор", lang));
    NewTextChild(variablesNode, "doc_cap_emd_no", getLocaleText("№ EMD", lang));
    NewTextChild(variablesNode, "doc_cap_total", getLocaleText("Итого:", lang));
    NewTextChild(variablesNode, "doc_cap_overall", getLocaleText("Всего:", lang));
    NewTextChild(variablesNode, "doc_cap_vou_quantity", getLocaleText("Кол-во ваучеров", lang));
    NewTextChild(variablesNode, "doc_cap_descr", getLocaleText("Описание", lang));
    NewTextChild(variablesNode, "doc_cap_rcpt_no", getLocaleText("№ квитанции", lang));
    NewTextChild(variablesNode, "doc_cap_service_code", getLocaleText("Код услуги", lang));
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

void GetZoneList(int point_id, xmlNodePtr dataNode)
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
    boost::shared_array<char> data (new char[len]);
    Qry.FieldAsLong("form", data.get());
    form.clear();
    form.append(data.get(), len);

    xmlNodePtr formNode = ReplaceTextChild(resNode, "form", form);
    SetProp(formNode, "name", name);
    SetProp(formNode, "version", version);
    if (Qry.FieldAsInteger("pr_locale") != 0)
        SetProp(formNode, "pr_locale");
}

void get_new_report_form(const string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(GetNode("LoadForm", reqNode) == NULL)
        return;
    get_report_form(name, resNode);
}

void get_compatible_report_form(const string name, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_new_report_form(name, reqNode, resNode);
}

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

