#ifndef _MAINDCS_H_
#define _MAINDCS_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class MainDCSInterface : public JxtInterface
{
public:
  MainDCSInterface() : JxtInterface("","MainDCS")
  {
     Handler *evHandle;
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::CheckUserLogon);
     AddEvent("CheckUserLogon",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::UserLogon);
     AddEvent("UserLogon",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::UserLogoff);
     AddEvent("UserLogoff",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::SetDefaultPasswd);
     AddEvent("PasswdBtn",evHandle);     
  };

  void CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
  void UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
  void SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif /*_MAINDCS_H_*/
