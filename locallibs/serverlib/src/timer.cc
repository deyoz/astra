#include "timer.h"
#include <chrono>
#include <iostream>

namespace HelpCpp
{

Timer::Timer(bool startOnCreate)
{
    if (startOnCreate) {
        restart();
    }
}

Timer::Timer(const std::string& name, bool startOnCreate)
    : name_(name)
{
    if (startOnCreate) {
        restart();
    }
}

const std::string& Timer::name() const
{
    return name_;
}

void Timer::restart()
{
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    startTime_ = now_ms.time_since_epoch().count();
}

template<typename T>
static long elapsed(long start)
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<T>(now
            - std::chrono::time_point<std::chrono::system_clock>(std::chrono::microseconds(start))).count();
}

long Timer::elapsedSeconds() const
{
    return elapsed<std::chrono::seconds>(startTime_);
}

long Timer::elapsedMilliseconds() const
{
    return elapsed<std::chrono::milliseconds>(startTime_);
}

long Timer::elapsedMicroseconds() const
{
    return elapsed<std::chrono::microseconds>(startTime_);
}

std::ostream& operator<<(std::ostream& os, const Timer& t)
{
    if (t.name_.empty()) {
        return os << t.elapsedMilliseconds() << "ms";
    } else {
        return os << t.name_ << ' ' << t.elapsedMilliseconds() << "ms";
    }
}

} // namespace HelpCpp

#ifdef XP_TESTING
#include <thread>
#include "checkunit.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

namespace
{

START_TEST(check_timer)
{
    using namespace std::literals;
    HelpCpp::Timer t;
    std::this_thread::sleep_for(16ms);
    auto delay = t.elapsedMilliseconds();
    LogTrace(TRACE5) << delay << "ms";
    fail_unless(delay >= 16);
} END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_timer)
}
TCASEFINISH

} // namespace
#endif //XP_TESTING
