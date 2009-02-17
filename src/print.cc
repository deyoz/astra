#include <fstream>
#include "print.h"
#include "oralib.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "misc.h"
#include "stages.h"
#include "docs.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "payment.h"
#include "exceptions.h"
#include "astra_misc.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;
using namespace StrUtils;

const string delim = "\xb";

typedef enum {pfBTP, pfATB, pfEPL2, pfEPSON, pfZEBRA} TPrnFormat;

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
                        len = ToInt(buf);
                        break;
                    case 1:
                        height = ToInt(buf);
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

    void convert(string &mso_form, TPrnType prn_type, xmlNodePtr reqNode)
    {
        double y_modif, x_modif;
        switch(prn_type) {
            case ptOLIVETTI:
            case ptOLIVETTICOM:
                x_modif = 4.76;
                y_modif = 6.9;
                break;
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
            mso_form = ConvertCodepage( mso_form, "CP866", prnParams.encoding );
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
                        x = ToInt(num);
                        num.erase();
                        Mode = 'Y';
                    } else
                        throw Exception("to_esc: x must be num");
                    break;
                case 'Y':
                    if(IsDigit(curr_char))
                        num += curr_char;
                    else if(curr_char == ',') {
                        y = ToInt(num);
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
        "       :LIST_SEAT_NO, "
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
    prnQry->DeclareVariable("LIST_SEAT_NO", otString);
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


    di1 = data.find("LIST_SEAT_NO");
    if(di1 != data.end())
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("PR_SMOKE");
    di2 = data.find("NO_SMOKE");
    di3 = data.find("SMOKE");
    if(printed(di1) || printed(di2) || printed(di3))
        prnQry->SetVariable(di1->first, ToInt(di1->second.StringVal));


    di1 = data.find("BAG_AMOUNT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, ToInt(di1->second.StringVal));


    di1 = data.find("BAG_WEIGHT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, ToInt(di1->second.StringVal));


    di1 = data.find("RK_WEIGHT");
    if(printed(di1) && di1->second.StringVal.size())
        prnQry->SetVariable(di1->first, ToInt(di1->second.StringVal));


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
        prnQry->SetVariable(di1->first, ToInt(di1->second.StringVal));


    di1 = data.find("SURNAME");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);


    di1 = data.find("SUFFIX");
    if(printed(di1))
        prnQry->SetVariable(di1->first, di1->second.StringVal);

    return prnQry;
}

int PrintDataParser::t_field_map::GetTagAsInteger(string name)
{
    TData::iterator di = data.find(upperc(name));
    if(di == data.end()) throw Exception("Tag not found " + name);
    if(!di->second.err_msg.empty())
        throw UserException(di->second.err_msg);
    return di->second.IntegerVal;
}

string PrintDataParser::t_field_map::GetTagAsString(string name)
{
    TData::iterator di = data.find(upperc(name));
    if(di == data.end()) throw Exception("Tag not found " + name);
    if(!di->second.err_msg.empty())
        throw UserException(di->second.err_msg);
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

void PrintDataParser::t_field_map::add_err_tag(string name, string val)
{
    name = upperc(name);
    TTagValue TagValue;
    TagValue.err_msg = val;
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
            if(di->second.err_msg.empty()) {
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
            } else {
                ProgTrace(TRACE5, "data[%s] = ERROR!!! %s", di->first.c_str(), di->second.err_msg.c_str());
            }
        }
        ProgTrace(TRACE5, "------MAP DUMP------");
}

string PrintDataParser::t_field_map::get_field(string name, int len, string align, string date_format, int field_lat)
{
    string result;
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
            if(data["ETICKET_NO"].StringVal.empty())
                add_tag("etkt", "");
            else
                add_tag("etkt", "ETKT" + data["ETICKET_NO"].StringVal);
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

    if(field_lat < 0) field_lat = pr_lat;

    if(name == "BCBP_M_2") // 2мерный баркод. Формируется что называется on the fly
        add_tag(name, BCBP_M_2(field_lat));

    TData::iterator di, di_ru;
    di = data.find(name);
    di_ru = di;

    if(field_lat && di != data.end()) {
        TData::iterator di_lat = data.find(name + "_LAT");
        if(di_lat != data.end()) di = di_lat;
    }
    if(di == data.end()) throw Exception("Tag not found " + name);
    ProgTrace(TRACE5, "TAG: %s", di->first.c_str());
    ProgTrace(TRACE5, "TAG err: %s", di->second.err_msg.c_str());
    if(!di->second.err_msg.empty())
        throw UserException(di->second.err_msg);


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
        if(name == "ONE_CHAR" || print_mode == 2) {
            size_t result_size = result.size();
            result = "";
            if(
                    name == "PAX_ID" ||
                    name == "TEST_SERVER" ||
                    data[name].type == otDate
                    )
                result.append(result_size, '8');
            else
                result = AlignString("8", result_size, align);
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

string PrintDataParser::t_field_map::BCBP_M_2(bool pr_lat)
{
    string TAG_SURNAME = "SURNAME";
    string TAG_NAME = "NAME";
    string TAG_AIRLINE = "AIRLINE";
    string TAG_AIRP_DEP = "AIRP_DEP";
    string TAG_AIRP_ARV = "AIRP_ARV";
    string TAG_SUFFIX = "SUFFIX";
    string TAG_CLASS = "CLASS";
    string TAG_SEAT_NO = "SEAT_NO";

    if(pr_lat) {
        char *lat_suffix = "_LAT";
        TAG_SURNAME += lat_suffix;
        TAG_NAME += lat_suffix;
        TAG_AIRLINE += lat_suffix;
        TAG_AIRP_DEP += lat_suffix;
        TAG_AIRP_ARV += lat_suffix;
        TAG_SUFFIX += lat_suffix;
        TAG_CLASS += lat_suffix;
        TAG_SEAT_NO += lat_suffix;
    }

    ostringstream result;
    result
        << "M"
        << 1;
    // Passenger Name
    string surname = data[TAG_SURNAME].StringVal;
    string name = data[TAG_NAME].StringVal;
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
    result << (data["ETKT"].StringVal.empty() ? " " : "E");
    // Operating carrier PNR code
    int pax_id = data["PAX_ID"].IntegerVal;
    vector<TPnrAddrItem> pnrs;
    GetPaxPnrAddr(pax_id, pnrs);
    vector<TPnrAddrItem>::iterator iv = pnrs.begin();
    for(; iv != pnrs.end(); iv++)
        if(data["AIRLINE"].StringVal == iv->airline) {
            ProgTrace(TRACE5, "PNR found: %s", iv->addr);
            break;
        }
    if(iv == pnrs.end())
        result << setw(7) << " ";
    else if(strlen(iv->addr) <= 7)
        result << setw(7) << left << convert_pnr_addr(iv->addr, pr_lat);
    // From City Airport Code
    result << setw(3) << data[TAG_AIRP_DEP].StringVal;
    // To City Airport Code
    result << setw(3) << data[TAG_AIRP_ARV].StringVal;
    // Operating Carrier Designator
    result << setw(3) << data[TAG_AIRLINE].StringVal;
    // Flight Number
    result
        << setw(4) << right << setfill('0') << data["FLT_NO"].StringVal
        << setw(1) << setfill(' ') << data[TAG_SUFFIX].StringVal;
    // Date of Flight
    TDateTime scd = data["SCD"].DateTimeVal;
    int Year, Month, Day;
    DecodeDate(scd, Year, Month, Day);
    TDateTime first, last;
    EncodeDate(Year, 1, 1, first);
    EncodeDate(Year, Month, Day + 1, last);
    TDateTime period = last - first;
    result
        << fixed << setprecision(0) << setw(3) << setfill('0') << period;
    // Compartment Code
    result << data[TAG_CLASS].StringVal;
    // Seat Number
    result << setw(4) << right << data[TAG_SEAT_NO].StringVal;
    // Check-In Sequence Number
    result
        << setw(4) <<  setfill('0') << data["REG_NO"].IntegerVal
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
        TPerson pers_type = DecodePerson((char *)data["PERS_TYPE"].StringVal.c_str());
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
        cond1 << setw(10) << right << setfill('0') << pax_id;

    }

    // Field size of following varible size field
    result << setw(2) << right << setfill('0') << hex << uppercase << cond1.str().size();
    result << cond1.str();


    return result.str();
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
//        "   points.AIRLINE, "
//        "   airlines.code_lat airline_lat, "
//        "   airlines.name airline_name, "
//        "   airlines.name_lat airline_name_lat, "
//        "   nvl(airlines.short_name, airlines.name) airline_short, "
//        "   nvl(airlines.short_name_lat, airlines.name_lat) airline_short_lat, "
        "   crafts.code craft, "
        "   crafts.code_lat craft_lat, "
        "   points.BORT, "
        "   system.transliter(points.BORT, 1) bort_lat "
//        "   DECODE(SIGN(LENGTH(points.flt_no)-3),-1,LPAD(points.flt_no,3,'0'),points.flt_no)||points.suffix flt_no, "
//        "   DECODE(SIGN(LENGTH(points.flt_no)-3),-1,LPAD(points.flt_no,3,'0'),points.flt_no)||tlg.convert_suffix(points.SUFFIX, 1) flt_no_lat, "
//        "   points.SUFFIX, "
//        "   tlg.convert_suffix(points.SUFFIX, 1) suffix_lat "
        "from "
        "   points, "
        "   airlines, "
        "   crafts "
        "where "
        "   points.point_id = :point_id and points.pr_del>=0 and "
        "   points.airline = airlines.code and "
        "   points.craft = crafts.code" ;
    Qry->CreateVariable("point_id", otInteger, trip_id);
    Qry->CreateVariable("brd_open_stage_id", otInteger, sOpenBoarding);
    Qry->CreateVariable("brd_close_stage_id", otInteger, sCloseBoarding);
    Qrys.push_back(Qry);

    {
        TQuery Qry(&OraSession);
        Qry.SQLText = "select amount, weight from bag2 where grp_id = :grp_id ";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
        Qry.Execute();
        string amount = "0";
        string weight = "0";
        if(!Qry.Eof) {
            amount = Qry.FieldAsString("amount");
            weight = Qry.FieldAsString("weight");
        }
        add_tag("bt_amount", amount);
        add_tag("bt_weight", weight);
    }

    {
        TQuery Qry(&OraSession);
        bool pr_bp_market_flt = false;
        Qry.SQLText = "select pr_bp_market_flt from trip_sets where point_id = :point_id";
        Qry.CreateVariable("point_id", otInteger, trip_id);
        Qry.Execute();
        if(!Qry.Eof && !Qry.FieldIsNULL(0))
            pr_bp_market_flt = Qry.FieldAsInteger(0) == 1;
        string airline;
        string airline_lat;
        string airline_name;
        string airline_name_lat;
        string airline_short;
        string airline_short_lat;
        string flt_no;
        string flt_no_lat;
        string suffix;
        string suffix_lat;
        string sel_airline;
        int sel_flt_no;
        string sel_suffix;

        if(pax_id != NoExists && pr_bp_market_flt) {
            Qry.Clear();
            Qry.SQLText = //pnr_market_flt
                "select "
                "   tlg_trips.airline, "
                "   tlg_trips.flt_no, "
                "   tlg_trips.suffix "
                "from "
                "   pax, "
                "   crs_pax, "
                "   crs_pnr, "
                "   tlg_trips "
                "where "
                "   pax.pax_id = :pax_id and "
                "   pax.pax_id = crs_pax.pax_id and "
                "   crs_pax.pnr_id = crs_pnr.pnr_id and "
                "   crs_pnr.point_id = tlg_trips.point_id ";
            Qry.CreateVariable("pax_id", otInteger, pax_id);
            Qry.Execute();
            if(!Qry.Eof) {
                sel_airline = Qry.FieldAsString("airline");
                sel_flt_no = Qry.FieldAsInteger("flt_no");
                sel_suffix = Qry.FieldAsString("suffix");
            } else {
                Qry.Clear();
                Qry.SQLText =
                    "select "
                    "   airline, "
                    "   flt_no, "
                    "   suffix "
                    "from "
                    "   points "
                    "where "
                    "   point_id = :point_id and points.pr_del>=0 ";
                Qry.CreateVariable("point_id", otInteger, trip_id);
                Qry.Execute();
                if(!Qry.Eof) {
                    sel_airline = Qry.FieldAsString("airline");
                    sel_flt_no = Qry.FieldAsInteger("flt_no");
                    sel_suffix = Qry.FieldAsString("suffix");
                }
            }
        } else {
            Qry.Clear();
            Qry.SQLText =
                "select "
                "   airline, "
                "   flt_no, "
                "   suffix "
                "from "
                "   points "
                "where "
                "   point_id = :point_id and points.pr_del>=0 ";
            Qry.CreateVariable("point_id", otInteger, trip_id);
            Qry.Execute();
            if(!Qry.Eof) {
                sel_airline = Qry.FieldAsString("airline");
                sel_flt_no = Qry.FieldAsInteger("flt_no");
                sel_suffix = Qry.FieldAsString("suffix");
            }
        }


        if(sel_airline.empty())
            throw Exception("fillBTBPMap: arline not defined");
        airline = sel_airline;
        suffix = sel_suffix;
        TBaseTableRow &airlineRow = base_tables.get("AIRLINES").get_row("code",airline);
        airline_lat = airlineRow.AsString("code", 1);
        airline_name = airlineRow.AsString("name", 0);
        airline_name_lat = airlineRow.AsString("name", 1);
        airline_short = airlineRow.AsString("short_name", 0);
        airline_short_lat = airlineRow.AsString("short_name", 1);
        suffix_lat = convert_suffix(suffix, 1);
        ostringstream buf;
        buf << setw(3) << setfill('0') << sel_flt_no;
        flt_no = buf.str() + suffix;
        flt_no_lat = buf.str() + suffix_lat;

        add_tag("airline", airline);
        add_tag("airline_lat", airline_lat);
        add_tag("airline_name", airline_name);
        add_tag("airline_name_lat", airline_name_lat);
        add_tag("airline_short", airline_short);
        add_tag("airline_short_lat", airline_short_lat);
        add_tag("flt_no", flt_no);
        add_tag("flt_no_lat", flt_no_lat);
        add_tag("suffix", suffix);
        add_tag("suffix_lat", suffix_lat);
    }

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
            "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'list',NULL,0) AS list_seat_no, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'voland',NULL,0) AS str_seat_no, "
            "   system.transliter(salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'voland',NULL,1)) AS str_seat_no_lat, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'seats',NULL,0) AS seat_no, "
            "   system.transliter(salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'seats',NULL,1)) AS seat_no_lat, "
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
            "   DECODE(pax.ticket_rem,'TKNE',ticket_no||'/'||coupon_no,NULL) eticket_no, "
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

string get_mso_point(const string &aairp, bool pr_lat)
{
    TBaseTable &airps = base_tables.get("airps");
    TBaseTable &cities = base_tables.get("cities");
    string city = airps.get_row("code", aairp).AsString("city");
    string point = cities.get_row("code", city).AsString("name", pr_lat);
    if(point.empty()) throw UserException((string)"Не определено" + (pr_lat ? "лат." : " ") + "название города '" + city + "'");
    TQuery airpsQry(&OraSession);
    airpsQry.SQLText =  "select count(*) from airps where city = :city";
    airpsQry.CreateVariable("city", otString, city);
    airpsQry.Execute();
    if(!airpsQry.Eof && airpsQry.FieldAsInteger(0) != 1) {
        string airp = airps.get_row("code", aairp).AsString("code", pr_lat);
        if(airp.empty()) throw UserException((string)"Не определен" + (pr_lat ? "лат." : " ") + "код а/п '" + aairp + "'");
        point += "(" + airp + ")";
    }
    return point;
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
};

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
};

double CalcPayRate(const TBagReceipt &rcpt)
{
  double pay_rate;
  if (rcpt.pay_rate_cur != rcpt.rate_cur)
    pay_rate = (rcpt.rate * rcpt.exch_pay_rate)/rcpt.exch_rate;
  else
    pay_rate = rcpt.rate;
  return pay_rate;
};

double CalcRateSum(const TBagReceipt &rcpt)
{
  double rate_sum;
  if(rcpt.service_type == 1 || rcpt.service_type == 2) {
      rate_sum = rcpt.rate * rcpt.ex_weight;
  } else {
      rate_sum = rcpt.rate * rcpt.value_tax/100;
  }
  return rate_sum;
};

double CalcPayRateSum(const TBagReceipt &rcpt)
{
  double pay_rate_sum;
  if(rcpt.service_type == 1 || rcpt.service_type == 2) {
      pay_rate_sum = CalcPayRate(rcpt) * rcpt.ex_weight;
  } else {
      pay_rate_sum = CalcPayRate(rcpt) * rcpt.value_tax/100;
  }
  return pay_rate_sum;
};

void PrintDataParser::t_field_map::add_mso_point(std::string name, std::string airp, bool pr_lat)
{
    try {
        add_tag(name, get_mso_point(airp, pr_lat));
    } catch(Exception E) {
        add_err_tag(name, E.what());
    }

}

void PrintDataParser::t_field_map::fillMSOMap(TBagReceipt &rcpt)
{
  TQuery Qry(&OraSession);

  add_tag("pax_name",rcpt.pax_name);
  add_tag("pax_doc",rcpt.pax_doc);

  Qry.Clear();
  Qry.SQLText =  "select name, name_lat from rcpt_service_types where code = :code";
  Qry.CreateVariable("code", otInteger, rcpt.service_type);
  Qry.Execute();
  if(Qry.Eof) throw Exception("fillMSOMap: service_type not found (code = %d)", rcpt.service_type);
  add_tag("service_type", (string)"10 " + Qry.FieldAsString("name"));
  add_tag("service_type_lat", (string)"10 " + Qry.FieldAsString("name_lat"));

  if(rcpt.service_type == 2 && rcpt.bag_type != -1)
  {
    string bag_name, bag_name_lat;
    if (rcpt.bag_name.empty())
    {
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
      if(rcpt.bag_type == 1 || rcpt.bag_type == 2)
      {
        //негабарит
          bag_name += " " + IntToString(rcpt.ex_amount) + " " + pieces(rcpt.ex_amount, 0);
          bag_name_lat += " " + IntToString(rcpt.ex_amount) + " " + pieces(rcpt.ex_amount, 1);
      }
      bag_name = upperc(bag_name);
      bag_name_lat = upperc(bag_name_lat);
    }
    else
    {
      //bag_name введен вручную
      bag_name = rcpt.bag_name;
      bag_name_lat = rcpt.bag_name;
    };

    add_tag("bag_name", bag_name);
    add_tag("bag_name_lat", bag_name_lat);

    bool pr_other=true;

    if(rcpt.bag_type == 20) //лыжи
    {
      add_tag("SkiBT", "x");
      pr_other=false;
    }
    else
      add_tag("SkiBT", "");

    if(rcpt.bag_type == 21 && rcpt.form_type != "Z61") //гольф
    {
      add_tag("GolfBT", "x");
      pr_other=false;
    }
    else
      add_tag("GolfBT", "");

    if(rcpt.bag_type == 4)  //животные
    {
      add_tag("PetBT", "x");
      pr_other=false;
    }
    else
      add_tag("PetBT", "");

    if(rcpt.bag_type == 1 || rcpt.bag_type == 2)
    {
      //негабарит
      add_tag("BulkyBT", "x");
      add_tag("BulkyBTLetter", IntToString(rcpt.ex_amount));
      pr_other=false;
    }
    else
    {
      add_tag("BulkyBT", "");
      add_tag("BulkyBTLetter", "");
    };

    if (pr_other)
    {
      add_tag("OtherBT", "x");
      add_tag("OtherBTLetter", bag_name);
      add_tag("OtherBTLetter_lat", bag_name_lat);
    }
    else
    {
      add_tag("OtherBT", "");
      add_tag("OtherBTLetter", "");
      add_tag("OtherBTLetter_lat", "");
    };
  }
  else
  {
    //это не платный багаж
    add_tag("bag_name", "");
    add_tag("bag_name_lat", "");

    add_tag("SkiBT", "");
    add_tag("GolfBT", "");
    add_tag("PetBT", "");
    add_tag("BulkyBT", "");
    add_tag("BulkyBTLetter", "");
    add_tag("OtherBT", "");
    add_tag("OtherBTLetter", "");
    add_tag("OtherBTLetter_lat", "");
  };

  double pay_rate=CalcPayRate(rcpt);
  double rate_sum=CalcRateSum(rcpt);
  double pay_rate_sum=CalcPayRateSum(rcpt);

  ostringstream remarks, remarks_lat, rate, rate_lat, ex_weight, ex_weight_lat,
                pay_types, pay_types_lat;

  bool pr_exchange=false;

  if(rcpt.service_type == 1 || rcpt.service_type == 2)
  {
      remarks << "ТАРИФ ЗА КГ=";
      remarks_lat << "RATE PER KG=";
      if(
             (rcpt.pay_rate_cur == "РУБ" ||
              rcpt.pay_rate_cur == "ДОЛ" ||
              rcpt.pay_rate_cur == "ЕВР") &&
              rcpt.pay_rate_cur != rcpt.rate_cur
        ) {
          rate
              << RateToString(pay_rate, rcpt.pay_rate_cur, false, 0)
              << "(" << RateToString(rcpt.rate, rcpt.rate_cur, false, 0) << ")";
          rate_lat
              << RateToString(pay_rate, rcpt.pay_rate_cur, true, 0)
              << "(" << RateToString(rcpt.rate, rcpt.rate_cur, true, 0) << ")";

          remarks
              << rate.str()
              << "(" << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, false)
              << ")";
          remarks_lat
              << rate_lat.str()
              << "(RATE " << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, true)
              << ")";
          pr_exchange=true;
      } else {
          rate << RateToString(rcpt.rate, rcpt.rate_cur, false, 0);
          rate_lat << RateToString(rcpt.rate, rcpt.rate_cur, true, 0);

          remarks << rate.str();
          remarks_lat << rate_lat.str();
      }
      ex_weight << IntToString(rcpt.ex_weight) << "КГ";
      ex_weight_lat << IntToString(rcpt.ex_weight) << "KG";

      add_tag("rate", rate.str());
      add_tag("rate_lat", rate_lat.str());
      add_tag("remarks1", remarks.str());
      add_tag("remarks1_lat", remarks_lat.str());
      add_tag("remarks2", ex_weight.str());
      add_tag("remarks2_lat", ex_weight_lat.str());
      if (rcpt.form_type == "M61")
      {
        add_tag("ex_weight", ex_weight.str());
        add_tag("ex_weight_lat", ex_weight_lat.str());
      }
      else
      {
        add_tag("ex_weight", IntToString(rcpt.ex_weight));
        add_tag("ex_weight_lat", IntToString(rcpt.ex_weight));
      };

      add_tag("ValueBT", "");
      add_tag("ValueBTLetter", "");
      add_tag("ValueBTLetter_lat", "");
  }
  else
  {
      //багаж с объявленной ценностью
      rate
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax <<"%";
      rate_lat
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax <<"%";
      remarks
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax << "% OT "
            << RateToString(rcpt.rate, rcpt.rate_cur, false, 0);
      remarks_lat
            << fixed << setprecision(get_value_tax_precision(rcpt.value_tax))
            << rcpt.value_tax << "% OF "
            << RateToString(rcpt.rate, rcpt.rate_cur, true, 0);
      add_tag("rate", rate.str());
      add_tag("rate_lat", rate_lat.str());
      add_tag("remarks1", remarks.str());
      add_tag("remarks1_lat", remarks_lat.str());
      add_tag("remarks2", "");
      add_tag("remarks2_lat", "");
      add_tag("ex_weight", "");
      add_tag("ex_weight_lat", "");

      string ValueBTLetter, ValueBTLetter_lat;
      {
          int iptr, fract;
          fract=separate_double(rcpt.rate, 2, &iptr);

          string buf_ru = vs_number(iptr, 0);
          string buf_lat = vs_number(iptr, 1);

          if(fract!=0) {
              string str_fract = IntToString(fract) + "/100 ";
              buf_ru += str_fract;
              buf_lat += str_fract;
          }
          ValueBTLetter = upperc(buf_ru) +
              base_tables.get("currency").get_row("code", rcpt.rate_cur).AsString("code", 0);
          ValueBTLetter_lat = upperc(buf_lat) +
              base_tables.get("currency").get_row("code", rcpt.rate_cur).AsString("code", 1);
      }
      add_tag("ValueBT", "x");
      add_tag("ValueBTLetter", ValueBTLetter);
      add_tag("ValueBTLetter_lat", ValueBTLetter_lat);
  };

  for(int fmt=0;fmt<=2;fmt++)
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
    if (fmt==0)
    {
      add_tag("charge", remarks.str());
      add_tag("charge_lat", remarks_lat.str());
    };
    if (fmt==1)
    {
      add_tag("amount_figures", remarks.str());
      add_tag("amount_figures_lat", remarks_lat.str());
    };
    if (fmt==2)
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
      int iptr, fract;
      fract=separate_double(fare_sum, 2, &iptr);

      string buf_ru = vs_number(iptr, 0);
      string buf_lat = vs_number(iptr, 1);

      if(fract!=0) {
          string str_fract = IntToString(fract) + "/100 ";
          buf_ru += str_fract;
          buf_lat += str_fract;
      }
      add_tag("amount_letters", upperc(buf_ru));
      add_tag("amount_letters_lat", upperc(buf_lat));
  }

  //exchange_rate
  remarks.str("");
  remarks_lat.str("");

  if (rcpt.pay_rate_cur != rcpt.rate_cur &&
      (!pr_exchange || rcpt.form_type != "M61"))
  {
      remarks
          << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, false);

      if (rcpt.form_type != "M61") remarks_lat << "RATE ";
      remarks_lat
          << ExchToString(rcpt.exch_rate, rcpt.rate_cur, rcpt.exch_pay_rate, rcpt.pay_rate_cur, true);
  };

  add_tag("exchange_rate", remarks.str());
  add_tag("exchange_rate_lat", remarks_lat.str());

  //total
  add_tag("total", RateToString(pay_rate_sum, rcpt.pay_rate_cur, false, 0));
  add_tag("total_lat", RateToString(pay_rate_sum, rcpt.pay_rate_cur, true, 0));

  add_tag("tickets", rcpt.tickets);
  add_tag("prev_no", rcpt.prev_no);

  vector<TBagPayType>::iterator i;
  TBaseTable& payTypeCodes=base_tables.get("pay_types");
  if (rcpt.pay_types.size()==1)
  {
    //одна форма оплаты
    i=rcpt.pay_types.begin();
    pay_types     << payTypeCodes.get_row("code", i->pay_type).AsString("code", false);
    pay_types_lat << payTypeCodes.get_row("code", i->pay_type).AsString("code", true);
    if (i->pay_type!=CASH_PAY_TYPE && !i->extra.empty())
    {
      pay_types     << ' ' << i->extra;
      pay_types_lat << ' ' << i->extra;
    };
  }
  else
  {
    //несколько форм оплаты
    //первой всегда идет НАЛ
    for(int k=0;k<=1;k++)
    {
      for(i=rcpt.pay_types.begin();i!=rcpt.pay_types.end();i++)
      {
        if (k==0 && i->pay_type!=CASH_PAY_TYPE ||
            k!=0 && i->pay_type==CASH_PAY_TYPE) continue;

        if (!pay_types.str().empty())
        {
          pay_types     << '+';
          pay_types_lat << '+';
        };
        pay_types     << payTypeCodes.get_row("code", i->pay_type).AsString("code", false)
                      << RateToString(i->pay_rate_sum, rcpt.pay_rate_cur, false, 0);

        pay_types_lat << payTypeCodes.get_row("code", i->pay_type).AsString("code", true)
                      << RateToString(i->pay_rate_sum, rcpt.pay_rate_cur, true, 0);
        if (i->pay_type!=CASH_PAY_TYPE && !i->extra.empty())
        {
          pay_types     << ' ' << i->extra;
          pay_types_lat << ' ' << i->extra;
        };
      };
    };
  };

  add_tag("pay_form", pay_types.str());
  add_tag("pay_form_lat", pay_types_lat.str());

  TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code", rcpt.airline);
  if(!airline.short_name.empty())
      add_tag("airline", airline.short_name);
  else if(!airline.name.empty())
      add_tag("airline", airline.name);
  else
      add_err_tag("airline", "Не определено название а/к '" + rcpt.airline + "'");

  if(!airline.short_name_lat.empty())
      add_tag("airline_lat", airline.short_name_lat);
  else if(!airline.name_lat.empty())
      add_tag("airline_lat", airline.name_lat);
  else
      add_err_tag("airline_lat", "Не определено лат. название а/к '" + rcpt.airline + "'");

  add_tag("aircode", rcpt.aircode);

  {
      ostringstream flt_no, flt_no_lat;

      if(rcpt.flt_no != -1) {
              flt_no << setw(3) << setfill('0') << rcpt.flt_no << convert_suffix(rcpt.suffix, false);
              flt_no_lat << setw(3) << setfill('0') << rcpt.flt_no << convert_suffix(rcpt.suffix, true);

      }
      add_tag("flt_no", flt_no.str());
      add_tag("flt_no_lat", flt_no_lat.str());


      add_mso_point("point_dep", rcpt.airp_dep, 0);
      add_mso_point("point_dep_lat", rcpt.airp_dep, 1);
      add_mso_point("point_arv", rcpt.airp_arv, 0);
      add_mso_point("point_arv_lat", rcpt.airp_arv, 1);

      try {
          if(not data["POINT_DEP"].err_msg.empty())
              throw Exception(data["POINT_DEP"].err_msg);
          if(not data["POINT_ARV"].err_msg.empty())
              throw Exception(data["POINT_ARV"].err_msg);

          ostringstream buf;
          buf
              << data["POINT_DEP"].StringVal << "-" << data["POINT_ARV"].StringVal << " ";

          ostringstream airline_code;
          airline_code << airline.code;

          if(rcpt.flt_no != -1)
              airline_code
                  << " "
                  << flt_no.str();
          buf << airline_code.str();
          add_tag("airline_code", airline_code.str());
          add_tag("to", buf.str());
      } catch(Exception E) {
          add_err_tag("airline_code", E.what());
          add_err_tag("to", E.what());
      }

      try {
          if(not data["POINT_DEP_LAT"].err_msg.empty())
              throw Exception(data["POINT_DEP_LAT"].err_msg);
          if(not data["POINT_ARV_LAT"].err_msg.empty())
              throw Exception(data["POINT_ARV_LAT"].err_msg);

          ostringstream buf;
          buf
              << data["POINT_DEP_LAT"].StringVal << "-" << data["POINT_ARV_LAT"].StringVal << " ";

          ostringstream airline_code, airline_code_lat;
          if(airline.code_lat.empty())
              throw UserException("Не определен лат. код а/к '%s'", rcpt.airline.c_str());
          airline_code << airline.code;

          if(rcpt.flt_no != -1)
              airline_code
                  << " "
                  << flt_no.str();
          buf << airline_code.str();
          add_tag("airline_code_lat", airline_code.str());
          add_tag("to_lat", buf.str());
      } catch(Exception E) {
          add_err_tag("airline_code_lat", E.what());
          add_err_tag("to_lat", E.what());
      }
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
            field_map.get_field(FieldName, ToInt(FieldLen), FieldAlign, DateFormat, FieldLat);
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
                        if(buf.size()) FieldLen = ToInt(buf);
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
    if(form.substr(i, 2) == "1\xa") {
        i += 2;
        pectab_format = 1;
    }
    if(form.substr(i, 2) == "XX") {
        i += 2;
        field_map.print_mode = 1;
    } else if(form.substr(i, 1) == "X") {
        i += 1;
        field_map.print_mode = 2;
    } else if(form.substr(i, 1) == "S") {
        i += 1;
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

void GetTripBPPectabs(int point_id, string dev_model, string fmt_type, xmlNodePtr node)
{
    if (node==NULL) return;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   prn_form_vers.form "
        "from "
        "   bp_models, "
        "   prn_form_vers "
        "where "
        "   bp_models.form_type IN (SELECT DISTINCT bp_type FROM trip_bp WHERE point_id=:point_id) and "
        "   bp_models.dev_model = :dev_model and "
        "   bp_models.fmt_type = :fmt_type and "
        "   bp_models.id = prn_form_vers.id and "
        "   bp_models.version = prn_form_vers.version and "
        "   prn_form_vers.form is not null";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bp_forms");
    for(;!Qry.Eof;Qry.Next())
      NewTextChild(formNode,"form",Qry.FieldAsString("form"));
}

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

void GetTripBTPectabs(int point_id, string dev_model, string fmt_type, xmlNodePtr node)
{
    if (node==NULL) return;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   prn_form_vers.form "
        "from "
        "   bt_models, "
        "   prn_form_vers "
        "where "
        "   bt_models.form_type IN (SELECT DISTINCT bp_type FROM trip_bp WHERE point_id=:point_id) and "
        "   bt_models.dev_model = :dev_model and "
        "   bt_models.fmt_type = :fmt_type and "
        "   bt_models.id = prn_form_vers.id and "
        "   bt_models.version = prn_form_vers.version ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bt_forms");
    for(;!Qry.Eof;Qry.Next())
        NewTextChild(formNode,"form",Qry.FieldAsString("form"));
}

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

void get_bt_forms(string tag_type, string dev_model, string fmt_type, xmlNodePtr pectabsNode, vector<string> &prn_forms)
{
    prn_forms.clear();
    TQuery FormsQry(&OraSession);
    FormsQry.SQLText =
        "select "
        "   bt_models.num, "
        "   prn_form_vers.form, "
        "   prn_form_vers.data "
        "from "
        "   bt_models, "
        "   prn_form_vers "
        "where "
        "   bt_models.form_type = :form_type and "
        "   bt_models.dev_model = :dev_model and "
        "   bt_models.fmt_type = :fmt_type and "
        "   bt_models.id = prn_form_vers.id and "
        "   bt_models.version = prn_form_vers.version ";
    FormsQry.CreateVariable("form_type", otString, tag_type);
    FormsQry.CreateVariable("dev_model", otString, dev_model);
    FormsQry.CreateVariable("fmt_type", otString, fmt_type);
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
    string suffix;
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
        ProgTrace(TRACE5, "suffix: %s", iv->suffix.c_str());
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

void set_via_fields(PrintDataParser &parser, vector<TBTRouteItem> &route, int start_idx, int end_idx)
{
    int via_idx = 1;
    for(int j = start_idx; j < end_idx; ++j) {
        string str_via_idx = IntToString(via_idx);
        ostringstream flt_no;
        flt_no << setw(3) << setfill('0') << route[j].flt_no;
        parser.add_tag("flt_no" + str_via_idx, flt_no.str() + convert_suffix(route[j].suffix, 0));
        parser.add_tag("flt_no" + str_via_idx + "_lat", flt_no.str() + convert_suffix(route[j].suffix, 1));
        parser.add_tag("flt_no" + str_via_idx, flt_no.str());
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
    string dev_model; // instead of prn_type in new terminal
    string fmt_type; // entirely new property from new terminal
    int grp_id, prn_type, pr_lat;
    double no; //no = Float!
    string type, color;
    TTagKey(): grp_id(0), prn_type(0), pr_lat(0), no(-1.0) {};
};

void get_route(TTagKey &tag_key, vector<TBTRouteItem> &route)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  "
        "   points.scd_out scd,  "
        "   points.airline,  "
        "   airlines.code_lat airline_lat,  "
        "   points.flt_no,  "
        "   points.suffix,  "
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
        "   pax_grp.point_dep = points.point_id and points.pr_del>=0 and  "
        "   points.airline = airlines.code and  "
        "   pax_grp.airp_arv = airps.code "
        "union  "
        "select  "
        "   trfer_trips.scd,  "
        "   trfer_trips.airline,  "
        "   airlines.code_lat airline_lat,  "
        "   trfer_trips.flt_no,  "
        "   trfer_trips.suffix,  "
        "   transfer.airp_arv,  "
        "   airps.code_lat airp_arv_lat,  "
        "   airps.name airp_arv_name, "
        "   airps.name_lat airp_arv_name_lat, "
        "   transfer.transfer_num "
        "from  "
        "   transfer,  "
        "   trfer_trips, "
        "   airlines,  "
        "   airps  "
        "where  "
        "   transfer.point_id_trfer = trfer_trips.point_id and "
        "   transfer.grp_id = :grp_id and  "
        "   trfer_trips.airline = airlines.code and  "
        "   transfer.airp_arv = airps.code "
        "order by "
        "   transfer_num ";
    Qry.CreateVariable("grp_id", otInteger, tag_key.grp_id);
    Qry.Execute();
    int Year, Month;
    TBaseTable &airpsTable = base_tables.get("AIRPS");
    TBaseTable &citiesTable = base_tables.get("CITIES");
    while(!Qry.Eof) {
        TBTRouteItem RouteItem;
        RouteItem.airline = Qry.FieldAsString("airline");
        RouteItem.airline_lat = Qry.FieldAsString("airline_lat");
        RouteItem.flt_no = Qry.FieldAsInteger("flt_no");
        RouteItem.suffix = Qry.FieldAsString("suffix");
        RouteItem.airp_arv = Qry.FieldAsString("airp_arv");
        RouteItem.airp_arv_lat = Qry.FieldAsString("airp_arv_lat");
        RouteItem.airp_arv_name = Qry.FieldAsString("airp_arv_name");
        RouteItem.airp_arv_name_lat = Qry.FieldAsString("airp_arv_name_lat");
        RouteItem.fltdate = DateTimeToStr(Qry.FieldAsDateTime("scd"),(string)"ddmmm",0);
        RouteItem.fltdate_lat = DateTimeToStr(Qry.FieldAsDateTime("scd"),(string)"ddmmm",1);
        DecodeDate(Qry.FieldAsDateTime("scd"), Year, Month, RouteItem.local_date);
        route.push_back(RouteItem);

        TBaseTableRow &airpRow = airpsTable.get_row("code",RouteItem.airp_arv);
        TBaseTableRow &citiesRow = citiesTable.get_row("code",airpRow.AsString("city"));
        tag_key.pr_lat = tag_key.pr_lat || citiesRow.AsString("country") != "РФ";

        Qry.Next();
    }
    DumpRoute(route);
}

void check_CUTE_certified(int &prn_type, string &dev_model, string &fmt_type)
{
    if(prn_type != NoExists) {
        if(prn_type == 90) {
            dev_model = "ATB CUTE";
            fmt_type = "ATB";
        } else if(prn_type == 91) {
            dev_model = "BTP CUTE";
            fmt_type = "BTP";
        } else
            throw UserException("Версия терминала устарела. Обновите терминал.");
        prn_type = NoExists;
    }
}

void GetPrintDataBT(xmlNodePtr dataNode, TTagKey &tag_key)
{
//    check_CUTE_certified(tag_key.prn_type, tag_key.dev_model, tag_key.fmt_type);
    vector<TBTRouteItem> route;
    get_route(tag_key, route);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp_dep, class, ckin.get_main_pax_id(:grp_id,0) AS pax_id FROM pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("GRP_ID", otInteger, tag_key.grp_id);
    Qry.Execute();
    if (Qry.Eof)
      throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    TBaseTableRow &airpRow = base_tables.get("AIRPS").get_row("code",Qry.FieldAsString("airp_dep"));
    TBaseTableRow &citiesRow = base_tables.get("CITIES").get_row("code",airpRow.AsString("city"));
    tag_key.pr_lat = tag_key.pr_lat || citiesRow.AsString("country") != "РФ";
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
            if(tag_key.dev_model.empty())
                get_bt_forms(tag_type, tag_key.prn_type, pectabsNode, prn_forms);
            else
                get_bt_forms(tag_type, tag_key.dev_model, tag_key.fmt_type, pectabsNode, prn_forms);
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
    tag_key.dev_model = NodeAsString("dev_model", reqNode, "");
    tag_key.fmt_type = NodeAsString("fmt_type", reqNode, "");
    tag_key.prn_type = NodeAsInteger("prn_type", reqNode, NoExists);
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
    tag_key.dev_model = NodeAsString("dev_model", reqNode, "");
    tag_key.fmt_type = NodeAsString("fmt_type", reqNode, "");
    tag_key.prn_type = NodeAsInteger("prn_type", reqNode, NoExists);
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
        "   prn_types.iface <> 'CUT' and "
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
    } else {
        // все валидаторы кроме ТКП у нас пока обрабатываются одинаково
        validator << sale_point << " " << DateTimeToStr(rcpt.issue_date, "ddmmmyy") << endl;
        validator << agency_name.substr(0, 19) << endl;
        validator << sale_point_descr.substr(0, 19) << endl;
        validator << city.AsString("Name").substr(0, 16) << "/" << country.AsString("code") << endl;
        validator << setw(4) << setfill('0') << private_num << endl;
    }
    return validator.str();
}

struct TSegPax {
    int seg_no;
    int grp_id;
    int pax_id;
    TSegPax(): seg_no(NoExists), grp_id(NoExists), pax_id(NoExists) {};
};

struct TSegPaxList {
    vector<TSegPax> items;
    void get(int pax_id)
    {
        TQuery Qry(&OraSession);        
        Qry.SQLText =
            "select tckin_id, seg_no from tckin_pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(!Qry.Eof) {
            int tckin_id = Qry.FieldAsInteger("tckin_id");
            int seg_no = Qry.FieldAsInteger("seg_no");
            Qry.Clear();
            Qry.SQLText =
                "select "
                "   tckin_pax.seg_no, "
                "   tckin_pax.pax_id, "
                "   tckin_pax_grp.grp_id "
                "from "
                "   tckin_pax, "
                "   tckin_pax_grp "
                "where "
                "   tckin_pax.tckin_id = :tckin_id and "
                "   tckin_pax.seg_no >= :seg_no and "
                "   tckin_pax.tckin_id = tckin_pax_grp.tckin_id and "
                "   tckin_pax.seg_no = tckin_pax_grp.seg_no and "
                "   (tckin_pax.seg_no = :seg_no or tckin_pax_grp.pr_depend <> 0) "
                "order by "
                "   tckin_pax.seg_no ";
            Qry.CreateVariable("tckin_id", otInteger, tckin_id);
            Qry.CreateVariable("seg_no", otInteger, seg_no);
            Qry.Execute();
            if(!Qry.Eof) {
                int col_seg_no = Qry.FieldIndex("seg_no");
                int col_pax_id = Qry.FieldIndex("pax_id");
                int col_grp_id = Qry.FieldIndex("grp_id");
                for(; !Qry.Eof; Qry.Next()) {
                    TSegPax item;
                    item.seg_no = Qry.FieldAsInteger(col_seg_no);
                    item.grp_id = Qry.FieldAsInteger(col_grp_id);
                    item.pax_id = Qry.FieldAsInteger(col_pax_id);
                    items.push_back(item);
                }
            }
        }
    }
};

void PrintInterface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr currNode = reqNode->children;
    int grp_id = NodeAsIntegerFast("grp_id", currNode, NoExists);
    int pax_id = NodeAsIntegerFast("pax_id", currNode, NoExists);
    int pr_all = NodeAsIntegerFast("pr_all", currNode, NoExists);
    int prn_type = NodeAsIntegerFast("prn_type", currNode, NoExists);
    string dev_model = NodeAsStringFast("dev_model", currNode, "");
    string fmt_type = NodeAsStringFast("fmt_type", currNode, "");
    int pr_lat = NodeAsIntegerFast("pr_lat", currNode, NoExists);
    xmlNodePtr clientDataNode = NodeAsNodeFast("clientData", currNode);

//    check_CUTE_certified(prn_type, dev_model, fmt_type);

    TQuery Qry(&OraSession);

    if(grp_id == NoExists) {
        Qry.Clear();
        Qry.SQLText="SELECT grp_id from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
        grp_id = Qry.FieldAsInteger("grp_id");
    }
    Qry.Clear();
    Qry.SQLText="SELECT point_dep, class FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    if(Qry.FieldIsNULL("class"))
        throw UserException("Для багажа без сопровождения посадочный талон не печатается.");
    int point_id = Qry.FieldAsInteger("point_dep");
    string cl = Qry.FieldAsString("class");
    Qry.Clear();
    Qry.SQLText =
        "SELECT bp_type FROM trip_bp "
        "WHERE point_id=:point_id AND (class IS NULL OR class=:class) "
        "ORDER BY class ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("class", otString, cl);
    Qry.Execute();
    if(Qry.Eof) throw UserException("На рейс или класс не назначен бланк посадочных талонов");
    string form_type = Qry.FieldAsString("bp_type");
    Qry.Clear();

    if(dev_model.empty()) {
        Qry.SQLText =
            "select "
            "   prn_formats.code "
            "from "
            "   prn_types, "
            "   prn_formats "
            "where "
            "   prn_types.code = :prn_type and "
            "   prn_types.format = prn_formats.id ";
        Qry.CreateVariable("prn_type", otInteger, prn_type);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("fmt_type not found for prn_type %d", prn_type);
        fmt_type = Qry.FieldAsString("code");
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
        Qry.CreateVariable("bp_type", otString, form_type);
        Qry.CreateVariable("prn_type", otInteger, prn_type);
    } else {
        Qry.SQLText =
            "select "
            "   prn_form_vers.form, "
            "   prn_form_vers.data "
            "from "
            "   bp_models, "
            "   prn_form_vers "
            "where "
            "   bp_models.form_type = :form_type and "
            "   bp_models.dev_model = :dev_model and "
            "   bp_models.fmt_type = :fmt_type and "
            "   bp_models.id = prn_form_vers.id and "
            "   bp_models.version = prn_form_vers.version ";
        Qry.CreateVariable("form_type", otString, form_type);
        Qry.CreateVariable("dev_model", otString, dev_model);
        Qry.CreateVariable("fmt_type", otString, fmt_type);
    }
    Qry.Execute();
    if(Qry.Eof||Qry.FieldIsNULL("data")||
    	 Qry.FieldIsNULL( "form" ) && (fmt_type == "BTP" || fmt_type == "ATB" || fmt_type == "EPL2")
    	)
      throw UserException("Печать пос. талона на выбранный принтер не производится");
    string form = Qry.FieldAsString("form");
    string data = Qry.FieldAsString("data");
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    NewTextChild(BPNode, "pectab", form);
    Qry.Clear();
    if(pax_id == NoExists) {
        if(pr_all)
            Qry.SQLText =
                "select pax_id, grp_id, reg_no from pax where grp_id = :grp_id and refuse is null order by reg_no";
        else
            Qry.SQLText =
                "select pax.pax_id, pax.grp_id, pax.reg_no  from "
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
    } else {
        Qry.SQLText = "select grp_id, pax_id, reg_no from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
    }
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    while(!Qry.Eof) {
        int pax_id = Qry.FieldAsInteger("pax_id");
        int grp_id = Qry.FieldAsInteger("grp_id");

        TSegPaxList seg_pax_list;
        seg_pax_list.get(pax_id);
        if(seg_pax_list.items.empty()) {
            TSegPax item;
            item.seg_no = 0;
            item.grp_id = grp_id;
            item.pax_id = pax_id;
            seg_pax_list.items.push_back(item);
        }

        for(vector<TSegPax>::iterator iv = seg_pax_list.items.begin(); iv != seg_pax_list.items.end(); iv++)  {

            PrintDataParser parser(iv->grp_id, iv->pax_id, pr_lat, clientDataNode);
            string prn_form = parser.parse(data);
            if(fmt_type == "EPSON") {
                TPrnType convert_prn_type;
                if(dev_model.empty())
                    convert_prn_type = TPrnType(prn_type);
                else {
                    if(dev_model == "OLIVETTI")
                        convert_prn_type = ptOLIVETTI;
                    else if(dev_model == "ML390")
                        convert_prn_type = ptOKIML390;
                    else if(dev_model == "ML3310")
                        convert_prn_type = ptOKIML3310;
                    else
                        throw Exception(dev_model + " not supported by to_esc::convert");
                }
                to_esc::convert(prn_form, convert_prn_type, NULL);
                prn_form = b64_encode(prn_form.c_str(), prn_form.size());
            }
            xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
            NewTextChild(paxNode, "prn_form", prn_form);
            {
                TQuery *Qry = parser.get_prn_qry();
                TDateTime time_print = NowUTC();
                Qry->CreateVariable("now_utc", otDate, time_print);
                Qry->Execute();
                SetProp(paxNode, "pax_id", iv->pax_id);
                SetProp(paxNode, "seg_no", iv->seg_no);
                SetProp(paxNode, "reg_no", parser.GetTagAsInteger("reg_no"));
                SetProp(paxNode, "time_print", DateTimeToStr(time_print));
            }
        }
        Qry.Next();
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(dataNode->doc).c_str());
}


void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
