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
                            Qry.CreateVariable("DESK", otString, TReqInfo::Instance()->desk.code);
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
                void add_tag(string name, int val);
                void add_tag(string name, string val);
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
        void add_tag(string name, int val) { return field_map.add_tag(name, val); };
        void add_tag(string name, string val) { return field_map.add_tag(name, val); };
};

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

TQuery *PrintDataParser::t_field_map::TPrnQryBuilder::get()
{
    string qry =
        "begin "
        "   delete from bp_print where pax_id = :pax_id and pr_print = 0 and desk=:desk; "
        "   insert into bp_print(pax_id, time_print, pr_print, desk" + name_list + ") "
        "   values(:pax_id, :now_utc, 0, :desk" + var_list + "); "
        "end;";
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
//            break;
            throw Exception("Tag not found " + name);
        (*ti)->Execute();
        for(int i = 0; i < (*ti)->FieldsCount(); i++) {
            if(data.find((*ti)->FieldName(i)) != data.end())
                throw Exception((string)"Duplicate field found " + (*ti)->FieldName(i));
            TTagValue TagValue;
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
                buf << DateTimeToStr(TagValue.DateTimeVal, date_format, pr_lat);
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
    if(tagsNode) {
        // ������� � �� ⥣� �� ������᪮�� �����
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
        "   system.localsysdate issued, "
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
        "   avia.ak_name airline_name, "
        "   avia.latname airline_name_lat, "
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
        "   pax_grp.target airp_arv, "
        "   airps.lat airp_arv_lat, "
        "   airps.name airp_arv_name, "
        "   airps.latname airp_arv_name_lat, "
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
    if(Qry.Eof) throw UserException("�� ३� �� �����祭 ����� ��ᠤ���� ⠫����");
    Pectab = Qry.FieldAsString("form");
    Print = Qry.FieldAsString("prn_form");
}

void GetPrintDataBP(xmlNodePtr dataNode, int pax_id, int prn_type, int pr_lat, xmlNodePtr clientDataNode)
{
    xmlNodePtr BPNode = NewTextChild(dataNode, "BP");
    TQuery Qry(&OraSession);
    Qry.SQLText = "select grp_id from pax where pax_id = :pax_id";
    Qry.CreateVariable("pax_id", otInteger, pax_id);
    Qry.Execute();
    string Pectab, Print;
    GetPrintData(Qry.FieldAsInteger("grp_id"), prn_type, Pectab, Print);
    NewTextChild(BPNode, "pectab", Pectab);
    PrintDataParser parser(pax_id, pr_lat, clientDataNode);
    xmlNodePtr prnFormsNode = NewTextChild(BPNode, "prn_forms");
    xmlNodePtr prnFormNode = NewTextChild(prnFormsNode, "prn_form", parser.parse(Print));
    {
        TQuery *Qry = parser.get_prn_qry();
        TDateTime time_print = NowUTC();
        Qry->CreateVariable("now_utc", otDate, time_print);
        ProgTrace(TRACE5, "PRN QUERY: %s", Qry->SQLText.SQLText());
        Qry->Execute();
        SetProp(prnFormNode, "pax_id", pax_id);
        SetProp(prnFormNode, "time_print", DateTimeToStr(time_print, ServerFormatDateTimeAsString));
    }
}

void GetPrintDataBP(xmlNodePtr dataNode, int grp_id, int prn_type, int pr_lat, bool pr_all, xmlNodePtr clientDataNode)
{
    string Pectab, Print;
    GetPrintData(grp_id, prn_type, Pectab, Print);
    xmlNodePtr BPNode = NewTextChild(dataNode, "BP");
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
            "   bp_print.pr_print(+) <> 0 "
            "order by "
            "   pax.reg_no";
    Qry.CreateVariable("grp_id", otInteger, grp_id);
    Qry.Execute();
    xmlNodePtr prnFormsNode = NewTextChild(BPNode, "prn_forms");
    while(!Qry.Eof) {
        int pax_id = Qry.FieldAsInteger("pax_id");
        PrintDataParser parser(pax_id, pr_lat, clientDataNode);
        xmlNodePtr prnFormNode = NewTextChild(prnFormsNode, "prn_form", parser.parse(Print));
        {
            TQuery *Qry = parser.get_prn_qry();
            TDateTime time_print = NowUTC();
            Qry->CreateVariable("now_utc", otDate, time_print);
            Qry->Execute();
            SetProp(prnFormNode, "pax_id", pax_id);
            SetProp(prnFormNode, "time_print", DateTimeToStr(time_print, ServerFormatDateTimeAsString));
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

void get_bt_forms(string tag_type, xmlNodePtr pectabsNode, vector<string> &prn_forms)
{
    prn_forms.clear();
    TQuery FormsQry(&OraSession);        
    FormsQry.SQLText = "select id, num, form, prn_form from bt_forms where tag_type = :tag_type order by id, num";
    FormsQry.CreateVariable("TAG_TYPE", otString, tag_type);
    FormsQry.Execute();
    if(FormsQry.Eof) throw Exception("bt_id not found (tag_type = " + tag_type);
    while(!FormsQry.Eof) {
        NewTextChild(pectabsNode, "pectab", FormsQry.FieldAsString("form"));
        prn_forms.push_back(FormsQry.FieldAsString("prn_form"));
        FormsQry.Next();
    }
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
        "   trips.scd,  "
        "   null local_date,  "
        "   trips.company airline,  "
        "   avia.latkod airline_lat,  "
        "   trips.flt_no,  "
        "   pax_grp.target airp_arv,  "
        "   airps.lat airp_arv_lat, "
        "   airps.name airp_arv_name, "
        "   airps.latname airp_arv_name_lat "
        "from  "
        "   pax_grp,  "
        "   trips,  "
        "   avia,  "
        "   airps  "
        "where  "
        "   pax_grp.grp_id = :grp_id and  "
        "   pax_grp.point_id = trips.trip_id and  "
        "   trips.company = avia.kod_ak and  "
        "   pax_grp.target = airps.cod "
        "union  "
        "select  "
        "   null,  "
        "   transfer.local_date,  "
        "   transfer.airline,  "
        "   avia.latkod airline_lat,  "
        "   transfer.flt_no,  "
        "   transfer.airp_arv,  "
        "   airps.lat airp_arv_lat,  "
        "   airps.name airp_arv_name, "
        "   airps.latname airp_arv_name_lat "
        "from  "
        "   transfer,  "
        "   avia,  "
        "   airps  "
        "where  "
        "   transfer.grp_id = :grp_id and  "
        "   transfer.airline = avia.kod_ak and  "
        "   transfer.airp_arv = airps.cod ";
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

void GetPrintDataBT(xmlNodePtr dataNode, int grp_id, int pr_lat)
{
    vector<TBTRouteItem> route;
    get_route(grp_id, route);
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "select pax_id from pax where grp_id = :grp_id and refuse is null and reg_no = "
        "(select min(reg_no) from pax where grp_id = :grp_id and refuse is null)";
    Qry.CreateVariable("GRP_ID", otInteger, grp_id);
    Qry.Execute();
    int pax_id = Qry.FieldAsInteger(0);
    Qry.SQLText = "select no, tag_type from bag_tags where grp_id = :grp_id and pr_print = 0 order by tag_type, num";
    Qry.Execute();
    string tag_type;
    vector<string> prn_forms;
    xmlNodePtr printBTNode = NewTextChild(dataNode, "printBT");
    xmlNodePtr pectabsNode = NewTextChild(printBTNode, "pectabs");
    xmlNodePtr tagsNode = NewTextChild(printBTNode, "tags");
    while(!Qry.Eof) {
        string tmp_tag_type = Qry.FieldAsString("tag_type");
        if(tag_type != tmp_tag_type) {
            tag_type = tmp_tag_type;
            get_bt_forms(tag_type, pectabsNode, prn_forms);
        }

        int tag_no = Qry.FieldAsInteger("no");
        int aircode = tag_no / 1000000;
        int no = tag_no % 1000000;

        xmlNodePtr tagNode = NewTextChild(tagsNode, "tag");
        SetProp(tagNode, "type", tag_type);
        SetProp(tagNode, "no", tag_no);

        PrintDataParser parser(pax_id, pr_lat, NULL);

        parser.add_tag("aircode", aircode);
        parser.add_tag("no", no);

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

void PrintInterface::GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    GetPrintDataBT(dataNode, NodeAsInteger("grp_id", reqNode), NodeAsInteger("pr_lat", reqNode));
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
