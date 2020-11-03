#if HAVE_CONFIG_H
#endif

#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "msg_queue.h"
pQueMsg newQueMsg (int len, const char *data)
{
    pQueMsg pp = mk_QueMsg ();
    if (pp) {
        pp->impl.len = len;
        if(len>0 && data)
        {
            pp->impl.ptr = (char *) malloc (len);
            if (pp->impl.ptr == NULL)
            {
                free (pp);
                return NULL;
            }
            memcpy (pp->impl.ptr, data, pp->impl.len);
        }
        else
            pp->impl.ptr = NULL;
    }
    return pp;
}

static void dl_quemsg (pItem p)
{
    free (((pQueMsg) p)->impl.ptr);
    free (((pQueMsg) p)->impl.ptr2);
    ((pQueMsg) p)->impl.ptr = 0;
    ((pQueMsg) p)->impl.ptr2 = 0;
}

static void pr_quemsg (pItem p)
{
}



int addData2QueMsg(pQueMsg pm,int len, const char *data)
{
    
       char *ptr;
        ptr=(char*)malloc(len);
        if(!ptr){
            return -1;
        }
        pm->impl.ptr2=ptr;
        pm->impl.len2=len;
        memcpy(pm->impl.ptr2,data,pm->impl.len2);
        return 0;
}
static pItem cl_quemsg (pItem p)
{
    pQueMsg pm;
    
    pm = newQueMsg (((pQueMsg) p)->impl.len,
                              ((pQueMsg) p)->impl.ptr);
    if(!pm){
        return 0;
    }
    if(((pQueMsg)p)->impl.ptr2){
        if(addData2QueMsg(pm,((pQueMsg)p)->impl.len2,
                    ((pQueMsg)p)->impl.ptr2)<0){
            deleteItem ((pItem)pm);
            return 0;
        }
    }
    return (pItem) pm;
}

static void pvitr_free(pItem i) {  free(i);  }

static pvirt pvtQueMsg[pvtMaxSize] = {
    dl_quemsg,
    pr_quemsg,
    (pvirt) cl_quemsg,
    0,
    pvitr_free,
};

pQueMsg mk_QueMsg ()
{
        pQueMsg ret;
    ret = (pQueMsg) calloc (1, sizeof (struct QueMsg));
    if (ret) {
        ret->I.pvt = pvtQueMsg;
    }
    return ret;
}

pMessageQueue newMessageQueue (pList pl)
{
    pMessageQueue pp = mk_MessageQueue ();
    if (pp) {
        deleteItem ((pItem) pp->impl.plist);
        pp->impl.plist = (pList) cloneItem ((pItem) pl);
        if (!pp->impl.plist) {
            free (pp);
            return 0;
        }
    }
    return pp;
}

static void dl_MessageQueue (pItem p)
{
    deleteItem ((pItem) ((pMessageQueue) p)->impl.plist);
    ((pMessageQueue) p)->impl.plist = 0;
}

static void pr_MessageQueue (pItem p)
{
}


static pItem cl_MessageQueue (pItem p)
{
    return (pItem) newMessageQueue (((pMessageQueue) p)->impl.plist);
}

static pvirt pvtMessageQueue[pvtMaxSize] = {
    dl_MessageQueue,
    pr_MessageQueue,
    (pvirt) cl_MessageQueue
};

pMessageQueue mk_MessageQueue ()
{
       pMessageQueue ret ;
    ret = (pMessageQueue) calloc (1, sizeof (struct MessageQueue));

    if (ret) {
        ret->I.pvt = pvtMessageQueue;
        ret->impl.plist = mk_List ();
        if (!ret->impl.plist) {
            free (ret);
            return 0;
        }
    }
    return ret;
}


void addToQueue (pMessageQueue pq, pQueMsg pm)
{
    addToList (pq->impl.plist, (pItem) pm);
}

void addToQueueFront (pMessageQueue pq, pQueMsg pm)
{
    addToListFront (pq->impl.plist, (pItem) pm);
}

pQueMsg getFromQueue (pMessageQueue pq)
{
    return (pQueMsg) detachFirstList (pq->impl.plist);
}

int getQueueLen (pMessageQueue pq)
{
    return ListLen (pq->impl.plist);
}
