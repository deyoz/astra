#include "cache_callbacks.h"
#include "exceptions.h"
#include "stl_utils.h"

#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

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

}

CacheTableCallbacks::~CacheTableCallbacks()
{

}

