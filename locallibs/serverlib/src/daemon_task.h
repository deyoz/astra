#ifndef DAEMON_TASK_H
#define DAEMON_TASK_H

#include <vector>
#include <list>
#include <queue>
#include <memory>
#include <functional>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "exception.h"
#include "monitor_ctl.h"

namespace ServerFramework 
{

class DaemonTaskTraits
{
    typedef std::function<void()> f;
    typedef std::list<f>::const_iterator fun_itr;
    std::list<f> before_;
    std::list<f> after_;
    std::list<f> fail_;
    std::list<f> finally_;
public:
    DaemonTaskTraits(f const &b, f const &a, f const &r, f const &fin) {
        before_.push_back(b);
        after_.push_back(a);
        fail_.push_back(r);
        finally_.push_back(fin);
    }
    void beforeTask() {
        for(fun_itr it=before_.begin();it!=before_.end();++it)
            if (*it)
                (*it)();
    }
    void afterTask() {
        for(fun_itr it=after_.begin();it!=after_.end();++it)
            if(*it)
                (*it)();
    }
    void afterFail() {
        for(fun_itr it=fail_.begin();it!=fail_.end();++it)
            if(*it)
                (*it)();
    }
    void finally() {
        for(fun_itr it=finally_.begin();it!=finally_.end();++it)
            if(*it)
                (*it)();
    }
    DaemonTaskTraits addBefore(f const &func) {
        before_.push_back(func);
        return *this;
    }
    DaemonTaskTraits addAfter(f const &func) {
        after_.push_back(func);
        return *this;
    }
    DaemonTaskTraits addAfterFail(f const &func) {
        fail_.push_back(func);
        return *this;
    }
    DaemonTaskTraits addFinally(f const &func) {
        finally_.push_back(func);
        return *this;
    }

    static DaemonTaskTraits Nothing();
    static DaemonTaskTraits OracleAndHooks();
    static DaemonTaskTraits MainAndHooks();
    static DaemonTaskTraits OracleOnly();
    static DaemonTaskTraits HooksOnly();
};

class DaemonEvent;

struct EnvHolder
{
    virtual ~EnvHolder() {}
};

// базовый тип для работы с текстами пинков (buff и buff_sz в ф-ции run)
class BaseDaemonTask
{
public:
    BaseDaemonTask(DaemonTaskTraits const &t):traits_(t) {}
    virtual ~BaseDaemonTask() {}

    DaemonTaskTraits traits() { return traits_; }
    virtual std::string name();
    virtual int run(const char* buff, size_t buff_sz, boost::posix_time::ptime const &) = 0;
    virtual void monitorRequest();
    virtual size_t timeout() const;
    virtual bool doNeedRepeat() { return false; }
    virtual EnvHolder* env_saver() { return NULL; }

private:
    DaemonTaskTraits traits_;
};

// самый обычный тип, наверняка вам нужен именно он
class DaemonTask 
    : public BaseDaemonTask
{
public:
    DaemonTask(DaemonTaskTraits const &t) : BaseDaemonTask(t) {}
    virtual ~DaemonTask(){}
    
    virtual int run(const boost::posix_time::ptime&) = 0;
    virtual int run(const char*, size_t, boost::posix_time::ptime const & dt) override {
        return this->run(dt);
    }
};

// циклически повторяющаяся задача, пока не кончается список данных
template<typename Data>
class CyclicDaemonTask
    : public DaemonTask
{
public:
    CyclicDaemonTask(DaemonTaskTraits const& t) : DaemonTask(t) {}

    using DaemonTask::run;
    virtual ~CyclicDaemonTask() {}
    // special non-virtual, to prevent overriding this function
    int run(const boost::posix_time::ptime& dt) override final {
        needRepeat_ = false;
        if (queue_.empty()) {
            if (fillQueueFunc_) {
                fillQueueFunc_(dt, queue_);
            } else {
                throw comtech::Exception( "CyclicDaemonTask fillQueueFunc_ not defined!" );
            }
            if (queue_.empty()) {
                return 0;
            }
            monitor_idle_zapr(1);
        }
        Data data(queue_.front());
        queue_.pop();
        needRepeat_ = (queue_.empty()) ? false : true;
        const int res = run(dt, data, queue_.empty());
        monitor_idle_zapr(1);
        return res;
    }
    // special non-virtual, to prevent overriding this function
    bool doNeedRepeat() override final { return needRepeat_; }
    virtual void monitorRequest() override { }

    virtual int run(const boost::posix_time::ptime&, const Data&, bool endOfData) = 0;

protected:
    std::function<void (const boost::posix_time::ptime& dt, std::queue<Data> &queue)> fillQueueFunc_;

private:
    std::queue<Data> queue_;
    bool needRepeat_;
};

typedef std::shared_ptr<BaseDaemonTask> DaemonTaskPtr;

} // namespace ServerFramework
namespace comtech = ServerFramework;

#endif /* DAEMON_TASK_H */

