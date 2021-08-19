#pragma once

#include "cache_callbacks.h"
#include "cache_access.h"
#include "astra_misc.h"

CacheTableCallbacks* SpawnCacheTableCallbacks(const std::string& cacheCode);

namespace CacheTable
{

class GrpBagTypesOutdated : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpBagTypes : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpRfiscOutdated : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpRfisc : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class FileTypes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class DeskWritable : public CacheTableWritable
{
  private:
    mutable std::optional< ViewAccess<DeskCode_t> > deskViewAccess;
  public:
    bool userDependence() const;
    std::string selectSql() const { return ""; }
    bool checkViewAccess(DB::TQuery& Qry, int idxDesk) const;
};

class FileEncoding : public DeskWritable
{
  private:
    bool out_;
  public:
    FileEncoding(bool out) : out_(out) {}
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class FileParamSets : public DeskWritable
{
  private:
    bool out_;
  public:
    FileParamSets(bool out) : out_(out) {}
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class Airlines : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Airps : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Cities : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TripTypes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TripSuffixes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TermProfileRights : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class PrnFormsLayout : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class GraphStages : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class GraphStagesWithoutInactive : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class StageSets : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class GraphTimes : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class StageNames : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class SoppStageStatuses : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class DeskPlusDeskGrpWritable : public CacheTableWritable
{
  private:
    mutable std::optional< ViewAccess<DeskGrpId_t> > deskGrpViewAccess;
    mutable std::optional< ViewAccess<DeskCode_t> > deskViewAccess;
  public:
    bool userDependence() const;
    std::string selectSql() const { return ""; }
    bool checkViewAccess(DB::TQuery& Qry, int idxDeskGrpId, int idxDesk) const;
    void checkAccess(const std::string& fieldDeskGrpId,
                     const std::string& fieldDesk,
                     const std::optional<CacheTable::Row>& oldRow,
                     std::optional<CacheTable::Row>& newRow) const;
};

class CryptSets : public DeskPlusDeskGrpWritable
{
  public:
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class CryptReqData : public DeskPlusDeskGrpWritable
{
  public:
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class SoppStations : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class HotelAcmd : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class HotelRoomTypes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class DevOperTypes : public CacheTableReadonly
{
public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class DevFmtOpers : public CacheTableReadonly
{
public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class DevFmtTypes : public CacheTableReadonly
{
public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class DevModels : public CacheTableReadonly
{
public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class CrsSet : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class Pacts : public CacheTableWritableHandmade
{
  public:
    bool userDependence() const;
    std::list<std::string> dbSessionObjectNames() const;

    std::string selectSql() const;
    std::string deleteSql() const;

    bool insertImplemented() const;
    bool updateImplemented() const;

    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const {}

    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;

    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class BaggageWt : public CacheTableWritableHandmade
{
  public:
    enum class Type { AllAirlines, SingleAirline, Basic };

  private:
    mutable std::optional<int> currentTid;
  protected:
    Type type_;
    std::string airlineSqlFilter() const;
    int getCurrentTid() const;
  public:
    BaggageWt(const Type type) : type_(type) {}
    virtual std::string getSelectOrRefreshSql(const bool isRefreshSql) const =0;
    bool userDependence() const;
    bool insertImplemented() const;
    bool updateImplemented() const;
    bool deleteImplemented() const;
    std::string selectSql() const;
    std::string refreshSql() const;
    void beforeSelectOrRefresh(const TCacheQueryType queryType,
                               const TParams& sqlParams,
                               DB::TQuery& Qry) const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const {}
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
};

class BagNorms : public BaggageWt
{
  public:
    BagNorms(const Type type) : BaggageWt(type) {}
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::list<std::string> dbSessionObjectNames() const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
};

class BagRates : public BaggageWt
{
  public:
    BagRates(const Type type) : BaggageWt(type) {}
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::list<std::string> dbSessionObjectNames() const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
};

class ValueBagTaxes : public BaggageWt
{
  public:
    ValueBagTaxes(const Type type) : BaggageWt(type) {}
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::list<std::string> dbSessionObjectNames() const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
};

class ExchangeRates : public BaggageWt
{
  public:
    ExchangeRates(const Type type) : BaggageWt(type) {}
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
};

class TripBaggageWt : public CacheTableReadonlyHandmade
{
  public:
    bool userDependence() const;
    static std::set<CityCode_t> getArrivalCities(const TAdvTripInfo& fltInfo);
};

class TripBagNorms : public TripBaggageWt
{
  private:
    bool outdated_;
  public:
    TripBagNorms(const bool outdated) : outdated_(outdated) {}
    bool prepareSelectQuery(const PointId_t& pointId,
                            const bool useMarkFlt,
                            const AirlineCode_t& airlineMark,
                            const FlightNumber_t& fltNumberMark,
                            DB::TQuery &Qry) const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class TripBagRates : public TripBaggageWt
{
  private:
    bool outdated_;
  public:
    TripBagRates(const bool outdated) : outdated_(outdated) {}
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class TripValueBagTaxes : public TripBaggageWt
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class TripExchangeRates : public TripBaggageWt
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class AirlineProfiles : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class ExtraRoleAccess : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class ExtraUserAccess : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeSelectOrRefresh(const TCacheQueryType queryType,
                               const TParams& sqlParams,
                               DB::TQuery& Qry) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class ValidatorTypes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class FormTypes : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class FormPacks : public CacheTableWritableHandmade
{
  public:
    bool userDependence() const;
    std::string selectSql() const { return ""; }
    bool updateImplemented() const;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
};

class SalePoints : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class Desk
{
  public:
    DeskCode_t code;
    RowId_t id;
    CurrencyCode_t currency;

    Desk(const DeskCode_t& code_,
         const RowId_t& id_,
         const CurrencyCode_t& currency_) :
      code(code_), id(id_), currency(currency_) {}
};

std::map<DeskCode_t, Desk> getDesks();

class SaleDesks : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const { return ""; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class Operators : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class PayClients : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class PosTermVendors : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const {}
};

class PosTermSets : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNamesForRead() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class PlaceCalc : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class CompSubclsSets : public CacheTableWritable
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    std::list<std::string> dbSessionObjectNames() const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
};

class CompRemTypes : public CacheTableKeepDeletedRows
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string refreshSql() const override;
    std::string insertSqlOnApplyingChanges() const override;
    std::string updateSqlOnApplyingChanges() const override;
    std::string deleteSqlOnApplyingChanges() const override;
    std::list<std::string> dbSessionObjectNamesForRead() const override;
    std::string tableName() const override;
    std::string idFieldName() const override;
    void bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const override;
    std::optional<RowId_t> getRowIdBeforeInsert(const CacheTable::Row& row) const override;
};

class CkinRemTypes : public CacheTableKeepDeletedRows
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string refreshSql() const override;
    std::string insertSqlOnApplyingChanges() const override;
    std::string updateSqlOnApplyingChanges() const override;
    std::string deleteSqlOnApplyingChanges() const override;
    std::list<std::string> dbSessionObjectNamesForRead() const override;
    std::string tableName() const override;
    std::string idFieldName() const override;
    void bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const override;
    std::optional<RowId_t> getRowIdBeforeInsert(const CacheTable::Row& row) const override;
};

class CodeshareSets : public CacheTableWritableHandmade
{
  public:
    std::string selectSql() const;
    bool userDependence() const override { return true; }
    bool insertImplemented() const override { return true; }
    bool updateImplemented() const override { return true; }
    bool deleteImplemented() const override { return true; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const {}
    void beforeSelectOrRefresh(const TCacheQueryType queryType,
                               const TParams& sqlParams,
                               DB::TQuery& Qry) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const;
    std::list<std::string> dbSessionObjectNames() const;
};

}

