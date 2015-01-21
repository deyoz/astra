/*
*  C++ Implementation: EdiTimeOutsDaemon
*
* Description: Edifact Time Outs Handler
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/


#include "tlg/EdiSessionTimeOut.h"
#include "exceptions.h"
#include "astra_main.h" // init_locale()

#include <tclmon/tclmon.h>
#include <serverlib/new_daemon.h>
#include <serverlib/daemon_event.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/timer.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


using namespace ServerFramework;
using namespace edifact;

class EdifactTimeOutsDaemon : public NewDaemon
{
    virtual void init();
public:
    static const char *Name;
    EdifactTimeOutsDaemon();
};

class EdifactTimeOutsHandler : public DaemonTask
{
public:
    virtual int run(const boost::posix_time::ptime& );
    virtual void monitorRequest() {}
    EdifactTimeOutsHandler();
};

const char *EdifactTimeOutsDaemon::Name = "Edifact Time Outs handler";

EdifactTimeOutsDaemon::EdifactTimeOutsDaemon()
    :NewDaemon("EDI_TIMER")
{
}

void EdifactTimeOutsDaemon::init()
{
    if(init_locale(/*Environment::Daemon*/)<0)
    {
        throw EXCEPTIONS::Exception("init_locale failed");
    }
}

EdifactTimeOutsHandler::EdifactTimeOutsHandler()
    :DaemonTask(DaemonTaskTraits::OracleAndHooks())
{
}

namespace
{
using namespace edilib;
void run_edi_timeout_handler()
{
    Timer::timer timer;
    LogTrace(TRACE1) << EdifactTimeOutsDaemon::Name << ": run at " <<
            boost::posix_time::second_clock::local_time();

    std::list<EdiSessionTimeOut> lExpired;
    EdiSessionTimeOut::readExpiredSessions(lExpired);

    for(std::list<EdiSessionTimeOut>::const_iterator iter=lExpired.begin();
        iter != lExpired.end(); iter ++)
    {
        LogTrace(TRACE1) << "Start handling timeout for " << iter->msgTypeStr() <<
                ":" << iter->funcCode();
        HandleEdiSessTimeOut(*iter);

        // Если кто ждал эту телеграмму, проснись мать твою! (пожалуйста, а?)
        // TODO
        //TlgHandling::PostponedTlgHandling::deleteWaiting(iter->ediSessionId());
        //iter->deleteEdiSession();
    }

    monitor_idle_zapr_type(lExpired.size(), QUEPOT_NULL);
}
}

void runEdiTimer_4testsOnly()
{
    //Environment::HandlerType ht = Environment::Environ::Instance().handlerType();
    //Environment::Environ::setHandlerType(Environment::Daemon);

    run_edi_timeout_handler();

    //Environment::Environ::setHandlerType(ht);
}

int EdifactTimeOutsHandler::run(const boost::posix_time::ptime &)
{
    run_edi_timeout_handler();
    return 0;
}

int main_edi_timer_tcl(int supervisorSocket, int argc, char *argv[])
{
    EdifactTimeOutsDaemon daemon;
    DaemonEventPtr timer(new TimerDaemonEvent(1));
    timer->addTask(DaemonTaskPtr(new EdifactTimeOutsHandler));
    daemon.addEvent(timer);

    daemon.run();

    return 0;
}
