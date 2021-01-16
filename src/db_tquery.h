#pragma once

#include "date_time.h"
#include "oralib.h"

namespace DbCpp {
    class CursCtl;
    class Session;
}//namespace DbCpp

/////////////////////////////////////////////////////////////////////////////////////////

namespace DB {

class TQuery
{
public:
    TQuery(DbCpp::Session& sess);
    ~TQuery();

    std::string SQLText;
    int Eof;

    void Close();
    void Execute();
    void Next();
    void Clear();

    void SetSession(DbCpp::Session& session);

    int FieldsCount();
    std::string FieldName(int ind);

    int GetFieldIndex(const std::string& fname);
    int FieldIndex(const std::string& fname);

    int FieldIsNULL(const std::string& fname);
    int FieldIsNULL(int ind);

    std::string FieldAsString(const std::string& fname);
    std::string FieldAsString(int ind);

    int FieldAsInteger(const std::string& fname);
    int FieldAsInteger(int ind);

    BASIC::date_time::TDateTime FieldAsDateTime(const std::string& fname);
    BASIC::date_time::TDateTime FieldAsDateTime(int ind);

    double FieldAsFloat(const std::string& fname);
    double FieldAsFloat(int ind);

    void CreateVariable(const std::string& vname, otFieldType vtype, int vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, double vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata);

    int VariablesCount();
    int GetVariableIndex(const std::string& vname);
    int VariableIndex(const std::string& vname);
    int VariableIsNULL(const std::string& vname);
    void DeleteVariable(const std::string& vname);
    void DeclareVariable(const std::string& vname, otFieldType vtype);

    void SetVariable(const std::string& vname, int vdata);
    void SetVariable(const std::string& vname, const std::string& vdata);
    void SetVariable(const std::string& vname, double vdata);
    void SetVariable(const std::string& vname, tnull vdata);

private:
    std::unique_ptr<class TQueryIfaceImpl> m_impl;
};

}//namespace DB
