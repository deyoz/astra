#if HAVE_CONFIG_H
#endif

/*
*  C++ Implementation: TlgLogger
*
* Description: logger for telegrams
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#include "TlgLogger.h"
#include <iostream>
#include "timer.h"
#include "lwriter.h"
#include "tcl_utils.h"
#include "tclmon.h"
#include "testmode.h"
#include "logwriter.h"

#define NICKNAME "ROMAN"
#include "slogger.h"

bool isNosir();

std::unique_ptr<LogWriter> TlgLogger::lw_;
bool TlgLogger::writeToRegularLog_ { true };

void TlgLogger::setLogging(const char* fileName, const bool writeToRegularLog)
{
    if(readIntFromTcl("USE_RSYSLOG", 0)) {
        std::string addr(readStringFromTcl("RSYSLOG_HOST"));

        addr.append(1, ':');
        addr.append(readStringFromTcl("RSYSLOG_PORT"));
        lw_ = makeLogger(LOGGER_SYSTEM_RSYSLOG, addr.c_str(), "airimp");
    } else if (inTestMode() || isNosir()) {
        lw_ = makeLogger(LOGGER_SYSTEM_FILE, fileName, tclmonCurrentProcessName());
    } else {
        Tcl_Obj* o = Tcl_GetVar2Ex(getTclInterpretator(), "logtlg", "SOCKET", TCL_GLOBAL_ONLY);
        if (!o) {
            write_log_message_stderr(STDLOG, "cannot find logtlg(SOCKET)");
            Abort(1);
        }
        lw_ = makeLogger(LOGGER_SYSTEM_LOGGER, Tcl_GetString(o), tclmonCurrentProcessName());
    }

    writeToRegularLog_ = writeToRegularLog;
}

void TlgLogger::flush()
{
    write_log_to_writer(lw_.get(), 0, "", stream_.str().c_str());
#ifdef _TLGLOG_ON_STDOUT_
    std::cout << buff << std::endl;
#endif /*_TLGLOG_ON_STDOUT_*/
}

TlgLogger::~TlgLogger()
{
    if (writeToRegularLog_) {
        LogTrace(0, "TLGLOG", "===", 0) << stream_.str();
    }

    flush();
}
