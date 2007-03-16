#include "print.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "misc.h"
#include "stages.h"
#include "str_utils.h"
#include "docs.h"
#include <fstream>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

//////////////////////////////// CLASS PrintDataParser ///////////////////////////////////

class PrintDataParser {
    public:
        enum TMapType {mtBTBP, mtMSO};
    private:
        class t_field_map {
            private:
                struct TTagValue {
                    bool null, pr_print;
                    otFieldType type;
                    string StringVal;
                    double FloatVal;
                    int IntegerVal;
                    TDateTime DateTimeVal;
                };

                typedef map<string, TTagValue> TData;
                TData data;
                void dump_data();

                string class_checked;
                int pax_id;
                int pr_lat;
                typedef vector<TQuery*> TQrys;
                TQrys Qrys;
                TQuery *prnQry;

                void fillBTBPMap();
                void fillMSOMap();
                string check_class(string val);
                bool printed(TData::iterator di);

            public:
                t_field_map(int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type);
                string get_field(string name, int len, string align, string date_format, int field_lat);
                void add_tag(string name, int val);
                void add_tag(string name, string val);
                void add_tag(string name, TDateTime val);
                TQuery *get_prn_qry();
                ~t_field_map();
        };

        int pr_lat;
        t_field_map field_map;
        string parse_field(int offset, string field);
        string parse_tag(int offset, string tag);
    public:
        PrintDataParser(int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type = mtBTBP):
            field_map(pax_id, pr_lat, tagsNode, map_type)
        {
            this->pr_lat = pr_lat;
        };
        string parse(string &form);
        TQuery *get_prn_qry() { return field_map.get_prn_qry(); };
        void add_tag(string name, int val) { return field_map.add_tag(name, val); };
        void add_tag(string name, string val) { return field_map.add_tag(name, val); };
        void add_tag(string name, TDateTime val) { return field_map.add_tag(name, val); };
};

string PrintDataParser::t_field_map::check_class(string val)
{
    if(class_checked.size()) return class_checked;

    string result = val;
    TData::iterator di = data.find("AIRLINE");
    if(di == data.end()) throw Exception("PrintDataParser::t_field_map::check_class: AIRLINE tag not found");
    if(di->second.StringVal == "ЮТ") {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "select * from pax_rem where pax_id = :pax_id and rem_code = 'MCLS'";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof) {
            di = data.find("SUBCLASS");
            if(di == data.end()) throw Exception("PrintDataParser::t_field_map::check_class: SUBCLASS tag not found");
            if(di->second.StringVal == "М")
                result = "M";
        } else
            result = "M";
    }
    class_checked = result;
    return result;
}

bool PrintDataParser::t_field_map::printed(TData::iterator di)
{
    return di != data.end() && di->second.pr_print;
}

TQuery *PrintDataParser::t_field_map::get_prn_qry()
{
    prnQry = OraSession.CreateQuery();
    prnQry->SQLText =
        "begin "
        "   delete from bp_print where pax_id = :pax_id and pr_print = 0 and desk=:desk; "
        "   insert into bp_print( "
        "       pax_id, "
        "       time_print, "
        "       pr_print, "
        "       desk, "
        "       AIRLINE, "
        "       SCD, "
        "       BRD_FROM, "
        "       BRD_TO, "
        "       AIRP_ARV, "
        "       AIRP_DEP, "
        "       CLASS, "
        "       GATE, "
        "       REG_NO, "
        "       NAME, "
        "       SEAT_NO, "
        "       PR_SMOKE, "
        "       BAG_AMOUNT, "
        "       BAG_WEIGHT, "
        "       RK_WEIGHT, "
        "       TAGS, "
        "       EXCESS, "
        "       PERS_TYPE, "
        "       FLT_NO, "
        "       SURNAME, "
        "       SUFFIX "
        "   ) values( "
        "       :pax_id, "
        "       :now_utc, "
        "       0, "
        "       :desk, "
        "       :AIRLINE, "
        "       :SCD, "
        "       :BRD_FROM, "
        "       :BRD_TO, "
        "       :AIRP_ARV, "
        "       :AIRP_DEP, "
        "       :CLASS, "
        "       :GATE, "
        "       :REG_NO, "
        "       :NAME, "
        "       :SEAT_NO, "
        "       :PR_SMOKE, "
        "       :BAG_AMOUNT, "
        "       :BAG_WEIGHT, "
        "       :RK_WEIGHT, "
        "       :TAGS, "
        "       :EXCESS, "
        "       :PERS_TYPE, "
        "       :FLT_NO, "
        "       :SURNAME, "
        "       :SUFFIX "
        "   ); "
        "end;";
    prnQry->CreateVariable("pax_id", otInteger, pax_id);
    prnQry->CreateVariable("DESK", otString, TReqInfo::Instance()->desk.code);
    prnQry->DeclareVariable("AIRLINE", otString);
    prnQry->DeclareVariable("SCD", otDate);
    prnQry->DeclareVariable("BRD_FROM", otDate);
    prnQry->DeclareVariable("BRD_TO", otDate);
    prnQry->DeclareVariable("AIRP_ARV", otString);
    prnQry->DeclareVariable("AIRP_DEP", otString);
    prnQry->DeclareVariable("CLASS", otString);
    prnQry->DeclareVariable("GATE", otString);
    prnQry->DeclareVariable("REG_NO", otInteger);
    prnQry->DeclareVariable("NAME", otString);
    prnQry->DeclareVariable("SEAT_NO", otString);
    prnQry->DeclareVariable("PR_SMOKE", otInteger);
    prnQry->DeclareVariable("BAG_AMOUNT", otInteger);
    prnQry->DeclareVariable("BAG_WEIGHT", otInteger);
    prnQry->DeclareVariable("RK_WEIGHT", otInteger);
    prnQry->DeclareVariable("TAGS", otString);
    prnQry->DeclareVariable("EXCESS", otInteger);
    prnQry->DeclareVariable("PERS_TYPE", otString);
    prnQry->DeclareVariable("FLT_NO", otInteger);
    prnQry->DeclareVariable("SURNAME", otString);
    prnQry->DeclareVariable("SUFFIX", otString);

    TData::iterator di1, di2, di3;

    di1 = data.find("AIRLINE");
    di2 = data.find("AIRLINE_NAME");
    if(printed(di1) || printed(di2))
        prnQry->SetVariable(di1->first, di1->second.StringVal);

    di1 = data.find("SCD");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.DateTimeVal);

    di1 = data.find("BRD_FROM");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.DateTimeVal);


    di1 = data.find("BRD_TO");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.DateTimeVal);


    di1 = data.find("AIRP_ARV");
    di2 = data.find("AIRP_ARV_NAME");
    if(printed(di1) || printed(di2))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("AIRP_DEP");
    di2 = data.find("AIRP_DEP_NAME");
    if(printed(di1) || printed(di2))
        prnQry->SetVariable("AIRP_DEP", di1->second.StringVal);


    di1 = data.find("CLASS");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("GATE");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("REG_NO");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("NAME");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("SEAT_NO");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("PR_SMOKE");
    di2 = data.find("NO_SMOKE");
    di3 = data.find("SMOKE");
    if(printed(di1) || printed(di2) || printed(di3))
        prnQry->SetVariable(di1->first, di1->second.FloatVal);


    di1 = data.find("BAG_AMOUNT");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("BAG_WEIGHT");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("RK_WEIGHT");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("TAGS");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("EXCESS");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("PERS_TYPE");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("FLT_NO");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.IntegerVal);


    di1 = data.find("SURNAME");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("SUFFIX");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);

    return prnQry;
}

void PrintDataParser::t_field_map::add_tag(string name, TDateTime val)
{
    name = upperc(name);
    TTagValue TagValue;
    TagValue.null = false;
    TagValue.type = otDate;
    TagValue.DateTimeVal = val;
    data[name] = TagValue;
}

void PrintDataParser::t_field_map::add_tag(string name, string val)
{
    name = upperc(name);
    TTagValue TagValue;
    TagValue.null = false;
    TagValue.type = otString;
    TagValue.StringVal = val;
    data[name] = TagValue;
}

void PrintDataParser::t_field_map::add_tag(string name, int val)
{
    name = upperc(name);
    TTagValue TagValue;
    TagValue.null = false;
    TagValue.type = otInteger;
    TagValue.IntegerVal = val;
    data[name] = TagValue;
}

void PrintDataParser::t_field_map::dump_data()
{
        ProgTrace(TRACE5, "------MAP DUMP------");
        for(TData::iterator di = data.begin(); di != data.end(); ++di) {
            ProgTrace(TRACE5, "data[%s] = %s", di->first.c_str(), di->second.StringVal.c_str());
        }
        ProgTrace(TRACE5, "------MAP DUMP------");
}

string PrintDataParser::t_field_map::get_field(string name, int len, string align, string date_format, int field_lat)
{
    string result;
    if(name == "ONE_CHAR")
        result = AlignString("X", len, align);
    if(name == "HUGE_CHAR") result.append(len, 'X');
    if(result.size()) return result;

    if(!(*Qrys.begin())->FieldsCount()) {
        for(TQrys::iterator ti = Qrys.begin(); ti != Qrys.end(); ti++) {
            (*ti)->Execute();
            for(int i = 0; i < (*ti)->FieldsCount(); i++) {
                if(data.find((*ti)->FieldName(i)) != data.end())
                    throw Exception((string)"Duplicate field found " + (*ti)->FieldName(i));
                TTagValue TagValue;
                TagValue.pr_print = 0;
                TagValue.null = (*ti)->FieldIsNULL(i);
                switch((*ti)->FieldType(i)) {
                    case otString:
                    case otChar:
                    case otLong:
                    case otLongRaw:
                        TagValue.type = otString;
                        TagValue.StringVal = (*ti)->FieldAsString(i);
                        break;
                    case otFloat:
                        TagValue.type = otFloat;
                        TagValue.FloatVal = (*ti)->FieldAsFloat(i);
                        break;
                    case otInteger:
                        TagValue.type = otInteger;
                        TagValue.IntegerVal = (*ti)->FieldAsInteger(i);
                        break;
                    case otDate:
                        TagValue.type = otDate;
                        TagValue.DateTimeVal = (*ti)->FieldAsDateTime(i);
                        break;
                }
                data[(*ti)->FieldName(i)] = TagValue;
            }
        }
        // Еще некоторые теги
        {
            TTagValue TagValue;
            TagValue.pr_print = 0;
            TagValue.null = true;
            TagValue.type = otString;

            TagValue.StringVal =
                data["CITY_DEP_NAME"].StringVal.substr(0, 7) +
                "(" + data["AIRP_DEP"].StringVal + ")";
            data["PLACE_DEP"] = TagValue;

            TagValue.StringVal =
                data["CITY_DEP_NAME_LAT"].StringVal.substr(0, 7) +
                "(" + data["AIRP_DEP_LAT"].StringVal + ")";
            data["PLACE_DEP_LAT"] = TagValue;

            TagValue.StringVal =
                data["CITY_DEP_NAME"].StringVal +
                " " + data["AIRP_DEP_NAME"].StringVal;
            data["FULL_PLACE_DEP"] = TagValue;

            TagValue.StringVal =
                data["CITY_DEP_NAME_LAT"].StringVal +
                " " + data["AIRP_DEP_NAME_LAT"].StringVal;
            data["FULL_PLACE_DEP_LAT"] = TagValue;





            TagValue.StringVal =
                data["CITY_ARV_NAME"].StringVal.substr(0, 7) +
                "(" + data["AIRP_ARV"].StringVal + ")";
            data["PLACE_ARV"] = TagValue;

            TagValue.StringVal =
                data["CITY_ARV_NAME_LAT"].StringVal.substr(0, 7) +
                "(" + data["AIRP_ARV_LAT"].StringVal + ")";
            data["PLACE_ARV_LAT"] = TagValue;

            TagValue.StringVal =
                data["CITY_ARV_NAME"].StringVal +
                " " + data["AIRP_ARV_NAME"].StringVal;
            data["FULL_PLACE_ARV"] = TagValue;

            TagValue.StringVal =
                data["CITY_ARV_NAME_LAT"].StringVal +
                " " + data["AIRP_ARV_NAME_LAT"].StringVal;
            data["FULL_PLACE_ARV_LAT"] = TagValue;
        }
    }

    TData::iterator di, di_ru;
    di = data.find(name);
    di_ru = di;
    if(field_lat < 0) field_lat = pr_lat;

    if(field_lat && di != data.end()) {
        TData::iterator di_lat = data.find(name + "_LAT");
        if(di_lat != data.end()) di = di_lat;
    }
    if(di == data.end()) throw Exception("Tag not found " + name);


    if(di != data.end()) {
        if(field_lat && !di_ru->second.null && di->second.null)
            throw Exception("value is empty for " + di->first);
        di_ru->second.pr_print = 1;
        TTagValue TagValue = di->second;
        ostringstream buf;
        buf.width(len);
        switch(TagValue.type) {
            case otString:
            case otChar:
            case otLong:
            case otLongRaw:
                if(di->first == "CLASS")
                        TagValue.StringVal = check_class(TagValue.StringVal);
                buf.fill(' ');
                buf << TagValue.StringVal;
                break;
            case otFloat:
                buf.fill('0');
                buf << TagValue.FloatVal;
                break;
            case otInteger:
                buf.fill('0');
                buf << TagValue.IntegerVal;
                break;
            case otDate:
                TDateTime PrintTime = TagValue.DateTimeVal;
                if(
                        di->first == "BRD_FROM" ||
                        di->first == "BRD_TO" ||
                        di->first == "SCD"
                  ) {
                    PrintTime = UTCToLocal(PrintTime, data.find("TZ_REGION")->second.StringVal);
                }
                buf << DateTimeToStr(PrintTime, date_format, field_lat);
                break;
        }
        if(!len) len = buf.str().size();
        result = AlignString(buf.str(), len, align);
    }
    return result;
}

PrintDataParser::t_field_map::~t_field_map()
{
    OraSession.DeleteQuery(*prnQry);
    for(TQrys::iterator iv = Qrys.begin(); iv != Qrys.end(); ++iv) OraSession.DeleteQuery(**iv);
}

void PrintDataParser::t_field_map::fillBTBPMap()
{
    int grp_id;
    int trip_id;
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   select "
            "      grp_id into :grp_id "
            "   from "
            "      pax "
            "   where "
            "      pax_id = :pax_id;"
            "   select "
            "      point_dep into :point_id "
            "   from "
            "      pax_grp "
            "   where "
            "      grp_id = :grp_id; "
            "end;";
        Qry.DeclareVariable("pax_id", otInteger);
        Qry.DeclareVariable("grp_id", otInteger);
        Qry.DeclareVariable("point_id", otInteger);
        Qry.SetVariable("pax_id", pax_id);
        Qry.Execute();
        grp_id = Qry.GetVariableAsInteger("grp_id");
        trip_id = Qry.GetVariableAsInteger("point_id");
    }

    TQuery *Qry;

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   gtimer.get_stage_time(points.point_id,:brd_open_stage_id) brd_from, "
        "   gtimer.get_stage_time(points.point_id,:brd_close_stage_id) brd_to, "
        "   points.POINT_ID trip_id, "
        "   points.SCD_OUT scd, "
        "   points.EST_OUT est, "
        "   points.ACT_OUT act, "
        "   points.AIRLINE, "
        "   airlines.code_lat airline_lat, "
        "   airlines.name airline_name, "
        "   airlines.name_lat airline_name_lat, "
        "   crafts.code craft, "
        "   crafts.code_lat craft_lat, "
        "   points.BORT, "
        "   system.transliter(points.BORT, 1) bort_lat, "
        "   to_char(points.FLT_NO) flt_no, "
        "   points.SUFFIX, "
        "   system.transliter(points.SUFFIX, 1) suffix_lat, "
        "   system.AirpTZRegion(points.airp) AS tz_region "
        "from "
        "   points, "
        "   airlines, "
        "   crafts "
        "where "
        "   points.point_id = :point_id and "
        "   points.airline = airlines.code and "
        "   points.craft = crafts.code" ;
    Qry->CreateVariable("point_id", otInteger, trip_id);
    Qry->CreateVariable("brd_open_stage_id", otInteger, sOpenBoarding);
    Qry->CreateVariable("brd_close_stage_id", otInteger, sCloseBoarding);
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   pax.PAX_ID, "
        "   pax.SURNAME, "
        "   system.transliter(pax.SURNAME, 1) surname_lat, "
        "   pax.NAME, "
        "   system.transliter(pax.NAME, 1) name_lat, "
        "   pax.surname||' '||pax.name fullname, "
        "   system.transliter(pax.surname||' '||pax.name) fullname_lat, "
        "   pax.pers_type pers_type, "
        "   pers_types.code_lat pers_type_lat, "
        "   pers_types.name pers_type_name, "
        "   LPAD(seat_no,3,'0')|| "
        "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no, "
        "   system.transliter(LPAD(seat_no,3,'0')|| "
        "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'')) AS seat_no_lat, "
        "   pax.SEAT_TYPE, "
        "   system.transliter(pax.SEAT_TYPE, 1) seat_type_lat, "
        "   DECODE( "
        "       pax.SEAT_TYPE, "
        "       'SMSA',1, "
        "       'SMSW',1, "
        "       'SMST',1, "
        "       0) pr_smoke, "
        "   DECODE( "
        "       pax.SEAT_TYPE, "
        "       'SMSA',' ', "
        "       'SMSW',' ', "
        "       'SMST',' ', "
        "       'X') no_smoke, "
        "   DECODE( "
        "       pax.SEAT_TYPE, "
        "       'SMSA','X', "
        "       'SMSW','X', "
        "       'SMST','X', "
        "       ' ') smoke, "
        "   pax.SEATS, "
        "   pax.REG_NO, "
        "   pax.TICKET_NO, "
        "   pax.COUPON_NO, "
        "   decode(pax.coupon_no, null, '', pax.ticket_no||'/'||pax.coupon_no) eticket_no, "
        "   system.transliter(pax.TICKET_NO, 1) ticket_no_lat, "
        "   pax.DOCUMENT, "
        "   system.transliter(pax.DOCUMENT, 1) document_lat, "
        "   pax.SUBCLASS, "
        "   system.transliter(pax.SUBCLASS, 1) subclass_lat, "
        "   ckin.get_birks(pax.grp_id, pax.reg_no, 0) tags, "
        "   ckin.get_birks(pax.grp_id, pax.reg_no, 1) tags_lat, "
        "   ckin.get_pax_pnr_addr(:pax_id) pnr, "
        "   system.transliter(ckin.get_pax_pnr_addr(:pax_id)) pnr_lat "
        "from "
        "   pax, "
        "   pers_types "
        "where "
        "   pax_id = :pax_id and "
        "   pax.pers_type = pers_types.code";
    Qry->CreateVariable("pax_id", otInteger, pax_id);
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "SELECT "
        "   airps.code airp_dep, "
        "   airps.code_lat airp_dep_lat, "
        "   airps.name airp_dep_name, "
        "   airps.name_lat airp_dep_name_lat, "
        "   cities.code city_dep, "
        "   cities.code_lat city_dep_lat, "
        "   cities.name city_dep_name, "
        "   cities.name_lat city_dep_name_lat "
        "FROM "
        "   pax_grp, "
        "   airps, "
        "   cities "
        "WHERE "
        "   pax_grp.grp_id = :grp_id AND "
        "   pax_grp.airp_dep=airps.code AND "
        "   airps.city=cities.code";
    Qry->CreateVariable("grp_id", otInteger, grp_id);
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   pax_grp.airp_arv, "
        "   airps.code_lat airp_arv_lat, "
        "   airps.name airp_arv_name, "
        "   airps.name_lat airp_arv_name_lat, "
        "   cities.code city_arv, "
        "   cities.code_lat city_arv_lat, "
        "   cities.name city_arv_name, "
        "   cities.name_lat city_arv_name_lat, "
        "   pax_grp.class, "
        "   classes.code_lat class_lat, "
        "   classes.name class_name, "
        "   classes.name_lat class_name_lat, "
        "   pax_grp.EXCESS, "
        "   pax_grp.HALL, "
        "   system.transliter(pax_grp.HALL) hall_lat, "
        "   ckin.get_bagAmount(pax_grp.grp_id, null) bag_amount, "
        "   ckin.get_bagWeight(pax_grp.grp_id, null) bag_weight, "
        "   ckin.get_rkWeight(pax_grp.grp_id, null) rk_weight "
        "from "
        "   pax_grp, "
        "   airps, "
        "   classes, "
        "   cities "
        "where "
        "   pax_grp.grp_id = :grp_id and "
        "   pax_grp.airp_arv = airps.code and "
        "   pax_grp.class = classes.code and "
        "   airps.city = cities.code ";
    Qry->CreateVariable("grp_id", otInteger, grp_id);
    Qrys.push_back(Qry);
}

void PrintDataParser::t_field_map::fillMSOMap()
{
    TQuery *Qry;

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        " receipt_id, "
        " no, "
        " form_type, "
        " grp_id, "
        " status, "
        " pr_lat, "
        " tickets, "
        " prev_no, "
        " airline, "
        " aircode, "
        " flt_no, "
        " suffix, "
        " airp_dep, "
        " airp_arv, "
        " ex_amount, "
        " ex_weight, "
        " bag_type, "
        " bag_name, "
        " value_tax, "
        " rate, "
        " rate_cur, "
        " pay_rate, "
        " pay_rate_cur, "
        " pay_type, "
        " pay_doc, "
        " remarks, "
        " issue_date, "
        " issue_place, "
        " issue_user_id, "
        " annul_date, "
        " annul_user_id, "
        " pax, "
        " document "
        "from "
        " bag_receipts "
        "where "
        " receipt_id = :id ";
    Qry->CreateVariable("id", otInteger, pax_id);
    Qrys.push_back(Qry);
    Qry->Execute();
    add_tag("pax", Qry->FieldAsString("pax"));
    add_tag("pax_lat", transliter(Qry->FieldAsString("pax")));
    add_tag("document", Qry->FieldAsString("document"));
    add_tag("document_lat", transliter(Qry->FieldAsString("document")));
    add_tag("issue_date", UTCToLocal(Qry->FieldAsDateTime("issue_date"), TReqInfo::Instance()->desk.tz_region));
    int ex_weight = Qry->FieldAsInteger("ex_weight");
    float rate = Qry->FieldAsFloat("rate");
    float pay_rate = Qry->FieldAsFloat("pay_rate");
    float fare_sum = ex_weight * rate;
    float pay_fare_sum = ex_weight * pay_rate;

    {
        double *iptr, fract;
        fract = modf(fare_sum, iptr);
        string buf = vs_number(int(*iptr), pr_lat);
        if(fract != 0) {
            buf += " " + IntToString(int(fract * 100)) + "/100";
        }
        buf.append(33, '-');
        add_tag("amount_letter", buf);
    }
}

PrintDataParser::t_field_map::t_field_map(int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type)
{
    this->pax_id = pax_id;
    this->pr_lat = pr_lat;
    if(tagsNode) {
        // Положим в мэп теги из клиентского запроса
        xmlNodePtr curNode = tagsNode->children;
        while(curNode) {
            string name = (char *)curNode->name;
            name = upperc(name);
            TTagValue TagValue;
            TagValue.null = false;
            TagValue.type = otString;
            TagValue.StringVal = NodeAsString(curNode);
            if(data.find(name) != data.end())
                throw Exception("Duplicate tag found in client data " + name);
            data[name] = TagValue;
            TagValue.StringVal = transliter(TagValue.StringVal);
            data[name + "_LAT"] = TagValue;
            curNode = curNode->next;
        }
    }

    switch(map_type) {
        case mtBTBP:
            fillBTBPMap();
            break;
        case mtMSO:
            fillMSOMap();
            break;
    }

}

string PrintDataParser::parse_field(int offset, string field)
{
    char Mode = 'S';
    string::size_type VarPos = 0;

    string FieldName = upperc(field);
    int FieldLen = 0;
    string FieldAlign = "L";
    string DateFormat = ServerFormatDateTimeAsString;
    int FieldLat = -1;

    string buf;
    string::size_type i = 0;
    for(; i < field.size(); i++) {
        char curr_char = field[i];
        switch(Mode) {
            case 'S':
                if(!IsLetter(curr_char))
                    throw Exception("first char in tag name must be letter at " + IntToString(offset + i + 1));
                Mode = 'L';
                break;
            case 'L':
                if(!IsDigitIsLetter(curr_char) && curr_char != '_')
                    if(curr_char == '(') {
                        FieldName = upperc(field.substr(0, i));
                        VarPos = i;
                        Mode = '1';
                    } else
                        throw Exception("wrong char in tag name at " + IntToString(offset + i + 1));
                break;
            case '1':
                if(!IsDigit(curr_char)) {
                    if(curr_char == ',') {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf.size()) FieldLen = StrToInt(buf);
                        VarPos = i;
                        Mode = '2';
                    } else
                        throw Exception("1st param must consist of digits only at " + IntToString(offset + i + 1));
                }
                break;
            case '2':
                if(curr_char != 'R' && curr_char != 'L' && curr_char != 'C') {
                    if(curr_char == ',') {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf.size()) FieldAlign = buf;
                        VarPos = i;
                        Mode = '3';
                    } else
                        throw Exception("2nd param must be one of R, L or C at " + IntToString(offset + i + 1));
                }
                break;
            case '3':
                if(i == field.size() - 1 && curr_char == ')') {
                    buf = field.substr(VarPos + 1, i - VarPos - 1);
                    if(buf.size()) {
                        if(buf.size() >= 2 && buf[buf.size() - 2] == ',') { // ,[E|R]
                            char lat_code = buf[buf.size() - 1];
                            switch(lat_code) {
                                case 'E':
                                    FieldLat = 1;
                                    break;
                                case 'R':
                                    FieldLat = 0;
                                    break;
                                default:
                                    throw Exception("4th param must be one of R or E at " + IntToString(offset + i + 1));
                            }
                            buf = buf.substr(0, buf.size() - 3);
                        }
                        DateFormat = buf;
                    }
                    Mode = 'F';
                }
                break;
        }
    }
    if(Mode != 'L' && Mode != 'F')
            throw Exception("')' not found at " + IntToString(offset + i + 1));
    return field_map.get_field(FieldName, FieldLen, FieldAlign, DateFormat, FieldLat);
}

string PrintDataParser::parse(string &form)
{
    string result;
    char Mode = 'S';
    string::size_type VarPos = 0;
    string::size_type i = 0;
    for(; i < form.size(); i++) {
        switch(Mode) {
            case 'S':
                if(form[i] == '[') {
                    VarPos = i + 1;
                    Mode = 'Q';
                } else
                    result += form[i];
                break;
            case 'Q':
                if(form[i] == ']')
                    Mode = 'R';
                else if(form[i] == '[')
                    Mode = 'L';
                break;
            case 'R':
                if(form[i] == ']')
                    Mode = 'Q';
                else {
                    --i;
                    Mode = 'S';
                    result += parse_tag(VarPos, form.substr(VarPos, i - VarPos));
                }
                break;
            case 'L':
                if(form[i] == '[')
                    Mode = 'Q';
                else
                    throw Exception("2nd '[' not found at " + IntToString(i + 1));
                break;
        }
    }

    if(Mode != 'S')
        throw Exception("']' not found at " + IntToString(i + 1));
    return result;
}

string PrintDataParser::parse_tag(int offset, string tag)
{
    u_int slash_point = 0;
    bool slash_found;
    {
        char Mode = 'S';
        for(; slash_point < tag.size(); ++slash_point) {
            switch(Mode) {
                case 'S':
                    if(tag[slash_point] == '/')
                        Mode = 'F';
                    break;
                case 'F':
                    if(tag[slash_point] == '/')
                        Mode = 'S';
                    else
                        Mode = 'L';
                    break;
            }
            if(Mode == 'L') break;
        }
        slash_found = (slash_point != tag.size() || Mode == 'F');
        if(slash_found) --slash_point;
    }

    u_int start_point, end_point;

    if(slash_found) {
        if(pr_lat) {
            start_point = slash_point + 1;
            end_point = tag.size();
        } else {
            start_point = 0;
            end_point = slash_point;
        }
    } else {
        start_point = 0;
        end_point = tag.size();
    }

    string result;
    char Mode = 'S';
    string::size_type VarPos = 0;
    string::size_type i = start_point;
    for(; i < end_point; i++) {
        switch(Mode) {
            case 'S':
                if(tag[i] == '>')
                    Mode = 'R';
                else if(tag[i] == '<')
                    Mode = 'Q';
                else if(tag[i] == '[')
                    Mode = 'M';
                else if(tag[i] == ']')
                    Mode = 'N';
                else if(tag[i] == '/')
                    Mode = 'O';
                else
                    result += tag[i];
                break;
            case 'Q':
                if(tag[i] == '<') {
                    result += '<';
                    Mode = 'S';
                } else {
                    VarPos = i;
                    Mode = 'L';
                }
                break;
            case 'L':
                if(tag[i] == '>') {
                    result += parse_field(i, tag.substr(VarPos, i - VarPos));
                    Mode = 'S';
                }
                break;
            case 'M':
                result += '[';
                Mode = 'S';
                break;
            case 'N':
                result += ']';
                Mode = 'S';
                break;
            case 'O':
                if(tag[i] == '/') {
                    result += '/';
                    Mode = 'S';
                } else
                    throw Exception("2nd '/' not found at " + IntToString(offset + i + 1));
                break;
            case 'R':
                if(tag[i] == '>') {
                    result += '>';
                    Mode = 'S';
                } else
                    throw Exception("2nd '>' not found at " + IntToString(offset + i + 1));
                break;
        }
    }
    if(Mode != 'S')
        throw Exception("'>' not found at " + IntToString(offset + i + 1));
    return result;
}

//////////////////////////////// END CLASS PrintDataParser ///////////////////////////////////

void GetPrintData(int grp_id, int prn_type, string &Pectab, string &Print)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select point_dep AS point_id, class from pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    int trip_id = Qry.FieldAsInteger("point_id");
    string cl = Qry.FieldAsString("class");
    Qry.Clear();
    Qry.SQLText =
        "SELECT bp_type FROM trip_bp "
        "WHERE point_id=:point_id AND (class IS NULL OR class=:class) "
        "ORDER BY class ";
    Qry.CreateVariable("point_id", otInteger, trip_id);
    Qry.CreateVariable("class", otString, cl);
    Qry.Execute();
    if(Qry.Eof) throw UserException("На рейс или класс не назначен бланк посадочных талонов");
    string bp_type = Qry.FieldAsString("bp_type");

    Qry.Clear();
    Qry.SQLText =
     "SELECT form,data FROM bp_forms "
     "WHERE bp_type=:bp_type AND prn_type=:prn_type";
    Qry.CreateVariable("bp_type", otString, bp_type);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();

    if(Qry.Eof||Qry.FieldIsNULL("data"))
      throw UserException("Печать пос. талона на выбранный принтер не производится");

    Pectab = Qry.FieldAsString("form");
    Print = Qry.FieldAsString("data");
}

void GetPrintDataBP(xmlNodePtr dataNode, int pax_id, int prn_type, int pr_lat, xmlNodePtr clientDataNode)
{
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    TQuery Qry(&OraSession);
    Qry.SQLText = "select grp_id from pax where pax_id = :pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    string Pectab, Print;
    GetPrintData(Qry.FieldAsInteger("grp_id"), prn_type, Pectab, Print);
    NewTextChild(BPNode, "pectab", Pectab);
    PrintDataParser parser(pax_id, pr_lat, clientDataNode);
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    xmlNodePtr paxNode = NewTextChild(passengersNode,"pax");
    NewTextChild(paxNode, "prn_form", parser.parse(Print));
    {
        TQuery *Qry = parser.get_prn_qry();
        TDateTime time_print = NowUTC();
        Qry->CreateVariable("now_utc", otDate, time_print);
        ProgTrace(TRACE5, "PRN QUERY: %s", Qry->SQLText.SQLText());
        Qry->Execute();
        SetProp(paxNode, "pax_id", pax_id);
        SetProp(paxNode, "time_print", DateTimeToStr(time_print));
    }
}

void GetPrintDataBP(xmlNodePtr dataNode, int grp_id, int prn_type, int pr_lat, bool pr_all, xmlNodePtr clientDataNode)
{
    string Pectab, Print;
    GetPrintData(grp_id, prn_type, Pectab, Print);
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    NewTextChild(BPNode, "pectab", Pectab);
    TQuery Qry(&OraSession);
    if(pr_all)
        Qry.SQLText =
            "select pax_id from pax where grp_id = :grp_id and refuse is null order by reg_no";
    else
        Qry.SQLText =
            "select pax.pax_id from "
            "   pax, bp_print "
            "where "
            "   pax.grp_id = :grp_id and "
            "   pax.refuse is null and "
            "   pax.pax_id = bp_print.pax_id(+) and "
            "   bp_print.pr_print(+) <> 0 and "
            "   bp_print.pax_id IS NULL "
            "order by "
            "   pax.reg_no";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    while(!Qry.Eof) {
        int pax_id = Qry.FieldAsInteger("pax_id");
        PrintDataParser parser(pax_id, pr_lat, clientDataNode);
        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        NewTextChild(paxNode, "prn_form", parser.parse(Print));
        {
            TQuery *Qry = parser.get_prn_qry();
            TDateTime time_print = NowUTC();
            Qry->CreateVariable("now_utc", otDate, time_print);
            Qry->Execute();
            SetProp(paxNode, "pax_id", pax_id);
            SetProp(paxNode, "time_print", DateTimeToStr(time_print));
        }
        Qry.Next();
    }
}

void PrintInterface::GetGRPPrintDataBPXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    GetPrintDataBP(
            dataNode,
            NodeAsInteger("grp_id", reqNode),
            NodeAsInteger("prn_type", reqNode),
            NodeAsInteger("pr_lat", reqNode),
            NodeAsInteger("pr_all", reqNode),
            NodeAsNode("clientData", reqNode)
            );
    ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
}

void PrintInterface::GetPrintDataBPXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    GetPrintDataBP(
            dataNode,
            NodeAsInteger("pax_id", reqNode),
            NodeAsInteger("prn_type", reqNode),
            NodeAsInteger("pr_lat", reqNode),
            NodeAsNode("clientData", reqNode)
            );
    ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
}

void get_bt_forms(string tag_type, int prn_type, xmlNodePtr pectabsNode, vector<string> &prn_forms)
{
    prn_forms.clear();
    TQuery FormsQry(&OraSession);
    FormsQry.SQLText =
      "SELECT num, form, data FROM bt_forms "
      "WHERE tag_type = :tag_type AND prn_type=:prn_type "
      "ORDER BY num";
    FormsQry.CreateVariable("tag_type", otString, tag_type);
    FormsQry.CreateVariable("prn_type", otInteger, prn_type);
    FormsQry.Execute();
    if(FormsQry.Eof)
      throw UserException("Печать баг. бирки %s на выбранный принтер не производится",tag_type.c_str());
    while(!FormsQry.Eof)
    {
        NewTextChild(pectabsNode, "pectab", FormsQry.FieldAsString("form"));
        if (FormsQry.FieldIsNULL("data"))
          throw UserException("Печать баг. бирки %s на выбранный принтер не производится",tag_type.c_str());
        prn_forms.push_back(FormsQry.FieldAsString("data"));
        FormsQry.Next();
    };
}

struct TBTRouteItem {
    string airline, airline_lat;
    int flt_no;
    string airp_arv, airp_arv_lat;
    int local_date;
    string fltdate, fltdate_lat;
    string airp_arv_name, airp_arv_name_lat;
};

void DumpRoute(vector<TBTRouteItem> &route)
{
    ProgTrace(TRACE5, "-----------DUMP ROUTE-----------");
    for(vector<TBTRouteItem>::iterator iv = route.begin(); iv != route.end(); ++iv) {
        ProgTrace(TRACE5, "airline: %s", iv->airline.c_str());
        ProgTrace(TRACE5, "airline_lat: %s", iv->airline_lat.c_str());
        ProgTrace(TRACE5, "flt_no: %d", iv->flt_no);
        ProgTrace(TRACE5, "airp_arv: %s", iv->airp_arv.c_str());
        ProgTrace(TRACE5, "airp_arv_lat: %s", iv->airp_arv_lat.c_str());
        ProgTrace(TRACE5, "local_date: %d", iv->local_date);
        ProgTrace(TRACE5, "fltdate: %s", iv->fltdate.c_str());
        ProgTrace(TRACE5, "fltdate_lat: %s", iv->fltdate_lat.c_str());
        ProgTrace(TRACE5, "airp_arv_name: %s", iv->airp_arv_name.c_str());
        ProgTrace(TRACE5, "airp_arv_name_lat: %s", iv->airp_arv_name_lat.c_str());
        ProgTrace(TRACE5, "-----------RouteItem-----------");
    }
}

void get_route(int grp_id, vector<TBTRouteItem> &route)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  "
        "   points.scd_out scd,  "
        "   null local_date,  "
        "   points.airline,  "
        "   airlines.code_lat airline_lat,  "
        "   points.flt_no,  "
        "   pax_grp.airp_arv,  "
        "   airps.code_lat airp_arv_lat, "
        "   airps.name airp_arv_name, "
        "   airps.name_lat airp_arv_name_lat, "
        "   0 transfer_num "
        "from  "
        "   pax_grp,  "
        "   points,  "
        "   airlines,  "
        "   airps  "
        "where  "
        "   pax_grp.grp_id = :grp_id and  "
        "   pax_grp.point_dep = points.point_id and  "
        "   points.airline = airlines.code and  "
        "   pax_grp.airp_arv = airps.code "
        "union  "
        "select  "
        "   null,  "
        "   transfer.local_date,  "
        "   transfer.airline,  "
        "   airlines.code_lat airline_lat,  "
        "   transfer.flt_no,  "
        "   transfer.airp_arv,  "
        "   airps.code_lat airp_arv_lat,  "
        "   airps.name airp_arv_name, "
        "   airps.name_lat airp_arv_name_lat, "
        "   transfer.transfer_num "
        "from  "
        "   transfer,  "
        "   airlines,  "
        "   airps  "
        "where  "
        "   transfer.grp_id = :grp_id and  "
        "   (transfer.airline = airlines.code or "
        "   transfer.airline = airlines.code_lat) and  "
        "   (transfer.airp_arv = airps.code or "
        "   transfer.airp_arv = airps.code_lat) "
        "order by "
        "   transfer_num ";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    int Year, Month, Day;
    while(!Qry.Eof) {
        TBTRouteItem RouteItem;
        RouteItem.airline = Qry.FieldAsString("airline");
        RouteItem.airline_lat = Qry.FieldAsString("airline_lat");
        RouteItem.flt_no = Qry.FieldAsInteger("flt_no");
        RouteItem.airp_arv = Qry.FieldAsString("airp_arv");
        RouteItem.airp_arv_lat = Qry.FieldAsString("airp_arv_lat");
        RouteItem.airp_arv_name = Qry.FieldAsString("airp_arv_name");
        RouteItem.airp_arv_name_lat = Qry.FieldAsString("airp_arv_name_lat");
        if(Qry.FieldIsNULL("local_date")) {
            DecodeDate(Qry.FieldAsDateTime("scd"), Year, Month, Day);
            RouteItem.local_date = Day;
        } else
            RouteItem.local_date = Qry.FieldAsInteger("local_date");
        if(RouteItem.local_date < Day) {
            if(Month == 12)
                Month = 1;
            else
                ++Month;
        }
        RouteItem.fltdate = IntToString(RouteItem.local_date) + ShortMonthNames[Month];
        RouteItem.fltdate_lat = IntToString(RouteItem.local_date) + ShortMonthNamesLat[Month];
        route.push_back(RouteItem);
        Day = RouteItem.local_date;
        Qry.Next();
    }
    DumpRoute(route);
}

void set_via_fields(PrintDataParser &parser, vector<TBTRouteItem> &route, int start_idx, int end_idx)
{
    int via_idx = 1;
    for(int j = start_idx; j < end_idx; ++j) {
        string str_via_idx = IntToString(via_idx);
        parser.add_tag("flt_no" + str_via_idx, route[j].flt_no);
        parser.add_tag("local_date" + str_via_idx, route[j].local_date);
        parser.add_tag("airline" + str_via_idx, route[j].airline);
        parser.add_tag("airline" + str_via_idx + "_lat", route[j].airline_lat);
        parser.add_tag("airp_arv" + str_via_idx, route[j].airp_arv);
        parser.add_tag("airp_arv" + str_via_idx + "_lat", route[j].airp_arv_lat);
        parser.add_tag("fltdate" + str_via_idx, route[j].fltdate);
        parser.add_tag("fltdate" + str_via_idx + "_lat", route[j].fltdate_lat);
        parser.add_tag("airp_arv_name" + str_via_idx, route[j].airp_arv_name);
        parser.add_tag("airp_arv_name" + str_via_idx + "_lat", route[j].airp_arv_name_lat);
        ++via_idx;
    }
}

struct TTagKey {
    int grp_id, prn_type, pr_lat;
    double no; //no = Float!
    string type, color;
    TTagKey(): grp_id(0), prn_type(0), pr_lat(0), no(-1.0) {};
};

void GetPrintDataBT(xmlNodePtr dataNode, const TTagKey &tag_key)
{
    vector<TBTRouteItem> route;
    get_route(tag_key.grp_id, route);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   pax_id "
        "from "
        "   pax "
        "where "
        "   grp_id = :grp_id and "
        "   refuse is null "
        "order by "
        "   decode(seats, 0, 1, 0), "
        "   decode(pers_type, 'ВЗ', 0, 'РБ', 1, 2), "
        "   reg_no ";
    Qry.CreateVariable("GRP_ID", otInteger, tag_key.grp_id);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    int pax_id = Qry.FieldAsInteger(0);
    string SQLText =
        "SELECT "
        "   bag_tags.no, "
        "   bag_tags.color, "
        "   bag_tags.tag_type, "
        "   to_char(bag2.amount) bag_amount, "
        "   to_char(bag2.weight) bag_weight "
        "FROM "
        "   bag_tags, "
        "   tag_types, "
        "   bag2 "
        "WHERE "
        "   bag_tags.tag_type=tag_types.code AND "
        "   bag_tags.grp_id = :grp_id AND "
        "   bag_tags.grp_id = bag2.grp_id(+) and "
        "   bag_tags.bag_num = bag2.num(+) and ";
    if(tag_key.no >= 0.0) {
        SQLText +=
            "   bag_tags.tag_type = :tag_type and "
            "   nvl(bag_tags.color, ' ') = nvl(:color, ' ') and "
            "   bag_tags.no = :no and ";
        Qry.CreateVariable("tag_type", otString, tag_key.type);
        Qry.CreateVariable("color", otString, tag_key.color);
        Qry.CreateVariable("no", otFloat, tag_key.no);
    } else
        SQLText +=
            "   bag_tags.pr_print = 0 AND ";
    SQLText +=
        "   tag_types.printable <> 0"
        "ORDER BY "
        "   bag_tags.tag_type, "
        "   bag_tags.num";
    Qry.SQLText = SQLText;
    Qry.Execute();
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    if (Qry.Eof) return;
    string tag_type;
    vector<string> prn_forms;
    xmlNodePtr printBTNode = NewTextChild(dataNode, "printBT");
    xmlNodePtr pectabsNode = NewTextChild(printBTNode, "pectabs");
    xmlNodePtr tagsNode = NewTextChild(printBTNode, "tags");

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = UTCToLocal(NowUTC(),reqInfo->desk.tz_region);

    while(!Qry.Eof) {
        string tmp_tag_type = Qry.FieldAsString("tag_type");
        if(tag_type != tmp_tag_type) {
            tag_type = tmp_tag_type;
            get_bt_forms(tag_type, tag_key.prn_type, pectabsNode, prn_forms);
        }

        u_int64_t tag_no = (u_int64_t)Qry.FieldAsFloat("no");
        int aircode = tag_no / 1000000;
        int no = tag_no % 1000000;

        xmlNodePtr tagNode = NewTextChild(tagsNode, "tag");
        SetProp(tagNode, "color", Qry.FieldAsString("color"));
        SetProp(tagNode, "type", tag_type);
        SetProp(tagNode, "no", tag_no);

        PrintDataParser parser(pax_id, tag_key.pr_lat, NULL);

        parser.add_tag("aircode", aircode);
        parser.add_tag("no", no);
        parser.add_tag("issued", issued);
        parser.add_tag("bt_amount", Qry.FieldAsString("bag_amount"));
        parser.add_tag("bt_weight", Qry.FieldAsString("bag_weight"));

        int VIA_num = prn_forms.size();
        int route_size = route.size();
        int BT_count = route_size / VIA_num;
        int BT_reminder = route_size % VIA_num;

        for(int i = 0; i < BT_count; ++i) {
            set_via_fields(parser, route, i * VIA_num, (i + 1) * VIA_num);
            NewTextChild(tagNode, "prn_form", parser.parse(prn_forms.back()));
        }

        if(BT_reminder) {
            set_via_fields(parser, route, route_size - BT_reminder, route_size);
            NewTextChild(tagNode, "prn_form", parser.parse(prn_forms[BT_reminder - 1]));
        }
        Qry.Next();
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
}

void PrintInterface::ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    TTagKey tag_key;
    tag_key.grp_id = NodeAsInteger("grp_id", reqNode);
    tag_key.prn_type = NodeAsInteger("prn_type", reqNode);
    tag_key.pr_lat = NodeAsInteger("pr_lat", reqNode);
    tag_key.type = NodeAsString("type", reqNode);
    tag_key.color = NodeAsString("color", reqNode);
    tag_key.no = NodeAsFloat("no", reqNode);
    GetPrintDataBT(dataNode, tag_key);
}

void PrintInterface::GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    TTagKey tag_key;
    tag_key.grp_id = NodeAsInteger("grp_id", reqNode);
    tag_key.prn_type = NodeAsInteger("prn_type", reqNode);
    tag_key.pr_lat = NodeAsInteger("pr_lat", reqNode);
    GetPrintDataBT(dataNode, tag_key);
}

void PrintInterface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery PaxQry(&OraSession);
    PaxQry.SQLText =
      "SELECT pax_id FROM pax "
      "WHERE pax_id=:pax_id AND tid=:tid ";
    PaxQry.DeclareVariable("pax_id",otInteger);
    PaxQry.DeclareVariable("tid",otInteger);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "update bp_print set pr_print = 1 where pax_id = :pax_id and time_print = :time_print and pr_print = 0";
    Qry.DeclareVariable("pax_id", otInteger);
    Qry.DeclareVariable("time_print", otDate);
    xmlNodePtr curNode = NodeAsNode("passengers/pax", reqNode);
    while(curNode) {
        PaxQry.SetVariable("pax_id", NodeAsInteger("@pax_id", curNode));
        PaxQry.SetVariable("tid", NodeAsInteger("@tid", curNode));
        PaxQry.Execute();
        if(PaxQry.Eof)
            throw UserException("Изменения по пассажиру производились с другой стойки. Обновите данные");
        Qry.SetVariable("pax_id", NodeAsInteger("@pax_id", curNode));
        Qry.SetVariable("time_print", NodeAsDateTime("@time_print", curNode));
        Qry.Execute();
        if (Qry.RowsProcessed()==0)
            throw UserException("Изменения по пассажиру производились с другой стойки. Обновите данные");
        curNode = curNode->next;
    }
}

void PrintInterface::ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "update bag_tags set pr_print = 1 where tag_type = :type and no = :no and "
        "   (color is null and :color is null or color = :color)";
    Qry.DeclareVariable("type", otString);
    Qry.DeclareVariable("no", otFloat);
    Qry.DeclareVariable("color", otString);
    xmlNodePtr curNode = NodeAsNode("tags/tag", reqNode);
    while(curNode) {
        Qry.SetVariable("type", NodeAsString("@type", curNode));
        Qry.SetVariable("no", NodeAsFloat("@no", curNode));
        Qry.SetVariable("color", NodeAsString("@color", curNode));
        Qry.Execute();
        if (Qry.RowsProcessed()==0)
            throw UserException("Изменения по багажу производились с другой стойки. Обновите данные");
        curNode = curNode->next;
    }
}

void PrintInterface::GetPrinterList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

    TDocType doc = DecodeDocType(NodeAsString("doc_type", reqNode));

    TQuery Qry(&OraSession);
    xmlNodePtr printersNode = NewTextChild(resNode, "printers");
    /*
    Qry.SQLText =
        "select 1 from desks, prn_drv where "
        "   desks.code = :desk and "
        "   desks.grp_id = prn_drv.desk_grp and "
        "   prn_drv.doc_type = :doc ";
    TReqInfo *reqInfo = TReqInfo::Instance();
    Qry.CreateVariable("desk", otString, reqInfo->desk.code);
    Qry.CreateVariable("doc", otInteger, doc);
    Qry.Execute();
    if(!Qry.Eof) {
        NewTextChild(printersNode, "drv");
        return;
    }

    Qry.Clear();
    */

    string table;

    switch(doc) {
        case dtBP:
            table = "bp_forms ";
            break;
        case dtBT:
            table = "bt_forms ";
            break;
        case dtReceipt:
        case dtFltDoc:
        case dtArchive:
        case dtDisp:
        case dtTlg:
            NewTextChild(printersNode, "drv");
            return;
        default:
            throw Exception("Unknown DocType " + IntToString(doc));
    }

    string SQLText =
        "select "
        "   prn_types.code, "
        "   prn_types.name, "
        "   prn_types.iface, "
        "   prn_formats.id format_id, "
        "   prn_formats.code format "
        "from "
        "   prn_types, "
        "   prn_formats "
        "where "
        "   prn_types.code in ( "
        "       select distinct prn_type from " + table +
        "   ) and "
        "   prn_types.format = prn_formats.id(+) "
        "order by "
        "   prn_types.name ";
    Qry.SQLText = SQLText;
    Qry.Execute();
    if(Qry.Eof) throw UserException("Принтеры не найдены");
    while(!Qry.Eof) {
        xmlNodePtr printerNode = NewTextChild(printersNode, "printer");
        NewTextChild(printerNode, "code", Qry.FieldAsInteger("code"));
        NewTextChild(printerNode, "name", Qry.FieldAsString("name"));
        NewTextChild(printerNode, "iface", Qry.FieldAsString("iface"));
        NewTextChild(printerNode, "format_id", Qry.FieldAsInteger("format_id"));
        NewTextChild(printerNode, "format", Qry.FieldAsString("format"));
        Qry.Next();
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

namespace to_esc {
    typedef struct {
        int x, y;
        string data;
    } TField;
    typedef vector<TField> TFields;

    bool lt(const TField &p1, const TField &p2)
    {
        return (p1.y == p2.y ? p1.x < p2.x : p1.y < p2.y);
    }

    void dump(string data, string fname)
    {
        ofstream out(fname.c_str());
        if(!out.good())
            throw Exception("dump: cannot open file '%s' for output", fname.c_str());
        out << data;
    }

    void convert(string &mso_form)
    {
        char Mode = 'S';
        TFields fields;
        string num;
        int x, y;
        TField field;
        for(string::iterator si = mso_form.begin(); si != mso_form.end(); si++) {
            char curr_char = *si;
            switch(Mode) {
                case 'S':
                    if(IsDigit(curr_char)) {
                        num += curr_char;
                        Mode = 'X';
                    } else
                        throw Exception("to_esc: x must start from digit");
                    break;
                case 'X':
                    if(IsDigit(curr_char))
                        num += curr_char;
                    else if(curr_char == ',') {
                        x = StrToInt(num);
                        num.erase();
                        Mode = 'Y';
                    } else
                        throw Exception("to_esc: x must be num");
                    break;
                case 'Y':
                    if(IsDigit(curr_char))
                        num += curr_char;
                    else if(curr_char == ',') {
                        y = StrToInt(num);
                        num.erase();
                        Mode = 'A';
                    } else
                        throw Exception("to_esc: y must be num");
                    break;
                case 'A':
                    if(curr_char == 10) {
                        field.x = x;
                        field.y = y;
                        field.data = num;
                        fields.push_back(field);
                        num.erase();
                        Mode = 'S';
                    } else
                        num += curr_char;
            }
        }
        sort(fields.begin(), fields.end(), lt);
        mso_form = "\x1b@\x1bM\x1bj#";
        int curr_y = 0;
        for(TFields::iterator fi = fields.begin(); fi != fields.end(); fi++) {
            int delta_y = fi->y - curr_y;
            if(delta_y) {
                int offset_y = delta_y * 7;
                int y256 = offset_y / 256;
                int y_reminder = offset_y % 256;

                for(int i = 0; i < y256; i++) {
                    mso_form += "\x1bJ\xff";
                }
                mso_form += "\x1bJ";
                if(y_reminder) mso_form += char(y_reminder - 1);
                curr_y = fi->y;
            }


            int offset_x = int(fi->x * 7.1);
            int x256 = offset_x / 256;
            int x_reminder = offset_x % 256;

            mso_form += "\x1b$";
            mso_form += (char)0;
            mso_form += (char)0;
            for(int i = 0; i < x256; i++) {
                mso_form += "\x1b\\\xff";
                mso_form += (char)0;
            }
            if(x_reminder) {
                mso_form += (string)"\x1b\\" + (char)(x_reminder - 1);
                mso_form += (char)0;
            }
            mso_form += fi->data;
        }
        mso_form += "\x0c\x1b@";
    }
}

void PrintInterface::GetPrintDataBR(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    PrintDataParser parser(
            NodeAsInteger("id", reqNode),
            NodeAsInteger("pr_lat", reqNode),
            NULL,
            PrintDataParser::mtMSO
            );
    int br_id = 0;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select form from br_forms where id = :id";
    Qry.CreateVariable("id", otInteger, br_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("Receipt form not found for id " + IntToString(br_id));
    string mso_form = Qry.FieldAsString("form");
    mso_form = parser.parse(mso_form);
    to_esc::convert(mso_form);
    NewTextChild(resNode, "form", b64_encode(mso_form.c_str(), mso_form.size()));
}

void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
