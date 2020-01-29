#pragma once

#include <memory>

#include "mct.h"

namespace ssim { namespace mct {

struct ScdSeg
{
    Status status;
    ct::Flight flt;
    nsi::PointId conxStation;
    nsi::PointId oppositeStation;
    boost::optional<nsi::TermId> conxTerminal;
    boost::optional<nsi::AircraftTypeId> aircraftType;
    AircraftBody aircraftBodyType;
    boost::optional<ct::Flight> oprFlt;
};

class SeekerImpl;

class Seeker
{
    std::unique_ptr<SeekerImpl> impl_;
public:

    enum FuzzyMatchPolicy { Minimum, Maximum };

    explicit Seeker(const std::vector<Record>&, FuzzyMatchPolicy);
    ~Seeker();

    const Record* search(const ScdSeg& arrSg, const ScdSeg& depSg, const boost::gregorian::date& dt) const;
};

class GeoRelationsImpl;

class GeoRelations
{
    std::unique_ptr<GeoRelationsImpl> impl_;
public:
    GeoRelations();
    ~GeoRelations();
    bool belongsTo(const nsi::GeozoneId&, const nsi::PointId&) const;
    bool belongsTo(const nsi::CountryId&, const nsi::PointId&) const;
    bool belongsTo(const nsi::RegionId&, const nsi::PointId&) const;
};

bool isMatched(
    const Record&, const ScdSeg& arrSeg, const ScdSeg& depSeg,
    const boost::gregorian::date&, const GeoRelations&
);

} } //ssim::mct
