#ifndef _SOPP_H_
#define _SOPP_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include "JxtInterface.h"
#include "basic.h"
#include "astra_utils.h"
#include "astra_consts.h"

struct Cargo {
	int cargo;
	int mail;
	int point_arv;
	std::string airp_arv;
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
	int excess;
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
	  excess = 0;
	}
};

struct Luggage {
	int pr_edit;
	std::string region;

	std::vector<PaxLoad> vpaxload;
	int max_commerce;
	std::vector<Cargo> vcargo;
	Luggage() {
		pr_edit = 0;
		max_commerce = 0;
	}
};


void GetLuggage( int point_id, Luggage &lug, bool pr_brd );
void GetLuggage( int point_id, Luggage &lug );

struct TSOPPDelay {
	std::string code;
	BASIC::TDateTime time;
};

struct TSOPPDest {
	bool modify;
  int point_id;
  int point_num;
  std::string airp;
  int airp_fmt;
  std::string city;
  int first_point;
  std::string airline;
  int airline_fmt;
  int flt_no;
  std::string suffix;
  int suffix_fmt;
  std::string craft;
  int craft_fmt;
  std::string bort;
  BASIC::TDateTime scd_in;
  BASIC::TDateTime est_in;
  BASIC::TDateTime act_in;
  BASIC::TDateTime scd_out;
  BASIC::TDateTime est_out;
  BASIC::TDateTime act_out;
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
};

struct TSoppStage {
  int stage_id;
  BASIC::TDateTime scd;
  BASIC::TDateTime est;
  BASIC::TDateTime act;
  bool pr_auto;
  bool pr_manual;
};

struct TSOPPStation {
  std::string name;
  std::string work_mode;
  bool pr_main;
};
typedef std::vector<TSOPPStation> tstations;


enum TTrferType { trferIn, trferOut, trferCkin };
typedef std::vector<TSOPPDest> TSOPPDests;
struct TSOPPTrip {
  int tid;
  int move_id;
  int point_id;

  std::string ref;

  std::string airline_in;
  int airline_in_fmt;
  int flt_no_in;
  std::string suffix_in;
  int suffix_in_fmt;
  std::string craft_in;
  int craft_in_fmt;
  std::string bort_in;
  BASIC::TDateTime scd_in;
  BASIC::TDateTime est_in;
  BASIC::TDateTime act_in;
  std::string triptype_in;
  std::string litera_in;
  std::string park_in;
  std::string remark_in;
  int pr_del_in;
  TSOPPDests places_in;

  std::string airp;
  int airp_fmt;
  std::string city;

  std::string airline_out;
  int airline_out_fmt;
  int flt_no_out;
  std::string suffix_out;
  int suffix_out_fmt;
  std::string craft_out;
  int craft_out_fmt;
  std::string bort_out;
  BASIC::TDateTime scd_out;
  BASIC::TDateTime est_out;
  BASIC::TDateTime act_out;
  std::string triptype_out;
  std::string litera_out;
  std::string park_out;
  std::string remark_out;
  int pr_del_out;
  int pr_reg;
  TSOPPDests places_out;
  std::vector<TSOPPDelay> delays;

  int pr_del;

  std::string classes;
  int reg;
  int resa;
  std::vector<TSoppStage> stages;
  tstations stations;
  std::string crs_disp_from;
  std::string crs_disp_to;

  std::string region;
  int trfer_out_point_id;
  BitSet<TTrferType> TrferType;

  TSOPPTrip() {
    flt_no_in = ASTRA::NoExists;
    scd_in = ASTRA::NoExists;
    est_in = ASTRA::NoExists;
    act_in = ASTRA::NoExists;
    pr_del_in = -1;

    flt_no_out = ASTRA::NoExists;
    scd_out = ASTRA::NoExists;
    est_out = ASTRA::NoExists;
    act_out = ASTRA::NoExists;
    pr_del_out = -1;
    pr_del = 0;
    reg = 0;
    resa = 0;
    pr_reg = 0;
    trfer_out_point_id = -1;
    TrferType.clearFlags();
  }
};

typedef std::vector<TSOPPTrip> TSOPPTrips;

void createSOPPTrip( int point_id, TSOPPTrips &trips );

class SoppInterface : public JxtInterface
{
private:
public:
  SoppInterface() : JxtInterface("","sopp")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::ReadTrips);
     AddEvent("ReadTrips",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetPaxTransfer);
     AddEvent("GetPaxTransfer",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::GetBagTransfer);
     AddEvent("GetBagTransfer",evHandle);
     evHandle=JxtHandler<SoppInterface>::CreateHandler(&SoppInterface::DeleteAllPassangers);
     AddEvent("DeleteAllPassangers",evHandle);
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
  };
  void ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, bool pr_bag);
  void GetPaxTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetBagTransfer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
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
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif /*_SOPP_H_*/

