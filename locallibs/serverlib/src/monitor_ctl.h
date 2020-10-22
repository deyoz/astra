#ifndef _MONITOR_CTL_
#define _MONITOR_CTL_
#include <tcl.h>
#include <signal.h>
#ifdef __cplusplus
#include <string>
extern "C" {
#endif
void addTimeStat(const char *name,long ticks);
void addTimeStatNoCount(const char *name,long ticks);

void check_risc_order();
/*вывести процесс из состояния работа + увеличить счетчик*/
void monitor_idle_zapr(int cnt);

/*перевести процесс в состояние работа*/
void monitor_beg_work();

/* информирует о работоспособности */
void monitor_working();

void monitor_working_zapr_type(int cnt, int type_zapr);

/*вывести процесс из состояния работа*/
void monitor_idle();

void monitor_idle_zapr_type(int cnt, int type_zapr);

/* установить код запроса, который сейчас обрабатывается процессом*/
void monitor_set_request(char *req);

/* очистить код запроса, который сейчас обрабатывается процессом */
/* (очистка производится автоматически при вызове monitor_idle() ) */
void monitor_clear_request(void);

/*попросить перезапустить процесс как можно раньше*/
void monitor_restart();
/*выставить признак - мы обычный обработчик*/
void monitor_regular();
/*выставить признак - мы демон*/
void monitor_special();
void monitor_archive();
/*странная функция нечеткого перезапуска -
 * см исходные тексты - артефакт */
void monitor_strange_restart();

/*функции работы с обработкой запросов в "фоне"*/

/*артефакт*/
int monitor_need_print_request(int base);

void set_monitor_timeout(int time);
/* возвращает установленный timeout */
int get_monitor_timeout();
 

/* ... to rsd_change*/
void notifyArchFtp(void);
void notifyArchDaemons(void);
void wake_up_airimps(void);
void send_signal(const char* addr, const void* buf, const size_t len);
int tcl_slave_mode(void);
/*flags in message*/

int do_not_redisplay(void);
int isXmlBatch(void);
void set_msg_type_and_timeout(int flag, unsigned  char timeout);
void set_timeout(unsigned  char timeout);
void tcl_mode_sleep(int socket,int s2,int *first,int seconds, void(*)(int len,char*));
void term3(int);
void under_gdb();
int our_signal(int sig, void(*f)(int),sigset_t sigs);
void set_signal(void(*f) (int));
void sigusr2(int sig);
int isSpecialProc(void);
int get_last_run_time(const char *file );
int mark_run_time(void);
int check_context_size(void);
int initC (int control,const char* sirenaBin);
void set_cur_grp( Tcl_Obj *grp);
void set_cur_grp_string( const char *s);
Tcl_Obj * current_group(void);
void initStackRandom(char c);
int write_number_to_file(const char *file,long num);
long get_number_from_file(const char *file,long def, long err);
#ifdef __cplusplus
}

std::string get_signalsock_name(Tcl_Interp *interp,Tcl_Obj * var1,Tcl_Obj *var2,int suff);
int makeSignalSocket(const std::string & name);
std::string current_group2();

#endif
#endif
