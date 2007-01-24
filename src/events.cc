#include "events.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "docs.h"

#define NICKNAME "DJEK"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    RunRpt("EventsLog", reqNode, resNode);
};
