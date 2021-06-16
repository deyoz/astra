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
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Airlines : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class Airps : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TripTypes : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::string refreshSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TripSuffixes : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class TermProfileRights : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

class PrnFormsLayout : public CacheTableReadonly
{
  public:
    std::string selectSql() const;
    std::list<std::string> dbSessionObjectNames() const;
};

}

