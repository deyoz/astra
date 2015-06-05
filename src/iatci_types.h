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
    OriginatorDetails() {} // for boost serialization only
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
    FlightDetails() {} // for boost serialization only
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
    {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct UpdateDetails
{
    enum UpdateActionCode_e
    {
        Add,
        Cancel,
        Replace
    };

protected:
    UpdateActionCode_e m_actionCode;

public:
    UpdateDetails(UpdateActionCode_e actionCode = Replace);

    UpdateActionCode_e actionCode() const;
    std::string        actionCodeAsString() const;

    static UpdateActionCode_e strToActionCode(const std::string& s);

};

//-----------------------------------------------------------------------------

struct UpdatePaxDetails: public UpdateDetails
{
protected:
    std::string m_surname;
    std::string m_name;
    std::string m_qryRef;

public:
    UpdatePaxDetails(UpdateActionCode_e actionCode,
                     const std::string& surname,
                     const std::string& name,
                     const std::string& qryRef = "");

    const std::string& surname() const;
    const std::string& name() const;
    const std::string& qryRef() const;

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
    ReservationDetails() {} // for boost serialization only
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
        Indifferent,
        None
    };

protected:
    SmokeIndicator_e       m_smokeInd;
    std::string            m_seat;
    std::list<std::string> m_characteristics;

public:
    SeatDetails(SmokeIndicator_e smokeInd);
    SeatDetails(const std::string& seat = "",
                SmokeIndicator_e smokeInd = None);

    SmokeIndicator_e              smokeInd() const;
    std::string                   smokeIndAsString() const;
    const std::string&            seat() const;
    const std::list<std::string>& characteristics() const;

    void addCharacteristic(const std::string& characteristic);

    static SmokeIndicator_e strToSmokeInd(const std::string& s);

};

//-----------------------------------------------------------------------------

struct UpdateSeatDetails: public UpdateDetails, public SeatDetails
{
public:
    UpdateSeatDetails(UpdateActionCode_e actionCode,
                      const std::string& seat,
                      const SmokeIndicator_e smokeInd = None);
};

//-----------------------------------------------------------------------------

struct FlightSeatDetails: public SeatDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_cabinClass;
    std::string m_securityId;

public:
    FlightSeatDetails(const std::string& seat,
                      const std::string& cabinClass,
                      const std::string& securityId,
                      SmokeIndicator_e smokeInd = Unknown);

    const std::string& cabinClass() const;
    const std::string& securityId() const;

protected:
    FlightSeatDetails(SmokeIndicator_e smokeInd = Unknown)
        : SeatDetails(smokeInd)
    {} // for boost serialization only
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
    {} // for boost serialization only
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
    {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct UpdateBaggageDetails: public UpdateDetails, public BaggageDetails
{
public:
    UpdateBaggageDetails(UpdateActionCode_e actionCode,
                         unsigned numOfPieces, unsigned weight);
};

//-----------------------------------------------------------------------------

struct SeatRequestDetails: public SeatDetails
{
    friend class Result;
    friend class boost::serialization::access;
protected:
    std::string m_cabinClass;

public:
    SeatRequestDetails(const std::string& cabinClass = "",
                       const SmokeIndicator_e smokeInd = None);

    const std::string& cabinClass() const;

protected:
    using SeatDetails::seat; // hide method from the base class
};

//-----------------------------------------------------------------------------

struct RowRange
{
    friend class CabinDetails;
    friend class boost::serialization::access;

protected:
    unsigned m_firstRow;
    unsigned m_lastRow;

public:
    RowRange(unsigned firstRow, unsigned lastRow);

    unsigned firstRow() const;
    unsigned lastRow() const;

protected:
    RowRange()
        : m_firstRow(0), m_lastRow(0)
    {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct SeatColumnDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_column;
    std::string m_desc1;
    std::string m_desc2;

public:
    SeatColumnDetails(const std::string& column, const std::string& desc1,
                      const std::string& desc2 = "");

    const std::string& column() const;
    const std::string& desc1() const;
    const std::string& desc2() const;

protected:
    SeatColumnDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct CabinDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string                  m_classDesignator;
    RowRange                     m_rowRange;
    std::string                  m_defSeatOccupation;
    std::list<SeatColumnDetails> m_seatColumns;
    std::string                  m_deck;
    boost::optional<RowRange>    m_smokingArea;
    boost::optional<RowRange>    m_overwingArea;

public:
    CabinDetails(const std::string& classDesignator,
                 const RowRange& rowRange,
                 const std::string& defaultSeatOccupation = "",
                 const std::list<SeatColumnDetails>& seatColumns = std::list<SeatColumnDetails>(),
                 const std::string& deck = "",
                 boost::optional<RowRange> smokingArea = boost::none,
                 boost::optional<RowRange> overwingArea = boost::none);

    const std::string&                  classDesignator() const;
    const RowRange&                     rowRange() const;
    const std::string&                  defaultSeatOccupation() const;
    const std::list<SeatColumnDetails>& seatColumns() const;
    const std::string&                  deck() const;
    boost::optional<RowRange>           smokingArea() const;
    boost::optional<RowRange>           overwingArea() const;

protected:
    CabinDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct SeatOccupationDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string            m_column;
    std::string            m_occupation;
    std::list<std::string> m_lCharacteristics;

public:
    SeatOccupationDetails(const std::string& column,
                          const std::string& occupation,
                          const std::list<std::string>& lCharacteristics = std::list<std::string>());

    const std::string&            column() const;
    const std::string&            occupation() const;
    const std::list<std::string>& lCharacteristics() const;

protected:
    SeatOccupationDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct RowDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string                      m_row;
    std::string                      m_characteristic;
    std::list<SeatOccupationDetails> m_lOccupationDetails;

public:
    RowDetails(const std::string& row,
               const std::string& characteristic,
               const std::list<SeatOccupationDetails>& lOccupationDetails = std::list<SeatOccupationDetails>());

    const std::string&                      row() const;
    const std::string&                      characteristic() const;
    const std::list<SeatOccupationDetails>& lOccupationDetails() const;

protected:
    RowDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct SeatmapDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::list<CabinDetails>             m_lCabinDetails;
    std::list<RowDetails>               m_lRowDetails;
    boost::optional<SeatRequestDetails> m_seatRequestDetails;

public:
    SeatmapDetails(const std::list<CabinDetails>& lCabinDetails,
                   const std::list<RowDetails>& lRowDetails,
                   boost::optional<SeatRequestDetails> seatRequestDetails = boost::none);

    const std::list<CabinDetails>&      lCabinDetails() const;
    const std::list<RowDetails>&        lRowDetails() const;
    boost::optional<SeatRequestDetails> seatRequestDetails() const;

protected:
    SeatmapDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct CascadeHostDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string            m_originAirline;
    std::string            m_originPoint;
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
    CascadeHostDetails() {} // for boost serialization only
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
    ErrorDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct EquipmentDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    std::string m_equipment;

public:
    EquipmentDetails(const std::string& equipment);

    const std::string equipment() const;

protected:
    EquipmentDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct WarningDetails
{
    friend class Result;
    friend class boost::serialization::access;

protected:
    Ticketing::ErrMsg_t m_warningCode;
    std::string         m_warningDesc;

public:
    WarningDetails(const Ticketing::ErrMsg_t& warningCode,
                   const std::string& warningDesc = "");

    const Ticketing::ErrMsg_t& warningCode() const;
    const std::string&         warningDesc() const;

protected:
    WarningDetails() {} // for boost serialization only
};

//-----------------------------------------------------------------------------

struct BaseParams
{
protected:
    OriginatorDetails                   m_origin;
    FlightDetails                       m_flight;
    boost::optional<FlightDetails>      m_flightFromPrevHost;
    boost::optional<CascadeHostDetails> m_cascadeDetails;

public:
    BaseParams(const OriginatorDetails& origin,
               const FlightDetails& flight,
               boost::optional<FlightDetails> flightFromPrevHost = boost::none,
               boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    const OriginatorDetails&            origin() const;
    const FlightDetails&                flight() const;
    boost::optional<FlightDetails>      flightFromPrevHost() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
};

//-----------------------------------------------------------------------------

struct Params: public BaseParams
{
protected:
    PaxDetails m_pax;

public:
    Params(const OriginatorDetails& origin,
           const PaxDetails& pax,
           const FlightDetails& flight,
           boost::optional<FlightDetails> flightFromPrevHost = boost::none,
           boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    const PaxDetails& pax() const;
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
    boost::optional<UpdatePaxDetails>     m_updPax;
    boost::optional<UpdateSeatDetails>    m_updSeat;
    boost::optional<UpdateBaggageDetails> m_updBaggage;

public:
    CkuParams(const OriginatorDetails& origin,
              const PaxDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<UpdatePaxDetails> updPax = boost::none,
              boost::optional<UpdateSeatDetails> updSeat = boost::none,
              boost::optional<UpdateBaggageDetails> updBaggage = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    boost::optional<UpdatePaxDetails>     updPax() const;
    boost::optional<UpdateSeatDetails>    updSeat() const;
    boost::optional<UpdateBaggageDetails> updBaggage() const;
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
    PaxSeatDetails m_paxEx;

public:
    PlfParams(const OriginatorDetails& origin,
              const PaxSeatDetails& pax,
              const FlightDetails& flight,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    const PaxSeatDetails& paxEx() const;
};

//-----------------------------------------------------------------------------

struct SmfParams: public BaseParams
{
    boost::optional<SeatRequestDetails> m_seatReqDetails;

public:
    SmfParams(const OriginatorDetails& origin,
              const FlightDetails& flight,
              boost::optional<SeatRequestDetails> seatReqDetails = boost::none,
              boost::optional<FlightDetails> flightFromPrevHost = boost::none,
              boost::optional<CascadeHostDetails> cascadeDetails = boost::none);

    boost::optional<SeatRequestDetails> seatRequestDetails() const;
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
        Passlist,
        Seatmap,
        SeatmapForPassenger
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
    boost::optional<SeatmapDetails>     m_seatmap;
    boost::optional<CascadeHostDetails> m_cascadeDetails;
    boost::optional<ErrorDetails>       m_errorDetails;
    boost::optional<WarningDetails>     m_warningDetails;
    boost::optional<EquipmentDetails>   m_equipmentDetails;

    Result(Action_e action,
           Status_e status,
           boost::optional<FlightDetails> flight,
           boost::optional<PaxDetails> pax,
           boost::optional<FlightSeatDetails> seat,
           boost::optional<SeatmapDetails> seatmap,
           boost::optional<CascadeHostDetails> cascadeDetails,
           boost::optional<ErrorDetails> errorDetails,
           boost::optional<WarningDetails> warningDetails,
           boost::optional<EquipmentDetails> equipmentDetails);

public:
    static Result makeResult(Action_e action,
                             Status_e status,
                             const FlightDetails& flight,
                             boost::optional<PaxDetails> pax,
                             boost::optional<FlightSeatDetails> seat,
                             boost::optional<SeatmapDetails> seatmap,
                             boost::optional<CascadeHostDetails> cascadeDetails,
                             boost::optional<ErrorDetails> errorDetails,
                             boost::optional<WarningDetails> warningDetails,
                             boost::optional<EquipmentDetails> equipmentDetails);

    static Result makeCheckinResult(Status_e status,
                                    const FlightDetails& flight,
                                    const PaxDetails& pax,
                                    boost::optional<FlightSeatDetails> seat = boost::none,
                                    boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                    boost::optional<ErrorDetails> errorDetails = boost::none,
                                    boost::optional<WarningDetails> warningDetails = boost::none,
                                    boost::optional<EquipmentDetails> equipmentDetails = boost::none);

    static Result makeUpdateResult(Status_e status,
                                   const FlightDetails& flight,
                                   const PaxDetails& pax,
                                   boost::optional<FlightSeatDetails> seat = boost::none,
                                   boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                   boost::optional<ErrorDetails> errorDetails = boost::none,
                                   boost::optional<WarningDetails> warningDetails = boost::none,
                                   boost::optional<EquipmentDetails> equipmentDetails = boost::none);

    static Result makeCancelResult(Status_e status,
                                   const FlightDetails& flight,
                                   const PaxDetails& pax,
                                   boost::optional<FlightSeatDetails> seat = boost::none,
                                   boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                   boost::optional<ErrorDetails> errorDetails = boost::none,
                                   boost::optional<WarningDetails> warningDetails = boost::none,
                                   boost::optional<EquipmentDetails> equipmentDetails = boost::none);

    static Result makePasslistResult(Status_e status,
                                     const FlightDetails& flight,
                                     const PaxDetails& pax,
                                     boost::optional<FlightSeatDetails> seat = boost::none,
                                     boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                     boost::optional<ErrorDetails> errorDetails = boost::none,
                                     boost::optional<WarningDetails> warningDetails = boost::none,
                                     boost::optional<EquipmentDetails> equipmentDetails = boost::none);

    static Result makeSeatmapResult(Status_e status,
                                    const FlightDetails& flight,
                                    const SeatmapDetails& seatmap,
                                    boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                                    boost::optional<ErrorDetails> errorDetails = boost::none,
                                    boost::optional<WarningDetails> warningDetails = boost::none,
                                    boost::optional<EquipmentDetails> equipmentDetails = boost::none);

    static Result makeFailResult(Action_e action,
                                 const ErrorDetails& errorDetails);

    Action_e                            action() const;
    Status_e                            status() const;
    const FlightDetails&                flight() const;
    boost::optional<PaxDetails>         pax() const;
    boost::optional<FlightSeatDetails>  seat() const;
    boost::optional<SeatmapDetails>     seatmap() const;
    boost::optional<CascadeHostDetails> cascadeDetails() const;
    boost::optional<ErrorDetails>       errorDetails() const;
    boost::optional<WarningDetails>     warningDetails() const;
    boost::optional<EquipmentDetails>   equipmentDetails() const;

    std::string                         actionAsString() const;
    std::string                         statusAsString() const;

    static Action_e strToAction(const std::string& a);
    static Status_e strToStatus(const std::string& s);

protected:
    Result()
        : m_status(Failed)
    {} // for boost serialization only
};

}//namespace iatci
