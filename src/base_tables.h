#ifndef _BASE_TABLES_H_
#define _BASE_TABLES_H_

#include <map>
#include <vector>
#include <string>
#include "exceptions.h"
#include "oralib.h"
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
    virtual std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      throw EBaseTableError("%s::AsString: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual int AsInteger(std::string field) const
    {
      throw EBaseTableError("%s::AsInteger: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual bool AsBoolean(std::string field) const
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
    virtual const char *get_table_name() = 0;
    virtual void create_variables(TQuery &Qry, bool pr_refresh) = 0;
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) = 0;
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
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    virtual const TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
    virtual void Invalidate();
};

class TNameBaseTableRow: public TBaseTableRow { //name, name_lat
    protected:
    public:
        std::string name, name_lat;
    virtual bool deleted() { return false; };
    virtual std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
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
        virtual void create_variables(TQuery &Qry, bool pr_refresh) {};
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void after_update() {};
  public:
};

class TIdBaseTableRow : public TNameBaseTableRow {
  public:
    int id;
    virtual int AsInteger(std::string field) const
    {
      if (lowerc(field)=="id") return id;
      return TNameBaseTableRow::AsInteger(field);
    };
};

class TIdBaseTable: public TNameBaseTable {
  private:
    std::map<int, TBaseTableRow*> id;
  protected:
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TNameBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
};


class TCodeBaseTableRow : public TNameBaseTableRow {
  public:
    std::string code,code_lat;
    virtual std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
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
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TNameBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
};

class TTIDBaseTableRow : public TCodeBaseTableRow {
  public:
    int id;
    bool pr_del;
    virtual bool deleted() { return pr_del; };
    virtual int AsInteger(std::string field) const
    {
      if (lowerc(field)=="id") return id;
      return TCodeBaseTableRow::AsInteger(field);
    };
    virtual bool AsBoolean(std::string field) const
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
    virtual void create_variables(TQuery &Qry, bool pr_refresh);
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
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
    virtual const TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
};

class TICAOBaseTableRow : public TTIDBaseTableRow {
  public:
    std::string code_icao,code_icao_lat;
    virtual std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
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
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
};
///////////////////////////////////////////////////////////////////
class TCountriesRow: public TTIDBaseTableRow {
  public:
    std::string code_iso;
    const char *get_row_name() const { return "TCountriesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_iso") return code_iso;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TCountries: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code_iso;
  protected:
    const char *get_table_name() { return "TCountries"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void delete_row(TBaseTableRow *row);
    void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    TCountries( ) {
          Init("countries");
    }
};

class TAirpsRow: public TICAOBaseTableRow {
  public:
    std::string city;
    const char *get_row_name() const { return "TAirpsRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="city") return city;
      return TICAOBaseTableRow::AsString(field,lang);
    };
};

class TAirps: public TICAOBaseTable {
  protected:
    const char *get_table_name() { return "TAirps"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TAirps( ) {
          Init("airps");
    }
};

class TPersTypesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TPersTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TPersTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TPersTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TPersTypes() {
          Init("pers_types");
      }
};

class TExtendedPersTypes: public TPersTypes {
  protected:
    const char *get_table_name() { return "TExtendedPersTypes"; }
  public:
    TExtendedPersTypes() {
          Init();
          select_sql =
            "SELECT code, code_lat, name, name_lat, priority FROM pers_types "
            "UNION "
            "SELECT '??', 'CBBG', '?????? ?????', 'Cabin baggage', 4 FROM dual";
      }
};

class TGenderTypesRow: public TCodeBaseTableRow {
  public:
    bool pr_inf;
    const char *get_row_name() const { return "TGenderTypesRow"; };
    bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="pr_inf") return pr_inf;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TGenderTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TGenderTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TGenderTypes() {
        Init( "gender_types" );
    }
};

class TReportTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TReportTypesRow"; };
};

class TReportTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TReportTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TReportTypes() {
        Init( "report_types" );
    }
};

class TTagColorsRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TTagColorsRow"; };
};

class TTagColors: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TTagColors"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TTagColors() {
        Init( "tag_colors" );
    }
};

class TPaxDocCountriesRow: public TTIDBaseTableRow {
  public:
    std::string country;
    const char *get_row_name() const { return "TPaxDocCountriesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="country") return country;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TPaxDocCountries: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> country;
  protected:
    const char *get_table_name() { return "TPaxDocCountries"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void delete_row(TBaseTableRow *row);
    void add_row(TBaseTableRow *row);
  public:
    using TTIDBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    TPaxDocCountries( ) {
          Init("pax_doc_countries");
    }
};

class TPaxDocTypesRow: public TCodeBaseTableRow {
  public:
    std::string code_mintrans;
    bool is_docs_type, is_doco_type;
    const char *get_row_name() const { return "TPaxDocTypesRow"; }
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_mintrans") return code_mintrans;
      return TCodeBaseTableRow::AsString(field,lang);
    }
    bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="is_docs_type") return is_docs_type;
      if (lowerc(field)=="is_doco_type") return is_doco_type;
      return TCodeBaseTableRow::AsBoolean(field);
    }
};

class TPaxDocTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TPaxDocTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TPaxDocTypes( ) {
        Init( "pax_doc_types" );
    }
};

class TPaxDocSubtypesRow: public TCodeBaseTableRow {
  public:
    std::string doc_subtype, doc_type;
    const char *get_row_name() const { return "TPaxDocSubtypesRow"; }
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="doc_subtype") return doc_subtype;
      if (lowerc(field)=="doc_type") return doc_type;
      return TCodeBaseTableRow::AsString(field,lang);
    }
};

class TPaxDocSubtypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TPaxDocSubtypes"; }
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {} //?????? ?????????
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
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TTypeBOptionValues: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TTypeBOptionValues"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="basic_type") return basic_type;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="pr_dep") return pr_dep;
      return TCodeBaseTableRow::AsInteger(field);
    };
    virtual bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="editable") return editable;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TTypeBTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TTypeBTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TTypeBTypes() {
          Init("typeb_types");
      }
};

class TCitiesRow: public TTIDBaseTableRow {
  public:
    std::string country,tz_region;
    const char *get_row_name() const { return "TCitiesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="country") return country;
      if (lowerc(field)=="tz_region") return tz_region;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TCities: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TCities"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCities() {
        Init( "cities" );
    }
};

class TAirlinesRow: public TICAOBaseTableRow {
  public:
    std::string aircode,short_name,short_name_lat,city;
    const char *get_row_name() const { return "TAirlinesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
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
    const char *get_table_name() { return "TAirlines"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    using TICAOBaseTable::get_row;
    virtual const TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    TAirlines() {
        Init( "airlines" );
    }
};

class TClassesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TClassesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TClasses: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TClasses"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TClasses() {
        Init( "classes" );
    }
};

class TSubclsRow: public TCodeBaseTableRow {
  public:
    std::string cl;
    const char *get_row_name() const { return "TSubclsRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="cl") return cl;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TSubcls: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TSubcls"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TSubcls() {
        Init( "subcls" );
    }
};

class TTripSuffixesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TTripSuffixesRow"; };
};

class TTripSuffixes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TTripSuffixes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TTripSuffixes() {
        Init( "trip_suffixes" );
    }
};

class TCraftsRow: public TICAOBaseTableRow {
  public:
    const char *get_row_name() const { return "TCraftsRow"; };
};

class TCrafts: public TICAOBaseTable {
  protected:
    const char *get_table_name() { return "TCrafts"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCrafts( ) {
        Init( "crafts" );
    }
};

class TCustomAlarmTypesRow: public TTIDBaseTableRow {
  public:
    std::string airline;
    const char *get_row_name() const { return "TCustomAlarmTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
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
    const char *get_table_name() { return "TCustomAlarmTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TCurrencyRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TCurrencyRow"; };
};

class TCurrency: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TCurrency"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TCurrency() {
        Init( "currency" );
    }
};

class TRefusalTypesRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TRefusalTypesRow"; };
};

class TRefusalTypes: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TRefusalTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TRefusalTypes( ) {
        Init( "refusal_types" );
    }
};

class TPayTypesRow: public TTIDBaseTableRow {
  public:
    const char *get_row_name() const { return "TPayTypesRow"; };
};

class TPayTypes: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TPayTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TPayTypes( ) {
        Init( "pay_types" );
    }
};

class TRcptDocTypesRow: public TTIDBaseTableRow {
  public:
    std::string code_pax_doc;
    std::string code_mintrans;
    const char *get_row_name() const { return "TRcptDocTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="code_pax_doc") return code_pax_doc;
      if (lowerc(field)=="code_mintrans") return code_mintrans;
      return TTIDBaseTableRow::AsString(field,lang);
    };
};

class TRcptDocTypes: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TRcptDocTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TRcptDocTypes( ) {
        Init( "rcpt_doc_types" );
    }
};

class TTripTypesRow: public TTIDBaseTableRow {
  public:
    int pr_reg;
    const char *get_row_name() const { return "TTripTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="pr_reg") return pr_reg;
      return TTIDBaseTableRow::AsInteger(field);
    };
};

class TTripTypes: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TTripTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TTripTypes() {
        Init( "trip_types" );
    }
};

class TClsGrpRow: public TTIDBaseTableRow {
  public:
    std::string airline,airp,cl;
    int priority;
    const char *get_row_name() const { return "TClsGrpRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TTIDBaseTableRow::AsInteger(field);
    };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="airline") return airline;
      if (lowerc(field)=="airp") return airp;
      if (lowerc(field)=="cl") return cl;
      return TCodeBaseTableRow::AsString(field,lang);
    };
};

class TClsGrp: public TTIDBaseTable {
  protected:
    const char *get_table_name() { return "TClsGrp"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TClsGrp() {
        Init( "cls_grp" );
    }
};

class TAlarmTypesRow: public TCodeBaseTableRow {
    public:
        const char *get_row_name() const { return "TAlarmTypesRow"; };
};

class TAlarmTypes: public TCodeBaseTable {
    protected:
        const char *get_table_name() { return "TAlarmTypes"; };
        void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
        void Invalidate() {}; //?????? ?????????
    public:
        TAlarmTypes() {
            Init( "alarm_types" );
        }
};

class TDevModelsRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevModelsRow"; };
};

class TDevModels: public TCodeBaseTable {
  protected:
        const char *get_table_name() { return "TDevModels"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevModels() {
        Init( "dev_models" );
    }
};

class TDevSessTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevSessTypesRow"; };
};

class TDevSessTypes: public TCodeBaseTable {
  protected:
        const char *get_table_name() { return "TDevSessTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevSessTypes( ) {
        Init( "dev_sess_types" );
    }
};

class TDevFmtTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevFmtTypesRow"; };
};

class TDevFmtTypes: public TCodeBaseTable {
  protected:
        const char *get_table_name() { return "TDevFmtTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevFmtTypes() {
        Init( "dev_fmt_types" );
    }
};

class TDevOperTypesRow: public TCodeBaseTableRow {
    public:
      const char *get_row_name() const { return "TDevOperTypesRow"; };
};

class TDevOperTypes: public TCodeBaseTable {
  protected:
        const char *get_table_name() { return "TDevOperTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
  public:
    TDevOperTypes() {
        Init( "dev_oper_types" );
    }
};

class TGrpStatusTypesRow: public TCodeBaseTableRow {
  public:
    std::string layer_type;
    int priority;
    const char *get_row_name() const { return "TGrpStatusTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="layer_type") return layer_type;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    }
};

class TGrpStatusTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TGrpStatusTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TGrpStatusTypes() {
        Init( "grp_status_types" );
    }
};

class TClientTypesRow: public TCodeBaseTableRow {
  public:
    std::string short_name,short_name_lat;
    int priority;
    const char *get_row_name() const { return "TClientTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      return TCodeBaseTableRow::AsString(field,lang);
    };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    }
};

class TClientTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TClientTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TClientTypes() {
        Init( "client_types" );
    }
};

class TCompLayerTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TCompLayerTypesRow"; };
};

class TCompLayerTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TCompLayerTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TCompLayerTypes() {
        Init( "comp_layer_types" );
    }
};

class TGraphStagesRow: public TIdBaseTableRow {
      public:
    int stage_time;
    bool pr_auto, pr_airp_stage;
    const char *get_row_name() const { return "TGraphStagesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="time") return stage_time;
      return TIdBaseTableRow::AsInteger(field);
    }
    bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="pr_auto") return pr_auto;
      if (lowerc(field)=="pr_airp_stage") return pr_airp_stage;
      return TIdBaseTableRow::AsBoolean(field);
    }
};

class TGraphStages: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TGraphStages"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    const char *get_table_name() { return "TMiscSetTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    const char *get_table_name() { return "TSeatAlgoTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TSeatAlgoTypes() {
        Init( "seat_algo_types" );
    };
};

class TRightsRow: public TIdBaseTableRow {
      public:
    const char *get_row_name() const { return "TRightsRow"; };
};

class TRights: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TRights"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    const char *get_table_name() { return "TUserTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="category") return category;
      return TIdBaseTableRow::AsString(field,lang);
    };
};

class TUserSetTypes: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TUserSetTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    const char *get_table_name() { return "TBagNormTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TBagNormTypes() {
        Init( "bag_norm_types" );
    };
};

class TBagTypesRow: public TIdBaseTableRow {
    public:
        std::string rem_code_lci, rem_code_ldm;
        const char *get_row_name() const { return "TBagTypesRow"; };
};

class TBagTypes: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TBagTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
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
    const char *get_table_name() { return "TLangTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TLangTypes() {
        Init( "lang_types" );
    };
};

class TStationModesRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TStationModesRow"; };
};

class TStationModes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TStationModes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TStationModes() {
      Init();
      select_sql =
        "SELECT '?' AS code, '???????????' AS name, 'Check-in' AS name_lat FROM dual "
        "UNION "
        "SELECT '?', '???????', 'Boarding' FROM dual";
    };
};

class TSeasonTypesRow: public TIdBaseTableRow {
    public:
    const char *get_row_name() const { return "TSeasonTypesRow"; };
};

class TSeasonTypes: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TSeasonTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TSeasonTypes() {
      Init();
      select_sql =
        "SELECT 0 AS id, '????' AS name, 'Winter' AS name_lat FROM dual "
        "UNION "
        "SELECT 1, '????', 'Summer' FROM dual";
    };
};

class TFormTypesRow: public TCodeBaseTableRow {
    public:
      std::string basic_type, validator;
      int series_len, no_len;
      bool pr_check_bit;
    const char *get_row_name() const { return "TFormTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="basic_type") return basic_type;
      if (lowerc(field)=="validator") return validator;
      return TCodeBaseTableRow::AsString(field);
    }
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="series_len") return series_len;
      if (lowerc(field)=="no_len") return no_len;
      return TCodeBaseTableRow::AsInteger(field);
    }
    bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="pr_check_bit") return pr_check_bit;
      return TCodeBaseTableRow::AsBoolean(field);
    }
};

class TFormTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TFormTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TFormTypes() {
    Init( "form_types" );
    };
};

class TCkinRemTypesRow: public TTIDBaseTableRow {
  public:
    int grp_id;
    bool is_iata;
    int priority;
    const char *get_row_name() const { return "TCkinRemTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="grp_id") return grp_id;
      if (lowerc(field)=="priority") return priority;
      return TTIDBaseTableRow::AsInteger(field);
    };
    bool AsBoolean(std::string field) const
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
        const char *get_table_name() { return "TMsgTransports"; };
        void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
        void Invalidate() {}; //?????? ?????????
    public:
        TMsgTransports() {
            Init( "msg_transports" );
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
    const char *get_table_name() { return "TCkinRemTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TRateColorsRow: public TCodeBaseTableRow {
    public:
    const char *get_row_name() const { return "TRateColorsRow"; };
};

class TRateColors: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TRateColors"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TRateColors() {
        Init( "rate_colors" );
    };
};

class TBIPrintTypesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TBIPrintTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TBIPrintTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TBIPrintTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TBIPrintTypes() {
        Init( "bi_print_types" );
    };
};

class TVoucherTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TVoucherTypesRow"; };
};

class TVoucherTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TVoucherTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //?????? ?????????
  public:
    TVoucherTypes() {
        Init( "voucher_types" );
    }
};

class DCSActionsRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "DCSActionsRow"; }
};

class DCSActions: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "DCSActions"; }
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {} //?????? ?????????
  public:
    DCSActions() {
        Init( "dcs_actions" );
    }
};

class TPayMethodTypesRow: public TIdBaseTableRow {
    public:
    const char *get_row_name() const { return "TPayMethodTypesRow"; }
};

class TPayMethodTypes: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TPayMethodTypes"; }
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}
  public:
    TPayMethodTypes() {
        Init( "pay_methods_types" );
    }
};


class TBaseTables {
    private:
        typedef std::map<std::string, TBaseTable *> TTables;
        TTables base_tables;
        TMemoryManager mem;
    public:
        TBaseTables();
        TBaseTable &get(std::string name);
        void Clear();
        void Invalidate();
        ~TBaseTables() { Clear(); };
};

extern TBaseTables base_tables;


#endif
