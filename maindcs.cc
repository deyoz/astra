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
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
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
        Qry->DeclareVariable("user_id", otInteger);
        Qry->SetVariable("user_id", reqinfo->user.user_id);
        Qry->Execute();
        xmlNodePtr modulesNode = NewTextChild(resNode, "modules");
        int screen_id = -1;
        bool modules_exists = false;
        while(!Qry->Eof) {
            if(screen_id != Qry->FieldAsInteger("id")) {
                if(Qry->FieldAsInteger("access_code") > 0) {
                    modules_exists = true;
                    xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
                    NewTextChild(moduleNode, "id", Qry->FieldAsInteger("id"));
                    NewTextChild(moduleNode, "name", Qry->FieldAsString("name"));
                    NewTextChild(moduleNode, "exe", Qry->FieldAsString("exe"));
                }
                screen_id = Qry->FieldAsInteger("id");
            }
            Qry->Next();
        }
        if(!modules_exists) throw UserException("���짮��⥫� ������ ����� �� �ᥬ �����");
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}

void MainDCSInterface::CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    if(reqinfo->user.user_id<0)
        return;
    GetModuleList(resNode);
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText = "update astra.users2 set desk = null where user_id = :user_id";
        Qry->DeclareVariable("user_id", otInteger);
        ProgTrace(TRACE5, "%d", TReqInfo::Instance()->user.user_id);
        Qry->SetVariable("user_id", TReqInfo::Instance()->user.user_id);
        Qry->Execute();
        showMessage(resNode, "����� ࠡ��� � ��⥬� �����襭");
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
}

void MainDCSInterface::UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    TQuery *Qry = OraSession.CreateQuery();
    try {
        Qry->SQLText =
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
        Qry->DeclareVariable("userr",otString);
        Qry->DeclareVariable("passwd",otString);
        Qry->DeclareVariable("user_id",otInteger);
        Qry->DeclareVariable("pr_denial",otInteger);
        Qry->DeclareVariable("desk",otString);
        Qry->DeclareVariable("new_desk",otString);
        Qry->SetVariable("userr", NodeAsString("userr", reqNode));
        Qry->SetVariable("passwd", NodeAsString("passwd", reqNode));
        Qry->SetVariable("new_desk", reqinfo->desk.code);
        try {
            Qry->Execute();
        } catch(EOracleError E) {
            tst();
            switch( E.Code ) {
              case 20000: throw UserException("����୮ 㪠��� ���짮��⥫� ��� ��஫�");
              default: throw;
            }
        }
        if(Qry->GetVariableAsInteger("pr_denial") != 0)
            throw UserException("���짮��⥫� �⪫�祭");
        if(!Qry->VariableIsNULL("desk") && (reqinfo->desk.code != Qry->GetVariableAsString("desk")))
            showMessage(resNode, "������ �ନ����");
        reqinfo->user.user_id = Qry->GetVariableAsInteger("user_id");
        GetModuleList(resNode);
        showMessage(resNode, "���� ���������� � ��⥬�");
    } catch(...) {
        OraSession.DeleteQuery(*Qry);
        throw;
    }
    OraSession.DeleteQuery(*Qry);
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
    TReqInfo::Instance()->MsgToLog("������� ��஫� ���짮��⥫�", evtAccess);
    showMessage(resNode, "��஫� �������");
}

void MainDCSInterface::SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );
  TQuery *Qry = OraSession.CreateQuery();
  int user_id = NodeAsInteger( "user_id", reqNode );
  ProgTrace( TRACE5, "���� ��஫� ���짮��⥫�(user_id=%d)", user_id );
  try {
    Qry->SQLText = "UPDATE users2 SET passwd='������' WHERE user_id=:user_id";
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", user_id ); 
    Qry->Execute();
    if ( Qry->RowsProcessed() == 0 ) 
      throw Exception( "���������� ����� ��஫�" );
    SetProp( resNode, "handle", "1" );
    Qry->Clear();
    Qry->SQLText = "SELECT descr FROM users2 WHERE user_id=:user_id";
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", user_id ); 
    Qry->Execute();    
    TReqInfo::Instance()->MsgToLog( string( "���襭 ��஫� ���짮��⥫� " ) + 
                                    Qry->FieldAsString( "descr" ), evtAccess );          
    showMessage( resNode, string( "���짮��⥫� " ) + Qry->FieldAsString( "descr" ) +
                          " �����祭 ��஫� �� 㬮�砭�� '������'" );
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    throw;
  }
  OraSession.DeleteQuery( *Qry );
}
