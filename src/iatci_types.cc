#include "iatci_types.h"
#include "iatci_help.h"
#include "basetables.h"
#include "xml_unit.h"
#include "date_time.h"
#include "astra_locale_adv.h"
#include "astra_msg.h"

#include <serverlib/exception.h>
#include <etick/exceptions.h>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace iatci {

using namespace Ticketing;
using namespace Ticketing::TickExceptions;

MagicTab MagicTab::fromNeg(int gt)
{
    ASSERT(gt < 0);
    /*const std::string pts = std::to_string(std::abs(gt));
    std::string p = pts.substr(0, pts.length() - 1);
    std::string t = pts.substr(pts.length() - 1);
    LogTrace(TRACE5) << "gt: " << gt;
    LogTrace(TRACE5) << "p: " << p;
    LogTrace(TRACE5) << "t: " << t;
    return MagicTab(std::atoi(p.c_str()), std::atoi(t.c_str()));*/

    return MagicTab(std::abs(gt), 1); // Hard Code до обновления терминала
}

int MagicTab::toNeg() const
{
    /*std::ostringstream s;
    s << "-";
    s << m_grpId;
    s << m_tabInd;
    LogTrace(TRACE5) << "P: " << m_grpId;
    LogTrace(TRACE5) << "T: " << m_tabInd;
    LogTrace(TRACE5) << "GT: " << std::atoi(s.str().c_str());
    return std::atoi(s.str().c_str());*/

    return -m_grpId;
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
                             const boost::posix_time::time_duration& brdTime,
                             const std::string& gate)
    : m_airline(airl),
      m_flightNum(flNum),
      m_depPort(depPoint),
      m_arrPort(arrPoint),
      m_depDate(depDate),
      m_arrDate(arrDate),
      m_depTime(depTime),
      m_arrTime(arrTime),
      m_boardingTime(brdTime),
      m_gate(gate)
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

const std::string& FlightDetails::gate() const
{
    return m_gate;
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

VisaDetails::VisaDetails(const std::string& visaType,
                         const std::string& issueCountry,
                         const std::string& no,
                         const std::string& placeOfIssue,
                         const boost::gregorian::date& issueDate,
                         const boost::gregorian::date& expiryDate)
    : m_visaType(visaType),
      m_issueCountry(issueCountry),
      m_no(no),
      m_placeOfIssue(placeOfIssue),
      m_issueDate(issueDate),
      m_expiryDate(expiryDate)
{}

const std::string& VisaDetails::visaType() const
{
    return m_visaType;
}

const std::string& VisaDetails::issueCountry() const
{
    return m_issueCountry;
}

const std::string& VisaDetails::no() const
{
    return m_no;
}

const std::string& VisaDetails::placeOfIssue() const
{
    return m_placeOfIssue;
}

const boost::gregorian::date& VisaDetails::issueDate() const
{
    return m_issueDate;
}

const boost::gregorian::date& VisaDetails::expiryDate() const
{
    return m_expiryDate;
}

//---------------------------------------------------------------------------------------

AddressDetails::AddrInfo::AddrInfo(const std::string& type,
                                   const std::string& country,
                                   const std::string& address,
                                   const std::string& city,
                                   const std::string& region,
                                   const std::string& postalCode)
    : m_type(type),
      m_country(country),
      m_address(address),
      m_city(city),
      m_region(region),
      m_postalCode(postalCode)
{}

const std::string& AddressDetails::AddrInfo::type() const
{
    return m_type;
}

const std::string& AddressDetails::AddrInfo::country() const
{
    return m_country;
}

const std::string& AddressDetails::AddrInfo::address() const
{
    return m_address;
}

const std::string& AddressDetails::AddrInfo::city() const
{
    return m_city;
}

const std::string& AddressDetails::AddrInfo::region() const
{
    return m_region;
}

const std::string& AddressDetails::AddrInfo::postalCode() const
{
    return m_postalCode;
}

//

AddressDetails::AddressDetails()
{}

AddressDetails::AddressDetails(const std::list<AddrInfo>& lAddr)
    : m_lAddr(lAddr)
{}

const std::list<AddressDetails::AddrInfo>& AddressDetails::lAddr() const
{
    return m_lAddr;
}

void AddressDetails::addAddr(const AddressDetails::AddrInfo& addr)
{
    m_lAddr.push_back(addr);
}

//---------------------------------------------------------------------------------------

PaxDetails::PaxDetails(const std::string& surname,
                       const std::string& name,
                       PaxType_e type,
                       const std::string& qryRef,
                       const std::string& respRef,
                       WithInftIndicator_e withInftIndic)
    : m_surname(surname),
      m_name(name),
      m_type(type),
      m_qryRef(qryRef),
      m_respRef(respRef),
      m_withInftIndic(withInftIndic)
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
    case Infant: return "IN";
    case Female: return "F";
    case Male:   return "M";
    case Adult:  return "A";
    default:     return "A";
    }
}

PaxDetails::WithInftIndicator_e PaxDetails::withInftIndicator() const
{
    return m_withInftIndic;
}

std::string PaxDetails::withInftIndicatorAsString() const
{
    switch(m_withInftIndic)
    {
    case WithInfant:    return "Y";
    case WithoutInfant: return "N";
    default:            return "N";
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

bool PaxDetails::isInfant() const
{
    return m_type == Infant;
}

bool PaxDetails::withInfant() const
{
    return m_withInftIndic == WithInfant;
}

PaxDetails::PaxType_e PaxDetails::strToType(const std::string& s)
{
    if(s == "C")      return Child;
    else if(s == "IN")return Infant;
    else if(s == "F") return Female;
    else if(s == "M") return Male;
    else if(s == "A") return Adult;
    else {
        LogError(STDLOG) << "Unknown pax type string: " << s;
        return Adult;
    }
}

PaxDetails::WithInftIndicator_e PaxDetails::strToWithInftIndicator(const std::string& s)
{
    if(s == "Y")      return WithInfant;
    else if(s == "N") return WithoutInfant;
    else if(s == "")  return WithoutInfant;
    else {
        LogError(STDLOG) << "Unknown with infant indicator: " << s;
        return WithoutInfant;
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

UpdateAddressDetails::UpdateAddressDetails(UpdateActionCode_e actionCode)
    : UpdateDetails(actionCode)
{}

UpdateAddressDetails::UpdateAddressDetails(UpdateActionCode_e actionCode,
                                           const std::list<AddrInfo> &lAddr)
    : UpdateDetails(actionCode),
      AddressDetails(lAddr)
{}

//---------------------------------------------------------------------------------------

UpdateVisaDetails::UpdateVisaDetails(UpdateActionCode_e actionCode,
                                     const std::string& visaType,
                                     const std::string& issueCountry,
                                     const std::string& no,
                                     const std::string& placeOfIssue,
                                     const boost::gregorian::date& issueDate,
                                     const boost::gregorian::date& expiryDate)
    : UpdateDetails(actionCode),
      VisaDetails(visaType, issueCountry, no,
                  placeOfIssue, issueDate, expiryDate)
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
                                     const std::string& regNo,
                                     SmokeIndicator_e smokeInd)
    : SeatDetails(seat, smokeInd),
      m_cabinClass(cabinClass), m_regNo(regNo)
{}

const std::string& FlightSeatDetails::cabinClass() const
{
    return m_cabinClass;
}

const std::string& FlightSeatDetails::regNo() const
{
    return m_regNo;
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

BaggageDetails::BagTagInfo::BagTagInfo(const std::string& carrierCode,
                                       const std::string& dest,
                                       uint64_t fullTag,
                                       unsigned qtty)
    : m_carrierCode(carrierCode),
      m_dest(dest),
      m_fullTag(fullTag),
      m_qtty(qtty)
{}

unsigned BaggageDetails::BagTagInfo::tagAccode() const
{
    return getTagAccode(m_fullTag);
}

unsigned BaggageDetails::BagTagInfo::tagNum() const
{
    return getTagNum(m_fullTag);
}

bool BaggageDetails::BagTagInfo::consistentWith(const BagTagInfo& bt) const
{
    if(carrierCode() == bt.carrierCode() &&
       dest()        == bt.dest() &&
       tagAccode()   == bt.tagAccode())
    {
       return abs((int)tagNum() - (int)bt.tagNum()) == 1;
    }

    return false;
}

uint64_t BaggageDetails::BagTagInfo::makeFullTag(unsigned tagAccode, unsigned tagNum)
{
    uint64_t fullTag = tagAccode;
    fullTag *= 1000000;
    fullTag += tagNum;
    return fullTag;
}

//---------------------------------------------------------------------------------------

BaggageDetails::BaggageDetails(const boost::optional<BagInfo>& bag,
                               const boost::optional<BagInfo>& handBag,
                               const std::list<BagTagInfo>& bagTags)
    : m_bag(bag), m_handBag(handBag), m_bagTags(bagTags)
{}

std::list<BaggageDetails::BagTagInfo> BaggageDetails::bagTagsReduced() const
{
    if(m_bagTags.size() < 2) return m_bagTags;

    std::list<BaggageDetails::BagTagInfo> res;
    unsigned currRangeQtty = 1;
    uint64_t firstFullTagOfTheRange = m_bagTags.begin()->fullTag();

    for(auto curr = std::next(m_bagTags.begin()); curr != m_bagTags.end(); ++curr)
    {
        auto prev = std::prev(curr);
        bool lastIter = (curr == std::prev(m_bagTags.end()));

        if(!curr->consistentWith(*prev)) {
            res.push_back(BaggageDetails::BagTagInfo(prev->carrierCode(),
                                                     prev->dest(),
                                                     firstFullTagOfTheRange,
                                                     currRangeQtty));
            firstFullTagOfTheRange = curr->fullTag();
            currRangeQtty = 1;
        }
        else {
            currRangeQtty++;
        }

        if(lastIter) {
            res.push_back(BaggageDetails::BagTagInfo(curr->carrierCode(),
                                                     curr->dest(),
                                                     firstFullTagOfTheRange,
                                                     currRangeQtty));
        }
    }

    return res;
}

std::list<BaggageDetails::BagTagInfo> BaggageDetails::bagTagsExpanded() const
{
    std::list<BaggageDetails::BagTagInfo> res;
    for(const auto& bagTag: m_bagTags)
    {
        for(unsigned i = 0; i < bagTag.qtty(); ++i)
        {
            res.push_back(BaggageDetails::BagTagInfo(bagTag.carrierCode(),
                                                     bagTag.dest(),
                                                     bagTag.fullTag() + i,
                                                     1));
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

UpdateBaggageDetails::UpdateBaggageDetails(UpdateActionCode_e actionCode,
                                           const boost::optional<BagInfo>& bag,
                                           const boost::optional<BagInfo>& handBag,
                                           const std::list<BagTagInfo>& bagTags)
    : UpdateDetails(actionCode), BaggageDetails(bag, handBag, bagTags)
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

bool ServiceDetails::SsrInfo::isTkn() const
{
    return m_ssrCode.substr(0, 3) == "TKN";
}

bool ServiceDetails::SsrInfo::isFqt() const
{
    return m_ssrCode.substr(0, 3) == "FQT";
}

bool ServiceDetails::SsrInfo::isTkne() const
{
    return m_ssrCode == "TKNE";
}

boost::optional<Ticketing::TicketCpn_t> ServiceDetails::SsrInfo::toTicketCpn() const
{
    LogTrace(TRACE3) << __FUNCTION__ 
                     << " ssr_code=" << m_ssrCode << "; "
                     << " ssr_text=" << m_ssrText;
           
    if(m_ssrCode == "TKNE" && m_ssrText.length() == 14/*ticknum(13)+cpnnum(1)*/) {
        return Ticketing::TicketCpn_t(m_ssrText.substr(0, 13),
                                      boost::lexical_cast<int>(m_ssrText.substr(13, 1)));
    }
    
    return boost::none;    
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

void ServiceDetails::addSsrFqtv(const std::string& remCode, const std::string& airline,
                                const std::string& fqtNum, unsigned tierLevel)
{
    m_lSsr.push_back(ServiceDetails::SsrInfo(remCode, fqtNum, false, "", airline, tierLevel ));
}

boost::optional<Ticketing::TicketCpn_t> ServiceDetails::findTicketCpn(bool inftTicket) const
{
    for(const auto& ssr: lSsr()) {
        if(ssr.ssrCode() == "TKNE" && ssr.isInfantTicket() == inftTicket) {
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

bool UpdateServiceDetails::containsFqt() const
{
    for(const auto& ssr: m_lUpdSsr) {
        if(ssr.isFqt()) {
            return true;
        }
    }

    return false;
}

bool UpdateServiceDetails::containsNonFqt() const
{
    for(const auto& ssr: m_lUpdSsr) {
        if(!ssr.isFqt()) {
            return true;
        }
    }

    return false;
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
                   const boost::optional<AddressDetails>& address,
                   const boost::optional<VisaDetails>& visa,
                   const boost::optional<PaxDetails>& infant,
                   const boost::optional<DocDetails>& infantDoc,
                   const boost::optional<AddressDetails>& infantAddress,
                   const boost::optional<VisaDetails>& infantVisa,
                   const boost::optional<FlightSeatDetails>& infantSeat)
    : iatci::PaxGroup(pax, reserv, baggage, service, doc, address, visa,
                      infant, infantDoc, infantAddress, infantVisa),
      m_seat(seat), m_infantSeat(infantSeat)
{}

const boost::optional<FlightSeatDetails>& PaxGroup::seat() const
{
    return m_seat;
}

const boost::optional<FlightSeatDetails>& PaxGroup::infantSeat() const
{
    return m_infantSeat;
}

Ticketing::TicketNum_t PaxGroup::tickNum() const
{
    ASSERT(service());
    boost::optional<Ticketing::TicketCpn_t> tickCpn = service()->findTicketCpn(false);
    ASSERT(tickCpn);
    return tickCpn->ticket();
}

boost::optional<Ticketing::TicketNum_t> PaxGroup::tickNumInfant() const
{
    ASSERT(service());
    boost::optional<Ticketing::TicketCpn_t> tickCpn = service()->findTicketCpn(true);
    if(tickCpn) {
        return tickCpn->ticket();
    }
    return boost::none;
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
                                 boost::optional<SeatmapDetails> seatmap,
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

std::list<Ticketing::TicketNum_t> Result::tickNums() const
{
    std::list<Ticketing::TicketNum_t> tnums;
    for(const auto& pxg: paxGroups()) {
        if(pxg.service()) {
            if(pxg.service()->findTicketCpn(false/*inft ticket*/)) {
                tnums.push_back(pxg.tickNum());
            } else {
                LogError(STDLOG) << "IATCI message doesnt't contains ETKT number";
            }
            if(pxg.infant()) {
                if(pxg.service()->findTicketCpn(true/*inft ticket*/)) {
                    boost::optional<Ticketing::TicketNum_t> inftTickNum = pxg.tickNumInfant();
                    ASSERT(inftTickNum);
                    tnums.push_back(inftTickNum.get());
                } else {
                    LogError(STDLOG) << "IATCI message doesn't contains infant's ETKT number";
                }
            }
        } else {
            LogError(STDLOG) << "IATCI message doesn't contains service information with ETKT number(s)";
        }
    }
    return tnums;
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

PaxGroup::PaxGroup(const PaxDetails& pax,
                   const boost::optional<ReservationDetails>& reserv,
                   const boost::optional<BaggageDetails>& baggage,
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<DocDetails>& doc,
                   const boost::optional<AddressDetails>& address,
                   const boost::optional<VisaDetails>& visa,
                   const boost::optional<PaxDetails>& infant,
                   const boost::optional<DocDetails>& infantDoc,
                   const boost::optional<AddressDetails>& infantAddress,
                   const boost::optional<VisaDetails>& infantVisa)
    : m_pax(pax),
      m_reserv(reserv),
      m_baggage(baggage),
      m_service(service),
      m_doc(doc),
      m_address(address),
      m_visa(visa),
      m_infant(infant),
      m_infantDoc(infantDoc),
      m_infantAddress(infantAddress),
      m_infantVisa(infantVisa)
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

const boost::optional<VisaDetails>& PaxGroup::visa() const
{
    return m_visa;
}

const boost::optional<PaxDetails>& PaxGroup::infant() const
{
    return m_infant;
}

const boost::optional<DocDetails>& PaxGroup::infantDoc() const
{
    return m_infantDoc;
}

const boost::optional<AddressDetails>& PaxGroup::infantAddress() const
{
    return m_infantAddress;
}

const boost::optional<VisaDetails>& PaxGroup::infantVisa() const
{
    return m_infantVisa;
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
                   const boost::optional<AddressDetails>& address,
                   const boost::optional<VisaDetails>& visa,
                   const boost::optional<PaxDetails>& infant,
                   const boost::optional<DocDetails>& infantDoc,
                   const boost::optional<AddressDetails>& infantAddress,
                   const boost::optional<VisaDetails>& infantVisa)
    : iatci::PaxGroup(pax, reserv, baggage, service, doc, address, visa,
                      infant, infantDoc, infantAddress, infantVisa),
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
                   const boost::optional<PaxDetails>& infant,
                   const boost::optional<UpdatePaxDetails>& updPax,
                   const boost::optional<UpdateSeatDetails>& updSeat,
                   const boost::optional<UpdateBaggageDetails>& updBaggage,
                   const boost::optional<UpdateServiceDetails>& updService,
                   const boost::optional<UpdateDocDetails>& updDoc,
                   const boost::optional<UpdateAddressDetails>& updAddress,
                   const boost::optional<UpdateVisaDetails>& updVisa,
                   const boost::optional<UpdatePaxDetails>& updInfant,
                   const boost::optional<UpdateDocDetails>& updInfantDoc,
                   const boost::optional<UpdateAddressDetails>& updInfantAddress,
                   const boost::optional<UpdateVisaDetails>& updInfantVisa)
    : iatci::PaxGroup(pax, reserv, baggage, service, boost::none, boost::none, boost::none, infant),
      m_updPax(updPax), m_updSeat(updSeat), m_updBaggage(updBaggage),
      m_updService(updService), m_updDoc(updDoc),
      m_updAddress(updAddress), m_updVisa(updVisa),
      m_updInfant(updInfant), m_updInfantDoc(updInfantDoc),
      m_updInfantAddress(updInfantAddress), m_updInfantVisa(updInfantVisa)
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

const boost::optional<UpdateAddressDetails>& PaxGroup::updAddress() const
{
    return m_updAddress;
}

const boost::optional<UpdateVisaDetails>& PaxGroup::updVisa() const
{
    return m_updVisa;
}

const boost::optional<UpdatePaxDetails>& PaxGroup::updInfant() const
{
    return m_updInfant;
}

const boost::optional<UpdateDocDetails>& PaxGroup::updInfantDoc() const
{
    return m_updInfantDoc;
}

const boost::optional<UpdateAddressDetails>& PaxGroup::updInfantAddress() const
{
    return m_updInfantAddress;
}

const boost::optional<UpdateVisaDetails>& PaxGroup::updInfantVisa() const
{
    return m_updInfantVisa;
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
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<PaxDetails>& infant)
    : iatci::PaxGroup(pax, reserv, baggage, service,
                      boost::none, boost::none, boost::none, infant),
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
                   const boost::optional<ServiceDetails>& service,
                   const boost::optional<PaxDetails>& infant)
    : iatci::PaxGroup(pax, reserv, baggage, service,
                      boost::none, boost::none, boost::none, infant)
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

}//namespace iatci
