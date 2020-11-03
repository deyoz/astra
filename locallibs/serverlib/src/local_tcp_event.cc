#if HAVE_CONFIG_H
#endif

#include <unistd.h> // unlink
#include <typeinfo>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio.hpp>

#include "tcl_utils.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

#include "monitor_ctl.h"
#include "daemon_impl.h"
#include "local_tcp_event.h"

namespace
{
using boost::asio::local::stream_protocol;
using boost::asio::async_read;

class LocalTcpDaemonEventImpl
{
public:
    LocalTcpDaemonEventImpl(const char* sockName, ServerFramework::LocalTcpDaemonEvent& event)
        : io_service_(ServerFramework::system_ios::Instance())
          , ep_(/*get_signalsock_name(getTclInterpretator(), Tcl_NewStringObj(*/sockName/*,-1), 0 , 0*//*suffix*//*)*/)
          , event_(event), answerBuffers_(), showErrors_(false)
    {
        sz_[0] = sz_[1] = 0;
        ansSize_[0] = htonl(0);
        ansSize_[1] = htonl(1);
        answerBuffers_.push_back(boost::asio::const_buffers_1(ansSize_, sizeof(ansSize_)));
        answerBuffers_.push_back(boost::asio::const_buffers_1(" ", 1));

        memset(data_, 0, sizeof(data_));
        sessionAsyncConnect();
    }
    virtual ~LocalTcpDaemonEventImpl() {}
    void handleConnect(const boost::system::error_code& e) {
        LogTrace(TRACE5) << __FUNCTION__;
        if (!e) {
            sessionStart();
            showErrors_ = true;
        } else {
            sleep(5);
            sessionHandleError(e);
            showErrors_ = true;
        }
    }
    void sessionAsyncConnect() {
        LogTrace(TRACE5) << __FUNCTION__;
        socketPtr_ = std::make_shared<stream_protocol::socket>(io_service_);
        socketPtr_->async_connect(ep_, [this](auto error){ this->handleConnect(error); });
    }
    void sessionHandleError(const boost::system::error_code& e) {
        LogTrace(TRACE5) << __FUNCTION__;
        if ((!showErrors_) || (e == boost::asio::error::eof)) {
            LogTrace(TRACE1) << "error (" << e << "): " << e.message();
        } else {
            LogError(STDLOG) << "error (" << e << "): " << e.message();
        }
        socketPtr_->close();
        sessionAsyncConnect();
    }
    void sessionStart() {
        LogTrace(TRACE5) << __FUNCTION__;
        async_read(*socketPtr_, boost::asio::buffer(sz_, sizeof(sz_)), [this](auto&& error, auto&& bytes){ this->sessionHandleReadLength(error,bytes); });
    }
    void sessionHandleReadLength(const boost::system::error_code& e, size_t bytes_transferred) {
        LogTrace(TRACE5) << __FUNCTION__;
        if (!e) {
            sz_[1] = ntohl(sz_[1]);
            if (sz_[1] > max_length) {
                LogError(STDLOG) << "invalid size: " << sz_[1];
                sessionHandleError(e);
            } else {
                ProgTrace(TRACE5, "length: %ud", sz_[1]);
                async_read(*socketPtr_, boost::asio::buffer(&data_, sz_[1]), [this](auto&& error, auto&& bytes){ this->sessionHandleReadData(error,bytes); });
            }
        } else {
            sessionHandleError(e);
        }
    }
    void sessionHandleWrite(const boost::system::error_code& e, size_t bytes_transferred) {
        LogTrace(TRACE5) << __FUNCTION__;
        if (!e) {
            // pass
        } else {
            sessionHandleError(e);
        }
    }
    void sessionWriteAnswer() {
        LogTrace(TRACE5) << __FUNCTION__;
        async_write(*socketPtr_, answerBuffers_, [this](auto&& error, auto&& bytes){ this->sessionHandleWrite(error,bytes); });
    }
    void sessionHandleReadData(const boost::system::error_code& e, size_t bytes_transferred) {
        LogTrace(TRACE5) << __FUNCTION__;
        
        if (!e) {
            ProgTrace(TRACE5, "process message: sz=%ud data=[%s]", sz_[1], data_);
            event_.runTasks(data_, sz_[1]);
            memset(data_, 0, sizeof(data_));
            sessionWriteAnswer();
            sessionStart();
        } else {
            sessionHandleError(e);
        }
    }
private:
    boost::asio::io_service& io_service_;
    stream_protocol::endpoint ep_;
    std::shared_ptr<stream_protocol::socket> socketPtr_;
    ServerFramework::LocalTcpDaemonEvent& event_;
    enum { max_length = 65536 };
    //sz_[0] fake value. Need read from dispatcher, because used in request handlers (obrzap)
    uint32_t sz_[2];
    uint32_t ansSize_[2];
    std::vector<boost::asio::const_buffers_1> answerBuffers_;
    char data_[max_length];
    bool showErrors_;
};
}

namespace ServerFramework
{

LocalTcpDaemonEvent::LocalTcpDaemonEvent(const char* sockName)
{
    pImpl_.reset(new LocalTcpDaemonEventImpl(sockName, *this));
}

void LocalTcpDaemonEvent::init()
{
    // do nothing
}


} // namespace ServerFramework
