#pragma once

#include "tlg/CheckinBaseTypes.h"

#include <etick/etick_msg_types.h>

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
    OriginatorDetails(const std::string& airl, const std::string& point = "");

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
                  const boost::posix_time::time_duration& brdTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time));

    const std::string&                      airline() const;
    Ticketing::FlightNum_t                  flightNum() const;
    const std::string&                      depPoint() const;
    const std::string&                      arrPoint() const;
    const boost::gregorian::date&           depDate() const;
    const boost::gregorian::date&           arrDate() const;
    const boost::posix_time::time_duration& depTime() const;
    const boost::posix_time::time_duration& arrTime() const;
    const boost::posix_time::time_duration& boardingTime() const;
    std::string                             toShortKeyString() const;

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
               const std::string& respRef = "");

    const std::string& surname() const;
    const std::string& name() const;
    PaxType_e          type() const;
    std::string        typeAsString() const;
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
    ReservationDetails(const std::string& rbd);

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
    std::string            m_seat;
    std::list<std::string> m_characteristics;

public:
    SeatDetails(SmokeIndicator_e smokeInd = Unknown,
                const std::string& seat = "");

    SmokeIndicator_e              smokeInd() const;
    std::string                   smokeIndAsString() const;
    const std::string&            seat() const;
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
                      SmokeIndicator_e smokeInd = Unknown);

    const std::string& seat() const;
    const std::string& cabinClass() const;
    const std::string& securityId() const;

protected:
    FlightSeatDetails(SmokeIndicator_e smokeInd = Unknown)
        : SeatDetails(smokeInd)
    {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct PaxSeatDetails: public PaxDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_rbd;
    std::string m_seat;
    std::string m_securityId;
    std::string m_recloc;
    std::string m_tickNum;

public:
    PaxSeatDetails(const std::string& surname,
                   const std::string& name,
                   const std::string& rbd = "",
                   const std::string& seat = "",
                   const std::string& securityId = "",
                   const std::string& recloc = "",
                   const std::string& tickNum = "",
                   const std::string& qryRef = "",
                   const std::string& respRef = "");

    const std::string& rbd() const;
    const std::string& seat() const;
    const std::string& securityId() const;
    const std::string& recloc() const;
    const std::string& tickNum() const;

protected:
    PaxSeatDetails()
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
    BaggageDetails(unsigned numOfPieces, unsigned weight);

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
    CascadeHostDetails(const std::string& host);
    CascadeHostDetails(const std::string& origAirl,
                       const std::string& origPoint);

    const std::string&            originAirline() const;
    const std::string&            originPoint() const;
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
    Ticketing::ErrMsg_t m_errCode;
    std::string         m_errDesc;

public:
    ErrorDetails(const Ticketing::ErrMsg_t& errCode,
                 const std::string& errDesc = "");

    const Ticketing::ErrMsg_t& errCode() const;
    const std::string&         errDesc() const;

protected:
    ErrorDetails() {} // only for boost serialization
};

//-----------------------------------------------------------------------------

struct Params
{
protected:
    OriginatorDetails                   m_origin;
    PaxDetails                          m_pax;
    FlightDetails                       m_flight;
    boost::optional<FlightDetails>      m_flightFromPrevHost;
    boost::optional<CascadeHostDetails> m_cascadeDetails;

public:
    Params(const OriginatorDetails& origin,
           const PaxDetails& pax,
           const FlightDetails& flight,
           boost::optional<FlightDetails> flightFromPrevHost = boost::none,
           boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    const OriginatorDetails&            origin() const;
    const PaxDetails&                   pax() const;
    const FlightDetails&                flight() const;
    boost::optional<FlightDetails>      flightFromPrevHost() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
};

//-----------------------------------------------------------------------------

struct CkiParams: public Params
{
protected:
    boost::optional<SeatDetails>        m_seat;
    boost::optional<BaggageDetails>     m_baggage;
    boost::optional<ReservationDetails> m_reserv;

public:
    CkiParams(const OriginatorDetails& origin,
              const PaxDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<SeatDetails> seat = boost::none,
              boost::optional<BaggageDetails> baggage = boost::none,
              boost::optional<ReservationDetails> reserv = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    boost::optional<SeatDetails>        seat() const;
    boost::optional<BaggageDetails>     baggage() const;
    boost::optional<ReservationDetails> reserv() const;
};

//-----------------------------------------------------------------------------

struct CkuParams: public Params
{
protected:
    boost::optional<PaxDetails>         m_updPax;
    boost::optional<SeatDetails>        m_updSeat;
    boost::optional<BaggageDetails>     m_updBaggage;
    boost::optional<ReservationDetails> m_updReserv;

public:
    CkuParams(const OriginatorDetails& origin,
              const PaxDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<PaxDetails> updPax = boost::none,
              boost::optional<SeatDetails> updSeat = boost::none,
              boost::optional<BaggageDetails> updBaggage = boost::none,
              boost::optional<ReservationDetails> updReserv = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    boost::optional<PaxDetails>         updPax() const;
    boost::optional<SeatDetails>        updSeat() const;
    boost::optional<ReservationDetails> updReserv() const;
    boost::optional<BaggageDetails>     updBaggage() const;
};

//-----------------------------------------------------------------------------

struct CkxParams: public Params
{
protected:
    boost::optional<ReservationDetails> m_reserv;
    boost::optional<SeatDetails>        m_seat;
    boost::optional<BaggageDetails>     m_baggage;

public:
    CkxParams(const OriginatorDetails& origin,
              const PaxDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

};

//-----------------------------------------------------------------------------

struct PlfParams: public Params
{
protected:
    PaxSeatDetails    m_paxEx;

public:
    PlfParams(const OriginatorDetails& origin,
              const PaxSeatDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    const PaxSeatDetails& paxEx() const;
};

//-----------------------------------------------------------------------------

struct Result
{
    friend class boost::serialization::access;

    enum Action_e
    {
        Checkin,
        Cancel,
        Update,
        Passlist
    };

    enum Status_e
    {
        Ok,
        OkWithNoData,
        Failed
    };

protected:
    Action_e                            m_action;
    Status_e                            m_status;
    boost::optional<FlightDetails>      m_flight;
    boost::optional<PaxDetails>         m_pax;
    boost::optional<FlightSeatDetails>  m_seat;
    boost::optional<CascadeHostDetails> m_cascadeDetails;
    boost::optional<ErrorDetails>       m_errorDetails;

    Result(Action_e action,
           Status_e status,
           boost::optional<FlightDetails> flight,
           boost::optional<PaxDetails> pax,
           boost::optional<FlightSeatDetails> seat,
           boost::optional<CascadeHostDetails> cascadeDetails,
           boost::optional<ErrorDetails> errorDetails);

public:
    static Result makeResult(Action_e action,
                             Status_e status,
                             const FlightDetails& flight,
                             boost::optional<PaxDetails> pax,
                             boost::optional<FlightSeatDetails> seat,
                             boost::optional<CascadeHostDetails> cascadeDetails,
                             boost::optional<ErrorDetails> errorDetails);

    static Result makeCheckinResult(Status_e status,
                                    const FlightDetails& flight,
                                    boost::optional<PaxDetails> pax = boost::none,
                                    boost::optional<FlightSeatDetails> seat = boost::none,
                                    boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                    boost::optional<ErrorDetails> errorDetails = boost::none);

    static Result makeUpdateResult(Status_e status,
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

    static Result makePasslistResult(Status_e status,
                                     const FlightDetails& flight,
                                     boost::optional<PaxDetails> pax = boost::none,
                                     boost::optional<FlightSeatDetails> seat = boost::none,
                                     boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                     boost::optional<ErrorDetails> errorDetails = boost::none);

    static Result makeFailResult(Action_e action,
                                 const ErrorDetails& errorDetails);

    Action_e                            action() const;
    Status_e                            status() const;
    const FlightDetails&                flight() const;
    boost::optional<PaxDetails>         pax() const;
    boost::optional<FlightSeatDetails>  seat() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
    boost::optional<ErrorDetails>       errorDetails() const;
    std::string                         actionAsString() const;
    std::string                         statusAsString() const;

    static Action_e strToAction(const std::string& a);
    static Status_e strToStatus(const std::string& s);

protected:
    Result()
        : m_status(Failed)
    {} // only for boost serialization
};

}//namespace iatci
