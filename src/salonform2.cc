#include <stdlib.h>
#include "salonform2.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "images.h"
#include "salons2.h"
#include "salons.h"
#include "seats_utils.h"
#include "convert.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "seats2.h" // only for convert_salons
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

const char CurrName[] = " (ТЕК.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

bool filterComp( const string &airline, const string &airp );


void SalonsInterface::CheckInShow( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) // !!!old terminal
{
  ProgTrace(TRACE5, "SalonsInterface::CheckInShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons( NodeAsInteger( "trip_id", reqNode ), SALONS2::rTripSalons );
  Salons.ClName = NodeAsString( "ClName", reqNode );
  bool PrepareShow = NodeAsInteger( "PrepareShow", reqNode );
  SetProp( resNode, "handle", "1" );
  xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
  SetProp( ifaceNode, "id", "SalonsInterface" );
  SetProp( ifaceNode, "ver", "1" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( PrepareShow )
    ImagesInterface::GetImages( reqNode, resNode );
  Salons.Read();
  SALONS::GetTripParams( Salons.trip_id, dataNode );
  Salons.Build( NewTextChild( dataNode, "salons" ) );
};

void SalonsInterface::SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", trip_id );
  TQuery Qry( &OraSession );
  tst();
  SALONS::GetTripParams( trip_id, dataNode );
  tst();
  Qry.SQLText = "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if ( !Qry.RowCount() )
  	throw AstraLocale::UserException( "MSG.FLIGHT.NOT_FOUND" );
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
  tst();
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
        	StrVal = ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) );
        else
        	StrVal = ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp" ) );
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
        NewTextChild( compNode, "craft", ElemIdToCodeNative( etCraft, Qry.FieldAsString( "craft" ) ) );
        NewTextChild( compNode, "bort", Qry.FieldAsString( "bort" ) );
        NewTextChild( compNode, "classes", Qry.FieldAsString( "classes" ) );
        NewTextChild( compNode, "descr", Qry.FieldAsString( "descr" ) );
      }
      Qry.Next();
    }
    if ( !compsNode )
      throw AstraLocale::UserException( "MSG.SALONS.NOT_FOUND_FOR_THIS_CRAFT" );
   tst();
   TSalons Salons( trip_id, SALONS2::rTripSalons );
   Salons.ClName.clear();
   Salons.Read();
   xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
   Salons.Build( salonsNode );
   if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, SEATS::Passengers, Salons.getLatSeat() ) )
 	   SEATS::Passengers.Build( Salons, dataNode );
 }
 catch( AstraLocale::UserException ue ) {
   AstraLocale::showErrorMessage( ue.getLexemaData());
 }
}

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amRead );
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", SALONS::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

bool ChangeCfg( TSalons &NewSalons, SALONS2::TSalons &OldSalons )
{
  if ( NewSalons.placelists.size() != OldSalons.placelists.size() ) {
    return true;
  }
  vector<TPlaceList*>::iterator sn=NewSalons.placelists.begin();
  for ( vector<SALONS2::TPlaceList*>::iterator so=OldSalons.placelists.begin();
        so!=OldSalons.placelists.end(); so++, sn++ ) {
    if ( (*so)->places.size() != (*sn)->places.size() ||
         (*so)->GetXsCount() != (*sn)->GetXsCount() ||
         (*so)->GetYsCount() != (*sn)->GetYsCount() )
      return true;
    IPlace snp = (*sn)->places.begin();
    for ( SALONS2::IPlace sop=(*so)->places.begin(); sop!=(*so)->places.end(); sop++, snp++ ) {
      if ( sop->visible != snp->visible ||
           sop->visible &&
           ( sop->x != snp->x || sop->y != snp->y || sop->isplace != snp->isplace ) )
        return true;
    }
  }
  return false;
}

void SalonsInterface::SalonFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormWrite" );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
  xmlNodePtr ctypeNode = NodeAsNode( "ctype", refcompNode );
  bool cBase = false;
  bool cChange = false;
  bool cSet = false;
  if ( ctypeNode ) {
    ctypeNode = ctypeNode->children; /* value */
    while ( ctypeNode ) {
    	string stype = NodeAsString( ctypeNode );
      cBase = ( stype == string( "cBase" ) || cBase );
      cChange = ( stype == string( "cChange" ) || cChange );
      cSet = ( stype == string( "cSet" ) || cSet );
      ctypeNode = ctypeNode->next;
    }
  }
  ProgTrace( TRACE5, "cBase=%d, cChange=%d, cSet=%d", cBase, cChange, cSet );
  Qry.SQLText = "SELECT airline,flt_no,suffix,airp,scd_out FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  TTripInfo info( Qry );
	tst();
  TSalons Salons( trip_id, SALONS2::rTripSalons );
  Salons.Parse( NodeAsNode( "salons", reqNode ) );
  Salons.verifyValidRem( "MCLS", "Э"); //???
  Salons.verifyValidRem( "SCLS", "Э"); //???
  Salons.verifyValidRem( "YCLS", "Э"); //???
  Salons.verifyValidRem( "LCLS", "Э"); //???
  Salons.trip_id = trip_id;
  Salons.ClName = "";
  bool pr_base_change = false;
  SALONS2::TSalons OldSalons( trip_id, SALONS2::rTripSalons );
  Qry.Clear();
  Qry.SQLText =
    "SELECT point_id FROM trip_comp_elems WHERE point_id=:point_id AND rownum<2";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if ( !Qry.Eof ) { // была старая компоновка
    OldSalons.Read();
    pr_base_change = ChangeCfg( Salons, OldSalons );
  }
  Qry.Clear();
  Qry.SQLText = "UPDATE trip_sets SET comp_id=:comp_id WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.DeclareVariable( "comp_id", otInteger );
  // пришла новая компоновка, но не пришел comp_id - значит были изменения компоновки - "сохраните базовую компоновку."
  if ( GetTripSets( tsCraftNoChangeSections, info ) ) {
    if ( comp_id == -2 && !cSet )
      throw UserException( "MSG.SALONS.SAVE_BASE_COMPON" );
    // может вызвать ошибку, если салон не был назначен на рейс
    if ( comp_id == -2 && pr_base_change ) // была старая компоновка
      throw UserException( "MSG.SALONS.NOT_CHANGE_CFG_ON_FLIGHT" );
    if ( comp_id != -2 && !cSet ) { //новая компоновку
      Qry.SetVariable( "comp_id", comp_id );
      Qry.Execute();
    }
  }
  else {
    if ( pr_base_change && cSet )
      comp_id = -2;
    if ( comp_id == -2 )
      Qry.SetVariable( "comp_id", FNull );
    else
      Qry.SetVariable( "comp_id", comp_id );
    Qry.Execute();
  }
  Salons.Write();
  
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
  /* инициализация VIP */
  SALONS2::InitVIP( trip_id );
  string msg = string( "Изменена компоновка рейса. Классы: " ) +
               NodeAsString( "classes", refcompNode );
  msg += string( ", кодировка: " ) + NodeAsString( "lang", refcompNode ); //???
  TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );

  if ( pr_initcomp ) { /* изменение компоновки */
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
  Salons.Read( );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );
  Salons.Build( salonsNode );
  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, SEATS::Passengers, Salons.getLatSeat() ) )
    SEATS::Passengers.Build( Salons, dataNode );
}

void SalonsInterface::DeleteReserveSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
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

  TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  vector<SALONS::TSalonSeat> seats;

  try {
  	SEATS2::ChangeLayer( cltProtCkin, point_id, pax_id, tid, "", "", SEATS2::stDropseat, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	Qry.SQLText =
  	  "SELECT "
  	  "  crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,"
      "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "
      "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no "
      "FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pr_del=0 AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( Qry.Eof )
    	throw AstraLocale::UserException( "MSG.PASSENGER.NOT_FOUND" );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );
    int crs_pax_id = Qry.FieldAsInteger( "pax_id" );
    string seat_xname = Qry.FieldAsString( "seat_xname" );
    string seat_yname = Qry.FieldAsString( "seat_yname" );
    int pre_seats = Qry.FieldAsInteger( "seats" );
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  :preseat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'seats'); "
      "END;";
    Qry.CreateVariable( "preseat_no", otString, FNull );
    Qry.CreateVariable( "pax_id", otInteger, crs_pax_id );
    Qry.CreateVariable( "xname", otString, seat_xname );
    Qry.CreateVariable( "yname", otString, seat_yname );
    Qry.CreateVariable( "seats", otInteger, pre_seats );
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "layer_type", otString, FNull );
    Qry.Execute();
    if ( !Qry.VariableIsNULL( "layer_type" ) &&
         DecodeCompLayerType( Qry.GetVariableAsString( "layer_type" ) ) == cltProtCkin &&
         !Qry.VariableIsNULL( "preseat_no" ) )
    	NewTextChild( dataNode, "preseat_no", Qry.GetVariableAsString( "preseat_no" ) );
//    ProgTrace( TRACE5, "preseat_no=%s, crs_pax_id=%d, seat_xname=%s, seat_yname=%s, pre_seats=%d, point_id=%d, layer_type=%s",
//               Qry.GetVariableAsString( "preseat_no" ), crs_pax_id, seat_xname.c_str(), seat_yname.c_str(), pre_seats, point_id, Qry.GetVariableAsString( "layer_type" ) );
   	SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( AstraLocale::UserException ue ) {
    TSalons Salons( point_id, SALONS2::rTripSalons );
    Salons.Read();
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
  	dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( SEATS::GetPassengersForManualSeat( point_id, cltCheckin, SEATS::Passengers, Salons.getLatSeat() ) )
      SEATS::Passengers.Build( Salons, dataNode );
  	AstraLocale::showErrorMessageAndRollback( ue.getLexemaData() );
  }
}

void SalonsInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  //TReqInfo::Instance()->user.check_access( amWrite );
  SEATS2::TSeatsType seat_type = SEATS2::stReseat;
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
  if ( Qry.Eof ) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
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
	if ( GetNode( "reserve", reqNode ) )
    layer_type = cltProtCkin;
  else {
  	Qry.Clear();
    Qry.SQLText = "SELECT layer_type FROM trip_comp_layers WHERE pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( !Qry.Eof )
    	layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
    else
  	  layer_type = cltCheckin;
  }
  ProgTrace(TRACE5, "SalonsInterface::Reseat, point_id=%d, pax_id=%d, tid=%d, layer_type=%s", point_id, pax_id, tid, EncodeCompLayerType(layer_type) );

  TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  vector<SALONS::TSalonSeat> seats;

  try {
    SEATS2::ChangeLayer( layer_type, point_id, pax_id, tid, xname, yname, seat_type, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	switch( layer_type ) {
  	  case cltTranzit:
  	  case cltGoShow:
  	  case cltCheckin:
  	  case cltTCheckin:
    	  Qry.SQLText =
    	    "SELECT "
          "  '' AS crs_seat_no, "
          "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no "
          "FROM pax,pax_grp "
          "WHERE pax.grp_id=pax_grp.grp_id AND "
          "      pax.pax_id=:pax_id";
        break;
  	  case cltProtCkin:
    	  Qry.SQLText =
    	    "SELECT "
    	    "  crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,"
          "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "
          "  salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'seats',rownum) AS seat_no "
          "FROM crs_pnr,crs_pax,pax,pax_grp "
          "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pr_del=0 AND "
          "      crs_pax.pax_id=pax.pax_id(+) AND "
          "      pax.grp_id=pax_grp.grp_id(+) AND "
          "      crs_pax.pax_id=:pax_id";
        break;
      default:
      	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw AstraLocale::UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }

    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    if ( Qry.Eof )
    	throw AstraLocale::UserException( "MSG.PASSENGER.NOT_FOUND" );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );

    if ( layer_type == cltProtCkin ) {
      string seat_xname = Qry.FieldAsString( "seat_xname" );
      string seat_yname = Qry.FieldAsString( "seat_yname" );
      int pre_seats = Qry.FieldAsInteger( "seats" );
      Qry.Clear();
      Qry.SQLText =
        "BEGIN "
        "  :preseat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,'seats'); "
        "END;";
      Qry.CreateVariable( "preseat_no", otString, FNull );
      Qry.CreateVariable( "pax_id", otInteger, pax_id );
      Qry.CreateVariable( "xname", otString, seat_xname );
      Qry.CreateVariable( "yname", otString, seat_yname );
      Qry.CreateVariable( "seats", otInteger, pre_seats );
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.CreateVariable( "layer_type", otString, FNull );
      Qry.Execute();
      if ( !Qry.VariableIsNULL( "layer_type" ) &&
           DecodeCompLayerType( Qry.GetVariableAsString( "layer_type" ) ) == cltProtCkin &&
           !Qry.VariableIsNULL( "preseat_no" ) )
        NewTextChild( dataNode, "preseat_no", Qry.GetVariableAsString( "preseat_no" ) );
    }
  /* надо передать назад новый tid */
    NewTextChild( dataNode, "tid", tid );
    NewTextChild( dataNode, "placename", denorm_iata_row( yname, NULL ) + denorm_iata_line( xname, pr_lat_seat ) );
    SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( AstraLocale::UserException ue ) {
    TSalons Salons( point_id, SALONS2::rTripSalons );
    Salons.Read();
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    if ( dataNode ) { // удаление всей инфы, т.к. случилась ошибка
      xmlUnlinkNode( dataNode );
      xmlFreeNode( dataNode );
    }
  	dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( SEATS::GetPassengersForManualSeat( point_id, cltCheckin, SEATS::Passengers, Salons.getLatSeat() ) )
      SEATS::Passengers.Build( Salons, dataNode );
  	AstraLocale::showErrorMessageAndRollback( ue.getLexemaData() );
  }

};

void SalonsInterface::AutoReseatsPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
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
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.CHANGED.REFRESH_DATA");
  int algo=SEATS::GetSeatAlgo(Qry,
                              Qry.FieldAsString("airline"),
                              Qry.FieldAsInteger("flt_no"),
                              Qry.FieldAsString("airp"));

  TSalons Salons( trip_id, SALONS2::rTripSalons );
  vector<SALONS::TSalonSeat> seats;
  Salons.Read( true );
  SEATS::TPassengers passengers;


  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, passengers, Salons.getLatSeat() ) ) {
  	SEATS::AutoReSeatsPassengers( Salons, passengers, algo );
  }
  else
  	throw AstraLocale::UserException( "MSG.SEATS.ALL_PAX_BOARDED.AUTO_SEATS_NOT_REQUIRED" );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );

  try {
    SALONS::getSalonChanges( Salons, seats );
    SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch(...) { //???
  	Salons.Read( );
    Salons.Build( salonsNode );
  }
  if ( SEATS::GetPassengersForManualSeat( trip_id, cltCheckin, passengers, Salons.getLatSeat() ) )
    passengers.Build( Salons, dataNode );
}

void SalonsInterface::BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::BaseComponFormShow, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons( comp_id, SALONS2::rComponSalons );
  Salons.Read( );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetCompParams( comp_id, dataNode );
  Salons.Build( salonsNode );
}

void SalonsInterface::BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::BaseComponFormWrite, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TSalons Salons( NodeAsInteger( "comp_id", reqNode ), SALONS2::rComponSalons );
  Salons.Parse( GetNode( "salons", reqNode ) );
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
        throw Exception( string( "Error in tag modify " ) + smodify );
  TReqInfo *r = TReqInfo::Instance();
  TElemFmt fmt;
  xmlNodePtr a = GetNode( "airline", reqNode );
  if ( a ) {
     Salons.airline = ElemToElemId( etAirline, NodeAsString( a ), fmt );
     if ( fmt == efmtUnknown )
     	 throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_INPUT" );
  }
  else
  	if ( r->user.access.airlines.size() == 1 )
  		Salons.airline = *r->user.access.airlines.begin();
 	a = GetNode( "airp", reqNode );
 	if ( a ) {
 		Salons.airp = ElemToElemId( etAirp, NodeAsString( a ), fmt );
 		if ( fmt == efmtUnknown )
 			throw AstraLocale::UserException( "MSG.AIRP.INVALID_SET_CODE" );
 		Salons.airline.clear();
 	}
 	else
  	if ( r->user.user_type != utAirline && r->user.access.airps.size() == 1 && !GetNode( "airline", reqNode ) ) {
  		Salons.airp = *r->user.access.airps.begin();
  		Salons.airline.clear();
    }
  if ( Salons.modify != mDelete ) {
    if ( (int)Salons.airline.empty() + (int)Salons.airp.empty() != 1 ) {
    	if ( Salons.airline.empty() )
    	  throw AstraLocale::UserException( "MSG.AIRLINE_OR_AIRP_MUST_BE_SET" );
    	else
    		throw AstraLocale::UserException( "MSG.NOT_SET_ONE_TIME_AIRLINE_AND_AIRP" ); // птому что компоновка принадлежит или авиакомпании или порту
    }

    if ( ( r->user.user_type == utAirline ||
           r->user.user_type == utSupport && Salons.airp.empty() && !r->user.access.airlines.empty() ) &&
    	   find( r->user.access.airlines.begin(),
    	         r->user.access.airlines.end(), Salons.airline ) == r->user.access.airlines.end() ) {
 	  	if ( Salons.airline.empty() )
 		  	throw AstraLocale::UserException( "MSG.AIRLINE.UNDEFINED" );
  	  else
    		throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRLINE" );
    }
    if ( ( r->user.user_type == utAirport ||
    	     r->user.user_type == utSupport && Salons.airline.empty() && !r->user.access.airps.empty() ) &&
    	   find( r->user.access.airps.begin(),
    	         r->user.access.airps.end(), Salons.airp ) == r->user.access.airps.end() ) {
 	  	if ( Salons.airp.empty() )
 	  		throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.NOT_SET_AIRP" );
 	  	else
 	  	  throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRP" );
    }
  }
  Salons.craft = NodeAsString( "craft", reqNode );
  if ( Salons.craft.empty() )
    throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
  Salons.craft = ElemToElemId( etCraft, Salons.craft, fmt );
  if ( fmt == efmtUnknown )
  	throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  Salons.verifyValidRem( "MCLS", "Э" );
  Salons.Write();
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
    NewTextChild( dataNode, "airline", ElemIdToCodeNative( etAirline, Salons.airline ) );
  if ( !Salons.airp.empty() )
    NewTextChild( dataNode, "airp", ElemIdToCodeNative( etAirp, Salons.airp ) );
  if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
    NewTextChild( dataNode, "craft", ElemIdToCodeNative( etCraft, Salons.craft ) );
  AstraLocale::showMessage( "MSG.CHANGED_DATA_COMMIT" );
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
  	throw AstraLocale::UserException( "MSG.SALONS.ACCESS_DENIED" );
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
        NewTextChild( rnode, "airline", ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) ) );
      if ( r->user.user_type == utAirport && r->user.access.airps.size() > 1 ||
           r->user.user_type == utSupport &&
           ( r->user.access.airps.empty() || r->user.access.airps.size() > 1 ||
             r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1 ) )
    	  NewTextChild( rnode, "airp", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp" ) ) );
      NewTextChild( rnode, "craft", ElemIdToCodeNative( etCraft, Qry.FieldAsString( "craft" ) ) );
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






