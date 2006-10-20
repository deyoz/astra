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
    Qry.Clear();
    Qry.SQLText=
      "SELECT DISTINCT screen.id,screen.name,screen.exe,screen.view_order "
      "FROM user_roles,role_rights,screen_rights,screen "
      "WHERE user_roles.role_id=role_rights.role_id AND "
      "      role_rights.right_id=screen_rights.right_id AND "
      "      screen_rights.screen_id=screen.id AND "
      "      user_roles.user_id=:user_id "
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
    else showErrorMessage("Пользователю закрыт доступ ко всем модулям");
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
  //reqInfo->user.check_access( amWrite );
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
