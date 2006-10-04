#include "print.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "misc.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

//////////////////////////////// CLASS PrintDataParser ///////////////////////////////////

class PrintDataParser {
    private:
        typedef vector<TQuery*> TTags;

        class t_field_map {
            private:
                TTags Tags;
                vector<string> unique_tag_names;
            public:
                t_field_map(int pax_id);
                string get_field(string name, int len, string align, string date_format);
                ~t_field_map();
        };

        t_field_map field_map;
        string parse_field(int offset, string field);
    public:
        PrintDataParser(int pax_id): field_map(pax_id) {};
        string parse(string &form);
};

string PrintDataParser::t_field_map::get_field(string name, int len, string align, string date_format)
{
    string result;
    TTags::iterator iv = Tags.begin();
    for(; iv != Tags.end(); ++iv) {
        if(!(*iv)->FieldsCount()) {
            (*iv)->Execute();
            // uniqueness check
            for(int i = 0; i < (*iv)->FieldsCount(); i++) {
                if(find(unique_tag_names.begin(), unique_tag_names.end(), (*iv)->FieldName(i)) != unique_tag_names.end())
                    throw Exception((string)"Duplicate field found " + (*iv)->FieldName(i));
                unique_tag_names.push_back((*iv)->FieldName(i));
            }
        }
        int i = 0;
        for(; i < (*iv)->FieldsCount(); i++) if((*iv)->FieldName(i) == name) break;
        if(i == (*iv)->FieldsCount()) continue;

        ostringstream buf;
        buf.width(len);
        switch((*iv)->FieldType(i)) {
            case otString:
            case otChar:
            case otLong:
            case otLongRaw:
                buf.fill(' ');
                buf << (*iv)->FieldAsString(i);
                break;
            case otFloat:
                buf.fill('0');
                buf << (*iv)->FieldAsFloat(i);
                break;
            case otInteger:
                buf.fill('0');
                buf << (*iv)->FieldAsInteger(i);
                break;
            case otDate:
                buf << DateTimeToStr((*iv)->FieldAsDateTime(i), date_format);
                break;
        }
        if(!len) len = buf.str().size();
        result = AlignString(buf.str(), len, align);
    }
    return result;
}

PrintDataParser::t_field_map::~t_field_map()
{
    for(TTags::iterator iv = Tags.begin(); iv != Tags.end(); ++iv) OraSession.DeleteQuery(**iv);
}

PrintDataParser::t_field_map::t_field_map(int pax_id)
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
            "      point_id into :trip_id "
            "   from "
            "      pax_grp "
            "   where "
            "      grp_id = :grp_id; "
            "end;";
        Qry.DeclareVariable("pax_id", otInteger);
        Qry.DeclareVariable("grp_id", otInteger);
        Qry.DeclareVariable("trip_id", otInteger);
        Qry.SetVariable("pax_id", pax_id);
        Qry.Execute();
        grp_id = Qry.GetVariableAsInteger("grp_id");
        trip_id = Qry.GetVariableAsInteger("trip_id");
    }

    TQuery *Qry = OraSession.CreateQuery();
    Qry->SQLText = "SELECT airps.cod AS air_cod,airps.lat AS air_cod_lat,airps.name AS air_name, "\
                  "       cities.cod AS city_cod,cities.name AS city_name,SYSDATE "\
                  "FROM options,airps,cities "\
                  "WHERE options.cod=airps.cod AND airps.city=cities.cod";
    Tags.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   TRIP_ID, "
        "   TRIP, "
        "   SCD, "
        "   EST, "
        "   ACT, "
        "   COMPANY, "
        "   BC, "
        "   BORT, "
        "   TRIPTYPE, "
        "   LITERA, "
        "   PARK, "
        "   MAX_COMMERCE, "
        "   REMARK, "
        "   MOVE_ROW_ID, "
        "   FLT_NO, "
        "   SUFFIX, "
        "   COMP_ID, "
        "   PR_ETSTATUS "
        "from "
        "   trips "
        "where "
        "   trip_id = :trip_id";
    Qry->CreateVariable("trip_id", otInteger, trip_id);
    Tags.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   PAX_ID, "
        "   GRP_ID, "
        "   SURNAME, "
        "   NAME, "
        "   PERS_TYPE, "
        "   SEAT_NO, "
        "   SEAT_TYPE, "
        "   SEATS, "
        "   PR_BRD, "
        "   REFUSE, "
        "   REG_NO, "
        "   TICKET_NO, "
        "   DOCUMENT, "
        "   SUBCLASS, "
        "   DOC_CHECK, "
        "   PREV_SEAT_NO, "
        "   COUPON_NO "
        "from "
        "   pax "
        "where "
        "   pax_id = :pax_id";
    Qry->CreateVariable("pax_id", otInteger, pax_id);
    Tags.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   airps.name arvname, "
        "   airps.latname lat_arvname, "
        "   pax_grp.POINT_ID, "
        "   pax_grp.TARGET, "
        "   pax_grp.CLASS, "
        "   pax_grp.CLASS_BAG, "
        "   pax_grp.EXCESS, "
        "   pax_grp.PR_WL, "
        "   pax_grp.WL_TYPE, "
        "   pax_grp.WL_NUM, "
        "   pax_grp.HALL, "
        "   pax_grp.PR_REFUSE "
        "from "
        "   pax_grp, "
        "   airps "
        "where "
        "   pax_grp.grp_id = :grp_id and "
        "   pax_grp.target = airps.cod ";
    Qry->CreateVariable("grp_id", otInteger, grp_id);
    Tags.push_back(Qry);
}

string PrintDataParser::parse_field(int offset, string field)
{
    char Mode = 'S';
    string::size_type VarPos = 0;

    string FieldName = upperc(field);
    int FieldLen = 0;
    string FieldAlign = "L";
    string DateFormat = ServerFormatDateTimeAsString;

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
                if(!IsLetter(curr_char) && curr_char != '_')
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
                    if(buf.size()) DateFormat = buf;
                    Mode = 'F';
                }
                break;
        }
    }
    if(Mode != 'L' && Mode != 'F')
            throw Exception("')' not found at " + IntToString(offset + i + 1));
    return field_map.get_field(FieldName, FieldLen, FieldAlign, DateFormat);
}

string PrintDataParser::parse(string &form)
{
    string result;
    char Mode = 'S';
    string::size_type VarPos = 0;
    for(string::size_type i = 0; i < form.size(); i++) {
        switch(Mode) {
            case 'S':
                if(form[i] == '>')
                    Mode = 'R';
                else if(form[i] == '<') {
                    Mode = 'Q';
                } else
                    result += form[i];
                break;
            case 'Q':
                if(form[i] == '<') {
                    result += '<';
                    Mode = 'S';
                } else {
                    VarPos = i;
                    Mode = 'L';
                }
                break;
            case 'L':
                if(form[i] == '>') {
                    result += parse_field(VarPos, form.substr(VarPos, i - VarPos));
                    Mode = 'S';
                }
                break;
            case 'R':
                if(form[i] == '>') {
                    result += '>';
                    Mode = 'S';
                } else
                    throw Exception("2nd '>' not found at " + IntToString(i + 1));
                break;
        }
    }
    return result;
}

//////////////////////////////// END CLASS PrintDataParser ///////////////////////////////////

void GetPrintData(int grp_id, int prn_type, string &Pectab, string &Print)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText = "select point_id, class from pax_grp where grp_id = :grp_id";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    int trip_id = Qry.FieldAsInteger("point_id");
    string cl = Qry.FieldAsString("class");
    Qry.Clear();
    Qry.SQLText = 
        "SELECT bp_id, form, prn_form FROM trip_bp tb, bp_forms bf "
        "WHERE bp_id = id and "
        "point_id=:point_id AND (class IS NULL OR class=:class) AND tb.pr_ier=:prn_type "
        "ORDER BY class ";
    Qry.CreateVariable("point_id", otInteger, trip_id);
    Qry.CreateVariable("class", otString, cl);
    Qry.CreateVariable("prn_type", otInteger, prn_type);
    Qry.Execute();
    if(Qry.Eof) throw UserException("На рейс не назначен бланк посадочных талонов");
    Pectab = Qry.FieldAsString("form");
    Print = Qry.FieldAsString("prn_form");
}

void PrintInterface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText = "select grp_id from pax where pax_id = :pax_id";
    Qry.CreateVariable("pax_id", otInteger, NodeAsInteger("pax_id", reqNode));
    Qry.Execute();
    string Pectab, Print;
    GetPrintData(Qry.FieldAsInteger("grp_id"), NodeAsInteger("prn_type", reqNode), Pectab, Print);
    PrintDataParser parser(NodeAsInteger("pax_id", reqNode));
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "print", parser.parse(Print));
}

void PrintInterface::GetPectabDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string Pectab, Print;
    GetPrintData(NodeAsInteger("grp_id", reqNode), NodeAsInteger("prn_type", reqNode), Pectab, Print);
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "pectab", Pectab);
}

void PrintInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
