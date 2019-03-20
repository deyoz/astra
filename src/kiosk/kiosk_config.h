#ifndef _KIOSK_CONFIG_H_
#define _KIOSK_CONFIG_H_

#include "jxtlib/JxtInterface.h"

namespace KIOSKCONFIG
{

class KioskRequestInterface : public JxtInterface
{
public:
  KioskRequestInterface() : JxtInterface("","KioskConfig")
  {
     Handler *evHandle;
     evHandle=JxtHandler<KioskRequestInterface>::CreateHandler(&KioskRequestInterface::AppParamsKiosk);
     AddEvent("AppParamsKiosk",evHandle);
     evHandle=JxtHandler<KioskRequestInterface>::CreateHandler(&KioskRequestInterface::AppAliasesKiosk);
     AddEvent("AppAliasesKiosk",evHandle);
  }
  void AppParamsKiosk(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AppAliasesKiosk(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

int alias_to_db(int argc, char **argv);

}

#endif
