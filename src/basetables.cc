#include "basetables.h"
#include "tlg/CheckinBaseTypesOci.h"

#include <serverlib/cursctl.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace BaseTables {

char const * const  RouterExceptionConf::thing="router";


#define __INIT_BT_TAB__(T) \
template <> std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>\
        CacheData<T>::cache = std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>();\
template<> bool CommonData<T::IdaType>::GetInstanceCode_help(\
        const char *sql,\
        const char *code,\
        T::IdaType &ida)\
{\
    T::IdaType IdaTmp(ida);\
    OciCpp::CursCtl c=make_curs(sql);\
    c.\
            bind(":code",code).\
            def(ida).\
            EXfet();\
\
    if(c.err()==NO_DATA_FOUND)\
        return false;\
    return true;\
}

#define __INIT_ROT_TAB__(T) \
template <> std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>\
        CacheData<T>::cache = std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>();\
template<> bool CommonData<T::IdaType>::GetInstanceCode_help(\
        const char *sql,\
        const char *code,\
        T::IdaType &ida)\
{\
    T::IdaType IdaTmp(ida);\
    OciCpp::CursCtl c=make_curs(sql);\
    static std::string ownCanonName = readStringFromTcl("OWN_CANON_NAME", "ASTRA");\
    c.\
            bind(":code",code).\
            bind(":own_canon_name",ownCanonName).\
            def(ida).\
            EXfet();\
\
    if(c.err()==NO_DATA_FOUND)\
        return false;\
    return true;\
}


__INIT_ROT_TAB__ (Router_impl);


Router_impl::Router_impl(IdaType ida)
{
    ida_= ida;
    short defval= 0;
    short H2H = 0;
    short translit = 0;
    short loopback = 0;
    OciCpp::CursCtl c = make_curs(
            "SELECT CANON_NAME, OWN_CANON_NAME, RESP_TIMEOUT, "
            "H2H, H2H_ADDR, OUR_H2H_ADDR, ROUTER_TRANSLIT, LOOPBACK "
            "FROM ROT "
            "WHERE ID=:ida");
    c.      bind(":ida", ida).
            def(canon_name_).
            def(own_canon_name_).
            defNull(resp_timeout_, defval).
            defNull(H2H, defval).
            defNull(H2hDestAddr_, "").
            defNull(H2hSrcAddr_, "").
            defNull(translit, defval).
            defNull(loopback, defval).
            EXfet();

    if(c.err() != NO_DATA_FOUND) {
        closed_ = 0;
    }

    IsH2h_    = (H2H != 0);
    Translit_ = (translit != 0);
    Loopback_ = (loopback != 0);

    LogTrace(TRACE3) << "ida_" << ida_;
    LogTrace(TRACE3) << "CanonName_" << canon_name_;
    LogTrace(TRACE3) << "IsH2h_" << IsH2h_;
    LogTrace(TRACE3) << "Translit_" << Translit_;
    LogTrace(TRACE3) << "Loopback_" << Loopback_;
}


const Router_impl * Router_impl::GetInstance(const char *code) {
    return CacheData<Router_impl>::GetInstanceCode (code,
            "SELECT ID FROM ROT "
            "WHERE CANON_NAME=:code and OWN_CANON_NAME=:own_canon_name");
}

short Router_impl::resp_timeout() const
{
    return resp_timeout_ < 5 ? 5 : resp_timeout_;
}

}//namespace BaseTables
