#include "iatci_help.h"
#include "base_tables.h"
#include "astra_elems.h"
#include "astra_consts.h"
#include "astra_utils.h"

#include <serverlib/dates_io.h>
#include <serverlib/dates_oci.h>
#include <serverlib/cursctl.h>

#include <ostream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

std::string fullFlightString(const FlightDetails& flight)
{
    std::ostringstream os;
    os << flight.airline()
       << flight.flightNum()
       << "/" << HelpCpp::string_cast(flight.depDate(), "%d.%m")
       << " " << flight.depPort()
       << " (EDI)";
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
    if(!flight.depTime().is_not_a_date_time()) {
        os << " " << HelpCpp::string_cast(flight.depTime(), "%H:%M:%S");
    } else {
        os << " " << "00:00:00";
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

//-----------------------------------------------------------------------------

const size_t IatciXmlDb::PageSize = 1000;

void IatciXmlDb::add(int grpId, const std::string& xmlText)
{
    // TODO
    LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    OciCpp::CursCtl cur = make_curs(
"insert into GRP_IATCI_DATA(GRP_ID, ACTIVE) "
"values (:grp_id, 1)");
    cur.bind(":grp_id", grpId)
       .exec();

    saveXml(grpId, xmlText);
}

void IatciXmlDb::del(int grpId)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; grpId=" << grpId;
    OciCpp::CursCtl cur = make_curs(
"update GRP_IATCI_DATA set ACTIVE=0 where GRP_ID=:grp_id");
    cur.bind(":grp_id", grpId)
       .exec();
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
    std::string res, page;
    OciCpp::CursCtl cur = make_curs(
"select XML_TEXT from GRP_IATCI_XML where GRP_ID=:grp_id "
"and exists (select 1 from GRP_IATCI_DATA where GRP_ID=:grp_id and ACTIVE=1) "
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

void IatciDb::add(int grpId, const std::list<iatci::Result>& lRes)
{
    addPax(grpId, lRes);
    addSeg(grpId, lRes);
}

std::list<iatci::PaxDetails> IatciDb::readPax(int grpId)
{
    OciCpp::CursCtl cur = make_curs(
"select SURNAME, NAME, PAX_TYPE "
"from IATCI_PAX "
"where GRP_ID = :grp_id");

    std::string surname, name, paxType;
    cur.def(surname)
       .defNull(name, "")
       .defNull(paxType, "")
       .bind(":grp_id", grpId)
       .EXfet(); // пока на один GRP_ID храним одного PAX

    std::list<iatci::PaxDetails> res;
    res.push_back(iatci::PaxDetails(surname, name, iatci::PaxDetails::strToType(paxType)));

    return res;
}

std::list<iatci::FlightDetails> IatciDb::readSeg(int grpId)
{
    OciCpp::CursCtl cur = make_curs(
"select AIRLINE, FLT_NO, DEP_PORT, ARR_PORT, DEP_DATE, ARR_DATE "
"from IATCI_SEG "
"where GRP_ID = :grp_id order by NUM");

    std::string airline, flNum, depPort, arrPort;
    boost::gregorian::date depDate, arrDate;

    cur.def(airline)
       .def(flNum)
       .def(depPort)
       .def(arrPort)
       .def(depDate)
       .defNull(arrDate, boost::gregorian::date())
       .bind(":grp_id", grpId)
       .exec();
    std::list<iatci::FlightDetails> res;
    while(!cur.fen())  {
        res.push_back(iatci::FlightDetails(airline,
                                           Ticketing::getFlightNum(flNum),
                                           depPort,
                                           arrPort,
                                           depDate,
                                           arrDate));
    }

    return res;
}

void IatciDb::addPax(int grpId, const std::list<iatci::Result>& lRes)
{
    ASSERT(!lRes.empty());
    const iatci::Result& firstRes = lRes.front();
    ASSERT(firstRes.pax());
    iatci::PaxDetails pax = firstRes.pax().get();
    OciCpp::CursCtl cur = make_curs(
"insert into IATCI_PAX (SURNAME, NAME, GRP_ID) "
"values (:surname, :name, :grp_id)");
    cur.stb()
       .bind(":surname", pax.surname())
       .bind(":name",    pax.name())
       .bind(":grp_id",  grpId)
       .exec();
}

void IatciDb::addSeg(int grpId, const std::list<iatci::Result>& lRes)
{
    unsigned segNum = 1;
    for(const iatci::Result res: lRes) {
        iatci::FlightDetails flight = res.flight();
        OciCpp::CursCtl cur = make_curs(
"insert into IATCI_SEG "
"(AIRLINE, FLT_NO, DEP_PORT, ARR_PORT, DEP_DATE, ARR_DATE, NUM, GRP_ID) "
"values "
"(:airline, :flt_no, :dep_port, :arr_port, :dep_date, :arr_date, :num, :grp_id)");
    cur.stb()
       .bind(":airline", flight.airline())
       .bind(":flt_no",  flight.flightNum().get())
       .bind(":dep_port",flight.depPort())
       .bind(":arr_port",flight.arrPort())
       .bind(":dep_date",flight.depDate())
       .bind(":arr_date",flight.arrDate())
       .bind(":num",     segNum++)
       .bind(":grp_id",  grpId)
       .exec();
    }
}

}//namespace iatci
