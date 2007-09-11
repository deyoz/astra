#ifndef _SEATS_H_
#define _SEATS_H_

#include "salons.h"
#include "astra_utils.h"
#include <map>
#include <libxml/tree.h>

enum TSeatStep { sLeft, sRight, sUp, sDown };
enum TWhere { sLeftRight, sUpDown, sEveryWhere };
enum TSeatsType { sreseats, sreserve };

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
  /*�室*/
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
  std::string placeRem; /* 'NSSA', 'NSSW', 'NSSB' � �. �. */
  bool prSmoke;
  std::string Elem_Type;
  std::string clname;
  std::string placeStatus;
  std::string pers_type;
  int priority;
  int tid;
  std::string preseat;
  /*��室*/
  TPlaceList *placeList; /* ᠫ�� */
  TPoint Pos; /* 㪠�뢠�� ���� */
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
};

typedef std::vector<TPassenger> VPassengers;

class TPassengers {
  private:
    std::map<std::string, int> remarks;
    VPassengers FPassengers;
    void addRemPriority( TPassenger &pass );
    void Calc_Priority( TPassenger &pass );
  public:
    TCounters counters;
    std::string clname;   // ����� � ����� �� ࠡ�⠥�
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
    bool Alone; /* ��ᠤ�� ������ � ��� - ����७��� ��६����� - �� �ண��� */
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
/* ��� ���ᠭ� ���� ����㯭� �-樨 */
/* ��⮬���᪠� ���ᠤ�� ���ᠦ�஢ �� ��������� ���������� */
void ReSeatsPassengers( TSalons *Salons, bool DeleteNotFreePlaces, bool SeatOnNotBase );
bool Reseat( TSeatsType seatstype, int trip_id, int pax_id, int &tid, int num, int x, int y, std::string &nplaceName, bool cancel=false );
void SelectPassengers( TSalons *Salons, TPassengers &p );
void SeatsPassengers( TSalons *Salons, bool FUse_BR=false );
void SavePlaces( );
}
extern TPassengers Passengers;

#endif /*_SEATS_H_*/

