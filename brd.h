#ifndef _BRD_H_
#define _BRD_H_

#include <libxml/tree.h>
#include "JxtInterface.h"		

class BrdInterface : public JxtInterface
{
public:
  BrdInterface() : JxtInterface("123","brd")
  {
     Handler *evHandle;
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::Trips);
     AddEvent("trips",evHandle);
  };	
  
  void Trips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
