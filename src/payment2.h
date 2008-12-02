#ifndef _PAYMENT2_H_
#define _PAYMENT2_H_

#include <libxml/tree.h>
#include "JxtInterface.h"
#include "basic.h"
#include "oralib.h"
#include "print.h"

class PaymentOldInterface : public JxtInterface
{
public:
  PaymentOldInterface() : JxtInterface("","payment")
  {
     Handler *evHandle;
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::LoadPax);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByRegNo",evHandle);
     AddEvent("PaxByReceiptNo",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::SaveBag);
     AddEvent("SaveBag",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::UpdPrepay);
     AddEvent("UpdPrepay",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::ViewReceipt);
     AddEvent("ViewReceipt",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::ReplaceReceipt);
     AddEvent("ReplaceReceipt",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::PrintReceipt);
     AddEvent("PrintReceipt",evHandle);
     evHandle=JxtHandler<PaymentOldInterface>::CreateHandler(&PaymentOldInterface::AnnulReceipt);
     AddEvent("AnnulReceipt",evHandle);
  };
  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadReceipts(int id, bool pr_grp, xmlNodePtr dataNode);
  void SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  int LockAndUpdTid(int point_dep, int grp_id, int tid);
  double GetCurrNo(int user_id, std::string form_type);

  //�� ���� � ��������
  bool GetReceipt(int id, TBagReceipt &rcpt);
  //�� ���� � ��������
  bool GetReceipt(TQuery &Qry, TBagReceipt &rcpt);
  //�� XML � ��������
  void GetReceipt(xmlNodePtr reqNode, TBagReceipt &rcpt);

  //�� �������� � ����
  int PutReceipt(TBagReceipt &rcpt, int point_id, int grp_id);
  //�� �������� � XML
  void PutReceipt(TBagReceipt &rcpt, int rcpt_id, xmlNodePtr resNode);

  //��ࠧ �� �������� � XML
  void PutReceiptFields(TBagReceipt &rcpt, PrintDataParser &parser, xmlNodePtr node);
  //��ࠧ �� ���� � XML
  void PutReceiptFields(int id, xmlNodePtr node);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};



#endif
