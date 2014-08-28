#ifndef _POINTS_H_
#define _POINTS_H_

#include <string>
#include <vector>
#include <map>
#include "basic.h"
#include "astra_consts.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "sopp.h"
#include "stages.h"

enum TStatus { tdUpdate, tdInsert, tdDelete };


enum TDestField { dfPoint_num, dfAirp, dfPr_tranzit, dfFirst_point,
                  dfAirline, dfFlt_no, dfSuffix, dfCraft, dfBort, dfScd_in,
                  dfEst_in, dfAct_in, dfScd_out, dfEst_out, dfAct_out,
                  dfTrip_type, dfLitera, dfPart_in, dfPart_out, dfRemark, dfPr_reg,
                  dfpr_del, DfTid, dfAirp_fmt, dfAirline_fmt, dfSuffix_fmt, dfCraft_fmt,
                  dfStations, dfStages, dfEvents };
                  
                  
enum TUseDestData { udNoCalcESTTimeStage, udDelays, udStages, udCargo, udMaxCommerce,
                    udNum, udStations };
enum TDestEvents { dmChangeCraft, dmSetCraft, dmInitStages, dmInitComps,
                   dmChangeBort, dmSetBort, dmSetCancel, dmSetUnCancel, dmSetDelete,
                   dmSetSCDOUT, dmChangeSCDOUT, dmDeleteSCDOUT,
                   dmSetSCDIN, dmChangeSCDIN, dmDeleteSCDIN,
                   dmSetESTOUT, dmChangeESTOUT, dmDeleteESTOUT,
                   dmSetESTIN, dmChangeESTIN, dmDeleteESTIN,
                   dmSetACTOUT, dmChangeACTOUT, dmDeleteACTOUT,
                   dmSetACTIN, dmChangeACTIN, dmDeleteACTIN,
                   dmChangeTripType, dmChangeLitera, dmChangeParkIn,
                   dmChangeParkOut, dmChangeDelays,
                   dmChangeAirline, dmChangeFltNo, dmChangeSuffix, dmChangeAirp,
                   dmTranzit, dmReg, dmFirst_Point, dmChangeRemark,
                   dmChangeStageESTTime, dmPoint_Num, dmChangeStages };
                   
enum TPointsEvents { peInsert, pePointNum };
enum TTripEvents { teNewLand, teNewTakeoff, teDeleteLand, teDeleteTakeoff,
                   teSetCancelLand, teSetCancelTakeoff,
                   teSetUnCancelLand, teSetUnCancelTakeoff,
                   teSetSCDIN, teChangeSCDIN, teDeleteSCDIN,
                   teSetESTIN, teChangeESTIN, teDeleteESTIN,
                   teSetACTIN, teChangeACTIN, teDeleteACTIN,
                   teChangeParkIn, teChangeCraftLand, teSetCraftLand,
                   teChangeBortLand, teSetBortLand, teChangeTripTypeLand,
                   teChangeLiteraLand, teChangeFlightAttrLand,
                   teInitStages, teInitComps, teChangeStageESTTime,
                   teSetSCDOUT, teChangeSCDOUT, teDeleteSCDOUT,
                   teSetESTOUT, teChangeESTOUT, teDeleteESTOUT,
                   teSetACTOUT, teChangeACTOUT, teDeleteACTOUT,
                   teChangeCraftTakeoff, teSetCraftTakeoff,
                   teChangeBortTakeoff, teSetBortTakeoff,
                   teChangeLiteraTakeoff, teChangeTripTypeTakeoff, teChangeParkOut,
                   teChangeFlightAttrTakeoff, teChangeDelaysTakeoff,
                   teTranzitTakeoff, teRegTakeoff, teFirst_PointTakeoff,
                   teChangeRemarkTakeoff, tePoint_NumTakeoff,
                   teNeedChangeComps, teNeedUnBindTlgs, teNeedBindTlgs,
                   teChangeCargos, teChangeMaxCommerce, teChangeStages,
                   teChangeStations, teNeedApisUSA };
                   
class TPointsDestDelay {
public:
	std::string code;
	BASIC::TDateTime time;
	TPointsDestDelay( const TSOPPDelay &delay ) {
    code = delay.code;
    time = delay.time;
	};
	TPointsDestDelay(){};
};

class TPointsDest;
class TPointDests;

struct TPointsDestCargo {
	int cargo;
	int mail;
	int point_arv;
	std::string key;
	std::string airp_arv;
	TElemFmt airp_arv_fmt;
	//int dosbag_weight;
	TPointsDestCargo() {
		cargo = 0;
		mail = 0;
		point_arv = ASTRA::NoExists;
		airp_arv_fmt = efmtUnknown;
	}
	bool equal( const TPointsDestCargo &vcargo ) {
    return ( cargo == vcargo.cargo &&
             mail == vcargo.mail &&
             point_arv == vcargo.point_arv &&
             airp_arv == vcargo.airp_arv &&
             airp_arv_fmt == vcargo.airp_arv_fmt &&
             key == vcargo.key );
	}
};

class TFlightCargos {
  private:
    std::vector<TPointsDestCargo> cargos;
    bool calc_point_id;
  public:
    TFlightCargos() {
      calc_point_id = false;
    }
    void Load( int point_id, bool pr_tranzit, int first_point, int point_num, int pr_cancel );
    void Save( int point_id, const std::vector<TPointsDest> &dests );
    void Add( TPointsDestCargo &cargo ) {
      if ( !cargo.key.empty() )
        calc_point_id = true;
      cargos.push_back( cargo );
    }
    void Get( std::vector<TPointsDestCargo> &vcargos ) {
       vcargos.clear();
       vcargos.insert( vcargos.begin(),  cargos.begin(), cargos.end() );
       return;
    }
    void Clear() {
      cargos.clear();
    }
    bool equal( const TFlightCargos &flightCargos ) {
      std::vector<TPointsDestCargo>::iterator icargo=cargos.begin();
      std::vector<TPointsDestCargo>::const_iterator iprior_cargo=flightCargos.cargos.begin();
      if ( cargos.size() != flightCargos.cargos.size() )
        return false;
      for ( ;
            icargo!=cargos.end() &&
            iprior_cargo!=flightCargos.cargos.end();
            icargo++, iprior_cargo++ ) {
        if ( !icargo->equal( *iprior_cargo ) ) {
          return false;
        }
      }
      return true;
    }
    bool calcPoint_id() {
      return calc_point_id;
    }
};

class TFlightMaxCommerce {
  private:
    int value;
  public:
    TFlightMaxCommerce() {
      value = ASTRA::NoExists;
    }
    void SetValue( int vvalue ) {
      value = vvalue;
    }
    int GetValue( ) {
      return value;
    }
    void Load( int point_id );
    void Save( int point_id );
    bool equal( const TFlightMaxCommerce &flightMaxCommerce ) {
      return ( value == flightMaxCommerce.value );
    }
};

class TFlightDelays {
  private:
    std::vector<TPointsDestDelay> delays;
  public:
    TFlightDelays( const std::vector<TSOPPDelay> &soppdelays ) {
      for ( std::vector<TSOPPDelay>::const_iterator i=soppdelays.begin(); i!=soppdelays.end(); i++ ) {
        TPointsDestDelay pdelay( *i );
        Add( pdelay );
      }
    }
    TFlightDelays(){};
    void Load( int point_id );
    void Save( int point_id );
    bool Empty() {
      return delays.empty();
    }
    void Add( TPointsDestDelay &delay ) {
      delays.push_back( delay );
    }
    bool equal( const TFlightDelays &flightDelays ) {
      if ( delays.size() != flightDelays.delays.size() )
        return false;
      std::vector<TPointsDestDelay>::iterator idelay=delays.begin();
      std::vector<TPointsDestDelay>::const_iterator ipriordelay=flightDelays.delays.begin();
      for ( ;
            idelay!=delays.end() &&
            ipriordelay!=flightDelays.delays.end();
            idelay++, ipriordelay++ ) {
        if ( idelay->code != ipriordelay->code ||
             idelay->time != ipriordelay->time ) {
          return false;
        }
      }
      return true;
    }
    void Get( std::vector<TPointsDestDelay> &vdelays ) {
      vdelays.clear();
      vdelays.insert( vdelays.begin(),  delays.begin(), delays.end() );
      return;
    }
};

class TFlightStages {
  private:
    TMapTripStages stages;
  public:
    void Load( int point_id );
    void Save( int point_id );
    bool Empty() {
      return stages.empty();
    }
    TTripStage GetStage( const TStage &stage_id ) {
      return stages[ stage_id ];
    }
    void SetStage( const TStage &stage_id, const TTripStage &stage ) {
      stages[ stage_id ].est = stage.est;
      stages[ stage_id ].act = stage.act;
      stages[ stage_id ].pr_auto = stage.pr_auto;
    }
    bool equal( const TFlightStages &flightStages ) {
      if ( stages.size() != flightStages.stages.size() )
        return false;
     TMapTripStages::iterator istage=stages.begin();
     TMapTripStages::const_iterator iprior_stage=flightStages.stages.begin();
      for ( ;
            istage!=stages.end() &&
            iprior_stage!=flightStages.stages.end();
            istage++, iprior_stage++ ) {
        if ( istage->first != iprior_stage->first ||
             !istage->second.equal( iprior_stage->second ) ) {
          return false;
        }
      }
      return true;
    }
};

class TFlightStations {
  private:
    tstations stations;
    bool intequal( const tstations &oldstations, const tstations &newstations, std::string work_mode ) {
      for ( tstations::const_iterator istation=oldstations.begin(); istation!=oldstations.end(); istation++ ) {
        if ( !work_mode.empty() && istation->work_mode != work_mode )
          continue;
        tstations::const_iterator jstation=newstations.begin();
        for ( ; jstation!=newstations.end(); jstation++ ) {
          if ( istation->work_mode == jstation->work_mode &&
               istation->name == jstation->name &&
               istation->pr_main == jstation->pr_main )
            break;
        }
        if ( jstation == newstations.end() )
          return false;
      }
      return true;
    }
  public:
    void Load( int point_id );
    void Save( int point_id );
    void Add( TSOPPStation &station ) {
      stations.push_back( station );
    }
    bool equal( const TFlightStations &flightStations, std::string work_mode ) {
      return ( intequal( stations, flightStations.stations, work_mode ) &&
               intequal( flightStations.stations, stations, work_mode ) );
    }
    bool equal( const TFlightStations &flightStations ) {
      std::string work_mode;
      return ( intequal( stations, flightStations.stations, work_mode ) &&
               intequal( flightStations.stations, stations, work_mode ) );
    }
    void Get( tstations &vstations ) {
      vstations = stations;
    }
};

class TPointsDest {
private:
public:
  BitSet<TDestEvents> events;
  BitSet<TUseDestData> UseData;
  TStatus status;
  int point_id;
  int point_num;
  std::string airp;
  bool pr_tranzit;
  int first_point;
  std::string airline;
  int flt_no;
  std::string suffix;
  std::string craft;
  std::string bort;
  int comp_id;
  BASIC::TDateTime scd_in;
  BASIC::TDateTime est_in;
  BASIC::TDateTime act_in;
  BASIC::TDateTime scd_out;
  BASIC::TDateTime est_out;
  BASIC::TDateTime act_out;
  std::string trip_type;
  std::string litera;
  std::string park_in;
  std::string park_out;
  std::string remark;
  bool pr_reg;
  int pr_del;
  int tid;
  TElemFmt airline_fmt;
  TElemFmt airp_fmt;
  TElemFmt suffix_fmt;
  TElemFmt craft_fmt;
  TFlightStages stages;
  TFlightDelays delays;
  BASIC::TDateTime stage_scd, stage_est; //для расчета задержки шага тех. графика
  TFlightCargos cargos;
  TFlightMaxCommerce max_commerce;
  TFlightStations stations;
  std::string key;
  TPointsDest() {
    status = tdUpdate;
    point_id = ASTRA::NoExists;
    point_num = ASTRA::NoExists;
    pr_tranzit = false;
    first_point = ASTRA::NoExists;
    flt_no = ASTRA::NoExists;
    scd_in = ASTRA::NoExists;
    est_in = ASTRA::NoExists;
    act_in = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    est_out = ASTRA::NoExists;
    act_out = ASTRA::NoExists;
    pr_del = 0;
    pr_reg = true;
    tid = ASTRA::NoExists;
    airp_fmt = efmtUnknown;
    airline_fmt = efmtUnknown;
    suffix_fmt = efmtUnknown;
    craft_fmt = efmtUnknown;
    stage_scd = ASTRA::NoExists;
    stage_est = ASTRA::NoExists;
    comp_id = ASTRA::NoExists;
  }
  void getDestData( TQuery &Qry );
  void Load( int vpoint_id, BitSet<TUseDestData> FUseData );
  void LoadProps( int vpoint_id, BitSet<TUseDestData> FUseData );
  void getEvents( const TPointsDest &vdest );
  void setRemark( const TPointsDest &dest );
  void DoEvents( int move_id, const TPointsDest &dest );
};

class TPointDests {
  private:
  public:
    std::vector<TPointsDest> items;
    void Load( int move_id, BitSet<TUseDestData> FUseData );
    //возвращаем new_dests с заданными point_id
    void sychDests( TPointDests &new_dests, bool pr_change_dests, bool pr_compare_date ); // возвращаем изменение в объекте, но не синхронизируем по существующим строкам. Надо делать отдельно
};

template <typename T> class KeyTrip {
  public:
    T key;
    std::vector<T> dests;
    void push_back( const T &vkey ) {
      dests.push_back( vkey );
    }
    KeyTrip( const T &vkey ) {
      key = vkey;
    }
};

template <typename T> class PointsKeyTrip : public KeyTrip<T> {
  private:
    BitSet<TTripEvents> events;
  public:
    void getEvents( KeyTrip<T> &trip );
    void DoEvents( int move_id );
    bool isNeedChangeComps() {
      return (events.isFlag( teNeedChangeComps ));
    }
    PointsKeyTrip( const T &vkey ):KeyTrip<T>( vkey ) {};
};

class TPoints {
private:
  int getPoint_id();
  void WriteDest( TPointsDest &dest );  //работает без использования Lock()
public:
  BitSet<TPointsEvents> events;
  TPointsEvents status;
  int move_id;
  std::string ref;
  TPointDests dests;
  TPoints() {
    status = peInsert;
    move_id = ASTRA::NoExists;
  }
  void Verify( bool ignoreException, AstraLocale::LexemaData &lexemaData );
  void Save( bool isShowMsg );
  void SaveCargos();
  static bool isDouble( int move_id, std::string airline, int flt_no,
	                      std::string suffix, std::string airp,
	                      BASIC::TDateTime scd_in, BASIC::TDateTime scd_out );
  static bool isDouble( int move_id, const TPointsDest &dest ) {
    return isDouble( move_id, dest.airline, dest.flt_no,
                     dest.suffix, dest.airp, dest.scd_in, dest.scd_out );
  }
  static bool isDouble( int move_id, std::string airline, int flt_no,
	                      std::string suffix, std::string airp,
	                      BASIC::TDateTime scd_in, BASIC::TDateTime scd_out,
                        int &findMove_id, int &point_id );
  static bool isDouble( int move_id, const TPointsDest &dest,
                        int &findMove_id, int &point_id ) {
    return isDouble( move_id, dest.airline, dest.flt_no,
                     dest.suffix, dest.airp, dest.scd_in, dest.scd_out,
                     findMove_id, point_id );
  }
};

void ConvertSOPPToPOINTS( int move_id, const TSOPPDests &dests, std::string reference, TPoints &points );
void WriteDests( TPoints &points, bool ignoreException,
                 XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
void parseFlt( const std::string &value, std::string &airline, int &flt_no, std::string &suffix );
void ReBindTlgs( int move_id, const std::vector<int> &oldPointsId );

struct TFndFlt {
  int point_id;
  int move_id;
  int pr_del;
};

typedef std::vector<TFndFlt> TFndFlts;

bool findFlt( const std::string &airline, const int &flt_no, const std::string &suffix,
              const BASIC::TDateTime &local_scd_out, const std::string &airp, const int &withDeleted,
              TFndFlts &flts );

enum TFlightType { ftTranzit, ftAll };

class FlightPoints:public std::vector<TTripRouteItem> {
  private:
  public:
    int point_dep;
    int point_arv;
    void Get( int vpoint_dep );
    bool inRoute( int point_id, bool with_land ) {
      for ( FlightPoints::const_iterator ipoint=begin();
            ipoint!=end(); ipoint++ ) {
        if ( ipoint->point_id == point_id ) {
          return ( ipoint->point_id != point_arv || with_land );
        }
      }
      return false;
    }
    FlightPoints() {
      point_dep = ASTRA::NoExists;
      point_arv = ASTRA::NoExists;
    }
};

class TFlights:public std::vector<FlightPoints> {
  public:
    void Get( const std::vector<int> &points, TFlightType flightType );
    void Get( int point_dep, TFlightType flightType ) {
      std::vector<int> points( 1, point_dep );
      Get( points, flightType );
    }
    void Lock();
};


#endif /*_POINTS_H_*/
