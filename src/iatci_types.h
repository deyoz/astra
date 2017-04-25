#pragma once

#include "tlg/CheckinBaseTypes.h"
#include "ticket_types.h"

#include <etick/etick_msg_types.h>
#include <etick/tick_data.h>

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/serialization/access.hpp>

#include <libxml/tree.h>

#include <list>
#include <vector>


namespace iatci {

namespace dcrcka {
struct Result;
}//namespace dcrcka

class MagicTab
{
    int m_grpId;
    unsigned m_tabInd;

public:
    MagicTab(int grpId, unsigned tabInd)
        : m_grpId(grpId), m_tabInd(tabInd)
    {}

    int grpId() const { return m_grpId; }
    unsigned tabInd() const { return m_tabInd; }

    int toNeg() const;

    static MagicTab fromNeg(int pt);
};

//---------------------------------------------------------------------------------------

class Seat
{
    std::string m_row;
    std::string m_col;

public:
    Seat(const std::string& row, const std::string& col);
    static Seat fromStr(const std::string& str);

    const std::string& row() const { return m_row; }
    const std::string& col() const { return m_col; }

    std::string toStr() const;
};

bool operator==(const Seat& left, const Seat& right);
bool operator!=(const Seat& left, const Seat& right);

//---------------------------------------------------------------------------------------

struct OriginatorDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_airline;
    std::string m_port;

public:
    OriginatorDetails(const std::string& airl, const std::string& port = "");

    const std::string& airline() const;
    const std::string& port() const;

protected:
    OriginatorDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct FlightDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string                      m_airline;
    Ticketing::FlightNum_t           m_flightNum;
    std::string                      m_depPort;
    std::string                      m_arrPort;
    boost::gregorian::date           m_depDate;
    boost::gregorian::date           m_arrDate;
    boost::posix_time::time_duration m_depTime;
    boost::posix_time::time_duration m_arrTime;
    boost::posix_time::time_duration m_boardingTime;

public:
    FlightDetails(const std::string& airl,
                  const Ticketing::FlightNum_t& flNum,
                  const std::string& depPort,
                  const std::string& arrPort,
                  const boost::gregorian::date& depDate,
                  const boost::gregorian::date& arrDate = boost::gregorian::date(),
                  const boost::posix_time::time_duration& depTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                  const boost::posix_time::time_duration& arrTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                  const boost::posix_time::time_duration& brdTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time));

    const std::string&                      airline() const;
    Ticketing::FlightNum_t                  flightNum() const;
    const std::string&                      depPort() const;
    const std::string&                      arrPort() const;
    const boost::gregorian::date&           depDate() const;
    const boost::gregorian::date&           arrDate() const;
    const boost::posix_time::time_duration& depTime() const;
    const boost::posix_time::time_duration& arrTime() const;
    const boost::posix_time::time_duration& boardingTime() const;
    std::string                             toShortKeyString() const;

protected:
    FlightDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct DocDetails
{
    friend class boost::serialization::access;

    protected:
        std::string m_docType;
        std::string m_issueCountry;
        std::string m_no;
        std::string m_surname;
        std::string m_name;
        std::string m_secondName;
        std::string m_gender;
        std::string m_nationality;
        boost::gregorian::date m_birthDate;
        boost::gregorian::date m_expiryDate;

    public:
        DocDetails(const std::string& docType,
                   const std::string& issueCountry,
                   const std::string& no,
                   const std::string& surname,
                   const std::string& name,
                   const std::string& secondName,
                   const std::string& gender,
                   const std::string& nationality,
                   const boost::gregorian::date& birthDate = boost::gregorian::date(),
                   const boost::gregorian::date& expiryDate = boost::gregorian::date());

        const std::string& docType() const;
        const std::string& issueCountry() const;
        const std::string& no() const;
        const std::string& surname() const;
        const std::string& name() const;
        const std::string& secondName() const;
        const std::string& gender() const;
        const std::string& nationality() const;
        const boost::gregorian::date& birthDate() const;
        const boost::gregorian::date& expiryDate() const;

    protected:
        DocDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct AddressDetails
{

};

//---------------------------------------------------------------------------------------

struct PaxDetails
{
    friend class dcrcka::Result;
    friend class PaxGroup;
    friend class boost::serialization::access;

    enum PaxType_e
    {
        Adult,
        Child,
        Female,
        Male,
        Infant // внутренний статус
    };

    enum WithInftIndicator_e
    {
        WithoutInfant,
        WithInfant
    };

protected:
    std::string         m_surname;
    std::string         m_name;
    PaxType_e           m_type;
    std::string         m_qryRef;
    std::string         m_respRef;
    WithInftIndicator_e m_withInftIndic;

public:
    PaxDetails(const std::string& surname,
               const std::string& name,
               PaxType_e type,
               const std::string& qryRef = "",
               const std::string& respRef = "",
               WithInftIndicator_e withInftIndic = WithoutInfant);

    const std::string&  surname() const;
    const std::string&  name() const;
    PaxType_e           type() const;
    std::string         typeAsString() const;
    WithInftIndicator_e withInftIndicator() const;
    std::string         withInftIndicatorAsString() const;
    const std::string&  qryRef() const;
    const std::string&  respRef() const;

    bool                isInfant() const;

    static PaxType_e strToType(const std::string& s);
    static WithInftIndicator_e strToWithInftIndicator(const std::string& s);

protected:
    PaxDetails()
        : m_type(Adult),
          m_withInftIndic(WithoutInfant)
    {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

struct UpdateDocDetails: public UpdateDetails, public DocDetails
{
    UpdateDocDetails(UpdateActionCode_e actionCode,
                     const std::string& docType,
                     const std::string& issueCountry,
                     const std::string& no,
                     const std::string& surname,
                     const std::string& name,
                     const std::string& secondName,
                     const std::string& gender,
                     const std::string& nationality,
                     const boost::gregorian::date& birthDate = boost::gregorian::date(),
                     const boost::gregorian::date& expiryDate = boost::gregorian::date());
};

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

struct ReservationDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_rbd;

public:
    ReservationDetails(const std::string& rbd);

    const std::string& rbd() const;

    Ticketing::SubClass subclass() const;

protected:
    ReservationDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct SeatDetails
{
    friend class dcrcka::Result;
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

//---------------------------------------------------------------------------------------

struct UpdateSeatDetails: public UpdateDetails, public SeatDetails
{
public:
    UpdateSeatDetails(UpdateActionCode_e actionCode,
                      const std::string& seat,
                      SmokeIndicator_e smokeInd = None);
};

//---------------------------------------------------------------------------------------

struct FlightSeatDetails: public SeatDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_cabinClass;
    std::string m_regNo;

public:
    FlightSeatDetails(const std::string& seat,
                      const std::string& cabinClass,
                      const std::string& regNo,
                      SmokeIndicator_e smokeInd = None);

    const std::string& cabinClass() const;
    const std::string& regNo() const;

protected:
    FlightSeatDetails(SmokeIndicator_e smokeInd = None)
        : SeatDetails(smokeInd)
    {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct SelectPersonalDetails: public PaxDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_rbd;
    std::string m_seat;
    std::string m_securityId;
    std::string m_recloc;
    std::string m_tickNum;

public:
    SelectPersonalDetails(const std::string& surname,
                          const std::string& name,
                          const std::string& rbd = "",
                          const std::string& seat = "",
                          const std::string& securityId = "",
                          const std::string& recloc = "",
                          const std::string& tickNum = "",
                          const std::string& qryRef = "",
                          const std::string& respRef = "");

    SelectPersonalDetails(const PaxDetails& pax);

    const std::string& rbd() const;
    const std::string& seat() const;
    const std::string& securityId() const;
    const std::string& recloc() const;
    const std::string& tickNum() const;

protected:
    SelectPersonalDetails()
    {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct BaggageDetails
{
    friend class dcrcka::Result;
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

//---------------------------------------------------------------------------------------

struct UpdateBaggageDetails: public UpdateDetails, public BaggageDetails
{
public:
    UpdateBaggageDetails(UpdateActionCode_e actionCode,
                         unsigned numOfPieces, unsigned weight);
};

//---------------------------------------------------------------------------------------

struct ServiceDetails
{
    struct SsrInfo
    {
        friend class boost::serialization::access;

    protected:
        std::string m_ssrCode;
        std::string m_ssrText;
        bool        m_isInfantTicket;
        std::string m_freeText;
        std::string m_airline;
        unsigned    m_quantity;

    public:
        SsrInfo(const std::string& ssrCode, const std::string& ssrText,
                bool isInftTicket = false, const std::string& freeText = "",
                const std::string& airline = "", unsigned quantity = 0);

        const std::string& ssrCode() const;
        const std::string& ssrText() const;
        bool               isInfantTicket() const;
        const std::string& freeText() const;
        const std::string& airline() const;
        unsigned           quantity() const;

        Ticketing::TicketCpn_t toTicketCpn() const;

    protected:
        SsrInfo()
        {} // for boost serialization only
    };

protected:
    std::list<SsrInfo> m_lSsr;
    std::string        m_osi;

public:
    ServiceDetails(const std::string& osi = "");
    ServiceDetails(const std::list<SsrInfo>& lSsr,
                   const std::string& osi = "");

    const std::list<SsrInfo>& lSsr() const;
    const std::string&        osi() const;

    void addSsr(const ServiceDetails::SsrInfo& ssr);
    void addSsr(const std::string& ssrCode, const std::string& ssrFreeText);
    void addSsrTkne(const std::string& tickNum, bool isInftTicket = false);
    void addSsrTkne(const std::string& tickNum, unsigned couponNum, bool inftTicket);
    void addSsrFqtv(const std::string& fqtvCode);

    boost::optional<Ticketing::TicketCpn_t> findTicketCpn(bool inftTicket) const;
};

//---------------------------------------------------------------------------------------

struct UpdateServiceDetails: public UpdateDetails
{
    struct UpdSsrInfo: public UpdateDetails, public ServiceDetails::SsrInfo
    {
    public:
        UpdSsrInfo(UpdateActionCode_e actionCode,
                   const std::string& ssrCode, const std::string& ssrText,
                   bool isInftTicket = false, const std::string& freeText = "",
                   const std::string& airline = "", unsigned quantity = 0);
    };

protected:
    std::list<UpdSsrInfo> m_lUpdSsr;

public:
    UpdateServiceDetails(UpdateActionCode_e actionCode = UpdateDetails::Replace);

    const std::list<UpdSsrInfo>& lSsr() const;
    void addSsr(const UpdateServiceDetails::UpdSsrInfo& updSsr);
};

//---------------------------------------------------------------------------------------

struct SeatRequestDetails: public SeatDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_cabinClass;

public:
    SeatRequestDetails(const std::string& cabinClass = "",
                       SmokeIndicator_e smokeInd = None);

    const std::string& cabinClass() const;

protected:
    using SeatDetails::seat; // hide method from the base class
};

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

struct SeatColumnDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_column;
    std::string m_desc1;
    std::string m_desc2;

public:
    SeatColumnDetails(const std::string& column,
                      const std::string& desc1 = "",
                      const std::string& desc2 = "");

    const std::string& column() const;
    const std::string& desc1() const;
    const std::string& desc2() const;

    void setAisle();

protected:
    SeatColumnDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct CabinDetails
{
    friend class dcrcka::Result;
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
    const boost::optional<RowRange>&    smokingArea() const;
    const boost::optional<RowRange>&    overwingArea() const;

protected:
    CabinDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct SeatOccupationDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string            m_column;
    std::string            m_occupation;
    std::list<std::string> m_lCharacteristics;

public:
    SeatOccupationDetails(const std::string& column,
                          const std::string& occupation = "",
                          const std::list<std::string>& lCharacteristics = std::list<std::string>());

    const std::string&            column() const;
    const std::string&            occupation() const;
    const std::list<std::string>& lCharacteristics() const;

    void setOccupied();

protected:
    SeatOccupationDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct RowDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string                      m_row;
    std::list<SeatOccupationDetails> m_lOccupationDetails;
    std::string                      m_characteristic;

public:
    RowDetails(const std::string& row,
               const std::list<SeatOccupationDetails>& lOccupationDetails,
               const std::string& characteristic = "");

    RowDetails(const unsigned& row,
               const std::list<SeatOccupationDetails>& lOccupationDetails);

    const std::string&                      row() const;
    const std::list<SeatOccupationDetails>& lOccupationDetails() const;
    const std::string&                      characteristic() const;

protected:
    RowDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct SeatmapDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::list<CabinDetails>             m_lCabin;
    std::list<RowDetails>               m_lRow;
    boost::optional<SeatRequestDetails> m_seatRequest;

public:
    SeatmapDetails(const std::list<CabinDetails>& lCabin,
                   const std::list<RowDetails>& lRow,
                   boost::optional<SeatRequestDetails> seatRequest = boost::none);

    const std::list<CabinDetails>&             lCabin() const;
    const std::list<RowDetails>&               lRow() const;
    const boost::optional<SeatRequestDetails>& seatRequest() const;

protected:
    SeatmapDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct CascadeHostDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string            m_originAirline;
    std::string            m_originPort;
    std::list<std::string> m_hostAirlines;

public:
    CascadeHostDetails(const std::string& host);
    CascadeHostDetails(const std::string& origAirl,
                       const std::string& origPort);

    const std::string&            originAirline() const;
    const std::string&            originPort() const;
    const std::list<std::string>& hostAirlines() const;

    void addHostAirline(const std::string& hostAirline);

protected:
    CascadeHostDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct ErrorDetails
{
    friend class dcrcka::Result;
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

//---------------------------------------------------------------------------------------

struct EquipmentDetails
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    std::string m_equipment;

public:
    EquipmentDetails(const std::string& equipment);

    const std::string& equipment() const;

protected:
    EquipmentDetails() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct WarningDetails
{
    friend class dcrcka::Result;
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

//---------------------------------------------------------------------------------------

class PlaceMatrix
{
public:
    struct Coord2D
    {
        unsigned m_x;
        unsigned m_y;

        Coord2D(unsigned x, unsigned y)
            : m_x(x), m_y(y)
        {}

        bool operator==(const Coord2D& other) const {
            return (m_x == other.m_x && m_y == other.m_y);
        }
        bool operator< (const Coord2D& other) const {
            return (m_y == other.m_y ? m_x < other.m_x : m_y < other.m_y);
        }
    };

    struct Limits
    {
        Coord2D m_topLeft;
        Coord2D m_bottomRight;

        Limits(const Coord2D& tl, const Coord2D& br)
            : m_topLeft(tl), m_bottomRight(br)
        {}

        unsigned width() const {
            return std::abs(m_bottomRight.m_x - m_topLeft.m_x);
        }

        unsigned height() const {
            return std::abs(m_bottomRight.m_y - m_topLeft.m_y);
        }
    };

    struct Place
    {
        std::string m_xName;
        std::string m_yName;
        std::string m_class;
        std::string m_elemType;
        bool        m_occupied;

        Place(bool occupied = false)
            : m_occupied(occupied)
        {}

        Place(const std::string& xName, const std::string& yName,
              const std::string& cls, const std::string& elemType,
              bool occupied)
            : m_xName(xName), m_yName(yName),
              m_class(cls), m_elemType(elemType),
              m_occupied(occupied)
        {}
    };

    struct PlaceList
    {
        std::map<Coord2D, Place> m_places;

        void setPlace(const Coord2D& coord, const Place& place) {
            m_places[coord] = place;
        }

        Place getPlace(const Coord2D& coord) const {
            return m_places.at(coord);
        }

        const std::map<Coord2D, Place>& places() const {
            return m_places;
        }

        boost::optional<Coord2D> findPlaceCoords(const std::string& xName,
                                                 const std::string& yName) const;

        Limits limits() const;
    };

private:
    std::map<size_t, PlaceList> m_matrix;

public:
    PlaceMatrix() {}

    const std::map<size_t, PlaceList>& placeLists() const {
        return m_matrix;
    }

    void addPlaceList(size_t num, const PlaceList& placeList) {
        m_matrix[num] = placeList;
    }

    size_t findPlaceListNum(const std::string& xName,
                            const std::string& yName) const;

};

//---------------------------------------------------------------------------------------

PlaceMatrix createPlaceMatrix(const SeatmapDetails& seatmap);

//---------------------------------------------------------------------------------------

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

    const OriginatorDetails&                   origin() const;
    const FlightDetails&                       flight() const;
    const boost::optional<FlightDetails>&      flightFromPrevHost() const;
    const boost::optional<CascadeHostDetails>& cascadeDetails() const;
};

//---------------------------------------------------------------------------------------

class IBaseParams
{
public:
    virtual const OriginatorDetails&                   org() const = 0;
    virtual const FlightDetails&                       outboundFlight() const = 0;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const = 0;
    virtual const boost::optional<CascadeHostDetails>& cascade() const = 0;

    virtual ~IBaseParams() {}
};

//---------------------------------------------------------------------------------------

struct Params: public BaseParams
{
protected:
    PaxDetails                      m_pax;
    boost::optional<ServiceDetails> m_service;

public:
    Params(const OriginatorDetails& origin,
           const PaxDetails& pax,
           const FlightDetails& flight,
           boost::optional<FlightDetails> flightFromPrevHost = boost::none,
           boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
           boost::optional<ServiceDetails> serviceDetails = boost::none);

    const PaxDetails&                      pax() const;
    const boost::optional<ServiceDetails>& service() const;
};

//---------------------------------------------------------------------------------------

class PaxGroup
{
protected:
    PaxDetails                          m_pax;
    boost::optional<ReservationDetails> m_reserv;
    boost::optional<BaggageDetails>     m_baggage;
    boost::optional<ServiceDetails>     m_service;
    boost::optional<DocDetails>         m_doc;
    boost::optional<AddressDetails>     m_address;
    boost::optional<PaxDetails>         m_infant;
    boost::optional<DocDetails>         m_infantDoc;

public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv,
             const boost::optional<BaggageDetails>& baggage,
             const boost::optional<ServiceDetails>& service,
             const boost::optional<DocDetails>& doc,
             const boost::optional<AddressDetails>& address,
             const boost::optional<PaxDetails>& infant = boost::none,
             const boost::optional<DocDetails>& infantDoc = boost::none);

    const PaxDetails&                          pax() const;
    const boost::optional<ReservationDetails>& reserv() const;
    const boost::optional<BaggageDetails>&     baggage() const;
    const boost::optional<ServiceDetails>&     service() const;
    const boost::optional<DocDetails>&         doc() const;
    const boost::optional<AddressDetails>&     address() const;
    const boost::optional<PaxDetails>&         infant() const;
    const boost::optional<DocDetails>&         infantDoc() const;

protected:
    PaxGroup() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

class FlightGroup
{
protected:
    FlightDetails                  m_outboundFlight;
    boost::optional<FlightDetails> m_inboundFlight;

public:
    FlightGroup(const FlightDetails& outboundFlight,
                const boost::optional<FlightDetails>& inboundFlight);

    const FlightDetails&                  outboundFlight() const;
    const boost::optional<FlightDetails>& inboundFlight() const;
};

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqcki {

class PaxGroup: public iatci::PaxGroup
{
protected:
    boost::optional<SeatDetails> m_seat;

public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv,
             const boost::optional<SeatDetails>& seat,
             const boost::optional<BaggageDetails>& baggage,
             const boost::optional<ServiceDetails>& service,
             const boost::optional<DocDetails>& doc,
             const boost::optional<AddressDetails>& address,
             const boost::optional<PaxDetails>& infant = boost::none,
             const boost::optional<DocDetails>& infantDoc = boost::none);

    const boost::optional<SeatDetails>& seat() const;
};

//---------------------------------------------------------------------------------------

class FlightGroup: public iatci::FlightGroup
{
protected:
    std::list<dcqcki::PaxGroup> m_paxGroups;

public:
    FlightGroup(const FlightDetails& outboundFlight,
                const boost::optional<FlightDetails>& inboundFlight,
                const std::list<dcqcki::PaxGroup>& paxGroups);

    const std::list<dcqcki::PaxGroup>& paxGroups() const;
};

}//namespace dcqcki

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqcku {

class PaxGroup: public iatci::PaxGroup
{
protected:
    boost::optional<UpdatePaxDetails>     m_updPax;
    boost::optional<UpdateSeatDetails>    m_updSeat;
    boost::optional<UpdateBaggageDetails> m_updBaggage;
    boost::optional<UpdateServiceDetails> m_updService;
    boost::optional<UpdateDocDetails>     m_updDoc;

public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv,
             const boost::optional<BaggageDetails>& baggage,
             const boost::optional<ServiceDetails>& service,
             const boost::optional<UpdatePaxDetails>& updPax,
             const boost::optional<UpdateSeatDetails>& updSeat,
             const boost::optional<UpdateBaggageDetails>& updBaggage,
             const boost::optional<UpdateServiceDetails>& updService,
             const boost::optional<UpdateDocDetails>& updDoc);

    const boost::optional<UpdatePaxDetails>&     updPax() const;
    const boost::optional<UpdateSeatDetails>&    updSeat() const;
    const boost::optional<UpdateBaggageDetails>& updBaggage() const;
    const boost::optional<UpdateServiceDetails>& updService() const;
    const boost::optional<UpdateDocDetails>&     updDoc() const;
};

//---------------------------------------------------------------------------------------

class FlightGroup: public iatci::FlightGroup
{
protected:
    std::list<dcqcku::PaxGroup> m_paxGroups;

public:
    FlightGroup(const FlightDetails& outboundFlight,
                const boost::optional<FlightDetails>& inboundFlight,
                const std::list<dcqcku::PaxGroup>& paxGroups);

    const std::list<dcqcku::PaxGroup>& paxGroups() const;
};

}//namespace dcqcku

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqckx {

class PaxGroup: public iatci::PaxGroup
{
protected:
    boost::optional<SeatDetails> m_seat;

public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv = boost::none,
             const boost::optional<SeatDetails>& seat = boost::none,
             const boost::optional<BaggageDetails>& baggage = boost::none,
             const boost::optional<ServiceDetails>& service = boost::none,
             const boost::optional<PaxDetails>& infant = boost::none);

    const boost::optional<SeatDetails>& seat() const;
};

//---------------------------------------------------------------------------------------

class FlightGroup: public iatci::FlightGroup
{
protected:
    std::list<dcqckx::PaxGroup> m_paxGroups;

public:
    FlightGroup(const FlightDetails& outboundFlight,
                const boost::optional<FlightDetails>& inboundFlight,
                const std::list<dcqckx::PaxGroup>& paxGroups);

    const std::list<dcqckx::PaxGroup>& paxGroups() const;
};

}//namespace dcqckx

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqbpr {

class PaxGroup: public iatci::PaxGroup
{
public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv,
             const boost::optional<BaggageDetails>& baggage,
             const boost::optional<ServiceDetails>& service);
};

//---------------------------------------------------------------------------------------

class FlightGroup: public iatci::FlightGroup
{
protected:
    std::list<dcqbpr::PaxGroup> m_paxGroups;

public:
    FlightGroup(const FlightDetails& outboundFlight,
                const boost::optional<FlightDetails>& inboundFlight,
                const std::list<dcqbpr::PaxGroup>& paxGroups);

    const std::list<dcqbpr::PaxGroup>& paxGroups() const;
};

}//namespace dcqbpr

/////////////////////////////////////////////////////////////////////////////////////////

class CkiParams: public IBaseParams
{
protected:
    OriginatorDetails                   m_org;
    boost::optional<CascadeHostDetails> m_cascade;
    dcqcki::FlightGroup                 m_fltGroup;

public:
    CkiParams(const OriginatorDetails& org,
              const boost::optional<CascadeHostDetails>& cascade,
              const dcqcki::FlightGroup& flg);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const dcqcki::FlightGroup&                         fltGroup() const;
};

//---------------------------------------------------------------------------------------

struct CkiParamsOld: public Params
{
protected:
    boost::optional<SeatDetails>        m_seat;
    boost::optional<BaggageDetails>     m_baggage;
    boost::optional<ReservationDetails> m_reserv;

public:
    CkiParamsOld(const OriginatorDetails& origin,
                 const PaxDetails& pax,
                 const FlightDetails& flight,
                 boost::optional<FlightDetails> flightFromPrevHost = boost::none,
                 boost::optional<SeatDetails> seat = boost::none,
                 boost::optional<BaggageDetails> baggage = boost::none,
                 boost::optional<ReservationDetails> reserv = boost::none,
                 boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                 boost::optional<ServiceDetails> serviceDetails = boost::none);

    const boost::optional<SeatDetails>&        seat() const;
    const boost::optional<BaggageDetails>&     baggage() const;
    const boost::optional<ReservationDetails>& reserv() const;
};

//---------------------------------------------------------------------------------------

struct CkuParams: public IBaseParams
{
protected:
    OriginatorDetails                   m_org;
    boost::optional<CascadeHostDetails> m_cascade;
    dcqcku::FlightGroup                 m_fltGroup;

public:
    CkuParams(const OriginatorDetails& org,
              const boost::optional<CascadeHostDetails>& cascade,
              const dcqcku::FlightGroup& flg);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const dcqcku::FlightGroup&                         fltGroup() const;

};

//---------------------------------------------------------------------------------------

struct CkuParamsOld: public Params
{
protected:
    boost::optional<UpdatePaxDetails>     m_updPax;
    boost::optional<UpdateServiceDetails> m_updService;
    boost::optional<UpdateSeatDetails>    m_updSeat;
    boost::optional<UpdateBaggageDetails> m_updBaggage;
    boost::optional<UpdateDocDetails>     m_updDoc;

public:
    CkuParamsOld(const OriginatorDetails& origin,
                 const PaxDetails& pax,
                 const FlightDetails& flight,
                 boost::optional<FlightDetails> flightFromPrevHost = boost::none,
                 boost::optional<UpdatePaxDetails> updPax = boost::none,
                 boost::optional<UpdateServiceDetails> updService = boost::none,
                 boost::optional<UpdateSeatDetails> updSeat = boost::none,
                 boost::optional<UpdateBaggageDetails> updBaggage = boost::none,
                 boost::optional<UpdateDocDetails> updDoc = boost::none,
                 boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                 boost::optional<ServiceDetails> serviceDetails = boost::none);

    const boost::optional<UpdatePaxDetails>&     updPax() const;
    const boost::optional<UpdateServiceDetails>& updService() const;
    const boost::optional<UpdateSeatDetails>&    updSeat() const;
    const boost::optional<UpdateBaggageDetails>& updBaggage() const;
    const boost::optional<UpdateDocDetails>&     updDoc() const;
};

//---------------------------------------------------------------------------------------

struct CkxParams: public IBaseParams
{
    OriginatorDetails                   m_org;
    boost::optional<CascadeHostDetails> m_cascade;
    dcqckx::FlightGroup                 m_fltGroup;

public:
    CkxParams(const OriginatorDetails& org,
              const boost::optional<CascadeHostDetails>& cascade,
              const dcqckx::FlightGroup& flg);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const dcqckx::FlightGroup&                         fltGroup() const;
};

//---------------------------------------------------------------------------------------

struct CkxParamsOld: public Params
{
protected:
    boost::optional<ReservationDetails> m_reserv;
    boost::optional<SeatDetails>        m_seat;
    boost::optional<BaggageDetails>     m_baggage;

public:
    CkxParamsOld(const OriginatorDetails& origin,
                 const PaxDetails& pax,
                 const FlightDetails& flight,
                 boost::optional<FlightDetails> flightFromPrevHost = boost::none,
                 boost::optional<CascadeHostDetails> cascadeDetails = boost::none,
                 boost::optional<ServiceDetails> serviceDetails = boost::none);

};

//---------------------------------------------------------------------------------------

struct PlfParams: public IBaseParams
{
protected:
    OriginatorDetails                   m_org;
    SelectPersonalDetails               m_personal;
    FlightDetails                       m_outboundFlight;
    boost::optional<FlightDetails>      m_inboundFlight;
    boost::optional<CascadeHostDetails> m_cascade;

public:
    PlfParams(const OriginatorDetails& org,
              const SelectPersonalDetails& personal,
              const FlightDetails& outboundFlight,
              const boost::optional<FlightDetails>& inboundFlight = boost::none,
              const boost::optional<CascadeHostDetails>& cascade = boost::none);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const SelectPersonalDetails& personal() const;
};

//---------------------------------------------------------------------------------------

struct SmfParams: public IBaseParams
{
protected:
    OriginatorDetails                   m_org;
    boost::optional<SeatRequestDetails> m_seatReq;
    FlightDetails                       m_outboundFlight;
    boost::optional<FlightDetails>      m_inboundFlight;
    boost::optional<CascadeHostDetails> m_cascade;

public:
    SmfParams(const OriginatorDetails& origin,
              const boost::optional<SeatRequestDetails>& seatReq,
              const FlightDetails& outboundFlight,
              const boost::optional<FlightDetails>& inboundFlight = boost::none,
              const boost::optional<CascadeHostDetails>& cascade = boost::none);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const boost::optional<SeatRequestDetails>& seatRequest() const;
};

//---------------------------------------------------------------------------------------

struct BprParams: public IBaseParams
{
    OriginatorDetails                   m_org;
    boost::optional<CascadeHostDetails> m_cascade;
    dcqbpr::FlightGroup                 m_fltGroup;

public:
    BprParams(const OriginatorDetails& org,
              const boost::optional<CascadeHostDetails>& cascade,
              const dcqbpr::FlightGroup& flg);

    virtual const OriginatorDetails&                   org() const;
    virtual const FlightDetails&                       outboundFlight() const;
    virtual const boost::optional<FlightDetails>&      inboundFlight() const;
    virtual const boost::optional<CascadeHostDetails>& cascade() const;

    const dcqbpr::FlightGroup&                         fltGroup() const;
};

//---------------------------------------------------------------------------------------

struct BprParamsOld: public CkiParamsOld
{
public:
    BprParamsOld(const OriginatorDetails& origin,
                 const PaxDetails& pax,
                 const FlightDetails& flight,
                 boost::optional<FlightDetails> flightFromPrevHost = boost::none,
                 boost::optional<CascadeHostDetails> cascadeDetails = boost::none);
};

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcrcka {

class PaxGroup: public iatci::PaxGroup
{
    friend class dcrcka::Result;
    friend class boost::serialization::access;

protected:
    boost::optional<FlightSeatDetails> m_seat;
    boost::optional<FlightSeatDetails> m_infantSeat;

public:
    PaxGroup(const PaxDetails& pax,
             const boost::optional<ReservationDetails>& reserv,
             const boost::optional<FlightSeatDetails>& seat,
             const boost::optional<BaggageDetails>& baggage,
             const boost::optional<ServiceDetails>& service,
             const boost::optional<DocDetails>& doc,
             const boost::optional<AddressDetails>& address,
             const boost::optional<PaxDetails>& infant = boost::none,
             const boost::optional<DocDetails>& infantDoc = boost::none,
             const boost::optional<FlightSeatDetails>& infantSeat = boost::none);

    const boost::optional<FlightSeatDetails>& seat() const;
    const boost::optional<FlightSeatDetails>& infantSeat() const;

protected:
    PaxGroup() {} // for boost serialization only
};

//---------------------------------------------------------------------------------------

struct Result
{
    friend class boost::serialization::access;

    enum Action_e
    {
        Checkin,
        Cancel,
        Update,
        Reprint,
        Passlist,
        Seatmap,
        SeatmapForPassenger
    };

    enum Status_e
    {
        Ok,
        OkWithNoData,
        Failed,
        RecoverableError
    };

protected:
    Action_e                            m_action;
    Status_e                            m_status;
    FlightDetails                       m_flight;
    std::list<dcrcka::PaxGroup>         m_paxGroups;
    boost::optional<SeatmapDetails>     m_seatmap;
    boost::optional<CascadeHostDetails> m_cascade;
    boost::optional<ErrorDetails>       m_error;
    boost::optional<WarningDetails>     m_warning;
    boost::optional<EquipmentDetails>   m_equipment;

    Result(Action_e action,
           Status_e status,
           const FlightDetails& flight,
           const std::list<dcrcka::PaxGroup>& paxGroups,
           const boost::optional<SeatmapDetails>& seatmap,
           const boost::optional<CascadeHostDetails>& cascade,
           const boost::optional<ErrorDetails>& error,
           const boost::optional<WarningDetails>& warning,
           const boost::optional<EquipmentDetails>& equipment);

public:
    static Result makeResult(Action_e action,
                             Status_e status,
                             const FlightDetails& flight,
                             const std::list<dcrcka::PaxGroup>& paxGroups,
                             boost::optional<SeatmapDetails> seatmap,
                             boost::optional<CascadeHostDetails> cascade,
                             boost::optional<ErrorDetails> error,
                             boost::optional<WarningDetails> warning,
                             boost::optional<EquipmentDetails> equipment);

    static Result makeCancelResult(Status_e status,
                                   const FlightDetails& flight,
                                   const std::list<dcrcka::PaxGroup>& paxGroups = std::list<dcrcka::PaxGroup>(),
                                   boost::optional<CascadeHostDetails> cascade = boost::none,
                                   boost::optional<ErrorDetails> error = boost::none,
                                   boost::optional<WarningDetails> warning = boost::none,
                                   boost::optional<EquipmentDetails> equipment = boost::none);

    static Result makeSeatmapResult(Status_e status,
                                    const FlightDetails& flight,
                                    const SeatmapDetails& seatmap,
                                    boost::optional<CascadeHostDetails> cascade = boost::none,
                                    boost::optional<ErrorDetails> error = boost::none,
                                    boost::optional<WarningDetails> warning = boost::none,
                                    boost::optional<EquipmentDetails> equipment = boost::none);

    static Result makeFailResult(Action_e action,
                                 const FlightDetails& flight,
                                 const ErrorDetails& error);

    Action_e                                   action() const;
    Status_e                                   status() const;
    const FlightDetails&                       flight() const;
    boost::optional<PaxDetails> 	       pax() const;
    const std::list<dcrcka::PaxGroup>&         paxGroups() const;
    const boost::optional<SeatmapDetails>&     seatmap() const;
    const boost::optional<CascadeHostDetails>& cascade() const;
    const boost::optional<ErrorDetails>&       error() const;
    const boost::optional<WarningDetails>&     warning() const;
    const boost::optional<EquipmentDetails>&   equipment() const;

    std::string actionAsString() const;
    std::string statusAsString() const;

    void toXml(xmlNodePtr node) const;
    void toSmpXml(xmlNodePtr node) const;
    void toSmpUpdXml(xmlNodePtr node,
                     const Seat& oldSeat,
                     const Seat& newSeat) const;

    static Action_e strToAction(const std::string& a);
    static Status_e strToStatus(const std::string& s);

protected:
    Result()
        : m_status(Failed)
    {} // for boost serialization only
};

}// namespace dcrcka

}//namespace iatci
