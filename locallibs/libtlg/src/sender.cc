#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <vector>
#include <deque>
#include <errno.h>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/lexical_cast.hpp>

#include <tclmon/tclmon.h>
#include <tclmon/tcl_utils.h>
#include <serverlib/exception.h>
#include <serverlib/cursctl.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/ourtime.h>
#include <serverlib/posthooks.h>
#include <serverlib/daemon_event.h>
#include <serverlib/daemon_impl.h>
#include <boost/optional.hpp>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

#ifdef HAVE_CONFIG_H
#endif
#include "telegrams.h"
#include "sender.h"
#include "net_func.h"
#include "router_cache.h"
#include "express.h"
#include "gateway.h"
#include "types.h"
#include "air_q_callbacks.h"


namespace
{
using namespace telegrams;

struct SenderCachedLastTlg
{
    tlgnum_t num;
    int counter;
    time_t time;

    SenderCachedLastTlg(const tlgnum_t& num_) : num(num_)
    {}
};

struct SenderCachedRouter
{
    boost::optional<SenderCachedLastTlg> lastTlg;
    telegrams::RouterInfo ri;
};
typedef std::map<int, SenderCachedRouter> SenderCachedRouters;

void reinitSenderRoutersCache(const std::list<telegrams::RouterInfo>& routers, SenderCachedRouters& cachedRouters)
{
    const SenderCachedRouters oldCachedRouters(cachedRouters);
    cachedRouters.clear();
    for(const telegrams::RouterInfo& ri : routers)
    {
        SenderCachedRouter cr = {};
        auto iter = oldCachedRouters.find(ri.ida);
        cr.ri = ri;
        if (iter != oldCachedRouters.cend())
            cr.lastTlg = iter->second.lastTlg;
        cachedRouters.insert(std::make_pair(ri.ida, cr));
    }
}

static size_t fillRoutersToSend(RoutersWithTlgs& routersToSend, int queueType)
{
    std::list<telegrams::AirQStat> stats = telegrams::airQManager()->getQStats(queueType);
    std::stringstream str;
    for(const telegrams::AirQStat& stat : stats) {
        RoutersWithTlg rtr(stat.q_num, stat.msg_id);
        LogTrace(TRACE5) << "routerNum=" << rtr.routerNum << " tlgNum=" << rtr.tlgNum;
        routersToSend.push_back(rtr);
        str << " " << rtr.routerNum;
        str << "[" << rtr.tlgNum << "]; ";
    }

    LogTrace(TRACE1) << routersToSend.size() << " routers detected: " << str.str();
    return routersToSend.size();
}

class DelayManager
{
public:
    void wait() {
        boost::posix_time::ptime tm(boost::posix_time::microsec_clock::local_time());

        if (times_.size() == 10) {
            boost::posix_time::ptime firstTime = times_.front();
            const int delta = (tm - firstTime).total_milliseconds();
            const size_t delay = (delta < 50) ? 50 - delta : 0;
            if (delay) {
                waitDelay(delay);
                tm = boost::posix_time::microsec_clock::local_time();
            }
            times_.pop_front();
        }
        times_.push_back(tm);
    }
private:
    void waitDelay(int delay) {
        pollfd pfd = {};
        const int ret = poll(&pfd, 0, delay);
        if (ret < 0) {
            ProgError(STDLOG, "failed poll ret=%d errno=%s", ret, strerror(errno));
        }
    }
    std::deque<boost::posix_time::ptime> times_;
};

class SenderImpl
{
    enum SenderResult_t {NothingToSend, WaitingForResend, SendSmth};
public:
    SenderImpl(
            int supervisorSocket,
            Sender* snd,
            const std::string& handlerName,
            const char* senderSocketName,
            const char* senderPortName,
            Direction qt)
        : pSnd_(snd), handlerName_(handlerName), queueType_(qt),
        reinitRoutersTimeout_(readIntFromTcl("REINIT_ROUTERS_TIMEOUT", 300)),
        senderTimeout_(readIntFromTcl("SENDER_TIMEOUT", 60)),
        senderResendTimeout_(readIntFromTcl("SENDER_RESEND_TIMEOUT", 2)),
        ios_(ServerFramework::system_ios::Instance()), timer_(ios_), reinitRoutersTimer_(ios_), resendTimer_(ios_),
        signalSock_(ios_), senderSock_(ios_), control_()
    {
        memset(signal_msg, 0, sizeof(signal_msg));
        maxrtrs_ = 0;
        LogTrace(TRACE1) << handlerName_ << " sender for queueType " << queueType_
            << " listens to socket: " << senderSocketName;
        std::string name = get_signalsock_name(getTclInterpretator(), Tcl_NewStringObj(senderSocketName, -1), 0 , 0);
        int sigfd = makeSignalSocket(name);
        if (sigfd < 0) {
            LogTrace(TRACE0) << "handlerName=" << handlerName << " senderSocketName="  << senderSocketName;
            sleep(5);
            throw comtech::Exception("makeSignalSocket failed");
        }
        signalSock_.assign(boost::asio::ip::udp::v4(), sigfd);
        port_ = readIntFromTcl(senderPortName);
        control_.init();
    }
    virtual ~SenderImpl() {}
    int run() {
        const int sockfd = initSenderLocalSocket(port_);
        senderSock_.assign(boost::asio::ip::udp::v4(), sockfd);
        handle_reinit_router_cache(boost::system::error_code());
        handle_timer(boost::system::error_code());
        signalSock_.async_receive_from(boost::asio::buffer(signal_msg, max_signal_msg_), signalEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        ServerFramework::Run();
        return 0;
    }
    void handle_socket(const boost::system::error_code& err, size_t bytes_transferred) {
        InitLogTime(handlerName_.c_str());
        LogTrace(TRACE5) << "err=" << err << " bytes_transferred=" << bytes_transferred;
        if (!err) {
            work();
            signalSock_.async_receive_from(boost::asio::buffer(signal_msg, max_signal_msg_), signalEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            signalSock_.async_receive_from(boost::asio::buffer(signal_msg, max_signal_msg_), signalEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        }
    }
    void handle_timer(const boost::system::error_code& err) {
        InitLogTime(handlerName_.c_str());
        if (!err) {
            work();
            timer_.expires_from_now(boost::posix_time::seconds(senderTimeout_));
            timer_.async_wait([this](auto&& error){ this->handle_timer(error); });
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            timer_.expires_from_now(boost::posix_time::seconds(senderTimeout_));
            timer_.async_wait([this](auto&& error){ this->handle_timer(error); });
        }
    }
    void handle_resend(const boost::system::error_code& err) {
        InitLogTime(handlerName_.c_str());
        if (!err) {
            LogTrace(TRACE5) << "wake up for resend";
            work();
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
        }
    }
    void handle_reinit_router_cache(const boost::system::error_code& err) {
        InitLogTime(handlerName_.c_str());
        if (!err) {
            std::list<RouterInfo> routers;
            callbacks()->readAllRouters(routers);
            reinitSenderRoutersCache(routers, cachedRouters_);
            reinitRoutersTimer_.expires_from_now(boost::posix_time::seconds(reinitRoutersTimeout_));
            reinitRoutersTimer_.async_wait([this](auto&& error){ this->handle_reinit_router_cache(error); });
            if (pSnd_->haveUnknownRouters(routers))
                ProgError(STDLOG, "Unknown routers found after initRouters");
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            reinitRoutersTimer_.expires_from_now(boost::posix_time::seconds(reinitRoutersTimeout_));
            reinitRoutersTimer_.async_wait([this](auto&& error){ this->handle_reinit_router_cache(error); });
        }
    }
    void work() {
        SenderResult_t res = SendSmth;
        while (res == SendSmth) {
            res = sendAll();
        }
        if (res == WaitingForResend) {
            resendTimer_.expires_from_now(boost::posix_time::seconds(senderResendTimeout_));
            resendTimer_.async_wait([this](auto&& error){ this->handle_resend(error); });
        }
    }
    Direction queueType() const {   return queueType_; }
    SenderResult_t sendAll() {
        monitor_beg_work();
        SenderResult_t repeatTimeout = SendSmth;
        try {
            monitor_idle();
            emptyHookTables();

            RoutersWithTlgs routersToSend;
            pSnd_->fillRoutersToSend(routersToSend, static_cast<int>(queueType_));
            // nothing to send - wait for kick
            if (routersToSend.empty()) {
                ProgTrace(TRACE5, "nothing to send - waiting for %d seconds", senderTimeout_);
                repeatTimeout = NothingToSend;
            } else {
                bool nothingWasSent = true;
                DelayManager dm;
                for (const RoutersWithTlg& router : routersToSend) {
                    SenderCachedRouters::iterator it = cachedRouters_.find(router.routerNum);
                    if (it == cachedRouters_.end()) {
                        ProgTrace(TRACE1, "router with number %d not found, skipping", router.routerNum);
                        continue;
                    }
                    SenderCachedRouter& cr = it->second;
                    if (cr.lastTlg && (router.tlgNum == cr.lastTlg->num)) {
                        if ((time(0) - cr.lastTlg->time) < senderResendTimeout_)
                            continue;
                        LogTrace(TRACE0) << "ROUTER_RESEND: " << router.routerNum << " tlgNum=" << router.tlgNum;
                    }
                    dm.wait();
                    sendTlgToRouter(cr);
                    nothingWasSent = false;
                }
                // if nothingWasSent we wait senderResendTimeout_ delay
                if (nothingWasSent) {
                    ProgTrace(TRACE5, "waiting for resend");
                    repeatTimeout = WaitingForResend;
                }
            }
        } catch (const comtech::Exception &e) {
            e.error(STDLOG);
            pSnd_->rollback();
        }
        callPostHooksAlways();
        return repeatTimeout;
    }

private:
    int sendTlg(const tlgnum_t& tlgNum, const TlgInfo& info, const RouterInfo& ri, const std::string& tlgText, boost::optional<hth::HthInfo> hthInfo) {
        AIRSRV_MSG tlg = {};
        if (tlgText.size() > sizeof(tlg.body)) {
            LogError(STDLOG) << tlgNum << " too long body: " << tlgText.size();
            return -1;
        }
        tlg.num = pSnd_->tlgnumAsInt32(tlgNum);
        strncpy(tlg.Sender, ri.senderName.c_str(), ROT_NAME_LEN);
        strncpy(tlg.Receiver, ri.canonName.c_str(), ROT_NAME_LEN);
        tlg.TTL = info.ttl;
        strcpy(tlg.body, tlgText.c_str());

        if (hthInfo) {
            hth::createTlg(tlg.body, hthInfo.get());
        }
        return sendMessage(tlgNum, tlg, ri);
    }
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

    void sendTlgToRouter(SenderCachedRouter& cr) {
        monitor_beg_work();
        TlgInfo info = {};
        info.queueType = queueType_;
        info.routerNum = cr.ri.ida;
        std::string tlgText;
        boost::optional<tlgnum_t> tlgNum = pSnd_->getTlg(info, tlgText);
        if (!tlgNum) {
            monitor_idle();
            return;
        }
        if (tlgNum && tlgText.empty()) {
            LogError(STDLOG) << "tlg " << *tlgNum << " with empty text - skip sending";
            callbacks()->gatewayQueue().delTlg(*tlgNum);
            pSnd_->commit();
            monitor_idle();
            return;
        }

        boost::optional<SenderCachedLastTlg>& lastMessage = cr.lastTlg;
        if (lastMessage && lastMessage->num == tlgNum) {
            lastMessage->counter++;
        } else {
            lastMessage = SenderCachedLastTlg(*tlgNum);
            lastMessage->counter = 0;
        }
        lastMessage->time = time(0);

        boost::optional<hth::HthInfo> hthInfo;
        {
            hth::HthInfo hi = {};
            int ret = callbacks()->readHthInfo(*tlgNum, hi);
            if (ret == 0) {
                hthInfo = hi;
            }
        }
        // TTL expired - not send
        if (info.ttl == 1) {
            callbacks()->ttlExpired(tlgText, true, info.routerNum, hthInfo);
            LogTrace(TRACE0) << "TTL expired for tlg num " << *tlgNum << " - deleting;";
            callbacks()->gatewayQueue().delTlg(*tlgNum);
            pSnd_->commit();
            return;
        }
        const int sent = sendTlg(*tlgNum, info, cr.ri, tlgText, hthInfo);
        if (sent < 0) {
            LogError(STDLOG) << *tlgNum << " sendTlg error: sent=" << sent;
        }
        monitor_idle_zapr_type(1, QUEPOT_NULL);
    }

    Sender* pSnd_;
    std::string handlerName_;
    Direction queueType_;
    SenderCachedRouters cachedRouters_;

    size_t maxrtrs_;
    int port_;
    //static const size_t max_signal_msg_ = MAX_TLG_SIZE;
    static const size_t max_signal_msg_ = 1; // kick is one space
    char signal_msg[max_signal_msg_ + 1];

    const int reinitRoutersTimeout_;
    const int senderTimeout_;
    const int senderResendTimeout_;

    boost::asio::io_service& ios_;
    boost::asio::deadline_timer timer_, reinitRoutersTimer_, resendTimer_;
    boost::asio::ip::udp::socket signalSock_;
    boost::asio::ip::udp::socket senderSock_;
    boost::asio::ip::udp::endpoint signalEndpoint_;
    ServerFramework::ControlPipeEvent control_;
};

}  // namespace

namespace telegrams
{

RoutersWithTlg::RoutersWithTlg(const int router, const tlgnum_t& tlg)
    : routerNum(router), tlgNum(tlg)
{
}

/***** Sender *****/

Sender::Sender(
        int supervisorSocket,
        const std::string& handlerName,
        const char* senderSocketName,
        const char* senderPortName,
        Direction qt)
{
    pImpl_ = new SenderImpl(supervisorSocket, this, handlerName, senderSocketName, senderPortName, qt);
}

Sender::~Sender()
{
    delete static_cast<SenderImpl*>(pImpl_);
}

int Sender::run()
{
    return static_cast<SenderImpl*>(pImpl_)->run();
}

boost::optional<tlgnum_t> Sender::getTlg(TlgInfo& info, std::string& tlgText)
{
    boost::optional<tlgnum_t> tlgNum = getTlg4Sending(info);
    if (!tlgNum) {
        ProgTrace(TRACE5, "no telegrams found: sys_q=%d q_num=%d", info.queueType, info.routerNum);
        return boost::none;
    }
    const int res = callbacks()->getTlg(*tlgNum, info, tlgText);
    if (res < 0) {
        LogTrace(TRACE0) << "TlgInfo: sys_q=" << info.queueType
            << " q_num=" << info.routerNum << " ttl=" << info.ttl;
        LogError(STDLOG) << "getTlg " << *tlgNum << " failed with code " << res;
        tlgText.clear();
        return tlgNum;
    }
    LogTrace(TRACE1) << "telegram for sending msg_id=" << *tlgNum << " sys_q=" << info.queueType
        << " q_num=" << info.routerNum << " ttl=" << info.ttl;
    return tlgNum;
}

Direction Sender::queueType() const
{
    return static_cast<SenderImpl*>(pImpl_)->queueType();
}


size_t Sender::fillRoutersToSend(RoutersWithTlgs& routersToSend, int queueType)
{
    return ::fillRoutersToSend(routersToSend, queueType);
}

int32_t Sender::tlgnumAsInt32(const tlgnum_t& tlgNum)
{
    return boost::lexical_cast<int32_t>(tlgNum.num.get());
}

void Sender::commit()
{
    airQManager()->commit();
}

void Sender::rollback()
{
    airQManager()->rollback();
}

} // namespace telegrams

