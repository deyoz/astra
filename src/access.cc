#include "access.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_utils.h"
#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "cache.h"
#include <set>
#include "misc.h"

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
        } catch(EOracleError &E) {
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
    } catch(EOracleError &E) {
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
}

class TARO {
    protected:
        TQuery Qry, usersQry;
        string usersSQLText, user_cond;
        string node_name;
        map<int, set<string> > items;
        string replace_user_cond(const string sql, const string cond);
        virtual string get_item(TQuery &Qry);
    public:
        set<string> &get(int user_id);
        void to_xml(int user_id, xmlNodePtr node);
        void get_users(set<string> &aro_params, vector<int> &users, bool &pr_find);
        TARO();
        virtual ~TARO(){};
};

string TARO::replace_user_cond(const string sql, const string cond)
{
    string result = sql;
    static const string user_cond = "user_cond";
    size_t idx = result.find(user_cond);
    result.replace(idx, user_cond.size(), cond);
    return result;
}

TARO::TARO():
    Qry(&OraSession),
    usersQry(&OraSession)
{
    Qry.DeclareVariable("user_id", otInteger);
};

class TRolesARO:public TARO {
    private:
        string get_item(TQuery &Qry)
        {
            int id = Qry.FieldAsInteger(0);
            ostringstream result;
            TQuery dummyQry(&OraSession);
            result << id << ";" << get_role_name(id, dummyQry);
            return result.str();

        }

    public:
        TRolesARO()
        {
            Qry.SQLText =
                "select "
                "   roles.role_id "
                "from "
                "   user_roles, "
                "   roles "
                "where "
                "   user_roles.user_id = :user_id and "
                "   user_roles.role_id = roles.role_id";
            usersSQLText =
                "select user_id from user_roles where user_id in "
                "   (select user_id from user_roles where role_id = :aro user_cond) "
                "group by user_id "
                "having count(*) = :count";
            user_cond = " and user_id = :user_id";
            node_name = "roles";
        }
};

class TAirpsARO:public TARO {
    public:
        TAirpsARO()
        {
            Qry.SQLText = "select airp from aro_airps where aro_id = :user_id";
            usersSQLText =
                "select aro_id from aro_airps where aro_id in "
                "   (select aro_id from aro_airps where airp = :aro user_cond) "
                "group by aro_id "
                "having count(*) = :count";
            user_cond = " and aro_id = :user_id";
            node_name = "airps";
        }
};

class TAirlinesARO:public TARO {
    public:
        TAirlinesARO()
        {
            Qry.SQLText = "select airline from aro_airlines where aro_id = :user_id";
            usersSQLText =
                "select aro_id from aro_airlines where aro_id in "
                "   (select aro_id from aro_airlines where airline = :aro user_cond) "
                "group by aro_id "
                "having count(*) = :count";
            user_cond = " and aro_id = :user_id";
            node_name = "airlines";
        }
};

template <class T>
bool real_equal(T &a, T &b)
{
    return a.size() == b.size() and equal(a.begin(), a.end(), b.begin());
}

string get_role_id(const string &role)
{
    string result;
    size_t idx = role.find(';');
    int i;
    if(idx != string::npos and BASIC::StrToInt(role.substr(0, idx).c_str(), i) != EOF)
            result = role.substr(0, idx);
    return result;
}

void TARO::get_users(set<string> &aro_params, vector<int> &users, bool &pr_find)
{
    if(not pr_find) return;
    if(aro_params.empty()) return;
    usersQry.Clear();

    string role_id = get_role_id(*aro_params.begin());
    usersQry.CreateVariable("aro", otString, (role_id.empty() ? *aro_params.begin() : role_id));

    usersQry.CreateVariable("count", otInteger, (int)aro_params.size());
    if(users.empty()) {
        usersQry.SQLText = replace_user_cond(usersSQLText, "").c_str();
        usersQry.Execute();
        for(; not usersQry.Eof; usersQry.Next()) {
            int user_id = usersQry.FieldAsInteger(0);
            if(aro_params.size() == 1) {
                items[user_id].insert(*aro_params.begin());
                users.push_back(user_id);
            } else
                if(real_equal(aro_params, get(user_id)))
                    users.push_back(user_id);
        }
    } else {
        vector<int> tmp_users;
        usersQry.SQLText = replace_user_cond(usersSQLText, user_cond).c_str();
        usersQry.DeclareVariable("user_id", otInteger);
        for(vector<int>::iterator iv = users.begin(); iv != users.end(); iv++) {
            usersQry.SetVariable("user_id", *iv);
            usersQry.Execute();
            if(not usersQry.Eof)
                if(aro_params.size() == 1) {
                    items[*iv].insert(*aro_params.begin());
                    tmp_users.push_back(*iv);
                } else
                    if(real_equal(aro_params, get(*iv)))
                        tmp_users.push_back(*iv);
        }
        users = tmp_users;
    }
    pr_find = not users.empty();
}

void TARO::to_xml(int user_id, xmlNodePtr node)
{
    set<string> &s = get(user_id);
    if(not s.empty()) {
        node = NewTextChild(node, node_name.c_str());
        for(set<string>::iterator is = s.begin(); is != s.end(); is++) {
            NewTextChild(node, "item", *is);
        }
    }
}

string TARO::get_item(TQuery &Qry)
{
    return Qry.FieldAsString(0);
}
set<string> &TARO::get(int user_id)
{
    map<int, set<string> >::iterator mi = items.find(user_id);
    if(mi == items.end()) {
        Qry.SetVariable("user_id", user_id);
        Qry.Execute();
        set<string> &s = items[user_id];
        for(; not Qry.Eof; Qry.Next())
            s.insert(get_item(Qry));
        mi = items.find(user_id);
    }
    return mi->second;
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
    set<string> airps, airlines, roles;
    int pr_denial;
    void initXML(xmlNodePtr node);
    void search(xmlNodePtr node);
    void insert();
    void update();
    void update_aro();
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

class TSearchResultXML {
    private:
        int col_user_id;
        int col_login;
        int col_descr;
        int col_type;
        int col_pr_denial;
        int col_time_fmt_code;
        int col_disp_airline_fmt_code;
        int col_disp_airp_fmt_code;
        int col_disp_craft_fmt_code;
        int col_disp_suffix_fmt_code;
    public:
        void build(
                TRolesARO &user_roles,
                TAirpsARO &user_airps,
                TAirlinesARO &user_airlines,
                TQuery &Qry,
                xmlNodePtr resNode,
                xmlNodePtr &rowsNode,
                int user_id);
        TSearchResultXML():
            col_user_id(NoExists),
            col_login(NoExists),
            col_descr(NoExists),
            col_type(NoExists),
            col_pr_denial(NoExists),
            col_time_fmt_code(NoExists),
            col_disp_airline_fmt_code(NoExists),
            col_disp_airp_fmt_code(NoExists),
            col_disp_craft_fmt_code(NoExists),
            col_disp_suffix_fmt_code(NoExists)
    {}
};

void TSearchResultXML::build(
        TRolesARO &user_roles,
        TAirpsARO &user_airps,
        TAirlinesARO &user_airlines,
        TQuery &Qry,
        xmlNodePtr resNode,
        xmlNodePtr &rowsNode,
        int user_id)
{
    Qry.Execute();
    if(!Qry.Eof) {
        if(col_user_id == NoExists) {
            if(user_id < 0)
                rowsNode = NewTextChild(resNode, "users");
            col_user_id = Qry.FieldIndex("user_id");
            col_login = Qry.FieldIndex("login");
            col_descr = Qry.FieldIndex("descr");
            col_type = Qry.FieldIndex("type");
            col_pr_denial = Qry.FieldIndex("pr_denial");
            col_time_fmt_code = Qry.FieldIndex("time_fmt_code");
            col_disp_airline_fmt_code = Qry.FieldIndex("disp_airline_fmt_code");
            col_disp_airp_fmt_code = Qry.FieldIndex("disp_airp_fmt_code");
            col_disp_craft_fmt_code = Qry.FieldIndex("disp_craft_fmt_code");
            col_disp_suffix_fmt_code = Qry.FieldIndex("disp_suffix_fmt_code");
        }
        for(; !Qry.Eof; Qry.Next()) {
            int new_user_id = Qry.FieldAsInteger(col_user_id);
            xmlNodePtr rowNode;
            if(user_id < 0)
                rowNode = NewTextChild(rowsNode, "item");
            else
                rowNode = resNode;
            NewTextChild(rowNode, "user_id", new_user_id);
            NewTextChild(rowNode, "descr", Qry.FieldAsString(col_descr));
            NewTextChild(rowNode, "login", Qry.FieldAsString(col_login));
            NewTextChild(rowNode, "type", Qry.FieldAsInteger(col_type));
            NewTextChild(rowNode, "pr_denial", Qry.FieldAsInteger(col_pr_denial));
            NewTextChild(rowNode, "time_fmt_code", Qry.FieldAsInteger(col_time_fmt_code));
            NewTextChild(rowNode, "disp_airline_fmt_code", Qry.FieldAsInteger(col_disp_airline_fmt_code));
            NewTextChild(rowNode, "disp_airp_fmt_code", Qry.FieldAsInteger(col_disp_airp_fmt_code));
            NewTextChild(rowNode, "disp_craft_fmt_code", Qry.FieldAsInteger(col_disp_craft_fmt_code));
            NewTextChild(rowNode, "disp_suffix_fmt_code", Qry.FieldAsInteger(col_disp_suffix_fmt_code));
            user_airps.to_xml(new_user_id, rowNode);
            user_airlines.to_xml(new_user_id, rowNode);
            user_roles.to_xml(new_user_id, rowNode);
        }
    }
}


void airps_to_xml(xmlNodePtr node, vector<string> &items) {
    if(not items.empty()) {
        node = NewTextChild(node, "airps");
        for(vector<string>::iterator iv = items.begin(); iv != items.end(); iv++) {
            NewTextChild(node, "item", *iv);
        }
    }
}

void airlines_to_xml(xmlNodePtr node, vector<string> &items) {
    if(not items.empty()) {
        node = NewTextChild(node, "airlines");
        for(vector<string>::iterator iv = items.begin(); iv != items.end(); iv++) {
            NewTextChild(node, "item", *iv);
        }
    }
}

void roles_to_xml(xmlNodePtr node, set<int> items) {
    if(not items.empty()) {
        node = NewTextChild(node, "roles");
        for(set<int>::iterator iv = items.begin(); iv != items.end(); iv++) {
            NewTextChild(node, "item", *iv);
        }
    }
}

bool roles_cmp(vector<int> &a, vector<int> &b) {
    bool Result;
    if(b.empty())
        Result = true;
    else {
        if(a.size() != b.size())
            Result = false;
        else {
            Result = true;
            for(vector<int>::iterator iv = a.begin(); iv != a.end(); iv++) {
                vector<int>::iterator iv1 = b.begin();
                for(; iv1 != b.end(); iv1++) {
                    if(*iv == *iv1)
                        break;
                }
                if(iv1 == b.end()) {
                    Result = false;
                    break;
                }
            }
        }
    }
    return Result;
}

bool aro_cmp(vector<string> &a, vector<string> &b) {
    bool Result;
    if(b.empty())
        Result = true;
    else {
        if(a.size() != b.size())
            Result = false;
        else {
            Result = true;
            for(vector<string>::iterator iv = a.begin(); iv != a.end(); iv++) {
                vector<string>::iterator iv1 = b.begin();
                for(; iv1 != b.end(); iv1++) {
                    if(*iv == *iv1)
                        break;
                }
                if(iv1 == b.end()) {
                    Result = false;
                    break;
                }
            }
        }
    }
    return Result;
}

void TUserData::search(xmlNodePtr resNode)
{
    TAirpsARO user_airps;
    TAirlinesARO user_airlines;
    TRolesARO user_roles;
    vector<int> users;
    if(user_id < 0) {
        bool pr_find = true; // искать дальше или нет
        user_roles.get_users(roles, users, pr_find);
        user_airps.get_users(airps, users, pr_find);
        user_airlines.get_users(airlines, users, pr_find);
        if(not pr_find) return;
    } else
        users.push_back(user_id);
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "       users2.user_id, "
        "       login, "
        "       descr, "
        "       type, "
        "       pr_denial, "
        "       time_fmt.code AS time_fmt_code, "
        "       disp_airline_fmt.code AS disp_airline_fmt_code, "
        "       disp_airp_fmt.code AS disp_airp_fmt_code, "
        "       disp_craft_fmt.code AS disp_craft_fmt_code, "
        "       disp_suffix_fmt.code AS disp_suffix_fmt_code "
        "FROM users2,user_sets, "
        "     user_set_types time_fmt, "
        "     user_set_types disp_airline_fmt, "
        "     user_set_types disp_airp_fmt, "
        "     user_set_types disp_craft_fmt, "
        "     user_set_types disp_suffix_fmt  "
        "WHERE "
        "      users2.user_id=user_sets.user_id(+) AND "
        "      DECODE(user_sets.time,        00,00,01,01,02,02,01)=time_fmt.code AND  "
        "      DECODE(user_sets.disp_airline,05,05,06,06,07,07,08,08,09,09,09)=disp_airline_fmt.code AND  "
        "      DECODE(user_sets.disp_airp,   05,05,06,06,07,07,08,08,09,09,09)=disp_airp_fmt.code AND "
        "      DECODE(user_sets.disp_craft,  05,05,06,06,07,07,08,08,09,09,09)=disp_craft_fmt.code AND "
        "      DECODE(user_sets.disp_suffix, 15,15,16,16,17,17,17)=disp_suffix_fmt.code and "
        "      adm.check_user_view_access(users2.user_id,:SYS_user_id)<>0 ";
    // связки с user_set_types необходимы для правильной работы поиска по параметрам *_fmt_code
    // чтобы юзеры с параметрами по умолчанию (т.е. для к-рых нет записей в user_set_types)
    // правильно доставались
    if(not users.empty()) {
        SQLText += "      and users2.user_id = :user_id ";
        Qry.DeclareVariable("user_id", otInteger);
    }

    if(user_id < 0) {
        if(not descr.empty()) {
            SQLText += " and users2.descr like :descr||'%' ";
            Qry.CreateVariable("descr", otString, descr);
        }
        if(not login.empty()) {
            SQLText += " and users2.login like :login||'%' ";
            Qry.CreateVariable("login", otString, login);
        }
        if(user_type >= 0) {
            SQLText += " and users2.type = :user_type ";
            Qry.CreateVariable("user_type", otInteger, user_type);
        }
        if(time_fmt >= 0) {
            SQLText += " and time_fmt.code = :time_fmt ";
            Qry.CreateVariable("time_fmt", otInteger, time_fmt);
        }
        if(airline_fmt >= 0) {
            SQLText += " and disp_airline_fmt.code = :airline_fmt ";
            Qry.CreateVariable("airline_fmt", otInteger, airline_fmt);
        }
        if(airp_fmt >= 0) {
            SQLText += " and disp_airp_fmt.code = :airp_fmt ";
            Qry.CreateVariable("airp_fmt", otInteger, airp_fmt);
        }
        if(craft_fmt >= 0) {
            SQLText += " and disp_craft_fmt.code = :craft_fmt ";
            Qry.CreateVariable("craft_fmt", otInteger, craft_fmt);
        }
        if(suff_fmt >= 0) {
            SQLText += " and disp_suffix_fmt.code = :suff_fmt ";
            Qry.CreateVariable("suff_fmt", otInteger, suff_fmt);
        }
        if(pr_denial >= 0) {
            SQLText += " and users2.pr_denial = :pr_denial ";
            Qry.CreateVariable("pr_denial", otInteger, pr_denial);
        }
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);

    TSearchResultXML srx;
    xmlNodePtr rowsNode = NULL;
    if(users.empty())
        srx.build(user_roles, user_airps, user_airlines, Qry, resNode, rowsNode, user_id);
    else
        for(vector<int>::iterator iv = users.begin(); iv != users.end(); iv++) {
            Qry.SetVariable("user_id", *iv);
            srx.build(user_roles, user_airps, user_airlines, Qry, resNode, rowsNode, user_id);
        }
}

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
    try {
        Qry.Execute();
    }
    catch(EOracleError &E)
    {
        if ( E.Code >= 20000 )
        {
            string str = E.what();
            throw AstraLocale::UserException(EOracleError2UserException(str));
        }
        else
            throw;
    };
}

void TUserData::update_aro()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  delete from aro_airlines where aro_id = :user_id; "
        "  delete from aro_airps where aro_id = :user_id; "
        "  delete from user_roles where user_id = :user_id; "
        "end;";
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText = "insert into aro_airlines(aro_id, airline) values(:user_id, :airline)";
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.DeclareVariable("airline", otString);
    for(set<string>::iterator iv = airlines.begin(); iv != airlines.end(); iv++) {
        Qry.SetVariable("airline", *iv);
        Qry.Execute();
    }
    Qry.Clear();
    Qry.SQLText = "insert into aro_airps(aro_id, airp) values(:user_id, :airp)";
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.DeclareVariable("airp", otString);
    for(set<string>::iterator iv = airps.begin(); iv != airps.end(); iv++) {
        Qry.SetVariable("airp", *iv);
        Qry.Execute();
    }
    Qry.Clear();
    Qry.SQLText = "insert into user_roles(user_id, role_id) values(:user_id, :role)";
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.DeclareVariable("role", otInteger);
    for(set<string>::iterator iv = roles.begin(); iv != roles.end(); iv++) {
        Qry.SetVariable("role", get_role_id(*iv));
        Qry.Execute();
    }
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
    try {
        Qry.Execute();
    }
    catch(EOracleError &E)
    {
        if ( E.Code >= 20000 )
        {
            string str = E.what();
            throw AstraLocale::UserException(EOracleError2UserException(str));
        }
        else
            throw;
    };
    update_aro();
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
    try {
        Qry.Execute();
    }
    catch(EOracleError &E)
    {
        if ( E.Code >= 20000 )
        {
            string str = E.what();
            throw AstraLocale::UserException(EOracleError2UserException(str));
        }
        else
            throw;
    };
    user_id = Qry.GetVariableAsInteger("user_id");
    update_aro();
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
                airps.insert(NodeAsString(node2));
        }
        node2 = GetNodeFast("airlines", node);
        if(node2 != NULL) {
            node2 = node2->children;
            for(; node2; node2 = node2->next)
                airlines.insert(NodeAsString(node2));
        }
        node2 = GetNodeFast("roles", node);
        if(node2 != NULL) {
            node2 = node2->children;
            for(; node2; node2 = node2->next)
                roles.insert(NodeAsString(node2));
        }
        pr_denial = NodeAsIntegerFast("pr_denial", node, -1);
    }
}

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
        try {
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
        } catch(EOracleError &E) {
            if(E.Code == 2291) // parent key not found
                throw UserException("MSG.CHANGED_FROM_OTHER_DESK.REFRESH_DATA");
            else
                throw;
        }
    }
    xmlNodePtr usersNode = NULL;
    for(map<int, TUserData>::iterator im = inserted.begin(); im != inserted.end(); im++) {
        if(usersNode == NULL)
            usersNode = NewTextChild(resNode, "users");
        xmlNodePtr itemNode = NewTextChild(usersNode, "item");
        SetProp(itemNode, "index", im->first);
        NewTextChild(itemNode, "user_id", im->second.user_id);
        im->second.search(itemNode);
    }
}

void AccessInterface::SaveUser(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TUserData user_data;
    user_data.initXML(reqNode->children);
    user_data.insert();
    user_data.search(resNode);
}

void AccessInterface::SearchUsers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TUserData user_data;
    user_data.initXML(reqNode->children);
    if(
            user_data.descr.empty() and
            user_data.login.empty() and
            user_data.roles.empty() and
            user_data.airps.empty() and
            user_data.airlines.empty()
      )
        throw UserException("MSG.ACCESS.MANDATORY_FIELDS_NOT_SET");
    user_data.search(resNode);
}

void AccessInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
