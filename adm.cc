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

void AdmInterface::LoadAdm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "AdmInterface::LoadAdm" );
/*    if sys.user.mode='' then
      ShowErrorMsg('Недостаточно прав. Доступ к информации невозможен');*/


  TQuery *Qry = OraSession.CreateQuery();
  try {
    Qry->SQLText = "SELECT cache,adm_cache_tables.title,depth,num,0 AS access_code, "\
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
                   "       SELECT role_cache_perms.cache,MAX(access_code) FROM user_roles,role_cache_perms "\
                   "       WHERE user_roles.role_id=role_cache_perms.role_id AND "\
                   "             user_roles.user_id=:user_id "\
                   "       GROUP BY role_cache_perms.cache) perms "\
                   "WHERE adm_cache_tables.cache=cache_tables.code AND "\
                   "      cache_tables.code=perms.cache(+) "\
                   " ORDER BY num,title,cache,access_code DESC ";
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", 1 ); //!!!
    Qry->Execute();

    SetProp(resNode, "handle", "1");
    xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
    SetProp(ifaceNode, "id", "adm");
    SetProp(ifaceNode, "ver", "1");
    xmlNodePtr node = NewTextChild( resNode, "properties" );
    node = NewTextChild( node, "PasswdBtn", "" );
    SetProp( node, "enabled", 1/*sys.user.mode=='w' !!!*/ );
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


void AdmInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

void AdmInterface::SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery *Qry = OraSession.CreateQuery();
  int user_id = NodeAsInteger( "user_id", reqNode );
  ProgTrace( TRACE5, "Сброс пароля пользователя(user_id=%d)", user_id );
  try {
    Qry->SQLText = "UPDATE users2 SET passwd='ПАРОЛЬ' WHERE user_id=:user_id";
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", user_id ); 
    Qry->Execute();
    if ( Qry->RowsProcessed() == 0 ) 
      throw Exception( "Невозможно сбросить пароль" );
    //!!!sys.MsgToLog( 'Сброшен пароль пользователя '+FieldAsString['descr',i], evtAccess );      
    SetProp( resNode, "handle", "1" );
/*    xmlNodePtr node = NewTextChild( resNode, "interface" );
    SetProp( node, "id", "adm" );
    SetProp( node, "ver", "1" );*/
    Qry->Clear();
    Qry->SQLText = "SELECT descr FROM users2 WHERE user_id=:user_id";
    Qry->DeclareVariable( "user_id", otInteger );
    Qry->SetVariable( "user_id", user_id ); 
    Qry->Execute();    
    NewTextChild( resNode, "message", string( "Пользователю " ) + Qry->FieldAsString( "descr" ) +
                           " назначен пароль по умолчанию 'ПАРОЛЬ' " );
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    throw;
  }
  OraSession.DeleteQuery( *Qry );
}

void AdmInterface::CopyBasicPayTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery *Qry = OraSession.CreateQuery();
  string name = NodeAsString( "name", reqNode );
  string airline = NodeAsString( "airline", reqNode );  
  ProgTrace( TRACE5, "CopyBasicPayTable, name=%s, airline=%s", name.c_str(), airline.c_str() );  
  try {
    string sqltext = "BEGIN";
    if ( name == "AIRLINE_BAG_NORMS" ) 
      sqltext += "  kassa.copy_basic_bag_norm(:airline);";
    if ( name == "AIRLINE_BAG_RATES" )
      sqltext += "  kassa.copy_basic_bag_rate(:airline);";
    if ( name == "AIRLINE_VALUE_BAG_TAXES" )
      sqltext += "  kassa.copy_basic_value_bag_tax(:airline);";
    if ( name == "AIRLINE_EXCHANGE_RATES" )
      sqltext += "  kassa.copy_basic_exchange_rate(:airline);";
    sqltext += "END;";
    Qry->DeclareVariable( "airline", otString );
    Qry->SetVariable( "airline", airline );
    Qry->Execute();    
/*!!!      if Name='AIRLINE_BAG_NORMS' then
        sys.MsgToLog('Багажные нормы авиакомпании '+airline+' установлены на основе базовых норм',evtPeriod);
      if Name='AIRLINE_BAG_RATES' then
        sys.MsgToLog('Багажные тарифы авиакомпании '+airline+' установлены на основе базовых тарифов',evtPeriod);
      if Name='AIRLINE_VALUE_BAG_TAXES' then
        sys.MsgToLog('Сборы за ценнось багажа для авиакомпании '+airline+' установлены на основе базовых сборов',evtPeriod);
      if Name='AIRLINE_EXCHANGE_RATES' then
        sys.MsgToLog('Курсы перевода валют авиакомпании '+airline+' установлены на основе базовых курсов',evtPeriod);*/
    string msg;
    if ( name == "AIRLINE_BAG_NORMS" )
      msg = "Багажные нормы авиакомпании установлены на основе базовых норм";
    if ( name == "AIRLINE_BAG_RATES" )
      msg = "Багажные тарифы авиакомпании установлены на основе базовых тарифов";
    if ( name == "AIRLINE_VALUE_BAG_TAXES" )
      msg = "Сборы за ценнось багажа для авиакомпании установлены на основе базовых сборов";
    if ( name == "AIRLINE_EXCHANGE_RATES" )
      msg = "Курсы перевода валют авиакомпании установлены на основе базовых курсов";
    NewTextChild( resNode, "message", msg );     
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    throw;
  }
  OraSession.DeleteQuery( *Qry );  
}

