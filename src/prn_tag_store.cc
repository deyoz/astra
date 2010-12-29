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

bool TPrnTagStore::TTagLang::IsInter(TBTRoute *aroute, string &country)
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

void TPrnTagStore::TTagLang::Init(bool apr_lat)
{
    pr_lat = apr_lat;
}

void TPrnTagStore::TTagLang::Init(int point_dep, int point_arv, TBTRoute *aroute, bool apr_lat)
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

bool TPrnTagStore::TTagLang::IsTstInter() const
{
    return tag_lang == "E" or pr_lat;
}

bool TPrnTagStore::TTagLang::IsInter() const
{
    return
        (route_inter || route_country_lang.empty() || route_country_lang!=TReqInfo::Instance()->desk.lang) or
        tag_lang == "E" or
        pr_lat;
}

string TPrnTagStore::TTagLang::GetLang(TElemFmt &fmt, string firm_lang) const
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

string TPrnTagStore::TTagLang::ElemIdToTagElem(TElemType type, const string &id, TElemFmt fmt, string firm_lang) const
{
  if (id.empty()) return "";

  string lang=GetLang(fmt, firm_lang); //специально вынесено, так как fmt в процедуре может измениться

  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, lang, fmts);
  return ElemIdToElem(type, id, fmts, true);
};

string TPrnTagStore::TTagLang::GetLang()
{
  string lang = TReqInfo::Instance()->desk.lang;
  if (IsInter()) lang=AstraLocale::LANG_EN;
  return lang;
}

string TPrnTagStore::TTagLang::ElemIdToTagElem(TElemType type, int id, TElemFmt fmt, string firm_lang) const
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

TPrnTagStore::TPrnTagStore(bool apr_lat)
{
    tag_lang.Init(apr_lat);
    prn_test_tags.Init();
}

TPrnTagStore::TPrnTagStore(int agrp_id, int apax_id, int apr_lat, xmlNodePtr tagsNode, TBTRoute *aroute)
{
    grpInfo.Init(agrp_id);
    tag_lang.Init(grpInfo.point_dep, grpInfo.point_arv, aroute, apr_lat != 0);
    pax_id = apax_id;
    if(aroute == NULL and pax_id == NoExists)
        throw Exception("TPrnTagStore::TPrnTagStore: pax_id not defined for bp mode");

    tag_list.insert(make_pair(TAG::BCBP_M_2,        TTagListItem(&TPrnTagStore::BCBP_M_2, POINT_INFO | PAX_INFO | PNR_INFO)));
    tag_list.insert(make_pair(TAG::ACT,             TTagListItem(&TPrnTagStore::ACT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE,         TTagListItem(&TPrnTagStore::AIRLINE, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_SHORT,   TTagListItem(&TPrnTagStore::AIRLINE_SHORT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_NAME,    TTagListItem(&TPrnTagStore::AIRLINE_NAME, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRP_ARV,        TTagListItem(&TPrnTagStore::AIRP_ARV)));
    tag_list.insert(make_pair(TAG::AIRP_ARV_NAME,   TTagListItem(&TPrnTagStore::AIRP_ARV_NAME)));
    tag_list.insert(make_pair(TAG::AIRP_DEP,        TTagListItem(&TPrnTagStore::AIRP_DEP)));
    tag_list.insert(make_pair(TAG::AIRP_DEP_NAME,   TTagListItem(&TPrnTagStore::AIRP_DEP_NAME)));
    tag_list.insert(make_pair(TAG::BAG_AMOUNT,      TTagListItem(&TPrnTagStore::BAG_AMOUNT, BAG_INFO)));
    tag_list.insert(make_pair(TAG::BAG_WEIGHT,      TTagListItem(&TPrnTagStore::BAG_WEIGHT, BAG_INFO)));
    tag_list.insert(make_pair(TAG::BRD_FROM,        TTagListItem(&TPrnTagStore::BRD_FROM, BRD_INFO)));
    tag_list.insert(make_pair(TAG::BRD_TO,          TTagListItem(&TPrnTagStore::BRD_TO, BRD_INFO)));
    tag_list.insert(make_pair(TAG::BT_AMOUNT,       TTagListItem(&TPrnTagStore::BT_AMOUNT)));
    tag_list.insert(make_pair(TAG::BT_WEIGHT,       TTagListItem(&TPrnTagStore::BT_WEIGHT)));
    tag_list.insert(make_pair(TAG::CITY_ARV_NAME,   TTagListItem(&TPrnTagStore::CITY_ARV_NAME)));
    tag_list.insert(make_pair(TAG::CITY_DEP_NAME,   TTagListItem(&TPrnTagStore::CITY_DEP_NAME)));
    tag_list.insert(make_pair(TAG::CLASS,           TTagListItem(&TPrnTagStore::CLASS)));
    tag_list.insert(make_pair(TAG::CLASS_NAME,      TTagListItem(&TPrnTagStore::CLASS_NAME)));
    tag_list.insert(make_pair(TAG::DOCUMENT,        TTagListItem(&TPrnTagStore::DOCUMENT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::EST,             TTagListItem(&TPrnTagStore::EST, POINT_INFO)));
    tag_list.insert(make_pair(TAG::ETICKET_NO,      TTagListItem(&TPrnTagStore::ETICKET_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::ETKT,            TTagListItem(&TPrnTagStore::ETKT, PAX_INFO)));
    tag_list.insert(make_pair(TAG::EXCESS,          TTagListItem(&TPrnTagStore::EXCESS)));
    tag_list.insert(make_pair(TAG::FLT_NO,          TTagListItem(&TPrnTagStore::FLT_NO, POINT_INFO)));
    tag_list.insert(make_pair(TAG::FQT,             TTagListItem(&TPrnTagStore::FQT, FQT_INFO)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_ARV,  TTagListItem(&TPrnTagStore::FULL_PLACE_ARV)));
    tag_list.insert(make_pair(TAG::FULL_PLACE_DEP,  TTagListItem(&TPrnTagStore::FULL_PLACE_DEP)));
    tag_list.insert(make_pair(TAG::FULLNAME,        TTagListItem(&TPrnTagStore::FULLNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::GATE,            TTagListItem(&TPrnTagStore::GATE)));
    tag_list.insert(make_pair(TAG::LONG_ARV,        TTagListItem(&TPrnTagStore::LONG_ARV)));
    tag_list.insert(make_pair(TAG::LONG_DEP,        TTagListItem(&TPrnTagStore::LONG_DEP)));
    tag_list.insert(make_pair(TAG::NAME,            TTagListItem(&TPrnTagStore::NAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::NO_SMOKE,        TTagListItem(&TPrnTagStore::NO_SMOKE, PAX_INFO)));
    tag_list.insert(make_pair(TAG::ONE_SEAT_NO,     TTagListItem(&TPrnTagStore::ONE_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PAX_ID,          TTagListItem(&TPrnTagStore::PAX_ID, PAX_INFO)));
    tag_list.insert(make_pair(TAG::PLACE_ARV,       TTagListItem(&TPrnTagStore::PLACE_ARV)));
    tag_list.insert(make_pair(TAG::PLACE_DEP,       TTagListItem(&TPrnTagStore::PLACE_DEP)));
    tag_list.insert(make_pair(TAG::REG_NO,          TTagListItem(&TPrnTagStore::REG_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SCD,             TTagListItem(&TPrnTagStore::SCD, POINT_INFO)));
    tag_list.insert(make_pair(TAG::SEAT_NO,         TTagListItem(&TPrnTagStore::SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::STR_SEAT_NO,     TTagListItem(&TPrnTagStore::STR_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::LIST_SEAT_NO,    TTagListItem(&TPrnTagStore::LIST_SEAT_NO, PAX_INFO)));
    tag_list.insert(make_pair(TAG::SURNAME,         TTagListItem(&TPrnTagStore::SURNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::TEST_SERVER,     TTagListItem(&TPrnTagStore::TEST_SERVER)));

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
    tag_list.insert(make_pair(TAG::PNR,             TTagListItem(&TPrnTagStore::PNR, PNR_INFO)));

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

string TPrnTagStore::get_test_field(std::string name, int len, std::string date_format)
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

string TPrnTagStore::get_real_field(std::string name, int len, std::string date_format)
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

string TPrnTagStore::get_tag(std::string name)
{
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
    if(fp.TagInfo.empty())
        return transliter(paxInfo.name, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    else
        return SURNAME(fp);
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
    if(fp.TagInfo.empty())
        return transliter(paxInfo.surname, 1, tag_lang.GetLang() != AstraLocale::LANG_RU);
    else {
        if(boost::any_cast<string>(&fp.TagInfo))
            return getLocaleText(boost::any_cast<string>(fp.TagInfo), tag_lang.GetLang());
        else if(boost::any_cast<int>(&fp.TagInfo))
            return get_unacc_name(boost::any_cast<int>(fp.TagInfo), tag_lang.GetLang() != AstraLocale::LANG_RU);
        else
            throw Exception("TPrnTagStore::SURNAME: unexpected TagInfo type");
    }
}

string TPrnTagStore::TEST_SERVER(TFieldParams fp)
{
    if(fp.len == 0) fp.len = 300;
    string result;
    string test = getLocaleText("CAP.TEST", tag_lang.GetLang());
    while(result.size() < fp.len)
        result += test + " ";
    return result;
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
        if(pnrInfo.pnrs[0].airline!=pnrInfo.airline)
            pnr += "/" + tag_lang.ElemIdToTagElem(etAirline, pnrInfo.pnrs[0].airline, efmtCodeNative);
    }
    return pnr;
}

