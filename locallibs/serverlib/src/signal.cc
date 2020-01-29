#if HAVE_CONFIG_H
#endif

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "tclmon.h"

int set_sig(void (*p) (int), int sig)
{
    struct sigaction sa = {};
    sa.sa_handler = p;
    sa.sa_flags=SA_RESTART;
    if (sigaction (sig, &sa, 0) < 0) {
        fprintf (stderr, "sigaction failed %s for %d\n", strerror(errno), sig);
        return -1;
    }
    return 0;
}

int stopped_by_signal=0;

void finish (int s)
{
    printf ("parent terminate by %d\n", s);
    stopped_by_signal=1;
}

void finish_chld (int s)
{
    printf ("chld terminate by %d\n", s);
    exit (1);
}

