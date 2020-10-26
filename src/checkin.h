#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "astra_ticket.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_types.h"
#include "rfisc_sirena.h"
#include "astra_misc.h"
#include "oralib.h"
#include "etick.h"
#include "remarks.h"
#include "transfer.h"
#include "events.h"
#include "tlg/tlg_parser.h"
#include "sirena_exchange.h"
#include "astra_types.h"
#include "checkin_segments.h"


using BASIC::date_time::TDateTime;

class TTrferSetsInfo
{
  public:
    bool trfer_permit, trfer_outboard;
    bool tckin_permit, tckin_waitlist, tckin_norec;
    TTrferSetsInfo()
    {
      Clear();
    };
    void Clear()
    {
      trfer_permit=false;
      trfer_outboard=false;
      tckin_permit=false;
      tckin_waitlist=false;
      tckin_norec=false;
    };
};

void traceTrfer( TRACE_SIGNATURE,
                 const std::string &descr,
                 const std::map<int, CheckIn::TTransferItem> &trfer );

void traceTrfer( TRACE_SIGNATURE,
                 const std::string &descr,
                 const std::map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> > &segs );

namespace CheckIn
{

enum TAfterSaveActionType { actionNone, actionSvcAvailability, actionSvcPaymentStatus };

class TAfterSaveSegInfo
{
  public:
    int point_dep;
    int grp_id;
    TGrpToLogInfo grpInfoBefore, grpInfoAfter;
    void clear()
    {
      point_dep=ASTRA::NoExists;
      grp_id=ASTRA::NoExists;
      grpInfoBefore.clear();
      grpInfoAfter.clear();
    }
    TAfterSaveSegInfo()
    {
      clear();
    }
};

class TAfterSaveInfo
{
  public:
    std::list<TAfterSaveSegInfo> segs;
    int tckin_id;
    CheckIn::TAfterSaveActionType action;
    int agent_stat_period;
    CheckIn::TGrpEMDProps handmadeEMDDiff;
    void clear()
    {
      segs.clear();
      tckin_id=ASTRA::NoExists;
      action=actionNone;
      agent_stat_period=ASTRA::NoExists;
      handmadeEMDDiff.clear();
    }
    TAfterSaveInfo()
    {
      clear();
    }
    void toLog(const std::string& where);
};

struct TAfterSaveInfoData
{
    xmlNodePtr reqNode;
    xmlNodePtr answerResNode;
    bool httpWasSent;
    int grpId;
    TAfterSaveActionType action;

    TAfterSaveInfoData(xmlNodePtr rNode, xmlNodePtr aNode)
        : reqNode(rNode), answerResNode(aNode), httpWasSent(false),
          grpId(ASTRA::NoExists), action(CheckIn::actionNone)
    {}

    bool needSync() const;
    xmlNodePtr getAnswerNode() const;
};

class TAfterSaveInfoList : public std::list<TAfterSaveInfo>
{
  public:
    void handle(TAfterSaveInfoData& data, const std::string& where);
  protected:
    void handleInner(TAfterSaveInfoData& data, const std::string& where);
};

} //namespace CheckIn

class CheckInInterface : public JxtInterface
{
public:
  CheckInInterface() : JxtInterface("","CheckIn")
  {
     AddEvent("LoadTagPacks",         JXT_HANDLER(CheckInInterface, LoadTagPacks));
     AddEvent("SearchGrp",            JXT_HANDLER(CheckInInterface, SearchGrp));
     AddEvent("SearchPax",            JXT_HANDLER(CheckInInterface, SearchPax));
     AddEvent("SearchPaxByDoc",       JXT_HANDLER(CheckInInterface, SearchPaxByDoc));
     AddEvent("TCkinSavePax",         JXT_HANDLER(CheckInInterface, SavePax));
     AddEvent("TCkinSaveUnaccompBag", JXT_HANDLER(CheckInInterface, SavePax));
     AddEvent("TCkinLoadPax",         JXT_HANDLER(CheckInInterface, LoadPax));
     AddEvent("PaxList",              JXT_HANDLER(CheckInInterface, PaxList));
     AddEvent("BagPaxList",           JXT_HANDLER(CheckInInterface, PaxList));
     AddEvent("ArrivalPaxList",       JXT_HANDLER(CheckInInterface, ArrivalPaxList));
     AddEvent("GetTripCounters",      JXT_HANDLER(CheckInInterface, GetTripCounters));
     AddEvent("OpenCheckInInfo",      JXT_HANDLER(CheckInInterface, OpenCheckInInfo));
     AddEvent("CheckTCkinRoute",      JXT_HANDLER(CheckInInterface, CheckTCkinRoute));
     AddEvent("ParseScanDocData",     JXT_HANDLER(CheckInInterface, ParseScanDocData));
     AddEvent("CREWCHECKIN",          JXT_HANDLER(CheckInInterface, CrewCheckin));
     AddEvent("GetFQTTierLevel",      JXT_HANDLER(CheckInInterface, GetFQTTierLevel));
  }

  void LoadTagPacks(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchGrp(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchPaxByDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ArrivalPaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripCounters(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void OpenCheckInInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CheckTCkinRoute(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ParseScanDocData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CrewCheckin(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetFQTTierLevel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}

  static void GetTCkinFlights(const TTripInfo &operFlt,
                              const std::map<int, CheckIn::TTransferItem> &trfer,
                              std::map<int, std::pair<CheckIn::TTransferItem, CheckIn::Segments> > &segs);

  static void SaveTransfer(int grp_id, const CheckIn::TTransferList &trfer,
                           const std::map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs,
                           int seg_no, TLogLocale& tlocale);
  static void SaveTCkinSegs(int grp_id,
                            const CheckIn::SegmentMap &segs,
                            const std::vector<CheckIn::IatciSegment>& iatciSegs,
                            TLogLocale& tlocale);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode,
                      TChangeStatusList &ChangeStatusInfo,
                      SirenaExchange::TLastExchangeList &SirenaExchangeList,
                      CheckIn::TAfterSaveInfoList &AfterSaveInfoList,
                      bool& httpWasSent);

  static void SaveTagPacks(xmlNodePtr node);

  static void AfterSaveAction(CheckIn::TAfterSaveInfoData& data);
  static void tryChangeFlight(const CheckIn::TSimplePaxGrpItem& grp,
                              xmlNodePtr reqNode, xmlNodePtr resNode);
  static void LoadPaxByGrpId(int grp_id, xmlNodePtr reqNode, xmlNodePtr resNode, bool afterSavePax);
  static void LoadPax(xmlNodePtr reqNode, xmlNodePtr resNode);
  static void LoadIatciPax(xmlNodePtr reqNode, xmlNodePtr resNode, const GrpId_t& grpId, bool needSync);
  static void PaxRemToXML(xmlNodePtr paxNode);
  static void BuildTransfer(const TTrferRoute &trfer, TTrferRouteType route_type, xmlNodePtr transferNode);
  static void BuildTCkinSegments(int grp_id, xmlNodePtr tckinNode);
  static void LoadTransfer(int grp_id, xmlNodePtr transferNode);

  static bool CheckFQTRem(const CheckIn::TPaxRemItem &rem, CheckIn::TPaxFQTItem &fqt);
  static bool ParseFQTRem(TypeB::TTlgParser &tlg, std::string &rem_text, CheckIn::TPaxFQTItem &fqt);
//  static bool CheckAPPSRems(const std::multiset<CheckIn::TPaxRemItem> &rems, std::string& override, bool& is_forced);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData(int point_id, xmlNodePtr dataNode);
  static void readTripSets( int point_id, xmlNodePtr dataNode );
  static void readTripSets( int point_id, const TTripInfo &fltInfo, xmlNodePtr tripSetsNode );

  static void GetOnwardCrsTransfer(int id, bool isPnrId,
                                   const TTripInfo &operFlt,
                                   const std::string &oper_airp_arv,
                                   std::map<int, CheckIn::TTransferItem> &trfer);

  static void LoadOnwardCrsTransfer(const std::map<int, CheckIn::TTransferItem> &trfer,
                                    const std::map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs,
                                    xmlNodePtr trferNode);
  static void LoadOnwardCrsTransfer(const std::map<int, std::pair<CheckIn::TTransferItem, TTrferSetsInfo> > &trfer,
                                    xmlNodePtr trferNode);

  static std::map<int, CheckIn::TTransferItem> getCrsTransferMap(const PaxId_t &pax_id);

  static void GetTrferSets(const TTripInfo &operFlt,
                           const std::string &oper_airp_arv,
                           const std::string &tlg_airp_dep,
                           const std::map<int, CheckIn::TTransferItem> &trfer,
                           const bool get_trfer_permit_only,
                           std::map<int, std::pair<CheckIn::Segments, TTrferSetsInfo> > &trfer_segs);

  static CheckInInterface* instance();
};

#ifdef XP_TESTING
int lastGeneratedPaxId();
void setLastGeneratedPaxId(int lgpid);
#endif/*XP_TESTING*/

#endif
