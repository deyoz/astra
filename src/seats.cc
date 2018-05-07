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
#include "convert.h"
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

#define NICKNAME "DJEK"
#include "serverlib/test.h"

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
  void addSeat( TPlaceList *placeList, int x, int xlen, int y, bool pr_window, bool pr_tube, bool pr_seats );
  std::string toString() const;
};

typedef vector<TSeatCoordsSalon> vecSeatCoordsVars;

class TSeatCoordsClass {
  private:
    vecSeatCoordsVars vecSeatCoords;
    void clear();
    void addBaseSeat( TPlaceList* placeList, TPoint &p );
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
 aAllUse - совпадение всех ремарок по самому приоритетному пассажиру
 sMaxUse - совпадение самой приоритетной ремарки
 sOnlyUse - хотя бы одна совпадает
 sNotUse - запрещено использовать место с разрешенной ремаркой, если у пассажира такой ремарки нет
 sNotUseDenial - запрещено использовать место с запрещенной ремаркой, кот есть у пассажира
 sIgnoreUse - игнорировать ремарки
*/
/* Нельзя разбивать трех, нельзя сажать по одному более одного раза, все можно */
enum TUseAlone { uFalse3 /* нельзя оставлять одного при рассадке группы*/,
                 uFalse1 /* можно оставлять одного только один раз при рассадке группы*/,
                 uTrue /*можно оставлять одного при рассадке группы любое кол-во раз*/ };

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

/* глобальные переменные для этого модуля */
TSeatPlaces SeatPlaces;
SALONS2::TSalons *CurrSalon;
TSeatCoordsClass seatCoords;

bool CanUseLayers; /* поиск по слою места */
bool CanUseMutiLayer; /* поиск по группе слоев */
TCompLayerType PlaceLayer; /* сам слой */
vector<TCompLayerType> SeatsLayers;
TUseLayers UseLayers; // можно ли использовать место со слоем для рассадки пассажиров с другими слоями

//bool CanUse_PS; /* можно ли использовать статус предв. рассадки для пассажиров с другими статусами */
bool CanUseSmoke; /* поиск курящих мест */

struct TAllowedAttributesSeat {
  vector<string> ElemTypes; /* разрешенные типы мест поиск мест по типу (табуретка)*/
  bool pr_isWorkINFT; /* РГ */
  int point_id;
  bool pr_INFT; /*признак рассадки подгруппы INFT */
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
    elems.push_back( "Д" ); //!!! здесь коды разрешенных мест
    elems.push_back( "К" ); //!!!
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
                      (string("РГ") == Qry.FieldAsString( "airline")/* || string("ЮТ") == Qry.FieldAsString( "airline")*/));
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
    if ( pers_type != "ВЗ" ) { // проверка на места у аварийного выхода
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

TUseRem CanUseRems; /* поиск по ремарке */
vector<string> Remarks; /* сама ремарка */
bool CanUseTube; /* поиск через проходы */
TUseAlone CanUseAlone; /* можно ли использовать посадку одного в ряду - может посадить
                          группу друг за другом */
TSeatAlg SeatAlg;
bool FindSUBCLS=false; // исходим из того, что в группе не может быть пассажиров с разными подклассами!
bool canUseSUBCLS=false;
string SUBCLS_REM;

bool canUseOneRow=true; // использовать при рассадке только один ряд - определяется на основе ф-ции getCanUseOneRow() и цикла

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
  bool pr_web; //признак того, что регистрируется платный пассажир
  bool use_rate; // использовать платные места для пассажиров без оплаченных мест
  bool ignore_rate; // игнорируем тариф
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
    use_rate = ( client_type == ctTerm || client_type == ctPNL );
    ProgTrace( TRACE5, "TCondRate::Init use_rate=%d", use_rate);
    rates.clear();
    ignore_rate = false;
    rates.insert( TSeatTariff( "", 0.0, "" ) ); // всегда задаем - означает, что надо использовать места без тарифа
    rates.insert( TSeatTariff( "", INT_MAX, "" ) );
    //!!!rates[ 0 ].rate = 0.0; // всегда задаем - означает, что надо использовать места без тарифа
    if ( pr_web ) {// не учитываем платные места
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
        if ( vars.find( univalue ) == vars.end() ) { // не нашли
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

  bool CanUseRate( TPlace *place ) { /* если все возможные тарифы попробовали при рассадке и не смогли рассадить или нет тарифов на рейсе или место без тарифа, то можно использовать */
    SeatsStat.start(__FUNCTION__);
    bool res = ( pr_web || (current_rate_end() && use_rate) /*!!!|| place->SeatTariff.empty()*/ || ignore_rate );
//    ProgTrace( TRACE5, "CanUseRate: x=%d, y=%d, place->SeatTariff=%s, res=%d,use_rate=%d, curr rate=%s",
//               place->x, place->y, place->SeatTariff.str().c_str(), res, use_rate, current_rate->str().c_str() );

    if ( !res ) {
      for ( set<TSeatTariff,RateCompare>::iterator i=rates.begin(); ; i++ ) { // просмотр всех тарифов, кот. сортированы в порядке возрастания приоритета использования
        if ( i != rates.end() &&
             i->rate == place->SeatTariff.rate &&
            ( (i->color.empty() && !place->SeatTariff.currency_id.empty()) || //цвет не задан  0.0 и у места задан тариф (не неизвестен !place->SeatTariff.currency_id.empty() )
              (i->color == place->SeatTariff.color && i->currency_id == place->SeatTariff.currency_id) ) ) {
          if ( use_rate || i->rate == 0.0 ) {
            res = true;
          }
          break;
        }
        if ( i == current_rate ) // далее тарифы нам не доступны
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

TSeatAlgoParams FSeatAlgoParams; // алгоритм рассадки


void GetUseLayers( TUseLayers &uselayers )
{
  uselayers.clear();
}

bool CanUseLayer( const TCompLayerType &layer, TUseLayers FUseLayers )
{
  return ( FUseLayers.find( layer ) != FUseLayers.end() && FUseLayers[ layer ] );
}

int MAXPLACE() {
    return (canUseOneRow)?1000:CONST_MAXPLACE; // при рассадке в один ряд кол-во мест в ряду неограничено иначе 3 места
}

bool getCanUseOneRow() {
    return FSeatAlgoParams.pr_canUseOneRow;
}

void TSeatCoordsSalon::addSeat( TPlaceList *placeList, int x, int xlen, int y,
                                bool pr_window, bool pr_tube, bool pr_seats )
{
  SALONS2::TPoint p( x, y );
  TPlace *place = placeList->place( p );
  if ( !place->isplace ||
       !place->visible ||
       CurrSalon->isExistsOccupySeat( placeList->num, x, y ) ) {
    return;
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
void TSeatCoordsClass::addBaseSeat( TPlaceList* placeList, TPoint &p )
{
  if ( !placeList->ValidPlace( p ) ) {
    return;
  }
  for ( vecSeatCoordsVars::iterator ilist=vecSeatCoords.begin(); ilist!=vecSeatCoords.end(); ilist++ ) {
    if ( ilist->placeList->num == placeList->num ) {
      ilist->addSeat( placeList, p.x, 0, p.y, false, false, true );
      ProgTrace( TRACE5, "addBaseSeat: x=%d, y=%d", p.x, p.y );
      return;
    }
  }
  TSeatCoordsSalon coordSalon;
  coordSalon.placeList = placeList;
  coordSalon.addSeat( placeList, p.x, 0, p.y, false, false, true );
  //ProgTrace( TRACE5, "addBaseSeat: x=%d, y=%d", p.x, p.y );
  vecSeatCoords.push_back( coordSalon );
}

void TSeatCoordsClass::addBaseSeats( const std::vector<TCoordSeat> &paxSeats )
{
   for ( std::vector<TCoordSeat>::const_iterator iseat=paxSeats.begin(); iseat!=paxSeats.end(); iseat++ ) {
     //находим свободные места вокруг
     for ( std::vector<TPlaceList*>::iterator item=CurrSalon->placelists.begin(); item!=CurrSalon->placelists.end(); item++ ) {
       if ( iseat->placeListIdx == (*item)->num ) {
         int xlen = (*item)->GetXsCount();
         TPoint p1 = iseat->p;
         p1.x++;
         if ( p1.x <= xlen - 1 && (*item)->GetXsName( p1.x ).empty() ) {
           p1.x++;
           addBaseSeat( *item, p1 );
         }
         else {
           addBaseSeat( *item, p1 );
         }
         p1 = iseat->p;
         p1.x--;
         if ( p1.x >= 0 && (*item)->GetXsName( p1.x ).empty() ) {
           p1.x--;
           addBaseSeat( *item, p1 );
         }
         else {
           addBaseSeat( *item, p1 );
         }
         p1 = iseat->p;
         p1.y--;
         addBaseSeat( *item, p1 );
         p1 = iseat->p;
         p1.y++;
         addBaseSeat( *item, p1 );
         break;
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
  clear();
  int xlen, ylen;
  // сверху вниз
  bool pr_UpDown = ( ASeatAlgoType == sdUpDown_Row || ASeatAlgoType == sdUpDown_Line );
  if ( pr_UpDown ) {
    for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
          iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
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
  else { // снизу вверх
    for ( vector<SALONS2::TPlaceList*>::reverse_iterator iplaceList=CurrSalon->placelists.rbegin();
          iplaceList!=CurrSalon->placelists.rend(); iplaceList++ ) {
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

/* откат части найденных мест для рассадки */
void TSeatPlaces::RollBack( int Begin, int End )
{
  if ( seatplaces.empty() )
    return;
  SeatsStat.start(__FUNCTION__);
  /* пробег по измененным найденным местам */
  TSeatPlace seatPlace;
  for ( int i=Begin; i<=End; i++ ) {
    seatPlace = seatplaces[ i ];
    /* пробег по старым сохраненным местам */
    for ( vector<SALONS2::TPlace>::iterator place=seatPlace.oldPlaces.begin();
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* получение места */
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      SALONS2::TPlace *pl = seatPlace.placeList->place( idx );
      /* отмена всех изменений места */
      *pl = *place;
      if ( CurrSalon->canAddOccupy( pl ) ) {
        CurrSalon->AddOccupySeat( seatPlace.placeList->num, pl->x, pl->y );
      }
      else {
        CurrSalon->RemoveOccupySeat( seatPlace.placeList->num, pl->x, pl->y );
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
      default: throw EXCEPTIONS::Exception( "Ошибка рассадки" );
    }
    seatPlace.oldPlaces.clear();
  } /* end for */
  vector<TSeatPlace>::iterator b = seatplaces.begin();
  seatplaces.erase( b + Begin, b + End + 1 );
  SeatsStat.stop(__FUNCTION__);
}

/* откат всех найденных мест для рассадки */
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

/* Заносим найденные места.
  FP - указатель на исходное место от которого начинается поиск
  EP - указатель на первое найденное место
  FoundCount - кол-во найденных мест
  Step - направление отсчета найденных мест
  Возвращаем кол-во использованных мест */
int TSeatPlaces::Put_Find_Places( SALONS2::TPoint FP, SALONS2::TPoint EP, int foundCount, TSeatStep Step )
{
  SeatsStat.start(__FUNCTION__);
  int p_RCount = 0, p_RCount2 = 0, p_RCount3 = 0; /* необходимое кол-во 3-х, 2-х, 1-х мест */
  int pp_Count = 0, pp_Count2 = 0, pp_Count3 = 0; /* имеющееся кол-во 3-х, 2-х, 1-х мест */
  int NTrunc_Count = 0, Trunc_Count = 0; /* кол-во выделенных из общего числа данных мест */
  int p_Prior = 0, p_Next = 0; /* Кол-во мест до FP и после него */
  int p_Step = 0; /* направление рассадки. Определяется в зависимости от кол-ва p_Prior и p_Next */
  int Need = 0;
  SALONS2::TPlaceList *placeList;
  int Result = 0; /* общее кол-во задействованных мест */
  if ( foundCount == 0 ) {
   SeatsStat.stop(__FUNCTION__);
   return Result; // не задано мест
  }
  placeList = CurrSalon->CurrPlaceList();
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
           p_Prior = abs( FP.y - EP.y ); // разница м/у FP и EP
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
      case 1: if ( p_RCount > 0 ) /* нужны 1-х местные места ?*/
                Trunc_Count = 1;
              else
                Trunc_Count = 0;
              break;
      case 2: if ( p_RCount2 > 0 ) /* нужны 2-х местные места ?*/
                Trunc_Count = 2;
              else
                Trunc_Count = 1;
              break;
      case 3: if ( p_RCount3 > 0 ) /* нужны 3-х местные места ?*/
                Trunc_Count = 3; /* нужны 3-х местные места */
              else
                Trunc_Count = 2; /* нужны 2-х местные места ? */
              break;
      default: Trunc_Count = 3;
    }
    if ( !Trunc_Count ) /* больше ничего не нужно из того, что предлагается */
      break;
    if ( Trunc_Count != NTrunc_Count ) {
      NTrunc_Count = Trunc_Count; /* выделили меньшую часть, а то такая большая не нужна */
      continue;
    }
   /* если мы здесь, то тогда мы имеем места, которые нас устраивают по кол-ву
      теперь перед нами стоит вопрос, с какого места начать посадку, если мест больше,
      чем мы сейчас выделии?
      Ответ: для того, чтобы наиболее быть близким к исходной точке FP
      начнем с такого края, у которого меньше мест до FP. */
    if ( !p_Step ) { /* признак того, что мы пока не определили с какого края надо начинать */
      Need = p_RCount; /* мы нуждаемся в таком кол-ве 1-х мест */
      switch( Trunc_Count ) {
        case 3: Need += p_RCount3*3 + p_RCount2*2; /* если надо 3, то учитываем все */
                break;
        case 2: Need += p_RCount2*2; /* если надо 2 => то не надо 3-х или общее кол-во мест не позволяет, */
                break;               /* тогда надо учитывать только 1-х и 2-х места */
      }


      /* Need - это общее нужное кол-во мест, которое возможно удасться выделить
         теперь определяем направление */
      if ( p_Prior >= p_Next && Need > p_Next ) { /* слева больше мест, начнем справа */
        p_Step = -1;
        if ( p_Next < Need ) /* направо/вниз больше мест, чем возможно понадобится */
          Need = p_Next; /* тогда больше мы не можем потребовать! */
        switch( (int)Step ) {
          case sRight:
          case sLeft:
             EP.x = FP.x + Need - 1; /* перемещаемся на исходную позицию */
             Step = sLeft; /* начинаем двигаться влево */
             break;
          case sDown:
          case sUp:
             EP.y = FP.y + Need - 1; /* перемещаемся на исходную позицию */
             Step = sUp; /* начинаем двигаться вверх */
             break;
        }
      }
      else {
       p_Step = 1;  /* начнем слева */
       if ( Need <= p_Next ) {
         Need = 0; /* ничего не трогаем и начинаем с текущей позиции */
       }
       else {
         if ( p_Prior < Need )
           Need = p_Prior;
       }
       switch( (int)Step ) {
         case sRight:
         case sLeft:
            EP.x = FP.x - Need; /* перемещаемся на исходную позицию */
            Step = sRight; /* начинаем двигаться вправо */
            break;
         case sDown:
         case sUp:
            EP.y = FP.y - Need; /* перемещаемся на исходную позицию */
            Step = sDown; /* начинаем двигаться вниз */
            break;
       }
      } /* конец определения направления ( p_Prior >= p_Next ... ) */
    } /* конец p_Step = 0 */

    /* закончили определять исходную позицию места и направления рассад */

    /* выделяем еще одно место под набор мест */
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
    /* сохраняем старые места и помечаем места в салоне как занятые */
    for ( int i=0; i<Trunc_Count; i++ ) {
      SALONS2::TPlace place;
      SALONS2::TPlace *pl = placeList->place( EP );
      place = *pl;
      if ( !CurrSalon->placeIsFree( &place ) || !place.isplace ) {
        throw EXCEPTIONS::Exception( "Рассадка выполнила недопустимую операцию: использование уже занятого места" );
      }
      seatplace.oldPlaces.push_back( place );
      pl->AddLayerToPlace( grp_status, 0, separately_seats_adults_crs_pax_id, NoExists, NoExists, CurrSalon->getPriority( grp_status ) );
      if ( CurrSalon->canAddOccupy( pl ) ) {
        CurrSalon->AddOccupySeat( placeList->num, pl->x, pl->y );
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
    } /* закончили сохранять */
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
    Result += Trunc_Count; /* кол-во уже использованных мест */
    foundCount -= Trunc_Count; /* общее кол-во не использованных мест */
    NTrunc_Count = foundCount; /* кол-во мест, которые сейчас начнут разбираться */
//    ProgTrace( TRACE5, "Result=%d", Result );
  } /* end while */
  SeatsStat.stop(__FUNCTION__);
  return Result;
}

/* ф-ция для определения возможности рассадки для мест у которых есть запрещенные ремарки */
bool DoNotRemsSeats( const vector<SALONS2::TRem> &rems )
{
    bool res = false; // признак того, что у пассажира есть ремарка, которая запрещена на выбранном месте
    bool pr_passsubcls = false;
    // определяем есть ли у пассажира ремарка подкласса
    //unsigned int i=0;
    vector<string>::iterator yrem=Remarks.begin();
    for (; yrem!=Remarks.end(); yrem++ ) {
        if ( isREM_SUBCLS( *yrem ) ) {
            pr_passsubcls = true;
            break;
        }
    }

/* old version	for ( ; i<sizeof(TSubcls_Remarks)/sizeof(const char*); i++ ) {
        if ( find( Remarks.begin(), Remarks.end(), string(TSubcls_Remarks[i]) ) != Remarks.end() ) { // у пассажира SUBCLS
            pr_passsubcls = true;
            break;
      }
    }*/

    bool no_subcls = false;
    // определяем есть ли запрещенная ремарка на месте, кот. заданы у пассажира
    for( vector<string>::iterator nrem=Remarks.begin(); nrem!=Remarks.end(); nrem++ ) {
      for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
          if ( !prem->pr_denial )
              continue;
            if ( *nrem == prem->rem && !res )
                res = true;
      }
    }
    // пробег по ремаркам места, если есть ремарка подкласса
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
  return res || no_subcls; // если есть запрещенная ремарка у пассажира или место с ремаркой SUBCLS, а у пассажира ее нет
}

bool VerifyUseLayer( TPlace *place )
{
    bool res = !CanUseLayers || ( PlaceLayer == cltUnknown && place->layers.empty() );
    if ( !res ) {
        for (std::vector<TPlaceLayer>::iterator i=place->layers.begin(); i!=place->layers.end(); i++ ) {
            if ( find( SeatsLayers.begin(), SeatsLayers.end(), i->layer_type ) == SeatsLayers.end() ) { // найден более приоритетный слой, которого нет в списке дозволенных слоев
                break;
          }
          if ( CanUseMutiLayer || i->layer_type == PlaceLayer ) { // найден нужный слой
            res = true;
            break;
          }
      }
    }
    return res;
}

/* поиск мест расположенных рядом последовательно
   возвращает кол-во найденных мест.
   FP - адрес места, с которого надо искать
   FoundCount - кол-во уже найденных мест
   Step - направление поиска
  Глобальные переменные:
   CanUselayer, Placelayer - поиск строго по статусу мест,
   CanUseSmoke - поиск курящих мест,
   AllowedAttrsSeat.ElemTypes - поиск строго по типу места (табуретка),
   AllowedAttrsSeat.Remarks - поиск строго по INFT
   CanUseRem, PlaceRem - поиск строго по ремарке места */
int TSeatPlaces::FindPlaces_From( SALONS2::TPoint FP, int foundCount, TSeatStep Step )
{
  SeatsStat.start(__FUNCTION__);
  int Result = 0;
  SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
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
  while ( !CurrSalon->isExistsOccupySeat( placeList->num, place->x, place->y ) &&
          //CurrSalon->placeIsFree( place ) && place->isplace && place->visible &&
          place->clname == Passengers.clname &&
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
           ( !FindSUBCLS && prem != place->rems.end() ) ) //  не смогли посадить на класс SUBCLS
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
         if ( place->rems.empty() || Remarks.empty() ) { // найдем хотя бы одну совпадающую
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
    Result++; /* нашли еще одно место */
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
  SeatsStat.stop(__FUNCTION__);
  return Result;
}

/* Рассадка группы в PlaceList, начиная с позиции FP,
   в направлении Step ( рассматриваются только sRight - горизонтальная ,
   и sDown - вертикальная рассадка => рассматриваем только пассажиров опр. группы.
   Приоритеты поиска sRight: sRight, sLeft, sTubeRight (через проход), sTubeLeft ).
   Wanted - особый случай, когда надо посадить 2x2, а не 3 и 1.
  Глобальные переменные:
   CanUseTube - поиск при Step = sRight через проходы
   Alone - посадить одно пассажира в ряду можно только один раз */
bool TSeatPlaces::SeatSubGrp_On( SALONS2::TPoint FP, TSeatStep Step, int Wanted )
{
  SeatsStat.start(__FUNCTION__);
  if ( Step == sLeft )
    Step = sRight;
  if ( Step == sUp )
    Step = sDown;
//  ProgTrace( TRACE5, "SeatSubGrp_On FP=(%d,%d), Step=%d, Wanted=%d", FP.x, FP.y, Step, Wanted );
  int foundCount = 0; // кол-во найденных мест всего
  int foundBefore = 0; // кол-во найденных мест до и после FP
  int foundTubeBefore = 0;
  int foundTubeAfter = 0;
  SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 ) /*нельзя оставлять одного*/
    Alone = true;// это первый заход сюда, надо проинициализировать гл. переменную Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
  if ( !foundAfter ) {
    SeatsStat.stop(__FUNCTION__);
    return false;
  }
//  ProgTrace( TRACE5, "FP=(%d,%d), foundafter=%d", FP.x, FP.y, foundAfter );
  foundCount += foundAfter;
  if ( foundAfter && Wanted ) { // если мы нашли места и нам надо Wanted
    if ( foundAfter > Wanted )
      foundAfter = Wanted;
    Wanted -= Put_Find_Places( FP, FP, foundAfter, Step );
    if ( Wanted <= 0 ) {
      SeatsStat.stop(__FUNCTION__);
      return true; // Ура все нашлось
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
  if ( foundBefore && Wanted ) { // если мы нашли места и нам надо Wanted
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
      return true; /* Ура все нашлось */
    }
  }
 /* далее попытаемся поискать через проход, при условии что поиск по горизонтали */
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE() ) {
    EP.x = FP.x + foundAfter + 1; //???/* устанавливаемся на предполагаемое место */
    if ( placeList->ValidPlace( EP ) ) {
      SALONS2::TPoint p( EP.x - 1, EP.y );
      SALONS2::TPlace *place = placeList->place( p ); /* берем пред. место */
      if ( !place->visible ) {
        foundTubeAfter = FindPlaces_From( EP, foundCount, Step ); /* поиск после прохода */
        foundCount += foundTubeAfter; /* увеличиваем общее кол-во мест */
//       ProgTrace( TRACE5, "FP=(%d,%d), foundTubeAfter=%d", EP.x, EP.y, foundTubeAfter );
        if ( foundTubeAfter && Wanted ) { /* если мы нашли места и нам надо Wanted */
          if ( foundTubeAfter > Wanted )
            foundTubeAfter = Wanted;
           Wanted -= Put_Find_Places( EP, EP, foundTubeAfter, Step ); /* первый параметр EP */
         /* т.к. точка отсчета должна находится на первом месте после прохода,
            иначе работать не будет */
           if ( Wanted <= 0 ) {
             SeatsStat.stop(__FUNCTION__);
             return true; /* Ура все нашлось */
           }
        }
      }
    }
    /* далее поиск налево через проход */
    if ( foundCount < MAXPLACE() ) {
      EP.x = FP.x - foundBefore - 2; // устанавливаемся на предполагаемое место
      //SALONS2::TPoint VP = EP;
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      if ( placeList->ValidPlace( EP ) ) {
        SALONS2::TPoint p( EP.x + 1, EP.y );
        SALONS2::TPlace *place = placeList->place( p ); /* берем след. место */
        if ( !place->visible ) { /* следующее место не видно => проход */
          foundTubeBefore = FindPlaces_From( EP, foundCount, sLeft );
          foundCount += foundTubeBefore;
//          ProgTrace( TRACE5, "EP=(%d,%d), foundTubeBefore=%d", EP.x, EP.y, foundTubeBefore );
          if ( foundTubeBefore && Wanted ) { /* если мы нашли места и нам надо Wanted */
            if ( foundTubeBefore > Wanted )
              foundTubeBefore = Wanted;
            EP.x -= foundTubeBefore - 1;
            Wanted -= Put_Find_Places( EP, EP, foundTubeBefore, sLeft ); /* первый параметр EP */
            if ( Wanted <= 0 ) {
              SeatsStat.stop(__FUNCTION__);
              return true; /* Ура все нашлось */
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
  /* если мы здесь, то Wanted = 0 ( изначально ) и
     мы имеем кол-во мест найденных слева, справа ... */
  int EndWanted = 0; /* признак того, что надо будет удалить одно лишнее место из найденных */
  if ( Step == sRight && foundCount == MAXPLACE() &&
       counters.p_Count_3() == Passengers.counters.p_Count_3() &&
       counters.p_Count_2() == Passengers.counters.p_Count_2() &&
       counters.p_Count() == Passengers.counters.p_Count() - MAXPLACE() - 1 ) {
    /* надо попробовать посадить на следующий ряд не одного чел-ка а 2-х */
    EP = FP;
    EP.y++;
    if ( EP.y >= placeList->GetYsCount() || !SeatSubGrp_On( EP, Step, 2 ) ) {
      SeatsStat.stop(__FUNCTION__);
      return false; /* не смогли посадить 2-х на следующий ряд */
    }
    else {
      EndWanted = seatplaces.size(); /* нашли 2 места на следующем ряду и положили их в FPlaces */
      /* обманем наши счетчики на одно место */
      counters.Add_p_Count( -1 );
    }
  }
  /* начинаем запоминать полученные места */
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
  if ( EndWanted > 0 ) { /* надо удалить одно самое плохое место: */
    /* самое удаленное от после	днего найденного в текущем ряду */
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
    /* нашли самое удаленное место и сейчас удалим его */
    RollBack( EndWanted, EndWanted );
    counters.Add_p_Count( 1 );
    SeatsStat.stop(__FUNCTION__);
    return true; /* все нашли выходим */
  }
  /* теперь обсудим следующий вариант: в текущем ряду нашли всего одно место.
     пусть такое допустимо только однажды */
  if ( foundCount == 1 && CanUseAlone != uTrue ) { /*нельзя оставлять одного*/
    if ( Alone ) {
      Alone = false;
    }
    else {  /* нельзя 2 раза чтобы появлялось одно место в ряду. */
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
  /* переходим на следующий ряд ???canUseOneRow??? */
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
         !SeatSubGrp_On( EP, Step, 0 ) ) { // ничего не смогли найти дальше
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
           !SeatSubGrp_On( EP, Step, 0 ) ) { // ничего не смогли найти дальше
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
  /* проверка а осталась ли часть группы, или группа состоит из одного человека ? */
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
  /* надо рассадить оставшуюся часть пассажиров */
  /* для этого надо обойти выбранные места вокруг и попробовать рассадить
    оставшуюся группу рядом с уже рассаженной */
  VSeatPlaces sp( seatplaces.begin(), seatplaces.end() );
  if ( Where != sUpDown )
  for (ISeatPlace isp = sp.begin(); isp!= sp.end(); isp++ ) {
    CurrSalon->SetCurrPlaceList( isp->placeList );
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      /* поищем справа от найденного места */
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
      Alone = true; /* можно найти одно место */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
       }
      SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
      SALONS2::TPoint p( EP.x + 1, EP.y );
      if ( CanUseTube && placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
       /* можно попробовать искать через проход */
       Alone = true; /* можно найти одно место */
       if ( SeatSubGrp_On( p, sRight, 0 ) ) {
         SeatsStat.stop(__FUNCTION__);
         return true;
       }
      }
      /* поищем слева от найденного места */
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
      Alone = true; /* можно найти одно место */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      placeList = CurrSalon->CurrPlaceList();
      p.x = EP.x - 1;
      p.y = EP.y;
      if ( CanUseTube &&
           placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
        /* можно попробовать искать через проход */
        Alone = true; /* можно найти одно место */
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
    /* поищем сверху и снизу от найденного места */
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
      /* рассадка сверху от занятых мест */
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
      /* теперь посмотрим снизу */
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
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* кол-во мест должно совпадать */
      continue;
    ispPlaceName_lat.clear();
    ispPlaceName_rus.clear();
    SALONS2::TPlaceList *placeList = isp->placeList;
    ispPlaceName_lat = denorm_iata_row( placeList->GetYsName( isp->Pos.y ) );
    ispPlaceName_rus = ispPlaceName_lat;
    ispPlaceName_lat += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 1 );
    ispPlaceName_rus += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 0 );
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    bool pr_valid_place = true;
    vector<string> vrems;
    pass.get_remarks( vrems );
    for (vector<string>::iterator irem=vrems.begin(); irem!= vrems.end(); irem++ ) { // пробег по ремаркам пассажира
      for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
        /* пробег по местам которые может занимать пассажир */
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
        if ( itr == ipl->rems.end() ) /* не нашли ремарку у места */
          EqualQ += PR_REM_TO_NOREM;
      } /* конец пробега по местам */
    } /* конец пробега по ремаркам пассажира */

    // тип места
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
    if ( pass.pers_type != "ВЗ" ) {
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
      throw EXCEPTIONS::Exception( "Недопустимое значение направления рассадки" );
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

/* рассадка всей группы начиная с позиции FP */
bool TSeatPlaces::SeatsGrp_On( SALONS2::TPoint FP  )
{
  SeatsStat.start(__FUNCTION__);
//  ProgTrace( TRACE5, "FP(x=%d, y=%d)", FP.x, FP.y );
  /* очистить помеченные места */
  RollBack( );
  /* если есть пассажиры в группе с вертикальной рассадкой, то пробуем их рассадить */
  if ( Passengers.counters.p_Count_3( sDown ) + Passengers.counters.p_Count_2( sDown ) > 0 ) {
    if ( !SeatSubGrp_On( FP, sDown, 0 ) ) { /* не получается */
      RollBack( );
      SeatsStat.stop(__FUNCTION__);
      return false;
    }
    /* если нет других пассажиров, то тогда рассадка выполнена успешно */
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
  /* если мы здесь то смогли рассадить часть группы
     рассаживаем оставшуюся часть группы */
  /* можно попытаться посадить и сверху и снизу от занятых мест */
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
      /* конвертация номеров мест пассажиров в зависимости от лат. или рус. салона */
      for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
            iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
        SALONS2::TPoint FP;
        if ( (*iplaceList)->GetisPlaceXY( placeName, FP ) ) {
          CurrSalon->SetCurrPlaceList( *iplaceList );
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

/* поиск в одном салоне и заданных глобальных переменных.
   изменяется лишь CanUseRems и PlaceRemark */
bool TSeatPlaces::SeatGrpOnBasePlace( )
{
  SeatsStat.start(__FUNCTION__);
//  ProgTrace( TRACE5, "SeatGrpOnBasePlace( )" );
  int G3 = Passengers.counters.p_Count_3( sRight );
  int G2 = Passengers.counters.p_Count_2( sRight );
  int G = Passengers.counters.p_Count( sRight );
  int V3 = Passengers.counters.p_Count_3( sDown );
  int V2 = Passengers.counters.p_Count_2( sDown );
  TUseRem OldCanUseRems = CanUseRems;
  vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
  try {
    /* поиск исходного места для пассажира */
    int lp = Passengers.getCount();
    for ( int i=0; i<lp; i++ ) {
      /* выделяем из группы главного пассажира и находим для него место */
      TPassenger &pass = Passengers.Get( i );
//      ProgTrace( TRACE5, "pass %s", pass.toString().c_str() );
      Passengers.SetCountersForPass( pass );
      getRemarks( pass );
      if ( !pass.preseat_no.empty() &&
           SeatsPassenger_OnBasePlace( pass.preseat_no, pass.Step ) && /* нашли базовое место */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) ) {
  //      tst();
        SeatsStat.stop(__FUNCTION__);
        return true;
      }
      RollBack( );
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* есть ремарка */
        SeatsStat.start("TSeatPlaces::SeatGrpOnBasePlace( ) !Remarks.empty()");
        // попытаемся найти по ремарке
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) {
          /* варианты поиска возле найденного места */
          for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
            CurrSalon->SetCurrPlaceList( icoord->placeList );
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
                RollBack( ); /* не получилось откат занятых мест */
                Passengers.SetCountersForPass( pass ); /* выделяем опять этого пассажира */
              }
            }
          }
        }
        SeatsStat.stop("TSeatPlaces::SeatGrpOnBasePlace( ) !Remarks.empty()");
      } /* конец поиска по ремарке */
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

/* рассадка группы по всем салонам */
bool TSeatPlaces::SeatsGrp( )
{
  SeatsStat.start(__FUNCTION__);
  RollBack( );
  vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
  for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
    CurrSalon->SetCurrPlaceList( icoord->placeList );
    for ( map<int,TSeatCoords>::iterator ivariant=icoord->coords.begin(); ivariant!=icoord->coords.end(); ivariant++ ) {
      for ( vector<TPoint>::iterator ic=ivariant->second.begin(); ic!=ivariant->second.end(); ic++ ) {
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

/* рассадка пассажиров по местам не учитывая группу */
bool TSeatPlaces::SeatsPassengers( bool pr_autoreseats )
{
  SeatsStat.start("TSeatPlaces::SeatsPassengers");
  bool OLDFindSUBCLS = FindSUBCLS;
  bool OLDcanUseSUBCLS = canUseSUBCLS;
  string OLDSUBCLS_REM = SUBCLS_REM;
  vector<TPassenger> npass;
  Passengers.copyTo( npass );
  string OLDclname = Passengers.clname;
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
    for ( int FCanUseINFT=AllowedAttrsSeat.pr_isWorkINFT; FCanUseINFT>=0; FCanUseINFT-- ) {
      for ( int ik=0; ik<=AllowedAttrsSeat.pr_isWorkINFT*2; ik++ )   { //0 -группа с детьми, 1-группа без детей, 2-группа без детей без учета мест
        for ( int FCanUseElem_Type=1/*pr_autoreseats*/; FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // вслучае автомат. рассадки 2 прохода: с учетом типа места и без учета типа места
          for ( /*int i=(int)!pr_autoreseats; i<=1+(int)pr_autoreseats; i++*/ int i=0; i<=2; i++ ) {
            for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
              /* когда пассажир посажен или рассадка на бронь и у пассажира статус не бронь или нет предвар. рассадки или у пассажира указано не то место */
              if ( ipass->InUse )
                pr_seat = true;
              if ( ipass->InUse )
                continue;
              if ( SALONS2::isUserProtectLayer( PlaceLayer ) && !CanUseLayer( PlaceLayer, UseLayers ) &&
                   ( ipass->preseat_layer != PlaceLayer ) ) {
                continue;
              }
              if (!( (ik == 0 && ((!AllowedAttrsSeat.pr_isWorkINFT) || (FCanUseINFT == 1 && ipass->countPlace > 0 && ipass->isRemark( "INFT" )))) ||
                     (ik == 1 && FCanUseINFT == 1 && !(ipass->countPlace > 0 && ipass->isRemark( "INFT" ))) ||
                     (ik == 2 && FCanUseINFT == 0 && !(ipass->countPlace > 0 && ipass->isRemark( "INFT" ))) )) {
                continue;
              }
//              ProgTrace( TRACE5, "pax_id=%d,ik=%d, FCanUseINFT=%d, ipass->countPlace=%d, pass.INFT=%d", ipass->paxId,ik, FCanUseINFT, ipass->countPlace, ipass->isRemark( "INFT" ) );
              /*???31.03.11        if ( ipass->InUse || PlaceLayer == cltProtCkin && !CanUseLayer( cltProtCkin, UseLayers ) && //!!!
                               ( ipass->layer != PlaceLayer || ipass->preseat.empty() || ipass->preseat != ipass->placeName ) )
          continue;*/
              Passengers.Clear();
              ipass->placeList = NULL;
              ipass->seat_no.clear();
              int old_index = ipass->index;

              AllowedAttrsSeat.clearAll();
              if ( FCanUseElem_Type == 1 && ipass->countPlace > 0 && ipass->pers_type != "ВЗ" ) {
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
              else { // первый проход: если используется поиск по подклассу, то сажаем всех пассажиров с нужным подклассом
                if ( i == 2 || ( i == 0 && canUseSUBCLS && ipass->SUBCLS_REM != SUBCLS_REM ) )
                  continue; // сейчас идет поиск по подклассам, а у пассажира его нет - не пытаемся его рассадить
                if ( i == 1 ) { // сейчас идет поиск мест для пассажиров у которых нет подкласса
/*                                                ProgTrace( TRACE5, "ipass->SUBCLS_REM=%s, SUBCLS_REM=%s, canUseSUBCLS=%d",
                           ipass->SUBCLS_REM.c_str(), SUBCLS_REM.c_str(), canUseSUBCLS );*/
                  if ( canUseSUBCLS && !pr_seat )
                    continue;
                  FindSUBCLS = false;
                  //       		  	canUseSUBCLS = false;
                }
              }
//              ProgTrace( TRACE5, "pax_id=%d,ik=%d, FCanUseINFT=%d, ipass->countPlace=%d, pass.INFT=%d, FCanUseElem_Type=%d, i=%d",
//                         ipass->paxId,ik, FCanUseINFT, ipass->countPlace, ipass->isRemark( "INFT" ), FCanUseElem_Type, i );

              Passengers.Add( *ipass );
              ipass->index = old_index;
//              ProgTrace( TRACE5, "Passengers.Count=%d - go seats", Passengers.getCount() );
              if ( SeatGrpOnBasePlace( ) ||
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
                     SeatsGrp( ) ) ) { // тогда можно находить место по всему салону
                if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
                  throw EXCEPTIONS::Exception( "Недопустимое значение направления рассадки" );
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
                grp_status = l;//??? надо так делать, но не красиво
                separately_seats_adults_crs_pax_id = vseparately_seats_adults_crs_pax_id;
                if ( !pr_autoreseats && i == 0 && canUseSUBCLS ) {
                  pr_seat = true;
                }
              }
            } // for passengers
          } // end for i=0..2
        } // end for FCanUseElem_Type
      } // for ik - подгруппы
    } // end for FCanUseINFT
  }
  catch( ... ) {
    FindSUBCLS = OLDFindSUBCLS;
    canUseSUBCLS = OLDcanUseSUBCLS;
    SUBCLS_REM = OLDSUBCLS_REM;
    AllowedAttrsSeat.pr_isWorkINFT = isWorkINFT;
    Passengers.Clear();
    Passengers.copyFrom( npass );
    Passengers.clname = OLDclname;
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
  Passengers.clname = OLDclname;
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

void TPassenger::add_rem( std::string code )
{
    if ( isREM_SUBCLS( code ) )
        SUBCLS_REM = code;
    rems.push_back( code );
}

void TPassenger::calc_priority( std::map<std::string, int> &remarks )
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
  for ( std::vector<string>::iterator irem = rems.begin(); irem != rems.end(); irem++ ) {
    if ( remarks[ *irem ] > vpriority ) {
      vpriority = remarks[ *irem ];
      maxRem = *irem;
    }
    priority += remarks[ *irem ]*countPlace; //???
  }
  //???  priority += priority*countPlace;
}

void TPassenger::get_remarks( std::vector<std::string> &vrems )
{
    vrems.clear();
  for (std::vector<string>::iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
    vrems.push_back( *irem );
  }
}

bool TPassenger::isRemark( std::string code )
{
  for (std::vector<string>::iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
    if ( *irem == code )
        return true;
  }
    return false;
}

bool TPassenger::is_valid_seats( const std::vector<SALONS2::TPlace> &places )
{
  for ( std::vector<string>::iterator irem=rems.begin(); irem!= rems.end(); irem++ ) {
    for( std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
     /* пробег по местам которые может занимать пассажир */
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

void TPassenger::build( xmlNodePtr pNode, const TDefaults& def )
{
  NewTextChild( pNode, "grp_id", grpId );
  NewTextChild( pNode, "pax_id", paxId );
  NewTextChild( pNode, "clname", clname, def.clname );
  NewTextChild( pNode, "grp_layer_type",
                       EncodeCompLayerType(grp_status),
                       EncodeCompLayerType(def.grp_status) );
  NewTextChild( pNode, "pers_type",
                       ElemIdToCodeNative(etPersType, pers_type),
                       ElemIdToCodeNative(etPersType, def.pers_type) );
  NewTextChild( pNode, "reg_no", regNo );
  NewTextChild( pNode, "name", fullName );
  NewTextChild( pNode, "seat_no", foundSeats, def.placeName );
  NewTextChild( pNode, "wl_type", wl_type, def.wl_type );
  NewTextChild( pNode, "seats", countPlace, def.countPlace );
  NewTextChild( pNode, "tid", tid );
  NewTextChild( pNode, "isseat", (int)isSeat, (int)def.isSeat );
  NewTextChild( pNode, "ticket_no", ticket_no, def.ticket_no );
  NewTextChild( pNode, "document", document, def.document );
  NewTextChild( pNode, "bag_weight", bag_weight, def.bag_weight );
  NewTextChild( pNode, "bag_amount", bag_amount, def.bag_amount );
  NewTextChild( pNode, "excess", excess, def.excess );
  NewTextChild( pNode, "trip_from", trip_from, def.trip_from );

  string comp_rem;
  bool pr_down = false;
  if ( !rems.empty() ) {
    for ( vector<string>::iterator r=rems.begin(); r!=rems.end(); r++ ) {
        comp_rem += *r + " ";
        if ( *r == "STCR" )
            pr_down = true;
    };
  };
  NewTextChild( pNode, "comp_rem", comp_rem, def.comp_rem );
  NewTextChild( pNode, "pr_down", (int)pr_down, (int)def.pr_down );
  NewTextChild( pNode, "pass_rem", pass_rem, def.pass_rem );
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
  clname.clear();
  this->UseSmoke = false;
  counters.Clear();
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
 // высчитываем класс
  if ( clname.empty() && !pass.clname.empty() )
    clname = pass.clname;
 // высчитываем приоритет
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
   throw EXCEPTIONS::Exception( "Не допустимое кол-во мест для расадки" );
  pass.preseatPlaces.clear();

  size_t i = 0;
  for (; i < pass.preseat_no.size(); i++)
    if ( pass.preseat_no[ i ] != '0' )
      break;
  if ( i )
    pass.preseat_no.erase( 0, i );
  pass.preseat_layer = cltUnknown; // означает, что агент назначил место, или это место от какого-нибудь слоя - надо проверять
  pass.preseat_pax_id = 0;
  // определение самого приоритетного слоя и номера места для пассажира
  if ( pass.grp_status == cltTranzit ) {
     pass.preseat_layer = cltProtTrzt;
  }
  else { // определение приоритетного слоя для пассажира
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
   throw EXCEPTIONS::Exception( "Недопустимое кол-во мест для расадки" );
  pass.preseatPlaces.clear();

  size_t i = 0;
  for (; i < pass.preseat_no.size(); i++)
    if ( pass.preseat_no[ i ] != '0' )
      break;
  if ( i )
    pass.preseat_no.erase( 0, i );
  pass.preseat_layer = cltUnknown; // означает, что агент назначил место, или это место от какого-нибудь слоя - надо проверять
  pass.preseat_pax_id = 0;
  // определение самого приоритетного слоя и номера места для пассажира
  if ( pass.grp_status == cltTranzit ) {
       pass.preseat_layer = cltProtTrzt;
  }
  else { // определение приоритетного слоя для пассажира
    std::map<int,TPaxList>::const_iterator ipaxlist = salonList.pax_lists.find( salonList.getDepartureId() );
    if ( ipaxlist != salonList.pax_lists.end() ) { // нашли пункт вылета
      std::map<int,TSalonPax>::const_iterator ipax = ipaxlist->second.find( pass.paxId );
      if ( ipax != ipaxlist->second.end() ) {
        tst();
        for ( std::map<TSeatLayer,TPaxLayerSeats,SeatLayerCompare>::const_iterator ilayer=ipax->second.layers.begin();
              ilayer!=ipax->second.layers.end(); ilayer++ ) {
          if ( ilayer->second.waitListReason.layerStatus == layerValid ) {
            //т.к. есть сортировка мест, то выдаем первое
            pass.preseat_layer = ilayer->first.layer_type;
            pass.preseat_pax_id = pass.paxId;
            std::set<TPlace*,CompareSeats>::const_iterator iseat = ilayer->second.seats.begin();
            pass.preseat_no = (*iseat)->denorm_view(salonList.isCraftLat());
            TCoordSeat coord;
            coord.placeListIdx = -1;
            for ( std::vector<TPlaceList*>::const_iterator plList=salonList.begin();
                  plList!=salonList.end(); plList++ ) {
              TPlaceList* placeList = *plList;
              TPoint p;
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
  /* если мы здесь то тогда мы смогли посадить главного чел-ка из группы
     попробуем посадить всех остальных */
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
  //не работаем с курящими местами
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

bool ExistsBasePlace( SALONS2::TSalons &Salons, TPassenger &pass )
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
             !Salons.placeIsFree( place ) || pass.clname != place->clname ||
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
          (*ipl)->AddLayerToPlace( pass.grp_status, 0, pass.paxId, NoExists, NoExists, Salons.getPriority( pass.grp_status ) );
          if ( CurrSalon->canAddOccupy( *ipl ) ) {
            CurrSalon->AddOccupySeat( placeList->num, (*ipl)->x, (*ipl)->y );
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
                          TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                          TClientType client_type,
                          TPassengers &passengers,
                          const std::map<int,TPaxList> &pax_lists );

/* рассадка пассажиров */
void SeatsPassengers( SALONS2::TSalonList &salonList,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                      TClientType client_type,
                      TPassengers &passes,
                      SALONS2::TAutoSeats &autoSeats )
{
  SeatsStat.clear();
  SeatsStat.deactivate();
  ProgTrace( TRACE5, "salonList NEWSEATS, ASeatAlgoParams=%d", (int)ASeatAlgoParams.SeatAlgoType );
  if ( !passes.getCount() )
    return;
  SeatsStat.start("SeatsPassengers(SalonList)");
  /* надо подготовить переменную CurrSalon на основе salonList */
  TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
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
    CurrSalon = &SalonsN;
    countP++;
    if ( countP == 20 ) {  //маленькая защитка
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
      SeatsPassengersGrps( CurrSalon,
                           ASeatAlgoParams,
                           client_type,
                           passes,
                           salonList.pax_lists );
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
          seat.ranges.push_back( TSeat( (*iseat)->yname, (*iseat)->xname ) );
          ProgTrace( TRACE5, "autoSeats.push_back pax_id=%d, seats=%d, yname=%s, xname=%s",
                     seat.pax_id, seat.seats, (*iseat)->yname.c_str(), (*iseat)->xname.c_str() );
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
    catch( UserException ue ) {
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


void dividePassengersToGrps( TPassengers &passengers, vector<TPassengers> &passGrps, bool separately_seat_adult_with_baby )
{
  SeatsStat.start(__FUNCTION__);
  passGrps.clear();
  TPassengers p;
  boolean ignoreINFT = !AllowedAttrsSeat.pr_isWorkINFT;
  //std::set<int> addedPasses;
  //for ( int icond=0; icond<4; icond++ ) { // AllowedAttrsSeat.pr_isWorkINFT делим на: 0=платный ребенок и 1=платный взрослый , 2=бесплатный ребеной, 3=бесплатный взрослый
    std::map<std::string,TPassengers> v; //делим по тарифам
    for ( int i=0; i<passengers.getCount(); i++ ) {
      TPassenger &pass = passengers.Get( i );
      bool pr_pay = false;
      if ( SALONS2::isUserProtectLayer( pass.preseat_layer ) ) {
        if ( pass.preseat_layer == cltProtBeforePay ||
             pass.preseat_layer == cltPNLBeforePay ||
             pass.preseat_layer == cltProtAfterPay ||
             pass.preseat_layer == cltPNLAfterPay )
          pr_pay = true;
      }
      //взрослые и взрослые с младенцами
      ostringstream grp_variant;
      if ( separately_seat_adult_with_baby &&
           pass.isRemark( "INFT" ) ) { //отдельно каждого пассажира
        grp_variant << "INFT" << setw(3) << pass.index;
      }
      else {
        grp_variant << "ZZZZ" << "999";
      }
      //дети и оплата
      grp_variant << pr_pay << (ignoreINFT || pass.isRemark( "INFT" ));
      //тариф
      grp_variant << pass.tariffs.key() << EncodeCompLayerType(pass.preseat_layer) << pass.dont_check_payment;
//      ProgTrace( TRACE5, "grp_variant=%s, pax=%s", grp_variant.str().c_str(), pass.toString().c_str() );
      v[ grp_variant.str() ].Add( pass, pass.index );
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
    int grp_idx = 0;
    for ( std::map<std::string,TPassengers>::iterator ip = v.begin(); ip!=v.end(); ip++ ) {
      if ( ip->second.getCount() > 0 ) {
        passGrps.push_back( ip->second );
/*        ProgTrace( TRACE5, "grp_num=%d, variant=%s, count=%d", grp_idx, ip->first.c_str(), ip->second.getCount() );
        for ( int jk=0; jk<ip->second.getCount(); jk++ ) {
          ProgTrace( TRACE5, "%s", ip->second.Get( jk ).toString().c_str() );
        }
        ProgTrace( TRACE5, "=================================================" );*/
        grp_idx++;
      }
    }
  //} //cond
  if ( passengers.getCount() > 0 ) {
//    ProgTrace( TRACE5, "add all grp: passGrps.push_back( %d )", passengers.getCount() );
    p = passengers;
    passGrps.push_back( p ); //last element contain prior passengers variant
    ProgTrace( TRACE5, "all grp, count=%d", p.getCount() );
/*    for ( int jk=0; jk<p.getCount(); jk++ ) {
      ProgTrace( TRACE5, "%s", p.Get( jk ).toString().c_str() );
    }
    ProgTrace( TRACE5, "=================================================" );*/
  }
  SeatsStat.stop(__FUNCTION__);
}

void SeatsPassengers( SALONS2::TSalons *Salons,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                      TClientType client_type,
                      bool pr_rollback,
                      bool separately_seats_adult_with_baby,
                      TPassengers &passengers,
                      const std::vector<TCoordSeat> &paxsSeats );

void SeatsPassengersGrps( SALONS2::TSalons *Salons,
                          TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                          TClientType client_type,
                          TPassengers &passengers,
                          const std::map<int,TPaxList> &pax_lists )
{
  if ( !passengers.getCount() )
    return;
  SeatsStat.start(__FUNCTION__);
  //разобъем пассажиров по группам
  std::vector<TPassengers> passGrps;
  std::set<int> pax_lists_with_baby;
  if ( Salons->trip_id == ASTRA::NoExists ) {
    ProgError( STDLOG, "SeatsPassengersGrps: point_id = NoExists!!!" );
  }
  std::map<int,TPaxList>::const_iterator ipaxs = pax_lists.find( Salons->trip_id );
  if ( ipaxs != pax_lists.end() ) {
    ipaxs->second.setPaxWithInfant( pax_lists_with_baby );
  }
  AllowedAttrsSeat.isWorkINFT( Salons->trip_id ); //нужно для инициализации переменной pr_INFT
  TZonesBetweenLines zonesBetweenLines( CurrSalon->trip_id );
  dividePassengersToGrps( passengers, passGrps, zonesBetweenLines.useInfantSection() );
  // passengers - скорее всего это глобальная переменная, надо ее запомнить, использовать, а потом восстановить
  //в последнем элементе вектора - вся группа до разбивки
  std::vector<TCoordSeat> paxsSeats;
  VSeatPlaces seatsGrps;
  for ( std::vector<TPassengers>::iterator ipassGrp=passGrps.begin(); ipassGrp!=passGrps.end(); ) {
    passengers = *ipassGrp;
    zonesBetweenLines.clear();
    bool separately_seats_adult_with_baby = zonesBetweenLines.useInfantSection() && passengers.withBaby();
    try {
      bool pr_rollback = ( !passGrps.empty() && ipassGrp == passGrps.end() - 1 ); //вся группа
      ProgTrace( TRACE5, "pr_rollback=%d, separately_seats_adult_with_baby=%d, passengers.getCount=%d", pr_rollback, separately_seats_adult_with_baby, passengers.getCount() );
      if ( separately_seats_adult_with_baby ) {
        TPaxsCover paxs;
        for ( int i=0; i<passengers.getCount(); i++ ) {
          ProgTrace( TRACE5, "pax(%i).pax_id=%d", i, passengers.Get(i).paxId );
          paxs.push_back( TPaxCover( passengers.Get(i).paxId, ASTRA::NoExists ) );
        }
        zonesBetweenLines.setDisabled( Salons, paxs, pax_lists_with_baby );
      }
      SeatsPassengers( Salons,
                       ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                       client_type,
                       pr_rollback,
                       separately_seats_adult_with_baby,
                       passengers,
                       paxsSeats );
      zonesBetweenLines.rollbackDisabled( Salons );
      //указать, что найденные места принадлежат пассажирам с детьми!!!
      ipassGrp++;
      SeatPlaces >> seatsGrps; //сохраняем места
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
          if ( ipassGrp == jpassGrp ) { // закончили, возвращаем всю входную группу в переменную passengers
            passengers = *jpassGrp;
            tst();
            break;
          }
        }
        if ( ipassGrp == passGrps.end() - 1 ) {
          tst();
          break;
        }
      }
    }
    catch(...) {
      if ( separately_seats_adult_with_baby ) {
        zonesBetweenLines.rollbackDisabled( Salons );
        SeatsStat.stop(__FUNCTION__);
        throw;
      }
      if ( !passGrps.empty() && ipassGrp == passGrps.end() - 1 ) {
        SeatsStat.stop(__FUNCTION__);
        throw;
      }
      //подгруппа
      if ( !passGrps.empty() ) {
        ipassGrp = passGrps.end() - 1; //группа
      }
      SeatPlaces = seatsGrps; //достаем места
      SeatPlaces.RollBack(); //откатываем
      paxsSeats.clear();
      continue;
    }
  }
//  ProgTrace( TRACE5, "seatsGrps.size()=%zu", seatsGrps.size() );
  SeatPlaces = seatsGrps; //достаем места
  SeatPlaces.RollBack(); //откатываем
  paxsSeats.clear();
  passengers.sortByIndex();
  SeatsStat.stop(__FUNCTION__);
  tst();
}


bool UsedPayedPreseatForPassenger( const TPlace &seat, int pass_preseat_pax_id, TCompLayerType pass_preseat_layer ) {
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
                 seat.layers.begin()->pax_id, pass_preseat_pax_id );
      return seat.layers.begin()->pax_id == pass_preseat_pax_id; //принадлежит пассажиру
    }
  }
  //у пассажира разметка с меньшим приоритетом чем платное
  BASIC_SALONS::TCompLayerTypes *compTypes = BASIC_SALONS::TCompLayerTypes::Instance();
  int priority = compTypes->priority( pass_preseat_layer );
  if ( priority > compTypes->priority( cltProtBeforePay )&&
       priority > compTypes->priority( cltProtAfterPay )&&
       priority > compTypes->priority( cltPNLBeforePay )&&
       priority > compTypes->priority( cltPNLAfterPay ) &&
       priority > compTypes->priority( cltProtSelfCkin )) {
    tst();
    return false;
  }
  tst();
  return true;
}


class AnomalisticConditionsPayment
{
  public:
    static void clearPreseatPaymentLayers( SALONS2::TSalons *Salons, TPassengers &passengers ) {
      //удаляем предварительно назначенное платное место
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
              if ( pass.preseat_layer == cltProtBeforePay || // не оплатили - удаляем из компоновки разметку слоем и пассажира делаем обычным
                   pass.preseat_layer == cltPNLBeforePay ||
                   (pass.preseat_layer == cltProtSelfCkin && !p->SeatTariff.empty()) || //резерв, но не оплачен
                   !UsedPayedPreseatForPassenger( *p, pass.preseat_pax_id, pass.preseat_layer ) ) { //очистка предварительно назначенных мест
                prClear = true;
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
              TSeatLayer layer = place->getCurrLayer( Salons->trip_id );
              if ( //place->isLayer( cltProtAfterPay, pass.paxId ) ) {
                   (layer.layer_type == cltProtAfterPay ||
                    layer.layer_type == cltProtSelfCkin) &&
                   layer.crs_pax_id == pass.paxId ) {
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
    static void setPayementOnWebSignal( SALONS2::TSalons *Salons, TPassengers &passengers ) {
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
              place->AddLayerToPlace( cltProtAfterPay, NowUTC(), pass.paxId, Salons->trip_id, pass.point_arv, BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltProtAfterPay ) );
              if ( CurrSalon->canAddOccupy( place ) ) {
                CurrSalon->AddOccupySeat( placeList->num, place->x, place->y );
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

/*    static void clearTariffsOnWebSignal( SALONS2::TSalons *Salons, TPassengers &passengers ) {
      //очищаем тариф места
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


/* рассадка пассажиров */
void SeatsPassengers( SALONS2::TSalons *Salons,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - умолчание */,
                      TClientType client_type,
                      bool pr_rollback,
                      bool separately_seats_adult_with_baby,
                      TPassengers &passengers,
                      const std::vector<TCoordSeat> &paxsSeats )
{
  ProgTrace( TRACE5, "NEWSEATS, ASeatAlgoParams=%d, Salons->placelists.size()=%zu, passengers.getCount()=%d",
            (int)ASeatAlgoParams.SeatAlgoType, Salons->placelists.size(), passengers.getCount() );
  if ( !passengers.getCount() ) {
    return;
  }
  SeatsStat.start("SeatsPassengers(TSalons)");
  //для всей группы одна разметка тарифом
  ProgTrace( TRACE5, "passengers.Get(0).tariffs=%s, tariffStatus=%d", passengers.Get(0).tariffs.key().c_str(), passengers.Get(0).tariffStatus );
  if ( passengers.Get(0).tariffStatus != TSeatTariffMap::stNotRFISC ) {
    Salons->SetTariffsByRFISCColor( Salons->trip_id, passengers.Get(0).tariffs, passengers.Get(0).tariffStatus );
  }
  AnomalisticConditionsPayment::clearPreseatPaymentLayers( Salons, passengers );
  //AnomalisticConditionsPayment::clearTariffsOnWebSignal( Salons, passengers );
  AnomalisticConditionsPayment::setPayementOnWebSignal( Salons, passengers );
  AnomalisticConditionsPayment::removeRemarksOnPaymentLayer( Salons, passengers );

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
  CurrSalon = Salons;
  CanUseLayers = true;
  CanUseSmoke = false; /* пока не будем работать с курящими местами */
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
                            separately_seats_adult_with_baby?paxsSeatsEmpty:paxsSeats );
  SeatAlg = sSeatGrpOnBasePlace;

  FindSUBCLS = false;
  canUseSUBCLS = false;
  SUBCLS_REM = "";
  CanUseMutiLayer = false;
  bool prElemTypes = false;
  bool prINFT = false;
  bool isWorkINFT = AllowedAttrsSeat.isWorkINFT( Salons->trip_id );

  /* не сделано!!! если у всех пассажиров есть места, то тогда рассадка по местам, без учета группы */

  /* определение есть ли в группе пассажир с предварительной рассадкой */
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
    if ( pass.countPlace > 0 && pass.pers_type != "ВЗ"  ) {
      prElemTypes = true;
    }
    if ( pass.countPlace > 0 && pass.isRemark( "INFT" ) ) {
      prINFT = true;
    }
    if ( !pass.SUBCLS_REM.empty() && !Salons->isExistSubcls( pass.SUBCLS_REM ) ) {
      pass.SUBCLS_REM.clear();
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
  condRates.Init( *Salons, pr_pay, TReqInfo::Instance()->client_type ); // собирает все типы платных мест в массив по приоритетам, если это web-клиент, то не учитываем

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
     // ??? что важнее для пассажиров с детьми: сохранить группу или сесть раздельно, но на разрешенные места ???
     // пока разбиваем
     for ( int FCanUseElem_Type=prElemTypes; FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // поиск по типам мест + игнорирование типа мест
       AllowedAttrsSeat.clearElems();
       if ( FCanUseElem_Type == 1 ) {
         AllowedAttrsSeat.getValidChildElem_Types();
       }
       for ( int FCanINFT=(!prINFT && AllowedAttrsSeat.pr_isWorkINFT); FCanINFT>=0; FCanINFT-- ) { //2 прохода для группы без INFT
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
                         SeatOnlyBasePlace,FCanUserSUBCLS,FCanINFT,prINFT,AllowedAttrsSeat.pr_isWorkINFT); //!!! в этом алгоритме тормозит
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
           /* если есть в группе предварительная рассадка, то тогда сажаем всех отдельно */
           /* если есть в группе подкласс С и он не у всех пассажиров, то тогда сажаем всех отдельно */
         //  ProgTrace( TRACE5, "use_preseat_layer=%d, SeatOnlyBasePlace=%d",use_preseat_layer,SeatOnlyBasePlace);
           if ( ( use_preseat_layer ||
                  Status_seat_no_BR ||
                  SeatOnlyBasePlace || // для каждого пассажира задано свой номер места
                  ( canUseSUBCLS && pr_SUBCLS && !pr_all_pass_SUBCLS ) ) // если есть группа пассажиров среди которых есть пассажиры с подклассом, нл не все
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
                     continue; /*??? что главнее группа или места с ремарками, кот не надо учитывать */
                 }
                 break;
               case sSeatPassengers:
                 if ( use_preseat_layer && CanUseRems != sIgnoreUse ) // игнорируем ремарки в случае предварительной разметки
                   continue;
                 break;
             }
             /* использование платных мест */
             bool ignore_rate = condRates.ignore_rate;
             for ( condRates.current_rate = condRates.rates.begin(); condRates.current_rate != condRates.rates.end(); condRates.current_rate++ ) {
//               ProgTrace( TRACE5, "current_rate=condRates.rates.current_rate=%s", condRates.current_rate->str().c_str() );
               if ( ( condRates.current_rate->rate != 0.0   &&
                      SeatAlg != sSeatPassengers ) ) { //рассадка на платные места только по одному SeatAlg=1
//                 ProgTrace( TRACE5, "condRates.current_rate=%f continue", condRates.current_rate->rate );
                 continue;
               }
               if ( use_preseat_layer && SeatAlg == sSeatPassengers ) { // если предварительно размеченный слой, то игнорируем платные места
                 condRates.ignore_rate = true;
               }
               else {
                 condRates.ignore_rate = ignore_rate;
               }
//               ProgTrace( TRACE5, "condRates.current_rate=%s, condRates.ignore_rate=%d, SeatAlg=%d", condRates.current_rate->str().c_str(), condRates.ignore_rate, SeatAlg );
               //ProgTrace( TRACE5, "SeatAlg == %d, condRates.current_rate.first=%d, condRates.current_rate->second.value=%f",
               //                        SeatAlg, condRates.current_rate->first, condRates.current_rate->second.value );
               /* использование статусов мест */
               //надо учитывать случай, когда остались места только со слоями cltProtBeforePay, cltPNLBeforePay, cltProtAfterPay, cltPNLAfterPay - надо уметь
               // вначале надо попробовать использовать только места со слоями cltProtBeforePay, cltPNLBeforePay, а потом + cltProtAfterPay, cltPNLAfterPay
               for ( int KeyLayers=1; KeyLayers>=-3; KeyLayers-- ) {
                 if ( ( !KeyLayers && ( SeatAlg == sSeatGrpOnBasePlace || SeatAlg == sSeatGrp ) ) ||
                      ( KeyLayers < -1 && ( SeatAlg != sSeatPassengers || !condRates.isIgnoreRates( ) ) ) ) { // если можно использовать платные слои, то только при рассадке пассажиров по одному
                   continue;
                 }
                 curr_preseat_layers.clear();
                 curr_preseat_layers.insert( preseat_layers.begin(), preseat_layers.end() );
//                 ProgTrace(TRACE5, "SeatOnlyBasePlace=%d, KeyLayers=%d", SeatOnlyBasePlace, KeyLayers );
                 if ( SeatOnlyBasePlace && KeyLayers <= -2 ) { //если указано место, которое оплатил другой, то нельзя его забирать
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
                   curr_preseat_layers[ cltPNLAfterPay ] = true;
                   curr_preseat_layers[ cltProtSelfCkin ] = true;
                 }
                 /* задаем массив статусов мест */
                 SetLayers( Salons,
                            SeatsLayers,
                            CanUseMutiLayer,
                            passengers.Get( 0 ).preseat_layer,
                            KeyLayers,
                            curr_preseat_layers,
                            client_type );
                 /* пробег по статусом */
                 for ( vector<TCompLayerType>::iterator l=SeatsLayers.begin(); l!=SeatsLayers.end(); l++ ) {
                   PlaceLayer = *l;
                   /* оставлять одного несколько раз на рядах при Рассадке группы */
                   for ( int FCanUseAlone=uFalse3; FCanUseAlone<=uTrue; FCanUseAlone++ ) {
                     CanUseAlone = (TUseAlone)FCanUseAlone;
                     if ( CanUseAlone == uFalse3 && passengers.getCount() < 2 ) { //чтобы не делать лишний циклов, т.к. рассадка все равно не пройдет
                       //Один пассажир должен сидеть один в ряду
                       continue;
                     }
                     if ( CanUseAlone == uTrue && CanUseRems == sNotUse_NotUseDenial	 ) {
                       // если пассажиров можно оставлять сколько угодно раз по одному в ряду и можно сажать только на места с нужными ремарками
                       continue;
                     }
                     if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers ) {
                       continue;
                     }
                     if ( !KeyLayers && CanUseAlone == uFalse3 ) {
                       continue;
                     }
                     /* учет режима рассадки в одном ряду */
                     for ( int FCanUseOneRow=getCanUseOneRow(); FCanUseOneRow>=0; FCanUseOneRow-- ) {
                       canUseOneRow = FCanUseOneRow;
                       /* учет проходов */
                       for ( int FCanUseTube=0; FCanUseTube<=1; FCanUseTube++ ) {
                         /* для рассадки отдельных пассажиров не надо учитывать проходы */
                         if ( !FCanUseTube && !paxsSeats.empty() && !separately_seats_adult_with_baby  ) {
                           ProgTrace( TRACE5, "separately_seats_adult_with_baby");
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
                               getRemarks( passengers.Get( 0 ) ); // для самого приоритетного
                               if ( SeatPlaces.SeatsGrp( ) )
                                 throw 1;
                               break;
                             case sSeatPassengers:
                               if ( SeatPlaces.SeatsPassengers() )
                                 throw 1;
                               if ( SeatOnlyBasePlace && pr_SUBCLS && !canUseSUBCLS ) { //перешли на последний алгоритм, когда есть подклассы и указаны места, но реально разметки подклассов нет в салоне
                                 SeatOnlyBasePlace = false;
                                 FSeatAlg=0;
                               }
                               break;
                           } /* end switch SeatAlg */
//                           ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseElem_Type=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyLayers=%d,FCanUseTube=%d,FCanUseSmoke=%d,PlaceLayer=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseSUBCLS=%d, SUBCLS_REM=%s",
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
    /* распределение полученных мест по пассажирам, только для SeatPlaces.SeatGrpOnBasePlace */
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

bool GetPassengersForWaitList( int point_id, TPassengers &p )
{
  bool res = false;
  TQuery Qry( &OraSession );
  TQuery RemsQry( &OraSession );
  TPaxSeats priorSeats( point_id );

  p.Clear();
  RemsQry.SQLText =
    "SELECT rem, rem_code, pax.pax_id, comp_rem_types.pr_comp "
    " FROM pax_rem, pax_grp, pax, comp_rem_types "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.status NOT IN ('E') AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      pax.seats > 0 AND "
    "      pax_rem.pax_id=pax.pax_id AND "
    "      rem_code=comp_rem_types.code(+) "
    " ORDER BY pax.pax_id, pr_comp, code ";
  RemsQry.CreateVariable( "point_id", otInteger, point_id );

  Qry.SQLText =
    "SELECT airline "
    " FROM points "
    " WHERE points.point_id=:point_id AND points.pr_del!=-1 AND points.pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
    throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  string airline = Qry.FieldAsString( "airline" );
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");

  Qry.Clear();
  Qry.SQLText =
    "SELECT pax_grp.grp_id, "
    "       pax.pax_id, "
    "       pax.reg_no, "
    "       surname, "
    "       pax.name, "
    "       pax_grp.class, "
    "       cls_grp.code subclass, "
    "       pax.seats, "
    "       pax.is_jmp, "
    "       pax_grp.status, "
    "       pax.pers_type, "
    "       pax.ticket_no, "
    "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
    "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
    "       ckin.get_excess(pax.grp_id,pax.pax_id) AS excess, "
    "       pax.tid, "
    "       pax.wl_type, "
    "       pax_grp.point_arv, "
    "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,pax_grp.point_dep,'list',rownum) AS seat_no, "
    "       tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no "
    "FROM pax_grp, pax, cls_grp, tckin_pax_grp "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.status NOT IN ('E') AND "
    "      pax_grp.class_grp = cls_grp.id AND "
    "      pax_grp.grp_id=tckin_pax_grp.grp_id(+) AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      pax.seats > 0 ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  TSublsRems subcls_rems(airline);

  RemsQry.Execute();
  TCkinRoute tckin_route;
  while ( !Qry.Eof ) {
    TPassenger pass;
    pass.paxId = Qry.FieldAsInteger( "pax_id" );
    pass.foundSeats = Qry.FieldAsString( "seat_no" );
    pass.clname = Qry.FieldAsString( "class" );
    pass.countPlace = Qry.FieldAsInteger( "seats" );
    pass.is_jmp = Qry.FieldAsInteger( "is_jmp" )!=0;
    pass.tid = Qry.FieldAsInteger( "tid" );
    pass.grpId = Qry.FieldAsInteger( "grp_id" );
    pass.regNo = Qry.FieldAsInteger( "reg_no" );
    string fname = Qry.FieldAsString( "surname" );
    pass.fullName = TrimString( fname ) + " " + Qry.FieldAsString( "name" );
    pass.ticket_no = Qry.FieldAsString( "ticket_no" );
    pass.document = CheckIn::GetPaxDocStr(NoExists, Qry.FieldAsInteger( "pax_id" ), true);
    pass.bag_weight = Qry.FieldAsInteger( "bag_weight" );
    pass.bag_amount = Qry.FieldAsInteger( "bag_amount" );
    pass.excess = Qry.FieldAsInteger( "excess" );
    pass.grp_status = DecodeCompLayerType(((const TGrpStatusTypesRow&)grp_status_types.get_row("code",Qry.FieldAsString( "status" ))).layer_type.c_str());
    pass.pers_type = Qry.FieldAsString( "pers_type" );
    pass.wl_type = Qry.FieldAsString( "wl_type" );
    pass.point_arv = Qry.FieldAsInteger( "point_arv" );
    pass.InUse = ( !pass.foundSeats.empty() );
    pass.isSeat = pass.InUse;
    if ( pass.foundSeats.empty() ) { // ???необходимо выбрать предыдущее место
        res = true;
        string old_seat_no;
            if ( pass.wl_type.empty() ) {
              old_seat_no = priorSeats.getSeats( pass.paxId, "seats" );
              if ( !old_seat_no.empty() )
                old_seat_no = "(" + old_seat_no + ")";
            }
            else
                old_seat_no = AstraLocale::getLocaleText("ЛО");
            if ( !old_seat_no.empty() )
                pass.foundSeats = old_seat_no;
    }
    while ( !RemsQry.Eof && RemsQry.FieldAsInteger( "pax_id" ) <= pass.paxId ) {
        if ( RemsQry.FieldAsInteger( "pax_id" ) == pass.paxId ) {
            pass.add_rem( RemsQry.FieldAsString( "rem_code" ) );
            pass.pass_rem += string( ".R/" ) + RemsQry.FieldAsString( "rem" ) + "   ";
        }
      RemsQry.Next();
    }
    string pass_rem;
    if ( subcls_rems.IsSubClsRem( Qry.FieldAsString( "subclass" ), pass_rem ) )
      pass.add_rem( pass_rem );


    if (!Qry.FieldIsNULL("tckin_id"))
    {
      TCkinRouteItem priorSeg;
      tckin_route.GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                              Qry.FieldAsInteger("seg_no"),
                              crtIgnoreDependent,
                              priorSeg);
      if (priorSeg.grp_id!=NoExists)
      {
        TDateTime local_scd_out = UTCToClient(priorSeg.operFlt.scd_out,AirpTZRegion(priorSeg.operFlt.airp));

          ostringstream trip;
          trip << ElemIdToElemCtxt( ecDisp, etAirline, priorSeg.operFlt.airline, priorSeg.operFlt.airline_fmt )
               << setw(3) << setfill('0') << priorSeg.operFlt.flt_no
               << ElemIdToElemCtxt( ecDisp, etSuffix, priorSeg.operFlt.suffix, priorSeg.operFlt.suffix_fmt )
               << "/" << DateTimeToStr( local_scd_out, "dd" );

          pass.trip_from = trip.str();
      };
    };

    p.Add( pass );
    Qry.Next();
  }
  return res;
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

/*bool getNextSeat( int point_id, TSeatRange &r, int pr_down )
{
    TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT num, x, y, xname, yname FROM trip_comp_elems t, "
    "(SELECT num, x, y, class FROM trip_comp_elems "
    "  WHERE point_id=:point_id AND xname = :xname AND yname = :yname ) a, "
    "( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) e "
    " WHERE t.point_id=:point_id AND "
    " t.num = a.num AND "
    " t.class = a.class AND "
    " t.x = DECODE(:pr_down,0,1,0) + a.x AND "
    " t.y = DECODE(:pr_down,0,0,1) + a.y AND "
    " t.elem_type = e.code ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "xname", otString, r.first.line );
  Qry.CreateVariable( "yname", otString, r.first.row );
  Qry.CreateVariable( "pr_down", otInteger, pr_down );
  Qry.Execute();
  if ( Qry.RowCount() ) {
    strcpy( r.first.line, Qry.FieldAsString( "xname" ) );
    strcpy( r.first.row, Qry.FieldAsString( "yname" ) );
    r.second = r.first;
    return true;
  }
  return false; //!!! салона может и не быть, а разметить надо
} */

bool getCurrSeat( TSalons &ASalons, TSeatRange &r, TSalonPoint &p )
{
    TPoint pt;
    bool res=false;
    for( vector<TPlaceList*>::iterator placeList = ASalons.placelists.begin();placeList != ASalons.placelists.end(); placeList++ ) {
        if ( (*placeList)->GetisPlaceXY( string(r.first.row)+r.first.line, pt ) ) {
            p.num = (*placeList)->num;
            p.x = pt.x;
            p.y = pt.y;
            res=true;
            break;
        }
    }
    return res;
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
      if ( string(PaxQry.FieldAsString( "pers_type" )) == string("ВЗ") ) {
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

/*!!!bool ChangeLayer( TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type,
                  bool pr_lat_seat, TChangeLayerProcFlag seatFlag )
{
  bool changedOrNotPay = true;
  // разметка и проверка возможна только для платных слоев
  if ( seatFlag != clNotPaySeat &&
       ( seat_type != stSeat || ( layer_type != cltProtBeforePay && layer_type != cltProtAfterPay && layer_type != cltProtSelfCkin  ) ) ) {
    tst();
    throw UserException("MSG.SEATS.SEAT_NO.NOT_AVAIL");
  }
  UseLayers[ cltProtCkin ] = false;
  UseLayers[ cltProtBeforePay ] = false;
  UseLayers[ cltPNLBeforePay ] = false;
  UseLayers[ cltProtAfterPay ] = false;
  UseLayers[ cltPNLAfterPay ] = false;
  UseLayers[ cltProtSelfCkin ] = false;
  first_xname = norm_iata_line( first_xname );
  first_yname = norm_iata_line( first_yname );
  ProgTrace( TRACE5, "layer=%s, point_id=%d, pax_id=%d, first_xname=%s, first_yname=%s",
             EncodeCompLayerType( layer_type ), point_id, pax_id, first_xname.c_str(), first_yname.c_str() );
  TQuery Qry( &OraSession );
  // лочим рейс
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

  / считываем инфу по пассажиру
  switch ( layer_type ) {
    case cltGoShow:
    case cltTranzit:
    case cltCheckin:
    case cltTCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, pax.is_jmp, a.step step, pax.tid, '' airp_arv, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,:point_dep,'list',rownum) AS seat_no, "
       "       class, pers_type "
       " FROM pax, pax_grp, "
       "( SELECT COUNT(*) step FROM pax_rem "
       "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
       "WHERE pax.pax_id=:pax_id AND "
       "      pax_grp.grp_id=pax.grp_id ";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltProtCkin:
    case cltProtBeforePay: //WEB ChangeProtPaidLayer
    case cltProtAfterPay:
    case cltPNLAfterPay:
    case cltProtSelfCkin:
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, 0 grp_id, seats, 0 is_jmp, a.step step, crs_pax.tid, airp_arv, point_id, 0 point_arv, "
        "       NULL AS seat_no, class, pers_type "
        " FROM crs_pax, crs_pnr, "
        "( SELECT COUNT(*) step FROM crs_pax_rem "
        "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "       crs_pax.pnr_id=crs_pnr.pnr_id";
     if ( layer_type == cltProtCkin || layer_type == cltProtSelfCkin ||
          (seatFlag == clNotPaySeat && ( layer_type == cltPNLAfterPay || layer_type == cltProtAfterPay )) ||
          seatFlag != clNotPaySeat ) {
        break;
     }
    default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  // пассажир не найден или изменеоизводились с другой стойки или при предв. рассадке пассажир уже зарегистрирован
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
  if ( Qry.FieldAsInteger( "step" ) )
    pr_down = 1;
  else
    pr_down = 0;
  string prior_seat = Qry.FieldAsString( "seat_no" );
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
  if ( ( layer_type != cltGoShow &&
         layer_type != cltCheckin &&
         layer_type != cltTCheckin &&
         layer_type != cltTranzit ) && SALONS2::Checkin( pax_id ) ) { //???
    ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
    throw UserException( "MSG.PASSENGER.CHECKED.REFRESH_DATA" );
  }

  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  TSeatRange r;
  TSalonPoint p;
  TPlace* place;
  TSeatRanges nseats;
  int priority = Salons.getPriority( layer_type );

  // проверка на то, что пассажир имеет уже более приоритетный слой, а хотим назначить менее приоритетный
  //если пассажир имеет платный слой, то нельзя работать с предварительной разметкой ???
  Qry.Clear();
  Qry.SQLText =
    "SELECT trip_comp_layers.layer_type, first_xname, first_yname "
    " FROM trip_comp_layers, comp_layer_types "
    "WHERE point_id=:point_id AND (crs_pax_id=:pax_id OR pax_id=:pax_id) AND "
    "      trip_comp_layers.layer_type = comp_layer_types.code AND "
    "      comp_layer_types.priority<=:priority "
    "ORDER BY priority";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.CreateVariable( "priority", otInteger, priority );
  Qry.Execute();
  // выбираем все более приоритетные слои по пассажиру
  while ( !Qry.Eof ) {
    strcpy( r.first.line, Qry.FieldAsString( "first_xname" ) );
    strcpy( r.first.row, Qry.FieldAsString( "first_yname" ) );
    r.second = r.first;
    // находим места со слоями в салоне
    if ( getCurrSeat( Salons, r, p ) ) {
      vector<TPlaceList*>::iterator placeList = Salons.placelists.end();
      for( placeList = Salons.placelists.begin();placeList != Salons.placelists.end(); placeList++ ) {
        if ( (*placeList)->num == p.num )
            break;
      }
      if ( placeList != Salons.placelists.end() ) {
        SALONS2::TPoint coord( p.x, p.y );
        place = (*placeList)->place( coord );
        ProgTrace( TRACE5, "pax_id=%d, seat_no=%s, layer_type=%s, priority=%d",
                   pax_id, string(place->yname+place->xname).c_str(), EncodeCompLayerType( layer_type ), Salons.getPriority( layer_type ) );
        // проверяем а правда ли слой действует или перекрыт более приоритетным слоем
        int p = Salons.getPriority( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) );
        if ( p < priority &&
             place->isLayer( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) ) ) {
          throw UserException( "MSG.SEATS.SEAT_NO.EXIST_MORE_PRIORITY" );
        }
        if ( p == priority ) { // сохраняем места с нашим слоем
          strcpy( r.first.line, place->xname.c_str() );
          strcpy( r.first.row, place->yname.c_str() );
          r.second = r.first;
          nseats.push_back( r );
        }
      }
    }
    Qry.Next();
  }

  TSeatRanges seats;
  if ( seat_type != stDropseat ) { // заполнение вектора мест + проверка
    TQuery QrySeatRules( &OraSession );
    QrySeatRules.SQLText =
        "SELECT pr_owner FROM comp_layer_rules "
        "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
    QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( layer_type ) );
    QrySeatRules.DeclareVariable( "old_layer", otString );
  // считываем слои по новому месту и делаем проверку на то, что этот слой уже занят другим пассажиром
    seats.clear();
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    if ( !getCurrSeat( Salons, r, p ) ) {
      tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    vector<TPlaceList*>::iterator placeList = Salons.placelists.end();
    for( placeList = Salons.placelists.begin();placeList != Salons.placelists.end(); placeList++ ) {
        if ( (*placeList)->num == p.num )
            break;
    }
    if ( placeList == Salons.placelists.end() ) {
      tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    std::vector<SALONS2::TPlace> verifyPlaces;
    TCompLayerType old_pax_layer = cltUnknown;
    for ( int i=0; i<seats_count; i++ ) { // пробег по кол-ву мест и по местам
        SALONS2::TPoint coord( p.x, p.y );
        place = (*placeList)->place( coord );
        if ( !place->visible || !place->isplace || place->clname != strclass ) {
            tst();
            throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
        }
        // проверка на то, что пассажир не "ВЗ" а место у аварийного выхода
      verifyPlaces.push_back( *place );
        // проверка на то, что мы имеем право назначить слой на эти места по пассажиру
      if ( !place->layers.empty() ) {
        if ( place->layers.begin()->pax_id == pax_id &&
             place->layers.begin()->layer_type == layer_type ) {
          if ( seatFlag == clNotPaySeat ) {
            throw UserException( "MSG.SEATS.SEAT_NO.PASSENGER_OWNER" );
          }
        }
        if ( seatFlag == clNotPaySeat ) {
          QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( place->layers.begin()->layer_type ) );
          ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( place->layers.begin()->layer_type ) );
          QrySeatRules.Execute();
          if ( QrySeatRules.Eof )
            throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
          if ( QrySeatRules.FieldAsInteger( "pr_owner" ) && pax_id != place->layers.begin()->pax_id )
            throw UserException( "MSG.SEATS.SEAT_NO.OCCUPIED_OTHER_PASSENGER" );
          if ( pax_id == place->layers.begin()->pax_id ) {
            old_pax_layer = place->layers.begin()->layer_type;
          }
        }
      }
      strcpy( r.first.line, place->xname.c_str() );
      strcpy( r.first.row, place->yname.c_str() );
      r.second = r.first;
      seats.push_back( r );
      if ( pr_down )
        p.y++;
      else
        p.x++;
    }
    //INFT
    bool pr_INFT = ( TReqInfo::Instance()->client_type != ctTerm &&
                     TReqInfo::Instance()->client_type != ctPNL &&
                     seatFlag == clNotPaySeat &&
                     AllowedAttrsSeat.isWorkINFT( point_id ) &&
                     isINFT( point_id, pax_id ) ); //расчитаем prINFT
    if ( pr_INFT || !AllowedAttrsSeat.passSeats( pers_type, pr_INFT, verifyPlaces ) ) { //web-пересдка INFT запрещена
      tst();
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    if ( seatFlag != clNotPaySeat &&
         ( old_pax_layer == cltProtBeforePay || old_pax_layer == cltProtAfterPay ) &&
         !nseats.empty() ) { // были платные места - не должны измениться
      tst();
      //сравниваем
      if ( seats.size() != nseats.size() ||
           seats != nseats ) {
        tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_COINCIDE_WITH_PREPAID" );
      }
      changedOrNotPay = false;
      if ( !( seatFlag == clPaySeatSet && //надо разметить слоем
            ( TReqInfo::Instance()->client_type == ctWeb ||
              TReqInfo::Instance()->client_type == ctMobile ) ) ) {
        return changedOrNotPay;
      }
    }
    tst();
    if ( seatFlag == clPaySeatCheck ) {
      tst();
      return changedOrNotPay;
    }
    //проверка мест с разметкой нашего слоя

//	  if ( Qry.Eof ) {
//        ProgError( STDLOG, "CanChangeLayer: error xname=%s, yname=%s", first_xname.c_str(), first_yname.c_str() );
//        throw UserException( "MSG.SEATS.SEAT_NO.SEATS_NOT_AVAIL" );
//      }
  }

  int curr_tid = NoExists;
  TPointIdsForCheck point_ids_spp;
  point_ids_spp.insert( make_pair(point_id, layer_type) );
  if ( seat_type != stSeat ) { // пересадка, высадка - удаление старого слоя
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
        case cltProtSelfCkin://WEB ChangeProtPaidLayer
            // удаление из салона, если есть разметка
            DeleteTlgSeatRanges( layer_type, pax_id, curr_tid, point_ids_spp );
            break;
        //case cltProtBeforePay: //WEB ChangeProtPaidLayer
//          break;
      default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  // назначение нового слоя
  if ( seat_type != stDropseat ) { // посадка на новое место
    switch ( layer_type ) {
        case cltGoShow:
        case cltTranzit:
        case cltCheckin:
        case cltTCheckin:
          SaveTripSeatRanges( point_id, layer_type, seats, pax_id, point_id, point_arv, NowUTC() );
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
      case cltProtAfterPay:
      case cltPNLAfterPay:
      case cltProtSelfCkin:
        if ( layer_type != cltProtCkin ) { //требуется удалить старые платные слои, иначе их кол-во будет увеличиваться, отдельно вынес cltProtCkin для того, чтобы ничего по этому слою не менялось
          DeleteTlgSeatRanges( layer_type, pax_id, curr_tid, point_ids_spp );
        }
        InsertTlgSeatRanges( point_id_tlg, airp_arv, layer_type, seats, pax_id, NoExists, time_limit, false, curr_tid, point_ids_spp );
        break;
      default:
        ProgTrace( TRACE5, "!!! Unuseable layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  { //mvd
    switch ( layer_type ) {
      case cltGoShow:
        case cltTranzit:
      case cltCheckin:
      case cltTCheckin:
        rozysk::sync_pax(pax_id, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr);
        break;
      default:
        break;
    }
  }

  tid = curr_tid;

  TReqInfo *reqinfo = TReqInfo::Instance();
  string new_seat_no;
  for (TSeatRanges::iterator ns=seats.begin(); ns!=seats.end(); ns++ ) {
    if ( !new_seat_no.empty() )
        new_seat_no += " ";
    new_seat_no += ns->first.denorm_view(pr_lat_seat);
  }
  switch( seat_type ) {
    case stSeat:
        switch( layer_type ) {
          case cltGoShow:
          case cltTranzit:
          case cltCheckin:
          case cltTCheckin:
            reqinfo->LocaleToLog("EVT.PASSENGER_SEATED_MANUALLY",
                                 LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                             << PrmSmpl<std::string>("seat", new_seat_no),
                                 evtPax, point_id, idx1, idx2);
          if ( is_sync_paxs( point_id ) )
            update_pax_change( point_id, pax_id, idx1, "Р" );
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
            reqinfo->LocaleToLog("EVT.PASSENGER_CHANGE_SEAT_MANUALLY",
                                 LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                             << PrmSmpl<std::string>("seat", new_seat_no),
                                 evtPax, point_id, idx1, idx2);
          if ( is_sync_paxs( point_id ) ) {
            update_pax_change( point_id, pax_id, idx1, "Р" );
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
          reqinfo->LocaleToLog("EVT.PASSENGER_DISEMBARKED_MANUALLY",
                               LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                           << PrmSmpl<std::string>("seat", prior_seat),
                               evtPax, point_id, idx1, idx2);
          if ( is_sync_paxs( point_id ) )
            update_pax_change( point_id, pax_id, idx1, "Р" );
          break;
        default:;
        }
        break;
  }
  check_layer_change( point_ids_spp, __FUNCTION__ );
  return changedOrNotPay;
}
!!!*/
void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &APass, TSeatAlgoParams ASeatAlgoParams )
{
  // салон содержит все нормальные места (нет инвалидных мест, например с разрывами
  // нужна информация о младенцах на руках - для рассадки их в первую очередь на спец. места
  if ( Salons.placelists.empty() )
    throw EXCEPTIONS::Exception( "Не задан салон для автоматической рассадки" );
  std::vector<TCoordSeat> paxsSeats;
  FSeatAlgoParams = ASeatAlgoParams;
  CurrSalon = &Salons;
  SeatAlg = sSeatPassengers;
  CanUseLayers = false; /* не учитываем статус мест */
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
  condRates.Init( Salons, false, TReqInfo::Instance()->client_type ); // собирает все типы платных мест в массив по приоритетам, если это web-клиент, то не учитываем

  //!!! не задано pass.placeName, pass.PrevPlaceName, pass.OldPlaceName, pass.placeList,pass.InUse ,x,y???
    TQuery Qry( &OraSession );
    Qry.SQLText =
    "SELECT layer_type FROM grp_status_types WHERE layer_type IS NOT NULL ORDER BY priority ";
    Qry.Execute();

  AllowedAttrsSeat.isWorkINFT( Salons.trip_id );
  if ( AllowedAttrsSeat.pr_isWorkINFT ) { //младенцев на руки - ремака пассажиру INFT - работает медленно, но этот кусок программы устарел!!!
    int s = APass.getCount();
    for ( int i=0; i<s; i++ ) { // пробег по пассажирам
      TPassenger &pass = APass.Get( i );
      if ( isINFT( Salons.trip_id, pass.paxId ) ) {
        ProgTrace( TRACE5, "AutoReSeatsPassengers: old version: isINFT(%d)", pass.paxId );
        pass.add_rem( "INFT" );
      }
    }
  }
  int s;
  try {
    while ( !Qry.Eof ) { // пробег по слоям
      for ( int vClass=0; vClass<=2; vClass++ ) { // пробег по классам
        Passengers.Clear();
        s = APass.getCount();
        for ( int i=0; i<s; i++ ) { // пробег по пассажирам
          TPassenger &pass = APass.Get( i );
          if ( pass.isSeat ) // пассажир посажен
            continue;
          if ( pass.grp_status != DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) ) // разбиваем пассажиров по типам Бронь, Транзит...
            continue;
          if ( pass.is_jmp) {
            //!!!djek ProgError?
            continue;
          }
          ProgTrace( TRACE5, "isSeat=%d, pass.pax_id=%d, pass.foundSeats=%s, pass.clname=%s, layer_type=%s, pass.grp_status=%s, equal_y_class=%d",
                     pass.isSeat, pass.paxId, pass.foundSeats.c_str(), pass.clname.c_str(), Qry.FieldAsString( "layer_type" ),
                     EncodeCompLayerType( pass.grp_status ), pass.clname == "Э" );

          switch( vClass ) {
            case 0:
               if ( pass.clname != "П" )
                 continue;
               break;
            case 1:
               if ( pass.clname != "Б" )
                 continue;
               break;
            case 2:
               if ( pass.clname != "Э" )
                 continue;
               break;
          }
          tst();
          if ( ExistsBasePlace( Salons, pass ) ) { // пассажир не посажен, но нашлось для него базовое место - пометили как занято //??? кодировка !!!
            tst();
            continue;
          }
          Passengers.Add( pass ); /* накапливаются те у которых не нашлось базовых мест */
        } /* пробежались по всем пассажирам */
        if ( Passengers.getCount() ) {
          Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
          Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
          seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType,
                                    Passengers.KWindow,
                                    Passengers.KTube,
                                    paxsSeats );
          /* рассадка пассажира у которого не найдено базовое место */
          ProgTrace( TRACE5, "AutoReSeatsPassengers: Passengers.getCount()=%d, layer_type=%s", Passengers.getCount(),Qry.FieldAsString( "layer_type" ) );
          SeatPlaces.grp_status = Passengers.Get( 0 ).grp_status;
          SeatPlaces.separately_seats_adults_crs_pax_id = Passengers.Get( 0 ).paxId;
          SeatPlaces.SeatsPassengers( true );
          SeatPlaces.RollBack( );
          int s = Passengers.getCount();
          for ( int i=0; i<s; i++ ) {
            TPassenger &pass = Passengers.Get( i );
            int k = APass.getCount();
            for ( int j=0; j<k; j++	 ) {
              TPassenger &opass = APass.Get( j );
              if ( opass.paxId == pass.paxId ) {
                opass = pass;
                if ( opass.isSeat ) {
                  paxsSeats.push_back( TCoordSeat( opass.placeList->num, opass.Pos.x, opass.Pos.y ) );
                }
                ProgTrace( TRACE5, "pass.pax_id=%d, pass foundSeats=%s, pass.isSeat=%d, pass.InUse=%d, pass.Pos(%d,%d), seat_no.size=%zu",
                           pass.paxId,pass.foundSeats.c_str(), pass.isSeat, pass.InUse, pass.Pos.x, pass.Pos.y, pass.seat_no.size() );
                break;
              }
            }
          }
        }
      } // конец пробега по классам
      Qry.Next();
    } // конец пробега по слоям
    TQuery QryPax( &OraSession );
    TQuery QryLayer( &OraSession );
    TQuery QryUpd( &OraSession );
    QryPax.SQLText =
      "SELECT point_dep, point_arv, "
      "       salons.get_seat_no(pax.pax_id,pax.seats,NULL,pax_grp.status,point_dep,'list',rownum) AS seat_no "
      " FROM pax, pax_grp "
      " WHERE pax.pax_id=:pax_id AND "
      "       pax.grp_id=pax_grp.grp_id ";
    QryPax.DeclareVariable( "pax_id", otInteger );
    QryLayer.SQLText =
      "DELETE FROM trip_comp_layers "
      " WHERE pax_id=:pax_id ";
    QryLayer.DeclareVariable( "pax_id", otInteger );
    QryUpd.SQLText =
      "UPDATE pax SET tid=cycle_tid__seq.nextval WHERE pax_id=:pax_id";
    QryUpd.DeclareVariable( "pax_id", otInteger );

    Passengers.Clear();

    s = APass.getCount();
    TDateTime time_create = NowUTC();
    for ( int i=0; i<s; i++ ) {
        TPassenger &pass = APass.Get( i );
        ProgTrace( TRACE5, "pass.pax_id=%d, pass.isSeat=%d", pass.paxId, pass.isSeat );
        Passengers.Add( pass );
        if ( pass.isSeat )
            continue;
        QryPax.SetVariable( "pax_id", pass.paxId );
      QryPax.Execute();
      if ( QryPax.Eof )
        throw UserException( "MSG.SEATS.SEATS_DIRECTION_NOT_SET" );
      int point_dep = QryPax.FieldAsInteger( "point_dep" );
      int point_arv = QryPax.FieldAsInteger( "point_arv" );
      string prev_seat_no = QryPax.FieldAsString( "seat_no" );
      ProgTrace( TRACE5, "pax_id=%d, prev_seat_no=%s,pass.InUse=%d", pass.paxId, prev_seat_no.c_str(), pass.InUse );

        if ( !pass.InUse ) { /* не смогли посадить */
          if ( !pass.foundSeats.empty() ) {
            TReqInfo::Instance()->LocaleToLog("EVT.PASSENGER_DISEMBARKED_DUE_TO_LAYOUT",
                                              LEvntPrms() << PrmSmpl<std::string>("name", pass.fullName)
                                              << PrmSmpl<std::string>("seat", prev_seat_no),
                                              evtPax, Salons.trip_id, pass.regNo, pass.grpId);
          if ( is_sync_paxs( Salons.trip_id ) )
            update_pax_change( Salons.trip_id, pass.paxId, pass.regNo, "Р" );
        }
      }
      else {
        TSeatRanges seats;
        for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
            TSeatRange r(*i,*i);
            seats.push_back( r );
        }
        // необходимо вначале удалить все его инвалидные места из слоя регистрации
        QryLayer.SetVariable( "pax_id", pass.paxId );
        QryLayer.Execute();
        SaveTripSeatRanges( Salons.trip_id, pass.grp_status, seats, pass.paxId, point_dep, point_arv, time_create ); //???
        QryUpd.SetVariable( "pax_id", pass.paxId );
        QryUpd.Execute();
        rozysk::sync_pax(pass.paxId, TReqInfo::Instance()->desk.code, TReqInfo::Instance()->user.descr);
        QryPax.Execute();
        string new_seat_no = QryPax.FieldAsString( "seat_no" );
        ProgTrace( TRACE5, "oldplace=%s, newplace=%s", prev_seat_no.c_str(), new_seat_no.c_str() );
        if ( prev_seat_no != new_seat_no ){ /* пересадили на другое место */

          TReqInfo::Instance()->LocaleToLog("EVT.PASSENGER_CHANGE_SEAT_DUE_TO_LAYOUT",
                                            LEvntPrms() << PrmSmpl<std::string>("name", pass.fullName)
                                            << PrmSmpl<std::string>("seat", new_seat_no),
                                            evtPax, Salons.trip_id, pass.regNo, pass.grpId);
          if ( is_sync_paxs( Salons.trip_id ) )
            update_pax_change( Salons.trip_id, pass.paxId, pass.regNo, "Р" );
        }
      }
    }
  }
  catch( ... ) {
    SeatPlaces.RollBack( );
    throw;
  }
  SeatPlaces.RollBack( );
  check_waitlist_alarm( Salons.trip_id );
  ProgTrace( TRACE5, "passengers.count=%d", APass.getCount() );
}

bool getCurrSeat( const TSalonList &salonList,
                  TSeatRange &r, SALONS2::TPoint &coord,
                  vector<TPlaceList*>::const_iterator &isalonList )
{
    isalonList = salonList.end();
    for( isalonList = salonList.begin(); isalonList != salonList.end(); isalonList++ ) {
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
  service_stat_rem_grp.Load(retSERVICE_STAT, airline_oper);

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
    if ((tariffs_status==TSeatTariffMap::stUseRFISC && it->second.rate==0.0) || it->second.code.empty()) continue;
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

#warning 6 ChangeLayer: передавать TSalonList, чтобы не делать очередную начитку + определение приоритета слоя (layer_type,time_create,point_dep, point_arv)
bool ChangeLayer( const TSalonList &salonList, TCompLayerType layer_type, int time_limit, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type, TChangeLayerProcFlag seatFlag )
{
  bool changedOrNotPay = true;
  if ( seatFlag != clNotPaySeat &&
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
  first_xname = norm_iata_line( first_xname );
  first_yname = norm_iata_line( first_yname );
  ProgTrace( TRACE5, "layer=%s, point_id=%d, pax_id=%d, first_xname=%s, first_yname=%s",
             EncodeCompLayerType( layer_type ), point_id, pax_id, first_xname.c_str(), first_yname.c_str() );
  TQuery Qry( &OraSession );
  TFlights flights;
  flights.Get( point_id, ftTranzit );
  flights.Lock(__FUNCTION__);

  TAdvTripInfo operFlt;
  operFlt.getByPointId( point_id );

  /* считываем инфу по пассажиру */
  switch ( layer_type ) {
    case cltGoShow:
    case cltTranzit:
    case cltCheckin:
    case cltTCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, pax.is_jmp, a.step step, pax.tid, '' airp_arv, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,:point_dep,'list',rownum) AS seat_no, "
       "       class, pers_type "
       " FROM pax, pax_grp, "
       "( SELECT COUNT(*) step FROM pax_rem "
       "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
       "WHERE pax.pax_id=:pax_id AND "
       "      pax_grp.grp_id=pax.grp_id ";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltProtCkin:
    case cltProtBeforePay: //WEB ChangeProtPaidLayer
    case cltProtAfterPay:
    case cltPNLAfterPay:
    case cltProtSelfCkin:
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, 0 grp_id, seats, 0 is_jmp, a.step step, crs_pax.tid, airp_arv, point_id, 0 point_arv, "
        "       NULL AS seat_no, class, pers_type "
        " FROM crs_pax, crs_pnr, "
        "( SELECT COUNT(*) step FROM crs_pax_rem "
        "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "       crs_pax.pnr_id=crs_pnr.pnr_id";
        if ( layer_type == cltProtCkin || layer_type == cltProtSelfCkin ||
             (seatFlag == clNotPaySeat && ( layer_type == cltPNLAfterPay || layer_type == cltProtAfterPay )) ||
             seatFlag != clNotPaySeat ) {
          break;
        }
    default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  // пассажир не найден или изменеоизводились с другой стойки или при предв. рассадке пассажир уже зарегистрирован
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
  if ( Qry.FieldAsInteger( "step" ) )
    pr_down = 1;
  else
    pr_down = 0;
  string prior_seat = Qry.FieldAsString( "seat_no" );
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
         layer_type != cltTranzit ) && prCheckin ) { //???!!!переделать
    ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
    throw UserException( "MSG.PASSENGER.CHECKED.REFRESH_DATA" );
  }

  // проверка на то, что пассажир имеет уже более приоритетный слой, а хотим назначить менее приоритетный
  //если пассажир имеет платный слой, то нельзя работать с предварительной разметкой ???
  TSeatLayer seatLayer;
  std::set<TPlace*,CompareSeats> seats, nseats;
  salonList.getPaxLayer( point_id, pax_id,
                         seatLayer, seats );
  if ( seatLayer.layer_type != cltUnknown ) {
    if ( BASIC_SALONS::TCompLayerTypes::Instance()->priority( seatLayer.layer_type ) <
         BASIC_SALONS::TCompLayerTypes::Instance()->priority( layer_type ) ) {
      throw UserException( "MSG.SEATS.SEAT_NO.EXIST_MORE_PRIORITY" ); //пассажир имеет более приоритетное место
    }
  }
  TSeatRanges seatRanges;
  vector<pair<TSeatRange,TRFISC> > tariffs;
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
  if ( seat_type != stDropseat ) { // заполнение вектора мест + проверка
    if ( prCheckin ) {
      ProgTrace( TRACE5, "pax_id=%d", pax_id );
      passTariffs.get( pax_id );
    }
    else {
      ProgTrace( TRACE5, "pax_id=%d", pax_id );
      TMktFlight flight;
      flight.getByCrsPaxId( pax_id );
      TTripInfo markFlt;
      markFlt.airline = flight.airline;
      CheckIn::TPaxTknItem tkn;
      CheckIn::LoadCrsPaxTkn( pax_id, tkn);
      passTariffs.get( operFlt, markFlt, tkn );
    }
    TQuery QrySeatRules( &OraSession );
    QrySeatRules.SQLText =
        "SELECT pr_owner FROM comp_layer_rules "
        "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
    QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( layer_type ) );
    QrySeatRules.DeclareVariable( "old_layer", otString );
  // считываем слои по новому месту и делаем проверку на то, что этот слой уже занят другим пассажиром
    seatRanges.clear();
    tariffs.clear();
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    if ( !getCurrSeat( salonList, r, coord, isalonList ) ) {
      tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    TZonesBetweenLines  zonesBetweenLines( point_id );
    TPaxsCover grpPaxs;
    grpPaxs.push_back( TPaxCover( pax_id, ASTRA::NoExists ) );
    std::set<int> pax_with_baby_lists;
    std::map<int,TPaxList>::const_iterator ipaxs = salonList.pax_lists.find( point_id );
    if ( zonesBetweenLines.useInfantSection() ) {
      if ( ipaxs != salonList.pax_lists.end() ) {
        //tst();
        ipaxs->second.setPaxWithInfant( pax_with_baby_lists );
      }
      if ( pax_with_baby_lists.find( pax_id ) != pax_with_baby_lists.end() ) {
        zonesBetweenLines.setDisabled( point_id, nullptr, *isalonList, grpPaxs, pax_with_baby_lists, TZonesBetweenLines::DisableMode::dlrss );
      }
    }
    std::vector<SALONS2::TPlace> verifyPlaces;
    for ( int i=0; i<seats_count; i++ ) { // пробег по кол-ву мест и по местам
      seat = (*isalonList)->place( coord );
      if ( !seat->visible || !seat->isplace || seat->clname != strclass ) {
        tst();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }
      //проверка на то, что блок мест принадлежит пассажиру с ребенком и у нас пассажир с ребенком
      if ( seat->isLayer( cltDisable ) ) {
        zonesBetweenLines.rollbackDisabled();
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL_WITH_INFT" );
      }
      //назначим тарифы для пассажира
      //TPropsPoints points( salonList.filterSets.filterRoutes, salonList.filterRoutes.point_dep, salonList.filterRoutes.point_arv );
      //bool pr_departure_tariff_only = true;
      TRFISC rfisc;
      ProgTrace( TRACE5, "RFISCMode=%d", salonList.getRFISCMode() );
      passTariffs.trace( TRACE5 );
      if ( passTariffs.status() == TSeatTariffMap::stUseRFISC ) {
        SALONS2::TSelfCkinSalonTariff SelfCkinSalonTariff;
        SelfCkinSalonTariff.setTariffMap( point_id, passTariffs );
        seat->SetRFISC( point_id, passTariffs );
        std::map<int, TRFISC,classcomp> vrfiscs;
        seat->GetRFISCs( vrfiscs );
        if ( vrfiscs.find( point_id ) != vrfiscs.end() ) {
          rfisc = vrfiscs[ point_id ];
        }
      }
      else { //старый режим работы ???
        seat->convertSeatTariffs( point_id );
        rfisc.color = seat->SeatTariff.color;
        rfisc.rate = seat->SeatTariff.rate;
        rfisc.currency_id = seat->SeatTariff.currency_id;
      }
      ProgTrace( TRACE5, "rfisc=%s", rfisc.str().c_str() );

/*!!!!      seat->convertSeatTariffs( point_id );
      seat->SetTariffsByColor( passTariffs, true );
      seat->SetRFICSRemarkByColor( point_id, passTariffs );*/
      verifyPlaces.push_back( *seat );
        // проверка на то, что пассажир не "ВЗ" а место у аварийного выхода
        // проверка на то, что мы имеем право назначить слой на эти места по пассажиру
      TSeatLayer tmp_layer = seat->getDropBlockedLayer( point_id );
      if ( tmp_layer.layer_type != cltUnknown ) {
        ProgTrace( TRACE5, "getDropBlockedLayer: %s add %s",
                   string(seat->yname+seat->xname).c_str(), tmp_layer.toString().c_str() );
        throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      }

      std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp > layers;
      seat->GetLayers( layers, glAll );
      for ( std::map<int, std::set<TSeatLayer,SeatLayerCompare>,classcomp >::iterator ilayers=layers.begin();
            ilayers!=layers.end(); ilayers++ ) {
        if ( ilayers->second.empty() ) {
          continue;
        }
        if ( ilayers->second.begin()->getPaxId() == pax_id &&
             ilayers->second.begin()->layer_type == layer_type ) {
          if ( seatFlag == clNotPaySeat ) {
            throw UserException( "MSG.SEATS.SEAT_NO.PASSENGER_OWNER" );
          }
          else {
            ProgTrace( TRACE5, "layer_type=%s, seat=%s", EncodeCompLayerType( layer_type ), string(seat->xname + seat->yname).c_str() );
            nseats.insert( seat );
            continue;
          }
        }
        QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( ilayers->second.begin()->layer_type ) );
        ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( ilayers->second.begin()->layer_type ) );
        QrySeatRules.Execute();
        if ( QrySeatRules.Eof ||
           ( QrySeatRules.FieldAsInteger( "pr_owner" ) &&
             pax_id != ilayers->second.begin()->getPaxId() ) ) {
          if ( ilayers->second.begin()->getPaxId() == NoExists ) {
            throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
          }
          string airp_dep, airp_arv;
          Qry.SetVariable( "point_id", ilayers->second.begin()->point_dep );
          Qry.Execute();
          if ( !Qry.Eof ) {
            airp_dep = Qry.FieldAsString( "airp" );
          }
          Qry.SetVariable( "point_id", ilayers->second.begin()->point_arv );
          Qry.Execute();
          if ( !Qry.Eof ) {
            airp_arv = Qry.FieldAsString( "airp" );
          }
          throw UserException( "MSG.SEATS.SEAT_NO.OCCUPIED_OTHER_LEG_PASSENGER",
                               LParams()<<LParam("airp_dep", ElemIdToCodeNative(etAirp,airp_dep) )
                                        <<LParam("airp_arv", ElemIdToCodeNative(etAirp,airp_arv) ) );
        }
      }
      if ( !UsedPayedPreseatForPassenger( *seat, pax_id, layer_type ) ) {
         throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
      }
      strcpy( r.first.line, seat->xname.c_str() );
      strcpy( r.first.row, seat->yname.c_str() );
      r.second = r.first;
      seatRanges.push_back( r );
      //!!!tariffs.push_back( make_pair(r, seat->SeatTariff ) );
      tariffs.push_back( make_pair(r, rfisc ) );
      if ( pr_down )
        coord.y++;
      else
        coord.x++;
    }
    bool pr_INFT = ( TReqInfo::Instance()->client_type != ctTerm &&
                     TReqInfo::Instance()->client_type != ctPNL &&
                     seatFlag == clNotPaySeat && // разметка платным слоем через
                     AllowedAttrsSeat.isWorkINFT( point_id ) &&
                     isINFT( point_id, pax_id ) );
    if ( pr_INFT || !AllowedAttrsSeat.passSeats( pers_type, pr_INFT, verifyPlaces ) ) { //web-пересадка INFT запрещена
      tst();
      throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    }
    if ( seatFlag != clNotPaySeat &&
         ( seatLayer.layer_type == cltProtBeforePay || seatLayer.layer_type == cltProtAfterPay ) &&
         !seats.empty() ) { // были платные места - не должны измениться!!!
      ProgTrace( TRACE5, "seats.size()=%zu, nseats.size()=%zu", seats.size(), nseats.size() );
      //сравниваем
      if ( seats.size() != nseats.size() ||
           seats != nseats ) {
        tst();
        throw  UserException( "MSG.SEATS.SEAT_NO.NOT_COINCIDE_WITH_PREPAID" );
      }
      changedOrNotPay = false;
      if ( !( seatFlag == clPaySeatSet &&
             ( TReqInfo::Instance()->client_type == ctWeb ||
               TReqInfo::Instance()->client_type == ctMobile ) ) ) {
        return changedOrNotPay;
      }
    }
    tst();
  }

  if ( seatFlag == clPaySeatCheck ) {
    tst();
    return changedOrNotPay;
  }

  int curr_tid = NoExists;
  TPointIdsForCheck point_ids_spp;
  point_ids_spp.insert( make_pair(point_id, layer_type) );
  if ( seat_type != stSeat ) { // пересадка, высадка - удаление старого слоя
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
            // удаление из салона, если есть разметка
            DeleteTlgSeatRanges( layer_type, pax_id, curr_tid, point_ids_spp );
        break;
     //   case cltProtBeforePay: //WEB ChangeProtPaidLayer
//          break;
      default:
        ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  // назначение нового слоя
  if ( seat_type != stDropseat ) { // посадка на новое место
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

  for ( vector<pair<TSeatRange,TRFISC> >::iterator it=tariffs.begin(); it!=tariffs.end(); it++ ) {
    if (it!=tariffs.begin())
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
          reqinfo->LocaleToLog("EVT.PASSENGER_SEATED_MANUALLY",
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
          reqinfo->LocaleToLog("EVT.PASSENGER_CHANGE_SEAT_MANUALLY",
                               LEvntPrms() << PrmSmpl<std::string>("name", fullname)
                                           << seatPrmEnum,
                               evtPax, point_id, idx1, idx2);
          if ( prCheckin ) {
            SyncPRSA( operFlt.airline, pax_id, passTariffs.status(), tariffs );
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
          reqinfo->LocaleToLog("EVT.PASSENGER_DISEMBARKED_MANUALLY",
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
  check_layer_change( point_ids_spp, paxs_external_logged, __FUNCTION__ );
  return changedOrNotPay;
}

//point_arv,class,ASTRA::TCompLayerType
std::map<int,map<string,map<ASTRA::TCompLayerType,vector<TPassenger> > > > passes;

void dividePassengersToGrpsAutoSeats( TIntStatusSalonPassengers::const_iterator ipass_status,
                                      const SALONS2::TSalonList &salonList,
                                      vector<TPassengers> &passGrps,
                                      std::set<int> &pax_lists_with_baby )
{
  TClsGrp &cls_grp = (TClsGrp &)base_tables.get("CLS_GRP");    //cls_grp.code subclass,
  SEATS2::TSublsRems subcls_rems( salonList.getAirline() );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT rem, rem_code, comp_rem_types.pr_comp "
    " FROM pax_rem, comp_rem_types "
    "WHERE pax_rem.pax_id=:pax_id AND "
    "      rem_code=comp_rem_types.code AND pr_comp<>0"
    " ORDER BY pr_comp, code ";
  Qry.DeclareVariable( "pax_id", otInteger );

  TPassengers PassWoBaby;
  passGrps.clear();
  for ( std::set<TSalonPax,ComparePassenger>::const_iterator ipass=ipass_status->second.begin();
        ipass!=ipass_status->second.end(); ipass++ ) {
    TWaitListReason waitListReason;
    //!!!TPassSeats ranges;
    string seat_no = ipass->seat_no( "one", salonList.isCraftLat(), waitListReason );
    if ( waitListReason.layerStatus == layerValid ) {
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
    vpass.pers_type = ipass->pers_type;
    vpass.paxId = ipass->pax_id;
    vpass.point_arv = ipass->point_arv;
    vpass.foundSeats = string("(") + ipass->prior_seat_no( "one", salonList.isCraftLat() ) + string(")");
    vpass.isSeat = false;
    vpass.countPlace = ipass->seats;
    vpass.is_jmp = ipass->is_jmp;
    vpass.clname = ipass->cl;
    if ( ipass->pr_infant != ASTRA::NoExists ) {
      ProgTrace( TRACE5, "AutoReSeatsPassengers: pax_id=%d add INFT", vpass.paxId );
      vpass.add_rem( "INFT" );
    }
    Qry.SetVariable( "pax_id", ipass->pax_id );
    Qry.Execute();
    vpass.Step = sRight;
    for ( ; !Qry.Eof; Qry.Next() ) {
      vpass.add_rem( Qry.FieldAsString( "rem_code" ) );
      if ( string(Qry.FieldAsString( "rem_code" )) == "STCR" ) {
        vpass.Step = sDown;
      }
    }
    string rem;
    const TBaseTableRow &row=cls_grp.get_row( "id", ipass->class_grp );
    if ( subcls_rems.IsSubClsRem( row.AsString( "code" ), rem ) ) {
      vpass.add_rem( rem );
    }
    ProgTrace( TRACE5, "pass add pax_id=%d", vpass.paxId );
    if ( !pax_lists_with_baby.empty() &&
         pax_lists_with_baby.find( vpass.paxId ) != pax_lists_with_baby.end() ) {
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
                            TSeatAlgoParams ASeatAlgoParams )
{
  ProgTrace( TRACE5, "AutoReSeatsPassengers: point_id=%d", salonList.getDepartureId() );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, salonList.getDepartureId() );
  Qry.Execute();
  if ( Qry.Eof )
    throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  string airline = Qry.FieldAsString( "airline" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT rem, rem_code, comp_rem_types.pr_comp "
    " FROM pax_rem, comp_rem_types "
    "WHERE pax_rem.pax_id=:pax_id AND "
    "      rem_code=comp_rem_types.code AND pr_comp<>0"
    " ORDER BY pr_comp, code ";
  Qry.DeclareVariable( "pax_id", otInteger );

  std::vector<TCoordSeat> paxsSeats;
  TPassengers Passes;
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");
  TZonesBetweenLines zonesBetweenLines( salonList.getDepartureId() );
  std::set<int> pax_lists_with_baby;
  if ( zonesBetweenLines.useInfantSection() ) {
    //находим всех младенцев
    std::map<int,TPaxList>::const_iterator ipaxs = salonList.pax_lists.find( salonList.getDepartureId() );
    if ( ipaxs != salonList.pax_lists.end() ) {
      ipaxs->second.setPaxWithInfant( pax_lists_with_baby );
    }
  }

  //arv - вначале идут пассажиры с самым длинным маршрутом
  for ( TIntArvSalonPassengers::const_iterator ipass_arv=passengers.begin();
        ipass_arv!=passengers.end(); ipass_arv++ ) {
    //class
    for ( TIntClassSalonPassengers::const_iterator ipass_class=ipass_arv->second.begin();
          ipass_class!=ipass_arv->second.end(); ipass_class++ ) {
      //grp_status
      for ( TIntStatusSalonPassengers::const_iterator ipass_status=ipass_class->second.begin();
            ipass_status!=ipass_class->second.end(); ipass_status++ ) {
        //заполняем пассажирами с одними характеристиками
        vector<ASTRA::TCompLayerType> grp_layers;
        const TGrpStatusTypesRow &grp_status_row = (const TGrpStatusTypesRow&)grp_status_types.get_row( "code", ipass_status->first );
        if ( DecodePaxStatus(ipass_status->first.c_str()) == psCrew )
          throw EXCEPTIONS::Exception("AutoReSeatsPassengers: DecodePaxStatus(ipass_status->first) == psCrew");
        ASTRA::TCompLayerType grp_layer_type = DecodeCompLayerType( grp_status_row.layer_type.c_str() );
        grp_layers.push_back( grp_layer_type );
        SALONS2::TFilterRoutesSets filterRoutes = salonList.getFilterRoutes();
        filterRoutes.point_arv = ipass_arv->first;
        SALONS2::TSalons SalonsN;
        if ( salonList.empty() )
          throw EXCEPTIONS::Exception( "Не задан салон для автоматической рассадки" );
        TDropLayersFlags dropLayersFlags;
        TCreateSalonPropFlags propFlags;
        propFlags.setFlag( clDepOnlyTariff );
        tst();
        if ( !salonList.CreateSalonsForAutoSeats( SalonsN,
                                                  filterRoutes,
                                                  propFlags,
                                                  grp_layers,
                                                  dropLayersFlags ) ) {
          throw EXCEPTIONS::Exception( "Не задан салон для автоматической рассадки" );
        }
        if ( SalonsN.placelists.empty() )
          throw EXCEPTIONS::Exception( "Не задан салон для автоматической рассадки" );
        //помечаем занятые места пассажирами
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
                                    BASIC_SALONS::TCompLayerTypes::Instance()->priority( cltCheckin ) );
            if ( CurrSalon->canAddOccupy( place ) ) {
              CurrSalon->AddOccupySeat( pass.placeList->num, place->x, place->y );
            }
          }
          tst();
        }
        tst();
        Passengers.Clear();
        FSeatAlgoParams = ASeatAlgoParams;
        CurrSalon = &SalonsN;
        SeatAlg = sSeatPassengers;
        CanUseLayers = false; /* не учитываем статус мест */
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
        condRates.Init( SalonsN, false, TReqInfo::Instance()->client_type ); // собирает все типы платных мест в массив по приоритетам, если это web-клиент, то не учитываем

        try {
          //набор групп
          vector<TPassengers> passGrps;
          dividePassengersToGrpsAutoSeats( ipass_status, salonList, passGrps, pax_lists_with_baby );
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
              Passengers.Add( vpass );
              ExistsBasePlace( SalonsN, vpass ); // пассажир не посажен, но нашлось для него базовое место - пометили как занято //??? кодировка !!!
            }
            if ( Passengers.getCount() ) {
              Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
              Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
              seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType,
                                        Passengers.KWindow,
                                        Passengers.KTube,
                                        paxsSeats );
              /* рассадка пассажира у которого не найдено базовое место */
              ProgTrace( TRACE5, "AutoReSeatsPassengers: Passengers.getCount()=%d, layer_type=%s",
                         Passengers.getCount(), grp_status_row.layer_type.c_str()  );
              SeatPlaces.grp_status = Passengers.Get( 0 ).grp_status;
              bool separately_seats_adult_with_baby = zonesBetweenLines.useInfantSection() && Passengers.withBaby();
              if ( separately_seats_adult_with_baby ) {
                SeatPlaces.separately_seats_adults_crs_pax_id = Passengers.Get( 0 ).paxId;
              }
              else {
                SeatPlaces.separately_seats_adults_crs_pax_id = ASTRA::NoExists;
              }
              zonesBetweenLines.clear();
              if ( separately_seats_adult_with_baby ) {
                TPaxsCover paxs;
                for ( int i=0; i<Passengers.getCount(); i++ ) {
                    paxs.push_back( TPaxCover( Passengers.Get(i).paxId, ASTRA::NoExists ) );
                }
                zonesBetweenLines.setDisabled( CurrSalon, paxs, pax_lists_with_baby );
              }
              SeatPlaces.SeatsPassengers( true );
              zonesBetweenLines.rollbackDisabled( CurrSalon );
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
    if ( pass.InUse ) { /* смогли посадить */
      TSeatRanges seats;
      for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
          TSeatRange r(*i,*i);
          seats.push_back( r );
      }
      // необходимо вначале удалить все его инвалидные места из слоя регистрации
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
      update_pax_change( salonList.getDepartureId(), ipass->paxId, ipass->regNo, "Р" );
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
};

bool CompGrp( TPassenger item1, TPassenger item2 )
{
  TBaseTable &classes=base_tables.get("classes");
  const TBaseTableRow &row1=classes.get_row("code",item1.clname);
  const TBaseTableRow &row2=classes.get_row("code",item2.clname);
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


void TPassengers::Build( xmlNodePtr dataNode )
{
  if ( !getCount() )
    return;
  for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
    p->InUse = false;
  }

  xmlNodePtr passNode = NewTextChild( dataNode, "passengers" );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT code,layer_type,name FROM grp_status_types WHERE layer_type IS NOT NULL ORDER BY priority";
  Qry.Execute();

  TDefaults def;
  bool createDefaults=false;
  vector<TPassenger> ps;
  while ( !Qry.Eof ) {
    for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
        if ( p->InUse || p->grp_status != DecodeCompLayerType(Qry.FieldAsString( "layer_type" )) )
            continue;
        ps.push_back( *p );
    }
    // сортировка по grp_status + класс + группа
    sort(ps.begin(),ps.end(),CompGrp);
    ProgTrace( TRACE5, "ps.size()=%zu, layer_type=%s", ps.size(), Qry.FieldAsString( "layer_type" ) );
    if ( !ps.empty() ) {
      createDefaults=true;
        xmlNodePtr pNode = NewTextChild( passNode, "layer_type", Qry.FieldAsString( "layer_type" ) );
        SetProp( pNode, "name", Qry.FieldAsString( "name" ) );
        for ( vector<TPassenger>::iterator ip=ps.begin(); ip!=ps.end(); ip++ ) {
            ip->build( NewTextChild( pNode, "pass" ), def );
            ip->InUse = true;
        }
        ps.clear();
    }
    Qry.Next();
  }
  if (createDefaults)
  {
    xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
    NewTextChild( defNode, "clname", def.clname );
    NewTextChild( defNode, "grp_layer_type", EncodeCompLayerType(def.grp_status) );
    NewTextChild( defNode, "pers_type", ElemIdToCodeNative(etPersType, def.pers_type) );
    NewTextChild( defNode, "seat_no", def.placeName );
    NewTextChild( defNode, "wl_type", def.wl_type );
    NewTextChild( defNode, "seats", def.countPlace );
    NewTextChild( defNode, "isseat", (int)def.isSeat );
    NewTextChild( defNode, "ticket_no", def.ticket_no );
    NewTextChild( defNode, "document", def.document );
    NewTextChild( defNode, "bag_weight", def.bag_weight );
    NewTextChild( defNode, "bag_amount", def.bag_amount );
    NewTextChild( defNode, "excess", def.excess );
    NewTextChild( defNode, "trip_from", def.trip_from );
    NewTextChild( defNode, "comp_rem", def.comp_rem );
    NewTextChild( defNode, "pr_down", (int)def.pr_down );
    NewTextChild( defNode, "pass_rem", def.pass_rem );
  };
}

bool TPassengers::existsNoSeats()
{
  int s = getCount();
  for ( int i=0; i<s; i++ ) {
    if ( !Get( i ).InUse )
      return true;
  }
  return false;
}

TPassengers Passengers;

} // end namespace SEATS2


//seats with:SeatAlg=2,FCanUseRems=sIgnoreUse,FCanUseAlone=0,KeyStatus=1,FCanUseTube=0,FCanUseSmoke=0,PlaceStatus=, MAXPLACE=3,canUseOneRow=0, CanUseSUBCLS=1, SUBCLS_REM=SUBCLS
//seats with:SeatAlg=2,FCanUseRems=sNotUse_NotUseDenial,FCanUseAlone=1,KeyStatus=1,FCanUseTube=0,FCanUseS



