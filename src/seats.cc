#include "seats.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_date_time.h"
#include "oralib.h"
#include "salons.h"
#include "comp_layers.h"
#include "seats_utils.h"
#include "images.h"
#include "serverlib/str_utils.h"
#include "tripinfo.h"
#include "aodb.h"
#include "term_version.h"
#include "alarms.h"
#include "passenger.h"
#include "rozysk.h"
#include "points.h"
#include "web_main.h"
#include "passenger.h"
#include "remarks.h"
#include "dcs_services.h"
#include "seat_descript.h"
#include "seat_number.h"
#include "flt_settings.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;

namespace SEATS2 //new terminal
{

  struct TExecProps {
    //boost::posix_time::ptime startTime;
    long long startTime;
    long long execTime;
    long count;
    TExecProps() {
      execTime = 0;
      count = 0;
    }
  };

  inline long long gettimeus()
  {
      struct timeval tv;
      gettimeofday( &tv, NULL );
      return (long long) tv.tv_sec * 1000000LL + (long long) tv.tv_usec;
  }

  template <typename T>
  class StatisticProps : public std::map<T, TExecProps>
  {
    private:
      bool active = true;
    public:
      void deactivate() {
        active = false;
      }

      void start(const T &elem)
      {
        if ( !active ) {
          return;
        }
        typename std::map<T, TExecProps>::iterator i = this->find(elem);
        if ( i == this->end() ) {
          i = this->insert(std::pair<T, TExecProps>(elem, TExecProps())).first;
        }
        i->second.startTime = -gettimeus();
        i->second.count++;
      }
      void stop(const T &elem)
      {
        if ( !active ) {
          return;
        }
         typename std::map<T, TExecProps>::iterator i = this->find(elem);
         if ( i != this->end() ) {
//            boost::posix_time::time_duration t = boost::posix_time::microsec_clock::local_time() - i->second.startTime;
//            i->second.execTime += t.total_milliseconds();
             i->second.execTime += i->second.startTime + gettimeus();
         }
      }

      long int count( const T &elem) const {
        typename std::map<T, int>::iterator i = this->find(elem);
        if ( i != this->end() ) {
          return i->second;
        }
        return 0;
      }

      long int execTime(const T &elem) const
      {
        typename std::map<T, TExecProps>::iterator i = this->find(elem);
        if ( i != this->end() ) {
          return i->second.execTime;
        }
        return 0;
      }
  };




const int PR_N_PLACE = 9;
const int PR_REMPLACE = 8;
const int PR_SMOKE = 7;

const int PR_REM_TO_REM = 100;
const int PR_REM_TO_NOREM = 90;
const int PR_EQUAL_N_PLACE = 50;
const int PR_EQUAL_REMPLACE = 20;
const int PR_EQUAL_SMOKE = 10;

const int CONST_MAXPLACE = 3;

StatisticProps<std::string> SeatsStat;


typedef vector<SALONS2::TPoint> TSeatCoords;

struct TSeatCoordsSalon {
  map<int,TSeatCoords> coords;
  SALONS2::TPlaceList *placeList;
  bool addSeat( TPlaceList *placeList, int x, int xlen, int y, bool pr_window, bool pr_tube, bool pr_seats );
  std::string toString() const;
};

typedef vector<TSeatCoordsSalon> vecSeatCoordsVars;

class TSeatCoordsClass {
  private:
    vecSeatCoordsVars vecSeatCoords;
    void clear();
    void addBaseSeat( TPlaceList* placeList, TPoint &p, std::set<int> &passCoord );
  public:
    void addBaseSeats( const std::vector<TCoordSeat> &paxsSeats );
    vecSeatCoordsVars &getSeatCoordsVars( );
    void refreshCoords( TSeatAlgoTypes ASeatAlgoType,
                        bool pr_window, bool pr_tube,
                        const std::vector<TCoordSeat> &paxsSeats );
    std::string toString();
};
enum TSeatAlg { sSeatGrpOnBasePlace, sSeatGrp, sSeatPassengers, seatAlgLength };
enum TUseRem { sAllUse, sMaxUse, sOnlyUse, sNotUse_NotUseDenial, sNotUse, sNotUseDenial, sIgnoreUse, useremLength };
/*
 aAllUse - ᮢ������� ��� ६�ப �� ᠬ��� �ਮ��⭮�� ���ᠦ���
 sMaxUse - ᮢ������� ᠬ�� �ਮ��⭮� ६�ન
 sOnlyUse - ��� �� ���� ᮢ������
 sNotUse - ����饭� �ᯮ�짮���� ���� � ࠧ�襭��� ६�મ�, �᫨ � ���ᠦ�� ⠪�� ६�ન ���
 sNotUseDenial - ����饭� �ᯮ�짮���� ���� � ����饭��� ६�મ�, ��� ���� � ���ᠦ��
 sIgnoreUse - �����஢��� ६�ન
*/
/* ����� ࠧ������ ���, ����� ᠦ��� �� ������ ����� ������ ࠧ�, �� ����� */
enum TUseAlone { uFalse3 /* ����� ��⠢���� ������ �� ��ᠤ�� ��㯯�*/,
                 uFalse1 /* ����� ��⠢���� ������ ⮫쪮 ���� ࠧ �� ��ᠤ�� ��㯯�*/,
                 uTrue /*����� ��⠢���� ������ �� ��ᠤ�� ��㯯� �� ���-�� ࠧ*/ };

string DecodeCanUseRems( TUseRem VCanUseRems )
{
    string res;
    switch( VCanUseRems ) {
        case sAllUse:
              res = "sAllUse";
              break;
        case sOnlyUse:
              res = "sOnlyUse";
              break;
        case sMaxUse:
              res = "sMaxUse";
              break;
      case sNotUse_NotUseDenial:
          res = "sNotUse_NotUseDenial";
          break;
        case sNotUseDenial:
              res = "sNotUseDenial";
              break;
        case sNotUse:
              res = "sNotUse";
              break;
        case sIgnoreUse:
              res = "sIgnoreUse";
              break;
        default:;
    }
    return res;
}

inline bool LSD( int G3, int G2, int G, int V3, int V2, TWhere Where );

struct CondSeats {
  bool SeatDescription;
  SeatsDescr p;
  SeatsDescr seatsDescr;
  CondSeats() {
    SeatDescription = false;
  }
  void SavePoint() {
    if ( !SeatDescription ) {
      return;
    }
    p.clear();
    LogTrace(TRACE5) << "CondSeats: SavePoint " <<  seatsDescr.traceStr();
    p.seatsDescr = seatsDescr.seatsDescr;
  }
  void Rollback() {
    if ( !SeatDescription ) {
      return;
    }
    seatsDescr.clear();
    seatsDescr.seatsDescr = p.seatsDescr;
    LogTrace(TRACE5) << "CondSeats: Rollback " <<  seatsDescr.traceStr();
  }
  bool isOk( ) {
    for ( auto i : seatsDescr.seatsDescr ) {
       if ( i.second > 0 ) {
         return false;
       }
    }
    return true;
  }

  bool findSeat( const std::string &seatDescr ) {
    if ( !SeatDescription ) {
      return true;
    }
    if ( seatsDescr.findSeat( seatDescr ) ) {
      LogTrace(TRACE5) << "CondSeats: find seat " << seatsDescr.traceStr();
      return true;
    }
    return false;
  }
};

/* �������� ��६���� ��� �⮣� ����� */
CondSeats condSeats;
TSeatPlaces SeatPlaces;

class CurrSalon {
private:
  static std::string airline;
  static BASIC_SALONS::TCompLayerTypes::Enum flag;
  static SALONS2::TSalons* FCurrSalon;
public:
  static void set( const std::string &_airline,
                   BASIC_SALONS::TCompLayerTypes::Enum _flag,
                   SALONS2::TSalons* S ) {
    FCurrSalon = S;
    airline = _airline;
    flag = _flag;
  }
  static SALONS2::TSalons& get( ){
    return *CurrSalon::FCurrSalon;
  }
  static std::string getAirline() {
    return CurrSalon::airline;
  }
  static BASIC_SALONS::TCompLayerTypes::Enum getFlag() {
    return CurrSalon::flag;
  }
  static int getPriority( ASTRA::TCompLayerType layer_type ) {
    return BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( airline, layer_type ), flag );
  }
};

std::string CurrSalon::airline = "";
BASIC_SALONS::TCompLayerTypes::Enum CurrSalon::flag = BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline;
SALONS2::TSalons* CurrSalon::FCurrSalon = nullptr;


TSeatCoordsClass seatCoords;

bool CanUseLayers; /* ���� �� ᫮� ���� */
bool CanUseMutiLayer; /* ���� �� ��㯯� ᫮�� */
TCompLayerType PlaceLayer; /* ᠬ ᫮� */
vector<TCompLayerType> SeatsLayers;
TUseLayers UseLayers; // ����� �� �ᯮ�짮���� ���� � ᫮�� ��� ��ᠤ�� ���ᠦ�஢ � ��㣨�� ᫮ﬨ

bool CanUseSmoke; /* ���� ������ ���� */

struct TAllowedAttributesSeat {
  vector<string> ElemTypes; /* ࠧ�襭�� ⨯� ���� ���� ���� �� ⨯� (⠡��⪠)*/
  bool pr_isWorkINFT; /* �� */
  int point_id;
  bool pr_INFT; /*�ਧ��� ��ᠤ�� �����㯯� INFT */
  TAllowedAttributesSeat() {
    point_id = ASTRA::NoExists;
  }
  void clearElems() {
    ElemTypes.clear();
  }
  void clearINFT() {
    pr_INFT = false;
  }
  void clearAll() {
    clearElems();
    clearINFT();
  }
  static void getValidChildElem_Types( vector<string> &elems ) {
    elems.clear();
    elems.push_back( "�" ); //!!! ����� ���� ࠧ�襭��� ����
    elems.push_back( "�" ); //!!!
  }
  void getValidChildElem_Types( ) {
    getValidChildElem_Types( ElemTypes );
  }

  bool isWorkINFT( int vpoint_id ) {
    SeatsStat.start(__FUNCTION__);
    point_id = vpoint_id;
    if ( TReqInfo::Instance()->client_type == ctTerm ||
         TReqInfo::Instance()->client_type == ctPNL ) {
      pr_isWorkINFT = false;
      SeatsStat.stop(__FUNCTION__);
      return pr_isWorkINFT;
    }
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if ( Qry.Eof ) {
      ProgError( STDLOG, "isWorkINFT: flight not found!!!, point_id=%d", point_id );
    }
    pr_isWorkINFT = ( !Qry.Eof &&
                      (string("��") == Qry.FieldAsString( "airline")/* || string("��") == Qry.FieldAsString( "airline")*/));
    SeatsStat.stop(__FUNCTION__);
    return pr_isWorkINFT;
  }

  bool passSeat( SALONS2::TPlace *place ) {
    return passSeat( ElemTypes, pr_INFT, *place );
  }
  bool passSeat( const vector<string> &elems,
                 bool _pr_INFT,
                 const SALONS2::TPlace &place ) {
    bool res = (elems.empty() || find( elems.begin(), elems.end(), place.elem_type ) != elems.end());
    if ( !pr_isWorkINFT || !res ) {
      return res;
    }
    bool pr_exists = false;
    if ( !place.rems.empty() ) {
      for ( std::vector<TRem>::const_iterator jrem=place.rems.begin(); jrem!=place.rems.end(); jrem++ ) {
        if ( string( "INFT" ) == jrem->rem ) {
          if ( !jrem->pr_denial ) {
            pr_exists = true;
          }
          break;
        }
      }
    }
    else {
      std::map<int, std::vector<TSeatRemark>,classcomp > remarks;
      place.GetRemarks( remarks );
      if ( remarks.find( point_id ) != remarks.end() ) {
        for ( std::vector<TSeatRemark>::iterator jrem=remarks[ point_id ].begin(); jrem!=remarks[ point_id ].end(); jrem++ ) {
          if ( string( "INFT" ) == jrem->value ) {
            if ( !jrem->pr_denial ) {
              pr_exists = true;
            }
            break;
          }
        }
      }
    }
    //ProgTrace( TRACE5, "(%d,%d),_pr_INFT=%d, pr_exists=%d",place.x,place.y,_pr_INFT,pr_exists);
    return ( _pr_INFT == pr_exists );
  }
  bool passSeats( const std::string &pers_type,
                  bool _pr_INFT,
                  const std::vector<SALONS2::TPlace> &places ) {
    vector<string> elems;
    if ( pers_type != "��" ) { // �஢�ઠ �� ���� � ���਩���� ��室�
      getValidChildElem_Types( elems );
    }
    for (std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
      if ( !passSeat( elems, _pr_INFT, *ipl ) ) {
        return false;
      }
    }
    return true;
  }
};

TAllowedAttributesSeat AllowedAttrsSeat;

TUseRem CanUseRems; /* ���� �� ६�થ */
vector<string> Remarks; /* ᠬ� ६�ઠ */
bool CanUseTube; /* ���� �१ ��室� */
TUseAlone CanUseAlone; /* ����� �� �ᯮ�짮���� ��ᠤ�� ������ � ��� - ����� ��ᠤ���
                          ��㯯� ��� �� ��㣮� */
TSeatAlg SeatAlg;
bool FindSUBCLS=false; // ��室�� �� ⮣�, �� � ��㯯� �� ����� ���� ���ᠦ�஢ � ࠧ�묨 �������ᠬ�!
bool canUseSUBCLS=false;
string SUBCLS_REM;

bool canUseOneRow=true; // �ᯮ�짮���� �� ��ᠤ�� ⮫쪮 ���� �� - ��।������ �� �᭮�� �-樨 getCanUseOneRow() � 横��

struct RateCompare {
  bool operator() ( const TSeatTariff &rate1, const TSeatTariff &rate2 ) const
  {
      if (rate1.rate != rate2.rate)
        return rate1.rate < rate2.rate;
      if (rate1.color != rate2.color)
        return rate1.color < rate2.color;
      return rate1.currency_id < rate2.currency_id;
  }
};


struct TCondRate {
  bool pr_web; //�ਧ��� ⮣�, �� ॣ��������� ����� ���ᠦ��
  bool use_rate; // �ᯮ�짮���� ����� ���� ��� ���ᠦ�஢ ��� ����祭��� ����
  bool ignore_rate; // ������㥬 ���
  set<SALONS2::TSeatTariff,RateCompare> rates;
  set<SALONS2::TSeatTariff,RateCompare>::iterator current_rate;
  TCondRate( ) {
    current_rate = rates.end();
  }
/*  double convertUniValue( TSeatTariff &rate ) {
    return rate.rate;
  }*/
  void Init( SALONS2::TSalons &Salons, bool apr_pay, TClientType client_type ) {
    SeatsStat.start(__FUNCTION__);
    pr_web = apr_pay;
    use_rate = ( client_type == ctTerm || client_type == ctPNL ||
                 isCheckinWOChoiceSeats( Salons.trip_id ) );
    ProgTrace( TRACE5, "TCondRate::Init use_rate=%d", use_rate);
    rates.clear();
    ignore_rate = false;
    rates.insert( TSeatTariff( "", 0.0, "" ) ); // �ᥣ�� ������ - ����砥�, �� ���� �ᯮ�짮���� ���� ��� ���
    rates.insert( TSeatTariff( "", INT_MAX, "" ) );
    //!!!rates[ 0 ].rate = 0.0; // �ᥣ�� ������ - ����砥�, �� ���� �ᯮ�짮���� ���� ��� ���
    if ( pr_web ) {// �� ���뢠�� ����� ����
      SeatsStat.stop(__FUNCTION__);
      return;
    }
    SALONS2::TPlaceList *placeList;
    //!!!!map<double,int> vars;
    for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
          plList!=Salons.placelists.end(); plList++ ) {
      placeList = *plList;
      for ( IPlace i=placeList->places.begin(); i!=placeList->places.end(); i++ ) {
        if ( i->SeatTariff.empty() || !i->visible || !i->isplace )
          continue;
        if ( rates.find( i->SeatTariff ) != rates.end() ) {
          continue;
        }
        rates.insert( i->SeatTariff );
    /* !!!   double univalue = convertUniValue( i->SeatTariff );
        if ( vars.find( univalue ) == vars.end() ) { // �� ��諨
          bool pr_ins = false;
          for ( map<double,int>::iterator p=vars.begin(); p!=vars.end(); p++ ) {
            if ( univalue < p->first ) {
              if ( !pr_ins ) {
                pr_ins = true;
                vars[ univalue ] = p->second;
                for ( int k=vars.size(); k >p->second; k-- )
                  rates[ k ] = rates[ k - 1 ];
                rates[ p->second ] = i->SeatTariff;
              }
              vars[ p->first ] = vars[ p->first ] + 1;
            }
          }
          if ( !pr_ins )
           vars[ univalue ] = vars.size();
           rates[ vars[ univalue ] ] = i->SeatTariff;
        }*/
      }
    }
    current_rate = rates.begin();
    for ( set<TSeatTariff,RateCompare>::iterator i=rates.begin(); i!=rates.end(); i++ ) {
      ProgTrace( TRACE5, "rates.value=%s", i->str().c_str() );
    }
    SeatsStat.stop(__FUNCTION__);
  }
  bool current_rate_end() {
    return current_rate == rates.end() || current_rate->rate == INT_MAX;
  }

  bool CanUseRate( TPlace *place ) { /* �᫨ �� �������� ���� ���஡����� �� ��ᠤ�� � �� ᬮ��� ��ᠤ��� ��� ��� ��䮢 �� ३� ��� ���� ��� ���, � ����� �ᯮ�짮���� */
    SeatsStat.start(__FUNCTION__);
    bool res = ( pr_web || (current_rate_end() && use_rate) /*!!!|| place->SeatTariff.empty()*/ || ignore_rate );
//    ProgTrace( TRACE5, "CanUseRate: x=%d, y=%d, place->SeatTariff=%s, res=%d,use_rate=%d, curr rate=%s",
//               place->x, place->y, place->SeatTariff.str().c_str(), res, use_rate, current_rate->str().c_str() );

    if ( !res ) {
      for ( set<TSeatTariff,RateCompare>::iterator i=rates.begin(); ; i++ ) { // ��ᬮ�� ��� ��䮢, ���. ���஢��� � ���浪� �����⠭�� �ਮ��� �ᯮ�짮�����
        if ( i != rates.end() &&
             i->rate == place->SeatTariff.rate &&
            ( (i->color.empty() && !place->SeatTariff.currency_id.empty()) || //梥� �� �����  0.0 � � ���� ����� ��� (�� �������⥭ !place->SeatTariff.currency_id.empty() )
              (i->color == place->SeatTariff.color && i->currency_id == place->SeatTariff.currency_id) ) ) {
          if ( use_rate || i->rate == 0.0 ) {
            res = true;
          }
          break;
        }
        if ( i == current_rate ) // ����� ���� ��� �� ����㯭�
          break;
      }
    }
    SeatsStat.stop(__FUNCTION__);
    return res;
  }
  bool isIgnoreRates( ) {
    set<TSeatTariff,RateCompare>::iterator i = rates.end();
    if ( !rates.empty() )
      i--;
    return ( (current_rate_end() && use_rate) || i == current_rate || ignore_rate);
  }
};

TCondRate condRates;

TSeatAlgoParams FSeatAlgoParams; // ������ ��ᠤ��


void GetUseLayers( TUseLayers &uselayers )
{
  uselayers.clear();
}

bool CanUseLayer( const TCompLayerType &layer, TUseLayers FUseLayers )
{
  return ( FUseLayers.find( layer ) != FUseLayers.end() && FUseLayers[ layer ] );
}

int MAXPLACE() {
    return (canUseOneRow)?1000:CONST_MAXPLACE; // �� ��ᠤ�� � ���� �� ���-�� ���� � ��� ����࠭�祭� ���� 3 ����
}

bool getCanUseOneRow() {
    return FSeatAlgoParams.pr_canUseOneRow;
}

bool TSeatCoordsSalon::addSeat( TPlaceList *placeList, int x, int xlen, int y,
                                bool pr_window, bool pr_tube, bool pr_seats )
{
  SALONS2::TPoint p( x, y );
  TPlace *place = placeList->place( p );
  if ( !place->isplace ||
       !place->visible ||
       CurrSalon::get().isExistsOccupySeat( placeList->num, x, y ) ) {
    return false;
  }
  int prioritySeats = 0;
  if ( !pr_seats ) {
    if ( pr_window ) {
      if ( x == 0 || x == xlen - 1 ) {
        prioritySeats = prioritySeats + 1;
      }
      else {
        prioritySeats = prioritySeats + 2;
      }
    }
    else {
      if ( pr_tube ) {
        if ( ( x - 1 >= 0 && placeList->GetXsName( x - 1 ).empty() ) ||
             ( x + 1 <= xlen - 1 && placeList->GetXsName( x + 1 ).empty() ) ) {
          prioritySeats = prioritySeats + 1;
        }
        else {
          prioritySeats = prioritySeats + 2;
        }
      }
      else {
        prioritySeats = prioritySeats + 2;
      }
    }
  }
  map<int,TSeatCoords>::iterator icoord = coords.find( prioritySeats );
  if ( icoord == coords.end() ) {
    icoord = coords.insert( make_pair(prioritySeats,TSeatCoords()) ).first;
  }
  icoord->second.push_back( p );
  return true;
}

std::string TSeatCoordsSalon::toString() const
{
  string res;
  res += "placelist num=" + IntToString( placeList->num );
  for (  map<int,TSeatCoords>::const_iterator i=coords.begin(); i!=coords.end(); i++ ) {
    res += ",priority=" + IntToString( i->first ) + ",size=" + IntToString( i->second.size() ) + '\n';
  }
  return res;
}
void TSeatCoordsClass::addBaseSeat( TPlaceList* placeList, TPoint &p, std::set<int> &passCoord )
{
  if ( !placeList->ValidPlace( p ) ) {
    return;
  }
  int key = getSeatKey::get( placeList->num, p.x, p.y );
  if ( passCoord.find( key ) != passCoord.end() ) {
    return;
  }
  passCoord.insert( key );
  /*???for ( vecSeatCoordsVars::iterator ilist=vecSeatCoords.begin(); ilist!=vecSeatCoords.end(); ilist++ ) {
    if ( ilist->placeList->num == placeList->num ) {
      if ( ilist->addSeat( placeList, p.x, 0, p.y, false, false, true ) ) {
        ProgTrace( TRACE5, "addBaseSeat: x=%d, y=%d", p.x, p.y );
      }
      return;
    }
  }*/
  TSeatCoordsSalon coordSalon;
  coordSalon.placeList = placeList;
  //LogTrace(TRACE5) << coordSalon.placeList->num;
  if ( coordSalon.addSeat( placeList, p.x, 0, p.y, false, false, true ) ) {
    ProgTrace( TRACE5, "addBaseSeat: x=%d, y=%d", p.x, p.y );
  }
  vecSeatCoords.emplace( vecSeatCoords.begin(), coordSalon );
}

bool SortPaxSeats( TCoordSeat item1, TCoordSeat item2 )
{
  return (
           getSeatKey::get( item1.placeListIdx, item1.p.x, item1.p.y ) >
           getSeatKey::get( item2.placeListIdx, item2.p.x, item2.p.y )
          );
}

void TSeatCoordsClass::addBaseSeats( const std::vector<TCoordSeat> &paxSeats )
{
  std::vector<TCoordSeat> paxS = paxSeats;
  sort( paxS.begin(), paxS.end(), SortPaxSeats );
  TSeatCoordsSalon passCoordSeats;
  std::set<int> passCoord;
  for ( int step=0; step<2; step++ ) {
    for ( std::vector<TCoordSeat>::const_iterator iseat=paxS.begin(); iseat!=paxS.end(); iseat++ ) {
      //��室�� ᢮����� ���� �����
      for ( std::vector<TPlaceList*>::iterator item=CurrSalon::get().placelists.begin(); item!=CurrSalon::get().placelists.end(); item++ ) {
        if ( iseat->placeListIdx == (*item)->num ) {
          int xlen = (*item)->GetXsCount();
          TPoint p1 = iseat->p;
          if ( step == 0 ) {
            p1.x++;
            if ( p1.x <= xlen - 1 && (*item)->GetXsName( p1.x ).empty() ) {
              p1.x++;
              addBaseSeat( *item, p1, passCoord );
            }
            else {
              addBaseSeat( *item, p1, passCoord );
            }
            p1 = iseat->p;
            p1.x--;
            if ( p1.x >= 0 && (*item)->GetXsName( p1.x ).empty() ) {
              p1.x--;
              addBaseSeat( *item, p1, passCoord );
            }
            else {
              addBaseSeat( *item, p1, passCoord );
            }
          }
          if ( step == 1 ) {
            p1 = iseat->p;
            p1.y--;
            addBaseSeat( *item, p1, passCoord );
            p1 = iseat->p;
            p1.y++;
            addBaseSeat( *item, p1, passCoord );
          }
          break;
        }
      }
    }
  }
}

vecSeatCoordsVars &TSeatCoordsClass::getSeatCoordsVars( )
{
    return vecSeatCoords;
}

void TSeatCoordsClass::clear() {
  vecSeatCoords.clear();
}

void TSeatCoordsClass::refreshCoords( TSeatAlgoTypes ASeatAlgoType,
                                      bool pr_window, bool pr_tube,
                                      const std::vector<TCoordSeat> &paxsSeats )
{
  SeatsStat.start(__FUNCTION__);
//  LogTrace(TRACE5) << paxsSeats.size();
  clear();
  int xlen, ylen;
  // ᢥ��� ����
  bool pr_UpDown = ( ASeatAlgoType == sdUpDown_Row || ASeatAlgoType == sdUpDown_Line );
  if ( pr_UpDown ) {
    for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon::get().placelists.begin();
          iplaceList!=CurrSalon::get().placelists.end(); iplaceList++ ) {
      TSeatCoordsSalon coordSalon;
      coordSalon.placeList = *iplaceList;
      xlen = (*iplaceList)->GetXsCount();
      ylen = (*iplaceList)->GetYsCount();
      if ( ASeatAlgoType == sdUpDown_Line ) {
        for ( int x=0; x<xlen; x++ ) {
          if ( (*iplaceList)->GetXsName( x ).empty() )
            continue;
          for ( int y=0; y<ylen; y++ ) {
            coordSalon.addSeat( *iplaceList, x, xlen, y, pr_window, pr_tube, false );
          } // end for ys
        } // end for xs
      } // end if
      else {
        for ( int y=0; y<ylen; y++ ) {
          for ( int x=0; x<xlen; x++ ) {
            if ( (*iplaceList)->GetXsName( x ).empty() )
              continue;
            coordSalon.addSeat( *iplaceList, x, xlen, y, pr_window, pr_tube, false );
          } // end for xs
        } // end for ys
      }
      vecSeatCoords.push_back( coordSalon );
    } // end for placelists
  }
  else { // ᭨�� �����
    for ( vector<SALONS2::TPlaceList*>::reverse_iterator iplaceList=CurrSalon::get().placelists.rbegin();
          iplaceList!=CurrSalon::get().placelists.rend(); iplaceList++ ) {
      TSeatCoordsSalon coordSalon;
      coordSalon.placeList = *iplaceList;
      xlen = (*iplaceList)->GetXsCount();
      ylen = (*iplaceList)->GetYsCount();
      if ( ASeatAlgoType == sdDownUp_Line ) {
        for ( int x=0; x<xlen; x++ ) {
          if ( (*iplaceList)->GetXsName( x ).empty() )
            continue;
          for ( int y=ylen-1; y>=0; y-- ) {
            coordSalon.addSeat( *iplaceList, x, xlen, y, pr_window, pr_tube, false );
          } // end for ys
        } // end for xs
      } // end if
      else {
        for ( int y=ylen-1; y>=0; y-- ) {
          for ( int x=0; x<xlen; x++ ) {
            if ( (*iplaceList)->GetXsName( x ).empty() )
              continue;
            coordSalon.addSeat( *iplaceList, x, xlen, y, pr_window, pr_tube, false );
          } // end for xs
        } // end for ys
      }
      vecSeatCoords.push_back( coordSalon );
    }
  }
  addBaseSeats( paxsSeats );
  //ProgTrace( TRACE5, "vecSeatCoords=%s", toString().c_str() );
  SeatsStat.stop(__FUNCTION__);
}

std::string TSeatCoordsClass::toString()
{
  ostringstream res;
  res << '\n' << "TSeatCoordsClass size=" << vecSeatCoords.size() << '\n';
  for ( vecSeatCoordsVars::const_iterator i=vecSeatCoords.begin(); i!=vecSeatCoords.end(); i++ ) {
    res << i->toString();
  }
  return res.str();
}

TCounters::TCounters()
{
  Clear();
}

void TCounters::Clear()
{
  p_Count_3G = 0;
  p_Count_2G = 0;
  p_CountG = 0;
  p_Count_3V = 0;
  p_Count_2V = 0;
}

int TCounters::p_Count_3( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_Count_3G;
  else
    return p_Count_3V;
}

int TCounters::p_Count_2( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_Count_2G;
  else
    return p_Count_2V;
}

int TCounters::p_Count( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_CountG;
  else
    return 0;
}

void TCounters::Set_p_Count_3( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_3G = Count;
  else
    p_Count_3V = Count;
}

void TCounters::Set_p_Count_2( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_2G = Count;
  else
    p_Count_2V = Count;
}

void TCounters::Set_p_Count( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_CountG = Count;
}

void TCounters::Add_p_Count_3( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_3G += Count;
  else
    p_Count_3V += Count;
}

void TCounters::Add_p_Count_2( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_2G += Count;
  else
    p_Count_2V += Count;
}

void TCounters::Add_p_Count( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_CountG += Count;
}

TSeatPlaces::TSeatPlaces(  )
{
  Clear();
}

TSeatPlaces::~TSeatPlaces()
{
  Clear();
}

void TSeatPlaces::Clear()
{
  grp_status = cltUnknown;
  separately_seats_adults_crs_pax_id = ASTRA::NoExists;
  seatplaces.clear();
  counters.Clear();
}

/* �⪠� ��� ��������� ���� ��� ��ᠤ�� */
void TSeatPlaces::RollBack( int Begin, int End )
{
  if ( seatplaces.empty() )
    return;
  SeatsStat.start(__FUNCTION__);
  /* �஡�� �� ��������� �������� ���⠬ */
  TSeatPlace seatPlace;
  for ( int i=Begin; i<=End; i++ ) {
    seatPlace = seatplaces[ i ];
    /* �஡�� �� ���� ��࠭���� ���⠬ */
    for ( vector<SALONS2::TPlace>::iterator place=seatPlace.oldPlaces.begin();
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* ����祭�� ���� */
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      SALONS2::TPlace *pl = seatPlace.placeList->place( idx );
      /* �⬥�� ��� ��������� ���� */
      *pl = *place;
      if ( CurrSalon::get().canAddOccupy( pl ) ) {
        CurrSalon::get().AddOccupySeat( seatPlace.placeList->num, pl->x, pl->y );
      }
      else {
        CurrSalon::get().RemoveOccupySeat( seatPlace.placeList->num, pl->x, pl->y );
      }
      switch( seatPlace.Step ) {
        case sRight: seatPlace.Pos.x++;
                     break;
        case sLeft:  seatPlace.Pos.x--;
                     break;
        case sDown:  seatPlace.Pos.y++;
                     break;
        case sUp:    seatPlace.Pos.y--;
                     break;
      }
    } /* end for */
    switch ( (int)seatPlace.oldPlaces.size() ) {
      case 3: counters.Add_p_Count_3( -1, seatPlace.Step );
              break;
      case 2: counters.Add_p_Count_2( -1, seatPlace.Step );
              break;
      case 1: counters.Add_p_Count( -1, seatPlace.Step );
              break;
      default: throw EXCEPTIONS::Exception( "�訡�� ��ᠤ��" );
    }
    seatPlace.oldPlaces.clear();
  } /* end for */
  vector<TSeatPlace>::iterator b = seatplaces.begin();
  seatplaces.erase( b + Begin, b + End + 1 );
  SeatsStat.stop(__FUNCTION__);
}

/* �⪠� ��� ��������� ���� ��� ��ᠤ�� */
void TSeatPlaces::RollBack( )
{
  if ( seatplaces.empty() )
    return;
  RollBack( 0, seatplaces.size() - 1 );
}

void TSeatPlaces::Add( TSeatPlace &seatplace )
{
  seatplaces.push_back( seatplace );
}

/* ����ᨬ �������� ����.
  FP - 㪠��⥫� �� ��室��� ���� �� ���ண� ��稭����� ����
  EP - 㪠��⥫� �� ��ࢮ� ��������� ����
  FoundCount - ���-�� ��������� ����
  Step - ���ࠢ����� ����� ��������� ����
  �����頥� ���-�� �ᯮ�짮������ ���� */
int TSeatPlaces::Put_Find_Places( SALONS2::TPoint FP, SALONS2::TPoint EP, int foundCount, TSeatStep Step )
{
  SeatsStat.start(__FUNCTION__);
  int p_RCount = 0, p_RCount2 = 0, p_RCount3 = 0; /* ����室���� ���-�� 3-�, 2-�, 1-� ���� */
  int pp_Count = 0, pp_Count2 = 0, pp_Count3 = 0; /* ����饥�� ���-�� 3-�, 2-�, 1-� ���� */
  int NTrunc_Count = 0, Trunc_Count = 0; /* ���-�� �뤥������ �� ��饣� �᫠ ������ ���� */
  int p_Prior = 0, p_Next = 0; /* ���-�� ���� �� FP � ��᫥ ���� */
  int p_Step = 0; /* ���ࠢ����� ��ᠤ��. ��।������ � ����ᨬ��� �� ���-�� p_Prior � p_Next */
  int Need = 0;
  SALONS2::TPlaceList *placeList;
  int Result = 0; /* ��饥 ���-�� ������⢮������ ���� */
  if ( foundCount == 0 ) {
   SeatsStat.stop(__FUNCTION__);
   return Result; // �� ������ ����
  }
  placeList = CurrSalon::get().CurrPlaceList();
  switch( (int)Step ) {
    case sLeft:
    case sRight:
           pp_Count = counters.p_Count( sRight );
           pp_Count2 = counters.p_Count_2( sRight );
           pp_Count3 = counters.p_Count_3( sRight );
           p_Prior = abs( FP.x - EP.x );
           break;
    case sDown:
    case sUp:
           pp_Count = counters.p_Count( sDown );
           pp_Count2 = counters.p_Count_2( sDown );
           pp_Count3 = counters.p_Count_3( sDown );
           p_Prior = abs( FP.y - EP.y ); // ࠧ��� �/� FP � EP
           break;
  }
  p_RCount = Passengers.counters.p_Count( Step ) - pp_Count;
  p_RCount2 = Passengers.counters.p_Count_2( Step ) - pp_Count2;
  p_RCount3 = Passengers.counters.p_Count_3( Step ) - pp_Count3;
  p_Next = foundCount - p_Prior;
  p_Step = 0;
  NTrunc_Count = foundCount;
  while ( foundCount > 0 && p_RCount + p_RCount2 + p_RCount3 > 0 ) {
    switch ( NTrunc_Count ) {
      case 1: if ( p_RCount > 0 ) /* �㦭� 1-� ����� ���� ?*/
                Trunc_Count = 1;
              else
                Trunc_Count = 0;
              break;
      case 2: if ( p_RCount2 > 0 ) /* �㦭� 2-� ����� ���� ?*/
                Trunc_Count = 2;
              else
                Trunc_Count = 1;
              break;
      case 3: if ( p_RCount3 > 0 ) /* �㦭� 3-� ����� ���� ?*/
                Trunc_Count = 3; /* �㦭� 3-� ����� ���� */
              else
                Trunc_Count = 2; /* �㦭� 2-� ����� ���� ? */
              break;
      default: Trunc_Count = 3;
    }
    if ( !Trunc_Count ) /* ����� ��祣� �� �㦭� �� ⮣�, �� �।�������� */
      break;
    if ( Trunc_Count != NTrunc_Count ) {
      NTrunc_Count = Trunc_Count; /* �뤥���� ������� ����, � � ⠪�� ������ �� �㦭� */
      continue;
    }
   /* �᫨ �� �����, � ⮣�� �� ����� ����, ����� ��� ���ࠨ���� �� ���-��
      ⥯��� ��। ���� �⮨� �����, � ������ ���� ����� ��ᠤ��, �᫨ ���� �����,
      祬 �� ᥩ�� �뤥���?
      �⢥�: ��� ⮣�, �⮡� �������� ���� ������� � ��室��� �窥 FP
      ��筥� � ⠪��� ���, � ���ண� ����� ���� �� FP. */
    if ( !p_Step ) { /* �ਧ��� ⮣�, �� �� ���� �� ��।����� � ������ ��� ���� ��稭��� */
      Need = p_RCount; /* �� �㦤����� � ⠪�� ���-�� 1-� ���� */
      switch( Trunc_Count ) {
        case 3: Need += p_RCount3*3 + p_RCount2*2; /* �᫨ ���� 3, � ���뢠�� �� */
                break;
        case 2: Need += p_RCount2*2; /* �᫨ ���� 2 => � �� ���� 3-� ��� ��饥 ���-�� ���� �� ��������, */
                break;               /* ⮣�� ���� ���뢠�� ⮫쪮 1-� � 2-� ���� */
      }


      /* Need - �� ��饥 �㦭�� ���-�� ����, ���஥ �������� 㤠����� �뤥����
         ⥯��� ��।��塞 ���ࠢ����� */
      if ( p_Prior >= p_Next && Need > p_Next ) { /* ᫥�� ����� ����, ��筥� �ࠢ� */
        p_Step = -1;
        if ( p_Next < Need ) /* ���ࠢ�/���� ����� ����, 祬 �������� ����������� */
          Need = p_Next; /* ⮣�� ����� �� �� ����� ���ॡ�����! */
        switch( (int)Step ) {
          case sRight:
          case sLeft:
             EP.x = FP.x + Need - 1; /* ��६�頥��� �� ��室��� ������ */
             Step = sLeft; /* ��稭��� ��������� ����� */
             break;
          case sDown:
          case sUp:
             EP.y = FP.y + Need - 1; /* ��६�頥��� �� ��室��� ������ */
             Step = sUp; /* ��稭��� ��������� ����� */
             break;
        }
      }
      else {
       p_Step = 1;  /* ��筥� ᫥�� */
       if ( Need <= p_Next ) {
         Need = 0; /* ��祣� �� �ண��� � ��稭��� � ⥪�饩 ����樨 */
       }
       else {
         if ( p_Prior < Need )
           Need = p_Prior;
       }
       switch( (int)Step ) {
         case sRight:
         case sLeft:
            EP.x = FP.x - Need; /* ��६�頥��� �� ��室��� ������ */
            Step = sRight; /* ��稭��� ��������� ��ࠢ� */
            break;
         case sDown:
         case sUp:
            EP.y = FP.y - Need; /* ��६�頥��� �� ��室��� ������ */
            Step = sDown; /* ��稭��� ��������� ���� */
            break;
       }
      } /* ����� ��।������ ���ࠢ����� ( p_Prior >= p_Next ... ) */
    } /* ����� p_Step = 0 */

    /* �����稫� ��।����� ��室��� ������ ���� � ���ࠢ����� ��ᠤ */

    /* �뤥�塞 �� ���� ���� ��� ����� ���� */
    TSeatPlace seatplace;
    seatplace.placeList = placeList;
    seatplace.Step = Step;
    seatplace.Pos = EP;
    switch( (int)Step ) {
      case sLeft:
         seatplace.Step = sRight;
         seatplace.Pos.x = seatplace.Pos.x - Trunc_Count + 1;
         break;
      case sUp:
         seatplace.Step = sDown;
         seatplace.Pos.y = seatplace.Pos.y - Trunc_Count + 1;
         break;
    }
    /* ��࠭塞 ���� ���� � ����砥� ���� � ᠫ��� ��� ������ */
    for ( int i=0; i<Trunc_Count; i++ ) {
      SALONS2::TPlace place;
      SALONS2::TPlace *pl = placeList->place( EP );
      place = *pl;
      if ( !CurrSalon::get().placeIsFree( &place ) || !place.isplace ) {
        throw EXCEPTIONS::Exception( "���ᠤ�� �믮����� �������⨬�� ������: �ᯮ�짮����� 㦥 ����⮣� ����" );
      }
      seatplace.oldPlaces.push_back( place );
      pl->AddLayerToPlace( grp_status, 0, separately_seats_adults_crs_pax_id, NoExists, NoExists,
                           CurrSalon::getPriority( grp_status ) );
      if ( CurrSalon::get().canAddOccupy( pl ) ) {
        CurrSalon::get().AddOccupySeat( placeList->num, pl->x, pl->y );
      }
      switch( Step ) {
        case sRight:
           EP.x++;
           break;
        case sLeft:
           EP.x--;
           break;
        case sDown:
           EP.y++;
           break;
        case sUp:
           EP.y--;
           break;
      }
    } /* �����稫� ��࠭��� */
    Add( seatplace );
    switch( Trunc_Count ) {
      case 3:
         p_RCount3--;
         pp_Count3++;
         counters.Set_p_Count_3( pp_Count3, Step );
         break;
      case 2:
         p_RCount2--;
         pp_Count2++;
         counters.Set_p_Count_2( pp_Count2, Step );
         break;
      case 1:
         p_RCount--;
         pp_Count++;
         counters.Set_p_Count( pp_Count, Step );
         break;
    }
    Result += Trunc_Count; /* ���-�� 㦥 �ᯮ�짮������ ���� */
    foundCount -= Trunc_Count; /* ��饥 ���-�� �� �ᯮ�짮������ ���� */
    NTrunc_Count = foundCount; /* ���-�� ����, ����� ᥩ�� ����� ࠧ������� */
//    ProgTrace( TRACE5, "Result=%d", Result );
  } /* end while */
  SeatsStat.stop(__FUNCTION__);
  return Result;
}

/* �-�� ��� ��।������ ���������� ��ᠤ�� ��� ���� � ������ ���� ����饭�� ६�ન */
bool DoNotRemsSeats( const vector<SALONS2::TRem> &rems )
{
    bool res = false; // �ਧ��� ⮣�, �� � ���ᠦ�� ���� ६�ઠ, ����� ����饭� �� ��࠭��� ����
    bool pr_passsubcls = false;
    // ��।��塞 ���� �� � ���ᠦ�� ६�ઠ ��������
    //unsigned int i=0;
    vector<string>::iterator yrem=Remarks.begin();
    for (; yrem!=Remarks.end(); yrem++ ) {
        if ( isREM_SUBCLS( *yrem ) ) {
            pr_passsubcls = true;
            break;
        }
    }

/* old version	for ( ; i<sizeof(TSubcls_Remarks)/sizeof(const char*); i++ ) {
        if ( find( Remarks.begin(), Remarks.end(), string(TSubcls_Remarks[i]) ) != Remarks.end() ) { // � ���ᠦ�� SUBCLS
            pr_passsubcls = true;
            break;
      }
    }*/

    bool no_subcls = false;
    // ��।��塞 ���� �� ����饭��� ६�ઠ �� ����, ���. ������ � ���ᠦ��
    for( vector<string>::iterator nrem=Remarks.begin(); nrem!=Remarks.end(); nrem++ ) {
      for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
          if ( !prem->pr_denial )
              continue;
            if ( *nrem == prem->rem && !res )
                res = true;
      }
    }
    // �஡�� �� ६�ઠ� ����, �᫨ ���� ६�ઠ ��������
  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
      if ( !prem->pr_denial && isREM_SUBCLS( prem->rem ) && (!pr_passsubcls || *yrem != prem->rem ) ) {
          no_subcls = true;
          break;
      }
    }
/*  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
    for ( unsigned int j=0; j<sizeof(TSubcls_Remarks)/sizeof(const char*); j++ ) {
      if ( !prem->pr_denial && prem->rem == TSubcls_Remarks[j] && (!pr_passsubcls || TSubcls_Remarks[j] != TSubcls_Remarks[i]) ) {
          no_subcls = true;
          break;
        }
      }
    }*/
  return res || no_subcls; // �᫨ ���� ����饭��� ६�ઠ � ���ᠦ�� ��� ���� � ६�મ� SUBCLS, � � ���ᠦ�� �� ���
}

bool VerifyUseLayer( TPlace *place )
{
    bool res = !CanUseLayers || ( PlaceLayer == cltUnknown && place->layers.empty() );
    if ( !res ) {
        for (std::vector<TPlaceLayer>::iterator i=place->layers.begin(); i!=place->layers.end(); i++ ) {
            if ( find( SeatsLayers.begin(), SeatsLayers.end(), i->layer_type ) == SeatsLayers.end() ) { // ������ ����� �ਮ���� ᫮�, ���ண� ��� � ᯨ᪥ ����������� ᫮��
                break;
          }
          if ( CanUseMutiLayer || i->layer_type == PlaceLayer ) { // ������ �㦭� ᫮�
            res = true;
            break;
          }
      }
    }
    return res;
}

/* ���� ���� �ᯮ�������� �冷� ��᫥����⥫쭮
   �����頥� ���-�� ��������� ����.
   FP - ���� ����, � ���ண� ���� �᪠��
   FoundCount - ���-�� 㦥 ��������� ����
   Step - ���ࠢ����� ���᪠
  �������� ��६����:
   CanUselayer, Placelayer - ���� ��ண� �� ������ ����,
   CanUseSmoke - ���� ������ ����,
   AllowedAttrsSeat.ElemTypes - ���� ��ண� �� ⨯� ���� (⠡��⪠),
   AllowedAttrsSeat.Remarks - ���� ��ண� �� INFT
   CanUseRem, PlaceRem - ���� ��ண� �� ६�થ ���� */
int TSeatPlaces::FindPlaces_From( SALONS2::TPoint FP, int foundCount, TSeatStep Step )
{
  SeatsStat.start(__FUNCTION__);
  int Result = 0;
  SALONS2::TPlaceList *placeList = CurrSalon::get().CurrPlaceList();
  if ( !placeList->ValidPlace( FP ) ) {
    SeatsStat.stop(__FUNCTION__);
    return Result;
  }
  SALONS2::TPoint EP = FP;
  SALONS2::TPlace *place = placeList->place( EP );
  vector<SALONS2::TRem>::iterator prem;
  vector<string>::iterator irem;
/*  if ( SeatAlg == 1 && !canUseSUBCLS && CanUseRems == sNotUse_NotUseDenial )
      ProgTrace( TRACE5, "sNotUse_NotUseDenial CurrSalon->placeIsFree( place )=%d,place->isplace=%d,place->visible=%d,Passengers.clname=%s, VerifyUseLayer( place )=%d, condRates.CanUseRate( place )=%d,  AllowedAttrsSeat.passSeat( place )=%d",
                 CurrSalon->placeIsFree( place ),place->isplace,place->visible,Passengers.clname.c_str(), VerifyUseLayer( place ), condRates.CanUseRate( place ),  AllowedAttrsSeat.passSeat( place ) );*/
  condSeats.SavePoint();
  while ( !CurrSalon::get().isExistsOccupySeat( placeList->num, place->x, place->y ) &&
          //CurrSalon->placeIsFree( place ) && place->isplace && place->visible &&
          place->clname == Passengers.cabin_clname &&
          Result + foundCount < MAXPLACE() &&
          VerifyUseLayer( place ) &&
          condRates.CanUseRate( place )
          /* 11.11.09
          ( !CanUseLayers ||
            place->isLayer( PlaceLayer ) ||
            PlaceLayer == cltUnknown && place->layers.empty() )*/ &&
          ( !CanUseSmoke || place->isLayer( cltSmoke ) ) &&
          ( AllowedAttrsSeat.passSeat( place ) ) ) {
    if ( canUseSUBCLS ) {
      for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
        if ( !prem->pr_denial && prem->rem == SUBCLS_REM )
            break;
      }
      if ( ( FindSUBCLS && prem == place->rems.end() ) ||
           ( !FindSUBCLS && prem != place->rems.end() ) ) //  �� ᬮ��� ��ᠤ��� �� ����� SUBCLS
          break;
    }
    if ( SUBCLS_REM.empty() || !canUseSUBCLS ) {
        bool pr_find=false;
        for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
          if ( !prem->pr_denial && isREM_SUBCLS( prem->rem ) ) {
            pr_find=true;
            break;
          }
        }
     if ( pr_find )
        break;
    }
    switch( (int)CanUseRems ) {
      case sOnlyUse:
         if ( place->rems.empty() || Remarks.empty() ) { // ������ ��� �� ���� ᮢ��������
           SeatsStat.stop(__FUNCTION__);
           return Result;
         }
         for ( irem = Remarks.begin(); irem != Remarks.end(); irem++ ) {
           for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
             if ( prem->rem == *irem && !prem->pr_denial )
               break;
           }
           if ( prem != place->rems.end() )
             break;
         }
         if ( irem == Remarks.end() || DoNotRemsSeats( place->rems ) ) {
           SeatsStat.stop(__FUNCTION__);
           return Result;
         }
         break;
      case sMaxUse:
      case sAllUse:
         if ( Remarks.size() != place->rems.size() ) {
           SeatsStat.stop(__FUNCTION__);
           return Result;
         }
         for ( irem = Remarks.begin(); irem != Remarks.end(); irem++ ) {
           for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
             if ( prem->rem == *irem && !prem->pr_denial )
               break;
           }
           if ( prem == place->rems.end() || DoNotRemsSeats( place->rems ) ) {
             SeatsStat.stop(__FUNCTION__);
             return Result;
           }
         }
         break;
      case sNotUse_NotUseDenial:
      case sNotUseDenial:
        if ( DoNotRemsSeats( place->rems ) ) {
            SeatsStat.stop(__FUNCTION__);
            return Result;
        }
        if ( CanUseRems == sNotUseDenial ) break;
        //[[fallthrough]]; //�᪮��������, �᫨ ��������� ��� break �����⨬�, ���� ��ࠢ��� �訡��
      case sNotUse:
         for( vector<SALONS2::TRem>::const_iterator prem=place->rems.begin(); prem!=place->rems.end(); prem++ ) {
//           ProgTrace( TRACE5, "sNotUse: Result=%d, FP.x=%d, FP.y=%d, rem=%s", Result, FP.x, FP.y, prem->rem.c_str() );
               if ( !prem->pr_denial ) {
                 SeatsStat.stop(__FUNCTION__);
                 return Result;
               }
         }
         break;
    } /* end switch */
    if ( !condSeats.findSeat( place->seatDescr ) ) {
      break;
    }
    Result++; /* ��諨 �� ���� ���� */
    if ( condSeats.SeatDescription &&
         condSeats.isOk() ) {
      break;
    }
    switch( Step ) {
      case sRight:
         EP.x++;
         break;
      case sLeft:
         EP.x--;
         break;
      case sDown:
         EP.y++;
         break;
      case sUp:
         EP.y--;
         break;
    }
    if ( !placeList->ValidPlace( EP ) )
      break;
    place = placeList->place( EP );

  } /* end while */
  if ( condSeats.SeatDescription ) {
    if ( !condSeats.isOk() ) {
      condSeats.Rollback();
    }
    else {
      LogTrace(TRACE5) << "condSeats.isOk - stop search";
    }
  }
  SeatsStat.stop(__FUNCTION__);
  return Result;
}

/* ���ᠤ�� ��㯯� � PlaceList, ��稭�� � ����樨 FP,
   � ���ࠢ����� Step ( ��ᬠ�ਢ����� ⮫쪮 sRight - ��ਧ��⠫쭠� ,
   � sDown - ���⨪��쭠� ��ᠤ�� => ��ᬠ�ਢ��� ⮫쪮 ���ᠦ�஢ ���. ��㯯�.
   �ਮ���� ���᪠ sRight: sRight, sLeft, sTubeRight (�१ ��室), sTubeLeft ).
   Wanted - �ᮡ� ��砩, ����� ���� ��ᠤ��� 2x2, � �� 3 � 1.
  �������� ��६����:
   CanUseTube - ���� �� Step = sRight �१ ��室�
   Alone - ��ᠤ��� ���� ���ᠦ�� � ��� ����� ⮫쪮 ���� ࠧ */
bool TSeatPlaces::SeatSubGrp_On( SALONS2::TPoint FP, TSeatStep Step, int Wanted )
{
  SeatsStat.start(__FUNCTION__);
  if ( Step == sLeft )
    Step = sRight;
  if ( Step == sUp )
    Step = sDown;
//  ProgTrace( TRACE5, "Seatd_On FP=(%d,%d), Step=%d, Wanted=%d", FP.x, FP.y, Step, Wanted );
  int foundCount = 0; // ���-�� ��������� ���� �ᥣ�
  int foundBefore = 0; // ���-�� ��������� ���� �� � ��᫥ FP
  int foundTubeBefore = 0;
  int foundTubeAfter = 0;
  SALONS2::TPlaceList *placeList = CurrSalon::get().CurrPlaceList();
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 ) /*����� ��⠢���� ������*/
    Alone = true;// �� ���� ��室 �, ���� �ந��樠����஢��� ��. ��६����� Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
  if ( !foundAfter ) {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
  //ProgTrace( TRACE5, "FP=(%d,%d), foundafter=%d", FP.x, FP.y, foundAfter );
  foundCount += foundAfter;
  if ( foundAfter && Wanted ) { // �᫨ �� ��諨 ���� � ��� ���� Wanted
    if ( foundAfter > Wanted )
      foundAfter = Wanted;
    Wanted -= Put_Find_Places( FP, FP, foundAfter, Step );
    if ( Wanted <= 0 ) {
      SeatsStat.stop(__FUNCTION__);
      return true; // �� �� ��諮��
    }
  }
  SALONS2::TPoint EP = FP;
  if ( foundAfter < MAXPLACE() ) {
    switch( (int)Step ) {
      case sRight:
         EP.x--;
         foundBefore = FindPlaces_From( EP, foundCount, sLeft );
         break;
      case sDown:
         EP.y--;
         foundBefore = FindPlaces_From( EP, foundCount, sUp );
         break;
    }
  }
  foundCount += foundBefore;
//  ProgTrace( TRACE5, "FP=(%d,%d), foundbefore=%d", FP.x, FP.y, foundBefore );
  if ( foundBefore && Wanted ) { // �᫨ �� ��諨 ���� � ��� ���� Wanted
    if ( foundBefore > Wanted )
      foundBefore = Wanted;
    switch( (int)Step ) {
      case sRight:
         EP.x -= foundBefore - 1;
         break;
      case sDown:
         EP.y -= foundBefore - 1;
         break;
    }
    Wanted -= Put_Find_Places( FP, EP, foundBefore, Step );
    if ( Wanted <= 0 ) {
      SeatsStat.stop(__FUNCTION__);
      return true; /* �� �� ��諮�� */
    }
  }
 /* ����� ����⠥��� ���᪠�� �१ ��室, �� �᫮��� �� ���� �� ��ਧ��⠫� */
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE() ) {
    EP.x = FP.x + foundAfter + 1; //???/* ��⠭���������� �� �।���������� ���� */
    if ( placeList->ValidPlace( EP ) ) {
      SALONS2::TPoint p( EP.x - 1, EP.y );
      SALONS2::TPlace *place = placeList->place( p ); /* ��६ �।. ���� */
      if ( !place->visible ) {
        foundTubeAfter = FindPlaces_From( EP, foundCount, Step ); /* ���� ��᫥ ��室� */
        foundCount += foundTubeAfter; /* 㢥��稢��� ��饥 ���-�� ���� */
//       ProgTrace( TRACE5, "FP=(%d,%d), foundTubeAfter=%d", EP.x, EP.y, foundTubeAfter );
        if ( foundTubeAfter && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
          if ( foundTubeAfter > Wanted )
            foundTubeAfter = Wanted;
           Wanted -= Put_Find_Places( EP, EP, foundTubeAfter, Step ); /* ���� ��ࠬ��� EP */
         /* �.�. �窠 ����� ������ ��室���� �� ��ࢮ� ���� ��᫥ ��室�,
            ���� ࠡ���� �� �㤥� */
           if ( Wanted <= 0 ) {
             SeatsStat.stop(__FUNCTION__);
             return true; /* �� �� ��諮�� */
           }
        }
      }
    }
    /* ����� ���� ������ �१ ��室 */
    if ( foundCount < MAXPLACE() ) {
      EP.x = FP.x - foundBefore - 2; // ��⠭���������� �� �।���������� ����
      //SALONS2::TPoint VP = EP;
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      if ( placeList->ValidPlace( EP ) ) {
        SALONS2::TPoint p( EP.x + 1, EP.y );
        SALONS2::TPlace *place = placeList->place( p ); /* ��६ ᫥�. ���� */
        if ( !place->visible ) { /* ᫥���饥 ���� �� ����� => ��室 */
          foundTubeBefore = FindPlaces_From( EP, foundCount, sLeft );
          foundCount += foundTubeBefore;
//          ProgTrace( TRACE5, "EP=(%d,%d), foundTubeBefore=%d", EP.x, EP.y, foundTubeBefore );
          if ( foundTubeBefore && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
            if ( foundTubeBefore > Wanted )
              foundTubeBefore = Wanted;
            EP.x -= foundTubeBefore - 1;
            Wanted -= Put_Find_Places( EP, EP, foundTubeBefore, sLeft ); /* ���� ��ࠬ��� EP */
            if ( Wanted <= 0 ) {
              SeatsStat.stop(__FUNCTION__);
              return true; /* �� �� ��諮�� */
            }
          }
        }
      }
    }
  } // end of found tube
  if ( Wanted ) {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
  /* �᫨ �� �����, � Wanted = 0 ( ����砫쭮 ) �
     �� ����� ���-�� ���� ��������� ᫥��, �ࠢ� ... */
  int EndWanted = 0; /* �ਧ��� ⮣�, �� ���� �㤥� 㤠���� ���� ��譥� ���� �� ��������� */
  if ( Step == sRight && foundCount == MAXPLACE() &&
       counters.p_Count_3() == Passengers.counters.p_Count_3() &&
       counters.p_Count_2() == Passengers.counters.p_Count_2() &&
       counters.p_Count() == Passengers.counters.p_Count() - MAXPLACE() - 1 ) {
    /* ���� ���஡����� ��ᠤ��� �� ᫥���騩 �� �� ������ 祫-�� � 2-� */
    EP = FP;
    EP.y++;
    if ( EP.y >= placeList->GetYsCount() || !SeatSubGrp_On( EP, Step, 2 ) ) {
      SeatsStat.stop(__FUNCTION__);
      return false; /* �� ᬮ��� ��ᠤ��� 2-� �� ᫥���騩 �� */
    }
    else {
      EndWanted = seatplaces.size(); /* ��諨 2 ���� �� ᫥���饬 ��� � �������� �� � FPlaces */
      /* ������� ��� ���稪� �� ���� ���� */
      counters.Add_p_Count( -1 );
    }
  }
  /* ��稭��� ���������� ����祭�� ���� */
  EP = FP;
  switch( (int)Step ) {
    case sRight:
       EP.x -= foundBefore;
       break;
    case sDown:
       EP.y -= foundBefore;
       break;
  }
  foundCount = Put_Find_Places( FP, EP, foundBefore + foundAfter, Step );
  if ( !foundCount ) {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
  if ( foundTubeAfter ) {
    EP = FP;
    switch( (int)Step ) {
      case sRight:
         EP.x += foundAfter + 1;
         break;
      case sDown:
         EP.y += foundAfter + 1;
         break;
    }
    foundCount +=  Put_Find_Places( EP, EP, foundTubeAfter, Step );
  }
  if ( foundTubeBefore ) {
    EP = FP;
    switch( (int)Step ) {
      case sRight:
         EP.x -= foundTubeBefore + foundBefore + 1;
         break;
      case sDown:
         EP.y += foundTubeBefore + foundBefore + 1;
         break;
    }
    foundCount += Put_Find_Places( FP, EP, foundTubeBefore, Step );
  }
  if ( EndWanted > 0 ) { /* ���� 㤠���� ���� ᠬ�� ���宥 ����: */
    /* ᠬ�� 㤠������ �� ��᫥	����� ���������� � ⥪�饬 ��� */
    EP.x = placeList->GetPlaceIndex( FP );
    EP.y = 0;
    int k = seatplaces.size();
    for ( int i=EndWanted; i<k; i++ ) {
      int m = abs( EP.x - placeList->GetPlaceIndex( seatplaces[ i ].Pos ) );
      if ( EP.y < m ) {
        EP.y = m;
        EndWanted = i;
      }
    }
    /* ��諨 ᠬ�� 㤠������ ���� � ᥩ�� 㤠��� ��� */
    RollBack( EndWanted, EndWanted );
    counters.Add_p_Count( 1 );
    SeatsStat.stop(__FUNCTION__);
    return true; /* �� ��諨 ��室�� */
  }
  /* ⥯��� ���㤨� ᫥���騩 ��ਠ��: � ⥪�饬 ��� ��諨 �ᥣ� ���� ����.
     ����� ⠪�� �����⨬� ⮫쪮 ������� */
  if ( foundCount == 1 && CanUseAlone != uTrue ) { /*����� ��⠢���� ������*/
    if ( Alone ) {
      Alone = false;
    }
    else {  /* ����� 2 ࠧ� �⮡� ��﫮�� ���� ���� � ���. */
      SeatsStat.stop(__FUNCTION__);
      return false; /*( p_Count_3( Step ) = Passengers.p_Count_3( Step ) )AND
              ( p_Count_2( Step ) = Passengers.p_Count_2( Step ) )AND
               ( p_Count( Step ) = Passengers.p_Count( Step ) ) ???};*/
    }
  }
  if ( counters.p_Count_3( sDown ) == Passengers.counters.p_Count_3( sDown ) &&
       counters.p_Count_2( sDown ) == Passengers.counters.p_Count_2( sDown ) &&
       counters.p_Count_3( sRight ) == Passengers.counters.p_Count_3( sRight ) &&
       counters.p_Count_2( sRight ) == Passengers.counters.p_Count_2( sRight ) &&
       counters.p_Count( Step ) == Passengers.counters.p_Count( Step ) ) {
    SeatsStat.stop(__FUNCTION__);
    return true;
  }
  int lines = placeList->GetXsCount(), visible=0;
  for ( int line=0; line<lines; line++ ) {
    SALONS2::TPoint f=FP;
    f.x = line;
//  	ProgTrace( TRACE5, "line=%d, name=%s", line, placeList->GetXsName( line ).c_str() );
    if ( placeList->GetXsName( line ).empty() ||
           !placeList->ValidPlace( f ) ||
           !placeList->place( f )->visible ||
           !placeList->place( f )->isplace )
        continue;
    visible++;
  }
//  ProgTrace( TRACE5, "getCanUseOneRow()=%d, canUseOneRow=%d, foundCount=%d, visible=%d",
//             getCanUseOneRow(), canUseOneRow, foundCount, visible );
  if ( !getCanUseOneRow() || !canUseOneRow || ( canUseOneRow && foundCount == visible ) ) {
  /* ���室�� �� ᫥���騩 �� ???canUseOneRow??? */
    EP = FP;
    switch( (int)Step ) {
      case sRight:
         EP.y++;
         break;
      case sDown:
         EP.x++;
         break;
    }
    if ( EP.x >= placeList->GetXsCount() || EP.y >= placeList->GetYsCount() ||
         !SeatSubGrp_On( EP, Step, 0 ) ) { // ��祣� �� ᬮ��� ���� �����
      EP = FP;
      switch( (int)Step ) {
        case sRight:
           EP.y--;
           break;
        case sDown:
           EP.x--;
           break;
      }
      if ( EP.x < 0 || EP.y < 0 ||
           !SeatSubGrp_On( EP, Step, 0 ) ) { // ��祣� �� ᬮ��� ���� �����
        SeatsStat.stop(__FUNCTION__);
        return false;
      }
    }
  }
  else {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
  SeatsStat.stop(__FUNCTION__);
  return true;
}

bool TSeatPlaces::SeatsStayedSubGrp( TWhere Where )
{
  SeatsStat.start(__FUNCTION__);
  /* �஢�ઠ � ��⠫��� �� ���� ��㯯�, ��� ��㯯� ��⮨� �� ������ 祫����� ? */
  if ( counters.p_Count_3( sRight ) == Passengers.counters.p_Count_3( sRight ) &&
       counters.p_Count_2( sRight ) == Passengers.counters.p_Count_2( sRight ) &&
       counters.p_Count( sRight ) == Passengers.counters.p_Count( sRight ) &&
       counters.p_Count_3( sDown ) == Passengers.counters.p_Count_3( sDown ) &&
       counters.p_Count_2( sDown ) == Passengers.counters.p_Count_2( sDown ) &&
       counters.p_Count( sDown ) == Passengers.counters.p_Count( sDown ) ) {
    SeatsStat.stop(__FUNCTION__);
    return true;
  }

  SALONS2::TPoint EP;
  /* ���� ��ᠤ��� ��⠢����� ���� ���ᠦ�஢ */
  /* ��� �⮣� ���� ����� ��࠭�� ���� ����� � ���஡����� ��ᠤ���
    ��⠢����� ��㯯� �冷� � 㦥 ��ᠦ����� */
  VSeatPlaces sp( seatplaces.begin(), seatplaces.end() );
  if ( Where != sUpDown )
  for (ISeatPlace isp = sp.begin(); isp!= sp.end(); isp++ ) {
    CurrSalon::get().SetCurrPlaceList( isp->placeList );
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      /* ���饬 �ࠢ� �� ���������� ���� */
      switch( (int)isp->Step ) {
        case sRight:
           EP.x = isp->Pos.x + isp->oldPlaces.size();
           break;
        case sLeft:
           EP.x = isp->Pos.x + 1;
           break;
        case sDown:
           EP.y = isp->Pos.y + distance( isp->oldPlaces.begin(), ipl );
           break;
        case sUp:
           EP.y = isp->Pos.y - distance( isp->oldPlaces.begin(), ipl );
           break;
      }
      switch( (int)isp->Step ) {
        case sRight:
        case sLeft:
           EP.y = isp->Pos.y;
           break;
        case sDown:
        case sUp:
           EP.x = isp->Pos.x + 1;
           break;
      }
      Alone = true; /* ����� ���� ���� ���� */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
       }
      SALONS2::TPlaceList *placeList = CurrSalon::get().CurrPlaceList();
      SALONS2::TPoint p( EP.x + 1, EP.y );
      if ( CanUseTube && placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
       /* ����� ���஡����� �᪠�� �१ ��室 */
       Alone = true; /* ����� ���� ���� ���� */
       if ( SeatSubGrp_On( p, sRight, 0 ) ) {
         SeatsStat.stop(__FUNCTION__);
         return true;
       }
      }
      /* ���饬 ᫥�� �� ���������� ���� */
      switch( (int)isp->Step ) {
        case sRight:
           EP.x = isp->Pos.x - 1;
           break;
        case sLeft:
           EP.x = isp->Pos.x - isp->oldPlaces.size();
           break;
        case sDown:
        case sUp:
           EP.x = isp->Pos.x - 1;
           break;
      }
      Alone = true; /* ����� ���� ���� ���� */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      placeList = CurrSalon::get().CurrPlaceList();
      p.x = EP.x - 1;
      p.y = EP.y;
      if ( CanUseTube &&
           placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
        /* ����� ���஡����� �᪠�� �१ ��室 */
        Alone = true; /* ����� ���� ���� ���� */
        p.x = EP.x - 1;
        p.y = EP.y;
        if ( SeatSubGrp_On( p, sRight, 0 ) ) {
          SeatsStat.stop(__FUNCTION__);
          return true;
        }
      }
      if ( isp->Step == sLeft || isp->Step == sRight )
        break;
    } /* end for */
    if ( Where != sLeftRight )
    /* ���饬 ᢥ��� � ᭨�� �� ���������� ���� */
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      switch( isp->Step ) {
        case sRight:
           EP.x = isp->Pos.x + distance( isp->oldPlaces.begin(), ipl );
           break;
        case sLeft:
           EP.x = isp->Pos.x - distance( isp->oldPlaces.begin(), ipl );
           break;
        case sDown:
           EP.y = isp->Pos.y + isp->oldPlaces.size();
           break;
        case sUp:
           EP.y = isp->Pos.y - isp->oldPlaces.size();
           break;
      }
      /* ��ᠤ�� ᢥ��� �� ������� ���� */
      switch( isp->Step ) {
        case sRight:
        case sLeft:
           EP.y = isp->Pos.y - 1;
           break;
        case sDown:
        case sUp:
           EP.x = isp->Pos.x;
           break;
      }
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      /* ⥯��� ��ᬮ�ਬ ᭨�� */
      switch( isp->Step ) {
        case sRight:
        case sLeft:
           EP.y = isp->Pos.y + 1;
           break;
        case sDown:
           EP.y = isp->Pos.y + isp->oldPlaces.size();
           break;
        case sUp:
           EP.y = isp->Pos.y - 1;
           break;
      }
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      if ( isp->Step == sUp || isp->Step == sDown )
        break;
    } /* end for */
  } /* end for */
  SeatsStat.stop(__FUNCTION__);
  return false;
}

TSeatPlace &TSeatPlaces::GetEqualSeatPlace( TPassenger &pass )
{
  SeatsStat.start(__FUNCTION__);
  int MaxEqualQ = -10000;
  ISeatPlace misp=seatplaces.end();
  string ispPlaceName_lat, ispPlaceName_rus;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++) {
//    ProgTrace( TRACE5, "isp->oldPlaces.size()=%zu, pass.countPlace=%d",
//                       isp->oldPlaces.size(), pass.countPlace );
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* ���-�� ���� ������ ᮢ������ */
      continue;
    ispPlaceName_lat.clear();
    ispPlaceName_rus.clear();
    SALONS2::TPlaceList *placeList = isp->placeList;
    ispPlaceName_lat = SeatNumber::tryDenormalizeRow( placeList->GetYsName( isp->Pos.y ) );
    ispPlaceName_rus = ispPlaceName_lat;
    ispPlaceName_lat += SeatNumber::tryDenormalizeLine( placeList->GetXsName( isp->Pos.x ), 1 );
    ispPlaceName_rus += SeatNumber::tryDenormalizeLine( placeList->GetXsName( isp->Pos.x ), 0 );
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    bool pr_valid_place = true;
    vector<string> vrems;
    pass.get_remarks( vrems );
    for (vector<string>::iterator irem=vrems.begin(); irem!= vrems.end(); irem++ ) { // �஡�� �� ६�ઠ� ���ᠦ��
      for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
        /* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
        vector<SALONS2::TRem>::iterator itr;
        for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ ) {
          if ( *irem == itr->rem ) {
            if ( itr->pr_denial ) {
              EqualQ -= PR_REM_TO_REM;
              pr_valid_place = false;
            }
            else {
              EqualQ += PR_REM_TO_REM;
            }
            break;
          }
        }
        if ( itr == ipl->rems.end() ) /* �� ��諨 ६��� � ���� */
          EqualQ += PR_REM_TO_NOREM;
      } /* ����� �஡��� �� ���⠬ */
    } /* ����� �஡��� �� ६�ઠ� ���ᠦ�� */

    // ⨯ ����
    ProgTrace( TRACE5, "pass.pers_type=%s, isp->oldPlaces->begin=%s, AllowedAttrsSeat.passSeats=%d", pass.pers_type.c_str(),
               string(isp->oldPlaces.begin()->xname + isp->oldPlaces.begin()->yname).c_str(), AllowedAttrsSeat.passSeats( pass.pers_type, pass.isRemark( "INFT" ),
               isp->oldPlaces ) );
    if ( AllowedAttrsSeat.passSeats( pass.pers_type, pass.isRemark( "INFT" ), isp->oldPlaces ) )
        EqualQ += PR_REM_TO_REM;
    else
        EqualQ -= PR_REM_TO_REM;

    /*01.04.11if ( pass.placeName == ispPlaceName_lat || pass.placeName == ispPlaceName_rus ||  //???agent_seat!!!
           pass.preseat == ispPlaceName_lat || pass.preseat == ispPlaceName_rus )*/
    if ( pass.preseat_no == ispPlaceName_lat || pass.preseat_no == ispPlaceName_rus )
      EqualQ += PR_EQUAL_N_PLACE;
    //ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( ( pass.placeRem.find( "SW" ) == 2  &&
           ( isp->Pos.x == 0 || isp->Pos.x == placeList->GetXsCount() - 1 )
         ) ||
         ( pass.placeRem.find( "SA" ) == 2  &&
           ( ( isp->Pos.x - 1 >= 0 &&
               placeList->GetXsName( isp->Pos.x - 1 ).empty() ) ||
             ( isp->Pos.x + 1 < placeList->GetXsCount() &&
               placeList->GetXsName( isp->Pos.x + 1 ).empty() ) )
         )
       )
      EqualQ += PR_EQUAL_REMPLACE;
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.prSmoke == isp->oldPlaces.begin()->isLayer( cltSmoke ) )
      EqualQ += PR_EQUAL_SMOKE;
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.pers_type != "��" ) {
      for (ISeatPlace nsp=seatplaces.begin(); nsp!=seatplaces.end(); nsp++) {
        if ( nsp == isp || nsp->InUse || nsp->oldPlaces.size() != isp->oldPlaces.size() || nsp->placeList != isp->placeList )
          continue;
        if ( abs( nsp->Pos.x - isp->Pos.x ) == 1 && abs( nsp->Pos.y - isp->Pos.y ) == 0 ) {
          EqualQ += 1;
          break;
        }
      }
    }
    //ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( MaxEqualQ < EqualQ ) {
      MaxEqualQ = EqualQ;
      misp = isp;
      misp->isValid = pr_valid_place;
    }
  } /* end for */
 if ( misp==seatplaces.end() )
   ProgError( STDLOG, "GetEqualSeatPlace: misp=seatplaces.end()=%d", misp==seatplaces.end() );
 misp->InUse = true;
 SeatsStat.stop(__FUNCTION__);
 return *misp;
}


bool CompSeats( TSeatPlace item1, TSeatPlace item2 )
{
    if ( item1.Pos.y < item2.Pos.y )
      return true;
    else
        if ( item1.Pos.y > item2.Pos.y )
            return false;
        else
            if ( item1.Pos.x < item2.Pos.x )
                return true;
            else
                if ( item1.Pos.x > item2.Pos.x )
                    return false;
                else
                    return true;
};


void TSeatPlaces::PlacesToPassengers()
{
  if ( seatplaces.empty() )
   return;
  SeatsStat.start(__FUNCTION__);
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++)
    isp->InUse = false;
  sort(seatplaces.begin(),seatplaces.end(),CompSeats);

  int lp = Passengers.getCount();
  for ( int i=0; i<lp; i++ ) {
    TPassenger &pass = Passengers.Get( i );
    TSeatPlace &seatPlace = GetEqualSeatPlace( pass );
    if ( seatPlace.Step == sLeft || seatPlace.Step == sUp )
      throw EXCEPTIONS::Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
    pass.placeList = seatPlace.placeList;
    pass.Pos = seatPlace.Pos;
    pass.Step = seatPlace.Step;
    pass.foundSeats = seatPlace.placeList->GetPlaceName( seatPlace.Pos );
    pass.isValidPlace = seatPlace.isValid;
    pass.set_seat_no();
  }
  SeatsStat.stop(__FUNCTION__);
}

void TSeatPlaces::operator = ( VSeatPlaces &items )
{
  seatplaces.clear();
  seatplaces = items;
}

void TSeatPlaces::operator >> ( VSeatPlaces &items )
{
  items.insert( items.end(), seatplaces.begin(), seatplaces.end() );
}

/* ��ᠤ�� �ᥩ ��㯯� ��稭�� � ����樨 FP */
bool TSeatPlaces::SeatsGrp_On( SALONS2::TPoint FP  )
{
  SeatsStat.start(__FUNCTION__);
  //ProgTrace( TRACE5, "FP(x=%d, y=%d)", FP.x, FP.y );
  /* ������ ����祭�� ���� */
  RollBack( );
  /* �᫨ ���� ���ᠦ��� � ��㯯� � ���⨪��쭮� ��ᠤ���, � �஡㥬 �� ��ᠤ��� */
  if ( Passengers.counters.p_Count_3( sDown ) + Passengers.counters.p_Count_2( sDown ) > 0 ) {
    if ( !SeatSubGrp_On( FP, sDown, 0 ) ) { /* �� ����砥��� */
      RollBack( );
      SeatsStat.stop(__FUNCTION__);
      return false;
    }
    /* �᫨ ��� ��㣨� ���ᠦ�஢, � ⮣�� ��ᠤ�� �믮����� �ᯥ譮 */
    if ( Passengers.counters.p_Count_3() +
         Passengers.counters.p_Count_2() +
         Passengers.counters.p_Count() == 0 ) {
      SeatsStat.stop(__FUNCTION__);
      return true;
    }
  }
  else {
    if ( SeatSubGrp_On( FP, sRight, 0 ) ) {
      SeatsStat.stop(__FUNCTION__);
      return true;
    }
    if ( CanUseAlone != uTrue ) {
      RollBack( );
      SeatsStat.stop(__FUNCTION__);
      return false;
    }
  }
  if ( seatplaces.empty() ) {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
  /* �᫨ �� ����� � ᬮ��� ��ᠤ��� ���� ��㯯�
     ��ᠦ����� ��⠢����� ���� ��㯯� */
  /* ����� ��������� ��ᠤ��� � ᢥ��� � ᭨�� �� ������� ���� */
  if ( SeatsStayedSubGrp( sEveryWhere ) ) {
    SeatsStat.stop(__FUNCTION__);
    return true;
  }
  RollBack( );
  SeatsStat.stop(__FUNCTION__);
  return false;
}

bool TSeatPlaces::SeatsPassenger_OnBasePlace( string &placeName, TSeatStep Step )
{
  SeatsStat.start(__FUNCTION__);
//  ProgTrace( TRACE5, "SeatsPassenger_OnBasePlace( ) placeName=%s", placeName.c_str() );
  bool OldCanUseSmoke = CanUseSmoke;
  //bool CanUseSmoke = false;
  try {
    if ( !placeName.empty() ) {
      /* ��������� ����஢ ���� ���ᠦ�஢ � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
      for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon::get().placelists.begin();
            iplaceList!=CurrSalon::get().placelists.end(); iplaceList++ ) {
        SALONS2::TPoint FP;
        if ( (*iplaceList)->GetisPlaceXY( placeName, FP ) ) {
          CurrSalon::get().SetCurrPlaceList( *iplaceList );
          if ( SeatSubGrp_On( FP, Step, 0 ) ) {
            tst();
            CanUseSmoke = OldCanUseSmoke;
            SeatsStat.stop(__FUNCTION__);
            return true;
          }
          break;
        }
      }
    }
  }
  catch( ... ) {
    CanUseSmoke = OldCanUseSmoke;
    throw;
  }
  CanUseSmoke = OldCanUseSmoke;
  SeatsStat.stop(__FUNCTION__);
  return false;
}

inline void getRemarks( TPassenger &pass )
{
  Remarks.clear();
  switch ( (int)CanUseRems ) {
    case sAllUse:
    case sOnlyUse:
    case sNotUse_NotUseDenial:
    case sNotUseDenial:
         pass.get_remarks( Remarks );
       break;
    case sMaxUse:
       if ( !pass.maxRem.empty() )
         Remarks.push_back( pass.maxRem );
       break;
  }
}

/* ���� � ����� ᠫ��� � �������� ��������� ��६�����.
   ��������� ���� CanUseRems � PlaceRemark */
bool TSeatPlaces::SeatGrpOnBasePlace( )
{
  SeatsStat.start(__FUNCTION__);
  //ProgTrace( TRACE5, "SeatGrpOnBasePlace( )" );
  int G3 = Passengers.counters.p_Count_3( sRight );
  int G2 = Passengers.counters.p_Count_2( sRight );
  int G = Passengers.counters.p_Count( sRight );
  int V3 = Passengers.counters.p_Count_3( sDown );
  int V2 = Passengers.counters.p_Count_2( sDown );
  TUseRem OldCanUseRems = CanUseRems;
  vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
  try {
    /* ���� ��室���� ���� ��� ���ᠦ�� */
    int lp = Passengers.getCount();
    for ( int i=0; i<lp; i++ ) {
      /* �뤥�塞 �� ��㯯� �������� ���ᠦ�� � ��室�� ��� ���� ���� */
      TPassenger &pass = Passengers.Get( i );
//      ProgTrace( TRACE5, "pass %s", pass.toString().c_str() );
      Passengers.SetCountersForPass( pass );
      getRemarks( pass );
      if ( !pass.preseat_no.empty() &&
           SeatsPassenger_OnBasePlace( pass.preseat_no, pass.Step ) && /* ��諨 ������� ���� */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) ) {
  //      tst();
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      RollBack( );
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* ���� ६�ઠ */
        SeatsStat.start("TSeatPlaces::SeatGrpOnBasePlace( ) !Remarks.empty()");
        // ����⠥��� ���� �� ६�થ
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) {
          /* ��ਠ��� ���᪠ ����� ���������� ���� */
          for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
            //LogTrace(TRACE5) << icoord->placeList->num;
            CurrSalon::get().SetCurrPlaceList( icoord->placeList );
            for ( map<int,TSeatCoords>::iterator ivariant=icoord->coords.begin(); ivariant!=icoord->coords.end(); ivariant++ ) {
              for ( vector<TPoint>::iterator ic=ivariant->second.begin(); ic!=ivariant->second.end(); ic++ ) {
/*               if ( CurrSalon->isExistsOccupySeat(icoord->placeList->num, ic->x, ic->y ) ) { //!!!
                continue;
              }*/
                if ( SeatSubGrp_On( *ic, pass.Step, 0 ) && LSD( G3, G2, G, V3, V2, (TWhere )Where ) ) {
                    //ProgTrace( TRACE5, "G3=%d, G2=%d, G=%d, V3=%d, V2=%d, commit", G3, G2, G, V3, V2 );
                  SeatsStat.stop(__FUNCTION__);
                  return true;
                }
                //ProgTrace( TRACE5, "rollback" );
                RollBack( ); /* �� ����稫��� �⪠� ������� ���� */
                Passengers.SetCountersForPass( pass ); /* �뤥�塞 ����� �⮣� ���ᠦ�� */
              }
            }
          }
        }
        SeatsStat.stop("TSeatPlaces::SeatGrpOnBasePlace( ) !Remarks.empty()");
      } /* ����� ���᪠ �� ६�થ */
    } /* end for */
  }
  catch( ... ) {
   Passengers.counters.Clear( );
   Passengers.counters.Add_p_Count_3( G3, sRight );
   Passengers.counters.Add_p_Count_2( G2, sRight );
   Passengers.counters.Add_p_Count( G, sRight );
   Passengers.counters.Add_p_Count_3( V3, sDown );
   Passengers.counters.Add_p_Count_2( V2, sDown );
   CanUseRems = OldCanUseRems;
   SeatsStat.stop(__FUNCTION__);
   throw;
  }
  Passengers.counters.Clear( );
  Passengers.counters.Add_p_Count_3( G3, sRight );
  Passengers.counters.Add_p_Count_2( G2, sRight );
  Passengers.counters.Add_p_Count( G, sRight );
  Passengers.counters.Add_p_Count_3( V3, sDown );
  Passengers.counters.Add_p_Count_2( V2, sDown );
  CanUseRems = OldCanUseRems;
  RollBack( );
  SeatsStat.stop(__FUNCTION__);
  return false;
}

/* ��ᠤ�� ��㯯� �� �ᥬ ᠫ���� */
bool TSeatPlaces::SeatsGrp( )
{
  SeatsStat.start(__FUNCTION__);
  RollBack( );
  vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
  for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
    //LogTrace(TRACE5) << icoord->placeList->num;
    CurrSalon::get().SetCurrPlaceList( icoord->placeList );
    for ( map<int,TSeatCoords>::iterator ivariant=icoord->coords.begin(); ivariant!=icoord->coords.end(); ivariant++ ) {
      for ( vector<TPoint>::iterator ic=ivariant->second.begin(); ic!=ivariant->second.end(); ic++ ) {
        //LogTrace(TRACE5) << "x=" << ic->x << " y=" << ic->y;
        if ( SeatsGrp_On( *ic ) ) {
          SeatsStat.stop(__FUNCTION__);
          return true;
        }
      }
    }
  }
  RollBack( );
  SeatsStat.stop(__FUNCTION__);
  return false;
}

/* ��ᠤ�� ���ᠦ�஢ �� ���⠬ �� ���뢠� ��㯯� */
bool TSeatPlaces::SeatsPassengers( bool pr_autoreseats )
{
  SeatsStat.start("TSeatPlaces::SeatsPassengers");
  bool OLDFindSUBCLS = FindSUBCLS;
  bool OLDcanUseSUBCLS = canUseSUBCLS;
  string OLDSUBCLS_REM = SUBCLS_REM;
  vector<TPassenger> npass;
  Passengers.copyTo( npass );
  string OLDcabin_clname = Passengers.cabin_clname;
  bool OLDSeatDescrPassengers = ( Passengers.SeatDescription && pr_autoreseats );
  bool OLDSeatDescrCondSeats = ( Passengers.SeatDescription && pr_autoreseats );
  bool OLDKTube = Passengers.KTube;
  bool OLDKWindow = Passengers.KWindow;
  bool OLDUseSmoke = Passengers.UseSmoke;
  int OLDp_Count_3G = Passengers.counters.p_Count_3(sRight);
  int OLDp_Count_2G = Passengers.counters.p_Count_2(sRight);
  int OLDp_CountG = Passengers.counters.p_Count(sRight);
  int OLDp_Count_3V = Passengers.counters.p_Count_3(sDown);
  int OLDp_Count_2V = Passengers.counters.p_Count_2(sDown);
  bool isWorkINFT = AllowedAttrsSeat.pr_isWorkINFT;

  bool pr_seat = false;
  //ProgTrace( TRACE5, "SeatsPassengers: pr_isWorkINFT=%d", AllowedAttrsSeat.pr_isWorkINFT );

  try {
//    for ( int FCanUseSeatDescr=OLDSeatDescrPassengers; FCanUseSeatDescr>=0; FCanUseSeatDescr-- ) {
      condSeats.SeatDescription = OLDSeatDescrPassengers;
      for ( int FCanUseINFT=AllowedAttrsSeat.pr_isWorkINFT; FCanUseINFT>=0; FCanUseINFT-- ) {
        for ( int ik=0; ik<=AllowedAttrsSeat.pr_isWorkINFT*2; ik++ )   { //0 -��㯯� � ���쬨, 1-��㯯� ��� ��⥩, 2-��㯯� ��� ��⥩ ��� ��� ����
          for ( int FCanUseElem_Type=1/*pr_autoreseats*/; FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // ���砥 ��⮬��. ��ᠤ�� 2 ��室�: � ��⮬ ⨯� ���� � ��� ��� ⨯� ����
            for ( /*int i=(int)!pr_autoreseats; i<=1+(int)pr_autoreseats; i++*/ int i=0; i<=2; i++ ) {
              for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
                /* ����� ���ᠦ�� ��ᠦ�� ��� ��ᠤ�� �� �஭� � � ���ᠦ�� ����� �� �஭� ��� ��� �।���. ��ᠤ�� ��� � ���ᠦ�� 㪠���� �� � ���� */
                if ( ipass->InUse ) {
                  pr_seat = true;
                  continue;
                }
                if ( SALONS2::isUserProtectLayer( PlaceLayer ) && !CanUseLayer( PlaceLayer, UseLayers ) &&
                     ( ipass->preseat_layer != PlaceLayer ) ) {
                  continue;
                }
                if (!( (ik == 0 && ((!AllowedAttrsSeat.pr_isWorkINFT) || (FCanUseINFT == 1 && ipass->countPlace > 0 && ipass->isRemark( "INFT" )))) ||
                       (ik == 1 && FCanUseINFT == 1 && !(ipass->countPlace > 0 && ipass->isRemark( "INFT" ))) ||
                       (ik == 2 && FCanUseINFT == 0 && !(ipass->countPlace > 0 && ipass->isRemark( "INFT" ))) )) {
                  continue;
                }
//                ProgTrace( TRACE5, "pax_id=%d,ik=%d, FCanUseINFT=%d, ipass->countPlace=%d, pass.INFT=%d", ipass->paxId,ik, FCanUseINFT, ipass->countPlace, ipass->isRemark( "INFT" ) );
                /*???31.03.11        if ( ipass->InUse || PlaceLayer == cltProtCkin && !CanUseLayer( cltProtCkin, UseLayers ) && //!!!
                                 ( ipass->layer != PlaceLayer || ipass->preseat.empty() || ipass->preseat != ipass->placeName ) )
            continue;*/
                Passengers.Clear();
                ipass->placeList = NULL;
                ipass->seat_no.clear();
                int old_index = ipass->index;

                AllowedAttrsSeat.clearAll();
                if ( FCanUseElem_Type == 1 && ipass->countPlace > 0 && ipass->pers_type != "��" ) {
                  AllowedAttrsSeat.getValidChildElem_Types( );
                }
                AllowedAttrsSeat.pr_isWorkINFT = ( FCanUseINFT == 1 );
                AllowedAttrsSeat.pr_INFT = ( ipass->countPlace > 0 && ipass->isRemark( "INFT" ) );
                if ( pr_autoreseats ) {
                  if ( i == 0 ) {
                    if ( ipass->SUBCLS_REM.empty() ) {
                      continue;
                    }
                    FindSUBCLS = true;
                    canUseSUBCLS = true;
                    SUBCLS_REM = ipass->SUBCLS_REM;
                  }
                  else {
                    if ( i == 1 ) {
                      if ( !ipass->SUBCLS_REM.empty() ) {
                        continue;
                      }
                      FindSUBCLS = false;
                      canUseSUBCLS = true;
                    }
                    else {
                      canUseSUBCLS = false;
                    }
                  }
                }
                else { // ���� ��室: �᫨ �ᯮ������ ���� �� ���������, � ᠦ��� ��� ���ᠦ�஢ � �㦭� �������ᮬ
                  if ( i == 2 || ( i == 0 && canUseSUBCLS && ipass->SUBCLS_REM != SUBCLS_REM ) )
                    continue; // ᥩ�� ���� ���� �� �������ᠬ, � � ���ᠦ�� ��� ��� - �� ��⠥��� ��� ��ᠤ���
                  if ( i == 1 ) { // ᥩ�� ���� ���� ���� ��� ���ᠦ�஢ � ������ ��� ��������
/*                                                  ProgTrace( TRACE5, "ipass->SUBCLS_REM=%s, SUBCLS_REM=%s, canUseSUBCLS=%d",
                             ipass->SUBCLS_REM.c_str(), SUBCLS_REM.c_str(), canUseSUBCLS );*/
                    if ( canUseSUBCLS && !pr_seat )
                      continue;
                    FindSUBCLS = false;
                    //       		  	canUseSUBCLS = false;
                  }
                }
//                ProgTrace( TRACE5, "pax_id=%d,ik=%d, FCanUseINFT=%d, ipass->countPlace=%d, pass.INFT=%d, FCanUseElem_Type=%d, i=%d",
//                           ipass->paxId,ik, FCanUseINFT, ipass->countPlace, ipass->isRemark( "INFT" ), FCanUseElem_Type, i );

                Passengers.Add( *ipass );
                condSeats.seatsDescr = ipass->seatsDescr;
                //LogTrace(TRACE5) << condSeats.seatsDescr.traceStr();
                ipass->index = old_index;
//                ProgTrace( TRACE5, "Passengers.Count=%d - go seats", Passengers.getCount() );
                if ( (!condSeats.SeatDescription && SeatGrpOnBasePlace( )) ||
                     ( ( CanUseRems == sNotUse_NotUseDenial ||
                         CanUseRems == sNotUse ||
                         CanUseRems == sIgnoreUse ||
                         CanUseRems == sNotUseDenial ) &&
                       ( !CanUseLayers ||
                         ( PlaceLayer == cltProtCkin && CanUseLayer( cltProtCkin, UseLayers ) ) ||
                         ( PlaceLayer == cltProtAfterPay && CanUseLayer( cltProtAfterPay, UseLayers ) ) ||
                         ( PlaceLayer == cltPNLAfterPay && CanUseLayer( cltPNLAfterPay, UseLayers ) ) ||
                         ( PlaceLayer == cltProtBeforePay && CanUseLayer( cltProtBeforePay, UseLayers ) ) ||
                         ( PlaceLayer == cltPNLBeforePay && CanUseLayer( cltPNLBeforePay, UseLayers ) ) ||
                         ( PlaceLayer == cltProtSelfCkin && CanUseLayer( cltProtSelfCkin, UseLayers ) ) ||
                         !SALONS2::isUserProtectLayer( PlaceLayer ) ) &&
                       SeatsGrp( ) ) ) { // ⮣�� ����� ��室��� ���� �� �ᥬ� ᠫ���
                  if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
                    throw EXCEPTIONS::Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
                  ipass->placeList = seatplaces.begin()->placeList;
                  ipass->Pos = seatplaces.begin()->Pos;
                  ipass->Step = seatplaces.begin()->Step;
                  ipass->foundSeats = ipass->placeList->GetPlaceName( ipass->Pos );
                  ipass->isValidPlace = ipass->is_valid_seats( seatplaces.begin()->oldPlaces );
                  ipass->InUse = true;
                  ipass->set_seat_no();
                  TCompLayerType l = grp_status;
                  int vseparately_seats_adults_crs_pax_id = separately_seats_adults_crs_pax_id;
                  Clear();
                  grp_status = l;//??? ���� ⠪ ������, �� �� ��ᨢ�
                  separately_seats_adults_crs_pax_id = vseparately_seats_adults_crs_pax_id;
                  if ( !pr_autoreseats && i == 0 && canUseSUBCLS ) {
                    pr_seat = true;
                  }
                }
              } // for passengers
            } // end for i=0..2
          } // end for FCanUseElem_Type
        } // for ik - �����㯯�
      } // end for FCanUseINFT
  //  } // end for condSeats.SeatDescription
  }
  catch( ... ) {
    FindSUBCLS = OLDFindSUBCLS;
    canUseSUBCLS = OLDcanUseSUBCLS;
    SUBCLS_REM = OLDSUBCLS_REM;
    AllowedAttrsSeat.pr_isWorkINFT = isWorkINFT;
    Passengers.Clear();
    Passengers.copyFrom( npass );
    Passengers.cabin_clname = OLDcabin_clname;
    Passengers.SeatDescription = OLDSeatDescrPassengers;
    condSeats.SeatDescription = OLDSeatDescrCondSeats;
    Passengers.KTube = OLDKTube;
    Passengers.KWindow = OLDKWindow;
    Passengers.UseSmoke = OLDUseSmoke;
    Passengers.counters.Set_p_Count_3( OLDp_Count_3G, sRight );
    Passengers.counters.Set_p_Count_2( OLDp_Count_2G, sRight );
    Passengers.counters.Set_p_Count( OLDp_CountG, sRight );
    Passengers.counters.Set_p_Count_3( OLDp_Count_3V, sDown );
    Passengers.counters.Set_p_Count_2( OLDp_Count_2V, sDown );
    throw;
  }
  FindSUBCLS = OLDFindSUBCLS;
  canUseSUBCLS = OLDcanUseSUBCLS;
  SUBCLS_REM = OLDSUBCLS_REM;
  Passengers.Clear();
  Passengers.copyFrom( npass );
  Passengers.cabin_clname = OLDcabin_clname;
  Passengers.SeatDescription = OLDSeatDescrPassengers;
  condSeats.SeatDescription = OLDSeatDescrCondSeats;
  Passengers.KTube = OLDKTube;
  Passengers.KWindow = OLDKWindow;
  Passengers.UseSmoke = OLDUseSmoke;
  AllowedAttrsSeat.pr_isWorkINFT = isWorkINFT;
  Passengers.counters.Set_p_Count_3( OLDp_Count_3G, sRight );
  Passengers.counters.Set_p_Count_2( OLDp_Count_2G, sRight );
  Passengers.counters.Set_p_Count( OLDp_CountG, sRight );
  Passengers.counters.Set_p_Count_3( OLDp_Count_3V, sDown );
  Passengers.counters.Set_p_Count_2( OLDp_Count_2V, sDown );

  for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
    if ( !ipass->InUse ) {
      SeatsStat.stop("TSeatPlaces::SeatsPassengers");
      return false;
    }
  }
  SeatsStat.stop("TSeatPlaces::SeatsPassengers");
  return true;
}
///////////////////////////////////////////////
void TPassenger::set_seat_no()
{
  seat_no.clear();
  int x = Pos.x;
  int y = Pos.y;
  for ( int j=0; j<countPlace; j++ ) {
    TSeat s;
    strcpy( s.line, placeList->GetXsName( x ).c_str() );
    strcpy( s.row, placeList->GetYsName( y ).c_str() );
    seat_no.push_back( s );
    switch( (int)Step ) {
        case sRight:
            x++;
            break;
        case sDown:
            y++;
            break;
    }
  }
}

TSublsRems::TSublsRems( const std::string &vairline )
{
    SeatsStat.start(__FUNCTION__);
    airline = vairline;
    TQuery Qry(&OraSession );
    Qry.SQLText =
     "SELECT subclass,rem FROM comp_subcls_sets "
     " WHERE airline=:airline ";
    Qry.CreateVariable( "airline", otString, vairline );
    Qry.Execute();
    while ( !Qry.Eof ) {
        TSublsRem r;
        r.subclass = Qry.FieldAsString( "subclass" );
        r.rem = Qry.FieldAsString( "rem" );
        rems.push_back( r );
        Qry.Next();
    }
    SeatsStat.stop(__FUNCTION__);
}


bool TSublsRems::IsSubClsRem( const string &subclass, string &vrem )
{
    vrem.clear();
    for ( vector<TSublsRem>::iterator i=rems.begin(); i!=rems.end(); i++ ) {
        if ( i->subclass == subclass ) {
            vrem = i->rem;
            break;
        }
    }
  return !vrem.empty();
}

void TPassenger::add_rem( const std::string &code, const TRemGrp& remGrp )
{
    if ( isREM_SUBCLS( code ) )
        SUBCLS_REM = code;
    rems.push_back( code );
    LogTrace(TRACE5) << code << " " << remGrp.exists( code );
    ignore_tariff |= remGrp.exists( code );
}

void TPassenger::remove_rem( const std::string &code,
                             const std::map<std::string, int> &remarks,
                             const TRemGrp& remGrp )
{
  if ( isREM_SUBCLS( code ) ) {
    SUBCLS_REM.clear();
  }
  if ( code == maxRem ) {
    maxRem.clear();
  }
  bool ch = false;
  std::vector<std::string>::iterator irem = std::find( rems.begin(), rems.end(), code );
  if ( irem != rems.end() ) {
    rems.erase( irem );
    ch = ignore_tariff && remGrp.exists( code );
  }
  if ( ch ) {
    ignore_tariff = false;
    for (vector<string>::iterator ir=rems.begin(); ir!=rems.end(); ) {
      if ( remGrp.exists( code ) ) {
        ignore_tariff = true;
        break;
      }
    }
  }

  calc_priority( remarks );
}

void TPassenger::calc_priority( const std::map<std::string, int> &remarks )
{
  for (vector<string>::iterator ir=rems.begin(); ir!=rems.end(); ) {
    if ( remarks.find( *ir ) == remarks.end() )
        ir = rems.erase( ir );
    else
        ir++;
  }
  priority = 0;
  if ( !preseat_no.empty() ) //???
    priority = PR_N_PLACE;
  if ( !placeRem.empty() )
    priority += PR_REMPLACE;
  if ( prSmoke )
    priority += PR_SMOKE;
  int vpriority = 0;
  maxRem.clear();
  for ( std::vector<string>::const_iterator irem = rems.begin(); irem != rems.end(); irem++ ) {
    std::map<std::string, int>::const_iterator iremark_prior = remarks.find( *irem );
    if ( iremark_prior != remarks.end() ) {
      if ( iremark_prior->second > vpriority ) {
        vpriority = iremark_prior->second;
        maxRem = *irem;
      }
      priority += iremark_prior->second*countPlace; //???
    }
  }
  //???  priority += priority*countPlace;
}

void TPassenger::get_remarks( std::vector<std::string> &vrems ) const
{
  vrems = rems;
}

bool TPassenger::isRemark( std::string code ) const
{
  return ( std::find( rems.begin(), rems.end(), code ) != rems.end() );
}

bool TPassenger::is_valid_seats( const std::vector<SALONS2::TPlace> &places )
{
  for ( std::vector<string>::iterator irem=rems.begin(); irem!= rems.end(); irem++ ) {
    for( std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
     /* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      std::vector<SALONS2::TRem>::const_iterator itr;
      for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ )
        if ( *irem == itr->rem && itr->pr_denial )
          return false;
    }
  }
  for( std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
    if ( !ipl->layers.empty() &&
         SALONS2::isUserProtectLayer( ipl->layers.begin()->layer_type ) &&
         ipl->layers.begin()->pax_id != paxId )
      return false;
  }
  return AllowedAttrsSeat.passSeats( pers_type, isRemark( "INFT" ), places );
}

/*//////////////////////////////// CLASS TPASSENGERS ///////////////////////////////////*/
TPassengers::TPassengers()
{
  Clear();
}

TPassengers::~TPassengers()
{
  Clear();
}

void TPassengers::Clear()
{
  FPassengers.clear();
  KTube = false;
  KWindow = false;
  cabin_clname.clear();
  UseSmoke = false;
  SeatDescription = false;
  counters.Clear();
  wo_aisle = false;
  issubgrp = false;
}

void TPassengers::copyTo( VPassengers &npass )
{
  npass.clear();
  npass.assign( FPassengers.begin(), FPassengers.end() );
}

void TPassengers::copyFrom( VPassengers &npass )
{
  FPassengers.clear();
  FPassengers.assign( npass.begin(), npass.end() );
}

bool greatIndex( const TPassenger &p1, const TPassenger &p2 )
{
  return p1.index < p2.index;
}

void TPassengers::sortByIndex()
{
  sort( FPassengers.begin(), FPassengers.end(), greatIndex );
}

void TPassengers::operator = (TPassengers &items)
{
  Clear();
  for ( int i=0; i<items.getCount(); i++ ) {
    TPassenger &pass = items.Get( i );
    Add( pass, pass.index );
  }
}

bool TPassengers::withBaby()
{
  for ( int i=0; i<getCount(); i++ ) {
    TPassenger &pass = Get( i );
    if ( pass.isRemark( "INFT" ) ) {
       return true;
    }
  }
  return false;
}

bool TPassengers::withCHIN()
{
  for ( int i=0; i<getCount(); i++ ) {
    TPassenger &pass = Get( i );
    if ( pass.isRemark( "CHIN" ) ) {
       return true;
    }
  }
  return false;
}


void TPassengers::Add( TPassenger &pass )
{
  Add( pass, (int)FPassengers.size() );
}

void TPassengers::Add( TPassenger &pass, int index )
{
  bool Pr_PLC = false;
  if ( pass.countPlace > 1 && pass.isRemark( string( "STCR" ) )	 ) {
    pass.Step = sDown;
    switch( pass.countPlace ) {
      case 2: counters.Add_p_Count_2( 1, sDown );
              break;
      case 3: counters.Add_p_Count_3( 1, sDown );
    }
    Pr_PLC = true;
  }
  if ( !Pr_PLC ) {
   pass.Step = sRight;
   switch( pass.countPlace ) {
     case 1: counters.Add_p_Count( 1, sRight );
             break;
     case 2: counters.Add_p_Count_2( 1, sRight );
             break;
     case 3: counters.Add_p_Count_3( 1, sRight );
   }
  }
 // �����뢠�� �����
  if ( cabin_clname.empty() && !pass.cabin_clname.empty() )
    cabin_clname = pass.cabin_clname;
 // �����뢠�� �ਮ���
  if ( remarks.empty() )
    SALONS2::LoadCompRemarksPriority( remarks );
  pass.calc_priority( remarks );
  if ( pass.placeRem.find( "SW" ) == 2 )
    KWindow = true;
  if ( pass.placeRem.find( "SA" ) == 2 )
    KTube = true;
  if ( pass.placeRem.find( "SM" ) == 0 )
    this->UseSmoke = true;
  vector<TPassenger>::iterator ipass;
  for ( ipass=FPassengers.begin(); ipass!=FPassengers.end(); ipass++ ) {
    if ( pass.priority > ipass->priority )
      break;
  }
  pass.index = index;
//  ProgTrace( TRACE5, "pass.index=%d, pass.paxId=%d", pass.index, pass.paxId );
  if ( ipass == FPassengers.end() )
    FPassengers.push_back( pass );
  else
    FPassengers.insert( ipass, pass );
}

void TPassengers::Add( SALONS2::TSalons &Salons, TPassenger &pass )
{
  if ( pass.countPlace > CONST_MAXPLACE || pass.countPlace <= 0 )
   throw EXCEPTIONS::Exception( "�� �����⨬�� ���-�� ���� ��� �ᠤ��" );
  pass.preseatPlaces.clear();

  size_t i = 0;
  for (; i < pass.preseat_no.size(); i++)
    if ( pass.preseat_no[ i ] != '0' )
      break;
  if ( i )
    pass.preseat_no.erase( 0, i );
  pass.preseat_layer = cltUnknown; // ����砥�, �� ����� �����稫 ����, ��� �� ���� �� ������-����� ᫮� - ���� �஢�����
  pass.preseat_pax_id = 0;
  // ��।������ ᠬ��� �ਮ��⭮�� ᫮� � ����� ���� ��� ���ᠦ��
  if ( pass.grp_status == cltTranzit ) {
     pass.preseat_layer = cltProtTrzt;
  }
  else { // ��।������ �ਮ��⭮�� ᫮� ��� ���ᠦ��
    SALONS2::TPlaceList *placeList;
    for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
          plList!=Salons.placelists.end(); plList++ ) {
      placeList = *plList;
      for ( IPlace i=placeList->places.begin(); i!=placeList->places.end(); i++ ) {
        if ( !i->visible ||
             !i->isplace ||
             !i->isPax ||
             (!i->layers.empty() && i->layers.begin()->pax_id <= 0) ||
             i->layers.begin()->pax_id != pass.paxId )
          continue;
        if ( pass.preseat_layer == cltUnknown ) {
          TPoint p( i->x, i->y );
          pass.preseat_no = i->denorm_view(Salons.getLatSeat());
          pass.preseat_layer = i->layers.begin()->layer_type;
          pass.preseat_pax_id = i->layers.begin()->pax_id;
        }
        tst();
        pass.preseatPlaces.push_back( TCoordSeat( (*plList)->num, i->x, i->y ) );
      }
    }
  }
  Add( pass );
}

void TPassengers::Add( const SALONS2::TSalonList &salonList, TPassenger &pass )
{
  if ( pass.countPlace > CONST_MAXPLACE || pass.countPlace <= 0 )
   throw EXCEPTIONS::Exception( "�������⨬�� ���-�� ���� ��� �ᠤ��" );
  pass.preseatPlaces.clear();

  size_t i = 0;
  for (; i < pass.preseat_no.size(); i++)
    if ( pass.preseat_no[ i ] != '0' )
      break;
  if ( i )
    pass.preseat_no.erase( 0, i );
  pass.preseat_layer = cltUnknown; // ����砥�, �� ����� �����稫 ����, ��� �� ���� �� ������-����� ᫮� - ���� �஢�����
  pass.preseat_pax_id = 0;
  // ��।������ ᠬ��� �ਮ��⭮�� ᫮� � ����� ���� ��� ���ᠦ��
  if ( pass.grp_status == cltTranzit ) {
       pass.preseat_layer = cltProtTrzt;
  }
  else { // ��।������ �ਮ��⭮�� ᫮� ��� ���ᠦ��
    std::map<int,TPaxList>::const_iterator ipaxlist = salonList.pax_lists.find( salonList.getDepartureId() );
    if ( ipaxlist != salonList.pax_lists.end() ) { // ��諨 �㭪� �뫥�
      std::map<int,TSalonPax>::const_iterator ipax = ipaxlist->second.find( pass.paxId );
      if ( ipax != ipaxlist->second.end() ) {
        tst();
        for ( TLayersPax::const_iterator ilayer=ipax->second.layers.begin();
              ilayer!=ipax->second.layers.end(); ilayer++ ) {
          if ( ilayer->second.waitListReason.status == layerValid ) {
            //�.�. ���� ���஢�� ����, � �뤠�� ��ࢮ�
            pass.preseat_layer = ilayer->first.layerType();
            pass.preseat_pax_id = pass.paxId;
            std::set<TPlace*,CompareSeats>::const_iterator iseat = ilayer->second.seats.begin();
            pass.preseat_no = (*iseat)->denorm_view(salonList.isCraftLat());
            TCoordSeat coord;
            coord.placeListIdx = -1;
            for ( std::vector<TPlaceList*>::const_iterator plList=salonList._seats.begin();
                  plList!=salonList._seats.end(); plList++ ) {
              TPlaceList* placeList = *plList;
              TPoint p;
              tst();
              if ( placeList->GetisPlaceXY( pass.preseat_no, p ) ) {
                coord.placeListIdx = placeList->num;
                break;
              }
            }
            if ( coord.placeListIdx >= 0 ) {
              for ( std::set<TPlace*,CompareSeats>::const_iterator iseat=ilayer->second.seats.begin();
                    iseat!=ilayer->second.seats.end(); iseat++ ) {
                tst();
                coord.p.x = (*iseat)->x;
                coord.p.y = (*iseat)->y;
                pass.preseatPlaces.push_back( coord );
              }
            }
            else {
              ProgError( STDLOG, "placeList not found %s", pass.preseat_no.c_str() );
            }
            break;
          }
        }
      }
    }
  }
  Add( pass );
}

int TPassengers::getCount() const
{
  return FPassengers.size();
}

TPassenger &TPassengers::Get( int Idx )
{
  if ( Idx < 0 || Idx >= (int)FPassengers.size() )
    throw EXCEPTIONS::Exception( "Passeneger index out of range" );
  return FPassengers[ Idx ];
}

void TPassengers::SetCountersForPass( TPassenger  &pass )
{
  counters.Clear();
  switch( pass.countPlace ) {
    case 1:
       counters.Add_p_Count( 1 );
       break;
    case 2:
       counters.Add_p_Count_2( 1, pass.Step );
       break;
    case 3:
       counters.Add_p_Count_3( 1, pass.Step );
       break;
  }
}

bool TSeatPlaces::LSD( int G3, int G2, int G, int V3, int V2, TWhere Where )
{
  if ( SeatAlg == sSeatPassengers )
    return true;
  SeatsStat.start(__FUNCTION__);
  /* �᫨ �� ����� � ⮣�� �� ᬮ��� ��ᠤ��� �������� 祫-�� �� ��㯯�
     ���஡㥬 ��ᠤ��� ��� ��⠫��� */
  Passengers.counters.Clear();
  Passengers.counters.Add_p_Count_3( G3, sRight );
  Passengers.counters.Add_p_Count_2( G2, sRight );
  Passengers.counters.Add_p_Count( G );
  Passengers.counters.Add_p_Count_3( V3, sDown );
  Passengers.counters.Add_p_Count_2( V2, sDown );
  TUseRem OldCanUseRems = CanUseRems;
  CanUseRems = sIgnoreUse;
  try {
    if ( SeatsStayedSubGrp( Where ) ) {
      CanUseRems = OldCanUseRems;
      SeatsStat.stop(__FUNCTION__);
      return true;
    }
  }
  catch( ... ) {
    CanUseRems = OldCanUseRems;
    SeatsStat.stop(__FUNCTION__);
    throw;
  }
  CanUseRems = OldCanUseRems;
  SeatsStat.stop(__FUNCTION__);
  return false;
}


void SetLayers( SALONS2::TSalons *Salons,
                vector<TCompLayerType> &Layers,
                bool &CanUseMutiLayer,
                TCompLayerType layer,
                int Step,
                const TUseLayers &preseat_layers,
                TClientType client_type )
{
  Layers.clear();
  CanUseMutiLayer = ( Step <= -1 );
  bool pr_uncomfort = Salons->isExistBaseLayer(cltUncomfort);
  //�� ࠡ�⠥� � ����騬� ���⠬�
  switch ( layer ) {
    case cltTranzit:
    case cltProtTrzt:
      if ( Step != 0 ) {
        Layers.push_back( cltProtTrzt );
        Layers.push_back( cltUnknown );
        if ( pr_uncomfort ) {
          Layers.push_back( cltUncomfort );
        }
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm ||
             client_type == ASTRA::ctPNL )
          Layers.push_back( cltProtect );
        Layers.push_back( cltPNLCkin );
        for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltProtect:
      if ( Step != 0 ) {
        if ( client_type == ASTRA::ctTerm ||
             client_type == ASTRA::ctPNL )
          Layers.push_back( cltProtect );
        Layers.push_back( cltUnknown );
      };
      if ( Step != 1 ) {
        if ( pr_uncomfort ) {
          Layers.push_back( cltUncomfort );
        }
        Layers.push_back( cltPNLCkin );
        for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltUnknown:
      if ( Step != 0 ) {
        Layers.push_back( cltUnknown );
        if ( pr_uncomfort ) {
          Layers.push_back( cltUncomfort );
        }
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm ||
             client_type == ASTRA::ctPNL )
          Layers.push_back( cltProtect );
        Layers.push_back( cltPNLCkin );
        for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltPNLCkin:
      if ( Step != 0 ) {
        Layers.push_back( cltPNLCkin );
        Layers.push_back( cltUnknown );
        if ( pr_uncomfort ) {
          Layers.push_back( cltUncomfort );
        }
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm ||
             client_type == ASTRA::ctPNL )
          Layers.push_back( cltProtect );
        for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltProtCkin:
    case cltPNLBeforePay: //???
    case cltProtBeforePay:
    case cltPNLAfterPay: //???
    case cltProtAfterPay:
    case cltProtSelfCkin:
        if ( Step != 0 ) {
        for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
        Layers.push_back( cltUnknown );
        if ( pr_uncomfort ) {
          Layers.push_back( cltUncomfort );
        }
      }
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm ||
             client_type == ASTRA::ctPNL )
          Layers.push_back( cltProtect );
        Layers.push_back( cltPNLCkin );
      }
      break;
    default:;
  }
}

/*///////////////////////////END CLASS TPASSENGERS/////////////////////////*/

bool ExistsBasePlace( const TTripInfo& fltInfo,
                      const BASIC_SALONS::TCompLayerTypes::Enum& flag,
                      SALONS2::TSalons &Salons, TPassenger &pass )
{
  SALONS2::TPlaceList *placeList;
  SALONS2::TPoint FP;
  vector<SALONS2::TPlace*> vpl;
  string placeName = pass.preseat_no;
  for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
        plList!=Salons.placelists.end(); plList++ ) {
    placeList = *plList;
    if ( placeList->GetisPlaceXY( placeName, FP ) ) {
      int j = 0;
      for ( ; j<pass.countPlace; j++ ) {
        if ( !placeList->ValidPlace( FP ) )
          break;
        SALONS2::TPlace *place = placeList->place( FP );
        bool findpass = !pass.SUBCLS_REM.empty();
        bool findplace = false;
        for ( vector<SALONS2::TRem>::iterator r=place->rems.begin(); r!=place->rems.end(); r++ ) {
            if ( !r->pr_denial && r->rem == pass.SUBCLS_REM ) {
                findplace = true;
                break;
            }
        }
        ProgTrace( TRACE5, "Salons.placeIsFree( place )=%d, seat_no=%s", Salons.placeIsFree( place ), string(place->yname+place->xname).c_str() );
        for ( std::vector<TPlaceLayer>::iterator i=place->layers.begin(); i!=place->layers.end(); i++ ) {
            ProgTrace( TRACE5, "layer_type=%s", EncodeCompLayerType(i->layer_type) );
        }

        if ( !place->visible || !place->isplace ||
             !Salons.placeIsFree( place ) || pass.cabin_clname != place->clname ||
             findpass != findplace )
          break;
        vpl.push_back( place );
        switch( (int)pass.Step ) {
          case sRight:
             FP.x++;
             break;
          case sDown:
             FP.y++;
             break;
        }
      }
      if ( j == pass.countPlace ) {
        for ( vector<SALONS2::TPlace*>::iterator ipl=vpl.begin(); ipl!=vpl.end(); ipl++ ) {
          if ( ipl == vpl.begin() ) {
            pass.InUse = true;
            pass.placeList = placeList;
            pass.Pos.x = (*ipl)->x;
            pass.Pos.y = (*ipl)->y;
            pass.set_seat_no();
          }
          (*ipl)->AddLayerToPlace( pass.grp_status, 0, pass.paxId, NoExists, NoExists,
                                   BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, pass.grp_status ),
                                                                                        flag ) );
          if ( CurrSalon::get().canAddOccupy( *ipl ) ) {
            CurrSalon::get().AddOccupySeat( placeList->num, (*ipl)->x, (*ipl)->y );
          }
        }
        return true;
      }
      vpl.clear();
    }
  }
  return false;
}

void SeatsPassengersGrps( SALONS2::TSalons *Salons,
                          TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                          TClientType client_type,
                          TRFISCMode UseRFISCMode,
                          TPassengers &passengers,
                          const std::map<int,TPaxList> &pax_lists,
                          const TRemGrp& remGrp );

/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( SALONS2::TSalonList &salonList,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                      TClientType client_type,
                      TPassengers &passes,
                      SALONS2::TAutoSeats &autoSeats,
                      const TRemGrp& remGrp )
{
  SeatsStat.clear();
  SeatsStat.deactivate();
  ProgTrace( TRACE5, "salonList NEWSEATS, ASeatAlgoParams=%d", (int)ASeatAlgoParams.SeatAlgoType );
  if ( !passes.getCount() )
    return;
  SeatsStat.start("SeatsPassengers(SalonList)");
  /* ���� �����⮢��� ��६����� CurrSalon �� �᭮�� salonList */
  TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
  BASIC_SALONS::TCompLayerTypes::Enum flag = (GetTripSets( tsAirlineCompLayerPriority, salonList.getfltInfo() )?
                                                  BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                                  BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
  TPaxsCover paxs;
  TCreateSalonPropFlags propFlags;
  for ( int i=0; i<passes.getCount(); i++ ) {
    TPassenger &pass = passes.Get( i );
    if ( SALONS2::isUserProtectLayer( pass.preseat_layer ) ) {
      if ( pass.preseat_layer == cltProtBeforePay ||
           pass.preseat_layer == cltPNLBeforePay ||
           pass.preseat_layer == cltProtAfterPay ||
           pass.preseat_layer == cltPNLAfterPay ) {
        propFlags.setFlag( clDepOnlyTariff );
      }
    }
    paxs.push_back( SALONS2::TPaxCover( pass.paxId, ASTRA::NoExists ) );
  }
  VPassengers rollbackPasses;
  passes.copyTo( rollbackPasses );
  int countP=0;
  vector<ASTRA::TCompLayerType> grp_layers;
  grp_layers.push_back( passes.Get( 0 ).grp_status );
  TSalons SalonsN;
  TDropLayersFlags dropLayersFlags;
  bool pr_baby_zones = false;

  while (salonList.CreateSalonsForAutoSeats( SalonsN,
                                             filterRoutes,
                                             propFlags,
                                             grp_layers,
                                             paxs,
                                             dropLayersFlags ) ) {
    CurrSalon::set( salonList.getAirline(), flag, &SalonsN );
    countP++;
    if ( countP == 20 ) {  //�����쪠� ���⪠
      ProgError( STDLOG, "SeatsPassengers dead loop!!!" );
      break;
    }
    passes.Clear();
    passes.copyFrom( rollbackPasses );
    passes.sortByIndex();
/*    for ( VPassengers::iterator ipass=rollbackPasses.begin(); ipass!=rollbackPasses.end(); ipass++ ) {
      passes.Add( *ipass );
    }*/
    autoSeats.clear();
    std::set<TPlace*,CompareSeats> values;
    try {
      SeatsPassengersGrps( &CurrSalon::get(),
                           ASeatAlgoParams,
                           client_type,
                           salonList.getRFISCMode(),
                           passes,
                           salonList.pax_lists,
                           remGrp );
      for ( int i=0; i<passes.getCount(); i++ ) {
        values.clear();
        TPassenger &pass=passes.Get( i );
        SALONS2::TAutoSeat seat;
        seat.pax_id = pass.paxId;
        ProgTrace( TRACE5, "seat.pax_id=%d", seat.pax_id );
        //ProgTrace( TRACE5, "seat.pax_id=%d,pass.placeList=%p,pass.Pos.x=%d,pass.Pos.y=%d", seat.pax_id, pass.placeList,pass.Pos.x,pass.Pos.y );
        seat.point.num = pass.placeList->num;
        seat.point.x = pass.Pos.x;
        seat.point.y = pass.Pos.y;
        seat.pr_down = ( pass.Step == sDown ||
                         pass.Step == sUp );
        seat.seats = pass.countPlace;
        TPoint seat_p( pass.Pos.x, pass.Pos.y );
        for ( int j=0; j<pass.countPlace; j++ ) {
          TPlace *place = pass.placeList->place( seat_p );
          values.insert( place );
          if ( seat.pr_down ) {
           seat_p.y++;
          }
          else {
            seat_p.x++;
          }
        }
        for ( std::set<TPlace*,CompareSeats>::iterator iseat=values.begin();
              iseat!=values.end(); iseat++ ) {
          TSeat s( (*iseat)->yname, (*iseat)->xname );
          seat.ranges.push_back( s );
          seat.descrs.insert( make_pair( s, SEAT_DESCR::descrSeatTypeOrigin( salonList.getAirline(), CurrSalon::get().trip_id, salonList.getRFISCMode(), pass.placeList,
                                                                                 (*iseat)->x, (*iseat)->y  ).toPropHintFormatStr() ) );
          ProgTrace( TRACE5, "autoSeats.push_back pax_id=%d, seats=%d, yname=%s, xname=%s, descr=%s",
                     seat.pax_id, seat.seats, (*iseat)->yname.c_str(), (*iseat)->xname.c_str(), seat.descrs[ s ].c_str() );
        }
        ProgTrace( TRACE5, "autoSeats.push_back pax_id=%d, seats=%d", seat.pax_id, seat.seats );
        autoSeats.push_back( seat );
      }
      SeatsStat.stop("SeatsPassengers(SalonList)");
      for ( StatisticProps<std::string>::const_iterator i=SeatsStat.begin(); i!=SeatsStat.end(); i++ ) {
        ProgTrace( TRACE5, "%s, count=%ld, time=%llu", i->first.c_str(), i->second.count, i->second.execTime );
      }
      return;
    }
    catch(const UserException& ue ) {
      ProgTrace( TRACE5, "UserException.what()=%s", ue.getLexemaData().lexema_id.c_str() );
      if ( ue.getLexemaData().lexema_id == string( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS" ) ||
           ue.getLexemaData().lexema_id == string( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS.BABY_ZONES" )) {
        tst();
        pr_baby_zones = (ue.getLexemaData().lexema_id == string( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS.BABY_ZONES" ));
      }
      else {
        for ( StatisticProps<std::string>::const_iterator i=SeatsStat.begin(); i!=SeatsStat.end(); i++ ) {
          ProgTrace( TRACE5, "%s, count=%ld, time=%llu", i->first.c_str(), i->second.count, i->second.execTime );
        }
        SeatsStat.stop("SeatsPassengers(SalonList)");
        throw;
      }
    }
  } //while ( salonList.CreateSalonsForAutoSeats( CurrSalon,
  SeatsStat.stop("SeatsPassengers(SalonList)");
  for ( StatisticProps<std::string>::const_iterator i=SeatsStat.begin(); i!=SeatsStat.end(); i++ ) {
     ProgTrace( TRACE5, "%s, count=%ld, time=%llu", i->first.c_str(), i->second.count, i->second.execTime );
  }
  if ( pr_baby_zones ) {
    throw UserException( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS.BABY_ZONES" );
  }
  throw UserException( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS" );
}

std::string separatelyRem( const std::string &rem, int idx )
{
  ostringstream rem_variant;
  if ( rem.empty() ) {
    rem_variant << "ZZZZ" << "999";
  }
  else {
    rem_variant << rem <<  setw(3) << idx;
  }
  return rem_variant.str();
}

bool isPassPay( const TCompLayerType& comp_layer_type )
{
  if ( SALONS2::isUserProtectLayer( comp_layer_type ) ) {
    if ( comp_layer_type == cltProtBeforePay ||
         comp_layer_type == cltPNLBeforePay ||
         comp_layer_type == cltProtAfterPay ||
         comp_layer_type == cltPNLAfterPay )
      return true;
  }
  return false;
}

struct TAdultWithBabys {
  int index;
  std::vector<int> indexs_childs;
  int infts;
  TAdultWithBabys( int vindex ) {
    index = vindex;
    infts = 0;
  }
  int priority() {
    return (int)indexs_childs.size() + infts*10;
  }
};

struct CompareGrp {
  bool operator() ( const std::string grp_status1, const std::string grp_status2  ) const {
    TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
     const TGrpStatusTypesRow &row1 = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status1 );
     const TGrpStatusTypesRow &row2 = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", grp_status2 );
    if ( row1.priority != row2.priority ) {
      return ( row1.priority < row2.priority );
    }
    return false;
  }
};

class TAdulstWithBabys:public std::map<std::string,std::vector<TAdultWithBabys>>
{
  private:
    bool separately_seat_adult_with_baby;
    bool separately_seat_chin_emergency;
    bool issubgrp;
  public:
    TAdulstWithBabys() {
      separately_seat_adult_with_baby = false;
      separately_seat_chin_emergency = false;
      issubgrp = false;
    }
    TAdulstWithBabys( bool vseparately_seat_adult_with_baby,
                      bool vseparately_seat_chin_emergency ) {
      separately_seat_adult_with_baby = vseparately_seat_adult_with_baby;
      vseparately_seat_chin_emergency = vseparately_seat_chin_emergency;
    }
    void setAdultToGrp( const TPassenger& pass ) {
      TAdultWithBabys adult( pass.index );
      ostringstream grp_variant;
      bool ignoreINFT = !AllowedAttrsSeat.pr_isWorkINFT;
      bool pr_pay = isPassPay( pass.preseat_layer );
      grp_variant << pass.cabin_clname;
      bool isInft = pass.isRemark("INFT");
      grp_variant << separatelyRem( isInft && separately_seat_adult_with_baby?"INFT":"", pass.index );
      if (isInft) adult.infts++;
      bool isChin = pass.isRemark("CHIN");
      grp_variant << separatelyRem( isChin && separately_seat_adult_with_baby?"CHIN":"", pass.index );
      //��� � �����
      grp_variant << pr_pay << pass.ignore_tariff << (ignoreINFT || pass.isRemark( "INFT" ));
      //���
      grp_variant << pass.tariffs.key() << EncodeCompLayerType(pass.preseat_layer) << pass.dont_check_payment;
      ProgTrace( TRACE5, "grp_variant=%s, pax=%s", grp_variant.str().c_str(), pass.toString().c_str() );
      ProgTrace( TRACE5, "adult.index=%d, infts=%d", adult.index, adult.infts );
      this->emplace( make_pair(grp_variant.str(), std::vector<TAdultWithBabys>() ) ).first->second.push_back(adult);
    }
    void addAdult( const TPassenger& pass ) {
      setAdultToGrp( pass );
    }
    void setChildToAdult( const TPassenger& child_pass ) {
      issubgrp = true;
      int pos_adult_priority = -1;
      int priority = 100;
      std::string Key;
      ProgTrace(TRACE5, "setChildToAdult child index=%d",child_pass.index);
      //�஡�� �� ��㯯��
      for ( auto adults : *this ) {
        for ( std::vector<TAdultWithBabys>::iterator iadult=adults.second.begin(); iadult!=adults.second.end(); iadult++ ) {
          int apriority = iadult->priority();
      //    ProgTrace(TRACE5, "adult index=%d, priority=%d",iadult->index, apriority);
          if ( apriority < priority ) {
            Key = adults.first;
            pos_adult_priority = std::distance( adults.second.begin(), iadult );
            priority = apriority;
            ProgTrace(TRACE5, "set adult Key grp %s priority %d  index=%d", Key.c_str(), priority, adults.second.at(pos_adult_priority).index);
          }
        }
      }
      tst();
      if ( pos_adult_priority >= 0 ) {
        auto& i = (*this)[ Key ];
        ProgTrace(TRACE5, "set adult(%d) child index=%d",i.at(pos_adult_priority).index,child_pass.index);
        i.at(pos_adult_priority).indexs_childs.emplace_back( child_pass.index );
        ProgTrace(TRACE5, "%s", Key.c_str() );
        ProgTrace(TRACE5, " adults vector size=%zu, adult index=%d", i.size(), i.at(pos_adult_priority).index );
        //� ��㯯� ����� ������ 祫�����, ⮣�� �뤥�塞 �⮣� ���᫮�� � �⤥���� ��㯯�
        if ( i.size() > 1 ) {
          //㤠�塞 �� ��饩 ��㯯� � ������ �⤥����
          ProgTrace(TRACE5, "%d, %zu", i.at(pos_adult_priority).index, i.size() );
          Key = Key + std::to_string( i.at(pos_adult_priority).index );
          ProgTrace(TRACE5, "%s", Key.c_str() );
          this->emplace( make_pair(Key, std::vector<TAdultWithBabys>() ) ).first->second.push_back(i.at(pos_adult_priority));
          i.erase( i.begin() + pos_adult_priority );
        }
      }
      else {
         ProgError(STDLOG, "setChildToAdult error");
         addAdult( child_pass );
      }
    }

    void getGrps( vector<TPassengers> &passGrps,
                  TPassengers &passengers ) {

      passGrps.clear();
      std::map<int,std::vector<TPassengers>,std::greater<int>> sortByCountPassengers;
      for ( const auto& igrp : *this ) {
        TPassengers ps;
        for ( const auto& p : igrp.second ) {
         TPassenger pass = passengers.Get( p.index );
          ps.Add( pass, pass.index );
          ps.issubgrp = issubgrp;
          for ( const auto &idx : p.indexs_childs ) {
            TPassenger pass = passengers.Get( idx );
            ps.Add( pass, pass.index );
            ps.wo_aisle = true;
          }
        }
        sortByCountPassengers.emplace( ps.getCount(), std::vector<TPassengers>() ).first->second.emplace_back(ps);
      }
      //�� �뢠���
      for ( const auto& vp : sortByCountPassengers ) {
        for ( const auto & p : vp.second ) {
          passGrps.emplace_back( p );
        }
      }
      //using Map = std::map<int, std::vector<TPassengers>>;
      //passGrps = algo::transform<std::vector>(sortByCountPassengers, [](const Map::value_type& kv) { return kv.second; });
      // � �� �ᥣ�� � ����
      if ( passengers.getCount() > 0 ) { //??? �������� �� ��直� ��砩, �⮡� �ࠡ�⠫ ���� ������ ��� ࠧ����� �� �����㯯�
        passGrps.emplace_back( passengers );
        ProgTrace( TRACE5, "all grp, count=%d", passengers.getCount() );
      }

      int i = 0;
      ostringstream logm;
      logm << "passGrps.size=" << passGrps.size() << "issubgrp=" << issubgrp << std::endl;
      for ( auto& g : passGrps ) {
        i++;
        logm << i << " wo_aisle=" << g.wo_aisle << std::endl;
        for ( int c=0; c<g.getCount(); c++ ) {
          logm << g.Get(c).toString() << std::endl;
        }
      }
      LogTrace(TRACE5) << logm.str();
    }
};

void dividePassengersToGrps( TPassengers &passengers, vector<TPassengers> &passGrps,
                             bool separately_seat_adult_with_baby,
                             bool separately_seat_chin_emergency,
                             const TRemGrp& remGrp )
{
  SeatsStat.start(__FUNCTION__);
  passGrps.clear();
  TPassengers p;
  //std::set<int> addedPasses;
  //for ( int icond=0; icond<4; icond++ ) { // AllowedAttrsSeat.pr_isWorkINFT ����� ��: 0=����� ॡ���� � 1=����� ����� , 2=��ᯫ��� ॡ����, 3=��ᯫ��� �����
  //std::map<std::string,TPassengers> v; //����� �� ��䠬
  TAdulstWithBabys adults(separately_seat_adult_with_baby,separately_seat_chin_emergency);
  for ( int ichild=0; ichild<=1; ichild++ ) {
    for ( int i=0; i<passengers.getCount(); i++ ) {
      TPassenger &pass = passengers.Get( i );
      if ( (pass.pers_type  == "��" && ichild == 1)||
           (pass.pers_type  != "��" && ichild == 0)) {
        continue;
      }
      if ( ichild == 0 ) {
        adults.addAdult( pass );
      }
      else {
        adults.setChildToAdult( pass );
      }
/*      ostringstream grp_variant;
      std::vector<std::string> vrems;
      pass.get_remarks( vrems );
      grp_variant << pass.cabin_clname
                  << separatelyRem( separately_seat_adult_with_baby?"INFT":"", vrems, pass.index )
                  << separatelyRem( separately_seat_chin_emergency?"CHIN":"", vrems, pass.index );
      //��� � �����
      grp_variant << pr_pay << pass.ignore_tariff << (ignoreINFT || pass.isRemark( "INFT" ));
      //���
      grp_variant << pass.tariffs.key() << EncodeCompLayerType(pass.preseat_layer) << pass.dont_check_payment;
      ProgTrace( TRACE5, "grp_variant=%s, pax=%s", grp_variant.str().c_str(), pass.toString().c_str() );
      v[ grp_variant.str() ].Add( pass, pass.index );*/
/*      if (
           (icond == 0 && pr_pay && (ignoreINFT || pass.isRemark( "INFT" ))) ||
           (icond == 1 && pr_pay && (ignoreINFT || !pass.isRemark( "INFT" ))) ||
           (icond == 2 && !pr_pay && (ignoreINFT || pass.isRemark( "INFT" ))) ||
           (icond == 3 && !pr_pay && (ignoreINFT || !pass.isRemark( "INFT" ))) ) {
        if ( addedPasses.find( pass.index ) != addedPasses.end() ) {
          tst();
          continue;
        }
        addedPasses.insert( pass.index  );
        ProgTrace( TRACE5, "pass.index=%d", pass.index );
        v[ pass.tariffs.key() + EncodeCompLayerType(pass.preseat_layer) + IntToString( (int)pass.dont_check_payment ) ].Add( pass, pass.index );

        ProgTrace( TRACE5, "dividePassengersToGrps: icond=%d, pass=%s, pass.tariffs.key()=%s", icond, pass.toString().c_str(), pass.tariffs.key().c_str() );*/
//          ProgTrace( TRACE5, "dividePassengersToGrps: j=%d, i=%d, pass.idx=%d, pass.pax_id=%d, INFT=%d", j,i, pass.paxId, pass.index, pass.isRemark( "INFT") );
      //}
    } // passs
  } //ichild
  //int grp_idx = 0;
  adults.getGrps( passGrps, passengers );
/*  for ( TAdulstWithBabys::iterator ip = adults.begin(); ip!=adults.end(); ip++ ) {
    if ( !ip->second.empty() ) {
      passGrps.push_back( adults.getPassengers(passengers) );
    //if ( ip->second.getCount() > 0 ) {
      passGrps.push_back( ip->second );*/
/*        ProgTrace( TRACE5, "grp_num=%d, variant=%s, count=%d", grp_idx, ip->first.c_str(), ip->second.getCount() );
      for ( int jk=0; jk<ip->second.getCount(); jk++ ) {
        ProgTrace( TRACE5, "%s", ip->second.Get( jk ).toString().c_str() );
      }
      ProgTrace( TRACE5, "=================================================" );*/
      //grp_idx++;
//    }
//  }
/*  if ( passengers.getCount() > 0 ) {
    p = passengers;
    passGrps.push_back( p ); //last element contain prior passengers variant

  }*/
  SeatsStat.stop(__FUNCTION__);
}

void SeatsPassengers( SALONS2::TSalons *Salons,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                      TClientType client_type,
                      bool pr_rollback,
                      bool separately_seats_adult_with_baby,
                      bool denial_emergency_seats,
                      TRFISCMode useRFISCMode,
                      TPassengers &passengers,
                      const std::vector<TCoordSeat> &paxsSeats,
                      const TRemGrp& remGrp );

void SeatsPassengersGrps( SALONS2::TSalons *Salons,
                          TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                          TClientType client_type,
                          TRFISCMode useRFISCMode,
                          TPassengers &passengers,
                          const std::map<int,TPaxList> &pax_lists,
                          const TRemGrp& remGrp )
{
  if ( !passengers.getCount() )
    return;
  SeatsStat.start(__FUNCTION__);
  //ࠧ��ꥬ ���ᠦ�஢ �� ��㯯��
  std::vector<TPassengers> passGrps;
  std::set<int> pax_lists_with_baby;
  if ( Salons->trip_id == ASTRA::NoExists ) {
    ProgError( STDLOG, "SeatsPassengersGrps: point_id = NoExists!!!" );
  }
  std::map<int,TPaxList>::const_iterator ipaxs = pax_lists.find( Salons->trip_id );
  if ( ipaxs != pax_lists.end() ) {
    ipaxs->second.setPaxWithInfant( pax_lists_with_baby );
  }
  AllowedAttrsSeat.isWorkINFT( Salons->trip_id ); //�㦭� ��� ���樠����樨 ��६����� pr_INFT
  TBabyZones babyZoness( CurrSalon::get().trip_id );
  TEmergencySeats emergencySeats( CurrSalon::get().trip_id );
  dividePassengersToGrps( passengers, passGrps,
                          babyZoness.useInfantSection(),
                          emergencySeats.deniedEmergencySection(),
                          remGrp
                          );
  // passengers - ᪮॥ �ᥣ� �� ������쭠� ��६�����, ���� �� ���������, �ᯮ�짮����, � ��⮬ ����⠭�����
  //� ��᫥���� ����� ����� - ��� ��㯯� �� ࠧ�����
  std::vector<TCoordSeat> paxsSeats;
  VSeatPlaces seatsGrps;
  for ( std::vector<TPassengers>::iterator ipassGrp=passGrps.begin(); ipassGrp!=passGrps.end(); ) {
    passengers = *ipassGrp;
    passengers.wo_aisle = ipassGrp->wo_aisle; //???
    passengers.issubgrp = ipassGrp->issubgrp; //???
    ProgTrace(TRACE5, "wo_aisle=%d, issubgrp=%d", passengers.wo_aisle, passengers.issubgrp );
    babyZoness.clear();
    emergencySeats.clear();
    bool separately_seats_adult_with_baby = babyZoness.useInfantSection() && passengers.withBaby();
    bool denial_emergency_seats = emergencySeats.deniedEmergencySection() && passengers.withCHIN();
    try {
      bool pr_rollback = ( !passGrps.empty() && ipassGrp == passGrps.end() - 1 ); //��� ��㯯�
      ProgTrace( TRACE5, "pr_rollback=%d, separately_seats_adult_with_baby=%d, passengers.getCount=%d", pr_rollback, separately_seats_adult_with_baby, passengers.getCount() );
      if ( separately_seats_adult_with_baby ||
           denial_emergency_seats ) {
        TPaxsCover paxs;
        for ( int i=0; i<passengers.getCount(); i++ ) {
          ProgTrace( TRACE5, "pax(%i).pax_id=%d", i, passengers.Get(i).paxId );
          paxs.push_back( TPaxCover( passengers.Get(i).paxId, ASTRA::NoExists ) );
        }
        if ( separately_seats_adult_with_baby ) {
          babyZoness.setDisabledBabySection( Salons, paxs, pax_lists_with_baby );
        }
        if ( denial_emergency_seats ) {
          emergencySeats.setDisabledEmergencySeats( Salons, paxs );
        }
      }
      SeatsPassengers( Salons,
                       ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                       client_type,
                       pr_rollback,
                       separately_seats_adult_with_baby,
                       denial_emergency_seats,
                       useRFISCMode,
                       passengers,
                       paxsSeats,
                       remGrp );
      babyZoness.rollbackDisabledBabySection( Salons );
      emergencySeats.rollbackDisabledEmergencySeats( Salons );
      //㪠����, �� �������� ���� �ਭ������� ���ᠦ�ࠬ � ���쬨!!!
      ipassGrp++;
      SeatPlaces >> seatsGrps; //��࠭塞 ����
      ProgTrace( TRACE5, "seatsGrps.size()=%zu", seatsGrps.size() );
      SeatPlaces.Clear();
      tst();
      if ( !passGrps.empty() ) {
        tst();
        if ( ipassGrp != passGrps.end() ) {
          std::vector<TPassengers>::iterator jpassGrp = passGrps.end() - 1;
          for ( int i=0; i<passengers.getCount(); i++ ) {
            TPassenger &pass0 = passengers.Get( i );
            for ( int j=0; j<jpassGrp->getCount(); j++ ) {
              TPassenger &pass1 = jpassGrp->Get( j );
              if ( pass0.index == pass1.index ) {
                pass1 = pass0;
                ProgTrace( TRACE5, "pass1 = %s", pass1.toString().c_str() );
                if ( pass1.isSeat ) {
                  paxsSeats.push_back( TCoordSeat( pass1.placeList->num, pass1.Pos.x, pass1.Pos.y ) );
                  ProgTrace( TRACE5, "paxsSeats add num=%d, x=%d, y=%d", pass1.placeList->num, pass1.Pos.x, pass1.Pos.y );
                }
                if ( separately_seats_adult_with_baby ) {
                  pax_lists_with_baby.insert( pass1.paxId );
                }
                ProgTrace( TRACE5, "i=%d, j=%d, paxId1=%d,paxId2=%d,indedx=%d, seat_no=(%d,%d)", i, j, pass0.paxId, pass1.paxId, pass1.index, pass1.Pos.x, pass1.Pos.y );
                break;
              }
            }
/*            TPassenger &pass = passengers.Get( i );
            TPassenger &pass1 = jpassGrp->Get( ipasscount );
            int idx = pass1.index;
            pass1 = pass;
            pass1.index = idx;
            ProgTrace( TRACE5, "ipasscount=%d, i=%d, indedx=%d, seat_no=(%d,%d)", ipasscount, i, idx, pass1.Pos.x, pass1.Pos.y );
            ipasscount++;*/
          }
          if ( ipassGrp == jpassGrp ) { // �����稫�, �����頥� ��� �室��� ��㯯� � ��६����� passengers
            passengers = *jpassGrp;
            tst();
            break;
          }
        }
        if ( ipassGrp == passGrps.end() - 1 ) {
          ProgError(STDLOG, "couldn't assign seats to subgrp passengers, but assign seats to old algo");
          break;
        }
      }
    }
    catch(...) {
      if ( denial_emergency_seats ||
           separately_seats_adult_with_baby ) {
        if ( denial_emergency_seats ) {
          emergencySeats.rollbackDisabledEmergencySeats( Salons );
        }
        if ( separately_seats_adult_with_baby ) {
          babyZoness.rollbackDisabledBabySection( Salons );
        }
        SeatsStat.stop(__FUNCTION__);
        throw;
      }
      if ( !passGrps.empty() && ipassGrp == passGrps.end() - 1 ) {
        SeatsStat.stop(__FUNCTION__);
        throw;
      }
      //�����㯯�
      if ( !passGrps.empty() ) {
        ipassGrp = passGrps.end() - 1; //��㯯�
      }
      SeatPlaces = seatsGrps; //���⠥� ����
      SeatPlaces.RollBack(); //�⪠�뢠��
      paxsSeats.clear();
      continue;
    }
  }
//  ProgTrace( TRACE5, "seatsGrps.size()=%zu", seatsGrps.size() );
  SeatPlaces = seatsGrps; //���⠥� ����
  SeatPlaces.RollBack(); //�⪠�뢠��
  paxsSeats.clear();
  passengers.sortByIndex();
  SeatsStat.stop(__FUNCTION__);
  tst();
}


bool UsedPayedPreseatForPassenger( const TTripInfo& fltInfo,
                                   BASIC_SALONS::TCompLayerTypes::Enum flag,
                                   TPlace &seat, const TPassenger &pass, bool check_rfisc  ) {
  if ( TReqInfo::Instance()->client_type == ctTerm && seat.SeatTariff.rate == 0.0 ) { // only for Term analize rates
    tst();
    return true;
  }
  if ( seat.SeatTariff.empty() ) { // for other analize color
    tst();
    return true;
  }

  if ( !seat.layers.empty() ) {
    if ( seat.layers.begin()->layer_type == cltProtBeforePay ||
         seat.layers.begin()->layer_type == cltProtAfterPay ||
         seat.layers.begin()->layer_type == cltPNLBeforePay ||
         seat.layers.begin()->layer_type == cltPNLAfterPay ||
         seat.layers.begin()->layer_type == cltProtSelfCkin ) {
      ProgTrace( TRACE5, "seat->pax_id=%d, pass.preseat_pax_id=%d",
                 seat.layers.begin()->pax_id, pass.preseat_pax_id );
      return seat.layers.begin()->pax_id == pass.preseat_pax_id; //�ਭ������� ���ᠦ���
    }
  }
  //� ���ᠦ�� ࠧ��⪠ � ����訬 �ਮ��⮬ 祬 ���⭮�
  BASIC_SALONS::TCompLayerTypes *compTypes = BASIC_SALONS::TCompLayerTypes::Instance();
  int priority = compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, pass.preseat_layer ), flag );
  if ( priority > compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltProtBeforePay ), flag )&&
       priority > compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltProtAfterPay ), flag )&&
       priority > compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltPNLBeforePay ), flag )&&
       priority > compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltPNLAfterPay ), flag ) &&
       priority > compTypes->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltProtSelfCkin ), flag ) ) {
    if ( pass.preseat_layer == cltProtCkin ) {
      if ( !check_rfisc ) {
        return false;
      }
      std::map<int, TRFISC,classcomp> vrfiscs;
      seat.GetRFISCs( vrfiscs );
      if ( vrfiscs.find( fltInfo.point_id ) != vrfiscs.end() ) {
        TRFISC rfisc = vrfiscs[ fltInfo.point_id ];
        LogTrace(TRACE5) << rfisc.str();
        TSeatTariffMapType::const_iterator irfisc;
        if ( !rfisc.empty() &&
              (irfisc=pass.tariffs.find( rfisc.color )) != pass.tariffs.end() ) {
          LogTrace(TRACE5) << "pr_prot_ckin=" <<irfisc->second.pr_prot_ckin;
          if ( irfisc->second.pr_prot_ckin ) {
            seat.SeatTariff.clear();
            tst();
            return true;
          }
        }
      }
    }
    return false;
  }
  tst();
  return true;
}


class AnomalisticConditionsPayment
{
  public:
    static void clearPreseatPaymentLayers( SALONS2::TSalons *Salons, const TTripInfo& fltInfo,
                                           BASIC_SALONS::TCompLayerTypes::Enum flag,
                                           bool check_prot_ckin_rfisc, TPassengers &passengers ) {
      //㤠�塞 �।���⥫쭮 �����祭��� ���⭮� ����
      for ( int i=0; i<passengers.getCount(); i++ ) {
        TPassenger &pass = passengers.Get( i );
        ProgTrace( TRACE5, "pass.preseatPlaces.size()=%zu, pax_id=%d", pass.preseatPlaces.size(), pass.paxId );
        if ( pass.preseatPlaces.empty() || pass.dont_check_payment ) {
          tst();
          continue;
        }
        vector<TPlace*> pls;
        bool prClear = false;
        ProgTrace( TRACE5, "get clear pass preseat, pax_id=%d, preseat_layer=%s", pass.preseat_pax_id, EncodeCompLayerType(pass.preseat_layer) );
        for ( std::vector<TCoordSeat>::iterator iseat=pass.preseatPlaces.begin(); iseat!=pass.preseatPlaces.end(); iseat++ ) {
          for ( std::vector<TPlaceList*>::iterator item=Salons->placelists.begin(); item!=Salons->placelists.end(); item++ ) {
            if ( iseat->placeListIdx == (*item)->num ) {
              TPlace *p = (*item)->place( iseat->p );
              if ( (pass.preseat_layer == cltProtBeforePay /* && !p->SeatTariff.empty() �᫨ ���� ��祣� �� �⮨� - ��ᨫ� �������*/) ||
                   pass.preseat_layer == cltPNLBeforePay ||
                   (pass.preseat_layer == cltProtSelfCkin && !p->SeatTariff.empty()) || //१��, �� �� ����祭
                   !UsedPayedPreseatForPassenger( fltInfo, flag, *p, pass, check_prot_ckin_rfisc ) ) { //���⪠ �।���⥫쭮 �����祭��� ����
                prClear = true; // �� ����⨫� - 㤠�塞 �� ���������� ࠧ���� ᫮�� � ���ᠦ�� ������ �����
                pls.push_back( p );
              }
            }
          }
        }
        if ( prClear ) {
          ProgTrace( TRACE5, "clear pass preseat, pax_id=%d, preseat_layer=%s", pass.preseat_pax_id, EncodeCompLayerType(pass.preseat_layer) );
          for ( vector<TPlace*>::iterator iseat=pls.begin(); iseat!=pls.end(); iseat++ ) {
            if ( !(*iseat)->layers.empty() &&
                 (*iseat)->layers.begin()->layer_type == pass.preseat_layer &&
                 (*iseat)->layers.begin()->pax_id == pass.preseat_pax_id ) {
              (*iseat)->layers.erase( (*iseat)->layers.begin() );
            }
          }
          pass.preseat_layer = cltUnknown;
          pass.preseat_no.clear();
          pass.preseat_pax_id = 0;
          pass.preseatPlaces.clear();
        }
      }
    }
    static void removeRemarksOnPaymentLayer( SALONS2::TSalons *Salons, TPassengers &passengers ) {
      for ( int i=0; i<passengers.getCount(); i++ ) {
        TPassenger &pass = passengers.Get( i );
      //  ProgTrace( TRACE5, "pass %s", pass.toString().c_str() );
        if ( pass.preseat_no.empty() ) {
          continue;
        }
        for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons->placelists.begin();
              plList!=Salons->placelists.end(); plList++ ) {
          TPlaceList* placeList = *plList;
          TPoint FP;
          if ( placeList->GetisPlaceXY( pass.preseat_no, FP ) ) {
            for ( int j=0; j<pass.countPlace; j++ ) {
              if ( !placeList->ValidPlace( FP ) )
                break;
              SALONS2::TPlace *place = placeList->place( FP );
              TLayerPrioritySeat layer = place->getCurrLayer( Salons->trip_id );
              if ( //place->isLayer( cltProtAfterPay, pass.paxId ) ) {
                   (layer.layerType() == cltProtAfterPay ||
                    layer.layerType() == cltProtSelfCkin ||
                    layer.layerType() == cltProtCkin ) && //!!!�������, �.�. ����, �� �᫨ ���� ࠧ���⪠ ����⮬, � �� ����� �� ������ � ����� ��⥬� ��ᠤ��� �� �� ����
                   layer.crs_pax_id() == pass.paxId ) {
                tst();
                for ( std::vector<TRem>::iterator irem=place->rems.begin(); irem!=place->rems.end(); ) {
                  if ( isREM_SUBCLS( irem->rem ) ) {
                    ++irem;
                    continue;
                  }
                  else {
                    ProgTrace( TRACE5, "removeRemarksOnPaymentLayer: remove rem=%s", irem->rem.c_str() );
                    irem = place->rems.erase(irem);
                  }
                }
              }
              switch( (int)pass.Step ) {
                case sRight:
                   FP.x++;
                   break;
                case sDown:
                   FP.y++;
                   break;
              }
            }
            break;
          }
        }
      }
    }
    static void setPayementOnWebSignal( SALONS2::TSalons *Salons, const TTripInfo& fltInfo,
                                        BASIC_SALONS::TCompLayerTypes::Enum flag,
                                        TPassengers &passengers ) {
      for ( int i=0; i<passengers.getCount(); i++ ) {
        TPassenger &pass = passengers.Get( i );
      //  ProgTrace( TRACE5, "pass %s", pass.toString().c_str() );
        if ( !pass.dont_check_payment || pass.preseat_no.empty() ) {
          continue;
        }
        for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons->placelists.begin();
              plList!=Salons->placelists.end(); plList++ ) {
          TPlaceList* placeList = *plList;
          TPoint FP;
          if ( placeList->GetisPlaceXY( pass.preseat_no, FP ) ) {
            for ( int j=0; j<pass.countPlace; j++ ) {
              if ( !placeList->ValidPlace( FP ) )
                break;
              SALONS2::TPlace *place = placeList->place( FP );
              if ( CurrSalon::get().canAddOccupy( place ) || //���� ����� ᫮� �� ��������騩 ��� ������ ���
                   (!place->layers.empty() && //� ���� ���� ᫮� �ਭ������騩 ��㣮�� ���ᠦ���, �� �� ������, � ����� � ��襣� ���ᠦ�� ��� �ࠢ� �� �� ����
                    place->layers.begin()->pax_id != ASTRA::NoExists &&
                    place->layers.begin()->pax_id != pass.paxId) ) {
                tst();
                break;
              }
              place->AddLayerToPlace( cltProtAfterPay, NowUTC(), pass.paxId, Salons->trip_id, pass.point_arv,
                                      BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( fltInfo.airline, cltProtAfterPay ),
                                                                                           flag ) );
              if ( CurrSalon::get().canAddOccupy( place ) ) {
                CurrSalon::get().AddOccupySeat( placeList->num, place->x, place->y );
              }
              pass.preseat_pax_id = pass.paxId;
              pass.preseat_layer = cltProtAfterPay;
              switch( (int)pass.Step ) {
                case sRight:
                   FP.x++;
                   break;
                case sDown:
                   FP.y++;
                   break;
              }
            }
            break;
          }
        }
      }
/*      for ( int i=0; i<passengers.getCount(); i++ ) {
        TPassenger &pass = passengers.Get( i );
        ProgTrace( TRACE5, "pass %s", pass.toString().c_str() );
      }*/
    }
    static void ClearTariffForSpecRemarks( SALONS2::TSalons *Salons, const TRemGrp& remGrp, TPassengers &passengers ) {
      std::vector<std::string> specRemarks, rems;
      for ( int i=0; i<passengers.getCount(); i++ ) {
         TPassenger &pass = passengers.Get( i );
         if ( !pass.ignore_tariff ) {
            continue;
         }
         pass.get_remarks( rems );
         for ( const auto &r : rems ) {
           if ( remGrp.exists( r ) &&
                find( specRemarks.begin(), specRemarks.end(), r ) == specRemarks.end() ) {
             specRemarks.emplace_back( r );
           }
         }
      }
      for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons->placelists.begin();
            plList!=Salons->placelists.end(); plList++ ) {
        TPlaceList* placeList = *plList;
        for ( IPlace i=placeList->places.begin(); i!=placeList->places.end(); i++ ) {
          if ( i->SeatTariff.empty() || !i->visible || !i->isplace )
            continue;
          for ( const auto & r : i->rems ) {
            if ( !r.pr_denial &&
                 find( specRemarks.begin(), specRemarks.end(), r.rem ) != specRemarks.end() ) {
              i->SeatTariff.clear();
              LogTrace(TRACE5)<<"remove tariff from " << i->yname << i->xname;
            }
          }
        }
      }
    }

/*    static void clearTariffsOnWebSignal( SALONS2::TSalons *Salons, TPassengers &passengers ) {
      //��頥� ��� ����
      for ( int i=0; i<passengers.getCount(); i++ ) {
        TPassenger &pass = passengers.Get( i );
        ProgTrace( TRACE5, "pass.preseat_no=%s", pass.preseat_no.c_str() );
        if ( !pass.dont_check_payment || pass.preseat_no.empty() ) {
          continue;
        }
        for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons->placelists.begin();
              plList!=Salons->placelists.end(); plList++ ) {
          TPlaceList* placeList = *plList;
          TPoint FP;
          if ( placeList->GetisPlaceXY( pass.preseat_no, FP ) ) {
            for ( int j=0; j<pass.countPlace; j++ ) {
              if ( !placeList->ValidPlace( FP ) )
                break;
              SALONS2::TPlace *place = placeList->place( FP );
              place->SeatTariff.clear();
              switch( (int)pass.Step ) {
                case sRight:
                   FP.x++;
                   break;
                case sDown:
                   FP.y++;
                   break;
              }
            }
            break;
          }
        }
      }
    }*/
};


/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( SALONS2::TSalons *Salons,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                      TClientType client_type,
                      bool pr_rollback,
                      bool separately_seats_adult_with_baby,
                      bool denial_emergency_seats,
                      TRFISCMode useRFISCMode,
                      TPassengers &passengers,
                      const std::vector<TCoordSeat> &paxsSeats,
                      const TRemGrp& remGrp )
{
  ProgTrace( TRACE5, "NEWSEATS, ASeatAlgoParams=%d, Salons->placelists.size()=%zu, passengers.getCount()=%d, paxsSeats.size()=%zu, separately_seats_adult_with_baby=%d, denial_emergency_seats=%d, wo_aisle=%d, issubgrp=%d, paxsSeats.size()=%zu",
            (int)ASeatAlgoParams.SeatAlgoType, Salons->placelists.size(), passengers.getCount(), paxsSeats.size(), separately_seats_adult_with_baby, denial_emergency_seats, passengers.wo_aisle, passengers.issubgrp, paxsSeats.size());
  if ( !passengers.getCount() ) {
    return;
  }

  SeatsStat.start("SeatsPassengers(TSalons)");
  //��� �ᥩ ��㯯� ���� ࠧ��⪠ ��䮬
  ProgTrace( TRACE5, "passengers.Get(0).tariffs=%s, tariffStatus=%d", passengers.Get(0).tariffs.key().c_str(), passengers.Get(0).tariffStatus );
  if ( passengers.Get(0).tariffStatus != TSeatTariffMap::stNotRFISC ) {
    Salons->SetTariffsByRFISCColor( Salons->trip_id, passengers.Get(0).tariffs, passengers.Get(0).tariffStatus );
  }
  TTripInfo fltInfo;
  fltInfo.getByPointId( Salons->trip_id );
  BASIC_SALONS::TCompLayerTypes::Enum flag = (GetTripSets( tsAirlineCompLayerPriority, fltInfo )?
                                                  BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                                  BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
  AnomalisticConditionsPayment::clearPreseatPaymentLayers( Salons, fltInfo, flag, useRFISCMode == rRFISC, passengers );
  //AnomalisticConditionsPayment::clearTariffProtCkinLayers( Salons, UseRFISCMode, passengers );
  //AnomalisticConditionsPayment::clearTariffsOnWebSignal( Salons, passengers );
  AnomalisticConditionsPayment::setPayementOnWebSignal( Salons, fltInfo, flag, passengers );
  AnomalisticConditionsPayment::removeRemarksOnPaymentLayer( Salons, passengers );
  AnomalisticConditionsPayment::ClearTariffForSpecRemarks( Salons, remGrp, passengers );

  GetUseLayers( UseLayers );
  TUseLayers preseat_layers, curr_preseat_layers;

  int param1=0,param2=0,param4=0,param5=0,param6=0,param7=0,param9=0,param10=0,param11=0;
  string param3,param8,param12;

  FSeatAlgoParams = ASeatAlgoParams;
  SeatPlaces.Clear();
  SeatPlaces.grp_status = passengers.Get( 0 ).grp_status;
  if ( separately_seats_adult_with_baby ) {
    SeatPlaces.separately_seats_adults_crs_pax_id = passengers.Get( 0 ).paxId;
    //ProgTrace( TRACE5, "SeatPlaces.separately_seats_adults_crs_pax_id=%d", SeatPlaces.separately_seats_adults_crs_pax_id );
  }
  CurrSalon::set( fltInfo.airline, flag, Salons );
  CanUseLayers = true;
  CanUseSmoke = false; /* ���� �� �㤥� ࠡ���� � ����騬� ���⠬� */
  preseat_layers[ cltProtCkin ] = CanUseLayer( cltProtCkin, UseLayers );
  preseat_layers[ cltProtBeforePay ] = CanUseLayer( cltProtBeforePay, UseLayers );
  preseat_layers[ cltProtAfterPay ] = CanUseLayer( cltProtAfterPay, UseLayers );
  preseat_layers[ cltPNLBeforePay ] = CanUseLayer( cltPNLBeforePay, UseLayers );
  preseat_layers[ cltPNLAfterPay ] = CanUseLayer( cltPNLAfterPay, UseLayers );
  preseat_layers[ cltProtSelfCkin ] = CanUseLayer( cltProtSelfCkin, UseLayers );

  Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
  Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
  std::vector<TCoordSeat> paxsSeatsEmpty;
  seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType,
                            Passengers.KWindow,
                            Passengers.KTube,
                            (separately_seats_adult_with_baby)?paxsSeatsEmpty:paxsSeats );
  SeatAlg = sSeatGrpOnBasePlace;

  FindSUBCLS = false;
  canUseSUBCLS = false;
  SUBCLS_REM = "";
  CanUseMutiLayer = false;
  bool prElemTypes = false;
  bool prINFT = false;
  bool isWorkINFT = AllowedAttrsSeat.isWorkINFT( Salons->trip_id );
  bool isDeniedSeatOnPNLAfterPay = GetTripSets( tsDeniedSeatOnPNLAfterPay, fltInfo );

  /* �� ᤥ����!!! �᫨ � ��� ���ᠦ�஢ ���� ����, � ⮣�� ��ᠤ�� �� ���⠬, ��� ��� ��㯯� */

  /* ��।������ ���� �� � ��㯯� ���ᠦ�� � �।���⥫쭮� ��ᠤ��� */
  bool pr_pay = false;
  bool Status_seat_no_BR=false, pr_all_pass_SUBCLS=true, pr_SUBCLS=false;
  for ( int i=0; i<passengers.getCount(); i++ ) {
    TPassenger &pass = passengers.Get( i );
    if ( SALONS2::isUserProtectLayer( pass.preseat_layer ) ) {
      preseat_layers[ pass.preseat_layer ] = true;
      if ( pass.preseat_layer == cltProtBeforePay ||
           pass.preseat_layer == cltPNLBeforePay ||
           pass.preseat_layer == cltProtAfterPay ||
           pass.preseat_layer == cltPNLAfterPay )
        pr_pay = true;
    }
    if ( pass.countPlace > 0 && pass.pers_type != "��"  ) {
      prElemTypes = true;
    }
    if ( pass.countPlace > 0 && pass.isRemark( "INFT" ) ) {
      prINFT = true;
    }
    if ( !pass.SUBCLS_REM.empty() && !Salons->isExistSubcls( pass.SUBCLS_REM ) ) {
      pass.remove_rem( pass.SUBCLS_REM, passengers.remarks, remGrp );
    }

    if ( !pass.SUBCLS_REM.empty() ) {
        pr_SUBCLS = true;
        if ( !SUBCLS_REM.empty() && SUBCLS_REM != pass.SUBCLS_REM )
          pr_all_pass_SUBCLS = false;
        SUBCLS_REM = pass.SUBCLS_REM;
    }
    else
        pr_all_pass_SUBCLS = false;
  }
  condRates.Init( *Salons, pr_pay, TReqInfo::Instance()->client_type ); // ᮡ�ࠥ� �� ⨯� ������ ���� � ���ᨢ �� �ਮ��⠬, �᫨ �� web-������, � �� ���뢠��

  ProgTrace( TRACE5, "pr_SUBCLS=%d,pr_all_pass_SUBCLS=%d,SUBCLS_REM=%s,pr_pay=%d,prINFT=%d",
             pr_SUBCLS, pr_all_pass_SUBCLS, SUBCLS_REM.c_str(), pr_pay, prINFT );

  bool VSeatOnlyBasePlace=true;
  for ( int i=0; i<passengers.getCount(); i++ ) {
    TPassenger &pass = passengers.Get( i );
    if ( pass.preseat_no.empty() ) {
        VSeatOnlyBasePlace=false;
        break;
    }
  }

  try {
   for ( int SeatOnlyBasePlace=VSeatOnlyBasePlace; SeatOnlyBasePlace>=0; SeatOnlyBasePlace-- ) {
   for ( int FCanUserSUBCLS=(int)pr_SUBCLS; FCanUserSUBCLS>=0; FCanUserSUBCLS-- ) {
     if ( pr_SUBCLS && FCanUserSUBCLS == 0 )
       ProgTrace( TRACE5, ">>>SeatsPassengers: error FCanUserSUBCLS=false, pr_SUBCLS=%d,pr_all_pass_SUBCLS=%d, SUBCLS_REM=%s", pr_SUBCLS, pr_all_pass_SUBCLS, SUBCLS_REM.c_str() );
     FindSUBCLS = FCanUserSUBCLS;
     canUseSUBCLS = FCanUserSUBCLS;
     // ??? �� ������ ��� ���ᠦ�஢ � ���쬨: ��࠭��� ��㯯� ��� ���� ࠧ���쭮, �� �� ࠧ�襭�� ���� ???
     // ���� ࠧ������
     for ( int FCanUseElem_Type=prElemTypes; FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // ���� �� ⨯�� ���� + �����஢���� ⨯� ����
       AllowedAttrsSeat.clearElems();
       if ( FCanUseElem_Type == 1 ) {
         AllowedAttrsSeat.getValidChildElem_Types();
       }
       for ( int FCanINFT=(!prINFT && AllowedAttrsSeat.pr_isWorkINFT); FCanINFT>=0; FCanINFT-- ) { //2 ��室� ��� ��㯯� ��� INFT
         if (!prINFT && FCanINFT == 0) {
           AllowedAttrsSeat.pr_isWorkINFT = false;
         }
         AllowedAttrsSeat.pr_INFT = prINFT;
         ProgTrace( TRACE5, "FCanINFT=%d,prINFT=%d,AllowedAttrsSeat.pr_isWorkINFT=%d",FCanINFT,prINFT,AllowedAttrsSeat.pr_isWorkINFT );
         for ( int FSeatAlg=0; FSeatAlg<seatAlgLength; FSeatAlg++ ) {
           SeatAlg = (TSeatAlg)FSeatAlg;
           switch(FSeatAlg) {
             case 0:
               ProgTrace(TRACE5, "start sSeatGrpOnBasePlace:SeatOnlyBasePlace=%d,FCanUserSUBCLS=%d,FCanINFT=%d,prINFT=%d,AllowedAttrsSeat.pr_isWorkINFT=%d",
                         SeatOnlyBasePlace,FCanUserSUBCLS,FCanINFT,prINFT,AllowedAttrsSeat.pr_isWorkINFT); //!!! � �⮬ �����⬥ �ମ���
               break;
             case 1:
               ProgTrace(TRACE5, "start sSeatGrp:SeatOnlyBasePlace=%d,FCanUserSUBCLS=%d,FCanINFT=%d,prINFT=%d,AllowedAttrsSeat.pr_isWorkINFT=%d",
                         SeatOnlyBasePlace,FCanUserSUBCLS,FCanINFT,prINFT,AllowedAttrsSeat.pr_isWorkINFT);
               break;
             case 2:
               ProgTrace(TRACE5, "start sSeatPassengers:SeatOnlyBasePlace=%d,FCanUserSUBCLS=%d,FCanINFT=%d,prINFT=%d,AllowedAttrsSeat.pr_isWorkINFT=%d",
                         SeatOnlyBasePlace,FCanUserSUBCLS,FCanINFT,prINFT,AllowedAttrsSeat.pr_isWorkINFT);
               break;
           }

           if ( SeatAlg == sSeatPassengers && prElemTypes && FCanUseElem_Type == 0 )
             continue;
           bool use_preseat_layer = ( CanUseLayer( cltProtCkin, preseat_layers ) ||
                                      CanUseLayer( cltProtBeforePay, preseat_layers ) ||
                                      CanUseLayer( cltPNLBeforePay, preseat_layers ) ||
                                      CanUseLayer( cltProtAfterPay, preseat_layers ) ||
                                      CanUseLayer( cltPNLAfterPay, preseat_layers ) ||
                                      CanUseLayer( cltProtSelfCkin, preseat_layers ));
           /* �᫨ ���� � ��㯯� �।���⥫쭠� ��ᠤ��, � ⮣�� ᠦ��� ��� �⤥�쭮 */
           /* �᫨ ���� � ��㯯� �������� � � �� �� � ��� ���ᠦ�஢, � ⮣�� ᠦ��� ��� �⤥�쭮 */
         //  ProgTrace( TRACE5, "use_preseat_layer=%d, SeatOnlyBasePlace=%d",use_preseat_layer,SeatOnlyBasePlace);
           if ( ( use_preseat_layer ||
                  Status_seat_no_BR ||
                  SeatOnlyBasePlace || // ��� ������� ���ᠦ�� ������ ᢮� ����� ����
                  ( canUseSUBCLS && pr_SUBCLS && !pr_all_pass_SUBCLS ) ) // �᫨ ���� ��㯯� ���ᠦ�஢ �।� ������ ���� ���ᠦ��� � �������ᮬ, �� �� ��
                &&
                SeatAlg != sSeatPassengers ) {
             continue;
           }
           for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
             CanUseRems = (TUseRem)FCanUseRems;
             switch( (int)SeatAlg ) {
               case sSeatGrpOnBasePlace:
                 switch( (int)CanUseRems ) {
                   case sIgnoreUse:
                   case sNotUse_NotUseDenial:
                   case sNotUseDenial:
                   case sNotUse:
                     continue;
                 }
                 break;
               case sSeatGrp:
                 switch( (int)CanUseRems ) {
                   case sAllUse:
                   case sMaxUse:
                   case sOnlyUse:
                   case sIgnoreUse:
                   case sNotUseDenial: //???
                     continue; /*??? �� ������� ��㯯� ��� ���� � ६�ઠ��, ��� �� ���� ���뢠�� */
                 }
                 break;
               case sSeatPassengers:
                 if ( use_preseat_layer && CanUseRems != sIgnoreUse ) // ������㥬 ६�ન � ��砥 �।���⥫쭮� ࠧ��⪨
                   continue;
                 break;
             }
             /* �ᯮ�짮����� ������ ���� */
             bool ignore_rate = condRates.ignore_rate;
             for ( condRates.current_rate = condRates.rates.begin(); condRates.current_rate != condRates.rates.end(); condRates.current_rate++ ) {
               //ProgTrace( TRACE5, "current_rate=condRates.rates.current_rate=%s", condRates.current_rate->str().c_str() );
               if ( condRates.current_rate->rate != 0.0   &&
                    !passengers.issubgrp &&
                    ( SeatAlg != sSeatPassengers || SeatOnlyBasePlace )
                      ) { //��ᠤ�� �� ����� ���� ⮫쪮 �� ������ SeatAlg=1 � ���� �����஢��� ��ᠤ�� �� ������ ����, �᫨ ��� ���� �����
                 //ProgTrace( TRACE5, "condRates.current_rate=%f continue", condRates.current_rate->rate );
                 continue;
               }
               if ( use_preseat_layer && SeatAlg == sSeatPassengers ) {
                 condRates.ignore_rate = true;
               }
               else {
                 condRates.ignore_rate = ignore_rate;
               }
//               ProgTrace( TRACE5, "condRates.current_rate=%s, condRates.ignore_rate=%d, SeatAlg=%d", condRates.current_rate->str().c_str(), condRates.ignore_rate, SeatAlg );
               //ProgTrace( TRACE5, "SeatAlg == %d, condRates.current_rate.first=%d, condRates.current_rate->second.value=%f",
               //                        SeatAlg, condRates.current_rate->first, condRates.current_rate->second.value );
               /* �ᯮ�짮����� ����ᮢ ���� */
               //���� ���뢠�� ��砩, ����� ��⠫��� ���� ⮫쪮 � ᫮ﬨ cltProtBeforePay, cltPNLBeforePay, cltProtAfterPay, cltPNLAfterPay - ���� 㬥��
               // ���砫� ���� ���஡����� �ᯮ�짮���� ⮫쪮 ���� � ᫮ﬨ cltProtBeforePay, cltPNLBeforePay, � ��⮬ + cltProtAfterPay, cltPNLAfterPay
               for ( int KeyLayers=1; KeyLayers>=-3; KeyLayers-- ) {
                 if ( ( !KeyLayers && ( SeatAlg == sSeatGrpOnBasePlace || SeatAlg == sSeatGrp ) ) ||
                      ( KeyLayers < -1 && ( SeatAlg != sSeatPassengers || !condRates.isIgnoreRates( ) ) ) ) { // �᫨ ����� �ᯮ�짮���� ����� ᫮�, � ⮫쪮 �� ��ᠤ�� ���ᠦ�஢ �� ������
                   continue;
                 }
                 curr_preseat_layers.clear();
                 curr_preseat_layers.insert( preseat_layers.begin(), preseat_layers.end() );
//                 ProgTrace(TRACE5, "SeatOnlyBasePlace=%d, KeyLayers=%d", SeatOnlyBasePlace, KeyLayers );
                 if ( SeatOnlyBasePlace && KeyLayers <= -2 ) { //�᫨ 㪠���� ����, ���஥ ����⨫ ��㣮�, � ����� ��� �������
                   continue;
                 }
                 if ( KeyLayers <= -2 &&
                      ( CanUseRems != sNotUseDenial && CanUseRems != sIgnoreUse ) ) {
                   continue;
                 }
                 if ( KeyLayers == -2 ) {
                   if ( CanUseLayer( cltProtBeforePay, curr_preseat_layers ) &&
                        CanUseLayer( cltPNLBeforePay, curr_preseat_layers ) &&
                        CanUseLayer( cltProtSelfCkin, curr_preseat_layers )) {
                     continue;
                   }
                   curr_preseat_layers[ cltProtBeforePay ] = true;
                   curr_preseat_layers[ cltPNLBeforePay ] = true;
                   curr_preseat_layers[ cltProtSelfCkin ] = true;
                 }
                 if ( KeyLayers == -3 ) {
                   if ( CanUseLayer( cltProtBeforePay, curr_preseat_layers ) &&
                        CanUseLayer( cltPNLBeforePay, curr_preseat_layers ) &&
                        CanUseLayer( cltProtSelfCkin, curr_preseat_layers )) {
                     continue;
                   }
                   curr_preseat_layers[ cltProtBeforePay ] = true;
                   curr_preseat_layers[ cltPNLBeforePay ] = true;
                   curr_preseat_layers[ cltProtAfterPay ] = true;
                   curr_preseat_layers[ cltPNLAfterPay ] = !isDeniedSeatOnPNLAfterPay;
                   curr_preseat_layers[ cltProtSelfCkin ] = true;
                 }
                 /* ������ ���ᨢ ����ᮢ ���� */
                 SetLayers( Salons,
                            SeatsLayers,
                            CanUseMutiLayer,
                            passengers.Get( 0 ).preseat_layer,
                            KeyLayers,
                            curr_preseat_layers,
                            client_type );
                 /* �஡�� �� ����ᮬ */
                 for ( vector<TCompLayerType>::iterator l=SeatsLayers.begin(); l!=SeatsLayers.end(); l++ ) {
                   PlaceLayer = *l;
                   /* ��⠢���� ������ ��᪮�쪮 ࠧ �� �鸞� �� ���ᠤ�� ��㯯� */
                   for ( int FCanUseAlone=uFalse3; FCanUseAlone<=uTrue; FCanUseAlone++ ) {
                     CanUseAlone = (TUseAlone)FCanUseAlone;
                     if ( CanUseAlone == uFalse3 && passengers.getCount() < 2 ) { //�⮡� �� ������ ��譨� 横���, �.�. ��ᠤ�� �� ࠢ�� �� �ன���
                       //���� ���ᠦ�� ������ ᨤ��� ���� � ���
                       continue;
                     }
                     if ( CanUseAlone == uTrue && CanUseRems == sNotUse_NotUseDenial	 ) {
                       // �᫨ ���ᠦ�஢ ����� ��⠢���� ᪮�쪮 㣮��� ࠧ �� ������ � ��� � ����� ᠦ��� ⮫쪮 �� ���� � �㦭묨 ६�ઠ��
                       continue;
                     }
                     if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers ) {
                       continue;
                     }
                     if ( !KeyLayers && CanUseAlone == uFalse3 ) {
                       continue;
                     }
                     /* ��� ०��� ��ᠤ�� � ����� ��� */
                     for ( int FCanUseOneRow=getCanUseOneRow(); FCanUseOneRow>=0; FCanUseOneRow-- ) {
                       canUseOneRow = FCanUseOneRow;
                       /* ��� ��室�� */
                       for ( int FCanUseTube=0; FCanUseTube<=(passengers.wo_aisle?0:1); FCanUseTube++ ) {
                         ProgTrace(TRACE5, "wo_aisle=%d", passengers.wo_aisle );
                         /* ��� ��ᠤ�� �⤥���� ���ᠦ�஢ �� ���� ���뢠�� ��室� */
                         if ( !FCanUseTube &&
                              !paxsSeats.empty() &&
                              !separately_seats_adult_with_baby &&
                              !passengers.wo_aisle &&
                              !passengers.issubgrp) {
                           //ProgTrace( TRACE5, "separately_seats_adult_with_baby");
                           continue;
                         }
/*                         tst();
                         if ( FCanUseTube && SeatAlg == sSeatPassengers ) {
                           continue;
                         }*/
                         if ( FCanUseTube && CanUseAlone == uFalse3 && !canUseOneRow ) {
                           continue;
                         }
                         if ( canUseOneRow && !FCanUseTube ) {
                           continue;
                         }
                         CanUseTube = FCanUseTube;
                         for ( int FCanUseSmoke=passengers.UseSmoke; FCanUseSmoke>=0; FCanUseSmoke-- ) {
                           CanUseSmoke = FCanUseSmoke;
                           param1 = (int)SeatAlg;
                           param2 = FCanUseElem_Type;
                           param3 = DecodeCanUseRems( CanUseRems );
                           param4 = FCanUseAlone;
                           param5 = KeyLayers;
                           param6 = FCanUseTube;
                           param7 = FCanUseSmoke;
                           param8 = EncodeCompLayerType(PlaceLayer);
                           param9 = MAXPLACE();
                           param10 = canUseOneRow;
                           param11 = canUseSUBCLS;
                           param12 = SUBCLS_REM;
                           switch( (int)SeatAlg ) {
                             case sSeatGrpOnBasePlace:
                               if ( SeatPlaces.SeatGrpOnBasePlace( ) ) {
                                 throw 1;
                               }
                               break;
                             case sSeatGrp:
                               getRemarks( passengers.Get( 0 ) ); // ��� ᠬ��� �ਮ��⭮��
                               if ( SeatPlaces.SeatsGrp( ) )
                                 throw 1;
                               break;
                             case sSeatPassengers:
                               if ( SeatPlaces.SeatsPassengers() )
                                 throw 1;
                               if ( SeatOnlyBasePlace && pr_SUBCLS && !canUseSUBCLS ) { //���諨 �� ��᫥���� ������, ����� ���� ��������� � 㪠���� ����, �� ॠ�쭮 ࠧ��⪨ �������ᮢ ��� � ᠫ���
                                 SeatOnlyBasePlace = false;
                                 FSeatAlg=0;
                               }
                               break;
                           } /* end switch SeatAlg */
                           //ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseElem_Type=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyLayers=%d,FCanUseTube=%d,FCanUseSmoke=%d,PlaceLayer=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseSUBCLS=%d, SUBCLS_REM=%s",
//                                      param1,param2,param3.c_str(),param4,param5,param6,param7,param8.c_str(),param9,param10,param11,param12.c_str());

                         } /* end for FCanUseSmoke */
                       } /* end for FCanUseTube */
                     } /* end for FCanUseOneRow */
                   } /* end for FCanUseAlone */
                 } /* end for use SeatsLayers */
               } /* end for KeyLayer */

             } /* end for condRates */
           } /* end for CanUseRem */
           ProgTrace(TRACE5, "end" );
         } /* end for FSeatAlg */
       } /* end for FCanUseRemarks */
     } /*  end for FCanUseElem_Type */
   } /*  end for FCanUserSUBCLS */
   }  /* end for SeatOnlyBasePlace */
    SeatAlg = (TSeatAlg)0;
  }
  catch( int ierror ) {
    AllowedAttrsSeat.pr_isWorkINFT = isWorkINFT;
    if ( ierror != 1 ) {
      SeatsStat.stop("SeatsPassengers(TSalons)");
      throw;
    }
    ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseElem_Type=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyLayers=%d,FCanUseTube=%d,FCanUseSmoke=%d,PlaceLayer=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseSUBCLS=%d, SUBCLS_REM=%s",
               param1,param2,param3.c_str(),param4,param5,param6,param7,param8.c_str(),param9,param10,param11,param12.c_str());

//    ProgTrace( TRACE5, "SeatAlg=%d, CanUseRems=%d", (int)SeatAlg, (int)CanUseRems );
    /* ��।������ ����祭��� ���� �� ���ᠦ�ࠬ, ⮫쪮 ��� SeatPlaces.SeatGrpOnBasePlace */
    if ( SeatAlg != sSeatPassengers ) {
      SeatPlaces.PlacesToPassengers( );
    }

    Passengers.sortByIndex();
    if ( pr_rollback ) {
      SeatPlaces.RollBack( );
    }
    SeatsStat.stop("SeatsPassengers(TSalons)");
    return;
  }
  AllowedAttrsSeat.pr_isWorkINFT = isWorkINFT;
  SeatPlaces.RollBack( );
  SeatsStat.stop("SeatsPassengers(TSalons)");
  if ( separately_seats_adult_with_baby ) {
    throw UserException( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS.BABY_ZONES" );
  }
  throw UserException( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS" );
}

void SaveTripSeatRanges( int point_id, TCompLayerType layer_type, TSeatRanges &seats,
                         int pax_id, int point_dep, int point_arv, TDateTime time_create )
{
  if (seats.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,point_dep,point_arv,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,pax_id,time_create) "
    "  VALUES "
    "    (:range_id,:point_id,:point_dep,:point_arv,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:pax_id,:time_create); "
    "  IF :pax_id IS NOT NULL THEN "
    "    UPDATE pax SET wl_type=NULL WHERE pax_id=:pax_id; "
    "  END IF; "
    "END; ";
  Qry.CreateVariable( "range_id", otInteger, FNull );
  Qry.CreateVariable( "point_id", otInteger,point_id );
  Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
  if ( pax_id > 0 )
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
  else
    Qry.CreateVariable( "pax_id", otInteger, FNull );
  if ( point_dep > 0 )
    Qry.CreateVariable( "point_dep", otInteger, point_dep );
  else
    Qry.CreateVariable( "point_dep", otInteger, FNull );
  if ( point_arv > 0 )
    Qry.CreateVariable( "point_arv", otInteger, point_arv );
  else
    Qry.CreateVariable( "point_arv", otInteger, FNull );
  Qry.DeclareVariable( "first_xname", otString );
  Qry.DeclareVariable( "last_xname", otString );
  Qry.DeclareVariable( "first_yname", otString );
  Qry.DeclareVariable( "last_yname", otString );
  Qry.CreateVariable( "time_create", otDate, time_create );

  for(TSeatRanges::iterator i=seats.begin();i!=seats.end();i++)
  {
    Qry.SetVariable("first_xname",i->first.line);
    Qry.SetVariable("last_xname",i->second.line);
    Qry.SetVariable("first_yname",i->first.row);
    Qry.SetVariable("last_yname",i->second.row);
    Qry.Execute();
  };
}

bool isINFT( int point_id, int pax_id ) {
  TQuery PaxQry( &OraSession );
  PaxQry.SQLText =
    "SELECT grp_id, 0 inf_id, 0 priority FROM pax WHERE pax_id=:pax_id "
    "UNION "
    "SELECT crs_pnr.pnr_id, crs_pax.inf_id, 1 priority FROM crs_pax, crs_pnr "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "       crs_pax.pax_id=:pax_id AND "
    "       crs_pax.pr_del=0 AND "
    "       crs_pnr.system='CRS' "
    "ORDER BY priority";
  PaxQry.CreateVariable( "pax_id", otInteger, pax_id );
  PaxQry.Execute();
  if ( PaxQry.Eof ) {
    throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"	);
  }
  if ( PaxQry.FieldAsInteger( "priority") != 0 ) {
    return !PaxQry.FieldIsNULL( "inf_id");
  }
  int grp_id = PaxQry.FieldAsInteger( "grp_id");
  PaxQry.Clear();
  PaxQry.SQLText =
    " SELECT pax.pax_id, pax.pers_type, pax.seats, "
    "        reg_no, pax.surname, crs_inf.pax_id AS parent_pax_id "
    "    FROM pax_grp, pax, crs_inf "
    "   WHERE pax.grp_id=pax_grp.grp_id AND "
    "         pax_grp.point_dep=:point_dep AND "
    "         pax.pax_id=crs_inf.inf_id(+) AND "
    "         pax_grp.status NOT IN ('E') AND "
    "         pax.refuse IS NULL AND "
    "         pax.grp_id=:grp_id ";
  PaxQry.CreateVariable( "point_dep", otInteger, point_id );
  PaxQry.CreateVariable( "grp_id", otInteger, grp_id );
  PaxQry.Execute();
  vector<TPass> InfItems, AdultItems;
  for ( ;!PaxQry.Eof; PaxQry.Next() ) {
    TPass pass;
    pass.grp_id = grp_id;
    pass.pax_id = PaxQry.FieldAsInteger( "pax_id" );
    pass.reg_no = PaxQry.FieldAsInteger( "reg_no" );
    pass.surname = PaxQry.FieldAsString( "surname" );
    pass.parent_pax_id = PaxQry.FieldAsInteger( "parent_pax_id" );
    if ( PaxQry.FieldAsInteger( "seats" ) == 0 ) {
      InfItems.push_back( pass );
    }
    else {
      if ( string(PaxQry.FieldAsString( "pers_type" )) == string("��") ) {
        AdultItems.push_back( pass );
      }
    }
  }
  SetInfantsToAdults<TPass,TPass>( InfItems, AdultItems );
  for ( vector<TPass>::iterator i=InfItems.begin(); i!=InfItems.end(); i++ ) {
    for ( vector<TPass>::iterator j=AdultItems.begin(); j!=AdultItems.end(); j++ ) {
      if ( i->parent_pax_id == j->pax_id &&
           j->pax_id == pax_id ) {
        return true;
      }
    }
  }
  return false;
}

bool getCurrSeat( const TSalonList &salonList,
                  TSeatRange &r, SALONS2::TPoint &coord,
                  CraftSeats::const_iterator &isalonList )
{
    isalonList = salonList._seats.end();
    for( isalonList = salonList._seats.begin(); isalonList != salonList._seats.end(); isalonList++ ) {
        if ( (*isalonList)->GetisPlaceXY( string(r.first.row)+r.first.line, coord ) ) {
            return true;
        }
    }
    return false;
}

void SyncPRSA( const string &airline_oper,
               int pax_id,
               TSeatTariffMap::TStatus tariffs_status,
               const vector<pair<TSeatRange,TRFISC> > &tariffs )
{
  if (tariffs.empty())
  {
    ProgError(STDLOG, "%s: tariffs.empty()", __FUNCTION__);
    return;
  }
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (TReqInfo::Instance()->client_type!=ctTerm) return;

  TRemGrp service_stat_rem_grp;
  service_stat_rem_grp.Load(retREM_STAT, airline_oper);

  multiset<CheckIn::TPaxRemItem> prior_rems;
  CheckIn::LoadPaxRem(pax_id, prior_rems);

  TQuery RemQry(&OraSession);
  RemQry.Clear();
  RemQry.SQLText="DELETE FROM pax_rem WHERE pax_id=:pax_id AND rem_code=:rem_code";
  RemQry.CreateVariable("pax_id",otInteger,pax_id);
  RemQry.CreateVariable("rem_code",otString,"PRSA");
  RemQry.Execute();

  RemQry.SQLText=
    "INSERT INTO pax_rem(pax_id,rem,rem_code) VALUES(:pax_id,:rem,:rem_code)";
  RemQry.DeclareVariable("rem",otString);

  for ( vector<pair<TSeatRange,TRFISC> >::const_iterator it=tariffs.begin(); it!=tariffs.end(); ++it )
  {
    if (tariffs_status==TSeatTariffMap::stNotFound ||
        tariffs_status==TSeatTariffMap::stNotRFISC) continue;
    if (/*(tariffs_status==TSeatTariffMap::stUseRFISC && it->second.rate==0.0) ||*/ it->second.code.empty()) continue;
    ostringstream rem;
    rem << "PRSA/" << it->second.code;
    if (tariffs_status==TSeatTariffMap::stUseRFISC)
      rem << "/" << it->second.rateView()
          << "/" << it->second.currencyView(LANG_EN);

    RemQry.SetVariable("rem", rem.str());
    RemQry.Execute();
  }

  multiset<CheckIn::TPaxRemItem> curr_rems;
  CheckIn::LoadPaxRem(pax_id, curr_rems);

  CheckIn::SyncPaxRemOrigin(service_stat_rem_grp,
                            pax_id,
                            prior_rems,
                            curr_rems,
                            reqInfo->user.user_id,
                            reqInfo->desk.code);
}

bool setRFISCQuestion(xmlNodePtr reqNode, xmlNodePtr resNode,
                      int point_id,TSeatsType seat_type,
                      const BitSet<TChangeLayerProcFlag> &procFlags,
                       TCompLayerType layer_type )
{
  bool reset=false;
  std::set<TCompLayerType> checkinLayers { cltGoShow, cltTranzit, cltCheckin, cltTCheckin };
  if ( TReqInfo::Instance()->client_type == ctTerm &&
       seat_type == SEATS2::stReseat && //���ᠤ��
        !procFlags.isFlag( procWaitList ) && //  �� �㣠����, �᫨ ���ᠤ�� ���� � ��
        reqNode != nullptr &&
        resNode != nullptr &&
        checkinLayers.find( layer_type ) != checkinLayers.end() ) { // 㦥 ��ॣ����஢������
      TTripInfo fltInfo;
      if (!fltInfo.getByPointId(point_id))
        throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
      if ( GetTripSets(tsReseatOnRFISC,fltInfo) ) {
        if (TReqInfo::Instance()->desk.compatible(RESEAT_QUESTION_VERSION) ) {
          tst();
          xmlNodePtr dataNode = GetNode( "data", resNode );
          if (GetNode("confirmations/msg1",reqNode)==NULL) {
            tst();
            if ( dataNode == nullptr ) {
              dataNode =  NewTextChild(resNode,"data");
            }
            xmlNodePtr confirmNode=NewTextChild(dataNode,"confirmation");
            NewTextChild(confirmNode,"reset",(int)reset);
            NewTextChild(confirmNode,"type","msg1");
            NewTextChild(confirmNode,"dialogMode","");
            ostringstream msg;
            msg << getLocaleText("MSG.PASSENGER.RESEAT_TO_RFISC") << endl
                << getLocaleText("QST.CONTINUE_RESEAT");
            NewTextChild(confirmNode,"message",msg.str());
            tst();
            return true;
          }
          else
           if ( NodeAsInteger("confirmations/msg1",reqNode) == 7 ) {
             throw UserException("MSG.PASSENGER.RESEAT_BREAK");
          }
        }
      }
    }
  return false;
}

BitSet<TChangeLayerSeatsProps>
     ChangeLayer( const TSalonList &salonList, TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type,
                  const BitSet<TChangeLayerProcFlag> &procFlags,
                  const std::string& whence,
                  xmlNodePtr reqNode, xmlNodePtr resNode )
{
  BitSet<TChangeLayerSeatsProps> propsSeatsFlags;
  propsSeatsFlags.clearFlags();
  propsSeatsFlags.setFlag(changedOrNotPay);
  if ( procFlags.isFlag( procPaySeatSet ) &&
       ( seat_type != stSeat || ( layer_type != cltProtBeforePay && layer_type != cltProtAfterPay && layer_type != cltProtSelfCkin ) ) ) {
    tst();
    throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
  }
  UseLayers[ cltProtCkin ] = false;
  UseLayers[ cltProtBeforePay ] = false;
  UseLayers[ cltPNLBeforePay ] = false;
  UseLayers[ cltProtAfterPay ] = false;
  UseLayers[ cltPNLAfterPay ] = false;
  UseLayers[ cltProtSelfCkin ] = false;
    //CanUse_PS = false; //!!!
  first_xname = SeatNumber::tryNormalizeLine( first_xname );
  first_yname = SeatNumber::tryNormalizeRow( first_yname );
  LogTrace( TRACE5 ) << ((salonList.getRFISCMode() == rTariff)?string(""):string("RFISC Mode ")) << "layer=" << EncodeCompLayerType( layer_type )
              << ",point_id=" << point_id << ",pax_id=" << pax_id << ",first_xname=" << first_xname << ",first_yname=" << first_yname;
  TQuery Qry( &OraSession );
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

  int step = 0;
  TAdvTripInfo operFlt;
  operFlt.getByPointId( point_id );
  BASIC_SALONS::TCompLayerTypes::Enum flag = (GetTripSets( tsAirlineCompLayerPriority, operFlt )?
                                                  BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                                  BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
  /* ���뢠�� ���� �� ���ᠦ��� */
  switch ( layer_type ) {
    case cltGoShow:
    case cltTranzit:
    case cltCheckin:
    case cltTCheckin:
      {
        multiset<CheckIn::TPaxRemItem> rems;
        CheckIn::LoadPaxRem(false /*crs*/, pax_id, rems, false /*onlyPD*/, "STCR");
        step = rems.size();
      }
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, pax.is_jmp, pax.tid, '' airp_arv, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,:point_dep,'list',rownum) AS seat_no, "
       "       NVL(pax.cabin_class, pax_grp.class) AS class, pers_type "
       " FROM pax, pax_grp "
       "WHERE pax.pax_id=:pax_id AND "
       "      pax_grp.grp_id=pax.grp_id ";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltProtCkin:
    case cltProtBeforePay: //WEB ChangeProtPaidLayer
    case cltProtAfterPay:
    case cltPNLAfterPay:
    case cltProtSelfCkin:
      {
        multiset<CheckIn::TPaxRemItem> rems;
        CheckIn::LoadCrsPaxRem(pax_id, rems, false /*onlyPD*/, "STCR");
        step = rems.size();
      }
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, 0 grp_id, seats, 0 is_jmp, crs_pax.tid, airp_arv, point_id, 0 point_arv, "
        "       NULL AS seat_no, pers_type, " +
        CheckIn::TSimplePaxItem::cabinClassFromCrsSQL() + " AS class "
        " FROM crs_pax, crs_pnr "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "       crs_pax.pnr_id=crs_pnr.pnr_id";
        if ( layer_type == cltProtCkin || layer_type == cltProtSelfCkin ||
             ( !procFlags.isFlag( procPaySeatSet ) && ( layer_type == cltPNLAfterPay || layer_type == cltProtAfterPay )) ||
             procFlags.isFlag( procPaySeatSet ) ) {
          break;
        }
        //[[fallthrough]]; //�᪮��������, �᫨ ��������� ��� break �����⨬�, ���� ��ࠢ��� �訡��
    default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  // ���ᠦ�� �� ������ ��� ����������������� � ��㣮� �⮩�� ��� �� �।�. ��ᠤ�� ���ᠦ�� 㦥 ��ॣ����஢��
  if ( !Qry.RowCount() ) {
    ProgTrace( TRACE5, "!!! Passenger not found in funct ChangeLayer" );
    throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"	);
  }

  string strclass = Qry.FieldAsString( "class" );
  ProgTrace( TRACE5, "subclass=%s", strclass.c_str() );
  string fullname = Qry.FieldAsString( "surname" );
  TrimString( fullname );
  fullname += string(" ") + Qry.FieldAsString( "name" );
  int idx1 = Qry.FieldAsInteger( "reg_no" );
  int idx2 = Qry.FieldAsInteger( "grp_id" );
  string airp_arv = Qry.FieldAsString( "airp_arv" );
  int point_id_tlg = Qry.FieldAsInteger( "point_id" );
  int point_arv = Qry.FieldAsInteger( "point_arv" );
  int seats_count = Qry.FieldAsInteger( "seats" );
  bool is_jmp = Qry.FieldAsInteger( "is_jmp" )!=0;
  int pr_down;
  if ( step )
    pr_down = 1;
  else
    pr_down = 0;
  //string prior_seat = Qry.FieldAsString( "seat_no" );
  string pers_type = Qry.FieldAsString( "pers_type" );
  if ( !seats_count ) {
    ProgTrace( TRACE5, "!!! Passenger has count seats=0 in funct ChangeLayer" );
    throw UserException( "MSG.SEATS.NOT_RESEATS_SEATS_ZERO" );
  }
  if (is_jmp) throw UserException( "MSG.SEATS.NOT_RESEATS_PASSENGER_FROM_JUMP_SEAT" );

  if ( Qry.FieldAsInteger( "tid" ) != tid  ) {
    ProgTrace( TRACE5, "!!! Passenger has changed in other term in funct ChangeLayer" );
    throw UserException( "MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                         LParams()<<LParam("surname", fullname ) );
  }
  bool prCheckin = SALONS2::Checkin( pax_id );
  if ( ( layer_type != cltGoShow &&
         layer_type != cltCheckin &&
         layer_type != cltTCheckin &&
         layer_type != cltTranzit ) && prCheckin ) { //???!!!��।�����
    ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
    throw UserException( "MSG.PASSENGER.CHECKED.REFRESH_DATA" );
  }

  // �஢�ઠ �� �, �� ���ᠦ�� ����� 㦥 ����� �ਮ���� ᫮�, � �⨬ �������� ����� �ਮ����
  //�᫨ ���ᠦ�� ����� ����� ᫮�, � ����� ࠡ���� � �।���⥫쭮� ࠧ��⪮� ???
  TLayerPrioritySeat layerPrioritySeat = TLayerPrioritySeat::emptyLayer();
  std::set<TPlace*,CompareSeats> seats, nseats;
  salonList.getPaxLayer( point_id, pax_id,
                         layerPrioritySeat, seats );
  if ( layerPrioritySeat.layerType() != cltUnknown ) {
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( salonList.getAirline(), layerPrioritySeat.layerType() ),
                                                              flag ) <
         BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( salonList.getAirline(), layer_type ),
                                                              flag ) ) {
      throw UserException( "MSG.SEATS.SEAT_NO.EXIST_MORE_PRIORITY" ); //���ᠦ�� ����� ����� �ਮ��⭮� ����
    }
  }
  TSeatRanges seatRanges;
  vector<pair<TSeatRange,TRFISC> > logSeats;
  TSeatRange r;
  vector<TPlaceList*>::const_iterator isalonList;
  SALONS2::TPoint coord;
  TPlace* seat;
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,airp,point_id,point_num,first_point,pr_tranzit "
    " FROM points WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );

  TSeatTariffMap passTariffs;
  if ( seat_type != stDropseat ) { // ���������� ����� ���� + �஢�ઠ
    if ( prCheckin ) {
      ProgTrace( TRACE5, "pax_id=%d", pax_id );
      passTariffs.get( pax_id );
    }
    else {
      ProgTrace( TRACE5, "pax_id=%d", pax_id );
      TMktFlight mktFlight;
      mktFlight.getByCrsPaxId( pax_id );
      CheckIn::TPaxTknItem tkn;
      CheckIn::LoadCrsPaxTkn( pax_id, tkn);
      if ( airp_arv.empty() ) {
        LogError(STDLOG) << "crs_pax_id=" << pax_id << " airp_arv is empty!!!";
      }
      passTariffs.get( operFlt, mktFlight, tkn, airp_arv );
    }
    TQuery QrySeatRules( &OraSession );
    QrySeatRules.SQLText =
        "SELECT pr_owner FROM comp_layer_rules "
        "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
    QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( layer_type ) );
    QrySeatRules.DeclareVariable( "old_layer", otString );
  // ���뢠�� ᫮� �� ������ ����� � ������ �஢��� �� �, �� ��� ᫮� 㦥 ����� ��㣨� ���ᠦ�஬
    seatRanges.clear();
    logSeats.clear();
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    if ( !getCurrSeat( salonList, r, coord, isalonList ) ) {
      tst();
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    TBabyZones  babyZones( point_id );
    TEmergencySeats emergencySeats( point_id );
    TPaxsCover grpPaxs;
    grpPaxs.push_back( TPaxCover( pax_id, ASTRA::NoExists ) );
    std::set<int> pax_with_baby_lists;
    std::map<int,TPaxList>::const_iterator ipaxs = salonList.pax_lists.find( point_id );
    if ( emergencySeats.deniedEmergencySection() ||
         babyZones.useInfantSection() ) {
      if ( ipaxs != salonList.pax_lists.end() ) {
        ipaxs->second.setPaxWithInfant( pax_with_baby_lists );
      }
    }
    //calc CHIN REM
    if ( emergencySeats.deniedEmergencySection() ) {
      bool flagCHIN = ( DecodePerson( pers_type.c_str() ) != ASTRA::adult || pax_with_baby_lists.find( pax_id ) != pax_with_baby_lists.end() );
      if ( !flagCHIN ) {
        multiset<CheckIn::TPaxRemItem> rems;
        if ( prCheckin ) {
          CheckIn::LoadPaxRem(pax_id, rems);
        }
        else {
          CheckIn::LoadCrsPaxRem(pax_id, rems);
        }
        for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
        {
          if (r->code=="BLND" ||
              r->code=="STCR" ||
              r->code=="UMNR" ||
              r->code=="WCHS" ||
              r->code=="MEDA") {
           flagCHIN=true;
           break;
         }
        }
      }
      if ( flagCHIN ) {
        emergencySeats.setDisabledEmergencySeats( nullptr, *isalonList, grpPaxs, TEmergencySeats::DisableMode::dlrss );
      }
    }
    if ( babyZones.useInfantSection() ) {
      if ( pax_with_baby_lists.find( pax_id ) != pax_with_baby_lists.end() ) {
        babyZones.setDisabledBabySection( point_id, nullptr, *isalonList, grpPaxs, pax_with_baby_lists, TBabyZones::DisableMode::dlrss );
      }
    }
    std::vector<SALONS2::TPlace> verifyPlaces;
    for ( int i=0; i<seats_count; i++ ) { // �஡�� �� ���-�� ���� � �� ���⠬
      seat = (*isalonList)->place( coord );
      if ( !seat->visible || !seat->isplace || seat->clname != strclass ) {
        tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }
      //�஢�ઠ �� �, �� ���� ���� �ਭ������� ���ᠦ��� � ॡ����� � � ��� ���ᠦ�� � ॡ�����
      if ( seat->isLayer( cltDisable ) ) {
        if ( emergencySeats.in( (*isalonList)->num, seat ) ) {
          tst();
          throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
        }
        else {
          tst();
          throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL_WITH_INFT" );
        }
        babyZones.rollbackDisabledBabySection();
        emergencySeats.rollbackDisabledEmergencySeats();
      }
      //�����稬 ���� ��� ���ᠦ��
      //TPropsPoints points( salonList.filterSets.filterRoutes, salonList.filterRoutes.point_dep, salonList.filterRoutes.point_arv );
      //bool pr_departure_tariff_only = true;
      TRFISC rfisc;
      ProgTrace( TRACE5, "RFISCMode=%d", salonList.getRFISCMode() );
      if ( salonList.getRFISCMode() ) {
        std::map<int, TRFISC,classcomp> vrfiscs;
        seat->GetRFISCs( vrfiscs );
        if ( vrfiscs.find( point_id ) != vrfiscs.end() ) {
          rfisc = vrfiscs[ point_id ];
        }
        if ( !rfisc.empty() &&
             passTariffs.find( rfisc.color ) != passTariffs.end() ) {
          if ( layer_type == cltProtCkin ) {
            LogTrace(TRACE5) << passTariffs[ rfisc.color ].str() << ",pr_prot_ckin=" << passTariffs[ rfisc.color ].pr_prot_ckin;
            if (!passTariffs[ rfisc.color ].pr_prot_ckin ) {
              throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL_WITH_RFISC",
                                  LParams()<<LParam("code", rfisc.code) );
            }
          }
          if ( !rfisc.empty() ) {
            ProgTrace( TRACE5, "rfisc=%s", rfisc.str().c_str() );
            propsSeatsFlags.setFlag(propRFISC);
            if ( setRFISCQuestion(reqNode,resNode,point_id, seat_type,procFlags, layer_type ) ) {
              propsSeatsFlags.setFlag(propRFISCQuestion);
              return propsSeatsFlags;
            }
          }
        }
      }
      passTariffs.trace( TRACE5 );
      if ( salonList.getRFISCMode()/*passTariffs.status() == TSeatTariffMap::stUseRFISC*/ ) {
        SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
        SelfCkinSalonTariff.setTariffMap( point_id, passTariffs );
        seat->SetRFISC( point_id, passTariffs );
        std::map<int, TRFISC,classcomp> vrfiscs;
        seat->GetRFISCs( vrfiscs );
        if ( vrfiscs.find( point_id ) != vrfiscs.end() ) {
          rfisc = vrfiscs[ point_id ];
        }
      }
      else { //���� ०�� ࠡ��� ???
        seat->convertSeatTariffs( point_id );
        rfisc.color = seat->SeatTariff.color;
        rfisc.rate = seat->SeatTariff.rate;
        rfisc.currency_id = seat->SeatTariff.currency_id;
      }
      if ( !rfisc.empty() ) {
        ProgTrace( TRACE5, "rfisc=%s", rfisc.str().c_str() );
      }

/*!!!!      seat->convertSeatTariffs( point_id );
      seat->SetTariffsByColor( passTariffs, true );
      seat->SetRFICSRemarkByColor( point_id, passTariffs );*/
      verifyPlaces.push_back( *seat );
        // �஢�ઠ �� �, �� ���ᠦ�� �� "��" � ���� � ���਩���� ��室�
        // �஢�ઠ �� �, �� �� ����� �ࠢ� �������� ᫮� �� �� ���� �� ���ᠦ���
      TLayerPrioritySeat tmp_layer = seat->getDropBlockedLayer( point_id );
      if ( tmp_layer.layerType() != cltUnknown ) {
        ProgTrace( TRACE5, "getDropBlockedLayer: %s add %s",
                   string(seat->yname+seat->xname).c_str(), tmp_layer.toString().c_str() );
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }

      std::map<int, TSetOfLayerPriority,classcomp > layers;
      seat->GetLayers( layers, glAll );
      for ( std::map<int, TSetOfLayerPriority,classcomp >::iterator ilayers=layers.begin();
            ilayers!=layers.end(); ilayers++ ) {
        if ( ilayers->second.empty() ) {
          continue;
        }
        if ( ilayers->second.begin()->getPaxId() == pax_id &&
             ilayers->second.begin()->layerType() == layer_type ) {
          if ( !procFlags.isFlag( procPaySeatSet ) ) {
            throw UserException( "MSG.SEATS.SEAT_NO.PASSENGER_OWNER" );
          }
          else {
            ProgTrace( TRACE5, "layer_type=%s, seat=%s", EncodeCompLayerType( layer_type ), string(seat->xname + seat->yname).c_str() );
            nseats.insert( seat );
            continue;
          }
        }
        QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( ilayers->second.begin()->layerType() ) );
        ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( ilayers->second.begin()->layerType() ) );
        QrySeatRules.Execute();
        if ( QrySeatRules.Eof ||
           ( QrySeatRules.FieldAsInteger( "pr_owner" ) &&
             pax_id != ilayers->second.begin()->getPaxId() ) ) {
          if ( ilayers->second.begin()->getPaxId() == NoExists ) {
            throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
          }
          string airp_dep, airp_arv;
          Qry.SetVariable( "point_id", ilayers->second.begin()->point_dep() );
          Qry.Execute();
          if ( !Qry.Eof ) {
            airp_dep = Qry.FieldAsString( "airp" );
          }
          Qry.SetVariable( "point_id", ilayers->second.begin()->point_arv() );
          Qry.Execute();
          if ( !Qry.Eof ) {
            airp_arv = Qry.FieldAsString( "airp" );
          }
          throw UserException( "MSG.SEATS.SEAT_NO.OCCUPIED_OTHER_LEG_PASSENGER",
                               LParams()<<LParam("airp_dep", ElemIdToCodeNative(etAirp,airp_dep) )
                                        <<LParam("airp_arv", ElemIdToCodeNative(etAirp,airp_arv) ) );
        }
      }
      TPassenger pass;
      pass.preseat_pax_id = pax_id;
      pass.preseat_layer = layer_type;
      if ( !UsedPayedPreseatForPassenger( operFlt, flag, *seat, pass, false ) ) {
         throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
      }
      strcpy( r.first.line, seat->xname.c_str() );
      strcpy( r.first.row, seat->yname.c_str() );
      r.second = r.first;
      seatRanges.push_back( r );
      logSeats.push_back( make_pair(r, rfisc ) );
      if ( pr_down )
        coord.y++;
      else
        coord.x++;
    }
    bool pr_INFT = ( TReqInfo::Instance()->client_type != ctTerm &&
                     TReqInfo::Instance()->client_type != ctPNL &&
                     !procFlags.isFlag( procPaySeatSet ) && // ࠧ��⪠ ����� ᫮�� �१
                     AllowedAttrsSeat.isWorkINFT( point_id ) &&
                     isINFT( point_id, pax_id ) );
    if ( pr_INFT && !AllowedAttrsSeat.passSeats( pers_type, pr_INFT, verifyPlaces ) ) { //web-���ᠤ�� INFT ����饭�
      tst();
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    if ( procFlags.isFlag( procPaySeatSet ) &&
         ( layerPrioritySeat.layerType() == cltProtBeforePay || layerPrioritySeat.layerType() == cltProtAfterPay ) &&
         !seats.empty() ) { // �뫨 ����� ���� - �� ������ ����������!!!
      ProgTrace( TRACE5, "seats.size()=%zu, nseats.size()=%zu", seats.size(), nseats.size() );
      //�ࠢ������
      if ( seats.size() != nseats.size() ||
           seats != nseats ) {
        tst();
        throw  UserException( "MSG.SEATS.SEAT_NO.NOT_COINCIDE_WITH_PREPAID" );
      }
      propsSeatsFlags.clearFlag(changedOrNotPay);
      if ( !( procFlags.isFlag( procPaySeatSet ) &&
             ( TReqInfo::Instance()->client_type == ctWeb ||
               TReqInfo::Instance()->client_type == ctMobile ) ) ) {
        return propsSeatsFlags;
      }
    }
    tst();
  }

  std::set<TCompLayerType> checkinLayers { cltGoShow, cltTranzit, cltCheckin, cltTCheckin };
  if (
       !procFlags.isFlag( procWaitList ) && //  �� �㣠����, �᫨ ���ᠤ�� ���� � ��
       seat_type == stReseat && //���ᠤ��
       checkinLayers.find( layer_type ) != checkinLayers.end() // 㦥 ��ॣ����஢������
     ) {
    DCSServiceApplying::RequiredRfiscs( DCSAction::ChangeSeatOnDesk, PaxId_t(pax_id) ).throwIfNotExists(); //����� ������ ���ᠤ��, �.�. ������ ���� ��㣨
  }

  int curr_tid = NoExists;
  TPointIdsForCheck point_ids_spp;
  point_ids_spp.insert( make_pair(point_id, layer_type) );
  if ( seat_type != stSeat ) { // ���ᠤ��, ��ᠤ�� - 㤠����� ��ண� ᫮�
    switch( layer_type ) {
        case cltGoShow:
        case cltTranzit:
        case cltCheckin:
        case cltTCheckin:
        Qry.Clear();
        Qry.SQLText =
          "BEGIN "
          " DELETE FROM trip_comp_layers "
          "  WHERE point_id=:point_id AND "
          "        layer_type=:layer_type AND "
          "        pax_id=:pax_id; "
          " IF :tid IS NULL THEN "
          "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
          "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
          " END IF;"
          "END;";
        Qry.CreateVariable( "point_id", otInteger, point_id );
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
        if ( curr_tid == NoExists )
          Qry.CreateVariable( "tid", otInteger, FNull );
        else
          Qry.CreateVariable( "tid", otInteger, curr_tid );
        Qry.Execute();
        curr_tid = Qry.GetVariableAsInteger( "tid" );
        break;
        case cltProtCkin:
        case cltProtAfterPay:
        case cltPNLAfterPay:
        case cltProtBeforePay:
        case cltProtSelfCkin:
            // 㤠����� �� ᠫ���, �᫨ ���� ࠧ��⪠
            DeleteTlgSeatRanges( {layer_type}, pax_id, curr_tid, point_ids_spp );
        break;
     //   case cltProtBeforePay: //WEB ChangeProtPaidLayer
//          break;
      default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  // �����祭�� ������ ᫮�
  if ( seat_type != stDropseat ) { // ��ᠤ�� �� ����� ����
    switch ( layer_type ) {
        case cltGoShow:
        case cltTranzit:
        case cltCheckin:
        case cltTCheckin:
          SaveTripSeatRanges( point_id, layer_type, seatRanges, pax_id, point_id, point_arv, NowUTC() );
          Qry.Clear();
          Qry.SQLText =
          "BEGIN "
          " IF :tid IS NULL THEN "
          "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
          "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
          " END IF;"
          "END;";
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        if ( curr_tid == NoExists )
          Qry.CreateVariable( "tid", otInteger, FNull );
        else
          Qry.CreateVariable( "tid", otInteger, curr_tid );
        Qry.Execute();
        curr_tid = Qry.GetVariableAsInteger( "tid" );
        break;
      case cltProtCkin:
      case cltProtBeforePay: //WEB ChangeProtPaidLayer
      case cltPNLAfterPay:
      case cltProtSelfCkin:
        tst();
        InsertTlgSeatRanges( point_id_tlg, airp_arv, layer_type, seatRanges, pax_id, NoExists, time_limit, false, curr_tid, point_ids_spp );
        break;
      default:
        ProgTrace( TRACE5, "!!! Unuseable layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
/*  { //mvd
    switch ( layer_type ) {
      case cltGoShow:
        case cltTranzit:
      case cltCheckin:
      case cltTCheckin:
        rozysk::sync_pax(pax_id, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr );
        break;
      default:
        break;
    }
  }*/

  tid = curr_tid;

  TReqInfo *reqinfo = TReqInfo::Instance();
  PrmEnum seatPrmEnum("seat", "");

  if ( seat_type == stDropseat ) { //� ��६����� seats ᯨ᮪ ����� ����, ��� ��� ��� � ��ୠ��
    logSeats.clear();
    for ( auto s : seats ) {
      logSeats.push_back( make_pair(TSeatRange(TSeat(s->yname,s->xname), TSeat(s->yname,s->xname)),TRFISC()));
    }
  }


  for ( vector<pair<TSeatRange,TRFISC> >::iterator it=logSeats.begin(); it!=logSeats.end(); it++ ) {
    if (it!=logSeats.begin())
      seatPrmEnum.prms << PrmSmpl<string>("", " ");
    seatPrmEnum.prms << PrmSmpl<string>("", it->first.first.denorm_view(salonList.isCraftLat()));

    if ( !it->second.code.empty() || !it->second.empty())
    {
      seatPrmEnum.prms << PrmSmpl<string>("", "(");
      if ( !it->second.code.empty() )
        seatPrmEnum.prms << PrmSmpl<string>("", it->second.code);

      if ( !it->second.empty() )
      {
        if ( !it->second.code.empty() )
          seatPrmEnum.prms << PrmSmpl<string>("", "/");

        if (passTariffs.status()==TSeatTariffMap::stUseRFISC ||
            passTariffs.status()==TSeatTariffMap::stNotRFISC)
          seatPrmEnum.prms << PrmSmpl<string>("", it->second.rateView())
                           << PrmElem<string>("", etCurrency, it->second.currency_id);
        else
          seatPrmEnum.prms << PrmLexema("", "EVT.UNKNOWN_RATE");
      }
      seatPrmEnum.prms << PrmSmpl<string>("", ")");
    }
  }
  switch( seat_type ) {
    case stSeat:
        switch( layer_type ) {
          case cltGoShow:
          case cltTranzit:
          case cltCheckin:
          case cltTCheckin:
            reqinfo->LocaleToLog(procFlags.isFlag( procWaitList )?"EVT.PASSENGER_SEATED_MANUALLY_WAITLIST":"EVT.PASSENGER_SEATED_MANUALLY",
                                 LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                             << seatPrmEnum,
                                 evtPax, point_id, idx1, idx2);
            break;
        default:;
        }
        break;
    case stReseat:
        switch( layer_type ) {
          case cltGoShow:
          case cltTranzit:
          case cltCheckin:
          case cltTCheckin:
            reqinfo->LocaleToLog(procFlags.isFlag( procWaitList )?"EVT.PASSENGER_CHANGE_SEAT_MANUALLY_WAITLIST":"EVT.PASSENGER_CHANGE_SEAT_MANUALLY",
                                 LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                             << seatPrmEnum,
                                 evtPax, point_id, idx1, idx2);
            if ( prCheckin ) {
              SyncPRSA( operFlt.airline, pax_id, passTariffs.status(), logSeats );
            }
            break;
        default:;
        }
        break;
    case stDropseat:
        switch( layer_type ) {
          case cltGoShow:
          case cltTranzit:
          case cltCheckin:
          case cltTCheckin:
            if (!logSeats.empty())
              reqinfo->LocaleToLog(procFlags.isFlag( procSyncCabinClass )?"EVT.PASSENGER_DISEMBARKED_DUE_TO_CLASS_CHANGE":"EVT.PASSENGER_DISEMBARKED_MANUALLY",
                                   LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                               << seatPrmEnum,
                                   evtPax, point_id, idx1, idx2);
          break;
        default:;
        }
        break;
  }
  std::set<int> paxs_external_logged;
  paxs_external_logged.insert( pax_id );
  check_layer_change( point_ids_spp, paxs_external_logged, whence );
  return propsSeatsFlags;
}

//point_arv,class,ASTRA::TCompLayerType
std::map<int,map<string,map<ASTRA::TCompLayerType,vector<TPassenger> > > > passes;

void dividePassengersToGrpsAutoSeats( TIntStatusSalonPassengers::const_iterator ipass_status,
                                      const SALONS2::TSalonList &salonList,
                                      vector<TPassengers> &passGrps,
                                      std::set<int> &pax_lists_with_baby,
                                      const TRemGrp& remGrp )
{
  TClsGrp &cls_grp = (TClsGrp &)base_tables.get("CLS_GRP");    //cls_grp.code subclass,
  SEATS2::TSublsRems subcls_rems( salonList.getAirline() );
  std::map<std::string, int> comp_rems;
  LoadCompRemarksPriority( comp_rems );

  TPassengers PassWoBaby;
  passGrps.clear();
  for ( std::set<TSalonPax,ComparePassenger>::const_iterator ipass=ipass_status->second.begin();
        ipass!=ipass_status->second.end(); ipass++ ) {
    TWaitListReason waitListReason;
    //!!!TPassSeats ranges;
    string seat_no = ipass->seat_no( "one", salonList.isCraftLat(), waitListReason );
    if ( waitListReason.status == layerValid ) {
      tst();
      continue;
    }
    if ( ipass->is_jmp) {
      //!!!djek ProgError?
      continue;
    }
    TPassenger vpass;
    vpass.grpId = ipass->grp_id;
    vpass.regNo = ipass->reg_no;
    vpass.fullName = ipass->surname;
    vpass.fullName = TrimString( vpass.fullName ) + string(" ") + ipass->name;
    vpass.pers_type = EncodePerson( ipass->pers_type );
    vpass.paxId = ipass->pax_id;
    vpass.point_arv = ipass->point_arv;
    vpass.foundSeats = string("(") + ipass->prior_seat_no( "one", salonList.isCraftLat() ) + string(")");
    vpass.isSeat = false;
    vpass.countPlace = ipass->seats;
    vpass.is_jmp = ipass->is_jmp;
    vpass.cabin_clname = ipass->cabin_cl;
    if ( ipass->pr_infant != ASTRA::NoExists ) {
      ProgTrace( TRACE5, "AutoReSeatsPassengers: pax_id=%d add INFT", vpass.paxId );
      vpass.add_rem( "INFT",remGrp );
    }
    vpass.Step = sRight;
    bool flagCHIN = ( ipass->pr_infant != ASTRA::NoExists ||
                      DecodePerson( vpass.pers_type.c_str() ) != ASTRA::adult ||
                      pax_lists_with_baby.find( vpass.paxId ) != pax_lists_with_baby.end() );
    ProgTrace( TRACE5, "pax_id=%d, flagCHIN=%d,ipass->pr_infant=%d,vpass.pers_type.c_str()=%s, baby=%d", ipass->pax_id, flagCHIN, ipass->pr_infant, vpass.pers_type.c_str(), pax_lists_with_baby.find( vpass.paxId ) != pax_lists_with_baby.end() );
    multiset<CheckIn::TPaxRemItem> rems;
    CheckIn::LoadPaxRem( vpass.paxId, rems);
    for(multiset<CheckIn::TPaxRemItem>::const_iterator r=rems.begin(); r!=rems.end(); ++r)
    {
      if (r->code=="BLND" ||
          r->code=="STCR" ||
          r->code=="UMNR" ||
          r->code=="WCHS" ||
          r->code=="MEDA") {
        flagCHIN=true;
      }
      if ( comp_rems.find( r->code ) != comp_rems.end() &&
           r->code == "STCR" ) {
        vpass.add_rem( "STCR",remGrp );
        vpass.Step = sDown;
      }
    }
    if ( flagCHIN ) {
      vpass.add_rem( "CHIN",remGrp );
    }
    string rem;
    const TBaseTableRow &row=cls_grp.get_row( "id", ipass->cabin_class_grp );
    if ( subcls_rems.IsSubClsRem( row.AsString( "code" ), rem ) ) {
      vpass.add_rem( rem,remGrp );
    }
    ProgTrace( TRACE5, "pass add pax_id=%d, add CHIN=%d", vpass.paxId, flagCHIN );
    //�� ������
    if ( pax_lists_with_baby.find( vpass.paxId ) != pax_lists_with_baby.end() ||
         vpass.isRemark( "CHIN" ) ) {
      TPassengers p;
      p.Add( salonList, vpass );
      passGrps.push_back( p );
    }
    else {
      PassWoBaby.Add( salonList, vpass );
    }
  } //end  grp_layer_type
  passGrps.push_back( PassWoBaby );
}

void AutoReSeatsPassengers( SALONS2::TSalonList &salonList,
                            const SALONS2::TIntArvSalonPassengers &passengers,
                            TSeatAlgoParams ASeatAlgoParams,
                            const TRemGrp& remGrp )
{
  LogTrace(TRACE5) << "AutoReSeatsPassengers: point_id=" << salonList.getDepartureId() << ",getSeatDescription=" << salonList.getSeatDescription();
  BASIC_SALONS::TCompLayerTypes::Enum flag = (GetTripSets( tsAirlineCompLayerPriority, salonList.getfltInfo() )?
                                                  BASIC_SALONS::TCompLayerTypes::Enum::useAirline:
                                                  BASIC_SALONS::TCompLayerTypes::Enum::ignoreAirline);
  SEAT_DESCR::paxsWaitListDescrSeat paxsSeatDescr;
  if ( salonList.getSeatDescription() ) {
    paxsSeatDescr.get( salonList.getDepartureId() );
  }
  std::vector<TCoordSeat> paxsSeats;
  TPassengers Passes;
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  TBabyZones babyZones( salonList.getDepartureId() );
  TEmergencySeats emergencySeats( salonList.getDepartureId() );
  std::set<int> pax_lists_with_baby;
  if ( babyZones.useInfantSection() ||
       emergencySeats.deniedEmergencySection() ) {
    //��室�� ��� ������楢
    std::map<int,TPaxList>::const_iterator ipaxs = salonList.pax_lists.find( salonList.getDepartureId() );
    if ( ipaxs != salonList.pax_lists.end() ) {
      ipaxs->second.setPaxWithInfant( pax_lists_with_baby );
    }
  }

  //arv - ���砫� ���� ���ᠦ��� � ᠬ� ������ ������⮬
  for ( TIntArvSalonPassengers::const_iterator ipass_arv=passengers.begin();
        ipass_arv!=passengers.end(); ipass_arv++ ) {
    //class
    for ( TIntClassSalonPassengers::const_iterator ipass_class=ipass_arv->second.begin();
          ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
      //grp_status
      for ( TIntStatusSalonPassengers::const_iterator ipass_status=ipass_class->second.begin();
            ipass_status!=ipass_class->second.end(); ipass_status++ ) {
        //������塞 ���ᠦ�ࠬ� � ������ �ࠪ���⨪���
        vector<ASTRA::TCompLayerType> grp_layers;
        const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", ipass_status->first );
        if ( DecodePaxStatus(ipass_status->first.c_str()) == psCrew )
          throw EXCEPTIONS::Exception("AutoReSeatsPassengers: DecodePaxStatus(ipass_status->first) == psCrew");
        ASTRA::TCompLayerType grp_layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
        grp_layers.push_back( grp_layer_type );
        SALONS2::TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
        filterRoutes.point_arv = ipass_arv->first;
        SALONS2::TSalons SalonsN;
        if ( salonList._seats.empty() )
          throw EXCEPTIONS::Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
        TDropLayersFlags dropLayersFlags;
        TCreateSalonPropFlags propFlags;
        propFlags.setFlag( clDepOnlyTariff );
        tst();
        if ( !salonList.CreateSalonsForAutoSeats( SalonsN,
                                                  filterRoutes,
                                                  propFlags,
                                                  grp_layers,
                                                  dropLayersFlags ) ) {
          throw EXCEPTIONS::Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
        }
        if ( SalonsN.placelists.empty() )
          throw EXCEPTIONS::Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
        //����砥� ������ ���� ���ᠦ�ࠬ�
        AllowedAttrsSeat.isWorkINFT( SalonsN.trip_id );
        int s = Passes.getCount();
        ProgTrace( TRACE5, "Passes.getCount()=%d", Passes.getCount() );
        for ( int i=0; i<s; i++ ) {
          TPassenger &pass = Passes.Get( i );
          if ( !pass.InUse ) {
            continue;
          }
          SALONS2::TPoint FP( pass.Pos.x, pass.Pos.y );
          for ( int j=0; j<pass.countPlace; j++ ) {
            SALONS2::TPlace *place = pass.placeList->place( FP );
            switch( (int)pass.Step ) {
              case sRight:
                 FP.x++;
                 break;
              case sDown:
                 FP.y++;
                 break;
            }
            ProgTrace( TRACE5, "add cltCheckin pax_id=%d, point_dep=%d, point_arv=%d",
                       pass.paxId, salonList.getDepartureId(), pass.point_arv );
            place->AddLayerToPlace( cltCheckin, NowUTC(), pass.paxId,
                                    salonList.getDepartureId(), pass.point_arv,
                                    BASIC_SALONS::TCompLayerTypes::Instance()->priority( BASIC_SALONS::TCompLayerTypes::LayerKey( salonList.getAirline(), cltCheckin ),
                                                                                         flag ) );
            if ( CurrSalon::get().canAddOccupy( place ) ) {
              CurrSalon::get().AddOccupySeat( pass.placeList->num, place->x, place->y );
            }
          }
          tst();
        }
        tst();
        Passengers.Clear();
        FSeatAlgoParams = ASeatAlgoParams;
        CurrSalon::set( salonList.getAirline(), flag, &SalonsN );
        SeatAlg = sSeatPassengers;
        CanUseLayers = false; /* �� ���뢠�� ����� ���� */
        UseLayers[ cltProtCkin ] = true;
        UseLayers[ cltProtBeforePay ] = true;
        UseLayers[ cltProtAfterPay ] = true;
        UseLayers[ cltPNLBeforePay ] = true;
        UseLayers[ cltPNLAfterPay ] = true;
        UseLayers[ cltProtSelfCkin ] = true;
        //CanUse_PS = true;
        CanUseSmoke = false;
        AllowedAttrsSeat.clearAll();
        CanUseRems = sIgnoreUse;
        Remarks.clear();
        CanUseTube = true;
        CanUseAlone = uTrue;
        SeatPlaces.Clear();
        condRates.Init( SalonsN, false, TReqInfo::Instance()->client_type ); // ᮡ�ࠥ� �� ⨯� ������ ���� � ���ᨢ �� �ਮ��⠬, �᫨ �� web-������, � �� ���뢠��

        try {
          //����� ��㯯
          vector<TPassengers> passGrps;
          dividePassengersToGrpsAutoSeats( ipass_status, salonList, passGrps, pax_lists_with_baby, remGrp );
          for ( vector<TPassengers>::iterator ipasses=passGrps.begin(); ipasses !=passGrps.end(); ++ipasses ) {
            int len = ipasses->getCount();
            if ( !len ) {
              continue;
            }
            Passengers.Clear();
            for ( int k=0; k<len; k++ ) {
              TPassenger vpass = ipasses->Get( k );
              vpass.grp_status = grp_layer_type;
              Passes.Add( vpass );
              std::vector<std::string> v;
              paxsSeatDescr.getDescr( vpass.paxId, v );
              vpass.seatsDescr.addSeatDescr(v);
              Passengers.Add( vpass );
              if ( !salonList.getSeatDescription() ) { //�஡�� �� ������ ���⠬ �� ��ᠤ�� ��� ��� ᢮��� ����
                ExistsBasePlace( salonList.getfltInfo(), flag, SalonsN, vpass ); // ���ᠦ�� �� ��ᠦ��, �� ��諮�� ��� ���� ������� ���� - ����⨫� ��� ����� //??? ����஢�� !!!
              }
            }
            if ( Passengers.getCount() ) {
              Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
              Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
              Passengers.SeatDescription = salonList.getSeatDescription();
              seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType,
                                        Passengers.KWindow,
                                        Passengers.KTube,
                                        paxsSeats );
              /* ��ᠤ�� ���ᠦ�� � ���ண� �� ������� ������� ���� */
              ProgTrace( TRACE5, "AutoReSeatsPassengers: Passengers.getCount()=%d, layer_type=%s",
                         Passengers.getCount(), grp_status_row.layer_type.c_str()  );
              SeatPlaces.grp_status = Passengers.Get( 0 ).grp_status;
              bool separately_seats_adult_with_baby = babyZones.useInfantSection() && Passengers.withBaby();
              bool denial_emergency_seats = emergencySeats.deniedEmergencySection() && Passengers.withCHIN();
              if ( separately_seats_adult_with_baby ) {
                SeatPlaces.separately_seats_adults_crs_pax_id = Passengers.Get( 0 ).paxId;
              }
              else {
                SeatPlaces.separately_seats_adults_crs_pax_id = ASTRA::NoExists;
              }
              babyZones.clear();
              emergencySeats.clear();
              if ( separately_seats_adult_with_baby ||
                   denial_emergency_seats ) {
                TPaxsCover paxs;
                for ( int i=0; i<Passengers.getCount(); i++ ) {
                    paxs.push_back( TPaxCover( Passengers.Get(i).paxId, ASTRA::NoExists ) );
                }
                babyZones.setDisabledBabySection( &CurrSalon::get(), paxs, pax_lists_with_baby );
                emergencySeats.setDisabledEmergencySeats( &CurrSalon::get(), paxs );
              }
              SeatPlaces.SeatsPassengers( true );
              emergencySeats.rollbackDisabledEmergencySeats( &CurrSalon::get() );
              babyZones.rollbackDisabledBabySection( &CurrSalon::get() );
              SeatPlaces.RollBack( );
              int s = Passengers.getCount();
              for ( int i=0; i<s; i++ ) {
                TPassenger &pass = Passengers.Get( i );
                int k = Passes.getCount();
                for ( int j=0; j<k; j++	 ) {
                  TPassenger &opass = Passes.Get( j );
                if ( opass.paxId == pass.paxId ) {
                    opass = pass;
                    if ( opass.isSeat ) {
                      paxsSeats.push_back( TCoordSeat( opass.placeList->num, opass.Pos.x, opass.Pos.y ) );
                    }
                    //ProgTrace( TRACE5, "pass=%s", pass.toString().c_str() );
                    break;
                  }
                }
              }
            }
          }
        }
        catch( ... ) {
          SeatPlaces.RollBack( );
          throw;
        }
        SeatPlaces.RollBack( );
        tst();
      } //end grp_status
    } //end class
  } //end point_arv

  TQuery QryLayer( &OraSession );
  TQuery QryUpd( &OraSession );
  QryLayer.SQLText =
    "DELETE FROM trip_comp_layers "
    " WHERE pax_id=:pax_id ";
  QryLayer.DeclareVariable( "pax_id", otInteger );
  QryUpd.SQLText =
    "UPDATE pax SET tid=cycle_tid__seq.nextval WHERE pax_id=:pax_id";
  QryUpd.DeclareVariable( "pax_id", otInteger );
  TDateTime time_create = NowUTC();
  int s = Passes.getCount();
//  bool pr_is_sync_paxs = is_sync_paxs( salonList.getDepartureId() );
  for ( int i=0; i<s; i++ ) {
    TPassenger &pass = Passes.Get( i );
    ProgTrace( TRACE5, "pass.pax_id=%d, pass.isSeat=%d", pass.paxId, pass.isSeat );
      if ( pass.isSeat ) {
      tst();
        continue;
    }
    if ( pass.InUse ) { /* ᬮ��� ��ᠤ��� */
      TSeatRanges seats;
      for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
          TSeatRange r(*i,*i);
          seats.push_back( r );
      }
      // ����室��� ���砫� 㤠���� �� ��� ��������� ���� �� ᫮� ॣ����樨
      QryLayer.SetVariable( "pax_id", pass.paxId );
      QryLayer.Execute();
      SaveTripSeatRanges( salonList.getDepartureId(), pass.grp_status, seats, pass.paxId, salonList.getDepartureId(), pass.point_arv, time_create ); //???
      QryUpd.SetVariable( "pax_id", pass.paxId );
      QryUpd.Execute();
    }
  }
  check_waitlist_alarm_on_tranzit_routes( salonList.getDepartureId(), __FUNCTION__ );
/*  if ( pr_is_sync_paxs ) {
    for ( vector<TPassenger>::iterator ipass=paxs.begin(); ipass!=paxs.end(); ipass++ ) {
      update_pax_change( salonList.getDepartureId(), ipass->paxId, ipass->regNo, "�" );
    }
  }*/
  ProgTrace( TRACE5, "passengers.count=%d", Passes.getCount() );
}

TSeatAlgoParams GetSeatAlgo(TQuery &Qry, string airline, int flt_no, string airp_dep)
{
  Qry.Clear();
  Qry.SQLText=
    "SELECT algo_type, pr_seat_in_row,"
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM seat_algo_sets "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,airline);
  Qry.CreateVariable("flt_no",otInteger,flt_no);
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.Execute();
  TSeatAlgoParams res;
  if (!Qry.Eof)
  {
    res.SeatAlgoType=(TSeatAlgoTypes)Qry.FieldAsInteger("algo_type");
    res.pr_canUseOneRow = Qry.FieldAsInteger("pr_seat_in_row");
  };
  Qry.Close();
  return res;
}

bool CompGrp( TPassenger item1, TPassenger item2 )
{
  TBaseTable &classes=base_tables.get("classes");
  const TBaseTableRow &row1=classes.get_row("code",item1.cabin_clname);
  const TBaseTableRow &row2=classes.get_row("code",item2.cabin_clname);
  if ( row1.AsInteger( "priority" ) < row2.AsInteger( "priority" ) )
    return true;
  else
    if ( row1.AsInteger( "priority" ) > row2.AsInteger( "priority" ) )
        return false;
    else
        if ( item1.grpId < item2.grpId )
          return true;
        else
            if ( item1.grpId > item2.grpId )
                return false;
            else
                if ( item1.paxId < item2.paxId )
                    return true;
                else
                    if ( item1.paxId > item2.paxId )
                        return false;
                    else
                        return true;
};

bool TPassengers::existsNoSeats()
{
  int s = getCount();
  for ( int i=0; i<s; i++ ) {
    if ( !Get( i ).InUse )
      return true;
  }
  return false;
}

bool isCheckinWOChoiceSeats( int point_id )
{
  TCachedQuery Qry("SELECT point_id,airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id AND pr_del>=0",
               QParams() << QParam("point_id", otInteger, point_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  TTripInfo info(Qry.get());
  return GetSelfCkinSets( tsKioskCheckinOnPaidSeat, info, TReqInfo::Instance()->client_type );
}

TPassengers Passengers;


} // end namespace SEATS2


//seats with:SeatAlg=2,FCanUseRems=sIgnoreUse,FCanUseAlone=0,KeyStatus=1,FCanUseTube=0,FCanUseSmoke=0,PlaceStatus=, MAXPLACE=3,canUseOneRow=0, CanUseSUBCLS=1, SUBCLS_REM=SUBCLS
//seats with:SeatAlg=2,FCanUseRems=sNotUse_NotUseDenial,FCanUseAlone=1,KeyStatus=1,FCanUseTube=0,FCanUseS



