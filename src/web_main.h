#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

#include "jxtlib/JxtInterface.h"
#include <string>
#include "passenger.h"
#include "typeb_utils.h"
#include "tlg/tlg_parser.h"

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
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetPaxsInfo);
     AddEvent("GetPaxsInfo",evHandle);
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

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

/*
1. Если кто-то уже начал работать с pnr (агент,разборщик PNL)
2. Если пассажир зарегистрировался, а разборщик PNL ставит признак удаления
*/

struct TWebPax {
  int pax_no; //виртуальный ид. для связи одного и того же пассажира разных сегментов
	int crs_pax_id;
	int crs_pax_id_parent;
  int reg_no;
	std::string surname;
	std::string name;
	std::string pers_type_extended; //может содержать БГ (CBBG)
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
  void SyncNewCHKD(int point_id_spp, const std::string& task_name);
  void SyncAllCHKD(int point_id_spp, const std::string& task_name);
} // namespace TypeB

#endif // __WEB_MAIN_H__

