#pragma once

#include <map>
#include <set>

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include <libnsi/nsi.h>
#include <coretypes/route.h>

namespace ssim {

enum class TrafficType
{
    Passengers,
    Cargo,
    Mail
};

struct Restriction
{
    nsi::RestrictionId code;
    boost::optional<ct::DeiCode> application;   //DEI 170-173
};
bool operator< (const Restriction&, const Restriction&);
std::ostream& operator<< (std::ostream&, const Restriction&);
bool operator== (const Restriction&, const Restriction&);

struct Restrictions
{
    std::set<Restriction> restrs;
    boost::optional<ct::DeiCode> qualifier;     //DEI 710-712
    std::map<ct::DeiCode, std::string> info;    //DEI 713-799

    bool empty() const;
};
std::ostream& operator<<(std::ostream&, const Restrictions&);
bool operator== (const Restrictions&, const Restrictions&);

boost::optional<Restrictions> parseTrNote(const std::string&);
boost::optional<Restrictions> mergeTrNotes(const std::vector<Restrictions>&);

namespace tr {

struct Seg
{
    nsi::CompanyId airline;
    boost::posix_time::ptime depTm;
    boost::posix_time::ptime arrTm;
    ssim::Restrictions trs;
    bool international;
};
typedef std::vector<Seg> Segs;

} //tr

bool isSegmentRestricted(const Restrictions&, TrafficType = TrafficType::Passengers);
bool isTripRestricted(const tr::Segs&, TrafficType = TrafficType::Passengers);

void setupTlgRestrictions(
    const std::function<void (ct::DeiCode, const std::string&)>&,
    const Restrictions&
);


} //ssim
