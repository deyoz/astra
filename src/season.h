#ifndef _SEASON_H_
#define _SEASON_H_

#include <bitset>
#include "date_time.h"
#include "astra_elems.h"
#include "astra_consts.h"
#include "jxtlib/JxtInterface.h"
#include "oralib.h"

#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_date_time.h"
#include "astra_utils.h"
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace ASTRA::date_time;
using namespace std;
using namespace AstraLocale;
using namespace boost::posix_time;

namespace SEASON {

using BASIC::date_time::TDateTime;

  struct TDest {
    int num;
    std::string airp;
    TElemFmt airp_fmt;
    std::string city;
    TElemFmt city_fmt;
    int pr_del;
    TDateTime scd_in;
    std::string airline;
    TElemFmt airline_fmt;
    std::string region;
    int trip;
    std::string craft;
    TElemFmt craft_fmt;
    std::string litera;
    std::string triptype;
    TDateTime scd_out;
    int f;
    int c;
    int y;
    std::string unitrip;
    std::string suffix;
    TElemFmt suffix_fmt;
    TDateTime diff;
    TDest() {
      diff = 0.0;
      num = ASTRA::NoExists;
      scd_out = ASTRA::NoExists;
      scd_in = ASTRA::NoExists;
      f = 0;
      c = 0;
      y = -1;
      pr_del = 0;
    }
  };

//////////////////////////////////////////////////////////////

  struct TRange {

    TDateTime first;
    TDateTime last;
    string days;

    TRange() {
      first = NoExists;
      last = NoExists;
    }

  };


  struct TPeriod {
    int move_id;

    //Для вычисления перехода  int first_dest;
    TDateTime first;
    TDateTime last;

    string days;
    string tlg;
    string ref;

    tmodify modify;

    bool pr_del;
    int hours;

    TPeriod() {
      modify = fnochange;
      pr_del = false;
    }
  };

  struct TSeason {
      time_period period;
      bool summer;
      string name;

      TSeason( ptime start_time, ptime end_time, bool asummer, string aname ):
              period( start_time, end_time), summer(asummer), name(aname) {};

      TSeason(const season& s) :
              period(DateTimeToBoost(s.begin()), DateTimeToBoost(s.end())),
              summer(s.isSummer())
       {
          int year = Year(s.begin());
          if(summer)
              name = getLocaleText( string( "Лето" ) ) + " " + IntToString( year );
          else
              name = getLocaleText( string( "Зима" ) ) + " " + IntToString( year ) + "-" + IntToString( year + 1 );
      }
  };

  struct timeDiff {
    TDateTime first;
    TDateTime last;
    int hours;
  };

  typedef vector<timeDiff> TTimeDiff;

  struct trip {
    int trip_id;
    string name;
    string print_name;
    string crafts;
    string airlineId;
    string airpId;
    string craftId;
    string triptypeId;
    string owncraft;
    string ownport;
    string portsForAirline;
    vector<TDest> vecportsFrom, vecportsTo;
    string bold_ports;
    TDateTime scd_in;
    TDateTime scd_out;
    TDateTime trap;
    string triptype;
    int pr_del;
    trip() {
      scd_in = NoExists;
      scd_out = NoExists;
      trap = NoExists;
    }
  };

  typedef vector<TDest> TDests;

  struct TDestList {
    bool pr_del;
    TDateTime flight_time;
    TDateTime last_day;
    TDests dests;
    //!!!08.04.13TDateTime diff;
    vector<trip> trips;
  };

  typedef map<int,TDestList> tmapds;

  typedef map<double,tmapds> TSpp;

  struct TRangeList {
    int trip_id;
    vector<TPeriod> periods;
  };

  class TFilter {
    public:
      string filter_tz_region; // регион, относительно которого рассчитывается период расписания
      deque<TSeason> periods; //периоды летнего и зимнего расписания
      int season_idx; // текущее расписание
      TRange range; // диапазон дат в фильтре, когда не задан - диапазон расписания с временами
      TDateTime firstTime;
      TDateTime lastTime;
      string airline;
      string city;
      string airp;
      string triptype;
      void Clear();
      void Parse( xmlNodePtr filterNode );
      void Build( xmlNodePtr filterNode );
      void GetSeason();
      bool isSummer( TDateTime pfirst );
      void InsertSectsPeriods( map<int,TDestList> &mapds,
                               vector<TPeriod> &speriods, vector<TPeriod> &nperiods, TPeriod p );
      bool isFilteredTime( TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                           const string &flight_tz_region );
      bool isFilteredUTCTime( TDateTime vd, TDateTime first, TDateTime dest_time );
      bool isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                           const string &flight_tz_region );
      TDateTime GetTZTimeDiff( TDateTime utcnow, TDateTime first, const string &tz_region );
      TFilter();
  };

  void int_write( const TFilter &filter, const std::string &flight, vector<TPeriod> &speriods,
                  int &trip_id, map<int,TDestList> &mapds );

struct TViewTrip {
  int trip_id;
  int move_id;
  std::string name;
  std::string crafts;
  std::string ports;
  TDateTime scd_in;
  TDateTime scd_out;
  TDateTime first;
  TDateTime last;
  std::string days;
  bool pr_del;
  TViewTrip() {
    first = ASTRA::NoExists;
  }
};

struct TViewPeriod {
  int trip_id;
  std::string exec;
  std::string noexec;
  std::vector<TViewTrip> trips;
};

void ReadTripInfo( int trip_id, std::vector<TViewPeriod> &viewp, xmlNodePtr reqNode );

} // namespace SEASON

void CreateSPP( TDateTime localdate );

class SeasonInterface : public JxtInterface
{
public:
  SeasonInterface() : JxtInterface("123","season")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Filter);
     AddEvent("filter",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Read);
     AddEvent("season_read",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Slots);
     AddEvent("season_slots",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Write);
     AddEvent("write",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::GetSPP);
     AddEvent("get_spp",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::DelRangeList);
     AddEvent("del_range_list",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Edit);
     AddEvent("edit",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::ViewSPP);
     AddEvent("spp",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::convert);
     AddEvent("convert",evHandle);
  };

  void Filter(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Slots(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Edit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

//std::string GetTZRegion( std::string &city, std::map<std::string,std::string> &regions, bool vexcept=1 );
std::string AddDays( std::string days, int delta );

class TDoubleTrip
{
  private:
     TQuery *Qry;
  public:
     TDoubleTrip();
     ~TDoubleTrip();
     bool IsExists( int move_id, std::string airline, int flt_no,
                      std::string suffix, std::string airp,
                          TDateTime scd_in, TDateTime scd_out,
                    int &point_id );
};

enum TConvert { mtoUTC, mtoLocal };

TDateTime ConvertFlightDate( TDateTime time, TDateTime first, const std::string &airp, bool pr_arr, TConvert convert );



#endif
