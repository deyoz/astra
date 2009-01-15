#ifndef _BASE_TABLES_H_
#define _BASE_TABLES_H_

#include <map>
#include <vector>
#include <string>
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"

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
    virtual const char *get_row_name() = 0;
    virtual bool deleted() = 0;
    virtual std::string AsString(std::string field, bool pr_lat=false)
    {
      throw EBaseTableError("%s::AsString: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual int AsInteger(std::string field)
    {
      throw EBaseTableError("%s::AsInteger: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
    virtual bool AsBoolean(std::string field)
    {
      throw EBaseTableError("%s::AsBoolean: wrong field '%s'",
                             get_row_name(),field.c_str());
    };
};

class TBaseTable {
  private:
   	bool pr_init,pr_actual;
   	std::vector<TBaseTableRow*> table;
    virtual const char *get_select_sql_text() = 0;
    virtual const char *get_refresh_sql_text() = 0;
  protected:
    void load_table();
    virtual const char *get_table_name() = 0;
    virtual void create_variables(TQuery &Qry, bool pr_refresh) = 0;
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) = 0;
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
    virtual void after_update() = 0;
  public:
	  TBaseTable() {pr_init=false; pr_actual=false;};
    virtual ~TBaseTable()
    {
      std::vector<TBaseTableRow*>::iterator i;
      for(i=table.begin();i!=table.end();i++) delete *i;
    };
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    virtual TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
    void Invalidate() { pr_actual=false; };
};

class TCodeBaseTableRow : public TBaseTableRow {
  public:
    std::string code,code_lat;
    virtual ~TCodeBaseTableRow() {};
    virtual bool deleted() { return false; };
    virtual std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="code") return pr_lat?code_lat:code;
      return TBaseTableRow::AsString(field,pr_lat);
    };
};

class TCodeBaseTable: public TBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code;
    std::map<std::string, TBaseTableRow*> code_lat;
  protected:
    virtual void create_variables(TQuery &Qry, bool pr_refresh) {};
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
    virtual void after_update() {};
  public:
    TCodeBaseTable() {};
    virtual ~TCodeBaseTable() {};
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
};

class TTIDBaseTableRow : public TCodeBaseTableRow {
  public:
    int id;
    bool pr_del;
    virtual ~TTIDBaseTableRow() {};
    virtual bool deleted() { return pr_del; };
    virtual int AsInteger(std::string field)
    {
      if (lowerc(field)=="id") return id;
      return TCodeBaseTableRow::AsInteger(field);
    };
    virtual bool AsBoolean(std::string field)
    {
      if (lowerc(field)=="pr_del") return pr_del;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TTIDBaseTable: public TCodeBaseTable {
  private:
    int tid,new_tid;
    std::map<int, TBaseTableRow*> id;
  protected:
    virtual void create_variables(TQuery &Qry, bool pr_refresh);
    virtual void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
    virtual void after_update();
  public:
    TTIDBaseTable() {tid=-1; new_tid=-1;};
    virtual ~TTIDBaseTable() {};
    virtual TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
};

class TICAOBaseTableRow : public TTIDBaseTableRow {
  public:
    std::string code_icao,code_icao_lat;
    virtual ~TICAOBaseTableRow() {};
    virtual std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="code_icao") return pr_lat?code_icao_lat:code_icao;
      return TTIDBaseTableRow::AsString(field,pr_lat);
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
    TICAOBaseTable() {};
    virtual ~TICAOBaseTable() {};
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);

};

class TCountriesRow: public TTIDBaseTableRow {
  public:
    std::string code_iso,name,name_lat;
    ~TCountriesRow() {};
    const char *get_row_name() { return "TCountriesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="code_iso") return code_iso;
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      return TTIDBaseTableRow::AsString(field,pr_lat);
    };
};

class TCountries: public TTIDBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> code_iso;
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_iso,name,name_lat,pr_del,tid "
   	    "FROM countries";
    };
    const char *get_refresh_sql_text()
    {
      return
    	  "SELECT id,code,code_lat,code_iso,name,name_lat,pr_del,tid "
    	  "FROM countries WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TCountries"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void delete_row(TBaseTableRow *row);
    void add_row(TBaseTableRow *row);
  public:
    TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
};

class TAirpsRow: public TICAOBaseTableRow {
  public:
    std::string name,name_lat,city;
    ~TAirpsRow() {};
    const char *get_row_name() { return "TAirpsRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      if (lowerc(field)=="city") return city;
      return TICAOBaseTableRow::AsString(field,pr_lat);
    };
};

class TAirps: public TICAOBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_icao,code_icao_lat,name,name_lat,city,pr_del,tid "
   	    "FROM airps";
    };
    const char *get_refresh_sql_text()
    {
      return
    	  "SELECT id,code,code_lat,code_icao,code_icao_lat,name,name_lat,city,pr_del,tid "
    	  "FROM airps WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TAirps"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TPersTypesRow: public TCodeBaseTableRow {
  public:
    std::string name;
    int priority,weight_win,weight_sum;
    ~TPersTypesRow() {};
    const char *get_row_name() { return "TPersTypesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return name;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
    int AsInteger(std::string field)
    {
      if (lowerc(field)=="priority") return priority;
      if (lowerc(field)=="weight_win") return weight_win;
      if (lowerc(field)=="weight_sum") return weight_sum;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TPersTypes: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code,code_lat,name,priority,weight_win,weight_sum FROM pers_types";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TPersTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};

class TGenderTypesRow: public TCodeBaseTableRow {
  public:
    std::string name;
    bool pr_inf;
    ~TGenderTypesRow() {};
    const char *get_row_name() { return "TGenderTypesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return name;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
    bool AsBoolean(std::string field)
    {
      if (lowerc(field)=="pr_inf") return pr_inf;
      return TCodeBaseTableRow::AsBoolean(field);
    };
};

class TGenderTypes: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code,code AS code_lat,name,pr_inf FROM gender_types";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TGenderTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};


class TTagColorsRow: public TCodeBaseTableRow {
  public:
    std::string name, name_lat;
    ~TTagColorsRow() {};
    const char *get_row_name() { return "TTagColorsRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat ? name_lat : name;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
};

class TPaxDocTypesRow: public TCodeBaseTableRow {
  public:
    std::string name;
    ~TPaxDocTypesRow() {};
    const char *get_row_name() { return "TPaxDocTypesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return name;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
};

class TTagColors: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code, code_lat, name, name_lat FROM tag_colors";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TTagColors"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};

class TPaxDocTypes: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code,code AS code_lat,name FROM pax_doc_types";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TPaxDocTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};

class TCitiesRow: public TTIDBaseTableRow {
  public:
    std::string name,name_lat,country,region;
    int tz;
    ~TCitiesRow() {};
    const char *get_row_name() { return "TCitiesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      if (lowerc(field)=="country") return country;
      if (lowerc(field)=="region") return region;
      return TTIDBaseTableRow::AsString(field,pr_lat);
    };
    int AsInteger(std::string field)
    {
      if (lowerc(field)=="tz") return tz;
      return TTIDBaseTableRow::AsInteger(field);
    };
};

class TCities: public TTIDBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,name,name_lat, "
        "       cities.country,cities.tz, "
        "       DECODE(tz_regions.pr_del,0,region,NULL) AS region,cities.pr_del, "
        "       GREATEST(cities.tid,NVL(tz_regions.tid, cities.tid)) AS tid "
        "FROM cities,tz_regions "
        "WHERE cities.country=tz_regions.country(+) AND "
        "      cities.tz=tz_regions.tz(+)";
    };
    const char *get_refresh_sql_text()
    {
      return
      	"SELECT id,code,code_lat,name,name_lat, "
      	"       cities.country,cities.tz, "
        "       DECODE(tz_regions.pr_del,0,region,NULL) AS region,cities.pr_del, "
        "       GREATEST(cities.tid,NVL(tz_regions.tid, cities.tid)) AS tid "
      	"FROM cities,tz_regions "
      	"WHERE cities.tid>:tid AND "
      	"      cities.country=tz_regions.country(+) AND "
        "      cities.tz=tz_regions.tz(+) "
        "UNION "
        "SELECT id,code,code_lat,name,name_lat, "
      	"       cities.country,cities.tz, "
        "       DECODE(tz_regions.pr_del,0,region,NULL) AS region,cities.pr_del, "
        "       GREATEST(cities.tid,NVL(tz_regions.tid, cities.tid)) AS tid "
      	"FROM cities,tz_regions "
      	"WHERE tz_regions.tid>:tid AND "
      	"      cities.country=tz_regions.country AND "
        "      cities.tz=tz_regions.tz ";
    };
  protected:
    const char *get_table_name() { return "TCities"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TAirlinesRow: public TICAOBaseTableRow {
  public:
    std::string aircode,name,name_lat,short_name,short_name_lat;
    ~TAirlinesRow() {};
    const char *get_row_name() { return "TAirlinesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      if (lowerc(field)=="short_name") return pr_lat?short_name_lat:short_name;
      if (lowerc(field)=="aircode") return aircode;
      return TICAOBaseTableRow::AsString(field,pr_lat);
    };
};

class TAirlines: public TICAOBaseTable {
  private:
    std::map<std::string, TBaseTableRow*> aircode;
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_icao,code_icao_lat,aircode,name,name_lat,short_name,short_name_lat,pr_del,tid "
        "FROM airlines";
    };
    const char *get_refresh_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_icao,code_icao_lat,aircode,name,name_lat,short_name,short_name_lat,pr_del,tid "
        "FROM airlines WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TAirlines"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    virtual void delete_row(TBaseTableRow *row);
    virtual void add_row(TBaseTableRow *row);
  public:
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
};

class TClassesRow: public TCodeBaseTableRow {
  public:
    std::string name,name_lat;
    int priority;
    ~TClassesRow() {};
    const char *get_row_name() { return "TClassesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
    int AsInteger(std::string field)
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TClasses: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code,code_lat,name,name_lat,priority FROM classes";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TClasses"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};

class TSubclsRow: public TCodeBaseTableRow {
  public:
    std::string cl;
    ~TSubclsRow() {};
    const char *get_row_name() { return "TSubclsRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="cl") return cl;
      return TCodeBaseTableRow::AsString(field,pr_lat);
    };
};

class TSubcls: public TCodeBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT code,code_lat,class AS cl FROM subcls";
    };
    const char *get_refresh_sql_text()
    {
      return get_select_sql_text();
    };
  protected:
    const char *get_table_name() { return "TSubcls"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
};

class TCraftsRow: public TICAOBaseTableRow {
  public:
    std::string name,name_lat;
    ~TCraftsRow() {};
    const char *get_row_name() { return "TCraftsRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      return TICAOBaseTableRow::AsString(field,pr_lat);
    };
};

class TCrafts: public TICAOBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_icao,code_icao_lat,name,name_lat,pr_del,tid "
        "FROM crafts";
    };
    const char *get_refresh_sql_text()
    {
      return
        "SELECT id,code,code_lat,code_icao,code_icao_lat,name,name_lat,pr_del,tid "
        "FROM crafts WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TCrafts"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TCurrencyRow: public TTIDBaseTableRow {
  public:
    std::string name,name_lat;
    ~TCurrencyRow() {};
    const char *get_row_name() { return "TCurrencyRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      return TTIDBaseTableRow::AsString(field,pr_lat);
    };
};

class TCurrency: public TTIDBaseTable {
  private:
    const char *get_select_sql_text ()
    {
      return
        "SELECT id,code,code_lat,name,name_lat,pr_del,tid "
        "FROM currency";
    };
    const char *get_refresh_sql_text()
    {
      return
        "SELECT id,code,code_lat,name,name_lat,pr_del,tid "
        "FROM currency WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TCurrency"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TPayTypesRow: public TTIDBaseTableRow {
  public:
    std::string name,name_lat;
    ~TPayTypesRow() {};
    const char *get_row_name() { return "TPayTypesRow"; };
    std::string AsString(std::string field, bool pr_lat=false)
    {
      if (lowerc(field)=="name") return pr_lat?name_lat:name;
      return TTIDBaseTableRow::AsString(field,pr_lat);
    };
};

class TPayTypes: public TTIDBaseTable {
  private:
    const char *get_select_sql_text()
    {
      return
        "SELECT id,code,code_lat,name,name_lat,pr_del,tid "
        "FROM pay_types";
    };
    const char *get_refresh_sql_text()
    {
      return
        "SELECT id,code,code_lat,name,name_lat,pr_del,tid "
        "FROM pay_types WHERE tid>:tid";
    };
  protected:
    const char *get_table_name() { return "TPayTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
};

class TBaseTables {
    private:
        typedef std::map<std::string, TBaseTable *> TTables;
        TTables base_tables;
    public:
        TBaseTable &get(std::string name);
        void Clear();
        void Invalidate();
        ~TBaseTables() { Clear(); };
};

extern TBaseTables base_tables;


#endif
