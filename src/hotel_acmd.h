#ifndef _HOTEL_ACMD_H_
#define _HOTEL_ACMD_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class HotelAcmdInterface : public JxtInterface
{
    public:
        HotelAcmdInterface() : JxtInterface("","hotel_acmd")
    {
        Handler *evHandle;
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::Save);
        AddEvent("Save",evHandle);
        evHandle=JxtHandler<HotelAcmdInterface>::CreateHandler(&HotelAcmdInterface::View);
        AddEvent("View",evHandle);
    }

        void View(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
