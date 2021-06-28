#include "cache_impl.h"
#include "PgOraConfig.h"
#include "astra_elems.h"
#include "astra_types.h"

#include <serverlib/algo.h>
#include <serverlib/str_utils.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;

CacheTableCallbacks* SpawnCacheTableCallbacks(const std::string& cacheCode)
{
  if (cacheCode=="GRP_BAG_TYPES1")      return new CacheTable::GrpBagTypesOutdated;
  if (cacheCode=="GRP_BAG_TYPES2")      return new CacheTable::GrpBagTypesOutdated;
  if (cacheCode=="GRP_BAG_TYPES")       return new CacheTable::GrpBagTypes;
  if (cacheCode=="GRP_RFISC1")          return new CacheTable::GrpRfiscOutdated;
  if (cacheCode=="GRP_RFISC")           return new CacheTable::GrpRfisc;
  if (cacheCode=="FILE_TYPES")          return new CacheTable::FileTypes;
  if (cacheCode=="TERM_PROFILE_RIGHTS") return new CacheTable::TermProfileRights;
  if (cacheCode=="PRN_FORMS_LAYOUT")    return new CacheTable::PrnFormsLayout;
  if (cacheCode=="GRAPH_STAGES")        return new CacheTable::GraphStages;
  if (cacheCode=="GRAPH_STAGES_WO_INACTIVE") return new CacheTable::GraphStagesWithoutInactive;
  if (cacheCode=="STAGE_SETS")          return new CacheTable::StageSets;
  if (cacheCode=="GRAPH_TIMES")         return new CacheTable::GraphTimes;
  if (cacheCode=="STAGE_NAMES")         return new CacheTable::StageNames;
  if (cacheCode=="SOPP_STAGE_STATUSES") return new CacheTable::SoppStageStatuses;
  if (cacheCode=="CRYPT_SETS")          return new CacheTable::CryptSets;
  if (cacheCode=="CRYPT_REQ_DATA")      return new CacheTable::CryptReqData;
  if (cacheCode=="TRIP_SUFFIXES")       return new CacheTable::TripSuffixes;
#ifndef ENABLE_ORACLE
  if (cacheCode=="AIRLINES")            return new CacheTable::Airlines;
  if (cacheCode=="AIRPS")               return new CacheTable::Airps;
  if (cacheCode=="TRIP_TYPES")          return new CacheTable::TripTypes;
#endif //ENABLE_ORACLE
  return nullptr;
}

namespace CacheTable
{

bool GrpBagTypesOutdated::userDependence() const {
  return false;
}

void GrpBagTypesOutdated::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"BAG_TYPE_LIST_ITEMS",
                                      "PAX_SERVICE_LISTS",
                                      "GRP_SERVICE_LISTS"}),
                 STDLOG);

  Qry.SQLText=
          "SELECT "
          "  pax_service_lists.grp_id AS bag_types_id, "
          "  bag_type_list_items.bag_type AS code, "
          "  bag_type_list_items.name, "
          "  bag_type_list_items.name_lat, "
          "  bag_type_list_items.descr, "
          "  bag_type_list_items.descr_lat, "
          "  bag_type_list_items.bag_type AS priority, "
          "  CASE "
          "    WHEN bag_type_list_items.category = 1 THEN 0 "
          "    WHEN bag_type_list_items.category = 4 THEN 0 "
          "    WHEN bag_type_list_items.category = 2 THEN 1 "
          "    WHEN bag_type_list_items.category = 5 THEN 1 "
          "    ELSE NULL "
          "  END AS pr_cabin "
          "FROM bag_type_list_items "
          "  JOIN pax_service_lists "
          "    ON bag_type_list_items.list_id = pax_service_lists.list_id "
          "WHERE "
          "  pax_service_lists.grp_id = :bag_types_id "
          "  AND pax_service_lists.transfer_num = 0 "
          "  AND pax_service_lists.category IN (1, 2) "
          "  AND bag_type_list_items.category <> 0 "
          "  AND bag_type_list_items.visible <> 0 "
          "  AND bag_type_list_items.bag_type <> ' ' "
          "UNION "
          "SELECT "
          "  grp_service_lists.grp_id AS bag_types_id, "
          "  bag_type_list_items.bag_type AS code, "
          "  bag_type_list_items.name, "
          "  bag_type_list_items.name_lat, "
          "  bag_type_list_items.descr, "
          "  bag_type_list_items.descr_lat, "
          "  bag_type_list_items.bag_type AS priority, "
          "  CASE "
          "    WHEN bag_type_list_items.category = 1 THEN 0 "
          "    WHEN bag_type_list_items.category = 4 THEN 0 "
          "    WHEN bag_type_list_items.category = 2 THEN 1 "
          "    WHEN bag_type_list_items.category = 5 THEN 1 "
          "    ELSE NULL "
          "  END AS pr_cabin "
          "FROM bag_type_list_items "
          "  JOIN grp_service_lists "
          "    ON bag_type_list_items.list_id = grp_service_lists.list_id "
          "WHERE "
          "  grp_service_lists.grp_id = :bag_types_id "
          "  AND grp_service_lists.transfer_num = 0 "
          "  AND grp_service_lists.category IN (1, 2) "
          "  AND bag_type_list_items.category <> 0 "
          "  AND bag_type_list_items.visible <> 0 "
          "  AND bag_type_list_items.bag_type <> ' ' "
          "ORDER BY code ";

  CreateVariablesFromParams({"bag_types_id"}, sqlParams, Qry);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromString (Qry, 1) //для более гибкого подхода имеет смысл до цикла вычислить idxCode=Qry.FieldIndex("code") и применить setFromString (Qry, idxCode)
        .setFromString (Qry, 2)
        .setFromString (Qry, 3)
        .setFromString (Qry, 4)
        .setFromString (Qry, 5)
        .setFromBoolean(Qry, 7)
        .setFromInteger(Qry, 6)
        .setFromInteger(Qry, 0)
        .addRow();
  }
}

bool GrpBagTypes::userDependence() const {
  return false;
}

void GrpBagTypes::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"BAG_TYPE_LIST_ITEMS",
                                      "PAX_SERVICE_LISTS",
                                      "GRP_SERVICE_LISTS"}),
                 STDLOG);

  Qry.SQLText=
       "SELECT items.airline, items.bag_type, items.name, items.name_lat, items.descr, items.descr_lat "
       "FROM bag_type_list_items items, pax_service_lists "
       "WHERE pax_service_lists.grp_id=:grp_id AND "
       "      items.list_id=pax_service_lists.list_id "
       "UNION "
       "SELECT items.airline, items.bag_type, items.name, items.name_lat, items.descr, items.descr_lat "
       "FROM bag_type_list_items items, grp_service_lists "
       "WHERE grp_service_lists.grp_id=:grp_id AND "
       "      items.list_id=grp_service_lists.list_id "
       "ORDER BY airline, bag_type";

  CreateVariablesFromParams({"grp_id"}, sqlParams, Qry);
  Qry.Execute();

  int idxAirline=Qry.FieldIndex("airline");
  int idxBagType=Qry.FieldIndex("bag_type");
  int idxName=Qry.FieldIndex("name");
  int idxNameLat=Qry.FieldIndex("name_lat");
  int idxDescr=Qry.FieldIndex("descr");
  int idxDescrLat=Qry.FieldIndex("descr_lat");
  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromString(Qry, idxAirline)
        .setFromString(ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromString(Qry, idxBagType)
        .setFromString(Qry, idxName)
        .setFromString(Qry, idxNameLat)
        .setFromString(Qry, idxDescr)
        .setFromString(Qry, idxDescrLat)
        .addRow();
  }
}

namespace {

typedef std::pair<std::string,std::string> AirlineRfiscKey;
typedef std::pair<int,std::string> PriorityRfiscKey;

struct GrpRfiscItem
{
    GrpId_t grp_id;
    std::string rfisc;
    std::string name;
    std::string name_lat;
    std::string airline;
    int pr_cabin;
};

typedef std::vector<GrpRfiscItem> GrpRfiscItems;

} // namespace

bool GrpRfiscOutdated::userDependence() const {
  return false;
}

void GrpRfiscOutdated::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"RFISC_LIST_ITEMS",
                                      "PAX_SERVICE_LISTS",
                                      "GRP_SERVICE_LISTS"}),
                 STDLOG);

  Qry.SQLText =
          "SELECT "
          "  pax_service_lists.grp_id AS bag_types_id, "
          "  rfisc_list_items.rfisc, "
          "  rfisc_list_items.name, "
          "  rfisc_list_items.name_lat, "
          "  rfisc_list_items.airline, "
          "  CASE "
          "    WHEN rfisc_list_items.category = 1 THEN 0 "
          "    WHEN rfisc_list_items.category = 4 THEN 0 "
          "    WHEN rfisc_list_items.category = 2 THEN 1 "
          "    WHEN rfisc_list_items.category = 5 THEN 1 "
          "    ELSE NULL "
          "  END AS pr_cabin "
          "FROM rfisc_list_items "
          "  JOIN pax_service_lists "
          "    ON rfisc_list_items.list_id = pax_service_lists.list_id "
          "WHERE "
          "  pax_service_lists.grp_id = :bag_types_id "
          "  AND pax_service_lists.transfer_num = 0 "
          "  AND pax_service_lists.category IN (1, 2) "
          "  AND rfisc_list_items.category <> 0 "
          "  AND rfisc_list_items.visible <> 0 "
          "  AND rfisc_list_items.service_type = 'C' "
          "UNION "
          "SELECT "
          "  grp_service_lists.grp_id AS bag_types_id, "
          "  rfisc_list_items.rfisc, "
          "  rfisc_list_items.name, "
          "  rfisc_list_items.name_lat, "
          "  rfisc_list_items.airline, "
          "  CASE "
          "    WHEN rfisc_list_items.category = 1 THEN 0 "
          "    WHEN rfisc_list_items.category = 4 THEN 0 "
          "    WHEN rfisc_list_items.category = 2 THEN 1 "
          "    WHEN rfisc_list_items.category = 5 THEN 1 "
          "    ELSE NULL "
          "  END AS pr_cabin "
          "FROM rfisc_list_items "
          "  JOIN grp_service_lists "
          "    ON rfisc_list_items.list_id = grp_service_lists.list_id "
          "WHERE "
          "  grp_service_lists.grp_id = :bag_types_id "
          "  AND grp_service_lists.transfer_num = 0 "
          "  AND grp_service_lists.category IN (1, 2) "
          "  AND rfisc_list_items.category <> 0 "
          "  AND rfisc_list_items.visible <> 0 "
          "  AND rfisc_list_items.service_type = 'C' ";
  CreateVariablesFromParams({"bag_types_id"}, sqlParams, Qry);
  Qry.Execute();

  std::map<AirlineRfiscKey,GrpRfiscItems> grpRfiscItemsMap;
  for(; !Qry.Eof; Qry.Next()) {
      const AirlineRfiscKey key = std::make_pair(Qry.FieldAsString("airline"),
                                                 Qry.FieldAsString("rfisc"));
      const GrpRfiscItem item = {
          GrpId_t(Qry.FieldAsInteger("bag_types_id")),
          Qry.FieldAsString("rfisc"),
          StrUtils::ToLower(Qry.FieldAsString("name")),
          StrUtils::ToLower(Qry.FieldAsString("name_lat")),
          Qry.FieldAsString("airline"),
          Qry.FieldAsInteger("pr_cabin")
      };
      auto result = grpRfiscItemsMap.emplace(key, GrpRfiscItems{item});
      if (!result.second) {
          GrpRfiscItems& found_items = result.first->second;
          found_items.push_back(item);
      }
  }
  std::map<PriorityRfiscKey,GrpRfiscItems> grpRfsicPriorityMap;
  for (const auto& data: grpRfiscItemsMap) {
      const AirlineRfiscKey& key = data.first;
      const std::string& airline = key.first;
      const std::string& rfisc = key.second;
      DB::TQuery QryPriority(PgOra::getROSession("RFISC_BAG_PROPS"), STDLOG);
      QryPriority.SQLText =
              "SELECT COALESCE(priority, 100000) AS priority "
              "FROM rfisc_bag_props "
              "WHERE airline = :airline "
              "AND code = :rfisc ";
      QryPriority.CreateVariable("airline", otString, airline);
      QryPriority.CreateVariable("rfisc", otString, rfisc);
      QryPriority.Execute();
      const PriorityRfiscKey& priorityKey = std::make_pair(
                  QryPriority.Eof ? 100000 : QryPriority.FieldAsInteger("priority"),
                  rfisc);
      const GrpRfiscItems& items = data.second;
      for (const GrpRfiscItem& item: items) {
          auto result = grpRfsicPriorityMap.emplace(priorityKey, GrpRfiscItems{item});
          if (!result.second) {
              GrpRfiscItems& found_items = result.first->second;
              found_items.push_back(item);
          }
      }
  }
  for (const auto& data: grpRfsicPriorityMap) {
      const PriorityRfiscKey& key = data.first;
      const int priority = key.first;
      const GrpRfiscItems& items = data.second;
      for (const GrpRfiscItem& item: items) {
          rows.setFromString (item.rfisc)
              .setFromString (item.name)
              .setFromString (item.name_lat)
              .setFromBoolean(bool(item.pr_cabin))
              .setFromInteger(priority)
              .setFromInteger(item.grp_id.get())
              .addRow();
      }
  }
}

bool GrpRfisc::userDependence() const {
  return false;
}

void GrpRfisc::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"RFISC_LIST_ITEMS",
                                      "PAX_SERVICE_LISTS",
                                      "GRP_SERVICE_LISTS"}),
                 STDLOG);

  Qry.SQLText=
       "SELECT items.airline, items.service_type, items.rfisc, "
       "       LOWER(items.name) AS name, LOWER(items.name_lat) AS name_lat "
       "FROM rfisc_list_items items, pax_service_lists "
       "WHERE pax_service_lists.grp_id=:grp_id AND "
       "      items.list_id=pax_service_lists.list_id "
       "UNION "
       "SELECT items.airline, items.service_type, items.rfisc, items.name, items.name_lat "
       "FROM rfisc_list_items items, grp_service_lists "
       "WHERE grp_service_lists.grp_id=:grp_id AND "
       "      items.list_id=grp_service_lists.list_id "
       "ORDER BY airline, service_type, rfisc";

  CreateVariablesFromParams({"grp_id"}, sqlParams, Qry);
  Qry.Execute();

  int idxAirline=Qry.FieldIndex("airline");
  int idxServiceType=Qry.FieldIndex("service_type");
  int idxRfisc=Qry.FieldIndex("rfisc");
  int idxName=Qry.FieldIndex("name");
  int idxNameLat=Qry.FieldIndex("name_lat");
  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromString(Qry, idxAirline)
        .setFromString(ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromString(Qry, idxServiceType)
        .setFromString(Qry, idxRfisc)
        .setFromString(Qry, idxName)
        .setFromString(Qry, idxNameLat)
        .addRow();
  }
}

//FileTypes

bool FileTypes::userDependence() const {
  return false;
}
std::string FileTypes::selectSql() const {
  return "SELECT code, name FROM file_types WHERE code NOT IN ('BSM', 'HTTP_TYPEB') ORDER BY code";
}
std::list<std::string> FileTypes::dbSessionObjectNames() const {
  return {"FILE_TYPES"};
}

//Airlines

bool Airlines::userDependence() const {
  return false;
}
std::string Airlines::selectSql() const {
  return "SELECT id, code, code_lat, code_icao, code_icao_lat, name, name_lat, short_name, short_name_lat, aircode, city, tid, pr_del "
         "FROM airlines ORDER BY code";
}
std::string Airlines::refreshSql() const {
  return "SELECT id, code, code_lat, code_icao, code_icao_lat, name, name_lat, short_name, short_name_lat, aircode, city, tid, pr_del "
         "FROM airlines WHERE tid>:tid ORDER BY code";
}
std::list<std::string> Airlines::dbSessionObjectNames() const {
  return {"AIRLINES"};
}

//Airps

bool Airps::userDependence() const {
  return false;
}
std::string Airps::selectSql() const {
  return "SELECT id, code, code_lat, code_icao, code_icao_lat, city, city AS city_name, name, name_lat, tid, pr_del "
         "FROM airps ORDER BY code";
}
std::string Airps::refreshSql() const {
  return "SELECT id, code, code_lat, code_icao, code_icao_lat, city, city AS city_name, name, name_lat, tid, pr_del "
         "FROM airps WHERE tid>:tid ORDER BY code";
}
std::list<std::string> Airps::dbSessionObjectNames() const {
  return {"AIRPS"};
}

//TripTypes

bool TripTypes::userDependence() const {
  return false;
}
std::string TripTypes::selectSql() const {
  return "SELECT id, code, code_lat, name, name_lat, pr_reg, tid, pr_del FROM trip_types ORDER BY code";
}
std::string TripTypes::refreshSql() const {
  return "SELECT id, code, code_lat, name, name_lat, pr_reg, tid, pr_del FROM trip_types WHERE tid>:tid ORDER BY code";
}
std::list<std::string> TripTypes::dbSessionObjectNames() const {
  return {"TRIP_TYPES"};
}

//TripSuffixes

bool TripSuffixes::userDependence() const {
  return false;
}
std::string TripSuffixes::selectSql() const {
  return "SELECT code,code_lat FROM trip_suffixes ORDER BY code";
}
std::list<std::string> TripSuffixes::dbSessionObjectNames() const {
  return {"TRIP_SUFFIXES"};
}

//TermProfileRights

bool TermProfileRights::userDependence() const {
  return false;
}
std::string TermProfileRights::selectSql() const {
  return "select airline, airp, right_id, 1 user_type "
         "from airline_profiles ap, profile_rights pr "
         "where ap.profile_id = pr.profile_id AND "
         "      right_id NOT IN (191, 192)";
}
std::list<std::string> TermProfileRights::dbSessionObjectNames() const {
  return {"AIRLINE_PROFILES", "PROFILE_RIGHTS"};
}

//PrnFormsLayout

bool PrnFormsLayout::userDependence() const {
  return false;
}
std::string PrnFormsLayout::selectSql() const {
  return
   "select "
   "  id "
   "  ,op_type "
   "  ,max(CASE param_name WHEN 'btn_caption'          THEN param_value END) btn_caption "
   "  ,max(CASE param_name WHEN 'models_cache'         THEN param_value END) models_cache "
   "  ,max(CASE param_name WHEN 'types_cache'          THEN param_value END) types_cache "
   "  ,max(CASE param_name WHEN 'blanks_lbl'           THEN param_value END) blanks_lbl "
   "  ,max(CASE param_name WHEN 'forms_lbl'            THEN param_value END) forms_lbl "
   "  ,max(CASE param_name WHEN 'blank_list_cache'     THEN param_value END) blank_list_cache "
   "  ,max(CASE param_name WHEN 'airline_set_cache'    THEN param_value END) airline_set_cache "
   "  ,max(CASE param_name WHEN 'trip_set_cache'       THEN param_value END) trip_set_cache "
   "  ,max(CASE param_name WHEN 'airline_set_caption'  THEN param_value END) airline_set_caption "
   "  ,max(CASE param_name WHEN 'trip_set_caption'     THEN param_value END) trip_set_caption "
   "  ,max(CASE param_name WHEN 'msg_insert_blank_seg' THEN param_value END) msg_insert_blank_seg "
   "  ,max(CASE param_name WHEN 'msg_insert_blank'     THEN param_value END) msg_insert_blank "
   "  ,max(CASE param_name WHEN 'msg_wait_printing'    THEN param_value END) msg_wait_printing "
   "from prn_forms_layout "
   "group by id, op_type "
   "order by id";
}
std::list<std::string> PrnFormsLayout::dbSessionObjectNames() const {
  return {"PRN_FORMS_LAYOUT"};
}

//GraphStages

bool GraphStages::userDependence() const {
  return false;
}
std::string GraphStages::selectSql() const {
  return "SELECT stage_id,name,name_lat,time,pr_auto "
         "FROM graph_stages "
         "WHERE stage_id>0 AND stage_id<99 "
         "ORDER BY stage_id";
}
std::list<std::string> GraphStages::dbSessionObjectNames() const {
  return {"GRAPH_STAGES"};
}

//GraphStagesWithoutInactive

bool GraphStagesWithoutInactive::userDependence() const {
  return false;
}
std::string GraphStagesWithoutInactive::selectSql() const {
  return "SELECT stage_id,name,name_lat "
         "FROM graph_stages "
         "WHERE stage_id>0 "
         "ORDER BY stage_id";
}
std::list<std::string> GraphStagesWithoutInactive::dbSessionObjectNames() const {
  return {"GRAPH_STAGES"};
}

//StageSets

bool StageSets::userDependence() const {
  return true;
}
std::string StageSets::selectSql() const {
  return
   "SELECT stage_id, stage_id AS stage_name, airline, airp, pr_auto, id "
   "FROM stage_sets "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirportsOrNull) +
   "ORDER BY airline, airp, stage_id";
}
std::string StageSets::insertSql() const {
  return "INSERT INTO stage_sets(stage_id, airline, airp, pr_auto, id) "
         "VALUES(:stage_id, :airline, :airp, :pr_auto, :id)";
}
std::string StageSets::updateSql() const {
  return "UPDATE stage_sets "
         "SET stage_id=:stage_id, airline=:airline, airp=:airp, pr_auto=:pr_auto "
         "WHERE id=:OLD_id";
}
std::string StageSets::deleteSql() const {
  return "DELETE FROM stage_sets WHERE id=:OLD_id";
}
std::list<std::string> StageSets::dbSessionObjectNames() const {
  return {"STAGE_SETS"};
}

void StageSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkNotNullStageAccess("stage_id", "airline", "airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void StageSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("stage_sets").synchronize(getRowId("id", oldRow, newRow));
}

//GraphTimes

bool GraphTimes::userDependence() const {
  return true;
}
std::string GraphTimes::selectSql() const {
  return
   "SELECT id, stage_id, stage_id AS stage_name, airline, airp, craft, trip_type, time "
   "FROM graph_times "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirportsOrNull) +
   "ORDER BY airline, airp, craft, trip_type, stage_id";
}
std::string GraphTimes::insertSql() const {
  return "INSERT INTO graph_times(id, stage_id, airline, airp, craft, trip_type, time) "
         "VALUES(:id, :stage_id, :airline, :airp, :craft, :trip_type, :time)";
}
std::string GraphTimes::updateSql() const {
  return "UPDATE graph_times "
         "SET stage_id=:stage_id, airline=:airline, airp=:airp, craft=:craft, trip_type=:trip_type, time=:time "
         "WHERE id=:OLD_id";
}
std::string GraphTimes::deleteSql() const {
  return "DELETE FROM graph_times WHERE id=:OLD_id";
}
std::list<std::string> GraphTimes::dbSessionObjectNames() const {
  return {"GRAPH_TIMES"};
}

void GraphTimes::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkNotNullStageAccess("stage_id", "airline", "airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void GraphTimes::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("graph_times").synchronize(getRowId("id", oldRow, newRow));
}

//StageNames

bool StageNames::userDependence() const {
  return true;
}
std::string StageNames::selectSql() const {
  return
   "SELECT stage_id, stage_id AS stage_name, airp, name, name_lat, id "
   "FROM stage_names "
   "WHERE " + getSQLFilter("airp", AccessControl::PermittedAirports) +
   "ORDER BY airp, stage_id";
}
std::string StageNames::insertSql() const {
  return "INSERT INTO stage_names(stage_id, airp, name, name_lat, id) "
         "VALUES(:stage_id, :airp, :name, :name_lat, :id)";
}
std::string StageNames::updateSql() const {
  return "UPDATE stage_names "
         "SET stage_id=:stage_id, airp=:airp, name=:name, name_lat=:name_lat "
         "WHERE id=:OLD_id";
}
std::string StageNames::deleteSql() const {
  return "DELETE FROM stage_names WHERE id=:OLD_id";
}
std::list<std::string> StageNames::dbSessionObjectNames() const {
  return {"STAGE_NAMES"};
}

void StageNames::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void StageNames::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("stage_names").synchronize(getRowId("id", oldRow, newRow));
}

//SoppStageStatuses

bool SoppStageStatuses::userDependence() const {
  return false;
}

void SoppStageStatuses::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  auto cur=make_db_curs("SELECT stage_id, stage_type, status, status_lat "
                        "FROM stage_statuses "
                        "ORDER BY stage_type, stage_id",
                        PgOra::getROSession("STAGE_STATUSES"));

  int stageId, stageType;
  std::string status, statusLat;
  cur.def(stageId)
     .def(stageType)
     .def(status)
     .defNull(statusLat, "")
     .exec();

  bool isLangRu=TReqInfo::Instance()->desk.lang==AstraLocale::LANG_RU;

  while (!cur.fen())
  {
    rows.setFromInteger(stageId)
        .setFromInteger(stageType)
        .setFromString(isLangRu?status:(statusLat.empty()?status:statusLat))
        .addRow();
  }
}

//DeskPlusDeskGrpWritable

bool DeskPlusDeskGrpWritable::userDependence() const {
  return true;
}

bool DeskPlusDeskGrpWritable::checkViewAccess(DB::TQuery& Qry, int idxDeskGrpId, int idxDesk) const
{
  if (!deskGrpViewAccess) deskGrpViewAccess.emplace();
  if (!deskViewAccess) deskViewAccess.emplace();

  DeskGrpId_t deskGrpId(Qry.FieldAsInteger(idxDeskGrpId));
  if (!deskGrpViewAccess.value().check(deskGrpId)) return false;

  if (!Qry.FieldIsNULL(idxDesk))
  {
    DeskCode_t deskCode(Qry.FieldAsString(idxDesk));
    if (!deskViewAccess.value().check(deskCode)) return false;
  }

  return true;
}

void DeskPlusDeskGrpWritable::checkAccess(const std::string& fieldDeskGrpId,
                                          const std::string& fieldDesk,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkDeskAndDeskGrp(fieldDesk, fieldDeskGrpId, newRow);
  checkNullableDeskAccess(fieldDesk, oldRow, newRow);
  checkNotNullDeskGrpAccess(fieldDeskGrpId, oldRow, newRow);
}

//CryptSets

void CryptSets::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("CRYPT_SETS"), STDLOG);

  Qry.SQLText="SELECT id, desk_grp_id, desk, pr_crypt "
              "FROM crypt_sets ORDER BY desk_grp_id";

  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxDeskGrpId=Qry.FieldIndex("desk_grp_id");
  int idxDesk=Qry.FieldIndex("desk");
  int idxPrCrypt=Qry.FieldIndex("pr_crypt");
  for(; !Qry.Eof; Qry.Next())
  {
    if (!checkViewAccess(Qry, idxDeskGrpId, idxDesk)) continue;

    rows.setFromInteger(Qry, idxId)
        .setFromInteger(Qry, idxDeskGrpId)
        .setFromString (ElemIdToNameLong(etDeskGrp, Qry.FieldAsInteger(idxDeskGrpId)))
        .setFromString (Qry, idxDesk)
        .setFromBoolean(Qry, idxPrCrypt)
        .addRow();
  }

}

std::string CryptSets::insertSql() const {
  return "INSERT INTO crypt_sets(id, desk_grp_id, desk, pr_crypt) "
         "VALUES(:id, :desk_grp_id, :desk, :pr_crypt)";
}
std::string CryptSets::updateSql() const {
  return "UPDATE crypt_sets "
         "SET desk_grp_id=:desk_grp_id, desk=:desk, pr_crypt=:pr_crypt "
         "WHERE id=:OLD_id";
}
std::string CryptSets::deleteSql() const {
  return "DELETE FROM crypt_sets WHERE id=:OLD_id";
}
std::list<std::string> CryptSets::dbSessionObjectNames() const {
  return {"CRYPT_SETS"};
}

void CryptSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkAccess("desk_grp_id", "desk", oldRow, newRow);

  setRowId("id", status, newRow);
}

void CryptSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("crypt_sets").synchronize(getRowId("id", oldRow, newRow));
}

//CryptReqData

void CryptReqData::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("CRYPT_REQ_DATA"), STDLOG);

  Qry.SQLText="SELECT id, desk_grp_id, desk, country, state, city, organization, organizational_unit, "
              "       title, user_name, email, pr_denial "
              "FROM crypt_req_data ORDER BY desk_grp_id";

  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxDeskGrpId=Qry.FieldIndex("desk_grp_id");
  int idxDesk=Qry.FieldIndex("desk");
  int idxCountry=Qry.FieldIndex("country");
  int idxState=Qry.FieldIndex("state");
  int idxCity=Qry.FieldIndex("city");
  int idxOrganization=Qry.FieldIndex("organization");
  int idxOrganizationalUnit=Qry.FieldIndex("organizational_unit");
  int idxTitle=Qry.FieldIndex("title");
  int idxUserName=Qry.FieldIndex("user_name");
  int idxEmail=Qry.FieldIndex("email");
  int idxPrDenial=Qry.FieldIndex("pr_denial");
  for(; !Qry.Eof; Qry.Next())
  {
    if (!checkViewAccess(Qry, idxDeskGrpId, idxDesk)) continue;

    rows.setFromInteger(Qry, idxId)
        .setFromInteger(Qry, idxDeskGrpId)
        .setFromString (ElemIdToNameLong(etDeskGrp, Qry.FieldAsInteger(idxDeskGrpId)))
        .setFromString (Qry, idxDesk)
        .setFromString (Qry, idxCountry)
        .setFromString (Qry, idxState)
        .setFromString (Qry, idxCity)
        .setFromString (Qry, idxOrganization)
        .setFromString (Qry, idxOrganizationalUnit)
        .setFromString (Qry, idxTitle)
        .setFromString (Qry, idxUserName)
        .setFromString (Qry, idxEmail)
        .setFromBoolean(Qry, idxPrDenial)
        .addRow();
  }

}

std::string CryptReqData::insertSql() const {
  return "INSERT INTO crypt_req_data "
         " (id, desk_grp_id, desk, country, state, city, organization, organizational_unit, "
         "  title, user_name, email, key_algo, keyslength, pr_denial) "
         "VALUES "
         " (:id, :desk_grp_id, :desk, :country, :state, :city, :organization, :organizational_unit, "
         "  :title, :user_name, :email, NULL, NULL, :pr_denial)";
}
std::string CryptReqData::updateSql() const {
  return "UPDATE crypt_req_data "
         "SET desk_grp_id=:desk_grp_id, desk=:desk, country=:country, state=:state, city=:city, "
         "    organization=:organization, organizational_unit=:organizational_unit, title=:title, "
         "    user_name=:user_name, email=:email, key_algo=NULL, keyslength=NULL, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string CryptReqData::deleteSql() const {
  return "DELETE FROM crypt_req_data WHERE id=:OLD_id";
}
std::list<std::string> CryptReqData::dbSessionObjectNames() const {
  return {"CRYPT_REQ_DATA"};
}

void CryptReqData::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  checkAccess("desk_grp_id", "desk", oldRow, newRow);

  setRowId("id", status, newRow);
}

void CryptReqData::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("crypt_req_data").synchronize(getRowId("id", oldRow, newRow));
}

} //namespace CacheTable

