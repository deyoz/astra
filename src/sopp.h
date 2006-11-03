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
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetPaxTransfer);
     AddEvent("GetPaxTransfer",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetBagTransfer);
     AddEvent("GetBagTransfer",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::DeleteAllPassangers);
     AddEvent("DeleteAllPassangers",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteTrips);
     AddEvent("WriteTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadTripInfo);
     AddEvent("ReadTripInfo",evHandle);     
  };
  void ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, bool pr_bag);
  void GetPaxTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBagTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteAllPassangers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif /*_SOPP_H_*/

