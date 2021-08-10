#include "cache_impl.h"
#include "PgOraConfig.h"
#include "astra_elems.h"
#include "astra_types.h"
#include "kassa.h"

#include <serverlib/algo.h>
#include <serverlib/str_utils.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace AstraLocale;

CacheTableCallbacks* SpawnCacheTableCallbacks(const std::string& cacheCode)
{
  if (cacheCode=="GRP_BAG_TYPES1")      return new CacheTable::GrpBagTypesOutdated;
  if (cacheCode=="GRP_BAG_TYPES2")      return new CacheTable::GrpBagTypesOutdated;
  if (cacheCode=="GRP_BAG_TYPES")       return new CacheTable::GrpBagTypes;
  if (cacheCode=="GRP_RFISC1")          return new CacheTable::GrpRfiscOutdated;
  if (cacheCode=="GRP_RFISC")           return new CacheTable::GrpRfisc;
  if (cacheCode=="FILE_TYPES")          return new CacheTable::FileTypes;
  if (cacheCode=="IN_FILE_ENCODING")    return new CacheTable::FileEncoding(false);
  if (cacheCode=="OUT_FILE_ENCODING")   return new CacheTable::FileEncoding(true);
  if (cacheCode=="IN_FILE_PARAM_SETS")  return new CacheTable::FileParamSets(false);
  if (cacheCode=="OUT_FILE_PARAM_SETS") return new CacheTable::FileParamSets(true);
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
  if (cacheCode=="SOPP_STATIONS")       return new CacheTable::SoppStations;
  if (cacheCode=="HOTEL_ACMD")          return new CacheTable::HotelAcmd;
  if (cacheCode=="BAG_NORMS")           return new CacheTable::BagNorms(CacheTable::BaggageWt::Type::AllAirlines);
  if (cacheCode=="AIRLINE_BAG_NORMS")   return new CacheTable::BagNorms(CacheTable::BaggageWt::Type::SingleAirline);
  if (cacheCode=="BASIC_BAG_NORMS")     return new CacheTable::BagNorms(CacheTable::BaggageWt::Type::Basic);
  if (cacheCode=="BAG_RATES")           return new CacheTable::BagRates(CacheTable::BaggageWt::Type::AllAirlines);
  if (cacheCode=="AIRLINE_BAG_RATES")   return new CacheTable::BagRates(CacheTable::BaggageWt::Type::SingleAirline);
  if (cacheCode=="BASIC_BAG_RATES")     return new CacheTable::BagRates(CacheTable::BaggageWt::Type::Basic);
  if (cacheCode=="VALUE_BAG_TAXES")          return new CacheTable::ValueBagTaxes(CacheTable::BaggageWt::Type::AllAirlines);
  if (cacheCode=="AIRLINE_VALUE_BAG_TAXES")  return new CacheTable::ValueBagTaxes(CacheTable::BaggageWt::Type::SingleAirline);
  if (cacheCode=="BASIC_VALUE_BAG_TAXES")    return new CacheTable::ValueBagTaxes(CacheTable::BaggageWt::Type::Basic);
  if (cacheCode=="EXCHANGE_RATES")           return new CacheTable::ExchangeRates(CacheTable::BaggageWt::Type::AllAirlines);
  if (cacheCode=="AIRLINE_EXCHANGE_RATES")   return new CacheTable::ExchangeRates(CacheTable::BaggageWt::Type::SingleAirline);
  if (cacheCode=="BASIC_EXCHANGE_RATES")     return new CacheTable::ExchangeRates(CacheTable::BaggageWt::Type::Basic);
  if (cacheCode=="TRIP_BAG_NORMS")      return new CacheTable::TripBagNorms(true);
  if (cacheCode=="TRIP_BAG_NORMS2")     return new CacheTable::TripBagNorms(false);
  if (cacheCode=="TRIP_BAG_RATES")      return new CacheTable::TripBagRates(true);
  if (cacheCode=="TRIP_BAG_RATES2")     return new CacheTable::TripBagRates(false);
  if (cacheCode=="TRIP_VALUE_BAG_TAXES")     return new CacheTable::TripValueBagTaxes;
  if (cacheCode=="TRIP_EXCHANGE_RATES")      return new CacheTable::TripExchangeRates;
  if (cacheCode=="AIRLINE_PROFILES")    return new CacheTable::AirlineProfiles;
  if (cacheCode=="EXTRA_ROLE_ACCESS")   return new CacheTable::ExtraRoleAccess;
  if (cacheCode=="EXTRA_USER_ACCESS")   return new CacheTable::ExtraUserAccess;
  if (cacheCode=="VALIDATOR_TYPES")     return new CacheTable::ValidatorTypes;
  if (cacheCode=="FORM_TYPES")          return new CacheTable::FormTypes;
  if (cacheCode=="FORM_PACKS")          return new CacheTable::FormPacks;
  if (cacheCode=="SALE_POINTS")         return new CacheTable::SalePoints;
  if (cacheCode=="SALE_DESKS")          return new CacheTable::SaleDesks;
  if (cacheCode=="OPERATORS")           return new CacheTable::Operators;
  if (cacheCode=="PAY_CLIENTS")         return new CacheTable::PayClients;
  if (cacheCode=="POS_TERM_VENDORS")    return new CacheTable::PosTermVendors;
  if (cacheCode=="POS_TERM_SETS")       return new CacheTable::PosTermSets;
  if (cacheCode=="PLACE_CALC")          return new CacheTable::PlaceCalc;
  if (cacheCode=="COMP_SUBCLS_SETS")    return new CacheTable::CompSubclsSets;
#ifndef ENABLE_ORACLE
  if (cacheCode=="AIRLINES")            return new CacheTable::Airlines;
  if (cacheCode=="AIRPS")               return new CacheTable::Airps;
  if (cacheCode=="CITIES")              return new CacheTable::Cities;
  if (cacheCode=="TRIP_TYPES")          return new CacheTable::TripTypes;
#endif //ENABLE_ORACLE
  if (cacheCode=="CRS_SET")          return new CacheTable::CrsSet;
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
    rows.setFromString (Qry, 1) //��� ����� ������� ���室� ����� ��� �� 横�� ���᫨�� idxCode=Qry.FieldIndex("code") � �ਬ����� setFromString (Qry, idxCode)
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

bool DeskWritable::userDependence() const {
  return true;
}

bool DeskWritable::checkViewAccess(DB::TQuery& Qry, int idxDesk) const
{
  if (!deskViewAccess) deskViewAccess.emplace();
  DeskCode_t deskCode(Qry.FieldAsString(idxDesk));
  if (!deskViewAccess.value().check(deskCode)) return false;

  return true;
}

//FileEncoding

void FileEncoding::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("FILE_ENCODING"), STDLOG);

  Qry.SQLText="SELECT id, type, point_addr, encoding "
              "FROM file_encoding "
              "WHERE own_point_addr=:own_point_addr AND pr_send=:pr_send AND "
              "      type NOT IN ('BSM', 'HTTP_TYPEB') "
              "ORDER BY type, point_addr";
  Qry.CreateVariable("own_point_addr", otString, OWN_POINT_ADDR());
  Qry.CreateVariable("pr_send", otInteger, (int)out_);

  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxType=Qry.FieldIndex("type");
  int idxPointAddr=Qry.FieldIndex("point_addr");
  int idxEncoding=Qry.FieldIndex("encoding");
  for(; !Qry.Eof; Qry.Next())
  {
    if (!checkViewAccess(Qry, idxPointAddr)) continue;

    rows.setFromInteger(Qry, idxId)
        .setFromString (Qry, idxType)
        .setFromString (Qry, idxPointAddr)
        .setFromString (Qry, idxEncoding)
        .addRow();
  }

}

std::string FileEncoding::insertSql() const {
  return "INSERT INTO file_encoding(id, type, point_addr, own_point_addr, encoding, pr_send) "
         "VALUES(:id, :type, :point_addr, :SYS_point_addr, :encoding, " + IntToString(out_) + ")";
}
std::string FileEncoding::updateSql() const {
  return "UPDATE file_encoding "
         "SET type=:type, point_addr=:point_addr, encoding=:encoding "
         "WHERE id=:OLD_id";
}
std::string FileEncoding::deleteSql() const {
  return "DELETE FROM file_encoding WHERE id=:OLD_id";
}
std::list<std::string> FileEncoding::dbSessionObjectNames() const {
  return {"FILE_ENCODING"};
}

void FileEncoding::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  checkDeskAccess("point_addr", oldRow, newRow);

  setRowId("id", status, newRow);
}

void FileEncoding::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("file_encoding").synchronize(getRowId("id", oldRow, newRow));
}

//FileParamSets

void FileParamSets::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("FILE_PARAM_SETS"), STDLOG);

  Qry.SQLText="SELECT id, type, point_addr, param_name, param_value, airline, flt_no, airp "
              "FROM file_param_sets "
              "WHERE own_point_addr=:own_point_addr AND pr_send=:pr_send AND "
              "      type NOT IN ('BSM', 'HTTP_TYPEB') AND "
              + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " AND "
              + getSQLFilter("airp",    AccessControl::PermittedAirportsOrNull) +
              "ORDER BY type, point_addr, airline, airp, flt_no";
  Qry.CreateVariable("own_point_addr", otString, OWN_POINT_ADDR());
  Qry.CreateVariable("pr_send", otInteger, (int)out_);

  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxType=Qry.FieldIndex("type");
  int idxPointAddr=Qry.FieldIndex("point_addr");
  int idxAirline=Qry.FieldIndex("airline");
  int idxFltNo=Qry.FieldIndex("flt_no");
  int idxAirp=Qry.FieldIndex("airp");
  int idxParamName=Qry.FieldIndex("param_name");
  int idxParamValue=Qry.FieldIndex("param_value");
  for(; !Qry.Eof; Qry.Next())
  {
    if (!checkViewAccess(Qry, idxPointAddr)) continue;

    rows.setFromInteger(Qry, idxId)
        .setFromString (Qry, idxType)
        .setFromString (Qry, idxPointAddr)
        .setFromString (Qry, idxAirline)
        .setFromString (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromInteger(Qry, idxFltNo)
        .setFromString (Qry, idxAirp)
        .setFromString (ElemIdToCodeNative(etAirp, Qry.FieldAsString(idxAirp)))
        .setFromString (Qry, idxParamName)
        .setFromString (Qry, idxParamValue)
        .addRow();
  }

}

std::string FileParamSets::insertSql() const {
  return "INSERT INTO file_param_sets(id, type, point_addr, own_point_addr, param_name, airline, airp, flt_no, param_value, pr_send) "
         "VALUES(:id, :type, :point_addr, :SYS_point_addr, :param_name, :airline, :airp, :flt_no, :param_value, " + IntToString(out_) + ")";
}
std::string FileParamSets::updateSql() const {
  return "UPDATE file_param_sets "
         "SET type=:type, point_addr=:point_addr, param_name=:param_name, "
         "    airline=:airline, flt_no=:flt_no, airp=:airp, param_value=:param_value "
         "WHERE id=:OLD_id";
}
std::string FileParamSets::deleteSql() const {
  return "DELETE FROM file_param_sets WHERE id=:OLD_id";
}
std::list<std::string> FileParamSets::dbSessionObjectNames() const {
  return {"FILE_PARAM_SETS"};
}

void FileParamSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             std::optional<CacheTable::Row>& newRow) const
{
  checkDeskAccess("point_addr", oldRow, newRow);
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void FileParamSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("file_param_sets").synchronize(getRowId("id", oldRow, newRow));
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

//Cities

bool Cities::userDependence() const {
  return false;
}
std::string Cities::selectSql() const {
  return "SELECT c.id, c.code, c.code_lat, c.country, c.name, c.name_lat, c.tz_region, c.pr_del, c.tid, d.gmt_offset "
         "FROM cities c LEFT OUTER JOIN date_time_zonespec d "
         "ON c.tz_region=d.id AND d.pr_del=0 "
         "ORDER BY c.code";
}
std::string Cities::refreshSql() const {
  return "SELECT c.id, c.code, c.code_lat, c.country, c.name, c.name_lat, c.tz_region, c.pr_del, c.tid, d.gmt_offset "
         "FROM cities c LEFT OUTER JOIN date_time_zonespec d "
         "ON c.tz_region=d.id AND d.pr_del=0 "
         "WHERE c.tid>:tid "
         "ORDER BY c.code";
}
std::list<std::string> Cities::dbSessionObjectNames() const {
  return {"CITIES", "DATE_TIME_ZONESPEC"};
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
  checkDeskAccess(fieldDesk, oldRow, newRow);
  checkNotNullDeskGrpAccess(fieldDeskGrpId, oldRow, newRow);
}

//CryptSets

void CryptSets::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("CRYPT_SETS"), STDLOG);

  Qry.SQLText="SELECT id, desk_grp_id, desk, pr_crypt "
              "FROM crypt_sets ORDER BY desk_grp_id, desk";

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
              "FROM crypt_req_data ORDER BY desk_grp_id, desk";

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

//SoppStations

bool SoppStations::userDependence() const {
  return false;
}

void SoppStations::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  bool isLangRu=TReqInfo::Instance()->desk.lang==AstraLocale::LANG_RU;

  rows.setFromString(isLangRu?"���":"ALL")
      .setFromString("")
      .addRow();

  TElemFmt fmt;
    std::string airp=ElemToElemId(etAirp, getParamValue("airp", sqlParams), fmt);
  if (fmt==efmtUnknown) return;

  DB::TQuery Qry(PgOra::getROSession("STATIONS"), STDLOG);
  Qry.SQLText="SELECT name, work_mode FROM stations WHERE airp=:airp ORDER BY work_mode DESC, name ASC";
  Qry.CreateVariable("airp", otString, airp);
  Qry.Execute();

  if (Qry.Eof) return;

  int idxName=Qry.FieldIndex("name");
  int idxWorkMode=Qry.FieldIndex("work_mode");
  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromString(Qry, idxName)
        .setFromString(Qry, idxWorkMode)
        .addRow();
  }
}

//HotelAcmd

bool HotelAcmd::userDependence() const {
  return true;
}
std::string HotelAcmd::selectSql() const {
  return
   "SELECT id, airline, airp, hotel_name, single_amount, double_amount "
   "FROM hotel_acmd "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirports) +
   "ORDER BY airline, airp, hotel_name";
}
std::string HotelAcmd::insertSql() const {
  return "INSERT INTO hotel_acmd(id, airline, airp, hotel_name, single_amount, double_amount) "
         "VALUES(:id, :airline, :airp, :hotel_name, :single_amount, :double_amount)";
}
std::string HotelAcmd::updateSql() const {
  return "UPDATE hotel_acmd "
         "SET single_amount=:single_amount, double_amount=:double_amount "
         "WHERE id=:OLD_id";
}
std::string HotelAcmd::deleteSql() const {
  return "DELETE FROM hotel_acmd WHERE id=:OLD_id";
}
std::list<std::string> HotelAcmd::dbSessionObjectNames() const {
  return {"HOTEL_ACMD"};
}

void HotelAcmd::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void HotelAcmd::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("hotel_acmd").synchronize(getRowId("id", oldRow, newRow));
}

//CrsSet

bool CrsSet::userDependence() const {
  return true;
}

std::string CrsSet::selectSql() const {
  return
   "SELECT id,airline,flt_no,airp_dep,crs,priority,pr_numeric_pnl "
   "FROM crs_set "
   "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp_dep", AccessControl::PermittedAirportsOrNull) +
   "ORDER BY airline,airp_dep,flt_no,crs";
}

std::string CrsSet::insertSql() const {
  return "INSERT INTO crs_set(id,airline,flt_no,airp_dep,crs,priority,pr_numeric_pnl) "
         "VALUES(:id,:airline,:flt_no,:airp_dep,:crs,:priority,:pr_numeric_pnl)";
}

std::string CrsSet::updateSql() const {
  return "UPDATE crs_set SET "
         "airline=:airline, "
         "flt_no=:flt_no, "
         "airp_dep=:airp_dep,"
         "crs=:crs, "
         "priority=:priority, "
         "pr_numeric_pnl=:pr_numeric_pnl "
         "WHERE id=:OLD_id";
}

std::string CrsSet::deleteSql() const {
  return "DELETE FROM crs_set "
         "WHERE id=:OLD_id";
}

std::list<std::string> CrsSet::dbSessionObjectNames() const {
  return {"CRS_SET"};
}

void CrsSet::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                      const std::optional<CacheTable::Row>& oldRow,
                                      std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  setRowId("id", status, newRow);
}

void CrsSet::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                     const std::optional<CacheTable::Row>& oldRow,
                                     const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("crs_set").synchronize(getRowId("id", oldRow, newRow));
}

std::string lastDateSelectSQL(const std::string& objectName)
{
  return PgOra::supportsPg(objectName)?"last_date - interval '1 second' AS last_date":
                                       "last_date-1/86400 AS last_date";
}

//BaggageWt

std::string BaggageWt::airlineSqlFilter() const
{
  switch(type_)
  {
    case Type::AllAirlines:   return "airline IS NOT NULL";
    case Type::SingleAirline: return "airline=:airline";
    case Type::Basic:         return "airline IS NULL";
  }

  return "";
}

int BaggageWt::getCurrentTid() const
{
  if (!currentTid) currentTid=PgOra::getSeqNextVal("tid__seq");

  return currentTid.value();
}

bool BaggageWt::userDependence() const {
  return true;
}
bool BaggageWt::insertImplemented() const {
  return true;
}
bool BaggageWt::updateImplemented() const {
  return true;
}
bool BaggageWt::deleteImplemented() const {
  return true;
}
std::string BaggageWt::selectSql() const {
  return getSelectOrRefreshSql(false);
}
std::string BaggageWt::refreshSql() const {
  return getSelectOrRefreshSql(true);
}

void BaggageWt::beforeSelectOrRefresh(const TCacheQueryType queryType,
                                      const TParams& sqlParams,
                                      DB::TQuery& Qry) const
{
  Qry.CreateVariable("now_local", otDate, BASIC::date_time::Now());
}

void BaggageWt::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess(type_==Type::Basic?"":"airline", oldRow, newRow);
  checkCityAccess("city_dep", oldRow, newRow);
}

//BagNorms

std::string BagNorms::getSelectOrRefreshSql(const bool isRefreshSql) const
{
    std::ostringstream sql;
  sql << "SELECT id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, class, flt_no, craft, trip_type, "
         "       first_date, " << lastDateSelectSQL("BAG_NORMS") << ", "
         "bag_type, amount, weight, per_unit, norm_type, extra, pr_del, tid "
         "FROM bag_norms "
         "WHERE direct_action=0"
      << " AND " << airlineSqlFilter()
      << " AND (last_date IS NULL OR last_date >= :now_local)"
      << (isRefreshSql?" AND tid>:tid":" AND pr_del=0")
      << " AND " << getSQLFilter("city_dep", AccessControl::PermittedCitiesOrNull);

  if (type_!=Type::Basic)
    sql << " AND " << getSQLFilter("airline", AccessControl::PermittedAirlines);

  return sql.str();
}

std::list<std::string> BagNorms::dbSessionObjectNames() const {
  return {"BAG_NORMS"};
}

void BagNorms::onApplyingRowChanges(const TCacheUpdateStatus status,
                                    const std::optional<CacheTable::Row>& oldRow,
                                    const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();
    Kassa::BagNorm::add(
                row.getAsInteger_ThrowOnEmpty("id"),
                type_ != Type::Basic ? row.getAsString("airline") : "",
                row.getAsBoolean("pr_trfer"),
                row.getAsString("city_dep"),
                row.getAsString("city_arv"),
                row.getAsString("pax_cat"),
                type_ != Type::Basic ? row.getAsString("subclass") : "",
                row.getAsString("class"),
                type_ != Type::Basic ? row.getAsInteger("flt_no", ASTRA::NoExists) : ASTRA::NoExists,
                row.getAsString("craft"),
                row.getAsString("trip_type"),
                row.getAsDateTime_ThrowOnEmpty("first_date"),
                row.getAsDateTime("last_date", ASTRA::NoExists),
                row.getAsInteger("bag_type", ASTRA::NoExists),
                row.getAsInteger("amount", ASTRA::NoExists),
                row.getAsInteger("weight", ASTRA::NoExists),
                row.getAsInteger("per_unit", ASTRA::NoExists),
                row.getAsString("norm_type"),
                row.getAsString("extra"),
                getCurrentTid());
  }

  if (status==usModified)
  {
    Kassa::BagNorm::modifyById(
                oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                newRow.value().getAsDateTime("last_date", ASTRA::NoExists));
  }

  if (status==usDeleted)
  {
    Kassa::BagNorm::deleteById(oldRow.value().getAsInteger_ThrowOnEmpty("id"));
  }

}

//BagRates

std::string BagRates::getSelectOrRefreshSql(const bool isRefreshSql) const
{
    std::ostringstream sql;
  sql << "SELECT id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, class, flt_no, craft, trip_type, "
         "       bag_type, first_date, " << lastDateSelectSQL("BAG_RATES") << ", rate, rate_cur, min_weight, extra, pr_del, tid "
         "FROM bag_rates "
         "WHERE " << airlineSqlFilter()
      << " AND (last_date IS NULL OR last_date >= :now_local)"
      << (isRefreshSql?" AND tid>:tid":" AND pr_del=0")
      << " AND " << getSQLFilter("city_dep", AccessControl::PermittedCitiesOrNull);

  if (type_!=Type::Basic)
    sql << " AND " << getSQLFilter("airline", AccessControl::PermittedAirlines);

  return sql.str();
}

std::list<std::string> BagRates::dbSessionObjectNames() const {
  return {"BAG_RATES"};
}

void BagRates::onApplyingRowChanges(const TCacheUpdateStatus status,
                                    const std::optional<CacheTable::Row>& oldRow,
                                    const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();

    Kassa::BagRate::add(
                row.getAsInteger_ThrowOnEmpty("id"),
                type_ != Type::Basic ? row.getAsString("airline") : "",
                row.getAsBoolean("pr_trfer"),
                row.getAsString("city_dep"),
                row.getAsString("city_arv"),
                row.getAsString("pax_cat"),
                type_ != Type::Basic ? row.getAsString("subclass") : "",
                row.getAsString("class"),
                type_ != Type::Basic ? row.getAsInteger("flt_no", ASTRA::NoExists) : ASTRA::NoExists,
                row.getAsString("craft"),
                row.getAsString("trip_type"),
                row.getAsDateTime_ThrowOnEmpty("first_date"),
                row.getAsDateTime("last_date", ASTRA::NoExists),
                row.getAsInteger("bag_type", ASTRA::NoExists),
                row.getAsInteger_ThrowOnEmpty("rate"),
                row.getAsString("rate_cur"),
                row.getAsInteger("min_weight", ASTRA::NoExists),
                row.getAsString("extra"),
                getCurrentTid());
  }

  if (status==usModified)
  {
    Kassa::BagRate::modifyById(
                oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                newRow.value().getAsDateTime("last_date", ASTRA::NoExists));
  }

  if (status==usDeleted)
  {
    Kassa::BagRate::deleteById(oldRow.value().getAsInteger_ThrowOnEmpty("id"));
  }
}

//ValueBagTaxes

std::string ValueBagTaxes::getSelectOrRefreshSql(const bool isRefreshSql) const
{
    std::ostringstream sql;
  sql << "SELECT id, airline, pr_trfer, city_dep, city_arv, "
         "       first_date, " << lastDateSelectSQL("VALUE_BAG_TAXES") << ", tax, min_value, min_value_cur, extra, pr_del, tid "
         "FROM value_bag_taxes "
         "WHERE " << airlineSqlFilter()
      << " AND (last_date IS NULL OR last_date >= :now_local)"
      << (isRefreshSql?" AND tid>:tid":" AND pr_del=0")
      << " AND " << getSQLFilter("city_dep", AccessControl::PermittedCitiesOrNull);

  if (type_!=Type::Basic)
    sql << " AND " << getSQLFilter("airline", AccessControl::PermittedAirlines);

  return sql.str();
}

std::list<std::string> ValueBagTaxes::dbSessionObjectNames() const {
  return {"VALUE_BAG_TAXES"};
}

void ValueBagTaxes::onApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();

    Kassa::ValueBagTax::add(
                row.getAsInteger_ThrowOnEmpty("id"),
                type_ != Type::Basic ? row.getAsString("airline") : "",
                row.getAsBoolean("pr_trfer"),
                row.getAsString("city_dep"),
                row.getAsString("city_arv"),
                row.getAsDateTime_ThrowOnEmpty("first_date"),
                row.getAsDateTime("last_date", ASTRA::NoExists),
                row.getAsInteger_ThrowOnEmpty("tax"),
                row.getAsInteger("min_value", ASTRA::NoExists),
                row.getAsString("min_value_cur"),
                row.getAsString("extra"),
                getCurrentTid());
  }

  if (status==usModified)
  {
    Kassa::ValueBagTax::modifyById(
                oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                newRow.value().getAsDateTime("last_date", ASTRA::NoExists));
  }

  if (status==usDeleted)
  {
    Kassa::ValueBagTax::deleteById(oldRow.value().getAsInteger_ThrowOnEmpty("id"));
  }
}

//ExchangeRates

std::string ExchangeRates::getSelectOrRefreshSql(const bool isRefreshSql) const
{
    std::ostringstream sql;
  sql << "SELECT id, airline, rate1, cur1, rate2, cur2, "
         "       first_date, " << lastDateSelectSQL("EXCHANGE_RATES") << ", extra, pr_del, tid "
         "FROM exchange_rates "
         "WHERE " << airlineSqlFilter()
      << " AND (last_date IS NULL OR last_date >= :now_local)"
      << (isRefreshSql?" AND tid>:tid":" AND pr_del=0");

  if (type_!=Type::Basic)
    sql << " AND " << getSQLFilter("airline", AccessControl::PermittedAirlines);

  return sql.str();
}

std::list<std::string> ExchangeRates::dbSessionObjectNames() const {
  return {"EXCHANGE_RATES"};
}

void ExchangeRates::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess(type_==Type::Basic?"":"airline", oldRow, newRow);
}

void ExchangeRates::onApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();

    Kassa::ExchangeRate::add(
                row.getAsInteger_ThrowOnEmpty("id"),
                type_ != Type::Basic ? row.getAsString("airline") : "",
                row.getAsInteger_ThrowOnEmpty("rate1"),
                row.getAsString("cur1"),
                row.getAsInteger_ThrowOnEmpty("rate2"),
                row.getAsString("cur2"),
                row.getAsDateTime_ThrowOnEmpty("first_date"),
                row.getAsDateTime("last_date", ASTRA::NoExists),
                row.getAsString("extra"),
                getCurrentTid());
  }

  if (status==usModified)
  {
    Kassa::ExchangeRate::modifyById(
                oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                newRow.value().getAsDateTime("last_date", ASTRA::NoExists));
  }

  if (status==usDeleted)
  {
    Kassa::ExchangeRate::deleteById(oldRow.value().getAsInteger_ThrowOnEmpty("id"));
  }
}

//TripBaggageWt

bool TripBaggageWt::userDependence() const {
  return false;
}

std::set<CityCode_t> TripBaggageWt::getArrivalCities(const TAdvTripInfo& fltInfo)
{
  std::set<CityCode_t> result;

  TTripRoute route;
  route.GetRouteAfter(fltInfo, trtNotCurrent, trtNotCancelled);
  for(const TTripRouteItem& item : route)
    result.insert(getCityByAirp(AirportCode_t(item.airp)));

  return result;
}

//TripBagNorms

bool TripBagNorms::prepareSelectQuery(const PointId_t& pointId,
                                      const bool useMarkFlt,
                                      const AirlineCode_t& airlineMark,
                                      const FlightNumber_t& fltNumberMark,
                                      DB::TQuery &Qry) const
{
  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(pointId.get())) return false;

  AirlineCode_t airline( useMarkFlt ? airlineMark.get() : fltInfo.airline );
  FlightNumber_t fltNumber( useMarkFlt ? fltNumberMark.get() : fltInfo.flt_no );
  CityCode_t cityDep(getCityByAirp(AirportCode_t(fltInfo.airp)));

  Qry.SQLText=
    "SELECT id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, class, flt_no, craft, trip_type, "
    "       first_date, " + lastDateSelectSQL("BAG_NORMS") + ", "
    "       bag_type, amount, weight, per_unit, norm_type, extra, "
    "       CASE WHEN pr_trfer=0 OR pr_trfer IS NULL THEN 0 ELSE 1 END AS pr_trfer_order "
    "FROM bag_norms "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (city_dep IS NULL OR city_dep=:city_dep) AND "
    "      (city_arv IS NULL OR "
    "       pr_trfer IS NULL OR pr_trfer<>0 OR "
    "       city_arv IN " + getSQLEnum(getArrivalCities(fltInfo)) + ") AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (craft IS NULL OR craft=:craft) AND "
    "      (trip_type IS NULL OR trip_type=:trip_type) AND "
    "      first_date<=:est_scd_out AND (last_date IS NULL OR last_date>:est_scd_out) AND "
    "      pr_del=0 AND direct_action=0 AND "
    "      (bag_type IS NULL OR bag_type<>99) "
    "ORDER BY airline, pr_trfer_order, id";

  Qry.CreateVariable("airline", otString, airline.get());
  Qry.CreateVariable("city_dep", otString, cityDep.get());
  Qry.CreateVariable("flt_no", otInteger, fltNumber.get());
  Qry.CreateVariable("craft", otString, fltInfo.craft);
  Qry.CreateVariable("trip_type", otString, fltInfo.trip_type);
  Qry.CreateVariable("est_scd_out", otDate, fltInfo.est_scd_out());

  return true;
}

void TripBagNorms::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  PointId_t pointId(notEmptyParamAsInteger("point_id", sqlParams));
  bool useMarkFlt=notEmptyParamAsBoolean("use_mark_flt", sqlParams);
  AirlineCode_t airlineMark(notEmptyParamAsString("airline_mark", sqlParams));
  FlightNumber_t fltNumberMark(notEmptyParamAsInteger("flt_no_mark", sqlParams));

  DB::TQuery Qry(PgOra::getROSession("BAG_NORMS"), STDLOG);

  if (!prepareSelectQuery(pointId, useMarkFlt, airlineMark, fltNumberMark, Qry)) return;

  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxAirline=Qry.FieldIndex("airline");
  int idxPrTrfer=Qry.FieldIndex("pr_trfer");
  int idxCityDep=Qry.FieldIndex("city_dep");
  int idxCityArv=Qry.FieldIndex("city_arv");
  int idxPaxCat=Qry.FieldIndex("pax_cat");
  int idxSubclass=Qry.FieldIndex("subclass");
  int idxClass=Qry.FieldIndex("class");
  int idxFltNo=Qry.FieldIndex("flt_no");
  int idxCraft=Qry.FieldIndex("craft");
  int idxTripType=Qry.FieldIndex("trip_type");
  int idxFirstDate=Qry.FieldIndex("first_date");
  int idxLastDate=Qry.FieldIndex("last_date");
  int idxBagType=Qry.FieldIndex("bag_type");
  int idxAmount=Qry.FieldIndex("amount");
  int idxWeight=Qry.FieldIndex("weight");
  int idxPerUnit=Qry.FieldIndex("per_unit");
  int idxNormType=Qry.FieldIndex("norm_type");
  int idxExtra=Qry.FieldIndex("extra");

  std::string trferBagAirline=Qry.FieldAsString(idxAirline);

  for(; !Qry.Eof; Qry.Next())
  {
    std::ostringstream bagType;
    if (!Qry.FieldIsNULL(idxBagType))
      bagType << std::setw(outdated_?0:2) << std::setfill('0') << Qry.FieldAsInteger(idxBagType);

    rows.setFromInteger (Qry, idxId)
        .setFromString  (Qry, idxAirline)
        .setFromString  (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromBoolean (Qry, idxPrTrfer)
        .setFromString  (Qry, idxCityDep)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityDep)))
        .setFromString  (Qry, idxCityArv)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityArv)))
        .setFromString  (Qry, idxPaxCat)
        .setFromString  (Qry, idxSubclass)
        .setFromString  (ElemIdToCodeNative(etSubcls, Qry.FieldAsString(idxSubclass)))
        .setFromString  (Qry, idxClass)
        .setFromString  (ElemIdToCodeNative(etClass, Qry.FieldAsString(idxClass)))
        .setFromInteger (Qry, idxFltNo)
        .setFromString  (Qry, idxCraft)
        .setFromString  (ElemIdToCodeNative(etCraft, Qry.FieldAsString(idxCraft)))
        .setFromString  (Qry, idxTripType)
        .setFromString  (ElemIdToCodeNative(etTripType, Qry.FieldAsString(idxTripType)))
        .setFromString  (bagType.str())
        .setFromString  (Qry, idxNormType)
        .setFromString  (ElemIdToCodeNative(etBagNormType, Qry.FieldAsString(idxNormType)))
        .setFromInteger (Qry, idxAmount)
        .setFromInteger (Qry, idxWeight)
        .setFromBoolean (Qry, idxPerUnit)
        .setFromDateTime(Qry, idxFirstDate)
        .setFromDateTime(Qry, idxLastDate)
        .setFromString  (Qry, idxExtra)
        .setFromInteger (pointId.get())
        .setFromBoolean (useMarkFlt)
        .setFromString  (airlineMark.get())
        .setFromInteger (fltNumberMark.get())
        .addRow();
  }

  if (outdated_)
  {
    rows.setField("id", IntToString(1000000000))
        .setField("airline", trferBagAirline)
        .setField("airline_view", ElemIdToCodeNative(etAirline, trferBagAirline))
        .setField("first_date", "01.01.2000 00:00:00")
        .setField("bag_type", "99")
        .setField("norm_type", "��")
        .setField("norm_type_view", ElemIdToCodeNative(etBagNormType, "��"))
        .setField("point_id", IntToString(pointId.get()))
        .setField("use_mark_flt", IntToString((int)useMarkFlt))
        .setField("airline_mark", airlineMark.get())
        .setField("flt_no_mark", IntToString(fltNumberMark.get()))
        .addRow();
  }
}

//TripBagRates

void TripBagRates::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  PointId_t pointId(notEmptyParamAsInteger("point_id", sqlParams));

  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(pointId.get())) return;

  bool useMarkFlt=notEmptyParamAsBoolean("use_mark_flt", sqlParams);
  AirlineCode_t airline( useMarkFlt ? notEmptyParamAsString("airline_mark", sqlParams) : fltInfo.airline );
  FlightNumber_t fltNumber( useMarkFlt ? notEmptyParamAsInteger("flt_no_mark", sqlParams) : fltInfo.flt_no );
  CityCode_t cityDep(getCityByAirp(AirportCode_t(fltInfo.airp)));

  DB::TQuery Qry(PgOra::getROSession("BAG_RATES"), STDLOG);

  Qry.SQLText=
    "SELECT id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, class, flt_no, craft, trip_type, "
    "       bag_type, first_date, " + lastDateSelectSQL("BAG_RATES") + ", "
    "       rate, rate_cur, min_weight, extra, "
    "       CASE WHEN pr_trfer=0 OR pr_trfer IS NULL THEN 0 ELSE 1 END AS pr_trfer_order "
    "FROM bag_rates "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (city_dep IS NULL OR city_dep=:city_dep) AND "
    "      (city_arv IS NULL OR "
    "       pr_trfer IS NULL OR pr_trfer<>0 OR "
    "       city_arv IN " + getSQLEnum(getArrivalCities(fltInfo)) + ") AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (craft IS NULL OR craft=:craft) AND "
    "      (trip_type IS NULL OR trip_type=:trip_type) AND "
    "      first_date<=:est_scd_out AND (last_date IS NULL OR last_date>:est_scd_out) AND "
    "      pr_del=0 "
    "ORDER BY airline, pr_trfer_order, id";

  Qry.CreateVariable("airline", otString, airline.get());
  Qry.CreateVariable("city_dep", otString, cityDep.get());
  Qry.CreateVariable("flt_no", otInteger, fltNumber.get());
  Qry.CreateVariable("craft", otString, fltInfo.craft);
  Qry.CreateVariable("trip_type", otString, fltInfo.trip_type);
  Qry.CreateVariable("est_scd_out", otDate, fltInfo.est_scd_out());
  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxAirline=Qry.FieldIndex("airline");
  int idxPrTrfer=Qry.FieldIndex("pr_trfer");
  int idxCityDep=Qry.FieldIndex("city_dep");
  int idxCityArv=Qry.FieldIndex("city_arv");
  int idxPaxCat=Qry.FieldIndex("pax_cat");
  int idxSubclass=Qry.FieldIndex("subclass");
  int idxClass=Qry.FieldIndex("class");
  int idxFltNo=Qry.FieldIndex("flt_no");
  int idxCraft=Qry.FieldIndex("craft");
  int idxTripType=Qry.FieldIndex("trip_type");
  int idxBagType=Qry.FieldIndex("bag_type");
  int idxFirstDate=Qry.FieldIndex("first_date");
  int idxLastDate=Qry.FieldIndex("last_date");
  int idxRate=Qry.FieldIndex("rate");
  int idxRateCur=Qry.FieldIndex("rate_cur");
  int idxMinWeight=Qry.FieldIndex("min_weight");
  int idxExtra=Qry.FieldIndex("extra");

  for(; !Qry.Eof; Qry.Next())
  {
    std::ostringstream bagType;
    if (!Qry.FieldIsNULL(idxBagType))
      bagType << std::setw(outdated_?0:2) << std::setfill('0') << Qry.FieldAsInteger(idxBagType);

    rows.setFromInteger (Qry, idxId)
        .setFromString  (Qry, idxAirline)
        .setFromString  (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromBoolean (Qry, idxPrTrfer)
        .setFromString  (Qry, idxCityDep)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityDep)))
        .setFromString  (Qry, idxCityArv)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityArv)))
        .setFromString  (Qry, idxPaxCat)
        .setFromString  (Qry, idxSubclass)
        .setFromString  (ElemIdToCodeNative(etSubcls, Qry.FieldAsString(idxSubclass)))
        .setFromString  (Qry, idxClass)
        .setFromString  (ElemIdToCodeNative(etClass, Qry.FieldAsString(idxClass)))
        .setFromInteger (Qry, idxFltNo)
        .setFromString  (Qry, idxCraft)
        .setFromString  (ElemIdToCodeNative(etCraft, Qry.FieldAsString(idxCraft)))
        .setFromString  (Qry, idxTripType)
        .setFromString  (ElemIdToCodeNative(etTripType, Qry.FieldAsString(idxTripType)))
        .setFromString  (bagType.str())
        .setFromDateTime(Qry, idxFirstDate)
        .setFromDateTime(Qry, idxLastDate)
        .setFromDouble  (Qry, idxRate)
        .setFromString  (Qry, idxRateCur)
        .setFromString  (ElemIdToCodeNative(etCurrency, Qry.FieldAsString(idxRateCur)))
        .setFromInteger (Qry, idxMinWeight)
        .setFromString  (Qry, idxExtra)
        .setFromInteger (pointId.get())
        .setFromBoolean (useMarkFlt)
        .setFromString  (airline.get())
        .setFromInteger (fltNumber.get())
        .addRow();
  }
}

//TripValueBagTaxes

void TripValueBagTaxes::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  PointId_t pointId(notEmptyParamAsInteger("point_id", sqlParams));

  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(pointId.get())) return;

  bool useMarkFlt=notEmptyParamAsBoolean("use_mark_flt", sqlParams);
  AirlineCode_t airline( useMarkFlt ? notEmptyParamAsString("airline_mark", sqlParams) : fltInfo.airline );
  CityCode_t cityDep(getCityByAirp(AirportCode_t(fltInfo.airp)));

  DB::TQuery Qry(PgOra::getROSession("VALUE_BAG_TAXES"), STDLOG);

  Qry.SQLText=
    "SELECT id, airline, pr_trfer, city_dep, city_arv, "
    "       first_date, " + lastDateSelectSQL("VALUE_BAG_TAXES") + ", "
    "       tax, min_value, min_value_cur, extra, "
    "       CASE WHEN pr_trfer=0 OR pr_trfer IS NULL THEN 0 ELSE 1 END AS pr_trfer_order "
    "FROM value_bag_taxes "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (city_dep IS NULL OR city_dep=:city_dep) AND "
    "      (city_arv IS NULL OR "
    "       pr_trfer IS NULL OR pr_trfer<>0 OR "
    "       city_arv IN " + getSQLEnum(getArrivalCities(fltInfo)) + ") AND "
    "      first_date<=:est_scd_out AND (last_date IS NULL OR last_date>:est_scd_out) AND "
    "      pr_del=0 "
    "ORDER BY airline, pr_trfer_order, id";

  Qry.CreateVariable("airline", otString, airline.get());
  Qry.CreateVariable("city_dep", otString, cityDep.get());
  Qry.CreateVariable("est_scd_out", otDate, fltInfo.est_scd_out());
  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxAirline=Qry.FieldIndex("airline");
  int idxPrTrfer=Qry.FieldIndex("pr_trfer");
  int idxCityDep=Qry.FieldIndex("city_dep");
  int idxCityArv=Qry.FieldIndex("city_arv");
  int idxFirstDate=Qry.FieldIndex("first_date");
  int idxLastDate=Qry.FieldIndex("last_date");
  int idxTax=Qry.FieldIndex("tax");
  int idxMinValue=Qry.FieldIndex("min_value");
  int idxMinValueCur=Qry.FieldIndex("min_value_cur");
  int idxExtra=Qry.FieldIndex("extra");

  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromInteger (Qry, idxId)
        .setFromString  (Qry, idxAirline)
        .setFromString  (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromBoolean (Qry, idxPrTrfer)
        .setFromString  (Qry, idxCityDep)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityDep)))
        .setFromString  (Qry, idxCityArv)
        .setFromString  (ElemIdToCodeNative(etCity, Qry.FieldAsString(idxCityArv)))
        .setFromDateTime(Qry, idxFirstDate)
        .setFromDateTime(Qry, idxLastDate)
        .setFromDouble  (Qry, idxTax)
        .setFromDouble  (Qry, idxMinValue)
        .setFromString  (Qry, idxMinValueCur)
        .setFromString  (ElemIdToCodeNative(etCurrency, Qry.FieldAsString(idxMinValueCur)))
        .setFromString  (Qry, idxExtra)
        .setFromInteger (pointId.get())
        .setFromBoolean (useMarkFlt)
        .setFromString  (airline.get())
        .addRow();
  }
}

//TripExchangeRates

void TripExchangeRates::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  PointId_t pointId(notEmptyParamAsInteger("point_id", sqlParams));

  TAdvTripInfo fltInfo;
  if (!fltInfo.getByPointId(pointId.get())) return;

  bool useMarkFlt=notEmptyParamAsBoolean("use_mark_flt", sqlParams);
  AirlineCode_t airline( useMarkFlt ? notEmptyParamAsString("airline_mark", sqlParams) : fltInfo.airline );

  DB::TQuery Qry(PgOra::getROSession("EXCHANGE_RATES"), STDLOG);

  Qry.SQLText=
    "SELECT id, airline, rate1, cur1, rate2, cur2, "
    "       first_date, " + lastDateSelectSQL("EXCHANGE_RATES") + ", extra "
    "FROM exchange_rates "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      first_date<=:est_scd_out AND (last_date IS NULL OR last_date>:est_scd_out) AND "
    "      pr_del=0 "
    "ORDER BY airline, id";

  Qry.CreateVariable("airline", otString, airline.get());
  Qry.CreateVariable("est_scd_out", otDate, fltInfo.est_scd_out());
  Qry.Execute();

  if (Qry.Eof) return;

  int idxId=Qry.FieldIndex("id");
  int idxAirline=Qry.FieldIndex("airline");
  int idxRate1=Qry.FieldIndex("rate1");
  int idxCur1=Qry.FieldIndex("cur1");
  int idxRate2=Qry.FieldIndex("rate2");
  int idxCur2=Qry.FieldIndex("cur2");
  int idxFirstDate=Qry.FieldIndex("first_date");
  int idxLastDate=Qry.FieldIndex("last_date");
  int idxExtra=Qry.FieldIndex("extra");

  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromInteger (Qry, idxId)
        .setFromString  (Qry, idxAirline)
        .setFromString  (ElemIdToCodeNative(etAirline, Qry.FieldAsString(idxAirline)))
        .setFromInteger (Qry, idxRate1)
        .setFromString  (Qry, idxCur1)
        .setFromString  (ElemIdToCodeNative(etCurrency, Qry.FieldAsString(idxCur1)))
        .setFromDouble  (Qry, idxRate2)
        .setFromString  (Qry, idxCur2)
        .setFromString  (ElemIdToCodeNative(etCurrency, Qry.FieldAsString(idxCur2)))
        .setFromDateTime(Qry, idxFirstDate)
        .setFromDateTime(Qry, idxLastDate)
        .setFromString  (Qry, idxExtra)
        .setFromInteger (pointId.get())
        .setFromBoolean (useMarkFlt)
        .setFromString  (airline.get())
        .addRow();
  }
}

//AirlineProfiles

bool AirlineProfiles::userDependence() const {
  return true;
}
std::string AirlineProfiles::selectSql() const {
  return
   "SELECT profile_id, name, airline, airp "
   "FROM airline_profiles "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirports) +
   "ORDER BY name, airline, airp";
}
std::string AirlineProfiles::insertSql() const {
  return "INSERT INTO airline_profiles(profile_id, name, airline, airp) "
         "VALUES(:profile_id, :name, :airline, :airp)";
}
std::string AirlineProfiles::updateSql() const {
  return "UPDATE airline_profiles "
         "SET name = :name, airline = :airline, airp = :airp "
         "WHERE profile_id=:OLD_profile_id";
}
std::string AirlineProfiles::deleteSql() const {
  return "DELETE FROM airline_profiles WHERE profile_id=:OLD_profile_id";
}

std::list<std::string> AirlineProfiles::dbSessionObjectNames() const {
  return {"AIRLINE_PROFILES"};
}

void AirlineProfiles::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                               const std::optional<CacheTable::Row>& oldRow,
                                               std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("profile_id", status, newRow);

  if (status==usDeleted)
  {
    auto cur=make_db_curs("SELECT id FROM profile_rights WHERE profile_id=:profile_id FOR UPDATE",
                          PgOra::getRWSession("PROFILE_RIGHTS"));

    int id;
    cur.stb()
       .def(id)
       .bind(":profile_id", oldRow.value().getAsInteger_ThrowOnEmpty("profile_id"))
       .exec();

    while (!cur.fen())
    {
      auto del=make_db_curs("DELETE FROM profile_rights WHERE id=:id",
                            PgOra::getRWSession("PROFILE_RIGHTS"));

      del.bind(":id", id)
         .exec();

      HistoryTable("profile_rights").synchronize(RowId_t(id));
    }
  }
}

void AirlineProfiles::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("airline_profiles").synchronize(getRowId("profile_id", oldRow, newRow));
}

//ExtraRoleAccess

bool ExtraRoleAccess::userDependence() const {
  return true;
}
std::string ExtraRoleAccess::selectSql() const {
  return
   "SELECT id, airline_from, airp_from, airline_to, airp_to, full_access "
   "FROM extra_role_access "
   "WHERE " + getSQLFilter("airline_from", AccessControl::PermittedAirlines) + " AND "
            + getSQLFilter("airline_to",   AccessControl::PermittedAirlines) + " AND "
            + getSQLFilter("airp_from",    AccessControl::PermittedAirports) + " AND "
            + getSQLFilter("airp_to",      AccessControl::PermittedAirports) +
   "ORDER BY id";
}
std::string ExtraRoleAccess::insertSql() const {
  return "INSERT INTO extra_role_access(id, airline_from, airp_from, airline_to, airp_to, full_access) "
         "VALUES(:id, :airline_from, :airp_from, :airline_to, :airp_to, :full_access)";
}
std::string ExtraRoleAccess::updateSql() const {
  return "UPDATE extra_role_access "
         "SET airline_from=:airline_from, airp_from=:airp_from, "
         "    airline_to=:airline_to, airp_to=:airp_to, full_access=:full_access "
         "WHERE id=:OLD_id";
}
std::string ExtraRoleAccess::deleteSql() const {
  return "DELETE FROM extra_role_access WHERE id=:OLD_id";
}

std::list<std::string> ExtraRoleAccess::dbSessionObjectNames() const {
  return {"EXTRA_ROLE_ACCESS"};
}

void ExtraRoleAccess::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                               const std::optional<CacheTable::Row>& oldRow,
                                               std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline_from", oldRow, newRow);
  checkAirportAccess("airp_from", oldRow, newRow);
  checkAirlineAccess("airline_to", oldRow, newRow);
  checkAirportAccess("airp_to", oldRow, newRow);

  setRowId("id", status, newRow);
}

void ExtraRoleAccess::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("extra_role_access").synchronize(getRowId("id", oldRow, newRow));
}

//ExtraUserAccess

bool ExtraUserAccess::userDependence() const {
  return true;
}
std::string ExtraUserAccess::selectSql() const {
  return
   "SELECT id, airline_from, airp_from, airline_to, airp_to, full_access, "
   "       type_from AS type_code_from, type_from AS type_name_from, "
   "       type_to AS type_code_to, type_to AS type_name_to "
   "FROM extra_user_access "
   "WHERE " + getSQLFilter("airline_from", AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airline_to",   AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp_from",    AccessControl::PermittedAirportsOrNull) + " AND "
            + getSQLFilter("airp_to",      AccessControl::PermittedAirportsOrNull) +
   (TReqInfo::Instance()->user.user_type!=utSupport?
      " AND (type_from=:user_type OR type_to=:user_type) ":"") +
   "ORDER BY id";
}
std::string ExtraUserAccess::insertSql() const {
  return "INSERT INTO extra_user_access(id, type_from, airline_from, airp_from, type_to, airline_to, airp_to, full_access) "
         "VALUES(:id, :type_code_from, :airline_from, :airp_from, :type_code_to, :airline_to, :airp_to, :full_access)";
}
std::string ExtraUserAccess::updateSql() const {
  return "UPDATE extra_user_access "
         "SET type_from=:type_code_from, airline_from=:airline_from, airp_from=:airp_from, "
         "    type_to=:type_code_to, airline_to=:airline_to, airp_to=:airp_to, full_access=:full_access "
         "WHERE id=:OLD_id";
}
std::string ExtraUserAccess::deleteSql() const {
  return "DELETE FROM extra_user_access WHERE id=:OLD_id";
}

std::list<std::string> ExtraUserAccess::dbSessionObjectNames() const {
  return {"EXTRA_USER_ACCESS"};
}

void ExtraUserAccess::beforeSelectOrRefresh(const TCacheQueryType queryType,
                                            const TParams& sqlParams,
                                            DB::TQuery& Qry) const
{
  TUserType userType=TReqInfo::Instance()->user.user_type;
  if (userType!=utSupport)
    Qry.CreateVariable("user_type", otInteger, (int)userType);
}

void ExtraUserAccess::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                               const std::optional<CacheTable::Row>& oldRow,
                                               std::optional<CacheTable::Row>& newRow) const
{
  checkUserTypesAccess("type_code_from", "type_code_to", oldRow, newRow);

  checkAirlineAccess("airline_from", oldRow, newRow);
  checkAirportAccess("airp_from", oldRow, newRow);
  checkAirlineAccess("airline_to", oldRow, newRow);
  checkAirportAccess("airp_to", oldRow, newRow);

  setRowId("id", status, newRow);
}

void ExtraUserAccess::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("extra_user_access").synchronize(getRowId("id", oldRow, newRow));
}

//ValidatorTypes

bool ValidatorTypes::userDependence() const {
  return false;
}
std::string ValidatorTypes::selectSql() const {
  return "SELECT code, code_lat, name, name_lat FROM validator_types ORDER BY code";
}
std::list<std::string> ValidatorTypes::dbSessionObjectNames() const {
  return {"VALIDATOR_TYPES"};
}

//FormTypes

bool FormTypes::userDependence() const {
  return false;
}
std::string FormTypes::selectSql() const {
  return
   "SELECT code, basic_type, name, name_lat, series_len, no_len, pr_check_bit, validator, id "
   "FROM form_types "
   "ORDER BY code";
}
std::string FormTypes::insertSql() const {
  return "insert into form_types(code, name, name_lat, series_len, no_len, pr_check_bit, validator, basic_type, id) "
         "values(:code, :name, :name_lat, :series_len, :no_len, :pr_check_bit, :validator, :basic_type, :id)";
}
std::string FormTypes::updateSql() const {
  return "";
}
std::string FormTypes::deleteSql() const {
  return "delete from form_types where code = :OLD_code";
}
std::list<std::string> FormTypes::dbSessionObjectNames() const {
  return {"FORM_TYPES"};
}

void FormTypes::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

void FormTypes::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("form_types").synchronize(getRowId("id", oldRow, newRow));
}

//FormPacks

bool FormPacks::userDependence() const {
  return true;
}
bool FormPacks::updateImplemented() const {
  return true;
}

void FormPacks::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession({"FORM_TYPES", "FORM_PACKS"}), STDLOG);

  Qry.SQLText=
    "SELECT form_types.code AS type, "
    "       form_types.basic_type, "
    "       form_types.series_len, "
    "       form_types.no_len, "
    "       form_types.pr_check_bit, "
    "       form_types.validator, "
    "       form_packs.curr_no "
    "FROM form_types LEFT OUTER JOIN form_packs "
    "ON form_types.code=form_packs.type AND form_packs.user_id=:user_id "
    "ORDER BY type";
  Qry.CreateVariable("user_id", otInteger, TReqInfo::Instance()->user.user_id);
  Qry.Execute();

  if (Qry.Eof) return;

  int idxType=Qry.FieldIndex("type");
  int idxBasicType=Qry.FieldIndex("basic_type");
  int idxSeriesLen=Qry.FieldIndex("series_len");
  int idxNoLen=Qry.FieldIndex("no_len");
  int idxPrCheckBit=Qry.FieldIndex("pr_check_bit");
  int idxValidator=Qry.FieldIndex("validator");
  int idxCurrNo=Qry.FieldIndex("curr_no");

  ViewAccess<ValidatorCode_t> validatorViewAccess;

  for(; !Qry.Eof; Qry.Next())
  {
    ValidatorCode_t validatorCode(Qry.FieldAsString(idxValidator));

    if (!validatorViewAccess.check(validatorCode)) continue;

    std::string currNoStr;
    if (!Qry.FieldIsNULL(idxCurrNo))
      currNoStr = StrUtils::LPad(FloatToString(Qry.FieldAsFloat(idxCurrNo), 0),
                                 Qry.FieldAsInteger(idxNoLen),
                                 '0');

    rows.setFromInteger(TReqInfo::Instance()->user.user_id)
        .setFromString(Qry, idxType)
        .setFromString(Qry, idxBasicType)
        .setFromString(currNoStr)
        .setFromInteger(Qry, idxSeriesLen)
        .setFromInteger(Qry, idxNoLen)
        .setFromBoolean(Qry, idxPrCheckBit)
        .addRow();
  }
}

void FormPacks::onApplyingRowChanges(const TCacheUpdateStatus status,
                                     const std::optional<CacheTable::Row>& oldRow,
                                     const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usModified)
  {
    std::optional<double> currNo=newRow.value().getAsDouble("curr_no");
    int userId=oldRow.value().getAsInteger_ThrowOnEmpty("user_id");
    std::string type=oldRow.value().getAsString_ThrowOnEmpty("type");
    if (!currNo)
    {
      auto cur=make_db_curs("DELETE FROM form_packs WHERE user_id=:user_id AND type=:type",
                            PgOra::getRWSession("FORM_PACKS"));

      cur.bind(":user_id", userId)
         .bind(":type", type)
         .exec();
    }
    else
    {
      auto cur=make_db_curs("UPDATE form_packs SET curr_no=:curr_no WHERE user_id=:user_id AND type=:type",
                            PgOra::getRWSession("FORM_PACKS"));

      cur.bind(":user_id", userId)
         .bind(":type", type)
         .bind(":curr_no", currNo.value())
         .exec();

      if (cur.rowcount()==0)
      {
        auto cur=make_db_curs("INSERT INTO form_packs(user_id, type, curr_no) VALUES(:user_id, :type, :curr_no)",
                              PgOra::getRWSession("FORM_PACKS"));

        cur.bind(":user_id", userId)
           .bind(":type", type)
           .bind(":curr_no", currNo.value())
           .exec();
      }
    }
  }
}

//SalePoints

bool SalePoints::userDependence() const {
  return false;
}
std::string SalePoints::selectSql() const {
  return "SELECT code, validator, agency, city, descr, descr_lat, phone, id FROM sale_points ORDER BY code";
}
std::string SalePoints::insertSql() const {
  return "INSERT INTO sale_points(code, validator, agency, city, descr, descr_lat, phone, forms, pr_permit, id) "
         "VALUES(:code, :validator, :agency, :city, :descr, :descr_lat, :phone, NULL, 1, :id)";
}
std::string SalePoints::updateSql() const {
  return "UPDATE sale_points "
         "SET code=:code, validator=:validator, agency=:agency, city=:city, descr=:descr, descr_lat=:descr_lat, phone=:phone "
         "WHERE id=:OLD_id";
}
std::string SalePoints::deleteSql() const {
  return "DELETE FROM sale_points WHERE id=:OLD_id";
}
std::list<std::string> SalePoints::dbSessionObjectNames() const {
  return {"SALE_POINTS"};
}

void SalePoints::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    std::string code=newRow.value().getAsString("code");
    if (code.size()!=8 ||
        !algo::all_of(code, [](const auto &c) { return IsUpperLetter(c) || IsDigit(c); }))
      throw UserException("MSG.SALE_POINT_CONSISTS_OF_8_LET_DIG");
  }

  setRowId("id", status, newRow);
}

void SalePoints::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("sale_points").synchronize(getRowId("id", oldRow, newRow));
}

std::map<DeskCode_t, Desk> getDesks()
{
  auto cur = make_db_curs("SELECT code, id, currency FROM desks",
                          PgOra::getROSession("DESKS"));

  std::string code, currency;
  int id;
  cur.def(code)
     .def(id)
     .def(currency)
     .exec();

  std::map<DeskCode_t, Desk> result;
  while (!cur.fen())
    result.emplace(DeskCode_t(code), Desk(DeskCode_t(code), RowId_t(id), CurrencyCode_t(currency)));

  return result;
}

//SaleDesks

bool SaleDesks::userDependence() const {
  return false;
}

void SaleDesks::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("SALE_DESKS"), STDLOG);

  Qry.SQLText="SELECT code, sale_point, validator, pr_denial, id FROM sale_desks ORDER BY code";
  Qry.Execute();

  if (Qry.Eof) return;

  const auto desks=getDesks();

  int idxCode=Qry.FieldIndex("code");
  int idxSalePoint=Qry.FieldIndex("sale_point");
  int idxValidator=Qry.FieldIndex("validator");
  int idxPrDenial=Qry.FieldIndex("pr_denial");
  int idxId=Qry.FieldIndex("id");

  for(; !Qry.Eof; Qry.Next())
  {
    std::optional<Desk> desk;

    std::string saleDeskCode=Qry.FieldAsString(idxCode);
    if (saleDeskCode.size()==6) //��⮬� �� �� �ॡ㥬 ��ண� 6 ᨬ����� �� ����� � sale_desks
      desk = algo::find_opt<std::optional>(desks, DeskCode_t(saleDeskCode));

    rows.setFromString(saleDeskCode)
        .setFromString(Qry, idxSalePoint)
        .setFromString(Qry, idxValidator)
        .setFromString(ElemIdToCodeNative(etValidatorType, Qry.FieldAsString(idxValidator)))
        .setFromString(desk?desk.value().currency.get():"")
        .setFromString(desk?ElemIdToCodeNative(etCurrency, desk.value().currency.get()):"")
        .setFromBoolean(Qry, idxPrDenial)
        .setFromString(desk?IntToString(desk.value().id.get()):"")
        .setFromInteger(Qry, idxId)
        .addRow();
  }
}

std::string SaleDesks::insertSql() const {
  return "INSERT INTO sale_desks(id, code, sale_point, validator, pr_denial) "
         "VALUES(:sale_desks_id, :code, :sale_point, :validator, :pr_denial)";
}
std::string SaleDesks::updateSql() const {
  return "UPDATE sale_desks "
         "SET code=:code, sale_point=:sale_point, validator=:validator, pr_denial=:pr_denial "
         "WHERE id=:OLD_sale_desks_id";
}
std::string SaleDesks::deleteSql() const {
  return "DELETE FROM sale_desks WHERE id=:OLD_sale_desks_id";
}
std::list<std::string> SaleDesks::dbSessionObjectNames() const {
  return {"SALE_DESKS"};
}

void SaleDesks::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  setRowId("sale_desks_id", status, newRow);

  if (newRow)
  {
    auto cur=make_db_curs("SELECT id FROM desks WHERE code=:code",
                          PgOra::getROSession("DESKS"));
    int id;
    cur.stb()
       .def(id)
       .bind(":code", newRow.value().getAsString("code"))
       .EXfet();

    if (cur.err() != DbCpp::ResultCode::NoDataFound)
      newRow.value().setFromInteger("desks_id", id);
    else
      newRow.value().setFromInteger("desks_id", std::nullopt);

  }
}

void SaleDesks::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    std::optional<int> deskId=newRow.value().getAsInteger("desks_id");

    if (deskId)
    {
      auto cur=make_db_curs("UPDATE desks SET currency=:currency WHERE id=:id",
                            PgOra::getRWSession("DESKS"));
      cur.stb()
         .bind(":id",       deskId.value())
         .bind(":currency", newRow.value().getAsString_ThrowOnEmpty("currency"))
         .exec();

      HistoryTable("desks").synchronize(RowId_t(deskId.value()));
    }
  }

  HistoryTable("sale_desks").synchronize(getRowId("sale_desks_id", oldRow, newRow));
}

//Operators

bool Operators::userDependence() const {
  return false;
}
std::string Operators::selectSql() const {
  return "SELECT login, private_num, agency, validator, descr, pr_denial, id FROM operators ORDER BY login";
}
std::string Operators::insertSql() const {
  return "INSERT INTO operators(login, private_num, agency, validator, descr, pr_denial, id) "
         "VALUES(:login, :private_num, :agency, :validator, :descr, :pr_denial, :id)";
}
std::string Operators::updateSql() const {
  return "UPDATE operators "
         "SET login=:login, private_num=:private_num, agency=:agency, validator=:validator, descr=:descr, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string Operators::deleteSql() const {
  return "DELETE FROM operators WHERE id=:OLD_id";
}
std::list<std::string> Operators::dbSessionObjectNames() const {
  return {"OPERATORS"};
}

void Operators::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

void Operators::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("operators").synchronize(getRowId("id", oldRow, newRow));
}

//PayClients

bool PayClients::userDependence() const {
  return true;
}
std::string PayClients::selectSql() const {
  return
   "SELECT id, client_id, airline, airp_dep, pact, pr_denial "
   "FROM pay_clients "
   "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp_dep", AccessControl::PermittedAirportsOrNull) +
   "ORDER BY client_id";
}
std::string PayClients::insertSql() const {
  return "INSERT INTO pay_clients(id, client_id, airline, airp_dep, pact, pr_denial) "
         "VALUES(:id, :client_id, :airline, :airp_dep, :pact, :pr_denial)";
}
std::string PayClients::updateSql() const {
  return "UPDATE pay_clients "
         "SET client_id=:client_id, airline=:airline, airp_dep=:airp_dep, pact=:pact, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string PayClients::deleteSql() const {
  return "DELETE FROM pay_clients WHERE id=:OLD_id";
}
std::list<std::string> PayClients::dbSessionObjectNames() const {
  return {"PAY_CLIENTS"};
}

void PayClients::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    if (newRow.value().getAsString("airline").empty() &&
        newRow.value().getAsString("airp_dep").empty())
      throw UserException("MSG.AIRLINE_OR_AIRPORT_REQUIRED");
  }

  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  setRowId("id", status, newRow);
}

void PayClients::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("pay_clients").synchronize(getRowId("id", oldRow, newRow));
}

//PosTermVendors

bool PosTermVendors::userDependence() const {
  return false;
}
std::string PosTermVendors::selectSql() const {
  return
   "SELECT id, code, serial_rule, descr, descr_lat "
   "FROM pos_term_vendors "
   "ORDER BY id";
}
std::string PosTermVendors::insertSql() const {
  return "INSERT INTO pos_term_vendors(id, code, serial_rule, descr, descr_lat) "
         "VALUES(:id, :code, :serial_rule, :descr, :descr_lat)";
}
std::string PosTermVendors::updateSql() const {
  return "UPDATE pos_term_vendors "
         "SET code=:code, serial_rule=:serial_rule, descr=:descr, descr_lat=:descr_lat "
         "WHERE id=:OLD_id";
}
std::string PosTermVendors::deleteSql() const {
  return "DELETE FROM pos_term_vendors WHERE id=:OLD_id";
}
std::list<std::string> PosTermVendors::dbSessionObjectNames() const {
  return {"POS_TERM_VENDORS"};
}

void PosTermVendors::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

//PosTermSets

bool PosTermSets::userDependence() const {
  return true;
}
std::string PosTermSets::selectSql() const {
  return
   "SELECT pos_term_sets.id, "
          "pos_term_sets.airline, "
          "pos_term_sets.airp, "
          "pos_term_sets.shop_id, "
          "pos_term_sets.client_id, "
          "pay_clients.client_id AS client_view, "
          "pos_term_sets.serial, "
          "pos_term_sets.address, "
          "pos_term_sets.name, "
          "pos_term_sets.pr_denial, "
          "pos_term_sets.vendor_id, "
          "pos_term_vendors.code AS vendor_view "
   "FROM pos_term_sets, pay_clients, pos_term_vendors "
   "WHERE " + getSQLFilter("pos_term_sets.airline", AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("pos_term_sets.airp",    AccessControl::PermittedAirportsOrNull) + " AND "
   "      pay_clients.id=pos_term_sets.client_id AND pos_term_sets.vendor_id=pos_term_vendors.id "
   "ORDER BY pos_term_sets.id";
}
std::string PosTermSets::insertSql() const {
  return "INSERT INTO pos_term_sets(id, airline, airp, shop_id, client_id, serial, address, name, pr_denial, vendor_id) "
         "VALUES(:id, :airline, :airp, :shop_id, :client_id, :serial, :address, :name, :pr_denial, :vendor_id)";
}
std::string PosTermSets::updateSql() const {
  return "UPDATE pos_term_sets "
         "SET airline=:airline, airp=:airp, shop_id=:shop_id, client_id=:client_id, serial=:serial, address=:address, "
         "    name=:name, pr_denial=:pr_denial, vendor_id=:vendor_id "
         "WHERE id=:OLD_id";
}
std::string PosTermSets::deleteSql() const {
  return "DELETE FROM pos_term_sets WHERE id=:OLD_id";
}
std::list<std::string> PosTermSets::dbSessionObjectNamesForRead() const {
  return {"POS_TERM_SETS", "PAY_CLIENTS", "POS_TERM_VENDORS"};
}
std::list<std::string> PosTermSets::dbSessionObjectNames() const {
  return {"POS_TERM_SETS"};
}

void PosTermSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    if (newRow.value().getAsString("airline").empty() &&
        newRow.value().getAsString("airp").empty())
      throw UserException("MSG.AIRLINE_OR_AIRPORT_REQUIRED");
  }

  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void PosTermSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("pos_term_sets").synchronize(getRowId("id", oldRow, newRow));
}

//PlaceCalc

bool PlaceCalc::userDependence() const {
  return true;
}
std::string PlaceCalc::selectSql() const {
  return "SELECT bc AS craft, cod_out AS airp_dep, cod_in AS airp_arv, time_out_in AS time, id "
         "FROM place_calc "
         "WHERE " + getSQLFilter("cod_out", AccessControl::PermittedAirports) + " OR "
                  + getSQLFilter("cod_in",  AccessControl::PermittedAirports) + " "
         "ORDER BY airp_dep,airp_arv,craft";
}
std::string PlaceCalc::insertSql() const {
  return "INSERT INTO place_calc(bc, cod_out, cod_in, time_out_in, id) "
         "VALUES(:craft, :airp_dep, :airp_arv, :time, :id)";
}
std::string PlaceCalc::updateSql() const {
  return "UPDATE place_calc "
         "SET bc=:craft, cod_out=:airp_dep, cod_in=:airp_arv, time_out_in=:time "
         "WHERE id=:OLD_id";
}
std::string PlaceCalc::deleteSql() const {
  return "DELETE FROM place_calc WHERE id=:OLD_id";
}
std::list<std::string> PlaceCalc::dbSessionObjectNames() const {
  return {"PLACE_CALC"};
}

void PlaceCalc::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkNotNullAirportOrAirportAccess("airp_dep", "airp_arv", oldRow, newRow);

  setRowId("id", status, newRow);
}

void PlaceCalc::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("place_calc").synchronize(getRowId("id", oldRow, newRow));
}

//CompSubclsSets

bool CompSubclsSets::userDependence() const {
  return true;
}
std::string CompSubclsSets::selectSql() const {
  return "SELECT airline, subclass, rem, id "
         "FROM comp_subcls_sets "
         "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) +
         "ORDER BY airline, rem, subclass";
}
std::string CompSubclsSets::insertSql() const {
  return "INSERT INTO comp_subcls_sets(airline, subclass, rem, id) "
         "VALUES(:airline, :subclass, :rem, :id)";
}
std::string CompSubclsSets::updateSql() const {
  return "UPDATE comp_subcls_sets "
         "SET airline=:airline, subclass=:subclass, rem=:rem "
         "WHERE id=:OLD_id";
}
std::string CompSubclsSets::deleteSql() const {
  return "DELETE FROM comp_subcls_sets WHERE id=:OLD_id";
}
std::list<std::string> CompSubclsSets::dbSessionObjectNames() const {
  return {"COMP_SUBCLS_SETS"};
}

void CompSubclsSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  setRowId("id", status, newRow);
}

void CompSubclsSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("comp_subcls_sets").synchronize(getRowId("id", oldRow, newRow));
}

} //namespace CacheTables
