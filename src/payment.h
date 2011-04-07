#ifndef _PAYMENT_H_
#define _PAYMENT_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "basic.h"
#include "oralib.h"
#include "print.h"

class PaymentInterface : public JxtInterface
{
public:
  PaymentInterface() : JxtInterface("","BagPayment")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::LoadPax);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByGrpId",evHandle);
     AddEvent("PaxByRegNo",evHandle);
     AddEvent("PaxByReceiptNo",evHandle);
     AddEvent("PaxByScanData",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::SaveBag);
     AddEvent("SaveBag",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::UpdPrepay);
     AddEvent("UpdPrepay",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::ViewReceipt);
     AddEvent("ViewReceipt",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::ReplaceReceipt);
     AddEvent("ReplaceReceipt",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::PrintReceipt);
     AddEvent("PrintReceipt",evHandle);
     evHandle=JxtHandler<PaymentInterface>::CreateHandler(&PaymentInterface::AnnulReceipt);
     AddEvent("AnnulReceipt",evHandle);
  };
  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadReceipts(int id, bool pr_grp, xmlNodePtr dataNode, xmlNodePtr reqNode);
  void SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  int LockAndUpdTid(int point_dep, int grp_id, int tid);
  double GetCurrNo(int user_id, std::string form_type);

  //из базы в структуру
  bool GetReceipt(int id, TBagReceipt &rcpt);
  //из базы в структуру
  bool GetReceipt(TQuery &Qry, TBagReceipt &rcpt);
  //из XML в структуру
  void GetReceipt(xmlNodePtr reqNode, TBagReceipt &rcpt);

  //из структуры в базу
  int PutReceipt(TBagReceipt &rcpt, int point_id, int grp_id);
  //из структуры в XML
  void PutReceipt(TBagReceipt &rcpt, int rcpt_id, xmlNodePtr resNode);

  //образ из структуры в XML
  void PutReceiptFields(TBagReceipt &rcpt, xmlNodePtr node);
  //образ из базы в XML
  void PutReceiptFields(int id, xmlNodePtr node, xmlNodePtr reqNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};



#endif
