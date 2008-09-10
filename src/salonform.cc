#include <stdlib.h>
#include "salonform.h"
#include "basic.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "str_utils.h"
#include "stl_utils.h"
#include "images.h"
#include "salons.h"
#include "seats.h"
#include "convert.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "seats.h" // only for convert_salons

const char CurrName[] = " (ТЕК.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

bool filterComp( const string &airline, const string &airp );


void SalonsInterface::CheckInShow( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::CheckInShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons;
  Salons.trip_id = NodeAsInteger( "trip_id", reqNode );
  Salons.ClName = NodeAsString( "ClName", reqNode );
  bool PrepareShow = NodeAsInteger( "PrepareShow", reqNode );
  SetProp( resNode, "handle", "1" );
  xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
  SetProp( ifaceNode, "id", "SalonsInterface" );
  SetProp( ifaceNode, "ver", "1" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( PrepareShow )
    ImagesInterface::GetImages( reqNode, resNode );
  Salons.Read( rTripSalons );
  SALONS::GetTripParams( Salons.trip_id, dataNode );
  Salons.Build( NewTextChild( dataNode, "salons" ) );
};

void SalonsInterface::SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", trip_id );
  TQuery Qry( &OraSession );
  SALONS::GetTripParams( trip_id, dataNode );
  Qry.SQLText = "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if ( !Qry.RowCount() )
  	throw UserException( "Рейс не найден" );
  string trip_airline = Qry.FieldAsString( "airline" );
  Qry.Clear();
  Qry.SQLText = "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                "       comps.descr,0 as pr_comp, comps.airline, comps.airp "\
                " FROM comps, points "\
                "WHERE points.craft = comps.craft AND points.point_id = :point_id "\
                "UNION "\
                "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                "       comps.descr,1 as pr_comp, null, null "\
                " FROM comps, points, trip_sets "\
                "WHERE points.point_id=trip_sets.point_id AND "\
                "      points.craft = comps.craft AND points.point_id = :point_id AND "\
                "      trip_sets.comp_id = comps.comp_id "\
                "UNION "\
                "SELECT -1, craft, bort, "\
                "        LTRIM(RTRIM( DECODE( a.f, 0, '', ' П'||a.f)||"\
                "        DECODE( a.c, 0, '', ' Б'||a.c)|| "\
                "        DECODE( a.y, 0, '', ' Э'||a.y) )) classes, "\
                "        null,1, null, null "\
                "FROM "\
                "(SELECT -1, craft, bort, "\
                "        NVL( SUM( DECODE( class, 'П', 1, 0 ) ), 0 ) as f, "\
                "        NVL( SUM( DECODE( class, 'Б', 1, 0 ) ), 0 ) as c, "\
                "        NVL( SUM( DECODE( class, 'Э', 1, 0 ) ), 0 ) as y "\
                "  FROM trip_comp_elems, comp_elem_types, points, trip_sets "\
                " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
                "       comp_elem_types.pr_seat <> 0 AND "\
                "       trip_comp_elems.point_id = points.point_id AND "\
                "       points.point_id = :point_id AND "\
                "       trip_sets.point_id(+) = points.point_id AND "\
                "       trip_sets.comp_id IS NULL "\
                "GROUP BY craft, bort) a "\
                "ORDER BY comp_id, craft, bort, classes, descr";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  try {
    xmlNodePtr compsNode = NULL;
    string StrVal;
    while ( !Qry.Eof ) {
    	if ( Qry.FieldAsInteger( "pr_comp" ) || /* поиск компоновки только по компоновкам нужной А/К или портовым компоновкам */
    		   ( Qry.FieldIsNULL( "airline" ) || trip_airline == Qry.FieldAsString( "airline" ) ) &&
    		   filterComp( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      	if ( !compsNode )
      		compsNode = NewTextChild( dataNode, "comps"  );
        xmlNodePtr compNode = NewTextChild( compsNode, "comp" );
        if ( !Qry.FieldIsNULL( "airline" ) )
        	StrVal = Qry.FieldAsString( "airline" );
        else
        	StrVal = Qry.FieldAsString( "airp" );
        if ( StrVal.length() == 2 )        	
          StrVal += "  ";
        else
        	StrVal += " ";
        if ( !Qry.FieldIsNULL( "bort" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
          StrVal += Qry.FieldAsString( "bort" );
        else
          StrVal += "  ";
        StrVal += string( "  " ) + Qry.FieldAsString( "classes" );
        if ( !Qry.FieldIsNULL( "descr" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
          StrVal += string( "  " ) + Qry.FieldAsString( "descr" );
        if ( Qry.FieldAsInteger( "pr_comp" ) == 1 )
          StrVal += CurrName;
        NewTextChild( compNode, "name", StrVal );
        NewTextChild( compNode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
        NewTextChild( compNode, "pr_comp", Qry.FieldAsInteger( "pr_comp" ) );
        NewTextChild( compNode, "craft", Qry.FieldAsString( "craft" ) );
        NewTextChild( compNode, "bort", Qry.FieldAsString( "bort" ) );
        NewTextChild( compNode, "classes", Qry.FieldAsString( "classes" ) );
        NewTextChild( compNode, "descr", Qry.FieldAsString( "descr" ) );
      }
      Qry.Next();
    }
    if ( !compsNode )
      throw UserException( "Нет компоновок по данному типу ВС" );    
   tst();
   TSalons Salons;
   Salons.trip_id = trip_id;
   Salons.ClName.clear();
   tst();
   Salons.Read( rTripSalons );
   xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
   Salons.Build( salonsNode );
   if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, Passengers, Salons.getLatSeat() ) )  
 	   Passengers.Build( Salons, dataNode );     
 }
 catch( UserException ue ) {
   showErrorMessage( ue.what() );
   tst();
 }
}

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amRead );
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", SALONS::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

void SalonsInterface::SalonFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormWrite" );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.Execute();
  Qry.SQLText = "UPDATE trip_sets SET comp_id=:comp_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "comp_id", otInteger );
  if ( comp_id == -2 )
    Qry.SetVariable( "comp_id", FNull );
  else
    Qry.SetVariable( "comp_id", comp_id );
  TSalons Salons;
  Salons.Parse( NodeAsNode( "salons", reqNode ) );
  Salons.verifyValidRem( "MCLS", "Э"); //???
  Salons.trip_id = trip_id;
  Salons.ClName = "";
  Qry.Execute();
  tst();
  Salons.Write( rTripSalons );
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );  
  /* инициализация VIP */  
  SALONS::InitVIP( trip_id );    
  xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
  string msg = string( "Изменена компоновка рейса. Классы: " ) +
               NodeAsString( "classes", refcompNode );     	
  msg += string( ", кодировка: " ) + NodeAsString( "lang", refcompNode ); //???
  TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );   
  
  if ( pr_initcomp ) { /* изменение компоновки */  	
    xmlNodePtr ctypeNode = NodeAsNode( "ctype", refcompNode );
    bool cBase = false;
    bool cChange = false;
    if ( ctypeNode ) {
    	tst();
      ctypeNode = ctypeNode->children; /* value */
      while ( ctypeNode ) {
      	string stype = NodeAsString( ctypeNode );
        cBase = ( stype == string( "cBase" ) || cBase );
        cChange = ( stype == string( "cChange" ) || cChange );
        ctypeNode = ctypeNode->next;
      }
    }
    ProgTrace( TRACE5, "cBase=%d, cChange=%d", cBase, cChange );
    if ( cBase ) {
      msg = string( "Назначена базовая компоновка (ид=" ) +
            IntToString( comp_id ) +
            "). Классы: " + NodeAsString( "classes", refcompNode );
      if ( cChange )
        msg = string( "Назначена компоновка рейса. Классы: " ) +
              NodeAsString( "classes", refcompNode );
    }
    msg += string( ", кодировка: " ) + NodeAsString( "lang", refcompNode );
    TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );
  }
  SALONS::setTRIP_CLASSES( trip_id );
 
    
  /* перечитываение компоновки из БД */	
  Salons.Read( rTripSalons );                
  
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );
  Salons.Build( salonsNode );
  tst();
  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, Passengers, Salons.getLatSeat() ) )  
    Passengers.Build( Salons, dataNode );
}

void SalonsInterface::DeleteReserveSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  TQuery Qry( &OraSession );    
  Qry.SQLText = 
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );	
  
/*  string xname, yname;
  xmlNodePtr n = GetNode( "placename", reqNode );
  bool seats_exists = true;
  if ( n ) {
  	string placeName = NodeAsString( n );
    TQuery Qry( &OraSession );  	
    Qry.SQLText = 
      "SELECT xname, yname FROM trip_comp_elems "
       " WHERE point_id=:point_id AND yname||xname=DECODE( INSTR( :placename, '0' ), 1, SUBSTR( :placename, 2 ), :placename )";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "placename", otString, placeName );
    Qry.Execute();
    seats_exists = ( Qry.RowCount() );    	
    if ( seats_exists ) {
    	xname = Qry.FieldAsString( "xname" );
    	yname = Qry.FieldAsString( "yname" );
    }
  }
  else {
  	xname = NodeAsString( "xname", reqNode );
  	yname = NodeAsString( "yname", reqNode );
  }  */
  
	ProgTrace(TRACE5, "SalonsInterface::DeleteReserveSeat, point_id=%d, pax_id=%d, tid=%d", point_id, pax_id, tid );    
	
  TSalons Salons;
  Salons.trip_id = point_id;
  Salons.Read( rTripSalons );
  vector<SALONS::TSalonSeat> seats;
	
  try {
  	SEATS::ChangeLayer( cltPreseat, point_id, pax_id, tid, "", "", stDropseat, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	Qry.SQLText =
  	  "SELECT "
      "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "     
      "  salons.get_crs_seat_no(crs_pax.pax_id,:preseat_layer,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS preseat_no, "          
      "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no "          
      "FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "preseat_layer", otString, EncodeCompLayerType(ASTRA::cltPreseat) );
    Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );    
    Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "Пассажир не найден" );    
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "preseat_no" ) )
    	NewTextChild( dataNode, "preseat_no", Qry.FieldAsString( "preseat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );
   	SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
    TSalons Salons;
    Salons.trip_id = point_id;
    Salons.Read( rTripSalons );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );    
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( SEATS::GetPassengersForManualSeat( point_id, cltCheckin, Passengers, Salons.getLatSeat() ) )  
      Passengers.Build( Salons, dataNode );    
  	showErrorMessageAndRollback( ue.what() );
  }  
}

void SalonsInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amWrite );
  TSeatsType seat_type = stReseat;
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  string xname, yname;
  xmlNodePtr n = GetNode( "num", reqNode );
  TQuery Qry( &OraSession );  
  
  Qry.SQLText = 
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );	
  
  if ( n ) {
    int num = NodeAsInteger( "num", reqNode );
    int x = NodeAsInteger( "x", reqNode );
    int y = NodeAsInteger( "y", reqNode );
    Qry.SQLText = 
      "SELECT xname, yname FROM trip_comp_elems "
       " WHERE point_id=:point_id AND num=:num AND x=:x AND y=:y";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "num", otInteger, num );
    Qry.CreateVariable( "x", otInteger, x );
    Qry.CreateVariable( "y", otInteger, y );
    Qry.Execute();
    if ( Qry.RowCount() ) {
    	xname = Qry.FieldAsString( "xname" );
    	yname = Qry.FieldAsString( "yname" );
    }     
  }
  else {
  	xname = norm_iata_line( NodeAsString( "xname", reqNode ) );
  	yname = norm_iata_row( NodeAsString( "yname", reqNode ) );  	
  }
  TCompLayerType layer_type;
  if ( GetNode( "checkin", reqNode ) )
  	layer_type = cltCheckin;
  else
  	if ( GetNode( "reserve", reqNode ) )
  		layer_type = cltPreseat;
  	else layer_type = cltCheckin; // cltUnknown -new; это в страом терминале так сделано 
  ProgTrace(TRACE5, "SalonsInterface::Reseat, point_id=%d, pax_id=%d, tid=%d", point_id, pax_id, tid );
  
  TSalons Salons;
  Salons.trip_id = point_id;
  Salons.Read( rTripSalons );
  vector<SALONS::TSalonSeat> seats;
  
  try {  		
  	SEATS::ChangeLayer( layer_type, point_id, pax_id, tid, xname, yname, seat_type, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	switch( layer_type ) {
  	  case cltCheckin:
    	  Qry.SQLText =
    	    "SELECT "
          "  '' AS crs_seat_no, "     
          "  '' AS preseat_no, "          
          "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no "          
          "FROM pax,pax_grp "
          "WHERE pax.grp_id=pax_grp.grp_id AND "
          "      pax.pax_id=:pax_id";  	  	
        break;
  	  case cltPreseat:
    	  Qry.SQLText =
    	    "SELECT "
          "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "     
          "  salons.get_crs_seat_no(crs_pax.pax_id,:preseat_layer,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS preseat_no, "          
          "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no "          
          "FROM crs_pnr,crs_pax,pax,pax_grp "
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND "
          "      pax.grp_id=pax_grp.grp_id(+) AND "
          "      crs_pax.pax_id=:pax_id";
        Qry.CreateVariable( "preseat_layer", otString, EncodeCompLayerType(ASTRA::cltPreseat) );          
        break;    
      default:        
      	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "Устанавливаемый слой запрещен для разметки" );
    }
        
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );    
    Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "Пассажир не найден" );    
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "preseat_no" ) )
    	NewTextChild( dataNode, "preseat_no", Qry.FieldAsString( "preseat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );  		
    /* надо передать назад новый tid */
    NewTextChild( dataNode, "tid", tid );
    NewTextChild( dataNode, "placename", denorm_iata_row( yname ) + denorm_iata_line( xname, pr_lat_seat ) );        
    SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
    tst();
    TSalons Salons;
    Salons.trip_id = point_id;
    Salons.Read( rTripSalons );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );    
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( SEATS::GetPassengersForManualSeat( point_id, cltCheckin, Passengers, Salons.getLatSeat() ) )  
      Passengers.Build( Salons, dataNode );    
  	showErrorMessageAndRollback( ue.what() );
  }
  
};

void SalonsInterface::AutoReseatsPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::AutoReseatsPassengers, trip_id=%d", trip_id );
  TQuery Qry( &OraSession );
  /* лочим рейс */
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  tst();
  TSalons Salons;
  vector<SALONS::TSalonSeat> seats;  
  Salons.trip_id = trip_id;
  Salons.Read( rTripSalons, true );
  TPassengers passengers;
  
  
  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, passengers, Salons.getLatSeat() ) ) {
  	SEATS::AutoReSeatsPassengers( Salons, passengers );
  }
  else
  	throw UserException( "Пассажиры все пассажены. Автоматическая рассадка не требуется" );
  	
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );  	
  	
  try {
    SALONS::getSalonChanges( Salons, seats );  		  
    SALONS::BuildSalonChanges( dataNode, seats );    	
  }
  catch(...) { //???
  	Salons.Read( rTripSalons );
    Salons.Build( salonsNode );  	
  }
  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, passengers, Salons.getLatSeat() ) )  
    passengers.Build( Salons, dataNode );
}

void SalonsInterface::BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::BaseComponFormShow, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons;
  Salons.comp_id = comp_id;
  Salons.Read( rComponSalons );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetCompParams( comp_id, dataNode );
  Salons.Build( salonsNode );
}

void SalonsInterface::BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::BaseComponFormWrite, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TSalons Salons;
  Salons.Parse( GetNode( "salons", reqNode ) );
  Salons.comp_id = NodeAsInteger( "comp_id", reqNode );
  string smodify = NodeAsString( "modify", reqNode );
  if ( smodify == "delete" )
    Salons.modify = mDelete;
  else
    if ( smodify == "add" )
      Salons.modify = mAdd;
    else
      if ( smodify == "change" )
        Salons.modify = mChange;
      else
        throw Exception( string( "Ошибка в значении тега modify " ) + smodify );       
  TReqInfo *r = TReqInfo::Instance();          
  xmlNodePtr a = GetNode( "airline", reqNode );
  if ( a )
   Salons.airline = NodeAsString( a );
  else 
  	if ( r->user.access.airlines.size() == 1 )
  		Salons.airline = *r->user.access.airlines.begin();
 	a = GetNode( "airp", reqNode );
 	if ( a ) {
 		Salons.airp = NodeAsString( a );
 		Salons.airline.clear();
 	}
 	else
  	if ( r->user.user_type != utAirline && r->user.access.airps.size() == 1 && !GetNode( "airline", reqNode ) ) {
  		Salons.airp = *r->user.access.airps.begin();
  		Salons.airline.clear();
    }
  TQuery Qry( &OraSession );    
  if ( Salons.modify != mDelete ) {
    if ( !Salons.airline.empty() ) {
      Qry.SQLText = "SELECT code FROM airlines WHERE code=:airline";
      Qry.CreateVariable( "airline", otString, Salons.airline );
      Qry.Execute();
      if ( !Qry.RowCount() )
        throw UserException( "Неправильно задан код авиакомпании" );  
    }
    if ( !Salons.airp.empty() ) {
      Qry.Clear();
      Qry.SQLText = "SELECT code FROM airps WHERE code=:airp";
      Qry.CreateVariable( "airp", otString, Salons.airp );
      Qry.Execute();
      if ( !Qry.RowCount() )
        throw UserException( "Неправильно задан код аэропорта" );    
    }
  
    if ( (int)Salons.airline.empty() + (int)Salons.airp.empty() != 1 ) { 
    	if ( Salons.airline.empty() )
    	  throw UserException( "Должен быть задан код авиакомпании или код аэропорта" );
    	else
    		throw UserException( "Одновременное задание авиакомпании и аэропорта запрещено" ); // ??? почему?
    }
  
    if ( ( r->user.user_type == utAirline ||
           r->user.user_type == utSupport && Salons.airp.empty() && !r->user.access.airlines.empty() ) &&
    	   find( r->user.access.airlines.begin(), 
    	         r->user.access.airlines.end(), Salons.airline ) == r->user.access.airlines.end() ) {
 	  	if ( Salons.airline.empty() )
 		  	throw UserException( "Не задан код авиакомпании" );
  	  else
    		throw UserException( "У оператора нет прав записи компоновки для заданной авиакомпании" );         
    }
    if ( ( r->user.user_type == utAirport || 
    	     r->user.user_type == utSupport && Salons.airline.empty() && !r->user.access.airps.empty() ) &&
    	   find( r->user.access.airps.begin(), 
    	         r->user.access.airps.end(), Salons.airp ) == r->user.access.airps.end() ) {
 	  	if ( Salons.airp.empty() )
 	  		throw UserException( "Не задан код аэропорта" );
 	  	else
 	  	  throw UserException( "У оператора нет прав записи компоновки для заданного аэропорта" ); 		   	  	         	
    }
  }	
  Salons.craft = NodeAsString( "craft", reqNode );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  if ( Salons.craft.empty() )
    throw UserException( "Не задан тип ВС" );    
  Qry.Clear();
  Qry.SQLText = "SELECT code FROM crafts WHERE code=:craft";
  Qry.DeclareVariable( "craft", otString );
  Qry.SetVariable( "craft", Salons.craft );
  Qry.Execute();
  if ( !Qry.RowCount() )
    throw UserException( "Неправильно задан тип ВС" );
  Salons.verifyValidRem( "MCLS", "Э" );
  Salons.Write( rComponSalons );
  string msg;
  switch ( Salons.modify ) {
    case mDelete:
      msg = string( "Удалена базовая компоновка (ид=" ) + IntToString( comp_id ) + ").";
      Salons.comp_id = -1;
      break;
    default:
      if ( Salons.modify == mAdd )
        msg = "Создана базовая компоновка (ид=";
      else
        msg = "Изменена базовая компоновка (ид=";
      msg += IntToString( Salons.comp_id );
      msg += "). Код а/к: ";
      if ( Salons.airline.empty() )
      	msg += "не указан";
      else
      	msg += Salons.airline; 
      msg += ", код а/п: ";
      if ( Salons.airp.empty() )
      	msg += "не указан";
      else
      	msg += Salons.airp;       
      msg += ", тип ВС: " + Salons.craft + ", борт: ";
      if ( Salons.bort.empty() )
        msg += "не указан";
      else
        msg += Salons.bort;
      msg += ", классы: " + Salons.classes + ", описание: ";
      if ( Salons.descr.empty() )
        msg += "не указано";
      else
        msg += Salons.descr;
      break;
  }
  r->MsgToLog( msg, evtComp, comp_id );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", Salons.comp_id );
  if ( !Salons.airline.empty() )
    NewTextChild( dataNode, "airline", Salons.airline );
  if ( !Salons.airp.empty() )
    NewTextChild( dataNode, "airp", Salons.airp );
  showMessage( "Изменения успешно сохранены" );
}

bool filterComp( const string &airline, const string &airp )
{
	TReqInfo *r = TReqInfo::Instance();	
  return
       ( (int)airline.empty() + (int)airp.empty() == 1 &&
 		   (( 
 		     r->user.user_type == utAirline &&
 		     find( r->user.access.airlines.begin(), 
 		           r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() /*&&
  		     ( r->user.access.airps.empty() || 
  		       find( r->user.access.airps.begin(), 
  		             r->user.access.airps.end(), 
  		             Qry.FieldAsString( "airp" ) ) != r->user.access.airps.end() )*/
  		  ) 
  		  ||
  		  (
  		    r->user.user_type == utAirport &&
  		    ( airp.empty() && ( r->user.access.airlines.empty() ||
  		       find( r->user.access.airlines.begin(), 
  		             r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() ) ||
  		       find( r->user.access.airps.begin(), 
  		             r->user.access.airps.end(), airp ) != r->user.access.airps.end() )
  		   )
  		   ||
  		   (
  		     r->user.user_type == utSupport &&
  		     ( airp.empty() ||
  		       r->user.access.airps.empty() ||
   		       find( r->user.access.airps.begin(), 
    	             r->user.access.airps.end(), airp ) != r->user.access.airps.end() ) &&
  		     ( airline.empty() ||
  		       r->user.access.airlines.empty() ||
  		       find( r->user.access.airlines.begin(), 
  		             r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() )
  		   )) 
  		 );
}

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *r = TReqInfo::Instance();	
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  if ( r->user.user_type == utAirline && r->user.access.airlines.empty() ||
  	   r->user.user_type == utAirport && r->user.access.airps.empty() )
  	throw UserException( "Нет прав доступа к базовым компоновкам" );
  //TReqInfo::Instance()->user.check_access( amRead );
  TQuery Qry( &OraSession );
  if ( r->user.user_type == utAirport )
    Qry.SQLText = "SELECT airline,airp,comp_id,craft,bort,descr,classes FROM comps "\
                  " ORDER BY airp,airline,craft,comp_id";  	
  else
    Qry.SQLText = "SELECT airline,airp,comp_id,craft,bort,descr,classes FROM comps "\
                  " ORDER BY airline,airp,craft,comp_id";
  Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "data" );
  node = NewTextChild( node, "compons" );
  while ( !Qry.Eof ) {
  	if ( filterComp( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      xmlNodePtr rnode = NewTextChild( node, "compon" );
      NewTextChild( rnode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
      if ( r->user.user_type == utAirline && 
           r->user.access.airlines.size() > 1 ||
           r->user.user_type != utAirline && 
           ( r->user.access.airlines.empty() || r->user.access.airlines.size() > 1 ) ||
           r->user.user_type == utSupport && r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1 )
        NewTextChild( rnode, "airline", Qry.FieldAsString( "airline" ) );   
      if ( r->user.user_type == utAirport && r->user.access.airps.size() > 1 ||
           r->user.user_type == utSupport &&
           ( r->user.access.airps.empty() || r->user.access.airps.size() > 1 ||
             r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1 ) )
    	  NewTextChild( rnode, "airp", Qry.FieldAsString( "airp" ) );    	         
      NewTextChild( rnode, "craft", Qry.FieldAsString( "craft" ) );
      NewTextChild( rnode, "bort", Qry.FieldAsString( "bort" ) );
      NewTextChild( rnode, "descr", Qry.FieldAsString( "descr" ) );
      NewTextChild( rnode, "classes", Qry.FieldAsString( "classes" ) );		     			                  		     			     
      if ( r->user.user_type == utAirport && !Qry.FieldIsNULL( "airline" ) )
        NewTextChild( rnode, "canedit", 0 );
		}
  	Qry.Next();		    	
  }
}

void SalonsInterface::ChangeBC(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SalonsInterface::ChangeBC" );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "BEGIN "\
    " UPDATE points SET craft = :craft WHERE point_id = :point_id; "\
    " UPDATE trip_sets SET comp_id = NULL WHERE point_id = :point_id; "\
    " DELETE trip_comp_rem WHERE point_id = :point_id; "\
    " DELETE trip_comp_elems WHERE point_id = :point_id; "\
    " DELETE trip_classes WHERE point_id = :point_id; "\
    "END; ";
  string bc = NodeAsString( "craft", reqNode );
  int trip_id = NodeAsInteger( "point_id", reqNode );
  Qry.CreateVariable( "craft", otString,  bc );
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  TReqInfo::Instance()->MsgToLog( string( "Изменен тип ВС на " ) + bc +
                                  ". Текущая компоновка рейса удалена." , evtFlt, trip_id );
  SalonFormShow( ctxt, reqNode, resNode );
}


void SalonsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

void convert_salons()
{
	TDateTime v = NowUTC();
	tst();
	TQuery Qry(&OraSession);
	TQuery QryUpd(&OraSession);
	TQuery QryLog(&OraSession);
	QryLog.SQLText =
	  "INSERT INTO salon_logs VALUES(:comp_id, :point_id, :msg)";
	QryLog.DeclareVariable( "comp_id", otInteger );
	QryLog.DeclareVariable( "point_id", otInteger );
	QryLog.DeclareVariable( "msg", otString );
	//1. Определение для всех базовых компоновок pr_lat_seat и запись их в comps.pr_lat_seat
	QryUpd.SQLText = 
	  "UPDATE comps SET pr_lat_seat=:pr_lat_seat WHERE comp_id=:comp_id";
	QryUpd.DeclareVariable( "pr_lat_seat", otInteger );
	QryUpd.DeclareVariable( "comp_id", otInteger );
	Qry.SQLText = 
	  "SELECT comp_id FROM comps";
	Qry.Execute();
	TSalons Salons;
  string rus_lines = rus_seat, lat_lines = lat_seat;	
  int count=0;
	while ( !Qry.Eof ) {
		count++;
    Salons.comp_id = Qry.FieldAsInteger( "comp_id" );
    Salons.Read( rComponSalons );
    int rus_count=0, lat_count=0;    
    for( vector<TPlaceList*>::iterator placeList = Salons.placelists.begin();
         placeList != Salons.placelists.end(); placeList++ ) {
      for ( TPlaces::iterator place = (*placeList)->places.begin();
            place != (*placeList)->places.end(); place++ ) {
        if ( !place->visible || !place->isplace )
         continue;
      	if ( rus_lines.find( place->xname ) != string::npos ) {      		
          rus_count++;
        }
    	  if ( lat_lines.find( place->xname ) != string::npos ) {
          lat_count++;           	
        }         
      }
    }
    //ProgTrace( TRACE5, "comp_id=%d, lat_count=%d, rus_count=%d", Qry.FieldAsInteger( "comp_id" ), lat_count, rus_count );
	  QryUpd.SetVariable( "pr_lat_seat", ( lat_count>=rus_count ) );
	  QryUpd.SetVariable( "comp_id", Qry.FieldAsInteger( "comp_id" ) );    
	  if ( lat_count > 0 && rus_count > 0 ) {
	  	QryLog.SetVariable( "comp_id", Qry.FieldAsInteger( "comp_id" ) );
	  	QryLog.SetVariable( "point_id", FNull );
	  	string msg = string( "Неоднозначное определение признака pr_lat_seat: lat=" ) + 
	  	             IntToString( lat_count ) + ", rus=" + IntToString( rus_count );
	  	QryLog.SetVariable( "msg", msg.c_str() );
	  	QryLog.Execute();
	  }
		Qry.Next();
	}
	ProgTrace( TRACE5, "exec time1.25 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );		
	//2. Определение для всех назначенных компоновок pr_lat_seat b запись их в trip_sets
	Qry.Clear();
	QryUpd.Clear();
	Qry.SQLText = 
	  "SELECT DISTINCT point_id FROM trip_comp_elems ";	
	Qry.Execute();
	QryUpd.SQLText = 
	  "UPDATE trip_sets SET pr_lat_seat=:pr_lat_seat WHERE point_id=:point_id";
	QryUpd.DeclareVariable( "pr_lat_seat", otInteger );
	QryUpd.DeclareVariable( "point_id", otInteger );	  
	count=0;
	while ( !Qry.Eof ) {
		count++;
    Salons.trip_id = Qry.FieldAsInteger( "point_id" );
    Salons.Read( rTripSalons );
    int rus_count=0, lat_count=0;    
    for( vector<TPlaceList*>::iterator placeList = Salons.placelists.begin();
         placeList != Salons.placelists.end(); placeList++ ) {
      for ( TPlaces::iterator place = (*placeList)->places.begin();
            place != (*placeList)->places.end(); place++ ) {
        if ( !place->visible || !place->isplace )
         continue;
      	if ( rus_lines.find( place->xname ) != string::npos ) {      		
          rus_count++;
        }
    	  if ( lat_lines.find( place->xname ) != string::npos ) {
          lat_count++;           	
        }         
      }
    }
	  QryUpd.SetVariable( "pr_lat_seat", ( lat_count>=rus_count ) );
	  QryUpd.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );    
	  if ( lat_count > 0 && rus_count > 0 ) {
	  	QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	QryLog.SetVariable( "comp_id", FNull );
	  	string msg = string( "Неоднозначное определение признака pt_lat_seat: lat=" ) + 
	  	             IntToString( lat_count ) + ", rus=" + IntToString( rus_count );
	  	QryLog.SetVariable( "msg", msg.c_str() );
	  	QryLog.Execute();
	  }		
		Qry.Next();
	}
	ProgTrace( TRACE5, "exec time1.5 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );			
	// приводим салоны к нормализованному виду
	Qry.Clear();
	Qry.SQLText = 
	 "UPDATE comp_elems SET xname=xname, yname=yname";
	Qry.Execute();  
	Qry.Clear();
	Qry.SQLText = 
	 "UPDATE trip_comp_elems SET xname=xname, yname=yname";
	Qry.Execute();  		
	ProgTrace( TRACE5, "exec time1 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );
	// работая со слоями
	// удаление всех слоев из tlg
	Qry.Clear();
	Qry.SQLText =
	 "DELETE tlg_comp_layers";
	Qry.Execute();
	Qry.Clear();
	//3. Перенос поля crs_pax.seat_no в слой trip_comp_layers +
	//4. Перенести crs_pax.seat_type в tlg_comp_layers.rem	
	Qry.Clear();
	Qry.SQLText =
    "SELECT point_id,crs_pax.pax_id,seat_no,preseat_no,seat_type,target,seats "
    "  FROM crs_pnr,crs_pax "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "      ( seat_no IS NOT NULL OR preseat_no IS NOT NULL ) ";
	Qry.Execute();
	ProgTrace( TRACE5, "exec time2 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );	
	vector<TSeatRange> seats;  
	Salons.trip_id = -1;
	TPoint p;
	bool pr_found;
	count = 0;
	while ( !Qry.Eof ) {
		count++;
    try {
    	if ( !Qry.FieldIsNULL( "seat_no" ) ) {
        ParseSeatRange(Qry.FieldAsString( "seat_no" ), seats, false);
        for (vector<TSeatRange>::iterator p=seats.begin(); p!=seats.end(); p++ )
      	  strcpy( p->rem, Qry.FieldAsString( "seat_type" ) );      
        SaveTlgSeatRanges( Qry.FieldAsInteger( "point_id" ), Qry.FieldAsString( "target" ), 
                           cltPNLCkin, seats, 
                           Qry.FieldAsInteger( "pax_id" ), 0, false );
      }
    }
    catch( EConvertError &e ) {
	  	QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	QryLog.SetVariable( "comp_id", FNull );
	  	string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=PNL_CKIN";
	  	QryLog.SetVariable( "msg", msg.c_str() );
	  	QryLog.Execute();    	    	
    }
    try {
    	if ( !Qry.FieldIsNULL( "preseat_no" ) ) {
        ParseSeatRange(Qry.FieldAsString( "preseat_no" ), seats, false);
        SaveTlgSeatRanges( Qry.FieldAsInteger( "point_id" ), Qry.FieldAsString( "target" ), 
                           cltPreseat, seats, 
                           Qry.FieldAsInteger( "pax_id" ), 0, false );
      }
    }
    catch( EConvertError &e ) {
	  	QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	QryLog.SetVariable( "comp_id", FNull );
	  	string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=PRESEAT";
	  	QryLog.SetVariable( "msg", msg.c_str() );
	  	QryLog.Execute();    	    	
    }    
		Qry.Next();
	}
	ProgTrace( TRACE5, "exec time3=%s, count=%d", DateTimeToStr( NowUTC() - v ).c_str(), count );	
	Qry.Clear();
	Qry.SQLText =
	 "DELETE trip_comp_layers";
	Qry.Execute();	
	Qry.Clear();
	QryUpd.Clear();
  //5. Перенос поля pax.seat_no, pax.prev_seat_no в слой trip_comp_layers 
  Qry.SQLText =
	 "SELECT DISTINCT point_dep, point_arv,pax.pax_id,NVL(seat_no,prev_seat_no) seat_no,seats, rem_code "
	 " FROM pax, pax_grp, pax_rem "
	 " WHERE pax_grp.grp_id = pax.grp_id AND NVL(seat_no,prev_seat_no) IS NOT NULL AND "
	 "       pax.pax_id=pax_rem.pax_id(+) AND rem_code(+)='STCR' AND seats>1"
	 "ORDER BY point_dep,pax.pax_id ";
	Qry.Execute();
	ProgTrace( TRACE5, "exec time4 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );	
	Salons.trip_id = -1;
	count=0;
	while ( !Qry.Eof ) {
		count++;
		if ( Salons.trip_id != Qry.FieldAsInteger( "point_dep" ) ) {
      Salons.trip_id = Qry.FieldAsInteger( "point_dep" );
      Salons.Read( rTripSalons );
    }
    pr_found = false;
    for ( vector<TPlaceList*>::iterator placeList=Salons.placelists.begin();
          placeList!=Salons.placelists.end(); placeList++ ) {
      if ( (*placeList)->GetisPlaceXY( Qry.FieldAsString( "seat_no" ), p ) ) {
      	for ( int i=0; i<Qry.FieldAsInteger( "seats" ); i++ ) {
          TSeatRange r;
          TPlace *place = (*placeList)->place( p );
          if ( place->visible && place->isplace ) {
            strcpy( r.first.line, norm_iata_line( place->xname ).c_str() );
            strcpy( r.first.row, norm_iata_row( place->yname ).c_str() );
            r.second = r.first;
		        seats.push_back(r);      		
		      }
		      else {
         	  QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	      QryLog.SetVariable( "comp_id", FNull );
	  	      string msg = string( "Место NVL(pax.seat_no.pax.prev_seat_no)=" ) + Qry.FieldAsString( "seat_no" ) + 
	  	                   " в компоновке рейса задано неверно (x=" + IntToString(p.x) + ",y=" + IntToString(p.y) + "), pax_id=" + Qry.FieldAsString( "pax_id" );
	  	      QryLog.SetVariable( "msg", msg.c_str() );
	  	      QryLog.Execute();    			      	
		      }
		      if ( !Qry.FieldIsNULL( "rem_code" ) )
		      	p.y++;
		      else
		      	p.x++;
      	}
      	pr_found = true;
      }
    }
    if ( pr_found )	{
    	SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_dep" ), cltCheckin, seats, 
    	                           Qry.FieldAsInteger( "pax_id" ), Qry.FieldAsInteger( "point_dep" ), 
    	                           Qry.FieldAsInteger( "point_arv" ) );
    }
    else {
	  	QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	QryLog.SetVariable( "comp_id", FNull );
	  	string msg = string( "Место pax.seat_no=" ) + Qry.FieldAsString( "seat_no" ) + " в компоновке рейса не найдено" + "), pax_id=" + Qry.FieldAsString( "pax_id" );
	  	QryLog.SetVariable( "msg", msg.c_str() );
	  	QryLog.Execute();    	
    }
 	  seats.clear();      
		Qry.Next();
	}  
	ProgTrace( TRACE5, "exec time5=%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );	
	QryUpd.Clear();
	QryUpd.SQLText =
	 "SELECT xname, yname FROM trip_comp_elems WHERE point_id=:point_id AND old_yname||old_xname=:seat_no";
	QryUpd.DeclareVariable( "point_id", otInteger );
	QryUpd.DeclareVariable( "seat_no", otString ); 
	Qry.Clear();
	Qry.SQLText =
	 "SELECT point_dep, point_arv,pax.pax_id,NVL(seat_no,prev_seat_no) seat_no "
	 " FROM pax, pax_grp "
	 " WHERE pax_grp.grp_id = pax.grp_id AND seats=1";
	Qry.Execute();
	ProgTrace( TRACE5, "exec time6 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );	
	count=0;
	while ( Qry.Eof ) {
		try {
			count++;
		  ParseSeatRange(Qry.FieldAsString( "seat_no" ), seats, false);
    	SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_dep" ), 
    		                         DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ), seats, 
  	                             Qry.FieldAsInteger( "pax_id" ), Qry.FieldAsInteger( "point_dep" ), 
  	                             Qry.FieldAsInteger( "point_arv" ) ); 		  
		}
    catch( EConvertError &e ) {
    	// при помощи салона!!!
    	QryUpd.SetVariable( "point_id", Qry.FieldAsInteger( "point_dep" ) );
	    QryUpd.SetVariable( "seat_no", Qry.FieldAsString( "seat_no" ) ); 
	    QryUpd.Execute();
	    if ( QryUpd.Eof ) {    	
	  	  QryLog.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
	  	  QryLog.SetVariable( "comp_id", FNull );
 	  	  string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=CHECKIN";
	  	  QryLog.SetVariable( "msg", msg.c_str() );
	  	  QryLog.Execute();    	    	
	  	}
	  	else {
	  		seats.clear();
        TSeatRange r;
        strcpy( r.first.line, norm_iata_line( QryUpd.FieldAsString( "xname" ) ).c_str() );
        strcpy( r.first.row, norm_iata_row( QryUpd.FieldAsString( "yname" ) ).c_str() );
        r.second = r.first;
        seats.push_back(r);      		
	    	SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_dep" ), 
      		                         DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ), seats, 
  	                               Qry.FieldAsInteger( "pax_id" ), Qry.FieldAsInteger( "point_dep" ), 
  	                               Qry.FieldAsInteger( "point_arv" ) ); 		  	  		
	    }
	  		
    }    		
		Qry.Next();
	}
  ProgTrace( TRACE5, "exec time7 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );
 
	 
  //6. Удаление trip_comp_elems.pr_free, trip_comp_elems.enabled, trip_comp_elems.status
  Qry.Clear();
  QryUpd.Clear();
  Qry.SQLText = 
    "SELECT point_id, xname, yname, enabled, status "
    " FROM trip_comp_elems WHERE enabled IS NULL OR status IN ('RZ','TR') ";
	Qry.Execute();
	ProgTrace( TRACE5, "exec time8 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );	
	TCompLayerType layer_type;
	vector<TSeatRange> RZseats, TRseats;  
	count=0;
	while ( !Qry.Eof ) {
		count++;
		layer_type = cltUnknown;
		if ( !Qry.FieldIsNULL( "enabled" ) ) {
 	    seats.clear();      			
      TSeatRange r;
      strcpy( r.first.line, norm_iata_line( Qry.FieldAsString( "xname" ) ).c_str() );
      strcpy( r.first.row, norm_iata_row( Qry.FieldAsString( "yname" ) ).c_str() );
      r.second = r.first;
      seats.push_back(r);
    	SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_id" ), 
    		                         cltBlockCent, seats, 
    	                           0, Qry.FieldAsInteger( "point_id" ), 0 );
		}
		if ( Qry.FieldAsString( "status" ) == "RZ" )
			layer_type = cltProtect;
		else
			if ( Qry.FieldAsString( "status" ) == "TR" )
			  layer_type = cltTranzit;
			else 
				layer_type = cltUnknown;
		if ( layer_type != cltUnknown ) {
 	    seats.clear();      			
      TSeatRange r;
      strcpy( r.first.line, norm_iata_line( Qry.FieldAsString( "xname" ) ).c_str() );
      strcpy( r.first.row, norm_iata_row( Qry.FieldAsString( "yname" ) ).c_str() );
      r.second = r.first;
      seats.push_back(r);      		
    	SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_id" ), 
    		                         layer_type, seats, 
    	                           0, Qry.FieldAsInteger( "point_id" ), 0 );
    }
		Qry.Next();
	} 
	ProgTrace( TRACE5, "exec time9 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );
	throw UserException( "END OF CONVERT!!!" );     
}


void SalonsInterface::Convert_salons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	tst();
	convert_salons();
}



