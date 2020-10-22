#ifndef SSIM_ASTRA_H
#define SSIM_ASTRA_H

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/optional.hpp>

#include <serverlib/period.h>
#include <serverlib/expected.h>
#include <serverlib/dates.h>
#include <serverlib/str_utils.h>
#include <serverlib/string_cast.h>
#include <coretypes/flight.h>

#include <nsi/nsi.h>
#include <nsi/callbacks.h>

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>

#include <libssim/callbacks.h>
#include <libssim/ssim_data_types.h>

#include <season.h>

//------------------------------------------------------------------------------------------

class AstraSsimCallbacks : public ssim::SsimTlgCallbacks
{
public:
    // getSchedules надо реализовать
    virtual ssim::ScdPeriods getSchedules(const ct::Flight&, const Period&) const override;

    // остальное заглушки
    virtual ssim::ScdPeriods getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const override;

    virtual Expected< ssim::PeriodicCshs > cshSettingsByTlg(nsi::CompanyId, const ssim::ScdPeriod&) const override;

    virtual Expected< ssim::PeriodicCshs > cshSettingsByScd(const ssim::ScdPeriod&) const override;

    virtual ssim::DefValueSetter prepareDefaultValueSetter(ct::DeiCode, const nsi::DepArrPoints&, bool byLeg) const override;
};
//------------------------------------------------------------------------------------------

class AstraCallbacks
    : public nsi::Callbacks
{
public:
    virtual ~AstraCallbacks() {}

    virtual bool needCheckVersion() const override;

    virtual nsi::CityId centerCity() const override;

    // заглушки
    virtual boost::optional<nsi::DocTypeId> findDocTypeId(const EncString&) override;
    virtual boost::optional<nsi::DocTypeData> findDocTypeData(const nsi::DocTypeId&) override;

    // заглушки
    virtual boost::optional<nsi::SsrTypeId> findSsrTypeId(const EncString&) override;
    virtual boost::optional<nsi::SsrTypeData> findSsrTypeData(const nsi::SsrTypeId&) override;

    // заглушки
    virtual boost::optional<nsi::GeozoneId> findGeozoneId(const EncString&) override;
    virtual boost::optional<nsi::GeozoneData> findGeozoneData(const nsi::GeozoneId&) override;

    virtual boost::optional<nsi::CountryId> findCountryId(const EncString&) override;
    virtual boost::optional<nsi::CountryId> findCountryIdByIso(const EncString&) override;
    virtual boost::optional<nsi::CountryData> findCountryData(const nsi::CountryId&) override;

    virtual boost::optional<nsi::RegionId> findRegionId(const EncString&) override;
    virtual boost::optional<nsi::RegionData> findRegionData(const nsi::RegionId&) override;

    virtual boost::optional<nsi::CityId> findCityId(const EncString&) override;
    virtual boost::optional<nsi::CityData> findCityData(const nsi::CityId&) override;

    virtual boost::optional<nsi::PointId> findPointId(const EncString&) override;
    virtual boost::optional<nsi::PointData> findPointData(const nsi::PointId&) override;

    virtual boost::optional<nsi::CompanyId> findCompanyId(const EncString&) override;
    virtual boost::optional<nsi::CompanyId> findCompanyIdByAccountCode(const EncString&) override;
    virtual boost::optional<nsi::CompanyData> findCompanyData(const nsi::CompanyId&) override;

    virtual boost::optional<nsi::AircraftTypeId> findAircraftTypeId(const EncString&) override;
    virtual boost::optional<nsi::AircraftTypeData> findAircraftTypeData(const nsi::AircraftTypeId&) override;

    // заглушки
    virtual boost::optional<nsi::RouterId> findRouterId(const EncString&) override;
    virtual boost::optional<nsi::RouterData> findRouterData(const nsi::RouterId&) override;

    // заглушки
    virtual boost::optional<nsi::CurrencyId> findCurrencyId(const EncString&) override;
    virtual boost::optional<nsi::CurrencyData> findCurrencyData(const nsi::CurrencyId&) override;

    // заглушки
    std::set<nsi::GeozoneId> getGeozones(const nsi::CountryId&) override;
    std::set<nsi::GeozoneId> getGeozones(const nsi::RegionId&) override;
    std::set<nsi::GeozoneId> getGeozones(const nsi::CityId&) override;
};
//------------------------------------------------------------------------------------------

struct AstraSsimParseCollector : public ssim::ParseRequisitesCollector
{
    boost::optional<ct::Flight> flt;

    virtual void appendFlight(const ct::Flight&) override;
    virtual void appendPeriod(const ct::Flight&, const Period&) override;
};
//------------------------------------------------------------------------------------------

#include "flt_binding.h"

void HandleSSMTlg(string body, int tlg_id, TypeB::TFlightsForBind& flightsForBind);

#endif
