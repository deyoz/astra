#ifndef _SEATPAX_H_
#define _SEATPAX_H_

#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "seats_utils.h"
#include "salons.h"

namespace SEATPAX
{

class paxSeats { //класс реализует вычисление свойств мест
  private:
   static const std::string PAX_LAYERS_ON_ELEMTYPE_SQL;
   std::map<int, SALONS2::TSalonList > salonLists;
   void SalonListFromDB( int point_id );
   bool isElemTypePaxSeat( int point_dep, int pax_id, const std::string &elem_type );
  public:
   enum EnumBPNotAllowedReason
    {
       EMERGENCY_SEAT
    };
   static const std::list< std::pair<EnumBPNotAllowedReason, std::string> >& pairs()
    {
      static std::list< std::pair<EnumBPNotAllowedReason, std::string> > l =
      {
        {EMERGENCY_SEAT, "emergency_seat"}
      };
      return l;
    }
   static std::string getNameBPNotAllowedReason( EnumBPNotAllowedReason e ) {
     for ( auto p : pairs() ) {
       if ( p.first == e ) {
         return p.second;
       }
     }
     throw EXCEPTIONS::Exception( "invalid EnumBPNotAllowedReason item" );
   }

   static bool paxLayersOnElemType( int point_id, int pax_id, const std::string &elem_type );
   static bool paxLayersOnEmergencyExit( int point_id, int pax_id ) {
     return paxLayersOnElemType( point_id, pax_id, SALONS2::ARMCHAIR_EMERGENCY_EXIT_TYPE );
   }
   paxSeats() {
   }
   std::list< std::pair<EnumBPNotAllowedReason, std::string> > boarding_pass_not_allowed_reasons( int point_id, int pax_id );

};

template <class T1>
class GrpNotAllowedReason
{
  public:
    void boarding_pass( int point_id,  std::vector<T1> &paxs ) {
      if ( paxs.empty() ) {
        return;
      }
      paxSeats paxSeats;
      for(auto ip = begin(paxs); ip != end(paxs); ++ip) {
        ip->bp_not_allowed_reasons.clear();
        std::list< std::pair<paxSeats::EnumBPNotAllowedReason, std::string> > ls =
                     paxSeats.boarding_pass_not_allowed_reasons( point_id, ip->crs_pax_id==ASTRA::NoExists?ip->pax_id:ip->crs_pax_id );
        for ( auto l: ls ) {
          ip->bp_not_allowed_reasons.insert( l.second );
        }
      }
    }
};

} //namespace SEATPAX
#endif //_SEATPAX_H_
