#ifndef _PAYMENT_H_
#define _PAYMENT_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "basic.h"
#include "oralib.h"
#include "astra_misc.h"

struct TBagPayType
{
  std::string pay_type;
  double pay_rate_sum;
  std::string extra;
};

struct TBagReceiptKitItem
{
  std::string form_type;
  double no;
  std::string aircode;
};

struct TBagReceipt
{
    private:
        std::vector<std::string> f_issue_place_idx;
        std::string service_name, service_name_lat;
    public:
        std::string form_type;
        double no;
        std::string pax_name,pax_doc;
        int service_type,bag_type;
        std::string bag_name;
        std::string tickets,prev_no;
        std::string airline,aircode,airp_dep,airp_arv,suffix;
        int flt_no;
        int ex_amount,ex_weight;
        double value_tax,rate,exch_pay_rate;
        int exch_rate;
        std::string rate_cur,pay_rate_cur;
        std::vector<TBagPayType> pay_types;
        std::string remarks;
        BASIC::TDateTime issue_date,annul_date;
        std::string issue_desk,annul_desk,issue_place;
        
        bool is_inter;
        std::string desk_lang;

        int kit_id, kit_num;
        std::vector<TBagReceiptKitItem> kit_items;


        bool pay_bt() { return service_type == 2 && bag_type != -1; };
        bool pr_other()
        {
            return not (
                    bag_type == 20 or
                    bag_type == 21 && form_type != "Z61" or
                    bag_type == 4 or
                    (bag_type == 1 || bag_type == 2)
                    );
        }
        double pay_rate();
        double rate_sum();
        double pay_rate_sum();
        std::string get_fmt_rate(int fmt, bool pr_inter);
        bool pr_exchange()
        {
            return
                (pay_rate_cur == "���" ||
                 pay_rate_cur == "���" ||
                 pay_rate_cur == "���") &&
                pay_rate_cur != rate_cur;
        };
        std::string issue_place_idx(int idx);
        std::string get_service_name(bool is_inter);
};

#define CASH_PAY_TYPE_ID "���"
#define NONE_PAY_TYPE_ID "���"

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
  void BuildTransfer(const TTrferRoute &trfer, xmlNodePtr transferNode);
  void LoadReceipts(int id, bool pr_grp, bool pr_lat, xmlNodePtr dataNode);
  void SaveBag(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void UpdPrepay(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void ViewReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReplaceReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PrintReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void AnnulReceipt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  int LockAndUpdTid(int point_dep, int grp_id, int tid);
  double GetCurrNo(int user_id, const std::string &form_type);
  double GetNextNo(const std::string &form_type, double no);

  //�� ���� � ��������
  bool GetReceiptFromDB(int id, TBagReceipt &rcpt);
  //�� ���� � ��������
  bool GetReceiptFromDB(TQuery &Qry, TBagReceipt &rcpt);
  //�� XML � ��������
  void GetReceiptFromXML(xmlNodePtr reqNode, TBagReceipt &rcpt);

  //�� �������� � ����
  int PutReceiptToDB(const TBagReceipt &rcpt, int point_id, int grp_id);
  //�� �������� � XML
  void PutReceiptToXML(const TBagReceipt &rcpt, int rcpt_id, bool pr_lat, xmlNodePtr rcptNode);

  //��ࠧ �� �������� � XML
  void PutReceiptFields(const TBagReceipt &rcpt, bool pr_lat, xmlNodePtr node);
  //��ࠧ �� ���� � XML
  void PutReceiptFields(int id, bool pr_lat, xmlNodePtr node);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};



#endif
