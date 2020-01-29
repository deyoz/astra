#if HAVE_CONFIG_H
#endif

#include <sys/socket.h>
#include <sys/un.h>

#include "daemon_kicker.h"
#include "tclmon.h"         //  send_signal_udp_suff
#include "helpcpp.h"

#define NICKNAME "NONSTOP"
#include "test.h"

namespace
{
void initBuffer(size_t sz, const char* buf, std::vector<char>& dataBuffer)
{
    if (!sz)
        return;
    dataBuffer.resize(sz);
    memcpy(&dataBuffer[0], buf, sz);
}
}

namespace ServerFramework
{

TcpDaemonKicker::TcpDaemonKicker(const std::string &socketName, const char* buf, size_t sz)
    : m_socketName(socketName), m_num(0)
{
    initBuffer(sz, buf, m_buf);
}

TcpDaemonKicker::TcpDaemonKicker(const std::string &socketName, size_t num, const char* buf, size_t sz)
    : m_socketName(socketName), m_num(num)
{
    initBuffer(sz, buf, m_buf);
}

void TcpDaemonKicker::run()
{
    ProgTrace(TRACE5, "send signal to daemon (%s:%zd)", m_socketName.c_str(), m_num);
    send_signal_tcp(m_socketName.c_str(), NULL, m_num, &m_buf[0], m_buf.size());
    ProgTrace(TRACE1, "message sent to %s size=%zd" , m_socketName.c_str(), m_buf.size());
}

TcpDaemonKicker* TcpDaemonKicker::clone() const 
{
    return new TcpDaemonKicker(*this);
}

bool TcpDaemonKicker::less2(const Posthooks::BaseHook *a) const noexcept
{
    const TcpDaemonKicker* p = dynamic_cast<const TcpDaemonKicker*>(a);
    FIELD_LESS(this->m_socketName , p->m_socketName);
    FIELD_LESS(this->m_num , p->m_num);
    return (this->m_buf < p->m_buf);
}

TcpInetDaemonKicker::TcpInetDaemonKicker(const std::string &address, short port, const char* buf, size_t sz)
    : m_address(address), m_port(port)
{
    initBuffer(sz, buf, m_buf);
}

void TcpInetDaemonKicker::run()
{
    ProgTrace(TRACE5, "send signal to daemon (%s:%d)", m_address.c_str(), m_port);
    send_signal_tcp_inet(m_address.c_str(), m_port, &m_buf[0], m_buf.size());
    ProgTrace(TRACE1, "message sent to %s:%d size=%zd" , m_address.c_str(), m_port, m_buf.size());
}

TcpInetDaemonKicker* TcpInetDaemonKicker::clone() const
{
    return new TcpInetDaemonKicker(*this);
}

bool TcpInetDaemonKicker::less2(const Posthooks::BaseHook *a) const noexcept
{
    const TcpInetDaemonKicker* p = dynamic_cast<const TcpInetDaemonKicker*>(a);
    auto rank = [](auto const* x){ return std::tie(x->m_address, x->m_port, x->m_buf); };
    return rank(this) < rank(p);
}

DaemonKicker::DaemonKicker(const std::string &socketName, size_t num)
    : m_socketName(socketName), m_num(num)
{
    m_buf.push_back(' ');
}

DaemonKicker::DaemonKicker(const std::string &socketName, const char* buf, size_t sz)
    : m_socketName(socketName), m_num(0)
{
    initBuffer(sz, buf, m_buf);
}

DaemonKicker::DaemonKicker(const std::string &socketName, size_t num, const char* buf, size_t sz)
    : m_socketName(socketName), m_num(num)
{
    initBuffer(sz, buf, m_buf);
}

void DaemonKicker::run()
{
    ProgTrace(TRACE5, "send signal to daemon (%s:%zd)", m_socketName.c_str(), m_num);
    sockaddr_un addr = {};
    send_signal_udp_suff(&addr, m_socketName.c_str(), 0, m_num, &m_buf[0], m_buf.size());
    ProgTrace(TRACE1, "message sent to %s size=%zd" , addr.sun_path, m_buf.size());
}

DaemonKicker * DaemonKicker::clone() const 
{
    return new DaemonKicker(*this);
}

bool DaemonKicker::less2(const Posthooks::BaseHook *a) const noexcept
{
    const DaemonKicker* p = dynamic_cast<const DaemonKicker *>(a);
    auto rank = [](auto const* x){ return std::tie(x->m_socketName, x->m_num, x->m_buf); };
    return rank(this) < rank(p);
}

}

