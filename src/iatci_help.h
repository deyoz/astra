#pragma once

#include "iatci_types.h"
#include "astra_api.h"
#include "xml_unit.h"
#include "tlg/edi_elements.h"

#include <libtlg/tlgnum.h>
#include <edilib/EdiSessionId_t.h>


namespace iatci {

std::string fullFlightString(const FlightDetails& flight, bool edi = true);
std::string flightString(const FlightDetails& flight);
std::string airlineAccode(const std::string& airline);
std::string airportCode(const std::string& airport);
std::string airportCityCode(const std::string& airport);
std::string airportCityName(const std::string& airport);
std::string depDateTimeString(const FlightDetails& flight);
std::string depTimeString(const FlightDetails& flight);
std::string fullAirportString(const std::string& airport);
std::string cityCode(const std::string& city);
std::string cityName(const std::string& city);
std::string paxTypeString(const PaxDetails& pax);
std::string paxSexString(const PaxDetails& pax);

//---------------------------------------------------------------------------------------

XMLDoc createXmlDoc(const std::string& xml);

//---------------------------------------------------------------------------------------

class IatciXmlDb
{
public:
    static const size_t PageSize;
    static void add(int grpId, const std::string& xmlText);
    static void del(int grpId);
    static void upd(int grpId, const std::string& xmlText);
    static std::string load(int grpId);

private:
    static void saveXml(int grpId, const std::string& xmlText);
    static void delXml(int grpId);
};

//---------------------------------------------------------------------------------------

iatci::PaxDetails              makePax(const edifact::PpdElem& ppd);
iatci::PaxDetails              makeInfant(const edifact::PpdElem& ppd);
iatci::ReservationDetails      makeReserv(const edifact::PrdElem& prd);
iatci::SeatDetails             makeSeat(const edifact::PsdElem& psd);
iatci::FlightSeatDetails       makeSeat(const edifact::PfdElem& pfd);
iatci::SelectPersonalDetails   makePersonal(const edifact::SpdElem& spd);
iatci::BaggageDetails          makeBaggage(const edifact::PbdElem& pbd);
iatci::ServiceDetails          makeService(const edifact::PsiElem& psi);
iatci::DocDetails              makeDoc(const edifact::PapElem& pap);
iatci::OriginatorDetails       makeOrg(const edifact::LorElem& lor);
iatci::CascadeHostDetails      makeCascade(const edifact::ChdElem& chd);
iatci::FlightDetails           makeOutboundFlight(const edifact::FdqElem& fdq);
boost::optional<FlightDetails> makeInboundFlight(const edifact::FdqElem& fdq);
iatci::SeatRequestDetails      makeSeatReq(const edifact::SrpElem& srp);
iatci::ErrorDetails            makeError(const edifact::ErdElem& erd);
iatci::WarningDetails          makeWarning(const edifact::WadElem& wad);
iatci::EquipmentDetails        makeEquipment(const edifact::EqdElem& eqd);

iatci::FlightDetails           makeFlight(const edifact::FdrElem& fdr,
                                          const boost::optional<edifact::FsdElem>& fsd);

boost::optional<FlightSeatDetails> makeInfantSeat(const edifact::PfdElem& pfd);

iatci::UpdatePaxDetails        makeUpdPax(const edifact::UpdElem& upd);
iatci::UpdateSeatDetails       makeUpdSeat(const edifact::UsdElem& usd);
iatci::UpdateBaggageDetails    makeUpdBaggage(const edifact::UbdElem& ubd);
iatci::UpdateServiceDetails    makeUpdService(const edifact::UsiElem& usi);
iatci::UpdateDocDetails        makeUpdDoc(const edifact::UapElem& uap);

//---------------------------------------------------------------------------------------

iatci::PaxDetails                          makeQryPax(const astra_api::astra_entities::PaxInfo& pax,
                                                      const boost::optional<astra_api::astra_entities::PaxInfo>& infant = boost::none);
iatci::PaxDetails                          makeRespPax(const astra_api::astra_entities::PaxInfo& pax,
                                                       const boost::optional<astra_api::astra_entities::PaxInfo>& infant = boost::none);
iatci::FlightDetails                       makeFlight(const astra_api::astra_entities::SegmentInfo& seg);
iatci::OriginatorDetails                   makeOrg(const astra_api::astra_entities::SegmentInfo& seg);
boost::optional<iatci::FlightSeatDetails>  makeFlightSeat(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::ReservationDetails> makeReserv(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::SeatDetails>        makeSeat(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::ServiceDetails>     makeService(const astra_api::astra_entities::PaxInfo& pax,
                                                       const boost::optional<astra_api::astra_entities::PaxInfo>& infant = boost::none);
boost::optional<iatci::BaggageDetails>     makeBaggage(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::BaggageDetails>     makeBaggage(const astra_api::astra_entities::PaxInfo& pax,
                                                       const std::list<astra_api::astra_entities::BagPool>& bags,
                                                       const std::list<astra_api::astra_entities::BagPool>& handBags);
boost::optional<iatci::DocDetails>         makeDoc(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::AddressDetails>     makeAddress(const astra_api::astra_entities::PaxInfo& pax);
boost::optional<iatci::CascadeHostDetails> makeCascade();

iatci::UpdatePaxDetails makeUpdPax(const astra_api::astra_entities::PaxInfo& newPax,
                                   iatci::UpdateDetails::UpdateActionCode_e act);
iatci::UpdateServiceDetails::UpdSsrInfo makeUpdSsr(const astra_api::astra_entities::Remark& rem,
                                                   iatci::UpdateDetails::UpdateActionCode_e act);
iatci::UpdateDocDetails makeUpdDoc(const astra_api::astra_entities::DocInfo& doc,
                                   iatci::UpdateDetails::UpdateActionCode_e act);
iatci::UpdateBaggageDetails makeUpdBaggage(const astra_api::astra_entities::BagPool& bagPool,
                                           const astra_api::astra_entities::BagPool& handBagPool);

//---------------------------------------------------------------------------------------

iatci::FlightDetails makeFlight(const astra_api::xml_entities::XmlSegment& seg);

//---------------------------------------------------------------------------------------

void saveDeferredCkiData(tlgnum_t msgId, const std::list<dcrcka::Result>& lRes);
std::list<dcrcka::Result> loadDeferredCkiData(tlgnum_t msgId);

void saveCkiData(edilib::EdiSessionId_t sessId, const std::list<dcrcka::Result>& lRes);
std::list<dcrcka::Result> loadCkiData(edilib::EdiSessionId_t sessId);

//---------------------------------------------------------------------------------------

iatci::PaxDetails::PaxType_e astra2iatci(ASTRA::TPerson personType);
ASTRA::TPerson iatci2astra(iatci::PaxDetails::PaxType_e paxType);

//---------------------------------------------------------------------------------------

std::string normSeatNum(const std::string& seatNum);
std::string normSeatLetter(const std::string& seatLetter);

//---------------------------------------------------------------------------------------

std::string denormSeatNum(const std::string& seatNum);

//---------------------------------------------------------------------------------------

class IatciViewXmlParams
{
public:
    IatciViewXmlParams(const std::list<Ticketing::TicketNum_t>& tickNumOrder);

    const std::list<Ticketing::TicketNum_t>& tickNumOrder() const;

private:
    std::list<Ticketing::TicketNum_t> m_tnOrder;
};

//---------------------------------------------------------------------------------------

void iatci2xml(xmlNodePtr node, const dcrcka::Result& res,
               const IatciViewXmlParams& viewParams);
void iatci2xml(xmlNodePtr node, const std::list<dcrcka::Result>& lRes,
               const IatciViewXmlParams& viewParams);
void iatci2xmlSmp(xmlNodePtr node, const dcrcka::Result& res);
void iatci2xmlSmpUpd(xmlNodePtr node, const dcrcka::Result& res,
                     const Seat& oldSeat, const Seat& newSeat);

}//namespace iatci
