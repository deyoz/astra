#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <libxml/tree.h>
#include "JxtInterface.h"		
#include "astra_ticket.h"

class CheckInInterface : public JxtInterface
{
public:
  CheckInInterface() : JxtInterface("","CheckIn")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::SavePax);
     AddEvent("SavePax",evHandle);     
     AddEvent("kick",JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::KickHandler));
  };	
  
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};  
};

typedef enum {csaFlt,csaGrp,csaPax} TETCheckStatusArea;

bool ETCheckStatus(const Ticketing::OrigOfRequest &org, int id, TETCheckStatusArea area, int point_id=-1);

#endif
