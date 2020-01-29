#ifndef DAEMON_EVENT_H
#define DAEMON_EVENT_H

#include <list>
#include <memory>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "daemon_task.h"

namespace boost {  namespace asio {  class io_context;  }  }

namespace ServerFramework
{

#ifdef XP_TESTING
void runTask(const DaemonTaskPtr& pTask, const boost::posix_time::ptime& now);
void runTaskSirena(const DaemonTaskPtr& pTask, const boost::posix_time::ptime& now,
                    int expectedErr = 0, bool catchException = false);
#endif // XP_TESTING

class DaemonEvent
{
    typedef std::list<DaemonTaskPtr> DaemonTasks;
public:
    DaemonEvent();
    virtual ~DaemonEvent() {}
    void addTask(DaemonTaskPtr pTask);
    void setName(const std::string&);
    virtual void runTasks(const char* buff, size_t buffSz);
    virtual void runTasks(const boost::posix_time::ptime&);
    virtual void runTasks();
    virtual void init() = 0;
private:
    DaemonTasks tasks_;
    std::string name_;
};
typedef std::shared_ptr<DaemonEvent> DaemonEventPtr;


class TimerDaemonEventImpl;

class TimerDaemonEvent : public DaemonEvent
{
public:
    explicit TimerDaemonEvent(size_t seconds);
    virtual ~TimerDaemonEvent() {}
    virtual void init();
private:
    std::shared_ptr<TimerDaemonEventImpl> pImpl_;
};

class TimerAfterDaemonEventImpl;

class TimerAfterDaemonEvent : public DaemonEvent
{
public:
    explicit TimerAfterDaemonEvent(size_t seconds);
    virtual ~TimerAfterDaemonEvent() {}
    virtual void init();
private:
    std::shared_ptr<TimerAfterDaemonEventImpl> pImpl_;
};

class TimerMilliSecAfterDaemonEventImpl;

class TimerMilliSecAfterDaemonEvent : public DaemonEvent
{
public:
    explicit TimerMilliSecAfterDaemonEvent(size_t milliseconds);
    virtual ~TimerMilliSecAfterDaemonEvent() {}
    virtual void init();
private:
    std::shared_ptr<TimerMilliSecAfterDaemonEventImpl> pImpl_;
};

class TimerMicroSecAfterDaemonEventImpl;

class TimerMicroSecAfterDaemonEvent : public DaemonEvent
{
public:
    explicit TimerMicroSecAfterDaemonEvent(size_t microseconds);
    virtual ~TimerMicroSecAfterDaemonEvent() {}
    virtual void init();
private:
    std::shared_ptr<TimerMicroSecAfterDaemonEventImpl> pImpl_;
};

class ScheduleDaemonEvent
    : public DaemonEvent
{
public:
    ScheduleDaemonEvent(size_t seconds);
    virtual ~ScheduleDaemonEvent() {}
    virtual void init();
    virtual void runTasks();
    virtual bool checkSchedule(const boost::posix_time::ptime&) = 0;
    virtual void updateSchedule(const boost::posix_time::ptime&) = 0;
private:
    std::shared_ptr<void> pImpl_;
};


class ControlPipeEventImpl;

class ControlPipeEvent
    : public DaemonEvent
{
public:
    ControlPipeEvent();
    virtual ~ControlPipeEvent() {}
    virtual void init();
    virtual void handleControlMsg(const char* buff, size_t buff_sz);
private:
    std::shared_ptr<ControlPipeEventImpl> pImpl_;
};

class TclmonDaemonEvent : public DaemonEvent
{
public:
    TclmonDaemonEvent(const char* cmd, int suffix);
    virtual ~TclmonDaemonEvent() {}
    virtual void init();
private:
    std::shared_ptr<void> pImpl_;
};

class AqDaemonEvent
    : public DaemonEvent
{
public:
    AqDaemonEvent(const std::string& aqName);
    virtual ~AqDaemonEvent();
    virtual void init();
private:
    std::shared_ptr<void> pImpl_;
};

// throw from daemon task to shutdown daemon
// DaemonEvent catches this inside DaemonEvent::runTask
// and calls system_ios::Instance().stop()
struct shutdown_ioservice_exception
{
    shutdown_ioservice_exception(){}
};

} // namespace ServerFramework
namespace comtech = ServerFramework;

#endif /* DAEMON_EVENT_H */

