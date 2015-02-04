#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_
#include <vector>
#include <string>
#include <set>
#include <map>

#include "jxtlib/JxtInterface.h"
#include "tlg/tlg_parser.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "baggage.h"
#include "passenger.h"
#include "remarks.h"
#include "typeb_utils.h"
#include "base_tables.h"

const size_t PART_SIZE = 3500;

struct TTlgCompLayer {
	int pax_id;
	int point_dep;
	int point_arv;
	ASTRA::TCompLayerType layer_type;
	std::string xname;
	std::string yname;
};


void getSalonLayers( int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     std::vector<TTlgCompLayer> &complayers,
                     bool pr_blocked );
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
                     const int typeb_tlg_id,
                     const int typeb_tlg_num,
                     const TTlgStatPoint &sender,
                     const TTlgStatPoint &receiver,
                     const BASIC::TDateTime time_create,
                     const std::string &tlg_type,
                     const int tlg_len,
                     const TTripInfo &fltInfo,
                     const std::string &airline_mark,
                     const std::string &extra);

  /*  putTypeBOut()
    sendTypeBOut()
    doneTypeBOut()*/
};

bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxTknItem &tkn, bool inf_indicator, CheckIn::TPaxRemItem &rem);
bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocItem &doc, bool inf_indicator, CheckIn::TPaxRemItem &rem);
bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocoItem &doco, bool inf_indicator, CheckIn::TPaxRemItem &rem);
bool getPaxRem(TypeB::TDetailCreateInfo &info, const CheckIn::TPaxDocaItem &doca, bool inf_indicator, CheckIn::TPaxRemItem &rem);

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
        void apply_comp(TypeB::TDetailCreateInfo &info, bool pr_blocked);
        void dump_comp() const;
        void dump_list(std::map<int, std::string> &list);
        void dump_list(std::map<int, TSeatRectList> &list);
        void get_seat_list(std::map<int, std::string> &list, bool pr_lat);
        int get_list_size(std::map<int, std::string> &list);
    public:
        std::vector<std::string> items;
        bool empty() { return comp.empty(); };
        void get(TypeB::TDetailCreateInfo &info);
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

const std::string FILE_PARAM_FORMAT = "FORMAT";
const std::string FILE_PARAM_POINT_ID = "POINT_ID";
const std::string FILE_PARAM_TLG_TYPE = "TLG_TYPE";
const std::string FILE_PARAM_ORIGIN = "ORIGIN";
const std::string FILE_PARAM_HEADING = "HEADING";
const std::string FILE_PARAM_ENDING = "ENDING";
const std::string FILE_PARAM_PR_LAT = "PR_LAT";
const std::string FILE_PARAM_TIME_CREATE = "TIME_CREATE";
const std::string FILE_PARAM_ORIGINATOR_ID = "ORIGINATOR_ID";
const std::string FILE_PARAM_AIRLINE_MARK = "AIRLINE_MARK";
const std::string FILE_PARAM_MANUAL_CREATION = "MANUAL_CREATION";
const std::string FILE_PARAM_EXTRA = "EXTRA_";
const std::string PARAM_TLG_TYPE = "TLG_TYPE";
const std::string PARAM_FILE_NAME_ENC = "FILE_NAME_ENC";

struct TTlgOutPartInfo
{
  int id,num,point_id;
  std::string tlg_type,addr,origin,heading,body,ending;
  bool pr_lat;
  BASIC::TDateTime time_create,time_send_scd;
  int originator_id;
  std::string airline_mark;
  bool manual_creation;
  std::map<std::string/*lang*/, std::string> extra;
  TTlgOutPartInfo ()
  {
    clear();
  };
  void clear()
  {
    id=ASTRA::NoExists;
    num=1;
    point_id=ASTRA::NoExists;
    tlg_type.clear();
    addr.clear();
    origin.clear();
    heading.clear();
    body.clear();
    ending.clear();
    pr_lat=false;
    time_create=ASTRA::NoExists;
    time_send_scd=ASTRA::NoExists;
    originator_id=ASTRA::NoExists;
    airline_mark.clear();
    extra.clear();
    manual_creation=false;
  };

  size_t textSize() const
  {
    return addr.size()+
           origin.size()+
           heading.size()+
           body.size()+
           ending.size();
  };

  TTlgOutPartInfo (const TypeB::TDetailCreateInfo &info);
  void addToFileParams(std::map<std::string, std::string> &params) const;
  void addFromFileParams(const std::map<std::string, std::string> &params);
  TTlgOutPartInfo& fromDB(TQuery &Qry);
  void getExtra();
};

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
};

class TTlgContent
{
  public:
    TypeB::TIndicator indicator;
    TTrferRouteItem OutFlt;
    TTrferRoute OnwardFlt;
    bool pr_lat_seat;

    std::string OutCls;
    std::vector< std::list<std::string/*class*/> > OnwardCls;

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

struct TBSMAddrs {
    std::vector<TypeB::TCreateInfo> createInfo;    
    bool empty() const { return createInfo.empty(); }
};
bool IsSend( const TAdvTripInfo &fltInfo, TBSMAddrs &addrs );
void Send( int point_dep, int grp_id, const TTlgContent &con1, const TBSMAddrs &addrs );

}; //namespace BSM

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
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::SaveInTlg);
     AddEvent("SaveInTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::LoadTlg);
     AddEvent("LoadTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::SaveTlg);
     AddEvent("SaveTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::SendTlg);
     AddEvent("SendTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::DeleteTlg);
     AddEvent("DeleteTlg",evHandle);
     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::LCI_srv);
     AddEvent("LCI_srv",evHandle);

     evHandle=JxtHandler<TelegramInterface>::CreateHandler(&TelegramInterface::TestSeatRanges);
     AddEvent("TestSeatRanges",evHandle);
  };

  void GetTlgIn2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveInTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LCI_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void TestSeatRanges(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static int create_tlg(const TypeB::TCreateInfo &createInfo,
                        TTypeBTypesRow &tlgTypeInfo,
                        bool manual_creation);

  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void SendTlg( int tlg_id );
  static void SendTlg(const std::vector<TypeB::TCreateInfo> &info);

  static void SaveTlgOutPart( TTlgOutPartInfo &info, bool completed, bool has_errors );
};

void send_tlg_help(const char *name);
int send_tlg(int argc,char **argv);

bool check_delay_code(int delay_code);
bool check_delay_code(const std::string &delay_code);
bool check_delay_value(BASIC::TDateTime delay_time);


#endif /*_TELEGRAM_H_*/


