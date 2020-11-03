#ifndef SERVERLIB_LOCAL_TCP_EVENT_H
#define SERVERLIB_LOCAL_TCP_EVENT_H

#include "daemon_event.h"

namespace ServerFramework
{

class LocalTcpDaemonEvent
    : public DaemonEvent
{
public:
    LocalTcpDaemonEvent(const char* socketName);
    virtual ~LocalTcpDaemonEvent() {}
    virtual void init();
private:
    boost::shared_ptr<void> pImpl_;
};

} // namespace ServerFramework


#endif /* SERVERLIB_LOCAL_TCP_EVENT_H */

