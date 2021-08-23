#pragma once

#include "astra_types.h"
#include "astra_dates.h"

#include <optional>
#include <vector>

namespace ASTRA {

std::optional<AirportCode_t> get_last_trfer_airp(const GrpId_t& grpId);

}//namespace ASTRA
