#if HAVE_CONFIG_H
#endif


#include "timing.h"
#include "monitor_ctl.h"
#include "exception.h"

#define NICKNAME "SYSTEM"
#include "test.h"

bool TimeAccounting::ReqData::CompCalls::operator()( const ReqData &a, const ReqData &b ) const
{
    auto tie = [](auto& r){ return std::tie(r.callcount, r.tv.tv_sec, r.tv.tv_sec, r.name); };
    return tie(a) > tie(b);
}

bool TimeAccounting::ReqData::CompTimes::operator()( const ReqData &a, const ReqData &b ) const
{
    auto tie = [](auto& r){ return std::tie(r.tv.tv_sec, r.tv.tv_sec, r.callcount, r.name); };
    return tie(a) > tie(b);
}

bool TimeAccounting::ReqData::CompName::operator()( const ReqData &a, const ReqData &b ) const
{
    return a.name<b.name;
}


using namespace TimeAccounting;
namespace {    
void addTimeStat2(const char *name,long ticks, bool cnt)
{
      static TimeAccount Counter1,Counter2;
      static int lastcount1,lastcount2;
      try {
          Counter1.add_call(name,ticks,cnt);
          Counter2.add_call(name,ticks,cnt);
          if(Counter1.callCount() && Counter1.callCount()%1000==0
                  && Counter1.callCount()!=lastcount1){
              const int count=20;
              lastcount1=Counter1.callCount();
              std::stringstream head;
              head <<"Total: "<<count << " most frequent calls" 
                  " out of " << Counter1.callCount() 
                  <<"\ntotal time "<< Counter1.totalTime() << " secs" ;
              ProgTrace(TRACE1, "%s", head.str().c_str());
              Counter1.report<ReqData::CompCalls>(count);
              head.str("");
              head << "Total: "<<count<< " most time consuming calls "
                  "out of " << Counter1.callCount()
                  <<"\ntotal time "<< Counter1.totalTime() << " secs" ;
              ProgTrace(TRACE1, "%s", head.str().c_str());
              Counter1.report<ReqData::CompTimes>(count);
          }
          
          if(Counter2.callCount() && Counter2.callCount()%100==0
                  && Counter1.callCount()!=lastcount1 ){
              const int count=5;
              lastcount2=Counter2.callCount();
              std::stringstream head;
              head <<"Average:" <<count << " most time consuming calls" 
                  " out of " << Counter1.callCount();
              ProgTrace(TRACE1, "%s", head.str().c_str());
              Counter1.report<ReqData::CompTimes>(count);
              head.str("");
              head <<"Momental:" <<count << " most time consuming calls" 
                  " out of " << Counter2.callCount();
              
              ProgTrace(TRACE1, "%s", head.str().c_str());
              Counter2.report<ReqData::CompTimes>(count);
              Counter2.reset(); 
          }
      }catch (const comtech::Exception &e) {
            ProgError(STDLOG,"Error in accounting  %s",e.what());
      }

}
}
void addTimeStat(const char *name,long ticks)
{
    addTimeStat2(name,ticks,true);
}
void addTimeStatNoCount(const char *name,long ticks)
{
    addTimeStat2(name,ticks,false);
}
using namespace std;
using namespace TimeAccounting;
#include "checkunit.h"
#ifdef XP_TESTING
namespace {
   
START_TEST(check_timer_count)
{
    TimeAccount t;
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    fail_unless(t.callCount()==4,"callCount failed");
}
END_TEST    

START_TEST(check_timer_reset)
{
    TimeAccount t;
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    t.add_call("aaa",100);
    t.reset();
    fail_unless(t.callCount()==0,"reset failed - count");
    int tv=t.totalTime();
    fail_unless(tv==0 ,"reset failed - time ");
}
END_TEST    

START_TEST(check_timer_report)
{
    TimeAccount t;
    const int  one_sec=sysconf(_SC_CLK_TCK);
    t.add_call("aaa",one_sec);
    t.add_call("aaa",one_sec);
    t.add_call("aaa",one_sec);
    t.add_call("aaa",one_sec);
    t.add_call("bbb",5*one_sec);
    t.add_call("bbb",5*one_sec);
    std::stringstream ss;
    t.report<ReqData::CompCalls>(1,&ss);
    int co;
    std::string name;
    int val;
    ss >> name >> co >> val;
    fail_unless(name=="aaa" && co==4 && val==4,"report<Calls> failed");
    std::stringstream ss2;
    t.report<ReqData::CompTimes>(1,&ss2);
    ss2 >> name >> co >> val;
    fail_unless(name=="bbb" && co==2 && val==10 ,"Report<Times> failed");
    fail_unless(t.totalTime()==14 ,"totalTime failed");
}
END_TEST    

START_TEST(check_timer)
{
    Timer<Times> t;
    sleep(2);
    t.stop();
    unsigned long t1 = t.value();
    sleep(2);
    fail_unless(t.value()==t1, "timer run after stop"); 
    fail_unless(t.value()!=0, "timer is zero"); 
}
END_TEST

class overflow_test
{
    static unsigned long value;
    public :
    static void setNext(unsigned long t)
    {
        value=t;
    }
    static unsigned long time()
    {
        return value;
    }
};
unsigned long overflow_test::value;

START_TEST(check_timer_overflow)
{
    unsigned long tv = 0x7FFFFFFF;
    overflow_test::setNext(tv);
    Timer<overflow_test> t;
    tv += 10;
    overflow_test::setNext(tv);
    t.stop();
    unsigned long t1 = t.value();
    fail_unless(t.value() == t1, "timer run after stop"); 
    fail_unless(t.value() == 10, "wrong overflow fix"); 
}
END_TEST
}

#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
  ADD_TEST(check_timer_count);
  ADD_TEST(check_timer_reset);
  ADD_TEST(check_timer_report);
  ADD_TEST(check_timer);
  ADD_TEST(check_timer_overflow);
TCASEFINISH
#endif //XP_TESTING

