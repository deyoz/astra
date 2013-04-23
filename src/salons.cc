#include <stdlib.h>
#include <boost/crc.hpp>
#include "salons.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "oralib.h"
#include "images.h"
#include "convert.h"
#include "astra_misc.h"
#include "astra_locale.h"
#include "base_tables.h"
#include "term_version.h"
#include "alarms.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC;
using namespace ASTRA;

namespace SALONS2
{

const int REM_VIP_F = 1;
const int REM_VIP_C = 1;
const int REM_VIP_Y = 3;


TDrawPropInfo getDrawProps( TDrawPropsType proptype )
{
  TDrawPropInfo res;
  if ( proptype == dpInfantWoSeats ) {
    res.figure = "framework";
    res.color = "$0005A5FA";
    res.name = "Взрослый с младенцем";
  }
  return res;
};

struct CompRoute {
  int point_id;
  string airline;
  string airp;
  string craft;
  string bort;
  bool pr_reg;
  bool pr_alarm;
  bool auto_comp_chg;
  bool inRoutes;
  CompRoute() {
     point_id = NoExists;
     pr_alarm = false;
     pr_reg = false;
     auto_comp_chg = false;
     inRoutes = false;
  };
};

typedef vector<CompRoute> TCompsRoutes;

bool compatibleLayer( TCompLayerType layer_type )
{
  if ( layer_type == cltDisable &&
       !TReqInfo::Instance()->desk.compatible( DISABLE_LAYERS ) )
    return false;
  if ( ( layer_type == ASTRA::cltProtBeforePay ||
         layer_type == ASTRA::cltProtAfterPay ||
         layer_type == ASTRA::cltPNLBeforePay ||
         layer_type == ASTRA::cltPNLAfterPay ) &&
        !TReqInfo::Instance()->desk.compatible( PROT_PAID_VERSION ) )
    return false;
  return true;
}

bool isBaseLayer( TCompLayerType layer_type, TReadStyle readStyle )
{
  return ( layer_type == cltSmoke ||
           layer_type == cltUncomfort ||
           ( ( layer_type == cltDisable || layer_type == cltProtect ) && readStyle == rComponSalons ) );
}

bool point_dep_AND_layer_type_FOR_TRZT_SOM_PRL( int point_id, int &point_dep, TCompLayerType &layer_type )
{
	TQuery Qry( &OraSession );
  Qry.SQLText=
  "SELECT point_num, "
  "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
  " FROM points "
  " WHERE points.point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
  	return false;
  int point_num = Qry.FieldAsInteger( "point_num" );
  int first_point = Qry.FieldAsInteger( "first_point" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT points.point_id AS point_dep, "
    "       DECODE(tlgs_in.type,'PRL',:prl_layer,:som_layer) AS layer_type "
    "FROM tlg_binding,tlg_source,tlgs_in, "
    "     (SELECT point_id,point_num FROM points "
    "      WHERE :first_point IN (first_point,point_id) AND point_num<:point_num AND pr_del=0 "
    "      ORDER BY point_num "
    "     ) points "
    "WHERE tlg_binding.point_id_spp=points.point_id AND "
    "      tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
    "      tlgs_in.id=tlg_source.tlg_id AND tlgs_in.num=1 AND tlgs_in.type IN ('PRL','SOM') "
    "ORDER BY point_num DESC,DECODE(tlgs_in.type,'PRL',0,1)";
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.CreateVariable( "som_layer", otString, EncodeCompLayerType( cltSOMTrzt ) );
  Qry.CreateVariable( "prl_layer", otString, EncodeCompLayerType( cltPRLTrzt ) );
  Qry.Execute();
  if ( Qry.Eof )
  	return false;
  point_dep = Qry.FieldAsInteger( "point_dep" );
  layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
  return true;
}


void TFilterLayers::getFilterLayers( int point_id )
{
	clearFlags();
	for ( int l=0; l!=cltTypeNum; l++ ) {
		if ( (TCompLayerType)l == cltTranzit ||
			   (TCompLayerType)l == cltProtTrzt ||
			   (TCompLayerType)l == cltBlockTrzt ||
			   (TCompLayerType)l == cltTranzit ||
			   (TCompLayerType)l == cltSOMTrzt ||
			   (TCompLayerType)l == cltPRLTrzt ||
         (TCompLayerType)l == cltProtBeforePay ||
         (TCompLayerType)l == cltProtAfterPay ||
         (TCompLayerType)l == cltPNLBeforePay ||
         (TCompLayerType)l == cltPNLAfterPay )
			continue;
	  setFlag( (TCompLayerType)l );
  }
	TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText =
	"SELECT pr_permit FROM trip_paid_ckin WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	
	if ( !Qry.Eof && Qry.FieldAsInteger( "pr_permit" )!=0 ) {
    setFlag( cltProtBeforePay );
    setFlag( cltProtAfterPay );
    setFlag( cltPNLBeforePay );
    setFlag( cltPNLAfterPay );
  }
  
  Qry.Clear();
	Qry.SQLText =
	"SELECT pr_tranz_reg,pr_block_trzt,ckin.get_pr_tranzit(:point_id) as pr_tranzit "
	"FROM trip_sets "
	"WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	
	if ( !Qry.Eof && Qry.FieldAsInteger( "pr_tranzit" ) ) { // это транзитный рейс
		if ( Qry.FieldAsInteger( "pr_tranz_reg" ) ) {
  	  setFlag( cltProtTrzt );
  	  setFlag( cltTranzit );
		}
		else {
			if ( Qry.FieldAsInteger( "pr_block_trzt" ) ) {
			  setFlag( cltBlockTrzt );
      }
			else {
				 TCompLayerType layer_tlg;
				if ( point_dep_AND_layer_type_FOR_TRZT_SOM_PRL( point_id, point_dep, layer_tlg ) ) {
				  setFlag( layer_tlg );
				}
			}
		}
	}
}

bool TFilterLayers::CanUseLayer( TCompLayerType layer_type, int point_id )
{
	switch( layer_type ) {
		case cltSOMTrzt:
			return ( isFlag( layer_type ) && point_dep == point_id );
		case cltPRLTrzt:
			return ( isFlag( layer_type ) && point_dep == point_id );
		default:
			return isFlag( layer_type );
	}
}

void TSalons::Clear( )
{
  FCurrPlaceList = NULL;
  for ( std::vector<TPlaceList*>::iterator i = placelists.begin(); i != placelists.end(); i++ ) {
    delete *i;
  }
  placelists.clear();
}

TSalons::TSalons( int id, TReadStyle vreadStyle, bool vdrop_not_used_pax_layers )
{
  drop_not_used_pax_layers = vdrop_not_used_pax_layers;
	readStyle = vreadStyle;
	if ( readStyle == rComponSalons )
		comp_id = id;
	else
	  trip_id = id;
	pr_lat_seat = false;
  FCurrPlaceList = NULL;
  modify = mNone;
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT code,priority,pr_occupy FROM comp_layer_types ORDER BY priority";
  Qry.Execute();
  while ( !Qry.Eof ) {
  	TCompLayerType l = DecodeCompLayerType( Qry.FieldAsString( "code" ) );
  	if ( l != cltUnknown ) {
  		layers_priority[ l ].name = ElemIdToNameLong(etCompLayerType,Qry.FieldAsString( "code" ));
  	  layers_priority[ l ].priority = Qry.FieldAsInteger( "priority" );
  	  layers_priority[ l ].pr_occupy = Qry.FieldAsInteger( "pr_occupy" );
  	}
  	Qry.Next();
  }
  
 	layers_priority[ cltUncomfort ].editable = true;
  layers_priority[ cltUncomfort ].name_view = layers_priority[ cltUncomfort ].name;
 	layers_priority[ cltSmoke ].editable = true;
  layers_priority[ cltSmoke ].name_view = layers_priority[ cltSmoke ].name;
 	layers_priority[ cltDisable ].editable = true;
  layers_priority[ cltDisable ].name_view = layers_priority[ cltDisable ].name;
  layers_priority[ cltProtect ].editable = true;
  layers_priority[ cltProtect ].name_view = layers_priority[ cltProtect ].name;

  if ( readStyle == rTripSalons ) {

    FilterLayers.getFilterLayers( trip_id ); // определение режима учета транзитных слоев

   	layers_priority[ cltBlockCent ].editable = true;
   	layers_priority[ cltBlockCent ].notfree = true;
    if ( FilterLayers.isFlag( cltProtTrzt ) )
    	layers_priority[ cltProtTrzt ].editable = true;
    else
    	layers_priority[ cltProtTrzt ].notfree = true;
    if ( FilterLayers.isFlag( cltBlockTrzt ) )
    	layers_priority[ cltBlockTrzt ].editable = true;
    else
    	layers_priority[ cltBlockTrzt ].notfree = true;
    layers_priority[ cltTranzit ].notfree = true;
    layers_priority[ cltCheckin ].notfree = true;
    layers_priority[ cltTCheckin ].notfree = true;
    layers_priority[ cltGoShow ].notfree = true;
    layers_priority[ cltSOMTrzt ].notfree = true;
    layers_priority[ cltPRLTrzt ].notfree = true;
    // что отобразить в help Ctrl+F4 - занято на клиенте
    layers_priority[ cltBlockCent ].name_view = layers_priority[ cltBlockCent ].name;
    if ( FilterLayers.isFlag( cltBlockCent ) )
    	layers_priority[ cltBlockCent ].func_key = "Shift+F2";
    if ( FilterLayers.isFlag( cltTranzit ) ||
    	   FilterLayers.isFlag( cltSOMTrzt ) ||
    	   FilterLayers.isFlag( cltPRLTrzt ) ) {
      layers_priority[ cltBlockTrzt ].name_view = AstraLocale::getLocaleText("Транзит");
    }
    layers_priority[ cltCheckin ].name_view = AstraLocale::getLocaleText("Регистрация");
    if ( FilterLayers.isFlag( cltProtTrzt ) ) {
    	layers_priority[ cltProtTrzt ].name_view = layers_priority[ cltProtTrzt ].name;
      layers_priority[ cltProtTrzt ].func_key = "Shift+F3";
    }
    if ( FilterLayers.isFlag( cltBlockTrzt ) ) {
    	layers_priority[ cltBlockTrzt ].name_view = layers_priority[ cltBlockTrzt ].name;
      layers_priority[ cltBlockTrzt ].func_key = "Shift+F3";
    }
    layers_priority[ cltPNLCkin ].name_view = layers_priority[ cltPNLCkin ].name;
    layers_priority[ cltProtCkin ].name_view = layers_priority[ cltProtCkin ].name;
    
    if ( FilterLayers.isFlag( cltProtBeforePay ) ||
         FilterLayers.isFlag( cltProtAfterPay ) ||
         FilterLayers.isFlag( cltPNLBeforePay ) ||
         FilterLayers.isFlag( cltPNLAfterPay ) )
      layers_priority[ cltProtBeforePay ].name_view = AstraLocale::getLocaleText("Резервирование платного места");
    if ( FilterLayers.isFlag( cltProtect ) )
      layers_priority[ cltProtect ].func_key = "Shift+F4";
    if ( FilterLayers.isFlag( cltUncomfort ) )
    	layers_priority[ cltUncomfort ].func_key = "Shift+F5";
    if ( FilterLayers.isFlag( cltSmoke ) )
    	layers_priority[ cltSmoke ].func_key = "Shift+F6";
    if ( FilterLayers.isFlag( cltDisable ) )
    	layers_priority[ cltDisable ].func_key = "Shift+F1";
  }
}

TSalons::~TSalons()
{
  Clear( );
}

TPlaceList *TSalons::CurrPlaceList()
{
  return FCurrPlaceList;
}

void TSalons::SetCurrPlaceList( TPlaceList *newPlaceList )
{
  FCurrPlaceList = newPlaceList;
}

void TSalons::BuildLayersInfo( xmlNodePtr salonsNode, const std::vector<TDrawPropsType> &props )
{
  int max_priority = -1;
  int id = 0;
  xmlNodePtr editNode = GetNode( "layers_prop", salonsNode );
  if ( editNode ) {
    ProgTrace( TRACE5, "TSalons::BuildLayersInfo - recreate" );
    xmlUnlinkNode( editNode );
    xmlFreeNode( editNode );
  }
  else
    ProgTrace( TRACE5, "TSalons::BuildLayersInfo - create" );
  editNode = NewTextChild( salonsNode, "layers_prop" );
  TReqInfo *r = TReqInfo::Instance();
  for( map<TCompLayerType,TLayerProp>::iterator i=layers_priority.begin(); i!=layers_priority.end(); i++ ) {
    if ( !compatibleLayer( i->first ) )
      continue;
    if ( readStyle == rComponSalons &&
         !isBaseLayer( i->first, readStyle ) )
      continue;
  	xmlNodePtr n = NewTextChild( editNode, "layer", EncodeCompLayerType( i->first ) );
  	SetProp( n, "id", id );
  	SetProp( n, "name", i->second.name );
  	SetProp( n, "priority", i->second.priority );
  	if ( max_priority < i->second.priority )
      max_priority = i->second.priority;
  	if ( i->second.editable ) { // надо еще проверить на права редактирования того или иного слоя
  		bool pr_edit = true;
    	if ( (i->first == cltBlockTrzt || i->first == cltProtTrzt )&&
  		   find( r->user.access.rights.begin(),  r->user.access.rights.end(), 430 ) == r->user.access.rights.end() )
  		  pr_edit = false;
  	  if ( i->first == cltBlockCent &&
  		   find( r->user.access.rights.begin(),  r->user.access.rights.end(), 420 ) == r->user.access.rights.end() )
   	    pr_edit = false;
      if ( (i->first == cltUncomfort || i->first == cltProtect || i->first == cltSmoke) &&
    	     find( r->user.access.rights.begin(),  r->user.access.rights.end(), 410 ) == r->user.access.rights.end() )
    	  pr_edit = false;
      if ( i->first == cltDisable &&
      	   find( r->user.access.rights.begin(),  r->user.access.rights.end(), 425 ) == r->user.access.rights.end() )
      	pr_edit = false;
    	if ( pr_edit ) {
  		  SetProp( n, "edit", 1 );
  		  if ( isBaseLayer( i->first, readStyle ) )
  		  	SetProp( n, "base_edit", 1 );
  		}
  	}
  	if ( i->second.notfree )
  		SetProp( n, "notfree", 1 );
  	if ( !i->second.name_view.empty() ) {
  		SetProp( n, "name_view_help", i->second.name_view );
  		if ( !i->second.func_key.empty() )
  			SetProp( n, "func_key", i->second.func_key );
  	}
   id++;
  }
  xmlNodePtr n = NewTextChild( editNode, "layer",  EncodeCompLayerType( cltUnknown ) );
  SetProp( n, "id", id );
 	SetProp( n, "name", "LAYER_CLEAR_ALL" );
 	SetProp( n, "priority", 10000 );
 	SetProp( n, "edit", 1 );
  SetProp( n, "name_view_help", AstraLocale::getLocaleText("Очистить все статусы мест") );
  SetProp( n, "func_key", "Shift+F8" );
  xmlNodePtr propNode = NewTextChild( salonsNode, "draw_props" );
  max_priority++;
  id++;
  ProgTrace( TRACE5, "max_priority=%d, props.size()=%zu", max_priority, props.size() );
  for ( vector<TDrawPropsType>::const_iterator i=props.begin(); i!=props.end(); i++ ) {
    TDrawPropInfo p = getDrawProps( *i );
    ProgTrace( TRACE5, "priority=%d, name=%s", max_priority, p.name.c_str() );
    n = NewTextChild( propNode, "draw_item", p.name );
    SetProp( n, "id", id );
    SetProp( n, "figure", p.figure );
    SetProp( n, "color", p.color );
    SetProp( n, "priority", max_priority );
    max_priority++;
  }
}

void TSalons::Build( xmlNodePtr salonsNode )
{
  vector<TDrawPropsType> props;
	SetProp( salonsNode, "pr_lat_seat", pr_lat_seat );
  for( vector<TPlaceList*>::iterator placeList = placelists.begin();
       placeList != placelists.end(); placeList++ ) {
    xmlNodePtr placeListNode = NewTextChild( salonsNode, "placelist" );
    SetProp( placeListNode, "num", (*placeList)->num );
    int xcount=0, ycount=0;
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      if ( !place->visible )
       continue;
      xmlNodePtr placeNode = NewTextChild( placeListNode, "place" );
      NewTextChild( placeNode, "x", place->x );
      NewTextChild( placeNode, "y", place->y );
      if ( place->x > xcount )
      	xcount = place->x;
      if ( place->y > ycount )
      	ycount = place->y;
      NewTextChild( placeNode, "elem_type", place->elem_type );
      if ( place->xprior != -1 )
        NewTextChild( placeNode, "xprior", place->xprior );
      if ( place->yprior != -1 )
        NewTextChild( placeNode, "yprior", place->yprior );
      if ( place->agle )
        NewTextChild( placeNode, "agle", place->agle );
      NewTextChild( placeNode, "class", place->clname );
      NewTextChild( placeNode, "xname", denorm_iata_line( place->xname, pr_lat_seat ) );
      NewTextChild( placeNode, "yname", denorm_iata_row( place->yname, NULL ) );
      xmlNodePtr remsNode = NULL;
      xmlNodePtr remNode;
      for ( vector<TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
        if ( !remsNode ) {
          remsNode = NewTextChild( placeNode, "rems" );
        }
        remNode = NewTextChild( remsNode, "rem" );
        NewTextChild( remNode, "rem", rem->rem );
        if ( rem->pr_denial )
          NewTextChild( remNode, "pr_denial" );
      }
      if ( !place->layers.empty() ) {
      	xmlNodePtr layersNode = NewTextChild( placeNode, "layers" );
      	for( std::vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
      		if ( l->layer_type  == cltDisable && !compatibleLayer( l->layer_type ) ) {
            if ( !remsNode ) {
              remsNode = NewTextChild( placeNode, "rems" );
            }
      		  remNode = NewTextChild( remsNode, "rem" );
      		  NewTextChild( remNode, "rem", "X" );    //!!!
      		  continue;
      		}
      		remNode = NewTextChild( layersNode, "layer" );
      		NewTextChild( remNode, "layer_type", EncodeCompLayerType( l->layer_type ) );
      	}
      }
      if ( place->WebTariff.value != 0.0 ) {
      	remNode = NewTextChild( placeNode, "tariff", place->WebTariff.value );
      	SetProp( remNode, "color", place->WebTariff.color );
      	SetProp( remNode, "currency_id", place->WebTariff.currency_id );
      }
      if ( !place->drawProps.empty() ) {
        remsNode = NewTextChild( placeNode, "drawProps" );
        for ( vector<TDrawPropsType>::iterator iprop=place->drawProps.begin(); iprop!=place->drawProps.end(); iprop++ ) {
          remNode = NewTextChild( remsNode, "drawProp" );
          TDrawPropInfo pinfo = getDrawProps( *iprop );
          SetProp( remNode, "figure", pinfo.figure );
          SetProp( remNode, "color", pinfo.color );
          vector<TDrawPropsType>::const_iterator jprop=props.begin();
          for ( ; jprop!=props.end(); jprop++ ) {
            if ( *iprop == *jprop )
             break;
          }
          if ( jprop == props.end() ) {
            props.push_back( *iprop );
          }
        }
      }
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
/*  if ( readStyle == rTripSalons ) {*/
  	BuildLayersInfo( salonsNode, props );
/*  }*/
}

void TSalons::Write()
{
  if ( readStyle == rTripSalons )
    ProgTrace( TRACE5, "TSalons::Write TripSalons with params trip_id=%d",
               trip_id );
  else {
    if ( modify == mNone )
      return;  //???
    FilterClass.clear();
    ProgTrace( TRACE5, "TSalons::Write ComponSalons with params comp_id=%d",
               comp_id );
  }
  TQuery Qry( &OraSession );
  TQuery QryLayers( &OraSession );
  QryLayers.SQLText =
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id,time_create) "
    "  VALUES "
    "    (:range_id,:point_id,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:crs_pax_id,:pax_id,system.UTCSYSDATE); "
    "END; ";
  QryLayers.CreateVariable( "range_id", otInteger, FNull );
  QryLayers.CreateVariable( "point_id", otInteger, trip_id );
  QryLayers.CreateVariable( "crs_pax_id", otInteger, FNull );
  QryLayers.CreateVariable( "pax_id", otInteger, FNull );
  QryLayers.DeclareVariable( "layer_type", otString );
  QryLayers.DeclareVariable( "first_xname", otString );
  QryLayers.DeclareVariable( "last_xname", otString );
  QryLayers.DeclareVariable( "first_yname", otString );
  QryLayers.DeclareVariable( "last_yname", otString );
  if ( readStyle == rTripSalons ) {
    Qry.SQLText = "BEGIN "\
                  " UPDATE points SET point_id=point_id WHERE point_id=:point_id; "
                  " UPDATE trip_sets SET pr_lat_seat=:pr_lat_seat WHERE point_id=:point_id; "
                  " DELETE trip_comp_rem WHERE point_id=:point_id; "
                  " DELETE trip_comp_baselayers WHERE point_id=:point_id; "
                  " DELETE trip_comp_rates WHERE point_id=:point_id; "
                  " DELETE trip_comp_elems WHERE point_id=:point_id; "
                  "END;";

    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText =
      "DELETE trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type=:layer_type";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.DeclareVariable( "layer_type", otString );
    for( map<TCompLayerType,TLayerProp>::iterator i=layers_priority.begin(); i!=layers_priority.end(); i++ ) {
    	if ( i->second.editable ) {
    		Qry.SetVariable( "layer_type", EncodeCompLayerType( i->first ) );
    		Qry.Execute();
    	}
    }
  }
  else { /* сохранение компоновки */
    if ( modify == mAdd ) {
      Qry.Clear();
      Qry.SQLText = "SELECT id__seq.nextval as comp_id FROM dual";
      Qry.Execute();
      comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    Qry.Clear();
    switch ( (int)modify ) {
      case mChange:
         Qry.SQLText = "BEGIN "\
                       " UPDATE comps SET airline=:airline,airp=:airp,craft=:craft,bort=:bort,descr=:descr, "
                       "        time_create=system.UTCSYSDATE,classes=:classes,pr_lat_seat=:pr_lat_seat "
                       "  WHERE comp_id=:comp_id; "
                       " DELETE comp_rem WHERE comp_id=:comp_id; "
                       " DELETE comp_baselayers WHERE comp_id=:comp_id; "
                       " DELETE comp_rates WHERE comp_id=:comp_id; "
                       " DELETE comp_elems WHERE comp_id=:comp_id; "
                       " DELETE comp_classes WHERE comp_id=:comp_id; "
                       "END; ";
         break;
      case mAdd:
         Qry.SQLText = "INSERT INTO comps(comp_id,airline,airp,craft,bort,descr,time_create,classes,pr_lat_seat) "
                       " VALUES(:comp_id,:airline,:airp,:craft,:bort,:descr,system.UTCSYSDATE,:classes,:pr_lat_seat) ";
         break;
      case mDelete:
         Qry.SQLText = "BEGIN "
                       " UPDATE trip_sets SET comp_id=NULL WHERE comp_id=:comp_id; "
                       " DELETE comp_rem WHERE comp_id=:comp_id; "
                       " DELETE comp_baselayers WHERE comp_id=:comp_id; "
                       " DELETE comp_rates WHERE comp_id=:comp_id; "
                       " DELETE comp_elems WHERE comp_id=:comp_id; "
                       " DELETE comp_sections WHERE comp_id=:comp_id; "
                       " DELETE comp_classes WHERE comp_id=:comp_id; "
                       " DELETE comps WHERE comp_id=:comp_id; "
                       "END; ";
         break;
    }
    Qry.DeclareVariable( "comp_id", otInteger );
    Qry.SetVariable( "comp_id", comp_id );
    if ( modify != mDelete ) {
      Qry.CreateVariable( "airline", otString, airline );
      Qry.CreateVariable( "airp", otString, airp );
      Qry.CreateVariable( "craft", otString, craft );
      Qry.CreateVariable( "descr", otString, descr );
      Qry.CreateVariable( "bort", otString, bort );
      Qry.CreateVariable( "classes", otString, classes );
      Qry.CreateVariable( "pr_lat_seat", otString, pr_lat_seat );
    }
    Qry.Execute();
  }
  if ( readStyle == rComponSalons && modify == mDelete )
    return; /* удалили компоновку */

  TQuery QryWebTariff( &OraSession );
  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
      " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
    LQry.SQLText =
      "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
      " VALUES(:point_id,:num,:x,:y,:layer_type)";
    LQry.CreateVariable( "point_id", otInteger, trip_id );
    QryWebTariff.SQLText =
      "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
      " VALUES(:point_id,:num,:x,:y,:color,:rate,:rate_cur)";
    QryWebTariff.CreateVariable( "point_id", otInteger, trip_id );
  }
  else {
    RQry.SQLText = "INSERT INTO comp_rem(comp_id,num,x,y,rem,pr_denial) "
                   " VALUES(:comp_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
    LQry.SQLText =
      "INSERT INTO comp_baselayers(comp_id,num,x,y,layer_type) "
      " VALUES(:comp_id,:num,:x,:y,:layer_type)";
    LQry.CreateVariable( "comp_id", otInteger, comp_id );
    QryWebTariff.SQLText =
      "INSERT INTO comp_rates(comp_id,num,x,y,color,rate,rate_cur) "
      " VALUES(:comp_id,:num,:x,:y,:color,:rate,:rate_cur)";
    QryWebTariff.CreateVariable( "comp_id", otInteger, comp_id );
  }

  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "x", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "rem", otString );
  RQry.DeclareVariable( "pr_denial", otInteger );

  QryWebTariff.DeclareVariable( "num", otInteger );
  QryWebTariff.DeclareVariable( "x", otInteger );
  QryWebTariff.DeclareVariable( "y", otInteger );
  QryWebTariff.DeclareVariable( "color", otString );
  QryWebTariff.DeclareVariable( "rate", otFloat );
  QryWebTariff.DeclareVariable( "rate_cur", otString );

  LQry.DeclareVariable( "num", otInteger );
  LQry.DeclareVariable( "x", otInteger );
  LQry.DeclareVariable( "y", otInteger );
  LQry.DeclareVariable( "layer_type", otString );

  Qry.Clear();
  if ( readStyle == rTripSalons ) {
    Qry.SQLText =
      "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
      " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, :xname,:yname)";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
  }
  else {
    Qry.SQLText =
      "INSERT INTO comp_elems(comp_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
      " VALUES(:comp_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class,:xname,:yname) ";
    Qry.DeclareVariable( "comp_id", otInteger );
    Qry.SetVariable( "comp_id", comp_id );
  }
  Qry.DeclareVariable( "num", otInteger );
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );
  Qry.DeclareVariable( "elem_type", otString );
  Qry.DeclareVariable( "xprior", otInteger );
  Qry.DeclareVariable( "yprior", otInteger );
  Qry.DeclareVariable( "agle", otInteger );
  Qry.DeclareVariable( "class", otString );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );

  vector<TPlaceList*>::iterator plist;
  map<TClass,int> countersClass;
  TClass cl;

  for ( plist = placelists.begin(); plist != placelists.end(); plist++ ) {
    Qry.SetVariable( "num", (*plist)->num );
    RQry.SetVariable( "num", (*plist)->num );
    LQry.SetVariable( "num", (*plist)->num );
    for ( TPlaces::iterator place = (*plist)->places.begin(); place != (*plist)->places.end(); place++ ) {
      if ( !place->visible )
       continue;
      Qry.SetVariable( "x", place->x );
      Qry.SetVariable( "y", place->y );
      Qry.SetVariable( "elem_type", place->elem_type );
      if ( place->xprior == -1 )
        Qry.SetVariable( "xprior", FNull );
      else
        Qry.SetVariable( "xprior", place->xprior );
      if ( place->yprior == -1 )
        Qry.SetVariable( "yprior", FNull );
      else
        Qry.SetVariable( "yprior", place->yprior );
      Qry.SetVariable( "agle", place->agle );
      if ( place->clname.empty() || !TCompElemTypes::Instance()->isSeat( place->elem_type ) )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", place->clname );
        cl = DecodeClass( place->clname.c_str() );
        if ( cl != NoClass )
          countersClass[ cl ] = countersClass[ cl ] + 1;
      }
      Qry.SetVariable( "xname", place->xname );
      Qry.SetVariable( "yname", place->yname );
      Qry.Execute();
      if ( !place->rems.empty() ) {
        RQry.SetVariable( "x", place->x );
        RQry.SetVariable( "y", place->y );
        for( vector<TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
          RQry.SetVariable( "rem", rem->rem );
          if ( !rem->pr_denial )
            RQry.SetVariable( "pr_denial", 0 );
          else
            RQry.SetVariable( "pr_denial", 1 );
          RQry.Execute();
        }
      }
      if ( !place->layers.empty() ) {
      	QryLayers.SetVariable( "first_xname", place->xname );
      	QryLayers.SetVariable( "last_xname", place->xname );
      	QryLayers.SetVariable( "first_yname", place->yname );
      	QryLayers.SetVariable( "last_yname", place->yname );
      	for ( vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
      	  ProgTrace( TRACE5, "write layers_priority.empty()=%d, layer=%s, editable=%d",
                     layers_priority.empty(), EncodeCompLayerType( l->layer_type ), layers_priority[ l->layer_type ].editable );
      	  if ( !layers_priority[ l->layer_type ].editable )
            continue;
    		  if ( isBaseLayer( l->layer_type, readStyle ) ) {
            LQry.SetVariable( "x", place->x );
            LQry.SetVariable( "y", place->y );
            LQry.SetVariable( "layer_type", EncodeCompLayerType( l->layer_type ) );
            ProgTrace( TRACE5, "(%d,%d)=%s", place->x, place->y, EncodeCompLayerType( l->layer_type ) );
            LQry.Execute();
    		  	continue;
          }
    		  QryLayers.SetVariable( "layer_type", EncodeCompLayerType( l->layer_type ) );
    		  QryLayers.Execute();
        }
      }
      if ( place->WebTariff.value != 0.0 ) {
        QryWebTariff.SetVariable( "num", (*plist)->num );
        QryWebTariff.SetVariable( "x", place->x );
        QryWebTariff.SetVariable( "y", place->y );
        QryWebTariff.SetVariable( "color", place->WebTariff.color );
        QryWebTariff.SetVariable( "rate", place->WebTariff.value );
        QryWebTariff.SetVariable( "rate_cur", place->WebTariff.currency_id );
        QryWebTariff.Execute();
      }
    } //for place
  }
  // сохраняем конфигурацию мест
  if ( readStyle != rTripSalons ) {
    Qry.Clear();
    Qry.SQLText =
     "INSERT INTO comp_classes(comp_id,class,cfg) VALUES(:comp_id,:class,:cfg)";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.DeclareVariable( "class", otString );
    Qry.DeclareVariable( "cfg", otInteger );
    for ( map<TClass,int>::iterator i=countersClass.begin(); i!=countersClass.end(); i++ ) {
      if ( i->second > 999 )
        throw UserException( "MSG.SALONS.MATCH_PLACES" );
      Qry.SetVariable( "class", EncodeClass( i->first ) );
      Qry.SetVariable( "cfg", i->second );
      Qry.Execute();
    }
  }
  if ( readStyle == rTripSalons )
    check_waitlist_alarm( trip_id );
}

struct TPaxLayer {
	TCompLayerType layer_type;
	TDateTime time_create;
	int priority;
	int valid; // 0 - ok
	vector<TSalonPoint>	places;
	TPaxLayer( TCompLayerType vlayer_type, TDateTime vtime_create, int vpriority, TSalonPoint p ) {
		priority = vpriority;
		layer_type = vlayer_type;
		time_create = vtime_create;
		valid = 0;
		places.push_back( p );
	}
};

struct TPaxLayerRec {
	unsigned int seats;
	string cl;
	vector<TPaxLayer> paxLayers;
	TPaxLayerRec() {
  	seats = 0;
	}
  void AddLayer( TPaxLayer paxLayer ) {
    std::vector<TPaxLayer>::iterator i;
    for (i=paxLayers.begin(); i!=paxLayers.end(); i++) {
      if ( paxLayer.priority < i->priority ||
      		 ( paxLayer.priority == i->priority &&
      		   paxLayer.time_create > i->time_create ) )
      	break;
    }
   	paxLayers.insert( i, paxLayer );

  };
};

typedef map< int, TPaxLayerRec > TPaxLayers;

void SetValidPaxLayer( TPaxLayers::iterator ipax, vector<TPaxLayer>::iterator ivalid_pax_layer )
{
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // пробег по слоям пассажира
    if ( ivalid_pax_layer != ipax_layer ) {
      if ( ipax_layer->valid != -1 ) {
        ipax_layer->valid = -1;
        ProgTrace( TRACE5, "SetValidPaxLayer: pax_id=%d, layer_type=%s -- not ok!", ipax->first, EncodeCompLayerType( ipax_layer->layer_type ) );
      }
    }
  }
}

void ClearInvalidPaxLayers( TSalons *CSalon, TPaxLayers::iterator ipax )
{
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // пробег по слоям пассажира
    if ( ipax_layer->valid != -1 )
      continue;
	  for( vector<TSalonPoint>::iterator icoord=ipax_layer->places.begin(); icoord!=ipax_layer->places.end(); icoord++ ) { // пробег по местам
  		for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {  // пробег по салонам
  			if ( (*it)->num == icoord->num ) {
  				TPlace* place = (*it)->place( (*it)->GetPlaceIndex( icoord->x, icoord->y ) );
  				for ( vector<TPlaceLayer>::iterator iplace_layer=place->layers.begin(); iplace_layer!=place->layers.end(); iplace_layer++ ) { // пробег по слоям места
      	    if ( iplace_layer->layer_type == ipax_layer->layer_type &&
    	  	       iplace_layer->time_create == ipax_layer->time_create &&
                 iplace_layer->pax_id == ipax->first ) {
              place->layers.erase( iplace_layer );
              break;
            }
          }
          break;
  		  }
  		}
    }
  }
}

void GetValidPaxLayer( TSalons *CSalon, TPaxLayers &pax_layers, TPaxLayers::iterator ipax )
{
//  ProgTrace( TRACE5, "GetValidPaxLayer1: pax_id=%d", ipax->first );
  TPlace *place;
  for ( vector<TPaxLayer>::iterator ipax_layer=ipax->second.paxLayers.begin(); ipax_layer!=ipax->second.paxLayers.end(); ipax_layer++ ) { // пробег по слоям пассажира
    int vfirst_x = NoExists;
    int vfirst_y = NoExists;
    int vlast_x = NoExists;
    int vlast_y = NoExists;
  //  ProgTrace( TRACE5, "GetValidPaxLayer2: layer_type=%s", EncodeCompLayerType( ipax_layer->layer_type ) );
    if ( ipax_layer->places.size() != ipax->second.seats )
      ipax_layer->valid = -1;
    if ( ipax_layer->valid == -1 ) {
      //tst();
      continue;
    }
    for( vector<TSalonPoint>::iterator icoord=ipax_layer->places.begin(); icoord!=ipax_layer->places.end(); icoord++ ) { // пробег по местам слоя
    	for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {
   			if ( (*it)->num == icoord->num ) {
 	  			place = (*it)->place( (*it)->GetPlaceIndex( icoord->x, icoord->y ) );
          if ( ipax->second.cl != place->clname || !place->isplace || ipax_layer->places.begin()->num != place->num ) {
      	    ipax_layer->valid = -1;
          }
 		  		break;
 		    }
  	  }
  	  if ( ipax_layer->valid == -1 ) {
        //tst();
        break;
      }
    //  ProgTrace( TRACE5, "GetValidPaxLayer3: seat_no=%s", string( place->yname+place->xname).c_str() );
      for ( vector<TPlaceLayer>::iterator iplace_layer=place->layers.begin(); iplace_layer!=place->layers.end(); iplace_layer++ ) { // пробег по слоям места
      //  ProgTrace( TRACE5, "GetValidPaxLayer4: seat layer_type=%s, pax_id=%d", EncodeCompLayerType( iplace_layer->layer_type ), iplace_layer->pax_id );
    	  if ( iplace_layer->layer_type == ipax_layer->layer_type &&
    	  	   iplace_layer->time_create == ipax_layer->time_create &&
             iplace_layer->pax_id == ipax->first ) // это наш слой - и он самый приоритетный
    		  break;
     	  if ( iplace_layer->pax_id <= 0 ) { // слой более приоритетный и он не принадлежит пассажиру
          ipax_layer->valid = -1;
          break;
        }
        //есть более приоритетный слой принадлежащий другому пассажиру
        // находим этого пассажира и пробегаем по его слоям для выяснения хороших слоев
        TPaxLayers::iterator inext_pax = pax_layers.find( iplace_layer->pax_id );
        if ( inext_pax == pax_layers.end() ) {
          ProgTrace( TRACE5, "GetValidPaxLayerError, iplace_layer->pax_id=%d", iplace_layer->pax_id );
          throw EXCEPTIONS::Exception( "not found iplace_layer->pax_id" );;
        }
        GetValidPaxLayer( CSalon, pax_layers, inext_pax );
        // проверяем слой в месте салона на то, что он хороший у пассажира
        for ( vector<TPaxLayer>::iterator jpax_layer=inext_pax->second.paxLayers.begin(); jpax_layer!=inext_pax->second.paxLayers.end(); jpax_layer++ ) { // пробег по слоям пассажира
    	    if ( iplace_layer->layer_type == jpax_layer->layer_type &&
    	  	     iplace_layer->time_create == jpax_layer->time_create &&
               iplace_layer->pax_id == inext_pax->first ) {
            if ( jpax_layer->valid != -1 ) {
              ipax_layer->valid = -1;
        //      ProgTrace( TRACE5, "GetValidPaxLayer5: invalid layer_type=%s, pax_id=%d", EncodeCompLayerType( ipax_layer->layer_type ), ipax->first );
            }
    		    break;
          }
        }
        if ( ipax_layer->valid == -1 )
          break;
      }
      if ( ipax_layer->valid == -1 )
        break;

      if ( vfirst_x == NoExists || vfirst_y == NoExists ||
        vfirst_y*1000+vfirst_x > place->y*1000+place->x ) {
        vfirst_x = place->x;
        vfirst_y = place->y;
      }
      if ( vlast_x == NoExists || vlast_y == NoExists ||
        vlast_y*1000+vlast_x<place->y*1000+place->x ) {
        vlast_x=place->x;
        vlast_y=place->y;
      }
    }  // конец пробега по местам
    if ( ipax_layer->valid == -1 ) {
      continue;
    }
    if ( !( ( vfirst_x == vlast_x && vfirst_y+(int)ipax_layer->places.size()-1 == vlast_y ) ||
            ( vfirst_y == vlast_y && vfirst_x+(int)ipax_layer->places.size()-1 == vlast_x ) ) ) {
      ipax_layer->valid = -1;
      continue;
    }
    // если мы здесь, то слой хороший. надо пометить все остальные слои у пассажира как плохие
    ProgTrace( TRACE5, "GetValidPaxLayer5: ipax->pax_id=%d, layer_type=%s -- ok", ipax->first, EncodeCompLayerType( ipax_layer->layer_type ) );
    SetValidPaxLayer( ipax, ipax_layer );
    break;
  }
}


//typedef map<int,FilterLayers> TFilterLayerDests;

//необходимо научиться собирать салон по маршруту и моментально отключать салон или заданные слои в любой момент времени в любом пункте
/*void TSalons::Read( const TFilterLayerDests &filterLayer )
{
}*/

struct TPass {
  int grp_id;
  int pax_id;
  int reg_no;
  std::string surname;
  int parent_pax_id;
  int temp_parent_id;
  bool pr_inf;
  TPass() {
    grp_id = NoExists;
    pax_id = NoExists;
    reg_no = NoExists;
    parent_pax_id = NoExists;
    temp_parent_id = NoExists;
    pr_inf = false;
  }
};

struct TTransferDests {
  int point_id;
  std::vector<int> points_arv;
  std::vector<int> points_dest; // если пассажир не задан, то вектор пустой, т.к. надо учитывать только предю пункты посадки
};

/*
  1. Разметка "недоступные места" не учитываются в таблице trip_comp_ranges
  триггер:
  BEFORE INSERT OR UPDATE OR DELETE
  OF point_id, num, x, y, xname, yname, class
  ON trip_comp_elems
  2. trip_comp_ranges - делает разметку относительно point_id, нам нужна разметка point_dep, point_arv
  
  Есть две сущности:
  1. Разметка слоя в пункте посадки point_id с указанием начала и окончания действия по маршруту - trip_comp_layers
  2. Разметка слоя в пункте посадки point_id без учета маршрута - trip_comp_ranges



void TSalons::Read( const TTransferDests &transferDests )
{
  //начитка карты мест салона
  TQuery Qry( &OraSession );
	Qry.SQLText =
    "SELECT num,x,y,elem_type,xprior,yprior,agle,class,pr_smoke,not_good,xname,yname "
    " FROM trip_comp_elems WHERE point_id = :point_id "
    "ORDER BY num, x desc, y desc ";
  Qry.DeclareVariable( "point_id", otInteger, transferDests.point_id );
  Qry.Execute();
  // начитка слоев в конкретном пункте посадки - цикл по маршруту
  Qry.Clear();
  Qry.SQLText =
    "SELECT r.range_id, r.point_id, num, x, y, point_dep, point_arv, l.layer_type, "
    " first_xname, last_xname, first_yname, last_yname, crs_pax_id, pax_id, time_create "
    " FROM trip_comp_layers l, trip_comp_ranges r"
    " WHERE r.point_id = :point_id AND "
    "       r.point_id = l.point_id AND "
    "       r.range_id = l.range_id ";
  Qry.DeclareVariable( "point_id", otInteger );
}
*/

void TSalons::Read( )
{
  if ( readStyle == rTripSalons )
  	;
  else {
    FilterClass.clear();
  }
  Clear();
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );
  TQuery QryWebTariff( &OraSession );
  TQuery PaxQry( &OraSession );


  if ( readStyle == rTripSalons ) {
    Qry.SQLText =
     "SELECT pr_lat_seat, NVL(comp_id,-1) comp_id FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
    comp_id = Qry.FieldAsInteger( "comp_id" );
  }
  else {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("MSG.SALONS.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  Qry.Clear();

  if ( readStyle == rTripSalons ) {
  	PaxQry.Clear();
  	PaxQry.SQLText =
      " SELECT pax.grp_id, pax.pax_id, pax.pers_type, pax.seats, class, "
      "        reg_no, pax.surname, crs_inf.pax_id AS parent_pax_id, 1 priority "
      "    FROM pax_grp, pax, crs_inf "
      "   WHERE pax.grp_id=pax_grp.grp_id AND "
      "         pax_grp.point_dep=:point_dep AND "
      "         pax.pax_id=crs_inf.inf_id(+) AND "
      "         pax.refuse IS NULL "
      " UNION "
      " SELECT NULL, pax_id, crs_pax.pers_type, crs_pax.seats, crs_pnr.class, "
      "        NULL, crs_pax.surname, NULL, 2 priority "
      "    FROM crs_pax, crs_pnr, tlg_binding "
      "   WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "         crs_pnr.point_id=tlg_binding.point_id_tlg AND "
      "         tlg_binding.point_id_spp=:point_dep AND "
      "         crs_pnr.system='CRS' AND "
      "         crs_pax.pr_del=0 "
      " ORDER BY priority ";
    PaxQry.CreateVariable( "point_dep", otInteger, trip_id );
  	// зачитываем все слои в салоне
  	Qry.SQLText =
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.xname,t.yname,t.class,r.layer_type, "
      "                NVL(l.pax_id, l.crs_pax_id) pax_id, l.point_dep, l.point_arv, l.time_create "
      " FROM trip_comp_elems t, trip_comp_ranges r, trip_comp_layers l "
      "WHERE t.point_id=:point_id AND "
      "      t.point_id=r.point_id(+) AND "
      "      t.num=r.num(+) AND "
      "      t.x=r.x(+) AND "
      "      t.y=r.y(+) AND "
      "      r.range_id=l.range_id(+) "
      "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
  }
  else {
    Qry.SQLText =
      "SELECT num,x,y,elem_type,xprior,yprior,agle,xname,yname,class "
      " FROM comp_elems "
      "WHERE comp_id=:comp_id "
      "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
  }
  Qry.Execute();
  if ( Qry.RowCount() == 0 ) {
    if ( readStyle == rTripSalons ) {
      throw UserException( "MSG.FLIGHT_WO_CRAFT_CONFIGURE" );
    }
    else {
      throw UserException( "MSG.SALONS.NOT_FOUND" );
    }
  }
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_elem_type = Qry.FieldIndex( "elem_type" );
  int col_xprior = Qry.FieldIndex( "xprior" );
  int col_yprior = Qry.FieldIndex( "yprior" );
  int col_agle = Qry.FieldIndex( "agle" );
  int col_xname = Qry.FieldIndex( "xname" );
  int col_yname = Qry.FieldIndex( "yname" );
  int col_class = Qry.FieldIndex( "class" );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    RQry.CreateVariable( "point_id", otInteger, trip_id );
    LQry.SQLText =
      "SELECT num,x,y,layer_type FROM trip_comp_baselayers "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    LQry.CreateVariable( "point_id", otInteger, trip_id );

    
    if ( FilterLayers.CanUseLayer( cltProtBeforePay, -1 ) ||
         FilterLayers.CanUseLayer( cltProtAfterPay, -1 ) ||
         FilterLayers.CanUseLayer( cltPNLBeforePay, -1 ) ||
         FilterLayers.CanUseLayer( cltPNLAfterPay, -1 ) ) {
      QryWebTariff.SQLText =
        "SELECT num,x,y,color,rate,rate_cur FROM trip_comp_rates "
        " WHERE point_id=:point_id "
        "ORDER BY num,x desc, y desc ";
      QryWebTariff.CreateVariable( "point_id", otInteger, trip_id );
    }
  }
  else {
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM comp_rem "
                   " WHERE comp_id=:comp_id "
                   "ORDER BY num, x desc, y desc ";
    RQry.CreateVariable( "comp_id", otInteger, comp_id );
    LQry.SQLText = "SELECT num,x,y,layer_type FROM comp_baselayers "
                   " WHERE comp_id=:comp_id "
                   "ORDER BY num, x desc, y desc ";
    LQry.CreateVariable( "comp_id", otInteger, comp_id );
    QryWebTariff.SQLText =
      "SELECT num,x,y,color,rate,rate_cur FROM comp_rates "
      " WHERE comp_id=:comp_id "
      "ORDER BY num,x desc, y desc ";
    QryWebTariff.CreateVariable( "comp_id", otInteger, comp_id );
  }
  RQry.Execute();
  int rem_col_num = RQry.FieldIndex( "num" );
  int rem_col_x = RQry.FieldIndex( "x" );
  int rem_col_y = RQry.FieldIndex( "y" );
  int rem_col_rem = RQry.FieldIndex( "rem" );
  int rem_col_pr_denial = RQry.FieldIndex( "pr_denial" );
  
  LQry.Execute();
  int baselayer_col_num = LQry.FieldIndex( "num" );
  int baselayer_col_x = LQry.FieldIndex( "x" );
  int baselayer_col_y = LQry.FieldIndex( "y" );
  int baselayer_col_layer_type = LQry.FieldIndex( "layer_type" );

  
  int webtariff_col_num;
  int webtariff_col_x;
  int webtariff_col_y;
  int webtariff_col_color;
  int webtariff_col_rate;
  int webtariff_col_rate_cur;

  if ( readStyle != rTripSalons ||
       FilterLayers.CanUseLayer( cltProtBeforePay, -1 ) ||
       FilterLayers.CanUseLayer( cltProtAfterPay, -1 ) ||
       FilterLayers.CanUseLayer( cltPNLBeforePay, -1 ) ||
       FilterLayers.CanUseLayer( cltPNLAfterPay, -1 ) ) {
    QryWebTariff.Execute();
    webtariff_col_num = QryWebTariff.FieldIndex( "num" );
    webtariff_col_x = QryWebTariff.FieldIndex( "x" );
    webtariff_col_y = QryWebTariff.FieldIndex( "y" );
    webtariff_col_color = QryWebTariff.FieldIndex( "color" );
    webtariff_col_rate = QryWebTariff.FieldIndex( "rate" );
    webtariff_col_rate_cur = QryWebTariff.FieldIndex( "rate_cur" );
  }
  string ClName = ""; /* перечисление всех классов, которые есть в салоне */
  TPlaceList *placeList = NULL;
  int num = -1;
  TPoint point_p;
  int pax_id;
  TPlaceLayer PlaceLayer( 0, 0, 0, cltUnknown, ASTRA::NoExists, 10000 );
  TPaxLayers pax_layers;
  //PaxsOnPlaces.clear();
  
  vector<TPass> InfItems, AdultItems;
  if ( readStyle == rTripSalons ) { // заполняем инфу по пассажиру
  	PaxQry.Execute();
    int idx_pax_id = PaxQry.FieldIndex( "pax_id" );
    int idx_grp_id = PaxQry.FieldIndex( "grp_id" );
    int idx_parent_pax_id = PaxQry.FieldIndex( "parent_pax_id" );
    int idx_reg_no = PaxQry.FieldIndex( "reg_no" );
    int idx_seats = PaxQry.FieldIndex( "seats" );
    int idx_pers_type = PaxQry.FieldIndex( "pers_type" );
    int idx_surname = PaxQry.FieldIndex( "surname" );
    int idx_class = PaxQry.FieldIndex( "class" );
    int idx_priority = PaxQry.FieldIndex( "priority" );
    while ( !PaxQry.Eof ) {
      if ( PaxQry.FieldAsInteger( idx_priority ) == 1 ) {
        TPass pass;
        pass.grp_id = PaxQry.FieldAsInteger( idx_grp_id );
        pass.pax_id = PaxQry.FieldAsInteger( idx_pax_id );
        pass.reg_no = PaxQry.FieldAsInteger( idx_reg_no );
        pass.surname = PaxQry.FieldAsString( idx_surname );
        pass.parent_pax_id = PaxQry.FieldAsInteger( idx_parent_pax_id );
        if ( PaxQry.FieldAsInteger( idx_seats ) == 0 ) {
          InfItems.push_back( pass );
        }
        else {
          ProgTrace( TRACE5, "PaxQry.FieldAsString( idx_pers_type )=%s", PaxQry.FieldAsString( idx_pers_type ) );
          if ( string(PaxQry.FieldAsString( idx_pers_type )) == string("ВЗ") ) {
            tst();
            AdultItems.push_back( pass );
          }
        }
      }
    	if ( PaxQry.FieldAsInteger( idx_seats ) >= 1 &&
           pax_layers.find( PaxQry.FieldAsInteger( idx_pax_id ) ) == pax_layers.end() ) {
    	  pax_layers[ PaxQry.FieldAsInteger( idx_pax_id ) ].seats = PaxQry.FieldAsInteger( idx_seats );
    	  pax_layers[ PaxQry.FieldAsInteger( idx_pax_id ) ].cl = PaxQry.FieldAsString( idx_class );
    	}
    	PaxQry.Next();
    }
    //привязали детей к взрослым
    ProgTrace( TRACE5, "SetInfantsToAdults: InfItems.size()=%zu, AdultItems.size()=%zu", InfItems.size(), AdultItems.size() );
    SetInfantsToAdults<TPass,TPass>( InfItems, AdultItems );
    for ( vector<TPass>::iterator i=InfItems.begin(); i!=InfItems.end(); i++ ) {
      ProgTrace( TRACE5, "Infant pax_id=%d", i->pax_id );
      for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
        if ( i->parent_pax_id == j->pax_id ) {
          j->pr_inf = true;
          ProgTrace( TRACE5, "Infant to pax_id=%d", j->pax_id );
          break;
        }
      }
    }
  }

  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( num != Qry.FieldAsInteger( col_num ) ) {
      if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
        placeList->places.clear();
      }
      else {
        placeList = new TPlaceList();
        placelists.push_back( placeList );
      }
      ClName.clear();
      num = Qry.FieldAsInteger( col_num );
      placeList->num = num;
    }
    // повторение мест! - разные слои
    TPlace place;
    point_p.x = Qry.FieldAsInteger( col_x );
    point_p.y = Qry.FieldAsInteger( col_y );
    if ( readStyle != rTripSalons || Qry.FieldIsNULL( "pax_id" ) )
    	pax_id = -1;
    else
    	pax_id = Qry.FieldAsInteger( "pax_id" );
    // если место еще не определено или место есть, но не проинициализировано
    if ( !placeList->ValidPlace( point_p ) || placeList->place( point_p )->x == -1 ) {
    	place.x = point_p.x;
    	place.y = point_p.y;
    	place.num = num;
      place.elem_type = Qry.FieldAsString( col_elem_type );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( Qry.FieldIsNULL( col_xprior ) )
        place.xprior = -1;
      else
        place.xprior = Qry.FieldAsInteger( col_xprior );
      if ( Qry.FieldIsNULL( col_yprior ) )
        place.yprior = -1;
      else
        place.yprior = Qry.FieldAsInteger( col_yprior );
      place.agle = Qry.FieldAsInteger( col_agle );
      place.clname = Qry.FieldAsString( col_class );
      place.xname = Qry.FieldAsString( col_xname );
      place.yname = Qry.FieldAsString( col_yname );
      while ( !RQry.Eof && RQry.FieldAsInteger( rem_col_num ) == num &&
              RQry.FieldAsInteger( rem_col_x ) == place.x &&
              RQry.FieldAsInteger( rem_col_y ) == place.y ) {
        TRem rem;
        rem.rem = RQry.FieldAsString( rem_col_rem );
        rem.pr_denial = RQry.FieldAsInteger( rem_col_pr_denial );
        place.rems.push_back( rem );
        RQry.Next();
      }
      while ( !LQry.Eof && LQry.FieldAsInteger( baselayer_col_num ) == num &&
              LQry.FieldAsInteger( baselayer_col_x ) == place.x &&
              LQry.FieldAsInteger( baselayer_col_y ) == place.y ) {
      	place.AddLayerToPlace( DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) ),
                               0, 0, NoExists, NoExists,
                               layers_priority[ DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) ) ].priority );
        LQry.Next();
      }
      if ( readStyle != rTripSalons ||
           FilterLayers.CanUseLayer( cltProtBeforePay, -1 ) ||
           FilterLayers.CanUseLayer( cltProtAfterPay, -1 ) ||
           FilterLayers.CanUseLayer( cltPNLBeforePay, -1 ) ||
           FilterLayers.CanUseLayer( cltPNLAfterPay, -1 ) ) {
        if ( !QryWebTariff.Eof &&
        	    QryWebTariff.FieldAsInteger( webtariff_col_num ) == num &&
              QryWebTariff.FieldAsInteger( webtariff_col_x ) == place.x &&
              QryWebTariff.FieldAsInteger( webtariff_col_y ) == place.y ) {
          place.WebTariff.color = QryWebTariff.FieldAsString( webtariff_col_color );
          place.WebTariff.value = QryWebTariff.FieldAsFloat( webtariff_col_rate );
          place.WebTariff.currency_id = QryWebTariff.FieldAsString( webtariff_col_rate_cur );
          QryWebTariff.Next();
        }
      }
      if ( ClName.find( Qry.FieldAsString( col_class ) ) == string::npos )
        ClName += Qry.FieldAsString(col_class );
    }
    else { // это место проинициализировано - это новый слой
    	place = *placeList->place( point_p );
      if ( place.x != point_p.x || place.y != point_p.y )
        throw EXCEPTIONS::Exception( "invalid x, y" );
    }
    PlaceLayer.pax_id = -1;
    if ( readStyle == rTripSalons ) { // здесь работа со всеми слоями для удаления менее приоритетных слоев по пассажирам
   		PlaceLayer.layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
   		if ( Qry.FieldIsNULL( "point_dep" ) )
   			PlaceLayer.point_dep = NoExists;
   		else
   		  PlaceLayer.point_dep = Qry.FieldAsInteger( "point_dep" );
   		if ( Qry.FieldIsNULL( "point_arv" ) )
   			PlaceLayer.point_arv = NoExists;
   		else
   		  PlaceLayer.point_arv = Qry.FieldAsInteger( "point_arv" );
   		PlaceLayer.time_create = Qry.FieldAsDateTime( "time_create" );
      if ( FilterLayers.CanUseLayer( PlaceLayer.layer_type, Qry.FieldAsInteger( "point_dep" ) ) ) { // этот слой используем
//      	ProgTrace( TRACE5, "seat_no=%s, pax_id=%d", string(string(Qry.FieldAsString("yname"))+Qry.FieldAsString("xname")).c_str(), pax_id );
      	if ( PlaceLayer.layer_type != cltUnknown ) { // слои сортированы по приоритету, первый - самый приоритетный слой в векторе
          place.AddLayerToPlace( PlaceLayer.layer_type, PlaceLayer.time_create, pax_id,
                                 PlaceLayer.point_dep, PlaceLayer.point_arv, layers_priority[ PlaceLayer.layer_type ].priority ); // может быть повторение слоев
          PlaceLayer.pax_id = pax_id;
        } // задан слой у места
      }
    }
    place.visible = true;
    placeList->Add( place );
    if ( PlaceLayer.pax_id > 0 ) {
      TPaxLayers::iterator ipl = pax_layers.find( PlaceLayer.pax_id );
      if ( ipl != pax_layers.end() ) { // нашли вектор слоев закрепленных за pax_id
       	vector<TPaxLayer>::iterator ip=ipl->second.paxLayers.end();
        for( ip=ipl->second.paxLayers.begin(); ip!=ipl->second.paxLayers.end(); ip++ ) {
         	if ( ip->layer_type == PlaceLayer.layer_type && ip->time_create == PlaceLayer.time_create )
         		break;
        }
        if ( ip != ipl->second.paxLayers.end() ) { // нашли слой, еще одно место у человека в этом слою
         	ip->places.push_back( TSalonPoint( point_p.x, point_p.y, placeList->num ) );
        }
        else {
        	ipl->second.AddLayer( TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
         	                                 layers_priority[ PlaceLayer.layer_type ].priority,
         	                                 TSalonPoint( point_p.x, point_p.y, placeList->num ) )); // слой не найден, создаем новый слой
         }
      }
      else { // пассажир не найден, создаем пассажира и слой
       	pax_layers[ pax_id ].AddLayer( TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
       	                                          layers_priority[ PlaceLayer.layer_type ].priority,
       	                                          TSalonPoint( point_p.x, point_p.y, placeList->num ) ));  //??? а надо ли djek
      }
    }
  }	/* end for */
  if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
  	ProgTrace( TRACE5, "placeList->num=%d", placeList->num );
    placelists.pop_back( );
    delete placeList; // нам этот класс/салон не нужен
  }
  
  if ( drop_not_used_pax_layers ) {
    for( TPaxLayers::iterator ipax=pax_layers.begin(); ipax!=pax_layers.end(); ipax++ ) { // пробег по пассажирам
      GetValidPaxLayer( this, pax_layers, ipax );
    }
    for( TPaxLayers::iterator ipax=pax_layers.begin(); ipax!=pax_layers.end(); ipax++ ) { // пробег по пассажирам
      ClearInvalidPaxLayers( this, ipax );
    }
  }
  
  if ( readStyle == rTripSalons ) { // пометить пассажиров с детьми
    for( vector<TPlaceList*>::iterator placeList = placelists.begin();
      placeList != placelists.end(); placeList++ ) {
      for ( TPlaces::iterator place = (*placeList)->places.begin();
            place != (*placeList)->places.end(); place++ ) {
        if ( !place->visible || !place->isplace ||
             place->layers.empty() || place->layers.begin()->pax_id == NoExists )
         continue;
        for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
          if ( !j->pr_inf || place->layers.begin()->pax_id != j->pax_id )
            continue;
          ProgTrace( TRACE5, "TDrawProp: seat_no=%s", string(place->yname+place->xname).c_str() );
          place->drawProps.push_back( dpInfantWoSeats );
          break;
        }
      }
    }
  }
}

void TSalons::Parse( xmlNodePtr salonsNode )
{
  if ( salonsNode == NULL )
    return;
  xmlNodePtr node;
  bool pr_lat_seat_init=false;
  node = GetNode( "@pr_lat_seat", salonsNode );
  if ( node ) {
  	tst();
  	pr_lat_seat = NodeAsInteger( node );
  	pr_lat_seat_init=true;
  }
  Clear();
  node = salonsNode->children;
  xmlNodePtr salonNode = NodeAsNodeFast( "placelist", node );
  TRem rem;
  int lat_count=0, rus_count=0;
  string rus_lines = rus_seat, lat_lines = lat_seat;
  TElemFmt fmt;
  while ( salonNode ) {
    TPlaceList *placeList = new TPlaceList();
    placeList->num = NodeAsInteger( "@num", salonNode );
    xmlNodePtr placeNode = salonNode->children;
    while ( placeNode ) {
      node = placeNode->children;
      TPlace place;
      place.x = NodeAsIntegerFast( "x", node );
      place.y = NodeAsIntegerFast( "y", node );
      place.elem_type = NodeAsStringFast( "elem_type", node );
      place.isplace = TCompElemTypes::Instance()->isSeat( place.elem_type );
      if ( !GetNodeFast( "xprior", node ) )
        place.xprior = -1;
      else
        place.xprior = NodeAsIntegerFast( "xprior", node );
      if ( !GetNodeFast( "yprior", node ) )
        place.yprior = -1;
      else
        place.yprior = NodeAsIntegerFast( "yprior", node );
      if ( !GetNodeFast( "agle", node ) )
        place.agle = 0;
      else
        place.agle = NodeAsIntegerFast( "agle", node );
      place.clname = NodeAsStringFast( "class", node );
      if ( !place.clname.empty() ) {
        place.clname = ElemToElemId( etClass, place.clname, fmt );
        if ( fmt == efmtUnknown )
      	  throw UserException( "MSG.INVALID_CLASS" );
      }

      place.xname = NodeAsStringFast( "xname", node );

      if ( !pr_lat_seat_init ) {
      	if ( rus_lines.find( place.xname ) != string::npos ) {
          rus_count++;
        }
    	  if ( lat_lines.find( place.xname ) != string::npos ) {
          lat_count++;
        }
    	}
      place.xname = norm_iata_line( place.xname );
      place.yname = norm_iata_row( NodeAsStringFast( "yname", node ) );

      xmlNodePtr remsNode = GetNodeFast( "rems", node );
      xmlNodePtr remNode;
      bool pr_disable_layer = false;
      if ( remsNode ) {
      	remsNode = remsNode->children;
      	while ( remsNode ) {
      	  remNode = remsNode->children;
      	  rem.rem = NodeAsStringFast( "rem", remNode );
      	  rem.pr_denial = GetNodeFast( "pr_denial", remNode );
      	  if ( rem.rem == "X" ) {
            if ( !pr_disable_layer && !compatibleLayer( cltDisable ) && !rem.pr_denial ) {
              pr_disable_layer = true;
            }
          }
      	  else
      	    place.rems.push_back( rem );
      	  remsNode = remsNode->next;
        }
      }
      remsNode = GetNodeFast( "layers", node );
      if ( remsNode ) {
      	remsNode = remsNode->children; //layer
      	while( remsNode ) {
      		remNode = remsNode->children;
      		TCompLayerType l = DecodeCompLayerType( NodeAsStringFast( "layer_type", remNode ) );
      		if ( l != cltUnknown && !place.isLayer( l ) )
      			 place.AddLayerToPlace( l, 0, 0, NoExists, NoExists, layers_priority[ l ].priority );
      		remsNode = remsNode->next;
      	}
      }
      if ( !compatibleLayer( cltDisable ) && pr_disable_layer ) {
        place.AddLayerToPlace( cltDisable, 0, 0, NoExists, NoExists, layers_priority[ cltDisable ].priority );
      }
      remNode = GetNodeFast( "tarif", node );
      if ( remNode ) {
      	place.WebTariff.color = NodeAsString( "@color", remNode );
      	place.WebTariff.value = NodeAsFloat( remNode );
      	place.WebTariff.currency_id = NodeAsString( "@currency_id", remNode );
      }
      place.visible = true;
      placeList->Add( place );
      placeNode = placeNode->next;
    }
    placelists.push_back( placeList );
    salonNode = salonNode->next;
  }
  if ( !pr_lat_seat_init ) {
  	pr_lat_seat = ( lat_count>=rus_count );
  }
}

void TSalons::verifyValidRem( std::string rem_name, std::string class_name )
{
  for( vector<TPlaceList*>::iterator placeList = placelists.begin();
    placeList != placelists.end(); placeList++ ) {
    for ( TPlaces::iterator place = (*placeList)->places.begin();
          place != (*placeList)->places.end(); place++ ) {
      if ( !place->visible || place->clname == class_name )
       continue;
      for ( vector<TRem>::iterator irem=place->rems.begin(); irem!=place->rems.end(); irem++ ) {
      	if ( irem->rem == rem_name )
      		throw UserException( "MSG.SALONS.REMARK_NOT_SET_IN_CLASS",
      		                     LParams()<<LParam("remark", rem_name )<<LParam("class", ElemIdToCodeNative(etClass,place->clname) ));
      }
    }
  }
}

int TPlaceList::GetXsCount()
{
  return xs.size();
}

int TPlaceList::GetYsCount()
{
  return ys.size();
}

TPlace *TPlaceList::place( int idx )
{
  if ( idx < 0 || idx >= (int)places.size() )
    throw EXCEPTIONS::Exception( "place index out of range" );
  return &places[ idx ];
}

TPlace *TPlaceList::place( TPoint &p )
{
  return place( GetPlaceIndex( p ) );
}

int TPlaceList::GetPlaceIndex( TPoint &p )
{
  return GetPlaceIndex( p.x, p.y );
}

int TPlaceList::GetPlaceIndex( int x, int y )
{
  return GetXsCount()*y + x;
}

bool TPlaceList::ValidPlace( TPoint &p )
{
 return ( p.x < GetXsCount() && p.x >= 0 && p.y < GetYsCount() && p.y >= 0 );
}

string TPlaceList::GetPlaceName( TPoint &p )
{
  if ( !ValidPlace( p ) )
    throw EXCEPTIONS::Exception( "Неправильные координаты места" );
  return ys[ p.y ] + xs[ p.x ];
}

string TPlaceList::GetXsName( int x )
{
  if ( x < 0 || x >= GetXsCount() ) {
    throw EXCEPTIONS::Exception( "Неправильные x координата места" );
  }
  return xs[ x ];
}

string TPlaceList::GetYsName( int y )
{
  if ( y < 0 || y >= GetYsCount() )
    throw EXCEPTIONS::Exception( "Неправильные y координата места" );
  return ys[ y ];
}

bool TPlaceList::GetisPlaceXY( string placeName, TPoint &p )
{
	TrimString(placeName);
	if ( placeName.empty() )
		return false;
  /* конвертация номеров мест в зависимости от лат. или рус. салона */
  size_t i = 0;
  for (; i < placeName.size(); i++)
    if ( placeName[ i ] != '0' )
      break;
  if ( i )
    placeName.erase( 0, i );
  string seat_no = placeName, salon_seat_no;
  if ( placeName == CharReplace( seat_no, rus_seat, lat_seat ) )
    CharReplace( seat_no, lat_seat, rus_seat );
  if ( placeName == seat_no )
    seat_no.clear();
  for( vector<string>::iterator ix=xs.begin(); ix!=xs.end(); ix++ )
    for ( vector<string>::iterator iy=ys.begin(); iy!=ys.end(); iy++ ) {
    	salon_seat_no = denorm_iata_row(*iy,NULL) + denorm_iata_line(*ix,false);
      if ( placeName == salon_seat_no ||
      	   ( !seat_no.empty() && seat_no == salon_seat_no ) ) {
      	p.x = distance( xs.begin(), ix );
      	p.y = distance( ys.begin(), iy );
      	return place( p )->isplace;
      }
    }
  return false;
}

void TPlaceList::Add( TPlace &pl )
{
//  ProgTrace( TRACE5, "TPlaceList::add pl(%d,%d)", pl.x, pl.y );
  int prior_max_x = (int)xs.size();
  int prior_max_y = (int)ys.size();
  if ( pl.x >= prior_max_x )
    xs.resize( pl.x + 1, "" );
  if ( !pl.xname.empty() )
    xs[ pl.x ] = pl.xname;
  if ( pl.y >= prior_max_y )
    ys.resize( pl.y + 1, "" );
  if ( !pl.yname.empty() )
    ys[ pl.y ] = pl.yname;
  if ( (int)xs.size()*(int)ys.size() > (int)places.size() ) {
    //places.resize( (int)xs.size()*(int)ys.size() );
    //нужен сдвиг!!!
//    ProgTrace( TRACE5, "TPlaceList::prior_max_x=%d, prior_max_y=%d, new_size=%d, old_size=%d",
//               prior_max_x, prior_max_y, (int)xs.size()*(int)ys.size(), (int)places.size() );
    for ( int iy=0; iy<prior_max_y-1; iy++ ) {
//        ProgTrace( TRACE5, "TPlaceList::insert iy=%d", iy );
        IPlace ip = places.begin() + GetPlaceIndex( prior_max_x - 1, iy );
        TPlace p;
//        ProgTrace( TRACE5, "TPlaceList:: ip(%d,%d) visible=%d, name=%s, idx=%d, count=%d",
//                   ip->x, ip->y, ip->visible, string(ip->xname+ip->yname).c_str(),
//                   GetPlaceIndex( prior_max_x - 1, prior_max_y - 1 ),
//                   (int)xs.size() - prior_max_x );
        if ( (int)xs.size() > prior_max_x )
          places.insert( ip + 1, (int)xs.size() - prior_max_x, p );
    }
  }
  if ( (int)xs.size()*(int)ys.size() > (int)places.size() ) {
    places.resize( (int)xs.size()*(int)ys.size() );
  }

  int idx = GetPlaceIndex( pl.x, pl.y );
  if ( pl.xprior >= 0 && pl.yprior >= 0 ) {
    TPoint p( pl.xprior, pl.yprior );
    place( p )->xnext = pl.x;
    place( p )->ynext = pl.y;
  }
  //ProgTrace( TRACE5, "TPlaceList::Add: pl(%d,%d) visible=%d, idx=%d", pl.x, pl.y, pl.visible, idx );
  places[ idx ] = pl;
}

bool Checkin( int pax_id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText = "SELECT pax_id FROM pax where pax_id=:pax_id";
	Qry.CreateVariable( "pax_id", otInteger, pax_id );
	Qry.Execute();
	return Qry.RowCount();
}

void GetTripParams( int trip_id, xmlNodePtr dataNode )
{
//  ProgTrace( TRACE5, "GetTripParams trip_id=%d", trip_id );

  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airp,airp_fmt,airline,airline_fmt,flt_no,suffix,suffix_fmt,craft,craft_fmt,bort,scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out, pr_del "
    "FROM points "
    "WHERE point_id=:point_id ";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TTripInfo info( Qry );

  NewTextChild( dataNode, "trip", GetTripName( info, ecCkin ) );
  NewTextChild( dataNode, "craft", ElemIdToElemCtxt( ecDisp, etCraft, Qry.FieldAsString( "craft" ), (TElemFmt)Qry.FieldAsInteger( "craft_fmt" ) ) );
  NewTextChild( dataNode, "bort", Qry.FieldAsString( "bort" ) );

  Qry.Clear();
  Qry.SQLText = "SELECT "\
                "       DECODE( trip_sets.comp_id, NULL, DECODE( e.pr_comp_id, 0, -2, -1 ), "\
                "               trip_sets.comp_id ) comp_id, "\
                "       comp.descr "\
                " FROM trip_sets, "\
                "  ( SELECT COUNT(*) pr_comp_id FROM trip_comp_elems "\
                "    WHERE point_id=:point_id AND rownum<2 ) e, "\
                "  ( SELECT comp_id, craft, bort, descr FROM comps ) comp "\
                " WHERE trip_sets.point_id = :point_id AND trip_sets.comp_id = comp.comp_id(+) ";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  /* comp_id>0 - базовый; comp_id=-1 - измененный; comp_id=-2 - не задан */
  NewTextChild( dataNode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
  NewTextChild( dataNode, "descr", Qry.FieldAsString( "descr" ) );
}

void GetCompParams( int comp_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT comp_id,craft,bort,descr from comps "
                " WHERE comp_id=:comp_id";
  Qry.DeclareVariable( "comp_id", otInteger );
  Qry.SetVariable( "comp_id", comp_id );
  Qry.Execute();
  NewTextChild( dataNode, "trip" );
  NewTextChild( dataNode, "craft", ElemIdToCodeNative( etCraft, Qry.FieldAsString( "craft" ) ) );
  NewTextChild( dataNode, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( dataNode, "comp_id", comp_id );
  NewTextChild( dataNode, "descr", Qry.FieldAsString( "descr" ) );
}


bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull )
{
  TQuery Qry( &OraSession );
  string sql = "SELECT pax.pax_id FROM pax_grp, pax "\
               " WHERE pax_grp.grp_id=pax.grp_id AND "\
               "       point_dep=:point_id AND "\
               "       pax.pr_brd IS NOT NULL AND "\
               "       seats > 0 AND rownum <= 1 ";
 if ( SeatNoIsNull ) {
  sql += " AND salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) IS NULL";
 }
 Qry.SQLText = sql;
 Qry.CreateVariable( "point_id", otInteger, trip_id );
 Qry.Execute( );
 return Qry.RowCount();
}

int SIGN( int a ) {
	return (a > 0) - (a < 0);
};

struct TComp {
  int sum;
  int comp_id;
  TComp() {
  	sum = 99999;
   	comp_id = -1;
  };
};

int GetCompId( const std::string craft, const std::string bort, const std::string airline,
               vector<std::string> airps,  int f, int c, int y )
{
	ProgTrace( TRACE5, "craft=%s, bort=%s, airline=%s, f=%d, c=%d, y=%d, airps.size=%zu",
	           craft.c_str(), bort.c_str(), airline.c_str(), f, c, y, airps.size() );
	if ( f + c + y == 0 )
		return -1;
	map<int,TComp,std::less<int> > CompMap;
	int idx;
	TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT * FROM "
	  "( SELECT COMPS.COMP_ID, BORT, AIRLINE, AIRP, "
    "         NVL( SUM( DECODE( CLASS, 'П', CFG, 0 )), 0 ) AS F, "
    "         NVL( SUM( DECODE( CLASS, 'Б', CFG, 0 )), 0 ) AS C, "
    "         NVL( SUM( DECODE( CLASS, 'Э', cfg, 0 )), 0 ) AS Y "
    "   FROM COMPS, COMP_CLASSES "
    "  WHERE COMP_CLASSES.COMP_ID = COMPS.COMP_ID AND COMPS.CRAFT = :craft "
    "  GROUP BY COMPS.COMP_ID, BORT, AIRLINE, AIRP ) "
    "WHERE  f - :vf >= 0 AND "
    "       c - :vc >= 0 AND "
    "       y - :vy >= 0 AND "
    "       f < 1000 AND c < 1000 AND y < 1000 "
    " ORDER BY comp_id ";
	Qry.CreateVariable( "craft", otString, craft );
	Qry.CreateVariable( "vf", otString, f );
	Qry.CreateVariable( "vc", otString, c );
	Qry.CreateVariable( "vy", otString, y );
	Qry.Execute();
	ProgTrace( TRACE5, "bort=%s, airline=%s", bort.c_str(), airline.c_str() );
  while ( !Qry.Eof ) {
  	string comp_airline = Qry.FieldAsString( "airline" );
  	string comp_airp = Qry.FieldAsString( "airp" );
  	bool airline_OR_airp = ( !comp_airline.empty() && airline == comp_airline ) ||
    	                     ( comp_airline.empty() &&
                             !comp_airp.empty() &&
                             find( airps.begin(), airps.end(), comp_airp ) != airps.end() );
    if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) && airline_OR_airp )
    	idx = 0; // когда совпадает борт+авиакомпания OR аэропорт
    else
    	if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) )
    		idx = 1; // когда совпадает борт
    	else
    		if ( airline_OR_airp )
    			idx = 2; // когда совпадает авиакомпания или аэропорт
    		else {
    			Qry.Next();
    			continue;
    		}
    // совпадение по кол-ву мест для каждого класса
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         Qry.FieldAsInteger( "f" ) >= f &&
         Qry.FieldAsInteger( "c" ) >= c &&
         Qry.FieldAsInteger( "y" ) >= y &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }

    idx += 3;
    // совпадение по классам и количеству мест >= общее кол-во мест
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    // совпадение по количеству мест >= общее кол-во мест
    idx += 3;
    if ( CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
  	Qry.Next();
  }
// ProgTrace( TRACE5, "CompMap.size()=%zu", CompMap.size() );
 if ( !CompMap.size() )
 	return -1;
 else
 	return CompMap.begin()->second.comp_id; // минимальный элемент - сортировка ключа по позрастанию
}

void setTRIP_CLASSES( int point_id )
{
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "BEGIN "
    "DELETE trip_classes WHERE point_id = :point_id; "
    "INSERT INTO trip_classes(point_id,class,cfg,block,prot) "
    "  SELECT  :point_id, "
    "          b.class, "
    "          NVL(b.cfg,0), "
    "          NVL(block,0), "
    "          NVL(prot,0) "
    "  FROM "
    " ( SELECT class, "
    "          NVL(SUM(DECODE( layer_type, :blockcent_layer, 1, :disable_layer, 1, 0 )),0) block, "
    "          NVL(SUM(DECODE( layer_type, :reserve_layer, 1, 0 )),0) prot "
    "    FROM trip_comp_ranges r, trip_comp_elems t, comp_elem_types "
    "   WHERE r.point_id=:point_id AND "
    "         t.point_id=r.point_id AND "
    "         t.num=r.num AND "
    "         t.x=r.x AND "
    "         t.y=r.y AND "
    "         t.elem_type = comp_elem_types.code AND "
    "         comp_elem_types.pr_seat <> 0 "
    "   GROUP BY class "
    "  ) a, "
    "  ( SELECT trip_comp_elems.class, "
    "           NVL( SUM( DECODE( trip_comp_elems.class, NULL, 0, 1 ) ), 0 ) cfg "
    "     FROM trip_comp_elems, comp_elem_types "
    "    WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "          comp_elem_types.pr_seat <> 0 AND "
    "          trip_comp_elems.point_id=:point_id "
    "    GROUP BY class "
    "  ) b "
    " WHERE b.class=a.class(+); "
    " ckin.recount( :point_id ); "
    "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "blockcent_layer", otString, EncodeCompLayerType(ASTRA::cltBlockCent) );
  Qry.CreateVariable( "disable_layer", otString, EncodeCompLayerType(ASTRA::cltDisable) );
  Qry.CreateVariable( "reserve_layer", otString, EncodeCompLayerType(ASTRA::cltProtect) );
  Qry.Execute();
}

void CreateComps( const TCompsRoutes &routes, int comp_id )
{
  TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  int pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

  TQuery QryTripSets(&OraSession);
  QryTripSets.SQLText =
    "UPDATE trip_sets SET comp_id=:comp_id, pr_lat_seat=:pr_lat_seat,crc_comp=:crc_comp WHERE point_id=:point_id";
  QryTripSets.CreateVariable( "comp_id", otInteger, comp_id );
  QryTripSets.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
  QryTripSets.DeclareVariable( "point_id", otInteger );
  QryTripSets.DeclareVariable( "crc_comp", otInteger );
	Qry.Clear();
	Qry.SQLText =
	  "BEGIN "
	  "DELETE trip_comp_rates WHERE point_id = :point_id;"
	  "DELETE trip_comp_rem WHERE point_id = :point_id; "
	  "DELETE trip_comp_baselayers WHERE point_id = :point_id; "
    "DELETE trip_comp_elems WHERE point_id = :point_id; "
    "DELETE trip_comp_layers "
    " WHERE point_id=:point_id AND layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
    " SELECT :point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname "
    "  FROM comp_elems "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,rem,pr_denial "
    "  FROM comp_rem "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
    " SELECT :point_id,num,x,y,layer_type "
    "  FROM comp_baselayers "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
    " SELECT :point_id,num,x,y,color,rate,rate_cur FROM comp_rates "
    " WHERE comp_id = :comp_id; "
    "END;";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.DeclareVariable( "point_id", otInteger );
  int crc_comp = 0;
  for (TCompsRoutes::const_iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( i->inRoutes && i->auto_comp_chg && i->pr_reg ) {
      Qry.SetVariable( "point_id", i->point_id );
      Qry.Execute();
      if ( crc_comp == 0 ) {
        crc_comp = CRC32_Comp( i->point_id );
      }
      InitVIP( i->point_id );
      setTRIP_CLASSES( i->point_id );
      QryTripSets.SetVariable( "point_id", i->point_id );
      QryTripSets.SetVariable( "crc_comp", crc_comp );
      QryTripSets.Execute();
      TReqInfo::Instance()->MsgToLog( string( "Назначена базовая компоновка (ид=" ) + IntToString( comp_id ) +
      	                              "). Классы: " + GetCfgStr(NoExists, i->point_id, AstraLocale::LANG_RU), evtFlt, i->point_id );
      check_waitlist_alarm( i->point_id );
    }
  }
}

bool CompRouteinRoutes( const CompRoute &item1, const CompRoute &item2 )
{
  return ( (item1.craft == item2.craft || item2.craft.empty()) &&
           (item1.bort == item2.bort || item2.bort.empty()) &&
           item1.airline == item2.airline );
}

void push_routes( const CompRoute &currroute,
                  const TTripRoute &routes,
                  bool pr_before,
                  TCompsRoutes &comps_routes )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT airline,airp,bort,craft,pr_reg FROM points WHERE point_id=:point_id AND pr_del!=-1";
  Qry.DeclareVariable( "point_id", otInteger );
  for ( vector<TTripRouteItem>::const_iterator i=routes.begin(); i!=routes.end(); i++ ) {
    //выделяем маршрут удовлетворяющий след. условиям:
    //1. Задан признак авто назначения компоновки
    //2. Задан признак регистрации
    //3. Борт и тип ВС, авиакомпания  совпадает с исходным пунктом
    //4. Не последний пункт в транзитном маршруте
    CompRoute route;
    bool inRoutes = true;
    route.point_id = i->point_id;
    route.auto_comp_chg = isAutoCompChg( i->point_id );
    Qry.SetVariable( "point_id", i->point_id );
    Qry.Execute();
    route.airline = Qry.FieldAsString( "airline" );
    route.airp = Qry.FieldAsString( "airp" );
    route.craft = Qry.FieldAsString( "craft" );
    route.bort = Qry.FieldAsString( "bort" );
    route.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
    route.inRoutes = ( CompRouteinRoutes( currroute, route ) && inRoutes);
    if ( !pr_before && (!route.inRoutes || i == routes.end() - 1) )
      inRoutes = false;
    comps_routes.push_back( route );
  }
}

void get_comp_routes( bool pr_tranzit_routes, int point_id, TCompsRoutes &routes )
{
  //!!! for prior version set pr_tranzit_routes = false;
  routes.clear();
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT point_num,first_point,pr_tranzit,"
    "       airline,airp,bort,craft,pr_reg "
    " FROM points "
    " WHERE point_id=:point_id AND pr_del!=-1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return;
  int point_num = Qry.FieldAsInteger( "point_num" );
  int first_point = Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  bool pr_tranzit = Qry.FieldAsInteger( "pr_tranzit" ) != 0;
  CompRoute currroute;
  currroute.point_id = point_id;
  currroute.airline = Qry.FieldAsString( "airline" );
  currroute.airp = Qry.FieldAsString( "airp" );
  currroute.bort = Qry.FieldAsString( "bort" );
  currroute.craft = Qry.FieldAsString( "craft" );
  currroute.pr_reg = ( Qry.FieldAsInteger( "pr_reg" ) == 1 );
  currroute.auto_comp_chg = isAutoCompChg( point_id );
  currroute.inRoutes = true;
	ProgTrace( TRACE5, "get_comp_routes: point_id=%d,pr_tranzit_routes=%d,point_num=%d,first_point=%d,pr_tranzit=%d,bort=%s,craft=%s",
             point_id,pr_tranzit_routes,point_num,first_point,pr_tranzit,currroute.bort.c_str(),currroute.craft.c_str() );
  TTripRoute routesB, routesA;
  if ( pr_tranzit ) {
    routesB.GetRouteBefore( NoExists,
                            point_id,
                            point_num,
                            first_point,
                            pr_tranzit,
                            trtNotCurrent,
                            trtNotCancelled );
  }
  routesA.GetRouteAfter( NoExists,
                         point_id,
                         point_num,
                         first_point,
                         pr_tranzit,
                         trtNotCurrent,
                         trtNotCancelled );
  if ( routesA.empty() ) { // рейс на прилет
    routes.push_back( currroute );
    ProgTrace( TRACE5, "get_comp_routes: routesA.empty()" );
    return;
  }
  tst();
  if ( pr_tranzit_routes ) { //задание компоновки для всего маршрута
    push_routes( currroute, routesB, true, routes );
  }
  routes.push_back( currroute );
  if ( pr_tranzit_routes )  //задание компоновки для всего маршрута
    push_routes( currroute, routesA, false, routes );
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    ProgTrace( TRACE5, "get_comp_routes: i->point_id=%d, i->inRoutes=%d, i->pr_reg=%d", i->point_id, i->inRoutes, i->pr_reg );
  }
}

struct TCounters {
  int point_id;
  int f, c, y;
  TCounters() {
    f = 0;
    c = 0;
    y = 0;
    point_id = -1;
  };
};

void getCrsData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT NVL( MAX( DECODE( class, 'П', cfg, 0 ) ), 0 ) f, "
    "       NVL( MAX( DECODE( class, 'Б', cfg, 0 ) ), 0 ) c, "
    "       NVL( MAX( DECODE( class, 'Э', cfg, 0 ) ), 0 ) y "
    " FROM crs_data,tlg_binding,points "
    " WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
    "       points.point_id=:point_id AND "
    "       point_id_spp=:point_id AND system='CRS' AND airp_arv=points.airp ";
  Qry.DeclareVariable( "point_id", otInteger );

  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    ProgTrace( TRACE5, "getCrsData: routes->point_id=%d", *i );
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    crs_data[ *i ].f = Qry.FieldAsInteger( "f" );
    crs_data[ *i ].c = Qry.FieldAsInteger( "c" );
    crs_data[ *i ].y = Qry.FieldAsInteger( "y" );
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
         crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
  }
}

void getCountersData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
	Qry.SQLText =
    "SELECT airp_arv,class, "
    "       0 AS priority, "
    "       crs_ok + crs_tranzit AS c "
    " FROM crs_counters "
    "WHERE point_dep=:point_id "
    "UNION "
    "SELECT airp_arv,class,1,resa + tranzit "
    " FROM trip_data "
    "WHERE point_id=:point_id "
    "ORDER BY priority DESC ";
  Qry.DeclareVariable( "point_id", otInteger );
  
  string vclass;
  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    ProgTrace( TRACE5, "getCountersData: routes->point_id=%d", *i );
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    int priority = -1;
    while ( !Qry.Eof ) {
   		if ( Qry.FieldAsInteger( "c" ) > 0 ) {
   		  priority = Qry.FieldAsInteger( "priority" );
        if ( priority != Qry.FieldAsInteger( "priority" ) )
          break;
        vclass = Qry.FieldAsString( "class" );
        ProgTrace( TRACE5, "point_id=%d, class=%s, count=%d", *i, vclass.c_str(), Qry.FieldAsInteger( "c" ) );
   	  	if ( vclass == "П" ) crs_data[ *i ].f += Qry.FieldAsInteger( "c" );
     		if ( vclass == "Б" ) crs_data[ *i ].c += Qry.FieldAsInteger( "c" );
   	  	if ( vclass == "Э" ) crs_data[ *i ].y += Qry.FieldAsInteger( "c" );
      }
    	Qry.Next();
    }
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
      crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
    ProgTrace( TRACE5, "crs_data[ %d ].f=%d, crs_data[ %d ].c=%d, crs_data[ %d ].y=%d",
               *i, crs_data[ *i ].f, *i, crs_data[ *i ].c, *i, crs_data[ *i ].y );
  }
  ProgTrace( TRACE5, "point_id=%d, crs_data[ -1 ].f=%d, crs_data[ -1 ].c=%d, crs_data[ -1 ].y=%d",
             crs_data[ -1 ].point_id, crs_data[ -1 ].f, crs_data[ -1 ].c, crs_data[ -1 ].y );
}

void getSeasonData( const vector<int> &points, map<int,TCounters> &crs_data )
{
  crs_data.clear();
  TQuery Qry(&OraSession);
	Qry.SQLText =
  	"SELECT ABS(f) f, ABS(c) c, ABS(y) y FROM trip_sets WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  
  int priorf = NoExists, priorc = NoExists, priory = NoExists;
  for ( vector<int>::const_iterator i=points.begin(); i!=points.end(); i++ ) {
    Qry.SetVariable( "point_id", *i );
    Qry.Execute();
    if ( !Qry.Eof ) {
      ProgTrace( TRACE5, "point_id=%d", *i );
      if ( priorf == Qry.FieldAsInteger( "f" ) &&
           priorc == Qry.FieldAsInteger( "c" ) &&
           priory == Qry.FieldAsInteger( "y" ) )
        continue;
      crs_data[ *i ].f = Qry.FieldAsInteger( "f" );
      crs_data[ *i ].c = Qry.FieldAsInteger( "c" );
      crs_data[ *i ].y = Qry.FieldAsInteger( "y" );
      priorf = Qry.FieldAsInteger( "f" );
      priorc = Qry.FieldAsInteger( "c" );
      priory = Qry.FieldAsInteger( "y" );
    }
    if ( crs_data[ -1 ].f + crs_data[ -1 ].c + crs_data[ -1 ].y <
      crs_data[ *i ].f + crs_data[ *i ].c + crs_data[ *i ].y ) {
      crs_data[ -1 ].f = crs_data[ *i ].f;
      crs_data[ -1 ].c = crs_data[ *i ].c;
      crs_data[ -1 ].y = crs_data[ *i ].y;
      crs_data[ -1 ].point_id = *i;
    }
    ProgTrace( TRACE5, "crs_data[ %d ].f=%d, crs_data[ %d ].c=%d, crs_data[ %d ].y=%d",
               *i, crs_data[ *i ].f, *i, crs_data[ *i ].c, *i, crs_data[ *i ].y );
  }
  ProgTrace( TRACE5, "point_id=%d, crs_data[ -1 ].f=%d, crs_data[ -1 ].c=%d, crs_data[ -1 ].y=%d",
             crs_data[ -1 ].point_id, crs_data[ -1 ].f, crs_data[ -1 ].c, crs_data[ -1 ].y );
}

int CRC32_Comp( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT num,x,y,elem_type,class,xname,yname FROM trip_comp_elems "
    " WHERE point_id=:point_id "
    "ORDER BY num,x,y";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    return 0;
  string buf;
  int idx_num = Qry.FieldIndex( "num" );
  int idx_x = Qry.FieldIndex( "x" );
  int idx_y = Qry.FieldIndex( "y" );
  int idx_elem_type = Qry.FieldIndex( "elem_type" );
  int idx_class = Qry.FieldIndex( "class" );
  int idx_xname = Qry.FieldIndex( "xname" );
  int idx_yname = Qry.FieldIndex( "yname" );
  while ( !Qry.Eof ) {
    buf += Qry.FieldAsString( idx_num );
    buf += Qry.FieldAsString( idx_x );
    buf += Qry.FieldAsString( idx_y );
    buf += Qry.FieldAsString( idx_elem_type );
    buf += Qry.FieldAsString( idx_class );
    buf += Qry.FieldAsString( idx_xname );
    buf += Qry.FieldAsString( idx_yname );
    Qry.Next();
  }
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  crc32.process_bytes( buf.c_str(), buf.size() );
  int comp_id = crc32.checksum();
  ProgTrace( TRACE5, "CRC32_Comp: point_id=%d, crc_comp=%d", point_id, comp_id );
  return comp_id;
}

int getCRC_Comp( int point_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT crc_comp,comp_id FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgError( STDLOG, "getCRC_Comp: point_id=%d, trip_sets not exists record", point_id );
    return 0;
  }
  return Qry.FieldAsInteger( "crc_comp" );
}

void calc_diffcomp_alarm( TCompsRoutes &routes )
{
  TCompsRoutes::iterator iprior = routes.end();
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    ProgTrace( TRACE5, "i->point_id=%d, i->pr_reg=%d", i->point_id, i->pr_reg );
    i->pr_alarm = false;
    if ( !i->pr_reg )
      continue;
    if ( iprior != routes.end() && i != routes.end()-1 ) {
      ProgTrace( TRACE5, "i->point_id=%d", i->point_id );
      int crc_comp1 = getCRC_Comp( iprior->point_id );
      int crc_comp2 = getCRC_Comp( i->point_id );
      if ( !CompRouteinRoutes( *iprior, *i ) ||
           ( crc_comp1 != 0 &&
             crc_comp2 != 0 &&
             crc_comp1 != crc_comp2 ) ) {
         i->pr_alarm = true;
      }
    }
    iprior = i;
  }
}

void check_diffcomp_alarm( TCompsRoutes &routes )
{
 calc_diffcomp_alarm( routes );
 for (  TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
   set_alarm( i->point_id, atDiffComps, i->pr_alarm );
 }
}

void check_diffcomp_alarm( int point_id )
{
  TCompsRoutes routes;
  get_comp_routes( true, point_id, routes );
  check_diffcomp_alarm( routes );
}

std::string getDiffCompsAlarmRoutes( int point_id )
{
/*  TCompsRoutes routes;
  get_comp_routes( true, point_id, routes );
  calc_diffcomp_alarm( routes );
  string res;
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( !i->pr_alarm )
      continue;
    if ( res.empty() )
      res += ":";
    else
      res = "-";
    res += i->airp;
  }
  ProgTrace( TRACE5, "getDiffCompsAlarmRoutes: point_id=%d, res=%s", point_id, res.c_str() );
  return res; */
  string res;
  return res;
}

TFindSetCraft SetCraft( bool pr_tranzit_routes, int point_id, TSetsCraftPoints &points )
{
  points.Clear();
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT bort,airline,airp,craft, NVL(comp_id,-1) comp_id "
    " FROM points, trip_sets "
    " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string bort = Qry.FieldAsString( "bort" );
  string airline = Qry.FieldAsString( "airline" );
  string airp = Qry.FieldAsString( "airp" );
  int old_comp_id = Qry.FieldAsInteger( "comp_id" );
	string craft = Qry.FieldAsString( "craft" );
	ProgTrace( TRACE5, "SetCraft: point_id=%d,pr_tranzit_routes=%d,bort=%s,craft=%s,old_comp_id=%d",
             point_id,pr_tranzit_routes,bort.c_str(),craft.c_str(),old_comp_id );
	if ( craft.empty() ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoCraftComps, craft.empty()" );
    return rsComp_NoCraftComps;
  }
  // проверка на существование заданной компоновки по типу ВС
	Qry.Clear();
	Qry.SQLText =
   "SELECT comp_id FROM comps "
   " WHERE craft=:craft AND rownum < 2";
 	Qry.CreateVariable( "craft", otString, craft );
 	Qry.Execute();
 	if ( Qry.Eof ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoCraftComps" );
    return rsComp_NoCraftComps;
  }
  tst();
  TCompsRoutes routes;
  get_comp_routes( pr_tranzit_routes, point_id, routes );
  tst();
  vector<string> airps;
  for ( TCompsRoutes::iterator i=routes.begin(); i!=routes.end(); i++ ) {
    if ( i->inRoutes && i->auto_comp_chg && i->pr_reg ) {
      points.push_back( i->point_id );
      airps.push_back( i->airp );
    }
    if ( i->point_id == point_id && !i->auto_comp_chg )
      return rsComp_NoChanges;
  }
  if ( points.empty() )
    return rsComp_NoChanges;
  map<int,TCounters> crs_data;
  
  for ( int step=0; step<=5; step++ ) {
    // выбираем макс. компоновку по CFG из PNL/ADL для всех центров бронирования
    ProgTrace( TRACE5, "step=%d", step );
    if ( step == 0 ) {
      getCrsData( points, crs_data );
    }
    if ( step == 2 ) {
      getCountersData( points, crs_data );
    }
    if ( step == 4 ) {
      getSeasonData( points, crs_data );
    }
    for ( map<int,TCounters>::iterator i=crs_data.begin(); i!=crs_data.end(); i++ ) {
      if ( step == 0 || step == 2 || step == 4 ) {
        if ( i->first != -1 ||
             i->second.f + i->second.c + i->second.y <= 0 )
          continue;
      }
      if ( step == 1 || step == 3 || step == 5 ) {
        if ( !pr_tranzit_routes ||
             i->first == -1 ||
             i->first == crs_data[ -1 ].point_id ||
             i->second.f + i->second.c + i->second.y <= 0 )
          continue;
      }
      points.comp_id = GetCompId( craft, bort, airline, airps, i->second.f, i->second.c, i->second.y );
      if ( points.comp_id >= 0 )
      	break;
    }
    if ( points.comp_id >= 0 )
    	break;
  }
  if ( points.comp_id < 0 ) {
    ProgTrace( TRACE5, "SetCraft: return rsComp_NoFound" );
    return rsComp_NoFound;
  }
	// найден вариант компоновки
	if ( old_comp_id == points.comp_id ) {
	  ProgTrace( TRACE5, "SetCraft: return rsComp_NoChanges" );
		return rsComp_NoChanges; // не нужно изменять компоновку
  }
  tst();
	CreateComps( routes, points.comp_id );
	tst();
	check_diffcomp_alarm( routes );
	ProgTrace( TRACE5, "SetCraft: return rsComp_Found" );
  return rsComp_Found;
}

TFindSetCraft AutoSetCraft( int point_id )
{
  TSetsCraftPoints points;
  return AutoSetCraft( true, point_id, points );
}

TFindSetCraft AutoSetCraft( int point_id, TSetsCraftPoints &points )
{
  return AutoSetCraft( true, point_id, points );
}

bool isAutoCompChg( int point_id )
{
	TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT auto_comp_chg FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "auto_comp_chg" ) ); // автоматическое назначение компоновки
}

void setManualCompChg( int point_id )
{
  //set flag auto change in false state
  TQuery Qry(&OraSession);
	Qry.SQLText = "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
}

TFindSetCraft AutoSetCraft( bool pr_tranzit_routes, int point_id, TSetsCraftPoints &points )
{
	ProgTrace( TRACE5, "AutoSetCraft, pr_tranzit_routes=%d, point_id=%d", pr_tranzit_routes, point_id );
	try {
	  points.Clear();
    if ( isAutoCompChg( point_id ) ) {
    	ProgTrace( TRACE5, "Auto set comp, point_id=%d", point_id );
      return SetCraft( pr_tranzit_routes, point_id, points );
    }
    return rsComp_NoChanges; // не требуется назначение компоновки
  }
  catch( EXCEPTIONS::Exception &e ) {
  	ProgError( STDLOG, "AutoSetCraft: Exception %s, point_id=%d", e.what(), point_id );
  }
  catch( ... ) {
  	ProgError( STDLOG, "AutoSetCraft: unknown error, point_id=%d", point_id );
  }
  return rsComp_NoChanges;
}

void InitVIP( int point_id )
{
	tst();
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
	TTripInfo info( Qry );
	if ( !GetTripSets( tsCraftInitVIP, info ) )
    return;
	// инициализация - разметра салона по умолчани
	TQuery QryVIP(&OraSession);
	Qry.Clear();
	Qry.SQLText =
	  "SELECT num, class, MIN( y ) miny, MAX( y ) maxy "
    " FROM trip_comp_elems, comp_elem_types "
    "WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "      comp_elem_types.pr_seat <> 0 AND "
    "      trip_comp_elems.point_id = :point_id "
    "GROUP BY trip_comp_elems.num, trip_comp_elems.class";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  QryVIP.SQLText =
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,'VIP', 0 FROM "
    " ( SELECT :point_id,num,x,y FROM trip_comp_elems "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        elem_type IN ( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) AND "
    "        trip_comp_elems.y = :y "
    "   MINUS "
    "  SELECT :point_id,num,x,y FROM trip_comp_ranges "
    "   WHERE point_id=:point_id AND layer_type IN (:block_cent_layer, :checkin_layer) "
    " ) "
    " MINUS "
    " SELECT :point_id,num,x,y,rem, 0 FROM trip_comp_rem "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        trip_comp_rem.y = :y "; //??? если есть ремарка назначенная пользователем, то не трогаем место
  QryVIP.CreateVariable( "point_id", otInteger, point_id );
  QryVIP.DeclareVariable( "num", otInteger );
  QryVIP.DeclareVariable( "y", otInteger );
  QryVIP.CreateVariable( "block_cent_layer", otString, EncodeCompLayerType(cltBlockCent) );
  QryVIP.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(cltCheckin) );
  while ( !Qry.Eof ) {
  	int ycount;
  	tst();
  	switch( DecodeClass( Qry.FieldAsString( "class" ) ) ) {
  		case F: ycount = REM_VIP_F;
  			break;
  		case C: ycount = REM_VIP_C;
  			break;
  		case Y: ycount = REM_VIP_Y;
  			break;
  		default: ycount = 0;
  	};
  	if ( ycount ) {
  		int vy = 0;
  		while ( Qry.FieldAsInteger( "miny" ) + vy <= Qry.FieldAsInteger( "maxy" ) && vy < ycount ) {
  			QryVIP.SetVariable( "num", Qry.FieldAsInteger( "num" ) );
  			QryVIP.SetVariable( "y", Qry.FieldAsInteger( "miny" ) + vy );
  			tst();
  			QryVIP.Execute();
  			tst();
  			if ( !QryVIP.RowsProcessed( ) )
  				break;
  			vy++;
  		}
  	}
  	Qry.Next();
  }
}

bool CompareRems( const vector<TRem> &rems1, const vector<TRem> &rems2 )
{
	if ( rems1.size() != rems2.size() )
		return false;
	for ( vector<TRem>::const_iterator p1=rems1.begin(),
		    p2=rems2.begin();
		    p1!=rems1.end(),
		    p2!=rems2.end();
		    p1++, p2++ ) {
		if ( p1->rem != p2->rem ||
			   p1->pr_denial != p2->pr_denial )
			return false;
  }
  return true;
}

bool CompareLayers( const vector<TPlaceLayer> &layer1, const vector<TPlaceLayer> &layer2 )
{
	if ( layer1.size() != layer2.size() )
		return false;
	for ( vector<TPlaceLayer>::const_iterator p1=layer1.begin(),
		    p2=layer2.begin();
		    p1!=layer1.end(),
		    p2!=layer2.end();
		    p1++, p2++ ) {
		if ( p1->layer_type != p2->layer_type )
			return false;
  }
  return true;
}

bool ComparedrawProps( const vector<TDrawPropsType> &drawProps1, const vector<TDrawPropsType> &drawProps2 )
{
  if ( drawProps1.size() != drawProps2.size() )
    return false;
  for ( vector<TDrawPropsType>::const_iterator p1=drawProps1.begin(),
        p2=drawProps2.begin();
        p1!=drawProps1.end(),
        p2!=drawProps2.end();
        p1++, p2++ ) {
    if ( *p1 != *p2 )
      return false;
  }
  return true;
};

bool EqualSalon( TPlaceList* oldsalon, TPlaceList* newsalon, TCompareCompsFlags compareFlags )
{
	//!!!возможно более тонко оценивать салон: удаление мест из ряда/линии - это места становятся невидимые, но при этом теряется само название ряда/линии
	bool res = ( oldsalon->places.size() == newsalon->places.size() &&
  			       oldsalon->GetXsCount() == newsalon->GetXsCount() &&
  			       oldsalon->GetYsCount() == newsalon->GetYsCount() );
  if ( !res )
  	return false;

  for ( TPlaces::iterator po = oldsalon->places.begin(),
    	                    pn = newsalon->places.begin();
        po != oldsalon->places.end(),
        pn != newsalon->places.end();
        po++, pn++ ) {
    if ( compareFlags.isFlag( ccCoord ) ) {
      if ( po->visible != pn->visible ||
           ( po->visible &&
             ( po->x != pn->x ||
     	         po->y != pn->y ) ) ) {
     	  return false;
      }
    }
    if ( compareFlags.isFlag( ccCoordOnlyVisible ) ) {
      if ( po->visible && pn->visible &&
      	   ( po->x != pn->x ||
       	     po->y != pn->y ) ) {
     	  return false;
      }
    }
    if ( compareFlags.isFlag( ccPlaceName ) ) {
      if ( po->visible && pn->visible &&
       	   ( po->xname != pn->xname ||
         	   po->yname != pn->yname ) ) {
        return false;
      }
    }
    if ( compareFlags.isFlag( ccPlaceType ) ) {
      if ( po->visible && pn->visible &&
       	   po->isplace != pn->isplace ) {
        return false;
      }
    }
    if ( compareFlags.isFlag( ccClass ) ) {
      if ( po->visible && pn->visible &&
       	   po->clname != pn->clname ) {
        return false;
      }
    }
  }
  return true;
}

struct TRowRef {
	string yname;
	string xnames;
};

typedef map<int,TRowRef,std::less<int> > RowsRef;

struct TStringRef {
	string value;
	bool pr_header;
	TStringRef( string vvalue, bool vpr_header ) {
		value = vvalue;
		pr_header = vpr_header;
	}
};


void SeparateEvents( vector<TStringRef> referStrs, vector<string> &eventsStrs, unsigned int line_len )
{
	for ( vector<TStringRef>::iterator i=referStrs.begin(); i!=referStrs.end(); i++ ) {
		ProgTrace( TRACE5, "i->value=%s, i->pr_header=%d",i->value.c_str(), i->pr_header);
  }
	eventsStrs.clear();
	if ( referStrs.empty() )
		return;

	string headStr, lineStr, prior_lineStr;
	bool pr_prior_header;
	if ( referStrs.begin()->pr_header )
		headStr = referStrs.begin()->value;
	lineStr = headStr;
	for ( vector<TStringRef>::iterator i=referStrs.begin(); i!=referStrs.end(); i++ ) {
		if ( i == referStrs.begin() && i->pr_header )
			continue;
		ProgTrace( TRACE5, "lineStr=%s, lineStr.size()=%zu, i->value=%s, i->pr_header=%d,i->size()=%zu, line_len=%d",
		           lineStr.c_str(),lineStr.size(),i->value.c_str(), i->pr_header, i->value.size(),line_len);
		if ( i->pr_header )
		  headStr = i->value;
		if ( lineStr.size() + i->value.size() >= line_len ) {
			if ( pr_prior_header )
				lineStr = prior_lineStr;
			tst();
			if ( lineStr.size() <= line_len ) {
				ProgTrace( TRACE5, "eventsStrs.push_back(%s)",lineStr.c_str() );
	      eventsStrs.push_back( lineStr );
	      lineStr.clear();
	    }
	    else
	    	while ( !lineStr.empty() ) {
	    		if ( lineStr.size() > line_len ) {
	    		  ProgTrace( TRACE5, "eventsStrs.push_back(%s)",lineStr.substr(0,line_len).c_str() );
	    		  eventsStrs.push_back( lineStr.substr(0,line_len) );
	    		  lineStr = lineStr.substr(line_len);
	    		  ProgTrace( TRACE5, "lineStr=%s)",lineStr.c_str() );
	    		}
	    		else {
	    		  eventsStrs.push_back( lineStr );
	    		  lineStr.clear();
	    		  tst();
	    		}
	    	}
	   prior_lineStr = lineStr;
	   if ( !i->pr_header )
	   	lineStr = headStr;
		}
		else
		  prior_lineStr = lineStr;
		if ( !lineStr.empty() )
	    lineStr += " ";
    lineStr += i->value;
    pr_prior_header = i->pr_header;
	}
	ProgTrace( TRACE5, "eventsStrs.push_back(%s)",lineStr.c_str() );
	eventsStrs.push_back( lineStr );
}


bool RightRows( const string &row1, const string &row2 )
{
	int r1, r2;
	if ( StrToInt( row1.c_str(), r1 ) == EOF || StrToInt( row2.c_str(), r2 ) == EOF )
		return false;
	ProgTrace( TRACE5, "r1=%d, r2=%d, EOF=%d, row1=%s, row2=%s", r1, r2, EOF, row1.c_str(), row2.c_str() );
	return ( r1 == r2 - 1 );
}



void getStrSeats( const RowsRef &rows, vector<TStringRef> &referStrs, bool pr_lat )
{
	for ( RowsRef::const_iterator i=rows.begin(); i!=rows.end(); i++ ) {
		 ProgTrace( TRACE5, "i->first=%d, i->second.yname=%s, i->second.xnames=%s", i->first, i->second.yname.c_str(), i->second.xnames.c_str() );
	}
	vector<TStringRef> strs1, strs2;
  string str, max_lines, denorm_max_lines;
  int var1_size=0, var2_size=0;
  bool pr_rr, pr_right_rows=true;
  for ( int i=0; i<=1; i++ ) {
    RowsRef::const_iterator first_isr=rows.begin();
    RowsRef::const_iterator prior_isr=rows.begin();
    RowsRef::const_iterator isr=rows.begin();
    while ( !rows.empty() ) {
    	if ( isr == rows.end() ||
    		   ( prior_isr != isr && !(pr_rr=RightRows( prior_isr->second.yname, isr->second.yname )) ) ||
    		   ( i == 0 && first_isr->second.xnames != isr->second.xnames ) || //описание блока мест с одинаковыми линиями
    		   ( i != 0 && first_isr->second.xnames.find_first_of( isr->second.xnames ) != string::npos ) ) { //описание блока мест с пересекающимися линиями
        if ( !pr_rr )
        	pr_right_rows = false;
    		for ( string::const_iterator sp=prior_isr->second.xnames.begin(); sp!=prior_isr->second.xnames.end(); sp++ ) {
    		  if ( max_lines.find( *sp ) == string::npos ) {
    			  max_lines += *sp;
    			  denorm_max_lines += denorm_iata_line( string(1,*sp), pr_lat );
    			}
    	  }
    		if ( i == 0 ) {
 	  	    str += denorm_iata_row( first_isr->second.yname, NULL );
    		  if ( prior_isr->first != first_isr->first )
 	  		    str += "-" + denorm_iata_row( prior_isr->second.yname, NULL );
 	  		  for ( string::const_iterator sp=first_isr->second.xnames.begin(); sp!=first_isr->second.xnames.end(); sp++ ) {
 	  	      str += denorm_iata_line( string(1,*sp), pr_lat );
 	  	    }
 	  	    var1_size += str.size();
 	  	    strs1.push_back( TStringRef(str,false) );
     	  	first_isr = isr;
     	  	str.clear();
     	  }
     	  else
     	  	if ( isr == rows.end() ) {
     	  		if ( first_isr->first != prior_isr->first )
     	  		  str = denorm_iata_row( first_isr->second.yname, NULL ) + "-" + denorm_iata_row( prior_isr->second.yname, NULL );
     	  		else
     	  			str = denorm_iata_row( first_isr->second.yname, NULL );
     	  		str += denorm_max_lines;
     	  		// пишем те места, кот нет
     	  		string minus_lines;
     	  		for ( RowsRef::const_iterator isr=rows.begin(); isr!=rows.end(); isr++ ) {
     	  			minus_lines.clear();
     	  			for ( string::iterator sp=max_lines.begin(); sp!=max_lines.end(); sp++ ) {
     	  				if ( isr->second.xnames.find( *sp ) == string::npos )
     	  					minus_lines += denorm_iata_line( string(1,*sp), pr_lat );
     	  			}
     	  			if ( !minus_lines.empty() ) {
     	  				str += " -" + denorm_iata_row( isr->second.yname, NULL ) + minus_lines;
     	  			}
     	  		}
     	  		var2_size += str.size();
     	  		strs2.push_back( TStringRef(str,false) );
     	  	}
   	  }
   	  if ( isr == rows.end() )
   		  break;
   	  prior_isr = isr;
   	  isr++;
    }
  }
  for ( vector<TStringRef>::iterator i=strs1.begin(); i!=strs1.end(); i++ ) {
    ProgTrace( TRACE5, "getStrSeats: str1=%s", i->value.c_str() );
  }
  for ( vector<TStringRef>::iterator i=strs2.begin(); i!=strs2.end(); i++ ) {
    ProgTrace( TRACE5, "getStrSeats: str2=%s", i->value.c_str() );
  }
  if ( var1_size < var2_size || !pr_right_rows )
    referStrs.insert(	referStrs.end(), strs1.begin(), strs1.end() );
  else
  	referStrs.insert(	referStrs.end(), strs2.begin(), strs2.end() );
}

void ReferPlaces( string name, TPlaces places, std::vector<TStringRef> &referStrs, bool pr_lat )
{
	referStrs.clear();
	ProgTrace( TRACE5, "ReferPlacesRow: name=%s", name.c_str() );
	string str, tmp;
	if ( places.empty() )
		return;
  TCompElemType elem_type;
  TCompElemTypes::Instance()->getElem( places.begin()->elem_type, elem_type );
	tmp = "ADD_COMMON_SALON_REF";
  if ( name.find( tmp ) != string::npos ) {
  	referStrs.push_back( TStringRef("+Cалон "+name.substr(name.find( tmp )+tmp.size() ) + " " + places.begin()->clname + " " +
                         elem_type.name + ":", true) );
  	name.clear();
  }
	tmp = "ADD_SALON";
  if ( name.find( tmp ) != string::npos ) {
  	referStrs.push_back( TStringRef("+Cалон "+name.substr(name.find( tmp )+tmp.size() ) + ":", true) );
  }
  tmp = string("DEL_SALON" );
  if ( name.find( tmp ) != string::npos ) {
  	referStrs.push_back( TStringRef("-Cалон "+name.substr(name.find( tmp )+tmp.size() ) + ":", true) );
  }
  tmp = "ADD_SEATS";
  if ( name.find( tmp ) != string::npos ) {
  	referStrs.push_back( TStringRef("+" + elem_type.name + " " + places.begin()->clname + ":", true) );
  }
  tmp = "DEL_SEATS";
  if ( name.find( tmp ) != string::npos ) {
  	referStrs.push_back( TStringRef("-" + elem_type.name + " " + places.begin()->clname + ":", true) );
  }
  tmp = "ADD_REMS";
  if ( name.find( tmp ) != string::npos ) {
  	string rem = name.substr( name.find( tmp )+tmp.size() );
  	referStrs.push_back( TStringRef("+" + rem + ":",true) );
  }
  tmp = "DEL_REMS";
  if ( name.find( tmp ) != string::npos ) {
  	string rem = name.substr( name.find( tmp )+tmp.size() );
  	referStrs.push_back( TStringRef("-" + rem + ":", true) );
  }
  tmp = "ADD_LAYERS";
  if ( name.find( tmp ) != string::npos ) {
  	string layer_type = name.substr( name.find( tmp )+tmp.size() );
  	TCompLayerTypes &comp_layer_types = (TCompLayerTypes&)base_tables.get("comp_layer_types");
  	referStrs.push_back( TStringRef("+" + ((TCompLayerTypesRow&)comp_layer_types.get_row( "code", layer_type, true )).name + ":", true) );
  }
  tmp = "DEL_LAYERS";
  if ( name.find( tmp ) != string::npos ) {
  	string layer_type = name.substr( name.find( tmp )+tmp.size() );
  	TCompLayerTypes &comp_layer_types = (TCompLayerTypes&)base_tables.get("comp_layer_types");
  	referStrs.push_back( TStringRef("-" + ((TCompLayerTypesRow&)comp_layer_types.get_row( "code", layer_type, true )).name + ":",true) );
  }
  tmp = "ADD_WEB_TARIFF";
  if ( name.find( tmp ) != string::npos ) {
  	ostringstream str;
  	str << std::fixed << std::setprecision(2) << places.begin()->WebTariff.value << places.begin()->WebTariff.currency_id;
  	referStrs.push_back( TStringRef("+Web-тариф " + str.str() + ":",true) );
  }
  tmp = "DEL_WEB_TARIFF";
  if ( name.find( tmp ) != string::npos ) {
  	ostringstream str;
  	str << std::fixed << std::setprecision(2) << places.begin()->WebTariff.value << places.begin()->WebTariff.currency_id;
  	referStrs.push_back( TStringRef("-Web-тариф " + str.str() + ":",true) );
  }

	RowsRef rows;
	SALONS2::TPlace first_in_row;
	TPlaces::iterator priorip;
  // имеем набор одиноких мест - попробуем сделать из них объединение по линии
	//собираем одну группу мест
  for ( TPlaces::iterator ip=places.begin(), priorip=places.begin(); ip!=places.end(); ip++ ) {
  	//ProgTrace( TRACE5, "ReferPlacesRow: name=%s, place(%d,%d)=%s", name.c_str(), ip->x, ip->y, string(ip->xname+ip->yname).c_str() );
  	if ( ip == priorip ) {
  		first_in_row = *ip;
  		rows[ ip->y ].xnames += ip->xname;
  		rows[ ip->y ].yname = ip->yname;
  		continue;
    }
  	if ( priorip->y == ip->y/*priorip->x == ip->x - 1*/ ) {
  		rows[ ip->y ].xnames += ip->xname;
  		rows[ ip->y ].yname = ip->yname;
  	}
  	else { // в ряду подряд идущие места закончились
  		if ( first_in_row.y == ip->y - 1 ) {
  			ProgTrace( TRACE5, "new row, ip, first_in_row (%d,%d)=%s", ip->x, ip->y, string(ip->xname+ip->yname).c_str() );
  			priorip = ip;
  			first_in_row = *ip;
  			rows[ ip->y ].xnames += ip->xname;
  			rows[ ip->y ].yname = ip->yname;
  	  }
      else { // время собирать строки
      	getStrSeats( rows, referStrs, pr_lat );
      	//ProgTrace( TRACE5, "str=%s, single=%d", str.c_str(), pr_single_place );
      	rows.clear();
    		first_in_row = *ip;
    		rows[ ip->y ].xnames += ip->xname;
    		rows[ ip->y ].yname = ip->yname;
  	  	priorip = ip;
      }
  	}
  }
 	getStrSeats( rows, referStrs, pr_lat );
}

struct TRP {
	TPlaces places;
	vector<TStringRef> refs;
};

struct TRefPlaces {
	vector<TPlaceList*>::iterator salonIter;
	map<string,TRP> mapRef;
};

bool salonChangesToText( TSalons &OldSalons, TSalons &NewSalons, std::vector<std::string> &referStrs, bool pr_set_base, int line_len )
{
	ProgTrace( TRACE5, "OldSalons->placelists.size()=%zu, NewSalons->placelists.size()=%zu", OldSalons.placelists.size(), NewSalons.placelists.size() );
	typedef vector<string> TVecStrs;
	map<string,TVecStrs>  mapStrs;
	referStrs.clear();
	vector<TRefPlaces> vecChanges;
	map<string,TRP> mapChanges;
	vector<int> salonNums;
  // поиск в новой компоновки нужного салона
  ProgTrace( TRACE5, "pr_set_base=%d", pr_set_base );
  TCompareCompsFlags compareFlags;
  compareFlags.setFlag( ccCoordOnlyVisible );
  compareFlags.setFlag( ccPlaceName );
  if ( !pr_set_base ) { //изменение базового
    for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(); so!=OldSalons.placelists.end(); so++ ) {
    	bool pr_find_salon=false;
    	for ( vector<TPlaceList*>::iterator sn=NewSalons.placelists.begin(); sn!=NewSalons.placelists.end(); sn++ ) {
  	  	pr_find_salon = EqualSalon( *so, *sn, compareFlags );
  		  ProgTrace( TRACE5, "so->num=%d, pr_find_salon=%d", (*so)->num, pr_find_salon );
  		  if ( !pr_find_salon )
  			  continue;
  		  salonNums.push_back( (*sn)->num );
  		  // это нужный салон
        for ( TPlaces::iterator po = (*so)->places.begin(), // бежим по местам
  	                            pn = (*sn)->places.begin();
                                po != (*so)->places.end(),
                                pn != (*sn)->places.end();
                                po++, pn++ ) {
          if ( ( pn->visible && !pn->visible ) || ( pn->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) ) )
            mapChanges[ "ADD_SEATS" + pn->clname + pn->elem_type ].places.push_back( *pn );
          if ( ( po->visible && !pn->visible ) || ( po->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) ) ) {
         	  mapChanges[ "DEL_SEATS" + po->clname + po->elem_type ].places.push_back( *po );
         	  // не надо больше никакой информации о месте
         	  continue;
         	}

          if ( !CompareRems( po->rems, pn->rems ) ) { // разные ремарки
           	bool pr_find_rem=false;
           	for ( vector<TRem>::const_iterator ro=po->rems.begin(); ro!=po->rems.end(); ro++ ) { // пробег по старым ремаркам
           		for ( vector<TRem>::const_iterator rn=pn->rems.begin(); rn!=pn->rems.end(); rn++ ) { // пробег по новым - поиск старых
           			if ( ro->rem == rn->rem && ro->pr_denial == rn->pr_denial ) {
         	  			pr_find_rem = true;
         		  		break;
           			}
           	  }
           	  if ( !pr_find_rem ) { // старую ремарку не нашли
           	  	if ( ro->pr_denial )
          	  	  mapChanges[ "DEL_REMS!" + ro->rem ].places.push_back( *po );
          	  	else
         	    		mapChanges[ "DEL_REMS" + ro->rem ].places.push_back( *po );
         	    }
         	  }
            pr_find_rem=false;
            for ( vector<TRem>::const_iterator rn=pn->rems.begin(); rn!=pn->rems.end(); rn++ ) {
            	for ( vector<TRem>::const_iterator ro=po->rems.begin(); ro!=po->rems.end(); ro++ ) {
            		if ( ro->rem == rn->rem && ro->pr_denial == rn->pr_denial ) {
            			pr_find_rem = true;
            			break;
            		}
              }
              if ( !pr_find_rem ) { // старую ремарку не нашли
              	if ( rn->pr_denial )
              	  mapChanges[ "ADD_REMS!" + rn->rem ].places.push_back( *pn );
              	else
              		mapChanges[ "ADD_REMS" + rn->rem ].places.push_back( *pn );
              }
            }
          }

          if ( !CompareLayers( po->layers, pn->layers ) ) { // разные слои
           	bool pr_find_layer=false;
           	for ( vector<TPlaceLayer>::const_iterator lo=po->layers.begin(); lo!=po->layers.end(); lo++ ) {
           		if ( !NewSalons.isEditableLayer( lo->layer_type ) )
           			continue;
           		for ( vector<TPlaceLayer>::const_iterator ln=pn->layers.begin(); ln!=pn->layers.end(); ln++ ) {
         	  		// надо сравнивать только редактируемые слои
         		    if ( !NewSalons.isEditableLayer( ln->layer_type ) )
         		 	    continue;
         		    if ( lo->layer_type == ln->layer_type ) {
         		 	    pr_find_layer = true;
         		 	    break;
         		    }
         	    }
         	    if ( !pr_find_layer ) {
         	  	  mapChanges[ "DEL_LAYERS" + string(EncodeCompLayerType(lo->layer_type)) ].places.push_back( *po );
         	    }
         	  }
            pr_find_layer=false;
            for ( vector<TPlaceLayer>::const_iterator ln=pn->layers.begin(); ln!=pn->layers.end(); ln++ ) {
            	if ( !NewSalons.isEditableLayer( ln->layer_type ) )
            		continue;
            	for ( vector<TPlaceLayer>::const_iterator lo=po->layers.begin(); lo!=po->layers.end(); lo++ ) {
            		// надо сравнивать только редактируемые слои
            	  if ( !NewSalons.isEditableLayer( lo->layer_type ) )
            	 	  continue;
          	    if ( lo->layer_type == ln->layer_type ) {
          	 	    pr_find_layer = true;
          	 	    break;
          	    }
              }
              if ( !pr_find_layer ) {
            	  mapChanges[ "ADD_LAYERS" + string(EncodeCompLayerType(ln->layer_type)) ].places.push_back( *pn );
              }
            }
          }

          if ( po->WebTariff.color != pn->WebTariff.color ||
         	     po->WebTariff.value != pn->WebTariff.value ||
         	     po->WebTariff.currency_id != pn->WebTariff.currency_id ) {
         	  if ( po->WebTariff.value != 0.0 ) // старый тариф был
         		  mapChanges[ "DEL_WEB_TARIFF"+po->WebTariff.color+FloatToString(po->WebTariff.value)+po->WebTariff.currency_id ].places.push_back( *po );
         	  if ( pn->WebTariff.value != 0.0 )
         		  mapChanges[ "ADD_WEB_TARIFF"+pn->WebTariff.color+FloatToString(pn->WebTariff.value)+pn->WebTariff.currency_id ].places.push_back( *pn );
          }
        } // end for places
        break;
      } // end for NewSalons

      if ( !pr_find_salon ) {
  	    // не нашли салон - считаем что удалии его и возможно добавили новый
        for ( TPlaces::iterator po = (*so)->places.begin(); po != (*so)->places.end(); po++ ) {
      	  if ( po->visible )
   	  	    mapChanges[ "DEL_SALON"+IntToString((*so)->num+1) ].places.push_back( *po ); //+описание салона
 	    	}
      }

      TRefPlaces refp;
      refp.salonIter = so;
      refp.mapRef = mapChanges;
      vecChanges.push_back( refp );
      mapChanges.clear();

    } // end for OldSalon
  } // не надо выдавать изменения если назначили базовую компоновку
  mapChanges.clear();
  // надо добавить все салоны из NewSalons, которые не нашлись в OldSalons
  ProgTrace( TRACE5, "NewSalons->placelists.size()=%zu", NewSalons.placelists.size() );
  for ( vector<TPlaceList*>::iterator sn=NewSalons.placelists.begin(); sn!=NewSalons.placelists.end(); sn++ ) {
  	ProgTrace( TRACE5, "(*sn)->num=%d", (*sn)->num );
    if ( find( salonNums.begin(), salonNums.end(), (*sn)->num ) != salonNums.end() )
    	continue; // этот салон уже описан
    	bool pr_equal_salon_and_seats=true;
    	string clname, elem_type, name;
      for ( TPlaces::iterator pn = (*sn)->places.begin(); pn != (*sn)->places.end(); pn++ ) {
      	if ( pn->visible ) {
      		if ( clname.empty() ) {
      			clname = pn->clname;
      			elem_type = pn->elem_type;
      			ProgTrace( TRACE5, "clname=%s, elem_type=%s", clname.c_str(), elem_type.c_str() );
      		}
 	  	    mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.push_back( *pn ); //+описание салона
 	  	    mapChanges[ "ADD_SEATS" + pn->clname + pn->elem_type ].places.push_back( *pn );
 	  	    if ( clname != pn->clname ||
 	  	    	   elem_type != pn->elem_type )
            pr_equal_salon_and_seats = false;
 	  	    for ( vector<TRem>::const_iterator rn=pn->rems.begin(); rn!=pn->rems.end(); rn++ )
  	        if ( rn->pr_denial )
              mapChanges[ "ADD_REMS!" + rn->rem ].places.push_back( *pn );
            else
            	mapChanges[ "ADD_REMS" + rn->rem ].places.push_back( *pn );
          for ( vector<TPlaceLayer>::const_iterator ln=pn->layers.begin(); ln!=pn->layers.end(); ln++ )
          	if ( NewSalons.isEditableLayer( ln->layer_type ) )
            	mapChanges[ "ADD_LAYERS" + string(EncodeCompLayerType(ln->layer_type)) ].places.push_back( *pn );
          if ( pn->WebTariff.value != 0.0 )
         		mapChanges[ "ADD_WEB_TARIFF"+pn->WebTariff.color+FloatToString(pn->WebTariff.value)+pn->WebTariff.currency_id ].places.push_back( *pn );
 	  	  }
 	  	}
 	  	if ( pr_equal_salon_and_seats && !mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.empty() ) {
 	  		mapChanges[ "ADD_COMMON_SALON_REF"+IntToString((*sn)->num+1) ].places.assign( mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.begin(),
 	  		                                                                              mapChanges[ "ADD_SALON"+IntToString((*sn)->num+1) ].places.end() );
 	  		mapChanges.erase( "ADD_SALON"+IntToString((*sn)->num+1) );
 	  		mapChanges.erase( "ADD_SEATS" + clname + elem_type );
 	  	}
      TRefPlaces refp;
      refp.salonIter = sn;
      refp.mapRef = mapChanges;
      vecChanges.push_back( refp );
      mapChanges.clear();
  }
  // имеем массив названий с местами и салонами
  //необходимо сортировать по салонам и действиям
  bool pr_lat;
  vector<string> eventsStrs;
  vector<TStringRef> Refs;
  // пробег по салонам
  for ( vector<TRefPlaces>::iterator iref=vecChanges.begin(); iref!=vecChanges.end(); iref++ ) {
  	// вначале все удаленные свойства
  	for ( int i=0; i<=1; i++ ) {
  		for ( int j=0; j<5; j++ ) {
    		// пробег по изменениям
    		Refs.clear();
    	  for ( map<string,TRP>::iterator im=iref->mapRef.begin(); im!=iref->mapRef.end(); im++ ) {
    		  if ( im->second.places.empty() )
            		continue;
          if ( ( i == 0 && im->first.find( "DEL" ) == string::npos ) ||
        	     ( i == 1 && im->first.find( "DEL" ) != string::npos ) )
 	        	continue;
          if ( ( j == 0 && im->first.find( "SALON" ) == string::npos ) ||
    	    	   ( j == 1 && im->first.find( "SEATS" ) == string::npos ) ||
    	     	   ( j == 2 && im->first.find( "LAYERS" ) == string::npos ) ||
    	     	   ( j == 3 && im->first.find( "REMS" ) == string::npos ) ||
    	     	   ( j == 4 && im->first.find( "WEB_TARIFF" ) == string::npos ) )
    	     	continue;
 	        if ( i == 0 )
 	        	pr_lat = OldSalons.getLatSeat();
 	        else
 	      	  pr_lat = NewSalons.getLatSeat();
    		  ReferPlaces( im->first, im->second.places, im->second.refs, pr_lat );
    		  Refs.insert( Refs.end(), im->second.refs.begin(), im->second.refs.end() );
   		    // деление по строкам
    	  }
    	  if ( !Refs.empty() ) {
          SeparateEvents( Refs, eventsStrs, line_len );
    		  referStrs.insert( referStrs.end(), eventsStrs.begin(), eventsStrs.end() );
    		}
      }
    }
  }
  // хорошо было бы проанализировать на совпадение мест по нескольким добавленным/удаленным св-вам
  return !referStrs.empty();
}

bool getSalonChanges( TSalons &OldSalons, TSalons &NewSalons, vector<TSalonSeat> &seats )
{
	seats.clear();
	if ( NewSalons.getLatSeat() != OldSalons.getLatSeat() ||
		   NewSalons.placelists.size() != OldSalons.placelists.size() )
		return false;
	for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
		                                  sn=NewSalons.placelists.begin();
		    so!=OldSalons.placelists.end(),
		    sn!=NewSalons.placelists.end();
		    so++, sn++ ) {
		if ( (*so)->places.size() != (*sn)->places.size() )
			return false;
    for ( TPlaces::iterator po = (*so)->places.begin(),
    	    /*TPlaces::iterator*/ pn = (*sn)->places.begin();
          po != (*so)->places.end(),
          pn != (*sn)->places.end();
          po++, pn++ ) {
      if ( po->visible != pn->visible ||
      	   ( po->visible == pn->visible &&
      	     ( po->x != pn->x ||
      	       po->y != pn->y ||
      	       po->elem_type != pn->elem_type ||
      	       po->isplace != pn->isplace ||
               po->xprior != pn->xprior ||
               po->yprior != pn->yprior ||
               po->xnext != pn->xnext ||
               po->ynext != pn->ynext ||
               po->agle != pn->agle ||
               po->clname != pn->clname ||
               po->xname != pn->xname ||
               po->yname != pn->yname ) ) )
        return false;
      if ( !po->visible )
      	continue;
      if ( !CompareRems( po->rems, pn->rems ) ||
      	   !CompareLayers( po->layers, pn->layers ) ||
           !ComparedrawProps( po->drawProps, pn->drawProps ) ) {
       seats.push_back( make_pair((*so)->num,*pn) );
      }
    }
	}
	return true;
}

// выбор изменений по салону
void getSalonChanges( TSalons &OldSalons, vector<TSalonSeat> &seats )
{
	seats.clear();
	TSalons Salons( OldSalons.trip_id, rTripSalons );
	Salons.Read();
	if ( !getSalonChanges( OldSalons, Salons, seats ) )
		throw UserException( "MSG.SALONS.COMPON_CHANGED.REFRESH_DATA" );
}

void BuildSalonChanges( xmlNodePtr dataNode, const vector<TSalonSeat> &seats )
{
  if ( seats.empty() )
  	return;
  xmlNodePtr node = NewTextChild( dataNode, "update_salons" );
 	node = NewTextChild( node, "seats" );
 	int num = -1;
 	xmlNodePtr salonNode;
	for ( vector<TSalonSeat>::const_iterator p=seats.begin(); p!=seats.end(); p++ ) {
		if ( num != p->first ) {
			salonNode = NewTextChild( node, "salon" );
			SetProp( salonNode, "num", p->first );
			num = p->first;
		}
		xmlNodePtr n = NewTextChild( salonNode, "place" );
		NewTextChild( n, "x", p->second.x );
		NewTextChild( n, "y", p->second.y );
    xmlNodePtr remsNode = NULL;
    xmlNodePtr remNode;
    for ( vector<TRem>::const_iterator rem = p->second.rems.begin(); rem != p->second.rems.end(); rem++ ) {
      if ( !remsNode ) {
        remsNode = NewTextChild( n, "rems" );
      }
      remNode = NewTextChild( remsNode, "rem" );
      NewTextChild( remNode, "rem", rem->rem );
      if ( rem->pr_denial )
        NewTextChild( remNode, "pr_denial" );
      }
    if ( p->second.layers.size() > 0 ) {
      xmlNodePtr layersNode = NewTextChild( n, "layers" );
      for( std::vector<TPlaceLayer>::const_iterator l=p->second.layers.begin(); l!=p->second.layers.end(); l++ ) {
    		if ( l->layer_type  == cltDisable && !compatibleLayer( l->layer_type ) ) {
          if ( !remsNode ) {
            remsNode = NewTextChild( n, "rems" );
          }
    		  remNode = NewTextChild( remsNode, "rem" );
    		  NewTextChild( remNode, "rem", "X" );    //!!!
          continue;
    		}
      	remNode = NewTextChild( layersNode, "layer" );
      	NewTextChild( remNode, "layer_type", EncodeCompLayerType( l->layer_type ) );
      }
    }
    if ( !p->second.drawProps.empty() ) {
      remsNode = NewTextChild( n, "drawProps" );
      for ( vector<TDrawPropsType>::const_iterator iprop=p->second.drawProps.begin(); iprop!=p->second.drawProps.end(); iprop++ ) {
        TDrawPropInfo pinfo = getDrawProps( *iprop );
        remNode = NewTextChild( remsNode, "drawProp" );
        SetProp( remNode, "figure", pinfo.figure );
        SetProp( remNode, "color", pinfo.color );
      }
    }
	}
}

void getXYName( int point_id, std::string seat_no, std::string &xname, std::string &yname )
{
	xname.clear();
	yname.clear();
	//!!! работа не по индексам!!!
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT xname, yname FROM trip_comp_elems "
	  " WHERE point_id=:point_id AND "
	  "       (salons.denormalize_yname(yname,NULL)||salons.denormalize_xname(xname,0)=:seat_no OR "
	  "        salons.denormalize_yname(yname,NULL)||salons.denormalize_xname(xname,1)=:seat_no)";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "seat_no", otString, seat_no );
	Qry.Execute();
	if ( !Qry.Eof ) {
		tst();
		xname = Qry.FieldAsString( "xname" );
		yname = Qry.FieldAsString( "yname" );
	}
}

void ParseCompSections( xmlNodePtr sectionsNode, std::vector<TCompSection> &CompSections )
{
  CompSections.clear();
  if ( !sectionsNode )
    return;
  sectionsNode = sectionsNode->children;
  while ( sectionsNode && string((char*)sectionsNode->name) == "section" ) {
    TCompSection cs;
    cs.name = NodeAsString( sectionsNode );
    cs.firstRowIdx = NodeAsInteger( "@FirstRowIdx", sectionsNode );
    cs.lastRowIdx = NodeAsInteger( "@LastRowIdx", sectionsNode );
    CompSections.push_back( cs );
    sectionsNode = sectionsNode->next;
  }
  ProgTrace( TRACE5, "CompSections.size()=%zu", CompSections.size() );
}

void getLayerPlacesCompSection( SALONS2::TSalons &NSalons, TCompSection &compSection,
                                bool only_high_layer,
                                map<ASTRA::TCompLayerType, TPlaces> &uselayers_places,
                                int &seats_count )
{
  seats_count = 0;
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    uselayers_places[ il->first ].clear();
  }
  int Idx = 0;
  for ( vector<TPlaceList*>::iterator si=NSalons.placelists.begin(); si!=NSalons.placelists.end(); si++ ) {
    for ( int y=0; y<(*si)->GetYsCount(); y++ ) {
      if ( Idx >= compSection.firstRowIdx && Idx <= compSection.lastRowIdx ) { // внутри секции
        for ( int x=0; x<(*si)->GetXsCount(); x++ ) {
         TPlace *p = (*si)->place( (*si)->GetPlaceIndex( x, y ) );
         if ( !p->isplace || !p->visible )
           continue;
         seats_count++;
         for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
           if ( ( only_high_layer && !p->layers.empty() && p->layers.begin()->layer_type == il->first ) || // !!!вверху самый приоритетный слой
                ( !only_high_layer && p->isLayer( il->first ) ) ) {
             uselayers_places[ il->first ].push_back( *p );
             break;
           }
         }
        }
      }
      Idx++;
    }
  }
  for ( map<ASTRA::TCompLayerType, TPlaces>::iterator il=uselayers_places.begin(); il!=uselayers_places.end(); il++ ) {
    ProgTrace( TRACE5, "getPaxPlacesCompSection: layer_type=%s, count=%zu", EncodeCompLayerType(il->first), il->second.size() );
  }
}

bool ChangeCfg( TSalons &NewSalons, TSalons &OldSalons, TCompareCompsFlags compareFlags )
{
  if ( NewSalons.placelists.size() != OldSalons.placelists.size() ) {
    return true;
  }
  for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
        sn=NewSalons.placelists.begin();
        so!=OldSalons.placelists.end(),
        sn!=NewSalons.placelists.end();
        so++, sn++ ) {
    if ( !EqualSalon( *so, *sn, compareFlags ) ) {
      return true;
    }
  }
  return false;
}



//новое



} // end namespace SALONS2


