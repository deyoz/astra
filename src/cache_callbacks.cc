#include "cache_callbacks.h"
#include "exceptions.h"
#include "stl_utils.h"

#include <serverlib/algo.h>

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
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

void DeclareVariablesFromParams(const std::set<string> &vars, const TParams &SQLParams, DB::TQuery &Qry,
                                const bool onlyIfParamExists)
{
  for(const string& var : vars) {
    if ( Qry.GetVariableIndex( var ) == -1 ) {
      map<std::string, TParam>::const_iterator ip = SQLParams.find( upperc(var) );
      if ( ip != SQLParams.end() )
      {
        switch( ip->second.DataType ) {
          case ctInteger:
            Qry.DeclareVariable( var, otInteger );
            LogTrace(TRACE5) << "DeclareVariable('" << var << "', otInteger)";
            break;
          case ctDouble:
            Qry.DeclareVariable( var, otFloat );
            LogTrace(TRACE5) << "DeclareVariable('" << var << "', otFloat)";
            break;
          case ctDateTime:
            Qry.DeclareVariable( var, otDate );
            LogTrace(TRACE5) << "DeclareVariable('" << var << "', otDate)";
            break;
          case ctString:
            Qry.DeclareVariable( var, otString );
            LogTrace(TRACE5) << "DeclareVariable('" << var << "', otString)";
            break;
        }
      }
      else
      {
        if (!onlyIfParamExists) {
          Qry.DeclareVariable( var, otString );
          LogTrace(TRACE5) << "DeclareVariable('" << var << "', otString)";
        }
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
      if (!ip->second.Value.empty())
      {
        Qry.SetVariable( var, ip->second.Value );
        LogTrace(TRACE5) << "SetVariable('" << var << "', '" << ip->second.Value << "')";
      }
      else
      {
        Qry.SetVariable( var, FNull );
        LogTrace(TRACE5) << "SetVariable('" << var << "', FNull)";
      }
      fieldsForLogging.set(var, ip->second.Value);
    }
    else
    {
      if (!onlyIfParamExists) {
        Qry.SetVariable( var, FNull );
        LogTrace(TRACE5) << "SetVariable('" << var << "', FNull)";
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

