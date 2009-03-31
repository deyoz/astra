#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include <string>
#include "basic.h"
#include "astra_consts.h"
#include "oralib.h"
#include "astra_utils.h"
#include "stages.h"

struct TMktFlight {
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string subcls;
    int scd;
    std::string airp_dep;
    std::string airp_arv;
    void get(int pax_id);
    bool IsNULL();
    void dump();
    TMktFlight():
        flt_no(ASTRA::NoExists),
        scd(ASTRA::NoExists)
    {};
};

class TTripInfo
{
  public:
    std::string airline,suffix,airp;
    int flt_no, pr_del, point_num, first_point;
    BASIC::TDateTime scd_out,real_out,real_out_local_date;
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
      real_out_local_date=ASTRA::NoExists; //GetTripName устанавливает значение
      pr_del = ASTRA::NoExists;
      point_num = ASTRA::NoExists;
      first_point = ASTRA::NoExists;
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
      real_out_local_date=ASTRA::NoExists;
      if (Qry.GetFieldIndex("pr_del")>=0)
        pr_del = Qry.FieldAsInteger("pr_del");
      else
        pr_del = ASTRA::NoExists;
      if (Qry.GetFieldIndex("point_num")>=0)
        point_num = Qry.FieldAsInteger("point_num");
      else
        point_num = ASTRA::NoExists;
      if (Qry.GetFieldIndex("first_point")>=0)
        first_point = Qry.FieldAsInteger("first_point");
      else
        first_point = ASTRA::NoExists;
    };
};

std::string GetTripName( TTripInfo &info, bool showAirp=false, bool prList=false  );

//настройки рейса
enum TTripSetType { tsETLOnly=11, tsIgnoreTrferSet=12, tsMixedNorms=13 };
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
std::string GetPaxPnrAddr(int pax_id, std::vector<TPnrAddrItem> &pnrs, std::string airline="");
BASIC::TDateTime DayToDate(int day, BASIC::TDateTime base_date);

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

enum TTripAlarmsType { atSalon, atWaitlist, atBrd, atOverload, atETStatus, atLength };
void TripAlarms( int point_id, BitSet<TTripAlarmsType> &Alarms );
std::string TripAlarmString( TTripAlarmsType &alarm );
#endif /*_ASTRA_MISC_H_*/


