#ifndef _SALONSFORM2_H_
#define _SALONFORM2_H_

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"


class SalonsInterface : public JxtInterface
{
private:
public:
  SalonsInterface() : JxtInterface("","salons")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::CheckInShow); // !!!old terminal
     AddEvent("CheckInShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::SalonFormShow); //!!old terminal
     AddEvent("SalonFormShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::SalonFormWrite);
     AddEvent("SalonFormWrite",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::ExistsRegPassenger);
     AddEvent("ExistsRegPassenger",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::Reseat); //!!old terminal
     AddEvent("Reseat",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::DeleteReserveSeat); //!!old terminal
     AddEvent("DeleteReserveSeat",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::AutoReseatsPassengers);  //!!old terminal
     AddEvent("AutoReseatsPassengers",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormShow); //!!old terminal
     AddEvent("BaseComponFormShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormWrite);//!!old terminal
     AddEvent("BaseComponFormWrite",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponsRead);
     AddEvent("BaseComponsRead",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::ChangeBC);
     AddEvent("ChangeBC",evHandle);
     //////////////////new layer mode////////////////////////////////
  };

  void CheckInShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SalonFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteReserveSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void AutoReseatsPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeBC(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

};


#endif /*_SALONFORM2_H_*/

