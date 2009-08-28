#include <stdlib.h>
#include "images.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_utils.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void ImagesInterface::GetisPlaceMap( map<string,bool> &ispl )
{
  ispl.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT code, pr_seat FROM comp_elem_types WHERE pr_del IS NULL OR pr_del=0";
  Qry.Execute();
  while ( !Qry.Eof ) {
    ispl[ Qry.FieldAsString( "code" ) ] = Qry.FieldAsInteger( "pr_seat" );
    Qry.Next();
  }
}

void ImagesInterface::GetImages( xmlNodePtr reqNode, xmlNodePtr resNode )
{
  //TReqInfo::Instance()->user.check_access( amRead );
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
   Qry->SQLText = "SELECT '�' AS def_comp_elem,MAX( time_create ) as lastUpdDate FROM comp_elem_types";
   Qry->Execute();
   lastUpdDate = Qry->FieldAsDateTime( "lastUpdDate" );
   SetProp( imagesNode, "default_comp_elem", Qry->FieldAsString( "def_comp_elem" ) );
   SetProp( imagesNode, "lastUpdDate", DateTimeToStr( lastUpdDate ) );

   bool sendImages = ( fabs( lastUpdDate - NodeAsDateTime( "@lastUpdDate", reqNode ) ) > 5.0/(24.0*60.0*60.0) );

   xmlNodePtr codeNode = GetNode( "codes", reqNode );
   if ( codeNode && !sendImages ) {
   	 codeNode = GetNode( "code", codeNode );
     vector<string> codes;
     while ( codeNode ) {
     	 codes.push_back( NodeAsString( codeNode ) );
     	 codeNode = codeNode->next;
     }
     Qry->Clear();
     Qry->SQLText = "SELECT code FROM comp_elem_types WHERE pr_del IS NULL OR pr_del = 0";
     Qry->Execute();
     while ( !Qry->Eof ) {
     	 if ( find( codes.begin(), codes.end(), Qry->FieldAsString( "code" ) ) == codes.end() ) { // � ���� ���� �, �� ��� �� ������
     	 	sendImages = true;
     	 	break;
     	 }
     	 Qry->Next();
     }
   }

   if ( sendImages ) {
   	 SetProp( imagesNode, "sendimages", "true" );
   }

   /* ����뫠�� �� ����� */
   Qry->Clear();
   Qry->SQLText = "SELECT code, name, pr_seat, image FROM comp_elem_types WHERE pr_del IS NULL OR pr_del = 0";
   Qry->Execute();
   int len = 0;
   while ( !Qry->Eof ) {
     xmlNodePtr imageNode = NewTextChild( imagesNode, "image" );
     NewTextChild( imageNode, "code", Qry->FieldAsString( "code" ) );
     NewTextChild( imageNode, "name", Qry->FieldAsString( "name" ) );
     NewTextChild( imageNode, "pr_seat", Qry->FieldAsInteger( "pr_seat" ) );
     if ( sendImages ) {
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
       string res = StrUtils::b64_encode( (const char*)data, len );
       NewTextChild( imageNode, "image", res.c_str() );
     }
     Qry->Next();
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
  //TReqInfo::Instance()->user.check_access( amWrite );
  ProgTrace(TRACE2, "ImagesInterface::SetImages" );
  TQuery *Qry = OraSession.CreateQuery();
  try {
   Qry->SQLText = "BEGIN "\
                  " UPDATE comp_elem_types "
                  "   SET name=:name,pr_seat=:pr_seat,time_create=:time_create,image=:image, pr_del=0 "\
                  "  WHERE code=:code; "\
                  " IF SQL%NOTFOUND THEN "\
                  "  INSERT INTO comp_elem_types(code,name,pr_seat,time_create,image,pr_del) "\
                  "   VALUES(:code,:name,:pr_seat,:time_create,:image,0); "\
                  " END IF; "\
                  "END;";
   Qry->DeclareVariable( "code", otString );
   Qry->DeclareVariable( "name", otString );
   Qry->DeclareVariable( "pr_seat", otInteger );
   Qry->DeclareVariable( "time_create", otDate );
   Qry->DeclareVariable( "image", otLong );
   xmlNodePtr node = GetNode("data/images", reqNode);
   TDateTime d = NodeAsDateTime( "@time_create", node );
   Qry->SetVariable( "time_create", d );
   if ( node != NULL ) {
     node = node->children;
     string StrDec;
     while ( node ) {
       Qry->SetVariable( "code", NodeAsString( "code", node ) );
       Qry->SetVariable( "name", NodeAsString( "name", node ) );
       Qry->SetVariable( "pr_seat", NodeAsString( "pr_seat", node ) );
       StrDec = NodeAsString( "image", node );
       StrDec = StrUtils::b64_decode( StrDec.c_str(), StrDec.length() );
       Qry->CreateLongVariable( "image", otLongRaw, (void*)StrDec.c_str(), StrDec.length() );
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

void ImagesInterface::GetDrawSalonData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	GetDrawSalonProp( reqNode, resNode );
}

void GetDrawSalonProp( xmlNodePtr reqNode, xmlNodePtr resNode )
{
	ImagesInterface::GetImages( reqNode, resNode );
	TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT code,color,figure FROM comp_layer_types";
  Qry.Execute();
  xmlNodePtr imagesNode = GetNode( "data/images", resNode );
  xmlNodePtr layersNode = GetNode( "layers_color", imagesNode );
  if ( !layersNode )
  	layersNode = NewTextChild( imagesNode, "layers_color" );
  while ( !Qry.Eof ) {
			xmlNodePtr n = NewTextChild( layersNode, "layer", Qry.FieldAsString( "code" ) );
      if ( !Qry.FieldIsNULL( "color" ) )
			  SetProp( n, "color", Qry.FieldAsString( "color" ) );
			if ( !Qry.FieldIsNULL( "figure" ) )
			  SetProp( n, "figure", Qry.FieldAsString( "figure" ) );
  	Qry.Next();
  }
}


void ImagesInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};


