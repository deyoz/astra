#ifndef _SEASON_H_
#define _SEASON_H_

#include <libxml/tree.h>
#include <string>
#include <map>
#include "basic.h"
#include "JxtInterface.h"

namespace SEASON {
	
struct TViewTrip {
	int trip_id;
	int move_id;
	std::string name;
	std::string crafts;
	std::string ports;
	BASIC::TDateTime land;
	BASIC::TDateTime takeoff;
};

struct TViewPeriod {
	int trip_id;
	std::string exec;
	std::string noexec;
	std::vector<TViewTrip> trips;
};

void ReadTripInfo( int trip_id, std::vector<TViewPeriod> &viewp, xmlNodePtr reqNode );

}

void CreateSPP( BASIC::TDateTime localdate );
	
class SeasonInterface : public JxtInterface
{
public:
  SeasonInterface() : JxtInterface("123","season")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Filter);
     AddEvent("filter",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Read);
     AddEvent("season_read",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Write);
     AddEvent("write",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::GetSPP);
     AddEvent("get_spp",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::DelRangeList);
     AddEvent("del_range_list",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Edit);
     AddEvent("edit",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::ViewSPP);
     AddEvent("spp",evHandle);
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::convert);
     AddEvent("convert",evHandle);     
  };

  void Filter(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Edit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

std::string GetCityFromAirp( std::string &airp );
std::string GetTZRegion( std::string &city, std::map<std::string,std::string> &regions, bool vexcept=1 );


#endif
