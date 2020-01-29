#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "tclmon.h"
#include "lwriter.h"
#include "memcached_api.h"
#include "monitor_ctl.h"
#include "cursctl.h"
#include "query_runner.h"
#include "ourtime.h"
#include "profiler.h"
#include "sirenaproc.h"
#define NICKNAME "SYSTEM"
#include "test.h"
#include "perfom.h"

#ifdef ENABLE_PG
#include "pg_cursctl.h"
#endif // ENABLE_PG

using namespace OciCpp;
using std::cerr;
using std::endl;
extern "C" int our_signal(int sig, void(*f)(int), sigset_t sigs)
{
    struct sigaction in;
    in.sa_handler=f;
    in.sa_mask=sigs;
    in.sa_flags=0;
    switch (sig) {
        case SIGABRT:
        case SIGSEGV:
        case SIGILL:
        case SIGFPE:
        case SIGBUS:
        case SIGQUIT: {
            if(getenv ("BROKEN_GDB")) {
                return 0;
            }
            in.sa_flags=SA_RESETHAND;
        }
    }
    if (sigaction(sig, &in, NULL) < 0) {
        ProgTrace(TRACE0, "sigaction failed for signal: #%d", sig);
        return -2;
    }
    return 0;    
}
bool setupCore();
clock_t tm1=0;
clock_t tm2;
struct  tms stm1,stm2;
int     OBR_ID;

boost::posix_time::ptime mcsTime;

extern "C"
void PerfomInit(void)
{
  tm1=times(&stm1);
  mcsTime = boost::posix_time::microsec_clock::universal_time();
}

extern "C"
void PerfomTest(int label)
{
  tm2=times(&stm2);
/*  printf("LABEL=%d:REQUEST EXECUTION TIME - %ld ms\n",label,MSec(tm2-tm1));*/
  ProgTrace(TRACE4,"LABEL=%d:REQUEST EXECUTION TIME - %ld ms\n",label,MSec(tm2-tm1));
}

extern "C"
void PerfomTest1(int label)
{
  tm2=times(&stm2);  
/*  printf("LABEL=%d:REQUEST EXECUTION TIME - %ld ms\n",label,MSec(tm2-tm1));*/
  ProgTrace(TRACE1,"LABEL=%d:REQUEST EXECUTION TIME - %ld ms\n",label,MSec(tm2-tm1));
}

extern "C" long int TimeElapsedMcSec()
{
    long int delta = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
    return delta;
}

extern "C" int TimeElapsedMSec()
{
  tm2=times(&stm2);
  return MSec(tm2-tm1);
}

extern "C" int TimeElapsed()
{
  return TimeElapsedMSec()/1000;
}


void gprofPrepare()
{
    if(getenv("FOR_GPROF")){
        fprintf(stderr, "exit under GPROF\n");
//        char buf[100];
 //       snprintf(buf,sizeof(buf),"Gprof/%d",getpid());
 //       mkdir(buf,0755);
 //       chdir(buf);
        exit (0);
    }

}


// all right, i don't know how, but the whole this code _must_ be rewritten.
// according to ISO-IEC 14882:
//
//   1. the only global variables to be changed are 'volatile sig_atomic_t'
//
// thus, we cannot even write logs, but should set a flag instead and let
// the caller know about the cause we're exiting somewhere in destructor
//
//   2. (citation) The routine handler must be very careful, since processing
//      elsewhere was interrupted at some arbitrary point. POSIX has the concept
//      of "safe function". If a signal interrupts an unsafe function, and handler
//      calls an unsafe function, then the behavior is undefined. Safe functions
//      are listed explicitly in the various standards.
//
// The POSIX.1-2003 list is
//
// _Exit() _exit() abort() accept() access() aio_error() aio_return() aio_suspend()
// alarm() bind() cfgetispeed() cfgetospeed() cfsetispeed() cfsetospeed() chdir()
// chmod() chown() clock_gettime() close() connect() creat() dup() dup2() execle()
// execve() fchmod() fchown() fcntl() fdatasync() fork() fpathconf() fstat() fsync()
// ftruncate() getegid() geteuid() getgid() getgroups() getpeername() getpgrp()
// getpid() getppid() getsockname() getsockopt() getuid() kill() link() listen()
// lseek() lstat() mkdir() mkfifo() open() pathconf() pause() pipe() poll()
// posix_trace_event() pselect() raise() read() readlink() recv() recvfrom()
// recvmsg() rename() rmdir() select() sem_post() send() sendmsg() sendto() setgid()
// setpgid() setsid() setsockopt() setuid() shutdown() sigaction() sigaddset()
// sigdelset() sigemptyset() sigfillset() sigismember() signal() sigpause()
// sigpending() sigprocmask() sigqueue() sigset() sigsuspend() sleep() socket()
// socketpair() stat() symlink() sysconf() tcdrain() tcflow() tcflush() tcgetattr()
// tcgetpgrp() tcsendbreak() tcsetattr() tcsetpgrp() time() timer_getoverrun()
// timer_gettime() timer_settime() times() umask() uname() unlink() utime() wait()
// waitpid() write().
//
// (by the way, it means we must append to log with a special safe procedure)

extern "C" void term3(int signo)
{
    static const int BUF_SIZE = 128;
    char msg_buf[BUF_SIZE];
    volatile static sig_atomic_t waschdir = 0; // C++, ISO-IEC 14882, 3rd Ed, 1.9.6

    fprintf (stderr, "term3 called pid %d sig %d\n", getpid(), signo);
    tm2 = times(&stm2);     // damn dangerous
    long ms = MSec(tm2 - tm1);
    int msg_size = snprintf(msg_buf, BUF_SIZE, "---REQUEST EXECUTION TIME - %ld ms", ms);
    write_log_from_signal_handler(-1, STDLOG, msg_buf, (0 < msg_size) ? msg_size : 0);
    switch(signo) {
        case SIGINT: {
            static const char message[] = "Received SIGINT";
            write_log_from_signal_handler(-1, STDLOG, message, sizeof(message) - 1);
            gprofPrepare();

            break;
        }
        case SIGPIPE: {
            static const char message[] = "Received SIGPIPE";
            write_log_from_signal_handler(-1, STDLOG, message, sizeof(message) - 1);

            break;
        }
        case SIGTERM: {
            static const char message[] = "Received SIGTERM";
            write_log_from_signal_handler(-1, STDLOG, message, sizeof(message) - 1);
            gprofPrepare();

            break;
        }
        case SIGXFSZ: { // File size limit exceeded POSIX.1-2001
            static const char message[] = "Received SIGXFSZ - exiting";
            write_log_from_signal_handler(-1, STDLOG, message, sizeof(message) - 1);

            break;
        }
        default:
            msg_size = snprintf(msg_buf, BUF_SIZE, "Killed in action :-( by %d", signo);
            write_log_from_signal_handler(-1, STDLOG, msg_buf, (0 < msg_size) ? msg_size : 0);
            {
                sigset_t se;
                sigemptyset(&se);
                sigaddset(&se,SIGABRT);
                sigaddset(&se,signo);
                our_signal(signo,SIG_DFL,se); 
                our_signal(SIGABRT,SIG_DFL,se); 
                if(setupCore() and not waschdir)
                {
                    waschdir = 1;
                    char buf[128];
                    snprintf(buf,sizeof(buf),"Cores/%d",getpid());
                    mkdir(buf,0755);
                    if(chdir(buf) < 0) {
                        waschdir = 0;
                    }
                }
                raise(SIGABRT);
// Example with raise from inside signal handler was taken from
// http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_21.html#SEC353
// section "Handlers That Terminate the Process"
// Another way to get proper core dump is to call
//              kill(getpid(), signo);
//
// Using abort() in your signal handler will raise signal SIGABRT.
// Yes, you get a core file…… of your signal handler, not the process that actually crashed.
//
// If there are several threads, you must obtain proper thread id.
// For Linux you can use syscall.
// #include <linux/unistd.h>
// #include <sys/syscall.h>
//              pid_t gettid( void ) {
//                return syscall( __NR_gettid );
//              }
//              kill(gettid(), signo);
// Discussion with examples can be found here:
// http://www.alexonlinux.com/how-to-handle-sigsegv-but-also-generate-core-dump
                return;
            }
    }

    switch (signo) {
        case -1:
        case SIGINT:
        case SIGPIPE:
        case SIGTERM:
        case SIGXFSZ:
            break;
        default: 
            monitor_restart();
    }
    _exit(1); // man 2 _exit
}

void under_gdb()
{
    std::string var=std::string("SLOW_START")+"_"+tclmonCurrentProcessName();
    char const *s= getenv(var.c_str());
    if (s) {
        fprintf(stderr, "%s sleep\ngdb obrzap %d\n",tclmonCurrentProcessName(),getpid());
        sleep(atoi(s));
    }
}

void sigusr2(int sig)
{
    startstop_profiling();
}

struct SigDesc
{
    SigDesc(int s_, const char* n_)
        : s(s_), name(n_)
    {}
    int s;
    const char *name;
};

extern "C" void set_signal(void(*f) (int))
{
    std::vector<SigDesc> tab;
#define regsignal(x) tab.push_back(SigDesc(x,#x));
    regsignal(SIGINT)
    regsignal(SIGQUIT)
    regsignal(SIGILL)
    regsignal(SIGTRAP)
    regsignal(SIGBUS)
    regsignal(SIGFPE)
    regsignal(SIGSEGV)
    regsignal(SIGPIPE)
    regsignal(SIGTERM)
    regsignal(SIGVTALRM)
    regsignal(SIGPWR)
    regsignal(SIGXFSZ)
    regsignal(SIGUSR2)

    if (getenv("SKIP_SIGABRT") == NULL) {
        regsignal(SIGABRT)
        regsignal(SIGIOT)
    }
#undef regsignal


    sigset_t sigset;
    if(sigemptyset(&sigset)<0){
        throw comtech::Exception("sigemptyset");
    }
    if(sigprocmask(SIG_SETMASK,&sigset,NULL)<0){
        throw comtech::Exception("sigprocmask");
    }


    for (size_t i = 0; i < tab.size(); i++) {
        if (sigaddset(&sigset, tab[i].s) < 0){
            ProgError(STDLOG,"sigaddset failed on %s\n",tab[i].name);
        }
    }
    for (size_t i = 0; i < tab.size(); i++) {
        if (our_signal(tab[i].s, f, sigset)<0){
            ProgError(STDLOG,"our_signal failed on %s\n",tab[i].name);
            throw comtech::Exception("our_signal");
        }
    }

    our_signal(SIGUSR2, sigusr2, sigset);
}

void check_risc_order(void)
{
#ifdef RISC_ORDER
    short a=256;
#else
    short a=1;
#endif
if(*((char *)&a)!=1){
    fputs("wrong byte order - check -D RISC_ORDER\n",stderr);
    exit(1);
}
}    
void connect2DB2(void)
{
    ServerFramework::applicationCallbacks()
        ->connect_db(); 
}
extern "C" int connect2DB(void)
{
    try {
        ServerFramework::applicationCallbacks()
            ->connect_db(); 
    }catch (OciCpp::ociexception &e){
         ProgError(STDLOG,"%s",e.what()); 
         return -1;
    }
    return 0;
}

// TODO move abort inside the only one function connect2DB 
void testInitDB()
{
    if (connect2DB()) {
        fprintf(stderr, "connect2DB failed");
        Abort(5);
    }
    OciCpp::CursCtl c=make_curs_no_cache("SELECT KEK FROM XP_TESTING");
    c.noThrowError(942);
    c.exec();
    if(c.err()==942){
        fprintf(stderr, "\nXP_TESTING doesn't exist, it's probably not a developer box\n\n"
                        "       have you ever run buildFromScratch?\n\n");
        Abort(5);
    }
    // specially for XP_TESTING - sets SP_XP_TESTING
    OciCpp::CursCtl("SAVEPOINT SP_XP_TESTING").exec();
}

void initTest()
{
    testInitDB();
    ServerFramework::applicationCallbacks()->init();
}

void testShutDBConnection()
{
    ServerFramework::applicationCallbacks()->rollback_db();
    if(memcache::callbacksInitialized()) {
        memcache::callbacks()->flushAll();
    }
}

#ifdef ENABLE_PG
static const char* getPgConnectString()
{
    static std::string connStr = readStringFromTcl("PG_CONNECT_STRING");
    return connStr.c_str();
}

static PgCpp::SessionDescriptor sd = 0;

PgCpp::details::SessionDescription *getPgSessionDescriptor()
{
    return sd;
}

void setupPgManagedSession()
{
    sd = PgCpp::getManagedSession(getPgConnectString());
}

void setupPgReadOnlySession()
{
    sd = PgCpp::getReadOnlySession(getPgConnectString());
}



#endif // ENABLE_PG

ourtime OurTime;
