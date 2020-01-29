#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <deque>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include <boost/asio.hpp>

#include <tclmon/tclmon.h>

#ifdef HAVE_CONFIG_H
#endif

#include <serverlib/exception.h>
#include <serverlib/cursctl.h>
#include <edilib/edi_user_func.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/daemon_kicker.h>
#include <serverlib/ourtime.h>
#include <serverlib/daemon_event.h>
#include <serverlib/daemon_impl.h>
#include <tclmon/tcl_utils.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>

#include "airsrv.h"
#include "tlgnum.h"
#include "telegrams.h"
#include "tlgsplit.h"
#include "net_func.h"
#include "router_cache.h"
#include "gateway.h"
#include "types.h"

namespace
{
using namespace telegrams;
    
struct AirsrvCachedMsg
{
    AirsrvCachedMsg() : 
        num(0), counter(0), time(0)
    {}
    int num;           ///< last tlg number
    int counter;            ///< times this tlg already received
    time_t time;            ///< time when we received this numTLg first
};

struct AirsrvCachedRouter
{
    std::map< ushort, AirsrvCachedMsg > msg_map;
    RouterInfo ri;
};
typedef std::map<std::string, AirsrvCachedRouter> AirSrvSenderCachedRouters;
struct AirSrvSenderCachedRouterManager
{
    AirSrvSenderCachedRouters::const_iterator find(const AirSrvSenderCachedRouters & cachedRouters, const RouterInfo& ri) const {
        return cachedRouters.find(ri.canonName);
    }
    void insert(AirSrvSenderCachedRouters & cachedRouters, const RouterInfo& ri, const AirsrvCachedRouter& cr) const {
        cachedRouters.insert(std::make_pair(ri.canonName, cr));
    }
};

static int initReceiverSocket(const char* portName)
{
    const std::string localHost = readStringFromTcl("LOCAL_HOST", "");
    unsigned srv_ip = 0;
    if (localHost == "")
        srv_ip = htonl(INADDR_ANY);
    srv_ip = telegrams::get_ip(localHost.c_str());
    if (!srv_ip) {
        ProgTrace(TRACE0, "Can't get IPaddress from LOCAL_HOST");
        srv_ip = htonl(INADDR_ANY);
    }
    const int srv_port = readIntFromTcl(portName);
    ProgTrace(TRACE0, "Start AIRSRV: srv_port=%d, srv_ip=%d(%s) ", srv_port, srv_ip, localHost.c_str());

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        ProgError(STDLOG, "Error socket: sock_fd %d  %d : %s", sock_fd, errno, strerror(errno));
        throw comtech::Exception("failed socket");
    }
    sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = srv_ip;    /*htonl(INADDR_ANY);*/
    adr.sin_port = htons(srv_port);

    if (bind(sock_fd, (sockaddr*)&adr, sizeof(adr)) < 0) {
        ProgError(STDLOG, "failed bind %d : %s", errno, strerror(errno));
        throw comtech::Exception("failed bind");
    } else
        ProgTrace(TRACE0, "srv_socket %d bound at port %d", sock_fd, srv_port);
    return sock_fd;
}

class AirsrvImpl
{
public:
    AirsrvImpl(int supervisorSocket, Airsrv& airsrv, const char* portName)
        : ReinitRoutersTimeout_(readIntFromTcl("REINIT_ROUTERS_TIMEOUT", 300)),
        airsrv_(airsrv),
        ios_(ServerFramework::system_ios::Instance()),
        localSock_(ios_),
        timer_(ios_),
        control_()
    {
        const int fd = initReceiverSocket(portName);
        localSock_.assign(boost::asio::ip::udp::v4(), fd);
        memset(&msg_, 0, sizeof(msg_));
        control_.init();
    }
    void run() {
        handle_timer(boost::system::error_code());
        localSock_.async_receive_from(boost::asio::buffer(&msg_, max_msg_sz_), remoteEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        ServerFramework::Run();
    }
private:
    void sendAnswerTo(const telegrams::AIRSRV_MSG& msg, const std::string& ipAddr, ushort port, u_short ackType) {
        telegrams::AIRSRV_MSG ackMsg = {0};
        sprintf(ackMsg.body, "%s %d", tlgBeg_.c_str(), msg.num);
        ackMsg.num = msg.num;
        ackMsg.type = ackType;
        ackMsg.TTL = 0;
        strncpy(ackMsg.Sender, msg.Receiver, ROT_NAME_LEN);
        strncpy(ackMsg.Receiver, msg.Sender, ROT_NAME_LEN);

        ackMsg.num = htonl(ackMsg.num);
        ackMsg.type = htons(ackMsg.type);
        ackMsg.TTL = htons(ackMsg.TTL);

        const size_t len = tlgLength(ackMsg);
        const boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(ipAddr), port);
        boost::system::error_code err;
        const size_t sent = localSock_.send_to(boost::asio::buffer(&ackMsg, len), destination, MSG_NOSIGNAL, err);
        if ((sent != len) || (err)) {
            LogError(STDLOG) << "num " << msg.num << " " << msg.Receiver
                << " failed send_to: " << sent << " bytes sent instead " << len << " err: " << err;
        }
    }
    /*
      Весь этот механизм призван не допускать повторной обработки
      одной и той же телеграммы, которая получена несколько раз подряд
    */
    bool checkForRepeatMsg(AirsrvCachedRouter& cr, ushort port) {
        if ((msg_.type == TLG_IN) || (msg_.type == TLG_OUT)) {
            AirsrvCachedMsg& cachedMsg = cr.msg_map[port];
            if((msg_.num != 0) && (msg_.num == cachedMsg.num)) {
                cachedMsg.counter++;
                LogTrace(TRACE0) << "skipped tlg " << msg_.num << " from " 
                                 << std::string(msg_.Sender, ROT_NAME_LEN) << ":" << port
                                 << " received " << cachedMsg.counter 
                                 << " times already (first " << time(NULL) - cachedMsg.time << " seconds ago)";
                return true;
            }
        }
        return false;
    }
    void processMessage() {
        monitor_idle();

        const std::string ipFrom = remoteEndpoint_.address().to_string();
        const ushort remotePort = remoteEndpoint_.port();
        ushort answerPort = remotePort; // WARNING: we may answer to another port
        const std::string senderName(msg_.Sender, ROT_NAME_LEN);

        ProgTrace(TRACE1, "TLG type %d, num %d, ttl %d from %s:%d:%s at %s",
                msg_.type, msg_.num, msg_.TTL, ipFrom.c_str(), answerPort, senderName.c_str(), tlgTime());

        if (msg_.type != TLG_CONN_REQ && msg_.type != TLG_CONN_RES) {
            tlgBeg_ = std::string(msg_.body, tlgBegLen_);
            AirSrvSenderCachedRouters::iterator cri = cachedRouters_.find(senderName);
            if (cri == cachedRouters_.end()) {
                LogError(STDLOG) << "rejected tlg from unknown sender: " << senderName << " (" << ipFrom << ")";
            } else {
                AirsrvCachedRouter& cr = cri->second;
                if (ipFrom != cr.ri.address) {
                    LogError(STDLOG) << "rejected tlg from invalid address: " << senderName << " (" << ipFrom << ")";
                } else {
                    answerPort = cr.ri.port;
                    if(!checkForRepeatMsg(cr, remotePort)) {
                        airsrv_.processMessage(msg_, cr.ri);
                        if (msg_.type == TLG_IN || msg_.type == TLG_OUT) {
                            if(msg_.num != 0) {
                                AirsrvCachedMsg& cachedMsg = cr.msg_map[remotePort];
                                cachedMsg.num = msg_.num;
                                cachedMsg.time = time(NULL);
                                cachedMsg.counter = 0;
                            }
                        }
                    }
                }
            }
        }
        switch (msg_.type) {
            case TLG_IN:
            case TLG_OUT:
                if (msg_.num == 0) // skip EXPRESS telegrams
                    return;
                monitor_idle_zapr_type(1, QUEPOT_TLG_INP);
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_F_ACK:
                /* !break; */
            case TLG_F_NEG:
            case TLG_CRASH:
            case TLG_ERR_TLG:
                if (msg_.num == 0) // skip EXPRESS telegrams
                    return;
                ProgTrace(TRACE5, "Sending TLG_ACK_ACK ack at %d tlg num %d", msg_.type, msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_ACK_ACK);
                break;
            case TLG_ACK:
                break;
            case TLG_ERR_CFG:
                break;
            case TLG_CONN_REQ:
                ProgTrace(TRACE5, "Sending TLG_CONN_RES ack at tlg num %d", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_CONN_RES);
                break;
            case TLG_FLOW_A_ON :
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_A_ON", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_FLOW_A_OFF:
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_A_OFF", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_FLOW_B_ON :
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_B_ON", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_FLOW_B_OFF:
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_B_OFF", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_FLOW_AB_ON :
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_AB_ON", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
            case TLG_FLOW_AB_OFF:
                ProgTrace(TRACE5, "Sending TLG_F_ACK ack at tlg num %d TLG_FLOW_AB_OFF", msg_.num);
                sendAnswerTo(msg_, ipFrom, answerPort, TLG_F_ACK);
                break;
        }
    }

    void handle_timer(const boost::system::error_code& err) {
        InitLogTime("AIRSRV");
        if (!err) {
            std::list<RouterInfo> routers;
            callbacks()->readAllRouters(routers);
            reinitAirsrvRoutersCache(routers, cachedRouters_, AirSrvSenderCachedRouterManager());
            timer_.expires_from_now(boost::posix_time::seconds(ReinitRoutersTimeout_));
            timer_.async_wait([this](auto&& error){ this->handle_timer(error); });
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            timer_.expires_from_now(boost::posix_time::seconds(ReinitRoutersTimeout_));
            timer_.async_wait([this](auto&& error){ this->handle_timer(error); });
        }
        airsrv_.rollback(); // критично для PG, для ORA не повредит.
    }
    void handle_socket(const boost::system::error_code& err, size_t bytes_transferred) {
        InitLogTime("AIRSRV");
        LogTrace(TRACE5) << "err=" << err << " bytes_transferred=" << bytes_transferred;
        if (!err) {
            msg_.num = ntohl(msg_.num);
            msg_.type = ntohs(msg_.type);
            msg_.TTL = ntohs(msg_.TTL);
            *((char*)(&msg_) + bytes_transferred) = 0; // final 0 in text
            processMessage();
            memset(&msg_, 0, sizeof(msg_));
            localSock_.async_receive_from(boost::asio::buffer(&msg_, max_msg_sz_), remoteEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        } else if (err == boost::asio::error::operation_aborted) {
            LogTrace(TRACE1) << __FUNCTION__ << " operation cancelled";
        } else {
            LogError(STDLOG) << __FUNCTION__ << " error: " << err;
            memset(&msg_, 0, sizeof(msg_));
            localSock_.async_receive_from(boost::asio::buffer(&msg_, max_msg_sz_), remoteEndpoint_, [this](auto&& error, auto&& bytes){ this->handle_socket(error,bytes); });
        }
    }
    AIRSRV_MSG msg_;
    const size_t ReinitRoutersTimeout_;
    static const size_t max_msg_sz_ = sizeof(AIRSRV_MSG) - 1;
    Airsrv& airsrv_;
    boost::asio::io_service& ios_;
    boost::asio::ip::udp::socket localSock_;
    boost::asio::ip::udp::endpoint remoteEndpoint_;
    boost::asio::deadline_timer timer_;
    std:: string tlgBeg_;
    static const size_t tlgBegLen_ = 9;
    AirSrvSenderCachedRouters cachedRouters_;
    ServerFramework::ControlPipeEvent control_;
};
}

namespace telegrams
{

Airsrv::Airsrv(int supervisorSocket, const char* portName, const Airsrv::SendersSockets& senders)
    : senders_(senders)
{
    set_signal(term3);
    pImpl_ = new AirsrvImpl(supervisorSocket, *this, portName);

    handlersMap_.emplace(TLG_IN,      [this](auto&& a, auto&& b, auto&& c){ return this->tlg_in(a,b,c); });
    handlersMap_.emplace(TLG_OUT,     [this](auto&& a, auto&& b, auto&& c){ return this->tlg_in(a,b,c); });
    handlersMap_.emplace(TLG_ACK,     [this](auto&& a, auto&& b, auto&& c){ return this->tlg_ack(a,b,c); });
    handlersMap_.emplace(TLG_F_ACK,   [this](auto&& a, auto&& b, auto&& c){ return this->tlg_f_ack(a,b,c); });
    handlersMap_.emplace(TLG_F_NEG,   [this](auto&& a, auto&& b, auto&& c){ return this->tlg_f_neg(a,b,c); });
    handlersMap_.emplace(TLG_ACK_ACK, [this](auto&& a, auto&& b, auto&& c){ return this->tlg_ack_ack(a,b,c); });
    handlersMap_.emplace(TLG_ERR_CFG, [this](auto&& a, auto&& b, auto&& c){ return this->tlg_err_cfg(a,b,c); });
    handlersMap_.emplace(TLG_ERR_TLG, [this](auto&& a, auto&& b, auto&& c){ return this->tlg_err_tlg(a,b,c); });
}

Airsrv::~Airsrv()
{
    AirsrvImpl* ai = static_cast<AirsrvImpl*>(pImpl_);
    delete ai;
}

void Airsrv::kickSender(const tlgnum_t& tlgNum)
{
    const int queueType = callbacks()->gatewayQueue().getQueueType(tlgNum);
    // tlg not found in queues for sending
    if (queueType < 0)
        return;
    Airsrv::SendersSockets::const_iterator it = senders_.find(Direction(queueType));
    if (it == senders_.end()) {
        LogTrace(TRACE1) << "no one sender will be kicked for tlg " << tlgNum << " from queue " << queueType;
        return;
    }

    LogTrace(TRACE5) << "tlg " << tlgNum << " in queue " << it->first << " must kick Sender socket " << it->second;
    sethAfter(ServerFramework::DaemonKicker(it->second));
}

int Airsrv::processExpiredTlg(const INCOMING_INFO& ii, const char* tlgBody)
{
    if (ii.isEdifact) {
        if (ii.hthInfo) {
            if (ii.hthInfo->part == 0 || ii.hthInfo->part == 1) {
                if (!callbacks()->ttlExpired(tlgBody, false, ii.router, ii.hthInfo))
                    return BAD_EDIFACT;
            } else {
                ProgTrace(TRACE1, "Tlg is expired HTH part - skipping CONTRL");
                return 0;
            }
        } else if (!callbacks()->ttlExpired(tlgBody, false, ii.router, ii.hthInfo))
            return BAD_EDIFACT;
    }
    return 0;
}

// размер списка номеров на которые приходили квитанции подтверждения ACK/F_ACK
static const size_t MAXACKED = 100;
bool Airsrv::wasAckForTlg(const tlgnum_t& tlgNum) const
{
    AckedTlgNums::const_iterator it = std::find(ackedNums_.begin(), ackedNums_.end(), tlgNum);
    return it != ackedNums_.end();
}
void Airsrv::ackForTlg(const tlgnum_t& tlgNum)
{
    if (ackedNums_.size() >= MAXACKED)
        ackedNums_.pop_front();
    ackedNums_.push_back(tlgNum);
}

int Airsrv::getQueueNumber(const char* /*tlgBody*/, bool/*ediTlg*/, int/*rtrIndex*/)
{
    return callbacks()->defaultHandler();
}

int Airsrv::tlg_in(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (ri.blockAnswers) {
        LogWarning(STDLOG) << "tlg " << tlgNum << " from router(" << ri.canonName << ") blocked due to configuration";

        return 0;
    }

    size_t bodySz = strlen(tlg.body);
    bool is_hth = false, edi_tlg = false;
    hth::HthInfo hthInfo = {};
    size_t hthHeadLen = hth::fromString(tlg.body, hthInfo);

    if (hthHeadLen > 0) {
        strcpy(tlg.body, tlg.body + hthHeadLen);
        hth::removeSpecSymbols(tlg.body);
        is_hth = true;
        hth::trace(TRACE5, hthInfo);
    }

    if (strlen(tlg.body) == 0) {
        LogWarning(STDLOG) << "skipping empty tlg " << tlgNum << " from " << ri.canonName
            << " bodySz=" << bodySz << " hthHeadLen=" << hthHeadLen;
        if (is_hth) {
            hth::trace(TRACE0, hthInfo);
        }

        return 0;
    }
    if (IsEdifactText(tlg.body, strlen(tlg.body))) {
        ProgTrace(TRACE5, "TLG is EDIFACT");
        edi_tlg = true;
    }

    INCOMING_INFO ii = {};
    ii.q_num = callbacks()->defaultHandler();
    ii.router = ri.ida;
    ii.isEdifact = edi_tlg;

    if (is_hth) {
        ii.hthInfo = hthInfo;
    }

    ii.ttl = tlg.TTL;
    if (tlg.TTL == 1) {
        LogWarning(STDLOG) << "Skipping telegram " << tlgNum << " expired TTL ";

        return processExpiredTlg(ii, tlg.body);
    }

    if (tlgNum.express) {
        return receiveExpressTlg(ri, (is_hth ? &hthInfo : NULL), tlg.TTL, tlg.body);
    }

    Expected<TlgResult, int> result = callbacks()->putTlg(tlg.body, tlg_text_filter(), ii.router, 0, ii.hthInfo ? &ii.hthInfo.get() : 0);
    if (!result) {
        LogError(STDLOG) << "putTlg failed with code " << result.err();
        return result.err();
    }

    tlgnum_t msgId = result->tlgNum;

    /// to be deleted
    afterReceiveTlg(msgId, tlg.body);

    const tlgnum_t oldMsgId(msgId);

    int ret = joinTlgParts(tlg.body, ri, ii, msgId, tlgNum);
    if (ret < 0) {
        return ret;
    }
    // part of telegram
    if (ret == 0) {
        return 0;
    }

    if (oldMsgId == msgId) {
        ii.q_num = getQueueNumber(tlg.body, edi_tlg, ri.ida);
    } else { // int telegram joined and msgId holds new msg_id
        std::string tlgText;
        ret = callbacks()->readTlg(msgId, tlgText);
        if (ret < 0) {
            return ret;
        }

        if (IsEdifactText(tlgText.c_str(), tlgText.size())) {
            ProgTrace(TRACE5, "TLG is EDIFACT");
            edi_tlg = true;
        }

        ii.q_num = getQueueNumber(tlgText.c_str(), edi_tlg, ri.ida);

        LogTrace(TRACE1) << "telegram joined " << tlgNum << " => newMsgId: " << msgId;
    }

    callbacks()->handlerQueue().addTlg(msgId, ii.q_num, ii.ttl);
    callbacks()->registerHandlerHook(ii.q_num);

    LogTrace(TRACE1) << "telegram " << tlgNum << " from " << ri.canonName << " => "
        << msgId << " edi_tlg: " << edi_tlg << " queueNumber: " << ii.q_num;

    return 0;
}

int Airsrv::tlg_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (tlgNum.express) {// EXPRESS telegrams
        return 0;
    }
    kickSender(tlgNum);
    ackForTlg(tlgNum);
    LogTrace(TRACE5) << "ACK received, moveTlgToQueue(" << tlgNum << ", =>WAIT_DELIV)";
    callbacks()->gatewayQueue().ackTlg(tlgNum); // -> WAIT_DELIV

    return 0;
}

int Airsrv::tlg_f_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    // если перед приходом TLG_F_ACK уже была принята TLG_ACK с таким же номером
    // то не следует пинать sender тк уже сделано
    // удаляем тлг, если еще не пинали sender, пинаем
    if (tlgNum.express) {// EXPRESS telegrams
        return 0;
    }
    if (!wasAckForTlg(tlgNum)) {
        kickSender(tlgNum);
        ackForTlg(tlgNum);
        LogTrace(TRACE1) << "ACK was not found for tlg: " << tlgNum;
    }

    LogTrace(TRACE5) << "TLG_F_ACK received, deleting tlg num " << tlgNum;
    callbacks()->gatewayQueue().delTlg(tlgNum);

    return 0;
}

int Airsrv::tlg_f_neg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (tlgNum.express) { // EXPRESS telegrams
        return 0;
    }

    LogTrace(TRACE1) << "TLG_F_NEG moveTlgToQueue(" << tlgNum << ", =>OTHER_OUT)";
    callbacks()->gatewayQueue().resendTlg(tlgNum); // -> OTHER_OUT

    return 0;
}

int Airsrv::tlg_ack_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (tlgNum.express) {// EXPRESS telegrams
        return 0;
    }

    LogTrace(TRACE5) << "TLG_ACK_ACK - do nothing";

    return 0;
}

int Airsrv::tlg_err_cfg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (tlgNum.express) {// EXPRESS telegrams
        return 0;
    }

    kickSender(tlgNum);
    LogError(STDLOG) << "TLG_ERR_CFG on tlg  " << tlgNum << " " << tlg.Receiver << "->" << tlg.Sender << "> => q_num=101, sys_s=REPEAT";
    callbacks()->errorQueue().addTlg(tlgNum, "GW CFG", 101, "ERR_CFG FROM GATEWAY"); // -> REPEAT

    return 0;
}

int Airsrv::tlg_err_tlg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri)
{
    if (tlgNum.express) {// EXPRESS telegrams
        return 0;
    }

    LogError(STDLOG) << "TLG_ERR_TLG on tlg  " << tlgNum << " " << tlg.Receiver << "->" << tlg.Sender << "> => q_num=101, sys_s=REPEAT";
    callbacks()->errorQueue().addTlg(tlgNum, "GW ERR", 101, "ERR_TLG FROM GATEWAY"); // -> REPEAT

    return 0;
}

int Airsrv::processTlg(AIRSRV_MSG& tlg, const RouterInfo& ri)
{
    tlgnum_t tlgNum(std::to_string(tlg.num));

    if (isExpressTlg(tlgNum, tlg.body)) {
        tlgNum.express = true;
    }

    HandlersMap::iterator i = handlersMap_.find(tlg.type);
    if (handlersMap_.end() != i) {
        return i->second(tlg, tlgNum, ri);
    } else {
        LogError(STDLOG) << "Unknown type (" << tlg.type << "): " << tlg.Sender << " -> " << tlg.Receiver << ", tlgnum: " << tlgNum;
        return TYPE_ERR;
    }
}

void Airsrv::afterReceiveTlg(const tlgnum_t&, const char* /*tlgText*/)
{
    ProgTrace(TRACE5, "stub: do nothing");
}

int Airsrv::run()
{
    AirsrvImpl* ai = static_cast<AirsrvImpl*>(pImpl_);
    ai->run();
    return 0;
}

void Airsrv::setHandler(telegrams::TLG_TYPE type, const HandlerType& handler) // add or replace, if exists
{
    handlersMap_[type] = handler;
}


void Airsrv::processMessage(AIRSRV_MSG& msg, const RouterInfo& ri)
{
    static const bool saveBadInputTlg = readIntFromTcl("AIRSRV_SAVE_BAD_TLG", 0);

    try {
        emptyHookTables();
        int ret = processTlg(msg, ri);
        if (ret < 0) {
            rollback();
            callRollbackPostHooks();
            ProgError(STDLOG, "processTlg()=%d, => do save_bad_tlg()", ret);

            if (saveBadInputTlg) {
                if (callbacks()->saveBadTlg(msg, ret)) {
                    commit();
                    ProgTrace(TRACE1, "bad tlg saved");
                } else {
                    rollback();
                    ProgError(STDLOG, "bad tlg not saved");
                    return;
                }
            }
        } else {
            callPostHooksBefore();
            commit();
            ProgTrace(TRACE5, "Commited");
            callPostHooksAfter();
            ProgTrace(TRACE5, "After hooks finished");
        }

    } catch (const comtech::Exception &e) {
        e.error(STDLOG);
        rollback();
    }
    callPostHooksAlways();
}

void Airsrv::commit()
{
    make_curs("commit").exec();
}

void Airsrv::rollback()
{
    make_curs("rollback").exec();
}

}   //  telegrams

#ifdef XP_TESTING

#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>
#include <serverlib/tcl_utils.h>

void initAirsrvTests()
{}

namespace
{
void startAirsrvTests()
{
}
void finishAirsrvTests()
{
}


START_TEST(check_reinitRoutersCache)
{
    std::list<RouterInfo> routers;
    AirSrvSenderCachedRouters cachedRouters;
#define ADD_ROUTER(id, name) \
    { \
        RouterInfo ri; \
        ri.ida = id; \
        ri.canonName = name; \
        routers.push_back(ri); \
    }
    ADD_ROUTER(1, "ROT01");
    ADD_ROUTER(2, "ROT02");
    ADD_ROUTER(3, "ROT03");
    ADD_ROUTER(4, "ROT04");
    ADD_ROUTER(5, "ROT05");

    const ushort port1 = 7878,
                 port2 = 7879;

    reinitAirsrvRoutersCache(routers, cachedRouters, AirSrvSenderCachedRouterManager());
    fail_unless(routers.size() == cachedRouters.size(), "size differ");
    fail_unless(cachedRouters.size() == 5, "size != 5");
    fail_unless(cachedRouters["ROT01"].msg_map[port1].counter == 0, "init bad counter");
    fail_unless(cachedRouters["ROT02"].msg_map[port1].num == 0, "init bad num");
    fail_unless(cachedRouters["ROT04"].msg_map[port2].counter == 0, "init bad counter");
    fail_unless(cachedRouters["ROT05"].msg_map[port2].num == 0, "init bad num");

    cachedRouters["ROT01"].msg_map[port1].counter = 2;
    cachedRouters["ROT02"].msg_map[port1].counter = 3;
    cachedRouters["ROT03"].msg_map[port2].counter = 1;
    cachedRouters["ROT04"].msg_map[port2].counter = 4;
    cachedRouters["ROT05"].msg_map[port2].num = 55;
    cachedRouters["ROT05"].msg_map[port1].num = 155;
    routers.erase(routers.begin());
    cachedRouters.erase(cachedRouters.find("ROT02"));
    reinitAirsrvRoutersCache(routers, cachedRouters, AirSrvSenderCachedRouterManager());
    fail_unless(routers.size() == cachedRouters.size(), "size differ: %d %d", routers.size(), cachedRouters.size());
    fail_unless(cachedRouters.size() == 4, "size != 4");
    fail_unless(cachedRouters["ROT01"].msg_map[port1].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT01"].msg_map[port2].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT02"].msg_map[port1].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT02"].msg_map[port2].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT03"].msg_map[port1].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT03"].msg_map[port2].counter == 1, "counter not copied");
    fail_unless(cachedRouters["ROT04"].msg_map[port1].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT04"].msg_map[port2].counter == 4, "counter not copied");
    fail_unless(cachedRouters["ROT05"].msg_map[port1].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT05"].msg_map[port2].counter == 0, "counter not copied");
    fail_unless(cachedRouters["ROT05"].msg_map[port2].num == 55, "num not copied");
    fail_unless(cachedRouters["ROT05"].msg_map[port1].num == 155, "num not copied");

}END_TEST

START_TEST(check_repeatMessageStructs)
{
#define PROC_TLG(tlgnum, port) \
{ \
    AirsrvCachedMsg& cachedMsg = cr.msg_map[port]; \
    if(cachedMsg.num == tlgnum) \
    { \
        cachedMsg.counter++; \
    } \
    else \
    { \
        cachedMsg.num = tlgnum; \
        cachedMsg.time = time(NULL); \
        cachedMsg.counter = 0; \
    } \
}

    const ushort port = 7878;
    AirsrvCachedRouter cr;
    PROC_TLG(123456, port);
    PROC_TLG(123456, port);
    PROC_TLG(123456, port);
    fail_unless(cr.msg_map[port].counter == 2);
    fail_unless(cr.msg_map[port].num == 123456);
    PROC_TLG(123457, port);
    fail_unless(cr.msg_map[port].counter == 0);
    fail_unless(cr.msg_map[port].num == 123457);
#undef PROC_TLG
}END_TEST

START_TEST(check_fillTlgNum)
{
    setTclVar("TLGNUM_FILL_LEADING_ZEROES", "1");
    tlgnum_t n1("1");
    fail_unless(n1.num.get() == "0000000001");

    tlgnum_t n2(std::string(9, '1').c_str());
    fail_unless(n2.num.get() == "0111111111");
    try {
        tlgnum_t n4("11111111111");
        fail_if(1, "too large tlgnum (length > 10) - must throw");
    } catch (const comtech::Exception&) {
    }
} END_TEST

#define SUITENAME "telegrams::Airsrv"
TCASEREGISTER(startAirsrvTests, finishAirsrvTests)
{
    ADD_TEST(check_reinitRoutersCache);
    ADD_TEST(check_repeatMessageStructs);
}
TCASEFINISH

#undef SUITENAME
#define SUITENAME "libtlg"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_fillTlgNum);
}
TCASEFINISH

}

#endif // XP_TESTING
