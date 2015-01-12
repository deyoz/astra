#ifndef EXTERNAL_SPP_SYNCH_H
#define EXTERNAL_SPP_SYNCH_H

#include <string>
#include "basic.h"
#include "jxtlib/JxtInterface.h"
#include "http_main.h"
#include "stl_utils.h"
#include "astra_elems.h"

class HTTPRequestsIface : public JxtInterface
{
public:
  HTTPRequestsIface() : JxtInterface("", SPP_SYNCH_JXT_INTERFACE_ID)
  {
     Handler *evHandle;
     // Расширенный поиск пассажиров
     evHandle=JxtHandler<HTTPRequestsIface>::CreateHandler(&HTTPRequestsIface::SaveSPP);
     AddEvent("SaveSPP",evHandle);
  }
  void SaveSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};

struct FlightProperty {
  std::string name;
  std::string value;
  FlightProperty(const std::string &vname,
                 const std::string &vvalue) {
    name = upperc( vname );
    name = TrimString( name );
    value = vvalue;
    value = TrimString( value );
  }
};

const std::string FormatFlightDateTime = "yyyy-mm-dd hh:nn:ss";

struct TCode {
  std::string code;
  TElemFmt fmt;
  TCode() {
    clear();
  }
  void clear() {
    code.clear();
    fmt = efmtUnknown;
  }
};

struct  TParseFlight {
  std::string own_region;
  std::string own_airp;
  std::string error;
  TCode airline;
  std::string fltNo;
  int flt_no;
  TCode suffix;
  std::string trip_type;
  BASIC::TDateTime scd;
  BASIC::TDateTime est;
  TCode craft;
  std::vector<TCode> airps_in;
  std::vector<TCode> airps_out;
  bool pr_landing;
  std::string status;
  std::string record;
  void add_airline( const std::string &value );
  void add_fltno( const std::string &value );
  void add_scd( const std::string &value );
  void add_est( const std::string &value );
  void add_craft( const std::string &value );
  void add_dests( const std::string &value );
  void add_status( const std::string &value ) {
    status = upperc( value );
  }
  void add_prlanding( const std::string &value ) {
    std::string tmp_value = upperc( value );
    pr_landing = ( tmp_value != "ВЗЛЕТ" );
  }
  TParseFlight& operator << (const FlightProperty &prop);
  void clear();
  TParseFlight( const std::string &airp ) {
    std::string city =((TAirpsRow&)base_tables.get("airps").get_row( "code", airp, true )).city;
    own_region = ((TCitiesRow&)base_tables.get("cities").get_row( "code", city, true )).region;
    own_airp = airp;
  }
  bool is_valid() {
    return error.empty();
  }
  std::string key() const {
    std::string res = airline.code + IntToString( flt_no ) + suffix.code + BASIC::DateTimeToStr( scd, "dd" );
    return res;
  }
};

void saveFlights( std::map<std::string,std::map<bool, TParseFlight> > &flights );

#endif // EXTERNAL_SPP_SYNCH_H
