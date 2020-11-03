#ifdef __cplusplus
extern "C" {
#endif
#ifndef _OBJECT_H
#define _OBJECT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef int AI_TAG;

#define classtype_(x) typedef x * p##x; \
typedef const x * pc##x



#define pvtDel 0
#define pvtPrint 1
#define pvtClone 2
#define pvtEqual 3
#define pvtSelfDel 4


/*this must be last number + 1*/
#define pvtMaxSize 5

struct Tag;
typedef struct Tag Item;

classtype_ (Item);

typedef void virt (pItem);
typedef virt *pvirt;
typedef pItem clonef (pItem);
typedef pItem equalf (pItem, pItem);
typedef int sortf (pItem, pItem);

virt printItem, deleteItem;
clonef cloneItem;

typedef pvirt pvtDestructor;
typedef pvirt pvtPrintItem;
typedef clonef *pvtCloneItem;
typedef equalf *pvtEqualItem;
typedef sortf  *pvtSortItem;

struct Tag
{
    struct Tag *next;
    struct Tag *prev;
    pvirt *pvt;
};


typedef struct listTag
{
    struct listTag *next;
    struct listTag *prev;
    pvirt *pvt;
    unsigned n;
    pItem head;
}
List;

classtype_ (List);

typedef struct listIterTag
{
    struct listIterTag *next;
    struct listIterTag *prev;
    pvirt *pvt;
    pList l;
    pItem p;
    unsigned n; /*now unused*/
}
listIterator;

classtype_ (listIterator);

/* generic list functions */


void initList (pList that);

void addToList (pList that, pItem p);
void addToListFront (pList that, pItem p);

unsigned ListLen (pList that);

pItem peekTail (pList);

pItem peekHead (pList);

pItem findInList (pList that, unsigned num);

pItem findInList2 (pList that, pItem fi);

pItem findInList3 (pList that, pItem fi, pvtEqualItem equal_func);

pItem insertToList (pList that, pItem p, unsigned num);

pItem detachFromList (pList that, unsigned num);

int delFromList (pList that, unsigned num);

pItem detachFromList2 (pList that, pItem p);

void delFromList2 (pList that, pItem p);

pItem detachFirstList (pList that);

pItem detachLastList (pList that);

void flushList (pList that);

unsigned EmptyList(pList that);

pList mk_List (void);

pItem cl_List (pItem p1);

void pr_List (pItem p1);

/* list iteration functions */

pItem initListIterator (plistIterator pi, pList pl);

pItem listIteratorCur (plistIterator pi);	/* current element */

pItem listIteratorNext (plistIterator pi);	/* ++i */

pItem listIteratorCurNext (plistIterator pi);	/* i++ */

pItem listIteratorPrev (plistIterator pi);	/* --i */

pItem listIteratorCurPrev (plistIterator pi);	/* i-- */

pItem restartListIterator (plistIterator pi);	/* to begin */

pItem rewindListIterator (plistIterator pi);	/* to end */

pItem detachFromListI (plistIterator pi);

/* by Roman K.
 return -1 error
 return 0 - not sorted
 return 1 - Ok
 */
int SortList(pList lList, pvtSortItem func);
pItem listIteratorSet (plistIterator pi, pList pl, pItem p);
pList attachToList(pList dest, pList src);
/*return NULL - error*/
pList clAttachList(pList l1, pList l2);
void setVirtToList(pItem p, pvirt *pv);
/* end Roman*/

#endif
#ifdef __cplusplus
}
#endif
