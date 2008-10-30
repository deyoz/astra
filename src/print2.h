#ifndef _PRINT2_H
#define _PRINT2_H

#include <libxml/tree.h>
#include "JxtInterface.h"

class Print2Interface: public JxtInterface
{
    public:
        Print2Interface(): JxtInterface("", "print2")
        {
            Handler *evHandle;
            evHandle=JxtHandler<Print2Interface>::CreateHandler(&Print2Interface::GetPrintDataBP);
            AddEvent("GetPrintDataBP",evHandle);
            AddEvent("GetGRPPrintDataBP",evHandle); // !!! temporary tag to test !!! need to abandon forever !!!
        }
        void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
#endif
