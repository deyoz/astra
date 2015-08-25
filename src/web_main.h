#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

#include "jxtlib/JxtInterface.h"
#include <string>
#include "passenger.h"
#include "typeb_utils.h"
#include "tlg/tlg_parser.h"
#include "web_search.h"
#include "checkin_utils.h"

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
bool is_sync_meridian( const TTripInfo &tripInfo );

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len);

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
     // ����㧪� PNR
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::LoadPnr);
     AddEvent("LoadPnr",evHandle);
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
     // �����⪠ ᫮� "����ࢨ஢���� ����稢������ ����"
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::AddProtPaidLayer);
     AddEvent("AddProtPaidLayer",evHandle);
     // �������� ᫮� "����ࢨ஢���� ����稢������ ����"
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::RemoveProtPaidLayer);
     AddEvent("RemoveProtPaidLayer",evHandle);
     // ����饭�� � �� �訡�� � ����� (���ਬ��, ��᫥ ��㤠筮� ����)
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ClientError);
     AddEvent("ClientError",evHandle);

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
  void AddProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RemoveProtPaidLayer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ClientError(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static bool SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode, xmlNodePtr resNode);

  void GetFlightInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ParseMessage(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetPaxsInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetCacheTable(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

/*
1. �᫨ ��-� 㦥 ��砫 ࠡ���� � pnr (�����,ࠧ���騪 PNL)
2. �᫨ ���ᠦ�� ��ॣ����஢����, � ࠧ���騪 PNL �⠢�� �ਧ��� 㤠�����
*/

struct TWebPax {
    int pax_no; //����㠫�� ��. ��� �裡 ������ � ⮣� �� ���ᠦ�� ࠧ��� ᥣ���⮢
    int crs_pax_id;
    int crs_pax_id_parent;
    int reg_no;
    std::string surname;
    std::string name;
    std::string pers_type_extended; //����� ᮤ�ঠ�� �� (CBBG)
    ASTRA::TCompLayerType crs_seat_layer;
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
    CheckIn::TPaxDocItem doc;
    CheckIn::TPaxDocoItem doco;
    std::list<CheckIn::TPaxDocaItem> doca;
    std::vector<TypeB::TFQTItem> fqt_rems;
    TWebPax() {
      pax_no = ASTRA::NoExists;
      crs_pax_id = ASTRA::NoExists;
      crs_pax_id_parent = ASTRA::NoExists;
      reg_no = ASTRA::NoExists;
      seats = 0;
      pax_id = ASTRA::NoExists;
      crs_pnr_tid = ASTRA::NoExists;
      crs_pax_tid	= ASTRA::NoExists;
      pax_grp_tid = ASTRA::NoExists;
      pax_tid = ASTRA::NoExists;
    };

    bool operator == (const TWebPax &pax) const
    {
      return transliter_equal(surname,pax.surname) &&
             transliter_equal(name,pax.name) &&
             pers_type_extended==pax.pers_type_extended &&
             ((seats==0 && pax.seats==0) || (seats!=0 && pax.seats!=0));
    };
};
bool isOwnerFreePlace( int pax_id, const std::vector<TWebPax> &pnr );

} // namespace AstraWeb

namespace TypeB
{
  void SyncNewCHKD(int point_id_spp, const std::string& task_name, const std::string& params);
  void SyncAllCHKD(int point_id_spp, const std::string& task_name, const std::string& params);
} // namespace TypeB

#endif // __WEB_MAIN_H__

