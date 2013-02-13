#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

//#ifdef __cpluplus
#include "jxtlib/JxtInterface.h"

#define WEB_JXT_IFACE_ID "WEB"
#define EMUL_CLIENT_TYPE ctWeb


namespace AstraWeb
{

struct InetClient
{
  int client_id;
  std::string pult;
  std::string opr;
  std::string client_type;
};

bool is_sync_meridian( const TTripInfo &tripInfo );

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len);

class WebRequestsIface : public JxtInterface
{
public:
  WebRequestsIface() : JxtInterface("",WEB_JXT_IFACE_ID)
  {
     Handler *evHandle;
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
  };

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

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};


} // namespace AstraWeb

//#endif // __cplusplus

#endif // __WEB_MAIN_H__

