#include <stdlib.h>
#include "tripinfo.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "stages.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "oralib.h"
#include "xml_unit.h"

using namespace std;

struct TTrferItem {
  std::string last_trfer;
  int hall_id;
  std::string hall_name;
  int seats,adult,child,baby,foreigner;
  int umnr,vip,rkWeight,bagAmount,bagWeight,excess;
};


struct TCounterItem {
  std::string cl;
  std::string target;
  std::vector<TTrferItem> trfer;
  int cfg,resa,tranzit;
};


void TSQLParams::addVariable( TVar &var ) 
{
  vars.push_back( var );
}

void TSQLParams::addVariable( string aname, otFieldType atype, string avalue ) 
{
  TVar var( aname, atype, avalue );
  vars.push_back( var );
}

void TSQLParams::clearVariables( ) 
{
  vars.clear();
}

void TSQLParams::setVariables( TQuery &Qry ) {
  Qry.ClearVariables();
  for ( std::vector<TVar>::iterator ip=vars.begin(); ip!=vars.end(); ip++ ) {
    Qry.CreateVariable( ip->name, ip->type, ip->value );
    ProgTrace( TRACE5, "Qry.CreateVariable name=%s, value=%s", ip->name.c_str(), ip->value.c_str() );
  }
}

TSQL::TSQL() {      	
 /* в этом конструкторе задаются окончания запроса по рейсам и переменные участв. в запросе */
 createSQLTrips();
}      

TSQL *TSQL::Instance() {
  static TSQL *instance_ = 0;
  if ( !instance_ )
    instance_ = new TSQL();
  return instance_;    
}  

void TSQL::createSQLTrips( ) {
  sqltrips[ "CENT.EXE" ].sqlfrom = 
    " FROM trips "\
    "WHERE act IS NULL AND trips.status=0 "\
    " AND NVL(est,scd) BETWEEN SYSDATE-1 AND SYSDATE+1  ";
  TSQLParams p;  	      
  p.sqlfrom = 
    " FROM trips "\
    "WHERE act IS NULL AND trips.status=0 AND "\
    "      gtimer.is_final_stage(trips.trip_id, :ckin_stage_type, :no_active_stage_id)=0  ";
  p.addVariable( "ckin_stage_type", otInteger, IntToString( stCheckIn ) );
  p.addVariable( "no_active_stage_id",  otInteger, IntToString( sNoActive ) );
  sqltrips[ "PREPREG.EXE" ] = p;
  p.clearVariables();  	    
  /* задаем текст */
  p.sqlfrom = 
    " FROM "\
    "    trips, "\
    "    trip_stations "\
    "WHERE trips.act IS NULL AND trips.status=0 AND "\
    "    trips.trip_id=trip_stations.trip_id AND "\
    "    trip_stations.name= :station AND "\
    "    trip_stations.work_mode='П' AND "\
    "    gtimer.is_final_stage(  trips.trip_id, :brd_stage_type, :brd_open_stage_id) <> 0 ";
  /* задаем переменные */
  p.addVariable( "brd_stage_type", otInteger, IntToString( stBoarding ) );
  p.addVariable( "brd_open_stage_id", otInteger, IntToString( sOpenBoarding ) );      
  /* запоминаем */
  sqltrips[ "BRDBUS.EXE" ] = p;      
  /* не забываем очищать за собой переменные */
  p.clearVariables();  	
}

void TSQL::setSQLTrips( TQuery &Qry, const string &screen ) {    
  Qry.Clear();
  TSQLParams p = Instance()->sqltrips[ screen ];
  string sql = 
    "SELECT trips.trip_id, "\
    "       trip||DECODE(TRUNC(SYSDATE),TRUNC(NVL(act,NVL(est,scd))),'', "\
    "                    TO_CHAR(NVL(act,NVL(est,scd)),'/DD'))|| "\
    "       DECODE(TRUNC(NVL(act,NVL(est,scd))),TRUNC(scd),'', "\
    "              TO_CHAR(scd,'(DD)')) AS str " + p.sqlfrom +
    " ORDER BY NVL(act,NVL(est,scd)) ";  
  Qry.SQLText = sql;
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  p.setVariables( Qry );    
  if ( screen == "BRDBUS.EXE" )
   Qry.CreateVariable( "station", otString, TReqInfo::Instance()->desk.code );
}    

/*******************************************************************************/
void TripsInterface::ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "TripsInterface::ReadTrips" );
  TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );  
  TQuery Qry( &OraSession );
  TSQL::setSQLTrips( Qry, TReqInfo::Instance()->screen );
  tst();
  Qry.Execute();
  tst();
  xmlNodePtr tripsNode = NewTextChild( dataNode, "trips" );
  while ( !Qry.Eof ) {
    xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
    NewTextChild( tripNode, "trip_id", Qry.FieldAsInteger( "trip_id" ) );
    NewTextChild( tripNode, "str", Qry.FieldAsString( "str" ) );
    Qry.Next();
  } 
};

void readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripsInterface::readTripCounters" );	
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

void viewPNL( int point_id, xmlNodePtr dataNode )
{
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession );  
  Qry.SQLText = 
    "SELECT pnr_ref, "\
    "       RTRIM(surname||' '||name) full_name, "\
    "       pers_type, "\
    "       class, "\
    "       seat_no, "\
    "       target, "\
    "       report.get_trfer_airp(airp_arv) AS last_target, "\
    "       report.get_PSPT(pax_id) AS document, "\
    "       pax_id, "\
    "       crs_pnr.pnr_id "\
    " FROM crs_pnr,crs_pax,v_last_crs_trfer "\
    "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "\
    "      crs_pnr.pnr_id=v_last_crs_trfer.pnr_id(+) AND "\
    "      crs_pnr.point_id=:point_id AND "\
    "      crs_pax.pr_del=0 "\
    "ORDER BY DECODE(pnr_ref,NULL,0,1),pnr_ref,pnr_id ";	
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  RQry.SQLText =
    "SELECT crs_pax_rem.rem, crs_pax_rem.rem_code, NVL(remark.priority,-1) AS priority "\
    " FROM crs_pax_rem,remark "\
    "WHERE crs_pax_rem.rem_code=remark.cod(+) AND crs_pax_rem.pax_id=:pax_id "\
    "ORDER BY priority DESC,rem_code,rem ";
  RQry.DeclareVariable( "pax_id", otInteger );

  dataNode = NewTextChild( dataNode, "trippnl" );
  while ( !Qry.Eof ) {
    xmlNodePtr itemNode = NewTextChild( dataNode, "item" );
    NewTextChild( itemNode, "pnr_ref", Qry.FieldAsString( "pnr_ref" ) );
    NewTextChild( itemNode, "full_name", Qry.FieldAsString( "full_name" ) );
    NewTextChild( itemNode, "pers_type", Qry.FieldAsString( "pers_type" ) );
    NewTextChild( itemNode, "class", Qry.FieldAsString( "class" ) );
    NewTextChild( itemNode, "seat_no", Qry.FieldAsString( "seat_no" ) );
    NewTextChild( itemNode, "target", Qry.FieldAsString( "target" ) );
    NewTextChild( itemNode, "last_target", Qry.FieldAsString( "last_target" ) );
    RQry.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
    RQry.Execute();
    string rem, rem_code, rcode, ticket;
    while ( !RQry.Eof ) {
      rem += string( ".R/" ) + RQry.FieldAsString( "rem" ) + "   ";
      rem_code = RQry.FieldAsString( "rem_code" );
      rcode = rem_code;
      rcode = upperc( rcode );
      if ( rcode.find( "TKNO" ) != string::npos  && ticket.empty() ) {
      	ticket = RQry.FieldAsString( "rem" );
      }
      RQry.Next();
    }
    NewTextChild( itemNode, "ticket", ticket );
    NewTextChild( itemNode, "document", Qry.FieldAsString( "document" ) );    
    NewTextChild( itemNode, "rem", rem );
    NewTextChild( itemNode, "pax_id", Qry.FieldAsInteger( "pax_id" ) );    
    NewTextChild( itemNode, "pnr_id", Qry.FieldAsInteger( "pnr_id" ) );
    Qry.Next();
  }
}

void TripsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
