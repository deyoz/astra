#if HAVE_CONFIG_H
#endif

#include <boost/asio.hpp>

#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "new_daemon.h"
#include "ourtime.h"
#include "monitor_ctl.h"
#include "query_runner.h"
#include "daemon_impl.h"
#include "daemon_event.h"

static int daemon_mode = 0;

bool inDaemonMode()
{
    return daemon_mode == 1;
}

void setDaemonMode( bool dm )
{
    daemon_mode = dm ? 1 : 0;
}

namespace ServerFramework
{

/***  NewDaemon  ***/
class NewDaemonImpl
{
    typedef std::list<DaemonEventPtr> DaemonEvents;
    typedef DaemonEvents::iterator DaemonEventsIt;
public:
    explicit NewDaemonImpl(const std::string& name)
        : io_service_(system_ios::Instance()), name_(name)
    {
        addEvent(DaemonEventPtr(new ControlPipeEvent()));
    }
    virtual ~NewDaemonImpl() {}

    void setName(const std::string& name)
    {
        name_ = name;
    }
    void addEvent(DaemonEventPtr pEvent)
    {
        events_.push_back(pEvent);
        pEvent->setName(name_);
    }

    void run()
    {
        ProgTrace(TRACE5, "%zd events", events_.size());
        for (DaemonEvents::iterator it = events_.begin();
                it != events_.end(); ++it)
            (*it)->init();
        ServerFramework::Run();
        // should never be here
        ProgTrace(TRACE5, "after run ");
    }

private:
    boost::asio::io_service& io_service_;
    DaemonEvents events_;
    std::string name_;
};

NewDaemon::NewDaemon(const std::string& name)
{
    daemon_mode = 1;
    under_gdb();
    set_signal(term3);

    InitLogTime(name.c_str());
    pImpl_.reset(new NewDaemonImpl(name));
    monitor_special();
    monitor_idle();
    applicationCallbacks()->connect_db();
}

NewDaemon::~NewDaemon()
{}
    
void NewDaemon::addEvent(DaemonEventPtr pEvent)
{
    NewDaemonImpl* p = (NewDaemonImpl*) pImpl_.get();
    p->addEvent(pEvent);
}

void NewDaemon::run()
{
    ProgTrace(TRACE5, "NewDaemon::run");
    NewDaemonImpl* p = (NewDaemonImpl*) pImpl_.get();
    init();
    p->run();
}

void Run()
{
    while(system_ios::Instance().run_one()) {
        control_pipe_ios::Instance().poll();
    }
}

} // namespace ServerFramework 

