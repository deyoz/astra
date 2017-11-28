#include "environ.h"
#include "basetables.h"
#include "tlg/remote_system_context.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Environment {

Ticketing::Airline_t Environ::airline()
{
    using Ticketing::RemoteSystemContext::SystemContext;
    return SystemContext::Instance(STDLOG).airlineImpl()->ida();
}

}//namespace Environment
