#ifndef _TIMING_H_
#define _TIMING_H_
#include <unistd.h>
#include <sys/times.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <vector>
#include <numeric>
#include <limits>
#include "simplelog.h"
namespace TimeAccounting{
struct TimeVal {
    int tv_sec, tv_usec;
    TimeVal(): tv_sec(0), tv_usec(0){}
    void addTicks(long t)
    {
        if (t<0) /*overflow*/
            return;
        t*=1000000;
        t/=sysconf(_SC_CLK_TCK);
        tv_sec+=t/1000000;
        tv_usec+=t%1000000;
        normalize();
    }
    void normalize()
    {
        if(tv_usec>1000000){
            tv_sec+=tv_usec/1000000;
            tv_usec=tv_usec%1000000;
        }
    }
    TimeVal operator+=(const TimeVal &p)
    {
        tv_sec+=p.tv_sec;
        tv_usec+=p.tv_usec;
        normalize();
        return *this;
    }
};

class Times {
    public:
    static long time()
    {
        struct tms tm1;
        return ::times(&tm1);
    }
};

template <typename TSOURCE > class Timer {
    bool started;
    unsigned long t;
    public:
    Timer() : started(true), t(TSOURCE::time()) {}
    Timer(bool started_) : started(started_), t(started ? TSOURCE::time() : 0) {}
    void start()
    {
        if (!started)
        {
            t = TSOURCE::time();
            started = true;
        }
    }
    void stop()
    {
        if (!started)
        {
            return;
        }
        unsigned long t2 = TSOURCE::time();

        if (t2 < t) /// overflow 
        {
            //Logger::getTracer().ProgTrace(0, "TIMING", __FILE__, __LINE__, "overflow %lx %lx", t, t2);
            t = t2 + (std::numeric_limits<unsigned long>::max() - t);
            //Logger::getTracer().ProgTrace(0, "TIMING", __FILE__, __LINE__, "overflow result %lu", t);
        }
        else
        {
            t = t2 - t;
        }
       started = false;
    }
    unsigned long value()
    {
        stop();
        return t;
    }
};

class ReqData {
        std::string name;
        TimeVal tv;
        int callcount; 
        
    public:
       struct reporter {
                std::stringstream *ps;
                reporter(std::stringstream *ss):ps(ss){}
                void operator()(const ReqData & r)
                {
                    if(ps)
                        *ps << r.report(true);
                    else
                        Logger::getTracer().ProgTrace(0,"TIMING",__FILE__,__LINE__,"%s",r.report().c_str());
                }
        };
        struct CompCalls {
                bool operator()(const ReqData &a, const ReqData &b) const;
        };
        struct  CompTimes {
                bool operator()(const ReqData &a, const ReqData &b) const;
        };
        struct  CompName {
                bool operator()(const ReqData &a, const ReqData &b) const;
        };
        friend struct CompName;
        friend struct CompTimes;
        friend struct CompCalls;
        ReqData(const char * n):name(n),callcount(0)
        {
        }
        void add(long t) 
        {
           callcount++; 
           tv.addTicks(t);
        }
        TimeVal getTime() const
        {
            return tv;
        }
        std::string report(bool test=false) const
        {
            std::stringstream s;
            if(test){
                s<<name<<" "<<callcount<<" " << tv.tv_sec;   
            }else{
            s<<name << " called " << callcount << " times," 
                << tv.tv_sec << " sec ";
            }
            return s.str();
        }
};
class TimeAccount
{
    std::set<ReqData,ReqData::CompName> s;
    int totcalls;
    public:
        TimeAccount():totcalls(0)
        {
        }
        template <typename T> void report(unsigned count,
                std::stringstream *ps=0)
        {
            std::vector<ReqData> v(s.begin(),s.end());
            if(count > v.size())
                count=v.size();

            std::partial_sort(v.begin(), v.begin()+count, v.end(), T());
            std::for_each(v.begin(), v.begin()+count, ReqData::reporter(ps));
        }
        
            
        void add_call(const char *name,long t, bool count=true){
            auto p=s.insert(ReqData(name)); 
            const_cast<ReqData &>(*p.first).add(t); //add don't touch key part
            if(count)
                totcalls++;
        }
        int callCount() const {
            return totcalls;
        }
        int totalTime() const {
            TimeVal tv;
            for(auto& r : s)
                tv += r.getTime();
            return tv.tv_sec;
        }
        void reset()
        {
            totcalls=0;
            s.clear();
        }
};

}


#endif
