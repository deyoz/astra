#include <stdlib.h>
#include "salonform2.h"
#include "salonform.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "salons.h"
#include "seats_utils.h"
#include "astra_elems.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", SALONS2::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

bool showComponAirlineColumn()
{
  TReqInfo *r = TReqInfo::Instance();
  return !r->user.access.airlines().only_single_permit() ||
         (r->user.user_type == utSupport &&
          r->user.access.airlines().only_single_permit() &&
          r->user.access.airps().only_single_permit());
}

bool showComponAirpColumn()
{
  TReqInfo *r = TReqInfo::Instance();
  return r->user.user_type != utAirline &&
         (!r->user.access.airps().only_single_permit() ||
          (r->user.user_type == utSupport &&
           r->user.access.airlines().only_single_permit() &&
           r->user.access.airps().only_single_permit())
         );
}

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *r = TReqInfo::Instance();
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  if (r->user.access.airlines().totally_not_permitted() &&
      r->user.access.airps().totally_not_permitted())
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
    if ( SALONS2::filterComponsForView( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      xmlNodePtr rnode = NewTextChild( node, "compon" );
      NewTextChild( rnode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
      if (showComponAirlineColumn())
        NewTextChild( rnode, "airline", ElemIdToCodeNative( etAirline, Qry.FieldAsString( "airline" ) ) );
      if (showComponAirpColumn())
        NewTextChild( rnode, "airp", ElemIdToCodeNative( etAirp, Qry.FieldAsString( "airp" ) ) );
      NewTextChild( rnode, "craft", ElemIdToCodeNative( etCraft, Qry.FieldAsString( "craft" ) ) );
      NewTextChild( rnode, "bort", Qry.FieldAsString( "bort" ) );
      NewTextChild( rnode, "descr", Qry.FieldAsString( "descr" ) );
      NewTextChild( rnode, "classes", Qry.FieldAsString( "classes" ) );
      if ( !SALONS2::filterComponsForEdit( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) )
        NewTextChild( rnode, "canedit", 0 );
    }
    Qry.Next();
  }
}

void SalonsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
}






