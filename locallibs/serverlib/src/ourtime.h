#ifndef _OURTIME_H
#define _OURTIME_H

#include <sys/types.h>
#include <sys/times.h>
#include <time.h>

struct ourtime
{
    time_t sec;
    clock_t clc;
    struct tm tm;
};

ourtime * getOurTime();
int InitLogTime(const char *s);

const char * user_log_head(int log, bool add_cc_mark);

#endif

