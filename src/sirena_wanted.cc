#include "basel_aero.h"

#include <string>
#include <vector>
#include <tcl.h>
#include "base_tables.h"

#include "basic.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "xml_unit.h"
#include "cache.h"
#include "passenger.h"
#include "events.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;



using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

struct TPaxWanted {

};


void get_pax_wanted( vector<TPaxWanted> &paxs )
{
   paxs.clear();
   
}

void send_pax_wanted( const vector<TPaxWanted> &paxs )
{
}

void sync_sirena_wanted( TDateTime utcdate )
{
   ProgTrace(TRACE5,"sync_sirena_codes started");
   vector<TPaxWanted> paxs;
   get_pax_wanted( paxs );
   send_pax_wanted( paxs );
}
