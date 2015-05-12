#include "iatci_types.h"

#include <serverlib/exception.h>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

OriginatorDetails::OriginatorDetails(const std::string& airl, const std::string& point)
    : m_airline(airl),
      m_point(point)
{}

const std::string& OriginatorDetails::airline() const
{
    return m_airline;
}

const std::string& OriginatorDetails::point() const
{
    return m_point;
}

//-----------------------------------------------------------------------------

FlightDetails::FlightDetails(const std::string& airl,
                             const Ticketing::FlightNum_t& flNum,
                             const std::string& depPoint,
                             const std::string& arrPoint,
                             const boost::gregorian::date& depDate,
                             const boost::gregorian::date& arrDate,
                             const boost::posix_time::time_duration& depTime,
                             const boost::posix_time::time_duration& arrTime,
                             const boost::posix_time::time_duration& brdTime)
    : m_airline(airl),
      m_flightNum(flNum),
      m_depPoint(depPoint),
      m_arrPoint(arrPoint),
      m_depDate(depDate),
      m_arrDate(arrDate),
      m_depTime(depTime),
      m_arrTime(arrTime),
      m_boardingTime(brdTime)
{}

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

PaxDetails::PaxDetails(const std::string& surname,
                       const std::string& name,
                       PaxType_e type,
                       const std::string& qryRef,
                       const std::string& respRef)
    : m_surname(surname),
      m_name(name),
      m_type(type),
      m_qryRef(qryRef),
      m_respRef(respRef)
{}

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
    if(s == "C")      return Child;
    else if(s == "F") return Female;
    else if(s == "M") return Male;
    else if(s == "A") return Adult;
    else {
        LogError(STDLOG) << "Unknown pax type string: " << s;
        return Adult;
    }
}

//-----------------------------------------------------------------------------

UpdateDetails::UpdateDetails(UpdateActionCode_e actionCode)
    : m_actionCode(actionCode)
{
}

UpdateDetails::UpdateActionCode_e UpdateDetails::actionCode() const
{
    return m_actionCode;
}

std::string UpdateDetails::actionCodeAsString() const
{
    switch(m_actionCode)
    {
    case Add:     return "A";
    case Replace: return "R";
    case Cancel:  return "C";
    default:      return "R";
    }
}

UpdateDetails::UpdateActionCode_e UpdateDetails::strToActionCode(const std::string& s)
{
    if(s == "A")      return Add;
    else if(s == "R") return Replace;
    else if(s == "C") return Cancel;
    else {
        if(!s.empty()) {
            LogError(STDLOG) << "Unknown update action code string: " << s;
        } else {
            LogTrace(TRACE0) << "Empty update action code string!";
        }
        return Replace;
    }
}

//-----------------------------------------------------------------------------

UpdatePaxDetails::UpdatePaxDetails(UpdateActionCode_e actionCode,
                                   const std::string& surname,
                                   const std::string& name,
                                   const std::string& qryRef)
    : UpdateDetails(actionCode),
      m_surname(surname), m_name(name), m_qryRef(qryRef)
{
}

const std::string& UpdatePaxDetails::surname() const
{
    return m_surname;
}

const std::string& UpdatePaxDetails::name() const
{
    return m_name;
}

const std::string& UpdatePaxDetails::qryRef() const
{
    return m_qryRef;
}

//-----------------------------------------------------------------------------

ReservationDetails::ReservationDetails(const std::string& rbd)
    : m_rbd(rbd)
{}

const std::string& ReservationDetails::rbd() const
{
    return m_rbd;
}

//-----------------------------------------------------------------------------

SeatDetails::SeatDetails(SmokeIndicator_e smokeInd)
    : m_smokeInd(smokeInd)
{
}

SeatDetails::SeatDetails(const std::string& seat, SmokeIndicator_e smokeInd)
    : m_smokeInd(smokeInd), m_seat(seat)
{}

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
    case None:          return "";
    default:            return "";
    }
}

const std::string& SeatDetails::seat() const
{
    return m_seat;
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
    if(s == "N")      return NonSmoking;
    else if(s == "P") return PartySeating;
    else if(s == "S") return Smoking;
    else if(s == "X") return Indifferent;
    else if(s == "U") return Unknown;
    else if(s == "")  return None;
    else {
        LogError(STDLOG) << "Unknown smoke indicator string: " << s;
        return Unknown;
    }
}

//-----------------------------------------------------------------------------

UpdateSeatDetails::UpdateSeatDetails(UpdateActionCode_e actionCode,
                                     const std::string& seat,
                                     const SmokeIndicator_e smokeInd)
    : UpdateDetails(actionCode), SeatDetails(seat, smokeInd)
{
}

//-----------------------------------------------------------------------------

FlightSeatDetails::FlightSeatDetails(const std::string& seat,
                                     const std::string& cabinClass,
                                     const std::string& securityId,
                                     SmokeIndicator_e smokeInd)
    : SeatDetails(seat, smokeInd),
      m_cabinClass(cabinClass), m_securityId(securityId)
{}

const std::string& FlightSeatDetails::cabinClass() const
{
    return m_cabinClass;
}

const std::string& FlightSeatDetails::securityId() const
{
    return m_securityId;
}

//-----------------------------------------------------------------------------

PaxSeatDetails::PaxSeatDetails(const std::string& surname,
                               const std::string& name,
                               const std::string& rbd,
                               const std::string& seat,
                               const std::string& securityId,
                               const std::string& recloc,
                               const std::string& tickNum,
                               const std::string& qryRef,
                               const std::string& respRef)
    : PaxDetails(surname, name, Adult, qryRef, respRef),
      m_rbd(rbd), m_seat(seat), m_securityId(securityId),
      m_recloc(recloc), m_tickNum(tickNum)
{
}

const std::string& PaxSeatDetails::rbd() const
{
    return m_rbd;
}

const std::string& PaxSeatDetails::seat() const
{
    return m_seat;
}

const std::string& PaxSeatDetails::securityId() const
{
    return m_securityId;
}

const std::string& PaxSeatDetails::recloc() const
{
    return m_recloc;
}

const std::string& PaxSeatDetails::tickNum() const
{
    return m_tickNum;
}

//-----------------------------------------------------------------------------

BaggageDetails::BaggageDetails(unsigned numOfPieces, unsigned weight)
    : m_numOfPieces(numOfPieces),
      m_weight(weight)
{}

unsigned BaggageDetails::numOfPieces() const
{
    return m_numOfPieces;
}

unsigned BaggageDetails::weight() const
{
    return m_weight;
}

//-----------------------------------------------------------------------------

UpdateBaggageDetails::UpdateBaggageDetails(UpdateActionCode_e actionCode,
                                           unsigned numOfPieces, unsigned weight)
    : UpdateDetails(actionCode), BaggageDetails(numOfPieces, weight)
{
}

//-----------------------------------------------------------------------------

CascadeHostDetails::CascadeHostDetails(const std::string& host)
{
    m_hostAirlines.push_back(host);
}

CascadeHostDetails::CascadeHostDetails(const std::string& origAirl,
                                       const std::string& origPoint)
    : m_originAirline(origAirl),
      m_originPoint(origPoint)
{}

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

ErrorDetails::ErrorDetails(const Ticketing::ErrMsg_t& errCode,
                           const std::string& errDesc)
    : m_errCode(errCode),
      m_errDesc(errDesc)
{}

const Ticketing::ErrMsg_t& ErrorDetails::errCode() const
{
    return m_errCode;
}

const std::string& ErrorDetails::errDesc() const
{
    return m_errDesc;
}

//-----------------------------------------------------------------------------

Result::Result(Action_e action,
               Status_e status,
               boost::optional<FlightDetails> flight,
               boost::optional<PaxDetails> pax,
               boost::optional<FlightSeatDetails> seat,
               boost::optional<CascadeHostDetails> cascadeDetails,
               boost::optional<ErrorDetails> errorDetails)
    : m_action(action),
      m_status(status),
      m_flight(flight),
      m_pax(pax),
      m_seat(seat),
      m_cascadeDetails(cascadeDetails),
      m_errorDetails(errorDetails)
{}

Result Result::makeResult(Action_e action,
                          Status_e status,
                          const FlightDetails& flight,
                          boost::optional<PaxDetails> pax,
                          boost::optional<FlightSeatDetails> seat,
                          boost::optional<CascadeHostDetails> cascadeDetails,
                          boost::optional<ErrorDetails> errorDetails)
{
    return Result(action,
                  status,
                  flight,
                  pax,
                  seat,
                  cascadeDetails,
                  errorDetails);
}

Result Result::makeCheckinResult(Status_e status,
                                 const FlightDetails& flight,
                                 const PaxDetails& pax,
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

Result Result::makeUpdateResult(Status_e status,
                                const FlightDetails& flight,
                                const PaxDetails& pax,
                                boost::optional<FlightSeatDetails> seat,
                                boost::optional<CascadeHostDetails> cascadeDetails,
                                boost::optional<ErrorDetails> errorDetails)
{
    return Result(Update,
                  status,
                  flight,
                  pax,
                  seat,
                  cascadeDetails,
                  errorDetails);
}

Result Result::makeCancelResult(Status_e status,
                                const FlightDetails& flight,
                                const PaxDetails& pax,
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

Result Result::makePasslistResult(Status_e status,
                                  const FlightDetails& flight,
                                  const PaxDetails& pax,
                                  boost::optional<FlightSeatDetails> seat,
                                  boost::optional<CascadeHostDetails> cascadeDetails,
                                  boost::optional<ErrorDetails> errorDetails)
{
    return Result(Passlist,
                  status,
                  flight,
                  pax,
                  seat,
                  cascadeDetails,
                  errorDetails);
}

Result Result::makeFailResult(Action_e action,
                              const ErrorDetails& errorDetails)
{
    return Result(action,
                  Failed,
                  boost::none,
                  boost::none,
                  boost::none,
                  boost::none,
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
    ASSERT(m_flight);
    return m_flight.get();
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
    if(a == "I")      return Checkin;
    else if(a == "X") return Cancel;
    else if(a == "U") return Update;
    else if(a == "P") return Passlist;
    else {
        throw EXCEPTIONS::Exception("Unknown iatci action code: %s", a.c_str());
    }

}

Result::Status_e Result::strToStatus(const std::string& s)
{
    if(s == "O")      return Ok;
    else if(s == "P") return OkWithNoData;
    else {
        LogError(STDLOG) << "Unknown status string: " << s;
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
    case Passlist:  return "P";
    }

    throw EXCEPTIONS::Exception("Unknown iatci action code value: %d", m_action);
}

std::string Result::statusAsString() const
{
    switch(m_status)
    {
    case Ok:           return "O";
    case OkWithNoData: return "P";
    case Failed:       return "X";
    }

    throw EXCEPTIONS::Exception("Unknown status value: %d", m_status);
}

//-----------------------------------------------------------------------------

Params::Params(const OriginatorDetails& origin,
               const PaxDetails& pax,
               const FlightDetails& flight,
               boost::optional<FlightDetails> flightFromPrevHost,
               boost::optional<CascadeHostDetails> cascadeDetails)
    : m_origin(origin), m_pax(pax), m_flight(flight),
      m_flightFromPrevHost(flightFromPrevHost), m_cascadeDetails(cascadeDetails)
{
}

const iatci::OriginatorDetails& Params::origin() const
{
    return m_origin;
}

const iatci::PaxDetails& Params::pax() const
{
    return m_pax;
}

const iatci::FlightDetails& Params::flight() const
{
    return m_flight;
}

boost::optional<FlightDetails> Params::flightFromPrevHost() const
{
    return m_flightFromPrevHost;
}

boost::optional<iatci::CascadeHostDetails> Params::cascadeDetails() const
{
    return m_cascadeDetails;
}

//-----------------------------------------------------------------------------

CkiParams::CkiParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<SeatDetails> seat,
                     boost::optional<BaggageDetails> baggage,
                     boost::optional<ReservationDetails> reserv,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails),
      m_seat(seat), m_baggage(baggage), m_reserv(reserv)
{}

boost::optional<iatci::SeatDetails> CkiParams::seat() const
{
    return m_seat;
}

boost::optional<iatci::BaggageDetails> CkiParams::baggage() const
{
    return m_baggage;
}

boost::optional<iatci::ReservationDetails> CkiParams::reserv() const
{
    return m_reserv;
}

//-----------------------------------------------------------------------------

CkuParams::CkuParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<UpdatePaxDetails> updPax,
                     boost::optional<UpdateSeatDetails> updSeat,
                     boost::optional<UpdateBaggageDetails> updBaggage,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails),
      m_updPax(updPax), m_updSeat(updSeat), m_updBaggage(updBaggage)
{
    if(!m_updPax && !m_updSeat && !m_updBaggage) {
        LogError(STDLOG) << "CkuParams without update information!";
    }
}

boost::optional<UpdatePaxDetails> CkuParams::updPax() const
{
    return m_updPax;
}

boost::optional<UpdateSeatDetails> CkuParams::updSeat() const
{
    return m_updSeat;
}

boost::optional<UpdateBaggageDetails> CkuParams::updBaggage() const
{
    return m_updBaggage;
}

//-----------------------------------------------------------------------------

CkxParams::CkxParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails)
{}

//-----------------------------------------------------------------------------

PlfParams::PlfParams(const OriginatorDetails& origin,
                     const PaxSeatDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails),
      m_paxEx(pax)
{}

const PaxSeatDetails& PlfParams::paxEx() const
{
    return m_paxEx;
}

}//namespace iatci
