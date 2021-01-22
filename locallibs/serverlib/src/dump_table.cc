#include "dump_table.h"
#include "dbcpp_session.h"

#include "str_utils.h"

#define NICKNAME "IISannikov"
#include "slogger.h"

namespace ServerFramework
{

template<typename Curs>
DumpTable<Curs>::DumpTable(const std::string& table)
    : table_(table)
{}

template<typename Curs>
DumpTable<Curs>& DumpTable<Curs>::addFld(const std::string& fieldsStr)
{
    std::vector<std::string> fields;
    StrUtils::split_string(fields, fieldsStr);
    for(const std::string& fld:  fields) {
        fields_.push_back(fld);
    }
    return *this;
}

template<typename Curs>
DumpTable<Curs>& DumpTable<Curs>::where(const std::string& wh)
{
    where_ = wh;
    return *this;
}

template<typename Curs>
DumpTable<Curs>& DumpTable<Curs>::order(const std::string& ord)
{
    order_ = ord;
    return *this;
}

class DumpTableOut
{
public:
    virtual ~DumpTableOut() = default;
    virtual void print(std::string const& s) const =0;
};

namespace {

class DumpTableOutLogger : public DumpTableOut
{
private:
    int loglevel__;
    std::string nick__;
    std::string file__;
    int line__;

public:
    DumpTableOutLogger(int loglevel,const char* nick,const char* file,int line)
      : loglevel__(loglevel),nick__(nick),file__(file),line__(line) {}
    virtual ~DumpTableOutLogger() = default;
    virtual void print(std::string const& s) const override final
    {
      LogTrace(loglevel__, nick__.c_str(), file__.c_str(), line__)<<s;
    }
};

class DumpTableOutString : public DumpTableOut
{
private:
    std::string &s__;

public:
    DumpTableOutString(std::string &out) : s__(out) { s__.clear(); }
    virtual ~DumpTableOutString() = default;
    virtual void print(std::string const& s) const override final
    {
        s__.append(s).append("\n");
    }
};

} // namespace

template<typename Curs>
void DumpTable<Curs>::exec(int loglevel, const char* nick, const char* file, int line)
{
    exec(DumpTableOutLogger(loglevel,nick,file,line));
}

template<typename Curs>
void DumpTable<Curs>::exec(std::string &out)
{
    exec(DumpTableOutString(out));
}

template<typename Curs>
void DumpTable<Curs>::exec(DumpTableOut const& out)
{
    out.print("--------------------- " + table_ + " DUMP ---------------------");
    std::string sql = "SELECT ";

    if (fields_.empty()) {
        std::string fld;
        Curs cr(createCursorToFindColumns());
        cr.def(fld);
        cr.exec();
        while (!cr.fen()) {
            fields_.push_back(fld);
        }
        if (fields_.empty()) {
            throwTableDoesNotExist();
        }
    }

    for(const std::string& fld:  fields_) {
        sql += fld + ", ";
    }
    sql = sql.substr(0, sql.length() - 2);
    sql += " FROM " + table_;
    if (!where_.empty()) {
        sql += " WHERE " + where_;
    }
    if (!order_.empty()) {
        sql += " ORDER BY " + order_;
    }

    out.print(sql);
    std::vector<std::string> vals(fields_.size());
    Curs cr(createCursor(sql));
    for(std::string& val:  vals) {
        cr.defNull(val, "NULL");
    }
    cr.exec();
    size_t count=0;
    while (!cr.fen()) {
        std::stringstream str;
        for(const std::string& val:  vals) {
            str << "[" << val << "] ";
        }
        out.print(str.str());
        count++;
    }
    out.print("------------------- END " + table_ + " DUMP COUNT="
      + HelpCpp:: string_cast(count) + " -------------------");
}

template class DumpTable<OciCpp::CursCtl>;
template class DumpTable<PgCpp::CursCtl>;
template class DumpTable<DbCpp::CursCtl>;

OraDumpTable::OraDumpTable(OciCpp::OciSession& sess, const std::string& table)
    : DumpTable<OciCpp::CursCtl>{table}, sess_{sess}
{}

OraDumpTable::OraDumpTable(const std::string& table)
    : DumpTable<OciCpp::CursCtl>{table}, sess_{OciCpp::mainSession()}
{}

OciCpp::CursCtl OraDumpTable::createCursor(const std::string& sql) const
{
    return sess_.createCursor(STDLOG, sql);
}

OciCpp::CursCtl OraDumpTable::createCursorToFindColumns() const
{
    return createCursor("SELECT COLUMN_NAME FROM USER_TAB_COLUMNS WHERE TABLE_NAME = UPPER('"
            + table_ + "') ORDER BY COLUMN_ID");
}

void OraDumpTable::throwTableDoesNotExist() const
{
    throw OciCpp::ociexception('[' + sess_.getConnectString() + "] table " + table_ + " doesn't exist");
}

PgDumpTable::PgDumpTable(PgCpp::SessionDescriptor sess, const std::string& table)
    : DumpTable<PgCpp::CursCtl>{table}, sess_{sess}
{}

PgCpp::CursCtl PgDumpTable::createCursor(const std::string& sql) const
{
    return make_pg_curs(sess_, sql);
}

PgCpp::CursCtl PgDumpTable::createCursorToFindColumns() const
{
    return createCursor("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = LOWER('"
            + table_ + "')");
}

void PgDumpTable::throwTableDoesNotExist() const
{
    throw PgCpp::PgException(STDLOG, sess_, PgCpp::NoDataFound,  "table " + table_ + " doesn't exist");
}

DbDumpTable::DbDumpTable(DbCpp::Session& sess, const std::string& table)
    : DumpTable<DbCpp::CursCtl>{table}, sess_{sess}
{}

DbCpp::CursCtl DbDumpTable::createCursor(const std::string& sql) const
{
    return make_db_curs(sql, sess_);
}

DbCpp::CursCtl DbDumpTable::createCursorToFindColumns() const
{
    if(sess_.getType() == DbCpp::DbType::Postgres) {
        return createCursor("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = LOWER('"
                + table_ + "') ORDER BY COLUMN_NAME ");
    } else {
        return createCursor("SELECT COLUMN_NAME FROM USER_TAB_COLUMNS WHERE TABLE_NAME = UPPER('"
                + table_ + "') ORDER BY COLUMN_ID");
    }
}

void DbDumpTable::throwTableDoesNotExist() const
{
    throw comtech::Exception("table " + table_ + " doesn't exist");
}

} // ServerFramework

#ifdef XP_TESTING
#include "sirenaproc.h"
#include "checkunit.h"
#include "tcl_utils.h"

namespace {

void setup()
{
    testInitDB();
    OciCpp::openGlobalCursors();
}

START_TEST(test_dump_table)
{
    try {
        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("CREATE TABLE TEST_CHECK_DUMP_TABLE (VALUE1 NUMBER, VALUE2 VARCHAR2(30))").exec();
        make_curs("INSERT INTO TEST_CHECK_DUMP_TABLE (VALUE1, VALUE2) VALUES (1234, 'lolkalolka')").exec();

        ServerFramework::OraDumpTable("TEST_CHECK_DUMP_TABLE").addFld("VALUE2").where("VALUE1 = 1234").exec(TRACE5);
        ServerFramework::OraDumpTable("TEST_CHECK_DUMP_TABLE").order("VALUE1").exec(TRACE5);
        ServerFramework::OraDumpTable("TEST_CHECK_DUMP_TABLE").addFld("value1,value2").exec(TRACE5);

        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    } catch (...) {
        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    }
}
END_TEST

static const char* getTestConnectString()
{
    static std::string connStr = readStringFromTcl("PG_CONNECT_STRING");
    return connStr.c_str();
}

START_TEST(test_pg_dump_table)
{
    const auto session = PgCpp::getManagedSession(getTestConnectString());
    try {
        make_pg_curs(session, "DROP TABLE IF EXISTS TEST_CHECK_DUMP_TABLE").exec();
        make_pg_curs(session, "CREATE TABLE TEST_CHECK_DUMP_TABLE (VALUE1 NUMERIC, VALUE2 VARCHAR(30))").exec();
        make_pg_curs(session, "INSERT INTO TEST_CHECK_DUMP_TABLE (VALUE1, VALUE2) VALUES (1234, 'lolkalolka')").exec();
        ServerFramework::PgDumpTable(session, "TEST_CHECK_DUMP_TABLE").addFld("VALUE2").where("VALUE1 = 1234").exec(TRACE5);
        ServerFramework::PgDumpTable(session, "TEST_CHECK_DUMP_TABLE").order("VALUE1").exec(TRACE5);
        ServerFramework::PgDumpTable(session, "TEST_CHECK_DUMP_TABLE").addFld("value1,value2").exec(TRACE5);

        make_pg_curs(session, "DROP TABLE IF EXISTS TEST_CHECK_DUMP_TABLE").exec();
    } catch (...) {
        make_pg_curs(session, "DROP TABLE IF EXISTS TEST_CHECK_DUMP_TABLE").exec();
    }
}
END_TEST

#define SUITENAME "SqlUtil"
TCASEREGISTER(setup, 0)
{
    ADD_TEST(test_dump_table);
}
TCASEFINISH
#undef SUITENAME // "SqlUtil"

#define SUITENAME "PgSqlUtil"
TCASEREGISTER(0, 0)
{
    ADD_TEST(test_pg_dump_table);
}
TCASEFINISH

} // namespace
#endif /* XP_TESTING */
