#include <array>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>

#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "tcl_utils.h"
#include "testmode.h"

//#define BOOST_UNORDERED_MAP
#ifdef BOOST_UNORDERED_MAP
#include <boost/unordered_map.hpp>
#else
#include <map>
#endif

#include "ocilocal.h"
#include "cursctl.h"
#include "oci_row.h"
#include "oci8.h"
#include "helpcpp.h"
#include "str_utils.h"

#include "dump_table.h"
#include "deadlock_exception.h"
#include "dates_oci.h"
#include "posthooks.h"
#include "copytable.h"
#include "oci_selector_char.h"
#include "oci_default_ptr_t.h"

#define NICKNAME "KONST"
#include "slogger.h"

#include <algorithm>
#ifndef OraText
#define OraText unsigned char
#endif

extern "C" char const * tclmonFullProcessName ();
static int abort7=0;
void  setAbort7()
{
    abort7=1;
}

bool alwaysOCI8()
{
    static int always=-1;
    return getVariableStaticBool("ALWAYS_OCI8", &always, 1);
}

namespace {
#ifdef XP_TESTING
void Sleep(int t)
{
    if(getenv("UNDER_GDB")){
        fprintf(stderr,"gdb obrzap %d\n",getpid());
        sleep(t);
    }
}
#endif
bool warnTwiceBind()
{
    static const bool warn = readIntFromTcl("CURSCTL_WARN_TWICE_BIND", 1);
    return warn;
}
}
const char* get_connect_string()
{
    static std::string connStr = readStringFromTcl("CONNECT_STRING");
    LogTrace(TRACE5) << "get_connect_string() :: "<<connStr<< ".";
    return connStr.c_str();
}


void split_connect_string(const std::string & connect_string, oracle_connection_param& params)
{
    std::string::size_type posSlash = connect_string.find("/");
    std::string::size_type posDog = connect_string.find("@");

    if (posSlash == std::string::npos) {
        ProgError(STDLOG, "Invalid connect string: '%s'", connect_string.c_str());
        throw comtech::Exception("Invalid Oracle connect string");
    } else {
        if (posDog == std::string::npos) { // no server specified : sirena/orient
            params.login = connect_string.substr(0, posSlash);
            params.password = connect_string.substr(posSlash + 1);
        } else {
            if (posSlash < posDog) { // sirena/orient@sir9
                params.login = connect_string.substr(0, posSlash);
                params.password = connect_string.substr(posSlash + 1, posDog - posSlash - 1);
                params.server = connect_string.substr(posDog + 1);
            } else { // sirena@sir9/orient
                params.login = connect_string.substr(0, posDog);
                params.password = connect_string.substr(posSlash + 1);
                params.server = connect_string.substr(posDog + 1, posSlash - posDog - 1);
            }
        }
    }
    if (params.login.find_first_of("@/") != std::string::npos) {
        ProgError(STDLOG, "Invalid connect string: '%s' login=%s", connect_string.c_str(), params.login.c_str());
        throw comtech::Exception("Invalid Oracle connect string");
    }
    if (params.password.find_first_of("@/") != std::string::npos) {
        ProgError(STDLOG, "Invalid connect string: '%s' password=%s", connect_string.c_str(), params.password.c_str());
        throw comtech::Exception("Invalid Oracle connect string");
    }
}

std::string make_connect_string(const oracle_connection_param& param)
{
    std::string connectionString = param.login + "/*****"; // masked password
    if (!param.server.empty())
    {
        connectionString += "@" + param.server;
    }

    return connectionString;
}
#if 0
static place_holder * find_place_holder(const char *sql)
{
    static const char *S;
    if ( sql )
    {
        S=sql;
    }
    if ( S )
    {
        const char * found=S;
        for(;found;)
        {
            found=strpbrk(found,":\'");
            if ( !found )
            {
                return NULL;
            }
            if ( *found==':' )
            {
                static place_holder ret;
                ret.len=strcspn(found+1," ;,)(=:-+/*><!?\'{}\n\t");
                if(ret.len==0)
                {
                    found++;
                    continue;
                }
                ret.len++;
                ret.p=found;
                S=found+1;
                return &ret;
            }
            found=strchr(found+1,'\'');
        }

    }
    return NULL;
}



/*-----------------12/01/1996 20:15-----------------
    finds placeholder in the list
 --------------------------------------------------*/
using namespace std;
static int look_for_plhl(vector<place_holder> &list, place_holder *ph)
{
    vector<place_holder>::iterator p;
    for (p = list.begin(); p !=list.end(); ++p) {
        if (p->len == ph->len &&
            !strncmp(p->p, ph->p, ph->len)) {
            return 1;
        }
    }

    return 0;
}

#ifdef __cplusplus
    extern "C"
#endif

/*-----------------12/01/1996 20:16-----------------
    creates tables for placeholders

    such table must be created for
    sql request in order to have easy
    access to placeholders names
    with obndrv and obndra OCI functions

    I suggest to allocate such tables only
    once on first call to function that
    contain this sql string

    I also suggest make sql strings in the
    same manner (on first call)

 --------------------------------------------------*/

plhl_tab *create_sqltab(const char *s)
{
    vector<place_holder> List;
    place_holder *ph;
    plhl_tab *ret;
    if ( !s )
        return NULL;
    for ( ph=find_place_holder(s); ph;
                ph=find_place_holder(NULL) ) {
        if ( !look_for_plhl(List,ph) ) {
            List.push_back(*ph);
        }
    }
    ret=(plhl_tab*)malloc(sizeof(plhl_tab));
    if ( ret ) {
        ret->len=List.size();
        ret->p=(place_holder *)malloc(ret->len*sizeof(place_holder));
        if ( ret->p ) {
            copy(List.begin(),List.end(),ret->p);
        }else{
            tst();
            free(ret);
            ret=NULL;
        }
    }
    tst();
    return ret;
}
#endif // 0
/*this function set values of null select items to
the empty strings, providing they have character type,
have associated indicator variables and todefin function was used;
called automatically */

int set_null_ind(cda_text *cursor,int line,const char *file)
{
    bind_tab *pt=cursor->fetbt; /* set to NULL by Oparse*/
    int count=rpc(cursor)-CU->old;
    int len=CU->len-1;
    if(pt && !cursor->curs.rc){
        for (;len>=0;len--){
            if(pt[len].ndx &&
                (pt[len].type==SQLT_STR  ||
                pt[len].type==SQLT_CHR  ||
                pt[len].type==SQLT_VCS  ||
                pt[len].type==SQLT_VBI)  ){
                int i;
                for(i=0;i<count;i++){
                    switch(pt[len].type){
                        case SQLT_STR:
                        case SQLT_CHR:
                            ((char*)pt[len].p+pt[len].len*count)[0]=0;
                            break;
                        case SQLT_VCS:
                        case SQLT_VBI:
                            ((sb2*)((char*)pt[len].p+pt[len].len*count))[0]=0;
                            break;
                        default:
                            Logger::getTracer().ProgTrace(TRACE1,
                                "UNHANDLED NULL VALUE %s %d N=%d",file,line,len);
                            break;
                    }
                }
            }
        }
    }
    return cursor->curs.rc;
}



/* debug output  functions*/


void ocierror(Lda_Def *LDa, int rc, int fc, int off,int line , const char *sqlt, const char* connectString,
    const char *file, const char *nick)
{
    char s[4000];
    oerhms(LDa,(sb2)rc,(OraText*)s,(sword)sizeof(s));
    Logger::getTracer().ProgError(nick,file,line,"[%s] %.4000s fc=%d off=%d\n%.3000s",connectString,s,fc,off,SNULL(sqlt));
    if (off>0 && sqlt!=nullptr && strlen(sqlt)>size_t(off))
    {
      int beg=off>40?off-40:0;
      int sep=off>40?39:off;

      Logger::getTracer().ProgTrace(getTraceLev(TRACE1),nick,file,line,"Query: %s",std::string(sqlt).substr(beg,80).c_str());
      Logger::getTracer().ProgTrace(getTraceLev(TRACE1),nick,file,line,"       %s",(std::string(sep,'-')+"^"+std::string(40,'-')).c_str());
    }
}

static cda_text staticCursorsArray[20];
cda_text

    * CU=staticCursorsArray, *CU2=staticCursorsArray+1,*CU3=staticCursorsArray+2,*DBMSOUT=staticCursorsArray+3,
    * ERRORS_C=staticCursorsArray+4, *CU6=staticCursorsArray+5,*CU7=staticCursorsArray+6;
/*    *CU8=staticCursorsArray+7,                 - these ones are not even initialized *
 *   * CU9=staticCursorsArray+8, *CU10=staticCursorsArray+9,*CU11=staticCursorsArray+10,*CU12=staticCursorsArray+11;             */

//static ub1 hda[256] = {0};
#define CURSN 7
static int ncurs=CURSN;
void set_cursor_num(int n)
{
    ncurs=n;
}

void validateStr(const char* str)
{
    if (!str)
        return;
    std::string res;
    int i = 0;
    while (str[i] != 0) {
        res += toupper(str[i]);
        i++;
    }
    res = StrUtils::trim(res);
    if (res == "COMMIT" || res == "ROLLBACK") {
        abort();
    }
}

static cda_text * getStaticCursor(const char *str, Lda_Def* ld, const OciCpp::OciSession& session)
{
#ifdef XP_TESTING
    if (inTestMode())
    {
        validateStr(str);
    }
#endif
    cda_text *Cursor = (cda_text*) malloc(sizeof(Cursor[0]));
    if (!Cursor)
    {
        abort();
    }

    if (OpenOciCursor(Cursor, ld))
    {
        OciCpp::error(Cursor, session, STDLOG);
        Logger::getTracer().ProgError(STDLOG, "getStaticCursor:Oopen failed");
        std::cerr << "getStaticCursor:Oopen failed" << std::endl;
        free(Cursor);
        return 0;
    }
    const char* str2 = strdup(str);
    if (!str2)
    {
        abort();
    }
    if (Oparse(Cursor, str2))
    {
        OciCpp::error(Cursor, session, STDLOG);
        Logger::getTracer().ProgError(STDLOG, "getStaticCursor: Oparse failed");
        std::cerr << "getStaticCursor:Oparse failed" << std::endl;
        free(const_cast<char*>(Cursor->sqltext));
        if(oclose(&Cursor->curs))
        {
            OciCpp::error(Cursor, session, STDLOG);
        }
        free(Cursor);
        return 0;
    }
    return Cursor;
}

int Oexec_l(cda_text *cur, char *body, int size_body)
{
        int size=1000;
        int curl=strlen(body)+1;
        int nchunks=0;
        ub1 piece;
        char *buf=body;
        ub4 bufl;
        ub4 iter,pltab;
        ub1 *cont;

        while(1){
                Oexec(cur);
                switch(cda_err(cur)){
                        case 0:
                                tst();
                                goto out;
                        case 3129:
                                /*give me more*/
                                tst();
                                if(size_body-curl<1000) goto out;
                                if(ogetpi(&cur->curs,&piece,(dvoid**)&cont,
                                                &iter,&pltab)){
                                        return -1;
                                }
                                if(piece==2 ){
                                        Logger::getTracer().ProgTrace(TRACE4,"bufl=%d",bufl);
                                        curl-=bufl;
                                }
                                if(curl<size){
                                        piece=3;        /*finish*/
                                        bufl=curl;
                                }else{
                                        bufl=size;
                                }
                                Logger::getTracer().ProgTrace(TRACE4,"piece=%d bufl=%d str=%.*s",
                                                piece,bufl,size,buf+nchunks*size);
                                if(osetpi(&cur->curs,piece,
                                                buf+nchunks*size,&bufl)<0){
                                        return -1;
                                }
                                nchunks++;
                                break;
                        default:
                                return -1;
                }
        }

out:
       return 0;
}


int oexfen_l_asm(cda_text *Cur,int el_size,int *buf_len,int alen_skip,
  int num_el,int exact)
/*  Executes SQL req with LONG field - to load data
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
{
#ifndef D_ASM_ERR_PROG
  #define D_ASM_ERR_PROG 14000
#endif
#ifndef MIN
  #define MIN(a,b) (((a)>(b))?(b):(a))
#endif
  int lErr=0;
  int *err=&lErr;

  if(*err==0)
  {
    int  key=TRUE;
    ub4  blsize=0;
    int  line_buf_len=0;
    char *line_buf=NULL;
    void *ctxpp=NULL;
    ub4  iterp=(ub4)-1;

    while(*err==0 && key)
    {
      Logger::getTracer().ProgTrace(TRACE5,"LOOP!!!---------------------------------------");
      OEXfet(Cur,num_el);
      switch(cda_err(Cur))
      {
        case CERR_EXACT:
        case NO_DATA_FOUND:
        case 0:
          tst();
          line_buf_len+=blsize;
          if ((signed)iterp>=0)
          {
            Logger::getTracer().ProgTrace(TRACE5,"iterp=%i",iterp);
            if (buf_len!=NULL)
              *(int*)((char*)buf_len+(iterp)*alen_skip)=line_buf_len;
            if (ctxpp!=NULL)
            {
              if (el_size<=0)
              {
                tst();
                *(char**)((char*)ctxpp+(iterp)*alen_skip)=line_buf;
                line_buf=NULL;
              }
              else
                memcpy((char*)ctxpp+(iterp)*alen_skip,line_buf,
                  el_size>line_buf_len?line_buf_len:el_size);

            };
          };
          if (*err==0 && cda_err(Cur)!=0 && exact!=0)
          {
            tst();
            *err=D_ASM_ERR_PROG;
          }
          if (*err==0 && cda_err(Cur)==NO_DATA_FOUND && (signed)iterp<0)
          {
            tst();
            *err=D_ASM_ERR_PROG;
          };
          key=FALSE;
          break;
        case REQUIRED_NEXT_BUFFER:
        {
          ub1  piece = 0;
          ub4  indexp = 0;

          tst();
          if (*err==0 && ogetpi(&Cur->curs,&piece,&ctxpp,&iterp,&indexp))
            *err=D_ASM_ERR_PROG;
          Logger::getTracer().ProgTrace(TRACE5,"piece=%u ctxpp=%p iterp=%i indexp=%u",piece,ctxpp,iterp,indexp);
          if(*err==0)
          {
            if (piece==OCI_FIRST_PIECE)
            {
              tst();
              line_buf_len+=blsize;
              if (iterp>0 && buf_len!=NULL)
                *(int*)((char*)buf_len+(iterp-1)*alen_skip)=line_buf_len;
              if (el_size<=0)
              {
                tst();
                if (iterp>0)
                  *(char**)((char*)ctxpp+(iterp-1)*alen_skip)=line_buf;
                *(char**)((char*)ctxpp+iterp*alen_skip)=NULL;
                line_buf=NULL;
              }
              else
              {
                tst();
                if (iterp>0)
                  memcpy((char*)ctxpp+(iterp-1)*alen_skip,line_buf,
                    el_size>line_buf_len?line_buf_len:el_size);
              };
              line_buf_len=0;
            };
            if (piece==OCI_NEXT_PIECE)
            {
              tst();
              line_buf_len+=blsize;
            };
            blsize=D_LONG_LOAD_BLSIZE;
            if (line_buf==NULL)
              line_buf=(char*)malloc(line_buf_len+blsize);
            else
              line_buf=(char*)realloc(line_buf,line_buf_len+blsize);
            if(line_buf==NULL)
            {
              Logger::getTracer().ProgError(STDLOG,"Malloc Error!!! (size=%i)",
                line_buf_len+blsize);
              *err=D_ASM_ERR_PROG;
            };
          };

          if(*err==0 &&
            osetpi(&Cur->curs,piece,line_buf+line_buf_len,&blsize))
            *err=D_ASM_ERR_PROG;
          break;
        };
        default:
        {
          Logger::getTracer().ProgTrace(TRACE1,"cda_err(Cur)=%i",cda_err(Cur));
          *err=D_ASM_ERR_PROG;
        };
      };
      Logger::getTracer().ProgTrace(TRACE5,"----!!!---------------------------------------");
    };
    tst();
    free(line_buf);
  };

  if (*err!=0)
  {
    Logger::getTracer().ProgTrace(TRACE1,"*err=%i",*err);
    return -1;
  };
  return 0;
}

int Oexec_l_asm(cda_text *Cur,int el_size,int alen_skip,int num_el)
/*  Executes SQL req with LONG field
       uses on arrays of structures
  Cur        -  opened cursor
  el_size    -  size of earch long element in structure, may be 0
           >0    rec[nem_el].char[el_size]
           ==0   rec[nem_el].char*
  alen_skip  -  size of structure
  num_el     -  count of items in array of structures to insert data
use like with
  OPbindps(Cur,":LONG",&(Tmp[0].long),sizeof(Tmp[0].long),
    sizeof(*Tmp),SQLT_LNG) ||
  Oexec_l_asm(Cur,sizeof(Tmp[0].long),sizeof(*Tmp),num_el)
or
  OPbindps(Cur,":LONG",&(Tmp[0].long),strlen(Tmp[0].long)+1,0,SQLT_LNG)||
  Oexec_l_asm(Cur,strlen(Tmp[0].long)+1,0,1)
Limitations:
  Only 1 OPbindps (long inserted field) in request !!!     */
{
#ifndef D_ASM_ERR_PROG
  #define D_ASM_ERR_PROG 14000
#endif
  int lErr=0;
  int *err=&lErr;
  int  tmp_len=0;

  tst();
  if (*err==0)
  {
    int key=TRUE;
    int  saved=0;
    ub4  blsize=0;

    while(*err==0 && key)
    {
      Oexn(Cur,num_el,0);
      switch(cda_err(Cur))
      {
        case 0:
          tst();
          key=FALSE;
          break;
        case REQUIRED_NEXT_PIECE:
        {
          ub1  piece;
          void *ctxpp;
          ub4  iterp,indexp;
          char *buf;

          if(*err==0 && ogetpi(&Cur->curs,&piece,&ctxpp,&iterp,&indexp))
            *err=D_ASM_ERR_PROG;
          if (*err==0)
          {
            if(piece==OCI_NEXT_PIECE)
              saved+=blsize;
            blsize=D_LONG_LOAD_BLSIZE;
            if (el_size>0)
            {
              buf=(char*)ctxpp+alen_skip*iterp;
              tmp_len=el_size;
            }
            else
            {
              buf=*((char**)((char*)ctxpp+alen_skip*iterp));
              if (tmp_len==0)
                tmp_len=strlen(buf)+1;
            };
            if (saved+blsize>=(ub4)tmp_len)
            {
              blsize=tmp_len-saved;
              piece=OCI_LAST_PIECE;
            };
          };
          if(*err==0 && osetpi(&Cur->curs,piece,buf+saved,&blsize))
            *err=D_ASM_ERR_PROG;
          if (piece==OCI_LAST_PIECE)
          {
            tmp_len=0;
            saved=0;
          };
          break;
        }
        default:
        {
          tst();
          *err=D_ASM_ERR_PROG;
        };
      };
    };
  };
  if (*err!=0)
    return -1;
  return 0;
}
#if 0
int Oexec_l2(cda_text *cur, char *body, int curl)
{
        int size=1000;
        int nchunks=0;
        ub1 piece;
        char *buf=body;
        ub4 bufl;
        ub4 iter,pltab;   ub1 *cont;

        while(1){
                Oexec(cur);
                switch(cda_err(cur)){
                        case 0:
                                goto out;
                        case 3129:
                                /*give me more*/
                                if(ogetpi(&cur->curs,&piece,(dvoid**)&cont,
                                                &iter,&pltab)){
                                        return -1;
                                }
                                if(piece==2 ){
                                        Logger::getTracer().ProgTrace(TRACE4,"bufl=%d",bufl);
                                        curl-=bufl;
                                }
                                if(curl<size){
                                        piece=3;        /*finish*/
                                        bufl=curl;
                                }else{
                                        bufl=size;
                                }
                                Logger::getTracer().ProgTrace(TRACE4,"piece=%d bufl=%d",
                                                piece,bufl);
                                if(osetpi(&cur->curs,piece,
                                                buf+nchunks*size,&bufl)<0){
                                        return -1;
                                }
                                nchunks++;
                                break;
                        default:
                                return -1;
                }
        }

out:
       return 0;
}
#endif // 0

static std::map<Lda_Def*, OciCpp::OciSession*> allSessions;

static void innerCloseCurs_7(const cda_text* crs )
{
    assert(crs);
    assert(crs->sqltext);
    auto it = allSessions.find(crs->ld);
    if (it != allSessions.end())
    {
        it->second->removeFromCache(crs->sqltext);
    }
    else
    {
        Logger::getTracer().ProgError(STDLOG, "close cursor, but session not found: %s", crs->sqltext);
    }
}

namespace OciCpp
{
namespace dt
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;
}

oracle_datetime::oracle_datetime()
{
  memcpy(&value, "\x64\x64\x00\x00\x01\x01\x01", 7);
}

oracle_datetime::oracle_datetime(int year, int month, int day, int hour, int min, int sec)
{
  value[3] = day;
  value[2] = month;
  value[0] = (int)(year / 100) + 100;
  value[1] = year % 100 + 100;
  value[4] = hour + 1;
  value[5] = min + 1;
  value[6] = sec + 1;
}

void removeFromCacheC(cda_text *curs)
{
    innerCloseCurs_7(curs);
}

std::string make_nick(const char* nick_, const char* file_, int line_)
{
    std::stringstream s;
    s<<nick_<<":"<<file_<<":"<<line_;
    return s.str();
}

std::ostream & operator << (std::ostream& os,const oracle_datetime &t)
{
    os<<boost::format("'%1$04d.%2$02d.%3$02d %4$02d:%5$02d:%6$02d'") \
        % int((t.value[0]-100)*100+t.value[1]-100) % int(t.value[2]) % int(t.value[3]) \
        % int(t.value[4]-1) % int(t.value[5]-1) % int(t.value[6]-1);
    return os;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

class OracleData
{
    unsigned ocicpp_bind_size = 0;
    unsigned ocicpp_def_size = 0;
    OciSession* sess_;

  public:

    explicit OracleData(OciSession *_sess) : sess_(_sess) {}
    virtual void throwException();
    virtual void exec_prepare(int){}
    virtual ~OracleData() {}
    virtual std::string error_text() const=0;
    virtual std::string error_text2() const =0;
    virtual int err() const noexcept =0;
    virtual int err8() const noexcept { return 0; }
    virtual    int rowcount() =0;
    virtual    char const * sqltext() const noexcept =0;
    virtual    void setErrCode(int code) =0;
    void save_vector_sizes( BindList_t const &bind_bufs,DefList_t const & sel_bufs)
    {
        if(ocicpp_bind_size == 0) {
            ocicpp_bind_size = bind_bufs.size();
        }
        if (ocicpp_def_size == 0) {
            ocicpp_def_size = sel_bufs.size();
        }
    }
    void resize_vectors(DefList_t  & sel_bufs);
    virtual    int exec() =0;
    virtual    void error(const char* nick, const char* file, int line)=0;
    virtual     void close()=0;
    virtual     int mOexfet(int count)=0;
    virtual         int mOEXfet( int count)=0;

    virtual     int mOfen( int count)=0;

    virtual     std::optional<size_t> field_size(size_t n) = 0;
    virtual     int definps( int n, char *Ptr,  int type_size, int type, indicator *ind, unsigned short *bufcurlen,
            ub2 *rcode,int data_skip, int skip,int curlen)=0;

    virtual    int bindps(char const * plh, int plh_len,
            char *Ptr, int ptr_size, int size3, int type, indicator *ind,
            int skip, unsigned short *bufcurlen, int size4, ub2* rcode, ub4 maxarrlen,ub4 * curarrlen) =0;
    virtual bool is_8_select() const { return false; }
    int mode() const {  return sess_->mode(); }

#ifndef XP_TESTING
    protected:
#endif
    OciSession* sess() const {  return sess_;  }
};
void OracleData::throwException()
{
    if(this->sess()->getDieOnError())
    {
        LogError(STDLOG) << "[" << this->sess()->getConnectString() << "] die on error - set session dead";
        this->sess()->setDead();
    }
    if(err() == CERR_U_CONSTRAINT)
        throw UniqConstrException(error_text2(), sqltext());
    throw ociexception(error_text2(), err(), std::string(sqltext()));
}
namespace {
    int ext_bind_size;
    int ext_def_size;
    int resizeDebug=0;
}
void OracleData::resize_vectors(DefList_t  & sel_bufs)
{

    if (ocicpp_bind_size != 0) {
        if (ocicpp_bind_size > 1000) {
            abort();
        }
    }
    if (ocicpp_def_size != 0) {
        if (ocicpp_def_size > 1000) {
            abort();
        }
        sel_bufs.reserve(ocicpp_def_size);
    }
    if (resizeDebug) {
        ext_bind_size = ocicpp_bind_size;
        ext_def_size = ocicpp_def_size;
    }
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

struct CursorCache
{
    std::shared_ptr<OracleData> find(const char* sql) {
        CursorCacheMap::iterator it = cursors_.find(sql);
        if (it != cursors_.end()) {
            return it->second;
        } else {
            return std::shared_ptr<OracleData>();
        }
    }
    void insert(std::shared_ptr<OracleData> odata) {
        cursors_.emplace(odata->sqltext(), odata);
    }
    void remove(const char* sql) {
        if(cursors_.find(sql)!=cursors_.end()){
            cursors_.erase(sql);
        } else {
            Logger::getTracer().ProgError(STDLOG, "CursorCache: %s",sql);
        }
    }
#ifdef BOOST_UNORDERED_MAP
typedef boost::unordered_map<const  char *,std::shared_ptr<OracleData>  , boost::hash<std::string> , HelpCpp::cstring_eq>
    CursorCacheMap;
#else /* BOOST_UNORDERED_MAP */
typedef std::map<const  char *, std::shared_ptr<OracleData>, HelpCpp::cstring_less >
    CursorCacheMap;
#endif /* BOOST_UNORDERED_MAP */
    CursorCacheMap cursors_;
};

CursCtl make_curs_no_cache_(std::string const &sql, OciCpp::OciSession* sess, const char* nick_, const char* file_, int line_)
{
    if (!sess)
    {
        abort();
    }

    return CursCtl(sess->cdaCursor(sql.c_str(), false), nick_, file_, line_);
}

CursCtl make_curs_no_cache_(std::string const &sql, const char* nick_, const char* file_, int line_)
{
    return make_curs_no_cache_(sql, &mainSession(), nick_, file_, line_);
}

char const * ociexception::what() const noexcept
{
    return what_str.c_str();

}

UniqConstrException::UniqConstrException(std::string const& msg, std::string const &err_msg)
    : ociexception(msg, CERR_U_CONSTRAINT, err_msg) {}

void  pragma_autonomous_transaction(std::string &at1,std::string &at2)
{
  at1="PRAGMA AUTONOMOUS_TRANSACTION;\n";
  at2="commit;\n";
#ifdef XP_TESTING
  if (inTestMode()) {
    at1="";
    at2="";
  }
#endif // XP_TESTING
}

static inline bool findInt(std::vector<int> const& v, int val)
{
    return std::find(v.begin(), v.end(), val) != v.end();
}

void SetSessionId(std::string const & sessid,OCIEnv *envhp,OCISvcCtx *svchp, OCIError *errhp, const std::string &connectString)
{
    if(inTestMode() ){
     //   return;
    }

    if(sessid.empty()){
        return;
    }


    OCIStmt *stmthp;
    OCIHandleAlloc(envhp, (void**)&stmthp, OCI_HTYPE_STMT, 0, 0);

    char const *query_str="begin dbms_session.set_identifier(:txt); end;";

    sword err=OCIStmtPrepare(stmthp,errhp, (const OraText *)query_str, strlen(query_str),
            (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT) ;
    if(checkerr8(STDLOG,err,errhp,connectString)){

        sleep(30);
        abort();
    }

    OCIBind *bin1;
    char const *bindvar=":txt";
    err=OCIBindByName(stmthp,&bin1,errhp,(const OraText *)bindvar,(sb4)strlen(bindvar),
            (dvoid*)sessid.c_str(),sessid.size(),SQLT_LNG,0,0,0,0,0,OCI_DEFAULT);
    if(checkerr8(STDLOG,err,errhp,connectString)){
        sleep(30);
        abort();
    }
    err=OCIStmtExecute(svchp,stmthp,errhp,1,0,0,0,OCI_COMMIT_ON_SUCCESS);
    if(checkerr8(STDLOG,err,errhp,connectString)){
        sleep(30);
        abort();
    }
    err = OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    if (err == OCI_INVALID_HANDLE)
    {
        ProgError(STDLOG, "Unable to free invalid statement handle");
    }
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

class Oracle7Data;

template <class Query> cda_text* _newCursor(OciSession &sess, Query&& query);

class Oracle7Data : public OracleData  {
    template<class Query> friend cda_text* _newCursor(OciSession &sess, Query&& query);
    cda_text *c;

    public:
    Oracle7Data (OciSession* s, cda_text *t);
    std::string error_text() const override
    {
        return OciCpp::error(c);
    }
    std::string error_text2() const override
    {
        return OciCpp::error2(c);
    }
    int err() const noexcept override;
    int rowcount() override
    {
        return rpc(c);
    }
    char const * sqltext() const noexcept override {
        return c->sqltext;
    }
    void setErrCode(int code) override {
        c->curs.rc = code;
    }
    int exec() override {
        return Oexec(c);
    }
    void error(const char* nick, const char* file, int line)override ;
     void close()override ;
     int mOexfet(int count)override {
         return Oexfet(c, count);
     }
     int mOEXfet( int count)
     override {
         return OEXfet(c, count);
     }

     int mOfen( int count) override {
         return Ofen(c, count);
     }

     std::optional<size_t> field_size(size_t n) override;
     int definps( int n, char *Ptr,  int type_size, int type, indicator *ind, unsigned short *bufcurlen,
             ub2 *rcode,int data_skip, int skip,int curlen)override {
         return odefinps(&c->curs, 1, n, (ub1*)Ptr,
                type_size, type, 0, ind,
                (text*)0, 0, 0, bufcurlen, rcode, data_skip,
                skip, curlen, 0);
     }
    int bindps(char const * plh, int plh_len,
                 char *Ptr, int ptr_size, int size3, int type, indicator *ind,
                 int skip, unsigned short *bufcurlen, int size4, ub2* rcode, ub4 maxarrlen,ub4 * curarrlen) override {
        return Lbindps3(c, plh, plh_len, Ptr, ptr_size, size3, type, ind, skip, bufcurlen, size4,
                rcode, (int)sizeof(ub2),maxarrlen,curarrlen);
    }
    ~Oracle7Data();
};

template <class Query> cda_text* _newCursor(OciSession &sess, Query&& query)
{
    sess.set7();
    auto odata7 = std::dynamic_pointer_cast<Oracle7Data>(sess.cdaCursor(std::forward<Query>(query), true));
    if (alwaysOCI8()) {
        sess.set8();
    }
    return odata7->c;
}
cda_text * newCursor(OciSession &sess, std::string s)
{
    return _newCursor(sess, std::move(s));
}
cda_text * newCursor(OciSession &sess,const char *s)
{
    return _newCursor(sess, s);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

class Oracle8Data : public OracleData
{
    std::string sql_text;
    OCIStmt* stmthp = nullptr;
    mutable int errcode = 0;
    mutable char err_buf[8192];
    std::vector<sb4> sizes;
    bool select = false;
    bool first = true;

    int mOexfet(int count, ub4 mode);
    void checkError(const char* n, const char* f, int l,sb4  code);
    void init();
  public:
    virtual void throwException() override;
    virtual void exec_prepare(int) override;
    Oracle8Data (OciSession *sess_, std::string const & sql);
    Oracle8Data (OciSession *sess_, std::string&& sql);
    std::string error(int *err) const;
    std::string error_text() const override;
    std::string error_text2() const override;
    int err() const noexcept override;
    int err8() const noexcept override { return err(); }
    int rowcount() override ;
    char const * sqltext() const noexcept override ;
    void setErrCode(int code) override ;
    int exec() override ;
    void error( const char* nick, const char* file , int line) override;
    void close() override {}
    int mOexfet(int count) override;
    int mOEXfet(int count) override;
    int mOfen( int count) override;
    std::optional<size_t> field_size(size_t n) override;
    int definps( int n, char *Ptr,  int type_size, int type, indicator *ind, unsigned short *bufcurlen,
            ub2 *rcode,int data_skip, int skip,int curlen) override;
    int bindps(char const * plh, int plh_len,
            char *Ptr, int ptr_size, int size3, int type, indicator *ind,
            int skip, unsigned short *bufcurlen, int size4, ub2* rcode, ub4 maxarrlen,ub4 * curarrlen) override;
    bool is_8_select() const override { return select; }
    virtual ~Oracle8Data();
};

static bool isUserDefinedError(const int error)
{
    return error >= 20000 && error <= 20999;
}

void Oracle8Data::throwException()
{
    if(err()!=1403 && err()!=1405 && err()!=!1422 && !isUserDefinedError(err())){
            sess()->removeFromCache(sqltext());
    }
    OracleData::throwException();
}

void Oracle8Data::exec_prepare(int n){
    if(not select)
        return;
    if(first){
        sizes.resize(n, -1);
        const auto status = OCIStmtExecute( sess()->native_.svchp,
                                            stmthp,sess()->native_.errhp,
                                            0, 0, 0, 0, OCI_DEFAULT );
        if(status == OCI_SUCCESS_WITH_INFO or status == OCI_SUCCESS)
            errcode = 0;
        else
        {
            error(&errcode);
            return;
        }
        first = false;
    }
    OCIDefine *defp;
    static char tmp[10] = { 0 };
    checkError(STDLOG, OCIDefineByPos ( stmthp,
                &defp, sess()->native_.errhp, 1, tmp, 9, SQLT_STR,
                0, 0,0, OCI_DEFAULT ));
}

Oracle8Data::Oracle8Data (OciSession *sess_, std::string&& sql)
    : OracleData(sess_) , sql_text(std::move(sql))
{
    init();
}

Oracle8Data::Oracle8Data (OciSession *sess_,std::string const & sql)
    : OracleData(sess_) , sql_text(sql)
{
    init();
}

void Oracle8Data::init()
{
    //LogTrace(TRACE1) << "oci 8 "<<sql;
    const auto status = OCIStmtPrepare2(sess()->native_.svchp, &stmthp, sess()->native_.errhp,
            reinterpret_cast<const OraText*>(sql_text.c_str()), sql_text.size(), // query
            reinterpret_cast<const OraText*>(sql_text.c_str()), sql_text.size(), // cache key
            OCI_NTV_SYNTAX, OCI_DEFAULT);

    if(status == OCI_SUCCESS_WITH_INFO or status == OCI_SUCCESS)
        errcode = 0;
    else
        checkError(STDLOG, status);

    ub4 stmttype=0;
    checkError(STDLOG,OCIAttrGet((void *)stmthp, OCI_HTYPE_STMT, (void *)&stmttype,
                      (ub4 *)0, OCI_ATTR_STMT_TYPE, sess()->native_.errhp));
    if(OCI_STMT_SELECT==stmttype){
        select=true;
    }
//    checkError(STDLOG, OCIAttrGet(stmthp, OCI_HTYPE_STMT, &stmttype, nullptr, OCI_ATTR_PARAM_COUNT, sess()->native_.errhp));
//    OracleData::ocicpp_def_size = stmttype;
}
std::string Oracle8Data::error(int *errcode) const
{
    char* ptr = err_buf;
    sword status = OCIErrorGet(sess()->native_.errhp, 1, nullptr, errcode,
                               reinterpret_cast<OraText*>(ptr), sizeof(err_buf),
                               OCI_HTYPE_ERROR);

    for(ub4 recordno = 2; status == OCI_SUCCESS; recordno++)
    {
        sb4 errcode_loc;
        ptr += strlen(ptr);
        status = OCIErrorGet(sess()->native_.errhp, recordno, nullptr, &errcode_loc,
                             reinterpret_cast<OraText*>(ptr), sizeof(err_buf) - (ptr - err_buf),
                             OCI_HTYPE_ERROR);
    }
    return err_buf;
}
std::string Oracle8Data::error_text() const
{
    return err_buf;
}
std::string Oracle8Data::error_text2() const
{
    return err_buf;
}
int Oracle8Data::err() const noexcept
{
    if(errcode == CERR_NOT_CONNECTED or errcode == CERR_NOT_LOGGED_ON)
        sess()->setDead();
    return errcode;
}
int Oracle8Data::rowcount()
{
    ub4 rows=0;
#ifdef XP_TESTING
//    LogTrace(TRACE1)<<__PRETTY_FUNCTION__<<' '<<rows;
#endif
    int saved_error=errcode;
    checkError(STDLOG, OCIAttrGet((void *)stmthp, OCI_HTYPE_STMT, (void *)&rows,
                      (ub4 *)0, OCI_ATTR_ROW_COUNT, sess()->native_.errhp));
#ifdef XP_TESTING
//    LogTrace(TRACE1)<<__PRETTY_FUNCTION__<<" saved_error="<<saved_error<<" errcode="<<errcode<<" rows="<<rows;
#endif
    errcode=saved_error;
    return rows;
}
char const * Oracle8Data::sqltext() const noexcept
{
    return sql_text.c_str();
}
void Oracle8Data::setErrCode(int code)
{
    errcode = code;
    snprintf(err_buf, sizeof err_buf, "error code manually set to %i\n", code);
}
void Oracle8Data::error(const char* nick,
        const char*  file , int line)
{
    Logger::getTracer().ProgError(nick,file,line,"[%s] %.4000s\n%s ",
            sess()->getConnectString().c_str(),error_text().c_str(),sql_text.c_str());
}

int Oracle8Data::mOexfet(int count, ub4 mode)
{
    const auto status = OCIStmtExecute( sess()->native_.svchp,
                                        stmthp,sess()->native_.errhp,// 1 for non select !
                                        count, 0, 0, 0, mode );
    if(status != OCI_SUCCESS and status != OCI_SUCCESS_WITH_INFO)
    {
        error(&errcode);
        return errcode;
    }
    return errcode = 0;
}

int Oracle8Data::mOexfet(int count)
{
    return this->mOexfet(count, OCI_DEFAULT);
}

int Oracle8Data::mOEXfet(int count)
{
    return this->mOexfet(count, OCI_EXACT_FETCH);
}

int Oracle8Data::exec()
{
    return this->mOexfet(select ? 0 : 1); // 1 for non select !
}
int Oracle8Data::mOfen( int count)
{
   sword res=OCIStmtFetch2 ( stmthp,
                      sess()->native_.errhp,
                      count,
                      OCI_FETCH_NEXT, 0,
                      OCI_DEFAULT );
    if(res!=OCI_SUCCESS && res!=OCI_SUCCESS_WITH_INFO){
        error(&errcode);
        return errcode;
    }
    return 0;
}

std::optional<size_t> Oracle8Data::field_size(size_t n)
{
    //Logger::getTracer().ProgTrace(TRACE1, "%s : n=%zu, select=%s first=%s", __FUNCTION__, n, select ? "true" : "false", first ? "true" : "false");
    if(select){
        auto& sz = sizes.at(n - 1);
        if(-1 == sz){
            dvoid* mypard = nullptr;
            checkError(STDLOG, OCIParamGet(stmthp, OCI_HTYPE_STMT, sess()->native_.errhp, &mypard, n));

            checkError(STDLOG, OCIAttrGet(mypard, OCI_DTYPE_PARAM, &sz, nullptr,
                                          OCI_ATTR_DATA_TYPE, sess()->native_.errhp));
            if(sz == SQLT_RDD)
                sz = OCI_ROWID_LEN;
            else
            {
                checkError(STDLOG, OCIAttrGet(mypard, OCI_DTYPE_PARAM, &sz, nullptr,
                                              OCI_ATTR_DATA_SIZE, sess()->native_.errhp));
                sb4 data_size = sz;
                checkError(STDLOG, OCIAttrGet(mypard, OCI_DTYPE_PARAM, &sz, nullptr,
                                              OCI_ATTR_DISP_SIZE, sess()->native_.errhp));
                sb4 char_size = sz;
                //Logger::getTracer().ProgTrace(TRACE1, "%s : data_size=%i, char_size=%i, type=%i", __FUNCTION__, data_size, char_size, sz);
                sz = std::max(data_size, char_size);
            }
            OCIDescriptorFree((void*)mypard, OCI_DTYPE_PARAM);
        }
        return sz;
    }
    return {};
}

int Oracle8Data::definps( int n, char *Ptr,  int type_size, int type, indicator *ind, unsigned short *bufcurlen,
        ub2 *rcode,int data_skip, int skip,int curlen){

    if(!select){
        sess()->removeFromCache(sqltext());
        throw ociexception( "not a select:"+sql_text);
    }

    OCIDefine *defp;
    checkError(STDLOG, OCIDefineByPos ( stmthp,
                &defp, sess()->native_.errhp, n, Ptr, type_size, type,
                ind, bufcurlen, rcode, OCI_DEFAULT ));


    checkError(STDLOG,OCIDefineArrayOfStruct ( defp,
                sess()->native_.errhp,
                data_skip,
                skip,
                data_skip,
                sizeof(ub2)));

    return 0;
}
int Oracle8Data::bindps(char const * plh, int plh_len,
        char *Ptr, int ptr_size, int size3, int type, indicator *ind,
        int skip, unsigned short *bufcurlen, int size4, ub2* rcode, ub4 maxarrlen,ub4 * curarrlen) {

    OCIBind       *bindpp=0;
    checkError(STDLOG,OCIBindByName ( stmthp,
                 &bindpp,
                 sess()->native_.errhp,
                 (OraText const *)plh,plh_len,
                 Ptr,
                 ptr_size,
                 type,
                 ind,
                 bufcurlen,
                 rcode,
                 maxarrlen,
                 curarrlen,
                 OCI_DEFAULT ));

    checkError(STDLOG,OCIBindArrayOfStruct ( bindpp,
                sess()->native_.errhp,
                size3,
                skip,
                size3,
                sizeof(ub2)));
    return 0;
}

Oracle8Data::~Oracle8Data()
{
    OCIStmtRelease(stmthp,sess()->native_.errhp,
        reinterpret_cast<const OraText*>(sql_text.c_str()), sql_text.size(), // cache key
        OCI_STRLS_CACHE_DELETE/*OCI_DEFAULT*/);
}


void Oracle8Data::checkError(const char* n, const char* f, int l,sb4  code)
{
    errcode = 0;
    if(code != OCI_SUCCESS) {
        LogWarning(STDLOG)<<"OCI_XX status is "<< code;
//        string msg=string(n)+":"+f+":"+std::to_string(l)+":"+error_text();
        if(code == OCI_SUCCESS_WITH_INFO){
            LogWarning(n,f,l) << this->error(&errcode);
            assert(errcode == 0);
        //    errcode=OCI_SUCCESS;
        } else {
            LogError(n,f,l) << this->error(&errcode);
            sess()->removeFromCache(sqltext());
            throwException();
        }
    }
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

OciSession::OciSessionNativeData::OciSessionNativeData():envhp(NULL), svchp(NULL), errhp(NULL), srvhp(NULL), usrhp(NULL)
{
}

void setClientInfo(OCIEnv *envhp, OCIError *errhp, OCISvcCtx *svchp,
                   const std::string &connectString,
                   const std::string &clientInfo)
{
    OCIStmt *stmthp;
    OCIHandleAlloc(envhp, (void**)&stmthp, OCI_HTYPE_STMT, 0, 0);

    char const *query_str="BEGIN DBMS_APPLICATION_INFO.SET_CLIENT_INFO(:clientInfo); END;";

    sword err=OCIStmtPrepare(stmthp,errhp, (const OraText *)query_str, strlen(query_str),
            (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT) ;
    if(checkerr8(STDLOG,err,errhp,connectString))
    {
        sleep(30);
        abort();
    }

    OCIBind *bin1;
    char const *bindvar=":clientInfo";
    err=OCIBindByName(stmthp,&bin1,errhp,(const OraText *)bindvar,(sb4)strlen(bindvar),
            (dvoid*)clientInfo.c_str(),clientInfo.size(),SQLT_LNG,0,0,0,0,0,OCI_DEFAULT);
    if(checkerr8(STDLOG,err,errhp,connectString))
    {
        sleep(30);
        abort();
    }
    err=OCIStmtExecute(svchp,stmthp,errhp,1,0,0,0,OCI_COMMIT_ON_SUCCESS);
    if(checkerr8(STDLOG,err,errhp,connectString)){
        sleep(30);
        abort();
    }
    err = OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    if (err == OCI_INVALID_HANDLE)
    {
        ProgError(STDLOG, "Unable to free invalid statement handle");
    }
}


void OciSession::setClientInfo(const std::string &clientInfo)
{
  OciCpp::setClientInfo(native_.envhp, native_.errhp, native_.svchp, getConnectString(), clientInfo);
  if (inTestMode()) // restore savepoint because OCI_COMMIT_ON_SUCCESS in setClientInfo()
  {
    const char* sql = "SAVEPOINT SP_XP_TESTING";
    LogTrace(TRACE3) << sql << " for " << getConnectString();
    auto odata = cdaCursor(sql,false);
    if (odata->exec())
    {
        fprintf(stderr, "SAVEPOINT SP_XP_TESTING failed");
        abort();
    }
  }

}

void OciSession::free_handlers()
{
  delete cache_;
  cache_=NULL;

 //  if (native_.svchp) OCIHandleFree(native_.svchp, OCI_HTYPE_SVCCTX ); //  cleared by OCILogoff ??
  if (native_.errhp) OCIHandleFree(native_.errhp, OCI_HTYPE_ERROR ); // cleared by OCIHandleFree(native_.envhp, OCI_HTYPE_ENV   )
  if (native_.envhp) OCIHandleFree(native_.envhp, OCI_HTYPE_ENV   );

  native_ =  OciSessionNativeData();
}

OciSession::OciSession(const char* nick, const char* file, int line, const std::string& connStr)
    : mode_(8), cache_(NULL),  nick_(nick), file_(file), line_(line), is_alive_(false)
{
    ProgTrace(TRACE0, "start OciSession: %s %s:%d", nick_, file_, line_);
    split_connect_string(connStr, connectionParams_);
    init();
    setClientInfo(std::string(file?file:"")+":"+std::to_string(line));
}

void OciSession::reconnect()
{
    int cur_mode = mode_;
    clear();
    init();
    if (cur_mode == 7)
    {
       set7();
    }
}

void OciSession::setDead()
{
    is_alive_ = false;
}

bool OciSession::is_alive() const
{
    return is_alive_;
}

bool oci8_set_cache_size(OCISvcCtx *svchp,OCIError *errhp)
{
  // Increase cache size (ora - 20 by default, we need more)
  int need_cache_size=2000;
  int cache_size=0;
  bool res=true;
  int status=0;
  if(res && (status = OCIAttrGet(svchp, OCI_HTYPE_SVCCTX, &cache_size, 0, OCI_ATTR_STMTCACHESIZE, errhp)))
  {
      LogError(STDLOG)<<"Ошибка получения атрибута";
      res=false;
  }
  if (res && need_cache_size>cache_size)
  {
    LogTrace(TRACE5)<<"cache_size="<<cache_size;
    if((status = OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, &need_cache_size, 0, OCI_ATTR_STMTCACHESIZE, errhp)))
    {
        LogError(STDLOG)<<"Ошибка установки атрибута";
        res=false;
    }
  }
  return res;
}

void OciSession::init()
{
    sb4 envFlags = OCI_THREADED | OCI_OBJECT | OCI_EVENTS;
    OCIEnvCreate(&native_.envhp, envFlags, 0, 0, 0, 0, 0, 0);
    OCIHandleAlloc(native_.envhp, (void **)&native_.errhp, OCI_HTYPE_ERROR, 0, 0);

    if (OCILogon2(native_.envhp, native_.errhp, &native_.svchp,
                (OraText const*)connectionParams_.login.c_str(), connectionParams_.login.size(),
                (OraText const*)connectionParams_.password.c_str(), connectionParams_.password.size(),
                (OraText const*)connectionParams_.server.c_str(), connectionParams_.server.size(),
                OCI_LOGON2_STMTCACHE))
    {
        LogError(STDLOG) << "[" << getConnectString() << "] " << getError();
        ociexception e(getError());
        free_handlers();
        throw e;
    }
    if (!oci8_set_cache_size(native_.svchp,native_.errhp))
    {
        LogError(STDLOG) << "[" << getConnectString() << "] " << getError() << " (oci8_set_cache_size)";
        ociexception e(getError());
        free_handlers();
        throw e;
    }

    cache_=new CursorCache;

    std::string sessid(tclmonFullProcessName());
    size_t lpos=sessid.find_last_not_of("0123456789");
    if(lpos!=std::string::npos){
        sessid=std::string(sessid,0,lpos+1);

        if(sessid.empty()){
            sleep(30);
            abort();
        }

        if(*sessid.rbegin()!=' '){
            sessid=tclmonFullProcessName();
        }else{
             lpos=sessid.find_last_not_of(" ");
             sessid=std::string(sessid,0,lpos+1);
             assert (*sessid.rbegin()!=' ');
        }

    }
    SetSessionId(sessid,native_.envhp,native_.svchp,native_.errhp,getConnectString());

#ifdef XP_TESTING
    commit(); // create savepoint for new session
    set8();
#endif // XP_TESTING
    is_alive_ = true;
}

void OciSession::clear()
{
    if (cache_)
    {
        cache_->cursors_.clear();
    }
    allSessions.erase(lda_);
    logoff();
    free_handlers();
}

OciSession::~OciSession()
{
    clear();
}

std::string OciSession::getError() const
{
    char s[10000];
    int errcode;

    OCIErrorGet(native_.errhp, 1, 0, &errcode, (OraText*)s, sizeof(s), OCI_HTYPE_ERROR);
    return s;
}

void OciSession::set7()
{
    if(abort7){
        abort();
    }
    if (mode_ != 7)
    {
        mode_ = 7;
        if (OCISvcCtxToLda(native_.svchp, native_.errhp, lda_))
        {
            throw comtech::Exception(std::string("oi  ") + getError() + " oi!");
        }
        allSessions[lda_] = this;
    }
}

void OciSession::set8()
{
    if (mode_ != 8)
    {
        mode_ = 8;
        allSessions.erase(lda_);
        OCILdaToSvcCtx(&native_.svchp, native_.errhp, lda_);
    }
}

void OciSession::logoff()
{
    set8();
    rollbackInTestMode();
    OCILogoff(native_.svchp, native_.errhp);
    /*
    return;
    set7();
    if (orol(lda_))
    {
        lda_error(lda_);
    }
    else
    {
        set8();
        OCILogoff(native_.svchp, native_.errhp);
    }
    */
}

void OciSession::commit()
{
    if(alwaysOCI8()){
        set8();
    }

#ifdef XP_TESTING
    if (inTestMode())
    {
        const char* sql = "SAVEPOINT SP_XP_TESTING";
        LogTrace(TRACE3) << sql << " for " << getConnectString();
        auto odata = cdaCursor(sql,false);
        if (odata->exec())
        {
            fprintf(stderr, "SAVEPOINT SP_XP_TESTING failed");
            abort();
        }
        return;
    }
#endif // XP_TESTING
    LogTrace(TRACE1) << "true commit for " << getConnectString();
    sword res = OCITransCommit(native_.svchp,
                               native_.errhp,
                               OCI_DEFAULT);
    if (checkerr8(STDLOG,res,native_.errhp,getConnectString()))
    {
        throw comtech::Exception(std::string("commit failed for ") + getConnectString());
    }
}

void OciSession::commitInTestMode()
{
    if(alwaysOCI8()){
        set8();
    }
    LogTrace(TRACE1) << "true commit";
     sword res=OCITransCommit ( native_.svchp,
                       native_.errhp,
                       OCI_DEFAULT );
    if (res!=OCI_SUCCESS && res!=OCI_SUCCESS_WITH_INFO)
    {
        throw comtech::Exception("commit failed");
    }
}

void OciSession::rollback()
{
    if(alwaysOCI8()){
        set8();
    }
#ifdef XP_TESTING
    if (inTestMode())
    {
        const char* sql = "ROLLBACK TO SAVEPOINT SP_XP_TESTING";
        LogTrace(TRACE3) << sql << " for " << getConnectString();
        auto odata = cdaCursor(sql,false);
        if (odata->exec())
        {
            LogError (STDLOG) << odata->error_text()<<"\n" << sql;
            fprintf(stderr, "ROLLBACK TO SAVEPOINT SP_XP_TESTING failed\n");
            abort();
        }
        return;
    }
#endif // XP_TESTING
    LogTrace(TRACE1) << "true rollback";
    sword res=OCITransRollback ( native_.svchp, native_.errhp,OCI_DEFAULT);

    if(res!=OCI_SUCCESS && res!=OCI_SUCCESS_WITH_INFO)
    {
        throw comtech::Exception("rollback failed");
    }
}

void OciSession::rollbackInTestMode()
{
    if(alwaysOCI8()){
        set8();
    }
    sword res=OCITransRollback ( native_.svchp, native_.errhp,OCI_DEFAULT);
    // do not throw exception in case of rollback failed:
    // this function is called only on close oci session (destruct or reconnect)
    checkerr8(STDLOG, res, native_.errhp, getConnectString());
}

std::shared_ptr<OracleData> OciSession::cdaCursor(std::string sql, bool cacheit)
{
    auto odata = cacheit ? cache_->find(sql.c_str()) : std::shared_ptr<OracleData>();
    if (!odata)
    {
        if (mode_==7)
        {
            if(auto cda=getStaticCursor(sql.c_str(),getLd(),*this))
                odata = std::make_shared<Oracle7Data>(this,cda);
            else
            {
                LogError(STDLOG) << "[" << getConnectString() << "] getStaticCursor failed - set session dead";
                setDead();
                throw ociexception(__FUNCTION__ + std::string(" : getStaticCursor failed"));
            }
        }
        else
        {
            odata = std::make_shared<Oracle8Data>(this,std::move(sql));
        }

        if(cacheit){
            cache_->insert(odata);
        }
    }
    return odata;
}

std::shared_ptr<OracleData> OciSession::cdaCursor(const char* sql, bool cacheit)
{
    auto odata = cacheit ? cache_->find(sql) : std::shared_ptr<OracleData>();
    if (!odata)
    {
        if (mode_==7)
        {
            if(auto cda=getStaticCursor(sql,getLd(),*this))
                odata = std::make_shared<Oracle7Data>(this,cda);
            else
            {
                LogError(STDLOG) << "[" << getConnectString() << "] getStaticCursor failed - set session dead";
                setDead();
                throw ociexception(__FUNCTION__ + std::string(" : getStaticCursor failed"));
            }
        }
        else
            odata = std::make_shared<Oracle8Data>(this,sql);

        if(cacheit){
            cache_->insert(odata);
        }
    }
    return odata;
}

void OciSession::removeFromCache(char const * sql_text)
{
    // don't check result of oclose
    // it seems to be broken
    // after ocan we receive here 'not logged on' error
    // on the other hand without ocan we receive
    // user request cancelled
    cache_->remove(sql_text);
}

CursCtl OciSession::createCursor(STDLOG_SIGNATURE, const char *sql)
{
    return CursCtl(cdaCursor(sql,true), nick, file, line);
}

CursCtl OciSession::createCursor(STDLOG_SIGNATURE, const std::string& sql)
{
    return createCursor(nick, file, line, sql.c_str());
}

std::string OciSession::getConnectString() const
{
    return make_connect_string(connectionParams_);
}

bool OciSession::getDieOnError() const
{
    return dieOnError_;
}

void OciSession::setDieOnError(const bool value)
{
    dieOnError_ = value;
}

static std::shared_ptr<OciSession> mainSessPtr;

void setMainSession(const std::shared_ptr<OciSession>& session)
{
    mainSessPtr = session;
}

static void openGlobalCursors()
{
    if(not mainSessPtr)
        throw comtech::Exception("not mainSessPtr!");
    mainSessPtr->set7();
    static bool isOpen = false;
    if (isOpen) {
        for (int i = 0; i < ncurs; ++i) {
            oclose(&CU[i].curs);
        }
        isOpen = false;
    }
    if (!isOpen) {
        for (int i=0; i < ncurs; ++i) {
            if (OpenOciCursor(CU + i, mainSession().getLd())) {
                std::cerr << __FILE__ << ":" << __LINE__ << " oopen failed" << std::endl;
                for (i--; i>=0; i--)
                    oclose(&CU[i].curs);
                throw comtech::Exception("failed connect to Oracle");
            }
        }
        isOpen = true;
    }
    if(alwaysOCI8()){
        mainSessPtr->set8();
    }
}

std::shared_ptr<OciSession> createMainSession(STDLOG_SIGNATURE, const char *str, bool open_global_cursors)
{
    try {
      mainSessPtr = std::make_shared<OciSession>(STDLOG_VARIABLE, str);
    }catch (...){
    LogTrace(TRACE1)<<"Create Main session exception";

    }
    if(mainSessPtr and open_global_cursors)
        openGlobalCursors();
    return mainSessPtr;
}

std::shared_ptr<OciSession> createMainSession(const char *str, bool open_global_cursors)
{
  return createMainSession(STDLOG,str,open_global_cursors);
}

std::shared_ptr<OciSession> mainSessionPtr()
{
    return mainSessPtr;
}

OciSession& mainSession()
{
    return *mainSessPtr;
}

OciSession* pmainSession()
{
    return mainSessPtr.get();
}


struct cursor_deleter
{
  void operator () ( std::pair< const std::string , cda_text * > & pos )
  {
    oclose( &pos.second->curs );
    free( const_cast<char*>(pos.second->sqltext) );
    free( pos.second );
  }
};
std::string error2(cda_text *x)
{
    char s[8000];
    oerhms(x->ld,x->curs.rc,(OraText*)s,(sword)sizeof(s));
    return std::string(s);
}
std::string error(cda_text *x)
{
    char s[8000]={0};
    oerhms(x->ld,x->curs.rc,(OraText*)s,(sword)sizeof(s));
    return std::string(s)+ " " +(x->sqltext ? x->sqltext : "");
}

void error(cda_text * x, OciSession const &session,
        const char* nick, const char* file, int line)
{
    ocierror(x->ld,x->curs.rc,x->curs.fc,x->curs.peo,line,x->sqltext,
             session.getConnectString().c_str(),file,nick);
}

#ifdef exec
#error exec defined
#endif

const char* CursCtl::__get_ph(const char* pl)
{
    if(char const *ppl = strstr(oracledata->sqltext(), pl))
        return ppl;
    oracledata->setErrCode(CERR_BIND);
    throwException(ex_id()+":no such bind variable: "+pl);
    return nullptr;
}
void CursCtl::dump()
{
    Logger::getTracer().ProgTrace(TRACE1, "%s:%s:%d i=%d", i_, nick, file, line);
}
void CursCtl::save_vector_sizes()
{
    this->oracledata->save_vector_sizes(bind_bufs,sel_bufs);
}
void CursCtl::resize_vectors()
{
    if(bind_bufs.empty())
    {
        unsigned colons = 0;
        const char* query = oracledata->sqltext();
        for(const char* c = query; (c = strchr(c, ':')); c++)
            colons++;
        bind_bufs.reserve(colons);
        if(isDebug())
            Logger::getTracer().ProgTrace(TRACE1, "CursCtl::%s reserve %u for bind_bufs", __func__, colons);
    }
    this->oracledata->resize_vectors(sel_bufs);
}

CursCtl& CursCtl::structSize(int data, int indicator_skip)
{
    data_skipsize=data;
    indicator_skipsize=indicator_skip;
    return *this;
}

int CursCtl::err() const { return oracledata->err(); }

int CursCtl::rowcount() { return oracledata->rowcount(); }

std::string CursCtl::queryString() const { return oracledata->sqltext(); }

//#############################################################################
namespace {
class special_class{};

static bool keepCursor(int err)
{
    static int saneErrors[] =
    {
        CERR_OK,
        CERR_NULL,
        CERR_TRUNC,
        CERR_NODAT,
        CERR_DUPK,
        CERR_EXACT,
        CERR_BUSY,
        CERR_U_CONSTRAINT,
        CERR_I_CONSTRAINT,
        CERR_SNAPSHOT_TOO_OLD,
        CERR_TOO_MANY_ROWS,
        CERR_INVALID_NUMBER,
        NO_DATA_FOUND,
        REQUIRED_NEXT_PIECE,
        REQUIRED_NEXT_BUFFER
    };

    const unsigned int count = HelpCpp::array_size(saneErrors);

    //const int UserDefined = 20100;

    if ( isUserDefinedError(err) )   return true;

    if ( std::find(&saneErrors[0], &saneErrors[count], err) != (&saneErrors[count]) )
        return true;

    return false;
}

static void severeErrorHandler(CursCtl *curs,
        const char* nick, const char* file, int line,
        const std::vector<int>& noThrowErrList)
{
    static int severeErrors[] =
    {
        3114,   //not connected to Oracle
    };

    const unsigned int count = HelpCpp::array_size(severeErrors);

    if (std::find(&severeErrors[0], &severeErrors[count], curs->err()) != (&severeErrors[count]) )
    {
        ProgTrace(TRACE0, "Critical error occured: %s!", curs->error_text().c_str() );
        sleep(5);
        exit(1);
    }


    if (!findInt(noThrowErrList, curs->err()))
    {
        if (curs->err() == CERR_DEADLOCK)
        {
            LogError(STDLOG) << "Deadlock: " << curs->error_text();
            curs->error(nick, file, line);
            throw DeadlockException(curs->error_text());
        }
        curs->error( nick, file, line);
        curs->throwException();
    }
}

} // namespace

//---------- default_ptr_t stuff ----------------------------------------

namespace {

template <typename T1, typename T2> struct fit_bit_cast {
    static constexpr bool value = sizeof(T1) == sizeof(T2) and
                                  std::is_trivially_copyable<T1>::value and
                                  std::is_trivially_copyable<T2>::value;
};
template <typename T1, typename T2>
/*inline*/ constexpr bool fit_bit_cast_v = fit_bit_cast<T1, T2>::value;

template <typename T, typename... Types> struct FitsInto
    : std::enable_if<fit_bit_cast_v<T, Types>, Types>... {};

using PtrFitsInto = FitsInto<void*, uint32_t, uint64_t>;

// one day will be replaced with std::bit_cast
template <typename To, typename From>
constexpr std::enable_if_t<fit_bit_cast_v<To, From>, To> _bit_cast(From src) noexcept
{
    To ret = {};
    std::memcpy(&ret, &src, sizeof(To));
    return ret;
}

} // anonymous namespace

void default_ptr_t::set_pod(void const* t_ptr, unsigned sz) noexcept
{
    PtrFitsInto::type l = 0;
    std::memcpy(&l, t_ptr, sz);
    ptr = _bit_cast<concept_t*>((l << 8) | (sz << 1) | 1);
}

void default_ptr_t::fill(void* out) const noexcept
{
    auto l = _bit_cast<PtrFitsInto::type>(ptr);
    if(l & 1)
    {
        unsigned m = l & 0xff;
        l >>= 8;
        std::memcpy(out, &l, m >> 1);
    }
    else if(ptr)
        ptr->fill(out);
}

default_ptr_t::~default_ptr_t()
{
    if(not (_bit_cast<PtrFitsInto::type>(ptr) & 1))
        delete ptr;
}

//-----------------------------------------------------------------------

CursCtl& CursCtl::defFull(void *ptr, int maxlen, short *ind, unsigned short *clen, int type)
{
    sel_bufs.emplace_back(++i_, 0, ptr, maxlen, type, ind, clen, External::pod, default_ptr_t::none(), false, maxlen, 0, nullptr);
    return *this;
}

CursCtl& CursCtl::defRow(BaseRow& r)
{
    r.def(*this);
    return *this;
}

CursCtl& CursCtl::bindFull(const std::string& pl, void *ptr, int maxlen, short *ind, unsigned short *clen, int type, ub4 maxarrlen, ub4* curarrlen)
{
    return bindFull(pl.c_str(), ptr, maxlen, ind, clen, type,maxarrlen,curarrlen);
}

CursCtl& CursCtl::bindFull(const char *pl, void *ptr, int maxlen, short *ind, unsigned short *clen, int type, ub4 maxarrlen, ub4* curarrlen)
{
    // Если такой bind уже есть, то заменяем новый
    bind_bufs.emplace_back(__get_ph(pl), strlen(pl), ptr, maxlen, type, ind, clen,
                           External::pod, default_ptr_t::none(), false, maxlen, maxarrlen, curarrlen);
    return *this;
}

void CursCtl::fetch_start(unsigned count)
{
    if(count > fetch_array_len)
    {
        throw ociexception(ex_id() + " fetch_array_len(" + std::to_string(fetch_array_len) + ") exceed: count==" + std::to_string(count));
    }
}

void CursCtl::exec(const char*)
{
    exec_start();

    if (oracledata->err8())
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
        return;
    }

    this->oracledata->exec();
    const int last_error = oracledata->err();

    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE5, "Execute cursor: %d", last_error);
    }
    if (last_error)
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
    exec_end();
    if (expected_rows != -1)
    {
        if (expected_rows != rowcount())
        {
            std::stringstream msg;
            msg << "expected " << expected_rows << " rows, fetched " << rowcount();
            throwException(msg.str());
        }
    }
}

// FIXME если stable уже включен, запретить unstb в runtime
static bool defaultStableBind()
{
    static bool val = readIntFromTcl("CURSCTL_DEFAULT_STABLE_BIND", 0) == 1;
    return val;
}

CursCtl::CursCtl(std::shared_ptr<OracleData>&& odata, const char* nick_, const char* file_, int line_)
    : oracledata(std::move(odata)),
      nick(nick_), file(file_), line(line_)
{
    init();
}

void CursCtl::init()
{
    stableBind_ = defaultStableBind();
    resize_vectors();
    noThrowError(NO_DATA_FOUND);
    initialSessionMode_ = oracledata->mode();
}

CursCtl::CursCtl(std::string sqls, const char* nick_, const char* file_, int line_)
     : nick(nick_), file(file_), line(line_)
{
    //ProgTrace(TRACE5,"%s(%s, in)", __func__, sqls.c_str());
    if(alwaysOCI8()){
        OciCpp::mainSession().set8();
    }else{
        OciCpp::mainSession().set7();
    }
    oracledata = OciCpp::mainSession().cdaCursor(std::move(sqls),true);
    init();
}

CursCtl::CursCtl(OCI8T, std::string sqls, const char* nick_, const char* file_, int line_)
     : nick(nick_), file(file_), line(line_)
{
    //ProgTrace(TRACE5,"%s(OCI8, string, in)", __func__);
    OciCpp::mainSession().set8();
    oracledata = OciCpp::mainSession().cdaCursor(std::move(sqls),true);
    init();
}

Oracle7Data::Oracle7Data (OciSession* sess_, cda_text *t) : OracleData(sess_), c(t)
{
    if (!t)
      abort();
}

Oracle7Data::~Oracle7Data()
{
    oclose(&c->curs);
    free(const_cast<char*>(c->sqltext));
    free(c);
}
void Oracle7Data::close()
{
     int last_error = cda_err(c);
     ocan(&c->curs);
     if (!keepCursor(last_error))
     {
         innerCloseCurs_7(c);
     }
}
int Oracle7Data::err() const noexcept
{
    const int errcode = cda_err(c);
    if(errcode == CERR_NOT_CONNECTED or errcode == CERR_NOT_LOGGED_ON)
        sess()->setDead();
    return errcode;
}

void Oracle7Data::error(const char* nick, const char* file, int line)
{
    OciCpp::error(c,*sess(),nick,file,line);
}

std::optional<size_t> Oracle7Data::field_size(size_t n)
{
    sb4 dbsize = 0;
    sb4 dpsize = 0;
    sb2 dbtype = 0;

    auto res = odescr(&c->curs, n, &dbsize, &dbtype, nullptr, nullptr, &dpsize, nullptr, nullptr, nullptr);
    //Logger::getTracer().ProgTrace(TRACE1, "%s : n=%zu, dbsize=%i dpsize=%i dbtype=%hi res=%i", __FUNCTION__, n, dbsize, dpsize, dbtype, res);
    if(res)
         return {};
    return std::max(dbsize, dpsize);
}

CursCtl::~CursCtl()
{
    oracledata->close();
}

#define ASSERT_SESSION_MODE_SWITCH(initialMode, currentMode) { \
    if (initialMode != currentMode) { \
        LogError(STDLOG) << "OciSession mode initial: " << initialMode << " is not equal to current: " << currentMode; \
        throw ociexception("session mode changed while cursor execution"); \
    } \
}

int CursCtl::exfet(unsigned count, const char* p)
{
    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::exfet %s", p ? p : ex_id().c_str());
    }
    exec_start();
    if (oracledata->err8())
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
        return oracledata->err();
    }
    fetch_start(count);
    /* if you please...
    for(unsigned i=0;i<sel_bufs.size();++i){
        LogTrace(TRACE1) << "s.type " << sel_bufs[i].type << " s.type_size " << sel_bufs[i].type_size;
        if(!sel_bufs[i].ind){
            LogTrace(TRACE1) << "no ind for "<<i;
        }
    }
    */
    oracledata->mOexfet( count);
    int last_error = err();
    if (last_error)
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
    try {
        fetch_end();
    }catch (special_class &){
        oracledata->setErrCode(1405);
        severeErrorHandler(this,  nick, file, line, noThrowErrList);

    }
    return last_error;
}

int CursCtl::EXfet(unsigned count, const char* p)
{
    if(isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1,"CursCtl::EXfet %s", p ? p : ex_id().c_str());
        Logger::getTracer().ProgTrace(TRACE5,"cursor is: %s", queryString().c_str());
    }
    exec_start();
    if (oracledata->err8())
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
        return oracledata->err();
    }
    fetch_start(count);
    oracledata->mOEXfet(count);
    int last_error = err();
    if (last_error)
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
    try {
        fetch_end();
    }catch (special_class &){
    LogTrace(TRACE1) << "special_class";
        oracledata->setErrCode(1405);
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
//    fetch_end();
    return last_error;
}

int CursCtl::fen(unsigned count)
{
    ASSERT_SESSION_MODE_SWITCH(initialSessionMode_, oracledata->mode());
    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::fen %s", ex_id().c_str());
    }
    fetch_start(count);

    oracledata->mOfen(count);

    int last_error = err();
    if(isDebug()) Logger::getTracer().ProgTrace(TRACE1, "CursCtl::fen last_error = %i", last_error);
    if (last_error)
    {
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
    try {
        fetch_end();
    }catch (special_class &){
        oracledata->setErrCode(1405);
        severeErrorHandler(this,  nick, file, line, noThrowErrList);
    }
    return last_error; //0 or NO_DATA_FOUND
}

int getTypeSize(const char* p, int type_size, int type)
{
    if (type_size == -1)
    {
        if (type == SQLT_STR)
        {
            return (int)strlen(p) + 1;
        }
        else
        {
            throw ociexception("not sqlt_str");
        }
    }
    return type_size;
}

template <typename T>
void fetch_end_struct<T>::operator() (sel_data<T> &s)
{
    if (s.bind_in)
    {
        return;
    }
    std::vector<const char*> input_ptr(rp, nullptr);
    for (unsigned i = 0; i < rp; ++i)
    {
        if(s.ind==0 && s.rcode.at(i)==1405 && c.oracledata->is_8_select()){
            throw special_class();
        }
    }
    int skip_size = c.data_skipsize;
    if (skip_size == 0)
    {
        skip_size = s.external_skip_size;
    }
    char * output_ptr = reinterpret_cast<char*>(s.value_addr);
    if (s.ind)
    {
        for (unsigned i = 0; i < rp; ++i)
        {
            if (s.ind[i] == inull)
            {
                if (s.default_value)
                {
                    s.default_value.fill(output_ptr + i * skip_size);
                }
                else if (s.curlen)
                {
                    s.curlen[i]=0;
                }
            }
            else if (s.ind[i] >= itruncate)
            {
               /// ProgError( STDLOG , "Achtung truncate !!! %d , %d" , s.n , i  );
               /// throw ociexception(c.ex_id()+
               //             "value too large for buffer");
            }
            else if (s.ind[i] == iok && s.data_type != External::pod)
            {
                if (s.data)
                {
                    input_ptr[i] = s.data.get() + i * s.type_size;
                }
            }
        }
    }
    for (unsigned i=0; i < rp; ++i, output_ptr += skip_size)
    {
        if(s.from and (!s.ind || s.ind[i] == iok))
        {
            if (!input_ptr[i] && s.data_type != External::pod && s.data)
                input_ptr[i] = s.data.get() + i * s.type_size;
            s.from(output_ptr, input_ptr[i], iok);
        }
    }
}

template<> void sel_data<char const *>::print() const
{
    Logger::getTracer().ProgTrace(TRACE1, "<%.*s> type=%d sz=%d ind=%d data=<%p>",
                                  plh_len, plh, type, type_size, (ind ? *ind : 0),
                                  value_addr);
}

template struct fetch_end_struct<unsigned>;
template struct fetch_end_struct<const char *>;
template struct buf_struct<unsigned>;
template struct buf_struct<const char *>;
template<> void buf_struct<unsigned>::operator() (sel_data<unsigned> &buf)
{
    if (c.isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "buf_struct<int>::operator(%i) %s", buf.plh, c.ex_id().c_str());
    }
    char* Ptr = reinterpret_cast<char *>(buf.value_addr);
    int skip = (buf.ind == 0 ? 0 : std::max(c.indicator_skipsize, int(sizeof(indicator))));
    buf.rcode.resize(c.fetch_array_len);
    if (buf.ind == 0 && buf.default_value)
    {
        if (c.fetch_array_len > 1)
        {
            buf.indv.reset(new indicator[c.fetch_array_len]);
            buf.ind=&(buf.indv[0]);
            skip = sizeof(indicator);
        }
        else
        {
            buf.ind=&buf.indic;
        }
    }
    if (c.isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, " buf.ind is %p skip=%d", buf.ind, skip);
    }
    int data_skip = 0;
    if (buf.data_type == External::string)
    {
        if (c.isDebug())
        {
            Logger::getTracer().ProgTrace(TRACE1, " before c.oracledata->field_size: data_skip=", data_skip);
        }
        if(auto dl = c.oracledata->field_size(buf.plh))
        {
            if (c.isDebug())
                Logger::getTracer().ProgTrace(TRACE1, " field_size: %zu",*dl);

            buf.maxlen = *dl + 1; // trailing \0
            buf.type_size = *dl + 1;
            if (buf.type_size and c.fetch_array_len)
                buf.data.reset(new char[buf.type_size * c.fetch_array_len]);
            Ptr = buf.data.get();
            data_skip = buf.type_size;
        }
        else
        {
            c.oracledata->error(c.nick, c.file, c.line);
            c.throwException("odescr "+ c.error_text());
        }
    }
    else if (buf.data_type == External::pod)
    {
        data_skip = std::max(buf.type_size, int(c.data_skipsize));
    }
    else
    {
        buf.data.reset(new char[buf.type_size * c.fetch_array_len]);
        Ptr = buf.data.get();
        data_skip = buf.type_size;
    }
    int curlen = (buf.curlen == 0 ? 0 : std::max(c.data_skipsize, int(sizeof(sb4))));
    if (c.isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, " data_skip=%d curlen=%d", data_skip, curlen);
        Logger::getTracer().ProgTrace(TRACE1, " Ldefin(pos=%d)", buf.plh);
    }
    if(c.oracledata->definps(buf.plh,Ptr,buf.type_size,buf.type,buf.ind,buf.curlen ,
           &buf.rcode[0],data_skip,skip,curlen) )
    {
        c.oracledata->error(c.nick, c.file, c.line);
        c.throwException("CursCtl::def error");
    }
}

template<>
void sel_data<char const *>::dump(std::ostream& ss) const
{
    char* Ptr;
    if (data_type == External::string)
    {
        Ptr = const_cast<char*>(((std::string*)value_addr)->c_str());
    }
/*    else if(data_type == External::date || data_type == External::time)
    {
        Ptr = reinterpret_cast<char*>(((oracle_datetime*)value_addr)->value);
    }*/
    else
    {
        Ptr = reinterpret_cast<char*>(value_addr);
    }
    ss << std::string(plh, plh_len) << " type=" << type << " sz=" << type_size;
    if(type == 3) {
        ss << std::setw(30-plh_len-6-1-4-1) << std::setfill(' ') << std::right << " value=";
        switch(type_size) {
            case 4: ss << *static_cast<const int32_t*>(value_addr); break;
            case 2: ss << *static_cast<const int16_t*>(value_addr); break;
            case 1: ss << int(*static_cast<const int8_t*>(value_addr)); break;
        }
    }
    if(type == 68) {
        ss << std::setw(30-plh_len-6-1-4-1) << std::setfill(' ') << std::right << " value=";
        switch(type_size) {
            case 4: ss << *static_cast<const uint32_t*>(value_addr); break;
            case 2: ss << *static_cast<const uint16_t*>(value_addr); break;
            case 1: ss << uint16_t(*static_cast<const uint8_t*>(value_addr)); break;
        }
    }
    ss << '\n' << (HelpCpp::memdump(Ptr, Ptr && type == 5 && type_size == -1 ? strlen(Ptr) : external_skip_size/*type_size*/));
}

template<> void sel_data<unsigned>::dump(std::ostream& ss) const
{
    ss << plh << " type=" << type << " sz="<< type_size <<'\n';
}

namespace {

std::string helpstring(const OciCpp::BindList_t &bufs, const OciCpp::DefList_t& s_bufs)
{
    std::ostringstream ss;
    for(auto& p : bufs)
        p.dump(ss);
    for(auto& p : s_bufs)
        p.dump(ss);
    return ss.str();
}
/*
    std::string make_a_neat_string(const char* nick, const char* file, int line)
    {
        if ((not file or not *file) and not line) {
            return "";
        } else {
            std::string result = "(";
            result += file and *file ? file : "file";
            result += ":" + std::to_string(line) + ")";
            return result;
        }
    }
*/
    std::string make_what_str(int err, const std::string& main_text, const std::string& text)
    {
        std::stringstream os;
        os << "oracle error =" << err << "\n"
                << "oracle error message: " << text <<
                (main_text.empty() ? "" : "other text: " + main_text);
        return os.str();
    }

    std::string make_help_str(const std::string& msg, const std::string& err_text,
            const std::string& query, int err)
    {
        std::ostringstream ost;
        ost << msg << "\nOracle text: " << err_text << " errcode:"<< err
            << (!query.empty() ? "\nquery: " + query : "");
        return ost.str();
    }
}

ociexception::ociexception(const std::string &msg)
    : comtech::Exception(msg), main_text(msg), ora_err(-1)
{
    what_str = make_what_str(sqlErr(), getOraText(), main_text);
}

ociexception::ociexception(const std::string &msg, int err_code, std::string const &err_msg)
    : comtech::Exception(msg), main_text(msg), ora_err(err_code), ora_text(err_msg)
{
    what_str = make_what_str(sqlErr(), getOraText(), main_text);
}

template <> void buf_struct<const char *>::operator() (sel_data<const char *> &buf)
{
    int skip = (buf.ind == 0) ? 0 : std::max<int>(c.indicator_skipsize, sizeof(indicator));
    if (buf.ind == 0 && buf.default_value)  // FIXME
    {
        assert(buf.maxarrlen==0);
        buf.ind = &buf.indic;
    }
    if (c.isDebug()) {
        buf.print();
    }
    sb2* local_indic = buf.ind;
    if (!c.stableBind() && buf.data_type != External::pod && buf.bind_in) //FIXME should be not stable
    {
        assert(buf.maxarrlen==0);
        buf.indic = buf.ind ? *buf.ind : indicator(iok);
        buf.data.reset(buf.to(buf.value_addr, buf.indic));
        if (!buf.ind && buf.indic == inull)
        {
            local_indic = &buf.indic;
            //buf.ind = &buf.indic;
        }
    }
    auto Ptr = buf.data_type != External::pod or (c.stableBind() && buf.bind_in)
               ? buf.data.get()
               : static_cast<char*>(buf.value_addr);
    auto ptr_size = buf.get_size ? Ptr ? buf.get_size(Ptr) : 0 : buf.type_size;

    if(buf.maxarrlen!=0){
        buf.rcode.resize(buf.maxarrlen);
    }
    if (c.oracledata->bindps( buf.plh, buf.plh_len,
                 Ptr, ptr_size, std::max(c.data_skipsize, ptr_size), buf.type, local_indic,//buf.ind,
                 skip,
                 buf.curlen, (buf.curlen == 0) ? 0 : std::max(c.data_skipsize, int(sizeof(sb4))),
                 &buf.rcode[0], buf.maxarrlen,buf.curarrlen))
    {
        c.oracledata->error( c.nick, c.file, c.line);
        c.throwException("CursCtl::bind error 1");
    }
}

void CursCtl::throwException(const std::string& s)
{
#ifdef XP_TESTING
    LogTrace(TRACE0) << oracledata->sess()->connectionParams_.login << '/' << oracledata->sess()->connectionParams_.password << '@' << oracledata->sess()->connectionParams_.server;
#endif
    LogTrace(TRACE0) << make_help_str(ex_id() + "\n" + s, oracledata->error_text2(), std::string(oracledata->sqltext()), err())
                     << '\n' << helpstring(bind_bufs, sel_bufs);
    oracledata->throwException();
}

void CursCtl::throwException()
{
#ifdef XP_TESTING
    LogTrace(TRACE0) << oracledata->sess()->connectionParams_.login << '/' << oracledata->sess()->connectionParams_.password << '@' << oracledata->sess()->connectionParams_.server;
#endif
    LogTrace(TRACE0) << make_help_str(ex_id(), oracledata->error_text2(),
                                      std::string(oracledata->sqltext()), err())
                     << '\n' << helpstring(bind_bufs, sel_bufs);
    oracledata->throwException();
}

std::string CursCtl::ex_id()
{
    return make_nick(nick, file, line);
}
void CursCtl::error(const char* nick, const char* file , int line)
{
    oracledata->error(nick, file, line);
}
std::string CursCtl::error_text()
{
    return oracledata->error_text();
}
CursCtl& CursCtl::fetchLen(int l)
{
    fetch_array_len=l;
    return *this;
}
CursCtl& CursCtl::autoNull()
{
    auto_null = true;
    return *this;
}

void CursCtl::exec_start()
{
    ASSERT_SESSION_MODE_SWITCH(initialSessionMode_, oracledata->mode());
    if(not bind_bufs.empty())
    {
        const auto e = std::unique(bind_bufs.rbegin(), bind_bufs.rend(),
                                   [](auto&l, auto&r){ return l.plh_len == r.plh_len and strncmp(l.plh, r.plh, l.plh_len) == 0; });
        if(e != bind_bufs.rend() and warnTwiceBind())
           Logger::getTracer().ProgError(nick, file, line, "WARNING!!! Twice bind!");
        bind_bufs.erase(bind_bufs.begin(), e.base());
    }
    rpc_local = 0;
    rows_now = 0;
    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::exec_start %s", ex_id().c_str());
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::bind_bufs size %d", bind_bufs.size());
    }
    oracledata->save_vector_sizes(bind_bufs,sel_bufs);
    std::for_each(bind_bufs.begin(),bind_bufs.end(), buf_struct<const char* >(*this));
    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::def_bufs size %d\n%s",
                sel_bufs.size(), helpstring(bind_bufs, sel_bufs).c_str());
    }
    oracledata->exec_prepare(sel_bufs.size());
    if(oracledata->err8()){
        return;
    }
    std::for_each(sel_bufs.begin(),sel_bufs.end(), buf_struct<unsigned>(*this));

    has_exec_ = true;
    //has_bind_ = false;
}

void CursCtl::exec_end()
{
    if (isDebug())
    {
        Logger::getTracer().ProgTrace(TRACE1, "CursCtl::exec_end %s", ex_id().c_str());
    }
    std::for_each(bind_bufs.begin(), bind_bufs.end(), fetch_end_struct<const char *>(*this, 1));
}

void  CursCtl::fetch_end()
{
    rows_now = rowcount() - rpc_local;
    rpc_local = rowcount();
    std::for_each(sel_bufs.begin(), sel_bufs.end(), fetch_end_struct<unsigned>(*this, rows_now));
}
//#############################################################################
static std::map<std::string, std::shared_ptr<OciCpp::OciSession> > sessionPool;

void putSession(const std::string& name, std::shared_ptr<OciCpp::OciSession> session)
{
    sessionPool.insert(std::make_pair(name, session));
}

OciCpp::OciSession& getSession(const std::string& name)
{
    auto found = sessionPool.find(name);
    if (found != sessionPool.end()) {
        return *(found->second);
    }
    return mainSession();
}
//###############################################################################

CopyTable::CopyTable(OciSession& sess, const std::string& table1, const std::string & table2,std::list<FdescMore> const &moreFields):
    sess_(sess),table1_(table1),table2_(table2),moreFields_(moreFields)
{

    std::string col_name,data_type;
    int data_len;
    CursCtl cr(sess_.createCursor(STDLOG,"SELECT COLUMN_NAME,DATA_TYPE,DATA_LENGTH FROM USER_TAB_COLUMNS WHERE TABLE_NAME = :tab ORDER BY COLUMN_ID"));
    cr.def(col_name).def(data_type).def(data_len);
    cr.bind(":tab", table1_);
    cr.exec();
    int buflen=0;
    while (!cr.fen()) {
        Fdesc fdesc={col_name};
        if(data_type=="VARCHAR2"){
            fdesc.data_type=SQLT_VCS;
            fdesc.data_len=data_len+2;
        }else if(data_type=="NUMBER"){
            fdesc.data_type=SQLT_VNU;
            fdesc.data_len=22;
        }else if(data_type=="DATE"){
            fdesc.data_type=SQLT_DAT;
            fdesc.data_len=7;
        }else{
            LogError(STDLOG) << "data_type is " << data_type;
            abort();
        }
        fdesc.alloc_len=((fdesc.data_len-1)/8+1)*8; //ensure alignment of varchar2 len
        buflen+=fdesc.alloc_len;
        fields_.push_back(fdesc);
    }

    buf.resize(buflen);
    indbuf.resize(fields_.size());

    select_query=buildSelect();
    insert_query=buildInsert();

    buflen=0;
    int counter=0;
    for(Fdesc  &fd:  fields_){
        fd.ptr=&buf[buflen];
        fd.indp=&indbuf[counter];
        buflen+=fd.alloc_len;
        ++counter;
        fd.bindvar=":V"+std::to_string(counter);
    }

}
std::string CopyTable::buildSelectList ()
{
    std::string sep(" ");
    std::string result("SELECT");
    for(Fdesc &fd:  fields_){
        result+=(sep+fd.col_name);
        sep=", ";
    }
    return result;
}
std::string CopyTable::buildSelect ()
{
    return buildSelectList()+" FROM "+table1_;
}
std::string CopyTable::buildInsert ()
{
    std::string sep(" ");
    std::string result1("INSERT INTO "+table2_+" (");
    std::string result2(" VALUES (");
    int counter=1;
    for(Fdesc const &fd:  fields_){
        result1+=(sep+fd.col_name);
        result2+=(sep+":V"+std::to_string(counter));
        sep=", ";
        ++counter;
    }
    for(FdescMore const &fd:  moreFields_){
        result1+=(sep+fd.col_name);
        result2+=sep+fd.bindvar();
        sep=", ";
    }
    return result1+")"+result2+")";
}
void CopyTable::exec(int loglevel, const char* nick, const char* file, int line)
{
#define STDLOGX   nick,file,line
    OciCpp::CursCtl sel_curs=sess_.createCursor(STDLOGX,select_query);
    OciCpp::CursCtl ins_cur(make_curs(insert_query));
    ins_cur.unstb();
    for(Fdesc const &fd:  fields_){
        sel_curs.defFull(fd.ptr,fd.data_len,fd.indp,0,fd.data_type);
        ins_cur.bindFull(fd.bindvar,fd.ptr,fd.data_len,fd.indp,0,fd.data_type);
    }
    for(FdescMore  const &fd:  moreFields_){
        threeTypesBinder bind_visitor(ins_cur,fd.bindvar());
        boost::apply_visitor ( bind_visitor,fd.data);
    }
    sel_curs.exec();
    while(sel_curs.fen()!=NO_DATA_FOUND){
        ins_cur.exec();
    }
}

//#############################################################################
DumpTable::DumpTable(OciSession& s, const std::string& table)
    : sess_(s), table_(table)
{}

DumpTable::DumpTable(const std::string& table)
    : sess_(mainSession()), table_(table)
{}

DumpTable& DumpTable::addFld(const std::string& fieldsStr)
{
    std::vector<std::string> fields;
    StrUtils::split_string(fields, fieldsStr);
    for(const std::string& fld:  fields) {
        fields_.push_back(fld);
    }
    return *this;
}

DumpTable& DumpTable::where(const std::string& wh)
{
    where_ = wh;
    return *this;
}

DumpTable& DumpTable::order(const std::string& ord)
{
    order_ = ord;
    return *this;
}


class DumpTableOut
{
  public:
    virtual ~DumpTableOut() {};
    virtual void print(std::string const& s) const =0;
};

namespace {

class DumpTableOutLogger : public DumpTableOut
{
  private:
    int loglevel__;
    std::string nick__;
    std::string file__;
    int line__;
  public:
    DumpTableOutLogger(int loglevel,const char* nick,const char* file,int line)
      : loglevel__(loglevel),nick__(nick),file__(file),line__(line) {}
    virtual ~DumpTableOutLogger() {};
    virtual void print(std::string const& s) const override final
    {
      LogTrace(loglevel__, nick__.c_str(), file__.c_str(), line__)<<s;
    }
};

class DumpTableOutString : public DumpTableOut
{
  private:
    std::string &s__;
  public:
    DumpTableOutString(std::string &out) : s__(out) { s__.clear(); }
    virtual ~DumpTableOutString() {};
    virtual void print(std::string const& s) const override final
    {
      s__.append(s).append("\n");
    }
};

} // namespace

void DumpTable::exec(DumpTableOut const& out)
{
    out.print("--------------------- " + table_ + " DUMP ---------------------");
    std::string sql = "SELECT ";

    if (fields_.empty()) {
        std::string fld;
        CursCtl cr(sess_.createCursor(STDLOG,"SELECT COLUMN_NAME FROM USER_TAB_COLUMNS WHERE TABLE_NAME = UPPER(:tab) ORDER BY COLUMN_ID"));
        cr.bind(":tab", table_);
        cr.def(fld);
        cr.exec();
        while (!cr.fen()) {
            fields_.push_back(fld);
        }
        if (fields_.empty()) {
            throw ociexception('[' + sess_.getConnectString() + "] table " + table_ + " doesn't exist");
        }
    }

    for(const std::string& fld:  fields_) {
        sql += fld + ", ";
    }
    sql = sql.substr(0, sql.length() - 2);
    sql += " FROM " + table_;
    if (!where_.empty())
        sql += " WHERE " + where_;
    if (!order_.empty())
        sql += " ORDER BY " + order_;

    out.print(sql);
    std::vector<std::string> vals(fields_.size());
    CursCtl cr(sess_.createCursor(STDLOG,sql));
    for(std::string& val:  vals) {
        cr.defNull(val, "NULL");
    }
    cr.exec();
    size_t count=0;
    while (!cr.fen()) {
        std::stringstream str;
        for(const std::string& val:  vals) {
            str << "[" << val << "] ";
        }
        out.print(str.str());
        count++;
    }
    out.print("------------------- END " + table_ + " DUMP COUNT="
      + HelpCpp:: string_cast(count) + " -------------------");
}


void DumpTable::exec(int loglevel, const char* nick, const char* file, int line)
{
  exec(DumpTableOutLogger(loglevel,nick,file,line));
}

void DumpTable::exec(std::string &out)
{
  exec(DumpTableOutString(out));
}

//#############################################################################

char* OciSelector<char const *>::to(const void* a, indicator& ind)
{
    if(ind != iok)
        return nullptr;
    const size_t a_len = strlen(static_cast<const char*>(a));
    char* memory = new char[a_len + 1];
    memcpy(memory, a, a_len);
    memory[a_len] = '\0';
    return memory;
}

int OciSelector<char const *>::size(const void* addr)
{
    return strlen(static_cast<const char*>(addr)) + 1;
}

char* OciSelector< const std::string >::to(const void* a, indicator& ind)
{
    if(ind != iok)
        return nullptr;
    const std::string *s = static_cast<const std::string*>(a);
    char* memory = new char[s->size() + 1];
    s->copy(memory, s->size());
    memory[s->size()] = '\0';
    return memory;
}

void OciSelector< std::string >::from(char* out_ptr, const char* in_ptr, indicator ind)
{
    if (ind == iok)
    {
        std::string *sp = reinterpret_cast<std::string *>(out_ptr);
        sp->assign(in_ptr);
    }
}

int OciSelector< const std::string >::size(const void* addr)
{
    return OciSelector<char const *>::size(addr);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

static bool operator == (const oracle_datetime& l, const oracle_datetime& r)
{
    return memcmp(&l, &r, sizeof l) == 0;
}

static oracle_datetime odt_pos_infin(4712, 12, 31, 23, 59, 59); // Oracle PL-SQL Programming
static oracle_datetime odt_neg_infin(-4711, 1, 1, 0, 0, 1); // 1 Jan 4712 BC (Oracle)

boost::gregorian::date from_oracle_date(const oracle_datetime &od)
{
    if(od == odt_pos_infin)
        return boost::gregorian::date(boost::gregorian::pos_infin);
    if(od == odt_neg_infin)
        return boost::gregorian::date(boost::gregorian::neg_infin);

    int year = (od.value[0] - 100) * 100 + od.value[1] - 100;
    int month = od.value[2];
    int day = od.value[3];
    if (!year && !month && !day)
        return {};  // it's not a date

    return boost::gregorian::date(year, month, day);
}

boost::posix_time::time_duration from_oracle_time_duration(const oracle_datetime& od)
{
    return boost::posix_time::time_duration(od.value[4] - 1, od.value[5] - 1, od.value[6] - 1, 0);
}

oracle_datetime to_oracle_datetime(const boost::gregorian::date & d)
{
    if (d.is_pos_infinity()) {
        return odt_pos_infin; // oracle_datetime(2049, 12, 31);
    } else if (d.is_neg_infinity()) {
        return odt_neg_infin; // oracle_datetime(1949, 1, 1);
    } else if (d.is_special()) {
        return oracle_datetime();
    } else {
        return oracle_datetime(d.year(), d.month(), d.day());
    }
}

boost::posix_time::ptime from_oracle_time(const oracle_datetime &od)
{
    auto date = from_oracle_date(od);
    if(date.is_special())
        return boost::posix_time::ptime(date);
    return boost::posix_time::ptime(date, from_oracle_time_duration(od));
}

oracle_datetime to_oracle_datetime(const boost::posix_time::ptime & pt)
{
    if(pt.is_special())
        return to_oracle_datetime(pt.date());

    const auto d = pt.date();
    const auto t = pt.time_of_day();
    return oracle_datetime(d.year(), d.month(), d.day(), t.hours(), t.minutes(), t.seconds());
}

//-----------------------------------------------------------------------

char* OciSelector<const boost::gregorian::date>::to(const void* a, indicator& indic)
{
    auto tmp_dt = reinterpret_cast<const boost::gregorian::date*>(a);
    if (tmp_dt->is_not_a_date())
        indic = inull;
    if (indic == inull)
        return nullptr;

    char* memory = new char[7];
    new (memory) oracle_datetime(to_oracle_datetime(*tmp_dt));
    return memory;
}

void OciSelector<boost::gregorian::date>::from(char* out_ptr, const char* in_ptr, indicator ind)
{
    boost::gregorian::date& tmp_date = *reinterpret_cast<boost::gregorian::date*>(out_ptr);
    const oracle_datetime& dt = *reinterpret_cast<const oracle_datetime*>(in_ptr);
    tmp_date = from_oracle_date(dt);
}

//-----------------------------------------------------------------------

char* OciSelector<const boost::posix_time::ptime>::to(const void* a, indicator& indic)
{
    auto tmp_dt = reinterpret_cast<const boost::posix_time::ptime*>(a);
    if (tmp_dt->is_not_a_date_time())
        indic = inull;
    if (indic == inull)
        return nullptr;

    char* memory = new char[7];
    new (memory) oracle_datetime(to_oracle_datetime(*tmp_dt));
    return memory;
}

void OciSelector<boost::posix_time::ptime>::from(char* out_ptr, const char* in_ptr, indicator ind)
{
    boost::posix_time::ptime& tmp_time = *reinterpret_cast<boost::posix_time::ptime*>(out_ptr);
    const oracle_datetime& dt = *reinterpret_cast<const oracle_datetime*>(in_ptr);
    tmp_time = from_oracle_time(dt);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

} // namespace OciCpp

#ifndef ENABLE_PG_TESTS
#ifdef XP_TESTING
#include "test.h"
#include "xp_test_utils.h"
#include "timer.h"
#include "memmap.h"
#include "checkunit.h"
#include "enum_oci.h"
#include "oci8cursor.h"
#include "oci_rowid.h"
#include "std_array_oci.h"

namespace {
using namespace std;
using namespace OciCpp;
void setup()
{
    // тестируется по-старому, т.е. с явным указанием stb() для курсора
    setTclVar("CURSCTL_DEFAULT_STABLE_BIND", "0");
    Sleep(30);
    testInitDB();
    openGlobalCursors();
}
void teardown()
{
    testShutDBConnection();
}

CursorCache::CursorCacheMap& getCursorCache()
{
    return mainSession().cache_->cursors_;
}

    CursorCache::CursorCacheMap& getCursorCache(const OciCpp::OciSession& sess)
    {
        return sess.cache_->cursors_;
    }

    static void printCursorCache()
    {
        CursorCache::CursorCacheMap& cmap = getCursorCache();

    LogTrace(TRACE5) << "------------------- Cache content: " << cmap.size() << " ------------------------";
    for (CursorCache::CursorCacheMap::iterator i = cmap.begin(); i != cmap.end(); ++i)
    {
        LogTrace(TRACE5) << i->first;
    }
    LogTrace(TRACE5) << "-------------------------------------------------------";
}

static bool isCursorInCache(const char* sql)
{
    return (mainSession().cache_->find(sql) != NULL);
}

START_TEST(test_commit_rollback)
{
    rollback();
}END_TEST

START_TEST(test_binddefFlt)
{
    double d = 2.3, resd = 0;
    make_curs("select :d - FLOOR(:d) from dual")
        .bind(":d", d)
        .def(resd)
        .EXfet();
    fail_unless(resd == 0.3, "bind/def double failed");

    float f = 1.5, resf = 0;
    make_curs("select :f - FLOOR(:f) from dual")
        .bind(":f", f)
        .def(resf)
        .EXfet();
    fail_unless(resf == 0.5, "bind/def float failed");
}
END_TEST


START_TEST(cursor_cache)
{
    const char *str="select 1 from dual";
    char s2[200];
    strcpy(s2, str);
#define CHECK_CACHE_SIZE(n) fail_unless(getCursorCache().size() == (n), "bad cache size (expected %zu, got %zu)", (n), getCursorCache().size())
    CHECK_CACHE_SIZE(1);
    auto c=mainSession().cdaCursor(str,true);
    // must be only one entry in cache
    fail_unless(c!=0, "cursor is null");
    CHECK_CACHE_SIZE(2);
    const char *s3 = "select 2 from dual";
    auto c2=mainSession().cdaCursor(s3,true);

    // must be only two entries in cache
    CHECK_CACHE_SIZE(3);
    fail_unless(c.get()==mainSession().cdaCursor(s2,true).get(),"cursor is different");
    // still two entries as string s2 already in cache
    CHECK_CACHE_SIZE(3);
    fail_unless(c2.get()==mainSession().cdaCursor(s3,true).get(),"cursor is different");
    // still two entries as string s3 already in cache
    CHECK_CACHE_SIZE(3);
#undef CHECK_CACHE_SIZE
}
END_TEST

START_TEST(cursor_cache_distruct1)
{
    // Check distruction of ptr for newCursor - step1

    OciCpp::mainSession().set7();
    OciCpp::mainSession().set8();

    make_curs("declare i number ;begin select 1 into i from dual;end;").exec();
    OciCpp::mainSession().set7();
    fail_unless(getCursorCache().size()==2, "bad cache size !=0");

    {
      std::string s("select 11 from dual");
      const char *ptr1=s.c_str();
      cda_text *c1=newCursor(OciCpp::mainSession(), ptr1);
      fail_unless(c1!=0,"cursor is null");

      s[7]='2'; // "select 21 from dual";
      const char *ptr2=s.c_str();
      fail_unless(ptr1==ptr2, "pointers is different");

      cda_text *c2=newCursor(OciCpp::mainSession(), ptr2);
      fail_unless(c1!=c2,"cursor is same");
    }

    //{
      //std::string s("select 12 from dual");
      //const char *ptr1=s.c_str();
      //cda_text *c1=newCursor(ptr1,false);
      //fail_unless(c1!=0,"cursor is null");

      //s[7]='2'; // "select 22 from dual";
      //const char *ptr2=s.c_str();
      //fail_unless(ptr1==ptr2,"pointers is different");

      //cda_text *c2=newCursor(ptr2,false);
      //fail_unless(c1==c2,"cursor is different");
    //}

    // getCursorCache() internal buffer invalidated
}
END_TEST

START_TEST(cursor_cache_distruct2)
{
    // Check distruction of ptr for newCursor - step2
    std::string s("select 13 from dual");
    const char *ptr1=s.c_str();
    cda_text *c1=newCursor(OciCpp::mainSession(), ptr1);
    fail_unless(c1!=0,"cursor is null");

    s[7]='2'; // "select 23 from dual";
    const char *ptr2=s.c_str();
    fail_unless(ptr1==ptr2,"pointers is different");

    cda_text *c2=newCursor(OciCpp::mainSession(), ptr2);
    fail_unless(c1!=c2,"cursor is same");
}
END_TEST

START_TEST(cursor_cache_distruct3)
{
    // Check distruction of ptr for newCursor - step3
    cda_text *c1=NULL;
    cda_text *c2=NULL;
    {
      std::string s1=std::string("select")+" "+"14 from dial";
      c1=newCursor(OciCpp::mainSession(), s1.c_str());
      fail_unless(c1!=0,"cursor is null");
      s1[0]='4';
    }
    {
      std::string s2=std::string("select")+" "+"34 from dial";
      std::string s1=std::string("select")+" "+"14 from dial";
      c2=newCursor(OciCpp::mainSession(), s1.c_str());
      fail_unless(c2!=0,"cursor is null");
    }
    fail_unless(c1==c2,"cursor is different");
}
END_TEST

//Стоит сделать новый тест на возврат адреса, но не через Addr_base.

/*template <typename T>  OciCpp::Addr_base_::
Addr_base *get(T &t)
{
       return
       new typename OciSelector<T>::Addr(&t);
}
START_TEST(test_Addr_template)
{
    try{
        char s[10];
        OciCpp::Addr_base_::Addr_base *ptr=get(s);
        fail_unless(ptr->getAddr()==(void*)s,"address not match");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST*/

START_TEST(test_Cstring_bindout)
{
    try{
        CursCtl c=make_curs("begin :s:='xxxx'; end;");
        char s[10];
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(string ("xxxx")==s, "s!=xxxx" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

//После исправления исключений надо переделать эти два теста.
/*START_TEST(test_null_bindout)
{
    try{
        CursCtl c=make_curs("begin :s:=null; end;");
        char s[10] = {0};
        c.setDebug().bindOut(":s", s).exec();
        fail_unless(0,"s is not null");
    }catch(ociexception &e){
        LogTrace(TRACE5) << e.what();
        fail_unless(std::string(e.what()).find(":s rcode=") != std::string::npos, "no rcode info in error");
    }
}
END_TEST

START_TEST(test_def_rcode)
{
    try{
        int i = 0, j = 0;
        make_curs("select null, 0 from dual")
            .def(i).def(j)
            .EXfet();
        fail_unless(0,"i is not null");
    }catch(ociexception& e){
        LogTrace(TRACE5) << e.what();
        fail_unless(std::string(e.what()).find("rcode=") != std::string::npos, "no rcode info in error");
    }
}
END_TEST*/

// даже при использовании bind перменная может быть изменена
// Это неправильное поведение
// оно будет изменено
// не используйте это
START_TEST(test_Cstring_bindout2)
{
    int x = 10;
    make_curs("begin :x:=123; end;").unstb()
        .bind(":x", x)
        .exec();
    fail_unless(x == 123, "x != 123" );
#if 0 // must work as written
    try {
        int x = 10;
        make_curs("begin :x:=123; end;")
            .bind(":x", x)
            .exec();
        fail_unless(0,"must throw because of changing x");
    } catch( ociexception & ) {
        // must be here and it is fine
    }
    try {
        int x = 10;
        make_curs("begin :x:=null; end;")
            .bind(":x", x)
            .exec();
        fail_unless(0,"must throw because of changing x");
    } catch( ociexception & ) {
        // must be here and it is fine
    }
    try {
        int x = 10;
        make_curs("begin if 0>1 then :x:=123; end if; end;")
            .bind(":x", x)
            .exec();
        fail_unless(0,"must throw because of changing x");
    } catch( ociexception & ) {
        // must be here and it is fine
    }
#endif // 0
}
END_TEST
/*
START_TEST(test_stable_bind)
{
    try{
        CursCtl c=make_curs("select :s1,:s2,:s3 from dual");
        char s[10],a1[10];
        char *s1=s;
        strcpy(s1,"aa");
        std::string s2("aa"),a2;
        int s3=1,a3;

        c.setDebug().stb().bind(":s1",s1).bind(":s2",s2).bind(":s3",s3);
        c.def(a1).def(a2).def(a3).exfet();
        fail_unless(string (a1)=="aa", "char*" );
        fail_unless(a2=="aa", "std::string" );
        fail_unless(a3==1, "int" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST */
START_TEST(test_Cstring_bindout_tab)
{
    try{
        char s[10];
        CursCtl c=make_curs("begin :s:='xxxx\011'; end;");
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(string ("xxxx\011")==s, "s!=xxxx_tab" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_bindFull)
{
    try{
        int msgin[3]={1,2,3};
        int msgout[3];
        unsigned short len=0;
        int s=0;
        CursCtl c=make_curs("begin :ss:=11;:s:=:v; end;");
        c.setDebug().bindOut(":ss",s).bindFull(":v",msgin,sizeof(msgin),
                0,0,SQLT_BIN).bindFull(":s",msgout,sizeof(msgout),
                    0,&len,SQLT_BIN).exec();
        fail_unless(len==sizeof(msgin),"wrong length");
        fail_unless(msgout[0]==1 && msgout[1]==2 &&
                msgout[2]==3,"wrong data");
        fail_unless(s==11,"wrong int");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
struct r2 {
    int m[6];
    unsigned short len;
    int i;
};
START_TEST(test_defFull)
{
    try{
        int msgin1[5]={1,2,3,4};
        int msgin2[5]={2,3,4,5};
        r2 r[3];
        unsigned short l1=12,l2=16;
        CursCtl c=make_curs(
                "select v,i from (select  :v1 v,"
                "11 i from dual "
                "union select :v2 v , 12 i from dual) order by i"
                );
        c.setDebug().fetchLen(3).structSize(sizeof(r2)).
            bindFull(":v1",msgin1,sizeof(msgin1),
                0,&l1,SQLT_BIN).bindFull(":v2",msgin2,sizeof(msgin2),
                    0,&l2,SQLT_BIN).defFull(&r[0].m,sizeof(r[0].m),
                        0,&r[0].len,SQLT_BIN).
                        def(r[0].i).exfet(3);

        fail_unless(c.rowcount()==2,"wrong rowcount");
        fail_unless(c.rowcount_now()==2,"wrong rowcount_now");
        fail_unless(r[0].i==11 && r[1].i==12,"wrong i");
        LogTrace(TRACE1) <<"r0 "<<r[0].len ;
        LogTrace(TRACE1) <<"r1 "<<r[1].len ;
        fail_unless(r[0].len==12 && r[1].len==16,"wrong len");
        fail_unless(memcmp(msgin1,r[0].m,12)==0 &&
                memcmp(msgin2,r[1].m,16)==0,"wrong data");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_resize)
{
    try{
        resizeDebug=1;
        for (int i=0;i<10;++i){
            OciCpp::CursCtl c("select 1, 2, 3,4, 'aaaa' from dual where :a=:b and :c!=:d");
            int odin=0 ,dva=0,tri=0,chetyre=0;
            std::string s1;
            c.bind(":a",1).bind(":b",1).bind(":c",2).bind(":d",3).def(odin).def(dva).def(tri).def(chetyre).def(s1);
            c.exec();
            c.fen();
            fail_unless(odin==1 && dva==2 && tri==3 && chetyre==4 && s1=="aaaa", "bad data feteched");
            if(i!=0) {
                fail_unless(ext_def_size==5 && ext_bind_size==4 ,"vector size not saved");
            }
        }


    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_Cstring_bindout_null)
{
    try{
        char s[10];
        CursCtl c=make_curs("begin :s:=null; end;");
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(0, "must be ociexception" );
    }catch(ociexception &e){
        //fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_Cstring_bindout_null_detect)
{
    try{
        char s[10];
        CursCtl c=make_curs("begin :s:=null; end;");
        c.setDebug().autoNull().bindOutNull(":s",s,"null").exec();
        ck_assert_str_eq(s, "null");
    }catch(ociexception &e){
        ProgTrace(TRACE5, "error: %s", e.what());
        fail_unless(0,"test_Cstring_bindout_null_detect failed");
    }
}
END_TEST

START_TEST(test_Cstring_bindout_auto_null)
{
    try{
        char s[10];
        CursCtl c=make_curs("begin :s:=null; end;");
        c.setDebug().autoNull().bindOut(":s",s).exec();
        fail_unless(strcmp("",s)==0, "default value for null failed" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_int_bindout_null)
{
    try{
        int s=7;
        CursCtl c=make_curs("begin :s:=null; end;");
        c.setDebug().bindOutNull(":s",s,8).exec();
        fail_unless(s==8, "default value for null failed" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_int_bindout_null2)
{
    try{
        int s=7;
        CursCtl c=make_curs("begin :s:=null; end;");
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(0, "must be exception" );
    }catch(ociexception &e){
        //fail_unless(0,e.what());
    }
}
END_TEST


START_TEST(test_int_bindout)
{
    try{
        int s=7;
        CursCtl c=make_curs("begin :s:=8; end;");
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(s==8, "default value for null failed" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_uint_bindout)
{
    try{
        unsigned short s=7;
        CursCtl c=make_curs("begin :s:=65000; end;");
        c.setDebug().bindOut(":s",s).exec();
        fail_unless(s==65000, "default value for null failed" );
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST


START_TEST(test_Cstring_bind)
{
    try {
        char s[10];
        char s2[10];
        strcpy(s2,"xx");
        CursCtl c=make_curs("select :s||:s from dual");
        c.setDebug().bind(":s",s2).def(s).EXfet();
        fail_unless(string("xxxx")==s,"xxxx!=xxxx");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_Cstring_bind_ptr)
{
    try {
        char s[10];
        char s2[10];
        char *ps=s2;
        strcpy(s2,"xx");
        CursCtl c=make_curs("select :s||:s from dual");
        c.setDebug().bind(":s",ps).def(s).EXfet();
        fail_unless(string("xxxx")==s,"xxxx!=xxxx");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_Cstring_bind_const)
{
    try {
        char s[10];
        char s2[10];
        strcpy(s2,"xx");
        CursCtl c=make_curs("select :s||:s from dual");
        c.setDebug().bind(":s",(const char *)s2).def(s).EXfet();
        fail_unless(string("xxxx")==s,"xxxx!=xxxx");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_bind)
{
    try {
        char s[10];
        string s2("xx");
        CursCtl c=make_curs("select :s||:s from dual");
        c.setDebug().bind(":s",s2).def(s).EXfet();
        fail_unless(string("xxxx")==s,"xxxx!=xxxx");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_bind_second_exec)
{
    try {
        char s[10];
        string s2("xx");
        CursCtl c=make_curs("select :s||:s from dual");
        c.setDebug().unstb().bind(":s",s2).def(s).EXfet();
        fail_unless(string("xxxx")==s,"xxxx!=xxxx");
        string dummy; dummy.reserve(1000);
        s2.reserve(1000);
        s2="yy";
        c.EXfet();
        fail_unless(string("yyyy")==s,"yyyy!=yyyy");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_bind_complex)
{
    try {
        string s("xx");
        char s2[10];
        CursCtl c=make_curs("begin :s2:=:s||:s; end;");
        c.setDebug().bind(":s",s).bindOut(":s2",s2).exec();
        fail_unless(string("xxxx")==s2,"xxxx!=xxxx");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST


START_TEST(test_Cstring)
{
    try {
        char s[10];
        CursCtl c=make_curs("select 'xxxx' from dual");
        c.setDebug().def(s).EXfet();
        fail_unless(strcmp("xxxx",s)==0,"xxxx!=xxxx");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_select1)
{
    try {
        string s;
        CursCtl c=make_curs("select 'xxxx' from dual");
        c.setDebug().def(s).EXfet();
        fail_unless(strcmp("xxxx",s.c_str())==0,"xxxx!=xxxx");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_select2)
{
    try {
        string s;
        int i=555555555;
        CursCtl c=make_curs("select :s from dual");
        c.setDebug().bind(":s",i).def(s).EXfet();
        fail_unless(strcmp("555555555",s.c_str())==0,"xxxx!=xxxx");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_select3)
{
    try {
        string s1,s2;
        const char *p="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        CursCtl c=make_curs("select null,null from dual");
        c.setDebug().autoNull().def(s1).defNull(s2,p).EXfet();
        fail_unless(strcmp("",s1.c_str())==0,"autoNull failed for string");
        fail_unless(strcmp(p,s2.c_str())==0,"defNull failed for string");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
struct result1 {
    int r; string s;
};
START_TEST(test_string_select4)
{
    try {
        result1 re[10];
        CursCtl c=make_curs("select A,B from (select 5 A ,'' B from dual "
                "union select 6 A, 'aaa' B from dual) order by A ");

        c.setDebug().fetchLen(10).structSize(sizeof(re[0])).def(re[0].r).
            defNull(re[0].s,"xx").exec();
        c.fen(10);
        ck_assert_int_eq(c.err(), NO_DATA_FOUND);
        ck_assert_int_eq(c.rowcount(), 2);
        ck_assert_int_eq(re[0].r, 5);
        ck_assert_str_eq(re[0].s, "xx");
        ck_assert_int_eq(re[1].r, 6);
        ck_assert_str_eq(re[1].s, "aaa");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_select5)
{
    try {
        string s[5];
        string s2[5];
        CursCtl c=make_curs("select x,n from (select 'xxx1' x, 1 n  from dual "
                "union select 'xxx2' x, 2 n  from dual "
                "union select 'xxx3' x,3 n from dual ) order by n");
        c.setDebug().fetchLen(10).def(s[0]).def(s2[0]).EXfet(3);
        fail_unless(s[0]=="xxx1","wrong data");
        fail_unless(s[1]=="xxx2","wrong data");
        fail_unless(s[2]=="xxx3","wrong data");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_string_select6)
{
    try {
        string s[5];
        string s2[5];
        CursCtl c=make_curs("select x,n from (select 'xxx1' x, 1 n  from dual "
                "union select 'xxx2' x, 2 n  from dual "
                "union select 'xxx3' x,3 n from dual ) order by n");
        c.setDebug().fetchLen(10).def(s[0]).def(s2[0]).exec();
        c.fen();
        fail_unless(s[0]=="xxx1","wrong data");
        c.fen();
        fail_unless(s[0]=="xxx2","wrong data");
        c.fen();
        fail_unless(s[0]=="xxx3","wrong data");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_Cstring_tab)
{
    try {
        char s[10];
        CursCtl c=make_curs("select 'xxxx\011' from dual");
        c.setDebug().def(s).EXfet();
        fail_unless(strcmp("xxxx\011",s)==0,"xxxx_tab!=xxxx_tab");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_Cstring_null)
{
    try {
        char s[10];
        make_curs("select null from dual").setDebug().def(s).EXfet();
        fail_unless(0,"is null");
    }catch(ociexception &e){
        //fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_Cstring_null2)
{
    try {
        char s[10];
        short ind=0;
        make_curs("select null from dual").setDebug().idef(s,&ind).EXfet();
        fail_unless(ind==-1,"indicator doesn't work");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(OciSelectorConv)
{
    auto r = OciSelector<char [5]>::conv("xxx");
    char x[11];
    r.fill(x);
    ck_assert_str_eq(x, "xxx");
}
END_TEST
START_TEST(test_Cstring_null3)
{
    try {
        char s[10];
        make_curs("select null from dual").setDebug().defNull(s,"xxx").EXfet();
        ck_assert_str_eq(s, "xxx");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_OciVcs)
{
    OciVcs<10> s1, s2;
    make_curs("select 'xxxx' from dual").def(s1).EXfet();
    fail_unless(string("xxxx") == string(s1.arr, s1.len), "xxxx!=xxxx");
    OciVcs<10> s3(s1);
    fail_unless(s3 == s1, "bad copy constructor");
    s2 = s1;
    fail_unless(s2 == s1, "bad assignment operator");
}END_TEST
START_TEST(OciSelectorConv3)
{
    /*
    buf_ptr_t r(OciSelector<OciVcs<5> >::conv("xxx"));
    OciVcs<5> s;
    memcpy(&s, r.get(),5);

    fail_unless(s.len==3,"OciSelector<OciVcs<5> >::conv");
    fail_unless(memcmp("xxx",s.arr,3)==0,"OciSelector<OciVcs<5> >::conv('xxx') = ('x' = %.02x) %zu : %.02x %.02x %.02x %.02x %.02x", 'x', s.len, s.arr[0], s.arr[1], s.arr[2], s.arr[3], s.arr[4]);
    */
}
END_TEST
START_TEST(test_OciVcs_null)
{
    try {
        OciVcs<10> s;
        make_curs("select null  from dual").setDebug().defNull(s,"default").EXfet();
        fail_unless(s.len==(int)strlen("default"),
                "OciVcs default value failed");
        fail_unless(strncmp("default",s.arr,s.len)==0,
                "OciVcs default value failed");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_int)
{
    try {
        int i=0;
        make_curs("select 5 from dual").setDebug().def(i).EXfet();
        fail_unless(i==5,"i!=5");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_long_long)
{
    try {
        long long i=0;
        make_curs("select 1000000000*1000000000 from dual").setDebug().def(i).EXfet();
        fail_unless(i==1000000000000000000,"i!=1'000'000'000'000'000'000");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
    try {
        long long i=0, j=1000000000000000000, k=1000;
        make_curs("select :v1/:v2 from dual").setDebug().bind(":v1",j).bind(":v2",k).def(i).EXfet();
        fail_unless(i==1000000000000000,"i!=1'000'000'000'000'000");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_int_null)
{
    try {
        int i; short ind=0;
        make_curs("select null  from dual").setDebug().idef(i,&ind).EXfet();
        fail_unless(ind==-1,"is not null");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
namespace {
    struct result {
        int r; char s[10];
        sb2 nd;
    };
}
START_TEST(test_select_struct)
{
    try {
        result re[10];
        memset(re,'E',sizeof(re));
        CursCtl c=make_curs("select A,B from (select 5 A ,'aaa' B from dual "
                       "union select 6 A, '' B from dual) order by A ");
        c.setDebug().fetchLen(10).structSize(sizeof(re[0]),sizeof(re[0])).def(re[0].r).
            defNull(re[0].s,"xx").exec();
        c.fen(10);
        fail_unless(c.err()==NO_DATA_FOUND,"should be no data found");
        fail_unless(c.rowcount()==2,"wrong rowcount select failed");
        ck_assert_int_eq(re[0].r, 5);
        ck_assert_int_eq(re[1].r, 6);
        ck_assert_str_eq(re[0].s, "aaa");
        ck_assert_str_eq(re[1].s, "xx");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_select_struct_null)
{
    bool err=false;
    try {
        result re[10];
        memset(re,'E',sizeof(re));
        OciCpp::CursCtl c=make_curs("select A,B from (select 5 A ,'aaa' B from dual "
                       "union select 6 A, '' B from dual "
                       "union select NULL A, 'ggg' B from dual) order by A ");
        c.setDebug().fetchLen(10).structSize(sizeof(re[0]),sizeof(re[0])).def(re[0].r).
            defNull(re[0].s,"xx").exec();
        c.fen(10);
    }catch(OciCpp::ociexception &e){
      if(e.sqlErr()==1405)
        err=true;
      else
        fail_unless(0,e.what());
    }
    fail_unless(err,"waiting for 'fetched column value is NULL' error");
}
END_TEST

START_TEST(test_select_struct_auto_null)
{
    try {
        result re[10];
        memset(re,8,sizeof(re));
        CursCtl c=make_curs("select A,B from (select 5 A ,'aaa' B from dual "
                        "union select 6 A, '' B from dual) order by A ");
        c.setDebug().fetchLen(10).autoNull().structSize(sizeof(re[0]),sizeof(re[0])).def(re[0].r).
            def(re[0].s).exec();
        c.fen(10);
        fail_unless(c.err()==NO_DATA_FOUND,"should be no data found");
        fail_unless(c.rowcount()==2,"wrong rowcount select failed");
        fail_unless(re[0].r==5 && re[1].r==6 &&
                strcmp(re[0].s,"aaa")==0 && strcmp(re[1].s,"")==0,
                "wrong data fetched");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_select_struct_auto_null_with_def)
{
    try {
        result re[10];
        memset(re,8,sizeof(re));
        CursCtl c=make_curs("select A,B from (select 5 A ,'aaa' B from dual "
                 "union select 6 A, 'xx' B from dual) order by A ");
        c.setDebug().fetchLen(10).autoNull().
            structSize(sizeof(re[0]),sizeof(re[0])).def(re[0].r).
            defNull(re[0].s,"xx").exec();
        c.fen(10);
        fail_unless(c.err()==NO_DATA_FOUND,"should be no data found");
        fail_unless(c.rowcount()==2,"wrong rowcount select failed");
        fail_unless(re[0].r==5 && re[1].r==6 &&
                strcmp(re[0].s,"aaa")==0 && strcmp(re[1].s,"xx")==0,
                "wrong data fetched");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

struct test_date
{
    dt::date date_;
    dt::date other_dt_;
};

START_TEST(test_date_struct_fetch)
{
    try
    {
        test_date dates[2];
        CursCtl curs = make_curs("select A,B from (select to_date('12.11.1991', 'DD-MM-YYYY')A, "
                                 "to_date('12.10.1995', 'DD-MM-YYYY') B from dual "
                                 "union select to_date('12.10.1991', 'DD-MM-YYYY') A, "
                                 "to_date('12.11.1995', 'DD-MM-YYYY') B from dual) ");
        curs.fetchLen(2).structSize(sizeof(dates[0])).def(dates[0].date_).def(dates[0].other_dt_).exec();
        curs.fen(2);
        fail_unless(curs.rowcount()==2, "wrong rowcount select failed");
        fail_unless(dt::to_iso_extended_string(dates[0].date_) == "1991-10-12" &&
                    dt::to_iso_extended_string(dates[0].other_dt_) == "1995-11-12" &&
                    dt::to_iso_extended_string(dates[1].date_) == "1991-11-12" &&
                    dt::to_iso_extended_string(dates[1].other_dt_) == "1995-10-12",
                    "wrong data fetched");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_int_null2)
{
    try {
        int i;
        make_curs("select null  from dual").setDebug().def(i).EXfet();
        fail_unless(0,"is not null");
    }catch(ociexception &e){
        //cerr <<e.what()<<endl;
    }
}
END_TEST
#if 1
START_TEST(test_int_null3)
{
    try {
        int i=0;
        make_curs("select null  from dual").setDebug().defNull(i,5).EXfet();
        fail_unless(i==5,"int default value failed");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
#endif

START_TEST(test_combine)
{
    try {
        int n[10] ; char t1[10][10] ; int n2[10]; short ind_n2[10];
        OciVcs<10> t2[10];string t3[10];

        CursCtl c=make_curs(
                "select n,t1||:w,n2,t2,t3 from (\n"
                "select 1 n , 'xxxx' t1,5 n2,null t2, 'aaaaa' t3 from dual\n"
                "union all\n"
                "select 2 n , 'aaaa' t1,3 n2, 'eeee' t2, 'a1aaa' t3  from dual\n"
                "union all\n"
                "select 3 n, 'bbbb' t1, null n2,  'llll' t2, 'aa2aa' t3   from dual\n"
                "union   all\n"
                "select 4 n, 'cccc' t1, 10 n2, null t2, 'aaa3a' t3  from dual\n"
                "union all\n"
                "select 5 n, null t1, 11 n2, null t2, '' t3  from dual\n"
               ") order by n");

        char w[10];
        char const *w2=w;
        strcpy(w,"X");
        c.setDebug().unstb().autoNull().fetchLen(10).bind(":w",w2).
            def(n[0]).defNull(t1[0],"").
            idef(n2[0],&ind_n2[0]).defNull(t2[0],"").def(t3[0]).exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxxX")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaaX")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbbX")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"ccccX")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"X")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");

        strcpy(w,"YY");
        c.exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxxYY")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaaYY")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbbYY")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"ccccYY")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"YY")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");
        strcpy(w,"");
        c.exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxx")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaa")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbb")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"cccc")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");


    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_combine2)
{
    try {
        int n[10] ; char t1[10][10] ; int n2[10]; short ind_n2[10];
        OciVcs<10> t2[10];string t3[10];

        CursCtl c=make_curs(
                "select n,t1||:w,n2,t2,t3 from (\n"
                "select 1 n , 'xxxx' t1,5 n2,null t2, 'aaaaa' t3 from dual\n"
                "union all\n"
                "select 2 n , 'aaaa' t1,3 n2, 'eeee' t2, 'a1aaa' t3  from dual\n"
                "union all\n"
                "select 3 n, 'bbbb' t1, null n2,  'llll' t2, 'aa2aa' t3   from dual\n"
                "union   all\n"
                "select 4 n, 'cccc' t1, 10 n2, null t2, 'aaa3a' t3  from dual\n"
                "union all\n"
                "select 5 n, null t1, 11 n2, null t2, '' t3  from dual\n"
               ") order by n");

        std::string w2("X");
        c.setDebug().unstb().autoNull().fetchLen(10).bind(":w",w2).
            def(n[0]).defNull(t1[0],"").
            idef(n2[0],&ind_n2[0]).defNull(t2[0],"").def(t3[0]).exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxxX")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaaX")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbbX")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"ccccX")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"X")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");

        w2="YY";
        c.exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxxYY")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaaYY")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbbYY")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"ccccYY")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"YY")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");
        w2="";
        c.exec();
        fail_unless(c.err()==0,"c.exec() error");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==1,"wrong n");
        fail_unless(strcmp(t1[0],"xxxx")==0,"wrong t1");
        fail_unless(n2[0]==5,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaaaa","t3");
        c.fen(2);
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==2,"wrong n");
        fail_unless(strcmp(t1[0],"aaaa")==0,"wrong t1");
        fail_unless(n2[0]==3,"wrong n2");
        fail_unless(string(t2[0].arr,t2[0].len)=="eeee","wrong t2");
        fail_unless(n[1]==3,"wrong n");
        fail_unless(strcmp(t1[1],"bbbb")==0,"wrong t1");
        fail_unless(ind_n2[1]==-1,"n2 is not null");
        fail_unless(string(t2[1].arr,t2[1].len)=="llll","wrong t2");
        fail_unless(t3[0]=="a1aaa","t3");
        fail_unless(t3[1]=="aa2aa","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==4,"wrong n");
        fail_unless(strcmp(t1[0],"cccc")==0,"wrong t1");
        fail_unless(n2[0]==10,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="aaa3a","t3");
        c.fen();
        fail_unless(c.err()==0,"c.fen() error");
        fail_unless(n[0]==5,"wrong n");
        fail_unless(strcmp(t1[0],"")==0,"wrong t1");
        fail_unless(n2[0]==11,"wrong n2");
        fail_unless(t2[0].len==0,"t2 is not null");
        fail_unless(t3[0]=="","t3");
        c.fen();
        fail_unless(c.err()==NO_DATA_FOUND,"c.fen() error");


    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_no_throw)
{
    try {
        char s[10];
        CursCtl c=make_curs("select null from dual");
        c.setDebug().noThrowError(1405).def(s).exfet();
        fail_unless(c.err()==1405,"must be error code for null");

    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_throw_all)
{
    try {
        char s[10];
        CursCtl c=make_curs("select '1' from dual where rownum <1");
        c.setDebug().throwAll().def(s).exfet();
        fail_unless(0,"must throw");
    } catch (const ociexception &e) {
        ProgTrace(TRACE5, "sqlErr=%d", e.sqlErr());
        fail_unless(e.sqlErr()==NO_DATA_FOUND,"must throw no_data_found");
    }
}
END_TEST
START_TEST(test_rowcount)
{
    try {
        int s;
        CursCtl c=make_curs("select 1 from dual union select 2 from dual");
        c.setDebug().def(s).exec();
        c.fen(1);
        c.fen(1);
        if ( c.rowcount_now() != 1 ) fail_unless(0,"rowcount_now()");

    }catch(ociexception &e){
        fail_unless(e.sqlErr()==NO_DATA_FOUND,"must throw no_data_found");
    }
}
END_TEST
START_TEST(test_rowcount_struct)
{
    try {
        struct result {
           int a ;
           int b;
        } re[2];
        CursCtl c=make_curs("select 1 , 2 from dual union select 2 , 2 from dual");
        c.setDebug().fetchLen(1).structSize(sizeof(result)).def(re[0].a).def(re[0].b).exec();
        c.fen(1);
        c.fen(1);
        if ( c.rowcount_now() != 1 ) fail_unless(0,"rowcount_now()");

    }catch(ociexception &e){
        fail_unless(e.sqlErr()==NO_DATA_FOUND,"must throw no_data_found");
    }
}
END_TEST


const int BLEN=100;
struct S1{
    char object_name[40];
    char status[40];
    int object_id;
} ;

START_TEST(test_vector_example1)
{
    try {
        S1 b[BLEN];
        vector<S1> res;
        CursCtl c=make_curs("select object_name,status,object_id"
                " from all_objects where rownum<999");
        c.setDebug().fetchLen(BLEN).structSize(sizeof(S1)).def(b[0].object_name).
            def(b[0].status).def(b[0].object_id).exec();
        while(!c.err()) { // NO_DATA_FOUND, all other throws exceptions
            c.fen(BLEN);
            if(c.rowcount_now()){
               res.insert( res.end(),
                       b,b+c.rowcount_now()); //добавить в конец
            }
        }
//        for(int i=0;i<res.size();++i){
//            cerr << res[i].object_name << " " <<
//                res[i].object_id << " " << res[i].status <<"\n";
//        }
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

//тоже но с конвертацией данных
struct S2 {
    string object_name;
    string status;
    int object_id;
    S2(const S1 &s):object_name(s.object_name),status(s.status),
        object_id(s.object_id){}
        S2(){object_id=0;}//для примера 3
};
START_TEST(test_vector_example2)
{
    try {
        S1 b[BLEN];
        vector<S2> res;
        CursCtl c=make_curs("select object_name,status,object_id"
                " from all_objects where rownum<999");
        c.setDebug().fetchLen(BLEN).structSize(sizeof(S1)).def(b[0].object_name).
            def(b[0].status).def(b[0].object_id).exec();
        while(!c.err()) { // NO_DATA_FOUND, all other throws exceptions
            c.fen(BLEN);
            if(c.rowcount_now()){

               res.insert( res.end(),
                       b,b+c.rowcount_now()); //конверсия на ходу
            }
        }

//        for(int i=0;i<res.size();++i){
//            cerr << res[i].object_name << " " <<
//                res[i].object_id << " " << res[i].status <<"\n";
//        }
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_vector_example3)
{
    try {
        S2 b[BLEN];
        vector<S2> res;
        CursCtl c=make_curs("select object_name,status,object_id"
                " from all_objects where rownum<999");
        c.setDebug().fetchLen(BLEN).structSize(sizeof(S2)).def(b[0].object_name).
            def(b[0].status).def(b[0].object_id).exec();
        while(!c.err()) { // NO_DATA_FOUND, all other throws exceptions
            c.fen(BLEN);
            if(c.rowcount_now()){
               res.insert( res.end(),
                       b,b+c.rowcount_now());
            }
        }

//        for(int i=0;i<res.size();++i){
//            cerr << res[i].object_name << " " <<
//                res[i].object_id << " " << res[i].status <<"\n";
//        }
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(temporary_bind_vars)
{
    try {
        int a1=1,b1=2,a2,b2;
        char var1[]=":qn";
        char var2[]=":q";
        CursCtl c=make_curs("select :qn,:q from dual");
        c.setDebug().bind(var1,a1).bind(var2,b1);
        var1[0]=0;var2[0]=0;
        c.def(a2).def(b2).exfet();
        fail_unless(a1==a2,"wrong");
        fail_unless(b1==b2,"wrong");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_sqlstr_limit)
{
  short code_ind ;
  char buf[6000] ;
  memset( buf , ' ' , 5999 );
  buf[5999] = 0 ;
  if(Oparse(CU,"SELECT  RPAD( ' ' , 5000 , '8' ) FROM DUAL " ) ||
     Idefin(CU,1,buf,5800,SQLT_STR,&code_ind)||
     Oexec(CU))
  {
    fail_unless(0, OciCpp::error2( CU).c_str());
  }
  if ( buf[5666] != ' ' && buf[3999] != '8' )
  {
    ProgTrace( TRACE1 , "result %c" , buf[5666] );
    fail_unless(0, "less 4000" );
  }
}
END_TEST
START_TEST(test_ref_bind)
{
    try{
        int i=1;
        int x=0;
        CursCtl c=make_curs("select :i from dual");
        c.setDebug().unstb().bind(":i",i).def(x).exfet();
        if ( x != i )
         fail_unless(0,"no work bind");
        i = 2;
        c.exfet();
        if ( x != i )
         fail_unless(0,"no work unstable bind");
    }catch(...){
        fail_unless(0,"wrong exception");
    }
}
END_TEST
START_TEST(test_bad_error)
{
    try{ make_curs("lol").exec(); } catch (...){ ProgTrace(TRACE1,"Ignore"); }
    try{ make_curs("lol").exec(); } catch (...){ ProgTrace(TRACE1,"Ignore"); }
}
END_TEST
START_TEST(test_ignore_select_list)
{
    try {
        OciCpp::CursCtl c=make_curs("select 1 from dual where 1=2");
        c.exfet();
        fail_unless(c.err()==NO_DATA_FOUND,"not no data found");
    }catch(ociexception &e){
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_oci_rowid)
{
        ROWIDOCI rowid;
        OciCpp::CursCtl c=make_curs("select rowid from dual");
        c.
                def(rowid).
                exfet();

        fail_unless(rowid.len > 0, "inv rowid length");
        LogTrace(TRACE1) << "rowid from dual is: " << rowid;

        OciCpp::CursCtl c2=make_curs(
                "declare\n"
                "rid rowid;\n"
                "begin\n"
                "select rowid into rid from dual;\n"
                "if rid != :rid_in then\n"
                "   raise_application_error(-20001,'jopa');\n"
                "end if;\n"
                "end;"
                                    );
}
END_TEST

START_TEST(test_char_buffer)
{
    char buff[10];
    memset(buff, '0', sizeof(buff));
    buff[9] = 0;
    LogTrace(TRACE5) << "before buffer: " << buff << " length=" << strlen(buff);
    make_curs("select '123456' from dual")
        .def(buff)
        .exfet();
    LogTrace(TRACE5) << "after buffer: " << buff << " length=" << strlen(buff);
    fail_unless(strlen(buff) == 6, "char buffer not NULL-terminated");
}
END_TEST

START_TEST(test_bind_null_ptr)
{
    try
    {
        char* nullPtr = NULL;
        make_curs("select :val from dual")
            .bind(":val", nullPtr)
            .exfet();
    }
    catch (const ociexception& e)
    {
        e.print(TRACE5);
        LogTrace(TRACE5) << e.what();
        return;
    }
    fail_unless(0,"bind(NULL) must throw exception");
}
END_TEST

START_TEST(test_double_cursors)
{
    try
    {
        int tmp1;
        CursCtl cr1 = make_curs("select 1 from dual union select 2 from dual union select 3 from dual");
        cr1.def(tmp1).exec();
        while(!cr1.fen())
        {
            LogTrace(TRACE5) << "cr1: error_text:" << cr1.error_text();

            int tmp2;
            CursCtl cr2 = make_curs("select 1 from dual where :res = 1");

            LogTrace(TRACE5) << "cr2: error_text:" << cr2.error_text();
            cr2.def(tmp2).bind(":res2", 23);

            LogTrace(TRACE5) << "cr1: error_text:" << cr1.error_text();
            LogTrace(TRACE5) << "cr2: error_text:" << cr2.error_text();
            cr2.exec();

            LogTrace(TRACE5) << "cr1: error_text:" << cr1.error_text();
            LogTrace(TRACE5) << "cr2: error_text:" << cr2.error_text();
        }
        fail_unless(0,"MUST be ociexception");
    }
    catch(const ociexception& e)
    {
        LogTrace(TRACE5) << e.what();
    }
}
END_TEST

START_TEST(test_empty_str_is_null)
{
    int res = 0;
    string emptyStr;
    make_curs("select 1 from dual where :emptyStr is null")
        .bind(":emptyStr", emptyStr).def(res)
        .exfet();
    fail_unless(res == 1, "empty string is null failed");
}
END_TEST

START_TEST(test_Obndri)
{
    int var = 2;
    int outVar = 0;
    if(Oparse(CU, "begin SELECT DECODE(:var, 2, 1, -1) INTO :outVar FROM DUAL; end;") ||
            Obndri(CU,":var", var) ||
            Obndri(CU, ":outVar", outVar) ||
            Oexec(CU)) {
        OciCpp::error(CU, OciCpp::mainSession(), STDLOG);
        fail_unless(0,"sql error");
    }
    fail_unless(var == 2, "var != 2, var=%d", var);
    fail_unless(outVar == 1, "outVar != 1, outVar=%d", outVar);
}
END_TEST

START_TEST(test_cursorNoCache)
{
    fail_unless(getCursorCache().size() == 1, "bad initial cursor cache");
    const size_t sz = 10;
    for (size_t i = 0; i < sz; ++i) {
        stringstream str;
        str << "select " << i << " from dual";

        int tmp = 0;
        {
            make_curs_no_cache(str.str()).def(tmp).EXfet();
        }
        fail_unless(getCursorCache().size() == 1+i, "bad cursor cache: %d", getCursorCache().size());

        stringstream str2;
        str2 << "select nvl(" << i << ", 1) from dual";
        {
            make_curs(str2.str()).def(tmp).EXfet();
        }
        fail_unless(getCursorCache().size() == i + 2, "bad cursor cache: %d", getCursorCache().size());
    }
    const size_t cacheSz = getCursorCache().size();
    for (size_t i = 0; i < 1000; ++i) {
        stringstream str;
        str << "select " << i << " from dual";

        int tmp = 0;
        {
            make_curs_no_cache(str.str()).def(tmp).EXfet();
        }
        fail_unless(getCursorCache().size() == cacheSz, "bad cursor cache: %d", getCursorCache().size());
    }
}
END_TEST

START_TEST(test_isgooderr)
{
    fail_unless(getCursorCache().size() == 1, "bad initial cursor cache");
    const size_t sz = 10;
    for (size_t i = 0; i < sz; ++i) {
        int tmp = 0;
        make_curs("select 2 from dual where 1=2").def(tmp).EXfet();
        make_curs("select 2 from dual where 1=1").def(tmp).EXfet();
        fail_unless(getCursorCache().size() == 3, "bad cursor cache: %d", getCursorCache().size());

        try {
            // when bad error cursor must be removed from cache
            stringstream str2;
            str2 << "select2 " << i << " from dual";
            make_curs(str2.str()).def(tmp).EXfet();
        } catch (const ociexception& e) {
            ProgTrace(TRACE5, "ociexception: %s", e.what());
        }
        fail_unless(getCursorCache().size() == 3, "bad cursor cache: %d", getCursorCache().size());
    }
}
END_TEST

START_TEST(test_bind_bad_var)
{
    try {
        CursCtl cr = make_curs("select 1 from dual where :n=2");
        cr.bind(":n2", 2);
        cr.EXfet();
        fail_unless(0,"must throw exception");
    } catch (const ociexception& e) {
        ProgTrace(TRACE5, "%s", e.what());
        fail_unless(e.sqlErr() == CERR_BIND, "not valid error code");
    }
}
END_TEST

START_TEST(test_cache_no_cache)
{
    const std::string sqlStr("select 1 from dual");
    int var1 = 0;
    CursCtl cr1 = make_curs(sqlStr);
    cr1.def(var1);
    {
        int var2 = 0;
        CursCtl cr2 = make_curs_no_cache(sqlStr);
        cr2.def(var2);
        cr2.EXfet();
        fail_unless(var2 == 1, "bad var2: %d", var2);
    }
    cr1.EXfet();
    fail_unless(var1 == 1, "bad var1: %d", var1);
}
END_TEST

START_TEST(test_check_row_count)
{
    try {
        make_curs("drop table TEST_CHECK_ROW_COUNT").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        ProgTrace(TRACE5, "create table TEST_CHECK_ROW_COUNT");
        make_curs("create table TEST_CHECK_ROW_COUNT (VAL NUMBER)").exec();

        make_curs("insert into TEST_CHECK_ROW_COUNT values (1)").exec();
        make_curs("insert into TEST_CHECK_ROW_COUNT values (2)").exec();

        make_curs("update TEST_CHECK_ROW_COUNT set val = val + 3")
            .checkRowCount(2)
            .exec();
        try {
            make_curs("update TEST_CHECK_ROW_COUNT set val = val + 3 where val < 0")
                .checkRowCount(2)
                .exec();
            fail_unless(0,"failed nothing found");
        } catch (const ociexception& e) {
            ProgTrace(TRACE5, "ociexception: %s", e.what());
        }
    } catch (...) {
        ProgTrace(TRACE5, "drop table TEST_CHECK_ROW_COUNT");
        make_curs("drop table TEST_CHECK_ROW_COUNT").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    }
    commit();
}
END_TEST

/**
 * checks bind with var name in temporary string
 * */
START_TEST(test_check_string_plname)
{
#if 0
    {
        int tmp;
        OciCpp::CursCtl cr = make_curs("select 1 from dual where :var1 = :var2");
        cr.stb().def(tmp);
        {   cr.bind(string("var") + "1", 25); }
        {   cr.bind(string("var") + "2", 25); }
        cr.exfet();
        fail_unless(cr.rowcount() == 1, "nothing selected - must be one row");
    }
    {
        int ret = 0;
        OciCpp::CursCtl cr = make_curs("begin :var3 := 3; end;");
        {
            string tmpStr("var3");
            cr.bindOut(tmpStr, ret);
            tmpStr[0] = 'A';
        }
        cr.exec();
        fail_unless(ret == 3, "nothing selected");
    }
    {
        int ret1 = 0, ret2 = 0;
        const int defRet1 = 3, defRet2 = 4;
        OciCpp::CursCtl cr = make_curs("begin :var3 := NULL; :var4 := NULL; end;");
        {
            string tmpStr1("var3"), tmpStr2("var4");
            cr.bindOutNull(tmpStr1, ret1, defRet1);
            tst();
            cr.bindOutNull(tmpStr2, ret2, &defRet2);
            tmpStr1[0] = 'A';
            tmpStr2[0] = 'A';
        }
        cr.exec();
        fail_unless(ret1 == 3, "nothing selected");
        fail_unless(ret2 == 4, "nothing selected");
    }
    {
        char ret[100] = {0};
        OciCpp::CursCtl cr = make_curs("begin :var3 := 'lalala'; end;");
        {
            string tmpStr("var3");
            cr.bindFull(tmpStr, reinterpret_cast<void*>(&ret[0]), sizeof(ret), 0, 0, SQLT_LNG);
            tmpStr[0] = 'A';
        }
        cr.exec();
        fail_unless(strcmp(ret, "lalala") == 0, "nothing selected");
    }
#endif
}
END_TEST

START_TEST(test_datetime_bindout)
{
    try
    {
        oracle_datetime date;
        make_curs("begin :s:=to_date('12.09.1991', 'DD.MM.YYYY'); end;").bindOut(":s", date).exec();
        std::ostringstream s;
        s << date;
        fail_unless(s.str() == "'1991.09.12 00:00:00'", "oracle_datetime bindout failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_datetime_def)
{
    try
    {
        oracle_datetime date;
        make_curs("select to_date('12.09.1991', 'DD.MM.YYYY') from dual").def(date).EXfet();
        std::ostringstream s;
        s << date;
        fail_unless(s.str() == "'1991.09.12 00:00:00'", "oracle_datetime def failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_datetime_bind)
{
    try
    {
        std::string s;
        s.resize(20, '.');
        oracle_datetime date(1991, 9, 12);
        make_curs("select to_char(:s+1, 'DD.MM.YY') from dual").bind(":s", date).def(s).EXfet();
        fail_unless(s == "13.09.91", "oracle_datetime bind failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_boostdt_bind)
{
    try
    {
        dt::date date(1991, 9, 12);
        dt::date new_date;
        make_curs("select :s from dual").bind(":s", date).def(new_date).EXfet();
        fail_unless(dt::to_iso_extended_string(new_date) == "1991-09-12", "dt::date bind failed");
        dt::ptime time(date, dt::time_duration(12, 59, 59));
        dt::ptime new_time;
        make_curs("select :s from dual").bind(":s", time).def(new_time).EXfet();
        fail_unless(dt::to_iso_extended_string(new_time) == "1991-09-12T12:59:59", "dt::ptime bind failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_boostdt_bind2)
{
    try
    {
        dt::date date (dt::day_clock::local_day()+dt::days(1) );
        OciCpp::CursCtl c= make_curs("select 1 from dual where trunc(sysdate)+1=:s");
        c.bind(":s", date).EXfet();
        fail_unless(c.rowcount()==1, "dt::ptime bind failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_boostdt_not_date)
{
    dt::date date;
    dt::date new_date;
    make_curs("select :s from dual").autoNull().bind(":s", date).def(new_date).EXfet();
    fail_unless(new_date.is_not_a_date(), "dt::date not a date failed");
    dt::ptime time;
    dt::ptime new_time;
    make_curs("select :s from dual").autoNull().bind(":s", time).def(new_time).EXfet();
    fail_unless(new_time.is_not_a_date_time(), "dt::ptime not a datetime failed");
    dt::ptime time2;
    int select_result = 0;
    make_curs("select 1 from dual where :t is null").bind(":t", time2).def(select_result).EXfet();
    fail_unless(1 == select_result, "default value boost::posix_time::ptime() bind to not null");
}
END_TEST

START_TEST(test_boostdt_default_value)
{
    try
    {
        dt::date date(1991, 9, 12);
        dt::date new_date;
        make_curs("select null from dual").defNull(new_date, date).EXfet();
        fail_unless(dt::to_iso_extended_string(new_date) == "1991-09-12", "dt::date default value failed");
        dt::ptime time(date, dt::time_duration(12, 59, 59));
        dt::ptime new_time;
        make_curs("select null from dual").defNull(new_time, time).EXfet();
        fail_unless(dt::to_iso_extended_string(new_time) == "1991-09-12T12:59:59", "dt::ptime default value failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_boostdt_neg_pos_inf)
{
    try
    {
        dt::date date(dt::pos_infin);
        dt::date new_date;
        make_curs("select :s from dual").bind(":s", date).def(new_date).exfet();
        fail_unless(new_date.is_pos_infinity(), "test_boostdt_pos_inf failed");
        dt::date date2(dt::neg_infin);
        dt::date new_date2;
        make_curs("select :s from dual").bind(":s", date2).def(new_date2).exfet();
        fail_unless(new_date2.is_neg_infinity(), "test_boostdt_neg_inf failed");

        {
        dt::ptime time(dt::pos_infin), new_time;
        make_curs("select :s from dual").bind(":s", time).def(new_time).exfet();
        fail_unless(new_time.is_pos_infinity(), "test_boost_ptime_pos_infin failed");
        }
        {
        dt::ptime time(dt::neg_infin), new_time;
        make_curs("select :s from dual").bind(":s", time).def(new_time).exfet();
        fail_unless(new_time.is_neg_infinity(), "test_boost_ptime_neg_infin failed");
        }
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_boostdt_bindOut)
{
    dt::date dt;
    make_curs("begin select sysdate into :dt from dual; end;").unstb().bindOut(":dt", dt).exec();
    fail_if(dt.is_special());
    dt::ptime pt;
    make_curs("begin select sysdate into :pt from dual; end;").unstb().bindOut(":pt", pt).exec();
    fail_if(pt.is_special());
}
END_TEST
START_TEST(test_def_very_large_number)
{
    try {
        int var = 0;
        make_curs("select 123456789012345678901234567890 from dual")
            .def(var)
            .EXfet();
        ProgTrace(TRACE5, "var=%d", var);
        fail_unless(0,"var=%d", var);
    } catch (const ociexception& e) {
        ProgTrace(TRACE5, "e=%s", e.what());
    }
}
END_TEST

START_TEST(test_split_connect_string)
{
    allowProgError();
    struct TParseConnectStringStruct {
        std::string connect_string;
        std::string login;
        std::string password;
        std::string server;
    };

    TParseConnectStringStruct vec [] = {
        {"sirena/orient", "sirena", "orient" , ""},
        {"sirena/orient@sir9", "sirena", "orient", "sir9"},
        {"sir_trunk/lolka@oracle1.komtex/build", "sir_trunk", "lolka" , "oracle1.komtex/build"},
        {"sirena@sir9/orient", "sirena", "orient" , "sir9"},
    };
    for (size_t i = 0; i < sizeof(vec)/sizeof(vec[0]); ++i) {
        const TParseConnectStringStruct& test = vec[i];
        oracle_connection_param res;
        split_connect_string(test.connect_string, res);
        fail_unless(res.login == test.login, "failed %s login: %s != %s",
                test.connect_string.c_str(), test.login.c_str(), res.login.c_str());
        fail_unless(res.server == test.server, "failed %s server: %s != %s",
                test.connect_string.c_str(), test.server.c_str(), res.server.c_str());
        fail_unless(res.password == test.password, "failed %s password: %s != %s",
                test.connect_string.c_str(), test.password.c_str(), res.password.c_str());
    }

    TParseConnectStringStruct badVec[] = {
        {"sir_trunk@oracle1.komtex/build/lolka", "", "" , ""},
        {"sirena@sir9", "", "" , ""}
    };
    for (size_t i = 0; i < sizeof(badVec)/sizeof(badVec[0]); ++i) {
        try {
            oracle_connection_param res;
            split_connect_string(badVec[i].connect_string, res);
            fail_unless(0,"must be exception on %s", badVec[i].connect_string.c_str());
        } catch (const comtech::Exception& e) {
            // must be here
        }
    }
}
END_TEST

START_TEST(test_bind_out_null)
{
    int intVal = 0;
    char strVal[10] = {};
    make_curs("create or replace procedure test_bind_out_null(intVal in number, strVal in out varchar2) as\n"
            "begin\n"
            "  dbms_output.put_line('intVal='||to_char(intVal)||' strVal='||strVal);\n"
            "end;")
        .exec();
    try {
        make_curs("begin test_bind_out_null(:invVal, :strVal); end;")
            .autoNull()
            .bindOut(":invVal", intVal).bindOut(":strVal", strVal)
            .exec();
    } catch (const ociexception& e) {
        fail_unless(0,"%s", e.what());
    }
    make_curs("drop procedure test_bind_out_null")
        .exec();
    commit();
}
END_TEST

START_TEST(test_cache_sql_error)
{
    // This test checks that we cache only 'serious' errors.
    // When we get CERR_NULL it's ok for cached cursor.
    // But in case of 'no statement parsed' (it happens for example when table name is invalid)
    // we cache result of Oparse and cached cursor stays invalid.
    // May be it would be better to wipe out this cursor from cache.
    // It can help against extra restart after fixing DB object.
    try {
        make_curs("drop table TEST_CHECK_SQL_ERROR").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("drop table TEST_CHECK_SQL_ERROR2").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("create table TEST_CHECK_SQL_ERROR (VAL NUMBER)").exec();

        make_curs("insert into TEST_CHECK_SQL_ERROR values (NULL)").exec();

        const char* sql1 = "select val from TEST_CHECK_SQL_ERROR";

        int val = 0;
        try {
            make_curs(sql1).def(val).EXfet();
            fail_unless(0,"must throw NULL");
        } catch (const ociexception& e) {
            fail_unless(e.sqlErr() == CERR_NULL, "here must be CERR_NULL error");
        }
        ProgTrace(TRACE5, "val=%d", val);
        fail_unless(val == 0, "val=%d but must be 0", val);

        make_curs("update TEST_CHECK_SQL_ERROR set val = 1").exec();
        make_curs(sql1).def(val).EXfet();
        ProgTrace(TRACE5, "val=%d", val);
        fail_unless(val == 1, "val=%d but must be 1", val);

        const char* sql2 = "select val from TEST_CHECK_SQL_ERROR2";
        try {
            make_curs(sql2).def(val).EXfet();
            fail_unless(0,"must throw CERR_TABLE_NOT_EXISTS");
        } catch (const ociexception& e) {
            LogTrace (TRACE1) << "code "<<e.sqlErr();
            fail_unless(e.sqlErr() == CERR_TABLE_NOT_EXISTS, "here must be CERR_TABLE_NOT_EXISTS error");
        }

        make_curs("create table TEST_CHECK_SQL_ERROR2 (VAL NUMBER)").exec();

        if (make_curs(sql2).def(val).EXfet() != NO_DATA_FOUND)
            fail_unless(0,"must parse and exec and return NO_DATA_FOUND");

        const char* sql3 = "begin\n"
                           "raise_application_error(-20101,'test');\n"
                           "end;";
        try
        {
            make_curs( sql3 ).exec();
            fail_unless(0,"must throw user defined exception");
        } catch (const ociexception& e) {
            fail_unless(e.sqlErr() == 20101, "here must be 20101 error");

            if( !isCursorInCache(sql3) )
                fail_unless(0,"cursor must be in cache");
        }
    } catch (...) {
        ProgTrace(TRACE5, "drop table TEST_CHECK_SQL_ERROR");
        make_curs("drop table TEST_CHECK_SQL_ERROR").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("drop table TEST_CHECK_SQL_ERROR2").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        fail_unless(0,"unknown exception catched");
    }

    printCursorCache();

    commit();
}END_TEST

START_TEST(test_several_session)
{
    int val1 = 0, val2 = 0;
    OciSession sess1(STDLOG, get_connect_string());
 //   sess1.set7();
    OciSession sess2(STDLOG, get_connect_string());
  //  sess2.set7();

    OciCpp::CursCtl cr1 = sess1.createCursor(STDLOG, "select 1 from dual");
    OciCpp::CursCtl cr2 = sess2.createCursor(STDLOG, "select 2 from dual");

    cr1.def(val1).EXfet();
    cr2.def(val2).EXfet();

    fail_unless(val1 == 1, "invalid val1: %d", val1);
    fail_unless(val2 == 2, "invalid val1: %d", val2);

    {
        // ошибка, которая приводит к удалению курсора из кеша
        // проверяем, что удаляется именно из кеша вспомогательной сессии, а не главной
        OciSession sess3(STDLOG, get_connect_string());
//        sess3.set7();
        for (size_t i = 0; i < 10; ++i) {
            try {
                OciCpp::CursCtl(sess3.cdaCursor("select :v from dual where :v = :v",true), STDLOG)
                    .def(val1)
                    .bind(":v2", 123)
                    .EXfet();
            } catch (const ociexception& e) {
            }
            LogTrace(TRACE1) <<getCursorCache(sess3).size() ;
            fail_unless(getCursorCache(sess3).size() == 0);
        }
    }
}END_TEST

START_TEST(test_switch_session_mode)
{
    try {
        OciCpp::OciSession& mainSess = OciCpp::mainSession();
        OciCpp::CursCtl cr(make_curs("SELECT 1 FROM DUAL UNION SELECT 2 FROM DUAL"));
        cr.exec();
        if (alwaysOCI8()) {
            mainSess.set7();
        } else {
            mainSess.set8();
        }
        cr.fen();
    } catch(const ociexception& e) {
        return;
    }

    fail_unless(false, "The error must be in case of changing the session mode.");
}END_TEST

START_TEST(test_several_dummy_session)
{
    OciSession sess1(STDLOG, get_connect_string());
    OciSession sess2(STDLOG, get_connect_string());
    OciSession sess3(STDLOG, get_connect_string());
}END_TEST

START_TEST(test_session_error_mem_leak)
{
  // Alloc all static ora variables
  {
    OciSession sess1(STDLOG, get_connect_string());
  }

  std::string connect_string="a/a";

  size_t mem_before= size_of_allocated_mem();

  // Find invalid connect string
  for(;;)
  {
    try {
      OciSession a(STDLOG, connect_string);
    } catch (OciCpp::ociexception const &e) {
      break;
    }
    connect_string+="a";
  }

  // Make fail connects
  for(int i=0; i<200; ++i)
  {
    try {
      OciSession a(STDLOG, connect_string);
    } catch (OciCpp::ociexception const &e) { }
  }

  size_t mem_after= size_of_allocated_mem();

  // ASM:
  //with err:  capacity_before=3191000 capacity_after=3119500 leak=71500   on 1000 connects
  //with err:  capacity_before=3191100 capacity_after=2519600 leak=671500  on 10000 connects
  //fixed:     capacity_before=3190900 capacity_after=3189600 leak=1300    on 1000 connects
  //fixed:     capacity_before=3191100 capacity_after=3189300 leak=1800 Kb on 5000 connects
  //fixed:     capacity_before=3191100 capacity_after=3188800 leak=2300    on 10000 connects

  // Box:
  //with err:  capacity_before=2939600 capacity_after=2838500 leak=101100 Kb on 200 connects
  //fixed:     capacity_before=2939600 capacity_after=2870100 leak=69500 Kb on 200 connects

  LogTrace(TRACE5) <<"mem_before=" <<mem_before <<" mem_after=" <<mem_after
      <<" leak=" << (mem_after <= mem_before ? 0 : mem_after - mem_before) <<" Kb";
  fail_unless(mem_after <= mem_before || mem_after - mem_before < 6000/*Kb*/, "memory leak detected");
}END_TEST

START_TEST(test_session_destroy_mem_leak)
{
  // Alloc all static ora variables
  {
    OciSession sess1(STDLOG, get_connect_string());
  }

  std::string connect_string=get_connect_string();

  size_t mem_before= size_of_allocated_mem();

  // Make connects
  for(int i=0; i<100; ++i)
  {
    OciSession a(STDLOG, connect_string);
  }

  size_t mem_after= size_of_allocated_mem();

  // ASM:
  //with err:  capacity_before=3191200 capacity_after=3177400 leak=13800 Kb on 100 connects
  //fixed:     capacity_before=3191100 capacity_after=3190000 leak=1100 Kb  on 50 connects
  //fixed:     capacity_before=3191000 capacity_after=3189700 leak=1300 Kb  on 75 connects
  //fixed:     capacity_before=3191100 capacity_after=3189800 leak=1300 Kb  on 100 connects

  // Box:
  //with err:  capacity_before=2939600 capacity_after=2819300 leak=120300 Kb on 125 connects
  //fixed:     capacity_before=2939700 capacity_after=2869300 leak=70400 Kb on 125 connects

  LogTrace(TRACE5) <<"mem_before=" <<mem_before <<" mem_after=" <<mem_after
      <<" leak=" <<(mem_after <= mem_before ? 0 : mem_after - mem_before) <<" Kb";

  fail_unless(mem_after <= mem_before || mem_after - mem_before < 6000 /*Kb*/, "memory leak detected");
}END_TEST

START_TEST(test_secondary_initdb)
{
  testInitDB();
}
END_TEST;

START_TEST(test_bind_dups)
{
    try {
        make_curs("DROP TABLE TEST_BIND_DUPS").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("CREATE TABLE TEST_BIND_DUPS (VALUE NUMBER)").exec();

        CursCtl query = make_curs("INSERT INTO TEST_BIND_DUPS VALUES (:VALUE)");
        int val = 123;
        query.unstb().setDebug().bind(":VALUE", val).exec();
        val = 456;
        query.exec();
        int value = 0;
        CursCtl select = make_curs("SELECT VALUE FROM TEST_BIND_DUPS");
        select.def(value).exec();
        select.fen();
        fail_unless(value == 123, "broken bind: %i != 123", value);
        select.fen();
        fail_unless(value == 456, "broken bind: %i != 456", value);

//        CursCtl query = make_curs("INSERT INTO TEST_BIND_DUPS VALUES (:VALUE)");
//        for (int i = 0; i < 10000; ++i) {
//            query.setDebug().bind(":VALUE", i).exec();
//            int value = 0;
//            make_curs("SELECT VALUE FROM TEST_BIND_DUPS").def(value).EXfet();
//            if (i != value) fail_unless(0,"no work bind");
//            make_curs("DELETE FROM TEST_BIND_DUPS").exec();
//        }
    } catch(ociexception &e) {
        fail_unless(0,e.what());
    }
    commit();
}
END_TEST;

START_TEST(test_dump_table)
{
    try {
        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
        make_curs("CREATE TABLE TEST_CHECK_DUMP_TABLE (VALUE1 NUMBER, VALUE2 VARCHAR2(30))").exec();
        make_curs("INSERT INTO TEST_CHECK_DUMP_TABLE (VALUE1, VALUE2) VALUES (1234, 'lolkalolka')").exec();

        DumpTable("test_check_dump_table").addFld("VALUE2").where("VALUE1 = 1234").exec(TRACE5);
        DumpTable("TEST_CHECK_DUMP_TABLE").order("VALUE1").exec(TRACE5);
        DumpTable("TEST_CHECK_DUMP_TABLE").addFld("value1,value2").exec(TRACE5);

        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    } catch (...) {
        make_curs("DROP TABLE TEST_CHECK_DUMP_TABLE").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    }
}END_TEST

START_TEST(test_oracle_datetime)
{
    OciCpp::oracle_datetime oraDt1(2009, 5, 13, 16, 32, 34);
    std::ostringstream s;
    s << oraDt1;
    fail_unless(s.str() == "'2009.05.13 16:32:34'");
    OciCpp::oracle_datetime oraDt2 = OciCpp::to_oracle_datetime(boost::gregorian::date(boost::gregorian::pos_infin));
    OciCpp::oracle_datetime oraDt3 = OciCpp::to_oracle_datetime(boost::gregorian::date(boost::gregorian::neg_infin));
    int res = make_curs("select 1 from dual where :dt1 between :dt2 and :dt3")
        .bind(":dt1", oraDt1)
        .bind(":dt2", oraDt2)
        .bind(":dt3", oraDt3)
        .EXfet();
    fail_unless(res != CERR_OK, "invalid res=%d", res);
} END_TEST

START_TEST(test_std_array)
{
    {
    std::array<uint8_t,3> m;
    m.fill('_');
    int res = make_curs("select utl_raw.cast_to_raw('abc') from dual where 1=1").def(m).EXfet();
    fail_unless(res == CERR_OK, "invalid res=%d", res);
    fail_unless(m[0] == 'a' and m[1] == 'b' and m[2] == 'c', "fetched : <%s>", reinterpret_cast<const char*>(m.data()));
    }
    {
    const std::array<uint32_t,2> m32 = {{'_','_'}};
    std::array<uint32_t,2> m32_ = {{'*','*'}};
    int res = make_curs("select :m32 from dual").setDebug(true).bind(":m32",m32).def(m32_).EXfet();
    fail_unless(res == CERR_OK, "invalid res=%d", res);
    fail_unless(m32 == m32_);
    //fail_unless(m32 == "aBcDeFhIjKlMn");
    }
} END_TEST

START_TEST(test_stable_bindout_correct)
{
    int t = 10;
    make_curs("begin :s1 := 123; end;")
        .stb()
        .bindOut(":s1", t).exec();
    fail_unless(t == 123);
}
END_TEST

START_TEST(test_const_string_bind_stable_correct)
{
    try
    {
        const std::string string = "abc";
        std::string my_str;
        make_curs("select :s from dual").setDebug().stb().def(my_str).bind(":s", string).exfet();
        fail_unless(my_str == "abc", "test_const_string_bind_stable_correct failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_char_pointer_bind_stable_correct)
{
    try
    {
        const char* ptr = "abc";
        char other_ptr[4];
        make_curs("select :s from dual").stb().bind(":s", ptr).def(other_ptr).EXfet();
        fail_unless(strcmp(other_ptr, "abc") == 0, "test_char_pointer_bind_stable_correct failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_indicator_change)
{
    try
    {
        std::string a("hello");
        std::string d;
        indicator ind = iok;
        CursCtl c = make_curs("select :s from dual");
        c.unstb().bind(":s", a, &ind).def(d).EXfet();
        fail_unless(d == a, "test_indicator_change failed");
        ind = inull;
        try
        {
            c.EXfet();
            fail_unless(0,"test_indicator_change failed: must be exception");
        }
        catch(const ociexception& e)
        {
        }
        ind = iok;
        c.EXfet();
        fail_unless(d == a, "test_indicator_change failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST

START_TEST(test_odescr)
{
    try
    {
        std::string s;
        CursCtl c = make_curs("select rpad('a', 3000, 'a') from dual");
        c.def(s).exfet();
        fail_unless(s == std::string(3000, 'a'), "test_odescr failed");
    }
    catch(const ociexception& e)
    {
        fail_unless(0,e.what());
    }
}
END_TEST
START_TEST(test_raw8_read_oci)
{
      OCIStmt   *stmthp;
      OciSession::OciSessionNativeData const & native=mainSession().native();
      char const * sql="begin  :raw1:=utl_raw.cast_to_raw('hello'); end;";
      OCIStmtPrepare2(native.svchp, &stmthp, native.errhp,
            (OraText *)sql, strlen(sql),
            (OraText *)sql, strlen(sql),
            OCI_NTV_SYNTAX, OCI_DEFAULT);





      char buf[1000];
      sb2 ind=0;
      ub2 alen=0;
      ub2 rcode=0;
      int status;
      OCIBind  *bnd1p = (OCIBind *) 0;
      if ((status = OCIBindByName(stmthp, &bnd1p, native.errhp, (text *) ":raw1",
                 5 , (dvoid *) buf,
                 sizeof(buf)-1, SQLT_BIN,
                 (ub2 *)&ind, &alen, &rcode,  0,  0, OCI_DEFAULT)) )
      {
          fail_unless(0,"bind") ;
      }


      if ((status = OCIStmtExecute(native.svchp, stmthp, native.errhp,  1,  0,
                      0, 0, OCI_DEFAULT))
              && status != 1)
      {
          fail_unless(0,"exec") ;
      }
      fail_unless(memcmp(buf,"hello",5)==0);
}
END_TEST
START_TEST(test_raw8_read)
{
    char buf[1000];
    ub2 len=0;
    sb2 ind=0;


      char const * sql="begin  :raw1:=utl_raw.cast_to_raw('hello'); end;";
    OciCpp::CursCtl(sql).bindFull(":raw1",buf,sizeof(buf)-1,&ind,&len,SQLT_BIN).exec();
      fail_unless(memcmp(buf,"hello",5)==0);
}
END_TEST

//time test
/*START_TEST(test_bind_date_speed)
{
    #define MAKE_CURS(query, bind_date, def_date) make_curs(query).bind(":s", bind_date).def(def_date).exfet()
    const char* sql = "select :s from dual";
    const size_t n = 100000;

    Timer::timer t;
    dt::date date(1991, 9, 12);
    dt::date other_date;
    for (size_t i = 0; i < n; ++i)
    {
        MAKE_CURS(sql, date, other_date);
    }
    LogTrace(TRACE5) << __FUNCTION__ << " " << n << " date: " << t.elapsed();
    oracle_datetime ora_dt(OciCpp::to_oracle_datetime(date));
    oracle_datetime other_ora_dt;
    t.restart();
    for (size_t i = 0; i < n; ++i)
    {
        MAKE_CURS(sql, ora_dt, other_ora_dt);
    }
    LogTrace(TRACE5) << __FUNCTION__ << " " << n << " ora_dt: " << t.elapsed();
    std::string dt_string = std::to_string(date, "%y%m%d");
    std::string other_dt_string;
    const char* string_sql = "select TO_CHAR(TO_DATE(:s, 'YYMMDD'), 'YY-MM-DD') from dual";
    t.restart();
    for (size_t i = 0; i < n; ++i)
    {
        MAKE_CURS(string_sql, dt_string, other_dt_string);
    }
    LogTrace(TRACE5) << __FUNCTION__ << " " << n << " dt_string: " << t.elapsed();
}
END_TEST*/

START_TEST(test_def_null_changes)
{
    const char* sql = "select null from dual";
    dt::date date(1991, 9, 12);
    indicator ind = 0;
    make_curs(sql).def(date, &ind).EXfet();
    fail_unless(date == dt::date(1991, 9, 12), "date_def_failed");
    dt::ptime time(date, dt::time_duration(12, 59, 59));
    ind = 0;
    make_curs(sql).def(time, &ind).EXfet();
    fail_unless(time == dt::ptime(date, dt::time_duration(12, 59, 59)), "time_def_failed");
}
END_TEST

START_TEST(test_max_open_cursors)
{
  bool with_exception=false;
  std::vector<string> sqls;
  for(int i=0; i<10000; ++i)
  {
    char sql[200];
    sprintf(sql,"SELECT %i FROM DUAL",i);
    try
    {
      sqls.push_back(sql);
      make_curs(sql).exec();
    }
    catch (ociexception const &e)
    {
      with_exception=true;
      break;
    }
  }
  for(size_t j=0;j<sqls.size();++j){
      mainSession().removeFromCache(sqls.at(j).c_str());
  }

  fail_unless(with_exception, "waiting for exception w/o kill");
}
END_TEST

START_TEST(test_bindArray1)
{
    for(int i=0;i<2;++i){
        try{
            char const *s=
                "declare\n"
                "type intl is table of integer index by binary_integer;\n"
                "type charl is table of varchar2(4) index by binary_integer;\n"
                "int_var1 intl;\n"
                "int_var2 intl;\n"
                "char_var1 charl;\n"
                "char_var2 charl;\n"
                "ind binary_integer;\n"
                "begin\n"
                "int_var1:=:arr1;\n"
                "int_var2:=int_var1;\n"
                ":arr2:=int_var2;\n"

                "end;\n";

            int arr1[5]={1,2,3,4,5};
            int arr2[5]={};
            ub4 ml1=5,ml2=5,cl1=5,cl2=0;
            arr2[0]=999;
            CursCtl c=make_curs(s);
            c._bindArray(":arr1",arr1[0],0,ml1,&cl1);
            c._bindArray(":arr2",arr2[0],0,ml2,&cl2);
            if(i==0){
                c._bindArray(":arr1",arr1[0],0,ml1,&cl1);
                c._bindArray(":arr2",arr2[0],0,ml2,&cl2);
            }else{
                c.bindFull(":arr1", arr1, sizeof(arr1[0]), 0, 0, SQLT_INT, ml1,&cl1 );
                c.bindFull(":arr2", arr2, sizeof(arr2[0]), 0, 0, SQLT_INT, ml2,&cl2 );
            }
            c.exec();

            ck_assert_int_eq(cl1, cl2);
            fail_unless(memcmp(arr1,arr2,sizeof(arr1))==0);
        }catch (ociexception const &e ){
            fail_unless(0,e.what());
        }
    }

}
END_TEST

START_TEST(test_bindArray2)
{
    for(int i=0;i<2;++i){
        try{
            char const *s=
                "declare\n"
                "type intl is table of integer index by binary_integer;\n"
                "type charl is table of varchar2(4) index by binary_integer;\n"
                "int_var1 intl;\n"
                "int_var2 intl;\n"
                "char_var1 charl;\n"
                "char_var2 charl;\n"
                "ind binary_integer;\n"
                "begin\n"
                "for ind in 1  .. 5 loop\n"
                "int_var2(ind):=ind;\n"
                "end loop;\n"
                "int_var2(3):=null;\n"
                ":arr2:=int_var2;\n"
                "end;\n";

            ub4 ml2=5,cl2=0;
            sb2 ind[5]={};
            int arr2[5]={};
            CursCtl c=make_curs(s);
            if(i==0){
                c._bindArray(":arr2",arr2[0],ind,ml2,&cl2);
            }else{
                c.bindFull(":arr2", arr2, sizeof(arr2[0]), ind, 0, SQLT_INT, ml2,&cl2 );
            }
            c.exec();
            fail_unless ( cl2==5);
            fail_unless ( arr2[0]==1 && ind[0]==0);
            fail_unless ( arr2[4]==5 && ind[0]==0);
            fail_unless ( ind[2]==-1);
        }catch (ociexception const &e ){
            fail_unless(0,e.what());
        }
    }

}
END_TEST
START_TEST(test_bindArray3)
{
    for(int i=0;i<2;++i){
        try{
            char const *s=
                "declare\n"
                "type intl is table of integer index by binary_integer;\n"
                "type charl is table of varchar2(4) index by binary_integer;\n"
                "int_var1 intl;\n"
                "int_var2 intl;\n"
                "char_var1 charl;\n"
                "char_var2 charl;\n"
                "ind binary_integer;\n"
                "begin\n"
                "for ind in 1  .. 5 loop\n"
                "int_var2(ind):=ind;\n"
                "end loop;\n"
                "int_var2(3):=null;\n"
                ":arr2:=int_var2;\n"
                "end;\n";

            ub4 ml2=5,cl2=0;
            sb2 ind[5]={};
            int arr2[5]={};
            CursCtl c=make_curs(s);
            if(i==0){
                c._bindArray(":arr2",arr2[0],0,ml2,&cl2);
            }else{
                c.bindFull(":arr2", arr2, sizeof(arr2[0]), 0, 0, SQLT_INT, ml2,&cl2 );
            }
            c.exec();
            fail_unless ( cl2==5);
            fail_unless ( arr2[0]==1 && ind[0]==0);
            fail_unless ( arr2[4]==5 && ind[0]==0);
            fail_unless ( ind[2]==-1);
            fail_unless(0);
        }catch (ociexception const &e ){
        }
    }


}
END_TEST
START_TEST(test_bindArray4)
{
        try{
            char const *s=
                "declare\n"
                "type intl is table of integer index by binary_integer;\n"
                "type charl is table of varchar2(2) index by binary_integer;\n"
                "int_var1 intl;\n"
                "int_var2 intl;\n"
                "char_var1 charl;\n"
                "char_var2 charl;\n"
                "ind binary_integer;\n"
                "begin\n"
                "char_var1:=:arr1;\n"
                "char_var2:=char_var1;\n"
                ":arr2:=char_var2;\n"

                "end;\n";

            char arr1[3][2]={};
            memcpy(arr1,"A1A2A3",sizeof(arr1));
            char arr2[3][2]={};
            memcpy(arr2,"XXXXXX",sizeof(arr1));
            ub4 ml1=3,ml2=3,cl1=3,cl2=0;
            CursCtl c=make_curs(s);
            c._bindArray(":arr1",arr1[0],0,ml1,&cl1);
            c._bindArray(":arr2",arr2[0],0,ml2,&cl2);
            c.bindFull(":arr1", arr1, sizeof(arr1[0]), 0, 0, SQLT_LNG, ml1,&cl1 );
            c.bindFull(":arr2", arr2, sizeof(arr2[0]), 0, 0, SQLT_LNG, ml2,&cl2 );
            c.exec();

            fail_unless ( cl1==cl2);
            fail_unless(memcmp(arr1,arr2,sizeof(arr1))==0);
        }catch (ociexception const &e ){
            fail_unless(0,e.what());
        }

}
END_TEST

enum TestEnum { te_1 = 1, te_2 = 2 };

START_TEST(test_row)
{
    // check def for several rows
    Row<int, std::string, double, TestEnum> row;
    CursCtl cr(make_curs("select * from ("
                " select 1, 'lol', 3.14, 2 from dual"
                " union"
                " select 2, 'ka', 4.13, 4 from dual"
                ") order by 1"));
    cr.defRow(row);
    cr.exec();
    int i = 0;
    while (!cr.fen()) {
        if (i == 0) {
            fail_unless(row.get<0>() == 1);
            fail_unless(row.get<1>() == "lol");
            fail_unless(HelpCpp::rough_eq(row.get<2>(), 3.14));
            fail_unless(row.get<3>() == TestEnum(2));
        } else if (i == 1) {
            fail_unless(row.get<0>() == 2);
            fail_unless(row.get<1>() == "ka");
            fail_unless(HelpCpp::rough_eq(row.get<2>(), 4.13));
            // конструктор enum не проверяет значение
            // поэтому имеем такой эффект
            fail_unless(row.get<3>() == TestEnum(4));
        }
        ++i;
    }
} END_TEST

START_TEST(test_row_null)
{
    Row< boost::optional<int> > row;
    CursCtl cr(make_curs("select v from ("
                " select 1 as n, NULL as v from dual"
                " union"
                " select 2 as n, 3 as v from dual"
                " union"
                " select 3 as n, NULL as v from dual) order by n"));
    cr.defRow(row);
    cr.exec();
    int i = 0;
    while (!cr.fen()) {
        if (i == 0 || i == 2) {
            fail_unless(static_cast<bool>(row.get<0>()) == false);
        } else if (i == 1) {
            fail_unless(static_cast<bool>(row.get<0>()) == true);
            fail_unless(row.get<0>().get() == 3);
        }
        ++i;
    }
} END_TEST

START_TEST(test_no_throw_nocache)
{
 {
CursCtl cc=make_curs("select 1 from dual");
 try {
 char s[10];
 CursCtl c=make_curs_no_cache("select null from dual");
 c.noThrowError(1405);
 c.def(s);
 c.exfet();
 fail_unless(c.err()==1405,"must be error code for null");

 }catch(ociexception &e){
 fail_unless(0,e.what());
 }
 }
tst();
 {
 try {
 char s[10];
 CursCtl c=make_curs_no_cache("select 1/0 from dual");
 c.noThrowError(1476);
 c.def(s);
 c.exfet();
 fail_unless(c.err()==1476,"must be error code for null");

 }catch(ociexception &e){
 fail_unless(0,e.what());
 }
 }
}
END_TEST
/*
START_TEST(testCopyTable)
{
    make_curs("DROP TABLE RSD_TEST1").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    make_curs("CREATE TABLE RSD_TEST1 AS SELECT * FROM RSD WHERE ROWNUM<1").exec();
    make_curs("ALTER TABLE RSD_TEST1 ADD (SNAPSHOT_DATE DATE, SERIAL_NUMBER NUMBER)").exec();
    OciSession ro_sess(STDLOG, get_connect_string());


    ro_sess.createCursor(STDLOG, "set transaction read only").exec();
    std::list<CopyTable::FdescMore> l;

    boost::posix_time::ptime snapshot_time =boost::posix_time::second_clock::local_time();
    unsigned long long serial_number=123456ULL*78910ULL;
    l.push_back(CopyTable::FdescMore("SNAPSHOT_DATE", snapshot_time ));
    l.push_back(CopyTable::FdescMore("SERIAL_NUMBER", serial_number));

    CopyTable c(ro_sess,"RSD","RSD_TEST1", l );
    c.exec(TRACE1);
    commitInTestMode();

    int count=0;
    std::string templ ("select count (*) from (%1% from %2% minus  %1% from %3%) ");
    std::string sel_list=c.buildSelectList();
    make_curs( (boost::format(templ) % sel_list % "RSD" % "RSD_TEST1").str()  ).def(count).exfet();
    LogTrace(TRACE1) << "count=" << count;
    fail_unless(count==0);
    make_curs( (boost::format(templ) % sel_list % "RSD_TEST1" % "RSD").str()  ).def(count).exfet();
    fail_unless(count==0);
    make_curs("select count(*) from rsd_test1 where snapshot_date!=:sd or SERIAL_NUMBER!=:sn").def(count).bind(":sd",snapshot_time)
        .bind(":sn",serial_number).exfet();

    fail_unless(count==0);


}END_TEST
*/
START_TEST(def_str_to_number)
{
    std::string one, rid;
    OciCpp::CursCtl c("select 1000000000, rowid from dual",STDLOG);
    c.setDebug(true).def(one).def(rid).EXfet();
}
END_TEST

START_TEST(test_bind_int_after_long_int)
{
  make_curs("DROP TABLE TEST_BIND_INT").noThrowError(CERR_TABLE_NOT_EXISTS).exec();
  make_curs("CREATE TABLE TEST_BIND_INT (FLD1 NUMBER)").exec();
  make_curs("INSERT INTO TEST_BIND_INT(FLD1) VALUES (999)").exec();
  std::string query="SELECT COUNT(*) FROM TEST_BIND_INT WHERE FLD1=:FLD";

  {// Создаём кэшированный курсор, bind-им переменную long long int
    long long int fld=999;

    int tmp=0;
    CursCtl cr = make_curs(query);
    cr.bind(":FLD",fld);
    cr.def(tmp);
    cr.exec();
    cr.fen();
    fail_unless(tmp == 1, "tmp=%d expected 1", tmp);
  }

  {// переоткрываем закэшированный курсор, и bind-им переменную int
    int fld=999;

    int tmp=0;
    CursCtl cr = make_curs(query);
    cr.bind(":FLD",fld);
    cr.def(tmp);
    cr.exec();
    cr.fen();
    fail_unless(tmp == 1, "tmp=%d expected 1", tmp);
  }

  commit();

}
END_TEST;

START_TEST(test_unstb_vector_dates)
{
    boost::posix_time::ptime v1=Dates::ptime();
    boost::posix_time::ptime v2=boost::posix_time::second_clock::local_time();

    std::vector<boost::posix_time::ptime> vv = { v2, v1, v2 };

   boost::posix_time::ptime v_res;
   OciCpp::CursCtl cr = make_curs("select :ddd from dual");
   cr.unstb().defNull(v_res,Dates::ptime());

   boost::posix_time::ptime r;
   cr.bind(":ddd",r);

   for(size_t i=0, iz=vv.size(); i<iz; i++)
   {
     r = vv[i];
     cr.exec();
     cr.fen();

     fail_unless(vv[i] == v_res, "vv[%zu] (%s) != v_res (%s)", i, HelpCpp::string_cast(vv[i]).c_str(), HelpCpp::string_cast(v_res).c_str());
   }
}
END_TEST

START_TEST(test_max_bind_count)
{
    std::string sql = "SELECT ";
    for (size_t i = 1; i < 1000; ++i) {
        sql += ":bnd" + std::to_string(i) + ",";
    }
    sql += ":bnd0 FROM DUAL";
    OciCpp::CursCtl cc = make_curs(sql);
    for (size_t i = 0; i < 1000; ++i) {
        cc.bind(":bnd" + std::to_string(i), i);
    }
    fail_unless(cc.EXfet() == 0);
}
END_TEST
std::string underlyingTypeToString(const char underlyingType)
{
    return std::to_string(static_cast<int>(underlyingType));
}

template <typename UnderlyingType>
std::string underlyingTypeToString(const UnderlyingType underlyingType)
{
    return std::to_string(underlyingType);
}

template<typename EnumType>
std::string enumToString(const EnumType enumType)
{
    return underlyingTypeToString(static_cast<typename std::underlying_type<EnumType>::type>(enumType));
}

enum Unscoped { U_ZERO, U_ONE, U_TWO };
enum class ScopedChar: char { ZERO, ONE, TWO };
enum UnscopedInt : int { U_INT_ZERO, U_INT_ONE, U_INT_TWO };
enum class ScopedUnsignedLongInt : unsigned long int { ZERO, ONE, TWO };

#define CHECK_EQUAL_ENUM(lhs, rhs) \
    fail_unless( \
            lhs == rhs, \
            "%s(%s) != %s(%s)", \
            #lhs, \
            enumToString(lhs).c_str(), \
            #rhs, \
            enumToString(rhs).c_str())

START_TEST(test_enum_oci)
{
    Unscoped unscoped = U_ONE;
    ScopedChar scopedChar = ScopedChar::ZERO;
    UnscopedInt unscopedInt = U_INT_ZERO;
    ScopedUnsignedLongInt scopedUnsignedLongInt = ScopedUnsignedLongInt::TWO;

    OciCpp::CursCtl curs = make_curs("SELECT :first, :second, :third, :fourth FROM DUAL");
    curs
        .stb()
        .def(unscoped)
        .def(scopedChar)
        .def(unscopedInt)
        .def(scopedUnsignedLongInt)
        .bind(":first", U_ZERO)
        .bind(":second", ScopedChar::ONE)
        .bind(":third", U_INT_TWO)
        .bind(":fourth", ScopedUnsignedLongInt::ONE)
        .exfet();

    CHECK_EQUAL_ENUM(unscoped, U_ZERO);
    CHECK_EQUAL_ENUM(scopedChar, ScopedChar::ONE);
    CHECK_EQUAL_ENUM(unscopedInt, U_INT_TWO);
    CHECK_EQUAL_ENUM(scopedUnsignedLongInt, ScopedUnsignedLongInt::ONE);
}
END_TEST

START_TEST(test_enum_oci8)
{
    Unscoped unscopedDef = U_ONE;
    ScopedChar scopedCharDef = ScopedChar::ZERO;
    UnscopedInt unscopedIntDef = U_INT_ZERO;
    ScopedUnsignedLongInt scopedUnsignedLongIntDef = ScopedUnsignedLongInt::TWO;

    const Unscoped unscopedBind = U_ZERO;
    const ScopedChar scopedCharBind = ScopedChar::ONE;
    const UnscopedInt unscopedIntBind = U_INT_TWO;
    const ScopedUnsignedLongInt scopedUnsignedLongIntBind = ScopedUnsignedLongInt::ONE;

    OciCpp::Curs8Ctl curs(STDLOG, "SELECT :first, :second, :third, :fourth FROM DUAL");
    curs
        .def(unscopedDef)
        .def(scopedCharDef)
        .def(unscopedIntDef)
        .def(scopedUnsignedLongIntDef)
        .bind(":first", unscopedBind)
        .bind(":second", scopedCharBind)
        .bind(":third", unscopedIntBind)
        .bind(":fourth", scopedUnsignedLongIntBind)
        .EXfet();

    CHECK_EQUAL_ENUM(unscopedDef, unscopedBind);
    CHECK_EQUAL_ENUM(scopedCharDef, scopedCharBind);
    CHECK_EQUAL_ENUM(unscopedIntDef, unscopedIntBind);
    CHECK_EQUAL_ENUM(scopedUnsignedLongIntDef, scopedUnsignedLongIntBind);
}
END_TEST

#undef CHECK_EQUAL_ENUM

#define SUITENAME "SqlUtil"
TCASEREGISTER(setup,teardown)
{
    ADD_TEST(cursor_cache);
    ADD_TEST(cursor_cache_distruct1);
    ADD_TEST(cursor_cache_distruct2);
    ADD_TEST(cursor_cache_distruct3);
    ADD_TEST(test_Cstring_bindout);
    //ADD_TEST(test_null_bindout);
    //ADD_TEST(test_def_rcode);
    //ADD_TEST(test_stable_bind);
    ADD_TEST(test_Cstring_bindout_tab);
    ADD_TEST(test_binddefFlt);
    ADD_TEST(test_bindFull);
    ADD_TEST(test_defFull);
    ADD_TEST(test_cursorNoCache);
    ADD_TEST(test_Cstring_bindout_null_detect);
    ADD_TEST(test_Cstring_bindout_null);
    ADD_TEST(test_Cstring_bindout_auto_null);
    ADD_TEST(test_int_bindout_null);
    ADD_TEST(test_int_bindout_null2);
    ADD_TEST(test_int_bindout);
    ADD_TEST(test_uint_bindout);
    ADD_TEST(test_Cstring_bind);
    ADD_TEST(test_Cstring_bind_ptr);
    ADD_TEST(test_Cstring_bind_const);
    ADD_TEST(test_string_bind);
    ADD_TEST(test_string_bind_second_exec);
    ADD_TEST(test_double_cursors);
    ADD_TEST(test_string_bind_complex);
    ADD_TEST(test_Cstring);
    ADD_TEST(test_string_select1);
    ADD_TEST(test_string_select2);
    ADD_TEST(test_string_select3);
    ADD_TEST(test_string_select4);
    ADD_TEST(test_string_select5);
    ADD_TEST(test_string_select6);
    ADD_TEST(test_Cstring_tab);
    ADD_TEST(test_Cstring_null);
    ADD_TEST(test_Cstring_null2);
    ADD_TEST(OciSelectorConv);
    ADD_TEST(test_Cstring_null3);
    ADD_TEST(test_OciVcs);
    ADD_TEST(OciSelectorConv3);
    ADD_TEST(test_OciVcs_null);
    ADD_TEST(test_int);
    ADD_TEST(test_long_long);
    ADD_TEST(test_int_null);
    ADD_TEST(test_select_struct);
    ADD_TEST(test_select_struct_null);
    ADD_TEST(test_select_struct_auto_null);
    ADD_TEST(test_select_struct_auto_null_with_def);
    ADD_TEST(test_int_null2);
    ADD_TEST(test_int_null3);
    ADD_TEST(test_combine);
    ADD_TEST(test_combine2);
    ADD_TEST(test_no_throw);
    ADD_TEST(test_throw_all);
    ADD_TEST(test_rowcount);
    ADD_TEST(test_rowcount_struct);
    ADD_TEST(test_vector_example1);
    ADD_TEST(test_vector_example2);
    ADD_TEST(test_vector_example3);
    ADD_TEST(temporary_bind_vars);
    ADD_TEST(test_ref_bind);
    ADD_TEST(test_sqlstr_limit);
    ADD_TEST(test_bad_error);
    ADD_TEST(test_ignore_select_list);
    ADD_TEST(test_oci_rowid);
    ADD_TEST(test_char_buffer);
    ADD_TEST(test_bind_null_ptr);
    ADD_TEST(test_empty_str_is_null);
    ADD_TEST(test_Obndri);
    ADD_TEST(test_isgooderr);
    ADD_TEST(test_bind_bad_var);
    ADD_TEST(test_cache_no_cache);
    ADD_TEST(test_check_string_plname);
    ADD_TEST(test_resize);
    ADD_TEST(test_def_very_large_number);
    ADD_TEST(test_commit_rollback);
    ADD_TEST(test_secondary_initdb);
    ADD_TEST(test_Cstring_bindout2);
    ADD_TEST(test_bind_dups);
    ADD_TEST(test_datetime_bindout);
    ADD_TEST(test_datetime_def);
    ADD_TEST(test_datetime_bind);
    ADD_TEST(test_boostdt_bind2);
    ADD_TEST(test_boostdt_bind);
    ADD_TEST(test_boostdt_not_date);
    ADD_TEST(test_boostdt_default_value);
    ADD_TEST(test_boostdt_neg_pos_inf);
    ADD_TEST(test_boostdt_bindOut)
    ADD_TEST(test_date_struct_fetch);
    ADD_TEST(test_oracle_datetime);
    ADD_TEST(test_std_array);
    ADD_TEST(test_stable_bindout_correct);
    ADD_TEST(test_const_string_bind_stable_correct);
    ADD_TEST(test_char_pointer_bind_stable_correct);
    ADD_TEST(test_indicator_change);
    ADD_TEST(test_odescr);
    //SET_TIMEOUT(300);
    //ADD_TEST(test_bind_date_speed);
    ADD_TEST(test_def_null_changes);
    ADD_TEST(test_raw8_read);
    ADD_TEST(test_raw8_read_oci);
    ADD_TEST(test_max_open_cursors);
    ADD_TEST(test_bindArray1);
    ADD_TEST(test_bindArray2);
    ADD_TEST(test_bindArray3);
    ADD_TEST(test_bindArray4);
    ADD_TEST(test_row);
    ADD_TEST(test_row_null);
    ADD_TEST(test_no_throw_nocache);
    ADD_TEST(test_unstb_vector_dates);
    ADD_TEST(test_enum_oci)
    ADD_TEST(test_enum_oci8)
    ADD_TEST(test_max_bind_count);
    ADD_TEST(def_str_to_number)
    ADD_TEST(test_bind_int_after_long_int)
}TCASEFINISH


TCASEREGISTER(setup, 0) // do not need rollback because of DDL inside test
{
    ADD_TEST(test_check_row_count);
    ADD_TEST(test_bind_out_null);
    ADD_TEST(test_cache_sql_error);
    ADD_TEST(test_dump_table);
//    ADD_TEST(testCopyTable); disables as table RSD may not exist
}TCASEFINISH

TCASEREGISTER(setup, 0)
{
    ADD_TEST(test_split_connect_string);
    ADD_TEST(test_several_session);
    ADD_TEST(test_switch_session_mode);
    ADD_TEST(test_several_dummy_session);
}TCASEFINISH

TCASEREGISTER(setup, 0)
{
  SET_TIMEOUT( 300 );
  ADD_TEST(test_session_error_mem_leak);
    ADD_TEST(test_session_destroy_mem_leak);
}TCASEFINISH

} // namespace
#endif /* XP_TESTING */
#endif /*ENABLE_PG_TESTS*/
