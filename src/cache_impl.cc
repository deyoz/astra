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
  return nullptr;
}

namespace CacheTable
{

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

}
