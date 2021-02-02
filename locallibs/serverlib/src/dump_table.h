#pragma once

#include <list>
#include <string>
#include "cursctl.h"
#include "pg_cursctl.h"
#include "dbcpp_cursctl.h"

namespace ServerFramework
{
class DumpTableOut;

template<typename Curs>
class DumpTable
{
public:
    DumpTable(const std::string& table);
    virtual ~DumpTable() = default;

    DumpTable& addFld(const std::string& fld);
    DumpTable& where(const std::string& wh);
    DumpTable& order(const std::string& ord);
    void exec(int loglevel, const char* nick, const char* file, int line);
    void exec(std::string& out);

protected:
    std::string table_;

private:
    std::list<std::string> fields_;
    std::string where_;
    std::string order_;

    void exec(DumpTableOut const& out);
    virtual Curs createCursorToFindColumns() const = 0;
    virtual Curs createCursor(const std::string& sql) const = 0;
    [[noreturn]] virtual void throwTableDoesNotExist() const = 0;
};

class OraDumpTable: public DumpTable<OciCpp::CursCtl>
{
public:
    OraDumpTable(OciCpp::OciSession&, const std::string& table);
    OraDumpTable(const std::string& table);

private:
    OciCpp::OciSession& sess_;

    OciCpp::CursCtl createCursorToFindColumns() const override;
    OciCpp::CursCtl createCursor(const std::string& sql) const override;
    [[noreturn]] void throwTableDoesNotExist() const override;
};

class PgDumpTable: public DumpTable<PgCpp::CursCtl>
{
public:
    PgDumpTable(PgCpp::SessionDescriptor, const std::string& table);

private:
    PgCpp::SessionDescriptor sess_;

    PgCpp::CursCtl createCursorToFindColumns() const override;
    PgCpp::CursCtl createCursor(const std::string& sql) const override;
    [[noreturn]] void throwTableDoesNotExist() const override;
};

class DbDumpTable: public DumpTable<DbCpp::CursCtl>
{
public:
    DbDumpTable(DbCpp::Session&, const std::string& table);

private:
    DbCpp::Session& sess_;

    DbCpp::CursCtl createCursorToFindColumns() const override;
    DbCpp::CursCtl createCursor(const std::string& sql) const override;
    [[noreturn]] void throwTableDoesNotExist() const override;
};

} // ServerFramework

namespace OciCpp {

using DumpTable = ServerFramework::OraDumpTable;

}

namespace DbCpp {

using DumpTable = ServerFramework::DbDumpTable;

}
