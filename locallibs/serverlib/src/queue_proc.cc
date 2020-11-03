#if HAVE_CONFIG_H
#endif


#include <errno.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <string.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <map>
#include <string>
#include <cstdlib>

#include <tcl.h>

#include "tcl_utils.h"
#include "tclmon.h"
#include "testmode.h"
#include "monitor_ctl.h"

#define NICKNAME "SYSTEM"
#include "test.h"

void send_signal(const char* address, const void* data, const size_t len)
{
    uint32_t msgid[4] = {};
    if(len >= 16)
        memcpy(msgid, static_cast<const uint32_t*>(data), 16);
    ProgTrace(TRACE1, "send_signal(type=%0x) { %u %u %u } to %s %zu bytes", msgid[0], msgid[1], msgid[2], msgid[3], address, len);

    sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, address);
    if (access(addr.sun_path, W_OK) < 0) {
        ProgError(STDLOG, "access %s %s", addr.sun_path, strerror(errno));
        return;
    }

    static std::map<std::string, int> socketmap;
    std::map<std::string, int>::iterator fdi = socketmap.find(addr.sun_path);
    int fd = 0;
    if (fdi == socketmap.end()) {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            ProgError(STDLOG, "socket %s", strerror(errno));
            sleep(5);
            abort();
        }
        if (connect(fd,(sockaddr*)&addr, sizeof(addr)) < 0)
        {
            if(!startupKeepSilence())
                ProgError(STDLOG, "connect %s %s", addr.sun_path, strerror(errno));
            else
                ProgTrace(TRACE1, "connect %s %s", addr.sun_path, strerror(errno));

            shutdown(fd, SHUT_RDWR);
            close(fd);
            return;
        }
        socketmap[addr.sun_path] = fd;
    } else {
        fd = fdi->second;
    }

    iovec iv[2] = {};
    uint32_t head = htonl(len);
    iv[0].iov_base = &head;
    iv[0].iov_len = sizeof(head);
    iv[1].iov_base = const_cast<void*>(data);
    iv[1].iov_len = len;

    msghdr msg = {};
    msg.msg_iov = iv;
    msg.msg_iovlen = 2;
    const ssize_t n = sendmsg(fd, &msg, MSG_NOSIGNAL);
    if (static_cast<size_t>(n) != len + sizeof(head)) {
        if (errno != EPIPE) {
            ProgError(STDLOG, "writew %s", strerror(errno));
            sleep(5);
            abort();
        } else {
            ProgError(STDLOG, "EPIPE(%s) on socket: %s", strerror(errno), addr.sun_path);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            fdi = socketmap.find(addr.sun_path);
            socketmap.erase(fdi);
        }
    }

}

void send_signal_tcp_inet(const char* address, short port, const void* data, const size_t len)
{
    const std::string hostkey = std::string(address) + ":" + std::to_string(port);

    static std::map<std::string, int> socketmap;
    std::map<std::string, int>::iterator fdi = socketmap.find(hostkey);
    int fd = 0;
    if (fdi == socketmap.end()) {
        const hostent* server = gethostbyname(address);
        if (server == NULL) {
            ProgError(STDLOG, "gethostbyname failed: %.120s", address);
            return;
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr = *((struct in_addr *)server->h_addr);
        addr.sin_port = htons(port);
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            ProgError(STDLOG, "socket %s", strerror(errno));
        }
        if (connect(fd,(sockaddr*)&addr, sizeof(addr)) < 0)
        {
            if(!startupKeepSilence())
                ProgError(STDLOG, "connect %s %s", hostkey.c_str(), strerror(errno));
            else
                ProgTrace(TRACE1, "connect %s %s", hostkey.c_str(), strerror(errno));

            shutdown(fd, SHUT_RDWR);
            close(fd);
            return;
        }
        socketmap[hostkey] = fd;
    } else {
        fd = fdi->second;
    }

    iovec iv[2] = {};
    uint32_t head = htonl(len);
    iv[0].iov_base = &head;
    iv[0].iov_len = sizeof(head);
    iv[1].iov_base = const_cast<void*>(data);
    iv[1].iov_len = len;

    msghdr msg = {};
    msg.msg_iov = iv;
    msg.msg_iovlen = 2;
    const ssize_t n = sendmsg(fd, &msg, MSG_NOSIGNAL);
    if (static_cast<size_t>(n) != len + sizeof(head)) {
        ProgError(STDLOG, "'%s' on socket: %s", strerror(errno), hostkey.c_str());
        shutdown(fd, SHUT_RDWR);
        close(fd);
        fdi = socketmap.find(hostkey);
        socketmap.erase(fdi);
    }

}
extern "C" void send_signal_tcp(const char* var, const char* var2, int suff,
                                const void* data, size_t len)
{
#ifdef XP_TESTING
    if (inTestMode()) {
        return;
    }
#endif // XP_TESTING

    std::string address;
    if (var) {
        Tcl_Obj* obj = Tcl_GetVar2Ex(getTclInterpretator(),
                (char*)var, (char*)var2, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (!obj) {
            ProgError(STDLOG, "%.30s(%.50s):%s", var, var2 ? var2 : "null",
                    Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
            sleep(5);
            abort();
        }
        address.assign(Tcl_GetString(obj));
    } else {
        address.assign(var2);
    }
    if(suff >= 0)
    {
        char tmpb[100];
        sprintf(tmpb, "%03d", suff);
        address += tmpb;
    }

    send_signal(address.c_str(), data, len);
}

