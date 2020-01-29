#pragma once

#include <memory>
#include <vector>
#include <string>

#include <serverlib/period.h>
#include <nsi/nsi.h>
#include <coretypes/coretypes.h>
#include <libssim/ssim_schedule.h>

namespace ssimexp {

typedef std::pair<ct::FlightNum, ct::FlightNum> FlightRange;

ssim::ScdPeriods createScheduleCollection(const nsi::CompanyId& company, const Period& period);

void createSsimFile(const std::string& filename, const nsi::Company& awk, bool iataOnly = false);

void createSsimFile(
    const std::string& filename,
    const nsi::Company& awk,
    const std::vector<FlightRange>& flightRanges,
    const Period& p,
    bool iataOnly = false
);

void createSsimFile(
    std::ostream& os,
    const nsi::Company& awk,
    const std::vector<FlightRange>& flightRanges,
    const Period& p,
    bool iataOnly = false
);

} //ssimexp
