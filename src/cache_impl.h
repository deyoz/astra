#pragma once

#include "cache_callbacks.h"
#include "cache_access.h"

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

}

