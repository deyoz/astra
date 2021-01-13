#include "pg_tquery.h"
#include "hooked_session.h"
#include "exceptions.h"

#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/pg_cursctl.h>
#include <serverlib/exception.h>
#include <serverlib/str_utils.h>
#include <serverlib/algo.h>

#include <algorithm>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace PG {

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


TQuery::TQuery()
    : m_sess(*get_main_pg_rw_sess(STDLOG)),
      Eof(0)
{
}

TQuery::TQuery(DbCpp::Session& sess)
    : m_sess(sess),
      Eof(0)
{
}

TQuery::~TQuery()
{
}

void TQuery::SetSession(DbCpp::Session& session)
{
    m_sess = session;
}

void TQuery::initInnerCursCtl()
{
    if(!m_cur) {
        m_cur = std::make_shared<DbCpp::CursCtl>(std::move(make_db_curs(SQLText, m_sess)));
    }
}

void TQuery::bindVariables()
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

std::string TQuery::fieldValueAsString(const char* fname, const std::string& val)
{
    std::string value;
    if(!PgCpp::details::PgTraits<std::string>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a String", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

int TQuery::fieldValueAsInteger(const char* fname, const std::string& val)
{
    int value = 0;
    if(!PgCpp::details::PgTraits<int>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to an Integer", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

TDateTime TQuery::fieldValueAsDateTime(const char* fname, const std::string& val)
{
    boost::posix_time::ptime value;
    if(!PgCpp::details::PgTraits<boost::posix_time::ptime>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a DateTime", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return BASIC::date_time::BoostToDateTime(value);
}

double TQuery::fieldValueAsFloat(const char* fname, const std::string& val)
{
    double value = 0.0;
    if(!PgCpp::details::PgTraits<double>::setValue(reinterpret_cast<char*>(&value), val.c_str(), val.length())) {
        char serror[ 100 ];
        sprintf(serror, "Cannot convert field %s to a Float", fname);
        throw EXCEPTIONS::EConvertError(serror);
    }

    return value;
}

void TQuery::Close()
{

}

void TQuery::Execute()
{
    initInnerCursCtl();
    bindVariables();
    m_cur->exec();

    if(isSelectQuery(SQLText)) {
        Next();
    }
}

void TQuery::Next()
{
    if(Eof) {
        tst();
        return;
    }

    ASSERT(m_cur);
    if(m_cur->nefen() == DbCpp::ResultCode::NoDataFound) {
        Eof = 1;
    }
}

void TQuery::Clear()
{
    m_variables.clear();
    SQLText.clear();
    m_cur.reset();
    Eof = 0;
}

int TQuery::FieldsCount()
{
    ASSERT(m_cur);
    return m_cur->fieldsCount();
}

std::string TQuery::FieldName(int ind)
{
    ASSERT(m_cur);
    return m_cur->fieldName(ind);
}

int TQuery::FieldIsNull(const std::string& fname)
{
    ASSERT(m_cur);
    return static_cast<int>(m_cur->fieldIsNull(fname));
}

int TQuery::FieldIsNull(int ind)
{
    ASSERT(m_cur);
    return static_cast<int>(m_cur->fieldIsNull(ind));
}

int TQuery::GetFieldIndex(const std::string& fname)
{
    ASSERT(m_cur);
    return m_cur->fieldIndex(fname);
}

int TQuery::FieldIndex(const std::string& fname)
{
    int fieldIndex = GetFieldIndex(fname);
    if(fieldIndex == -1)
    {
        char serror[ 100 ];
        sprintf(serror, "Field %s does not exists", fname.c_str());
        throw EOracleError(serror, 0, SQLText.c_str());
    }
    return fieldIndex;
}

std::string TQuery::FieldAsString(const std::string& fname)
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

std::string TQuery::FieldAsString(int ind)
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

int TQuery::FieldAsInteger(const std::string& fname)
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

int TQuery::FieldAsInteger(int ind)
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

TDateTime TQuery::FieldAsDateTime(const std::string& fname)
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

TDateTime TQuery::FieldAsDateTime(int ind)
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

double TQuery::FieldAsFloat(const std::string& fname)
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

double TQuery::FieldAsFloat(int ind)
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

void TQuery::CreateVariable(const std::string& vname, otFieldType, int vdata)
{
    m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
}

void TQuery::CreateVariable(const std::string& vname, otFieldType, const std::string& vdata)
{
    m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
}

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, double vdata)
{
    if(vtype == otDate) {
        m_variables.emplace(vname, Variable{BASIC::date_time::DateTimeToBoost(vdata), false/*IsNull*/});
    } else {
        m_variables.emplace(vname, Variable{vdata, false/*IsNull*/});
    }
}

void TQuery::CreateVariable(const std::string& vname, otFieldType vtype, tnull)
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

int TQuery::VariablesCount()
{
    return m_variables.size();
}

// Фиктивный метод. По сути работает как exists
// Вернет 1, если переменная существует,
//       -1, если иначе
int TQuery::GetVariableIndex(const std::string& vname)
{
    return algo::find_opt<std::optional>(m_variables, vname) ? 1 : -1;
}

int TQuery::VariableIndex(const std::string& vname)
{
    int ind = GetVariableIndex(vname);
    if(ind == -1) {
        char serror[ 100 ];
        sprintf(serror, "Variable %s does not exists", vname.c_str());
        throw EOracleError(serror, 0, SQLText.c_str());
    }

    return ind;
}

int TQuery::VariableIsNull(const std::string& vname)
{
    if(vname.empty()) {
        throw EOracleError( "You cannot get value to variable with an empty name", 1 );
    }

    auto optVar = algo::find_opt<std::optional>(m_variables, vname);
    if(!optVar) {
        char serror[ 100 ];
        sprintf(serror, "Variable %s does not exists", vname.c_str());
        throw EOracleError(serror, 0, SQLText.c_str());
    }

    return static_cast<int>(optVar->IsNull);
}

void TQuery::DeleteVariable(const std::string& vname)
{
    if(vname.empty()) {
        throw EOracleError("You cannot delete a variable with an empty name", 1);
    }

    if(VariableIndex(vname) >= 0) {
        m_variables.erase(vname);
    }
}

void TQuery::DeclareVariable(const std::string& vname, otFieldType vtype)
{
    if(vname.empty()) {
        throw EOracleError("You cannot declare a variable with an empty name", 1);
    }

    if(GetVariableIndex(vname) >= 0) {
        m_variables.erase(vname);
    }

    CreateVariable(vname, vtype, FNull);
}

void TQuery::checkVariable4Set(const std::string& vname)
{
    if(vname.empty()) {
       throw EOracleError("You cannot set value to variable with an empty name", 1);
    }

    if(GetVariableIndex(vname) < 0) {
        throw EOracleError("Field index out of range", 0);
    }
}

void TQuery::SetVariable(const std::string& vname, int vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otInteger, vdata);
}

void TQuery::SetVariable(const std::string& vname, const std::string& vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otString, vdata);
}

void TQuery::SetVariable(const std::string& vname, double vdata)
{
    checkVariable4Set(vname);
    DeleteVariable(vname);
    CreateVariable(vname, otFloat, vdata);
}

void TQuery::SetVariable(const std::string& vname, tnull vdata)
{
    checkVariable4Set(vname);
    // nothing to do - already have null
}

}//namespace PG

/////////////////////////////////////////////////////////////////////////////////////////

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "xp_testing.h"
#include "pg_session.h"

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

    PG::TQuery Qry;
    Qry.SQLText = "insert into TEST_TQUERY(FLD1, FLD2 , FLD3, FLD4) values (:fld1, :fld2, :fld3, :fld4)";
    Qry.DeclareVariable("fld1", otInteger);
    Qry.SetVariable("fld1", intval);
    Qry.CreateVariable("fld2", otDate,    timevaldt);
    Qry.CreateVariable("fld3", otString,  strval);
    Qry.CreateVariable("fld4", otInteger, FNull);

    fail_unless(Qry.VariableIsNull("fld2") == 0, "VariableIsNull failed");
    fail_unless(Qry.VariableIsNull("fld4") == 1, "VariableIsNull failed");

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

        fail_unless(!Qry.FieldIsNull("FLD1"), "FieldIsNull failed");
        fail_unless(!Qry.FieldIsNull(1),      "FieldIsNull failed");
        fail_unless(!Qry.FieldIsNull("FLD3"), "FieldIsNull failed");
        fail_unless(Qry.FieldIsNull("FLD4"),  "FieldIsNull failed");
        fail_unless(Qry.FieldIsNull(3),       "FieldIsNull failed");
        fail_unless(!Qry.FieldIsNull("FLD5"), "FieldIsNull failed");

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



#define SUITENAME "pg_tquery"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(pg_tquery_common_usage);
}
TCASEFINISH;
#undef SUITENAME

#endif /*XP_TESTING*/
