#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

#include "jxtlib/JxtInterface.h"
#include <string>
#include "passenger.h"
#include "typeb_utils.h"
#include "web_search.h"
#include "checkin_utils.h"
#include "brands.h"
#include "etick.h"
#include "trip_tasks.h"
#include "web_exchange.h"
#include <tuple>
#include <vector>
#include <cstdint>
#include "print.h"

#define WEB_JXT_IFACE_ID "WEB"
#define EMUL_CLIENT_TYPE ctWeb

struct InetClient
{
  std::string client_id;
  std::string pult;
  std::string opr;
  std::string client_type;
};

InetClient getInetClient(std::string client_id);

namespace AstraWeb
{

void GetBPPaxFromScanCode(const std::string &scanCode, PrintInterface::BPPax &pax);

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> internet_main(const std::vector<uint8_t>& body, const char *head, size_t hlen);

class WebRequestsIface : public JxtInterface
{
public:
  WebRequestsIface() : JxtInterface("",WEB_JXT_IFACE_ID)
  {
     Handler *evHandle;
     // ����७�� ���� ���ᠦ�஢
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SearchPNRs);
     AddEvent("SearchPNRs",evHandle);
     // ���ଠ�� � ३�
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SearchFlt);
     AddEvent("SearchFlt",evHandle);
     AddEvent("SearchFltMulti",evHandle);
     // ����㧪� PNR
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::LoadPnr);
     AddEvent("LoadPnr",evHandle);
     AddEvent("LoadPnrMulti",evHandle);
     // ����㧪� ����������
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ViewCraft);
     AddEvent("ViewCraft",evHandle);
     // ��������� ���ᠦ�஢
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SavePax);
     AddEvent("SavePax",evHandle);
     // ����祭�� ���⠡� � ������ ��� ��ᠤ�筮�� ⠫���
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetPrintDataBP);
     AddEvent("GetPrintDataBP",evHandle);
     // ���⢥ত���� ����
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ConfirmPrintBP);
     AddEvent("ConfirmPrintBP",evHandle);
     // ����祭�� ⥣�� ��� ���. ⠫���
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetBPTags);
     AddEvent("GetBPTags",evHandle);

     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ChangeProtLayer);
     AddEvent("AddProtPaidLayer",evHandle);
     AddEvent("AddProtLayer",evHandle);
     AddEvent("RemoveProtPaidLayer",evHandle);
     AddEvent("RemoveProtLayer",evHandle);

     // ����饭�� � �� �訡�� � ����� (���ਬ��, ��᫥ ��㤠筮� ����)
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ClientError);
     AddEvent("ClientError",evHandle);
     // ���⢥ত���� ������
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::PaymentStatus);
     AddEvent("PaymentStatus",evHandle);

     //���⥬� ��ਤ���
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetFlightInfo);
     AddEvent("GetFlightInfo",evHandle);
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ParseMessage);
     AddEvent("Message",evHandle);
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetPaxsInfo);
     AddEvent("GetPaxsInfo",evHandle);
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetCacheTable);
     AddEvent("GetCacheTable",evHandle);
  };

  void SearchPNRs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ConfirmPrintBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangeProtLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaymentStatus(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);

  static void IntViewCraft(xmlNodePtr reqNode, xmlNodePtr resNode);

  void GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void ParseMessage(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetCacheTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void emulateClientType();

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){}
};

/*
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
*/

struct TWebPax {
    int pax_no; //����㠫�� ��. ��� �裡 ������ � ⮣� �� ���ᠦ�� ࠧ��� ᥣ���⮢
    int crs_pnr_id;
    int crs_pax_id;
    int crs_pax_id_parent;
    int reg_no;
    std::string surname;
    std::string name;
    std::string pers_type_extended; //����� ᮤ�ঠ�� �� (CBBG)
    std::string seat_status;
    std::string crs_seat_no;
    std::string seat_no;
    std::string pass_class;
    std::string pass_subclass;
    int seats;
    int pax_id;
    int crs_pnr_tid;
    int crs_pax_tid;
    int pax_grp_tid;
    int pax_tid;
    std::string checkin_status;
    std::set<std::string> agent_checkin_reasons;
    CheckIn::TPaxTknItem tkn;
    TBrand brand;
    CheckIn::TPaxDocItem doc;
    CheckIn::TPaxDocoItem doco;
    TETickItem etick;
    CheckIn::TDocaMap doca_map;
    std::set<CheckIn::TPaxFQTItem> fqts;
    std::multiset<CheckIn::TPaxRemItem> rems_and_asvc;
    TPnrAddrs pnr_addrs;
    TWebPax() {
      clear();
    }

    TWebPax(const ProtLayerResponse::Pax& pax)
    {
      clear();
      crs_pax_id=pax.id;
      crs_seat_no=pax.seat_no;
      crs_pnr_tid=pax.crs_pnr_tid;
      crs_pax_tid=pax.crs_pax_tid;
      pax_grp_tid=pax.pax_grp_tid;
      pax_tid=pax.pax_tid;
      pass_class=pax.pnr_class;
      pass_subclass=pax.pnr_subclass;
      seats=pax.seats;
    }

    void clear()
    {
      pax_no = ASTRA::NoExists;
      crs_pnr_id = ASTRA::NoExists;
      crs_pax_id = ASTRA::NoExists;
      crs_pax_id_parent = ASTRA::NoExists;
      reg_no = ASTRA::NoExists;
      seats = 0;
      pax_id = ASTRA::NoExists;
      crs_pnr_tid = ASTRA::NoExists;
      crs_pax_tid	= ASTRA::NoExists;
      pax_grp_tid = ASTRA::NoExists;
      pax_tid = ASTRA::NoExists;
    }

    bool operator == (const TWebPax &pax) const
    {
      return transliter_equal(surname,pax.surname) &&
             transliter_equal(name,pax.name) &&
             pers_type_extended==pax.pers_type_extended &&
             ((seats==0 && pax.seats==0) || (seats!=0 && pax.seats!=0)) &&
             pnr_addrs.equalPnrExists(pax.pnr_addrs);
    };

    bool suitable(const WebSearch::TPNRFilter &filter) const;
    void toXML(xmlNodePtr paxParentNode, const TRemGrp &outputRemGrp) const;
};
int bcbp_test(int argc,char **argv);

} // namespace AstraWeb

namespace TypeB
{
  void SyncNewCHKD(const TTripTaskKey &task);
  void SyncAllCHKD(const TTripTaskKey &task);
} // namespace TypeB

#endif // __WEB_MAIN_H__

