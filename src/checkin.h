#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "JxtInterface.h"
#include "astra_ticket.h"

class CheckInInterface : public JxtInterface
{
public:
  CheckInInterface() : JxtInterface("","CheckIn")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::LoadTagPacks);
     AddEvent("LoadTagPacks",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::SearchGrp);
     AddEvent("SearchGrp",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::SearchPax);
     AddEvent("SearchPax",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::SavePax);
     AddEvent("SavePax",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::LoadPax);
     AddEvent("LoadPax",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::PaxList);
     AddEvent("PaxList",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::GetTripCounters);
     AddEvent("GetTripCounters",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::GetEvents);
     AddEvent("GetEvents",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::TestDateTime);
     AddEvent("TestDateTime",evHandle);
  };

  void LoadTagPacks(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchGrp(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  void SavePaxRem(xmlNodePtr paxNode);
  std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms);
  void SaveTransfer(xmlNodePtr grpNode);
  void SaveBag(xmlNodePtr grpNode);
  void SavePaidBag(xmlNodePtr grpNode);

  void SaveBagToLog(xmlNodePtr grpNode);
  void SaveTagPacks(xmlNodePtr node);

  void LoadPaxRem(xmlNodePtr paxNode);
  void LoadPaxNorms(xmlNodePtr paxNode);
  void LoadTransfer(xmlNodePtr grpNode);
  void LoadBag(xmlNodePtr grpNode);
  void LoadPaidBag(xmlNodePtr grpNode);

  int CheckCounters(int point_dep, int point_arv, char* cl, char grp_status);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
};


#endif
