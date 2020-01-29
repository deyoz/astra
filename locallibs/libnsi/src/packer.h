#ifndef LIBNSI_PACKER_H
#define LIBNSI_PACKER_H

#include <serverlib/json_packer.h>

#include "nsi.h"
#include "details.h"

namespace json_spirit
{

JSON_PACK_UNPACK_DECL(nsi::DocTypeId);
JSON_PACK_UNPACK_DECL(nsi::SsrTypeId);
JSON_PACK_UNPACK_DECL(nsi::GeozoneId);
JSON_PACK_UNPACK_DECL(nsi::CountryId);
JSON_PACK_UNPACK_DECL(nsi::RegionId);
JSON_PACK_UNPACK_DECL(nsi::CityId);
JSON_PACK_UNPACK_DECL(nsi::PointId);
JSON_PACK_UNPACK_DECL(nsi::PointOrCode);
JSON_PACK_UNPACK_DECL(nsi::TermId);
JSON_PACK_UNPACK_DECL(nsi::CompanyId);
JSON_PACK_UNPACK_DECL(nsi::AircraftTypeId);
JSON_PACK_UNPACK_DECL(nsi::RouterId);
JSON_PACK_UNPACK_DECL(nsi::CurrencyId);
JSON_PACK_UNPACK_DECL(nsi::DepArrCities);
JSON_PACK_UNPACK_DECL(nsi::DepArrPoints);
JSON_PACK_UNPACK_DECL(nsi::RestrictionId);
JSON_PACK_UNPACK_DECL(nsi::MealServiceId);
JSON_PACK_UNPACK_DECL(nsi::InflServiceId);
JSON_PACK_UNPACK_DECL(nsi::ServiceTypeId);
JSON_PACK_UNPACK_DECL(nsi::Router);
JSON_PACK_UNPACK_DECL(EncString);
JSON_PACK_UNPACK_DECL(nsi::FltServiceType);
JSON_PACK_UNPACK_DECL(nsi::CityPoint);
} // json_spirit

#endif /* LIBNSI_PACKER_H */

