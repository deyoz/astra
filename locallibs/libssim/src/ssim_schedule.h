#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <serverlib/expected.h>
#include <serverlib/period.h>

#include <coretypes/flight.h>
#include <coretypes/route.h>
#include <coretypes/rbdorder.h>

#include "meal_service.h"
#include "traffic_restriction.h"
#include "inflight_service.h"

namespace ssim {

struct Section
{
    nsi::PointId from, to;
    boost::posix_time::time_duration dep, arr;

    Section(nsi::PointId , nsi::PointId, const boost::posix_time::time_duration&, const boost::posix_time::time_duration&);
    nsi::DepArrPoints dap() const;
};
std::ostream& operator<< (std::ostream&, const Section&);
bool operator== (const Section&, const Section&);

struct Franchise
{
    boost::optional<nsi::CompanyId> code;
    std::string name;
};
std::ostream& operator<< (std::ostream&, const Franchise&);
bool operator== (const Franchise&, const Franchise&);

struct Leg
{
    Leg(const Section&, nsi::ServiceTypeId, nsi::AircraftTypeId, const ct::RbdLayout&);

    Section s;

    nsi::ServiceTypeId serviceType;
    nsi::AircraftTypeId aircraftType;
    ct::RbdLayout subclOrder;

    boost::optional<nsi::TermId> depTerm, arrTerm;

    boost::optional<ct::Flight> oprFlt;
    std::vector<ct::Flight> manFlts;
    boost::optional<Franchise> franchise;

    MealServiceInfo meals;
    InflightServices services;
    bool et;
    bool secureFlight;
};

using Legs = std::vector<Leg>;
std::ostream& operator<< (std::ostream&, const Leg&);
bool operator== (const Leg&, const Leg&);

struct SegProperty
{
    boost::optional<ct::RbdLayout> subclOrder;
    boost::optional<bool> et;
    boost::optional<MealServiceInfo> meals;
    Restrictions restrictions;

    bool empty() const;
};

using SegmentsProps = std::map<ct::SegNum, SegProperty>;
std::ostream& operator<< (std::ostream&, const SegProperty&);
bool operator==(const SegProperty&, const SegProperty&);

struct Route
{
    Route() {}
    Route(const Legs& i_legs, const SegmentsProps& i_segProps) : legs(i_legs), segProps(i_segProps) {}
    Legs legs;

    std::vector<nsi::PointId> getSegPoints(ct::SegNum) const;
    size_t segCount() const;

    SegmentsProps segProps;
};

std::ostream& operator<< (std::ostream&, const Route&);
bool operator==(const Route&, const Route&);

typedef std::pair<Period, ssim::Route> PeriodicRoute;
typedef std::vector<PeriodicRoute> PeriodicRoutes;

struct ExtraSegInfo
{
    boost::optional<nsi::PointId> depPt;
    boost::optional<nsi::PointId> arrPt;
    ct::DeiCode code;
    std::string content;
};

using ExtraSegInfos = std::vector<ExtraSegInfo>;
std::ostream& operator<< (std::ostream&, const ExtraSegInfo&);
bool operator==(const ExtraSegInfo&, const ExtraSegInfo&);

struct ScdPeriod
{
    ScdPeriod(const ct::Flight&);

    ct::Flight flight;
    Period period;
    Route route;
    bool adhoc;
    bool cancelled;

    ExtraSegInfos auxData;  //unsupported segment information in library
};

using ScdPeriods = std::vector<ScdPeriod>;
bool operator<(const ScdPeriod&, const ScdPeriod&);
bool operator==(const ScdPeriod&, const ScdPeriod&);
std::ostream& operator<< (std::ostream&, const ScdPeriod&);

boost::optional<ct::Flight> operatingFlight(const Route&);

} //ssim
