#ifndef _SERVERLIB_DISPATCHER_H_
#define _SERVERLIB_DISPATCHER_H_

#include <stdexcept>
#include <deque>
#include <functional>
#include <fstream>
#include <memory>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>

#include "daemon_event.h"
#include "blev.h"
#include "monitor_ctl.h"
#include "tclmon.h"
#include "daemon_impl.h"

#define NICKNAME "MIXA"
#include "slogger.h"

namespace Dispatcher {

typedef boost::asio::local::stream_protocol::socket WorkerSocket;
typedef boost::asio::local::stream_protocol::socket SignalSocket;

//-----------------------------------------------------------------------
template<typename K>
class WorkerMsg
{
public:
    WorkerMsg(const uint64_t bi /*balancer id*/,
              const K& clientConnectionId,
              std::vector<uint8_t>& head,
              std::vector<uint8_t>& data) :
        blncrId_(bi),
        cltConnId_(clientConnectionId),
        headSize_(htonl(head.size())),
        dataSize_(htonl(data.size())),
        reqHead_(),
        reqData_(),
        ansHead_(),
        ansData_(),
        time_(std::time(0)),
        recall_(false)
    {
        reqHead_.swap(head);
        reqData_.swap(data);
    }
    //-----------------------------------------------------------------------
    WorkerMsg(const WorkerMsg& ) = delete;
    WorkerMsg& operator=(const WorkerMsg& ) = delete;
    //-----------------------------------------------------------------------
    WorkerMsg(WorkerMsg&& ) = delete;
    WorkerMsg& operator=(WorkerMsg&& ) = delete;
    //-----------------------------------------------------------------------
    //for async_write()
    const std::vector<boost::asio::const_buffers_1>& requestToBuffers();
    //-----------------------------------------------------------------------
    //for async_read()
    const std::vector<boost::asio::mutable_buffers_1>& answerSizeToBuffers()
    {
        static std::vector<boost::asio::mutable_buffers_1> v;

        v.clear();
        v.reserve(2);
        v.emplace_back(&headSize_, sizeof(headSize_));
        v.emplace_back(&dataSize_, sizeof(dataSize_));

        return v;
    }
    //-----------------------------------------------------------------------
    //for async_read()
    const std::vector<boost::asio::mutable_buffers_1>& answerToBuffers()
    {
        static std::vector<boost::asio::mutable_buffers_1> v;

        v.clear();
        v.reserve(2);
        v.emplace_back(ansHead_.data(), ansHead_.size());
        v.emplace_back(ansData_.data(), ansData_.size());

        return v;
    }
    //-----------------------------------------------------------------------
    std::vector<uint8_t>& requestHeader() {
        return reqHead_;
    }
    //-----------------------------------------------------------------------
    std::vector<uint8_t>& requestData() {
        return reqData_;
    }
    //-----------------------------------------------------------------------
    void requestResetSize() {
        dataSize_ = htonl(reqData_.size());
    }
    //-----------------------------------------------------------------------
    std::vector<uint8_t>& answerHeader() {
        return ansHead_;
    }
    //-----------------------------------------------------------------------
    std::vector<uint8_t>& answerData() {
        return ansData_;
    }
    //-----------------------------------------------------------------------
    void resizeAnswer() {
        ansHead_.resize(ntohl(headSize_));
        ansData_.resize(ntohl(dataSize_));
    }
    //-----------------------------------------------------------------------
    const K& clientConnectionId() {
        return cltConnId_;
    }
    //-----------------------------------------------------------------------
    const uint64_t& balancerId() {
        return blncrId_;
    }
    //-----------------------------------------------------------------------
    std::time_t createTime() {
        return time_;
    }
    //-----------------------------------------------------------------------
    bool recall() {
        return recall_;
    }
    //-----------------------------------------------------------------------
    void recall(const bool value) {
        recall_ = value;
    }
    //-----------------------------------------------------------------------

private:
    const uint64_t blncrId_;
    const K cltConnId_;
    uint32_t headSize_;
    uint32_t dataSize_;
    std::vector<uint8_t> reqHead_;
    std::vector<uint8_t> reqData_;
    std::vector<uint8_t> ansHead_;
    std::vector<uint8_t> ansData_;
    std::time_t time_;
    bool recall_;
};
//-----------------------------------------------------------------------
class Signal
{
public:
    explicit Signal() :
        size_(0),
        data_()
    { }

    Signal(const Signal& ) = delete;
    Signal& operator=(const Signal& ) = delete;

    Signal(Signal&& ) = delete;
    Signal& operator=(Signal&& ) = delete;

    boost::asio::mutable_buffers_1 sizeToBuffer() {
        return boost::asio::mutable_buffers_1(&size_, sizeof(size_));
    }

    void resizeData() {
        data_.resize(ntohl(size_));
    }

    boost::asio::mutable_buffers_1 dataToBuffer() {
        return boost::asio::mutable_buffers_1(data_.data(), data_.size());
    }

    const std::vector<uint8_t>& data() {
        return data_;
    }

private:
    uint32_t size_;
    std::vector<uint8_t> data_;
};
//-----------------------------------------------------------------------
template<typename T>
class Connection
{
public:
    explicit Connection(boost::asio::io_service& io_s) :
        socket_(io_s)
    { }

    Connection(const Connection& ) = delete;
    Connection& operator=(const Connection& ) = delete;

    Connection(Connection&& ) = delete;
    Connection& operator=(Connection&& ) = delete;

    ~Connection() {
        boost::system::error_code e;

        socket_.shutdown(T::shutdown_both, e);
        socket_.close(e);
    }

    T& socket() {
        return socket_;
    }
private:
    T socket_;
};
//-----------------------------------------------------------------------
template<typename T>
class Dispatcher
{
    typedef WorkerSocket::protocol_type WorkerProtocol;
    typedef WorkerProtocol::acceptor WorkerAcceptor;
    typedef Connection<WorkerSocket> WorkerConnection;
    typedef std::shared_ptr<WorkerConnection> WorkerConnectionPtr;

    typedef SignalSocket::protocol_type SignalProtocol;
    typedef SignalProtocol::acceptor SignalAcceptor;
    typedef Connection<SignalSocket> SignalConnection;
    typedef std::shared_ptr<SignalConnection> SignalConnectionPtr;

    typedef std::deque<WorkerConnectionPtr> WorkerList;
    typedef std::map<ServerFramework::MsgId,
                     std::pair<
                         std::shared_ptr<WorkerMsg<typename T::KeyType> >,
                         std::shared_ptr<boost::asio::deadline_timer>
                     >
            > WaitMsgStorage;
    typedef std::list<std::shared_ptr<WorkerMsg<typename T::KeyType> > > MessageQueue;
public:
    enum class WHAT_DROP_IF_QUEUE_FULL {FRONT = 0, BACK = 1};

public:
    Dispatcher(
            boost::asio::io_service& io_s,
            const int controlPipe,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            const char* workerSockName,
            const char* signalSockName,
            const uint16_t queueWarn,
            const uint16_t queueWarnLog,
            const uint16_t queueDrop,
            const bool queueFullTimeStamp,
            const size_t maxOpenConnections,
            const int msgExpTimeout) :
        service_(io_s),
        workerAcceptor_(io_s),
        signalAcceptor_(io_s),
        newWorkerConn_(new WorkerConnection(io_s)),
        newSignalConn_(new SignalConnection(io_s)),
        protocol_(io_s, headType, addr, port, *this, maxOpenConnections),
        idle_(),
        waitMsgMap_(),
        msgQueue_(),
        lastWriteTime_(std::chrono::steady_clock::now()),
        waitTime_(0),
        attemptAcceptWorkerCount_(ATTEMPT_ACCEPT_WORKER_COUNT),
        attemptAcceptSignalCount_(ATTEMPT_ACCEPT_SIGNAL_COUNT),
        control_(new ServerFramework::ControlPipeEvent()),
        queueTimer_(io_s),
        MESSAGE_EXPIRED_TIMEOUT(msgExpTimeout),
        queueTraits_(queueWarn, queueWarnLog, queueDrop, queueFullTimeStamp)
    {
        init(workerSockName, signalSockName);
    }
    //-----------------------------------------------------------------------
    Dispatcher(
            boost::asio::io_service& io_s,
            const int controlPipe,
            const uint8_t headType,
            const boost::optional<std::pair<std::string, uint16_t> >& addr,
            const boost::optional<std::pair<std::string, uint16_t> >& balancerAddr,
            const char* workerSockName,
            const char* signalSockName,
            const uint16_t queueWarn,
            const uint16_t queueWarnLog,
            const uint16_t queueDrop,
            const bool queueFullTimeStamp,
            const size_t maxOpenConnections,
            const time_t msgExpTimeout) :
        service_(io_s),
        workerAcceptor_(io_s),
        signalAcceptor_(io_s),
        newWorkerConn_(new WorkerConnection(io_s)),
        newSignalConn_(new SignalConnection(io_s)),
        protocol_(io_s, headType, addr, balancerAddr, *this, maxOpenConnections),
        idle_(),
        waitMsgMap_(),
        msgQueue_(),
        lastWriteTime_(std::chrono::steady_clock::now()),
        waitTime_(0),
        attemptAcceptWorkerCount_(ATTEMPT_ACCEPT_WORKER_COUNT),
        attemptAcceptSignalCount_(ATTEMPT_ACCEPT_SIGNAL_COUNT),
        control_(new ServerFramework::ControlPipeEvent()),
        queueTimer_(io_s),
        MESSAGE_EXPIRED_TIMEOUT(msgExpTimeout),
        queueTraits_(queueWarn, queueWarnLog, queueDrop, queueFullTimeStamp)
    {
        init(workerSockName, signalSockName);
    }
    //-----------------------------------------------------------------------
    Dispatcher(
            boost::asio::io_service& io_s,
            const int controlPipe,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            const char* workerSockName,
            const char* signalSockName,
            const uint16_t queueWarn,
            const uint16_t queueWarnLog,
            const uint16_t queueDrop,
            const bool queueFullTimeStamp,
            const std::string& certificateFileName,
            const std::string& privateKeyFileName,
            const std::string& dhParamsFileName,
            const time_t msgExpTimeout) :
        service_(io_s),
        workerAcceptor_(io_s),
        signalAcceptor_(io_s),
        newWorkerConn_(new WorkerConnection(io_s)),
        newSignalConn_(new SignalConnection(io_s)),
        protocol_(io_s, headType, addr, port, *this, certificateFileName, privateKeyFileName, dhParamsFileName),
        idle_(),
        waitMsgMap_(),
        msgQueue_(),
        lastWriteTime_(std::chrono::steady_clock::now()),
        waitTime_(0),
        attemptAcceptWorkerCount_(ATTEMPT_ACCEPT_WORKER_COUNT),
        attemptAcceptSignalCount_(ATTEMPT_ACCEPT_SIGNAL_COUNT),
        control_(new ServerFramework::ControlPipeEvent()),
        queueTimer_(io_s),
        MESSAGE_EXPIRED_TIMEOUT(msgExpTimeout),
        queueTraits_(queueWarn, queueWarnLog, queueDrop, queueFullTimeStamp)
    {
        init(workerSockName, signalSockName);
    }
    //-----------------------------------------------------------------------
    Dispatcher(
            boost::asio::io_service& io_s,
            const int controlPipe,
            const char* clientSockName,
            const char* workerSockName,
            const char* signalSockName,
            const uint16_t queueWarn,
            const uint16_t queueWarnLog,
            const uint16_t queueDrop,
            const bool queueFullTimeStamp,
            const time_t msgExpTimeout) :
        service_(io_s),
        workerAcceptor_(io_s),
        signalAcceptor_(io_s),
        newWorkerConn_(new WorkerConnection(io_s)),
        newSignalConn_(new SignalConnection(io_s)),
        protocol_(io_s, clientSockName, *this),
        idle_(),
        waitMsgMap_(),
        msgQueue_(),
        lastWriteTime_(std::chrono::steady_clock::now()),
        waitTime_(0),
        attemptAcceptWorkerCount_(ATTEMPT_ACCEPT_WORKER_COUNT),
        attemptAcceptSignalCount_(ATTEMPT_ACCEPT_SIGNAL_COUNT),
        control_(new ServerFramework::ControlPipeEvent()),
        queueTimer_(io_s),
        MESSAGE_EXPIRED_TIMEOUT(msgExpTimeout),
        queueTraits_(queueWarn, queueWarnLog, queueDrop, queueFullTimeStamp)
    {
        init(workerSockName, signalSockName);
    }
    //-----------------------------------------------------------------------
    Dispatcher(const Dispatcher& ) = delete;
    Dispatcher& operator=(const Dispatcher& ) = delete;
    //-----------------------------------------------------------------------
    Dispatcher(Dispatcher&& ) = delete;
    Dispatcher& operator=(Dispatcher&& ) = delete;
    //-----------------------------------------------------------------------
    ~Dispatcher()
    { }
    //-----------------------------------------------------------------------
    static bool proxy() {
        return false;
    }
    //-----------------------------------------------------------------------
    int run();

    void takeRequest(
            const uint64_t balancerId,
            const typename T::KeyType& clientConnectionId,
            const std::string& ,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull
    );

private:
    void init(const char* workerSockName, const char* signalSockName);

    uint32_t msgWaitTime();

    void workerAsyncAccept();

    void handleWorkerAccept(const boost::system::error_code& e);

    void workerAsyncWrite(const WorkerConnectionPtr& conn);

    void workerAsyncWriteNextMessage();

    void handleWorkerAsyncWrite(
            const WorkerConnectionPtr& conn,
            const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
            const boost::system::error_code& e);

    void handleWorkerReadSize(
            const WorkerConnectionPtr& conn,
            const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
            const boost::system::error_code& e);

    void handleWorkerReadAnswer(
            const WorkerConnectionPtr& conn,
            const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
            const boost::system::error_code& e);

    void signalAsyncAccept();

    void handleSignalAccept(const boost::system::error_code& e);

    void signalAsyncRead(const SignalConnectionPtr& conn);

    void handleReadSignalSize(
            const SignalConnectionPtr& conn,
            const std::shared_ptr<Signal>& signal,
            const boost::system::error_code& e);

   void handleReadSignalData(
            const SignalConnectionPtr& conn,
            const std::shared_ptr<Signal>& signal,
            const boost::system::error_code& e);

    void delayedHandleSignal(
            const std::shared_ptr<Signal>& signal,
            const std::shared_ptr<boost::asio::deadline_timer>& timer,
            const struct timeval& t,
            const boost::system::error_code& e);

    bool handleSignal(const std::vector<uint8_t>& signal, const struct timeval& tmstamp);

    void handleWorkerError(
            const char* functionName,
            const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
            const boost::system::error_code& e);

    void handleSignalError(
            const char* functionName,
            const boost::system::error_code& e);

    void waitMessageTimerExpire(std::weak_ptr<WorkerMsg<typename T::KeyType> >& w, const boost::system::error_code& e);

    void handleQueueTimerExpire(const boost::system::error_code& e);

private:
    static const int ATTEMPT_ACCEPT_WORKER_COUNT = 7;
    static const int ATTEMPT_ACCEPT_SIGNAL_COUNT = 7;

private:
    boost::asio::io_service& service_;
    WorkerAcceptor workerAcceptor_;
    SignalAcceptor signalAcceptor_;
    WorkerConnectionPtr newWorkerConn_;
    SignalConnectionPtr newSignalConn_;
    T protocol_;
    WorkerList idle_;
    WaitMsgStorage waitMsgMap_;
    MessageQueue msgQueue_;
    std::chrono::steady_clock::time_point lastWriteTime_;
    std::chrono::steady_clock::duration waitTime_;
    int attemptAcceptWorkerCount_;
    int attemptAcceptSignalCount_;
    std::unique_ptr<ServerFramework::ControlPipeEvent> control_;
    boost::asio::deadline_timer queueTimer_;
    const time_t MESSAGE_EXPIRED_TIMEOUT; //seconds
    const struct QueueTraits {
        QueueTraits(const uint16_t w, const uint16_t wl, const uint16_t d, const bool fullTmStamp) :
            warn_(w),
            warnLog_(wl),
            drop_(d),
            fullTimestamp_(fullTmStamp)
        { }

        uint16_t warn_;
        uint16_t warnLog_;
        uint16_t drop_;
        bool fullTimestamp_;
    } queueTraits_;
};
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::init(const char* workerSockName, const char* signalSockName)
{
    unlink(workerSockName);
    unlink(signalSockName);

    const WorkerProtocol::endpoint wep(workerSockName);
    const SignalProtocol::endpoint sep(signalSockName);

    workerAcceptor_.open(WorkerProtocol());
    workerAcceptor_.set_option(WorkerAcceptor::reuse_address(true));
    workerAcceptor_.bind(wep);
    workerAcceptor_.listen();

    signalAcceptor_.open(SignalProtocol());
    signalAcceptor_.set_option(SignalAcceptor::reuse_address(true));
    signalAcceptor_.bind(sep);
    signalAcceptor_.listen();

    queueTimer_.expires_from_now(boost::posix_time::seconds(MESSAGE_EXPIRED_TIMEOUT));
    queueTimer_.async_wait(boost::bind(&Dispatcher::handleQueueTimerExpire, this, boost::asio::placeholders::error));

    control_->init();
}
//-----------------------------------------------------------------------
template<typename T>
int Dispatcher<T>::run()
{
    workerAsyncAccept();
    signalAsyncAccept();
    ServerFramework::Run();

    return 0;
}
//-----------------------------------------------------------------------
template<typename T>
uint32_t Dispatcher<T>::msgWaitTime()
{
    using duration = std::chrono::duration<int64_t, std::ratio<1, 64> >;

    return msgQueue_.size()
        ? std::chrono::duration_cast<duration>(waitTime_).count()
        : 0;
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::workerAsyncAccept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (attemptAcceptWorkerCount_--) {
        workerAcceptor_.async_accept(newWorkerConn_->socket(),
                                     boost::bind(&Dispatcher::handleWorkerAccept,
                                                 this,
                                                 boost::asio::placeholders::error
                                     )
        );
    } else {
        LogTrace(TRACE0) << "Can't accept workers";
        service_.stop();
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleWorkerAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << newWorkerConn_->socket().native_handle();

    if (!e) {
        attemptAcceptWorkerCount_ = ATTEMPT_ACCEPT_WORKER_COUNT;
        workerAsyncWrite(newWorkerConn_);
    } else {
        LogError(STDLOG) << __FUNCTION__ << "(" << e << "): " << e.message() << "on socket " << newWorkerConn_->socket().native_handle();
    }
    newWorkerConn_.reset(new WorkerConnection(service_));
    workerAsyncAccept();
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::workerAsyncWrite(const WorkerConnectionPtr& conn)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (msgQueue_.empty()) {
        LogTrace(TRACE5) << "Connection " << conn->socket().native_handle() << " added to idle list";
        idle_.emplace_front(conn);

        return;
    }
    std::shared_ptr<WorkerMsg<typename T::KeyType> > msg = msgQueue_.front();

    boost::asio::async_write(conn->socket(),
            msg->requestToBuffers(),
            boost::bind(&Dispatcher::handleWorkerAsyncWrite,
                this,
                conn,
                msg,
                boost::asio::placeholders::error
            )
    );
    msgQueue_.pop_front();
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleWorkerAsyncWrite(const WorkerConnectionPtr& conn,
        const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        write_set_flag_type(0, 1, QUEPOT_LEVB, nullptr);
        boost::asio::async_read(conn->socket(),
                msg->answerSizeToBuffers(),
                boost::bind(&Dispatcher::handleWorkerReadSize,
                    this,
                    conn,
                    msg,
                    boost::asio::placeholders::error
                )
        );

        auto now = std::chrono::steady_clock::now();
        waitTime_ = now - lastWriteTime_;
        lastWriteTime_ = now;
    } else {
        msgQueue_.emplace_front(msg);
        LogTrace(TRACE0) << __FUNCTION__ << ": error  on " << conn->socket().native_handle() << ": ( " << e << ") - " << e.message();
        workerAsyncWriteNextMessage();
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleWorkerReadSize(const WorkerConnectionPtr& conn,
        const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        msg->resizeAnswer();
        boost::asio::async_read(conn->socket(),
                msg->answerToBuffers(),
                boost::bind(&Dispatcher::handleWorkerReadAnswer,
                    this,
                    conn,
                    msg,
                    boost::asio::placeholders::error
                )
        );
    } else {
        handleWorkerError(__FUNCTION__, msg, e);
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleWorkerReadAnswer(
        const WorkerConnectionPtr& conn,
        const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        //protocol_.answeringNow(...) должна вернуть значение таймаута, в течение которого нужно ждать
        //сигнала с тем же MsgId, что и у msg. Если возвращаемое значение равно нулю,
        //то это означает, что ответ готов  и ждать не нужно
        uint32_t timeout = protocol_.answeringNow(msg->answerHeader());

        if (timeout) {
            auto timer = std::make_shared<boost::asio::deadline_timer>(service_, boost::posix_time::seconds(timeout));
            waitMsgMap_.insert(std::make_pair(protocol_.msgIdFromHeader(msg->requestHeader()), std::make_pair(msg, timer)));
            timer->async_wait(boost::bind(
                        &Dispatcher::waitMessageTimerExpire,
                        this,
                        std::weak_ptr<WorkerMsg<typename T::KeyType> >(msg),
                        boost::asio::placeholders::error
                             )
            );
            workerAsyncWrite(conn);
        } else {
            protocol_.sendAnswer(msg->balancerId(), msg->clientConnectionId(), msg->answerHeader(),
                                 msg->answerData(), msgQueue_.size(), msgWaitTime());
            workerAsyncWrite(conn);
        }
    } else {
        handleWorkerError(__FUNCTION__, msg, e);
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::signalAsyncAccept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (attemptAcceptSignalCount_--) {
        signalAcceptor_.async_accept(newSignalConn_->socket(),
                                     boost::bind(&Dispatcher::handleSignalAccept,
                                                 this,
                                                 boost::asio::placeholders::error
                                    )
        );
    } else {
        LogTrace(TRACE0) << "Can't accept signal";
        service_.stop();
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleSignalAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (!e) {
        attemptAcceptSignalCount_ = ATTEMPT_ACCEPT_SIGNAL_COUNT;
        signalAsyncRead(newSignalConn_);
    } else {
        LogError(STDLOG) << __FUNCTION__ << "(" << e << "): " << e.message();
    }
    newSignalConn_.reset(new SignalConnection(service_));
    signalAsyncAccept();
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::signalAsyncRead(const SignalConnectionPtr& conn)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    std::shared_ptr<Signal> signal(new Signal());
    boost::asio::async_read(
            conn->socket(),
            signal->sizeToBuffer(),
            boost::bind(&Dispatcher::handleReadSignalSize,
                this, conn, signal, boost::asio::placeholders::error)
    );
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleReadSignalSize(
        const SignalConnectionPtr& conn,
        const std::shared_ptr<Signal>& signal,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        signal->resizeData();
        boost::asio::async_read(
                conn->socket(),
                signal->dataToBuffer(),
                boost::bind(&Dispatcher::handleReadSignalData,
                    this, conn, signal, boost::asio::placeholders::error)
        );
    } else {
        handleSignalError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleReadSignalData(
        const SignalConnectionPtr& conn,
        const std::shared_ptr<Signal>& signal,
        const boost::system::error_code& e)
{
    static const size_t MIN_SIGNAL_DATA_SIZE = 16;

    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().native_handle();
    if (!e) {
        if (signal->data().size() < MIN_SIGNAL_DATA_SIZE) {
            throw std::runtime_error("handleReadSignalData() : unbelievable msg size");
        }

        struct timeval tmstamp;
        gettimeofday( &tmstamp, 0 );

        if(!handleSignal(signal->data(), tmstamp)) // the remote center round-trip has been way faster then the worker send answer
        {
            auto timer = std::make_shared<boost::asio::deadline_timer>(service_, boost::posix_time::seconds(1));
            timer->async_wait(
                    boost::bind(&Dispatcher::delayedHandleSignal, this, signal, timer,
                                tmstamp, boost::asio::placeholders::error));
        }

        signalAsyncRead(conn);
    } else {
        handleSignalError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::delayedHandleSignal(
        const std::shared_ptr<Signal>& signal,
        const std::shared_ptr<boost::asio::deadline_timer>& timer,
        const struct timeval& t,
        const boost::system::error_code& e)
{
    static const int HANDLE_SIGNAL_TIMEOUT = 33;

    struct timeval tmstamp;
    gettimeofday( &tmstamp, 0 );

    LogTrace(TRACE5) << __FUNCTION__;

    if (e) {
        LogTrace(TRACE1) << __FUNCTION__ << " : delta tv_sec = " << tmstamp.tv_sec - t.tv_sec << ", error: " << e.message();

        return;
    }

    if(handleSignal(signal->data(), tmstamp)) {
        LogTrace(TRACE5) << __FUNCTION__ << " : handled signal with delta tv_ses = " << tmstamp.tv_sec - t.tv_sec;
    } else if(tmstamp.tv_sec < t.tv_sec + HANDLE_SIGNAL_TIMEOUT) {
        boost::system::error_code error;
        timer->expires_from_now(boost::posix_time::seconds(1), error);
        if (error) {
            const uint32_t* d = reinterpret_cast<const uint32_t*>(signal->data().data());
            LogError(STDLOG)
                << __FUNCTION__
                << " : expires_from_now failed with " << error.message().c_str()
                << " => unmatched signal { " << d[1] << ' ' << d[2] <<  ' ' << d[3] << " }";
        } else {
            LogTrace(TRACE5) << __FUNCTION__ << " : further requeueing delta tv_sec = " << tmstamp.tv_sec - t.tv_sec;
            timer->async_wait(boost::bind(&Dispatcher::delayedHandleSignal, this, signal, timer, t, boost::asio::placeholders::error));
        }
    } else {
        const uint32_t* d = reinterpret_cast<const uint32_t*>(signal->data().data());
        LogTrace(TRACE1) << __FUNCTION__ << " : unmatched signal { " << d[1] << ' ' << d[2] << ' ' << d[3] << " }";
    }
}
//-----------------------------------------------------------------------
template<typename T>
bool Dispatcher<T>::handleSignal(const std::vector<uint8_t>& signal, const struct timeval& tmstamp)
{
    LogTrace(TRACE5) << __FUNCTION__;

    const typename WaitMsgStorage::iterator i = waitMsgMap_.find(protocol_.msgIdFromSignal(signal));
    if (waitMsgMap_.end() != i) {
        i->second.second.reset();
        auto& msg = *i->second.first;
        msg.requestHeader() = msg.answerHeader();
        protocol_.makeSignalSendable(signal, msg.requestHeader(), msg.requestData());
        msg.requestResetSize();
        msg.recall(true);
        msgQueue_.emplace_front(i->second.first);
        waitMsgMap_.erase(i);
        workerAsyncWriteNextMessage();

        return true;
    } else {
        const uint32_t* d = reinterpret_cast<const uint32_t*>(signal.data());

        LogTrace(TRACE5) << __FUNCTION__
            << " : couldn't find matching for { " 
            << d[1] << ' ' << d[2] << ' ' << d[3]
            << " }";

        return false;
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::workerAsyncWriteNextMessage()
{
    //Если сообщений больше чем обработчиков, то отправить все сообщения сейчас не удастся,
    //аналогично, если обработчиков больше чем сообщений, то найти для всех "работу" не получится,
    //часть останется ожидать
    size_t maybeSend = std::min(msgQueue_.size(), idle_.size());

    LogTrace(TRACE5) << __FUNCTION__;

    if (!maybeSend) {
        LogTrace(TRACE5) << "Wait messages in queue: " << msgQueue_.size() << ", idling handlers: " << idle_.size();

        return;
    }

    for(WorkerList::iterator i = idle_.begin(); maybeSend--; i = idle_.erase(i)) {
        workerAsyncWrite(*i);
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleWorkerError(
        const char* functionName,
        const std::shared_ptr<WorkerMsg<typename T::KeyType> >& msg,
        const boost::system::error_code& e)
{
    const ServerFramework::MsgId& id = protocol_.msgIdFromHeader(msg->requestHeader());
    std::ostringstream os;

    os << '{' << id.b[0];
    for(int i = 1; i < ServerFramework::MsgId::BUF_SIZE; ++i) {
        os << ' ' << id.b[i];
    }
    os << '}';
    if((boost::asio::error::connection_reset == e) || (boost::asio::error::eof == e)) {
        LogTrace(TRACE0) << functionName << "(" << e << "): " << e.message();
    } else {
        LogError(STDLOG) << functionName << "(" << e << "): " << e.message();
    }

    LogTrace(TRACE0) << "Following message not handle:";
    LogTrace(TRACE0) << os.str();

    protocol_.rmConnectionId(msg->clientConnectionId());
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleSignalError(
        const char* functionName,
        const boost::system::error_code& e)
{
    if (boost::asio::error::eof == e) {
        LogTrace(TRACE1) << functionName << "(" << e << "): " << e.message();
    } else {
        LogError(STDLOG) << functionName << "(" << e << "): " << e.message();
    }
}

//-----------------------------------------------------------------------
//Content of input parameters head and data will be change
template<typename T>
void Dispatcher<T>::takeRequest(
        const uint64_t balancerId,
        const typename T::KeyType& clientConnectionId,
        const std::string& ,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << clientConnectionId;

    if ((queueTraits_.warn_ < msgQueue_.size()) && (msgQueue_.size() < queueTraits_.warnLog_)) {
        write_set_queue_size(msgQueue_.size(),queueTraits_.drop_, 0);
    } else if (queueTraits_.warnLog_ <= msgQueue_.size()){
        write_set_queue_size(msgQueue_.size(),queueTraits_.drop_,1);

        if (queueTraits_.fullTimestamp_) try {
            static std::ofstream file("que_full_timestamp.txt", std::ios::out|std::ios::app);

            file << std::time(0) << std::endl;
        } catch(const std::exception& e) {
            LogError(STDLOG) << "cannot work with QUEUE_FULL_TIMESTAMP_FILE";
        }

        if (queueTraits_.drop_ < msgQueue_.size()) {
            if (WHAT_DROP_IF_QUEUE_FULL::BACK == whatDropIfQueueFull) {
                protocol_.queueOverflow(balancerId, clientConnectionId, head, data, msgQueue_.size(), msgWaitTime());

                return;
            } else {
                protocol_.queueOverflow(
                        balancerId,
                        msgQueue_.front()->clientConnectionId(),
                        msgQueue_.front()->requestHeader(),
                        msgQueue_.front()->requestData(),
                        msgQueue_.size(),
                        msgWaitTime()
                );
                msgQueue_.pop_front();
            }
        }
    }

    if (msgQueue_.empty()) {
        lastWriteTime_ = std::chrono::steady_clock::now();
    }

    msgQueue_.emplace_back(std::make_shared<WorkerMsg<typename T::KeyType> >(balancerId, clientConnectionId, head, data));
    workerAsyncWriteNextMessage();
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::waitMessageTimerExpire(std::weak_ptr<WorkerMsg<typename T::KeyType> >& w, const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (e) {
        LogTrace(TRACE5)
            << __FUNCTION__
            << " error: "
            << e.message();
    } else if (std::shared_ptr<WorkerMsg<typename T::KeyType> > msg = w.lock()) {
        waitMsgMap_.erase(protocol_.msgIdFromHeader(msg->requestHeader()));
        protocol_.sendAnswer(msg->balancerId(), msg->clientConnectionId(), msg->answerHeader(),
                             msg->answerData(), msgQueue_.size(), msgWaitTime());
    }
}
//-----------------------------------------------------------------------
template<typename T>
void Dispatcher<T>::handleQueueTimerExpire(const boost::system::error_code& e)
{
    if (!e) {
        const std::time_t expiredTime = std::time(0) - MESSAGE_EXPIRED_TIMEOUT;
        typename MessageQueue::iterator tmp;
        for (typename MessageQueue::iterator i = msgQueue_.begin(); i != msgQueue_.end(); ) {
            if (!(*i)->recall() && ((*i)->createTime() < expiredTime)) {
                protocol_.timeoutExpired((*i)->balancerId(), (*i)->clientConnectionId(), (*i)->requestHeader(),
                                         (*i)->requestData(), msgQueue_.size(), msgWaitTime());
                tmp = i;
                ++i;
                msgQueue_.erase(tmp);
            } else {
                ++i;
            }
        }
        queueTimer_.expires_from_now(boost::posix_time::seconds(1));
        queueTimer_.async_wait(boost::bind(&Dispatcher::handleQueueTimerExpire, this, boost::asio::placeholders::error));
    } else {
        LogError(STDLOG) << __FUNCTION__ << " (" << e << "): " << e.message();
    }
}
//-----------------------------------------------------------------------
class Worker
{
public:
    Worker(boost::asio::io_service& io_s, int controlPipe, const char* dispatcherSocketName);

    Worker(const Worker& ) = delete;
    Worker& operator=(const Worker& ) = delete;

    Worker(Worker&& ) = delete;
    Worker& operator=(Worker&& ) = delete;

    ~Worker();

    int run();

private:
    void connect();

    void handleConnect(const boost::system::error_code& e);

    void readSize();

    void handleReadSize(const boost::system::error_code& e);

    void handleReadRequest(const boost::system::error_code& e);

    void handleError(const char* functionName, const boost::system::error_code& e);

    const std::vector<boost::asio::mutable_buffers_1>& requestToBuffers();

    const std::vector<boost::asio::const_buffers_1>& answerToBuffers();

private:
    static const int ATTEMPT_CONNECT_COUNT = 7;

private:
    boost::asio::io_service& service_;
    WorkerSocket socket_;
    WorkerSocket::endpoint_type ep_;
    uint32_t headSize_;
    uint32_t dataSize_;
    std::vector<boost::asio::mutable_buffers_1> sizeBuffers_;
    std::vector<uint8_t> reqHead_;
    std::vector<uint8_t> reqData_;
    std::vector<uint8_t> ansHead_;
    std::vector<uint8_t> ansData_;
    int attemptConnectCount_;
    std::unique_ptr<ServerFramework::ControlPipeEvent> control_;
};

#undef NICKNAME

} //namespace Dispatcher

#endif //_SERVERLIB_DISPATCHER_H_
