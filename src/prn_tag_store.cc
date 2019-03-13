#include "prn_tag_store.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "xml_unit.h"
#include "misc.h"
#include "docs/docs_common.h"
#include "tripinfo.h"
#include "passenger.h"
#include "qrys.h"
#include "seats_utils.h"
#include "franchise.h"
#include <serverlib/str_utils.h>
#include <serverlib/testmode.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace AstraLocale;

const int POINT_INFO = 1;
const int PAX_INFO = 2;
//const int BAG_INFO = 4; не используется с момента привязки конкретного пассажира к багажу
const int BRD_INFO = 8;
const int FQT_INFO = 16;
const int PNR_INFO = 32;
const int RSTATION_INFO = 64;
const int REM_INFO = 128;

bool TTagLang::IsInter(const TTrferRoute &aroute, string &country)
{
    country.clear();
    string tmp_country;
    // 1-й эл-т вектора содержит данные из pax_grp, а нас интересуют только данные transfer
    // поэтому его не анализируем.
    for(TTrferRoute::const_iterator ir = aroute.begin(); ir != aroute.end(); ++ir) {
        if(ir == aroute.begin()) continue;
        if(IsTrferInter(ir->operFlt.airp, ir->airp_arv, country))
            return true;
        if(tmp_country.empty())
            tmp_country = country;
        else if(tmp_country != country) {
            country.clear();
            return true;
        }
    }
    return false;
}

void TPrnTagStore::set_print_mode(int val)
{
    print_mode = val;
}

void TTagLang::Init(const TBagReceipt &arcpt, bool apr_lat) // Bag receipts
{
    pr_lat = apr_lat;
    is_inter = arcpt.is_inter or apr_lat;
    desk_lang = arcpt.desk_lang;
}

void TTagLang::Init(bool apr_lat)
{
    pr_lat = apr_lat;
    is_inter = apr_lat;
    desk_lang = TReqInfo::Instance()->desk.lang;
}

void TTagLang::Init(const string &airp_dep, const string &airp_arv, bool apr_lat)
{
    TBaseTable &airps = base_tables.get("AIRPS");
    TBaseTable &cities = base_tables.get("CITIES");
    route_country = cities.get_row("code",airps.get_row("code",airp_dep).AsString("city")).AsString("country");
    string c = cities.get_row("code",airps.get_row("code",airp_arv).AsString("city")).AsString("country");
    bool route_inter = false;
    if(route_country != c) {
        route_inter = true;
        route_country.clear();
    }

    std::string route_country_lang; //язык страны, где выполняется внутренний рейс
    if(route_country == "РФ")
        route_country_lang = AstraLocale::LANG_RU;
    else
        route_country_lang = "";
    pr_lat = apr_lat;
    is_inter =
        (route_inter || route_country_lang.empty() || route_country_lang!=TReqInfo::Instance()->desk.lang) or
        pr_lat;
    desk_lang = TReqInfo::Instance()->desk.lang;
}

bool TTagLang::IsInter() const
{
    return is_inter or english_tag();
}

string TTagLang::GetLang(TElemFmt &fmt, string firm_lang) const
{ string lang = firm_lang;
  if (lang.empty())
  {
      if(tag_lang.empty()) {
          lang = desk_lang;
          if (IsInter())
          {
              if (fmt==efmtNameShort || fmt==efmtNameLong) lang=AstraLocale::LANG_EN;
              if (fmt==efmtCodeNative) fmt=efmtCodeInter;
              if (fmt==efmtCodeICAONative) fmt=efmtCodeICAOInter;
          };
      } else {
          lang = (tag_lang == "R" ? LANG_RU : LANG_EN);
      }
  };
  return lang;
}

string TTagLang::ElemIdToTagElem(TElemType type, const string &id, TElemFmt fmt, string firm_lang) const
{
  if (id.empty()) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};

string TTagLang::GetLang() const
{
    if(tag_lang.empty()) {
        string lang = TReqInfo::Instance()->desk.lang;
        if (IsInter()) lang=AstraLocale::LANG_EN;
        return lang;
    } else {
        return (tag_lang == "R" ? LANG_RU : LANG_EN);
    }
}

string TTagLang::ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
{
  if (id==ASTRA::NoExists) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  return ElemIdToPrefferedElem(type, id, fmt, lang, true);
};


void TPrnTagStore::tst_get_tag_list(std::vector<string> &tags)
{
    for(map<const string, TTagListItem>::iterator im = tag_list.begin(); im != tag_list.end(); im++)
        tags.push_back(im->first);
}

void TPrnTagStore::TPrnTestTags::Init()
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select name, type, value, value_lat from prn_test_tags";
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("TPrnTagStore::TPrnTestTags::Init: table prn_test_tags is empty");
    for(; not Qry.Eof; Qry.Next())
        items.insert(make_pair(
                    upperc(Qry.FieldAsString("name")),
                    TPrnTestTagsItem(
                        Qry.FieldAsString("type")[0],
                        Qry.FieldAsString("value"),
                        Qry.FieldAsString("value_lat")
                        )
                    )
                );
}

void TPrnTagStore::TTagProps::Init(TDevOper::Enum vop)
{
    ProgTrace(TRACE5, "TTagProps::Init: %s", DevOperTypes().encode(vop).c_str());
    if(op == vop) return;
    op = vop;
    ProgTrace(TRACE5, "TTagProps::Init: load from base");
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   code, "
        "   length, "
        "   EXCEPT_WHEN_GREAT_LEN, "
        "   EXCEPT_WHEN_ONLY_LAT, "
        "   convert_char_view "
        "from "
        "   prn_tag_props "
        "where "
        "   op_type = :op_type";
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op));
    Qry.Execute();
    items.clear();
    for(; not Qry.Eof; Qry.Next())
        items.insert(make_pair(Qry.FieldAsString("code"),
                    TTagPropsItem(
                        Qry.FieldAsInteger("length"),
                        Qry.FieldAsInteger("except_when_great_len") != 0,
                        Qry.FieldAsInteger("except_when_only_lat") != 0,
                        Qry.FieldAsInteger("convert_char_view") != 0
                    )));
    op = vop;
}

TPrnTagStore::TTagProps::TTagProps(TDevOper::Enum vop): op(TDevOper::Unknown)
{
    Init(vop);
}

string TPrnTagStore::TRemarksInfo::rem_at(size_t idx)
{
    --idx;
    string result;
    if(idx < size())
        result = at(idx);
    return result;
}

void TPrnTagStore::TRemarksInfo::add(const string &val)
{
    if(not val.empty()) push_back(val);
}

void TPrnTagStore::TRemarksInfo::Init(TPrnTagStore &pts)
{
    add(pts.get_tag_no_err(TAG::REMARKS1));
    add(pts.get_tag_no_err(TAG::REMARKS2));
    add(pts.get_tag_no_err(TAG::REMARKS3));
    add(pts.get_tag_no_err(TAG::REMARKS4));
    add(pts.get_tag_no_err(TAG::REMARKS5));
    fexists = true;
}

// Bag receipts
TPrnTagStore::TPrnTagStore(const TBagReceipt &arcpt, bool apr_lat):
    rcpt(arcpt),
    time_print(NowUTC()),
    space_if_empty(false),
    prn_tag_props(TDevOper::PrnBR)
{
    from_scan_code = false;
    print_mode = 0;
    tag_lang.Init(arcpt, apr_lat);
    tag_list.insert(make_pair(TAG::BULKY_BT,            TTagListItem(&TPrnTagStore::BULKY_BT)));
    tag_list.insert(make_pair(TAG::BULKY_BT_LETTER,     TTagListItem(&TPrnTagStore::BULKY_BT_LETTER)));
    tag_list.insert(make_pair(TAG::GOLF_BT,             TTagListItem(&TPrnTagStore::GOLF_BT)));
    tag_list.insert(make_pair(TAG::OTHER_BT,            TTagListItem(&TPrnTagStore::OTHER_BT)));
    tag_list.insert(make_pair(TAG::OTHER_BT_LETTER,     TTagListItem(&TPrnTagStore::OTHER_BT_LETTER)));
    tag_list.insert(make_pair(TAG::PET_BT,              TTagListItem(&TPrnTagStore::PET_BT)));
    tag_list.insert(make_pair(TAG::SKI_BT,              TTagListItem(&TPrnTagStore::SKI_BT)));
    tag_list.insert(make_pair(TAG::VALUE_BT,            TTagListItem(&TPrnTagStore::VALUE_BT)));
    tag_list.insert(make_pair(TAG::VALUE_BT_LETTER,     TTagListItem(&TPrnTagStore::VALUE_BT_LETTER)));
    tag_list.insert(make_pair(TAG::AIRCODE,             TTagListItem(&TPrnTagStore::BR_AIRCODE)));
    tag_list.insert(make_pair(TAG::AIRLINE,             TTagListItem(&TPrnTagStore::BR_AIRLINE)));
    tag_list.insert(make_pair(TAG::AIRLINE_CODE,        TTagListItem(&TPrnTagStore::AIRLINE_CODE)));
    tag_list.insert(make_pair(TAG::AMOUNT_FIGURES,      TTagListItem(&TPrnTagStore::AMOUNT_FIGURES)));
    tag_list.insert(make_pair(TAG::AMOUNT_LETTERS,      TTagListItem(&TPrnTagStore::AMOUNT_LETTERS)));
    tag_list.insert(make_pair(TAG::BAG_NAME,            TTagListItem(&TPrnTagStore::BAG_NAME)));
    tag_list.insert(make_pair(TAG::CHARGE,              TTagListItem(&TPrnTagStore::CHARGE)));
    tag_list.insert(make_pair(TAG::CURRENCY,            TTagListItem(&TPrnTagStore::CURRENCY)));
    tag_list.insert(make_pair(TAG::EQUI_AMOUNT_PAID,    TTagListItem(&TPrnTagStore::EQUI_AMOUNT_PAID)));
    tag_list.insert(make_pair(TAG::EX_WEIGHT,           TTagListItem(&TPrnTagStore::EX_WEIGHT)));
    tag_list.insert(make_pair(TAG::EXCHANGE_RATE,       TTagListItem(&TPrnTagStore::EXCHANGE_RATE)));
    tag_list.insert(make_pair(TAG::ISSUE_DATE,          TTagListItem(&TPrnTagStore::ISSUE_DATE)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE1,        TTagListItem(&TPrnTagStore::ISSUE_PLACE1)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE2,        TTagListItem(&TPrnTagStore::ISSUE_PLACE2)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE3,        TTagListItem(&TPrnTagStore::ISSUE_PLACE3)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE4,        TTagListItem(&TPrnTagStore::ISSUE_PLACE4)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE5,        TTagListItem(&TPrnTagStore::ISSUE_PLACE5)));
    tag_list.insert(make_pair(TAG::NDS,                 TTagListItem(&TPrnTagStore::NDS)));
    tag_list.insert(make_pair(TAG::PAX_DOC,             TTagListItem(&TPrnTagStore::PAX_DOC)));
    tag_list.insert(make_pair(TAG::PAX_NAME,            TTagListItem(&TPrnTagStore::PAX_NAME)));
    tag_list.insert(make_pair(TAG::PAY_FORM,            TTagListItem(&TPrnTagStore::PAY_FORM)));
    tag_list.insert(make_pair(TAG::POINT_ARV,           TTagListItem(&TPrnTagStore::POINT_ARV)));
    tag_list.insert(make_pair(TAG::POINT_DEP,           TTagListItem(&TPrnTagStore::POINT_DEP)));
    tag_list.insert(make_pair(TAG::PREV_NO,             TTagListItem(&TPrnTagStore::PREV_NO)));
    tag_list.insert(make_pair(TAG::RATE,                TTagListItem(&TPrnTagStore::RATE)));
    tag_list.insert(make_pair(TAG::REMARKS1,            TTagListItem(&TPrnTagStore::REMARKS1)));
    tag_list.insert(make_pair(TAG::REMARKS2,            TTagListItem(&TPrnTagStore::REMARKS2)));
    tag_list.insert(make_pair(TAG::REMARKS3,            TTagListItem(&TPrnTagStore::REMARKS3)));
    tag_list.insert(make_pair(TAG::REMARKS4,            TTagListItem(&TPrnTagStore::REMARKS4)));
    tag_list.insert(make_pair(TAG::REMARKS5,            TTagListItem(&TPrnTagStore::REMARKS5)));
    tag_list.insert(make_pair(TAG::SERVICE_TYPE,        TTagListItem(&TPrnTagStore::SERVICE_TYPE)));
    tag_list.insert(make_pair(TAG::TICKETS,             TTagListItem(&TPrnTagStore::TICKETS)));
    tag_list.insert(make_pair(TAG::TO,                  TTagListItem(&TPrnTagStore::TO)));
    tag_list.insert(make_pair(TAG::TOTAL,               TTagListItem(&TPrnTagStore::TOTAL)));
    remarksInfo.Init(*this);
}

TPrnTagStore::TPrnTagStore(const std::string& airp_dep,
                           const std::string& airp_arv, bool apr_lat) :
    pax_id(NoExists),
    print_mode(0),
    time_print(NowUTC()),
    space_if_empty(false),
    prn_tag_props(TDevOper::PrnBP)
{
    from_scan_code = false;
    init_bp_tags();
    tag_lang.Init(airp_dep, airp_arv, apr_lat);
}

// Test tags
TPrnTagStore::TPrnTagStore(bool apr_lat):
    time_print(NowUTC()),
    space_if_empty(false),
    prn_tag_props(TDevOper::Unknown)
{
    from_scan_code = false;
    print_mode = 0;
    tag_lang.Init(apr_lat);
    prn_test_tags.Init();
}

TPrnTagStore::TPrnTagStore(TDevOper::Enum _op_type, const string &ascan, boost::optional<const std::list<AstraLocale::LexemaData> &> aerrors, bool apr_lat):
    op_type(_op_type),
    scan(ascan),
    time_print(NowUTC()),
    space_if_empty(false),
    prn_tag_props(TDevOper::PrnBP)
{
    errors = aerrors;
    from_scan_code = true;
    scan_data = boost::shared_ptr<BCBPSections>(new BCBPSections());
    BCBPSections::get(ascan, 0, ascan.size(), *scan_data);
    print_mode = 0;
    tag_lang.Init(apr_lat);
    init_bp_tags();
}

void TPrnTagStore::init_bp_tags()
{
    tag_list.insert(make_pair(TAG::BCBP_V_5,                TTagListItem(&TPrnTagStore::BCBP_V_5, POINT_INFO | PAX_INFO | PNR_INFO)));
    tag_list.insert(make_pair(TAG::BCBP_M_2,                TTagListItem(&TPrnTagStore::BCBP_M_2, POINT_INFO | PAX_INFO | PNR_INFO)));
    tag_list.insert(make_pair(TAG::ACT,                     TTagListItem(&TPrnTagStore::ACT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AGENT,                   TTagListItem(&TPrnTagStore::AGENT)));
    tag_list.insert(make_pair(TAG::AIRLINE,                 TTagListItem(&TPrnTagStore::AIRLINE, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_SHORT,           TTagListItem(&TPrnTagStore::AIRLINE_SHORT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_NAME,            TTagListItem(&TPrnTagStore::AIRLINE_NAME, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRP_ARV,                TTagListItem(&TPrnTagStore::AIRP_ARV)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME,           TTagListItem(&TPrnTagStore::AIRP_ARV_NAME)));
    tag_list.insert(make_pair(TAG::AIRP_DEP,                TTagListItem(&TPrnTagStore::AIRP_DEP)));
    tag_list.insert(make_pair(TAG::AIRP_DEP_NAME,           TTagListItem(&TPrnTagStore::AIRP_DEP_NAME)));
    tag_list.insert(make_pair(TAG::BAG_AMOUNT,              TTagListItem(&TPrnTagStore::BAG_AMOUNT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::TAGS,                    TTagListItem(&TPrnTagStore::TAGS, PAX_INFO)));
    tag_list.insert(make_pair(TAG::BAG_WEIGHT,              TTagListItem(&TPrnTagStore::BAG_WEIGHT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::BAGGAGE,                 TTagListItem(&TPrnTagStore::BAGGAGE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::BRAND,                   TTagListItem(&TPrnTagStore::BRAND, PAX_INFO)));
    tag_list.insert(make_pair(TAG::BRD_FROM,                TTagListItem(&TPrnTagStore::BRD_FROM, BRD_INFO)));
    tag_list.insert(make_pair(TAG::BRD_TO,                  TTagListItem(&TPrnTagStore::BRD_TO, BRD_INFO | POINT_INFO)));
    tag_list.insert(make_pair(TAG::CHD,                     TTagListItem(&TPrnTagStore::CHD, PAX_INFO)));
    tag_list.insert(make_pair(TAG::CITY_ARV_NAME,           TTagListItem(&TPrnTagStore::CITY_ARV_NAME)));
    tag_list.insert(make_pair(TAG::CITY_DEP_NAME,           TTagListItem(&TPrnTagStore::CITY_DEP_NAME)));
    tag_list.insert(make_pair(TAG::CLASS,                   TTagListItem(&TPrnTagStore::CLASS)));
    tag_list.insert(make_pair(TAG::CLASS_NAME,              TTagListItem(&TPrnTagStore::CLASS_NAME)));
    tag_list.insert(make_pair(TAG::DESK,                    TTagListItem(&TPrnTagStore::DESK)));
    tag_list.insert(make_pair(TAG::DOCUMENT,                TTagListItem(&TPrnTagStore::DOCUMENT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::DUPLICATE,               TTagListItem(&TPrnTagStore::DUPLICATE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::EST,                     TTagListItem(&TPrnTagStore::EST, POINT_INFO)));
    tag_list.insert(make_pair(TAG::ETICKET_NO,              TTagListItem(&TPrnTagStore::ETICKET_NO, PAX_INFO))); // !!!
    tag_list.insert(make_pair(TAG::ETKT,                    TTagListItem(&TPrnTagStore::ETKT, PAX_INFO))); // !!!
    tag_list.insert(make_pair(TAG::EXCESS,                  TTagListItem(&TPrnTagStore::EXCESS)));
    tag_list.insert(make_pair(TAG::FLT_NO,                  TTagListItem(&TPrnTagStore::FLT_NO, POINT_INFO))); // !!!
    tag_list.insert(make_pair(TAG::FQT,                     TTagListItem(&TPrnTagStore::FQT, FQT_INFO)));
    tag_list.insert(make_pair(TAG::FQT_TIER_LEVEL,          TTagListItem(&TPrnTagStore::FQT_TIER_LEVEL, FQT_INFO)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_ARV,          TTagListItem(&TPrnTagStore::FULL_PLACE_ARV)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_DEP,          TTagListItem(&TPrnTagStore::FULL_PLACE_DEP)));
    tag_list.insert(make_pair(TAG::FULLNAME,                TTagListItem(&TPrnTagStore::FULLNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::GATE,                    TTagListItem(&TPrnTagStore::GATE)));
    tag_list.insert(make_pair(TAG::GATES,                   TTagListItem(&TPrnTagStore::GATES, POINT_INFO)));
    tag_list.insert(make_pair(TAG::HALL,                    TTagListItem(&TPrnTagStore::HALL)));
    tag_list.insert(make_pair(TAG::IMG,                     TTagListItem(&TPrnTagStore::IMG)));
    tag_list.insert(make_pair(TAG::INF,                     TTagListItem(&TPrnTagStore::INF, PAX_INFO)));
    tag_list.insert(make_pair(TAG::LONG_ARV,                TTagListItem(&TPrnTagStore::LONG_ARV)));
    tag_list.insert(make_pair(TAG::LONG_DEP,                TTagListItem(&TPrnTagStore::LONG_DEP)));
    tag_list.insert(make_pair(TAG::NAME,                    TTagListItem(&TPrnTagStore::NAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::NO_SMOKE,                TTagListItem(&TPrnTagStore::NO_SMOKE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::ONE_SEAT_NO,             TTagListItem(&TPrnTagStore::ONE_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PAX_ID,                  TTagListItem(&TPrnTagStore::PAX_ID, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PLACE_ARV,               TTagListItem(&TPrnTagStore::PLACE_ARV)));
    tag_list.insert(make_pair(TAG::PLACE_DEP,               TTagListItem(&TPrnTagStore::PLACE_DEP)));
    tag_list.insert(make_pair(TAG::REG_NO,                  TTagListItem(&TPrnTagStore::REG_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM,                     TTagListItem(&TPrnTagStore::REM, REM_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT0,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT1,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT2,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT3,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT4,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT5,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT6,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT7,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT8,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::REM_TXT9,                TTagListItem(&TPrnTagStore::REM_TXT, POINT_INFO | PAX_INFO)));
    tag_list.insert(make_pair(TAG::RFISC_BSN_LONGUE,        TTagListItem(&TPrnTagStore::RFISC_BSN_LONGUE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::RFISC_FAST_TRACK,        TTagListItem(&TPrnTagStore::RFISC_FAST_TRACK, PAX_INFO)));
    tag_list.insert(make_pair(TAG::RFISC_UPGRADE,           TTagListItem(&TPrnTagStore::RFISC_UPGRADE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::RK_AMOUNT,               TTagListItem(&TPrnTagStore::RK_AMOUNT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::RK_WEIGHT,               TTagListItem(&TPrnTagStore::RK_WEIGHT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::RSTATION,                TTagListItem(&TPrnTagStore::RSTATION, RSTATION_INFO)));
    tag_list.insert(make_pair(TAG::SCD,                     TTagListItem(&TPrnTagStore::SCD, POINT_INFO)));
    tag_list.insert(make_pair(TAG::SEAT_NO,                 TTagListItem(&TPrnTagStore::SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::STR_SEAT_NO,             TTagListItem(&TPrnTagStore::STR_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SUBCLS,                  TTagListItem(&TPrnTagStore::SUBCLS, PAX_INFO)));
    tag_list.insert(make_pair(TAG::LIST_SEAT_NO,            TTagListItem(&TPrnTagStore::LIST_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SURNAME,                 TTagListItem(&TPrnTagStore::SURNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::TEST_SERVER,             TTagListItem(&TPrnTagStore::TEST_SERVER)));
    tag_list.insert(make_pair(TAG::TICKET_NO,               TTagListItem(&TPrnTagStore::TICKET_NO, PAX_INFO))); // !!!
    tag_list.insert(make_pair(TAG::TIME_PRINT,              TTagListItem(&TPrnTagStore::TIME_PRINT)));
    tag_list.insert(make_pair(TAG::PAX_TITLE,               TTagListItem(&TPrnTagStore::PAX_TITLE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PNR,                     TTagListItem(&TPrnTagStore::PNR, PNR_INFO)));
    tag_list.insert(make_pair(TAG::BI_HALL,                 TTagListItem(&TPrnTagStore::BI_HALL, POINT_INFO)));
    tag_list.insert(make_pair(TAG::BI_HALL_CAPTION,         TTagListItem(&TPrnTagStore::BI_HALL_CAPTION, POINT_INFO)));
    tag_list.insert(make_pair(TAG::BI_RULE,                 TTagListItem(&TPrnTagStore::BI_RULE)));
    tag_list.insert(make_pair(TAG::BI_RULE_GUEST,           TTagListItem(&TPrnTagStore::BI_RULE_GUEST)));
    tag_list.insert(make_pair(TAG::BI_AIRP_TERMINAL,        TTagListItem(&TPrnTagStore::BI_AIRP_TERMINAL)));
    tag_list.insert(make_pair(TAG::VOUCHER_CODE,            TTagListItem(&TPrnTagStore::VOUCHER_CODE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT,            TTagListItem(&TPrnTagStore::VOUCHER_TEXT)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT1,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT2,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT3,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT4,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT5,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT6,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT7,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT8,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT9,           TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
    tag_list.insert(make_pair(TAG::VOUCHER_TEXT10,          TTagListItem(&TPrnTagStore::VOUCHER_TEXT_FREE)));
}

void TPrnTagStore::tagsFromXML(xmlNodePtr tagsNode)
{
    if(tagsNode) {
        // Положим теги из клиентского запроса
        for(xmlNodePtr curNode = tagsNode->children; curNode; curNode = curNode->next) {
            string value = NodeAsString(curNode);
            if(value.empty()) continue;
            set_tag(upperc((const char *)curNode->name), NodeAsString(curNode));
        }
    }
}

// BP && BT
TPrnTagStore::TPrnTagStore(TDevOper::Enum _op_type, int agrp_id, int apax_id, bool afrom_scan_code, int apr_lat, xmlNodePtr tagsNode, const TTrferRoute &aroute):
    op_type(_op_type),
    time_print(NowUTC()),
    space_if_empty(false),
    prn_tag_props(aroute.empty() ? TDevOper::PrnBP : TDevOper::PrnBT)
{
    if(op_type == TDevOper::PrnBP or op_type == TDevOper::PrnBI) rfisc_descr.fromDB(agrp_id, apax_id);

    from_scan_code = afrom_scan_code;
    print_mode = 0;
    grpInfo.Init(agrp_id, apax_id);
    tag_lang.Init(
            grpInfo.airp_dep,
            (aroute.empty() ? grpInfo.airp_arv : aroute.back().airp_arv),
            apr_lat != 0);
    pax_id = apax_id;
    if(prn_tag_props.op == TDevOper::PrnBP and pax_id == NoExists)
        throw Exception("TPrnTagStore::TPrnTagStore: pax_id not defined for bp mode");
    init_bp_tags();

    // specific for bag tags
    tag_list.insert(make_pair(TAG::AIRCODE,         TTagListItem(&TPrnTagStore::AIRCODE)));
    tag_list.insert(make_pair(TAG::NO,              TTagListItem(&TPrnTagStore::NO)));
    tag_list.insert(make_pair(TAG::ISSUED,          TTagListItem(&TPrnTagStore::ISSUED)));
    tag_list.insert(make_pair(TAG::BT_AMOUNT,       TTagListItem(&TPrnTagStore::BT_AMOUNT)));
    tag_list.insert(make_pair(TAG::BT_WEIGHT,       TTagListItem(&TPrnTagStore::BT_WEIGHT)));
    tag_list.insert(make_pair(TAG::LIAB_LIMIT,      TTagListItem(&TPrnTagStore::LIAB_LIMIT)));
    tag_list.insert(make_pair(TAG::FLT_NO1,         TTagListItem(&TPrnTagStore::FLT_NO1)));
    tag_list.insert(make_pair(TAG::FLT_NO2,         TTagListItem(&TPrnTagStore::FLT_NO2)));
    tag_list.insert(make_pair(TAG::FLT_NO3,         TTagListItem(&TPrnTagStore::FLT_NO3)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE1,     TTagListItem(&TPrnTagStore::LOCAL_DATE1)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE2,     TTagListItem(&TPrnTagStore::LOCAL_DATE2)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE3,     TTagListItem(&TPrnTagStore::LOCAL_DATE3)));
    tag_list.insert(make_pair(TAG::AIRLINE1,        TTagListItem(&TPrnTagStore::AIRLINE1)));
    tag_list.insert(make_pair(TAG::AIRLINE2,        TTagListItem(&TPrnTagStore::AIRLINE2)));
    tag_list.insert(make_pair(TAG::AIRLINE3,        TTagListItem(&TPrnTagStore::AIRLINE3)));
    tag_list.insert(make_pair(TAG::AIRP_ARV1,       TTagListItem(&TPrnTagStore::AIRP_ARV1)));
    tag_list.insert(make_pair(TAG::AIRP_ARV2,       TTagListItem(&TPrnTagStore::AIRP_ARV2)));
    tag_list.insert(make_pair(TAG::AIRP_ARV3,       TTagListItem(&TPrnTagStore::AIRP_ARV3)));
    tag_list.insert(make_pair(TAG::FLTDATE1,        TTagListItem(&TPrnTagStore::FLTDATE1)));
    tag_list.insert(make_pair(TAG::FLTDATE2,        TTagListItem(&TPrnTagStore::FLTDATE2)));
    tag_list.insert(make_pair(TAG::FLTDATE3,        TTagListItem(&TPrnTagStore::FLTDATE3)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME1,  TTagListItem(&TPrnTagStore::AIRP_ARV_NAME1)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME2,  TTagListItem(&TPrnTagStore::AIRP_ARV_NAME2)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME3,  TTagListItem(&TPrnTagStore::AIRP_ARV_NAME3)));

    tagsFromXML(tagsNode);
}

void TPrnTagStore::clear()
{
    for(map<const string, TTagListItem>::iterator im = tag_list.begin(); im != tag_list.end(); im++)
        im->second.english_only = true;
}

void TPrnTagStore::set_tag(string name, const TBCBPData &value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

void TPrnTagStore::set_tag(string name, const BIPrintRules::TRule &value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

void TPrnTagStore::set_tag(string name, TDateTime value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

void TPrnTagStore::set_tag(string name, int value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

void TPrnTagStore::set_tag(string name, string value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

string TPrnTagStore::get_field_from_bcbp(const std::string &name, size_t len, const std::string &text, const std::string &date_format)
{
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::get_field_from_bcbp: tag '%s' not implemented", name.c_str());
    string result = (this->*im->second.tag_funct)(TFieldParams(name, date_format, im->second.TagInfo, len, text));
    im->second.processed = true;
    im->second.english_only &= tag_lang.english_tag();
    return result;
}

string TPrnTagStore::get_test_field(const std::string &name, size_t len, const std::string &text, const std::string &date_format)
{
    string upper_name = upperc(name);
    map<string, TPrnTestTagsItem>::iterator im = prn_test_tags.items.find(upper_name);
    if(im == prn_test_tags.items.end())
        throw Exception("TPrnTagStore::get_test_field: %s tag not found", upper_name.c_str());
    string value = tag_lang.IsInter() ? im->second.value_lat : im->second.value;
    if(value.empty())
        value = im->second.value;
    TDateTime date = 0;
    ostringstream result;
    switch(im->second.type) {
        case 'D':
            StrToDateTime(value.c_str(), ServerFormatDateTimeAsString, date);
            result << DateTimeToStr(date, date_format, tag_lang.IsInter());
            break;
        case 'S':
            result << value;
            break;
        case 'I':
            result << setw(len) << setfill('0') << ToInt(value);
            break;
    }
    im->second.processed = true;
    return result.str();
}

string TPrnTagStore::get_real_field(const std::string &name, size_t len, const std::string &text, const std::string &date_format)
{
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::get_field: tag '%s' not implemented", name.c_str());
    if((im->second.info_type & POINT_INFO) == POINT_INFO)
        pointInfo.Init(prn_tag_props.op, grpInfo.point_dep, grpInfo.grp_id);
    if((im->second.info_type & PAX_INFO) == PAX_INFO)
        paxInfo.Init(grpInfo, pax_id, tag_lang);
    if((im->second.info_type & BRD_INFO) == BRD_INFO)
        brdInfo.Init(grpInfo.point_dep);
    if((im->second.info_type & FQT_INFO) == FQT_INFO)
        fqtInfo.Init(pax_id);
    if((im->second.info_type & PNR_INFO) == PNR_INFO)
        pnrInfo.Init(pax_id);
    if((im->second.info_type & RSTATION_INFO) == RSTATION_INFO)
        rstationInfo.Init();
    if((im->second.info_type & REM_INFO) == REM_INFO)
        remInfo.Init(grpInfo.point_dep);
    string result;
    try {
        result = (this->*im->second.tag_funct)(TFieldParams(name, date_format, im->second.TagInfo, len, text));
        im->second.processed = true;
        im->second.english_only &= tag_lang.english_tag();
    } catch(UserException E) {
        throw;
    } catch(Exception E) {
        throw Exception("tag %s failed: %s", name.c_str(), E.what());
    } catch(boost::bad_any_cast E) {
        throw Exception("tag %s failed: %s", name.c_str(), E.what());
    }
    return result;
}

bool TPrnTagStore::tag_processed(std::string name)
{
    if(prn_test_tags.items.empty()) {
        map<const string, TTagListItem>::iterator im = tag_list.find(name);
        if(im == tag_list.end())
            throw Exception("TPrnTagStore::tag_processed: tag '%s' not implemented", name.c_str());
        return im->second.processed;
    } else {
        map<string, TPrnTestTagsItem>::iterator im = prn_test_tags.items.find(name);
        if(im == prn_test_tags.items.end())
            throw Exception("TPrnTagStore::tag_processed: %s tag not found", name.c_str());
        return im->second.processed;
    }
}

string TPrnTagStore::get_tag_no_err(string name, string date_format, string tag_lang)
{
    return get_field(upperc(name), 0, "", "L", date_format, tag_lang, false);
}


string TPrnTagStore::get_tag(string name, string date_format, string tag_lang)
{
    return get_field(upperc(name), 0, "", "L", date_format, tag_lang);
}

string cut_result(string result)
{
    TrimString(result);
    if(result.size() > 20)
        result = result.substr(0, 20) + "...";
    return result;
}

string TPrnTagStore::get_field(std::string name, size_t len, const std::string &text, std::string align, std::string date_format, string tag_lang, bool pr_user_except)
{
    std::map<std::string, TTagPropsItem>::iterator iprops = prn_tag_props.items.find(name);
    if(iprops == prn_tag_props.items.end())
        throw Exception("Tag props not found for name = '%s'", name.c_str());

    this->tag_lang.set_tag_lang(tag_lang);
    try {
        string result;
        if(scan_data != NULL) {
            result = get_field_from_bcbp(name, len, text, date_format);
        } else if(prn_test_tags.items.empty())
            result = get_real_field(name, len, text, date_format);
        else {
            result = get_test_field(name, len, text, date_format);
            if(iprops->second.length == 0 and len != 0)
                result = result.substr(0, len);
        }
        if(!len) len = result.size();

        if(print_mode == 1 or (print_mode == 2 and (name == TAG::PAX_ID or name == TAG::TEST_SERVER)))
            return string(len, '8');
        if(print_mode == 2)
            return AlignString("8", len, align);
        if(print_mode == 3)
            return string(len, ' ');

        if(len < result.length()) {
            if(iprops->second.length == 0 or len < iprops->second.length) {
                if(iprops->second.except_when_great_len)
                    throw Exception("Длина данных больше длины тега '%s'", name.c_str());
                else
                    result = string(len, '?');
            } else
                result = result.substr(0, len);
        } else {
            if(StrUtils::trim(result).empty())
                result.clear();
            else {
                result = AlignString(result, len, align);
                result = StrUtils::rtrim(result);
            }
        }
        if(this->tag_lang.get_pr_lat() and not IsAscii7(result)) {
            bool replace_good = false;
            if(iprops->second.convert_char_view) {
                result = convert_char_view(result, true);
                replace_good = IsAscii7(result);
            }
            if(not replace_good) {
                LexemaData err;
                err.lexema_id = "MSG.NO_LAT_PRN_DATA";
                err.lparams << LParam("tag", name) << LParam("value", cut_result(result));
                if(iprops->second.except_when_only_lat) {
                    ProgTrace(TRACE0, "Данные печати не латинские: %s = \"%s\"", name.c_str(), result.c_str());
                    if(pr_user_except)
                        throw UserException(err.lexema_id, err.lparams);
                } else {
                    showErrorMessage(err);
                    for(string::iterator si = result.begin(); si != result.end(); si++)
                        if(not IsAscii7(*si)) *si = '_';
                }
            }
        }
        this->tag_lang.set_tag_lang("");
        //ProgTrace(TRACE5, "name: %s, value: '%s'", name.c_str(), result.c_str());
        if(result.empty() and space_if_empty.get()) result.append(1, ' ');
        return result;
    } catch(...) {
        this->tag_lang.set_tag_lang("");
        throw;
    }
}

void add_part(string &tag, std::string &part1, std::string &part2)
{
    part1 += tag + ", ";
    part2 += ":" + tag + ", ";
}

class TPrnQryBuilder {
    private:
        TQuery &Qry;
        string part1, part2;
        void add_parts(string name);
    public:
        TPrnQryBuilder(TQuery &aQry);
        string text();
        void add_part(string name, int value);
        void add_part(string name, string value);
        void add_part(string name, TDateTime value);
};

string TPrnQryBuilder::text()
{
    return part1 + part2 +
        "   ); "
        "end;";
}
void TPrnQryBuilder::add_parts(string name)
{
    part1 += ", " + name;
    part2 += ", :" + name;
}

void TPrnQryBuilder::add_part(string name, int value)
{
    if(value == NoExists)
        Qry.CreateVariable(name, otInteger, FNull);
    else
        Qry.CreateVariable(name, otInteger, value);
    add_parts(name);
}

void TPrnQryBuilder::add_part(string name, TDateTime value)
{
    Qry.CreateVariable(name, otDate, value);
    add_parts(name);
}

void TPrnQryBuilder::add_part(string name, string value)
{
    Qry.CreateVariable(name, otString, value);
    add_parts(name);
}

TPrnQryBuilder::TPrnQryBuilder(TQuery &aQry): Qry(aQry)
{
    part1 =
        "begin "
        "   delete from confirm_print where "
        "       pax_id = :pax_id and "
        "       pr_print = 0 and "
        "       desk=:desk and "
        "       (voucher = :voucher OR voucher IS NULL AND :voucher IS NULL) and "
        OP_TYPE_COND("op_type")"; "
        "   insert into confirm_print( "
        "       pax_id, "
        "       time_print, "
        "       pr_print, "
        "       desk, "
        "       client_type, "
        "       from_scan_code, "
        "       op_type ";
    part2 =
        "   ) values( "
        "       :pax_id, "
        "       :now_utc, "
        "       :pr_print, "
        "       :desk, "
        "       :client_type, "
        "       :from_scan_code, "
        "       :op_type ";
};

TDateTime get_date_from_bcbp(int julian_date)
{
    //вообще-то здесь нужно различать TDirection
    //для time_print TDirection::before, для date_of_flight TDirection::everywhere
    //это на потом
    JulianDate d(julian_date, NowUTC(), JulianDate::TDirection::everywhere);
    d.trace(__FUNCTION__);
    return d.getDateTime();
}

string TPrnTagStore::errorsToString()
{
    string result;
    if(errors and not errors.get().empty())
        result = getLocaleText(errors.get().begin()->lexema_id, errors.get().begin()->lparams, LANG_EN);
    return result.substr(0, 200);
}

void TPrnTagStore::save_foreign_scan()
{
    pair<int, char> bcbp_flt_no = scan_data->flight_number(0);
    string suffix;
    suffix.append(1, bcbp_flt_no.second);
    suffix = StrUtils::trim(suffix); // пустой суффикс почему-то хранится как пробел ' '

    TCachedQuery Qry(
            "insert into foreign_scan ( "
            "   id, "
            "   time_print, "
            "   desk, "
            "   desk_airp, "
            "   descr, "
            "   client_type, "
            "   airline, "
            "   flt_no, "
            "   suffix, "
            "   scd_out, "
            "   airp_dep, "
            "   airp_arv, "
            "   scan_data, "
            "   errors "
            ") values ( "
            "   cycle_id__seq.nextval, "
            "   :time_print, "
            "   :desk, "
            "   (select airp from desk_grp where grp_id = (select grp_id from desks where code = :desk)), "
            "   (select descr from web_clients where desk = :desk), "
            "   :client_type, "
            "   :airline, "
            "   :flt_no, "
            "   :suffix, "
            "   :scd_out, "
            "   :airp_dep, "
            "   :airp_arv, "
            "   :scan_data, "
            "   :errors "
            ") ",
        QParams()
            << QParam("time_print", otDate, time_print.val)
            << QParam("desk", otString, TReqInfo::Instance()->desk.code)
            << QParam("client_type", otString, EncodeClientType(TReqInfo::Instance()->client_type))
            << QParam("airline", otString, getElemId(etAirline, StrUtils::trim(scan_data->operating_carrier_designator(0))))
            << QParam("flt_no", otInteger, bcbp_flt_no.first)
            << QParam("suffix", otString, (suffix.empty() ? suffix : getElemId(etSuffix, suffix)))
            << QParam("scd_out", otDate, get_date_from_bcbp(scan_data->date_of_flight(0)))
            << QParam("airp_dep", otString, getElemId(etAirp, scan_data->from_city_airport(0)))
            << QParam("airp_arv", otString, getElemId(etAirp, scan_data->to_city_airport(0)))
            << QParam("scan_data", otString, scan)
            << QParam("errors", otString, errorsToString()));

    Qry.get().Execute();
}

void TPrnTagStore::confirm_print(bool pr_print, TDevOper::Enum op_type)
{
    if(scan_data != NULL) return;
    if (isTestPaxId(pax_id)) return;
    if(not prn_test_tags.items.empty())
        throw Exception("confirm_print can't be called in test mode");
    TQuery Qry(&OraSession);
    TPrnQryBuilder prnQry(Qry);
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("now_utc", otDate, time_print.val);
    // Если печать с киоска, подтверждаем сразу.
    Qry.CreateVariable("pr_print", otInteger, (pr_print ? pr_print : TReqInfo::Instance()->client_type == ctKiosk));
    Qry.CreateVariable("desk", otString, TReqInfo::Instance()->desk.code);
    Qry.CreateVariable("client_type", otString, EncodeClientType(TReqInfo::Instance()->client_type));
    Qry.CreateVariable("from_scan_code", otInteger, from_scan_code);
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));

    if(
            tag_list[TAG::SEAT_NO].processed or
            tag_list[TAG::ONE_SEAT_NO].processed or
            tag_list[TAG::STR_SEAT_NO].processed or
            tag_list[TAG::LIST_SEAT_NO].processed
      ) {
        bool seat_no_lat = true;

        if(tag_list[TAG::SEAT_NO].processed)
            seat_no_lat &= tag_list[TAG::SEAT_NO].english_only;
        if(tag_list[TAG::ONE_SEAT_NO].processed)
            seat_no_lat &= tag_list[TAG::ONE_SEAT_NO].english_only;
        if(tag_list[TAG::STR_SEAT_NO].processed)
            seat_no_lat &= tag_list[TAG::STR_SEAT_NO].english_only;
        if(tag_list[TAG::LIST_SEAT_NO].processed)
            seat_no_lat &= tag_list[TAG::LIST_SEAT_NO].english_only;

        prnQry.add_part("seat_no", get_fmt_seat("list", seat_no_lat));
        prnQry.add_part("seat_no_lat", get_fmt_seat("list", true));
    }
    if(tag_list[TAG::BI_HALL].processed) {
        prnQry.add_part("hall_id", BIHallInfo.hall_id);
    }

    prnQry.add_part("voucher",
            (tag_list[TAG::VOUCHER_CODE].TagInfo.empty() ? string() :
             boost::any_cast<string>(tag_list[TAG::VOUCHER_CODE].TagInfo)));

    Qry.SQLText = prnQry.text();

    try {
        Qry.Execute();
    } catch(EOracleError &E) {
        if(E.Code == 1) {
            if(TReqInfo::Instance()->client_type == ctTerm and op_type == TDevOper::PrnBP)
                throw UserException("MSG.PRINT.BP_ALREADY_PRODUCED");
        } else
            throw;
    }
}

void TPrnTagStore::TRStationInfo::Init()
{
    if(not pr_init) {
        pr_init = true;
        TQuery Qry(&OraSession);
        Qry.SQLText = "select name from stations where desk = :desk and work_mode = :wm";
        Qry.CreateVariable("desk", otString, TReqInfo::Instance()->desk.code);
        Qry.CreateVariable("wm", otString, "Р");
        Qry.Execute();
        if(not Qry.Eof)
            name = Qry.FieldAsString("name");
    }
}

void TPrnTagStore::TPnrInfo::Init(int apax_id)
{
    if(apax_id == NoExists) {
        LogTrace(TRACE3) << "Fake pax_id detected!";
        return;
    }
    if(not pr_init) {
        pr_init = true;
        pnrs.getByPaxId(apax_id, airline);
    }
}

void TPrnTagStore::TFqtInfo::Init(int apax_id)
{
    if(apax_id == NoExists) {
        LogTrace(TRACE3) << "Fake pax_id detected!";
        return;
    }
    if(not pr_init) {
        pr_init = true;
        TQuery Qry(&OraSession);
        if (!isTestPaxId(apax_id))
            Qry.SQLText = "select airline, no, extra, tier_level from pax_fqt where pax_id = :pax_id and rownum < 2";
        else
            Qry.SQLText = "SELECT pnr_airline AS airline, fqt_no AS no, NULL AS extra, null tier_level FROM test_pax WHERE id=:pax_id";
        Qry.CreateVariable("pax_id", otInteger, apax_id);
        Qry.Execute();
        if(!Qry.Eof) {
            airline = Qry.FieldAsString("airline");
            no = Qry.FieldAsString("no");
            extra = Qry.FieldAsString("extra");
            tier_level = Qry.FieldAsString("tier_level");
        }
    }
}

void TPrnTagStore::TRemInfo::Init(int point_id)
{
    if(point_id == NoExists) {
        LogTrace(TRACE3) << "Fake point_id detected!";
        return;
    }
    if(not pr_init) {
        pr_init = true;
        rem.Load(retBP, point_id);
    }
}

void TPrnTagStore::TBrdInfo::Init(int point_id)
{
    if(point_id == NoExists) {
        LogTrace(TRACE3) << "Fake point_id detected!";
        return;
    }
    if(brd_from == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   select nvl( est, scd ) into :brd_from from trip_stages where point_id = :point_id and stage_id = :brd_open_stage_id; "
            "   select est, scd into :brd_to_est, :brd_to_scd from trip_stages where point_id = :point_id and stage_id = :brd_close_stage_id; "
            "end; ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.CreateVariable("brd_open_stage_id", otInteger, sOpenBoarding);
        Qry.CreateVariable("brd_close_stage_id", otInteger, sCloseBoarding);
        Qry.DeclareVariable("brd_from", otDate);
        Qry.DeclareVariable("brd_to_est", otDate);
        Qry.DeclareVariable("brd_to_scd", otDate);
        try {
            Qry.Execute();
        } catch(EOracleError &E) {
            if(E.Code == 1403)
                throw Exception("TPrnTagStore::TBrdInfo::Init no data found for point_id = %d", point_id);
            else
                throw;
        }
        brd_from = Qry.GetVariableAsDateTime("brd_from");
        if(not Qry.VariableIsNULL("brd_to_est"))
            brd_to_est = Qry.GetVariableAsDateTime("brd_to_est");
        brd_to_scd = Qry.GetVariableAsDateTime("brd_to_scd");
    }
}

void TPrnTagStore::TPaxInfo::Init(const TGrpInfo &grp_info, int apax_id, TTagLang &tag_lang)
{
    if(apax_id == NoExists) {
        LogTrace(TRACE3) << "Fake pax_id detected!";
        return;
    }
    if(pax_id == NoExists) {
        pax_id = apax_id;
        TQuery Qry(&OraSession);
        if (!isTestPaxId(pax_id))
        {
            LoadPaxDoc(pax_id, doc);
            Qry.SQLText =
                "select "
                "   pax.surname, "
                "   pax.name, "
                "   pax.ticket_rem, "
                "   pax.ticket_no, "
                "   pax.coupon_no, "
                "   DECODE( "
                "       pax.SEAT_TYPE, "
                "       'SMSA',1, "
                "       'SMSW',1, "
                "       'SMST',1, "
                "       0) pr_smoke, "
                "   is_jmp, "
                "   pax.reg_no, "
                "   pax.seats, "
                "   pax.pers_type, "
                "   ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bag_amount, "
                "   ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bag_weight, "
                "   ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rk_amount, "
                "   ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rk_weight, "
                "   ckin.get_excess_wt(pax.grp_id, NULL) AS excess_wt, "
                "   ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
                "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
                "   pax.subclass, "
                "   crs_pnr.class crs_cls "
                "from "
                "   pax, "
                "   crs_pax, "
                "   crs_pnr "
                "where "
                "   pax.pax_id = :pax_id and "
                "   pax.pax_id = crs_pax.pax_id(+) and "
                "   crs_pax.pr_del(+) = 0 and "
                "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
                "   crs_pnr.system(+) = 'CRS' ";
            Qry.CreateVariable("lang", otString, tag_lang.GetLang());

            TPrPrint().get_pr_print(grp_info.grp_id, pax_id, pr_bp_print, pr_bi_print);
            brands.get(pax_id);
        }
        else
        {
            Qry.SQLText =
                "SELECT "
                "   surname, "
                "   NULL AS name, "
                "   'TKNE' AS ticket_rem, "
                "   tkn_no AS ticket_no, "
                "   1 AS coupon_no, "
                "   0 AS pr_smoke, "
                "   0 AS is_jmp, "
                "   reg_no, "
                "   1 AS seats, "
                "   :adult AS pers_type, "
                "   0 AS bag_amount, "
                "   0 AS bag_weight, "
                "   0 AS rk_amount, "
                "   0 AS rk_weight, "
                "   0 AS excess_wt, "
                "   0 AS excess_pc, "
                "   NULL AS tags, "
                "   null subclass, "
                "   null crs_cls "
                "FROM "
                "   test_pax "
                "WHERE "
                "   id = :pax_id ";
            Qry.CreateVariable("adult", otString, EncodePerson(adult));
            pr_bp_print = false;
            pr_bi_print = false;
        };
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TPrnTagStore::TPaxInfo::Init no data found for pax_id = %d", pax_id);
        surname_2d = Qry.FieldAsString("surname");
        name_2d = Qry.FieldAsString("name");
        if(doc.surname.empty() or doc.first_name.empty()) {
            surname = surname_2d;
            name = name_2d;
        } else {
            surname = doc.surname;
            name = doc.first_name;
        }
        document = CheckIn::GetPaxDocStr(NoExists, pax_id, false, tag_lang.GetLang());
        ticket_rem = Qry.FieldAsString("ticket_rem");
        ticket_no = Qry.FieldAsString("ticket_no");
        coupon_no = Qry.FieldAsInteger("coupon_no");
        pr_smoke = Qry.FieldAsInteger("pr_smoke") != 0;
        is_jmp = Qry.FieldAsInteger("is_jmp") != 0;
        reg_no = Qry.FieldAsInteger("reg_no");
        seats = Qry.FieldAsInteger("seats");
        pers_type = Qry.FieldAsString("pers_type");
        bag_amount = Qry.FieldAsInteger("bag_amount");
        bag_weight = Qry.FieldAsInteger("bag_weight");
        rk_amount = Qry.FieldAsInteger("rk_amount");
        rk_weight = Qry.FieldAsInteger("rk_weight");
        excess_wt = Qry.FieldAsInteger("excess_wt");
        excess_pc = Qry.FieldAsInteger("excess_pc");
        tags = Qry.FieldAsString("tags");
        subcls = Qry.FieldAsString("subclass");
        crs_cls = Qry.FieldAsString("crs_cls");

        /*
           TETickItem ETickItem;
           ETickItem.fromDB(ticket_no, coupon_no, TETickItem::Display, false);
           if(not ETickItem.empty()) crs_cls = ETickItem.cls;
           */

        // daem dorabotat' do konca, potom

        if(grp_info.pr_print_fio_pnl) {
            // esli nastroyka vkluchena, togda vytaskivaem dannye is PNL
            TCachedQuery Qry("select name, surname from crs_pax where pax_id = :pax_id",
                    QParams() << QParam("pax_id", otInteger, pax_id));
            Qry.get().Execute();
            if(not Qry.get().Eof) {
                surname = Qry.get().FieldAsString("surname");
                name = Qry.get().FieldAsString("name");
                surname_2d = surname;
                name_2d = name;
            }
        }
    }
}

void TPrnTagStore::TGrpInfo::Init(int agrp_id, int apax_id)
{
    if(agrp_id == NoExists) {
        LogTrace(TRACE3) << "Fake grp_id or pax_id detected!";
        return;
    }
    if(grp_id == NoExists) {
        grp_id = agrp_id;
        TQuery Qry(&OraSession);
        if (!isTestPaxId(grp_id))
        {
            Qry.SQLText =
                "select "
                "   point_dep, "
                "   point_arv, "
                "   airp_dep, "
                "   airp_arv, "
                "   class_grp, "
                "   class, "
                "   hall "
                "from "
                "   pax_grp "
                "where "
                "   grp_id = :grp_id ";
            Qry.CreateVariable("grp_id", otInteger, grp_id);
            Qry.Execute();
            if(Qry.Eof)
                throw Exception("TPrnTagStore::TGrpInfo::Init no data found for grp_id = %d", grp_id);
            airp_dep = Qry.FieldAsString("airp_dep");
            airp_arv = Qry.FieldAsString("airp_arv");
            point_dep = Qry.FieldAsInteger("point_dep");
            point_arv = Qry.FieldAsInteger("point_arv");
            if(not Qry.FieldIsNULL("class_grp"))
                class_grp = Qry.FieldAsInteger("class_grp");
            cls = Qry.FieldAsString("class");
            if(not Qry.FieldIsNULL("hall"))
                hall = Qry.FieldAsInteger("hall");

            TTripInfo info;
            info.getByPointId(point_dep);
            pr_print_fio_pnl = GetTripSets(tsPrintFioPNL,info);
        }
        else
        {
            point_dep=grp_id-TEST_ID_BASE;
            Qry.Clear();
            Qry.SQLText =
                "SELECT airp AS airp_dep FROM points WHERE point_id=:point_id AND pr_del>=0";
            Qry.CreateVariable("point_id", otInteger, point_dep);
            Qry.Execute();
            if(Qry.Eof)
                throw Exception("TPrnTagStore::TGrpInfo::Init no data found for grp_id = %d", grp_id);
            airp_dep = Qry.FieldAsString("airp_dep");

            TTripRouteItem next;
            TTripRoute().GetNextAirp(NoExists,point_dep,trtNotCancelled,next);
            if (next.point_id==NoExists || next.airp.empty())
                throw Exception("TPrnTagStore::TGrpInfo::Init no data found for grp_id = %d", grp_id);
            point_arv = next.point_id;
            airp_arv = next.airp;

            Qry.Clear();
            Qry.SQLText =
                "SELECT cls_grp.id AS class_grp "
                "FROM test_pax, subcls, cls_grp "
                "WHERE test_pax.subclass=subcls.code AND "
                "      subcls.class=cls_grp.class AND "
                "      cls_grp.airline IS NULL AND cls_grp.airp IS NULL AND "
                "      test_pax.id=:pax_id";
            if(apax_id == NoExists) {
                LogError(STDLOG) << "pax_id is not initialized but used!";
            }
            Qry.CreateVariable("pax_id", otInteger, apax_id);
            Qry.Execute();
            if(Qry.Eof)
                throw Exception("TPrnTagStore::TGrpInfo::Init no data found for grp_id = %d", grp_id);
            class_grp = Qry.FieldAsInteger("class_grp");
        };
    }
}

void flt_infants(int point_id, vector<TInfantAdults> &infants)
{
    infants.clear();
    TCachedQuery Qry(
            "SELECT pax.*, crs_inf.pax_id crs_inf_pax_id "
            "FROM pax_grp,pax,crs_inf WHERE "
            "   pax_grp.grp_id=pax.grp_id AND "
            "   pax_grp.point_dep=:point_id AND "
            "   pax_grp.status NOT IN ('E') AND "
            "   pax.seats=0 AND "
            "   pax.pr_brd is not null and "
            "   pax.pax_id=crs_inf.inf_id(+) ",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        infants.push_back(TInfantAdults(Qry.get()));
    if(not infants.empty()) {
        vector<TInfantAdults> adults;
        TCachedQuery adultQry(
                "SELECT * FROM pax_grp,pax WHERE "
                "   pax_grp.grp_id=pax.grp_id AND "
                "   pax_grp.point_dep=:point_id AND "
                "   pax_grp.status NOT IN ('E') AND "
                "   pax.pers_type='ВЗ' AND "
                "   pax.pr_brd is not null ",
                QParams() << QParam("point_id", otInteger, point_id));
        adultQry.get().Execute();
        for(; not adultQry.get().Eof; adultQry.get().Next())
            adults.push_back(TInfantAdults(adultQry.get()));
        SetInfantsToAdults( infants, adults );
    }
}

void TPrnTagStore::TPointInfo::Init(TDevOper::Enum op, int apoint_id, int agrp_id)
{
    if(apoint_id == NoExists || agrp_id == NoExists) {
        LogTrace(TRACE3) << "Fake point_id or grp_id detected!";
        return;
    }
    if(operFlt.point_id == NoExists) {
        if(not operFlt.getByPointId(apoint_id))
            throw Exception("TPointInfo::Init failed. point_id %d not found", apoint_id);
        airline = operFlt.airline;
        flt_no = operFlt.flt_no;
        suffix = operFlt.suffix;
        if (!isTestPaxId(agrp_id))
        {
            Franchise::TProp franchise_prop;
            if(op == TDevOper::PrnBP or op == TDevOper::PrnBT)
                franchise_prop.get(operFlt.point_id, (op == TDevOper::PrnBP ? Franchise::TPropType::bp : Franchise::TPropType::bt));
            if(franchise_prop.val == Franchise::pvNo) {
                airline = franchise_prop.franchisee.airline;
                flt_no = franchise_prop.franchisee.flt_no;
                suffix = franchise_prop.franchisee.suffix;
            } else {
                TQuery Qry(&OraSession);
                Qry.SQLText=
                    "SELECT mark_trips.airline,mark_trips.flt_no,mark_trips.suffix, "
                    "       mark_trips.scd AS scd_out,mark_trips.airp_dep AS airp "
                    "FROM pax_grp,mark_trips "
                    "WHERE pax_grp.point_id_mark=mark_trips.point_id AND pax_grp.grp_id=:grp_id";
                Qry.CreateVariable("grp_id",otInteger,agrp_id);
                Qry.Execute();
                if (!Qry.Eof)
                {
                    TTripInfo markFlt(Qry);
                    TCodeShareSets codeshareSets;
                    codeshareSets.get(operFlt,markFlt);
                    if ( op == TDevOper::PrnBP and codeshareSets.pr_mark_bp )
                    {
                        airline = markFlt.airline;
                        flt_no = markFlt.flt_no;
                        suffix = markFlt.suffix;
                    };
                }
            }
            flt_infants(operFlt.point_id, infants);
        };
        TripsInterface::readGates(operFlt.point_id, gates);
    }
}


string TPrnTagStore::BCBP_V_5(TFieldParams fp)
{
    if(scan_data != NULL)
        return scan;
    BCBPSections barcode;
    //scan_data = barcode;
    string surname = transliter(paxInfo.surname_2d, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    string name = transliter(paxInfo.name_2d, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    barcode.add_section();
    barcode.set_passenger_name_surname(name, surname);
    barcode.set_version(5);
    barcode.set_electronic_ticket_indicator((ETKT(fp).empty() ? " " : "E"));
    TPnrAddrs::const_iterator iv = pnrInfo.pnrs.begin();
    for(; iv != pnrInfo.pnrs.end(); iv++)
        if(pointInfo.airline == iv->airline) {
            ProgTrace(TRACE5, "PNR found: %s", iv->addr.c_str());
            break;
        }
    if(iv != pnrInfo.pnrs.end() && (iv->addr.size() <= 7))
        barcode.set_operatingCarrierPNR(convert_pnr_addr(iv->addr, tag_lang.GetLang() != AstraLocale::LANG_RU), 0);
    barcode.set_from_city_airport(AIRP_DEP(fp), 0);
    barcode.set_to_city_airport(AIRP_ARV(fp), 0);
    barcode.set_operating_carrier_designator(AIRLINE(fp), 0);
    try{
        if(pointInfo.flt_no > 0)
            barcode.set_flight_number(BCBPSectionsEnums::to_string(pointInfo.flt_no), 0 );
        barcode.set_date_of_flight(UTCToLocal(pointInfo.operFlt.scd_out, AirpTZRegion(grpInfo.airp_dep)),0);
    }
    catch(EConvertError e)
    {
    }

    std::string cc = CLASS(fp);
    if(cc.size() == 1)
        barcode.set_compartment_code(cc[0], 0);
    barcode.set_seat_number(ONE_SEAT_NO(fp), 0);
    barcode.set_check_in_seq_number("0", 0);
    barcode.set_passenger_status('1', 0);

    TPerson pers_type = DecodePerson(paxInfo.pers_type.c_str());
    if(pers_type == NoPerson) throw Exception("BCBP_V_5: something wrong with pers_type");
    barcode.set_passenger_description(static_cast<BCBPSectionsEnums::PassengerDescr>(pers_type));
    barcode.set_doc_type(BCBPSectionsEnums::boarding_pass);
    barcode.set_komtech_pax_id(paxInfo.pax_id, 0);


    std::string x  = barcode.build_bcbp_str();
    return x;
}

// Если печатается БП или ваучер, то:
// В 2D-баркоде:
// 1. Ducument type = 'I',
// 2. не пишем pax_id в баркод,
// чтобы нельзя было пройти досмотр/посадку по такому баркоду
// В 1D-баркод забивается getEmptyPaxId()
bool TPrnTagStore::isBoardingPass()
{
#ifdef XP_TESTING
    if(inTestMode()) {
        return op_type == TDevOper::PrnBP;
    }
#endif //XP_TESTING

    return op_type == TDevOper::PrnBP and tag_list[TAG::VOUCHER_CODE].TagInfo.empty();
}

string airp_code_2D(const string &code)
{
    string result = code;
    if(result.size() > 3)
        result = "XXX";
    return result;
}

string TBCBPData::toString(const TTagLang &tag_lang)
{
    ostringstream result;
    result
        << "M"
        << 1;
    // Passenger Name
    result << left;
    string _surname = transliter(surname, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    string _name = transliter(name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    string pax_name = _surname;
    if(!_name.empty())
        pax_name += "/" + _name;
    if(pax_name.size() > 20){
        size_t diff = pax_name.size() - 20;
        if(_name.empty()) {
            result << _surname.substr(0, _surname.size() - diff);
        } else {
            if(_name.size() > diff) {
                _name = _name.substr(0, _name.size() - diff);
            } else {
                diff -= _name.size() - 1;
                _name = _name[0];
                _surname = _surname.substr(0, _surname.size() - diff);
            }
            result << _surname + "/" + _name;
        }
    } else
        result << setw(20) << left << pax_name;

    // Electronic Ticket Indicator
    result << (not etkt ? " " : "E");
    // Operating carrier PNR code
    result << left;
    if(pnr.empty() or pnr.size() > 7)
        result << setw(7) << " ";
    else
        result << setw(7) << convert_pnr_addr(pnr, tag_lang.GetLang() != AstraLocale::LANG_RU);
    // From City Airport Code
    result << setw(3) << airp_code_2D(tag_lang.ElemIdToTagElem(etAirp, airp_dep, efmtCodeNative));
    // To City Airport Code
    result << setw(3) << airp_code_2D(tag_lang.ElemIdToTagElem(etAirp, airp_arv, efmtCodeNative));
    // Operating Carrier Designator
    result << left << setw(3) << tag_lang.ElemIdToTagElem(etAirline, airline, efmtCodeNative);
    // Flight Number
    result
        << right
        << setw(4) << right << setfill('0') << flt_no
        << setw(1) << setfill(' ') << tag_lang.ElemIdToTagElem(etSuffix, suffix, efmtCodeNative);
    // Date of Flight
    JulianDate _scd(scd);
    _scd.trace(__FUNCTION__);
    // Date of flight(Julian Date)
    result << right << setw(3) << setfill('0') << _scd.getJulianDate();
    // Compartment Code
    if(class_grp != NoExists)
        result << tag_lang.ElemIdToTagElem(etClsGrp, class_grp, efmtCodeNative);
    else
        result << tag_lang.ElemIdToTagElem(etClass, cls, efmtCodeNative);
    // Seat Number
    result << setw(4) << right << seat_no;
    // Check-In Sequence Number
    result
        << right << setw(4) <<  setfill('0') << reg_no
        << " ";
    // Passenger Status
    // Я так понимаю что к этому моменту (т.е. вывод пос. талона на печать)
    // статус пассажира "1": passenger checked in
    result << 1;

    ostringstream cond1; // first conditional field
    { // filling up cond1
        cond1
            << ">"
            << 2;
        // field size of following structured message
        // постоянное значение равное сумме зарезервированных длин последующих 7-и полей
        // в данной версии эта длина равна 24 (двадцать четыре)
        cond1 << "18";
        // Passenger Description
        TPerson _pers_type = DecodePerson(pers_type.c_str());
        int result_pers_type = 0;
        switch(_pers_type) {
            case adult:
                result_pers_type = 0;
                break;
            case child:
                result_pers_type = 3;
                break;
            case baby:
                result_pers_type = 4;
                break;
            case NoPerson:
                throw Exception("BCBP_M_2: something wrong with pers_type");
        }
        cond1 << result_pers_type;
        // Source of Check-In
        cond1 << "O";
        // Source of Boarding Pass Issuance
        cond1 << "O";
        // Date of Issue of Boarding Pass (not used)
        cond1 << setw(4) << " ";
        // Document type (B - Boarding Pass, I - Itinerary Receipt)
        cond1 << (is_boarding_pass ? "B" : "I");
        // Airline Designator of Boarding Pass Issuer (not used)
        cond1 << setw(3) << " ";
        // Baggage Tag License Plate Number(s) (not used  because dont know how)
        cond1 << setw(13) << " ";
        // end of 11-length structured message

        // field size of following structured message (41, hex 29)
        cond1 << "00";

        /*  We'll discuss it later

        // field size of following structured message (41, hex 29)
        cond1 << "29";
        // Airline Numeric Code (not used)
        cond1 << setw(3) << " ";
        // Document Form/Serial Number (not used)
        cond1 << setw(10) << " ";
        // Selectee Indicator (not used)
        cond1 << " ";
        // International Documentation Verification (0 - not required)
        cond1 << 0;
        // Marketing carrier designator
        cond1 << setw(3) << setfill(' ') << left << mkt_airline(pax_id);

        //......
        */

        // For individual airline use
        if(pax_id != NoExists)
            cond1 << setw(10) << right << setfill('0') << pax_id;

    }

    // Field size of following varible size field
    result << setw(2) << right << setfill('0') << hex << uppercase << cond1.str().size();
    result << cond1.str();

    string buf = result.str();
    if((tag_lang.get_pr_lat() or tag_lang.english_tag()) and not IsAscii7(buf))
        for(string::iterator si = buf.begin(); si != buf.end(); si++)
            if(not IsAscii7(*si)) *si = 'X';
    return buf;
};

string TPrnTagStore::BCBP_M_2(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<TBCBPData>(fp.TagInfo).toString(tag_lang);
    } else {
        if(scan_data != NULL) {
            return scan;
        } else {
            TBCBPData bcbp_data;
            bcbp_data.surname = paxInfo.surname_2d;
            bcbp_data.name = paxInfo.name_2d;
            bcbp_data.etkt = not ETKT(fp).empty();

            TPnrAddrs::const_iterator iv = pnrInfo.pnrs.begin();
            for(; iv != pnrInfo.pnrs.end(); iv++)
                if(pointInfo.airline == iv->airline) {
                    ProgTrace(TRACE5, "PNR found: %s", iv->addr.c_str());
                    break;
                }
            if(iv != pnrInfo.pnrs.end())
                bcbp_data.pnr = iv->addr;
            bcbp_data.airp_dep = grpInfo.airp_dep;
            bcbp_data.airp_arv = grpInfo.airp_arv;
            bcbp_data.airline = pointInfo.operFlt.airline;
            bcbp_data.flt_no = pointInfo.operFlt.flt_no;
            bcbp_data.suffix = pointInfo.operFlt.suffix;
            bcbp_data.scd = UTCToLocal(pointInfo.operFlt.scd_out, AirpTZRegion(grpInfo.airp_dep));
            bcbp_data.class_grp = grpInfo.class_grp;
            bcbp_data.seat_no = ONE_SEAT_NO(fp);
            bcbp_data.reg_no = paxInfo.reg_no;
            bcbp_data.pers_type = paxInfo.pers_type;
            bcbp_data.is_boarding_pass = isBoardingPass();
            if(isBoardingPass())
                bcbp_data.pax_id = pax_id;
            return bcbp_data.toString(tag_lang);
        }
    }
}

string TPrnTagStore::AGENT(TFieldParams fp)
{
    return TReqInfo::Instance()->user.login;
}

string TPrnTagStore::AIRLINE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airlCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirline, airlCode, efmtCodeNative);
    } else {
        string airline;
        if(scan_data == NULL)
            airline = tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtCodeNative);
        else {
            string buf = StrUtils::trim(scan_data->operating_carrier_designator(0));
            TElemFmt fmt;
            airline = ElemToElemId(etAirline, buf, fmt);
            if (fmt==efmtUnknown)
                airline = buf;
            else
                airline = tag_lang.ElemIdToTagElem(etAirline, airline, efmtCodeNative);
        }
        return airline;
    }
}

string get_date_from_bcbp(TDateTime date, const string &date_format, bool pr_lat)
{
    string result = DateTimeToStr(date, date_format, pr_lat);
    if(date_format != ServerFormatDateTimeAsString) {
        // для формата ServerFormatDateTimeAsString данная проверка не проводится
        // т.к. даты в этом формате отсылаются в swc и они не должны быть пустыми
        string result2 = DateTimeToStr(date + 1./24 + 1./(24*60) + 1./(24*60*60), date_format, pr_lat);
        // Если в формате тега (date_format) участвуют часы, минуты, секунды, то
        // данные не выводить, т.к. в julian date нет такого
        if(result != result2)
            result.clear();
    }
    return result;
}

string get_date_from_bcbp(int julian_date, const string &date_format, bool pr_lat)
{
    return get_date_from_bcbp(get_date_from_bcbp(julian_date), date_format, pr_lat);
}

string TPrnTagStore::ACT(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        TDateTime dt = boost::any_cast<TDateTime>(fp.TagInfo);
        return dt != ASTRA::NoExists ? DateTimeToStr(dt, fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU): "";
    } else {
        if(scan_data != NULL)
            return get_date_from_bcbp(scan_data->date_of_flight(0), fp.date_format,  tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
        {
            return DateTimeToStr(UTCToLocal(pointInfo.operFlt.act_est_scd_out(), AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
}

string TPrnTagStore::AIRLINE_SHORT(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airlCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirline, airlCode, efmtNameShort);
    } else {
        string airline_short;
        if(scan_data == NULL)
            airline_short = tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtNameShort);
        else {
            string buf = StrUtils::trim(scan_data->operating_carrier_designator(0));
            TElemFmt fmt;
            airline_short = ElemToElemId(etAirline, buf, fmt);
            if (fmt==efmtUnknown)
                airline_short = buf;
            else
                airline_short = tag_lang.ElemIdToTagElem(etAirline, airline_short, efmtNameShort);
        }
        return airline_short;
    }
}

string TPrnTagStore::AIRLINE_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airlCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirline, airlCode, efmtNameLong);
    } else {
        string airline_name;
        if(scan_data == NULL)
            airline_name = tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtNameLong);
        else {
            string buf = StrUtils::trim(scan_data->operating_carrier_designator(0));
            TElemFmt fmt;
            airline_name = ElemToElemId(etAirline, buf, fmt);
            if (fmt==efmtUnknown)
                airline_name = buf;
            else
                airline_name = tag_lang.ElemIdToTagElem(etAirline, airline_name, efmtNameLong);
        }
        return airline_name;
    }
}

string TPrnTagStore::AIRP_ARV(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirp, airpCode, efmtCodeNative);
    } else {
        string airp_arv;
        if(scan_data == NULL)
            airp_arv = tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtCodeNative);
        else {
            string buf = StrUtils::trim(scan_data->to_city_airport(0));
            TElemFmt fmt;
            airp_arv = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                airp_arv = buf;
            else
                airp_arv = tag_lang.ElemIdToTagElem(etAirp, airp_arv, efmtCodeNative);
        }
        return airp_arv;
    }
}

string TPrnTagStore::AIRP_ARV_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirp, airpCode, efmtNameLong);
    } else {
        string airp_arv;
        if(scan_data == NULL)
            airp_arv = tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtNameLong);
        else {
            string buf = StrUtils::trim(scan_data->to_city_airport(0));
            TElemFmt fmt;
            airp_arv = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                airp_arv = buf;
            else
                airp_arv = tag_lang.ElemIdToTagElem(etAirp, airp_arv, efmtNameLong);
        }
        return airp_arv;
    }
}

string TPrnTagStore::AIRP_DEP(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirp, airpCode, efmtCodeNative);
    } else {
        string airp_dep;
        if(scan_data == NULL)
            airp_dep = tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtCodeNative);
        else {
            string buf = StrUtils::trim(scan_data->from_city_airport(0));
            TElemFmt fmt;
            airp_dep = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                airp_dep = buf;
            else
                airp_dep = tag_lang.ElemIdToTagElem(etAirp, airp_dep, efmtCodeNative);
        }
        return airp_dep;
    }
}

string TPrnTagStore::AIRP_DEP_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        return tag_lang.ElemIdToTagElem(etAirp, airpCode, efmtNameLong);
    } else {
        string airp_dep;
        if(scan_data == NULL)
            airp_dep = tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtNameLong);
        else {
            string buf = StrUtils::trim(scan_data->from_city_airport(0));
            TElemFmt fmt;
            airp_dep = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                airp_dep = buf;
            else
                airp_dep = tag_lang.ElemIdToTagElem(etAirp, airp_dep, efmtNameLong);
        }
        return airp_dep;
    }
}

string TPrnTagStore::BAGGAGE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data == NULL) {
            ostringstream result;
            if(paxInfo.bag_amount != 0)
                result << paxInfo.bag_amount << "/" << paxInfo.bag_weight;
            return result.str();
        } else
            return string();
    }
}

string TPrnTagStore::RK_AMOUNT(TFieldParams fp)
{
    if(scan_data == NULL) {
        int rk_am = 0;
        if(!fp.TagInfo.empty())
            rk_am = boost::any_cast<int>(fp.TagInfo);
        else
            rk_am = paxInfo.rk_amount;
        return (rk_am == 0 ? "" : IntToString(rk_am));
    } else {
        return string();
    }
}

string TPrnTagStore::RK_WEIGHT(TFieldParams fp)
{
    if(scan_data == NULL) {
        int rk_we = 0;
        if(!fp.TagInfo.empty())
            rk_we = boost::any_cast<int>(fp.TagInfo);
        else
            rk_we = paxInfo.rk_weight;
        return (rk_we == 0 ? "" : IntToString(rk_we));
    } else {
        return string();
    }
}

string TPrnTagStore::BAG_AMOUNT(TFieldParams fp)
{
    int amBag = 0;
    if(!fp.TagInfo.empty())
        amBag = boost::any_cast<int>(fp.TagInfo);
    else if(scan_data == NULL)
        amBag = paxInfo.bag_amount;
    return (amBag == 0 ? "" : IntToString(amBag));
}

string TPrnTagStore::TAGS(TFieldParams fp)
{
    /* !!! TODO
       if(scan_data != NULL)
       return some;
       else
       */
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        return paxInfo.tags;
    }
}

string TPrnTagStore::BAG_WEIGHT(TFieldParams fp)
{
    int bagWeight = 0;
    if(!fp.TagInfo.empty())
        bagWeight = boost::any_cast<int>(fp.TagInfo);
    else if(scan_data == NULL)
        bagWeight = paxInfo.bag_weight;
    return (bagWeight == 0 ? "" : IntToString(paxInfo.bag_weight));
}

string TPrnTagStore::EXCESS(TFieldParams fp)
{
    int exbg_wt = 0;
    if(!fp.TagInfo.empty())
        exbg_wt = boost::any_cast<int>(fp.TagInfo);
    else if(scan_data == NULL)
        exbg_wt = paxInfo.excess_wt.getQuantity();
    return (exbg_wt == 0 ? "" : IntToString(exbg_wt));
}

string TPrnTagStore::BRAND(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data == NULL) {
            ostringstream result;
            if(not paxInfo.brands.empty())
                result << tag_lang.ElemIdToTagElem(etBrand, paxInfo.brands.begin()->id, efmtNameLong);
            return result.str();
        } else
            return string();
    }
}

string TPrnTagStore::BRD_FROM(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else
            return DateTimeToStr(UTCToLocal(brdInfo.brd_from, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
    }
}

string TPrnTagStore::BRD_TO(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else {
            if(brdInfo.brd_to == NoExists) {
                TTripInfo info;
                info.airline = pointInfo.airline;
                info.flt_no = pointInfo.flt_no;
                info.airp = grpInfo.airp_dep;
                if (GetTripSets(tsPrintSCDCloseBoarding, info))
                    brdInfo.brd_to = brdInfo.brd_to_scd;
                else
                    brdInfo.brd_to = (brdInfo.brd_to_est == NoExists ? brdInfo.brd_to_scd : brdInfo.brd_to_est);
            }
            return DateTimeToStr(UTCToLocal(brdInfo.brd_to, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
}

string TPrnTagStore::BT_AMOUNT(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        return "";
    else
        return IntToString(boost::any_cast<int>(fp.TagInfo));
}

string TPrnTagStore::BT_WEIGHT(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        return "";
    else
        return IntToString(boost::any_cast<int>(fp.TagInfo));
}

string TPrnTagStore::CITY_ARV_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",airpCode);
        return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
    } else {
        if(scan_data != NULL) {
            string buf = StrUtils::trim(scan_data->to_city_airport(0));
            TElemFmt fmt;
            string city = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                city = buf;
            else {
                const TAirpsRow& row = (const TAirpsRow&)(base_tables.get("airps").get_row("code",city));
                city = tag_lang.ElemIdToTagElem(etCity, row.city, efmtNameLong);
            }
            return city;
        } else {
            const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_arv);
            return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
        }
    }
}

string TPrnTagStore::CITY_DEP_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string airpCode = boost::any_cast<std::string>(fp.TagInfo);
        const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",airpCode);
        return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
    } else {
        if(scan_data != NULL) {
            string buf = StrUtils::trim(scan_data->from_city_airport(0));
            TElemFmt fmt;
            string city = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                city = buf;
            else {
                const TAirpsRow& row = (const TAirpsRow&)(base_tables.get("airps").get_row("code",city));
                city = tag_lang.ElemIdToTagElem(etCity, row.city, efmtNameLong);
            }
            return city;
        } else {
            const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_dep);
            return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
        }
    }
}

string TPrnTagStore::CLASS(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string cls = boost::any_cast<std::string>(fp.TagInfo);
        return !cls.empty() ? tag_lang.ElemIdToTagElem(etClass, cls, efmtCodeNative) : "";
    } else {
        if(scan_data != NULL) {
            string buf = string(1, scan_data->compartment_code(0));
            TElemFmt fmt;
            string cl = ElemToElemId(etClass, buf, fmt);
            if (fmt==efmtUnknown)
                cl = buf;
            else
                cl = tag_lang.ElemIdToTagElem(etClass, cl, efmtCodeNative);
            return cl;
        } else {
            string result;
            if(grpInfo.class_grp != NoExists)
                result = tag_lang.ElemIdToTagElem(etClsGrp, grpInfo.class_grp, efmtCodeNative);
            return result;
        }
    }
}

string TPrnTagStore::CLASS_NAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string cls = boost::any_cast<std::string>(fp.TagInfo);
        return !cls.empty() ? tag_lang.ElemIdToTagElem(etClass, cls, efmtNameLong) : "";
    }
    if(scan_data != NULL) {
        string buf = string(1, scan_data->compartment_code(0));
        TElemFmt fmt;
        string cl = ElemToElemId(etClass, buf, fmt);
        if (fmt==efmtUnknown)
            cl = buf;
        else
            cl = tag_lang.ElemIdToTagElem(etClass, cl, efmtNameLong);
        return cl;
    } else {
        string result;
        if(grpInfo.class_grp != NoExists)
            result = tag_lang.ElemIdToTagElem(etClsGrp, grpInfo.class_grp, efmtNameLong);
        return result;
    }
}

string TPrnTagStore::DESK(TFieldParams fp)
{
    return TReqInfo::Instance()->desk.code;
}

string TPrnTagStore::DOCUMENT(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else
            return paxInfo.document;
    }
}

string TPrnTagStore::DUPLICATE(TFieldParams fp)
{
    bool pr_bp = false;
    if(!fp.TagInfo.empty()) {
        pr_bp = boost::any_cast<int>(fp.TagInfo);
    } else {
        pr_bp = paxInfo.pr_bp_print;
    }
    string result;
    if(pr_bp)
        result = getLocaleText("ДУБЛИКАТ", tag_lang.GetLang());
    return result;
}

string TPrnTagStore::EST(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        TDateTime est = boost::any_cast<TDateTime>(fp.TagInfo);
        return est != ASTRA::NoExists ? DateTimeToStr(est, fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU) : "";
    } else {
        if(scan_data != NULL) {
            return get_date_from_bcbp(scan_data->date_of_flight(0), fp.date_format,  tag_lang.GetLang() != AstraLocale::LANG_RU);
        } else {
            return DateTimeToStr(UTCToLocal(pointInfo.operFlt.est_scd_out(), AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
}

string TPrnTagStore::ETICKET_NO(TFieldParams fp) // !!! lat ???
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL) {
            boost::optional<int> airline_num_code = scan_data->airline_num_code(0);
            string doc_num = scan_data->doc_serial_num(0);
            ostringstream result;
            if(airline_num_code != boost::none and not doc_num.empty())
                result << doc_num << *airline_num_code;
            return result.str();
        } else {
            ostringstream result;
            if(paxInfo.ticket_rem == "TKNE")
                result << paxInfo.ticket_no << "/" << paxInfo.coupon_no;
            return result.str();
        }
    }
}

string TPrnTagStore::TICKET_NO(TFieldParams fp)
{
    string result = ETICKET_NO(fp);
    if(result.empty())
        result = paxInfo.ticket_no;
    return result;
}

string TPrnTagStore::ETKT(TFieldParams fp)
{
    string result = ETICKET_NO(fp);
    if(not result.empty())
        result = "ETKT" + result;
    return result;
}

string TPrnTagStore::FLT_NO(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const int flt_no = boost::any_cast<int>(fp.TagInfo);
        ostringstream result;
        result << setw(3) << setfill('0') << flt_no;
        return result.str();
    } else {
        if(scan_data != NULL) {
            pair<int, char> bcbp_flt_no = scan_data->flight_number(0);
            string buf;
            buf.append(1, bcbp_flt_no.second);
            TElemFmt fmt;
            string suffix = ElemToElemId(etSuffix, buf, fmt);
            if (fmt==efmtUnknown)
                suffix = buf;
            ostringstream result;
            result << setw(3) << setfill('0') << bcbp_flt_no.first << (fmt == efmtUnknown ? suffix : tag_lang.ElemIdToTagElem(etSuffix, suffix, efmtCodeNative));
            return result.str();
        } else {
            ostringstream result;
            result << setw(3) << setfill('0') << pointInfo.flt_no << tag_lang.ElemIdToTagElem(etSuffix, pointInfo.suffix, efmtCodeNative);
            return result.str();
        }
    }
}

string TPrnTagStore::FQT_TIER_LEVEL(TFieldParams fp)
{
    if(!fp.TagInfo.empty())
        return boost::any_cast<std::string>(fp.TagInfo);
    else
        return transliter(fqtInfo.tier_level, 1,tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::FQT(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        string result;
        if(not fqtInfo.airline.empty()) {
            string airline = tag_lang.ElemIdToTagElem(etAirline, fqtInfo.airline, efmtCodeNative);
            string extra = transliter(fqtInfo.extra, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
            result = (airline + " " + fqtInfo.no + string(" ", extra.empty() ? 0 : 1) + extra);
            size_t min_width = airline.size() + fqtInfo.no.size() + 1;
            if(fp.len == 0)
                ;
            else if(min_width > fp.len)
                result = fqtInfo.no;
            else
                result = result.substr(0, fp.len > min_width ? fp.len : fp.len == 0 ? string::npos : min_width);
        }
        return result;
    }
}

string cut_full_place(string city, string airp, size_t len)
{
    return
        (city == airp ? city : city + " " + airp)
        .substr(0, len > 17 ? len : len == 0 ? string::npos : 17);
}

string TPrnTagStore::FULL_PLACE_ARV(TFieldParams fp)
{
    return cut_full_place(CITY_ARV_NAME(fp), AIRP_ARV_NAME(fp), fp.len);
}

string TPrnTagStore::FULL_PLACE_DEP(TFieldParams fp)
{
    return cut_full_place(CITY_DEP_NAME(fp), AIRP_DEP_NAME(fp), fp.len);
}

string TPrnTagStore::FULLNAME(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        const std::string fullnm = boost::any_cast<std::string>(fp.TagInfo);
        return transliter(fullnm, 1, tag_lang.GetLang() != AstraLocale::LANG_RU)
            .substr(0, fp.len > 10 ? fp.len : fp.len == 0 ? string::npos : 10);
    } else {
        string name, surname;
        if(scan_data != NULL) {
            surname = scan_data->unique.passengerName().first;
            name = scan_data->unique.passengerName().second;
        } else {
            surname = paxInfo.surname;
            name = paxInfo.name;
        }
        return
            transliter(name.empty() ? surname : surname + " " + name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU)
            .substr(0, fp.len > 10 ? fp.len : fp.len == 0 ? string::npos : 10);
    }
}

string TPrnTagStore::GATE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL) {
            return string();
        } else {
            if(fp.TagInfo.empty() && TReqInfo::Instance()->client_type == ctTerm)
                throw AstraLocale::UserException("MSG.GATE_NOT_SPECIFIED");
            return boost::any_cast<string>(fp.TagInfo);
        }
    }
}

string TPrnTagStore::GATES(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        string result;
        for(vector<string>::const_iterator iv = pointInfo.gates.begin(); iv != pointInfo.gates.end(); iv++)
        {
            size_t next_len = result.size() + iv->size() + 1;
            if(fp.len != 0 and not result.empty() and (next_len > fp.len)) {
                int diff = fp.len - result.size();
                if(diff  > 3)
                    result += ('/' + *iv ).substr(0, diff - 3) + "...";
                else if(diff == 3)
                    result += "...";
                else {
                    if(iv == pointInfo.gates.begin() + 1)
                        result = *iv + "...";
                    else
                        result = result.substr(0, result.size() - 3 + diff) + "...";
                }
                break;
            }
            if(not result.empty()) result += '/';
            result += *iv;
        }
        return result;
    }
}

string TPrnTagStore::CHD(TFieldParams fp)
{
    string result;
    if(DecodePerson(paxInfo.pers_type.c_str()) == child)
        result = tag_lang.ElemIdToTagElem(etPersType, paxInfo.pers_type, efmtCodeNative);
    return result;
}

string TPrnTagStore::HALL(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data == NULL) {
            ostringstream result;
            result << tag_lang.ElemIdToTagElem(etHall, grpInfo.hall, efmtNameLong);
            return result.str();
        } else
            return string();
    }
}

string TPrnTagStore::IMG(TFieldParams fp)
{
    string result;
    return result;
}

string TPrnTagStore::INF(TFieldParams fp)
{
    std::string pers_type;
    if(!fp.TagInfo.empty()) {
        pers_type = boost::any_cast<std::string>(fp.TagInfo);
    } else {
        pers_type = paxInfo.pers_type;
    }
    string result;
    if(DecodePerson(pers_type.c_str()) == baby)
        result = tag_lang.ElemIdToTagElem(etPersType, pers_type, efmtCodeNative);
    return result;
}

string cut_place(string airp, string city_name, int len)
{
    if(not city_name.empty() and not airp.empty()) {
        ptrdiff_t diff = len - (airp.size() + 2);
        if(diff < 0) diff = string::npos;
        if(diff >= 3 or len == 0)
            return city_name.substr(0, diff) + "(" + airp + ")";
        else
            return airp;
    } else if(city_name.empty())
        return airp;
    else {
        if(len == 0)
            return city_name;
        else
            return city_name.substr(0, (len < 3 ? 3 : len));
    }
}

string cut_long_place(string city1, string airp1, string city2, string airp2, int len)
{
    string result;
    if(city1 == city2 and airp1 == airp2)
        result = cut_place(airp1, city1, len);
    else {
        string part1, part2;
        if(city1 == city2) city2.erase();
        if(airp1 == airp2) airp2.erase();

        part1 = city1 + "(" + airp1 + ")" + "/";
        if(city2.empty())
            part2 = airp2;
        else
            part2 = city2 + (airp2.empty() ? airp2 : "(" + airp2 + ")");

        int diff = len - part1.size();
        if(diff >= (int)part2.size() or len == 0)
            result = part1 + part2;
        else {
            if(diff == 0) { // потому что cut_place в сл-е 0 возвращает полное название
                if(airp2.empty())
                    part2 = city2.substr(0, 3);
                else
                    part2 = airp2;
            } else
                part2 = cut_place(airp2, city2, diff);
            diff = len - part2.size();
            if(diff >= (int)part1.size())
                result = part1 + part2;
            else {
                result = cut_place(airp1, city1, diff - 1) + "/" + part2;
                if(result.size() > (size_t)len)
                    result = airp1;
            }
        }
    }
    return result;
}

string TPrnTagStore::LONG_ARV(TFieldParams fp)
{
    string result;
    if(tag_lang.GetLang() != AstraLocale::LANG_RU) {
        result = cut_place(AIRP_ARV(fp), CITY_ARV_NAME(fp), fp.len);
    } else {
        string airp_arv;
        if(scan_data != NULL) {
            string buf = StrUtils::trim(scan_data->to_city_airport(0));
            TElemFmt fmt;
            airp_arv = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                return buf;
        } else
            if(!fp.TagInfo.empty())
                airp_arv = boost::any_cast<std::string>(fp.TagInfo);
            else
                airp_arv = grpInfo.airp_arv;

        const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",airp_arv);
        result = cut_long_place(
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etAirp, airp_arv, efmtCodeNative, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, AstraLocale::LANG_EN),
                tag_lang.ElemIdToTagElem(etAirp, airp_arv, efmtCodeNative, AstraLocale::LANG_EN),
                fp.len
                );
    }
    return result;
}

string TPrnTagStore::LONG_DEP(TFieldParams fp)
{
    string result;
    if(tag_lang.GetLang() != AstraLocale::LANG_RU) {
        result = cut_place(AIRP_DEP(fp), CITY_DEP_NAME(fp), fp.len);
    } else {
        string airp_dep;
        if(scan_data != NULL) {
            string buf = StrUtils::trim(scan_data->to_city_airport(0));
            TElemFmt fmt;
            airp_dep = ElemToElemId(etAirp, buf, fmt);
            if (fmt==efmtUnknown)
                return buf;
        } else
            if(!fp.TagInfo.empty())
                airp_dep = boost::any_cast<std::string>(fp.TagInfo);
            else
                airp_dep = grpInfo.airp_dep;
        const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("AIRPS").get_row("code",airp_dep);
        result = cut_long_place(
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etAirp, airp_dep, efmtCodeNative, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, AstraLocale::LANG_EN),
                tag_lang.ElemIdToTagElem(etAirp, airp_dep, efmtCodeNative, AstraLocale::LANG_EN),
                fp.len
                );
    }
    return result;
}

string TPrnTagStore::NAME(TFieldParams fp)
{
    string result;
    if(scan_data != NULL) {
        result = transliter(scan_data->unique.passengerName().second, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    } else {
        if(fp.TagInfo.empty())
            result = transliter(paxInfo.name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
            result = SURNAME(fp);
    }
    return result;
}

string TPrnTagStore::NO_SMOKE(TFieldParams fp)
{
    if(scan_data != NULL)
        return string();
    else {
        bool pr_smoke = false;
        if(!fp.TagInfo.empty()) {
            pr_smoke = boost::any_cast<int>(fp.TagInfo);
        } else {
            pr_smoke = paxInfo.pr_smoke;
        }
        return (pr_smoke ? " " : "X");
    }
}

string TPrnTagStore::ONE_SEAT_NO(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        return get_fmt_seat("one", tag_lang.english_tag());
    }
}

string TPrnTagStore::PAX_ID(TFieldParams fp)
{
    ostringstream result;
    if(not isBoardingPass())
        result << setw(10) << setfill('0') << getEmptyPaxId();
    else if(!fp.TagInfo.empty()) {
        int paxId = boost::any_cast<int>(fp.TagInfo);
        if(paxId == ASTRA::NoExists || paxId == -1)
            paxId = getEmptyPaxId();
        result << setw(10) << setfill('0') << paxId;
    } else {
        result << setw(10) << setfill('0') << (scan_data != NULL?getEmptyPaxId():paxInfo.pax_id);
    }
    return result.str();
}

string TPrnTagStore::PLACE_ARV(TFieldParams fp)
{
    return cut_place(AIRP_ARV(fp), CITY_ARV_NAME(fp), fp.len);
}

string TPrnTagStore::PLACE_DEP(TFieldParams fp)
{
    return cut_place(AIRP_DEP(fp), CITY_DEP_NAME(fp), fp.len);
}

string TPrnTagStore::TRfiscDescr::get(const string &crs_cls, TBPServiceTypes::Enum code, TPrnTagStore &pts)
{
    if(code == TBPServiceTypes::UP and pts.paxInfo.pers_type == "РМ") {
        // найдем родителя
        vector<TInfantAdults>::iterator i = pts.pointInfo.infants.begin();
        for(; i != pts.pointInfo.infants.end(); i++)
            if(i->pax_id == pts.paxInfo.pax_id) break;
        string result;
        if(i != pts.pointInfo.infants.end()) {
            CheckIn::TSimplePaxItem parent_pax;
            parent_pax.getByPaxId(i->parent_pax_id);
            TPrnTagStore parent_pts(
                    pts.get_op_type(),
                    parent_pax.grp_id,
                    parent_pax.id,
                    false, false, NULL);
            if(
                    not StrUtils::trim(parent_pts.get_tag(TAG::RFISC_UPGRADE)).empty() or
                    not StrUtils::trim(parent_pts.get_tag(TAG::RFISC_BSN_LONGUE)).empty()
              )
                result = TBPServiceTypesDescr().encode(TBPServiceTypes::UP);
        }
        return result;
    } else {
        string tag_bi_rule = StrUtils::trim(pts.get_tag(TAG::BI_HALL));
        if(
                not (
                    found_services.find(TBPServiceTypes::UP) != found_services.end() and
                    crs_cls == "Б")
                and (
                    (code == TBPServiceTypes::LG) and not tag_bi_rule.empty()
                    or
                    (code == TBPServiceTypes::TS_FT) and
                    (crs_cls == "Б" or crs_cls == "П" or not tag_bi_rule.empty())
                    )
          )
            return TBPServiceTypesDescr().encode(code);
        else
            return (found_services.find(code) != found_services.end() ? TBPServiceTypesDescr().encode(code) : "");
    }
}

void TPrnTagStore::TRfiscDescr::dump() const
{
    LogTrace(TRACE5) << "---TPrnTagStore::TRfiscDescr::dump---";
    for(const auto i: found_services)
        LogTrace(TRACE5) << TBPServiceTypesCode().encode(i);
    LogTrace(TRACE5) << "-------------------------------------";
}

void TPrnTagStore::TRfiscDescr::fromDB(int grp_id, int pax_id)
{
    LogTrace(TRACE5) << "TPrnTagStore::TRfiscDescr::fromDB BEGIN";
    LogTrace(TRACE5) << "grp_id: " << grp_id;
    LogTrace(TRACE5) << "pax_id: " << pax_id;
    clear();

    CheckIn::TServicePaymentListWithAuto paid_services; // оплаченные услуги
    TPaidRFISCListWithAuto registered_services;         // зарегистрирванные услуги

    paid_services.fromDB(grp_id);
    registered_services.fromDB(pax_id, false);

    paid_services.dump(__FILE__, __LINE__);

    for(const auto &service_type: TBPServiceTypes::pairsCodes()) {
        if(service_type.first == TBPServiceTypes::Unknown) continue;
        string grp, subgrp;

        if(service_type.first == TBPServiceTypes::TS_FT) {
            vector<string> tokens;
            boost::split(tokens, service_type.second, boost::is_any_of(" "));
            if(tokens.size() != 2)
                throw Exception("%s: wrong service type code: %s", __FUNCTION__, service_type.second.c_str());
            grp = tokens[0];
            subgrp = tokens[1];
        } else
            grp = service_type.second;

        // Пробег по всем RFISC группы
        // если хотя бы одна оплачена return true
        if(paid_services.isRFISCGrpExists(pax_id, grp, subgrp))
            found_services.insert(service_type.first);
        // Пробег по всем RFISC группы
        // если хотя бы одна не оплачена return true
        else if(registered_services.isRFISCGrpNeedForPayment(pax_id, grp, subgrp)) {
            throw UserException("MSG.UNPAID_SERVICE_EXISTS");
        }
    }
    dump();
}

string TPrnTagStore::REM_TXT(TFieldParams fp)
{
    int tag_index = ToInt(fp.tag_name.substr(fp.tag_name.size() - 1));
    return get_rem_txt(pointInfo.airline, paxInfo.pax_id, tag_index);
}

string TPrnTagStore::RFISC_BSN_LONGUE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) return boost::any_cast<std::string>(fp.TagInfo);
    return rfisc_descr.get(paxInfo.crs_cls, TBPServiceTypes::LG, *this);
}

string TPrnTagStore::RFISC_FAST_TRACK(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) return boost::any_cast<std::string>(fp.TagInfo);
    return rfisc_descr.get(paxInfo.crs_cls, TBPServiceTypes::TS_FT, *this);
}

string TPrnTagStore::RFISC_UPGRADE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) return boost::any_cast<std::string>(fp.TagInfo);
    return rfisc_descr.get(paxInfo.crs_cls, TBPServiceTypes::UP, *this);
}

string TPrnTagStore::REM(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else
            return GetRemarkStr(remInfo.rem, pax_id, tag_lang.GetLang(), " ");
    }
}

string TPrnTagStore::REG_NO(TFieldParams fp)
{
    ostringstream result;
    int regno = NoExists;

    if(scan_data != NULL)
        regno = ToInt(scan_data->check_in_seq_number(0));
    else if(!fp.TagInfo.empty()) {
        regno = boost::any_cast<int>(fp.TagInfo);
    } else {
        regno = paxInfo.reg_no;
    }

    if(regno != NoExists)
        result << setw(3) << setfill('0') << regno;
    return result.str();
}

string TPrnTagStore::RSTATION(TFieldParams fp)
{
    return rstationInfo.name;
}

string TPrnTagStore::SCD(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        TDateTime dt = boost::any_cast<TDateTime>(fp.TagInfo);
        return dt != ASTRA::NoExists ? DateTimeToStr(dt, fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU) : "";
    } else {
        if(scan_data != NULL)
            return get_date_from_bcbp(scan_data->date_of_flight(0), fp.date_format,  tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
            return DateTimeToStr(UTCToLocal(pointInfo.operFlt.scd_out, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
    }
}

string TPrnTagStore::SEAT_NO(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return scan_data->seat_number(0);
        else
            return get_fmt_seat("seats", tag_lang.english_tag());
    }
}

string TPrnTagStore::SUBCLS(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else
            return tag_lang.ElemIdToTagElem(etSubcls, paxInfo.subcls, efmtCodeNative);
    }
}

string TPrnTagStore::STR_SEAT_NO(TFieldParams fp)
{
    if(scan_data != NULL || !fp.TagInfo.empty())
        return SEAT_NO(fp);
    else
        return get_fmt_seat("voland", tag_lang.english_tag());
}

string TPrnTagStore::get_fmt_seat(string fmt, bool english_tag)
{
    TQuery Qry(&OraSession);
    if (!isTestPaxId(paxInfo.pax_id))
    {
        Qry.SQLText =
            "select "
            "   salons.get_seat_no(:pax_id,:seats,:is_jmp,NULL,NULL,:fmt,NULL,:is_inter) AS seat_no "
            "from dual";
        Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
        Qry.CreateVariable("seats", otInteger, paxInfo.seats);
        Qry.CreateVariable("is_jmp", otInteger, (int)paxInfo.is_jmp);
        Qry.CreateVariable("fmt", otString, fmt);

        Qry.CreateVariable("is_inter", otInteger, 0);
        Qry.Execute();
        if ((tag_lang.get_pr_lat() or english_tag) && not IsAscii7(Qry.FieldAsString("seat_no")))
        {
            Qry.SetVariable("is_inter",1);
            Qry.Execute();
        }
        return Qry.FieldAsString("seat_no");
    }
    else
    {
        Qry.SQLText =
            "SELECT seat_xname, seat_yname FROM test_pax WHERE id=:pax_id";
        Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
        Qry.Execute();
        if (Qry.Eof || Qry.FieldIsNULL("seat_yname") || Qry.FieldIsNULL("seat_xname")) return "";
        TSeat seat(Qry.FieldAsString("seat_yname"),Qry.FieldAsString("seat_xname"));
        string result=GetSeatView(seat, lowerc(fmt), false);
        if ((tag_lang.get_pr_lat() or english_tag) && not IsAscii7(result))
            result=GetSeatView(seat, lowerc(fmt), true);
        return result;
    };
}

string TPrnTagStore::LIST_SEAT_NO(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        return get_fmt_seat("list", tag_lang.english_tag());
    }
}

string get_unacc_name(int bag_type, TTagLang &tag_lang)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   decode(:is_inter, 0, name, name_lat) name "
        "from "
        "   unaccomp_bag_names "
        "where "
        "   code = :code ";
    Qry.CreateVariable("is_inter", otInteger, tag_lang.GetLang() != AstraLocale::LANG_RU);
    Qry.CreateVariable("code", otInteger, bag_type);
    Qry.Execute();
    string result;
    if(Qry.Eof)
        result = getLocaleText("БЕЗ СОПРОВОЖДЕНИЯ", tag_lang.GetLang());
    else
        result = Qry.FieldAsString("name");
    return result;
}

string TPrnTagStore::SURNAME(TFieldParams fp)
{
    string result;
    if(scan_data != NULL) {
        result = transliter(scan_data->unique.passengerName().first, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    } else if(fp.TagInfo.empty())
        result = transliter(paxInfo.surname, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    else {
        if(boost::any_cast<int>(&fp.TagInfo))
            result = get_unacc_name(boost::any_cast<int>(fp.TagInfo), tag_lang);
        else if(boost::any_cast<std::string>(&fp.TagInfo))
            result = transliter(boost::any_cast<std::string>(fp.TagInfo), 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
            throw Exception("TPrnTagStore::SURNAME: unexpected TagInfo type");
    }
    return result.substr(0, fp.len > 8 ? fp.len : fp.len == 0 ? string::npos : 8);
}

string TPrnTagStore::PAX_TITLE(TFieldParams fp)
{
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return string();
        else {
            const std::string pt = paxInfo.pers_type;
            TPerson pers_type = DecodePerson(pt.c_str());
            string result;
            switch(pers_type) {
                case adult:
                    {
                        int is_female = CheckIn::is_female(paxInfo.doc.gender, "");
                        if(is_female == NoExists) // по умолчанию
                            result = "Г-Н";
                        else {
                            result = (is_female != 0 ? "Г-ЖА" : "Г-Н");
                        }
                    }
                    break;
                case child:
                    result = "РБ";
                    break;
                case baby:
                    result = "РМ";
                    break;
                case NoPerson:
                    throw Exception("PAX_TITLE: something wrong with pers_type");
            }
            return getLocaleText(result, tag_lang.GetLang());
        }
    }
}

string TPrnTagStore::TIME_PRINT(TFieldParams fp)
{
    if(scan_data != NULL) {
        boost::optional<TDateTime> time_print = scan_data->date_of_boarding_pass_issuance();
        if(time_print != boost::none) {
            return get_date_from_bcbp(*time_print, fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
        } else {
            return string();
        }
    } else {
        TReqInfo *reqInfo = TReqInfo::Instance();
        return DateTimeToStr(UTCToLocal(time_print.val, reqInfo->desk.tz_region), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
    }
}

string TPrnTagStore::TEST_SERVER(TFieldParams fp)
{
    if(fp.len == 0) fp.len = 300;
    string result;
    if(get_test_server()) {
        string test = getLocaleText("CAP.TEST", tag_lang.GetLang());
        while(result.size() < fp.len)
            result += test + " ";
    }
    return result.substr(0, fp.len);
}

string TPrnTagStore::AIRCODE(TFieldParams fp) {
    ostringstream result;
    result << setw(4) << setfill('0') << boost::any_cast<int>(fp.TagInfo);
    return result.str();
}

string TPrnTagStore::NO(TFieldParams fp) {
    ostringstream result;
    result << setw(6) << setfill('0') << boost::any_cast<int>(fp.TagInfo);
    return result.str();
}

string TPrnTagStore::ISSUED(TFieldParams fp) {
    return DateTimeToStr(boost::any_cast<TDateTime>(fp.TagInfo), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::LIAB_LIMIT(TFieldParams fp) {
    return getLocaleText(boost::any_cast<string>(fp.TagInfo), tag_lang.GetLang());
}

string TPrnTagStore::FLT_NO1(TFieldParams fp) {
    string flt_no = boost::any_cast<string>(fp.TagInfo);
    string suffix;
    if(not flt_no.empty()) {
        if(not IsDigit(flt_no[flt_no.size() - 1])) {
            suffix = tag_lang.ElemIdToTagElem(etSuffix, flt_no.substr(flt_no.size() - 1), efmtCodeNative);
            flt_no.erase(flt_no.size() - 1, 1);
        }
    };
    ostringstream result;
    result << setw(3) << setfill('0') << flt_no << suffix;
    return result.str();
}

string TPrnTagStore::FLT_NO2(TFieldParams fp) {
    return FLT_NO1(fp);
}

string TPrnTagStore::FLT_NO3(TFieldParams fp) {
    return FLT_NO1(fp);
}

string TPrnTagStore::LOCAL_DATE1(TFieldParams fp) {
    TDateTime scd = boost::any_cast<TDateTime>(fp.TagInfo);
    int Year, Month, Day;
    DecodeDate(scd, Year, Month, Day);
    ostringstream result;
    result << setw(2) << setfill('0') << Day;
    return result.str();
}

string TPrnTagStore::LOCAL_DATE2(TFieldParams fp) {
    return LOCAL_DATE1(fp);
}

string TPrnTagStore::LOCAL_DATE3(TFieldParams fp) {
    return LOCAL_DATE1(fp);
}

string TPrnTagStore::AIRLINE1(TFieldParams fp) {
    return tag_lang.ElemIdToTagElem(etAirline, boost::any_cast<string>(fp.TagInfo), efmtCodeNative);
}

string TPrnTagStore::AIRLINE2(TFieldParams fp) {
    return AIRLINE1(fp);
}

string TPrnTagStore::AIRLINE3(TFieldParams fp) {
    return AIRLINE1(fp);
}

string TPrnTagStore::AIRP_ARV1(TFieldParams fp) {
    return tag_lang.ElemIdToTagElem(etAirp, boost::any_cast<string>(fp.TagInfo), efmtCodeNative);
}

string TPrnTagStore::AIRP_ARV2(TFieldParams fp) {
    return AIRP_ARV1(fp);
}

string TPrnTagStore::AIRP_ARV3(TFieldParams fp) {
    return AIRP_ARV1(fp);
}

string TPrnTagStore::FLTDATE1(TFieldParams fp) {
    return DateTimeToStr(boost::any_cast<TDateTime>(fp.TagInfo), (string)"ddmmm", tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::FLTDATE2(TFieldParams fp) {
    return FLTDATE1(fp);
}

string TPrnTagStore::FLTDATE3(TFieldParams fp) {
    return FLTDATE1(fp);
}

string TPrnTagStore::AIRP_ARV_NAME1(TFieldParams fp) {
    return tag_lang.ElemIdToTagElem(etAirp, boost::any_cast<string>(fp.TagInfo), efmtNameLong);
}

string TPrnTagStore::AIRP_ARV_NAME2(TFieldParams fp) {
    return AIRP_ARV_NAME1(fp);
}

string TPrnTagStore::AIRP_ARV_NAME3(TFieldParams fp) {
    return AIRP_ARV_NAME1(fp);
}

string TPrnTagStore::BI_RULE_GUEST(TFieldParams fp) {
    string result = BI_RULE(fp);
    if(not result.empty())
        result += getLocaleText("гость", tag_lang.GetLang());
    return result;
}

string TPrnTagStore::BI_RULE(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        const BIPrintRules::TRule &rule = boost::any_cast<BIPrintRules::TRule>(fp.TagInfo);
        if(rule.exists() and rule.print_type == BIPrintRules::TPrintType::OnePlusOne)
                result << "+1";
    }
    return result.str();
}

string TPrnTagStore::VOUCHER_CODE(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        string code = boost::any_cast<string>(fp.TagInfo);
        result << transliter(
                tag_lang.ElemIdToTagElem(etVoucherType, code, efmtCodeNative),
                1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    }
    return result.str();
}

string TPrnTagStore::VOUCHER_TEXT_FREE(TFieldParams fp) {
    string result;
    if(not tag_list[TAG::VOUCHER_CODE].TagInfo.empty())
        result = fp.text;
    return result;
}

string TPrnTagStore::VOUCHER_TEXT(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        string code = boost::any_cast<string>(fp.TagInfo);
        result << transliter(
                tag_lang.ElemIdToTagElem(etVoucherType, code, efmtNameLong),
                1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    }
    return result.str();
}

string TPrnTagStore::BI_AIRP_TERMINAL(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        const BIPrintRules::TRule &rule = boost::any_cast<BIPrintRules::TRule>(fp.TagInfo);
        if(rule.exists() and not rule.halls.empty()) {
            BIHallInfo.hall_id = rule.halls.begin()->first;
            BIHallInfo.airp_terminal_id = rule.halls.begin()->second;
            result << transliter(
                    tag_lang.ElemIdToTagElem(etAirpTerminal, BIHallInfo.airp_terminal_id, efmtNameLong),
                    1, tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
    return result.str();
}

string TPrnTagStore::BI_HALL_CAPTION(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        const BIPrintRules::TRule &rule = boost::any_cast<BIPrintRules::TRule>(fp.TagInfo);
        if(rule.exists()) {
            BIHallInfo.hall_id = rule.halls.begin()->first;
            result << upperc(getLocaleText("Бизнес зал", tag_lang.GetLang()));
        }
    }
    return result.str();
}

string TPrnTagStore::BI_HALL(TFieldParams fp) {
    ostringstream result;
    if(!fp.TagInfo.empty()) {
        const BIPrintRules::TRule &rule = boost::any_cast<BIPrintRules::TRule>(fp.TagInfo);
        if(rule.exists() and not rule.halls.empty()) {
            BIHallInfo.hall_id = rule.halls.begin()->first;
            int hall = rule.halls.begin()->first;
            result << transliter(tag_lang.ElemIdToTagElem(etBIHall, hall, efmtNameLong), 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
    return result.str();
}

string TPrnTagStore::PNR(TFieldParams fp) {
    if(!fp.TagInfo.empty()) {
        return boost::any_cast<std::string>(fp.TagInfo);
    } else {
        if(scan_data != NULL)
            return scan_data->operatingCarrierPNR(0);
        else {
            string pnr;
            if (!pnrInfo.pnrs.empty()) {
                pnr = convert_pnr_addr(pnrInfo.pnrs[0].addr, tag_lang.GetLang() != AstraLocale::LANG_RU);
                if(pnrInfo.pnrs[0].airline!=pnrInfo.airline and pnr.size() + pnrInfo.pnrs[0].airline.size() + 1 <= fp.len)
                    pnr += "/" + tag_lang.ElemIdToTagElem(etAirline, pnrInfo.pnrs[0].airline, efmtCodeNative);
            }
            return pnr;
        }
    }
}

string TPrnTagStore::BULKY_BT(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and (rcpt.bag_type == 1 || rcpt.bag_type == 2))
        result = "x";
    return result;
}

string TPrnTagStore::BULKY_BT_LETTER(TFieldParams fp)
{
    ostringstream result;
    if(rcpt.pay_bt() and (rcpt.bag_type == 1 || rcpt.bag_type == 2))
        result << rcpt.ex_amount;
    return result.str();
}

string TPrnTagStore::GOLF_BT(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and rcpt.bag_type == 21 and rcpt.form_type != "Z61")
        result = "x";
    return result;
}

string TPrnTagStore::OTHER_BT(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and rcpt.pr_other())
        result = "x";
    return result;
}

string TPrnTagStore::OTHER_BT_LETTER(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and rcpt.pr_other())
        result = BAG_NAME(fp);
    return result;
}

string TPrnTagStore::PET_BT(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and rcpt.bag_type == 4)
        result = "x";
    return result;
}

string TPrnTagStore::SKI_BT(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt() and rcpt.bag_type == 20)
        result = "x";
    return result;
}

string TPrnTagStore::VALUE_BT(TFieldParams fp)
{
    string result;
    if(rcpt.service_type == 3)
        result = "x";
    return result;
}

int separate_double(double d, int precision, int *iptr)
{
    double pd;
    int pi;
    switch (precision)
    {
        case 0: pd=1.0;     pi=1;     break;
        case 1: pd=10.0;    pi=10;    break;
        case 2: pd=100.0;   pi=100;   break;
        case 3: pd=1000.0;  pi=1000;  break;
        case 4: pd=10000.0; pi=10000; break;
        default: throw Exception("separate_double: wrong precision %d",precision);
    };
    int i=int(round(d*pd));
    if (iptr!=NULL) *iptr=i/pi;
    return i%pi;
};

string TPrnTagStore::VALUE_BT_LETTER(TFieldParams fp)
{
    string result;
    if(rcpt.service_type == 3) {
        int iptr, fract;
        fract=separate_double(rcpt.rate, 2, &iptr);

        string buf = vs_number(iptr, tag_lang.GetLang() != AstraLocale::LANG_RU);

        if(fract!=0) {
            string str_fract = IntToString(fract) + "/100 ";
            buf += str_fract;
        }
        result = upperc(buf) + tag_lang.ElemIdToTagElem(etCurrency, rcpt.rate_cur, efmtCodeNative);
    }
    return result;
}

string TPrnTagStore::BR_AIRCODE(TFieldParams fp)
{
    return rcpt.aircode;
}

string TPrnTagStore::BR_AIRLINE(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT code FROM airlines WHERE aircode=:aircode AND pr_del=0";
    Qry.CreateVariable("aircode", otString, rcpt.aircode);
    Qry.Execute();
    if (Qry.Eof) return "";
    string airline=Qry.FieldAsString("code");
    Qry.Next();
    if (!Qry.Eof) throw Exception("TPrnTagStore::BR_AIRLINE: aircode %s duplicated", rcpt.aircode.c_str());

    vector<pair<TElemFmt, string> > fmts;
    fmts.push_back(make_pair(efmtNameShort, tag_lang.GetLang()));
    fmts.push_back(make_pair(efmtNameLong, tag_lang.GetLang()));
    fmts.push_back(make_pair(efmtNameShort, AstraLocale::LANG_RU));
    fmts.push_back(make_pair(efmtNameLong, AstraLocale::LANG_RU));
    return ElemIdToElem(etAirline, airline, fmts);
}

string TPrnTagStore::AIRLINE_CODE(TFieldParams fp)
{
    ostringstream result;
    result << tag_lang.ElemIdToTagElem(etAirline, rcpt.airline, efmtCodeNative);
    if(rcpt.form_type != FT_FV_451 and rcpt.flt_no != -1)
        result << " " << setw(3) << setfill('0') << rcpt.flt_no << tag_lang.ElemIdToTagElem(etSuffix, rcpt.suffix, efmtCodeNative);
    return result.str();
}

double TBagReceipt::pay_rate()
{
    double pay_rate;
    if (pay_rate_cur != rate_cur)
        pay_rate = (rate * exch_pay_rate)/exch_rate;
    else
        pay_rate = rate;
    return pay_rate;
}

double TBagReceipt::rate_sum()
{
    double rate_sum;
    if(service_type == 1 || service_type == 2) {
        rate_sum = rate * ex_weight;
    } else {
        rate_sum = rate * value_tax/100;
    }
    return rate_sum;
}

double TBagReceipt::pay_rate_sum()
{
    double pay_rate_sum;
    if(service_type == 1 || service_type == 2) {
        pay_rate_sum = pay_rate() * ex_weight;
    } else {
        pay_rate_sum = pay_rate() * value_tax/100;
    }
    return pay_rate_sum;
}

int get_rate_precision(double rate, string rate_cur)
{
    int precision;
    if(
            rate_cur == "ЕВР" ||
            rate_cur == "ДОЛ" ||
            rate_cur == "ГРН" ||
            rate_cur == "ГБП"
      )
        precision = 2;
    else
    {
        if (separate_double(rate,2,NULL)!=0)
            precision = 2;
        else
            precision = 0;
    };
    return precision;
}

string RateToString(double rate, string rate_cur, bool pr_lat, int fmt_type)
{
    //fmt_type=1 - только rate
    //fmt_type=2 - только rate_cur
    //иначе rate+rate_cur
    ostringstream buf;
    if (fmt_type!=2 && !pr_lat)
        buf << setprecision(get_rate_precision(rate, rate_cur)) << fixed << rate;
    if (fmt_type!=1)
        buf << base_tables.get("currency").get_row("code", rate_cur).AsString("code", pr_lat?AstraLocale::LANG_EN:AstraLocale::LANG_RU);
    if (fmt_type!=2 && pr_lat)
        buf << setprecision(get_rate_precision(rate, rate_cur)) << fixed << rate;
    return buf.str();
};

string TBagReceipt::get_fmt_rate(int fmt, bool pr_inter)
{
    ostringstream result;
    if(form_type == FT_M61) {
        result << RateToString(rate_sum(), rate_cur, pr_inter, fmt);
    } else {
        if(pr_exchange())
            result
                << RateToString(pay_rate_sum(), pay_rate_cur, pr_inter, fmt)
                << "(" << RateToString(rate_sum(), rate_cur, pr_inter, fmt) << ")";
        else
            result << RateToString(rate_sum(), rate_cur, pr_inter, fmt);
    }
    return result.str();
}

string TPrnTagStore::EQUI_AMOUNT_PAID(TFieldParams fp)
{
    string result;
    if(rcpt.pay_rate_cur != rcpt.rate_cur)
        result = RateToString(rcpt.pay_rate_sum(), rcpt.pay_rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
    return result;
}

string TPrnTagStore::AMOUNT_FIGURES(TFieldParams fp)
{
    return rcpt.get_fmt_rate(1, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::AMOUNT_LETTERS(TFieldParams fp)
{
    int iptr, fract;
    fract=separate_double(rcpt.pr_exchange() and rcpt.form_type != FT_M61 ? rcpt.pay_rate_sum() : rcpt.rate_sum(), 2, &iptr);

    string result = vs_number(iptr, tag_lang.GetLang() != AstraLocale::LANG_RU);

    if(fract!=0) {
        string str_fract = IntToString(fract) + "/100 ";
        result += str_fract;
    }
    return upperc(result);
}

string TPrnTagStore::BAG_NAME(TFieldParams fp)
{
    string result;
    if(rcpt.pay_bt()) {
        if(rcpt.bag_name.empty()) {
            TQuery Qry(&OraSession);
            Qry.SQLText =
                "select "
                "  nvl(rcpt_bag_names.name, bag_types.name) name, "
                "  nvl(rcpt_bag_names.name_lat, bag_types.name_lat) name_lat "
                "from "
                "  bag_types, "
                "  rcpt_bag_names "
                "where "
                "  bag_types.code = :code and "
                "  bag_types.code = rcpt_bag_names.code(+)";
            Qry.CreateVariable("code", otInteger, rcpt.bag_type);
            Qry.Execute();
            if(Qry.Eof) throw Exception("TPrnTagStore::BAG_NAME: bag_type not found (code = %d)", rcpt.bag_type);
            if(tag_lang.GetLang() != AstraLocale::LANG_RU)
                result = Qry.FieldAsString("name_lat");
            else
                result = Qry.FieldAsString("name");
            if(rcpt.bag_type == 1 || rcpt.bag_type == 2)
                //негабарит
                result += " " + IntToString(rcpt.ex_amount) + " " + getLocaleText("MSG.BR.SEATS", tag_lang.GetLang());
            result = upperc(result);
        } else
            result = rcpt.bag_name;
    }
    return result;
}

string TPrnTagStore::CHARGE(TFieldParams fp)
{
    return rcpt.get_fmt_rate(0, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::CURRENCY(TFieldParams fp)
{
    return rcpt.get_fmt_rate(2, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::EX_WEIGHT(TFieldParams fp)
{
    ostringstream result;
    if(rcpt.service_type == 1 || rcpt.service_type == 2) {
        result << rcpt.ex_weight;
        if (rcpt.form_type == FT_M61)
            result << getLocaleText("MSG.BR.KG", tag_lang.GetLang());
    }
    return result.str();
}

int get_exch_precision(double rate)
{
    int i;
    i=separate_double(rate,4,NULL);

    if (i==0) return 0;
    if (i%100==0) return 2;
    return 4;

    /*double iptr;
      ostringstream ssbuf;
      ssbuf << noshowpoint << modf(rate, &iptr);
      int precision = ssbuf.str().size();
      if(precision == 1)
      precision = 0;
      else
      precision -= 2;

      if(precision >= 3)
      precision = 4;
      else if(precision >= 1)
      precision = 2;
      return precision; */
}

string ExchToString(int rate1, string rate_cur1, double rate2, string rate_cur2, TTagLang &tag_lang)
{
    ostringstream buf;
    buf
        << rate1
        << tag_lang.ElemIdToTagElem(etCurrency, rate_cur1, efmtCodeNative)
        << "="
        << fixed
        << setprecision(get_exch_precision(rate2))
        << rate2
        << tag_lang.ElemIdToTagElem(etCurrency, rate_cur2, efmtCodeNative);
    return buf.str();
};

string TPrnTagStore::EXCHANGE_RATE(TFieldParams fp)
{
    ostringstream result;
    if (
            rcpt.pay_rate_cur != rcpt.rate_cur &&
            (not (rcpt.service_type != 3 and rcpt.pr_exchange()) or rcpt.form_type != FT_M61)
       )
    {
        if (rcpt.form_type != FT_M61 and tag_lang.GetLang() != AstraLocale::LANG_RU) result << "RATE ";
        result << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, tag_lang);
    };
    return result.str();
}

string TPrnTagStore::ISSUE_DATE(TFieldParams fp)
{
    return DateTimeToStr(
            UTCToLocal(rcpt.issue_date, CityTZRegion(DeskCity(rcpt.issue_desk, false))),
            fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TBagReceipt::issue_place_idx(int idx)
{
    if(idx < 0 or idx > 4)
        throw Exception("TBagReceipt::issue_place_idx: idx out of range: %d", idx);
    if(f_issue_place_idx.empty()) {
        string buf = issue_place;
        int line_num = 0;
        while(line_num < 5) {
            string::size_type i = buf.find('\n');
            f_issue_place_idx.push_back(buf.substr(0, i));
            buf.erase(0, i + 1);
            line_num++;
        }
        if(f_issue_place_idx.size() != 5)
            throw Exception("TBagReceipt::issue_place_idx: wrong issue_place format: lines = %zu", f_issue_place_idx.size());
    }
    return f_issue_place_idx[idx];
}

string TPrnTagStore::ISSUE_PLACE1(TFieldParams fp)
{
    return rcpt.issue_place_idx(0);
}

string TPrnTagStore::ISSUE_PLACE2(TFieldParams fp)
{
    return rcpt.issue_place_idx(1);
}

string TPrnTagStore::ISSUE_PLACE3(TFieldParams fp)
{
    return rcpt.issue_place_idx(2);
}

string TPrnTagStore::ISSUE_PLACE4(TFieldParams fp)
{
    return rcpt.issue_place_idx(3);
}

string TPrnTagStore::ISSUE_PLACE5(TFieldParams fp)
{
    return rcpt.issue_place_idx(4);
}
string TPrnTagStore::PAX_DOC(TFieldParams fp)
{
    return rcpt.pax_doc;
}

string TPrnTagStore::PAX_NAME(TFieldParams fp)
{
    return rcpt.pax_name;
}

string TPrnTagStore::PAY_FORM(TFieldParams fp)
{
    vector<TBagPayType>::iterator i;
    ostringstream result;
    if (rcpt.pay_types.size()==1)
    {
        //одна форма оплаты
        i=rcpt.pay_types.begin();
        result << tag_lang.ElemIdToTagElem(etPayType, i->pay_type, efmtCodeNative);
        if (i->pay_type!=CASH_PAY_TYPE_ID && !i->extra.empty())
            result     << ' ' << i->extra;
    }
    else
    {
        //несколько форм оплаты
        //первой всегда идет НАЛ
        for(int k=0;k<=1;k++)
        {
            for(i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();i++)
            {
                if ((k==0 and i->pay_type!=CASH_PAY_TYPE_ID) or
                        (k!=0 and i->pay_type==CASH_PAY_TYPE_ID)) continue;

                if (!result.str().empty())
                    result     << '+';
                result
                    << tag_lang.ElemIdToTagElem(etPayType, i->pay_type, efmtCodeNative)
                    << RateToString(i->pay_rate_sum, rcpt.pay_rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
                if (i->pay_type!=CASH_PAY_TYPE_ID && !i->extra.empty())
                    result << ' ' << i->extra;
            }
        }
    }
    return result.str();
}

string get_mso_point(const string &aairp, TTagLang &tag_lang)
{
    TBaseTable &airps = base_tables.get("airps");
    string city = airps.get_row("code", aairp).AsString("city");
    string point = tag_lang.ElemIdToTagElem(etCity, city, efmtNameLong);
    TQuery airpsQry(&OraSession);
    airpsQry.SQLText =  "select count(*) from airps where city = :city and pr_del = 0";
    airpsQry.CreateVariable("city", otString, city);
    airpsQry.Execute();
    if(!airpsQry.Eof && airpsQry.FieldAsInteger(0) != 1) {
        string airp = tag_lang.ElemIdToTagElem(etAirp, aairp, efmtCodeNative);
        point += "(" + airp + ")";
    }
    return point;
}

string TPrnTagStore::POINT_ARV(TFieldParams fp)
{
    return get_mso_point(rcpt.airp_arv, tag_lang);
}

string TPrnTagStore::POINT_DEP(TFieldParams fp)
{
    return get_mso_point(rcpt.airp_dep, tag_lang);
}

string TPrnTagStore::PREV_NO(TFieldParams fp)
{
    return rcpt.prev_no;
}

int get_value_tax_precision(double tax)
{
    return 1;
};

string TPrnTagStore::RATE(TFieldParams fp)
{
    ostringstream result;
    if(rcpt.service_type == 1 || rcpt.service_type == 2) {
        if(rcpt.pr_exchange())
            result
                << RateToString(rcpt.pay_rate(), rcpt.pay_rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0)
                << "(" << RateToString(rcpt.rate, rcpt.rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0) << ")";
        else
            result << RateToString(rcpt.rate, rcpt.rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
    } else {
        //багаж с объявленной ценностью
        result
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax <<"%";
    }
    return result.str();
}

string TPrnTagStore::REMARKS1(TFieldParams fp)
{
    ostringstream result;
    if(remarksInfo.exists()) {
        result << remarksInfo.rem_at(1);
    } else {
        if(
                rcpt.form_type == FT_M61 or
                rcpt.form_type == FT_298_401
          ) {
            if(rcpt.service_type == 1 || rcpt.service_type == 2) {
                result << getLocaleText("MSG.BR.RATE_PER_KG", tag_lang.GetLang()) << RATE(fp);
                if(rcpt.pr_exchange())
                    result
                        << "("
                        << (tag_lang.GetLang() != AstraLocale::LANG_RU ? "RATE " : "")
                        << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, tag_lang)
                        << ")";
            } else {
                result
                    << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
                    << rcpt.value_tax
                    << getLocaleText("MSG.BR.RATE_OF", tag_lang.GetLang())
                    << RateToString(rcpt.rate, rcpt.rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
            }
        } else if(
                rcpt.form_type == FT_298_451 or
                rcpt.form_type == FT_823_451 or
                rcpt.form_type == FT_FV_451
                )
            result << get_tag_no_err(TAG::EXCHANGE_RATE);
    }
    return result.str();
}

string TPrnTagStore::REMARKS2(TFieldParams fp)
{
    ostringstream result;
    if(remarksInfo.exists()) {
        result << remarksInfo.rem_at(2);
    } else {
        if(
                rcpt.form_type == FT_M61 or
                rcpt.form_type == FT_298_401
          ) {
            if(rcpt.service_type == 1 || rcpt.service_type == 2)
                result
                    << rcpt.ex_weight
                    << getLocaleText("MSG.BR.KG", tag_lang.GetLang());
        } else if(
                rcpt.form_type == FT_298_451 or
                rcpt.form_type == FT_823_451
                )
            result << get_tag_no_err(TAG::NDS);
        else if(rcpt.form_type == FT_FV_451) {
            result << tag_lang.ElemIdToTagElem(etAirline, rcpt.airline, efmtCodeNative);
            if(rcpt.flt_no != -1)
                result << setw(3) << setfill('0') << rcpt.flt_no << tag_lang.ElemIdToTagElem(etSuffix, rcpt.suffix, efmtCodeNative);
            if (rcpt.scd_local_date!=ASTRA::NoExists)
                result << "/" << DateTimeToStr(rcpt.scd_local_date, "ddmmmyy", tag_lang.GetLang() != AstraLocale::LANG_RU);
        }
    }
    return result.str();
}

string TPrnTagStore::REMARKS3(TFieldParams fp)
{
    ostringstream result;
    if(remarksInfo.exists()) {
        result << remarksInfo.rem_at(3);
    } else {
        if(rcpt.form_type == FT_M61)
            result << get_tag_no_err(TAG::NDS);
    }
    return result.str();
}

string TPrnTagStore::REMARKS4(TFieldParams fp)
{
    ostringstream result;
    if(remarksInfo.exists())
        result << remarksInfo.rem_at(4);
    else {}
    return result.str();
}

string TPrnTagStore::REMARKS5(TFieldParams fp)
{
    ostringstream result;
    if(remarksInfo.exists())
        result << remarksInfo.rem_at(5);
    else {}
    return result.str();
}

string TBagReceipt::get_service_name(bool is_inter)
{
    if(service_name.empty()) {
        TQuery Qry(&OraSession);
        Qry.SQLText =  "select name, name_lat from rcpt_service_types where code = :code";
        Qry.CreateVariable("code", otInteger, service_type);
        Qry.Execute();
        if(Qry.Eof) throw Exception("TBagReceipt::get_service_name: service_type not found (code = %d)", service_type);
        service_name = Qry.FieldAsString("name");
        service_name_lat = Qry.FieldAsString("name_lat");
    }
    return is_inter ? service_name_lat : service_name;
}

string TPrnTagStore::SERVICE_TYPE(TFieldParams fp)
{
    return "10 " + rcpt.get_service_name(tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::TICKETS(TFieldParams fp)
{
    return rcpt.tickets;
}

string TPrnTagStore::TO(TFieldParams fp)
{
    ostringstream s;
    s << POINT_DEP(fp) << "-" << POINT_ARV(fp) << " " << AIRLINE_CODE(fp);
    if (rcpt.scd_local_date!=ASTRA::NoExists)
        s << " "
            << DateTimeToStr(rcpt.scd_local_date, "ddmmm", tag_lang.GetLang() != AstraLocale::LANG_RU);
    return  s.str();
}

string TPrnTagStore::NDS(TFieldParams fp)
{
    ostringstream result;
    if(rcpt.nds != NoExists)
        result << getLocaleText("НДС", tag_lang.GetLang()) << fixed << setprecision(2) << rcpt.nds;
    return result.str();
}

string TPrnTagStore::TOTAL(TFieldParams fp)
{
    return RateToString(rcpt.pay_rate_sum(), rcpt.pay_rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
}

std::ostream & operator <<(std::ostream &os, BIPrintRules::TPrintType::Enum const &value)
{
    os << BIPrintRules::PrintTypesView().encode(value);
    return os;
}

