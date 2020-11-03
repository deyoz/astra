#include <typeinfo>
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "tclmon.h"
#include "mes.h"
#include "tcl_utils.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "profiler.h"
#include "exception.h"
#include "ourtime.h"
#include "oci8.h"
#include "monitor_ctl.h"
#include "perfom.h"
#include "posthooks.h"
#include "daemon_impl.h"
#include "daemon_event.h"
#include "deadlock_exception.h"
#include "new_daemon.h"

static int runTask__(const ServerFramework::DaemonTaskPtr& pTask, const char* buff, size_t buffSz, const boost::posix_time::ptime& time)
{
    const size_t timeout = pTask->timeout();
    monitor_beg_work();
    if (timeout) {
        write_set_flag(timeout, 0);
    } else {
        write_set_flag(0, 0);
    }
    write_set_cur_req(pTask->name().c_str());

    std::unique_ptr<ServerFramework::EnvHolder> p(pTask->env_saver());
    pTask->traits().beforeTask();
    const boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    const int res = pTask->run(buff, buffSz, time);
    const boost::posix_time::ptime stop = boost::posix_time::microsec_clock::local_time();

    const double workingTime = double((stop-start).total_milliseconds())/1000;
    if (timeout && (workingTime > (timeout * 2. / 3.))) {
        LogError(STDLOG) << "Task " << pTask->name() <<" took "<< workingTime << " seconds, timeout: " << timeout;
    } else {
        LogTrace(TRACE0) << "Task " << pTask->name() <<" took "<< workingTime << " seconds";
    }
    if (res != 0) {
        pTask->monitorRequest();
        return res;
    }
    pTask->traits().afterTask();
    pTask->monitorRequest();
    return res;
}

static int callRunTaskAndCatch(const ServerFramework::DaemonTaskPtr& pTask,
        const char* buff, size_t buffSz,
        const boost::posix_time::ptime& time)
{
    ServerFramework::DaemonTaskTraits traits = pTask->traits();
    int res = 0;
    try {
        res = runTask__(pTask, buff, buffSz, time);
        if (res != 0) {
            LogTrace(TRACE0) << pTask->name().c_str() << " err=" << res;
            traits.afterFail();
        }
    } catch (const comtech::Exception &e) {
        LogError(STDLOG) << pTask->name().c_str() << " " << e.what();
        e.print(TRACE1);
        traits.afterFail();
    }
    return res;
}

static int callRunTaskWithoutCatch(const ServerFramework::DaemonTaskPtr& pTask,
        const char* buff, size_t buffSz,
        const boost::posix_time::ptime& time)
{
    ServerFramework::DaemonTaskTraits traits = pTask->traits();
    const int res = runTask__(pTask, buff, buffSz, time);
    if (res != 0) {
        LogTrace(TRACE0) << pTask->name().c_str() << " err=" << res;
        traits.afterFail();
    }
    return res;
}

static int runTask_(const ServerFramework::DaemonTaskPtr& pTask,
        const char* buff, size_t buffSz,
        const boost::posix_time::ptime& time, const std::string& name, bool catchException)
{
    InitLogTime(name.c_str());
    PerfomInit();
    ServerFramework::DaemonTaskTraits traits = pTask->traits();
    int ll = 0;
    int res = 0;
    static const std::string noCatch(getenv("XP_NO_CATCH") == nullptr ? "" : getenv("XP_NO_CATCH"));
    try {
        if (!catchException || noCatch == "1") {
            res = callRunTaskWithoutCatch(pTask, buff, buffSz, time);
        } else {
            res = callRunTaskAndCatch(pTask, buff, buffSz, time);
        }
    } catch (const ServerFramework::shutdown_ioservice_exception& e) {
        LogTrace(TRACE0) << "Task " << pTask->name() << " calls stop daemon";
        ServerFramework::system_ios::Instance().stop();
    }
    if (ll) {
        setLogLevelExternal(-1);
        LogTrace(TRACE5) << "logLevel=" << ll;
    }
    monitor_idle();
    traits.finally();
    emptyHookTables();
    return res;
}

namespace ServerFramework
{

void execTclCommandStr(const char* cmdStr, size_t cmdLen)
{
    Tcl_Obj* pobj = Tcl_NewStringObj(cmdStr, cmdLen);
    if (pobj == NULL) {
        ProgTrace(TRACE1, "Tcl_NewStringObj failed: cmdLen=%zd cmdStr=%.*s", cmdLen, static_cast<int>(cmdLen), cmdStr);
    } else {
        const int ret = Tcl_EvalObjEx(getTclInterpretator(), pobj, TCL_EVAL_GLOBAL);
        ProgTrace(TRACE5, "Tcl_EvalObjEx exited with code %d", ret);
        const char* resStr = Tcl_GetStringResult(getTclInterpretator());
        ProgTrace(TRACE5, "result: %s", resStr);
    }
}

static void postWrap(const ServerFramework::DaemonTaskPtr& pTask,
        const char* buff, size_t buffSz,
        const boost::posix_time::ptime& time, const std::string& name)
{
    ::runTask_(pTask, buff, buffSz, time, name, true);
    if (pTask->doNeedRepeat()) {
        ServerFramework::system_ios::Instance().post(boost::bind(&postWrap, pTask, static_cast<const char*>(NULL), 0, time, name));
    }
}

#ifdef XP_TESTING
void runTask(const DaemonTaskPtr& pTask, const boost::posix_time::ptime& now)
{
    do {
        ::runTask_(pTask, static_cast<const char*>(NULL), 0, now, "TestEvent", true);
    } while (pTask->doNeedRepeat());
}

void runTaskSirena(const DaemonTaskPtr& pTask, const boost::posix_time::ptime& now, int expectedErr, bool catchException)
{
    do {
        int err = ::runTask_(pTask, static_cast<const char*>(NULL), 0, now, "TestEvent", catchException);
        if(err != expectedErr)
            throw comtech::Exception(STDLOG, __FUNCTION__,
                    "excpected err = " + std::to_string(expectedErr) +
                    " got err: " + std::to_string(err));
    } while (pTask->doNeedRepeat());
}

#endif // XP_TESTING

/***   DaemonEvent   ***/

DaemonEvent::DaemonEvent()
{
}

void DaemonEvent::addTask(DaemonTaskPtr pTask)
{
    tasks_.push_back(pTask);
}

void DaemonEvent::setName(const std::string& name)
{
    name_ = name;
}

void DaemonEvent::runTasks(const boost::posix_time::ptime& workTime)
{
    for (DaemonTasks::iterator it = tasks_.begin(); it != tasks_.end(); ++it) {
        postWrap(*it, NULL, 0, workTime, name_);
    }
}

void DaemonEvent::runTasks()
{
    runTasks(NULL, 0);
}

void DaemonEvent::runTasks(const char* buff, size_t buffSz)
{
    const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    for (DaemonTasks::iterator it = tasks_.begin(); it != tasks_.end(); ++it) {
        postWrap(*it, buff, buffSz, now, name_);
    }
}

/***  TimerDaemonEvent  ***/

class TimerDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    TimerDaemonEventImpl(size_t seconds, DaemonEvent& event)
        : timeout_(seconds), io_service_(system_ios::Instance()), event_(event)
    {}
    virtual ~TimerDaemonEventImpl() {}
    void init() {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
        handle();
    }
private:
    void handle() {
        reset_timeout();
        event_.runTasks();
    }
    void reset_timeout() {
        timer_->expires_from_now(boost::posix_time::seconds(timeout_));
        timer_->async_wait(boost::bind(&TimerDaemonEventImpl::handle, this));
    }

    size_t timeout_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    DaemonEvent& event_;
};

TimerDaemonEvent::TimerDaemonEvent(size_t seconds)
    : pImpl_(new TimerDaemonEventImpl(seconds, *this))
{
}

void TimerDaemonEvent::init()
{
    pImpl_->init();
}

/***  TimerAfterDaemonEvent  ***/
class TimerAfterDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    TimerAfterDaemonEventImpl(size_t seconds, DaemonEvent& event)
        : timeout_(seconds), io_service_(system_ios::Instance()), event_(event)
    {}
    virtual ~TimerAfterDaemonEventImpl() {}
    void init() {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
        handle();
    }
private:
    void handle() {
        event_.runTasks();
        reset_timeout();
    }
    void reset_timeout() {
        timer_->expires_from_now(boost::posix_time::seconds(timeout_));
        timer_->async_wait(boost::bind(&TimerAfterDaemonEventImpl::handle, this));
    }

    size_t timeout_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    DaemonEvent& event_;
};

TimerAfterDaemonEvent::TimerAfterDaemonEvent(size_t seconds)
    : pImpl_(new TimerAfterDaemonEventImpl(seconds, *this))
{
}

void TimerAfterDaemonEvent::init()
{
    pImpl_->init();
}

/***  TimerMilliSecAfterDaemonEvent  ***/
class TimerMilliSecAfterDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    TimerMilliSecAfterDaemonEventImpl(size_t milliseconds, DaemonEvent& event)
        : timeout_(milliseconds), io_service_(system_ios::Instance()), event_(event)
    {}
    virtual ~TimerMilliSecAfterDaemonEventImpl() {}
    void init() {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
        handle();
    }
private:
    void handle() {
        event_.runTasks();
        reset_timeout();
    }
    void reset_timeout() {
        timer_->expires_from_now(boost::posix_time::milliseconds(timeout_));
        timer_->async_wait(boost::bind(&TimerMilliSecAfterDaemonEventImpl::handle, this));
    }

    size_t timeout_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    DaemonEvent& event_;
};

TimerMilliSecAfterDaemonEvent::TimerMilliSecAfterDaemonEvent(size_t milliseconds)
    : pImpl_(new TimerMilliSecAfterDaemonEventImpl(milliseconds, *this))
{
}

void TimerMilliSecAfterDaemonEvent::init()
{
    pImpl_->init();
}

/***  TimerMicroSecAfterDaemonEvent  ***/
class TimerMicroSecAfterDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    TimerMicroSecAfterDaemonEventImpl(size_t microseconds, DaemonEvent& event)
        : timeout_(microseconds), io_service_(system_ios::Instance()), event_(event)
    {}
    virtual ~TimerMicroSecAfterDaemonEventImpl() {}
    void init() {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
        handle();
    }
private:
    void handle() {
        event_.runTasks();
        reset_timeout();
    }
    void reset_timeout() {
        timer_->expires_from_now(boost::posix_time::microseconds(timeout_));
        timer_->async_wait(boost::bind(&TimerMicroSecAfterDaemonEventImpl::handle, this));
    }

    size_t timeout_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    DaemonEvent& event_;
};

TimerMicroSecAfterDaemonEvent::TimerMicroSecAfterDaemonEvent(size_t microseconds)
    : pImpl_(new TimerMicroSecAfterDaemonEventImpl(microseconds, *this))
{
}

void TimerMicroSecAfterDaemonEvent::init()
{
    pImpl_->init();
}

/*** ScheduledDaemonEvent ***/
class ScheduleDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    ScheduleDaemonEventImpl(size_t seconds, ScheduleDaemonEvent& event)
        : timeout_(seconds), io_service_(system_ios::Instance()), event_(event)
    {}
    virtual ~ScheduleDaemonEventImpl() {}
    void init() {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
        handle();
    }
private:
    void handle() {
        reset_timeout();
        event_.runTasks();
    }
    void reset_timeout() {
        timer_->expires_from_now(boost::posix_time::seconds(timeout_));
        timer_->async_wait(boost::bind(&ScheduleDaemonEventImpl::handle, this));
    }

    size_t timeout_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    ScheduleDaemonEvent& event_;
};

ScheduleDaemonEvent::ScheduleDaemonEvent(size_t seconds)
{
    pImpl_.reset(new ScheduleDaemonEventImpl(seconds, *this));
}

void ScheduleDaemonEvent::init()
{
    ScheduleDaemonEventImpl* p = reinterpret_cast<ScheduleDaemonEventImpl*>(pImpl_.get());
    p->init();
}
void ScheduleDaemonEvent::runTasks()
{
    const boost::posix_time::ptime today = boost::posix_time::second_clock::local_time();
    if (checkSchedule(today)) {
        DaemonEvent::runTasks();
        updateSchedule(today);
    }
}

/*** ControlPipeEvent ***/
class ControlPipeEventImpl
{
public:
    ControlPipeEventImpl(ControlPipeEvent& event, boost::asio::io_service& controlService, boost::asio::io_service& systemService)
        : controlSock_(controlService), systemSock_(systemService), event_(event)
    {}

    virtual ~ControlPipeEventImpl() {}

    void init(int pipe_descriptor) {
        controlSock_.assign(boost::asio::local::stream_protocol(), pipe_descriptor);
        checkControlSock();
        systemSock_.assign(boost::asio::local::stream_protocol(), pipe_descriptor);
        checkSystemSock(boost::system::error_code());
    }

private:
    void checkSystemSock(const boost::system::error_code& e) {
        if (!e) {
            LogTrace(TRACE5) << __FUNCTION__;
            systemSock_.async_read_some(boost::asio::null_buffers(),
                    boost::bind(&ControlPipeEventImpl::checkSystemSock, this, boost::asio::placeholders::error));
        } else {
            boost::system::error_code error;
            systemSock_.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both, error);
        }
    }

    void checkControlSock() {
        controlSock_.async_read_some(boost::asio::null_buffers(),
                boost::bind(&ControlPipeEventImpl::tclmon_handle, this, boost::asio::placeholders::error));
    }

    void tclmon_handle(boost::system::error_code e) {
        if (!e) {
            LogTrace(TRACE5) << __FUNCTION__ << " time=" << time(NULL);

            msg_ = {};
            boost::asio::read(controlSock_, boost::asio::buffer(reinterpret_cast<char*>(&msg_.mes_head), sizeof(msg_.mes_head)),
                              boost::asio::transfer_all(), e);
            if (e) {
                handleError(e);
                return;
            }

            boost::asio::read(controlSock_, boost::asio::buffer(msg_.mes_text, msg_.mes_head.len_text), boost::asio::transfer_all(), e);

            if (e) {
                handleError(e);
                return;
            }

            event_.handleControlMsg(msg_.mes_text, msg_.mes_head.len_text);
            checkControlSock();
        } else {
            handleError(e);
        }
    }

    void handleError(boost::system::error_code e) {
        LogError(STDLOG) << "tclmon socket error :: " << e;
        controlSock_.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both, e);
    }

private:
    boost::asio::local::stream_protocol::socket controlSock_;
    boost::asio::local::stream_protocol::socket systemSock_;
    _Message_ msg_;
    ControlPipeEvent& event_;
};

ControlPipeEvent::ControlPipeEvent()
{
    pImpl_.reset(new ControlPipeEventImpl(*this, control_pipe_ios::Instance(), system_ios::Instance()));
}

void ControlPipeEvent::init()
{
    pImpl_->init(getControlPipe());
}

void ControlPipeEvent::handleControlMsg(const char* buff, size_t buff_sz)
{
    ProgTrace(TRACE5, "handleControlMsg: sz=%zd buff=%.*s", buff_sz, static_cast<int>(buff_sz), buff);
    if (isProfilingCmd(buff, buff_sz)) {
        handle_profiling_cmd(buff, buff_sz);
    } else {
        execTclCommandStr(buff, buff_sz);
    }
}

/*** TclmonDaemon ***/
class TclmonDaemonEventImpl
{
    enum State {WaitForMesHead, WaitForMesText};
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    TclmonDaemonEventImpl(const char* cmd, int suffix, TclmonDaemonEvent& event)
        : io_service_(system_ios::Instance())
          , skipKicks_(readIntFromTcl("SKIP_TCLMON_KICKS", 100))
          , n_(0)
          , event_(event)
    {
        const std::string name = get_signalsock_name(getTclInterpretator(), Tcl_NewStringObj(cmd,-1), 0 , suffix);
        ::unlink(name.c_str());
        signal_sock_.reset(new boost::asio::local::datagram_protocol::socket(io_service_,
                                            boost::asio::local::datagram_protocol::endpoint(name)));
    }
    virtual ~TclmonDaemonEventImpl()
    {
        boost::system::error_code ec;
        signal_sock_->close(ec);
    }
    void init() {
        ProgTrace(TRACE1 , "Daemon init complete - planning work");
        signal_sock_->async_receive(boost::asio::buffer(signal_msg, max_signal_msg_),
                boost::bind(&TclmonDaemonEventImpl::signal_handle, this,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        event_.runTasks();
    }
private:
    void signal_handle(const boost::system::error_code& e, size_t bytes_transferred) {
        if (!e) {
            LogTrace(TRACE5) << "bytes_transferred=" << bytes_transferred;
            size_t sz = signal_sock_->available();
            if (!sz || n_ >= skipKicks_) {
                n_ = 0;
                event_.runTasks(signal_msg, bytes_transferred);
            }
            ++n_;
            signal_sock_->async_receive(boost::asio::buffer(signal_msg, max_signal_msg_), 
                    boost::bind(&TclmonDaemonEventImpl::signal_handle, this,
                        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        } else {
            LogError(STDLOG) << "signal socket error" << e;
        }
    }

    boost::asio::io_service& io_service_;
    boost::shared_ptr<boost::asio::local::datagram_protocol::socket> signal_sock_;
    //boost::asio::ip::udp::socket signal_sock_;
    const size_t skipKicks_;
    size_t n_;

    static const size_t max_tclmon_msg_ = sizeof(_Message_);
    static const size_t max_signal_msg_ = 65000;
    char signal_msg[max_signal_msg_];
    TclmonDaemonEvent& event_;
};

TclmonDaemonEvent::TclmonDaemonEvent(const char* cmd, int suffix)
    : pImpl_(new TclmonDaemonEventImpl(cmd, suffix, *this))
{
}

void TclmonDaemonEvent::init()
{
    TclmonDaemonEventImpl* p = reinterpret_cast<TclmonDaemonEventImpl*>(pImpl_.get());
    p->init();
}

/***  AqDaemonEvent  ***/
class AqDaemonEventImpl;
class AqEventCallback
    : public OciCpp::AqEventCallback
{
public:
    AqEventCallback(AqDaemonEventImpl* pAq)
        : pAq_(pAq)
    {}
    virtual ~AqEventCallback() {}
    virtual void onEvent();
private:
    AqDaemonEventImpl* pAq_;
};

class AqDaemonEventImpl
{
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer_t;
public:
    AqDaemonEventImpl(const std::string& aqName, DaemonEvent& event)
        : aqName_(aqName), io_service_(system_ios::Instance()), event_(event), cb_(this)
    {
        timer_.reset(new boost::asio::deadline_timer(io_service_));
    }
    virtual ~AqDaemonEventImpl() {
        ProgTrace(TRACE5, "~AqDaemonEventImpl");
    }
    void init() {
        OciCpp::Oci8Session& sess = OciCpp::Oci8Session::instance(STDLOG);
        sess.setAqCallback(aqName_, &cb_);
        timer_handle();
        handle();
    }
    void post() {
        io_service_.post(boost::bind(&AqDaemonEventImpl::handle, this));
    }
private:
    void handle() {
        ProgTrace(TRACE5, "AqDaemonEventImpl handle");
        event_.runTasks();
    }
    /**
     * we need timer handle to do not stop io_service.run()
     **/
    void timer_handle() {
        timer_->expires_from_now(boost::posix_time::seconds(120));
        timer_->async_wait(boost::bind(&AqDaemonEventImpl::timer_handle, this));
    }
    
    const std::string aqName_;
    boost::asio::io_service& io_service_;
    timer_t timer_;
    DaemonEvent& event_;
    AqEventCallback cb_;
};

void AqEventCallback::onEvent()
{
    ProgTrace(TRACE5, "AqEventCallback onMessage");
    pAq_->post();
}

AqDaemonEvent::AqDaemonEvent(const std::string& aqName)
{
    pImpl_.reset(new AqDaemonEventImpl(aqName, *this));
}
AqDaemonEvent::~AqDaemonEvent()
{
    ProgTrace(TRACE5, "~AqDaemonEvent");
}

void AqDaemonEvent::init()
{
    AqDaemonEventImpl* p = reinterpret_cast<AqDaemonEventImpl*>(pImpl_.get());
    ProgTrace(TRACE5, "AqDaemonEvent::init: pImpl_=%p", p);
    p->init();
}
} // namespace ServerFramework

