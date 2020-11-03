#ifndef CORETYPES_FLIGHT_H
#define CORETYPES_FLIGHT_H

#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <serverlib/lngv.h>
#include <serverlib/rip_validators.h>
#include <libnsi/nsi.h>

namespace ct
{

DECL_RIP_RANGED(FlightNum, int, 1, 9999);
DECL_RIP_LESS(Suffix, unsigned, 27);

struct Flight
{
    Flight(nsi::CompanyId, FlightNum);
    Flight(nsi::CompanyId, FlightNum, Suffix);

    nsi::CompanyId airline;
    FlightNum number;
    Suffix suffix;

    std::string toString(Language lang = ENGLISH) const;
    std::string toStringUtf(Language lang = ENGLISH) const;
    std::string toTlgString(Language lang = ENGLISH) const;
    std::string toTlgStringUtf(Language lang = ENGLISH) const;
    static boost::optional<Flight> fromStr(const std::string&);
};

bool operator==(const Flight& lhs, const Flight& rhs);
bool operator!=(const Flight& lhs, const Flight& rhs);
bool operator<(const Flight& lhs, const Flight& rhs);
bool operator>(const Flight& lhs, const Flight& rhs);
std::ostream& operator<<(std::ostream&, const Flight&);

boost::optional<Suffix> suffixFromString(const std::string&);
boost::optional<Suffix> suffixFromString(unsigned char c);
std::string suffixToString(Suffix, Language lang = ENGLISH);

struct FlightDate
{
    Flight flt;
    boost::gregorian::date dt;
};
bool operator==(const FlightDate& lhs, const FlightDate& rhs);
bool operator!=(const FlightDate& lhs, const FlightDate& rhs);
bool operator<(const FlightDate& lhs, const FlightDate& rhs);
bool operator>(const FlightDate& lhs, const FlightDate& rhs);
std::ostream& operator<<(std::ostream&, const FlightDate&);

struct FlightSeg
{
    FlightSeg(const Flight&, const nsi::DepArrPoints&);

    Flight flt;
    nsi::DepArrPoints seg;
};
bool operator==(const FlightSeg& lhs, const FlightSeg& rhs);
bool operator!=(const FlightSeg& lhs, const FlightSeg& rhs);
bool operator<(const FlightSeg& lhs, const FlightSeg& rhs);
bool operator>(const FlightSeg& lhs, const FlightSeg& rhs);
std::ostream& operator<<(std::ostream&, const FlightSeg&);

struct FlightSegDate
{
    FlightSegDate(const Flight&, const nsi::DepArrPoints&, const boost::gregorian::date&);

    Flight flt;
    nsi::DepArrPoints seg;
    boost::gregorian::date dt;
};
bool operator==(const FlightSegDate& lhs, const FlightSegDate& rhs);
bool operator!=(const FlightSegDate& lhs, const FlightSegDate& rhs);
bool operator<(const FlightSegDate& lhs, const FlightSegDate& rhs);
bool operator>(const FlightSegDate& lhs, const FlightSegDate& rhs);
std::ostream& operator<<(std::ostream&, const FlightSegDate&);

struct FlightPointDate
{
    FlightPointDate(const Flight&, const nsi::PointId&, const boost::gregorian::date&);

    Flight flt;
    nsi::PointId dep;
    boost::gregorian::date dt;
};
bool operator==(const FlightPointDate& lhs, const FlightPointDate& rhs);
std::ostream& operator<<(std::ostream&, const FlightPointDate&);

} // ct

#endif /* CORETYPES_FLIGHT_H */

