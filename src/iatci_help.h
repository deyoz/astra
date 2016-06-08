#pragma once

#include "iatci_types.h"
#include "xml_unit.h"


namespace iatci {

std::string fullFlightString(const FlightDetails& flight);
std::string flightString(const FlightDetails& flight);
std::string airlineAccode(const std::string& airline);
std::string airportCode(const std::string& airport);
std::string airportCityCode(const std::string& airport);
std::string airportCityName(const std::string& airport);
std::string depDateTimeString(const FlightDetails& flight);
std::string fullAirportString(const std::string& airport);
std::string cityCode(const std::string& city);
std::string cityName(const std::string& city);
std::string paxTypeString(const PaxDetails& pax);
std::string paxSexString(const PaxDetails& pax);

//-----------------------------------------------------------------------------

XMLDoc createXmlDoc(const std::string& xml);

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

class IatciDb
{
public:
    static void add(int grpId, const std::list<iatci::Result>& lRes);

    static std::list<iatci::PaxDetails> readPax(int grpId);
    static std::list<iatci::FlightDetails> readSeg(int grpId);

private:
    static void addPax(int grpId, const std::list<iatci::Result>& lRes);
    static void addSeg(int grpId, const std::list<iatci::Result>& lRes);
};

}//namespace iatci
