#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"
#include <map>
#include <vector>
#include <string>
#include "stages.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

struct TDest {
  string cod;
  string city;
  int pr_cancel;
  string land;
  string company;
  int trip;
  string bc;
  string litera;
  string triptype;
  string takeoff;
  int f;
  int c;
  int y;
  string unitrip;
  string suffix;	
};

typedef vector<TDest> TDests;

void SeasonInterface::DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );
    TQuery Qry(&OraSession);
    string sql = "begin ";
    sql += " delete ";
    sql += COMMON_ORAUSER();
    sql += ".routes where move_id = :move_id; ";
    sql += " delete ";
    sql += COMMON_ORAUSER();    
    sql += ".sched_days where move_id = :move_id; ";
    sql += " end; ";

    Qry.SQLText = sql;
    Qry.DeclareVariable("move_id", otInteger);
    xmlNodePtr curNode = NodeAsNode("Moves/move_id", reqNode);
    while(curNode) {
        Qry.SetVariable("move_id", NodeAsInteger(curNode));
        Qry.Execute();
        curNode = curNode->next;
    }
    TReqInfo::Instance()->MsgToLog("Удаление рейса ", evtSeason, NodeAsInteger("trip_id", reqNode));
}

void SeasonInterface::GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
    TQuery Qry(&OraSession);        
    Qry.SQLText = 
        "begin "
        "   season.get_spp(:vdata,:common_orauser); "
        "end; ";
    Qry.DeclareVariable("vdata", otDate);
    Qry.DeclareVariable( "common_orauser", otString );
    Qry.SetVariable("vdata", NodeAsDateTime("date", reqNode));
    Qry.SetVariable( "common_orauser", COMMON_ORAUSER() );
    Qry.Execute();
    showMessage("Данные успешно сохранены");
}

void SeasonInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
    TQuery NQry(&OraSession);        
    string sql = "SELECT ";
    sql += COMMON_ORAUSER();
    sql += ".routes_move_id.nextval AS move_id FROM dual";
    NQry.SQLText = sql;
    TQuery SQry(&OraSession);        
    TQuery RQry( &OraSession );
    int trip_id = 0;
    if(NodeIsNULL("trip_id", reqNode)) {
      sql = "SELECT ";
      sql += COMMON_ORAUSER();
      sql += ".routes_trip_id.nextval AS trip_id FROM dual";
      SQry.SQLText = sql;
      SQry.Execute();
      trip_id = SQry.FieldAsInteger(0);
    } else {
    	sql = "BEGIN ";
    	sql += "DELETE ";
    	sql += COMMON_ORAUSER();
    	sql += ".routes WHERE move_id IN (SELECT move_id FROM ";
    	sql += COMMON_ORAUSER();
    	sql += ".sched_days WHERE trip_id=:trip_id ); ";
    	sql += "DELETE ";
    	sql += COMMON_ORAUSER();
    	sql += ".sched_days WHERE trip_id=:trip_id; END; ";
        SQry.SQLText = sql;
        SQry.DeclareVariable("TRIP_ID", otInteger);
        trip_id = NodeAsInteger("trip_id", reqNode);
        SQry.SetVariable("TRIP_ID", trip_id);
        SQry.Execute();
    }

    SQry.Clear();
    sql = "INSERT INTO ";
    sql += COMMON_ORAUSER();
    sql += ".sched_days(trip_id,move_id,num,first_day,last_day,days,pr_cancel,tlg,reference) ";
    sql += "VALUES(:trip_id,:move_id,:num,:first_day,:last_day,:days,:pr_cancel,:tlg,:reference) ";
    SQry.SQLText = sql;
    SQry.DeclareVariable( "TRIP_ID", otInteger );
    SQry.DeclareVariable( "MOVE_ID", otInteger );
    SQry.DeclareVariable( "NUM", otInteger );
    SQry.DeclareVariable( "FIRST_DAY", otDate );
    SQry.DeclareVariable( "LAST_DAY", otDate );
    SQry.DeclareVariable( "DAYS", otString );
    SQry.DeclareVariable( "PR_CANCEL", otInteger );
    SQry.DeclareVariable( "TLG", otString );
    SQry.DeclareVariable( "REFERENCE", otString );

    RQry.Clear();
    sql = "INSERT INTO ";
    sql += COMMON_ORAUSER();
    sql += ".routes(move_id,num,cod,pr_cancel,land,company,trip,bc,takeoff,litera, ";
    sql += "triptype,f,c,y,unitrip,delta_in,delta_out,suffix) VALUES(:move_id,:num,:cod,:pr_cancel,:land, ";
    sql += ":company,:trip,:bc,:takeoff,:litera,:triptype,:f,:c,:y,:unitrip,:delta_in,:delta_out,:suffix) ";
    RQry.SQLText = sql;
    RQry.DeclareVariable( "MOVE_ID", otInteger );
    RQry.DeclareVariable( "NUM", otInteger );
    RQry.DeclareVariable( "COD", otString );
    RQry.DeclareVariable( "PR_CANCEL", otInteger );
    RQry.DeclareVariable( "LAND", otDate );
    RQry.DeclareVariable( "COMPANY", otString );
    RQry.DeclareVariable( "TRIP", otInteger );
    RQry.DeclareVariable( "BC", otString );
    RQry.DeclareVariable( "TAKEOFF", otDate );
    RQry.DeclareVariable( "LITERA", otString );
    RQry.DeclareVariable( "TRIPTYPE", otString );
    RQry.DeclareVariable( "F", otInteger );
    RQry.DeclareVariable( "C", otInteger );
    RQry.DeclareVariable( "Y", otInteger );
    RQry.DeclareVariable( "UNITRIP", otString );
    RQry.DeclareVariable( "DELTA_IN", otInteger );
    RQry.DeclareVariable( "DELTA_OUT", otInteger );
    RQry.DeclareVariable( "SUFFIX", otString );

    xmlNodePtr curNode = NodeAsNode("SubRangeList/SubRange", reqNode);

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "trip_id", trip_id);
    xmlNodePtr NewMovesNode = NewTextChild(dataNode, "NewMoves");
    int move_id, num;
    xmlNodePtr snode; 
    while(curNode) {        
        snode = curNode->children;
        num = NodeAsIntegerFast( "NUM", snode );
        bool FNewMove_id = GetNodeFast( "FNewMove_id", snode );
        if ( FNewMove_id ) {
          NQry.Execute();
          move_id = NQry.FieldAsInteger(0);
          xmlNodePtr NewMoveNode = NewTextChild(NewMovesNode, "NewMove");
          NewTextChild(NewMoveNode, "move_id", move_id);
          NewTextChild(NewMoveNode, "num", num );
        }        
        else
          move_id = NodeAsIntegerFast( "Move_id", snode );

        SQry.SetVariable( "TRIP_ID", trip_id);
        SQry.SetVariable( "MOVE_ID", move_id);
        SQry.SetVariable( "NUM", num );
        SQry.SetVariable( "FIRST_DAY", NodeAsDateTimeFast( "FFirst", snode ) );
        SQry.SetVariable( "LAST_DAY", NodeAsDateTimeFast( "FLast", snode ) );
        SQry.SetVariable( "DAYS", NodeAsStringFast( "FDays", snode ) );
        SQry.SetVariable( "PR_CANCEL", NodeAsIntegerFast( "Cancel", snode ) );
        SQry.SetVariable( "TLG", NodeAsStringFast( "Tlg", snode ) );
        SQry.SetVariable( "REFERENCE", NodeAsStringFast( "FReference", snode )  );
        SQry.Execute();

        xmlNodePtr curDestNode = GetNode("DestList/Dest", curNode);
        xmlNodePtr tnode;
        if(curDestNode) {
            while(curDestNode) {
                snode = curDestNode->children;
                RQry.SetVariable( "MOVE_ID", move_id );
                RQry.SetVariable( "NUM", NodeAsIntegerFast( "NUM", snode ) );
                RQry.SetVariable( "COD", NodeAsStringFast( "FCod", snode ) );
                RQry.SetVariable( "PR_CANCEL", NodeAsIntegerFast( "Pr_Cancel", snode ) );
                tnode = GetNodeFast( "land", snode );
                if ( tnode )
                  RQry.SetVariable("LAND", NodeAsDateTime( tnode ) );
                else 
                  RQry.SetVariable("LAND", FNull);
                tnode = GetNodeFast( "company", snode );
                if ( tnode ) {
                  RQry.SetVariable( "COMPANY", NodeAsString( tnode ) );
                  ProgTrace( TRACE5, "company=%s", NodeAsString( tnode ) );
                }
                else
                  RQry.SetVariable( "COMPANY", FNull );
                tnode = GetNodeFast( "trip", snode );
                if( tnode )
                  RQry.SetVariable("TRIP", NodeAsInteger( tnode ) );
                else 
                  RQry.SetVariable( "TRIP", FNull );
                tnode = GetNodeFast( "bc", snode );
                if ( tnode )
                  RQry.SetVariable( "BC", NodeAsString( tnode ) );
                else
                  RQry.SetVariable( "BC", FNull );
                tnode = GetNodeFast( "takeoff", snode );
                if ( tnode )
                  RQry.SetVariable( "TAKEOFF", NodeAsDateTime( tnode ) );
                else
                  RQry.SetVariable( "TAKEOFF", FNull );
                RQry.SetVariable( "TRIPTYPE", NodeAsStringFast( "FTriptype", snode ) );
                RQry.SetVariable( "F", NodeAsIntegerFast( "FF", snode ) );
                RQry.SetVariable( "C", NodeAsIntegerFast( "FC", snode ) );
                RQry.SetVariable( "Y", NodeAsIntegerFast( "FY", snode ) );
                RQry.SetVariable( "LITERA", NodeAsStringFast( "Litera", snode ) );
                RQry.SetVariable( "UNITRIP", NodeAsStringFast( "FUnitrip", snode ) );
                RQry.SetVariable( "DELTA_IN", NodeAsIntegerFast( "delta_in", snode ) );
                RQry.SetVariable( "DELTA_OUT", NodeAsIntegerFast( "delta_out", snode )  );
                RQry.SetVariable( "SUFFIX", NodeAsStringFast( "FSuffix", snode )  );

                RQry.Execute();
                curDestNode = curDestNode->next;
            }
        }

        curNode = curNode->next;
    }
    TReqInfo::Instance()->MsgToLog("Изменение характеристик рейса ", evtSeason, trip_id);
}

void SeasonInterface::Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  map<int,TDests> mapds;
  TReqInfo::Instance()->user.check_access( amRead );	
  TPerfTimer tm;
  TQuery SQry( &OraSession );
  SQry.SQLText = "SELECT cod FROM options"; /*???!!!*/
  SQry.Execute();
  if ( !SQry.RowCount() )
    throw Exception( "table options is empty" );
  string airp = SQry.FieldAsString( "cod" );
  string sql;
  sql = "SELECT TO_CHAR( winter, 'HH:DD.MM.Y' ) as winter, TO_CHAR( summer, 'HH:DD.MM.Y' ) as summer FROM ";
  sql += COMMON_ORAUSER();
  sql += ".options";
  SQry.Clear();
  SQry.SQLText = sql;
  SQry.Execute();
  if ( !SQry.RowCount() )
    throw Exception( "table options is empty" );
  xmlNodePtr dataNode = NewTextChild(resNode, "data");                  
  NewTextChild( dataNode, "airp", airp );
  NewTextChild( dataNode, "winter", SQry.FieldAsString( "winter" ) );
  NewTextChild( dataNode, "summer", SQry.FieldAsString( "summer" ) );
  SQry.Clear();
  sql = "SELECT trip_id,move_id,first_day,last_day,days,pr_cancel,tlg,reference ";
  sql += " FROM ";
  sql += COMMON_ORAUSER();
  sql += ".sched_days ";
  sql += "ORDER BY trip_id,move_id,num";
  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  SQry.SQLText = sql;
  SQry.Execute();
  int idx_trip_id = SQry.FieldIndex("trip_id");
  int idx_smove_id = SQry.FieldIndex("move_id");
  int idx_first_day = SQry.FieldIndex("first_day");
  int idx_last_day = SQry.FieldIndex("last_day");
  int idx_days = SQry.FieldIndex("days");
  int idx_scancel = SQry.FieldIndex("pr_cancel");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_reference = SQry.FieldIndex("reference");  
  
  if ( !SQry.RowCount() )
    throw UserException( "В расписании отсутствуют рейсы" );
  TQuery RQry( &OraSession );
  sql = "SELECT move_id,routes.cod cod,airps.city city,pr_cancel,land+delta_in land,company,"\
        "       trip,bc,litera,triptype,takeoff+delta_out takeoff,f,c,y,unitrip,suffix ";
  sql += " FROM ";
  sql += COMMON_ORAUSER();
  sql += ".routes, airps ";
  sql += "WHERE routes.cod=airps.cod "\
         "ORDER BY move_id,num";

  ProgTrace( TRACE5, "sql=%s", sql.c_str() );
  RQry.SQLText = sql;
  RQry.Execute();  
  
  ProgTrace(TRACE5, "Executing qry %ld", tm.Print());  
  tm.Init();  
  
  int idx_rmove_id = RQry.FieldIndex("move_id");
  int idx_cod = RQry.FieldIndex("cod");
  int idx_city = RQry.FieldIndex("city");
  int idx_rcancel = RQry.FieldIndex("pr_cancel");
  int idx_land = RQry.FieldIndex("land");
  int idx_company = RQry.FieldIndex("company");
  int idx_trip = RQry.FieldIndex("trip");
  int idx_bc = RQry.FieldIndex("bc");
  int idx_takeoff = RQry.FieldIndex("takeoff");
  int idx_litera = RQry.FieldIndex("litera");
  int idx_triptype = RQry.FieldIndex("triptype");
  int idx_f = RQry.FieldIndex("f");
  int idx_c = RQry.FieldIndex("c");
  int idx_y = RQry.FieldIndex("y");
  int idx_unitrip = RQry.FieldIndex("unitrip");
  int idx_suffix = RQry.FieldIndex("suffix");

  int move_id = -1;
  TDests ds;
  TDest d;
  while ( !RQry.Eof ) {
    if ( move_id != RQry.FieldAsInteger( idx_rmove_id ) ) {
      if ( move_id >= 0 ) {
        mapds.insert(std::make_pair( move_id, ds ) );
        ds.clear();
      }
      move_id = RQry.FieldAsInteger( idx_rmove_id );
    }
    d.cod = RQry.FieldAsString( idx_cod );
    d.city = RQry.FieldAsString( idx_city );
    d.pr_cancel = RQry.FieldAsInteger( idx_rcancel );    
    if ( RQry.FieldIsNULL( idx_land ) )
      d.land = "";
    else
      d.land = DateTimeToStr( RQry.FieldAsDateTime( idx_land ) );
    d.company = RQry.FieldAsString( idx_company );
    if ( RQry.FieldIsNULL( idx_trip ) )
      d.trip = -1;
    else
      d.trip = RQry.FieldAsInteger( idx_trip );
    d.bc = RQry.FieldAsString( idx_bc );
    d.litera = RQry.FieldAsString( idx_litera );
    d.triptype = RQry.FieldAsString( idx_triptype );
    if ( RQry.FieldIsNULL( idx_takeoff ) )
      d.takeoff = "";
    else
      d.takeoff = DateTimeToStr( RQry.FieldAsDateTime( idx_takeoff ) );      	  
    d.f = RQry.FieldAsInteger( idx_f );
    d.c = RQry.FieldAsInteger( idx_c );
    d.y = RQry.FieldAsInteger( idx_y );
    d.unitrip = RQry.FieldAsString( idx_unitrip );
    d.suffix = RQry.FieldAsString( idx_suffix );
    ds.push_back( d );
    RQry.Next();
  }
  mapds.insert(std::make_pair( move_id, ds ) );
  ProgTrace(TRACE5, "getdata %ld", tm.Print());  
    
  xmlNodePtr rangeListNode, destsNode = NULL;
  xmlNodePtr destNode;
  int trip_id = -1;
  move_id = -1;
  int t;  
  string s;
  while ( !SQry.Eof ) {
    if ( trip_id != SQry.FieldAsInteger( idx_trip_id ) ) {
      rangeListNode = NewTextChild(dataNode, "rangeList");
      trip_id = SQry.FieldAsInteger( idx_trip_id );      
      NewTextChild( rangeListNode, "trip_id", trip_id );
    }
    xmlNodePtr range = NewTextChild( rangeListNode, "range" );
    move_id = SQry.FieldAsInteger( idx_smove_id );
    NewTextChild( range, "move_id", move_id );
    NewTextChild( range, "first", DateTimeToStr( SQry.FieldAsDateTime( idx_first_day ) ) );    
    NewTextChild( range, "last", DateTimeToStr( SQry.FieldAsDateTime( idx_last_day ) ) );    
    NewTextChild( range, "days", SQry.FieldAsString( idx_days ) );
    t = SQry.FieldAsInteger( idx_scancel );
    if ( t )
      NewTextChild( range, "cancel", t );
    s = SQry.FieldAsString( idx_tlg );
    if ( !s.empty() )
      NewTextChild( range, "tlg", s );
    s = SQry.FieldAsString( idx_reference );
    if ( !s.empty() )
      NewTextChild( range, "ref", s );      
    ds = mapds[ move_id ];
    if ( !ds.empty() ) {
      destsNode = NewTextChild( range, "dests" );
      for ( TDests::iterator id=ds.begin(); id!=ds.end(); id++ ) {
      	destNode = NewTextChild( destsNode, "dest" );
      	NewTextChild( destNode, "cod", id->cod );
      	if ( id->cod != id->city )
      	  NewTextChild( destNode, "city", id->city );      	
      	if ( id->pr_cancel )
      	  NewTextChild( destNode, "cancel", id->pr_cancel );
      	if ( !id->land.empty() )
      	  NewTextChild( destNode, "land", id->land );
      	if ( !id->company.empty() )
      	  NewTextChild( destNode, "company", id->company ); 
      	if ( id->trip >= 0 )
      	  NewTextChild( destNode, "trip", id->trip );
      	if ( !id->bc.empty() )
      	  NewTextChild( destNode, "bc", id->bc );
      	if ( !id->litera.empty() )
      	  NewTextChild( destNode, "litera", id->litera );      	
      	if ( id->triptype != "п" )
      	  NewTextChild( destNode, "triptype", id->triptype );      	      	  
      	if ( !id->takeoff.empty() )
      	  NewTextChild( destNode, "takeoff", id->takeoff );      	  
      	if ( id->f )
      	  NewTextChild( destNode, "f", id->f );
      	if ( id->c )
      	  NewTextChild( destNode, "c", id->c );
      	if ( id->y )
      	  NewTextChild( destNode, "y", id->y );      	
      	if ( !id->unitrip.empty() )
      	  NewTextChild( destNode, "unitrip", id->unitrip );      	      	        	  
      	if ( !id->suffix.empty() )
      	  NewTextChild( destNode, "suffix", id->suffix );      	      	        	        	        	
      }
      ProgTrace( TRACE5, "move_id=%d", move_id );
      mapds[ move_id ].clear(); /* уже использовали маршрут */
    }
    SQry.Next();
  }
  ProgTrace(TRACE5, "build %ld", tm.Print());  
}

void SeasonInterface::RemovalGangWayTimes(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amRead );	
  vector<TStageTimes> st;
  GetStageTimes( st, sRemovalGangWay );
  xmlNodePtr dataNode = NewTextChild(resNode, "data");                  
  xmlNodePtr node = NewTextChild(dataNode, "times"); 
  xmlNodePtr stnode;
  for (vector<TStageTimes>::iterator i=st.begin(); i!=st.end(); i++ ) {
    stnode = NewTextChild( node, "time" );
    NewTextChild( stnode, "craft", i->craft );
    NewTextChild( stnode, "trip_type", i->trip_type );
    NewTextChild( stnode, "time", i->time );
  }  	
}

void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
