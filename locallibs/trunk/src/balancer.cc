#include <sys/mman.h>

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/random.hpp>

#include "monitor_ctl.h"
#include "daemon_event.h"
#include "str_utils.h"
#include "tcl_list_utils.h"
#include "monitor.h"
#include "dates.h"

#include "balancer.h"
#include "http_parser.h"
#include "balancer_frontend.h"
#include <serverlib/json_pack_types.h>
#include <serverlib/json_packer_heavy.h>

#define NICKNAME "MIXA"
#include "slogger.h"

namespace json_spirit {

JSON_DESC_TYPE_DECL(balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::OutgoingStats);
JSON_DESC_TYPE_DECL(balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::Stats);
JSON_DESC_TYPE_DECL(balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::ClientStats);

using OutgoingStats = balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::OutgoingStats;
JSON_BEGIN_DESC_TYPE(OutgoingStats)
    DESC_TYPE_FIELD("count", count)
    DESC_TYPE_FIELD("waitTimeAvg", waitTimeAvg)
JSON_END_DESC_TYPE(OutgoingStats)

using Stats = balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::Stats;
JSON_BEGIN_DESC_TYPE(Stats)
    DESC_TYPE_FIELD("incoming", incoming)
    DESC_TYPE_FIELD("outgoing", outgoing)
JSON_END_DESC_TYPE(Stats)

using ClientStats = balancer::ClientStatsAssistant< balancer::ClientStatsFrontend >::ClientStats;
JSON_BEGIN_DESC_TYPE(ClientStats)
    DESC_TYPE_FIELD("begin_utc", beginUtc)
    DESC_TYPE_FIELD("end_utc", endUtc)
    DESC_TYPE_FIELD("stats", stats)
JSON_END_DESC_TYPE(ClientStats)

}

namespace balancer {

template<typename T>
unsigned Balancer<T>::k1_;

template<typename T>
unsigned Balancer<T>::k2_;

static int tcl_read_config_nop(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int tcl_read_config(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);

static std::string sharedMemoryName()
{
    auto shm = readStringFromTcl(current_group2() + "(SHARED_MEMORY_NAME)", "/balancer.shm");
    if (!shm.empty()) {
        const char slash = '/';
        if (slash != shm.front()) {
            shm.insert(0, 1, slash);
        }

        std::replace(std::begin(shm) + 1, std::end(shm), slash, '_');
    }

    return shm;
}

static bool enableClientStats()
{
    static const bool enable = readIntFromTcl(current_group2() + "(ENABLE_CLIENTS_STATS)", 0);
    return enable;
}

template<typename T>
Balancer<T>::Connection::Connection(boost::asio::io_service& ios, T& f, Node& n,
                                    const std::string& h, const std::string& s)
    : service_(ios),
      frontend_(f),
      node_(n),
      socket_(),
      queue_(),
      query_(h, s, boost::asio::ip::tcp::resolver::query::address_configured
                   | boost::asio::ip::tcp::resolver::query::numeric_service),
      ep_(),
      queueSizeBuf_(0),
      waitTimeBuf_(0),
      msgSize_(0),
      nodeStatBuf_(),
      state_(State::Reconnect),
      attemptCount_(0)
{
    nodeStatBuf_.emplace_back(&ccid_, sizeof(ccid_));
    nodeStatBuf_.emplace_back(&queueSizeBuf_, sizeof(queueSizeBuf_));
    nodeStatBuf_.emplace_back(&waitTimeBuf_, sizeof(waitTimeBuf_));
    nodeStatBuf_.emplace_back(&msgSize_, sizeof(msgSize_));

    resolve();
}

template<typename T>
Balancer<T>::Connection::~Connection() {
    shutdown(true);
}

template<typename T>
void Balancer<T>::Connection::finalized() {
    state_ = State::Final;
}

template<typename T>
void Balancer<T>::Connection::write() {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_->native_handle() << ' '
                     << query_.host_name() << ':' << query_.service_name();

    if (State::Reconnect == state_) {
        LogTrace(TRACE1) << __FUNCTION__ << ": connection_id: " << this
                         << " not connected right now: " << static_cast<int>(state_);
        return;
    }

    if (!queue_.empty()) {
        boost::asio::async_write(
                *socket_,
                queue_.front().toBuffers(),
                boost::bind(
                    &Balancer<T>::Connection::handleWrite,
                    this->shared_from_this(),
                    boost::asio::placeholders::error
                )
        );
    }
}

template<typename T>
void Balancer<T>::Connection::write(const typename T::KeyType& ccid /*client connection id*/,
                                    const uint16_t clientId /*original client id from XML header*/,
                                    const uint32_t messageId /*original message id from XML header*/,
                                    const std::string& rep /*remote endpoint*/,
                                    std::vector<uint8_t>& head, std::vector<uint8_t>& data) {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_->native_handle() << ' '
                     << query_.host_name() << ':' << query_.service_name();

    static uint64_t c;

    queue_.emplace(++c, ccid, clientId, messageId, rep, head, data);

    if (1 == queue_.size()) {
        write();
    }
}

template<typename T>
void Balancer<T>::Connection::wait(void (Balancer<T>::Connection::*f)()) {
    static constexpr long ML_SECOND = 1024;
    static boost::random::mt19937 gen(std::time(0));
    //Количество милисекунд, в течение которых процесс "не проснется"
    static boost::random::uniform_int_distribution<long> distribution(ML_SECOND / 2, ML_SECOND);

    auto timer = std::make_shared<boost::asio::deadline_timer>(service_);
    timer->expires_from_now(boost::posix_time::milliseconds(distribution(gen)));
    timer->async_wait([this, timer, f](const boost::system::error_code& ec) { (this->*f)(); });
}

template<typename T>
void Balancer<T>::Connection::resolve() {
    static constexpr unsigned ATTEMPT_COUNT = 4;

    if (ATTEMPT_COUNT == ++attemptCount_) {
        LogError(STDLOG) << "Can't resolve host " << query_.host_name();
        attemptCount_ = 0;
    }

    auto resolver = std::make_shared<boost::asio::ip::tcp::resolver>(service_);
    auto timer = std::make_shared<boost::asio::deadline_timer>(service_);
    resolver->async_resolve(query_,
            [this, resolver, timer](const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator it) {
                timer->cancel();
                if (!e) {
                    attemptCount_ = 0;
                    ep_ = std::make_shared<boost::asio::ip::tcp::endpoint>(it->endpoint());
                    connect();
                } else {
                    LogTrace(TRACE0) << "Attempt to resolve query "
                                     << query_.host_name() << ':' << query_.service_name()
                                     << " for " << (socket_ ? socket_->native_handle() : -1) << " failed: " << e.message();
                    this->wait(&Balancer::Connection::resolve);
                }
            });

    timer->expires_from_now(boost::posix_time::seconds(16));
    timer->async_wait([this, resolver, timer](const boost::system::error_code& ec){
        if(!ec) {
            LogError(STDLOG) << "Resolving host " << query_.host_name() << ':'
                             << query_.service_name() << " timed out";

            resolver->cancel();
        }
    });
}

template<typename T>
void Balancer<T>::Connection::connect() {
    static constexpr unsigned ATTEMPT_COUNT = 16;

    assert(ep_);
    assert(State::Reconnect == state_);

    if (ATTEMPT_COUNT == ++attemptCount_) {
        LogError(STDLOG) << "Can't established connection to " << query_.host_name()
                         << ':' << query_.service_name() << " socket: " << (socket_ ? socket_->native_handle() : -1);
        attemptCount_ = 0;
    }

    socket_ = std::make_shared<boost::asio::ip::tcp::socket>(service_);
    auto timer = std::make_shared<boost::asio::deadline_timer>(service_);
    socket_->async_connect(*ep_,
            [this, timer](const boost::system::error_code& e) {
                timer->cancel();
                if (!e) {
                    attemptCount_ = 0;
                    state_ = State::Live;
                    node_.move(this->shared_from_this());
                    this->read();
                    this->write();

                    LogTrace(TRACE1) << "Socket descriptor: " << socket_->native_handle()
                                     << " == " << query_.host_name() << ':' << query_.service_name()
                                     << " remote endpoint - " << *ep_ << ": " << this;
                } else {
                    LogTrace(TRACE0) << __FUNCTION__ << ": attempt to connect to "
                                     << query_.host_name() << ':' << query_.service_name()
                                     << " for socket " << socket_->native_handle()
                                     << " failed: " << e.message();
                    this->wait(&Balancer::Connection::connect);
                }
            });

    timer->expires_from_now(boost::posix_time::seconds(32));
    timer->async_wait([this, timer](const boost::system::error_code& ec){
        if(!ec) {
            boost::system::error_code e;

            LogError(STDLOG) << "Connection to " << query_.host_name()
                             << ':' << query_.service_name() << " timed out";

            this->socket_->cancel(e);
        }
    });
}

template<typename T>
void Balancer<T>::Connection::read() {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_->native_handle();

    boost::asio::async_read(*socket_,
            nodeStatBuf_,
            boost::bind(&Balancer<T>::Connection::handleReadStat,
                this->shared_from_this(),
                boost::asio::placeholders::error
            )
    );
}

template<typename T>
void Balancer<T>::Connection::handleReadStat(const boost::system::error_code& e) {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_->native_handle();

    if (e) {
        handleError(__FUNCTION__, e);
        return;
    }

    node_.nodeStats(ntohl(queueSizeBuf_), ntohl(waitTimeBuf_));
    msgSize_ = ntohl(msgSize_);

    auto buf = std::make_shared<std::vector<uint8_t> >(msgSize_);
    auto c = this->shared_from_this();

    boost::asio::async_read(*socket_,
            boost::asio::mutable_buffers_1(buf->data(), buf->size()),
            [this, c, buf](const boost::system::error_code& e, std::size_t ) {
                if (e) {
                    handleError(__FUNCTION__, e);
                    return;
                }

                const uint16_t clientIdFromHeader { frontend_.clientIdFromHeader(*buf) };
                const uint32_t msgIdFromHeader { frontend_.messageIdFromHeader(*buf) };

                LogTrace(TRACE1) << "Received answer header: {"
                                 << " client connection id: " << ccid_
                                 << ", client id: " << clientIdFromHeader
                                 << ", message id: " << msgIdFromHeader
                                 << " }";

                auto i = waitClientIds_.find(ccid_);
                if (std::end(waitClientIds_) != i) {
                    if (clientIdFromHeader != i->second.clientId) {
                        LogError(STDLOG) << "Unexpected client id from header: " << clientIdFromHeader
                                         << ". Expected " << i->second.clientId;
                    }

                    if (auto p = i->second.data.lock()) {
                        frontend_.sendAnswer(p, *buf);

                        const auto wt = (boost::posix_time::microsec_clock::local_time() - i->second.time).total_milliseconds();
                        node_.accountReply(i->second.clientId, wt);

                    } else {
                        LogTrace(TRACE0) << "Client connecion id: " << ccid_ << " already expired.";
                        LogTrace(TRACE0) << "Answer for client id: " << i->second.clientId << " will be removed.";
                    }

                    waitClientIds_.erase(i);
                } else {
                    LogTrace(TRACE0) << "Can't find client connection by " << ccid_;
                }

                c->read();
            });
}

template<typename T>
void Balancer<T>::Connection::handleError(const char* functionName, const boost::system::error_code& e)
{
    if (State::Final == state_) {
        LogTrace(TRACE0) << "Connection " << this << " to " << query_.host_name()
                         << ':' << query_.service_name() << " finalized";
        return;
    }


    LogTrace(TRACE0) << functionName << ": Re-connect to  "
                     << query_.host_name() << ':' << query_.service_name()
                     << " conn: " << this
                     << ". Error: " << "(" << e << "): " << e.message();
    shutdown(false);
    node_.stash(this->shared_from_this());

    connect();
}


template<typename T>
void Balancer<T>::Connection::handleWrite(const boost::system::error_code& e) {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << socket_->native_handle();

    if (!e) {
/* TODO expired connections
        auto r = waitClientIds_.equal_range(queue_.front().ccid());
        while (r.first != r.second) {
            if (r.first->second.expired()) {
                r.first = waitClientIds_.erase(r.first);
            } else {
                ++r.first;
            }
        }
*/

        LogTrace(TRACE1) << "Request from client id: " << queue_.front().clientId()
                         << " with message id: " << queue_.front().messageId()
                         << " was sent to node '" << node_.name() << "' from group '" << node_.group()
                         << "' using client connection id '" << queue_.front().ccid() << "'.";

        const auto now = boost::posix_time::microsec_clock::local_time();
        const auto res = waitClientIds_.emplace(queue_.front().ccid(),
                                                WaitClientIdData { queue_.front().clientId(),
                                                                   now,
                                                                   queue_.front().key() });
        node_.accountRequest(queue_.front().clientId());

        assert(res.second && "Duplicate ccid.");

        write_set_flag_type(0, 1, QUEPOT_LEVB, node_.group().c_str());
        queue_.pop();

        write();
    } else {
        LogTrace(TRACE0) << __FUNCTION__ << ": " << socket_->native_handle() << " ( " << e << " ): "
                         << e.message() << ". Queue size: " << queue_.size();
    }
}

template<typename T>
void Balancer<T>::Connection::shutdown(const bool f /*final*/) {
    boost::system::error_code e;

    state_ = f ? State::Final : State::Reconnect;
    for (auto c : waitClientIds_) {
        if (auto p = c.second.data.lock()) {
            frontend_.rmConnectionId(p);
        }
    }

    waitClientIds_.clear();

    socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
    socket_->close(e);
}

template<typename T>
Balancer<T>::Node::Node(boost::asio::io_service& ios, T& f, NodeSet& ns, const size_t id,
                        const std::string& n, unsigned cpn, const std::string& h, const std::string& s)
    : ns_(ns),
      id_(id),
      name_(n),
      connections_(),
      stash_()
{
    monitorNotify("creation");

    while(cpn--) {
        stash_.emplace(std::make_shared<Connection>(ios, f, *this, h, s));
    }
}

template<typename T>
Balancer<T>::Node::~Node()
{
    monitorNotify("destruction");
}

template<typename T>
size_t Balancer<T>::Node::id() const
{
    return id_;
}

template<typename T>
const std::string& Balancer<T>::Node::name() const
{
    return name_;
}

template<typename T>
const std::string& Balancer<T>::Node::group() const
{
    return ns_.name();
}

template<typename T>
void Balancer<T>::Node::nodeStats(const uint32_t qs, const uint32_t wt)
{
    ns_.nodeStats(id_, qs, wt);
}

template<typename T>
void Balancer<T>::Node::accountRequest(const uint16_t clientId)
{
    ns_.accountRequest(clientId);
}

template<typename T>
void Balancer<T>::Node::accountReply(const uint16_t clientId, const uint32_t wt)
{
    ns_.accountReply(clientId, wt);
}

template<typename T>
void Balancer<T>::Node::stash(const std::shared_ptr<Connection>& c)
{
    auto i = std::find(std::begin(connections_), std::end(connections_), c);
    if (std::end(connections_) != i) {
        connections_.erase(i);
    }

    stash_.emplace(c);
    if (connections_.empty()) {
        ns_.stash(id_);
    }
}

template<typename T>
void Balancer<T>::Node::move(const std::shared_ptr<Connection>& c)
{
    stash_.erase(c);
    connections_.emplace_front(c);

    if (1 == connections_.size()) {
        ns_.start(id_);
    }
}

template<typename T>
std::shared_ptr<typename Balancer<T>::Connection> Balancer<T>::Node::get() {
    if (connections_.empty()) {
        LogTrace(TRACE0) << "Can't find available connection";
        return std::shared_ptr<Connection>();
    }

    auto c = connections_.front();
    connections_.emplace_back(c);
    connections_.pop_front();

    return c;
}

template<typename T>
void Balancer<T>::Node::finalized() {
    monitorNotify("finalization");

    for(auto& c : connections_)
        c->finalized();
    for(auto& c : stash_)
        c->finalized();
}

template<typename T>
void Balancer<T>::Node::shutdown() {
    monitorNotify("shutdown");

    for(auto& c : connections_)
        c->shutdown(true);
    for(auto& c : stash_)
        c->shutdown(true);
}

template<typename T>
void Balancer<T>::Node::monitorNotify(const char* const action)
{
    const auto msg = Dates::to_iso_string(Dates::currentDateTime())
                     + (LogTrace(TRACE0) << " " << Pid_p << " Node " << name_
                                         << '(' << this << ", " << id_ << ") " << action << '.'
                       ).stream().str();

    static const bool fullNotify = readIntFromTcl("FULL_MONITOR_NOTIFICATION", 0);

    if (fullNotify) {
        monitorControl::is_errors_control(msg.c_str(), msg.size());
    }
}

template<typename T>
Balancer<T>::Stats::~Stats()
{ }

template<typename T>
int64_t Balancer<T>::LocalStats::NodeStats::rank() const
{
    return Balancer<T>::k1() * this->queueSize + Balancer<T>::k2() * this->waitTime;
}

template<typename T>
Balancer<T>::LocalStats::LocalStats(const size_t count)
    : stats_{ }
{
    stats_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        stats_.emplace_back(NodeStats { i, 0, 0 });
    }
}

template<typename T>
Balancer<T>::LocalStats::~LocalStats()
{ }

template<typename T>
void Balancer<T>::LocalStats::nodeStats(const size_t id, const uint32_t qs, const uint32_t wt)
{
    NodeStats& s = stats_.at(id);

    s.waitTime = wt;
    s.queueSize = qs;;
}

template<typename T>
void Balancer<T>::LocalStats::accountRequest(const uint16_t clientId)
{ }

template<typename T>
void Balancer<T>::LocalStats::accountReply(const uint16_t clientId, const uint32_t wt)
{ }

template<typename T>
void Balancer<T>::LocalStats::inc(const size_t id)
{
    ++(stats_.at(id).queueSize);
}

template<typename T>
bool Balancer<T>::LocalStats::less(const size_t lhs, const size_t rhs) const
{
    const NodeStats& l = stats_.at(lhs);
    const NodeStats& r = stats_.at(rhs);

    const auto lstat = l.rank();
    const auto rstat = r.rank();

    return (lstat == rstat) ? lhs < rhs : lstat < rstat;
}

template<typename T>
size_t Balancer<T>::LocalStats::id(const size_t id) const
{
    return stats_.at(id).id;
}

template<typename T>
uint32_t Balancer<T>::LocalStats::queueSize(const size_t id) const
{
    return stats_.at(id).queueSize;
}

template<typename T>
size_t Balancer<T>::LocalStats::size() const
{
    return stats_.size();
}

template<typename T>
void Balancer<T>::LocalStats::finalized()
{ }

template<typename T>
Balancer<T>::SharedStats::Lock::Lock(const std::string& name)
    : fd_ { -1 }, state_ { State::UNLOCKED }, name_ { name }
{
    fd_ = open(name.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    assert(-1 != fd_);

    LogTrace(TRACE1) << "File " << name << " has been opened: " << fd_;
}

template<typename T>
Balancer<T>::SharedStats::Lock::~Lock()
{
    unlock();
    close(fd_);
}

template<typename T>
bool Balancer<T>::SharedStats::Lock::try_to_unique_lock()
{
    if (State::EXCLUSIVE == state_) {
        LogError(STDLOG) << "Double attempt to obtain exclusive lock on " << fd_ << " (" << name_ << ") will be skipped.";
        return true;
    }

    struct flock fl { };

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    const bool res  = (-1 != fcntl(fd_, F_SETLK, &fl));

    LogTrace(TRACE1) << "File '" << name_ << "' (fd " << fd_ << ")"
                     << (res ? " has been locked" : " has not been locked") << " in exclusive mode.";

    state_ = res ? State::EXCLUSIVE : state_;

    return State::EXCLUSIVE == state_;
}

template<typename T>
void Balancer<T>::SharedStats::Lock::unique_lock()
{
    if (State::EXCLUSIVE == state_) {
        LogError(STDLOG) << "Double attempt to obtain exclusive lock on " << fd_ << " (" << name_ << ") will be skipped.";
        return;
    }

    struct flock fl { };

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int res = -1;
    while (-1 == (res = fcntl(fd_, F_SETLKW, &fl)) && (EINTR == errno))
        ;

    assert(-1 != res);

    state_ = State::EXCLUSIVE;

    LogTrace(TRACE1) << "File '" << name_ << "' (fd " << fd_ << ") has been locked in exclusive mode.";
}

template<typename T>
void Balancer<T>::SharedStats::Lock::shared_lock()
{
    if (State::SHARED == state_) {
        LogError(STDLOG) << "Double attempt to obtain shared lock on " << fd_ << " (" << name_ << ") will be skipped.";
        return;
    }

    struct flock fl { };

    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int res = -1;
    while (-1 == (res = fcntl(fd_, F_SETLKW, &fl)) && (EINTR == errno))
        ;

    assert(-1 != res);

    state_ = State::SHARED;

    LogTrace(TRACE1) << "File '" << name_ << "' (fd " << fd_ << ") has been locked in shared mode.";
}

template<typename T>
void Balancer<T>::SharedStats::Lock::unlock()
{
    if (State::UNLOCKED == state_) {
        LogError(STDLOG) << "Double attempt to release unlocked lock on " << fd_ << " (" << name_ << ") will be skipped.";
        return;
    }

    if (State::UNLOCKED != state_) {
        struct flock fl { };

        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;

        assert(-1 != fcntl(fd_, F_SETLK, &fl));

        state_ = State::UNLOCKED;

        LogTrace(TRACE1) << "File '" << name_ << "' (fd " << fd_ << ") has been unlocked.";
    }
}

template<typename T>
Balancer<T>::SharedStats::NodeStats::NodeStats(const size_t id)
    : id_{ id }, stats_{ uint64_t { 0 } }
{ }

template<typename T>
Balancer<T>::SharedStats::NodeStats::~NodeStats()
{ }

template<typename T>
size_t Balancer<T>::SharedStats::NodeStats::id() const
{
    return id_;
}

template<typename T>
void Balancer<T>::SharedStats::NodeStats::stats(const uint32_t qs, const uint32_t wt)
{
    stats_.store((uint64_t { wt } << (sizeof(uint32_t) * CHAR_BIT)) | uint64_t { qs });

    LogTrace(TRACE7) << __FUNCTION__ << " : set node " << id_ << " stats: [" << qs << ' ' << wt << ']';
}

template<typename T>
uint32_t Balancer<T>::SharedStats::NodeStats::queueSize() const
{
    return stats_.load() & 0xFFFFFFFF;
}

template<typename T>
bool Balancer<T>::SharedStats::NodeStats::operator<(const NodeStats& other) const
{
    return this->rank() < other.rank();
}

template<typename T>
typename Balancer<T>::SharedStats::NodeStats& Balancer<T>::SharedStats::NodeStats::operator++()
{
    ++stats_;
    return *this;
}

template<typename T>
int64_t Balancer<T>::SharedStats::NodeStats::rank() const
{
    const uint64_t s = this->stats_.load();
    return Balancer<T>::k1() * (s & 0xFFFFFFFF) + Balancer<T>::k2() * (s >> (sizeof(uint32_t) * CHAR_BIT));

    // The same below
    /* return (Balancer<T>::k1() * this->queueSize_ + Balancer<T>::k2() * this->waitTime_); */
}

template<typename T>
Balancer<T>::SharedStats::SharedStats(boost::asio::io_service& ios, const std::string& shm,
                                      const std::string& lock, const std::string& barrier, const size_t count)
    : Stats { }, ios_ { ios }, stats_ { nullptr }, clientIdsStats_ { nullptr }, count_ { count },
      shmLock_ { std::make_unique< Lock >(lock) }, barrier_ { std::make_shared< Lock >(barrier) }
{
    int fd = -1;

    const size_t nodesStatsLength = sizeof(NodeStats[ count_ ]);
    const size_t clientIdsStatsSize = enableClientStats() ? sizeof(ClientIdStats[ ClientIdStats::CLIENT_IDS_COUNT ]) : 0;
    const size_t length = clientIdsStatsSize + nodesStatsLength;

    if (shmLock_->try_to_unique_lock()) {
        LogTrace(TRACE1) << __FUNCTION__ << " : " << this << " shared memory creating.";

        shm_unlink(shm.c_str());

        fd = shm_open(shm.c_str(), O_RDWR | O_CREAT | O_EXCL, 00600);

        assert(0 < fd);
        assert(0 == ftruncate(fd, length));

        uint8_t* const m = reinterpret_cast< uint8_t* >(mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

        assert(MAP_FAILED != m);

        if (enableClientStats()) {
            clientIdsStats_ = reinterpret_cast< ClientIdStats* >(m);
            for (size_t i = 0; i < ClientIdStats::CLIENT_IDS_COUNT; ++i) {
                new (clientIdsStats_ + i) ClientIdStats();
            }
        }

        stats_ = reinterpret_cast< NodeStats* >(m + clientIdsStatsSize);

        for (size_t i = 0; i < count_; ++i) {
            new (stats_ + i) NodeStats(i);
        }

        shmLock_->shared_lock();
    } else {
        shmLock_->shared_lock();

        LogTrace(TRACE1) << __FUNCTION__ << " : " << this << " shared memory opening.";

        fd = shm_open(shm.c_str(), O_RDWR, 00600);

        assert(0 < fd);

        uint8_t* const m = reinterpret_cast< uint8_t* >(mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

        assert(MAP_FAILED != m);

        clientIdsStats_ = enableClientStats() ? reinterpret_cast< ClientIdStats* >(m) : nullptr;
        stats_ = reinterpret_cast< NodeStats* >(m + clientIdsStatsSize);
    }

    static const auto timeout = boost::posix_time::seconds { readIntFromTcl("BARRIER_LOCK_TIMEOUT", 8) };

    auto b = barrier_;
    auto timer = std::make_shared< boost::asio::deadline_timer >(ios_);
    timer->expires_from_now(timeout);
    timer->async_wait([ timer, b ](const boost::system::error_code& ec) {
        b->shared_lock();
    });

    LogTrace(TRACE1) << "shmem fd: " << fd;

    close(fd);
}

template<typename T>
Balancer<T>::SharedStats::~SharedStats()
{
    LogTrace(TRACE1) << __FUNCTION__;

    if (enableClientStats()) {
        assert(nullptr != clientIdsStats_);
        assert(0 == munmap(clientIdsStats_, sizeof(ClientIdStats[ ClientIdStats::CLIENT_IDS_COUNT ]) + sizeof(NodeStats) * count_));
    } else {
        assert(0 == munmap(stats_, sizeof(NodeStats) * count_));
    }

    clientIdsStats_ = nullptr;
    stats_ = nullptr;
}

template<typename T>
void Balancer<T>::SharedStats::nodeStats(const size_t id, const uint32_t qs, const uint32_t wt)
{
    assert(id < count_);
    stats_[ id ].stats(qs, wt);
}

template<typename T>
void Balancer<T>::SharedStats::accountRequest(const uint16_t clientId)
{
    if (nullptr != clientIdsStats_) {
        clientIdsStats_[ clientId ].incoming();
    }
}

template<typename T>
void Balancer<T>::SharedStats::accountReply(const uint16_t clientId, const uint32_t wt)
{
    if (nullptr != clientIdsStats_) {
        clientIdsStats_[ clientId ].outgoing(wt);
    }
}

template<typename T>
void Balancer<T>::SharedStats::inc(const size_t id)
{
    assert(id < count_);
    ++(stats_[ id ]);
}

template<typename T>
bool Balancer<T>::SharedStats::less(const size_t lhs, const size_t rhs) const
{
    return stats_[ lhs ] < stats_[ rhs ];
}

template<typename T>
size_t Balancer<T>::SharedStats::id(const size_t id) const
{
    assert(id < count_);
    return stats_[ id ].id();
}

template<typename T>
uint32_t Balancer<T>::SharedStats::queueSize(const size_t id) const
{
    assert(id < count_);
    return stats_[ id ].queueSize();
}

template<typename T>
size_t Balancer<T>::SharedStats::size() const
{
    return count_;
}

template<typename T>
void Balancer<T>::SharedStats::finalized()
{
    shmLock_.reset(); // Release shared lock & close descriptor of lock file ('Advisory record locking' - man fcntl).

    barrier_->unlock();
    barrier_->unique_lock(); // Wait for unique lock (Wait for other processes)

    barrier_.reset(); // Release lock & close descriptor of lock file ('Advisory record locking' - man fcntl).
}

template<typename T>
Balancer<T>::NodeSet::Index::~Index()
{ }

template<typename T>
Balancer<T>::NodeSet::SingleIndex::SingleIndex(Stats& s)
    : index_{ IndexCmp { s } }
{ }

template<typename T>
Balancer<T>::NodeSet::SingleIndex::~SingleIndex()
{ }

template<typename T>
void Balancer<T>::NodeSet::SingleIndex::emplace(const size_t id)
{
    LogTrace(TRACE7) << "Emplace node " << id;

    assert(std::cend(index_) == std::find(std::cbegin(index_), std::cend(index_), id));

    index_.emplace(id);
}

template<typename T>
bool Balancer<T>::NodeSet::SingleIndex::remove(const size_t id)
{
    LogTrace(TRACE7) << "Remove node " << id;

    return 0 < index_.erase(id);
}

template<typename T>
void Balancer<T>::NodeSet::SingleIndex::stash(const size_t id)
{
    remove(id);
}

template<typename T>
size_t Balancer<T>::NodeSet::SingleIndex::min() const
{
    assert(!index_.empty());

    return *std::begin(index_);
}

template<typename T>
size_t Balancer<T>::NodeSet::SingleIndex::max() const
{
    assert(!index_.empty());

    return *std::rbegin(index_);
}

template<typename T>
bool Balancer<T>::NodeSet::SingleIndex::empty() const
{
    return index_.empty();
}

template<typename T>
Balancer<T>::NodeSet::SharedIndex::SharedIndex(Stats& s)
    : stats_{ s }, index_{ }
{ }

template<typename T>
Balancer<T>::NodeSet::SharedIndex::~SharedIndex()
{ }

template<typename T>
void Balancer<T>::NodeSet::SharedIndex::emplace(const size_t id)
{
    assert(std::cend(index_) == std::find(std::cbegin(index_), std::cend(index_), id));

    index_.emplace_back(id);
}

template<typename T>
bool Balancer<T>::NodeSet::SharedIndex::remove(const size_t id)
{
    return false;
}

template<typename T>
void Balancer<T>::NodeSet::SharedIndex::stash(const size_t id)
{
    index_.erase(std::remove(std::begin(index_), std::end(index_), id), std::end(index_));
}

template<typename T>
size_t Balancer<T>::NodeSet::SharedIndex::min() const
{
    assert(!index_.empty());

    return *std::min_element(std::cbegin(index_), std::cend(index_), IndexCmp { stats_ });
}

template<typename T>
size_t Balancer<T>::NodeSet::SharedIndex::max() const
{
    assert(!index_.empty());

    return *std::max_element(std::cbegin(index_), std::cend(index_), IndexCmp { stats_ });
}

template<typename T>
bool Balancer<T>::NodeSet::SharedIndex::empty() const
{
    return index_.empty();
}

template<typename T>
Balancer<T>::NodeSet::NodeSet(boost::asio::io_service& ios, const std::string& name, const std::map< std::string, size_t >& nodeNames,
                              std::vector< uint16_t >&& clientIds, const uint32_t queueSizeLimit, T& f, Stats& s)
    : stats_(s), index_{ nullptr }, nodes_{ }, name_{ name }, clientIds_ { std::move(clientIds) }, queueSizeLimit_{ queueSizeLimit }
{
    monitorNotify("creation");

    if (stats_.shared()) {
        index_ = std::make_unique<SharedIndex>(stats_);
    } else {
        index_ = std::make_unique<SingleIndex>(stats_);
    }

    for (const auto& n : nodeNames) {
        nodes_.emplace(n.second, std::make_shared<Node>(ios, f, *this, n.second, n.first,
                                                        readIntFromTcl(n.first + "(CONNECTIONS_PER_NODE)", 16),
                                                        readStringFromTcl(n.first + "(HOST)"),
                                                        readStringFromTcl(n.first + "(PORT)")));
    }
}

template<typename T>
Balancer<T>::NodeSet::~NodeSet()
{
    monitorNotify("destruction");
}

template<typename T>
const std::string& Balancer<T>::NodeSet::name() const
{
    return name_;
}

template<typename T>
const std::vector< uint16_t >& Balancer<T>::NodeSet::clientIds() const
{
    return clientIds_;
}

template<typename T>
void Balancer<T>::NodeSet::start(const size_t id) {
    LogTrace(TRACE7) << __FUNCTION__ << " : node " << id << " added to index of nodes set " << name_;

    assert(id == stats_.id(id));

    stats_.nodeStats(id, 0, 0);
    index_->emplace(id);
}

template<typename T>
std::shared_ptr<typename Balancer<T>::Connection> Balancer<T>::NodeSet::get() {
    if (index_->empty()) {
        LogTrace(TRACE0) << "Can't find available node from node set " << name_ << " (" << this << ')';
        return std::shared_ptr<Connection>();
    }

    const size_t i = index_->min();
    auto& n = *(nodes_[ i ]);

    assert((i == n.id()) && (i == stats_.id(i)));

    if (queueSizeLimit_ < stats_.queueSize(i)) {
        return std::shared_ptr<Connection>();
    }

    auto c = n.get();
    if (c) {
        const auto r = index_->remove(i);

        stats_.inc(i);

        if (r) {
            index_->emplace(i);
        }
    }

    return c;
}

template<typename T>
void Balancer<T>::NodeSet::finalized()
{
    monitorNotify("finalization");

    for (const auto& n : nodes_) {
        n.second->finalized();
    }
}

template<typename T>
void Balancer<T>::NodeSet::shutdown()
{
    LogTrace(TRACE0) <<  " Nodes set " << name_ << '(' << this << ") shutdown.";

    for (const auto& n : nodes_) {
        n.second->shutdown();
    }
}

template<typename T>
void Balancer<T>::NodeSet::stash(const size_t id) {
    LogTrace(TRACE7) << __FUNCTION__;

    index_->stash(id);
}

template<typename T>
void Balancer<T>::NodeSet::nodeStats(const size_t id, const uint32_t qs, const uint32_t wt)
{
    LogTrace(TRACE7) << __FUNCTION__ << " : " << this << " " << id;

    assert(id == stats_.id(id));

    auto r = index_->remove(id);

    stats_.nodeStats(id, qs, wt);

    if (r) {
        index_->emplace(id);
    }
}

template<typename T>
void Balancer<T>::NodeSet::accountRequest(const uint16_t clientId)
{
    stats_.accountRequest(clientId);
}

template<typename T>
void Balancer<T>::NodeSet::accountReply(const uint16_t clientId, const uint32_t wt)
{
    stats_.accountReply(clientId, wt);
}

template<typename U>
void handleShakeUpTimer(std::shared_ptr<boost::asio::deadline_timer>& timer,
                        std::weak_ptr<typename Balancer<U>::NodeSet>& nodeSet,
                        const std::string& name, const boost::system::error_code& e)
{
    LogTrace(TRACE7) << "Shake up timer handler: " << timer;

    auto ns = nodeSet.lock();
    if (!ns) {
        LogTrace(TRACE0) << "Nodes set " << name << " has expired.";
        return;
    }

    if (!e) {
       ns->shakeUp();
    }

    timer->expires_from_now(boost::posix_time::seconds(Balancer<U>::NodeSet::CLEAN_TIMEOUT));
    timer->async_wait(boost::bind(&handleShakeUpTimer<U>, timer, nodeSet, ns->name(), boost::asio::placeholders::error));
}

template<typename T>
void Balancer<T>::NodeSet::shakeUp() {
    LogTrace(TRACE7) << __FUNCTION__ << ": " << name_ << " (" << this << ")";

    if (!index_->empty()) {
        const size_t id = index_->max();

        this->nodeStats(id, 0, 0);
    }
}

template<typename T>
void Balancer<T>::NodeSet::monitorNotify(const char* const action)
{
    const auto msg = Dates::to_iso_string(Dates::currentDateTime())
                     + " " + std::to_string(Pid_p)
                     + ((LogTrace(TRACE0) <<  " Nodes set " << name_
                                         << '(' << this << ") " << action << '.'
                       ).stream().str());

    monitorControl::is_errors_control(msg.c_str(), msg.size());
}

template<typename T>
void Balancer<T>::NodesGroups::emplace(boost::asio::io_service& ios, const std::string& name,
                                       const std::map< std::string, size_t >& nodes, std::vector<uint16_t>&& clientIds,
                                       const uint32_t queueSizeLimit, T& f, Stats& stats)
{
    groups_.emplace_back(std::make_shared<Balancer<T>::NodeSet>(ios, name, nodes, std::move(clientIds), queueSizeLimit, f, stats));
    for (const auto clientId : groups_.back()->clientIds()) {
        clientIdIndex_[ clientId ] = groups_.size() - 1;
    }

    auto timer = std::make_shared<boost::asio::deadline_timer>(ios);
    timer->expires_from_now(boost::posix_time::seconds(Balancer<T>::NodeSet::CLEAN_TIMEOUT));
    timer->async_wait(boost::bind(&handleShakeUpTimer<T>, timer, std::weak_ptr<NodeSet>(groups_.back()), name, boost::asio::placeholders::error));
}

struct NodeSetParam
{
    const std::string name;
    std::map< std::string, size_t > nodes;
    std::vector<uint16_t> clientIds;
    const uint32_t queueSizeLimit;

    template<typename T>
    NodeSetParam(const std::string& nm, T&& nd, std::vector<uint16_t>&& ids, const uint32_t limit)
        : name { nm }, nodes { std::forward<T>(nd) }, clientIds { std::move(ids) }, queueSizeLimit { limit }
    { }
};

template<typename T>
Balancer<T>::NodesGroups::NodesGroups(boost::asio::io_service& ios, T& f)
    : groups_ { }, stats_ { }, clientIdIndex_ { }
{
    LogTrace(TRACE0) << "Create nodes groups: " << this;

    static const auto CONFIG_DEFAULT_NODES_GROUP = current_group2() + "(DEFAULT_NODES_GROUP)";
    static const auto CONFIG_DEDICATED_NODES_GROUPS = current_group2() + "(DEDICATED_NODES_GROUPS)";

    const auto defaultNodesGroup = tcl_utils::readListOfStringsFromTcl(CONFIG_DEFAULT_NODES_GROUP);
    assert(!defaultNodesGroup.empty() && "Default nodes group must be described.");

    LogTrace(TRACE1) << __FUNCTION__ << ": default nodes group: " << LogCont(" ", defaultNodesGroup);

    const auto dedicatedNodesGroups = tcl_utils::readListOfStringsFromTcl(CONFIG_DEDICATED_NODES_GROUPS);

    LogTrace(TRACE1) << __FUNCTION__ << ": dedicated nodes groups: " << LogCont(" ", dedicatedNodesGroups);

    const auto listenersCount = readIntFromTcl(current_group2() + "(NUMBER_OF_LISTENERS)", 1);

    size_t id { 0 };
    std::map< std::string, size_t > global { };
    std::vector<NodeSetParam> params { };

    for (const auto& n : defaultNodesGroup) {
        global.emplace(n, id++);
    }

    // Default nodes group
    params.emplace_back("DEFAULT_NODES_GROUP", global, std::vector<uint16_t> { }, std::numeric_limits<uint32_t>::max());

    clientIdIndex_.fill(0);
    for (const auto& g : dedicatedNodesGroups) {
        const auto nodes = tcl_utils::readListOfStringsFromTcl(g + "(NODES)");
        if (nodes.empty()) {
            LogError(STDLOG) << "Skip group " << g << " without nodes.";
            continue;
        }

        std::map< std::string, size_t > numberedNodes { };
        for (const auto& n : nodes) {
            auto it = global.find(n);
            if (std::cend(global) == it) {
                it = global.emplace(n, id++).first;
            }

            numberedNodes.emplace(*it);
        }

        auto clientIds = tcl_utils::readListOfIntsFromTcl<uint16_t>(g + "(CLIENT_IDS)");
        if (clientIds.empty()) {
            LogError(STDLOG) << "Skip group " << g << " without client ids.";
            continue;
        }

        const uint32_t queueSizeLimit = readIntFromTcl(g + "(QUEUE_SIZE_LIMIT)", std::numeric_limits<uint32_t>::max());

        params.emplace_back(g, std::move(numberedNodes), std::move(clientIds), queueSizeLimit);
    }

    if (1 < listenersCount) {
        auto shm = sharedMemoryName();
        const auto ln = readStringFromTcl(current_group2() + "(LOCK_NAME)", "balancer");

        stats_ = std::make_unique< SharedStats >(ios, shm, ln + ".lock", ln + ".guard", global.size()); // MUST BE before NodeSet construct.
    } else {
        stats_ = std::make_unique<LocalStats>(global.size()); // The same as above.
    }

    LogTrace(TRACE1) << "Count of unique nodes: " << global.size();

    for (auto&& p : params) {
        this->emplace(ios, p.name, p.nodes, std::move(p.clientIds), p.queueSizeLimit, f, *stats_);
    }
}

template<typename T>
Balancer<T>::NodesGroups::~NodesGroups()
{
    LogTrace(TRACE0) << "Nodes groups " << this << " desctruction.";
}

template<typename T>
std::shared_ptr<typename Balancer<T>::Connection> Balancer<T>::NodesGroups::get(const uint16_t clientId)
{
    if (const auto c = groups_[ clientIdIndex_[ clientId ] ]->get()) {
        return c;
    }

    assert(!groups_.empty());

    return groups_.front()->get();
}

template<typename T>
void Balancer<T>::NodesGroups::finalized()
{
    for (const auto& g : groups_) {
        g->finalized();
    }

    stats_->finalized();
}

template<typename T>
void Balancer<T>::NodesGroups::shutdown()
{
    for (const auto& g : groups_) {
        g->shutdown();
    }
}

template<typename T>
Balancer<T>::Balancer(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const unsigned short port,
        const size_t maxOpenConnections,
        const unsigned k1,
        const unsigned k2,
        std::set<uint16_t>&& traceableClientIds,
        const bool multiListener)
    : service_(io_s),
      frontend_(io_s, headType, addr, port, *this, maxOpenConnections, std::move(traceableClientIds), multiListener),
      nodesGroups_()
{
    k1_ = k1;
    k2_ = k2;
}

template<typename T>
void Balancer<T>::ratio(const unsigned k1, const unsigned k2)
{
    k1_ = k1;
    k2_ = k2;
}

template<typename T>
void Balancer<T>::reconfigure()
{
    LogTrace(TRACE0) << __FUNCTION__ << ": balancer " << this << " will be reconfigured.";
    if (!Tcl_CreateObjCommand(getTclInterpretator(), "read_config",
                               balancer::tcl_read_config_nop, static_cast<ClientData>(this), 0)) {
        LogError(STDLOG) << "create read_config command failed: "
                         << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));
        assert(false && "Create Tcl command failed.");
    }

    auto balancer = this;
    auto timer = std::make_shared< boost::asio::deadline_timer >(service_);
    timer->expires_from_now(boost::posix_time::seconds(10)); // TODO BARRIER_LOCK_TIMEOUT
    timer->async_wait([ timer, balancer ](const boost::system::error_code& ec) {
        if (ec) {
            LogTrace(TRACE0) << __FUNCTION__ << " error: " << ec;
        }

        if (!Tcl_CreateObjCommand(getTclInterpretator(), "read_config",
                                   balancer::tcl_read_config, static_cast<ClientData>(balancer), 0)) {
            LogError(STDLOG) << "create read_config command failed: "
                             << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));
            assert(false && "Create Tcl command failed.");
        }
    });

    if (nodesGroups_) {
        LogTrace(TRACE0) << __FUNCTION__ << ": nodes groups " << nodesGroups_.get() << " will be finalized.";

        nodesGroups_->finalized();

        std::shared_ptr<NodesGroups> ng(std::move(nodesGroups_));
        auto timer = std::make_shared<boost::asio::deadline_timer>(service_);
        timer->expires_from_now(boost::posix_time::minutes(16));
        timer->async_wait([timer, ng](const boost::system::error_code& e) {
                             if (e) {
                                 LogError(STDLOG) << "Timeout error: " << e << ": " << e.message();
                             }

                             ng->shutdown();
                         });
    }

    nodesGroups_ = std::make_unique<NodesGroups>(service_, frontend_);
}

template<typename T>
bool Balancer<T>::enableTrace(const uint16_t clientId)
{
    return frontend_.enableTrace(clientId);
}

template<typename T>
bool Balancer<T>::disableTrace(const uint16_t clientId)
{
    return frontend_.disableTrace(clientId);
}

template<typename T>
void Balancer<T>::takeRequest(
        const uint64_t balancerId,
        const typename T::KeyType& clientConnectionId,
        const std::string& rep,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull)
{
    const uint16_t clientId { frontend_.clientIdFromHeader(head) };
    const uint32_t messageId { frontend_.messageIdFromHeader(head) };

    auto c = nodesGroups_->get(clientId);
    if (!c) {
        LogTrace(TRACE0) << "Can't find available node";
        frontend_.queueOverflow(balancerId, clientConnectionId, head, data, 0, 0);
        return;
    }

    c->write(clientConnectionId, clientId, messageId, rep, head, data);
}

ClientIdStats::ClientIdStats()
    : incomingCount_ { uint32_t { 0 } }, outgoingStats_ { uint64_t { 0 } }
{ }

ClientIdStats::~ClientIdStats()
{ }

ClientIdStats::Stats ClientIdStats::get()
{
    const std::pair< uint32_t, uint32_t > outgoingStats { get(outgoingStats_.load()) };
    return { incomingCount_.load(), outgoingStats.second, outgoingStats.first };
}

void ClientIdStats::incoming()
{
    ++incomingCount_;
}

void ClientIdStats::outgoing(const uint32_t wt)
{
    outgoingStats_ += (uint64_t { wt } << (sizeof(uint32_t) * CHAR_BIT)) | uint64_t { 1 };
}

ClientIdStats::Stats ClientIdStats::clear()
{
    const std::pair< uint32_t, uint32_t > outgoingStats { get(outgoingStats_.exchange(uint64_t { 0 })) };
    return { incomingCount_.exchange(uint32_t { 0 }), outgoingStats.second, outgoingStats.first };
}

std::pair< uint32_t, uint32_t > ClientIdStats::get(const uint64_t s)
{
    return { s >> (sizeof(uint32_t) * CHAR_BIT), s & 0xFFFFFFFF };
}

ClientStatsFrontend::ClientStatsFrontend(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const uint16_t port,
        balancer::ClientStatsAssistant< ClientStatsFrontend >& d)
    : Dispatcher::bf::BasePlainHttpFrontend< balancer::ClientStatsAssistant< ClientStatsFrontend > >(io_s, headType, addr, port, d)
{ }

static int tcl_client_stats_assistant_read_config(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    ClientStatsAssistant< ClientStatsFrontend >* const assistant = reinterpret_cast<ClientStatsAssistant< ClientStatsFrontend >* >(cl);
    if (!assistant) {
        LogError(STDLOG) << "Invalid client data";

        return TCL_ERROR;
    }

    assistant->reconfigure();

    return TCL_OK;
}

template< typename T >
ClientStatsAssistant< T >::ClientStatsAssistant(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const unsigned short port)
    : service_ { io_s }, timer_ { io_s }, frontend_ { io_s, headType, addr, port, *this },
      stats_ { nullptr }, dump_ { }, begin_ { boost::posix_time::microsec_clock::universal_time() }
{
    init();
}

static int nop(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    LogError(STDLOG) << "'read_config' unavailable. Try later.";
    return TCL_OK;
}

template< typename T >
ClientStatsAssistant< T >::~ClientStatsAssistant()
{
    LogTrace(TRACE1) << __FUNCTION__;

    unmap();
}

template< typename T >
void ClientStatsAssistant< T >::takeRequest(
        const uint64_t balancerId,
        const typename T::KeyType& clientConnectionId,
        const std::string& rep,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull)
{
    if (nullptr == stats_) {
        frontend_.queueOverflow(balancerId, clientConnectionId, head, data, 0, 0);

        return;
    }

    ServerFramework::HTTP::request_parser parser;
    ServerFramework::HTTP::request req;
    boost::tribool result;
    ServerFramework::HTTP::reply reply;
    std::vector< uint8_t > answer { };

    boost::tie(result, boost::tuples::ignore) = parser.parse(req, std::cbegin(data), std::cend(data));
    if (result) {
        LogTrace(TRACE1) << "req.method: " << req.method;
        LogTrace(TRACE1) << "req.uri: " << req.uri;

        reply = ServerFramework::HTTP::reply::stock_reply(ServerFramework::HTTP::reply::ok,
                                                          dump_.empty()
                                                              ? dump(false, boost::posix_time::microsec_clock::universal_time())
                                                              : dump_,
                                                          "application/json");
    } else {
        reply = ServerFramework::HTTP::reply::stock_reply(ServerFramework::HTTP::reply::bad_request);
    }

    reply.to_buffer(answer);

    frontend_.sendAnswer(balancerId, clientConnectionId, head, answer, 0, 0);

    write_set_flag_type(0, 1, QUEPOT_LEVB, nullptr);

    return;
}

template< typename T >
void ClientStatsAssistant< T >::reconfigure()
{
    LogTrace(TRACE1) << __FUNCTION__;

    unmap();
    init();
}

template< typename T >
std::string ClientStatsAssistant< T >::dump(const bool clear, const boost::posix_time::ptime& end)
{
    auto fn = std::mem_fn(clear
                                   ? &ClientIdStats::clear
                                   : static_cast< ClientIdStats::Stats(ClientIdStats::*)() >(&ClientIdStats::get)
    );

    ClientStats clientStats { begin_, end, { } };
    for (size_t i = 0; i < ClientIdStats::CLIENT_IDS_COUNT; ++i) {
        auto s = fn(stats_[ i ]);
        if (0 < s.incoming) {
            boost::optional< OutgoingStats > outgoingStats;
            if (0 < s.outgoing) {
                outgoingStats = OutgoingStats { s.outgoing, static_cast< double >(s.waitTime) / s.outgoing };
            }

            clientStats.stats.emplace(i, Stats{ s.incoming, outgoingStats });
        }
    }

    return json_spirit::write(json_spirit::Traits< ClientStatsAssistant::ClientStats >::packExt(clientStats),
                              json_spirit::pretty_print | json_spirit::raw_utf8 |
                              json_spirit::remove_trailing_zeros |
                              json_spirit::single_line_arrays);
}

template< typename T >
void ClientStatsAssistant< T >::init()
{
    LogTrace(TRACE1) << __FUNCTION__;

    if (!Tcl_CreateObjCommand(getTclInterpretator(), "read_config",
                               balancer::nop, nullptr, 0)) {
        LogError(STDLOG) << "create read_config command failed: "
                         << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));

        assert(false && "Create Tcl command failed.");
    }

    timer_.expires_from_now(boost::posix_time::seconds(4));
    timer_.async_wait([ this ](const boost::system::error_code& ec) {
        LogTrace(TRACE1) << "ClientStatsAssistant< T >::init: " << this << " shared memory opening.";

        const int fd = shm_open(sharedMemoryName().c_str(), O_RDWR, 00600);

        assert(0 < fd);

        const size_t clientIdsStatsSize = sizeof(ClientIdStats[ ClientIdStats::CLIENT_IDS_COUNT ]);
        stats_ = reinterpret_cast< ClientIdStats* >(mmap(nullptr, clientIdsStatsSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

        LogTrace(TRACE1) << "shmem address: " << stats_;

        assert(MAP_FAILED != stats_);

        LogTrace(TRACE1) << "shmem fd: " << fd;

        close(fd);

        resetTimer();
        timer_.async_wait(std::bind(&ClientStatsAssistant< T >::handleTimer, this, std::placeholders::_1));

        if (!Tcl_CreateObjCommand(getTclInterpretator(), "read_config",
                                   balancer::tcl_client_stats_assistant_read_config, static_cast< ClientData >(this), 0)) {
            LogError(STDLOG) << "create read_config command failed: "
                             << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));

            assert(false && "Create Tcl command failed.");
        }

        dump_.clear();
        begin_ = boost::posix_time::microsec_clock::universal_time();
    });
}

template< typename T >
void ClientStatsAssistant< T >::unmap()
{
    LogTrace(TRACE1) << __FUNCTION__;

    if (nullptr != stats_) {
        assert(0 == munmap(stats_, sizeof(ClientIdStats[ ClientIdStats::CLIENT_IDS_COUNT ])));

        stats_ = nullptr;
    }
}

template< typename T >
void ClientStatsAssistant< T >::resetTimer()
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto nextMinute = boost::posix_time::hours(now.time_of_day().hours()) + boost::posix_time::minutes(now.time_of_day().minutes() + 1);
    timer_.expires_at(boost::posix_time::ptime(now.date(), nextMinute));
}

template< typename T >
void ClientStatsAssistant< T >::handleTimer(const boost::system::error_code& ec)
{
    if (ec) {
        LogTrace(TRACE1) << __FUNCTION__ << " error: " << ec;
        if (boost::asio::error::operation_aborted == ec) {
            return;
        }
    }

    auto t = timer_.expires_at();
    dump_ = dump(true, t);
    begin_ = t;

    resetTimer();
    timer_.async_wait(std::bind(&ClientStatsAssistant< T >::handleTimer, this, std::placeholders::_1));
}

static int tcl_set_ratio(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    if (objc < 3) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Invalid arguments.");

        return TCL_ERROR;
    }

    int k1 = readIntFromTcl(objv[1], Balancer<balancer::BalancerFrontend>::k1());
    int k2 = readIntFromTcl(objv[2], Balancer<balancer::BalancerFrontend>::k2());

    Balancer<balancer::BalancerFrontend>::ratio(k1, k2);

    LogError(STDLOG) << "The balancer ratio new value: (" << k1 << ',' << k2 << ')';

    return TCL_OK;
}

static int tcl_read_config_nop(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    LogError(STDLOG) << "'read_config' unavailable now. Try later.";
    return TCL_OK;
}

static int tcl_read_config(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    if (TCL_OK != Tcl_EvalFile(interp, readStringFromTcl("NODES_CFG_FILE").c_str())) {
        LogError(STDLOG) << "Read config failed: " << Tcl_GetStringResult(interp);

        return TCL_ERROR;
    }

    Balancer<BalancerFrontend>* const balancer = reinterpret_cast<Balancer<BalancerFrontend>* >(cl);
    if (!balancer) {
        LogError(STDLOG) << "Invalid client data";

        return TCL_ERROR;
    }

    balancer->reconfigure();

    return TCL_OK;
}

template<typename F>
static int switchClientIdTraceability(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[], F f, const char* const successMsg)
{
    if (objc < 2) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Invalid arguments.");

        return TCL_ERROR;
    }

    const int clientId = readIntFromTcl(objv[1], 0);
    if (0 == clientId) {
        LogError(STDLOG) << "Invalid client id: " << readStringFromTcl(objv[1], "");

        return TCL_ERROR;
    }

    Balancer<BalancerFrontend>* const balancer = reinterpret_cast<Balancer<BalancerFrontend>* >(cl);
    if (!balancer) {
        LogError(STDLOG) << "Invalid client data";

        return TCL_ERROR;
    }

    if (f(balancer, clientId)) {
        const auto msg = Dates::to_iso_string(Dates::currentDateTime())
                         + ((LogTrace(TRACE0) << " : Client id " << clientId << successMsg).stream().str());

        monitorControl::is_errors_control(msg.c_str(), msg.size());
    }

    return TCL_OK;
}

static int tcl_enable_trace(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    return switchClientIdTraceability(cl, interp, objc, objv,
               std::mem_fn(&Balancer<balancer::BalancerFrontend>::enableTrace), " became traceable.");
}

static int tcl_disable_trace(ClientData cl, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    return switchClientIdTraceability(cl, interp, objc, objv,
               std::mem_fn(&Balancer<balancer::BalancerFrontend>::disableTrace), " is no longer traceable.");
}

static int createCommand(Balancer<balancer::BalancerFrontend>* const balancer)
{
    if (!Tcl_CreateObjCommand(getTclInterpretator(), "set_ratio",
                               balancer::tcl_set_ratio, static_cast<ClientData>(0), 0)) {
        LogError(STDLOG) << "create set_ratio command failed: "
                         << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));
        return TCL_ERROR;
    }

    if (!Tcl_CreateObjCommand(getTclInterpretator(), "enable_trace",
                               balancer::tcl_enable_trace, static_cast<ClientData>(balancer), 0)) {
        LogError(STDLOG) << "create enable_trace command failed: "
                         << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));
        return TCL_ERROR;
    }

    if (!Tcl_CreateObjCommand(getTclInterpretator(), "disable_trace",
                               balancer::tcl_disable_trace, static_cast<ClientData>(balancer), 0)) {
        LogError(STDLOG) << "create disable_trace command failed: "
                         << Tcl_GetString(Tcl_GetObjResult(getTclInterpretator()));
        return TCL_ERROR;
    }

    return TCL_OK;
}

int main_balancer(int cp /*control pipe descriptor*/, int argc, char* argv[])
{
    ASSERT(1 < argc);

    set_signal(term3);

    const std::string grp(argv[1]);
    set_cur_grp_string(grp.c_str());

    ServerFramework::ControlPipeEvent ev;

    if (0 != cp) {
        ev.init();
    }

    const auto traceableClientIds = tcl_utils::readListOfIntsFromTcl<uint16_t>(grp + "(TRACEABLE_CLIENT_IDS)");

    Balancer<balancer::BalancerFrontend> balancer(
            ServerFramework::system_ios::Instance(),
            readIntFromTcl(grp + "(HEADTYPE)"),
            readStringFromTcl(grp + "(IP_ADDRESS)", "0.0.0.0"),
            readIntFromTcl(grp + "(PORT)"),
            readIntFromTcl(grp + "(MAX_OPEN_CONNECTIONS)", 0),
            readIntFromTcl(grp + "(K1)", 64),
            readIntFromTcl(grp + "(K2)", 1),
            std::set<uint16_t>(std::cbegin(traceableClientIds), std::cend(traceableClientIds)),
            1 < readIntFromTcl(grp + "(NUMBER_OF_LISTENERS)", 1)
    );

    if (const auto res = balancer::createCommand(&balancer)) {
        return res;
    }

    tcl_read_config(&balancer, getTclInterpretator(), 0, static_cast<Tcl_Obj* CONST*>(0));

    return balancer.run();
}

int main_balancer_clients_stats(int cp /*control pipe descriptor*/, int argc, char* argv[])
{
    set_signal(term3);

    ASSERT(1 < argc);

    const std::string grp(argv[1]);
    set_cur_grp_string(grp.c_str());

    ServerFramework::ControlPipeEvent ev;

    if (0 != cp) {
        ev.init();
    }

    ClientStatsAssistant< ClientStatsFrontend > assistant(
            ServerFramework::system_ios::Instance(),
            readIntFromTcl(grp + "(HEADTYPE_STATS)"),
            readStringFromTcl(grp + "(IP_ADDRESS_STATS)", "0.0.0.0"),
            readIntFromTcl(grp + "(PORT_STATS)")
    );

    return assistant.run();
}

} // namespace balancer
