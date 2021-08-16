#pragma once

#include "cache_callbacks.h"

namespace CacheTable
{

class KioskAliasesList : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override {}
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override {}
};

class KioskAppList : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override;
};

class KioskConfigList : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override {}
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override {}
};

class KioskAddr : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override;
};

class KioskGrpNames : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override {}
};


} //namespace CacheTable

