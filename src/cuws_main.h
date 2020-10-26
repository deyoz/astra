#pragma once

#include "jxtlib/JxtInterface.h"

#define CUWS_JXT_IFACE_ID "CUWS"

namespace CUWS {
    void CUWSDispatcher(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
    void PostProcessXMLAnswer();
    xmlNodePtr NewTextChildNoPrefix(xmlNodePtr parent, const char *name, const char *content = NULL);
}

class CUWSInterface: public JxtInterface
{
    public:
        CUWSInterface(): JxtInterface("", CUWS_JXT_IFACE_ID)
        {
            AddEvent("CUWS",  JXT_HANDLER(CUWSInterface, CUWS));
        }

        void CUWS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
