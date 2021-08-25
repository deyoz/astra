#include "seatPax.h"
#include "date_time.h"
#include "misc.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_date_time.h"
#include "oralib.h"
#include "astra_locale.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;

namespace SEATPAX
{

const std::string paxSeats::PAX_LAYERS_ON_ELEMTYPE_SQL =
    "SELECT e.num, e.x, e.y, l.layer_type, l.time_create, "
    "       l.point_id, l.point_dep, l.point_arv, "
    "       l.first_xname, l.first_yname, l.last_xname, l.last_yname "
    " FROM trip_comp_ranges r, trip_comp_layers l, trip_comp_elems e "
    " WHERE l.point_id = :point_id AND "
    "       l.range_id = r.range_id AND "
    "       e.point_id = l.point_id AND "
    "       e.num = r.num AND "
    "       e.x = r.x AND "
    "       e.y = r.y AND "
    "       (crs_pax_id = :pax_id OR pax_id = :pax_id) AND "
    "       e.elem_type = :elem_type AND "
    "       rownum < 2";

bool paxSeats::paxLayersOnElemType( int point_id, int pax_id, const std::string &elem_type )
{
   TQuery Qry(&OraSession);
   Qry.SQLText = PAX_LAYERS_ON_ELEMTYPE_SQL;
   Qry.CreateVariable( "point_id", otInteger, point_id );
   Qry.CreateVariable( "pax_id", otInteger, pax_id );
   Qry.CreateVariable( "elem_type", otString, elem_type );
   Qry.Execute();
   LogTrace(TRACE5) << "point_id=" << point_id << ",pax_id=" << pax_id << ",elem_type=" << elem_type << ",Qry.Eof=" << Qry.Eof;
   return !Qry.Eof;
}

void paxSeats::SalonListFromDB( int point_id )
{
  if ( salonLists.find( point_id ) == salonLists.end() ) {
    salonLists[ point_id ].ReadFlight( SALONS2::TFilterRoutesSets( point_id, ASTRA::NoExists ), "", NoExists );
  }
}

bool paxSeats::isElemTypePaxSeat( int point_dep, int pax_id, const std::string &elem_type )
{
  SalonListFromDB( point_dep );
  if ( salonLists[ point_dep ].pax_lists.find( point_dep ) != salonLists[ point_dep ].pax_lists.end() &&
       salonLists[ point_dep ].pax_lists[ point_dep ].find( pax_id ) != salonLists[ point_dep ].pax_lists[ point_dep ].end() ) {
    SALONS2::TSalonPax p = salonLists[ point_dep ].pax_lists[ point_dep ][ pax_id ];
    SALONS2::TWaitListReason waitListReason;
    TPassSeats ranges;
    std::map<TSeat,SALONS2::TPlace*,CompareSeat> seats;
    p.get_seats( waitListReason, ranges, seats, true );
    if ( waitListReason.status == SALONS2::layerValid ) {
      for ( auto s : seats ) {
        if ( s.second->elem_type == elem_type ) {
          return true;
        }
      }
    }
  }
  return false;
}

std::list< std::pair<paxSeats::EnumBPNotAllowedReason, std::string> > paxSeats::boarding_pass_not_allowed_reasons( int point_id, int pax_id )
{
  std::list< std::pair<paxSeats::EnumBPNotAllowedReason, std::string> > r;
  if ( !paxLayersOnEmergencyExit( point_id, pax_id ) ) {
    return r;
  }
  if ( isElemTypePaxSeat( point_id, pax_id, SALONS2::ARMCHAIR_EMERGENCY_EXIT_TYPE ) ) {
    r.push_back( std::make_pair(EMERGENCY_SEAT, getNameBPNotAllowedReason( EMERGENCY_SEAT )) );
  }

  return r;
}


} // end namespace SEATPAX
