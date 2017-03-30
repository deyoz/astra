#include "iatci_types.h"
#include "iatci_help.h"
#include "xml_unit.h"
#include "date_time.h"
#include "astra_locale_adv.h"
#include "astra_msg.h"

#include <serverlib/exception.h>
#include <serverlib/xml_tools.h>
#include <serverlib/xmllibcpp.h>

#include <etick/exceptions.h>

#include <ostream>
#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace iatci {

using namespace Ticketing;
using namespace Ticketing::TickExceptions;

MagicTab MagicTab::fromNeg(int gt)
{
    ASSERT(gt < 0);
    const std::string pts = std::to_string(std::abs(gt));
    std::string p = pts.substr(0, pts.length() - 1);
    std::string t = pts.substr(pts.length() - 1);
    LogTrace(TRACE5) << "gt: " << gt;
    LogTrace(TRACE5) << "p: " << p;
    LogTrace(TRACE5) << "t: " << t;
    return MagicTab(std::atoi(p.c_str()), std::atoi(t.c_str()));
}

int MagicTab::toNeg() const
{
    std::ostringstream s;
    s << "-";
    s << m_grpId;
    s << m_tabInd;
    LogTrace(TRACE5) << "P: " << m_grpId;
    LogTrace(TRACE5) << "T: " << m_tabInd;
    LogTrace(TRACE5) << "GT: " << std::atoi(s.str().c_str());
    return std::atoi(s.str().c_str());
}

//---------------------------------------------------------------------------------------

Seat::Seat(const std::string& row, const std::string& col)
    : m_row(row), m_col(col)
{}

Seat Seat::fromStr(const std::string& str)
{
    LogTrace(TRACE5) << __FUNCTION__ << " " << str;
    if(!(str.length() > 1 && str.size() < 5)) {
        LogTrace(TRACE5) << "invalid seat string [" << str << "]";
        throw tick_soft_except(STDLOG, AstraErr::INV_SEAT);
    }

    std::string row = std::string(str.begin(), --str.end());
    std::string col = str.substr(str.length() - 1);
    LogTrace(TRACE5) << "row:" << row << " "
                     << "col:" << col;

    return Seat(row, col);
}

std::string Seat::toStr() const
{
    return m_row + m_col;
}

bool operator==(const Seat& left, const Seat& right)
{
    return (left.row() == right.row() &&
            left.col() == right.col());
}

bool operator!=(const Seat& left, const Seat& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

OriginatorDetails::OriginatorDetails(const std::string& airl, const std::string& point)
    : m_airline(airl),
      m_port(point)
{}

const std::string& OriginatorDetails::airline() const
{
    return m_airline;
}

const std::string& OriginatorDetails::port() const
{
    return m_port;
}

//---------------------------------------------------------------------------------------

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
      m_depPort(depPoint),
      m_arrPort(arrPoint),
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

const std::string& FlightDetails::depPort() const
{
    return m_depPort;
}

const std::string& FlightDetails::arrPort() const
{
    return m_arrPort;
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

//---------------------------------------------------------------------------------------

DocDetails::DocDetails(const std::string& docType,
                       const std::string& issueCountry,
                       const std::string& no,
                       const std::string& surname,
                       const std::string& name,
                       const std::string& secondName,
                       const std::string& gender,
                       const std::string& nationality,
                       const boost::gregorian::date& birthDate,
                       const boost::gregorian::date& expiryDate)
    : m_docType(docType), m_issueCountry(issueCountry), m_no(no),
      m_surname(surname), m_name(name), m_secondName(secondName),
      m_gender(gender), m_nationality(nationality),
      m_birthDate(birthDate), m_expiryDate(expiryDate)
{
}

const std::string& DocDetails::docType() const
{
    return m_docType;
}

const std::string& DocDetails::issueCountry() const
{
    return m_issueCountry;
}

const std::string& DocDetails::no() const
{
    return m_no;
}

const std::string& DocDetails::surname() const
{
    return m_surname;
}

const std::string& DocDetails::name() const
{
    return m_name;
}

const std::string& DocDetails::secondName() const
{
    return m_secondName;
}

const std::string& DocDetails::gender() const
{
    return m_gender;
}

const std::string& DocDetails::nationality() const
{
    return m_nationality;
}

const boost::gregorian::date& DocDetails::birthDate() const
{
    return m_birthDate;
}

const boost::gregorian::date& DocDetails::expiryDate() const
{
    return m_expiryDate;
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

UpdatePaxDetails::UpdatePaxDetails(UpdateActionCode_e actionCode,
                                   const std::string& surname,
                                   const std::string& name,
                                   const std::string& qryRef)
    : UpdateDetails(actionCode),
      m_surname(surname), m_name(name), m_qryRef(qryRef)
{}

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

//---------------------------------------------------------------------------------------

ReservationDetails::ReservationDetails(const std::string& rbd)
    : m_rbd(rbd)
{}

const std::string& ReservationDetails::rbd() const
{
    return m_rbd;
}

Ticketing::SubClass ReservationDetails::subclass() const
{
    if(m_rbd.empty()) {
        return Ticketing::SubClass();
    }

    return Ticketing::SubClass(m_rbd);
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

UpdateDocDetails::UpdateDocDetails(UpdateActionCode_e actionCode,
                                   const std::string& docType,
                                   const std::string& issueCountry,
                                   const std::string& no,
                                   const std::string& surname,
                                   const std::string& name,
                                   const std::string& secondName,
                                   const std::string& gender,
                                   const std::string& nationality,
                                   const boost::gregorian::date& birthDate,
                                   const boost::gregorian::date& expiryDate)
    : UpdateDetails(actionCode),
      DocDetails(docType, issueCountry, no,
                 surname, name, secondName,
                 gender, nationality, birthDate, expiryDate)
{}

//---------------------------------------------------------------------------------------

UpdateSeatDetails::UpdateSeatDetails(UpdateActionCode_e actionCode,
                                     const std::string& seat,
                                     SmokeIndicator_e smokeInd)
    : UpdateDetails(actionCode), SeatDetails(seat, smokeInd)
{
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

SelectPersonalDetails::SelectPersonalDetails(const std::string& surname,
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

SelectPersonalDetails::SelectPersonalDetails(const PaxDetails& pax)
    : PaxDetails(pax)
{
}

const std::string& SelectPersonalDetails::rbd() const
{
    return m_rbd;
}

const std::string& SelectPersonalDetails::seat() const
{
    return m_seat;
}

const std::string& SelectPersonalDetails::securityId() const
{
    return m_securityId;
}

const std::string& SelectPersonalDetails::recloc() const
{
    return m_recloc;
}

const std::string& SelectPersonalDetails::tickNum() const
{
    return m_tickNum;
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

UpdateBaggageDetails::UpdateBaggageDetails(UpdateActionCode_e actionCode,
                                           unsigned numOfPieces, unsigned weight)
    : UpdateDetails(actionCode), BaggageDetails(numOfPieces, weight)
{}

//---------------------------------------------------------------------------------------

ServiceDetails::SsrInfo::SsrInfo(const std::string& ssrCode, const std::string& ssrText,
                                 bool isInftTicket, const std::string& freeText,
                                 const std::string& airline, unsigned quantity)
    : m_ssrCode(ssrCode), m_ssrText(ssrText),
      m_isInfantTicket(isInftTicket), m_freeText(freeText),
      m_airline(airline), m_quantity(quantity)
{
    const size_t MaxFreeTextLen = 70;
    if(m_freeText.length() > MaxFreeTextLen) {
        throw AstraLocale::UserException("MSG.TOO_LONG_SSR_FREE_TEXT");
    }
}

const std::string& ServiceDetails::SsrInfo::ssrCode() const
{
    return m_ssrCode;
}

const std::string& ServiceDetails::SsrInfo::ssrText() const
{
    return m_ssrText;
}

bool ServiceDetails::SsrInfo::isInfantTicket() const
{
    return m_isInfantTicket;
}

const std::string& ServiceDetails::SsrInfo::freeText() const
{
    return m_freeText;
}

const std::string& ServiceDetails::SsrInfo::airline() const
{
    return m_airline;
}

unsigned ServiceDetails::SsrInfo::quantity() const
{
    return m_quantity;
}

Ticketing::TicketCpn_t ServiceDetails::SsrInfo::toTicketCpn() const
{
    ASSERT(m_ssrCode == "TKNE");
    ASSERT(m_ssrText.length() == 14); // ticknum(13)+cpnnum(1)
    return Ticketing::TicketCpn_t(m_ssrText.substr(0, 13),
                                  boost::lexical_cast<int>(m_ssrText.substr(13, 1)));
}

//

ServiceDetails::ServiceDetails(const std::string& osi)
    : m_osi(osi)
{}

ServiceDetails::ServiceDetails(const std::list<SsrInfo>& lSsr,
                               const std::string& osi)
    : m_lSsr(lSsr),
      m_osi(osi)
{}

const std::list<ServiceDetails::SsrInfo>& ServiceDetails::lSsr() const
{
    return m_lSsr;
}

const std::string& ServiceDetails::osi() const
{
    return m_osi;
}

void ServiceDetails::addSsr(const ServiceDetails::SsrInfo& ssr)
{
    m_lSsr.push_back(ssr);
}

void ServiceDetails::addSsr(const std::string& ssrCode, const std::string& ssrFreeText)
{
    m_lSsr.push_back(ServiceDetails::SsrInfo(ssrCode, "", false, ssrFreeText));
}

void ServiceDetails::addSsrTkne(const std::string& tickNum, bool isInftTicket)
{
    m_lSsr.push_back(ServiceDetails::SsrInfo("TKNE", tickNum, isInftTicket));
}

void ServiceDetails::addSsrTkne(const std::string& tickNum, unsigned couponNum, bool inftTicket)
{
    std::ostringstream tkne;
    tkne << tickNum << couponNum;
    addSsrTkne(tkne.str(), inftTicket);
}

void ServiceDetails::addSsrFqtv(const std::string& fqtvCode)
{
    m_lSsr.push_back(ServiceDetails::SsrInfo("FQTV", fqtvCode));
}

boost::optional<Ticketing::TicketCpn_t> ServiceDetails::findTicketCpn() const
{
    BOOST_FOREACH(const ServiceDetails::SsrInfo& ssr, lSsr()) {
        if(ssr.ssrCode() == "TKNE") {
            return ssr.toTicketCpn();
        }
    }

    return boost::none;
}

//---------------------------------------------------------------------------------------

UpdateServiceDetails::UpdSsrInfo::UpdSsrInfo(UpdateActionCode_e actionCode,
                                             const std::string& ssrCode,
                                             const std::string& ssrText,
                                             bool isInftTicket,
                                             const std::string& freeText,
                                             const std::string& airline,
                                             unsigned qtty)
    : UpdateDetails(actionCode),
      ServiceDetails::SsrInfo(ssrCode, ssrText, isInftTicket,
                              freeText, airline, qtty)
{}

UpdateServiceDetails::UpdateServiceDetails(UpdateActionCode_e actionCode)
    : UpdateDetails(actionCode)
{}

const std::list<UpdateServiceDetails::UpdSsrInfo>& UpdateServiceDetails::lSsr() const
{
    return m_lUpdSsr;
}

void UpdateServiceDetails::addSsr(const UpdateServiceDetails::UpdSsrInfo& updSsr)
{
    m_lUpdSsr.push_back(updSsr);
}

//---------------------------------------------------------------------------------------

SeatRequestDetails::SeatRequestDetails(const std::string& cabinClass,
                                       SmokeIndicator_e smokeInd)
    : SeatDetails(smokeInd),
      m_cabinClass(cabinClass)
{}

const std::string& SeatRequestDetails::cabinClass() const
{
    return m_cabinClass;
}

//---------------------------------------------------------------------------------------

RowRange::RowRange(unsigned firstRow, unsigned lastRow)
    : m_firstRow(firstRow), m_lastRow(lastRow)
{}

unsigned RowRange::firstRow() const
{
    return m_firstRow;
}

unsigned RowRange::lastRow() const
{
    return m_lastRow;
}

//---------------------------------------------------------------------------------------

SeatColumnDetails::SeatColumnDetails(const std::string& column,
                                     const std::string& desc1,
                                     const std::string& desc2)
    : m_column(column), m_desc1(desc1), m_desc2(desc2)
{}

const std::string& SeatColumnDetails::column() const
{
    return m_column;
}

const std::string& SeatColumnDetails::desc1() const
{
    return m_desc1;
}

const std::string& SeatColumnDetails::desc2() const
{
    return m_desc2;
}

void SeatColumnDetails::setAisle()
{
    m_desc1 = "A";
}

//---------------------------------------------------------------------------------------

CabinDetails::CabinDetails(const std::string& classDesignator,
                           const RowRange& rowRange,
                           const std::string& defaultSeatOccupation,
                           const std::list<SeatColumnDetails>& seatColumns,
                           const std::string& deck,
                           boost::optional<RowRange> smokingArea,
                           boost::optional<RowRange> overwingArea)
    : m_classDesignator(classDesignator), m_rowRange(rowRange),
      m_defSeatOccupation(defaultSeatOccupation), m_seatColumns(seatColumns),
      m_deck(deck), m_smokingArea(smokingArea), m_overwingArea(overwingArea)
{}

const std::string& CabinDetails::classDesignator() const
{
    return m_classDesignator;
}

const RowRange& CabinDetails::rowRange() const
{
    return m_rowRange;
}

const std::string& CabinDetails::defaultSeatOccupation() const
{
    return m_defSeatOccupation;
}

const std::list<SeatColumnDetails>& CabinDetails::seatColumns() const
{
    return m_seatColumns;
}

const std::string& CabinDetails::deck() const
{
    return m_deck;
}

const boost::optional<RowRange>& CabinDetails::smokingArea() const
{
    return m_smokingArea;
}

const boost::optional<RowRange>& CabinDetails::overwingArea() const
{
    return m_overwingArea;
}

//---------------------------------------------------------------------------------------

SeatOccupationDetails::SeatOccupationDetails(const std::string& column,
                                             const std::string& occupation,
                                             const std::list<std::string>& lCharacteristics)
    : m_column(column), m_occupation(occupation),
      m_lCharacteristics(lCharacteristics)
{}

const std::string& SeatOccupationDetails::column() const
{
    return m_column;
}

const std::string& SeatOccupationDetails::occupation() const
{
    return m_occupation;
}

const std::list<std::string>& SeatOccupationDetails::lCharacteristics() const
{
    return m_lCharacteristics;
}

void SeatOccupationDetails::setOccupied()
{
    m_occupation = "O";
}

//---------------------------------------------------------------------------------------

RowDetails::RowDetails(const std::string& row,
                       const std::list<SeatOccupationDetails>& lOccupationDetails,
                       const std::string& characteristic)
    : m_row(row),
      m_lOccupationDetails(lOccupationDetails),
      m_characteristic(characteristic)
{}

RowDetails::RowDetails(const unsigned& row,
                       const std::list<SeatOccupationDetails>& lOccupationDetails)
    : m_row(boost::lexical_cast<std::string>(row)),
      m_lOccupationDetails(lOccupationDetails)
{}

const std::string& RowDetails::row() const
{
    return m_row;
}

const std::list<SeatOccupationDetails>& RowDetails::lOccupationDetails() const
{
    return m_lOccupationDetails;
}

const std::string& RowDetails::characteristic() const
{
    return m_characteristic;
}

//---------------------------------------------------------------------------------------

EquipmentDetails::EquipmentDetails(const std::string& equipment)
    : m_equipment(equipment)
{}

const std::string& EquipmentDetails::equipment() const
{
    return m_equipment;
}

//---------------------------------------------------------------------------------------

SeatmapDetails::SeatmapDetails(const std::list<CabinDetails>& lCabinDetails,
                               const std::list<RowDetails>& lRowDetails,
                               boost::optional<SeatRequestDetails> seatRequestDetails)
    : m_lCabin(lCabinDetails), m_lRow(lRowDetails),
      m_seatRequest(seatRequestDetails)
{}

const boost::optional<SeatRequestDetails>& SeatmapDetails::seatRequest() const
{
    return m_seatRequest;
}

const std::list<CabinDetails>& SeatmapDetails::lCabin() const
{
    return m_lCabin;
}

const std::list<RowDetails>& SeatmapDetails::lRow() const
{
    return m_lRow;
}

//---------------------------------------------------------------------------------------

CascadeHostDetails::CascadeHostDetails(const std::string& host)
{
    m_hostAirlines.push_back(host);
}

CascadeHostDetails::CascadeHostDetails(const std::string& origAirl,
                                       const std::string& origPort)
    : m_originAirline(origAirl),
      m_originPort(origPort)
{}

const std::string& CascadeHostDetails::originAirline() const
{
    return m_originAirline;
}

const std::string& CascadeHostDetails::originPort() const
{
    return m_originPort;
}

const std::list<std::string>& CascadeHostDetails::hostAirlines() const
{
    return m_hostAirlines;
}

void CascadeHostDetails::addHostAirline(const std::string& hostAirline)
{
    m_hostAirlines.push_back(hostAirline);
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

WarningDetails::WarningDetails(const Ticketing::ErrMsg_t& warningCode,
                               const std::string& warningDesc)
    : m_warningCode(warningCode),
      m_warningDesc(warningDesc)
{}

const Ticketing::ErrMsg_t& WarningDetails::warningCode() const
{
    return m_warningCode;
}

const std::string& WarningDetails::warningDesc() const
{
    return m_warningDesc;
}

//---------------------------------------------------------------------------------------

namespace dcrcka {

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<FlightSeatDetails>& seat,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<DocDetails>& doc,
                   const boost::optional<AddressDetails>& address)
    : iatci::PaxGroup(pax, reserv, baggage, service, doc, address),
      m_seat(seat)
{}

const boost::optional<FlightSeatDetails>& PaxGroup::seat() const
{
    return m_seat;
}

//---------------------------------------------------------------------------------------


Result::Result(Action_e action,
               Status_e status,
               const FlightDetails& flight,
               const std::list<dcrcka::PaxGroup>& paxGroups,
               const boost::optional<SeatmapDetails>& seatmap,
               const boost::optional<CascadeHostDetails>& cascade,
               const boost::optional<ErrorDetails>& error,
               const boost::optional<WarningDetails>& warning,
               const boost::optional<EquipmentDetails>& equipment)
    : m_action(action),
      m_status(status),
      m_flight(flight),
      m_paxGroups(paxGroups),
      m_seatmap(seatmap),
      m_cascade(cascade),
      m_error(error),
      m_warning(warning),
      m_equipment(equipment)
{}

Result Result::makeResult(Action_e action,
                          Status_e status,
                          const FlightDetails& flight,
                          const std::list<dcrcka::PaxGroup>& paxGroups,
                          boost::optional<SeatmapDetails> seatmap,
                          boost::optional<CascadeHostDetails> cascade,
                          boost::optional<ErrorDetails> error,
                          boost::optional<WarningDetails> warning,
                          boost::optional<EquipmentDetails> equipment)
{
    return Result(action,
                  status,
                  flight,
                  paxGroups,
                  seatmap,
                  cascade,
                  error,
                  warning,
                  equipment);
}

Result Result::makeCancelResult(Status_e status,
                                const FlightDetails& flight,
                                const std::list<dcrcka::PaxGroup>& paxGroups,
                                boost::optional<CascadeHostDetails> cascade,
                                boost::optional<ErrorDetails> error,
                                boost::optional<WarningDetails> warning,
                                boost::optional<EquipmentDetails> equipment)
{
    return makeResult(Cancel,
                      status,
                      flight,
                      paxGroups,
                      boost::none,
                      cascade,
                      error,
                      warning,
                      equipment);
}

Result Result::makeSeatmapResult(Status_e status,
                                 const FlightDetails& flight,
                                 const SeatmapDetails& seatmap,
                                 boost::optional<CascadeHostDetails> cascade,
                                 boost::optional<ErrorDetails> error,
                                 boost::optional<WarningDetails> warning,
                                 boost::optional<EquipmentDetails> equipment)
{
    return makeResult(Seatmap,
                      status,
                      flight,
                      std::list<dcrcka::PaxGroup>(),
                      seatmap,
                      cascade,
                      error,
                      warning,
                      equipment);
}

Result Result::makeFailResult(Action_e action,
                              const FlightDetails& flight,
                              const ErrorDetails& error)
{
    return makeResult(action,
                      Failed,
                      flight,
                      std::list<dcrcka::PaxGroup>(),
                      boost::none,
                      boost::none,
                      error,
                      boost::none,
                      boost::none);
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

const std::list<dcrcka::PaxGroup>& Result::paxGroups() const
{
    return m_paxGroups;
}

boost::optional<PaxDetails> Result::pax() const
{
    if(paxGroups().empty()) {
        return boost::none;
    }
  
    return m_paxGroups.front().pax();  
}

const boost::optional<SeatmapDetails>& Result::seatmap() const
{
    return m_seatmap;
}

const boost::optional<CascadeHostDetails>& Result::cascade() const
{
    return m_cascade;
}

const boost::optional<ErrorDetails>& Result::error() const
{
    return m_error;
}

const boost::optional<WarningDetails>& Result::warning() const
{
    return m_warning;
}

const boost::optional<EquipmentDetails>& Result::equipment() const
{
    return m_equipment;
}

Result::Action_e Result::strToAction(const std::string& a)
{
    if     (a == "I") return Checkin;
    else if(a == "X") return Cancel;
    else if(a == "U") return Update;
    else if(a == "B") return Reprint;
    else if(a == "P") return Passlist;
    else if(a == "S") return Seatmap;
    else if(a == "T") return SeatmapForPassenger;
    else {
        throw EXCEPTIONS::Exception("Unknown iatci action code: %s", a.c_str());
    }
}

Result::Status_e Result::strToStatus(const std::string& s)
{
    if     (s == "O") return Ok;
    else if(s == "P") return OkWithNoData;
    else if(s == "X") return Failed;
    else if(s == "N") return RecoverableError;
    else {
        LogError(STDLOG) << "Unknown status string: " << s;
        return Failed;
    }
}

std::string Result::actionAsString() const
{
    switch(m_action)
    {
    case Checkin:             return "I";
    case Cancel:              return "X";
    case Update:              return "U";
    case Reprint:             return "B";
    case Passlist:            return "P";
    case Seatmap:             return "S";
    case SeatmapForPassenger: return "T";
    }

    throw EXCEPTIONS::Exception("Unknown iatci action code value: %d", m_action);
}

std::string Result::statusAsString() const
{
    switch(m_status)
    {
    case Ok:               return "O";
    case OkWithNoData:     return "P";
    case Failed:           return "X";
    case RecoverableError: return "N";
    }

    throw EXCEPTIONS::Exception("Unknown status value: %d", m_status);
}

void Result::toXml(xmlNodePtr node) const
{
    xmlNodePtr segNode = newChild(node, "segment");

    xmlNodePtr tripHeaderNode = newChild(segNode, "tripheader");
    NewTextChild(tripHeaderNode, "flight",  fullFlightString(flight()));
    NewTextChild(tripHeaderNode, "airline", flight().airline());
    NewTextChild(tripHeaderNode, "aircode", airlineAccode(flight().airline()));
    NewTextChild(tripHeaderNode, "flt_no",  flightString(flight()));
    NewTextChild(tripHeaderNode, "suffix",  "");
    NewTextChild(tripHeaderNode, "airp",    flight().depPort());
    NewTextChild(tripHeaderNode, "scd_out_local", depDateTimeString(flight()));
    NewTextChild(tripHeaderNode, "pr_etl_only", "0"); // TODO
    NewTextChild(tripHeaderNode, "pr_etstatus", "0"); // TODO
    NewTextChild(tripHeaderNode, "pr_no_ticket_check", "0"); // TODO)
    NewTextChild(tripHeaderNode, "pr_auto_pt_print", 0); // TODO
    NewTextChild(tripHeaderNode, "pr_auto_pt_print_reseat", 0); // TODO

    xmlNodePtr tripDataNode = newChild(segNode, "tripdata");
    xmlNodePtr airpsNode = newChild(tripDataNode, "airps");
    xmlNodePtr airpNode = newChild(airpsNode, "airp");
    NewTextChild(airpNode, "point_id", -1);
    NewTextChild(airpNode, "airp_code", airportCode(flight().arrPort()));
    NewTextChild(airpNode, "city_code", airportCityCode(flight().arrPort()));
    NewTextChild(airpNode, "target_view", fullAirportString(flight().arrPort()));
    xmlNodePtr checkInfoNode = newChild(airpNode, "check_info");
    xmlNodePtr passNode = newChild(checkInfoNode, "pass"); // TODO
    xmlNodePtr crewNode = newChild(checkInfoNode, "crew"); // TODO
    xmlNodePtr classesNode = newChild(tripDataNode, "classes"); // TODO
    xmlNodePtr gatesNode = newChild(tripDataNode, "gates"); // TODO
    xmlNodePtr hallsNode = newChild(tripDataNode, "halls"); // TODO
    xmlNodePtr markFltsNode = newChild(tripDataNode, "mark_flights"); // TODO

    NewTextChild(segNode, "grp_id", -1);
    NewTextChild(segNode, "point_dep", -1);
    NewTextChild(segNode, "airp_dep", flight().depPort());
    NewTextChild(segNode, "point_arv", -1);
    NewTextChild(segNode, "airp_arv", flight().arrPort());
    NewTextChild(segNode, "class", "�"); // TODO
    NewTextChild(segNode, "status", "K"); // TODO
    NewTextChild(segNode, "bag_refuse", ""); // TODO
    NewTextChild(segNode, "piece_concept", 0); // TODO
    NewTextChild(segNode, "tid", 0); // TODO
    NewTextChild(segNode, "city_arv", airportCityCode(flight().arrPort()));
    //xmlNodePtr markFltNode = newChild(segNode, "mark_flight"); // TODO

    xmlNodePtr paxesNode = newChild(segNode, "passengers");
    int currPax = 0;
    for(const auto& pxg: m_paxGroups)
    { 
        xmlNodePtr paxNode = newChild(paxesNode, "pax");
        NewTextChild(paxNode, "pax_id", -(++currPax));
        NewTextChild(paxNode, "surname", pxg.pax().surname());
        NewTextChild(paxNode, "name", pxg.pax().name());
        NewTextChild(paxNode, "pers_type", paxTypeString(pxg.pax())); // TODO
        NewTextChild(paxNode, "seat_no", pxg.seat() ? pxg.seat()->seat() : "");
        NewTextChild(paxNode, "seat_type", ""); // TODO
        NewTextChild(paxNode, "seats", 1); // TODO
        NewTextChild(paxNode, "refuse", ""); // TODO
        NewTextChild(paxNode, "reg_no", pxg.seat() ? pxg.seat()->securityId() : "");
        NewTextChild(paxNode, "subclass", "�"); // TODO
        NewTextChild(paxNode, "bag_pool_num", ""); // TODO
        NewTextChild(paxNode, "tid", 0); // TODO
        boost::optional<Ticketing::TicketCpn_t> tickCpn;
        std::string tickRem;
        if(pxg.service()) {
            tickCpn = pxg.service()->findTicketCpn();
            tickRem = "TKNE";
        }
        if(tickCpn) {
            NewTextChild(paxNode, "ticket_no", tickCpn->ticket().get());
            NewTextChild(paxNode, "coupon_no", tickCpn->cpn().get());
        }
        else {
            NewTextChild(paxNode, "ticket_no");
            NewTextChild(paxNode, "coupon_no");
        }

        NewTextChild(paxNode, "ticket_rem", tickRem);
        NewTextChild(paxNode, "ticket_confirm", "1"); // TODO

        xmlNodePtr docNode = newChild(paxNode, "document");
        LogTrace(TRACE3) << "about to show doc";
        if(pxg.doc())
        {
            tst();
            NewTextChild(docNode, "type", pxg.doc()->docType());
            NewTextChild(docNode, "issue_country", pxg.doc()->issueCountry());
            NewTextChild(docNode, "no", pxg.doc()->no());
            NewTextChild(docNode, "nationality", pxg.doc()->nationality());
            if(!pxg.doc()->birthDate().is_not_a_date()) {
                NewTextChild(docNode, "birth_date", BASIC::date_time::boostDateToAstraFormatStr(pxg.doc()->birthDate()));
            }
            NewTextChild(docNode, "gender", pxg.doc()->gender());
            NewTextChild(docNode, "surname", pxg.doc()->surname());
            NewTextChild(docNode, "first_name", pxg.doc()->name());
            if(!pxg.doc()->secondName().empty()) {
                NewTextChild(docNode, "second_name", pxg.doc()->secondName());
            }
            if(!pxg.doc()->expiryDate().is_not_a_date()) {
                NewTextChild(docNode, "expiry_date", BASIC::date_time::boostDateToAstraFormatStr(pxg.doc()->expiryDate()));
            }
        }

        NewTextChild(paxNode, "pr_norec", 0); // TODO
        NewTextChild(paxNode, "pr_bp_print", 0); // TODO
        NewTextChild(paxNode, "pr_bi_print", 0); // TODO


        xmlNodePtr paxRemsNode = newChild(paxNode, "rems");
        if(pxg.service())
        {
            for(const ServiceDetails::SsrInfo& ssr: pxg.service()->lSsr()) {
                std::string remCode = ssr.ssrCode(),
                            remText;
                if(remCode == "TKNE") continue;

                /*if(remCode == "FQTV") {
                    remText = ssr.ssrText();
                } else {
                    remText = ssr.freeText();
                }*/

                remText = ssr.freeText();

                xmlNodePtr paxRemNode = newChild(paxRemsNode, "rem");
                NewTextChild(paxRemNode, "rem_code", remCode);
                NewTextChild(paxRemNode, "rem_text", remText);
            }
        }
    }

    NewTextChild(segNode, "paid_bag_emd", ""); // TODO
    xmlNodePtr tripCountersNode = newChild(segNode, "tripcounters"); // TODO
    NewTextChild(segNode, "load_residue", ""); // TODO
}

void Result::toSmpXml(xmlNodePtr node) const
{
    ASSERT(seatmap());
    PlaceMatrix placeMatrix = createPlaceMatrix(seatmap().get());

    NewTextChild(node, "trip",        fullFlightString(flight(), false));
    NewTextChild(node, "craft",       ""); // TODO get it
    NewTextChild(node, "bort",        ""); // TODO get it
    NewTextChild(node, "travel_time", depTimeString(flight()));
    NewTextChild(node, "comp_id",     -1);
    NewTextChild(node, "descr",       "");

    xmlNodePtr salonsNode = newChild(node, "salons"); // TODO fill node properties
    xmlSetProp(salonsNode, "pr_lat_seat", 1);
    xmlSetProp(salonsNode, "RFISCMode",   0);

    xmlNodePtr filterRoutesNode = newChild(salonsNode, "filterRoutes");
    NewTextChild(filterRoutesNode, "point_dep", -1);
    NewTextChild(filterRoutesNode, "point_arv", -1);

    xmlNodePtr itemsNode = newChild(filterRoutesNode, "items");

    xmlNodePtr depItemNode = newChild(itemsNode, "item");
    NewTextChild(depItemNode, "point_id", -1);
    NewTextChild(depItemNode, "airp", airportCode(flight().depPort()));

    xmlNodePtr arrItemNode = newChild(itemsNode, "item");
    NewTextChild(arrItemNode, "point_id", -1);
    NewTextChild(arrItemNode, "airp", airportCode(flight().arrPort()));

    for(const auto& pm: placeMatrix.placeLists()) {
        PlaceMatrix::Limits limits = pm.second.limits();
        xmlNodePtr placeListNode = newChild(salonsNode, "placelist");
        xmlSetProp(placeListNode, "num", pm.first);
        xmlSetProp(placeListNode, "xcount", limits.width() ? limits.width() + 1 : 0);
        xmlSetProp(placeListNode, "ycount", limits.height() ? limits.height() + 1 : 0);

        for(const auto& pl: pm.second.places()) {
            xmlNodePtr placeNode = newChild(placeListNode, "place");
            NewTextChild(placeNode, "x", (int)pl.first.m_x);
            NewTextChild(placeNode, "y", (int)pl.first.m_y);
            if(pl.second.m_occupied) {
                xmlNodePtr layersNode = newChild(placeNode, "layers");
                xmlNodePtr layerNode = newChild(layersNode, "layer");
                NewTextChild(layerNode, "layer_type", "CHECKIN");
            }
            NewTextChild(placeNode, "elem_type", pl.second.m_elemType);
            NewTextChild(placeNode, "class", pl.second.m_class);
            NewTextChild(placeNode, "xname", pl.second.m_xName);
            NewTextChild(placeNode, "yname", pl.second.m_yName);
        }
    }
}

void Result::toSmpUpdXml(xmlNodePtr node,
                         const Seat& oldSeat,
                         const Seat& newSeat) const
{
    ASSERT(seatmap());
    PlaceMatrix placeMatrix = createPlaceMatrix(seatmap().get());

    xmlNodePtr salonsNode = newChild(node, "update_salons");
    xmlSetProp(salonsNode, "RFISCMode", 0);

    xmlNodePtr seatsNode = newChild(salonsNode, "seats");

    size_t newSeatSalonNum = placeMatrix.findPlaceListNum(newSeat.col(), newSeat.row());
    size_t oldSeatSalonNum = placeMatrix.findPlaceListNum(oldSeat.col(), oldSeat.row());

    xmlNodePtr salonNode = newChild(seatsNode, "salon");
    xmlSetProp(salonNode, "num", oldSeatSalonNum);

    const PlaceMatrix::PlaceList& oldSeatSalon = placeMatrix.placeLists().at(oldSeatSalonNum);
    xmlNodePtr oldPlaceNode = newChild(salonNode, "place");
    boost::optional<PlaceMatrix::Coord2D> oldPlaceCoord = oldSeatSalon.findPlaceCoords(oldSeat.col(), oldSeat.row());
    ASSERT(oldPlaceCoord);
    NewTextChild(oldPlaceNode, "x", (int)oldPlaceCoord->m_x);
    NewTextChild(oldPlaceNode, "y", (int)oldPlaceCoord->m_y);


    if(newSeatSalonNum != oldSeatSalonNum) {
        salonNode = newChild(seatsNode, "salon");
        xmlSetProp(salonNode, "num", newSeatSalonNum);
    }

    const PlaceMatrix::PlaceList& newSeatSalon = placeMatrix.placeLists().at(newSeatSalonNum);
    xmlNodePtr newPlaceNode = newChild(salonNode, "place");
    boost::optional<PlaceMatrix::Coord2D> newPlaceCoord = newSeatSalon.findPlaceCoords(newSeat.col(), newSeat.row());
    ASSERT(newPlaceCoord);
    NewTextChild(newPlaceNode, "x", (int)newPlaceCoord->m_x);
    NewTextChild(newPlaceNode, "y", (int)newPlaceCoord->m_y);
    xmlNodePtr layersNode = newChild(newPlaceNode, "layers");
    xmlNodePtr layerNode = newChild(layersNode, "layer");
    NewTextChild(layerNode, "layer_type", "CHECKIN");
}

}//namespace dcrcka

//---------------------------------------------------------------------------------------

boost::optional<PlaceMatrix::Coord2D> PlaceMatrix::PlaceList::findPlaceCoords(const std::string& xName,
                                                                              const std::string& yName) const
{
    for(std::map<PlaceMatrix::Coord2D, PlaceMatrix::Place>::const_iterator it = m_places.begin();
        it != m_places.end(); ++it)
    {
        if(it->second.m_xName == xName && it->second.m_yName == yName) {
            return it->first;
        }
    }
    return boost::none;
}

size_t PlaceMatrix::findPlaceListNum(const std::string& xName,
                                     const std::string& yName) const
{
    for(std::map<size_t, PlaceList>::const_iterator it = m_matrix.begin();
        it != m_matrix.end(); ++it)
    {
        if(it->second.findPlaceCoords(xName, yName)) {
            return it->first;
        }
    }
    throw EXCEPTIONS::Exception("Bad seat: %s %s", xName.c_str(), yName.c_str());
}

PlaceMatrix::Limits PlaceMatrix::PlaceList::limits() const
{
    return PlaceMatrix::Limits(m_places.begin()->first,
                               m_places.rbegin()->first);
}

//---------------------------------------------------------------------------------------

PlaceMatrix createPlaceMatrix(const SeatmapDetails& seatmap)
{
    PlaceMatrix placeMatrix;
    size_t placeListNum = 0;
    for(const CabinDetails& cbd: seatmap.lCabin()) {
        PlaceMatrix::PlaceList placeList;
        unsigned y = 0;
        for(unsigned row = cbd.rowRange().firstRow(); row <= cbd.rowRange().lastRow(); ++row)  {
            unsigned x = 0;
            bool wasAisle = false;
            for(const SeatColumnDetails& col: cbd.seatColumns()) {
                std::string curXName = col.column(),
                            curYName = boost::lexical_cast<std::string>(row);
                std::string cls = cbd.classDesignator();
                if(!cls.empty()) {
                    cls = Ticketing::SubClass(cls)->baseClass()->code(RUSSIAN);
                }
                PlaceMatrix::Place place(curXName,
                                         curYName,
                                         cls,
                                         "�", // TODO get it
                                         cbd.defaultSeatOccupation() == "O" ? true : false);
                for(const RowDetails& rod: seatmap.lRow()) {
                    if(rod.row() == curYName) {
                        for(const SeatOccupationDetails& seatOccup: rod.lOccupationDetails()) {
                            if(seatOccup.column() == curXName) {
                                if(seatOccup.occupation() == "O") {
                                    place.m_occupied = true;
                                }
                            }
                        }
                    }
                }
                placeList.setPlace(PlaceMatrix::Coord2D(x, y), place);
                ++x;
                if(col.desc1() == "A") {
                    if(!wasAisle) {
                        wasAisle = true;
                        ++x;
                    } else {
                        wasAisle = false;
                    }
                }

            }
            ++y;
        }

        placeMatrix.addPlaceList(placeListNum++, placeList);
    }

    return placeMatrix;
}

//---------------------------------------------------------------------------------------

BaseParams::BaseParams(const OriginatorDetails& origin,
                       const FlightDetails& flight,
                       boost::optional<FlightDetails> flightFromPrevHost,
                       boost::optional<CascadeHostDetails> cascadeDetails)
    : m_origin(origin), m_flight(flight),
      m_flightFromPrevHost(flightFromPrevHost), m_cascadeDetails(cascadeDetails)
{}

const iatci::OriginatorDetails& BaseParams::origin() const
{
    return m_origin;
}

const iatci::FlightDetails& BaseParams::flight() const
{
    return m_flight;
}

const boost::optional<FlightDetails>& BaseParams::flightFromPrevHost() const
{
    return m_flightFromPrevHost;
}

const boost::optional<iatci::CascadeHostDetails>& BaseParams::cascadeDetails() const
{
    return m_cascadeDetails;
}

//---------------------------------------------------------------------------------------

Params::Params(const OriginatorDetails& origin,
               const PaxDetails& pax,
               const FlightDetails& flight,
               boost::optional<FlightDetails> flightFromPrevHost,
               boost::optional<CascadeHostDetails> cascadeDetails,
               boost::optional<ServiceDetails> serviceDetails)
    : BaseParams(origin, flight, flightFromPrevHost, cascadeDetails),
      m_pax(pax), m_service(serviceDetails)
{}

const iatci::PaxDetails& Params::pax() const
{
    return m_pax;
}

const boost::optional<ServiceDetails>& Params::service() const
{
    return m_service;
}

//---------------------------------------------------------------------------------------

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<DocDetails>& doc,
                   const boost::optional<AddressDetails>& address)
    : m_pax(pax),
      m_reserv(reserv),
      m_baggage(baggage),
      m_service(service),
      m_doc(doc),
      m_address(address)
{}

const PaxDetails& PaxGroup::pax() const
{
    return m_pax;
}

const boost::optional<ReservationDetails>& PaxGroup::reserv() const
{
    return m_reserv;
}

const boost::optional<BaggageDetails>& PaxGroup::baggage() const
{
    return m_baggage;
}

const boost::optional<ServiceDetails>& PaxGroup::service() const
{
    return m_service;
}

const boost::optional<DocDetails>& PaxGroup::doc() const
{
    return m_doc;
}

const boost::optional<AddressDetails>& PaxGroup::address() const
{
    return m_address;
}

//---------------------------------------------------------------------------------------

FlightGroup::FlightGroup(const FlightDetails& outboundFlight,
                         const boost::optional<FlightDetails>& inboundFlight)
    : m_outboundFlight(outboundFlight),
      m_inboundFlight(inboundFlight)
{}

const FlightDetails& FlightGroup::outboundFlight() const
{
    return m_outboundFlight;
}

const boost::optional<FlightDetails>& FlightGroup::inboundFlight() const
{
    return m_inboundFlight;
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqcki {

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<SeatDetails>& seat,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<DocDetails>& doc,
                   const boost::optional<AddressDetails>& address)
    : iatci::PaxGroup(pax, reserv, baggage, service, doc, address),
      m_seat(seat)
{}

const boost::optional<SeatDetails>& PaxGroup::seat() const
{
    return m_seat;
}

//---------------------------------------------------------------------------------------

FlightGroup::FlightGroup(const FlightDetails& outboundFlight,
                         const boost::optional<FlightDetails>& inboundFlight,
                         const std::list<dcqcki::PaxGroup>& paxGroups)
    : iatci::FlightGroup(outboundFlight, inboundFlight),
      m_paxGroups(paxGroups)
{}

const std::list<dcqcki::PaxGroup>& FlightGroup::paxGroups() const
{
    return m_paxGroups;
}

}//namespace dcqcki

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqcku {

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<UpdatePaxDetails>& updPax,
                   const boost::optional<UpdateSeatDetails>& updSeat,
                   const boost::optional<UpdateBaggageDetails>& updBaggage,
                   const boost::optional<UpdateServiceDetails>& updService,
                   const boost::optional<UpdateDocDetails>& updDoc)
    : iatci::PaxGroup(pax, reserv, baggage, service, boost::none, boost::none),
      m_updPax(updPax), m_updSeat(updSeat), m_updBaggage(updBaggage),
      m_updService(updService), m_updDoc(updDoc)
{}

const boost::optional<UpdatePaxDetails>& PaxGroup::updPax() const
{
    return m_updPax;
}

const boost::optional<UpdateSeatDetails>& PaxGroup::updSeat() const
{
    return m_updSeat;
}

const boost::optional<UpdateBaggageDetails>& PaxGroup::updBaggage() const
{
    return m_updBaggage;
}

const boost::optional<UpdateServiceDetails>& PaxGroup::updService() const
{
    return m_updService;
}

const boost::optional<UpdateDocDetails>& PaxGroup::updDoc() const
{
    return m_updDoc;
}

//---------------------------------------------------------------------------------------

FlightGroup::FlightGroup(const FlightDetails& outboundFlight,
                         const boost::optional<FlightDetails>& inboundFlight,
                         const std::list<dcqcku::PaxGroup>& paxGroups)
    : iatci::FlightGroup(outboundFlight, inboundFlight),
      m_paxGroups(paxGroups)
{}

const std::list<dcqcku::PaxGroup>& FlightGroup::paxGroups() const
{
    return m_paxGroups;
}

}//namespace dcqcku

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqckx {

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<SeatDetails>& seat,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service)
    : iatci::PaxGroup(pax, reserv, baggage, service, boost::none, boost::none),
      m_seat(seat)
{}

const boost::optional<SeatDetails>& PaxGroup::seat() const
{
    return m_seat;
}

//---------------------------------------------------------------------------------------

FlightGroup::FlightGroup(const FlightDetails& outboundFlight,
                         const boost::optional<FlightDetails>& inboundFlight,
                         const std::list<dcqckx::PaxGroup>& paxGroups)
    : iatci::FlightGroup(outboundFlight, inboundFlight),
      m_paxGroups(paxGroups)
{}

const std::list<dcqckx::PaxGroup>& FlightGroup::paxGroups() const
{
    return m_paxGroups;
}

}//namespace dcqckx

/////////////////////////////////////////////////////////////////////////////////////////

namespace dcqbpr {

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service)
    : iatci::PaxGroup(pax, reserv, baggage, service, boost::none, boost::none)
{}

//---------------------------------------------------------------------------------------

FlightGroup::FlightGroup(const FlightDetails& outboundFlight,
                         const boost::optional<FlightDetails>& inboundFlight,
                         const std::list<dcqbpr::PaxGroup>& paxGroups)
    : iatci::FlightGroup(outboundFlight, inboundFlight),
      m_paxGroups(paxGroups)
{}

const std::list<dcqbpr::PaxGroup>& FlightGroup::paxGroups() const
{
    return m_paxGroups;
}

}//namespace dcqbpr

/////////////////////////////////////////////////////////////////////////////////////////

CkiParams::CkiParams(const OriginatorDetails& org,
                     const boost::optional<CascadeHostDetails>& cascade,
                     const dcqcki::FlightGroup& flg)
    : m_org(org),
      m_cascade(cascade),
      m_fltGroup(flg)
{}

const OriginatorDetails& CkiParams::org() const
{
    return m_org;
}

const FlightDetails& CkiParams::outboundFlight() const
{
    return m_fltGroup.outboundFlight();
}

const boost::optional<FlightDetails>& CkiParams::inboundFlight() const
{
    return m_fltGroup.inboundFlight();
}

const boost::optional<CascadeHostDetails>& CkiParams::cascade() const
{
    return m_cascade;
}

const dcqcki::FlightGroup& CkiParams::fltGroup() const
{
    return m_fltGroup;
}

//---------------------------------------------------------------------------------------

CkiParamsOld::CkiParamsOld(const OriginatorDetails& origin,
                           const PaxDetails& pax,
                           const FlightDetails& flight,
                           boost::optional<FlightDetails> flightFromPrevHost,
                           boost::optional<SeatDetails> seat,
                           boost::optional<BaggageDetails> baggage,
                           boost::optional<ReservationDetails> reserv,
                           boost::optional<CascadeHostDetails> cascadeDetails,
                           boost::optional<ServiceDetails> serviceDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails, serviceDetails),
      m_seat(seat), m_baggage(baggage), m_reserv(reserv)
{}

const boost::optional<iatci::SeatDetails>& CkiParamsOld::seat() const
{
    return m_seat;
}

const boost::optional<iatci::BaggageDetails>& CkiParamsOld::baggage() const
{
    return m_baggage;
}

const boost::optional<ReservationDetails>& CkiParamsOld::reserv() const
{
    return m_reserv;
}

//---------------------------------------------------------------------------------------

CkuParams::CkuParams(const OriginatorDetails& org,
                     const boost::optional<CascadeHostDetails>& cascade,
                     const dcqcku::FlightGroup& flg)
    : m_org(org),
      m_cascade(cascade),
      m_fltGroup(flg)
{}

const OriginatorDetails& CkuParams::org() const
{
    return m_org;
}

const FlightDetails& CkuParams::outboundFlight() const
{
    return m_fltGroup.outboundFlight();
}

const boost::optional<FlightDetails>& CkuParams::inboundFlight() const
{
    return m_fltGroup.inboundFlight();
}

const boost::optional<CascadeHostDetails>& CkuParams::cascade() const
{
    return m_cascade;
}

const dcqcku::FlightGroup& CkuParams::fltGroup() const
{
    return m_fltGroup;
}

//---------------------------------------------------------------------------------------

CkuParamsOld::CkuParamsOld(const OriginatorDetails& origin,
                           const PaxDetails& pax,
                           const FlightDetails& flight,
                           boost::optional<FlightDetails> flightFromPrevHost,
                           boost::optional<UpdatePaxDetails> updPax,
                           boost::optional<UpdateServiceDetails> updService,
                           boost::optional<UpdateSeatDetails> updSeat,
                           boost::optional<UpdateBaggageDetails> updBaggage,
                           boost::optional<UpdateDocDetails> updDoc,
                           boost::optional<CascadeHostDetails> cascadeDetails,
                           boost::optional<ServiceDetails> serviceDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails, serviceDetails),
      m_updPax(updPax), m_updService(updService), m_updSeat(updSeat),
      m_updBaggage(updBaggage), m_updDoc(updDoc)
{
    if(!m_updPax && !m_updService && !m_updSeat && !m_updBaggage && !m_updDoc) {
        LogError(STDLOG) << "CkuParams without update information!";
    }
}

const boost::optional<UpdatePaxDetails>& CkuParamsOld::updPax() const
{
    return m_updPax;
}

const boost::optional<UpdateServiceDetails>& CkuParamsOld::updService() const
{
    return m_updService;
}

const boost::optional<UpdateSeatDetails>& CkuParamsOld::updSeat() const
{
    return m_updSeat;
}

const boost::optional<UpdateBaggageDetails>& CkuParamsOld::updBaggage() const
{
    return m_updBaggage;
}

const boost::optional<UpdateDocDetails>& CkuParamsOld::updDoc() const
{
    return m_updDoc;
}

//---------------------------------------------------------------------------------------

CkxParams::CkxParams(const OriginatorDetails& org,
                     const boost::optional<CascadeHostDetails>& cascade,
                     const dcqckx::FlightGroup& flg)
    : m_org(org),
      m_cascade(cascade),
      m_fltGroup(flg)
{}

const OriginatorDetails& CkxParams::org() const
{
    return m_org;
}

const FlightDetails& CkxParams::outboundFlight() const
{
    return m_fltGroup.outboundFlight();
}

const boost::optional<FlightDetails>& CkxParams::inboundFlight() const
{
    return m_fltGroup.inboundFlight();
}

const boost::optional<CascadeHostDetails>& CkxParams::cascade() const
{
    return m_cascade;
}

const dcqckx::FlightGroup& CkxParams::fltGroup() const
{
    return m_fltGroup;
}

//---------------------------------------------------------------------------------------

CkxParamsOld::CkxParamsOld(const OriginatorDetails& origin,
                           const PaxDetails& pax,
                           const FlightDetails& flight,
                           boost::optional<FlightDetails> flightFromPrevHost,
                           boost::optional<CascadeHostDetails> cascadeDetails,
                           boost::optional<ServiceDetails> serviceDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails, serviceDetails)
{}

//---------------------------------------------------------------------------------------

PlfParams::PlfParams(const OriginatorDetails& org,
                     const SelectPersonalDetails& personal,
                     const FlightDetails& outboundFlight,
                     const boost::optional<FlightDetails>& inboundFlight,
                     const boost::optional<CascadeHostDetails>& cascade)
    : m_org(org), m_personal(personal),
      m_outboundFlight(outboundFlight), m_inboundFlight(inboundFlight),
      m_cascade(cascade)
{}

const OriginatorDetails& PlfParams::org() const
{
    return m_org;
}

const FlightDetails& PlfParams::outboundFlight() const
{
    return m_outboundFlight;
}

const boost::optional<FlightDetails>& PlfParams::inboundFlight() const
{
    return m_inboundFlight;
}

const boost::optional<CascadeHostDetails>& PlfParams::cascade() const
{
    return m_cascade;
}

const SelectPersonalDetails& PlfParams::personal() const
{
    return m_personal;
}

//---------------------------------------------------------------------------------------

SmfParams::SmfParams(const OriginatorDetails& org,
                     const boost::optional<SeatRequestDetails>& seatReq,
                     const FlightDetails& outboundFlight,
                     const boost::optional<FlightDetails>& inboundFlight,
                     const boost::optional<CascadeHostDetails>& cascade)
    : m_org(org), m_seatReq(seatReq),
      m_outboundFlight(outboundFlight), m_inboundFlight(inboundFlight),
      m_cascade(cascade)
{}

const OriginatorDetails& SmfParams::org() const
{
    return m_org;
}

const FlightDetails& SmfParams::outboundFlight() const
{
    return m_outboundFlight;
}

const boost::optional<FlightDetails>& SmfParams::inboundFlight() const
{
    return m_inboundFlight;
}

const boost::optional<CascadeHostDetails>& SmfParams::cascade() const
{
    return m_cascade;
}

const boost::optional<SeatRequestDetails>& SmfParams::seatRequest() const
{
    return m_seatReq;
}

//---------------------------------------------------------------------------------------

BprParams::BprParams(const OriginatorDetails& org,
                     const boost::optional<CascadeHostDetails>& cascade,
                     const dcqbpr::FlightGroup& flg)
    : m_org(org),
      m_cascade(cascade),
      m_fltGroup(flg)
{}

const OriginatorDetails& BprParams::org() const
{
    return m_org;
}

const FlightDetails& BprParams::outboundFlight() const
{
    return m_fltGroup.outboundFlight();
}

const boost::optional<FlightDetails>& BprParams::inboundFlight() const
{
    return m_fltGroup.inboundFlight();
}

const boost::optional<CascadeHostDetails>& BprParams::cascade() const
{
    return m_cascade;
}

const dcqbpr::FlightGroup& BprParams::fltGroup() const
{
    return m_fltGroup;
}

//---------------------------------------------------------------------------------------

BprParamsOld::BprParamsOld(const OriginatorDetails& origin,
                           const PaxDetails& pax,
                           const FlightDetails& flight,
                           boost::optional<FlightDetails> flightFromPrevHost,
                           boost::optional<CascadeHostDetails> cascadeDetails)
    : CkiParamsOld(origin, pax, flight, flightFromPrevHost, boost::none,
                   boost::none, boost::none, cascadeDetails)
{}

}//namespace iatci
