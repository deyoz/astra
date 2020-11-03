#if HAVE_CONFIG_H
#endif


#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <tcl.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include "lwriter.h"
#include "object.h"
#include "log_queue.h"
#include "msg_queue.h"
#include "tclmon.h"
#include "monitor.h" //for monitorControl
#include "msg_framework.h"
#include "mes.h"
#include "exception.h"

#define NICKNAME "SYSTEM"
#include "test.h"

using std::ofstream;
using std::endl;
using std::ios_base;

enum STATE {
    STATE_FREE=0,
    STATE_CONTROL,
    STATE_ACCEPT,
    STATE_SIGNAL,
    STATE_LOG,
    STATE_TOBEFREED,
    STATE_TOBEFREED2
};
struct CL_Table {
    enum STATE st; 
    pLogQueue plq;
    int too_many;
    int pid;
    _MesBl_ *p_mbl;
    handle_control_msg_t f_handle_control_msg;
};
static ofstream Logfp;
ofstream & getLoggerFp()
{
    return Logfp;
}
static void writeErrnoToLog(char const *s, int err)
{
    getLoggerFp() << s << " " << strerror(err) << "\n";
    getLoggerFp().flush();
}
    
static char filename[100];
#define MAXTABLE 320
struct CL_Table SockT[MAXTABLE];
struct pollfd PollT[MAXTABLE];
static int lastfree;

typedef struct _DiapItem_
{
  int count_beg;
  int count_end;
  int fail;
  int progerr;
}DiapItem;

static short debug_flag;
int debugLogger(void )
{
    return debug_flag;
}

int init_logger_tab(int control,int accept,int sig, handle_control_msg_t f_handle_control_msg)
{
    memset(SockT,0,sizeof SockT);
    memset(PollT,0,sizeof PollT);
    if(control>=0){
        SockT[lastfree].p_mbl = new_mbl();
        if(SockT[lastfree].p_mbl == NULL)
        {
           return -1;
        }
        SockT[lastfree].st=STATE_CONTROL;
        SockT[lastfree].f_handle_control_msg = f_handle_control_msg;
        PollT[lastfree].fd=control;
        PollT[lastfree].events=POLLIN;
        ++lastfree;
    }
    if(accept>=0){
        SockT[lastfree].st=STATE_ACCEPT;
        PollT[lastfree].fd=accept;
        PollT[lastfree].events=POLLIN;
        ++lastfree;
    }
    if(sig>=0){
        SockT[lastfree].st=STATE_SIGNAL;
        PollT[lastfree].fd=sig;
        PollT[lastfree].events=POLLIN;
        ++lastfree;
    }
return 0;
}


static int init_sock(char *path,int type )
{
    struct sockaddr_un saddr;
    int ret;
    saddr.sun_family=AF_UNIX;
    unlink(path);
    strcpy(saddr.sun_path,path);
    ret=socket(AF_UNIX,type,0);
    if(ret<0){
        writeErrnoToLog("logger:socket failed:",errno);
        ProgError(STDLOG,"socket failed: %s",strerror(errno));
        return -1;
    }
    if(bind(ret,(struct sockaddr*)&saddr,sizeof saddr)){
        writeErrnoToLog("logger:bind failed: ",errno);
        ProgError(STDLOG,"bind failed: %s",strerror(errno));
        return -1;
    }
    if(type==SOCK_STREAM){
        if (listen(ret, 128) < 0) {
            writeErrnoToLog("logger:listen failed: ",errno);
            ProgError(STDLOG,"listen failed: %s",strerror(errno));
            close(ret);
            return -1;
        }
    }
    return ret;
}


static int finish=0;
void set_finish_on_sig(int sig)
{
    if(finish==0)
        finish=1;
}
static void free_socks()
{
    int i,j=-1,del=0;
    int count_act=0;
    for(i=0;i<lastfree;i++){
        switch(SockT[i].st){
            case STATE_TOBEFREED2:
                shutdown(PollT[i].fd,2);
                if(SockT[i].p_mbl != NULL)
                {
                   free_mbl(SockT[i].p_mbl);
                }
                if(SockT[i].plq){
                    writeLogQueue(SockT[i].plq,getLoggerFp().rdbuf());
                    deleteItem((pItem)SockT[i].plq);
                }
            case STATE_TOBEFREED:    
                close(PollT[i].fd);
                SockT[i].st=STATE_FREE;
                SockT[i].plq=0;
                if(j==-1){
                    j=i;
                }

                del++;
                break;
            default:
                if(j!=-1){
                    SockT[j]=SockT[i];
                    PollT[j]=PollT[i];
                    j++;
                }
                if(finish==1){
                    if(SockT[i].st==STATE_LOG){
                        ++count_act;
                    }
                }
        }
    }
    if(finish==1 && count_act==0){
        finish=2;
    }
    lastfree-=del;
}


static int Loglevel=99;
static int Faillevel=99;
static int GetCurLogLev(void)
{
    return Loglevel;
}
static int GetCurFailLev(void)
{
    return Faillevel;
}


void writeToLog(time_t time_sec){
        time_t curt=time(0);
        time_t mintime=curt;
        int i;
        int minindex;
        pLogMsg pm;
        do{
            mintime=curt;
            minindex=-1;
            for(i=0;i<lastfree;i++){
                if(SockT[i].st==STATE_LOG ){
                     pm=topLogQueue(SockT[i].plq);
                     if(pm){
                        int t=getLogMsgTime(pm);
                        if(t!=-1 && t<mintime && t<curt-time_sec){
                            minindex=i;
                            mintime=t;
                        }
                     }
                }
            }
            if(minindex!=-1){
                listIterator I;
                DiapItem diap[1000];
                DiapItem *p_diap=NULL;
                short nz;    
                int count;
                int n_diap;
                int i_diap;
                int cur_diap;

                n_diap = 0;
                p_diap = &diap[0];
                p_diap->progerr = 0;
                p_diap->fail = 0;
                nz = -2;
                for( count=0,pm=initLogQueueIter(&I,SockT[minindex].plq) ;
                     pm &&  getLogMsgTime(pm)==mintime ;
                     pm=(pLogMsg)listIteratorNext(&I),count++){
                       if(nz!=pm->impl.msg_no )
                       {
                           if(p_diap->progerr != 0 ||
                              p_diap->fail != 0)
                           {
                              n_diap++;
                           }
                           if(n_diap >= 1000)
                           {
                              getLoggerFp()<< "logger:Too many diapazons count="<< count<<endl;
                              ProgError(STDLOG,"logger:Too many diapazons count=%d", count);
                              break;
                           }
                           nz = pm->impl.msg_no;
                           p_diap = &(diap[n_diap]);
                           p_diap->count_beg = count;
                           p_diap->fail = 0;
                           p_diap->progerr = 0;
                       }
                       p_diap->count_end = count;
                       if( !(p_diap->progerr) )
                          p_diap->progerr=getLogMsgProgErr( pm ) ? 1:0 ;
                       if( !(p_diap->fail) )
                          p_diap->fail=getLogMsgFail( pm ) ? 1:0 ;
                }
                if(p_diap != NULL && 
                   (p_diap->progerr>0 || p_diap->fail > 0) )
                {
                   n_diap++;
                }
                count=0;
                cur_diap = 0;
                while(1){
                    pm=topLogQueue(SockT[minindex].plq);
                    if(!pm || getLogMsgTime(pm)!=mintime)
                        break;
                    pm=getFromLogQueue(SockT[minindex].plq);
                    if(!pm){
                        getLoggerFp()<<"logger:getFromLogQueue NULL"<<endl;
                        ProgError(STDLOG,"getFromLogQueue NULL");
                        Abort(1);
                    }
                    if( getLogMsgLev(pm)<=GetCurLogLev() )
                    {
                       writeLogMsg(pm,getLoggerFp().rdbuf()); 
                    }
                    else
                    {
                       for(i_diap=cur_diap;i_diap<n_diap;i_diap++)
                       {
                          if(diap[i_diap].count_beg<=count &&
                             diap[i_diap].count_end>=count )
                          {
                             p_diap = &( diap[i_diap]);
                             cur_diap = i_diap;
                             if( p_diap->progerr>0 || 
                                 (p_diap->fail>0 
                                   &&  getLogMsgLev(pm) <= GetCurFailLev()) )
                             {
                                writeLogMsg(pm,getLoggerFp().rdbuf()); 
                             }
                             break;
                          }
                          if(count < diap[i_diap].count_beg)
                          {
                             break;
                          }
                       }
                    }
                    deleteItem((pItem)pm);
                    count++;
                }
            }
        }while(minindex>=0);
        for(i=0;i<lastfree;i++){
            if(SockT[i].st==STATE_LOG ){
                 int l =getLogQueueLen(SockT[i].plq);
                 if(l>5000 && SockT[i].too_many==0){
                      SockT[i].too_many=1;
                      getLoggerFp()<<
                                "logger:too many lines in memory "<<l<<
                                ", see below one sample line\n ---"<<endl;
                     writeLogMsg(topLogQueue(SockT[i].plq),getLoggerFp().rdbuf());
                     getLoggerFp().flush();                      
                 }
                 if(SockT[i].too_many==1 && l<2000 ){
                      SockT[i].too_many=0;
                 }
            }
        }
}

static void reopen_log(void){
    /*writeToLog(0);*/
    Logfp.close();
    Logfp.open(filename,ios_base::app | ios_base::out);
    if(!Logfp){
        ProgError(STDLOG,"failed to open %s", filename);
        Abort(1);
    }
}

extern "C" int logger_init(int supervisorSocket, int argc, char *argv[], handle_control_msg_t HandleControlMsg)
{
    ASSERT(2 < argc);

    Tcl_Interp* const interp = getTclInterpretator();
    Tcl_Obj* obj1 = Tcl_NewStringObj(argv[1], -1);
    Tcl_Obj* obj2 = Tcl_NewStringObj(argv[2], -1);
    Tcl_Obj* obj  = NULL;
    int sig_s;
    int accept_s;

    obj = Tcl_ObjGetVar2(interp, obj1, Tcl_NewStringObj("FILE",-1), TCL_GLOBAL_ONLY) ;
    if(!obj)
    {
        ProgError(STDLOG,"FILE: %s", Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }

    strcpy(filename, Tcl_GetString(obj));
    Logfp.open(filename, ios_base::app | ios_base::out);
    if(!Logfp)
    {
        ProgError(STDLOG,"failed to open %s", filename);
        return -1;
    }
    setLogReopen(reopen_log);
    obj = Tcl_ObjGetVar2(interp, obj1,Tcl_NewStringObj("SOCKET",-1),
                         TCL_GLOBAL_ONLY) ;
    if(!obj) {
        getLoggerFp()<<"logger:SOCKET: "<<
                Tcl_GetString(Tcl_GetObjResult(interp)) <<endl;
        ProgError(STDLOG,"SOCKET: %s",
               Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }
    if((accept_s = init_sock(Tcl_GetString(obj),SOCK_STREAM))<0){ /*log socket */
        Abort(1);
    }
    obj=Tcl_ObjGetVar2(interp, obj1,Tcl_NewStringObj("DEBUG",-1),
                       TCL_GLOBAL_ONLY);
    if(obj) {
        getLoggerFp()<<"logger: DEBUG "<< Tcl_GetString(obj)<<endl;
        if(strstr("#yes#Yes#YES#ON#on#On#true#True#TRUE#1#",
                  Tcl_GetString(obj))){
            debug_flag=1;
        }
    }
    getLoggerFp()<<"logger: DEBUG "<< debug_flag<<endl;
    obj = Tcl_ObjGetVar2(interp, obj1,Tcl_NewStringObj("LEVEL",-1),
                       TCL_GLOBAL_ONLY) ;
    if(!obj) {
        getLoggerFp()<<"logger:LEVEL: "<<
                Tcl_GetString(Tcl_GetObjResult(interp))<<endl;
        ProgError(STDLOG,"LEVEL: %s",
               Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }
    if(TCL_OK!=Tcl_GetIntFromObj(interp, obj,&Loglevel)){
        getLoggerFp()<<"logger:LEVEL: "<<
                Tcl_GetString(Tcl_GetObjResult(interp))<<endl;
        ProgError(STDLOG,"LEVEL: %s",
               Tcl_GetString(Tcl_GetObjResult(interp)));
        return 1;
    }

    obj=Tcl_ObjGetVar2(interp, obj1,Tcl_NewStringObj("FAILLEVEL",-1),
                       TCL_GLOBAL_ONLY) ;
    if(obj){
        if(TCL_OK!=Tcl_GetIntFromObj(interp, obj,&Faillevel)){
            getLoggerFp()<<"logger:LEVEL: "<<
                    Tcl_GetString(Tcl_GetObjResult(interp))<<endl;
            ProgError(STDLOG,"LEVEL: %s",
                   Tcl_GetString(Tcl_GetObjResult(interp)));
            return 1;
        }
    }

    if((sig_s=init_sock(Tcl_GetString(obj2),SOCK_DGRAM))<0){ /*signal socket */
        return 1;
    }
    if(init_logger_tab(supervisorSocket, accept_s,  sig_s, HandleControlMsg) != 0)
    {
        return 1;
    }

    return 0;
}
/*
static int recv_all(int fd, char *buf,int n, int ready)
{
  int nread,nread_total=0;
  int pollret;
    struct pollfd pf;

    pf.fd=fd;
    pf.events=POLLIN;
    pf.revents=0;

  while(1){
        if(!ready){
             pollret=poll(&pf,1,15000);
             if(pollret!=1){
                 if(pollret==0 ){
                     ProgError(STDLOG,"poll timeout in recv_all");
                     return -1;
                 }
                 if(pollret<0 ){
                     ProgError(STDLOG,"poll:%s",strerror(errno));
                     return -1;
                 }
             }
             if( (pf.revents&POLLIN)){
                 if( (pf.revents&POLLERR)){
                     ProgError(STDLOG,"POLLERR happened");
                     return -1;
                 }
                 if( (pf.revents&POLLHUP)){
                     ProgError(STDLOG,"POLLHUP happened");
                     return -1;
                 }
                 ProgError(STDLOG,"pf.revents %d",pf.revents);
                 return -1; 
             }
        }
        ready=0;
    nread=read(fd,buf+nread_total,n-nread_total);
    if(nread==0){
      return 0;
        }

    if(nread<0){
            ProgError(STDLOG,"read failed %s",strerror(errno));
      return -1;
    }
    nread_total+=nread;
    if(nread_total==n)
      break;
  }
  return nread_total;
}
*/
void logger_main_loop(void)
{
    int i;
    if(set_sig(set_finish_on_sig,SIGINT)<0 or set_sig(set_finish_on_sig,SIGTERM)<0)
        Abort(1);
    set_sig(regLogReopen,SIGUSR1);
    for(;finish!=2;){
        int will_free=0;
        int new_sock=0;
        int n;
        if(finish == 0)
        {
           write_set_flag(-1, 0);
        }
        n=poll(PollT,lastfree,5000);
        if(n<0){
            if (errno == EINTR)
                continue;
            ProgError(STDLOG, "poll:%s", strerror(errno));
            writeErrnoToLog("logger:poll:", errno);
            Abort(1);
        }
        reopenLog();
        if(n==0){
            writeToLog(0);
            getLoggerFp().flush();
            continue;
        }
        for(i=0;i<lastfree;i++){
            if(PollT[i].revents)
                n--;
            if(PollT[i].revents & POLLERR){
                will_free=1;
                getLoggerFp()<<"logger:POLLERR: socktype="<<SockT[i].st
                    <<", error <" << strerror(errno)<< ">"<<endl;
                ProgError(STDLOG,"POLLERR: socktype=%d, error <%s>",
                        SockT[i].st,strerror(errno));
                if(SockT[i].st==STATE_SIGNAL ||
                        SockT[i].st==STATE_ACCEPT ||
                        SockT[i].st==STATE_CONTROL){
                    goto out;
                }
                switch (SockT[i].st){
                    case STATE_LOG:
                        SockT[i].st=STATE_TOBEFREED2;
                        break;
                    default:
                        SockT[i].st=STATE_TOBEFREED;
                        break;
                }
            }else if(PollT[i].revents & POLLIN){
                switch(SockT[i].st){
                    case STATE_LOG:
                        /*read*/
                        {
                            const int LOGBSIZE=20480;
                            char buf[LOGBSIZE+1] = {0};
                            char *pbuf = 0;
                            int n = 0;
                            n=read(PollT[i].fd,buf,LOGBSIZE);
                            if(n<=0){
                                if(n<0){
                                    writeErrnoToLog("logger:read failed ",errno);            
                                    ProgError(STDLOG,"read failed %s",
                                            strerror(errno));            
                                }
                                SockT[i].st=STATE_TOBEFREED2;
                                will_free=1;
                                goto end_cycle;
                            }
                            pbuf=buf;
                            {
                                pLogMsg pm;
                                char *start=pbuf,
                                     *end=pbuf+n;
                                while(start && start!=end){
                                    char *hint; 
                                    int rc;
                                    hint=(char*)memchr(start,'\n',end-start);
                                    if(hint)
                                        hint++;
                                    rc=addOrAppendLQ(SockT[i].plq,start,
                                            (hint?hint:end)-start);
                                    if(rc<0){
                                        ProgError(STDLOG,"close bad connection");
                                        getLoggerFp()<<
                                            "logger: close bad connection"<<endl;
                                        SockT[i].st=STATE_TOBEFREED2;
                                        will_free=1;
                                        goto end_cycle;
                                    }
                                    pm = tailLogQueue(SockT[i].plq);
                                    start=hint;
                                }
                            }
                        }
                        break;
                    case STATE_SIGNAL:    
                        break;
                    case STATE_CONTROL:    
                        {
                         int r;
                         r = recv_mbl(PollT[i].fd, SockT[i].p_mbl);
                         if(r <= 0)
                         {
                            if(finish==0)
                              finish=1;
                              SockT[i].st=STATE_TOBEFREED;
                              will_free=1;
                         }
                         else if(r > 0)
                         {
                           mes_for_process_from_monitor(SockT[i].p_mbl, SockT[i].f_handle_control_msg);
                           reset_mbl(SockT[i].p_mbl);
                         }
                        }
                        break;
                    case STATE_ACCEPT:  
                        {
                             int s=accept(PollT[i].fd,0,0);
                             if(s<0){
                                    writeErrnoToLog("logger:accept: ",errno);
                                    ProgError(STDLOG,"accept: %s",
                                            strerror(errno));
                                    continue;                
                             }
                             if(lastfree+new_sock>=MAXTABLE){
                                 shutdown(s,2);
                                 close(s);
                                 getLoggerFp()<<
                                         "logger:too many connections"<<endl;
                                 ProgError(STDLOG,"too many connections");
                                 continue;
                             }
                             SockT[lastfree+new_sock].st=STATE_LOG ;
                             PollT[lastfree+new_sock].events=POLLIN;
                             PollT[lastfree+new_sock].fd=s;
                             SockT[lastfree+new_sock].plq=newLogQueue();
                             new_sock++;
                        }
                        break;
                    case STATE_TOBEFREED2:
                    case STATE_TOBEFREED:
                    case STATE_FREE:
                        getLoggerFp()<<"logger:wrong socket POLLIN\n";
                        ProgError(STDLOG,"wrong socket POLLIN");
                        goto out;
                        break;
                }
            }
            if(PollT[i].revents&POLLHUP){
                will_free=1;
                switch (SockT[i].st){
                    case STATE_LOG:
                        SockT[i].st=STATE_TOBEFREED2;
                        break;
                    case STATE_CONTROL:    
                        {
                            int n,b;
                            n=read(PollT[i].fd,&b,1);
                            if(n==0){
                                if(finish==0)
                                  finish=1;
                                  SockT[i].st=STATE_TOBEFREED;
                            }
                        }
                        break;
                    default:
                        SockT[i].st=STATE_TOBEFREED;
                        break;
                }
            }
        }
end_cycle:        
        writeToLog(5);
        lastfree+=new_sock;
        if(will_free){
            free_socks();
        }
    }
    out:
    for(i=0;i<lastfree;i++){
        switch(SockT[i].st){
            case STATE_LOG:
                SockT[i].st=STATE_TOBEFREED2;
                break;
            case STATE_CONTROL:
            case STATE_SIGNAL:
                SockT[i].st=STATE_TOBEFREED;
                break;
            case STATE_ACCEPT:
                close(PollT[i].fd);
                break;
            default:
                break;
        }
    }
    free_socks();
}

int connect_logger(const char* f)
{
    static struct sockaddr_un addr;
    int s;
    s = socket(addr.sun_family = AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        ProgError(STDLOG, "socket failed:%s", strerror(errno));
        return -1;
    }
    if (strlen(f) >= sizeof addr.sun_path) {
        ProgError(STDLOG, "too long file name %.120s", f);
        return -1;
    }
    strcpy(addr.sun_path, f);
    if (connect(s, (struct sockaddr*)&addr, sizeof addr) < 0) {
        ProgError(STDLOG, "cannot connect to logger process on <%s>:\n%s",
                f, strerror(errno));
        close(s);
        return -1;
    }
    return s;
}
int reconnect_logger(const char* f, int count)
{
    const int sleep_timeout = 2;
    int j = 0, s = -1;
    for (; j < count; j++) {
        s = connect_logger(f);
        if (s >= 0)
            break;
        sleep(sleep_timeout);
    }
    if (s < 0) {
        ProgError(STDLOG, "reconnect_logger: timeout %d sec exceeded", sleep_timeout * count);
        Abort(1);
    }
    return s;
}

#define CYCLES 5
int check_logger(int s,const char *f)
{
    struct pollfd P;
    static int saved_fd=-1000;
    int n;
    if(saved_fd==-1000){
        saved_fd=s;
    }
    if(s!=saved_fd){
        ProgError(STDLOG,"logger descriptor changed from %d to %d",saved_fd,s);
    }
    P.fd=s;
    P.events=POLLOUT;
    while(1){ 
/*        time_t t1=time(0),t2;*/
        n=poll(&P,1,500);
/*        t2=time(0);
        if(t2-t1>1){
            syslog(FL_LOG_ERR,"poll lasts %d",t2-t1);
        }*/
        if(n==0){
            ProgError(STDLOG,"logger timeout");
            break;
        }else if(n<0){
            ProgError(STDLOG,"poll failed:%s",strerror(errno));
            continue;
        }
        int need_reconnect = 0;
        if (P.revents&POLLHUP) {
            ProgError(STDLOG, "POLLHUP on logger");
            need_reconnect = 1;
        }
        if (P.revents&POLLERR) {
            ProgError(STDLOG, "POLLERR on logger");
            need_reconnect = 1;
        }
        if ((P.revents&POLLOUT) == 0) {
            ProgTrace(TRACE0, "no POLLOUT on logger");
            need_reconnect = 1;
        }
        if (need_reconnect) {
            shutdown(s, 2);
            close(s);
            s = P.fd = reconnect_logger(f, 5);
        }
        break;
    }
    
    if(s<0){
        ProgError(STDLOG,"failed to reconnect to logger process after failure");
        Abort(5);
    }
    return s;
}

