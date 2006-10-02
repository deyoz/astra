#ifndef _SOPP_H_
#define _SOPP_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include "JxtInterface.h"

class SoppInterface : public JxtInterface
{
private:
public:
  SoppInterface() : JxtInterface("","sopp")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadTrips);
     AddEvent("ReadTrips",evHandle);     
  };
  void ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
 
#endif /*_SOPP_H_*/

