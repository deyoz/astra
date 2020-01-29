#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/asio.hpp>

#include <tclmon/tclmon.h>
#include <tclmon/tcl_utils.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/ourtime.h>
#include <serverlib/daemon_event.h>
#include <serverlib/daemon_impl.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

#ifdef HAVE_CONFIG_H
#endif
#include "telegrams.h"
#include "sender.h"
#include "net_func.h"
#include "express_sender.h"
#include "router_cache.h"
#include "express.h"
#include "tlgnum.h"
#include "gateway.h"

/***** ExpressSender *****/
namespace
{
using namespace telegrams;
using boost::asio::local::stream_protocol;
using boost::asio::async_read;

enum {max_length = 65000};

struct TcpMsg
{
    uint32_t sz;
    char data[max_length + 1];
};
struct ExpressSession
{
    stream_protocol::socket sock;
    TcpMsg msg;
    ExpressSession(boost::asio::io_service& ios)
        : sock(ios)
    { memset(&msg, 0, sizeof(msg)); }
};
typedef boost::shared_ptr<ExpressSession> ExpressSessionPtr;

class ExpressSenderImpl
{
public:
    ExpressSenderImpl(
            int supervisorSocket,
            const std::string& handlerName,
            const char* xprSenderSocketName,
            const char* xprSndPortName)
        : handlerName_(handlerName),
        reinitRoutersTimeout_(readIntFromTcl("REINIT_ROUTERS_TIMEOUT", 300)),
        ios_(ServerFramework::system_ios::Instance()), xprSndAcceptor_(ios_), reinitRoutersTimer_(ios_),
        senderSock_(ios_), control_()
    {
        maxrtrs_ = 0;
        port_ = readIntFromTcl(xprSndPortName);
        InitLogTime(handlerName.c_str());
        control_.init();
        LogTrace(TRACE1) << handlerName_ << " express sender listens to socket: " << xprSenderSocketName;
        const std::string sockNameWithSuff = get_signalsock_name(getTclInterpretator(), Tcl_NewStringObj(xprSenderSocketName, -1), 0 , 0/*suffix*/);
        unlink(sockNameWithSuff.c_str());
        const stream_protocol::endpoint ep(sockNameWithSuff);
        xprSndAcceptor_.open(ep.protocol());
        xprSndAcceptor_.set_option(stream_protocol::acceptor::reuse_address(true));
        xprSndAcceptor_.bind(ep);
        xprSndAcceptor_.listen();
    }
    virtual ~ExpressSenderImpl() {}
    int run() {
        const int sockfd = initSenderLocalSocket(port_);
        senderSock_.assign(boost::asio::ip::udp::v4(), sockfd);
        handle_reinit_router_cache(boost::system::error_code());
        asyncAccept();
        ServerFramework::Run();
        return 0;
    }
    void asyncAccept() {
        ProgTrace(TRACE5, "%s", __FUNCTION__);
        ExpressSessionPtr es(new ExpressSession(ios_));
        xprSndAcceptor_.async_accept(es->sock, [this,es](auto&& error){ this->handleAccept(es, error); });
    }
    void handleError(const char* func, ExpressSessionPtr es, const boost::system::error_code& e) {
        if ((!e)
                || (e == boost::asio::error::operation_aborted)
                || (e == boost::asio::error::eof)) {
            LogTrace(TRACE0) << func << "(" << e << "): " << e.message();
        } else {
            LogError(STDLOG) << func << "(" << e << "): " << e.message();
        }
        es->sock.close();
        asyncAccept();
    }
    void handleAccept(ExpressSessionPtr es, const boost::system::error_code& e) {
        if (!e) {
            asyncReadLength(es);
            asyncAccept();
        } else {
            LogError(STDLOG) << __FUNCTION__ << "(" << e << "): " << e.message();
            asyncAccept();
        }
    }
    void asyncReadLength(ExpressSessionPtr es) {
        ProgTrace(TRACE5, "%s %p", __FUNCTION__, es.get());
        async_read(es->sock, boost::asio::buffer(&es->msg.sz, sizeof(es->msg.sz)), [this,es](auto&& error, auto&& bytes){ this->handleReadLength(es,error,bytes); });
    }
    void handleReadLength(ExpressSessionPtr es, const boost::system::error_code& e, size_t /*bytes_transferred*/) {
        if (!e) {
            es->msg.sz = ntohl(es->msg.sz);
            if (es->msg.sz > max_length) {
                LogError(STDLOG) << "invalid size: " << es->msg.sz;
                handleError(__FUNCTION__, es, e);
            } else {
                ProgTrace(TRACE5, "%s %p sz=%d", __FUNCTION__, es.get(), es->msg.sz);
                async_read(es->sock, boost::asio::buffer(es->msg.data, es->msg.sz), [this,es](auto&& error, auto&& bytes){ this->handleReadData(es,error,bytes); });
            }
        } else {
            handleError(__FUNCTION__, es, e);
        }
    }
    void handleReadData(ExpressSessionPtr es, const boost::system::error_code& err, size_t bytes_transferred) {
        InitLogTime(handlerName_.c_str());
        LogTrace(TRACE5) << "err=" << err << " bytes_transferred=" << bytes_transferred;
        if (!err) {
            es->msg.data[es->msg.sz] = 0; // end buffer with 0
            work(es->msg.data, es->msg.sz);
            asyncReadLength(es);
        } else {
            handleError(__FUNCTION__, es, err);
        }
    }
    void handle_reinit_router_cache(const boost::system::error_code& err) {
        InitLogTime(handlerName_.c_str());
        if (!err) {
            std::list<RouterInfo> routers;
            callbacks()->readAllRouters(routers);
            cachedRouters_.clear();
            for (const RouterInfo& routerInfo : routers)
            {
                cachedRouters_[routerInfo.ida] = routerInfo;
            }
            reinitRoutersTimer_.expires_from_now(boost::posix_time::seconds(reinitRoutersTimeout_));
            reinitRoutersTimer_.async_wait([this](auto&& error){ this->handle_reinit_router_cache(error); });
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            reinitRoutersTimer_.expires_from_now(boost::posix_time::seconds(reinitRoutersTimeout_));
            reinitRoutersTimer_.async_wait([this](auto&& error){ this->handle_reinit_router_cache(error); });
        }
        callbacks()->rollback();
    }
    void work(const char* msg, size_t bytes_transferred) {
        if (bytes_transferred == 0) {
            LogTrace(TRACE1) << "no bytes_transferred";
            return;
        }

        boost::optional<telegrams::express::Header> header = telegrams::express::parseSendHeader(msg);
        if (!header) {
            ProgError(STDLOG, "bad msg format msg: %.20s", msg);
            return;
        }
        tlgnum_t tlgNum = header->tlgNum;
        tlgNum.express = true;
        AIRSRV_MSG tlg = {};
        size_t tlgSz = bytes_transferred - header->size;
        if (tlgSz > sizeof(tlg.body)) {
            LogError(STDLOG) << " too long body: " << tlgSz;
            return;
        }
        auto routerIter = cachedRouters_.find(header->routerNum);
        if (routerIter == cachedRouters_.end()) {
            ProgTrace(TRACE1, "router with number %d not found, skipping", header->routerNum);
            return;
        }
        const RouterInfo& ri = routerIter->second;
        tlg.num = 0; // use 0 for express telegrams
        strncpy(tlg.Sender, ri.senderName.c_str(), ROT_NAME_LEN);
        strncpy(tlg.Receiver, ri.canonName.c_str(), ROT_NAME_LEN);
        tlg.TTL = header->ttl;
        strcpy(tlg.body, msg + header->size);

        monitor_beg_work();
        if (sendMessage(tlgNum, tlg, ri) < 0) {
            LogError(STDLOG) << "sendMessage failed";
            monitor_idle();
        } else {
            monitor_idle_zapr_type(1, QUEPOT_NULL);
        }
    }
private:
    int sendMessage(const tlgnum_t& realTlgNum, AIRSRV_MSG& tlg, const RouterInfo& ri) {
        const size_t tlg_len = tlgLength(tlg);
        LogTrace(TRACE1) << "Sending TLG " << realTlgNum << " ttl " << tlg.TTL << " "
            << tlg_len << "b [" << tlg.Sender << "]->[" << tlg.Receiver << "] to "
            << ri.address << ":" << ri.port << " at " <<tlgTime();

        tlg.num = htonl(tlg.num);
        tlg.type = htons(TLG_OUT);
        tlg.TTL = htons(tlg.TTL);
        boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(ri.address), ri.port);
        boost::system::error_code err;
        size_t sent = senderSock_.send_to(boost::asio::buffer(&tlg, tlg_len), destination, MSG_NOSIGNAL, err);
        if ((sent != tlg_len) || err) {
            LogError(STDLOG) << " failed send_to: " << sent << " bytes sent instead " << tlg_len << " err: " << err;
            return -1;
        }
        return 0;
    }
    std::string handlerName_;
    const std::string ourname_;
    std::map<int, RouterInfo> cachedRouters_;
    int port_;
    size_t maxrtrs_;
    const size_t reinitRoutersTimeout_;

    boost::asio::io_service& ios_;
    stream_protocol::acceptor xprSndAcceptor_;
    boost::asio::deadline_timer reinitRoutersTimer_;
    boost::asio::ip::udp::socket senderSock_;
    ServerFramework::ControlPipeEvent control_;
};
}


namespace telegrams
{
namespace express
{
Sender::Sender(
        int supervisorSocket,
        const std::string& handlerName,
        const char* senderSocketName,
        const char* senderPortName)
{
    pImpl_ = new ExpressSenderImpl(supervisorSocket, handlerName, senderSocketName, senderPortName);
}

Sender::~Sender()
{
    delete static_cast<ExpressSenderImpl*>(pImpl_);
}

int Sender::run()
{
    return static_cast<ExpressSenderImpl*>(pImpl_)->run();
}

} // namespace express
} // namespace telegrams

