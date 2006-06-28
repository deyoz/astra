#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "oralib.h"
#include "astra_utils.h"

#include "JxtInterface.h"

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


enum TCacheFieldCharCase {ecNormal, ecUpperCase, ecLowerCase};
enum TAlignment {taLeftJustify, taRightJustify, taCenter};
enum TCacheFieldType {ftSignedNumber, ftUnsignedNumber, ftDate, ftTime, ftString, ftBoolean, ftStringList,
                      ftUnknown, NumFieldType};
enum TCacheConvertType {ctInteger,ctDouble,ctDateTime,ctString};
enum TCacheUpdateStatus {usUnmodified, usModified, usInserted, usDeleted};


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
    int VarIdx[2];
    TCacheField2()
    {
        Width = 0;
        DataSize = 0;
        Scale = 0;
        Nullable = true;
        Ident = false;
        ReadOnly = true;
        ReferLevel = 0;
        VarIdx[0] = -1;
        VarIdx[1] = -1;
    }
};

typedef struct {
    std::string Value;
    TCacheConvertType DataType;
} TParam;

typedef std::map<std::string, TParam> TParams;

typedef struct {
    std::vector<std::string> cols;
    std::vector<std::string> old_cols;
    std::vector<std::string> new_cols;
    TCacheUpdateStatus status;
    TParams params;
    int index;
} TRow;

typedef std::vector<TRow> TTable;

class TCacheTable {
    private:
        TQuery *Qry;
        TParams Params, SQLParams;
        std::string title;
        std::string SelectSQL;
        std::string RefreshSQL;
        std::string InsertSQL;
        std::string UpdateSQL;
        std::string DeleteSQL;
        TEventType EventType;
        bool Logging;
        bool Forbidden, ReadOnly;
        std::vector<TCacheField2> FFields;
        int tid, itid;
        int newitid; // текущее значение tid интерфейса
        TTable table;

        bool pr_irefresh, pr_drefresh;
        void getParams(xmlNodePtr paramNode, TParams &vparams);
        bool refreshInterface();
        bool refreshData();
        void initFields();
        void XMLInterface(const xmlNodePtr resNode);
        void XMLData(const xmlNodePtr resNode);
        void DeclareVariables(const std::vector<std::string> &vars);
        void SetVariables(TRow &row, const std::vector<std::string> &vars);
        void parse_updates(xmlNodePtr rowsNode);
        int getIfaceVer();
        void OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus,
                        const std::vector<std::string> &vars );
    public:
        void refresh();
        void buildAnswer(xmlNodePtr resNode);
        void ApplyUpdates(xmlNodePtr reqNode);
        bool changeIfaceVer();
        TCacheTable(xmlNodePtr cacheNode);
        ~TCacheTable();
};
#endif /*_CACHE_H_*/

