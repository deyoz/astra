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
    string sql =
        string( "SELECT 1 AS priority,MAX(access_code) AS access_code, " ) +
        "       screen.id,screen.name,screen.exe " +
        "FROM " + COMMON_ORAUSER() + ".user_perms," +
        COMMON_ORAUSER()+".screen "+
        "WHERE user_perms.screen_id=screen.id AND "+
        "      user_perms.user_id=:user_id "+
        "GROUP BY screen.id,screen.name,screen.exe "+
        "UNION "+
        "SELECT 2,MAX(access_code), "+
        "       screen.id,screen.name,screen.exe "+
        "FROM "+COMMON_ORAUSER()+".user_roles,"+
        COMMON_ORAUSER()+".role_perms,"+
        COMMON_ORAUSER()+".screen "+
        "WHERE user_roles.role_id=role_perms.role_id AND "+
        "      role_perms.screen_id=screen.id AND "+
        "      user_roles.user_id=:user_id "+
        "GROUP BY screen.id,screen.name,screen.exe "+
        "ORDER BY id,priority ";        
    Qry.SQLText = sql;
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
    if(!modules_exists) showErrorMessage("Пользователю закрыт доступ ко всем модулям");        
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
    string sql=
      string("SELECT user_id, login, passwd, descr, pr_denial, desk FROM ")+
      COMMON_ORAUSER()+".users2 "+
      "WHERE login= UPPER(:userr) AND passwd= UPPER(:passwd) FOR UPDATE ";
    Qry.SQLText = sql;     
    Qry.CreateVariable("userr", otString, NodeAsString("userr", reqNode));
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));      
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw UserException("Неверно указан пользователь или пароль");
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "Пользователю отказано в доступе" );
    reqInfo->user.user_id = Qry.FieldAsInteger("user_id");
    reqInfo->user.login = Qry.FieldAsString("login");
    reqInfo->user.descr = Qry.FieldAsString("descr");      
    if(Qry.FieldIsNULL("desk"))
    {      
      showMessage( reqInfo->user.descr + ", добро пожаловать в систему");         
    }  
    else    
     if (reqInfo->desk.code != Qry.FieldAsString("desk"))
       showMessage("Замена терминала");
    if (Qry.FieldAsString("passwd")==(string)"ПАРОЛЬ" )
      showErrorMessage("Пользователю необходимо изменить пароль");
    Qry.Clear();
    sql = string( "BEGIN " )+
                  "  UPDATE "+COMMON_ORAUSER()+".users2 SET desk = NULL WHERE desk = :desk; "+
                  "  UPDATE "+COMMON_ORAUSER()+".users2 SET desk = :desk WHERE user_id = :user_id; "+
                  "END;";
    Qry.SQLText = sql;                  
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
    string sql =
     string( "UPDATE " ) + COMMON_ORAUSER()+".users2 SET desk = NULL WHERE user_id = :user_id";
    Qry.SQLText = sql;
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    showMessage("Сеанс работы в системе завершен");
    reqInfo->user.clear();
    reqInfo->desk.clear();
    showBasicInfo();  
}

void MainDCSInterface::ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    string sql = string("UPDATE ") + COMMON_ORAUSER()+".users2 SET passwd = :passwd WHERE user_id = :user_id";
    Qry.SQLText = sql;
    Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw Exception("user not found (user_id=%d)",TReqInfo::Instance()->user.user_id);
    TReqInfo::Instance()->MsgToLog("Изменен пароль пользователя", evtAccess);
    showMessage("Пароль изменен");
}

void MainDCSInterface::SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo=TReqInfo::Instance();
  reqInfo->user.check_access( amWrite );
  TQuery Qry(&OraSession);  
  int user_id = NodeAsInteger( "user_id", reqNode );  
  string sql= "UPDATE " + COMMON_ORAUSER()+".users2 SET passwd='ПАРОЛЬ' WHERE user_id=:user_id";
  Qry.SQLText = sql;
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();
  if ( Qry.RowsProcessed() == 0 ) 
    throw Exception( "Невозможно сбросить пароль" );
  SetProp( resNode, "handle", "1" );
  Qry.Clear();
  sql = string( "SELECT descr FROM " ) + COMMON_ORAUSER() + ".users2 WHERE user_id=:user_id";
  Qry.SQLText = sql;
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();    
  reqInfo->MsgToLog( string( "Сброшен пароль пользователя " ) + 
                                  Qry.FieldAsString( "descr" ), evtAccess );          
  showMessage( string( "Пользователю " ) + Qry.FieldAsString( "descr" ) +
                        " назначен пароль по умолчанию 'ПАРОЛЬ'" );  
}
