#pragma once

#include <map>
#include <vector>
#include <string>

#include "xml_unit.h"
#include "db_tquery.h"
#include "hist.h"

#include <serverlib/dbcpp_cursctl.h>

enum TCacheConvertType {ctInteger,ctDouble,ctDateTime,ctString};

std::ostream & operator <<(std::ostream& os, TCacheConvertType value);

enum TCacheUpdateStatus {usUnmodified, usModified, usInserted, usDeleted};
enum TCacheQueryType {cqtSelect, cqtRefresh, cqtInsert, cqtUpdate, cqtDelete};

struct TParam {
    std::string Value;
    TCacheConvertType DataType;
    TParam() { DataType = ctString; };
};

typedef std::map<std::string, TParam> TParams;

std::ostream & operator <<(std::ostream& os, const TParams& params);

class FieldsForLogging
{
  private:
    std::map<std::string, std::string> fields;
  public:
    void set(const std::string& name, const std::string& value);
    std::string get(const std::string& name) const;
    void trace() const;
};

void DeclareVariable(const std::string& name, const TCacheConvertType type, DB::TQuery &Qry);
void SetVariable(const std::string& name, const TCacheConvertType type, const std::string& value, DB::TQuery &Qry);
void CreateVariable(const std::string& name, const TCacheConvertType type, const std::string& value, DB::TQuery &Qry);

void DeclareVariablesFromParams(const std::set<std::string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                                const bool onlyIfParamExists=true);

void SetVariablesFromParams(const std::set<std::string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                            FieldsForLogging& fieldsForLogging, const bool onlyIfParamExists=true);

void CreateVariablesFromParams(const std::set<std::string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                               const bool onlyIfParamExists=true);

std::string getParamValue(const std::string& name, const TParams &SQLParams);
std::string notEmptyParamAsString(const std::string& name, const TParams &SQLParams);
int notEmptyParamAsInteger(const std::string& name, const TParams &SQLParams);
bool notEmptyParamAsBoolean(const std::string& name, const TParams &SQLParams);

RowId_t newRowId();

class GeneratedTid
{
  private:
    mutable std::optional<int> tid;
  public:
    int get() const;
};

namespace CacheTable
{

enum class RefreshStatus { None, Exists, ClearAll };

class SelectedRow
{
  public:
    std::vector<std::string> fields;
    std::optional<bool> deleted;
    SelectedRow(const size_t fieldsCount) : fields(fieldsCount) {}
};

class SelectedRows
{
  private:
    std::map<std::string, size_t> fieldIndexes_;
    std::vector<SelectedRow> rows;
    size_t currentFieldIdx_=0;
    std::optional<int> maxSelectedTid;
    std::optional<int> tid_;

    SelectedRow& currentRow();
    size_t& currentFieldIdx();
    SelectedRows& setCurrentField(const std::string& value, const std::string& whence);
    const std::optional<int>& tid() const { return tid_; }
  public:
    SelectedRows(const std::map<std::string, size_t>& fieldIndexes,
                 const std::optional<int>& tid);

    SelectedRows& setField(const std::string& name, const std::string& value);

    SelectedRows& setFromString  (const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromInteger (const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromBoolean (const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromDateTime(const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromDouble  (const size_t idx, DB::TQuery &Qry, const int qryIdx);

    SelectedRows& setFromString  (DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromInteger (DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromBoolean (DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromDateTime(DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromDouble  (DB::TQuery &Qry, const int qryIdx);

    SelectedRows& setFromString (const std::string& value);
    SelectedRows& setFromInteger(const std::optional<int>& value);
    SelectedRows& setFromBoolean(const std::optional<bool>& value);

    void addRow();
    void addRow(const bool deletedStatus, const std::optional<int> tid);

    void toXML(const xmlNodePtr dataNode) const;

    RefreshStatus status() const;
};

class Row
{
  private:
    std::map<std::string, std::string> fields_;
    std::string& fieldValue(const std::string& name, const std::string& whence);
    const std::string& fieldValue(const std::string& name, const std::string& whence) const;
  public:
    Row(const std::map<std::string, std::string>& fields);

    Row& setFromString (const std::string& name, const std::string& value);
    Row& setFromInteger(const std::string& name, const std::optional<int>& value);
    Row& setFromBoolean(const std::string& name, const std::optional<bool>& value);

    std::string getAsString(const std::string& name) const;
    std::string getAsString(const std::string& name, const std::string& defaultValue) const;
    std::string getAsString_ThrowOnEmpty(const std::string& name) const;

    std::optional<int> getAsInteger(const std::string& name) const;
    int getAsInteger(const std::string& name, int defaultValue) const;
    int getAsInteger_ThrowOnEmpty(const std::string& name) const;

    std::optional<bool> getAsBoolean(const std::string& name) const;
    bool getAsBoolean(const std::string& name, bool defaultValue) const;
    bool getAsBoolean_ThrowOnEmpty(const std::string& name) const;

    std::optional<TDateTime> getAsDateTime(const std::string& name) const;
    TDateTime getAsDateTime(const std::string& name, TDateTime defaultValue) const;
    TDateTime getAsDateTime_ThrowOnEmpty(const std::string& name) const;

    std::optional<double> getAsDouble(const std::string& name) const;
    double getAsDouble(const std::string& name, double defaultValue) const;
    double getAsDouble_ThrowOnEmpty(const std::string& name) const;
};

void setRowId(const std::string& fieldName,
              const TCacheUpdateStatus status,
              std::optional<CacheTable::Row>& newRow);

RowId_t getRowId(const std::string& fieldName,
                 const std::optional<CacheTable::Row>& oldRow,
                 const std::optional<CacheTable::Row>& newRow);


}

class CacheTableCallbacks
{
  public:
    virtual std::string selectSql() const =0;
    virtual std::string refreshSql() const =0;
    virtual std::string insertSql() const =0;
    virtual std::string updateSql() const =0;
    virtual std::string deleteSql() const =0;
    virtual std::list<std::string> dbSessionObjectNames() const =0;
    virtual std::list<std::string> dbSessionObjectNamesForRead() const { return dbSessionObjectNames(); }
    virtual std::list<std::string> dbSessionObjectNamesForWrite() const { return dbSessionObjectNames(); }
    virtual void beforeSelectOrRefresh(const TCacheQueryType queryType,
                                       const TParams& sqlParams,
                                       DB::TQuery& Qry) const {}
    virtual void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const =0;
    virtual bool userDependence() const =0;

    virtual bool insertImplemented() const { return !insertSql().empty(); }
    virtual bool updateImplemented() const { return !updateSql().empty(); }
    virtual bool deleteImplemented() const { return !deleteSql().empty(); }
    virtual void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                          const std::optional<CacheTable::Row>& oldRow,
                                          std::optional<CacheTable::Row>& newRow) const =0;
    virtual void onApplyingRowChanges(const TCacheUpdateStatus status,
                                      const std::optional<CacheTable::Row>& oldRow,
                                      const std::optional<CacheTable::Row>& newRow) const =0;
    virtual void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                         const std::optional<CacheTable::Row>& oldRow,
                                         const std::optional<CacheTable::Row>& newRow) const =0;
    virtual ~CacheTableCallbacks();
};

class CacheTableReadonly : public CacheTableCallbacks
{
  public:
    std::string refreshSql() const { return ""; }
    std::string insertSql() const { return ""; }
    std::string updateSql() const { return ""; }
    std::string deleteSql() const { return ""; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const {}
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const {}
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const {}
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const {}
};

class CacheTableReadonlyHandmade : public CacheTableCallbacks
{
  public:
    std::string selectSql() const { return ""; };
    std::string refreshSql() const { return ""; }
    std::string insertSql() const { return ""; }
    std::string updateSql() const { return ""; }
    std::string deleteSql() const { return ""; }
    std::list<std::string> dbSessionObjectNames() const { return {}; }
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const {}
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const {}
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const {}
};

class CacheTableWritable : public CacheTableCallbacks
{
  public:
    std::string refreshSql() const { return ""; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const {}
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const {}
};

class CacheTableWritableHandmade : public CacheTableCallbacks
{
  public:
    GeneratedTid generatedTid;
    std::string refreshSql() const { return ""; }
    std::string insertSql() const { return ""; }
    std::string updateSql() const { return ""; }
    std::string deleteSql() const { return ""; }
    std::list<std::string> dbSessionObjectNames() const { return {}; }
    void beforeApplyingRowChanges(const TCacheUpdateStatus status,
                                  const std::optional<CacheTable::Row>& oldRow,
                                  std::optional<CacheTable::Row>& newRow) const {}
    void afterApplyingRowChanges(const TCacheUpdateStatus status,
                                 const std::optional<CacheTable::Row>& oldRow,
                                 const std::optional<CacheTable::Row>& newRow) const {}
};

class CacheTableKeepDeletedRows : public CacheTableWritableHandmade
{
  private:
    void bindAndExec(const RowId_t& rowId, const CacheTable::Row& row,
                     const bool deleted, DbCpp::CursCtl& cur) const;
    void insert(const RowId_t& rowId, const CacheTable::Row& row,
                const bool deletedAfterInsert) const;
    void update(const RowId_t& rowId, const CacheTable::Row& row,
                const bool deletedBeforeUpdate) const;
  public:
    bool insertImplemented() const override { return true; }
    bool updateImplemented() const override { return true; }
    bool deleteImplemented() const override { return true; }
    void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const override {}
    void onApplyingRowChanges(const TCacheUpdateStatus status,
                              const std::optional<CacheTable::Row>& oldRow,
                              const std::optional<CacheTable::Row>& newRow) const override;

    virtual std::string insertSqlOnApplyingChanges() const = 0;
    virtual std::string updateSqlOnApplyingChanges() const = 0;
    virtual std::string deleteSqlOnApplyingChanges() const = 0;
    virtual std::string tableName() const = 0;
    virtual std::string idFieldName() const = 0;
    virtual void bind(const CacheTable::Row& row, DbCpp::CursCtl& cur) const = 0;
    virtual std::optional<RowId_t> getRowIdBeforeInsert(const CacheTable::Row& row) const = 0;
};

using TFieldTitles = std::map<std::string, std::string>;
struct TCacheInfo {
    std::string title;
    TFieldTitles fieldTitle;
};
TCacheInfo getCacheInfo(const std::string &code);
