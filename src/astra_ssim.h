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
using namespace ct;

#include <nsi/nsi.h>
#include <nsi/callbacks.h>
using namespace nsi;

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>

#include <libssim/callbacks.h>
#include <libssim/ssim_data_types.h>

#include <season.h>
//using namespace SEASON;

//------------------------------------------------------------------------------------------

class AstraSsimCallbacks : public ssim::SsimTlgCallbacks
{
public:
    // getSchedules надо реализовать
    virtual ssim::ScdPeriods getSchedules(const ct::Flight&, const Period&) const override;

    // остальное заглушки
    virtual ssim::ScdPeriods getSchedulesWithOpr(nsi::CompanyId, const ct::Flight&, const Period&) const override;

    virtual Expected< boost::optional<ssim::CshSettings> > cshSettingsByTlg(nsi::CompanyId, ssim::ScdPeriod&) const override;

    virtual Expected< boost::optional<ssim::CshSettings> > cshSettingsByScd(const ssim::ScdPeriod&) const override;

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
    virtual boost::optional<DocTypeData> findDocTypeData(const nsi::DocTypeId&) override;

    // заглушки
    virtual boost::optional<nsi::SsrTypeId> findSsrTypeId(const EncString&) override;
    virtual boost::optional<SsrTypeData> findSsrTypeData(const nsi::SsrTypeId&) override;

    // заглушки
    virtual boost::optional<nsi::GeozoneId> findGeozoneId(const EncString&) override;
    virtual boost::optional<GeozoneData> findGeozoneData(const nsi::GeozoneId&) override;

    virtual boost::optional<nsi::CountryId> findCountryId(const EncString&) override;
    virtual boost::optional<nsi::CountryId> findCountryIdByIso(const EncString&) override;
    virtual boost::optional<CountryData> findCountryData(const nsi::CountryId&) override;

    virtual boost::optional<nsi::RegionId> findRegionId(const EncString&) override;
    virtual boost::optional<RegionData> findRegionData(const nsi::RegionId&) override;

    virtual boost::optional<nsi::CityId> findCityId(const EncString&) override;
    virtual boost::optional<CityData> findCityData(const nsi::CityId&) override;

    virtual boost::optional<nsi::PointId> findPointId(const EncString&) override;
    virtual boost::optional<PointData> findPointData(const nsi::PointId&) override;

    virtual boost::optional<nsi::CompanyId> findCompanyId(const EncString&) override;
    virtual boost::optional<nsi::CompanyId> findCompanyIdByAccountCode(const EncString&) override;
    virtual boost::optional<CompanyData> findCompanyData(const nsi::CompanyId&) override;

    virtual boost::optional<nsi::AircraftTypeId> findAircraftTypeId(const EncString&) override;
    virtual boost::optional<AircraftTypeData> findAircraftTypeData(const nsi::AircraftTypeId&) override;

    // заглушки
    virtual boost::optional<RouterId> findRouterId(const EncString&) override;
    virtual boost::optional<RouterData> findRouterData(const nsi::RouterId&) override;

    // заглушки
    virtual boost::optional<CurrencyId> findCurrencyId(const EncString&) override;
    virtual boost::optional<CurrencyData> findCurrencyData(const nsi::CurrencyId&) override;

    // заглушки
    virtual boost::optional<OrganizationId> findOrganizationId(const EncString&) override;
    virtual boost::optional<OrganizationData> findOrganizationData(const OrganizationId&) override;

};
//------------------------------------------------------------------------------------------

struct AstraSsimParseCollector : public ssim::ParseRequisitesCollector
{
    boost::optional<ct::Flight> flt;

    virtual void appendFlight(const ct::Flight&) override;
    virtual void appendPeriod(const ct::Flight&, const Period&) override;
};
//------------------------------------------------------------------------------------------

int HandleSSMTlg(string body);
int ssim_test(int argc, char **argv);

#endif
