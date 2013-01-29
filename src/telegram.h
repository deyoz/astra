#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_
#include <vector>
#include <string>
#include <set>

#include "jxtlib/JxtInterface.h"
#include "tlg/tlg_parser.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "baggage.h"
#include "passenger.h"
#include "remarks.h"

const size_t PART_SIZE = 3500;

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

struct TOriginatorInfo
{
  int id;
  std::string addr;
  TOriginatorInfo():id(ASTRA::NoExists) {};
};

struct TTlgInfo {
    // Информация о коммерческом рейсе
    /*const TCodeShareInfo &mark_info;*/
    TCodeShareInfo mark_info;
    // кодировка салона
    bool pr_lat_seat;
    std::string tlg_type;
    //адреса получателей
    std::string addrs;
    //адрес отправителя
    TOriginatorInfo originator;
    //время создания
    BASIC::TDateTime time_create;
    //рейс
    int point_id;
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp_dep;
    std::string airp_arv;
    BASIC::TDateTime scd_utc;
    BASIC::TDateTime est_utc;
    BASIC::TDateTime scd_local;
    BASIC::TDateTime act_local;
    int scd_local_day;
    std::string bort;
    //вспомогательные чтобы вытаскивать маршрут
    int first_point;
    int point_num;
    bool pr_tranzit;
    //центр бронирования
    std::string crs;
    //дополнительная инфа
    std::string extra;
    //разные настройки
    bool pr_lat;
    bool vcompleted;
    TElemFmt elem_fmt;
    std::string lang;
    // список ошибок телеграммы
    TErrLst err_lst;
    std::string add_err(std::string err, std::string val);
    std::string add_err(std::string err, const char *format, ...);

    std::string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt = efmtUnknown);
    std::string TlgElemIdToElem(TElemType type, std::string id, TElemFmt fmt = efmtUnknown);
    bool operator == (const TMktFlight &s) const;

    std::string airline_view(bool always_operating = true);
    int flt_no_view(bool always_operating = true);
    std::string suffix_view(bool always_operating = true);
    std::string airp_dep_view();
    std::string airp_arv_view();
    std::string flight_view(bool always_operating = true);

    TTlgInfo(){
        time_create = ASTRA::NoExists;
        point_id = -1;
        flt_no = -1;
        scd_utc = 0;
        est_utc = 0;
        scd_local = 0;
        act_local = 0;
        scd_local_day = 0;
        first_point = ASTRA::NoExists;
        point_num = -1;
        pr_lat = false;
        vcompleted = false;
    }
};

struct TTlgStatPoint
{
  std::string sita_addr;
  std::string canon_name;
  std::string descr;
  std::string country;
  TTlgStatPoint(std::string v_sita_addr,
                std::string v_canon_name,
                std::string v_descr,
                std::string v_country):sita_addr(v_sita_addr),
                                       canon_name(v_canon_name),
                                       descr(v_descr),
                                       country(v_country) {};
};

class TTlgStat
{
  public:
    void putTypeBOut(const int queue_tlg_id,
                     const int tlg_id,
                     const int tlg_num,
                     const TTlgStatPoint &sender,
                     const TTlgStatPoint &receiver,
                     const BASIC::TDateTime time_create,
                     const std::string &tlg_type,
                     const int tlg_len,
                     const TTripInfo &fltInfo,
                     const std::string &extra);

  /*  putTypeBOut()
    sendTypeBOut()
    doneTypeBOut()*/
};

bool getPaxRem(TTlgInfo &info, const CheckIn::TPaxTknItem &tkn, CheckIn::TPaxRemItem &rem);
bool getPaxRem(TTlgInfo &info, const CheckIn::TPaxDocItem &doc, CheckIn::TPaxRemItem &rem);
bool getPaxRem(TTlgInfo &info, const CheckIn::TPaxDocoItem &doco, CheckIn::TPaxRemItem &rem);

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
        std::string get_seat_one(bool pr_lat) const;
        void Clear() { comp.clear(); };
};

// End of previous stuff

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
  bool pr_lat;
  BASIC::TDateTime time_create,time_send_scd;
  int originator_id;
  TTlgOutPartInfo ()
  {
    id=-1;
    num=1;
    time_create=ASTRA::NoExists;
    time_send_scd=ASTRA::NoExists;
    originator_id=ASTRA::NoExists;
  };
  TTlgOutPartInfo (const TTlgInfo &info)
  {
    id=-1;
    num = 1;
    tlg_type = info.tlg_type;
    point_id = info.point_id;
    pr_lat = info.pr_lat;
    extra = info.extra;
    addr = info.addrs;
    time_create = info.time_create;
    time_send_scd = ASTRA::NoExists;
    originator_id = info.originator.id;
  };
};

std::string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt, std::string lang);
std::string TlgElemIdToElem(TElemType type, std::string id, TElemFmt fmt, std::string lang);
TOriginatorInfo getOriginator(const std::string &airline,
                              const std::string &airp_dep,
                              const std::string &tlg_type,
                              const BASIC::TDateTime &time_create,
                              bool with_exception);

namespace BSM
{

std::string TlgElemIdToElem(TElemType type, int id, bool pr_lat);
std::string TlgElemIdToElem(TElemType type, std::string id, bool pr_lat);

class TPaxItem
{
  public:
    std::string surname,name,status,pnr_addr;
    TTlgSeatList seat_no;
    int reg_no;
    int bag_amount, bag_weight, rk_weight;
    int bag_pool_num;
    TPaxItem()
    {
      reg_no=ASTRA::NoExists;
      bag_amount=0;
      bag_weight=0;
      rk_weight=0;
      bag_pool_num=ASTRA::NoExists;
    };
    bool operator < (const TPaxItem &item) const
    {
      return reg_no < item.reg_no;
    };
};

class TTlgContent
{
  public:
    TypeB::TIndicator indicator;
    TTrferRouteItem OutFlt;
    TTrferRoute OnwardFlt;
    bool pr_lat_seat;

    std::map<double, CheckIn::TTagItem> tags;
    std::map<int/*bag_num*/, CheckIn::TBagItem> bags;
    std::map<int/*bag_pool_num*/, TPaxItem> pax;
    TTlgContent()
    {
      indicator=TypeB::None;
      pr_lat_seat=false;
    };
    bool addTag(double no, const TTlgContent& src);
    bool addTag(const CheckIn::TTagItem &tag);
    bool addBag(const CheckIn::TBagItem &bag);
};

void LoadContent(int grp_id, TTlgContent& con);
void CompareContent(const TTlgContent& con1, const TTlgContent& con2, std::vector<TTlgContent>& bsms);
std::string CreateTlgBody(const TTlgContent& con, bool pr_lat);
struct TBSMAddrs {
    std::map<bool,std::string> addrs;
    std::map<std::string, std::string> HTTPGETparams;
    bool empty() const { return addrs.empty() and HTTPGETparams.empty(); }
};
bool IsSend( TTypeBSendInfo info, TBSMAddrs &addrs );
void Send( int point_dep, int grp_id, const TTlgContent &con1, const TBSMAddrs &addrs );

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
};

std::string fetch_addr(std::string &addr, TTlgInfo *info = NULL);
std::string format_addr_line(std::string vaddrs, TTlgInfo *info = NULL);

void ReadSalons( TTlgInfo &info, std::vector<TTlgCompLayer> &complayers, bool pr_blocked = false );

void send_tlg_help(const char *name);
int send_tlg(int argc,char **argv);

bool check_delay_code(int delay_code);
bool check_delay_code(const std::string &delay_code);
bool check_delay_value(BASIC::TDateTime delay_time);

#endif /*_TELEGRAM_H_*/


