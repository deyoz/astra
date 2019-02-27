#ifndef _KIOSK_EVENTS_H_
#define _KIOSK_EVENTS_H_

#include "jxtlib/JxtInterface.h"


class KioskRequestInterface : public JxtInterface
{
public:
  KioskRequestInterface() : JxtInterface("","KioskRequest")
  {
     Handler *evHandle;
     evHandle=JxtHandler<KioskRequestInterface>::CreateHandler(&KioskRequestInterface::EventToServer);
     AddEvent("EventToServer",evHandle);
     evHandle=JxtHandler<KioskRequestInterface>::CreateHandler(&KioskRequestInterface::ViewCraft);
     AddEvent("ViewCraft",evHandle);
  }

  void EventToServer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
