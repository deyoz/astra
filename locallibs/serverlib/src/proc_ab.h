#ifndef __PROC_AB_H_
#define __PROC_AB_H_

#include <string>

namespace ServerFramework {

struct Aparams
{
    std::pair<std::string, uint16_t> addr;
    uint16_t obrzaps;
    uint32_t headtype;
    std::string ipc_c;
    std::string ipc_signal;
    time_t msg_expired_timeout;
    struct Queue {
        uint16_t drop;
        uint16_t warn;
        uint16_t warn_log;
        bool full_timestamp;
    } queue;
};

struct ATcpParams : public Aparams
{
    size_t max_connections;
    std::pair<std::string, uint16_t> balancerAddr;
};

int proc_ab_tcp_impl(int control_pipe, const ATcpParams& p);
int proc_ab_fcg_impl(int control_pipe, const ATcpParams& p);
int proc_ab_udp_impl(int control_pipe, const Aparams& p);
int proc_ab_http_impl(int control_pipe, const ATcpParams& p);
int proc_ab_http_secure_impl(int control, const ATcpParams& p);

}

#endif // __PROC_AB_H_
