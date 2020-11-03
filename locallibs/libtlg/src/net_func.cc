#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h> //  strerror
#include <unistd.h> //  close
#include <ctype.h>  //  isalnum
#include <arpa/inet.h>
#include <netdb.h>

#include <tclmon/tcl_utils.h>
#include <serverlib/exception.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

#include "gateway.h"
#include "net_func.h"

namespace telegrams
{

void setSocketOptions(int sock)
{
    int dflt_val = 0;
    int new_val = sizeof(AIRSRV_MSG);
    LIBTLG_SOCKET_LENGTH_TYPE dflt_len = sizeof(dflt_val);
    if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &dflt_val, &dflt_len) == -1) {
        ProgError(STDLOG, "getsockopt(SO_SNDBUF): %s", strerror(errno));
    } else {
        if (dflt_val < new_val) {
            int new_len = sizeof(new_val);
            if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &new_val, new_len) == -1)
                ProgError(STDLOG, "setsockopt(SO_SNDBUF): %s", strerror(errno));
            else
                ProgTrace(TRACE0, "setsockopt(): socket's SNDBUF changed %db -> %db",
                          dflt_val, new_val);
        } else
            ProgTrace(TRACE0, "socket's SNDBUF unchanged %db", dflt_val);
    }

    new_val = sizeof(struct AIRSRV_MSG);
    dflt_len = sizeof(dflt_val);
    if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &dflt_val, &dflt_len) == -1) {
        ProgError(STDLOG, "getsockopt(SO_RCVBUF): %s", strerror(errno));
    } else {
        if (dflt_val < new_val) {
            int new_len = sizeof(new_val);
            if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &new_val, new_len) == -1) {
                ProgError(STDLOG, "setsockopt(SO_RCVBUF): %s", strerror(errno));
            } else {
                ProgTrace(TRACE0, "setsockopt(SO_RCVBUF): socket's RCVBUF changed %db -> %db",
                          dflt_val, new_val);
            }
        } else {
            ProgTrace(TRACE0, "socket's RCVBUF unchanged %db", dflt_val);
        }
    }
}

unsigned get_ip(const char *IPstr)
{
    unsigned ip_num;
    struct hostent *host;
    const int sz = strlen(IPstr) + 1;
    char ip_str_buff[sz];

    strcpy(ip_str_buff, IPstr);
    char* ip_str = ip_str_buff;

    ProgTrace(TRACE4, "host before str=<%s>", ip_str);
    while ( *ip_str && !(isalnum(*ip_str) || *ip_str=='.') ) {
        ip_str++;
    }

    if ( (ip_str=strtok(ip_str, " \t\r\n")) )
        ProgTrace(TRACE4, "host after str=<%s>", ip_str);
    else {
        ProgTrace(TRACE0, "get_ip(LOCAL_HOST) NULL Error");
        return 0;
    }

    if ( inet_addr(ip_str) != INADDR_NONE )
        ip_num = inet_addr(ip_str);
    else {
        if (!(host = gethostbyname(ip_str)))
            return 0;
        else
            ip_num = *((unsigned *)(host->h_addr));
        /* memcpy((char *)&ip_num, host->h_addr,host->h_length); */
    }
    return ip_num;
}

int initSenderLocalSocket(int portnum)
{
    std::string localHost = readStringFromTcl("LOCAL_HOST", "");
    unsigned srv_ip = 0;
    if (localHost == "")
        srv_ip = htonl(INADDR_ANY);
    else
        srv_ip = get_ip(localHost.c_str());
    if (!srv_ip) {
        ProgTrace(TRACE0, "Can't get IPaddress from LOCAL_HOST");
        srv_ip = htonl(INADDR_ANY);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        ProgError(STDLOG, "socket: %s", strerror(errno));
        sleep(10);
        throw comtech::Exception(BIGLOG, "socket failed");
    }

    setSocketOptions(sockfd);
    sockaddr_in snd_addr;
    memset(&snd_addr, 0, sizeof(snd_addr));
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = srv_ip;   /* htonl(INADDR_ANY); */
    snd_addr.sin_port = htons(portnum);

    if (bind(sockfd, (struct sockaddr*)&snd_addr, sizeof(snd_addr)) < 0) {
        ProgError(STDLOG, "bind %d : %s", errno, strerror(errno));
        sleep(10);
        throw comtech::Exception(BIGLOG, "bind failed");
    }
    LogTrace(TRACE1) << "sender socket bound at " << localHost << ":" <<  portnum;
    return sockfd;
}
} // namespace telegrams

