#pragma once

#include <string>
#include <iosfwd>
#include <stdint.h>

#include "oci_selector.h"
#include "string_cast.h"
#include "ociexception.h"
#include "oracle_connection_param.h"
#include "ocilocal.h"
#include "oci_types.h"
#include "oci_default_ptr_t.h"
#include "simplelog.h"
#include "optional.h"
#include "oci_selector_char.h"
#include "commit_rollback.h"

#include <vector>
#define TRACESYS 0,"SYSTEM",__FILE__,__LINE__
#define OCI8_CURSCTL_SUPPORT

/**
  * @namespace OciCpp
  * @brief Пространство имен для C++/Oci */

namespace OciCpp
{
class OciSession;
class BaseRow;

/**
  * сообщить в лог об ошибке Oracle
  * @param x курсор
  * @param session сессия
  * @param nick NICKNAME
  * @param file __FILE__
  * @param line __LINE__ */
void error(cda_text * x, OciSession const &session,
        const char* nick, const char*  file, int line);

/**
  * создать строку с сообщением об ошибке Oracle
  * @param x курсор
  * @return строку с ошибкой */
std::string error(cda_text *x);
std::string error2(cda_text *x);

/// @fn newCursor Создать новый курсор в кеше
/**
  * Создать заново или найти новый курсор для данного sql запроса
  * @param s sql запрос
  * @param sess сессия
  * @return указатель на курсор */
cda_text * newCursor(OciSession &sess,const char *s);
cda_text * newCursor(OciSession &sess, std::string s);

void removeFromCacheC(cda_text *curs);
inline void closeCursorC(cda_text *curs)
{
    removeFromCacheC(curs);
}


// do not use cursor cache - use with dynamic creation of SQL string
cda_text * newCursorNoCache(const  char *s, bool dup = false);
inline cda_text * newCursorNoCache(const std::string &s)
{
    return newCursorNoCache(s.c_str(), true);
}

template<typename T>
struct OciNumericTypeTraits {
    using T::unknown_oci_type;
};

#define OCICPP_OCI_NUMERIC_TYPE_TRAITS(_type_, _otype_) \
template<> struct OciNumericTypeTraits<_type_>  {  enum { otype = _otype_ };  }

OCICPP_OCI_NUMERIC_TYPE_TRAITS(char, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(unsigned char, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(bool, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(int, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(short, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(long, SQLT_INT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(unsigned int, SQLT_UIN);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(unsigned long, SQLT_UIN);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(unsigned short, SQLT_UIN);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(float, SQLT_FLT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(double, SQLT_FLT);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(unsigned long long, SQLT_UIN);
OCICPP_OCI_NUMERIC_TYPE_TRAITS(long long int, SQLT_INT); // oracle 9 don't work with int64

#define OciSelectorNum(_type_) \
template <> struct OciSelector<const _type_> \
{ \
    enum { canObind   = 1 }; \
    enum { canBindArray = 1 }; \
    static constexpr unsigned len = sizeof(_type_); \
    enum { type = OciNumericTypeTraits<_type_>::otype }; \
    static constexpr External::type data_type = External::pod; \
    static bool canBind(const _type_ &) noexcept { return true; } \
    static void* addr(const _type_ *a) noexcept { return const_cast<_type_*>(a); } \
    static char* to(const void* a, indicator& ind)\
    {\
        if (ind == iok)\
        {\
            char* memory = new char[sizeof(_type_)];\
            memcpy(memory, a, sizeof(_type_));\
            return memory;\
        }\
        return nullptr; \
    }\
    static constexpr int size(const void* ) noexcept { return sizeof(_type_); }\
    static void check(_type_ const *) noexcept {} \
}; \
template <> struct OciSelector<_type_> : public OciSelector<const _type_> \
{ \
    enum { canOdef    = 1 }; \
    enum { canBindout = 1 }; \
    static void from(char* , const char* , indicator) {}\
};

OciSelectorNum(char);
OciSelectorNum(unsigned char);
OciSelectorNum(bool);
OciSelectorNum(int);
OciSelectorNum(long);
OciSelectorNum(short);
OciSelectorNum(unsigned int);
OciSelectorNum(unsigned long);
OciSelectorNum(unsigned short);
OciSelectorNum(float);
OciSelectorNum(double);
OciSelectorNum(unsigned long long);
OciSelectorNum(long long int); // oracle 9 don't work with int64
//OciSelectorNum(int64_t);

template <unsigned L> struct OciSelector< const OciVcs<L> >
{
    enum { canObind   = 1 } ;
    static constexpr unsigned len=L+2;
    enum { type = SQLT_VCS };
    static constexpr External::type data_type = External::pod;
    static bool canBind(const OciVcs<L>&) { return true; }
    static char* to(const void* a, indicator& ind) 
    {
        char* memory = new char[len];
        if (ind == iok)
        {
            memcpy(memory, a, len);
        }
        return memory;
    }
    static int size(const void* /*addr*/) { return len; }
    static void * addr(const OciVcs<L> *a){ return const_cast<OciVcs<L>*>(a); };
    static void check(OciVcs<L> const *){}
};
template <unsigned L> struct OciSelector< OciVcs<L> > : public OciSelector< const OciVcs<L> >
{
    enum { canOdef    = 1 };
    enum { canBindout = 1 };
    static constexpr bool auto_null_value=true;
    static void from(char* /*out_ptr*/, const char* /*in_ptr*/, indicator /*ind*/) {}
};

template <> struct OciSelector<const std::string>
{
    enum { canObind = 1 };
    static const int len=-1;
    static const int type =SQLT_STR;
    static const External::type data_type = External::string;
    static bool canBind(const std::string&) {return true;}
    static void* addr(const std::string* a) {return const_cast<std::string*>(a);}
    static char* to(const void* a, indicator& ind);
    static int size(const void* addr);
    static void check(std::string const *){}
};

template <> struct OciSelector<std::string> : public OciSelector<const std::string>
{
    enum { canOdef  = 1 };
    enum { canBindout = 1 };
    static void from(char* out_ptr, const char* in_ptr, indicator ind);
    static const bool auto_null_value=true;
};

template <> struct OciSelector< void * > 
{
};

//-----------------------------------------------------------------------

std::string make_nick(const char* nick_, const char*  file_, int line_);

// общая структура данных для выборки и биндинга
template <typename T> struct sel_data
{
    T plh;                               // placeholder (напр.: номер или ":id"), он же colname или colnum
    unsigned plh_len;                    // длина placeholder'а (only T=char const *)
    void* value_addr;                    // адрес переменной
    int type_size;                       // размер типа данных
    int type;                            // тип данных (SQLT_INT, SQLT_FLT и т.д.)
    indicator *ind;
    unsigned short *curlen;
    buf_ptr_t data;
    ind_ptr_t indv;
    indicator indic = iok;
    std::vector<ub2> rcode;
    unsigned long maxlen = 0;
    External::type data_type;
    default_ptr_t default_value;
    bool bind_in;
    int external_skip_size;
    char* (*to)(const void*, indicator&) = nullptr;
    void (*from)(char*, const char*, indicator) = nullptr;
    int (*get_size)(const void*) = nullptr;
    ub4 maxarrlen;
    ub4* curarrlen;
    std::vector<sb2> inner_ind;

    sel_data(T plh_, int plh_len_,
             void* value_addr_,
             int type_size_, int type_ ,short * ind_,
             unsigned short *clen, External::type data_type_,
             default_ptr_t def_, bool b_in, int ex_sk_sz, ub4 maxarrlen_, ub4* curarrlen_)
               : plh(plh_), plh_len(plh_len_),
                 value_addr(value_addr_),
                 type_size(type_size_), type(type_), ind(ind_),
                 curlen(clen),
                 rcode(1),
                 data_type(data_type_),
                 default_value(std::move(def_)), bind_in(b_in), external_skip_size(ex_sk_sz),
                 maxarrlen(maxarrlen_),curarrlen(curarrlen_)
    {
    }

    sel_data& operator=(sel_data&&) = default;
    sel_data(sel_data&&) = default;
    ~sel_data() = default;

    void print() const;
    void dump(std::ostream& ss) const;
};

class CursCtl;
template <typename T> struct buf_struct

{
    CursCtl &c;
    buf_struct(CursCtl &c1) : c(c1) {}
    void operator() (sel_data<T> &buf);
};
template <typename T> struct fetch_end_struct
{
    CursCtl &c;
    unsigned rp;
    fetch_end_struct(CursCtl &c1, unsigned r) : c(c1), rp(r) {}
    void operator() (sel_data<T> &s);
};
/**
  * @class CursCtl
  * @brief manages sql queries
  */

using BindList_t = std::vector< sel_data<const char *> >;
using DefList_t = std::vector< sel_data<unsigned> >;

class OracleData;

class CursCtl
{
    template <typename U> friend struct fetch_end_struct;
    template <typename U> friend struct buf_struct;
    std::shared_ptr<OracleData> oracledata;
    unsigned i_ = 0;
    bool debug = false;
    bool stableBind_;
    const char* nick;
    const char* file;
    int line;
    int data_skipsize = 0;
    int indicator_skipsize = 0;
    unsigned fetch_array_len = 1;
    int rpc_local = 0;
    unsigned rows_now = 0;
    bool auto_null = false;
    int expected_rows = -1;
    DefList_t sel_bufs;
    BindList_t bind_bufs;
    std::vector<int> noThrowErrList;
    //bool has_bind_ = false;
    bool has_exec_ = false;
    int initialSessionMode_ = -1;

public:
    CursCtl& operator = (const CursCtl&) = delete;
    CursCtl(const CursCtl&) = delete;

    // Чтобы иметь возможность возвращать курсор из функций и инициализировать его временным объектом
    // в отсуствие конструктора копирования необходимо объявить перемещающий конструктор.
    // По факту же компилятор использует copy elision, конструктор не используется и реализацию можно не писать.
    // Перемещающий конструктор по умолчанию здесь не подойдет,поскольку курсор содержит внутри указатель на собственную память (см. sel_data.ind).
    CursCtl(CursCtl&&);

    void throwException(const std::string& s);
    void throwException();
    std::string ex_id();
    void error(const char* nick, const char* file , int line);
    std::string error_text();
    CursCtl& fetchLen(int l);
    CursCtl& autoNull();
    /**
     * when enabled exec throws when rowcount() != expected_rows
     * */
    CursCtl& checkRowCount(int expected_rows_)
    {
        expected_rows = expected_rows_;
        return *this;
    }
    CursCtl& makeBig(int size=50)
    {
        sel_bufs.reserve(size);
//        bind_bufs.reserve(size);
//        converts_use.reserve(size);
        return *this;
    }
    CursCtl& structSize(int data, int indicator_skip=0);
    void dump();
    int err() const;
    int rowcount_now() const { return rows_now; }
    /**
     * @return number of processed rows
    */
    int rowcount();
    /**
     * ociexception won't be thrown when Oracle error with code err occurs.
     * @param err error code (OciCppErrs)
    */
    CursCtl& noThrowError(int _err)
    {
        noThrowErrList.push_back(_err);
        return *this;
    }
    CursCtl& throwAll()
    {
        noThrowErrList.clear();
        return *this;
    }
    CursCtl& setDebug(bool how=true)
    {
        debug=how;
        return *this;
    }
    CursCtl& stb()
    {
        stableBind_=true;
        return *this;
    }
    CursCtl& unstb()
    {
        stableBind_=false;
        return *this;
    }
    bool stableBind() const { return stableBind_; }
    bool isDebug() const { return debug; }

    ~CursCtl();
    enum OCI8T { OCI8 };
    CursCtl(OCI8T, std::string sqls, const char* nick_="", const char* file_="", int line_=0);
    CursCtl(std::shared_ptr<OracleData>&& odata, const char* nick_="", const char* file_="", int line_=0);
    CursCtl(std::string sqls, const char* nick_="", const char* file_="", int line_=0);
private:
    void init();

    template <class T> static auto __get_convnull_e(long) { return default_ptr_t(T()); }
    template <class T> static auto __get_convnull_e(int) -> decltype(OciSelector<T>::convnull()) { return OciSelector<T>::convnull(); }
    template <class T> static auto __get_convnull(std::enable_if_t<OciSelector<T>::auto_null_value, int>) { return __get_convnull_e<T>(0); }
    template <class T> static auto __get_convnull(long) { return default_ptr_t::none(); }
    template <class T, class TDef> static auto __get_convvar_t(TDef&& , long) { return default_ptr_t::none(); }
    template <class T, class TDef> static auto __get_convvar_t(TDef&& defval, int) -> decltype(default_ptr_t(T(defval))) { return default_ptr_t(T(defval)); }
    template <class T, class TDef> static auto __get_convvar(TDef&& defval, long) { return __get_convvar_t<T>(std::forward<TDef>(defval), 0); }
    template <class T, class TDef> static auto __get_convvar(TDef&& defval, int)
        -> decltype(OciSelector<T>::conv(std::forward<TDef>(defval))) { return OciSelector<T>::conv(std::forward<TDef>(defval)); }

    template <class T, class TDef> static auto _get_def_buf(bool, TDef&& defval) { return __get_convvar<T>(std::forward<TDef>(defval), 0); }
    template <class T> static auto _get_def_buf(bool nil, std::nullptr_t) { return nil ? __get_convnull<T>(0) : default_ptr_t::none(); }

    template <class T, typename Plh, class T2> auto make_sel_buf(Plh plh, unsigned plh_len, T& t, short* ind, T2&& defval)
    {
        using TT = OciSelector<T>;
        default_ptr_t def_buf = ind ? default_ptr_t::none() : _get_def_buf<T>(auto_null, std::forward<T2>(defval));
        sel_data<Plh> buf(plh, plh_len, TT::addr(&t), TT::len, TT::type, ind, 0, TT::data_type,
                                        std::move(def_buf), false, sizeof(T), 0, 0);
        return buf;
    }

    template <typename T, typename T2> CursCtl& def_(T &t, short *ind, T2&& defval)
    {
        static_assert(OciSelector<T>::canOdef == 1, "canOdef is not defined for this type");
        if (isDebug())
            Logger::getTracer().ProgTrace(TRACESYS, "%d type=%d", i_, OciSelector<T>::type);

        auto buf = make_sel_buf(++i_, 0, t, ind, std::forward<T2>(defval));
        buf.from = &OciSelector<T>::from;
        sel_bufs.push_back(std::move(buf));
        return *this;
    }

public:
    CursCtl & defFull(void *ptr, int maxlen, short *ind, unsigned short *clen, int type);

    CursCtl& defRow(BaseRow&);

    template <typename T> CursCtl& idef(T& t, short *ind)
    {
        return def_(t, ind, nullptr);
    }
    template <typename T> CursCtl& def(T& t, short *ind=0)
    {
        return def_(t, ind, nullptr);
    }
    template <typename T,typename T2> CursCtl& defNull(T &t, T2&& f)
    {
        return def_(t, nullptr, std::forward<T2>(f));
    }
    template <typename T> CursCtl& bind_(const char *pl, const T& t, indicator *ind, ub4 maxarrlen, ub4* curarrlen )
    {
        static_assert(OciSelector<T>::canObind == 1, "canBind is not defined for this type");

        if (ind )
        {
            for(unsigned i=0;i<(maxarrlen?maxarrlen:1);++i){
                if(ind[i]!=inull) {
                    OciSelector<T>::check(&t+i);
                }
            }
        }
        if (isDebug())
        {
            Logger::getTracer().ProgTrace(TRACESYS, "%s", stableBind() ? "stable" : "unstable");
        }
        const char* ppl = __get_ph(pl); // ppl - потому что pl помрёт после запяточки, а sqltext остаётся с нами
        auto buf = make_sel_buf(ppl, strlen(pl), t, ind, nullptr);
        buf.default_value = default_ptr_t::none();
        buf.bind_in = true;
        buf.maxarrlen = maxarrlen;
        buf.curarrlen = curarrlen;
        buf.to = &OciSelector<T>::to;
        buf.get_size = &OciSelector<T>::size;
        if (isDebug())
        {
            Logger::getTracer().ProgTrace(TRACESYS, "%s", "In bind_");
            buf.print();
        }
        if (stableBind() && maxarrlen==0) //FIXME 
        {
            buf.data.reset(OciSelector<T>::to(buf.value_addr, buf.ind ? *buf.ind : buf.indic));
        }
        // Если такой bind уже есть, то затрём его более потом в exec_start()
        bind_bufs.emplace_back(std::move(buf));
        if (has_exec_)
        {
        //    has_bind_ = true;
        }
        return *this;
    }

    template <typename T>
    CursCtl& smart_bind(const char *pl, const T& t, short *ind, ub4 maxarrlen, ub4* curarrlen)
    {
        if (!pl)
        {
            throw ociexception(ex_id() + std::string("failed find(NULL, ...)"));
        }
        // check for pointer char* != NULL
        // can do this only at RUNTIME
        if (!OciSelector<T>::canBind(t))
        {
            std::string errText = ":failed bind(";
            errText += pl;
            errText += ", NULL)";
            throw ociexception(ex_id() + errText);
        }
        return bind_(pl, t, ind, maxarrlen,curarrlen);
    }
    template <typename T> CursCtl& bind(const std::string& pl, const T& t, short *ind=0)
    {
        return smart_bind(pl.c_str(), t, ind,0,0);
    }
    template <typename T> CursCtl& bind(const char *pl, const T& t, short *ind=0)
    {
        return smart_bind(pl, t, ind,0,0);
    }
    template <typename T> CursCtl& _bindArray(const char *pl, const T& t, short *ind, ub4 maxarrlen, ub4* curarrlen)
    {
        (void)(OciSelector<T>::canBindArray); 
        return smart_bind(pl, t, ind,maxarrlen,curarrlen);
    }

private:
    const char* __get_ph(const char* pl);
    template <typename T, typename T2> CursCtl& bindOut_(const char *pl, T& t,short *ind, T2&& defval)
    {
        static_assert(OciSelector<T>::canBindout == 1, "canBindout is not defined for this type");
        if(OciSelector<T>::type==SQLT_STR && OciSelector<T>::data_type==External::pod){
            if(memchr(OciSelector<T>::addr(&t),'\0',OciSelector<T>::len)==NULL){
                *((char*)OciSelector<T>::addr(&t))='\0';
            }
        }
        const char* ppl = __get_ph(pl);
        auto buf = make_sel_buf(ppl, strlen(pl), t, ind, std::forward<T2>(defval));
        buf.from = &OciSelector<T>::from;
        buf.get_size = &OciSelector<T>::size;
        if (OciSelector<T>::data_type != External::pod && buf.maxarrlen==0) //FIXME
        {
            buf.data.reset(OciSelector<T>::to(buf.value_addr, buf.ind ? *buf.ind : buf.indic));
            if(not buf.data and OciSelector<T>::data_type == External::wrapper) // TODO authomatic use of OciSelector<T>::mem_buf()
                buf.data.reset(new char[buf.type_size]); // память-то нужна полюбас
        }
        // Если такой bind уже есть, то затрём его более потом в exec_start()
        bind_bufs.emplace_back(std::move(buf));
        return *this;
    }

public:
    CursCtl& bindFull(const std::string& pl, void *ptr, int maxlen, short *ind, unsigned short *clen, int type, ub4 maxarrlen=0, ub4* curarrlen=0);
    CursCtl& bindFull(const char *pl, void *ptr, int maxlen, short *ind, unsigned short *clen, int type, ub4 maxarrlen=0, ub4* curarrlen=0);

    template <typename T> CursCtl & bindOut(const std::string& pl, T &t, short *ind=0)
    {
        return bindOut(pl.c_str(), t, ind);
    }
    template <typename T> CursCtl & bindOut(const char *pl, T &t, short *ind=0)
    {
        return bindOut_(pl, t, ind, nullptr);
    }

    template <typename T,typename T2> CursCtl& bindOutNull(const std::string& pl, T& t, T2&& _def)
    {
        return bindOut_(pl.c_str(), t, nullptr, std::forward<T2>(_def));
    }
    template <typename T,typename T2> CursCtl& bindOutNull(const char *pl, T& t, T2&& _def)
    {
        return bindOut_(pl, t, nullptr, std::forward<T2>(_def));
    }
    template <typename T> CursCtl & ibindOut(const std::string& pl, T& t, short *ind)
    {
        return ibindOut(pl.c_str(), t, ind);
    }
    template <typename T> CursCtl & ibindOut(const char *p, T& t, short *ind)
    {
        return bindOut(p, t, ind);
    }
#ifdef exec
#error exec defined
#endif
    /**
      * executes query
      * @param p deprecated */
    void exec(const char *p=0);

private:
    void fetch_start(unsigned count);
    void exec_start();
    void exec_end();
    void fetch_end();
    void resize_vectors();
    void save_vector_sizes();

public:
    /**
     * Executes query and fetches one line by default.
     * @param count number of lines per one fetch
     * @param p deprecated
     * @return Oracle error code */
    int exfet(unsigned count=1, const char *p=0);
    /**
     * Executes query and fetches one line by default.
     * If there is more than count lines raises ociexception.
     * @param count number of lines per one fetch
     * @param p deprecated */
    int EXfet(unsigned count=1, const char *p=0);
    /**
     * Fetches one line by default.
     * @param count number of line per one fetch
     * @return Oracle error code */
    int fen(unsigned count=1);

    template <typename T> std::vector<T> fetchall(std::vector<T>& r)
    {
        std::vector<T> v;
        fetchLen(r.size());
        structSize(sizeof(T));
        exec();
        while(err() == 0)
        {
            fen(r.size());
            v.insert(v.end(), r.begin(), r.begin() + rowcount_now());
        }
        return v;
    }

    std::string queryString() const;
}; // class CursCtl

inline CursCtl make_curs_(std::string sql, const char* nick_, const char* file_, int line_)
{
    return CursCtl(std::move(sql), nick_, file_, line_);
}
inline CursCtl make_curs8_(std::string const &sql, const char* nick_, const char* file_, int line_)
{
    return CursCtl(CursCtl::OCI8, sql, nick_, file_, line_);
}



CursCtl make_curs_no_cache_(std::string const &sql, const char* nick_, const char* file_, int line_);
CursCtl make_curs_no_cache_(std::string const &sql, OciSession* sess, const char* nick_, const char* file_, int line_);
/// creates CusrCtl object from sql string
#define make_curs(x) OciCpp::make_curs_((x),STDLOG)
#define make_curs8(x) OciCpp::make_curs8_((x),STDLOG)
#define make_curs_no_cache(x) OciCpp::make_curs_no_cache_((x),STDLOG)

class CursorCache;
class Oracle8Data;
class OciSession 
{
    friend class Oracle8Data;
public:
    struct OciSessionNativeData
    {
        OciSessionNativeData();
        OCIEnv        *envhp; // OCI Environment
        OCISvcCtx     *svchp; // OCI Service context
        OCIError      *errhp; // OCI Error handler
        OCIServer     *srvhp; // OCI Server context
        OCISession    *usrhp; // OCI user context
    };

public:
    OciSession(const char* nick, const char* file, int line, const std::string& connStr);
    ~OciSession();
    void set7();
    void set8();
    void logoff();
    std::string getError() const;

    bool is_alive() const;
    void setDead();
    void commit();
    void commitInTestMode();
    void rollback();
    void rollbackInTestMode();
    void reconnect();

    std::shared_ptr<OracleData> cdaCursor(const char* sql, bool cacheit=true);
    std::shared_ptr<OracleData> cdaCursor(std::string sql, bool cacheit=true);
    void removeFromCache(char const * sql_text);

    CursCtl createCursor(const char* nick, const char* file, int line, const char* str);
    CursCtl createCursor(const char* nick, const char* file, int line, const std::string& str);

    Lda_Def* getLd() { return lda_; } // to use in old OCI7 code
    int mode() const { return mode_; } 
    const OciSessionNativeData& native() const { return native_; } //FIXME unused
    const std::string& getLogin() const { return connectionParams_.login; }
    std::string getConnectString() const;
    bool getDieOnError() const;
    void setDieOnError(bool value);
    void setClientInfo(const std::string &clientInfo);
#ifndef XP_TESTING
private:
#endif // XP_TESTING
    void clear();
    void init();
    OciSessionNativeData native_;
    Lda_Def lda_[1];
    int mode_; // OCI version, OCI8 by default, but mostly used is OCI7
    CursorCache* cache_;
    const char* nick_;
    const char* file_;
    int line_;
    oracle_connection_param connectionParams_;
private:
    OciSession(const OciSession&); // deny copying
    OciSession operator=(const OciSession&); // deny copying
    void free_handlers();
    bool is_alive_;
    bool dieOnError_ = false;
};


/**
 * Create default session for sirena. OCI7 yet.
 * */
std::shared_ptr<OciSession> createMainSession(const char* connStr, bool open_global_cursors = true);
std::shared_ptr<OciSession> createMainSession(STDLOG_SIGNATURE, const char* connStr, bool open_global_cursors = true);
void setMainSession(const std::shared_ptr<OciSession>& session);
OciSession& mainSession();
OciSession* pmainSession();
std::shared_ptr<OciSession> mainSessionPtr();

void putSession(const std::string& name, std::shared_ptr<OciCpp::OciSession> session);
OciSession& getSession(const std::string& name);

// Increase cache size (ora - 20 by default, we need more)  
bool oci8_set_cache_size(OCISvcCtx *svchp,OCIError *errhp);
void  pragma_autonomous_transaction(std::string &at1,std::string &at2);

} // namespace OciCpp

#define OciCpp_OciSelector_Enum(T) \
namespace OciCpp { \
OCICPP_OCI_NUMERIC_TYPE_TRAITS(T, SQLT_INT); \
OciSelectorNum(T) \
}
