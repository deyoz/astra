#include "iatci_help.h"
#include "iatci_serialization.h"
#include "base_tables.h"
#include "astra_elems.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "basetables.h"
#include "convert.h"
#include "tlg/edi_msg.h"

#include <serverlib/dates_io.h>
#include <serverlib/dates_oci.h>
#include <serverlib/cursctl.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/dates_io.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/rip_oci.h>
#include <serverlib/algo.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

using BASIC::date_time::DateTimeToBoost;


std::string fullFlightString(const FlightDetails& flight, bool edi)
{
    std::ostringstream os;
    os << flight.airline()
       << flight.flightNum()
       << "/" << HelpCpp::string_cast(flight.depDate(), "%d.%m")
       << " " << flight.depPort();
    if(edi)
       os << " (EDI)";
    return os.str();
}

std::string flightString(const FlightDetails& flight)
{
    std::ostringstream os;
    os << flight.flightNum();
    return os.str();
}

std::string airlineAccode(const std::string& airline)
{
    const TAirlinesRow& airlRow = dynamic_cast<const TAirlinesRow&>(base_tables.get("airlines").get_row("code/code_lat", airline));
    std::ostringstream os;
    os << airlRow.AsString("aircode");
    return os.str();
}

std::string airportCode(const std::string& airport)
{
    const TAirpsRow& airpsRow = dynamic_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code", airport));
    return ElemIdToCodeNative(etAirp, airpsRow.code);
}

std::string airportCityCode(const std::string& airport)
{
    const TAirpsRow& airpsRow = dynamic_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code", airport));
    return cityCode(airpsRow.city);
}

std::string airportCityName(const std::string& airport)
{
    const TAirpsRow& airpsRow = dynamic_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code", airport));
    return cityName(airpsRow.name);
}

std::string depDateTimeString(const FlightDetails& flight)
{
    std::ostringstream os;
    os << HelpCpp::string_cast(flight.depDate(), "%d.%m.%Y");
    os << " " << depTimeString(flight);

    return os.str();
}

std::string depTimeString(const FlightDetails& flight)
{
    std::ostringstream os;
    if(!flight.depTime().is_not_a_date_time()) {
        os << HelpCpp::string_cast(flight.depTime(), "%H:%M:%S");
    } else {
        os << "00:00:00";
    }

    return os.str();
}

std::string fullAirportString(const std::string& airport)
{
    const TAirpsRow& airpsRow = dynamic_cast<const TAirpsRow&>(base_tables.get("airps").get_row("code", airport));
    std::ostringstream os;
    os << ElemIdToNameLong(etCity, airpsRow.city)
       << " (" << ElemIdToCodeNative(etAirp, airpsRow.code) << ")";
    return os.str();
}

std::string cityString(const std::string& city)
{
    const TCitiesRow& citiesRow = dynamic_cast<const TCitiesRow&>(base_tables.get("cities").get_row("code", city));
    return ElemIdToNameShort(etCity, citiesRow.id);
}

std::string cityCode(const std::string& city)
{
    const TCitiesRow& citiesRow = dynamic_cast<const TCitiesRow&>(base_tables.get("cities").get_row("code", city));
    return ElemIdToCodeNative(etCity, citiesRow.id);
}

std::string cityName(const std::string& city)
{
    const TCitiesRow& citiesRow = dynamic_cast<const TCitiesRow&>(base_tables.get("cities").get_row("code", city));
    return ElemIdToNameShort(etCity, citiesRow.id);
}

static ASTRA::TPerson convertPaxType(const PaxDetails::PaxType_e paxType)
{
    switch(paxType)
    {
    case PaxDetails::Adult:
    case PaxDetails::Male:
    case PaxDetails::Female:
        return ASTRA::adult;
    case PaxDetails::Child:
        return ASTRA::child;
    case PaxDetails::Infant:
        return ASTRA::baby;
    default:
        ;
    }

    return ASTRA::NoPerson;
}

std::string paxTypeString(const PaxDetails& pax)
{
    ASTRA::TPerson persType = convertPaxType(pax.type());
    return EncodePerson(persType);
}

std::string paxSexString(const PaxDetails& pax)
{
    switch(pax.type())
    {
    case PaxDetails::Male:
        return "M";
    case PaxDetails::Female:
        return "F";
    default:
        ;
    }

    return "";
}

static std::string getAstraDocType(const std::string& iatciDocType)
{
    if     (iatciDocType == "PT") return "P";
    else if(iatciDocType == "VI") return "V";
    else if(iatciDocType == "MI") return "M";
    else                          return iatciDocType;
}

static std::string getAstraAddrType(const std::string& iatciAddrType)
{
    if     (iatciAddrType == "700") return "R";
    else if(iatciAddrType == "701") return "B";
    else if(iatciAddrType == "703") return "D";
    else                            return iatciAddrType;
}

static std::string getIatciAddrType(const std::string& astraAddrType)
{
    if     (astraAddrType == "B") return "701";
    else if(astraAddrType == "R") return "700";
    else if(astraAddrType == "D") return "703";
    else                          return "";
}

//---------------------------------------------------------------------------------------

XMLDoc createXmlDoc(const std::string& xml)
{
    XMLDoc doc;
    doc.set(ConvertCodepage(xml, "CP866", "UTF-8"));
    if(doc.docPtr() == NULL) {
        throw EXCEPTIONS::Exception("document %s has wrong XML format", xml.c_str());
    }
    xml_decode_nodelist(doc.docPtr()->children);
    return doc;
}

//---------------------------------------------------------------------------------------

const size_t IatciXmlDb::PageSize = 1000;

void IatciXmlDb::add(int grpId, const std::string& xmlText)
{
    LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;   
    saveXml(grpId, xmlText);
}

void IatciXmlDb::del(int grpId)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    delXml(grpId);
}

void IatciXmlDb::upd(int grpId, const std::string& xmlText)
{
    LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    delXml(grpId);
    saveXml(grpId, xmlText);
}

void IatciXmlDb::saveXml(int grpId, const std::string& xmlText)
{
    LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId
                     << "; xmlText size=" << xmlText.size()
                     << "; pageSize=" << PageSize;
    std::string::const_iterator itb = xmlText.begin(), ite = itb;
    for(size_t pageNo = 1; itb < xmlText.end(); pageNo++)
    {
        ite = itb + PageSize;
        if(ite > xmlText.end()) ite = xmlText.end();
        std::string page(itb, ite);
        LogTrace(TRACE3) << "pageNo=" << pageNo << "; page=" << page;
        itb = ite;

        make_curs(
"insert into GRP_IATCI_XML(GRP_ID, PAGE_NO, XML_TEXT) "
"values (:grp_id, :page_no, :xml_text)")
        .bind(":grp_id", grpId)
        .bind(":page_no", pageNo)
        .bind(":xml_text", page)
        .exec();
    }
}

void IatciXmlDb::delXml(int grpId)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    OciCpp::CursCtl cur = make_curs(
"delete from GRP_IATCI_XML where GRP_ID=:grp_id");
    cur.bind(":grp_id", grpId)
       .exec();
}

std::string IatciXmlDb::load(int grpId)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    std::string res, page;
    OciCpp::CursCtl cur = make_curs(
"select XML_TEXT from GRP_IATCI_XML where GRP_ID=:grp_id "
"order by PAGE_NO");
    cur.bind(":grp_id", grpId)
       .def(page)
       .exec();
    while(!cur.fen()) {
        res += page;
    }

    return res;
}

//---------------------------------------------------------------------------------------

iatci::PaxDetails makePax(const edifact::PpdElem& ppd)
{
    return iatci::PaxDetails(ppd.m_passSurname,
                             ppd.m_passName,
                             iatci::PaxDetails::strToType(ppd.m_passType),
                             ppd.m_passQryRef,
                             ppd.m_passRespRef,
                             iatci::PaxDetails::strToWithInftIndicator(ppd.m_withInftIndicator));
}

iatci::PaxDetails makeInfant(const edifact::PpdElem& ppd)
{
    return iatci::PaxDetails(!ppd.m_inftSurname.empty() ? ppd.m_inftSurname
                                                        : ppd.m_passSurname,
                             ppd.m_inftName,
                             iatci::PaxDetails::Infant,
                             ppd.m_inftQryRef,
                             ppd.m_inftRespRef);
}

iatci::ReservationDetails makeReserv(const edifact::PrdElem& prd)
{
    return iatci::ReservationDetails(prd.m_rbd);
}

iatci::SeatDetails makeSeat(const edifact::PsdElem& psd)
{
    return iatci::SeatDetails(psd.m_seat,
                              iatci::SeatDetails::strToSmokeInd(psd.m_noSmokingInd));
}

iatci::FlightSeatDetails makeSeat(const edifact::PfdElem& pfd)
{
    return iatci::FlightSeatDetails(pfd.m_seat,
                                    pfd.m_cabinClass,
                                    pfd.m_regNo,
                                    iatci::FlightSeatDetails::strToSmokeInd(pfd.m_noSmokingInd));
}

boost::optional<iatci::FlightSeatDetails> makeInfantSeat(const edifact::PfdElem& pfd)
{
    return iatci::FlightSeatDetails("",
                                    pfd.m_cabinClass,
                                    pfd.m_infantRegNo,
                                    iatci::FlightSeatDetails::strToSmokeInd(pfd.m_noSmokingInd));
}

iatci::SelectPersonalDetails makePersonal(const edifact::SpdElem& spd)
{
    return iatci::SelectPersonalDetails(spd.m_passSurname,
                                        spd.m_passName,
                                        spd.m_rbd,
                                        spd.m_passSeat,
                                        spd.m_securityId,
                                        spd.m_recloc,
                                        spd.m_tickNum,
                                        spd.m_passQryRef,
                                        spd.m_passRespRef);
}

iatci::BaggageDetails makeBaggage(const edifact::PbdElem& pbd)
{
    boost::optional<iatci::BaggageDetails::BagInfo> bag, handBag;
    if(pbd.m_bag) {
        bag = iatci::BaggageDetails::BagInfo(pbd.m_bag->m_numOfPieces,
                                             pbd.m_bag->m_weight);
    }
    if(pbd.m_handBag) {
        handBag = iatci::BaggageDetails::BagInfo(pbd.m_handBag->m_numOfPieces,
                                                 pbd.m_handBag->m_weight);
    }
    return iatci::BaggageDetails(bag, handBag);
}

iatci::ServiceDetails makeService(const edifact::PsiElem& psi)
{
    iatci::ServiceDetails service;
    for(const auto& ediSsr: psi.m_lSsr) {
        bool inftTicket = false;
        std::string ssrText = ediSsr.m_ssrText;
        if(ediSsr.m_ssrCode == "TKNE") {
            if(ssrText.length() > 13) {
                if(ssrText.substr(0, 3) == "INF") {
                    ssrText = ssrText.substr(3);
                    inftTicket = true;
                }
            }
        }
        service.addSsr(iatci::ServiceDetails::SsrInfo(ediSsr.m_ssrCode,
                                                      ssrText,
                                                      inftTicket,
                                                      ediSsr.m_freeText,
                                                      ediSsr.m_airline,
                                                      ediSsr.m_numOfPieces));
    }
    return service;
}

iatci::DocDetails makeDoc(const edifact::PapElem& pap)
{
    return iatci::DocDetails(getAstraDocType(pap.m_docQualifier),
                             pap.m_placeOfIssue,
                             pap.m_docNumber,
                             pap.m_surname,
                             pap.m_name,
                             pap.m_otherName,
                             pap.m_gender,
                             pap.m_nationality,
                             pap.m_birthDate,
                             pap.m_expiryDate);
}

iatci::AddressDetails makeAddress(const edifact::AddElem& add)
{
    iatci::AddressDetails addrs;
    for(const auto& ediAddr: add.m_lAddr) {
        addrs.addAddr(iatci::AddressDetails::AddrInfo(getAstraAddrType(ediAddr.m_purposeCode),
                                                      ediAddr.m_country,
                                                      ediAddr.m_address,
                                                      ediAddr.m_city,
                                                      ediAddr.m_region,
                                                      ediAddr.m_postalCode));
    }
    return addrs;
}

iatci::OriginatorDetails makeOrg(const edifact::LorElem& lor)
{
    return iatci::OriginatorDetails(lor.m_airline.empty() ? ""
                                        : BaseTables::Company(lor.m_airline)->rcode(),
                                    lor.m_port.empty() ? ""
                                        : BaseTables::Port(lor.m_port)->rcode());
}

iatci::CascadeHostDetails makeCascade(const edifact::ChdElem& chd)
{
    iatci::CascadeHostDetails cascade =
            iatci::CascadeHostDetails(chd.m_origAirline.empty() ? ""
                                          : BaseTables::Company(chd.m_origAirline)->rcode(),
                                      chd.m_origPoint.empty() ? ""
                                           : BaseTables::Port(chd.m_origPoint)->rcode());
    for(const auto& hostAirline: chd.m_hostAirlines) {
        cascade.addHostAirline(BaseTables::Company(hostAirline)->rcode());
    }

    return cascade;
}

iatci::FlightDetails makeOutboundFlight(const edifact::FdqElem& fdq)
{
    return iatci::FlightDetails(BaseTables::Company(fdq.m_outbAirl)->rcode(),
                                fdq.m_outbFlNum,
                                BaseTables::Port(fdq.m_outbDepPoint)->rcode(),
                                BaseTables::Port(fdq.m_outbArrPoint)->rcode(),
                                fdq.m_outbDepDate,
                                Dates::Date_t(),
                                fdq.m_outbDepTime);
}

boost::optional<FlightDetails> makeInboundFlight(const edifact::FdqElem& fdq)
{
    if(!fdq.m_inbAirl.empty()) {
        return iatci::FlightDetails(BaseTables::Company(fdq.m_inbAirl)->rcode(),
                                    fdq.m_inbFlNum,
                                    fdq.m_inbDepPoint.empty() ? ""
                                        : BaseTables::Port(fdq.m_inbDepPoint)->rcode(),
                                    fdq.m_inbArrPoint.empty() ? ""
                                        : BaseTables::Port(fdq.m_inbArrPoint)->rcode(),
                                    fdq.m_inbDepDate,
                                    fdq.m_inbArrDate,
                                    fdq.m_inbDepTime,
                                    fdq.m_inbArrTime);
    }

    return boost::none;
}

iatci::ErrorDetails makeError(const edifact::ErdElem& erd)
{
    return iatci::ErrorDetails(edifact::getInnerErrByErd(erd.m_messageNumber),
                               erd.m_messageText);
}

iatci::WarningDetails makeWarning(const edifact::WadElem& wad)
{
    return iatci::WarningDetails(edifact::getInnerErrByErd(wad.m_messageNumber),
                                 wad.m_messageText);
}

iatci::EquipmentDetails makeEquipment(const edifact::EqdElem& eqd)
{
    return iatci::EquipmentDetails(eqd.m_equipment);
}

iatci::FlightDetails makeFlight(const edifact::FdrElem& fdr,
                                const boost::optional<edifact::FsdElem>& fsd)
{
    return iatci::FlightDetails(BaseTables::Company(fdr.m_airl)->rcode(),
                                fdr.m_flNum,
                                BaseTables::Port(fdr.m_depPoint)->rcode(),
                                BaseTables::Port(fdr.m_arrPoint)->rcode(),
                                fdr.m_depDate,
                                fdr.m_arrDate,
                                fdr.m_depTime,
                                fdr.m_arrTime,
                                fsd ? fsd->m_boardingTime : Dates::not_a_date_time);
}

iatci::SeatRequestDetails makeSeatReq(const edifact::SrpElem& srp)
{
    return iatci::SeatRequestDetails(srp.m_cabinClass,
                                     iatci::SeatRequestDetails::strToSmokeInd(srp.m_noSmokingInd));
}

iatci::UpdatePaxDetails makeUpdPax(const edifact::UpdElem& upd)
{
    return iatci::UpdatePaxDetails(iatci::UpdateDetails::strToActionCode(upd.m_actionCode),
                                   upd.m_surname,
                                   upd.m_name,
                                   upd.m_passQryRef);
}

iatci::UpdateSeatDetails makeUpdSeat(const edifact::UsdElem& usd)
{
    return iatci::UpdateSeatDetails(iatci::UpdateSeatDetails::strToActionCode(usd.m_actionCode),
                                    usd.m_seat,
                                    iatci::UpdateSeatDetails::strToSmokeInd(usd.m_noSmokingInd));
}

iatci::UpdateBaggageDetails makeUpdBaggage(const edifact::UbdElem& ubd)
{
    boost::optional<iatci::UpdateBaggageDetails::BagInfo> updBag, updHandBag;
    if(ubd.m_bag) {
        updBag = iatci::UpdateBaggageDetails::BagInfo(ubd.m_bag->m_numOfPieces,
                                                      ubd.m_bag->m_weight);
    }
    if(ubd.m_handBag) {
        updHandBag = iatci::UpdateBaggageDetails::BagInfo(ubd.m_handBag->m_numOfPieces,
                                                          ubd.m_handBag->m_weight);
    }
    return iatci::UpdateBaggageDetails(iatci::UpdateDetails::Replace,
                                       updBag, updHandBag);
}

iatci::UpdateServiceDetails makeUpdService(const edifact::UsiElem& usi)
{
    iatci::UpdateServiceDetails updService(iatci::UpdateDetails::Replace/*ignored*/);
    for(auto& ssr: usi.m_lSsr) {
        updService.addSsr(iatci::UpdateServiceDetails::UpdSsrInfo(
                              iatci::UpdateDetails::strToActionCode(ssr.m_actionCode),
                              ssr.m_ssrCode,
                              ssr.m_ssrText,
                              false,
                              ssr.m_freeText,
                              ssr.m_airline,
                              ssr.m_numOfPieces));
    }

    return updService;
}

iatci::UpdateDocDetails makeUpdDoc(const edifact::UapElem& uap)
{
    return iatci::UpdateDocDetails(iatci::UpdateDetails::strToActionCode(uap.m_actionCode),
                                   getAstraDocType(uap.m_docQualifier),
                                   uap.m_placeOfIssue,
                                   uap.m_docNumber,
                                   uap.m_surname,
                                   uap.m_name,
                                   uap.m_otherName,
                                   uap.m_gender,
                                   uap.m_nationality,
                                   uap.m_birthDate,
                                   uap.m_expiryDate);
}

iatci::UpdateAddressDetails makeUpdAddress(const edifact::AddElem& add)
{
    iatci::UpdateAddressDetails updAddress(iatci::UpdateDetails::strToActionCode(add.m_actionCode));
    for(const auto& ediAddr: add.m_lAddr) {
        updAddress.addAddr(iatci::AddressDetails::AddrInfo(getAstraAddrType(ediAddr.m_purposeCode),
                                                           ediAddr.m_country,
                                                           ediAddr.m_address,
                                                           ediAddr.m_city,
                                                           ediAddr.m_region,
                                                           ediAddr.m_postalCode));
    }
    return updAddress;
}

//---------------------------------------------------------------------------------------

iatci::PaxDetails makeQryPax(const astra_api::astra_entities::PaxInfo& pax,
                             const boost::optional<astra_api::astra_entities::PaxInfo>& infant)
{
    return iatci::PaxDetails(pax.m_surname,
                             pax.m_name,
                             astra2iatci(pax.m_persType),
                             "",
                             pax.theirId(),
                             infant ? iatci::PaxDetails::WithInfant
                                    : iatci::PaxDetails::WithoutInfant);
}

iatci::PaxDetails makeRespPax(const astra_api::astra_entities::PaxInfo& pax,
                              const boost::optional<astra_api::astra_entities::PaxInfo>& infant)
{
    return iatci::PaxDetails(pax.m_surname,
                             pax.m_name,
                             astra2iatci(pax.m_persType),
                             "",
                             pax.ourId(),
                             infant ? iatci::PaxDetails::WithInfant
                                    : iatci::PaxDetails::WithoutInfant);
}

iatci::FlightDetails makeFlight(const astra_api::astra_entities::SegmentInfo& seg)
{
    return iatci::FlightDetails(BaseTables::Company(seg.m_markFlight.m_airline)->rcode(),
                                Ticketing::FlightNum_t(seg.m_markFlight.m_flightNum),
                                BaseTables::Port(seg.m_airpDep)->rcode(),
                                BaseTables::Port(seg.m_airpArv)->rcode(),
                                seg.m_markFlight.m_scdDepDate);
}

iatci::OriginatorDetails makeOrg(const astra_api::astra_entities::SegmentInfo& seg)
{
    return iatci::OriginatorDetails(BaseTables::Company(seg.m_markFlight.m_airline)->rcode(),
                                    BaseTables::Port(seg.m_markFlight.m_airpDep)->rcode());
}

boost::optional<iatci::ReservationDetails> makeReserv(const astra_api::astra_entities::PaxInfo& pax)
{
    if(pax.m_subclass) {
        return iatci::ReservationDetails(pax.m_subclass->code(ENGLISH));
    }

    return boost::none;
}

boost::optional<iatci::FlightSeatDetails> makeFlightSeat(const astra_api::astra_entities::PaxInfo& pax)
{
    if(!pax.m_seatNo.empty() || pax.isInfant()) {
        return iatci::FlightSeatDetails(pax.m_seatNo,
                                        pax.m_subclass ? pax.m_subclass->code(ENGLISH) : "",
                                        pax.m_regNo,
                                        iatci::SeatDetails::NonSmoking); // пока курить нельзя!
    }

    return boost::none;
}

boost::optional<iatci::SeatDetails> makeSeat(const astra_api::astra_entities::PaxInfo& pax)
{
    if(!pax.m_seatNo.empty()) {
        return iatci::SeatDetails(pax.m_seatNo);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> makeService(const astra_api::astra_entities::PaxInfo& pax,
                                                   const boost::optional<astra_api::astra_entities::PaxInfo>& infant)
{
    boost::optional<iatci::ServiceDetails> service;

    if(!pax.m_ticketNum.empty()) {
        if(!service) {
            service = iatci::ServiceDetails();
        }

        if(pax.m_ticketRem == "TKNE") {
            service->addSsrTkne(pax.m_ticketNum,
                                pax.m_couponNum,
                                false);
        } else {
            service->addSsr(pax.m_ticketRem,
                            pax.m_ticketNum);
        }

        if(infant) {
            if(infant->m_ticketRem == "TKNE") {
                service->addSsrTkne(infant->m_ticketNum,
                                    infant->m_couponNum,
                                    true);
            }
        }
    }

    if(pax.m_rems) {
        if(!service) {
            service = iatci::ServiceDetails();
        }

        for(const auto& rem: pax.m_rems->m_lRems) {
            if(rem.m_remCode != "TKNE" && rem.m_remCode.substr(0, 3) != "FQT") {
                service->addSsr(rem.m_remCode, rem.m_remText);
            }
        }
    }
    
    if(pax.m_fqtRems) {
        if(!service) {
            service = iatci::ServiceDetails();
        }
        
        for(const auto& rem: pax.m_fqtRems->m_lFqtRems) {
            service->addSsrFqtv(rem.m_remCode, rem.m_airline, rem.m_fqtNo);
        }
    }

    return service;
}

boost::optional<iatci::BaggageDetails> makeBaggage(const astra_api::astra_entities::PaxInfo& pax)
{
    return iatci::BaggageDetails(iatci::BaggageDetails::BagInfo());
}

boost::optional<iatci::BaggageDetails> makeBaggage(const astra_api::astra_entities::PaxInfo& pax,
                                                   const std::list<astra_api::astra_entities::BagPool>& bags,
                                                   const std::list<astra_api::astra_entities::BagPool>& handBags)
{
    if(!pax.bagPoolNum()) {
        return boost::none;
    }

    boost::optional<iatci::BaggageDetails::BagInfo> bag, handBag;
    for(const auto& b: bags) {
        if(b.poolNum() == pax.bagPoolNum()) {
            bag = iatci::BaggageDetails::BagInfo(b.amount(), b.weight());
        }
    }

    for(const auto& hb: handBags) {
        if(hb.poolNum() == pax.bagPoolNum()) {
            handBag = iatci::BaggageDetails::BagInfo(hb.amount(), hb.weight());
        }
    }

    return iatci::BaggageDetails(bag, handBag);
}

boost::optional<iatci::DocDetails> makeDoc(const astra_api::astra_entities::PaxInfo& pax)
{
    if(pax.m_doc) {
        return iatci::DocDetails(pax.m_doc->m_type,
                                 pax.m_doc->m_country,
                                 pax.m_doc->m_num,
                                 pax.m_doc->m_surname,
                                 pax.m_doc->m_name,
                                 pax.m_doc->m_secName,
                                 pax.m_doc->m_gender,
                                 pax.m_doc->m_citizenship,
                                 pax.m_doc->m_birthDate,
                                 pax.m_doc->m_expiryDate);
    }

    return boost::none;
}

boost::optional<iatci::AddressDetails> makeAddress(const astra_api::astra_entities::PaxInfo& pax)
{
    if(pax.m_addrs) {
        boost::optional<iatci::AddressDetails> addrs;
        for(const auto& addr: pax.m_addrs->m_lAddrs) {
            if(!addr.isEmpty()) {
                if(!addrs) {
                    addrs = iatci::AddressDetails();
                }
                addrs->addAddr(iatci::AddressDetails::AddrInfo(getIatciAddrType(addr.m_type),
                                                               addr.m_country,
                                                               addr.m_address,
                                                               addr.m_city,
                                                               addr.m_region,
                                                               addr.m_postalCode));
            }
        }
        return addrs;
    }

    return boost::none;
}

boost::optional<iatci::CascadeHostDetails> makeCascade()
{
    // TODO
    return boost::none;
}

iatci::UpdatePaxDetails makeUpdPax(const astra_api::astra_entities::PaxInfo& newPax,
                                   iatci::UpdateDetails::UpdateActionCode_e act)
{
    return iatci::UpdatePaxDetails(act,
                                   newPax.m_surname,
                                   newPax.m_name);
}

iatci::UpdateServiceDetails::UpdSsrInfo makeUpdSsr(const astra_api::astra_entities::Remark& rem,
                                                   iatci::UpdateDetails::UpdateActionCode_e act)
{
    return iatci::UpdateServiceDetails::UpdSsrInfo(act,
                                                   rem.m_remCode,
                                                   "",
                                                   false,
                                                   rem.m_remText);
}

iatci::UpdateServiceDetails::UpdSsrInfo makeUpdSsrFqt(const astra_api::astra_entities::FqtRemark& rem,
                                                      iatci::UpdateDetails::UpdateActionCode_e act)
{
    return iatci::UpdateServiceDetails::UpdSsrInfo(act,
                                                   rem.m_remCode,
                                                   rem.m_fqtNo,
                                                   false,
                                                   "",
                                                   rem.m_airline);
}

iatci::UpdateDocDetails makeUpdDoc(const astra_api::astra_entities::DocInfo& newDoc,
                                   iatci::UpdateDetails::UpdateActionCode_e act)
{
    return iatci::UpdateDocDetails(act,
                                   newDoc.m_type,
                                   newDoc.m_country,
                                   newDoc.m_num,
                                   newDoc.m_surname,
                                   newDoc.m_name,
                                   newDoc.m_secName,
                                   newDoc.m_gender,
                                   newDoc.m_citizenship,
                                   newDoc.m_birthDate,
                                   newDoc.m_expiryDate);
}

iatci::UpdateBaggageDetails makeUpdBaggage(const astra_api::astra_entities::BagPool& bagPool,
                                           const astra_api::astra_entities::BagPool& handBagPool)
{

    return iatci::UpdateBaggageDetails(iatci::UpdateDetails::Replace,
                                       iatci::UpdateBaggageDetails::BagInfo(bagPool.amount(),
                                                                            bagPool.weight()),
                                       iatci::UpdateBaggageDetails::BagInfo(handBagPool.amount(),
                                                                            handBagPool.weight()));
}

//---------------------------------------------------------------------------------------

iatci::FlightDetails makeFlight(const astra_api::xml_entities::XmlSegment& seg)
{
    std::string airl      = seg.trip_header.airline;
    int         flNum     = seg.trip_header.flt_no;
    TDateTime   scd_local = seg.trip_header.scd_out_local;
    std::string airp_dep  = seg.seg_info.airp_dep;
    std::string airp_arv  = seg.seg_info.airp_arv;

    if(airl.empty()) {
        airl = seg.mark_flight.airline;
    }
    if(flNum == ASTRA::NoExists) {
        flNum = seg.mark_flight.flt_no;
    }
    if(scd_local == ASTRA::NoExists) {
        scd_local = seg.mark_flight.scd;
    }

    boost::gregorian::date scd_dep_date, scd_arr_date;
    if(scd_local != ASTRA::NoExists) {
        scd_dep_date = DateTimeToBoost(scd_local).date();
    }

    return iatci::FlightDetails(BaseTables::Company(airl)->rcode(),
                                Ticketing::FlightNum_t(flNum),
                                BaseTables::Port(airp_dep)->rcode(),
                                BaseTables::Port(airp_arv)->rcode(),
                                scd_dep_date,
                                scd_arr_date,
                                boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                                boost::posix_time::time_duration(boost::posix_time::not_a_date_time));
}

//---------------------------------------------------------------------------------------

boost::optional<iatci::SeatRequestDetails> makeSeatReq(const astra_api::xml_entities::XmlSegment& seg)
{
    ASSERT(!seg.passengers.empty());
    std::string subCls = seg.passengers.front().subclass;
    for(const auto& pax: seg.passengers) {
        if(subCls != pax.subclass) {
            LogTrace(TRACE1) << "Different subclasses in group!";
            return boost::none;
        }
    }
    return iatci::SeatRequestDetails(subCls);
}

//---------------------------------------------------------------------------------------

static const size_t MaxDataPartLen = 4000;
static const size_t MaxDataParts = 5;


static std::string serialize(const std::list<dcrcka::Result>& lRes)
{
    std::ostringstream os;
    {
        boost::archive::text_oarchive oa(os);
        oa << lRes;
    }
    return os.str();
}

static void deserialize(std::list<dcrcka::Result>& lRes, const std::string& data)
{
    std::stringstream is;
    is << data;
    {
        boost::archive::text_iarchive ia(is);
        ia >> lRes;
    }
}

//---------------------------------------------------------------------------------------

static std::vector<std::string> splitBySize(const std::string& in, size_t size)
{
    std::vector<std::string> ret;
    for(size_t i = 0; i < in.length(); i += size) {
        ret.push_back(in.substr(i, size));
    }
    return ret;
}

//---------------------------------------------------------------------------------------

static void normalizeParts(std::vector<std::string>& parts, size_t maxParts)
{
    ASSERT(parts.size() <= maxParts);
    for(size_t i = parts.size(); i < maxParts; ++i) {
        parts.push_back("");
    }
}

//---------------------------------------------------------------------------------------

static std::vector<std::string> slpitDataParts(const std::string& in)
{
    std::vector<std::string> parts = splitBySize(in, MaxDataPartLen);
    normalizeParts(parts, MaxDataParts);
    return parts;
}

//---------------------------------------------------------------------------------------

void saveDeferredCkiData(tlgnum_t msgId, const std::list<dcrcka::Result>& lRes)
{
    std::string serialized = serialize(lRes);

    LogTrace(TRACE3) << __FUNCTION__
                     << " by msgId: " << msgId
                     << " [data size=" << serialized.size() << "]";

    std::vector<std::string> parts = slpitDataParts(serialized);

    OciCpp::CursCtl cur = make_curs(
"insert into DEFERRED_CKI_DATA(MSG_ID, DATA1, DATA2, DATA3, DATA4, DATA5) "
"values (:msg_id, :data1, :data2, :data3, :data4, :data5)");
    cur.bind(":msg_id", msgId.num)
       .bind(":data1", parts[0])
       .bind(":data2", parts[1])
       .bind(":data3", parts[2])
       .bind(":data4", parts[3])
       .bind(":data5", parts[4])
       .exec();
}

std::list<dcrcka::Result> loadDeferredCkiData(tlgnum_t msgId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by msgId: " << msgId;

    char data[MaxDataParts][MaxDataPartLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data1:=NULL;\n"
":data2:=NULL;\n"
":data3:=NULL;\n"
":data4:=NULL;\n"
":data5:=NULL;\n"
"delete from DEFERRED_CKI_DATA where MSG_ID=:msg_id "
"returning DATA1, DATA2, DATA3, DATA4, DATA5 "
"into :data1, :data2, :data3, :data4, :data5; \n"
"end;");
    cur.bind(":msg_id", msgId.num)
       .bindOut(":data1",     data[0])
       .bindOutNull(":data2", data[1], "")
       .bindOutNull(":data3", data[2], "")
       .bindOutNull(":data4", data[3], "")
       .bindOutNull(":data5", data[4], "")
       .exec();

    std::string serialized(std::string(data[0]) + data[1] + data[2] + data[3] + data[5]);
    if(serialized.empty()) {
        tst();
        return std::list<dcrcka::Result>();
    }

    std::list<dcrcka::Result> lRes;
    deserialize(lRes, serialized);
    return lRes;
}

void saveCkiData(edilib::EdiSessionId_t sessId, const std::list<dcrcka::Result>& lRes)
{
    std::string serialized = serialize(lRes);

    LogTrace(TRACE3) << __FUNCTION__
                     << " by sessId: " << sessId
                     << " [data size=" << serialized.size() << "]";

    std::vector<std::string> parts = slpitDataParts(serialized);

    OciCpp::CursCtl cur = make_curs(
"insert into CKI_DATA(EDISESSION_ID, DATA1, DATA2, DATA3, DATA4, DATA5) "
"values (:sessid, :data1, :data2, :data3, :data4, :data5)");
    cur.bind(":sessid", sessId)
       .bind(":data1", parts[0])
       .bind(":data2", parts[1])
       .bind(":data3", parts[2])
       .bind(":data4", parts[3])
       .bind(":data5", parts[4])
       .exec();
}

std::list<dcrcka::Result> loadCkiData(edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by sessId: " << sessId;

    char data[MaxDataParts][MaxDataPartLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data1:=NULL;\n"
":data2:=NULL;\n"
":data3:=NULL;\n"
":data4:=NULL;\n"
":data5:=NULL;\n"
"delete from CKI_DATA where EDISESSION_ID=:sessid "
"returning DATA1, DATA2, DATA3, DATA4, DATA5 "
"into :data1, :data2, :data3, :data4, :data5; \n "
"end;");

    cur.bind(":sessid", sessId)
       .bindOut(":data1",     data[0])
       .bindOutNull(":data2", data[1], "")
       .bindOutNull(":data3", data[2], "")
       .bindOutNull(":data4", data[3], "")
       .bindOutNull(":data5", data[4], "")
       .exec();

    std::string serialized(std::string(data[0]) + data[1] + data[2] + data[3] + data[5]);
    if(serialized.empty()) {
        tst();
        return std::list<dcrcka::Result>();
    }

    std::list<dcrcka::Result> lRes;
    deserialize(lRes, serialized);
    return lRes;
}

//---------------------------------------------------------------------------------------

iatci::PaxDetails::PaxType_e astra2iatci(ASTRA::TPerson personType)
{
    switch(personType)
    {
    case ASTRA::adult:
        return iatci::PaxDetails::Adult;
    case ASTRA::child:
        return iatci::PaxDetails::Child;
    case ASTRA::baby:
        return iatci::PaxDetails::Infant;
    default:
        throw EXCEPTIONS::Exception("Unknown astra person type: %d", personType);
    }
}

ASTRA::TPerson iatci2astra(iatci::PaxDetails::PaxType_e paxType)
{
    switch(paxType)
    {
    case iatci::PaxDetails::Adult:
    case iatci::PaxDetails::Female:
    case iatci::PaxDetails::Male:
        return ASTRA::adult;
    case iatci::PaxDetails::Child:
        return ASTRA::child;
    case iatci::PaxDetails::Infant:
        return ASTRA::baby;
    default:
        throw EXCEPTIONS::Exception("Unknow iatci pax type: %d", paxType);
    }
}

//---------------------------------------------------------------------------------------

std::string normSeatNum(const std::string& seatNum)
{
    if(seatNum.empty()) return seatNum;

    if(!(seatNum.length() > 1 && seatNum.length() < 5)) {
        LogTrace(TRACE1) << "invalid seat num: " << seatNum;
        throw EXCEPTIONS::Exception("invalid seat num: %s", seatNum.c_str());
    }

    const char letter = *(--seatNum.end());
    std::string row(seatNum.begin(), --seatNum.end());

    std::ostringstream norm;
    norm << norm_iata_row(row) << norm_iata_line(std::string(1, letter));
    return norm.str();
}

std::string normSeatLetter(const std::string& seatLetter)
{
    if(seatLetter.length() != 1) {
        LogTrace(TRACE1) << "invalid seat letter: " << seatLetter;
        throw EXCEPTIONS::Exception("invalid seat letter: %s", seatLetter.c_str());
    }

    return norm_iata_line(seatLetter);
}

//---------------------------------------------------------------------------------------

std::string denormSeatNum(const std::string& seatNum)
{
    if(seatNum.empty()) return seatNum;

    if(!(seatNum.length() > 1 && seatNum.length() < 5)) {
        LogTrace(TRACE1) << "invalid seat num: " << seatNum;
        throw EXCEPTIONS::Exception("invalid seat num: %s", seatNum.c_str());
    }

    const char letter = *(--seatNum.end());
    std::string row(seatNum.begin(), --seatNum.end());

    std::ostringstream denorm;
    denorm << denorm_iata_row(row) << denorm_iata_line(std::string(1, letter), true);
    return denorm.str();
}

//---------------------------------------------------------------------------------------

IatciViewXmlParams::IatciViewXmlParams(const std::list<Ticketing::TicketNum_t>& tnOrder)
    : m_tnOrder(tnOrder)
{}

const std::list<Ticketing::TicketNum_t>& IatciViewXmlParams::tickNumOrder() const
{
    return m_tnOrder;
}

//---------------------------------------------------------------------------------------

static xmlNodePtr xmlViewIatciFlight(xmlNodePtr node, const iatci::FlightDetails& flight)
{
    xmlNodePtr segNode = newChild(node, "segment");
    xmlNodePtr tripHeaderNode = newChild(segNode, "tripheader");
    NewTextChild(tripHeaderNode, "flight",  fullFlightString(flight));
    NewTextChild(tripHeaderNode, "airline", flight.airline());
    NewTextChild(tripHeaderNode, "aircode", airlineAccode(flight.airline()));
    NewTextChild(tripHeaderNode, "flt_no",  flightString(flight));
    NewTextChild(tripHeaderNode, "suffix",  "");
    NewTextChild(tripHeaderNode, "airp",    flight.depPort());
    NewTextChild(tripHeaderNode, "scd_out_local", depDateTimeString(flight));
    NewTextChild(tripHeaderNode, "pr_etl_only", "0"); // TODO
    NewTextChild(tripHeaderNode, "pr_etstatus", "0"); // TODO
    NewTextChild(tripHeaderNode, "pr_no_ticket_check", "0"); // TODO)
    NewTextChild(tripHeaderNode, "pr_auto_pt_print", 0); // TODO
    NewTextChild(tripHeaderNode, "pr_auto_pt_print_reseat", 0); // TODO

    xmlNodePtr tripDataNode = newChild(segNode, "tripdata");
    xmlNodePtr airpsNode = newChild(tripDataNode, "airps");
    xmlNodePtr airpNode = newChild(airpsNode, "airp");
    NewTextChild(airpNode, "point_id", -1);
    NewTextChild(airpNode, "airp_code", airportCode(flight.arrPort()));
    NewTextChild(airpNode, "city_code", airportCityCode(flight.arrPort()));
    NewTextChild(airpNode, "target_view", fullAirportString(flight.arrPort()));
    xmlNodePtr checkInfoNode = newChild(airpNode, "check_info");
    xmlNodePtr passNode = newChild(checkInfoNode, "pass"); // TODO
    xmlNodePtr crewNode = newChild(checkInfoNode, "crew"); // TODO
    xmlNodePtr classesNode = newChild(tripDataNode, "classes"); // TODO
    xmlNodePtr gatesNode = newChild(tripDataNode, "gates"); // TODO
    xmlNodePtr hallsNode = newChild(tripDataNode, "halls"); // TODO
    xmlNodePtr markFltsNode = newChild(tripDataNode, "mark_flights"); // TODO

    NewTextChild(segNode, "grp_id", -1);
    NewTextChild(segNode, "point_dep", -1);
    NewTextChild(segNode, "airp_dep", flight.depPort());
    NewTextChild(segNode, "point_arv", -1);
    NewTextChild(segNode, "airp_arv", flight.arrPort());
    NewTextChild(segNode, "class", "Э"); // TODO
    NewTextChild(segNode, "status", "K"); // TODO
    NewTextChild(segNode, "bag_refuse", ""); // TODO
    NewTextChild(segNode, "piece_concept", 0); // TODO
    NewTextChild(segNode, "tid", 0); // TODO
    NewTextChild(segNode, "city_arv", airportCityCode(flight.arrPort()));
    //xmlNodePtr markFltNode = newChild(segNode, "mark_flight"); // TODO
    return segNode;
}

static xmlNodePtr xmlViewIatciPax(xmlNodePtr paxesNode,
                                  const iatci::PaxDetails& pax,
                                  const boost::optional<ReservationDetails>& reserv,
                                  const boost::optional<FlightSeatDetails>& seat,
                                  const boost::optional<ServiceDetails>& service,
                                  const boost::optional<DocDetails>& doc,
                                  int parentPaxId,
                                  int& currPax)
{
    xmlNodePtr paxNode = newChild(paxesNode, "pax");
    NewTextChild(paxNode, "pax_id", --currPax);
    NewTextChild(paxNode, "surname", pax.surname());
    NewTextChild(paxNode, "name", pax.name());
    NewTextChild(paxNode, "pers_type", paxTypeString(pax));
    NewTextChild(paxNode, "seat_no", seat ? seat->seat() : "");
    NewTextChild(paxNode, "seat_type", "");
    NewTextChild(paxNode, "seats", pax.isInfant() ? 0 : 1);
    NewTextChild(paxNode, "refuse", "");
    NewTextChild(paxNode, "reg_no", seat ? seat->regNo() : "");
    std::string subcls = "Э";
    if(reserv && reserv->subclass()) {
        subcls = reserv->subclass()->code(RUSSIAN);
    } else {
        LogWarning(STDLOG) << "use default subclass [Э]";
    }
    NewTextChild(paxNode, "subclass", subcls); // TODO
    NewTextChild(paxNode, "bag_pool_num", ""); // здесь не может заполнить - нет контекста
    NewTextChild(paxNode, "tid", 0); // TODO

    boost::optional<Ticketing::TicketCpn_t> tickCpn;
    std::string tickRem;
    if(service) {
        tickCpn = service->findTicketCpn(pax.isInfant());
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
    if(doc)
    {
        tst();
        NewTextChild(docNode, "type", doc->docType());
        NewTextChild(docNode, "issue_country", doc->issueCountry());
        NewTextChild(docNode, "no", doc->no());
        NewTextChild(docNode, "nationality", doc->nationality());
        if(!doc->birthDate().is_not_a_date()) {
            NewTextChild(docNode, "birth_date",
                         BASIC::date_time::boostDateToAstraFormatStr(doc->birthDate()));
        }
        NewTextChild(docNode, "gender", doc->gender());
        NewTextChild(docNode, "surname", doc->surname());
        NewTextChild(docNode, "first_name", doc->name());
        if(!doc->secondName().empty()) {
            NewTextChild(docNode, "second_name", doc->secondName());
        }
        if(!doc->expiryDate().is_not_a_date()) {
            NewTextChild(docNode, "expiry_date",
                         BASIC::date_time::boostDateToAstraFormatStr(doc->expiryDate()));
        }
    }

    NewTextChild(paxNode, "pr_norec", 0); // TODO
    NewTextChild(paxNode, "pr_bp_print", 0); // TODO
    NewTextChild(paxNode, "pr_bi_print", 0); // TODO


    xmlNodePtr paxRemsNode = newChild(paxNode, "rems");
    if(service)
    {
        for(const ServiceDetails::SsrInfo& ssr: service->lSsr())
        {
            if(ssr.isTkn()) continue;
            if(ssr.isFqt()) continue;

            xmlNodePtr paxRemNode = newChild(paxRemsNode, "rem");
            NewTextChild(paxRemNode, "rem_code", ssr.ssrCode());
            NewTextChild(paxRemNode, "rem_text", ssr.freeText());
        }
    }

    xmlNodePtr paxFqtRemsNode = newChild(paxNode, "fqt_rems");
    if(service)
    {
        for(const ServiceDetails::SsrInfo& ssr: service->lSsr())
        {
            if(!ssr.isFqt()) continue;

            xmlNodePtr paxFqtRemNode = newChild(paxFqtRemsNode, "fqt_rem");
            NewTextChild(paxFqtRemNode, "rem_code", ssr.ssrCode());
            NewTextChild(paxFqtRemNode, "airline",  ssr.airline());
            NewTextChild(paxFqtRemNode, "no",       ssr.ssrText());
        }
    }

    NewTextChild(paxNode, "iatci_pax_id", pax.respRef());
    if(parentPaxId) {
        NewTextChild(paxNode, "iatci_parent_pax_id", parentPaxId);
    } else {
        NewTextChild(paxNode, "iatci_parent_pax_id");
    }

    return paxNode;
}

static boost::optional<dcrcka::PaxGroup> findPaxGroup(const std::list<dcrcka::PaxGroup>& lPxg,
                                                      const Ticketing::TicketNum_t& tickNum,
                                                      bool& isInfant)
{
    for(const auto& pxg: lPxg) {
        if(pxg.service()) {
            for(const auto& ssr: pxg.service()->lSsr()) {
                if(ssr.isTkne()) {
                    if(ssr.toTicketCpn().ticket() == tickNum) {
                        isInfant = ssr.isInfantTicket();
                        return pxg;
                    }
                }
            }
        }
    }

    return boost::none;
}

static void traceTickNums(const std::list<Ticketing::TicketNum_t>& tnums,
                          const std::string& header,
                          int loglevel, const char* nick, const char* file, int line)
{
    LogTrace(loglevel, nick, file, line) << header;
    for(const auto& t: tnums) {
        LogTrace(loglevel, nick, file, line) << t << ", ";
    }
}

static bool tickNumsAreEqual(const std::list<Ticketing::TicketNum_t>& tn1,
                             const std::list<Ticketing::TicketNum_t>& tn2)
{
    if(tn1.size() != tn2.size()) return false;
    for(const auto& t: tn1) {
        if(!algo::contains(tn2, t)) {
            return false;
        }
    }

    return true;
}

bool checkTickNums(const iatci::dcrcka::Result& res,
                   const std::list<Ticketing::TicketNum_t>& tickNums)
{
    std::list<Ticketing::TicketNum_t> resTickNums = res.tickNums();
    if(!tickNumsAreEqual(resTickNums, tickNums)) {
        LogError(STDLOG) << "Ticknums check failed!";
        traceTickNums(resTickNums, "iatci ticknums", TRACE0);
        traceTickNums(tickNums, "ticknums", TRACE0);
        return false;
    }
    return true;
}

void xmlViewIatciPaxes_tickNumOrder(xmlNodePtr paxesNode,
                                    const std::list<dcrcka::PaxGroup>& paxGroups,
                                    const std::list<Ticketing::TicketNum_t>& tickNumOrder)
{
    ASSERT(!tickNumOrder.empty());

    std::map<Ticketing::TicketNum_t, int> inftTicknums;

    int currPax = 0;
    for(const auto& tickNum: tickNumOrder) {
        bool isInfant = false;
        boost::optional<dcrcka::PaxGroup> pxgOpt = findPaxGroup(paxGroups,
                                                                tickNum,
                                                                isInfant);
        ASSERT(pxgOpt);
        const auto& pxg = pxgOpt.get();
        int parentPaxId = 0;
        if(!isInfant) {
            xmlViewIatciPax(paxesNode,
                            pxg.pax(),
                            pxg.reserv(),
                            pxg.seat(),
                            pxg.service(),
                            pxg.doc(),
                            parentPaxId,
                            currPax);
            if(pxg.infant()) {
                ASSERT(pxg.tickNumInfant());
                inftTicknums.insert(std::make_pair(pxg.tickNumInfant().get(), currPax));
            }
        } else {
            ASSERT(pxg.infant());
            if(algo::contains(inftTicknums, tickNum)) {
                parentPaxId = inftTicknums.at(tickNum);
            } else {
                LogError(STDLOG) << "Unable to find infant parent! Ticknum: " << tickNum;
            }

            xmlViewIatciPax(paxesNode,
                            pxg.infant().get(),
                            pxg.reserv(),
                            pxg.infantSeat(),
                            pxg.service(),
                            pxg.infantDoc(),
                            parentPaxId,
                            currPax);
        }
    }
}

void xmlViewIatciPaxes_asisOrder(xmlNodePtr paxesNode,
                                 const std::list<dcrcka::PaxGroup>& paxGroups)
{
    int currPax = 0;
    for(const auto& pxg: paxGroups) {
        xmlViewIatciPax(paxesNode,
                        pxg.pax(),
                        pxg.reserv(),
                        pxg.seat(),
                        pxg.service(),
                        pxg.doc(),
                        0,
                        currPax);
        if(pxg.infant()) {
            xmlViewIatciPax(paxesNode,
                            pxg.infant().get(),
                            pxg.reserv(),
                            pxg.infantSeat(),
                            pxg.service(),
                            pxg.infantDoc(),
                            currPax-1,
                            currPax);
        }
    }
}

void iatci2xml(xmlNodePtr node, const iatci::dcrcka::Result& res,
               const IatciViewXmlParams& viewParams)
{
    xmlNodePtr segNode = xmlViewIatciFlight(node, res.flight());
    xmlNodePtr paxesNode = newChild(segNode, "passengers");
    if(!viewParams.tickNumOrder().empty() && checkTickNums(res, viewParams.tickNumOrder())) {
        xmlViewIatciPaxes_tickNumOrder(paxesNode, res.paxGroups(), viewParams.tickNumOrder());
    } else {
        LogError(STDLOG) << "Warning! Unable to perform ticknums order!";
        xmlViewIatciPaxes_asisOrder(paxesNode, res.paxGroups());
    }

    NewTextChild(segNode, "paid_bag_emd", ""); // TODO
    xmlNodePtr tripCountersNode = newChild(segNode, "tripcounters"); // TODO
    NewTextChild(segNode, "load_residue", ""); // TODO
}

void iatci2xml(xmlNodePtr node, const std::list<iatci::dcrcka::Result>& lRes,
               const IatciViewXmlParams& viewParams)
{
    for(const auto& res: lRes) {
        iatci2xml(node, res, viewParams);
    }
}

//---------------------------------------------------------------------------------------

static PlaceMatrix createPlaceMatrix(const SeatmapDetails& seatmap)
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
                                         "К", // TODO get it
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

void iatci2xmlSmp(xmlNodePtr node, const dcrcka::Result& res)
{
    ASSERT(res.seatmap());
    PlaceMatrix placeMatrix = createPlaceMatrix(res.seatmap().get());

    const iatci::FlightDetails& flight = res.flight();

    NewTextChild(node, "trip",        fullFlightString(flight, false));
    NewTextChild(node, "craft",       ""); // TODO get it
    NewTextChild(node, "bort",        ""); // TODO get it
    NewTextChild(node, "travel_time", depTimeString(flight));
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
    NewTextChild(depItemNode, "airp", airportCode(flight.depPort()));

    xmlNodePtr arrItemNode = newChild(itemsNode, "item");
    NewTextChild(arrItemNode, "point_id", -1);
    NewTextChild(arrItemNode, "airp", airportCode(flight.arrPort()));

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

void iatci2xmlSmpUpd(xmlNodePtr node, const dcrcka::Result& res,
                     const iatci::Seat& oldSeat, const iatci::Seat& newSeat)
{
    ASSERT(res.seatmap());
    PlaceMatrix placeMatrix = createPlaceMatrix(res.seatmap().get());

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

}//namespace iatci
