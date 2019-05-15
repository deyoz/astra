#include <fstream>
#include <set>
#include "print.h"
#include "oralib.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_api.h"
#include "iatci_help.h"
#include "iatci.h"
#include "misc.h"
#include "stages.h"
#include "docs/docs_common.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "payment.h"
#include "exceptions.h"
#include "astra_misc.h"
#include "dev_utils.h"
#include "term_version.h"
#include "emdoc.h"
#include "serverlib/str_utils.h"
#include "qrys.h"
#include "sopp.h"
#include "points.h"
#include "telegram.h"
#include "cr_lf.h"
#include "dcs_services.h"

#define NICKNAME "DENIS"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
//using namespace StrUtils;


const string STX = "\x2";
const string delim = "\xb";

typedef enum {pfBTP, pfATB, pfEPL2, pfEPSON, pfZEBRA, pfDATAMAX} TPrnFormat;

void TPrnParams::get_prn_params(xmlNodePtr reqNode)
{
    if(reqNode == NULL) throw Exception("TPrnParams::get_prn_params: reqnode not defined");

    xmlNodePtr prnParamsNode = GetNode("prnParams", reqNode);
    if(prnParamsNode!=NULL) {
        xmlNodePtr currNode = prnParamsNode->children;
        encoding = NodeAsStringFast("encoding", currNode);
        offset = NodeAsIntegerFast("offset", currNode, 20);
        top = NodeAsIntegerFast("top", currNode, 0);
        pr_lat = NodeAsIntegerFast("pr_lat", currNode)!=0;
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
                Qry.CreateVariable("fmt_type", otString, DevFmtTypes().encode(TDevFmt::EPSON));
                Qry.DeclareVariable("param_name", otString);
                Qry.CreateVariable("desk_grp_id", otInteger, TReqInfo::Instance()->desk.grp_id);
            };
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

    void convert(string &mso_form, const TConvertParams &ConvertParams, const TPrnParams &prnParams)
    {
        char Mode = 'S';
        TFields fields;
        string num;
        int x = 0, y = 0, font = 0, prnParamsOffset = 0, prnParamsTop = 0;

        try {
            mso_form = ConvertCodepage( mso_form, "CP866", prnParams.encoding );
        } catch(EConvertError &E) {
            ProgError(STDLOG, "%s", E.what());
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
                if(!IsDigitIsLetter(curr_char) && curr_char != '_') {
                    if(curr_char == '(') {
                        FieldName = upperc(field.substr(0, i));
                        VarPos = i;
                        Mode = '1';
                    } else
                        throw Exception("wrong char in tag name at " + IntToString(offset + i + 1));
                }
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
                            tag_lang = "E";
                        } else {
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
            pts.get_field(FieldName, 0, "", "L", DateFormat, tag_lang);
    else
        result =
            pts.get_field(FieldName, ToInt(FieldLen), "", FieldAlign, DateFormat, tag_lang);
    return result;
}



string PrintDataParser::parse_field0(int offset, string field)
{
    char Mode = 'S';
    string::size_type VarPos = 0;

    string FieldName = upperc(field);
    int FieldLen = 0;
    string FieldAlign = "L";
    string FieldText;
    string DateFormat = ServerFormatDateTimeAsString;
    string tag_lang;
    bool screened_quote = false;

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
                if(!IsDigitIsLetter(curr_char) && curr_char != '_') {
                    if(curr_char == '(') {
                        FieldName = upperc(field.substr(0, i));
                        VarPos = i;
                        Mode = '1';
                    } else
                        throw Exception("wrong char in tag name at " + IntToString(offset + i + 1));
                }
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
            case '4':
                {
                    if(curr_char == '\'') {
                        if(screened_quote)
                            screened_quote = false;
                        else {
                            screened_quote = field.size() - 1 >= i + 1 and field[i + 1]  == '\'';
                            if(not screened_quote) {
                                FieldText = field.substr(VarPos + 1, i - VarPos - 1);
                                boost::replace_all(FieldText, "''", "'");
                                VarPos = i;
                                Mode = '3';
                            }
                        }
                    }
                }
                break;
            case '2':
                if(curr_char == '\'') {
                        VarPos = i;
                        Mode = '4';
                } else if(curr_char != 'R' && curr_char != 'L' && curr_char != 'C') {
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
                                    tag_lang = "E";
                                    break;
                                case 'R':
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

    return pts.get_field(FieldName, FieldLen, FieldText, FieldAlign, DateFormat, tag_lang);
}

string PrintDataParser::parse(const string &form)
{
    pts.get_pectab_tags(form);
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

        string place_LF(string data)
        {
            size_t pos = 0;
            while(true) {
                pos = data.find(CR, pos);
                if(pos == string::npos)
                    break;
                data.erase(pos, 1);
            }
            clean_ending(data, LF);
            return data;
        }

        string DoIt(TDevFmt::Enum fmt_type, string data)
        {
            string result;
            switch(fmt_type) {
                case TDevFmt::TEXT:
                case TDevFmt::FRX:
                    result = data;
                    break;
                case TDevFmt::ATB:
                case TDevFmt::BTP:
                case TDevFmt::ZPL2:
                    result = delete_all_CR_LF(data);
                    break;
                case TDevFmt::EPL2:
                case TDevFmt::DPL:
                case TDevFmt::EPSON:
                    result = place_CR_LF(data);
                    break;
                case TDevFmt::Graphics2D:
                    result = place_LF(data);
                    break;
                case TDevFmt::SCAN1:
                case TDevFmt::SCAN2:
                case TDevFmt::SCAN3:
                case TDevFmt::BCR:
                case TDevFmt::Unknown:
                    throw Exception("AdjustCR_LF: unknown fmt_type");
            }
            return result;
        }

        string DoIt(string fmt_type, string data)
        {
            return DoIt(DevFmtTypes().decode(fmt_type), data);
        }
    };

void GetTripBPPectabs(int point_id, TDevOper::Enum op_type, const string &dev_model, const string &fmt_type, xmlNodePtr node)
{
    if (node==NULL) return;
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "SELECT bp_type, "
        "       class, "
        "       0 AS priority "
        "FROM trip_bp "
        "WHERE point_id=:point_id AND op_type=:op_type "
        "ORDER BY class, priority DESC ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
    Qry.Execute();
    vector<string> bp_types;
    string prior_class;
    for(;!Qry.Eof;Qry.Next())
    {
      if (bp_types.empty() || prior_class!=Qry.FieldAsString("class"))
      {
        const char* bp_type=Qry.FieldAsString("bp_type");
        if (find(bp_types.begin(), bp_types.end(), bp_type)==bp_types.end()) bp_types.push_back(bp_type);
        prior_class=Qry.FieldAsString("class");
      };
    };

    Qry.Clear();
    Qry.SQLText =
        "select "
        "   prn_form_vers.form "
        "from "
        "   bp_models, "
        "   prn_form_vers "
        "where "
        "   bp_models.form_type = :form_type and "
        "   bp_models.op_type = :op_type and "
        "   bp_models.dev_model = :dev_model and "
        "   bp_models.fmt_type = :fmt_type and "
        "   bp_models.id = prn_form_vers.id and "
        "   bp_models.version = prn_form_vers.version and "
        "   prn_form_vers.form IS NOT NULL";
    Qry.DeclareVariable("form_type", otString);
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    xmlNodePtr formNode=NewTextChild(node,"bp_forms");
    for(vector<string>::const_iterator i=bp_types.begin();i!=bp_types.end();i++)
    {
      Qry.SetVariable("form_type", *i);
      Qry.Execute();
      if (Qry.Eof) continue;
      NewTextChild(formNode,"form",AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("form")));
    };
}

void GetTripBTPectabs(int point_id, const string &dev_model, const string &fmt_type, xmlNodePtr node)
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
        "   prn_form_vers.form IS NOT NULL";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    xmlNodePtr formNode=NewTextChild(node,"bt_forms");
    for(;!Qry.Eof;Qry.Next())
        NewTextChild(formNode,"form",Qry.FieldAsString("form"));
}

void previewDeviceSets(bool conditional, string msg)
{
 /* xmlNodePtr resNode=NodeAsNode("/term/answer",getXmlCtxt()->resDoc);
    if (conditional)
      NewTextChild(resNode,"preview_device_sets","conditional");
    else
      NewTextChild(resNode,"preview_device_sets");
    showErrorMessageAndRollback(msg);
  }*/
  throw AstraLocale::UserException(msg);
};

void get_bt_forms(const string &tag_type, const string &dev_model, const string &fmt_type, const xmlNodePtr pectabsNode, vector<string> &prn_forms)
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
      previewDeviceSets(true, "MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE");
    while(!FormsQry.Eof)
    {
        NewTextChild(pectabsNode, "pectab", AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("form")));
        if (FormsQry.FieldIsNULL("data"))
          previewDeviceSets(true, "MSG.PRINT.BAG_TAG_UNAVAILABLE_FOR_THIS_DEVICE");
        prn_forms.push_back(AdjustCR_LF::DoIt(fmt_type, FormsQry.FieldAsString("data")));
        FormsQry.Next();
    };
}

void DumpRoute(const TTrferRoute &route)
{
    ProgTrace(TRACE5, "-----------DUMP ROUTE-----------");
    for(TTrferRoute::const_iterator iv = route.begin(); iv != route.end(); ++iv) {
        ProgTrace(TRACE5, "airline: %s", iv->operFlt.airline.c_str());
        ProgTrace(TRACE5, "flt_no: %d", iv->operFlt.flt_no);
        ProgTrace(TRACE5, "suffix: %s", iv->operFlt.suffix.c_str());
        ProgTrace(TRACE5, "airp_dep: %s", iv->operFlt.airp.c_str());
        ProgTrace(TRACE5, "airp_arv: %s", iv->airp_arv.c_str());
        ProgTrace(TRACE5, "scd: %s",
                  DateTimeToStr(iv->operFlt.act_est_scd_out()==ASTRA::NoExists?iv->operFlt.scd_out:
                                                                               iv->operFlt.act_est_scd_out(), ServerFormatDateTimeAsString).c_str());
        ProgTrace(TRACE5, "-----------RouteItem-----------");
    }
}

void set_via_fields(PrintDataParser &parser, const TTrferRoute &route, int start_idx, int end_idx)
{
    int via_idx = 1;
    for(int j = start_idx; j < end_idx; ++j) {

        string view_airline = route[j].operFlt.airline;
        int view_flt_no = route[j].operFlt.flt_no;
        string view_suffix = route[j].operFlt.suffix;
        TDateTime real_local=route[j].operFlt.act_est_scd_out()==ASTRA::NoExists?route[j].operFlt.scd_out:route[j].operFlt.act_est_scd_out();

        // Франчайз применяем только к первому пункту маршрута
        if(j == 0 and route[j].operFlt.point_id != NoExists) {
            Franchise::TProp franchise_prop;
            franchise_prop.get(route[j].operFlt.point_id, Franchise::TPropType::bt);
            if(franchise_prop.val == Franchise::pvNo) {
                view_airline = franchise_prop.franchisee.airline;
                view_flt_no = franchise_prop.franchisee.flt_no;
                view_suffix = franchise_prop.franchisee.suffix;
            }
        }

        string str_via_idx = IntToString(via_idx);
        ostringstream flt_no;
        flt_no << setw(3) << setfill('0') << view_flt_no;

        parser.pts.set_tag("flt_no" + str_via_idx, flt_no.str() + view_suffix);
        parser.pts.set_tag("local_date" + str_via_idx, real_local);
        parser.pts.set_tag("airline" + str_via_idx, view_airline);
        parser.pts.set_tag("airp_arv" + str_via_idx, route[j].airp_arv);
        parser.pts.set_tag("fltdate" + str_via_idx, real_local);
        parser.pts.set_tag("airp_arv_name" + str_via_idx, route[j].airp_arv);


        ++via_idx;
    }
}

struct TTagKey {
    string dev_model;
    string fmt_type;
    int grp_id;
    bool pr_lat;
    double no; //no = Float!
    string type, color;
    TTagKey(): grp_id(0), pr_lat(false), no(-1.0) {};
};

void big_test(PrintDataParser &parser, TDevOper::Enum op_type)
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
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
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
    ProgTrace(TRACE5, "bt_type: '%s'", tag_key.type.c_str());
    //трансфер
    TTrferRoute route;
    if (!route.GetRoute(tag_key.grp_id, trtWithFirstSeg) || route.empty())
      throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    DumpRoute(route);
    //бирки
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "  pax_grp.class, "
        "  bag_tags.tag_type, "
        "  bag_tags.no, "
        "  bag_tags.color, "
        "  bag2.bag_type, "
        "  bag2.amount AS bag_amount, "
        "  bag2.weight AS bag_weight, "
        "  bag2.pr_liab_limit, "
        "  ckin.get_bag_pool_pax_id(bag_tags.grp_id,NVL(bag2.bag_pool_num,1)) AS pax_id, "
        "  tag_types.printable "
        "FROM "
        "  pax_grp, "
        "  bag_tags, "
        "  tag_types, "
        "  bag2 "
        "WHERE "
        "  bag_tags.grp_id=pax_grp.grp_id AND "
        "  tag_types.code=bag_tags.tag_type AND "
        "  bag_tags.grp_id = bag2.grp_id(+) AND "
        "  bag_tags.bag_num = bag2.num(+) AND "
        "  pax_grp.grp_id = :grp_id AND "
        "  ckin.bag_pool_refused(bag_tags.grp_id, "
        "                        NVL(bag2.bag_pool_num,1), "
        "                        pax_grp.class, "
        "                        pax_grp.bag_refuse)=0 AND ";
    if(tag_key.no >= 0.0) {
        SQLText +=
        "  bag_tags.no = :no AND "
        "  bag_tags.tag_type = :tag_type AND "
        "  NVL(bag_tags.color, ' ') = NVL(:color, ' ') AND "
        "  (tag_types.printable <> 0 OR tag_types.printable IS NULL) ";
        Qry.CreateVariable("tag_type", otString, tag_key.type);
        Qry.CreateVariable("color", otString, tag_key.color);
        Qry.CreateVariable("no", otFloat, tag_key.no);
    } else
        SQLText +=
        "  bag_tags.pr_print = 0 AND "
        "  tag_types.printable <> 0 ";
    SQLText +=
        "ORDER BY "
        "   bag_tags.tag_type, "
        "   bag_tags.num";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("grp_id", otInteger, tag_key.grp_id);
    Qry.Execute();
    //ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    if (Qry.Eof) return;
    if(tag_key.no >= 0.0 && Qry.FieldIsNULL("printable")) //перепечатка специальной бирки
      throw AstraLocale::UserException("MSG.PRINTING_BAGTAG_PROHIBITED");

    bool pr_unaccomp = Qry.FieldIsNULL("class");

    vector<string> prn_forms;
    xmlNodePtr printBTNode = NewTextChild(dataNode, "printBT");
    xmlNodePtr pectabsNode = NewTextChild(printBTNode, "pectabs");
    xmlNodePtr tagsNode = NewTextChild(printBTNode, "tags");

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = UTCToLocal(NowUTC(),reqInfo->desk.tz_region);
    string prior_tag_type;
    while(!Qry.Eof) {
        string tag_type = Qry.FieldAsString("tag_type");
        if(prior_tag_type != tag_type) {
            prior_tag_type = tag_type;
            get_bt_forms(tag_type, tag_key.dev_model, tag_key.fmt_type, pectabsNode, prn_forms);
        }

        u_int64_t tag_no = (u_int64_t)Qry.FieldAsFloat("no");
        int aircode = (tag_no / 1000000) % 1000;
        int no = tag_no % 1000000;

        xmlNodePtr tagNode = NewTextChild(tagsNode, "tag");
        SetProp(tagNode, "type", Qry.FieldAsString("tag_type"));
        SetProp(tagNode, "color", Qry.FieldAsString("color"));
        SetProp(tagNode, "no", FloatToString(Qry.FieldAsFloat("no"),0));

        int pax_id=NoExists;
        if(!pr_unaccomp)
        {
          if (Qry.FieldIsNULL("pax_id"))
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
          pax_id = Qry.FieldAsInteger("pax_id");
        };

        PrintDataParser parser(TDevOper::PrnBT, tag_key.grp_id, pax_id, false, tag_key.pr_lat, NULL, route);

        parser.pts.set_tag(TAG::AIRCODE, aircode);
        parser.pts.set_tag(TAG::NO, no);
        parser.pts.set_tag(TAG::ISSUED, issued);
        parser.pts.set_tag(TAG::BT_AMOUNT, Qry.FieldAsInteger("bag_amount"));
        parser.pts.set_tag(TAG::BT_WEIGHT, Qry.FieldAsInteger("bag_weight"));

        bool pr_liab = Qry.FieldAsInteger("pr_liab_limit") != 0;

        parser.pts.set_tag("liab_limit", upperc((pr_liab ? "Огр. ответственности" : "")));

        if(pr_unaccomp) {
            int bag_type = Qry.FieldIsNULL("bag_type")?NoExists:Qry.FieldAsInteger("bag_type");
            parser.pts.set_tag(TAG::SURNAME, bag_type);
            parser.pts.set_tag(TAG::FULLNAME, bag_type);
        }

        int VIA_num = prn_forms.size();
        int route_size = route.size();
        int BT_count = route_size / VIA_num;
        int BT_reminder = route_size % VIA_num;

        for(int i = 0; i < BT_count; ++i) {
            set_via_fields(parser, route, i * VIA_num, (i + 1) * VIA_num);
//            big_test(parser, TDevOper::PrnBT);
            string prn_form = parser.parse(prn_forms.back());
//            ProgTrace(TRACE5, "prn_form: %s", prn_form.c_str());
            SetProp(NewTextChild(tagNode, "prn_form", prn_form),"hex",(int)false);
        }

        if(BT_reminder) {
            set_via_fields(parser, route, route_size - BT_reminder, route_size);
            string prn_form = parser.parse(prn_forms[BT_reminder - 1]);
//            ProgTrace(TRACE5, "prn_form: %s", prn_form.c_str());
            SetProp(NewTextChild(tagNode, "prn_form", prn_form),"hex",(int)false);
        }
        Qry.Next();
    }
}

bool PrintInterface::BPPax::fromDB(int vpax_id, int test_point_dep)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (test_point_dep==NoExists)
    Qry.SQLText =
        "SELECT pax_grp.grp_id, pax_grp.point_dep, "
        "       pax.reg_no, pax.surname||' '||pax.name full_name "
        "FROM pax_grp, pax "
        "WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND "
        "      pax_grp.status NOT IN ('E')";
  else
    Qry.SQLText =
        "SELECT reg_no, surname||' '||name full_name "
        "FROM test_pax WHERE id=:pax_id";

  Qry.CreateVariable( "pax_id", otInteger, vpax_id );
  Qry.Execute();
  if ( Qry.Eof ) return false;
  pax_id = vpax_id;
  reg_no = Qry.FieldAsInteger( "reg_no" );
  full_name = Qry.FieldAsString( "full_name" );
  if (test_point_dep!=NoExists)
  {
    Qry.Clear();
    Qry.SQLText =
        "SELECT :grp_id AS grp_id, point_id AS point_dep "
        "FROM points "
        "WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable( "grp_id", otInteger, test_point_dep + TEST_ID_BASE );
    Qry.CreateVariable( "point_id", otInteger, test_point_dep );
    Qry.Execute();
    if ( Qry.Eof ) return false;
  };
  point_dep = Qry.FieldAsInteger( "point_dep" );
  grp_id = Qry.FieldAsInteger( "grp_id" );
  return true;
}

void PrintInterface::ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    TTagKey tag_key;
    tag_key.grp_id = NodeAsInteger("grp_id", reqNode);
    tag_key.dev_model = NodeAsString("dev_model", reqNode);
    tag_key.fmt_type = NodeAsString("fmt_type", reqNode);
    TPrnParams prnParams(reqNode);
    tag_key.pr_lat = prnParams.pr_lat;
    tag_key.type = NodeAsString("type", reqNode);
    tag_key.color = NodeAsString("color", reqNode);
    tag_key.no = NodeAsFloat("no", reqNode);
    if(tag_key.dev_model.empty())
      previewDeviceSets(false, "MSG.PRINTER_NOT_SPECIFIED");
    GetPrintDataBT(dataNode, tag_key);
}

void PrintInterface::GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    TTagKey tag_key;
    tag_key.grp_id = NodeAsInteger("grp_id", reqNode);
    tag_key.dev_model = NodeAsString("dev_model", reqNode);
    tag_key.fmt_type = NodeAsString("fmt_type", reqNode);
    TPrnParams prnParams(reqNode);
    tag_key.pr_lat = prnParams.pr_lat;
    if(tag_key.dev_model.empty())
      previewDeviceSets(false, "MSG.PRINTER_NOT_SPECIFIED");
    GetPrintDataBT(dataNode, tag_key);
}

void PrintInterface::ConfirmPrintUnregVO(
        const std::vector<BPPax> &paxs,
        CheckIn::UserException &ue)
{
    TCachedQuery unregVOConfirmQry("update confirm_print_vo_unreg set pr_print = 1 where id = :pax_id ",
            QParams() << QParam("pax_id", otInteger));
    for (std::vector<BPPax>::const_iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax ) {
        try
        {
            unregVOConfirmQry.get().SetVariable("pax_id", iPax->pax_id);
            unregVOConfirmQry.get().Execute();
            LEvntPrms params;
            params << PrmSmpl<std::string>("full_name", iPax->full_name);
            params << PrmSmpl<string>("voucher", ElemIdToNameLong(etVoucherType, iPax->voucher));
            TReqInfo::Instance()->LocaleToLog("EVT.PRINT_VOUCHER", params, ASTRA::evtPax, iPax->point_dep);
        }
        catch(AstraLocale::UserException &e)
        {
            ue.addError(e.getLexemaData(), iPax->point_dep, iPax->pax_id);
        };
    }
}

void PrintInterface::ConfirmPrintBP(TDevOper::Enum op_type,
                                    const std::vector<BPPax> &paxs,
                                    CheckIn::UserException &ue)
{
    TCachedQuery BIStatQry(
        "update bi_stat set pr_print = 1 where pax_id = :pax_id",
        QParams() << QParam("pax_id", otInteger));

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        "  UPDATE confirm_print "
        "  SET pr_print = 1 "
        "  WHERE "
        "   pax_id = :pax_id AND "
        "   time_print = :time_print AND "
        "   pr_print = 0 AND "
        "   desk=:desk and "
        "   (voucher = :voucher OR voucher IS NULL AND :voucher IS NULL) and "
        OP_TYPE_COND("op_type")
        "  RETURNING seat_no INTO :seat_no; "
        "  :rows:=SQL%ROWCOUNT; "
        "END;";
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
    Qry.DeclareVariable("rows", otInteger);
    Qry.DeclareVariable("seat_no", otString);
    Qry.DeclareVariable("pax_id", otInteger);
    Qry.DeclareVariable("time_print", otDate);
    Qry.DeclareVariable("voucher", otString);
    Qry.CreateVariable("desk", otString, TReqInfo::Instance()->desk.code);
    for (std::vector<BPPax>::const_iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
    {
        try
        {
            BIStatQry.get().SetVariable("pax_id", iPax->pax_id);
            BIStatQry.get().Execute();

            Qry.SetVariable("pax_id", iPax->pax_id);
            Qry.SetVariable("time_print", iPax->time_print);
            Qry.SetVariable("voucher", iPax->voucher);
            Qry.Execute();
            if (Qry.GetVariableAsInteger("rows")==0)
                throw AstraLocale::UserException("MSG.PASSENGER.NO_PARAM.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
            string seat_no = Qry.GetVariableAsString("seat_no");

            LEvntPrms params;
            params << PrmSmpl<std::string>("full_name", iPax->full_name);

            string lexeme;
            switch(op_type) {
                case TDevOper::PrnBP:
                    lexeme = (iPax->voucher.empty() ? "EVT.PRINT_BOARDING_PASS" : "EVT.PRINT_VOUCHER");
                    if(not iPax->voucher.empty())
                        params << PrmSmpl<string>("voucher", ElemIdToNameLong(etVoucherType, iPax->voucher));
                    else {
                        if (seat_no.empty()) params << PrmBool("seat_no", false);
                        else params << PrmSmpl<std::string>("seat_no", seat_no);
                    }
                    break;
                case TDevOper::PrnBI:
                    lexeme = "EVT.PRINT_INVITATION";
                    break;
                default:
                    throw Exception("%d: %d: unexpected dev oper type %d", op_type);
            }
            TReqInfo::Instance()->LocaleToLog(lexeme, params, ASTRA::evtPax, iPax->point_dep,
                                              iPax->reg_no, iPax->grp_id);
        }
        catch(AstraLocale::UserException &e)
        {
          ue.addError(e.getLexemaData(), iPax->point_dep, iPax->pax_id);
        };
    };
};

string getVoucherCode(xmlNodePtr prnCodeNode)
{
    string result;
    if(prnCodeNode) {
        vector<string> tokens;
        result = NodeAsString(prnCodeNode);
        boost::split(tokens, result, boost::is_any_of("."));
        if(
                tokens.size() != 2 or
                tokens[0] != "voucher"
                )
            throw Exception("getVoucherCode: wrong prn code format: %s", result.c_str());
        result = tokens[1];
    }
    return result;
}

void PrintInterface::ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TDevOper::Enum op_type = DevOperTypes().decode(NodeAsString("@op_type", reqNode, DevOperTypes().encode(TDevOper::PrnBP).c_str()));
    TQuery PaxQry(&OraSession);
    PaxQry.SQLText =
        "SELECT pax.grp_id, surname||' '||name full_name, reg_no, point_dep "
        "FROM pax, pax_grp  "
        "WHERE pax.grp_id = pax_grp.grp_id AND pax_id=:pax_id";
    PaxQry.DeclareVariable("pax_id",otInteger);

    TCachedQuery unregVOConfirmQry("   select point_id, surname||' '||name full_name from confirm_print_vo_unreg where id = :pax_id ",
            QParams() << QParam("pax_id", otInteger));

    std::vector<BPPax> paxs, unreg_vo;
    xmlNodePtr curNode = NodeAsNode("passengers/pax", reqNode);
    set<int> ids_set;
    for(; curNode != NULL; curNode = curNode->next)
    {
        BPPax pax;
        pax.pax_id=NodeAsInteger("@pax_id", curNode);

        pax.time_print=NodeAsDateTime("@time_print", curNode);
        // Св-во print_code на данный момент передается только при печати ваучера
        // в остальных случаях pax.voucher пустой.
        pax.voucher = getVoucherCode(GetNode("@print_code", curNode));

        int id = NodeAsInteger("@id", curNode, NoExists);
        if(id != NoExists) {
            // ваучер для незарег. пакса
            unregVOConfirmQry.get().SetVariable("pax_id", pax.pax_id);
            unregVOConfirmQry.get().Execute();
            if(not unregVOConfirmQry.get().Eof) {
                pax.point_dep = unregVOConfirmQry.get().FieldAsInteger("point_id");
                pax.full_name = unregVOConfirmQry.get().FieldAsString("full_name");
                unreg_vo.push_back(pax);
            }
            continue;
        }


        PaxQry.SetVariable("pax_id", pax.pax_id);
        PaxQry.Execute();
        if(PaxQry.Eof) continue;

        pax.point_dep=PaxQry.FieldAsInteger("point_dep");
        ids_set.insert(pax.point_dep);
        pax.grp_id=PaxQry.FieldAsInteger("grp_id");
        pax.reg_no=PaxQry.FieldAsInteger("reg_no");
        pax.full_name=PaxQry.FieldAsString("full_name");
        paxs.push_back(pax);
    };

    vector<int> point_ids(ids_set.begin(), ids_set.end());
    TFlights flightsForLock;
    flightsForLock.Get( point_ids, ftTranzit );
    //лочить рейсы надо по возрастанию poind_dep иначе может быть deadlock
    flightsForLock.Lock(__FUNCTION__);

    CheckIn::UserException ue;
    ConfirmPrintBP(op_type, paxs, ue); //не надо прокидывать ue в терминал - подтверждаем все что можем!
    ConfirmPrintUnregVO(unreg_vo, ue);
};

void PrintInterface::ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    typedef pair<double, string> TNoColor;
    typedef pair<string, TNoColor> TType;
    list<TType> incoming_tags;

    xmlNodePtr curNode = NodeAsNode("tags/tag", reqNode);
    while(curNode) {
        incoming_tags.push_back(
                make_pair(
                    NodeAsString("@type", curNode),
                    make_pair(NodeAsFloat("@no", curNode),
                        NodeAsString("@color", curNode))
                ));
        curNode = curNode->next;
    }

    set<int> ids_set;
    TCachedQuery pointQry(
            "select point_dep from pax_grp, bag_tags where "
            "   no = :no and tag_type = :type and "
            "   (color is null and :color is null or color = :color) and "
            "   bag_tags.grp_id = pax_grp.grp_id ",
            QParams()
            << QParam("type", otString)
            << QParam("no", otFloat)
            << QParam("color", otString));
    for(list<TType>::iterator tag = incoming_tags.begin();
            tag != incoming_tags.end(); tag++) {
        pointQry.get().SetVariable("type", tag->first);
        pointQry.get().SetVariable("no", tag->second.first);
        pointQry.get().SetVariable("color", tag->second.second);
        pointQry.get().Execute();
        if(pointQry.get().Eof)
            throw AstraLocale::UserException("MSG.LUGGAGE.CHANGE_FROM_OTHER_DESK_REFRESH");
        for(; not pointQry.get().Eof; pointQry.get().Next())
            ids_set.insert(pointQry.get().FieldAsInteger("point_dep"));
    }

    vector<int> point_ids(ids_set.begin(), ids_set.end());
    TFlights flightsForLock;
    flightsForLock.Get( point_ids, ftTranzit );
    //лочить рейсы надо по возрастанию poind_dep иначе может быть deadlock
    flightsForLock.Lock(__FUNCTION__);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "update bag_tags set pr_print = 1 where no = :no and tag_type = :type and "
        "   (color is null and :color is null or color = :color)";
    Qry.DeclareVariable("type", otString);
    Qry.DeclareVariable("no", otFloat);
    Qry.DeclareVariable("color", otString);

    for(list<TType>::iterator tag = incoming_tags.begin();
            tag != incoming_tags.end(); tag++) {
        Qry.SetVariable("type", tag->first);
        Qry.SetVariable("no", tag->second.first);
        Qry.SetVariable("color", tag->second.second);
        Qry.Execute();
        if (Qry.RowsProcessed()==0)
            throw AstraLocale::UserException("MSG.LUGGAGE.CHANGE_FROM_OTHER_DESK_REFRESH");
    }
}

void PrintInterface::GetPrintDataBR(string &form_type, PrintDataParser &parser,
        string &Print, bool &hex, xmlNodePtr reqNode)
{
//    big_test(parser, TDevOper::PrnBR);
    xmlNodePtr currNode = reqNode->children;
    string dev_model = NodeAsStringFast("dev_model", currNode);
    string fmt_type = NodeAsStringFast("fmt_type", currNode);
    if(dev_model.empty())
        previewDeviceSets(false, "MSG.PRINTER_NOT_SPECIFIED");

    TPrnParams prnParams(reqNode);

    TQuery Qry(&OraSession);
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
    Qry.Execute();
    if(Qry.Eof||Qry.FieldIsNULL("data"))
        previewDeviceSets(true, "MSG.PRINT.RECEIPT_UNAVAILABLE_FOR_THIS_DEVICE");

    string mso_form = AdjustCR_LF::DoIt(fmt_type, Qry.FieldAsString("data"));
    mso_form = parser.parse(mso_form);
    hex=false;
    if(DevFmtTypes().decode(fmt_type) == TDevFmt::EPSON) {
      to_esc::TConvertParams ConvertParams;
      ConvertParams.init(dev_model);
      LogTrace(TRACE5) << "br form: " << mso_form;
      to_esc::convert(mso_form, ConvertParams, prnParams);
        StringToHex( string(mso_form), mso_form );
        hex=true;
    };
    Print=mso_form;
}


string get_validator(const TBagReceipt &rcpt, bool pr_lat)
{
    ostringstream validator;
    string agency, sale_point_city, sale_point;
    int private_num;

    TTagLang tag_lang;
    tag_lang.Init(rcpt, pr_lat);

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

    const TBaseTableRow &city = base_tables.get("cities").get_row("code", sale_point_city);
    const TBaseTableRow &country = base_tables.get("countries").get_row("code", city.AsString("country"));
    if(validator_type == "ТКП") {
        // agency
        validator
            << tag_lang.ElemIdToTagElem(etAgency, agency, efmtCodeNative)
            << " " << tag_lang.ElemIdToTagElem(etValidatorType, validator_type, efmtCodeNative)
            << endl;
        // agency descr
        validator << tag_lang.ElemIdToTagElem(etSalePoint, sale_point, efmtNameLong).substr(0, 19)  << endl;
        // agency city
        validator
            << tag_lang.ElemIdToTagElem(etCity, sale_point_city, efmtNameLong).substr(0, 16)
            << " "
            << tag_lang.ElemIdToTagElem(etCountry, country.AsString("code"), efmtCodeNative)
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
            << DateTimeToStr(UTCToLocal(rcpt.issue_date, CityTZRegion(desk_city)), "ddmmmyy", tag_lang.GetLang() != AstraLocale::LANG_RU)
            << endl;
        validator << tag_lang.ElemIdToTagElem(etAgency, agency, efmtNameLong).substr(0, 19) << endl;
        validator << tag_lang.ElemIdToTagElem(etSalePoint, sale_point, efmtNameLong).substr(0, 19) << endl;
        validator
            << tag_lang.ElemIdToTagElem(etCity, sale_point_city, efmtNameLong).substr(0, 16)
            << "/"
            << tag_lang.ElemIdToTagElem(etCountry, country.AsString("code"), efmtCodeNative)
            << endl;
        validator << setw(4) << setfill('0') << private_num << endl;
    }
    return validator.str();
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

void tst_dump(int pax_id, int grp_id, bool pr_lat)
{
    vector<string> tags;
    {
        TPrnTagStore pts(TDevOper::PrnBP, grp_id, pax_id, false, pr_lat, NULL);
        pts.tst_get_tag_list(tags);
    }
    for(vector<string>::iterator iv = tags.begin(); iv != tags.end(); iv++) {
        TPrnTagStore tmp_pts(TDevOper::PrnBP, grp_id, pax_id, false, pr_lat, NULL);
        tmp_pts.set_tag("gate", "");
        ProgTrace(TRACE5, "tag: %s; value: '%s'", iv->c_str(), tmp_pts.get_field(*iv, 0, "", "L", "dd.mm hh:nn", "R").c_str());
        tmp_pts.confirm_print(false, TDevOper::PrnBP);
    }
}

void PrintInterface::check_pectab_availability(BPParams &params, TDevOper::Enum op_type, int point_id, const string &cl)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT bp_type, "
        "       DECODE(class,NULL,0,1) AS priority "
        "FROM trip_bp "
        "WHERE point_id=:point_id AND "
        "      (class IS NULL OR class=:class) AND "
        "      op_type=:op_type "
        "ORDER BY priority DESC ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("class", otString, cl);
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
    Qry.Execute();
    if(Qry.Eof)
        switch(op_type) {
            case TDevOper::PrnBP:
                throw AstraLocale::UserException("MSG.BP_TYPE_NOT_ASSIGNED_FOR_FLIGHT_OR_CLASS");
            case TDevOper::PrnBI:
                throw AstraLocale::UserException("MSG.BI_TYPE_NOT_ASSIGNED_FOR_FLIGHT_OR_CLASS");
            default:
                throw Exception("%d: %d: unexpected dev oper type %d", op_type);
        }
    params.form_type = Qry.FieldAsString("bp_type");
}

void PrintInterface::check_pectab_availability(BPParams &params, int grp_id, TDevOper::Enum op_type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText="SELECT point_dep, class, status FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    if (DecodePaxStatus(Qry.FieldAsString("status"))==psCrew)
        throw AstraLocale::UserException("MSG.BOARDINGPASS_NOT_AVAILABLE_FOR_CREW");
    if(Qry.FieldIsNULL("class"))
        throw AstraLocale::UserException("MSG.BOARDINGPASS_NOT_AVAILABLE_FOR_UNACC_BAGGAGE");
    int point_id = Qry.FieldAsInteger("point_dep");
    string cl = Qry.FieldAsString("class");
    check_pectab_availability(params, op_type, point_id, cl);
}

void PrintInterface::get_pectab(
        TDevOper::Enum op_type,
        BPParams &params,
        string &data,
        string &pectab
        )
{
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   prn_form_vers.form, "
        "   prn_form_vers.data "
        "from "
        "   bp_models, "
        "   prn_form_vers "
        "where "
        "   bp_models.form_type = :form_type and "
        "   bp_models.op_type = :op_type and "
        "   bp_models.dev_model = :dev_model and "
        "   bp_models.fmt_type = :fmt_type and "
        "   bp_models.id = prn_form_vers.id and "
        "   bp_models.version = prn_form_vers.version ";
    Qry.CreateVariable("form_type", otString, params.form_type);
    Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
    Qry.CreateVariable("dev_model", otString, params.dev_model);
    Qry.CreateVariable("fmt_type", otString, params.fmt_type);
    Qry.Execute();
    if(Qry.Eof or
       Qry.FieldIsNULL("data") or
       (Qry.FieldIsNULL( "form" ) and (DevFmtTypes().decode(params.fmt_type) == TDevFmt::BTP or
                                     DevFmtTypes().decode(params.fmt_type) == TDevFmt::ATB))
      ) {
        switch(op_type) {
            case TDevOper::PrnBP:
                previewDeviceSets(true, "MSG.PRINT.BP_UNAVAILABLE_FOR_THIS_DEVICE");
                break;
            case TDevOper::PrnBI:
                previewDeviceSets(true, "MSG.PRINT.BI_UNAVAILABLE_FOR_THIS_DEVICE");
                break;
            default:
                throw Exception("%d: %d: unexpected dev oper type %d", op_type);
        }
    }
    pectab = AdjustCR_LF::DoIt(params.fmt_type, Qry.FieldAsString("form"));
    data = AdjustCR_LF::DoIt(params.fmt_type, Qry.FieldAsString("data"));
}

bool IsErrPax(const PrintInterface::BPPax &pax)
{
    return pax.error;
}

void PrintInterface::BPPax::checkBPPrintAllowed()
{
    boost::optional<SEATPAX::paxSeats> paxSeats;
    return checkBPPrintAllowed(paxSeats);
}

void PrintInterface::BPPax::checkBPPrintAllowed(boost::optional<SEATPAX::paxSeats> &paxSeats)
{
    if(isTestPaxId(pax_id)) return;
    if(TReqInfo::Instance()->isSelfCkinClientType()) {
        // point_dep м.б. NoExists, поэтому достаем рейс по grp_id
        TTripInfo flt_info;
        if(not flt_info.getByGrpId(grp_id))
            throw Exception("%s: flight not found for grp_id: %d", __FUNCTION__, grp_id);
        if(not paxSeats) paxSeats = boost::in_place();
        if(
                GetSelfCkinSets(tsEmergencyExitBPNotAllowed, flt_info, TReqInfo::Instance()->client_type) and
                not paxSeats->boarding_pass_not_allowed_reasons(flt_info.point_id, pax_id).empty()
          )
            throw AstraLocale::UserException("MSG.BP_PRINT_TERM_ONLY");
    }
}

void PrintInterface::GetPrintDataBP(
                                    TDevOper::Enum op_type,
                                    BPParams &params,
                                    const std::string &data,
                                    BIPrintRules::Holder &bi_rules,
                                    std::vector<BPPax> &paxs,
                                    boost::optional<AstraLocale::LexemaData> &error
                                    )
{
    if(paxs.empty()) return;
    error = boost::none;

    boost::optional<SEATPAX::paxSeats> paxSeats;
    for (std::vector<BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax ) {

        boost::shared_ptr<PrintDataParser> parser;
        if(iPax->pax_id!=NoExists) {
            iPax->checkBPPrintAllowed(paxSeats);
            try {
                parser = boost::shared_ptr<PrintDataParser> (new PrintDataParser ( op_type, iPax->grp_id, iPax->pax_id, iPax->from_scan_code, params.prnParams.pr_lat, params.clientDataNode ));
                DCSServiceApplying::throwIfNotAllowed( iPax->pax_id, DCSService::Enum::PrintBPOnDesk );
            } catch(UserException &E) {
                if(not error) {
                    TTripInfo fltInfo;
                    fltInfo.getByGrpId(iPax->grp_id);
                    error = GetLexemeDataWithFlight(GetLexemeDataWithRegNo(E.getLexemaData( ), iPax->reg_no), fltInfo);
                }
                iPax->error = true;
                continue;
            }
        } else
            parser = boost::shared_ptr<PrintDataParser> (new PrintDataParser ( op_type, iPax->scan, iPax->errors, true));

        // Из за косяка в клиентской части киоска происходит ошибка, если встречается строка вида
        // 1260,500,T,arial.ttf,36,L,0,
        // Т.е. отсутствуют данные для печати
        // Причем, если буквы L нет, то ошибка не возникает:
        // 1260,500,T,arial.ttf,36,0,
        // Договорились для формата Graphics2D в случае пустых данных передавать пробел

        parser->set_space_if_empty(params.isGraphics2D());

        //        big_test(parser, TDevOper::PrnBP);
        // если это нулевой сегмент, то тогда печатаем выход на посадку иначе не нечатаем
        //надо удалить выход на посадку из данных по пассажиру
        if (iPax->gate.second)
            parser->pts.set_tag("gate", iPax->gate.first);

        if(
                TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION) or
                TReqInfo::Instance()->isSelfCkinClientType()
          ) {
            if(iPax->grp_id > 0 && iPax->pax_id > 0) {
                const BIPrintRules::TRule &bi_rule = bi_rules.get(iPax->grp_id, iPax->pax_id);
                // В случае саморегистрации сегмент всегда считается первым
                if(bi_rule.tags_enabled(op_type, (not iPax->gate.second or TReqInfo::Instance()->isSelfCkinClientType()))) {
                    parser->pts.set_tag(TAG::BI_HALL, bi_rule);
                    parser->pts.set_tag(TAG::BI_HALL_CAPTION, bi_rule);
                    parser->pts.set_tag(TAG::BI_RULE, bi_rule);
                    parser->pts.set_tag(TAG::BI_RULE_GUEST, bi_rule);
                    parser->pts.set_tag(TAG::BI_AIRP_TERMINAL, bi_rule);
                }
            }
        }

        iPax->prn_form = parser->parse(data);
        iPax->hex=false;
        if(DevFmtTypes().decode(params.fmt_type) == TDevFmt::EPSON) {
            to_esc::TConvertParams ConvertParams;
            ConvertParams.init(params.dev_model);
            ProgTrace(TRACE5, "prn_form: %s", iPax->prn_form.c_str());
            to_esc::convert(iPax->prn_form, ConvertParams, params.prnParams);
            StringToHex( string(iPax->prn_form), iPax->prn_form );
            iPax->hex=true;
        }
        if(iPax->pax_id!=NoExists)
            parser->pts.confirm_print(false, op_type);
        else
            parser->pts.save_foreign_scan();
        iPax->time_print=parser->pts.get_time_print();
    }
    paxs.erase(remove_if(paxs.begin(), paxs.end(), IsErrPax), paxs.end());
}

static TBCBPData makeIatciTBCBPData(const astra_api::xml_entities::XmlSegment& seg,
                                    const astra_api::xml_entities::XmlPax& pax)
{
    TBCBPData bcbp;
    bcbp.surname     = pax.surname;
    bcbp.name        = pax.name;
    bcbp.etkt        = pax.ticket_rem == "TKNE";
    bcbp.cls         = pax.subclass;
    bcbp.seat_no     = pax.seat_no;
    bcbp.reg_no      = pax.reg_no;
    bcbp.pers_type   = pax.pers_type;
    bcbp.airp_dep    = seg.seg_info.airp_dep;
    bcbp.airp_arv    = seg.seg_info.airp_arv;
    bcbp.airline     = seg.trip_header.airline;
    bcbp.flt_no      = seg.trip_header.flt_no;
    bcbp.suffix      = seg.trip_header.suffix;
    bcbp.scd         = seg.trip_header.scd_out_local;
    return bcbp;
}

/**
 *  возвращает false - если послана тлг DCQBPR,
 *              true - в остальных случаях
 */
bool PrintInterface::GetIatciPrintDataBP(xmlNodePtr reqNode,
                                         int grpId,
                                         const std::string& data,
                                         const BPParams &params,
                                         std::vector<BPPax> &paxs)
{
    using namespace astra_api::xml_entities;

    LogTrace(TRACE3) << __FUNCTION__ << " for grpId: " << grpId;

    std::string loaded = iatci::IatciXmlDb::load(grpId);    
    if(!loaded.empty())
    {        
        if(!ReqParams(reqNode).getBoolParam("after_kick", false)) {
            tst();
            IatciInterface::ReprintRequest(reqNode);
            return false;
        }

        XMLDoc xml = ASTRA::createXmlDoc(loaded);
        std::list<XmlSegment> lSeg = XmlEntityReader::readSegs(findNodeR(xml.docPtr()->children, "segments"));

        for(const XmlSegment& xmlSeg: lSeg)
        {
            for(const XmlPax& xmlPax: xmlSeg.passengers)
            {
                boost::shared_ptr<PrintDataParser> parser;
                parser = boost::shared_ptr<PrintDataParser>(new PrintDataParser(xmlSeg.seg_info.airp_dep,
                                                                                xmlSeg.seg_info.airp_arv,
                                                                                params.prnParams.pr_lat));

                // билет/купон
                std::ostringstream tickCpn;
                if(xmlPax.ticket_rem == "TKNE") {
                    tickCpn << xmlPax.ticket_no << "/" << xmlPax.coupon_no;
                }

                // полное имя
                std::ostringstream fullName;
                fullName << xmlPax.surname << " " << xmlPax.name;

                parser->pts.set_tag(TAG::ACT,           xmlSeg.trip_header.scd_out_local);
                parser->pts.set_tag(TAG::AIRLINE,       xmlSeg.trip_header.airline);
                parser->pts.set_tag(TAG::AIRLINE_NAME,  xmlSeg.trip_header.airline);
                parser->pts.set_tag(TAG::AIRLINE_SHORT, xmlSeg.trip_header.airline);
                parser->pts.set_tag(TAG::AIRP_ARV,      xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::AIRP_ARV_NAME, xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::AIRP_DEP,      xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::AIRP_DEP_NAME, xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::BAG_AMOUNT,    0); // TODO get it
                parser->pts.set_tag(TAG::BAGGAGE,       ""); // TODO get it
                parser->pts.set_tag(TAG::BAG_WEIGHT,    0); // TODO get it
                parser->pts.set_tag(TAG::BCBP_M_2,      makeIatciTBCBPData(xmlSeg, xmlPax));
                parser->pts.set_tag(TAG::BRD_FROM,      "");
                parser->pts.set_tag(TAG::BRD_TO,        xmlSeg.trip_header.scd_brd_to_local);
                parser->pts.set_tag(TAG::CHD,           ""); // TODO get it
                parser->pts.set_tag(TAG::CITY_ARV_NAME, xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::CITY_DEP_NAME, xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::CLASS,         xmlPax.subclass);
                parser->pts.set_tag(TAG::CLASS_NAME,    xmlPax.subclass);
                parser->pts.set_tag(TAG::DOCUMENT,      xmlPax.doc ? xmlPax.doc->no : "");
                parser->pts.set_tag(TAG::DUPLICATE,     0); // TODO get it
                parser->pts.set_tag(TAG::EST,           xmlSeg.trip_header.scd_out_local);
                parser->pts.set_tag(TAG::ETICKET_NO,    tickCpn.str());
                parser->pts.set_tag(TAG::ETKT,          tickCpn.str());
                parser->pts.set_tag(TAG::EXCESS,        0); // TODO get it
                parser->pts.set_tag(TAG::FLT_NO,        xmlSeg.trip_header.flt_no);
                parser->pts.set_tag(TAG::FQT,           ""); // TODO get it
                parser->pts.set_tag(TAG::FULLNAME,      fullName.str());
                parser->pts.set_tag(TAG::FULL_PLACE_ARV,xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::FULL_PLACE_DEP,xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::GATE,          xmlSeg.trip_header.remote_gate);
                parser->pts.set_tag(TAG::GATES,         ""); // TODO get it
                parser->pts.set_tag(TAG::HALL,          ""); // TODO get it
                parser->pts.set_tag(TAG::INF,           xmlPax.pers_type);
                parser->pts.set_tag(TAG::LIST_SEAT_NO,  xmlPax.seat_no);
                parser->pts.set_tag(TAG::LONG_ARV,      xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::LONG_DEP,      xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::NAME,          xmlPax.name);
                parser->pts.set_tag(TAG::NO_SMOKE,      0); // TODO get it
                parser->pts.set_tag(TAG::ONE_SEAT_NO,   xmlPax.seat_no);
                parser->pts.set_tag(TAG::PAX_ID,        xmlPax.pax_id);
                parser->pts.set_tag(TAG::PAX_TITLE,     ""); // TODO get it
                parser->pts.set_tag(TAG::PLACE_ARV,     xmlSeg.seg_info.airp_arv);
                parser->pts.set_tag(TAG::PLACE_DEP,     xmlSeg.seg_info.airp_dep);
                parser->pts.set_tag(TAG::PNR,           ""); // TODO get it
                parser->pts.set_tag(TAG::REG_NO,        xmlPax.reg_no);
                parser->pts.set_tag(TAG::REM,           ""); // TODO get it
                parser->pts.set_tag(TAG::RK_AMOUNT,      0); // TODO get it
                parser->pts.set_tag(TAG::RK_WEIGHT,      0); // TODO get it
                parser->pts.set_tag(TAG::SCD,           xmlSeg.trip_header.scd_out_local);
                parser->pts.set_tag(TAG::SEAT_NO,       xmlPax.seat_no);
                parser->pts.set_tag(TAG::STR_SEAT_NO,   xmlPax.seat_no);
                parser->pts.set_tag(TAG::SUBCLS,        xmlPax.subclass);
                parser->pts.set_tag(TAG::SURNAME,       xmlPax.surname);
                parser->pts.set_tag(TAG::TAGS,          ""); // TODO get it


                BPPax pax;
                pax.pax_id = -1;
                pax.reg_no = xmlPax.reg_no; // Зачем?
                pax.prn_form = parser->parse(data);
                pax.hex = false;
                if(DevFmtTypes().decode(params.fmt_type) == TDevFmt::EPSON) {
                    to_esc::TConvertParams ConvertParams;
                    ConvertParams.init(params.dev_model);
                    ProgTrace(TRACE5, "iatci prn_form: %s", pax.prn_form.c_str());
                    to_esc::convert(pax.prn_form, ConvertParams, params.prnParams);
                    StringToHex(string(pax.prn_form), pax.prn_form);
                    pax.hex = true;
                }
                pax.time_print = parser->pts.get_time_print();
                paxs.push_back(pax);
            }
        }
    }

    return true;
}

void PrintInterface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    GetPrintDataBP(reqNode, resNode);
}

void tripVOToXML(list<int> &pax_ids, int point_id, xmlNodePtr resNode, bool pr_paxes)
{
    set<string> trip_vouchers;
    getTripVouchers(point_id, trip_vouchers);

    if(trip_vouchers.empty())
        throw AstraLocale::UserException("MSG.VOUCHER.ACCESS_DENIED");

    xmlNodePtr printNode = NewTextChild(NewTextChild(resNode, "data"), "print");
    SetProp(printNode, "vouchers");

    xmlNodePtr voNode = NewTextChild(printNode, "voucher_types");
    for(set<string>::iterator
            i = trip_vouchers.begin(); i != trip_vouchers.end(); i++) {
        xmlNodePtr itemNode = NewTextChild(voNode, "item");
        NewTextChild(itemNode, "code", *i);
        NewTextChild(itemNode, "name", ElemIdToNameLong(etVoucherType, *i));
    }

    xmlNodePtr paxListNode = NewTextChild(printNode, "passengers");
    for(list<int>::iterator i = pax_ids.begin(); i != pax_ids.end(); i++) {
        xmlNodePtr paxNode = NewTextChild(paxListNode, "passenger");
        NewTextChild(paxNode,
                (pr_paxes ? "pax_id" : "id"),
                *i);
        voNode = NewTextChild(paxNode, "vouchers");
        for(set<string>::iterator
                i = trip_vouchers.begin(); i != trip_vouchers.end(); i++) {
            xmlNodePtr itemNode = NewTextChild(voNode, "item");
            NewTextChild(itemNode, "code", *i);
            NewTextChild(itemNode, "pr_print", true);
        }
    }
}

void PrintInterface::GetPrintDataVO(
        int first_seg_grp_id,
        int pax_id,
        int pr_all,
        BPParams &params,
        xmlNodePtr reqNode,
        xmlNodePtr resNode
        )
{
    xmlNodePtr currNode = reqNode->children;
    currNode = GetNodeFast("vouchers", currNode);
    if(not currNode)
        throw Exception("PrintInterface::GetPrintDataVO: vouchers tag not found");
    if(not currNode->children) {
        TCachedQuery Qry(
                "SELECT pax_id, point_dep "
                " FROM pax, pax_grp "
                "WHERE pax.grp_id = :grp_id AND "
                "      refuse IS NULL and "
                "      pax.grp_id = pax_grp.grp_id "
                "ORDER BY pax.reg_no ",
                QParams() << QParam("grp_id", otInteger, first_seg_grp_id));
        Qry.get().Execute();

        int point_id = NoExists;
        list<int> pax_ids;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            point_id = Qry.get().FieldAsInteger("point_dep");
            pax_ids.push_back(Qry.get().FieldAsInteger("pax_id"));
        }

        if(pax_ids.empty())
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

        tripVOToXML(pax_ids, point_id, resNode, true);
    } else {
        // С клиента пришел список паксов с выбранными значениями
        typedef list<pair<string, bool> > TvList;
        typedef map<int, TvList> TPaxList;
        TPaxList pax_list;
        for(xmlNodePtr paxNode = currNode->children;
                paxNode; paxNode = paxNode->next) {
            currNode = paxNode->children;
            int pax_id = NodeAsIntegerFast("pax_id", currNode);
            currNode = NodeAsNodeFast("vouchers", currNode);
            for(xmlNodePtr itemNode = currNode->children;
                    itemNode; itemNode = itemNode->next) {
                currNode = itemNode->children;
                string vCode = NodeAsStringFast("code", currNode);
                bool pr_print = NodeAsIntegerFast("pr_print", currNode) != 0;
                pax_list[pax_id].push_back(make_pair(vCode, pr_print));
            }
        }

        xmlNodePtr BPNode = NewTextChild(NewTextChild(resNode, "data"), "print");
        string data, pectab;
        get_pectab(TDevOper::PrnBP, params, data, pectab);
        NewTextChild(BPNode, "pectab", pectab);
        xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");

        for(TPaxList::iterator
                pax = pax_list.begin();
                pax != pax_list.end();
                pax++) {
            for(TvList::iterator
                    v = pax->second.begin();
                    v != pax->second.end();
                    v++) {
                if(not v->second) continue;

                TCachedQuery Qry("select grp_id, reg_no from pax where pax_id = :pax_id",
                        QParams() << QParam("pax_id", otInteger, pax->first));
                Qry.get().Execute();
                if(Qry.get().Eof)
                    throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");

                int grp_id = Qry.get().FieldAsInteger("grp_id");
                int reg_no = Qry.get().FieldAsInteger("reg_no");

                PrintDataParser parser(TDevOper::PrnBP, grp_id, pax->first, false, params.prnParams.pr_lat, params.clientDataNode);

                parser.pts.set_tag(TAG::VOUCHER_CODE, v->first);
                parser.pts.set_tag(TAG::VOUCHER_TEXT, v->first);
                parser.pts.set_tag(TAG::DUPLICATE, 0);

                string prn_form = parser.parse(data);
                bool hex = false;
                if(DevFmtTypes().decode(params.fmt_type) == TDevFmt::EPSON) {
                    to_esc::TConvertParams ConvertParams;
                    ConvertParams.init(params.dev_model);
                    ProgTrace(TRACE5, "prn_form: %s", prn_form.c_str());
                    to_esc::convert(prn_form, ConvertParams, params.prnParams);
                    StringToHex( string(prn_form), prn_form );
                    LogTrace(TRACE5) << "after StringToHex prn_form: " << prn_form;
                    hex=true;
                }
                parser.pts.confirm_print(false, TDevOper::PrnBP);

                xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
                SetProp(paxNode, "pax_id", pax->first);
                SetProp(paxNode, "reg_no", reg_no);
                SetProp(paxNode, "pr_print", true);
                SetProp(paxNode, "print_code", "voucher." + v->first);
                SetProp(paxNode, "time_print", DateTimeToStr(parser.pts.get_time_print()));
                SetProp(NewTextChild(paxNode, "prn_form", prn_form),"hex",hex);
            }
        }
    }
}

void PrintInterface::GetPrintDataVOUnregistered(
        BPParams &params,
        TDevOper::Enum op_type,
        xmlNodePtr reqNode,
        xmlNodePtr resNode)
{
    LogTrace(TRACE5) << "GetPrintDataVOUnregistered begin";
    xmlNodePtr currNode = reqNode->children;
    xmlNodePtr vouchersNode = GetNodeFast("vouchers", currNode);
    if(not vouchersNode)
        throw Exception("GetPrintDataVOUnregistered: vouchers node not found where expected");
    string cl = NodeAsString("@cl", vouchersNode);
    int point_id = NodeAsInteger("@point_id", vouchersNode);
    if(cl.empty()) throw AstraLocale::UserException("MSG.PASSENGERS.NOT_SET_CLASS");

    check_pectab_availability(params, op_type, point_id, cl);

    currNode = vouchersNode->children;
    currNode = GetNodeFast("pax_list", currNode);
    if(currNode) { // пришли незареганные паксы, которых надо вывести в форму ваучеров.
        currNode = currNode->children;
        list<int> ids;
        for(; currNode; currNode = currNode->next)
            ids.push_back(NodeAsInteger(currNode));
        tripVOToXML(ids, point_id, resNode, false);
    } else { // пришли паксы на печать
        struct TVOPax {
            int id;
            string name;
            string surname;
        };

        typedef list<pair<string, bool> > TvList;
        typedef list<pair<TVOPax, TvList> > TPaxList;

        TPaxList pax_list;
        for(xmlNodePtr paxNode = vouchersNode->children;
                paxNode; paxNode = paxNode->next) {
            currNode = paxNode->children;
            TVOPax pax;
            pax.id = NodeAsIntegerFast("id", currNode);
            pax.surname = NodeAsStringFast("surname", currNode);
            pax.name = NodeAsStringFast("name", currNode);
            pax_list.push_back(make_pair(pax, TvList()));

            currNode = NodeAsNodeFast("vouchers", currNode);
            for(xmlNodePtr itemNode = currNode->children;
                    itemNode; itemNode = itemNode->next) {
                currNode = itemNode->children;
                string vCode = NodeAsStringFast("code", currNode);
                bool pr_print = NodeAsIntegerFast("pr_print", currNode) != 0;
                pax_list.back().second.push_back(make_pair(vCode, pr_print));
            }
        }

        xmlNodePtr BPNode = NewTextChild(NewTextChild(resNode, "data"), "print");
        string data, pectab;
        get_pectab(TDevOper::PrnBP, params, data, pectab);
        NewTextChild(BPNode, "pectab", pectab);
        xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");

        TTripRoute route;
        route.GetRouteAfter(NoExists, point_id, trtWithCurrent, trtNotCancelled);
        if(route.size() < 2) throw Exception("%s: wrong route", __FUNCTION__);
        string airp_dep = route[0].airp;
        string airp_arv = route[1].airp;

        TTripInfo info;
        info.getByPointId(point_id);

        PrintDataParser parser(airp_dep, airp_arv, params.prnParams.pr_lat);

        TBCBPData bcbp;
        bcbp.cls         = "Э";
        bcbp.reg_no      = 0;
        bcbp.pers_type   = "ВЗ";
        bcbp.airp_dep    = airp_dep;
        bcbp.airp_arv    = airp_arv;
        bcbp.airline     = info.airline;
        bcbp.flt_no      = info.flt_no;
        bcbp.suffix      = info.suffix;
        bcbp.scd         = UTCToLocal(info.scd_out, AirpTZRegion(airp_dep));
        bcbp.is_boarding_pass = false;
        bcbp.seat_no = "000X";

        parser.pts.set_tag(TAG::ACT,           UTCToLocal(info.act_est_scd_out(), AirpTZRegion(airp_dep)));
        parser.pts.set_tag(TAG::AIRLINE,       info.airline);
        parser.pts.set_tag(TAG::AIRLINE_NAME,  info.airline);
        parser.pts.set_tag(TAG::AIRLINE_SHORT, info.airline);
        parser.pts.set_tag(TAG::AIRP_ARV,      airp_arv);
        parser.pts.set_tag(TAG::AIRP_ARV_NAME, airp_arv);
        parser.pts.set_tag(TAG::AIRP_DEP,      airp_dep);
        parser.pts.set_tag(TAG::AIRP_DEP_NAME, airp_dep);
        parser.pts.set_tag(TAG::BAG_AMOUNT,    0); // TODO get it
        parser.pts.set_tag(TAG::BAGGAGE,       ""); // TODO get it
        parser.pts.set_tag(TAG::BAG_WEIGHT,    0); // TODO get it
        parser.pts.set_tag(TAG::BCBP_M_2,      " "); // TODO get it
        parser.pts.set_tag(TAG::BRD_FROM,      ""); // TODO get it
        parser.pts.set_tag(TAG::BRD_TO,        ""); // TODO get it
        parser.pts.set_tag(TAG::CHD,           ""); // TODO get it
        parser.pts.set_tag(TAG::CITY_ARV_NAME, airp_arv);
        parser.pts.set_tag(TAG::CITY_DEP_NAME, airp_dep);
        parser.pts.set_tag(TAG::CLASS,         cl);
        parser.pts.set_tag(TAG::CLASS_NAME,    cl);
        parser.pts.set_tag(TAG::DOCUMENT,      "");
        parser.pts.set_tag(TAG::DUPLICATE,     0); // TODO get it
        parser.pts.set_tag(TAG::EST,           UTCToLocal(info.est_scd_out(), AirpTZRegion(airp_dep)));;
        parser.pts.set_tag(TAG::ETICKET_NO,    "");
        parser.pts.set_tag(TAG::ETKT,          "");
        parser.pts.set_tag(TAG::EXCESS,        0); // TODO get it
        parser.pts.set_tag(TAG::FLT_NO,        info.flt_no);
        parser.pts.set_tag(TAG::FQT,           ""); // TODO get it
        parser.pts.set_tag(TAG::FULL_PLACE_ARV,airp_arv);
        parser.pts.set_tag(TAG::FULL_PLACE_DEP,airp_dep);
        // parser.pts.set_tag(TAG::GATE,          ""); // приходит с клиента см. tagsFromXML ниже
        parser.pts.set_tag(TAG::GATES,         ""); // TODO get it
        parser.pts.set_tag(TAG::HALL,          ""); // TODO get it
        parser.pts.set_tag(TAG::INF,           "");
        parser.pts.set_tag(TAG::LIST_SEAT_NO,  "");
        parser.pts.set_tag(TAG::LONG_ARV,      airp_arv);
        parser.pts.set_tag(TAG::LONG_DEP,      airp_dep);
        parser.pts.set_tag(TAG::NO_SMOKE,      0); // TODO get it
        parser.pts.set_tag(TAG::ONE_SEAT_NO,   "");
        parser.pts.set_tag(TAG::PAX_ID,        NoExists);
        parser.pts.set_tag(TAG::PAX_TITLE,     ""); // TODO get it
        parser.pts.set_tag(TAG::PLACE_ARV,     airp_arv);
        parser.pts.set_tag(TAG::PLACE_DEP,     airp_dep);
        parser.pts.set_tag(TAG::PNR,           ""); // TODO get it
        parser.pts.set_tag(TAG::REG_NO,        NoExists);
        parser.pts.set_tag(TAG::REM,           ""); // TODO get it
        parser.pts.set_tag(TAG::RK_AMOUNT,      0); // TODO get it
        parser.pts.set_tag(TAG::RK_WEIGHT,      0); // TODO get it
        parser.pts.set_tag(TAG::SCD,           UTCToLocal(info.scd_out, AirpTZRegion(airp_dep)));
        parser.pts.set_tag(TAG::SEAT_NO,       "");
        parser.pts.set_tag(TAG::STR_SEAT_NO,   "");
        parser.pts.set_tag(TAG::SUBCLS,        "");
        parser.pts.set_tag(TAG::TAGS,          ""); // TODO get it

        parser.pts.tagsFromXML(params.clientDataNode);

        QParams qryParams;
        qryParams
            << QParam("time_print", otDate, NowUTC())
            << QParam("point_id", otInteger, point_id)
            << QParam("scd_out", otDate, info.scd_out)
            << QParam("surname", otString)
            << QParam("name", otString)
            << QParam("voucher", otString)
            << QParam("id", otInteger)
            << QParam("desk", otString, TReqInfo::Instance()->desk.code)
            // В версиях ранее VO_STAT_VERSION печать подтверждается сразу
            << QParam("pr_print", otInteger, not TReqInfo::Instance()->desk.compatible(VO_STAT_VERSION));
        TCachedQuery confirmQry(
                "begin "
                "  insert into confirm_print_vo_unreg ( "
                "     id, "
                "     time_print, "
                "     point_id, "
                "     scd_out, "
                "     surname, "
                "     name, "
                "     voucher, "
                "     desk, "
                "     pr_print "
                "  ) values ( "
                "     id__seq.nextval, "
                "     :time_print, "
                "     :point_id, "
                "     :scd_out, "
                "     :surname, "
                "     :name, "
                "     :voucher, "
                "     :desk, "
                "     :pr_print "
                "  ) returning id into :id; "
                "end; ",
            qryParams);
        for(TPaxList::iterator
                pax = pax_list.begin();
                pax != pax_list.end();
                pax++) {
            for(TvList::iterator
                    v = pax->second.begin();
                    v != pax->second.end();
                    v++) {
                if(not v->second) continue;


                parser.pts.set_tag(TAG::VOUCHER_CODE, v->first);
                parser.pts.set_tag(TAG::VOUCHER_TEXT, v->first);
                parser.pts.set_tag(TAG::DUPLICATE, 0);

                std::ostringstream fullName;
                fullName << pax->first.surname << " " << pax->first.name;
                parser.pts.set_tag(TAG::FULLNAME,      fullName.str());
                parser.pts.set_tag(TAG::NAME,          pax->first.name);
                parser.pts.set_tag(TAG::SURNAME,       pax->first.surname);

                bcbp.surname     = pax->first.surname;
                bcbp.name        = pax->first.name;
                parser.pts.set_tag(TAG::BCBP_M_2,      bcbp);

                string prn_form = parser.parse(data);
                bool hex = false;
                if(DevFmtTypes().decode(params.fmt_type) == TDevFmt::EPSON) {
                    to_esc::TConvertParams ConvertParams;
                    ConvertParams.init(params.dev_model);
                    ProgTrace(TRACE5, "prn_form: %s", prn_form.c_str());
                    to_esc::convert(prn_form, ConvertParams, params.prnParams);
                    StringToHex( string(prn_form), prn_form );
                    LogTrace(TRACE5) << "after StringToHex prn_form: " << prn_form;
                    hex=true;
                }
                //parser.pts.confirm_print(false, TDevOper::PrnBP);
                confirmQry.get().SetVariable("surname", pax->first.surname);
                confirmQry.get().SetVariable("name", pax->first.name);
                confirmQry.get().SetVariable("voucher", v->first);
                confirmQry.get().Execute();

                if(not TReqInfo::Instance()->desk.compatible(VO_STAT_VERSION)) {
                    LEvntPrms params;
                    params << PrmSmpl<std::string>("full_name", fullName.str());
                    params << PrmSmpl<string>("voucher", ElemIdToNameLong(etVoucherType, v->first));
                    TReqInfo::Instance()->LocaleToLog("EVT.PRINT_VOUCHER", params, ASTRA::evtPax, point_id);
                }



                xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
                SetProp(paxNode, "pax_id", confirmQry.get().GetVariableAsInteger("id"));
                SetProp(paxNode, "id", pax->first.id);
                SetProp(paxNode, "reg_no", 0);
                SetProp(paxNode, "pr_print", true);
                SetProp(paxNode, "print_code", "voucher." + v->first);
                SetProp(paxNode, "time_print", DateTimeToStr(parser.pts.get_time_print()));
                SetProp(NewTextChild(paxNode, "prn_form", prn_form),"hex",hex);
            }
        }
    }
}

void PrintInterface::GetPrintDataBP(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr currNode = reqNode->children;
    BPParams params;
    int first_seg_grp_id = NodeAsIntegerFast("grp_id", currNode, NoExists); // grp_id - первого сегмента или ид. группы
    int pax_id = NodeAsIntegerFast("pax_id", currNode, NoExists);
    int pr_all = NodeAsIntegerFast("pr_all", currNode, NoExists);
    TDevOper::Enum op_type = DevOperTypes().decode(NodeAsStringFast("op_type", currNode, DevOperTypes().encode(TDevOper::PrnBP).c_str()));
    xmlNodePtr vouchersNode = GetNodeFast("vouchers", currNode);
    xmlNodePtr VOPaxListNode = GetNode("pax_list", vouchersNode);
    params.dev_model = NodeAsStringFast("dev_model", currNode);
    params.fmt_type = NodeAsStringFast("fmt_type", currNode);
    params.prnParams.get_prn_params(reqNode);
    params.clientDataNode = NodeAsNodeFast("clientData", currNode);
    if(params.dev_model.empty())
      previewDeviceSets(false, "MSG.PRINTER_NOT_SPECIFIED");

    if(VOPaxListNode or first_seg_grp_id == 0)
        return GetPrintDataVOUnregistered(params, op_type, reqNode, resNode);

    TQuery Qry(&OraSession);
    if(first_seg_grp_id == NoExists) {
        Qry.Clear();
        Qry.SQLText="SELECT grp_id from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        first_seg_grp_id = Qry.FieldAsInteger("grp_id");
    }

    check_pectab_availability(params, first_seg_grp_id, op_type);

    if(vouchersNode)
        return GetPrintDataVO(
                first_seg_grp_id,
                pax_id,
                pr_all,
                params,
                reqNode,
                resNode
                );

    std::vector<BPPax> paxs;
    Qry.Clear();
    if ( pax_id == NoExists ) { // печать всех или только тех, у которых не подтверждена распечатка
        vector<int> grps;
        TCkinRoute cr;
        cr.GetRouteAfter(first_seg_grp_id, crtWithCurrent, crtOnlyDependent);
        if (!cr.empty())
        {
          for(vector<TCkinRouteItem>::iterator iv = cr.begin(); iv != cr.end(); iv++)
              grps.push_back(iv->grp_id);
        }
        else grps.push_back(first_seg_grp_id);

        Qry.Clear();
        if ( pr_all )
            Qry.SQLText =
                "SELECT pax_id, grp_id, reg_no "
                " FROM pax "
                "WHERE grp_id = :grp_id AND "
                "      refuse IS NULL "
                "ORDER BY pax.reg_no, pax.seats DESC";
        else {
            // Если не все билеты (приглашения) отпечатаны
            // запрос должен возвращать паксов у которых есть неотпечанные билеты (приглашения)
            Qry.SQLText =
                "SELECT pax.pax_id, pax.grp_id, pax.reg_no "
                " FROM pax, confirm_print cp "
                "WHERE  pax.grp_id = :grp_id AND "
                "       pax.refuse IS NULL AND "
                "       pax.pax_id = cp.pax_id(+) AND "
                "       cp.voucher(+) is null and "
                "       nvl(cp.op_type(+), :op_type) = :op_type and " // дефайн OP_TYPE_COND здесь не катит, т.к. плюсик.
                "       cp.pr_print(+) <> 0 AND "
                "       cp.pax_id IS NULL "
                "ORDER BY pax.reg_no, pax.seats DESC";
            Qry.CreateVariable("op_type", otString, DevOperTypes().encode(op_type));
        }
        Qry.DeclareVariable( "grp_id", otInteger );
        for( vector<int>::iterator igrp=grps.begin(); igrp!=grps.end(); igrp++ ) {
            Qry.SetVariable( "grp_id", *igrp );
            Qry.Execute();

            if ( Qry.Eof && pr_all ) // анализ на клиенте по сегментам, значит и мы должны анализировать по сегментам
                throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA"); //все посадочные отпечатаны
            while ( !Qry.Eof ) {
                paxs.push_back( BPPax( Qry.FieldAsInteger("grp_id"),
                                       Qry.FieldAsInteger("pax_id"),
                                       Qry.FieldAsInteger("reg_no") ) );
                Qry.Next();
            }
        }
        if ( !pr_all && paxs.empty() ) //все посадочные отпечатаны, но при этом надо было напечатать те, которые были не напечатанны
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
    }
    else if(pax_id > 0) { // печать конкретного пассажира. Если < 0, то это пакс iatci, его начитывает GetIatciPrintDataBP ниже
        Qry.SQLText =
            "SELECT grp_id, pax_id, reg_no FROM pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if ( Qry.Eof )
            throw AstraLocale::UserException("MSG.CHECKIN.GRP.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
        paxs.push_back( BPPax( Qry.FieldAsInteger("grp_id"),
                               Qry.FieldAsInteger("pax_id"),
                               Qry.FieldAsInteger("reg_no") ) );
    };

    for (std::vector<BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax )
      if(first_seg_grp_id != iPax->grp_id) iPax->gate=make_pair("", true);

    // Начитываем правила БП для всех паксов
    BIPrintRules::Holder bi_rules(op_type);
    if(
            TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION) or
            TReqInfo::Instance()->isSelfCkinClientType()
      ) {
        bool bi_access = false;
        for (std::vector<BPPax>::iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax ) {
            if(first_seg_grp_id != iPax->grp_id) continue;

            LogTrace(TRACE5) << "bi_rules.get pax_id = " << iPax->pax_id;

            const BIPrintRules::TRule &bi_rule = bi_rules.get(iPax->grp_id, iPax->pax_id);

            bi_access = bi_access or not (not bi_rule.exists() or not bi_rule.pr_print_bi);
        }
        if(op_type == TDevOper::PrnBI and not bi_access)
            throw AstraLocale::UserException("MSG.BI.ACCESS_DENIED");
    }

    // Если с клиента пришли выбранные залы, выбираем их
    bi_rules.select(reqNode);
    if(
            (
             TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION) or
             TReqInfo::Instance()->isSelfCkinClientType()
            ) and
            not bi_rules.complete() // требуется назначить залы пассажирам
      ) {
        LogTrace(TRACE5) << "complete false";
        bi_rules.toXML(op_type, resNode);
    } else {

        LogTrace(TRACE5) << "complete true";

        string pectab, data;
        get_pectab(op_type, params, data, pectab);
        boost::optional<AstraLocale::LexemaData> error;
        GetPrintDataBP(op_type, params, data, bi_rules, paxs, error);

        if(pax_id < 0 and !GetIatciPrintDataBP(reqNode, first_seg_grp_id, data, params, paxs)) {
            tst();
            return AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR");
        }

        xmlNodePtr BPNode = NewTextChild(NewTextChild(resNode, "data"),
                (TReqInfo::Instance()->desk.compatible(OP_TYPE_VERSION) or TReqInfo::Instance()->isSelfCkinClientType() ? "print" : "printBP")
                );
        NewTextChild(BPNode, "pectab", pectab);
        xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
        for (std::vector<BPPax>::const_iterator iPax=paxs.begin(); iPax!=paxs.end(); ++iPax ) {
            bool pr_print = (op_type == TDevOper::PrnBP);

            if(iPax->grp_id > 0 && iPax->pax_id > 0) {
                // В режиме приглашений выводим только тех, у кого есть правила.
                const BIPrintRules::TRule &bi_rule = bi_rules.get(iPax->grp_id, iPax->pax_id);
                pr_print |= (op_type == TDevOper::PrnBI and bi_rule.exists() and first_seg_grp_id == iPax->grp_id);
            }

            xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
            SetProp(paxNode, "pax_id", iPax->pax_id);
            SetProp(paxNode, "reg_no", (int)iPax->reg_no);
            SetProp(paxNode, "pr_print", pr_print);
            if(pr_print) {
                SetProp(paxNode, "time_print", DateTimeToStr(iPax->time_print));
                bool unbound_emd_warning=(pax_id == NoExists &&               // печать всех или только тех, у которых не подтверждена распечатка
                        iPax->grp_id == first_seg_grp_id && // только для пассажиров первого сегмента сквозной регистрации
                        PaxASVCList::ExistsPaxUnboundBagEMD(iPax->pax_id));
                NewTextChild(paxNode, "unbound_emd_warning", (int)unbound_emd_warning, (int)false);
                SetProp(NewTextChild(paxNode, "prn_form", iPax->prn_form),"hex",(int)iPax->hex);
                bi_rules.toStat(iPax->grp_id, iPax->pax_id, iPax->time_print);
            }
        }
        if(error)
            AstraLocale::showErrorMessage(error.get());
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
    map<TDevOper::Enum, TOpsItem> items;
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
        items[DevOperTypes().decode(NodeAsString("@type", currNode))] = opsItem;
    }
}

void PrintInterface::print_bp2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr contentNode = NodeAsNode("content", reqNode)->children;
    if(not contentNode) throw Exception("content is null");

    TSearchFltInfo filter;
    filter.airline = getElemId(etAirline, NodeAsString("airline", contentNode));
    filter.flt_no = NodeAsInteger("flt_no", contentNode);
    filter.airp_dep = getElemId(etAirp,NodeAsString("airp_dep", contentNode));
    if(StrToDateTime(NodeAsString("scd_out", contentNode), "dd.mm.yy", filter.scd_out) == EOF)
        throw Exception("print_bp: can't convert scd_out: %s", NodeAsString("scd_Out", contentNode));
    filter.scd_out_in_utc = true;
    filter.only_with_reg = false;

    list<TAdvTripInfo> flts;
    SearchFlt(filter, flts);

    TNearestDate nd(filter.scd_out);
    for(list<TAdvTripInfo>::iterator i = flts.begin(); i != flts.end(); ++i)
        nd.sorted_points[i->scd_out] = i->point_id;
    int point_id = nd.get();
    if(point_id == NoExists)
        throw Exception("print_bp: flight not found");

    int reg_no = NodeAsInteger("reg_no", contentNode);

    TCachedQuery Qry(
            "select "
            "   pax.grp_id, "
            "   pax.pax_id "
            "from "
            "   pax_grp, "
            "   pax "
            "where "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.grp_id = pax.grp_id and "
            "   pax.reg_no = :reg_no ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("reg_no", otInteger, reg_no)
            );
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int grp_id = Qry.get().FieldAsInteger("grp_id");
        int pax_id = Qry.get().FieldAsInteger("pax_id");

        TDevOper::Enum op_type = TDevOper::PrnBP;
        BPParams params;
        params.dev_model = NodeAsString("dev_model", contentNode);
        params.fmt_type = NodeAsString("fmt_type", contentNode);
        check_pectab_availability(params, grp_id, op_type);
        string pectab, data;
        get_pectab(op_type, params, data, pectab);

        PrintDataParser parser(op_type, grp_id, pax_id, false, false, NULL);
        parser.set_space_if_empty(params.isGraphics2D());
        parser.pts.set_tag(TAG::GATE, "ВРАТА");
        parser.pts.set_tag(TAG::DUPLICATE, 1);
        parser.pts.set_tag(TAG::VOUCHER_CODE, "DV");
        parser.pts.set_tag(TAG::VOUCHER_TEXT, "DV");
        data = StrUtils::b64_encode(
                ConvertCodepage(parser.parse(data),"CP866","UTF-8"));
        SetProp(NewTextChild(resNode, "content", data), "b64", true);
    } else
        throw Exception("reg_no " + IntToString(reg_no) + " not found");
}

void PrintInterface::print_bp(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string content = NodeAsString("content", reqNode);
    size_t idx = content.find('\n');
    if(idx != string::npos) {
        vector<string> params;
        string buf = content.substr(0, idx);
        boost::split(params, buf, boost::is_any_of(" "));
        if(params.size() > 6 or params.size() < 5)
            throw Exception("print_bp: wrong params");
        content.erase(0, idx + 1);

        TSearchFltInfo filter;
        filter.airline = getElemId(etAirline, params[0]);
        filter.flt_no = ToInt(params[1]);
        filter.airp_dep = getElemId(etAirp, params[3]);
        if(StrToDateTime(params[2].c_str(), "dd.mm.yy", filter.scd_out) == EOF)
            throw Exception("print_bp: can't convert scd_out: %s", params[2].c_str());
        filter.scd_out_in_utc = true;
        filter.only_with_reg = false;

        list<TAdvTripInfo> flts;
        SearchFlt(filter, flts);

        TNearestDate nd(filter.scd_out);
        for(list<TAdvTripInfo>::iterator i = flts.begin(); i != flts.end(); ++i)
            nd.sorted_points[i->scd_out] = i->point_id;
        int point_id = nd.get();
        if(point_id == NoExists)
            throw Exception("print_bp: flight not found");

        int reg_no = ToInt(params[4]);
        bool is_boarding_pass = true;
        if(params.size() == 6) {
            if(params[5] == "I")
                is_boarding_pass = false;
            else
                throw Exception("print_bp: wrong BP param: %s", params[5].c_str());
        }

        TCachedQuery Qry(
                "select "
                "   pax.grp_id, "
                "   pax.pax_id "
                "from "
                "   pax_grp, "
                "   pax "
                "where "
                "   pax_grp.point_dep = :point_id and "
                "   pax_grp.grp_id = pax.grp_id and "
                "   pax.reg_no = :reg_no ",
                QParams()
                << QParam("point_id", otInteger, point_id)
                << QParam("reg_no", otInteger, reg_no)
                );
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int grp_id = Qry.get().FieldAsInteger("grp_id");
            int pax_id = Qry.get().FieldAsInteger("pax_id");
            PrintDataParser parser(TDevOper::PrnBP, grp_id, pax_id, false, false, NULL);
            parser.pts.set_tag(TAG::GATE, "ВРАТА");
            parser.pts.set_tag(TAG::DUPLICATE, 1);
            if(not is_boarding_pass) {
                parser.pts.set_tag(TAG::VOUCHER_CODE, "DV");
                parser.pts.set_tag(TAG::VOUCHER_TEXT, "DV");
            }
            content = StrUtils::b64_encode(
                    ConvertCodepage(parser.parse(content),"CP866","UTF-8"));
            SetProp(NewTextChild(resNode, "content", content), "b64", true);
        } else
            throw Exception("reg_no %d not found", reg_no);
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
                map<TDevOper::Enum, TOpsItem>::iterator oi = ops.items.find(DevOperTypes().decode(item.op_type));
                if(
                        oi != ops.items.end()and
                        oi->second.fmt_type == item.fmt_type
                  )
                    prnParams = oi->second.prnParams;
                parser.pts.prn_tag_props.Init(DevOperTypes().decode(item.op_type));
                parser.pts.tag_lang.Init(prnParams.pr_lat);
                data = parser.parse(data);
                TDevOper::Enum dev_oper_type = DevOperTypes().decode(item.op_type);
                TDevFmt::Enum dev_fmt_type = DevFmtTypes().decode(item.fmt_type);
                bool hex=false;
                if(dev_fmt_type == TDevFmt::EPSON) {
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
                  if (dev_oper_type == TDevOper::PrnBP) {
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

void PrintInterface::GetImg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string name = NodeAsString("name", reqNode);
    QParams QryParams;
    QryParams << QParam("name", otString, name);

    TCachedQuery Qry(
            "select data from images_data, images where images.name = :name and images.id = images_data.id order by page_no",
            QryParams
            );

    Qry.get().Execute();

    string result;
    for(; not Qry.get().Eof; Qry.get().Next())
        result += Qry.get().FieldAsString("data");
    if(result.empty())
        throw Exception("image %s not found", name.c_str());
    xmlNodePtr kioskImgNode = NewTextChild(resNode, "kiosk_img");
    NewTextChild(kioskImgNode, "data", result);
}

void PrintInterface::GetTripVouchersSet(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  set<string> trip_vouchers;
  getTripVouchers(point_id, trip_vouchers);
  NewTextChild( resNode, "pr_vouchers", !trip_vouchers.empty() );
}


void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
