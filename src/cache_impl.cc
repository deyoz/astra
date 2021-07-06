#include "cache_impl.h"
#include "PgOraConfig.h"
#include "astra_elems.h"

#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;

CacheTableCallbacks* SpawnCacheTableCallbacks(const std::string& cacheCode)
{
//  if (cacheCode=="GRP_BAG_TYPES1")      return new CacheTable::GrpBagTypesOutdated;
//  if (cacheCode=="GRP_BAG_TYPES2")      return new CacheTable::GrpBagTypesOutdated;
//  if (cacheCode=="GRP_BAG_TYPES")       return new CacheTable::GrpBagTypes;
//  if (cacheCode=="GRP_RFISC1")          return new CacheTable::GrpRfiscOutdated;
//  if (cacheCode=="GRP_RFISC")           return new CacheTable::GrpRfisc;
//  if (cacheCode=="FILE_TYPES")          return new CacheTable::FileTypes;
//  if (cacheCode=="TERM_PROFILE_RIGHTS") return new CacheTable::TermProfileRights;
//  if (cacheCode=="PRN_FORMS_LAYOUT")    return new CacheTable::PrnFormsLayout;
//  if (cacheCode=="TRIP_SUFFIXES")       return new CacheTable::TripSuffixes;
  return nullptr;
}

namespace CacheTable
{

bool GrpBagTypesOutdated::userDependence() const {
  return false;
}

void GrpBagTypesOutdated::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("ORACLE"), STDLOG);

  Qry.SQLText=
       "SELECT pax.grp_id AS bag_types_id, \n"
       "       bag_type_list_items.bag_type AS code, \n"
       "       bag_type_list_items.name, \n"
       "       bag_type_list_items.name_lat, \n"
       "       bag_type_list_items.descr, \n"
       "       bag_type_list_items.descr_lat, \n"
       "       bag_type_list_items.bag_type AS priority, \n"
       "       DECODE(bag_type_list_items.category, 1, 0, 4, 0, 2, 1, 5, 1, NULL) AS pr_cabin \n"
       "FROM bag_type_list_items, pax_service_lists, pax \n"
       "WHERE bag_type_list_items.list_id=pax_service_lists.list_id AND \n"
       "      pax_service_lists.pax_id=pax.pax_id AND \n"
       "      pax.grp_id=:bag_types_id AND \n"
       "      pax_service_lists.transfer_num=0 AND \n"
       "      pax_service_lists.category IN (1,2) AND \n"
       "      bag_type_list_items.category<>0 AND \n"
       "      bag_type_list_items.visible<>0 AND \n"
       "      bag_type_list_items.bag_type<>' ' \n"
       "UNION \n"
       "SELECT grp_service_lists.grp_id AS bag_types_id, \n"
       "       bag_type_list_items.bag_type AS code, \n"
       "       bag_type_list_items.name, \n"
       "       bag_type_list_items.name_lat, \n"
       "       bag_type_list_items.descr, \n"
       "       bag_type_list_items.descr_lat, \n"
       "       bag_type_list_items.bag_type AS priority, \n"
       "       DECODE(bag_type_list_items.category, 1, 0, 4, 0, 2, 1, 5, 1, NULL) AS pr_cabin \n"
       "FROM bag_type_list_items, grp_service_lists \n"
       "WHERE bag_type_list_items.list_id=grp_service_lists.list_id AND \n"
       "      grp_service_lists.grp_id=:bag_types_id AND \n"
       "      grp_service_lists.transfer_num=0 AND \n"
       "      grp_service_lists.category IN (1,2) AND \n"
       "      bag_type_list_items.category<>0 AND \n"
       "      bag_type_list_items.visible<>0 AND \n"
       "      bag_type_list_items.bag_type<>' ' \n"
       "ORDER BY code";

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
  DB::TQuery Qry(PgOra::getROSession("ORACLE"), STDLOG);

  Qry.SQLText=
       "SELECT items.airline, items.bag_type, items.name, items.name_lat, items.descr, items.descr_lat \n"
       "FROM bag_type_list_items items, pax, pax_service_lists \n"
       "WHERE pax.pax_id=pax_service_lists.pax_id AND pax.grp_id=:grp_id AND \n"
       "      items.list_id=pax_service_lists.list_id \n"
       "UNION \n"
       "SELECT items.airline, items.bag_type, items.name, items.name_lat, items.descr, items.descr_lat \n"
       "FROM bag_type_list_items items, grp_service_lists \n"
       "WHERE grp_service_lists.grp_id=:grp_id AND \n"
       "      items.list_id=grp_service_lists.list_id \n"
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

bool GrpRfiscOutdated::userDependence() const {
  return false;
}

void GrpRfiscOutdated::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("ORACLE"), STDLOG);

  Qry.SQLText=
       "SELECT bag_types_id, rfisc AS code, LOWER(name) AS name, LOWER(name_lat) AS name_lat, \n"
       "       NVL(rfisc_bag_props.priority, 100000) AS priority, pr_cabin \n"
       "FROM rfisc_bag_props, \n"
       "(SELECT pax.grp_id AS bag_types_id, \n"
       "        rfisc_list_items.rfisc, \n"
       "        rfisc_list_items.name, \n"
       "        rfisc_list_items.name_lat, \n"
       "        rfisc_list_items.airline, \n"
       "        DECODE(rfisc_list_items.category, 1, 0, 4, 0, 2, 1, 5, 1, NULL) AS pr_cabin \n"
       " FROM rfisc_list_items, pax_service_lists, pax \n"
       " WHERE rfisc_list_items.list_id=pax_service_lists.list_id AND \n"
       "       pax_service_lists.pax_id=pax.pax_id AND \n"
       "       pax.grp_id=:bag_types_id AND \n"
       "       pax_service_lists.transfer_num=0 AND \n"
       "       pax_service_lists.category IN (1,2) AND \n"
       "       rfisc_list_items.category<>0 AND \n"
       "       rfisc_list_items.visible<>0 AND \n"
       "       rfisc_list_items.service_type='C' \n"
       " UNION \n"
       " SELECT grp_service_lists.grp_id AS bag_types_id, \n"
       "        rfisc_list_items.rfisc, \n"
       "        rfisc_list_items.name, \n"
       "        rfisc_list_items.name_lat, \n"
       "        rfisc_list_items.airline, \n"
       "        DECODE(rfisc_list_items.category, 1, 0, 4, 0, 2, 1, 5, 1, NULL) AS pr_cabin \n"
       " FROM rfisc_list_items, grp_service_lists \n"
       " WHERE rfisc_list_items.list_id=grp_service_lists.list_id AND \n"
       "       grp_service_lists.grp_id=:bag_types_id AND \n"
       "       grp_service_lists.transfer_num=0 AND \n"
       "       grp_service_lists.category IN (1,2) AND \n"
       "       rfisc_list_items.category<>0 AND \n"
       "       rfisc_list_items.visible<>0 AND \n"
       "       rfisc_list_items.service_type='C') r \n"
       "WHERE r.airline=rfisc_bag_props.airline(+) AND \n"
       "      r.rfisc=rfisc_bag_props.code(+) \n"
       "ORDER BY priority NULLS LAST, code";

  CreateVariablesFromParams({"bag_types_id"}, sqlParams, Qry);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    rows.setFromString (Qry, 1)
        .setFromString (Qry, 2)
        .setFromString (Qry, 3)
        .setFromBoolean(Qry, 5)
        .setFromInteger(Qry, 4)
        .setFromInteger(Qry, 0)
        .addRow();
  }
}

bool GrpRfisc::userDependence() const {
  return false;
}

void GrpRfisc::onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const
{
  DB::TQuery Qry(PgOra::getROSession("ORACLE"), STDLOG);

  Qry.SQLText=
       "SELECT items.airline, items.service_type, items.rfisc, \n"
       "       LOWER(items.name) AS name, LOWER(items.name_lat) AS name_lat \n"
       "FROM rfisc_list_items items, pax, pax_service_lists \n"
       "WHERE pax.pax_id=pax_service_lists.pax_id AND pax.grp_id=:grp_id AND \n"
       "      items.list_id=pax_service_lists.list_id \n"
       "UNION \n"
       "SELECT items.airline, items.service_type, items.rfisc, items.name, items.name_lat \n"
       "FROM rfisc_list_items items, grp_service_lists \n"
       "WHERE grp_service_lists.grp_id=:grp_id AND \n"
       "      items.list_id=grp_service_lists.list_id \n"
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

}
