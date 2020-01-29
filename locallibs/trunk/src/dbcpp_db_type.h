#pragma once

namespace DbCpp
{
    enum class DbType
    {
        Oracle = 0,
        Postgres,
    };
    enum class DbSessionType
    {
        Managed    = 1,
        Autonomous = 2
    };
} // DbCpp
