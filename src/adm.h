#ifndef _ADM_H_
#define _ADM_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class AdmInterface : public JxtInterface
{
public:
  AdmInterface() : JxtInterface("","adm")
  {
     Handler *evHandle;
     evHandle=JxtHandler<AdmInterface>::CreateHandler(&AdmInterface::LoadAdm);
     AddEvent("LoadAdm",evHandle);
     evHandle=JxtHandler<AdmInterface>::CreateHandler(&AdmInterface::SetDefaultPasswd);
     AddEvent("PasswdBtn",evHandle);
  };

  void LoadAdm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SetDefaultPasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};


#endif /*_ADM_H_*/

