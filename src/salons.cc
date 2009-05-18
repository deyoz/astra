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
#include "tripinfo.h"
//#include "seats.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
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
			   (TCompLayerType)l == cltPRLTrzt )
			continue;
	  setFlag( (TCompLayerType)l );
  }
	TQuery Qry(&OraSession);
	Qry.SQLText =
	"SELECT pr_tranz_reg,pr_block_trzt,ckin.get_pr_tranzit(:point_id) as pr_tranzit "
	" FROM trip_sets, points "
	"WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id";
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


TSalons::TSalons( int id, TReadStyle vreadStyle )
{
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
    "SELECT code,name,priority FROM comp_layer_types ORDER BY priority";
  Qry.Execute();
  while ( !Qry.Eof ) {
  	TCompLayerType l = DecodeCompLayerType( Qry.FieldAsString( "code" ) );
  	if ( l != cltUnknown ) {
  		layers_priority[ l ].name = Qry.FieldAsString( "name" );
  	  layers_priority[ l ].priority = Qry.FieldAsInteger( "priority" );
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
    	layers_priority[ cltBlockCent ].func_key = "Ctrl+F7";
    if ( FilterLayers.isFlag( cltTranzit ) ||
    	   FilterLayers.isFlag( cltSOMTrzt ) ||
    	   FilterLayers.isFlag( cltPRLTrzt ) ) {
      layers_priority[ cltBlockTrzt ].name_view = "Транзит";
    }
    layers_priority[ cltCheckin ].name_view = "Регистрация";
    if ( FilterLayers.isFlag( cltProtTrzt ) ) {
    	layers_priority[ cltProtTrzt ].name_view = layers_priority[ cltProtTrzt ].name;
      layers_priority[ cltBlockTrzt ].func_key = "Ctrl+F8";
    }
    if ( FilterLayers.isFlag( cltBlockTrzt ) ) {
    	layers_priority[ cltBlockTrzt ].name_view = layers_priority[ cltBlockTrzt ].name;
      layers_priority[ cltBlockTrzt ].func_key = "Ctrl+F8";
    }
    layers_priority[ cltPNLCkin ].name_view = layers_priority[ cltPNLCkin ].name;
    layers_priority[ cltProtCkin ].name_view = layers_priority[ cltProtCkin ].name;
    layers_priority[ cltProtect ].name_view = layers_priority[ cltProtect ].name;
    if ( FilterLayers.isFlag( cltProtect ) )
      layers_priority[ cltProtect ].func_key = "Ctrl+F5";
    layers_priority[ cltUncomfort ].name_view = layers_priority[ cltUncomfort ].name;
    if ( FilterLayers.isFlag( cltUncomfort ) )
    	layers_priority[ cltUncomfort ].func_key = "Ctrl+F11";
    layers_priority[ cltSmoke ].name_view = layers_priority[ cltSmoke ].name;
    if ( FilterLayers.isFlag( cltSmoke ) )
    	layers_priority[ cltSmoke ].func_key = "Ctrl+F10";
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
      NewTextChild( placeNode, "yname", denorm_iata_row( place->yname ) );
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
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
  xmlNodePtr editNode = NewTextChild( salonsNode, "layers_prop" );
  TReqInfo *r = TReqInfo::Instance();
  for( map<TCompLayerType,TLayerProp>::iterator i=layers_priority.begin(); i!=layers_priority.end(); i++ ) {
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
  SetProp( n, "name_view_help", "Очистить все статусы мест" );
  SetProp( n, "func_key", "Ctrl+F4" );
}

void TSalons::Write()
{
  if ( readStyle == rTripSalons )
    ProgTrace( TRACE5, "TSalons::Write TripSalons with params trip_id=%d",
               trip_id );
  else {
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
                       " UPDATE comps SET airline=:airline,airp=:airp,craft=:craft,bort=:bort,descr=:descr, "\
                       "        time_create=system.UTCSYSDATE,classes=:classes,pr_lat_seat=:pr_lat_seat "\
                       "  WHERE comp_id=:comp_id; "\
                       " DELETE comp_rem WHERE comp_id=:comp_id; "\
                       " DELETE comp_elems WHERE comp_id=:comp_id; "\
                       "END; ";
         break;
      case mAdd:
         Qry.SQLText = "INSERT INTO comps(comp_id,airline,airp,craft,bort,descr,time_create,classes,pr_lat_seat) "\
                       " VALUES(:comp_id,:airline,:airp,:craft,:bort,:descr,system.UTCSYSDATE,:classes,:pr_lat_seat) ";
         break;
      case mDelete:
         Qry.SQLText = "BEGIN "\
                       " UPDATE trip_sets SET comp_id=NULL WHERE comp_id=:comp_id; "\
                       " DELETE comp_rem WHERE comp_id=:comp_id; "\
                       " DELETE comp_elems WHERE comp_id=:comp_id; "\
                       " DELETE comps WHERE comp_id=:comp_id; "\
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

  TQuery RQry( &OraSession );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText = "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "\
                   " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
  }
  else {
    RQry.SQLText = "INSERT INTO comp_rem(comp_id,num,x,y,rem,pr_denial) "\
                   " VALUES(:comp_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
  }

  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "x", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "rem", otString );
  RQry.DeclareVariable( "pr_denial", otInteger );

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
      else
        Qry.SetVariable( "class", place->clname );
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
      	tst();
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
    } //for place
  }
  if ( readStyle == rTripSalons )
    check_waitlist_alarm( trip_id );
}

struct TPaxLayer {
	TCompLayerType layer_type;
	TDateTime time_create;
	int priority;
	int valid; // 0 - не вычислен 1 - true -1 - false
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
  void clearLayer( TSalons *CSalon, int pax_id, vector<TPaxLayer>::iterator &p )
  {
  	for( vector<TSalonPoint>::iterator ipp=p->places.begin(); ipp!=p->places.end(); ipp++ ) { // пробег по местам
  		for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {  // пробег по салонам
  			if ( (*it)->num == ipp->num ) {
  				TPlace* ip = (*it)->place( (*it)->GetPlaceIndex( ipp->x, ipp->y ) );
  		    for( vector<TPlaceLayer>::iterator il=ip->layers.begin(); il!=ip->layers.end(); il++ ) {
  			    if ( il->layer_type == p->layer_type &&
  			    	   il->pax_id == pax_id &&
  			    	   il->time_create == p->time_create ) {
  		  	  	ip->layers.erase( il );
  		  	  	tst();
  		  	  	break;
  		    	}
  		    }
  		  	break;
  		  }
  		}
    }
  }
};

typedef map< int, TPaxLayerRec > TPaxLayers;

bool isValidLayer( TSalons *CSalon, TPaxLayers::iterator &ipax, TPaxLayers pls, vector<TPaxLayer>::iterator &p )
{
  if ( p->valid == -1 || p->places.size() != ipax->second.seats )
  	return false;
  // вычисление случая, когда одно место размечено разными слоями
  int vfirst_x = NoExists;
  int vfirst_y = NoExists;
  int vlast_x = NoExists;
  int vlast_y = NoExists;
  TPlace *ip;
  for( vector<TSalonPoint>::iterator ipp=p->places.begin(); ipp!=p->places.end(); ipp++ ) { // пробег по местам слоя
  	for (vector<TPlaceList*>::iterator it=CSalon->placelists.begin(); it!=CSalon->placelists.end(); it++ ) {
 			if ( (*it)->num == ipp->num ) {
 				ip = (*it)->place( (*it)->GetPlaceIndex( ipp->x, ipp->y ) );
        if ( ipax->second.cl != ip->clname || !ip->isplace || p->places.begin()->num != ip->num ) {
    	    p->valid = -1;
    	    tst();
          return false;
        }
 				break;
 		  }
  	}
    for ( vector<TPlaceLayer>::iterator iplace_layer=ip->layers.begin(); iplace_layer!=ip->layers.end(); iplace_layer++ ) { // пробег по слоям места
    	if ( iplace_layer->layer_type == p->layer_type &&
    		   iplace_layer->time_create == p->time_create )
    		break;
    	if ( iplace_layer->pax_id <= 0 ) // слой более приоритетный и он не принадлежит пассажиру
    		return false;
      TPaxLayers::iterator inpax = pls.find( iplace_layer->pax_id ); // находим пассажира за которым размечено место
      if ( inpax == pls.end() )
      	return false;
      for (vector<TPaxLayer>::iterator ir=inpax->second.paxLayers.begin(); ir!=inpax->second.paxLayers.end(); ir++ ) { // пробег по слоям пассажира
      	if ( ir->layer_type == iplace_layer->layer_type &&
    	  	   ir->time_create == iplace_layer->time_create ) {
    		  if ( isValidLayer( CSalon, inpax, pls, ir ) ) {
	    		  p->valid = -1;
		    	  return false;
		      }
	      }
	      break;
      }
    }
    if ( vfirst_x == NoExists || vfirst_y == NoExists ||
         vfirst_y*1000+vfirst_x > ip->y*1000+ip->x ) {
      vfirst_x = ip->x;
      vfirst_y = ip->y;
    }
    if ( vlast_x == NoExists || vlast_y == NoExists ||
         vlast_y*1000+vlast_x<ip->y*1000+ip->x ) {
      vlast_x=ip->x;
      vlast_y=ip->y;
    }
  }
  if ( vfirst_x == vlast_x && vfirst_y+(int)p->places.size()-1 == vlast_y ||
       vfirst_y == vlast_y && vfirst_x+(int)p->places.size()-1 == vlast_x )
    p->valid = 1;
  else
  	p->valid = -1;
  return ( p->valid == 1 );
}

/*bool ComparePlaceLayers( TPlaceLayer t1, TPlaceLayer t2 )
{
	if ( t1.priority < t2.priority )
		return true;
	else
		if ( t1.priority > t2.priority )
			return false;
		else
			return ( t1.time_create > t2.time_create );
};

void ClearPaxLayer( TPaxLayers &pax_layers, int pax_id, TCompLayerType layer_type, TDateTime time_create )
{
  TPaxLayers::iterator ipax = pax_layers.find( pax_id );
  // поиск нужного слоя
  for (vector<TPaxLayer>::iterator r=ipax->second.paxLayers.begin(); r!=ipax->second.paxLayers.end(); r++ ) { // пробег по слоям пассажира
  	if ( !r->inwork )
   		continue;
  	if ( r->layer_type == layer_type && r->time_create == time_create ) {
      for (vector<TPlace*>::iterator ip=r->places.begin(); ip!=r->places.end(); ip++ ) {
      	(*ip)->clearLayer( layer_type, time_create );
      }
      ProgTrace( TRACE5, "clear layer: pax_id=%d,layer_type=%s, time_create=%f, layer not inwork",
                 pax_id, EncodeCompLayerType( r->layer_type ), r->time_create );
  		r->inwork = false;
  		break;
    }
  }
};*/

void TSalons::Read( )
{
  if ( readStyle == rTripSalons )
  	;
/*    ProgTrace( TRACE5, "TSalons::Read TripSalons with params trip_id=%d, ClassName=%s",
               trip_id, ClName.c_str() );*/
  else {
    ClName.clear();
/*    ProgTrace( TRACE5, "TSalons::Read ComponSalons with params comp_id=%d",
               comp_id );*/
  }
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  TQuery PaxQry( &OraSession );


  if ( readStyle == rTripSalons ) {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  else {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("Компоновка не найдена. Обновите данные");
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
    Qry.SQLText = "SELECT num,x,y,elem_type,xprior,yprior,agle,pr_smoke,not_good,xname,yname,class "
                  " FROM comp_elems "\
                  "WHERE comp_id=:comp_id "
                  "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
  }
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    if ( readStyle == rTripSalons )
      throw UserException( "На рейс не назначен салон" );
    else
      throw UserException( "Не найдена компоновка" );
  tst();
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
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "
                   " WHERE point_id=:point_id "
                   "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
  }
  else {
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM comp_rem "
                   " WHERE comp_id=:comp_id "
                   "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
  }
  RQry.Execute();
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
    ProgTrace( TRACE5, "pax_layers.size()=%d", pax_layers.size() );
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
    // повторение мест!!! - разные слои
    TPlace place;
    point_p.x = Qry.FieldAsInteger( col_x );
    point_p.y = Qry.FieldAsInteger( col_y );
    ProgTrace( TRACE5, "point_p=(%d,%d)", point_p.x, point_p.y );
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
      while ( !RQry.Eof && RQry.FieldAsInteger( col_num ) == num &&
              RQry.FieldAsInteger( col_x ) == place.x &&
              RQry.FieldAsInteger( col_y ) == place.y ) {
        TRem rem;
        rem.rem = RQry.FieldAsString( "rem" );
        rem.pr_denial = RQry.FieldAsInteger( "pr_denial" );
        place.rems.push_back( rem );
        RQry.Next();
      }
      if ( ClName.find( Qry.FieldAsString( col_class ) ) == string::npos )
        ClName += Qry.FieldAsString(col_class );
    }
    else { // это место проинициализировано - это новый слой
    	tst();
    	place = *placeList->place( point_p );
    	tst();
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
      	ProgTrace( TRACE5, "seat_no=%s, pax_id=%d", string(string(Qry.FieldAsString("yname"))+Qry.FieldAsString("xname")).c_str(), pax_id );
      	if ( PlaceLayer.layer_type != cltUnknown ) { // слои сортированы по приоритету, первый - самый приоритетный слой в векторе
      		tst();
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
          	ipl->second.paxLayers.push_back(TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
          	                                           layers_priority[ PlaceLayer.layer_type ].priority,
          	                                           TSalonPoint( point_p.x, point_p.y, placeList->num ) )); // слой не найден, создаем новый слой
         }
      }
      else { // пассажир не найден, создаем пассажира и слой
       	pax_layers[ pax_id ].paxLayers.push_back( TPaxLayer( PlaceLayer.layer_type, PlaceLayer.time_create,
       	                                                     layers_priority[ PlaceLayer.layer_type ].priority,
       	                                                     TSalonPoint( point_p.x, point_p.y, placeList->num ) ));
      }
    }
  }	/* end for */
  if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
  	ProgTrace( TRACE5, "placeList->num=%d", placeList->num );
    placelists.pop_back( );
    delete placeList; // нам этот класс/салон не нужен
  }
  // имеем салон и места в салоне со всеми слоями. Нам предстоит разобраться какие из них лишние. До этого мы фильтровали только те, которые не использвем
  for( TPaxLayers::iterator ipax=pax_layers.begin(); ipax!=pax_layers.end(); ipax++ ) { // пробег по пассажирам
  	for (vector<TPaxLayer>::iterator r=ipax->second.paxLayers.begin(); r!=ipax->second.paxLayers.end(); r++ ) { // пробег по слоям пассажира
  			ProgTrace( TRACE5, "pax_id=%d, seats=%d, class=%s,layer_type=%s, time_create=%f, r->places.size()=%d",
  			           ipax->first, ipax->second.seats, ipax->second.cl.c_str(), EncodeCompLayerType( r->layer_type ), r->time_create, r->places.size() );

  		if ( !isValidLayer( this, ipax, pax_layers, r )  ) {
  			ipax->second.clearLayer( this, ipax->first, r );
  		}
  		else { // слой правильный
        // вычисление случая, когда разные места размечены под одного пассажира (разные слои)
        for (vector<TPaxLayer>::iterator ir=ipax->second.paxLayers.begin(); ir!=ipax->second.paxLayers.end(); ir++ ) { // пробег по слоям пассажира
	        // пассажир может занимать разные места с одним слоем???
	        if ( !isValidLayer( this, ipax, pax_layers, ir ) || ir == r )
	  	      continue;
	        if ( ir->priority < r->priority ||
         	     ir->priority == r->priority &&
	  	         ir->time_create > r->time_create ) { //есть более приоритетный слой у пассажира
  	    		ipax->second.clearLayer( this, ipax->first, r );
	  	      r->valid = -1;
	  	      break;
	        }
	      }
  	  }
			if ( r->valid == 1 )
 			  ProgTrace( TRACE5, "layer ok" );
 			else
  			ProgTrace( TRACE5, "invalid layer, result=%d", r->valid );
    }
  }



/*

  bool pr_valid;
  for ( std::vector<TPlaceList*>::iterator iplaces = placelists.begin(); iplaces != placelists.end(); iplaces++ ) {
  	for ( TPlaces::iterator ip= iplaces->places.begin(); ip!=iplaces->places.end(); ip++ ) {
  		if ( !ip->isPax )
  			continue;
  		for ( vector<TPlaceLayer>::iterator il=ip->layers.begin(); il!=ip->layers.end(); il++ ) {
  			if ( il->pax_id <= 0 )
  				continue;
  			// имеем пассажирский слой наиболее приоритетный
  			// определяем правильность этого слоя
  			TPaxLayers::iterator ipax = pax_layers.find( l->pax_id ); // список всех слоев пассажира
  			for (vector<TPaxLayer>::iterator r=ipax->second.paxLayers.begin(); r!=ipax->second.paxLayers.end(); r++ ) { // пробег по слоям пассажира
  				if ( !r->inwork )
      		  continue;
  				if ( r->layer_type == il->layer_type && r->time_create == il->time_create ) {
           if ( pr_valid = r->isValid( ipax->second.seats, ipax->second.cl, p->first ) ) { // если проверка прошла успешно, то надо удалить все остальные слои у места по пассажирам
      			tst();
      			for( vector<TPlaceLayer>::iterator l1=p->second.begin();l1!=p->second.end(); l1++ ) {
      				if ( l1 != l ) {
      					ClearPaxLayer( pax_layers, l1->pax_id, l1->layer_type, l1->time_create );
      				}
      			}
      		}
      		else {
      			ClearPaxLayer( pax_layers, l->pax_id, l->layer_type, l->time_create );
      		}
          r->inwork = false;
      		break;
        }
  				}
  		  }
  		}
  	}
  }

  for (TPlacePaxs::iterator p=PaxsOnPlaces.begin(); p!=PaxsOnPlaces.end(); p++ ) { // пробег по всем местам на которых есть pax_id
    if ( p->second.size() > 1 ) {
  		sort( p->second.begin(), p->second.end(), ComparePlaceLayers );
    }
    ProgTrace( TRACE5, "placename=%s, p->second.size()=%d", string(p->first->yname+p->first->xname).c_str(), p->second.size() );
    for (vector<TPlaceLayer>::iterator l=p->second.begin();l!=p->second.end(); l++) { // пробег по слоям места
    	ProgTrace( TRACE5, "pax_id=%d, layer_type=%s, priority=%d", l->pax_id, EncodeCompLayerType( l->layer_type ), l->priority );
      TPaxLayers::iterator ipax = pax_layers.find( l->pax_id );
      // поиск нужного слоя
      ProgTrace( TRACE5, "ipax find=%d, ipax->second.paxLayers.size()=%d", ipax != pax_layers.end(), ipax->second.paxLayers.size() );
      for (vector<TPaxLayer>::iterator r=ipax->second.paxLayers.begin(); r!=ipax->second.paxLayers.end(); r++ ) { // пробег по слоям пассажира
      	ProgTrace( TRACE5, "pax_id=%d, l->layer_type=%s, l->time_create=%f, r->layer_type=%s, r->time_create=%f, inwork=%d",
      	           l->pax_id, EncodeCompLayerType( l->layer_type ), l->time_create, EncodeCompLayerType( r->layer_type ), r->time_create, r->inwork );
      	if ( !r->inwork )
      		continue;
      	tst();
      	if ( r->layer_type == l->layer_type && r->time_create == l->time_create ) {
      		if ( pr_valid = r->isValid( ipax->second.seats, ipax->second.cl, p->first ) ) { // если проверка прошла успешно, то надо удалить все остальные слои у места по пассажирам
      			tst();
      			for( vector<TPlaceLayer>::iterator l1=p->second.begin();l1!=p->second.end(); l1++ ) {
      				if ( l1 != l ) {
      					ClearPaxLayer( pax_layers, l1->pax_id, l1->layer_type, l1->time_create );
      				}
      			}
      		}
      		else {
      			ClearPaxLayer( pax_layers, l->pax_id, l->layer_type, l->time_create );
      		}
          r->inwork = false;
      		break;
        }
      }
      // со всеми слоями у места разобрались, переходим на новое место
      if ( pr_valid ) // если нашли нормальный слой, то обработка места закончилась, иначе переходим на другой слой у места
        break;
    }
  }*/
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
      	tst();
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
      		throw UserException( string( "Ремарка " ) + rem_name + " не может быть задана в классе " + place->clname );
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
    throw Exception( "place index out of range" );
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
    throw Exception( "Неправильные координаты места" );
  return ys[ p.y ] + xs[ p.x ];
}

string TPlaceList::GetXsName( int x )
{
  if ( x < 0 || x >= GetXsCount() ) {
    throw Exception( "Неправильные x координата места" );
  }
  return xs[ x ];
}

string TPlaceList::GetYsName( int y )
{
  if ( y < 0 || y >= GetYsCount() )
    throw Exception( "Неправильные y координата места" );
  return ys[ y ];
}

bool TPlaceList::GetisPlaceXY( string placeName, TPoint &p )
{
	placeName = trim( placeName );
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
    	salon_seat_no = denorm_iata_row(*iy) + denorm_iata_line(*ix,false);
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
  if ( pl.x >= (int)xs.size() )
    xs.resize( pl.x + 1, "" );
  if ( !pl.xname.empty() )
    xs[ pl.x ] = pl.xname;
  if ( pl.y >= (int)ys.size() )
    ys.resize( pl.y + 1, "" );
  if ( !pl.yname.empty() )
    ys[ pl.y ] = pl.yname;
  if ( (int)xs.size()*(int)ys.size() > (int)places.size() ) {
    places.resize( (int)xs.size()*(int)ys.size() );
  }
  int idx = GetPlaceIndex( pl.x, pl.y );
  if ( pl.xprior >= 0 && pl.yprior >= 0 ) {
    TPoint p( pl.xprior, pl.yprior );
    place( p )->xnext = pl.x;
    place( p )->ynext = pl.y;
  }
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
    "SELECT airp,airline,flt_no,suffix,craft,bort,scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    "FROM points "
    "WHERE point_id=:point_id ";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

  TTripInfo info;
  info.airline=Qry.FieldAsString("airline");
  info.flt_no=Qry.FieldAsInteger("flt_no");
  info.suffix=Qry.FieldAsString("suffix");
  info.airp=Qry.FieldAsString("airp");
  info.scd_out=Qry.FieldAsDateTime("scd_out");
  info.real_out=Qry.FieldAsDateTime("real_out");

  NewTextChild( dataNode, "trip", GetTripName(info) );
  NewTextChild( dataNode, "craft", Qry.FieldAsString( "craft" ) );
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
  if (Qry.Eof) throw UserException("Рейс не найден. Обновите данные");

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
  NewTextChild( dataNode, "craft", Qry.FieldAsString( "craft" ) );
  NewTextChild( dataNode, "bort", Qry.FieldAsString( "bort" ) );
  NewTextChild( dataNode, "comp_id", comp_id );
  NewTextChild( dataNode, "descr", Qry.FieldAsString( "descr" ) );
}


bool InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull )
{
  TQuery Qry( &OraSession );
  //!!!
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
   "SELECT comp_id, bort, airline, airp, f, c, y FROM "
   " ( SELECT comp_elems.comp_id, bort, airline, airp, "
   "           NVL( SUM( DECODE( class, 'П', 1, 0 ) ), 0 ) as f, "
   "           NVL( SUM( DECODE( class, 'Б', 1, 0 ) ), 0 ) as c, "
   "           NVL( SUM( DECODE( class, 'Э', 1, 0 ) ), 0 ) as y "
   "      FROM comp_elems, comp_elem_types, comps "
   "     WHERE comp_elems.elem_type = comp_elem_types.code AND comp_elem_types.pr_seat <> 0 AND "
   "           comp_elems.comp_id = comps.comp_id AND comps.craft = :craft "
   "    GROUP BY comp_elems.comp_id,bort, airline, airp ) "
   " WHERE f - :vf >= 0 AND "
   "       c - :vc >= 0 AND "
   "       y - :vy >= 0 AND "
   "       f < 1000 AND c < 1000 AND y < 1000 "
   "ORDER BY comp_id";
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
  catch( Exception &e ) {
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
      " WHERE crs_data.point_id=tlg_binding.point_id_tlg AND point_id_spp=:point_id AND target=:airp ";
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
      "SELECT airp_arv AS target,class, "
      "       0 AS priority, "
      "       crs_ok + crs_tranzit AS c "
      " FROM crs_counters "
      "WHERE point_dep=:point_id "
      "UNION "
      "SELECT target,class,1,resa + tranzit "
      " FROM trip_data "
      "WHERE point_id=:point_id "
      "ORDER BY target,class,priority DESC ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    string target, vclass;
    f = 0; c = 0; y = 0;
    while ( !Qry.Eof ) {
    	if ( target.empty() ||
    		   vclass.empty() ||
    		   target != Qry.FieldAsString( "target" ) ||
    		   vclass != Qry.FieldAsString( "class" ) ) {
    		target = Qry.FieldAsString( "target" );
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
    "UPDATE trip_sets SET comp_id = :comp_id, pr_lat_seat = :pr_lat_seat WHERE point_id = :point_id; "
    "END;";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
  Qry.Execute();
  InitVIP( point_id );
  setTRIP_CLASSES( point_id );
  Qry.Clear();
  Qry.SQLText =
   "SELECT ckin.get_classes(:point_id) cl FROM dual ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  TReqInfo::Instance()->MsgToLog( string( "Назначена базовая компоновка (ид=" ) + IntToString( comp_id ) +
  	                              "). Классы: " + Qry.FieldAsString( "cl" ), evtFlt, point_id );
  check_waitlist_alarm( point_id );
  return comp_id;
}

void InitVIP( int point_id )
{
	tst();
	// инициализация - разметра салона по умолчани
	TQuery Qry(&OraSession);
	TQuery QryVIP(&OraSession);
	Qry.SQLText =
	  "SELECT airline,flt_no,airp FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  string airline = Qry.FieldAsString( "airline" );
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  string airp = Qry.FieldAsString( "airp" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT pr_misc, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    " FROM misc_set "
    " WHERE type=1 AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp) "
    " ORDER BY priority DESC ";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.CreateVariable( "flt_no", otInteger, flt_no );
  Qry.CreateVariable( "airp", otString, airp );
  Qry.Execute();
  if ( Qry.Eof || !Qry.FieldAsInteger( "pr_misc" ) )
  	return; // разметра не нужна
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
		throw UserException( "Изменена компоновка рейса. Обновите данные" );
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


} // end namespace SALONS2


