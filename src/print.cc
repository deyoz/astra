#include "print.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "misc.h"
#include "stages.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

//////////////////////////////// CLASS PrintDataParser ///////////////////////////////////

class PrintDataParser {
    private:
        class t_field_map {
            private:
                struct TTagValue {
                    bool null;
                    otFieldType type;
                    string StringVal;
                    double FloatVal;
                    int IntegerVal;
                    TDateTime DateTimeVal;
                };

                typedef map<string, TTagValue> TData;
                TData data;
                void dump_data();

                class TPrnQryBuilder {
                    private:
                        TQuery Qry;
                        TQuery prnFields;
                        string name_list, var_list;
                    public:
                        TPrnQryBuilder(int pax_id): Qry(&OraSession), prnFields(&OraSession)
                        {
                            Qry.CreateVariable("PAX_ID", otInteger, pax_id);
                            prnFields.SQLText = "select * from bp_print where 1 = 0";
                            prnFields.Execute();
                        };
                        void set_field(string name, TTagValue &val);
                        TQuery *get();
                } *PrnQryBuilder;

                int pr_lat;
                typedef vector<TQuery*> TQrys;
                TQrys Qrys;

            public:
                t_field_map(int pax_id, int pr_lat, xmlNodePtr tagsNode);
                string get_field(string name, int len, string align, string date_format);
                TQuery *get_prn_qry() { return PrnQryBuilder->get(); };
                ~t_field_map();
        };

        int pr_lat;
        t_field_map field_map;
        string parse_field(int offset, string field);
        string parse_tag(int offset, string tag);
    public:
        PrintDataParser(int pax_id, int pr_lat, xmlNodePtr tagsNode): field_map(pax_id, pr_lat, tagsNode)
        {
            this->pr_lat = pr_lat;
        };
        string parse(string &form);
        TQuery *get_prn_qry() { return field_map.get_prn_qry(); };
};

TQuery *PrintDataParser::t_field_map::TPrnQryBuilder::get()
{
    string qry =
        "insert into bp_print(pax_id, time_print, pr_print" + name_list + ") values(:pax_id, sysdate, 0" + var_list + ")";
    Qry.SQLText = qry;
    return &Qry;
}

void PrintDataParser::t_field_map::TPrnQryBuilder::set_field(string name, TTagValue &val)
{
    if(prnFields.GetFieldIndex(name) != -1 && Qry.Variables->FindVariable(name.c_str()) < 0) {
        name_list += ", " + name;
        var_list += ", :" + name;
        Qry.DeclareVariable(name, val.type);
        switch(val.type) {
            case otString:
            case otChar:
            case otLong:
            case otLongRaw:
                Qry.SetVariable(name, val.StringVal);
                break;
            case otFloat:
                Qry.SetVariable(name, val.FloatVal);
                break;
            case otInteger:
                Qry.SetVariable(name, val.IntegerVal);
                break;
            case otDate:
                Qry.SetVariable(name, val.DateTimeVal);
                break;
        }
    }
}

void PrintDataParser::t_field_map::dump_data()
{
        ProgTrace(TRACE5, "------MAP DUMP------");
        for(TData::iterator di = data.begin(); di != data.end(); ++di) {
            ProgTrace(TRACE5, "data[%s] = %s", di->first.c_str(), di->second.StringVal.c_str());
        }
        ProgTrace(TRACE5, "------MAP DUMP------");
}

string PrintDataParser::t_field_map::get_field(string name, int len, string align, string date_format)
{
    string result;
    TData::iterator di, di_ru;
    while(1) {
        di = data.find(name);
        di_ru = di;
        if(pr_lat && di != data.end()) {
            TData::iterator di_lat = data.find(name + "_LAT");
            if(di_lat != data.end()) di = di_lat;
        }
        if(di != data.end()) break;
        TQrys::iterator ti = Qrys.begin();
        for(; ti != Qrys.end(); ++ti)
            if(!(*ti)->FieldsCount()) break;
        if(ti == Qrys.end())
            throw Exception("Tag not found " + name);
        (*ti)->Execute();
        for(int i = 0; i < (*ti)->FieldsCount(); i++) {
            if(data.find((*ti)->FieldName(i)) != data.end())
                throw Exception((string)"Duplicate field found " + (*ti)->FieldName(i));
            TTagValue TagValue;
            TagValue.null = (*ti)->FieldIsNULL(i);
            ProgTrace(TRACE5, "FIELD: %s", (*ti)->FieldName(i));
            switch((*ti)->FieldType(i)) {
                case otString:
                case otChar:
                case otLong:
                case otLongRaw:
                    ProgTrace(TRACE5, "TYPE %d: otString", (*ti)->FieldType(i));
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

    if(di != data.end()) {
        if(pr_lat && !di_ru->second.null && di->second.null)
            throw Exception("value is empty for " + di->first);
        PrnQryBuilder->set_field(di_ru->first, di_ru->second);
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
                buf << DateTimeToStr(TagValue.DateTimeVal, date_format);
                break;
        }
        if(!len) len = buf.str().size();
        result = AlignString(buf.str(), len, align);
    }
    return result;
}

PrintDataParser::t_field_map::~t_field_map()
{
    for(TQrys::iterator iv = Qrys.begin(); iv != Qrys.end(); ++iv) OraSession.DeleteQuery(**iv);
    delete(PrnQryBuilder);
}

PrintDataParser::t_field_map::t_field_map(int pax_id, int pr_lat, xmlNodePtr tagsNode)
{
    PrnQryBuilder = new TPrnQryBuilder(pax_id);
    this->pr_lat = pr_lat;
    {
        // Положим в мэп теги из клиентского запроса
        xmlNodePtr curNode = tagsNode->children;
        while(curNode) {
            string name = (char *)curNode->name;
            name = upperc(name);
            TTagValue TagValue;
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
    Qry->SQLText =
        "SELECT "
        "   airps.cod air_cod, "
        "   airps.lat air_cod_lat, "
        "   airps.name air_name, "
        "   airps.latname air_name_lat, "
        "   cities.cod city_cod, "
        "   cities.lat city_cod_lat, "
        "   cities.name city_name, "
        "   cities.latname city_name_lat "
        "FROM "
        "   options, "
        "   airps, "
        "   cities "
        "WHERE "
        "   options.cod=airps.cod AND "
        "   airps.city=cities.cod";
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   gtimer.get_stage_time(trips.trip_id,:brd_open_stage_id) brd_from, "
        "   gtimer.get_stage_time(trips.trip_id,:brd_close_stage_id) brd_to, "
        "   trips.TRIP_ID, "
        "   trips.SCD, "
        "   trips.EST, "
        "   trips.ACT, "
        "   trips.company airline, "
        "   avia.latkod airline_lat, "
        "   trips.BC, "
        "   system.transliter(trips.BC, 1) bc_lat, "
        "   trips.BORT, "
        "   system.transliter(trips.BORT, 1) bort_lat, "
        "   trips.TRIPTYPE, "
        "   system.transliter(trips.TRIPTYPE, 1) triptype_lat, "
        "   trips.LITERA, "
        "   system.transliter(trips.LITERA, 1) litera_lat, "
        "   trips.PARK, "
        "   system.transliter(trips.PARK, 1) park_lat, "
        "   trips.MAX_COMMERCE, "
        "   trips.REMARK, "
        "   system.transliter(trips.REMARK, 1) remark_lat, "
        "   trips.MOVE_ROW_ID, "
        "   trips.FLT_NO, "
        "   trips.SUFFIX, "
        "   system.transliter(trips.SUFFIX, 1) suffix_lat, "
        "   trips.COMP_ID, "
        "   trips.PR_ETSTATUS "
        "from "
        "   trips, "
        "   avia "
        "where "
        "   trips.trip_id = :trip_id and "
        "   trips.company = avia.kod_ak ";
    Qry->CreateVariable("trip_id", otInteger, trip_id);
    Qry->CreateVariable("brd_open_stage_id", otInteger, sOpenBoarding);
    Qry->CreateVariable("brd_close_stage_id", otInteger, sCloseBoarding);
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   pax.PAX_ID, "
        "   pax.GRP_ID, "
        "   pax.SURNAME, "
        "   system.transliter(pax.SURNAME, 1) surname_lat, "
        "   pax.NAME, "
        "   system.transliter(pax.NAME, 1) name_lat, "
        "   pax.pers_type pers_type, "
        "   persons.code_lat pers_type_lat, "
        "   persons.name pers_type_name, "
        "   pax.SEAT_NO, "
        "   system.transliter(pax.SEAT_NO, 1) seat_no_lat, "
        "   pax.SEAT_TYPE, "
        "   system.transliter(pax.SEAT_TYPE, 1) seat_type_lat, "
        "   pax.SEATS, "
        "   pax.PR_BRD, "
        "   pax.REFUSE, "
        "   system.transliter(pax.REFUSE, 1) refuse_lat, "
        "   refuse.name refuse_name, "
        "   refuse.lat refuse_name_lat, "
        "   pax.REG_NO, "
        "   pax.TICKET_NO, "
        "   system.transliter(pax.TICKET_NO, 1) ticket_no_lat, "
        "   pax.DOCUMENT, "
        "   system.transliter(pax.DOCUMENT, 1) document_lat, "
        "   pax.SUBCLASS, "
        "   system.transliter(pax.SUBCLASS, 1) subclass_lat, "
        "   pax.DOC_CHECK, "
        "   pax.PREV_SEAT_NO, "
        "   ckin.get_birks(pax.grp_id, pax.reg_no, 0) tags, "
        "   ckin.get_birks(pax.grp_id, pax.reg_no, 1) tags_lat, "
        "   pax.COUPON_NO "
        "from "
        "   pax, "
        "   persons, "
        "   refuse "
        "where "
        "   pax_id = :pax_id and "
        "   pax.pers_type = persons.code  and "
        "   pax.refuse = refuse.cod(+) ";
    Qry->CreateVariable("pax_id", otInteger, pax_id);
    Qrys.push_back(Qry);

    Qry = OraSession.CreateQuery();
    Qry->SQLText =
        "select "
        "   airps.name arvname, "
        "   airps.latname arvname_lat, "
        "   pax_grp.point_id, "
        "   pax_grp.class, "
        "   classes.lat class_lat, "
        "   classes.name class_name, "
        "   classes.name_lat class_name_lat, "
        "   pax_grp.class_bag, "
        "   cl_bag.lat class_bag_lat, "
        "   cl_bag.name class_bag_name, "
        "   cl_bag.name_lat class_bag_name_lat, "
        "   pax_grp.EXCESS, "
        "   pax_grp.PR_WL, "
        "   pax_grp.WL_TYPE, "
        "   pax_grp.WL_NUM, "
        "   pax_grp.HALL, "
        "   pax_grp.PR_REFUSE, "
        "   ckin.get_bagAmount(pax_grp.grp_id, null) bag_amount, "
        "   ckin.get_bagWeight(pax_grp.grp_id, null) bag_weight, "
        "   ckin.get_rkWeight(pax_grp.grp_id, null) rk_weight "
        "from "
        "   pax_grp, "
        "   airps, "
        "   classes, "
        "   classes cl_bag "
        "where "
        "   pax_grp.grp_id = :grp_id and "
        "   pax_grp.target = airps.cod and "
        "   pax_grp.class = classes.id and "
        "   pax_grp.class_bag = cl_bag.id ";
    Qry->CreateVariable("grp_id", otInteger, grp_id);
    Qrys.push_back(Qry);
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
                    result += parse_field(VarPos, tag.substr(VarPos, i - VarPos));
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
    PrintDataParser parser(
            NodeAsInteger("pax_id", reqNode),
            NodeAsInteger("pr_lat", reqNode),
            NodeAsNode("tags", reqNode)
            );
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "print", parser.parse(Print));
    {
        TQuery *Qry = parser.get_prn_qry();
        ProgTrace(TRACE5, "PRN QUERY: %s", Qry->SQLText.SQLText());
        Qry->Execute();
    }
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
