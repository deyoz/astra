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
  //ri->user.check_access( amRead );
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=   
    "SELECT adm_cache_tables.cache,NVL(adm_cache_tables.title,cache_tables.title) AS title, " 
    "       depth,num "
    "FROM adm_cache_tables,cache_tables,user_roles,role_rights "
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
  Qry.DeclareVariable( "user_id", otInteger );
  Qry.SetVariable( "user_id", ri->user.user_id );
  Qry.Execute();  
  xmlNodePtr node = NewTextChild( resNode, "CacheTables" );
  xmlNodePtr rowNode;  
  for(;!Qry.Eof;Qry.Next())
  {
    rowNode = NewTextChild( node, "CacheTable" );    
    NewTextChild( rowNode, "cache", Qry.FieldAsString("cache") );    
    NewTextChild( rowNode, "title", Qry.FieldAsString("title") );        
    NewTextChild( rowNode, "depth", Qry.FieldAsInteger("depth") );    
  };      
  Qry.Close();        
};






