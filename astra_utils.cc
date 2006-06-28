#include "setup.h"
#include "astra_utils.h"
#include <string.h>

using namespace std;

const char * EventTypeS[ evtTypeNum ] =
    {"‘…‡","„ˆ‘","…‰","ƒ”","€‘","‹","ŠŒ","’‹ƒ","„‘’","‘ˆ‘","Š„","„","!","?"};

TEventType DecodeEventType( const string ev_type )
{
  int i;
  for( i=0; i<(int)evtTypeNum; i++ )
    if ( ev_type == EventTypeS[ i ] )
      break;
  if ( i == evtTypeNum )
    return evtUnknown;
  else
    return (TEventType)i;
}

string EncodeEventType(const TEventType ev_type )
{
  string s = EventTypeS[ ev_type ];
  return s;
}



