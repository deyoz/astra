
#include <arpa/inet.h>
#include <time.h>

#include <boost/random.hpp>

#include "dispatcher.h"
#include "dispatcher_frontend.h"
#include "blev.h"
#include "proc_c.h"
#include "proc_c.h"
#include "proc_ab.h"
#include "daemon_impl.h"

namespace Dispatcher {

#define NICKNAME "MIXA"
#include "slogger.h"

//-----------------------------------------------------------------------
template<typename K>
const std::vector<boost::asio::const_buffers_1>& WorkerMsg<K>::requestToBuffers()
{
    static std::vector<boost::asio::const_buffers_1> v;

    v.clear();
    v.reserve(4);
    v.emplace_back(&headSize_, sizeof(headSize_));
    v.emplace_back(&dataSize_, sizeof(dataSize_));
    v.emplace_back(reqHead_.data(), reqHead_.size());
    v.emplace_back(reqData_.data(), reqData_.size());

    return v;
}
//-----------------------------------------------------------------------
Worker::Worker(boost::asio::io_service& io_s, int controlPipe, const char* dispatcherSocketName) :
    service_(io_s),
    socket_(io_s),
    ep_(dispatcherSocketName),
    headSize_(),
    dataSize_(),
    sizeBuffers_(),
    reqHead_(),
    reqData_(),
    ansHead_(),
    ansData_(),
    attemptConnectCount_(ATTEMPT_CONNECT_COUNT),
    control_(new ServerFramework::ControlPipeEvent())
{
    control_->init();
    sizeBuffers_.reserve(2);
    sizeBuffers_.emplace_back(&headSize_, sizeof(headSize_));
    sizeBuffers_.emplace_back(&dataSize_, sizeof(dataSize_));

    connect();
}
//-----------------------------------------------------------------------
Worker::~Worker()
{
    boost::system::error_code e;

    socket_.shutdown(WorkerSocket::shutdown_both, e);
    socket_.close(e);
}
//-----------------------------------------------------------------------
int Worker::run()
{
    ServerFramework::Run();

    return 0;
}
//-----------------------------------------------------------------------
void Worker::connect()
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

    if (attemptConnectCount_--) {
        socket_.async_connect(ep_, [this](auto error){ this->handleConnect(error); });
    } else {
        LogTrace(TRACE0) << "Can't establish connection";
        service_.stop();
    }
}
//-----------------------------------------------------------------------
void Worker::handleConnect(const boost::system::error_code& e)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

    if (!e) {
        attemptConnectCount_ = ATTEMPT_CONNECT_COUNT;
        readSize();
    } else {
        static constexpr long ML_SECOND = 1000;

        static boost::random::mt19937 gen(std::time(0));
        //Количество милисекунд, в течение которых процесс "не проснется"
        static boost::random::uniform_int_distribution<long> distribution(1, ML_SECOND);

        timespec t = {0, distribution(gen) * ML_SECOND * ML_SECOND};

        LogTrace(TRACE0) << __FUNCTION__ << ": attempt connect for " << socket_.native_handle() << " failed: " << e.message();
        nanosleep(&t, 0);
        connect();
    }
}
//-----------------------------------------------------------------------
void Worker::readSize()
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();
    boost::asio::async_read(socket_, sizeBuffers_,  [this](auto error, size_t N){ this->handleReadSize(error); });
}
//-----------------------------------------------------------------------
void Worker::handleReadSize(const boost::system::error_code& e)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

    if (!e) {
        boost::asio::async_read(socket_, requestToBuffers(), [this](auto error, size_t N){ this->handleReadRequest(error); });
    } else {
        handleError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
void Worker::handleReadRequest(const boost::system::error_code& e)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

    if (!e) {
        ServerFramework::process(reqHead_, reqData_, ansHead_, ansData_);

        boost::asio::write(socket_, answerToBuffers());
        readSize();
    } else {
        handleError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
void Worker::handleError(const char* functionName, const boost::system::error_code& e)
{
    boost::system::error_code error;

    LogTrace(TRACE0) << functionName << "(" << e << "): " << e.message();
    socket_.shutdown(WorkerSocket::shutdown_both, error);
    socket_.close(error);
    service_.stop();
}
//-----------------------------------------------------------------------
const std::vector<boost::asio::const_buffers_1>& Worker::answerToBuffers()
{
    static std::vector<boost::asio::const_buffers_1> v;

    headSize_ = htonl(ansHead_.size());
    dataSize_ = htonl(ansData_.size());

    v.clear();
    v.emplace_back(&headSize_, sizeof(headSize_));
    v.emplace_back(&dataSize_, sizeof(dataSize_));
    v.emplace_back(ansHead_.data(), ansHead_.size());
    v.emplace_back(ansData_.data(), ansData_.size());

    return v;
}
//-----------------------------------------------------------------------
const std::vector<boost::asio::mutable_buffers_1>& Worker::requestToBuffers()
{
    static std::vector<boost::asio::mutable_buffers_1> v;
    v.reserve(2);

    reqHead_.resize(ntohl(headSize_));
    reqData_.resize(ntohl(dataSize_));
    v.clear();
    v.emplace_back(reqHead_.data(), reqHead_.size());
    v.emplace_back(reqData_.data(), reqData_.size());

    return v;
}
//-----------------------------------------------------------------------

} //namespace Dispatcher

namespace ServerFramework {

int proc_ab_tcp_impl(int control, const ATcpParams& p)
{
    Dispatcher::Dispatcher<Dispatcher::TcpFrontend> dispatcher(
            ServerFramework::system_ios::Instance(),
            control,
            p.headtype,
            p.addr.second
                ? boost::optional<std::pair<std::string, uint16_t> >(std::make_pair(p.addr.first, p.addr.second))
                : boost::none,
            p.balancerAddr.second
                ? boost::optional<std::pair<std::string, uint16_t> >(std::make_pair(p.balancerAddr.first, p.balancerAddr.second))
                : boost::none,
            p.ipc_c.c_str(),
            p.ipc_signal.c_str(),
            p.queue.warn,
            p.queue.warn_log,
            p.queue.drop,
            p.queue.full_timestamp,
            p.max_connections,
            p.msg_expired_timeout
    );

    return dispatcher.run();
}
int proc_ab_fcg_impl(int control, const ATcpParams& p)
{
    return 0; //proc_impl<AFcgi>(control, p);
}

int proc_ab_http_impl(int control, const ATcpParams& p)
{
    Dispatcher::Dispatcher<Dispatcher::HttpFrontend> dispatcher(
            ServerFramework::system_ios::Instance(),
            control,
            p.headtype,
            p.addr.first,
            p.addr.second,
            p.ipc_c.c_str(),
            p.ipc_signal.c_str(),
            p.queue.warn,
            p.queue.warn_log,
            p.queue.drop,
            p.queue.full_timestamp,
            p.max_connections,
            p.msg_expired_timeout
    );

    return dispatcher.run();
}

int proc_ab_http_secure_impl(int control, const ATcpParams& p)
{
    Dispatcher::Dispatcher<Dispatcher::HttpSSLFrontend> dispatcher(
            ServerFramework::system_ios::Instance(),
            control,
            p.headtype,
            p.addr.first,
            p.addr.second,
            p.ipc_c.c_str(),
            p.ipc_signal.c_str(),
            p.queue.warn,
            p.queue.warn_log,
            p.queue.drop,
            p.queue.full_timestamp,
            readStringFromTcl("SSL_CERTIFICATE"),
            readStringFromTcl("SSL_PRIVATE_KEY"),
            readStringFromTcl("DIFFIE_HELLMAN_PARAMS"),
            p.msg_expired_timeout
    );

    return dispatcher.run();
}

int proc_ab_udp_impl(int control, const Aparams& p)
{
    Dispatcher::Dispatcher<Dispatcher::TxtUdpFrontend> dispatcher(
            ServerFramework::system_ios::Instance(),
            control,
            p.headtype,
            p.addr.first,
            p.addr.second,
            p.ipc_c.c_str(),
            p.ipc_signal.c_str(),
            p.queue.warn,
            p.queue.warn_log,
            p.queue.drop,
            p.queue.full_timestamp,
            0,
            p.msg_expired_timeout
    );

    return dispatcher.run();
}

int proc_c_impl(int control, const Cparams& p)
{
    Dispatcher::Worker worker(
            ServerFramework::system_ios::Instance(),
            control,
            p.ipc_c.c_str()
    );

    return worker.run();
}


} // namespace ServerFramework

int main_express_dispatcher(int supervisorSocket, int argc, char *argv[])
{
    Dispatcher::Dispatcher<Dispatcher::AirSrvFrontend> dispatcher(
            ServerFramework::system_ios::Instance(),
            supervisorSocket,
            get_signalsock_name(
                getTclInterpretator(),
                Tcl_NewStringObj("XPR_DISP_IN", -1), 0, 0/*suffix*/
            ).c_str(),
            get_signalsock_name(
                getTclInterpretator(),
                Tcl_NewStringObj("XPR_DISP_OUT", -1), 0, 0/*suffix*/
            ).c_str(),
            get_signalsock_name(
                    getTclInterpretator(),
                    Tcl_NewStringObj("XPR_DISP_SIGNAL", -1), 0, 0/*suffix*/
            ).c_str(),
            readIntFromTcl("XPR_DISP_QUEUE_WARN", 768),
            readIntFromTcl("XPR_DISP_QUEUE_WARN_LOG", 896),
            readIntFromTcl("XPR_DISP_QUEUE_DROP", 1024),
            getVariableStaticBool("XPR_DISP_QUEUE_FULL_TIMESTAMP", 0, 0),
            readIntFromTcl("XPR_DISP_MSG_EXPIRED_TIMEOUT", 60)
    );

    return dispatcher.run();
}
