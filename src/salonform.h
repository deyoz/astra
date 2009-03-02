#ifndef _SALONSFORM_H_
#define _SALONFORM_H_

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"


class SalonFormInterface : public JxtInterface
{
private:
public:
  SalonFormInterface() : JxtInterface("","salonform")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Show);
     AddEvent("Show",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Write);
     AddEvent("Write",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::Reseat);
     AddEvent("Reseat",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::DropSeats);
     AddEvent("DropSeats",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::DeleteProtCkinSeat);
     AddEvent("DeleteProtCkinSeat",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::WaitList);
     AddEvent("WaitList",evHandle);
     evHandle=JxtHandler<SalonFormInterface>::CreateHandler(&SalonFormInterface::AutoSeats);
     AddEvent("AutoSeats",evHandle);
  };
  void Show(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DropSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteProtCkinSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WaitList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AutoSeats(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};


#endif /*_SALONFORM_H_*/

