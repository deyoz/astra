//
// C++ Interface: timer
//
// Description: Timer
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TIMER_H_
#define _TIMER_H_
#include <iosfwd>
#include <string>
#include <sys/time.h>

namespace Timer
{
class timer
{
    struct timeval StartTime;
    std::string TName;

public:
    /**
     * @brief no name timer constr
     */
    timer();
    /**
     * @brief named timer constr
     * @param name
     */
    timer(const std::string &name);

    /**
     * @brief timer name
     * @return
     */
    const std::string &name() const { return TName; }

    /**
     * @brief restarts timer
     */
    void restart();

    /**
     * @brief time elapsed in seconds since last restart or construct
     * @return
     */
    double elapsed() const;

    /**
     * @brief time elapsed in microseconds since last restart or construct
     * @return
     */
    long usec() const;
};
std::ostream & operator << (std::ostream& os, const timer &);
}
#endif /*_TIMER_H_*/
