#pragma once

#include <string>
#include <serverlib/cursctl.h>

namespace OciCpp 
{

class Savepoint
{
public:
    Savepoint(std::string const &p);
    Savepoint(const Savepoint&) = delete;
    void operator=(const Savepoint&) = delete;

    void rollback();
    void reset();
private:
    std::string point;
};

class AutoRollback
{
public:
    AutoRollback(std::string const & s) : sp(s), armed(true){}
    ~AutoRollback() { if (armed) { sp.rollback(); } }
    void reset() { armed = false; }
private:
    OciCpp::Savepoint sp;
    bool armed;
};

} // OciCpp

#ifdef ENABLE_PG
#include <serverlib/pg_cursctl.h>

namespace PgCpp
{

class Session;

class Savepoint
{
public:
    Savepoint(SessionDescriptor, const std::string& p);
    Savepoint(const Savepoint&) = delete;
    void operator=(const Savepoint&) = delete;

    void rollback();
    void reset();
private:
    SessionDescriptor sd_;
    std::string name_;
};

} // PgCpp
#endif // ENABLE_PG
