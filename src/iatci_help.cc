#include "iatci_help.h"
#include "base_tables.h"
#include "astra_elems.h"
#include "astra_consts.h"
#include "astra_utils.h"

#include <serverlib/dates_io.h>
#include <serverlib/dates_oci.h>
#include <serverlib/cursctl.h>
#include <serverlib/xml_stuff.h>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

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
    const TAirlinesRow& airlRow = (const TAirlinesRow&)base_tables.get("airlines").get_row("code/code_lat", airline);
    std::ostringstream os;
    os << airlRow.AsString("aircode");
    return os.str();
}

std::string airportCode(const std::string& airport)
{
    const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code", airport);
    return ElemIdToCodeNative(etAirp, airpsRow.code);
}

std::string airportCityCode(const std::string& airport)
{
    const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code", airport);
    return cityCode(airpsRow.city);
}

std::string airportCityName(const std::string& airport)
{
    const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code", airport);
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
    const TAirpsRow& airpsRow = (const TAirpsRow&)base_tables.get("airps").get_row("code", airport);
    std::ostringstream os;
    os << ElemIdToNameLong(etCity, airpsRow.city)
       << " (" << ElemIdToCodeNative(etAirp, airpsRow.code) << ")";
    return os.str();
}

std::string cityString(const std::string& city)
{
    const TCitiesRow& citiesRow = (TCitiesRow&)base_tables.get("cities").get_row("code", city);
    return ElemIdToNameShort(etCity, citiesRow.id);
}

std::string cityCode(const std::string& city)
{
    const TCitiesRow& citiesRow = (TCitiesRow&)base_tables.get("cities").get_row("code", city);
    return ElemIdToCodeNative(etCity, citiesRow.id);
}

std::string cityName(const std::string& city)
{
    const TCitiesRow& citiesRow = (TCitiesRow&)base_tables.get("cities").get_row("code", city);
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

}//namespace iatci
