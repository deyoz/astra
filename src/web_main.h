#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

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

int internet_main(const char *body, int blen, const char *head,
                  int hlen, char **res, int len);

class WebRequestsIface : public JxtInterface
{
public:
  WebRequestsIface() : JxtInterface("",WEB_JXT_IFACE_ID)
  {
     Handler *evHandle;
     // Расширенный поиск пассажиров
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SearchPNRs);
     AddEvent("SearchPNRs",evHandle);
     // Информация о рейсе
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SearchFlt);
     AddEvent("SearchFlt",evHandle);
     // Загрузка PNR
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::LoadPnr);
     AddEvent("LoadPnr",evHandle);
     // Загрузка компоновки
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ViewCraft);
     AddEvent("ViewCraft",evHandle);
     // Регистрация пассажиров
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::SavePax);
     AddEvent("SavePax",evHandle);
     // Получение пектаба и данных для посадочного талона
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetPrintDataBP);
     AddEvent("GetPrintDataBP",evHandle);
     // Подтверждение печати
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ConfirmPrintBP);
     AddEvent("ConfirmPrintBP",evHandle);
     // Получение тегов для пос. талона
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetBPTags);
     AddEvent("GetBPTags",evHandle);
     // Разметка слоя "Резервирование оплачиваемого места"
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::AddProtPaidLayer);
     AddEvent("AddProtPaidLayer",evHandle);
     // Удаление слоя "Резервирование оплачиваемого места"
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::RemoveProtPaidLayer);
     AddEvent("RemoveProtPaidLayer",evHandle);
     // Сообщение о любой ошибке в Астру (например, после неудачной печати)
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ClientError);
     AddEvent("ClientError",evHandle);
     
     //Система Меридиан
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetFlightInfo);
     AddEvent("GetFlightInfo",evHandle);
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::ParseMessage);
     AddEvent("Message",evHandle);

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

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};


} // namespace AstraWeb

#endif // __WEB_MAIN_H__

