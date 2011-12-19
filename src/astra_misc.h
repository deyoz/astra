#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include <string>
#include "basic.h"
#include "astra_consts.h"
#include "oralib.h"
#include "astra_utils.h"
#include "astra_elems.h"
#include "astra_locale.h"
#include "stages.h"
#include "xml_unit.h"

struct TMktFlight {
  private:
    void get(TQuery &Qry, int id);
  public:
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string subcls;
    int scd_day_local;
    BASIC::TDateTime scd_date_local;
    std::string airp_dep;
    std::string airp_arv;

    void getByPaxId(int pax_id);
    void getByCrsPaxId(int pax_id);
    void getByPnrId(int pnr_id);
    bool IsNULL();
    void clear();
    void dump();
    TMktFlight():
        flt_no(ASTRA::NoExists),
        scd_day_local(ASTRA::NoExists),
        scd_date_local(ASTRA::NoExists)
    {
    };
};

class TTripInfo
{
  public:
    std::string airline,suffix,airp;
    int flt_no, pr_del;
    TElemFmt airline_fmt, suffix_fmt, airp_fmt;
    BASIC::TDateTime scd_out,real_out;
    TTripInfo()
    {
      Clear();
    };
    TTripInfo( TQuery &Qry )
    {
      Init(Qry);
    };
    void Clear()
    {
      airline.clear();
      flt_no=0;
      suffix.clear();
      airp.clear();
      scd_out=ASTRA::NoExists;
      real_out=ASTRA::NoExists;
      pr_del = ASTRA::NoExists;
      airline_fmt = efmtUnknown;
      suffix_fmt = efmtUnknown;
      airp_fmt = efmtUnknown;
    };
    void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("airline");
      flt_no=Qry.FieldAsInteger("flt_no");
      suffix=Qry.FieldAsString("suffix");
      airp=Qry.FieldAsString("airp");
      scd_out=Qry.FieldAsDateTime("scd_out");
      if (Qry.GetFieldIndex("real_out")>=0)
        real_out = Qry.FieldAsDateTime("real_out");
      else
        real_out = ASTRA::NoExists;
      if (Qry.GetFieldIndex("pr_del")>=0)
        pr_del = Qry.FieldAsInteger("pr_del");
      else
        pr_del = ASTRA::NoExists;
      if (Qry.GetFieldIndex("airline_fmt")>=0)
          airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
      if (Qry.GetFieldIndex("suffix_fmt")>=0)
          suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
      if (Qry.GetFieldIndex("airp_fmt")>=0)
          airp_fmt = (TElemFmt)Qry.FieldAsInteger("airp_fmt");
    };
    void Assign(const TTripInfo &info)
    {
      airline=info.airline;
      suffix=info.suffix;
      airp=info.airp;
      flt_no=info.flt_no;
      pr_del=info.pr_del;
      airline_fmt=info.airline_fmt;
      suffix_fmt=info.suffix_fmt;
      airp_fmt=info.airp_fmt;
      scd_out=info.scd_out;
      real_out=info.real_out;
    };
    void get_client_dates(BASIC::TDateTime &scd_out_client, BASIC::TDateTime &real_out_client, bool trunc_time=true) const;
};

std::string GetTripDate( const TTripInfo &info, const std::string &separator, const bool advanced_trip_list  );
std::string GetTripName( const TTripInfo &info, TElemContext ctxt, bool showAirp=false, bool prList=false );

class TLastTrferInfo
{
  public:
    std::string airline, suffix, airp_arv;
    int flt_no;
    TLastTrferInfo()
    {
      Clear();
    };
    TLastTrferInfo( TQuery &Qry )
    {
      Init(Qry);
    };
    void Clear()
    {
      airline.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
      airp_arv.clear();
    };
    virtual void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("trfer_airline");
      if (!Qry.FieldIsNULL("trfer_flt_no"))
        flt_no=Qry.FieldAsInteger("trfer_flt_no");
      else
        flt_no=ASTRA::NoExists;
      suffix=Qry.FieldAsString("trfer_suffix");
      airp_arv=Qry.FieldAsString("trfer_airp_arv");
    };
    bool IsNULL()
    {
      return (airline.empty() &&
              flt_no==ASTRA::NoExists &&
              suffix.empty() &&
              airp_arv.empty());
    };
    std::string str();
    virtual ~TLastTrferInfo() {};
};

class TLastTCkinSegInfo : public TLastTrferInfo
{
  public:
    TLastTCkinSegInfo():TLastTrferInfo() {};
    TLastTCkinSegInfo( TQuery &Qry ):TLastTrferInfo()
    {
      Init(Qry);
    };
    virtual void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("tckin_seg_airline");
      if (!Qry.FieldIsNULL("tckin_seg_flt_no"))
        flt_no=Qry.FieldAsInteger("tckin_seg_flt_no");
      else
        flt_no=ASTRA::NoExists;
      suffix=Qry.FieldAsString("tckin_seg_suffix");
      airp_arv=Qry.FieldAsString("tckin_seg_airp_arv");
    };
};

//настройки рейса
enum TTripSetType { tsCraftInitVIP=1,
                    tsOutboardTrfer=10,
                    tsETLOnly=11,
                    tsIgnoreTrferSet=12,
                    tsMixedNorms=13,
                    tsNoTicketCheck=15,
                    tsCharterSearch=16,
                    tsCraftNoChangeSections=17 };
                    
const long int DOC_TYPE_FIELD=0x0001;
const long int DOC_ISSUE_COUNTRY_FIELD=0x0002;
const long int DOC_NO_FIELD=0x0004;
const long int DOC_NATIONALITY_FIELD=0x0008;
const long int DOC_BIRTH_DATE_FIELD=0x0010;
const long int DOC_GENDER_FIELD=0x0020;
const long int DOC_EXPIRY_DATE_FIELD=0x0040;
const long int DOC_SURNAME_FIELD=0x0080;
const long int DOC_FIRST_NAME_FIELD=0x0100;
const long int DOC_SECOND_NAME_FIELD=0x0200;

const long int DOCO_BIRTH_PLACE_FIELD=0x0001;
const long int DOCO_TYPE_FIELD=0x0002;
const long int DOCO_NO_FIELD=0x0004;
const long int DOCO_ISSUE_PLACE_FIELD=0x0008;
const long int DOCO_ISSUE_DATE_FIELD=0x0010;
const long int DOCO_APPLIC_COUNTRY_FIELD=0x0020;

const long int TKN_TICKET_NO_FIELD=0x0001;
                                  
                    
class TCheckDocTknInfo
{
  public:
    bool is_inter;
    long int required_fields; //битовая маска
    long int readonly_fields; //битовая маска
    TCheckDocTknInfo()
    {
      is_inter=false;
      required_fields=0x0000;
      readonly_fields=0x0000;
    };
    void ToXML(xmlNodePtr node)
    {
      if (node==NULL) return;
      NewTextChild(node, "is_inter", (int)is_inter, (int)false);
      NewTextChild(node, "required_fields", required_fields, 0x0000);
      NewTextChild(node, "readonly_fields", readonly_fields, 0x0000);
    };
};

class TCheckDocInfo: public std::pair<TCheckDocTknInfo, TCheckDocTknInfo> {};
                    
bool GetTripSets( const TTripSetType setType, const TTripInfo &info );

TCheckDocInfo GetCheckDocInfo(const int point_dep, const std::string& airp_arv);
TCheckDocTknInfo GetCheckTknInfo(const int point_dep);

class TPnrAddrItem
{
  public:
    char airline[4];
    char addr[21];
    TPnrAddrItem()
    {
      *airline=0;
      *addr=0;
    };
};

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs);
std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline);
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs);
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string &airline);

//процедура перевода отдельного дня (без месяца и года) в полноценный TDateTime
//ищет ближайшую или совпадающую дату по отношению к base_date
//параметр back - направление поиска (true - в прошлое от base_date, false - в будущее)
BASIC::TDateTime DayToDate(int day, BASIC::TDateTime base_date, bool back);

struct TTripRouteItem
{
  int point_id;
  int point_num;
  std::string airp;
  bool pr_cancel;
  TTripRouteItem()
  {
    Clear();
  };
  void Clear()
  {
    point_id = ASTRA::NoExists;
    point_num = ASTRA::NoExists;
    airp.clear();
    pr_cancel = true;
  }
};

//несколько общих моментов для пользования функций работы с маршрутом:
//point_id = points.point_id
//first_point = points.first_point
//point_num = points.point_num
//pr_tranzit = points.pr_tranzit
//TTripRouteType1 = включать в маршрут пункт с point_id
//TTripRouteType2 = включать в маршрут отмененные пункты

//функции возвращают false, если в таблице points не найден point_id

enum TTripRouteType1 { trtNotCurrent,
                       trtWithCurrent };
enum TTripRouteType2 { trtNotCancelled,
                       trtWithCancelled };

class TTripRoute : public std::vector<TTripRouteItem>
{
  private:
    void GetRoute(BASIC::TDateTime part_key,
                  int point_id,
                  int point_num,
                  int first_point,
                  bool pr_tranzit,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2,
                  TQuery& Qry);
    bool GetRoute(BASIC::TDateTime part_key,
                  int point_id,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2);

  public:
    //маршрут после пункта point_id
    bool GetRouteAfter(BASIC::TDateTime part_key,
                       int point_id,
                       TTripRouteType1 route_type1,
                       TTripRouteType2 route_type2);
    void GetRouteAfter(BASIC::TDateTime part_key,
                       int point_id,
                       int point_num,
                       int first_point,
                       bool pr_tranzit,
                       TTripRouteType1 route_type1,
                       TTripRouteType2 route_type2);
    //маршрут до пункта point_id
    bool GetRouteBefore(BASIC::TDateTime part_key,
                        int point_id,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2);
    void GetRouteBefore(BASIC::TDateTime part_key,
                        int point_id,
                        int point_num,
                        int first_point,
                        bool pr_tranzit,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2);

    //возвращает следующий пункт маршрута
    void GetNextAirp(BASIC::TDateTime part_key,
                     int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);
    bool GetNextAirp(BASIC::TDateTime part_key,
                     int point_id,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);

    //возвращает предыдущий пункт маршрута
    void GetPriorAirp(BASIC::TDateTime part_key,
                      int point_id,
                      int point_num,
                      int first_point,
                      bool pr_tranzit,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);
    bool GetPriorAirp(BASIC::TDateTime part_key,
                      int point_id,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);
};

class TPaxSeats {
	private:
	  int pr_lat_seat;
	  TQuery *Qry;
	public:
		TPaxSeats( int point_id );
		std::string getSeats( int pax_id, const std::string format );
    ~TPaxSeats();
};

struct TTrferRouteItem
{
  TTripInfo operFlt;
  std::string airp_arv;
  TElemFmt airp_arv_fmt;
  TTrferRouteItem()
  {
    Clear();
  };
  void Clear()
  {
    operFlt.Clear();
    airp_arv.clear();
    airp_arv_fmt=efmtUnknown;
  };
};

enum TTrferRouteType { trtNotFirstSeg,
                       trtWithFirstSeg };
                       
class TTrferRoute : public std::vector<TTrferRouteItem>
{
  public:
    bool GetRoute(int grp_id,
                  TTrferRouteType route_type);
};

struct TCkinRouteItem
{
  int grp_id;
  int point_dep, point_arv;
  std::string airp_dep, airp_arv;
  int seg_no;
  TTripInfo operFlt;
  TCkinRouteItem()
  {
    Clear();
  };
  void Clear()
  {
    grp_id = ASTRA::NoExists;
    point_dep = ASTRA::NoExists;
    point_arv = ASTRA::NoExists;
    seg_no = ASTRA::NoExists;
    operFlt.Clear();
  };
};

enum TCkinRouteType1 { crtNotCurrent,
                       crtWithCurrent };
enum TCkinRouteType2 { crtOnlyDependent,
                       crtIgnoreDependent };
                          
class TCkinRoute : public std::vector<TCkinRouteItem>
{
  private:
    void GetRoute(int tckin_id,
                  int seg_no,
                  bool after_current,
                  TCkinRouteType1 route_type1,
                  TCkinRouteType2 route_type2,
                  TQuery& Qry);
    bool GetRoute(int grp_id,
                  bool after_current,
                  TCkinRouteType1 route_type1,
                  TCkinRouteType2 route_type2);

  public:
    //сквозной маршрут после стыковки grp_id
    bool GetRouteAfter(int grp_id,
                       TCkinRouteType1 route_type1,
                       TCkinRouteType2 route_type2);   //результат=false только если для grp_id не производилась сквозная регистрация!
    void GetRouteAfter(int tckin_id,
                       int seg_no,
                       TCkinRouteType1 route_type1,
                       TCkinRouteType2 route_type2);
    //сквозной маршрут до стыковки grp_id
    bool GetRouteBefore(int grp_id,
                        TCkinRouteType1 route_type1,
                        TCkinRouteType2 route_type2);  //результат=false только если для grp_id не производилась сквозная регистрация!
    void GetRouteBefore(int tckin_id,
                        int seg_no,
                        TCkinRouteType1 route_type1,
                        TCkinRouteType2 route_type2);

    //возвращает следующий сегмент стыковки
    void GetNextSeg(int tckin_id,
                    int seg_no,
                    TCkinRouteType2 route_type2,
                    TCkinRouteItem& item);
    bool GetNextSeg(int grp_id,
                    TCkinRouteType2 route_type2,
                    TCkinRouteItem& item);     //результат=false только если для grp_id не производилась сквозная регистрация!
                                               //отсутствие следующего сегмента всегда лучше проверять по возвращенному item

    //возвращает предыдущий сегмент стыковки
    void GetPriorSeg(int tckin_id,
                     int seg_no,
                     TCkinRouteType2 route_type2,
                     TCkinRouteItem& item);
    bool GetPriorSeg(int grp_id,
                     TCkinRouteType2 route_type2,
                     TCkinRouteItem& item);    //результат=false только если для grp_id не производилась сквозная регистрация!
                                               //отсутствие предыдущего сегмента всегда лучше проверять по возвращенному item
};

enum TCkinSegmentSet { cssNone,
                       cssAllPrev,
                       cssAllPrevCurr,
                       cssAllPrevCurrNext,
                       cssCurr };

bool SeparateTCkin(int grp_id,
                   TCkinSegmentSet upd_depend,
                   TCkinSegmentSet upd_tid,
                   int tid,
                   int &tckin_id, int &seg_no);

enum TTripAlarmsType { atSalon, atWaitlist, atBrd, atOverload, atETStatus, atSeance, atLength };
void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms );
std::string TripAlarmString( TTripAlarmsType &alarm );

struct TCodeShareSets {
  private:
    TQuery *Qry;
  public:
    //настройки
    bool pr_mark_norms;
    bool pr_mark_bp;
    bool pr_mark_rpt;
    TCodeShareSets();
    ~TCodeShareSets();
    void clear()
    {
      pr_mark_norms=false;
      pr_mark_bp=false;
      pr_mark_rpt=false;
    };
    //важно! время вылета scd_out у operFlt в UTC
    void get(const TTripInfo &operFlt,
             const TTripInfo &markFlt);
};

//важно! время вылета scd_out у operFlt должно быть в UTC
//       return_scd_utc=false: время вылета в markFltInfo возвращается локальное относительно airp
//       return_scd_utc=true: время вылета в markFltInfo возвращается в UTC
void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo, bool return_scd_utc=false);

//важно! время вылета scd_out у operFlt должно быть в UTC
//       время вылета в markFltInfo передается локальное относительно airp
std::string GetMktFlightStr( const TTripInfo &operFlt, const TTripInfo &markFlt, bool &equal);
bool IsMarkEqualOper( const TTripInfo &operFlt, const TTripInfo &markFlt );

void GetCrsList(int point_id, std::vector<std::string> &crs);
bool IsRouteInter(int point_dep, int point_arv, std::string &country);
bool IsTrferInter(std::string airp_dep, std::string airp_arv, std::string &country);

std::string GetRouteAfterStr(BASIC::TDateTime part_key,  //NoExists если в оперативной базе, иначе в архивной
                             int point_id,
                             TTripRouteType1 route_type1,
                             TTripRouteType2 route_type2,
                             const std::string &lang="",
                             bool show_city_name=false,
                             const std::string &separator="-");
                             
std::string GetCfgStr(BASIC::TDateTime part_key,  //NoExists если в оперативной базе, иначе в архивной
                      int point_id,
                      const std::string &lang="",
                      const std::string &separator=" ");
                      
std::string GetPaxDocStr(BASIC::TDateTime part_key,
                         int pax_id,
                         TQuery& PaxDocQry,
                         bool with_issue_country=false,
                         const std::string &lang="");
                         
class TBagTagNumber
{
  public:
    std::string alpha_part;
    double numeric_part;
    TBagTagNumber(const std::string &apart, double npart):alpha_part(apart),numeric_part(npart) {};
    bool operator < (const TBagTagNumber &no) const
    {
      if (alpha_part!=no.alpha_part)
        return alpha_part<no.alpha_part;
      return numeric_part<no.numeric_part;
    };
};

void GetTagRanges(const std::vector<TBagTagNumber> &tags,
                  std::vector<std::string> &ranges);   //ranges сортирован

std::string GetTagRangesStr(const std::vector<TBagTagNumber> &tags);

std::string GetBagRcptStr(int grp_id, int pax_id);

bool BagPaymentCompleted(int grp_id, int *value_bag_count=NULL);

#endif /*_ASTRA_MISC_H_*/


