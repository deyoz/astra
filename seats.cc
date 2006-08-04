#include <stdlib.h>
#include "seats.h"
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
#include "str_utils.h"
#include "images.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void SeatsInterface::BuildSalons( TSalons *Salons, xmlNodePtr salonsNode )
{  
  int indplacelist = 0;
  for( vector<TPlaceList*>::iterator placeList = Salons->placelists.begin();
       placeList != Salons->placelists.end(); placeList++ ) {
    xmlNodePtr placeListNode = NewTextChild( salonsNode, "placelist" );           	
    SetProp( placeListNode, "num", (*placeList)->num );
    SetProp( placeListNode, "index", indplacelist );
    int indplace = 0;
    for ( vector<TPlace>::iterator place = (*placeList)->places.begin(); 
          place != (*placeList)->places.end(); place++ ) {
      xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );          	
      SetProp( placeNode, "index", indplace );
      NewTextChild( placeNode, "x", place->x );
      NewTextChild( placeNode, "y", place->y );
      NewTextChild( placeNode, "elem_type", place->elem_type );
      NewTextChild( placeNode, "isplace", place->isplace );
      NewTextChild( placeNode, "xprior", place->xprior );
      NewTextChild( placeNode, "yprior", place->yprior );       
      NewTextChild( placeNode, "agle", place->agle );
      NewTextChild( placeNode, "clname", place->clname );
      NewTextChild( placeNode, "pr_smoke", place->pr_smoke );
      NewTextChild( placeNode, "not_good", place->not_good );         
      NewTextChild( placeNode, "xname", place->xname );
      NewTextChild( placeNode, "yname", place->yname );
      NewTextChild( placeNode, "status", place->status );
      NewTextChild( placeNode, "pr_free", place->pr_free );
      NewTextChild( placeNode, "block", place->block );
      int indrem = 0;
      xmlNodePtr remsNode = NULL;
      xmlNodePtr remNode;
      for ( vector<TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
        if ( !remsNode )
         remsNode = NewTextChild( placeNode, "rems" );
         remNode = NewTextChild( remsNode, "rem" );               	
         SetProp( remNode, "index", indrem );         
         NewTextChild( remNode, "rem", rem->rem );
         NewTextChild( remNode, "pr_denial", rem->pr_denial );
         indrem++;                  
      }
      indplace++;
    }
    indplacelist++;
  }
}

void SeatsInterface::InternalReadSalons( TSalons *Salons )
{
  TReqInfo::Instance()->user.check_access( amRead );	
  ProgTrace( TRACE5, "SeatsInterface::InternalReadSalon with params trip_id=%d, ClassName=%s",
             Salons->trip_id, Salons->ClName.c_str() );	
  TQuery *Qry = OraSession.CreateQuery();
  TQuery *RQry = OraSession.CreateQuery();
  
  try { /* ??? :class */
    Qry->SQLText = "SELECT num,x,y,elem_type,xprior,yprior,agle,pr_smoke,not_good,xname,yname, "\
                   "       status,class,pr_free,enabled,pr_seat isplace "\
                   " FROM trip_comp_elems,comp_elem_types "\
                   "WHERE trip_id=:trip_id AND comp_elem_types.code=trip_comp_elems.elem_type "\
                   " ORDER BY num, x desc, y desc ";
    Qry->DeclareVariable( "trip_id", otInteger );
    Qry->SetVariable( "trip_id", Salons->trip_id );
    /*Qry.DeclareVariable( "class", otString );
      Qry.SetVariable( "class", Salons->ClName );*/
    Qry->Execute();
    if ( Qry->RowCount() == 0 )
      throw UserException( "На рейс не назначен салон" );	
    RQry->SQLText = "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "\
                    " WHERE trip_id=:trip_id "\
                    "ORDER BY num, x desc, y desc ";
    RQry->DeclareVariable( "trip_id", otInteger );
    RQry->SetVariable( "trip_id", Salons->trip_id );
    RQry->Execute();
    string ClName = ""; /* перечисление всех классов, которые есть в салоне */
    TPlaceList *placeList = NULL;
    int num = -1;
    while ( !Qry->Eof ) {
     if ( num != Qry->FieldAsInteger( "num" ) ) {
      if ( placeList && !Salons->ClName.empty() && ClName.find( Salons->ClName ) == string::npos ) {
        placeList->places.clear();
      }
      else {
        placeList = new TPlaceList();
        Salons->placelists.push_back( placeList );  
      }
      ClName.clear();        
      num = Qry->FieldAsInteger( "num" );
      placeList->num = num;
     }
     TPlace place;
     place.x = Qry->FieldAsInteger( "x" );
     place.y = Qry->FieldAsInteger( "y" );
     place.elem_type = Qry->FieldAsString( "elem_type" );
     place.isplace = Qry->FieldAsInteger( "isplace" );
     if ( Qry->FieldIsNULL( "xprior" ) )
       place.xprior = -1;
     else  
       place.xprior = Qry->FieldAsInteger( "xprior" );
     if ( Qry->FieldIsNULL( "yprior" ) )
       place.yprior = -1;       
     else 
       place.yprior = Qry->FieldAsInteger( "yprior" );
     place.agle = Qry->FieldAsInteger( "agle" );
     place.clname = Qry->FieldAsString( "class" );
     place.pr_smoke = Qry->FieldAsInteger( "pr_smoke" );
     if ( Qry->FieldIsNULL( "not_good" ) )
       place.not_good = 0;         
     else 
       place.not_good = Qry->FieldAsInteger( "not_good" );
     place.xname = Qry->FieldAsString( "xname" );
     place.yname = Qry->FieldAsString( "yname" );
     place.status = Qry->FieldAsString( "status" );
     place.pr_free = Qry->FieldAsInteger( "pr_free" );
     if ( Qry->FieldIsNULL( "enabled" ) )
       place.block = 1;
     else
       place.block = 0;
     while ( !RQry->Eof && RQry->FieldAsInteger( "num" ) == num &&
             RQry->FieldAsInteger( "x" ) == place.x &&
             RQry->FieldAsInteger( "y" ) == place.y ) {
       TRem rem;
       rem.rem = RQry->FieldAsString( "rem" );
       rem.pr_denial = RQry->FieldAsInteger( "pr_denial" );
       place.rems.push_back( rem );
       RQry->Next();
     }              
     placeList->places.push_back( place );
     if ( ClName.find( Qry->FieldAsString( "class" ) ) == string::npos )
      ClName += Qry->FieldAsString( "class" );       
     Qry->Next();
    }	/* end while */
    if ( placeList && !Salons->ClName.empty() && ClName.find( Salons->ClName ) == string::npos ) {
      Salons->placelists.pop_back( );
      delete placeList; // нам этот класс/салон не нужен
    }
  }
  catch ( ... ) {
    OraSession.DeleteQuery( *Qry );
    OraSession.DeleteQuery( *RQry );    
    throw;
  }
  OraSession.DeleteQuery( *Qry );  
  OraSession.DeleteQuery( *RQry );  	  
}

void SeatsInterface::XMLReadSalons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "SeatsInterface::XMLReadSalons" );
  TSalons *Salons = new TSalons();      
  try {
    Salons->trip_id = NodeAsInteger( "trip_id", reqNode );
    Salons->ClName = NodeAsString( "ClName", reqNode );
    bool PrepareShow = NodeAsInteger( "PrepareShow", reqNode );
    SetProp( resNode, "handle", "1" );
    xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
    SetProp( ifaceNode, "id", "SeatsInterface" );
    SetProp( ifaceNode, "ver", "1" );    
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    if ( PrepareShow )
      ImagesInterface::GetImages( reqNode, resNode );
    InternalReadSalons( Salons );
    BuildSalons( Salons, NewTextChild( dataNode, "salons" ) );
  }
  catch( ... ) {
    delete Salons;
    throw;
  }
  delete Salons;
};

void SeatsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};


