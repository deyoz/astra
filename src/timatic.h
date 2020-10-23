#pragma once

#include "jxtlib/JxtInterface.h"

class TimaticInterface: public JxtInterface
{
    public:
        TimaticInterface(): JxtInterface("", "timatic")
        {
            AddEvent("TimaticDoc",  JXT_HANDLER(TimaticInterface, TimaticDoc));
            AddEvent("layout",      JXT_HANDLER(TimaticInterface, layout));
            AddEvent("run",      JXT_HANDLER(TimaticInterface, run));
        }

        void TimaticDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void layout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void run(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
