#ifndef _OBRNOSIR_H_
#define _OBRNOSIR_H_

#include "date_time.h"

using BASIC::date_time::TDateTime;

int main_nosir_user(int argc,char **argv);
void help_nosir_user(void);
void nosir_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5);
bool getDateRangeFromArgs(int argc, char **argv,
                          TDateTime& firstDate,
                          TDateTime& lastDate);

#endif /*_OBRNOSIR_H_*/

