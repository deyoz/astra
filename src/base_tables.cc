#include <string>
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "serverlib/logger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;

void TBaseTables::Invalidate()
{
	for(TTables::iterator ti = base_tables.begin(); ti != base_tables.end(); ti++)
		ti->second->Invalidate();
};

void TBaseTables::Clear()
{
    for(TTables::iterator ti = base_tables.begin(); ti != base_tables.end(); ti++)
        delete ti->second;
    base_tables.clear();
}

TBaseTable &TBaseTables::get(string name)
{
    name = upperc(name);
    TTables::iterator ti = base_tables.find(name);
    if(ti == base_tables.end()) {
        if(name == "AIRPS")
            base_tables[name] = new TAirps();
        else if(name == "COUNTRIES")
            base_tables[name] = new TCountries();
        else if(name == "PERS_TYPES")
            base_tables[name] = new TPersTypes();
        else if(name == "GENDER_TYPES")
            base_tables[name] = new TGenderTypes();
        else if(name == "TAG_COLORS")
            base_tables[name] = new TTagColors();
        else if(name == "PAX_DOC_TYPES")
            base_tables[name] = new TPaxDocTypes();
        else if(name == "CITIES")
            base_tables[name] = new TCities();
        else if(name == "AIRLINES")
            base_tables[name] = new TAirlines();
        else if(name == "CLASSES")
            base_tables[name] = new TClasses();
        else if(name == "PAY_TYPES")
            base_tables[name] = new TPayTypes();
        else if(name == "CURRENCY")
            base_tables[name] = new TCurrency();
        else if(name == "CLS_GRP")
            base_tables[name] = new TClsGrp();
        else if(name == "REFUSAL_TYPES")
            base_tables[name] = new TRefusalTypes();
        else if(name == "SUBCLS")
            base_tables[name] = new TSubcls();
        else if(name == "CRAFTS")
            base_tables[name] = new TCrafts();
        else if(name == "TRIP_TYPES")
        	  base_tables[name] = new TTripTypes();
        else if(name == "COMP_ELEM_TYPES")
        	  base_tables[name] = new TCompElemTypes();
        else if(name == "GRP_STATUS_TYPES")
        	  base_tables[name] = new TGrpStatusTypes();
        else if(name == "CLIENT_TYPES")
        	  base_tables[name] = new TClientTypes();
        else if(name == "COMP_LAYER_TYPES")
        	  base_tables[name] = new TCompLayerTypes();
        else if(name == "DEV_MODELS")
        	  base_tables[name] = new TDevModels();
        else if(name == "DEV_SESS_TYPES")
        	  base_tables[name] = new TDevSessTypes();
        else if(name == "DEV_FMT_TYPES")
        	  base_tables[name] = new TDevFmtTypes();
        else if(name == "DEV_OPER_TYPES")
        	  base_tables[name] = new TDevOperTypes();
        else if(name == "GRAPH_STAGES")
        	  base_tables[name] = new TGraphStages();
        else if(name == "MISC_SET_TYPES")
        	  base_tables[name] = new TMiscSetTypes();
        else if(name == "SEAT_ALGO_TYPES")
        	  base_tables[name] = new TSeatAlgoTypes();
        else if(name == "TRIP_SUFFIXES")
        	  base_tables[name] = new TTripSuffixes();
        else if(name == "TYPEB_TYPES")
        	  base_tables[name] = new TTypeBTypes();
        else if(name == "BAG_NORM_TYPES")
        	  base_tables[name] = new TBagNormTypes();
        else if(name == "LANG_TYPES")
        	  base_tables[name] = new TLangTypes();
        else
            throw Exception("TBaseTables::get_base_table: " + name + " not found");
    }
    return *(base_tables[name]);
};

void TBaseTable::load_table()
{
  if(!pr_init || !pr_actual)
  {
    TQuery Qry(&OraSession);
    if (!pr_init)
    {
      ProgTrace(TRACE5,"%s: Qry.SQLText = get_select_sql_text=%s",get_table_name(), get_select_sql_text() );
      Qry.SQLText = get_select_sql_text();
      create_variables(Qry,false);
    }
    else
    {
      ProgTrace(TRACE5,"%s: Qry.SQLText = get_refresh_sql_text=%s",get_table_name(), get_refresh_sql_text() );
      Qry.SQLText = get_refresh_sql_text();
      create_variables(Qry,true);
    };
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TBaseTableRow *row=NULL, *replaced_row=NULL;
      try
      {
        create_row(Qry,&row,&replaced_row);
        delete_row(replaced_row);
        add_row(row); //???
      }
      catch(...)
      {
        if (row!=NULL) delete row;
        throw;
      };
    };
    //ProgTrace(TRACE5,"TABLE %s UPDATED: %d rows",get_table_name(),table.size());
    pr_init=true;
    pr_actual=true;
    after_update();
  };
};

void TBaseTable::delete_row(TBaseTableRow *row)
{
  if (row==NULL) return;
  vector<TBaseTableRow*>::iterator i;
  i=find(table.begin(),table.end(),row);
  delete *i;
  table.erase(i);
};

void TBaseTable::add_row(TBaseTableRow *row)
{
  if (row==NULL) return;
  table.push_back(row);
};

TBaseTableRow& TBaseTable::get_row(std::string field, std::string value, bool with_deleted)
{
  throw EBaseTableError("%s::get_row: wrong search field '%s'",
                        get_table_name(),field.c_str());
};

TBaseTableRow& TBaseTable::get_row(std::string field, int value, bool with_deleted)
{
  throw EBaseTableError("%s::get_row: wrong search field '%s'",
                        get_table_name(),field.c_str());
};

//////////////////////////////////////////////////////////////
void TNameBaseTable::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	int idx;
	if ( (idx=Qry.GetFieldIndex( "name" )) >= 0 )
    ((TNameBaseTableRow*)*row)->name=Qry.FieldAsString(idx);
  if ( (idx=Qry.GetFieldIndex( "name_lat" )) >= 0 )
    ((TNameBaseTableRow*)*row)->name_lat=Qry.FieldAsString(idx);
  else
  	((TNameBaseTableRow*)*row)->name_lat=((TNameBaseTableRow*)*row)->name;
}
/////////////////////////////////////////////////////////////
void TIdBaseTable::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  ((TIdBaseTableRow*)*row)->id=Qry.FieldAsInteger("id");
  if (*replaced_row==NULL)
  {
    map<int, TBaseTableRow*>::iterator i;
    i=id.find(((TIdBaseTableRow*)*row)->id);
    if (i!=id.end()) *replaced_row=i->second;
  };
  TNameBaseTable::create_row( Qry, row, replaced_row );
}

void TIdBaseTable::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<int, TBaseTableRow*>::iterator i;
    i=id.find(((TIdBaseTableRow*)row)->id);
    if (i->second==row) id.erase(i);
  };
  TBaseTable::delete_row(row);
}

void TIdBaseTable::add_row(TBaseTableRow *row)
{
  TBaseTable::add_row(row);
  if (row!=NULL && !row->deleted())
  {
      id[((TIdBaseTableRow*)row)->id]=row;
  };
}
TBaseTableRow& TIdBaseTable::get_row(std::string field, int value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="id")
  {
    std::map<int, TBaseTableRow*>::iterator i;
    i=id.find(value);
    if (i==id.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%d not found",
                            get_table_name(),field.c_str(),value);
    return *(i->second);
  };
  return TBaseTable::get_row(field,value,with_deleted);
}
/////////////////////////////////////////////////////////////
void TCodeBaseTable::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	int idx;
  ((TCodeBaseTableRow*)*row)->code=Qry.FieldAsString("code");
  if ( (idx=Qry.GetFieldIndex( "code_lat" )) >= 0 )
    ((TCodeBaseTableRow*)*row)->code_lat=Qry.FieldAsString(idx);
  else
  	((TCodeBaseTableRow*)*row)->code_lat=((TCodeBaseTableRow*)*row)->code;
  if (*replaced_row==NULL)
  {
    map<string, TBaseTableRow*>::iterator i;
    i=code.find(((TCodeBaseTableRow*)*row)->code);
    if (i!=code.end()) *replaced_row=i->second;
  };
  TNameBaseTable::create_row( Qry, row, replaced_row );
}

void TCodeBaseTable::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<string, TBaseTableRow*>::iterator i;
    i=code.find(((TCodeBaseTableRow*)row)->code);
    if (i->second==row) code.erase(i);
    i=code_lat.find(((TCodeBaseTableRow*)row)->code_lat);
    if (i->second==row) code_lat.erase(i);
  };
  TBaseTable::delete_row(row);
}

void TCodeBaseTable::add_row(TBaseTableRow *row)
{
  TBaseTable::add_row(row);
  if (row!=NULL && !row->deleted())
  {
    if(strcmp(get_table_name(), "TClsGrp") != 0) {   //!!!vlad плохое решение проблемы
      if (!((TCodeBaseTableRow*)row)->code.empty())
        code[((TCodeBaseTableRow*)row)->code]=row;
      if (!((TCodeBaseTableRow*)row)->code_lat.empty())
        code_lat[((TCodeBaseTableRow*)row)->code_lat]=row;
    }
  };
}

TBaseTableRow& TCodeBaseTable::get_row(std::string field, std::string value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="code")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code.find(value);
    if (i==code.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  if (lowerc(field)=="code_lat")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_lat.find(value);
    if (i==code_lat.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: table '%s': %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  if (lowerc(field)=="code/code_lat")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_lat.find(value);
    if (i==code_lat.end()||
        !with_deleted && i->second->deleted())
    {
      i=code.find(value);
      if (i==code.end()||
          !with_deleted && i->second->deleted())
        throw EBaseTableError("%s::get_row: %s=%s not found",
                              get_table_name(),field.c_str(),value.c_str());
    };
    return *(i->second);
  };
  return TBaseTable::get_row(field,value,with_deleted);
}
//////////////////////////////////////////////////////////////////
void TTIDBaseTable::create_variables(TQuery &Qry, bool pr_refresh)
{
  TCodeBaseTable::create_variables(Qry, pr_refresh);
  if (pr_refresh)
    Qry.CreateVariable("tid",otInteger,tid);
  new_tid=tid;
};

void TTIDBaseTable::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  ((TTIDBaseTableRow*)*row)->id=Qry.FieldAsInteger("id");
  ((TTIDBaseTableRow*)*row)->pr_del=Qry.FieldAsInteger("pr_del")!=0;
  if (new_tid<Qry.FieldAsInteger("tid")) new_tid=Qry.FieldAsInteger("tid");
  if (*replaced_row==NULL)
  {
    map<int, TBaseTableRow*>::iterator i;
    i=id.find(((TTIDBaseTableRow*)*row)->id);
    if (i!=id.end()) *replaced_row=i->second;
  };
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TTIDBaseTable::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<int, TBaseTableRow*>::iterator i;
    i=id.find(((TTIDBaseTableRow*)row)->id);
    if (i->second==row) id.erase(i);
  };
  TCodeBaseTable::delete_row(row);
};

void TTIDBaseTable::add_row(TBaseTableRow *row)
{
  TCodeBaseTable::add_row(row);
  if (row!=NULL) id[((TTIDBaseTableRow*)row)->id]=row;
};

void TTIDBaseTable::after_update()
{
  TCodeBaseTable::after_update();
  //ProgTrace(TRACE5,"UPDATE TABLE %s tid=%d new_tid=%d",
  //                 get_table_name(),tid,new_tid);
  tid=new_tid;
};

TBaseTableRow& TTIDBaseTable::get_row(std::string field, int value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="id")
  {
    std::map<int, TBaseTableRow*>::iterator i;
    i=id.find(value);
    if (i==id.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%d not found",
                            get_table_name(),field.c_str(),value);
    return *(i->second);
  };
  return TBaseTable::get_row(field,value,with_deleted);
};
/////////////////////////////////////////////////////////////
void TICAOBaseTable::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<string, TBaseTableRow*>::iterator i;
    i=code_icao.find(((TICAOBaseTableRow*)row)->code_icao);
    if (i->second==row) code_icao.erase(i);
    i=code_icao_lat.find(((TICAOBaseTableRow*)row)->code_icao_lat);
    if (i->second==row) code_icao_lat.erase(i);
  };
  TTIDBaseTable::delete_row(row);
};

void TICAOBaseTable::add_row(TBaseTableRow *row)
{
  TTIDBaseTable::add_row(row);
  if (row!=NULL && !((TICAOBaseTableRow*)row)->code_icao.empty())
    code_icao[((TICAOBaseTableRow*)row)->code_icao]=row;
  if (row!=NULL && !((TICAOBaseTableRow*)row)->code_icao_lat.empty())
    code_icao_lat[((TICAOBaseTableRow*)row)->code_icao_lat]=row;
};

TBaseTableRow& TICAOBaseTable::get_row(std::string field, std::string value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="code_icao")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_icao.find(value);
    if (i==code_icao.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  if (lowerc(field)=="code_icao_lat")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_icao_lat.find(value);
    if (i==code_icao_lat.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: table '%s': %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  if (lowerc(field)=="code_icao/code_icao_lat")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_icao_lat.find(value);
    if (i==code_icao_lat.end()||
        !with_deleted && i->second->deleted())
    {
      i=code_icao.find(value);
      if (i==code_icao.end()||
          !with_deleted && i->second->deleted())
        throw EBaseTableError("%s::get_row: %s=%s not found",
                              get_table_name(),field.c_str(),value.c_str());
    };
    return *(i->second);
  };
  return TCodeBaseTable::get_row(field,value,with_deleted);
};

void TICAOBaseTable::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  ((TICAOBaseTableRow*)*row)->code_icao=Qry.FieldAsString("code_icao");
  ((TICAOBaseTableRow*)*row)->code_icao_lat=Qry.FieldAsString("code_icao_lat");
  TTIDBaseTable::create_row(Qry,row,replaced_row);
};

void TCountries::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<string, TBaseTableRow*>::iterator i;
    i=code_iso.find(((TCountriesRow*)row)->code_iso);
    if (i->second==row) code_iso.erase(i);
  };
  TTIDBaseTable::delete_row(row);
};

void TCountries::add_row(TBaseTableRow *row)
{
  TTIDBaseTable::add_row(row);
  if (row!=NULL && !((TCountriesRow*)row)->code_iso.empty())
    code_iso[((TCountriesRow*)row)->code_iso]=row;
};

TBaseTableRow& TCountries::get_row(std::string field, std::string value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="code_iso")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=code_iso.find(value);
    if (i==code_iso.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  return TCodeBaseTable::get_row(field,value,with_deleted);
};

void TCountries::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TCountriesRow;
  ((TCountriesRow*)*row)->code_iso=Qry.FieldAsString("code_iso");
  TTIDBaseTable::create_row(Qry,row,replaced_row);
};

void TAirps::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TAirpsRow;
  ((TAirpsRow*)*row)->city=Qry.FieldAsString("city");
  TICAOBaseTable::create_row(Qry,row,replaced_row);
};

void TPersTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
  *row = new TPersTypesRow;
  ((TPersTypesRow*)*row)->priority=Qry.FieldAsInteger("priority");
  ((TPersTypesRow*)*row)->weight_win=Qry.FieldAsInteger("weight_win");
  ((TPersTypesRow*)*row)->weight_sum=Qry.FieldAsInteger("weight_sum");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
}

void TGenderTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TGenderTypesRow;
  ((TGenderTypesRow*)*row)->pr_inf=Qry.FieldAsInteger("pr_inf")!=0;
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TTagColors::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TTagColorsRow;
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TPaxDocTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TPaxDocTypesRow;
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TTypeBTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TTypeBTypesRow;
  ((TTypeBTypesRow*)*row)->basic_type=Qry.FieldAsString("basic_type");
  ((TTypeBTypesRow*)*row)->editable=Qry.FieldAsInteger("editable")!=0;
  ((TTypeBTypesRow*)*row)->short_name=Qry.FieldAsString("short_name");
  ((TTypeBTypesRow*)*row)->short_name_lat=Qry.FieldAsString("short_name_lat");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
}

void TCities::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TCitiesRow;
  ((TCitiesRow*)*row)->country=Qry.FieldAsString("country");
  ((TCitiesRow*)*row)->region=Qry.FieldAsString("region");
  ((TCitiesRow*)*row)->tz=Qry.FieldAsInteger("tz");
  TTIDBaseTable::create_row(Qry,row,replaced_row);
};

void TAirlines::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TAirlinesRow;
  ((TAirlinesRow*)*row)->short_name=Qry.FieldAsString("short_name");
  ((TAirlinesRow*)*row)->short_name_lat=Qry.FieldAsString("short_name_lat");
  ((TAirlinesRow*)*row)->aircode=Qry.FieldAsString("aircode");
  TICAOBaseTable::create_row(Qry,row,replaced_row);
};

void TAirlines::delete_row(TBaseTableRow *row)
{
  if (row!=NULL)
  {
    map<string, TBaseTableRow*>::iterator i;
    i=aircode.find(((TAirlinesRow*)row)->aircode);
    if (i->second==row) aircode.erase(i);
  };
  TICAOBaseTable::delete_row(row);
};

void TAirlines::add_row(TBaseTableRow *row)
{
  TICAOBaseTable::add_row(row);
  if (row!=NULL && !((TAirlinesRow*)row)->aircode.empty())
    aircode[((TAirlinesRow*)row)->aircode]=row;
};

TBaseTableRow& TAirlines::get_row(std::string field, std::string value, bool with_deleted)
{
  load_table();
  if (lowerc(field)=="aircode")
  {
    std::map<string, TBaseTableRow*>::iterator i;
    i=aircode.find(value);
    if (i==aircode.end()||
        !with_deleted && i->second->deleted())
      throw EBaseTableError("%s::get_row: %s=%s not found",
                            get_table_name(),field.c_str(),value.c_str());
    return *(i->second);
  };
  return TICAOBaseTable::get_row(field,value,with_deleted);
};

void TClasses::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TClassesRow;
  ((TClassesRow*)*row)->priority=Qry.FieldAsInteger("priority");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TSubcls::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TSubclsRow;
  ((TSubclsRow*)*row)->cl=Qry.FieldAsString("class");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TTripSuffixes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TTripSuffixesRow;
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TCrafts::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TCraftsRow;
  TICAOBaseTable::create_row(Qry,row,replaced_row);
}

void TCurrency::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	*row = new TCurrencyRow;
	TTIDBaseTable::create_row(Qry,row,replaced_row);
}

void TRefusalTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	*row = new TRefusalTypesRow;
	TTIDBaseTable::create_row(Qry,row,replaced_row);
}

void TPayTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	*row = new TPayTypesRow;
	TTIDBaseTable::create_row(Qry,row,replaced_row);
}

void TTripTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TTripTypesRow;
  ((TTripTypesRow*)*row)->pr_reg=Qry.FieldAsInteger("pr_reg");
  TTIDBaseTable::create_row(Qry,row,replaced_row);
};

void TClsGrp::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TClsGrpRow;
  ((TClsGrpRow*)*row)->airline=Qry.FieldAsString("airline");
  ((TClsGrpRow*)*row)->airp=Qry.FieldAsString("airp");
  ((TClsGrpRow*)*row)->cl=Qry.FieldAsString("class");
  ((TClsGrpRow*)*row)->priority=Qry.FieldAsInteger("priority");
  TTIDBaseTable::create_row(Qry,row,replaced_row);
};

void TCompElemTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
	*row = new TCompElemTypesRow;
  TCodeBaseTable::create_row(Qry, row, replaced_row);
};

void TDevModels::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
	*row = new TDevModelsRow;
  TCodeBaseTable::create_row(Qry, row, replaced_row);
};

void TDevSessTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
  *row = new TDevSessTypesRow;
  TCodeBaseTable::create_row(Qry, row, replaced_row);
};

void TDevFmtTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
	*row = new TDevFmtTypesRow;
  TCodeBaseTable::create_row(Qry, row, replaced_row);
};

void TDevOperTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row) {
	*row = new TDevOperTypesRow;
  TCodeBaseTable::create_row(Qry, row, replaced_row);
};

void TGrpStatusTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TGrpStatusTypesRow;
  ((TGrpStatusTypesRow*)*row)->priority=Qry.FieldAsInteger("priority");
  ((TGrpStatusTypesRow*)*row)->layer_type=Qry.FieldAsString("layer_type");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TClientTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TClientTypesRow;
  ((TClientTypesRow*)*row)->short_name=Qry.FieldAsString("short_name");
  ((TClientTypesRow*)*row)->short_name_lat=Qry.FieldAsString("short_name_lat");
  ((TClientTypesRow*)*row)->priority=Qry.FieldAsInteger("priority");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TCompLayerTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TCompLayerTypesRow;
  ((TCompLayerTypesRow*)*row)->priority=Qry.FieldAsInteger("priority");
  TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TGraphStages::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TGraphStagesRow;
  ((TGraphStagesRow*)*row)->stage_time=Qry.FieldAsInteger("time");
  ((TGraphStagesRow*)*row)->pr_auto=(Qry.FieldAsInteger("pr_auto")!=0);
  TIdBaseTable::create_row(Qry,row,replaced_row);
};

void TMiscSetTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TMiscSetTypesRow;
  TIdBaseTable::create_row(Qry,row,replaced_row);
};

void TSeatAlgoTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
  *row = new TSeatAlgoTypesRow;
  TIdBaseTable::create_row(Qry,row,replaced_row);
};

void TBagNormTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	*row = new TBagNormTypesRow;
	TCodeBaseTable::create_row(Qry,row,replaced_row);
};

void TLangTypes::create_row(TQuery &Qry, TBaseTableRow** row, TBaseTableRow **replaced_row)
{
	*row = new TLangTypesRow;
	TCodeBaseTable::create_row(Qry,row,replaced_row);
};


TBaseTables base_tables;
