#ifndef _BASEL_AERO_H_
#define _BASEL_AERO_H_
#include <vector>
#include <string>
#include "basic.h"

bool is_sync_basel_pax( int point_id );
void getSyncBaselAirps( std::vector<std::string> &airps );
void sych_basel_aero_stat( BASIC::TDateTime utcdate );

#endif /*_BASEL_AERO_H_*/
