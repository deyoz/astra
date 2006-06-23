#include "etick.h"
#include <string>
#define NICKNAME "VLAD"
#include "test.h"
#include "xml_unit.h"

using namespace std;

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5,"SearchETByTickNo!");	
  string tick_no=NodeAsString("TickNoEdit",reqNode);
  ProgTrace(TRACE5,"tick_no=%s",tick_no.c_str());
  	
};

void ETSearchInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

