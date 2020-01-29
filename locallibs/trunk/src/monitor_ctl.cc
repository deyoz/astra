#if HAVE_CONFIG_H
#endif

#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <tcl.h>

#include "monitor_ctl.h"
#include "tclmon.h"
#include "tcl_utils.h"
#include "mes.h"

#define NICKNAME "KSE"
#include "test.h"

namespace ServerFramework {
    void execTclCommandStr(const char* cmdStr, size_t cmdLen);
}

static int timeout=0;
int tcl_slave_mode()
{
    static int mode=-1;
    if(mode==-1){
        Tcl_Obj *obj;
        obj=Tcl_GetVar2Ex(getTclInterpretator(),
                "SLAVEMODE",0,TCL_GLOBAL_ONLY);
        mode=0;
        if(obj){
            if(strcmp("1",Tcl_GetString(obj))==0){
                mode=1;
            }
        }
        fprintf(stderr,"tcl_slave_mode=%d\n",mode);
    }
    return mode;
}
    
    
static int working;
void monitor_beg_work()
{
    static int flag = -1;
    if (!getVariableStaticBool("UNDER_GDB", &flag, 0))
        write_set_flag(timeout,0); 
    working = 1;
    return;
}

void monitor_working()
{
    write_set_flag(-1, 0);
    return;
}

void monitor_working_zapr_type(int cnt, int type_zapr)
{
    write_set_flag_type(-1, cnt, type_zapr, nullptr);
    return;
}

void monitor_idle_zapr_type(int cnt, int type_zapr)
{
    write_set_flag_type(0, cnt, type_zapr, nullptr);
    working = 0;
    return;
}

void monitor_idle_zapr(int cnt)
{
  monitor_idle_zapr_type(cnt, QUEPOT_ZAPR);
}


void monitor_idle()
{
    monitor_idle_zapr_type(0, QUEPOT_ZAPR);
}

/* установить код запроса, который сейчас обрабатывается процессом*/
void monitor_set_request(char *req)
{
  write_set_cur_req(req);
}

/* очистить код запроса, который сейчас обрабатывается процессом */
/* (очистка производится автоматически при вызове monitor_idle() ) */
void monitor_clear_request(void)
{
  write_clear_cur_req();
}



void monitor_restart()
{
    signal(SIGABRT, SIG_DFL);
    abort();
}

void monitor_regular()
{
    timeout = 40;
    return;
}

void monitor_archive()
{
    timeout = 600;
    return;
}

void monitor_special()
{
    timeout = 180;
    return;
}

void monitor_strange_restart()
{
}

int monitor_need_print_request(int base)
{        
    static int cnt;
    return (++cnt % base) == 0;
}

void set_monitor_timeout(int time)
{
 timeout=time;
}

int get_monitor_timeout()
{
 return timeout;
}


static Tcl_Obj * prepare_addrlist(const char * name, const char * num )
{
        int Num,i;
        Tcl_Obj *obj,*plist;
        obj=Tcl_GetVar2Ex(getTclInterpretator(), num, 0, TCL_GLOBAL_ONLY);
        if(obj!=NULL && TCL_OK!=Tcl_GetIntFromObj(getTclInterpretator(),obj,
                    &Num)){
                ProgError(STDLOG,"invalid %s in tcl config: %s,%s",num,
                Tcl_GetString(obj),
                        Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                return 0;
        }    

        obj = Tcl_GetVar2Ex(getTclInterpretator(), name, 0, TCL_GLOBAL_ONLY);
        if(!obj){
               ProgError(STDLOG,"%s in tcl config: %s",name,
                     Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                return 0;
        }
        plist=Tcl_NewListObj(0,0);
        if(!plist){
               ProgError(STDLOG,"Tcl_NewListObj: %s",
                     Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                return 0;
        }
        for(i=0;i<Num;i++){
            struct sockaddr_un addr;
            addr.sun_family=AF_UNIX;
            sprintf(addr.sun_path,"%s%03d",Tcl_GetString(obj),i);
            if(TCL_OK!=Tcl_ListObjAppendElement(getTclInterpretator(),plist,
                  Tcl_NewByteArrayObj((unsigned char*)&addr,sizeof addr))){
                        
                ProgError(STDLOG,"Tcl_ListObjAppendElement: %s",
                      Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                    return 0;
            }
        }
        Tcl_IncrRefCount(plist);
        return plist;
}
static int send_from_addrlist(Tcl_Obj *plist,const void * data, int len){
        int Num,i;
        if(TCL_OK!=Tcl_ListObjLength(getTclInterpretator(),plist,&Num)){
              ProgError(STDLOG,"Tcl_ListObjLength: %s",
              Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
        
        }
        for(i=0;i<Num;i++){
            struct sockaddr_un addr;
            unsigned char *p;
            int l;
            Tcl_Obj *obj;
            if(TCL_OK!=Tcl_ListObjIndex(getTclInterpretator(),
                        plist,i,&obj)){
                  ProgError(STDLOG,"Tcl_ListObjIndex: %s",
                      Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                  return -1;
            }
            p=Tcl_GetByteArrayFromObj(obj,
                    &l);
            if(!p){
                ProgError(STDLOG,"Tcl_ListObjIndex: %s",
                    Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                return -1;
            }
            if(l!=sizeof addr){
                ProgError(STDLOG,"wrong data stored in ByteArray len=%d",l);
                return -1;
            }
            memcpy(&addr,p,sizeof addr);
	    ProgTrace(TRACE5,"send signal msg to %s",addr.sun_path);
            send_signal_udp(&addr,0,0, data,len);
        }
        return 0;
}
static void send_many_addr(Tcl_Obj **plist,const char *name,
            const char *num,void const *data,int len  )
{
		ProgTrace(TRACE1,"%s - %s",name,num);
        if(!*plist){ 
            *plist=prepare_addrlist(name,num);
            if(!*plist){
                   ProgError(STDLOG,"prepare_addrlist failed");
                   return;
            }
        }
        if(send_from_addrlist(*plist,data,len)<0){
             ProgError(STDLOG,"send_from_addrlist failed, %s %s",name,num);
        }
}

void notifyArchDaemons(void)
{
     static Tcl_Obj *plist;
     send_many_addr(&plist, "ARCH_CMD", "ARCH_NUM", " ", 1);
     return;
}

void notifyArchFtp(void)
{
    static struct sockaddr_un addr;
    
    send_signal_udp(&addr, "CMD_ARCH_FTP", 0, " ", 1);
    return;
}

bool setupCore()
{
    volatile static sig_atomic_t calculated = 0;
    if(not calculated)
    {
        struct rlimit rl = { .rlim_cur=0, .rlim_max=RLIM_INFINITY };
        if(getrlimit(RLIMIT_CORE,&rl) < 0)
        {
            fprintf(stderr, "%s :: getrlimit failed: %.200s\n", __func__, strerror(errno));
            return false;
        }
        if(rl.rlim_max == 0)
        {
            //fprintf(stderr, "%s :: won't make cores (rlim_max is 0)\n", __func__);
            calculated = 2;
        }
        else if(rl.rlim_cur < rl.rlim_max)
        {
            rl.rlim_cur = rl.rlim_max;
            if(setrlimit(RLIMIT_CORE,&rl) < 0)
            {
                fprintf(stderr, "%s :: setrlimit failed: %.200s\n", __func__, strerror(errno));
                return false;
            }
            calculated = 1;
        }
    }
    return calculated < 2;
}

std::string get_signalsock_name(Tcl_Interp *interp,Tcl_Obj *var1,Tcl_Obj *var2,int suff)
{
    Tcl_Obj *obj;

    if(0==(obj=Tcl_ObjGetVar2(interp,var1,var2,
                TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG)))
    {
        ProgError(STDLOG,"%s %s %s",Tcl_GetString(var1), var2 ? Tcl_GetString(var2):"(null)",
                                    Tcl_GetString(Tcl_GetObjResult(interp)));
                                    Tcl_ResetResult(interp);
        return std::string()  ;
    }
    std::string result = Tcl_GetString( obj ) ;
    if(suff>=0)
    {
        char tmp[18];
        sprintf(tmp,"%03d",suff);
        result += tmp ;
    }
    return result ;
}

/*makeSignalSocket используется для создания сокета AF_UNIX
 * с именем из переменной конфигурационного файла var1(var2)
 * или var1 если var2==0
 * сокет используется для подачи передачи управляющей информации типа
 * сигнала ( в простейшем случае один байт - пробел
 *  int sigdesc=makeSignalSocket(getTclInterpretator(),
 *      Tcl_NewStringObj("THIS_DAEMON_SIGSOCK",1),0);
 *      if(sigdesc <0 ){
 *          error
 *      }
 *
 * */

static int signal_sock=-1;
int makeSignalSocket( const std::string & name )
{
    int s = 0;
    struct sockaddr_un addr;
    memset(&addr,0,sizeof addr);
    signal_sock=s=socket(addr.sun_family=AF_UNIX,SOCK_DGRAM,0);
    if(s<0)
    {
        ProgError(STDLOG,"socket %s",strerror(errno));
        return -1;
    }
    name.copy( addr.sun_path , sizeof( addr.sun_path ) - 1 );
    addr.sun_path[ name.length() ] = 0 ;
    unlink(addr.sun_path);
    if(bind(s,(struct sockaddr *)&addr,sizeof addr)<0)
    {
        ProgError(STDLOG,"bind %s",strerror(errno));
        //ProgError(STDLOG,"bind %s",strerror(errno));
        close(s);
        return -1;
    }
    return s;
}

/*
 *
 *  используется вместо sleep
 *  совместно с makeSignalSocket
 *  позволяет кроме ожидания сигнала обрабатывть события на
 *  контрольном сокете - (используется библиотекой обработки сообщений
 *  может быть модифицирована для обработки других событий
 *  аргумент first позволяет управлять поведением при первом вызове
 *
 *  int first=1;
 *  while(1){
 *      tcl_mode_sleep(sigdesc,-1,&
 *
 * */

void tcl_mode_sleep2(int sigdesc,int s2,int *first,int mseconds,
        void(*pf)(int ,char *),int addsock,int *result)
{
    struct pollfd pfd[3];
    int nd;
    int nd2;
    char buf[2048];
    int len;
    clock_t tm1,tm2;
    struct tms tms1,tms2;
    if(first && *first){
        *first=0;
        return;
    }
    if(sigdesc<0){
        sigdesc=signal_sock;
    }
    if(s2<0 ){
        s2=getControlPipe();
    }

    pfd[0].events=POLLIN;
    pfd[1].events=POLLIN;
    pfd[2].events=POLLIN;
    pfd[0].fd=sigdesc;
    pfd[1].fd=s2;
    pfd[2].fd=addsock;
    tm1=times(&tms1);
    nd=poll(pfd, addsock >=0 ? 3:2,mseconds);
    int saved_errno=errno;
    nd2=nd;
    tm2=times(&tms2);
#ifndef MSec
#define  MSec(a) (((a)*1000)/sysconf(_SC_CLK_TCK))
#endif
    ProgTrace(TRACE2,"poll waited %li ms",MSec(tm2-tm1));

    while(1){
        ProgTrace(TRACE2,"poll - nd=%d,\n"
                "revents0=0x%X,revents1=0x%X,revents2=0x%X",
                nd,pfd[0].revents,pfd[1].revents,pfd[2].revents);
        if(nd<0){
            if(saved_errno!=EINTR){
                  ProgError(STDLOG,"poll failed %s",strerror(saved_errno));
                  sleep(1);
            }else{
                  ProgTrace(TRACE0,"%s",strerror(saved_errno));
            }
            if(addsock>=0){
                *result=-1;
            }
        }
        if(nd>0 && pfd[0].revents){
            //read msg and process
            len=recvfrom(pfd[0].fd,buf,sizeof buf,0,0,0);
            saved_errno=errno;
            ProgTrace(TRACE2,"signal received len=%d",len);
            if(len<0){
                ProgTrace(TRACE0,"recvfrom failed=%s",strerror(saved_errno));
                sleep(1);
            }else{
                if(pf){
                    pf(len,buf);
                }
            }
            nd--;
        }
        if(s2>=0 && nd>0 && pfd[1].revents){
            //read msg and process
            if(pfd[1].revents & POLLIN){
                enum State {WaitMesHead = 0, WaitMesText};
                static _Message_ msg;
                static State state;
                static int len;

                switch (state) {
                case WaitMesHead: {
                    len += read(pfd[1].fd, reinterpret_cast<uint8_t*>(&msg.mes_head) + len, sizeof(msg.mes_head) - len);
                    if ((0 < len) && (len == sizeof(msg.mes_head))) {
                        len = 0;
                        state = WaitMesText;
                    } else if (-1 == len) {
                        ProgTrace(TRACE0, "error: %s", strerror(errno));
                        Abort(1);
                    } else if (!len) {
                        ProgTrace(TRACE1, "Control pipe closed");
                        Abort(1);
                    }

                    break;
                }
                case WaitMesText: {
                    len += read(pfd[1].fd, msg.mes_text + len, msg.mes_head.len_text - len);
                    if ((0 < len) && (sizeof(msg.mes_head.len_text) == len)) {
                        ServerFramework::execTclCommandStr(msg.mes_text, msg.mes_head.len_text);
                        len = 0;
                        state = WaitMesHead;
                    } else if (-1 == len) {
                        ProgTrace(TRACE0, "error: %s", strerror(errno));
                        Abort(1);
                    } else if (!len) {
                        ProgTrace(TRACE1, "Control pipe closed");
                        Abort(1);
                    }

                    break;
                }
                default:
                    ProgTrace(TRACE1, "Unknown state");
                    Abort(1);
                }
            }
            if(pfd[1].revents & (POLLHUP|POLLERR)){
                ProgTrace(TRACE0,"control socket POLLHUP|POLLERR %d",
                        pfd[1].revents);
                Abort(1);
            }
            nd--;
        }
        if(addsock>=0 && nd>0 && pfd[2].revents){
            //read msg and process
            if(0==(pfd[2].revents & POLLIN)){
                ProgTrace(TRACE0,"additional socket error %d",
                        pfd[2].revents);
                *result=-2;
            }else{
                tst();
                *result=1;
            }
        }else if(addsock>=0){
            *result=0;
        }
        if(nd2>=0 && (addsock<0 || *result==0)){
            nd2=nd=poll(pfd,addsock>=0 ? 3:2,0);
            ProgTrace(TRACE2,"continue polling %d",nd);
            if(nd<=0){
               break;
            }
        }else{
            break;
        }
    }
}
void tcl_mode_sleep(int sigdesc,int s2,int *first,int seconds,
        void(*pf)(int ,char *))
{
    tcl_mode_sleep2(sigdesc,s2,first,seconds*1000,pf,-1,0);
}

long get_number_from_file(const char *file,long def, long err)
{
    FILE *fp;
    long t;
    fp=fopen(file,"r");
    if(!fp ){
        if(errno==ENOENT){
            return def;
        }
        fprintf(stderr,"cannot open %s %s\n",file,strerror(errno));
        return err;
    }
    if(fscanf(fp,"%ld",&t)!=1){
        fprintf(stderr,"cannot read number from %s \n",file);
        fclose(fp); 
        return err;
    }
    fclose(fp);
    return  t; 
}
int write_number_to_file(const char *file,long num)
{
    FILE *fp;
    char filetmp[512];
    if(!file)
        return 0;
    sprintf(filetmp,"%s.%d",file,getpid());
    fp=fopen(filetmp,"w");
    if(!fp){
            fprintf(stderr,"cannot open %s %s\n",filetmp,strerror(errno));
            return -1;
    }
    fprintf(fp,"%ld\n",(long)num);
    fclose(fp);
    if(rename(filetmp,file)<0){
            fprintf(stderr,"cannot rename to %s %s\n",file,strerror(errno));
            return -1;
    }
    return 0;
}

