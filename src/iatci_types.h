#pragma once

#include "tlg/CheckinBaseTypes.h"

#include <list>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <boost/serialization/access.hpp>


namespace iatci {

struct OriginatorDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_airline;
    std::string m_point;

public:
    OriginatorDetails(const std::string& airl, const std::string& point = "")
        : m_airline(airl),
          m_point(point)
    {}

    const std::string& airline() const;
    const std::string& point() const;

protected:
    OriginatorDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct FlightDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string                      m_airline;
    Ticketing::FlightNum_t           m_flightNum;
    std::string                      m_depPoint;
    std::string                      m_arrPoint;
    boost::gregorian::date           m_depDate;
    boost::gregorian::date           m_arrDate;
    boost::posix_time::time_duration m_depTime;
    boost::posix_time::time_duration m_arrTime;
    boost::posix_time::time_duration m_boardingTime;

public:
    FlightDetails(const std::string& airl,
                  const Ticketing::FlightNum_t& flNum,
                  const std::string& depPoint,
                  const std::string& arrPoint,
                  const boost::gregorian::date& depDate,
                  const boost::gregorian::date& arrDate,
                  const boost::posix_time::time_duration& depTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                  const boost::posix_time::time_duration& arrTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                  const boost::posix_time::time_duration& brdTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time))
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

    const std::string& airline() const;
    Ticketing::FlightNum_t flightNum() const;
    const std::string& depPoint() const;
    const std::string& arrPoint() const;
    const boost::gregorian::date& depDate() const;
    const boost::gregorian::date& arrDate() const;
    const boost::posix_time::time_duration& depTime() const;
    const boost::posix_time::time_duration& arrTime() const;
    const boost::posix_time::time_duration& boardingTime() const;
    std::string toShortKeyString() const;

protected:
    FlightDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct PaxDetails
{
    friend class Result;
    friend class boost::serialization::access;

    enum PaxType_e
    {
        Adult,
        Child,
        Female,
        Male
    };

protected:
    std::string m_surname;
    std::string m_name;
    PaxType_e   m_type;
    std::string m_qryRef;
    std::string m_respRef;

public:
    PaxDetails(const std::string& surname,
               const std::string& name,
               PaxType_e type,
               const std::string& qryRef = "",
               const std::string& respRef = "")
        : m_surname(surname),
          m_name(name),
          m_type(type),
          m_qryRef(qryRef),
          m_respRef(respRef)
    {}

    const std::string& surname() const;
    const std::string& name() const;
    PaxType_e type() const;
    std::string typeAsString() const;
    const std::string& qryRef() const;
    const std::string& respRef() const;

    static PaxType_e strToType(const std::string& s);

protected:
    PaxDetails()
        : m_type(Adult)
    {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct ReservationDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_rbd;

public:
    ReservationDetails(const std::string& rbd)
        : m_rbd(rbd)
    {}

    const std::string& rbd() const;

protected:
    ReservationDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct SeatDetails
{
    friend class Result;
    friend class boost::serialization::access;

    enum SmokeIndicator_e
    {
        NonSmoking,
        PartySeating,
        Smoking,
        Unknown,
        Indifferent
    };

protected:
    SmokeIndicator_e       m_smokeInd;
    std::list<std::string> m_characteristics;

public:
    SeatDetails(SmokeIndicator_e smokeInd = Unknown)
        : m_smokeInd(smokeInd)
    {}

    SmokeIndicator_e smokeInd() const;
    std::string smokeIndAsString() const;
    const std::list<std::string>& characteristics() const;

    void addCharacteristic(const std::string& characteristic);

    static SmokeIndicator_e strToSmokeInd(const std::string& s);

};

//-----------------------------------------------------------------------------

struct FlightSeatDetails: public SeatDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_seat;
    std::string m_cabinClass;
    std::string m_securityId;

public:
    FlightSeatDetails(const std::string& seat,
                      const std::string& cabinClass,
                      const std::string& securityId,
                      SmokeIndicator_e smokeInd = Unknown)
        : SeatDetails(smokeInd),
          m_seat(seat), m_cabinClass(cabinClass), m_securityId(securityId)
    {}

    const std::string& seat() const;
    const std::string& cabinClass() const;
    const std::string& securityId() const;

protected:
    FlightSeatDetails(SmokeIndicator_e smokeInd = Unknown)
        : SeatDetails(smokeInd)
    {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct BaggageDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    unsigned m_numOfPieces;
    unsigned m_weight;

public:
    BaggageDetails(unsigned numOfPieces, unsigned weight)
        : m_numOfPieces(numOfPieces),
          m_weight(weight)
    {}

    unsigned numOfPieces() const;
    unsigned weight() const;

protected:
    BaggageDetails()
        : m_numOfPieces(0), m_weight(0)
    {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct CascadeHostDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_originAirline;
    std::string m_originPoint;
    std::list<std::string> m_hostAirlines;

public:
    CascadeHostDetails(const std::string& host)
    {
        m_hostAirlines.push_back(host);
    }

    CascadeHostDetails(const std::string& origAirl,
                       const std::string& origPoint)
        : m_originAirline(origAirl),
          m_originPoint(origPoint)
    {}

    const std::string& originAirline() const;
    const std::string& originPoint() const;
    const std::list<std::string>& hostAirlines() const;

    void addHostAirline(const std::string& hostAirline);

protected:
    CascadeHostDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct ErrorDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_errText;

public:
    ErrorDetails(const std::string& errText)
        : m_errText(errText)
    {}

    const std::string& errText() const;

protected:
    ErrorDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct CkiParams
{
protected:
    OriginatorDetails  m_origin;
    FlightDetails      m_flight;
    FlightDetails      m_flightFromPrevHost;
    PaxDetails         m_pax;
    boost::optional<ReservationDetails> m_reserv;
    boost::optional<SeatDetails>        m_seat;
    boost::optional<BaggageDetails>     m_baggage;
    boost::optional<CascadeHostDetails> m_cascadeDetails;

public:
    CkiParams(const OriginatorDetails& origin,
              const FlightDetails& flight,
              const FlightDetails& flightFromPrevHost,
              const PaxDetails& pax,
              boost::optional<ReservationDetails> reserv,
              boost::optional<SeatDetails> seat,
              boost::optional<BaggageDetails> baggage,
              boost::optional<CascadeHostDetails> cascadeDetails)
        : m_origin(origin),
          m_flight(flight),
          m_flightFromPrevHost(flightFromPrevHost),
          m_pax(pax),
          m_reserv(reserv),
          m_seat(seat),
          m_baggage(baggage),
          m_cascadeDetails(cascadeDetails)
    {}

    const OriginatorDetails& origin() const;
    const FlightDetails& flight() const;
    const FlightDetails& flightFromPrevHost() const;
    const PaxDetails& pax() const;
    boost::optional<ReservationDetails> reserv() const;
    boost::optional<SeatDetails> seat() const;
    boost::optional<BaggageDetails> baggage() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
};

//-----------------------------------------------------------------------------

struct CkuParams
{
    OriginatorDetails  m_origin;
    FlightDetails      m_nextFlight;
    FlightDetails      m_currFlight;
    PaxDetails         m_pax;
    ReservationDetails m_reserv;
    SeatDetails        m_seat;
    BaggageDetails     m_baggage;

    CkuParams(const OriginatorDetails& origin,
              const FlightDetails& nextFlight,
              const FlightDetails& currFlight,
              const PaxDetails& pax,
              const ReservationDetails& reserv,
              const SeatDetails& seat,
              const BaggageDetails& baggage)
        : m_origin(origin),
          m_nextFlight(nextFlight),
          m_currFlight(currFlight),
          m_pax(pax),
          m_reserv(reserv),
          m_seat(seat),
          m_baggage(baggage)
    {}
};

//-----------------------------------------------------------------------------

struct CkxParams
{
protected:
    OriginatorDetails m_origin;
    FlightDetails     m_flight;
    PaxDetails        m_pax;
    boost::optional<CascadeHostDetails> m_cascadeDetails;

public:
    CkxParams(const OriginatorDetails& origin,
              const FlightDetails& flight,
              const PaxDetails& pax,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none)
        : m_origin(origin),
          m_flight(flight),
          m_pax(pax),
          m_cascadeDetails(cascadeDetails)
    {}

    const OriginatorDetails& origin() const;
    const FlightDetails& flight() const;
    const PaxDetails& pax() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
};

//-----------------------------------------------------------------------------

struct Result
{
    friend class boost::serialization::access;

    enum Action_e
    {
        Checkin,
        Cancel,
        Update
    };

    enum Status_e
    {
        Ok,
        OkWithNoData,
        Failed
    };

protected:
    Action_e      m_action;
    Status_e      m_status;
    FlightDetails m_flight;
    boost::optional<PaxDetails> m_pax;
    boost::optional<FlightSeatDetails> m_seat;
    boost::optional<CascadeHostDetails> m_cascadeDetails;
    boost::optional<ErrorDetails> m_errorDetails;

public:
    Result(Action_e action,
           Status_e status,
           const FlightDetails& flight,
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

    static Result makeCheckinResult(Status_e status,
                                    const FlightDetails& flight,
                                    boost::optional<PaxDetails> pax = boost::none,
                                    boost::optional<FlightSeatDetails> seat = boost::none,
                                    boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                    boost::optional<ErrorDetails> errorDetails = boost::none);

    static Result makeCancelResult(Status_e status,
                                   const FlightDetails& flight,
                                   boost::optional<PaxDetails> pax = boost::none,
                                   boost::optional<FlightSeatDetails> seat = boost::none,
                                   boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                   boost::optional<ErrorDetails> errorDetails = boost::none);

    Action_e action() const;
    Status_e status() const;
    const FlightDetails& flight() const;
    boost::optional<PaxDetails> pax() const;
    boost::optional<FlightSeatDetails> seat() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
    boost::optional<ErrorDetails> errorDetails() const;
    std::string actionAsString() const;
    std::string statusAsString() const;

    static Action_e strToAction(const std::string& a);
    static Status_e strToStatus(const std::string& s);

protected:
    Result()
        : m_status(Failed)
    {} // only for boost serialization
};

}//namespace iatci
