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
                  
                  
enum TUseDestData { udDelays, udCargo, udMaxCommerce, udNum };
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
                   dmChangeStageESTTime, dmPoint_Num };
                   
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
                   teChangeCargos, teChangeMaxCommerce };
                   
class TPointsDestDelay {
public:
	std::string code;
	BASIC::TDateTime time;
};

struct TPointsDestCargo {
	int cargo;
	int mail;
	int point_arv;
	std::string key;
	std::string airp_arv;
	TElemFmt airp_arv_fmt;
	int dosbag_weight;
	TPointsDestCargo() {
		cargo = 0;
		mail = 0;
		point_arv = ASTRA::NoExists;
	}
};


class TPointsDest {
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
  TMapTripStages stages;
  std::vector<TPointsDestDelay> delays;
  BASIC::TDateTime stage_scd, stage_est; //для расчета задержки шага тех. графика
  std::vector<TPointsDestCargo> cargos;
  int max_commerce;
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
    max_commerce = ASTRA::NoExists;
    comp_id = ASTRA::NoExists;
  }
  void Load( int vpoint_id, BitSet<TUseDestData> FUseData );
  void getEvents( const TPointsDest &vdest );
  void setRemark( const TPointsDest &dest );
  void DoEvents( int move_id, const TPointsDest &dest );
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
public:
  BitSet<TPointsEvents> events;
  TPointsEvents status;
  int move_id;
  std::string ref;
  std::vector<TPointsDest> dests;
  TPoints() {
    status = peInsert;
    move_id = ASTRA::NoExists;
  }
  void Load( int vmove_id ) {};
  void Verify( bool ignoreException, AstraLocale::LexemaData &lexemaData );
  void WriteDest( TPointsDest &dest );
  void Save( bool isShowMsg );
  void SaveCargos();
  static bool isDouble( int move_id, std::string airline, int flt_no,
	                      std::string suffix, std::string airp,
	                      BASIC::TDateTime scd_in, BASIC::TDateTime scd_out );
};

void ConvertSOPPToPOINTS( int move_id, const TSOPPDests &dests, std::string reference, TPoints &points );
void WriteDests( TPoints &points, bool ignoreException,
                 XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
void parseFlt( const std::string &value, std::string &airline, int &flt_no, std::string &suffix );

struct TFndFlt {
  int point_id;
  int move_id;
  int pr_del;
};

typedef std::vector<TFndFlt> TFndFlts;

bool findFlt( const std::string &airline, const int &flt_no, const std::string &suffix,
              const BASIC::TDateTime &local_scd_out, const std::string &airp, const int &withDeleted,
              TFndFlts &flts );
void lockPoints( int move_id );
#endif /*_POINTS_H_*/
