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
   TSalons Salons;
   Salons.trip_id = trip_id;
   Salons.ClName.clear();
   Salons.Read( rTripSalons );
   xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
   Salons.Build( salonsNode );
   if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, Passengers, Salons.getLatSeat() ) )
 	   Passengers.Build( Salons, dataNode );
 }
 catch( UserException ue ) {
   showErrorMessage( ue.what() );
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
  //set flag auto change in false state
  Qry.Clear();
	Qry.SQLText = "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();


  /* перечитываение компоновки из БД */
  Salons.Read( rTripSalons );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );
  Salons.Build( salonsNode );
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
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id,airline,flt_no,airp FROM points "
    "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 FOR UPDATE";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  if (Qry.Eof) throw UserException("Рейс изменен. Обновите данные");
  int algo=SEATS::GetSeatAlgo(Qry,
                              Qry.FieldAsString("airline"),
                              Qry.FieldAsInteger("flt_no"),
                              Qry.FieldAsString("airp"));

  TSalons Salons;
  vector<SALONS::TSalonSeat> seats;
  Salons.trip_id = trip_id;
  Salons.Read( rTripSalons, true );
  TPassengers passengers;


  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, passengers, Salons.getLatSeat() ) ) {
  	SEATS::AutoReSeatsPassengers( Salons, passengers, algo );
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

void ParseSeat(string str, TSeat &seat)
{
  seat.Clear();
  char c;
  int res;

  c=0;
  res=sscanf(str.c_str(),"%3[0-9]%1[A-ZА-ЯЁ]%c",
             seat.row,seat.line,&c);
  if (c==0 && res==2)
  {
    NormalizeSeat(seat);
    return;
  };
  throw EConvertError("ParseSeat: wrong seat %s", str.c_str());
}

void convert_salons( int step, bool pr_commit )
{
	TDateTime v = NowUTC();
	TQuery Qry(&OraSession);
	TQuery QryUpd(&OraSession);
	int count=0;
	string rus_lines = rus_seat, lat_lines = lat_seat;
	TSalons Salons;
  Salons.trip_id = -1;
	bool pr_found;
	TPoint p;
	vector<TSeatRange> seats;
  TSeat s_e_a_t;


	switch ( step ) {
		case 0:
	  {
	    //0. Определение для всех базовых компоновок pr_lat_seat и запись их в comps.pr_lat_seat
	    Qry.Clear();
	    QryUpd.Clear();
	    QryUpd.SQLText =
	      "UPDATE comps SET pr_lat_seat=:pr_lat_seat WHERE comp_id=:comp_id";
	    QryUpd.DeclareVariable( "pr_lat_seat", otInteger );
	    QryUpd.DeclareVariable( "comp_id", otInteger );
    	Qry.SQLText =
	      "SELECT comp_id FROM comps";
	    Qry.Execute();
      count=0;
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
  	    QryUpd.SetVariable( "pr_lat_seat", ( lat_count>=rus_count ) );
  	    QryUpd.SetVariable( "comp_id", Qry.FieldAsInteger( "comp_id" ) );
  	    QryUpd.Execute();
  	    if ( lat_count > 0 && rus_count > 0 ) {
  	    	string msg = string( "Неоднозначное определение признака pr_lat_seat: lat=" ) +
  	    	             IntToString( lat_count ) + ", rus=" + IntToString( rus_count );
  	    	ProgError( STDLOG, "convert_salons: %s, comp_id=%d", msg.c_str(), Qry.FieldAsInteger( "comp_id" ) );
  	    }
  		  Qry.Next();
  	  }
	    ProgTrace( TRACE5, "exec end convert comps.pr_lat_seat time2 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );
	    if ( !pr_commit )
    	  OraSession.Rollback();
    	else
    	  OraSession.Commit();
	    break;
	  }
	  case 1:
	  {
  	  //1. Определение для всех назначенных компоновок pr_lat_seat b запись их в trip_sets
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
	      QryUpd.Execute();
	      if ( lat_count > 0 && rus_count > 0 ) {
	  	    string msg = string( "Неоднозначное определение признака pt_lat_seat: lat=" ) +
	  	                 IntToString( lat_count ) + ", rus=" + IntToString( rus_count );
	  	    ProgError( STDLOG, "convert_salons: %s, point_id=%d", msg.c_str(), Qry.FieldAsInteger( "point_id" ) );
	      }
		    Qry.Next();
	    }
	    ProgTrace( TRACE5, "exec end convert trip_sets.pr_lat_seat time3 =%s, count=%d", DateTimeToStr(  NowUTC() - v ).c_str(), count );
	    if ( !pr_commit )
    	  OraSession.Rollback();
    	else
    	  OraSession.Commit();
	    break;
	  }
	  case 2:
	  {
   	  // 2.приводим компоновки к нормализованному виду
  	/*  Qry.Clear();
  	  Qry.SQLText =
  	    "UPDATE comp_elems SET xname=xname, yname=yname";
  	  Qry.Execute();
  	  ProgTrace( TRACE5, "exec normalize comp_elems time4 =%s", DateTimeToStr(  NowUTC() - v ).c_str() );
  	  if ( !pr_commit )
    	  OraSession.Rollback();
    	else
    	  OraSession.Commit();*/
  	  break;
  	}
    case 3:
    {
      // 3.приводим компоновки к нормализованному виду
      Qry.Clear();
      Qry.SQLText = "SELECT TRUNC(MIN(scd_out)) min_scd, TRUNC(MAX(scd_out)) max_scd FROM points";
      Qry.Execute();
      TDateTime min_scd = Qry.FieldAsDateTime( "min_scd" );
      TDateTime max_scd = Qry.FieldAsDateTime( "max_scd" );
      ProgTrace( TRACE5, "minday=%s, maxday=%s", DateTimeToStr(min_scd).c_str(), DateTimeToStr(max_scd).c_str() );
      TQuery Points(&OraSession);
      Points.SQLText = "SELECT point_id FROM points WHERE scd_out>=:scd_out AND scd_out<:scd_out+1";
      Points.DeclareVariable( "scd_out", otDate );
      for ( TDateTime scd=min_scd; scd<=max_scd; scd+=1.0 ) {
      	Points.SetVariable( "scd_out", scd );
      	Points.Execute();
      	while ( !Points.Eof ) {
       /*   Qry.Clear();
    	    Qry.SQLText =
  	      "UPDATE trip_comp_elems SET xname=xname, yname=yname WHERE point_id=:point_id";
  	      Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
  	      Qry.Execute();*/
    	  	//5. Перенос поля pax.seat_no, pax.prev_seat_no в слой trip_comp_layers
     	    Qry.Clear();
    	    Qry.SQLText =
    	      "DELETE trip_comp_layers WHERE point_id=:point_id";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    Qry.Clear();
          Qry.SQLText =
    	      "SELECT DISTINCT point_dep, point_arv,pax.pax_id,NVL(seat_no,prev_seat_no) seat_no,seats, rem_code "
    	      " FROM pax, pax_grp, pax_rem "
    	      " WHERE pax_grp.grp_id = pax.grp_id AND NVL(seat_no,prev_seat_no) IS NOT NULL AND refuse IS NULL AND "
    	      "       pax.pax_id=pax_rem.pax_id(+) AND rem_code(+)='STCR' AND seats>1 AND pax_grp.point_dep=:point_id "
    	      "ORDER BY point_dep,pax.pax_id ";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    if (!Qry.Eof)
    	    {
      	    Salons.trip_id = Points.FieldAsInteger( "point_id" );
      	    Salons.Read( rTripSalons );
      	    count=0;
      	    seats.clear();
      	    while ( !Qry.Eof ) {
      		    count++;
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
      	  	          string msg = string( "Место NVL(pax.seat_no.pax.prev_seat_no)=" ) + Qry.FieldAsString( "seat_no" ) +
      	  	                       " в компоновке рейса задано неверно (x=" + IntToString(p.x) + ",y=" + IntToString(p.y) + "), pax_id=" + Qry.FieldAsString( "pax_id" );
      	  	          ProgError( STDLOG, "convert_salons: %s", msg.c_str() );
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
      	  	    string msg = string( "Место pax.seat_no=" ) + Qry.FieldAsString( "seat_no" ) + " в компоновке рейса не найдено" + "), pax_id=" + Qry.FieldAsString( "pax_id" );
      	  	    ProgError( STDLOG, "convert_salons: %s", msg.c_str() );
              }
       	      seats.clear();
      		    Qry.Next();
      	    }
      	  };
    	    QryUpd.Clear();
    	    QryUpd.SQLText =
    	     "SELECT xname, yname FROM trip_comp_elems WHERE point_id=:point_id AND old_yname||old_xname=:seat_no";
    	    QryUpd.DeclareVariable( "point_id", otInteger );
    	    QryUpd.DeclareVariable( "seat_no", otString );
    	    Qry.Clear();
    	    Qry.SQLText =
    	      "SELECT point_dep, point_arv,pax.pax_id,NVL(seat_no,prev_seat_no) seat_no "
    	      " FROM pax, pax_grp "
    	      " WHERE pax_grp.grp_id = pax.grp_id AND seats=1 AND refuse IS NULL AND point_dep=:point_id";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    count=0;
    	    while ( !Qry.Eof ) {
    		    try {
    			    count++;
              ParseSeat(Qry.FieldAsString( "seat_no" ), s_e_a_t);
              seats.clear();
              seats.push_back(TSeatRange(s_e_a_t,s_e_a_t));
        	    SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_dep" ),
        		                             cltCheckin, seats,
      	                                 Qry.FieldAsInteger( "pax_id" ), Qry.FieldAsInteger( "point_dep" ),
      	                                 Qry.FieldAsInteger( "point_arv" ) );
    		    }
            catch( EConvertError &e ) {
        	    // при помощи салона!!!
        	    QryUpd.SetVariable( "point_id", Qry.FieldAsInteger( "point_dep" ) );
    	        QryUpd.SetVariable( "seat_no", Qry.FieldAsString( "seat_no" ) );
    	        QryUpd.Execute();
    	        if ( QryUpd.Eof ) {
     	  	      string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=CHECKIN";
      	  	    ProgError( STDLOG, "convert_salons: %s, point_dep=%d, seat_no=%s", msg.c_str(),
      	  	               Qry.FieldAsInteger( "point_dep" ), Qry.FieldAsString( "seat_no" ) );
    	  	    }
    	  	    else {
    	  		    seats.clear();
                TSeatRange r;
                strcpy( r.first.line, norm_iata_line( QryUpd.FieldAsString( "xname" ) ).c_str() );
                strcpy( r.first.row, norm_iata_row( QryUpd.FieldAsString( "yname" ) ).c_str() );
                //ProgTrace( TRACE5, "Not ParseSeat: pax_id=%d, r.first.line=%s, r.first.row=%s, xname=%s, yname=%s",
                //         Qry.FieldAsInteger( "pax_id" ), r.first.line, r.first.row,
                //         QryUpd.FieldAsString( "xname" ), QryUpd.FieldAsString( "yname" ) );
                r.second = r.first;
                seats.push_back(r);
    	    	    SEATS::SaveTripSeatRanges( Qry.FieldAsInteger( "point_dep" ),
          		                             cltCheckin, seats,
      	                                   Qry.FieldAsInteger( "pax_id" ), Qry.FieldAsInteger( "point_dep" ),
      	                                   Qry.FieldAsInteger( "point_arv" ) );
    	        }

            }
    		    Qry.Next();
    	    }

          //6. Удаление trip_comp_elems.pr_free, trip_comp_elems.enabled, trip_comp_elems.status
          Qry.Clear();
          Qry.SQLText =
            "SELECT point_id, xname, yname, enabled, status "
            " FROM trip_comp_elems WHERE (enabled IS NULL OR status IN ('RZ','TR')) AND point_id=:point_id";
          Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
        	TCompLayerType layer_type;
    	    vector<TSeatRange> RZseats, TRseats;
    	    count=0;
    	    while ( !Qry.Eof ) {
    		    count++;
    		    layer_type = cltUnknown;
    		    if ( Qry.FieldIsNULL( "enabled" ) ) {
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
    		    if ( string(Qry.FieldAsString( "status" )) == string("RZ") )
    			    layer_type = cltProtect;
    		    else
    			    if ( string(Qry.FieldAsString( "status" )) == string("TR") )
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

    	    //7. Проверка
    	    Qry.Clear();
    	    Qry.SQLText =
    	      "SELECT point_id,diff_check,diff_tranzit,diff_block_cent,diff_protect "
    	      " FROM ( "
    	      "SELECT a.point_id, "
    	      "NVL(a.checkin,0)-NVL(b.checkin,0) diff_check, "
    	      "NVL(a.tranzit,0)-NVL(b.tranzit,0) diff_tranzit, "
    	      "NVL(a.block_cent,0)-NVL(b.block_cent,0) diff_block_cent, "
    	      "NVL(a.protect,0)-NVL(b.protect,0) diff_protect "
    	      " FROM "
    	      "( "
            "SELECT point_id, "
            " SUM(DECODE(old_pr_free,NULL,1,0)) checkin, "
            " SUM(DECODE(old_status, 'TR',1,0)) tranzit, "
            " SUM(DECODE(old_enabled,NULL,1,0)) block_cent, "
            " SUM(DECODE(old_status, 'RZ',1,0)) protect "
            " FROM trip_comp_elems WHERE point_id=:point_id "
            "	GROUP BY point_id "
            " ) a, "
    	      " ( "
    	      "SELECT r.point_id,"
    	      "  SUM(DECODE(layer_type, 'CHECKIN', 1, 0) ) checkin, "
    	      "  SUM(DECODE(layer_type, 'TRANZIT', 1, 0) ) tranzit, "
    	      "  SUM(DECODE(layer_type, 'BLOCK_CENT', 1, 0) ) block_cent, "
    	      "  SUM(DECODE(layer_type, 'PROTECT', 1, 0) ) protect "
    	      " FROM trip_comp_ranges r WHERE point_id=:point_id "
    	      "GROUP BY r.point_id "
    	      " ) b "
    	      " WHERE a.point_id=b.point_id(+) ) "
    	      " WHERE diff_check <> 0 OR diff_tranzit <> 0 OR diff_block_cent <> 0 OR diff_protect <> 0 ";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    while ( !Qry.Eof ) {
     	      string msg = string("Количество мест не совпадает, point_id=") + IntToString(Qry.FieldAsInteger( "point_id" ));
     	      if ( Qry.FieldAsInteger( "diff_check" ) )
     	  	    msg += string(" регистрация: ") + Qry.FieldAsString( "diff_check" );
     	      if ( Qry.FieldAsInteger( "diff_tranzit" ) )
     	  	    msg += string(" транзит: ") + Qry.FieldAsString( "diff_tranzit" );
     	      if ( Qry.FieldAsInteger( "diff_block_cent" ) )
     	  	    msg += string(" блокировка центровки: ") + Qry.FieldAsString( "diff_block_cent" );
     	      if ( Qry.FieldAsInteger( "diff_protect" ) )
     	  	    msg += string(" резерв: ") + Qry.FieldAsString( "diff_protect" );
     	      ProgError( STDLOG, "convert_salons: verify layers: %s", msg.c_str() );
    		    Qry.Next();
    	    }

    	    Qry.Clear();
    	    Qry.SQLText=
    	      "SELECT pax_id,pax.grp_id,prev_seat_no,seat_no,salons.get_seat_no(pax_id,'CHECKIN',seats,NULL,'one') AS new_seat_no	"
    	      "FROM pax,pax_grp "
    	      "WHERE pax.grp_id=pax_grp.grp_id AND point_dep=:point_id AND "
    	      "      NVL(seat_no,' ')<>NVL(salons.get_seat_no(pax_id,'CHECKIN',seats,NULL,'one'),' ')";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    QryUpd.Clear();
    	    QryUpd.SQLText = "SELECT point_id, airline||flt_no||' '||airp||' '||TO_CHAR(scd_out,'DD.MM.YY') flt FROM pax_grp, points "
    	    " WHERE pax_grp.point_dep=points.point_id AND pax_grp.grp_id=:grp_id";
          QryUpd.DeclareVariable( "grp_id", otInteger );
    	    for(;!Qry.Eof;Qry.Next())
    	    {
    		    QryUpd.SetVariable( "grp_id", Qry.FieldAsInteger( "grp_id" ) );
    		    QryUpd.Execute();
    		    ProgError( STDLOG, "Different seat_no (point_id=%d, flt=%s, pax_id=%d, old_seat_no=%s, prev_seat_no=%s, new_seat_no=%s)",
    		                       QryUpd.FieldAsInteger( "point_id" ),  QryUpd.FieldAsString( "flt" ),
    		                       Qry.FieldAsInteger("pax_id"),
    		                       Qry.FieldAsString("seat_no"),
    		                       Qry.FieldAsString("prev_seat_no"),
    		                       Qry.FieldAsString("new_seat_no"));

    	    };
    	    if ( !pr_commit )
    	      OraSession.Rollback();
    	    else
    	      OraSession.Commit();

        	Points.Next();
       	}
        ProgTrace( TRACE5, "Day %s: step3 current time: %s", DateTimeToStr(scd).c_str(), DateTimeToStr(  NowUTC() - v ).c_str() );
      };


      break;
    }
    case 4:
    {
    	Qry.Clear();
      Qry.SQLText = "SELECT TRUNC(MIN(scd)) min_scd, TRUNC(MAX(scd)) max_scd FROM tlg_trips";
      Qry.Execute();
      TDateTime min_scd = Qry.FieldAsDateTime( "min_scd" );
      TDateTime max_scd = Qry.FieldAsDateTime( "max_scd" );
      TQuery Points(&OraSession);
      Points.SQLText = "SELECT point_id FROM tlg_trips WHERE scd>=:scd_out AND scd<:scd_out+1";
      Points.DeclareVariable( "scd_out", otDate );
      for ( TDateTime scd=min_scd; scd<=max_scd; scd++ ) {
      	Points.SetVariable( "scd_out", scd );
      	Points.Execute();
      	while ( !Points.Eof ) {


      	  // 4. работая со слоями удаление всех слоев из tlg
       		//  Перенос поля crs_pax.seat_no в слой tlg_comp_layer + crs_pax.seat_type в tlg_comp_layers.rem
    	    Qry.Clear();
       	  Qry.SQLText =
    	      "DELETE tlg_comp_layers WHERE point_id=:point_id";
    	    Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    Qry.Clear();
    	    Qry.SQLText =
            "SELECT point_id,crs_pax.pax_id,seat_no,preseat_no,seat_type,target,seats "
            "  FROM crs_pnr,crs_pax "
            " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pnr.point_id=:point_id AND "
            "      ( seat_no IS NOT NULL OR preseat_no IS NOT NULL ) ";
          Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
    	    Qry.Execute();
    	    count = 0;
    	    TQuery Q(&OraSession);
    	    Q.SQLText =
    	      "UPDATE crs_pax SET seat_xname=:xname, seat_yname=:yname WHERE pax_id=:pax_id";
    	    Q.DeclareVariable( "pax_id", otInteger );
    	    Q.DeclareVariable( "xname", otString );
    	    Q.DeclareVariable( "yname", otString );

    	    while ( !Qry.Eof ) {
    		    count++;
            try {
        	    if ( !Qry.FieldIsNULL( "seat_no" ) ) {
                ParseSeat(Qry.FieldAsString( "seat_no" ), s_e_a_t);
                seats.clear();
                seats.push_back(TSeatRange(s_e_a_t,s_e_a_t));
                for (vector<TSeatRange>::iterator p=seats.begin(); p!=seats.end(); p++ )
          	      strcpy( p->rem, Qry.FieldAsString( "seat_type" ) );
                SaveTlgSeatRanges( Qry.FieldAsInteger( "point_id" ), Qry.FieldAsString( "target" ),
                                   cltPNLCkin, seats,
                                   Qry.FieldAsInteger( "pax_id" ), 0, false );
                Q.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
                Q.SetVariable( "xname", s_e_a_t.line );
                Q.SetVariable( "yname", s_e_a_t.row );
                Q.Execute();
              }
            }
            catch( EConvertError &e ) {
    	  	    string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=PNL_CKIN";
    	  	    ProgError( STDLOG, "convert_salons: %s", msg.c_str() );
            }
            try {
        	    if ( !Qry.FieldIsNULL( "preseat_no" ) ) {
                ParseSeat(Qry.FieldAsString( "preseat_no" ), s_e_a_t);
                seats.clear();
                seats.push_back(TSeatRange(s_e_a_t,s_e_a_t));
                SaveTlgSeatRanges( Qry.FieldAsInteger( "point_id" ), Qry.FieldAsString( "target" ),
                                   cltPreseat, seats,
                                   Qry.FieldAsInteger( "pax_id" ), 0, false );
              }
            }
            catch( EConvertError &e ) {
    	  	    string msg = string(e.what()) + ",pax_id=" + Qry.FieldAsString( "pax_id" ) + ",layer_type=PRESEAT";
    	  	    ProgError( STDLOG, "convert_salons: %s", msg.c_str() );
            }
    		    Qry.Next();
    	    }
    	    //проверка
    	    Qry.Clear();
    	    Qry.SQLText=
            " SELECT pax_id, new_crs_seat_no, seat_no "
            " FROM  "
            "  ( SELECT pax_id, seat_xname, seat_yname, seats, seat_no, rownum, "
            "    salons.get_crs_seat_no(seat_xname,seat_yname,seats,NULL,'one',rownum,0) AS new_crs_seat_no,  "
            "    salons.get_crs_seat_no(seat_xname,seat_yname,seats,NULL,'one',rownum,1) AS new_crs_seat_no_lat  "
            "    FROM crs_pnr,crs_pax "
            "    WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND point_id=:point_id AND seat_no IS NOT NULL ) a "
            " WHERE NVL(a.seat_no,' ')<>NVL(new_crs_seat_no,' ')	AND "
            "       NVL(a.seat_no,' ')<>NVL(new_crs_seat_no_lat,' ') ";
          Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
          Qry.Execute();
    	    for(;!Qry.Eof;Qry.Next())
    	    {
    		    ProgError( STDLOG, "Different seat_no (pax_id=%d, old_seat_no=%s, new_seat_no=%s)",
    		                        Qry.FieldAsInteger("pax_id"),
    		                        Qry.FieldAsString("seat_no"),
    		                        Qry.FieldAsString("new_crs_seat_no"));

    	    };

    	    QryUpd.Clear();
    	    QryUpd.SQLText =
    	      "SELECT points.point_id, airline||flt_no||' '||airp||' '||TO_CHAR(scd_out,'DD.MM.YY') flt "
    	      "FROM crs_pnr,tlg_binding,points "
    	      "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND "
    	      "      tlg_binding.point_id_spp=points.point_id(+) AND "
    	      "      crs_pnr.pnr_id=:pnr_id";
          QryUpd.DeclareVariable( "pnr_id", otInteger );

    	    Qry.Clear();
    	    Qry.SQLText=
            " SELECT pax_id, new_preseat_no, preseat_no, pnr_id "
            " FROM  "
            " ( SELECT pax_id, seats, preseat_no, rownum, crs_pnr.pnr_id, "
            " salons.get_crs_seat_no(pax_id,:layer_type,seats,NULL,'one',rownum,0) AS new_preseat_no, "
            " salons.get_crs_seat_no(pax_id,:layer_type,seats,NULL,'one',rownum,1) AS new_preseat_no_lat "
            " FROM crs_pnr,crs_pax "
            " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND point_id=:point_id AND preseat_no IS NOT NULL ) a "
            " WHERE NVL(a.preseat_no,' ')<>NVL(new_preseat_no,' ') AND "
            "       NVL(a.preseat_no,' ')<>NVL(new_preseat_no_lat,' ')";
          Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
          Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( cltPreseat ) );
          Qry.Execute();
    	    for(;!Qry.Eof;Qry.Next())
    	    {
    	      QryUpd.SetVariable("pnr_id",Qry.FieldAsInteger("pnr_id"));
    	      QryUpd.Execute();
    	      if (!QryUpd.Eof && !QryUpd.FieldIsNULL("point_id"))
    	         ProgError( STDLOG, "Different preseat_no (layer) (point_id=%d, flt=%s, pax_id=%d, old_preseat_no=%s, new_preseat_no=%s)",
    	                            QryUpd.FieldAsInteger( "point_id" ),  QryUpd.FieldAsString( "flt" ),
    		                          Qry.FieldAsInteger("pax_id"),
    		                          Qry.FieldAsString("preseat_no"),
    		                          Qry.FieldAsString("new_preseat_no"));
    	      else
    		      ProgError( STDLOG, "Different preseat_no (layer) (pax_id=%d, old_preseat_no=%s, new_preseat_no=%s)",
    		                         Qry.FieldAsInteger("pax_id"),
    		                         Qry.FieldAsString("preseat_no"),
    		                         Qry.FieldAsString("new_preseat_no"));

      	  };

    	    Qry.Clear();
    	    Qry.SQLText=
            " SELECT pax_id, new_seat_no, seat_no "
            " FROM  "
            " ( SELECT pax_id, seats, seat_no, rownum, "
            " salons.get_crs_seat_no(pax_id,:layer_type,seats,NULL,'one',rownum,0) AS new_seat_no, "
            " salons.get_crs_seat_no(pax_id,:layer_type,seats,NULL,'one',rownum,1) AS new_seat_no_lat "
            " FROM crs_pax,crs_pnr "
            " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND point_id=:point_id AND seat_no IS NOT NULL ) a "
            " WHERE NVL(a.seat_no,' ')<>NVL(new_seat_no,' ') AND "
            "       NVL(a.seat_no,' ')<>NVL(new_seat_no_lat,' ') ";
          Qry.CreateVariable( "point_id", otInteger, Points.FieldAsInteger( "point_id" ) );
          Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( cltPNLCkin ) );
          Qry.Execute();
    	    for(;!Qry.Eof;Qry.Next())
    	    {
    		    ProgError( STDLOG, "Different seat_no (layer) (pax_id=%d, old_seat_no=%s, new_seat_no=%s)",
    		                       Qry.FieldAsInteger("pax_id"),
    		                       Qry.FieldAsString("seat_no"),
    		                       Qry.FieldAsString("new_seat_no"));

    	    };
    	    if ( !pr_commit )
    	      OraSession.Rollback();
    	    else
    	      OraSession.Commit();

        	Points.Next();
       	}
        ProgTrace( TRACE5, "Day %s: step4 current time: %s", DateTimeToStr(scd).c_str(), DateTimeToStr(  NowUTC() - v ).c_str() );
      };
	    break;
	  }
  }
}


void SalonsInterface::Convert_salons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	convert_salons(NodeAsInteger( "step", reqNode ), NodeAsInteger( "pr_commit", reqNode ));
}




