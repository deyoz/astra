#pragma once

#include "cache_callbacks.h"

CacheTableCallbacks* SpawnCacheTableCallbacks(const std::string& cacheCode);

namespace CacheTable
{

class GrpBagTypesOutdated : public CacheTableReadonlyHandmade
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpBagTypes : public CacheTableReadonlyHandmade
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpRfiscOutdated : public CacheTableReadonlyHandmade
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class GrpRfisc : public CacheTableReadonlyHandmade
{
  public:
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const;
};

class FileTypes : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "SELECT code, name FROM file_types WHERE code NOT IN ('BSM', 'HTTP_TYPEB') ORDER BY code";
    };
    std::string dbSessionObjectName() const { return "FILE_TYPES"; }
};

}

