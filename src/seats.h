#ifndef _SEATS_H_
#define _SEATS_H_

#include "astra_consts.h"
#include "salons.h"
#include "astra_utils.h"
#include "seats_utils.h"
#include "base_tables.h"
#include <map>
#include <libxml/tree.h>

namespace SEATS2
{
using namespace SALONS2;
enum TSeatStep { sLeft, sRight, sUp, sDown };
enum TWhere { sLeftRight, sUpDown, sEveryWhere };
enum TSeatsType { stSeat, stReseat, stDropseat };

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
	private:
    std::vector<std::string> rems;
	public:
  /*вход*/
  int index;
  int grpId;
  int regNo;
  std::string fullName;
  std::string pers_type;
  int pax_id; /* pax_id */
  std::string placeName;
  std::string PrevPlaceName;
  //std::string OldPlaceName;
  bool isSeat;
  int countPlace;
  TSeatStep Step;
  bool pr_MCLS;
  std::string maxRem;
  std::string placeRem; /* 'NSSA', 'NSSW', 'NSSB' и т. д. */
  bool prSmoke;
  std::string Elem_Type;
  std::string clname;
  ASTRA::TCompLayerType layer; // статус пассажира предв. рассадка, бронь, ...
  ASTRA::TCompLayerType grp_status; // статус группы Т - транзит ...
  int priority;
  int tid;
  std::string preseat;
  std::string agent_seat;
  std::vector<TSeat> seat_no;
  std::string ticket_no;
  std::string document;
  int bag_weight;
  int bag_amount;
  int excess;
  std::string trip_from;
  std::string pass_rem;
  /*выход*/
  SALONS2::TPlaceList *placeList; /* салон */
  SALONS2::TPoint Pos; /* указывает место */
  bool InUse;
  bool isValidPlace;
  TPassenger() {
  	bag_weight = 0;
  	bag_amount = 0;
  	excess = 0;
    countPlace = 1;
    prSmoke = false;
    layer = ASTRA::cltUnknown;
    grp_status = ASTRA::cltUnknown;
    priority = 0;
    pr_MCLS = false;
    placeList = NULL;
    Pos.x = 0;
    Pos.y = 0;
    InUse = false;
    isValidPlace = true;
    tid = -1;
  }
  void set_seat_no();
  void add_rem( std::string code );
  void calc_priority(std::map<std::string, int> &remarks);
  void get_remarks( std::vector<std::string> &vrems );
  bool isRemark( std::string code );
  bool is_valid_seats( const std::vector<SALONS2::TPlace> &places );
  void build( xmlNodePtr pNode );
};

typedef std::vector<TPassenger> VPassengers;

class TPassengers {
  private:
    std::map<std::string, int> remarks;
    VPassengers FPassengers;
    void LoadRemarksPriority( std::map<std::string, int> &rems );
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
    void Build( xmlNodePtr passNode );
    void sortByIndex();
};

struct TSeatPlace {
  SALONS2::TPlaceList *placeList;
  SALONS2::TPoint Pos;
  std::vector<SALONS2::TPlace> oldPlaces;
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
    int Put_Find_Places( SALONS2::TPoint FP, SALONS2::TPoint EP, int foundCount, TSeatStep Step );
    int FindPlaces_From( SALONS2::TPoint FP, int foundCount, TSeatStep Step );
    bool SeatSubGrp_On( SALONS2::TPoint FP, TSeatStep Step, int Wanted );
    bool SeatsStayedSubGrp( TWhere Where );
    TSeatPlace &GetEqualSeatPlace( TPassenger &pass );
    bool LSD( int G3, int G2, int G, int V3, int V2, TWhere Where );
    bool SeatsGrp_On( SALONS2::TPoint FP );
    bool SeatsPassenger_OnBasePlace( std::string &placeName, TSeatStep Step );
  public:
  	ASTRA::TCompLayerType grp_status;
    TCounters counters;
    TSeatPlaces( /*ASTRA::TCompLayerType layer_type*/ );
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

/* тут описаны будут доступные ф-ции */
/* автоматическая пересадка пассажиров при изменении компоновки */
void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &passengers, int SeatAlgo );
void SeatsPassengers( SALONS2::TSalons *Salons, int SeatAlgo, TPassengers &passengers, bool FUse_BR=false );
void ChangeLayer( ASTRA::TCompLayerType layer_type, int point_id, int pax_id, int &tid,
                  std::string first_xname, std::string first_yname, TSeatsType seat_type, bool pr_lat_seat );
void SaveTripSeatRanges( int point_id, ASTRA::TCompLayerType layer_type, std::vector<TSeatRange> &seats,
	                       int pax_id, int point_dep, int point_arv );
bool GetPassengersForWaitList( int point_id, TPassengers &p, bool pr_exists=false );
int GetSeatAlgo(TQuery &Qry, std::string airline, int flt_no, std::string airp_dep);
extern TPassengers Passengers;
} // end namespace SEATS2

#endif /*_SEATS2_H_*/

