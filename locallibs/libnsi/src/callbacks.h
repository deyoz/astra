#ifndef LIBNSI_CALLBACKS_H
#define LIBNSI_CALLBACKS_H

#include <boost/optional.hpp>

#include "nsi.h"

namespace nsi
{

class Callbacks
{
public:
    virtual ~Callbacks() {}

    virtual bool needCheckVersion() const = 0;

    virtual nsi::CityId centerCity() const = 0;

    virtual boost::optional<CompanyId> findCompanyId(const EncString&) = 0;
    virtual boost::optional<CompanyId> findCompanyIdByAccountCode(const EncString&) = 0;
    virtual boost::optional<CompanyData> findCompanyData(const CompanyId&) = 0;
    virtual size_t getCompanyMaxVersion() const;
    virtual CompanyId replaceObsoleteId(const CompanyId&);

    virtual boost::optional<DocTypeId> findDocTypeId(const EncString&) = 0;
    virtual boost::optional<DocTypeData> findDocTypeData(const DocTypeId&) = 0;
    virtual size_t getDocTypeMaxVersion() const;
    virtual DocTypeId replaceObsoleteId(const DocTypeId&);

    virtual boost::optional<SsrTypeId> findSsrTypeId(const EncString&) = 0;
    virtual boost::optional<SsrTypeData> findSsrTypeData(const SsrTypeId&) = 0;
    virtual size_t getSsrTypeMaxVersion() const;
    virtual SsrTypeId replaceObsoleteId(const SsrTypeId&);

    virtual boost::optional<GeozoneId> findGeozoneId(const EncString&) = 0;
    virtual boost::optional<GeozoneData> findGeozoneData(const GeozoneId&) = 0;
    virtual size_t getGeozoneMaxVersion() const;
    virtual GeozoneId replaceObsoleteId(const GeozoneId&);

    virtual std::set<GeozoneId> getGeozones(const CountryId&) = 0;
    virtual std::set<GeozoneId> getGeozones(const RegionId&) = 0;
    virtual std::set<GeozoneId> getGeozones(const CityId&) = 0;

    virtual boost::optional<CountryId> findCountryId(const EncString&) = 0;
    virtual boost::optional<CountryId> findCountryIdByIso(const EncString&) = 0;
    virtual boost::optional<CountryData> findCountryData(const CountryId&) = 0;
    virtual size_t getCountryMaxVersion() const;
    virtual CountryId replaceObsoleteId(const CountryId&);

    virtual boost::optional<RegionId> findRegionId(const EncString&) = 0;
    virtual boost::optional<RegionData> findRegionData(const RegionId&) = 0;
    virtual size_t getRegionMaxVersion() const;
    virtual RegionId replaceObsoleteId(const RegionId&);

    virtual boost::optional<CityId> findCityId(const EncString&) = 0;
    virtual boost::optional<CityData> findCityData(const CityId&) = 0;
    virtual size_t getCityMaxVersion() const;
    virtual CityId replaceObsoleteId(const CityId&);

    virtual boost::optional<PointId> findPointId(const EncString&) = 0;
    virtual boost::optional<PointData> findPointData(const PointId&) = 0;
    virtual size_t getPointMaxVersion() const;
    virtual PointId replaceObsoleteId(const PointId&);

    virtual boost::optional<AircraftTypeId> findAircraftTypeId(const EncString&) = 0;
    virtual boost::optional<AircraftTypeData> findAircraftTypeData(const AircraftTypeId&) = 0;
    virtual size_t getAircraftTypeMaxVersion() const;
    virtual AircraftTypeId replaceObsoleteId(const AircraftTypeId&);

    virtual boost::optional<RouterId> findRouterId(const EncString&) = 0;
    virtual boost::optional<RouterData> findRouterData(const nsi::RouterId&) = 0;
    virtual size_t getRouterMaxVersion() const;
    virtual RouterId replaceObsoleteId(const nsi::RouterId&);

    virtual boost::optional<CurrencyId> findCurrencyId(const EncString&) = 0;
    virtual boost::optional<CurrencyData> findCurrencyData(const nsi::CurrencyId&) = 0;
    virtual size_t getCurrencyMaxVersion() const;
    virtual CurrencyId replaceObsoleteId(const nsi::CurrencyId&);

    boost::optional<RestrictionId> findRestrictionId(const EncString&);
    boost::optional<RestrictionData> findRestrictionData(const RestrictionId&);
    size_t getRestrictionMaxVersion() const;
    RestrictionId replaceObsoleteId(const RestrictionId&);

    boost::optional<MealServiceId> findMealServiceId(const EncString&);
    boost::optional<MealServiceData> findMealServiceData(const nsi::MealServiceId&);
    size_t getMealServiceMaxVersion() const;
    MealServiceId replaceObsoleteId(const MealServiceId&);

    boost::optional<InflServiceId> findInflServiceId(const EncString&);
    boost::optional<InflServiceData> findInflServiceData(const nsi::InflServiceId&);
    size_t getInflServiceMaxVersion() const;
    InflServiceId replaceObsoleteId(const InflServiceId&);

    boost::optional<ServiceTypeId> findServiceTypeId(const EncString&);
    boost::optional<ServiceTypeData> findServiceTypeData(const nsi::ServiceTypeId&);
    size_t getServiceTypeMaxVersion() const;
    ServiceTypeId replaceObsoleteId(const ServiceTypeId&);
};

#ifdef XP_TESTING

void setupTestNsi();

#endif

} // nsi


#endif /* LIBNSI_CALLBACKS_H */

