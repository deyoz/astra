#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "astra_ticket.h"
#include "astra_consts.h"
#include "astra_misc.h"

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
     AddEvent("SaveUnaccompBag",evHandle);
     AddEvent("TCkinSavePax",evHandle);
     AddEvent("TCkinSaveUnaccompBag",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::LoadPax);
     AddEvent("LoadPax",evHandle);
     AddEvent("TCkinLoadPax",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::PaxList);
     AddEvent("PaxList",evHandle);
     AddEvent("BagPaxList",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::GetTripCounters);
     AddEvent("GetTripCounters",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::OpenCheckInInfo);
     AddEvent("OpenCheckInInfo",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::CheckTCkinRoute);
     AddEvent("CheckTCkinRoute",evHandle);

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
  void OpenCheckInInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CheckTCkinRoute(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  bool CheckCkinFlight(const int point_dep,
                       const std::string& airp_dep,
                       int& point_arv,
                       const std::string& airp_arv,
                       bool lock,
                       TTripInfo& fltInfo);

  void SavePaxRem(xmlNodePtr paxNode);
  void SavePaxTransfer(int pax_id, xmlNodePtr paxNode, xmlNodePtr transferNode, int seg_no);
  std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms, bool pr_unaccomp );
  std::string SaveTransfer(int grp_id, xmlNodePtr transferNode, bool pr_unaccomp, int seg_no);
  static void SaveBag(int point_id, int grp_id, xmlNodePtr bagtagNode);
  static void SavePaidBag(int grp_id, xmlNodePtr paidbagNode);

  void SaveBagToLog(int point_id, int grp_id, xmlNodePtr bagtagNode);
  void SaveTagPacks(xmlNodePtr node);

  void LoadPax(int grp_id, xmlNodePtr resNode, bool tckin_version);
  void LoadPaxRem(xmlNodePtr paxNode);
  void LoadPaxTransfer(int pax_id, xmlNodePtr paxNode, xmlNodePtr transferNode);
  void LoadPaxNorms(xmlNodePtr paxNode, bool pr_unaccomp);
  void LoadTransfer(int grp_id, xmlNodePtr transferNode);
  static void LoadBag(int grp_id, xmlNodePtr bagtagNode);
  static void LoadPaidBag(int grp_id, xmlNodePtr grpNode);

  int CheckCounters(int point_dep, int point_arv, char* cl, ASTRA::TPaxStatus grp_status);
  bool CheckFltOverload(int point_id, const TTripInfo &fltInfo);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
};


#endif
