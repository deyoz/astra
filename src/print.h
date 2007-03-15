#ifndef _PRINT_H
#define _PRINT_H

#include <libxml/tree.h>
#include "JxtInterface.h"		

void GetPrintDataBT(xmlNodePtr dataNode, int grp_id, int pr_lat);
void GetPrintDataBP(xmlNodePtr dataNode, int pax_id, int prn_type, int pr_lat, xmlNodePtr clientDataNode);
void GetPrintDataBP(xmlNodePtr dataNode, int grp_id, int prn_type, int pr_lat, bool pr_all, xmlNodePtr clientDataNode);

class PrintInterface: public JxtInterface
{
    public:
        PrintInterface(): JxtInterface("123", "print")
        {
            Handler *evHandle;
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBPXML);
            AddEvent("GetPrintDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetGRPPrintDataBPXML);
            AddEvent("GetGRPPrintDataBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ReprintDataBTXML);
            AddEvent("ReprintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBTXML);
            AddEvent("GetPrintDataBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBT);
            AddEvent("ConfirmPrintBT",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::ConfirmPrintBP);
            AddEvent("ConfirmPrintBP",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrinterList);
            AddEvent("GetPrinterList",evHandle);
            evHandle=JxtHandler<PrintInterface>::CreateHandler(&PrintInterface::GetPrintDataBR);
            AddEvent("GetPrintDataBR",evHandle);
        }

        void GetPrinterList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetGRPPrintDataBPXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBPXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ReprintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBTXML(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBT(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void GetPrintDataBR(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
