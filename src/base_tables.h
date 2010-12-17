#ifndef _BASE_TABLES_H_
#define _BASE_TABLES_H_

#include <map>
#include <vector>
#include <string>
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "astra_locale.h"

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
  	TBaseTable() {
  		Init();
  	}
    virtual ~TBaseTable()
    {
      std::vector<TBaseTableRow*>::iterator i;
      for(i=table.begin();i!=table.end();i++) delete *i;
    };
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
    virtual TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
    virtual void Invalidate() { pr_actual=false; };
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
    virtual TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
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
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
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
    virtual TBaseTableRow& get_row(std::string field, int value, bool with_deleted=false);
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
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
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
    TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
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
    int priority,weight_win,weight_sum;
    const char *get_row_name() const { return "TPersTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      if (lowerc(field)=="weight_win") return weight_win;
      if (lowerc(field)=="weight_sum") return weight_sum;
      return TCodeBaseTableRow::AsInteger(field);
    };
};

class TPersTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TPersTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TPersTypes() {
 		  Init("pers_types");
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
    void Invalidate() {}; //всегда актуальна
  public:
  	TGenderTypes() {
  		Init( "gender_types" );
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
    void Invalidate() {}; //всегда актуальна
  public:
  	TTagColors() {
  		Init( "tag_colors" );
  	}
};

class TPaxDocTypesRow: public TCodeBaseTableRow {
  public:
    const char *get_row_name() const { return "TPaxDocTypesRow"; };
};

class TPaxDocTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TPaxDocTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
    TPaxDocTypes( ) {
    	Init( "pax_doc_types" );
    }
};

class TTypeBTypesRow: public TCodeBaseTableRow {
  public:
    std::string basic_type;
    bool editable;
    std::string short_name,short_name_lat;
    const char *get_row_name() const { return "TTypeBTypesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="basic_type") return basic_type;
      return TCodeBaseTableRow::AsString(field,lang);
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
    void Invalidate() {}; //всегда актуальна
  public:
    TTypeBTypes() {
 		  Init("typeb_types");
 	  }
};

class TCitiesRow: public TTIDBaseTableRow {
  public:
    std::string country,region;
    int tz;
    const char *get_row_name() const { return "TCitiesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="country") return country;
      if (lowerc(field)=="region") return region;
      return TTIDBaseTableRow::AsString(field,lang);
    };
    int AsInteger(std::string field) const
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
    std::string aircode,short_name,short_name_lat;
    const char *get_row_name() const { return "TAirlinesRow"; };
    std::string AsString(std::string field, const std::string lang=AstraLocale::LANG_RU) const
    {
      if (lowerc(field)=="short_name") return lang!=AstraLocale::LANG_RU?short_name_lat:short_name;
      if (lowerc(field)=="aircode") return aircode;
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
    virtual TBaseTableRow& get_row(std::string field, std::string value, bool with_deleted=false);
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
    void Invalidate() {}; //всегда актуальна
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
    void Invalidate() {}; //всегда актуальна
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
    void Invalidate() {}; //всегда актуальна
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

class TCompElemTypesRow: public TCodeBaseTableRow {
	public:
	  const char *get_row_name() const { return "TCompElemTypesRow"; };
};

class TCompElemTypes: public TCodeBaseTable {
  protected:
		const char *get_table_name() { return "TCompElemTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
  	TCompElemTypes() {
  		Init( "comp_elem_types" );
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
    void Invalidate() {}; //всегда актуальна
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
    void Invalidate() {}; //всегда актуальна
  public:
  	TClientTypes() {
  		Init( "client_types" );
  	}
};

class TCompLayerTypesRow: public TCodeBaseTableRow {
  public:
    int priority;
    const char *get_row_name() const { return "TCompLayerTypesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="priority") return priority;
      return TCodeBaseTableRow::AsInteger(field);
    }
};

class TCompLayerTypes: public TCodeBaseTable {
  protected:
    const char *get_table_name() { return "TCompLayerTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
  	TCompLayerTypes() {
  		Init( "comp_layer_types" );
  	}
};

class TGraphStagesRow: public TIdBaseTableRow {
	  public:
    int stage_time;
    bool pr_auto;
    const char *get_row_name() const { return "TGraphStagesRow"; };
    int AsInteger(std::string field) const
    {
      if (lowerc(field)=="time") return stage_time;
      return TIdBaseTableRow::AsInteger(field);
    }
    bool AsBoolean(std::string field) const
    {
      if (lowerc(field)=="pr_auto") return pr_auto;
      return TIdBaseTableRow::AsBoolean(field);
    }
};

class TGraphStages: public TIdBaseTable {
  protected:
    const char *get_table_name() { return "TGraphStages"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
  	TGraphStages() {
  		Init();
  		select_sql = "SELECT stage_id id,name,name_lat,time,pr_auto FROM graph_stages";
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
    const char *get_table_name() { return "TSeatAlgoTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
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
    const char *get_table_name() { return "TUserTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
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
    const char *get_table_name() { return "TBagNormTypes"; };
    void create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row);
    void Invalidate() {}; //всегда актуальна
  public:
  	TBagNormTypes() {
  		Init( "bag_norm_types" );
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
    void Invalidate() {}; //всегда актуальна
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
    void Invalidate() {}; //всегда актуальна
  public:
  	TStationModes() {
      Init();
  		select_sql =
  		  "SELECT 'Р' AS code, 'Регистрация' AS name, 'Check-in' AS name_lat FROM dual "
        "UNION "
        "SELECT 'П', 'Посадка', 'Boarding' FROM dual";
  	};
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
