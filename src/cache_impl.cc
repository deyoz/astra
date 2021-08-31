#include "cache_impl.h"
#include "PgOraConfig.h"
#include "astra_elems.h"
#include "astra_types.h"
#include "kassa.h"
#include "brands.h"
#include "kiosk/kiosk_cache_impl.h"
#include "cache_tables/roles.h"
#include "stat/stat_general.h"
#include "codeshare_sets.h"
#include "apis_settings.h"

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
  if (cacheCode=="HOTEL_ROOM_TYPES")    return new CacheTable::HotelRoomTypes;
  if (cacheCode=="DEV_OPER_TYPES")      return new CacheTable::DevOperTypes;
  if (cacheCode=="DEV_FMT_OPERS")       return new CacheTable::DevFmtOpers;
  if (cacheCode=="DEV_FMT_TYPES")       return new CacheTable::DevFmtTypes;
  if (cacheCode=="DEV_MODELS")          return new CacheTable::DevModels;
  if (cacheCode=="PACTS")               return new CacheTable::Pacts;
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
  if (cacheCode=="PAY_METHODS_TYPES")   return new CacheTable::PayMethodsTypes;
  if (cacheCode=="PAY_METHODS_SET")     return new CacheTable::PayMethodsSet;
  if (cacheCode=="PAY_CLIENTS")         return new CacheTable::PayClients;
  if (cacheCode=="POS_TERM_VENDORS")    return new CacheTable::PosTermVendors;
  if (cacheCode=="POS_TERM_SETS")       return new CacheTable::PosTermSets;
  if (cacheCode=="PLACE_CALC")          return new CacheTable::PlaceCalc;
  if (cacheCode=="COMP_SUBCLS_SETS")    return new CacheTable::CompSubclsSets;
  if (cacheCode=="KIOSK_LANG")          return new CacheTable::KioskLang;
  if (cacheCode=="KIOSK_ALIASES_LIST")  return new CacheTable::KioskAliasesList;
  if (cacheCode=="KIOSK_APP_LIST")      return new CacheTable::KioskAppList;
  if (cacheCode=="KIOSK_CONFIG_LIST")   return new CacheTable::KioskConfigList;
  if (cacheCode=="KIOSK_ADDR")          return new CacheTable::KioskAddr;
  if (cacheCode=="KIOSK_GRP_NAMES")     return new CacheTable::KioskGrpNames;
  if (cacheCode=="KIOSK_ALIASES")       return new CacheTable::KioskAliases;
  if (cacheCode=="KIOSK_CONFIG")        return new CacheTable::KioskConfig;
  if (cacheCode=="KIOSK_GRP")           return new CacheTable::KioskGrp;
  if (cacheCode=="CODESHARE_SETS")      return new CacheTable::CodeshareSets;
#ifndef ENABLE_ORACLE
  if (cacheCode=="AIRLINES")            return new CacheTable::Airlines;
  if (cacheCode=="AIRPS")               return new CacheTable::Airps;
  if (cacheCode=="CITIES")              return new CacheTable::Cities;
  if (cacheCode=="TRIP_TYPES")          return new CacheTable::TripTypes;
#endif //ENABLE_ORACLE
  if (cacheCode=="COMP_REM_TYPES")      return new CacheTable::CompRemTypes;
  if (cacheCode=="CKIN_REM_TYPES")      return new CacheTable::CkinRemTypes;
  if (cacheCode=="BALANCE_TYPES")       return new CacheTable::BalanceTypes;
  if (cacheCode=="BALANCE_SETS")        return new CacheTable::BalanceSets;
  if (cacheCode=="CRS_SET")             return new CacheTable::CrsSet;
  if (cacheCode=="SELF_CKIN_TYPES")     return new CacheTable::SelfCkinTypes;
  if (cacheCode=="WEB_CKIN_SETS")           return new CacheTable::WebCkinSets;
  if (cacheCode=="AIRLINE_WEB_CKIN_SETS")   return new CacheTable::AirlineWebCkinSets;
  if (cacheCode=="KIOSK_CKIN_SETS")         return new CacheTable::KioskCkinSets;
  if (cacheCode=="AIRLINE_KIOSK_CKIN_SETS") return new CacheTable::AirlineKioskCkinSets;
  if (cacheCode=="BP_TYPES")            return new CacheTable::BpTypes;
  if (cacheCode=="BP_MODELS")           return new CacheTable::BpModels;
  if (cacheCode=="BP_BLANK_LIST")       return new CacheTable::BpBlankList;
  if (cacheCode=="BI_TYPES")            return new CacheTable::BiTypes;
  if (cacheCode=="BI_MODELS")           return new CacheTable::BiModels;
  if (cacheCode=="BI_BLANK_LIST")       return new CacheTable::BiBlankList;
  if (cacheCode=="VO_MODELS")           return new CacheTable::VoModels;
  if (cacheCode=="VO_BLANK_LIST")       return new CacheTable::VoBlankList;
  if (cacheCode=="EMDA_MODELS")         return new CacheTable::EmdAModels;
  if (cacheCode=="EMDA_BLANK_LIST")     return new CacheTable::EmdABlankList;
  if (cacheCode=="BR_MODELS")           return new CacheTable::BrModels;
  if (cacheCode=="BR_BLANK_LIST")       return new CacheTable::BrBlankList;
  if (cacheCode=="BT_MODELS")           return new CacheTable::BtModels;
  if (cacheCode=="BT_BLANK_LIST")       return new CacheTable::BtBlankList;
  if (cacheCode=="PRN_FORM_VERS")       return new CacheTable::PrnFormVers;
  if (cacheCode=="PRN_FORMS")           return new CacheTable::PrnForms;
  if (cacheCode=="TRIP_BT")             return new CacheTable::TripBt;
  if (cacheCode=="BRAND_FARES")         return new CacheTable::BrandFares;
  if (cacheCode=="BI_PRINT_TYPES")      return new CacheTable::BiPrintTypes;
  if (cacheCode=="BI_PRINT_RULES")      return new CacheTable::BiPrintRules;
  if (cacheCode=="EDIFACT_PROFILES")    return new CacheTable::EdifactProfiles;
  if (cacheCode=="EDI_ADDRS")           return new CacheTable::EdiAddrs;
  if (cacheCode=="ET_ADDR_SET")         return new CacheTable::EtAddrSet;
  if (cacheCode=="DCS_ADDR_SET")        return new CacheTable::DcsAddrSet;
  if (cacheCode=="GENDER_TYPES")        return new CacheTable::GenderTypes;
  if (cacheCode=="DESK_OWNERS_ADD")     return new CacheTable::DeskOwnersAdd;
  if (cacheCode=="DESK_OWNERS_GRP")     return new CacheTable::DeskOwnersGrp;
  if (cacheCode=="RFISC_RATES")         return new CacheTable::RfiscRates;
  if (cacheCode=="RFISC_RATES_SELF_CKIN") return new CacheTable::RfiscRatesSelfCkin;
  if (cacheCode=="RFIC_TYPES")          return new CacheTable::RficTypes;
  if (cacheCode=="RFISC_TYPES")         return new CacheTable::RfiscTypes;
  if (cacheCode=="RFISC_SETS")          return new CacheTable::RfiscSets;
  if (cacheCode=="RFISC_COMP_PROPS")    return new CacheTable::RfiscCompProps;
  if (cacheCode=="RFISC_BAG_PROPS")     return new CacheTable::RfiscBagProps;
  if (cacheCode=="DCS_SERVICE_APPLYING") return new CacheTable::DcsServiceApplying;
  if (cacheCode=="DCS_ACTIONS1")        return new CacheTable::DcsActions1;
  if (cacheCode=="DCS_ACTIONS2")        return new CacheTable::DcsActions2;
  if (cacheCode=="REM_TXT_SETS")        return new CacheTable::RemTxtSets;
  if (cacheCode=="CUSTOM_ALARM_SETS")   return new CacheTable::CustomAlarmSets;
  if (cacheCode=="CUSTOM_ALARM_TYPES")  return new CacheTable::CustomAlarmTypes;
  if (cacheCode=="CONFIRMATION_SETS")   return new CacheTable::ConfirmationSets;
  if (cacheCode=="BRANDS")              return new CacheTable::Brands;
  if (cacheCode=="APIS_FORMATS")        return new CacheTable::ApisFormats;
  if (cacheCode=="APIS_TRANSPORTS")     return new CacheTable::ApisTransports;
  if (cacheCode=="APIS_SETS")           return new CacheTable::ApisSets;
  if (cacheCode=="TIMATIC_SETS")        return new CacheTable::TimaticSets;
  if (cacheCode=="ROLES")               return new CacheTable::Roles;
  if (cacheCode=="TRIP_LIST_DAYS")      return new CacheTable::TripListDays;
  if (cacheCode=="RIGHTS")              return new CacheTable::Rights;
  if (cacheCode=="PROFILED_RIGHTS_LIST") return new CacheTable::ProfiledRightsList;
  if (cacheCode=="USERS2")              return new CacheTable::Users;
  if (cacheCode=="USERS_TYPES")         return new CacheTable::UserTypes;

  return nullptr;
}

namespace CacheTable
{

void checkInvalidSymbolInName(const std::string &value,
                              const bool latinOnly,
                              const std::string &additionalSymbols,
                              const std::string &cacheTable,
                              const std::string &cacheField)
{
  const std::optional<std::string> ch = invalidSymbolInName(value, latinOnly, additionalSymbols);
  if (ch) {
    std::string fieldName=getLocaleText(getCacheInfo(cacheTable).fieldTitle.at(cacheField));
    throw UserException("MSG.FIELD_INCLUDE_INVALID_CHARACTER1",
                        LParams() << LParam("field_name", fieldName)
                                  << LParam("symbol", *ch));
  }
}

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
  if (Qry.Eof) return;

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
  if (Qry.Eof) return;

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
  checkDeskAndDeskGrp(fieldDesk, fieldDeskGrpId, false, newRow);
  checkDeskAccess(fieldDesk, oldRow, newRow);
  checkDeskGrpAccess(fieldDeskGrpId, false, oldRow, newRow);
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

  rows.setFromString(isLangRu?"ВСЕ":"ALL")
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

//HotelRoomTypes

bool HotelRoomTypes::userDependence() const
{
  return false;
}

std::string HotelRoomTypes::selectSql() const
{
  return "SELECT id, name FROM hotel_room_types";
}

std::list<std::string> HotelRoomTypes::dbSessionObjectNames() const
{
  return {"HOTEL_ROOM_TYPES"};
}

//DevOperTypes

bool DevOperTypes::userDependence() const
{
  return false;
}

std::string DevOperTypes::selectSql() const
{
  return "SELECT code, name FROM dev_oper_types ORDER BY code, name ";
}

std::list<std::string> DevOperTypes::dbSessionObjectNames() const
{
  return {"DEV_OPER_TYPES"};
}

//DevFmtOpers

bool DevFmtOpers::userDependence() const
{
  return false;
}

std::string DevFmtOpers::selectSql() const
{
  return "SELECT op_type, fmt_type FROM dev_fmt_opers ORDER BY op_type, fmt_type ";
}

std::list<std::string> DevFmtOpers::dbSessionObjectNames() const
{
  return {"DEV_FMT_OPERS"};
}

//DevFmtTypes

bool DevFmtTypes::userDependence() const
{
  return false;
}

std::string DevFmtTypes::selectSql() const
{
  return "SELECT code, name FROM dev_fmt_types ";
}

std::string DevFmtTypes::insertSql() const
{
  return "INSERT INTO dev_fmt_types(code, name) VALUES(:code, :name) ";
}

std::string DevFmtTypes::updateSql() const
{
  return "UPDATE dev_fmt_types SET name = :name WHERE code = :OLD_code ";
}

std::string DevFmtTypes::deleteSql() const
{
  return "DELETE FROM dev_fmt_types WHERE code = :OLD_code ";
}

std::list<std::string> DevFmtTypes::dbSessionObjectNames() const
{
  return {"DEV_FMT_TYPES"};
}

//DevModels

bool DevModels::userDependence() const
{
  return false;
}

std::string DevModels::selectSql() const
{
  return "SELECT DISTINCT "
         "   dev_model_sess_fmt.dev_model code, "
         "   dev_models.name, "
         "   dev_fmt_opers.op_type, "
         "   dev_fmt_opers.fmt_type "
         "FROM   "
         "   dev_model_sess_fmt, "
         "   dev_models, "
         "   dev_fmt_opers "
         "WHERE   "
         "   dev_model_sess_fmt.dev_model = dev_models.code "
         "   AND dev_model_sess_fmt.fmt_type = dev_fmt_opers.fmt_type "
         "ORDER BY op_type, fmt_type, code ";
}

std::list<std::string> DevModels::dbSessionObjectNames() const
{
  return {"DEV_FMT_OPERS","DEV_MODELS","DEV_MODEL_SESS_FMT"};
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

std::string lastDateSelectSQL(const std::string& objectName, const std::string& field_name = "last_date")
{
  return PgOra::supportsPg(objectName)
          ? (field_name + " - interval '1 second' AS " + field_name)
          : (field_name + " - 1 / 86400 AS " + field_name);
}

//SelfCkinTypes

bool SelfCkinTypes::userDependence() const
{
  return false;
}

std::string SelfCkinTypes::selectSql() const
{
  return "SELECT code, name, name_lat "
         "FROM client_types "
         "WHERE code IN ('WEB', 'MOBIL', 'KIOSK') "
         "ORDER BY priority ";
}

std::list<std::string> SelfCkinTypes::dbSessionObjectNames() const
{
  return {"CLIENT_TYPES"};
}

//WebCkinSets

bool WebCkinSets::userDependence() const
{
  return true;
}

std::string WebCkinSets::selectSql() const
{
  return "SELECT id,airline,airp_dep,flt_no,pr_permit,pr_waitlist,pr_tckin,pr_upd_stage "
         "FROM ckin_client_sets WHERE client_type='WEB' AND desk_grp_id IS NULL AND "
         + selectSqlAddCondition() +
         "      " + getSQLFilter("airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
         "      " + getSQLFilter("airp_dep", AccessControl::PermittedAirportsOrNull) + " "
         "ORDER BY airp_dep,airline,id ";
}

std::string WebCkinSets::insertSql() const
{
  return "INSERT INTO ckin_client_sets( "
         "  id,client_type,desk_grp_id,flt_no,airline,airp_dep,pr_permit,pr_waitlist,pr_tckin,pr_upd_stage "
         ") VALUES("
         "  :id,'WEB',NULL,:flt_no,:airline,:airp_dep,:pr_permit,0,:pr_tckin,:pr_upd_stage "
         ") ";
}

std::string WebCkinSets::updateSql() const
{
  return "UPDATE ckin_client_sets "
         "SET airline=:airline,flt_no=:flt_no,airp_dep=:airp_dep,pr_permit=:pr_permit, "
         "    pr_waitlist=:pr_waitlist,pr_tckin=:pr_tckin,pr_upd_stage=:pr_upd_stage "
         "WHERE id=:OLD_id ";
}

std::string WebCkinSets::deleteSql() const
{
  return "DELETE FROM ckin_client_sets "
         "WHERE id=:OLD_id ";
}

std::list<std::string> WebCkinSets::dbSessionObjectNames() const
{
  return {"CKIN_CLIENT_SETS"};
}

void WebCkinSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  setRowId("id", status, newRow);
}

void WebCkinSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          const std::optional<CacheTable::Row>& newRow) const
{
    HistoryTable("ckin_client_sets").synchronize(getRowId("id", oldRow, newRow));
}

std::string WebCkinSets::selectSqlAddCondition() const
{
  return "";
}

//AirlineWebCkinSets

std::string AirlineWebCkinSets::selectSqlAddCondition() const
{
  return "airline=:airline AND ";
}

//KioskCkinSets

bool KioskCkinSets::userDependence() const
{
  return true;
}

std::string KioskCkinSets::insertSql() const
{
  return "INSERT INTO ckin_client_sets( "
         "  id,client_type,desk_grp_id,flt_no,airline,airp_dep,pr_permit,pr_waitlist,pr_tckin,pr_upd_stage "
         ") VALUES("
         "  :id,'KIOSK',:desk_grp_id,:flt_no,:airline,:airp_dep,:pr_permit,0,:pr_tckin,:pr_upd_stage "
         ") ";
}

std::string KioskCkinSets::updateSql() const
{
  return "UPDATE ckin_client_sets "
         "SET desk_grp_id=:desk_grp_id,airline=:airline,flt_no=:flt_no,airp_dep=:airp_dep,pr_permit=:pr_permit, "
         "    pr_waitlist=:pr_waitlist,pr_tckin=:pr_tckin,pr_upd_stage=:pr_upd_stage "
         "WHERE id=:OLD_id ";
}

std::string KioskCkinSets::deleteSql() const
{
  return "DELETE FROM ckin_client_sets "
         "WHERE id=:OLD_id ";
}

std::list<std::string> KioskCkinSets::dbSessionObjectNames() const
{
  return {"CKIN_CLIENT_SETS"};
}

void KioskCkinSets::onSelectOrRefresh(const TParams& sqlParams, SelectedRows& rows) const
{
  DB::TCachedQuery Qry(
              PgOra::getROSession("CKIN_CLIENT_SETS"),
              "SELECT id,airline,airp_dep,flt_no,pr_permit,pr_waitlist,pr_tckin,pr_upd_stage, "
              "       desk_grp_id, desk_grp_id AS desk_grp_view "
              "FROM ckin_client_sets "
              "WHERE client_type='KIOSK' AND desk_grp_id IS NOT NULL AND "
              + selectSqlAddCondition() +
              "      " + getSQLFilter("ckin_client_sets.airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
              "      " + getSQLFilter("ckin_client_sets.airp_dep", AccessControl::PermittedAirportsOrNull) + " "
              "ORDER BY airp_dep,airline,desk_grp_view,ckin_client_sets.id ",
              STDLOG);

  bindAddParams(sqlParams, Qry.get());
  Qry.get().Execute();

  if (Qry.get().Eof) return;

  const int idxId = Qry.get().FieldIndex("id");
  const int idxAirline = Qry.get().FieldIndex("airline");
  const int idxAirpDep = Qry.get().FieldIndex("airp_dep");
  const int idxFltNo = Qry.get().FieldIndex("flt_no");
  const int idxPrPermit = Qry.get().FieldIndex("pr_permit");
  const int idxPrWaitlist = Qry.get().FieldIndex("pr_waitlist");
  const int idxPrTckin = Qry.get().FieldIndex("pr_tckin");
  const int idxPrUpdStage = Qry.get().FieldIndex("pr_upd_stage");
  const int idxDeskGrpId = Qry.get().FieldIndex("desk_grp_id");
  const int idxDeskGrpView = Qry.get().FieldIndex("desk_grp_view");

  ViewAccess<DeskGrpId_t> validatorViewAccess;

  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (!Qry.get().FieldIsNULL(idxDeskGrpId) &&
        !validatorViewAccess.check(DeskGrpId_t(Qry.get().FieldAsInteger(idxDeskGrpId))))
    {
        continue;
    }

    rows.setFromInteger(Qry.get(), idxId)
        .setFromString(Qry.get(), idxAirline)
        .setFromString(Qry.get(), idxAirpDep)
        .setFromInteger(Qry.get(), idxFltNo)
        .setFromInteger(Qry.get(), idxPrPermit)
        .setFromInteger(Qry.get(), idxPrWaitlist)
        .setFromInteger(Qry.get(), idxPrTckin)
        .setFromInteger(Qry.get(), idxPrUpdStage)
        .setFromInteger(Qry.get(), idxDeskGrpId)
        .setFromInteger(Qry.get(), idxDeskGrpView)
        .addRow();
  }
}

void KioskCkinSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);
  checkDeskGrpAccess("desk_grp_id", true, oldRow, newRow);

  setRowId("id", status, newRow);
}

void KioskCkinSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("ckin_client_sets").synchronize(getRowId("id", oldRow, newRow));
}

std::string KioskCkinSets::selectSqlAddCondition() const
{
  return "";
}

void KioskCkinSets::bindAddParams(const TParams& sqlParams, DB::TQuery& Qry) const
{
  ;
}

//AirlineKioskCkinSets

std::string AirlineKioskCkinSets::selectSqlAddCondition() const
{
  return "ckin_client_sets.airline=:airline AND ";
}

void AirlineKioskCkinSets::bindAddParams(const TParams& sqlParams, DB::TQuery& Qry) const
{
  CreateVariablesFromParams({"airline"}, sqlParams, Qry);
}

//Pacts

bool Pacts::userDependence() const
{
  return true;
}

std::list<std::string> Pacts::dbSessionObjectNames() const
{
  return {"PACTS"};
}

std::string Pacts::selectSql() const {
  return
   "SELECT id, airline, airp, first_date, " + lastDateSelectSQL("PACTS") + ", descr "
   "FROM pacts "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirports) +
   "ORDER BY airline, airp, first_date, last_date, descr ";
}

std::string Pacts::deleteSql() const
{
  return "DELETE FROM pacts WHERE id=:OLD_id ";
}

bool Pacts::insertImplemented() const
{
  return true;
}

bool Pacts::updateImplemented() const
{
  return true;
}

void Pacts::onApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row>& oldRow,
                                 const std::optional<Row>& newRow) const
{
  if (status == usInserted)
  {
    const CacheTable::Row& row = newRow.value();
    insertPact(row.getAsInteger_ThrowOnEmpty("id"),
               row.getAsDateTime_ThrowOnEmpty("first_date"),
               row.getAsDateTime("last_date", ASTRA::NoExists),
               row.getAsString("airline"),
               row.getAsString("airp"),
               row.getAsString("descr"));
  }

  if (status == usModified)
  {
    updatePact(oldRow.value().getAsInteger_ThrowOnEmpty("id"),
               newRow.value().getAsDateTime_ThrowOnEmpty("first_date"),
               newRow.value().getAsDateTime("last_date", ASTRA::NoExists),
               newRow.value().getAsString("airline"),
               newRow.value().getAsString("airp"));
  }
}

void Pacts::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void Pacts::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                    const std::optional<CacheTable::Row>& oldRow,
                                    const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("pacts").synchronize(getRowId("id", oldRow, newRow));
}

//BpTypes

bool BpTypes::userDependence() const
{
  return false;
}

std::string BpTypes::selectSql() const
{
  return "SELECT code,airline,airp,name,id "
         "FROM bp_types "
         "WHERE op_type='" + operType() + "' "
         "ORDER BY code ";
}

std::string BpTypes::insertSql() const
{
  return "INSERT INTO bp_types(code, op_type, airline, airp, name, id) "
         "VALUES(:code, '" + operType() + "', :airline, :airp, :name, :id) ";
}

std::string BpTypes::deleteSql() const
{
  return "DELETE FROM bp_types "
         "WHERE code = :OLD_code AND op_type='" + operType() + "' ";
}

std::list<std::string> BpTypes::dbSessionObjectNames() const
{
  return {"BP_TYPES"};
}

void BpTypes::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                       const std::optional<CacheTable::Row>& oldRow,
                                       std::optional<CacheTable::Row>& newRow) const
{
  setRowId("id", status, newRow);
}

void BpTypes::afterApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row>& oldRow, const std::optional<Row>& newRow) const
{
  HistoryTable("bp_types").synchronize(getRowId("id", oldRow, newRow));
}

//BpModels

bool BpModels::userDependence() const
{
  return false;
}

std::string BpModels::selectSql() const
{
  return "SELECT bp_models.form_type code, bp_models.dev_model, bp_models.fmt_type, "
         "       prn_forms.name form_name, bp_models.id, bp_models.version, prn_form_vers.descr "
         "FROM bp_models, prn_forms, prn_form_vers "
         "WHERE bp_models.id = prn_forms.id "
         "AND bp_models.id = prn_form_vers.id "
         "AND bp_models.version = prn_form_vers.version "
         "AND bp_models.op_type = '" + operType() + "' "
         "ORDER BY bp_models.dev_model, bp_models.fmt_type, prn_forms.name, bp_models.version ";
}

std::string BpModels::insertSql() const
{
  return "INSERT INTO bp_models(form_type, op_type, dev_model, fmt_type, id, version) "
         "VALUES (:code, '" + operType() + "', :dev_model, :fmt_type, :id, :version) ";
}

std::string BpModels::updateSql() const
{
  return "UPDATE bp_models "
         "SET id = :id, version = :version "
         "WHERE form_type = :OLD_code "
         "AND op_type = '" + operType() + "' "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::string BpModels::deleteSql() const
{
  return "DELETE FROM bp_models "
         "WHERE form_type = :OLD_code "
         "AND op_type = '" + operType() + "' "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::list<std::string> BpModels::dbSessionObjectNamesForRead() const
{
  return {"BP_MODELS","PRN_FORMS","PRN_FORM_VERS"};
}

std::list<std::string> BpModels::dbSessionObjectNames() const
{
  return {"BP_MODELS"};
}

//BpBlankList

bool BpBlankList::userDependence() const
{
  return false;
}

std::string BpBlankList::selectSql() const
{
  return "SELECT id, version, form_type, dev_model, fmt_type "
         "FROM bp_models "
         "WHERE op_type='" + operType() + "' ";
}

std::string BpBlankList::insertSql() const
{
  return "INSERT INTO bp_models(id, version, form_type, op_type, dev_model, fmt_type) "
         "VALUES(:id, :version, :form_type, '" + operType() + "', :dev_model, :fmt_type) ";
}

std::string BpBlankList::updateSql() const
{
  return "UPDATE bp_models "
         "SET form_type = :form_type, "
         "    dev_model = :dev_model, "
         "    fmt_type = :fmt_type "
         "WHERE form_type = :OLD_form_type "
         "AND op_type='" + operType() + "' "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::string BpBlankList::deleteSql() const
{
  return "DELETE FROM bp_models "
         "WHERE form_type = :OLD_form_type "
         "AND op_type='" + operType() + "' "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::list<std::string> BpBlankList::dbSessionObjectNames() const
{
  return {"BP_MODELS"};
}

//BrModels

bool BrModels::userDependence() const
{
  return false;
}

std::string BrModels::selectSql() const
{
  return "SELECT br_models.form_type code, br_models.dev_model, br_models.fmt_type, "
         "       prn_forms.name form_name, br_models.id, br_models.version, prn_form_vers.descr "
         "FROM br_models, prn_forms, prn_form_vers "
         "WHERE br_models.id = prn_forms.id "
         "AND br_models.id = prn_form_vers.id "
         "AND br_models.version = prn_form_vers.version "
         "ORDER BY br_models.dev_model, br_models.fmt_type, prn_forms.name, br_models.version ";
}

std::string BrModels::insertSql() const
{
  return "INSERT INTO br_models(form_type, dev_model, fmt_type, id, version) "
         "VALUES (:code, :dev_model, :fmt_type, :id, :version) ";
}

std::string BrModels::updateSql() const
{
  return "UPDATE br_models "
         "SET id = :id, version = :version "
         "WHERE form_type = :OLD_code "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::string BrModels::deleteSql() const
{
  return "DELETE FROM br_models "
         "WHERE form_type = :OLD_code "
         "AND dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type ";
}

std::list<std::string> BrModels::dbSessionObjectNamesForRead() const
{
  return {"BR_MODELS","PRN_FORMS","PRN_FORM_VERS"};
}

std::list<std::string> BrModels::dbSessionObjectNames() const
{
  return {"BR_MODELS"};
}

//BrBlankList

bool BrBlankList::userDependence() const
{
  return false;
}

std::string BrBlankList::selectSql() const
{
  return "SELECT id, version, form_type, dev_model, fmt_type "
         "FROM br_models ";
}

std::string BrBlankList::insertSql() const
{
  return "INSERT INTO br_models(id, version, form_type, dev_model, fmt_type) "
         "VALUES(:id, :version, :form_type, :dev_model, :fmt_type) ";
}

std::string BrBlankList::updateSql() const
{
  return "UPDATE br_models "
         "SET form_type = :form_type, "
         "    dev_model = :dev_model, "
         "    fmt_type = :fmt_type "
         "WHERE dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type "
         "AND form_type = :OLD_form_type ";
}

std::string BrBlankList::deleteSql() const
{
  return "DELETE FROM br_models "
         "WHERE dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type "
         "AND form_type = :OLD_form_type ";
}

std::list<std::string> BrBlankList::dbSessionObjectNames() const
{
  return {"BR_MODELS"};
}

//BtModels

bool BtModels::userDependence() const
{
  return false;
}

std::string BtModels::selectSql() const
{
  return "SELECT bt_models.form_type code, bt_models.dev_model, bt_models.fmt_type, bt_models.num, "
         "       prn_forms.name form_name, bt_models.id, bt_models.version, prn_form_vers.descr "
         "FROM bt_models, prn_forms, prn_form_vers "
         "WHERE bt_models.id = prn_forms.id "
         "AND bt_models.id = prn_form_vers.id "
         "AND bt_models.version = prn_form_vers.version "
         "ORDER BY bt_models.dev_model, bt_models.fmt_type, prn_forms.name, bt_models.version ";
}

std::string BtModels::insertSql() const
{
  return "INSERT INTO bt_models(form_type, dev_model, fmt_type, num, id, version) "
         "VALUES (:code, :dev_model, :fmt_type, :num, :id, :version) ";
}

std::string BtModels::updateSql() const
{
  return "UPDATE bt_models "
         "SET id = :id, version = :version "
         "WHERE form_type = :OLD_code "
         "AND dev_model = :OLD_dev_model "
         "AND num = :OLD_num "
         "AND fmt_type = :OLD_fmt_type ";
}

std::string BtModels::deleteSql() const
{
  return "DELETE FROM bt_models "
         "WHERE form_type = :OLD_code "
         "AND dev_model = :OLD_dev_model "
         "AND num = :OLD_num "
         "AND fmt_type = :OLD_fmt_type ";
}

std::list<std::string> BtModels::dbSessionObjectNamesForRead() const
{
  return {"BT_MODELS","PRN_FORMS","PRN_FORM_VERS"};
}

std::list<std::string> BtModels::dbSessionObjectNames() const
{
  return {"BT_MODELS"};
}

//BtBlankList

bool BtBlankList::userDependence() const
{
  return false;
}

std::string BtBlankList::selectSql() const
{
  return "SELECT id, version, form_type, dev_model, fmt_type, num "
         "FROM bt_models ";
}

std::string BtBlankList::insertSql() const
{
  return "INSERT INTO bt_models(id, version, form_type, dev_model, fmt_type, num) "
         "VALUES(:id, :version, :form_type, :dev_model, :fmt_type, :num) ";
}

std::string BtBlankList::updateSql() const
{
  return "UPDATE bt_models "
         "SET form_type = :form_type, "
         "    dev_model = :dev_model, "
         "    fmt_type = :fmt_type, "
         "    num = :num "
         "WHERE dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type "
         "AND form_type = :OLD_form_type "
         "AND num = :OLD_num ";
}

std::string BtBlankList::deleteSql() const
{
  return "DELETE FROM bt_models "
         "WHERE dev_model = :OLD_dev_model "
         "AND fmt_type = :OLD_fmt_type "
         "AND form_type = :OLD_form_type "
         "AND num = :OLD_num ";
}

std::list<std::string> BtBlankList::dbSessionObjectNames() const
{
  return {"BT_MODELS"};
}

//PrnFormVers

bool PrnFormVers::userDependence() const
{
  return false;
}

std::string PrnFormVers::selectSql() const
{
  return "SELECT id, version, descr, (CASE WHEN read_only = 0 THEN 1 ELSE 0 END) read_only "
         "FROM prn_form_vers "
         "ORDER BY id, version, descr ";
}

std::string PrnFormVers::insertSql() const
{
  return "INSERT INTO prn_form_vers(id, version, descr, read_only) "
         "VALUES(:id, :version, :descr, (CASE WHEN :read_only = 0 THEN 1 ELSE 0 END)) ";
}

std::string PrnFormVers::updateSql() const
{
  return "UPDATE prn_form_vers "
         "SET descr = :descr, "
         "    read_only = (CASE WHEN :read_only = 0 THEN 1 ELSE 0 END) "
         "WHERE id = :id AND version = :version ";
}

std::string PrnFormVers::deleteSql() const
{
  return "DELETE FROM prn_form_vers "
         "WHERE id = :OLD_id AND version = :OLD_version ";
}

std::list<std::string> PrnFormVers::dbSessionObjectNames() const
{
  return {"PRN_FORM_VERS"};
}

//PrnForms

bool PrnForms::userDependence() const
{
  return false;
}

std::string PrnForms::selectSql() const
{
  return "SELECT id, op_type, name, fmt_type "
         "FROM prn_forms "
         "ORDER BY name, fmt_type";
}

std::string PrnForms::insertSql() const
{
  return "INSERT INTO prn_forms(id, name, fmt_type, op_type) "
         "VALUES(:id, :name, :fmt_type, :op_type) ";
}

std::string PrnForms::updateSql() const
{
  return "UPDATE prn_forms "
         "SET name = :name, "
         "    fmt_type = :fmt_type, "
         "    op_type = :op_type "
         "WHERE id = :OLD_id ";
}

std::string PrnForms::deleteSql() const
{
  return "DELETE FROM prn_forms "
         "WHERE id = :OLD_id ";
}

std::list<std::string> PrnForms::dbSessionObjectNames() const
{
  return {"PRN_FORMS"};
}

void PrnForms::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<Row>& oldRow,
                                        std::optional<Row>& newRow) const
{
  setRowId("id", status, newRow);
}

//TripBt

bool TripBt::userDependence() const
{
  return false;
}

std::string TripBt::selectSql() const
{
  return "";
}

std::string TripBt::insertSql() const
{
  return "INSERT INTO trip_bt(point_id,tag_type) "
         "VALUES(:point_id,:bt_code) ";
}

std::string TripBt::updateSql() const
{
  return "UPDATE trip_bt "
         "SET tag_type=:bt_code "
         "WHERE point_id=:OLD_point_id ";
}

std::string TripBt::deleteSql() const
{
  return "DELETE FROM trip_bt "
         "WHERE point_id=:OLD_point_id ";
}

std::list<std::string> TripBt::dbSessionObjectNames() const
{
  return {"TRIP_BT"};
}

void TripBt::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery tagsQry(PgOra::getROSession("TAG_TYPES"), STDLOG);
  tagsQry.SQLText =
          "SELECT code, name "
          "FROM tag_types ";
  tagsQry.Execute();
  std::map<std::string,std::string> tagsMap;
  for(; !tagsQry.Eof; tagsQry.Next()) {
      const std::string code = tagsQry.FieldAsString("code");
      const std::string name = tagsQry.FieldAsString("name");
      tagsMap[code] = name;
  }

  DB::TQuery btQry(PgOra::getROSession("TRIP_BT"), STDLOG);
  btQry.SQLText =
          "SELECT point_id, trip_bt.tag_type AS bt_code "
          "FROM trip_bt "
          "WHERE point_id=:point_id ";

  CreateVariablesFromParams({"point_id"}, sqlParams, btQry);
  btQry.Execute();
  if (btQry.Eof) return;
  const int idxPointId = btQry.FieldIndex("point_id");
  const int idxBtCode = btQry.FieldIndex("bt_code");
  for(; !btQry.Eof; btQry.Next())
  {
    const std::string bt_code = btQry.FieldAsString("bt_code");
    auto pos = tagsMap.find(bt_code);
    if (pos == tagsMap.end()) {
        continue;
    }
    const std::string& bt_name = pos->second;
    rows.setFromInteger(btQry, idxPointId)
        .setFromString(btQry, idxBtCode)
        .setFromString(bt_name)
        .addRow();
  }
}

//BrandFares

bool BrandFares::userDependence() const
{
  return true;
}

std::string BrandFares::selectSql() const
{
  return "SELECT brand_fares.id, brand_fares.airline, brand_fares.fare_basis, brand_fares.brand, "
         "       brands.id AS brand_view, brand_fares.sale_first_date, "
         "       " + lastDateSelectSQL("BRAND_FARES", "sale_last_date") + " "
         "FROM brand_fares, brands "
         "WHERE brand_fares.airline=brands.airline AND "
         "      brand_fares.brand=brands.code AND "
         "      " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY brand_fares.airline, brand_fares.brand, brand_fares.fare_basis ";
}

std::string BrandFares::deleteSql() const
{
  return "DELETE FROM brand_fares "
         "WHERE id = :OLD_id ";
}

bool BrandFares::insertImplemented() const
{
  return true;
}

bool BrandFares::updateImplemented() const
{
  return true;
}

void BrandFares::onApplyingRowChanges(const TCacheUpdateStatus status,
                                      const std::optional<Row>& oldRow,
                                      const std::optional<Row>& newRow) const
{
  if (status == usInserted)
  {
    const CacheTable::Row& row = newRow.value();
    insertBrandFare(row.getAsInteger_ThrowOnEmpty("id"),
                    row.getAsDateTime_ThrowOnEmpty("sale_first_date"),
                    row.getAsDateTime("sale_last_date", ASTRA::NoExists),
                    row.getAsString("airline"),
                    row.getAsString("fare_basis"),
                    row.getAsString("brand"));
  }

  if (status == usModified)
  {
    updateBrandFare(oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                    newRow.value().getAsDateTime_ThrowOnEmpty("sale_first_date"),
                    newRow.value().getAsDateTime("sale_last_date", ASTRA::NoExists),
                    newRow.value().getAsString("airline"),
                    newRow.value().getAsString("fare_basis"),
                    newRow.value().getAsString("brand"));
  }
}

std::list<std::string> BrandFares::dbSessionObjectNames() const
{
  return {"BRAND_FARES"};
}

std::list<std::string> BrandFares::dbSessionObjectNamesForRead() const
{
  return {"BRAND_FARES","BRANDS"};
}

void BrandFares::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  if (newRow)
  {
    const std::string fareBasis = newRow.value().getAsString_ThrowOnEmpty("fare_basis");
    checkInvalidSymbolInName(fareBasis, false /*latinOnly*/, "-/*", "BRAND_FARES", "FARE_BASIS");
  }

  setRowId("id", status, newRow);
}

void BrandFares::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("brand_fares").synchronize(getRowId("id", oldRow, newRow));
}

//BiPrintTypes

bool BiPrintTypes::userDependence() const
{
  return false;
}

std::string BiPrintTypes::selectSql() const
{
  return "SELECT code, name, name_lat "
         "FROM bi_print_types "
         "ORDER BY priority";
}

std::list<std::string> BiPrintTypes::dbSessionObjectNames() const
{
  return {"BI_PRINT_TYPES"};
}

//BiPrintRules

bool BiPrintRules::userDependence() const
{
  return true;
}

std::string BiPrintRules::selectSql() const
{
  return "SELECT rules.id, rules.airline, rules.brand_airline, rules.brand_code, brands.id AS brand_view, rules.fqt_airline, rules.fqt_tier_level, "
         "       rules.aircode, rules.rem_code, rules.class, rules.subclass, rules.rfisc, rules.print_type, rules.pr_denial "
         "FROM bi_print_rules rules "
         "LEFT OUTER JOIN brands "
         "ON rules.brand_airline = brands.airline AND rules.brand_code = brands.code "
         "WHERE " + getSQLFilter("rules.airline", AccessControl::PermittedAirlines) +
         "ORDER BY rules.airline, rules.id ";
}

std::string BiPrintRules::insertSql() const
{
  return "INSERT INTO bi_print_rules( "
         "id, airline, brand_airline, brand_code, fqt_airline, fqt_tier_level, aircode, rem_code, class, subclass, rfisc, print_type, pr_denial "
         ") VALUES ("
         ":id, :airline, :brand_airline, :brand_code, :fqt_airline, :fqt_tier_level, :aircode, :rem_code, :class, :subclass, :rfisc, :print_type, :pr_denial "
         ") ";
}

std::string BiPrintRules::updateSql() const
{
  return "UPDATE bi_print_rules "
         "SET airline=:airline, brand_airline=:brand_airline, brand_code=:brand_code, "
         "    fqt_airline=:fqt_airline, fqt_tier_level=:fqt_tier_level, "
         "    aircode=:aircode, rem_code=:rem_code, class=:class, subclass=:subclass, "
         "    rfisc=:rfisc, print_type=:print_type, pr_denial=:pr_denial "
         "WHERE id=:OLD_id ";
}

std::string BiPrintRules::deleteSql() const
{
  return "DELETE FROM bi_print_rules "
         "WHERE id=:OLD_id; ";
}

std::list<std::string> BiPrintRules::dbSessionObjectNames() const
{
  return {"BI_PRINT_RULES"};
}

std::list<std::string> BiPrintRules::dbSessionObjectNamesForRead() const
{
  return {"BI_PRINT_RULES","BRANDS"};
}

void BiPrintRules::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<Row>& oldRow,
                                            std::optional<Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  if (newRow) {
    const std::string airline = newRow.value().getAsString("airline");
    const std::string brand_airline = newRow.value().getAsString("brand_airline");
    if (!brand_airline.empty() && brand_airline != airline) {
      throw UserException("MSG.BRAND_DOES_NOT_MEET_AIRLINE");
    }
  }

  setRowId("id", status, newRow);
}

void BiPrintRules::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<Row>& oldRow,
                                           const std::optional<Row>& newRow) const
{
  HistoryTable("bi_print_rules").synchronize(getRowId("id", oldRow, newRow));
}

//RfiscRates

bool RfiscRates::userDependence() const
{
  return true;
}

std::string RfiscRates::selectSql() const
{
  return "SELECT rfisc_rates.id, rfisc_rates.airline, rfisc_rates.airp_dep, rfisc_rates.airp_arv, "
         "       rfisc_rates.brand, rfisc_rates.sale_first_date, "
         "       " + lastDateSelectSQL("RFISC_RATES", "sale_last_date") + ", "
         "       rfisc_rates.rfisc, rfisc_rates.rate, rfisc_rates.rate_cur, "
         "       brands.id AS brand_view, rfisc_rates.airline AS rfisc_airline, "
         "       rfisc_rates.airp_dep AS rfisc_airp_dep, rfisc_rates.airp_arv AS rfisc_airp_arv "
         "FROM rfisc_rates, brands "
         "WHERE rfisc_rates.airline=brands.airline AND "
         "      rfisc_rates.brand=brands.code AND "
         "      " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " AND "
         "      " + getSQLFilter("airp_dep",  AccessControl::PermittedAirportsOrNull) + " AND "
         "      " + getSQLFilter("airp_arv",  AccessControl::PermittedAirportsOrNull) + " "
         "ORDER BY rfisc_rates.airline, rfisc_rates.airp_dep, rfisc_rates.airp_arv, rfisc_rates.brand, "
         "         rfisc_rates.rfisc, rfisc_rates.sale_first_date, rfisc_rates.sale_last_date NULLS LAST ";
}

std::string RfiscRates::deleteSql() const
{
  return "DELETE FROM rfisc_rates "
         "WHERE id = :OLD_id ";
}

bool RfiscRates::insertImplemented() const
{
  return true;
}

bool RfiscRates::updateImplemented() const
{
  return true;
}

void RfiscRates::onApplyingRowChanges(const TCacheUpdateStatus status,
                                      const std::optional<Row>& oldRow,
                                      const std::optional<Row>& newRow) const
{
  if (status == usInserted)
  {
    const CacheTable::Row& row = newRow.value();
    insertRfiscRates(row.getAsInteger_ThrowOnEmpty("id"),
                     row.getAsDateTime_ThrowOnEmpty("sale_first_date"),
                     row.getAsDateTime("sale_last_date", ASTRA::NoExists),
                     row.getAsString("airline"),
                     row.getAsString("airp_dep"),
                     row.getAsString("airp_arv"),
                     row.getAsString("brand"),
                     row.getAsString("rfisc_airline"),
                     row.getAsString("rfisc"),
                     row.getAsInteger_ThrowOnEmpty("rate"),
                     row.getAsString("rate_cur"));
  }

  if (status == usModified)
  {
    updateRfiscRates(oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                     newRow.value().getAsDateTime_ThrowOnEmpty("sale_first_date"),
                     newRow.value().getAsDateTime("sale_last_date", ASTRA::NoExists),
                     newRow.value().getAsString("airline"),
                     newRow.value().getAsString("airp_dep"),
                     newRow.value().getAsString("airp_arv"),
                     newRow.value().getAsString("brand"),
                     newRow.value().getAsString("rfisc_airline"),
                     newRow.value().getAsString("rfisc"),
                     newRow.value().getAsInteger_ThrowOnEmpty("rate"),
                     newRow.value().getAsString("rate_cur"));
  }
}

std::list<std::string> RfiscRates::dbSessionObjectNames() const
{
  return {"RFISC_RATES"};
}

std::list<std::string> RfiscRates::dbSessionObjectNamesForRead() const
{
  return {"RFISC_RATES","BRANDS"};
}

void RfiscRates::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    const std::string airp_dep = newRow.value().getAsString("airp_dep");
    const std::string airp_arv = newRow.value().getAsString("airp_arv");
    if ((!airp_dep.empty() && airp_arv.empty())
        || (airp_dep.empty() && !airp_arv.empty()))
    {
      throw UserException("MSG.AIRPS_ISNULL_OR_REQUIRED");
    }
  }

  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);
  checkAirportAccess("airp_arv", oldRow, newRow);

  if (newRow)
  {
    const std::string rfisc_airline = newRow.value().getAsString("rfisc_airline");
    const std::string airline = newRow.value().getAsString("airline");
    if (!rfisc_airline.empty() && rfisc_airline != airline) {
        throw UserException("MSG.BRAND_DOES_NOT_MEET_RFISC");
    }
  }

  setRowId("id", status, newRow);
}

void RfiscRates::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rfisc_rates").synchronize(getRowId("id", oldRow, newRow));
}

//RfiscRatesSelfCkin

bool RfiscRatesSelfCkin::userDependence() const
{
  return true;
}

std::string RfiscRatesSelfCkin::selectSql() const
{
  return "SELECT id, airline, airp_dep, airp_arv, craft, rfisc, rate, rate_cur "
         "FROM rfisc_rates_self_ckin "
         "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " AND "
         "      " + getSQLFilter("airp_dep",  AccessControl::PermittedAirportsOrNull) + " "
         "ORDER BY airline, rfisc ";
}

std::string RfiscRatesSelfCkin::insertSql() const
{
  return "INSERT INTO rfisc_rates_self_ckin(id, airline, airp_dep, airp_arv, craft, rfisc, rate, rate_cur) "
         "VALUES(:id, :airline, :airp_dep, :airp_arv, :craft, :rfisc, :rate, :rate_cur) ";
}

std::string RfiscRatesSelfCkin::updateSql() const
{
  return "UPDATE rfisc_rates_self_ckin "
         "SET airline=:airline, airp_dep=:airp_dep, airp_arv=:airp_arv, craft=:craft, "
         "    rfisc=:rfisc, rate=:rate, rate_cur=:rate_cur "
         "WHERE id = :OLD_id ";
}

std::string RfiscRatesSelfCkin::deleteSql() const
{
  return "DELETE FROM rfisc_rates_self_ckin "
         "WHERE id = :OLD_id ";
}

std::list<std::string> RfiscRatesSelfCkin::dbSessionObjectNames() const
{
  return {"RFISC_RATES_SELF_CKIN"};
}

void RfiscRatesSelfCkin::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                                  const std::optional<CacheTable::Row>& oldRow,
                                                  std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  setRowId("id", status, newRow);
}

void RfiscRatesSelfCkin::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                                 const std::optional<CacheTable::Row>& oldRow,
                                                 const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rfisc_rates_self_ckin").synchronize(getRowId("id", oldRow, newRow));
}

//RficTypes

bool RficTypes::userDependence() const
{
  return true;
}

std::string RficTypes::selectSql() const
{
  return "SELECT code,name,name_lat "
         "FROM rfic_types "
         "ORDER BY code ";
}

std::list<std::string> RficTypes::dbSessionObjectNames() const
{
  return {"RFIC_TYPES"};
}

//RfiscTypes

bool RfiscTypes::userDependence() const
{
  return true;
}

std::string RfiscTypes::selectSql() const
{
  return "SELECT airline, code "
         "FROM rfisc_comp_props "
         "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " "
         "ORDER BY airline, code";
}

std::list<std::string> RfiscTypes::dbSessionObjectNames() const
{
  return {"RFISC_COMP_PROPS"};
}

//RfiscSets

bool RfiscSets::userDependence() const
{
  return true;
}

std::string RfiscSets::selectSql() const
{
  return "SELECT id, airline, flt_no, airp_dep, rfic, rfisc, auto_checkin "
         "FROM rfisc_sets "
         "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
         "      " + getSQLFilter("airp_dep",  AccessControl::PermittedAirportsOrNull) + " "
         "ORDER BY airline, rfic, id ";
}

std::string RfiscSets::insertSql() const
{
  return "INSERT INTO rfisc_sets(id, airline, flt_no, airp_dep, rfic, rfisc, auto_checkin) "
         "VALUES(:id, :airline, :flt_no, :airp_dep, :rfic, :rfisc, :auto_checkin) ";
}

std::string RfiscSets::updateSql() const
{
  return "UPDATE rfisc_sets "
         "SET airline=:airline, flt_no=:flt_no, airp_dep=:airp_dep, rfic=:rfic, "
         "    rfisc=:rfisc, auto_checkin=:auto_checkin "
         "WHERE id=:OLD_id ";
}

std::string RfiscSets::deleteSql() const
{
  return "DELETE FROM rfisc_sets "
         "WHERE id=:OLD_id ";
}

std::list<std::string> RfiscSets::dbSessionObjectNames() const
{
  return {"RFISC_SETS"};
}

void RfiscSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  if (newRow) {
      const std::string rfic = newRow.value().getAsString("rfic");
      if (rfic == "C") {
          throw UserException("MSG.FORBIDDEN_INSERT_RFIC",
                              LParams() << LParam("rfic", rfic));
      }
  }

  setRowId("id", status, newRow);
}

void RfiscSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rfisc_sets").synchronize(getRowId("id", oldRow, newRow));
}

//RfiscCompProps

bool RfiscCompProps::userDependence() const
{
  return true;
}

std::string RfiscCompProps::selectSql() const
{
  return "SELECT airline, code, rate_color, id, pr_prot_ckin "
         "FROM rfisc_comp_props "
         "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY airline, code ";
}

std::string RfiscCompProps::insertSql() const
{
  return "INSERT INTO rfisc_comp_props(airline, code, rate_color, id, pr_prot_ckin) "
         "VALUES(:airline, :code, :rate_color, :id, :pr_prot_ckin) ";
}

std::string RfiscCompProps::updateSql() const
{
  return "UPDATE rfisc_comp_props "
         "SET airline=:airline, code=:code, rate_color=:rate_color, pr_prot_ckin=:pr_prot_ckin "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::string RfiscCompProps::deleteSql() const
{
  return "DELETE FROM rfisc_comp_props "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::list<std::string> RfiscCompProps::dbSessionObjectNames() const
{
  return {"RFISC_COMP_PROPS"};
}

void RfiscCompProps::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                              const std::optional<CacheTable::Row>& oldRow,
                                              std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  setRowId("id", status, newRow);
}

void RfiscCompProps::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rfisc_comp_props").synchronize(getRowId("id", oldRow, newRow));
}

//RfiscBagProps

bool RfiscBagProps::userDependence() const
{
  return true;
}

std::string RfiscBagProps::selectSql() const
{
  return "SELECT airline, code, rem_code_lci, rem_code_ldm, priority, min_weight, max_weight, id "
         "FROM rfisc_bag_props "
         "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY priority NULLS LAST, code ";
}

std::string RfiscBagProps::insertSql() const
{
  return "INSERT INTO rfisc_bag_props(airline, code, rem_code_lci, rem_code_ldm, priority, min_weight, max_weight, id) "
         "VALUES(:airline, :code, :rem_code_lci, :rem_code_ldm, :priority, :min_weight, :max_weight, :id) ";
}

std::string RfiscBagProps::updateSql() const
{
  return "UPDATE rfisc_bag_props "
         "SET airline=:airline, code=:code, rem_code_lci = :rem_code_lci, "
         "rem_code_ldm = :rem_code_ldm, priority=:priority, min_weight=:min_weight, max_weight=:max_weight "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::string RfiscBagProps::deleteSql() const
{
  return "DELETE FROM rfisc_bag_props "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::list<std::string> RfiscBagProps::dbSessionObjectNames() const
{
  return {"RFISC_BAG_PROPS"};
}

void RfiscBagProps::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                             const std::optional<CacheTable::Row>& oldRow,
                                             std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  if (newRow) {
    checkRange(newRow.value().getAsInteger("min_weight"),
               newRow.value().getAsInteger("max_weight"),
               "RFISC_BAG_PROPS", "MAX_WEIGHT");
  }

  setRowId("id", status, newRow);
}

void RfiscBagProps::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rfisc_bag_props").synchronize(getRowId("id", oldRow, newRow));
}

//DcsServiceApplying

bool DcsServiceApplying::userDependence() const
{
  return true;
}

std::string DcsServiceApplying::selectSql() const
{
  return "SELECT a.id, a.airline, a.dcs_service, a.brand_airline, a.brand_code, brands.id AS brand_view, "
         "       a.fqt_airline, a.fqt_tier_level, a.class, a.rfisc, a.pr_denial "
         "FROM dcs_service_applying a "
         "LEFT OUTER JOIN brands "
         "ON a.brand_airline = brands.airline AND a.brand_code = brands.code "
         "LEFT OUTER JOIN dcs_actions ON a.dcs_service = dcs_actions.code "
         "WHERE " + getSQLFilter("a.airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY a.airline, dcs_actions.view_order, a.id ";
}

std::string DcsServiceApplying::insertSql() const
{
  return "INSERT INTO dcs_service_applying( "
         "  id, airline, dcs_service, brand_airline, brand_code, fqt_airline, fqt_tier_level, class, rfisc, pr_denial "
         ") VALUES ( "
         "  :id, :airline, :dcs_service, :brand_airline, :brand_code, :fqt_airline, :fqt_tier_level, :class, :rfisc, :pr_denial "
         ") ";
}

std::string DcsServiceApplying::updateSql() const
{
  return "UPDATE dcs_service_applying "
         "SET airline=:airline, dcs_service=:dcs_service, brand_airline=:brand_airline, brand_code=:brand_code, "
         "    fqt_airline=:fqt_airline, fqt_tier_level=:fqt_tier_level, class=:class, rfisc=:rfisc, pr_denial=:pr_denial "
         "WHERE id=:OLD_id ";
}

std::string DcsServiceApplying::deleteSql() const
{
  return "DELETE FROM dcs_service_applying "
         "WHERE id = :OLD_id ";
}

std::list<std::string> DcsServiceApplying::dbSessionObjectNames() const
{
  return {"DCS_SERVICE_APPLYING"};
}

std::list<std::string> DcsServiceApplying::dbSessionObjectNamesForRead() const
{
  return {"DCS_SERVICE_APPLYING","BRANDS","DCS_ACTIONS"};
}

void DcsServiceApplying::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                                  const std::optional<CacheTable::Row>& oldRow,
                                                  std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  if (newRow) {
      const std::string brand_airline = newRow.value().getAsString("brand_airline");
      const std::string airline = newRow.value().getAsString("airline");
      if (!brand_airline.empty() && brand_airline != airline) {
          throw UserException("MSG.BRAND_DOES_NOT_MEET_AIRLINE");
      }
  }

  setRowId("id", status, newRow);
}

void DcsServiceApplying::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                                 const std::optional<CacheTable::Row>& oldRow,
                                                 const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("dcs_service_applying").synchronize(getRowId("id", oldRow, newRow));
}

//DcsActions

bool DcsActions::userDependence() const
{
  return true;
}

std::string DcsActions::selectSql() const
{
  return "SELECT code, name, name_lat FROM dcs_actions "
         "WHERE code IN (" + codeList() + ") "
         "ORDER BY view_order ";
}

std::list<std::string> DcsActions::dbSessionObjectNames() const
{
  return {"DCS_ACTIONS"};
}

//DcsActions1

std::string DcsActions1::codeList() const
{
  return "'PRINT_BP_ON_DESK', 'CHG_SEAT_ON_DESK'";
}

//DcsActions2

std::string DcsActions2::codeList() const
{
  return "'CHECKIN_ON_DESK', 'BOARDING'";
}

//RemTxtSets

bool RemTxtSets::userDependence() const
{
  return true;
}

std::string RemTxtSets::selectSql() const
{
  return "SELECT r.id, r.airline, rfisc, brand_airline, brand_code, brands.id brand_view, "
         "       fqt_airline, fqt_tier_level, tag_index, text_length, text, pr_lat "
         "FROM rem_txt_sets r "
         "LEFT OUTER JOIN brands "
         "ON brand_airline = brands.airline AND brand_code = brands.code "
         "WHERE " + getSQLFilter("r.airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY r.airline, rfisc, brand_code, fqt_tier_level, tag_index ";
}

std::string RemTxtSets::insertSql() const
{
  return "INSERT INTO rem_txt_sets( "
         "  id, airline, rfisc, brand_airline, brand_code, fqt_airline, fqt_tier_level, "
         "  tag_index, text_length, text, pr_lat "
         ") VALUES ( "
         "  :id, :airline, :rfisc, :brand_airline, :brand_code, :fqt_airline, :fqt_tier_level, "
         "  :tag_index, :text_length, :text, :pr_lat "
         ") ";
}

std::string RemTxtSets::updateSql() const
{
  return "UPDATE rem_txt_sets SET "
         "  airline = :airline, "
         "  rfisc = :rfisc, "
         "  brand_airline = :brand_airline, "
         "  brand_code = :brand_code, "
         "  fqt_airline = :fqt_airline, "
         "  fqt_tier_level = :fqt_tier_level, "
         "  tag_index = :tag_index, "
         "  text_length = :text_length, "
         "  text = :text, "
         "  pr_lat = :pr_lat "
         "WHERE id = :old_id ";
}

std::string RemTxtSets::deleteSql() const
{
  return "DELETE FROM rem_txt_sets "
         "WHERE id = :OLD_id ";
}

std::list<std::string> RemTxtSets::dbSessionObjectNames() const
{
  return {"REM_TXT_SETS"};
}

std::list<std::string> RemTxtSets::dbSessionObjectNamesForRead() const
{
  return {"REM_TXT_SETS","BRANDS"};
}

void RemTxtSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  if (newRow) {
      const std::string brand_airline = newRow.value().getAsString("brand_airline");
      const std::string airline = newRow.value().getAsString("airline");
      if (!brand_airline.empty() && brand_airline != airline) {
          throw UserException("MSG.BRAND_DOES_NOT_MEET_AIRLINE");
      }
  }

  setRowId("id", status, newRow);
}

void RemTxtSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("rem_txt_sets").synchronize(getRowId("id", oldRow, newRow));
}

//CustomAlarmSets

bool CustomAlarmSets::userDependence() const
{
  return true;
}

std::string CustomAlarmSets::selectSql() const
{
  return "SELECT r.id, r.airline, rfisc, rfisc_tlg, brand_airline, brand_code, "
         "       brands.id brand_view, fqt_airline, fqt_tier_level, alarm "
         "FROM custom_alarm_sets r "
         "LEFT OUTER JOIN brands "
         "ON brand_airline = brands.airline AND brand_code = brands.code "
         "WHERE " + getSQLFilter("r.airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY r.airline, brand_code, fqt_airline, fqt_tier_level, rfisc, rfisc_tlg ";
}

std::string CustomAlarmSets::insertSql() const
{
  return "INSERT INTO custom_alarm_sets( "
         "  id, airline, rfisc, rfisc_tlg, brand_airline, brand_code, fqt_airline, fqt_tier_level, alarm "
         ") VALUES ( "
         "  :id, :airline, :rfisc, :rfisc_tlg, :brand_airline, :brand_code, :fqt_airline, :fqt_tier_level, :alarm "
         ") ";
}

std::string CustomAlarmSets::updateSql() const
{
  return "UPDATE custom_alarm_sets SET "
         "  airline = :airline, "
         "  rfisc = :rfisc, "
         "  rfisc_tlg = :rfisc_tlg, "
         "  brand_airline = :brand_airline, "
         "  brand_code = :brand_code, "
         "  fqt_airline = :fqt_airline, "
         "  fqt_tier_level = :fqt_tier_level, "
         "  alarm = :alarm "
         "WHERE id = :old_id ";
}

std::string CustomAlarmSets::deleteSql() const
{
  return "DELETE FROM custom_alarm_sets "
         "WHERE id = :OLD_id ";
}

std::list<std::string> CustomAlarmSets::dbSessionObjectNames() const
{
  return {"CUSTOM_ALARM_SETS"};
}

std::list<std::string> CustomAlarmSets::dbSessionObjectNamesForRead() const
{
  return {"CUSTOM_ALARM_SETS","BRANDS"};
}

void CustomAlarmSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  if (newRow) {
      const std::string brand_airline = newRow.value().getAsString("brand_airline");
      const std::string airline = newRow.value().getAsString("airline");
      if (!brand_airline.empty() && brand_airline != airline) {
          throw UserException("MSG.BRAND_DOES_NOT_MEET_AIRLINE");
      }
  }

  setRowId("id", status, newRow);
}

void CustomAlarmSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("custom_alarm_sets").synchronize(getRowId("id", oldRow, newRow));
}

//CustomAlarmTypes

bool CustomAlarmTypes::userDependence() const
{
  return true;
}

std::string CustomAlarmTypes::selectSql() const
{
  return "SELECT id,airline,name,name_lat,pr_del,tid "
         "FROM custom_alarm_types "
         "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " "
         "ORDER BY airline,name,name_lat ";
}

std::string CustomAlarmTypes::refreshSql() const
{
  return "";
}

std::string CustomAlarmTypes::insertSqlOnApplyingChanges() const
{
  return "INSERT INTO custom_alarm_types (airline, name, name_lat, id, tid, pr_del) "
         "VALUES (:airline, :name, :name_lat, :id, :tid, :pr_del) ";
}

std::string CustomAlarmTypes::updateSqlOnApplyingChanges() const
{
  return "UPDATE custom_alarm_types "
         "SET airline = :airline, name = :name, name_lat = :name_lat, tid = :tid, pr_del=:pr_del "
         "WHERE id = :id ";
}

std::string CustomAlarmTypes::deleteSqlOnApplyingChanges() const
{
  return "DELETE FROM custom_alarm_types "
         "WHERE id = :id ";
}

std::list<std::string> CustomAlarmTypes::dbSessionObjectNamesForRead() const
{
  return {"CUSTOM_ALARM_TYPES"};
}

std::string CustomAlarmTypes::tableName() const
{
  return "custom_alarm_types";
}

std::string CustomAlarmTypes::idFieldName() const
{
    return "id";
}

void CustomAlarmTypes::bind(const Row& row, DbCpp::CursCtl& cur) const
{
  cur.stb()
     .bind(":airline",  row.getAsString("airline"))
     .bind(":name",     row.getAsString("name"))
     .bind(":name_lat", row.getAsString("name_lat"));
}

std::optional<RowId_t> CustomAlarmTypes::getRowIdBeforeInsert(const Row& row) const
{
  DB::TCachedQuery lockQry(
              PgOra::getRWSession({"CUSTOM_ALARM_TYPES","PAX_CUSTOM_ALARMS"}),
              "SELECT id FROM custom_alarm_types "
              "WHERE pr_del <> 0 "
              "AND NOT EXISTS ("
              "  SELECT * FROM pax_custom_alarms "
              "  WHERE alarm_type = custom_alarm_types.id "
              "  FETCH FIRST 1 ROWS ONLY) "
              "FETCH FIRST 1 ROWS ONLY "
              "FOR UPDATE ",
              STDLOG);
  lockQry.get().Execute();
  if (!lockQry.get().Eof) {
    return RowId_t(lockQry.get().FieldAsInteger("id"));
  }

  return std::nullopt;
}

//ConfirmationSets

bool ConfirmationSets::userDependence() const
{
  return true;
}

std::string ConfirmationSets::selectSql() const
{
  return "SELECT a.id, a.airline, a.airp_dep, a.class, a.subclass, a.rfisc, a.rem_code, "
         "       a.brand_airline, a.brand_code, brands.id AS brand_view, "
         "       a.fqt_airline, a.fqt_tier_level, a.dcs_action, a.text, a.text_lat "
         "FROM confirmation_sets a "
         "LEFT OUTER JOIN brands "
         "ON a.brand_airline = brands.airline AND a.brand_code = brands.code "
         "WHERE " + getSQLFilter("a.airline",  AccessControl::PermittedAirlines) + " AND "
          "     " + getSQLFilter("a.airp_dep",  AccessControl::PermittedAirportsOrNull) + " "
         "ORDER BY a.airline, a.dcs_action, a.id ";
}

std::string ConfirmationSets::insertSql() const
{
  return "INSERT INTO confirmation_sets(id, airline, airp_dep, class, subclass, rfisc, rem_code, "
         "  brand_airline, brand_code, fqt_airline, fqt_tier_level, dcs_action, text, text_lat) "
         "VALUES(:id, :airline, :airp_dep, :class, :subclass, :rfisc, :rem_code, "
         "  :brand_airline, :brand_code, :fqt_airline, :fqt_tier_level, :dcs_action, :text, :text_lat) ";
}

std::string ConfirmationSets::updateSql() const
{
  return "UPDATE confirmation_sets "
         "SET text=:text, text_lat=:text_lat "
         "WHERE id=:OLD_id ";
}

std::string ConfirmationSets::deleteSql() const
{
  return "DELETE FROM confirmation_sets "
         "WHERE id = :OLD_id ";
}

std::list<std::string> ConfirmationSets::dbSessionObjectNames() const
{
  return {"CONFIRMATION_SETS"};
}

std::list<std::string> ConfirmationSets::dbSessionObjectNamesForRead() const
{
  return {"CONFIRMATION_SETS","BRANDS"};
}

void ConfirmationSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                                const std::optional<CacheTable::Row>& oldRow,
                                                std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);

  if (status == usInserted) {
      const std::string brand_airline = newRow.value().getAsString("brand_airline");
      const std::string airline = newRow.value().getAsString("airline");
      if (!brand_airline.empty() && brand_airline != airline) {
          throw UserException("MSG.BRAND_DOES_NOT_MEET_AIRLINE");
      }
  }

  if (newRow) {
      const std::string text_lat = newRow.value().getAsString("text_lat");
      checkASCII(text_lat, "CONFIRMATION_SETS", "TEXT_LAT");
  }

  setRowId("id", status, newRow);
}

void ConfirmationSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                               const std::optional<CacheTable::Row>& oldRow,
                                               const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("confirmation_sets").synchronize(getRowId("id", oldRow, newRow));
}

//Brands

bool Brands::userDependence() const
{
  return true;
}

std::string Brands::selectSql() const
{
  return "SELECT airline, code, name, name_lat, id "
         "FROM brands "
         "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlines) + " "
         "ORDER BY airline, code ";
}

std::string Brands::insertSql() const
{
  return "INSERT INTO brands(airline, code, name, name_lat, id) "
         "VALUES(:airline, :code, :name, :name_lat, :id) ";
}

std::string Brands::updateSql() const
{
  return "UPDATE brands "
         "SET airline=:airline, code=:code, name=:name, name_lat=:name_lat "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::string Brands::deleteSql() const
{
  return "DELETE FROM brands "
         "WHERE airline=:OLD_airline AND code=:OLD_code ";
}

std::list<std::string> Brands::dbSessionObjectNames() const
{
  return {"BRANDS"};
}

void Brands::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                                const std::optional<CacheTable::Row>& oldRow,
                                                std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineAccess("airline", oldRow, newRow);

  setRowId("id", status, newRow);
}

void Brands::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                               const std::optional<CacheTable::Row>& oldRow,
                                               const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("brands").synchronize(getRowId("id", oldRow, newRow));
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
        .setField("norm_type", "БП")
        .setField("norm_type_view", ElemIdToCodeNative(etBagNormType, "БП"))
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
    if (code.size()!=8 || !IsUpperLettersOrDigits(code))
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
    if (saleDeskCode.size()==6) //потому что не требуем строго 6 символов при вводе в sale_desks
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

//PayMethodsTypes

bool PayMethodsTypes::userDependence() const {
  return false;
}
std::string PayMethodsTypes::selectSql() const {
  return "SELECT id, name, name_lat, descr, descr_lat FROM pay_methods_types ORDER BY name";
}
std::list<std::string> PayMethodsTypes::dbSessionObjectNames() const {
  return {"PAY_METHODS_TYPES"};
}

//PayMethodsSet

bool PayMethodsSet::userDependence() const {
  return true;
}
std::string PayMethodsSet::selectSql() const {
  return
   "SELECT id, desk_grp_id, desk_grp_id descr, desk, airline, airp_dep, method_type "
   "FROM pay_methods_set "
   "WHERE " + getSQLFilter("airline",  AccessControl::PermittedAirlinesOrNull) + " AND "
            + getSQLFilter("airp_dep", AccessControl::PermittedAirportsOrNull) +
   "ORDER BY id";
}
std::string PayMethodsSet::insertSql() const {
  return "INSERT INTO pay_methods_set(id, desk_grp_id, desk, airline, airp_dep, method_type) "
         "VALUES(:id, :desk_grp_id, :desk, :airline, :airp_dep, :method_type)";
}
std::string PayMethodsSet::updateSql() const {
  return "UPDATE pay_methods_set "
         "SET desk_grp_id = :desk_grp_id, desk = :desk, airline=:airline, airp_dep=:airp_dep, method_type=:method_type "
         "WHERE id=:OLD_id";
}
std::string PayMethodsSet::deleteSql() const {
  return "DELETE FROM pay_methods_set WHERE id=:OLD_id";
}
std::list<std::string> PayMethodsSet::dbSessionObjectNames() const {
  return {"PAY_METHODS_SET"};
}

void PayMethodsSet::beforeApplyingRowChanges(const TCacheUpdateStatus status,
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

  checkDeskAndDeskGrp("desk", "desk_grp_id", true, newRow);
  checkDeskAccess("desk", oldRow, newRow);
  checkDeskGrpAccess("desk_grp_id", true, oldRow, newRow);

  setRowId("id", status, newRow);
}

void PayMethodsSet::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("pay_methods_set").synchronize(getRowId("id", oldRow, newRow));
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

//CompRemTypes

bool CompRemTypes::userDependence() const {
  return false;
}
std::string CompRemTypes::selectSql() const {
  return "SELECT id, code, code_lat, name, name_lat, pr_comp AS priority, tid, pr_del "
         "FROM comp_rem_types "
         "ORDER BY code";
}
std::string CompRemTypes::refreshSql() const {
  return "SELECT id, code, code_lat, name, name_lat, pr_comp AS priority, tid, pr_del "
         "FROM comp_rem_types "
         "WHERE tid>:tid "
         "ORDER BY code";
}
std::string CompRemTypes::insertSqlOnApplyingChanges() const {
  return "INSERT INTO comp_rem_types(code, code_lat, name, name_lat, pr_comp, id, tid, pr_del) "
         "VALUES(:code, :code_lat, :name, :name_lat, :priority, :id, :tid, :pr_del)";
}
std::string CompRemTypes::updateSqlOnApplyingChanges() const {
  return "UPDATE comp_rem_types "
         "SET code=:code, code_lat=:code_lat, name=:name, name_lat=:name_lat, pr_comp=:priority, tid=:tid, pr_del=:pr_del "
         "WHERE id=:id";
}
std::string CompRemTypes::deleteSqlOnApplyingChanges() const {
  return "DELETE FROM comp_rem_types WHERE id=:id";
}
std::list<std::string> CompRemTypes::dbSessionObjectNamesForRead() const {
  return {"COMP_REM_TYPES"};
}
std::string CompRemTypes::tableName() const {
  return "comp_rem_types";
}
std::string CompRemTypes::idFieldName() const {
  return "id";
}

void CompRemTypes::bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const
{
  cur.stb()
     .bind(":code",     row.getAsString("code"))
     .bind(":code_lat", row.getAsString("code_lat"))
     .bind(":name",     row.getAsString("name"))
     .bind(":name_lat", row.getAsString("name_lat"))
     .bind(":priority", row.getAsInteger_ThrowOnEmpty("priority"));
}

std::optional<RowId_t> CompRemTypes::getRowIdBeforeInsert(const CacheTable::Row& row) const
{
  auto cur=make_db_curs("SELECT id FROM comp_rem_types WHERE code=:code AND pr_del<>0 FOR UPDATE",
                        PgOra::getRWSession("COMP_REM_TYPES"));
  int id;
  cur.stb()
     .def(id)
     .bind(":code", row.getAsString_ThrowOnEmpty("code"))
     .EXfet();

  if (cur.err() != DbCpp::ResultCode::NoDataFound) return RowId_t(id);

  return std::nullopt;
}

//CkinRemTypes

bool CkinRemTypes::userDependence() const {
  return false;
}
std::string CkinRemTypes::selectSql() const {
  return "SELECT id, code, code_lat, name, name_lat, grp_id, grp_id AS grp, is_iata, tid, pr_del "
         "FROM ckin_rem_types "
         "ORDER BY code";
}
std::string CkinRemTypes::refreshSql() const {
  return "SELECT id, code, code_lat, name, name_lat, grp_id, grp_id AS grp, is_iata, tid, pr_del "
         "FROM ckin_rem_types "
         "WHERE tid>:tid "
         "ORDER BY code";
}
std::string CkinRemTypes::insertSqlOnApplyingChanges() const {
  return "INSERT INTO ckin_rem_types(code, code_lat, name, name_lat, grp_id, is_iata, id, tid, pr_del) "
         "VALUES(:code, :code_lat, :name, :name_lat, :grp_id, :is_iata, :id, :tid, :pr_del)";
}
std::string CkinRemTypes::updateSqlOnApplyingChanges() const {
  return "UPDATE ckin_rem_types "
         "SET code=:code, code_lat=:code_lat, name=:name, name_lat=:name_lat, grp_id=:grp_id, is_iata=:is_iata, tid=:tid, pr_del=:pr_del "
         "WHERE id=:id";
}
std::string CkinRemTypes::deleteSqlOnApplyingChanges() const {
  return "DELETE FROM ckin_rem_types WHERE id=:id";
}
std::list<std::string> CkinRemTypes::dbSessionObjectNamesForRead() const {
  return {"CKIN_REM_TYPES"};
}
std::string CkinRemTypes::tableName() const {
  return "ckin_rem_types";
}
std::string CkinRemTypes::idFieldName() const {
  return "id";
}

void CkinRemTypes::bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const
{
  cur.stb()
     .bind(":code",     row.getAsString("code"))
     .bind(":code_lat", row.getAsString("code_lat"))
     .bind(":name",     row.getAsString("name"))
     .bind(":name_lat", row.getAsString("name_lat"))
     .bind(":grp_id",   row.getAsInteger_ThrowOnEmpty("grp_id"))
     .bind(":is_iata",  row.getAsBoolean_ThrowOnEmpty("is_iata"));
}

std::optional<RowId_t> CkinRemTypes::getRowIdBeforeInsert(const CacheTable::Row& row) const
{
  auto cur=make_db_curs("SELECT id FROM ckin_rem_types WHERE code=:code AND pr_del<>0 FOR UPDATE",
                        PgOra::getRWSession("CKIN_REM_TYPES"));
  int id;
  cur.stb()
     .def(id)
     .bind(":code", row.getAsString_ThrowOnEmpty("code"))
     .EXfet();

  if (cur.err() != DbCpp::ResultCode::NoDataFound) return RowId_t(id);

  return std::nullopt;
}

//BalanceTypes

bool BalanceTypes::userDependence() const {
  return false;
}
std::string BalanceTypes::selectSql() const {
  return "SELECT code, name, name_lat FROM balance_types ORDER BY code";
}
std::list<std::string> BalanceTypes::dbSessionObjectNames() const {
  return {"BALANCE_TYPES"};
}

//BalanceSets

bool BalanceSets::userDependence() const {
  return true;
}
std::string BalanceSets::selectSql() const {
  return
   "SELECT id, airp, airline, flt_no, bort, balance_type, pr_denial "
   "FROM balance_sets "
   "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " AND "
            + getSQLFilter("airp",    AccessControl::PermittedAirports) +
   "ORDER BY airp, airline, flt_no, bort";
}
std::string BalanceSets::insertSql() const {
  return "INSERT INTO balance_sets(id, airp, airline, flt_no, bort, balance_type, pr_denial) "
         "VALUES(:id, :airp, :airline, :flt_no, :bort, :balance_type, :pr_denial)";
}
std::string BalanceSets::updateSql() const {
  return "UPDATE balance_sets "
         "SET airline=:airline, airp=:airp, flt_no=:flt_no, bort=:bort, "
             "balance_type=:balance_type, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string BalanceSets::deleteSql() const {
  return "DELETE FROM balance_sets WHERE id=:OLD_id";
}
std::list<std::string> BalanceSets::dbSessionObjectNames() const {
  return {"BALANCE_SETS"};
}

void BalanceSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                           const std::optional<CacheTable::Row>& oldRow,
                                           std::optional<CacheTable::Row>& newRow) const
{
  if (newRow)
  {
    std::string bort=checkAndNormalizeBort(newRow.value().getAsString("bort"));
    newRow.value().setFromString("bort", bort);
  }

  checkAirlineAccess("airline", oldRow, newRow);
  checkAirportAccess("airp", oldRow, newRow);

  setRowId("id", status, newRow);
}

void BalanceSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("balance_sets").synchronize(getRowId("id", oldRow, newRow));
}

//CodeshareSets

std::string CodeshareSets::selectSql() const
{
  return
    "SELECT id, "
           "airline_oper, "
           "flt_no_oper, "
           "suffix_oper, "
           "airp_dep, "
           "airline_mark, "
           "flt_no_mark, "
           "suffix_mark, "
           "pr_mark_norms, "
           "pr_mark_bp, "
           "pr_mark_rpt, "
           "days, "
           "first_date, "
          + lastDateSelectSQL("CODESHARE_SETS") + ", "
           "tid, "
           "0 AS pr_denial "
      "FROM codeshare_sets "
     "WHERE pr_del = 0 "
       "AND (last_date IS NULL OR last_date >= :lower_bound) "
       "AND (" + getSQLFilter("airline_oper", AccessControl::PermittedAirlines) +
         "OR " + getSQLFilter("airline_mark", AccessControl::PermittedAirlines) + ") "
       "AND " + getSQLFilter("airp_dep", AccessControl::PermittedAirports) +
     "ORDER BY airline_oper, flt_no_oper, suffix_oper, airp_dep, first_date";
}

void CodeshareSets::beforeSelectOrRefresh(const TCacheQueryType queryType,
                                          const TParams& sqlParams,
                                          DB::TQuery& Qry) const
{
  BASIC::date_time::TDateTime lowerBound;
  modf(BASIC::date_time::NowUTC() - 3, &lowerBound);
  Qry.CreateVariable("lower_bound", otDate, lowerBound);
}

void CodeshareSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                            const std::optional<CacheTable::Row>& oldRow,
                                            std::optional<CacheTable::Row>& newRow) const
{
  checkAirlineOrAirlineAccess("airline_oper", "airline_mark", oldRow, newRow);
  checkAirportAccess("airp_dep", oldRow, newRow);
}

void CodeshareSets::onApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const
{
  if (status==usInserted)
  {
    const CacheTable::Row& row=newRow.value();

    CodeshareSet::add(
                row.getAsInteger("id", ASTRA::NoExists),
                row.getAsString("airline_oper"),
                row.getAsInteger("flt_no_oper", ASTRA::NoExists),
                row.getAsString("suffix_oper"),
                row.getAsString("airp_dep"),
                row.getAsString("airline_mark"),
                row.getAsInteger("flt_no_mark", ASTRA::NoExists),
                row.getAsString("suffix_mark"),
                row.getAsBoolean_ThrowOnEmpty("pr_mark_norms"),
                row.getAsBoolean_ThrowOnEmpty("pr_mark_bp"),
                row.getAsBoolean_ThrowOnEmpty("pr_mark_rpt"),
                row.getAsString("days"),
                row.getAsDateTime_ThrowOnEmpty("first_date"),
                row.getAsDateTime("last_date", ASTRA::NoExists),
                row.getAsBoolean_ThrowOnEmpty("pr_denial"),
                ASTRA::NoExists);
  }

  if (status==usModified)
  {
    CodeshareSet::modifyById(
                oldRow.value().getAsInteger_ThrowOnEmpty("id"),
                newRow.value().getAsDateTime("last_date", ASTRA::NoExists));
  }

  if (status==usDeleted)
  {
    CodeshareSet::deleteById(oldRow.value().getAsInteger_ThrowOnEmpty("id"));
  }
}

std::list<std::string> CodeshareSets::dbSessionObjectNames() const {
    return {"CODESHARE_SETS"};
}


void DeskOwnersAdd::onSelectOrRefresh(const TParams &sqlParams, SelectedRows &rows) const
{
    rows.setFromInteger(0)
        .setFromString("")
        .setFromString("")
        .setFromInteger(0)
        .setFromInteger(0)
            .addRow();
}

void DeskOwnersAdd::onApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row> &oldRow,
                                         const std::optional<Row> &newRow) const
{
    if(!newRow) return;
    Row row = *newRow;
    int desk_grp_id = row.getAsInteger_ThrowOnEmpty("desk_grp_id");
    std::string desk_prefix = row.getAsString_ThrowOnEmpty("desk_prefix");
    int desk_begin_num = row.getAsInteger_ThrowOnEmpty("desk_begin_num");
    int desk_amount = row.getAsInteger_ThrowOnEmpty("desk_amount");
    bool processed = false;
    std::string desk_code;
    for(int i = desk_begin_num; i < desk_begin_num + desk_amount-1; i++) {
        int id_next_val = newRowId().get();
        processed = true;
        desk_code = desk_prefix + StrUtils::lpad(std::to_string(i), 6-desk_prefix.size(), '0');

        DB::TQuery QryDesks(PgOra::getRWSession("DESKS"), STDLOG);
        QryDesks.SQLText = "INSERT INTO desks(code, grp_id, currency, id) "
                           " VALUES(:desk_code, :desk_grp_id, 'РУБ', :id_next_val); ";
        QryDesks.CreateVariable("desk_code", otString, desk_code);
        QryDesks.CreateVariable("desk_grp_id", otInteger, desk_grp_id);
        QryDesks.CreateVariable("id_next_val", otInteger, id_next_val);
        QryDesks.Execute();
        HistoryTable("DESKS").synchronize(RowId_t(id_next_val));

        id_next_val = newRowId().get();
        DB::TQuery QryDeskOwn(PgOra::getRWSession("DESK_OWNERS"), STDLOG);
        QryDeskOwn.SQLText = " INSERT INTO desk_owners(id, desk, airline, pr_denial) "
                             " VALUES(:id_next_val, :desk_code, null, 1); ";
        QryDeskOwn.CreateVariable("desk_code", otString, desk_code);
        QryDeskOwn.CreateVariable("id_next_val", otInteger, id_next_val);
        QryDeskOwn.Execute();
        HistoryTable("DESK_OWNERS").synchronize(RowId_t(id_next_val));
    }
    if(processed) {
        int tid = generatedTid.get();
        DB::TQuery Qry(PgOra::getRWSession("CACHE_TABLES"), STDLOG);
        Qry.SQLText = "update cache_tables set tid = :tid_next_val where code in ('DESK_OWNERS', 'DESKS') ";
        Qry.CreateVariable("tid_next_val", otInteger, tid);
        Qry.Execute();
    }
}

void DeskOwnersAdd::beforeApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row> &oldRow,
                                             std::optional<Row> &newRow) const
{
      checkDeskGrpAccess("desk_grp_id", false, std::nullopt, newRow);

      if(!newRow) return;
      Row row = *newRow;
      std::string desk_prefix = row.getAsString_ThrowOnEmpty("desk_prefix");
      int desk_begin_num = row.getAsInteger_ThrowOnEmpty("desk_begin_num");
      int desk_amount = row.getAsInteger_ThrowOnEmpty("desk_amount");

      if (desk_prefix.size() < 4 ) {
          throw AstraLocale::UserException("MSG.DESK_OWNERS_ADD.WRONG_DESK_PREFIX_LEN");
      }
      if(desk_begin_num == 0) {
           throw AstraLocale::UserException("MSG.DESK_OWNERS_ADD.DESK_BEGIN_NUM_NOT_SET");
      }
      if(desk_amount == 0) {
          throw AstraLocale::UserException("MSG.DESK_OWNERS_ADD.DESK_AMOUNT_NOT_SET");
      }
      if((desk_prefix.size() + desk_begin_num + desk_amount - 1) > 6) {
          throw AstraLocale::UserException("MSG.DESK_OWNERS_ADD.WRONG_INTERVAL");
      }
}

void DeskOwnersGrp::onSelectOrRefresh(const TParams &sqlParams, SelectedRows &rows) const
{
    rows.setFromInteger(0)
        .setFromString("")
        .setFromString("")
        .setFromString("")
        .setFromBoolean(1)
        .addRow();
}

void DeskOwnersGrp::onApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row> &oldRow,
                                         const std::optional<Row> &newRow) const
{
    if(!newRow) return;
    Row row = *newRow;
    int desk_grp_id = row.getAsInteger_ThrowOnEmpty("desk_grp_id");
    std::string airline = row.getAsString("airline", "");
    int pr_denial = row.getAsInteger_ThrowOnEmpty("pr_denial");

    bool pr_found = false;

    DB::TQuery QryMain(PgOra::getROSession({"DESK_OWNERS","DESKS"}), STDLOG);
    QryMain.SQLText = " select desk_owners.id, desk_owners.desk from desk_owners, desks where "
                      " desk_owners.desk = desks.code and desks.grp_id = :desk_grp_id and "
                      " (desk_owners.airline = :airline OR (desk_owners.airline IS NULL AND :airline IS NULL))";
    QryMain.CreateVariable("desk_grp_id",otInteger, desk_grp_id);
    QryMain.CreateVariable("airline",otString, airline);
    QryMain.Execute();
    for(;!QryMain.Eof; QryMain.Next()) {
        pr_found = true;
        int id = QryMain.FieldAsInteger("id");

        auto cur = make_db_curs( " update DESK_OWNERS set pr_denial = :pr_denial "
                                 " where id = :id and "
                                 " (airline = :airline OR (airline IS NULL AND :airline IS NULL))",
                              PgOra::getRWSession("DESK_OWNERS"));

        cur.noThrowError(DbCpp::ResultCode::ConstraintFail)
           .stb()
           .bind(":id", id)
           .bind(":pr_denial", pr_denial)
           .bind(":airline", airline)
           .exec();

        HistoryTable("DESK_OWNERS").synchronize(RowId_t(id));
    }

    if(!pr_found) {
        if(airline.empty()) {
            throw AstraLocale::UserException("MSG.NOT_DATA");
        } else {
            pr_found = false;

            DB::TQuery QryMain(PgOra::getROSession({"DESK_OWNERS","DESKS"}), STDLOG);
            QryMain.SQLText = " select desk_owners.id, desk_owners.desk from desk_owners, desks where "
                              " desk_owners.desk = desks.code and desks.grp_id = :desk_grp_id and "
                              " desk_owners.airline IS NULL ";
            QryMain.CreateVariable("desk_grp_id",otInteger, desk_grp_id);
            QryMain.Execute();

            for(;!QryMain.Eof; QryMain.Next()) {
                pr_found = true;
                int id_next_val = newRowId().get();
                std::string desk = QryMain.FieldAsString("desk");

                DB::TQuery Qry(PgOra::getRWSession("DESK_OWNERS"), STDLOG);
                Qry.SQLText = " insert into DESK_OWNERS(id, desk, airline, pr_denial) "
                               " values(:id_next_val, :desk, :airline, :pr_denial) ";
                Qry.CreateVariable("id_next_val", otInteger, id_next_val);
                Qry.CreateVariable("desk", otString, desk);
                Qry.CreateVariable("airline", otString, airline);
                Qry.CreateVariable("pr_denial", otInteger, pr_denial);
                Qry.Execute();
                HistoryTable("DESK_OWNERS").synchronize(RowId_t(id_next_val));
            }

            if(!pr_found) {
                throw AstraLocale::UserException("MSG.NOT_DATA");
            } else {
                int tid = generatedTid.get();
                DB::TQuery Qry(PgOra::getRWSession("CACHE_TABLES"), STDLOG);
                Qry.SQLText = "update cache_tables set tid = :tid_next_val where code = 'DESK_OWNERS' ";
                Qry.CreateVariable("tid_next_val", otInteger, tid);
                Qry.Execute();
            }
        }
    }

}

void DeskOwnersGrp::beforeApplyingRowChanges(const TCacheUpdateStatus status, const std::optional<Row> &oldRow,
                                             std::optional<Row> &newRow) const
{
    checkAirlineAccess("airline", std::nullopt, newRow);
    checkDeskGrpAccess("desk_grp_id", false, std::nullopt, newRow);
}

// edifact_profiles

bool EdifactProfiles::userDependence() const {
    return false;
}

std::string EdifactProfiles::selectSql() const {
    return "SELECT name, version, sub_version, ctrl_agency, syntax_name, syntax_ver "
           "FROM EDIFACT_PROFILES "
           "ORDER BY name";
}

std::string EdifactProfiles::insertSql() const {
    return "INSERT INTO EDIFACT_PROFILES(name, version, sub_version, ctrl_agency, syntax_name, syntax_ver) "
           "VALUES(:name, :version, :sub_version, :ctrl_agency, :syntax_name, :syntax_ver) ";
}

std::string EdifactProfiles::updateSql() const {
    return "UPDATE EDIFACT_PROFILES SET "
           "version=:version, "
           "sub_version=:sub_version, "
           "ctrl_agency=:ctrl_agency, "
           "syntax_name=:syntax_name, "
           "syntax_ver=:syntax_ver "
           "WHERE name=:OLD_name";
}

std::string EdifactProfiles::deleteSql() const {
    return "DELETE FROM EDIFACT_PROFILES "
           "WHERE name=:OLD_name";
}

std::list<std::string> EdifactProfiles::dbSessionObjectNames() const {
    return { "EDIFACT_PROFILES" };
}

// edi_addrs

bool EdiAddrs::userDependence() const {
    return false;
}

std::string EdiAddrs::selectSql() const {
    return "SELECT addr,canon_name "
           "FROM EDI_ADDRS "
           "ORDER BY addr";
}

std::string EdiAddrs::insertSql() const {
    return "INSERT INTO EDI_ADDRS(addr, canon_name) "
           "VALUES(:addr, :canon_name)";
}

std::string EdiAddrs::updateSql() const {
    return "UPDATE EDI_ADDRS SET "
           "canon_name=:canon_name "
           "WHERE addr=:OLD_addr";
}

std::string EdiAddrs::deleteSql() const {
    return "DELETE FROM EDI_ADDRS "
           "WHERE addr=:OLD_addr";
}

std::list<std::string> EdiAddrs::dbSessionObjectNames() const {
    return { "EDI_ADDRS" };
}

// et_addr_set

bool EtAddrSet::userDependence() const {
    return true;
}

std::string EtAddrSet::selectSql() const {
    return "SELECT id, airline, flt_no, edi_addr, edi_addr_ext, edi_own_addr, edi_own_addr_ext, edifact_profile "
           "FROM et_addr_set "
           "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " "
           "ORDER BY airline, flt_no";
}
std::string EtAddrSet::insertSql() const {
    return "INSERT INTO et_addr_set(id, airline, flt_no, edi_addr, edi_addr_ext, edi_own_addr, edi_own_addr_ext, edifact_profile) "
           "VALUES(:id, :airline, :flt_no, :edi_addr, :edi_addr_ext, :edi_own_addr, :edi_own_addr_ext, :edifact_profile)";
}
std::string EtAddrSet::updateSql() const {
    return "UPDATE et_addr_set "
           "SET airline=:airline, flt_no=:flt_no, "
           "    edi_addr=:edi_addr, edi_addr_ext=:edi_addr_ext, "
           "    edi_own_addr=:edi_own_addr, edi_own_addr_ext=:edi_own_addr_ext, "
           "    edifact_profile=:edifact_profile "
           "WHERE id=:OLD_id";
}

std::string EtAddrSet::deleteSql() const {
    return "DELETE FROM et_addr_set "
           "WHERE id=:OLD_id";
}

std::list<std::string> EtAddrSet::dbSessionObjectNames() const {
    return {"ET_ADDR_SET"};
}

void EtAddrSet::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
    checkAirlineAccess("airline", oldRow, newRow);
    if(newRow)
    {
        checkEdiAddr(newRow.value().getAsString("edi_addr"));
        checkEdiAddr(newRow.value().getAsString("edi_own_addr"));
    }

    setRowId("id", status, newRow);
}

void EtAddrSet::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
    HistoryTable("et_addr_set").synchronize(getRowId("id", oldRow, newRow));
}

// dcs_addr_set

bool DcsAddrSet::userDependence() const {
    return true;
}

std::string DcsAddrSet::selectSql() const {
    return "SELECT id, airline, flt_no, edi_addr, edi_addr_ext, own_airline, own_flt_no, edi_own_addr, edi_own_addr_ext, edifact_profile "
           "FROM dcs_addr_set "
           "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlines) + " "
           "ORDER BY airline, flt_no";
}
std::string DcsAddrSet::insertSql() const {
    return "INSERT INTO dcs_addr_set(id, airline, flt_no, edi_addr, edi_addr_ext, own_airline, own_flt_no, edi_own_addr, edi_own_addr_ext, edifact_profile) "
           "VALUES(:id, :airline, :flt_no, :edi_addr, :edi_addr_ext, :own_airline, :own_flt_no, :edi_own_addr, :edi_own_addr_ext, :edifact_profile)";
}
std::string DcsAddrSet::updateSql() const {
    return "UPDATE dcs_addr_set "
           "SET airline=:airline, flt_no=:flt_no, "
           "    edi_addr=:edi_addr, edi_addr_ext=:edi_addr_ext, "
           "    edi_own_addr=:edi_own_addr, edi_own_addr_ext=:edi_own_addr_ext, "
           "    own_airline=:own_airline, own_flt_no=:own_flt_no, "
           "    edifact_profile=:edifact_profile "
           "WHERE id=:OLD_id";
}

std::string DcsAddrSet::deleteSql() const {
    return "DELETE FROM dcs_addr_set "
           "WHERE id=:OLD_id";
}

std::list<std::string> DcsAddrSet::dbSessionObjectNames() const {
    return {"DCS_ADDR_SET"};
}

void DcsAddrSet::beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         std::optional<CacheTable::Row>& newRow) const
{
    checkAirlineAccess("airline", oldRow, newRow);
    if(newRow)
    {
        checkEdiAddr(newRow.value().getAsString("edi_addr"));
        checkEdiAddr(newRow.value().getAsString("edi_own_addr"));
    }

    setRowId("id", status, newRow);
}

void DcsAddrSet::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                        const std::optional<CacheTable::Row>& oldRow,
                                        const std::optional<CacheTable::Row>& newRow) const
{
    HistoryTable("dcs_addr_set").synchronize(getRowId("id", oldRow, newRow));
}

// gender_types

bool GenderTypes::userDependence() const {
    return false;
}

std::string GenderTypes::selectSql() const {
    return "SELECT code, code code_lat, name, name_lat "
           "FROM gender_types "
           "ORDER BY (CASE WHEN pr_inf=0 THEN 0 ELSE 1 END), code";
}

std::list<std::string> GenderTypes::dbSessionObjectNames() const {
    return { "GENDER_TYPES" };
}

//TimaticSets

bool TimaticSets::userDependence() const {
  return true;
}
std::string TimaticSets::selectSql() const {
  return "SELECT id, airline, airp, host, port, username, sub_username, pwd, pr_denial "
         "FROM timatic_sets "
         "WHERE " + getSQLFilter("airline", AccessControl::PermittedAirlinesOrNull) + " AND "
                  + getSQLFilter("airp",    AccessControl::PermittedAirportsOrNull) +
         "ORDER BY id";
}
std::string TimaticSets::insertSql() const {
  return "INSERT INTO timatic_sets(id, airline, airp, host, port, username, sub_username, pwd, pr_denial) "
         "VALUES(:id, :airline, :airp, :host, :port, :username, :sub_username, :pwd, :pr_denial)";
}
std::string TimaticSets::updateSql() const {
  return "UPDATE timatic_sets "
         "SET airline=:airline, airp=:airp, host=:host, port=:port, username=:username, "
             "sub_username=:sub_username, pwd=:pwd, pr_denial=:pr_denial "
         "WHERE id=:OLD_id";
}
std::string TimaticSets::deleteSql() const {
  return "DELETE FROM timatic_sets WHERE id=:OLD_id";
}
std::list<std::string> TimaticSets::dbSessionObjectNames() const {
  return {"TIMATIC_SETS"};
}

void TimaticSets::beforeApplyingRowChanges(const TCacheUpdateStatus status,
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

void TimaticSets::afterApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          const std::optional<CacheTable::Row>& newRow) const
{
  HistoryTable("timatic_sets").synchronize(getRowId("id", oldRow, newRow));
}


} //namespace CacheTables
