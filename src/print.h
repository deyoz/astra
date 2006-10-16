#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "JxtInterface.h"		

void GetPrintDataBT(xmlNodePtr dataNode, int grp_id, int pr_lat);
void GetPrintDataBP(xmlNodePtr dataNode, int pax_id, int prn_type, int pr_lat, xmlNodePtr tagsNode);

class PrintInterface: public JxtInterface
{
    public:
        PrintInterface(): JxtInterface("123", "print")
        {
            Handler *evHandle;
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPectabDataBP);
            AddEvent("GetPectabDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBPXML);
            AddEvent("GetPrintDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBTXML);
            AddEvent("GetPrintDataBT",evHandle);
        }

        void GetPectabDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBPXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
