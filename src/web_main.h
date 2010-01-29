#ifndef __WEB_MAIN_H__
#define __WEB_MAIN_H__

//#ifdef __cpluplus
#include "jxtlib/JxtInterface.h"

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
  WebRequestsIface() : JxtInterface("","WEBC")
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
     // ����祭�� ������ ��� ���. ⠫���
     evHandle=JxtHandler<WebRequestsIface>::CreateHandler(&WebRequestsIface::GetBPTags);
     AddEvent("GetBPTags",evHandle);
  };

  void SearchFlt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPnr(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewCraft(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBPTags(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};


} // namespace AstraWeb

//#endif // __cplusplus

#endif // __WEB_MAIN_H__

