#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "JxtInterface.h"		

class PrintInterface: public JxtInterface
{
    public:
        PrintInterface(): JxtInterface("123", "print")
        {
            Handler *evHandle;
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPectabDataBP);
            AddEvent("GetPectabDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBP);
            AddEvent("GetPrintDataBP",evHandle);
        }

        void GetPectabDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
