#include <stdlib.h>
#include "salons2.h"
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
#include "alarms.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;
using namespace ASTRA;

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
				if ( SALONS2::point_dep_AND_layer_type_FOR_TRZT_SOM_PRL( point_id, point_dep, layer_tlg ) ) {
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


string DecodeLayer( const std::string &layer )
{
	switch( DecodeCompLayerType( (char*)layer.c_str() ) ) {
		case cltProtCkin:
			return "PS";
    case cltPNLCkin:
    	return "BR";
    case cltPRLTrzt:
    case cltSOMTrzt:
    case cltProtTrzt:
    case cltBlockTrzt:
    	return "TR";
    case cltBlockCent:
    	return "BL";
    case cltProtect:
    	return "RZ";
    default: return "";
  }
}

string EncodeLayer( const std::string &int_layer, TFilterLayers &FilterLayers )
{
	if ( int_layer == "BL" )
		return EncodeCompLayerType( cltBlockCent );
	else
		if ( int_layer == "RZ" )
			return  EncodeCompLayerType( cltProtect );
		else
			if ( int_layer == "TR" ) {
				if ( FilterLayers.isFlag( cltBlockTrzt ) )
					return EncodeCompLayerType( cltBlockTrzt );
				else
					if ( FilterLayers.isFlag( cltProtTrzt ) )
  		      return EncodeCompLayerType( cltProtTrzt );
  		    else
  		    	return "";
  		}
		  else return "";
}

void TSalons::Clear( )
{
  FCurrPlaceList = NULL;
  for ( std::vector<TPlaceList*>::iterator i = placelists.begin(); i != placelists.end(); i++ ) {
    delete *i;
  }
  placelists.clear();
}


TSalons::TSalons( int id, SALONS2::TReadStyle vreadStyle )
{
	readStyle = vreadStyle;
	if ( readStyle == SALONS2::rComponSalons )
		comp_id = id;
	else
	  trip_id = id;
	pr_lat_seat = false;
  FCurrPlaceList = NULL;
  modify = mNone;
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT code, priority FROM comp_layer_types ORDER BY priority";
  Qry.Execute();
  tst();
  string status;
  while ( !Qry.Eof ) {
  	ProgTrace( TRACE5, "code=%s", Qry.FieldAsString( "code" ) );
  	status = DecodeLayer( Qry.FieldAsString( "code" ) );
  	ProgTrace( TRACE5, "status=%s", status.c_str() );
  	if ( !status.empty() && status_priority.find( status ) == status_priority.end() )
  	  status_priority[ status ] = Qry.FieldAsInteger( "priority" );
  	TLayerPriority lp;
  	lp.layer = Qry.FieldAsString( "code" );
  	lp.code = status;
  	lp.priority = Qry.FieldAsInteger( "priority" );
  	layer_priority.push_back( lp );
  	Qry.Next();
  }
  if ( readStyle == SALONS2::rTripSalons ) {
    FilterLayers.getFilterLayers( trip_id ); // определение режима учета транзитных слоев
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
      if ( !place->isplace )
        NewTextChild( placeNode, "isnotplace" );
      if ( place->xprior != -1 )
        NewTextChild( placeNode, "xprior", place->xprior );
      if ( place->yprior != -1 )
        NewTextChild( placeNode, "yprior", place->yprior );
      if ( place->agle )
        NewTextChild( placeNode, "agle", place->agle );
      NewTextChild( placeNode, "class", place->clname );
      if ( place->pr_smoke )
        NewTextChild( placeNode, "pr_smoke" );
      if ( place->not_good )
        NewTextChild( placeNode, "not_good" );
      NewTextChild( placeNode, "xname", denorm_iata_line( place->xname, pr_lat_seat ) );
      NewTextChild( placeNode, "yname", denorm_iata_row( place->yname, NULL ) );
      if ( place->status != "FP" )
        NewTextChild( placeNode, "status", place->status ); // вычисляем статус исходя из слоев
      if ( !place->pr_free )
        NewTextChild( placeNode, "pr_notfree" );
      if ( place->block )
        NewTextChild( placeNode, "block" );
      xmlNodePtr remsNode = NULL;
      xmlNodePtr remNode;
      for ( vector<SALONS2::TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
        if ( !remsNode ) {
          remsNode = NewTextChild( placeNode, "rems" );
        }
        remNode = NewTextChild( remsNode, "rem" );
        NewTextChild( remNode, "rem", rem->rem );
        if ( rem->pr_denial )
          NewTextChild( remNode, "pr_denial" );
      }
      if ( place->layers.size() > 0 ) {
      	remsNode = NewTextChild( placeNode, "layers" );
      	for( std::vector<std::string>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
      		remNode = NewTextChild( remsNode, "layer" );
      		NewTextChild( remNode, "layer_type", *l );
      	}
      }
    }
    SetProp( placeListNode, "xcount", xcount + 1 );
    SetProp( placeListNode, "ycount", ycount + 1 );
  }
}

void TSalons::Write()
{
  if ( readStyle == SALONS2::rTripSalons )
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

  if ( readStyle == SALONS2::rTripSalons ) {
    Qry.SQLText = "BEGIN "\
                  " UPDATE points SET point_id=point_id WHERE point_id=:point_id; "
                  " UPDATE trip_sets SET pr_lat_seat=:pr_lat_seat WHERE point_id=:point_id; "
                  " DELETE trip_comp_rem WHERE point_id=:point_id; "
                  " DELETE trip_comp_baselayers WHERE point_id=:point_id; "
                  " DELETE trip_comp_elems WHERE point_id=:point_id; "
                  " DELETE trip_comp_layers "
                  " WHERE point_id=:point_id AND layer_type IN ( :tranzit_layer, :blockcent_layer, :prot_layer );"
//                  layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
                  "END;";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
    // тут возможно 2 варианта слоя PROT_TRZT OR BLOCK_TRZT, надо определить какой слой надо удалить
    if ( FilterLayers.isFlag( cltBlockTrzt ) )
      Qry.CreateVariable( "tranzit_layer", otString, EncodeCompLayerType( cltBlockTrzt ) );
    else
    	Qry.CreateVariable( "tranzit_layer", otString, EncodeCompLayerType( cltProtTrzt ) );
    Qry.CreateVariable( "blockcent_layer", otString, EncodeCompLayerType( cltBlockCent ) );
    Qry.CreateVariable( "prot_layer", otString, EncodeCompLayerType( cltProtect ) );
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
                       " DELETE comp_rem WHERE comp_id=:comp_id; "
                       " DELETE comp_baselayers WHERE comp_id=:comp_id; "\
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
  }
  Qry.Execute();
  if ( readStyle == SALONS2::rComponSalons && modify == mDelete )
    return; /* удалили компоновку */

  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );
  if ( readStyle == SALONS2::rTripSalons ) {
    RQry.SQLText =
      "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
      " VALUES(:point_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
    LQry.SQLText =
      "INSERT INTO trip_comp_baselayers(point_id,num,x,y,layer_type) "
      " VALUES(:point_id,:num,:x,:y,:layer_type)";
    LQry.DeclareVariable( "point_id", otInteger );
    LQry.SetVariable( "point_id", trip_id );
  }
  else {
    RQry.SQLText =
      "INSERT INTO comp_rem(comp_id,num,x,y,rem,pr_denial) "
      " VALUES(:comp_id,:num,:x,:y,:rem,:pr_denial)";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
    LQry.SQLText =
      "INSERT INTO comp_baselayers(comp_id,num,x,y,layer_type) "
      " VALUES(:comp_id,:num,:x,:y,:layer_type)";
    LQry.DeclareVariable( "comp_id", otInteger );
    LQry.SetVariable( "comp_id", comp_id );
  }

  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "x", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "rem", otString );
  RQry.DeclareVariable( "pr_denial", otInteger );
  
  LQry.DeclareVariable( "num", otInteger );
  LQry.DeclareVariable( "x", otInteger );
  LQry.DeclareVariable( "y", otInteger );
  LQry.DeclareVariable( "layer_type", otString );

  Qry.Clear();
  if ( readStyle == SALONS2::rTripSalons ) {
    Qry.SQLText =
      "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class,xname,yname) "
      " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class,:xname,:yname) ";
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
      if ( place->clname.empty() || !ispl[ place->elem_type ] )
        Qry.SetVariable( "class", FNull );
      else {
        Qry.SetVariable( "class", place->clname );
        cl = DecodeClass( place->clname.c_str() );
        if ( cl != NoClass )
          countersClass[ cl ] = countersClass[ cl ] + 1;
      }
     	if ( place->pr_smoke ) {
        LQry.SetVariable( "x", place->x );
        LQry.SetVariable( "y", place->y );
        LQry.SetVariable( "layer_type", EncodeCompLayerType( cltSmoke ) );
        LQry.Execute();
      }
     	if ( place->not_good ) {
        LQry.SetVariable( "x", place->x );
        LQry.SetVariable( "y", place->y );
        LQry.SetVariable( "layer_type", EncodeCompLayerType( cltUncomfort ) );
        LQry.Execute();
      }
      Qry.SetVariable( "xname", place->xname );
      Qry.SetVariable( "yname", place->yname );
      Qry.Execute();
      if ( !place->rems.empty() ) {
        RQry.SetVariable( "x", place->x );
        RQry.SetVariable( "y", place->y );
        for( vector<SALONS2::TRem>::iterator rem = place->rems.begin(); rem != place->rems.end(); rem++ ) {
          RQry.SetVariable( "rem", rem->rem );
          if ( !rem->pr_denial )
            RQry.SetVariable( "pr_denial", 0 );
          else
            RQry.SetVariable( "pr_denial", 1 );
          RQry.Execute();
        }
      }

      if ( !place->layers.empty() ) {
      	//!надо вставить слой
      	QryLayers.SetVariable( "first_xname", place->xname );
      	QryLayers.SetVariable( "last_xname", place->xname );
      	QryLayers.SetVariable( "first_yname", place->yname );
      	QryLayers.SetVariable( "last_yname", place->yname );
      	for ( vector<string>::iterator l=place->layers.begin(); l!=place->layers.end(); l++ ) {
      		QryLayers.SetVariable( "layer_type", *l );
      		QryLayers.Execute();
      	}
      }
    } //for place
  }
  // сохраняем конфигурацию мест
  if ( readStyle != SALONS2::rTripSalons ) {
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
  if ( readStyle == SALONS2::rTripSalons )
    check_waitlist_alarm( trip_id );
}

  struct TPlaceLayer {
  	TPlaceList *placelist;
  	int x, y;
  	string layer;
  	int priority;
  	TPlaceLayer() {
  		layer = "FP";
  		priority = INT_MAX;
  		placelist = NULL;
  		x = -1;
  		y = -1;
  	}
  };

void TSalons::Read( bool wo_invalid_seat_no )
{
  if ( readStyle == SALONS2::rTripSalons );
  else {
    ClName.clear();
  }
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  TQuery LQry( &OraSession );

  if ( readStyle == SALONS2::rTripSalons ) {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.Execute();
    if ( Qry.Eof ) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  else {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.Execute();
    if ( Qry.Eof ) throw AstraLocale::UserException("MSG.SALONS.NOT_FOUND.REFRESH_DATA");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  Qry.Clear();

  if ( readStyle == SALONS2::rTripSalons ) {
  	string sql_text =
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.xname,t.yname,t.class,r.layer_type, "
      "                NVL(l.pax_id, l.crs_pax_id) pax_id, l.point_dep "
      " FROM trip_comp_elems t, trip_comp_ranges r, trip_comp_layers l "
      "WHERE t.point_id=:point_id AND "
      "      t.point_id=r.point_id(+) AND "
      "      t.num=r.num(+) AND "
      "      t.x=r.x(+) AND "
      "      t.y=r.y(+) AND "
      "      r.range_id=l.range_id(+) ";
    if ( wo_invalid_seat_no )
    	sql_text +=
    	  " MINUS "
        "SELECT DISTINCT t1.num,t1.x,t1.y,t1.elem_type,t1.xprior,t1.yprior,t1.agle, "
        "                t1.xname,t1.yname,t1.class,r.layer_type, "
        "                NVL(l.pax_id, l.crs_pax_id) pax_id, l.point_dep "
        " FROM trip_comp_layers l, pax, pax_grp, "
        "      trip_comp_ranges r, trip_comp_elems t1 "
        "WHERE l.point_id=:point_id AND "
        "      l.layer_type=:layer_type AND "
        "      l.pax_id=pax.pax_id AND "
        "      r.point_id=l.point_id AND "
        "      r.range_id=l.range_id AND "
        "      t1.point_id=r.point_id AND "
        "      t1.num=r.num AND "
        "      t1.x=r.x AND "
        "      t1.y=r.y AND "
        "      pax_grp.point_dep=:point_id AND "
        "      pax.grp_id=pax_grp.grp_id AND "
        "      salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) IS NULL ";
    sql_text += " ORDER BY num, x desc, y desc ";
    Qry.SQLText = sql_text;
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    if ( wo_invalid_seat_no )
      Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType(cltCheckin) );
  }
  else {
    Qry.SQLText = "SELECT num,x,y,elem_type,xprior,yprior,agle,xname,yname,class "
                  " FROM comp_elems "
                  "WHERE comp_id=:comp_id "
                  "ORDER BY num, x desc, y desc ";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
  }
  Qry.Execute();
  if ( Qry.RowCount() == 0 )
    if ( readStyle == SALONS2::rTripSalons )
      throw AstraLocale::UserException( "MSG.FLIGHT_WO_CRAFT_CONFIGURE" );
    else
      throw AstraLocale::UserException( "MSG.SALONS.NOT_FOUND" );
  tst();
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
  if ( readStyle == SALONS2::rTripSalons ) {
    RQry.SQLText =
      "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
    LQry.SQLText =
      "SELECT num,x,y,layer_type FROM trip_comp_baselayers "
      " WHERE point_id=:point_id "
      "ORDER BY num, x desc, y desc ";
    LQry.DeclareVariable( "point_id", otInteger );
    LQry.SetVariable( "point_id", trip_id );
  }
  else {
    RQry.SQLText =
      "SELECT num,x,y,rem,pr_denial FROM comp_rem "
      " WHERE comp_id=:comp_id "
      "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
    LQry.SQLText =
      "SELECT num,x,y,layer_type FROM comp_baselayers "
      " WHERE comp_id=:comp_id "
      "ORDER BY num, x desc, y desc ";
    LQry.DeclareVariable( "comp_id", otInteger );
    LQry.SetVariable( "comp_id", comp_id );
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
  
  string ClName = ""; /* перечисление всех классов, которые есть в салоне */
  TPlaceList *placeList = NULL;
  int num = -1;
  SALONS2::TPoint point_p;
  int pax_id;
  map<int,TPlaceLayer> mp;
  map<int,TPlaceLayer>::iterator imp;
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
    if ( readStyle != SALONS2::rTripSalons || Qry.FieldIsNULL( "pax_id" ) )
    	pax_id = -1;
    else
    	pax_id = Qry.FieldAsInteger( "pax_id" );
    // если место еще не определено или место есть, но не проинициализировано
    if ( !placeList->ValidPlace( point_p ) || placeList->place( point_p )->x == -1 ) {
    	place.x = point_p.x;
    	place.y = point_p.y;
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
      place.xname = Qry.FieldAsString( col_xname );
      place.yname = Qry.FieldAsString( col_yname );
      while ( !RQry.Eof && RQry.FieldAsInteger( rem_col_num ) == num &&
              RQry.FieldAsInteger( rem_col_x ) == place.x &&
              RQry.FieldAsInteger( rem_col_y ) == place.y ) {
        SALONS2::TRem rem;
        rem.rem = RQry.FieldAsString( rem_col_rem );
        rem.pr_denial = RQry.FieldAsInteger( rem_col_pr_denial );
        place.rems.push_back( rem );
        RQry.Next();
      }
      place.pr_smoke = false;
      place.not_good = false;
      while ( !LQry.Eof && LQry.FieldAsInteger( baselayer_col_num ) == num &&
              LQry.FieldAsInteger( baselayer_col_x ) == place.x &&
              LQry.FieldAsInteger( baselayer_col_y ) == place.y ) {
        place.pr_smoke = place.pr_smoke || ( DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) ) == cltSmoke );
        place.not_good = place.not_good || ( DecodeCompLayerType( LQry.FieldAsString( baselayer_col_layer_type ) ) == cltUncomfort );
        LQry.Next();
      }
      if ( ClName.find( Qry.FieldAsString( col_class ) ) == string::npos )
        ClName += Qry.FieldAsString( col_class );
    }
    else { // это место проинициализировано - это новый слой
    	place = *placeList->place( point_p );
    }
    if ( readStyle == SALONS2::rTripSalons ) { // здесь работа со всеми слоями для выявления разных признаков
      if ( FilterLayers.CanUseLayer( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ), Qry.FieldAsInteger( "point_dep" ) ) ) { // этот слой используем
//      	ProgTrace( TRACE5, "seat_no=%s", string(string(Qry.FieldAsString("yname"))+Qry.FieldAsString("xname")).c_str() );
        SALONS::SetLayer( this->status_priority, Qry.FieldAsString( "layer_type" ), place );
        SALONS::SetFree( Qry.FieldAsString( "layer_type" ), place );
        SALONS::SetBlock( Qry.FieldAsString( "layer_type" ), place );
        if ( pax_id > 0 ) {
        	int priority = -1;
          for (vector<TLayerPriority>::iterator ipr=layer_priority.begin(); ipr!=layer_priority.end(); ipr++ ) {
          	if ( ipr->layer == Qry.FieldAsString( "layer_type" ) ) {
          		priority = ipr->priority;
          		break;
          	}
          }
          if ( priority >= 0 ) {
          	//    		ProgTrace( TRACE5, "pax_id=%d, layer=%s, mp[ pax_id ].priority=%d, priority=%d, place.x=%d, place.y=%d",
          	//    		           pax_id, Qry.FieldAsString( "layer_type" ), mp[ pax_id ].priority, priority, place.x, place.y );
          	if ( mp[ pax_id ].priority > priority ) {
          		if ( mp[ pax_id ].placelist ) {
          			SALONS2::TPoint p(mp[ pax_id ].x,mp[ pax_id ].y);
          			SALONS::ClearLayer( this->status_priority, mp[ pax_id ].layer, *mp[ pax_id ].placelist->place( p ) );
          			}
          			mp[ pax_id ].placelist = placeList;
          			mp[ pax_id ].x = place.x;
          			mp[ pax_id ].y = place.y;
          			mp[ pax_id ].layer = Qry.FieldAsString( "layer_type" );
          			mp[ pax_id ].priority = priority;
          	}
          	else
          		if ( mp[ pax_id ].priority < priority ) {
          			SALONS::ClearLayer( this->status_priority, Qry.FieldAsString( "layer_type" ), place );
          		}
          }
      	}
      }
    }
    else
      place.block = 0;
    place.visible = true;
    placeList->Add( place );
  }	/* end for */
  if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
    placelists.pop_back( );
    delete placeList; // нам этот класс/салон не нужен
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
  SALONS2::TRem rem;
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
      place.pr_smoke = GetNodeFast( "pr_smoke", node );
      place.not_good = GetNodeFast( "not_good", node );
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
      if ( !GetNodeFast( "status", node ) )
        place.status = "FP";
      else
        place.status = NodeAsStringFast( "status", node );
      place.pr_free = !GetNodeFast( "pr_notfree", node );
      place.block = GetNodeFast( "block", node );

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
      		//???string l = EncodeLayer( NodeAsStringFast( "layer_type", remNode ) );
/*new version      		if ( !l.empty() )
      		  place.layers.push_back( l ); */
      		remsNode = remsNode->next;
      	}
      }
      else { //old version
      		string l = EncodeLayer( place.status, FilterLayers );
      		if ( !l.empty()  )
      		  place.layers.push_back( l );
      		if ( place.block ) {
      			place.layers.clear();
      			place.layers.push_back( EncodeLayer( "BL", FilterLayers ) );
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
      for ( vector<SALONS2::TRem>::iterator irem=place->rems.begin(); irem!=place->rems.end(); irem++ ) {
      	if ( irem->rem == rem_name )
      		throw AstraLocale::UserException( "MSG.SALONS.NOT_FOUND", LParams() << LParam("remark", rem_name) << LParam("class", place->clname));
      }
    }
  }
}

void TPlace::Assign( TPlace &pl )
{
  selected = pl.selected;
  visible = pl.visible;
  x = pl.x;
  y = pl.y;
  elem_type = pl.elem_type;
  isplace = pl.isplace;
  xprior = pl.xprior;
  yprior = pl.yprior;
  xnext = pl.xnext;
  ynext = pl.ynext;
  agle = pl.agle;
  clname = pl.clname;
  pr_smoke = pl.pr_smoke;
  not_good = pl.not_good;
  xname = pl.xname;
  yname = pl.yname;
  status = pl.status;
  pr_free = pl.pr_free;
  block = pl.block;
  rems.clear();
  rems = pl.rems;
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

TPlace *TPlaceList::place( SALONS2::TPoint &p )
{
  return place( GetPlaceIndex( p ) );
}

int TPlaceList::GetPlaceIndex( SALONS2::TPoint &p )
{
  return GetPlaceIndex( p.x, p.y );
}

int TPlaceList::GetPlaceIndex( int x, int y )
{
  return GetXsCount()*y + x;
}

bool TPlaceList::ValidPlace( SALONS2::TPoint &p )
{
 return ( p.x < GetXsCount() && p.x >= 0 && p.y < GetYsCount() && p.y >= 0 );
}

string TPlaceList::GetPlaceName( SALONS2::TPoint &p )
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

bool TPlaceList::GetisPlaceXY( string placeName, SALONS2::TPoint &p )
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
    SALONS2::TPoint p( pl.xprior, pl.yprior );
    place( p )->xnext = pl.x;
    place( p )->ynext = pl.y;
  }
  places[ idx ] = pl;
}


namespace SALONS
{

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

void SetLayer( const std::map<std::string,int> &status_priority, const std::string &layer, TPlace &pl )
{
	if ( layer.empty() )
		return;
  const map<string,int>::const_iterator n = status_priority.find( DecodeLayer( layer ) );
  const map<string,int>::const_iterator p = status_priority.find( pl.status );

  if ( n != status_priority.end() ) {
  	 if ( p == status_priority.end() || n->second < p->second )
  	 	pl.status = n->first;
  }
  pl.layers.push_back( layer );
}
void ClearLayer( const std::map<std::string,int> &status_priority, const std::string &layer, TPlace &pl )
{
	if ( layer.empty() )
		return;
  vector<string>::iterator il = find( pl.layers.begin(), pl.layers.end(), layer );
  if ( il != pl.layers.end() ) {
  	pl.status = "FP";
  	tst();
  	pl.layers.erase( il );
  	tst();
  	vector<string> layers = pl.layers;
  	pl.layers.clear();
  	for ( vector<string>::iterator i=layers.begin(); i!=layers.end(); i++ ) {
  		SetLayer( status_priority, *i, pl );
  	}
  }
}

void SetFree( const std::string &layer, TPlace &pl )
{
	TCompLayerType layer_type = DecodeCompLayerType( (char*)layer.c_str() );
	if ( layer_type == cltCheckin ||
	     layer_type == cltTCheckin ||
		   layer_type == cltTranzit ||
		   layer_type == cltBlockTrzt ||
		   layer_type == cltSOMTrzt ||
		   layer_type == cltPRLTrzt ) /* !!! будет неправильно отображаться на клиенте */
		pl.pr_free = false;
}

void SetBlock( const std::string &layer, TPlace &pl )
{
	if ( DecodeCompLayerType( (char*)layer.c_str() ) == cltBlockCent )
		pl.block = true;
}

bool CompareRems( const vector<SALONS2::TRem> &rems1, const vector<SALONS2::TRem> &rems2 )
{
	if ( rems1.size() != rems2.size() )
		return false;
	for ( vector<SALONS2::TRem>::const_iterator p1=rems1.begin(),
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

bool CompareLayers( const vector<string> &layer1, const vector<string> &layer2 )
{
	if ( layer1.size() != layer2.size() )
		return false;
	for ( vector<string>::const_iterator p1=layer1.begin(),
		    p2=layer2.begin();
		    p1!=layer1.end(),
		    p2!=layer2.end();
		    p1++, p2++ ) {
		if ( *p1 != *p2 )
			return false;
  }
  return true;
}

} // end namespace


