#pragma once

#include <vector>
#include <map>

#include <serverlib/expected.h>
#include <coretypes/rbdorder.h>
#include <coretypes/route.h>
#include <libnsi/nsi.h>

#include "csh_context.h"

namespace ssim {

using MealServices = std::vector<nsi::MealServiceId>;
using MealServicesMap = std::map<ct::Rbd, MealServices>;

struct MealServiceInfo
{
    MealServicesMap rbdMeals;
    MealServices groupMeals;
};
bool operator== (const MealServiceInfo&, const MealServiceInfo&);

boost::optional<MealServiceInfo> getMealsMap(const std::string&);

Expected<MealServiceInfo> adjustedMeals(
    const MealServiceInfo&, const nsi::DepArrPoints&,
    const boost::optional<details::CshContext>&, bool byAcv
);

Expected<MealServicesMap> unfoldedMeals(
    const ct::RbdLayout& order, const MealServiceInfo& msi,
    const nsi::DepArrPoints& dap, bool byAcv
);

std::string mealsToString(const MealServicesMap&);
std::string mealsToString(const MealServiceInfo&);

using MealServiceCollector = std::function<void (ct::DeiCode, const std::string&)>;

void setupTlgMeals(
    const MealServiceCollector&,
    const boost::optional<MealServicesMap>&,const ct::Rbds&,
    bool segOvrd = false
);

} //ssim
