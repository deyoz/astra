#pragma once

#include <iosfwd>

namespace DbCpp
{
    enum class DbType
    {
        Oracle = 0,
        Postgres,
    };
    std::ostream & operator << (std::ostream& os,const DbType x);    
    
    enum class DbSessionType
    {
        Managed    = 1,
        Autonomous = 2
    };
    std::ostream & operator << (std::ostream& os,const DbSessionType x);    
} // DbCpp
