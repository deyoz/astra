#include <stdlib.h>
#include "images.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "str_utils.h"
#include "astra_utils.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void ImagesInterface::GetImages( xmlNodePtr reqNode, xmlNodePtr resNode )
{
  TReqInfo::Instance()->user.check_access( amRead );	
  ProgTrace( TRACE5, "ImagesInterface::GetImages" );	
  xmlNodePtr dataNode = GetNode( "data", resNode );
  if ( dataNode == NULL )
    dataNode = NewTextChild( resNode, "data" );
  
  xmlNodePtr imagesNode = GetNode( "images", dataNode );
  if ( imagesNode == NULL )
    imagesNode = NewTextChild( dataNode, "images" );
  
  TDateTime lastUpdDate;
  TQuery *Qry = OraSession.CreateQuery();
  string Def_Elem;
  void *data = NULL;  
  try {
   Qry->SQLText = "SELECT def_comp_elem, a.t as lastUpdDate FROM options, "\
                  "(SELECT MAX( time_create ) as t FROM comp_elem_types) a ";
   Qry->Execute();
   lastUpdDate = Qry->FieldAsDateTime( "lastUpdDate" );
   SetProp( imagesNode, "default_comp_elem", Qry->FieldAsString( "def_comp_elem" ) );   
   SetProp( imagesNode, "lastUpdDate", DateTimeToStr( lastUpdDate ) );
   if ( fabs( lastUpdDate - NodeAsDateTime( "@lastUpdDate", reqNode ) ) <= 5.0/(24.0*60.0*60.0) )
    return; /* ���������� �� �㦭� */
   /* ����뫠�� �� ����� */
   Qry->Clear();
   Qry->SQLText = "SELECT code, name, pr_seat, image FROM comp_elem_types WHERE pr_del = 0";
   Qry->Execute();   
   int i = 0;
   int len = 0;
   while ( !Qry->Eof ) {
     xmlNodePtr imageNode = NewTextChild( imagesNode, "image" );
     SetProp( imageNode, "index", i );
     NewTextChild( imageNode, "code", Qry->FieldAsString( "code" ) );
     NewTextChild( imageNode, "name", Qry->FieldAsString( "name" ) );
     NewTextChild( imageNode, "pr_seat", Qry->FieldAsInteger( "pr_seat" ) );
     if ( len != Qry->GetSizeLongField( "image" ) ) {
       len = Qry->GetSizeLongField( "image" );
       if ( data == NULL )
         data = malloc( len );
       else
         data = realloc( data, len );
     }
     if ( data == NULL )
       throw Exception( "�訡�� �ணࠬ��" );     
     Qry->FieldAsLong( "image", data );
     for ( int k=0; k<len; k++ )
       ProgTrace( TRACE5, "c[%i]=%i", k, ((unsigned char*)data)[ k ] );
     string res = b64_encode( (const char*)data, len );
     ProgTrace( TRACE5, "image=%s, len=%d", res.c_str(), res.size() );
     NewTextChild( imageNode, "image", res.c_str() );
     Qry->Next();
     i++;
   }
  }  
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    if ( data != NULL )
      free( data );  	    
    throw;    
  }
  OraSession.DeleteQuery( *Qry );    
  if ( data != NULL )
    free( data );  
}

void ImagesInterface::SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
  ProgTrace(TRACE2, "ImagesInterface::SetImages" );
  TQuery *Qry = OraSession.CreateQuery();
  try {
   Qry->SQLText = "BEGIN "\
                  " UPDATE comp_elem_types SET name=:name,pr_seat=:pr_seat,time_create=:time_create,image=:image "\
                  "  WHERE code=:code; "\
                  " IF SQL%NOTFOUND THEN "\
                  "  INSERT INTO comp_elem_types(code,name,pr_seat,time_create,image) "\
                  "   VALUES(:code,:name,:pr_seat,:time_create,:image); "\
                  " END IF; "\
                  "END;";
   Qry->DeclareVariable( "code", otString );
   Qry->DeclareVariable( "name", otString );
   Qry->DeclareVariable( "pr_seat", otInteger );
   Qry->DeclareVariable( "time_create", otDate );
   Qry->DeclareVariable( "image", otLong );                  
   xmlNodePtr node = GetNode("images", reqNode);
   if ( node != NULL ) {
     node = node->children;
     string StrDec;
     while ( node ) {
       Qry->SetVariable( "code", NodeAsString( "code", node ) );
       Qry->SetVariable( "name", NodeAsString( "name", node ) );
       Qry->SetVariable( "pr_seat", NodeAsString( "pr_seat", node ) );       
       Qry->SetVariable( "time_create", NodeAsDateTime( "time_create", node ) );       
       StrDec = NodeAsString( "image", node );
       StrDec = b64_decode( StrDec.c_str(), StrDec.length() );       
       Qry->CreateLongVariable( "time_create", otLongRaw, (void*)StrDec.c_str(), StrDec.length() );              
       Qry->Execute();
       node = node->next;       
     }	
   }
  }  
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );  	
    throw;    
  }
  OraSession.DeleteQuery( *Qry );    
  showMessage( "����� �ᯥ譮 ��࠭���" ); 
};

void ImagesInterface::GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetImages( reqNode, resNode );
};


void ImagesInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};


