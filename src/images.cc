#include <stdlib.h>
#include "images.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "salons.h"
#include "term_version.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const std::string default_elem_type = "К";  //!!!здесь определяем

void TCompElemTypes::Update()
{
  is_places.clear();
  max_time_create = -1;
  default_elem_code.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT code, pr_seat, pr_del, time_create, image FROM comp_elem_types";
  Qry.Execute();
  int len = 0;
  void *data = NULL;
  try {
    for ( ;!Qry.Eof; Qry.Next() ) {
      if ( Qry.FieldIsNULL( "pr_del" ) || Qry.FieldAsInteger( "pr_del" ) == 0 ) {
        TCompElemType comp_elem;
        comp_elem.code = Qry.FieldAsString( "code" );
        comp_elem.name = ElemIdToNameLong( etCompElemType, comp_elem.code );
        comp_elem.is_seat = Qry.FieldAsInteger( "pr_seat" );
        comp_elem.is_default = ( comp_elem.code == default_elem_type );
        if ( comp_elem.is_default ) {
          default_elem_code = comp_elem.code;
        }
        comp_elem.time_create = Qry.FieldAsDateTime( "time_create" );
        if ( len != Qry.GetSizeLongField( "image" ) ) {
          len = Qry.GetSizeLongField( "image" );
          if ( data == NULL )
            data = malloc( len );
          else
            data = realloc( data, len );
        }
        if ( data == NULL )
          throw Exception( "Ошибка программы" );
        Qry.FieldAsLong( "image", data );
        StringToHex( string((char*)data, len), comp_elem.image );
        if ( max_time_create < comp_elem.time_create )
          max_time_create = comp_elem.time_create;
        is_places.insert( make_pair( comp_elem.code, comp_elem ) );
      }
    }
    if ( data != NULL )
      free( data );
  }
  catch(...) {
    if ( data != NULL )
      free( data );
    throw;
  }
}

TCompElemTypes::TCompElemTypes()
{
  Update();
}

void ImagesInterface::GetImages( xmlNodePtr reqNode, xmlNodePtr resNode )
{
  ProgTrace( TRACE5, "ImagesInterface::GetImages" );

  xmlNodePtr dataNode = GetNode( "data", resNode );
  if ( dataNode == NULL )
    dataNode = NewTextChild( resNode, "data" );

  xmlNodePtr imagesNode = GetNode( "images", dataNode );
  if ( imagesNode == NULL )
    imagesNode = NewTextChild( dataNode, "images" );
  SetProp( imagesNode, "default_comp_elem", TCompElemTypes::Instance()->getDefaultElemType() );
  SetProp( imagesNode, "lastUpdDate", DateTimeToStr( TCompElemTypes::Instance()->getLastUpdateDate() ) );

   bool sendImages = ( fabs( TCompElemTypes::Instance()->getLastUpdateDate() - NodeAsDateTime( "@lastUpdDate", reqNode ) ) > 5.0/(24.0*60.0*60.0) );

   std::vector<std::string> server_elem_types;
   TCompElemTypes::Instance()->getElemTypes( server_elem_types );
   xmlNodePtr codeNode = GetNode( "codes", reqNode );
   if ( codeNode && !sendImages ) {
   	 codeNode = GetNode( "code", codeNode );
     std::vector<std::string> client_elem_codes;
     while ( codeNode && string((char*)codeNode->name) == "code" ) {
     	 client_elem_codes.push_back( NodeAsString( codeNode ) );
     	 codeNode = codeNode->next;
     }
     // надо убедиться что на клиенте есть все элементы сервера
     for ( std::vector<std::string>::iterator icode=server_elem_types.begin();
           icode!=server_elem_types.end(); icode++ ) {
       if ( find( client_elem_codes.begin(), client_elem_codes.end(), *icode ) == client_elem_codes.end() ) {
         sendImages = true;
     	   break;
       }
     }
   }
   if ( sendImages ) {
   	 SetProp( imagesNode, "sendimages", "true" );
   }
   /* пересылаем все данные */
   TCompElemType elem_type;
   for ( std::vector<std::string>::iterator icode=server_elem_types.begin();
         icode!=server_elem_types.end(); icode++ ) {
     if ( TCompElemTypes::Instance()->getElem( *icode, elem_type ) ) {
       xmlNodePtr imageNode = NewTextChild( imagesNode, "image" );
       NewTextChild( imageNode, "code", elem_type.code );
       NewTextChild( imageNode, "name", elem_type.name );
       NewTextChild( imageNode, "pr_seat", elem_type.is_seat );
       if ( sendImages ) {
         NewTextChild( imageNode, "image", elem_type.image );
       }
     }
   }
}

void ImagesInterface::SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
        HexToString( string(StrDec), StrDec );
        Qry->CreateLongVariable( "image", otLongRaw, (void*)StrDec.c_str(), StrDec.length() );
        Qry->Execute();
        node = node->next;
      }
    }
    TCompElemTypes::Instance()->Update(); //перечитать информацию
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );
  AstraLocale::showMessage( "MSG.DATA_SAVED" );
};

void ImagesInterface::GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetImages( reqNode, resNode );
};

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
      ASTRA::TCompLayerType l = DecodeCompLayerType( Qry.FieldAsString( "code" ) );
      if ( !SALONS2::compatibleLayer( l ) ) {
        Qry.Next();
        continue;
      }

			xmlNodePtr n = NewTextChild( layersNode, "layer", Qry.FieldAsString( "code" ) );
      if ( !Qry.FieldIsNULL( "color" ) )
			  SetProp( n, "color", Qry.FieldAsString( "color" ) );
			if ( !Qry.FieldIsNULL( "figure" ) )
			  SetProp( n, "figure", Qry.FieldAsString( "figure" ) );
  	Qry.Next();
  }
}

void GetDrawWebTariff( xmlNodePtr reqNode, xmlNodePtr resNode )
{
	TReqInfo *reqInfo = TReqInfo::Instance();
  if ( find( reqInfo->user.access.rights.begin(), reqInfo->user.access.rights.end(), 431) == reqInfo->user.access.rights.end() )
  	return;

	xmlNodePtr tariffsNode = GetNode( "data", resNode );
	if ( !tariffsNode ) {
		tariffsNode = NewTextChild( resNode, "data" );
	}
	tariffsNode = GetNode( "data/images", resNode );
	if ( !tariffsNode ) {
		tariffsNode = NewTextChild( tariffsNode, "images" );
	}
	tariffsNode = NewTextChild( tariffsNode, "web_tariff_property" );
	xmlNodePtr n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$00CECF00" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$004646FF" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$000DCAA4" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$00FF64FF" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$00000000" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$001C66FF" );
	SetProp( n, "figure", "rurect" );
  n = NewTextChild( tariffsNode, "tarif" );
	SetProp( n, "color", "$00FD2D71" );
	SetProp( n, "figure", "rurect" );
}

void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode)
{
	GetDrawSalonProp( reqNode, resNode );
	GetDrawWebTariff( reqNode, resNode );
}

void ImagesInterface::GetDrawSalonData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	GetDataForDrawSalon( reqNode, resNode );
}

void ImagesInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};


