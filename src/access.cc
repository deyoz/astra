#include "access.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_utils.h"
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "cache.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;

enum TRightState {rsOn, rsOff};
enum TRightListType{rltRights, rltAssignRights, rltNone};
const char *TRightListTypeS[] = {"RIGHTS", "ASSIGN", ""};

TRightListType DecodeRightListType(string s)
{
  unsigned int i;
  for(i=0;i<sizeof(TRightListTypeS)/sizeof(TRightListTypeS[0]);i+=1) if (s == TRightListTypeS[i]) break;
  if (i<sizeof(TRightListTypeS)/sizeof(TRightListTypeS[0]))
    return (TRightListType)i;
  else
    return rltNone;
}

string EncodeRightListType(TRightListType s)
{
  return TRightListTypeS[s];
};

string get_rights_table(TRightListType rlt)
{
    string table;
    switch(rlt) {
        case rltRights:
            table = "role_rights";
            break;
        case rltAssignRights:
            table = "role_assign_rights";
            break;
        default:
            throw Exception("AccessInterface::RoleRights: unexpected TRightListType: %d", rlt);
    }
    return table;
}

void AccessInterface::SaveRoleRights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    int role_id = NodeAsInteger("role_id", reqNode);
    string table = get_rights_table(DecodeRightListType(NodeAsString("rlt", reqNode)));
    xmlNodePtr itemNode = NodeAsNode("items", reqNode)->children;
    TQuery Qry(&OraSession);
    Qry.CreateVariable("user_id", otInteger, info.user.user_id);
    Qry.CreateVariable("role_id", otInteger, role_id);
    Qry.DeclareVariable("right_id", otInteger);
    for(; itemNode; itemNode = itemNode->next) {
        xmlNodePtr dataNode = itemNode->children;
        Qry.SetVariable("right_id", NodeAsIntegerFast("id", dataNode));
        TRightState state = TRightState(NodeAsIntegerFast("state", dataNode));
        string SQLText;
        switch(state) {
            case rsOn:
                SQLText =
                    "BEGIN "
                    "  :role_id:=adm.check_role_access(:role_id,:user_id,1); "
                    "  :right_id:=adm.check_right_access(:right_id,:user_id,1); "
                    "  INSERT INTO " + table + "(role_id,right_id) VALUES(:role_id,:right_id); "
                    "END;";
                break;
            case rsOff:
                SQLText =
                    "BEGIN "
                    "  :role_id:=adm.check_role_access(:role_id,:user_id,2); "
                    "  :right_id:=adm.check_right_access(:right_id,:user_id,2); "
                    "  DELETE FROM " + table + " "
                    "  WHERE role_id=:role_id AND right_id=:right_id; "
                    "END;";
                break;
        }
        Qry.SQLText = SQLText;
        try {
        Qry.Execute();
        } catch(EOracleError E) {
          if ( E.Code >= 20000 ) {
            string str = E.what();
            EOracleError2UserException(str);
            throw UserException( str );
          } else
              throw;
        }
    }
}

void AccessInterface::CmpRole(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int role_id = NodeAsInteger("role", reqNode);
    string login = NodeAsString("login", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText = "select user_id from users2 where login = :login";
    Qry.CreateVariable("login", otString, login);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Неверно введен логин пользователя");
    int user_id = Qry.FieldAsInteger("user_id");
    Qry.Clear();
    Qry.SQLText = "select adm.check_role_aro_access(:role_id, :user_id) from dual";
    Qry.CreateVariable("role_id", otInteger, role_id);
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.Execute();
    if(Qry.FieldAsInteger(0) == 0)
        NewTextChild(resNode, "aro", "Пользователю запрещен доступ к а/к или а/п роли");
    Qry.SQLText =
        "SELECT\n"
        "    rights_list.name\n"
        "FROM\n"
        "  rights_list, "
        "  (SELECT role_rights.right_id\n"
        "   FROM role_rights\n"
        "   WHERE role_id=:role_id\n"
        "   UNION\n"
        "   SELECT role_assign_rights.right_id\n"
        "   FROM role_assign_rights\n"
        "   WHERE role_id=:role_id) role_rights,\n"
        "  (SELECT role_assign_rights.right_id\n"
        "   FROM user_roles,role_assign_rights\n"
        "   WHERE user_roles.role_id=role_assign_rights.role_id AND\n"
        "         user_roles.user_id=:user_id) user_rights\n"
        "WHERE role_rights.right_id=user_rights.right_id(+) AND\n"
        "      user_rights.right_id IS NULL and\n"
        "      role_rights.right_id = rights_list.ida\n";
    Qry.Execute();
    if(not Qry.Eof) {
        xmlNodePtr rightsNode = NewTextChild(resNode, "rights");
        for(; not Qry.Eof; Qry.Next())
            NewTextChild(rightsNode, "item", Qry.FieldAsString("name"));

    }

}

void AccessInterface::Clone(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int src_role = NodeAsInteger("src_role", reqNode);
    int dst_role = NodeAsInteger("dst_role", reqNode);
    int pr_force = NodeAsInteger("pr_force", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  delete from role_rights where role_id = :dst_role; "
        "  if :pr_force = 0 and sql%rowcount > 0 then "
        "    raise_application_error(-20000, 'role_rights found'); "
        "  end if; "
        "  delete from role_assign_rights where role_id = :dst_role; "
        "  if :pr_force = 0 and sql%rowcount > 0 then "
        "    raise_application_error(-20000, 'role_assign_rights found'); "
        "  end if; "
        "  insert into role_rights(role_id, right_id) select :dst_role, right_id from role_rights where role_id = :src_role; "
        "  insert into role_assign_rights(role_id, right_id) select :dst_role, right_id from role_assign_rights where role_id = :src_role; "
        "end; ";
    Qry.CreateVariable("src_role", otInteger, src_role);
    Qry.CreateVariable("dst_role", otInteger, dst_role);
    Qry.CreateVariable("pr_force", otInteger, pr_force);
    try {
        Qry.Execute();
    } catch(EOracleError E) {
        if ( E.Code == 20000 )
            NewTextChild(resNode, "alert");
        else
            throw;
    }
}

void AccessInterface::RoleRights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string table = get_rights_table(DecodeRightListType(NodeAsString("rlt", reqNode)));
    TQuery Qry(&OraSession);
    string SQLText =
        "select "
        "   rights_list.ida, "
        "   rights_list.name "
        "from "
        "   " + table + ", "
        "   rights_list "
        "where "
        "   " + table + ".role_id = :role_id and "
        "   " + table + ".right_id=rights_list.ida and "
        "   adm.check_role_view_access(" + table + ".role_id, :user_id)<>0 "
        "order by "
        "   ida ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("role_id", otInteger, NodeAsInteger("role_id", reqNode));
    Qry.CreateVariable("user_id", otInteger, info.user.user_id);
    Qry.Execute();
    xmlNodePtr roleRightsNode = NULL;
    for(; !Qry.Eof; Qry.Next()) {
        if(!roleRightsNode)
            roleRightsNode = NewTextChild(resNode, "role_rights");
        xmlNodePtr itemNode = NewTextChild(roleRightsNode, "item");
        NewTextChild(itemNode, "id", Qry.FieldAsInteger("ida"));
        NewTextChild(itemNode, "name", Qry.FieldAsString("name"));
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

struct TUserData {
    int user_id;
    string descr;
    string login;
    int user_type;
    int time_fmt;
    int airline_fmt;
    int airp_fmt;
    int craft_fmt;
    int suff_fmt;
    vector<string> airps, airlines;
    vector<int> roles;
    int pr_denial;
    void initXML(xmlNodePtr node);
    void insert();
    void update();
    void del();
    void create_vars(TQuery &Qry, bool pr_update = false);
    void create_var(TQuery &Qry, string name, int val);
    TUserData():
        user_id(-1),
        user_type(-1),
        time_fmt(-1),
        airline_fmt(-1),
        airp_fmt(-1),
        craft_fmt(-1),
        suff_fmt(-1),
        pr_denial(-1)
    {};
};

void TUserData::create_vars(TQuery &Qry, bool pr_update)
{
    Qry.CreateVariable("login", otString, login);
    if(not pr_update)
        Qry.CreateVariable("descr", otString, descr);
    Qry.CreateVariable("type_code", otString, user_type);
    Qry.CreateVariable("pr_denial", otString, pr_denial);
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
    create_var(Qry, "time_fmt_code", time_fmt);
    create_var(Qry, "disp_airline_fmt_code", airline_fmt);
    create_var(Qry, "disp_airp_fmt_code", airp_fmt);
    create_var(Qry, "disp_craft_fmt_code", craft_fmt);
    create_var(Qry, "disp_suffix_fmt_code", suff_fmt);
}

void TUserData::create_var(TQuery &Qry, string name, int val)
{
    if(val < 0)
        Qry.CreateVariable(name, otInteger, FNull);
    else
        Qry.CreateVariable(name, otInteger, val);
}

void TUserData::del()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        "  adm.delete_user(:OLD_user_id,:SYS_user_id); "
        "END;";
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
    Qry.CreateVariable("OLD_user_id", otInteger, user_id);
    Qry.Execute();
}

void TUserData::update()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        "  adm.update_user(:OLD_user_id,:login,:type_code,:pr_denial,:SYS_user_id, "
        "                  :time_fmt_code,:disp_airline_fmt_code,:disp_airp_fmt_code,:disp_craft_fmt_code,:disp_suffix_fmt_code); "
        "END;";
    create_vars(Qry, true);
    Qry.CreateVariable("OLD_user_id", otInteger, user_id);
    Qry.Execute();
}

void TUserData::insert()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        "  adm.insert_user(:login,:descr,:type_code,:pr_denial,:SYS_user_id, "
        "                  :time_fmt_code,:disp_airline_fmt_code,:disp_airp_fmt_code,:disp_craft_fmt_code,:disp_suffix_fmt_code); "
        "  select user_id into :user_id from users2 where login = :login; "
        "END;";
    create_vars(Qry);
    Qry.DeclareVariable("user_id", otInteger);
    Qry.Execute();
    user_id = Qry.GetVariableAsInteger("user_id");
}

void TUserData::initXML(xmlNodePtr node)
{
    if(node != NULL) {
        user_id = NodeAsIntegerFast("user_id", node, -1);
        descr = NodeAsStringFast("descr", node, "");
        login = NodeAsStringFast("login", node, "");
        user_type = NodeAsIntegerFast("user_type", node, -1);
        time_fmt = NodeAsIntegerFast("time_fmt", node, -1);
        airline_fmt = NodeAsIntegerFast("airline_fmt", node, -1);
        airp_fmt = NodeAsIntegerFast("airp_fmt", node, -1);
        craft_fmt = NodeAsIntegerFast("craft_fmt", node, -1);
        suff_fmt = NodeAsIntegerFast("suff_fmt", node, -1);
        xmlNodePtr node2 = GetNodeFast("airps", node);
        if(node2 != NULL) {
            node2 = node2->children;
            for(; node2; node2 = node2->next)
                airps.push_back(NodeAsString(node2));
        }
        node2 = GetNodeFast("airlines", node);
        if(node2 != NULL) {
            node2 = node2->children;
            for(; node2; node2 = node2->next)
                airlines.push_back(NodeAsString(node2));
        }
        pr_denial = NodeAsIntegerFast("pr_denial", node, -1);
    }
}

class TAirps:vector<string> {
    public:
        bool cmp(vector<string> val) {
            bool Result;
            if(size() != val.size())
                Result = false;
            else {
                Result = true;
                for(vector<string>::iterator iv = begin(); iv != end(); iv++) {
                    vector<string>::iterator iv1 = val.begin();
                    for(; iv1 != val.end(); iv1++) {
                        if(*iv == *iv1)
                            break;
                    }
                    if(iv1 == val.end()) {
                        Result = false;
                        break;
                    }
                }
            }
            return Result;
        }
        void to_xml(xmlNodePtr node) {
            if(not empty()) {
                node = NewTextChild(node, "airps");
                for(vector<string>::iterator iv = begin(); iv != end(); iv++) {
                    NewTextChild(node, "item", *iv);
                }
            }
        }
        void get(int user_id) {
            ProgTrace(TRACE5, "WITHIN TAIRPS");
            TQuery Qry(&OraSession);
            Qry.SQLText = "select airp from aro_airps where aro_id = :user_id";
            Qry.CreateVariable("user_id", otInteger, user_id);
            Qry.Execute();
            for(; !Qry.Eof; Qry.Next()) {
                push_back(Qry.FieldAsString(0));
                ProgTrace(TRACE5, "AIRP: %s", back().c_str());
            }
        }
};

class TAirlines:vector<string> {
    public:
        bool cmp(vector<string> val) {
            bool Result;
            if(size() != val.size())
                Result = false;
            else {
                Result = true;
                for(vector<string>::iterator iv = begin(); iv != end(); iv++) {
                    vector<string>::iterator iv1 = val.begin();
                    for(; iv1 != val.end(); iv1++) {
                        if(*iv == *iv1)
                            break;
                    }
                    if(iv1 == val.end()) {
                        Result = false;
                        break;
                    }
                }
            }
            return Result;
        }
        void to_xml(xmlNodePtr node) {
            if(not empty()) {
                node = NewTextChild(node, "airlines");
                for(vector<string>::iterator iv = begin(); iv != end(); iv++) {
                    NewTextChild(node, "item", *iv);
                }
            }
        }
        TAirlines(int user_id) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select airline from aro_airlines where aro_id = :user_id";
            Qry.CreateVariable("user_id", otInteger, user_id);
            Qry.Execute();
            for(; Qry.Eof; Qry.Next())
                push_back(Qry.FieldAsString(0));
        }
};

class TRoles:vector<int> {
    public:
        bool cmp(vector<int> val) {
            bool Result;
            if(size() != val.size())
                Result = false;
            else {
                Result = true;
                for(vector<int>::iterator iv = begin(); iv != end(); iv++) {
                    vector<int>::iterator iv1 = val.begin();
                    for(; iv1 != val.end(); iv1++) {
                        if(*iv == *iv1)
                            break;
                    }
                    if(iv1 == val.end()) {
                        Result = false;
                        break;
                    }
                }
            }
            return Result;
        }
        void to_xml(xmlNodePtr node) {
            if(not empty()) {
                node = NewTextChild(node, "roles");
                for(vector<int>::iterator iv = begin(); iv != end(); iv++) {
                    NewTextChild(node, "item", *iv);
                }
            }
        }
        TRoles(int user_id) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select role_id from user_roles where user_id = :user_id";
            Qry.CreateVariable("user_id", otInteger, user_id);
            Qry.Execute();
            for(; Qry.Eof; Qry.Next())
                push_back(Qry.FieldAsInteger(0));
        }
};

void AccessInterface::ApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children->children;
    map<int, TUserData> inserted;
    for(; node; node = node->next) {
        string buf = NodeAsString("@status", node);
        TCacheUpdateStatus status;
        if(buf == "inserted")
            status = usInserted;
        else if(buf == "modified")
            status = usModified;
        else if(buf == "deleted")
            status = usDeleted;
        else
            throw Exception("AccessInterface::ApplyUpdates: unknown status '%s'", buf.c_str());
        TUserData user_data;
        user_data.initXML(node->children);
        switch(status) {
            case usInserted:
                user_data.insert();
                inserted[NodeAsInteger("@index", node)] = user_data;
                break;
            case usUnmodified:
                break;
            case usModified:
                user_data.update();
                break;
            case usDeleted:
                user_data.del();
                break;
        }
    }
    xmlNodePtr usersNode = NULL;
    for(map<int, TUserData>::iterator im = inserted.begin(); im != inserted.end(); im++) {
        if(usersNode == NULL)
            usersNode = NewTextChild(resNode, "users");
        xmlNodePtr itemNode = NewTextChild(usersNode, "item");
        SetProp(itemNode, "index", im->first);
        NewTextChild(itemNode, "user_id", im->second.user_id);
        TAirps airps;
        airps.get(im->second.user_id);
        TAirlines airlines(im->second.user_id);
        airps.to_xml(itemNode);
        airlines.to_xml(itemNode);
    }
}

void AccessInterface::SearchUsers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TUserData user_data;
    user_data.initXML(reqNode->children);

    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "       users2.user_id, "
        "       login, "
        "       descr, "
        "       user_types.code AS type, "
        "       pr_denial, "
        "       time_fmt.code AS time_fmt_code, "
        "       disp_airline_fmt.code AS disp_airline_fmt_code, "
        "       disp_airp_fmt.code AS disp_airp_fmt_code, "
        "       disp_craft_fmt.code AS disp_craft_fmt_code, "
        "       disp_suffix_fmt.code AS disp_suffix_fmt_code "
        "FROM users2,user_types,user_sets, "
        "     user_set_types time_fmt, "
        "     user_set_types disp_airline_fmt, "
        "     user_set_types disp_airp_fmt, "
        "     user_set_types disp_craft_fmt, "
        "     user_set_types disp_suffix_fmt  "
        "WHERE users2.type=user_types.code AND  "
        "      users2.user_id=user_sets.user_id(+) AND "
        "      DECODE(user_sets.time,        00,00,01,01,02,02,01)=time_fmt.code AND  "
        "      DECODE(user_sets.disp_airline,05,05,06,06,07,07,08,08,09,09,09)=disp_airline_fmt.code AND  "
        "      DECODE(user_sets.disp_airp,   05,05,06,06,07,07,08,08,09,09,09)=disp_airp_fmt.code AND "
        "      DECODE(user_sets.disp_craft,  05,05,06,06,07,07,08,08,09,09,09)=disp_craft_fmt.code AND "
        "      DECODE(user_sets.disp_suffix, 15,15,16,16,17,17,17)=disp_suffix_fmt.code AND "
        "      adm.check_user_view_access(users2.user_id,:SYS_user_id)<>0 ";
    if(not user_data.descr.empty()) {
        SQLText += " and users2.descr like :descr||'%' ";
        Qry.CreateVariable("descr", otString, user_data.descr);
    }
    if(not user_data.login.empty()) {
        SQLText += " and users2.descr like :login||'%' ";
        Qry.CreateVariable("login", otString, user_data.login);
    }
    if(user_data.user_type >= 0) {
        SQLText += " and users2.type = :user_type ";
        Qry.CreateVariable("user_type", otString, user_data.user_type);
    }
    if(user_data.airline_fmt >= 0) {
        SQLText += " and disp_airline_fmt.code = :airline_fmt ";
        Qry.CreateVariable("airline_fmt", otInteger, user_data.airline_fmt);
    }
    if(user_data.airp_fmt >= 0) {
        SQLText += " and disp_airp_fmt.code = :airp_fmt ";
        Qry.CreateVariable("airp_fmt", otInteger, user_data.airp_fmt);
    }
    if(user_data.craft_fmt >= 0) {
        SQLText += " and disp_craft_fmt.code = :craft_fmt ";
        Qry.CreateVariable("craft_fmt", otInteger, user_data.craft_fmt);
    }
    if(user_data.suff_fmt >= 0) {
        SQLText += " and disp_suffix_fmt.code = :suff_fmt ";
        Qry.CreateVariable("suff_fmt", otInteger, user_data.suff_fmt);
    }
    if(user_data.pr_denial >= 0) {
        SQLText += " and users2.pr_denial = :pr_denial ";
        Qry.CreateVariable("pr_denial", otInteger, user_data.pr_denial);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
    Qry.Execute();
    if(!Qry.Eof) {
        xmlNodePtr rowsNode = NewTextChild(resNode, "users");
        int col_user_id = Qry.FieldIndex("user_id");
        int col_login = Qry.FieldIndex("login");
        int col_descr = Qry.FieldIndex("descr");
        int col_type = Qry.FieldIndex("type");
        int col_pr_denial = Qry.FieldIndex("pr_denial");
        int col_time_fmt_code = Qry.FieldIndex("time_fmt_code");
        int col_disp_airline_fmt_code = Qry.FieldIndex("disp_airline_fmt_code");
        int col_disp_airp_fmt_code = Qry.FieldIndex("disp_airp_fmt_code");
        int col_disp_craft_fmt_code = Qry.FieldIndex("disp_craft_fmt_code");
        int col_disp_suffix_fmt_code = Qry.FieldIndex("disp_suffix_fmt_code");
        for(; !Qry.Eof; Qry.Next()) {
            ProgTrace(TRACE5, "CHECK 0");
            int user_id = Qry.FieldAsInteger(col_user_id);
//            ProgTrace(TRACE5, "user_id: %d", user_id);
            TAirps airps;
//            airps.get(user_id);
//            TAirlines airlines(user_id);
//            TRoles roles(user_id);
//            ProgTrace(TRACE5, "CHECK 1");
//            if(
//                    not (
//                        airps.cmp(user_data.airps) and
//                        airlines.cmp(user_data.airlines) and
//                        roles.cmp(user_data.roles)
//                        )
//              )
//                continue;
            ProgTrace(TRACE5, "CHECK 2");
            xmlNodePtr rowNode = NewTextChild(rowsNode, "item");
            tst();
            NewTextChild(rowNode, "user_id", user_id);
            tst();
            NewTextChild(rowNode, "descr", Qry.FieldAsString(col_descr));
            tst();
            NewTextChild(rowNode, "login", Qry.FieldAsString(col_login));
            tst();
            NewTextChild(rowNode, "type", Qry.FieldAsString(col_type));
            tst();
            NewTextChild(rowNode, "pr_denial", Qry.FieldAsString(col_pr_denial));
            tst();
            NewTextChild(rowNode, "time_fmt_code", Qry.FieldAsString(col_time_fmt_code));
            tst();
            NewTextChild(rowNode, "disp_airline_fmt_code", Qry.FieldAsString(col_disp_airline_fmt_code));
            tst();
            NewTextChild(rowNode, "disp_airp_fmt_code", Qry.FieldAsString(col_disp_airp_fmt_code));
            tst();
            NewTextChild(rowNode, "disp_craft_fmt_code", Qry.FieldAsString(col_disp_craft_fmt_code));
            tst();
            NewTextChild(rowNode, "disp_suffix_fmt_code", Qry.FieldAsString(col_disp_suffix_fmt_code));
            tst();
            ProgTrace(TRACE5, "CHECK 3");
//            airps.to_xml(rowNode);
//            ProgTrace(TRACE5, "CHECK 4");
//            airlines.to_xml(rowNode);
//            ProgTrace(TRACE5, "CHECK 5");
//            roles.to_xml(rowNode);
//            ProgTrace(TRACE5, "CHECK 6");
        }
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void AccessInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
