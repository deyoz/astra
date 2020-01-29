#pragma once
#include <ctype.h>
#include <string.h>

#include <oci.h>
#include "commit_rollback.h"

#ifdef __cplusplus
    extern "C" {
#endif
#include "oci_err.h"
#include <ociapr.h>
#ifdef __cplusplus
    }
#endif

#define cda_err(x)  ((x)->curs.rc)
#define err_cda(x)  ((x)->rc)
#define rpc(x)      ((x)->curs.rpc)

/// errors returned by CursCtl::err()
typedef struct {
    sb2 len;
    const char *p;
} place_holder;

typedef struct  {
    sb2 len;
    place_holder *p;
} plhl_tab;

/**
  * variable type , address and length table */
typedef struct {
    sb2 type;
    sword len;
    void *p;
    sb2  *ndx;
} bind_tab;

struct cda_text
{
    Cda_Def curs;        // курсор (CDA, cursor date area)
    Lda_Def *ld;         // структура данных соединения (LDA, logon data area)
    const char *sqltext; // текст SQL запроса
    bind_tab *fetbt;
    int len,old;
    bind_tab *bt;
    plhl_tab *ph;
};
/*
typedef struct _cda_text {
    Cda_Def curs;        // курсор (CDA, cursor date area)
    Lda_Def *ld;         // структура данных соединения (LDA, logon data area)
    const char *sqltext; // текст SQL запроса
    bind_tab *fetbt;
    int len,old;
    bind_tab *bt;
    plhl_tab *ph;
    int ocicpp_bind_size;
    int ocicpp_def_size;

} cda_text;
*/
typedef cda_text* cda_text_ptr;

#ifdef __cplusplus


// Remove 'warning: useless cast' in gcc 4.8 
template < typename T1, typename T2 >
struct __type_cast {
  inline static T1 cast(T2 s) {return reinterpret_cast<T1>(s);}
};
template < typename T > 
struct __type_cast<T,T> {
  inline static  T cast(T s) {return s;}
};
// end

inline text* oratext_cast(const char *s)
{
    return __type_cast<text*,char*>::cast(const_cast<char*>(s));
}
inline text* oratext_cast(unsigned const char *s)
{
  return __type_cast<text*,unsigned char*>::cast(const_cast<unsigned char*>(s));
}

/*
inline text* oratext_cast(const char *s)
{
    return reinterpret_cast<text*>(const_cast<char*>(s));
}
inline text* oratext_cast(unsigned const char *s)
{
    return reinterpret_cast<text*>(const_cast<unsigned char*>(s));
}
*/

inline text* oratext_cast(text *s)
{
    return s;
}

template<typename T> ub1* orabuf_cast(T* t)
{
    return reinterpret_cast<ub1*>(t);
}
template<typename T> ub1* orabuf_cast(const T* t)
{
    return orabuf_cast(const_cast<T*>(t));
}

#else
#define oratext_cast (text*)
#define orabuf_cast (ub1*)
#endif
#ifdef __cplusplus
extern "C"
#endif
int set_null_ind(cda_text *cursor,int line,const char *file);
bool alwaysOCI8();

#define Odefin(c,p,b,bl,t) odefin(&(c)->curs,(p),orabuf_cast(b),(bl),(t),-1,NULL,\
                NULL,-1,-1, NULL, NULL)

#define Odefinps(c,p,b,bl,bs,t) odefinps(&(c)->curs,1,(p),orabuf_cast(b),(bl),\
          (t),0,NULL,NULL,0,0,NULL,NULL,(bs),0,0,0)
#define Idefinps(c,p,b,bl,bs,t,i,is) odefinps(&(c)->curs,1,(p),orabuf_cast(b),(bl),\
          (t),0,i,NULL,0,0,NULL,NULL,(bs),(is),0,0)
#define Ldefinps(c,p,b,bl,bs,t,i,is,l,ls) odefinps(&(c)->curs,1,(p),orabuf_cast(b),(sb4)(bl),\
          (t),0,i,NULL,0,0,(l),NULL,(bs),(is),(ls),0)

#define OPdefinps(c,p,b,bl,bs,t) odefinps(&(c)->curs,0,(p),orabuf_cast(b),(bl),\
          (t),0,NULL,NULL,0,0,NULL,NULL,(bs),0,0,0)
#define IPdefinps(c,p,b,bl,bs,t,i,is) odefinps(&(c)->curs,0,(p),orabuf_cast(b),(bl),\
          (t),0,i,NULL,0,0,NULL,NULL,(bs),(is),0,0)

#define Idefin(c,p,b,bl,t,i) odefin(&(c)->curs,(p),orabuf_cast(b),(bl),(t),-1,i,\
                NULL,-1,-1, NULL, NULL)
#define Ldefin(c,p,b,bl,t,i,l) odefin(&(c)->curs,(p),orabuf_cast(b),(bl),(t),-1,(i),\
                NULL,-1,-1,(l), NULL)



#define Obndraf(c,v,b,bl,t,ind,alen,arcode,ms,cs)   obindps(&(c)->curs,1,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                 0,(ind),(alen),(arcode),bl,(sizeof(sb2)),(sizeof(ub2)),(sizeof(ub2)),ms,cs,NULL,0,0)


#define Obndrv(c,v,b,bl,t) obndrv(&(c)->curs, oratext_cast(v),-1,orabuf_cast(b),(bl),(t),-1,NULL,NULL,-1,-1)

#define Obndrvl(c,v,vl,b,bl,t) obndrv(&(c)->curs,oratext_cast(v),(vl),orabuf_cast(b),(bl),(t),-1,NULL,NULL,-1,-1)

#define Ibndrv(c,v,b,bl,t,ind) obndrv(&(c)->curs,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),-1,(ind),NULL,-1,-1)

#define Obindps(c,v,b,bl,bs,t) obindps(&(c)->curs,1,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                0,NULL,NULL,NULL,(bs),0,0,0,0,NULL,NULL,0,0)

#define Ibindps(c,v,b,bl,bs,t,i,is) obindps(&(c)->curs,1,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                0,i,NULL,NULL,(bs),(is),0,0,0,NULL,NULL,0,0)

#define Lbindps(c,v,b,bl,bs,t,i,is,l,ls) obindps(&(c)->curs,1,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                0,i,(l),NULL,(bs),(is),(ls),0,0,NULL,NULL,0,0)

#define Lbindps3(c,v,vl,b,bl,bs,t,i,is,l,ls,rc,rcs,maxarrlen,currarrlen) obindps(&(c)->curs,1,oratext_cast(v),vl,orabuf_cast(b),(bl),(t),\
                 0,(i),(l),(rc),(bs),(is),(ls),(rcs),maxarrlen,currarrlen,NULL,0,0)

#define OPbindps(c,v,b,bl,bs,t) obindps(&(c)->curs,0,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                 0,NULL,NULL,NULL,(bs),0,0,0,0,NULL,NULL,0,0)

#define IPbindps(c,v,b,bl,bs,t,i,is) obindps(&(c)->curs,0,oratext_cast(v),-1,orabuf_cast(b),(bl),(t),\
                 0,i,NULL,NULL,(bs),(is),0,0,0,NULL,NULL,0,0)


#define OpenOciCursor(x,y) ((x)->sqltext=NULL, \
    (x)->ld=y,oopen(&(x)->curs,(y),NULL,-1,-1,NULL,-1))
#define Osql3(x,y) osql3(&(x)->curs,(((x)->sqltext=(y)),y),-1)
#define Oparse(x,y) ((x)->bt=NULL,(x)->fetbt=NULL,(x)->ph=NULL, \
    oparse(&(x)->curs,(((x)->sqltext=(y)),oratext_cast(y)),-1,1,1))
#define Oexec(x) oexec(&(x)->curs)
#define Oexn(x,y,z) oexn(&(x)->curs,(y),(z))

#define Ofen(x,y) ((x)->old=rpc(x),ofen(&(x)->curs,(y)), \
        set_null_ind((x),__LINE__,__FILE__))

#define Oexfet(x,y) ((x)->old=0,oexfet(&(x)->curs,(y),0,0), \
        set_null_ind((x),__LINE__,__FILE__))

/**
  * same as above, but EXact number of rows */
#define OEXfet(x,y) ((x)->old=0,oexfet(&(x)->curs,(y),1,1), \
        set_null_ind((x),__LINE__,__FILE__))


#define Obndr(cu,v,b,t)    Obndrv((cu),oratext_cast(v),&(b),sizeof(b),(t))
#define Obndrs(cu,v,b)    Obndrv((cu),oratext_cast(v),(b),-1,SQLT_STR)
#define Obndri(cu,v,b)    Obndrv((cu),oratext_cast(v),&(b),sizeof(b),SQLT_INT)
#define Ibndr(cu,v,b,t,i)    Ibndrv((cu),oratext_cast(v),&(b),sizeof(b),(t),i)
#define Ibndrs(cu,v,b,i)    Ibndrv((cu),oratext_cast(v),(b),-1,SQLT_STR,i)
#define Ibndri(cu,v,b,i)    Ibndrv((cu),oratext_cast(v),&(b),sizeof(b),SQLT_INT,i)


#ifdef __cplusplus
extern "C" {
#endif
void ocierror(Lda_Def *LD,int rc, int fc, int off,int line , const char *sqlt, const char* connectString,
     const char *file, const char *nick);

//plhl_tab *create_sqltab(const char *s);

#define oci_error(x,connectString) ocierror((x)->ld,(x)->curs.rc, \
    (x)->curs.fc,(x)->curs.peo,__LINE__,(x)->sqltext,(connectString),__FILE__,NICKNAME)

#define IPP ipp++

extern cda_text * CU, *CU2,*CU3,*DBMSOUT,*ERRORS_C,*CU6,*CU7;
/* ,*CU8,*CU9,*CU10,*CU11; - theese ones are not even initialized  */


#define FIELD_SIZE(type,member) (sizeof( static_cast<type*>(NULL)->member ))
#define FIELD_OFFSET(type,member) ((char* )(&((type*)NULL)->member )-(char *)NULL)

//int OciClose(int close);
void set_cursor_num(int n);

int Oexec_l(cda_text *cur, char *body, int size_body);
//int Oexec_l2(cda_text *cur, char *body, int curl);
int Ofen_l(cda_text *cur, char *first, int size_buff, int size_el, int num_el);

#define D_LONG_LOAD_BLSIZE 2048

/**
 * Executes SQL req with LONG field - to load data
       uses on arrays of structures
  Cur        -  opened cursor
  el_size    -  size of earch long element in structure, may be 0
           >0    rec[nem_el].char[el_size]
           ==0   rec[nem_el].char* (alloc)
  buf_len    -  pointer to fied of structure array to store lengthes,
                may be NULL        rec[nem_el].int
  alen_skip  -  size of structure
  num_el     -  count of items in array of structures to insert data
  exact      -  If this parameter is non-zero,
                oexfen_l_asm() returns an error if the number of rows
                that satisfy the query is not exactly the same as the
                number specified in the nrows parameter. Nevertheless,
                the rows are returned.

                If the number of rows returned by the query is less
                than the number specified in the nrows parameter,
                Oracle returns the error

                ORA-01403:  no data found

                If the number of rows returned by the query is greater
                than the number specified in the nrows parameter,
                Oracle returns the error

                ORA-01422:  Exact fetch returns more than requested

Limitations:
  Only 1 OPdefinps (long field reading) in request !!!     */
int oexfen_l_asm(cda_text *Cur,int el_size,int *buf_len,int alen_skip,
  int num_el,int exact);

/**
 * Executes SQL req with LONG field
       uses on arrays of structures
  Cur        -  opened cursor
  el_size    -  size of earch long element in structure, may be 0
           >0    rec[nem_el].char[el_size]
           ==0   rec[nem_el].char*
  alen_skip  -  size of structure
  num_el     -  count of items in array of structures to insert data
use like with
  OPbindps(Cur,(text*)":LONG",&(Tmp[0].long),sizeof(Tmp[0].long),
    sizeof(*Tmp),SQLT_LNG) ||
  Oexec_l_asm(Cur,sizeof(Tmp[0].long),sizeof(*Tmp),num_el)
or
  OPbindps(Cur,(text*)":LONG",&(Tmp[0].long),strlen(Tmp[0].long)+1,0,SQLT_LNG)||
  Oexec_l_asm(Cur,strlen(Tmp[0].long)+1,0,1)
Limitations:
  Only 1 OPbindps (long inserted field) in request !!!     */
int Oexec_l_asm(cda_text *Cur,int el_size,int alen_skip,int num_el);

/**
  * Returns any count of records */
/*#define Oexfen_l_asm(Cur,el_size,buf_len,alen_skip,num_el) \
  oexfen_l_asm((Cur),(el_size),(buf_len),(alen_skip),(num_el),0) */
/**
  * for compatibility only:  */
#define Ofen_l_asm(Cur,el_size,buf_len,alen_skip,num_el) \
  oexfen_l_asm((Cur),(el_size),(buf_len),(alen_skip),(num_el),0)

/**
  * returns only requested count of records */
/*#define OEXfen_l_asm(Cur,el_size,buf_len,alen_skip,num_el) \
  oexfen_l_asm((Cur),(el_size),(buf_len),(alen_skip),(num_el),1)*/


#ifdef __cplusplus
} // extern "C"
#endif
