#ifndef NEW_DAEMON_H
#define NEW_DAEMON_H

#include <memory>
#include <string>

#include "daemon_event.h"

namespace ServerFramework 
{

class NewDaemon
{
public:
    explicit NewDaemon(const std::string& name);
    virtual ~NewDaemon();
    
    virtual void init() {}
    
    void addEvent(DaemonEventPtr pEvChecker);
    void run();
private:
    std::shared_ptr<void> pImpl_;
};

} // namespace ServerFramework

#endif /* NEW_DAEMON_H */

