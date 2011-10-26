#include <stdlib.h>
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
#include "tripinfo.h"
#include "astra_locale.h"
#include "base_tables.h"
#include "term_version.h"

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
    "ORDER BY point_num DESC,DECODE(tlgs_in.type,'PRL',1,0)";
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
			if ( Qry.FieldAsInteger( "pr_block_trzt" ) )
				setFlag( cltBlockTrzt );
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

  if ( readStyle == rTripSalons ) {

    FilterLayers.getFilterLayers( trip_id ); // определение режима учета транзитных слоев

  	layers_priority[ cltUncomfort ].editable = true;
   	layers_priority[ cltSmoke ].editable = true;
   	layers_priority[ cltBlockCent ].editable = true;
   	layers_priority[ cltBlockCent ].notfree = true;
   	layers_priority[ cltProtect ].editable = true;
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
    layers_priority[ cltProtect ].name_view = layers_priority[ cltProtect ].name;
    if ( FilterLayers.isFlag( cltProtect ) )
      layers_priority[ cltProtect ].func_key = "Shift+F4";
    layers_priority[ cltUncomfort ].name_view = layers_priority[ cltUncomfort ].name;
    if ( FilterLayers.isFlag( cltUncomfort ) )
    	layers_priority[ cltUncomfort ].func_key = "Shift+F5";
    layers_priority[ cltSmoke ].name_view = layers_priority[ cltSmoke ].name;
    if ( FilterLayers.isFlag( cltSmoke ) )
    	layers_priority[ cltSmoke ].func_key = "Shift+F6";
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

void TSalons::BuildLayersInfo( xmlNodePtr salonsNode )
{
  xmlNodePtr editNode = NewTextChild( salonsNode, "layers_prop" );
  TReqInfo *r = TReqInfo::Instance();
  for( map<TCompLayerType,TLayerProp>::iterator i=layers_priority.begin(); i!=layers_priority.end(); i++ ) {
    if (
         ( i->first == ASTRA::cltProtBeforePay ||
           i->first == ASTRA::cltProtAfterPay ||
           i->first == ASTRA::cltPNLBeforePay ||
           i->first == ASTRA::cltPNLAfterPay ) &&
         !TReqInfo::Instance()->desk.compatible( PROT_PAID_VERSION ) ) {
      continue;
    }

  	xmlNodePtr n = NewTextChild( editNode, "layer", EncodeCompLayerType( i->first ) );
  	SetProp( n, "name", i->second.name );
  	SetProp( n, "priority", i->second.priority );
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
    	if ( pr_edit ) {
  		  SetProp( n, "edit", 1 );
  		  if ( i->first == cltSmoke || i->first == cltUncomfort )
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
  }
 	xmlNodePtr n = NewTextChild( editNode, "layer",  EncodeCompLayerType( cltUnknown ) );
 	SetProp( n, "name", "LAYER_CLEAR_ALL" );
 	SetProp( n, "priority", 10000 );
 	SetProp( n, "edit", 1 );
  SetProp( n, "name_view_help", AstraLocale::getLocaleText("Очистить все статусы мест") );
  SetProp( n, "func_key", "Shift+F8" );
}

void TSalons::Build( xmlNodePtr salonsNode )
{
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
      	remsNode = NewTextChild( placeNode, "layers" );
      	for( std::vector<TPlaceLayer>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
      		remNode = NewTextChild( remsNode, "layer" );
      		NewTextChild( remNode, "layer_type", EncodeCompLayerType( l->layer_type ) );
      	}
      }
      if ( place->WebTariff.value != 0.0 ) {
      	remNode = NewTextChild( placeNode, "tariff", place->WebTariff.value );
      	SetProp( remNode, "color", place->WebTariff.color );
      	SetProp( remNode, "currency_id", place->WebTariff.currency_id );
      }
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
  if ( readStyle == rTripSalons ) {
  	BuildLayersInfo( salonsNode );
  }
}

void TSalons::Write()
{
  if ( readStyle == rTripSalons )
    ProgTrace( TRACE5, "TSalons::Write TripSalons with params trip_id=%d",
               trip_id );
  else {
    if ( modify == mNone )
      return;  //???
    ClName.clear();
    ProgTrace( TRACE5, "TSalons::Write ComponSalons with params comp_id=%d",
               comp_id );
  }
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
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
  }
  Qry.Execute();
  if ( readStyle == rComponSalons && modify == mDelete )
    return; /* удалили компоновку */

  TQuery QryWebTariff( &OraSession );
  TQuery RQry( &OraSession );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
      " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
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


  Qry.Clear();
  if ( readStyle == rTripSalons ) {
    Qry.SQLText = "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class, "
                  "                            pr_smoke,not_good,xname,yname) "
                  " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, "
                  "        :pr_smoke,:not_good,:xname,:yname)";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
  }
  else {
    Qry.SQLText = "INSERT INTO comp_elems(comp_id,num,x,y,elem_type,xprior,yprior,agle,class, "\
                  "                       pr_smoke,not_good,xname,yname) "\
                  " VALUES(:comp_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, "\
                  "        :pr_smoke,:not_good,:xname,:yname) ";
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
  Qry.DeclareVariable( "pr_smoke", otInteger );
  Qry.DeclareVariable( "not_good", otInteger );
  Qry.DeclareVariable( "xname", otString );
  Qry.DeclareVariable( "yname", otString );

  vector<TPlaceList*>::iterator plist;
  map<TClass,int> countersClass;
  TClass cl;
  for ( plist = placelists.begin(); plist != placelists.end(); plist++ ) {
    Qry.SetVariable( "num", (*plist)->num );
    RQry.SetVariable( "num", (*plist)->num );
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
      if ( place->clname.empty() || !ispl[ place->elem_type ] )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", place->clname );
        cl = DecodeClass( place->clname.c_str() );
        if ( cl != NoClass )
          countersClass[ cl ] = countersClass[ cl ] + 1;
      }
      if ( place->isLayer( cltSmoke ) ) {
      	tst();
      	Qry.SetVariable( "pr_smoke", 1 );
      }
      else
        Qry.SetVariable( "pr_smoke", FNull );
      if ( place->isLayer( cltUncomfort ) ) {
      	tst();
      	Qry.SetVariable( "not_good", 1 );
      }
      else
        Qry.SetVariable( "not_good", FNull );
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
      	  ProgTrace( TRACE5, "write layer=%s, editable=%d", EncodeCompLayerType( l->layer_type ), layers_priority[ l->layer_type ].editable );
      		if ( !layers_priority[ l->layer_type ].editable || l->layer_type == cltUncomfort || l->layer_type == cltSmoke )
      			continue;
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
      		 paxLayer.priority == i->priority &&
      		 paxLayer.time_create > i->time_create )
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
//      tst();
      continue;
    }
    if ( !( vfirst_x == vlast_x && vfirst_y+(int)ipax_layer->places.size()-1 == vlast_y ||
            vfirst_y == vlast_y && vfirst_x+(int)ipax_layer->places.size()-1 == vlast_x ) ) {
  //    tst();
      ipax_layer->valid = -1;
      continue;
    }
    // если мы здесь, то слой хороший. надо пометить все остальные слои у пассажира как плохие
    ProgTrace( TRACE5, "GetValidPaxLayer5: ipax->pax_id=%d, layer_type=%s -- ok", ipax->first, EncodeCompLayerType( ipax_layer->layer_type ) );
    SetValidPaxLayer( ipax, ipax_layer );
    break;
  }
}

void TSalons::Read( )
{
  if ( readStyle == rTripSalons )
  	;
  else {
    ClName.clear();
  }
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
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
      " SELECT pax.pax_id, pax.seats, class, 1 priority "
      "    FROM pax_grp,pax "
      "   WHERE pax.grp_id=pax_grp.grp_id AND "
      "         pax_grp.point_dep=:point_dep AND "
      "         pax.seats >= 1 AND "
      "         pax.refuse IS NULL "
      " UNION "
      " SELECT pax_id, crs_pax.seats, crs_pnr.class, 2 priority "
      "    FROM crs_pax, crs_pnr, tlg_binding "
      "   WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "         crs_pnr.point_id=tlg_binding.point_id_tlg AND "
      "         tlg_binding.point_id_spp=:point_dep AND "
      "         crs_pnr.system='CRS' AND "
      "         crs_pax.seats >= 1 AND "
      "         crs_pax.pr_del=0 "
      " ORDER BY priority ";
    PaxQry.CreateVariable( "point_dep", otInteger, trip_id );
  	// зачитываем все слои в салоне
  	Qry.SQLText =
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.pr_smoke,t.not_good,t.xname,t.yname,t.class,r.layer_type, "
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
      "SELECT num,x,y,elem_type,xprior,yprior,agle,pr_smoke,not_good,xname,yname,class "
      " FROM comp_elems "
      "WHERE comp_id=:comp_id "
      "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
  }
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    if ( readStyle == rTripSalons )
      throw UserException( "MSG.FLIGHT_WO_CRAFT_CONFIGURE" );
    else
      throw UserException( "MSG.SALONS.NOT_FOUND" );
  int col_num = Qry.FieldIndex( "num" );
  int col_x = Qry.FieldIndex( "x" );
  int col_y = Qry.FieldIndex( "y" );
  int col_elem_type = Qry.FieldIndex( "elem_type" );
  int col_xprior = Qry.FieldIndex( "xprior" );
  int col_yprior = Qry.FieldIndex( "yprior" );
  int col_agle = Qry.FieldIndex( "agle" );
  int col_pr_smoke = Qry.FieldIndex( "pr_smoke" );
  int col_not_good = Qry.FieldIndex( "not_good" );
  int col_xname = Qry.FieldIndex( "xname" );
  int col_yname = Qry.FieldIndex( "yname" );
  int col_class = Qry.FieldIndex( "class" );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText =
      "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    RQry.CreateVariable( "point_id", otInteger, trip_id );
    
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
  if ( readStyle == rTripSalons ) { // заполняем инфу по пассажиру
  	PaxQry.Execute();
    while ( !PaxQry.Eof ) {
    	if ( pax_layers.find( PaxQry.FieldAsInteger( "pax_id" ) ) == pax_layers.end() ) {
    	  pax_layers[ PaxQry.FieldAsInteger( "pax_id" ) ].seats = PaxQry.FieldAsInteger( "seats" );
    	  pax_layers[ PaxQry.FieldAsInteger( "pax_id" ) ].cl = PaxQry.FieldAsString( "class" );
    	}
    	PaxQry.Next();
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
      place.isplace = ispl[ place.elem_type ];
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
      if ( !Qry.FieldIsNULL( col_pr_smoke ) )
      	place.AddLayerToPlace( cltSmoke, 0, 0, NoExists, NoExists, layers_priority[ cltSmoke ].priority );
      if ( !Qry.FieldIsNULL( col_not_good ) )
      	place.AddLayerToPlace( cltUncomfort, 0, 0, NoExists, NoExists, layers_priority[ cltUncomfort ].priority );
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
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
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
      place.isplace = ispl[ place.elem_type ];
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
      if ( remsNode ) {
      	remsNode = remsNode->children;
      	while ( remsNode ) {
      	  remNode = remsNode->children;
      	  rem.rem = NodeAsStringFast( "rem", remNode );
      	  rem.pr_denial = GetNodeFast( "pr_denial", remNode );
      	  place.rems.push_back( rem );
      	  remsNode = remsNode->next;
        }
      }
      remsNode = GetNodeFast( "layers", node );
      if ( remsNode ) {
      	remsNode = remsNode->children; //layer
      	while( remsNode ) {
      		remNode = remsNode->children;
//      		ProgTrace( TRACE5, "la
      		TCompLayerType l = DecodeCompLayerType( NodeAsStringFast( "layer_type", remNode ) );
      		if ( l != cltUnknown && !place.isLayer( l ) )
      			 place.AddLayerToPlace( l, 0, 0, NoExists, NoExists, layers_priority[ l ].priority );
      		remsNode = remsNode->next;
      	}
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
      	   !seat_no.empty() && seat_no == salon_seat_no ) {
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
                        std::string airp,  int f, int c, int y )
{
	ProgTrace( TRACE5, "craft=%s, bort=%s, airline=%s, airp=%s, f=%d, c=%d, y=%d",
	           craft.c_str(), bort.c_str(), airline.c_str(), airp.c_str(), f, c, y );
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
	ProgTrace( TRACE5, "bort=%s, airline=%s, airp=%s", bort.c_str(), airline.c_str(), airp.c_str() );
  while ( !Qry.Eof ) {
  	string comp_airline = Qry.FieldAsString( "airline" );
  	string comp_airp = Qry.FieldAsString( "airp" );
  	bool airline_OR_airp = !comp_airline.empty() && airline == comp_airline ||
    	                     comp_airline.empty() && !comp_airp.empty() && airp == comp_airp;
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
// ProgTrace( TRACE5, "CompMap.size()=%d", CompMap.size() );
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
    "          NVL(SUM(DECODE( layer_type, :blockcent_layer, 1, 0 )),0) block, "
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
  Qry.CreateVariable( "reserve_layer", otString, EncodeCompLayerType(ASTRA::cltProtect) );
  Qry.Execute();
}

int AutoSetCraft( int point_id, std::string &craft, int comp_id )
{
	ProgTrace( TRACE5, "AutoSetCraft, point_id=%d", point_id );
	try {
	  TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT auto_comp_chg FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( !Qry.Eof && Qry.FieldAsInteger( "auto_comp_chg" ) ) { // автоматическое назначение компоновки
    	ProgTrace( TRACE5, "Auto set comp, point_id=%d", point_id );
      return SetCraft( point_id, craft, comp_id );
    }
    return 0; // не требуется назначение компоновки
  }
  catch( EXCEPTIONS::Exception &e ) {
  	ProgError( STDLOG, "AutoSetCraft: Exception %s, point_id=%d", e.what(), point_id );
  }
  catch( ... ) {
  	ProgError( STDLOG, "AutoSetCraft: unknown error, point_id=%d", point_id );
  }
  return 0;
}

int SetCraft( int point_id, std::string &craft, int comp_id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	  "SELECT bort,airline,suffix,airp,craft, NVL(comp_id,-1) comp_id "
    " FROM points, trip_sets "
    " WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id(+)";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  int f,c,y;
  string bort = Qry.FieldAsString( "bort" );
  string airline = Qry.FieldAsString( "airline" );
  string suffix = Qry.FieldAsString( "suffix" );
  string airp = Qry.FieldAsString( "airp" );
  int old_comp_id = Qry.FieldAsInteger( "comp_id" );
  if ( craft.empty() )
  	craft = Qry.FieldAsString( "craft" );
  // проверка на существование заданной компоновки по типу ВС
	Qry.Clear();
	Qry.SQLText =
   "SELECT comp_id FROM comps "
   " WHERE ( comp_id=:comp_id OR :comp_id < 0 ) AND craft=:craft AND rownum < 2";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
 	Qry.CreateVariable( "craft", otString, craft );
 	Qry.Execute();
 	if ( Qry.Eof )
 		if ( comp_id < 0 )
 	  	return -1;
 	  else
 	  	return -2;
 	while ( true ) {
  // выбираем макс. компоновку по CFG из PNL/ADL для всех центров бронирования
  	Qry.Clear();
  	Qry.SQLText =
      "SELECT NVL( MAX( DECODE( class, 'П', cfg, 0 ) ), 0 ) f, "
      "       NVL( MAX( DECODE( class, 'Б', cfg, 0 ) ), 0 ) c, "
      "       NVL( MAX( DECODE( class, 'Э', cfg, 0 ) ), 0 ) y "
      " FROM crs_data,tlg_binding "
      " WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
      "       point_id_spp=:point_id AND system='CRS' AND airp_arv=:airp ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "airp", otString, airp );
    Qry.Execute();
    f = Qry.FieldAsInteger( "f" );
    c = Qry.FieldAsInteger( "c" );
    y = Qry.FieldAsInteger( "y" );
    if ( f + c + y > 0 )
      comp_id = GetCompId( craft, bort, airline, airp, f, c, y );
      if ( comp_id >= 0 )
      	break;
  	Qry.Clear();
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
      "ORDER BY airp_arv,class,priority DESC ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    string airp_arv, vclass;
    f = 0; c = 0; y = 0;
    while ( !Qry.Eof ) {
    	if ( airp_arv.empty() ||
    		   vclass.empty() ||
    		   airp_arv != Qry.FieldAsString( "airp_arv" ) ||
    		   vclass != Qry.FieldAsString( "class" ) ) {
    		airp_arv = Qry.FieldAsString( "airp_arv" );
    		vclass = Qry.FieldAsString( "class" );
    		if ( vclass == "П" ) f = Qry.FieldAsInteger( "c" );
    		if ( vclass == "Б" ) c = Qry.FieldAsInteger( "c" );
    		if ( vclass == "Э" ) y = Qry.FieldAsInteger( "c" );
    	}
    	Qry.Next();
    }
    if ( f + c + y > 0 ) { // поиск варианта бронь + транзит
    	comp_id = GetCompId( craft, bort, airline, airp, f, c, y );
    	if ( comp_id >= 0 )
    		break;
    }
    else { // данных из бронирования не поступало, тогда поиск по данным из сезонки(f,c,y)
    	Qry.Clear();
    	Qry.SQLText =
    	  "SELECT ABS(f) f, ABS(c) c, ABS(y) y FROM trip_sets WHERE point_id=:point_id";
    	Qry.CreateVariable( "point_id", otInteger, point_id );
    	Qry.Execute();
    	if ( Qry.Eof ) {
    		f = 0;
    		c = 0;
    		y = 0;
    	}
    	else {
       f = Qry.FieldAsInteger( "f" );
       c = Qry.FieldAsInteger( "c" );
       y = Qry.FieldAsInteger( "y" );
    	}
      comp_id = GetCompId( craft, bort, airline, airp, f, c, y );
      if ( comp_id >= 0 )
      	break;
    }
    return -3;
	}
	// найден вариант компоновки
	if ( old_comp_id == comp_id )
		return 0; // не нужно изменять компоновку
	tst();
	Qry.Clear();
	Qry.SQLText =
	  "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.Execute();
  int pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

	Qry.Clear();
	Qry.SQLText =
	  "BEGIN "
	  "DELETE trip_comp_rates WHERE point_id = :point_id;"
	  "DELETE trip_comp_rem WHERE point_id = :point_id; "
    "DELETE trip_comp_elems WHERE point_id = :point_id; "
    "DELETE trip_comp_layers "
    " WHERE point_id=:point_id AND layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class, "
    "                            pr_smoke,not_good,xname,yname) "
    " SELECT :point_id,num,x,y,elem_type,xprior,yprior,agle,class, "
    "        pr_smoke,not_good,xname,yname "
    "  FROM comp_elems "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,rem,pr_denial "
    "  FROM comp_rem "
    " WHERE comp_id = :comp_id; "
    "INSERT INTO trip_comp_rates(point_id,num,x,y,color,rate,rate_cur) "
    " SELECT :point_id,num,x,y,color,rate,rate_cur FROM comp_rates "
    " WHERE comp_id = :comp_id; "
    "UPDATE trip_sets SET comp_id = :comp_id, pr_lat_seat = :pr_lat_seat WHERE point_id = :point_id; "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
  Qry.Execute();
  InitVIP( point_id );
  setTRIP_CLASSES( point_id );
  TReqInfo::Instance()->MsgToLog( string( "Назначена базовая компоновка (ид=" ) + IntToString( comp_id ) +
  	                              "). Классы: " + GetCfgStr(NoExists, point_id, AstraLocale::LANG_RU), evtFlt, point_id );
  check_waitlist_alarm( point_id );
  return comp_id;
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

bool EqualSalon( TPlaceList* oldsalon, TPlaceList* newsalon, bool equal_seats_cfg )
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
/*   	ProgTrace( TRACE5, "EqualSalon: po(%d,%d), oname=%s, pn(%d,%d) nname=%s, po->viisble=%d, pn->visible=%d",
               po->x, po->y, string(po->xname+po->yname).c_str(),
   	           pn->x, pn->y, string(pn->xname+pn->yname).c_str(),
               po->visible, pn->visible );*/

    if ( equal_seats_cfg ) { // сравнение конфигурации салона
      if ( po->visible != pn->visible ||
           po->visible &&
           ( po->x != pn->x ||
     	       po->y != pn->y ||
             po->isplace != pn->isplace ) ) {
/*       	ProgTrace( TRACE5, "EqualSalon: po(%d,%d), oname=%s, pn(%d,%d) nname=%s", po->x, po->y, string(po->xname+po->yname).c_str(),
      	           pn->x, pn->y, string(pn->xname+pn->yname).c_str() );*/
     	  return false;
      }
    }
    else {
      if ( po->visible && pn->visible &&
      	   ( po->x != pn->x ||
       	     po->y != pn->y ||
       	     po->xname != pn->xname ||
         	   po->yname != pn->yname ) ) {
/*       	ProgTrace( TRACE5, "EqualSalon: po(%d,%d), oname=%s, pn(%d,%d) nname=%s", po->x, po->y, string(po->xname+po->yname).c_str(),
     	             pn->x, pn->y, string(pn->xname+pn->yname).c_str() );*/
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
		ProgTrace( TRACE5, "lineStr=%s, lineStr.size()=%d, i->value=%s, i->pr_header=%d,i->size()=%d, line_len=%d",
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
    		   i == 0 && first_isr->second.xnames != isr->second.xnames || //описание блока мест с одинаковыми линиями
    		   i != 0 && first_isr->second.xnames.find_first_of( isr->second.xnames ) != string::npos ) { //описание блока мест с пересекающимися линиями
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
	tmp = "ADD_COMMON_SALON_REF";
  if ( name.find( tmp ) != string::npos ) {
  	tst();
  	TCompElemTypes &comp_elem_types = (TCompElemTypes&)base_tables.get( "comp_elem_types" );
  	referStrs.push_back( TStringRef("+Cалон "+name.substr(name.find( tmp )+tmp.size() ) + " " + places.begin()->clname + " " +
  	                     ((TCompElemTypesRow&)comp_elem_types.get_row( "code", places.begin()->elem_type, true )).name.c_str() + ":", true) );
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
  	TCompElemTypes &comp_elem_types = (TCompElemTypes&)base_tables.get( "comp_elem_types" );
  	referStrs.push_back( TStringRef("+" + ((TCompElemTypesRow&)comp_elem_types.get_row( "code", places.begin()->elem_type, true )).name+ " " + places.begin()->clname + ":", true) );
  }
  tmp = "DEL_SEATS";
  if ( name.find( tmp ) != string::npos ) {
  	TCompElemTypes &comp_elem_types = (TCompElemTypes&)base_tables.get( "comp_elem_types" );
  	referStrs.push_back( TStringRef("-" + ((TCompElemTypesRow&)comp_elem_types.get_row( "code", places.begin()->elem_type, true )).name+ " " + places.begin()->clname + ":", true) );
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
	ProgTrace( TRACE5, "OldSalons->placelists.size()=%d, NewSalons->placelists.size()=%d", OldSalons.placelists.size(), NewSalons.placelists.size() );
	typedef vector<string> TVecStrs;
	map<string,TVecStrs>  mapStrs;
	referStrs.clear();
	vector<TRefPlaces> vecChanges;
	map<string,TRP> mapChanges;
	vector<int> salonNums;
  // поиск в новой компоновки нужного салона
  ProgTrace( TRACE5, "pr_set_base=%d", pr_set_base );
  if ( !pr_set_base ) { //изменение базового
    for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(); so!=OldSalons.placelists.end(); so++ ) {
    	bool pr_find_salon=false;
    	for ( vector<TPlaceList*>::iterator sn=NewSalons.placelists.begin(); sn!=NewSalons.placelists.end(); sn++ ) {
  	  	pr_find_salon = EqualSalon( *so, *sn, false );
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
          if ( pn->visible && !pn->visible || pn->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) )
            mapChanges[ "ADD_SEATS" + pn->clname + pn->elem_type ].places.push_back( *pn );
          if ( po->visible && !pn->visible || po->visible && ( po->elem_type != pn->elem_type || po->clname != pn->clname ) ) {
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
  ProgTrace( TRACE5, "NewSalons->placelists.size()=%d", NewSalons.placelists.size() );
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
          if ( i == 0 && im->first.find( "DEL" ) == string::npos ||
        	     i == 1 && im->first.find( "DEL" ) != string::npos )
 	        	continue;
          if ( j == 0 && im->first.find( "SALON" ) == string::npos ||
    	    	   j == 1 && im->first.find( "SEATS" ) == string::npos ||
    	     	   j == 2 && im->first.find( "LAYERS" ) == string::npos ||
    	     	   j == 3 && im->first.find( "REMS" ) == string::npos ||
    	     	   j == 4 && im->first.find( "WEB_TARIFF" ) == string::npos )
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
      	   po->visible == pn->visible &&
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
             po->yname != pn->yname ) )
        return false;
      if ( !po->visible )
      	continue;
      if ( !CompareRems( po->rems, pn->rems ) ||
      	   !CompareLayers( po->layers, pn->layers ) ) {
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
      remsNode = NewTextChild( n, "layers" );
      for( std::vector<TPlaceLayer>::const_iterator l=p->second.layers.begin(); l!=p->second.layers.end(); l++ ) {
      	remNode = NewTextChild( remsNode, "layer" );
      	NewTextChild( remNode, "layer_type", EncodeCompLayerType( l->layer_type ) );
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

void ParseCompSections( xmlNodePtr sectionsNode, std::vector<TCompSections> &CompSections )
{
  CompSections.clear();
  if ( !sectionsNode )
    return;
  sectionsNode = sectionsNode->children;
  while ( sectionsNode && string((char*)sectionsNode->name) == "section" ) {
    TCompSections cs;
    cs.name = NodeAsString( sectionsNode );
    cs.firstRowIdx = NodeAsInteger( "@FirstRowIdx", sectionsNode );
    cs.lastRowIdx = NodeAsInteger( "@LastRowIdx", sectionsNode );
    CompSections.push_back( cs );
    sectionsNode = sectionsNode->next;
  }
  ProgTrace( TRACE5, "CompSections.size()=%d", CompSections.size() );
}

void getLayerPlacesCompSection( SALONS2::TSalons &NSalons, TCompSections &compSection,
                                bool only_high_layer, map<ASTRA::TCompLayerType, int> &uselayers_count )
{
  for ( map<ASTRA::TCompLayerType, int>::iterator il=uselayers_count.begin(); il!=uselayers_count.end(); il++ ) {
    uselayers_count[ il->first ] = 0;
  }
  int Idx = 0;
  for ( vector<TPlaceList*>::iterator si=NSalons.placelists.begin(); si!=NSalons.placelists.end(); si++ ) {
    for ( int y=0; y<(*si)->GetYsCount(); y++ ) {
      if ( Idx >= compSection.firstRowIdx && Idx <= compSection.lastRowIdx ) { // внутри секции
        for ( int x=0; x<(*si)->GetXsCount(); x++ ) {
         TPlace *p = (*si)->place( (*si)->GetPlaceIndex( x, y ) );
         if ( !p->isplace || !p->visible )
           continue;
         for ( map<ASTRA::TCompLayerType, int>::iterator il=uselayers_count.begin(); il!=uselayers_count.end(); il++ ) {
           if ( only_high_layer && !p->layers.empty() && p->layers.begin()->layer_type == il->first || // !!!вверху самый приоритетный слой
                !only_high_layer && p->isLayer( il->first ) ) {
             uselayers_count[ il->first ] = uselayers_count[ il->first ] + 1;
             break;
           }
         }
        }
      }
      Idx++;
    }
  }
  for ( map<ASTRA::TCompLayerType, int>::iterator il=uselayers_count.begin(); il!=uselayers_count.end(); il++ ) {
    ProgTrace( TRACE5, "getPaxPlacesCompSection: layer_type=%s, count=%d", EncodeCompLayerType(il->first), il->second );
  }
}

bool ChangeCfg( TSalons &NewSalons, TSalons &OldSalons )
{
  if ( NewSalons.placelists.size() != OldSalons.placelists.size() ) {
    return true;
  }
  for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
        sn=NewSalons.placelists.begin();
        so!=OldSalons.placelists.end(),
        sn!=NewSalons.placelists.end();
        so++, sn++ ) {
    if ( !EqualSalon( *so, *sn, true ) ) {
      return true;
    }
  }
  return false;
}

} // end namespace SALONS2


