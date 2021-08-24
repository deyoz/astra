#include "kiosk_cache_impl.h"
#include "PgOraConfig.h"
#include "astra_consts.h"
#include "astra_utils.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

namespace CacheTable
{

//KioskLang

bool KioskLang::userDependence() const {
  return false;
}
std::string KioskLang::selectSql() const {
  return "SELECT id, code, descr FROM kiosk_lang ORDER BY code";

}
std::string KioskLang::insertSql() const {
  return "INSERT INTO kiosk_lang(id, code, descr) "
         "VALUES(:id, :code, :descr)";
}
std::string KioskLang::updateSql() const {
  return "";
}
std::string KioskLang::deleteSql() const {
  return "DELETE FROM kiosk_lang WHERE id=:OLD_id";
}
std::list<std::string> KioskLang::dbSessionObjectNames() const {
  return {"KIOSK_LANG"};
}

void KioskLang::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

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

//KioskAliases

bool KioskAliases::userDependence() const {
  return false;
}
std::string KioskAliases::getSelectOrRefreshSql(const bool isRefreshSql) const {
  return std::string(
         "SELECT kiosk_aliases.*, kiosk_grp_names.name AS gname, kiosk_aliases.name AS code, "
                "kiosk_app_list.code AS appname, kiosk_lang.code AS lang "
         "FROM kiosk_aliases "
              "LEFT OUTER JOIN kiosk_grp_names ON kiosk_aliases.grp_id=kiosk_grp_names.id "
              "LEFT OUTER JOIN kiosk_app_list ON kiosk_aliases.app_id=kiosk_app_list.id "
              "INNER JOIN kiosk_lang ON kiosk_aliases.lang_id=kiosk_lang.id ") +
         "WHERE " + (isRefreshSql?"kiosk_aliases.tid>:tid ":"kiosk_aliases.pr_del=0 ") +
         "ORDER BY kiosk_aliases.name, kiosk_lang.code, kiosk_app_list.code, kiosk_aliases.grp_id, kiosk_aliases.kiosk_id";
}
std::string KioskAliases::insertSqlOnApplyingChanges() const {
  return "INSERT INTO kiosk_aliases(name, value, lang_id, app_id, grp_id, kiosk_id, descr, id, tid, pr_del) "
         "VALUES(:name, :value, :lang_id, :app_id, :grp_id, :kiosk_id, :descr, :id, :tid, :pr_del)";
}
std::string KioskAliases::updateSqlOnApplyingChanges() const {
  return "UPDATE kiosk_aliases "
         "SET name=:name, value=:value, lang_id=:lang_id, app_id=:app_id, grp_id=:grp_id, "
             "kiosk_id=:kiosk_id, descr=:descr, tid=:tid, pr_del=:pr_del "
         "WHERE id=:id";
}
std::string KioskAliases::deleteSqlOnApplyingChanges() const {
  return "DELETE FROM kiosk_aliases WHERE id=:id";
}
std::list<std::string> KioskAliases::dbSessionObjectNamesForRead() const {
  return {"KIOSK_ALIASES", "KIOSK_GRP_NAMES", "KIOSK_APP_LIST", "KIOSK_LANG"};
}
std::string KioskAliases::tableName() const {
  return "kiosk_aliases";
}
std::string KioskAliases::idFieldName() const {
  return "id";
}

void KioskAliases::bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const
{
  short null = -1, nnull = 0;
  std::optional<int> appId=row.getAsInteger("app_id");
  std::optional<int> grpId=row.getAsInteger("grp_id");

  cur.stb()
     .bind(":name",  row.getAsString("name"))
     .bind(":value", row.getAsString("value"))
     .bind(":lang_id", row.getAsInteger_ThrowOnEmpty("lang_id"))
     .bind(":app_id", appId ? appId.value() : 0, appId ? &nnull : &null)
     .bind(":grp_id", grpId ? grpId.value() : 0, grpId ? &nnull : &null)
     .bind(":kiosk_id", row.getAsString("kiosk_id"))
     .bind(":descr", row.getAsString("descr"));
}

std::optional<RowId_t> KioskAliases::getRowIdBeforeInsert(const CacheTable::Row& row) const
{
  auto cur=make_db_curs("SELECT id FROM kiosk_aliases WHERE id IN "
                        "  (SELECT id FROM kiosk_aliases WHERE pr_del<>0 FETCH FIRST 1 ROWS ONLY) FOR UPDATE",
                        PgOra::getRWSession("KIOSK_ALIASES"));
  int id;
  cur.stb()
     .def(id)
     .EXfet();

  if (cur.err() != DbCpp::ResultCode::NoDataFound) return RowId_t(id);

  return std::nullopt;
}

//KioskConfig

bool KioskConfig::userDependence() const {
  return false;
}
std::string KioskConfig::getSelectOrRefreshSql(const bool isRefreshSql) const {
  return std::string(
         "SELECT kiosk_config.*, kiosk_config.name AS code, kiosk_grp_names.name AS gname, "
                "kiosk_app_list.code AS appname, kiosk_config_list.descr as list_descr "
         "FROM kiosk_config "
              "LEFT OUTER JOIN kiosk_grp_names ON kiosk_config.grp_id=kiosk_grp_names.id "
              "LEFT OUTER JOIN kiosk_app_list ON kiosk_config.app_id=kiosk_app_list.id "
              "INNER JOIN kiosk_config_list ON kiosk_config.name=kiosk_config_list.code ")+
         "WHERE " + (isRefreshSql?"kiosk_config.tid>:tid ":"kiosk_config.pr_del=0 ") +
         "ORDER BY kiosk_config.name, kiosk_app_list.code, kiosk_config.grp_id, kiosk_config.kiosk_id";
}
std::string KioskConfig::insertSqlOnApplyingChanges() const {
  return "INSERT INTO kiosk_config(name, value, app_id, grp_id, kiosk_id, descr, id, tid, pr_del) "
         "VALUES(:name, :value, :app_id, :grp_id, :kiosk_id, :descr, :id, :tid, :pr_del)";
}
std::string KioskConfig::updateSqlOnApplyingChanges() const {
  return "UPDATE kiosk_config "
         "SET name=:name, value=:value, app_id=:app_id, grp_id=:grp_id, "
             "kiosk_id=:kiosk_id, descr=:descr, tid=:tid, pr_del=:pr_del "
         "WHERE id=:id";
}
std::string KioskConfig::deleteSqlOnApplyingChanges() const {
  return "DELETE FROM kiosk_config WHERE id=:id";
}
std::list<std::string> KioskConfig::dbSessionObjectNamesForRead() const {
  return {"KIOSK_CONFIG", "KIOSK_GRP_NAMES", "KIOSK_APP_LIST", "KIOSK_CONFIG_LIST"};
}
std::string KioskConfig::tableName() const {
  return "kiosk_config";
}
std::string KioskConfig::idFieldName() const {
  return "id";
}

void KioskConfig::bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const
{
  short null = -1, nnull = 0;
  std::optional<int> appId=row.getAsInteger("app_id");
  std::optional<int> grpId=row.getAsInteger("grp_id");

  cur.stb()
     .bind(":name",  row.getAsString("name"))
     .bind(":value", row.getAsString("value"))
     .bind(":app_id", appId ? appId.value() : 0, appId ? &nnull : &null)
     .bind(":grp_id", grpId ? grpId.value() : 0, grpId ? &nnull : &null)
     .bind(":kiosk_id", row.getAsString("kiosk_id"))
     .bind(":descr", row.getAsString("descr"));
}

std::optional<RowId_t> KioskConfig::getRowIdBeforeInsert(const CacheTable::Row& row) const
{
  auto cur=make_db_curs("SELECT id FROM kiosk_config WHERE id IN "
                        "  (SELECT id FROM kiosk_config WHERE pr_del<>0 FETCH FIRST 1 ROWS ONLY) FOR UPDATE",
                        PgOra::getRWSession("KIOSK_CONFIG"));
  int id;
  cur.stb()
     .def(id)
     .EXfet();

  if (cur.err() != DbCpp::ResultCode::NoDataFound) return RowId_t(id);

  return std::nullopt;
}

//KioskGrp

bool KioskGrp::userDependence() const {
  return false;
}
std::string KioskGrp::insertSql() const {
  return "INSERT INTO kiosk_grp(id, kiosk_id, grp_id) "
         "VALUES(:id, :kiosk_id, :grp_id)";
}
std::string KioskGrp::updateSql() const {
  return "UPDATE kiosk_grp "
         "SET kiosk_id=:kiosk_id, grp_id=:grp_id "
         "WHERE id=:OLD_id";
}
std::string KioskGrp::deleteSql() const {
  return "DELETE FROM kiosk_grp WHERE id=:OLD_id";
}
std::list<std::string> KioskGrp::dbSessionObjectNames() const {
  return {"KIOSK_GRP"};
}

std::set<std::string> getKioskIds()
{
  auto cur = make_db_curs("SELECT kiosk_id FROM web_clients WHERE client_type=:client_type AND kiosk_id IS NOT NULL",
                          PgOra::getROSession("WEB_CLIENTS"));

  std::string kioskId;
  cur.stb()
     .bind(":client_type", EncodeClientType(ASTRA::ctKiosk))
     .def(kioskId)
     .exec();

  std::set<std::string> kioskIds;
  while(!cur.fen()) kioskIds.insert(kioskId);

  return kioskIds;
}

void KioskGrp::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"KIOSK_GRP", "KIOSK_GRP_NAMES"}),
                 STDLOG);

  Qry.SQLText="SELECT kiosk_grp.id, kiosk_grp.grp_id, kiosk_grp.kiosk_id, "
                     "kiosk_grp_names.name, kiosk_grp_names.descr "
              "FROM kiosk_grp, kiosk_grp_names "
              "WHERE kiosk_grp.grp_id=kiosk_grp_names.id "
              "ORDER BY kiosk_grp_names.name, kiosk_grp.kiosk_id";
  Qry.Execute();

  if (Qry.Eof) return;

  std::set<std::string> kioskIds=getKioskIds();

  int idxId=Qry.FieldIndex("id");
  int idxGrpId=Qry.FieldIndex("grp_id");
  int idxKioskId=Qry.FieldIndex("kiosk_id");
  int idxName=Qry.FieldIndex("name");
  int idxDescr=Qry.FieldIndex("descr");
  for(; !Qry.Eof; Qry.Next())
  {
    if (!algo::contains(kioskIds, Qry.FieldAsString(idxKioskId))) continue;

    rows.setFromInteger(Qry, idxId)
        .setFromInteger(Qry, idxGrpId)
        .setFromString (Qry, idxName)
        .setFromString (Qry, idxKioskId)
        .setFromString (Qry, idxDescr)
        .addRow();
  }
}

void KioskGrp::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}


} //namespace CacheTables
