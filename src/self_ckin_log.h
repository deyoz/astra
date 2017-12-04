#ifndef _SELF_CKIN_LOG_H_
#define _SELF_CKIN_LOG_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class SelfCkinLogInterface : public JxtInterface
{
    public:
        SelfCkinLogInterface() : JxtInterface("","self_ckin_log")
    {
        Handler *evHandle;
        evHandle=JxtHandler<SelfCkinLogInterface>::CreateHandler(&SelfCkinLogInterface::Run);
        AddEvent("Run",evHandle);
    }

        void Run(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
