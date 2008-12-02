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
  "SELECT DISTINCT point_dep,point_num,layer_type "
  "FROM trip_comp_layers, "
  "(SELECT point_id,point_num from points "
  "  WHERE NVL(first_point,point_id)=:first_point AND point_num<:point_num AND pr_del=0 "
  ") a "
  "WHERE trip_comp_layers.point_id=:point_id AND "
  "      trip_comp_layers.point_dep=a.point_id AND "
  "      layer_type IN (:som_layer,:prl_layer) "
  "ORDER BY point_num DESC,layer_type";
  Qry.CreateVariable( "first_point", otInteger, first_point );
  Qry.CreateVariable( "point_num", otInteger, point_num );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "som_layer", otString, EncodeCompLayerType( cltSOMTrzt ) );
  Qry.CreateVariable( "prl_layer", otString, EncodeCompLayerType( cltPRLTrzt ) );
  Qry.Execute();
  if ( Qry.Eof )
  	return false;
  point_num = Qry.FieldAsInteger( "point_num" );
  point_dep = Qry.FieldAsInteger( "point_dep" );
  layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
  if ( layer_type == cltPRLTrzt ) {
  	Qry.Next();
  	if ( !Qry.Eof &&
  		   point_num == Qry.FieldAsInteger( "point_num" ) &&
  		   DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) == cltSOMTrzt ) {
  		layer_type = cltSOMTrzt;
  	}
  }
  return true;
}


void TFilterLayers::getFilterLayers( int point_id )
{
	clearFlags();
	setFlag( cltBlockCent );
	setFlag( cltProtect );
	setFlag( cltCheckin );
	setFlag( cltProtCkin );
	setFlag( cltPNLCkin );
	setFlag( cltTranzit );
	TQuery Qry(&OraSession);
	Qry.SQLText =
	"SELECT pr_tranz_reg,pr_block_trzt,ckin.get_pr_tranzit(:point_id) as pr_tranzit "
	" FROM trip_sets, points "
	"WHERE points.point_id=:point_id AND points.point_id=trip_sets.point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	tst();
	if ( !Qry.Eof && Qry.FieldAsInteger( "pr_tranzit" ) ) { // �� �࠭���� ३�
		if ( Qry.FieldAsInteger( "pr_tranz_reg" ) )
			setFlag( cltProtTrzt );
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

bool TFilterLayers::CanUseLayer( TCompLayerType layer_type, int point_dep )
{
	ProgTrace( TRACE5, "layer_type=%s, isFlag=%d", EncodeCompLayerType( layer_type ), isFlag( layer_type ) );
	switch( layer_type ) {
		case cltSOMTrzt:
			return ( isFlag( layer_type ) && point_dep == point_dep );
		case cltPRLTrzt:
			return ( isFlag( layer_type ) && point_dep == point_dep );
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
  if ( readStyle == rTripSalons ) {
    FilterLayers.getFilterLayers( trip_id ); // ��।������ ०��� ��� �࠭����� ᫮��
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
        NewTextChild( placeNode, "status", place->status ); // ����塞 ����� ��室� �� ᫮��
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
                  " WHERE point_id=:point_id AND layer_type IN ( :tranzit_layer, :blockcent_layer, :prot_layer );"
//                  layer_type IN ( SELECT code from comp_layer_types where del_if_comp_chg<>0 ); "
                  "END;";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.CreateVariable( "pr_lat_seat", otInteger, pr_lat_seat );
    // ��� �������� 2 ��ਠ�� ᫮� PROT_TRZT OR BLOCK_TRZT, ���� ��।����� ����� ᫮� ���� 㤠����
    if ( FilterLayers.isFlag( cltBlockTrzt ) )
      Qry.CreateVariable( "tranzit_layer", otString, EncodeCompLayerType( cltBlockTrzt ) );
    else
    	Qry.CreateVariable( "tranzit_layer", otString, EncodeCompLayerType( cltProtTrzt ) );
    Qry.CreateVariable( "blockcent_layer", otString, EncodeCompLayerType( cltBlockCent ) );
    Qry.CreateVariable( "prot_layer", otString, EncodeCompLayerType( cltProtect ) );
  }
  else { /* ��࠭���� ���������� */
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
    return; /* 㤠���� ���������� */

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
      	//!!! ���� ��⠢��� ᫮�
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


  if ( readStyle == rTripSalons ) {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("���� �� ������. ������� �����");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  else {
    Qry.SQLText =
     "SELECT pr_lat_seat FROM comps WHERE comp_id=:comp_id";
    Qry.CreateVariable( "comp_id", otInteger, comp_id );
    Qry.Execute();
    if ( Qry.Eof ) throw UserException("���������� �� �������. ������� �����");
    pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );
  }
  Qry.Clear();

  if ( readStyle == rTripSalons ) {
  	string sql_text =
      "SELECT DISTINCT t.num,t.x,t.y,t.elem_type,t.xprior,t.yprior,t.agle,"
      "                t.pr_smoke,t.not_good,t.xname,t.yname,t.class,r.layer_type, "
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
        "                t1.pr_smoke,t1.not_good,t1.xname,t1.yname,t1.class,r.layer_type, "
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
        "      salons.get_seat_no(pax.pax_id,:layer_type,pax.seats,pax_grp.point_dep,'one',rownum) IS NULL ";
    sql_text += " ORDER BY num, x desc, y desc ";
    Qry.SQLText = sql_text;
    Qry.CreateVariable( "point_id", otInteger, trip_id );
    if ( wo_invalid_seat_no )
      Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType(cltCheckin) );
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
      throw UserException( "�� ३� �� �����祭 ᠫ��" );
    else
      throw UserException( "�� ������� ����������" );
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
  string ClName = ""; /* ����᫥��� ��� ����ᮢ, ����� ���� � ᠫ��� */
  TPlaceList *placeList = NULL;
  int num = -1;
  TPoint point_p;
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
    // ����७�� ����!!! - ࠧ�� ᫮�
    TPlace place;
    point_p.x = Qry.FieldAsInteger( col_x );
    point_p.y = Qry.FieldAsInteger( col_y );
    if ( readStyle != rTripSalons || Qry.FieldIsNULL( "pax_id" ) )
    	pax_id = -1;
    else
    	pax_id = Qry.FieldAsInteger( "pax_id" );
    // �᫨ ���� �� �� ��।����� ��� ���� ����, �� �� �ந��樠����஢���
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
      place.pr_smoke = Qry.FieldAsInteger( col_pr_smoke );
      if ( Qry.FieldIsNULL( col_not_good ) )
        place.not_good = 0;
      else
        place.not_good = Qry.FieldAsInteger( col_not_good );
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
        ClName += Qry.FieldAsString( col_class );
    }
    else { // �� ���� �ந��樠����஢��� - �� ���� ᫮�
    	place = *placeList->place( point_p );
    }
    if ( readStyle == rTripSalons ) { // ����� ࠡ�� � �ᥬ� ᫮ﬨ ��� ������ ࠧ��� �ਧ�����
      if ( FilterLayers.CanUseLayer( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ), Qry.FieldAsInteger( "point_dep" ) ) ) { // ��� ᫮� �ᯮ��㥬
      	ProgTrace( TRACE5, "seat_no=%s", string(string(Qry.FieldAsString("yname"))+Qry.FieldAsString("xname")).c_str() );
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
          			TPoint p(mp[ pax_id ].x,mp[ pax_id ].y);
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
    delete placeList; // ��� ��� �����/ᠫ�� �� �㦥�
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
      for ( vector<TRem>::iterator irem=place->rems.begin(); irem!=place->rems.end(); irem++ ) {
      	if ( irem->rem == rem_name )
      		throw UserException( string( "����ઠ " ) + rem_name + " �� ����� ���� ������ � ����� " + place->clname );
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
    throw Exception( "���ࠢ���� ���न���� ����" );
  return ys[ p.y ] + xs[ p.x ];
}

string TPlaceList::GetXsName( int x )
{
  if ( x < 0 || x >= GetXsCount() ) {
    throw Exception( "���ࠢ���� x ���न��� ����" );
  }
  return xs[ x ];
}

string TPlaceList::GetYsName( int y )
{
  if ( y < 0 || y >= GetYsCount() )
    throw Exception( "���ࠢ���� y ���न��� ����" );
  return ys[ y ];
}

bool TPlaceList::GetisPlaceXY( string placeName, TPoint &p )
{
	placeName = trim( placeName );
	if ( placeName.empty() )
		return false;
  /* ��������� ����஢ ���� � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
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
//  ProgTrace( TRACE5, "GetTripParams trip_id=%d", trip_id );

  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airp,airline,flt_no,suffix,craft,bort,scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS real_out "
    "FROM points "
    "WHERE point_id=:point_id ";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");

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
  if (Qry.Eof) throw UserException("���� �� ������. ������� �����");

  /* comp_id>0 - ������; comp_id=-1 - ���������; comp_id=-2 - �� ����� */
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
  sql += " AND salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'one',rownum) IS NULL";
  Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );
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
   "           NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as f, "
   "           NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as c, "
   "           NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as y "
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
    	idx = 0; // ����� ᮢ������ ����+������������ OR ��ய���
    else
    	if ( !bort.empty() && bort == Qry.FieldAsString( "bort" ) )
    		idx = 1; // ����� ᮢ������ ����
    	else
    		if ( airline_OR_airp )
    			idx = 2; // ����� ᮢ������ ������������ ��� ��ய���
    		else {
    			Qry.Next();
    			continue;
    		}
    // ᮢ������� �� ���-�� ���� ��� ������� �����
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
    // ᮢ������� �� ����ᠬ � �������� ���� >= ��饥 ���-�� ����
    if ( SIGN( Qry.FieldAsInteger( "f" ) ) == SIGN( f ) &&
         SIGN( Qry.FieldAsInteger( "c" ) ) == SIGN( c ) &&
         SIGN( Qry.FieldAsInteger( "y" ) ) == SIGN( y ) &&
         CompMap[ idx ].sum > Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y ) {
      CompMap[ idx ].sum = Qry.FieldAsInteger( "f" ) + Qry.FieldAsInteger( "c" ) + Qry.FieldAsInteger( "y" ) - f - c - y;
      CompMap[ idx ].comp_id = Qry.FieldAsInteger( "comp_id" );
    }
    // ᮢ������� �� �������� ���� >= ��饥 ���-�� ����
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
 	return CompMap.begin()->second.comp_id; // ��������� ����� - ���஢�� ���� �� �����⠭��
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
    if ( !Qry.Eof && Qry.FieldAsInteger( "auto_comp_chg" ) ) { // ��⮬���᪮� �����祭�� ����������
    	ProgTrace( TRACE5, "Auto set comp, point_id=%d", point_id );
      return SetCraft( point_id, craft, comp_id );
    }
    return 0; // �� �ॡ���� �����祭�� ����������
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
  // �஢�ઠ �� ����⢮����� �������� ���������� �� ⨯� ��
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
  // �롨ࠥ� ����. ���������� �� CFG �� PNL/ADL ��� ��� 業�஢ �஭�஢����
  	Qry.Clear();
  	Qry.SQLText =
      "SELECT NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) f, "
      "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) c, "
      "       NVL( MAX( DECODE( class, '�', cfg, 0 ) ), 0 ) y "
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
    		if ( vclass == "�" ) f = Qry.FieldAsInteger( "c" );
    		if ( vclass == "�" ) c = Qry.FieldAsInteger( "c" );
    		if ( vclass == "�" ) y = Qry.FieldAsInteger( "c" );
    	}
    	Qry.Next();
    }
    if ( f + c + y > 0 ) { // ���� ��ਠ�� �஭� + �࠭���
    	comp_id = GetCompId( craft, bort, airline, airp, f, c, y );
    	if ( comp_id >= 0 )
    		break;
    }
    else { // ������ �� �஭�஢���� �� ����㯠��, ⮣�� ���� �� ����� �� ᥧ����(f,c,y)
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
	// ������ ��ਠ�� ����������
	if ( old_comp_id == comp_id )
		return 0; // �� �㦭� �������� ����������
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
  TReqInfo::Instance()->MsgToLog( string( "�����祭� ������� ���������� (��=" ) + IntToString( comp_id ) +
  	                              "). ������: " + Qry.FieldAsString( "cl" ), evtFlt, point_id );
  return comp_id;
}

void InitVIP( int point_id )
{
	tst();
	// ���樠������ - ࠧ���� ᠫ��� �� 㬮�砭�
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
  	return; // ࠧ���� �� �㦭�
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
    "        trip_comp_rem.y = :y "; //??? �᫨ ���� ६�ઠ �����祭��� ���짮��⥫��, � �� �ண��� ����
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
		   layer_type == cltTranzit ||
		   layer_type == cltBlockTrzt ||
		   layer_type == cltSOMTrzt ||
		   layer_type == cltPRLTrzt ) /* !!! �㤥� ���ࠢ��쭮 �⮡ࠦ����� �� ������ */
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


// �롮� ��������� �� ᠫ���
void getSalonChanges( TSalons &OldSalons, vector<TSalonSeat> &seats )
{
	seats.clear();
	TSalons Salons( OldSalons.trip_id, rTripSalons );
	Salons.Read();
	if ( Salons.getLatSeat() != OldSalons.getLatSeat() ||
		   Salons.placelists.size() != OldSalons.placelists.size() )
		throw UserException( "�������� ���������� ३�. ������� �����" );
	for ( vector<TPlaceList*>::iterator so=OldSalons.placelists.begin(),
		    /*vector<TPlaceList*>::iterator */sn=Salons.placelists.begin();
		    so!=OldSalons.placelists.end(),
		    sn!=Salons.placelists.end();
		    so++, sn++ ) {
		if ( (*so)->places.size() != (*sn)->places.size() )
			throw UserException( "�������� ���������� ३�. ������� �����" );
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
        throw UserException( "�������� ���������� ३�. ������� �����" );
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
      NewTextChild( n, "status", p->second.status ); // ����塞 ����� ��室� �� ᫮��//!!!old version
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


