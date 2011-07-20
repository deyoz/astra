#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_
#include <vector>
#include <string>
#include <set>

#include "jxtlib/JxtInterface.h"
#include "tlg/tlg_parser.h"
#include "astra_consts.h"
#include "astra_misc.h"


struct TTlgCompLayer {
	int pax_id;
	int point_dep;
	int point_arv;
	ASTRA::TCompLayerType layer_type;
	std::string xname;
	std::string yname;
};

struct TCodeShareInfo {
    std::string airline;
    int flt_no;
    std::string suffix;
    bool pr_mark_header;
    void init(xmlNodePtr node);
    void dump() const;
    bool IsNULL() const;
    bool operator == (const TMktFlight &s) const;
    TCodeShareInfo():
        flt_no(ASTRA::NoExists),
        pr_mark_header(false)
    {};
};

struct TCreateTlgInfo {
    std::string type;
    int         point_id;
    std::string airp_trfer;
    std::string crs;
    std::string extra;
    bool        pr_lat;
    std::string addrs;
    TCodeShareInfo mark_info;
    bool pr_alarm;
};

struct TTlgDraftPart {
    std::string addr, heading, ending, body;
};

struct TErrLst:std::map<int, std::string> {
    void dump();
    void fix(std::vector<TTlgDraftPart> &parts);
    void fetch_err(std::set<int> &txt_errs, std::string body);
};

struct TTlgInfo {
    // ���ଠ�� � �������᪮� ३�
    /*const TCodeShareInfo &mark_info;*/
    TCodeShareInfo mark_info;
    // ����஢�� ᠫ���
    bool pr_lat_seat;
    std::string tlg_type;
    //���� �����⥫��
    std::string addrs;
    //���� ��ࠢ�⥫�
    std::string sender;
    //३�
    int point_id;
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp_dep;
    std::string airp_arv;
    BASIC::TDateTime scd_utc;
    BASIC::TDateTime scd_local;
    BASIC::TDateTime act_local;
    int scd_local_day;
    bool pr_summer;
    std::string bort;
    //�ᯮ����⥫�� �⮡� ���᪨���� �������
    int first_point;
    int point_num;
    bool pr_tranzit;
    //��� �뢮�� � ⥫��ࠬ��
    std::string airline_view;
    std::string suffix_view;
    std::string airp_dep_view;
    std::string airp_arv_view;
    //業�� �஭�஢����
    std::string crs;
    //�������⥫쭠� ���
    std::string extra;
    //ࠧ�� ����ன��
    bool pr_lat;
    bool vcompleted;
    TElemFmt elem_fmt;
    std::string lang;
    // ᯨ᮪ �訡�� ⥫��ࠬ��
    TErrLst err_lst;
    std::string add_err(std::string err, std::string val);
    std::string add_err(std::string err, const char *format, ...);

    std::string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt = efmtUnknown);
    std::string TlgElemIdToElem(TElemType type, std::string id, TElemFmt fmt = efmtUnknown);
    bool operator == (const TMktFlight &s) const;
    /*TTlgInfo(const TCodeShareInfo &aCodeShareInfo): mark_info(aCodeShareInfo) {
    	TTlgInfo();
    };*/
    TTlgInfo(){
        point_id = -1;
        flt_no = -1;
        scd_utc = 0;
        scd_local = 0;
        act_local = 0;
        scd_local_day = 0;
        pr_summer = false;
        first_point = -1;
        point_num = -1;
        pr_lat = false;
        vcompleted = false;
    }
};

// stuff used to form seat ranges in tlgs
struct TTlgPlace {
    int x, y, num, point_arv;
    std::string xname, yname;
    void dump();
    TTlgPlace() {
        num = ASTRA::NoExists;
        x = ASTRA::NoExists;
        y = ASTRA::NoExists;
        point_arv = ASTRA::NoExists;
    }
};

typedef std::map<std::string, TTlgPlace> t_tlg_row;
typedef std::map<std::string, t_tlg_row> t_tlg_comp;

struct TSeatRect {
    bool del;
    std::string row1, row2, line1, line2;
    std::string str();
    TSeatRect(): del(false) {};
};

struct TSeatRectList: std::vector<TSeatRect> {
    std::string ToTlg();
    void pack();
    void vert_pack();
};

struct TTlgSeatList {
    private:
        t_tlg_comp comp;
        void apply_comp(TTlgInfo &info, bool pr_blocked);
        void dump_comp() const;
        void dump_list(std::map<int, std::string> &list);
        void dump_list(std::map<int, TSeatRectList> &list);
        void get_seat_list(std::map<int, std::string> &list, bool pr_lat);
        int get_list_size(std::map<int, std::string> &list);
    public:
        std::vector<std::string> items;
        void get(TTlgInfo &info);
        void add_seat(int point_id, std::string xname, std::string yname); // used in SOM
        void add_seat(std::string xname, std::string yname) { // used in PRL
            add_seat(0, xname, yname);
        };
        void add_seats(int pax_id, std::vector<TTlgCompLayer> &complayers);
        std::string get_seat_list(bool pr_lat); // used in PRL
        std::vector<std::string>  get_seat_vector(bool pr_lat) const;
        std::string get_seat_one(bool pr_lat);
        void Clear() { comp.clear(); };
};

// End of previous stuff

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
    std::string surname,name,status,pnr_addr;
    TTlgSeatList seat_no;
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
    bool pr_lat_seat;
    std::vector<TTransferItem> OnwardFlt;
    std::vector<TBSMTagItem> tags;
    TBSMPaxItem pax;
    TBSMBagItem bag;
    TBSMContent()
    {
      indicator=None;
      pr_lat_seat=false;
    };
};


struct TTypeBSendInfo
{
  std::string tlg_type,airline,airp_dep,airp_arv;
  int flt_no,point_id,first_point,point_num;
  bool pr_tranzit;
  TTypeBSendInfo() {};
  TTypeBSendInfo(const TTripInfo &info)
  {
    airline=info.airline;
    flt_no=info.flt_no;
    airp_dep=info.airp;
  };
};

struct TTypeBAddrInfo
{
  std::string tlg_type,airline,airp_dep,airp_arv;
  int flt_no,point_id,first_point,point_num;
  bool pr_tranzit;
  std::string airp_trfer,crs;
  bool pr_lat;
  TCodeShareInfo mark_info;
  TTypeBAddrInfo() {};
  TTypeBAddrInfo(const TTypeBSendInfo &info)
  {
    tlg_type=info.tlg_type;
    airline=info.airline;
    airp_dep=info.airp_dep;
    airp_arv=info.airp_arv;
    flt_no=info.flt_no;
    point_id=info.point_id;
    first_point=info.first_point;
    point_num=info.point_num;
    pr_tranzit=info.pr_tranzit;
  };
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
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::GetTlgIn2);
     AddEvent("GetTlgIn2",evHandle);
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

  void GetTlgIn2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestSeatRanges(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static int create_tlg(
          const             TCreateTlgInfo &createInfo
          );

  static std::string GetTlgLogMsg(const TCreateTlgInfo &createInfo);

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

std::string fetch_addr(std::string &addr, TTlgInfo *info = NULL);
std::string format_addr_line(std::string vaddrs, TTlgInfo *info = NULL);

void ReadSalons( TTlgInfo &info, std::vector<TTlgCompLayer> &complayers, bool pr_blocked = false );

int test_prl(int argc,char **argv);

#endif /*_TELEGRAM_H_*/


