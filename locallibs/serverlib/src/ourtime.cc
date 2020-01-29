#if HAVE_CONFIG_H
#endif

#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "tcl_utils.h"
#include "dates.h"
#include "ourtime.h"

using namespace std;

static char IdStr[20]="@@@@@@";
static ourtime OurTime;
static int cur_q_num;

const char * user_log_head(int log, bool add_cc_mark)
{
    static char head2[200];
    if (log == -10) {
        return IdStr;
    }
    if (log == -11) {
        snprintf(head2, sizeof(head2), "%s ", IdStr);
        return head2;
    }
    if (log == -12) {
        return "";
    }
    static int true_time_log = -1;

    if(true_time_log == -1)
    {
        if(readIntFromTcl("TRUE_TIME_LOG_STYLE", 0) != 0)
        {
            true_time_log = 1;
        }
        else
        {
            true_time_log = 0;
        }
    }

    int headlen = 0;
    if(true_time_log) {
        headlen = Dates::localtime_fullformat(head2, sizeof head2, "%02d%02d%02dT%02d%02d%02d.%06lu");
    } else {
        headlen = sprintf(head2, "%012lu", (long)(OurTime.sec ? OurTime.sec : time(0)));
    }

    snprintf(head2+headlen, sizeof(head2)-headlen, "+%02d %02d %s%.15s %07d ", cur_q_num, log, add_cc_mark ? "CC " : "", IdStr, getpid());
    return head2;
}

ourtime * getOurTime()
{
    return &OurTime;
}

int InitLogTime(const char *s)
{
        struct tms tms_s;
        struct tm *ptm;
        if(s)
        {
            cur_q_num=(cur_q_num+1)%100;
        }
        if(s)
        {
            strncpy(IdStr, s, 19);
        }
        else
        {
            strcpy(IdStr, "@@@@@@");
        }
        OurTime.sec=time(NULL);
        OurTime.clc=times(&tms_s);
        if(OurTime.sec == -1)
        {
            fprintf(stderr, "time failed: %s\n", strerror(errno));
            //OurT=NULL;
            return -1;
        }
        ptm=localtime(&OurTime.sec);
        if(!ptm)
        {
            fprintf(stderr, "localtime failed: %s\n", strerror(errno));
            //OurT=NULL;
            return -1;
        }    
        OurTime.tm=*ptm;            
        //OurT=ot;
        return 0;
}

