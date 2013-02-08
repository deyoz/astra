#ifndef _BASEL_AERO_H_
#define _BASEL_AERO_H_
#include <map>
#include <string>
#include "basic.h"
#include "astra_misc.h"

class TBaselAeroAirps: public std::map<std::string,std::string>
{
  public:
   static TBaselAeroAirps *Instance();
   TBaselAeroAirps();
};

bool is_sync_basel_pax( const TTripInfo &tripInfo );
void getSyncBaselAirps( std::vector<std::string> &airps );
void sych_basel_aero_stat( BASIC::TDateTime utcdate );

#endif /*_BASEL_AERO_H_*/
