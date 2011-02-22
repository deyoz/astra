#include <fstream>
#include <set>
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
#include "dev_utils.h"
#include "term_version.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;
using namespace ASTRA;
//using namespace StrUtils;


const string STX = "\x2";
const string CR = "\xd";
const string LF = "\xa";
const string TAB = "\x9";
const string delim = "\xb";

typedef enum {pfBTP, pfATB, pfEPL2, pfEPSON, pfZEBRA, pfDATAMAX} TPrnFormat;

void TPrnParams::get_prn_params(xmlNodePtr reqNode)
{
    if(not reqNode)
        ProgTrace(TRACE5, "get_prn_params: reqNode NULL; use default params");
    if(reqNode == NULL) return;
    encoding = "CP866";
    offset = 20;
    top = 0;
    xmlNodePtr currNode = reqNode->children;
    pr_lat = NodeAsIntegerFast("pr_lat", currNode, NoExists);
    if(reqNode) {
        xmlNodePtr prnParamsNode = GetNode("prnParams", reqNode);
        if(prnParamsNode) {
            currNode = prnParamsNode->children;
            encoding = NodeAsStringFast("encoding", currNode);
            offset = NodeAsIntegerFast("offset", currNode);
            top = NodeAsIntegerFast("top", currNode);
            if(pr_lat == NoExists)
                pr_lat = NodeAsIntegerFast("pr_lat", currNode, 0);
        }
    }
}

namespace to_esc {
    struct TConvertParams {
        private:
            string dev_model;
            TQuery Qry;
            void Exec(string name);
            double AsFloat(string name);
            int AsInteger(string name);
            string AsString(string name);
        public:
            double x_modif, y_modif;
            TConvertParams(): Qry(&OraSession), x_modif(0), y_modif(0)
            {
                Qry.SQLText =
                    "select "
                    "  param_value "
                    "from "
                    "  dev_model_fmt_params "
                    "where "
                    "  (dev_model = :dev_model or dev_model is null) and "
                    "  fmt_type = :fmt_type and "
                    "  param_name = :param_name and "
                    "  (desk_grp_id = :desk_grp_id or desk_grp_id is null) "
                    "order by "
                    "  dev_model nulls last, "
                    "  desk_grp_id nulls last ";
                Qry.DeclareVariable("dev_model", otString);
                Qry.CreateVariable("fmt_type", otString, EncodeDevFmtType(dftEPSON));
                Qry.DeclareVariable("param_name", otString);
                Qry.CreateVariable("desk_grp_id", otInteger, TReqInfo::Instance()->desk.grp_id);
            };
            void init(TPrnType t); // Старый терминал
            void init(string dev_model);
            void dump()
            {
                ProgTrace(TRACE5, "TConvertParams::dump()");
                ProgTrace(TRACE5, "x_modif: %.2f", x_modif);
                ProgTrace(TRACE5, "y_modif: %.2f", y_modif);
            }
    };

    void TConvertParams::Exec(string name)
    {
        Qry.SetVariable("dev_model", dev_model);
        Qry.SetVariable("param_name", name);
        Qry.Execute();
        if(Qry.Eof)
            throw Exception("Параметр %s не найден для модели устройства %s.", name.c_str(), dev_model.c_str());
    }

    double TConvertParams::AsFloat(string name)
    {
        Exec(name);
        return Qry.FieldAsFloat("param_value");
    }

    string TConvertParams::AsString(string name)
    {
        Exec(name);
        return Qry.FieldAsString("param_value");
    }

    int TConvertParams::AsInteger(string name)
    {
        Exec(name);
        return Qry.FieldAsInteger("param_value");
    }

    void TConvertParams::init(string dev_model)
    {
        ProgTrace(TRACE5, "TConvertParams::init");
        this->dev_model = dev_model;
        x_modif = AsFloat("x_modif");
        y_modif = AsFloat("y_modif");
    }

    void TConvertParams::init(TPrnType prn_type)
    {
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
                throw Exception("to_esc::TConvertParams::init: unknown prn_type " + IntToString(prn_type));
        }
    }

    typedef struct {
        int x, y, font;
        int len, height, rotation;
        string align;
        string data;
        void dump();
        void parse_data();
    } TField;

    void TField::dump()
    {
        ProgTrace(TRACE5, "TField::dump()");
        ProgTrace(TRACE5, "x: %d", x);
        ProgTrace(TRACE5, "y: %d", y);
        ProgTrace(TRACE5, "font: %d", font);
        ProgTrace(TRACE5, "len: %d", len);
        ProgTrace(TRACE5, "height: %d", height);
        ProgTrace(TRACE5, "rotation: %d", rotation);
        ProgTrace(TRACE5, "align: %s", align.c_str());
        ProgTrace(TRACE5, "data: %s", data.c_str());
        ProgTrace(TRACE5, "--------------------");
    }

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

    void parse_dmx(string &prn_form)
    {
        prn_form = STX + prn_form;
        size_t pos = 0;
        while(true) {
            pos = prn_form.find(LF);
            if(pos == string::npos)
                break;
            prn_form.erase(pos, 1);
        }
    }

    void convert(string &mso_form, const TConvertParams &ConvertParams, const TPrnParams &prnParams)
    {
        char Mode = 'S';
        TFields fields;
        string num;
        int x, y, font, prnParamsOffset, prnParamsTop;

        try {
            mso_form = ConvertCodepage( mso_form, "CP866", prnParams.encoding );
        } catch(EConvertError &E) {
            ProgError(STDLOG, E.what());
            throw AstraLocale::UserException("MSG.CONVERT_INTO_ERR", LParams() << LParam("enc", prnParams.encoding));
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
                    if(curr_char == CR[0]) {
                        field.x = x + prnParamsOffset;
                        field.y = y + prnParamsTop;
                        field.font = font;
                        field.data = num;
                        field.parse_data();
                        if(field.font == 'B' && field.data.size() != 10)
                            throw Exception("barcode data len must be 10: %s", field.data.c_str());
                        fields.push_back(field);
                        num.erase();
                        Mode = 'C';
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
                case 'C':
                    if(curr_char == LF[0])
                        Mode = 'S';
                    else
                        throw Exception("to_esc: LF not found where expected");
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
                int offset_y = int(delta_y * ConvertParams.y_modif);
                int y256 = offset_y / 256;
                int y_reminder = offset_y % 256;

                for(int i = 0; i < y256; i++) {
                    mso_form += "\x1bJ\xff";
                }
                mso_form += "\x1bJ";
                if(y_reminder) mso_form += char(y_reminder - 1);
                curr_y = fi->y;
            }


            int offset_x = int(fi->x * ConvertParams.x_modif);
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
    string tag_lang;

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
                        if(buf == "e") {
                            FieldLat = 1;
                            tag_lang = "E";
                        } else {
                            FieldLat = 0;
                            tag_lang = "R";
                        }
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
            pts.get_field(FieldName, 0, "L", DateFormat, tag_lang);
    else
        result =
            pts.get_field(FieldName, ToInt(FieldLen), FieldAlign, DateFormat, tag_lang);
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
    string tag_lang;

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
                                    tag_lang = "E";
                                    break;
                                case 'R':
                                    FieldLat = 0;
                                    tag_lang = "R";
                                    break;
                                default:
                                    throw Exception("4th param must be one of R or E at " + IntToString(offset + i + 1));
                            }
                            buf = buf.substr(0, buf.size() - 2);
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
    return pts.get_field(FieldName, FieldLen, FieldAlign, DateFormat, tag_lang);
}

string PrintDataParser::parse(string &form)
{
    string result;
    char Mode = 'S';
    string::size_type VarPos = 0;
    string::size_type i = 0;
    if(form.substr(i, 3) == "1" + CR + LF) {
        i += 3;
        pectab_format = 1;
    } else
        pectab_format = 0;
    if(form.substr(i, 2) == "XX") {
        i += 2;
        pts.set_print_mode(1);
    } else if(form.substr(i, 1) == "X") {
        i += 1;
        pts.set_print_mode(2);
    } else if(form.substr(i, 1) == "S") {
        i += 1;
        pts.set_print_mode(3);
    } else
        pts.set_print_mode(0);
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
    {
        if(Mode == 'R') {
                --i;
                result += parse_tag(VarPos, form.substr(VarPos, i - VarPos));
        } else {
            int piece_len = form.size() < 10 ? form.size() : 10;
            throw Exception("Mode: %c, ERR STR: %s, sym: '%c'", Mode, form.substr(i - piece_len, piece_len).c_str(), form[i]);
        }
    }
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
        if(pts.tag_lang.GetLang() != AstraLocale::LANG_RU) {
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

string get_fmt_type(int prn_type);

    namespace AdjustCR_LF {
        string delete_all_CR_LF(string data)
        {
            string result;
            for(string::iterator i = data.begin(); i != data.end(); i++) {
                if(*i == CR[0] or *i == LF[0])
                    continue;
                result += *i;
            }
            return TrimString(result);
        }

        string place_CR_LF(string data)
        {
            size_t pos = 0;
            while(true) {
                pos = data.find(LF, pos);
                if(pos == string::npos)
                    break;
                else {
                    if(pos == 0 or data[pos - 1] != CR[0]) {
                        data.replace(pos, 1, CR + LF);
                        pos += 2;
                    } else
                        pos += 1;
                }
            }
            if(not data.empty()) {
                while(true) { // удалим все пробелы, TAB и CR/LF из конца строки
                    size_t last_ch = data.size() - 1;
                    if(
                            data[last_ch] == CR[0] or
                            data[last_ch] == LF[0] or
                            data[last_ch] == TAB[0] or
                            data[last_ch] == ' '
                      )
                        data.erase(last_ch);
                    else
                        break;
                }
                //и добавим один CR/LF
                data += CR + LF;
            }
            return data;
        }

        string DoIt(TDevFmtType fmt_type, string data)
        {
            string result;
            switch(fmt_type) {
                case dftTEXT:
                case dftFRX:
                    result = data;
                    break;
                case dftATB:
                case dftBTP:
                case dftZPL2:
                    result = delete_all_CR_LF(data);
                    break;
                case dftEPL2:
                case dftZEBRA:
                case dftDPL:
                case dftEPSON:
                    result = place_CR_LF(data);
                    break;
                case dftUnknown:
                    throw Exception("AdjustCR_LF: unknown fmt_type");
            }
            return result;
        }

        string DoIt(string fmt_type, string data)
        {
            return DoIt(DecodeDevFmtType(fmt_type), data);
        }
    };

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
        "   prn_form_vers.form IS NOT NULL";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bp_forms");
    for(;!Qry.Eof;Qry.Next())
      NewTextChild(formNode,"form",AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("form")));
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
        NewTextChild(formNode,"form",AdjustCR_LF::DoIt(get_fmt_type(prn_type), Qry.FieldAsString("form")));
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
        "   bt_models.form_type IN (SELECT DISTINCT tag_type FROM trip_bt WHERE point_id=:point_id) and "
        "   bt_models.dev_model = :dev_model and "
        "   bt_models.fmt_type = :fmt_type and "
        "   bt_models.id = prn_form_vers.id and "
        "   bt_models.version = prn_form_vers.version and "
        "   prn_form_vers.form IS NOT NULL";;
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

void previewDeviceSets(bool conditional, string msg)
{
 /* if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION))
  {
    xmlNodePtr resNode=NodeAsNode("/term/answer",getXmlCtxt()->resDoc);
    if (conditional)
      NewTextChild(resNode,"preview_device_sets","conditional");
    else
      NewTextChild(resNode,"preview_device_sets");
    showErrorMessageAndRollback(msg);
  }
  else*/
    throw AstraLocale::UserException(msg);
};

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
      previewDeviceSets(true, getLocaleText("MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE"));
    while(!FormsQry.Eof)
    {
        NewTextChild(pectabsNode, "pectab", AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("form")));
        if (FormsQry.FieldIsNULL("data"))
          previewDeviceSets(true, getLocaleText("MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE"));
        prn_forms.push_back(AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("data")));
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
      previewDeviceSets(true, getLocaleText("MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE"));
    string fmt_type = get_fmt_type(prn_type);
    while(!FormsQry.Eof)
    {
        NewTextChild(pectabsNode, "pectab", AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("form")));
        if (FormsQry.FieldIsNULL("data"))
          previewDeviceSets(true, getLocaleText("MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE"));
        prn_forms.push_back(AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("data")));
        FormsQry.Next();
    };
}

void DumpRoute(TBTRoute &route)
{
    ProgTrace(TRACE5, "-----------DUMP ROUTE-----------");
    for(TBTRoute::iterator iv = route.begin(); iv != route.end(); ++iv) {
        ProgTrace(TRACE5, "airline: %s", iv->airline.c_str());
        ProgTrace(TRACE5, "flt_no: %d", iv->flt_no);
        ProgTrace(TRACE5, "suffix: %s", iv->suffix.c_str());
        ProgTrace(TRACE5, "airp_dep: %s", iv->airp_dep.c_str());
        ProgTrace(TRACE5, "airp_arv: %s", iv->airp_arv.c_str());
        ProgTrace(TRACE5, "scd: %s", DateTimeToStr(iv->scd, ServerFormatDateTimeAsString).c_str());
        ProgTrace(TRACE5, "-----------RouteItem-----------");
    }
}

void set_via_fields(PrintDataParser &parser, TBTRoute &route, int start_idx, int end_idx)
{
    int via_idx = 1;
    for(int j = start_idx; j < end_idx; ++j) {
        string str_via_idx = IntToString(via_idx);
        ostringstream flt_no;
        flt_no << setw(3) << setfill('0') << route[j].flt_no;

        parser.pts.set_tag("flt_no" + str_via_idx, flt_no.str() + route[j].suffix);
        parser.pts.set_tag("local_date" + str_via_idx, route[j].scd);
        parser.pts.set_tag("airline" + str_via_idx, route[j].airline);
        parser.pts.set_tag("airp_arv" + str_via_idx, route[j].airp_arv);
        parser.pts.set_tag("fltdate" + str_via_idx, route[j].scd);
        parser.pts.set_tag("airp_arv_name" + str_via_idx, route[j].airp_arv);


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

void get_route(TTagKey &tag_key, TBTRoute &route, string airp_dep)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  "
        "   nvl(points.est_out, points.scd_out) scd,  "
        "   points.airline,  "
        "   points.flt_no,  "
        "   points.suffix,  "
        "   pax_grp.airp_dep,  "
        "   pax_grp.airp_arv,  "
        "   0 transfer_num "
        "from  "
        "   pax_grp,  "
        "   points  "
        "where  "
        "   pax_grp.grp_id = :grp_id and  "
        "   pax_grp.point_dep = points.point_id and points.pr_del>=0 "
        "union  "
        "select  "
        "   trfer_trips.scd,  "
        "   trfer_trips.airline,  "
        "   trfer_trips.flt_no,  "
        "   trfer_trips.suffix,  "
        "   trfer_trips.airp_dep, "
        "   transfer.airp_arv,  "
        "   transfer.transfer_num "
        "from  "
        "   transfer,  "
        "   trfer_trips "
        "where  "
        "   transfer.point_id_trfer = trfer_trips.point_id and "
        "   transfer.grp_id = :grp_id "
        "order by "
        "   transfer_num ";
    Qry.CreateVariable("grp_id", otInteger, tag_key.grp_id);
    Qry.Execute();
    while(!Qry.Eof) {
        TBTRouteItem RouteItem;
        RouteItem.airline = Qry.FieldAsString("airline");
        RouteItem.flt_no = Qry.FieldAsInteger("flt_no");
        RouteItem.suffix = Qry.FieldAsString("suffix");
        RouteItem.airp_dep = Qry.FieldAsString("airp_dep");
        RouteItem.airp_arv = Qry.FieldAsString("airp_arv");
        RouteItem.scd = UTCToLocal(Qry.FieldAsDateTime("scd"), AirpTZRegion(airp_dep));
        route.push_back(RouteItem);

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
            throw AstraLocale::UserException("MSG.TERM_VERSION_OUTDATED.REFRESH_DATA");
        prn_type = NoExists;
    }
}

string get_fmt_type(int prn_type)
{
    TQuery Qry(&OraSession);
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
    return Qry.FieldAsString("code");
}

void big_test(PrintDataParser &parser, TDevOperType op_type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   id, "
        "   version, "
        "   connect_string, "
        "   fmt_type, "
        "   data "
        "from "
        "   drop_all_pectabs "
        "where "
        "   op_type = :op_type "
//        "   and connect_string not in ('beta/beta@beta') "
//        "   and connect_string = 'beta/beta@beta' "
        "order by "
        "   connect_string, "
        "   op_type, "
        "   fmt_type, "
        "   id, "
        "   version";
    Qry.CreateVariable("op_type", otString, EncodeDevOperType(op_type));
    Qry.Execute();
    ofstream out("check_parse");
    for(; not Qry.Eof; Qry.Next()) {
        int id = Qry.FieldAsInteger("id");
        int version = NoExists;
        if(not Qry.FieldIsNULL("version"))
            version = Qry.FieldAsInteger("version");
        string connect_string = Qry.FieldAsString("connect_string");
        if(
                false and
                not (
                    id == 173941 and
                    version == 1 and
                    connect_string == "stand/llrkxwsp@stand"
                    )
          )
            continue;
        string data = AdjustCR_LF::DoIt(Qry.FieldAsString("fmt_type"), Qry.FieldAsString("data"));
        string parse_result;
        ostringstream idx;
        idx
            << "id: " << id << " "
            << "version: " << version << " "
            << "connect_string: '" << connect_string << "'";
        try {
            parse_result = parser.parse(data);
        } catch(Exception E) {
            out
                << "parse failed: "
                << idx.str()
                << " err msg: " << E.what()
                << endl
                << "err data:" << endl
                << data << endl;
            continue;
        }
        string result;
        for(string::iterator is = parse_result.begin(); is != parse_result.end(); is++) {
            if(*is == '#')
                result += "#\n";
            else
                result += *is;
        }
        out << idx.str() << endl << result << endl;
    }
}

void GetPrintDataBT(xmlNodePtr dataNode, TTagKey &tag_key)
{
//    check_CUTE_certified(tag_key.prn_type, tag_key.dev_model, tag_key.fmt_type);
    TBTRoute route;
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp_dep, class, ckin.get_main_pax_id(:grp_id,0) AS pax_id FROM pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("GRP_ID", otInteger, tag_key.grp_id);
    Qry.Execute();
    if (Qry.Eof)
      throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    get_route(tag_key, route, Qry.FieldAsString("airp_dep"));
    ProgTrace(TRACE5, "route.size(): %d", route.size()); //!!!
    bool pr_unaccomp = Qry.FieldIsNULL("class");
    int pax_id=NoExists;
    if(!pr_unaccomp)
    {
      if (Qry.FieldIsNULL("pax_id"))
        throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
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
        "   bag2.bag_type "
        "from "
        "   bag_tags, "
        "   bag2 "
        "where "
        "   bag_tags.grp_id=bag2.grp_id and "
        "   bag_tags.bag_num=bag2.num and "
        "   bag_tags.grp_id=:grp_id and bag_tags.num=:tag_num ";
    unaccQry.CreateVariable("grp_id",otInteger,tag_key.grp_id);
    unaccQry.DeclareVariable("tag_num",otInteger);

    while(!Qry.Eof) {
        string tmp_tag_type = Qry.FieldAsString("tag_type");
        if(tag_type != tmp_tag_type) {
            tag_type = tmp_tag_type;
            if(tag_key.dev_model.empty()) {
                tag_key.fmt_type = get_fmt_type(tag_key.prn_type);
                get_bt_forms(tag_type, tag_key.prn_type, pectabsNode, prn_forms);
            } else
                get_bt_forms(tag_type, tag_key.dev_model, tag_key.fmt_type, pectabsNode, prn_forms);
        }

        u_int64_t tag_no = (u_int64_t)Qry.FieldAsFloat("no");
        int aircode = tag_no / 1000000;
        int no = tag_no % 1000000;

        xmlNodePtr tagNode = NewTextChild(tagsNode, "tag");
        SetProp(tagNode, "color", Qry.FieldAsString("color"));
        SetProp(tagNode, "type", tag_type);
        SetProp(tagNode, "no", tag_no);

        PrintDataParser parser(tag_key.grp_id, pax_id, tag_key.pr_lat, NULL, &route);

        parser.pts.set_tag(TAG::AIRCODE, aircode);
        parser.pts.set_tag(TAG::NO, no);
        parser.pts.set_tag(TAG::ISSUED, issued);
        parser.pts.set_tag(TAG::BT_AMOUNT, Qry.FieldAsInteger("bag_amount"));
        parser.pts.set_tag(TAG::BT_WEIGHT, Qry.FieldAsInteger("bag_weight"));

        bool pr_liab = Qry.FieldAsInteger("pr_liab_limit") != 0;

        parser.pts.set_tag("liab_limit", upperc((pr_liab ? "Огр. ответственности" : "")));

        if(pr_unaccomp) {
            unaccQry.SetVariable("tag_num",Qry.FieldAsInteger("num"));
            unaccQry.Execute();
            if (!unaccQry.Eof)
            {
                parser.pts.set_tag(TAG::SURNAME, unaccQry.FieldAsInteger("bag_type"));
                parser.pts.set_tag(TAG::FULLNAME, unaccQry.FieldAsInteger("bag_type"));
            } else {
                string print_name="БЕЗ СОПРОВОЖДЕНИЯ";
                parser.pts.set_tag(TAG::SURNAME, print_name);
                parser.pts.set_tag(TAG::FULLNAME, print_name);
            }
        }

        int VIA_num = prn_forms.size();
        int route_size = route.size();
        int BT_count = route_size / VIA_num;
        int BT_reminder = route_size % VIA_num;

        for(int i = 0; i < BT_count; ++i) {
            set_via_fields(parser, route, i * VIA_num, (i + 1) * VIA_num);
//!!!            big_test(parser, dotPrnBT);
            string prn_form = parser.parse(prn_forms.back());
            if(DecodeDevFmtType(tag_key.fmt_type) == dftDPL) {
              if (!reqInfo->desk.compatible(NEW_TERM_VERSION)) {
                to_esc::parse_dmx(prn_form);
                prn_form = StrUtils::b64_encode(prn_form.c_str(), prn_form.size());
              }
            }
            SetProp(NewTextChild(tagNode, "prn_form", prn_form),"hex",(int)false);
        }

        if(BT_reminder) {
            set_via_fields(parser, route, route_size - BT_reminder, route_size);
            string prn_form = parser.parse(prn_forms[BT_reminder - 1]);
            if(DecodeDevFmtType(tag_key.fmt_type) == dftDPL) {
              if (!reqInfo->desk.compatible(NEW_TERM_VERSION)) {
                to_esc::parse_dmx(prn_form);
                prn_form = StrUtils::b64_encode(prn_form.c_str(), prn_form.size());
              }
            }
            SetProp(NewTextChild(tagNode, "prn_form", prn_form),"hex",(int)false);
        }
        Qry.Next();
    }
}

void PrintInterface::ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    TTagKey tag_key;
    tag_key.grp_id = NodeAsInteger("grp_id", reqNode);
    tag_key.dev_model = NodeAsString("dev_model", reqNode, "");
    tag_key.fmt_type = NodeAsString("fmt_type", reqNode, "");
    tag_key.prn_type = NodeAsInteger("prn_type", reqNode, NoExists);
    tag_key.pr_lat = NodeAsInteger("pr_lat", reqNode, NoExists);
    if(tag_key.pr_lat == NoExists) {
        TPrnParams prnParams;
        prnParams.get_prn_params(reqNode);
        tag_key.pr_lat = prnParams.pr_lat;
    }
    tag_key.type = NodeAsString("type", reqNode);
    tag_key.color = NodeAsString("color", reqNode);
    tag_key.no = NodeAsFloat("no", reqNode);
    if(tag_key.prn_type == NoExists and tag_key.dev_model.empty())
      previewDeviceSets(false, getLocaleText("MSG.PRINTER_NOT_SPECIFIED"));
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
    tag_key.pr_lat = NodeAsInteger("pr_lat", reqNode, NoExists);
    if(tag_key.pr_lat == NoExists) {
        TPrnParams prnParams;
        prnParams.get_prn_params(reqNode);
        tag_key.pr_lat = prnParams.pr_lat;
    }
    if(tag_key.prn_type == NoExists and tag_key.dev_model.empty())
      previewDeviceSets(false, getLocaleText("MSG.PRINTER_NOT_SPECIFIED"));
    GetPrintDataBT(dataNode, tag_key);
}

void PrintInterface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery PaxQry(&OraSession);
    PaxQry.SQLText =
      "SELECT pax.pax_id, pax.grp_id, surname||' '||name fullname, reg_no,  "
      " salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,'list',NULL,0) seat_no,  "
      " point_dep point_id  "
      "FROM pax, pax_grp  "
      "WHERE pax_id=:pax_id AND pax.tid=:tid and  "
      " pax.grp_id = pax_grp.grp_id ";
    PaxQry.DeclareVariable("pax_id",otInteger);
    PaxQry.DeclareVariable("tid",otInteger);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "update bp_print set pr_print = 1 where pax_id = :pax_id and time_print = :time_print and pr_print = 0";
    Qry.DeclareVariable("pax_id", otInteger);
    Qry.DeclareVariable("time_print", otDate);
    xmlNodePtr curNode = NodeAsNode("passengers/pax", reqNode);
    for(; curNode != NULL; curNode = curNode->next) {
        PaxQry.SetVariable("pax_id", NodeAsInteger("@pax_id", curNode));
        PaxQry.SetVariable("tid", NodeAsInteger("@tid", curNode));
        PaxQry.Execute();
        if(PaxQry.Eof)
            throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        Qry.SetVariable("pax_id", NodeAsInteger("@pax_id", curNode));
        Qry.SetVariable("time_print", NodeAsDateTime("@time_print", curNode));
        Qry.Execute();
        if (Qry.RowsProcessed()==0)
            throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        string seat_no = PaxQry.FieldAsString("seat_no");
        string msg =
                (string)"Напечатан пос. талон для " + PaxQry.FieldAsString("fullname") +
                ". Рег. номер: " + IntToString(PaxQry.FieldAsInteger("reg_no")) +
                ". Место: " + (seat_no.empty() ? "нет" : seat_no) + ".";
        ProgTrace(TRACE5, "CONFIRM PRINT_BP LOG MSG: %s", msg.c_str());
        TReqInfo::Instance()->MsgToLog(
                msg,
                ASTRA::evtPax,
                PaxQry.FieldAsInteger("point_id"),
                PaxQry.FieldAsInteger("reg_no"),
                PaxQry.FieldAsInteger("grp_id")
                );
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
            throw AstraLocale::UserException("MSG.LUGGAGE.CHANGE_FROM_OTHER_DESK_REFRESH");
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
    if(Qry.Eof) throw AstraLocale::UserException("MSG.PRINTERS_NOT_FOUND");
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
}

void PrintInterface::GetPrintDataBR(string &form_type, PrintDataParser &parser,
        string &Print, bool &hex, xmlNodePtr reqNode)
{
//!!!    big_test(parser, dotPrnBR);
    xmlNodePtr currNode = reqNode->children;
    int prn_type = NodeAsIntegerFast("prn_type", currNode, NoExists);
    string dev_model = NodeAsStringFast("dev_model", currNode, "");
    string fmt_type = NodeAsStringFast("fmt_type", currNode, "");
    if(prn_type == NoExists and dev_model.empty())
        previewDeviceSets(false, getLocaleText("MSG.PRINTER_NOT_SPECIFIED"));

    TPrnParams prnParams;
    prnParams.get_prn_params(reqNode);

    TQuery Qry(&OraSession);
    if(dev_model.empty()) {
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
        fmt_type = get_fmt_type(prn_type);
    } else {
        Qry.SQLText =
            "select "
            "   prn_form_vers.form, "
            "   prn_form_vers.data "
            "from "
            "   br_models, "
            "   prn_form_vers "
            "where "
            "   br_models.form_type = :form_type and "
            "   br_models.dev_model = :dev_model and "
            "   br_models.fmt_type = :fmt_type and "
            "   br_models.id = prn_form_vers.id and "
            "   br_models.version = prn_form_vers.version ";
        Qry.CreateVariable("form_type", otString, form_type);
        Qry.CreateVariable("dev_model", otString, dev_model);
        Qry.CreateVariable("fmt_type", otString, fmt_type);
    }
    Qry.Execute();
    if(Qry.Eof||Qry.FieldIsNULL("data"))
        previewDeviceSets(true, getLocaleText("MSG.PRINT.RECEIPT_UNAVAILABLE_FOR_THIS_DEVICE"));

    string mso_form = AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("data"));
    mso_form = parser.parse(mso_form);
    to_esc::TConvertParams ConvertParams;
    if ( dev_model.empty() )
        ConvertParams.init(TPrnType(prn_type)); // !!! Старый терминал
    else
        ConvertParams.init(dev_model);
    to_esc::convert(mso_form, ConvertParams, prnParams);
    TReqInfo *reqInfo = TReqInfo::Instance();
    hex=false;
    if (!reqInfo->desk.compatible(NEW_TERM_VERSION))
      Print = StrUtils::b64_encode(mso_form.c_str(), mso_form.size());
    else
    {
    	StringToHex( mso_form, Print );
    	hex=true;
    };
}


string get_validator(TBagReceipt &rcpt)
{
    tst();
    ostringstream validator;
    string agency, sale_point_city, sale_point;
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
    if (Qry.Eof) throw AstraLocale::UserException("MSG.RECEIPT_PROCESSING_OF_FORM_DENIED_FROM_THIS_DESK", LParams() << LParam("form", rcpt.form_type));
    sale_point=Qry.FieldAsString("sale_point");

    Qry.Clear();
    Qry.SQLText=
        "SELECT "
        "   agency, "
        "   city "
        "from "
        "   sale_points "
        "where "
        "   code = :code and "
        "   validator = :validator ";
    Qry.CreateVariable("code", otString, sale_point);
    Qry.CreateVariable("validator", otString, validator_type);
    Qry.Execute();
    if (Qry.Eof) throw Exception("sale point '%s' not found for validator '%s'", sale_point.c_str(), validator_type.c_str());
    agency = Qry.FieldAsString("agency");
    sale_point_city = Qry.FieldAsString("city");

    Qry.Clear();
    Qry.SQLText =
        "SELECT private_num, agency FROM operators "
        "WHERE login=:login AND validator=:validator AND pr_denial=0";
    Qry.CreateVariable("login",otString,reqInfo->user.login);
    Qry.CreateVariable("validator", otString, validator_type);
    Qry.Execute();
    if(Qry.Eof) throw AstraLocale::UserException("MSG.RECEIPT_PROCESSING_OF_FORM_DENIED_FROM_THIS_USER", LParams() << LParam("form", rcpt.form_type));
    private_num = Qry.FieldAsInteger("private_num");
    ProgTrace(TRACE5, "AGENCIES: %s %s", agency.c_str(), Qry.FieldAsString("agency"));
    if(agency != Qry.FieldAsString("agency")) // Агентство пульта не совпадает с агентством кассира
        throw AstraLocale::UserException("MSG.DESK_AGENCY_NOT_MATCH_THE_USER_ONE");

    TBaseTableRow &city = base_tables.get("cities").get_row("code", sale_point_city);
    TBaseTableRow &country = base_tables.get("countries").get_row("code", city.AsString("country"));
    if(validator_type == "ТКП") {
        // agency
        validator
            << rcpt.tag_lang.ElemIdToTagElem(etAgency, agency, efmtCodeNative)
            << " " << rcpt.tag_lang.ElemIdToTagElem(etValidatorType, validator_type, efmtCodeNative)
            << endl;
        // agency descr
        validator << rcpt.tag_lang.ElemIdToTagElem(etSalePoint, sale_point, efmtNameLong).substr(0, 19)  << endl;
        // agency city
        validator
            << rcpt.tag_lang.ElemIdToTagElem(etCity, sale_point_city, efmtNameLong).substr(0, 16)
            << " "
            << rcpt.tag_lang.ElemIdToTagElem(etCountry, country.AsString("code"), efmtCodeNative)
            << endl;
        // agency code
        validator
            << sale_point << "  "
            << setw(4) << setfill('0') << private_num << endl;
        validator << endl; // empty string for this type
    } else {
        // все валидаторы кроме ТКП у нас пока обрабатываются одинаково
        string desk_city = DeskCity(rcpt.issue_desk, false);
        if(desk_city.empty())
            throw Exception("get_validator: issue_desk not found (code = %s)", rcpt.issue_desk.c_str());
        validator
            << sale_point
            << " "
            << DateTimeToStr(UTCToLocal(rcpt.issue_date, CityTZRegion(desk_city)), "ddmmmyy", rcpt.tag_lang.GetLang() != AstraLocale::LANG_RU)
            << endl;
        validator << rcpt.tag_lang.ElemIdToTagElem(etAgency, agency, efmtNameLong).substr(0, 19) << endl;
        validator << rcpt.tag_lang.ElemIdToTagElem(etSalePoint, sale_point, efmtNameLong).substr(0, 19) << endl;
        validator
            << rcpt.tag_lang.ElemIdToTagElem(etCity, sale_point_city, efmtNameLong).substr(0, 16)
            << "/"
            << rcpt.tag_lang.ElemIdToTagElem(etCountry, country.AsString("code"), efmtCodeNative)
            << endl;
        validator << setw(4) << setfill('0') << private_num << endl;
    }
    return validator.str();
}

struct TPaxPrint {
  int grp_id;
  int pax_id;
  int reg_no;
  TPaxPrint( int vgrp_id, int vpax_id, int vreg_no ) {
    grp_id=vgrp_id;
	  pax_id=vpax_id;
	  reg_no=vreg_no;
	};
};

bool rus_airp(string airp)
{
    string city = base_tables.get("AIRPS").get_row("code", airp).AsString("city");
    return base_tables.get("CITIES").get_row("code", city).AsString("country") == "РФ";
}

bool get_bp_pr_lat(int grp_id, bool pr_lat)
{
    TQuery Qry(&OraSession);
    Qry.SQLText=
        "select airp_dep, airp_arv from pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    return pr_lat or not rus_airp(Qry.FieldAsString("airp_dep")) or not rus_airp(Qry.FieldAsString("airp_arv"));
}

void tst_dump(int pax_id, int grp_id, int pr_lat)
{
    vector<string> tags;
    {
        TPrnTagStore pts(grp_id, pax_id, pr_lat, NULL);
        pts.tst_get_tag_list(tags);
    }
    for(vector<string>::iterator iv = tags.begin(); iv != tags.end(); iv++) {
        TPrnTagStore tmp_pts(grp_id, pax_id, pr_lat, NULL);
        tmp_pts.set_tag("gate", "");
        ProgTrace(TRACE5, "tag: %s; value: '%s'", iv->c_str(), tmp_pts.get_field(*iv, 0, "L", "dd.mm hh:nn", "R").c_str());
        TQuery Qry(&OraSession);
        tmp_pts.get_prn_qry(Qry);
        TDateTime time_print = NowUTC();
        Qry.CreateVariable("now_utc", otDate, time_print);
        Qry.Execute();
    }
}

void PrintInterface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr currNode = reqNode->children;
    int grp_id = NodeAsIntegerFast("grp_id", currNode, NoExists); // grp_id - первого сегмента или ид. группы
    int pax_id = NodeAsIntegerFast("pax_id", currNode, NoExists);
    int pr_all = NodeAsIntegerFast("pr_all", currNode, NoExists);
    int prn_type = NodeAsIntegerFast("prn_type", currNode, NoExists);
    string dev_model = NodeAsStringFast("dev_model", currNode, "");
    string fmt_type = NodeAsStringFast("fmt_type", currNode, "");
    TPrnParams prnParams;
    prnParams.get_prn_params(reqNode);
    xmlNodePtr clientDataNode = NodeAsNodeFast("clientData", currNode);
    if(prn_type == NoExists and dev_model.empty())
      previewDeviceSets(false, getLocaleText("MSG.PRINTER_NOT_SPECIFIED"));

    //    check_CUTE_certified(prn_type, dev_model, fmt_type);

    TQuery Qry(&OraSession);
    if(grp_id == NoExists) {
        Qry.Clear();
        Qry.SQLText="SELECT grp_id from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        grp_id = Qry.FieldAsInteger("grp_id");
    }


    Qry.Clear();
    Qry.SQLText="SELECT point_dep, class FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    if(Qry.FieldIsNULL("class"))
        throw AstraLocale::UserException("MSG.BOARDINGPASS_NOT_AVAILABLE_FOR_UNACC_BAGGAGE");
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
    if(Qry.Eof) throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_FLIGHT_OR_CLASS");
    string form_type = Qry.FieldAsString("bp_type");
    ProgTrace(TRACE5, "bp_type: %s", form_type.c_str());
    Qry.Clear();

    if(dev_model.empty()) {
        fmt_type = get_fmt_type(prn_type);
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
            Qry.FieldIsNULL( "form" ) && (DecodeDevFmtType(fmt_type) == dftBTP || DecodeDevFmtType(fmt_type) == dftATB)
      )
        previewDeviceSets(true, getLocaleText("MSG.PRINT.BP_UNAVAILABLE_FOR_THIS_DEVICE"));
    string form = AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("form"));
    string data = AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("data"));
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    NewTextChild(BPNode, "pectab", form);
    vector<int> grps;
    vector<TPaxPrint> paxs;
    Qry.Clear();
    if ( pax_id == NoExists ) { // печать всех или только тех, у которых не подтверждена распечатка
        Qry.SQLText =
            "SELECT t2.grp_id "
            " FROM tckin_pax_grp t1, tckin_pax_grp t2 "
            " WHERE t1.grp_id=:grp_id AND "
            "       t1.tckin_id=t2.tckin_id AND "
            "       t2.seg_no >= t1.seg_no AND "
            "       ( t2.pr_depend <> 0 OR t2.grp_id=:grp_id ) "
            " ORDER BY t2.seg_no ";
        Qry.CreateVariable( "grp_id", otInteger, grp_id );
        Qry.Execute();
        while ( !Qry.Eof ) {
            grps.push_back( Qry.FieldAsInteger("grp_id") );
            Qry.Next();
        }
        if ( grps.empty() ) { // нет сквозняка - тогда кладем просто группу - grp_id
            ProgTrace( TRACE5, "grps.empty, grp_id=%d", grp_id );
            grps.push_back( grp_id );
        }
        Qry.Clear();
        if ( pr_all )
            Qry.SQLText =
                "SELECT pax_id, grp_id, reg_no "
                " FROM pax "
                "WHERE grp_id = :grp_id AND "
                "      refuse IS NULL "
                "ORDER BY reg_no";
        else
            Qry.SQLText =
                "SELECT pax.pax_id, pax.grp_id, pax.reg_no "
                " FROM pax, bp_print "
                "WHERE  pax.grp_id = :grp_id AND "
                "       pax.refuse IS NULL AND "
                "       pax.pax_id = bp_print.pax_id(+) AND "
                "       bp_print.pr_print(+) <> 0 AND "
                "       bp_print.pax_id IS NULL "
                "ORDER BY pax.reg_no";
        Qry.DeclareVariable( "grp_id", otInteger );
        for( vector<int>::iterator igrp=grps.begin(); igrp!=grps.end(); igrp++ ) {
            Qry.SetVariable( "grp_id", *igrp );
            Qry.Execute();
            if ( Qry.Eof && pr_all ) //!!! анализ на клиенте по сегментам, значит и мы должны анализировать по сегментам
                throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //все посадочные отпечатаны
            while ( !Qry.Eof ) {
                paxs.push_back( TPaxPrint( Qry.FieldAsInteger("grp_id"),
                            Qry.FieldAsInteger("pax_id"),
                            Qry.FieldAsInteger("reg_no") ) );
                Qry.Next();
            }
        }
        if ( !pr_all && paxs.empty() ) //все посадочные отпечатаны, но при этом надо было напечатать те, которые были не напечатанны
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    }
    else { // печать конкретного пассажира
        Qry.SQLText =
            "SELECT grp_id, pax_id, reg_no FROM pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if ( Qry.Eof )
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        paxs.push_back( TPaxPrint( Qry.FieldAsInteger("grp_id"),
                    Qry.FieldAsInteger("pax_id"),
                    Qry.FieldAsInteger("reg_no") ) );
    }
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    TReqInfo *reqInfo = TReqInfo::Instance();
    for (vector<TPaxPrint>::iterator iprint=paxs.begin(); iprint!=paxs.end(); iprint++ ) {
//!!!        tst_dump(iprint->pax_id, iprint->grp_id, prnParams.pr_lat);
        PrintDataParser parser( iprint->grp_id, iprint->pax_id, prnParams.pr_lat, clientDataNode );
//!!!        big_test(parser, dotPrnBP);
        // если это нулевой сегмент, то тогда печатаем выход на посадку иначе не нечатаем
        //надо удалить выход на посадку из данных по пассажиру
        if(grp_id != iprint->grp_id)
            parser.pts.set_tag("gate", "");

        string prn_form = parser.parse(data);
        bool hex=false;
        if(DecodeDevFmtType(fmt_type) == dftEPSON) {
            to_esc::TConvertParams ConvertParams;
            if ( dev_model.empty() )
                ConvertParams.init(TPrnType(prn_type)); // !!! Старый терминал
            else
                ConvertParams.init(dev_model);
            to_esc::convert(prn_form, ConvertParams, prnParams);
            if (!reqInfo->desk.compatible(NEW_TERM_VERSION))
              prn_form = StrUtils::b64_encode(prn_form.c_str(), prn_form.size());
            else
            {
            	StringToHex( string(prn_form), prn_form );
            	hex=true;
            };
        }
        if(DecodeDevFmtType(fmt_type) == dftDPL) {
            if (!reqInfo->desk.compatible(NEW_TERM_VERSION)) {
              to_esc::parse_dmx(prn_form);
              prn_form = StrUtils::b64_encode(prn_form.c_str(), prn_form.size());
            }
        }
        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        SetProp(NewTextChild(paxNode, "prn_form", prn_form),"hex",(int)hex);

        {
            TQuery Qry(&OraSession);
            parser.pts.get_prn_qry(Qry);
            TDateTime time_print = NowUTC();
            Qry.CreateVariable("now_utc", otDate, time_print);
            Qry.Execute();
            SetProp(paxNode, "pax_id", iprint->pax_id);
            //!!!SetProp(paxNode, "seg_no", iv->seg_no);
            SetProp(paxNode, "reg_no", parser.pts.get_tag(TAG::REG_NO));
            SetProp(paxNode, "time_print", DateTimeToStr(time_print));
        }
    }
}

struct TPrnTestsKey {
    string op_type, fmt_type, dev_model;
};

struct TPrnTestCmp {
    bool operator() (const TPrnTestsKey &l, const TPrnTestsKey &r) const
    {
        if(l.op_type == r.op_type)
            if(l.fmt_type == r.fmt_type)
                return l.dev_model < r.dev_model;
            else
                return l.fmt_type < r.fmt_type;
        else
            return l.op_type < r.op_type;
    }
};

typedef set<TPrnTestsKey, TPrnTestCmp> TPrnTests;

struct TOpsItem {
    string dev_model, fmt_type;
    TPrnParams prnParams;
};

struct TOps {
    map<TDevOperType, TOpsItem> items;
    TOps(xmlNodePtr node);
};

TOps::TOps(xmlNodePtr node)
{
    xmlNodePtr currNode = NodeAsNode("ops", node)->children;
    for(; currNode; currNode = currNode->next) {
        TOpsItem opsItem;
        opsItem.dev_model = NodeAsString("dev_model", currNode);
        if(opsItem.dev_model.empty())
            continue;
        opsItem.fmt_type = NodeAsString("fmt_type", currNode);
        opsItem.prnParams.get_prn_params(currNode);
        items[DecodeDevOperType(NodeAsString("@type", currNode))] = opsItem;
    }
}

void PrintInterface::RefreshPrnTests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TOps ops(reqNode);
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "  desk_grp_id, "
        "  op_type, "
        "  fmt_type, "
        "  dev_model, "
        "  form, "
        "  data "
        "from "
        "  prn_tests "
        "where "
        "  desk_grp_id = :desk_grp_id or desk_grp_id is null "
        "order by "
        "  desk_grp_id nulls last, "
        "  op_type, "
        "  fmt_type, "
        "  dev_model ";
    Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
    Qry.Execute();
    if(!Qry.Eof) {
        xmlNodePtr prnTestsNode = NewTextChild(resNode, "prn_tests");
        xmlNodePtr node;
        TPrnTests prn_tests;
        PrintDataParser parser;
        for(; !Qry.Eof; Qry.Next()) {
            TPrnTestsKey item;
            item.op_type = Qry.FieldAsString("op_type");
            item.fmt_type = Qry.FieldAsString("fmt_type");
            item.dev_model = Qry.FieldAsString("dev_model");
            if(prn_tests.find(item) == prn_tests.end()) {
                prn_tests.insert(item);
                string form = AdjustCR_LF::DoIt(item.fmt_type, Qry.FieldAsString("form"));
                string data = AdjustCR_LF::DoIt(item.fmt_type, Qry.FieldAsString("data"));
                TPrnParams prnParams;
                map<TDevOperType, TOpsItem>::iterator oi = ops.items.find(DecodeDevOperType(item.op_type));
                if(
                        oi != ops.items.end()and
                        oi->second.fmt_type == item.fmt_type
                  )
                    prnParams = oi->second.prnParams;
                parser.pts.prn_tag_props.Init(DecodeDevOperType(item.op_type));
                parser.pts.tag_lang.Init(prnParams.pr_lat);
                data = parser.parse(data);
                TDevOperType dev_oper_type = DecodeDevOperType(item.op_type);
                TDevFmtType dev_fmt_type = DecodeDevFmtType(item.fmt_type);
                bool hex=false;
                if(dev_fmt_type == dftEPSON) {
                    to_esc::TConvertParams ConvertParams;
                    ConvertParams.init(item.dev_model);
                    to_esc::convert(data, ConvertParams, prnParams);
                    StringToHex( string(data), data );
                    hex=true;
                }
                xmlNodePtr itemNode = NewTextChild(prnTestsNode, "item");
                NewTextChild(itemNode, "op_type", item.op_type);
                NewTextChild(itemNode, "fmt_type", item.fmt_type);
                NewTextChild(itemNode, "dev_model", item.dev_model, "");
                node=NewTextChild(itemNode, "form", form, "");
                if (node!=NULL) SetProp(node, "hex", (int)hex);
                node=NewTextChild(itemNode, "data", data, "");
                if (node!=NULL)
                {
                  SetProp(node, "hex", (int)hex);
                  if (dev_oper_type == dotPrnBP) {
                    string barcode;
                    if(parser.pts.tag_processed(TAG::PAX_ID))
                        barcode = parser.pts.get_tag(TAG::PAX_ID);
                    if(parser.pts.tag_processed(TAG::BCBP_M_2))
                        barcode += CR + LF + parser.pts.get_tag(TAG::BCBP_M_2);
                    node=NewTextChild(itemNode, "scan", barcode, "");
                    if (node!=NULL) SetProp(node, "hex", (int)false);
                  };
                };
            }
        }
    }
}

void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
