#include "db_tquery.h"
#include "hooked_session.h"
#include "exceptions.h"

#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/pg_cursctl.h>
#include <serverlib/exception.h>
#include <serverlib/str_utils.h>
#include <serverlib/algo.h>

#include <algorithm>
#include <memory>
#include <variant>
#include <map>
#include <set>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace DB {

using BASIC::date_time::TDateTime;

namespace {

bool isSelectQuery(const std::string& s)
{
    auto pos_beg = s.find_first_not_of(' ');
    if (pos_beg == std::string::npos) {
        return false;
    }
    auto pos = s.find_first_of(" (+-'\n",pos_beg);
    if (pos == std::string::npos) {
        return false;
    }
    std::string cmd = s.substr(pos_beg, pos);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    return cmd == "SELECT";
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

class TQueryIfaceImpl
{
public:
    virtual ~TQueryIfaceImpl() {}

    virtual void Close() = 0;
    virtual void Execute() = 0;
    virtual void Next() = 0;
    virtual void Clear() = 0;
    virtual int FieldsCount() = 0;
    virtual std::string FieldName(int ind) = 0;
    virtual int GetFieldIndex(const std::string& fname) = 0;
    virtual int FieldIndex(const std::string& fname) = 0;
    virtual int FieldIsNULL(const std::string& fname) = 0;
    virtual int FieldIsNULL(int ind) = 0;
    virtual std::string FieldAsString(const std::string& fname) = 0;
    virtual std::string FieldAsString(int ind) = 0;
    virtual int FieldAsInteger(const std::string& fname) = 0;
    virtual int FieldAsInteger(int ind) = 0;
    virtual TDateTime FieldAsDateTime(const std::string& fname) = 0;
    virtual TDateTime FieldAsDateTime(int ind) = 0;
    virtual double FieldAsFloat(const std::string& fname) = 0;
    virtual double FieldAsFloat(int ind) = 0;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, int vdata) = 0;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata) = 0;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, double vdata) = 0;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata) = 0;
    virtual int VariablesCount() = 0;
    virtual int GetVariableIndex(const std::string& vname) = 0;
    virtual int VariableIndex(const std::string& vname) = 0;
    virtual int VariableIsNULL(const std::string& vname) = 0;
    virtual void DeleteVariable(const std::string& vname) = 0;
    virtual void DeclareVariable(const std::string& vname, otFieldType vtype) = 0;
    virtual void SetVariable(const std::string& vname, int vdata) = 0;
    virtual void SetVariable(const std::string& vname, const std::string& vdata) = 0;
    virtual void SetVariable(const std::string& vname, double vdata) = 0;
    virtual void SetVariable(const std::string& vname, tnull vdata) = 0;
};

//---------------------------------------------------------------------------------------

class TQueryIfaceDbCppImpl final: public TQueryIfaceImpl
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
    std::string &m_sqlText;
    int &m_eof;
    std::map<std::string, Variable> m_variables;

public:
    TQueryIfaceDbCppImpl(DbCpp::Session& sess, std::string& sqlText, int& eof);

    virtual void Close() override;
    virtual void Execute() override;
    virtual void Next() override;
    virtual void Clear() override;
    virtual int FieldsCount() override;
    virtual std::string FieldName(int ind) override;
    virtual int GetFieldIndex(const std::string& fname) override;
    virtual int FieldIndex(const std::string& fname) override;
    virtual int FieldIsNULL(const std::string& fname) override;
    virtual int FieldIsNULL(int ind) override;
    virtual std::string FieldAsString(const std::string& fname) override;
    virtual std::string FieldAsString(int ind) override;
    virtual int FieldAsInteger(const std::string& fname) override;
    virtual int FieldAsInteger(int ind) override;
    virtual TDateTime FieldAsDateTime(const std::string& fname) override;
    virtual TDateTime FieldAsDateTime(int ind) override;
    virtual double FieldAsFloat(const std::string& fname) override;
    virtual double FieldAsFloat(int ind) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, int vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, double vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata) override;
    virtual int VariablesCount() override;
    virtual int GetVariableIndex(const std::string& vname) override;
    virtual int VariableIndex(const std::string& vname) override;
    virtual int VariableIsNULL(const std::string& vname) override;
    virtual void DeleteVariable(const std::string& vname) override;
    virtual void DeclareVariable(const std::string& vname, otFieldType vtype) override;
    virtual void SetVariable(const std::string& vname, int vdata) override;
    virtual void SetVariable(const std::string& vname, const std::string& vdata) override;
    virtual void SetVariable(const std::string& vname, double vdata) override;
    virtual void SetVariable(const std::string& vname, tnull vdata) override;

protected:
    void initInnerCursCtl();
    void bindVariables();
    void checkVariable4Set(const std::string& vname);

    static std::string fieldValueAsString(const char* fname, const std::string& val);
    static int         fieldValueAsInteger(const char* fname, const std::string& val);
    static TDateTime   fieldValueAsDateTime(const char* fname, const std::string& val);
    static double      fieldValueAsFloat(const char* fname, const std::string& val);
};

//

TQueryIfaceDbCppImpl::TQueryIfaceDbCppImpl(DbCpp::Session& sess,
                                           std::string& sqlText,
                                           int& eof)
    : m_sess(sess),
      m_sqlText(sqlText),
      m_eof(eof)
{
}

void TQueryIfaceDbCppImpl::Close()
{

}

void TQueryIfaceDbCppImpl::Execute()
{
    initInnerCursCtl();
    bindVariables();
    m_cur->exec();

    if(isSelectQuery(m_sqlText)) {
        Next();
    }
}

void TQueryIfaceDbCppImpl::Next()
{
    if(m_eof) {
        tst();
        return;
    }

    ASSERT(m_cur);
    if(m_cur->nefen() == DbCpp::ResultCode::NoDataFound) {
        m_eof = 1;
    }
}

void TQueryIfaceDbCppImpl::Clear()
{
    m_variables.clear();
    m_sqlText.clear();
    m_cur.reset();
    m_eof = 0;
}

int TQueryIfaceDbCppImpl::FieldsCount()
{
    ASSERT(m_cur);
    return m_cur->fieldsCount();
}

std::string TQueryIfaceDbCppImpl::FieldName(int ind)
{
    ASSERT(m_cur);
    return m_cur->fieldName(ind);
}

int TQueryIfaceDbCppImpl::GetFieldIndex(const std::string& fname)
{
    ASSERT(m_cur);
    return m_cur->fieldIndex(fname);
}

int TQueryIfaceDbCppImpl::FieldIndex(const std::string& fname)
{
    int fieldIndex = GetFieldIndex(fname);
    if(fieldIndex == -1)
    {
        char serror[ 100 ];
        sprintf(serror, "Field %s does not exists", fname.c_str());
        throw EOracleError(serror, 0, m_sqlText.c_str());
    }
    return fieldIndex;
}

int TQueryIfaceDbCppImpl::FieldIsNULL(const std::string& fname)
{
    ASSERT(m_cur);
    return static_cast<int>(m_cur->fieldIsNull(fname));
}

int TQueryIfaceDbCppImpl::FieldIsNULL(int ind)
{
    ASSERT(m_cur);
    return static_cast<int>(m_cur->fieldIsNull(ind));
}

std::string TQueryIfaceDbCppImpl::FieldAsString(const std::string& fname)
{
    ASSERT(m_cur);
    const std::string DefVal = "";
    if(m_cur->fieldIsNull(fname)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(fname);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsString(fname.c_str(), val);
}

std::string TQueryIfaceDbCppImpl::FieldAsString(int ind)
{
    ASSERT(m_cur);
    const std::string DefVal = "";
    if(m_cur->fieldIsNull(ind)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(ind);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsString(std::to_string(ind).c_str(), val);
}

int TQueryIfaceDbCppImpl::FieldAsInteger(const std::string& fname)
{
    ASSERT(m_cur);
    const int DefVal = 0;
    if(m_cur->fieldIsNull(fname)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(fname);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsInteger(fname.c_str(), val);
}

int TQueryIfaceDbCppImpl::FieldAsInteger(int ind)
{
    ASSERT(m_cur);
    const int DefVal = 0;
    if(m_cur->fieldIsNull(ind)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(ind);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsInteger(std::to_string(ind).c_str(), val);
}

TDateTime TQueryIfaceDbCppImpl::FieldAsDateTime(const std::string& fname)
{
    ASSERT(m_cur);
    const TDateTime DefVal = 0.0;
    if(m_cur->fieldIsNull(fname)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(fname);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsDateTime(fname.c_str(), val);
}

TDateTime TQueryIfaceDbCppImpl::FieldAsDateTime(int ind)
{
    ASSERT(m_cur);
    const TDateTime DefVal = 0.0;
    if(m_cur->fieldIsNull(ind)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(ind);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsDateTime(std::to_string(ind).c_str(), val);
}

double TQueryIfaceDbCppImpl::FieldAsFloat(const std::string& fname)
{
    ASSERT(m_cur);
    const TDateTime DefVal = 0.0;
    if(m_cur->fieldIsNull(fname)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(fname);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsFloat(fname.c_str(), val);
}

double TQueryIfaceDbCppImpl::FieldAsFloat(int ind)
{
    ASSERT(m_cur);
    const TDateTime DefVal = 0.0;
    if(m_cur->fieldIsNull(ind)) {
        return DefVal;
    }

    std::string val = m_cur->fieldValue(ind);
    if(val.empty()) {
        return DefVal;
    }

    return fieldValueAsFloat(std::to_string(ind).c_str(), val);
}

void TQueryIfaceDbCppImpl::CreateVariable(const std::string& vname, otFieldType vtype, int vdata)
{
    m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
}

void TQueryIfaceDbCppImpl::CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata)
{
    m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
}

void TQueryIfaceDbCppImpl::CreateVariable(const std::string& vname, otFieldType vtype, double vdata)
{
    if(vtype == otDate) {
        m_variables.emplace(vname, Variable{BASIC::date_time::DateTimeToBoost(vdata), false/*IsNull*/});
    } else {
        m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
    }
}

void TQueryIfaceDbCppImpl::CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata)
{
    if(vtype == otInteger) {
        m_variables.emplace(vname, Variable{0, true/*IsNull*/});
    } else if(vtype == otFloat) {
        m_variables.emplace(vname, Variable{0.0, true/*IsNull*/});
    } else if(vtype == otString) {
        m_variables.emplace(vname, Variable{"", true/*IsNull*/});
    } else if(vtype == otDate) {
        m_variables.emplace(vname, Variable{boost::posix_time::second_clock::local_time(), true/*IsNull*/});
    } else {
        LogError(STDLOG) << "Unsupported null variable type " << vtype << " for variable " << vname;
    }
}

int TQueryIfaceDbCppImpl::VariablesCount()
{
    return m_variables.size();
}

// ���⨢�� ��⮤. �� ��� ࠡ�⠥� ��� exists
// ��୥� 1, �᫨ ��६����� �������,
//       -1, �᫨ ����
int TQueryIfaceDbCppImpl::GetVariableIndex(const std::string& vname)
{
    return algo::find_opt<std::optional>(m_variables, vname) ? 1 : -1;
}

int TQueryIfaceDbCppImpl::VariableIndex(const std::string& vname)
{
    int ind = GetVariableIndex(vname);
    if(ind == -1) {
        char serror[ 100 ];
        sprintf(serror, "Variable %s does not exists", vname.c_str());
        throw EOracleError(serror, 0, m_sqlText.c_str());
    }

    return ind;
}

int TQueryIfaceDbCppImpl::VariableIsNULL(const std::string& vname)
{
    if(vname.empty()) {
        throw EOracleError( "You cannot get value to variable with an empty name", 1 );
    }

    auto optVar = algo::find_opt<std::optional>(m_variables, vname);
    if(!optVar) {
        char serror[ 100 ];
        sprintf(serror, "Variable %s does not exists", vname.c_str());
        throw EOracleError(serror, 0, m_sqlText.c_str());
    }

    return static_cast<int>(optVar->IsNull);
}

void TQueryIfaceDbCppImpl::DeleteVariable(const std::string& vname)
{
    if(vname.empty()) {
        throw EOracleError("You cannot delete a variable with an empty name", 1);
    }

    if(VariableIndex(vname) >= 0) {
        m_variables.erase(vname);
    }
}

void TQueryIfaceDbCppImpl::DeclareVariable(const std::string& vname, otFieldType vtype)
{
    if(vname.empty()) {
        throw EOracleError("You cannot declare a variable with an empty name", 1);
    }

    if(GetVariableIndex(vname) >= 0) {
        m_variables.erase(vname);
    }

    CreateVariable(vname, vtype, FNull);
}

void TQueryIfaceDbCppImpl::SetVariable(const std::string& vname, int vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otInteger, vdata);
}

void TQueryIfaceDbCppImpl::SetVariable(const std::string& vname, const std::string& vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otString, vdata);
}

void TQueryIfaceDbCppImpl::SetVariable(const std::string& vname, double vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otFloat, vdata);
}

void TQueryIfaceDbCppImpl::SetVariable(const std::string& vname, tnull vdata)
{
    checkVariable4Set(vname);
    // nothing to do - already have null
}

void TQueryIfaceDbCppImpl::initInnerCursCtl()
{
    if(!m_cur) {
        m_cur = std::make_shared<DbCpp::CursCtl>(std::move(make_db_curs(m_sqlText, m_sess)));
    }
}

void TQueryIfaceDbCppImpl::bindVariables()
{
    ASSERT(m_cur);
    m_cur->stb();
    short null = -1, nnull = 0;
    for(auto& [name, var] : m_variables) {
        std::visit([&](auto &v) {
            m_cur->bind(":" + name, v, var.IsNull ? &null: &nnull);
        },
                   var.Value);
    }
}

void TQueryIfaceDbCppImpl::checkVariable4Set(const std::string& vname)
{
    if(vname.empty()) {
       throw EOracleError("You cannot set value to variable with an empty name", 1);
    }

    if(GetVariableIndex(vname) < 0) {
        throw EOracleError("Field index out of range", 0);
    }
}

std::string TQueryIfaceDbCppImpl::fieldValueAsString(const char* fname, const std::string& val)
{
    std::string value;
    if(!PgCpp::details::PgTraits<std::string>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a String", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

int TQueryIfaceDbCppImpl::fieldValueAsInteger(const char* fname, const std::string& val)
{
    int value = 0;
    if(!PgCpp::details::PgTraits<int>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to an Integer", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

TDateTime TQueryIfaceDbCppImpl::fieldValueAsDateTime(const char* fname, const std::string& val)
{
    boost::posix_time::ptime value;
    if(!PgCpp::details::PgTraits<boost::posix_time::ptime>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a DateTime", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return BASIC::date_time::BoostToDateTime(value);
}

double TQueryIfaceDbCppImpl::fieldValueAsFloat(const char* fname, const std::string& val)
{
    double value = 0.0;
    if(!PgCpp::details::PgTraits<double>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a Float", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

//---------------------------------------------------------------------------------------

class TQueryIfaceNativeImpl final: public TQueryIfaceImpl
{
public:
    TQueryIfaceNativeImpl(std::string& sqlText, int& eof);

    virtual void Close() override;
    virtual void Execute() override;
    virtual void Next() override;
    virtual void Clear() override;
    virtual int FieldsCount() override;
    virtual std::string FieldName(int ind) override;
    virtual int GetFieldIndex(const std::string& fname) override;
    virtual int FieldIndex(const std::string& fname) override;
    virtual int FieldIsNULL(const std::string& fname) override;
    virtual int FieldIsNULL(int ind) override;
    virtual std::string FieldAsString(const std::string& fname) override;
    virtual std::string FieldAsString(int ind) override;
    virtual int FieldAsInteger(const std::string& fname) override;
    virtual int FieldAsInteger(int ind) override;
    virtual TDateTime FieldAsDateTime(const std::string& fname) override;
    virtual TDateTime FieldAsDateTime(int ind) override;
    virtual double FieldAsFloat(const std::string& fname) override;
    virtual double FieldAsFloat(int ind) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, int vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, double vdata) override;
    virtual void CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata) override;
    virtual int VariablesCount() override;
    virtual int GetVariableIndex(const std::string& vname) override;
    virtual int VariableIndex(const std::string& vname) override;
    virtual int VariableIsNULL(const std::string& vname) override;
    virtual void DeleteVariable(const std::string& vname) override;
    virtual void DeclareVariable(const std::string& vname, otFieldType vtype) override;
    virtual void SetVariable(const std::string& vname, int vdata) override;
    virtual void SetVariable(const std::string& vname, const std::string& vdata) override;
    virtual void SetVariable(const std::string& vname, double vdata) override;
    virtual void SetVariable(const std::string& vname, tnull vdata) override;

protected:
    void beforeNativeCall();
    void afterNativeCall();

private:
    std::shared_ptr<::TQuery> m_qry;
    std::string &m_sqlText;
    int &m_eof;
};

//

#define __NATIVE_CALL_WITHOUT_ARGS__(method) \
    beforeNativeCall(); \
    m_qry->method(); \
    afterNativeCall();

#define __RETURN_NATIVE_CALL_WITHOUT_ARGS__(method) \
    beforeNativeCall(); \
    auto ret = m_qry->method(); \
    afterNativeCall(); \
    return ret;

#define __NATIVE_CALL_WITH_1_ARG__(method, arg) \
    beforeNativeCall(); \
    m_qry->method(arg); \
    afterNativeCall();

#define __RETURN_NATIVE_CALL_WITH_1_ARG__(method, arg) \
    beforeNativeCall(); \
    auto ret = m_qry->method(arg); \
    afterNativeCall(); \
    return ret;

#define __NATIVE_CALL_WITH_2_ARGS__(method, arg1, arg2) \
    beforeNativeCall(); \
    m_qry->method(arg1, arg2); \
    afterNativeCall();

#define __NATIVE_CALL_WITH_3_ARGS__(method, arg1, arg2, arg3) \
    beforeNativeCall(); \
    m_qry->method(arg1, arg2, arg3); \
    afterNativeCall();

//

TQueryIfaceNativeImpl::TQueryIfaceNativeImpl(std::string& sqlText, int& eof)
    : m_sqlText(sqlText),
      m_eof(eof)
{
    m_qry.reset(new ::TQuery(&OraSession));
}

void TQueryIfaceNativeImpl::Close()
{
    __NATIVE_CALL_WITHOUT_ARGS__(Close);
}

void TQueryIfaceNativeImpl::Execute()
{
    __NATIVE_CALL_WITHOUT_ARGS__(Execute);
}

void TQueryIfaceNativeImpl::Next()
{
    __NATIVE_CALL_WITHOUT_ARGS__(Next);
}

void TQueryIfaceNativeImpl::Clear()
{
    __NATIVE_CALL_WITHOUT_ARGS__(Clear);
}

int TQueryIfaceNativeImpl::FieldsCount()
{
    __RETURN_NATIVE_CALL_WITHOUT_ARGS__(FieldsCount);
}

std::string TQueryIfaceNativeImpl::FieldName(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldName, ind);
}

int TQueryIfaceNativeImpl::GetFieldIndex(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(GetFieldIndex, fname);
}

int TQueryIfaceNativeImpl::FieldIndex(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldIndex, fname);
}

int TQueryIfaceNativeImpl::FieldIsNULL(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldIsNULL, fname);
}

int TQueryIfaceNativeImpl::FieldIsNULL(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldIsNULL, ind);
}

std::string TQueryIfaceNativeImpl::FieldAsString(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsString, fname);
}

std::string TQueryIfaceNativeImpl::FieldAsString(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsString, ind);
}

int TQueryIfaceNativeImpl::FieldAsInteger(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsInteger, fname);
}

int TQueryIfaceNativeImpl::FieldAsInteger(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsInteger, ind);
}

TDateTime TQueryIfaceNativeImpl::FieldAsDateTime(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsDateTime, fname);
}

TDateTime TQueryIfaceNativeImpl::FieldAsDateTime(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsDateTime, ind);
}

double TQueryIfaceNativeImpl::FieldAsFloat(const std::string& fname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsFloat, fname);
}

double TQueryIfaceNativeImpl::FieldAsFloat(int ind)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(FieldAsFloat, ind);
}

void TQueryIfaceNativeImpl::CreateVariable(const std::string& vname, otFieldType vtype, int vdata)
{
    __NATIVE_CALL_WITH_3_ARGS__(CreateVariable, vname, vtype, vdata);
}

void TQueryIfaceNativeImpl::CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata)
{
    __NATIVE_CALL_WITH_3_ARGS__(CreateVariable, vname, vtype, vdata);
}

void TQueryIfaceNativeImpl::CreateVariable(const std::string& vname, otFieldType vtype, double vdata)
{
    __NATIVE_CALL_WITH_3_ARGS__(CreateVariable, vname, vtype, vdata);
}

void TQueryIfaceNativeImpl::CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata)
{
    __NATIVE_CALL_WITH_3_ARGS__(CreateVariable, vname, vtype, vdata);
}

int TQueryIfaceNativeImpl::VariablesCount()
{
    __RETURN_NATIVE_CALL_WITHOUT_ARGS__(VariablesCount);
}

int TQueryIfaceNativeImpl::GetVariableIndex(const std::string& vname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(GetVariableIndex, vname);
}

int TQueryIfaceNativeImpl::VariableIndex(const std::string& vname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(VariableIndex, vname);
}

int TQueryIfaceNativeImpl::VariableIsNULL(const std::string& vname)
{
    __RETURN_NATIVE_CALL_WITH_1_ARG__(VariableIsNULL, vname);
}

void TQueryIfaceNativeImpl::DeleteVariable(const std::string& vname)
{
    __NATIVE_CALL_WITH_1_ARG__(DeleteVariable, vname);
}

void TQueryIfaceNativeImpl::DeclareVariable(const std::string& vname, otFieldType vtype)
{
    __NATIVE_CALL_WITH_2_ARGS__(DeclareVariable, vname, vtype);
}

void TQueryIfaceNativeImpl::SetVariable(const std::string& vname, int vdata)
{
    __NATIVE_CALL_WITH_2_ARGS__(SetVariable, vname, vdata);
}

void TQueryIfaceNativeImpl::SetVariable(const std::string& vname, const std::string& vdata)
{
    __NATIVE_CALL_WITH_2_ARGS__(SetVariable, vname, vdata);
}

void TQueryIfaceNativeImpl::SetVariable(const std::string& vname, double vdata)
{
    __NATIVE_CALL_WITH_2_ARGS__(SetVariable, vname, vdata);
}

void TQueryIfaceNativeImpl::SetVariable(const std::string& vname, tnull vdata)
{
    __NATIVE_CALL_WITH_2_ARGS__(SetVariable, vname, vdata);
}

void TQueryIfaceNativeImpl::beforeNativeCall()
{
    ASSERT(m_qry);
    m_qry->SQLText = m_sqlText;
}

void TQueryIfaceNativeImpl::afterNativeCall()
{
    ASSERT(m_qry);
    m_eof = m_qry->Eof;
}

//

#undef __NATIVE_CALL_WITHOUT_ARGS__
#undef __RETURN_NATIVE_CALL_WITHOUT_ARGS__
#undef __NATIVE_CALL_WITH_1_ARG__
#undef __RETURN_NATIVE_CALL_WITH_1_ARG__
#undef __NATIVE_CALL_WITH_2_ARGS__
#undef __NATIVE_CALL_WITH_3_ARGS__

//---------------------------------------------------------------------------------------

TQuery::TQuery(DbCpp::Session& sess)
      : Eof(0)
{
    if(sess.getType() == DbCpp::DbType::Oracle) {
        m_impl.reset(new TQueryIfaceNativeImpl(SQLText, Eof));
    } else {
        m_impl.reset(new TQueryIfaceDbCppImpl(sess, SQLText, Eof));
    }
}

TQuery::~TQuery()
{
}

void TQuery::Close() { m_impl->Close(); }
void TQuery::Execute() { m_impl->Execute(); }
void TQuery::Next() { m_impl->Next(); }
void TQuery::Clear() { m_impl->Clear(); }

int TQuery::FieldsCount() { return m_impl->FieldsCount(); }
std::string TQuery::FieldName(int ind) { return m_impl->FieldName(ind); }

int TQuery::GetFieldIndex(const std::string& fname) { return m_impl->GetFieldIndex(fname); }
int TQuery::FieldIndex(const std::string& fname) { return m_impl->FieldIndex(fname); }

int TQuery::FieldIsNULL(const std::string& fname) { return m_impl->FieldIsNULL(fname); }
int TQuery::FieldIsNULL(int ind) { return m_impl->FieldIsNULL(ind); }

std::string TQuery::FieldAsString(const std::string& fname) { return m_impl->FieldAsString(fname); }
std::string TQuery::FieldAsString(int ind) { return m_impl->FieldAsString(ind); }

int TQuery::FieldAsInteger(const std::string& fname) { return m_impl->FieldAsInteger(fname); }
int TQuery::FieldAsInteger(int ind) { return m_impl->FieldAsInteger(ind); }

TDateTime TQuery::FieldAsDateTime(const std::string& fname) { return m_impl->FieldAsDateTime(fname); }
TDateTime TQuery::FieldAsDateTime(int ind) { return m_impl->FieldAsDateTime(ind); }

double TQuery::FieldAsFloat(const std::string& fname) { return m_impl->FieldAsFloat(fname); }
double TQuery::FieldAsFloat(int ind) { return m_impl->FieldAsFloat(ind); }

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, int vdata) {
    m_impl->CreateVariable(vname, vtype, vdata);
}

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, const std::string& vdata) {
    m_impl->CreateVariable(vname, vtype, vdata);
}

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, double vdata) {
    m_impl->CreateVariable(vname, vtype, vdata);
}

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, tnull vdata) {
    m_impl->CreateVariable(vname, vtype, vdata);
}

int TQuery::VariablesCount() { return m_impl->VariablesCount(); }
int TQuery::GetVariableIndex(const std::string& vname) { return m_impl->GetVariableIndex(vname); }
int TQuery::VariableIndex(const std::string& vname) { return m_impl->VariableIndex(vname); }
int TQuery::VariableIsNULL(const std::string& vname) { return m_impl->VariableIsNULL(vname); }
void TQuery::DeleteVariable(const std::string& vname) { m_impl->DeleteVariable(vname); }
void TQuery::DeclareVariable(const std::string& vname, otFieldType vtype) { m_impl->DeclareVariable(vname, vtype); }

void TQuery::SetVariable(const std::string& vname, int vdata) { m_impl->SetVariable(vname, vdata); }
void TQuery::SetVariable(const std::string& vname, const std::string& vdata) { m_impl->SetVariable(vname, vdata); }
void TQuery::SetVariable(const std::string& vname, double vdata) { m_impl->SetVariable(vname, vdata); }
void TQuery::SetVariable(const std::string& vname, tnull vdata) { m_impl->SetVariable(vname, vdata); }

}//namespace DB

/////////////////////////////////////////////////////////////////////////////////////////

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "xp_testing.h"
#include "pg_session.h"

#include <serverlib/cursctl.h>

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace xp_testing;

START_TEST(pg_tquery_common_usage)
{
    int intval = 20;
    boost::posix_time::ptime timeval = boost::posix_time::second_clock::local_time();
    TDateTime timevaldt = BASIC::date_time::BoostToDateTime(timeval);
    std::string strval = "hello";
    double floatval = 3.14159;

    get_pg_curs_autocommit("drop table if exists TEST_TQUERY").exec();
    get_pg_curs_autocommit("create table TEST_TQUERY(FLD1 integer, FLD2 timestamp, FLD3 varchar(20), FLD4 integer);").exec();

    DB::TQuery Qry(*get_main_pg_rw_sess(STDLOG));
    Qry.SQLText = "insert into TEST_TQUERY(FLD1, FLD2 , FLD3, FLD4) values (:fld1, :fld2, :fld3, :fld4)";
    Qry.DeclareVariable("fld1", otInteger);
    Qry.SetVariable("fld1", intval);
    Qry.CreateVariable("fld2", otDate,    timevaldt);
    Qry.CreateVariable("fld3", otString,  strval);
    Qry.CreateVariable("fld4", otInteger, FNull);

    fail_unless(Qry.VariableIsNULL("fld2") == 0, "VariableIsNULL failed");
    fail_unless(Qry.VariableIsNULL("fld4") == 1, "VariableIsNULL failed");

    fail_unless(Qry.GetVariableIndex("fld1") == 1,  "VariableIndex failed");
    fail_unless(Qry.GetVariableIndex("fld4") == 1,  "VariableIndex failed");
    fail_unless(Qry.GetVariableIndex("fld9") == -1, "VariableIndex failed");

    Qry.Execute();

    Qry.Clear();

    Qry.SQLText = "select FLD1, FLD2, FLD3, FLD4, 3.14159 as FLD5 from TEST_TQUERY where FLD1=:fld1 and FLD2=:fld2 and FLD3=:fld3";

    Qry.CreateVariable("fld1", otInteger, intval);
    Qry.CreateVariable("fld2", otDate,    timevaldt);
    Qry.CreateVariable("fld3", otString,  strval);

    fail_unless(Qry.GetVariableIndex("fld3") == 1,  "VariableIndex failed");
    Qry.DeleteVariable("fld3");
    fail_unless(Qry.GetVariableIndex("fld3") == -1,  "DeleteVariable failed");
    Qry.CreateVariable("fld3", otString,  strval);
    fail_unless(Qry.GetVariableIndex("fld3") == 1,  "VariableIndex failed");


    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        LogTrace(TRACE3) << "Value of field FLD1=" << Qry.FieldAsInteger("FLD1");
        LogTrace(TRACE3) << "Value of field FLD2=" << Qry.FieldAsDateTime("fLd2");
        LogTrace(TRACE3) << "Value of field FLD3=" << Qry.FieldAsString("fld3");

        fail_unless(!Qry.FieldIsNULL("FLD1"), "FieldIsNULL failed");
        fail_unless(!Qry.FieldIsNULL(1),      "FieldIsNULL failed");
        fail_unless(!Qry.FieldIsNULL("FLD3"), "FieldIsNULL failed");
        fail_unless(Qry.FieldIsNULL("FLD4"),  "FieldIsNULL failed");
        fail_unless(Qry.FieldIsNULL(3),       "FieldIsNULL failed");
        fail_unless(!Qry.FieldIsNULL("FLD5"), "FieldIsNULL failed");

        fail_unless(Qry.FieldAsInteger("FLD1") == intval,     "FieldAsInteger failed");
        fail_unless(Qry.FieldAsDateTime("FLD2") == timevaldt, "FieldAsDateTime failed");
        fail_unless(Qry.FieldAsString("FLD3") == strval,      "FieldAsString failed");
        fail_unless(Qry.FieldAsInteger("FLD4") == 0,          "FieldAsInteger failed");
        fail_unless(Qry.FieldAsFloat("FLD5") == floatval,     "FieldAsFloat failed");

        fail_unless(Qry.FieldAsInteger(0) == intval,     "FieldAsInteger failed");
        fail_unless(Qry.FieldAsDateTime(1) == timevaldt, "FieldAsDateTime failed");
        fail_unless(Qry.FieldAsString(2) == strval,      "FieldAsString failed");
        fail_unless(Qry.FieldAsInteger(3) == 0,          "FieldAsInteger failed");
        fail_unless(Qry.FieldAsFloat(4) == floatval,     "FieldAsFloat failed");

        fail_unless(Qry.FieldIndex("FLD2") == 1, "FieldIndex failed!");
        fail_unless(Qry.GetFieldIndex("FLD9") == -1, "GetFieldIndex failed!");

        fail_unless(Qry.FieldsCount() == 5,    "FieldsCount failed");
        fail_unless(Qry.VariablesCount() == 3, "VariablesCount failed");

        fail_unless(Qry.FieldName(0) == "FLD1", "FieldName failed");
        fail_unless(Qry.FieldName(4) == "FLD5", "FieldName failed");
    }
}
END_TEST;


START_TEST(check_ora_sessions)
{
    // create table
    make_curs("drop table TEST_ORA_SESSIONS")
            .noThrowError(CERR_TABLE_NOT_EXISTS)
            .exec();
    make_curs("create table TEST_ORA_SESSIONS(ID number)")
            .exec();

    int id = 0, tmpId = 0;

    // insert via OciCpp::CursCtl
    make_curs("insert into TEST_ORA_SESSIONS(ID) values (:id)")
            .bind(":id", ++id)
            .exec();

    // insert via DbCpp::CursCtl
    make_db_curs("insert into TEST_ORA_SESSIONS(ID) values (:id)", *get_main_ora_sess(STDLOG))
            .bind(":id", ++id)
            .exec();

    // insert via TQuery
    TQuery InsQry(&OraSession);
    InsQry.SQLText = "insert into TEST_ORA_SESSIONS(ID) values (:id)";
    InsQry.CreateVariable("id", otInteger, ++id);
    InsQry.Execute();

    std::vector<int> ids1, ids2, ids3;

    // select via OciCpp::CursCtl
    auto ocicur = make_curs("select ID from TEST_ORA_SESSIONS");
    ocicur
            .def(tmpId)
            .exec();
    while(!ocicur.fen()) {
        LogTrace(TRACE3) << "id=" << tmpId << " was read from ocicur";
        ids1.emplace_back(tmpId);
    }

    // select via DbCpp::CursCtl
    auto dbcur = make_db_curs("select ID from TEST_ORA_SESSIONS", *get_main_ora_sess(STDLOG));
    dbcur
            .def(tmpId)
            .exec();
    while(dbcur.fen() == DbCpp::ResultCode::Ok) {
        LogTrace(TRACE3) << "id=" << tmpId << " was read from dbcur";
        ids2.emplace_back(tmpId);
    }

    // select via TQuery
    TQuery Qry(&OraSession);
    Qry.SQLText = "select ID from TEST_ORA_SESSIONS";
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        tmpId = Qry.FieldAsInteger("ID");
        LogTrace(TRACE3) << "id=" << tmpId << " was read from TQuery";
        ids3.emplace_back(tmpId);
    }

    fail_unless(ids1 == ids2);
    fail_unless(ids1 == ids3);
}
END_TEST;


#define SUITENAME "pg_tquery"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(pg_tquery_common_usage);
}
TCASEFINISH;
#undef SUITENAME

#define SUITENAME "ora_sessions"
TCASEREGISTER(testInitDB, 0)
{
    ADD_TEST(check_ora_sessions);
}
TCASEFINISH;
#undef SUITENAME

#endif /*XP_TESTING*/
