#include <unistd.h>
#include "startparam.h"

#define NICKNAME ""
#include "slogger.h"

namespace Supervisor {
std::map<std::string, int> StartParam::grpID;

GroupDescription::GroupDescription(int handlersCount,
                                   const std::string& startLine,
                                   int priority,
                                   PROCESS_LEVEL pl):
    handlersCount_(handlersCount),
    startLine_(startLine),
    priority_(priority),
    pl_(pl)
{
}

GroupDescription::GroupDescription(const GroupDescription& other):
                    handlersCount_(other.handlersCount_),
                    startLine_(other.startLine_), priority_(other.priority_), pl_(other.pl_)

{
}

GroupDescription& GroupDescription::operator=(const GroupDescription& rhs)
{
    GroupDescription tmp(rhs);

    this->swap(tmp);

    return *this;
}

StartParam& StartParam::Instance()
{
    static StartParam instance;
    return instance;
}

void log_trace_1(const char* msg)
{
    ProgTrace(TRACE1,"%s",msg);
}

} //namespace Supervisor
