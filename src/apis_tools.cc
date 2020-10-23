#include "apis_tools.h"
#include "oralib.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "astra_misc.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC::date_time;

void FlightLeg::toXML(xmlNodePtr FlightLegsNode) const
{
  xmlNodePtr legNode = NewTextChild(FlightLegsNode, "FlightLeg");
  SetProp(legNode, "LocationQualifier", loc_qualifier);
  SetProp(legNode, "Airport", airp);
  SetProp(legNode, "Country", country);
  if (sch_in != ASTRA::NoExists)
    SetProp(legNode, "ArrivalDateTime", DateTimeToStr(sch_in, "yyyy-mm-dd'T'hh:nn:00"));
  if (sch_out != ASTRA::NoExists)
    SetProp(legNode, "DepartureDateTime", DateTimeToStr(sch_out, "yyyy-mm-dd'T'hh:nn:00"));
}

void FlightLegs::FlightLegstoXML(xmlNodePtr FlightLegsNode) const {
  for (std::vector<FlightLeg>::const_iterator iter=begin(); iter != end(); iter++)
     iter->toXML(FlightLegsNode);
}

void FlightLegs::FillLocQualifier()
{
  /* Code set:
  87 : airport initial arrival in target country.
  125: last departure airport before arrival in target country.
  130: final destination airport in target country.
  92: in-transit airport. */
  std::string target_country;
  bool change_flag = false;
  std::vector<FlightLeg>::reverse_iterator previous, next;

  for (previous=rbegin(), (next=rbegin())++; next!=rend(); previous++, next++)
  {
    if (previous==rbegin())
      target_country = previous->Country();

    if(change_flag)
      next->setLocQualifier(92);
    else

      if (previous->Country() != next->Country() && previous->Country() == target_country)
      {
        previous->setLocQualifier(87);
        next->setLocQualifier(125);
        change_flag = true;
      }
      else
      {
        next->setLocQualifier(87); // ��ࠢ���� APIS_TEST
        if (previous==rbegin())
          previous->setLocQualifier(130);
        else
          previous->setLocQualifier(92);
      }
  }
}

const std::string generate_envelope_id (const std::string& airl)
{
#if APIS_TEST
  return airl + std::string("-") + std::string("TEST_ENVELOPE_ID");
#endif
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  std::stringstream ss;
  ss << uuid;
  return airl + std::string("-") + ss.str();
}

const std::string get_msg_identifier ()
{
#if APIS_TEST
  return "TEST_MSG_ID";
#endif
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
  catch(const EOracleError& E)
  {
    if (E.Code!=1) throw;
  };
}

void getTBTripItem(const int point_dep, const int point_arv, const std::string& country,
                   std::string& tb_date, std::string& tb_time, std::string& tb_airp )

{
  TAdvTripRoute route, tmp;
  route.GetRouteBefore( ASTRA::NoExists, point_dep, trtWithCurrent, trtNotCancelled );
  tmp.GetRouteAfter( ASTRA::NoExists, point_dep, trtNotCurrent, trtNotCancelled );
  route.insert( route.end(), tmp.begin(), tmp.end() );

  std::string airp;
  TDateTime date_time = ASTRA::NoExists;
  if ( getCountryByAirp(route.front().airp).code_lat != country ) {
    // inbound flight
    for(TAdvTripRoute::const_iterator it=route.begin(); it->point_id!=point_arv; it++) {
      if (getCountryByAirp(it->airp).code_lat == country) {
        airp = it->airp;
        date_time = it->scd_in;
      }
    }
  }
  else {
    for(TAdvTripRoute::const_reverse_iterator rit=route.rbegin(); rit->point_id!=point_dep; rit++) {
      // outbound flight
      if (getCountryByAirp(rit->airp).code_lat == country) {
        airp = rit->airp;
        date_time = rit->scd_out;
      };
    }
  }

  if ( !airp.empty() && date_time!=ASTRA::NoExists ) {
    const TAirpsRow& airp_row = (const TAirpsRow&)base_tables.get("airps").get_row("code",airp);
    TDateTime date_time_local = UTCToLocal(date_time,AirpTZRegion(airp_row.code));
    tb_date = DateTimeToStr(date_time_local,"dd.mm.yyyy");
    tb_time = DateTimeToStr(date_time_local,"hh:nn");
    tb_airp = airp_row.code_lat;
  }
}

std::string getTripType( ASTRA::TPaxStatus status, const int grp_id, const std::string& direction, const std::string& apis_country )
{
  if (status == ASTRA::psTransit)
    return "T";

  if (direction == "O") {
    // �஢�ਬ �室�騩 �࠭���
    auto prior=TCkinRoute::getPriorGrp(GrpId_t(grp_id),
                                       TCkinRoute::IgnoreDependence,
                                       TCkinRoute::WithoutTransit);
    if ( prior && getCountryByAirp( prior.get().airp_dep ).code_lat != apis_country )
      return "X";
  }
  else {
    // �஢�ਬ ��室�騩 �࠭���
    auto next=TCkinRoute::getNextGrp(GrpId_t(grp_id),
                                     TCkinRoute::IgnoreDependence,
                                     TCkinRoute::WithoutTransit);
    if ( next && getCountryByAirp( next.get().airp_arv ).code_lat != apis_country )
      return "X";
  }
  return "N";
}
