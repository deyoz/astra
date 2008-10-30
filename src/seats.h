#ifndef _SEATS_H_
#define _SEATS_H_

#include "astra_consts.h"
#include "salons.h"
#include "astra_utils.h"
#include <map>
#include <libxml/tree.h>

enum TSeatStep { sLeft, sRight, sUp, sDown };
enum TWhere { sLeftRight, sUpDown, sEveryWhere };
enum TSeatsType { stSeat, stReseat, stDropseat };

class TSeat
{
  public:
    char row[5]; //001-099,101-199
    char line[5];//A-Z...
    TSeat()
    {
      Clear();
    };
    void Clear()
    {
      *row=0;
      *line=0;
    };
    bool Empty()
    {
      return *row==0 || *line==0;
    };

    TSeat& operator = ( const TSeat& seat )
    {
      if (this == &seat) return *this;
      strncpy(this->row,seat.row,sizeof(seat.row));
      strncpy(this->line,seat.line,sizeof(seat.line));
      return *this;
    };

    friend bool operator == ( const TSeat& seat1, const TSeat& seat2 )
    {
      return strcmp(seat1.row,seat2.row)==0 &&
             strcmp(seat1.line,seat2.line)==0;
    };

    friend bool operator != ( const TSeat& seat1, const TSeat& seat2 )
    {
      return !(strcmp(seat1.row,seat2.row)==0 &&
               strcmp(seat1.line,seat2.line)==0);
    };

    friend bool operator < ( const TSeat& seat1, const TSeat& seat2 )
    {
      int res;
      res=strcmp(seat1.row,seat2.row);
      if (res==0)
        res=strcmp(seat1.line,seat2.line);
      return res<0;
    };
};

class TSeatRange : public std::pair<TSeat,TSeat>
{
  public:
    char rem[5];
    TSeatRange() : std::pair<TSeat,TSeat>()
    {
      *rem=0;
    };
    TSeatRange(TSeat seat1, TSeat seat2) : std::pair<TSeat,TSeat>(seat1,seat2)
    {
      *rem=0;
    };
    friend bool operator < ( const TSeatRange& range1, const TSeatRange& range2 )
    {
      return range1.first<range2.first;
    };
};

//все нижеследующие функции работают только с IATA (нормальными) местами
void NormalizeSeat(TSeat &seat);
void NormalizeSeatRange(TSeatRange &range);
bool NextNormSeatRow(TSeat &seat);
bool PriorNormSeatRow(TSeat &seat);
TSeat& FirstNormSeatRow(TSeat &seat);
TSeat& LastNormSeatRow(TSeat &seat);
bool NextNormSeatLine(TSeat &seat);
bool PriorNormSeatLine(TSeat &seat);
TSeat& FirstNormSeatLine(TSeat &seat);
TSeat& LastNormSeatLine(TSeat &seat);
bool NextNormSeat(TSeat &seat);
bool SeatInRange(TSeatRange &range, TSeat &seat);
bool NextSeatInRange(TSeatRange &range, TSeat &seat);

class TCounters {
  private:
    int p_Count_3G;
    int p_Count_2G;
    int p_CountG;
    int p_Count_3V;
    int p_Count_2V;
  public:
    TCounters();
    void Clear();
    int p_Count_3( TSeatStep Step = sRight );
    int p_Count_2( TSeatStep Step = sRight );
    int p_Count( TSeatStep Step = sRight );
    void Set_p_Count_3( int Count, TSeatStep Step = sRight );
    void Set_p_Count_2( int Count, TSeatStep Step = sRight );
    void Set_p_Count( int Count, TSeatStep Step = sRight );
    void Add_p_Count_3( int Count, TSeatStep Step = sRight );
    void Add_p_Count_2( int Count, TSeatStep Step = sRight );
    void Add_p_Count( int Count, TSeatStep Step = sRight );
};

struct TPassenger {
  /*вход*/
  int index;
  int grpId;
  int regNo;
  std::string fullName;
  int pax_id; /* pax_id */
  std::string placeName;
  std::string PrevPlaceName;
  std::string OldPlaceName;
  bool isSeat;
  int countPlace;
  TSeatStep Step;
  std::vector<std::string> rems;
  std::string maxRem;
  std::string placeRem; /* 'NSSA', 'NSSW', 'NSSB' и т. д. */
  bool prSmoke;
  std::string Elem_Type;
  std::string clname;
  std::string placeStatus;
  std::string pers_type;
  int priority;
  int tid;
  std::string preseat;
  std::string agent_seat;
  std::vector<TSeat> seat_no;
  /*выход*/
  TPlaceList *placeList; /* салон */
  TPoint Pos; /* указывает место */
  bool InUse;
  bool isValidPlace;
  TPassenger() {
    countPlace = 1;
    prSmoke = false;
    placeStatus = "FP";
    priority = 0;
    placeList = NULL;
    Pos.x = 0;
    Pos.y = 0;
    InUse = false;
    isValidPlace = true;
    tid = -1;
  }
  void set_seat_no();
};

typedef std::vector<TPassenger> VPassengers;

class TPassengers {
  private:
    std::map<std::string, int> remarks;
    VPassengers FPassengers;
    void LoadRemarksPriority();
    void addRemPriority( TPassenger &pass );
    void Calc_Priority( TPassenger &pass );
  public:
    TCounters counters;
    std::string clname;   // класс с которым мы работаем
    bool KTube;
    bool KWindow;
    bool UseSmoke;
    TPassengers();
    ~TPassengers();
    void Clear();
    void Add( TPassenger &pass );
    int getCount();
    TPassenger &Get( int Idx );
    void copyTo( VPassengers &npass );
    void copyFrom( VPassengers &npass );
    void SetCountersForPass( TPassenger  &pass );
    bool existsNoSeats();
    void Build( TSalons &Salons, xmlNodePtr passNode );
    void sortByIndex();
};

struct TSeatPlace {
  TPlaceList *placeList;
  TPoint Pos;
  std::vector<TPlace> oldPlaces;
  TSeatStep Step;
  bool InUse;
  bool isValid;
  TSeatPlace() {
    placeList = NULL;
    InUse = false;
    isValid = true;
  }
};

typedef std::vector<TSeatPlace> VSeatPlaces;
typedef VSeatPlaces::iterator ISeatPlace;

class TSeatPlaces {
  private:
    VSeatPlaces seatplaces;
    bool Alone; /* посадка одного в ряду - внутренняя переменная - не трогать */
    int Put_Find_Places( TPoint FP, TPoint EP, int foundCount, TSeatStep Step );
    int FindPlaces_From( TPoint FP, int foundCount, TSeatStep Step );
    bool SeatSubGrp_On( TPoint FP, TSeatStep Step, int Wanted );
    bool SeatsStayedSubGrp( TWhere Where );
    TSeatPlace &GetEqualSeatPlace( TPassenger &pass );
    bool LSD( int G3, int G2, int G, int V3, int V2, TWhere Where );
    bool SeatsGrp_On( TPoint FP );
    bool SeatsPassenger_OnBasePlace( std::string &placeName, TSeatStep Step );
  public:
    TCounters counters;
    TSeatPlaces();
    ~TSeatPlaces();
    void Clear();
    void Add( TSeatPlace &seatplace );
    void RollBack( int Begin, int End );
    void RollBack( );
    bool SeatGrpOnBasePlace( );
    bool SeatsGrp( );
    bool SeatsPassengers( bool pr_autoreseats = false );
    void PlacesToPassengers();
};

namespace SEATS {
/* тут описаны будут доступные ф-ции */
/* автоматическая пересадка пассажиров при изменении компоновки */
void AutoReSeatsPassengers( TSalons &Salons, TPassengers &passengers, int SeatAlgo );
void SeatsPassengers( TSalons *Salons, int SeatAlgo, bool FUse_BR=false );
void ChangeLayer( ASTRA::TCompLayerType layer_type, int point_id, int pax_id, int &tid,
                  std::string first_xname, std::string first_yname, TSeatsType seat_type, bool pr_lat_seat );
void SaveTripSeatRanges( int point_id, ASTRA::TCompLayerType layer_type, std::vector<TSeatRange> &seats,
	                       int pax_id, int point_dep, int point_arv );
bool GetPassengersForManualSeat( int point_id, ASTRA::TCompLayerType layer_type, TPassengers &p, bool pr_lat_seat );
int GetSeatAlgo(TQuery &Qry, std::string airline, int flt_no, std::string airp_dep);
}
extern TPassengers Passengers;

#endif /*_SEATS_H_*/

