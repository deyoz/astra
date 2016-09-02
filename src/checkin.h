#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "baggage_pc.h"
#include "astra_ticket.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "oralib.h"
#include "etick.h"
#include "remarks.h"
#include "transfer.h"
#include "events.h"
#include "tlg/tlg_parser.h"

using BASIC::date_time::TDateTime;

struct TSegInfo
{
  int point_dep,point_arv;
  std::string airp_dep,airp_arv;
  int point_num,first_point;
  bool pr_tranzit;
  TTripInfo fltInfo;
};

struct TCkinSegFlts
{
  std::vector<TSegInfo> flts;
  bool is_edi;
};

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
                 const std::map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > &segs );

namespace CheckIn
{

enum TAfterSaveActionType { actionNone, actionCheckPieceConcept, actionRefreshPaidBagPC };

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
    CheckIn::TPaidBagEMDProps handmadeEMDDiff;
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

class TAfterSaveInfoList : public std::list<TAfterSaveInfo>
{
  public:
    void handle(const std::string& where);
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

  static std::string GetSearchPaxSubquery(ASTRA::TPaxStatus pax_status,
                                          bool return_pnr_ids,
                                          bool exclude_checked,
                                          bool exclude_deleted,
                                          bool select_pad_with_ok,
                                          std::string sql_filter);

  static bool CheckCkinFlight(const int point_dep,
                              const std::string& airp_dep,
                              const int point_arv,
                              const std::string& airp_arv,
                              TSegInfo& segInfo);
  static void GetTCkinFlights(const std::map<int, CheckIn::TTransferItem> &trfer,
                              std::map<int, std::pair<CheckIn::TTransferItem, TCkinSegFlts> > &segs);

  static void ParseTransfer(xmlNodePtr trferNode,
                            xmlNodePtr paxNode,
                            const TSegInfo &firstSeg,
                            std::vector<CheckIn::TTransferItem> &segs);
  static void ParseTransfer(xmlNodePtr trferNode,
                            xmlNodePtr paxNode,
                            const std::string &airp_arv,
                            const TDateTime scd_out_local,
                            std::vector<CheckIn::TTransferItem> &segs);

  static void SavePaxTransfer(int pax_id, int pax_no, const std::vector<CheckIn::TTransferItem> &trfer, int seg_no);
  static void SaveTransfer(int grp_id, const std::vector<CheckIn::TTransferItem> &trfer,
                           const std::map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs,
                           bool pr_unaccomp, int seg_no, TLogLocale& tlocale);
  static void SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const std::map<int,TSegInfo> &segs, int seg_no, TLogLocale& tlocale);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode,
                      TChangeStatusList &ChangeStatusInfo,
                      SirenaExchange::TLastExchangeList &SirenaExchangeList,
                      CheckIn::TAfterSaveInfoList &AfterSaveInfoList);

  static void SaveTagPacks(xmlNodePtr node);

  static void AfterSaveAction(int first_grp_id, CheckIn::TAfterSaveActionType action);
  static void LoadPax(int grp_id, xmlNodePtr reqNode, xmlNodePtr resNode, bool afterSavePax);
  static void LoadPax(xmlNodePtr reqNode, xmlNodePtr resNode);
  static void LoadIatciPax(xmlNodePtr reqNode, xmlNodePtr resNode, int grpId, bool needSync);
  static void LoadPaxRem(xmlNodePtr paxNode);
  static void BuildTransfer(const TTrferRoute &trfer, TTrferRouteType route_type, xmlNodePtr transferNode);
  static void LoadTransfer(int grp_id, xmlNodePtr transferNode);

  static int CheckCounters(int point_dep,
                           int point_arv,
                           const std::string &cl,
                           ASTRA::TPaxStatus grp_status,
                           const TCFG &cfg,
                           bool free_seating);

  static bool CheckFQTRem(const CheckIn::TPaxRemItem &rem, CheckIn::TPaxFQTItem &fqt);
  static bool ParseFQTRem(TypeB::TTlgParser &tlg, std::string &rem_text, CheckIn::TPaxFQTItem &fqt);
  static bool CheckAPPSRems(const std::multiset<CheckIn::TPaxRemItem> &rems, std::string& override, bool& is_forced);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void readTripSets( int point_id, xmlNodePtr dataNode );
  static void readTripSets( int point_id, const TTripInfo &fltInfo, xmlNodePtr tripSetsNode );

  static void GetOnwardCrsTransfer(int pnr_id, TQuery &Qry,
                                   const TTripInfo &operFlt,
                                   const std::string &oper_airp_arv,
                                   std::map<int, CheckIn::TTransferItem> &trfer);

  static void LoadOnwardCrsTransfer(const std::map<int, CheckIn::TTransferItem> &trfer,
                                    const std::map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs,
                                    xmlNodePtr trferNode);
  static void LoadOnwardCrsTransfer(const std::map<int, std::pair<CheckIn::TTransferItem, TTrferSetsInfo> > &trfer,
                                    xmlNodePtr trferNode);

  static void GetTrferSets(const TTripInfo &operFlt,
                           const std::string &oper_airp_arv,
                           const std::string &tlg_airp_dep,
                           const std::map<int, CheckIn::TTransferItem> &trfer,
                           const bool get_trfer_permit_only,
                           std::map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs);

  static CheckInInterface* instance();
};

namespace CheckIn
{

class OverloadException: public AstraLocale::UserException
{
  public:
    OverloadException(const std::string &msg):AstraLocale::UserException(msg) {}
    virtual ~OverloadException() throw(){}
};

class UserException:public AstraLocale::UserException
{
    public:
      std::map<int, std::map <int, AstraLocale::LexemaData> > segs;

      UserException(const AstraLocale::LexemaData &lexemeData,
                  int point_id,
                  int pax_id = ASTRA::NoExists):AstraLocale::UserException(lexemeData.lexema_id, lexemeData.lparams)
    {
      addError(lexemeData, point_id, pax_id);
    };
    UserException():AstraLocale::UserException("Empty CheckIn::UserException!", AstraLocale::LParams()) {};
    ~UserException() throw(){};
    void addError(const AstraLocale::LexemaData &lexemeData,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
      if (segs.empty()) setLexemaData(lexemeData);
      segs[point_id][pax_id]=lexemeData;
    }
/*  если кто-то надумает раскомментарить этот кусок, обратитесь сначала к Владу
    void addError(const std::string &lexema_id, const AstraLocale::LParams &lparams,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
      AstraLocale::LexemaData data;
      data.lexema_id = lexema_id;
        data.lparams = lparams;
        addError(data, point_id, pax_id);
    };
    void addError(const std::string &lexema_id,
                  int point_id,
                  int pax_id = ASTRA::NoExists)
    {
        addError(lexema_id, AstraLocale::LParams(), point_id, pax_id);
    };*/
    bool empty() { return segs.empty(); }
};

void showError(const std::map<int, std::map <int, AstraLocale::LexemaData> > &segs);

} //namespace CheckIn

#endif
