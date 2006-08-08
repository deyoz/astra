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
    if(!modules_exists) showErrorMessage("Пользователю закрыт доступ ко всем модулям");
    
}

void MainDCSInterface::CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    if(reqinfo->user.login.empty())
        return;    
    GetModuleList(resNode);    
}

void MainDCSInterface::UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);                    
    Qry.SQLText = 
      "SELECT user_id, login, descr, pr_denial, desk FROM users2 "
      "WHERE login= UPPER(:userr) AND passwd= UPPER(:passwd) FOR UPDATE ";
    Qry.CreateVariable("userr", otString, NodeAsString("userr", reqNode));
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));      
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw UserException("Неверно указан пользователь или пароль");
    
    	  
      
    
 /*   SELECT user_id, login, descr, pr_denial FROM astra.users2 "\
                  " WHERE desk = UPPER(:pult) ";
    Qry.DeclareVariable( "pult", otString );
    Qry.SetVariable( "pult", vpult );
    Qry.Execute();
    
    if ( Qry.RowCount() == 0 )
    {      
      if (checkBasicInfo)
       	return;
      else 	
        throw UserException( "Пользователь не вошел в систему. Используйте главный модуль." );
    };  
    if ( !vopr.empty() )
      if ( vopr != Qry.FieldAsString( "login" ) )
        throw UserException( "Пользователь не вошел в систему. Используйте главный модуль." );
        
      if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
        throw UserException( "Пользователю отказано в доступе" );
    user.user_id = Qry.FieldAsInteger( "user_id" );
    user.descr = Qry.FieldAsString( "descr" );    
    user.login = Qry.FieldAsString( "login" );    */
    
        "begin "
        "   begin "
        "       SELECT user_id,pr_denial,desk into :user_id, :pr_denial, :desk FROM astra.users2 "
        "       WHERE UPPER(login)= :userr AND UPPER(passwd)= :passwd for update; "
        "   exception "
        "       when no_data_found then "
        "           raise_application_error(-20000, 'wrong logon'); "
        "   end; "
        "   update astra.users2 set desk = :new_desk where user_id = :user_id; "
        "end; ";
    Qry.DeclareVariable("userr",otString);
    Qry.DeclareVariable("passwd",otString);
    Qry.DeclareVariable("user_id",otInteger);
    Qry.DeclareVariable("pr_denial",otInteger);
    Qry.DeclareVariable("desk",otString);
    Qry.DeclareVariable("new_desk",otString);
    Qry.SetVariable("userr", NodeAsString("userr", reqNode));
    Qry.SetVariable("passwd", NodeAsString("passwd", reqNode));
    Qry.SetVariable("new_desk", reqinfo->desk.code);
    try {
        Qry.Execute();
    } catch(EOracleError E) {
        tst();
        switch( E.Code ) {
          case 20000: throw UserException("Неверно указан пользователь или пароль");
          default: throw;
        }
    }
    if(Qry.GetVariableAsInteger("pr_denial") != 0)
        throw UserException("Пользователь отключен");
    if(!Qry.VariableIsNULL("desk") && (reqinfo->desk.code != Qry.GetVariableAsString("desk")))
        showMessage("Замена терминала");
    reqinfo->user.user_id = Qry.GetVariableAsInteger("user_id");
    GetModuleList(resNode);
    showMessage("Добро пожаловать в систему");    
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);            
    Qry.SQLText = "update astra.users2 set desk = null where user_id = :user_id";
    Qry.DeclareVariable("user_id", otInteger);
    ProgTrace(TRACE5, "%d", TReqInfo::Instance()->user.user_id);
    Qry.SetVariable("user_id", TReqInfo::Instance()->user.user_id);
    Qry.Execute();
    showMessage("Сеанс работы в системе завершен");    
}

void MainDCSInterface::ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "update users2 set passwd = :passwd where user_id = :user_id";
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
  TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry(&OraSession);  
  int user_id = NodeAsInteger( "user_id", reqNode );  
  Qry.SQLText = "UPDATE users2 SET passwd='ПАРОЛЬ' WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();
  if ( Qry.RowsProcessed() == 0 ) 
    throw Exception( "Невозможно сбросить пароль" );
  SetProp( resNode, "handle", "1" );
  Qry.Clear();
  Qry.SQLText = "SELECT descr FROM users2 WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id ); 
  Qry.Execute();    
  TReqInfo::Instance()->MsgToLog( string( "Сброшен пароль пользователя " ) + 
                                  Qry.FieldAsString( "descr" ), evtAccess );          
  showMessage( string( "Пользователю " ) + Qry.FieldAsString( "descr" ) +
                        " назначен пароль по умолчанию 'ПАРОЛЬ'" );  
}
