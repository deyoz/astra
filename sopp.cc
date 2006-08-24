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

struct TTrip {
  bool build;
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
  TTrip() {
    build = false;
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
  trip.build = true;
}

void SoppInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
  OutQry.Execute();
  InQry.Execute();
  RegQry.Execute();
  ResaQry.Execute();
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
  map<int,TTrip>::iterator itrip, jtrip;
  /* заполняем вылет и если найдем, то и прилет */
  for ( itrip=trips.begin(); itrip!=trips.end(); itrip++ ) {
    xmlNodePtr tripNode = NewTextChild( dataNode, "trip" );
    NewTextChild( tripNode, "trip_id", itrip->first );    
    buildTrip( itrip->second, NewTextChild( tripNode, "out" ) );
    jtrip = trips_in.find( itrip->first );
    if ( jtrip != trips_in.end() ) { /* есть прилет */
      buildTrip( jtrip->second, NewTextChild( tripNode, "in" ) );
    }
  }
  /* заполняем оставшийся прилет */
  for ( itrip=trips_in.begin(); itrip!=trips_in.end(); itrip++ ) {
    xmlNodePtr tripNode = NewTextChild( dataNode, "trip" );
    NewTextChild( tripNode, "trip_id", itrip->first );    
    if ( itrip->second.build )
      continue;
    buildTrip( itrip->second, NewTextChild( tripNode, "in" ) );
  }
 
  
    
/*       'SELECT * FROM('+
       'SELECT trips.trip_id as trip_id,trips.move_row_id as move_row_id,'+
       'trips_in.move_row_id as move_row_id_in,trips.trip as trip,trips_in.trip as trip_in,'+
       'trips.company as company,trips_in.company as company_in,trips.scd as scd,trips_in.scd as scd_in,'+
       'trips.est as est,trips_in.est as est_in,trips.act as act,trips_in.act as act_in,'+
       'trips.bc as bc,trips.bort as bort,trips.park as park,'+
       'trips.status as statusnum,NVL(trips_in.status,-1) as statusnum_in,'+
       'b.reg as reg,d.resa as resa,trips.triptype as triptype,trips.litera as litera,'+
       'SUBSTR( ckin.get_classes( trips.trip_id ), 1, 24 ) as classes,'+
       'trips.remark as remark '+
       'FROM trips,trips_in,(SELECT trips.trip_id, SUM(pax.seats) as reg FROM trips, pax_grp, pax '+
       'WHERE (trips.trip_id=pax_grp.point_id)AND(pax_grp.grp_id=pax.grp_id)AND(pax.pr_brd IS NOT NULL) GROUP BY trips.trip_id) b,'+
       '(SELECT point_id AS trip_id,SUM(crs_ok) as resa FROM counters2 GROUP BY point_id) d '+
       'WHERE (trips.status!=-1)AND(trips.trip_id=trips_in.trip_id(+))AND'+
       '(trips.trip_id=b.trip_id(+))AND(trips.trip_id=d.trip_id(+)) '+
       'UNION '+
       'SELECT trips_in.trip_id,TO_NUMBER(NULL),TO_NUMBER(trips_in.move_row_id),'+
       'TO_CHAR(NULL),trips_in.trip,TO_CHAR(NULL),trips_in.company,TO_DATE(NULL),trips_in.scd,'+
       'TO_DATE(NULL),trips_in.est,TO_DATE(NULL),trips_in.act,trips_in.bc,trips_in.bort,trips_in.park,'+
       '-1,trips_in.status,'+
       'TO_NUMBER(NULL),'+
       'TO_NUMBER(NULL),TO_CHAR(NULL),TO_CHAR(NULL),TO_CHAR(NULL),'+
       'TO_CHAR(NULL) '+
       'FROM trips_in,trips WHERE (trips_in.status!=-1)AND(trips.trip_id(+)=trips_in.trip_id)AND'+
       '((trips.status=-1)OR(trips.trip_id IS NULL))) '+
       ' ORDER BY trip_id';*/
    	
}

void SoppInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
