#ifndef _SERVERLIB_BALANCER_H_
#define _SERVERLIB_BALANCER_H_

#include <list>
#include <set>
#include <unordered_map>

#include "msg_const.h"
#include "balancer_client_stats.h"

#define NICKNAME "MIXA"
#include "slogger.h"

namespace balancer {

template<typename T>
class Balancer
{
public:
    enum class WHAT_DROP_IF_QUEUE_FULL {FRONT = 0, BACK = 1};

public:
    class Node;
    class NodeSet;
    class Connection
        : public std::enable_shared_from_this<Connection>
    {
        class Request
        {
        public:
            Request(const uint64_t ccid, const typename T::KeyType& k, const uint16_t clientId,
                    const uint32_t messageId, const std::string& rep,
                    std::vector<uint8_t>& head, std::vector<uint8_t>& data)
                : ccid_(ccid),
                  key_(k),
                  clientId_ { clientId },
                  messageId_ { messageId },
                  rep_(rep),
                  head_(),
                  data_(),
                  buffers_()
            {
                assert(!rep_.empty());

                rep_.resize(IDLENA + IDLENB - 2, '\0');
                rep_[rep_.size() - 1] = '\0';

                head_.swap(head);
                data_.swap(data);

                buffers_.emplace_back(&ccid_, sizeof(ccid_));
                buffers_.emplace_back(rep_.data(),  rep_.size());
                buffers_.emplace_back(head_.data(), head_.size());
                buffers_.emplace_back(data_.data(), data_.size());
            }

            const std::vector<boost::asio::const_buffers_1>& toBuffers() const {
                return buffers_;
            }

            const uint64_t& ccid() const {
                return ccid_;
            }

            const std::weak_ptr<typename T::KeyType::element_type>& key() const {
                return key_;
            }

            uint16_t clientId() const {
                return clientId_;
            }

            uint32_t messageId() const {
                return messageId_;
            }

            Request(const Request& other) = delete;
            Request& operator=(const Request&) = delete;

        private:
            uint64_t ccid_;
            std::weak_ptr<typename T::KeyType::element_type> key_;
            const uint16_t clientId_;
            const uint32_t messageId_;
            std::string rep_; // remote endpoint
            std::vector<uint8_t> head_;
            std::vector<uint8_t> data_;
            std::vector<boost::asio::const_buffers_1> buffers_;
        };

        struct WaitClientIdData
        {
            uint16_t clientId; // From XML header
            boost::posix_time::ptime time; // Write time
            std::weak_ptr< typename T::KeyType::element_type > data;
        };

    public:
        Connection(boost::asio::io_service& ios, T& f, Node& n, const std::string& h, const std::string& s);
        ~Connection();

        Connection(const Connection& ) = delete;
        Connection& operator=(const Connection& ) = delete;

        Connection(Connection&& ) = delete;
        Connection& operator=(Connection&& ) = delete;

        void finalized();
        void shutdown(const bool f /*final*/);

        void write(const typename T::KeyType& ccid /*client connection id*/,
                   const uint16_t clientId /*original client id from XML header*/,
                   const uint32_t messageId /*original message id from XML header*/,
                   const std::string& rep /*remote endpoint*/,
                   std::vector<uint8_t>& head, std::vector<uint8_t>& data);

    private:
        enum class State {Live = 0, Reconnect, Final};

    private:
        void wait(void (Balancer<T>::Connection::*f)());
        void resolve();
        void connect();

        void read();
        void handleReadStat(const boost::system::error_code& e);

        void handleError(const char* functionName, const boost::system::error_code& e);

        void write();
        void handleWrite(const boost::system::error_code& e);

    private:
        boost::asio::io_service& service_;
        T& frontend_;
        Node& node_;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
        std::queue<Request> queue_;
        std::unordered_map< uint64_t, WaitClientIdData > waitClientIds_;
        boost::asio::ip::tcp::resolver::query query_;
        std::shared_ptr<boost::asio::ip::tcp::endpoint> ep_;
        uint64_t ccid_;      // Client connection id
        uint32_t queueSizeBuf_;
        uint32_t waitTimeBuf_;
        uint32_t msgSize_;   // Total length of size (include header)
        std::vector<boost::asio::mutable_buffers_1> nodeStatBuf_;
        State state_;
        unsigned attemptCount_;
    };

    class Node
        : public std::enable_shared_from_this<Node>
    {
    public:
        Node(boost::asio::io_service& ios, T& f, NodeSet& ns, const size_t id, const std::string& n /*name*/,
             unsigned cpn /*connections per node*/, const std::string& h, const std::string& s);

        Node(const Node& ) = delete;
        Node& operator=(const Node& ) = delete;

        Node(Node&& ) = delete;
        Node& operator=(Node&& ) = delete;

        ~Node();

        size_t id() const;
        const std::string& name() const;
        const std::string& group() const;

        void nodeStats(const uint32_t qs, const uint32_t wt);

        void accountRequest(const uint16_t clientId);
        void accountReply(const uint16_t clientId, const uint32_t wt);

        void stash(const std::shared_ptr<Connection>& c);
        void move(const std::shared_ptr<Connection>& c);
        std::shared_ptr<Connection> get();

        void finalized();
        void shutdown();

    private:
        void monitorNotify(const char* const action);

    private:
        NodeSet& ns_;
        const size_t id_;
        const std::string name_;
        std::list<std::shared_ptr<Connection> > connections_;
        std::set<std::shared_ptr<Connection> > stash_;
    };

    class Stats
    {
    public:
        virtual ~Stats();

        virtual void nodeStats(const size_t id, const uint32_t qs, const uint32_t wt) = 0;

        virtual void accountRequest(const uint16_t clientId) = 0;
        virtual void accountReply(const uint16_t clientId, const uint32_t wt) = 0;

        virtual void inc(const size_t id) = 0;

        virtual bool less(const size_t lhs, const size_t rhs) const = 0;

        virtual size_t id(const size_t id) const = 0;

        virtual uint32_t queueSize(const size_t id) const = 0;

        virtual size_t size() const = 0;

        virtual void finalized() = 0;

        virtual bool shared() = 0;
    };

    class LocalStats
        : public Stats
    {
        struct NodeStats
        {
            const size_t id;
            uint32_t waitTime;
            uint32_t queueSize;

            int64_t rank() const;
        };

    public:
        explicit LocalStats(const size_t count);

        LocalStats(const LocalStats& ) = delete;
        LocalStats& operator=(const LocalStats& ) = delete;

        LocalStats(LocalStats&& ) = delete;
        LocalStats& operator=(LocalStats&& ) = delete;

        virtual ~LocalStats() override;

        virtual void nodeStats(const size_t id, const uint32_t qs, const uint32_t wt) override;

        virtual void accountRequest(const uint16_t clientId) override;
        virtual void accountReply(const uint16_t clientId, const uint32_t wt) override;

        virtual void inc(const size_t id) override;

        virtual bool less(const size_t lhs, const size_t rhs) const override;

        virtual size_t id(const size_t id) const override;

        virtual uint32_t queueSize(const size_t id) const override;

        virtual size_t size() const override;

        virtual void finalized() override;

        virtual bool shared() override { return false; }

    private:
        std::vector<NodeStats> stats_;
    };

    class SharedStats
        : public Stats
    {
        class Lock
        {
            enum class State { UNLOCKED = 0, SHARED, EXCLUSIVE };

        public:
            explicit Lock(const std::string& name);
            ~Lock();

            bool try_to_unique_lock();
            void unique_lock();
            void shared_lock();

            void unlock();

        private:
            int fd_;
            State state_;
            const std::string name_;
        };

        class NodeStats
        {
        public:
            NodeStats(const size_t id);

            NodeStats(const NodeStats& ) = delete;
            NodeStats& operator=(const NodeStats& ) = delete;

            NodeStats(NodeStats&& ) = delete;
            NodeStats& operator=(NodeStats&& ) = delete;

            ~NodeStats();

            size_t id() const;

            void stats(const uint32_t qs, const uint32_t wt);

            uint32_t queueSize() const;

            bool operator<(const NodeStats& other) const;
            NodeStats& operator++(); // prefix only

        private:
            int64_t rank() const;

        private:
            const size_t id_;
            std::atomic<uint64_t> stats_; // uint32_t (waitTime_) << sizeof(uint32_t) | uint32_t (queueSize_)
        };

    public:
        SharedStats(boost::asio::io_service& ios, const std::string& shm, const std::string& lock,
                    const std::string& barrier, const size_t count);

        SharedStats(const SharedStats& ) = delete;
        SharedStats& operator=(const SharedStats& ) = delete;

        SharedStats(SharedStats&& ) = delete;
        SharedStats& operator=(SharedStats&& ) = delete;

        virtual ~SharedStats() override;

        virtual void nodeStats(const size_t id, const uint32_t qs, const uint32_t wt) override;

        virtual void accountRequest(const uint16_t clientId) override;
        virtual void accountReply(const uint16_t clientId, const uint32_t wt) override;

        virtual void inc(const size_t id) override;

        virtual bool less(const size_t lhs, const size_t rhs) const override;

        virtual size_t id(const size_t id) const override;

        virtual uint32_t queueSize(const size_t id) const override;

        virtual size_t size() const override;

        virtual void finalized() override;

        virtual bool shared() override { return true; }

    private:
        boost::asio::io_service& ios_;
        NodeStats* stats_;
        ClientIdStats* clientIdsStats_;
        const size_t count_;
        std::unique_ptr< Lock > shmLock_;
        std::shared_ptr< Lock > barrier_;
    };

    class NodesGroups;

    class NodeSet
    {
        class IndexCmp
        {
        public:
            explicit IndexCmp(Stats& s)
                : stats_(s)
            { }

            bool operator()(const size_t lhs, const size_t rhs) const {
                return stats_.less(lhs, rhs);
            }

        private:
            Stats& stats_;
        };

        class Index
        {
        public:
            virtual ~Index();

            virtual void emplace(const size_t id) = 0;
            virtual bool remove(const size_t id) = 0;
            virtual void stash(const size_t id) = 0;

            virtual size_t min() const = 0;
            virtual size_t max() const = 0;

            virtual bool empty() const = 0;
        };

        class SingleIndex
            : public Index
        {
        public:
            SingleIndex(Stats& s);

            virtual ~SingleIndex();

            virtual void emplace(const size_t id);
            virtual bool remove(const size_t id);
            virtual void stash(const size_t id);

            virtual size_t min() const;
            virtual size_t max() const;

            virtual bool empty() const;

        private:
            std::set< size_t, IndexCmp > index_;
        };

        class SharedIndex
            : public Index
        {
        public:
            SharedIndex(Stats& s);

            virtual ~SharedIndex();

            virtual void emplace(const size_t id);
            virtual bool remove(const size_t id);
            virtual void stash(const size_t id);

            virtual size_t min() const;
            virtual size_t max() const;

            virtual bool empty() const;

        private:
            Stats& stats_;
            std::vector< size_t > index_;
        };

    public:
        NodeSet(boost::asio::io_service& ios, const std::string& name, const std::map< std::string, size_t >& nodeNames,
                std::vector<uint16_t>&& clientIds, const uint32_t queueSizeLimit, T& f, Stats& d);

        NodeSet(const NodeSet& ) = delete;
        NodeSet& operator=(const NodeSet& ) = delete;

        NodeSet(NodeSet&& ) = delete;
        NodeSet& operator=(NodeSet&& ) = delete;

        ~NodeSet();

        const std::string& name() const;
        const std::vector< uint16_t >& clientIds() const;

        void finalized();
        void shutdown();

    private:
        friend class Node;
        friend class NodesGroups;

        template<typename U>
        friend void handleShakeUpTimer(std::shared_ptr<boost::asio::deadline_timer>& timer,
                                       std::weak_ptr<typename Balancer<U>::NodeSet>& nodeSet,
                                       const std::string& name, const boost::system::error_code& e);

        std::shared_ptr<Connection> get();

        void start(const size_t id);

        void stash(const size_t id);

        void nodeStats(const size_t num, const uint32_t qs, const uint32_t wt);

        void accountRequest(const uint16_t clientId);
        void accountReply(const uint16_t qs, const uint32_t wt);

        void shakeUp();

        void monitorNotify(const char* const action);

    private:
        static constexpr unsigned CLEAN_TIMEOUT = 16u;

    private:
        Stats& stats_;
        std::unique_ptr<Index> index_;
        std::map< size_t, std::shared_ptr<Node> > nodes_;
        const std::string name_;
        const std::vector< uint16_t > clientIds_;
        const uint32_t queueSizeLimit_;
    };

    class NodesGroups
    {
        using Groups = std::vector< std::shared_ptr<NodeSet> >;

    public:
        NodesGroups(boost::asio::io_service& ios, T& f);

        ~NodesGroups();

        std::shared_ptr<Connection> get(const uint16_t clientId);

        void finalized();

        void shutdown();

    private:
        void emplace(boost::asio::io_service& ios, const std::string& name,
                     const std::map< std::string, size_t >& nodes, std::vector<uint16_t>&& clientIds,
                     const uint32_t queueSizeLimit, T& f, Stats& stats);

    private:
        Groups groups_; // Default group MUST BE first
        std::unique_ptr<Stats> stats_;
        std::array< typename Groups::size_type, std::numeric_limits<uint16_t>::max() + 1 > clientIdIndex_;
    };

public:
    Balancer(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            const size_t maxOpenConnections,
            const unsigned k1,
            const unsigned k2,
            std::set<uint16_t>&& traceableClientIds,
            const bool multiListeners);
    //-----------------------------------------------------------------------
    Balancer(const Balancer& ) = delete;
    Balancer& operator=(const Balancer& ) = delete;
    //-----------------------------------------------------------------------
    Balancer(Balancer&& ) = delete;
    Balancer& operator=(Balancer&& ) = delete;
    //-----------------------------------------------------------------------
    static bool proxy() {
        return true;
    }
    //-----------------------------------------------------------------------
    static unsigned k1() {
        return k1_;
    }
    //-----------------------------------------------------------------------
    static unsigned k2() {
        return k2_;
    }
    //-----------------------------------------------------------------------
    static void ratio(const unsigned k1, const unsigned k2);
    //-----------------------------------------------------------------------
    void reconfigure();
    //-----------------------------------------------------------------------
    bool enableTrace(const uint16_t clientId);
    //-----------------------------------------------------------------------
    bool disableTrace(const uint16_t clientId);
    //-----------------------------------------------------------------------
    void takeRequest(
            const uint64_t ,
            const typename T::KeyType& clientConnectionId,
            const std::string& rep,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull);
    //-----------------------------------------------------------------------
    int run() {
        ServerFramework::Run();
        return 0;
    }

private:
    static unsigned k1_;
    static unsigned k2_;

private:
    boost::asio::io_service& service_;
    T frontend_;
    std::unique_ptr<NodesGroups> nodesGroups_;
};

} // namespace balancer

#undef NICKNAME

#endif //_SERVERLIB_BALANCER_H_
