#include <stdlib.h>
#include "tripinfo.h"
#include "basic.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include <map>
#include "stages.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

class TSQLWhere
{
private:
  map<string,string> sqltrips;
  static TSQLWhere *Instance() {
    static TSQLWhere *instance_ = 0;
    if ( !instance_ )
      instance_ = new TSQLWhere();
    return instance_;    
  }  
public:
  TSQLWhere() {
    sqltrips[ "CENT.EXE" ] = 
       "/*NVL(est,scd) BETWEEN SYSDATE-1 AND SYSDATE+1 AND act IS NULL AND*/ "\
       " trips.status=0 ";    	
  }      
  static string &tripswhere( const string &screen ) {    
    return Instance()->sqltrips[ screen ];
  }  
  
};


void TripInfoInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//!!! убрать коментарии в sql запросе
  ProgTrace(TRACE5, "TripInfoInterface::ReadTrips" );
  TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );  
  TSQLWhere SQLWhere;
  TQuery Qry( &OraSession );
  Qry.Clear();
  string sql = "SELECT trips.trip_id, "\
               "       trip||DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'', "\
               "                    TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))|| "\
               "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "\
               "              TO_CHAR(scd,'(DD)')) AS str "\
               " FROM trips "\
               "WHERE " + TSQLWhere::tripswhere( TReqInfo::Instance()->screen ) + 
               " ORDER BY NVL(act,NVL(est,scd)) ";
  Qry.SQLText = sql;
  Qry.Execute();
  xmlNodePtr tripsNode = NewTextChild( dataNode, "trips" );
  while ( !Qry.Eof ) {
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "trip_id", Qry.FieldAsInteger( "trip_id" ) );
    NewTextChild( tripNode, "str", Qry.FieldAsString( "str" ) );
    Qry.Next();
  } 
};

void TripInfoInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT  trips.trip_id, "\
                "        trips.bc, "\
                "        SUBSTR(ckin.get_classes(trips.trip_id),1,255) AS classes, "\
                "        SUBSTR(ckin.get_places(trips.trip_id),1,255) AS places, "\
                "        scd, est, act, "\
                "        trips.triptype, "\
                "        trips.litera, "\
                "        trips.remark, "\
                "        comp.pr_saloninit "\
                " FROM  trips, "\
                " (SELECT COUNT(*) AS pr_saloninit FROM trip_comp_elems "\
                "   WHERE trip_id=:trip_id AND rownum<2) comp "\
                "  WHERE trips.trip_id= :trip_id AND "\
                "/* NVL(est,scd) BETWEEN SYSDATE-1 AND SYSDATE+1 AND act IS NULL AND */ "\
                " trips.status=0 ";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.SetVariable( "trip_id", point_id );
  Qry.Execute();  
  TTripStages tripstages( point_id );  
  if ( !Qry.RowCount() )
    showErrorMessage( "Информация о рейсе недоступна" );
  else {
    xmlNodePtr node = NewTextChild( dataNode, "tripheader" );
    NewTextChild( node, "trip_id", Qry.FieldAsInteger( "trip_id" ) );
    NewTextChild( node, "bc", Qry.FieldAsString( "bc" ) );
    NewTextChild( node, "classes", Qry.FieldAsString( "classes" ) );
    NewTextChild( node, "places", Qry.FieldAsString( "places" ) );
    TDateTime brd_to = tripstages.time( sCloseBoarding );
    NewTextChild( node, "brd_to", DateTimeToStr( brd_to, "hh:nn" ) );
    TDateTime takeoff;
    if ( !Qry.FieldIsNULL( "act" ) )
      takeoff = Qry.FieldAsDateTime( "act" );
    else
      if ( !Qry.FieldIsNULL( "est" ) )
        takeoff = Qry.FieldAsDateTime( "est" );
      else
        takeoff = Qry.FieldAsDateTime( "scd" );
    NewTextChild( node, "takeoff", DateTimeToStr( takeoff, "hh:nn" ) );
    NewTextChild( node, "triptype", Qry.FieldAsString( "triptype" ) );
    NewTextChild( node, "litera", Qry.FieldAsString( "litera" ) );
    TStage stage;
    if ( !Qry.FieldIsNULL( "act" ) )
      stage = sTakeoff;
    else 
      stage = tripstages.getStage( stCheckIn );
    NewTextChild( node, "ckin_stage", stage );
    if ( !Qry.FieldIsNULL( "act" ) )
      stage = sTakeoff;
    else stage = tripstages.getStage( stCraft );
    NewTextChild( node, "craft_stage", stage );
    NewTextChild( node, "remark", Qry.FieldAsString( "remark" ) );
    NewTextChild( node, "pr_saloninit", Qry.FieldAsInteger( "pr_saloninit" ) );    
  }	
}

void TripInfoInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  vector<TCounterItem> counters;
  /*считаем информацию по классам и п/н из Counters2 */
  TQuery Qry( &OraSession );  
  Qry.SQLText = "SELECT counters2.class, "\
                "       counters2.airp, "\
                "       trip_classes.cfg, "\
                "       counters2.crs_ok, "\
                "       counters2.crs_tranzit "\
                " FROM counters2,classes,trip_classes "\
                " WHERE counters2.class=classes.id AND "\
                "       counters2.point_id=trip_classes.trip_id AND "\
                "       counters2.class=trip_classes.class AND "\
                "       counters2.point_id=:point_id "\
                " ORDER BY classes.lvl,counters2.point_num ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  TCounterItem counterItem;
  while ( !Qry.Eof ) {
    counterItem.cl = Qry.FieldAsString( "class" );
    counterItem.target = Qry.FieldAsString( "airp" );
    counterItem.cfg = Qry.FieldAsInteger( "cfg" );
    counterItem.resa = Qry.FieldAsInteger( "crs_ok" );
    counterItem.tranzit = Qry.FieldAsInteger( "crs_tranzit" );
    counters.push_back( counterItem );
    Qry.Next();
  }
  
  /*считаем цифровую информацию о пассажирах и багаже по классам, п/н рейса, п/н трансфера и залам */
  Qry.Clear();
  Qry.SQLText = "SELECT a.class,a.target,DECODE(a.last_trfer,' ',NULL,a.last_trfer) AS last_trfer, "\
                "       halls2.id AS hall_id,halls2.name AS hall_name, "\
                "       a.seats,a.adult,a.child,a.baby,b.foreigner,d.excess, "\
                "       b.umnr,b.vip,c.bagAmount,c.bagWeight,c.rkWeight "\
                " FROM halls2, "\
                "( SELECT class,target,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(seats) AS seats, "\
                "         SUM(DECODE(pers_type,'ВЗ',1,0)) AS adult, "\
                "         SUM(DECODE(pers_type,'РБ',1,0)) AS child, "\
                "         SUM(DECODE(pers_type,'РМ',1,0)) AS baby "\
                "   FROM pax_grp,pax,v_last_trfer "\
                "  WHERE pax_grp.grp_id=pax.grp_id AND "\
                "        pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        point_id=:point_id AND pr_brd IS NOT NULL "\
                "  GROUP BY class,target,last_trfer,hall ) a, "\
                "( SELECT class,target,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(DECODE(rem_code,'UMNR',1,0)) AS umnr, "\
                "         SUM(DECODE(rem_code,'VIP',1,0)) AS vip, "\
                "         SUM(DECODE(rem_code,'FRGN',1,0)) AS foreigner "\
                "   FROM pax_grp,pax,v_last_trfer, "\
                "  (SELECT DISTINCT pax_id,rem_code FROM pax_rem "\
                "    WHERE rem_code IN ('UMNR','VIP','FRGN')) pax_rem "\
                "  WHERE pax_grp.grp_id=pax.grp_id AND "\
                "        pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        pax.pax_id=pax_rem.pax_id AND "\
                "        point_id=:point_id AND pr_brd IS NOT NULL "\
                "  GROUP BY class,target,last_trfer,hall) b, "\
                "( SELECT class,target,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "         SUM(DECODE(pr_cabin,0,amount,0)) AS bagAmount, "\
                "         SUM(DECODE(pr_cabin,0,weight,0)) AS bagWeight, "\
                "         SUM(DECODE(pr_cabin,0,0,weight)) AS rkWeight "\
                "   FROM pax_grp,v_last_trfer,bag2 "\
                "  WHERE pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "        pax_grp.grp_id=bag2.grp_id AND "\
                "        point_id=:point_id AND pr_refuse=0 AND pr_wl=0 "\
                "  GROUP BY class,target,last_trfer,hall) c, "\
                "(SELECT class,target,NVL(last_trfer,' ') AS last_trfer,hall, "\
                "        SUM(excess) AS excess "\
                "  FROM pax_grp,v_last_trfer "\
                " WHERE pax_grp.grp_id=v_last_trfer.grp_id(+) AND "\
                "       point_id=:point_id AND pr_refuse=0 AND pr_wl=0 "\
                " GROUP BY class,target,last_trfer,hall) d "\
                "WHERE a.hall=halls2.id AND "\
                "      a.class=b.class(+) AND "\
                "      a.target=b.target(+) AND "\
                "      a.last_trfer=b.last_trfer(+) AND "\
                "      a.hall=b.hall(+) AND "\
                "      a.class=c.class(+) AND "\
                "      a.target=c.target(+) AND "\
                "      a.last_trfer=c.last_trfer(+) AND "\
                "      a.hall=c.hall(+) AND "\
                "      a.class=d.class(+) AND "\
                "      a.target=d.target(+) AND "\
                "      a.last_trfer=d.last_trfer(+) AND "\
                "      a.hall=d.hall(+) ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  string cl,target;
  vector<TCounterItem>::iterator c;
  while ( !Qry.Eof ) {
    cl = Qry.FieldAsString( "class" );
    target = Qry.FieldAsString( "target" );
    for ( c=counters.begin(); c!=counters.end(); c++ ) {
      if ( c->cl == cl && c->target == target )
        break;
    }
    if ( c == counters.end() ) {
      TCounterItem counterItem;
      counterItem.cl = Qry.FieldAsString( "class" );
      counterItem.target = Qry.FieldAsString( "target" );
      counterItem.cfg = 0;
      counterItem.resa = 0;
      counterItem.tranzit = 0;
      c = counters.insert( counters.end(), counterItem );
    }
    TTrferItem TrferItem;
    TrferItem.last_trfer = Qry.FieldAsString( "last_trfer" );
    TrferItem.hall_id = Qry.FieldAsInteger( "hall_id" );
    TrferItem.hall_name = Qry.FieldAsString( "hall_name" );
    TrferItem.seats = Qry.FieldAsInteger( "seats" );
    TrferItem.adult = Qry.FieldAsInteger( "adult");
    TrferItem.child = Qry.FieldAsInteger( "child" );
    TrferItem.baby = Qry.FieldAsInteger( "baby" );
    TrferItem.foreigner = Qry.FieldAsInteger( "foreigner" );
    TrferItem.umnr = Qry.FieldAsInteger( "umnr" );
    TrferItem.vip = Qry.FieldAsInteger( "vip" );
    TrferItem.rkWeight = Qry.FieldAsInteger( "rkWeight" );
    TrferItem.bagAmount = Qry.FieldAsInteger( "bagAmount" );
    TrferItem.bagWeight = Qry.FieldAsInteger( "bagWeight" );
    TrferItem.excess = Qry.FieldAsInteger( "excess" );
    c->trfer.push_back( TrferItem ); 
    Qry.Next();
  }
  /* считаем информацию о досылаемом багаже по п/н рейса */
  Qry.Clear();
  Qry.SQLText = "SELECT target,SUM(amount) AS amount,SUM(weight) AS weight "\
                " FROM dos_bag WHERE trip_id=:point_id "\
                " GROUP BY target ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    target = Qry.FieldAsString( "target" );
    for ( c=counters.begin(); c!=counters.end(); c++ ) {
      if ( c->cl.empty() && c->target == target )
        break;
    }
    if ( c == counters.end() ) {
      TCounterItem counterItem;
      counterItem.target = Qry.FieldAsString( "target" );
      counterItem.cfg = 0;
      counterItem.resa = 0;
      counterItem.tranzit = 0;
      c = counters.insert( counters.end(), counterItem );
    }
    TTrferItem TrferItem;
    TrferItem.seats = 0;
    TrferItem.adult = 0;
    TrferItem.child = 0;
    TrferItem.baby = 0;
    TrferItem.foreigner = 0;
    TrferItem.umnr = 0;
    TrferItem.vip = Qry.FieldAsInteger( "vip" );
    TrferItem.rkWeight = 0;
    TrferItem.bagAmount = Qry.FieldAsInteger( "bagAmount" );
    TrferItem.bagWeight = Qry.FieldAsInteger( "bagWeight" );
    TrferItem.excess = 0;
    c->trfer.push_back( TrferItem ); 
    Qry.Next();
  }
  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );	
  xmlNodePtr counterItemNode, TrferNode, TrferItemNode;
  for ( vector<TCounterItem>::iterator c=counters.begin(); c!=counters.end(); c++ ) {
    counterItemNode = NewTextChild( node, "counteritem" );
    NewTextChild( counterItemNode, "cl",  c->cl );
    NewTextChild( counterItemNode, "target", c->target );
    NewTextChild( counterItemNode, "cfg", c->cfg );
    NewTextChild( counterItemNode, "resa", c->resa );
    NewTextChild( counterItemNode, "tranzit", c->tranzit );
    TrferNode = NewTextChild( counterItemNode, "trfer" );
    for ( vector<TTrferItem>::iterator trfer=c->trfer.begin(); trfer!=c->trfer.end(); trfer++ ) {
      TrferItemNode = NewTextChild( TrferNode, "trferitemnode" );
      NewTextChild( TrferItemNode, "last_trfer", trfer->last_trfer ); 
      NewTextChild( TrferItemNode, "hall_id", trfer->hall_id ); 
      NewTextChild( TrferItemNode, "hall_name", trfer->hall_name ); 
      NewTextChild( TrferItemNode, "seats", trfer->seats ); 
      NewTextChild( TrferItemNode, "adult", trfer->adult );
      NewTextChild( TrferItemNode, "child", trfer->child );
      NewTextChild( TrferItemNode, "baby", trfer->baby );
      NewTextChild( TrferItemNode, "foreigner", trfer->foreigner );
      NewTextChild( TrferItemNode, "umnr", trfer->umnr );
      NewTextChild( TrferItemNode, "vip", trfer->vip );
      NewTextChild( TrferItemNode, "rkweight", trfer->rkWeight );
      NewTextChild( TrferItemNode, "bagamount", trfer->bagAmount );
      NewTextChild( TrferItemNode, "bagweight", trfer->bagWeight );
      NewTextChild( TrferItemNode, "excess", trfer->excess );
    }    
  }  
}

void TripInfoInterface::ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace(TRACE5, "TripInfoInterface::ReadTrips, point_id=%d", point_id );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "point_id", point_id );
  if ( GetNode( "tripheader", reqNode ) ) /* Считать заголовок */
    readTripHeader( point_id, dataNode );    
  if ( GetNode( "counters", reqNode ) ) /* Считать заголовок */
    readTripCounters( point_id, dataNode );    
    
}


void TripInfoInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
