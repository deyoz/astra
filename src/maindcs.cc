#include "maindcs.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include <string>
#include "xml_unit.h"
#include "jxt_cont.h"

using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace std;
using namespace JxtContext;

void GetModuleList(xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT DISTINCT screen.id,screen.name,screen.exe,screen.view_order "
      "FROM user_roles,role_rights,screen_rights,screen "
      "WHERE user_roles.role_id=role_rights.role_id AND "
      "      role_rights.right_id=screen_rights.right_id AND "
      "      screen_rights.screen_id=screen.id AND "
      "      user_roles.user_id=:user_id AND view_order IS NOT NULL "
      "ORDER BY view_order";
    Qry.DeclareVariable("user_id", otInteger);
    Qry.SetVariable("user_id", reqinfo->user.user_id);
    Qry.Execute();
    xmlNodePtr modulesNode = NewTextChild(resNode, "modules");
    if (!Qry.Eof)
    {
      for(;!Qry.Eof;Qry.Next())
      {
        xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
        NewTextChild(moduleNode, "id", Qry.FieldAsInteger("id"));
        NewTextChild(moduleNode, "name", Qry.FieldAsString("name"));
        NewTextChild(moduleNode, "exe", Qry.FieldAsString("exe"));
      };
    }
    else showErrorMessage("���짮��⥫� ������ ����� �� �ᥬ �����");
}

bool MainDCSInterface::GetSessionAirlines(xmlNodePtr node, string &str)
{
  str.clear();
  if (node==NULL) return true;
  vector<string> airlines;
  for(node=node->children;node!=NULL;node=node->next)
  {
    try
    {
      airlines.push_back(base_tables.get("airlines").get_row("code/code_lat",NodeAsString(node)).AsString("code"));
    }
    catch(EBaseTableError)
    {
      str=NodeAsString(node);
      return false;
    };
  };
  if (airlines.empty()) return true;
  sort(airlines.begin(),airlines.end());
  string prior_airline;
  //㤠��� ��������� ��������
  for(vector<string>::iterator i=airlines.begin();i!=airlines.end();i++)
  {
    if (*i!=prior_airline)
    {
      if (!str.empty()) str.append("/");
      str.append(*i);
      prior_airline=*i;
    };
  };
  return true;
};

void MainDCSInterface::CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    try
    {
      if(reqinfo->user.login.empty()) throw 0;

      string airlines;
      if (!GetSessionAirlines(GetNode("airlines", reqNode),airlines)) throw 0;

      // �஢�ਬ session_airlines
      if (getJxtContHandler()->sysContext()->read("session_airlines")!=airlines)
        throw 0;

      GetModuleList(resNode);
    }
    catch(int)
    {
      reqinfo->user.clear();
      reqinfo->desk.clear();
      showBasicInfo();
    };
}

void MainDCSInterface::UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "SELECT user_id, login, passwd, descr, pr_denial, desk "
      "FROM users2 "
      "WHERE login= UPPER(:userr) AND passwd= UPPER(:passwd) FOR UPDATE ";
    Qry.CreateVariable("userr", otString, NodeAsString("userr", reqNode));
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw UserException("����୮ 㪠��� ���짮��⥫� ��� ��஫�");
    if ( Qry.FieldAsInteger( "pr_denial" ) == -1 )
    	throw UserException( "���짮��⥫� 㤠��� �� ��⥬�" );
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "���짮��⥫� �⪠���� � ����㯥" );
    reqInfo->user.user_id = Qry.FieldAsInteger("user_id");
    reqInfo->user.login = Qry.FieldAsString("login");
    reqInfo->user.descr = Qry.FieldAsString("descr");
    if(Qry.FieldIsNULL("desk"))
    {
      showMessage( reqInfo->user.descr + ", ���� ���������� � ��⥬�");
    }
    else
     if (reqInfo->desk.code != Qry.FieldAsString("desk"))
       showMessage("������ �ନ����");
    if (Qry.FieldAsString("passwd")==(string)"������" )
      showErrorMessage("���짮��⥫� ����室��� �������� ��஫�");
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  UPDATE users2 SET desk = NULL WHERE desk = :desk; "
      "  UPDATE users2 SET desk = :desk WHERE user_id = :user_id; "
      "END;";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();

    string airlines;
    if (!GetSessionAirlines(GetNode("airlines", reqNode),airlines))
      throw UserException("�� ������ ��� ������������ %s",airlines.c_str());
    getJxtContHandler()->sysContext()->write("session_airlines",airlines);

    GetModuleList(resNode);

    xmlNodePtr node=NodeAsNode("/term/query",ctxt->reqDoc);
    std::string screen = NodeAsString("@screen", node);
    std::string opr = NodeAsString("@opr", node);
    xmlNodePtr modeNode = GetNode("@mode", node);
    std::string mode;
    if (modeNode!=NULL)
      mode = NodeAsString(modeNode);
    reqInfo->Initialize( screen, ctxt->pult, opr, mode, false );
    showBasicInfo();
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.SQLText = "UPDATE users2 SET desk = NULL WHERE user_id = :user_id";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    getJxtContHandler()->sysContext()->remove("session_airlines");
    showMessage("����� ࠡ��� � ��⥬� �����襭");
    reqInfo->user.clear();
    reqInfo->desk.clear();
    showBasicInfo();
}

void MainDCSInterface::ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "UPDATE users2 SET passwd = :passwd WHERE user_id = :user_id";
    Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw Exception("user not found (user_id=%d)",TReqInfo::Instance()->user.user_id);
    TReqInfo::Instance()->MsgToLog("������� ��஫� ���짮��⥫�", evtAccess);
    showMessage("��஫� �������");
}

