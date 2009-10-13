#ifndef _ACCESS_H_
#define _ACCESS_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class AccessInterface : public JxtInterface
{
public:
  AccessInterface() : JxtInterface("123","access")
  {
     Handler *evHandle;
     evHandle=JxtHandler<AccessInterface>::CreateHandler(&AccessInterface::RoleRights);
     AddEvent("role_rights",evHandle);
  };

  void RoleRights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
