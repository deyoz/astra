#ifndef _KIOSK_ALIAS_H_
#define _KIOSK_ALIAS_H_

#include "jxtlib/JxtInterface.h"

namespace KIOSK {
    int alias_to_db(int argc, char **argv);
}

class KioskAliasInterface : public JxtInterface
{
public:
  KioskAliasInterface() : JxtInterface("","KioskAlias")
  {
     Handler *evHandle;
     evHandle=JxtHandler<KioskAliasInterface>::CreateHandler(&KioskAliasInterface::KioskAlias);
     AddEvent("kiosk_alias",evHandle);
     evHandle=JxtHandler<KioskAliasInterface>::CreateHandler(&KioskAliasInterface::KioskAliasLocale);
     AddEvent("kiosk_alias_locale",evHandle);
  };

  void KioskAlias(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KioskAliasLocale(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
