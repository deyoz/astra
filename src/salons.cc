#include <stdlib.h>
#include "setup.h"
#define NICKNAME "DJEK"
#include "test.h"
#include "salons.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "str_utils.h"
#include "images.h"
#include "tripinfo.h"
#include "convert.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const int REM_VIP_F = 1;
const int REM_VIP_C = 1;
const int REM_VIP_Y = 3;

string DecodeLayer( const std::string &layer )
{
	switch( DecodeCompLayerType( (char*)layer.c_str() ) ) {
		case cltPreseat:
			return "PS";
    case cltPNLCkin:
    	return "BR";
    case cltPRLTrzt:
    case cltSOMTrzt:
    case cltTranzit:
    	return "TR";
    case cltBlockCent:
    	return "BL";
    case cltProtect:
    	return "RZ";
    default: return "";
  }
}

string EncodeLayer( const std::string &int_layer )
{
	if ( int_layer == "BL" )
		return EncodeCompLayerType( cltBlockCent );
	else
		if ( int_layer == "RZ" )
			return  EncodeCompLayerType( cltProtect );
		else
			if ( int_layer == "TR" )
				return EncodeCompLayerType( cltTranzit );
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


TSalons::TSalons()
{
	pr_lat_seat = false;
  FCurrPlaceList = NULL;
  modify = mNone;  
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT code, priority FROM comp_layer_types ORDER BY priority";
  Qry.Execute();
  string status;
  while ( !Qry.Eof ) {
  	status = DecodeLayer( Qry.FieldAsString( "code" ) );  	
  	if ( !status.empty() && layer_priority.find( status ) == layer_priority.end() )
  	  layer_priority[ status ] = Qry.FieldAsInteger( "priority" );
  	Qry.Next();
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
      NewTextChild( placeNode, "yname", denorm_iata_row( place->yname ) );
      if ( place->status != "FP" )
        NewTextChild( placeNode, "status", place->status ); // вычисляем статус исходя из слоев
      if ( !place->pr_free )
        NewTextChild( placeNode, "pr_notfree" );
      if ( place->block )
        NewTextChild( placeNode, "block" );
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

void TSalons::Write( TReadStyle readStyle )
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
    "     first_xname,last_xname,first_yname,last_yname,crs_pax_id,pax_id) "
    "  VALUES "
    "    (:range_id,:point_id,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:crs_pax_id,:pax_id); "
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
                  " DELETE trip_comp_layers "                  
                  " WHERE point_id=:point_id AND layer_type IN ( 'TRANZIT', 'BLOCK_CENT', 'PROTECT' );"
//                  layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
                  "END;";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );    
    tst();
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
  tst();
  if ( readStyle == rComponSalons && modify == mDelete )
    return; /* удалили компоновку */
  tst();

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
    Qry.SQLText = "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class, "\
                  "                            pr_smoke,not_good,xname,yname,pr_free,enabled) "\
                  " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, "\
                  "        :pr_smoke,:not_good,:xname,:yname,:pr_free,:enabled)";
    Qry.DeclareVariable( "point_id", otInteger );
//    Qry.DeclareVariable( "status", otString );
    Qry.DeclareVariable( "pr_free", otInteger );
    Qry.DeclareVariable( "enabled", otInteger );
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
      if ( !place->pr_smoke )
        Qry.SetVariable( "pr_smoke", FNull );
      else
        Qry.SetVariable( "pr_smoke", 1 );
      if ( !place->not_good )
        Qry.SetVariable( "not_good", FNull );
      else
        Qry.SetVariable( "not_good", 1 );
      Qry.SetVariable( "xname", place->xname );
      Qry.SetVariable( "yname", place->yname );
      if ( readStyle == rTripSalons ) {
        //Qry.SetVariable( "status", place->status );
        if ( !place->pr_free )
          Qry.SetVariable( "pr_free", FNull );
        else
          Qry.SetVariable( "pr_free", 1 );
        if ( place->block )
          Qry.SetVariable( "enabled", FNull );
        else
          Qry.SetVariable( "enabled", 1 );
      }
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
      	//!!! надо вставить слой
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
}

void TSalons::Read( TReadStyle readStyle )
{
  if ( readStyle == rTripSalons )
    ProgTrace( TRACE5, "TSalons::Read TripSalons with params trip_id=%d, ClassName=%s",
               trip_id, ClName.c_str() );
  else {
    ClName.clear();
    ProgTrace( TRACE5, "TSalons::Read ComponSalons with params comp_id=%d",
               comp_id );
  }
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );
  
  
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
    Qry.SQLText = 
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.pr_smoke,t.not_good,t.xname,t.yname,t.class,r.layer_type "
      " FROM trip_comp_elems t, trip_comp_ranges r "
      "WHERE t.point_id=:point_id AND "
      "      t.point_id=r.point_id(+) AND "
      "      t.num=r.num(+) AND "
      "      t.x=r.x(+) AND "
      "      t.y=r.y(+) "
      " ORDER BY t.num, t.x desc, t.y desc ";
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
  while ( !Qry.Eof ) {
    if ( num != Qry.FieldAsInteger( "num" ) ) {
      if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
        placeList->places.clear();
      }
      else {
        placeList = new TPlaceList();
        placelists.push_back( placeList );
      }
      ClName.clear();
      num = Qry.FieldAsInteger( "num" );
      placeList->num = num;
    }
    // повторение мест!!! - разные слои
    TPlace place;
    point_p.x = Qry.FieldAsInteger( "x" );
    point_p.y = Qry.FieldAsInteger( "y" );
    // если место еще не определено или место есть, но не проинициализировано
    if ( !placeList->ValidPlace( point_p ) || placeList->place( point_p )->x == -1 ) {
    	place.x = point_p.x;
    	place.y = point_p.y;
      place.elem_type = Qry.FieldAsString( "elem_type" );
      place.isplace = ispl[ place.elem_type ];
      if ( Qry.FieldIsNULL( "xprior" ) )
        place.xprior = -1;
      else
        place.xprior = Qry.FieldAsInteger( "xprior" );
      if ( Qry.FieldIsNULL( "yprior" ) )
        place.yprior = -1;
      else
        place.yprior = Qry.FieldAsInteger( "yprior" );
      place.agle = Qry.FieldAsInteger( "agle" );
      place.clname = Qry.FieldAsString( "class" );
      place.pr_smoke = Qry.FieldAsInteger( "pr_smoke" );
      if ( Qry.FieldIsNULL( "not_good" ) )
        place.not_good = 0;
      else
        place.not_good = Qry.FieldAsInteger( "not_good" );
      place.xname = Qry.FieldAsString( "xname" );
      place.yname = Qry.FieldAsString( "yname" );
      while ( !RQry.Eof && RQry.FieldAsInteger( "num" ) == num &&
              RQry.FieldAsInteger( "x" ) == place.x &&
              RQry.FieldAsInteger( "y" ) == place.y ) {
        TRem rem;
        rem.rem = RQry.FieldAsString( "rem" );
        rem.pr_denial = RQry.FieldAsInteger( "pr_denial" );
        place.rems.push_back( rem );
        RQry.Next();
      }      
      if ( ClName.find( Qry.FieldAsString( "class" ) ) == string::npos )
        ClName += Qry.FieldAsString( "class" );      
    }
    else { // это место проинициализировано - это новый слой
      tst();    	
    	place = *placeList->place( point_p );
      tst();
    }
    if ( readStyle == rTripSalons ) { // здесь работа со всеми слоями для выявления разных признаков
      SALONS::SetLayer( this->layer_priority, Qry.FieldAsString( "layer_type" ), place );
      SALONS::SetFree( Qry.FieldAsString( "layer_type" ), place );
    }
    if ( readStyle == rTripSalons )
      SALONS::SetBlock( Qry.FieldAsString( "layer_type" ), place );
    else
      place.block = 0;
    place.visible = true;
    placeList->Add( place );
    Qry.Next();
  }	/* end while */
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
  node = GetNode( "@pr_lat_seat", salonsNode );
  if ( node ) {
  	tst();
  	pr_lat_seat = NodeAsInteger( node ); 
  }
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  node = salonsNode->children;
  xmlNodePtr salonNode = NodeAsNodeFast( "placelist", node );
  TRem rem;
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
      place.pr_smoke = GetNodeFast( "pr_smoke", node );
      place.not_good = GetNodeFast( "not_good", node );
      place.xname = norm_iata_line( NodeAsStringFast( "xname", node ) );
      place.yname = norm_iata_row( NodeAsStringFast( "yname", node ) );
      if ( !GetNodeFast( "status", node ) )
        place.status = "FP";
      else
        place.status = NodeAsStringFast( "status", node ); //!!!
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
      		string l = EncodeLayer( NodeAsStringFast( "layer_type", remNode ) );
      		if ( !l.empty() )
      		  place.layers.push_back( l );
      		remsNode = remsNode->next;
      	}
      }
      else { //old version
      		string l = EncodeLayer( place.status );
      		ProgTrace( TRACE5, "status-layer=%s", l.c_str() );
      		if ( !l.empty()  )
      		  place.layers.push_back( l );
      		if ( place.block && find( place.layers.begin(), place.layers.end(), string("BL") ) == place.layers.end() )
      			place.layers.push_back( EncodeLayer( "BL" ) );
      }
      place.visible = true;
      placeList->Add( place );
      placeNode = placeNode->next;
    }
    placelists.push_back( placeList );
    salonNode = salonNode->next;
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
  ProgTrace( TRACE5, "GetisPlaceXY: seat_no=%s, new_seat_no=%s", placeName.c_str(), seat_no.c_str() );
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


namespace SALONS
{
	
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
  ProgTrace( TRACE5, "GetTripParams trip_id=%d", trip_id );

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
  tst();
  Qry.Execute();
  tst();
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
 if ( SeatNoIsNull )
  sql += " AND seat_no IS NULL";
 Qry.SQLText = sql;
 Qry.DeclareVariable( "point_id", otInteger );
 Qry.SetVariable( "point_id", trip_id );
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
   "       f < 1000 AND c < 1000 AND y < 1000 ";
	Qry.CreateVariable( "craft", otString, craft );
	Qry.CreateVariable( "vf", otString, f );
	Qry.CreateVariable( "vc", otString, c );
	Qry.CreateVariable( "vy", otString, y );	
	Qry.Execute();
	ProgTrace( TRACE5, "bort=%s, airline=%s, airp=%s", bort.c_str(), airline.c_str(), airp.c_str() );
  while ( !Qry.Eof ) {  	
    if ( bort == Qry.FieldAsString( "bort" ) && 
    	   ( airline == Qry.FieldAsString( "airline" ) || airp == Qry.FieldAsString( "airp" ) ) )
    	idx = 0;
    else
    	if ( bort == Qry.FieldAsString( "bort" ) )
    		idx = 1;
    	else
    		if ( airline == Qry.FieldAsString( "airline" ) || airp == Qry.FieldAsString( "airp" ) )
    			idx = 2;
    		else {
    			Qry.Next();
    			continue;
    		}
    // совпадение по классам и количеству мест
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );         	
    }
    // совпадение по количеству мест
    idx += 3;
    if ( CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );    	
    }
  	Qry.Next();
  }
 ProgTrace( TRACE5, "CompMap.size()=%d", CompMap.size() );
 if ( !CompMap.size() )
 	return -1;
 else
 	return CompMap.begin()->second.comp_id; // минимальный элемент - сортировка ключа по позрастанию
}

int SetCraft( int point_id, std::string &craft, int comp_id )
{
	tst();
	TQuery Qry(&OraSession);
	Qry.SQLText = 
	  "SELECT bort,airline,suffix,airp,craft "
    " FROM points WHERE point_id=:point_id ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();  
  tst();
  int f,c,y;
  string bort = Qry.FieldAsString( "bort" );
  string airline = Qry.FieldAsString( "airline" );
  string suffix = Qry.FieldAsString( "suffix" );
  string airp = Qry.FieldAsString( "airp" );  
  if ( craft.empty() )
  	craft = Qry.FieldAsString( "craft" );
  tst();
  // проверка на существование заданной компоновки по типу ВС  
	Qry.Clear();
	Qry.SQLText =
   "SELECT comp_id FROM comps "
   " WHERE ( comp_id=:comp_id OR :comp_id < 0 ) AND craft=:craft AND rownum < 2";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
 	Qry.CreateVariable( "craft", otString, craft );
 	Qry.Execute();
 	tst();
 	if ( Qry.Eof )
 		if ( comp_id < 0 )
 	  	return -1;
 	  else
 	  	return -2;
 	tst();  	
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
    tst();
    string target, vclass;
    f = 0; c = 0; y = 0;
    if ( !Qry.Eof ) {
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
    tst();
    return -3;
	}
	tst();
	// найден вариант компоновки
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
    "DELETE trip_classes WHERE point_id = :point_id; "
    "DELETE trip_comp_layers "
    " WHERE point_id=:point_id AND layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
    "INSERT INTO trip_comp_elems(point_id,num,x,y,elem_type,xprior,yprior,agle,class, "
    "                            pr_smoke,not_good,xname,yname,status,pr_free,enabled) "
    " SELECT :point_id,num,x,y,elem_type,xprior,yprior,agle,class, "
    "        pr_smoke,not_good,xname,yname,'FP',1,1 "
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
  tst();
  InitVIP( point_id );
  tst();
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "INSERT INTO trip_classes(point_id,class,cfg,block,prot) "
    " SELECT :point_id, "
    "        class, "
    "        NVL( SUM( DECODE( class, NULL, 0, 1 ) ), 0 ), "
    "        NVL( SUM( DECODE( class, NULL, 0, DECODE( enabled, NULL, 1, 0 ) ) ), 0 ), "
    "        0 "
    "  FROM trip_comp_elems, comp_elem_types "
    " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "       comp_elem_types.pr_seat <> 0 AND "
    "       trip_comp_elems.point_id=:point_id "
    " GROUP BY class; "
    " ckin.recount( :point_id ); "
    "END; ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  tst();
  Qry.Clear();
  Qry.SQLText = 
   "SELECT ckin.get_classes(:point_id) cl FROM dual ";
  Qry.CreateVariable( "point_id", otInteger, point_id ); 
  Qry.Execute();
  tst();
  TReqInfo::Instance()->MsgToLog( string( "Назначена базовая компоновка (ид=" ) + IntToString( comp_id ) + 
  	                              "). Классы: " + Qry.FieldAsString( "cl" ), evtFlt, point_id );
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
  tst();
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
  tst();
  if ( Qry.Eof || !Qry.FieldAsInteger( "pr_misc" ) ) 
  	return; // разметра не нужна
  tst();
  Qry.Clear();	
	Qry.SQLText = 
	  "SELECT num, class, MIN( y ) miny, MAX( y ) maxy "
    " FROM trip_comp_elems, comp_elem_types "
    "WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
    "      comp_elem_types.pr_seat <> 0 AND "
    "      trip_comp_elems.point_id = :point_id "
    "GROUP BY trip_comp_elems.num, trip_comp_elems.class";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  tst();
  Qry.Execute();
  tst();
  QryVIP.SQLText =
    "INSERT INTO trip_comp_rem(point_id,num,x,y,rem,pr_denial) "
    " SELECT :point_id,num,x,y,'VIP', 0 FROM trip_comp_elems "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        pr_free <> 0 AND enabled IS NOT NULL AND "
    "        elem_type IN ( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) AND "
    "        trip_comp_elems.y = :y "
    " MINUS "
    " SELECT :point_id,num,x,y,rem, 0 FROM trip_comp_rem "
    "  WHERE point_id = :point_id AND num = :num AND "
    "        trip_comp_rem.y = :y "; //??? если есть ремарка назначенная пользователем, то не трогаем место
  QryVIP.CreateVariable( "point_id", otInteger, point_id );
  QryVIP.DeclareVariable( "num", otInteger );
  QryVIP.DeclareVariable( "y", otInteger );
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
  tst();
}

void SetLayer( const std::map<std::string,int> &layer_priority, const std::string &layer, TPlace &pl )
{
	if ( layer.empty() )
		return;		
  const map<string,int>::const_iterator n = layer_priority.find( DecodeLayer( layer ) );
  const map<string,int>::const_iterator p = layer_priority.find( pl.status );
 
  if ( n != layer_priority.end() ) {
  	 if ( p == layer_priority.end() || n->second < p->second )
  	 	pl.status = n->first;
  }  
  pl.layers.push_back( layer );
}
void SetFree( const std::string &layer, TPlace &pl )
{
	if ( DecodeCompLayerType( (char*)layer.c_str() ) == cltCheckin )
		pl.pr_free = false;
}

void SetBlock( const std::string &layer, TPlace &pl )
{
	if ( DecodeCompLayerType( (char*)layer.c_str() ) == cltBlockCent )
		pl.block = true;	
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


// выбор изменений по салону
void getSalonChanges( TSalons &OldSalons, vector<TSalonSeat> &seats )
{
	seats.clear();
	TSalons Salons;
	Salons.trip_id = OldSalons.trip_id;
	Salons.Read( rTripSalons );
	if ( Salons.getLatSeat() != OldSalons.getLatSeat() ||
		   Salons.placelists.size() != OldSalons.placelists.size() )
		throw UserException( "Изменена компоновка рейса. Обновите данные" );
	for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
		    /*vector<TPlaceList*>::iterator */sn=Salons.placelists.begin(); 
		    so!=OldSalons.placelists.end(), 
		    sn!=Salons.placelists.end(); 
		    so++, sn++ ) {
		if ( (*so)->places.size() != (*sn)->places.size() )
			throw UserException( "Изменена компоновка рейса. Обновите данные" );
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
        throw UserException( "Изменена компоновка рейса. Обновите данные" );	   
      if ( !po->visible )
      	continue;
      if ( po->pr_smoke != pn->pr_smoke ||
      	   po->not_good != pn->not_good ||
      	   !CompareRems( po->rems, pn->rems ) ||
      	   !CompareLayers( po->layers, pn->layers ) ) {
       seats.push_back( make_pair((*so)->num,*pn) );
      }
    }
	}		
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
    if ( p->second.pr_smoke )
      NewTextChild( n, "pr_smoke" );
    if ( p->second.not_good )
      NewTextChild( n, "not_good" );
    if ( p->second.status != "FP" ) //!!!old version
      NewTextChild( n, "status", p->second.status ); // вычисляем статус исходя из слоев//!!!old version
    if ( !p->second.pr_free )//!!!old version
      NewTextChild( n, "pr_notfree" );//!!!old version
    if ( p->second.block )//!!!old version
      NewTextChild( n, "block" );//!!!old version
      
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
      for( std::vector<std::string>::const_iterator l=p->second.layers.begin(); l!=p->second.layers.end(); l++ ) {
      	remNode = NewTextChild( remsNode, "layer" );
      	NewTextChild( remNode, "layer_type", *l );
      }
    }
	} 	
}


} // end namespace
