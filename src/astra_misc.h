#ifndef _ASTRA_MISC_H_
#define _ASTRA_MISC_H_

#include <vector>
#include <string>
#include "basic.h"
#include "astra_consts.h"
#include "oralib.h"

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

std::string mkt_airline(int pax_id);

#endif /*_ASTRA_MISC_H_*/


