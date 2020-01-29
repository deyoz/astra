#pragma once
#ifdef ENABLE_PG
#include "pg_cursctl.h"

namespace PgCpp
{

class Sequence
{
public:
    Sequence(SessionDescriptor sd, const std::string& name)
        : name_(name), sd_(sd)
    {}
    template<typename T>
    T nextval(const char* nick, const char* file, int line) {
        T t;
        PgCpp::make_curs_(nick, file, line, sd_, "SELECT nextval('" + name_ + "')")
            .def(t)
            .exfet();
        return t;
    }
private:
    std::string name_;
    SessionDescriptor sd_;
};

} // PgCpp
#endif // ENABLE_PG
