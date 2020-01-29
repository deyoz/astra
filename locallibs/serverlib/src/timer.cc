#if HAVE_CONFIG_H
#endif

/*
*  C++ Implementation: timer
*
* Description: Timer
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <iomanip>
#include "timer.h"

#define NICKNAME "TIMER"
#include "slogger.h"

namespace Timer
{

inline void currentTime(struct timeval &_Time)
{
    if(gettimeofday(&_Time, 0))
        LogWarning(STDLOG) << "gettimeofday failed to initialize";
}

inline double time_to_double(const struct timeval &NowTime)
{
    return NowTime.tv_sec + NowTime.tv_usec / 1000000.;
}

void timer::restart()
{
#ifdef USE_BOOST
    StartTime = boost::posix_time::microsec_clock::universal_time();
#else
    currentTime(StartTime);
#endif /*USE_BOOST*/
}

double timer::elapsed() const
{
#ifdef USE_BOOST
    using namespace boost::posix_time;
    ptime now = microsec_clock::universal_time();
    time_duration::tick_type elapsed = (now - StartTime).total_microseconds();
    return static_cast<double>(elapsed) / 1000000;
#else
    struct timeval NowTime;
    currentTime(NowTime);

    return time_to_double(NowTime) - time_to_double(StartTime);
#endif /*USE_BOOST*/
}

long timer::usec() const
{
#ifdef USE_BOOST
    using namespace boost::posix_time;
    ptime now = microsec_clock::universal_time();
    return (now - StartTime).total_microseconds();
#else
    struct timeval NowTime;
    currentTime(NowTime);
    return (NowTime.tv_sec - StartTime.tv_sec) * 1e6 + (NowTime.tv_usec - StartTime.tv_usec);
#endif /*USE_BOOST*/
}

std::ostream & operator <<(std::ostream & os, const timer &t)
{
    os << t.name() << (t.name().empty() ? "" : ": ")
       << std::setiosflags(std::ios::fixed) << std::setprecision(6)
       << t.elapsed() << "s";
    return os;
}

timer::timer()
{
    restart();
}

timer::timer(const std::string &name)
    :TName(name)
{
    restart();
}

}


#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"
#include "timing.h"

START_TEST( check_timer_timing )
{
    Timer::timer t;
    sleep(1);
    LogTrace(TRACE1) << t;
}
END_TEST

START_TEST( check_timer_perf )
{
    Timer::timer t;
    for(size_t i = 0; i < 1000000; i++) {
        Timer::timer t2;
        t2.elapsed();
    }

    LogTrace(TRACE1) << t;
    // 0.307353s - NO boost
    // 1.536118s - YES boost
}
END_TEST

START_TEST( check_timing_perf )
{
    Timer::timer t;
    TimeAccounting::Timer<TimeAccounting::Times> t1;
    t1.start();
    for(size_t i = 0; i < 1000000; i++) {
        TimeAccounting::Timer<TimeAccounting::Times> t2;
        t2.start();
        t2.value();
    }

    LogTrace(TRACE1) << t;
    LogTrace(TRACE1) << t1.value() / 100.;
    // 0.453916s
}
END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER( 0, 0 )
{
    ADD_TEST( check_timer_timing );
    ADD_TEST( check_timer_perf );
    ADD_TEST( check_timing_perf );
}
TCASEFINISH

#endif /*XP_TESTING*/
