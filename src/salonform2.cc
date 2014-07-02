#include <stdlib.h>
#include "salonform2.h"
#include "salonform.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "salons2.h"
#include "salons.h"
#include "seats_utils.h"
#include "astra_elems.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", SALONS2::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

void SalonsInterface::BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) //!!old terminal
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::BaseComponFormShow, comp_id=%d", comp_id );
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
           (r->user.user_type == utSupport && Salons.airp.empty() && !r->user.access.airlines.empty()) ) &&
    	   find( r->user.access.airlines.begin(),
    	         r->user.access.airlines.end(), Salons.airline ) == r->user.access.airlines.end() ) {
 	  	if ( Salons.airline.empty() )
 		  	throw AstraLocale::UserException( "MSG.AIRLINE.UNDEFINED" );
  	  else
    		throw AstraLocale::UserException( "MSG.SALONS.OPER_WRITE_DENIED_FOR_THIS_AIRLINE" );
    }
    if ( ( r->user.user_type == utAirport ||
    	     (r->user.user_type == utSupport && Salons.airline.empty() && !r->user.access.airps.empty()) ) &&
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
  string lexema_id;
  LEvntPrms params;
  switch ( Salons.modify ) {
    case mDelete:
      lexema_id = "EVT.BASE_LAYOUT_DELETED";
      params << PrmSmpl<int>("id", comp_id);
      Salons.comp_id = -1;
      break;
    default:
      if ( Salons.modify == mAdd )
        lexema_id = "EVT.BASE_LAYOUT_CREATED";
      else
        lexema_id = "EVT.BASE_LAYOUT_MODIFIED";
      params << PrmSmpl<int>("id", Salons.comp_id);
      if ( Salons.airline.empty() )
        params << PrmLexema("airl", "EVT.UNKNOWN");
      else
        params << PrmElem<std::string>("airl", etAirline, Salons.airline);
      if ( Salons.airp.empty() )
        params << PrmLexema("airp", "EVT.UNKNOWN");
      else
        params << PrmElem<std::string>("airp", etAirp, Salons.airp);
      params << PrmElem<std::string>("craft", etCraft, Salons.craft);
      if ( Salons.bort.empty() )
        params << PrmLexema("bort", "EVT.UNKNOWN");
      else
        params << PrmSmpl<std::string>("bort", Salons.bort);
      params << PrmSmpl<std::string>("cls", Salons.classes);
      if ( Salons.descr.empty() )
        params << PrmLexema("descr", "EVT.UNKNOWN");
      else
        params << PrmSmpl<std::string>("descr", Salons.descr);
      break;
  }
  r->LocaleToLog(lexema_id, params, evtComp, comp_id);
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

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *r = TReqInfo::Instance();
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  if ( (r->user.user_type == utAirline && r->user.access.airlines.empty()) ||
  	   (r->user.user_type == utAirport && r->user.access.airps.empty()) )
  	throw AstraLocale::UserException( "MSG.SALONS.ACCESS_DENIED" );
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
  	if ( filterCompons( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      xmlNodePtr rnode = NewTextChild( node, "compon" );
      NewTextChild( rnode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
      if ( (r->user.user_type == utAirline &&
            r->user.access.airlines.size() > 1) ||
           (r->user.user_type != utAirline &&
            ( r->user.access.airlines.empty() || r->user.access.airlines.size() > 1 )) ||
           (r->user.user_type == utSupport && r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1) )
        NewTextChild( rnode, "airline", ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) ) );
      if ( (r->user.user_type == utAirport && r->user.access.airps.size() > 1) ||
           (r->user.user_type == utSupport &&
            ( r->user.access.airps.empty() || r->user.access.airps.size() > 1 ||
              (r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1) ) ) )
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

void SalonsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};






