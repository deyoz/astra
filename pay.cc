#include "pay.h"
#include <iostream>
#include <iomanip>
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

void PayInterface::LoadBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amRead );
  int grp_id = NodeAsInteger( "grp_id", reqNode );
  ProgTrace(TRACE2, "PayInterface::LoadBag, grp_id=%d", grp_id );
  TQuery *Qry = OraSession.CreateQuery();
  try {
    Qry->SQLText = "SELECT num,value,value_cur,NVL(tax_id,-1) AS tax_id,tax "\
                   "FROM value_bag "\
                   "WHERE grp_id=:grp_id "\
                   "ORDER BY num";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();
    
    SetProp( resNode, "handle", "1" );
    xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
    SetProp( ifaceNode, "id", "pay" );
    SetProp( ifaceNode, "ver", "1" );    
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr curNode;
    int valBagCount = 0;    
    if ( !Qry->Eof ) {
      curNode = NewTextChild( dataNode, "valBags" );
      while ( !Qry->Eof ) {
        if ( valBagCount != Qry->FieldAsInteger( "num" ) - 1 )
          throw Exception( string( "Нарушена целостность данных (value_bag.grp_id=" ) + IntToString( grp_id ) + ")" );
        xmlNodePtr node = NewTextChild( curNode, "valBag" );
        SetProp( node, "index", valBagCount );
        NewTextChild( node, "value", Qry->FieldAsFloat( "value" ) );
        NewTextChild( node, "value_cur", Qry->FieldAsString( "value_cur" ) );
        NewTextChild( node, "tax_id", Qry->FieldAsInteger( "tax_id" ) );
        NewTextChild( node, "tax", Qry->FieldAsFloat( "tax" ) );
        valBagCount++;
        Qry->Next();
      }
    }
    Qry->Clear();
    Qry->SQLText = "SELECT num,NVL(bag_type,-1) AS bag_type,pr_cabin,amount,weight, "\
                   "       value_bag_num "\
                   "FROM bag2 "\
                   "WHERE grp_id=:grp_id "\
                   "ORDER BY num ";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();
    int BagCount = 0;        
    if ( !Qry->Eof ) {
      curNode = NewTextChild( dataNode, "Bags" );
      while ( !Qry->Eof ) {
        if ( BagCount != Qry->FieldAsInteger( "num" ) - 1 )
          throw Exception( string( "Нарушена целостность данных (bag2.grp_id=" ) + IntToString( grp_id ) + ")" );
        xmlNodePtr node = NewTextChild( curNode, "Bag" );
        SetProp( node, "index", BagCount );        
        NewTextChild( node, "bag_type", Qry->FieldAsInteger( "bag_type" ) );
        NewTextChild( node, "pr_cabin", Qry->FieldAsInteger( "pr_cabin" ) );
        NewTextChild( node, "amount", Qry->FieldAsInteger( "amount" ) );
        NewTextChild( node, "weight", Qry->FieldAsInteger( "weight" ) );
        int j = -1;
        if ( !Qry->FieldIsNULL( "value_bag_num" ) ) {
          j = Qry->FieldAsInteger( "value_bag_num" ) - 1;
          if ( j < 0 || j > valBagCount - 1 )
      	    throw Exception( string( "Нарушена целостность данных (bag2.grp_id=" ) + IntToString( grp_id ) + ")" );
        };
        NewTextChild( node, "value_bag_num", j );
        Qry->Next();
        BagCount++;
      }
    }
    Qry->Clear();
    Qry->SQLText = "SELECT num,tag_type,no_len,no,color,bag_num,pr_ier,pr_print "\
                   "FROM bag_tags,tag_types "\
                   "WHERE bag_tags.tag_type=tag_types.code AND grp_id=:grp_id "\
                   "ORDER BY num";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();    
    if ( !Qry->Eof ) {
      curNode = NewTextChild( dataNode, "Tags" );    
      int TagsCount = 0;
      while ( !Qry->Eof ) {
        if ( TagsCount != Qry->FieldAsInteger( "num" ) - 1 )
          throw Exception( string( "Нарушена целостность данных (bag_tags.grp_id=" ) + IntToString( grp_id ) + ")" );
        xmlNodePtr node = NewTextChild( curNode, "tag" );
        SetProp( node, "index", TagsCount );               
        NewTextChild( node, "tag_type", Qry->FieldAsString( "tag_type" ) );
        NewTextChild( node, "no_len", Qry->FieldAsInteger( "no_len" ) );
        NewTextChild( node, "no", Qry->FieldAsFloat( "no" ) );
        NewTextChild( node, "color", Qry->FieldAsString( "color" ) );
        int j = -1;
        if ( !Qry->FieldIsNULL( "bag_num" ) ) {
      	  j = Qry->FieldAsInteger( "bag_num" ) - 1;
          if ( j < 0 || j > BagCount - 1 ) 
            throw Exception( string( "Нарушена целостность данных (bag_tags.grp_id=" ) + IntToString( grp_id ) + ")" );      	
        }
        NewTextChild( node, "bag_num", j );
        NewTextChild( node, "pr_ier", Qry->FieldAsInteger( "pr_ier" ) );
        NewTextChild( node, "pr_print", Qry->FieldAsInteger( "pr_print" ) );
        Qry->Next();
        TagsCount++;      
      }
    }
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );
};

void PayInterface::LoadPaidBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amRead );	
  int grp_id = NodeAsInteger( "grp_id", reqNode );
  ProgTrace(TRACE2, "PayInterface::LoadPaindBag, grp_id=%d", grp_id );
  TQuery *Qry = OraSession.CreateQuery();
  try {
    Qry->SQLText = "SELECT NVL(paid_bag.bag_type,-1) AS bag_type,paid_bag.weight, "\
                   "       NVL(rate_id,-1) AS rate_id,rate,rate_cur "\
                   "FROM paid_bag,bag_rates "\
                   "WHERE paid_bag.rate_id=bag_rates.id(+) AND grp_id=:grp_id ";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();
    
    SetProp( resNode, "handle", "1" );
    xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
    SetProp( ifaceNode, "id", "pay" );
    SetProp( ifaceNode, "ver", "1" );    
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr curNode;
    if ( !Qry->Eof ) {
      int PaidBagsCount = 0;
      curNode = NewTextChild( dataNode, "PaidBags" );    
      while ( !Qry->Eof ) {
        xmlNodePtr node = NewTextChild( curNode, "PaidBag" );
        SetProp( node, "index", PaidBagsCount );        
        NewTextChild( node, "bag_type", Qry->FieldAsInteger( "bag_type" ) );
        NewTextChild( node, "weight", Qry->FieldAsInteger( "weight" ) );
        NewTextChild( node, "rate_id", Qry->FieldAsInteger( "rate_id" ) );
        NewTextChild( node, "rate", Qry->FieldAsFloat( "rate" ) ); 
        NewTextChild( node, "rate_cur", Qry->FieldAsString( "rate_cur" ) );      	
        Qry->Next();
        PaidBagsCount++;
      }
    }    
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );	
}

void PayInterface::SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
  int grp_id = NodeAsInteger( "grp_id", reqNode );
  ProgTrace(TRACE2, "PayInterface::SaveBag, grp_id=%d", grp_id );
  TQuery *Qry = OraSession.CreateQuery();
  try {
    Qry->SQLText = "BEGIN "\
                   "  DELETE FROM bag_tags WHERE grp_id=:grp_id; "\
                   "  DELETE FROM bag2 WHERE grp_id=:grp_id; "\
                   "  DELETE FROM value_bag WHERE grp_id=:grp_id; "\
                   "END; ";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();
    xmlNodePtr node = GetNode( "valBags", reqNode );      	
    xmlNodePtr curNode;
    if ( node != NULL ) {
      Qry->Clear();
      Qry->SQLText = "INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax) "\
                     "VALUES(:grp_id,:num,:value,:value_cur, "\
                     "       DECODE(:tax_id,-1,NULL,:tax_id), "\
                     "       DECODE(:tax_id,-1,NULL,:tax)) ";
      Qry->DeclareVariable( "grp_id", otInteger );
      Qry->DeclareVariable( "num", otInteger );
      Qry->DeclareVariable( "value", otFloat );
      Qry->DeclareVariable( "value_cur", otString );
      Qry->DeclareVariable( "tax_id", otInteger );
      Qry->DeclareVariable( "tax", otFloat );
      Qry->SetVariable( "grp_id", grp_id );
      curNode = node->children;
      while ( curNode != NULL ) {
        Qry->SetVariable( "num", NodeAsInteger( "num", curNode ) );
        Qry->SetVariable( "value", NodeAsFloat( "value", curNode ) );
        Qry->SetVariable( "value_cur", NodeAsString( "value_cur", curNode ) );
        Qry->SetVariable( "tax_id", NodeAsInteger( "tax_id", curNode ) );
        Qry->SetVariable( "tax", NodeAsFloat( "tax", curNode ) );
        Qry->Execute();      	
      	curNode = curNode->next;
      }	
    }
    node = GetNode( "Bags", reqNode );
    if ( node != NULL ) {
      Qry->Clear();
      Qry->SQLText = "INSERT INTO bag2 (grp_id,num,bag_type,pr_cabin,amount,weight,value_bag_num) "\
                     "VALUES (:grp_id,:num,DECODE(:bag_type,-1,NULL,:bag_type),:pr_cabin, "\
                     "        :amount,:weight,:value_bag_num) ";
      Qry->DeclareVariable( "grp_id", otInteger );
      Qry->DeclareVariable( "num", otInteger );
      Qry->DeclareVariable( "bag_type", otInteger );
      Qry->DeclareVariable( "pr_cabin", otInteger );
      Qry->DeclareVariable( "amount", otInteger );
      Qry->DeclareVariable( "weight", otInteger );
      Qry->DeclareVariable( "value_bag_num", otInteger );
      Qry->SetVariable( "grp_id", grp_id );
      curNode = node->children;
      while ( curNode != NULL ) {
        Qry->SetVariable( "num", NodeAsInteger( "num", curNode ) );
        Qry->SetVariable( "bag_type", NodeAsInteger( "bag_type", curNode ) );
        Qry->SetVariable( "pr_cabin", NodeAsInteger( "pr_cabin", curNode ) );
        Qry->SetVariable( "amount", NodeAsInteger( "amount", curNode ) );
        Qry->SetVariable( "weight", NodeAsInteger( "weight", curNode ) );
        int j = NodeAsInteger( "value_bag_num", curNode );
        if ( j == -1 )
          Qry->SetVariable( "value_bag_num", FNull );
        else
          Qry->SetVariable( "value_bag_num", j );
        Qry->Execute();      	
      	curNode = curNode->next;
      }	      
    }
    node = GetNode( "Tags", reqNode );
    if ( node != NULL ) {
      Qry->Clear();
      Qry->SQLText = "INSERT INTO bag_tags(grp_id,num,tag_type,no,color,bag_num,pr_print) "\
                     "VALUES (:grp_id,:num,:tag_type,:no,:color,:bag_num,:pr_print) ";
      Qry->DeclareVariable( "grp_id", otInteger );
      Qry->DeclareVariable( "num", otInteger );
      Qry->DeclareVariable( "tag_type", otString );
      Qry->DeclareVariable( "no", otFloat );
      Qry->DeclareVariable( "color", otString );
      Qry->DeclareVariable( "bag_num", otInteger );
      Qry->DeclareVariable( "pr_print", otInteger );
      Qry->SetVariable( "grp_id", grp_id );
      curNode = node->children;
      while ( curNode != NULL ) {
        Qry->SetVariable( "num", NodeAsInteger( "num", curNode ) );
        Qry->SetVariable( "tag_type", NodeAsString( "tag_type", curNode ) );
        Qry->SetVariable( "no", NodeAsFloat( "no", curNode ) );
        Qry->SetVariable( "color", NodeAsString( "color", curNode ) );
        int j = NodeAsInteger( "bag_num", curNode );
        if ( j == -1 )
          Qry->SetVariable( "bag_num", FNull );
        else
          Qry->SetVariable( "bag_num", j );
        Qry->SetVariable( "pr_print", NodeAsInteger( "pr_print", curNode ) );
        try {      	
          Qry->Execute();      	
        }
        catch( EOracleError e ) {
          if ( e.Code == 1 ) {
            /* для вывода в нормальном формате номера бирки достаем ее длину из таблицы */
            Qry->Clear();
            Qry->SQLText = "SELECT no_len FROM tag_types WHERE code=:code";
            Qry->DeclareVariable( "code", otString );
            Qry->SetVariable( "code", NodeAsString( "tag_type", curNode ) );
            Qry->Execute();
            ostringstream buf;
            buf<<setw(5)<<setfill('0')<<right<<NodeAsFloat( "no", curNode );
            ProgTrace( TRACE5, "no=%s", buf.str().c_str() );          	
            throw UserException( string( "Бирка " ) + NodeAsString( "tag_type", curNode ) + " " +
                                 NodeAsString( "color", curNode ) + buf.str() + " уже зарегистрирована." );
          }
          else
            throw;
        }
      	curNode = curNode->next;
      }	                           
    }    
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );		
}

void PayInterface::SavePaidBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );  
  int grp_id = NodeAsInteger( "grp_id", reqNode );
  ProgTrace(TRACE2, "PayInterface::SavePaidBag, grp_id=%d", grp_id );
  TQuery *Qry = OraSession.CreateQuery();
  try {
    Qry->SQLText = "DELETE FROM paid_bag WHERE grp_id=:grp_id";
    Qry->DeclareVariable( "grp_id", otInteger );
    Qry->SetVariable( "grp_id", grp_id );
    Qry->Execute();
    xmlNodePtr node = GetNode( "Paids", reqNode );      	
    xmlNodePtr curNode;
    if ( node != NULL ) {
      Qry->Clear();
      Qry->SQLText = "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id) "\
                     "VALUES(:grp_id,DECODE(:bag_type,-1,NULL,:bag_type), "\
                     "       :weight,DECODE(:rate_id,-1,NULL,:rate_id)) ";
      
      Qry->DeclareVariable( "grp_id", otInteger );
      Qry->DeclareVariable( "bag_type", otInteger );
      Qry->DeclareVariable( "weight", otInteger );
      Qry->DeclareVariable( "rate_id", otInteger );
      Qry->SetVariable( "grp_id", grp_id );
      curNode = node->children;
      while ( curNode != NULL ) {
        Qry->SetVariable( "bag_type", NodeAsInteger( "bag_type", curNode ) );
        Qry->SetVariable( "weight", NodeAsInteger( "weight", curNode ) );
        Qry->SetVariable( "rate_id", NodeAsInteger( "rate_id", curNode ) );
        Qry->Execute();      	
      	curNode = curNode->next;
      }
    }
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );		    
}    

void PayInterface::CopyBasicPayTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );  
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
    if ( name == "AIRLINE_BAG_NORMS" )
      TReqInfo::Instance()->MsgToLog( string( "Багажные нормы авиакомпании " ) + airline +
                                      " установлены на основе базовых норм", evtPeriod );
    if ( name == "AIRLINE_BAG_RATES" )
      TReqInfo::Instance()->MsgToLog( string( "Багажные тарифы авиакомпании " ) + airline +
                                      " установлены на основе базовых тарифов", evtPeriod );
    
    if ( name == "AIRLINE_VALUE_BAG_TAXES" )
      TReqInfo::Instance()->MsgToLog( string( "Сборы за ценнось багажа для авиакомпании " ) + airline +
                                      " установлены на основе базовых сборов", evtPeriod );
    if ( name == "AIRLINE_EXCHANGE_RATES" ) 
      TReqInfo::Instance()->MsgToLog( string( "Курсы перевода валют авиакомпании " ) + airline +
                                      " установлены на основе базовых курсов", evtPeriod );
    string msg;
    if ( name == "AIRLINE_BAG_NORMS" )
      msg = "Багажные нормы авиакомпании установлены на основе базовых норм";
    if ( name == "AIRLINE_BAG_RATES" )
      msg = "Багажные тарифы авиакомпании установлены на основе базовых тарифов";
    if ( name == "AIRLINE_VALUE_BAG_TAXES" )
      msg = "Сборы за ценнось багажа для авиакомпании установлены на основе базовых сборов";
    if ( name == "AIRLINE_EXCHANGE_RATES" )
      msg = "Курсы перевода валют авиакомпании установлены на основе базовых курсов";
    showMessage( msg );     
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    throw;
  }
  OraSession.DeleteQuery( *Qry );  
}

void PayInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};


