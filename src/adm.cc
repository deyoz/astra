#include "adm.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;
using namespace AstraLocale;

void AdmInterface::LoadAdm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *ri = TReqInfo::Instance();
  //ri->user.check_access( amRead );
  TQuery Qry(&OraSession);
  Qry.Clear();
  string sql=
    string("SELECT adm_cache_tables.cache,NVL(adm_cache_tables.title,cache_tables.title) AS title, ")+
    "       depth,num "
    "FROM adm_cache_tables,cache_tables,user_roles,role_rights "+
    "WHERE adm_cache_tables.cache=cache_tables.code AND "
    "      user_roles.role_id=role_rights.role_id AND "
    "      role_rights.right_id IN (select_right,insert_right,update_right,delete_right) AND "
    "      user_roles.user_id=:user_id "
    "UNION "
    "SELECT adm_cache_tables.cache,NVL(adm_cache_tables.title,cache_tables.title), "
    "       depth,num "
    "FROM adm_cache_tables,cache_tables "
    "WHERE adm_cache_tables.cache=cache_tables.code AND "
    "      cache_tables.select_right IS NULL "
    "UNION "
    "SELECT cache,title,depth,num FROM adm_cache_tables WHERE cache IS NULL "
    "ORDER BY num ";
  Qry.SQLText=sql;
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", ri->user.user_id );
  Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "CacheTables" );
  xmlNodePtr rowNode=NULL;
  for(;!Qry.Eof;Qry.Next())
  {
    rowNode = NewTextChild( node, "CacheTable" );
    NewTextChild( rowNode, "cache", Qry.FieldAsString("cache") );
    NewTextChild( rowNode, "title", AstraLocale::getLocaleText(Qry.FieldAsString("title")) );
    NewTextChild( rowNode, "depth", Qry.FieldAsInteger("depth") );
  };
  Qry.Close();

  int depth=-1;
  for(;rowNode!=NULL;)
  {
    if (!NodeIsNULL("cache",rowNode) ||
        depth>NodeAsInteger("depth",rowNode))
    {
      depth=NodeAsInteger("depth",rowNode);
      rowNode=rowNode->prev;
      continue;
    };
    //удалить заголовок
    xmlNodePtr node2=rowNode;
    rowNode=rowNode->prev;
    xmlUnlinkNode(node2);
    xmlFreeNode(node2);
  };
};

void AdmInterface::SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo=TReqInfo::Instance();
  //reqInfo->user.check_access( amWrite );
  TQuery Qry(&OraSession);
  int user_id = NodeAsInteger( "user_id", reqNode );
  Qry.SQLText =
    "UPDATE users2 SET passwd='ПАРОЛЬ' WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id );
  Qry.Execute();
  if ( Qry.RowsProcessed() == 0 )
    throw Exception("Невозможно сбросить пароль");
  SetProp( resNode, "handle", "1" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT descr FROM users2 WHERE user_id=:user_id";
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", user_id );
  Qry.Execute();
  reqInfo->MsgToLog( string( "Сброшен пароль пользователя " ) +
                                  Qry.FieldAsString( "descr" ), evtAccess );
  AstraLocale::showMessage("MSG.PASSWORD.ASSIGNED_DEFAULT",
  	                       LParams() << LParam("user", (string)Qry.FieldAsString( "descr" ))<<
  	                                    LParam("passwd",string("'ПАРОЛЬ'")) );
}





