#ifndef _SEATS_H_
#define _SEATS_H_

#include "astra_consts.h"
#include "salons.h"
#include "astra_utils.h"
#include "seats_utils.h"
#include "base_tables.h"
#include "date_time.h"
#include <map>
#include <libxml/tree.h>

namespace SEATS2
{
using BASIC::date_time::TDateTime;
using namespace SALONS2;
enum TSeatStep { sLeft, sRight, sUp, sDown };
enum TWhere { sLeftRight, sUpDown, sEveryWhere };
enum TSeatsType { stSeat, stReseat, stDropseat };
enum TChangeLayerFlags { flWaitList, flQuestionReseat, flSetPayLayer, flCheckPayLayer };
enum TChangeLayerProcFlag { clNotPaySeat, clPaySeatSet, clPaySeatCheck };

/* алгоритм рассадки пассажиров
       sdUpDown_Row - сверху вниз в ряд
       sdUpDown_Line - сверху вниз в линию
       sdDownUp_Row - снизу вверх в ряд
       sdDownUp_Line - снизу вверх в линию
*/
enum TSeatAlgoTypes { sdUpDown_Line, sdDownUp_Line, sdUpDown_Row, sdDownUp_Row };

struct TSeatAlgoParams {
     TSeatAlgoTypes SeatAlgoType;
     bool pr_canUseOneRow;
     TSeatAlgoParams() {
         SeatAlgoType = sdUpDown_Line;
         pr_canUseOneRow = false;
     }
};

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

struct TDefaults {
  ASTRA::TCompLayerType grp_status;
  std::string pers_type;
  std::string clname;
  std::string placeName;
  std::string wl_type;
  int countPlace;
  bool isSeat;
  std::string ticket_no;
  std::string document;
  int bag_weight;
  int bag_amount;
  int excess;
  std::string trip_from;
  std::string pass_rem;
  std::string comp_rem;
  bool pr_down;
  TDefaults() {
    grp_status = ASTRA::cltCheckin;
    pers_type = EncodePerson( ASTRA::adult );
    clname = EncodeClass( ASTRA::Y );
    countPlace = 1;
    isSeat = true;
    bag_weight = 0;
    bag_amount = 0;
    excess = 0;
    pr_down = false;
  };
};

struct TCoordSeat {
    int placeListIdx;
    TPoint p;
    TCoordSeat() {}
    TCoordSeat( int vnum, int x, int y ) {
      placeListIdx = vnum;
      p.x = x;
      p.y = y;
    }
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
    int paxId; /* pax_id */
    int preseat_pax_id;
    int point_arv;
    //std::string placeName;
    std::string foundSeats;
    bool isSeat;
    std::string wl_type;
    int countPlace;
    bool is_jmp;
    TSeatStep Step;
    std::string SUBCLS_REM;
    std::string maxRem;
    std::string placeRem; /* 'NSSA', 'NSSW', 'NSSB' и т. д. */
    bool prSmoke;
    std::string clname;
    //ASTRA::TCompLayerType layer; // статус пассажира предв. рассадка, бронь, ...
    ASTRA::TCompLayerType grp_status; // статус группы Т - транзит ...
    int priority;
    int tid;
    std::string preseat_no;
    ASTRA::TCompLayerType preseat_layer;
    std::vector<TCoordSeat> preseatPlaces;
    bool dont_check_payment;
    //std::string agent_seat;
    std::string ticket_no;
    std::string document;
    int bag_weight;
    int bag_amount;
    int excess;
    std::string trip_from;
    std::string pass_rem;
    /*выход*/
    std::vector<TSeat> seat_no;
    SALONS2::TPlaceList *placeList; /* салон */
    SALONS2::TPoint Pos; /* указывает место */
    bool InUse;
    bool isValidPlace;
    TSeatTariffMapType tariffs;
    TSeatTariffMap::TStatus tariffStatus;
    TPassenger() {
      regNo = -1;
      bag_weight = 0;
      bag_amount = 0;
      excess = 0;
      countPlace = 1;
      is_jmp = false;
      prSmoke = false;
      dont_check_payment = false;
      preseat_layer = ASTRA::cltUnknown;
      grp_status = ASTRA::cltUnknown;
      priority = 0;
      SUBCLS_REM = "";
      placeList = NULL;
      Pos.x = 0;
      Pos.y = 0;
      InUse = false;
      isValidPlace = true;
      tid = -1;
      point_arv = ASTRA::NoExists;
    }
    void set_seat_no();
    void add_rem( std::string code );
    void calc_priority(std::map<std::string, int> &remarks);
    void get_remarks( std::vector<std::string> &vrems );
    bool isRemark( std::string code );
    bool is_valid_seats( const std::vector<SALONS2::TPlace> &places );
    void build( xmlNodePtr pNode, const TDefaults& def);
    std::string toString() const {
      std::ostringstream buf;
      buf << std::fixed << std::setprecision(2);
      buf << "index=" << index << ",";
      buf << "grpId=" << grpId << ",";
      buf << "regNo=" << regNo << ",";
      if ( !fullName.empty() ) {
        buf << "fullName=" << fullName << ",";
      }
      if ( !pers_type.empty() ) {
        buf << "pers_type=" << pers_type << ",";
      }
      buf << "paxId=" << paxId << ",";
      buf << "preseat_pax_id=" << preseat_pax_id << ",";
      buf << "point_arv=" << point_arv << ",";
      if ( !foundSeats.empty() ) {
        buf << "foundSeats=" << foundSeats << ",";
      }
      if ( isSeat ) {
        buf << "isSeat=" << isSeat << ",";
      }
      if ( !wl_type.empty() ) {
        buf << "wl_type=" << wl_type << ",";
      }
      if ( countPlace != 1 ) {
        buf << "countPlace=" << countPlace << ",";
      }
      if ( is_jmp ) {
        buf << "is_jmp,";
      }
      if ( Step != sLeft && Step != sRight ) {
        buf << "Step=UpDown";
      }
      if ( !SUBCLS_REM.empty() ) {
        buf << "SUBCLS_REM=" << SUBCLS_REM << ",";
      }
      if ( !maxRem.empty() ) {
        buf << "maxRem=" << maxRem << ",";
      }
      if ( !placeRem.empty() ) {
        buf << "placeRem=" << placeRem << ",";
      }
      if ( prSmoke ) {
        buf << "prSmoke,";
      }
      if ( !clname.empty() ) {
        buf << "clname=" << clname << ",";
      }
      buf << "grp_status=" << EncodeCompLayerType(grp_status) << ",";
      if ( priority != 0 ) {
        buf << "priority=" << priority << ",";
      }
      if ( tid  >= 0 ) {
        buf << "tid=" << tid << ",";
      }
      if ( !preseat_no.empty() ) {
        buf << "preseat_no=" << preseat_no << ",";
      }
      if ( preseat_layer != ASTRA::cltUnknown ) {
        buf << "preseat_layer=" << EncodeCompLayerType(preseat_layer) << ",";
      }
      if ( !ticket_no.empty() ) {
        buf << "ticket_no=" << ticket_no << ",";
      }
      if ( !document.empty() ) {
        buf << "document=" << document << ",";
      }
      if ( bag_weight > 0 ) {
        buf << "bag_weight=" << bag_weight << ",";
      }
      if ( bag_amount > 0 ) {
        buf << "bag_amount=" << bag_amount << ",";
      }
      if ( excess != 0 ) {
        buf << "excess=" << excess << ",";
      }
      if ( !trip_from.empty() ) {
        buf << "trip_from=" << trip_from << ",";
      }
      if ( !pass_rem.empty() ) {
        buf << "pass_rem=" << pass_rem << ",";
      }
      if ( !seat_no.empty() ) {
        buf << "seats=";
        for ( std::vector<TSeat>::const_iterator iseat=seat_no.begin(); iseat!=seat_no.end(); iseat++ ) {
          buf << iseat->row << iseat->line << " ";
        }
        buf << ",";
      }
      if ( InUse ) {
        buf << "InUse,";
      }
      if ( !isValidPlace ) {
        buf << "not isValidPlace,";
      }
      if ( dont_check_payment ) {
        buf << "dont_check_payment,";
      }
      buf << "tariffPassStatus=";
      switch(tariffStatus)
      {
        case TSeatTariffMap::stNotFound:      buf << "stNotFound";      break;
        case TSeatTariffMap::stNotOperating:  buf << "stNotOperating";  break;
        case TSeatTariffMap::stNotET:         buf << "stNotET";         break;
        case TSeatTariffMap::stUnknownETDisp: buf << "stUnknownETDisp"; break;
        case TSeatTariffMap::stNotRFISC:      buf << "stNotRFISC";  break;
        case TSeatTariffMap::stUseRFISC:      buf << "stUseRFISC";  break;
      }
      if ( !tariffs.empty() ) {
        buf << "tariffs=" << tariffs.key() << ",";
      }
      if ( !rems.empty() ) {
         buf << "rems=";
         for ( std::vector<std::string>::const_iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
           buf << *irem << " ";
         }
      }
      return buf.str();
    }
};

typedef std::vector<TPassenger> VPassengers;

class TPassengers {
  private:
    std::map<std::string, int> remarks;
    VPassengers FPassengers;
  public:
    TCounters counters;
    std::string clname;   // класс с которым мы работаем
    bool KTube;
    bool KWindow;
    bool UseSmoke;
    TPassengers();
    ~TPassengers();
    void Clear();
    void Add( const SALONS2::TSalonList &salonList, TPassenger &pass );
    void Add( SALONS2::TSalons &Salons, TPassenger &pass );
    void Add( TPassenger &pass );
    void Add( TPassenger &pass, int index );
    int getCount() const;
    TPassenger &Get( int Idx );
    void copyTo( VPassengers &npass );
    void copyFrom( VPassengers &npass );
    void SetCountersForPass( TPassenger  &pass );
    bool existsNoSeats();
    void Build( xmlNodePtr passNode );
    void sortByIndex();
    void operator = (TPassengers &items);
    bool withBaby();
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
    int separately_seats_adults_crs_pax_id;
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
    void operator = ( VSeatPlaces &items );
    void operator >> ( VSeatPlaces &items );
};

struct TSublsRem {
    std::string subclass;
    std::string rem;
};

struct TSublsRems {
    std::string airline;
    std::vector<TSublsRem> rems;
    TSublsRems( const std::string &airline );
    bool IsSubClsRem( const std::string &subclass, std::string &rem );
};

typedef std::map<ASTRA::TCompLayerType,bool> TUseLayers;

/* тут описаны будут доступные ф-ции */
/* автоматическая пересадка пассажиров при изменении компоновки */
void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &APass, TSeatAlgoParams ASeatAlgoParams );
void AutoReSeatsPassengers( SALONS2::TSalonList &salonList,
                            const SALONS2::TIntArvSalonPassengers &passengers,
                            TSeatAlgoParams ASeatAlgoParams );
void SeatsPassengers( SALONS2::TSalonList &salonList,
                      TSeatAlgoParams ASeatAlgoParams,
                      ASTRA::TClientType client_type,
                      TPassengers &passengers,
                      SALONS2::TAutoSeats &seats );
bool ChangeLayer( const SALONS2::TSalonList &salonList, ASTRA::TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  std::string first_xname, std::string first_yname, TSeatsType seat_type, TChangeLayerProcFlag seatFlag );
bool ChangeLayer( ASTRA::TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  std::string first_xname, std::string first_yname, TSeatsType seat_type,
                  bool pr_lat_seat, TChangeLayerProcFlag seatFlag );
void SaveTripSeatRanges( int point_id, ASTRA::TCompLayerType layer_type, TSeatRanges &seats,
                         int pax_id, int point_dep, int point_arv, TDateTime time_create );
bool GetPassengersForWaitList( int point_id, TPassengers &p );
TSeatAlgoParams GetSeatAlgo(TQuery &Qry, std::string airline, int flt_no, std::string airp_dep);
bool IsSubClsRem( const std::string &airline, const std::string &subclass, std::string &rem );

extern TPassengers Passengers;
} // end namespace SEATS2

#endif /*_SEATS2_H_*/

