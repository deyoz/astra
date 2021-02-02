#pragma once

#include "date_time.h"
#include "oralib.h"//!

#include <memory>
#include <variant>
#include <map>
#include <set>

namespace DbCpp {
    class CursCtl;
    class Session;
}//namespace DbCpp

using BASIC::date_time::TDateTime;

/////////////////////////////////////////////////////////////////////////////////////////

namespace PG {

class TQuery
{
private:
    using BindVariant = std::variant<int, double, std::string, boost::posix_time::ptime>;

    struct Variable
    {
        BindVariant Value;
        bool        IsNull;
    };

    std::shared_ptr<DbCpp::CursCtl> m_cur;
    DbCpp::Session &m_sess;
    std::map<std::string, Variable> m_variables;

public:
    TQuery();
    TQuery(DbCpp::Session& session);
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

    int FieldIsNull(const std::string& fname);
    int FieldIsNull(int ind);

    std::string FieldAsString(const std::string& fname);
    std::string FieldAsString(int ind);

    int FieldAsInteger(const std::string& fname);
    int FieldAsInteger(int ind);

    TDateTime FieldAsDateTime(const std::string& fname);
    TDateTime FieldAsDateTime(int ind);

    double FieldAsFloat(const std::string& fname);
    double FieldAsFloat(int ind);

    void CreateVariable(const std::string& vname, otFieldType vtype, int vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, double vdata);
    void CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata);

    int VariablesCount();
    int GetVariableIndex(const std::string& vname);
    int VariableIndex(const std::string& vname);
    int VariableIsNull(const std::string& vname);
    void DeleteVariable(const std::string& vname);
    void DeclareVariable(const std::string& vname, otFieldType vtype);

    void SetVariable(const std::string& vname, int vdata);
    void SetVariable(const std::string& vname, const std::string& vdata);
    void SetVariable(const std::string& vname, double vdata);
    void SetVariable(const std::string& vname, tnull vdata);

protected:
    void initInnerCursCtl();
    void bindVariables();
    void checkVariable4Set(const std::string& vname);

    static std::string fieldValueAsString(const char* fname, const std::string& val);
    static int         fieldValueAsInteger(const char* fname, const std::string& val);
    static TDateTime   fieldValueAsDateTime(const char* fname, const std::string& val);
    static double      fieldValueAsFloat(const char* fname, const std::string& val);
};

}//namespace PG
