#include "prn_tag_store.h"
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "exceptions.h"
#include "oralib.h"
#include "stages.h"
#include "astra_utils.h"
#include "astra_misc.h"

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

void TPrnTagStore::TTagLang::Init(int point_dep, int point_arv, bool apr_lat)
{
    string route_country;
    route_inter = IsRouteInter(point_dep, point_arv, route_country);
    if(route_country == "��")
        route_country_lang = AstraLocale::LANG_RU;
    else
        route_country_lang = "";
    pr_lat = apr_lat;
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

  string lang=GetLang(fmt, firm_lang); //ᯥ樠�쭮 �뭥ᥭ�, ⠪ ��� fmt � ��楤�� ����� ����������

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

  string lang=GetLang(fmt, firm_lang); //ᯥ樠�쭮 �뭥ᥭ�, ⠪ ��� fmt � ��楤�� ����� ����������

  vector< pair<TElemFmt,string> > fmts;

  getElemFmts(fmt, lang, fmts);
  return ElemIdToElem(type, id, fmts, true);
};


void TPrnTagStore::tst_get_tag_list(std::vector<string> &tags)
{
    for(map<const string, TTagListItem>::iterator im = tag_list.begin(); im != tag_list.end(); im++)
        tags.push_back(im->first);
}

TPrnTagStore::TPrnTagStore(int agrp_id, int apax_id, int apr_lat)
{
    grpInfo.Init(agrp_id);
    tag_lang.Init(grpInfo.point_dep, grpInfo.point_arv, apr_lat != 0);
    pax_id = apax_id;

    tag_list.insert(make_pair(TAG::BCBP_M_2,        TTagListItem(&TPrnTagStore::BCBP_M_2, POINT_INFO | PAX_INFO | PNR_INFO)));
    tag_list.insert(make_pair(TAG::ACT,             TTagListItem(&TPrnTagStore::ACT, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE,         TTagListItem(&TPrnTagStore::AIRLINE, POINT_INFO)));
    tag_list.insert(make_pair(TAG::AIRLINE_SHORT,   TTagListItem(&TPrnTagStore::AIRLINE_SHORT, POINT_INFO)));
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
    tag_list.insert(make_pair(TAG::SURNAME,         TTagListItem(&TPrnTagStore::SURNAME, PAX_INFO)));
    tag_list.insert(make_pair(TAG::TEST_SERVER,     TTagListItem(&TPrnTagStore::TEST_SERVER)));

}

void TPrnTagStore::set_tag(string name, string value)
{
    name = upperc(name);
    map<const string, TTagListItem>::iterator im = tag_list.find(name);
    if(im == tag_list.end())
        throw Exception("TPrnTagStore::set_tag: tag '%s' not implemented", name.c_str());
    im->second.TagInfo = value;
}

string TPrnTagStore::get_field(std::string name, int len, std::string align, std::string date_format, string tag_lang)
{
    this->tag_lang.set_tag_lang(tag_lang);
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
    string result = (this->*im->second.tag_funct)(TFieldParams(date_format, im->second.TagInfo, len));
    if(!len) len = result.size();
    return AlignString(result, len, align);
}

void TPrnTagStore::TPnrInfo::Init(int apax_id)
{
    if(not pr_init) {
        pr_init = true;
        GetPaxPnrAddr(apax_id, pnrs);
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
            "       'SMSA',' ', "
            "       'SMSW',' ', "
            "       'SMST',' ', "
            "       'X') no_smoke, "
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
        ticket_no = Qry.FieldAsFloat("ticket_no");
        coupon_no = Qry.FieldAsInteger("coupon_no");
        no_smoke = Qry.FieldAsString("no_smoke");
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
        result << setw(7) << left << convert_pnr_addr(iv->addr, tag_lang.IsInter());
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
    // � ⠪ ������� �� � �⮬� ������� (�.�. �뢮� ���. ⠫��� �� �����)
    // ����� ���ᠦ�� "1": passenger checked in
    result << 1;

    ostringstream cond1; // first conditional field
    { // filling up cond1
        cond1
            << ">"
            << 2;
        // field size of following structured message
        // ����ﭭ�� ���祭�� ࠢ��� �㬬� ��१�ࢨ஢����� ���� ��᫥����� 7-� �����
        // � ������ ���ᨨ �� ����� ࠢ�� 24 (������� ����)
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
    return DateTimeToStr(UTCToLocal(pointInfo.act, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.IsInter());
}

string TPrnTagStore::AIRLINE_SHORT(TFieldParams fp)
{
    return tag_lang.ElemIdToTagElem(etAirline, pointInfo.airline, efmtNameShort);
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
    return DateTimeToStr(UTCToLocal(brdInfo.brd_from, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.IsInter());
}

string TPrnTagStore::BRD_TO(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(brdInfo.brd_to, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.IsInter());
}

string TPrnTagStore::BT_AMOUNT(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        return "";
    else
        return boost::any_cast<string>(fp.TagInfo);
}

string TPrnTagStore::BT_WEIGHT(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        return "";
    else
        return boost::any_cast<string>(fp.TagInfo);
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
    return DateTimeToStr(UTCToLocal(pointInfo.est, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.IsInter());
}

string TPrnTagStore::ETICKET_NO(TFieldParams fp) // !!! lat ???
{
    ostringstream result;
    if(paxInfo.ticket_rem == "TKNE")
        result << fixed << setprecision(0) << paxInfo.ticket_no << "/" << paxInfo.coupon_no;
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
    string result;
    if(not fqtInfo.airline.empty()) {
        result += tag_lang.ElemIdToTagElem(etAirline, fqtInfo.airline, efmtCodeNative) + " " + fqtInfo.no;
        if(not fqtInfo.extra.empty())
            result += " " + transliter(fqtInfo.extra, 1, tag_lang.IsInter());
    }
    return result;
}

string TPrnTagStore::FULL_PLACE_ARV(TFieldParams fp)
{
    return CITY_ARV_NAME(fp) + " " + AIRP_ARV_NAME(fp);
}

string TPrnTagStore::FULL_PLACE_DEP(TFieldParams fp)
{
    return CITY_DEP_NAME(fp) + " " + AIRP_DEP_NAME(fp);
}

string TPrnTagStore::FULLNAME(TFieldParams fp)
{
    return transliter(paxInfo.surname + " " + paxInfo.name, 1, tag_lang.IsInter());
}

string TPrnTagStore::GATE(TFieldParams fp)
{
    if(fp.TagInfo.empty())
        throw AstraLocale::UserException("MSG.GATE_NOT_SPECIFIED");
    return boost::any_cast<string>(fp.TagInfo);
}

string TPrnTagStore::LONG_ARV(TFieldParams fp)
{
    string result = CITY_ARV_NAME(fp).substr(0, 9) + "(" + AIRP_ARV(fp) + ")"; // !!! � ��襬 ��砥 ��୥��� ���᪨� ���?
    if(not tag_lang.IsInter()) {
        TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_arv);
        result =
            tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()).substr(0, 9) +
            tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_arv, efmtCodeNative, tag_lang.dup_lang()) +
            "/" + result;
    }
    return result;
}

string TPrnTagStore::LONG_DEP(TFieldParams fp)
{
    string result = CITY_DEP_NAME(fp).substr(0, 9) + "(" + AIRP_DEP(fp) + ")"; // !!! � ��襬 ��砥 ��୥��� ���᪨� ���?
    if(not tag_lang.IsInter()) {
        TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("AIRPS").get_row("code",grpInfo.airp_dep);
        result =
            tag_lang.ElemIdToTagElem(etCity, airpRow.city, efmtNameLong, tag_lang.dup_lang()).substr(0, 9) +
            tag_lang.ElemIdToTagElem(etAirp, grpInfo.airp_dep, efmtCodeNative, tag_lang.dup_lang()) +
            "/" + result;
    }
    return result;
}

string TPrnTagStore::NAME(TFieldParams fp)
{
    return transliter(paxInfo.name, 1, tag_lang.IsInter());
}

string TPrnTagStore::NO_SMOKE(TFieldParams fp)
{
    return paxInfo.no_smoke;
}

string TPrnTagStore::ONE_SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   system.transliter(salons.get_seat_no(:pax_id,:seats,NULL,NULL,'one',NULL,1),:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    Qry.CreateVariable("is_inter", otInteger, tag_lang.IsInter());
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
    return CITY_ARV_NAME(fp).substr(0, 7) + "(" + AIRP_ARV(fp) + ")";
}

string TPrnTagStore::PLACE_DEP(TFieldParams fp)
{
    return CITY_DEP_NAME(fp).substr(0, 7) + "(" + AIRP_DEP(fp) + ")";
}

string TPrnTagStore::REG_NO(TFieldParams fp)
{
    ostringstream result;
    result << setw(3) << setfill('0') << paxInfo.reg_no;
    return result.str();
}

string TPrnTagStore::SCD(TFieldParams fp)
{
    return DateTimeToStr(UTCToLocal(pointInfo.scd, AirpTZRegion(grpInfo.airp_dep)), fp.date_format, tag_lang.IsInter());
}

string TPrnTagStore::SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   system.transliter(salons.get_seat_no(:pax_id,:seats,NULL,NULL,'seats',NULL,1),:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    Qry.CreateVariable("is_inter", otInteger, tag_lang.IsInter());
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string TPrnTagStore::STR_SEAT_NO(TFieldParams fp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   system.transliter(salons.get_seat_no(:pax_id,:seats,NULL,NULL,'voland',NULL,1),:is_inter) AS seat_no "
        "from dual";
    Qry.CreateVariable("pax_id", otInteger, paxInfo.pax_id);
    Qry.CreateVariable("seats", otInteger, paxInfo.seats);
    Qry.CreateVariable("is_inter", otInteger, tag_lang.IsInter());
    Qry.Execute();
    return Qry.FieldAsString("seat_no");
}

string TPrnTagStore::SURNAME(TFieldParams fp)
{
    return transliter(paxInfo.surname, 1, tag_lang.IsInter());
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
