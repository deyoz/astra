#include <stdlib.h>
#include "salonform.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "images.h"
#include "salons.h"
#include "seats.h"
#include "seats_utils.h"
#include "convert.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "serverlib/str_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

const char CurrName[] = " (ТЕК.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

bool filterCompons( const string &airline, const string &airp )
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

void SalonFormInterface::Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonFormInterface::Show" );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", trip_id );

  TQuery Qry( &OraSession );
  SALONS2::GetTripParams( trip_id, dataNode );
  bool pr_comps = GetNode( "pr_comps", reqNode );
  bool pr_images = GetNode( "pr_images", reqNode );
  if ( pr_comps ) {
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
    xmlNodePtr compsNode = NULL;
    string StrVal;
    while ( !Qry.Eof ) {
    	if ( Qry.FieldAsInteger( "pr_comp" ) || /* поиск компоновки только по компоновкам нужной А/К или портовым компоновкам */
    		   ( Qry.FieldIsNULL( "airline" ) || trip_airline == Qry.FieldAsString( "airline" ) ) &&
    		   filterCompons( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
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
    if ( !compsNode ) {
    	showErrorMessage( "Нет компоновок по данному типу ВС" );
    	return;
    }
  }
  SALONS2::TSalons Salons( trip_id, SALONS2::rTripSalons );
  if ( GetNode( "ClName", reqNode ) )
  	Salons.ClName = NodeAsString( "ClName", reqNode );
  else
    Salons.ClName.clear();
  try {
    Salons.Read();
  }
  catch( UserException ue ) {
    showErrorMessage( ue.what() );
  }
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  Salons.Build( salonsNode );
  if ( pr_comps ) {
    SEATS2::TPassengers p;
    if ( SEATS2::GetPassengersForWaitList( trip_id, p, true ) ) {
    	showErrorMessage( "Рассадка пассажиров с местами произведена не полностью" );
    	NewTextChild( dataNode, "passengers" );
    }
 	}
 	if ( pr_images ) {
 		GetDrawSalonProp( reqNode, resNode );
 	}
}

void SalonFormInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
  SALONS2::TSalons Salons( trip_id, SALONS2::rTripSalons );
  Salons.Parse( NodeAsNode( "salons", reqNode ) );
  Salons.verifyValidRem( "MCLS", "Э"); //???
  Salons.verifyValidRem( "SCLS", "Э"); //???
  Salons.verifyValidRem( "YCLS", "Э"); //???
  Salons.verifyValidRem( "LCLS", "Э"); //???
  Salons.trip_id = trip_id;
  Salons.ClName = "";
  Qry.Execute();
  //SALONS2::TSalons OSalons( trip_id, SALONS2::rTripSalons );
  //OSalons.Read();
  Salons.Write();
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
  /* инициализация VIP */
  SALONS2::InitVIP( trip_id );
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
  SALONS2::setTRIP_CLASSES( trip_id );
  //set flag auto change in false state
  Qry.Clear();
	Qry.SQLText = "UPDATE trip_sets SET auto_comp_chg=0 WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();


  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  SALONS2::GetTripParams( trip_id, dataNode );
  vector<SEATS2::TSalonSeat> seats;
/*  if ( getSalonChanges( OSalons, Salons, seats ) ) { // tolog
  	//!!!tolog change
  	//SALONS2::BuildSalonChanges( dataNode, seats );
  	tst();
  };*/
  // надо перечитать заново
  Salons.Clear();
  Salons.Read();
  // конец перечитки
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  Salons.Build( salonsNode );
  SEATS2::TPassengers p;
  if ( SEATS2::GetPassengersForWaitList( trip_id, p, true ) ) {
  	showErrorMessage( "Рассадка пассажиров с местами произведена не полностью" );
  	NewTextChild( dataNode, "passengers" );
  }
  else
  	showMessage( "Данные успешно сохранены" );
}

void getSeat_no( int pax_id, bool pr_pnl, const string &format, string &seat_no, string &slayer_type, int &tid )
{
	seat_no.clear();
  TQuery SQry( &OraSession );
  if ( pr_pnl ) {
      SQry.SQLText =
        "SELECT "
        "      crs_pax.tid tid, "
        "      pax_grp.point_dep, "
        "      crs_pax.seat_xname, "
        "      crs_pax.seat_yname, "
        "      crs_pax.seats seats, "
        "      crs_pnr.point_id AS point_id_tlg, "
        "      pax.seats pax_seats, "
        "      pax_grp.status, "
        "      pax.grp_id, "
        "      pax.refuse "
        "FROM crs_pnr,crs_pax,pax,pax_grp "
        "WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pax_id=pax.pax_id(+) AND "
        "      pax.grp_id=pax_grp.grp_id(+)";
  }
  else {
      SQry.SQLText =
        "SELECT "
        "      pax.tid tid, "
        "      pax_grp.point_dep, "
        "      NULL seat_xname, "
        "      NULL seat_yname, "
        "      NULL seats, "
        "      NULL point_id_tlg, "
        "      pax.seats pax_seats, "
        "      pax_grp.status, "
        "      pax.grp_id, "
        "      pax.refuse "
        "FROM pax,pax_grp "
        "WHERE pax.pax_id=:pax_id AND "
        "      pax.grp_id=pax_grp.grp_id";
  };
  SQry.CreateVariable( "pax_id", otInteger, pax_id );
  SQry.Execute();
  if ( SQry.Eof )
  	throw UserException( "Пассажир не найден" );
  tid = SQry.FieldAsInteger( "tid" );
  int point_dep = SQry.FieldAsInteger( "point_dep" );
  string xname = SQry.FieldAsString( "seat_xname" );
  string yname = SQry.FieldAsString( "seat_yname" );
  int seats = SQry.FieldAsInteger( "seats" );
  string grp_status = SQry.FieldAsString( "status" );
  int point_id_tlg = SQry.FieldAsInteger( "point_id_tlg" );
  int pax_seats = SQry.FieldAsInteger( "pax_seats" );
  bool pr_grp_id = !SQry.FieldIsNULL( "grp_id" );
  bool pr_refuse = !SQry.FieldIsNULL( "refuse" );
  if ( pr_grp_id && pr_refuse )
  	return;
  SQry.Clear();
  SQry.SQLText =
    "BEGIN "
    " IF :mode=0 THEN "
    "  :seat_no:=salons.get_seat_no(:pax_id,:seats,:grp_status,:point_id,:format,:pax_row); "
    " ELSE "
    "  :seat_no:=salons.get_crs_seat_no(:pax_id,:xname,:yname,:seats,:point_id,:layer_type,:format,:crs_row); "
    " END IF; "
    "END;";
  SQry.CreateVariable( "format", otString, format.c_str() );
  SQry.CreateVariable( "mode", otInteger, (int)!pr_grp_id );
  SQry.CreateVariable( "pax_id", otInteger, pax_id );
  SQry.CreateVariable( "xname", otString, xname );
  SQry.CreateVariable( "yname", otString, yname );
  SQry.CreateVariable( "grp_status", otString, grp_status );
  SQry.CreateVariable( "layer_type", otString, FNull );
  if ( pr_grp_id ) {
    SQry.CreateVariable( "seats", otInteger, pax_seats );
    SQry.CreateVariable( "point_id", otInteger, point_dep );
  }
  else {
  	SQry.CreateVariable( "seats", otInteger, seats );
  	SQry.CreateVariable( "point_id", otInteger, point_id_tlg );
  }
  SQry.CreateVariable( "pax_row", otInteger, 1 );
  SQry.CreateVariable( "crs_row", otInteger, 1 );
  SQry.CreateVariable( "seat_no", otString, FNull );
	SQry.Execute();
	seat_no = SQry.GetVariableAsString( "seat_no" );
	if ( !seat_no.empty() ) {
		if ( pr_grp_id ) {
			SQry.Clear();
			SQry.SQLText = "SELECT layer_type FROM grp_status_types WHERE code=:code";
			SQry.CreateVariable( "code", otString, grp_status );
			SQry.Execute();
			if ( !SQry.Eof )
			  slayer_type = SQry.FieldAsString( "layer_type" );
		}
		else
			slayer_type = SQry.GetVariableAsString( "layer_type" );
	}
};

void ChangeSeats( xmlNodePtr reqNode, xmlNodePtr resNode, SEATS2::TSeatsType seat_type )
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool pr_waitlist = GetNode( "waitlist", reqNode );
  string xname, yname;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

  if ( seat_type != SEATS2::stDropseat ) {
    xname = norm_iata_line( NodeAsString( "xname", reqNode ) );
    yname = norm_iata_row( NodeAsString( "yname", reqNode ) );
  }
  TCompLayerType layer_type;
  Qry.Clear();
  Qry.SQLText =
   "SELECT layer_type FROM grp_status_types, pax, pax_grp "
   " WHERE pax_id=:pax_id AND pax.grp_id=pax_grp.grp_id AND pax_grp.status=grp_status_types.code ";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( !Qry.Eof ) {
  	layer_type = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
  }
  else {
  	layer_type = DecodeCompLayerType( NodeAsString( "layer", reqNode ) );
  }

  ProgTrace(TRACE5, "SalonsInterface::Reseat, point_id=%d, pax_id=%d, tid=%d, layer=%s", point_id, pax_id, tid, EncodeCompLayerType( layer_type ) );

  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();

  // если место у пассажира имеет предварительную рассадку для этого пассажира, и мы еще не спрашивали, то спросить!!!
  if ( seat_type != SEATS2::stDropseat && !pr_waitlist && GetNode( "question_reseat", reqNode ) ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT seat_no1,seat_no2 FROM "
      "(SELECT first_yname||first_xname seat_no1 FROM trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type=:protckin_layer AND crs_pax_id=:pax_id) a,"
      "(SELECT first_yname||first_xname seat_no2 FROM trip_comp_layers "
      " WHERE point_id=:point_id AND layer_type=:layer_type AND pax_id=:pax_id ) b ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "protckin_layer", otString, EncodeCompLayerType( cltProtCkin ) );
    Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType(layer_type) );
    Qry.Execute();
    ProgTrace( TRACE5, "Qry.Eof=%d, pax_id=%d,point_id=%d,layer1=%s,layer2=%s", Qry.Eof,pax_id,point_id,EncodeCompLayerType( cltProtCkin ),EncodeCompLayerType(layer_type) );
    if ( !Qry.Eof && string(Qry.FieldAsString( "seat_no1" )) == Qry.FieldAsString( "seat_no2" ) ) {
    	ProgTrace( TRACE5, "seat_no1=%s, seat_no2=%s", Qry.FieldAsString( "seat_no1" ), Qry.FieldAsString( "seat_no2" ) );
    	NewTextChild( resNode, "question_reseat", "Пассажир имеет предварительную рассадку. Пересадить пассажира?" );
    	return;
    }
  }

  vector<SALONS2::TSalonSeat> seats;

  try {
  	SEATS2::ChangeLayer( layer_type, point_id, pax_id, tid, xname, yname, seat_type, pr_lat_seat );
  	SALONS2::getSalonChanges( Salons, seats );
  	ProgTrace( TRACE5, "salon changes seats.size()=%d", seats.size() );
  	string seat_no, slayer_type;
  	if ( layer_type == cltProtCkin )
  	  getSeat_no( pax_id, true, string("seats"), seat_no, slayer_type, tid );
    else
    	getSeat_no( pax_id, false, string("one"), seat_no, slayer_type, tid );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
    	NewTextChild( dataNode, "seat_no", seat_no );
    	NewTextChild( dataNode, "layer_type", slayer_type );
    }
    SALONS2::BuildSalonChanges( dataNode, seats );
    if ( pr_waitlist ) {
    	SEATS2::TPassengers p;
    	if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
      	showErrorMessage( "Рассадка пассажиров завершена" );
      p.Build( dataNode );
    }
  }
  catch( UserException ue ) {
    xmlNodePtr dataNode = GetNode( "data", resNode );
    if ( !dataNode )
    	dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
      SEATS2::TPassengers p;
    	if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
      	showErrorMessage( "Рассадка пассажиров завершена" );
      p.Build( dataNode );
    }
  	showErrorMessageAndRollback( ue.what() );
  }
};

void SalonFormInterface::DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	 // удаление мест пассажира
  ChangeSeats( reqNode, resNode, SEATS2::stDropseat );
};

void SalonFormInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ChangeSeats( reqNode, resNode, SEATS2::stReseat );

};

void SalonFormInterface::DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  bool pr_update_salons = GetNode( "update_salons", reqNode );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );

	ProgTrace(TRACE5, "SalonsInterface::DeleteProtCkinSeat, point_id=%d, pax_id=%d, tid=%d, pr_update_salons=%d",
	          point_id, pax_id, tid, pr_update_salons );

  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  vector<SALONS2::TSalonSeat> seats;

  try {
  	SEATS2::ChangeLayer( cltProtCkin, point_id, pax_id, tid, "", "", SEATS2::stDropseat, pr_lat_seat );
  	if ( pr_update_salons )
  	  SALONS2::getSalonChanges( Salons, seats );
  	string seat_no, slayer_type;
  	getSeat_no( pax_id, true, string("seats"), seat_no, slayer_type, tid );
    /* надо передать назад новый tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !seat_no.empty() ) {
    	NewTextChild( dataNode, "seat_no", seat_no );
    	NewTextChild( dataNode, "layer_type", slayer_type );
    }
    if ( pr_update_salons )
   	  SALONS2::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
  	if ( pr_update_salons ) {
      xmlNodePtr dataNode = GetNode( "data", resNode );
      if ( !dataNode )
      	dataNode = NewTextChild( resNode, "data" );
      xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
      SALONS2::GetTripParams( point_id, dataNode );
      Salons.Build( salonsNode );
    }
  	showErrorMessageAndRollback( ue.what() );
  }
}

void SalonFormInterface::WaitList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "trip_id", reqNode );
	bool pr_filter = GetNode( "filter", reqNode );
	bool pr_salons = GetNode( "salons", reqNode );
  SEATS2::TPassengers p;
  if ( SEATS2::GetPassengersForWaitList( point_id, p ) ) {
  	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    p.Build( dataNode );
    if ( pr_filter ) {
      TQuery Qry( &OraSession );
      Qry.SQLText = "SELECT code, name, layer_type FROM grp_status_types";
      Qry.Execute();
      dataNode = NewTextChild( dataNode, "filter" );
      while ( !Qry.Eof ) {
      	xmlNodePtr lNode = NewTextChild( dataNode, "status" );
      	SetProp( lNode, "code", Qry.FieldAsString( "code" ) );
      	SetProp( lNode, "name", Qry.FieldAsString( "name" ) );
      	SetProp( lNode, "layer_type", Qry.FieldAsString( "layer_type" ) );
      	Qry.Next();
      }
    }
    if ( pr_salons ) {
      SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
      Salons.Read();
      Salons.Build( NewTextChild( dataNode, "salons" ) );
      SALONS2::GetTripParams( point_id, dataNode );
    }
  }
}

void SalonFormInterface::AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	int point_id = NodeAsInteger( "trip_id", reqNode );
	ProgTrace( TRACE5, "AutoSeats: point_id=%d", point_id );
	bool pr_waitlist = GetNode( "waitlist", reqNode );
	TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline, flt_no, airp FROM points WHERE point_id=:point_id FOR UPDATE";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("Рейс не найден. Обновите данные");
  string airline = Qry.FieldAsString( "airline" );
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  string airp = Qry.FieldAsString( "airp" );
  SEATS2::TPassengers p;
  if ( !SEATS2::GetPassengersForWaitList( point_id, p ) )
  	throw UserException( "Все пассажиры рассажены" );
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  vector<SALONS2::TSalonSeat> seats;
  Salons.Read();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  try {
    SEATS2::AutoReSeatsPassengers( Salons, p, SEATS2::GetSeatAlgo( Qry, airline, flt_no, airp ) );
    tst();
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
    	p.Clear();
    	if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
    		showErrorMessage( "Рассадка пассажиров с местами произведена не полностью" );
      else
      	showErrorMessage( "Рассадка пассажиров завершена" );
      p.Build( dataNode );
    }
  }
  catch( UserException ue ) {
  	tst();
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    SALONS2::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
    if ( pr_waitlist ) {
      p.Clear();
    	if ( SEATS2::GetPassengersForWaitList( point_id, p ) )
    		showErrorMessage( "Рассадка пассажиров с местами произведена не полностью" );
      else
      	showErrorMessage( "Рассадка пассажиров завершена" );
      p.Build( dataNode );
    }
  	showErrorMessageAndRollback( ue.what() );
  }
}

