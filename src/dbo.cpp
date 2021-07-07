#include "dbo.h"
#include <serverlib/algo.h>
#include <serverlib/str_utils.h>
#include "hooked_session.h"

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

using namespace std;


namespace dbo
{

std::string buildQuery(const std::shared_ptr<MappingInfo> &mapInfo, const QueryOps &ops, bool isOracle)
{
    std::stringstream res;
    if(!ops.select.empty()) {
        res << ops.select;
    } else {
        if(!mapInfo) {
            throw EXCEPTIONS::Exception("Not correct select type or forgot select query");
        }
        res << "select " << mapInfo->columnsStr(mapInfo->tableName());
    }
    if(!ops.from.empty()) {
        res << " from " << ops.from;
    } else {
        if(!mapInfo) {
            throw EXCEPTIONS::Exception("Not correct select type");
        }
        res << " from " << mapInfo->tableName();
    }
    if(!ops.where.empty()) {
        res <<  " where " << ops.where;
    }
    if(!ops.order_by.empty()) {
        res <<  " order by " << ops.order_by;
    }
    if(!ops.fetch_first.empty()) {
        if(isOracle && ops.for_update) {
            res << " and rownum <= " << ops.fetch_first;
        } else {
            res << " fetch first " << ops.fetch_first << " rows only";
        }
    }
    if(ops.for_update) {
        res << " for update";
    }
    return res.str();
}

std::string firstTableFrom(const std::string& from)
{
    std::vector<std::string> tableNames;
    StrUtils::split_string(tableNames, from);
    if(!tableNames.empty()) return tableNames[0];
    return "";
}

DbCpp::Session* getSession(CurrentDb db, const std::shared_ptr<MappingInfo>& mapInfo,
                           bool isForUpdate, const std::string& from)
{
    DbCpp::Session* session = nullptr;
    std::string tableName = mapInfo ? mapInfo->tableName() : firstTableFrom(from);
    if(db==Config) {
        if(isForUpdate) {session = &PgOra::getRWSession(tableName);}
        else            {session = &PgOra::getROSession(tableName);}
    }
    else if(db==Postgres) {
        bool isGroupArx = (PgOra::getGroup(tableName) == "SP_PG_GROUP_ARX");
        if(isForUpdate) {
            if(isGroupArx)  {session = get_arx_pg_rw_sess(STDLOG);}
            else            {session = get_main_pg_rw_sess(STDLOG);}
        } else {
            if(isGroupArx)  {session = get_arx_pg_ro_sess(STDLOG);}
            else            {session = get_main_pg_ro_sess(STDLOG);}
        }
    }
    else if(db==Oracle) {session = get_main_ora_sess(STDLOG);}
    return session;
}

Transaction Session::transactPolicy(DbCpp::Session &session)
{
    if(!session.isOracle() && !_ignoreErrors.empty()) {
        return Transaction(session, Transaction::Policy::Managed);
    }
    return Transaction(session, Transaction::Policy::AutoCommit);
}

std::vector<DbCpp::ResultCode> Session::moveErrors()
{
    auto ret = std::move(_ignoreErrors);
    _ignoreErrors.clear();
    return ret;
}


std::string Cursor::dump(size_t fields_size)
{
    std::stringstream res;
    std::vector<std::string> vals(fields_size);

    for(std::string& val:  vals) {
        cur_.defNull(val, "NULL");
    }
    cur_.exec();

    while(!cur_.fen()) {
        for(const std::string& val:  vals) {
            res << "[" << val << "] ";
        }
        res << std::endl;
    }

    return res.str();
}

std::vector<std::string> MappingInfo::columns() const
{
    std::vector<std::string> res;
    for(const auto & f: m_fields) {
        res.push_back(f.name());
    }
    return res;
}

std::string MappingInfo::columnsStr(const std::string& table) const
{
    std::vector<std::string> cols = columns();
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
    LogTrace(TRACE6) << " INSERT: " << st.str();
    return st.str();
}

std::string dumpColumns(shared_ptr<MappingInfo>& mapInfo, const vector<std::string>& fields)
{
    std::string result;
    for(const std::string & field: fields) {
        std::optional<FieldInfo> optField = algo::find_opt_if<std::optional>(mapInfo->fields(),
                      [&field](const FieldInfo & f) { return f.name() == StrUtils::ToLower(field);} );
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

std::string Session::dump(const std::string &tableName, const vector<std::string> &tokens,
                          const std::string &query)
{
    //std::string result_query;
    std::string tblName = StrUtils::ToLower(tableName);
    shared_ptr<MappingInfo> mapInfo = Mapper::getInstance().getMapping(tblName);
    if(!mapInfo) {
        throw EXCEPTIONS::Exception("Unknown table name: " + tblName);
    }
    auto fields = tokens.empty() ? mapInfo->columns() : tokens;
    std::string select_query = "select " + dumpColumns(mapInfo, fields);
    std::string from_query = " from " + tableName + " " + query;

    DbCpp::Session& session = PgOra::getROSession(tableName);
    LogTrace(TRACE6) << "---------------- " << tableName << " " << " DUMP ----------------------";
    LogTrace(TRACE6) << StrUtils::join(',', fields);
    Cursor cur(session.createCursor(STDLOG, select_query + from_query), Transaction(session));
    std::string dump = cur.dump(fields.size());
    LogTrace(TRACE6) << dump;
    return dump;
}

}


