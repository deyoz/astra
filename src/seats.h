#ifndef _SEATS_H_
#define _SEATS_H_

#include "astra_consts.h"
#include "salons.h"
#include "astra_utils.h"
#include "seats_utils.h"
#include "base_tables.h"
#include "date_time.h"
#include "payment_base.h"
#include "baggage_base.h"


namespace SEATS2
{
using BASIC::date_time::TDateTime;
using namespace SALONS2;
enum TSeatStep { sLeft, sRight, sUp, sDown };
enum TWhere { sLeftRight, sUpDown, sEveryWhere };
enum TSeatsType { stSeat, stReseat, stDropseat };
enum TChangeLayerFlags { flWaitList, flQuestionReseat, flSetPayLayer, flSyncCabinClass };
enum TChangeLayerProcFlag { procPaySeatSet,
                            procWaitList, /*�ਧ��� ⮣�, �� ���ᠤ�� ���� � ��*/
                            procSyncCabinClass };
enum TChangeLayerSeatsProps { propRFISC, changedOrNotPay, propRFISCQuestion };

/* ������ ��ᠤ�� ���ᠦ�஢
       sdUpDown_Row - ᢥ��� ���� � ��
       sdUpDown_Line - ᢥ��� ���� � �����
       sdDownUp_Row - ᭨�� ����� � ��
       sdDownUp_Line - ᭨�� ����� � �����
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
  ASTRA::TPerson pers_type;
  std::string cabin_clname;
  std::string placeName;
  std::string seat_descr;
  std::string wl_type;
  int countPlace;
  bool isSeat;
  std::string ticket_no;
  std::string document;
  int bag_weight;
  int bag_amount;
  std::string trip_from;
  std::string pass_rem;
  std::string comp_rem;
  bool pr_down;
  TDefaults() {
    grp_status = ASTRA::cltCheckin;
    pers_type = ASTRA::adult;
    cabin_clname = EncodeClass( ASTRA::Y );
    countPlace = 1;
    isSeat = true;
    bag_weight = 0;
    bag_amount = 0;
    pr_down = false;
  }
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

struct SeatsDescr {
  std::map<std::string,int> seatsDescr;
  void addSeatDescr( const std::vector<std::string> &v ) {
    clear();
    for ( auto i : v ) {
      addSeatDescr( i );
    }
  }
  void addSeatDescr( const std::string &seatDescr ) {
    std::map<std::string, int>::iterator it = seatsDescr.find( seatDescr );
    if ( it == seatsDescr.end() ) {
      seatsDescr.insert( make_pair(seatDescr,1) );
    }
    else {
      it->second++;
    }
  }
  bool findSeat( const std::string &seatDescr ) {
    std::map<std::string, int>::iterator it = seatsDescr.find( seatDescr );
    if ( it != seatsDescr.end() &&
         it->second > 0 ) {
      it->second--;
      return true;
    }
    return false;
  }

  void clear() {
    seatsDescr.clear();
  }

  std::string traceStr() const {
    std::ostringstream buf;
    buf << ",seatsDescr={" << toString("multi",true) << "}";
    return buf.str();
  }

  std::string toString( std::string format, bool trace=false ) const {
    if ( seatsDescr.empty() ) {
      return "";
    }
    std::ostringstream buf;
    bool first = true;
    for ( std::map<std::string, int>::const_iterator it=seatsDescr.begin(); it!=seatsDescr.end(); it++ ) {
      if ( !first ) {
       buf << ",";
      }
      first = false;
      if ( trace ) {
        buf << "(" << it->first << "," << it->second << ") ";
      }
      else {
        buf << it->first;
      }
      if ( format == "one" ) {
        break;
      }
    }
    return buf.str();
  }
};

struct TPassenger {
  private:
    std::vector<std::string> rems;
  public:
    /*�室*/
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
    SeatsDescr seatsDescr;
    bool isSeat;
    std::string wl_type;
    int countPlace;
    bool is_jmp;
    TSeatStep Step;
    std::string SUBCLS_REM;
    std::string maxRem;
    std::string placeRem; /* 'NSSA', 'NSSW', 'NSSB' � �. �. */
    bool prSmoke;
    std::string cabin_clname;
    //ASTRA::TCompLayerType layer; // ����� ���ᠦ�� �।�. ��ᠤ��, �஭�, ...
    ASTRA::TCompLayerType grp_status; // ����� ��㯯� � - �࠭��� ...
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
    TBagKilos excess_wt;
    TBagPieces excess_pc;
    std::string trip_from;
    std::string pass_rem;
    /*��室*/
    std::vector<TSeat> seat_no;
    SALONS2::TPlaceList *placeList; /* ᠫ�� */
    SALONS2::TPoint Pos; /* 㪠�뢠�� ���� */
    bool InUse;
    bool isValidPlace;
    TSeatTariffMapType tariffs;
    TSeatTariffMap::TStatus tariffStatus;
    TPassenger() :
      excess_wt(0), excess_pc(0)
    {
      regNo = -1;
      bag_weight = 0;
      bag_amount = 0;
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
    void add_rem( const std::string &code );
    void remove_rem( const std::string &code, const std::map<std::string, int> &remarks );
    void calc_priority( const std::map<std::string, int> &remarks );
    void get_remarks( std::vector<std::string> &vrems );
    bool isRemark( std::string code );
    bool is_valid_seats( const std::vector<SALONS2::TPlace> &places );
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
      if ( !cabin_clname.empty() ) {
        buf << "cabin_clname=" << cabin_clname << ",";
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
      if ( !excess_wt.zero() ) {
        buf << "excess_wt=" << excess_wt.view(AstraLocale::OutputLang(AstraLocale::LANG_EN), true) << ",";
      }
      if ( !excess_pc.zero() ) {
        buf << "excess_pc=" << excess_pc.view(AstraLocale::OutputLang(AstraLocale::LANG_EN), true) << ",";
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
        case TSeatTariffMap::TStatus::stNotFound:      buf << "stNotFound";      break;
        case TSeatTariffMap::TStatus::stNotOperating:  buf << "stNotOperating";  break;
        case TSeatTariffMap::TStatus::stNotET:         buf << "stNotET";         break;
        case TSeatTariffMap::TStatus::stUnknownETDisp: buf << "stUnknownETDisp"; break;
        case TSeatTariffMap::TStatus::stNotRFISC:      buf << "stNotRFISC";  break;
        case TSeatTariffMap::TStatus::stUseRFISC:      buf << "stUseRFISC";  break;
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
      buf << seatsDescr.traceStr();
      return buf.str();
    }
};

typedef std::vector<TPassenger> VPassengers;

class TPassengers {
  private:
    VPassengers FPassengers;
  public:
    std::map<std::string, int> remarks;
    TCounters counters;
    std::string cabin_clname;   // ����� � ����� �� ࠡ�⠥�
    bool KTube;
    bool KWindow;
    bool UseSmoke;
    bool SeatDescription;
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
    void sortByIndex();
    void operator = (TPassengers &items);
    bool withBaby();
    bool withCHIN();
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
    bool Alone; /* ��ᠤ�� ������ � ��� - ����७��� ��६����� - �� �ண��� */
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

/* ��� ���ᠭ� ���� ����㯭� �-樨 */
/* ��⮬���᪠� ���ᠤ�� ���ᠦ�஢ �� ��������� ���������� */
void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &APass, TSeatAlgoParams ASeatAlgoParams );
void AutoReSeatsPassengers( SALONS2::TSalonList &salonList,
                            const SALONS2::TIntArvSalonPassengers &passengers,
                            TSeatAlgoParams ASeatAlgoParams );
void SeatsPassengers( SALONS2::TSalonList &salonList,
                      TSeatAlgoParams ASeatAlgoParams,
                      ASTRA::TClientType client_type,
                      TPassengers &passengers,
                      SALONS2::TAutoSeats &seats );
BitSet<TChangeLayerSeatsProps>
     ChangeLayer( const SALONS2::TSalonList &salonList, ASTRA::TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  std::string first_xname, std::string first_yname, TSeatsType seat_type,
                  const BitSet<TChangeLayerProcFlag> &procFlags,
                  const std::string& whence, xmlNodePtr reqNode, xmlNodePtr resNode );
void SaveTripSeatRanges( int point_id, ASTRA::TCompLayerType layer_type, TSeatRanges &seats,
                         int pax_id, int point_dep, int point_arv, TDateTime time_create );
TSeatAlgoParams GetSeatAlgo(TQuery &Qry, std::string airline, int flt_no, std::string airp_dep);
bool IsSubClsRem( const std::string &airline, const std::string &subclass, std::string &rem );
bool isCheckinWOChoiceSeats( int point_id );

extern TPassengers Passengers;
} // end namespace SEATS2

#endif /*_SEATS2_H_*/

