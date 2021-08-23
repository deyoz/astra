#include "cache_callbacks.h"
#include "astra_locale_adv.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "PgOraConfig.h"

#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

std::ostream & operator <<(std::ostream& os, TCacheConvertType value)
{
  switch (value) {
  case ctInteger:  os << "ctInteger";  break;
  case ctDouble:   os << "ctDouble";   break;
  case ctDateTime: os << "ctDateTime"; break;
  case ctString:   os << "ctString";   break;
  }
  return os;
}

std::ostream & operator <<(std::ostream& os, const TParams& params)
{
  for (const auto& param: params) {
    const std::string& name = param.first;
    const TParam& data = param.second;
    os << std::endl << "param { name:'" << name << "', type:" << data.DataType << ", value:'" << data.Value << "' }";
  }

  return os;
}

void FieldsForLogging::set(const std::string& name, const std::string& value)
{
  fields[upperc(name)]=value;
}

std::string FieldsForLogging::get(const std::string& name) const
{
  const auto value=algo::find_opt<boost::optional>(fields, upperc(name));
  if (!value)
    throw Exception("FieldsForLogging::get: field '%s' not defined");
  return value.value();
}

void FieldsForLogging::trace() const
{
  for(const auto& f : fields)
    LogTrace(TRACE5) << f.first << " = " << f.second;
}

void DeclareVariable(const string& name, const TCacheConvertType type, DB::TQuery &Qry)
{
  switch( type ) {
    case ctInteger:
      Qry.DeclareVariable( name, otInteger );
      LogTrace(TRACE5) << "DeclareVariable('" << name << "', otInteger)";
      break;
    case ctDouble:
      Qry.DeclareVariable( name, otFloat );
      LogTrace(TRACE5) << "DeclareVariable('" << name << "', otFloat)";
      break;
    case ctDateTime:
      Qry.DeclareVariable( name, otDate );
      LogTrace(TRACE5) << "DeclareVariable('" << name << "', otDate)";
      break;
    case ctString:
      Qry.DeclareVariable( name, otString );
      LogTrace(TRACE5) << "DeclareVariable('" << name << "', otString)";
      break;
  }
}

void SetVariable(const string& name, const TCacheConvertType type, const string& value, DB::TQuery &Qry)
{
  if (value.empty())
  {
    Qry.SetVariable( name, FNull );
    LogTrace(TRACE5) << "SetVariable('" << name << "', FNull)";
    return;
  }

  switch( type ) {
    case ctInteger:
      {
        int i;
        if (StrToInt( value.c_str(), i ) == EOF)
          throw EConvertError("Cannot convert variable %s to an Integer", value.c_str());
        Qry.SetVariable( name, i );
        LogTrace(TRACE5) << "SetVariable('" << name << "', " << i << ")";
      }
      break;
    case ctDouble:
      {
        double f;
        if (StrToFloat( value.c_str(), f ) == EOF)
          throw EConvertError("Cannot convert variable %s to an Float", value.c_str());
        Qry.SetVariable( name, f );
        LogTrace(TRACE5) << "SetVariable('" << name << "', " << f << ")";
      }
      break;
    case ctDateTime:
      {
        TDateTime d;
        if (StrToDateTime( value.c_str(), d ) == EOF)
          throw EConvertError("Cannot convert variable %s to an DateTime", value.c_str());
        Qry.SetVariable( name, d );
        LogTrace(TRACE5) << "SetVariable('" << name << "', " << value << ")";
      }
      break;
    case ctString:
      Qry.SetVariable( name, value );
      LogTrace(TRACE5) << "SetVariable('" << name << "', '" << value << "')";
      break;
  }
}

void CreateVariable(const string& name, const TCacheConvertType type, const string& value, DB::TQuery &Qry)
{
  DeclareVariable(name, type, Qry);
  SetVariable(name, type, value, Qry);
}

void DeclareVariablesFromParams(const std::set<string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                                const bool onlyIfParamExists)
{
  for(const string& var : vars) {
    if ( Qry.GetVariableIndex( var ) == -1 ) {
      map<std::string, TParam>::const_iterator ip = SQLParams.find( upperc(var) );
      if ( ip != SQLParams.end() )
      {
        DeclareVariable(var, ip->second.DataType, Qry);
      }
      else
      {
        if (!onlyIfParamExists)
          DeclareVariable(var, ctString, Qry);
      }
    }
  }
}

void SetVariablesFromParams(const std::set<string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                            FieldsForLogging& fieldsForLogging, const bool onlyIfParamExists)
{
  for(const string& var : vars) {
    if ( Qry.GetVariableIndex( var ) == -1 ) continue;
    map<std::string, TParam>::const_iterator ip = SQLParams.find( upperc(var) );
    if ( ip != SQLParams.end() )
    {
      SetVariable(var, ip->second.DataType, ip->second.Value, Qry);
      fieldsForLogging.set(var, ip->second.Value);
    }
    else
    {
      if (!onlyIfParamExists) {
        SetVariable(var, ctString, "", Qry);
        fieldsForLogging.set(var, "");
      }
    }
  }
}

void CreateVariablesFromParams(const std::set<string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                               const bool onlyIfParamExists)
{
  FieldsForLogging fieldsForLogging;
  DeclareVariablesFromParams(vars, SQLParams, Qry, onlyIfParamExists);
  SetVariablesFromParams(vars, SQLParams, Qry, fieldsForLogging, onlyIfParamExists);
}

std::string getParamValue(const std::string& name, const TParams &SQLParams)
{
  map<std::string, TParam>::const_iterator ip = SQLParams.find( upperc(name) );
  if ( ip == SQLParams.end() )
    throw Exception("%s: parameter %s not found", __func__, name.c_str());
  return ip->second.Value;
}

std::string notEmptyParamAsString(const std::string& name, const TParams &SQLParams)
{
  string value=getParamValue(name, SQLParams);

  if (value.empty())
    throw EConvertError("%s: empty parameter '%s'", __func__, name.c_str());

  return value;
}

int notEmptyParamAsInteger(const std::string& name, const TParams &SQLParams)
{
  string value=getParamValue(name, SQLParams);

  int result;
  if (StrToInt( value.c_str(), result ) == EOF) //на пустые параметры тоже ругаемся
    throw EConvertError("%s: cannot convert parameter '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return result;
}

bool notEmptyParamAsBoolean(const std::string& name, const TParams &SQLParams)
{
  string value=getParamValue(name, SQLParams);

  int result;
  if (StrToInt( value.c_str(), result ) == EOF) //на пустые параметры тоже ругаемся
    throw EConvertError("%s: cannot convert parameter '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return result!=0;
}

RowId_t newRowId()
{
  return RowId_t(PgOra::getSeqNextVal_int("id__seq"));
}

int GeneratedTid::get() const
{
  if (!tid) tid=PgOra::getSeqNextVal_int("tid__seq");

  return tid.value();
}

namespace CacheTable
{

SelectedRow& SelectedRows::currentRow()
{
  if (rows.empty() || rows.back().deleted)
    rows.emplace_back(fieldIndexes_.size());

  return rows.back();
}

size_t& SelectedRows::currentFieldIdx()
{
  if (rows.empty() || rows.back().deleted) currentFieldIdx_=0;

  return currentFieldIdx_;
}

SelectedRows::SelectedRows(const std::map<std::string, size_t>& fieldIndexes,
                           const std::optional<int>& tid) :
  fieldIndexes_(fieldIndexes), tid_(tid)
{
  if (fieldIndexes_.empty())
    throw Exception("%s: empty fieldIndexes", __func__);

  for(const auto& f : fieldIndexes_)
    if (f.second>=fieldIndexes_.size())
      throw Exception("%s: wrong fieldIndexes", __func__);
}

SelectedRows& SelectedRows::setField(const std::string& name, const std::string& value)
{
  const auto fieldIdxOpt = algo::find_opt<std::optional>(fieldIndexes_, upperc(name));
  if (!fieldIdxOpt)
    throw Exception("%s: wrong field '%s'", __func__, name.c_str());

  currentRow().fields[fieldIdxOpt.value()]=value;

  return *this;
}

SelectedRows& SelectedRows::setFromString(const size_t idx, DB::TQuery &Qry, const int qryIdx)
{
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", __func__, idx);

  currentRow().fields[idx]=Qry.FieldIsNULL(qryIdx)?"":Qry.FieldAsString(qryIdx);

  return *this;
}

SelectedRows& SelectedRows::setFromInteger(const size_t idx, DB::TQuery &Qry, const int qryIdx)
{
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", __func__, idx);

  currentRow().fields[idx]=Qry.FieldIsNULL(qryIdx)?"":IntToString( Qry.FieldAsInteger(qryIdx) );

  return *this;
}

SelectedRows& SelectedRows::setFromBoolean(const size_t idx, DB::TQuery &Qry, const int qryIdx)
{
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", __func__, idx);

  currentRow().fields[idx]=Qry.FieldIsNULL(qryIdx)?"":IntToString( (int)(Qry.FieldAsInteger(qryIdx)!=0) );

  return *this;
}

SelectedRows& SelectedRows::setFromDateTime(const size_t idx, DB::TQuery &Qry, const int qryIdx)
{
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", __func__, idx);

  currentRow().fields[idx]=Qry.FieldIsNULL(qryIdx)?"":DateTimeToStr(Qry.FieldAsDateTime(qryIdx), ServerFormatDateTimeAsString);

  return *this;
}

SelectedRows& SelectedRows::setFromDouble(const size_t idx, DB::TQuery &Qry, const int qryIdx)
{
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", __func__, idx);

  currentRow().fields[idx]=Qry.FieldIsNULL(qryIdx)?"":FloatToString( Qry.FieldAsFloat(qryIdx) );

  return *this;
}

SelectedRows& SelectedRows::setFromString(DB::TQuery &Qry, const int qryIdx)
{
  return setFromString(currentFieldIdx()++, Qry, qryIdx);
}

SelectedRows& SelectedRows::setFromInteger(DB::TQuery &Qry, const int qryIdx)
{
  return setFromInteger(currentFieldIdx()++, Qry, qryIdx);
}

SelectedRows& SelectedRows::setFromBoolean(DB::TQuery &Qry, const int qryIdx)
{
  return setFromBoolean(currentFieldIdx()++, Qry, qryIdx);
}

SelectedRows& SelectedRows::setFromDateTime(DB::TQuery &Qry, const int qryIdx)
{
  return setFromDateTime(currentFieldIdx()++, Qry, qryIdx);
}

SelectedRows& SelectedRows::setFromDouble(DB::TQuery &Qry, const int qryIdx)
{
  return setFromDouble(currentFieldIdx()++, Qry, qryIdx);
}

SelectedRows& SelectedRows::setCurrentField(const std::string& value, const std::string& whence)
{
  size_t idx=currentFieldIdx()++;
  if (idx>=fieldIndexes_.size())
    throw Exception("%s: wrong index %zu", whence.c_str(), idx);

  currentRow().fields[idx]=value;

  return *this;
}

SelectedRows& SelectedRows::setFromString(const std::string& value)
{
  return setCurrentField(value, __func__);
}

SelectedRows& SelectedRows::setFromInteger(const std::optional<int>& value)
{
  return setCurrentField(value?IntToString(value.value()):"", __func__);
}

SelectedRows& SelectedRows::setFromBoolean(const std::optional<bool>& value)
{
  return setCurrentField(value?IntToString((int)(value.value())):"", __func__);
}

void SelectedRows::addRow()
{
  currentRow().deleted=false;
}

void SelectedRows::addRow(const bool deletedStatus,
                          const std::optional<int> tid)
{
  currentRow().deleted=deletedStatus;

  if (tid && (!maxSelectedTid || maxSelectedTid.value()<tid)) maxSelectedTid=tid;
}

void SelectedRows::toXML(const xmlNodePtr dataNode) const
{
  xmlNodePtr rowsNode = NewTextChild(dataNode, "rows");
  SetProp(rowsNode, "tid", maxSelectedTid?maxSelectedTid.value():-1);
  for(const SelectedRow& row : rows)
  {
    if (!row.deleted) continue;

    xmlNodePtr rowNode = NewTextChild(rowsNode, "row");
    SetProp(rowNode, "pr_del", row.deleted.value());
    for(const string& field : row.fields)
      NewTextChild(rowNode, "col", field);
  }
}

RefreshStatus SelectedRows::status() const
{
  if (algo::any_of(rows, [](const SelectedRow& row) { return row.deleted; })) // начитали изменения
    return CacheTable::RefreshStatus::Exists;
  else if (tid_) // нет изменений
    return CacheTable::RefreshStatus::None;
  else
    return CacheTable::RefreshStatus::ClearAll; // все удалили
}

Row::Row(const std::map<std::string, std::string>& fields)
{
  fields_=algo::transform( fields, [](const auto& i) { return make_pair(upperc(i.first), i.second); } );
}

std::string& Row::fieldValue(const std::string& name, const std::string& whence)
{
  const auto i=fields_.find(upperc(name));
  if (i==fields_.end())
    throw Exception("%s: wrong field '%s'", whence.c_str(), name.c_str());

  return i->second;
}

const std::string& Row::fieldValue(const std::string& name, const std::string& whence) const
{
  const auto i=fields_.find(upperc(name));
  if (i==fields_.end())
    throw Exception("%s: wrong field '%s'", whence.c_str(), name.c_str());

  return i->second;
}

Row& Row::setFromString(const std::string& name, const std::string& value)
{
  fieldValue(name, __func__)=value;

  return *this;
}

Row& Row::setFromInteger(const std::string& name, const std::optional<int>& value)
{
  fieldValue(name, __func__)=value?IntToString(value.value()):"";

  return *this;
}

Row& Row::setFromBoolean(const std::string& name, const std::optional<bool>& value)
{
  fieldValue(name, __func__)=value?IntToString((int)value.value()):"";

  return *this;
}

string Row::getAsString(const string& name) const
{
  return fieldValue(name, __func__);
}

string Row::getAsString(const string& name, const string& defaultValue) const
{
  const string value = getAsString(name);
  if (value.empty()) {
    return defaultValue;
  }
  return value;
}

string Row::getAsString_ThrowOnEmpty(const string& name) const
{
  const string value = getAsString(name);
  if (value.empty()) {
    throw Exception("%s: field '%s' value is empty", __func__, name.c_str());
  }
  return value;
}

std::optional<int> Row::getAsInteger(const std::string& name) const
{
  string value=fieldValue(name, __func__);
  if (value.empty()) return std::nullopt;

  int i;
  if (StrToInt( value.c_str(), i ) == EOF)
    throw EConvertError("%s: cannot convert field '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return i;
}

int Row::getAsInteger(const string& name, int defaultValue) const
{
  const std::optional<int> value = getAsInteger(name);
  if (!value) {
    return defaultValue;
  }
  return *value;
}

int Row::getAsInteger_ThrowOnEmpty(const string& name) const
{
  const std::optional<int> value = getAsInteger(name);
  if (!value) {
    throw Exception("%s: field '%s' value is empty", __func__, name.c_str());
  }
  return *value;
}

std::optional<bool> Row::getAsBoolean(const std::string& name) const
{
  string value=fieldValue(name, __func__);
  if (value.empty()) return std::nullopt;

  int i;
  if (StrToInt( value.c_str(), i ) == EOF)
    throw EConvertError("%s: cannot convert field '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return i!=0;
}

bool Row::getAsBoolean(const string& name, bool defaultValue) const
{
  const std::optional<bool> value = getAsBoolean(name);
  if (!value) {
    return defaultValue;
  }
  return *value;
}

bool Row::getAsBoolean_ThrowOnEmpty(const string& name) const
{
  const std::optional<bool> value = getAsBoolean(name);
  if (!value) {
    throw Exception("%s: field '%s' value is empty", __func__, name.c_str());
  }
  return *value;
}

std::optional<TDateTime> Row::getAsDateTime(const string& name) const
{
  string value=fieldValue(name, __func__);
  if (value.empty()) return std::nullopt;

  TDateTime result;
  if (StrToDateTime(value.c_str(), result) == EOF)
      throw EConvertError("%s: cannot convert field '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return result;
}

TDateTime Row::getAsDateTime(const string& name, TDateTime defaultValue) const
{
  const std::optional<TDateTime> value = getAsDateTime(name);
  if (!value) {
    return defaultValue;
  }
  return *value;
}

TDateTime Row::getAsDateTime_ThrowOnEmpty(const string& name) const
{
  const std::optional<TDateTime> value = getAsDateTime(name);
  if (!value) {
    throw Exception("%s: field '%s' value is empty", __func__, name.c_str());
  }
  return *value;
}

std::optional<double> Row::getAsDouble(const std::string& name) const
{
  string value=fieldValue(name, __func__);
  if (value.empty()) return std::nullopt;

  double d;
  if (StrToFloat( value.c_str(), d ) == EOF)
    throw EConvertError("%s: cannot convert field '%s' (value=%s)", __func__, name.c_str(), value.c_str());

  return d;
}

double Row::getAsDouble(const string& name, double defaultValue) const
{
  const std::optional<double> value = getAsDouble(name);
  if (!value) {
    return defaultValue;
  }
  return *value;
}

double Row::getAsDouble_ThrowOnEmpty(const string& name) const
{
  const std::optional<double> value = getAsDouble(name);
  if (!value) {
    throw Exception("%s: field '%s' value is empty", __func__, name.c_str());
  }
  return *value;
}

void setRowId(const std::string& fieldName,
              const TCacheUpdateStatus status,
              std::optional<CacheTable::Row>& newRow)
{
  if (status==usInserted)
    newRow.value().setFromInteger(fieldName, newRowId().get());
}

RowId_t getRowId(const std::string& fieldName,
                 const std::optional<CacheTable::Row>& oldRow,
                 const std::optional<CacheTable::Row>& newRow)
{
  return RowId_t((oldRow?oldRow.value():newRow.value()).getAsInteger_ThrowOnEmpty(fieldName));
}


} //namespace CacheTable

CacheTableCallbacks::~CacheTableCallbacks() {}

//CacheTableKeepDeletedRows

void CacheTableKeepDeletedRows::bindAndExec(const RowId_t& rowId, const CacheTable::Row& row,
                                            const bool deleted, DbCpp::CursCtl& cur) const
{
  bind(row, cur);

  cur.stb()
     .bind(":tid",      generatedTid.get())
     .bind(":pr_del",   deleted)
     .bind(":id",       rowId.get())
     .exec();
}

void CacheTableKeepDeletedRows::insert(const RowId_t& rowId, const CacheTable::Row& row,
                                       const bool deletedAfterInsert) const
{
  auto cur=make_db_curs(insertSqlOnApplyingChanges(),
                        PgOra::getRWSession(upperc(tableName())));

  bindAndExec(rowId, row, deletedAfterInsert, cur);
}

void CacheTableKeepDeletedRows::update(const RowId_t& rowId, const CacheTable::Row& row,
                                       const bool deletedBeforeUpdate) const
{
  auto cur=make_db_curs(updateSqlOnApplyingChanges() + (deletedBeforeUpdate?" AND pr_del<>0":" AND pr_del=0"),
                        PgOra::getRWSession(upperc(tableName())));

  bindAndExec(rowId, row, false, cur);
}

void CacheTableKeepDeletedRows::onApplyingRowChanges(const TCacheUpdateStatus status,
                                                     const std::optional<CacheTable::Row>& oldRow,
                                                     const std::optional<CacheTable::Row>& newRow) const
{
  std::optional<RowId_t> rowId;

  if (status==usInserted)
  {
    rowId=getRowIdBeforeInsert(newRow.value());
    if (!rowId)
    {
      rowId=newRowId();
      insert(rowId.value(), newRow.value(), false);
    }
    else
    {
      update(rowId.value(), newRow.value(), true);
    }
  }

  if (status==usModified)
  {
    rowId=getRowId(idFieldName(), oldRow, std::nullopt);
    update(rowId.value(), newRow.value(), false);
  }

  if (status==usDeleted)
  {
    rowId=getRowId(idFieldName(), oldRow, std::nullopt);

    auto cur=make_db_curs(deleteSqlOnApplyingChanges() + " AND pr_del=0",
                          PgOra::getRWSession(upperc(tableName())));
    cur.stb()
       .bind(":id", rowId.value().get())
       .exec();

    if (cur.rowcount()>0)
      insert(rowId.value(), oldRow.value(), true);
  }

  if (rowId) HistoryTable(lowerc(tableName())).synchronize(rowId.value());
}


TCacheInfo getCacheInfo(const std::string &code)
{
    TCacheInfo info;

    auto curTab = make_db_curs("SELECT title FROM cache_tables WHERE code=:code",
                               PgOra::getROSession("CACHE_TABLES"));

    if (curTab.stb().def(info.title).bind(":code", code).EXfet() == DbCpp::ResultCode::NoDataFound)
        throw AstraLocale::UserException("Cache " + code + " not found");

    auto curFld = make_db_curs("SELECT name, title FROM cache_fields WHERE code=:code",
                               PgOra::getROSession("CACHE_FIELDS"));

    std::string name;
    std::string title;

    curFld.stb()
          .autoNull()
          .def(name)
          .defNull(title, std::string())
          .bind(":code", code)
          .exec();

    while (curFld.fen() == DbCpp::ResultCode::Ok) {
        if (title.empty())
            title = name;
        info.fieldTitle.emplace(name, title);
    }

    return info;
}
