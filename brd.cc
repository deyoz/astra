#include "brd.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DENIS"
#include "test.h"

using namespace EXCEPTIONS;
using namespace std;

void BrdInterface::Trips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
}

void BrdInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
