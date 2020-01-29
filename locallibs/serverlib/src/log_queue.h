#ifndef _LOG_QUEUE_
#define _LOG_QUEUE_
#include "object.h"
#ifdef __cplusplus
#include <streambuf>

extern "C" {
#endif
struct LogMsgChunk;
struct LogMsgImpl
{
    int len;                    /*length of the text in ptr */
    char *ptr;                  /* contents */
    time_t t;
    short progerr;
    short failflag;
    short lev;
    short msg_no;
};


typedef struct
{
    Item I;
    struct LogMsgChunk *chunk;
    struct LogMsgImpl impl;
}
LogMsg;
classtype_ (LogMsg);



struct LogQueueImpl
{
    pList plist;
};
typedef struct
{
    Item I;
    struct LogQueueImpl impl;
}
LogQueue;
classtype_ (LogQueue);
pLogMsg newLogMsg (int len, char *data);
pLogQueue newLogQueue ();
void addToLogQueue (pLogQueue pq, pLogMsg pm);
pLogMsg getFromLogQueue (pLogQueue pq);
pLogMsg topLogQueue (pLogQueue pq);
pLogMsg initLogQueueIter(listIterator *I,pLogQueue pq);
int debugLogger(void );
time_t getLogMsgTime(pLogMsg p);
int getLogMsgLev(pLogMsg p);
int getLogMsgProgErr(pLogMsg p);
int getLogMsgFail(pLogMsg p);
int getLogQueueLen (pLogQueue pq);
pLogMsg tailLogQueue (pLogQueue pq);
int addOrAppendLQ(pLogQueue pq, char *data,int len);
/* mask CC here, or what ever we want */
#ifdef __cplusplus
}

class LogCensor
{
public:
#ifdef XP_TESTING
    static void reinit(); // for testing purposes only
#endif /*XP_TESTING*/

    static bool apply(char* buf) __attribute__((warn_unused_result));
private:
    explicit LogCensor();
    static bool& needMask();
};

void writeLogMsg(pLogMsg pm, std::streambuf * );
void writeLogQueue(pLogQueue pq, std::streambuf * );
#endif
#endif /* _MSG_QUEUE_ */
