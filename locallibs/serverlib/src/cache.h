#ifndef _CACHE_H_
#define _CACHE_H_

#include <map>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <string>
#include <typeinfo>
#include <string.h>
#include "loki/Singleton.h"

extern "C" int get_hdr();

namespace Cache
{

void set_stat_log_level(int level);
int get_stat_log_level();

void stat_statistic_print(int Level, const char *nickname,const char *filename,
  int line,unsigned int hit_int,unsigned int fail_int,
  unsigned int total_hit_int,unsigned int total_fail_int,size_t max_size);

void cache_statistic_print(int Level, const char *nickname,const char *filename,
  int line,const char *key_name,const char *data_name,
  size_t holder_size,size_t value_type_size,size_t element_type_size);

void message_print(const char *key_name,const char *data_name,const char *mess);
void message_print(const char *key_name,const char *data_name, std::string&& mess);

template <class TKey, class TData, class TLifeTime> class TCacheStatisticElement
{
  private:
    unsigned int hit_int;
    unsigned int fail_int;
    unsigned int total_hit_int;
    unsigned int total_fail_int;
    size_t max_size;
  public:
    void clear() {
      hit_int=0; fail_int=0; }
    void hit() { hit_int++; total_hit_int++; }
    void fail() { fail_int++; total_fail_int++; }
    void invalidate() { hit_int--; total_hit_int--; fail(); }
    void set_size(size_t s) { if (s>max_size) max_size=s; }
    TCacheStatisticElement(): hit_int(0),fail_int(0),total_hit_int(0),
      total_fail_int(0), max_size(0) {};
    void statistic(int Level, const char *nickname,const char *filename,
      int line) const {
      stat_statistic_print(Level,nickname,filename,line,
        hit_int,fail_int,total_hit_int,total_fail_int,max_size); }
};

template <class TKey, class TData, class TLifeTime> class TCacheStatistic
{
  private:
    typedef Loki::SingletonHolder<TCacheStatisticElement<TKey,TData,TLifeTime>,
      Loki::CreateUsingNew,Loki::DefaultLifetime,Loki::SingleThreaded> Stat;
  public:
    static void clear() { Stat::Instance().clear(); }
    void hit() { Stat::Instance().hit(); }
    void fail() { Stat::Instance().fail(); }
    void invalidate() { Stat::Instance().invalidate(); }
    void set_size(size_t ss) { Stat::Instance().set_size(ss); }
    static void statistic(int Level, const char *nickname,const char *filename,
      int line) {
      Stat::Instance().statistic(Level,nickname,filename,line); }
};

struct lifetime_Request
{
    static void register2clear(void (*p)(void));
};

struct lifetime_Always
{
    static void register2clear(void (*p)(void)) {}
};

namespace private_details {
template <class TKey, class TData, class TLifeTime=lifetime_Request, typename Tag=TKey> class TCacheHolder // never use directly, getCached() instead
{
  public:
    typedef std::map< TKey , TData > holder_type;
    typedef typename holder_type::value_type value_type;
    typedef typename holder_type::iterator hint_type;
    typedef typename holder_type::key_type key_type;
    typedef TData element_type;
  private:
    template <typename TTag, typename TType, typename TLife> struct TaggedData { std::tuple<TType,TLife> data; };
    typedef Loki::SingletonHolder< TaggedData<Tag,holder_type,TLifeTime>,
          Loki::CreateUsingNew,Loki::DefaultLifetime,
          Loki::SingleThreaded> holder_inst;
    typedef TCacheStatistic<Tag,TData,TLifeTime> stat_type;
    stat_type stat;
    bool log;
    inline static holder_type &holder_instance()
      { return std::get<0>(holder_inst::Instance().data); }
  public:
    TCacheHolder<TKey,TData,TLifeTime,Tag>() : TCacheHolder{true} {}
    TCacheHolder<TKey,TData,TLifeTime,Tag>(bool alog) : log(alog) {
        static std::mutex m;
        std::lock_guard<std::mutex> lk(m);
    }
    static void clear() {
      if(::get_hdr() != 2)
      {
        message_print(typeid(Tag).name(),typeid(TData).name(),
          "Cache CLEAR!!!");
        TCacheHolder<TKey,TData,TLifeTime,Tag>::statistic(
          get_stat_log_level(), "ASM", __FILE__, __LINE__);
      }
      holder_instance().clear();
      stat_type::clear(); }
    size_t size() const { return holder_instance().size(); }
    
    const element_type* find(const key_type &sample)
    {
      hint_type hint;
      return find(sample,hint);
    }
    
    const element_type* find(const key_type &sample,hint_type& hint)
    {
      auto& holder = holder_instance();
      
      hint=holder.lower_bound(sample);  
      if(hint == holder.end() || holder.key_comp()(sample, hint->first))
      {
        if(log)
          message_print(typeid(Tag).name(),typeid(TData).name(),"Cache FAIL!!!");
        stat.fail();
        return nullptr;
      }
      else
      {
        if( log )
          message_print(typeid(Tag).name(),typeid(TData).name(),"Cache HIT!!!");
        stat.hit();
        return &(hint->second);
      }
    }

    void invalidate(const key_type &key) {
      holder_instance().erase(key);
      stat.invalidate();
    }

    const element_type& cache_it(key_type&& key, element_type&& data)
    {
      auto &holder=holder_instance();
      if (holder.empty())
      {
        TLifeTime::register2clear(&clear);
      }

      auto iter=holder.lower_bound(key);  
      if(iter == holder.end() || holder.key_comp()(key, iter->first))
      {
        iter = holder.emplace_hint( iter, std::move(key), std::move(data) );
        stat.set_size(holder.size());
      }
      else
      {
        if( log )
          message_print(typeid(Tag).name(),typeid(TData).name(),"Cache INVALIDATE!!!");
        stat.invalidate();
        iter->second = std::move(data);
      }
        
      return iter->second;
    }
    const element_type& cache_it_hint(hint_type const& hint,
      key_type&& key, element_type&& data)
    {
      auto &holder=holder_instance();
      if (holder.empty())
      {
        TLifeTime::register2clear(&clear);
      }

      auto iter=hint;  
      if(iter == holder.end() || holder.key_comp()(key, iter->first))
      {
        iter = holder.emplace_hint( iter, std::move(key), std::move(data) );
        stat.set_size(holder.size());
      }
      else
      {
        if( log )
          message_print(typeid(Tag).name(),typeid(TData).name(),"Cache INVALIDATE!!!");
        stat.invalidate();
        iter->second = std::move(data);
      }
        
      return iter->second;
    }
    
    static void statistic(int Level, const char *nickname,const char *filename,
      int line) {
      holder_type &holder=holder_instance();
      cache_statistic_print(Level,nickname,filename,line,typeid(Tag).name(),
        typeid(TData).name(),holder.size(),sizeof(value_type),
        sizeof(element_type));
      stat_type::statistic(Level,nickname,filename,line); }
};
} // namespace private_details
using private_details::TCacheHolder;

//-----------------------------------------------------------------------

template <typename Lifetime, typename Tag, typename F, typename K, typename C, typename... A>
auto getCached_Atom(F&& get_from_db, K&& make_key, C&& check_plus, A&&... key_args)
    -> const decltype(get_from_db(key_args...))&
{
    auto key = make_key(key_args...);
    typedef decltype(get_from_db(key_args...)) Value;
    typedef decltype(key) Key;
    typedef typename private_details::TCacheHolder<Key, Value, Lifetime, Tag> cache_type;
    cache_type cache_holder;
    typename cache_type::hint_type hint;
    if(auto foundData = cache_holder.find(key,hint))
    {
        if(check_plus(*foundData))
            return *foundData;
    }
    return cache_holder.cache_it_hint( hint, std::move(key), get_from_db(std::forward<A>(key_args)...) );
}

//-----------------------------------------------------------------------

template <typename D> bool check_plus_always_true(const D& ) {  return true;  }


template <size_t n, typename... A> struct _check_for_pointer_;

template <size_t n> struct _check_for_pointer_<n> : std::true_type {};
template <size_t n, typename T, typename... A> struct _check_for_pointer_<n, T, A...> : std::true_type
{
    static_assert(not std::is_pointer<typename std::decay<T>::type>::value, "You must not to provide pointer arguments to getCached*()");
    typedef typename _check_for_pointer_<n+1, A...>::type ValidInvocation;
};

//-----------------------------------------------------------------------

template <typename Tag, typename F, typename... A> auto getCached_Always(F&& get_from_db, A&&... key_args) -> const decltype(get_from_db(key_args...))&
{
    typedef typename _check_for_pointer_<1, A...>::type ValidInvocation;
    return getCached_Atom<lifetime_Always,Tag>(std::forward<F>(get_from_db),
                                               std::make_tuple<A...>,
                                               check_plus_always_true<decltype(get_from_db(key_args...))>,
                                               std::forward<A>(key_args)...);
}

template <typename Tag, typename F, typename... A> auto getCached(F&& get_from_db, A&&... key_args) -> const decltype(get_from_db(key_args...))&
{
    typedef typename _check_for_pointer_<1, A...>::type ValidInvocation;
    return getCached_Atom<lifetime_Request,Tag>(std::forward<F>(get_from_db),
                                                std::make_tuple<A...>,
                                                check_plus_always_true<decltype(get_from_db(key_args...))>,
                                                std::forward<A>(key_args)...);
}

template <typename F, typename K, typename... A> auto getCached_WithKey(F&& get_from_db, K&& make_key, A&&... key_args) -> const decltype(get_from_db(key_args...))&
{
    typedef decltype(make_key(key_args...)) Tag;
    return getCached_Atom<lifetime_Request,Tag>(std::forward<F>(get_from_db),
                                                std::forward<K>(make_key),
                                                check_plus_always_true<decltype(get_from_db(key_args...))>,
                                                std::forward<A>(key_args)...);
}

template <typename Tag, typename F, typename C, typename... A> auto getCached_WithCheck(F&& get_from_db, C&& check_plus, A&&... key_args) -> const decltype(get_from_db(key_args...))&
{
    typedef typename _check_for_pointer_<1, A...>::type ValidInvocation;
    return getCached_Atom<lifetime_Request,Tag>(std::forward<F>(get_from_db),
                                                std::make_tuple<A...>,
                                                std::forward<C>(check_plus),
                                                std::forward<A>(key_args)...);
}

template <typename Tag, typename F, typename K, typename C, typename... A> auto getCached_WithKey_WithCheck(F&& get_from_db, K&& make_key, C&& check_plus, A&&... key_args) -> const decltype(get_from_db(key_args...))&
{
    return getCached_Atom<lifetime_Request,Tag>(std::forward<F>(get_from_db),
                                                std::forward<K>(make_key),
                                                std::forward<C>(check_plus),
                                                std::forward<A>(key_args)...);
}

}; // namespace Cache


#endif /* _CACHE_H_ */
