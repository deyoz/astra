#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_
#include <vector>
#include <string>

#include "JxtInterface.h"
#include "tlg/tlg_parser.h"

class TBSMTagItem
{
  public:
    double no;
    int bag_amount,bag_weight;
    TBSMTagItem()
    {
      no=-1;
      bag_amount=-1;
      bag_weight=-1;
    };
};

class TBSMBagItem
{
  public:
    int rk_weight;
    TBSMBagItem()
    {
      rk_weight=-1;
    };
};

class TBSMPaxItem
{
  public:
    std::string surname,name,seat_no,status,pnr_addr;
    int reg_no;
    TBSMPaxItem()
    {
      reg_no=-1;
    };
};

class TBSMContent
{
  public:
    TIndicator indicator;
    TTransferItem OutFlt;
    std::vector<TTransferItem> OnwardFlt;
    std::vector<TBSMTagItem> tags;
    TBSMPaxItem pax;
    TBSMBagItem bag;
    TBSMContent()
    {
      indicator=None;
    };
};


class TelegramInterface : public JxtInterface
{
public:
  TelegramInterface() : JxtInterface("","Telegram")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::GetTlgIn);
     AddEvent("GetTlgIn",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::GetTlgOut);
     AddEvent("GetTlgOut",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::GetAddrs);
     AddEvent("GetAddrs",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::CreateTlg);
     AddEvent("CreateTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::LoadTlg);
     AddEvent("LoadTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::SaveTlg);
     AddEvent("SaveTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::SendTlg);
     AddEvent("SendTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::DeleteTlg);
     AddEvent("DeleteTlg",evHandle);
  };

  void GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  void LoadBSMContent(int grp_id, TBSMContent& con);
  std::vector<TBSMContent>& CreateBSMContent(TBSMContent& con1, TBSMContent& con2);
  void CreateBSMBody(TBSMContent& con, bool pr_lat);

  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void SendTlg( int tlg_id );
  static void SendTlg( int point_id, std::vector<std::string> &tlg_types );
};

#endif /*_TELEGRAM_H_*/


