#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <utime.h>
#include <poll.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/un.h>
#include <tcl.h>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>

#include "shmsrv.h"
#include "serverlib_md5.h"
#include "tclmon.h"
#include "object.h"
#include "guarantee_write.h"
#include "monitor_ctl.h"
#include "perfom.h"
#include "ourtime.h"
#include "mes.h"
#include "daemon_event.h"
#include "daemon_impl.h"
#include "testmode.h"
#include "noncopyable.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"
#include "test.h"

#define hard(a) my_error.severity=(a)
#define syserr(a) my_error.type=(a)
#define ERROR_CODE(a) my_error.Errno=(a)
#define err_name(a)   strncpy(my_error.name,#a,99);

extern int OBR_ID;
static const unsigned int SHM_MAGIC = 0xABCDEF09;
// структура, отслеживаюшая последнюю ошибку
struct my_error {
    int severity;       /* 1 if severe */
    int type;       /* 1 if syscall failed 0 otherwise */
    int Errno;      /* errno if syscall, internal code otherwise */
    char name[100];     /* name of failed function function  */
};

typedef struct {
  int zone;
  int len;
  short result;
  short opr;
} ShmReq;

typedef struct {
  int obrid;
  int n_ops;
  int result;
  int n_error;
  char who[25];
  void* shmid;
  unsigned int magic;
  size_t len;
  ShmReq cmd;
} ShmReqH;

typedef struct {
   List L;
   decltype(ShmReqH::who) who;
   pZone Last_Found;
} ZList;
classtype_(ZList);

struct my_error my_error;
static int shm_num;
static int avost;
static char OurID[sizeof(decltype(ShmReqH::who))+1+6]; // appending '_SHMSRV' to the `who`
static jmp_buf termpoint;

static void runLoop(int controlPipe, char *addr);
static void CleanUp();
static void command_ADD_ZONE(ShmReq *command, pZList pzl,const void *dataptr);
static void command_DEL_ZONE(ShmReq *command, pZList pzl,const void *dataptr);
static void command_UPD_ZONE(ShmReq *command, pZList pzl,
        const void *dataptr,int error);
static void command_GET_ZONE(ShmReqH *header, pZList pzl);
static void command_GET_LIST(ShmReqH *header, pZList pzl);
static void pr_Zone(pItem p);
static void dl_Zone(pItem p);
static pItem cl_Zone(pItem p);
static pZone sysFindZone(pZList L, int num);
static int sysUpdZone1(pZone pp, int num, size_t len, const void* data, const char* head);
static pZone mk_Zone(int n, size_t len, int flag, const char* head);
static int read_write_data(boost::asio::local::stream_protocol::socket& socket, ShmReqH* const req);

pvirt pvtZoneTab[pvtMaxSize]={
    dl_Zone,
    pr_Zone,
    (pvirt)cl_Zone,
};

class Shmsrv
    : private comtech::noncopyable
{
    typedef boost::asio::local::stream_protocol::socket SocketType;
private:
    class Connection
        : private comtech::noncopyable,
          public boost::enable_shared_from_this<Shmsrv::Connection>
    {
    public:
        Connection(boost::asio::io_service& service)
            : header_(), socket_(service)
        {
        }

        ~Connection()
        {
            boost::system::error_code error;
            socket_.shutdown(Shmsrv::SocketType::shutdown_both, error);
            LogTrace(TRACE1) << __FUNCTION__ << ": " << error << ": " << error.message();
        }

        SocketType& socket() {
            return socket_;
        }

        void start() {
            boost::asio::async_read(socket_, boost::asio::buffer(reinterpret_cast<char*>(&header_), sizeof(header_)),
                    boost::bind(&Shmsrv::Connection::handleReadHead, shared_from_this(), boost::asio::placeholders::error));
        }

    private:
        void handleReadHead(const boost::system::error_code& e) {
            if (!e) {
                monitor_beg_work();
                int retVal;
                if ((retVal = read_write_data(socket_, &header_)) < 0) {
                    if (-2 != retVal) {
                        LogError(STDLOG) << "read_write_data failed";
                    }
                } else {
                    start();
                }
                monitor_idle();
            } else {
                LogTrace(TRACE1) << __FUNCTION__ << " error: " << e << ": " << e.message();
            }
        }
    private:
        ShmReqH header_;
        SocketType socket_;
    };

public:
    Shmsrv(int controlPipe, const char* addr);

    ~Shmsrv();

    void run() {
        ServerFramework::Run();
    }

private:
    void accept();

    void handleAccept(const boost::system::error_code& e);

private:
    boost::asio::local::stream_protocol::acceptor acceptor_;
    boost::shared_ptr<Connection> newConnection_;
    ServerFramework::ControlPipeEvent control_;
};

Shmsrv::Shmsrv(int controlPipe, const char* addr) :
    acceptor_(ServerFramework::system_ios::Instance()),
    newConnection_(),
    control_()
{
    acceptor_.open(boost::asio::local::stream_protocol());
    acceptor_.set_option(boost::asio::local::stream_protocol::acceptor::reuse_address(true));
    acceptor_.bind(boost::asio::local::stream_protocol::endpoint(addr));
    acceptor_.listen();
    accept();
}

Shmsrv::~Shmsrv()
{
}

void Shmsrv::accept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    newConnection_.reset(new Connection(ServerFramework::system_ios::Instance()));
    acceptor_.async_accept(newConnection_->socket(),
                                 boost::bind(&Shmsrv::handleAccept,
                                             this,
                                             boost::asio::placeholders::error
                                 )
    );
}

void Shmsrv::handleAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (!e) {
        newConnection_->start();
    } else {
        LogError(STDLOG) << __FUNCTION__ << "(" << e << "): " << e.message();
    }
    accept();
}

unsigned ZonesInList(pZList that)
{
    return ListLen(&that->L);
}

pZList mk_ZList(const char* who)
{
    pZList ret;
    ProgTrace(TRACE1, "%s: sizeof *pZList = %zu", __func__, sizeof(*ret));
    ret = (pZList)malloc(sizeof(*ret));
    if (ret == NULL) {
        ProgError(STDLOG, "malloc() failed");
        return NULL;
    }
    memset(ret,0,sizeof(*ret));
    initList(&ret->L);
    strncpy(ret->who, who, sizeof(ret->who));
    ret->who[sizeof(ret->who) - 1] = 0;
    return ret;
}

int sysAddZone(pZList L, pZone p)
{
    if (sysFindZone(L, ((pZone)p)->id)) {
        ProgTrace(TRACE5, "Zone N %d", (int)((pZone)p)->id);
        return -1;
    }
    addToList(&L->L, (pItem)p);
    return 0;
}

int sysDeleteZone(pZList L, int num)
{
    pItem pp = (pItem)sysFindZone(L, num);
    if (pp) {
        delFromList2(&L->L, pp);
        return 0;
    }
    ProgTrace(TRACE5, "Zone not found %d", num);
    return -1;
}

static pZone sysFindZone(pZList L, int num)
{
    listIterator i;
    for (pItem pp = initListIterator(&i, &L->L); pp; pp = listIteratorNext(&i)) {
        if (((pZone)pp)->id == num) {
            return (L->Last_Found = (pZone)pp);
        }
    }
    return NULL;
}

int sysUpdZone(pZList L, int num, int len, void* data, const char* head)
{
    pZone pp = sysFindZone(L, num);
    if (!pp) {
        ProgTrace(TRACE5, "Zone N %d", num);
    }
    return sysUpdZone1(pp, num, len, data, head);
}

static int sysUpdZone1(pZone pp, int num, size_t len, const void* data, const char* head)
{
    if (pp == NULL) {
        tst();
        return -1;
    }

    if (pp->len != len) {
        void* pdata = realloc(pp->ptr, len);
        if (pdata == NULL) {
            ProgError(STDLOG, "realloc() "/*"or malloc() "*/"failed");
            return -2;
        }
        pp->ptr = pdata;
        pp->len = len;
    }
    if (data) {
        memcpy(pp->ptr, data, len);
    }
    if (head) {
        strncpy(pp->title, head, sizeof(pp->title));
        pp->title[sizeof(pp->title) - 1] = 0;
    }
    return 0;
}

void pr_digest(const unsigned char* p, char* out)
{
    for (int i = 0; i < 16; ++i) {
        out += sprintf(out, "%02x", *p++);
    }
}

const char* pr_digest2(const unsigned char* p)
{
    static char out[100];
    pr_digest(p, out);
    return out;
}

static void pr_Zone(pItem p)
{
    pZone pz = (pZone)p;

    ProgTrace(TRACE4, "Zone: %i, Head: %.30s, Length: %zu\nn"
            "flag=%08x ptr=0x%p\n"
            "Data: %.s", pz->id, pz->title,
            pz->len, pz->flag, pz->ptr, (char*)pz->ptr);

}

static void sysInitZone(pZone p)
{
    p->I.pvt = pvtZoneTab;
}

static pItem cl_Zone(pItem p)
{
    pZone res;
    if (p == NULL)
    { return NULL; }
    res = mk_Zone(((pZone)p)->id, ((pZone)p)->len, ((pZone)p)->flag,
            ((pZone)p)->title);
    if (res == NULL) {
        ProgError(STDLOG, "mk_Zone() failed");
        return NULL;
    }
    memcpy(res->ptr, ((pZone)p)->ptr, res->len);
    return (pItem)res;
}

static void dl_Zone(pItem p)
{
    if (p) {
        free(((pZone)p)->ptr);
    }
}

static pZone mk_Zone(int n, size_t len, int flag, const char* head)
{
    pZone ret = static_cast<pZone>(malloc(sizeof(Zone)));
    if (ret == NULL) {
        ProgError(STDLOG, "malloc failed");
        return NULL;
    }
    sysInitZone(ret);
    ret->ptr = malloc(len);
    if (ret->ptr == NULL) {
        ProgError(STDLOG, "malloc failed trying to get %zu bytes", len);
        free(ret);
        tst();
        return NULL;
    }
    ret->flag = flag;
    ret->title[0] = '\0';
    if(head) {
        strncpy(ret->title, head, sizeof(ret->title));
        ret->title[sizeof(ret->title) - 1] = 0;
    }
    ret->len = len;
    ret->id = n;
    return ret;
}

static void CleanUp()
{
}

static void on_term(int a)
{
    static int first = 1;

    if (a == SIGPIPE) {
        ProgError(STDLOG, "SIGPIPE caught");
        return;
    }

    WriteLog(STDLOG,
            (a == SIGINT || a == -1) ?
            "Received signal - %d, exiting ..." : "Killed in action - %d " , a);
    if (a != SIGINT) {
        avost = 1;
    }
    if (a != SIGINT && a != SIGTERM) {
        monitor_restart();
    }
    tst();
    if (first) {
        tst();
        first = 0;
        tst();
        longjmp(termpoint, 1);
    }
    tst();
}

const char* str_my_error(void)
{
    static  char out[200];
    sprintf(out, "%s%s errcode %d %s%s\n%s", my_error.severity ? "severe " : "",
            my_error.type == 1 ? "syscall " : "",
            my_error.Errno, my_error.type == 1 ? "\n" : "",
            my_error.type == 1 ? strerror(errno) : "", my_error.name);
    return out;
}

int main_shmserv(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    char timestr[40];
    char addr[30];
    Tcl_Obj* obj = NULL;
    struct sigaction sigact;
    sigset_t sigset;
    sigemptyset(&sigset);
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_mask = sigset;


    shm_num = boost::lexical_cast<int>(argv[1]);
    if ((obj = Tcl_ObjGetVar2(getTclInterpretator(),
                    Tcl_NewStringObj("SHMSERV", -1), 0, TCL_GLOBAL_ONLY |
                    TCL_LEAVE_ERR_MSG)) == 0) {
        printf("ERROR:main_shmserv wrong parameter SHMSERV:%s\n",
                Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
        return 1;
    }

    sprintf(addr, "%s-%d", Tcl_GetString(obj), shm_num);
    strcpy(OurID, "SHMSRV");
    if (InitLogTime(OurID) < 0) {
        strcpy(timestr, "time?????");
    } else {
        strftime(timestr, 39, "%c", &getOurTime()->tm);
    }

    WriteLog(STDLOG, "Shared memory server started %s", timestr);
    monitor_idle();
    monitor_special();

    {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0 || dup2(fd, 1) < 0) {
            ProgError(STDLOG, "stdout redirection failed");
        }
    }

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    if (setjmp(termpoint) == 0) {
        sigact.sa_handler = on_term;
        if (sigaction(SIGINT, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGINT");
        }
        sigact.sa_handler = on_term;
        if (sigaction(SIGPIPE, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGPIPE");
        }
        sigact.sa_handler = on_term;
        if (sigaction(SIGTERM, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGTERM");
        }
        sigact.sa_handler = sigusr2;
        if (sigaction(SIGUSR2, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGUSR2");
        }

        if (!getenv("BROKEN_GDB"))      {
            sigact.sa_handler = on_term;
            if (sigaction(SIGSEGV, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGSEGV");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGBUS, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGBUS");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGFPE, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGFPE");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGILL, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGILL");
            }
        }
        runLoop(supervisorSocket, addr);
    }
    tst();
    CleanUp();

    if (!avost) {
        return 0;
    }

    return 1;
}

static void runLoop(int controlPipe, char* addr)
{
    unlink(addr);

    Shmsrv srv(controlPipe, addr);
    srv.run();
}

static int read_n_bytes(int fd, size_t n, void* buf)
{
    static const int POLL_TIMEOUT = 5 * 1000;

    size_t nread_total = 0;
    struct pollfd pfd = {pfd.fd = fd, pfd.events = POLLIN, pfd.revents = 0};

    while (1) {
        const int ret = poll(&pfd, 1, POLL_TIMEOUT);
        if (ret < 0) {
            hard(1);
            syserr(1);
            ERROR_CODE(errno);
            err_name("poll");
            return -1;
        }
        if (!ret) {
            hard(1);
            syserr(0);
            ERROR_CODE(0);
            err_name("poll_timeout");
            return -1;
        }
        if (pfd.revents & POLLERR || pfd.revents & POLLNVAL) {
            LogError(STDLOG) << __FUNCTION__ << " POLLERR or POLLNVAL";
            return -1;
        }
        if (pfd.revents & POLLHUP) {
            return -2;
        }

        auto nread = read(fd, static_cast<uint8_t*>(buf) + nread_total, n - nread_total);
        if (0 == nread) {
            return -2;
        }
        if (nread < 0) {
            hard(1);
            syserr(1);
            ERROR_CODE(errno);
            err_name(read);
            return -1;
        }
        nread_total += nread;
        if (nread_total == n) {
            break;
        }
    }
    return 0;
}

#define IAMHERE ProgTrace(TRACE1, "%s", __func__);

static int read_write_data(boost::asio::local::stream_protocol::socket& socket, ShmReqH* const header)
{
    IAMHERE
    int i, needadd;

    int NEw = 0;
    int n_ops = 0;
    ShmReq*  commands;
    char* dataptr, *endptr;
    char timestr[50];
    boost::system::error_code error;

    if (header->magic != SHM_MAGIC) {
        ProgError(STDLOG, "wrong magic");
        return -1;
    }

    snprintf(OurID, sizeof(OurID), "%s%.15s", header->who, "_SHMSRV");
    if (InitLogTime(OurID) < 0)
    { strcpy(timestr, "time?????"); }
    else
    { strftime(timestr, 39, "%c", &getOurTime()->tm); }
    ProgTrace(TRACE1, "Receive : n_ops=%d,cmd=%d,zone=%d,shmid=%p\n"
            "obrid=%d,who=<%.29s>", header->n_ops,
            header->cmd.opr, header->cmd.zone,
            header->shmid, header->obrid, header->who);


    /* сфоpмиpовать ответ */
    if (header->shmid == NULL) {
        if (header->n_ops != 1) {
            ProgError(STDLOG, "n_ops!=1");
            commands = NULL;
        } else {
            commands = &header->cmd;
        }
    } else {
        header->shmid = commands = (ShmReq*)malloc(header->len);
        if (commands == NULL) {
            header->result = SERVER_ERROR;
            ProgError(STDLOG, "malloc failed");
        } else {
            boost::asio::read(socket, boost::asio::buffer(reinterpret_cast<char*>(commands), header->len),
                    boost::asio::transfer_all(), error);
            if (error) {
                if (boost::asio::error::eof != error) {
                    LogError(STDLOG) << "boost::asio::read failed with error: " << error << ": " << error.message();
                }
                free(commands);
                header->shmid = commands = NULL;
            }
        }
    }

    header->n_error = 0;
    if (commands != NULL) {
        static std::unordered_map<std::string, pZList> htab;
        auto bptr = htab.emplace(header->who, nullptr).first;

        pZList pzl = bptr->second;
        if (pzl == NULL) {
            tst();
            pzl = mk_ZList(header->who);
            if (pzl == NULL) {
                ProgError(STDLOG, "Cannot mk_ZList()");
                header->result = SERVER_ERROR;
            } else {
                NEw = 1;
                header->result = 0;
            }
        } else {
            header->result = 0;
        }

        dataptr = (char*)(commands + header->n_ops);
        endptr = (char*)commands + header->len;
        ProgTrace(TRACE5, "DataLen=%td %zi (ShmReq=%zu hlen=%zu n_ops=%i)", endptr - dataptr, header->len - header->n_ops * sizeof(ShmReq), sizeof(ShmReq), header->len, header->n_ops);
        n_ops = header->n_ops; /* header->n_ops может измениться */
        for (i = 0; header->result == 0 && i < n_ops ; i++) {
            commands[i].result = 0;
            needadd = 1;
            ProgTrace(TRACE1, "cmd=%d zone=%d len=%d", commands[i].opr,
                    commands[i].zone, commands[i].len);
            switch (commands[i].opr) {
            case ADD_ZONE:
                if (dataptr + commands[i].len > endptr) {
                    ProgError(STDLOG, "command N %d length=%d", i,
                            commands[i].len);
                    header->result = REQUEST_ERROR;
                    break;
                }
                command_ADD_ZONE(commands + i, pzl, dataptr);
                if (commands[i].result) {
                    ProgError(STDLOG, "command_ADD_ZONE() %d", commands[i].result);
                    header->n_error++;
                }
                break;
            case DEL_ZONE:
                if (dataptr + commands[i].len > endptr) {
                    ProgError(STDLOG, "command N %d length=%d", i,
                            commands[i].len);
                    header->result = REQUEST_ERROR;
                    break;
                }
                command_DEL_ZONE(commands + i, pzl, dataptr);
                if (commands[i].result) {
                    ProgError(STDLOG, "command_DEL_ZONE() %d", commands[i].result);
                    header->n_error++;
                }
                break;
            case UPD_ZONE:
                if (dataptr + commands[i].len > endptr) {
                    ProgError(STDLOG, "command N %d length=%d", i,
                            commands[i].len);
                    header->result = REQUEST_ERROR;
                    break;
                }
                command_UPD_ZONE(commands + i, pzl, dataptr, 1);
                if (commands[i].result) {
                    ProgError(STDLOG, "command_UPD_ZONE() %d", commands[i].result);
                    header->n_error++;
                }
                break;
            case ADU_ZONE:
                if (dataptr + commands[i].len > endptr) {
                    ProgError(STDLOG, "command N %d length=%d", i,
                            commands[i].len);
                    header->result = REQUEST_ERROR;
                    break;
                }
                command_UPD_ZONE(commands + i, pzl, dataptr, 0);
                if (commands[i].result == INVALID_ZONE_NUMBER) {
                    tst();
                    commands[i].result = 0;
                    command_ADD_ZONE(commands + i, pzl, dataptr);
                }
                if (commands[i].result) {
                    ProgError(STDLOG, "command_UPD(ADD)_ZONE() %d", commands[i].result);
                    header->n_error++;
                }
                break;
            case GET_ZONE:
                needadd = 0;
                tst();
                if (header->n_ops != 1) {
                    ProgError(STDLOG, "Error in number of ops %d", header->n_ops);
                    commands[i].result = REQUEST_ERROR;
                } else {
                    tst();
                    command_GET_ZONE(header, pzl);
                }
                break;
            case GET_LIST:
                needadd = 0;
                if (header->n_ops != 1) {
                    ProgError(STDLOG, "Error in number of ops %d", header->n_ops);
                    commands[i].result = REQUEST_ERROR;
                } else {
#ifdef TIMING_ON
                    struct tms stm1, stm2;
                    clock_t tm1, tm2;
                    tm1 = times(&stm1);
#endif
                    command_GET_LIST(header, pzl);
#ifdef TIMING_ON
                    tm2 = times(&stm2);
                    ProgTrace(TRACE4, "GET_LIST %ld", (long)(MSec(tm2 - tm1)));
                    tst();
#endif
                }
                break;
            case EXIT_PULT:
                needadd = 0;
                tst();
                if (header->n_ops != 1 || NEw) {
                    if (header->n_ops != 1) {
                        ProgError(STDLOG, "EXIT_PULT n_ops=%d NEw=%d", header->n_ops, NEw);
                        commands[i].result = REQUEST_ERROR;
                    }
                } else {
                    deleteItem((pItem)pzl);
                    bptr->second = nullptr;
                }

                break;
            case DROP_LIST:
                needadd = 0;
                ProgTrace(TRACE1, "command_DROP_LIST");
                if (header->n_ops != 1 || NEw) {
                    if (header->n_ops != 1) {
                        ProgError(STDLOG, "DROP_LIST n_ops=%d NEw=%d", header->n_ops, NEw);
                        commands[i].result = REQUEST_ERROR;
                    }
                } else {
                    flushList(&pzl->L);
                }
                break;

            default:
                TST();
                commands[i].result = REQUEST_ERROR;
                break;
            }
            tst();
            if (needadd) {
                dataptr += commands[i].len;
                tst();
            }
        }

        if (header->result != 0 || (NEw && ZonesInList(pzl) == 0)) {
            ProgTrace(TRACE1,"%s: header->result=%i NEw=%i ZonesInList(pzl)=%u", __func__, header->result, NEw, ZonesInList(pzl));
            deleteItem((pItem)(pzl));
            bptr->second = nullptr;
        } else if (NEw) {
            ProgTrace(TRACE1,"%s: NEw=%i", __func__, NEw);
            bptr->second = pzl;
        } else {
            ProgTrace(TRACE1,"%s: not setting bptr->p", __func__);
        }
    }

    if (n_ops != 1 && header->len) {
        tst();
        header->len = sizeof(ShmReq) * n_ops;
    }

    /* записать в память  6 */
    ProgTrace(TRACE4, "write: header->len=%zu", header->len);
    header->magic = SHM_MAGIC;

    boost::asio::write(socket, boost::asio::buffer(header, sizeof(ShmReqH)), boost::asio::transfer_all(), error);
    if (error) {
        LogError(STDLOG) << "boost::asio::write failed: " << error << ": " << error.message();
        free(header->shmid);
        header->shmid = 0;
        return -1;
    }
    if (header->len != 0 && header->shmid != NULL) {
        boost::asio::write(socket, boost::asio::buffer(header->shmid, header->len), boost::asio::transfer_all(), error);
        if (error) {
            LogError(STDLOG) << "boost::asio::write failed: " << error << ": " << error.message();
            free(header->shmid);
            header->shmid = 0;
            return -1;
        }
    }
    free(header->shmid);
    header->shmid = 0;

    return 0;
}

static void command_ADD_ZONE(ShmReq* command, pZList pzl, const void* dataptr)
{
    IAMHERE
    pZone p, p2;
    Zone Z;
    p = const_cast<pZone>(reinterpret_cast<pcZone>(dataptr));
    if (p == NULL) {
        TST();
        command->result = SERVER_ERROR;
    } else if (memcpy(&Z, p, sizeof(Z)), Z.ptr = p + 1, p = &Z, (p->len <= 0)) {
        TST();
        command->result = REQUEST_ERROR;
    } else {
        Z.I.pvt = pvtZoneTab;
        p2 = (pZone)cloneItem((pItem)&Z);
        if (p2 == NULL) {
            TST();
            command->result = SERVER_ERROR;
            /*we can also try old zones and delete them*/
        } else if (sysAddZone(pzl, p2) < 0) {
            deleteItem((pItem)p2);
            command->result = INVALID_ZONE_NUMBER;
        }
    }
}

static void command_DEL_ZONE(ShmReq* command, pZList pzl, const void* dataptr)
{
    IAMHERE
    if (sysDeleteZone(pzl, command->zone) < 0)
    { command->result = INVALID_ZONE_NUMBER; }
}

static void command_UPD_ZONE(ShmReq* command, pZList pzl, const void* dataptr, int error)
{
    IAMHERE
    pZone p;
    Zone Z;
    p = const_cast<pZone>(reinterpret_cast<pcZone>(dataptr));
    if (p == NULL) {
        TST();
        command->result = SERVER_ERROR;
    } else if (memcpy(&Z, p, sizeof(Z)), Z.ptr = p + 1, p = &Z,
            (p->len <= 0)) {
        TST();
        command->result = REQUEST_ERROR;
    } else {
        int ret = sysUpdZone(pzl, command->zone, p->len, p->ptr, p->title);
        if (ret < 0) {
            if (ret != -1 || error == 1) {
                ProgError(STDLOG, "zone %d,len %d,title %.10s",
                        (int)command->zone, (int)p->len, p->title);
                command->result = SERVER_ERROR;
            } else {
                command->result = INVALID_ZONE_NUMBER;
            }
        }
    }
}

void command_GET_LIST(ShmReqH* header, pZList pzl)
{
    IAMHERE
//    unsigned char digh2[16];
    size_t i = 0;
    ShmReq* command = &header->cmd;
    listIterator I;
    size_t datalen = 0;
    char* datptr;
    printItem((pItem)pzl);
//    (void) digh2;
    for (auto p = (pZone)initListIterator(&I, &pzl->L); p; p = (pZone)listIteratorNext(&I))
    {
        ProgTrace(TRACE1, "p->len=%zu", p->len);
        if (p->len > 0) {
            i++;
            datalen += p->len;
        }
    }
    size_t size = i * sizeof(Zone) + datalen;
    ProgTrace(TRACE1, "%s: i=%zu, sizeof(Zone)=%zu, datalen=%zu => size=%zu", __func__, i, sizeof(Zone), datalen, size);
    pZone pm = nullptr;
    if (size) {
        pm = (pZone)malloc(size);
        if (pm == nullptr) {
            ProgError(STDLOG, "malloc failed");
        }
    }
    if (pm != NULL && size) {

        datptr = (char*)(pm + i);
        i = 0;
        for (auto p = (pZone)initListIterator(&I, &pzl->L); p; p = (pZone)listIteratorNext(&I))
        {
            if (p->len > 0) {
                pm[i] = *p;
                ProgTrace(TRACE5, "p->len=%zu", p->len);
                memcpy(datptr, p->ptr, p->len);
                datptr += p->len;
                i++;
            }
        }

        header->n_ops = i;
        command->len = size;
    }
    free(header->shmid);
    header->shmid = pm;
    header->len = size;
}

static void command_GET_ZONE(ShmReqH* header, pZList pzl)
{
    IAMHERE
    pZone pm;
//    unsigned char digh2[16];
    int i = 0;
    ShmReq* command = &header->cmd;
    listIterator I;
    pZone p;
    int datalen = 0;
    char* datptr;
    int size;
    printItem((pItem)pzl);
//    (void) digh2;
    tst();
    for (p = (pZone)initListIterator(&I, &pzl->L); p;
            p = (pZone)listIteratorNext(&I)) {
        ProgTrace(TRACE5, "p->id %d, command->zone %d", p->id, command->zone);
        if (p->id != command->zone) {
            continue;
        }
        tst();
        if (p->len > 0) {
            i++;
            datalen += p->len;
        }
        break;
    }
    size = i * sizeof(Zone) + datalen;

    if (size) {
        pm = (pZone)malloc(size);
        if (pm == NULL) {
            ProgError(STDLOG, "malloc failed");
        }

    } else {
        pm = NULL;
    }
    tst();
    if (pm != NULL && size) {
        tst();
        datptr = (char*)(pm + 1);
        if (p->len > 0) {
            pm[0] = *p;
            ProgTrace(TRACE5, "p->len=%zu", p->len);
            memcpy(datptr, p->ptr, p->len);
            //datptr += p->len;
            header->n_ops = 1;
        } else {
            header->n_ops = 0;
        }
        command->len = size;
    }
    free(header->shmid);
    header->shmid = pm;
    header->len = size;

}

//Client part
static int Initialized;
static int local_test_mode;
static unsigned int curshm;
static pZList ClientZL; /*указатель на список зон клиентского процесса*/
static pZList OldClientZL;
static ShmReqH private_header; /*локальный заголовок запроса и указатель на разделяемый заголовок*/
static int shm_sock[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static char Pul[30];

static int process_command(ShmReqH& private_header);
static int get_list(pZList pl, const char* who);
static int process_list(pZList pl, const char* who, ShmReqH& private_header);

enum class Drop { False=0, True=1 };
static int  InitZoneList2(Drop = Drop::False);

void printZoneList()
{
    ProgTrace(TRACE3, "Current Zone List");
    printItem((pItem)ClientZL);
}

void setShmservTestMode()
{
    local_test_mode = 1;
}

pZone NewZone(int n, size_t len, const char* head)
{
    ProgTrace(TRACE5, "%s(zone#=%i of len %zu, head=%s)", __func__, n, len, head);
    pZone ret;
    int flag = SHMA_NEW | SHMA_CHNGD;

    if (!Initialized) {
        InitZoneList2();
    }
    if (/*!Initialized ||*/ ClientZL == NULL) {
        return NULL;
    }
    if ((ret = sysFindZone(ClientZL, n))) {
        flag = ((ret->flag & (~SHMA_DEL))) | SHMA_CHNGD;
        delFromList2(&ClientZL->L, (pItem)ret);
    }
    ret = mk_Zone(n, len, flag, head);
    if (!ret) {
        return NULL;
    }
    if (sysAddZone(ClientZL, ret) < 0) {
        deleteItem((pItem)ret);
        return NULL;
    }
    return ret;
}

pZone getZonePtr(int n)
{
    if (!Initialized) {
        InitZoneList2();
        ProgTrace(TRACE1, "%s(ClientZL = 0x%p)", __func__, ClientZL);
    }
    if (ClientZL == NULL) {
        tst();
        return NULL;
    }
    pZone ret = sysFindZone(ClientZL, n);
    if (ret && (ret->flag & SHMA_DEL))
    { tst(); return NULL; }
    return ret;
}

int setZoneData(pZone p, const void* data, size_t start, int len)
{
    if (start + len > p->len) {
        return -1;
    }
    p->flag |= SHMA_CHNGD;
    memcpy(static_cast<char*>(p->ptr) + start, data, len);

    return 0;
}

int setZoneData2(pZone p, const void* data, size_t elemsize, int start, int nelem)
{
    if (elemsize * nelem > p->len - elemsize * start) {
        return -1;
    }
    p->flag |= SHMA_CHNGD;
    memcpy((char*)p->ptr + start * elemsize, data, nelem * elemsize);

    return 0;
}

int getZoneData(const pZone p, void* data, size_t start, size_t len)
{
    if (len + start > p->len) {
        return -1;
    }
    memcpy(data, static_cast<const char*>(p->ptr) + start, len);

    return 0;
}
int getZoneData2(const pZone p, void* data, size_t elemsize, size_t start, size_t nelem)
{
    if (elemsize * (nelem + start) > p->len) {
        return -1;
    }
    memcpy(data, static_cast<const char*>(p->ptr) + start * elemsize, nelem * elemsize);

    return 0;
}


int UpdateZone(int n, size_t len, const void* data, const char* head)
{
    if (!Initialized) {
        InitZoneList2();
    }
    if (/*!Initialized ||*/ ClientZL == NULL) {
        tst();
        return -1;
    }
    if (pZone pp = sysFindZone(ClientZL, n)) {
        pp->flag &= ~SHMA_DEL;
        pp->flag |= SHMA_CHNGD;
        if (sysUpdZone1(pp, n, len, data, head) < 0) {
            tst();
            return -1;
        }
    } else {
        tst();
        return -1;
    }
    return 0;
}

int ReallocZone(pZone pp, size_t len)
{
    if (sysUpdZone1(pp, pp->id, len, NULL, NULL) < 0) {
        return -1;
    }
    pp->flag &= ~SHMA_DEL;
    pp->flag |= SHMA_CHNGD;
    return 0;
}

int DelZone2(int n, int force)
{
    pZone ret;
    if (!Initialized) {
        InitZoneList2();
    }
    if (/*!Initialized ||*/ ClientZL == NULL) {
        tst();
        return -1;
    }
    if ((ret = sysFindZone(ClientZL, n)))
    { ret->flag |= SHMA_DEL; }

    if (force != 0 && ret == 0) {
        ret = NewZone(n, 10, "<force delete>");
        if (ret) {
            ret->flag = SHMA_DEL;
        }
    }

    return ret ? 0 : -1;
}

int DelZone(int n)
{
    return DelZone2(n, 0);
}

static unsigned short hash_f2(const unsigned char * p)
{
    int ret=17;
    for (;*p;p++) {
        ret=(*p+ret)*37;
        ret=((ret>>16) ^ ret) & 0xFFFF;
    }
    ret=ret&0x003FF;

    return ret;
}

unsigned getshmn(const char* who)
{
    static int N;
    if (!N) {
        if (TCL_OK != Tcl_GetIntFromObj(getTclInterpretator(),
                    Tcl_ObjGetVar2(getTclInterpretator(),
                        Tcl_NewStringObj("SHMSERV_NUM", -1), 0,
                        TCL_GLOBAL_ONLY), &N)) {
            ProgError(STDLOG, "invalid SHMSERV_NUM in tcl config: %s",
                    Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
            N = 1;
        }
    }
    if (N > 10) {
        ProgError(STDLOG, "%d>10,set to 1", N);
        N = 1;
    }
    if (*who) {
        return hash_f2(reinterpret_cast<const unsigned char*>(who)) % N;
    } else {
        return 0;
    }
}

static void InitZoneList3()
{
    char addr[30];
    const char* pu = Pul;
    unsigned int sn = getshmn(Pul);
    curshm = sn;
    ProgTrace(TRACE3, "pult %.30s, shmserv N %d", pu, sn);
    Initialized = 0;
    Tcl_Obj* obj = Tcl_ObjGetVar2(getTclInterpretator(),
            Tcl_NewStringObj("SHMSERV", -1), 0, TCL_GLOBAL_ONLY);
    sprintf(addr, "%s-%d", Tcl_GetString(obj), sn);
    if (shm_sock[sn] >= 0) {
        int pollres;
        struct pollfd pfd;
        pfd.fd = shm_sock[sn];
        pfd.revents = pfd.events = 0;
        pollres = poll(&pfd, 1, 0);
        if (pollres) {
            ProgError(STDLOG, "Error on socket %d", pfd.revents);
            shutdown(shm_sock[sn], 2);
            close(shm_sock[sn]);
            shm_sock[sn] = -1;
        } else {
            Initialized = 1;
        }
    }

#ifdef XP_TESTING
    if (inTestMode()) {
        Initialized = 2; /*No shmserv*/
        LogTrace(TRACE0) << "In test mode work without shmserv.";
    }
#endif // XP_TESTING

    if ((-1 == shm_sock[sn]) && (2 != Initialized)) {
        static unsigned tryCount;

        const int sockfd = socket(AF_UNIX, SOCK_STREAM, 0); /* create socket */
        if (sockfd < 0) {
            hard(1);
            syserr(1);
            ERROR_CODE(errno);
            err_name(socket);
            Initialized = 2; /*No shmserv*/
            ProgError(STDLOG, "%s", str_my_error());
        }

        struct sockaddr_un serv_addr = {};
        serv_addr.sun_family = AF_UNIX;
        strcpy(serv_addr.sun_path, addr);
        int serv_addr_size = sizeof(serv_addr);   /* may be use strlen */


        tryCount = 5 + 1;
        while (--tryCount && (connect(sockfd, (struct sockaddr*)&serv_addr, serv_addr_size) < 0)) { /* conect to address*/
            LogTrace(TRACE0) << __FUNCTION__ << ": attempt connect to " << addr << " failed: (" << errno << ") - " << strerror(errno);
            random_sleep();
        }

        if (!tryCount) {
            hard(1);
            syserr(1);
            ERROR_CODE(errno);
            err_name(connect);
            Initialized = 2; /*No shmserv*/
            if (!inTestMode()) {
                ProgError(STDLOG, "%s", str_my_error());
            }
        } else {
            Initialized = 1;
            shm_sock[sn] = sockfd;
        }
    }
}
/*получить все зоны данного пульта*/
static int get_list(pZList pl, const char* who)
{
    int ret = 0;
    int crc_err;
    int crc_cycle;
    char* dataptr;
    if (!Initialized) {
        InitZoneList3();
    }
    if (Initialized != 1) {
        ProgError(STDLOG, "WORK WITHOUT SHMSERV");
        return 0;
    }
    if (pl == NULL) {
        tst();
        return -1;
    }
    for (crc_cycle = 0; crc_cycle < 3 ; crc_cycle++) {
        crc_err = 0;
        private_header.n_ops = 1;
        private_header.cmd.opr = GET_LIST;
        private_header.cmd.zone = -1;
        private_header.shmid = NULL;
        private_header.obrid = OBR_ID;
        strncpy(private_header.who, who, sizeof private_header.who);
        private_header.who[sizeof(private_header.who)-1] = '\0';
        ProgTrace(TRACE4, "before process_command, who=%s", private_header.who);
        if (process_command(private_header) < 0) {
            ProgTrace(TRACE4, "after process_command");
            return -1;
        }
        ProgTrace(TRACE4, "after process_command private_header { .result=%i .cmd.result=%i .n_ops=%i }", private_header.result, private_header.cmd.result, private_header.n_ops);

        if (private_header.result == 0 &&
                private_header.cmd.result == 0) {
            if (private_header.n_ops == 0) {
                return 0;
            }
            if(auto p = static_cast<pZone>(private_header.shmid))
            {
                dataptr = (char*)(p + private_header.n_ops);
                /* в p  находятся обекты Zone а затем данные*/
                for (int i = 0; i < private_header.n_ops; i++) {
                    p[i].ptr = dataptr;
                    p[i].I.pvt = pvtZoneTab;
                    if(auto pz = (pZone)cloneItem((pItem)(p + i)))
                    {
                        if (sysAddZone(pl, pz) < 0) {
                            ProgError(STDLOG, "Error adding zone on client");;
                        } else {
                            pz->flag = 0/*SHMA_CHNGD*/;
                        }
                        dataptr += p[i].len;

                    } else {
                        ProgError(STDLOG, "cloneItem() failed");;
                        ret = -1;
                        break;
                    }
                }
                if (ret < 0) {
                    WriteLog(STDLOG, "Zone list flushed due to previos error(s)");
                    flushList(&pl->L);
                }
                free(p);
            }
        } else {
            ProgError(STDLOG, "Server returnd error");
            ret = -1;
        }
        if (ret < 0)
        { break; }
        if (!crc_err)
        { break; }
        flushList(&pl->L);
    }
    if (crc_err && ret == 0) {
        flushList(&pl->L);
        ret = -1;
        TST();
    }
    ProgTrace(TRACE4, "leave get_list");
    return ret;
}


int EndShmOP(void) /*для множественных серверов shm*/
{
    Initialized = 0;
    return 0;
}

void InitZoneList(const char* pu)
{
    ProgTrace(TRACE1, "%s(%s)", __func__, pu);
    strcpy(Pul, pu);
    Initialized = 0;
}

struct ZList_deleter {  void operator()(pZList x){  deleteItem((pItem)x);  }  };

static int  InitZoneList2(Drop drop)
{
    if (!local_test_mode) {
        InitZoneList3();
        if (ClientZL) {
            ProgTrace(TRACE0, "before delete old ClientZL 0x%p", ClientZL);
            deleteItem((pItem)ClientZL);
            ProgTrace(TRACE0, "after delete old ClientZL");

        }
        ClientZL = mk_ZList(Pul);
    } else {
        typedef std::unique_ptr<ZList, ZList_deleter> upZList;
        static std::map<std::string, upZList> ZL;

        ClientZL = ZL[Pul].get();
        ProgTrace(TRACE1, "ZL[%s] = %p, drop=%i", Pul, ClientZL, int(drop));

        if(drop == Drop::True)
        {
            ClientZL = nullptr;
            ZL.erase(Pul);
        }
        else if(!ClientZL)
        {
            ZL[Pul] = upZList(mk_ZList(Pul));
            ClientZL = ZL[Pul].get();
            ProgTrace(TRACE1, "in test mode 0x%p", ClientZL);
            Initialized = 1;
        }
        return 0;
    }

    ProgTrace(TRACE4, "New ClientZL 0x%p", ClientZL);

    if (ClientZL) {
        if (Initialized && get_list(ClientZL, Pul) < 0) {
            tst();
            deleteItem((pItem)ClientZL);
            ClientZL = NULL;
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

void DropZoneList(void)
{
    if (ClientZL && !local_test_mode) {
        deleteItem((pItem)ClientZL);
        ClientZL = NULL;
    }
}

int SaveZoneList(void)
{
    deleteItem((pItem)OldClientZL);
    OldClientZL = ClientZL ? (pZList)cloneItem((pItem)ClientZL) : NULL;
    return OldClientZL == NULL ? -1 : 0;
}

void RestoreZoneList(void)
{
    deleteItem((pItem)ClientZL);
    ClientZL = OldClientZL;
    OldClientZL = NULL;
}

void UpdateZoneList(const char* pu)
{
    ProgTrace(TRACE1, "%s(%s)", __func__, pu);
    if (!local_test_mode) {
        ProgTrace(TRACE4, "ZL=0x%p", ClientZL);
        if (ClientZL) {
            if (Initialized == 1) {
                ProgTrace(TRACE4, "before process_list");
                process_list(ClientZL, pu, private_header);
            }
            ProgTrace(TRACE4, "before delete ClientZL");
            deleteItem((pItem)ClientZL);

            ProgTrace(TRACE4, "after delete ClientZL");
            ClientZL = NULL;
        }
    }
}
/*обновление зон на сервере*/
static int process_list(pZList pl, const char* who, ShmReqH& private_header)
{
    int ret;
    listIterator I;
    int number = 0;
    int datalen = 0;
    char* dataptr;
    pZone p;
    if (Initialized != 1) {
        ProgError(STDLOG, "WORK WITHOUT SHMSERV");
        return -1;
    }
    if (pl == NULL) {
        tst();
        return -1;
    }
    private_header.obrid = OBR_ID;
    strncpy(private_header.who, who, sizeof private_header.who);
    private_header.who[sizeof(private_header.who)-1] = '\0';
    if (ZonesInList(pl) == 0) {
        return 0;
    }
    for (p = (pZone)initListIterator(&I, &pl->L); p;
            p = (pZone)listIteratorNext(&I)) {

        if ((p->flag & SHMA_DEL)) {
            if (!(p->flag & SHMA_NEW)) {
                number++;
                datalen += sizeof(Zone);
            }
        } else if (p->flag & SHMA_CHNGD) {
            number++;
            datalen += p->len + sizeof(Zone);
        }
    }
    if (number == 0) {
        return 0;
    }
    private_header.len = sizeof(ShmReq) * number + datalen;
    ShmReq* commands = static_cast<ShmReq*>(malloc(private_header.len));
    private_header.shmid = commands;
    if (commands == NULL) {
        ProgError(STDLOG, "malloc failed to get %zu bytes", private_header.len);
        return -1;
    }
    ProgTrace(TRACE5, "DataLen=%d", datalen);
    private_header.n_ops = number;
    dataptr = (char*)(commands + number);
    number = 0;

    /*сначала лежат команды, затем Zone - data, Zone - дата*/

    for (p = (pZone)initListIterator(&I, &pl->L); p;
            p = (pZone)listIteratorNext(&I)) {

        if ((p->flag & SHMA_DEL)) {
            if (!(p->flag & SHMA_NEW)) {
                commands[number].len = sizeof(Zone);
                commands[number].zone = p->id;
                commands[number].opr = DEL_ZONE;
                ProgTrace(TRACE5, "command N %d length=%d", number,
                        commands[number].len);
                memcpy(dataptr, p, sizeof(Zone));
                dataptr += commands[number++].len;
            }
        } else if (p->flag & SHMA_CHNGD) {
            commands[number].zone = p->id;
            commands[number].len = sizeof(Zone) + p->len;
            ProgTrace(TRACE5, "command N %d length=%d", number,
                    commands[number].len);
            commands[number].opr = (p->flag & SHMA_NEW) ? ADU_ZONE : UPD_ZONE;
            memcpy(dataptr, p, sizeof(Zone));
            memcpy(dataptr + sizeof(Zone), p->ptr, p->len);
            dataptr += commands[number++].len;
        }
    }
    ret = 0;

    if (process_command(private_header) < 0) { /*запрос к серверу*/
        ProgError(STDLOG, "process_command() failed");
        ret = -1;
    } else if (private_header.n_error) {
        ProgError(STDLOG, "Server found %d error(s)", private_header.n_error);
        ret = -1;
    }
    free(private_header.shmid);
    return ret;
}

static void handleError(int fd, int shm_sock[], unsigned curshm, ShmReqH& private_header, int& isInit)
{
    LogTrace(TRACE0) << __FUNCTION__ <<
        ": fd = " << fd
        << ", curshm: " << curshm
        << ", shm_sock[curshm]: " << shm_sock[curshm]
        << ", private_header.shmid: " << private_header.shmid
        << ", Initialized: " << Initialized;

    shutdown(fd, SHUT_RDWR);
    close(fd);
    shm_sock[curshm] = -1;
    private_header.shmid = NULL;
    isInit = 0;
}

int static process_command(ShmReqH& private_header)
{
    int fd = shm_sock[curshm];
    int ret = 0;

    if (Initialized != 1) {
        return -1;
    }
    private_header.magic = SHM_MAGIC;

    if (gwrite(STDLOG, fd, &private_header, sizeof(ShmReqH)) < 0) {
        ProgError(STDLOG, "write failed %d %s", errno, strerror(errno));
        ret = -1;
    } else if (private_header.shmid
            && gwrite(STDLOG, fd, private_header.shmid, private_header.len) < 0) {
        ProgError(STDLOG, "write failed %d %s", errno, strerror(errno));
        ret = -1;
    }

    free(private_header.shmid);
    private_header.shmid = NULL;

    ProgTrace(TRACE5, "ret=%d", ret);
    ProgTrace(TRACE5, "magic=%d", private_header.magic);
    memset(&private_header, 0, sizeof private_header);

    ProgTrace(TRACE5, "magic=%d", private_header.magic);
    if (ret < 0) {
        handleError(fd, shm_sock, curshm, private_header, Initialized);
    } else {
        if (read_n_bytes(fd, sizeof(ShmReqH), &private_header) < 0) {
            ProgError(STDLOG, "read_n_bytes %s", str_my_error());
            ret = -1;
            handleError(fd, shm_sock, curshm, private_header, Initialized);
        } else if (private_header.magic != SHM_MAGIC) {
            ProgError(STDLOG, "wrong magic");
            ret = -1;
            handleError(fd, shm_sock, curshm, private_header, Initialized);
        } else {
            ProgTrace(TRACE4, "read_n_bytes -> private_header.shmid = 0x%p, private_header.len = %zu", private_header.shmid, private_header.len);
            if (private_header.shmid != NULL) {
                private_header.shmid = malloc(private_header.len);
                ProgTrace(TRACE4, "malloc 0x%p", private_header.shmid);

                if (private_header.shmid == NULL) {
                    ProgError(STDLOG, "malloc failed, %zu", private_header.len);
                    ret = -1;
                    private_header.shmid = NULL;
                }
                if (ret >= 0 && read_n_bytes(fd, private_header.len, private_header.shmid) < 0) {
                    ProgError(STDLOG, "read_n_bytes %s", str_my_error());
                    ret = -1;
                    handleError(fd, shm_sock, curshm, private_header, Initialized);
                    free(private_header.shmid);
                }
            }
        }
        ProgTrace(TRACE5, "magic=%d", private_header.magic);
    }

    return ret;
}


static int DropAllAreas2(const char* who, int del)
{

    if (Initialized != 1) {
        return -1;
    }
    private_header.n_ops = 1;
    private_header.cmd.opr = DROP_LIST;
    private_header.cmd.zone = -1;
    private_header.shmid = NULL;
    private_header.obrid = OBR_ID;
    strncpy(private_header.who, who, sizeof private_header.who);
    private_header.who[sizeof(private_header.who)-1] = '\0';

    if (process_command(private_header) < 0) { /*запрос к серверу*/
        return -1;
    }
    if (private_header.result == 0 &&
            private_header.cmd.result == 0) {
        if (del) {
            deleteItem((pItem)ClientZL);
            ClientZL = NULL;
        }
        return 0;
    }
    return -1;
}

int DropAllAreas(const char* who)
{
    ProgTrace(TRACE5, "%s(%s)", __func__, who);
    int ret;
    int del;
    char oldp[31] = {};
    strncpy(oldp, Pul, 30);

    InitZoneList(who);
    InitZoneList3();
    del = (strncmp(who, oldp, 30) == 0);
    ret = DropAllAreas2(who, del);
    if (ret < 0 && !inTestMode()) {
        ProgError(STDLOG, "DropAllAreas2 failed");
    }
    if(inTestMode())
        InitZoneList2(Drop::True);
    InitZoneList(oldp);
    InitZoneList2();
    return ret;
}

