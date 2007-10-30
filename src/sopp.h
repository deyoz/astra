#ifndef _SOPP_H_
#define _SOPP_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include "JxtInterface.h"

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
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif /*_SOPP_H_*/

