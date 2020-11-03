#include "object.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _MSG_QUEUE_
#define _MSG_QUEUE_
struct QueMsgImpl
{
    int len;                    /*length of the text in ptr */
    int type;                   /*for timeouts implementation on lev B*/
    int timeout;                /*for timeouts implementation on lev B*/
    char *ptr;                  /* contents */
    char *ptr2;                  /* saved contents */
    int len2;                   /* saved contents len*/
    int dont_split;
    short special;
};


struct QueMsg
{
    Item I;
    struct QueMsgImpl impl;
} ;
typedef struct QueMsg * pQueMsg;

struct MessageQueueImpl
{
    pList plist;
};
struct MessageQueue
{
    Item I;
    struct MessageQueueImpl impl;
} ;
typedef struct MessageQueue *pMessageQueue;
pQueMsg newQueMsg (int len, const char *data);
int addData2QueMsg(pQueMsg pm,int len, const char *data);
pQueMsg mk_QueMsg ();
void addToQueue (pMessageQueue pq, pQueMsg pm);
void addToQueueFront (pMessageQueue pq, pQueMsg pm);
pQueMsg getFromQueue (pMessageQueue pq);
int getQueueLen (pMessageQueue pq);
pMessageQueue mk_MessageQueue ();
#endif /* _MSG_QUEUE_ */
#ifdef __cplusplus
}
#endif
