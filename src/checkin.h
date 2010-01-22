#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "astra_ticket.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "tlg/tlg_parser.h"

struct TSegInfo
{
  int point_dep,point_arv;
  std::string airp_dep,airp_arv;
  int point_num,first_point;
  bool pr_tranzit;
  TTripInfo fltInfo;
};


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
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::ArrivalPaxList);
     AddEvent("ArrivalPaxList",evHandle);
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
  void ArrivalPaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void OpenCheckInInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CheckTCkinRoute(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static bool CheckCkinFlight(const int point_dep,
                              const std::string& airp_dep,
                              const int point_arv,
                              const std::string& airp_arv,
                              bool lock,
                              TSegInfo& segInfo);

  static void SavePaxRem(xmlNodePtr paxNode);
  static void SavePaxTransfer(int pax_id, xmlNodePtr paxNode, xmlNodePtr transferNode, int seg_no);
  static std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms, bool pr_unaccomp);
  static std::string SaveTransfer(int grp_id, xmlNodePtr transferNode, bool pr_unaccomp, int seg_no);
  static std::string SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const std::map<int,TSegInfo> &segs, int seg_no);
  static void SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);
  static void SaveBag(int point_id, int grp_id, xmlNodePtr bagtagNode);
  static void SavePaidBag(int grp_id, xmlNodePtr paidbagNode);

  static void SaveBagToLog(int point_id, int grp_id, xmlNodePtr bagtagNode);
  static void SaveTagPacks(xmlNodePtr node);

  static void LoadPax(int grp_id, xmlNodePtr resNode, bool tckin_version);
  static void LoadPaxRem(xmlNodePtr paxNode);
  static void LoadPaxTransfer(int pax_id, xmlNodePtr paxNode, xmlNodePtr transferNode);
  static void LoadPaxNorms(xmlNodePtr paxNode, bool pr_unaccomp);
  static void LoadTransfer(int grp_id, xmlNodePtr transferNode);
  static void LoadBag(int grp_id, xmlNodePtr bagtagNode);
  static void LoadPaidBag(int grp_id, xmlNodePtr grpNode);

  static int CheckCounters(int point_dep, int point_arv, char* cl, ASTRA::TPaxStatus grp_status);

  static bool ParseFQTRem(TTlgParser &tlg,std::string &rem_text,TFQTItem &fqt);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
};


#endif
