#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_
#include <vector>
#include <string>

#include "JxtInterface.h"
#include "tlg/tlg_parser.h"
#include "astra_consts.h"

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


struct TTypeBSendInfo
{
  std::string tlg_type,airline,airp_dep,airp_arv;
  int flt_no,first_point,point_num;
};

struct TTypeBAddrInfo
{
  std::string tlg_type,airline,airp_dep,airp_arv;
  int flt_no,first_point,point_num;

  std::string airp_trfer,crs;
  bool pr_lat;
};

struct TTlgOutPartInfo
{
  int id,num,point_id;
  std::string tlg_type,addr,heading,body,ending,extra;
  bool pr_lat, pr_tst;
  BASIC::TDateTime time_create,time_send_scd;
  TTlgOutPartInfo ()
  {
    id=-1;
    num=1;
    time_create=ASTRA::NoExists;
    time_send_scd=ASTRA::NoExists;
    pr_tst = true; // telegram in tst mode
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

     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::TestSeatRanges);
     AddEvent("TestSeatRanges",evHandle);
  };

  void GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateTlg2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, int tlg_id);
  void LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestSeatRanges(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static int create_tlg(
          const std::string vtype,
          const int         vpoint_id,
          const std::string vairp_trfer,
          const std::string vcrs,
          const std::string vextra,
          const bool        vpr_lat,
          const std::string vaddrs,
          const int         tst_tlg_id = -1
          );
  void delete_tst_tlg(int tlg_id);

  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void SendTlg( int tlg_id );
  static void SendTlg( int point_id, std::vector<std::string> &tlg_types );

  static bool IsTypeBSend( TTypeBSendInfo &info );
  static std::string GetTypeBAddrs( TTypeBAddrInfo &info );
  static std::string GetTypeBAddrs( std::string tlg_type, bool pr_lat );

  static void SaveTlgOutPart( TTlgOutPartInfo &info );

  //BSM
  static void LoadBSMContent(int grp_id, TBSMContent& con);
  static void CompareBSMContent(TBSMContent& con1, TBSMContent& con2, std::vector<TBSMContent>& bsms);
  static std::string CreateBSMBody(TBSMContent& con, bool pr_lat);
  static bool IsBSMSend( TTypeBSendInfo info, std::map<bool,std::string> &addrs );
  static void SendBSM(int point_dep, int grp_id, TBSMContent &con1, std::map<bool,std::string> &addrs );
};

#endif /*_TELEGRAM_H_*/


