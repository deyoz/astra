#include "access.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_utils.h"
#define NICKNAME "DENIS"
#include "serverlib/test.h"

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
        throw UserException("����୮ ������ ����� ���짮��⥫�");
    int user_id = Qry.FieldAsInteger("user_id");
    Qry.Clear();
    Qry.SQLText = "select adm.check_role_aro_access(:role_id, :user_id) from dual";
    Qry.CreateVariable("role_id", otInteger, role_id);
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.Execute();
    if(Qry.FieldAsInteger(0) == 0)
        NewTextChild(resNode, "aro", "���짮��⥫� ����饭 ����� � �/� ��� �/� ஫�");
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

void AccessInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
