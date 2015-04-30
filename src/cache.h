#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "oralib.h"
#include "astra_elems.h"
#include "astra_utils.h"

#include "jxtlib/JxtInterface.h"

using namespace ASTRA;
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
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#define TAG_REFRESH_DATA        "DATA_VER"
#define TAG_REFRESH_INTERFACE   "INTERFACE_VER"
#define TAG_CODE                "CODE"

enum TCacheFieldCharCase {ecNormal, ecUpperCase, ecLowerCase};
enum TAlignment {taLeftJustify, taRightJustify, taCenter};
enum TCacheFieldType {ftSignedNumber, ftUnsignedNumber, ftDate, ftTime, ftString, ftBoolean, ftStringList,
                      ftUnknown, NumFieldType};
enum TCacheConvertType {ctInteger,ctDouble,ctDateTime,ctString};
enum TCacheUpdateStatus {usUnmodified, usModified, usInserted, usDeleted};
enum TCacheQueryType {cqtSelect,cqtRefresh,cqtInsert,cqtUpdate,cqtDelete};
enum TCacheElemCategory {cecNone, cecCode, cecNameShort, cecName, cecRoleName, cecUserName, cecUserPerms};

extern const char * CacheFieldTypeS[NumFieldType];

const
  struct
  {
    const char* CacheCode;
    TElemType ElemType;
  } ReferCacheTable[] = {
                         {"AIRLINES",                etAirline},
                         {"AIRPS",                   etAirp},
                         {"BAG_NORM_TYPES",          etBagNormType},
                         {"BAG_TYPES",               etBagType},
                         {"CITIES",                  etCity},
                         {"CKIN_REM_TYPES",          etCkinRemType},
                         {"CLASSES",                 etClass},
                         {"CODE4_FMT",               etUserSetType},
                         {"COUNTRIES",               etCountry},
                         {"CRAFTS",                  etCraft},
                         {"CURRENCY",                etCurrency},
                         {"DESK_GRP",                etDeskGrp},
                         {"ENCODING_FMT",            etUserSetType},
                         {"GRAPH_STAGES",            etGraphStage},
                         {"GRAPH_STAGES_WO_INACTIVE",etGraphStageWOInactive},
                         {"HALLS",                   etHall},
                         {"KIOSK_CKIN_DESK_GRP",     etDeskGrp},
                         {"MISC_SET_TYPES",          etMiscSetType},
                         {"REM_GRP",                 etRemGrp},
                         {"RIGHTS",                  etRight},
                         {"SEASON_TYPES",            etSeasonType},
                         {"SEAT_ALGO_TYPES",         etSeatAlgoType},
                         {"SELF_CKIN_SET_TYPES",     etMiscSetType},
                         {"SELF_CKIN_TYPES",         etClientType},
                         {"STATION_MODES",           etStationMode},
                         {"SUBCLS",                  etSubcls},
                         {"TIME_FMT",                etUserSetType},
                         {"TRIP_TYPES",              etTripType},
                         {"TYPEB_LCI_ACTION_CODE",   etTypeBOptionValue},
                         {"TYPEB_LCI_SEAT_RESTRICT", etTypeBOptionValue},
                         {"TYPEB_LCI_SEAT_PLAN",     etTypeBOptionValue},
                         {"TYPEB_LCI_WEIGHT_AVAIL",  etTypeBOptionValue},
                         {"TYPEB_LDM_VERSION",       etTypeBOptionValue},
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
    TAlignment Align;
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

struct TParam {
    std::string Value;
    TCacheConvertType DataType;
    TParam() { DataType = ctString; };
};

typedef std::map<std::string, TParam> TParams;

class TParams1 : public  std::map<std::string, TParam>
{
    private:
    public:
        void getParams(xmlNodePtr paramNode);
        void setSQL(TQuery *Qry);
};

typedef struct {
    std::vector<std::string> cols;
    std::vector<std::string> old_cols;
/*    std::vector<std::string> new_cols; */
    TCacheUpdateStatus status;
    TParams params;
    int index;
} TRow;

typedef std::vector<TRow> TTable;

class TCacheTable;

typedef void  (*TBeforeRefreshEvent)(TCacheTable &, TQuery &, const TCacheQueryType);
typedef void  (*TBeforeApplyEvent)(TCacheTable &, const TRow &, TQuery &, const TCacheQueryType);
typedef void  (*TAfterApplyEvent)(TCacheTable &, const TRow &, TQuery &, const TCacheQueryType);
typedef void  (*TBeforeApplyAllEvent)(TCacheTable &);
typedef void  (*TAfterApplyAllEvent)(TCacheTable &);

enum TUpdateDataType {upNone, upExists, upClearAll};

class TCacheTable {
    protected:
        TQuery *Qry;
        TParams Params, SQLParams;
        std::string Title;
        std::string SelectSQL;
        std::string RefreshSQL;
        std::string InsertSQL;
        std::string UpdateSQL;
        std::string DeleteSQL;
        TEventType EventType;
        bool Logging;
        bool KeepLocally;
        bool KeepDeletedRows;
        int SelectRight;
        int InsertRight;
        int UpdateRight;
        int DeleteRight;
        bool Forbidden, ReadOnly;
        std::vector<TCacheChildTable> FChildTables;
        std::vector<TCacheField2> FFields;
        int clientVerData;
        int curVerIface;
        int clientVerIface;
        TTable table;
        std::vector<std::string> vars;

        void getPerms( );
        bool pr_irefresh, pr_dconst;
        TUpdateDataType refresh_data_type;
        void getParams(xmlNodePtr paramNode, TParams &vparams);
        bool refreshInterface();
        TUpdateDataType refreshData();
        virtual void initChildTables();
        virtual void initFields();
        void XMLInterface(const xmlNodePtr resNode);
        void XMLData(const xmlNodePtr resNode);
        void DeclareSysVariables(std::vector<std::string> &vars, TQuery *Qry);
        void DeclareVariables(const std::vector<std::string> &vars);
        void SetVariables(TRow &row, const std::vector<std::string> &vars);
        void parse_updates(xmlNodePtr rowsNode);
        int getIfaceVer();
        void OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus );
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
        virtual ~TCacheTable();
};

std::string get_role_name(int role_id, TQuery &Qry);
std::string get_user_descr(int user_id,
                      TQuery &Qry, TQuery &Qry1, TQuery &Qry2,
                      bool only_airlines_airps);

inline bool lf( const TCacheField2 &item1, const TCacheField2 &item2 )
{
    return item1.num<item2.num;
}

#endif /*_CACHE_H_*/

