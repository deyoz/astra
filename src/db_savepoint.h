#pragma once

#include <string>


namespace DB {

class Savepoint
{
public:
    Savepoint(const std::string& name);
    Savepoint(const Savepoint&) =  delete;
    void operator=(const Savepoint&) = delete;

    void rollback();

private:
    std::string m_name;
};

//---------------------------------------------------------------------------------------

void execSpCmd(const std::string& cmd);

}//namespace DB
