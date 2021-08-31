#pragma once

#include "cache_callbacks.h"

namespace CacheTable
{

class Roles : public CacheTableWritable
{
  public:
    std::string selectSql() const { return "";}
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    bool userDependence() const override { return true; }
    bool insertImplemented() const override { return true; }
    bool updateImplemented() const override { return true; }
    bool deleteImplemented() const override { return true; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TripListDays : public CacheTableWritable
{
  public:
    std::string selectSql() const { return "";}
    std::string insertSql() const;
    std::string updateSql() const;
    std::string deleteSql() const;
    bool userDependence() const override { return true; }
    bool insertImplemented() const override { return true; }
    bool updateImplemented() const override { return true; }
    bool deleteImplemented() const override { return true; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Rights : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    void beforeSelectOrRefresh(const TCacheQueryType queryType,
                               const TParams& sqlParams,
                               DB::TQuery& Qry) const;
    std::list<std::string> dbSessionObjectNames() const;
};

class ProfiledRightsList : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Users : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    void beforeSelectOrRefresh(const TCacheQueryType queryType,
                               const TParams& sqlParams,
                               DB::TQuery& Qry) const;
    std::list<std::string> dbSessionObjectNames() const;
};

class UserTypes : public CacheTableReadonly
{
  public:
    bool userDependence() const;
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

}
