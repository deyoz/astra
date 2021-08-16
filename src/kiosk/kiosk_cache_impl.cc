#include "kiosk_cache_impl.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

namespace CacheTable
{

//KioskAliasesList

bool KioskAliasesList::userDependence() const {
  return false;
}
std::string KioskAliasesList::selectSql() const {
  return "SELECT code, code AS name, descr FROM kiosk_aliases_list ORDER BY name";

}
std::string KioskAliasesList::insertSql() const {
  return "INSERT INTO kiosk_aliases_list(code, descr) "
         "VALUES(:name, :descr)";
}
std::string KioskAliasesList::updateSql() const {
  return "UPDATE kiosk_aliases_list "
         "SET code=:name, descr=:descr "
         "WHERE code=:OLD_name";
}
std::string KioskAliasesList::deleteSql() const {
  return "DELETE FROM kiosk_aliases_list WHERE code=:OLD_name";
}
std::list<std::string> KioskAliasesList::dbSessionObjectNames() const {
  return {"KIOSK_ALIASES_LIST"};
}

//KioskAppList

bool KioskAppList::userDependence() const {
  return false;
}
std::string KioskAppList::selectSql() const {
  return "SELECT id, code, descr FROM kiosk_app_list ORDER BY code";

}
std::string KioskAppList::insertSql() const {
  return "INSERT INTO kiosk_app_list(id, code, descr) "
         "VALUES(:id, :code, :descr)";
}
std::string KioskAppList::updateSql() const {
  return "UPDATE kiosk_app_list "
         "SET code=:code, descr=:descr "
         "WHERE id=:OLD_id";
}
std::string KioskAppList::deleteSql() const {
  return "DELETE FROM kiosk_app_list WHERE id=:OLD_id";
}
std::list<std::string> KioskAppList::dbSessionObjectNames() const {
  return {"KIOSK_APP_LIST"};
}

void KioskAppList::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

void KioskAppList::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("kiosk_app_list").synchronize(getRowId("id", oldRow, newRow));
}

//KioskConfigList

bool KioskConfigList::userDependence() const {
  return false;
}
std::string KioskConfigList::selectSql() const {
  return "SELECT code, code AS name, descr FROM kiosk_config_list ORDER BY name";

}
std::string KioskConfigList::insertSql() const {
  return "INSERT INTO kiosk_config_list(code, descr) "
         "VALUES(:name, :descr)";
}
std::string KioskConfigList::updateSql() const {
  return "UPDATE kiosk_config_list "
         "SET code=:name, descr=:descr "
         "WHERE code=:OLD_name";
}
std::string KioskConfigList::deleteSql() const {
  return "DELETE FROM kiosk_config_list WHERE code=:OLD_name";
}
std::list<std::string> KioskConfigList::dbSessionObjectNames() const {
  return {"KIOSK_CONFIG_LIST"};
}

//KioskAddr

bool KioskAddr::userDependence() const {
  return false;
}
std::string KioskAddr::selectSql() const {
  return "SELECT id, name FROM kiosk_addr ORDER BY id";

}
std::string KioskAddr::insertSql() const {
  return "INSERT INTO kiosk_addr(id, name) "
         "VALUES(:id, :name)";
}
std::string KioskAddr::updateSql() const {
  return "UPDATE kiosk_addr "
         "SET name=:name "
         "WHERE id=:OLD_id";
}
std::string KioskAddr::deleteSql() const {
  return "DELETE FROM kiosk_addr WHERE id=:OLD_id";
}
std::list<std::string> KioskAddr::dbSessionObjectNames() const {
  return {"KIOSK_ADDR"};
}

void KioskAddr::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

void KioskAddr::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("kiosk_addr").synchronize(getRowId("id", oldRow, newRow));
}

//KioskGrpNames

bool KioskGrpNames::userDependence() const {
  return false;
}
std::string KioskGrpNames::selectSql() const {
  return "SELECT id, name, descr FROM kiosk_grp_names ORDER BY name";

}
std::string KioskGrpNames::insertSql() const {
  return "INSERT INTO kiosk_grp_names(id, name, descr) "
         "VALUES(:id, :name, :descr)";
}
std::string KioskGrpNames::updateSql() const {
  return "UPDATE kiosk_grp_names "
         "SET name=:name, descr=:descr "
         "WHERE id=:OLD_id";
}
std::string KioskGrpNames::deleteSql() const {
  return "DELETE FROM kiosk_grp_names WHERE id=:OLD_id";
}
std::list<std::string> KioskGrpNames::dbSessionObjectNames() const {
  return {"KIOSK_GRP_NAMES"};
}

void KioskGrpNames::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}


} //namespace CacheTables
