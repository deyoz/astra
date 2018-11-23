#include "zamar_dsm.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "term_version.h"

#define NICKNAME "GRIG"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;



void ZamarDSMInterface::PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{ 
  NewTextChild( resNode, "sessionId", string(NodeAsString( "sessionId", reqNode, "empty sessionid" )) + ", Hello world!!!" );
}
