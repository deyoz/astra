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
    void get_client_dates(BASIC::TDateTime &scd_out_client, BASIC::TDateTime &real_out_client) const;
};

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
enum TTripSetType { tsOutboardTrfer=10,
                    tsETLOnly=11,
                    tsIgnoreTrferSet=12,
                    tsMixedNorms=13,
                    tsNoTicketCheck=15,
                    tsCharterSearch=16 };
bool GetTripSets( TTripSetType setType, TTripInfo &info );

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

std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs, std::string airline="");
std::string GetPnrAddr(int pnr_id, std::vector<TPnrAddrItem> &pnrs);
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string airline="");

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
    void GetRoute(int point_id,
                  int point_num,
                  int first_point,
                  bool pr_tranzit,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2,
                  TQuery& Qry);
    bool GetRoute(int point_id,
                  bool after_current,
                  TTripRouteType1 route_type1,
                  TTripRouteType2 route_type2);

  public:
    //маршрут после пункта point_id
    bool GetRouteAfter(int point_id,
                       TTripRouteType1 route_type1,
                       TTripRouteType2 route_type2);
    void GetRouteAfter(int point_id,
                       int point_num,
                       int first_point,
                       bool pr_tranzit,
                       TTripRouteType1 route_type1,
                       TTripRouteType2 route_type2);
    //маршрут до пункта point_id
    bool GetRouteBefore(int point_id,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2);
    void GetRouteBefore(int point_id,
                        int point_num,
                        int first_point,
                        bool pr_tranzit,
                        TTripRouteType1 route_type1,
                        TTripRouteType2 route_type2);

    //возвращает следующий пункт маршрута
    void GetNextAirp(int point_id,
                     int point_num,
                     int first_point,
                     bool pr_tranzit,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);
    bool GetNextAirp(int point_id,
                     TTripRouteType2 route_type2,
                     TTripRouteItem& item);

    //возвращает предыдущий пункт маршрута
 /*   void GetPriorAirp(int point_id,
                      int point_num,
                      int first_point,
                      bool pr_tranzit,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);
    bool GetPriorAirp(int point_id,
                      TTripRouteType2 route_type2,
                      TTripRouteItem& item);*/
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
//       время вылета в markFltInfo возвращается локальное относительно airp
void GetMktFlights(const TTripInfo &operFltInfo, std::vector<TTripInfo> &markFltInfo);

//важно! время вылета scd_out у operFlt должно быть в UTC
//       время вылета в markFltInfo возвращается локальное относительно airp
std::string GetMktFlightStr( const TTripInfo &operFlt, const TTripInfo &markFlt );

void GetCrsList(int point_id, std::vector<std::string> &crs);
bool IsRouteInter(int point_dep, int point_arv, std::string &country);
bool IsTrferInter(std::string airp_dep, std::string airp_arv, std::string &country);

#endif /*_ASTRA_MISC_H_*/


