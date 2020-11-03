#ifndef _SERVERLIB_BASE_FRONTEND_H_
#define _SERVERLIB_BASE_FRONTEND_H_


#include <set>
#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/optional/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "blev.h"
#include "msg_const.h"
#include "http_parser.h"
#include "a_utl.h"
#include "tclmon.h"

#define NICKNAME "MIXA"
#include "slogger.h"
#include "TlgLogger.h"

namespace Dispatcher {

template<typename T>
class Dispatcher;

namespace bf { // base frontend

bool chunkedTransferEncoding(const uint8_t* const headers, const size_t size);
size_t parseContentLength(const uint8_t* const headers, const size_t size);
bool consumeCRLF(boost::asio::streambuf& sbuf);
boost::optional<size_t> parseChunkHeader(const std::vector<uint8_t>& header);
std::vector<uint8_t> normalizeHttpRequestHeader(const std::vector<uint8_t>& buf);


template<typename D>
class BaseTcpFrontend
{
    class Connection :
        public std::enable_shared_from_this<Connection>
    {
        class Answer
        {
        public:
            Answer(std::vector<uint8_t>&& ansHead, const size_t headOffset, const uint16_t clientId,
                   const uint32_t messageId, std::vector<uint8_t>&& ansData, const uint64_t bi,
                   const std::size_t queueSize, const std::time_t waitTime, const bool pure) :
                head_(std::move(ansHead)),
                clientId_ { clientId },
                messageId_ { messageId },
                headOffset_(headOffset),
                data_(std::move(ansData)),
                balancerId_(bi),
                queueSize_(htonl(queueSize)),
                waitTime_(htonl(waitTime)),
                msgSize_(htonl(head_.size() - headOffset_ + data_.size())),
                buffers_()
            {
                if (!pure) {
                    buffers_.emplace_back(&balancerId_, sizeof(balancerId_)),
                    buffers_.emplace_back(&queueSize_, sizeof(queueSize_));
                    buffers_.emplace_back(&waitTime_, sizeof(waitTime_));
                    buffers_.emplace_back(&msgSize_, sizeof(msgSize_));
                }

                buffers_.emplace_back(head_.data() + headOffset_, head_.size() - headOffset_);
                buffers_.emplace_back(data_.data(), data_.size());
            }

            Answer(const uint16_t clientId, const uint32_t messageId, std::vector<uint8_t>&& answer) :
                head_(),
                clientId_ { clientId },
                messageId_ { messageId },
                headOffset_(0),
                data_(std::move(answer)),
                balancerId_(0),
                queueSize_(0),
                waitTime_(0),
                msgSize_(0),
                buffers_()
            {
                buffers_.emplace_back(data_.data(), data_.size());
            }

            uint16_t clientId() {
                return clientId_;
            }

            uint32_t messageId() {
                return messageId_;
            }

            const std::vector<boost::asio::const_buffers_1>& toBuffers() {
                return buffers_;
            }

            Answer(const Answer& other) = delete;
            Answer& operator=(const Answer&) = delete;

        private:
            std::vector<uint8_t> head_;
            const uint16_t clientId_; // original client id from XML header
            const uint32_t messageId_; // original message id from XML header
            const size_t headOffset_;
            std::vector<uint8_t> data_;
            const uint64_t balancerId_;
            const uint32_t queueSize_;
            const uint32_t waitTime_;
            const uint32_t msgSize_;
            std::vector<boost::asio::const_buffers_1> buffers_;
        };

    public:
        Connection(boost::asio::io_service& io_s, const bool p /*pure protocol?*/) :
            socket_(io_s),
            balancerId_(0),
            head_(),
            data_(),
            queue_(),
            remoteEndpoint_(),
            timeStamp_(),
            live_(true),
            waitRequests_(0),
            pure_(p)
        { }
        //-----------------------------------------------------------------------
        Connection(const Connection& ) = delete;
        Connection& operator=(const Connection& ) = delete;
        //-----------------------------------------------------------------------
        Connection(Connection&& ) = delete;
        Connection& operator=(Connection&& ) = delete;
        //-----------------------------------------------------------------------
        void shutdown() {
            boost::system::error_code e;

            live_ = false;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
            socket_.close(e);
        }
        //-----------------------------------------------------------------------
        ~Connection() {
            shutdown();
        }
        //-----------------------------------------------------------------------
        boost::asio::ip::tcp::socket& socket() {
            return socket_;
        }
        //-----------------------------------------------------------------------
        uint64_t& balancerId() {
            return balancerId_;
        }
        //-----------------------------------------------------------------------
        std::vector<uint8_t>& head() {
            return head_;
        }
        //-----------------------------------------------------------------------
        std::vector<uint8_t>& data() {
            return data_;
        }
        //-----------------------------------------------------------------------
        void resizeData(const size_t size) {
            data_.resize(size);
        }
        //-----------------------------------------------------------------------
        const std::string& peer() {
            return remoteEndpoint_;
        }
        //-----------------------------------------------------------------------
        boost::asio::mutable_buffers_1 peerToBuf() {
            assert(!pure_);

            remoteEndpoint_.resize(IDLENA + IDLENB - 2, '\0');
            return boost::asio::mutable_buffers_1(&remoteEndpoint_[0], remoteEndpoint_.size());
        }
        //-----------------------------------------------------------------------
        void shrinkPeer() {
            if (!pure_) {
                auto i = remoteEndpoint_.find('\0');
                if (std::string::npos != i) {
                    remoteEndpoint_.resize(i);
                }
            }
        }
        //-----------------------------------------------------------------------
        void updateAccessTime() {
            gettimeofday(&timeStamp_, 0);
        }
        //-----------------------------------------------------------------------
        unsigned waitRequests() const {
            return waitRequests_;
        }
        //-----------------------------------------------------------------------
        void beforeHandle() {
            updateAccessTime();
            ++waitRequests_;
        }
        //-----------------------------------------------------------------------
        void afterHandle() {
            updateAccessTime();
            --waitRequests_;
        }
        //-----------------------------------------------------------------------
        bool pure() const {
            return pure_;
        }
        //-----------------------------------------------------------------------
        bool operator<(const Connection& other) {
            return (this->timeStamp_.tv_sec != other.timeStamp_.tv_sec)
                        ? (this->timeStamp_.tv_sec < other.timeStamp_.tv_sec)
                        : (this->timeStamp_.tv_usec != other.timeStamp_.tv_usec)
                            ? (this->timeStamp_.tv_usec < other.timeStamp_.tv_usec)
                            : (this->waitRequests_ != other.waitRequests_)
                                ? (this->waitRequests_ < other.waitRequests_)
                                : (this < &other);
        }
        //-----------------------------------------------------------------------
        bool setPeer() {
            boost::system::error_code error;
            const boost::asio::ip::tcp::endpoint& ep = socket_.remote_endpoint(error);

            if (error) {
                LogTrace(TRACE0)
                    << "Can't get remote endpoint from socket: "
                    << socket_.native_handle()
                    << ", error: "
                    << error
                    << " (" << error.message()
                    << ")";

                return false;
            }

            LogTrace(TRACE1) << "Socket descriptor: " << socket_.native_handle() << " == " << ep << ": " << this;
            ServerFramework::utl::stringize(ep).swap(remoteEndpoint_);
            return true;
        }
        //-----------------------------------------------------------------------
        void writeAnswer(std::vector<uint8_t>& ansHead,
                         const size_t headOffset,
                         const uint16_t clientId /*original client id from XML header*/,
                         const uint32_t messageId /*original message id from XML header*/,
                         std::vector<uint8_t>& ansData,
                         const uint64_t balancerId,
                         const std::size_t queueSize,
                         const std::time_t waitTime) {
            LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

            if (!live_) {
                LogTrace(TRACE1) << __FUNCTION__ << ": connection_id: " << this << " already shutdown";
                return;
            }

            queue_.emplace(std::move(ansHead), headOffset, clientId, messageId,
                           std::move(ansData), balancerId, queueSize, waitTime, this->pure());
            if (1 == queue_.size()) {
                boost::asio::async_write(
                        socket_,
                        queue_.front().toBuffers(),
                        boost::bind(
                            &Connection::handleWriteAnswer,
                            this->shared_from_this(),
                            boost::asio::placeholders::error
                        )
                );
            }
        }
        //-----------------------------------------------------------------------
        void writeAnswer(const uint16_t clientId, const uint32_t messageId, std::vector<uint8_t>& answer) {
            LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

            if (!live_) {
                LogTrace(TRACE1) << __FUNCTION__ << ": connection_id: " << this << " already shutdown";
                return;
            }

            queue_.emplace(clientId, messageId, std::move(answer));
            if (1 == queue_.size()) {
                boost::asio::async_write(
                        socket_,
                        queue_.front().toBuffers(),
                        boost::bind(
                            &Connection::handleWriteAnswer,
                            this->shared_from_this(),
                            boost::asio::placeholders::error
                        )
                );
            }
        }
        //-----------------------------------------------------------------------
        void handleWriteAnswer(const boost::system::error_code& e) {
            LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_.native_handle();

            if (!e) {
                if (D::proxy()) {
                    LogTlg() << "Answer to client id: " << queue_.front().clientId()
                             << " message id: " << queue_.front().messageId()
                             << " was sent.";
                } else {
                    LogTrace(TRACE1) << "Answer to client id: " << queue_.front().clientId()
                                     << " message id: " << queue_.front().messageId()
                                     << " was sent.";
                }

                queue_.pop();
                if (!live_) {
                    LogTrace(TRACE1) << __FUNCTION__ << ": connection_id: " << this << " already shutdown";
                    return;
                }

                if (!queue_.empty()) {
                    boost::asio::async_write(
                            socket_,
                            queue_.front().toBuffers(),
                            boost::bind(
                                &Connection::handleWriteAnswer,
                                this->shared_from_this(),
                                boost::asio::placeholders::error
                            )
                    );
                }
            } else {
                LogTrace(TRACE0) << __FUNCTION__ << ": " << socket_.native_handle() << " ( " << e << " ): "
                                 << e.message() << ". Queue size: " << queue_.size();
            }
        }
        //-----------------------------------------------------------------------
    private:
        boost::asio::ip::tcp::socket socket_;
        uint64_t balancerId_;
        std::vector<uint8_t> head_;
        std::vector<uint8_t> data_;
        std::queue<Answer> queue_;
        std::string remoteEndpoint_;
        struct timeval timeStamp_;
        bool live_;
        unsigned waitRequests_;
        const bool pure_;
    };
//-----------------------------------------------------------------------
    class ConnectionsSet
    {
        const size_t max_;

        using ConnectionPtr = std::shared_ptr<Connection>;
        struct Less { bool operator()(const ConnectionPtr& lhs, const ConnectionPtr& rhs) const { return *lhs < *rhs; } };

        std::set<ConnectionPtr, Less> connections_;
    public:
        ConnectionsSet(const size_t maxConn)
            : max_(maxConn ? maxConn - 1 : std::numeric_limits<size_t>::max()), connections_()
        { }

        ConnectionsSet(const ConnectionsSet& ) = delete;
        ConnectionsSet& operator=(const ConnectionsSet& ) = delete;

        ConnectionsSet(ConnectionsSet&& ) = delete;
        ConnectionsSet& operator=(ConnectionsSet&& ) = delete;

        void add(const ConnectionPtr& c) {
            if (max_ < connections_.size()) {
                const ConnectionPtr conn(*connections_.begin());
                connections_.erase(conn);
                conn->shutdown();
            }

            c->updateAccessTime();
            connections_.insert(c);
        }

        void beforeHandle(const ConnectionPtr& c) {
            if (connections_.erase(c)) {
                c->beforeHandle();
                connections_.insert(c);
            }
        }

        void afterHandle(const ConnectionPtr& c) {
            if (connections_.erase(c)) {
                c->afterHandle();
                connections_.insert(c);
            }
        }

        bool find(const ConnectionPtr& c) {
            return connections_.end() != connections_.find(c);
        }

        void erase(const ConnectionPtr& c) {
            c->shutdown();
            connections_.erase(c);
        }

        void eraseOneWait(const ConnectionPtr& c) {
            if (1 == c->waitRequests()) {
                c->shutdown();
                connections_.erase(c);
            } else {
                afterHandle(c);
            }
        }
    };

//-----------------------------------------------------------------------
public:
    typedef std::shared_ptr<Connection> KeyType;

public:
    BaseTcpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const boost::optional<std::pair<std::string, unsigned short> >& addr,
            const boost::optional<std::pair<std::string, unsigned short> >& balancerAddr,
            D& d,
            const size_t maxOpenConnections,
            std::set<uint16_t>&& traceableClientIds,
            const bool multiListener
    );
    //-----------------------------------------------------------------------
    BaseTcpFrontend(const BaseTcpFrontend& ) = delete;
    BaseTcpFrontend& operator=(const BaseTcpFrontend& ) = delete;
    //-----------------------------------------------------------------------
    BaseTcpFrontend(BaseTcpFrontend&& ) = delete;
    BaseTcpFrontend& operator=(BaseTcpFrontend&& ) = delete;
    //-----------------------------------------------------------------------
    ~BaseTcpFrontend();
    //-----------------------------------------------------------------------
    void sendAnswer(
            const uint64_t balancerId,
            const std::shared_ptr<Connection>& conn,
            std::vector<uint8_t>& answerHead,
            std::vector<uint8_t>& answerData,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void sendAnswer(
            const std::shared_ptr<Connection>& conn,
            std::vector<uint8_t>& answer
    );
    //-----------------------------------------------------------------------
    void timeoutExpired(
            const uint64_t balancerId,
            const std::shared_ptr<Connection>& conn,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void queueOverflow(
            const uint64_t balancerId,
            const std::shared_ptr<Connection>& conn,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void rmConnectionId(const std::shared_ptr<Connection>& conn) {
        connectionsSet_.eraseOneWait(conn);
    }
    //-----------------------------------------------------------------------
    uint32_t answeringNow(std::vector<uint8_t>& ansHeader);
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromSignal(const std::vector<uint8_t>& signal) {
        return blev_->signal_msgid(signal);
    }
    //-----------------------------------------------------------------------
    uint16_t clientIdFromHeader(const std::vector<uint8_t>& head) {
        return blev_->client_id(head.data());
    }
    //-----------------------------------------------------------------------
    uint32_t messageIdFromHeader(const std::vector<uint8_t>& head) {
        return blev_->message_id(head.data());
    }
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromHeader(const std::vector<uint8_t>& head) {
        static ServerFramework::MsgId id;

        assert((LEVBDATAOFF + sizeof(id.b)) <= head.size());
        memcpy(id.b, head.data() + LEVBDATAOFF, sizeof(id.b));

        return id;
    }
    //-----------------------------------------------------------------------
    void makeSignalSendable(const std::vector<uint8_t>& signal,
            std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData)
    {
        blev_->make_signal_sendable(signal, ansHead, ansData);
    }
    //-----------------------------------------------------------------------
    bool enableTrace(const uint16_t clientId);
    //-----------------------------------------------------------------------
    bool disableTrace(const uint16_t clientId);
    //-----------------------------------------------------------------------
private:
    void accept();
    //-----------------------------------------------------------------------
    void balancerAccept();
    //-----------------------------------------------------------------------
    void handleAccept(const boost::system::error_code& e);
    //-----------------------------------------------------------------------
    void readHead(const std::shared_ptr<Connection>& conn);
    //-----------------------------------------------------------------------
    void handleReadHead(
            const std::shared_ptr<Connection>& conn,
            const uint8_t* const startHead,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void handleReadData(const std::shared_ptr<Connection>& conn, const boost::system::error_code& e);
    //-----------------------------------------------------------------------
    void handleError(const char* functionName, const std::shared_ptr<Connection>& conn, const boost::system::error_code& e);
    //-----------------------------------------------------------------------
private:
    boost::asio::io_service& service_;
    const std::unique_ptr<ServerFramework::BLev> blev_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::acceptor balancerAcceptor_;
    D& dispatcher_;
    std::shared_ptr<Connection> newPureConnection_;
    std::shared_ptr<Connection> newBalancerConnection_;
    ConnectionsSet connectionsSet_;
    std::set<uint16_t> traceableClientIds_;
};
//-----------------------------------------------------------------------
template<typename D>
BaseTcpFrontend<D>::BaseTcpFrontend(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const boost::optional<std::pair<std::string, unsigned short> >& addr,
        const boost::optional<std::pair<std::string, unsigned short> >& balancerAddr,
        D& d,
        const size_t maxOpenConnections,
        std::set<uint16_t>&& traceableClientIds,
        const bool multiListener) :
    service_(io_s),
    blev_(ServerFramework::make_blev(headType)),
    acceptor_(io_s),
    balancerAcceptor_(io_s),
    dispatcher_(d),
    newPureConnection_(std::make_shared<Connection>(io_s, true)),
    newBalancerConnection_(std::make_shared<Connection>(io_s, false)),
    connectionsSet_(maxOpenConnections),
    traceableClientIds_(std::move(traceableClientIds))
{
    std::string curRequest;
    if (addr) {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(addr->first), addr->second);
        acceptor_.open(ep.protocol());
        acceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );

        if (D::proxy() && multiListener) {
            const int reuse = 1;
            assert(0 == ::setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)));
        }

        acceptor_.bind(ep);
        acceptor_.listen();

        accept();

        curRequest = ServerFramework::utl::stringize(ep);
    }

    if (balancerAddr) {
        boost::asio::ip::tcp::endpoint bep(boost::asio::ip::address::from_string(balancerAddr->first), balancerAddr->second);
        balancerAcceptor_.open(bep.protocol());
        balancerAcceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );
        balancerAcceptor_.bind(bep);
        balancerAcceptor_.listen();

        balancerAccept();

        curRequest += addr ? "-" : "";
        curRequest += ServerFramework::utl::stringize(bep);
    }

    write_set_cur_req(curRequest.c_str());
}
//-----------------------------------------------------------------------
template<typename D>
BaseTcpFrontend<D>::~BaseTcpFrontend()
{ }
//-----------------------------------------------------------------------
template<typename D>
uint32_t BaseTcpFrontend<D>::answeringNow(std::vector<uint8_t>& ansHeader)
{
    static const uint32_t MAX_TIME_WAIT = 768; //second
    uint32_t tmp = blev_->enqueable(ansHeader);

    assert(tmp < MAX_TIME_WAIT);

    return blev_->enqueable(ansHeader);
}
//-----------------------------------------------------------------------
template<typename D>
bool BaseTcpFrontend<D>::enableTrace(const uint16_t clientId)
{
    return traceableClientIds_.insert(clientId).second;
}
//-----------------------------------------------------------------------
template<typename D>
bool BaseTcpFrontend<D>::disableTrace(const uint16_t clientId)
{
    return 0 < traceableClientIds_.erase(clientId);
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::accept()
{
    LogTrace(TRACE7) << __FUNCTION__;

    acceptor_.async_accept(newPureConnection_->socket(),
            boost::bind(&BaseTcpFrontend<D>::handleAccept, this, boost::asio::placeholders::error));
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::balancerAccept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    balancerAcceptor_.async_accept(newBalancerConnection_->socket(),
            [this](const boost::system::error_code& e) {
                LogTrace(TRACE5) << __FUNCTION__ << ": " << newBalancerConnection_->socket().native_handle();

                if (!e) {
                    connectionsSet_.add(newBalancerConnection_);
                    readHead(newBalancerConnection_);
                } else {
                    LogTrace(TRACE5) << __FUNCTION__ << ": " << newBalancerConnection_->socket().native_handle()
                                     << ": error(" << e << ") - " << e.message();
                }

                newBalancerConnection_.reset(new Connection(service_, false));
                balancerAccept();
            });
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::handleAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << newPureConnection_->socket().native_handle();

    if (!e) {
        if (newPureConnection_->setPeer()) {
            connectionsSet_.add(newPureConnection_);
            readHead(newPureConnection_);
        }
    } else {
        LogTrace(TRACE7) << __FUNCTION__ << ": " << newPureConnection_->socket().native_handle() << ": error(" << e << ") - " << e.message();
    }

    newPureConnection_.reset(new Connection(service_, true));
    accept();
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::readHead(const std::shared_ptr<Connection>& conn)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn->socket().native_handle();

    uint8_t* const startHead = D::proxy()
        ? blev_->prepare_proxy_head(conn->head())
        : blev_->prepare_head(conn->head());

    std::vector<boost::asio::mutable_buffers_1> buf;
    if (!conn->pure()) {
        buf.emplace_back(&(conn->balancerId()), sizeof(conn->balancerId()));
        buf.emplace_back(conn->peerToBuf());
    }

    buf.emplace_back(startHead, blev_->hlen());
    boost::asio::async_read(
            conn->socket(),
            buf,
            boost::bind(&BaseTcpFrontend<D>::handleReadHead, this, conn, startHead, boost::asio::placeholders::error)
    );
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::handleReadHead(
        const std::shared_ptr<Connection>& conn,
        const uint8_t* const startHead,
        const boost::system::error_code& e)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        conn->shrinkPeer();

        if (D::proxy()) {
            LogTlg() << __FUNCTION__ << ": Received request header: {"
                     << " client_id: " << blev_->client_id(startHead)
                     << ", message_id: " << blev_->message_id(startHead)
                     << ", remote endpoint: " << conn->peer()
                     << " }";
        } else {
            LogTrace(TRACE1) << __FUNCTION__ << ": Received request header: {"
                             << " client_id: " << blev_->client_id(startHead)
                             << ", message_id: " << blev_->message_id(startHead)
                             << ", remote endpoint: " << conn->peer()
                             << " }";
        }

        const uint32_t dataSize = blev_->blen(startHead);
        if ((1024 * 1024 * 32) < dataSize) {
            boost::system::error_code error;
            conn->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
            connectionsSet_.erase(conn);
            LogError(STDLOG) << __FUNCTION__ << ": dataSize = " << dataSize << " remote endpoint: " << conn->peer();
            return;
        }

        conn->resizeData(dataSize);
        boost::asio::async_read(
                conn->socket(),
                boost::asio::mutable_buffers_1(conn->data().data(), conn->data().size()),
                boost::bind(
                    &BaseTcpFrontend<D>::handleReadData,
                    this,
                    conn,
                    boost::asio::placeholders::error
                )
        );
    } else {
        handleError(__FUNCTION__, conn, e);
    }
}
//-----------------------------------------------------------------------
struct TraceFlags
{
    const uint8_t first;
    const uint8_t second;
};
std::ostream& operator<<(std::ostream& os, const TraceFlags& tf);

static void traceHeader(const std::set<uint16_t>& traceableClientIds, const std::vector<uint8_t>& head, const char* const beginning)
{
    static constexpr unsigned CREATION_TIME_OFFSET = 4u;
    static constexpr unsigned MESSAGE_ID_OFFSET = CREATION_TIME_OFFSET + 4u;
    static constexpr unsigned CLIENT_ID_OFFSET = 44u;
    static constexpr unsigned FIRST_FLAG_OFFSET = CLIENT_ID_OFFSET + 2u;
    static constexpr unsigned SECOND_FLAG_OFFSET = FIRST_FLAG_OFFSET + 1u;
    static constexpr unsigned SYM_KEY_ID_OFFSET = SECOND_FLAG_OFFSET + 1u;

    if (head.size() < (SYM_KEY_ID_OFFSET + sizeof(uint32_t))) {
        LogError(STDLOG) << "Invalid head size: " << head.size();
        return;
    }

    const uint16_t clientId = ntohs(*reinterpret_cast<const uint16_t*>(head.data() + CLIENT_ID_OFFSET));

    if (!traceableClientIds.count(clientId)) {
        return;
    }

    const time_t creationTime = ntohl(*reinterpret_cast<const uint32_t*>(head.data() + CREATION_TIME_OFFSET));
    const auto creationTimeAsString { boost::posix_time::to_iso_extended_string(boost::posix_time::from_time_t(creationTime)) };

    LogTlg() << beginning << "{"
             << " data lenght: " << ntohl(*reinterpret_cast<const uint32_t*>(head.data()))
             << ", creation time: " << creationTime << " (" <<  creationTimeAsString << ")"
             << ", message id: " << ntohl(*reinterpret_cast<const uint32_t*>(head.data() + MESSAGE_ID_OFFSET))
             << ", client id: " << clientId
             << ", flag bytes: " << TraceFlags { head[ FIRST_FLAG_OFFSET ], head[ SECOND_FLAG_OFFSET ] }
             << ", sym key id: " << ntohl(*reinterpret_cast<const uint32_t*>(head.data() + SYM_KEY_ID_OFFSET))
             << " }";
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::handleReadData(const std::shared_ptr<typename BaseTcpFrontend<D>::Connection>& conn,
                                        const boost::system::error_code& e)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn->socket().native_handle();

    if (!e) {
        if (!D::proxy()) {
            struct timeval tmstamp;

            gettimeofday(&tmstamp, 0);
            blev_->ok_build_all_the_stuff(conn->head(), tmstamp);
            blev_->fill_endpoint(conn->head(), conn->peer());
        } else {
            traceHeader(traceableClientIds_, conn->head(), "Received request header: ");
        }

        connectionsSet_.beforeHandle(conn);
        dispatcher_.takeRequest(
                conn->balancerId(),
                conn,
                conn->peer(),
                conn->head(),
                conn->data(),
                D::WHAT_DROP_IF_QUEUE_FULL::BACK
        );
        readHead(conn);
    } else {
        handleError(__FUNCTION__, conn, e);
    }
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::sendAnswer(
        const uint64_t balancerId,
        const std::shared_ptr<Connection>& conn,
        std::vector<uint8_t>& answerHead,
        std::vector<uint8_t>& answerData,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn;


    const ServerFramework::MsgId& msgId = msgIdFromHeader(answerHead);
    const size_t offset = blev_->filter(answerHead);
    const uint16_t clientId = blev_->client_id(answerHead.data() + offset);
    const uint32_t messageId = blev_->message_id(answerHead.data() + offset);

    LogTrace(TRACE7) << __FUNCTION__ << ": connection_id: " << conn
                     << " successfully found for client_id: " << clientId
                     << " message_id: " << messageId
                     << " msg_id: " << msgId;

    connectionsSet_.afterHandle(conn);
    conn->writeAnswer(answerHead, offset, clientId, messageId, answerData, balancerId, queueSize, waitTime);
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::sendAnswer(
        const std::shared_ptr<Connection>& conn,
        std::vector<uint8_t>& answer)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn;

    if (D::proxy()) {
        traceHeader(traceableClientIds_, answer, "Sending response header: ");
    }

    connectionsSet_.afterHandle(conn);

    const uint16_t clientId = blev_->client_id(answer.data());
    const uint32_t messageId = blev_->message_id(answer.data());

    conn->writeAnswer(clientId, messageId, answer);
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::timeoutExpired(
        const uint64_t balancerId,
        const std::shared_ptr<Connection>& conn,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn;

    blev_->make_expired(head, data);
    sendAnswer(balancerId, conn, head, data, queueSize, waitTime);
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::queueOverflow(
        const uint64_t balancerId,
        const std::shared_ptr<Connection>& conn,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE7) << __FUNCTION__ << ": " << conn;

    if (D::proxy()) {
        head.emplace(std::begin(head), 0 /*should be "head type"*/);
        blev_->make_excrescent(head, data);
        head.erase(std::begin(head));

        head.reserve(head.size() + data.size());
        head.insert(std::end(head), std::begin(data), std::end(data));
        sendAnswer(conn, head);
    } else {
        blev_->make_excrescent(head, data);
        sendAnswer(balancerId, conn, head, data, queueSize, waitTime);
    }
}
//-----------------------------------------------------------------------
template<typename D>
void BaseTcpFrontend<D>::handleError(const char* functionName, const std::shared_ptr<BaseTcpFrontend<D>::Connection>& conn,
                              const boost::system::error_code& e)
{
    connectionsSet_.erase(conn);
    LogTrace(TRACE0) << functionName << ": Remove native connection: "
                     << conn->socket().native_handle()
                     << " endpoint: " << conn->peer()
                     << " conn: " << conn
                     << ". Error: " << "(" << e << "): " << e.message();
}

//-----------------------------------------------------------------------

template<typename T, typename D>
class BaseHttpFrontend
{
protected:
    template<typename S>
    class Connection
    {
    public:
        Connection(boost::asio::io_service& io_s) :
            socket_(io_s),
            sBuf_(),
            request_()
        { }

        Connection(boost::asio::io_service& io_s, boost::asio::ssl::context& context) :
            socket_(io_s, context),
            sBuf_(),
            request_()
        { }

        Connection(const Connection& ) = delete;
        Connection& operator=(const Connection& ) = delete;

        Connection(Connection&& ) = delete;
        Connection& operator=(Connection&& ) = delete;

        virtual ~Connection()
        {
            boost::system::error_code e;
            socket_.lowest_layer().shutdown(T::lowest_layer_type::shutdown_both, e);
        }

        S& socket() {
            return socket_;
        }

        std::vector<uint8_t>& headers() {
            return request_.headers;
        }

        std::vector<uint8_t>& body() {
            return request_.body;
        }

        boost::asio::streambuf& streamBuf() {
            return sBuf_;
        }

    private:
        S socket_;
        boost::asio::streambuf sBuf_;
        struct {
            std::vector<uint8_t> headers;
            std::vector<uint8_t> body;
        } request_;
    };

private:
    typedef std::set<std::shared_ptr<Connection<T> > > ClientConnectionSet;
    typedef boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type> BufIt;

public:
    typedef typename ClientConnectionSet::key_type KeyType;

public:
    BaseHttpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            D& d
    );
    //-----------------------------------------------------------------------
    BaseHttpFrontend(const BaseHttpFrontend& ) = delete;
    BaseHttpFrontend& operator=(const BaseHttpFrontend& ) = delete;
    //-----------------------------------------------------------------------
    BaseHttpFrontend(BaseHttpFrontend&& ) = delete;
    BaseHttpFrontend& operator=(BaseHttpFrontend&& ) = delete;
    //-----------------------------------------------------------------------
    virtual ~BaseHttpFrontend()
    { }
    //-----------------------------------------------------------------------
    void sendAnswer(
            const uint64_t ,
            const KeyType& clientConnectionId,
            std::vector<uint8_t>& answerHead,
            std::vector<uint8_t>& answerData,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void timeoutExpired(
            const uint64_t balancerId,
            const KeyType& clientConnectionId,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void queueOverflow(
            const uint64_t balancerId,
            const KeyType& clientConnectionId,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void rmConnectionId(const KeyType& clientConnectionId) {
        waitAnswerClients_.erase(clientConnectionId);
    }
    //-----------------------------------------------------------------------
    uint32_t answeringNow(std::vector<uint8_t>& ansHeader);
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromSignal(const std::vector<uint8_t>& signal);
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromHeader(const std::vector<uint8_t>& head);
    //-----------------------------------------------------------------------
    void makeSignalSendable(const std::vector<uint8_t>& signal,
            std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData);
    //-----------------------------------------------------------------------
protected:
    //-----------------------------------------------------------------------
    void readHeaders(const std::shared_ptr<Connection<T> >& conn);
    //-----------------------------------------------------------------------
private:
    static bool endOfCRLF(const boost::asio::streambuf& streamBuf, BufIt& endCRLF);
    //-----------------------------------------------------------------------
    static size_t endOfCRLFCRLF(const boost::asio::streambuf& streamBuf);
    //-----------------------------------------------------------------------
    void handleReadHeaders(
            const std::shared_ptr<Connection<T> >& conn,
            size_t bytes_transferred,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void readBody(const std::shared_ptr<Connection<T> >& conn);
    //-----------------------------------------------------------------------
    void handleReadBody(
            const std::shared_ptr<Connection<T> >,
            const size_t contentLen,
            const size_t bytes_transferred,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void readRequestComplete(const std::shared_ptr<Connection<T> >& conn);
    //-----------------------------------------------------------------------
    void handleWriteAnswer(
            const std::shared_ptr<Connection<T> >& conn,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void readChunkHeader(const std::shared_ptr<Connection<T> >& conn);
    //-----------------------------------------------------------------------
    void handleReadChunkHeader(
            const std::shared_ptr<Connection<T> >& conn,
            size_t bytes_transferred,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void readChunkData(const std::shared_ptr<Connection<T> >& conn, const size_t chunkSize);
    //-----------------------------------------------------------------------
    void handleReadChunkData(
            const std::shared_ptr<Connection<T> >& conn,
            const size_t chunkSize,
            const size_t bytes_transferred,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
    void readFromStreamBuf(uint8_t* buf, const size_t size, std::streambuf& streamBuf)
    {
        streamBuf.sgetn(reinterpret_cast<char*>(buf), size);
    }
    //-----------------------------------------------------------------------
    void readFromStreamBuf(std::vector<uint8_t>& buf, boost::asio::streambuf& streamBuf, const size_t size)
    {
        std::copy(BufIt::begin(streamBuf.data()), BufIt::begin(streamBuf.data()) + size, std::back_inserter(buf));
    }
    //-----------------------------------------------------------------------
protected:
    const uint8_t headType_;
    const pid_t pid_;
    boost::asio::io_service& service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    D& dispatcher_;
    ClientConnectionSet waitAnswerClients_;
};
//-----------------------------------------------------------------------
template<typename T, typename D>
BaseHttpFrontend<T, D>::BaseHttpFrontend(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const unsigned short port,
        D& d) :
    headType_(headType),
    pid_(getpid()),
    service_(io_s),
    acceptor_(io_s),
    dispatcher_(d),
    waitAnswerClients_()
{
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(addr), port);
    acceptor_.open(ep.protocol());
    acceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );
    acceptor_.bind(ep);
    acceptor_.listen();
    write_set_cur_req(ServerFramework::utl::stringize(ep).c_str());
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::sendAnswer(
        const uint64_t ,
        const KeyType& clientConnectionId,
        std::vector<uint8_t>& answerHead,
        std::vector<uint8_t>& answerData,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/
        )
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << clientConnectionId;

    typename ClientConnectionSet::iterator conn = waitAnswerClients_.find(clientConnectionId);

    if (waitAnswerClients_.end() != conn) {

        (*conn)->headers().swap(answerHead);
        (*conn)->body().swap(answerData);
        boost::asio::async_write(
                (*conn)->socket(),
                boost::asio::buffer((*conn)->body()),
                boost::bind(
                    &BaseHttpFrontend::handleWriteAnswer, this,
                    *conn,
                    boost::asio::placeholders::error
                )
        );
        waitAnswerClients_.erase(conn);
    } else {
        LogTrace(TRACE1) << __FUNCTION__ << ": Connection " << clientConnectionId << " not find";
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::timeoutExpired(
        const uint64_t balancerId,
        const KeyType& clientConnectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    ServerFramework::HTTP::reply::stock_reply(
            ServerFramework::HTTP::reply::service_unavailable
    ).to_buffer(data);

    sendAnswer(balancerId, clientConnectionId, head, data, queueSize, waitTime);
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::queueOverflow(
        const uint64_t balancerId,
        const KeyType& clientConnectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    timeoutExpired(balancerId, clientConnectionId, head, data, queueSize, waitTime);
}
//-----------------------------------------------------------------------
template<typename T, typename D>
uint32_t BaseHttpFrontend<T, D>::answeringNow(std::vector<uint8_t>& ansHeader)
{
    assert(HTTP_HEADER_PREFIX == ansHeader.size());

    if (ansHeader[HTTP_HEADER_FLAGS] & MSG_ANSW_STORE_WAIT_SIG) {
        return *(reinterpret_cast<uint32_t*>(ansHeader.data() + HTTP_HEADER_TIMEOUT));
    }

    return 0;
}
//-----------------------------------------------------------------------
template<typename T, typename D>
const ServerFramework::MsgId& BaseHttpFrontend<T, D>::msgIdFromSignal(const std::vector<uint8_t>& signal)
{
    static ServerFramework::MsgId id;

    LogTrace(TRACE7) << __FUNCTION__;

    assert((4 * sizeof(uint32_t)) <= signal.size());
    const uint32_t* buf = reinterpret_cast<const uint32_t*>(signal.data());
    assert(*buf == htonl(1));

    memcpy(id.b, buf + 1, sizeof(id.b));

    LogTrace(TRACE7) << __FUNCTION__ << ": deduced { " << id.b[0] << ' '  << id.b[1] << ' ' << id.b[2] << " }";

    return id;
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::makeSignalSendable(const std::vector<uint8_t>& signal,
        std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData)
{
    static const uint8_t offset = 4 * sizeof(uint32_t);

    LogTrace(TRACE7) << __FUNCTION__;

    const size_t bodySize = signal.size() - offset;

    ansData.resize(bodySize);
    memcpy(ansData.data(), signal.data() + offset, bodySize);
    ansHead[HTTP_HEADER_FLAGS] = MSG_TYPE_PERESPROS;

    LogTrace(TRACE7) << __FUNCTION__ << ": done hdr: " << ansHead[0] << " msgid: { "
                     << signal[1] << ' '  << signal[2] << ' ' << signal[3] << " }";
}
//-----------------------------------------------------------------------
template<typename T, typename D>
const ServerFramework::MsgId& BaseHttpFrontend<T, D>::msgIdFromHeader(const std::vector<uint8_t>& head)
{
    static ServerFramework::MsgId id;

    assert((HTTP_HEADER_PREFIX <= head.size()));
    memcpy(id.b, head.data() + 1, sizeof(id.b));

    return id;
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::readHeaders(const std::shared_ptr<Connection<T> >& conn)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();
    static const boost::regex end_of_header_re("\r?\n\r?\n");

    boost::asio::async_read_until(
            conn->socket(),
            conn->streamBuf(),
            end_of_header_re,
            boost::bind(
                &BaseHttpFrontend::handleReadHeaders,
                this,
                conn,
                boost::asio::placeholders::bytes_transferred,
                boost::asio::placeholders::error
            )
    );
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::handleReadHeaders(
        const std::shared_ptr<Connection<T> >& conn,
        size_t bytes_transferred,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (!e) {
        BufIt end; //Random access iterator

        ProgTrace(TRACE5, "before bytes_transferred: %zu", bytes_transferred);
        bytes_transferred = BaseHttpFrontend<T, D>::endOfCRLFCRLF(conn->streamBuf());
        ProgTrace(TRACE5, "after bytes_transferred: %zu", bytes_transferred);

        conn->headers().resize(HTTP_HEADER_PREFIX, 0);
        conn->body().resize(bytes_transferred);
        readFromStreamBuf(conn->body().data(), bytes_transferred, conn->streamBuf());
        readBody(conn);
    } else {
        LogTrace(TRACE1) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": (" << e << ") - " << e.message();
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::readBody(const std::shared_ptr<Connection<T> >& conn)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (chunkedTransferEncoding(
                conn->body().data(), conn->body().size())) {
        readChunkHeader(conn);
    } else {
        const size_t contentLen = parseContentLength(conn->body().data(), conn->body().size());
        /* Сколько байт осталось дочитать */
        const size_t rest = (conn->streamBuf().size() < contentLen) ?
            contentLen - conn->streamBuf().size() : 0;

        LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": need read " << rest << " bytes";

        boost::asio::async_read(
                conn->socket(),
                conn->streamBuf(),
                boost::asio::transfer_at_least(rest),
                boost::bind(
                    &BaseHttpFrontend::handleReadBody,
                    this,
                    conn,
                    contentLen,
                    boost::asio::placeholders::bytes_transferred,
                    boost::asio::placeholders::error
                )
        );
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::handleReadBody(
        const std::shared_ptr<Connection<T> > conn,
        const size_t contentLen,
        const size_t bytes_transferred,
        const boost::system::error_code& e
)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (!e) {
        const size_t httpHeadersSize = conn->body().size();

        conn->body().resize(httpHeadersSize + contentLen);
        readFromStreamBuf(conn->body().data() + httpHeadersSize, contentLen, conn->streamBuf());
        readRequestComplete(conn);
    } else {
        LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": error( " << e << " ) - " << e.message();
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::readRequestComplete(const std::shared_ptr<Connection<T> >& conn)
{
    static unsigned msgidPart;

    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    waitAnswerClients_.insert(conn);
    conn->headers()[0] = headType_;
    *reinterpret_cast<uint32_t*>(conn->headers().data() + 1) = pid_;
    *reinterpret_cast<uint32_t*>(conn->headers().data() + 1 + sizeof(uint32_t)) = ++msgidPart;
    *reinterpret_cast<uint32_t*>(conn->headers().data() + 1 + sizeof(uint32_t) + sizeof(uint32_t)) = std::time(0);

    conn->body() = normalizeHttpRequestHeader(conn->body());
    dispatcher_.takeRequest(
            0,
            conn,
            std::string(),
            conn->headers(),
            conn->body(),
            D::WHAT_DROP_IF_QUEUE_FULL::BACK
    );
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::handleWriteAnswer(const std::shared_ptr<Connection<T> >& conn, const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (!e) {
        readHeaders(conn);
    } else if (boost::asio::error::broken_pipe == e) {
        LogTrace(TRACE0) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle()
                         << ": error( " << e << " ) - " << e.message();
    } else {
        LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle()
            << ": error( " << e << " ) - " << e.message();
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::readChunkHeader(const std::shared_ptr<Connection<T> >& conn)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    const BufIt end(BufIt::end(conn->streamBuf().data()));
    const BufIt begin(BufIt::begin(conn->streamBuf().data()));
    std::string endof_header = "\r\n";
    BufIt crlf = std::search(begin, end, endof_header.data(), endof_header.data() + 2);

    if(end == crlf) {
        endof_header = "\n";
        crlf = std::search(begin, end, endof_header.data(), endof_header.data() + 1);
    }

    if (end != crlf) {
        handleReadChunkHeader(
                conn,
                std::distance(begin, crlf) + endof_header.length(), // + 2 it is "\r\n"
                boost::system::error_code()
        );
    } else {
        static const boost::regex end_of_line_re("\r?\n");
        boost::asio::async_read_until(
                conn->socket(),
                conn->streamBuf(),
                end_of_line_re, //"\r\n" or "\n"
                boost::bind(
                    &BaseHttpFrontend::handleReadChunkHeader,
                    this,
                    conn,
                    boost::asio::placeholders::bytes_transferred,
                    boost::asio::placeholders::error
                )
        );
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::handleReadChunkHeader(
        const std::shared_ptr<Connection<T> >& conn,
        size_t bytes_transferred,
        const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (!e) {
        assert(bytes_transferred <= conn->streamBuf().size());

        BufIt end; //Random access iterator
        if(!endOfCRLF(conn->streamBuf(), end)) {
            bytes_transferred = std::distance(BufIt::begin(conn->streamBuf().data()), end);
        }

        std::vector<uint8_t> chunkHeader(bytes_transferred);

        readFromStreamBuf(chunkHeader.data(), bytes_transferred, conn->streamBuf());

        const boost::optional<size_t> chunkSize = parseChunkHeader(chunkHeader);
        if (!chunkSize) {
            LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": No chunk size in http request";
        } else if (*chunkSize == 0) {
            //End of content http message
            readRequestComplete(conn);
        } else {
            //Read chunk data
            readChunkData(conn, *chunkSize);
        }
    } else {
        LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": error( " << e << ") - " << e.message();
    }
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::readChunkData(const std::shared_ptr<Connection<T> >& conn, const size_t chunkSize)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    const size_t chunkWithCRLF = chunkSize + 2;
    /* Сколько байт не хватает в буфере? */
    const size_t transferAtLeast = (conn->streamBuf().size() > chunkWithCRLF) ? 0 : chunkWithCRLF - conn->streamBuf().size();

    boost::asio::async_read(
            conn->socket(),
            conn->streamBuf(),
            boost::asio::transfer_at_least(transferAtLeast),
            boost::bind(
                &BaseHttpFrontend::handleReadChunkData,
                this,
                conn,
                chunkSize,
                boost::asio::placeholders::bytes_transferred,
                boost::asio::placeholders::error
            )
    );
}
//-----------------------------------------------------------------------
template<typename T, typename D>
void BaseHttpFrontend<T, D>::handleReadChunkData(
        const std::shared_ptr<Connection<T> >& conn,
        const size_t chunkSize,
        const size_t bytes_transferred,
        const boost::system::error_code& e
)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle();

    if (!e) {
        assert(conn->streamBuf().size() >= chunkSize + 2);
        readFromStreamBuf(conn->body(), conn->streamBuf(), chunkSize);

        if (!consumeCRLF(conn->streamBuf())) {
            LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": No CRLF after chunk data";
        } else {
            readChunkHeader(conn);
        }
    } else {
        LogError(STDLOG) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle() << ": error( " << e << ") - " << e.message();
    }
}
//-----------------------------------------------------------------------
//In streamBuf must be a sequence containing "\r\n\r\n", maybe not at the end!
//Example - result async_read_until(..., "\r\n\r\n", ...);
template<typename T, typename D>
size_t BaseHttpFrontend<T, D>::endOfCRLFCRLF(const boost::asio::streambuf& streamBuf)
{
    const auto& data = streamBuf.data();
    std::string buf(BufIt::begin(data), BufIt::end(data));

    std::string delim = "\r\n\r\n";
    size_t idx = buf.find(delim);
    if(idx == std::string::npos) {
        delim = "\n\n";
        idx = buf.find(delim);
    }
    assert(idx != std::string::npos);
    return idx + delim.length();
}
//-----------------------------------------------------------------------
//In streamBuf must be a sequence containing "\r\n", maybe not at the end!
//For example, result async_read_until(..., "\r\n", ...);
template<typename T, typename D>
bool BaseHttpFrontend<T, D>::endOfCRLF(const boost::asio::streambuf& streamBuf, BufIt& endCRLF)
{
    endCRLF = BufIt::end(streamBuf.data());
    BufIt i(endCRLF - 1);

    if(*i == '\n') {
        return true;
    } else {
        endCRLF = std::find(BufIt::begin(streamBuf.data()), endCRLF, '\n') + 1;
        return false;
    }
}

//-----------------------------------------------------------------------

template< typename D >
class BasePlainHttpFrontend :
    public bf::BaseHttpFrontend< boost::asio::ip::tcp::socket, D >
{
    using SocketType = boost::asio::ip::tcp::socket;
    using BaseType = bf::BaseHttpFrontend< SocketType, D >;

public:
    BasePlainHttpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            D& d):
        bf::BaseHttpFrontend< typename BasePlainHttpFrontend< D >::SocketType, D >(io_s, headType, addr, port, d),
        newConnection_(new typename BaseType::template Connection< SocketType >(io_s))
    {
        accept();
    }

    ~BasePlainHttpFrontend() override { }

private:
    void accept() {
        LogTrace(TRACE5) << __FUNCTION__;

        BaseType::acceptor_.async_accept(
                newConnection_->socket(),
                boost::bind(
                    &BasePlainHttpFrontend::handleAccept,
                    this,
                    boost::asio::placeholders::error
                )
        );
    }

    void handleAccept(const boost::system::error_code& e) {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << newConnection_->socket().native_handle();

        if (!e) {
            BaseType::readHeaders(newConnection_);
        } else {
            LogError(STDLOG) << __FUNCTION__ << newConnection_->socket().native_handle()
                << ": " << ": error( " << e << " ) - " << e.message();
        }

        newConnection_.reset(new typename BaseType::template Connection< SocketType >(BaseType::service_));
        accept();
    }

private:
    std::shared_ptr< typename BaseType::template Connection< SocketType > > newConnection_;
};

} //namespace bf

} //namespace Dispatcher

#undef NICKNAME

#endif //_SERVERLIB_BASE_FRONTEND_H_
