#ifndef DAEMON_IMPL_H
#define DAEMON_IMPL_H

#include "loki/Singleton.h"
#include <boost/asio.hpp>

namespace ServerFramework 
{

typedef Loki::SingletonHolder<boost::asio::io_service,
        Loki::CreateUsingNew,
        Loki::DefaultLifetime,
        Loki::SingleThreaded> system_ios;

typedef Loki::SingletonHolder<boost::asio::io_service,
        Loki::CreateUsingNew,
        Loki::PhoenixSingleton,
        Loki::SingleThreaded> control_pipe_ios;

void Run();

} // ServerFramework

#endif /* DAEMON_IMPL_H */



