#include "iatci_types.h"
#include "iatci_help.h"
#include "xml_unit.h"

#include <serverlib/exception.h>
#include <serverlib/xml_tools.h>
#include <serverlib/xmllibcpp.h>

#include <ostream>
#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

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

//-----------------------------------------------------------------------------

PaxDetails::DocInfo::DocInfo(const std::string& docType,
                             const std::string& issueCountry,
                             const std::string& no,
                             const std::string& surname,
                             const std::string& name,
                             const std::string& gender,
                             const std::string& nationality,
                             const boost::gregorian::date& birthDate,
                             const boost::gregorian::date& expiryDate)
    : m_docType(docType), m_issueCountry(issueCountry),
      m_no(no), m_surname(surname), m_name(name),
      m_gender(gender), m_nationality(nationality),
      m_birthDate(birthDate), m_expiryDate(expiryDate)
{
}

const std::string& PaxDetails::DocInfo::docType() const
{
    return m_docType;
}

const std::string& PaxDetails::DocInfo::issueCountry() const
{
    return m_issueCountry;
}

const std::string& PaxDetails::DocInfo::no() const
{
    return m_no;
}

const std::string& PaxDetails::DocInfo::surname() const
{
    return m_surname;
}

const std::string& PaxDetails::DocInfo::name() const
{
    return m_name;
}

const std::string& PaxDetails::DocInfo::gender() const
{
    return m_gender;
}

const std::string& PaxDetails::DocInfo::nationality() const
{
    return m_nationality;
}

const boost::gregorian::date& PaxDetails::DocInfo::birthDate() const
{
    return m_birthDate;
}

const boost::gregorian::date& PaxDetails::DocInfo::expiryDate() const
{
    return m_expiryDate;
}

//

PaxDetails::PaxDetails(const std::string& surname,
                       const std::string& name,
                       PaxType_e type,
                       const boost::optional<DocInfo>& doc,
                       const std::string& qryRef,
                       const std::string& respRef)
    : m_surname(surname),
      m_name(name),
      m_type(type),
      m_doc(doc),
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

const boost::optional<PaxDetails::DocInfo>& PaxDetails::doc() const
{
    return m_doc;
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
                                     SmokeIndicator_e smokeInd)
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
    : PaxDetails(surname, name, Adult, boost::none, qryRef, respRef),
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
{}

//-----------------------------------------------------------------------------

ServiceDetails::SsrInfo::SsrInfo(const std::string& ssrCode, const std::string& ssrText,
                                 bool isInftTicket, const std::string& freeText,
                                 const std::string& airline, unsigned quantity)
    : m_ssrCode(ssrCode), m_ssrText(ssrText),
      m_isInfantTicket(isInftTicket), m_freeText(freeText),
      m_airline(airline), m_quantity(quantity)
{}

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

TicketCpn_t ServiceDetails::SsrInfo::toTicketCpn() const
{
    ASSERT(m_ssrCode == "TKNE");
    ASSERT(m_ssrText.length() == 14); // ticknum(13)+cpnnum(1)
    return TicketCpn_t(m_ssrText.substr(0, 13),
                       boost::lexical_cast<unsigned>(m_ssrText.substr(13, 1)));
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

void ServiceDetails::addSsr(const std::string& ssrCode, const std::string& ssrText)
{
    m_lSsr.push_back(ServiceDetails::SsrInfo(ssrCode, ssrText));
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

boost::optional<TicketCpn_t> ServiceDetails::findTicketCpn() const
{
    BOOST_FOREACH(const ServiceDetails::SsrInfo& ssr, lSsr()) {
        if(ssr.ssrCode() == "TKNE") {
            return ssr.toTicketCpn();
        }
    }

    return boost::none;
}

//-----------------------------------------------------------------------------

SeatRequestDetails::SeatRequestDetails(const std::string& cabinClass,
                                       SmokeIndicator_e smokeInd)
    : SeatDetails(smokeInd),
      m_cabinClass(cabinClass)
{}

const std::string& SeatRequestDetails::cabinClass() const
{
    return m_cabinClass;
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

SeatColumnDetails::SeatColumnDetails(const std::string& column, const std::string& desc1,
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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

RowDetails::RowDetails(const std::string& row,
                       const std::list<SeatOccupationDetails>& lOccupationDetails,
                       const std::string& characteristic)
    : m_row(row), m_lOccupationDetails(lOccupationDetails),
      m_characteristic(characteristic)
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

//-----------------------------------------------------------------------------

EquipmentDetails::EquipmentDetails(const std::string& equipment)
    : m_equipment(equipment)
{}

const std::string& EquipmentDetails::equipment() const
{
    return m_equipment;
}

//-----------------------------------------------------------------------------

SeatmapDetails::SeatmapDetails(const std::list<CabinDetails>& lCabinDetails,
                               const std::list<RowDetails>& lRowDetails,
                               boost::optional<SeatRequestDetails> seatRequestDetails)
    : m_lCabinDetails(lCabinDetails), m_lRowDetails(lRowDetails),
      m_seatRequestDetails(seatRequestDetails)
{}

const boost::optional<SeatRequestDetails>& SeatmapDetails::seatRequestDetails() const
{
    return m_seatRequestDetails;
}

const std::list<CabinDetails>& SeatmapDetails::lCabinDetails() const
{
    return m_lCabinDetails;
}

const std::list<RowDetails>& SeatmapDetails::lRowDetails() const
{
    return m_lRowDetails;
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

Result::Result(Action_e action,
               Status_e status,
               boost::optional<FlightDetails> flight,
               boost::optional<PaxDetails> pax,
               boost::optional<FlightSeatDetails> seat,
               boost::optional<SeatmapDetails> seatmap,
               boost::optional<CascadeHostDetails> cascadeDetails,
               boost::optional<ErrorDetails> errorDetails,
               boost::optional<WarningDetails> warningDetails,
               boost::optional<EquipmentDetails> equipmentDetails,
               boost::optional<ServiceDetails> serviceDetails)
    : m_action(action),
      m_status(status),
      m_flight(flight),
      m_pax(pax),
      m_seat(seat),
      m_seatmap(seatmap),
      m_cascadeDetails(cascadeDetails),
      m_errorDetails(errorDetails),
      m_warningDetails(warningDetails),
      m_equipmentDetails(equipmentDetails),
      m_serviceDetails(serviceDetails)
{}

Result Result::makeResult(Action_e action,
                          Status_e status,
                          const FlightDetails& flight,
                          boost::optional<PaxDetails> pax,
                          boost::optional<FlightSeatDetails> seat,
                          boost::optional<SeatmapDetails> seatmap,
                          boost::optional<CascadeHostDetails> cascadeDetails,
                          boost::optional<ErrorDetails> errorDetails,
                          boost::optional<WarningDetails> warningDetails,
                          boost::optional<EquipmentDetails> equipmentDetails,
                          boost::optional<ServiceDetails> serviceDetails)
{
    return Result(action,
                  status,
                  flight,
                  pax,
                  seat,
                  seatmap,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makeCheckinResult(Status_e status,
                                 const FlightDetails& flight,
                                 const PaxDetails& pax,
                                 boost::optional<FlightSeatDetails> seat,
                                 boost::optional<CascadeHostDetails> cascadeDetails,
                                 boost::optional<ErrorDetails> errorDetails,
                                 boost::optional<WarningDetails> warningDetails,
                                 boost::optional<EquipmentDetails> equipmentDetails,
                                 boost::optional<ServiceDetails> serviceDetails)
{
    return Result(Checkin,
                  status,
                  flight,
                  pax,
                  seat,
                  boost::none,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makeUpdateResult(Status_e status,
                                const FlightDetails& flight,
                                const PaxDetails& pax,
                                boost::optional<FlightSeatDetails> seat,
                                boost::optional<CascadeHostDetails> cascadeDetails,
                                boost::optional<ErrorDetails> errorDetails,
                                boost::optional<WarningDetails> warningDetails,
                                boost::optional<EquipmentDetails> equipmentDetails,
                                boost::optional<ServiceDetails> serviceDetails)
{
    return Result(Update,
                  status,
                  flight,
                  pax,
                  seat,
                  boost::none,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makeCancelResult(Status_e status,
                                const FlightDetails& flight,
                                boost::optional<PaxDetails> pax,
                                boost::optional<FlightSeatDetails> seat,
                                boost::optional<CascadeHostDetails> cascadeDetails,
                                boost::optional<ErrorDetails> errorDetails,
                                boost::optional<WarningDetails> warningDetails,
                                boost::optional<EquipmentDetails> equipmentDetails,
                                boost::optional<ServiceDetails> serviceDetails)
{
    return Result(Cancel,
                  status,
                  flight,
                  pax,
                  seat,
                  boost::none,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makeReprintResult(Status_e status,
                                 const FlightDetails& flight,
                                 const PaxDetails& pax,
                                 boost::optional<FlightSeatDetails> seat,
                                 boost::optional<CascadeHostDetails> cascadeDetails,
                                 boost::optional<ErrorDetails> errorDetails,
                                 boost::optional<WarningDetails> warningDetails,
                                 boost::optional<EquipmentDetails> equipmentDetails,
                                 boost::optional<ServiceDetails> serviceDetails)
{
    return Result(Reprint,
                  status,
                  flight,
                  pax,
                  seat,
                  boost::none,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makePasslistResult(Status_e status,
                                  const FlightDetails& flight,
                                  const PaxDetails& pax,
                                  boost::optional<FlightSeatDetails> seat,
                                  boost::optional<CascadeHostDetails> cascadeDetails,
                                  boost::optional<ErrorDetails> errorDetails,
                                  boost::optional<WarningDetails> warningDetails,
                                  boost::optional<EquipmentDetails> equipmentDetails,
                                  boost::optional<ServiceDetails> serviceDetails)
{
    return Result(Passlist,
                  status,
                  flight,
                  pax,
                  seat,
                  boost::none,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  serviceDetails);
}

Result Result::makeSeatmapResult(Status_e status,
                                 const FlightDetails& flight,
                                 const SeatmapDetails& seatmap,
                                 boost::optional<CascadeHostDetails> cascadeDetails,
                                 boost::optional<ErrorDetails> errorDetails,
                                 boost::optional<WarningDetails> warningDetails,
                                 boost::optional<EquipmentDetails> equipmentDetails)
{
    return Result(Seatmap,
                  status,
                  flight,
                  boost::none,
                  boost::none,
                  seatmap,
                  cascadeDetails,
                  errorDetails,
                  warningDetails,
                  equipmentDetails,
                  boost::none);
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
                  boost::none,
                  errorDetails,
                  boost::none,
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
    ASSERT(m_flight);
    return m_flight.get();
}

const boost::optional<iatci::PaxDetails>& Result::pax() const
{
    return m_pax;
}

const boost::optional<FlightSeatDetails>& Result::seat() const
{
    return m_seat;
}

const boost::optional<SeatmapDetails>& Result::seatmap() const
{
    return m_seatmap;
}

const boost::optional<CascadeHostDetails>& Result::cascadeDetails() const
{
    return m_cascadeDetails;
}

const boost::optional<ErrorDetails>& Result::errorDetails() const
{
    return m_errorDetails;
}

const boost::optional<WarningDetails>& Result::warningDetails() const
{
    return m_warningDetails;
}

const boost::optional<EquipmentDetails>& Result::equipmentDetails() const
{
    return m_equipmentDetails;
}

const boost::optional<ServiceDetails>& Result::serviceDetails() const
{
    return m_serviceDetails;
}

Result::Action_e Result::strToAction(const std::string& a)
{
    if(a == "I")      return Checkin;
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
    if(s == "O")      return Ok;
    else if(s == "P") return OkWithNoData;
    else if(s == "F") return Failed;
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
    case Ok:           return "O";
    case OkWithNoData: return "P";
    case Failed:       return "X";
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
    NewTextChild(segNode, "class", "ù"); // TODO
    NewTextChild(segNode, "status", "K"); // TODO
    NewTextChild(segNode, "bag_refuse", ""); // TODO
    NewTextChild(segNode, "piece_concept", 0); // TODO
    NewTextChild(segNode, "tid", 0); // TODO
    NewTextChild(segNode, "city_arv", cityCode(flight().arrPort()));
    //xmlNodePtr markFltNode = newChild(segNode, "mark_flight"); // TODO

    if(pax())
    {
        xmlNodePtr paxesNode = newChild(segNode, "passengers");
        xmlNodePtr paxNode = newChild(paxesNode, "pax");
        NewTextChild(paxNode, "pax_id", -1);
        NewTextChild(paxNode, "surname", pax()->surname());
        NewTextChild(paxNode, "name", pax()->name());
        NewTextChild(paxNode, "pers_type", paxTypeString(pax().get())); // TODO
        NewTextChild(paxNode, "seat_no", seat() ? seat()->seat() : "");
        NewTextChild(paxNode, "seat_type", ""); // TODO
        NewTextChild(paxNode, "seats", 1); // TODO
        NewTextChild(paxNode, "refuse", ""); // TODO
        NewTextChild(paxNode, "reg_no", "1"); // TODO
        NewTextChild(paxNode, "subclass", "ù"); // TODO
        NewTextChild(paxNode, "bag_pool_num", ""); // TODO
        NewTextChild(paxNode, "tid", 0); // TODO
        boost::optional<TicketCpn_t> tickCpn;
        if(serviceDetails()) {
            tickCpn = serviceDetails()->findTicketCpn();
        }
        if(tickCpn) {
            NewTextChild(paxNode, "ticket_no", tickCpn->tickNum());
            NewTextChild(paxNode, "coupon_no", (const int)tickCpn->couponNum());
        }
        else {
            NewTextChild(paxNode, "ticket_no");
            NewTextChild(paxNode, "coupon_no");
        }

        NewTextChild(paxNode, "ticket_rem", ""); // TODO
        NewTextChild(paxNode, "ticket_confirm", "1"); // TODO

        xmlNodePtr docNode = newChild(paxNode, "document");
        if(pax()->doc())
        {
            NewTextChild(docNode, "type", pax()->doc()->docType());
            NewTextChild(docNode, "issue_country", pax()->doc()->issueCountry());
            NewTextChild(docNode, "no", pax()->doc()->no());
            NewTextChild(docNode, "nationality", pax()->doc()->nationality());
            NewTextChild(docNode, "birth_date", "10.03.1986 00:00:00");
            NewTextChild(docNode, "gender", pax()->doc()->gender());
            NewTextChild(docNode, "surname", pax()->doc()->surname());
            NewTextChild(docNode, "first_name", pax()->doc()->name());
            NewTextChild(docNode, "expiry_date", "31.12.2049 00:00:00");
        }
        else
        {
            NewTextChild(docNode, "type", "");
            NewTextChild(docNode, "issue_country", "");
            NewTextChild(docNode, "no", "");
            NewTextChild(docNode, "nationality", "");
            NewTextChild(docNode, "birth_date", "");
            NewTextChild(docNode, "gender", "");
            NewTextChild(docNode, "surname", "");
            NewTextChild(docNode, "first_name", "");
            NewTextChild(docNode, "expiry_date", "");
        }

        NewTextChild(paxNode, "pr_norec", 0); // TODO
        NewTextChild(paxNode, "pr_bp_print", 0); // TODO
        xmlNodePtr paxRemsNode = newChild(paxNode, "rems"); // TODO
    }

    NewTextChild(segNode, "paid_bag_emd", ""); // TODO
    xmlNodePtr tripCountersNode = newChild(segNode, "tripcounters"); // TODO
    NewTextChild(segNode, "load_residue", ""); // TODO
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

CkiParams::CkiParams(const OriginatorDetails& origin,
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

const boost::optional<iatci::SeatDetails>& CkiParams::seat() const
{
    return m_seat;
}

const boost::optional<iatci::BaggageDetails>& CkiParams::baggage() const
{
    return m_baggage;
}

const boost::optional<ReservationDetails>& CkiParams::reserv() const
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
                     boost::optional<CascadeHostDetails> cascadeDetails,
                     boost::optional<ServiceDetails> serviceDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails, serviceDetails),
      m_updPax(updPax), m_updSeat(updSeat), m_updBaggage(updBaggage)
{
    if(!m_updPax && !m_updSeat && !m_updBaggage) {
        LogError(STDLOG) << "CkuParams without update information!";
    }
}

const boost::optional<UpdatePaxDetails>& CkuParams::updPax() const
{
    return m_updPax;
}

const boost::optional<UpdateSeatDetails>& CkuParams::updSeat() const
{
    return m_updSeat;
}

const boost::optional<UpdateBaggageDetails>& CkuParams::updBaggage() const
{
    return m_updBaggage;
}

//-----------------------------------------------------------------------------

CkxParams::CkxParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<CascadeHostDetails> cascadeDetails,
                     boost::optional<ServiceDetails> serviceDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails, serviceDetails)
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

PlfParams::PlfParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : Params(origin, pax, flight, flightFromPrevHost, cascadeDetails),
      m_paxEx(PaxSeatDetails(pax.surname(), pax.name()))
{}

const PaxSeatDetails& PlfParams::paxEx() const
{
    return m_paxEx;
}

//-----------------------------------------------------------------------------

SmfParams::SmfParams(const OriginatorDetails& origin,
                     const FlightDetails& flight,
                     boost::optional<SeatRequestDetails> seatReqDetails,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<CascadeHostDetails> cascadeDetails)
    : BaseParams(origin, flight, flightFromPrevHost, cascadeDetails),
      m_seatReqDetails(seatReqDetails)
{}

const boost::optional<SeatRequestDetails>& SmfParams::seatRequestDetails() const
{
    return m_seatReqDetails;
}

//-----------------------------------------------------------------------------

BprParams::BprParams(const OriginatorDetails& origin,
                     const PaxDetails& pax,
                     const FlightDetails& flight,
                     boost::optional<FlightDetails> flightFromPrevHost,
                     boost::optional<SeatDetails> seat,
                     boost::optional<BaggageDetails> baggage,
                     boost::optional<ReservationDetails> reserv,
                     boost::optional<CascadeHostDetails> cascadeDetails,
                     boost::optional<ServiceDetails> serviceDetails)
    : CkiParams(origin, pax, flight, flightFromPrevHost, seat,
                baggage, reserv, cascadeDetails, serviceDetails)
{}

}//namespace iatci
