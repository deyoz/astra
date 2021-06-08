#pragma once

#include "astra_misc.h"
#include "PgOraConfig.h"

namespace gtimer {

bool isClientStage(const int point_id, const TStage stage, int& pr_permit);

void sync_trip_final_stages(const int point_id);

bool execStage(const int point_id, const TStage stage, Dates::DateTime_t& act);

} // namespace gtimer
