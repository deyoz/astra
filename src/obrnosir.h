#ifndef _OBRNOSIR_H_
#define _OBRNOSIR_H_

int main_nosir_user(int argc,char **argv);
void help_nosir_user(void);
void nosir_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5);

#endif /*_OBRNOSIR_H_*/

