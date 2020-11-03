#pragma once

#include "ssim_schedule.h"
#include "dei_default_values.h"
#include "csh_settings.h"

namespace ssim {

class ParseRequisitesCollector
{
public:
    virtual ~ParseRequisitesCollector() {}

    virtual void appendFlight(const ct::Flight&) = 0;
    virtual void appendPeriod(const ct::Flight&, const Period&) = 0;
};

class SsimTlgCallbacks
{
public:
    virtual ~SsimTlgCallbacks() {}

    virtual ssim::ScdPeriods getSchedules(const ct::Flight&, const Period&) const = 0;

    virtual ssim::ScdPeriods getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const = 0;

    //get codeshare settings by complete schedule constructed by telegram (NEW/RPL)
    virtual Expected< PeriodicCshs > cshSettingsByTlg(nsi::CompanyId, const ssim::ScdPeriod&) const = 0;

    //get codeshare settings by schedule based on existing one
    virtual Expected< PeriodicCshs > cshSettingsByScd(const ssim::ScdPeriod&) const = 0;

    virtual DefValueSetter prepareDefaultValueSetter(ct::DeiCode, const nsi::DepArrPoints&, bool byLeg) const = 0;
};

} //ssim
