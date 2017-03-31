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

//---------------------------------------------------------------------------------------

XMLDoc createXmlDoc(const std::string& xml)
{
    XMLDoc doc;
    doc.set(ConvertCodepage(xml, "CP866", "UTF-8"));
    if(doc.docPtr() == NULL) {
        throw EXCEPTIONS::Exception("context %s has wrong XML format", xml.c_str());
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
                             ppd.m_passRespRef);
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
                                    pfd.m_securityId,
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
    return iatci::BaggageDetails(pbd.m_numOfPieces,
                                 pbd.m_weight);
}

iatci::ServiceDetails makeService(const edifact::PsiElem& psi)
{
    iatci::ServiceDetails service;
    for(const auto& ediSsr: psi.m_lSsr) {
        service.addSsr(iatci::ServiceDetails::SsrInfo(ediSsr.m_ssrCode,
                                                      ediSsr.m_ssrText,
                                                      false,
                                                      ediSsr.m_freeText,
                                                      ediSsr.m_airline));
    }
    return service;
}

iatci::DocDetails makeDoc(const edifact::PapElem& pap)
{
    return iatci::DocDetails(pap.m_docQualifier,
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
    return iatci::UpdateBaggageDetails(iatci::UpdateBaggageDetails::strToActionCode(ubd.m_actionCode),
                                       ubd.m_numOfPieces, ubd.m_weight);
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
                              ssr.m_freeText));
    }

    return updService;
}

iatci::UpdateDocDetails makeUpdDoc(const edifact::UapElem& uap)
{
    return iatci::UpdateDocDetails(iatci::UpdateDetails::strToActionCode(uap.m_actionCode),
                                   uap.m_docQualifier,
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

//---------------------------------------------------------------------------------------

iatci::PaxDetails makePax(const astra_api::astra_entities::PaxInfo& pax)
{
    LogTrace(TRACE3) << "pax.iatci_paxId = " << pax.m_iatciPaxId;
    return iatci::PaxDetails(pax.m_surname,
                             pax.m_name,
                             astra2iatci(pax.m_persType),
                             "",
                             pax.m_iatciPaxId);
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
    if(!pax.m_seatNo.empty()) {
        return iatci::FlightSeatDetails(pax.m_seatNo,
                                        pax.m_subclass ? pax.m_subclass->code(ENGLISH) : "",
                                        pax.m_regNo);
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

boost::optional<iatci::ServiceDetails> makeService(const astra_api::astra_entities::PaxInfo& pax)
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
    }

    if(pax.m_rems) {
        if(!service) {
            service = iatci::ServiceDetails();
        }

        for(const auto& rem: pax.m_rems->m_lRems) {
            service->addSsr(rem.m_remCode, rem.m_remText);
        }
    }

    return service;
}

boost::optional<iatci::BaggageDetails> makeBaggage(const astra_api::astra_entities::PaxInfo& pax)
{
    return iatci::BaggageDetails(0, 0);
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
    // TODO
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

static const int MaxSerializedDataLen = 4000;

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

void saveDeferredCkiData(tlgnum_t msgId, const std::list<dcrcka::Result>& lRes)
{
    std::string serialized = serialize(lRes);

    LogTrace(TRACE3) << __FUNCTION__
                     << " by msgId: " << msgId
                     << " [data size=" << serialized.size() << "]";

    ASSERT(serialized.size() < MaxSerializedDataLen);

    OciCpp::CursCtl cur = make_curs(
"insert into DEFERRED_CKI_DATA(MSG_ID, DATA) "
"values (:msg_id, :data)");
    cur.bind(":msg_id", msgId.num)
       .bind(":data", serialized)
       .exec();
}

std::list<dcrcka::Result> loadDeferredCkiData(tlgnum_t msgId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by msgId: " << msgId;

    char data[MaxSerializedDataLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data:=NULL;\n"
"delete from DEFERRED_CKI_DATA where MSG_ID=:msg_id "
"returning DATA into :data; \n"
"end;");
    cur.bind(":msg_id", msgId.num)
       .bindOutNull(":data", data, "")
       .exec();

    std::string serialized(data);

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

    ASSERT(serialized.size() < MaxSerializedDataLen);

    OciCpp::CursCtl cur = make_curs(
"insert into CKI_DATA(EDISESSION_ID, DATA) "
"values (:sessid, :data)");
    cur.bind(":sessid", sessId)
       .bind(":data", serialized)
       .exec();
}

std::list<dcrcka::Result> loadCkiData(edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by sessId: " << sessId;

    char data[MaxSerializedDataLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data:=NULL;\n"
"delete from CKI_DATA where EDISESSION_ID=:sessid "
"returning DATA into :data; \n"
"end;");

    cur.bind(":sessid", sessId)
       .bindOutNull(":data", data, "")
       .exec();

    std::string serialized(data);
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
    default:
        throw EXCEPTIONS::Exception("Unknown astra person type: %d", personType);
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
    denorm << denorm_iata_row(row, NULL) << denorm_iata_line(std::string(1, letter), true);
    return denorm.str();
}

}//namespace iatci
