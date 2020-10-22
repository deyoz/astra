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
/*�뢥�� ����� �� ���ﭨ� ࠡ�� + 㢥����� ���稪*/
void monitor_idle_zapr(int cnt);

/*��ॢ��� ����� � ���ﭨ� ࠡ��*/
void monitor_beg_work();

/* ���ନ��� � ࠡ��ᯮᮡ���� */
void monitor_working();

void monitor_working_zapr_type(int cnt, int type_zapr);

/*�뢥�� ����� �� ���ﭨ� ࠡ��*/
void monitor_idle();

void monitor_idle_zapr_type(int cnt, int type_zapr);

/* ��⠭����� ��� �����, ����� ᥩ�� ��ࠡ��뢠���� ����ᮬ*/
void monitor_set_request(char *req);

/* ������ ��� �����, ����� ᥩ�� ��ࠡ��뢠���� ����ᮬ */
/* (���⪠ �ந�������� ��⮬���᪨ �� �맮�� monitor_idle() ) */
void monitor_clear_request(void);

/*������� ��१������� ����� ��� ����� ࠭��*/
void monitor_restart();
/*���⠢��� �ਧ��� - �� ����� ��ࠡ��稪*/
void monitor_regular();
/*���⠢��� �ਧ��� - �� �����*/
void monitor_special();
void monitor_archive();
/*��࠭��� �㭪�� ���⪮�� ��१���᪠ -
 * � ��室�� ⥪��� - ���䠪� */
void monitor_strange_restart();

/*�㭪樨 ࠡ��� � ��ࠡ�⪮� ����ᮢ � "䮭�"*/

/*���䠪�*/
int monitor_need_print_request(int base);

void set_monitor_timeout(int time);
/* �����頥� ��⠭������� timeout */
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
