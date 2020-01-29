#pragma once

#include "exceptions.h"
#include "tlg/CheckinBaseTypes.h"

#include <serverlib/helpcpp.h>
#include <serverlib/posthooks.h>
#include <serverlib/loki/LokiTypeInfo.h>

#include <typeinfo>
#include <boost/shared_ptr.hpp>


namespace BaseTables {

struct PortExceptionConf{
    static char const * const thing;
};
struct CityExceptionConf{
    static char const * const thing;
};
struct RouterExceptionConf {
    static char const * const thing;
};
struct CompanyExceptionConf {
    static char const * const thing;
};
struct CountryExceptionConf {
    static char const * const thing;
};


template <typename T> class noSuchThing : public EXCEPTIONS::Exception
{
    std::string ThingCode;
    std::string ThingName;
public:
    template <class idaT> noSuchThing(idaT ida, bool close=false) throw()
        : EXCEPTIONS::Exception(std::string("no such ") + T::thing +
                               ":ida=" + HelpCpp::string_cast(ida.get()) + (close?", close=1":"")),
            ThingCode(),
            ThingName(T::thing)
    {}

    noSuchThing(const std::string &s, bool close=false) throw()
        : EXCEPTIONS::Exception(std::string("no such ") +T::thing+
                               " :"+s + (close?", close=1":"")),
          ThingCode(s),
          ThingName(T::thing)
    {}
    noSuchThing()
        : EXCEPTIONS::Exception("Bad exception"),
          ThingName(T::thing)
    {}

    const std::string &thingCode() { return ThingCode; }
    const std::string &thingName() { return ThingName; }
    virtual ~noSuchThing()throw() {}
};

typedef noSuchThing<PortExceptionConf>    noSuchPort;
typedef noSuchThing<CityExceptionConf>    noSuchCity;
typedef noSuchThing<RouterExceptionConf>  noSuchRouter;
typedef noSuchThing<CompanyExceptionConf> noSuchCompany;
typedef noSuchThing<CountryExceptionConf> noSuchCountry;


template <typename T>    class IdaHolder ;
template <typename IdaT> class CacheData;
template <typename T>    class CommonData;

template <class T> struct  compCommonDataCode : public
    std::unary_function<const typename CacheData<T>::cache_elem &, bool >
{
    std::string code;
    Loki::TypeInfo t;
    compCommonDataCode(const std::string &pp, Loki::TypeInfo tt) : code(pp), t(tt) {}
    bool operator()(const typename CacheData<T>::cache_elem &a) {
        return (a.second->rcode()==code||a.second->lcode()==code) && a.first==t;
    }
};

template <class T> struct  compCommonDataIda : public
    std::unary_function<const typename CacheData<T>::cache_elem &, bool >
{
    typename T::IdaType p;
    Loki::TypeInfo t;
    compCommonDataIda(typename T::IdaType pp, Loki::TypeInfo tt) : p(pp), t(tt) {}
    bool operator() (const typename CacheData<T>::cache_elem &a) {
        return a.second->ida()==p && a.first==t;
    }
};



template <typename T> class CacheData
{
public:
    typedef std::pair<Loki::TypeInfo,std::shared_ptr<T> > cache_elem ;

protected:
    struct cache_elem_comp: public std::binary_function<const cache_elem &,
            const cache_elem & , bool >
    {
        bool operator()(const cache_elem &a, const cache_elem &b)
        {
            if(a.first==b.first )
                return a.second->ida()<b.second->ida();
            return (a.first<b.first);
        }
    };
    static std::set<cache_elem,cache_elem_comp > cache;
    static void cacheIt(std::type_info const &t,std::shared_ptr<T> p)
    {
        bool clear=cache.empty();
        cache.insert(
               std::make_pair(
                   Loki::TypeInfo(t),p
               )
       ) ;
       if(clear){
            registerHookAlways(&clearCache);
            registerHookAfter(&clearCache);
       }
    }
public:
    static T const * GetInstanceCode(const char * code, const char *sql)
    {
        typename T::IdaType ida;
        if(!CommonData<typename T::IdaType>::GetInstanceCode_help(sql, code, ida))
        {
            return 0;
        }
        return new T(ida);
//        typename std::set<cache_elem,cache_elem_comp>::iterator i=
//                std::find_if(cache.begin(),cache.end(),
//                        compCommonDataCode<T>(code,Loki::TypeInfo(typeid(T))));
//        if(i==cache.end())
//        {
//            typename T::IdaType ida;
//            if(!CommonData<typename T::IdaType>::GetInstanceCode_help(sql, code, ida))
//            {
//                return 0;
//            }
//            std::shared_ptr<T> ptr(new T(ida));
//            if(!ptr->initialized())
//                abort();
//            cacheIt(typeid(T),ptr);
//            return ptr.get();
//        }
//        return i->second.get();
    }

    static T const * GetInstance(typename T::IdaType ida)
    {
//        typename std::set<cache_elem,cache_elem_comp>::iterator i=
//                std::find_if(cache.begin(),cache.end(),
//                        compCommonDataIda<T>(ida,Loki::TypeInfo(typeid(T))));
//        if(i==cache.end()){
//            std::shared_ptr<T> ptr(new T(ida));
//            if(ptr->initialized())
//            {
//                cacheIt(typeid(T),ptr);
//                return ptr.get();
//            }
//            else
//                return 0;
//        }
//        T const *ret=dynamic_cast<T const *> (i->second.get());
//        if(ret==0) {
//            abort();
//        }
//        return ret;
        return new T(ida);
    }

    static void clearCache()
    {
        cache.clear();
    }
    static bool isCacheEmpty() // for unit_tests
    {
        return cache.empty();
    }
};


enum StrictLang_t{ lang_strict, lang_nostrict};


template <typename IdaT> class CommonData
{
public:
    typedef IdaT IdaType;
    static bool GetInstanceCode_help(const char *sql, const char *code, IdaType &ida);
private:
    static const int close_magic = 5213888;
    template <typename T> friend class IdaHolder;
protected:
    IdaT ida_;
    int closed_;
    std::string rcode_;
    std::string lcode_;
    std::string rname_;
    std::string lname_;
    CommonData():closed_(close_magic){}
public:
    IdaT ida() const
    {
        return ida_;
    }
    bool initialized() const
    {
        return closed_ != close_magic;
    }
    int closed() const
    {
        if(!initialized())
        {
            throw EXCEPTIONS::Exception(
                    std::string("closed must be intialized ")
                    +typeid(*this).name());
        }
        return closed_;
    }
    const std::string rcode(StrictLang_t strict=lang_nostrict) const
    {
        return code(RUSSIAN, strict);
    }
    const std::string lcode(StrictLang_t strict=lang_nostrict) const
    {
        return code(ENGLISH, strict);
    }
    const std::string rname(StrictLang_t strict=lang_nostrict) const
    {
        return name(RUSSIAN, strict);
    }
    const std::string lname(StrictLang_t strict=lang_nostrict) const
    {
        return name(ENGLISH, strict);
    }
    std::string code( Language lang, StrictLang_t strict=lang_nostrict ) const
    {
       std::string const &ret=(lang==ENGLISH ? lcode_ : rcode_);
       if(strict == lang_strict)
           return ret;
       if(ret.empty())
           return (lang==ENGLISH ? rcode_ : lcode_);
       return ret;
    }
    std::string code(StrictLang_t strict=lang_nostrict) const
    {
        return code(ENGLISH,strict);
    }
    std::string name( Language lang, StrictLang_t strict=lang_nostrict) const
    {
        std::string const &ret=(lang==ENGLISH ? lname_: rname_);
        if(strict == lang_strict)
            return ret;
        if(ret.empty())
            return (lang==ENGLISH ? rname_ : lname_);
        return ret;
    }
    virtual ~CommonData (){}
};


typedef enum {
    NoClose,
    YesClose,
}ReadClose_t;


typedef enum {
    NoThrowND,
    YesThrowND,
}ThrowNoData_t;



template <typename T> class IdaHolder {
    typename T::IdaType ida_;
    const T *cache;
    bool m_noClose;
    bool m_throwNoData;
    struct boolean {int i;};
public:
    /**
     * конструктор по умолчанию
     */
    IdaHolder()
            :ida_(),cache(0),m_noClose(true), m_throwNoData(true)
    {}

    /**
     * @brief по умолчанию читаем и записи с close = 1
     */
    explicit IdaHolder(typename T::IdaType p,
                       ThrowNoData_t thr = YesThrowND,
                       ReadClose_t close = YesClose)
            :ida_(), m_noClose(close == NoClose), m_throwNoData(thr == YesThrowND)
    {
        cache = CacheData<T>::GetInstance(p);
        if(cache)
        {
            if(cache->closed() && m_noClose)
            {
                if(m_throwNoData)
                    throw typename T::NoSuchThing(p, m_noClose);
            }
            else
                ida_ = typename T::IdaType(std::abs(cache->ida().get()));
        }
        else if(m_throwNoData)
            throw typename T::NoSuchThing(p);
    }
    /**
      * @brief по-умолчанию НЕ читаем записи с close = 1
      * @param p код элемента
      * @param thr - кидать ли исключение в случае отсутствия кода в базе
      * @param close
      */
    explicit IdaHolder(const std::string &p,
                       ThrowNoData_t thr = YesThrowND,
                       ReadClose_t close = NoClose)
             :ida_(),
              m_noClose(close == NoClose),
              m_throwNoData(thr == YesThrowND)
    {
        cache = T::GetInstance(p.c_str());
        if(cache)
        {
            if(cache->closed() && m_noClose)
            {
                if(m_throwNoData)
                    throw typename T::NoSuchThing(p, m_noClose);
            }
            else
                ida_ = typename T::IdaType(std::abs(cache->ida().get()));
        }
        else if(m_throwNoData)
            throw typename T::NoSuchThing(p);
    }
    /**
     * @brief по-умолчанию НЕ читаем записи с close = 1
     * @param p код элемента
     * @param thr - кидать ли исключение в случае отсутствия кода в базе
     * @param close
     */
    explicit IdaHolder(const char *p,
                       ThrowNoData_t thr = YesThrowND,
                       ReadClose_t close = NoClose)
    {
        *this = IdaHolder(std::string(p), thr, close);
    }

    const T *operator->() const
    {
        if(!(*this))
        {
            typename T::NoSuchThing thing;
            throw EXCEPTIONS::Exception("IdaHolder for " + thing.thingName() +  ": Use of uninitialized value");
        }
        return cache;
    }
    /**
    * проверка на инициированность
    */
    operator int boolean::*() const
    {
        if(ida_)
        {
            return &boolean::i;
        }
        else
        {
            return NULL;
        }
    }
};


class Country_impl;
class City_impl;
class Port_impl;
class Company_impl;
class Router_impl;

typedef IdaHolder<Country_impl> Country;
typedef IdaHolder<City_impl>    City;
typedef IdaHolder<Port_impl>    Port;
typedef IdaHolder<Company_impl> Company;
typedef IdaHolder<Router_impl>  Router;


class Country_impl: public CommonData <Ticketing::Country_t>
{
    std::string codeIso_;
public:
    typedef noSuchCountry NoSuchThing;
    typedef Ticketing::Country_t IdaType;
    explicit Country_impl(IdaType ida);
    static const Country_impl* GetInstance(const char* code);
    const std::string& codeIso() const { return codeIso_; }
};


class City_impl: public CommonData <Ticketing::City_t>
{
    std::string country_;
    std::string tzRegion_;
public:
    typedef noSuchCity NoSuchThing;
    typedef Ticketing::City_t IdaType;
    explicit City_impl(IdaType ida);
    static const City_impl* GetInstance(const char* code);
    Country country() const { return Country(country_); }
    const std::string& tzRegion() const { return tzRegion_; }
};


class Port_impl: public CommonData <Ticketing::Port_t>
{
    std::string city_;
    std::string codeIcao_;
    std::string lcodeIcao_;
public:
    typedef noSuchPort NoSuchThing;
    typedef Ticketing::Port_t IdaType;
    explicit Port_impl(IdaType ida);
    static const Port_impl* GetInstance(const char* code);
    City city() const { return City(city_); }
};


class Company_impl: public CommonData <Ticketing::Airline_t>
{
private:
    std::string accode_;
    std::string codeIcao_;
    std::string lcodeIcao_;
public:
    typedef noSuchCompany NoSuchThing;
    typedef Ticketing::Airline_t IdaType;
    explicit Company_impl(IdaType ida);
    static const Company_impl *GetInstance(const char *code);

    const std::string&    accode() const { return accode_; }
    const std::string&  codeIcao() const { return codeIcao_; }
    const std::string& lcodeIcao() const { return lcodeIcao_; }
};


class Router_impl: public CommonData <Ticketing::RouterId_t>
{
private:
    std::string canon_name_;
    std::string own_canon_name_;
    short resp_timeout_;
    std::string H2hSrcAddr_;
    std::string H2hDestAddr_;
    char H2hRemAddrNum_;
    bool IsH2h_;
    bool Translit_;
    bool Loopback_;
public:
    typedef noSuchRouter NoSuchThing;
    typedef Ticketing::RouterId_t IdaType;
    explicit Router_impl(IdaType ida);
    static const Router_impl *GetInstance(const char *code);

    const std::string &canonName() const { return canon_name_; }
    const std::string &ownCanonName() const { return own_canon_name_; }

    /**
     * @brief response timeout
     * @return
     */
    short resp_timeout() const;

    /**
     * @brief h2h from address std::string
    */
    const std::string &h2hSrcAddr() const { return H2hSrcAddr_; }

    /**
     * @brief h2h destination address std::string
     * @return
     */
    const std::string &h2hDestAddr() const { return H2hDestAddr_; }

    /**
     * @brief h2h layer5 remote addr number
     * @return
     */
    char remAddrNum() const { return H2hRemAddrNum_; }

    /**
     * @brief is H2h supported by this type of router
     * @return true if supported
     */
    bool isH2h() const { return IsH2h_; }

    /**
     * @brief translit
     * @return
     */
    bool translit() const { return Translit_; }

    /**
     * @brief loopback
     * @return
     */
    bool loopback() const { return Loopback_; }
};

}//namespace BaseTables
