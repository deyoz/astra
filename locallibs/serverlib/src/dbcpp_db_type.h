#pragma once

#include <iosfwd>

namespace DbCpp
{
    enum class DbType : int
    {
        Oracle = 0,
        Postgres,
    };
    std::ostream & operator << (std::ostream& os,const DbType x);    
    
    enum class DbSessionType : int
    {
        Managed    = 1,
        Autonomous = 2
    };
    std::ostream & operator << (std::ostream& os,const DbSessionType x);    

    enum class DbSessionForType : int
    {
        Reading    = 0, // session type Managed/Autonomous not determinated  
        ManagedReadWrite  = 1, // session type Managed
        AutonomousReadWrite = 2, // session type Autonomous
    };
    std::ostream & operator << (std::ostream& os,const DbSessionForType x);    
} // DbCpp
