#if HAVE_CONFIG_H
#endif

//#include <math.h>
#include <cmath>

#include "posthooks.h"
#include "cache.h"

#define NICKNAME "ASM"
#include "slogger.h"

namespace Cache
{

namespace
{
  int stat_log_level = 12;
} // namespace

void set_stat_log_level(int level)
{
  stat_log_level = level;
}

int get_stat_log_level()
{
  return stat_log_level;
}

void stat_statistic_print(int Level, const char *nickname,const char *filename,
  int line,unsigned int hit_int,unsigned int fail_int,
  unsigned int total_hit_int,unsigned int total_fail_int,size_t max_size)
{
  ProgTrace(Level,nickname,filename,line,
    "           hit=%i(%i%%)/%i(%i%%) fail=%i(%i%%)/%i(%i%%) max size=%i",
    hit_int,
    hit_int+fail_int==0?0:(int)floor(.5+(100.*hit_int)/(hit_int+fail_int)),
    total_hit_int,
    total_hit_int+total_fail_int==0?0:
      (int)floor(.5+(100.*total_hit_int)/(total_hit_int+total_fail_int)),
    fail_int,
    hit_int+fail_int==0?0:(int)floor(.5+(100.*fail_int)/(hit_int+fail_int)),
    total_fail_int,
    total_hit_int+total_fail_int==0?0:
      (int)floor(.5+(100.*total_fail_int)/(total_hit_int+total_fail_int)),
    (int)max_size); 
}

void cache_statistic_print(int Level, const char *nickname,const char *filename,
  int line,const char *key_name,const char *data_name,
  size_t holder_size,size_t value_type_size,size_t element_type_size)
{
  LogTrace(Level,nickname,filename,line)<<key_name<<"/"<<
    data_name<<":  current size="<<holder_size<<
    " sizeof="<<value_type_size<<"+"<<element_type_size<<" bytes";
}

void message_print(const char *key_name,const char *data_name, std::string&& mess)
{
  LogTrace(TRACE5)<<key_name<<"/"<<data_name<<": "<<mess;
}

void message_print(const char *key_name,const char *data_name,const char *mess)
{
  LogTrace(TRACE5)<<key_name<<"/"<<data_name<<": "<<mess;
}


void lifetime_Request::register2clear(void (*p)(void))
{
  registerHookAlways(p); 
}

}// namespace Cache

#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"

namespace 
{
  
void startTests()
{
//  testInitDB();
//  Sirena::Environment::Env::init(Sirena::Environment::TextHandler);
//  xp_testing::randomLogin();
  tst();
}
 
void finishTests()
{
  tst();
//  testShutDBConnection();
//  tst();
}

namespace
{

int func_nocache(int i)
{
  static int a=0;
  
  a++;
  int res=a;
  if (i<0)
    res=-a;
  LogTrace(TRACE5)<<"func_nocache:"<<res;
  return res;
}

class TCacheKey
{
  private:
    int  i;
  public:
    TCacheKey(int a_i) : i(a_i) {}
    bool operator< (const TCacheKey &y) const
    {
      const TCacheKey &x=*this;
     
      if(x.i != y.i)  return x.i<y.i;
      return false;
    }
};

class TCacheElement
{
  private:
    int v;  
  public:
    const int &data() const { return v; }
    TCacheElement(const int &av): v(av){};
};

int func_default_hint(int i)
{
  int res=0;
  
  TCacheKey sample(i);
  Cache::TCacheHolder<TCacheKey,TCacheElement> base;
  Cache::TCacheHolder<TCacheKey,TCacheElement>::hint_type hint;
  const TCacheElement* found_data=base.find(sample,hint);

  if(found_data!=nullptr)
  {
    res=(*found_data).data();
  }  
  else 
  {
    res=func_nocache(i);
    base.cache_it_hint(hint,std::move(sample),TCacheElement(res));
  }
  LogTrace(TRACE5)<<"func_default_hint:"<<res;
  return res;
}


int func_default(int i)
{
  int res=0;
  
  TCacheKey sample(i);
  Cache::TCacheHolder<TCacheKey,TCacheElement> base;
  const TCacheElement* found_data=base.find(sample);

  if(found_data!=nullptr)
  {
    res=(*found_data).data();
  }  
  else 
  {
    res=func_nocache(i);
    base.cache_it(std::move(sample),TCacheElement(res));
  }
  LogTrace(TRACE5)<<"func_default:"<<res;
  return res;
}

int func_request(int i)
{
  int res=0;
  
  TCacheKey sample(i);
  Cache::TCacheHolder<TCacheKey,TCacheElement,Cache::lifetime_Request> base;
  const TCacheElement* found_data=base.find(sample);

  if(found_data) 
  {
    res=(*found_data).data();
  }  
  else 
  {
    res=func_nocache(i);
    base.cache_it(std::move(sample),TCacheElement(res));
  }
  LogTrace(TRACE5)<<"func_request:"<<res;
  return res;
}

int func_always(int i)
{
  int res=0;
  
  TCacheKey sample(i);
  Cache::TCacheHolder<TCacheKey,TCacheElement,Cache::lifetime_Always> base;
  const TCacheElement* found_data=base.find(sample);

  if(found_data!=nullptr) 
  {
    res=(*found_data).data();
  }  
  else 
  {
    res=func_nocache(i);
    base.cache_it(std::move(sample),TCacheElement(res));
  }
  LogTrace(TRACE5)<<"func_always:"<<res;
  return res;
}

} // namespace


START_TEST(check_nocache)
{
  fail_unless(func_nocache(1)==1,"Invalid value");
  fail_unless(func_nocache(-1)==-2,"Invalid value");
  fail_unless(func_nocache(-1)==-3,"Invalid value");
  fail_unless(func_nocache(1)==4,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_nocache(1)==5,"Invalid value");
  fail_unless(func_nocache(1)==6,"Invalid value");
  fail_unless(func_nocache(2)==7,"Invalid value");
}   
END_TEST

START_TEST(check_cache_lifetime_default)
{
  fail_unless(func_default(1)==1,"Invalid value");
  fail_unless(func_default(-1)==-2,"Invalid value");
  fail_unless(func_default(-1)==-2,"Invalid value");
  fail_unless(func_default(1)==1,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_default(1)==3,"Invalid value");
  fail_unless(func_default(1)==3,"Invalid value");
  fail_unless(func_default(2)==4,"Invalid value");
}   
END_TEST

START_TEST(check_cache_default_hint)
{
  fail_unless(func_default_hint(1)==1,"Invalid value");
  fail_unless(func_default_hint(-1)==-2,"Invalid value");
  fail_unless(func_default_hint(-1)==-2,"Invalid value");
  fail_unless(func_default_hint(1)==1,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_default_hint(1)==3,"Invalid value");
  fail_unless(func_default_hint(1)==3,"Invalid value");
  fail_unless(func_default_hint(2)==4,"Invalid value");
}   
END_TEST

START_TEST(check_cache_lifetime_request)
{
  fail_unless(func_request(1)==1,"Invalid value");
  fail_unless(func_request(-1)==-2,"Invalid value");
  fail_unless(func_request(-1)==-2,"Invalid value");
  fail_unless(func_request(1)==1,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_request(1)==3,"Invalid value");
  fail_unless(func_request(1)==3,"Invalid value");
  fail_unless(func_request(2)==4,"Invalid value");
}   
END_TEST

START_TEST(check_cache_lifetime_always)
{
  fail_unless(func_always(1)==1,"Invalid value");
  fail_unless(func_always(-1)==-2,"Invalid value");
  fail_unless(func_always(-1)==-2,"Invalid value");
  fail_unless(func_always(1)==1,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_always(1)==1,"Invalid value");
  fail_unless(func_always(1)==1,"Invalid value");
  fail_unless(func_always(2)==3,"Invalid value");
}   
END_TEST

START_TEST(check_cache_both)
{
  fail_unless(func_request(1)==1,"Invalid value");
  fail_unless(func_request(1)==1,"Invalid value");
  fail_unless(func_always(1)==2,"Invalid value");
  fail_unless(func_always(1)==2,"Invalid value");
  callPostHooksAlways();
  fail_unless(func_request(1)==3,"Invalid value");
  fail_unless(func_request(1)==3,"Invalid value");
  fail_unless(func_always(1)==2,"Invalid value");
  fail_unless(func_always(1)==2,"Invalid value");
}   
END_TEST

}

#define SUITENAME "Serverlib"
TCASEREGISTER(startTests, finishTests)
{
  SET_TIMEOUT(40);  //если работатем с чужой базой может надо и 600
  ADD_TEST(check_nocache);
  ADD_TEST(check_cache_lifetime_default);
  ADD_TEST(check_cache_default_hint);
  ADD_TEST(check_cache_lifetime_request);
  ADD_TEST(check_cache_lifetime_always);
  ADD_TEST(check_cache_both);
}
TCASEFINISH

#endif /*XP_TESTING*/
