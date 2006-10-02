#include <stdlib.h>
#include "cent.h"
#include "basic.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "stages.h"
#include "oralib.h"
#include "tripinfo.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

void CentInterface::readTripHeader( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "TripInfoInterface::readTripHeader" );
  TQuery Qry( &OraSession );
  Qry.SQLText = 
      "SELECT  trips.trip_id, "\
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
  Qry.CreateVariable( "trip_id", otInteger, point_id );  
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
    TStage ckin_stage;
    if ( !Qry.FieldIsNULL( "act" ) )
      ckin_stage = sTakeoff;
    else 
      ckin_stage = tripstages.getStage( stCheckIn );
    NewTextChild( node, "ckin_stage", ckin_stage );
    TStage craft_stage;
    if ( !Qry.FieldIsNULL( "act" ) )
      craft_stage = sTakeoff;
    else craft_stage = tripstages.getStage( stCraft );
    NewTextChild( node, "craft_stage", craft_stage );
    
    if ( craft_stage == sRemovalGangWay || craft_stage == sTakeoff )
      NewTextChild( node, "status", TStagesRules::Instance()->status( stCraft, craft_stage ) );
    else
      NewTextChild( node, "status", TStagesRules::Instance()->status( stCheckIn, ckin_stage ) );    
    NewTextChild( node, "remark", Qry.FieldAsString( "remark" ) );
    NewTextChild( node, "pr_saloninit", Qry.FieldAsInteger( "pr_saloninit" ) );    
  }	
}

void CentInterface::ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  ProgTrace(TRACE5, "CentInterface::ReadTrips, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amRead );    
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "point_id", point_id );
  if ( GetNode( "tripheader", reqNode ) ) /* Считать заголовок */
    readTripHeader( point_id, dataNode );    
  if ( GetNode( "counters", reqNode ) ) /* Считать заголовок */
    readTripCounters( point_id, dataNode );        
}

void CentInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
