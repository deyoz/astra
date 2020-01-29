#ifdef __cplusplus
extern "C" {
#endif
#ifndef _PERFOM_H_
#define _PERFOM_H_

#include <unistd.h>
#define  MSec(a) (((a)*1000)/sysconf(_SC_CLK_TCK))

void PerfomInit(void);

void PerfomTest(int label);
void PerfomTest1(int label); // same as PerfomTest but TRACE1

// Returns working time till PerfomInit in microseconds
long int TimeElapsedMcSec();
// Returns working time of obrzap in miliseconds
int TimeElapsedMSec();
// Returns working time of obrzap in seconds
int TimeElapsed();

#endif
#ifdef __cplusplus
}
#endif
