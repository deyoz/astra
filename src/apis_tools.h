#ifndef APIS_TOOLS_H
#define APIS_TOOLS_H

#include <string>
#include <vector>
#include "date_time.h"
#include "xml_unit.h"
#include "astra_misc.h"
#include "astra_consts.h"

using BASIC::date_time::TDateTime;

class FlightLeg {
private:
  int loc_qualifier;
  std::string airp;
  std::string country;
  TDateTime sch_in;
  TDateTime sch_out;
public:
  FlightLeg (std::string airp, std::string country, TDateTime sch_in, TDateTime sch_out):
    airp(airp), country(country), sch_in(sch_in), sch_out(sch_out) {}
  void setLocQualifier(const int value) {loc_qualifier = value; }
  const std::string Country() { return country; }
  void toXML(xmlNodePtr FlightLegsNode) const;
};

class FlightLegs : public std::vector<FlightLeg> {
  public:
  void FlightLegstoXML(xmlNodePtr FlightLegsNode) const;
  void FillLocQualifier();
};

const std::string generate_envelope_id (const std::string& airl);
const std::string get_msg_identifier ();
bool get_trip_apis_param (const int point_id, const std::string& format, const std::string& param_name, int& param_value);
void set_trip_apis_param(const int point_id, const std::string& format, const std::string& param_name, const int param_value);
void getTBTripItem( const int point_dep, const int point_arv, const std::string& country,
                    std::string& tb_date, std::string& tb_time, std::string& tb_airp );
std::string getTripType( ASTRA::TPaxStatus status, const int grp_id, const std::string& direction,
                         const std::string& apis_country );

#endif // APIS_TOOLS_H
