#ifndef ZAMAR_DSM_H
#define ZAMAR_DSM_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class ZamarDSMInterface: public JxtInterface
{
    public:
        ZamarDSMInterface(): JxtInterface("456", "ZamarDSM")
        {
            AddEvent("PassengerSearch",    JXT_HANDLER(ZamarDSMInterface, PassengerSearch));
        }

        void PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif // ZAMAR_DSM_H
