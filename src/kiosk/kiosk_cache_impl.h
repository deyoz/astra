#pragma once

#include "cache_callbacks.h"

namespace CacheTable
{

class KioskLang : public CacheTableWritable
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

class KioskAliasesList : public CacheTableWritableSimple
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
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

class KioskConfigList : public CacheTableWritableSimple
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override;
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
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

class KioskAliases : public CacheTableKeepDeletedRows
{
  public:
    bool userDependence() const override;
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::string selectSql() const override { return getSelectOrRefreshSql(false); };
    std::string refreshSql() const override { return getSelectOrRefreshSql(true); };
    std::string insertSqlOnApplyingChanges() const override;
    std::string updateSqlOnApplyingChanges() const override;
    std::string deleteSqlOnApplyingChanges() const override;
    std::list<std::string> dbSessionObjectNamesForRead() const override;
    std::string tableName() const override;
    std::string idFieldName() const override;
    void bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const override;
    std::optional<RowId_t> getRowIdBeforeInsert(const CacheTable::Row& row) const override;
};

class KioskConfig : public CacheTableKeepDeletedRows
{
  public:
    bool userDependence() const override;
    std::string getSelectOrRefreshSql(const bool isRefreshSql) const;
    std::string selectSql() const override { return getSelectOrRefreshSql(false); };
    std::string refreshSql() const override { return getSelectOrRefreshSql(true); };
    std::string insertSqlOnApplyingChanges() const override;
    std::string updateSqlOnApplyingChanges() const override;
    std::string deleteSqlOnApplyingChanges() const override;
    std::list<std::string> dbSessionObjectNamesForRead() const override;
    std::string tableName() const override;
    std::string idFieldName() const override;
    void bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const override;
    std::optional<RowId_t> getRowIdBeforeInsert(const CacheTable::Row& row) const override;
};

class KioskGrp : public CacheTableWritable
{
  public:
    bool userDependence() const override;
    std::string selectSql() const override { return ""; };
    std::string insertSql() const override;
    std::string updateSql() const override;
    std::string deleteSql() const override;
    std::list<std::string> dbSessionObjectNames() const override;
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const override;
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const override;
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const override {}
};


} //namespace CacheTable

