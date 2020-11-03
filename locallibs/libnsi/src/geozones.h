#pragma once

#include <set>
#include <libnsi/nsi.h>

namespace nsi {

std::set<GeozoneId> getGeozones(const CountryId&);
std::set<GeozoneId> getGeozones(const RegionId&);
std::set<GeozoneId> getGeozones(const CityId&);

} //nsi
