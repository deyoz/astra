#ifndef _PAYMENT_H_
#define _PAYMENT_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

class PaymentInterface : public JxtInterface
{
public:
  PaymentInterface() : JxtInterface("","payment")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::LoadPax);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByRegNo",evHandle);
     AddEvent("PaxByReceiptNo",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::SaveBag);
     AddEvent("SaveBag",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::UpdReceipt);
     AddEvent("UpdReceipt",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::UpdPrepay);
     AddEvent("UpdPrepay",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::ViewReceipt);
     AddEvent("ViewReceipt",evHandle);
  };

  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadReceipts(int id, bool pr_grp, xmlNodePtr dataNode);
  void SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UpdReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static int LockAndUpdTid(int point_dep, int grp_id, int tid);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif
