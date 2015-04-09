#include "iatci_types.h"

#include <serverlib/exception.h>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

const std::string& OriginatorDetails::airline() const
{
    return m_airline;
}

const std::string& OriginatorDetails::point() const
{
    return m_point;
}

//-----------------------------------------------------------------------------

const std::string& FlightDetails::airline() const
{
    return m_airline;
}

Ticketing::FlightNum_t FlightDetails::flightNum() const
{
    return m_flightNum;
}

const std::string& FlightDetails::depPoint() const
{
    return m_depPoint;
}

const std::string& FlightDetails::arrPoint() const
{
    return m_arrPoint;
}

const boost::gregorian::date& FlightDetails::depDate() const
{
    return m_depDate;
}

const boost::gregorian::date& FlightDetails::arrDate() const
{
    return m_arrDate;
}

const boost::posix_time::time_duration& FlightDetails::depTime() const
{
    return m_depTime;
}

const boost::posix_time::time_duration& FlightDetails::arrTime() const
{
    return m_arrTime;
}

const boost::posix_time::time_duration& FlightDetails::boardingTime() const
{
    return m_boardingTime;
}

std::string FlightDetails::toShortKeyString() const
{
    std::ostringstream os;
    os << m_airline << m_flightNum;
    return os.str();
}

//-----------------------------------------------------------------------------

const std::string& PaxDetails::surname() const
{
    return m_surname;
}

const std::string& PaxDetails::name() const
{
    return m_name;
}

PaxDetails::PaxType_e PaxDetails::type() const
{
    return m_type;
}

std::string PaxDetails::typeAsString() const
{
    switch(m_type)
    {
    case Child:  return "C";
    case Female: return "F";
    case Male:   return "M";
    case Adult:  return "A";
    default:     return "A";
    }
}

const std::string& PaxDetails::qryRef() const
{
    return m_qryRef;
}

const std::string& PaxDetails::respRef() const
{
    return m_respRef;
}

PaxDetails::PaxType_e PaxDetails::strToType(const std::string& s)
{
    if(s == "C") return Child;
    else if(s == "F") return Female;
    else if(s == "M") return Male;
    else if(s == "A") return Adult;
    else
        return Adult;
}

//-----------------------------------------------------------------------------

const std::string& ReservationDetails::rbd() const
{
    return m_rbd;
}

//-----------------------------------------------------------------------------

SeatDetails::SmokeIndicator_e SeatDetails::smokeInd() const
{
    return m_smokeInd;
}

std::string SeatDetails::smokeIndAsString() const
{
    switch(m_smokeInd)
    {
    case NonSmoking:    return "N";
    case PartySeating:  return "P";
    case Smoking:       return "S";
    case Indifferent:   return "X";
    case Unknown:       return "U";
    default:            return "U";
    }
}

const std::list<std::string>& SeatDetails::characteristics() const
{
    return m_characteristics;
}

void SeatDetails::addCharacteristic(const std::string& characteristic)
{
    m_characteristics.push_back(characteristic);
}

SeatDetails::SmokeIndicator_e SeatDetails::strToSmokeInd(const std::string& s)
{
    if(s == "N") return NonSmoking;
    else if(s == "P") return PartySeating;
    else if(s == "S") return Smoking;
    else if(s == "X") return Indifferent;
    else if(s == "U") return Unknown;
    else
        return Unknown;
}

//-----------------------------------------------------------------------------

const std::string& FlightSeatDetails::seat() const
{
    return m_seat;
}

const std::string& FlightSeatDetails::cabinClass() const
{
    return m_cabinClass;
}

const std::string& FlightSeatDetails::securityId() const
{
    return m_securityId;
}

//-----------------------------------------------------------------------------

unsigned BaggageDetails::numOfPieces() const
{
    return m_numOfPieces;
}

unsigned BaggageDetails::weight() const
{
    return m_weight;
}

//-----------------------------------------------------------------------------

const std::string& CascadeHostDetails::originAirline() const
{
    return m_originAirline;
}

const std::string& CascadeHostDetails::originPoint() const
{
    return m_originPoint;
}

const std::list<std::string>& CascadeHostDetails::hostAirlines() const
{
    return m_hostAirlines;
}

void CascadeHostDetails::addHostAirline(const std::string& hostAirline)
{
    m_hostAirlines.push_back(hostAirline);
}

//-----------------------------------------------------------------------------

const std::string& ErrorDetails::errText() const
{
    return m_errText;
}

//-----------------------------------------------------------------------------

Result Result::makeCheckinResult(Status_e status,
                                 const FlightDetails& flight,
                                 boost::optional<PaxDetails> pax,
                                 boost::optional<FlightSeatDetails> seat,
                                 boost::optional<CascadeHostDetails> cascadeDetails,
                                 boost::optional<ErrorDetails> errorDetails)
{
    return Result(Checkin,
                  status,
                  flight,
                  pax,
                  seat,
                  cascadeDetails,
                  errorDetails);
}

Result Result::makeCancelResult(Status_e status,
                                const FlightDetails& flight,
                                boost::optional<PaxDetails> pax,
                                boost::optional<FlightSeatDetails> seat,
                                boost::optional<CascadeHostDetails> cascadeDetails,
                                boost::optional<ErrorDetails> errorDetails)
{
    return Result(Cancel,
                  status,
                  flight,
                  pax,
                  seat,
                  cascadeDetails,
                  errorDetails);
}

Result::Action_e Result::action() const
{
    return m_action;
}

Result::Status_e Result::status() const
{
    return m_status;
}

const iatci::FlightDetails& Result::flight() const
{
    return m_flight;
}

boost::optional<iatci::PaxDetails> Result::pax() const
{
    return m_pax;
}

boost::optional<FlightSeatDetails> Result::seat() const
{
    return m_seat;
}

boost::optional<CascadeHostDetails> Result::cascadeDetails() const
{
    return m_cascadeDetails;
}

boost::optional<ErrorDetails> Result::errorDetails() const
{
    return m_errorDetails;
}

Result::Action_e Result::strToAction(const std::string& a)
{
    if(a == "I") return Checkin;
    else if(a == "X") return Cancel;
    else if(a == "U") return Update;
    else {
        throw EXCEPTIONS::Exception("Unknown iatci action code: %s", a.c_str());
    }

}

Result::Status_e Result::strToStatus(const std::string& s)
{
    if(s == "O") return Ok;
    else if(s == "P") return OkWithNoData;
    else {
        return Failed;
    }
}

std::string Result::actionAsString() const
{
    switch(m_action)
    {
    case Checkin:   return "I";
    case Cancel:    return "X";
    case Update:    return "U";
    }

    throw EXCEPTIONS::Exception("Unknown iatci action code value: %d", m_action);
}

std::string Result::statusAsString() const
{
    switch(m_status)
    {
    case Ok:           return "O";
    case OkWithNoData: return "P";
    default:           return "X";
    }
}

//-----------------------------------------------------------------------------

const iatci::OriginatorDetails& CkiParams::origin() const
{
    return m_origin;
}

const iatci::FlightDetails& CkiParams::flight() const
{
    return m_flight;
}

const iatci::FlightDetails& CkiParams::flightFromPrevHost() const
{
    return m_flightFromPrevHost;
}

const iatci::PaxDetails& CkiParams::pax() const
{
    return m_pax;
}

boost::optional<iatci::ReservationDetails> CkiParams::reserv() const
{
    return m_reserv;
}

boost::optional<iatci::SeatDetails> CkiParams::seat() const
{
    return m_seat;
}

boost::optional<iatci::BaggageDetails> CkiParams::baggage() const
{
    return m_baggage;
}

boost::optional<iatci::CascadeHostDetails> CkiParams::cascadeDetails() const
{
    return m_cascadeDetails;
}

//-----------------------------------------------------------------------------

const iatci::OriginatorDetails& CkxParams::origin() const
{
    return m_origin;
}

const iatci::FlightDetails& CkxParams::flight() const
{
    return m_flight;
}

const iatci::PaxDetails& CkxParams::pax() const
{
    return m_pax;
}

boost::optional<iatci::CascadeHostDetails> CkxParams::cascadeDetails() const
{
    return m_cascadeDetails;
}

}//namespace iatci
