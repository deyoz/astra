#pragma once

#include <string>
#include <map>
#include <libxml/tree.h>
#include "oralib.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "db_tquery.h"
#include "cache_callbacks.h"

#include "jxtlib/JxtInterface.h"

class CacheInterface : public JxtInterface
{
public:
  CacheInterface() : JxtInterface("","cache")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CacheInterface>::CreateHandler(&CacheInterface::LoadCache);
     AddEvent("cache",evHandle);
     evHandle=JxtHandler<CacheInterface>::CreateHandler(&CacheInterface::SaveCache);
     AddEvent("cache_apply",evHandle);
  };

  void LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
};

#define TAG_REFRESH_DATA        "DATA_VER"
#define TAG_REFRESH_INTERFACE   "INTERFACE_VER"
#define TAG_CODE                "CODE"

enum TCacheFieldCharCase {ecNormal, ecUpperCase, ecLowerCase};
enum TCacheFieldType {ftSignedNumber, ftUnsignedNumber, ftDate, ftTime, ftString, ftBoolean, ftStringList, ftUTF,
                      ftUnknown, NumFieldType};
enum TCacheUpdateStatus {usUnmodified, usModified, usInserted, usDeleted};
enum TCacheQueryType {cqtSelect,cqtRefresh,cqtInsert,cqtUpdate,cqtDelete};
enum TCacheElemCategory {
    cecNone,
    cecCode,
    cecNameShort,
    cecName,
    cecRoleName,
    cecUserName
};

extern const char * CacheFieldTypeS[NumFieldType];

const
  struct
  {
    const char* CacheCode;
    TElemType ElemType;
  } ReferCacheTable[] = {
                         {"AIRLINES",                etAirline},
                         {"AIRPS",                   etAirp},
                         {"AIRP_TERMINALS",          etAirpTerminal},
                         {"APIS_TRANSPORTS",         etMsgTransport},
                         {"BAG_NORM_TYPES",          etBagNormType},
                         {"BAG_TYPES",               etBagType},
                         {"BI_HALLS",                etBIHall},
                         {"BI_PRINT_TYPES",          etBIPrintType},
                         {"BRANDS",                  etBrand},
                         {"CITIES",                  etCity},
                         {"CKIN_REM_TYPES",          etCkinRemType},
                         {"CLASSES",                 etClass},
                         {"CODE4_FMT",               etUserSetType},
                         {"COUNTRIES",               etCountry},
                         {"CRAFTS",                  etCraft},
                         {"CURRENCY",                etCurrency},
                         {"CUSTOM_ALARM_TYPES",      etCustomAlarmType},
                         {"DCS_ACTIONS1",            etDCSAction},
                         {"DCS_ACTIONS2",            etDCSAction},
                         {"DESK_GRP",                etDeskGrp},
                         {"ENCODING_FMT",            etUserSetType},
                         {"FQT_REM_TYPES",           etCkinRemType},
                         {"GRAPH_STAGES",            etGraphStage},
                         {"GRAPH_STAGES_WO_INACTIVE",etGraphStageWOInactive},
                         {"HALLS",                   etHall},
                         {"KIOSK_CKIN_DESK_GRP",     etDeskGrp},
                         {"KIOSKS_GRP",              etKiosksGrp},
                         {"MISC_SET_TYPES",          etMiscSetType},
                         {"NO_TXT_REPORT_TYPES",     etReportType},
                         {"PAY_METHODS_TYPES",       etPayMethodType},
                         {"RATE_COLORS",             etRateColor},
                         {"REM_GRP",                 etRemGrp},
                         {"REPORT_TYPES",            etReportType},
                         {"RIGHTS",                  etRight},
                         {"SEASON_TYPES",            etSeasonType},
                         {"SEAT_ALGO_TYPES",         etSeatAlgoType},
                         {"SELF_CKIN_SET_TYPES",     etMiscSetType},
                         {"SELF_CKIN_TYPES",         etClientType},
                         {"STATION_MODES",           etStationMode},
                         {"SUBCLS",                  etSubcls},
                         {"TIME_FMT",                etUserSetType},
                         {"TRIP_SUFFIXES",           etSuffix},
                         {"TRIP_TYPES",              etTripType},
                         {"TYPEB_LCI_ACTION_CODE",   etTypeBOptionValue},
                         {"TYPEB_LCI_SEAT_PLAN",     etTypeBOptionValue},
                         {"TYPEB_LCI_SEAT_RESTRICT", etTypeBOptionValue},
                         {"TYPEB_LCI_WEIGHT_AVAIL",  etTypeBOptionValue},
                         {"TYPEB_LDM_VERSION",       etTypeBOptionValue},
                         {"TYPEB_LDM_CFG",           etTypeBOptionValue},
                         {"TYPEB_PRL_CREATE_POINT",  etTypeBOptionValue},
                         {"TYPEB_PRL_PAX_STATE",     etTypeBOptionValue},
                         {"TYPEB_TRANSPORTS_OTHERS", etMsgTransport},
                         {"TYPEB_TYPES",             etTypeBType},
                         {"TYPEB_TYPES_ALL",         etTypeBType},
                         {"TYPEB_TYPES_MARK",        etTypeBType},
                         {"TYPEB_TYPES_BSM",         etTypeBType},
                         {"TYPEB_TYPES_LDM",         etTypeBType},
                         {"TYPEB_TYPES_LCI",         etTypeBType},
                         {"TYPEB_TYPES_PNL",         etTypeBType},
                         {"TYPEB_TYPES_PRL",         etTypeBType},
                         {"TYPEB_TYPES_MVT",         etTypeBType},
                         {"TYPEB_TYPES_ETL",         etTypeBType},
                         {"TYPEB_TYPES_FORWARDING",  etTypeBType},
                         {"USER_TYPES",              etUserType},
                         {"VALIDATOR_TYPES",         etValidatorType}
                        };

struct TCacheChildField {
    std::string field_parent, field_child;
    std::string select_var, insert_var, update_var, delete_var;
    bool auto_insert, check_equal, read_only;
};

struct TCacheChildTable {
    std::string code, title;
    std::vector<TCacheChildField> fields;
};

struct TCacheField2 {
    std::string Name;
    std::string Title;
    int Width;
    TCacheFieldCharCase CharCase;
    TAlignment::Enum Align;
    TCacheFieldType DataType;
    int DataSize;
    int Scale;
    bool Nullable;
    bool Ident;
    bool ReadOnly;
    std::string ReferCode;
    std::string ReferName;
    int ReferLevel;
    int ReferIdent;
    int VarIdx[2];
    int num;
    TCacheElemCategory ElemCategory;
    TElemType ElemType;
    TCacheField2()
    {
        Width = 0;
        DataSize = 0;
        Scale = 0;
        Nullable = true;
        Ident = false;
        ReadOnly = true;
        ReferLevel = 0;
        ReferIdent = -1;
        VarIdx[0] = -1;
        VarIdx[1] = -1;
        ElemCategory = cecNone;
    }
};

typedef struct {
    std::vector<std::string> cols;
    std::vector<std::string> old_cols;
    TCacheUpdateStatus status;
    TParams params;
} TRow;

typedef std::vector<TRow> TTable;

class TCacheTable;

typedef void  (*TBeforeRefreshEvent)(TCacheTable &, DB::TQuery &, const TCacheQueryType);
typedef void  (*TBeforeApplyEvent)(TCacheTable &, const TRow &, DB::TQuery &, const TCacheQueryType);
typedef void  (*TAfterApplyEvent)(TCacheTable &, const TRow &, DB::TQuery &, const TCacheQueryType);
typedef void  (*TBeforeApplyAllEvent)(TCacheTable &);
typedef void  (*TAfterApplyAllEvent)(TCacheTable &);

class TCacheTable {
    protected:
        std::unique_ptr<CacheTableCallbacks> callbacks;
        TParams Params, SQLParams;
        std::string Title;
        std::string SelectSQL;
        std::string RefreshSQL;
        std::string InsertSQL;
        std::string UpdateSQL;
        std::string DeleteSQL;
        std::string dbSessionObjectName;
        ASTRA::TEventType EventType;
        bool Logging;
        bool KeepLocally;
        bool KeepDeletedRows;
        std::optional<int> SelectRight;
        std::optional<int> InsertRight;
        std::optional<int> UpdateRight;
        std::optional<int> DeleteRight;
        bool Forbidden, ReadOnly;
        std::vector<TCacheChildTable> FChildTables;
        std::vector<TCacheField2> FFields;
        int clientVerData;
        int curVerIface;
        int clientVerIface;
        TTable table;
        std::optional<CacheTable::SelectedRows> selectedRows;

        void getPerms( );
        bool pr_irefresh, pr_dconst;
        CacheTable::RefreshStatus refresh_data_type;
        void getParams(xmlNodePtr paramNode, TParams &vparams);
        bool refreshInterface();
        CacheTable::RefreshStatus refreshData();
        CacheTable::RefreshStatus refreshDataIndividual();
        CacheTable::RefreshStatus refreshDataCommon();
        virtual void initChildTables();
        virtual void initFields();
        void XMLInterface(const xmlNodePtr resNode);
        void XMLData(const xmlNodePtr resNode);
        void DeclareSysVariable(const std::string& name, const int value,
                                std::vector<std::string> &vars, DB::TQuery& Qry,
                                FieldsForLogging& fieldsForLogging);
        void DeclareSysVariable(const std::string& name, const std::string& value,
                                std::vector<std::string> &vars, DB::TQuery& Qry,
                                FieldsForLogging& fieldsForLogging);
        void DeclareSysVariables(std::vector<std::string> &vars, DB::TQuery& Qry,
                                 FieldsForLogging& fieldsForLogging);
        void DeclareVariables(const std::vector<std::string> &vars, DB::TQuery& Qry);
        void SetVariables(const TRow &row, const std::vector<std::string> &vars,
                          DB::TQuery& Qry, FieldsForLogging& fieldsForLogging);
        void parse_updates(xmlNodePtr rowsNode);
        int getIfaceVer();
        void OnLogging(const TRow &row, TCacheUpdateStatus UpdateStatus,
                       const std::vector<std::string> &vars,
                       const FieldsForLogging& fieldsForLogging);
        void Clear();
    public:
        TBeforeRefreshEvent OnBeforeRefresh;
        TBeforeApplyEvent OnBeforeApply;
        TAfterApplyEvent OnAfterApply;
        TBeforeApplyAllEvent OnBeforeApplyAll;
        TAfterApplyAllEvent OnAfterApplyAll;
        void refresh();
        void buildAnswer(xmlNodePtr resNode);
        void ApplyUpdates(xmlNodePtr reqNode);
        bool changeIfaceVer();
        std::string code();
        std::optional<int> dataVersion() const;
        std::string getSelectSql() { return SelectSQL; }
        int FieldIndex( const std::string name );
        std::string FieldValue( const std::string name, const TRow &row );
        std::string FieldOldValue( const std::string name, const TRow &row );
        TCacheTable()
        {
          OnBeforeApplyAll = NULL;
          OnAfterApplyAll = NULL;
          OnBeforeApply = NULL;
          OnBeforeRefresh = NULL;
          OnAfterApply = NULL;
        };
        virtual void Init(xmlNodePtr cacheNode);
        virtual ~TCacheTable() {};
};

std::string get_role_name(int role_id);

inline bool lf( const TCacheField2 &item1, const TCacheField2 &item2 )
{
    return item1.num<item2.num;
}

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

class CacheTableTermRequest : public TCacheTable
{
  private:
    std::string queryLogin;
    std::string queryLang;
    boost::optional<XMLDoc> sqlParamsDoc;
    std::list< std::pair<TCacheQueryType, std::map<std::string, std::string> > > appliedRows;
    void addAppliedRow(const TCacheQueryType queryType,
                       const std::vector<std::string>::const_iterator& b,
                       const std::vector<std::string>::const_iterator& e);
    void addSQLParams(const std::vector<std::string>::const_iterator& b,
                      const std::vector<std::string>::const_iterator& e);
    void appliedRowToXml(const int rowIndex,
                         const TCacheQueryType queryType,
                         const std::map<std::string, std::string>& fields,
                         xmlNodePtr rowsNode) const;
    void queryPropsToXml(xmlNodePtr queryNode) const;

    static boost::optional<TCacheQueryType> tryGetQueryType(const std::string& par);
    static std::string getAppliedRowStatus(const TCacheQueryType queryType);

  public:
    CacheTableTermRequest(const std::vector<std::string> &par);
    std::string getXml() const;

    static boost::optional<int> getInterfaceVersion(const std::string& cacheCode);
    static std::string getSQLParamXml(const std::vector<std::string> &par);

};

#endif/*XP_TESTING*/


