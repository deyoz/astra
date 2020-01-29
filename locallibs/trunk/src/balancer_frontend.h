#ifndef _SERVERLIB_BALANCER_FRONTEND_H_
#define _SERVERLIB_BALANCER_FRONTEND_H_

#include "base_frontend.h"

namespace balancer {

template<typename T>
class Balancer;

template<typename T>
class Stats;

class BalancerFrontend
    : public Dispatcher::bf::BaseTcpFrontend<Balancer<BalancerFrontend> >
{
public:
    BalancerFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const uint16_t port,
            balancer::Balancer<BalancerFrontend>& d,
            const size_t maxOpenConnections,
            std::set<uint16_t>&& traceableClientIds,
            const bool multiListener)
        : BaseTcpFrontend(io_s, headType, std::make_pair(addr, port), boost::none, d,
                          maxOpenConnections, std::move(traceableClientIds), multiListener)
    { }

    BalancerFrontend(const BalancerFrontend& ) = delete;
    BalancerFrontend& operator=(const BalancerFrontend& ) = delete;

    BalancerFrontend(BalancerFrontend&& ) = delete;
    BalancerFrontend& operator=(BalancerFrontend&& ) = delete;
};

class ClientStatsFrontend
    : public Dispatcher::bf::BasePlainHttpFrontend< balancer::ClientStatsAssistant< ClientStatsFrontend > >
{
public:
    ClientStatsFrontend( boost::asio::io_service& io_s,
                   const uint8_t headType,
                   const std::string& addr,
                   const uint16_t port,
                   balancer::ClientStatsAssistant< ClientStatsFrontend >& d);

    ~ClientStatsFrontend() override = default;

    ClientStatsFrontend(const ClientStatsFrontend& ) = delete;
    ClientStatsFrontend& operator=(const ClientStatsFrontend& ) = delete;

    ClientStatsFrontend(ClientStatsFrontend&& ) = delete;
    ClientStatsFrontend& operator=(ClientStatsFrontend&& ) = delete;
};

} // namespace balancer

#endif //_SERVERLIB_BALANCER_FRONTEND_H_
