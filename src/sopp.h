#ifndef _SOPP_H_
#define _SOPP_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <iostream>
#include "xml_unit.h"
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "alarms.h"


using BASIC::date_time::TDateTime;

enum TTrip_Calc_Data { tDesksGates, tTrferExists };

struct Cargo {
    int cargo;
    int mail;
    int point_arv;
    std::string airp_arv;
    TElemFmt airp_arv_fmt;
    int dosbag_weight;
    Cargo() {
        cargo = 0;
        mail = 0;
        point_arv = 0;
    }
};

struct PaxLoad {
    std::string cl;
    int point_arv;
    int seatsadult;
    int seatschild;
    int seatsbaby;
    int bag_weight;
    int rk_weight;
    int adult;
    int child;
    int baby;
    PaxLoad() {
        point_arv = 0;
        seatsadult = 0;
        seatschild = 0;
        seatsbaby = 0;
      bag_weight = 0;
      rk_weight = 0;
      adult = 0;
      child = 0;
      baby = 0;
    }
};

struct Luggage {
    int pr_edit;
    TDateTime scd_out;
    std::string region;

    std::vector<PaxLoad> vpaxload;
    int max_commerce;
    std::vector<Cargo> vcargo;
    Luggage() {
    scd_out = ASTRA::NoExists;
        pr_edit = 0;
        max_commerce = 0;
    }
};


struct TSOPPDelay {
    std::string code;
    TDateTime time;
};

struct TSOPPDest {
  bool modify;
  int point_id;
  int point_num;
  std::string airp;
  TElemFmt airp_fmt;
  std::string city;
  int first_point;
  std::string airline;
  TElemFmt airline_fmt;
  int flt_no;
  std::string suffix;
  TElemFmt suffix_fmt;
  std::string craft;
  TElemFmt craft_fmt;
  std::string bort;
  std::string commander;
  int cockpit, cabin;
  TDateTime scd_in;
  TDateTime est_in;
  TDateTime act_in;
  TDateTime scd_out;
  TDateTime est_out;
  TDateTime act_out;
  TDateTime part_key;
  std::vector<TSOPPDelay> delays;
  std::string triptype;
  std::string litera;
  std::string park_in;
  std::string park_out;
  std::string remark;
  int pr_tranzit;
  int pr_reg;
  int pr_del;
  int tid;
  std::string region;
  TSOPPDest(): cockpit(0), cabin(0) {}
  std::string toString() const;
  void fromDB( TQuery &Qry );
};

struct TSoppStage {
  int stage_id;
  TDateTime scd;
  TDateTime est;
  TDateTime act;
  bool pr_auto;
  bool pr_manual;
  bool pr_permit;
};

const static std::string GATE_WORK_MODE = "?";
const static std::string TERM_WORK_MODE = "?";

struct TSOPPStation {
  std::string name;
  std::string work_mode;
  bool pr_main;
  bool pr_del;
  TSOPPStation() {
    clear();
  }
  TSOPPStation( const std::string &vname, const std::string &vwork_mode ) {
    clear();
    name = vname;
    work_mode = vwork_mode;
  }
  TSOPPStation( const std::string &vname,
                const std::string &vwork_mode,
                bool vpr_main ) {
    clear();
    name = vname;
    work_mode = vwork_mode;
    pr_main = vpr_main;
  }
  void clear() {
    name.clear();
    work_mode.clear();
    pr_main = false;
    pr_del = false;
  }
  bool operator < ( const TSOPPStation& station ) const {
    if ( name != station.name )
      return name < station.name;
    if ( work_mode != station.work_mode )
      return work_mode < station.work_mode;
    return pr_del < station.pr_del;
  }
  bool operator == ( const TSOPPStation& station ) const {
    return ( name == station.name ) &&
           ( work_mode == station.work_mode ) &&
           ( pr_main == station.pr_main ) &&
           ( pr_del == station.pr_del );
  }
  static bool isTerm( const std::string& work_mode ) {
    return work_mode == TERM_WORK_MODE;
  }
  static bool isGate( const std::string& work_mode ) {
    return work_mode == GATE_WORK_MODE;
  }
  bool isTerm( ) const {
    return isTerm( work_mode );
  }
  bool isGate() const {
    return  isGate( work_mode );
  }
};


class tstations:public std::vector<TSOPPStation>
{
  public:
  enum toDbMode { dbStandart = 0, // ?????????????? ?????? ?????????
                  dbRewriteAll, //?????????????? ???
                  dbWriteReceiveChanged //???????? ?????? ??????????
                };
  enum toDBModeRewriteAll {
                  dbRewriteAll_Delete_Terms,
                  dbRewriteAll_Delete_Gates
                           };
  static void fromString( const std::string& value, const std::string& work_mode, tstations &stations ) {
    std::istringstream iss(value);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    for ( const auto &t : tokens ) {
      stations.emplace_back( TSOPPStation(t,work_mode) );
    }
  }
  std::string toString() {
    std::string res = TERM_WORK_MODE + ": ";
    res += toString( *this, TERM_WORK_MODE );
    res += ", " + GATE_WORK_MODE + ": ";
    res += toString( *this, GATE_WORK_MODE );
    return res;
  }
  static std::string toString( const tstations &stations, const std::string& work_mode ) {
    std::string res;
    for ( const auto &t : stations ) {
      if ( t.work_mode != work_mode ) {
        continue;
      }
      if ( !res.empty() ) {
        res += " ";
      }
      res += t.name;
    }
    return res;
  }
  void toXML( xmlNodePtr node ) const {
    xmlNodePtr n = empty()?nullptr:NewTextChild( node, "stations" );
    for ( const auto &st : *this  ) {
      SetProp( NewTextChild( n, "station", st.name ), "work_mode", st.work_mode );
    }
  }
  void fromDB( int point_id, const std::string& work_mode = "" );
  void fromDBGates( int point_id ) {
    fromDB( point_id, GATE_WORK_MODE );
  }

  void toDB( const std::string &whereabouts, int point_id, toDbMode mode, const BitSet<toDBModeRewriteAll> &flags = BitSet<toDBModeRewriteAll>() );
};

enum TTrferType { trferIn, trferOut, trferCkin };
typedef std::vector<TSOPPDest> TSOPPDests;
struct TSOPPTrip {
  int tid;
  int move_id;
  int point_id;

  std::string ref;

  std::string airline_in;
  TElemFmt airline_in_fmt;
  int flt_no_in;
  std::string suffix_in;
  TElemFmt suffix_in_fmt;
  std::string craft_in;
  TElemFmt craft_in_fmt;
  std::string bort_in;
  std::string commander_in;
  int cockpit_in;
  int cabin_in;
  TDateTime scd_in;
  TDateTime est_in;
  TDateTime act_in;
  std::string triptype_in;
  std::string litera_in;
  std::string park_in;
  std::string remark_in;
  int pr_del_in;
  TSOPPDests places_in;

  std::string airp;
  TElemFmt airp_fmt;
  std::string city;

  std::string airline_out;
  TElemFmt airline_out_fmt;
  int flt_no_out;
  std::string suffix_out;
  TElemFmt suffix_out_fmt;
  std::string craft_out;
  TElemFmt craft_out_fmt;
  std::string bort_out;
  std::string commander_out;
  int cockpit_out;
  int cabin_out;
  TDateTime scd_out;
  TDateTime est_out;
  TDateTime act_out;
  TDateTime part_key;
  std::string triptype_out;
  std::string litera_out;
  std::string park_out;
  std::string remark_out;
  int pr_del_out;
  int pr_reg;
  TSOPPDests places_out;
  std::vector<TSOPPDelay> delays;

  int pr_del;

  std::vector<TCFGItem> cfg;
  int reg;
  int resa;
  std::vector<TSoppStage> stages;
  tstations stations;
  std::string crs_disp_from;
  std::string crs_disp_to;

  std::string region;
  int trfer_out_point_id;
  BitSet<TTrferType> TrferType;
  BitSet<Alarm::Enum> Alarms;

  TSOPPTrip() {
    flt_no_in = ASTRA::NoExists;
    scd_in = ASTRA::NoExists;
    est_in = ASTRA::NoExists;
    act_in = ASTRA::NoExists;
    cockpit_in = 0;
    cabin_in = 0;
    pr_del_in = -1;

    flt_no_out = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    est_out = ASTRA::NoExists;
    act_out = ASTRA::NoExists;
    part_key = ASTRA::NoExists;
    cockpit_out = 0;
    cabin_out = 0;
    pr_del_out = -1;
    pr_del = 0;
    reg = 0;
    resa = 0;
    pr_reg = 0;
    trfer_out_point_id = -1;
    TrferType.clearFlags();
    Alarms.clearFlags();
  }
};

typedef std::vector<TSOPPTrip> TSOPPTrips;

void createSOPPTrip( int point_id, TSOPPTrips &trips );

bool filter_time( TDateTime time, TSOPPTrip &tr, TDateTime first_date, TDateTime next_date, std::string &errcity );
bool FilterFlightDate( TSOPPTrip &tr, TDateTime first_date, TDateTime next_date, /*bool LocalAll,*/
                       std::string &errcity, bool pr_isg );


class TDeletePaxFilter
{
  public:
    std::string status;
    int inbound_point_dep;
    bool with_crew;
    TDeletePaxFilter():inbound_point_dep(ASTRA::NoExists),with_crew(false) {};
};

enum TSoppWriteOwner { ownerDisp, ownerMVT, ownerLDM };

void DeletePassengers( int point_id, const TDeletePaxFilter &filter, std::map<int,TAdvTripInfo> &segs );
void DeletePassengersAnswer( std::map<int,TAdvTripInfo> &segs, xmlNodePtr resNode );
void validateField( const std::string &surname, const std::string &fieldname );
void UpdateCrew( int point_id, std::string commander, int cockpit, int cabin, TSoppWriteOwner owner );

class SoppInterface : public JxtInterface
{
private:
public:
  SoppInterface() : JxtInterface("","sopp")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadTrips);
     AddEvent("ReadTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetTransfer);
     AddEvent("GetPaxTransfer",evHandle);
     AddEvent("GetBagTransfer",evHandle);
     AddEvent("GetInboundTCkin",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::DeleteAllPassangers);
     AddEvent("TCkinDeleteAllPassangers",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteTrips);
     AddEvent("WriteTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadTripInfo);
     AddEvent("ReadTripInfo",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadCRS_Displaces);
     AddEvent("ReadCRS_Displaces",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteCRS_Displaces);
     AddEvent("WriteCRS_Displaces",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadDests);
     AddEvent("ReadDests",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteDests);
     AddEvent("WriteDests",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::DropFlightFact);
     AddEvent("DropFlightFact",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteISGTrips);
     AddEvent("WriteISGTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::DeleteISGTrips);
     AddEvent("DeleteISGTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetReportForm);
     AddEvent("get_report_form",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadCrew);
     AddEvent("ReadCrew",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteCrew);
     AddEvent("WriteCrew",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadDoc);
     AddEvent("ReadDoc",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteDoc);
     AddEvent("WriteDoc",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetTime);
     AddEvent("GetTime",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::CreateAPIS);
     AddEvent("CreateAPIS",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadVoucher);
     AddEvent("ReadVoucher",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::WriteVoucher);
     AddEvent("WriteVoucher",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::readPaxZoneLoad);
     AddEvent("readPaxZoneLoad",evHandle);
  };
  void readPaxZoneLoad(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteAllPassangers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteISGTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadCRS_Displaces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteCRS_Displaces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadDests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteDests(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DropFlightFact(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeleteISGTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetReportForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadCrew(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteCrew(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteDoc(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTime(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CreateAPIS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadVoucher(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void WriteVoucher(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

void ChangeACT_OUT( int point_id, TDateTime old_act, TDateTime act );
void check_TrferExists( int point_id );
void get_DesksGates( int point_id, std::string &ckin_desks, std::string &gates );
void get_DesksGates( int point_id, tstations &stations );
void check_DesksGates( int point_id );
void check_trip_tasks( const TSOPPDests &dests );
void check_trip_tasks( int move_id );
bool CheckApis_USA( const std::string &airp );
void IntReadTrips( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, long int &exec_time );

void set_flight_sets(int point_id, int f=0, int c=0, int y=0);
void updateTransitIfNeeded(const TAdvTripInfo& flt, bool new_pr_tranzit);

void SetFlightFact(int point_id, TDateTime utc_act_out);
void getTripVouchers( int point_id, std::set<std::string> &trip_vouchers );
void ChangeBortFromLDM(const std::string &bort, int point_id);

void changeSCDIN_AtDests( const std::set<int>& points_scd_ins );

void CargoMailWeight(const std::string &prefix, int point_id, int point_arv, int cargo, int mail);

#endif /*_SOPP_H_*/

