#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include "object.h"
#include "tcl_utils.h"
#include "log_queue.h"
#include "cc_censor.h"

#include <sys/types.h>

#define NICKNAME "SYSTEM"
#include "test.h"

static pLogMsg mk_LogMsg ();
std::ofstream getLoggerFp();

struct LogMsgChunk {
    Item I;
    LogMsg ptr;
    char text[200];
};


static pList getChunkList(void)
{
    static pList pl;
    if(!pl)
        pl=mk_List();
    if(!pl){
        ProgError(STDLOG,"failed to create chunk list");
    }
    return pl;
}
static struct LogMsgChunk *getLogMsgChunk(void)
{
   static int cnt;
   static int cnt_tot;
   static void *fre;
   struct LogMsgChunk *pc=(struct LogMsgChunk *)
        detachFirstList(getChunkList());
   if(fre==0){
        fre=malloc(1000);
   }
   if(!pc){
       pc=(struct LogMsgChunk*)calloc(1,sizeof pc[0]);
       cnt++;
       if(cnt%1000==0){
        if(debugLogger())
            fprintf(stderr,"LogChunk count =%d,pid=%d\n",cnt,getpid());
       }
       if(!pc){
           free(fre);
           fre=0;
           ProgError(STDLOG,"cannot malloc LogMsgChunk %d",cnt);
       }
   }
   cnt_tot++;
   if(cnt_tot%10000==0){
       if(debugLogger())
        fprintf(stderr,"calls LogChunk count =%d,pid=%d\n",cnt_tot,getpid());
   }
   return pc;
}

static void freeLogMsgChunk(struct LogMsgChunk *p)
{
    static int cnt;
    if(p==0)
        abort();
    cnt++;
    if(cnt%10000==0){
        if(debugLogger())
        fprintf(stderr,"free LogChunk count =%d,pid=%d\n",cnt,getpid());
    }
    addToList(getChunkList(),(pItem)p);
}

static int get_time_and_lev(int len,char *data,time_t *t,
        short *lev,short *progerr, short *failflag, short *msg_no)
{
    *t=-1;
    *lev=-1;
    *progerr=0;
    *failflag=0;
    *msg_no=-1;

    if(len >0 && data[len-1]=='\n'){
        char *ad,*ad2,*ad3;
        char *err;
        size_t tlen = strspn(data,"0123456789.T");
        if(tlen!=12 && tlen!=22 && tlen!=15){
            // 20100120T150635 wo microsec
            // 20100120T150635.000157 with microsec
            // 001263992697 - standart unix time
            ProgError(STDLOG,"cannot get timestamp from %.22s",data);
            return -1;
        }
        if(tlen==12)
        {
          *t=strtol(data,&ad,10);
        }
        else
        {
          *t=time(0);
          ad = data + tlen;
        }
        if(data==ad){
            ProgError(STDLOG,"cannot get timestamp from %.15s",data);
            return -1;
        }
        if(*ad == '+')
        {
           ad3 = ad+1;
           *msg_no=strtol(ad3,&ad,10);
           if(ad==ad3){
               ProgError(STDLOG,"cannot get msg_no from %.15s",ad3);
               return -1;
           }
        }
        *lev=strtol(ad,&ad2,10);
        if(ad==ad2){
            ProgError(STDLOG,"cannot get loglev from %.15s",ad);
            return -1;
        }
        if(*lev==-2){
            *failflag=1;
        }
        if(*lev==-1){
            err=(char*)memchr(data,'>',len);
            if(err &&  (err <= data+(len-5)) &&  strncmp(err,">>>>>",5)==0){
                *progerr=1;
            }
        }
    }
    return 0;
}




pLogMsg newLogMsg (int len, char *data)
{
    short lev;
    time_t t;
    short progerr;
    short failflag;
    short msg_no;
    pLogMsg pp;
    static int cnt;
    if (data==0){
        abort();
    }
    if(get_time_and_lev(len,data,&t,&lev,&progerr,&failflag,&msg_no)<0)
        return 0;
    pp = mk_LogMsg ();
    if (pp) {
        pp->impl.lev=lev;
        pp->impl.progerr=progerr;
        pp->impl.msg_no=msg_no;
        pp->impl.failflag=failflag;
        pp->impl.t=t;
        pp->impl.len = len;
        if(t!=-1 && len <sizeof(pp->chunk->text)){
            pp->impl.ptr = pp->chunk->text;
        }else{
            pp->impl.ptr = (char *) malloc (len + 1);
            if (pp->impl.ptr == NULL) {
                ProgError(STDLOG,"cannot malloc %d bytes",len);
                freeLogMsgChunk (pp->chunk);
                return NULL;
            }
            cnt++;
            if(cnt%1000==0){
                if(debugLogger())
                fprintf(stderr,"malloc in LogChunk count =%d,pid=%d\n",cnt,getpid());
            }
        }
        memcpy (pp->impl.ptr, data, pp->impl.len);
        pp->impl.ptr[pp->impl.len] = 0;
    }
    return pp;
}


static int addToLogMsg (pLogMsg pp,int len, char *data)
{
    char *p;
    if(pp==0 || data==0){
        abort();
    }
    if(pp->impl.ptr==pp->chunk->text){
        p=(char*)malloc(pp->impl.len+len);
        if(p){
            memcpy(p,pp->impl.ptr,pp->impl.len);
        }
    }else{
        p=(char*)realloc(pp->impl.ptr,pp->impl.len+len);
    }
    if(!p){
        return -1;
    }
    memcpy(p+pp->impl.len,data,len);
    pp->impl.ptr=p;
    pp->impl.len+=len;
    if(get_time_and_lev(pp->impl.len,pp->impl.ptr,&pp->impl.t,&pp->impl.lev,
                &pp->impl.progerr,&pp->impl.failflag,&pp->impl.msg_no)<0){
        if(pp->impl.len>100){
            ProgError(STDLOG,"logger: bad data2  %.*s\n",
                    pp->impl.len,
                    pp->impl.ptr);
            getLoggerFp()<<"logger: bad data2  "
                << std::string( (char*)pp->impl.ptr, pp->impl.len)<<std::endl;
            return -2;
        }

        return -1;
    }

    return 0;
}

time_t getLogMsgTime(pLogMsg p)
{
    if(p==0)
        abort();
    return p->impl.t;
}

int getLogMsgLev(pLogMsg p)
{
    if(p==0)
        abort();
    return p->impl.lev;
}

int getLogMsgProgErr(pLogMsg p)
{
    if(!p)
        return 0;
    return p->impl.progerr;
}

int getLogMsgFail(pLogMsg p)
{
    if(!p)
        return 0;
    return p->impl.failflag;
}




static void dl_quemsg (pItem p)
{
    static int cnt;
    if(((pLogMsg) p)->impl.ptr!=((pLogMsg) p)->chunk->text){
        free (((pLogMsg) p)->impl.ptr);
        cnt++;
        if(cnt%1000==0){
            if(debugLogger())
            fprintf(stderr,"dl_queue LogChunk count =%d,pid=%d\n",cnt,getpid());
        }
    }
    ((pLogMsg) p)->impl.ptr = 0;
}

static void pr_quemsg (pItem p)
{
}



static pItem cl_quemsg (pItem p)
{
    if(p==0)
        abort();
    return (pItem) newLogMsg (((pLogMsg) p)->impl.len,
                              ((pLogMsg) p)->impl.ptr);
}

static void  msg_que_del_chunk(pLogMsg p)
{
    if(p==0)
        abort();
    freeLogMsgChunk(p->chunk);
}
static pvirt pvtLogMsg[pvtMaxSize] = {
    dl_quemsg,
    pr_quemsg,
    (pvirt) cl_quemsg,
    0,
    (pvirt)msg_que_del_chunk,
};

static pLogMsg mk_LogMsg ()
{
    struct LogMsgChunk *pc=getLogMsgChunk();
    pLogMsg ret;
    if(!pc)
        return 0;
    ret = &pc->ptr;
    ret->chunk=pc;
    ret->I.pvt = pvtLogMsg;
    return ret;
}

static void dl_LogQueue (pItem p)
{
    if(p==0)
        abort();
    deleteItem ((pItem) ((pLogQueue) p)->impl.plist);
    ((pLogQueue) p)->impl.plist = 0;
}

static void pr_LogQueue (pItem p)
{
}


static pItem cl_LogQueue (pItem p)
{
    pLogQueue pp=newLogQueue ();
    if(p==0)
        abort();
    if (pp) {
        deleteItem ((pItem) pp->impl.plist);
        pp->impl.plist = (pList) cloneItem ((pItem)((pLogQueue) p)->impl.plist);
        if (!pp->impl.plist) {
            free (pp);
            return 0;
        }
    }
    return (pItem)pp;
}

static pvirt pvtLogQueue[pvtMaxSize] = {
    dl_LogQueue,
    pr_LogQueue,
    (pvirt) cl_LogQueue
};

pLogQueue newLogQueue ()
{
    pLogQueue ret;
    ret= (pLogQueue) calloc (1, sizeof (LogQueue));

    if (ret) {
        ret->I.pvt = pvtLogQueue;
        ret->impl.plist = mk_List ();
        if (!ret->impl.plist) {
            free (ret);
            return 0;
        }
    }
    return ret;
}

void addToLogQueue (pLogQueue pq, pLogMsg pm)
{
    if(pm==0 || pq==0)
        abort();
    addToList (pq->impl.plist, (pItem) pm);
}

int addOrAppendLQ(pLogQueue pq, char *data,int len)
{
    pLogMsg pm;
    if(pq==0)
        abort();
    pm=tailLogQueue(pq);
    if(!pm || pm->impl.t!=-1){
        short lev; time_t t; short progerr; short failflag; short msg_no;
        if(get_time_and_lev(len,data,&t,&lev,&progerr,&failflag,&msg_no)<0){
            ProgError(STDLOG,"bad data %.*s",len,data);
            (getLoggerFp()<<"logger: bad data ").write(data,len)<<std::endl;
            return -1;
        }
        pm=newLogMsg(len,data);
        if(!pm){
            ProgError(STDLOG,"cannot create log msg");
            abort();
        }
        addToLogQueue(pq,pm);
    }else{
        int rc=addToLogMsg(pm,len,data);
        if(rc<0){
            ProgError(STDLOG,"addToLogMsg failed");
            if(rc==-1){
                abort();
            }
            return -1;
        }
    }
    return 0;
}

pLogMsg getFromLogQueue (pLogQueue pq)
{
    if(pq==0)
        abort();
    return (pLogMsg) detachFirstList (pq->impl.plist);
}
pLogMsg topLogQueue (pLogQueue pq)
{
    if(pq==0)
        abort();
    return (pLogMsg) peekHead (pq->impl.plist);
}

pLogMsg initLogQueueIter(listIterator *I,pLogQueue pq)
{
    return (pLogMsg)initListIterator(I, pq->impl.plist );
}


pLogMsg tailLogQueue (pLogQueue pq)
{
    if(pq==0)
        abort();
    return (pLogMsg) peekTail (pq->impl.plist);
}


int getLogQueueLen (pLogQueue pq)
{
    if(pq==0)
        abort();
    return ListLen (pq->impl.plist);
}

#ifdef XP_TESTING
void LogCensor::reinit()
{
    needMask() = (getConfigTclInt("LOGGER", "HIDE_CC_NUM", 0) > 0);
}
#endif /*XP_TESTING*/

bool& LogCensor::needMask()
{
    static bool instance = (getConfigTclInt("LOGGER", "HIDE_CC_NUM", 0) > 0);

    return instance;
}

/* mask CC card numbers VI/CA/AX */
/* buff - must be a null terminated */
bool LogCensor::apply(char *buff)
{
    if(!needMask()) {
        return false;
    }
    return censure(buff);
}

void writeLogMsg(pLogMsg pm, std::streambuf *fp )
{
    if(pm==0 || fp==0)
        abort();

    if(LogCensor::apply(pm->impl.ptr))
    {
        if(pm->impl.ptr[25] == ' ' and
           pm->impl.ptr[28] == ' ' and
           pm->impl.ptr[26] != ' ' and
           pm->impl.ptr[27] >= '0' and pm->impl.ptr[27] <= '9')
            pm->impl.ptr[26] = pm->impl.ptr[27] = 'C';
    }
    fp->sputn(pm->impl.ptr,pm->impl.len);
    if(pm->impl.ptr[pm->impl.len-1]!='\n')
        fp->sputc('\n');
}

void writeLogQueue(pLogQueue pq, std::streambuf *fp )
{
    pLogMsg pm;
    if(pq==0 || fp==0)
        abort();
    while ((pm=getFromLogQueue(pq))!=0){
        writeLogMsg(pm,fp);
        deleteItem((pItem)pm);
    }
}
