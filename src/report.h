#pragma once

#include "astra_types.h"
#include "astra_dates.h"

#include <optional>
#include <vector>

namespace ASTRA {

std::optional<AirportCode_t> get_last_trfer_airp(const GrpId_t& grpId);


//std::vector<Dates::DateTime_t> get_airline_period_first_dates(const AirlineCode_t& airl);

//std::optional<Dates::DateTime_t> get_airline_period_last_date(const AirlineCode_t& airl,
//                                                              const Dates::DateTime_t& periodFirstDate);

//std::vector<Dates::time_period> get_airline_periods(const AirlineCode_t& airl,
//                                                    const Dates::DateTime_t& firstDate,
//                                                    const Dates::DateTime_t& lastDate);

//std::vector<AirlineCode_t> get_airline_list(const AirportCode_t& airp,
//                                            const Dates::time_period& period);



//std::vector<Dates::DateTime_t> get_airport_period_first_dates(const AirportCode_t& airp);

//std::optional<Dates::DateTime_t> get_airport_period_last_date(const AirportCode_t& airp,
//                                                              const Dates::DateTime_t& periodFirstDate);

//std::vector<Dates::time_period> get_airport_periods(const AirportCode_t& airp,
//                                                    const Dates::DateTime_t& firstDate,
//                                                    const Dates::DateTime_t& lastDate);

//std::vector<AirportCode_t> get_airport_list(const AirlineCode_t& airl,
//                                            const Dates::time_period& period);

}//namespace ASTRA
