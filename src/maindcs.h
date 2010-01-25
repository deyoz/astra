#ifndef _MAINDCS_H_
#define _MAINDCS_H_

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"


class MainDCSInterface : public JxtInterface
{
public:
  MainDCSInterface() : JxtInterface("","MainDCS")
  {
     Handler *evHandle;
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::CheckUserLogon);
     AddEvent("CheckUserLogon",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::UserLogon);
     AddEvent("UserLogon",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::UserLogoff);
     AddEvent("UserLogoff",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::ChangePasswd);
     AddEvent("ChangePasswd",evHandle);

     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::GetDeviceList);
     AddEvent("GetDeviceList",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::GetDeviceInfo);
     AddEvent("GetDeviceInfo",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::DetermineScanParams);
     AddEvent("DetermineScanParams",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::SaveDeskTraces);
     AddEvent("SaveDeskTraces",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::GetCertificates);
     AddEvent("GetCertificates",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::RequestCertificateData);
     AddEvent("RequestCertificateData",evHandle);
     evHandle=JxtHandler<MainDCSInterface>::CreateHandler(&MainDCSInterface::PutRequestCertificate);
     AddEvent("PutRequestCertificate",evHandle);
  };

  void CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  bool GetSessionAirlines(xmlNodePtr node, std::string &str);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  void GetDeviceList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetDeviceInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DetermineScanParams(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveDeskTraces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

void GetDevices(xmlNodePtr reqNode, xmlNodePtr resNode);

#endif /*_MAINDCS_H_*/
