#pragma once

#include <nsi/nsi.h>
#include <coretypes/rbdorder.h>
#include <coretypes/flight.h>
#include <serverlib/period.h>

namespace ssim {

struct RbdMapping
{
    ct::Rbd manRbd;
    ct::Cabin manCabin;
    ct::Rbd opRbd;
    ct::Cabin opCabin;
};

using RbdMappings = std::vector<RbdMapping>;

struct CshSettings
{
    nsi::CompanyId mkt;
    nsi::CompanyId opr;
    boost::optional<ct::Flight> oprFlt;

    std::map<nsi::DepArrPoints, RbdMappings> mappings;
};

using PeriodicCsh = std::pair<Period, CshSettings>;
using PeriodicCshs = std::vector<PeriodicCsh>;

} //ssim
