#ifndef _SALONSFORM2_H_
#define _SALONFORM2_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"


class SalonsInterface : public JxtInterface
{
private:
public:
  SalonsInterface() : JxtInterface("","salons")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::ExistsRegPassenger);
     AddEvent("ExistsRegPassenger",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormShow); //!!old terminal
     AddEvent("BaseComponFormShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormWrite);//!!old terminal
     AddEvent("BaseComponFormWrite",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponsRead);
     AddEvent("BaseComponsRead",evHandle);
  };
  void ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

};

bool showComponAirlineColumn();
bool showComponAirpColumn();


#endif /*_SALONFORM2_H_*/

