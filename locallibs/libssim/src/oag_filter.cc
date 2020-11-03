#include <serverlib/str_utils.h>

#include "oag_filter.h"

namespace Oag {

OagFlightFilter::OagFlightFilter(const std::string& air, std::size_t flNum)
    : airline(air), flightNum(flNum)
{
}

bool OagFlightFilter::applyFilter(const CarrierRecord& rec)
{
    if (flightNum == std::size_t(-1)) {
        if (airline.empty()) {
            return true;
        }
        return StrUtils::trim(rec.airline) == airline;
    }
    if (rec.flightLegs.empty()) {
        return false;
    }
    return StrUtils::trim(rec.airline) == airline && rec.flightLegs.front().flightNum == flightNum;
}

bool OagFlightFilter::checkAirline(const CarrierRecord& rec)
{
    if (airline.empty()) {
        return true;
    }
    return StrUtils::trim(rec.airline) == airline;
}

ServiceTypeFilter::ServiceTypeFilter(const std::string & v)
    : ignoredServices(v)
{}

bool ServiceTypeFilter::applyFilter(const CarrierRecord& rec)
{
    for (const auto& leg : rec.flightLegs) {
        if (ignoredServices.find(leg.serviceType) != std::string::npos) {
            return false;
        }
    }
    return true;
}

bool ServiceTypeFilter::checkAirline(const CarrierRecord&)
{
    return true;
}

}
