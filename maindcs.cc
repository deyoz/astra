#include "maindcs.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include <string>
#include "xml_unit.h"

using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace std;

void GetModuleList(xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "SELECT 1 AS priority,MAX(access_code) AS access_code, "
        "       screen.id,screen.name,screen.exe "
        "FROM user_perms,screen "
        "WHERE user_perms.screen_id=screen.id AND "
        "      user_perms.user_id=:user_id "
        "GROUP BY screen.id,screen.name,screen.exe "
        "UNION "
        "SELECT 2,MAX(access_code), "
        "       screen.id,screen.name,screen.exe "
        "FROM user_roles,role_perms,screen "
        "WHERE user_roles.role_id=role_perms.role_id AND "
        "      role_perms.screen_id=screen.id AND "
        "      user_roles.user_id=:user_id "
        "GROUP BY screen.id,screen.name,screen.exe "
        "ORDER BY id,priority ";
    Qry.DeclareVariable("user_id", otInteger);
    Qry.SetVariable("user_id", reqinfo->user.user_id);
    Qry.Execute();
    xmlNodePtr modulesNode = NewTextChild(resNode, "modules");
    int screen_id = -1;
    bool modules_exists = false;
    while(!Qry.Eof) {
        if(screen_id != Qry.FieldAsInteger("id")) {
            if(Qry.FieldAsInteger("access_code") > 0) {
                modules_exists = true;
                xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
                NewTextChild(moduleNode, "id", Qry.FieldAsInteger("id"));
                NewTextChild(moduleNode, "name", Qry.FieldAsString("name"));
                NewTextChild(moduleNode, "exe", Qry.FieldAsString("exe"));
            }
            screen_id = Qry.FieldAsInteger("id");
        }
        Qry.Next();
    }
    if(!modules_exists) showErrorMessage("���짮��⥫� ������ ����� �� �ᥬ �����");        
}

void MainDCSInterface::CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    if(reqinfo->user.login.empty())
    {
    	reqinfo->desk.clear();
    	showBasicInfo();  
        return;    
    };    
    GetModuleList(resNode);    
    
}

void MainDCSInterface::UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);                    
    Qry.Clear();
    Qry.SQLText = 
      "SELECT user_id, login, passwd, descr, pr_denial, desk FROM users2 "
      "WHERE login= UPPER(:userr) AND passwd= UPPER(:passwd) FOR UPDATE ";
    Qry.CreateVariable("userr", otString, NodeAsString("userr", reqNode));
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));      
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw UserException("����୮ 㪠��� ���짮��⥫� ��� ��஫�");
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
    Qry.SQLText = "BEGIN "
                  "  UPDATE users2 SET desk = NULL WHERE desk = :desk; " 
                  "  UPDATE users2 SET desk = :desk WHERE user_id = :user_id; "
                  "END;";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();        
    GetModuleList(resNode);        
    showBasicInfo();  
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();	
    TQuery Qry(&OraSession);            
    Qry.SQLText = "UPDATE users2 SET desk = NULL WHERE user_id = :user_id";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    showMessage("����� ࠡ��� � ��⥬� �����襭");
    reqInfo->user.clear();
    reqInfo->desk.clear();
    showBasicInfo();  
}

void MainDCSInterface::ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "UPDATE users2 SET passwd = :passwd WHERE user_id = :user_id";
    Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw Exception("user not found (user_id=%d)",TReqInfo::Instance()->user.user_id);
    TReqInfo::Instance()->MsgToLog("������� ��஫� ���짮��⥫�", evtAccess);
    showMessage("��஫� �������");
}

void MainDCSInterface::SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo=TReqInfo::Instance();
  reqInfo->user.check_access( amWrite );
  TQuery Qry(&OraSession);  
  int user_id = NodeAsInteger( "user_id", reqNode );  
  Qry.SQLText = "UPDATE users2 SET passwd='������' WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();
  if ( Qry.RowsProcessed() == 0 ) 
    throw Exception( "���������� ����� ��஫�" );
  SetProp( resNode, "handle", "1" );
  Qry.Clear();
  Qry.SQLText = "SELECT descr FROM users2 WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();    
  reqInfo->MsgToLog( string( "���襭 ��஫� ���짮��⥫� " ) + 
                                  Qry.FieldAsString( "descr" ), evtAccess );          
  showMessage( string( "���짮��⥫� " ) + Qry.FieldAsString( "descr" ) +
                        " �����祭 ��஫� �� 㬮�砭�� '������'" );  
}
