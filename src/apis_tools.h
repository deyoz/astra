#ifndef APIS_TOOLS_H
#define APIS_TOOLS_H

#include <string>
#include <vector>
#include <basic.h>
#include "xml_unit.h"

class FlightLeg {
private:
  int loc_qualifier;
  std::string airp;
  std::string country;
  BASIC::TDateTime sch_in;
  BASIC::TDateTime sch_out;
public:
  FlightLeg (std::string airp, std::string country, BASIC::TDateTime sch_in, BASIC::TDateTime sch_out):
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

#endif // APIS_TOOLS_H
