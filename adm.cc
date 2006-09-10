#include "adm.h"
#include "basic.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

void AdmInterface::LoadAdm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{  
  TReqInfo *ri = TReqInfo::Instance();  
  ri->user.check_access( amRead );

  TQuery *Qry = OraSession.CreateQuery();
  try {
    string sql =
     "SELECT cache,adm_cache_tables.title,depth,num,0 AS access_code, "\
     "       0 AS pr_ins,0 AS pr_upd,0 AS pr_del "\
     "FROM adm_cache_tables WHERE cache IS NULL "\
     "UNION "\
     "SELECT adm_cache_tables.cache,NVL(adm_cache_tables.title,cache_tables.title), "\
     "       depth,num,NVL(access_code,0),"\
     "       DECODE(insert_sql,NULL,0,1) AS pr_ins, "\
     "       DECODE(update_sql,NULL,0,1) AS pr_upd, "\
     "       DECODE(delete_sql,NULL,0,1) AS pr_del "\
     "FROM adm_cache_tables,cache_tables, "\
     "      (SELECT user_cache_perms.cache,access_code FROM user_cache_perms "\
     "       WHERE user_id=:user_id "\
     "       UNION "\
     "       SELECT role_cache_perms.cache,MAX(access_code) FROM ";
     sql += COMMON_ORAUSER();
     sql += ".user_roles,";
     sql += COMMON_ORAUSER();
     sql += ".role_cache_perms ";
     sql +=
     "       WHERE user_roles.role_id=role_cache_perms.role_id AND "\
     "             user_roles.user_id=:user_id "\
     "       GROUP BY role_cache_perms.cache) perms "\
     "WHERE adm_cache_tables.cache=cache_tables.code AND "\
     "      cache_tables.code=perms.cache(+) "\
     " ORDER BY num,title,cache,access_code DESC ";
    Qry->SQLText = sql;
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", ri->user.user_id );
    Qry->Execute();

    SetProp(resNode, "handle", "1");
    xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
    SetProp(ifaceNode, "id", "adm");
    SetProp(ifaceNode, "ver", "1");
    xmlNodePtr node = NewTextChild( resNode, "properties" );
    node = NewTextChild( node, "PasswdBtn", "" );
    SetProp( node, "enabled", ri->user.getAccessMode( amWrite ) );
    node = NewTextChild( resNode, "data" );
    if ( Qry->RowCount() ) {
      node = NewTextChild( node, "CacheTables" );
      int i = 0;
      int depth;
      string priorCode;
      xmlNodePtr rowNode;
      while ( !Qry->Eof ) {
        if ( Qry->FieldIsNULL( "cache" ) || i == 0 ||
             priorCode != Qry->FieldAsString( "cache" ) ) {
          rowNode = NewTextChild( node, "CacheTable" );
          SetProp( rowNode, "index", i );
          NewTextChild( rowNode, "cache", Qry->FieldAsString( "cache" ) );
          NewTextChild( rowNode, "title", Qry->FieldAsString( "title" ) );
          depth = Qry->FieldAsInteger( "depth" );
          if (depth <= 0 )
           depth = 1;
          NewTextChild( rowNode, "depth", depth );
          NewTextChild( rowNode, "access_code", Qry->FieldAsInteger( "access_code" ) );
          NewTextChild( rowNode, "pr_ins", Qry->FieldAsInteger( "pr_ins" ) );
          NewTextChild( rowNode, "pr_upd", Qry->FieldAsInteger( "pr_upd" ) );
          NewTextChild( rowNode, "pr_del", Qry->FieldAsInteger( "pr_del" ) );
          priorCode = Qry->FieldAsString( "cache" );
          i++;
        }
        Qry->Next();
      }
    }
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );
};






