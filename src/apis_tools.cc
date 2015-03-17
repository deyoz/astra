#include "apis_tools.h"
#include "oralib.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "exceptions.h"
#include "astra_utils.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

void FlightLeg::toXML(xmlNodePtr FlightLegsNode) const
{
  xmlNodePtr legNode = NewTextChild(FlightLegsNode, "FlightLeg");
  SetProp(legNode, "LocationQualifier", loc_qualifier);
  SetProp(legNode, "Airport", airp);
  SetProp(legNode, "Country", country);
  if (sch_in != ASTRA::NoExists)
    SetProp(legNode, "ArrivalDateTime", BASIC::DateTimeToStr(sch_in, "yyyy-mm-dd'T'hh:nn:00"));
  if (sch_out != ASTRA::NoExists)
    SetProp(legNode, "DepartureDateTime", BASIC::DateTimeToStr(sch_out, "yyyy-mm-dd'T'hh:nn:00"));
}

void FlightLegs::FlightLegstoXML(xmlNodePtr FlightLegsNode) const {
  for (std::vector<FlightLeg>::const_iterator iter=begin(); iter != end(); iter++)
     iter->toXML(FlightLegsNode);
}

void FlightLegs::FillLocQualifier() {
  /* Code set:
  87 : airport initial arrival in target country.
  125: last departure airport before arrival in target country.
  130: final destination airport in target country.
  92: in-transit airport. */
  std::string target_country;
  bool change_flag = false;
  std::vector<FlightLeg>::reverse_iterator previos, next;
  for (previos=rbegin(), (next=rbegin())++; next!=rend(); previos++, next++) {
    if(previos==rbegin()) target_country = previos->Country();
    if(change_flag) next->setLocQualifier(92);
    else if(previos->Country() != next->Country() && previos->Country() == target_country) {
      previos->setLocQualifier(87);
      next->setLocQualifier(125);
      change_flag = true;
    }
    else if(previos==rbegin())
      previos->setLocQualifier(130);
    else previos->setLocQualifier(92);
  }
}

const std::string generate_envelope_id (const std::string& airl)
{
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  std::stringstream ss;
  ss << uuid;
  std::string res = airl + std::string("-") + ss.str();
  return airl + std::string("-") + ss.str();
}

const std::string get_msg_identifier ()
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT apis_id__seq.nextval vid FROM dual";
  Qry.Execute();
  std::stringstream ss;
  ss << std::string("ASTRA") << std::setw(7) << std::setfill('0') << Qry.FieldAsString("vid");
  return ss.str();
}

bool get_trip_apis_param (const int point_id, const std::string& format, const std::string& param_name, int& param_value)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT TO_NUMBER(param_value) AS param_value "
    "FROM trip_apis_params "
    "WHERE point_id=:point_id AND format=:format "
    "AND param_name=:param_name ";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("format", otString, format);
  Qry.CreateVariable("param_name", otString, param_name);
  Qry.Execute();
  if (Qry.Eof) return false;
  param_value = Qry.FieldAsInteger("param_value");
  return true;
}

void set_trip_apis_param(const int point_id, const std::string& format, const std::string& param_name, const int param_value)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "BEGIN "
    "  UPDATE trip_apis_params SET param_value=TO_CHAR(:param_value) "
    "  WHERE point_id=:point_id AND format=:format "
    "  AND param_name=:param_name; "
    "  IF SQL%ROWCOUNT=0 THEN "
    "    INSERT INTO trip_apis_params(point_id, format, param_name, param_value)"
    "    VALUES (:point_id, :format, :param_name, TO_CHAR(:param_value));"
    "  END IF; "
    "END;";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("format", otString, format);
  Qry.CreateVariable("param_name", otString, param_name);
  Qry.CreateVariable("param_value", otInteger, param_value);
  try
  {
    Qry.Execute();
  }
  catch(EOracleError E)
  {
    if (E.Code!=1) throw;
  };
}
