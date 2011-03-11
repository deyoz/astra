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

class TCkinSetsInfo
{
  public:
    bool pr_permit,pr_waitlist,pr_norec;
    TCkinSetsInfo()
    {
      Clear();
    };
    void Clear()
    {
      pr_permit=false;
      pr_waitlist=false;
      pr_norec=false;
    };
};

namespace CheckIn
{

class TPaxTransferItem
{
  public:
    int pax_id;
    std::string subclass;
    TElemFmt subclass_fmt;
    TPaxTransferItem()
    {
      pax_id=ASTRA::NoExists;
      subclass_fmt=efmtUnknown;
    };
};

class TTransferItem
{
  public:
    std::string flight_view;
    TTripInfo operFlt;
    int grp_id;
    std::string airp_arv;
    TElemFmt airp_arv_fmt;
    std::string subclass;
    TElemFmt subclass_fmt;
    std::vector<TPaxTransferItem> pax;
    TTransferItem()
    {
      grp_id=ASTRA::NoExists;
      airp_arv_fmt=efmtUnknown;
      subclass_fmt=efmtUnknown;
    };
};

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
                              bool lock,
                              TSegInfo& segInfo);
  static void GetTCkinFlights(const std::vector<CheckIn::TTransferItem> &trfer,
                              std::vector< TCkinSegFlts > &segs);

  static void ParseTransfer(xmlNodePtr trferNode,
                            xmlNodePtr paxNode,
                            const TSegInfo &firstSeg,
                            std::vector<CheckIn::TTransferItem> &segs);
  static void ParseTransfer(xmlNodePtr trferNode,
                            xmlNodePtr paxNode,
                            const std::string &airp_arv,
                            const BASIC::TDateTime scd_out_local,
                            std::vector<CheckIn::TTransferItem> &segs);

  static void SavePaxRem(xmlNodePtr paxNode);
  static void SavePaxTransfer(int pax_id, int pax_no, const std::vector<CheckIn::TTransferItem> &trfer, int seg_no);
  static std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms, bool pr_unaccomp);
  static std::string SaveTransfer(int grp_id, const std::vector<CheckIn::TTransferItem> &trfer, bool pr_unaccomp, int seg_no);
  static std::string SaveTCkinSegs(int grp_id, xmlNodePtr segsNode, const std::map<int,TSegInfo> &segs, int seg_no);
  static bool SavePax(xmlNodePtr termReqNode, xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);
  static void SaveBag(int point_id, int grp_id, int hall, xmlNodePtr bagtagNode);
  static void SavePaidBag(int grp_id, xmlNodePtr paidbagNode);

  static void SaveBagToLog(int point_id, int grp_id, xmlNodePtr bagtagNode);
  static void SaveTagPacks(xmlNodePtr node);

  static void LoadPax(int grp_id, xmlNodePtr resNode, bool tckin_version);
  static void LoadPaxRem(xmlNodePtr paxNode);
  static void LoadPaxTransfer(int pax_id, xmlNodePtr paxNode, xmlNodePtr transferNode);
  static void LoadPaxNorms(xmlNodePtr paxNode, bool pr_unaccomp);
  static void LoadTransfer(int grp_id, std::vector<CheckIn::TTransferItem> &trfer);
  static void BuildTransfer(const std::vector<CheckIn::TTransferItem> &trfer, xmlNodePtr transferNode);
  static void LoadTransfer(int grp_id, xmlNodePtr transferNode);
  static void LoadBag(int grp_id, xmlNodePtr bagtagNode);
  static void LoadPaidBag(int grp_id, xmlNodePtr grpNode);

  static int CheckCounters(int point_dep, int point_arv, char* cl, ASTRA::TPaxStatus grp_status);

  static bool CheckFQTRem(xmlNodePtr remNode, TFQTItem &fqt);
  static bool ParseFQTRem(TTlgParser &tlg,std::string &rem_text,TFQTItem &fqt);

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void readTripSets( int point_id, xmlNodePtr dataNode );
  static void readTripSets( int point_id, const TTripInfo &fltInfo, xmlNodePtr tripSetsNode );
  static void readTripSets( const TTripInfo &fltInfo, int pr_etstatus, xmlNodePtr tripSetsNode);
  
  static void GetOnwardCrsTransfer(int pnr_id, TQuery &Qry, std::vector<TTransferItem> &trfer);
  static void LoadOnwardCrsTransfer(const TTripInfo &operFlt,
                                    const std::string &oper_airp_arv,
                                    const std::string &tlg_airp_dep,
                                    const std::vector<TTransferItem> &crs_trfer,
                                    std::vector<CheckIn::TTransferItem> &trfer,
                                    xmlNodePtr trferNode);

  static bool CheckTCkinPermit(const std::string &airline_in,
                               const int flt_no_in,
                               const std::string &airp,
                               const std::string &airline_out,
                               const int flt_no_out,
                               TCkinSetsInfo &sets);
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
      segs[point_id][pax_id]=lexemeData;
    };
    ~UserException() throw(){};
};

void showError(const std::map<int, std::map <int, AstraLocale::LexemaData> > &segs);

} //namespace CheckIn

#endif
