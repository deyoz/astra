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
#include "base_tables.h"
#include "stl_utils.h"
#include "payment.h"
#include "exceptions.h"
#include <fstream>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const string delim = "\xb";

typedef enum {pfBTP, pfATB, pfEPL2, pfEPSON} TPrnFormat;
typedef enum {
    ptIER506A = 1,
    ptIER508A,
    ptIER506B,
    ptIER508B,
    ptIER557A,
    ptIER567A,
    ptGenicom,
    ptDRV,
    ptIER508BR,
    ptOKIML390,
    ptOKIML3310
} TPrnType;

namespace to_esc {
    struct TPrnParams {
        string encoding;
        int offset, top;
        void get_prn_params(xmlNodePtr prnParamsNode);
    };

    void TPrnParams::get_prn_params(xmlNodePtr reqNode)
    {
        encoding = "CP866";
        offset = 20;
        top = 0;
        if(reqNode) {
            xmlNodePtr prnParamsNode = GetNode("prnParams", reqNode);
            if(prnParamsNode) {
                encoding = NodeAsString("encoding", prnParamsNode);
                offset = NodeAsInteger("offset", prnParamsNode);
                top = NodeAsInteger("top", prnParamsNode);
            } else {
                encoding = NodeAsString("encoding", reqNode);
            }
        }
    }

    typedef struct {
        int x, y, font;
        int len, height;
        string align;
        string data;
        void parse_data();
    } TField;

    void TField::parse_data()
    {
        size_t si = data.find(delim);
        if(si == string::npos) {
            len = 0;
            height = 1;
            align = "L";
        } else {
            for(int i = 0; i < 3; i++) {
                string buf = data.substr(0, si);
                data.erase(0, si + 1);
                si = data.find(delim);
                switch(i) {
                    case 0:
                        len = StrToInt(buf);
                        break;
                    case 1:
                        height = StrToInt(buf);
                        break;
                    case 2:
                        align = buf;
                        break;
                }
            }
        }
    };

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

    void convert(string &mso_form, TPrnType prn_type, xmlNodePtr reqNode = NULL)
    {
        double y_modif, x_modif;
        switch(prn_type) {
            case ptOKIML390:
                x_modif = 7;
                y_modif = 7.1;
                break;
            case ptOKIML3310:
                x_modif = 4.67;
                y_modif = 8.66;
                break;
            default:
                throw Exception("to_esc::convert: unknown prn_type " + IntToString(prn_type));
        }
        char Mode = 'S';
        TFields fields;
        string num;
        int x, y, font, prnParamsOffset, prnParamsTop;

        TPrnParams prnParams;
        prnParams.get_prn_params(reqNode);

        try {
            mso_form = ConvertCodePage(prnParams.encoding, "CP866", mso_form);
        } catch(EConvertError &E) {
            ProgError(STDLOG, E.what());
            throw UserException("Ошибка конвертации в %s", prnParams.encoding.c_str());
        }

        prnParamsOffset = 20 - prnParams.offset;
        prnParamsTop = prnParams.top;
        if(prnParamsOffset < 0)
            prnParamsOffset = 0;
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
                        Mode = 'B';
                    } else
                        throw Exception("to_esc: y must be num");
                    break;
                case 'A':
                    if(curr_char == 10) {
                        field.x = x + prnParamsOffset;
                        field.y = y + prnParamsTop;
                        field.font = font;
                        field.data = num;
                        field.parse_data();
                        if(field.font == 'B' && field.data.size() != 10)
                            throw Exception("barcode data len must be 10");
                        fields.push_back(field);
                        num.erase();
                        Mode = 'S';
                    } else
                        num += curr_char;
                    break;
                case 'B':
                    if(IsDigit(curr_char) || curr_char == 'B')
                        num += curr_char;
                    else if(curr_char == ',') {
                        if(num.size() != 1) throw Exception("font fild must by 1 char");
                        font = num[0];
                        num.erase();
                        Mode = 'A';
                    } else
                        throw Exception("to_esc: font must be num or 'B'");
                    break;
            }
        }
        {
            TFields tmp_fields;
            for(TFields::iterator fi = fields.begin(); fi != fields.end(); fi++) {
                if(fi->height > 1) {
                    vector<string> strs;
                    SeparateString(fi->data.c_str(), fi->len, strs);
                    int height = fi->height;
                    int y = 0;
                    for(vector<string>::iterator iv = strs.begin(); iv != strs.end() && height != 0; iv++, height--) {
                        if(iv == strs.begin()) {
                            fi->data = AlignString(*iv, fi->len, fi->align);
                            fi->height = 1;
                            y = fi->y;
                        } else {
                            TField fbuf;
                            fbuf.x = fi->x;

                            switch(fi->font) {
                                case '0':
                                    y += 3;
                                    break;
                                case '1':
                                    y += 3;
                                    break;
                                case '2':
                                    y += 6;
                                    break;
                                default:
                                    throw Exception("convert: unknown font for multiple rows field: %c", fi->font);
                                    break;
                            }
                            fbuf.y = y;

                            fbuf.font = fi->font;
                            fbuf.len = fi->len;
                            fbuf.height = 1;
                            fbuf.align = fi->align;
                            fbuf.data = AlignString(*iv, fi->len, fi->align);
                            tmp_fields.push_back(fbuf);
                        }
                    }
                }
            }
            for(TFields::iterator fi = tmp_fields.begin(); fi != tmp_fields.end(); fi++) {
                fields.push_back(*fi);
            }
        }
        sort(fields.begin(), fields.end(), lt);
        mso_form = "\x1b@\x1bM\x1bj#";
        int curr_y = 0;
        for(TFields::iterator fi = fields.begin(); fi != fields.end(); fi++) {
            int delta_y = fi->y - curr_y;
            if(delta_y) {
                int offset_y = int(delta_y * y_modif);
                int y256 = offset_y / 256;
                int y_reminder = offset_y % 256;

                for(int i = 0; i < y256; i++) {
                    mso_form += "\x1bJ\xff";
                }
                mso_form += "\x1bJ";
                if(y_reminder) mso_form += char(y_reminder - 1);
                curr_y = fi->y;
            }


            int offset_x = int(fi->x * x_modif);
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
            if(fi->font == 'B') {
                mso_form += "\x1b""\x10""A""\x08""\x03";
                mso_form += (char)0;
                mso_form += (char)0;
                mso_form += "\x05""\x01""\x01""\x01";
                mso_form += (char)0;
                mso_form += "\x1b""\x10""B""\x0b";
                mso_form += (char)0;
            }
            if(fi->font == '1') mso_form += "\x1b\x0f";
            if(fi->font == '2') mso_form += "\x1bw\x01\x1bW\x01";
            mso_form += fi->data;
            if(fi->font == '1') mso_form += "\x12";
            if(fi->font == '2') {
                mso_form += "\x1bw";
                mso_form += (char)0;
                mso_form += "\x1bW";
                mso_form += (char)0;
            }
        }
        mso_form += "\x0c\x1b@";
    }
}

bool PrintDataParser::t_field_map::printed(TData::iterator di)
{
    return di != data.end() && di->second.pr_print;
}

TQuery *PrintDataParser::t_field_map::get_prn_qry()
{
    dump_data();
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
        prnQry->SetVariable(di1->first, StrToInt(di1->second.StringVal));


    di1 = data.find("BAG_AMOUNT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, StrToInt(di1->second.StringVal));


    di1 = data.find("BAG_WEIGHT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, StrToInt(di1->second.StringVal));


    di1 = data.find("RK_WEIGHT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, StrToInt(di1->second.StringVal));


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
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, StrToInt(di1->second.StringVal));


    di1 = data.find("SURNAME");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("SUFFIX");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);

    return prnQry;
}

string PrintDataParser::t_field_map::GetTagAsString(string name)
{
    TData::iterator di = data.find(upperc(name));
    if(di == data.end()) throw Exception("Tag not found " + name);
    return di->second.StringVal;
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
            switch(di->second.type) {
                case otInteger:
                    ProgTrace(TRACE5, "data[%s] = %d", di->first.c_str(), di->second.IntegerVal);
                    break;
                case otFloat:
                    ProgTrace(TRACE5, "data[%s] = %f", di->first.c_str(), di->second.FloatVal);
                    break;
                case otString:
                    ProgTrace(TRACE5, "data[%s] = %s", di->first.c_str(), di->second.StringVal.c_str());
                    break;
                case otDate:
                    ProgTrace(TRACE5, "data[%s] = %s", di->first.c_str(),
                            DateTimeToStr(di->second.DateTimeVal, "dd.mm.yyyy hh:nn:ss").c_str());
                    break;
                default:
                    ProgTrace(TRACE5, "data[%s] = %s", di->first.c_str(), di->second.StringVal.c_str());
                    break;
            }
        }
        ProgTrace(TRACE5, "------MAP DUMP------");
}

string PrintDataParser::t_field_map::get_field(string name, int len, string align, string date_format, int field_lat)
{
    tst();
    ProgTrace(TRACE5, "TAG: %s", name.c_str());
    string result;
    if(name == "ONE_CHAR" || print_mode == 2) {
        if(len)
            result = AlignString("8", len, align);
        else
            result = "8";
    }
    if(name == "HUGE_CHAR") result.append(len, '8');
    if(result.size()) return result;

    if(Qrys.size() && !(*Qrys.begin())->FieldsCount()) {
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

            string test_server;
            if(get_test_server())
                for(int i = 0; i < 150; i++)
                    test_server += "ТЕСТ ";
            add_tag("test_server", test_server);
            test_server = transliter(test_server, 1);
            add_tag("test_server_lat", test_server);
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
                    PrintTime = UTCToLocal(PrintTime,
                            AirpTZRegion(data.find("AIRP_DEP")->second.StringVal));
                }
                buf << DateTimeToStr(PrintTime, date_format, field_lat);
                break;
        }
        if(!len) len = buf.str().size();
        result = AlignString(buf.str(), len, align);
        if(print_mode == 1) {
            size_t result_size = result.size();
            result = "";
            result.append(result_size, '8');
        }
        if(print_mode == 3) {
            size_t result_size = result.size();
            result = "";
            result.append(result_size, ' ');
        }
    }
    {
        string buf = result;
        TrimString(buf);
        if(buf.empty() && print_mode != 3) {
            if(name == "GATE") throw UserException("Не указан выход на посадку");
        }
    }
    tst();
    return result;
}

PrintDataParser::t_field_map::~t_field_map()
{
    OraSession.DeleteQuery(*prnQry);
    for(TQrys::iterator iv = Qrys.begin(); iv != Qrys.end(); ++iv) OraSession.DeleteQuery(**iv);
}

void PrintDataParser::t_field_map::fillBTBPMap()
{
    ProgTrace(TRACE5, "fillBTBPMap: grp_id = %d", grp_id);
    int trip_id;
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "   select "
            "      point_dep "
            "   from "
            "      pax_grp "
            "   where "
            "      grp_id = :grp_id";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        trip_id = Qry.FieldAsInteger("point_dep");
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
        "   nvl(airlines.short_name, airlines.name) airline_short, "
        "   nvl(airlines.short_name_lat, airlines.name_lat) airline_short_lat, "
        "   crafts.code craft, "
        "   crafts.code_lat craft_lat, "
        "   points.BORT, "
        "   system.transliter(points.BORT, 1) bort_lat, "
        "   to_char(points.FLT_NO)||points.suffix flt_no, "
        "   points.SUFFIX, "
        "   tlg.convert_suffix(points.SUFFIX, 1) suffix_lat "
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
    if(pax_id == NoExists) {
        Qry->SQLText =
            "select "
            "   '' NAME, "
            "   '' name_lat, "
            "   '' pers_type, "
            "   '' pers_type_lat, "
            "   '' pers_type_name, "
            "   '' seat_no, "
            "   '' seat_no_lat, "
            "   '' SEAT_TYPE, "
            "   '' seat_type_lat, "
            "   '' pr_smoke, "
            "   '' no_smoke, "
            "   '' smoke, "
            "   '' SEATS, "
            "   '' REG_NO, "
            "   '' TICKET_NO, "
            "   '' COUPON_NO, "
            "   '' eticket_no, "
            "   '' ticket_no_lat, "
            "   '' DOCUMENT, "
            "   '' document_lat, "
            "   '' SUBCLASS, "
            "   '' subclass_lat, "
            "   ckin.get_birks(:grp_id, NULL, 0) tags, "
            "   ckin.get_birks(:grp_id, NULL, 1) tags_lat, "
            "   '' pnr, "
            "   '' pnr_lat "
            "from "
            "   dual ";
        Qry->CreateVariable("grp_id", otInteger, grp_id);
    } else {
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
            "   ckin.get_seat_no(pax.pax_id, 'voland') str_seat_no, "
            "   tlg.convert_seat_no(ckin.get_seat_no(pax.pax_id, 'voland'), 1) str_seat_no_lat, "
            "   LPAD(seat_no,3,'0')|| "
            "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') AS seat_no, "
            "   tlg.convert_seat_no(LPAD(seat_no,3,'0')|| "
            "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),''), 1) AS seat_no_lat, "
            "   pax.SEAT_TYPE, "
            "   system.transliter(pax.SEAT_TYPE, 1) seat_type_lat, "
            "   to_char(DECODE( "
            "       pax.SEAT_TYPE, "
            "       'SMSA',1, "
            "       'SMSW',1, "
            "       'SMST',1, "
            "       0)) pr_smoke, "
            "   to_char(DECODE( "
            "       pax.SEAT_TYPE, "
            "       'SMSA',' ', "
            "       'SMSW',' ', "
            "       'SMST',' ', "
            "       'X')) no_smoke, "
            "   to_char(DECODE( "
            "       pax.SEAT_TYPE, "
            "       'SMSA','X', "
            "       'SMSW','X', "
            "       'SMST','X', "
            "       ' ')) smoke, "
            "   pax.SEATS, "
            "   pax.REG_NO, "
            "   pax.TICKET_NO, "
            "   pax.COUPON_NO, "
            "   decode(pax.coupon_no, null, '', pax.ticket_no||'/'||pax.coupon_no) eticket_no, "
            "   decode(pax.coupon_no, null, '', 'ETKT'||pax.ticket_no||'/'||pax.coupon_no) etkt, "
            "   system.transliter(pax.TICKET_NO, 1) ticket_no_lat, "
            "   pax.DOCUMENT, "
            "   system.transliter(pax.DOCUMENT, 1) document_lat, "
            "   pax.SUBCLASS, "
            "   system.transliter(pax.SUBCLASS, 1) subclass_lat, "
            "   ckin.get_birks(pax.grp_id, pax.pax_id, 0) tags, "
            "   ckin.get_birks(pax.grp_id, pax.pax_id, 1) tags_lat, "
            "   ckin.get_pax_pnr_addr(:pax_id) pnr, "
            "   tlg.convert_pnr_addr(ckin.get_pax_pnr_addr(:pax_id), 1) pnr_lat "
            "from "
            "   pax, "
            "   pers_types "
            "where "
            "   pax_id = :pax_id and "
            "   pax.pers_type = pers_types.code";
        Qry->CreateVariable("pax_id", otInteger, pax_id);
    }
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
        "   cls_grp.code class, "
        "   cls_grp.code_lat class_lat, "
        "   cls_grp.name class_name, "
        "   cls_grp.name_lat class_name_lat, "
        "   pax_grp.EXCESS, "
        "   pax_grp.HALL, "
        "   system.transliter(pax_grp.HALL) hall_lat, "
        "   to_char(ckin.get_bagAmount(pax_grp.grp_id, null)) bag_amount, "
        "   to_char(ckin.get_bagWeight(pax_grp.grp_id, null)) bag_weight, "
        "   to_char(ckin.get_rkWeight(pax_grp.grp_id, null)) rk_weight "
        "from "
        "   pax_grp, "
        "   airps, "
        "   cls_grp, "
        "   cities "
        "where "
        "   pax_grp.grp_id = :grp_id and "
        "   pax_grp.airp_arv = airps.code and "
        "   pax_grp.class_grp = cls_grp.id(+) and "
        "   airps.city = cities.code ";
    Qry->CreateVariable("grp_id", otInteger, grp_id);
    Qrys.push_back(Qry);
}

void get_mso_point(const string &airp, string &point, string &point_lat)
{
    TBaseTable &airps = base_tables.get("airps");
    TBaseTable &cities = base_tables.get("cities");



    string city = airps.get_row("code", airp).AsString("city");
    point = cities.get_row("code", city).AsString("name", 0);
    if(point.empty()) throw UserException("Не определено название города '" + city + "'");
    point_lat = cities.get_row("code", city).AsString("name", 1);
    if(point_lat.empty()) throw UserException("Не определено лат. название города '" + city + "'");

    TQuery airpsQry(&OraSession);
    airpsQry.SQLText =  "select count(*) from airps where city = :city";
    airpsQry.CreateVariable("city", otString, base_tables.get("airps").get_row("code", airp).AsString("city"));
    airpsQry.Execute();
    if(!airpsQry.Eof && airpsQry.FieldAsInteger(0) != 1) {
        point += "(" + airps.get_row("code", airp).AsString("code", 0) + ")";
        point_lat += "(" + airps.get_row("code", airp).AsString("code", 1) + ")";
    }
}

string pieces(int ex_amount, bool pr_lat)
{
    string result;
    if(pr_lat) {
        return "pieces";
    }
    else {
        return "мест";
    };
}

int get_exch_precision(double rate)
{
    double iptr;
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
    return precision;
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
    else {
        double iptr;
        if(modf(rate, &iptr) < 0.01)
            precision = 0;
        else
            precision = 2;
    }
    return precision;
}

int get_value_tax_precision(double tax)
{
  return 1;
};

string ExchToString(int rate1, string rate_cur1, double rate2, string rate_cur2, bool pr_lat)
{
    ostringstream buf;
    buf
        << rate1
        << base_tables.get("currency").get_row("code", rate_cur1).AsString("code", pr_lat)
        << "="
        << fixed
        << setprecision(get_exch_precision(rate2))
        << rate2
        << base_tables.get("currency").get_row("code", rate_cur2).AsString("code", pr_lat);
    return buf.str();
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
      buf << base_tables.get("currency").get_row("code", rate_cur).AsString("code", pr_lat);
    if (fmt_type!=2 && pr_lat)
      buf << setprecision(get_rate_precision(rate, rate_cur)) << fixed << rate;
    return buf.str();
}

void PrintDataParser::t_field_map::fillMSOMap(TBagReceipt &rcpt)
{
    if(
            rcpt.form_type != "M61" &&
            rcpt.form_type != "Z61" &&
            rcpt.form_type != "451" &&
            rcpt.form_type != "35"
            )
        throw UserException("Тип бланка '" + rcpt.form_type + "' временно не поддерживается системой");
  add_tag("pax_name",rcpt.pax_name);
  add_tag("pax_doc",rcpt.pax_doc);

  TQuery Qry(&OraSession);
  Qry.SQLText =  "select name, name_lat from rcpt_service_types where code = :code";
  Qry.CreateVariable("code", otInteger, rcpt.service_type);
  Qry.Execute();
  if(Qry.Eof) throw Exception("fillMSOMap: service_type not found (code = %d)", rcpt.service_type);
  add_tag("service_type", (string)"10 " + Qry.FieldAsString("name"));
  add_tag("service_type_lat", (string)"10 " + Qry.FieldAsString("name_lat"));
  string bag_name, bag_name_lat;
  if(rcpt.service_type == 2 && rcpt.bag_type != -1) {
      Qry.Clear();
      Qry.SQLText =
          "select "
          "  nvl(rcpt_bag_names.name, bag_types.name) name, "
          "  nvl(rcpt_bag_names.name_lat, bag_types.name) name_lat "
          "from "
          "  bag_types, "
          "  rcpt_bag_names "
          "where "
          "  bag_types.code = :code and "
          "  bag_types.code = rcpt_bag_names.code(+)";
      Qry.CreateVariable("code", otInteger, rcpt.bag_type);
      Qry.Execute();
      if(Qry.Eof) throw Exception("fillMSOMap: bag_type not found (code = %d)", rcpt.bag_type);
      bag_name = Qry.FieldAsString("name");
      bag_name_lat = Qry.FieldAsString("name_lat");
      if(rcpt.bag_type == 1 || rcpt.bag_type == 2) {
          bag_name += " " + IntToString(rcpt.ex_amount) + " " + pieces(rcpt.ex_amount, 0);
          bag_name_lat += " " + IntToString(rcpt.ex_amount) + " " + pieces(rcpt.ex_amount, 1);
      }
      upperc(bag_name);
      upperc(bag_name_lat);
      add_tag("bag_name", bag_name);
      add_tag("bag_name_lat", bag_name_lat);
  } else {
      add_tag("bag_name", "");
      add_tag("bag_name_lat", "");
  }

  string
      SkiBT,
      GolfBT,
      PetBT,
      BulkyBT, BulkyBTLetter,
      OtherBT, OtherBTLetter,  OtherBTLetter_lat,
      ValueBT, ValueBTLetter, ValueBTLetter_lat;
  if(rcpt.service_type == 3) {
      ValueBT = "x";

      {
          double iptr, fract;
          fract = modf(rcpt.rate, &iptr);
          string buf_ru = vs_number(int(iptr), 0);
          string buf_lat = vs_number(int(iptr), 1);
          if(fract >= 0.01) {
              string str_fract = " " + IntToString(int(fract * 100)) + "/100";
              buf_ru += str_fract;
              buf_lat += str_fract;
          }
          ValueBTLetter = upperc(buf_ru) +
              base_tables.get("currency").get_row("code", rcpt.rate_cur).AsString("code", 0);
          ValueBTLetter_lat = upperc(buf_lat) +
              base_tables.get("currency").get_row("code", rcpt.rate_cur).AsString("code", 1);
      }
  }
  add_tag("ValueBT", ValueBT);
  add_tag("ValueBTLetter", ValueBTLetter);
  add_tag("ValueBTLetter_lat", ValueBTLetter_lat);
  if(rcpt.bag_type != -1) {
      if(
              rcpt.form_type == "451" ||
              rcpt.form_type == "35"
              ) {
          switch(rcpt.bag_type) {
              case 20:
                  SkiBT = "x";
                  break;
              case 21:
                  GolfBT = "x";
                  break;
              case 4:
                  PetBT = "x";
                  break;
              case 1:
              case 2:
                  BulkyBT = "x";
                  BulkyBTLetter = IntToString(rcpt.ex_amount);
                  break;
              default:
                  OtherBT = "x";
                  OtherBTLetter = bag_name;
                  OtherBTLetter_lat = bag_name_lat;
                  break;
          }
      } else if(rcpt.form_type == "Z61") {
          switch(rcpt.bag_type) {
              case 20:
                  SkiBT = "x";
                  break;
              case 4:
                  PetBT = "x";
                  break;
              case 1:
              case 2:
                  BulkyBT = "x";
                  BulkyBTLetter = IntToString(rcpt.ex_amount);
                  break;
              default:
                  OtherBT = "x";
                  OtherBTLetter = bag_name;
                  OtherBTLetter_lat = bag_name_lat;
                  break;
          }
      }
  }
  add_tag("SkiBT", SkiBT);
  add_tag("GolfBT", GolfBT);
  add_tag("PetBT", PetBT);
  add_tag("BulkyBT", BulkyBT);
  add_tag("BulkyBTLetter", BulkyBTLetter);
  add_tag("OtherBT", OtherBT);
  add_tag("OtherBTLetter", OtherBTLetter);
  add_tag("OtherBTLetter_lat", OtherBTLetter_lat);

  double pay_rate;
  if (rcpt.pay_rate_cur != rcpt.rate_cur)
    pay_rate = (rcpt.rate * rcpt.exch_pay_rate)/rcpt.exch_rate;
  else
    pay_rate = rcpt.rate;
  double rate_sum;
  double pay_rate_sum;
  if(rcpt.service_type == 1 || rcpt.service_type == 2) {
      rate_sum = rcpt.rate * rcpt.ex_weight;
      pay_rate_sum = pay_rate * rcpt.ex_weight;
  } else {
      rate_sum = rcpt.rate * rcpt.value_tax/100;
      pay_rate_sum = pay_rate * rcpt.value_tax/100;
  }


  ostringstream remarks, remarks_lat, rate, rate_lat, ex_weight, ex_weight_lat;

  if(rcpt.service_type == 1 || rcpt.service_type == 2) {
      remarks << "ТАРИФ ЗА КГ=";
      remarks_lat << "RATE PER KG=";
      if(
             (rcpt.pay_rate_cur == "РУБ" ||
              rcpt.pay_rate_cur == "ДОЛ" ||
              rcpt.pay_rate_cur == "ЕВР") &&
              rcpt.pay_rate_cur != rcpt.rate_cur
        ) {
          remarks
              << RateToString(pay_rate, rcpt.pay_rate_cur, false, 0)
              << "(" << RateToString(rcpt.rate, rcpt.rate_cur, false, 0) << ")"
              << "(" << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, false)
              << ")";
          remarks_lat
              << RateToString(pay_rate, rcpt.pay_rate_cur, true, 0)
              << "(" << RateToString(rcpt.rate, rcpt.rate_cur, true, 0) << ")"
              << "(RATE " << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, true)
              << ")";
      } else {
          rate << RateToString(rcpt.rate, rcpt.rate_cur, false, 0);
          rate_lat << RateToString(rcpt.rate, rcpt.rate_cur, true, 0);
          remarks << rate.str();
          remarks_lat << rate_lat.str();
      }
      ex_weight << IntToString(rcpt.ex_weight) << "КГ";
      ex_weight_lat << IntToString(rcpt.ex_weight) << "KG";
  } else {
      //багаж с объявленной ценностью
      rate
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax <<"%";
      rate_lat
          << rate.str();
      remarks
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax << "% OT "
            << RateToString(rcpt.rate, rcpt.rate_cur, false, 0);
      remarks_lat
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax << "% OF "
            << RateToString(rcpt.rate, rcpt.rate_cur, true, 0);
      add_tag("remarks2", "");
      add_tag("remarks2_lat", "");
  }
  add_tag("rate", rate.str());
  add_tag("rate_lat", rate_lat.str());
  add_tag("remarks1", remarks.str());
  add_tag("remarks1_lat", remarks_lat.str());
  add_tag("remarks2", ex_weight.str());
  add_tag("remarks2_lat", ex_weight_lat.str());
  add_tag("ex_weight", ex_weight.str());
  add_tag("ex_weight_lat", ex_weight_lat.str());

  for(int fmt=1;fmt<=2;fmt++)
  {
    remarks.str("");
    remarks_lat.str("");
    if(
           (rcpt.pay_rate_cur == "РУБ" ||
            rcpt.pay_rate_cur == "ДОЛ" ||
            rcpt.pay_rate_cur == "ЕВР") &&
            rcpt.pay_rate_cur != rcpt.rate_cur
      ) {
        remarks
            << RateToString(pay_rate_sum, rcpt.pay_rate_cur, false, fmt)
            << "(" << RateToString(rate_sum, rcpt.rate_cur, false, fmt) << ")";
        remarks_lat
            << RateToString(pay_rate_sum, rcpt.pay_rate_cur, true, fmt)
            << "(" << RateToString(rate_sum, rcpt.rate_cur, true, fmt) << ")";
    } else {
        remarks << RateToString(rate_sum, rcpt.rate_cur, false, fmt);
        remarks_lat << RateToString(rate_sum, rcpt.rate_cur, true, fmt);
    };
    if (fmt==1)
    {
      add_tag("amount_figures", remarks.str());
      add_tag("amount_figures_lat", remarks_lat.str());
    }
    else
    {
      add_tag("currency", remarks.str());
      add_tag("currency_lat", remarks_lat.str());
    };
  };

  double fare_sum;
  if(
         (rcpt.pay_rate_cur == "РУБ" ||
          rcpt.pay_rate_cur == "ДОЛ" ||
          rcpt.pay_rate_cur == "ЕВР") &&
          rcpt.pay_rate_cur != rcpt.rate_cur
    )
    fare_sum=pay_rate_sum;
  else
    fare_sum=rate_sum;

  //amount in figures
  {
      double iptr, fract;
      fract = modf(fare_sum, &iptr);
      string buf_ru = vs_number(int(iptr), 0);
      string buf_lat = vs_number(int(iptr), 1);
      if(fract >= 0.01) {
          string str_fract = " " + IntToString(int(fract * 100)) + "/100";
          buf_ru += str_fract;
          buf_lat += str_fract;
      }
      add_tag("amount_letters", upperc(buf_ru));
      add_tag("amount_letters_lat", upperc(buf_lat));
  }

  //exchange_rate
  remarks.str("");
  remarks_lat.str("");
  if (!(
         (rcpt.pay_rate_cur == "РУБ" ||
          rcpt.pay_rate_cur == "ДОЛ" ||
          rcpt.pay_rate_cur == "ЕВР") &&
          rcpt.pay_rate_cur != rcpt.rate_cur)
     )
  {
      if (rcpt.pay_rate_cur != rcpt.rate_cur)
      {
          remarks
              << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, false);
          remarks_lat
              << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, true);
      };
  };
  add_tag("exchange_rate", remarks.str());
  add_tag("exchange_rate_lat", remarks_lat.str());

  //total
  add_tag("total", RateToString(pay_rate_sum, rcpt.pay_rate_cur, false, 0));
  add_tag("total_lat", RateToString(pay_rate_sum, rcpt.pay_rate_cur, true, 0));

  add_tag("tickets", rcpt.tickets);
  add_tag("prev_no", rcpt.prev_no);
  add_tag("pay_form", rcpt.pay_form);

  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", rcpt.airline);
  if(!airline.short_name.empty())
      add_tag("airline", airline.short_name);
  else if(!airline.name.empty())
      add_tag("airline", airline.name);
  else
      throw UserException("Не определено название а/к '%s'", rcpt.airline.c_str());

  if(!airline.short_name_lat.empty())
      add_tag("airline_lat", airline.short_name_lat);
  else if(!airline.name_lat.empty())
      add_tag("airline_lat", airline.name_lat);
  else
      throw UserException("Не определено лат. название а/к '%s'", rcpt.airline.c_str());

  add_tag("aircode", rcpt.aircode);

  {
      string point_dep, point_dep_lat;
      string point_arv, point_arv_lat;
      ostringstream airline_code, airline_code_lat;

      get_mso_point(rcpt.airp_dep, point_dep, point_dep_lat);
      get_mso_point(rcpt.airp_arv, point_arv, point_arv_lat);

      if(airline.code_lat.empty())
          throw UserException("Не определен лат. код а/к '%s'", rcpt.airline.c_str());
      ostringstream buf;
      buf
          << point_dep << "-" << point_arv << " ";

      airline_code << airline.code;
      if(rcpt.flt_no != -1)
          airline_code
              << " "
              << setw(3) << setfill('0') << rcpt.flt_no << convert_suffix(rcpt.suffix, false);
      buf << airline_code.str();
      add_tag("airline_code", airline_code.str());
      add_tag("to", buf.str());
      buf.str("");
      buf
          << point_dep_lat << "-" << point_arv_lat << " ";
      airline_code_lat << airline.code_lat;
      if(rcpt.flt_no != -1)
          airline_code_lat
              << " "
              << setw(3) << setfill('0') << rcpt.flt_no << convert_suffix(rcpt.suffix, true);
      buf << airline_code_lat.str();
      add_tag("airline_code_lat", airline_code_lat.str());
      add_tag("to_lat", buf.str());

      add_tag("point_dep", point_dep);
      add_tag("point_dep_lat", point_dep_lat);
      add_tag("point_arv", point_arv);
      add_tag("point_arv_lat", point_arv_lat);
  }

  Qry.Clear();
  Qry.SQLText =
      "select desk_grp.city from "
      "  desk_grp, "
      "  desks "
      "where "
      "  desks.code = :code and "
      "  desks.grp_id = desk_grp.grp_id ";
  Qry.CreateVariable("code", otString, rcpt.issue_desk);
  Qry.Execute();
  if(Qry.Eof)
      throw Exception("fillMSOMap: issue_desk not found (code = %s)", rcpt.issue_desk.c_str());
  TDateTime issue_date_local = UTCToLocal(rcpt.issue_date, CityTZRegion(Qry.FieldAsString("city")));
  add_tag("issue_date", issue_date_local);
  add_tag("issue_date_str", DateTimeToStr(issue_date_local, (string)"ddmmmyy", 0));
  add_tag("issue_date_str_lat", DateTimeToStr(issue_date_local, (string)"ddmmmyy", 1));

  {
      string buf = rcpt.issue_place;
      ProgTrace(TRACE5, "BUFFER: %s", buf.c_str());
      int line_num = 1;
      while(line_num <= 5) {
          string::size_type i = buf.find('\n');
          string issue_place = buf.substr(0, i);
          buf.erase(0, i + 1);
//          if(buf.empty() && line_num < 5)
//              throw Exception("fillMSOMap: Not enough lines in buffer\n");
          add_tag("issue_place" + IntToString(line_num), issue_place);
          line_num++;
      }
  }


  dump_data();
};

/*
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
        расчетный код а/к " issue_user_id, "
        " annul_date, "
        " annul_user_id, "
        " pax " //, "
//        " document "
        "from "
        " bag_receipts "
        "where "
        " receipt_id = :id ";
    Qry->CreateVariable("id", otInteger, pax_id);
    Qrys.push_back(Qry);
    Qry->Execute();
    add_tag("pax", Qry->FieldAsString("pax"));
    add_tag("pax_lat", transliter(Qry->FieldAsString("pax"),true));
//    add_tag("document", Qry->FieldAsString("document"));
//    add_tag("document_lat", transliter(Qry->FieldAsString("document"),true));
    add_tag("issue_date", UTCToLocal(Qry->FieldAsDateTime("issue_date"), TReqInfo::Instance()->desk.tz_region));
    int ex_weight = Qry->FieldAsInteger("ex_weight");
    float rate = Qry->FieldAsFloat("rate");
    float pay_rate = Qry->FieldAsFloat("pay_rate");
    float fare_sum = ex_weight * rate;
    float pay_fare_sum = ex_weight * pay_rate;
    string pay_curr = Qry->FieldAsString("pay_rate_cur");
    string curr = Qry->FieldAsString("rate_cur");

    {
        double iptr, fract;
        fract = modf(fare_sum, &iptr);
        string buf_ru = vs_number(int(iptr), 0);
        string buf_lat = vs_number(int(iptr), 1);
        if(fract != 0) {
            string str_fract = " " + IntToString(int(fract * 100)) + "/100";
            buf_ru += str_fract;
            buf_lat += str_fract;
        }
        buf_ru.append(33, '-');
        buf_lat.append(33, '-');
        add_tag("amount_letter", buf_ru);
        add_tag("amount_letter_lat", buf_lat);
    }

    {
        ostringstream buf;
        buf
            << setprecision(0)
            << fixed
            << fare_sum;
        if(pay_curr != curr)
            buf
                << "("
                << setprecision(2)
                << pay_fare_sum << ")";
        add_tag("amount_figure", buf.str());
    }

    TBaseTable &currency = base_tables.get("currency");
    {
        string buf, buf_lat;
        TBaseTableRow &curr_row = currency.get_row("code", curr);
        buf = curr_row.AsString("code", 0);
        buf_lat = curr_row.AsString("code", 1);
        if(curr != pay_curr) {
            TBaseTableRow &pay_curr_row = currency.get_row("code", pay_curr);
            buf += "(" + pay_curr_row.AsString("code", 0) + ")";
            buf_lat += "(" + pay_curr_row.AsString("code", 1) + ")";
        }
        add_tag("currency", buf);
        add_tag("currency_lat", buf_lat);
    }

    {
        ostringstream buf;
        buf
            << setprecision(2)
            << fixed
            << fare_sum;
        add_tag("total", buf.str() + currency.get_row("code", curr).AsString("code", 0));
        add_tag("total_lat", currency.get_row("code", curr).AsString("code", 1) + buf.str());
    }

    {
        string airp_dep = Qry->FieldAsString("airp_dep");
        string airp_arv = Qry->FieldAsString("airp_arv");

        string point_dep, point_dep_lat;
        string point_arv, point_arv_lat;

        get_mso_point(airp_dep, point_dep, point_dep_lat);
        get_mso_point(airp_arv, point_arv, point_arv_lat);

        add_tag("to", point_dep + "-" + point_arv);
        add_tag("to_lat", point_dep_lat + "-" + point_arv_lat);
    }
    add_tag("tickets", Qry->FieldAsString("tickets"));
    {
        ostringstream buf, buf_lat;
        buf
            << setprecision(2)
            << fixed
            << rate
            << curr;
        buf_lat
            << currency.get_row("code", curr).AsString("code", 1)
            << setprecision(2)
            << fixed
            << rate;
        if(pay_curr != curr) {
            buf
                << "("
                << pay_rate
                << pay_curr
                << ")";
            buf_lat
                << "("
                << currency.get_row("code", pay_curr).AsString("code", 1)
                << pay_rate
                << ")";
        }
        add_tag("rate", buf.str());
        add_tag("rate_lat", buf_lat.str());
    }
    add_tag("ex_weight", Qry->FieldAsString("ex_weight"));
    TBaseTableRow &pt_row = base_tables.get("pay_types").get_row("code", Qry->FieldAsString("pay_type"));
    add_tag("pay_type", pt_row.AsString("code"));
    add_tag("pay_type_lat", pt_row.AsString("code", 1));

    TBaseTableRow &airline = base_tables.get("airlines").get_row("code", Qry->FieldAsString("airline"));
    add_tag("airline", airline.AsString("short_name"));
    add_tag("airline_lat", airline.AsString("short_name", 1));
    add_tag("aircode", Qry->FieldAsString("aircode"));

    vector<string> validator;
    get_validator(validator);

    add_tag("agency", validator[0]);
    add_tag("agency_descr",validator[1]);
    add_tag("agency_city",validator[2]);
    add_tag("agency_code",validator[3]);
}*/

PrintDataParser::t_field_map::t_field_map(TBagReceipt &rcpt)
{
    print_mode = 0;
    this->pr_lat = rcpt.pr_lat;
    fillMSOMap(rcpt);
}

PrintDataParser::t_field_map::t_field_map(int grp_id, int pax_id, int pr_lat, xmlNodePtr tagsNode, TMapType map_type)
{
    print_mode = 0;
    this->grp_id = grp_id;
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
            TagValue.StringVal = transliter(TagValue.StringVal,true);
            data[name + "_LAT"] = TagValue;
            curNode = curNode->next;
        }
    }

    switch(map_type) {
        case mtBTBP:
            fillBTBPMap();
            break;
    }

}

string PrintDataParser::parse_field(int offset, string field)
{
    string result;
    switch(pectab_format) {
        case 0:
            result = parse_field0(offset, field);
            break;
        case 1:
            result = parse_field1(offset, field);
            break;
    }
    return result;
}

bool PrintDataParser::IsDelim(char curr_char, char &Mode)
{
    bool result = true;
    switch(curr_char) {
        case 'C':
        case 'D':
        case 'F':
        case 'H':
        case 'L':
            Mode = curr_char;
            break;
        case ')':
            Mode = 'F';
            break;
        default:
            result = false;
            break;
    }
    return result;
}

string PrintDataParser::parse_field1(int offset, string field)
{
    char Mode = 'S';
    string::size_type VarPos = 0;

    string FieldName = upperc(field);
    string FieldLen = "0";
    string FieldHeight = "1";
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
                Mode = 'N';
                break;
            case 'N':
                if(!IsDigitIsLetter(curr_char) && curr_char != '_')
                    if(curr_char == '(') {
                        FieldName = upperc(field.substr(0, i));
                        VarPos = i;
                        Mode = '1';
                    } else
                        throw Exception("wrong char in tag name at " + IntToString(offset + i + 1));
                break;
            case '1':
                if(IsDelim(curr_char, Mode))
                    VarPos = i;
                else
                    throw Exception("first char in tag params  must be name of param at " + IntToString(offset + i + 1));
                break;
            case 'L':
                if(!IsDigit(curr_char)) {
                    if(IsDelim(curr_char, Mode)) {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf.size()) FieldLen = buf;
                        VarPos = i;
                    } else
                        throw Exception("L param must consist of digits only at " + IntToString(offset + i + 1));
                }
                break;
            case 'H':
                if(!IsDigit(curr_char)) {
                    if(IsDelim(curr_char, Mode)) {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf.size()) FieldHeight = buf;
                        VarPos = i;
                    } else
                        throw Exception("H param must consist of digits only at " + IntToString(offset + i + 1));
                }
                break;
            case 'C':
                if(curr_char != 'r' && curr_char != 'l' && curr_char != 'c') {
                    if(IsDelim(curr_char, Mode)) {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf.size()) FieldAlign = upperc(buf);
                        VarPos = i;
                    } else
                        throw Exception("C param must be one of r, l or c at " + IntToString(offset + i + 1));
                }
                break;
            case 'D':
                if(IsDelim(curr_char, Mode)) {
                    buf = field.substr(VarPos + 1, i - VarPos - 1);
                    DateFormat = buf;
                    VarPos = i;
                }
                break;
            case 'F':
                if(curr_char != 'r' && curr_char != 'e') {
                    if(IsDelim(curr_char, Mode)) {
                        buf = field.substr(VarPos + 1, i - VarPos - 1);
                        if(buf == "e")
                            FieldLat = 1;
                        else
                            FieldLat = 0;
                        VarPos = i;
                    } else
                        throw Exception("D param must be one of r or e at " + IntToString(offset + i + 1));
                }
                break;
        }
    }
    if(Mode != 'N' && Mode != 'F')
        throw Exception("')' not found at " + IntToString(offset + i + 1));
    string result;
    if(FieldHeight != "1")
        result =
            FieldLen +
            delim +
            FieldHeight +
            delim +
            FieldAlign +
            delim +
            field_map.get_field(FieldName, 0, "L", DateFormat, FieldLat);
    else
        result =
            field_map.get_field(FieldName, StrToInt(FieldLen), FieldAlign, DateFormat, FieldLat);
    return result;
}



string PrintDataParser::parse_field0(int offset, string field)
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
    if(form.substr(0, 2) == "1\xa") {
        i = 2;
        pectab_format = 1;
    }
    if(form.substr(0, 2) == "XX") {
        i = 2;
        field_map.print_mode = 1;
    } else if(form.substr(0, 1) == "X") {
        i = 1;
        field_map.print_mode = 2;
    } else if(form.substr(0, 1) == "S") {
        i = 1;
        field_map.print_mode = 3;
    }
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

void GetTripBPPectabs(int point_id, int prn_type, xmlNodePtr node)
{
    if (node==NULL) return;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
            "select  "
            "   bp_forms.form "
            "from  "
            "   bp_forms, "
            "   ( "
            "    select "
            "        bp_type, "
            "        prn_type, "
            "        max(version) version "
            "    from "
            "        bp_forms "
            "    group by "
            "        bp_type, "
            "        prn_type "
            "   ) a "
            "where  "
            "   a.bp_type IN (SELECT DISTINCT bp_type FROM trip_bp WHERE point_id=:point_id) and "
            "   a.prn_type = :prn_type and "
            "   a.bp_type = bp_forms.bp_type and "
            "   a.prn_type = bp_forms.prn_type and "
            "   a.version = bp_forms.version and "
            "   bp_forms.form IS NOT NULL ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bp_forms");
    for(;!Qry.Eof;Qry.Next())
      NewTextChild(formNode,"form",Qry.FieldAsString("form"));
};

void GetTripBTPectabs(int point_id, int prn_type, xmlNodePtr node)
{
    if (node==NULL) return;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "select  "
        "   bt_forms.form  "
        "from  "
        "   bt_forms, "
        "   ( "
        "    select "
        "        tag_type, "
        "        prn_type, "
        "        num, "
        "        max(version) version "
        "    from "
        "        bt_forms "
        "    group by "
        "        tag_type, "
        "        prn_type, "
        "        num "
        "   ) a "
        "where  "
        "   a.tag_type IN (SELECT DISTINCT tag_type FROM trip_bt WHERE point_id=:point_id) and "
        "   a.prn_type = :prn_type and "
        "   a.tag_type = bt_forms.tag_type and "
        "   a.prn_type = bt_forms.prn_type and "
        "   a.num = bt_forms.num and "
        "   a.version = bt_forms.version and "
        "   bt_forms.form IS NOT NULL ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bt_forms");
    for(;!Qry.Eof;Qry.Next())
      NewTextChild(formNode,"form",Qry.FieldAsString("form"));

};

void GetPrintData(int grp_id, int prn_type, string &Pectab, string &Print)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select format from prn_types where code = :prn_type";
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    if(Qry.Eof) throw Exception("Unknown prn_type: " + IntToString(prn_type));
    TPrnFormat  prn_format = (TPrnFormat)Qry.FieldAsInteger("format");
    Qry.Clear();
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
            "select  "
            "   bp_forms.form,  "
            "   bp_forms.data  "
            "from  "
            "   bp_forms, "
            "   ( "
            "    select "
            "        bp_type, "
            "        prn_type, "
            "        max(version) version "
            "    from "
            "        bp_forms "
            "    group by "
            "        bp_type, "
            "        prn_type "
            "   ) a "
            "where  "
            "   a.bp_type = :bp_type and "
            "   a.prn_type = :prn_type and "
            "   a.bp_type = bp_forms.bp_type and "
            "   a.prn_type = bp_forms.prn_type and "
            "   a.version = bp_forms.version ";
    Qry.CreateVariable("bp_type", otString, bp_type);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();


    if(Qry.Eof||Qry.FieldIsNULL("data")||
    	 Qry.FieldIsNULL( "form" ) && (prn_format==pfBTP || prn_format==pfATB || prn_format==pfEPL2)
    	)
      throw UserException("Печать пос. талона на выбранный принтер не производится");

    Pectab = Qry.FieldAsString("form");
    Print = Qry.FieldAsString("data");
}

void GetPrintDataBP(xmlNodePtr dataNode, int pax_id, int prn_type, int pr_lat, xmlNodePtr clientDataNode)
{
    tst();
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    TQuery Qry(&OraSession);
    Qry.SQLText = "select format from prn_types where code = :prn_type";
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    if(Qry.Eof) throw Exception("Unknown prn_type: " + IntToString(prn_type));
    TPrnFormat  prn_format = (TPrnFormat)Qry.FieldAsInteger("format");
    Qry.Clear();
    Qry.SQLText = "select grp_id, reg_no from pax where pax_id = :pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    string Pectab, Print;
    int reg_no = Qry.FieldAsInteger("reg_no");
    int grp_id = Qry.FieldAsInteger("grp_id");
    GetPrintData(grp_id, prn_type, Pectab, Print);
    NewTextChild(BPNode, "pectab", Pectab);
    tst();
    PrintDataParser parser(grp_id, pax_id, pr_lat, clientDataNode);
    tst();
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    xmlNodePtr paxNode = NewTextChild(passengersNode,"pax");
        string prn_form = parser.parse(Print);
    if(prn_format == pfEPSON) {
        to_esc::convert(prn_form, TPrnType(prn_type), NULL);
        prn_form = b64_encode(prn_form.c_str(), prn_form.size());
    }
    NewTextChild(paxNode, "prn_form", prn_form);
    {
        TQuery *Qry = parser.get_prn_qry();
        TDateTime time_print = NowUTC();
        Qry->CreateVariable("now_utc", otDate, time_print);
        ProgTrace(TRACE5, "PRN QUERY: %s", Qry->SQLText.SQLText());
        Qry->Execute();
        SetProp(paxNode, "pax_id", pax_id);
        SetProp(paxNode, "reg_no", reg_no);
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
    Qry.Clear();
    Qry.SQLText="SELECT class FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    if(Qry.FieldIsNULL("class"))
        throw UserException("Для багажа без сопровождения посадочный талон не печатается.");
    Qry.Clear();
    Qry.SQLText = "select format from prn_types where code = :prn_type";
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    if(Qry.Eof) throw Exception("Unknown prn_type: " + IntToString(prn_type));
    TPrnFormat  prn_format = (TPrnFormat)Qry.FieldAsInteger("format");
    Qry.Clear();
    if(pr_all)
        Qry.SQLText =
            "select pax_id, reg_no from pax where grp_id = :grp_id and refuse is null order by reg_no";
    else
        Qry.SQLText =
            "select pax.pax_id, pax.reg_no  from "
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
        int reg_no = Qry.FieldAsInteger("reg_no");
        tst();
        PrintDataParser parser(grp_id, pax_id, pr_lat, clientDataNode);
        tst();
        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        string prn_form = parser.parse(Print);
        tst();
        if(prn_format == pfEPSON) {
            to_esc::convert(prn_form, TPrnType(prn_type), NULL);
            prn_form = b64_encode(prn_form.c_str(), prn_form.size());
        }
        NewTextChild(paxNode, "prn_form", prn_form);
        {
            TQuery *Qry = parser.get_prn_qry();
            TDateTime time_print = NowUTC();
            Qry->CreateVariable("now_utc", otDate, time_print);
            Qry->Execute();
            SetProp(paxNode, "pax_id", pax_id);
            SetProp(paxNode, "reg_no", reg_no);
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
        "select  "
        "   bt_forms.num, "
        "   bt_forms.form,  "
        "   bt_forms.data  "
        "from  "
        "   bt_forms, "
        "   ( "
        "    select "
        "        tag_type, "
        "        prn_type, "
        "        num, "
        "        max(version) version "
        "    from "
        "        bt_forms "
        "    group by "
        "        tag_type, "
        "        prn_type, "
        "        num "
        "   ) a "
        "where  "
        "   a.tag_type = :tag_type and "
        "   a.prn_type = :prn_type and "
        "   a.tag_type = bt_forms.tag_type and "
        "   a.prn_type = bt_forms.prn_type and "
        "   a.num = bt_forms.num and "
        "   a.version = bt_forms.version "
        "order by "
        "   bt_forms.num ";
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
        "SELECT class, ckin.get_main_pax_id(:grp_id,0) AS pax_id FROM pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("GRP_ID", otInteger, tag_key.grp_id);
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    bool pr_unaccomp = Qry.FieldIsNULL("class");
    int pax_id=NoExists;
    if(!pr_unaccomp)
    {
      if (Qry.FieldIsNULL("pax_id"))
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
      pax_id = Qry.FieldAsInteger("pax_id");
    };
    string SQLText =
        "SELECT "
        "   bag_tags.num, "
        "   bag2.pr_liab_limit, "
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

    TQuery unaccQry(&OraSession);
    unaccQry.SQLText =
        "select "
        "   unaccomp_bag_names.name, "
        "   unaccomp_bag_names.name_lat "
        "from "
        "   bag_tags, "
        "   bag2, "
        "   unaccomp_bag_names "
        "where "
        "   bag_tags.grp_id=bag2.grp_id and "
        "   bag_tags.bag_num=bag2.num and "
        "   bag2.bag_type=unaccomp_bag_names.code and "
        "   bag_tags.grp_id=:grp_id and bag_tags.num=:tag_num ";
    unaccQry.CreateVariable("grp_id",otInteger,tag_key.grp_id);
    unaccQry.DeclareVariable("tag_num",otInteger);

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

        PrintDataParser parser(tag_key.grp_id, pax_id, tag_key.pr_lat, NULL);

        parser.add_tag("aircode", aircode);
        parser.add_tag("no", no);
        parser.add_tag("issued", issued);
        parser.add_tag("bt_amount", Qry.FieldAsString("bag_amount"));
        parser.add_tag("bt_weight", Qry.FieldAsString("bag_weight"));
        bool pr_liab = Qry.FieldAsInteger("pr_liab_limit") == 1;
        parser.add_tag("liab_limit", upperc((pr_liab ? "Огр. ответственности" : "")));
        parser.add_tag("liab_limit_lat", upperc((pr_liab ? "Liab. limit" : "")));

        if(pr_unaccomp) {
            tst();
            unaccQry.SetVariable("tag_num",Qry.FieldAsInteger("num"));
            tst();
            unaccQry.Execute();
            string print_name="БЕЗ СОПРОВОЖДЕНИЯ",print_name_lat="UNACCOMPANIED";
            if (!unaccQry.Eof)
            {
                print_name=unaccQry.FieldAsString("name");
                print_name_lat=unaccQry.FieldAsString("name_lat");
            };
            parser.add_tag("surname",print_name );
            parser.add_tag("surname_lat", print_name_lat);
            parser.add_tag("fullname",print_name );
            parser.add_tag("fullname_lat", print_name_lat);
        }

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
            table = "br_forms ";
            break;
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
        "   prn_formats.code format, "
        "   prn_types.pr_stock "
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

        int code = Qry.FieldAsInteger("code");
        string name = Qry.FieldAsString("name");
        string iface = Qry.FieldAsString("iface");
        int format_id = Qry.FieldAsInteger("format_id");
        string format = Qry.FieldAsString("format");
        int pr_stock = Qry.FieldAsInteger("pr_stock");

        NewTextChild(printerNode, "code", code);
        NewTextChild(printerNode, "name", name);
        NewTextChild(printerNode, "iface", iface);
        NewTextChild(printerNode, "format_id", format_id);
        NewTextChild(printerNode, "format", format);
        NewTextChild(printerNode, "pr_stock", pr_stock);

        Qry.Next();
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void PrintInterface::GetPrintDataBR(string &form_type, int prn_type, PrintDataParser &parser, string &Print,
        xmlNodePtr reqNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
            "select  "
            "   br_forms.data  "
            "from  "
            "   br_forms, "
            "   ( "
            "    select "
            "        form_type, "
            "        prn_type, "
            "        max(version) version "
            "    from "
            "        br_forms "
            "    group by "
            "        form_type, "
            "        prn_type "
            "   ) a "
            "where  "
            "   a.form_type = :form_type and "
            "   a.prn_type = :prn_type and "
            "   a.form_type = br_forms.form_type and "
            "   a.prn_type = br_forms.prn_type and "
            "   a.version = br_forms.version ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    if(Qry.Eof||Qry.FieldIsNULL("data"))
      throw UserException("Печать квитанции на выбранный принтер не производится");

    string mso_form = Qry.FieldAsString("data");
    mso_form = parser.parse(mso_form);

    to_esc::convert(mso_form, TPrnType(prn_type), reqNode);

    Print = b64_encode(mso_form.c_str(), mso_form.size());
}


string get_validator(TBagReceipt &rcpt)
{
    tst();
    ostringstream validator;
    string agency, agency_name, sale_point_descr, sale_point_city, sale_point;
    int private_num;

    TQuery Qry(&OraSession);

    Qry.Clear();
    Qry.SQLText="SELECT validator FROM form_types WHERE code=:code";
    Qry.CreateVariable("code", otString, rcpt.form_type);
    Qry.Execute();
    if (Qry.Eof) throw Exception("get_validator: unknown form_type %s",rcpt.form_type.c_str());
    string validator_type=Qry.FieldAsString("validator");

    TReqInfo *reqInfo = TReqInfo::Instance();
    Qry.Clear();
    Qry.SQLText="SELECT sale_point FROM sale_desks "
                "WHERE code=:code AND validator=:validator AND pr_denial=0";
    Qry.CreateVariable("code", otString, reqInfo->desk.code);
    Qry.CreateVariable("validator", otString, validator_type);
    Qry.Execute();
    if (Qry.Eof) throw UserException("Оформление квитанции формы %s с данного пульта запрещено", rcpt.form_type.c_str());
    sale_point=Qry.FieldAsString("sale_point");

    Qry.Clear();
    Qry.SQLText=
        "SELECT "
        "   sale_points.agency, "
        "   agencies.name agency_name, "
        "   sale_points.descr, "
        "   sale_points.city "
        "FROM "
        "   sale_points, "
        "   agencies "
        "WHERE "
        "   sale_points.code=:code AND "
        "   sale_points.validator=:validator and "
        "   sale_points.agency = agencies.code ";
    Qry.CreateVariable("code", otString, sale_point);
    Qry.CreateVariable("validator", otString, validator_type);
    Qry.Execute();
    if (Qry.Eof) throw Exception("sale point '%s' not found for validator '%s'", sale_point.c_str(), validator_type.c_str());
    agency = Qry.FieldAsString("agency");
    agency_name = Qry.FieldAsString("agency_name");
    sale_point_descr = Qry.FieldAsString("descr");
    sale_point_city = Qry.FieldAsString("city");

    Qry.Clear();
    Qry.SQLText =
        "SELECT private_num, agency FROM operators "
        "WHERE login=:login AND validator=:validator AND pr_denial=0";
    Qry.CreateVariable("login",otString,reqInfo->user.login);
    Qry.CreateVariable("validator", otString, validator_type);
    Qry.Execute();
    if(Qry.Eof) throw UserException("Оформление квитанции формы %s данному пользователю запрещено", rcpt.form_type.c_str());
    private_num = Qry.FieldAsInteger("private_num");
    ProgTrace(TRACE5, "AGENCIES: %s %s", agency.c_str(), Qry.FieldAsString("agency"));
    if(agency != Qry.FieldAsString("agency")) // Агентство пульта не совпадает с агентством кассира
        throw UserException("Агентство пульта не совпадает с агентством пользователя");


    TBaseTableRow &city = base_tables.get("cities").get_row("code", sale_point_city);
    TBaseTableRow &country = base_tables.get("countries").get_row("code", city.AsString("country"));
    if(validator_type == "ТКП") {
        // agency
        validator << agency << " ТКП";
        validator << endl;
        // agency descr
        validator << sale_point_descr.substr(0, 19) << endl;
        // agency city
        validator << city.AsString("Name").substr(0, 16) << " " << country.AsString("code") << endl;
        // agency code
        validator
            << sale_point << "  "
            << setw(4) << setfill('0') << private_num << endl;
        validator << endl; // empty string for this type
    } else if(
            validator_type == "ЮТ" ||
            validator_type == "ИАТА") {
        validator << sale_point << " " << DateTimeToStr(rcpt.issue_date, "ddmmmyy") << endl;
        validator << agency_name.substr(0, 19) << endl;
        validator << sale_point_descr.substr(0, 19) << endl;
        validator << city.AsString("Name").substr(0, 16) << "/" << country.AsString("code") << endl;
        validator << setw(4) << setfill('0') << private_num << endl;
    } else
        throw Exception("get_validator: unknown validator type %s", validator_type.c_str());
    return validator.str();
}



void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
