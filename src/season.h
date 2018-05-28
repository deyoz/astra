#ifndef _SEASON_H_
#define _SEASON_H_

#include <bitset>
#include "date_time.h"
#include "astra_elems.h"
#include "astra_consts.h"
#include "jxtlib/JxtInterface.h"
#include "oralib.h"

namespace SEASON {

using BASIC::date_time::TDateTime;

  struct TDest {
    int num;
    std::string airp;
    TElemFmt airp_fmt;
    std::string city;
    TElemFmt city_fmt;
    int pr_del;
    TDateTime scd_in;
    std::string airline;
    TElemFmt airline_fmt;
    std::string region;
    int trip;
    std::string craft;
    TElemFmt craft_fmt;
    std::string litera;
    std::string triptype;
    TDateTime scd_out;
    int f;
    int c;
    int y;
    std::string unitrip;
    std::string suffix;
    TElemFmt suffix_fmt;
    TDateTime diff;
    TDest() {
      diff = 0.0;
      num = ASTRA::NoExists;
    }
  };


/////////////////////////////////SSM//////////////////////////////////////////////////////
  struct SSIMFlight {
    std::string airline;
    int flt_no;
    std::string suffix;
  };

  struct SSIMPeriod {
    TDateTime first;
    TDateTime last;
    std::string days;
    SSIMPeriod( ) {}
    SSIMPeriod( const TDateTime &vfirst, const TDateTime &vlast, const std::string &vdays ) {
      first = vfirst;
      last = vlast;
      days = vdays;
    }
  };

  struct SSIMSection
  {
      std::string from, to;
      boost::posix_time::time_duration arr, dep;
  };


  struct SSIMLeg
  {
      SSIMSection section;
      std::string craft;
      SSIMLeg(const SSIMSection& vsection, std::string vcraft) {
        section = vsection;
        craft = vcraft;
      }
  };

  using SSIMLegs = std::vector<SSIMLeg>;

  struct SSIMRoute {
    void FromDB( int move_id );
    SSIMLegs legs;
    //SSIMSegmentsProps segProps;
  };

  struct SSIMScdPeriod {
    SSIMFlight flight;
    SSIMPeriod period;
    SSIMRoute route;
    SSIMScdPeriod(const SSIMFlight& cflight) {
      flight = cflight;
    }
  };

  class SSIMScdPeriods: public std::vector<SSIMScdPeriod> {
    SSIMScdPeriods( ) {
      trip_id = ASTRA::NoExists;
    }

    int trip_id;
   public:
    void fromDB( const SSIMFlight &flight, const SSIMPeriod &period );
    void toDB( const SSIMFlight &flight, const SSIMPeriod &period );
  };
////////////////////////////END SSM //////////////////////////////////////////////////////
struct TViewTrip {
	int trip_id;
	int move_id;
	std::string name;
	std::string crafts;
	std::string ports;
	TDateTime scd_in;
	TDateTime scd_out;
	TDateTime first;
	TDateTime last;
	std::string days;
	bool pr_del;
	TViewTrip() {
	  first = ASTRA::NoExists;
	}
};

struct TViewPeriod {
	int trip_id;
	std::string exec;
	std::string noexec;
	std::vector<TViewTrip> trips;
};

void ReadTripInfo( int trip_id, std::vector<TViewPeriod> &viewp, xmlNodePtr reqNode );

}

void CreateSPP( TDateTime localdate );

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
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Slots);
     AddEvent("season_slots",evHandle);
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
  void Slots(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Edit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ViewSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

std::string GetCityFromAirp( std::string &airp );
//std::string GetTZRegion( std::string &city, std::map<std::string,std::string> &regions, bool vexcept=1 );
std::string AddDays( std::string days, int delta );

class TDoubleTrip
{
	private:
		 TQuery *Qry;
	public:
		 TDoubleTrip();
		 ~TDoubleTrip();
     bool IsExists( int move_id, std::string airline, int flt_no,
                      std::string suffix, std::string airp,
                          TDateTime scd_in, TDateTime scd_out,
                    int &point_id );
};



#endif
