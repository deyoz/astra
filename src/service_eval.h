#ifndef _SERVICEEVAL_H_
#define _SERVICEEVAL_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "SWCExchangeIface.h"
#include "rfisc_price.h" //!!!

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
     //evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::exchange);
     //AddEvent("exchange",evHandle);
     //evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::BeforePaid);
     //AddEvent("before_paid",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::Paid);
     AddEvent("paid",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::CheckPaid);
     AddEvent("check_paid",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::CancelPaid);
     AddEvent("cancel_paid",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::PosAssign);
     AddEvent("pos_assign",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::PosSet);
     AddEvent("pos_set",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::PosRelease);
     AddEvent("pos_release",evHandle);
//     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::AfterPaid);
//     AddEvent("after_paid",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::PayDocParamsRequest);
     AddEvent("PayDocParamsRequest",evHandle);
     evHandle=JxtHandler<ServiceEvalInterface>::CreateHandler(&ServiceEvalInterface::PayDocParamsAnswer);
     AddEvent("PayDocParamsAnswer",evHandle);
     addResponseHandler("check_in_get_pnr", response_check_in_get_pnr);
     addResponseHandler("order",response_order);
     addResponseHandler("svc_emd_issue_query",response_svc_emd_issue_query);
     addResponseHandler("svc_emd_issue_confirm",response_svc_emd_issue_confirm);
     addResponseHandler("svc_emd_void",backPaid);
     addResponseHandler("svc_emd_issue_cancel",backPaid);
  }
  static void RequestFromGrpId(xmlNodePtr reqNode, int point_id, SWC::SWCExchange& req, int client_id = ASTRA::NoExists);
  void Evaluation(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Filtered(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  //void exchange(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  //void BeforePaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  //void AfterPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Paid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CheckPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CancelPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PosAssign(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PosSet(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PosRelease(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PayDocParamsRequest(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PayDocParamsAnswer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void response_check_in_get_pnr(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_order(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_svc_emd_issue_query(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_svc_emd_issue_confirm(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void continuePaidRequest( xmlNodePtr reqNode, xmlNodePtr resNode);
  static void backPaid(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void AfterPaid(xmlNodePtr reqNode, xmlNodePtr resNode);
  static void SendOrderReq( xmlNodePtr reqNode, int grp_id, const SvcEmdRegnum& regnum,
                            const std::string& surname, int client_id = ASTRA::NoExists );
  //static void MPSNotify( int grp_id, const TPriceRFISCList& prices, xmlNodePtr reqNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
  virtual ~ServiceEvalInterface() {}

};

int getSWCClientFromPOS( int pos_id );

bool isPaymentAtDesk( int point_id );
bool isPaymentAtDesk( int point_id, int &method_type );

#endif /*_SERVICEEVAL_H_*/

