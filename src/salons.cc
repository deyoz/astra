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

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

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
  FCurrPlaceList = NULL;
  modify = mNone;
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
      NewTextChild( placeNode, "xname", place->xname );
      NewTextChild( placeNode, "yname", place->yname );
      if ( place->status != "FP" )
        NewTextChild( placeNode, "status", place->status );
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
  if ( readStyle == rTripSalons ) {
    Qry.SQLText = "BEGIN "\
                  " UPDATE points SET point_id=point_id WHERE point_id=:point_id; "\
                  " DELETE trip_comp_rem WHERE point_id=:point_id; "\
                  " DELETE trip_comp_elems WHERE point_id=:point_id; "\
                  "END;";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
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
                       "        time_create=system.UTCSYSDATE,classes=:classes "\
                       "  WHERE comp_id=:comp_id; "\
                       " DELETE comp_rem WHERE comp_id=:comp_id; "\
                       " DELETE comp_elems WHERE comp_id=:comp_id; "\
                       "END; ";
         break;
      case mAdd:
         Qry.SQLText = "INSERT INTO comps(comp_id,airline,airp,craft,bort,descr,time_create,classes) "\
                       " VALUES(:comp_id,:airline,:airp,:craft,:bort,:descr,system.UTCSYSDATE,:classes) ";
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
                  "                            pr_smoke,not_good,xname,yname,status,pr_free,enabled) "\
                  " VALUES(:point_id,:num,:x,:y,:elem_type,:xprior,:yprior,:agle,:class, "\
                  "        :pr_smoke,:not_good,:xname,:yname,:status,:pr_free,:enabled)";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.DeclareVariable( "status", otString );
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
        Qry.SetVariable( "status", place->status );
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
    }
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
    /* ??? :class */
    Qry.SQLText = "SELECT num,x,y,elem_type,xprior,yprior,agle,pr_smoke,not_good,xname,yname, "\
                  "       status,class,pr_free,enabled "\
                  " FROM trip_comp_elems "\
                  "WHERE point_id=:point_id "\
                  " ORDER BY num, x desc, y desc ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
    /*Qry.DeclareVariable( "class", otString );
      Qry.SetVariable( "class", ClName );*/
  }
  else {
    Qry.SQLText = "SELECT num,x,y,elem_type,xprior,yprior,agle,class,pr_smoke,not_good, "\
                  "       xname,yname "\
                  " FROM comp_elems "\
                  "WHERE comp_id=:comp_id "\
                  "ORDER BY num, x desc, y desc ";
    Qry.DeclareVariable( "comp_id", otInteger );
    Qry.SetVariable( "comp_id", comp_id );
  }
  if ( Qry.RowCount() == 0 )
    if ( readStyle == rTripSalons )
      throw UserException( "На рейс не назначен салон" );
    else
      throw UserException( "Не найдена компоновка" );
  if ( readStyle == rTripSalons ) {
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM trip_comp_rem "\
                   " WHERE point_id=:point_id "\
                   "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "point_id", otInteger );
    RQry.SetVariable( "point_id", trip_id );
  }
  else {
    RQry.SQLText = "SELECT num,x,y,rem,pr_denial FROM comp_rem "\
                   " WHERE comp_id=:comp_id "\
                   "ORDER BY num, x desc, y desc ";
    RQry.DeclareVariable( "comp_id", otInteger );
    RQry.SetVariable( "comp_id", comp_id );
  }
  Qry.Execute();
  RQry.Execute();
  string ClName = ""; /* перечисление всех классов, которые есть в салоне */
  TPlaceList *placeList = NULL;
  int num = -1;
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
    TPlace place;
    place.x = Qry.FieldAsInteger( "x" );
    place.y = Qry.FieldAsInteger( "y" );
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
    place.visible = true;
    if ( readStyle == rTripSalons ) {
      place.status = Qry.FieldAsString( "status" );
      place.pr_free = Qry.FieldAsInteger( "pr_free" );
    }
    if ( readStyle == rTripSalons && Qry.FieldIsNULL( "enabled" ) )
      place.block = 1;
    else
      place.block = 0;
    while ( !RQry.Eof && RQry.FieldAsInteger( "num" ) == num &&
            RQry.FieldAsInteger( "x" ) == place.x &&
            RQry.FieldAsInteger( "y" ) == place.y ) {
      TRem rem;
      rem.rem = RQry.FieldAsString( "rem" );
      rem.pr_denial = RQry.FieldAsInteger( "pr_denial" );
      place.rems.push_back( rem );
      RQry.Next();
    }
    place.visible = true;
    placeList->Add( place );
    if ( ClName.find( Qry.FieldAsString( "class" ) ) == string::npos )
      ClName += Qry.FieldAsString( "class" );
    Qry.Next();
  }	/* end while */
  if ( placeList && !ClName.empty() && ClName.find( ClName ) == string::npos ) {
    placelists.pop_back( );
    delete placeList; // нам этот класс/салон не нужен
  }
  tst();
}

void TSalons::GetTripParams( int trip_id, xmlNodePtr dataNode )
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

void TSalons::GetCompParams( int comp_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT comp_id,craft,bort,descr from comps "\
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

bool TSalons::InternalExistsRegPassenger( int trip_id, bool SeatNoIsNull )
{
  TQuery Qry( &OraSession );
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

void TSalons::Parse( xmlNodePtr salonsNode )
{
  if ( salonsNode == NULL )
    return;
  Clear();
  map<string,bool> ispl;
  ImagesInterface::GetisPlaceMap( ispl );
  xmlNodePtr node;
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
      place.xname = NodeAsStringFast( "xname", node );
      place.yname = NodeAsStringFast( "yname", node );
      if ( !GetNodeFast( "status", node ) )
        place.status = "FP";
      else
        place.status = NodeAsStringFast( "status", node );
      place.pr_free = !GetNodeFast( "pr_notfree", node );
      place.block = GetNodeFast( "block", node );

      xmlNodePtr remNode = GetNodeFast( "rems", node );
      if ( remNode ) {
      	remNode = remNode->children;
      	while ( remNode ) {
      	  node = remNode->children;
      	  rem.rem = NodeAsStringFast( "rem", node );
      	  rem.pr_denial = GetNodeFast( "pr_denial", node );
      	  place.rems.push_back( rem );
      	  remNode = remNode->next;
        }
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
  if ( !placeName.empty() && placeName[ 0 ] == '0' )
    placeName.erase( 0, 1 );
  for( vector<string>::iterator ix=xs.begin(); ix!=xs.end(); ix++ )
    for ( vector<string>::iterator iy=ys.begin(); iy!=ys.end(); iy++ ) {
      if ( placeName == *iy + *ix ) {
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


