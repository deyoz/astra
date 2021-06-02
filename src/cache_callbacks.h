#pragma once

#include <map>
#include <vector>
#include <string>

#include "xml_unit.h"
#include "db_tquery.h"

enum TCacheConvertType {ctInteger,ctDouble,ctDateTime,ctString};

struct TParam {
    std::string Value;
    TCacheConvertType DataType;
    TParam() { DataType = ctString; };
};

typedef std::map<std::string, TParam> TParams;

class FieldsForLogging
{
  private:
    std::map<std::string, std::string> fields;
  public:
    void set(const std::string& name, const std::string& value);
    std::string get(const std::string& name) const;
    void trace() const;
};



void DeclareVariablesFromParams(const std::vector<std::string> &vars, const TParams &SQLParams, DB::TQuery &Qry);
void SetVariablesFromParams(const TParams &SQLParams, DB::TQuery &Qry, FieldsForLogging& fieldsForLogging);
void CreateVariablesFromParams(const std::vector<std::string> &vars, const TParams &SQLParams, DB::TQuery &Qry);


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

    SelectedRows& setFromString (const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromInteger(const size_t idx, DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromBoolean(const size_t idx, DB::TQuery &Qry, const int qryIdx);

    SelectedRows& setFromString (DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromInteger(DB::TQuery &Qry, const int qryIdx);
    SelectedRows& setFromBoolean(DB::TQuery &Qry, const int qryIdx);

    SelectedRows& setFromString (const std::string& value);
    SelectedRows& setFromInteger(const std::optional<int>& value);
    SelectedRows& setFromBoolean(const std::optional<bool>& value);

    void addRow();
    void addRow(const bool deletedStatus, const std::optional<int> tid);

    void toXML(const xmlNodePtr dataNode) const;

    RefreshStatus status() const;
};

}

class CacheTableCallbacks
{
  public:
    virtual std::string selectSql() const =0;
    virtual std::string refreshSql() const =0;
    virtual std::string insertSql() const =0;
    virtual std::string updateSql() const =0;
    virtual std::string deleteSql() const =0;
    virtual std::string dbSessionObjectName() const =0;
    virtual void onSelectOrRefresh(const TParams& sqlParams, CacheTable::SelectedRows& rows) const =0;

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
};

class CacheTableReadonlyHandmade : public CacheTableCallbacks
{
  public:
    std::string selectSql() const { return ""; };
    std::string refreshSql() const { return ""; }
    std::string insertSql() const { return ""; }
    std::string updateSql() const { return ""; }
    std::string deleteSql() const { return ""; }
    std::string dbSessionObjectName() const { return ""; }
};


