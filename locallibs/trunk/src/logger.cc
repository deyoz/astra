#if HAVE_CONFIG_H
#endif



#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <tcl.h>
#include <cstring>
#include <limits>

#include "ourtime.h"
#include "logger.h"
#include "tclmon.h"
#include "tcl_utils.h"
#include "lwriter.h"
#include "monitor_ctl.h"
#include "log_manager.h"

#define NICKNAME "SYSTEM"
#include "test.h"

#ifdef XP_TESTING
static int abortOnErrorFlag = 1;
void allowProgError()
{
    abortOnErrorFlag = 0;
}
void denyProgError()
{
    abortOnErrorFlag = 1;
}
void abortOnError()
{
    const int progErrorHunt = readIntFromTcl("PROG_ERROR_HUNT", 0);
    if (abortOnErrorFlag && progErrorHunt)
        abort();
}
#endif // XP_TESTING
static int was_prog_err = 0;

int was_prog_error()
{
  return was_prog_err;
}

void reset_prog_error()
{
  was_prog_err = 0;
}

int OpenLogFile(const char* processName, const char* grpName)
{
    static const size_t GRP_LENGTH = 4;

    struct sigaction sigact;
    sigset_t sigset;
    sigemptyset(&sigset);
    memset(&sigact,0,sizeof(sigact));
    sigact.sa_mask=sigset;
    sigact.sa_flags=SA_RESTART;
    sigact.sa_handler=sigusr2;

    Logger::setTracer(new Logger::ServerlibTracer());
    if (sigaction(SIGUSR2, &sigact, 0) < 0) {
        ProgError(STDLOG, "Sigaction failed - SIGUSR2");
    }

    if (!processName) {
        if (grpName) {
            if(!strncmp(grpName, "grp2", GRP_LENGTH) || !strncmp(grpName, "grp5", GRP_LENGTH) ||
               !strncmp(grpName, "grp6", GRP_LENGTH) || !strncmp(grpName, "grp7", GRP_LENGTH))
            {
                setLoggingGroup("loginet", LOGGER_SYSTEM_LOGGER);
            } else if(strncmp(grpName, "grp3", 4)==0) {
                setLoggingGroup("logjxt", LOGGER_SYSTEM_LOGGER);
            } else {
                setLoggingGroup("log1", LOGGER_SYSTEM_LOGGER);
            }
        }
    } else if (!std::strcmp("monitor", processName)) {
        if (readIntFromTcl("CUT_MONITOR_LOG", 1)) {
            CutLogHolder::Instance().setLogLevel(std::numeric_limits<int>::min());
        } else {
            setLoggingGroup("MONITOR_LOG", LOGGER_SYSTEM_FILE);
        }
    } else if (std::strcmp("logger", processName)) {
        setLoggingGroup(processName, LOGGER_SYSTEM_LOGGER);
    }

    return 0;
}

void ProgError(const char *nickname, const char *filename, int line, const char *format,  ...)
{
    va_list ap;
    was_prog_err=1;
    va_start(ap,format);
    write_log_message(-1, nickname, filename, line, format, ap);
    va_end(ap);
}	

void ProgTrace(int l, const char *nickname, const char *filename,int line, const char *format,  ...)
{
    if (LogManager::Instance().isWriteLog(l)) {
        va_list ap;
        va_start(ap, format);
        write_log_message(l, nickname, filename, line, format, ap);
        va_end(ap);
    }
}	

void WriteLog(const char *nickname, const char *filename, int line, const char *format,  ...)
{
    va_list ap;
    va_start(ap, format);
    write_log_message(0, nickname, filename, line, format, ap);
    va_end(ap);
}	

static int externalLogLevel=-1;

int logLevelExternal()
{
    return externalLogLevel;
}

void setLogLevelExternal(int l)
{
    externalLogLevel=l;
}

namespace ServerFramework
{

void LogLevel::setLevel(int lvl)
{
    setLogLevelExternal(lvl);
}

}

int getRealTraceLev(int level)
{
    if(logLevelExternal()!=-1)
    {
        return logLevelExternal();
    }
    return level;
}

int getTraceLev(int level, const char *nickname,const char *filename, int line)
// Returns loglevel for TRACE : getTraceLev(TRACE5) returns 12
{
  return level;
}

int getTraceLevByNum(int trace_num)
// Returns loglevel for 5 : getTraceLevByNum(5)=getTraceLev(TRACE5) returns 12
{
  if (trace_num<0 || trace_num>=7)
  {
    return getTraceLev(TRACE7);
  }
  else if (trace_num==6)
  {
    return getTraceLev(TRACE6);
  }
  else if (trace_num==5)
  {
    return getTraceLev(TRACE5);
  }
  else if (trace_num==4)
  {
    return getTraceLev(TRACE4);
  }
  else if (trace_num==3)
  {
    return getTraceLev(TRACE3);
  }
  else if (trace_num==2)
  {
    return getTraceLev(TRACE2);
  }
  else if (trace_num==1)
  {
    return getTraceLev(TRACE1);
  }

  return getTraceLev(TRACE0);  
}

namespace Logger 
{

void ServerlibTracer::ProgError(const char *nickname, const char *filename, int line, const char *format,  ...)
{
    va_list ap;
    was_prog_err=1;
    va_start(ap,format);
    write_log_message(-1, nickname, filename, line, format, ap);
    va_end(ap);
}

void ServerlibTracer::ProgTrace(int l, const char *nickname, const char *filename, int line, const char *format,  ...)
{
    if (LogManager::Instance().isWriteLog(l)) {
        va_list ap;
        va_start(ap, format);
        write_log_message(l, nickname, filename, line, format, ap);
        va_end(ap);
    }
}	

void ServerlibTracer::WriteLog(const char *nickname, const char *filename, int line, const char *format,  ...)
{
    va_list ap;
    va_start(ap, format);
    write_log_message(0, nickname, filename, line, format, ap);
    va_end(ap);
}	

} // Logger

