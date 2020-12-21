#include "dbo.h"
#include "hooked_session.h"

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace dbo
{

std::vector<string> MappingInfo::columns() const
{
    std::vector<string> res;
    for(const auto & f: m_fields) {
        res.push_back(f.name());
    }
    return res;
}

std::string MappingInfo::columnsStr(const std::string& table) const
{
    std::vector<string> cols = columns();
    std::string result;
    for(const auto & col: cols) {
        if(!result.empty()) {
            result += ",";
        }
        if(!table.empty()) {
            result += table+"."+col;
        } else {
            result += col;
        }
    }
    return result;

}

std::string MappingInfo::insertColumns() const
{
    std::stringstream st;
    st << "INSERT INTO " << tableName() << "(" << columnsStr() << ")" << " VALUES (";

    std::string result;
    for(const auto & col: columns()) {
        if(!result.empty()) {
            result += ",";
        }
        result += ":" + col;
    }
    st << result << ")";
    return st.str();
}

std::string MappingInfo::stringColumns(const vector<std::string>& fields) const
{
    std::string result;
    vector<std::string> get_fields = fields;
    if (get_fields.empty()) {
        get_fields = columns();
    }
    for(const std::string & field: get_fields) {
        std::optional<FieldInfo> optField = algo::find_opt_if<std::optional>(m_fields,
                      [&field](const FieldInfo & f) { return f.name() == str_tolower(field);} );
        if(!optField) {
            throw EXCEPTIONS::Exception(" Not such field :" + field);
        }
        FieldInfo &f = *optField;
        if(!result.empty()) {
            result += ',';
        }
        if (f.type()== typeid(Dates::DateTime_t)) {
            result += "TO_CHAR(" + f.name() + ",'DD.MM.YYYY')";
        } else {
            result += f.name();
        }
    }
    return  result;
}

Session::Session(DbCpp::Session* oraSess,
                 DbCpp::Session* pgSessRo, DbCpp::Session* pgSessRw)
    : ora_dbcpp_session(oraSess),
      pg_dbcpp_session_ro(pgSessRo), pg_dbcpp_session_rw(pgSessRw)
{
}

void Session::clearIgnoreErrors()
{
    ignoreErrors.clear();
}

Session& Session::getMainInstance()
{
    static Session mainInst(get_main_ora_sess(STDLOG),
                            get_main_pg_ro_sess(STDLOG), get_main_pg_rw_sess(STDLOG));
    return mainInst;
}

Session& Session::getArxInstance()
{
    static Session arxInst(get_arx_ora_rw_sess(STDLOG),
                           get_arx_pg_ro_sess(STDLOG), get_arx_pg_rw_sess(STDLOG));
    return arxInst;
}

string Session::dump(const string &tableName, const vector<string> &tokens, const string &query)
{
    std::string result_query;
    std::string tblName = str_tolower(tableName);
    shared_ptr<MappingInfo> mapInfo = Mapper::getInstance().getMapping(tblName);
    if(!mapInfo) {
        throw EXCEPTIONS::Exception("Unknown table name: " + tblName);
    }

    int size = tokens.empty() ? mapInfo->columnsCount() : tokens.size();
    result_query = "select " + mapInfo->stringColumns(tokens);
    result_query += " from " + tableName + " " + query;

    std::string DB;
    if(db == CurrentDb::Postgres) {
        DB = " Postgres ";
    } else {
        DB = " Oracle ";
    }
    LogTrace1 << "---------------- " << tableName << DB << " DUMP ----------------------";
    LogTrace1 << result_query;
    Cursor cur = getWriteCursor(result_query);
    std::string dump = cur.dump(size);
    LogTrace1 << dump;
    return dump;
}

}

