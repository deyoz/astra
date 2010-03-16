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
     evHandle=JxtHandler<AccessInterface>::CreateHandler(&AccessInterface::SaveRoleRights);
     AddEvent("save_role_rights",evHandle);
  };

  void SaveRoleRights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RoleRights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
