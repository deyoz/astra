#include <stdlib.h>
#include "sopp.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "basic.h"
#include "sys/times.h"
#include <map>

using namespace std;
using namespace BASIC;

struct TSoppStage {
  int stage_id;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
};

struct TTrip {
  int move_row_id;
  string company;
  int flt_no;
  string suffix;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
  string bc;
  string bort;
  string park;
  int status;
  string triptype;
  string litera;
  string classes;
  string remark;
  int reg;
  int resa;
  vector<string> places;
  vector<TSoppStage> stages;
  TTrip() {
    move_row_id = -1;
    flt_no = -1;
    scd = -1;
    est = -1;
    act = -1;
    reg = 0;
    resa = 0;
    status = -1;
  }
};

inline int fillTrip( TTrip &trip, TQuery &Qry )
{
  if ( Qry.FieldIsNULL( 1 ) )
    trip.move_row_id = -1;
  else
    trip.move_row_id = Qry.FieldAsInteger( 1 );
  trip.company = Qry.FieldAsString( 2 );
  trip.flt_no = Qry.FieldAsInteger( 3 );
  trip.suffix = Qry.FieldAsString( 4 );  
  trip.scd = Qry.FieldAsDateTime( 5 );
  if ( Qry.FieldIsNULL( 6 ) )
    trip.est = -1;
  else
    trip.est = Qry.FieldAsDateTime( 6 );
  if ( Qry.FieldIsNULL( 7 ) )
    trip.act = -1;
  else
    trip.act = Qry.FieldAsDateTime( 7 );
  trip.bc = Qry.FieldAsString( 8 );
  trip.bort = Qry.FieldAsString( 9 );
  trip.park = Qry.FieldAsString( 10 );
  trip.status = Qry.FieldAsInteger( 11 );
  trip.triptype = Qry.FieldAsString( 12 );
  trip.litera = Qry.FieldAsString( 13 );
  trip.classes = Qry.FieldAsString( 14 );
  trip.remark = Qry.FieldAsString( 15 );	
  return Qry.FieldAsInteger( 0 );
}

inline void buildTrip( TTrip &trip, xmlNodePtr outNode )
{
  if ( trip.move_row_id >= 0 )
    NewTextChild( outNode, "move_row_id", trip.move_row_id );
  NewTextChild( outNode, "company", trip.company );
  NewTextChild( outNode, "flt_no", trip.flt_no );
  if ( !trip.suffix.empty() )
    NewTextChild( outNode, "suffix", trip.suffix );
  NewTextChild( outNode, "scd", DateTimeToStr( trip.scd ) );
  if ( trip.est >= 0 )
    NewTextChild( outNode, "est", DateTimeToStr( trip.est ) );
  if ( trip.act >= 0 )
    NewTextChild( outNode, "act", DateTimeToStr( trip.act ) );
  NewTextChild( outNode, "bc", trip.bc );
  if ( !trip.bort.empty() )
    NewTextChild( outNode, "bort", trip.bort );
  if ( !trip.park.empty() )
    NewTextChild( outNode, "park", trip.park );
  if ( trip.status )
    NewTextChild( outNode, "status", trip.status );
  NewTextChild( outNode, "triptype", trip.triptype );
  if ( !trip.litera.empty() )
    NewTextChild( outNode, "litera", trip.litera );
  if ( !trip.classes.empty() )
    NewTextChild( outNode, "classes", trip.classes );
  if ( !trip.remark.empty() )
    NewTextChild( outNode, "remark", trip.remark );
  if ( trip.reg )
    NewTextChild( outNode, "reg", trip.reg );
  if ( trip.resa )
    NewTextChild( outNode, "resa", trip.resa );    
  xmlNodePtr node = NewTextChild( outNode, "places" );
  for ( vector<string>::iterator isp=trip.places.begin(); isp!=trip.places.end(); isp++ ) {
    NewTextChild( node, "cod", *isp );
  }
  if ( !trip.stages.empty() ) {
    xmlNodePtr node = NewTextChild( outNode, "stages" );
    for ( vector<TSoppStage>::iterator iss=trip.stages.begin(); iss!=trip.stages.end(); iss++ ) {
      xmlNodePtr n = NewTextChild( node, "stage" );      
      NewTextChild( n, "stage_id", iss->stage_id );
      NewTextChild( outNode, "scd", DateTimeToStr( iss->scd ) );
      if ( iss->est >= 0 )
        NewTextChild( outNode, "est", DateTimeToStr( iss->est ) );
      if ( iss->act >= 0 )
        NewTextChild( outNode, "act", DateTimeToStr( iss->act ) );
    }  
  }
}

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{	
  ProgTrace( TRACE5, "ReadTrips" );
  TQuery OutQry( &OraSession );
  OutQry.SQLText = 
    "SELECT trip_id,move_row_id,company,flt_no,suffix,scd,est,act,bc,bort,park,status,triptype,litera, "\
    "       SUBSTR( ckin.get_classes( trips.trip_id ), 1, 24 ) as classes, "\
    "       remark "\
    " FROM trips "
    "WHERE status!=-1 "\
    "ORDER BY trip_id ";
  TQuery InQry( &OraSession );
  InQry.SQLText = 
    "SELECT trip_id,move_row_id,company,flt_no,suffix,scd,est,act,bc,bort,park,status,triptype,litera, "\
    "       '',remark "\
    " FROM trips_in "\
    "WHERE status!=-1 "\
    "ORDER BY trip_id ";
  TQuery RegQry( &OraSession );
  RegQry.SQLText = 
    "SELECT pax_grp.point_id as trip_id, SUM(pax.seats) as reg FROM pax_grp, pax "\
    " WHERE pax_grp.grp_id=pax.grp_id AND pax.pr_brd IS NOT NULL "\
    "GROUP BY pax_grp.point_id "\
    "ORDER BY pax_grp.point_id ";
  TQuery ResaQry( &OraSession );
  ResaQry.SQLText = 
    "SELECT point_id as trip_id,SUM(crs_ok) as resa FROM counters2 "\
    " GROUP BY point_id "\
    "ORDER BY point_id ";
  TQuery PlacesQry( &OraSession );
  PlacesQry.SQLText = 
    "SELECT trip_id,num,cod FROM place"\
    " WHERE num > 0 "\
    " UNION "\
    "SELECT trip_id,num,cod FROM place_in "\
    " WHERE num < 0 "\
    " ORDER BY trip_id,num ";
  TQuery StagesQry( &OraSession );
  StagesQry.SQLText =     
    "SELECT point_id,stage_id,scd,est,act FROM trip_stages "\
    " ORDER BY point_id,stage_id ";    
  OutQry.Execute();
  InQry.Execute();
  RegQry.Execute();
  ResaQry.Execute();
  PlacesQry.Execute();
  StagesQry.Execute();
  map<int,TTrip> trips, trips_in;
  int trip_id;
  while ( !OutQry.Eof ) {
    TTrip trip;
    trip_id = fillTrip( trip, OutQry );
    while ( !RegQry.Eof && trip_id >= RegQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == RegQry.FieldAsInteger( "trip_id" ) ) {
      	trip.reg = RegQry.FieldAsInteger( "reg" );
      	break;
      }
      RegQry.Next();
    }
    while ( !ResaQry.Eof && trip_id >= ResaQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == ResaQry.FieldAsInteger( "trip_id" ) ) {
      	trip.resa = ResaQry.FieldAsInteger( "resa" );
      	break;
      }
      ResaQry.Next();
    }    
    while ( !PlacesQry.Eof && trip_id >= PlacesQry.FieldAsInteger( "trip_id" ) ) {
      if ( trip_id == PlacesQry.FieldAsInteger( "trip_id" ) ) {
      	trip.places.push_back( PlacesQry.FieldAsString( "cod" ) );
      }
      PlacesQry.Next();
    }            
    TSoppStage stage;    
    while ( !StagesQry.Eof && trip_id >= StagesQry.FieldAsInteger( "point_id" ) ) {
      if ( trip_id == StagesQry.FieldAsInteger( "point_id" ) ) {      	
      	stage.stage_id = StagesQry.FieldAsInteger( "stage_id" );
      	stage.scd = StagesQry.FieldAsDateTime( "scd" );
      	if ( StagesQry.FieldIsNULL( "est" ) )
      	  stage.est = -1;
      	else
      	  stage.est = StagesQry.FieldAsDateTime( "est" );
      	if ( StagesQry.FieldIsNULL( "act" ) )
      	  stage.act = -1;
      	else
      	  stage.act = StagesQry.FieldAsDateTime( "act" );      	  
      	trip.stages.push_back( stage );
      }
      PlacesQry.Next();
    }                
    trips.insert( make_pair( trip_id, trip ) );
    OutQry.Next();
  }
  while ( !InQry.Eof ) {
    TTrip trip;
    trip_id = fillTrip( trip, InQry );
    trips_in.insert( make_pair( trip_id, trip ) );
    InQry.Next();    
  }
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );  
  dataNode = NewTextChild( dataNode, "trips" );
  map<int,TTrip>::iterator itrip=trips.begin(), jtrip=trips_in.begin();
  while ( itrip != trips.end() || jtrip != trips_in.end() ) {
    xmlNodePtr tripNode = NewTextChild( dataNode, "trip" );
    if ( itrip != trips.end() && jtrip != trips_in.end() ) {
      if ( itrip->first == jtrip->first ) {
        NewTextChild( tripNode, "trip_id", itrip->first );
        buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
        buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
        itrip++;
        jtrip++;
      }
      else
        if ( itrip->first < jtrip->first ) {
          NewTextChild( tripNode, "trip_id", itrip->first );
          buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );          
          itrip++;
        }
        else {
          NewTextChild( tripNode, "trip_id", jtrip->first );
          buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
          jtrip++;
        }
    }
    else
      if ( itrip == trips.end() ) {
        NewTextChild( tripNode, "trip_id", jtrip->first );
        buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
        jtrip++;
      }
      else {
        NewTextChild( tripNode, "trip_id", itrip->first );
        buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
        itrip++;
      } 
  }
    
}

void SoppInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
