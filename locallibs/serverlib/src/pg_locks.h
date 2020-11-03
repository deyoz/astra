#pragma once

#include <serverlib/pg_cursctl.h>

namespace PgCpp {

class LockTimeout
{
    SessionDescriptor psd_;
public:
    LockTimeout(SessionDescriptor, size_t timeout_ms);
    ~LockTimeout();
};

} //PgCpp
