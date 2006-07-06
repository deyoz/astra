#include "etick.h"
#include <string>
#define NICKNAME "VLAD"
#include "test.h"
#include "xml_unit.h"
#include "tlg/edi_tlg.h"

using namespace std;

void ETSearchInterface::SearchETByTickNo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string tick_no=NodeAsString("TickNoEdit",reqNode);

  TickDispByNum tickDisp(tick_no);
  SendEdiTlgTKCREQ_Disp( tickDisp );
};

void ETSearchInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

