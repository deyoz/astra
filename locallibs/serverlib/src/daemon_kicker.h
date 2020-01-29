#ifndef DAEMON_KICKER_H
#define DAEMON_KICKER_H

#include <string>
#include <vector>

#include "posthooks.h"

namespace ServerFramework 
{

class TcpDaemonKicker
    : public Posthooks::BaseHook
{
public:
    /**
     * using user specified message
     * */
    TcpDaemonKicker(const std::string &socketName, const char* buf, size_t sz);
    TcpDaemonKicker(const std::string &socketName, size_t num, const char* buf, size_t sz);
    virtual ~TcpDaemonKicker()
    {}
    void run();
    TcpDaemonKicker * clone() const;
private:
    bool less2(const Posthooks::BaseHook *a) const noexcept;

    std::string m_socketName;
    size_t m_num;
    std::vector<char> m_buf;
};

class TcpInetDaemonKicker
    : public Posthooks::BaseHook
{
public:
    /**
     * using user specified message
     * */
    TcpInetDaemonKicker(const std::string &address, short port, const char* buf, size_t sz);
    virtual ~TcpInetDaemonKicker()
    {}
    void run();
    TcpInetDaemonKicker * clone() const;
private:
    bool less2(const Posthooks::BaseHook *a) const noexcept;
 
    std::string m_address;
    short m_port;
    std::vector<char> m_buf;
};

class DaemonKicker
    : public Posthooks::BaseHook
{
public:
    /**
     * using standard message
     * */
    DaemonKicker(const std::string &socketName, size_t num = 0);
    /**
     * using user specified message
     * */
    DaemonKicker(const std::string &socketName, const char* buf, size_t sz);
    DaemonKicker(const std::string &socketName, size_t num, const char* buf, size_t sz);
    virtual ~DaemonKicker()
    {}
    void run();
    DaemonKicker * clone() const;
private:
    bool less2(const Posthooks::BaseHook *a) const noexcept;
    
    std::string m_socketName;
    size_t m_num;
    std::vector<char> m_buf;
};

} // namespace ServerFramework
namespace comtech = ServerFramework;

#endif /* DAEMON_KICKER_H */

