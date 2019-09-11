#ifndef _SERVICEEVAL_H_
#define _SERVICEEVAL_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "SWCExchangeIface.h"

typedef std::function<void(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)> ExchangeResponseHandler;

class ServiceEvalInterface : public SWC::SWCExchangeIface
{
private:
  static const std::string getServiceName() {
    return "service_eval";
  }
protected:
    virtual void BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req);
    virtual void BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId);
public:
  ServiceEvalInterface() : SWC::SWCExchangeIface(getServiceName())
  {
     Handler *evHandle;
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::Evaluation);
     AddEvent("evaluation",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::Filtered);
     AddEvent("filtered",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::Paid);
     AddEvent("paid",evHandle);
     addResponseHandler("check_in_get_pnr", response_check_in_get_pnr);
     addResponseHandler("order",response_order);
     addResponseHandler("svc_emd_issue_query",response_svc_emd_issue_query);
     addResponseHandler("svc_emd_issue_confirm",response_svc_emd_issue_confirm);
     addResponseHandler("svc_emd_void",backPaid);
     addResponseHandler("svc_emd_issue_cancel",backPaid);
  }
  void Evaluation(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Filtered(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Paid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void response_check_in_get_pnr(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_order(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_svc_emd_issue_query(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_svc_emd_issue_confirm(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void continuePaidRequest( xmlNodePtr reqNode, xmlNodePtr resNode);
  static void backPaid(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
  virtual ~ServiceEvalInterface() {}

};



bool isPaymentAtDesk( int point_id );


#endif /*_SERVICEEVAL_H_*/

