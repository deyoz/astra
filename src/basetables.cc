#include "basetables.h"
#include "PgOraConfig.h"

#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace BaseTables {

char const* const PortExceptionConf::thing   ="port";
char const* const CityExceptionConf::thing   ="city";
char const* const CompanyExceptionConf::thing="company";
char const* const CountryExceptionConf::thing="country";
char const* const RouterExceptionConf::thing ="router";


#define __INIT_BT_TAB__(T) \
template <> std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>\
        CacheData<T>::cache = std::set<CacheData<T>::cache_elem,CacheData<T>::cache_elem_comp>();\
template<> bool CommonData<T::IdaType>::GetInstanceCode_help(\
        const char *sql,\
        const char *code,\
        T::IdaType &ida)\
{\
    T::IdaType::BaseType IdaTmp = 0;\
    auto c = make_db_curs(sql, PgOra::getROSession("SP_PG_GROUP_BASETABLES"));\
    c.\
            bind(":code",code).\
            def(IdaTmp).\
            EXfet();\
\
    if(c.err()==DbCpp::ResultCode::NoDataFound)\
        return false;\
    ida = T::IdaType(IdaTmp);\
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
    T::IdaType::BaseType IdaTmp = 0;\
    auto c = make_db_curs(sql, PgOra::getROSession("ROT"));\
    static std::string ownCanonName = readStringFromTcl("OWN_CANON_NAME", "ASTRA");\
    c.\
            bind(":code",code).\
            bind(":own_canon_name",ownCanonName).\
            def(IdaTmp).\
            EXfet();\
\
    if(c.err()==DbCpp::ResultCode::NoDataFound)\
        return false;\
    ida = T::IdaType(IdaTmp);\
    return true;\
}

__INIT_BT_TAB__(City_impl);
__INIT_BT_TAB__(Port_impl);
__INIT_BT_TAB__(Company_impl);
__INIT_BT_TAB__(Country_impl);

__INIT_ROT_TAB__ (Router_impl);


Country_impl::Country_impl(IdaType ida)
{
    ida_ = ida;
    auto c = make_db_curs(
            "SELECT RTRIM(CODE), RTRIM(CODE_LAT), RTRIM(NAME), RTRIM(NAME_LAT), "
            "RTRIM(CODE_ISO), PR_DEL "
            "FROM COUNTRIES WHERE ID=:ida",
                PgOra::getROSession("COUNTRIES"));
    c.stb()
     .autoNull()
     .bind(":ida", ida.get())
     .def(rcode_)
     .defNull(lcode_, "")
     .def(rname_)
     .defNull(lname_, "")
     .defNull(codeIso_, "")
     .defNull(closed_, 0)
     .EXfet();
}

const Country_impl* Country_impl::GetInstance(const char* code)
{
    return CacheData<Country_impl>::GetInstanceCode (code,
            "SELECT ID FROM COUNTRIES "
            "WHERE PR_DEL=0 AND (CODE=:code OR CODE_LAT=:code)");
}

City_impl::City_impl(IdaType ida)
{
    ida_ = ida;
    auto c = make_db_curs(
            "SELECT RTRIM(CODE), RTRIM(CODE_LAT), RTRIM(NAME), RTRIM(NAME_LAT), "
            "TZ_REGION, COUNTRY, PR_DEL "
            "FROM CITIES WHERE ID=:ida",
                PgOra::getROSession("CITIES"));
    c.stb()
     .autoNull()
     .bind(":ida", ida.get())
     .def(rcode_)
     .defNull(lcode_, "")
     .def(rname_)
     .defNull(lname_, "")
     .defNull(tzRegion_, "")
     .def(country_)
     .defNull(closed_, 0)
     .EXfet();
}

const City_impl* City_impl::GetInstance(const char* code)
{
    return CacheData<City_impl>::GetInstanceCode (code,
            "SELECT ID FROM CITIES "
            "WHERE PR_DEL=0 AND (CODE=:code OR CODE_LAT=:code)");
}


Port_impl::Port_impl(IdaType ida)
{
    ida_ = ida;
    auto c = make_db_curs(
            "SELECT RTRIM(CODE), RTRIM(CODE_LAT), RTRIM(NAME), RTRIM(NAME_LAT), "
            "RTRIM(CODE_ICAO), RTRIM(CODE_ICAO_LAT), CITY, PR_DEL "
            "FROM AIRPS WHERE ID=:ida",
                PgOra::getROSession("AIRPS"));
    c.stb()
     .autoNull()
     .bind(":ida", ida.get())
     .def(rcode_)
     .defNull(lcode_, "")
     .def(rname_)
     .defNull(lname_, "")
     .defNull(codeIcao_, "")
     .defNull(lcodeIcao_, "")
     .def(city_)
     .defNull(closed_, 0)
     .EXfet();
}

const Port_impl* Port_impl::GetInstance(const char* code)
{
    return CacheData<Port_impl>::GetInstanceCode (code,
            "SELECT ID FROM AIRPS "
            "WHERE PR_DEL=0 AND (CODE=:code OR CODE_LAT=:code)");
}


Company_impl::Company_impl(IdaType ida)
{
    ida_ = ida;
    auto c = make_db_curs(
            "SELECT RTRIM(CODE), RTRIM(CODE_LAT), RTRIM(NAME), RTRIM(NAME_LAT), "
            "RTRIM(CODE_ICAO), RTRIM(CODE_ICAO_LAT), AIRCODE, PR_DEL "
            "FROM AIRLINES WHERE ID=:ida",
                PgOra::getROSession("AIRLINES"));
    c.stb()
     .autoNull()
     .bind(":ida", ida.get())
     .def(rcode_)
     .defNull(lcode_, "")
     .def(rname_)
     .defNull(lname_, "")
     .defNull(codeIcao_, "")
     .defNull(lcodeIcao_, "")
     .defNull(accode_, "")
     .defNull(closed_, 0)
     .EXfet();
}

const Company_impl* Company_impl::GetInstance(const char *code)
{
    return CacheData<Company_impl>::GetInstanceCode (code,
            "SELECT ID FROM AIRLINES "
            "WHERE PR_DEL=0 AND (CODE=:code OR CODE_LAT=:code)");
}


Router_impl::Router_impl(IdaType ida)
{
    ida_= ida;
    short defval= 0;
    short H2H = 0;
    short translit = 0;
    short loopback = 0;
    int remAddrNumTmp = 0;
    auto c = make_db_curs(
            "SELECT CANON_NAME, OWN_CANON_NAME, RESP_TIMEOUT, "
            "IP_ADDRESS, IP_PORT, "
            "H2H, H2H_ADDR, OUR_H2H_ADDR, H2H_REM_ADDR_NUM, ROUTER_TRANSLIT, LOOPBACK "
            "FROM ROT "
            "WHERE ID=:ida",
                PgOra::getROSession("ROT"));
    c.      stb().
            bind(":ida", ida.get()).
            def(canon_name_).
            def(own_canon_name_).
            defNull(resp_timeout_, (short)15).
            def(ipAddress_).
            def(ipPort_).
            defNull(H2H, defval).
            defNull(H2hDestAddr_, "").
            defNull(H2hSrcAddr_, "").
            def(remAddrNumTmp).
            defNull(translit, defval).
            defNull(loopback, defval).
            EXfet();

    if(c.err() != DbCpp::ResultCode::NoDataFound) {
        closed_ = 0;
    }

    IsH2h_    = (H2H != 0);
    Translit_ = (translit != 0);
    Loopback_ = (loopback != 0);
    H2hRemAddrNum_ = remAddrNumTmp + int('0');
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
