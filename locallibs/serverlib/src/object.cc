#if HAVE_CONFIG_H
#endif


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "object.h"


#define NICKNAME "object"
#include "test.h"


virt dl_List, dl_Item, pr_List;
clonef cl_List;
equalf equal_List;

/* pseudo virtual tables */


pvirt pvtListTab[pvtMaxSize] = {
    dl_List,
    pr_List,
    (pvirt) cl_List,
    NULL,
};


/* generic list functions */


void initList (pList that)
{
    that->n = 0;
    that->head = NULL;
    that->pvt = pvtListTab;
}

pList mk_List (void)
{
    pList p = (pList) malloc (sizeof (List));
    if (!p)
        return NULL;
    initList (p);
    return p;
}


void addToListFront (pList that, pItem p)
{

    if (that->n == 0) {
        that->head = p;
        that->head->next = that->head->prev = that->head;
    } else {
        p->next = that->head;
        p->prev = that->head->prev;
        that->head->prev->next = p;
        that->head->prev = p;
        that->head=p;
    }
    that->n++;
}

void addToList (pList that, pItem p)
{

    if (that->n == 0) {
        that->head = p;
        that->head->next = that->head->prev = that->head;
    } else {
        p->next = that->head;
        p->prev = that->head->prev;
        that->head->prev->next = p;
        that->head->prev = p;
    }
    that->n++;
}

unsigned ListLen (pList that)
{
    return that->n;
}
unsigned EmptyList(pList that)
{
    return that->head==0 ? 1:0;
}

pItem findInList (pList that, unsigned num)
{
    if (num < that->n) {
        unsigned i;
        pItem rover;
        for (i = 0, rover = that->head; i < num; i++, rover = rover->next);
        return rover;
    } else {
        return NULL;
    }
}

pItem insertToList (pList that, pItem p, unsigned num)
{
    pItem old = findInList (that, num);
    if (!old) {
        unsigned ll = ListLen (that);
        if (ll == 0 || ll == num) {
            addToList (that, p);
            return p;
        }
    }

    p->next = old;
    p->prev = old->prev;

    p->prev->next = p;
    old->prev = p;

    if (num == 0)
        that->head = p;
    that->n++;
    return p;
}


pItem detachFromList (pList that, unsigned num)
{
    pItem p = findInList (that, num);
    if (!p)
        return NULL;

    detachFromList2 (that, p);
    return p;
}

pItem detachFromListI (plistIterator pi)
{
    pItem p=pi->p;
    if (p == NULL ||  p->next == pi->l->head)
        pi->p = 0;
    else
        pi->p=p->next;
    detachFromList2(pi->l,p);
    return p;
}

int delFromList (pList that, unsigned num)
{
#if 0                           /* was */
    pItem p = detachFromList (that, num);

    if (!p->pvt || !p->pvt[pvtDel])
        return 0;
    (*(pvtDestructor) (p->pvt[pvtDel])) (p);
    return 1;
#else
    pItem p = detachFromList (that, num);
    if (!p)
        return 0;
    deleteItem (p);
    return 1;
#endif
}


pItem detachFromList2 (pList that, pItem p)
{
    if (that->n == 0)
        return NULL;

    that->n--;

    if (that->n == 0) {
        that->head = NULL;
    } else {
        if (p == that->head) {
            that->head = p->next;
        }
        p->prev->next = p->next;
        p->next->prev = p->prev;
    }
    return p;
}

void delFromList2 (pList that, pItem p)
{
    detachFromList2 (that, p);
    /*
       if(p->pvt && p->pvt[pvtDel])
       (*(pvtDestructor)(p->pvt[pvtDel]))(p);
     */
    deleteItem (p);
}

pItem detachFirstList (pList that)
{
    return detachFromList2 (that, that->head);
}

pItem detachLastList (pList that)
{
    assert (that);
    return that->head ? detachFromList2 (that, that->head->prev) : NULL;
}

pItem peekTail (pList that)
{
    assert (that);
    return that->head ? that->head->prev : NULL;
}

pItem peekHead (pList that)
{
    assert (that);
    return that->head;
}


pItem cl_List (pItem p1)
{
    listIterator i;
    pItem pp, ppp;
    pList p = mk_List ();
    p->pvt = ((pList)p1)->pvt;
    for (pp = initListIterator (&i, (pList) p1); pp;
         pp = listIteratorNext (&i)) {
        ppp = cloneItem (pp);
	if (!ppp) {
	    deleteItem ((pItem) p);
            return NULL;
        }
        addToList (p, ppp);
    }
    return (pItem) p;
}

void flushList (pList that)
{

    if (that) {
        while (ListLen (that)) {
            delFromList2 (that, that->head->prev);
        }
    }
}

void dl_List (pItem p)
{
    flushList ((pList) p);
}

void pr_List (pItem p1)
{
    listIterator I;
    pItem p;
    int i = 1;
    for (p = initListIterator (&I, (pList) p1); p;
         p = listIteratorNext (&I)) {
        printItem (p);
        i = 0;
    }
}


/*
static pList  getN_S_T_List(void)
{
    static *pl;
    if(!pl)
        pl=mk_List();
    return pl;
}

struct nst_type {
    Item I;
    pvirt vptr;
    int virt_len;
}


int addNonStandardType(pvirt pv, int n)
{
    struct nst_type *p=(struct nst_type*)calloc (1,sizeof *p);
    if(!p)
        return -1;
    p->vptr=pv;
    p->virt_len=n;
    addToList(getN_S_T_List(),(pItem)p);
    return 0;
}
*/




void printItem (pItem p)
{
    if (p && p->pvt && p->pvt[pvtPrint])
        (*(pvtPrintItem) (p->pvt[pvtPrint])) (p);
    else {
        ProgError (STDLOG, "Null pointer or \'pr_Item\' not defined");
    }
}


pItem cloneItem (pItem p)
{
    assert (p);
    if (p->pvt && p->pvt[pvtClone])
        return (*(pvtCloneItem) (p->pvt[pvtClone])) (p);
    assert (0);
    return NULL;
}







void deleteItem (pItem p)
{
    if (p) {
        if (p->pvt && p->pvt[pvtDel])
            (*(pvtDestructor) (p->pvt[pvtDel])) (p);
        if(p->pvt && p->pvt[pvtSelfDel]){
            ((pvirt)p->pvt[pvtSelfDel])(p);
        }else{
            free (p);
        }
    }
}


pItem initListIterator (plistIterator pi, pList pl)
{
    pi->p = pl->head;
    pi->l = pl;
    return pi->p;
}

pItem listIteratorCur (plistIterator pi)
{
    return pi->p;
}

pItem listIteratorNext (plistIterator pi)
{
    if (pi->p == NULL
        ||  pi->p->next ==
        pi->l->head) return NULL;
    return pi->p = pi->p->next;

}

pItem listIteratorCurNext (plistIterator pi)
{
    pItem ret=pi->p;
    if (ret==0)
        return ret;
    if (ret->next == pi->l->head)
        pi->p=0;
    else
        pi->p=ret->next;
    return ret;
}

pItem listIteratorPrev (plistIterator pi)
{
    if (pi->p==0)
        return 0;
    if (pi->p == pi->l->head)
        pi->p=0;
    else
        pi->p=pi->p->prev;
    return pi->p;
}

pItem listIteratorCurPrev (plistIterator pi)
{
    pItem ret=pi->p;
    if (ret==0)
        return 0;
    if (pi->p == pi->l->head)
        pi->p=0;
    else
        pi->p=pi->p->prev;
    return ret;
}

pItem restartListIterator (plistIterator pi)
{
    return initListIterator (pi, pi->l);
}

pItem rewindListIterator (plistIterator pi)
{
    pList l = pi->l;
    initListIterator (pi, l);
    pi->p = l->head->prev;
    return pi->p;
}


pItem findInList2 (pList that, pItem fi)
{
    listIterator I;
    pItem p;
    assert (that);
    assert (fi);
    for (p = initListIterator (&I, that); p; p = listIteratorNext (&I)) {
        if (p->pvt && p->pvt[pvtEqual]) {
            if ((*(pvtEqualItem) (p->pvt[pvtEqual])) (p, fi))
                return p;
        } else {
            assert (0);
        }
    }
    return NULL;
}

pItem findInList3 (pList that, pItem fi, pvtEqualItem equal_func)
{
    listIterator I;
    pItem p;
    assert (that);
/*	assert(fi);
	fi м.б. нулевым, но equal_func д.б. готов к этому */
    for (p = initListIterator (&I, that); p; p = listIteratorNext (&I)) {
        if ((equal_func) (p, fi)) {
            return p;
        }
    }
    return NULL;
}


/* by Roman K.
 Функция имеет два параметра и должна возвращать [>0, <0, 0].
 Для сортировки по возрастанию как есть (param1-param2),
 по убыванию (-(param1-param2)) => (param2-param1).
 return -1 error
 return 0 - not sorted
 return 1 - Ok
 */
int SortList(pList lList, pvtSortItem func)
{
    listIterator I = {};
    pItem p, pm=NULL, pmn=NULL;
    pItem psm=NULL, psmn=NULL;

    int i, lLen=ListLen(lList);

    int sch;

    if(lLen <= 1){
	return 0;
    }
    for(i=0;i<lLen/2;i++){
	sch=i;
	for((i)?((&I)->p = (p=pmn->next)):(p=initListIterator(&I,lList));
	    p;
	    p=listIteratorNext(&I)){

	    if(sch==i){
		pm=p;
		pmn=p;
		sch++;
		continue;
	    }
	    if(sch==lLen-i)
		break;

	    if(func(p, pm)>0){
		pm=p;
	    }else if(func(p, pmn)<0){
		pmn=p;
	    }
	    sch++;
	} /*for(...)*/

	if(pm!=pmn){
            pm=detachFromList2(lList,pm);
	    if(pm==NULL)
		return -1;
	    if(!i){
		addToList(lList,pm);
	    }   else {
		pm->next = psm;
		pm->prev = psm->prev;
		pm->prev->next = pm;
		psm->prev = pm;
		lList->n++;
	    }
	    pmn=detachFromList2(lList, pmn);
	    if(pmn==NULL)
		return -1;
	    if(!i){
		insertToList(lList,pmn,i);
	    }else {
		psm = psmn->next;
		pmn->next = psm;
		pmn->prev = psm->prev;
		pmn->prev->next = pmn;
		psm->prev = pmn;
		lList->n++;
	    }
	    psm=pm;
	    psmn=pmn;
	}else {
            break;
	}
    }   /*for (i=0...)*/
    return 1;
}
/*by Roman*/
pItem listIteratorSet (plistIterator pi, pList pl, pItem p)
{
    pi->p = p;
    pi->l = pl;
    return pi->p;
}

/*by Roman
 */
pList attachToList(pList dest, pList src)
{
    pItem src_last = NULL;
    assert(dest);
    assert(src);

    if(src->head){
	src_last = src->head->prev;
    }else {
        return dest;
    }

    if(dest->head){
	dest->head->prev->next = src->head;
	src->head->prev = dest->head->prev;
	src_last->next = dest->head;
	dest->head->prev = src_last;
	dest->n += src->n;
    }else {
	dest->head = src->head;
        dest->n = src->n;
    }

    src->head = NULL;
    src->n = 0;

    return dest;
}

void setVirtToList(pItem p, pvirt *pv)
{
    p->pvt = pv;
}

/*
 By Roman
 return NULL - error
 */
pList clAttachList(pList l1, pList l2)
{
    pList p1=(pList)cl_List((pItem)l1), p2=(pList)cl_List((pItem)l2);
    pList res=NULL;

    if(!p1 || !p2){
	if(p1){
	    flushList(p1);
            deleteItem((pItem)p1);
	}else{
            flushList(p2);
            deleteItem((pItem)p2);
	}
        return NULL;
    }
    res = attachToList(p1, p2);

    deleteItem((pItem)p2);
    return res;
}

#ifdef WITH_MAIN_TEST
typedef struct {
    Item I;
    int f;
} testS;
classtype_(testS);

void test_print( pItem p)
{
    printf("--%d\n",((ptestS)p)->f);
}
static pvirt pvttestS[pvtMaxSize] = {
    0,
    test_print,
    0,
};
ptestS mk_testS (int f)
{
    ptestS ret;
    ret = (ptestS) calloc (1, sizeof (testS));
    if (ret) {
        ret->I.pvt = pvttestS;
        ret->f=f;
    }
    return ret;
}

int main()
{
    pList pl=mk_List();
    listIterator i;
    pItem pp;
    addToList(pl,(pItem)mk_testS(1));
    addToList(pl,(pItem)mk_testS(2));
    addToList(pl,(pItem)mk_testS(3));
    printItem((pItem)pl);
    addToListFront(pl,(pItem)mk_testS(11));
    addToListFront(pl,(pItem)mk_testS(12));
    printItem((pItem)pl);
    while(ListLen(pl)){
        printf("---------\n");
        delFromList(pl,0);
        printItem((pItem)pl);
    }
}
#endif
