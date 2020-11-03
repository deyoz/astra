#pragma once

#include <string>

#include <boost/optional.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <serverlib/message.h>
#include <coretypes/flight.h>
#include <nsi/nsi.h>

namespace ssim { namespace mct {

enum class Status {
    Domestic = 0,
    International
};
ENUM_NAMES_DECL(Status)

enum class AircraftBody {
    Any = 0,
    Wide,
    Narrow
};
ENUM_NAMES_DECL(AircraftBody)

enum class ActionIndicator {
    Add = 0,
    Delete
};

enum class ContentIndicator {
    FullReplacement = 0,
    Updates
};

using FlightRange = std::pair<unsigned int, unsigned int>;

struct SegDesc
{
    Status status;

    boost::optional<nsi::PointId> conxStation;    //may be empty for global suppression
    boost::optional<nsi::TermId> conxTerminal;

    boost::optional<nsi::CompanyId> carrier;
    boost::optional<nsi::CompanyId> cshOprCarrier;
    bool cshIndicator;

    boost::optional<FlightRange> fltRange;

    boost::optional<nsi::AircraftTypeId> aircraftType;
    AircraftBody aircraftBodyType;

    //it is actually variant: none | geozone | country | country region | station
    boost::optional<nsi::GeozoneId> oppGeozone;     //region in iata terms
    boost::optional<nsi::CountryId> oppCountry;
    boost::optional<nsi::RegionId> oppRegion;       //state in iata terms
    boost::optional<nsi::PointId> oppStation;

    friend bool operator== (const SegDesc&, const SegDesc&);
    friend std::ostream& operator<< (std::ostream&, const SegDesc&);
};

bool isEqual(const SegDesc&, const SegDesc&);

struct Record
{
    SegDesc arrival;
    SegDesc departure;

    boost::gregorian::date effectiveFrom;
    boost::gregorian::date effectiveTo;

    bool suppressionIndicator;
    boost::optional<nsi::GeozoneId> suppressionGeozone;
    boost::optional<nsi::CountryId> suppressionCountry;
    boost::optional<nsi::RegionId> suppressionRegion;

    boost::posix_time::time_duration time;

    friend bool operator== (const Record&, const Record&);
    friend std::ostream& operator<< (std::ostream&, const Record&);
};

Message validate(const Record&);
bool hasPriority(const Record&, const Record&);
bool isDuplicated(const Record&, const Record&);

boost::posix_time::time_duration defaultMct(
    nsi::PointId arrPt, Status arrSt,
    nsi::PointId depPt, Status depSt
);

std::string packRecord(const Record&);
boost::optional<Record> unpackRecord(const std::string&);

} } //ssim::mct
