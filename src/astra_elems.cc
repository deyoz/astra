#include <string>
#include <sstream>

#include "astra_elems.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "db_tquery.h"
#include "PgOraConfig.h"

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
                         {etAgency,                "etAgency",                ""},
                         {etAirline,               "etAirline",               "airlines"},
                         {etAirp,                  "etAirp",                  "airps"},
                         {etAirpTerminal,          "etAirpTerminal",          ""},
                         {etAlarmType,             "etAlarmType",             "alarm_types"},
                         {etBagNormType,           "etBagNormType",           "bag_norm_types"},
                         {etBagType,               "etBagType",               "bag_types"},
                         {etBIHall,                "etBIHall",                ""},
                         {etBIPrintType,           "etBIPrintType",           "bi_print_types"},
                         {etBIType,                "etBIType",                ""},
                         {etVOType,                "etVOType",                ""},
                         {etEMDAType,              "etEMDAType",              ""},
                         {etBrand,                 "etBrand",                 ""},
                         {etBPType,                "etBPType",                ""},
                         {etBTType,                "etBTType",                ""},
                         {etCity,                  "etCity",                  "cities"},
                         {etCkinRemType,           "etCkinRemType",           "ckin_rem_types"},
                         {etClass,                 "etClass",                 "classes"},
                         {etClientType,            "etClientType",            "client_types"},
                         {etClsGrp,                "etClsGrp",                "cls_grp"},
                         {etCompLayerType,         "etCompLayerType",         "comp_layer_types"},
                         {etCompElemType,          "etCompElemType",          ""},
                         {etCountry,               "etCountry",               "countries"},
                         {etCraft,                 "etCraft",                 "crafts"},
                         {etCurrency,              "etCurrency",              "currency"},
                         {etCustomAlarmType,       "etCustomAlarmType",       "custom_alarm_types"},
                         {etDCSAction,             "etDCSAction",             "dcs_actions"},
                         {etDelayType,             "etDelayType",             ""},
                         {etDeskGrp,               "etDeskGrp",               ""},
                         {etDevFmtType,            "etDevFmtType",            "dev_fmt_types"},
                         {etDevModel,              "etDevModel",              "dev_models"},
                         {etDevOperType,           "etDevOperType",           "dev_oper_types"},
                         {etDevSessType,           "etDevSessType",           "dev_sess_types"},
                         {etExtendedPersType,      "etExtendedPersType",      "extended_pers_types"},
                         {etGenderType,            "etGenderType",            "gender_types"},
                         {etGraphStage,            "etGraphStage",            "graph_stages"},
                         {etGraphStageWOInactive,  "etGraphStageWOInactive",  "graph_stages"},
                         {etGrpStatusType,         "etGrpStatusType",         "grp_status_types"},
                         {etHall,                  "etHall",                  ""},
                         {etHotel,                 "etHotel",                 ""},
                         {etHotelRoomType,         "etHotelRoomType",         ""},
                         {etKiosksGrp,             "etKiosksGrp",             ""},
                         {etLangType,              "etLangType",              "lang_types"},
                         {etMiscSetType,           "etMiscSetType",           "misc_set_types"},
                         {etMsgTransport,          "etMsgTransport",          "msg_transports"},
                         {etPaxDocCountry,         "etPaxDocCountry",         "pax_doc_countries"},
                         {etPaxDocSubtype,         "etPaxDocSubtype",         "pax_doc_subtypes"},
                         {etPaxDocType,            "etPaxDocType",            "pax_doc_types"},
                         {etPayType,               "etPayType",               "pay_types"},
                         {etPersType,              "etPersType",              "pers_types"},
                         {etProfiles,              "etProfiles",              ""},
                         {etPayMethodType,         "etPayMethodsTypes",       "pay_methods_types"},
                         {etRateColor,             "etRateColor",             "rate_colors"},
                         {etRcptDocType,           "etRcptDocType",           "rcpt_doc_types"},
                         {etRefusalType,           "etRefusalType",           "refusal_types"},
                         {etReportType,            "etReportType",             "report_types"},
                         {etRemGrp,                "etRemGrp",                ""},
                         {etRight,                 "etRight",                 "rights"},
                         {etRoles,                 "etRoles",                 ""},
                         {etSalePoint,             "etSalePoint",             ""},
                         {etSeasonType,            "etSeasonType",            "season_types"},
                         {etSeatAlgoType,          "etSeatAlgoType",          "seat_algo_types"},
                         {etStationMode,           "etStationMode",           "station_modes"},
                         {etSubcls,                "etSubcls",                "subcls"},
                         {etSuffix,                "etSuffix",                "trip_suffixes"},
                         {etTagColor,              "etTagColor",              "tag_colors"},
                         {etTripLiter,             "etTripLiter",             ""},
                         {etTripType,              "etTripType",              "trip_types"},
                         {etTypeBOptionValue,      "etTypeBOptionValue",      "typeb_option_values"},
                         {etTypeBSender,           "etTypeBSender",           ""},
                         {etTypeBType,             "etTypeBType",             "typeb_types"},
                         {etUsers,                 "etUsers",                 ""},
                         {etUserSetType,           "etUserSetType",           "user_set_types"},
                         {etUserType,              "etUserType",              "user_types"},
                         {etValidatorType,         "etValidatorType",         ""},
                         {etVoucherType,           "etVoucherType",           "voucher_types"},
                       };

TElemType DecodeElemType(const char *type)
{
    int i=sizeof(ElemBaseTables)/sizeof(ElemBaseTables[0])-1;
    for(;i>=0;i--)
        if ((string)ElemBaseTables[i].EncodeStr==type) return ElemBaseTables[i].ElemType;
    throw Exception("%s: not found for %s", __func__, type);
}

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
      TUserSettingType user_fmt = ustCodeNative;
      switch(ctxt) {
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
        if ( hard_verify || fmt == efmtUnknown ) {
          if (!((user_fmt==ustCodeNative     && fmt==efmtCodeNative) ||
                (user_fmt==ustCodeInter      && fmt==efmtCodeInter) ||
                (user_fmt==ustCodeICAONative && fmt==efmtCodeICAONative) ||
                (user_fmt==ustCodeICAOInter  && fmt==efmtCodeICAOInter) ||
                (user_fmt==ustCodeMixed      && (fmt==efmtCodeNative||
                                                fmt==efmtCodeInter||
                                                fmt==efmtCodeICAONative||
                                                fmt==efmtCodeICAOInter))))
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
          if (!((user_fmt==ustEncNative && fmt==efmtCodeNative) ||
                (user_fmt==ustEncLatin && fmt==efmtCodeInter) ||
                (user_fmt==ustEncMixed && (fmt==efmtCodeNative||fmt==efmtCodeInter))))
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
      TUserSettingType user_fmt = ustCodeNative;
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
        id=((const TCodeBaseTableRow&)CodeBaseTable.get_row("code",elem,with_deleted)).code;
        fmt=efmtCodeNative;
        return id;
      }
      catch (const EBaseTableError&) {};
      try
      {
        id=((const TCodeBaseTableRow&)CodeBaseTable.get_row("code_lat",elem,with_deleted)).code;
        fmt=efmtCodeInter;
        return id;
      }
      catch (const EBaseTableError&) {};

      if ((type==etAirp || type==etCity) &&
          (elem=="KVD" || elem=="GNJ"))
      {
        fmt=efmtCodeInter;
        return "ГНЖ";
      }
    }
    catch (const bad_cast&) {};

    try
    {
      TICAOBaseTable& ICAOBaseTable=dynamic_cast<TICAOBaseTable&>(BaseTable);
      //это code_icao,code_icao_lat
      try
      {
        id=((const TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao",elem,with_deleted)).code;
        fmt=efmtCodeICAONative;
        return id;
      }
      catch (const EBaseTableError&) {};
      try
      {
        id=((const TICAOBaseTableRow&)ICAOBaseTable.get_row("code_icao_lat",elem,with_deleted)).code;
        fmt=efmtCodeICAOInter;
        return id;
      }
      catch (const EBaseTableError&) {};
    }
    catch (const bad_cast&) {};

    try
    {
      TCountries& Countries=dynamic_cast<TCountries&>(BaseTable);
      //это code_iso
      try
      {
        id=((const TCountriesRow&)Countries.get_row("code_iso",elem,with_deleted)).code;
        fmt=efmtCodeISOInter;
        return id;
      }
      catch (const EBaseTableError&) {};
    }
    catch (const bad_cast&) {};
  }
  else
  {
    switch (type) {
        case etDelayType:
        {
          DB::TQuery Qry(PgOra::getROSession("DELAYS"), STDLOG);
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
        }
        break;

        case etTripLiter:
        {
          DB::TQuery Qry(PgOra::getROSession("TRIP_LITERS"), STDLOG);
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
        }
        break;

      //надо бы добавить в будущем etValidatorType
        default:;
    }

  };

  return id;
}

string ElemToElemId(TElemType type, const string &elem, TElemFmt &fmt, bool with_deleted)
{
  return ElemToElemId(type, elem, fmt, "", with_deleted);
};

template<class TQueryT>
void getElem(TElemFmt fmt, const std::string &lang, TQueryT& Qry, string &elem)
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
          (fmt==efmtCodeNative && lang!=AstraLocale::LANG_RU))
        field_idx=Qry.GetFieldIndex("code_lat");
      break;

    case efmtCodeICAONative:
    case efmtCodeICAOInter:
      if (fmt==efmtCodeICAONative && lang==AstraLocale::LANG_RU)
        field_idx=Qry.GetFieldIndex("code_icao");
      if (fmt==efmtCodeICAOInter ||
          (fmt==efmtCodeICAONative && lang!=AstraLocale::LANG_RU))
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
      catch (const EBaseTableError&) {};
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
      catch (const bad_cast&) {};
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
            (fmt==efmtCodeNative && lang!=AstraLocale::LANG_RU))
        {
          elem=row.code_lat;
          return;
        };
      }
      catch (const bad_cast&) {};
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
            (fmt==efmtCodeICAONative && lang!=AstraLocale::LANG_RU))
        {
          elem=row.code_icao_lat;
          return;
        };

      }
      catch (const bad_cast&) {};
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
      catch (const bad_cast&) {};
      break;

    case efmtUnknown:
      break;
  };
};

static const string& GetElemByElemIdQueryText(const TElemType& type)
{
    switch(type) {
    case etTypeBSender: {
        static const string text =
            "SELECT name, name_lat"
            "  FROM typeb_senders"
            "    WHERE code=:id";
        return text;
    }
    case etDelayType: {
        static const string text =
            "SELECT code, code_lat, name, name_lat"
            "  FROM delays"
            "    WHERE code=:id";
        return text;
    }
    case etTripLiter: {
        static const string text =
            "SELECT code, code_lat, name, name_lat"
            "  FROM trip_liters"
            "    WHERE code=:id";
        return text;
    }
    case etValidatorType: {
        static const string text =
            "SELECT code, code_lat, name, name_lat"
            "  FROM validator_types"
            "    WHERE code=:id";
        return text;
    }
    case etSalePoint: {
        static const string text =
            "SELECT code, descr name, descr_lat name_lat"
            "  FROM sale_points"
            "    WHERE code=:id";
        return text;
    }
    case etAgency: {
        static const string text =
            "SELECT code, code_lat, name, name_lat"
            "  FROM agencies"
            "    WHERE code=:id";
        return text;
    }
    case etCompElemType: {
        static const string text =
            "SELECT name, name_lat"
            "  FROM comp_elem_types"
            "    WHERE code=:id";
        return text;
    }
    case etBPType: {
        static const string text =
            "SELECT name AS name, name AS name_lat"
            "  FROM bp_types"
            "    WHERE code=:id AND op_type='PRINT_BP'";
        return text;
    }
    case etBIType: {
        static const string text =
            "SELECT name AS name, name AS name_lat"
            "  FROM bp_types"
            "    WHERE code=:id AND op_type='PRINT_BI'";
        return text;
    }
    case etVOType: {
        static const string text =
            "SELECT name AS name, name AS name_lat"
            "  FROM bp_types"
            "    WHERE code=:id AND op_type='PRINT_VO'";
        return text;
    }
    case etEMDAType: {
        static const string text =
            "SELECT name AS name, name AS name_lat"
            "  FROM bp_types"
            "    WHERE code=:id AND op_type='PRINT_EMDA'";
        return text;
    }
    case etBTType: {
        static const string text =
            "SELECT name AS name, name AS name_lat"
            "  FROM tag_types"
            "    WHERE code=:id";
        return text;
    }
    default: {
        break;
    }}
    throw Exception("Unexpected elem type %s", EncodeElemType(type));
}

template<class RowOrQueryT>
static string GetElem(const TElemFmt& fmt, const std::string &lang, RowOrQueryT&& rowOrQuery)
{
    string elem;
    getElem(fmt, lang, std::forward<RowOrQueryT>(rowOrQuery), elem);
    return elem;
}

static string GetElemByTableNameFromBaseTables(
    const string& table_name,
    const string& id,
    const vector<pair<TElemFmt, string>>& fmts,
    const bool with_deleted)
{
    try {
        const TBaseTableRow& BaseTableRow = base_tables.get(table_name).get_row("code", id, with_deleted);
        for (const auto& [fmt, lang] : fmts) {
            const string elem = GetElem(fmt, lang, BaseTableRow);
            if (!elem.empty()) {
                return elem;
            }
        };
    }
    catch (const EBaseTableError&) {
    };
    return "";
}

static string GetOraPgTableNameFromType(const TElemType& type)
{
    switch(type) {
    case etTypeBSender:   return "TYPEB_SENDERS";
    case etDelayType:     return "DELAYS";
    case etTripLiter:     return "TRIP_LITERS";
    case etValidatorType: return "VALIDATOR_TYPES";
    case etSalePoint:     return "SALE_POINTS";
    case etAgency:        return "AGENCIES";
    case etCompElemType:  return "COMP_ELEM_TYPES";
    case etBPType:
    case etBIType:
    case etVOType:
    case etEMDAType:      return "BP_TYPES";
    case etBTType:        return "TAG_TYPES";
    default: break;
    }
    return "";
}

template<class TQueryT>
static string GetElemByTypeFromDB_(
    TQueryT& Qry,
    const TElemType& type,
    const string& id,
    const vector<pair<TElemFmt, string>>& fmts,
    const bool with_deleted)
{
    Qry.SQLText = GetElemByElemIdQueryText(type);
    Qry.CreateVariable("id", otString, id);
    Qry.Execute();
    for (const auto& [fmt, lang] : fmts) {
        const string elem = GetElem(fmt, lang, Qry);
        if (!elem.empty()) {
            return elem;
        }
    };
    return "";
}

static string GetElemByTypeFromDB(
    const TElemType& type,
    const string& id,
    const vector<pair<TElemFmt, string>>& fmts,
    const bool with_deleted)
{
    const string table_name = GetOraPgTableNameFromType(type);
    if (!table_name.empty()) {
        DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
        return GetElemByTypeFromDB_(Qry, type, id, fmts, with_deleted);
    }
    else {
        TQuery Qry(&OraSession);
        return GetElemByTypeFromDB_(Qry, type, id, fmts, with_deleted);
    }
}

string ElemIdToElem(
    TElemType type,
    const string& id,
    const vector<pair<TElemFmt, string>>& fmts,
    bool with_deleted)
{
    if (id.empty()) {
        return "";
    }

    const string table_name = getTableName(type);
    if (!table_name.empty()) {
        return GetElemByTableNameFromBaseTables(table_name, id, fmts, with_deleted);
    }
    return GetElemByTypeFromDB(type, id, fmts, with_deleted);
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
      const TBaseTableRow& BaseTableRow=base_tables.get(table_name).get_row("id",id,with_deleted);
      for(vector< pair<TElemFmt,string> >::const_iterator i=fmts.begin();i!=fmts.end();i++)
      {
        getElem(i->first, i->second, BaseTableRow, elem);
        if (!elem.empty()) break;
      };
    }
    catch (const EBaseTableError&) {};
  } else
  {
    //не base_table
    std::string table_name;
    std::string sql;
    switch(type)
    {
    case etHall:
      table_name = "HALLS2";
      sql = "SELECT name,name_lat FROM halls2 WHERE id=:id";
      break;
    case etBIHall:
      table_name = "BI_HALLS";
      sql = "SELECT name,name_lat FROM bi_halls WHERE id=:id";
      break;
    case etDeskGrp:
      table_name = "DESK_GRP";
      sql = "SELECT descr AS name, descr_lat AS name_lat FROM desk_grp WHERE grp_id=:id";
      break;
    case etRemGrp:
      table_name = "REM_GRP";
      sql = "SELECT name, name_lat FROM rem_grp WHERE id=:id";
      break;
    case etUsers:
      table_name = "USERS2";
      sql = "SELECT descr AS name, descr AS name_lat FROM users2 WHERE user_id=:id";
      break;
    case etRoles:
      table_name = "ROLES";
      sql = "SELECT name AS name, name AS name_lat FROM roles WHERE role_id=:id";
      break;
    case etProfiles:
      table_name = "AIRLINE_PROFILES";
      sql = "SELECT name AS name, name AS name_lat FROM airline_profiles WHERE profile_id=:id";
      break;
    case etAirpTerminal:
      table_name = "AIRP_TERMINALS";
      sql = "SELECT name AS name, name AS name_lat FROM airp_terminals WHERE id=:id";
      break;
    case etBrand:
      table_name = "BRANDS";
      sql = "SELECT code, code code_lat, name, name_lat FROM brands WHERE id=:id";
      break;
    case etHotel:
      table_name = "HOTEL_ACMD";
      sql = "SELECT hotel_name name, hotel_name name_lat FROM hotel_acmd WHERE id=:id";
      break;
    case etHotelRoomType:
      table_name = "HOTEL_ROOM_TYPES";
      sql = "SELECT name, name name_lat FROM hotel_room_types WHERE id=:id";
      break;
    case etKiosksGrp:
      table_name = "KIOSKS_GRP";
      sql = "SELECT name, name name_lat FROM kiosks_grp WHERE id=:id";
      break;
    default: throw Exception("Unexpected elem type %s", EncodeElemType(type));
    };
    DB::TQuery Qry(PgOra::getROSession(table_name), STDLOG);
    Qry.SQLText = sql;
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
         [[fallthrough]];
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

std::string ElemIdToPrefferedElem(TElemType type, const int &id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  if (id==ASTRA::NoExists) return "";
  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, lang, fmts);
  return ElemIdToElem(type, id, fmts, with_deleted);
};

std::string ElemIdToPrefferedElem(TElemType type, const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  if (id.empty()) return "";
  vector< pair<TElemFmt,string> > fmts;
  getElemFmts(fmt, lang, fmts);
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

string ElemToPaxDocCountryId(const string &elem, TElemFmt &fmt)
{
  string result=ElemToElemId(etPaxDocCountry,elem,fmt);
  if (fmt==efmtUnknown)
  {
    //проверим countries
    string country=ElemToElemId(etCountry,elem,fmt);
    if (fmt!=efmtUnknown)
    {
      fmt=efmtUnknown;
      //найдем в pax_doc_countries.country
      try
      {
        result=ElemToElemId(etPaxDocCountry,
                            getBaseTable(etPaxDocCountry).get_row("country",country).AsString("code"),
                            fmt);
      }
      catch (const EBaseTableError&) {};
    };
  };
  return result;
}

std::string PaxDocCountryIdToPrefferedElem(const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted)
{
  string result;
  if (fmt==efmtCodeISOInter)
  {
    try
    {
      result=ElemIdToPrefferedElem(etCountry,
                                   getBaseTable(etPaxDocCountry).get_row("code",id).AsString("country"),
                                   fmt, lang, with_deleted);
    }
    catch (const EBaseTableError&) {}
  }
  if (result.empty())
    result=ElemIdToPrefferedElem(etPaxDocCountry, id, fmt, lang, with_deleted);
  return result;
}

std::string classIdsToCodeNative(const std::string &orig_cl,
                                 const std::string &cabin_cl)
{
  return (cabin_cl!=orig_cl?ElemIdToCodeNative(etClass, orig_cl)+"->":"") + ElemIdToCodeNative(etClass, cabin_cl);
}

std::string clsGrpIdsToCodeNative(const int orig_cl_grp,
                                  const int cabin_cl_grp)
{
  return (cabin_cl_grp!=orig_cl_grp?ElemIdToCodeNative(etClsGrp, orig_cl_grp)+"->":"") + ElemIdToCodeNative(etClsGrp, cabin_cl_grp);
}

AstraLocale::OutputLang::OutputLang(const std::string& lang) :
  _lang(lang.empty()?TReqInfo::Instance()->desk.lang:lang),
  _onlyTrueIATACodes(TReqInfo::Instance()->isSelfCkinClientType()) {}

AstraLocale::OutputLang::OutputLang(const std::string& lang,
                                    const std::set<Props>& props) :
  _lang(lang.empty()?TReqInfo::Instance()->desk.lang:lang),
  _onlyTrueIATACodes(props.find(OnlyTrueIataCodes)!=props.end()) {}

string airlineToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang.get());
  if (lang.onlyTrueIATACodes() && result.size()==3) //типа ИКАО
    result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang.get()==AstraLocale::LANG_EN?AstraLocale::LANG_RU:
                                                                                                   AstraLocale::LANG_EN);
  return result;
}

string airpToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang.get());
  if (lang.onlyTrueIATACodes() && result.size()==4) //типа ИКАО
    result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang.get()==AstraLocale::LANG_EN?AstraLocale::LANG_RU:
                                                                                                AstraLocale::LANG_EN);
  return result;
}

string craftToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang)
{
  string result;
  result=ElemIdToPrefferedElem(etCraft, code, efmtCodeNative, lang.get());
  if (lang.onlyTrueIATACodes() && result.size()==4) //типа ИКАО
    result=ElemIdToPrefferedElem(etCraft, code, efmtCodeNative, lang.get()==AstraLocale::LANG_EN?AstraLocale::LANG_RU:
                                                                                                 AstraLocale::LANG_EN);
  return result;
}

string getElemId(TElemType type, const string &elem)
{
    TElemFmt fmt;
    string result = ElemToElemId(type, elem, fmt, false);
    if(fmt == efmtUnknown)
        throw Exception("getElemId: elem not found (type = %s, elem = %s)",
                EncodeElemType(type),elem.c_str());
    return result;
}
