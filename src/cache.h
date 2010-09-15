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
enum TCacheElemCategory {cecNone, cecCode, cecName};

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
        bool Keep_Locally;
        int SelectRight;
        int InsertRight;
        int UpdateRight;
        int DeleteRight;
        bool Forbidden, ReadOnly;
        std::vector<TCacheField2> FFields;
        int clientVerData;
        int curVerIface;
        int clientVerIface;
        TTable table;
        std::vector<std::string> vars;

        void getPerms( );
        bool pr_irefresh, pr_drefresh;
        void getParams(xmlNodePtr paramNode, TParams &vparams);
        bool refreshInterface();
        bool refreshData();
        virtual void initFields();
        void XMLInterface(const xmlNodePtr resNode);
        void XMLData(const xmlNodePtr resNode);
        void DeclareSysVariables(std::vector<std::string> &vars, TQuery *Qry);
        void DeclareVariables(std::vector<std::string> &vars);
        void SetVariables(TRow &row, const std::vector<std::string> &vars);
        void parse_updates(xmlNodePtr rowsNode);
        int getIfaceVer();
        void OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus );
        void Clear();
    public:
        TBeforeRefreshEvent OnBeforeRefresh;
        TBeforeApplyEvent OnBeforeApply;
        void refresh();
        void buildAnswer(xmlNodePtr resNode);
        void ApplyUpdates(xmlNodePtr reqNode);
        bool changeIfaceVer();
        std::string code();
        int FieldIndex( const std::string name );
        std::string FieldValue( const std::string name, const TRow &row );
        std::string FieldOldValue( const std::string name, const TRow &row );
        TCacheTable()
        {
          OnBeforeApply = NULL;
          OnBeforeRefresh = NULL;
        };
        virtual void Init(xmlNodePtr cacheNode);
        virtual ~TCacheTable();
};
#endif /*_CACHE_H_*/

