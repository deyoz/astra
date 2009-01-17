#ifndef _SALONSFORM_H_
#define _SALONFORM_H_

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
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::CheckInShow);
     AddEvent("CheckInShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::SalonFormShow);
     AddEvent("SalonFormShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::SalonFormWrite);
     AddEvent("SalonFormWrite",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::ExistsRegPassenger);
     AddEvent("ExistsRegPassenger",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::Reseat);
     AddEvent("Reseat",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::DeleteReserveSeat);
     AddEvent("DeleteReserveSeat",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::AutoReseatsPassengers);
     AddEvent("AutoReseatsPassengers",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormShow);
     AddEvent("BaseComponFormShow",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponFormWrite);
     AddEvent("BaseComponFormWrite",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::BaseComponsRead);
     AddEvent("BaseComponsRead",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::ChangeBC);
     AddEvent("ChangeBC",evHandle);
     evHandle=JxtHandler<SalonsInterface>::CreateHandler(&SalonsInterface::Convert_salons);
     AddEvent("Convert_salons",evHandle);
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

  void Convert_salons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

};


#endif /*_SALONFORM_H_*/

