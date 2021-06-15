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
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"FILE_TYPES"};
    }
};

class Airlines : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "SELECT id, code, code_lat, code_icao, code_icao_lat, name, name_lat, short_name, short_name_lat, aircode, city, tid, pr_del \n"
             "FROM airlines ORDER BY code";
    }
    std::string refreshSql() const {
      return "SELECT id, code, code_lat, code_icao, code_icao_lat, name, name_lat, short_name, short_name_lat, aircode, city, tid, pr_del \n"
             "FROM airlines WHERE tid>:tid ORDER BY code";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"AIRLINES"};
    }
};

class Airps : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "SELECT id, code, code_lat, code_icao, code_icao_lat, city, city AS city_name, name, name_lat, tid, pr_del \n"
             "FROM airps ORDER BY code";
    }
    std::string refreshSql() const {
      return "SELECT id, code, code_lat, code_icao, code_icao_lat, city, city AS city_name, name, name_lat, tid, pr_del \n"
             "FROM airps WHERE tid>:tid ORDER BY code";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"AIRPS"};
    }
};

class TripTypes : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "SELECT id, code, code_lat, name, name_lat, pr_reg, tid, pr_del FROM trip_types ORDER BY code";
    }
    std::string refreshSql() const {
      return "SELECT id, code, code_lat, name, name_lat, pr_reg, tid, pr_del FROM trip_types WHERE tid>:tid ORDER BY code";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"TRIP_TYPES"};
    }
};

class TripSuffixes : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "SELECT code,code_lat FROM trip_suffixes ORDER BY code";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"TRIP_SUFFIXES"};
    }
};

class TermProfileRights : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return "select airline, airp, right_id, 1 user_type \n"
             "from airline_profiles ap, profile_rights pr \n"
             "where ap.profile_id = pr.profile_id AND \n"
             "      right_id NOT IN (191, 192)";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"AIRLINE_PROFILES", "PROFILE_RIGHTS"};
    }
};

class PrnFormsLayout : public CacheTableReadonly
{
  public:
    std::string selectSql() const {
      return
       "select \n"
       "  id \n"
       "  ,op_type \n"
       "  ,max(CASE param_name WHEN 'btn_caption'          THEN param_value END) btn_caption \n"
       "  ,max(CASE param_name WHEN 'models_cache'         THEN param_value END) models_cache \n"
       "  ,max(CASE param_name WHEN 'types_cache'          THEN param_value END) types_cache \n"
       "  ,max(CASE param_name WHEN 'blanks_lbl'           THEN param_value END) blanks_lbl \n"
       "  ,max(CASE param_name WHEN 'forms_lbl'            THEN param_value END) forms_lbl \n"
       "  ,max(CASE param_name WHEN 'blank_list_cache'     THEN param_value END) blank_list_cache \n"
       "  ,max(CASE param_name WHEN 'airline_set_cache'    THEN param_value END) airline_set_cache \n"
       "  ,max(CASE param_name WHEN 'trip_set_cache'       THEN param_value END) trip_set_cache \n"
       "  ,max(CASE param_name WHEN 'airline_set_caption'  THEN param_value END) airline_set_caption \n"
       "  ,max(CASE param_name WHEN 'trip_set_caption'     THEN param_value END) trip_set_caption \n"
       "  ,max(CASE param_name WHEN 'msg_insert_blank_seg' THEN param_value END) msg_insert_blank_seg \n"
       "  ,max(CASE param_name WHEN 'msg_insert_blank'     THEN param_value END) msg_insert_blank \n"
       "  ,max(CASE param_name WHEN 'msg_wait_printing'    THEN param_value END) msg_wait_printing \n"
       "from prn_forms_layout \n"
       "group by id, op_type \n"
       "order by id";
    }
    std::list<std::string> dbSessionObjectNames() const {
      return {"PRN_FORMS_LAYOUT"};
    }
};

}

