#pragma once

#include "jxtlib/JxtInterface.h"

#define CUWS_JXT_IFACE_ID "CUWS"

class CUWSInterface: public JxtInterface
{
    public:
        CUWSInterface(): JxtInterface("", CUWS_JXT_IFACE_ID)
        {
            AddEvent("CUWS",  JXT_HANDLER(CUWSInterface, CUWS));
        }

        void CUWS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
