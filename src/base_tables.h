#ifndef _BASE_TABLES_H_
#define _BASE_TABLES_H_

#include <map>
#include <vector>
#include <string>
#include "exceptions.h"
#include "db_tquery.h"
#include "stl_utils.h"
#include "astra_locale.h"
#include "memory_manager.h"

class EBaseTableError:public EXCEPTIONS::Exception
{
  public:
    EBaseTableError(const char *format, ...): Exception("")
    {
      va_list ap;
      va_start(ap, format);
      vsnprintf(Message, sizeof(Message), format, ap);
      Message[sizeof(Message)-1]=0;
      va_end(ap);
    };
    EBaseTableError(std::string msg): Exception(msg) {};
};



class TBaseTableRow {
  public:
    virtual ~TBaseTableRow() {};
    virtual const char *get_row_name() const = 0;
    virtual bool deleted() = 0;
    virtual std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      throw EBaseTableError("%s::AsString: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual int AsInteger(const std::string &field) const
    {
      throw EBaseTableError("%s::AsInteger: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual bool AsBoolean(const std::string &field) const
    {
      throw EBaseTableError("%s::AsBoolean: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
};

class TBaseTable {
  private:
    bool pr_init,pr_actual;
    std::vector<TBaseTableRow*> table;
    virtual const char *get_select_sql_text() {
        return select_sql.c_str();
    }
    virtual const char *get_refresh_sql_text() {
        return select_sql.c_str();
    }
  protected:
    TMemoryManager mem;
    int prior_mem_count;
    std::string select_sql;
    void load_table();
    virtual const char *get_bt_class_name() const = 0;
    virtual std::string get_db_table_name() const = 0;
    virtual void create_variables(DB::TQuery &Qry, bool pr_refresh) = 0;
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) = 0;
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
    virtual void after_update() = 0;
    virtual void Init(const std::string &sql_table_name="") {
        pr_init=false;
        pr_actual=false;
        if ( !sql_table_name.empty() ) {
          select_sql = std::string("SELECT * FROM ") + sql_table_name;
        }
    }
  public:
    TBaseTable();
    virtual ~TBaseTable();
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
    virtual const TBaseTableRow& get_row(const std::string &field, int value, bool with_deleted=false);
    virtual void Invalidate();
};

class TNameBaseTableRow: public TBaseTableRow { //name, name_lat
    protected:
    public:
        std::string name, name_lat;
    virtual bool deleted() { return false; };
    virtual std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="name") return lang!=AstraLocale::LANG_RU?name_lat:name;
      return TBaseTableRow::AsString(field,lang);
    };
};

class TNameBaseTable: public TBaseTable {
  private:
    bool pr_name;
    bool pr_name_lat;
  protected:
        virtual void create_variables(DB::TQuery &Qry, bool pr_refresh) {};
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void after_update() {};
  public:
};

class TIdBaseTableRow : public TNameBaseTableRow {
  public:
    int id;
    virtual int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="id") return id;
      return TNameBaseTableRow::AsInteger(field);
    };
};

class TIdBaseTable: public TNameBaseTable {
  private:
    std::map<int, TBaseTableRow*> id;
  protected:
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TNameBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, int value, bool with_deleted=false);
};


class TCodeBaseTableRow : public TNameBaseTableRow {
  public:
    std::string code,code_lat;
    virtual std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code") return lang!=AstraLocale::LANG_RU?code_lat:code;
      return TNameBaseTableRow::AsString(field,lang);
    };
};

class TCodeBaseTable: public TNameBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code;
    std::map<std::string, TBaseTableRow*> code_lat;
  protected:
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TNameBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
};

class TTIDBaseTableRow : public TCodeBaseTableRow {
  public:
    int id;
    bool pr_del;
    virtual bool deleted() { return pr_del; };
    virtual int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="id") return id;
      return TCodeBaseTableRow::AsInteger(field);
    };
    virtual bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="pr_del") return pr_del;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TTIDBaseTable: public TCodeBaseTable {
  private:
    std::string refresh_sql;
    int tid,new_tid;
    std::map<int, TBaseTableRow*> id;
    virtual const char *get_refresh_sql_text() {
        return refresh_sql.c_str();
    }
  protected:
    virtual void create_variables(DB::TQuery &Qry, bool pr_refresh);
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
    virtual void after_update();
    virtual void Init(const std::string &sql_table_name="") {
        TCodeBaseTable::Init( sql_table_name );
        if ( !sql_table_name.empty() ) {
          refresh_sql = std::string("SELECT * FROM ") + sql_table_name + " WHERE tid>:tid";
        }
    }
  public:
    TTIDBaseTable() {tid=-1; new_tid=-1;};
    using TCodeBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, int value, bool with_deleted=false);
};

class TICAOBaseTableRow : public TTIDBaseTableRow {
  public:
    std::string code_icao,code_icao_lat;
    virtual std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_icao") return lang!=AstraLocale::LANG_RU?code_icao_lat:code_icao;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TICAOBaseTable: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code_icao;
    std::map<std::string, TBaseTableRow*> code_icao_lat;
  protected:
    virtual void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
};
///////////////////////////////////////////////////////////////////
class TCountriesRow: public TTIDBaseTableRow {
  public:
    std::string code_iso;
    const char *get_row_name() const { return "TCountriesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_iso") return code_iso;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TCountries: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code_iso;
  protected:
    const char *get_bt_class_name() const { return "TCountries"; };
    std::string get_db_table_name() const { return "COUNTRIES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void delete_row(TBaseTableRow *row);
    void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
    TCountries( ) {
          Init(get_db_table_name());
    }
};

class TAirpsRow: public TICAOBaseTableRow {
  public:
    std::string city;
    const char *get_row_name() const { return "TAirpsRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="city") return city;
      return TICAOBaseTableRow::AsString(field,lang);
    };
};

class TAirps: public TICAOBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TAirps"; };
    std::string get_db_table_name() const { return "AIRPS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TAirps( ) {
          Init(get_db_table_name());
    }
};

class TPersTypesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TPersTypesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TPersTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TPersTypes"; };
    std::string get_db_table_name() const { return "PERS_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TPersTypes() {
          Init(get_db_table_name());
      }
};

class TExtendedPersTypes: public TPersTypes {
  protected:
    const char *get_bt_class_name() const { return "TExtendedPersTypes"; }
    std::string get_db_table_name() const { return "PERS_TYPES"; }
  public:
    TExtendedPersTypes();
};

class TGenderTypesRow: public TCodeBaseTableRow {
  public:
    bool pr_inf;
    const char *get_row_name() const { return "TGenderTypesRow"; };
    bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="pr_inf") return pr_inf;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TGenderTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TGenderTypes"; };
    std::string get_db_table_name() const { return "GENDER_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TGenderTypes() {
        Init( get_db_table_name() );
    }
};

class TReportTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TReportTypesRow"; };
};

class TReportTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TReportTypes"; };
    std::string get_db_table_name() const { return "REPORT_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TReportTypes() {
        Init( get_db_table_name() );
    }
};

class TTagColorsRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TTagColorsRow"; };
};

class TTagColors: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TTagColors"; };
    std::string get_db_table_name() const { return "TAG_COLORS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TTagColors() {
        Init( get_db_table_name() );
    }
};

class TPaxDocCountriesRow: public TTIDBaseTableRow {
  public:
    std::string country;
    const char *get_row_name() const { return "TPaxDocCountriesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="country") return country;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TPaxDocCountries: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> country;
  protected:
    const char *get_bt_class_name() const { return "TPaxDocCountries"; };
    std::string get_db_table_name() const { return "PAX_DOC_COUNTRIES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void delete_row(TBaseTableRow *row);
    void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
    TPaxDocCountries( ) {
          Init(get_db_table_name());
    }
};

class TPaxDocTypesRow: public TCodeBaseTableRow {
  public:
    std::string code_mintrans;
    bool is_docs_type, is_doco_type;
    const char *get_row_name() const { return "TPaxDocTypesRow"; }
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_mintrans") return code_mintrans;
      return TCodeBaseTableRow::AsString(field,lang);
    }
    bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="is_docs_type") return is_docs_type;
      if (lowerc(field)=="is_doco_type") return is_doco_type;
      return TCodeBaseTableRow::AsBoolean(field);
    }
};

class TPaxDocTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TPaxDocTypes"; };
    std::string get_db_table_name() const { return "PAX_DOC_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TPaxDocTypes( ) {
        Init( get_db_table_name() );
    }
};

class TPaxDocSubtypesRow: public TCodeBaseTableRow {
  public:
    std::string doc_subtype, doc_type;
    const char *get_row_name() const { return "TPaxDocSubtypesRow"; }
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="doc_subtype") return doc_subtype;
      if (lowerc(field)=="doc_type") return doc_type;
      return TCodeBaseTableRow::AsString(field,lang);
    }
};

class TPaxDocSubtypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TPaxDocSubtypes"; }
    std::string get_db_table_name() const { return "PAX_DOC_SUBTYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {} //всегда актуальна
  public:
    TPaxDocSubtypes( ) {
      Init();
      select_sql = "SELECT doc_type||'+'||code AS code, name, name_lat, code AS doc_subtype, doc_type FROM pax_doc_subtypes";
    }
    static std::string ConstructCode(const std::string& doc_type, const std::string& doc_subtype)
    {
      return doc_type+'+'+doc_subtype;
    }
};

class TTypeBOptionValuesRow: public TCodeBaseTableRow {
  public:
    std::string short_name,short_name_lat;
    const char *get_row_name() const { return "TTypeBOptionValuesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TTypeBOptionValues: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TTypeBOptionValues"; };
    std::string get_db_table_name() const { return "TYPEB_OPTION_VALUES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TTypeBOptionValues() {
      Init();
      select_sql = "SELECT tlg_type||'+'||category||'+'||value AS code, "
                   "       short_name, short_name_lat, name, name_lat "
                   "FROM typeb_option_values";
      }
};

class TTypeBTypesRow: public TCodeBaseTableRow {
  public:
    std::string basic_type;
    int pr_dep;
    bool editable;
    std::string short_name,short_name_lat;
    const char *get_row_name() const { return "TTypeBTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="basic_type") return basic_type;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="pr_dep") return pr_dep;
      return TCodeBaseTableRow::AsInteger(field);
    };
    virtual bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="editable") return editable;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TTypeBTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TTypeBTypes"; };
    std::string get_db_table_name() const { return "TYPEB_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TTypeBTypes() {
          Init( get_db_table_name() );
      }
};

class TCitiesRow: public TTIDBaseTableRow {
  public:
    std::string country,tz_region;
    const char *get_row_name() const { return "TCitiesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="country") return country;
      if (lowerc(field)=="tz_region") return tz_region;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TCities: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TCities"; };
    std::string get_db_table_name() const { return "CITIES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCities() {
        Init( get_db_table_name() );
    }
};

class TAirlinesRow: public TICAOBaseTableRow {
  public:
    std::string aircode,short_name,short_name_lat,city;
    const char *get_row_name() const { return "TAirlinesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="aircode") return aircode;
      if (lowerc(field)=="city") return city;
      return TICAOBaseTableRow::AsString(field,lang);
    };
};

class TAirlines: public TICAOBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> aircode;
  protected:
    const char *get_bt_class_name() const { return "TAirlines"; };
    std::string get_db_table_name() const { return "AIRLINES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TICAOBaseTable::get_row;
    virtual const TBaseTableRow& get_row(const std::string &field, const std::string &value, bool with_deleted=false);
    TAirlines() {
        Init( get_db_table_name() );
    }
};

class TClassesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TClassesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TClasses: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TClasses"; };
    std::string get_db_table_name() const { return "CLASSES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TClasses() {
        Init( get_db_table_name() );
    }
};

class TSubclsRow: public TCodeBaseTableRow {
  public:
    std::string cl;
    const char *get_row_name() const { return "TSubclsRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="cl") return cl;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TSubcls: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TSubcls"; };
    std::string get_db_table_name() const { return "SUBCLS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TSubcls() {
        Init( get_db_table_name() );
    }
};

class TTripSuffixesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TTripSuffixesRow"; };
};

class TTripSuffixes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TTripSuffixes"; };
    std::string get_db_table_name() const { return "TRIP_SUFFIXES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TTripSuffixes() {
        Init( get_db_table_name() );
    }
};

class TCraftsRow: public TICAOBaseTableRow {
  public:
    const char *get_row_name() const { return "TCraftsRow"; };
};

class TCrafts: public TICAOBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TCrafts"; };
    std::string get_db_table_name() const { return "CRAFTS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCrafts( ) {
        Init( get_db_table_name() );
    }
};

class TCustomAlarmTypesRow: public TTIDBaseTableRow {
  public:
    std::string airline;
    const char *get_row_name() const { return "TCustomAlarmTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="airline") return airline;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TCustomAlarmTypes: public TTIDBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return "SELECT custom_alarm_types.*, LPAD(TO_CHAR(id),9,'0') AS code FROM custom_alarm_types";
    };
    const char *get_refresh_sql_text()
    {
      static const std::string result = (std::string)get_select_sql_text() + " WHERE tid>:tid";
      return result.c_str();
    };
  protected:
    const char *get_bt_class_name() const { return "TCustomAlarmTypes"; };
    std::string get_db_table_name() const { return "CUSTOM_ALARM_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TCurrencyRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TCurrencyRow"; };
};

class TCurrency: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TCurrency"; };
    std::string get_db_table_name() const { return "CURRENCY"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCurrency() {
        Init( get_db_table_name() );
    }
};

class TRefusalTypesRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TRefusalTypesRow"; };
};

class TRefusalTypes: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TRefusalTypes"; };
    std::string get_db_table_name() const { return "REFUSAL_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TRefusalTypes( ) {
        Init( get_db_table_name() );
    }
};

class TPayTypesRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TPayTypesRow"; };
};

class TPayTypes: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TPayTypes"; };
    std::string get_db_table_name() const { return "PAY_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TPayTypes( ) {
        Init( get_db_table_name() );
    }
};

class TRcptDocTypesRow: public TTIDBaseTableRow {
  public:
    std::string code_pax_doc;
    std::string code_mintrans;
    const char *get_row_name() const { return "TRcptDocTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_pax_doc") return code_pax_doc;
      if (lowerc(field)=="code_mintrans") return code_mintrans;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TRcptDocTypes: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TRcptDocTypes"; };
    std::string get_db_table_name() const { return "RCPT_DOC_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TRcptDocTypes( ) {
        Init( get_db_table_name() );
    }
};

class TTripTypesRow: public TTIDBaseTableRow {
  public:
    int pr_reg;
    const char *get_row_name() const { return "TTripTypesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="pr_reg") return pr_reg;
      return TTIDBaseTableRow::AsInteger(field);
    };
};

class TTripTypes: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TTripTypes"; };
    std::string get_db_table_name() const { return "TRIP_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TTripTypes() {
        Init( get_db_table_name() );
    }
};

class TClsGrpRow: public TTIDBaseTableRow {
  public:
    std::string airline,airp,cl;
    int priority;
    const char *get_row_name() const { return "TClsGrpRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TTIDBaseTableRow::AsInteger(field);
    };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="airline") return airline;
      if (lowerc(field)=="airp") return airp;
      if (lowerc(field)=="cl") return cl;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TClsGrp: public TTIDBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TClsGrp"; };
    std::string get_db_table_name() const { return "CLS_GRP"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TClsGrp() {
        Init( get_db_table_name() );
    }
};

class TAlarmTypesRow: public TCodeBaseTableRow {
    public:
        const char *get_row_name() const { return "TAlarmTypesRow"; };
};

class TAlarmTypes: public TCodeBaseTable {
    protected:
        const char *get_bt_class_name() const { return "TAlarmTypes"; };
        std::string get_db_table_name() const { return "ALARM_TYPES"; }
        void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
        void Invalidate() {}; //всегда актуальна
    public:
        TAlarmTypes() {
            Init( get_db_table_name() );
        }
};

class TDevModelsRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevModelsRow"; };
};

class TDevModels: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TDevModels"; };
    std::string get_db_table_name() const { return "DEV_MODELS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevModels() {
        Init( get_db_table_name() );
    }
};

class TDevSessTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevSessTypesRow"; };
};

class TDevSessTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TDevSessTypes"; };
    std::string get_db_table_name() const { return "DEV_SESS_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevSessTypes( ) {
        Init( get_db_table_name() );
    }
};

class TDevFmtTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevFmtTypesRow"; };
};

class TDevFmtTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TDevFmtTypes"; };
    std::string get_db_table_name() const { return "DEV_FMT_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevFmtTypes() {
        Init( get_db_table_name() );
    }
};

class TDevOperTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevOperTypesRow"; };
};

class TDevOperTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TDevOperTypes"; };
    std::string get_db_table_name() const { return "DEV_OPER_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevOperTypes() {
        Init( get_db_table_name() );
    }
};

class TGrpStatusTypesRow: public TCodeBaseTableRow {
  public:
    std::string layer_type;
    int priority;
    const char *get_row_name() const { return "TGrpStatusTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="layer_type") return layer_type;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    }
};

class TGrpStatusTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TGrpStatusTypes"; };
    std::string get_db_table_name() const { return "GRP_STATUS_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TGrpStatusTypes() {
        Init( get_db_table_name() );
    }
};

class TClientTypesRow: public TCodeBaseTableRow {
  public:
    std::string short_name,short_name_lat;
    int priority;
    const char *get_row_name() const { return "TClientTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    }
};

class TClientTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TClientTypes"; };
    std::string get_db_table_name() const { return "CLIENT_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TClientTypes() {
        Init( get_db_table_name() );
    }
};

class TCompLayerTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TCompLayerTypesRow"; };
};

class TCompLayerTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TCompLayerTypes"; };
    std::string get_db_table_name() const { return "COMP_LAYER_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TCompLayerTypes() {
        Init( get_db_table_name() );
    }
};

class TGraphStagesRow: public TIdBaseTableRow {
      public:
    int stage_time;
    bool pr_auto, pr_airp_stage;
    const char *get_row_name() const { return "TGraphStagesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="time") return stage_time;
      return TIdBaseTableRow::AsInteger(field);
    }
    bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="pr_auto") return pr_auto;
      if (lowerc(field)=="pr_airp_stage") return pr_airp_stage;
      return TIdBaseTableRow::AsBoolean(field);
    }
};

class TGraphStages: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TGraphStages"; };
    std::string get_db_table_name() const { return "GRAPH_STAGES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TGraphStages() {
        Init();
        select_sql = "SELECT stage_id id,name,name_lat,time,pr_auto,pr_airp_stage FROM graph_stages";
    };
};

class TMiscSetTypesRow: public TIdBaseTableRow {
      public:
    const char *get_row_name() const { return "TMiscSetTypesRow"; };
};

class TMiscSetTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TMiscSetTypes"; };
    std::string get_db_table_name() const { return "MISC_SET_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TMiscSetTypes() {
        Init();
        select_sql = "SELECT code id,name,name_lat FROM misc_set_types";
    };
};

class TSeatAlgoTypesRow: public TIdBaseTableRow {
      public:
    const char *get_row_name() const { return "TSeatAlgoTypesRow"; };
};

class TSeatAlgoTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TSeatAlgoTypes"; };
    std::string get_db_table_name() const { return "SEAT_ALGO_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TSeatAlgoTypes() {
        Init( get_db_table_name() );
    };
};

class TRightsRow: public TIdBaseTableRow {
      public:
    const char *get_row_name() const { return "TRightsRow"; };
};

class TRights: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TRights"; };
    std::string get_db_table_name() const { return "RIGHTS_LIST"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TRights() {
        Init();
        select_sql = "SELECT ida AS id,name,name_lat FROM rights_list";
    };
};

class TUserTypesRow: public TIdBaseTableRow {
      public:
    const char *get_row_name() const { return "TUserTypesRow"; };
};

class TUserTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TUserTypes"; };
    std::string get_db_table_name() const { return "USER_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TUserTypes() {
        Init();
        select_sql = "SELECT code AS id,name,name_lat FROM user_types";
    };
};

class TUserSetTypesRow: public TIdBaseTableRow {
  public:
    std::string short_name,short_name_lat,category;
    const char *get_row_name() const { return "TUserSetTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="category") return category;
      return TIdBaseTableRow::AsString(field,lang);
    };
};

class TUserSetTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TUserSetTypes"; };
    std::string get_db_table_name() const { return "USER_SET_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TUserSetTypes() {
        Init();
        select_sql = "SELECT code AS id,category,short_name,short_name_lat,name,name_lat "
                   "FROM user_set_types";
    }
};

class TBagNormTypesRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TBagNormTypesRow"; };
};

class TBagNormTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TBagNormTypes"; };
    std::string get_db_table_name() const { return "BAG_NORM_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TBagNormTypes() {
        Init( get_db_table_name() );
    };
};

class TBagTypesRow: public TIdBaseTableRow {
    public:
        std::string rem_code_lci, rem_code_ldm;
        const char *get_row_name() const { return "TBagTypesRow"; };
};

class TBagTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TBagTypes"; };
    std::string get_db_table_name() const { return "BAG_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TBagTypes() {
        Init();
        select_sql =
            "select "
            "   code id, "
            "   name, "
            "   name_lat, "
            "   rem_code_lci, "
            "   rem_code_ldm "
            "from bag_types";
    };
};

class TLangTypesRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TLangTypesRow"; };
};

class TLangTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TLangTypes"; };
    std::string get_db_table_name() const { return "LANG_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TLangTypes() {
        Init( get_db_table_name() );
    };
};

class TStationModesRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TStationModesRow"; };
};

class TStationModes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TStationModes"; };
    std::string get_db_table_name() const { return "FAKE_DUAL_BT"; } // !!!
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TStationModes();
};

class TSeasonTypesRow: public TIdBaseTableRow {
    public:
    const char *get_row_name() const { return "TSeasonTypesRow"; };
};

class TSeasonTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TSeasonTypes"; };
    std::string get_db_table_name() const { return "FAKE_DUAL_BT"; } // !!!
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TSeasonTypes();
};

class TFormTypesRow: public TCodeBaseTableRow {
    public:
      std::string basic_type, validator;
      int series_len, no_len;
      bool pr_check_bit;
    const char *get_row_name() const { return "TFormTypesRow"; };
    std::string AsString(const std::string &field, const std::string &lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="basic_type") return basic_type;
      if (lowerc(field)=="validator") return validator;
      return TCodeBaseTableRow::AsString(field);
    }
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="series_len") return series_len;
      if (lowerc(field)=="no_len") return no_len;
      return TCodeBaseTableRow::AsInteger(field);
    }
    bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="pr_check_bit") return pr_check_bit;
      return TCodeBaseTableRow::AsBoolean(field);
    }
};

class TFormTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TFormTypes"; };
    std::string get_db_table_name() const { return "FORM_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TFormTypes() {
    Init( get_db_table_name() );
    };
};

class TCkinRemTypesRow: public TTIDBaseTableRow {
  public:
    int grp_id;
    bool is_iata;
    int priority;
    const char *get_row_name() const { return "TCkinRemTypesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="grp_id") return grp_id;
      if (lowerc(field)=="priority") return priority;
      return TTIDBaseTableRow::AsInteger(field);
    };
    bool AsBoolean(const std::string &field) const
    {
      if (lowerc(field)=="is_iata") return is_iata;
      return TTIDBaseTableRow::AsBoolean(field);
    }
};

class TMsgTransportsRow: public TCodeBaseTableRow {
    public:
        const char *get_row_name() const { return "TMsgTransportsRow"; };
};

class TMsgTransports: public TCodeBaseTable {
    protected:
        const char *get_bt_class_name() const { return "TMsgTransports"; };
        std::string get_db_table_name() const { return "MSG_TRANSPORTS"; }
        void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
        void Invalidate() {}; //всегда актуальна
    public:
        TMsgTransports() {
            Init( get_db_table_name() );
        }
};

class TCkinRemTypes: public TTIDBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT ckin_rem_types.id, ckin_rem_types.code, ckin_rem_types.code_lat, "
        "       ckin_rem_types.name, ckin_rem_types.name_lat, ckin_rem_types.grp_id, "
        "       ckin_rem_types.is_iata, ckin_rem_types.pr_del, ckin_rem_types.tid, "
        "       rem_grp.priority "
        "FROM ckin_rem_types, rem_grp "
        "WHERE ckin_rem_types.grp_id=rem_grp.id";
    };
    const char *get_refresh_sql_text()
    {
      return
        "SELECT ckin_rem_types.id, ckin_rem_types.code, ckin_rem_types.code_lat, "
        "       ckin_rem_types.name, ckin_rem_types.name_lat, ckin_rem_types.grp_id, "
        "       ckin_rem_types.is_iata, ckin_rem_types.pr_del, ckin_rem_types.tid, "
        "       rem_grp.priority "
        "FROM ckin_rem_types, rem_grp "
        "WHERE ckin_rem_types.grp_id=rem_grp.id AND ckin_rem_types.tid>:tid";
    };
  protected:
    const char *get_bt_class_name() const { return "TCkinRemTypes"; };
    std::string get_db_table_name() const { return "CKIN_REM_TYPES"; } // + REM_GRP
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TRateColorsRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TRateColorsRow"; };
};

class TRateColors: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TRateColors"; };
    std::string get_db_table_name() const { return "RATE_COLORS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TRateColors() {
        Init( get_db_table_name() );
    };
};

class TBIPrintTypesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TBIPrintTypesRow"; };
    int AsInteger(const std::string &field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TBIPrintTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TBIPrintTypes"; };
    std::string get_db_table_name() const { return "BI_PRINT_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TBIPrintTypes() {
        Init( get_db_table_name() );
    };
};

class TVoucherTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TVoucherTypesRow"; };
};

class TVoucherTypes: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TVoucherTypes"; };
    std::string get_db_table_name() const { return "VOUCHER_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TVoucherTypes() {
        Init( get_db_table_name() );
    }
};

class DCSActionsRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "DCSActionsRow"; }
};

class DCSActions: public TCodeBaseTable {
  protected:
    const char *get_bt_class_name() const { return "DCSActions"; }
    std::string get_db_table_name() const { return "DCS_ACTIONS"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {} //всегда актуальна
  public:
    DCSActions() {
        Init( get_db_table_name() );
    }
};

class TPayMethodTypesRow: public TIdBaseTableRow {
    public:
    const char *get_row_name() const { return "TPayMethodTypesRow"; }
};

class TPayMethodTypes: public TIdBaseTable {
  protected:
    const char *get_bt_class_name() const { return "TPayMethodTypes"; }
    std::string get_db_table_name() const { return "PAY_METHODS_TYPES"; }
    void create_row(DB::TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}
  public:
    TPayMethodTypes() {
        Init( get_db_table_name() );
    }
};


class TBaseTables {
    private:
        typedef std::map<std::string, TBaseTable *> TTables;
        TTables base_tables;
        TMemoryManager mem;
    public:
        TBaseTables();
        TBaseTable &get(const std::string &name);
        void Clear();
        void Invalidate();
        ~TBaseTables() { Clear(); };
};

extern TBaseTables base_tables;


#endif
