#ifndef _PAY_H_
#define _PAY_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class PayInterface : public JxtInterface
{
public:
  PayInterface() : JxtInterface("","pay")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PayInterface>::CreateHandler(&PayInterface::LoadBag);
     AddEvent("LoadBag",evHandle);
     evHandle=JxtHandler<PayInterface>::CreateHandler(&PayInterface::LoadPaidBag);
     AddEvent("LoadPaidBag",evHandle);          
     evHandle=JxtHandler<PayInterface>::CreateHandler(&PayInterface::SaveBag);
     AddEvent("SaveBag",evHandle);          
     evHandle=JxtHandler<PayInterface>::CreateHandler(&PayInterface::SavePaidBag);
     AddEvent("SavePaidBag",evHandle);               
  };

  void LoadBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPaidBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
  void SavePaidBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_PAY_H_*/

