#include <string>
#include <sstream>

#include "astra_elems.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;

const char* EncodeElemContext(const TElemContext ctxt)
{
  switch( ctxt ) {
		case ecDisp:
		  return "ecDisp";
		case ecCkin:
		  return "ecCkin";
		case ecTrfer:
		  return "ecTrfer";
		case ecTlgTypeB:
		  return "ecTlgTypeB";
		case ecNone:
		  return "ecNone";
	}
	return "";
};

const char* EncodeElemFmt(const TElemFmt type)
{
    switch( type ) {
        case efmtUnknown:
            return "efmtUnknown";
        case efmtCodeNative:
            return "efmtCodeNative";
        case efmtCodeInter:
            return "efmtCodeInter";
        case efmtCodeICAONative:
            return "efmtCodeICAONative";
        case efmtCodeICAOInter:
            return "efmtCodeICAOInter";
        case efmtCodeISOInter:
            return "efmtCodeISOInter";
        case efmtNameLong:
            return "efmtNameLong";
        case efmtNameShort:
            return "efmtNameShort";
    }
    return "";
};

const
  struct
  {
    TElemType ElemType;
    const char* EncodeStr;
    const char* BaseTableName;
  } ElemBaseTables[] = {
                         {etAirline,               "etAirline",               "airlines"},
                         {etAirp,                  "etAirp",                  "airps"},
                         {etBagNormType,           "etBagNormType",           "bag_norm_types"},
                         {etCity,                  "etCity",                  "cities"},
                         {etClass,                 "etClass",                 "classes"},
                         {etClientType,            "etClientType",            "client_types"},
                         {etClsGrp,                "etClsGrp",                "cls_grp"},
                         {etCompElemType,          "etCompElemType",          "comp_elem_types"},
                         {etCompLayerType,         "etCompLayerType",         "comp_layer_types"},
                         {etCountry,               "etCountry",               "countries"},
                         {etCraft,                 "etCraft",                 "crafts"},
                         {etCrs,                   "etCrs",                   ""},
                         {etCurrency,              "etCurrency",              "currency"},
                         {etDelayType,             "etDelayType",             ""},
                         {etDeskGrp,               "etDeskGrp",               ""},
                         {etDevFmtType,            "etDevFmtType",            "dev_fmt_types"},
                         {etDevModel,              "etDevModel",              "dev_models"},
                         {etDevOperType,           "etDevOperType",           "dev_oper_types"},
                         {etDevSessType,           "etDevSessType",           "dev_sess_types"},
                         {etGenderType,            "etGenderType",            "gender_types"},
                         {etGraphStage,            "etGraphStage",            "graph_stages"},
                         {etGrpStatusType,         "etGrpStatusType",         "grp_status_types"},
                         {etHall,                  "etHall",                  ""},
                         {etLangType,              "etLangType",              "lang_types"},
                         {etMiscSetType,           "etMiscSetType",           "misc_set_types"},
                         {etPaxDocType,            "etPaxDocType",            "pax_doc_types"},
                         {etPayType,               "etPayType",               "pay_types"},
                         {etPersType,              "etPersType",              "pers_types"},
                         {etRefusalType,           "etRefusalType",           "refusal_types"},
                         {etRight,                 "etRight",                 "rights"},
                         {etSeatAlgoType,          "etSeatAlgoType",          "seat_algo_types"},
                         {etStationMode,           "etStationMode",           "station_modes"},
                         {etSubcls,                "etSubcls",                "subcls"},
                         {etSuffix,                "etSuffix",                "trip_suffixes"},
                         {etTagColor,              "etTagColor",              "tag_colors"},
                         {etTripLiter,             "etTripLiter",             ""},
                         {etTripType,              "etTripType",              "trip_types"},
                         {etTypeBType,             "etTypeBType",             "typeb_types"},
                         {etUserSetType,           "etUserSetType",           "user_set_types"},
                         {etUserType,              "etUserType",              "user_types"},
                         {etValidatorType,         "etValidatorType",         ""}
                       };

const char* EncodeElemType(const TElemType type)
{
  int i=sizeof(ElemBaseTables)/sizeof(ElemBaseTables[0])-1;
  for(;i>=0;i--)
    if (ElemBaseTables[i].ElemType==type) return ElemBaseTables[i].EncodeStr;
  return "";
};

void DoElemEConvertError( TElemContext ctxt, TElemType type, const string &elem )
{
	ostringstream msg;
	msg << "Can't convert elem to id: "
	    << "ctxt=" << EncodeElemContext(ctxt) << ", "
	    << "type=" << EncodeElemType(type) << ", "
	    << "elem=" << elem;

  throw EConvertError( msg.str().c_str() );
}

string ElemCtxtToElemId(TElemContext ctxt,TElemType type, string code, TElemFmt &fmt,
                        bool hard_verify, bool with_deleted)
{
  string id;
  fmt=efmtUnknown;

  if (code.empty()) return id;

  id = ElemToElemId(type,code,fmt,"",with_deleted);

  if ( ctxt==ecDisp || ctxt==ecCkin )
  {
    if(type==etAirline ||
       type==etAirp ||
       type==etCraft ||
       type==etSuffix)
    {
      TReqInfo *reqInfo = TReqInfo::Instance();
      TUserSettingType user_fmt;
      switch(ctxt) {
      	case ecDisp:
          if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
          if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
          if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
          if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
          break;
      	case ecCkin:
          if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
          if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
          if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
          if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
          break;
        default:;
      }
      if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
      if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
      if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
      if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
      if (type==etAirline ||
          type==etAirp ||
          type==etCraft)
      {
      	if ( hard_verify || fmt == efmtUnknown ) {
          if (!(user_fmt==ustCodeNative     && fmt==efmtCodeNative ||
                user_fmt==ustCodeInter      && fmt==efmtCodeInter ||
                user_fmt==ustCodeICAONative && fmt==efmtCodeICAONative ||
                user_fmt==ustCodeICAOInter  && fmt==efmtCodeICAOInter ||
                user_fmt==ustCodeMixed      && (fmt==efmtCodeNative||
                                                fmt==efmtCodeInter||
                                                fmt==efmtCodeICAONative||
                                                fmt==efmtCodeICAOInter)))
          {
            //проблемы
            DoElemEConvertError( ctxt, type, code );
          }
        }
        else {
          switch( user_fmt )  {
          	case ustCodeNative:
          		fmt = efmtCodeNative;
          		break;
          	case ustCodeInter:
          		fmt = efmtCodeInter;
          		break;
          	case ustCodeICAONative:
          		fmt = efmtCodeICAONative;
          		break;
          	case ustCodeICAOInter:
          		fmt = efmtCodeICAOInter;
          		break;
          	default:;
          }
        }
      }
      else
      {
      	if ( hard_verify || fmt == efmtUnknown ) {
          if (!(user_fmt==ustEncNative && fmt==efmtCodeNative ||
                user_fmt==ustEncLatin && fmt==efmtCodeInter ||
                user_fmt==ustEncMixed && (fmt==efmtCodeNative||fmt==efmtCodeInter)))
          {
            //проблемы
            DoElemEConvertError( ctxt, type, code );
          }
        }
        else {
          switch( user_fmt )  {
          	case ustEncNative:
          		fmt = efmtCodeNative;
          		break;
          	case ustEncLatin:
          		fmt = efmtCodeInter;
          		break;
          	default:;
          }

        }
      };

    }
    else
    	if ( hard_verify || fmt == efmtUnknown ) {
        if (fmt!=efmtCodeNative)
        {
          //проблемы
            DoElemEConvertError( ctxt, type, code );
        };
      }
      else {
      	fmt = efmtCodeNative;
      }
  };

  return id;
};

string ElemIdToElemCtxt(TElemContext ctxt,TElemType type, string id,
                        TElemFmt fmt, bool with_deleted)
{
	TElemFmt fmt2=fmt;  //efmtCodeNative;
  if ( ctxt==ecDisp || ctxt==ecCkin )
  {
    if(type==etAirline ||
       type==etAirp ||
       type==etCraft ||
       type==etSuffix)
    {
      TReqInfo *reqInfo = TReqInfo::Instance();
      TUserSettingType user_fmt;
      switch (ctxt) {
      	case ecDisp:
          if (type==etAirline) user_fmt=reqInfo->user.sets.disp_airline;
          if (type==etAirp) user_fmt=reqInfo->user.sets.disp_airp;
          if (type==etCraft) user_fmt=reqInfo->user.sets.disp_craft;
          if (type==etSuffix) user_fmt=reqInfo->user.sets.disp_suffix;
          break;
        case ecCkin:
          if (type==etAirline) user_fmt=reqInfo->user.sets.ckin_airline;
          if (type==etAirp) user_fmt=reqInfo->user.sets.ckin_airp;
          if (type==etCraft) user_fmt=reqInfo->user.sets.ckin_craft;
          if (type==etSuffix) user_fmt=reqInfo->user.sets.ckin_suffix;
          break;
        default:;
      }
      if (type==etAirline ||
          type==etAirp ||
          type==etCraft)
      {
        switch(user_fmt)
        {
          case ustCodeNative:     fmt2=efmtCodeNative; break;
          case ustCodeInter:      fmt2=efmtCodeInter; break;
          case ustCodeICAONative: fmt2=efmtCodeICAONative; break;
          case ustCodeICAOInter:  fmt2=efmtCodeICAOInter; break;
          case ustCodeMixed:      fmt2=fmt; break;
          default: ;
        };
      }
      else
      {
        switch(user_fmt)
        {
          case ustEncNative: fmt2=efmtCodeNative; break;
          case ustEncLatin:  fmt2=efmtCodeInter; break;
          case ustEncMixed:  fmt2=fmt; break;
          default: ;
        };
      };
    };
  };

  return ElemIdToClientElem(type,id,fmt2,with_deleted);
};

string getTableName(TElemType type)
{
  int i=sizeof(ElemBaseTables)/sizeof(ElemBaseTables[0])-1;
  for(;i>=0;i--)
    if (ElemBaseTables[i].ElemType==type) return ElemBaseTables[i].BaseTableName;
  return "";
};

TBaseTable& getBaseTable(TElemType type)
{
  return base_tables.get(getTableName(type));
};

string ElemToElemId(TElemType type, const string &elem, TElemFmt &fmt, const std::string &lang, bool with_deleted)
{
  fmt=efmtUnknown;
  if (elem.empty()/* || lang.empty()*/) return "";

  string id;

  string table_name=getTableName(type);

  if (!table_name.empty())
  {
    //это коды
    TBaseTable& BaseTable=base_tables.get(table_name);
    try
    {
      TCodeBaseTable& CodeBaseTable=dynamic_cast<TCodeBaseTable&>(BaseTable);
      //это code/code_lat
      try
      {
        id=((TCodeBaseTableRow&)CodeBaseTable.get_row("code",elem,with_deleted)).code;
        fmt=efmtCodeNative;
        return id;
      }
      catch (EBaseTableError) {};
      try
      {
        id=((TCodeBaseTableRow&)CodeBaseTable.get_row("code_lat",elem,with_deleted)).code;
        fmt=efmtCodeInter;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};

    try
    {
      TICAOBaseTable& ICAOBaseTable=dynamic_cast<TICAOBaseTable&>(BaseTable);
      //это code_icao,code_icao_lat
      try
      {
        id=((TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao",elem,with_deleted)).code;
        fmt=efmtCodeICAONative;
        return id;
      }
      catch (EBaseTableError) {};
      try
      {
        id=((TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao_lat",elem,with_deleted)).code;
        fmt=efmtCodeICAOInter;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};

    try
    {
      TCountries& Countries=dynamic_cast<TCountries&>(BaseTable);
      //это code_iso
      try
      {
        id=((TCountriesRow&)Countries.get_row("code_iso",elem,with_deleted)).code;
        fmt=efmtCodeISOInter;
        return id;
      }
      catch (EBaseTableError) {};
    }
    catch (bad_cast) {};
  }
  else
  {
  	TQuery Qry(&OraSession);
  	switch (type) {
  		case etDelayType:
  			Qry.SQLText =
  		    "SELECT code AS id, code, code_lat FROM delays where :code IN (code, code_lat)";
  		  Qry.CreateVariable( "code", otString, elem );
  		  Qry.Execute();
  		  if ( !Qry.Eof ) {
  		  	id = Qry.FieldAsString( "id" );
          if ( elem == Qry.FieldAsString( "code_lat" ) )
          	fmt = efmtCodeInter;
          else
          	fmt = efmtCodeNative;
  		  }
  			break;
  		case etTripLiter:
  			Qry.SQLText =
  		    "SELECT code AS id, code, code_lat FROM trip_liters where :code IN (code, code_lat)";
  		  Qry.CreateVariable( "code", otString, elem );
  		  Qry.Execute();
  		  if ( !Qry.Eof ) {
  		  	id = Qry.FieldAsString( "id" );
          if ( elem == Qry.FieldAsString( "code_lat" ) )
          	fmt = efmtCodeInter;
          else
          	fmt = efmtCodeNative;
  		  }
  			break;
      //надо бы добавить в будущем etValidatorType
  		default:;
  	}

  };

  return id;
};

string ElemToElemId(TElemType type, const string &elem, TElemFmt &fmt, bool with_deleted)
{
  return ElemToElemId(type, elem, fmt, "", with_deleted);
};

void getElem(TElemFmt fmt, const std::string &lang, TQuery& Qry, string &elem)
{
  elem.clear();
  if(Qry.Eof)
      return;
  int field_idx=-1;
  switch(fmt)
  {
    case efmtNameShort:
      if (lang==AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("short_name");
      else
        field_idx=Qry.GetFieldIndex("short_name_lat");
      break;

    case efmtNameLong:
      if (lang==AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("name");
      else
        field_idx=Qry.GetFieldIndex("name_lat");
      break;

    case efmtCodeNative:
    case efmtCodeInter:
      if (fmt==efmtCodeNative && lang==AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("code");
      if (fmt==efmtCodeInter ||
          fmt==efmtCodeNative && lang!=AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("code_lat");
      break;

    case efmtCodeICAONative:
    case efmtCodeICAOInter:
      if (fmt==efmtCodeICAONative && lang==AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("code_icao");
      if (fmt==efmtCodeICAOInter ||
          fmt==efmtCodeICAONative && lang!=AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("code_icao_lat");
      break;

    case efmtCodeISOInter:
      if (fmt==efmtCodeISOInter)
        field_idx=Qry.GetFieldIndex("code_iso");
      break;

    case efmtUnknown:
      break;
  };
  if (field_idx>=0)
    elem=Qry.FieldAsString(field_idx);
  return;
};

void getElem(TElemFmt fmt, const std::string &lang, const TBaseTableRow& BaseTableRow, string &elem)
{
  elem.clear();
  switch(fmt)
  {
    case efmtNameShort:
      try
      {
        elem=BaseTableRow.AsString("short_name", lang);
        return;
      }
      catch (EBaseTableError) {};
      break;

    case efmtNameLong:
      try
      {
        const TNameBaseTableRow& row=dynamic_cast<const TNameBaseTableRow&>(BaseTableRow);
        if (lang==AstraLocale::LANG_RU)
          elem=row.name;
        else
          elem=row.name_lat;
        return;
      }
      catch (bad_cast) {};
      break;

    case efmtCodeNative:
    case efmtCodeInter:
      try
      {
        const TCodeBaseTableRow& row=dynamic_cast<const TCodeBaseTableRow&>(BaseTableRow);
        if (fmt==efmtCodeNative && lang==AstraLocale::LANG_RU)
        {
          elem=row.code;
          return;
        };
        if (fmt==efmtCodeInter ||
            fmt==efmtCodeNative && lang!=AstraLocale::LANG_RU)
        {
          elem=row.code_lat;
          return;
        };
      }
      catch (bad_cast) {};
      break;

    case efmtCodeICAONative:
    case efmtCodeICAOInter:
      try
      {
        const TICAOBaseTableRow& row=dynamic_cast<const TICAOBaseTableRow&>(BaseTableRow);
        if (fmt==efmtCodeICAONative && lang==AstraLocale::LANG_RU)
        {
          elem=row.code_icao;
          return;
        };
        if (fmt==efmtCodeICAOInter ||
            fmt==efmtCodeICAONative && lang!=AstraLocale::LANG_RU)
        {
          elem=row.code_icao_lat;
          return;
        };

      }
      catch (bad_cast) {};
      break;

    case efmtCodeISOInter:
      try
      {
        const TCountriesRow& row=dynamic_cast<const TCountriesRow&>(BaseTableRow);
        if (fmt==efmtCodeISOInter)
        {
          elem=row.code_iso;
          return;
        };
      }
      catch (bad_cast) {};
      break;

    case efmtUnknown:
      break;
  };
};

string ElemIdToElem(TElemType type, const string &id, const vector< pair<TElemFmt,string> > &fmts, bool with_deleted)
{
  if (id.empty()) return "";

  string elem;

  string table_name=getTableName(type);

  if (!table_name.empty())
  {
    try
    {
      TBaseTableRow& BaseTableRow=base_tables.get(table_name).get_row("code",id,with_deleted);

      for(vector< pair<TElemFmt,string> >::const_iterator i=fmts.begin();i!=fmts.end();i++)
      {
        getElem(i->first, i->second, BaseTableRow, elem);
        if (!elem.empty()) break;
      };
    }
    catch (EBaseTableError) {};
  }
  else
  {
    TQuery Qry(&OraSession);
    //не base_table
    switch(type)
    {
                case etCrs: Qry.SQLText="SELECT name,name_lat FROM crs2 WHERE code=:id"; break;
          case etDelayType: Qry.SQLText="SELECT code,code_lat,name,name_lat FROM delays WHERE code=:id";break;
          case etTripLiter: Qry.SQLText="SELECT code,code_lat,name,name_lat FROM trip_liters WHERE code=:id";break;
      case etValidatorType: Qry.SQLText="SELECT code,code_lat,name,name_lat FROM validator_types WHERE code=:id";break;
      default: throw Exception("Unexpected elem type %s", EncodeElemType(type));
    };
    Qry.CreateVariable("id",otString,id);
    Qry.Execute();
    for(vector< pair<TElemFmt,string> >::const_iterator i=fmts.begin();i!=fmts.end();i++)
    {
      getElem(i->first, i->second, Qry, elem);
      if (!elem.empty()) break;
    };
  };

  return elem;
};

string ElemIdToElem(TElemType type, const string &id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  vector< pair<TElemFmt,string> > fmts;
  fmts.push_back( make_pair(fmt, lang) );
  return ElemIdToElem(type, id, fmts, with_deleted);
};

string ElemIdToElem(TElemType type, int id, const vector< pair<TElemFmt,string> > &fmts, bool with_deleted)
{
  if (id==ASTRA::NoExists) return "";

  string elem;

  string table_name=getTableName(type);

  if (!table_name.empty())
  {
    try
    {
      TBaseTableRow& BaseTableRow=base_tables.get(table_name).get_row("id",id,with_deleted);

      for(vector< pair<TElemFmt,string> >::const_iterator i=fmts.begin();i!=fmts.end();i++)
      {
        getElem(i->first, i->second, BaseTableRow, elem);
        if (!elem.empty()) break;
      };
    }
    catch (EBaseTableError) {};
  } else
  {
    TQuery Qry(&OraSession);
    //не base_table
    switch(type)
    {
         case etHall: Qry.SQLText="SELECT name,name_lat FROM halls2 WHERE id=:id"; break;
      case etDeskGrp: Qry.SQLText="SELECT descr AS name, descr_lat AS name_lat FROM desk_grp WHERE grp_id=:id"; break;
      default: throw Exception("Unexpected elem type %s", EncodeElemType(type));
    };
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    for(vector< pair<TElemFmt,string> >::const_iterator i=fmts.begin();i!=fmts.end();i++)
    {
      getElem(i->first, i->second, Qry, elem);
      if (!elem.empty()) break;
    };
  };

  return elem;
};

string ElemIdToElem(TElemType type, int id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  vector< pair<TElemFmt,string> > fmts;
  fmts.push_back( make_pair(fmt, lang) );
  return ElemIdToElem(type, id, fmts, with_deleted);
};

void getElemFmts(TElemFmt fmt, string basic_lang, vector< pair<TElemFmt,string> > &fmts)
{
  fmts.clear();
  for(int pass=0; pass<=1; pass++)
  {
    string lang;
    if (pass==0)
    {
      if (!basic_lang.empty())
        lang=basic_lang;
      else
        continue;
    }
    else
    {
      if (basic_lang!=AstraLocale::LANG_RU)
        lang=AstraLocale::LANG_RU;
      else
        continue;
    };

    switch(fmt)
    {
       case efmtNameLong:
         fmts.push_back( make_pair(efmtNameLong, lang) );
       case efmtNameShort:
         fmts.push_back( make_pair(efmtNameShort, lang) );
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         break;
       case efmtCodeISOInter:
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         break;
       case efmtCodeICAOInter:
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         break;
       case efmtCodeICAONative:
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         break;
       case efmtCodeInter:
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         break;
       case efmtCodeNative:
         fmts.push_back( make_pair(efmtCodeNative, lang) );
         fmts.push_back( make_pair(efmtCodeInter, lang) );
         fmts.push_back( make_pair(efmtCodeICAONative, lang) );
         fmts.push_back( make_pair(efmtCodeICAOInter, lang) );
         fmts.push_back( make_pair(efmtCodeISOInter, lang) );
         break;
       case efmtUnknown:
         break;
    };
  };
};

string ElemIdToClientElem(TElemType type, const string &id, TElemFmt fmt, bool with_deleted)
{
  if (id.empty()) return "";

  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, TReqInfo::Instance()->desk.lang, fmts);
  return ElemIdToElem(type, id, fmts, with_deleted);
};

string ElemIdToClientElem(TElemType type, int id, TElemFmt fmt, bool with_deleted)
{
  if (id==ASTRA::NoExists) return "";

  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, TReqInfo::Instance()->desk.lang, fmts);
  return ElemIdToElem(type, id, fmts, with_deleted);
};

string ElemIdToCodeNative(TElemType type, const string &id)
{
  return ElemIdToClientElem(type, id, efmtCodeNative, true);
};

string ElemIdToCodeNative(TElemType type, int id)
{
  return ElemIdToClientElem(type, id, efmtCodeNative, true);
};

string ElemIdToNameLong(TElemType type, const string &id)
{
  return ElemIdToClientElem(type, id, efmtNameLong, true);
};

string ElemIdToNameLong(TElemType type, int id)
{
  return ElemIdToClientElem(type, id, efmtNameLong, true);
};

string ElemIdToNameShort(TElemType type, const string &id)
{
  return ElemIdToClientElem(type, id, efmtNameShort, true);
};

string ElemIdToNameShort(TElemType type, int id)
{
  return ElemIdToClientElem(type, id, efmtNameShort, true);
};

TElemFmt prLatToElemFmt(TElemFmt fmt, bool pr_lat)
{
  TElemFmt res=fmt;
  if (pr_lat)
  {
    if (fmt==efmtCodeNative) res=efmtCodeInter;
    if (fmt==efmtCodeICAONative) res=efmtCodeICAOInter;
  };
  return res;
};

