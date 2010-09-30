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

const char* EncodeElemType(const TElemType type)
{
  switch( type ) {
  	case etCountry:
  		return "etCountry";
  	case etCity:
  		return "etCity";
  	case etAirline:
  		return "etAirline";
  	case etAirp:
  		return "etAirp";
  	case etCraft:
  		return "etCraft";
  	case etClass:
  		return "etClass";
  	case etSubcls:
  		return "etSubcls";
  	case etPersType:
  		return "etPersType";
  	case etGenderType:
  		return "etGenderType";
  	case etPaxDocType:
  		return "etPaxDocType";
  	case etPayType:
  		return "etPayType";
  	case etCurrency:
  		return "etCurrency";
  	case etRefusalType:
  	  return "etRefusalType";
    case etSuffix:
  		return "etSuffix";
  	case etTripType:
  		return "etTripType";
  	case etCompElemType:
  		return "etCompElemType";
  	case etGrpStatusType:
  		return "etGrpStatusType";
  	case etClientType:
  		return "etClientType";
  	case etCompLayerType:
  		return "etCompLayerType";
  	case etCrs:
  		return "etCrs";
  	case etDevModel:
  		return "etDevModel";
    case etDevSessType:
  		return "etDevSessType";
    case etDevFmtType:
  		return "etDevFmtType";
  	case etDevOperType:
  		return "etDevOperType";
  	case etGraphStage:
  		return "etGraphStage";
  }
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

  //далее проверим а вообще имели ли мы право вводить в таком формате
  /*
  if ( hard_verify ) {
    if (ctxt==ecTlgTypeB && (type!=etCountry && fmt!=0 && fmt!=1 ||
                             type==etCountry && fmt!=0 && fmt!=1 && fmt!=4) ||
        ctxt==ecCkin && (fmt!=0) ||
        ctxt==ecTrfer && (fmt!=0))
    {
      //проблемы
      DoElemEConvertError( ctxt, type, code );
    };
  } !!!vlad */
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
	TElemFmt fmt2=efmtCodeNative;
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
	string table_name;
  switch(type)
  {
    case etCountry:
      table_name="countries";
      break;
    case etCity:
      table_name="cities";
      break;
    case etAirline:
      table_name="airlines";
      break;
    case etAirp:
      table_name="airps";
      break;
    case etCraft:
      table_name="crafts";
      break;
    case etClass:
      table_name="classes";
      break;
    case etSubcls:
      table_name="subcls";
      break;
    case etPersType:
      table_name="pers_types";
      break;
    case etGenderType:
      table_name="gender_types";
      break;
    case etPaxDocType:
      table_name="pax_doc_types";
      break;
    case etPayType:
      table_name="pay_types";
      break;
    case etCurrency:
      table_name="currency";
      break;
    case etRefusalType:
      table_name="refusal_types";
      break;
    case etTripType:
      table_name="trip_types";
      break;
    case etSuffix:
      table_name="trip_suffixes";
      break;
    case etCompElemType:
      table_name="comp_elem_types";
      break;
    case etGrpStatusType:
    	table_name="grp_status_types";
    	break;
    case etClientType:
    	table_name="client_types";
    	break;
    case etCompLayerType:
    	table_name="comp_layer_types";
    	break;
    case etDevModel:
    	table_name="dev_models";
    	break;
    case etDevSessType:
  		table_name="dev_sess_types";
  		break;
    case etDevFmtType:
  		table_name="dev_fmt_types";
  		break;
  	case etDevOperType:
  		table_name="dev_oper_types";
  		break;
    case etGraphStage:
  		table_name="graph_stages";
  		break;
  };
  return table_name;
}

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
      default: ;
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
  };

  return elem;
};

string ElemIdToElem(TElemType type, int id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  vector< pair<TElemFmt,string> > fmts;
  fmts.push_back( make_pair(fmt, lang) );
  return ElemIdToElem(type, id, fmts, with_deleted);
};

void getElemFmts(TElemFmt fmt, vector< pair<TElemFmt,string> > &fmts)
{
  fmts.clear();
  for(int pass=0; pass<=1; pass++)
  {
    string lang;
    if (pass==0)
    {
      if (!TReqInfo::Instance()->desk.lang.empty())
        lang=TReqInfo::Instance()->desk.lang;
      else
        continue;
    }
    else
    {
      if (TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU)
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
  getElemFmts(fmt, fmts);
  return ElemIdToElem(type, id, fmts, with_deleted);
};

string ElemIdToClientElem(TElemType type, int id, TElemFmt fmt, bool with_deleted)
{
  if (id==ASTRA::NoExists) return "";

  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, fmts);
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

