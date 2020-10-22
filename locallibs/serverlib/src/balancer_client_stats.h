#pragma once

#include <boost/asio/io_service.hpp>

#include "daemon_impl.h"

#include "balancer_locks.h"

namespace balancer {

class ClientIdStats
{
public:
    static constexpr size_t CLIENT_IDS_COUNT = std::numeric_limits< uint16_t >::max() + 1;

public:
    struct Stats
    {
        uint32_t incoming;
        uint32_t outgoing;
        uint32_t waitTime;
    };

public:
    ClientIdStats();

    ClientIdStats(const ClientIdStats& ) = delete;
    ClientIdStats& operator=(const ClientIdStats& ) = delete;

    ClientIdStats(ClientIdStats&& ) = delete;
    ClientIdStats& operator=(ClientIdStats&& ) = delete;

    ~ClientIdStats();

    Stats get();

    void incoming();
    void outgoing(const uint32_t wt);

    Stats clear();

private:
    std::pair< uint32_t, uint32_t > get(const uint64_t s); // [ waitTime, outgoingCount ]

private:
    std::atomic< uint32_t > incomingCount_;
    std::atomic< uint64_t > outgoingStats_; // uint32_t (waitTime_) << sizeof(uint32_t) | uint32_t (outgoingCount_)
};

template< typename T >
class ClientStatsAssistant
{
public:
    enum class WHAT_DROP_IF_QUEUE_FULL { FRONT = 0, BACK = 1 };

    struct OutgoingStats
    {
        uint32_t count;
        double waitTimeAvg;
    };

    struct Stats
    {
        uint32_t incoming;
        boost::optional< OutgoingStats > outgoing;
    };

    struct ClientStats
    {
        boost::posix_time::ptime beginUtc; // UTC. Include begin value.
        boost::posix_time::ptime endUtc;   // UTC. Exclude end value.
        std::map< unsigned, Stats > stats;
    };

public:
    ClientStatsAssistant(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port);

    //-----------------------------------------------------------------------
    ~ClientStatsAssistant();
    //-----------------------------------------------------------------------
    ClientStatsAssistant(const ClientStatsAssistant& ) = delete;
    ClientStatsAssistant& operator=(const ClientStatsAssistant& ) = delete;
    //-----------------------------------------------------------------------
    ClientStatsAssistant(ClientStatsAssistant&& ) = delete;
    ClientStatsAssistant& operator=(ClientStatsAssistant&& ) = delete;
    //-----------------------------------------------------------------------
    static bool proxy() {
        return false;
    }
    //-----------------------------------------------------------------------
    void takeRequest(
            const uint64_t ,
            const typename T::KeyType& clientConnectionId,
            const std::string& rep,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            WHAT_DROP_IF_QUEUE_FULL whatDropIfQueueFull);
    //-----------------------------------------------------------------------
    void release();
    //-----------------------------------------------------------------------
    void reconfigure();
    //-----------------------------------------------------------------------
    int run() {
        ServerFramework::Run();
        return 0;
    }

private:
    std::string dump(const bool clear, const boost::posix_time::ptime& end);

    void init();
    void unmap();

    void resetTimer();
    void handleTimer(const boost::system::error_code& ec);

private:
    boost::asio::io_service& service_;
    boost::asio::deadline_timer timer_;
    T frontend_;
    ClientIdStats* stats_;
    std::string dump_;
    boost::posix_time::ptime begin_;
    std::unique_ptr< Lock > shmLock_;
    std::unique_ptr< Barrier > barrier_;
};

} // namespace balancer
