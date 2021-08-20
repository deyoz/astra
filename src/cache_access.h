#pragma once

#include <string>
#include <optional>

#include "astra_types.h"
#include "cache_callbacks.h"
#include "astra_utils.h"

enum class AccessControl { PermittedAirports,
                           PermittedAirportsOrNull,
                           PermittedAirlines,
                           PermittedAirlinesOrNull,
                           PermittedCities,
                           PermittedCitiesOrNull };

std::string getSQLFilter(const std::string& sqlFieldName, const AccessControl accessControl);

bool isPermitted(const std::optional<AirportCode_t>& airportOpt);
bool isPermitted(const std::optional<AirlineCode_t>& airlineOpt);
bool isPermitted(const std::optional<CityCode_t>& cityOpt);

DECL_RIP_LENGTH(ValidatorCode_t, std::string, 1, 4);

namespace CacheTable
{

void checkAirportAccess(const std::string& fieldName,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow);

void checkAirlineAccess(const std::string& fieldName,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow);

void checkCityAccess(const std::string& fieldName,
                     const std::optional<CacheTable::Row>& oldRow,
                     const std::optional<CacheTable::Row>& newRow);

void checkNotNullStageAccess(const std::string& stageIdFieldName,
                             const std::string& airlineFieldName,
                             const std::string& airportFieldName,
                             const std::optional<CacheTable::Row>& oldRow,
                             const std::optional<CacheTable::Row>& newRow);

} //CacheTable

template<class T>
class Access
{
  private:
    std::optional< std::set<T> > permitted;
    std::optional<T> id_;
    void init();
    void downloadPermissions();
  protected:
    bool totally_permitted=false;
  public:
    Access() { init(); }
    Access(const T& id) : id_(id) { init(); }
    bool check(const T& id);
};

template<class T>
class ViewAccess : public Access<T>
{
  private:
    std::optional< std::set<T> > permitted;
    std::optional<T> id_;
    void downloadPermissions();
  public:
    ViewAccess() {}
    ViewAccess(const T& id) :
      Access<T>(id), id_(id) {};
    bool check(const T& id);
};

template<class T>
bool Access<T>::check(const T& id)
{
  if (totally_permitted) return true;

  if (!permitted) downloadPermissions();

  return algo::contains(permitted.value(), id);
}

template<class T>
bool ViewAccess<T>::check(const T& id)
{
  if (Access<T>::check(id)) return true;

  if (!permitted) downloadPermissions();

  return algo::contains(permitted.value(), id);
}

namespace CacheTable
{

void checkDeskGrpAccess(const std::string& deskGrpIdFieldName,
                        const bool deskGrpIdFieldIsNullable,
                        const std::optional<CacheTable::Row>& oldRow,
                        const std::optional<CacheTable::Row>& newRow);

void checkDeskAccess(const std::string& deskFieldName,
                     const std::optional<CacheTable::Row>& oldRow,
                     const std::optional<CacheTable::Row>& newRow);

void checkDeskAndDeskGrp(const std::string& deskFieldName,
                         const std::string& deskGrpIdFieldName,
                         const bool deskGrpIdFieldIsNullable,
                         std::optional<CacheTable::Row>& row);

void checkUserTypesAccess(const std::string& userTypeFieldName1,
                          const std::string& userTypeFieldName2,
                          const std::optional<CacheTable::Row>& oldRow,
                          const std::optional<CacheTable::Row>& newRow);

void checkNotNullAirportOrAirportAccess(const std::string& fieldName1,
                                        const std::string& fieldName2,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow);

void checkAirlineOrAirlineAccess(const std::string& fieldName1,
                                 const std::string& fieldName2,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow);

} //namespace CacheTable

