#include "prn_tag_store.h"
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "xml_unit.h"
#include "misc.h"
#include "docs.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace BASIC;
using namespace AstraLocale;

const int POINT_INFO = 1;
const int PAX_INFO = 2;
const int BAG_INFO = 4;
const int BRD_INFO = 8;
const int FQT_INFO = 16;
const int PNR_INFO = 32;

bool TTagLang::IsInter(TBTRoute *aroute, string &country)
{
    country.clear();
    if(aroute == NULL) return false;
    string tmp_country;
    // 1-й эл-т вектора содержит данные из pax_grp, а нас интересуют только данные transfer
    // поэтому его не анализируем.
    for(TBTRoute::iterator ir = aroute->begin(); ir != aroute->end(); ir++) {
        if(ir == aroute->begin()) continue;
        if(IsTrferInter(ir->airp_dep, ir->airp_arv, country))
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

void TPrnTagStore::set_pr_lat(bool vpr_lat)
{
    tag_lang.Init(vpr_lat);
}

void TTagLang::RcptInit(bool apr_lat)
{
    rcpt_is_inter = apr_lat;
}

void TTagLang::Init(bool apr_lat)
{
    pr_lat = apr_lat;
}

void TTagLang::Init(int point_dep, int point_arv, TBTRoute *aroute, bool apr_lat)
{
    string route_country;
    route_inter = IsRouteInter(point_dep, point_arv, route_country);
    if(aroute != NULL and aroute->size() > 1) { // список стыковочных сегментов для бирки
        string trfer_route_country;
        IsInter(aroute, trfer_route_country);
        if(not route_inter and route_country != trfer_route_country) {
            route_inter = true;
            route_country.clear();
        }
    }
    if(route_country == "РФ")
        route_country_lang = AstraLocale::LANG_RU;
    else
        route_country_lang = "";
    pr_lat = apr_lat;
}

bool TTagLang::IsTstInter() const
{
    return tag_lang == "E" or pr_lat;
}

bool TTagLang::IsInter() const
{
    if(rcpt_is_inter != NoExists)
        return rcpt_is_inter or tag_lang == "E";
    else
        return
            (route_inter || route_country_lang.empty() || route_country_lang!=TReqInfo::Instance()->desk.lang) or
            tag_lang == "E" or
            pr_lat;
}

string TTagLang::GetLang(TElemFmt &fmt, string firm_lang) const
{
  string lang = firm_lang;
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

string TTagLang::ElemIdToTagElem(TElemType type, const string &id, TElemFmt fmt, string firm_lang) const
{
  if (id.empty()) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, lang, fmts);
  return ElemIdToElem(type, id, fmts, true);
};

string TTagLang::GetLang()
{
  string lang = TReqInfo::Instance()->desk.lang;
  if (IsInter()) lang=AstraLocale::LANG_EN;
  return lang;
}

string TTagLang::ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
{
  if (id==ASTRA::NoExists) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  vector< pair<TElemFmt,string> > fmts;

  getElemFmts(fmt, lang, fmts);
  return ElemIdToElem(type, id, fmts, true);
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

// Bag receipts
TPrnTagStore::TPrnTagStore(TBagReceipt &arcpt)
{
    rcpt = arcpt;
    tag_lang.RcptInit(rcpt.pr_lat);

    tag_list.insert(make_pair(TAG::BULKY_BT,        TTagListItem(1,  &TPrnTagStore::BULKY_BT, 0)));
    tag_list.insert(make_pair(TAG::BULKY_BT_LETTER, TTagListItem(1,  &TPrnTagStore::BULKY_BT_LETTER, 0)));
    tag_list.insert(make_pair(TAG::GOLF_BT,         TTagListItem(1,  &TPrnTagStore::GOLF_BT, 0)));
    tag_list.insert(make_pair(TAG::OTHER_BT,        TTagListItem(1,  &TPrnTagStore::OTHER_BT, 0)));
    tag_list.insert(make_pair(TAG::OTHER_BT_LETTER, TTagListItem(1,  &TPrnTagStore::OTHER_BT_LETTER, 0)));
    tag_list.insert(make_pair(TAG::PET_BT,          TTagListItem(1,  &TPrnTagStore::PET_BT, 0)));
    tag_list.insert(make_pair(TAG::SKI_BT,          TTagListItem(1,  &TPrnTagStore::SKI_BT, 0)));
    tag_list.insert(make_pair(TAG::VALUE_BT,        TTagListItem(1,  &TPrnTagStore::VALUE_BT, 0)));
    tag_list.insert(make_pair(TAG::VALUE_BT_LETTER, TTagListItem(1,  &TPrnTagStore::VALUE_BT_LETTER, 0)));
    tag_list.insert(make_pair(TAG::AIRCODE,         TTagListItem(1,  &TPrnTagStore::BR_AIRCODE, 0)));
    tag_list.insert(make_pair(TAG::AIRLINE,         TTagListItem(1,  &TPrnTagStore::BR_AIRLINE, 0)));
    tag_list.insert(make_pair(TAG::AIRLINE_CODE,    TTagListItem(1,  &TPrnTagStore::AIRLINE_CODE, 0)));
    tag_list.insert(make_pair(TAG::AMOUNT_FIGURES,  TTagListItem(1,  &TPrnTagStore::AMOUNT_FIGURES, 0)));
    tag_list.insert(make_pair(TAG::AMOUNT_LETTERS,  TTagListItem(1,  &TPrnTagStore::AMOUNT_LETTERS, 0)));
    tag_list.insert(make_pair(TAG::BAG_NAME,        TTagListItem(1,  &TPrnTagStore::BAG_NAME, 0)));
    tag_list.insert(make_pair(TAG::CURRENCY,        TTagListItem(1,  &TPrnTagStore::CURRENCY, 0)));
    tag_list.insert(make_pair(TAG::EX_WEIGHT,       TTagListItem(1,  &TPrnTagStore::EX_WEIGHT, 0)));
    tag_list.insert(make_pair(TAG::EXCHANGE_RATE,   TTagListItem(1,  &TPrnTagStore::EXCHANGE_RATE, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_DATE,      TTagListItem(1,  &TPrnTagStore::ISSUE_DATE, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE1,    TTagListItem(1,  &TPrnTagStore::ISSUE_PLACE1, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE2,    TTagListItem(1,  &TPrnTagStore::ISSUE_PLACE2, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE3,    TTagListItem(1,  &TPrnTagStore::ISSUE_PLACE3, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE4,    TTagListItem(1,  &TPrnTagStore::ISSUE_PLACE4, 0)));
    tag_list.insert(make_pair(TAG::ISSUE_PLACE5,    TTagListItem(1,  &TPrnTagStore::ISSUE_PLACE5, 0)));
    tag_list.insert(make_pair(TAG::PAX_DOC,         TTagListItem(1,  &TPrnTagStore::PAX_DOC, 0)));
    tag_list.insert(make_pair(TAG::PAX_NAME,        TTagListItem(1,  &TPrnTagStore::PAX_NAME, 0)));
    tag_list.insert(make_pair(TAG::PAY_FORM,        TTagListItem(1,  &TPrnTagStore::PAY_FORM, 0)));
    tag_list.insert(make_pair(TAG::POINT_ARV,       TTagListItem(1,  &TPrnTagStore::POINT_ARV, 0)));
    tag_list.insert(make_pair(TAG::POINT_DEP,       TTagListItem(1,  &TPrnTagStore::POINT_DEP, 0)));
    tag_list.insert(make_pair(TAG::PREV_NO,         TTagListItem(1,  &TPrnTagStore::PREV_NO, 0)));
    tag_list.insert(make_pair(TAG::RATE,            TTagListItem(1,  &TPrnTagStore::RATE, 0)));
    tag_list.insert(make_pair(TAG::REMARKS1,        TTagListItem(1,  &TPrnTagStore::REMARKS1, 0)));
    tag_list.insert(make_pair(TAG::REMARKS2,        TTagListItem(1,  &TPrnTagStore::REMARKS2, 0)));
    tag_list.insert(make_pair(TAG::SERVICE_TYPE,    TTagListItem(1,  &TPrnTagStore::SERVICE_TYPE, 0)));
    tag_list.insert(make_pair(TAG::TICKETS,         TTagListItem(1,  &TPrnTagStore::TICKETS, 0)));
    tag_list.insert(make_pair(TAG::TO,              TTagListItem(1,  &TPrnTagStore::TO, 0)));
    tag_list.insert(make_pair(TAG::TOTAL,           TTagListItem(1,  &TPrnTagStore::TOTAL, 0)));
}

// Test tags
TPrnTagStore::TPrnTagStore(bool apr_lat)
{
    tag_lang.Init(apr_lat);
    prn_test_tags.Init();
}

// BP && BT
TPrnTagStore::TPrnTagStore(int agrp_id, int apax_id, int apr_lat, xmlNodePtr tagsNode, TBTRoute *aroute)
{
    grpInfo.Init(agrp_id);
    tag_lang.Init(grpInfo.point_dep, grpInfo.point_arv, aroute, apr_lat != 0);
    pax_id = apax_id;
    if(aroute == NULL and pax_id == NoExists)
        throw Exception("TPrnTagStore::TPrnTagStore: pax_id not defined for bp mode");

    tag_list.insert(make_pair(TAG::BCBP_M_2,        TTagListItem(0,  &TPrnTagStore::BCBP_M_2, POINT_INFO | PAX_INFO | PNR_INFO)));
    tag_list.insert(make_pair(TAG::ACT,             TTagListItem(0,  &TPrnTagStore::ACT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE,         TTagListItem(0,  &TPrnTagStore::AIRLINE, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_SHORT,   TTagListItem(10, &TPrnTagStore::AIRLINE_SHORT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_NAME,    TTagListItem(15, &TPrnTagStore::AIRLINE_NAME, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRP_ARV,        TTagListItem(0,  &TPrnTagStore::AIRP_ARV)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME,   TTagListItem(11, &TPrnTagStore::AIRP_ARV_NAME)));
    tag_list.insert(make_pair(TAG::AIRP_DEP,        TTagListItem(0,  &TPrnTagStore::AIRP_DEP)));
    tag_list.insert(make_pair(TAG::AIRP_DEP_NAME,   TTagListItem(11, &TPrnTagStore::AIRP_DEP_NAME)));
    tag_list.insert(make_pair(TAG::BAG_AMOUNT,      TTagListItem(0,  &TPrnTagStore::BAG_AMOUNT, BAG_INFO)));
    tag_list.insert(make_pair(TAG::BAG_WEIGHT,      TTagListItem(0,  &TPrnTagStore::BAG_WEIGHT, BAG_INFO)));
    tag_list.insert(make_pair(TAG::BRD_FROM,        TTagListItem(0,  &TPrnTagStore::BRD_FROM, BRD_INFO)));
    tag_list.insert(make_pair(TAG::BRD_TO,          TTagListItem(0,  &TPrnTagStore::BRD_TO, BRD_INFO)));
    tag_list.insert(make_pair(TAG::CITY_ARV_NAME,   TTagListItem(8,  &TPrnTagStore::CITY_ARV_NAME)));
    tag_list.insert(make_pair(TAG::CITY_DEP_NAME,   TTagListItem(8,  &TPrnTagStore::CITY_DEP_NAME)));
    tag_list.insert(make_pair(TAG::CLASS,           TTagListItem(0,  &TPrnTagStore::CLASS)));
    tag_list.insert(make_pair(TAG::CLASS_NAME,      TTagListItem(0,  &TPrnTagStore::CLASS_NAME)));
    tag_list.insert(make_pair(TAG::DOCUMENT,        TTagListItem(10, &TPrnTagStore::DOCUMENT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::EST,             TTagListItem(0,  &TPrnTagStore::EST, POINT_INFO)));
    tag_list.insert(make_pair(TAG::ETICKET_NO,      TTagListItem(16, &TPrnTagStore::ETICKET_NO, PAX_INFO))); // !!!
    tag_list.insert(make_pair(TAG::ETKT,            TTagListItem(21, &TPrnTagStore::ETKT, PAX_INFO))); // !!!
    tag_list.insert(make_pair(TAG::EXCESS,          TTagListItem(0,  &TPrnTagStore::EXCESS)));
    tag_list.insert(make_pair(TAG::FLT_NO,          TTagListItem(0,  &TPrnTagStore::FLT_NO, POINT_INFO))); // !!!
    tag_list.insert(make_pair(TAG::FQT,             TTagListItem(0,  &TPrnTagStore::FQT, FQT_INFO)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_ARV,  TTagListItem(0,  &TPrnTagStore::FULL_PLACE_ARV)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_DEP,  TTagListItem(0,  &TPrnTagStore::FULL_PLACE_DEP)));
    tag_list.insert(make_pair(TAG::FULLNAME,        TTagListItem(0,  &TPrnTagStore::FULLNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::GATE,            TTagListItem(3,  &TPrnTagStore::GATE)));
    tag_list.insert(make_pair(TAG::LONG_ARV,        TTagListItem(0,  &TPrnTagStore::LONG_ARV)));
    tag_list.insert(make_pair(TAG::LONG_DEP,        TTagListItem(0,  &TPrnTagStore::LONG_DEP)));
    tag_list.insert(make_pair(TAG::NAME,            TTagListItem(1,  &TPrnTagStore::NAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::NO_SMOKE,        TTagListItem(0,  &TPrnTagStore::NO_SMOKE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::ONE_SEAT_NO,     TTagListItem(0,  &TPrnTagStore::ONE_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PAX_ID,          TTagListItem(10, &TPrnTagStore::PAX_ID, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PLACE_ARV,       TTagListItem(0,  &TPrnTagStore::PLACE_ARV)));
    tag_list.insert(make_pair(TAG::PLACE_DEP,       TTagListItem(0,  &TPrnTagStore::PLACE_DEP)));
    tag_list.insert(make_pair(TAG::REG_NO,          TTagListItem(0,  &TPrnTagStore::REG_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SCD,             TTagListItem(0,  &TPrnTagStore::SCD, POINT_INFO)));
    tag_list.insert(make_pair(TAG::SEAT_NO,         TTagListItem(3,  &TPrnTagStore::SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::STR_SEAT_NO,     TTagListItem(6,  &TPrnTagStore::STR_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::LIST_SEAT_NO,    TTagListItem(0,  &TPrnTagStore::LIST_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SURNAME,         TTagListItem(8,  &TPrnTagStore::SURNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::TEST_SERVER,     TTagListItem(0,  &TPrnTagStore::TEST_SERVER)));

    // specific for bag tags
    tag_list.insert(make_pair(TAG::AIRCODE,         TTagListItem(0,  &TPrnTagStore::AIRCODE)));
    tag_list.insert(make_pair(TAG::NO,              TTagListItem(0,  &TPrnTagStore::NO)));
    tag_list.insert(make_pair(TAG::ISSUED,          TTagListItem(0,  &TPrnTagStore::ISSUED)));
    tag_list.insert(make_pair(TAG::BT_AMOUNT,       TTagListItem(0,  &TPrnTagStore::BT_AMOUNT)));
    tag_list.insert(make_pair(TAG::BT_WEIGHT,       TTagListItem(0,  &TPrnTagStore::BT_WEIGHT)));
    tag_list.insert(make_pair(TAG::LIAB_LIMIT,      TTagListItem(10, &TPrnTagStore::LIAB_LIMIT)));
    tag_list.insert(make_pair(TAG::FLT_NO1,         TTagListItem(3,  &TPrnTagStore::FLT_NO1)));
    tag_list.insert(make_pair(TAG::FLT_NO2,         TTagListItem(3,  &TPrnTagStore::FLT_NO2)));
    tag_list.insert(make_pair(TAG::FLT_NO3,         TTagListItem(3,  &TPrnTagStore::FLT_NO3)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE1,     TTagListItem(0,  &TPrnTagStore::LOCAL_DATE1)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE2,     TTagListItem(0,  &TPrnTagStore::LOCAL_DATE2)));
    tag_list.insert(make_pair(TAG::LOCAL_DATE3,     TTagListItem(0,  &TPrnTagStore::LOCAL_DATE3)));
    tag_list.insert(make_pair(TAG::AIRLINE1,        TTagListItem(0,  &TPrnTagStore::AIRLINE1)));
    tag_list.insert(make_pair(TAG::AIRLINE2,        TTagListItem(0,  &TPrnTagStore::AIRLINE2)));
    tag_list.insert(make_pair(TAG::AIRLINE3,        TTagListItem(0,  &TPrnTagStore::AIRLINE3)));
    tag_list.insert(make_pair(TAG::AIRP_ARV1,       TTagListItem(0,  &TPrnTagStore::AIRP_ARV1)));
    tag_list.insert(make_pair(TAG::AIRP_ARV2,       TTagListItem(0,  &TPrnTagStore::AIRP_ARV2)));
    tag_list.insert(make_pair(TAG::AIRP_ARV3,       TTagListItem(0,  &TPrnTagStore::AIRP_ARV3)));
    tag_list.insert(make_pair(TAG::FLTDATE1,        TTagListItem(0,  &TPrnTagStore::FLTDATE1)));
    tag_list.insert(make_pair(TAG::FLTDATE2,        TTagListItem(0,  &TPrnTagStore::FLTDATE2)));
    tag_list.insert(make_pair(TAG::FLTDATE3,        TTagListItem(0,  &TPrnTagStore::FLTDATE3)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME1,  TTagListItem(22, &TPrnTagStore::AIRP_ARV_NAME1)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME2,  TTagListItem(22, &TPrnTagStore::AIRP_ARV_NAME2)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME3,  TTagListItem(22, &TPrnTagStore::AIRP_ARV_NAME3)));
    tag_list.insert(make_pair(TAG::PNR,             TTagListItem(0,  &TPrnTagStore::PNR, PNR_INFO)));

    if(tagsNode) {
        // Положим теги из клиентского запроса
        for(xmlNodePtr curNode = tagsNode->children; curNode; curNode = curNode->next)
            set_tag(upperc((char *)curNode->name), NodeAsString(curNode));
    }
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

string TPrnTagStore::get_test_field(std::string name, size_t len, std::string date_format)
{
    name = upperc(name);
    map<string, TPrnTestTagsItem>::iterator im = prn_test_tags.items.find(name);
    if(im == prn_test_tags.items.end())
        throw Exception("TPrnTagStore::get_test_field: %s tag not found", name.c_str());
    string value = tag_lang.IsTstInter() ? im->second.value_lat : im->second.value;
    if(value.empty())
        value = im->second.value;
    TDateTime date = 0;
    ostringstream result;
    switch(im->second.type) {
        case 'D':
            StrToDateTime(value.c_str(), ServerFormatDateTimeAsString, date);
            result << DateTimeToStr(date, value, tag_lang.IsTstInter());
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

string TPrnTagStore::get_real_field(std::string name, size_t len, std::string date_format)
{
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::get_field: tag '%s' not implemented", name.c_str());
    if((im->second.info_type & POINT_INFO) == POINT_INFO)
        pointInfo.Init(grpInfo.point_dep, grpInfo.grp_id);
    if((im->second.info_type & PAX_INFO) == PAX_INFO)
        paxInfo.Init(pax_id);
    if((im->second.info_type & BAG_INFO) == BAG_INFO)
        bagInfo.Init(grpInfo.grp_id);
    if((im->second.info_type & BRD_INFO) == BRD_INFO)
        brdInfo.Init(grpInfo.point_dep);
    if((im->second.info_type & FQT_INFO) == FQT_INFO)
        fqtInfo.Init(pax_id);
    if((im->second.info_type & PNR_INFO) == PNR_INFO)
        pnrInfo.Init(pax_id);
    string result;
    try {
        result = (this->*im->second.tag_funct)(TFieldParams(date_format, im->second.TagInfo, len));
        im->second.processed = true;
        if(len != 0 and len < result.length())
            if(im->second.length == 0 or len < im->second.length)
                result = string(len, '?');
            else
                result = result.substr(0, len);

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

string TPrnTagStore::get_tag(string name, string tag_lang)
{
    this->tag_lang.set_tag_lang(tag_lang);
    if(prn_test_tags.items.empty())
        return get_real_field(name, 0, ServerFormatDateTimeAsString);
    else
        return get_test_field(name, 0, ServerFormatDateTimeAsString);
}

string TPrnTagStore::get_field(std::string name, size_t len, std::string align, std::string date_format, string tag_lang)
{
    this->tag_lang.set_tag_lang(tag_lang);
    string result;
    if(prn_test_tags.items.empty())
        result = get_real_field(name, len, date_format);
    else
        result = get_test_field(name, len, date_format);
    if(!len) len = result.size();

    if(print_mode == 1 or print_mode == 2 and (name == TAG::PAX_ID or name == TAG::TEST_SERVER))
        return string(len, '8');
    if(print_mode == 2)
        return AlignString("8", len, align);
    if(print_mode == 3)
        return string(len, ' ');

    if(result.size() > len)
        result = string(len, '?');
    else
        result = AlignString(result, len, align);
    if(this->tag_lang.get_pr_lat() and not is_lat(result)) {
        ProgError(STDLOG, "Данные печати не латинские: %s = \"%s\"", name.c_str(), result.c_str());
        if(result.size() > 20)
            result = result.substr(0, 20) + "...";
        throw UserException("MSG.NO_LAT_PRN_DATA", LParams() << LParam("tag", name) << LParam("value", result));
    }
    return result;
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
        "   delete from bp_print where pax_id = :pax_id and pr_print = 0 and desk=:desk; "
        "   insert into bp_print( "
        "       pax_id, "
        "       time_print, "
        "       pr_print, "
        "       desk ";
    part2 =
        "   ) values( "
        "       :pax_id, "
        "       :now_utc, "
        "       0, "
        "       :desk ";
};

void TPrnTagStore::get_prn_qry(TQuery &Qry)
{
    if(not prn_test_tags.items.empty())
        throw Exception("get_prn_qry can't be called in test mode");
    TPrnQryBuilder prnQry(Qry);
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.CreateVariable("DESK", otString, TReqInfo::Instance()->desk.code);

    if(tag_list[TAG::AIRLINE].processed or tag_list[TAG::AIRLINE_NAME].processed)
        prnQry.add_part(TAG::AIRLINE, pointInfo.airline);
    if(tag_list[TAG::SCD].processed)
        prnQry.add_part(TAG::SCD, pointInfo.scd);
    if(tag_list[TAG::BRD_FROM].processed)
        prnQry.add_part(TAG::BRD_FROM, brdInfo.brd_from);
    if(tag_list[TAG::BRD_TO].processed)
        prnQry.add_part(TAG::BRD_TO, brdInfo.brd_to);
    if(tag_list[TAG::AIRP_ARV].processed or tag_list[TAG::AIRP_ARV_NAME].processed)
        prnQry.add_part(TAG::AIRP_ARV, grpInfo.airp_arv);
    if(tag_list[TAG::AIRP_DEP].processed or tag_list[TAG::AIRP_DEP_NAME].processed)
        prnQry.add_part(TAG::AIRP_DEP, grpInfo.airp_dep);
    if(tag_list[TAG::CLASS].processed)
        prnQry.add_part("class_grp", grpInfo.class_grp);
    if(tag_list[TAG::GATE].processed)
        prnQry.add_part(TAG::GATE, boost::any_cast<string>(tag_list[TAG::GATE].TagInfo));
    if(tag_list[TAG::REG_NO].processed)
        prnQry.add_part(TAG::REG_NO, paxInfo.reg_no);
    if(
            tag_list[TAG::SEAT_NO].processed or
            tag_list[TAG::ONE_SEAT_NO].processed or
            tag_list[TAG::STR_SEAT_NO].processed or
            tag_list[TAG::LIST_SEAT_NO].processed
      )
        prnQry.add_part("seat_no", get_tag(TAG::LIST_SEAT_NO));
    if(tag_list[TAG::NAME].processed)
        prnQry.add_part(TAG::NAME, paxInfo.name);
    if(tag_list[TAG::NO_SMOKE].processed)
        prnQry.add_part("pr_smoke", paxInfo.pr_smoke);
    if(tag_list[TAG::BAG_AMOUNT].processed)
        prnQry.add_part(TAG::BAG_AMOUNT, bagInfo.bag_amount);
    if(tag_list[TAG::BAG_WEIGHT].processed)
        prnQry.add_part(TAG::BAG_WEIGHT, bagInfo.bag_weight);
    if(tag_list[TAG::EXCESS].processed)
        prnQry.add_part(TAG::EXCESS, grpInfo.excess);
    if(tag_list[TAG::FLT_NO].processed) {
        prnQry.add_part(TAG::FLT_NO, pointInfo.flt_no);
        prnQry.add_part("suffix", pointInfo.suffix);
    }
    if(tag_list[TAG::SURNAME].processed)
        prnQry.add_part(TAG::SURNAME, paxInfo.surname);
    string SQLText = prnQry.text();
    Qry.SQLText = SQLText;
}

void TPrnTagStore::TPnrInfo::Init(int apax_id)
{
    if(not pr_init) {
        pr_init = true;
        GetPaxPnrAddr(apax_id, pnrs, airline);
    }
}

void TPrnTagStore::TFqtInfo::Init(int apax_id)
{
    if(not pr_init) {
        pr_init = true;
        TQuery Qry(&OraSession);
        Qry.SQLText = "select airline, no, extra from pax_fqt where pax_id = :pax_id and rownum < 2";
        Qry.CreateVariable("pax_id", otInteger, apax_id);
        Qry.Execute();
        if(!Qry.Eof) {
            airline = Qry.FieldAsString("airline");
            no = Qry.FieldAsString("no");
            extra = Qry.FieldAsString("extra");
        }
    }
}

void TPrnTagStore::TBrdInfo::Init(int point_id)
{
    if(brd_from == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   gtimer.get_stage_time(:point_id,:brd_open_stage_id) brd_from, "
            "   gtimer.get_stage_time(:point_id,:brd_close_stage_id) brd_to "
            "from "
            "   dual ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.CreateVariable("brd_open_stage_id", otInteger, sOpenBoarding);
        Qry.CreateVariable("brd_close_stage_id", otInteger, sCloseBoarding);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TPrnTagStore::TBrdInfo::Init no data found for point_id = %d", point_id);
        brd_from = Qry.FieldAsDateTime("brd_from");
        brd_to = Qry.FieldAsDateTime("brd_to");
    }
}

void TPrnTagStore::TBagInfo::Init(int grp_id)
{
    if(bag_amount == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   ckin.get_bagAmount2(:grp_id, null, null) bag_amount, "
            "   ckin.get_bagWeight2(:grp_id, null, null) bag_weight "
            "from "
            "   dual ";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TPrnTagStore::TBagInfo::Init no data found for grp_id = %d", grp_id);
        bag_amount = Qry.FieldAsInteger("bag_amount");
        bag_weight = Qry.FieldAsInteger("bag_weight");
    }
}

void TPrnTagStore::TPaxInfo::Init(int apax_id)
{
    if(apax_id == NoExists) return;
    if(pax_id == NoExists) {
        pax_id = apax_id;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   surname, "
            "   name, "
            "   document, "
            "   ticket_rem, "
            "   ticket_no, "
            "   coupon_no, "
            "   DECODE( "
            "       pax.SEAT_TYPE, "
            "       'SMSA',1, "
            "       'SMSW',1, "
            "       'SMST',1, "
            "       0) pr_smoke, "
            "   reg_no, "
            "   seats, "
            "   pers_type "
            "from "
            "   pax "
            "where "
            "   pax_id = :pax_id ";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TPrnTagStore::TPaxInfo::Init no data found for pax_id = %d", pax_id);
        surname = Qry.FieldAsString("surname");
        name = Qry.FieldAsString("name");
        document = Qry.FieldAsString("document");
        ticket_rem = Qry.FieldAsString("ticket_rem");
        ticket_no = Qry.FieldAsString("ticket_no");
        coupon_no = Qry.FieldAsInteger("coupon_no");
        pr_smoke = Qry.FieldAsInteger("pr_smoke") != 0;
        reg_no = Qry.FieldAsInteger("reg_no");
        seats = Qry.FieldAsInteger("seats");
        pers_type = Qry.FieldAsString("pers_type");
    }
}

void TPrnTagStore::TGrpInfo::Init(int agrp_id)
{
    if(grp_id == NoExists) {
        grp_id = agrp_id;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   point_dep, "
            "   point_arv, "
            "   airp_dep, "
            "   airp_arv, "
            "   class_grp, "
            "   excess "
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
        excess = Qry.FieldAsInteger("excess");
    }
}

void TPrnTagStore::TPointInfo::Init(int apoint_id, int agrp_id)
{
    if(point_id == NoExists) {
        point_id = apoint_id;
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select "
            "   airline, "
            "   flt_no, "
            "   suffix, "
            "   airp, "
            "   points.scd_out, "
            "   NVL( points.est_out, points.scd_out ) est, "
            "   NVL( points.act_out, NVL( points.est_out, points.scd_out ) ) act, "
            "   craft, "
            "   points.BORT "
            "from "
            "   points "
            "where "
            "   points.point_id = :point_id and points.pr_del>=0 ";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("TPointInfo::Init failed. point_id %d not found", point_id);
        scd = Qry.FieldAsDateTime("scd_out");
        est = Qry.FieldAsDateTime("est");
        act = Qry.FieldAsDateTime("act");
        craft = Qry.FieldAsString("craft");
        bort = Qry.FieldAsString("bort");
        TTripInfo operFlt(Qry);
        airline = operFlt.airline;
        flt_no = operFlt.flt_no;
        suffix = operFlt.suffix;
        Qry.Clear();
        Qry.SQLText=
            "SELECT airline, flt_no, suffix, airp_dep AS airp, scd AS scd_out "
            "FROM market_flt WHERE grp_id=:grp_id";
        Qry.CreateVariable("grp_id",otInteger,agrp_id);
        Qry.Execute();
        if (!Qry.Eof)
        {
            TTripInfo markFlt(Qry);
            TCodeShareSets codeshareSets;
            codeshareSets.get(operFlt,markFlt);
            if ( codeshareSets.pr_mark_bp )
            {
                airline = markFlt.airline;
                flt_no = markFlt.flt_no;
                suffix = markFlt.suffix;
            };
        }
    }
}

string TPrnTagStore::BCBP_M_2(TFieldParams fp)
{
    ostringstream result;
    result
        << "M"
        << 1;
    // Passenger Name
    string surname = SURNAME(fp);
    string name = NAME(fp);
    string pax_name = surname;
    if(!name.empty())
        pax_name += "/" + name;
    if(pax_name.size() > 20){
        size_t diff = pax_name.size() - 20;
        if(name.empty()) {
            result << surname.substr(0, surname.size() - diff);
        } else {
            if(name.size() > diff) {
                name = name.substr(0, name.size() - diff);
            } else {
                diff -= name.size() - 1;
                name = name[0];
                surname = surname.substr(0, surname.size() - diff);
            }
            result << surname + "/" + name;
        }
    } else
        result << setw(20) << left << pax_name;

    // Electronic Ticket Indicator
    result << (ETKT(fp).empty() ? " " : "E");
    // Operating carrier PNR code
    vector<TPnrAddrItem>::iterator iv = pnrInfo.pnrs.begin();
    for(; iv != pnrInfo.pnrs.end(); iv++)
        if(pointInfo.airline == iv->airline) {
            ProgTrace(TRACE5, "PNR found: %s", iv->addr);
            break;
        }
    if(iv == pnrInfo.pnrs.end())
        result << setw(7) << " ";
    else if(strlen(iv->addr) <= 7)
        result << setw(7) << left << convert_pnr_addr(iv->addr, tag_lang.GetLang() != AstraLocale::LANG_RU);
    // From City Airport Code
    result << setw(3) << AIRP_DEP(fp);
    // To City Airport Code
    result << setw(3) << AIRP_ARV(fp);
    // Operating Carrier Designator
    result << setw(3) << AIRLINE(fp);
    // Flight Number
    result
        << setw(4) << right << setfill('0') << pointInfo.flt_no
        << setw(1) << setfill(' ') << tag_lang.ElemIdToTagElem(etSuffix, pointInfo.suffix, efmtCodeNative);
    // Date of Flight
    TDateTime scd = pointInfo.scd;
    int Year, Month, Day;
    DecodeDate(scd, Year, Month, Day);
    TDateTime first, last;
    EncodeDate(Year, 1, 1, first);
    EncodeDate(Year, Month, Day, last);
    TDateTime period = last + 1 - first;
    result
        << fixed << setprecision(0) << setw(3) << setfill('0') << period;
    // Compartment Code
    result << CLASS(fp);
    // Seat Number
    result << setw(4) << right << ONE_SEAT_NO(fp);
    // Check-In Sequence Number
    result
        << setw(4) <<  setfill('0') << paxInfo.reg_no
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
        TPerson pers_type = DecodePerson((char *)paxInfo.pers_type.c_str());
        int result_pers_type;
        switch(pers_type) {
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
        cond1 << "B";
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
        cond1 << setw(10) << right << setfill('0') << paxInfo.pax_id;

    }

    // Field size of following varible size field
    result << setw(2) << right << setfill('0') << hex << uppercase << cond1.str().size();
    result << cond1.str();

    return result.str();
}

string TPrnTagStore::AIRLINE(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtCodeNative);
}

string TPrnTagStore::ACT(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(pointInfo.act, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::AIRLINE_SHORT(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtNameShort);
}

string TPrnTagStore::AIRLINE_NAME(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtNameLong);
}

string TPrnTagStore::AIRP_ARV(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtCodeNative);
}

string TPrnTagStore::AIRP_ARV_NAME(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtNameLong);
}

string TPrnTagStore::AIRP_DEP(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtCodeNative);
}

string TPrnTagStore::AIRP_DEP_NAME(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtNameLong);
}

string TPrnTagStore::BAG_AMOUNT(TFieldParams fp)
{
    return IntToString(bagInfo.bag_amount);
}

string TPrnTagStore::BAG_WEIGHT(TFieldParams fp)
{
    return IntToString(bagInfo.bag_weight);
}

string TPrnTagStore::BRD_FROM(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(brdInfo.brd_from, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::BRD_TO(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(brdInfo.brd_to, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
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
    TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_arv);
    return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
}

string TPrnTagStore::CITY_DEP_NAME(TFieldParams fp)
{
    TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_dep);
    return tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong);
}

string TPrnTagStore::CLASS(TFieldParams fp)
{
    string result;
    if(grpInfo.class_grp != NoExists)
        result = tag_lang.ElemIdToTagElem(etClsGrp, grpInfo.class_grp, efmtCodeNative);
    return result;
}

string TPrnTagStore::CLASS_NAME(TFieldParams fp)
{
    string result;
    if(grpInfo.class_grp != NoExists)
        result = tag_lang.ElemIdToTagElem(etClsGrp, grpInfo.class_grp, efmtNameLong);
    return result;
}

string TPrnTagStore::DOCUMENT(TFieldParams fp)
{
    return paxInfo.document;
}

string TPrnTagStore::EST(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(pointInfo.est, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::ETICKET_NO(TFieldParams fp) // !!! lat ???
{
    ostringstream result;
    if(paxInfo.ticket_rem == "TKNE")
        result << paxInfo.ticket_no << "/" << paxInfo.coupon_no;
    return result.str();
}

string TPrnTagStore::ETKT(TFieldParams fp)
{
    string result = ETICKET_NO(fp);
    if(not result.empty())
        result = "ETKT" + result;
    return result;
}

string TPrnTagStore::EXCESS(TFieldParams fp)
{
    return IntToString(grpInfo.excess);
}

string TPrnTagStore::FLT_NO(TFieldParams fp)
{
    ostringstream result;
    result << setw(3) << setfill('0') << pointInfo.flt_no << pointInfo.suffix;
    return result.str();
}

string TPrnTagStore::FQT(TFieldParams fp)
{
    string airline = tag_lang.ElemIdToTagElem(etAirline, fqtInfo.airline, efmtCodeNative);
    string extra = transliter(fqtInfo.extra, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    string result = (airline + " " + fqtInfo.no + string(" ", extra.empty() ? 0 : 1) + extra);
    size_t min_width = airline.size() + fqtInfo.no.size() + 1;
    if(fp.len == 0)
        ;
    else if(min_width > fp.len)
        result = fqtInfo.no;
    else
        result = result.substr(0, fp.len > min_width ? fp.len : fp.len == 0 ? string::npos : min_width);
    return result;
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
    return
        transliter(paxInfo.name.empty() ? paxInfo.surname : paxInfo.surname + " " + paxInfo.name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU)
        .substr(0, fp.len > 10 ? fp.len : fp.len == 0 ? string::npos : 10);
}

string TPrnTagStore::GATE(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        throw AstraLocale::UserException("MSG.GATE_NOT_SPECIFIED");
    return boost::any_cast<string>(fp.TagInfo);
}

string cut_place(string airp, string city_name, int len)
{
    if(not city_name.empty() and not airp.empty()) {
        int diff = len - (airp.size() + 2);
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
        TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_arv);
        result = cut_long_place(
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtCodeNative, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, AstraLocale::LANG_EN),
                tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtCodeNative, AstraLocale::LANG_EN),
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
        TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_dep);
        result = cut_long_place(
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtCodeNative, tag_lang.dup_lang()),
                tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, AstraLocale::LANG_EN),
                tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtCodeNative, AstraLocale::LANG_EN),
                fp.len
                );
    }
    return result;
}

string TPrnTagStore::NAME(TFieldParams fp)
{
    string result;
    if(fp.TagInfo.empty())
        result = transliter(paxInfo.name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    else
        result = SURNAME(fp);
    return result;
}

string TPrnTagStore::NO_SMOKE(TFieldParams fp)
{
    return (paxInfo.pr_smoke ? " " : "X");
}

string TPrnTagStore::ONE_SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   salons.get_seat_no(:pax_id,:seats,NULL,NULL,'one',NULL,:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    Qry.CreateVariable("is_inter", otInteger, tag_lang.GetLang() != AstraLocale::LANG_RU);
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string TPrnTagStore::PAX_ID(TFieldParams fp)
{
    ostringstream result;
    result << setw(10) << setfill('0') << paxInfo.pax_id;
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

string TPrnTagStore::REG_NO(TFieldParams fp)
{
    ostringstream result;
    result << setw(3) << setfill('0') << paxInfo.reg_no;
    return result.str();
}

string TPrnTagStore::SCD(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(pointInfo.scd, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   salons.get_seat_no(:pax_id,:seats,NULL,NULL,'seats',NULL,:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    Qry.CreateVariable("is_inter", otInteger, tag_lang.GetLang() != AstraLocale::LANG_RU);
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string TPrnTagStore::STR_SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   salons.get_seat_no(:pax_id,:seats,NULL,NULL,'voland',NULL,:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    ProgTrace(TRACE5, "tag_lang.GetLang(): '%s'", tag_lang.GetLang().c_str());
    Qry.CreateVariable("is_inter", otInteger, tag_lang.GetLang() != AstraLocale::LANG_RU);
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string TPrnTagStore::LIST_SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   salons.get_seat_no(:pax_id,:seats,NULL,NULL,'list',NULL,:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    ProgTrace(TRACE5, "tag_lang.GetLang(): '%s'", tag_lang.GetLang().c_str());
    Qry.CreateVariable("is_inter", otInteger, tag_lang.GetLang() != AstraLocale::LANG_RU);
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string get_unacc_name(int bag_type, bool is_inter)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   decode(:is_inter, 0, name, name_lat) name "
        "from "
        "   unaccomp_bag_names "
        "where "
        "   code = :code ";
    Qry.CreateVariable("is_inter", otInteger, is_inter);
    Qry.CreateVariable("code", otInteger, bag_type);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("get_unacc_name: name not found for bag_type %d", bag_type);
    return Qry.FieldAsString("name");
}

string TPrnTagStore::SURNAME(TFieldParams fp)
{
    string result;
    if(fp.TagInfo.empty())
        result = transliter(paxInfo.surname, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    else {
        if(boost::any_cast<string>(&fp.TagInfo))
            result = getLocaleText(boost::any_cast<string>(fp.TagInfo), tag_lang.GetLang());
        else if(boost::any_cast<int>(&fp.TagInfo))
            result = get_unacc_name(boost::any_cast<int>(fp.TagInfo), tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
            throw Exception("TPrnTagStore::SURNAME: unexpected TagInfo type");
    }
    return result.substr(0, fp.len > 8 ? fp.len : fp.len == 0 ? string::npos : 8);
}

string TPrnTagStore::TEST_SERVER(TFieldParams fp)
{
    if(fp.len == 0) fp.len = 300;
    string result;
    string test = getLocaleText("CAP.TEST", tag_lang.GetLang());
    while(result.size() < fp.len)
        result += test + " ";
    return result.substr(0, fp.len);
}

string TPrnTagStore::AIRCODE(TFieldParams fp) {
    ostringstream result;
    result << setw(fp.len) << setfill('0') << boost::any_cast<int>(fp.TagInfo);
    return result.str();
}

string TPrnTagStore::NO(TFieldParams fp) {
    ostringstream result;
    result << setw(fp.len) << setfill('0') << boost::any_cast<int>(fp.TagInfo);
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
            suffix = tag_lang.ElemIdToTagElem(etSuffix, flt_no[flt_no.size() - 1], efmtCodeNative);
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
    result << setw(fp.len) << setfill('0') << Day;
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

string TPrnTagStore::PNR(TFieldParams fp) {
    string pnr;
    if (!pnrInfo.pnrs.empty()) {
        pnr = convert_pnr_addr(pnrInfo.pnrs[0].addr, tag_lang.GetLang() != AstraLocale::LANG_RU);
        if(pnrInfo.pnrs[0].airline!=pnrInfo.airline and pnr.size() + strlen(pnrInfo.pnrs[0].airline) + 1 >= fp.len)
            pnr += "/" + tag_lang.ElemIdToTagElem(etAirline, pnrInfo.pnrs[0].airline, efmtCodeNative);
    }
    return pnr;
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

string TPrnTagStore::BAG_NAME(TFieldParams fp)
{
    string result;
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
    return tag_lang.ElemIdToTagElem(etAirline, rcpt.airline, efmtNameLong);
}

string TPrnTagStore::AIRLINE_CODE(TFieldParams fp)
{
    ostringstream result;
    result << tag_lang.ElemIdToTagElem(etAirline, rcpt.airline, efmtCodeNative);
    if(rcpt.flt_no != -1)
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
    if(pr_exchange())
        result
            << RateToString(pay_rate_sum(), pay_rate_cur, pr_inter, fmt)
            << "(" << RateToString(rate_sum(), rate_cur, pr_inter, fmt) << ")";
    else
        result << RateToString(rate_sum(), rate_cur, pr_inter, fmt);
    return result.str();
}

string TPrnTagStore::AMOUNT_FIGURES(TFieldParams fp)
{
    return rcpt.get_fmt_rate(1, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::AMOUNT_LETTERS(TFieldParams fp)
{
    int iptr, fract;
    fract=separate_double(rcpt.pr_exchange() ? rcpt.pay_rate_sum() : rcpt.rate_sum(), 2, &iptr);

    string result = vs_number(iptr, tag_lang.GetLang() != AstraLocale::LANG_RU);

    if(fract!=0) {
        string str_fract = IntToString(fract) + "/100 ";
        result += str_fract;
    }
    return result;
}

string TPrnTagStore::CURRENCY(TFieldParams fp)
{
    return rcpt.get_fmt_rate(2, tag_lang.GetLang() != AstraLocale::LANG_RU);
}

string TPrnTagStore::EX_WEIGHT(TFieldParams fp)
{
    ostringstream result;
    result << rcpt.ex_weight;
    if (rcpt.form_type == "M61")
        result << getLocaleText("MSG.BR.KG", tag_lang.GetLang());
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
    if (rcpt.pay_rate_cur != rcpt.rate_cur &&
            (not rcpt.pr_exchange() or rcpt.form_type != "M61"))
    {
        if (rcpt.form_type != "M61" and tag_lang.GetLang() != AstraLocale::LANG_RU) result << "RATE ";
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
        ProgTrace(TRACE5, "BUFFER: %s", buf.c_str());
        int line_num = 0;
        while(line_num < 5) {
            string::size_type i = buf.find('\n');
            f_issue_place_idx.push_back(buf.substr(0, i));
            buf.erase(0, i + 1);
            line_num++;
        }
        if(f_issue_place_idx.size() != 5)
            throw Exception("TBagReceipt::issue_place_idx: wrong issue_place format: lines = %d", f_issue_place_idx.size());
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
                if (k==0 && i->pay_type!=CASH_PAY_TYPE_ID ||
                        k!=0 && i->pay_type==CASH_PAY_TYPE_ID) continue;

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
    return result.str();
}

string TPrnTagStore::REMARKS2(TFieldParams fp)
{
    string result;
    if(rcpt.service_type == 1 || rcpt.service_type == 2)
        result = EX_WEIGHT(fp);
    return result;
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
    return POINT_DEP(fp) + "-" + POINT_ARV(fp) + " " + AIRLINE_CODE(fp);
}

string TPrnTagStore::TOTAL(TFieldParams fp)
{
  return RateToString(rcpt.pay_rate_sum(), rcpt.pay_rate_cur, tag_lang.GetLang() != AstraLocale::LANG_RU, 0);
}
