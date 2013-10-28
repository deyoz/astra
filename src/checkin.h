#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "astra_ticket.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "oralib.h"
#include "etick.h"
#include "remarks.h"
#include "transfer.h"
#include "tlg/tlg_parser.h"

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
    bool trfer_permit;
    bool tckin_permit, tckin_waitlist, tckin_norec;
    TTrferSetsInfo()
    {
      Clear();
    };
    void Clear()
    {
      trfer_permit=false;
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
     AddEvent("TCkinSavePax",evHandle);
     AddEvent("TCkinSaveUnaccompBag",evHandle);
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::LoadPax);
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
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::ParseScanDocData);
     AddEvent("ParseScanDocData",evHandle);
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
  void ParseScanDocData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestDateTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

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
                            const BASIC::TDateTime scd_out_local,
                            std::vector<CheckIn::TTransferItem> &segs);

  static void SavePaxTransfer(int pax_id, int pax_no, const std::vector<CheckIn::TTransferItem> &trfer, int seg_no);
  static std::string SaveTransfer(int grp_id,
                                  const std::vector<CheckIn::TTransferItem> &trfer,
                                  const std::map<int, std::pair<TCkinSegFlts, TTrferSetsInfo> > &trfer_segs,
                                  bool pr_unaccomp,
                                  int seg_no);
  static std::string SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const std::map<int,TSegInfo> &segs, int seg_no);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode,
                      int &first_grp_id, TChangeStatusList &ETInfo, int &tckin_id);

  static void SaveTagPacks(xmlNodePtr node);

  static void LoadPax(int grp_id, xmlNodePtr resNode, bool afterSavePax);
  static void LoadPaxRem(xmlNodePtr paxNode);
  static void LoadPaxTransfer(int pax_id, xmlNodePtr paxNode);
  static void LoadTransfer(int grp_id, std::vector<CheckIn::TTransferItem> &trfer);
  static void BuildTransfer(const TTrferRoute &trfer, xmlNodePtr transferNode);
  static void LoadTransfer(int grp_id, xmlNodePtr transferNode);

  static int CheckCounters(int point_dep,
                           int point_arv,
                           const std::string &cl,
                           ASTRA::TPaxStatus grp_status);

  static bool CheckFQTRem(CheckIn::TPaxRemItem &rem, CheckIn::TPaxFQTItem &fqt);
  static bool ParseFQTRem(TypeB::TTlgParser &tlg, std::string &rem_text, CheckIn::TPaxFQTItem &fqt);

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
};

namespace CheckIn
{

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
    };
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
    bool empty() { return segs.empty(); };
};

void showError(const std::map<int, std::map <int, AstraLocale::LexemaData> > &segs);

} //namespace CheckIn

#endif
